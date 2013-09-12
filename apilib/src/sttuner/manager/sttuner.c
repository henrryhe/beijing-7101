/* ----------------------------------------------------------------------------
File Name: sttuner.c

Description: 

     This module handles the management functions for the API
    interface of STTUNER, these include Init, Open, Close and Term etc.


Copyright (C) 2006-2007 STMicroelectronics

History:
 date:	 
version: 
 author: 
comment: 

   
Reference:
    ST API Definition "TUNER Driver API" DVD-API-06
---------------------------------------------------------------------------- */


/* Includes ---------------------------------------------------------------- */

/* C libs */


#ifndef ST_OSLINUX
#include <string.h>                     
#endif
/* Standard includes */
#include "stlite.h"

/* STTUNER */
#include "sttbx.h"

#include "stevt.h"
#include "sttuner.h"                    

/* local to sttuner */
#include "util.h"       /* generic utility functions for sttuner */
#include "dbtypes.h"    /* data types for databases */
#include "sysdbase.h"   /* functions to accesss system data */
#ifndef STTUNER_MINIDRIVER
#include "ioarch.h"     /* IO control */
#include "drivers.h"    /* device driver (un)install top level handler */
#include "chip.h"
#include "swioctl.h"
#endif

/* satellite manager API (functions in this file exported via this header) */
#ifdef STTUNER_USE_SAT
 #include "satapi.h"     
#endif

/* cable manager API */
#ifdef STTUNER_USE_CAB
 #include "cabapi.h"
#endif

/* terrestrial manager API */
#if defined(STTUNER_USE_TER) || defined(STTUNER_USE_ATSC_VSB)
 #include "terapi.h"
#endif

/* other header for this file, contains exported functions that are not for sttuner.h  */
#include "stapitnr.h" 

/* private variables ------------------------------------------------------- */

/* Union to convert integer value to function pointer. This is to avoid compiler
 warnings about cast between function pointer and non-function object. */
typedef union 
{
    long int AsAddress;
    void (*NotifyFunction)(STTUNER_Handle_t Handle, STTUNER_EventType_t EventType, ST_ErrorCode_t Error);
} STTUNER_Fn_Lvalue_t;

/* driver revision */
ST_Revision_t Revision;



/* system initalization state, refers to things that have to be done ONCE only per system boot */
static BOOL STTUNER_Initalized = FALSE;


/* guard calls to functions that have to have serial access */
static semaphore_t *Lock_InitTermOpenClose;



/* local functions ------------------------------------------------------- */

ST_ErrorCode_t STTUNER_CheckOpenHandle(STTUNER_Handle_t Handle);
ST_ErrorCode_t STTUNER_FreeLists(U32 index);
ST_ErrorCode_t STTUNER_NoSem_Close(U32 index);
ST_ErrorCode_t STTUNER_PIOHandleInInstDbase(U32 index,U32 HandlePIO);


/* ----------------------------------------------------------------------------
Name: STTUNER_IsInitalized()

Description:
    check if initalized.
    
Parameters:
    
Return Value:
    
See Also:
   FALSE if not initalized otherwise TRUE.
---------------------------------------------------------------------------- */
BOOL STTUNER_IsInitalized(void)
{
    return(STTUNER_Initalized);
    
} /* STTUNER_IsInitalized() */



/* ----------------------------------------------------------------------------
Name: STTUNER_GetRevision()

Description:
    Obtains the revision number of the tuner device driver.
    
Parameters:
    None.
    
Return Value:
    Pointer to string containing the driver revision number.
    
See Also:
    Revision (variable).
---------------------------------------------------------------------------- */
ST_Revision_t STTUNER_GetRevision(void)
{
	Revision = "STTUNER-REL_6.2.6";
    return(Revision);
    
} /* STTUNER_GetRevision() */


/* ----------------------------------------------------------------------------
Name: STTUNER_GetLLARevision()

Description:
    Obtains the revision number of the tuner LLA device driver.
    
Parameters:
    demodname eg:STTUNER_DEMOD_STV0360,STTUNER_DEMOD_STV0299,STTUNER_DEMOD_STV0399,
    		 STTUNER_DEMOD_STV0297J,STTUNER_DEMOD_STV0297,STTUNER_DEMOD_STV0297E,STTUNER_DEMOD_STV0498,STTUNER_DEMOD_STV0361,
    		 STTUNER_DEMOD_STV0362,STTUNER_ALL_IN_CURRENT_BUILD(for getting versions of all in current build).
---------------------------------------------------------------------------- */
#ifndef STTUNER_MINIDRIVER
ST_Revision_t STTUNER_GetLLARevision(int demodtype)
{
	extern ST_Revision_t STTUNER_DRV_GetLLARevision(int);
	char temp1[50];/*max length of string describing LLA version=49+1(for '\0');total 5 demods,therefore 5*50=250*/
	char * LLARevision1=temp1;
	memset(LLARevision1,'\0',sizeof(temp1));
  strcpy(LLARevision1, STTUNER_DRV_GetLLARevision(demodtype));
	return (LLARevision1);	
} /* STTUNER_GetLLARevision() */
#endif


/* ----------------------------------------------------------------------------
Name: STTUNER_Init()

Description:
    Initializes a media device.
    
Parameters:
    DeviceName  STTUNER global instance name device name as set during initialization.
   *InitParams  pointer to initialization parameters.
    
Return Value:
    ST_NO_ERROR,                    the operation was carried out without error.
    ST_ERROR_NO_MEMORY,             memory allocation fail.
    
See Also:
    STTUNER_Term()
---------------------------------------------------------------------------- */

ST_ErrorCode_t STTUNER_Init(ST_DeviceName_t DeviceName, STTUNER_InitParams_t *InitParams_p)
{
#ifdef STTUNER_DEBUG_USERSPACE_TO_KERNEL
  printk("STTUNER_TEST: ------Display the STTUNER_InitParams_t ----- \n");
  printk("STTUNER_TEST: Device is %d\n",InitParams_p->Device);
  printk("STTUNER_TEST: Max_BandList is %d\n",InitParams_p->Max_BandList);
  printk("STTUNER_TEST: DemodType is %d\n",InitParams_p->DemodType);
  printk("STTUNER_TEST: DemodIO->DriverName is %s\n",InitParams_p->DemodIO.DriverName);
  printk("STTUNER_TEST: FECMode is %d\n",InitParams_p->FECMode);
  #ifdef STTUNER_USE_SAT
  printk("STTUNER_TEST: LnbType is %d\n",InitParams_p->LnbType);
  printk("STTUNER_TEST: LnbIO->DriverName is %s\n",InitParams_p->LnbIO.DriverName);
  #endif
#endif  /* STTUNER_DEBUG_USERSPACE_KERNE */

#ifdef STTUNER_DEBUG_MODULE_STTUNER
    const char *identity = "STTUNER sttuner.c STTUNER_Init()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
   
    U32 index;
    STTUNER_InstanceDbase_t    *Inst;
    
#ifndef STTUNER_MINIDRIVER
    DRIVERS_InstallParams_t DriversInstallParams;
    /* ---------- check InitParams_p ---------- */
    Error = VALIDATE_InitParams(InitParams_p);
    if( Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s fail Initparams not valid\n", identity));
#endif
        return(Error);
    }
    
    Inst = STTUNER_GetDrvInst();


    if (STTUNER_Initalized == FALSE)
    {

#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s setup driver database..\n", identity));
#endif
        /* setup driver databases */
        DriversInstallParams.MemoryPartition = InitParams_p->DriverPartition;
        DriversInstallParams.TunerType = InitParams_p->TunerType;


        Error = STTUNER_DRV_Install(&DriversInstallParams);
        if (Error != ST_NO_ERROR)
        {
            
#ifdef STTUNER_DEBUG_MODULE_STTUNER
            STTBX_Print(("fail setup driver database Error=%d\n", Error));
#endif
            return(Error);
        }

        /* clean handles */
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s setup handles...", identity));
#endif
        for(index = 0; index < STTUNER_MAX_HANDLES; index++)
        {
            Inst[index].Device = STTUNER_DEVICE_NONE;
            Inst[index].Status = STTUNER_DASTATUS_FREE;
            memset(Inst[index].Name, '\0', ST_MAX_DEVICE_NAME);
        }
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("ok\n"));
#endif  

        /* init IO manager (ok if already initalized) */
        Error = ChipInit();
        if ((Error != ST_NO_ERROR) && (Error != STTUNER_ERROR_INITSTATE))
        {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
            STTBX_Print(("%s fail init I/O manager\n", identity));
#endif    
            return(Error);
        }

        /* done */  
        STTUNER_Initalized = TRUE;

        Lock_InitTermOpenClose = STOS_SemaphoreCreateFifo(NULL,1);


    }   /* if (not STTUNER_Initalized) */


    SEM_LOCK(Lock_InitTermOpenClose);


    /* ---------- check that the name is unique ---------- */
    Error = VALIDATE_IsUniqueName(DeviceName);
    if( Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s fail Initparams not valid\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    /* ---------- find a free instance ---------- */
    Inst = STTUNER_GetDrvInst();
    index = 0;
    while(Inst[index].Status != STTUNER_DASTATUS_FREE)
    {
        index++;
        if (index >= STTUNER_MAX_HANDLES)
        {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
            STTBX_Print(("%s no free handles\n", identity));
#endif
            SEM_UNLOCK(Lock_InitTermOpenClose);
            return(ST_ERROR_NO_FREE_HANDLES);
        }
    }
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s handle index=%d\n", identity, index));
#endif


    /* ---------- clean out instance ---------- */

    memset(&Inst[index].Capability,       '\0', sizeof(STTUNER_Capability_t));
    memset(&Inst[index].TunerInfo,        '\0', sizeof(STTUNER_TunerInfo_t));
    memset(&Inst[index].CurrentTunerInfo, '\0', sizeof(STTUNER_TunerInfo_t));
    memset(&Inst[index].QualityFormat,    '\0', sizeof(STTUNER_QualityFormat_t));


    /* ---------- populate instance params  (common) ---------- */

    strcpy(Inst[index].Name, DeviceName);

    Inst[index].Device = InitParams_p->Device;

    Inst[index].Max_BandList      = InitParams_p->Max_BandList;
    Inst[index].Max_ThresholdList = InitParams_p->Max_ThresholdList;
    Inst[index].Max_ScanList      = InitParams_p->Max_ScanList;
    
 
    strcpy(Inst[index].EVT_DeviceName,     InitParams_p->EVT_DeviceName);
    strcpy(Inst[index].EVT_RegistrantName, InitParams_p->EVT_RegistrantName);

    Inst[index].DriverPartition = InitParams_p->DriverPartition;
   
   

    /* ---------- setup lists ---------- */

#ifdef STTUNER_DEBUG_MODULE_STTUNER
    STTBX_Print(("%s setup lists\n", identity));
#endif

    Inst[index].BandList.NumElements      = 0;
    Inst[index].ThresholdList.NumElements = 0;
    Inst[index].ScanList.NumElements      = 0;

    /* size of ThresholdHits list == size of SignalListSize */
    Inst[index].ThresholdHits = STOS_MemoryAllocateClear(Inst[index].DriverPartition, Inst[index].Max_ThresholdList, sizeof(U32));

    if (Inst[index].ThresholdHits == NULL)  
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s fail memory allocation ThresholdHits\n", identity));
#endif    
        STTUNER_FreeLists(index);
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_NO_MEMORY);           
    }


    /*  allocate memory for band  */
    Inst[index].BandList.BandList = STOS_MemoryAllocateClear(Inst[index].DriverPartition, Inst[index].Max_BandList, sizeof(STTUNER_Band_t));

    if (Inst[index].BandList.BandList == NULL)  
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s fail memory allocation BandList\n", identity));
#endif    
        STTUNER_FreeLists(index);
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_NO_MEMORY);           
    }


    /*  allocate memory for signal threshold  */
    Inst[index].ThresholdList.ThresholdList = STOS_MemoryAllocateClear(Inst[index].DriverPartition, Inst[index].Max_ThresholdList, sizeof(STTUNER_SignalThreshold_t));

    if (Inst[index].ThresholdList.ThresholdList == NULL)  
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s fail memory allocation ThresholdList\n", identity));
#endif    
        STTUNER_FreeLists(index);
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_NO_MEMORY);           
    }

    /*  allocate memory for scan */
    Inst[index].ScanList.ScanList = STOS_MemoryAllocateClear(Inst[index].DriverPartition,Inst[index].Max_ScanList, sizeof(STTUNER_Scan_t));
    if (Inst[index].ScanList.ScanList == NULL)  
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s fail memory allocation ScanDbase\n", identity));
#endif    
        STTUNER_FreeLists(index);
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_NO_MEMORY);           
    }
   
    /******** To setup Standby Mode variable ***********/
    Inst[index].Task_Run=0;
  
    /* ---------- initalize an instance of a specific device ---------- */
    switch(Inst[index].Device)
    {

        case STTUNER_DEVICE_SATELLITE:
#ifdef STTUNER_USE_SAT
            Error = STTUNER_SAT_Init(index, InitParams_p);
#else
            Error = STTUNER_ERROR_ID;
#endif
            break;

        case STTUNER_DEVICE_CABLE:
#ifdef STTUNER_USE_CAB
            Error = STTUNER_CABLE_Init(index, InitParams_p);
#else
            Error = STTUNER_ERROR_ID;
#endif
            break;

        case STTUNER_DEVICE_TERR:
#if defined(STTUNER_USE_TER) || defined(STTUNER_USE_ATSC_VSB)
            Error = STTUNER_TERR_Init(index, InitParams_p);
#else
            Error = STTUNER_ERROR_ID;
#endif
            break;


        default:
#ifdef STTUNER_DEBUG_MODULE_STTUNER
            STTBX_Print(("%s no valid device %d\n", identity, Inst[index].Device));
#endif
            Error = STTUNER_ERROR_ID;
            break;
                    
    } /* switch */
#endif

#ifdef STTUNER_MINIDRIVER
            index = 0;
            Inst = STTUNER_GetDrvInst();
            Inst[index].Device = STTUNER_DEVICE_NONE;
            Inst[index].Status = STTUNER_DASTATUS_FREE;
            memset(Inst[index].Name, '\0', ST_MAX_DEVICE_NAME);
	    STTUNER_Initalized = TRUE;
               
#if defined(ST_OS21) || defined(ST_OSLINUX)
        Lock_InitTermOpenClose = semaphore_create_fifo(1);
#else
        semaphore_init_fifo(&Lock_InitTermOpenClose, 1);
#endif

 
    SEM_LOCK(Lock_InitTermOpenClose);

   /* ---------- clean out instance ---------- */

    memset(&Inst[index].Capability,       '\0', sizeof(STTUNER_Capability_t));
    memset(&Inst[index].TunerInfo,        '\0', sizeof(STTUNER_TunerInfo_t));
    memset(&Inst[index].CurrentTunerInfo, '\0', sizeof(STTUNER_TunerInfo_t));
    memset(&Inst[index].QualityFormat,    '\0', sizeof(STTUNER_QualityFormat_t));


    /* ---------- populate instance params  (common) ---------- */

    strcpy(Inst[index].Name, DeviceName);

    Inst[index].Device = InitParams_p->Device;

    Inst[index].Max_BandList      = InitParams_p->Max_BandList;
   
    strcpy(Inst[index].EVT_DeviceName,     InitParams_p->EVT_DeviceName);
    strcpy(Inst[index].EVT_RegistrantName, InitParams_p->EVT_RegistrantName);

    Inst[index].DriverPartition = InitParams_p->DriverPartition;
   
   

    /* ---------- setup lists ---------- */

#ifdef STTUNER_DEBUG_MODULE_STTUNER
    STTBX_Print(("%s setup lists\n", identity));
#endif

    Inst[index].BandList.NumElements      = 0;
    
    
    /*  allocate memory for band  */
    Inst[index].BandList.BandList = STOS_MemoryAllocateClear(Inst[index].DriverPartition, Inst[index].Max_BandList, sizeof(STTUNER_Band_t));

    if (Inst[index].BandList.BandList == NULL)  
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s fail memory allocation BandList\n", identity));
#endif    
        STTUNER_FreeLists(index);
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_NO_MEMORY);           
    }
#ifdef STTUNER_USE_SAT
  Error = STTUNER_SAT_Init(index, InitParams_p);
#endif  

#ifdef STTUNER_USE_TER
  Error = STTUNER_TERR_Init(index, InitParams_p);
#endif
#endif

    /* if all went well record that this instance has been initalized and ready for opening */
    if (Error == ST_NO_ERROR)
    {
        Inst[index].Status = STTUNER_DASTATUS_INIT;
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s initalized ok\n", identity));
#endif    
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s fail initalization\n", identity));
#endif    
    }

    SEM_UNLOCK(Lock_InitTermOpenClose);
    return(Error);
    
} /* STTUNER_Init() */



/* ----------------------------------------------------------------------------
Name: STTUNER_Term()
Description:
    Terminates ALL tuner drivers.  If a force terminate is required, then
    the open handle will be closed also.
Parameters:
    DeviceName, the tuner device name as set during initialization.
    TermParams_p, parameters to guide termination of the driver.
Return Value:
    ST_NO_ERROR,                the operation was carried out without error.
    ST_ERROR_UNKNOWN_DEVICE     the device name is invalid.
    ST_ERROR_OPEN_HANDLE,       a handle is still open -- unable to term.
    ST_ERROR_BAD_PARAMETER      TermParams_p NULL 
See Also:
    STTUNER_Init()
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_Term(ST_DeviceName_t DeviceName, const STTUNER_TermParams_t *TermParams_p)
{
#ifdef STTUNER_DEBUG_USERSPACE_TO_KERNEL
  printk("STTUNER_TEST: ------Display the STTUNER_TermParams_t ----- \n");
  printk("STTUNER_TEST: ForceTerminate is %d\n",TermParams_p->ForceTerminate);
#endif

#ifdef STTUNER_DEBUG_MODULE_STTUNER
    const char *identity = "STTUNER STTUNER_Term.c()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_InstanceDbase_t    *Inst;
    U32 index;

#ifndef STTUNER_MINIDRIVER
    if (STTUNER_Initalized == FALSE)
    {

#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s nothing initalized\n", identity));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }
     /* ---------- now safe to lock ---------- */
    SEM_LOCK(Lock_InitTermOpenClose);
     /* ---------- find handle from name ---------- */
    Inst  = STTUNER_GetDrvInst();
    index = 0;

#ifdef STTUNER_DEBUG_MODULE_STTUNER
    STTBX_Print(("%s looking (%s) ", identity, Inst[index].Name));
#endif
    while(strcmp(Inst[index].Name, DeviceName) != 0)
    {
        index++;
        if (index >= STTUNER_MAX_HANDLES)
        {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
            STTBX_Print(("\n%s fail no block named '%s' before end of list\n", identity, DeviceName));
#endif
            SEM_UNLOCK(Lock_InitTermOpenClose);
            return(ST_ERROR_BAD_PARAMETER);
        }
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("(%s)", Inst[index].Name));
#endif
    }    
#ifdef STTUNER_DEBUG_MODULE_STTUNER
    STTBX_Print(("\n"));
#endif
    /* ---------- close handle if forced to ---------- */
    if ( (Inst[index].Status == STTUNER_DASTATUS_OPEN) && (TermParams_p->ForceTerminate == TRUE) )
    {
        Error = STTUNER_NoSem_Close(index);
        if (Error != ST_NO_ERROR)
        {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
            STTBX_Print(("%s fail close() device Error=%d\n", identity, Error));
#endif    
            SEM_UNLOCK(Lock_InitTermOpenClose);
            return(Error);           
        }
    }

    /* ----------test handle is closed ---------- */
    if ((Inst[index].Status != STTUNER_DASTATUS_CLOSE) && (Inst[index].Status != STTUNER_DASTATUS_INIT))
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s fail Status is not STTUNER_DASTATUS_CLOSE or STTUNER_DASTATUS_INIT for handle %d\n", identity, index));
#endif    
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_OPEN_HANDLE);           
    }

    /* ---------- term the instance of a specific device ---------- */
    switch(Inst[index].Device)
    {

        case STTUNER_DEVICE_SATELLITE:
#ifdef STTUNER_USE_SAT
            Error = STTUNER_SAT_Term(index, TermParams_p);
#else
            Error = STTUNER_ERROR_ID;
#endif
            break;

        case STTUNER_DEVICE_CABLE:
#ifdef STTUNER_USE_CAB
            Error = STTUNER_CABLE_Term(index, TermParams_p);
#else
            Error = STTUNER_ERROR_ID;
#endif
            break;

        case STTUNER_DEVICE_TERR:
#if defined(STTUNER_USE_TER) || defined(STTUNER_USE_ATSC_VSB)
            Error = STTUNER_TERR_Term(index, TermParams_p);
#else
            Error = STTUNER_ERROR_ID;
#endif
            break;


        default:
#ifdef STTUNER_DEBUG_MODULE_STTUNER
            STTBX_Print(("%s no valid device %d\n", identity, Inst[index].Device));
#endif
            Error = STTUNER_ERROR_ID;
            break;
                    
    } /* switch */
#endif

#ifdef STTUNER_MINIDRIVER
   /* ---------- now safe to lock ---------- */
    SEM_LOCK(Lock_InitTermOpenClose);
    /* ---------- find handle from name ---------- */
    Inst  = STTUNER_GetDrvInst();
    index = 0;
    /* ---------- close handle if forced to ---------- */
    if ( (Inst[index].Status == STTUNER_DASTATUS_OPEN) && (TermParams_p->ForceTerminate == TRUE) )
    {
        Error = STTUNER_NoSem_Close(index);
        if (Error != ST_NO_ERROR)
        {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
            STTBX_Print(("%s fail close() device Error=%d\n", identity, Error));
#endif    
            SEM_UNLOCK(Lock_InitTermOpenClose);
            return(Error);           
        }
    }
    /* ----------test handle is closed ---------- */
    if ((Inst[index].Status != STTUNER_DASTATUS_CLOSE) && (Inst[index].Status != STTUNER_DASTATUS_INIT))
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s fail Status is not STTUNER_DASTATUS_CLOSE or STTUNER_DASTATUS_INIT for handle %d\n", identity, index));
#endif    
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_OPEN_HANDLE);           
    }
    /* ---------- term the instance of a specific device ---------- */

#ifdef STTUNER_USE_SAT
Error = STTUNER_SAT_Term(index, TermParams_p);
#endif

#ifdef STTUNER_USE_TER
Error = STTUNER_TERR_Term(index, TermParams_p);
#endif

            
#endif

	

    STTUNER_FreeLists(index);

    Inst[index].Max_BandList      = 0;
    Inst[index].Max_ThresholdList = 0;
    Inst[index].Max_ScanList      = 0;

    memset(Inst[index].Name, '\0', ST_MAX_DEVICE_NAME);
    memset(Inst[index].EVT_DeviceName,     '\0', ST_MAX_DEVICE_NAME);
    memset(Inst[index].EVT_RegistrantName, '\0', ST_MAX_DEVICE_NAME);

    Inst[index].DriverPartition = NULL;
    Inst[index].Device = STTUNER_DEVICE_NONE;
    Inst[index].Status = STTUNER_DASTATUS_FREE;

    /* maybe a TODO: if all handles free then close I/O manager (freeing PIO pins for I2C) */



  index = 0;
    while(index < STTUNER_MAX_HANDLES)
    {
        if (Inst[index].Status != STTUNER_DASTATUS_FREE)
        {
         SEM_UNLOCK(Lock_InitTermOpenClose);
            break;
            }
        else
        {
            index++;        
            }
    }


if (index >= STTUNER_MAX_HANDLES)
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s all free handles\n", identity));
#endif


Error = ChipTerm();
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s fail close I/O Error=%d\n", identity, Error));
#endif
    }


 Error = STTUNER_DRV_UnInstall();
        if (Error != ST_NO_ERROR)
        {
            
#ifdef STTUNER_DEBUG_MODULE_STTUNER
            STTBX_Print(("fail uninstall driver database Error=%d\n", Error));
#endif
            SEM_UNLOCK(Lock_InitTermOpenClose);
            return(Error);
        }
 SEM_UNLOCK(Lock_InitTermOpenClose);
    STOS_SemaphoreDelete(NULL,Lock_InitTermOpenClose);
STTUNER_Initalized = FALSE;
}

    return(Error);           
     

} /* STTUNER_Term() */



/* ----------------------------------------------------------------------------
Name: STTUNER_Open()

Description:
    Initializes a media device.
    
Parameters:
   *InitParams  pointer to initialization parameters.
    
Return Value:
    ST_NO_ERROR,                    the operation was carried out without error.
    ST_ERROR_NO_MEMORY,             memory allocation fail.
    
See Also:
    STTUNER_Close()
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_Open(const ST_DeviceName_t DeviceName, const STTUNER_OpenParams_t *OpenParams_p, STTUNER_Handle_t *Handle_p)
{
#ifdef STTUNER_DEBUG_USERSPACE_TO_KERNEL
  printk("STTUNER_TEST: ------Display the STTUNER_OpenParams_t ----- \n");
  printk("STTUNER_TEST: DeviceName is %s\n",DeviceName);
#endif

#ifdef STTUNER_DEBUG_MODULE_STTUNER
    const char *identity = "STTUNER sttuner.c STTUNER_Open()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_InstanceDbase_t    *Inst;
   
#ifdef STTUNER_DEBUG_MODULE_STTUNER
    STTUNER_Fn_Lvalue_t aFunction;
#endif
    U32 index;

#ifndef STTUNER_MINIDRIVER
 STEVT_OpenParams_t EVTOpenParams;
    if (STTUNER_Initalized == FALSE)
    {

#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s nothing initalized\n", identity));
#endif
        return(ST_ERROR_UNKNOWN_DEVICE);
    }


    /* ---------- now safe to lock ---------- */
    SEM_LOCK(Lock_InitTermOpenClose);

     /* ---------- find handle from name ---------- */
    Inst  = STTUNER_GetDrvInst();
    index = 0;
#ifdef STTUNER_DEBUG_MODULE_STTUNER
    STTBX_Print(("%s looking (%s) ", identity, Inst[index].Name));
#endif
    while(strcmp(Inst[index].Name, DeviceName) != 0)
    {
        index++;
        if (index >= STTUNER_MAX_HANDLES)
        {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
            STTBX_Print(("\n%s fail no block named '%s' before end of list\n", identity, DeviceName));
#endif
            SEM_UNLOCK(Lock_InitTermOpenClose);
            return(ST_ERROR_UNKNOWN_DEVICE);
        }
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("(%s)", Inst[index].Name));
#endif
    }    
#ifdef STTUNER_DEBUG_MODULE_STTUNER
    STTBX_Print(("\n"));
#endif


    /* ---------- make sure state is initalized ---------- */
    if (Inst[index].Status != STTUNER_DASTATUS_INIT)
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s fail Status is not STTUNER_DASTATUS_INIT for handle %d\n", identity, index));
#endif    
        SEM_UNLOCK(Lock_InitTermOpenClose);

        if (Inst[index].Status == STTUNER_DASTATUS_OPEN)    /* handle already opened */
        {
            return(ST_ERROR_OPEN_HANDLE);
        }
        else if (Inst[index].Status == STTUNER_DASTATUS_CLOSE)
        {
          /* Do Nothing 
             This else if is added to solve the bug GNBvd21356 */	
        }	
        else
        {
            return(ST_ERROR_UNKNOWN_DEVICE);
        }
    }

    /* ---------- open an instance of STEVT? ---------- */

    if(OpenParams_p->NotifyFunction == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s EVT_RegistrantName: (%s) EVT_DeviceName: (%s) for handle #%d\n", identity, Inst[index].EVT_RegistrantName, Inst[index].EVT_DeviceName, index));
#endif

        Error = STEVT_Open(Inst[index].EVT_DeviceName, &EVTOpenParams, &Inst[index].EVTHandle);

        if (Error == ST_NO_ERROR)
        {
           Error |= STEVT_RegisterDeviceEvent(Inst[index].EVTHandle, Inst[index].EVT_RegistrantName, STTUNER_EV_LOCKED,        &Inst[index].EvtId[EventToId(STTUNER_EV_LOCKED)]        );
           Error |= STEVT_RegisterDeviceEvent(Inst[index].EVTHandle, Inst[index].EVT_RegistrantName, STTUNER_EV_UNLOCKED,      &Inst[index].EvtId[EventToId(STTUNER_EV_UNLOCKED)]      );
           Error |= STEVT_RegisterDeviceEvent(Inst[index].EVTHandle, Inst[index].EVT_RegistrantName, STTUNER_EV_SCAN_FAILED,   &Inst[index].EvtId[EventToId(STTUNER_EV_SCAN_FAILED)]   );
           Error |= STEVT_RegisterDeviceEvent(Inst[index].EVTHandle, Inst[index].EVT_RegistrantName, STTUNER_EV_TIMEOUT,       &Inst[index].EvtId[EventToId(STTUNER_EV_TIMEOUT)]       );
           Error |= STEVT_RegisterDeviceEvent(Inst[index].EVTHandle, Inst[index].EVT_RegistrantName, STTUNER_EV_SIGNAL_CHANGE, &Inst[index].EvtId[EventToId(STTUNER_EV_SIGNAL_CHANGE)] );
           Error |= STEVT_RegisterDeviceEvent(Inst[index].EVTHandle, Inst[index].EVT_RegistrantName, STTUNER_EV_SCAN_NEXT,     &Inst[index].EvtId[EventToId(STTUNER_EV_SCAN_NEXT)]     );
           Error |= STEVT_RegisterDeviceEvent(Inst[index].EVTHandle, Inst[index].EVT_RegistrantName, STTUNER_EV_LNB_FAILURE,     &Inst[index].EvtId[EventToId(STTUNER_EV_LNB_FAILURE)]     );
		
            if (Error != ST_NO_ERROR)
            {
                STEVT_Close(Inst[index].EVTHandle); /* Close handle to EVT */
#ifdef STTUNER_DEBUG_MODULE_STTUNER
                STTBX_Print(("%s fail STEVT registrations\n", identity));
#endif
                SEM_UNLOCK(Lock_InitTermOpenClose);
                return(Error);
            }

        }
        else
        {
            /* Event open failed */
#ifdef STTUNER_DEBUG_MODULE_STTUNER
            STTBX_Print(("%s fail STEVT open()\n", identity));
#endif
            SEM_UNLOCK(Lock_InitTermOpenClose);
            return(Error);
        }
    }

    /* ---------- open an instance of a specific device ---------- */
    switch(Inst[index].Device)
    {

        case STTUNER_DEVICE_SATELLITE:
#ifdef STTUNER_USE_SAT
            Error = STTUNER_SAT_Open(index, OpenParams_p);
#else
            Error = STTUNER_ERROR_ID;
#endif
            break;

        case STTUNER_DEVICE_CABLE:
#ifdef STTUNER_USE_CAB
            Error = STTUNER_CABLE_Open(index, OpenParams_p);
#else
            Error = STTUNER_ERROR_ID;
#endif
            break;

        case STTUNER_DEVICE_TERR:
#if defined(STTUNER_USE_TER) || defined(STTUNER_USE_ATSC_VSB)
            Error = STTUNER_TERR_Open(index, OpenParams_p);
#else
            Error = STTUNER_ERROR_ID;
#endif
            break;


        default:
#ifdef STTUNER_DEBUG_MODULE_STTUNER
            STTBX_Print(("%s no valid device %d\n", identity, Inst[index].Device));
#endif
            Error = STTUNER_ERROR_ID;
            break;
                    
    } /* switch */

#endif

#ifdef STTUNER_MINIDRIVER
/* ---------- now safe to lock ---------- */
    SEM_LOCK(Lock_InitTermOpenClose);
    /* ---------- find handle from name ---------- */
    Inst  = STTUNER_GetDrvInst();
    index = 0;

    /* ---------- make sure state is initalized ---------- */
    if (Inst[index].Status != STTUNER_DASTATUS_INIT)
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s fail Status is not STTUNER_DASTATUS_INIT for handle %d\n", identity, index));
#endif    
        

        if (Inst[index].Status == STTUNER_DASTATUS_OPEN)    /* handle already opened */
        {
            SEM_UNLOCK(Lock_InitTermOpenClose);
            return(ST_ERROR_OPEN_HANDLE);
        }
        else if (Inst[index].Status == STTUNER_DASTATUS_CLOSE)
        {
          /* Do Nothing 
             This else if is added to solve the bug GNBvd21356 */	
        }	
        else
        {
            SEM_UNLOCK(Lock_InitTermOpenClose);
            return(ST_ERROR_UNKNOWN_DEVICE);
        }
    }
    
#ifdef STTUNER_USE_SAT
Error = STTUNER_SAT_Open(index, OpenParams_p);
#endif

#ifdef STTUNER_USE_TER
Error = STTUNER_TERR_Open(index, OpenParams_p);
#endif


#endif
    if (Error == ST_NO_ERROR)
    {
        Inst[index].Status         = STTUNER_DASTATUS_OPEN;
        Inst[index].NotifyFunction = OpenParams_p->NotifyFunction;
        *Handle_p                  = INDEX2HANDLE( index );
        #ifndef STTUNER_MINIDRIVER
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        if(OpenParams_p->NotifyFunction != NULL)
        {
            aFunction.NotifyFunction = OpenParams_p->NotifyFunction;
            STTBX_Print(("%s NotifyFunction at 0x%08x setup ok\n", identity, aFunction.AsAddress));
        }
        else
        {
            STTBX_Print(("%s NotifyFunction NOT setup (using STEVT) ok\n", identity));
        }    
        STTBX_Print(("%s opened ok\n", identity));
#endif    
    }
    else
    {
        if(OpenParams_p->NotifyFunction == NULL)
        {
            STEVT_Close(Inst[index].EVTHandle); /* Close handle to EVT */
#ifdef STTUNER_DEBUG_MODULE_STTUNER
            STTBX_Print(("%s EVT closed.\n", identity));
#endif    
        }
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s fail open\n", identity));
#endif    

#endif
    }

 
    SEM_UNLOCK(Lock_InitTermOpenClose);
    return(Error);
    
} /* STTUNER_Open() */



/* ----------------------------------------------------------------------------
Name: STTUNER_Close()

Description:
    close instance of driver
    
Parameters:
    
Return Value:
    ST_NO_ERROR,                    the operation was carried out without error.
    
See Also:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_Close(STTUNER_Handle_t Handle)
{
#ifdef STTUNER_DEBUG_USERSPACE_TO_KERNEL
  printk("STTUNER_TEST: ------Display the STTUNER_OpenParams_t ----- \n");
  printk("STTUNER_TEST: Handle is %d\n", Handle);
#endif

#ifdef STTUNER_DEBUG_MODULE_STTUNER
    const char *identity = "STTUNER sttuner.c STTUNER_Close()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

    /* ---------- check handle is open ---------- */
    Error = STTUNER_CheckOpenHandle(Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s fail Status not STTUNER_DASTATUS_OPEN or bad handle(%d)\n", identity, Handle));
#endif
        return(Error);
    }

    /* ---------- now safe to lock ---------- */
    SEM_LOCK(Lock_InitTermOpenClose);

    Error = STTUNER_NoSem_Close( HANDLE2INDEX(Handle) );

    SEM_UNLOCK(Lock_InitTermOpenClose);
    return(Error);
    
} /* STTUNER_Close() */



/* ----------------------------------------------------------------------------
Name: STTUNER_NoSem_Close()

Description:
    close instance of driver
    
Parameters:
    
Return Value:
    ST_NO_ERROR,    the operation was carried out without error.
    
See Also:   STTUNER_Term(), STTUNER_Close()
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_NoSem_Close(U32 index)
{
#ifdef STTUNER_DEBUG_MODULE_STTUNER
    const char *identity = "STTUNER sttuner.c STTUNER_NoSem_Close()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_InstanceDbase_t    *Inst;

    Inst = STTUNER_GetDrvInst();
#ifndef STTUNER_MINIDRIVER
    /* ---------- close this instance of STEVT ---------- */
    Error = STEVT_Close(Inst[index].EVTHandle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s fail STEVT close()\n", identity));
#endif
    }   /* keep going */
    /* ---------- close instance of device ---------- */
    switch(Inst[index].Device)
    {

        case STTUNER_DEVICE_SATELLITE:
#ifdef STTUNER_USE_SAT
            Error = STTUNER_SAT_Close(index);
#else
            Error = STTUNER_ERROR_ID;
#endif
            break;

        case STTUNER_DEVICE_CABLE:
#ifdef STTUNER_USE_CAB
            Error = STTUNER_CABLE_Close(index);
#else
            Error = STTUNER_ERROR_ID;
#endif
            break;

        case STTUNER_DEVICE_TERR:
#if defined(STTUNER_USE_TER) || defined(STTUNER_USE_ATSC_VSB)
            Error = STTUNER_TERR_Close(index);
#else
            Error = STTUNER_ERROR_ID;
#endif
            break;

        default:
#ifdef STTUNER_DEBUG_MODULE_STTUNER
            STTBX_Print(("%s no valid device %d\n", identity, Inst[index].Device));
#endif
            Error = STTUNER_ERROR_ID;
            break;
                    
    } /* switch */
#endif

#ifdef STTUNER_MINIDRIVER

#ifdef STTUNER_USE_SAT
	Error = STTUNER_SAT_Close(index);
#endif

#ifdef STTUNER_USE_TER
	Error = STTUNER_TERR_Close(index);
#endif

#endif

    /* ---------- disable notify function ---------- */
    Inst[index].NotifyFunction = NULL;

    /* ---------- change status ---------- */
    Inst[index].Status = STTUNER_DASTATUS_CLOSE;

    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s closed but with ERRORS\n", identity));
#endif
    } 
    else
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s closed ok\n", identity));
#endif    
    }

    return(Error);
}


#ifndef STTUNER_MINIDRIVER
/* ----------------------------------------------------------------------------
Name: STTUNER_Ioctl()

Description:
    access device (driver) specific features
    
Parameters:
    
Return Value:
    ST_NO_ERROR - the operation was carried out without error.
    
See Also:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_Ioctl(STTUNER_Handle_t Handle, U32 DeviceID, U32 FunctionIndex, void *InParams, void *OutParams)
{
#ifdef STTUNER_DEBUG_USERSPACE_TO_KERNEL
  printk("STTUNER_TEST: ------Display the STTUNER_Ioctl ----- \n");
  printk("STTUNER_TEST: DeviceID is %d\n",DeviceID);
  printk("STTUNER_TEST: FunctionIndex is %d\n", FunctionIndex);
  if((DeviceID == STTUNER_SOFT_ID ) && (FunctionIndex == STTUNER_IOCTL_TUNERSTEP )) {
    printk("STTUNER_TEST: Inparams is %d\n",*(U32 *)InParams);
    printk("STTUNER_TEST: set OutParams is 44\n");
    *(U32*)OutParams = 44;
  }
 
#endif

#ifdef STTUNER_DEBUG_MODULE_STTUNER
    const char *identity = "STTUNER sttuner.c STTUNER_Ioctl()";
#endif
    ST_ErrorCode_t Error;
    STTUNER_InstanceDbase_t *Inst;
    STTUNER_Da_Status_t Status = STTUNER_DASTATUS_UNKNOWN;
    U32 index;

    /* ---------- validate handle ---------- */
    Error = STTUNER_CheckOpenHandle(Handle);
    if (Error != ST_NO_ERROR)
    {   /* Call STTUNER_SWIOCTL_Default() directly if handle not open (Handle == STTUNER_MAX_HANDLES) 
          and STTUNER_SOFT_ID specified, must refer to a non-instance software or special I/O function */
        if (DeviceID == STTUNER_SOFT_ID)
        {
            Error = STTUNER_SWIOCTL_Default(STTUNER_MAX_HANDLES, FunctionIndex, InParams, OutParams, &Status);
            return(Error);
        }
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s fail\n", identity));
#endif
        return(Error);
    }

    index = HANDLE2INDEX( Handle );
    Inst = STTUNER_GetDrvInst();
    /* ---------- select device ---------- */
    switch(Inst[index].Device)
    {

        case STTUNER_DEVICE_SATELLITE:
#ifdef STTUNER_USE_SAT
            Error = STTUNER_SAT_Ioctl(index, DeviceID, FunctionIndex, InParams, OutParams);
#else
            Error = STTUNER_ERROR_ID;
#endif
            break;

        case STTUNER_DEVICE_CABLE:
#ifdef STTUNER_USE_CAB
            Error = STTUNER_CABLE_Ioctl(index, DeviceID, FunctionIndex, InParams, OutParams);
#else
            Error = STTUNER_ERROR_ID;
#endif
            break;

        case STTUNER_DEVICE_TERR:
#if defined(STTUNER_USE_TER) || defined(STTUNER_USE_ATSC_VSB)
            Error = STTUNER_TERR_Ioctl(index, DeviceID, FunctionIndex, InParams, OutParams);
#else
            Error = STTUNER_ERROR_ID;
#endif
            break;


        default:
#ifdef STTUNER_DEBUG_MODULE_STTUNER
                STTBX_Print(("%s no valid device %d\n", identity, Inst[index].Device));
#endif
                Error = STTUNER_ERROR_ID;
                break;
                    
    } /* switch */


#ifdef STTUNER_DEBUG_MODULE_STTUNER
    STTBX_Print(("%s done\n", identity));
#endif
    return(Error);

} /* STTUNER_Ioctl() */


#endif
#ifndef STTUNER_MINIDRIVER
/* ----------------------------------------------------------------------------
Name: STTUNER_GetCapability()

Description:
    get opened device capabilities.
    
Parameters:
    
Return Value:
    ST_NO_ERROR     the operation was carried out without error
    
See Also:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_GetCapability(const ST_DeviceName_t DeviceName, STTUNER_Capability_t *Capability_p)
{
#ifdef STTUNER_DEBUG_USERSPACE_TO_KERNEL
  printk("STTUNER_TEST: ------Display the STTUNER_GetCapabilityParams_t ----- \n");
  printk("STTUNER_TEST: DeviceName is %s\n",DeviceName);
  printk("STTUNER_TEST: ------Set the value in the sttuner.c ----- \n");
  printk("STTUNER_TEST: ------Set LnbType  as STTUNER_LNB_LNBH21(215) ----- \n");
  Capability_p->LnbType = STTUNER_LNB_LNBH21;
#endif

#ifdef STTUNER_DEBUG_MODULE_STTUNER
    const char *identity = "STTUNER sttuner.c STTUNER_GetCapability()";
#endif
    ST_ErrorCode_t Error;
    STTUNER_InstanceDbase_t *Inst;
    U32 index;

    /* ---------- Check the parameters ---------- */
    Error = STTUNER_Util_CheckPtrNull(Capability_p);
    if( Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s fail Capability not valid\n", identity));
#endif
        return(Error);
    }

    Error = STTUNER_Util_CheckPtrNull((void *)DeviceName);
    if( Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s fail DeviceName not valid\n", identity));
#endif
        return(Error);
    }

    /* ---------- find instance associated with name ---------- */

    Inst = STTUNER_GetDrvInst();

    index = 0;
#ifdef STTUNER_DEBUG_MODULE_STTUNER
    STTBX_Print(("[%s] ", Inst[index].Name));
    
#endif
    while(strcmp(Inst[index].Name, DeviceName) != 0)
    {
        
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("[%s] ", Inst[index].Name));
#endif
        index++;
        if (index >= STTUNER_MAX_HANDLES)
        {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
            STTBX_Print(("\n%s fail DeviceName (%s) not found\n", identity, DeviceName));
#endif
            return(ST_ERROR_UNKNOWN_DEVICE);
        }

    }


#ifdef STTUNER_DEBUG_MODULE_STTUNER
    STTBX_Print(("%s found (%s) at index %d",identity, DeviceName, index));
#endif

    
  /*  Capability_p = &Inst[index].Capability;*/
    
    memcpy(Capability_p,&Inst[index].Capability,sizeof(STTUNER_Capability_t));
 
#ifdef STTUNER_DEBUG_MODULE_STTUNER
    STTBX_Print(("%s Capability recovered ok\n", identity));
#endif


    return(Error);

} /* STTUNER_GetCapability() */

#endif
#ifndef STTUNER_MINIDRIVER
/* ----------------------------------------------------------------------------
Name: STTUNER_Scan()

Description:
    
Parameters:
    
Return Value:
    ST_NO_ERROR, the operation was carried out without error.
    
See Also:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_Scan(STTUNER_Handle_t Handle, U32 FreqFrom, U32 FreqTo, U32 FreqStep, U32 Timeout)
{
#ifdef STTUNER_DEBUG_USERSPACE_TO_KERNEL
  printk("STTUNER_TEST: ------Display the STTUNER_Scan()       ----- \n");
  printk("STTUNER_TEST: FreqFrom is %d, FreqTo is %d,FreqStep is %d\n",FreqFrom,FreqTo,FreqStep);
#endif

#ifdef STTUNER_DEBUG_MODULE_STTUNER
    const char *identity = "STTUNER sttuner.c STTUNER_Scan()";
#endif
    ST_ErrorCode_t Error;
    STTUNER_InstanceDbase_t *Inst;
    U32 index;

    /* ---------- validate handle ---------- */
    Error = STTUNER_CheckOpenHandle(Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s fail\n", identity));
#endif
        return(Error);
    }

    index = HANDLE2INDEX( Handle );
    Inst = STTUNER_GetDrvInst();

    /* ---------- select device ---------- */
    switch(Inst[index].Device)
    {

        case STTUNER_DEVICE_SATELLITE:
#ifdef STTUNER_USE_SAT
            Error = STTUNER_SAT_Scan(index, FreqFrom, FreqTo, FreqStep, Timeout);
#else
            Error = STTUNER_ERROR_ID;
#endif
            break;

        case STTUNER_DEVICE_CABLE:
#ifdef STTUNER_USE_CAB
            Error = STTUNER_CABLE_Scan(index,  FreqFrom, FreqTo, FreqStep, Timeout);
#else
            Error = STTUNER_ERROR_ID;
#endif
            break;

        case STTUNER_DEVICE_TERR:
#if defined(STTUNER_USE_TER) || defined(STTUNER_USE_ATSC_VSB)
            Error = STTUNER_TERR_Scan(index,  FreqFrom, FreqTo, FreqStep, Timeout);
#else
            Error = STTUNER_ERROR_ID;
#endif
            break;


            default:
#ifdef STTUNER_DEBUG_MODULE_STTUNER
                STTBX_Print(("%s no valid device %d\n", identity, Inst[index].Device));
#endif
                Error = STTUNER_ERROR_ID;
                break;
                    
    } /* switch */

    if (Error == ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s done ok \n", identity));
#endif
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s fail\n", identity));
#endif
    }


    return(Error);

} /* STTUNER_Scan() */

/* ----------------------------------------------------------------------------
Name: STTUNER_BlindScan()

Description:
    
Parameters:
    
Return Value:
    ST_NO_ERROR, the operation was carried out without error.
    
See Also:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_BlindScan(STTUNER_Handle_t Handle, U32 FreqFrom, U32 FreqTo, U32 FreqStep, STTUNER_BlindScan_t *BlindScanParams, U32 Timeout)
{
#ifdef STTUNER_DEBUG_MODULE_STTUNER
    const char *identity = "STTUNER sttuner.c STTUNER_Scan()";
#endif
    ST_ErrorCode_t Error;
    STTUNER_InstanceDbase_t *Inst;
    U32 index;

    /* ---------- validate handle ---------- */
    Error = STTUNER_CheckOpenHandle(Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s fail\n", identity));
#endif
        return(Error);
    }

    index = HANDLE2INDEX( Handle );
    Inst = STTUNER_GetDrvInst();

    /* ---------- select device ---------- */
    switch(Inst[index].Device)
    {

        case STTUNER_DEVICE_SATELLITE:
#ifdef STTUNER_USE_SAT
            Error = STTUNER_SAT_BlindScan(index, FreqFrom, FreqTo, FreqStep, BlindScanParams, Timeout);
#else
            Error = STTUNER_ERROR_ID;
#endif
            break;


      
            default:
#ifdef STTUNER_DEBUG_MODULE_STTUNER
                STTBX_Print(("%s no valid device %d\n", identity, Inst[index].Device));
#endif
                Error = STTUNER_ERROR_ID;
                break;
                    
    } /* switch */

    if (Error == ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s done ok \n", identity));
#endif
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s fail\n", identity));
#endif
    }


    return(Error);

} /* STTUNER_BlindScan() */
#endif
#ifndef STTUNER_MINIDRIVER
/* ----------------------------------------------------------------------------
Name: STTUNER_ScanContinue()

Description:
    
Parameters:
    
Return Value:
    ST_NO_ERROR, the operation was carried out without error.
    
See Also:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_ScanContinue(STTUNER_Handle_t Handle, U32 Timeout)
{
#ifdef STTUNER_DEBUG_USERSPACE_TO_KERNEL
  printk("STTUNER_TEST: ------Display the STTUNER_ScanContinue----- \n");
  printk("STTUNER_TEST: Timeout is %d\n",Timeout);
#endif

#ifdef STTUNER_DEBUG_MODULE_STTUNER
    const char *identity = "STTUNER sttuner.c STTUNER_ScanContinue()";
#endif
    ST_ErrorCode_t Error;
    STTUNER_InstanceDbase_t *Inst;
    U32 index;


    /* ---------- validate handle ---------- */

    Error = STTUNER_CheckOpenHandle(Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s fail\n", identity));
#endif
        return(Error);
    }

    index = HANDLE2INDEX( Handle );
    Inst = STTUNER_GetDrvInst();

    /* ---------- select device ---------- */

    switch(Inst[index].Device)
    {

        case STTUNER_DEVICE_SATELLITE:
#ifdef STTUNER_USE_SAT
            Error = STTUNER_SAT_ScanContinue(index, Timeout);
#else
            Error = STTUNER_ERROR_ID;
#endif
            break;

        case STTUNER_DEVICE_CABLE:
#ifdef STTUNER_USE_CAB
            Error = STTUNER_CABLE_ScanContinue(index, Timeout);
#else
            Error = STTUNER_ERROR_ID;
#endif
            break;

        case STTUNER_DEVICE_TERR:
#if defined(STTUNER_USE_TER) || defined(STTUNER_USE_ATSC_VSB)
            Error = STTUNER_TERR_ScanContinue(index, Timeout);
#else
            Error = STTUNER_ERROR_ID;
#endif
            break;


            default:
#ifdef STTUNER_DEBUG_MODULE_STTUNER
                STTBX_Print(("%s no valid device %d\n", identity, Inst[index].Device));
#endif
                Error = STTUNER_ERROR_ID;
                break;
                    
    } /* switch */

    if (Error == ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s done ok \n", identity));
#endif
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s fail\n", identity));
#endif
    }


    return(Error);

} /* STTUNER_ScanContinue() */
#endif


/* ----------------------------------------------------------------------------
Name: STTUNER_Unlock()

Description:
    
Parameters:
    
Return Value:
    ST_NO_ERROR, the operation was carried out without error.
    
See Also:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_Unlock(STTUNER_Handle_t Handle)
{
#ifdef STTUNER_DEBUG_USERSPACE_TO_KERNEL
  printk("STTUNER_TEST: ------Display the STTUNER_Unlock ----- \n");
  printk("STTUNER_TEST: Handle is %d\n",Handle);
#endif

#ifdef STTUNER_DEBUG_MODULE_STTUNER
    const char *identity = "STTUNER sttuner.c STTUNER_Unlock()";
#endif
    ST_ErrorCode_t Error;
    STTUNER_InstanceDbase_t *Inst;
    U32 index;


    /* ---------- validate handle ---------- */

    Error = STTUNER_CheckOpenHandle(Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s fail\n", identity));
#endif
        return(Error);
    }

    index = HANDLE2INDEX( Handle );
    Inst = STTUNER_GetDrvInst();

    /* ---------- select device ---------- */
#ifndef STTUNER_MINIDRIVER
    switch(Inst[index].Device)
    {

        case STTUNER_DEVICE_SATELLITE:
#ifdef STTUNER_USE_SAT
            Error = STTUNER_SAT_Unlock(index);
#else
            Error = STTUNER_ERROR_ID;
#endif
            break;

        case STTUNER_DEVICE_CABLE:
#ifdef STTUNER_USE_CAB
            Error = STTUNER_CABLE_Unlock(index);
#else
            Error = STTUNER_ERROR_ID;
#endif
            break;

        case STTUNER_DEVICE_TERR:
#if defined(STTUNER_USE_TER) || defined(STTUNER_USE_ATSC_VSB)
            Error = STTUNER_TERR_Unlock(index);
#else
            Error = STTUNER_ERROR_ID;
#endif
            break;


            default:
#ifdef STTUNER_DEBUG_MODULE_STTUNER
                STTBX_Print(("%s no valid device %d\n", identity, Inst[index].Device));
#endif
                Error = STTUNER_ERROR_ID;
                break;
                    
    } /* switch */
    #endif
    

    if (Error == ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s done ok \n", identity));
#endif
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s fail\n", identity));
#endif
    }


    return(Error);

} /* STTUNER_Unlock() */


#ifndef STTUNER_MINIDRIVER
/* ----------------------------------------------------------------------------
Name: STTUNER_GetBandList()

Description:
    
Parameters:
    
Return Value:
    ST_NO_ERROR, the operation was carried out without error.
    
See Also:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_GetBandList(STTUNER_Handle_t Handle, STTUNER_BandList_t *BandList_p)
{
#ifdef STTUNER_DEBUG_USERSPACE_TO_KERNEL
  printk("STTUNER_TEST: ------Display the STTUNER_GetBandList ----- \n");  
  printk("STTUNER_TEST: ------Set the value in the sttuner.c ----- \n");
  printk("STTUNER_TEST: ------Set BandList_p->BandList->BandStart as 111 ----- \n");
  BandList_p->BandList->BandStart = 111; 
#endif

#ifdef STTUNER_DEBUG_MODULE_STTUNER
    const char *identity = "STTUNER sttuner.c STTUNER_GetBandList()";
#endif
    ST_ErrorCode_t Error;
    STTUNER_InstanceDbase_t *Inst;
    U32 index;


    /* ---------- validate handle ---------- */

    Error = STTUNER_CheckOpenHandle(Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s fail\n", identity));
#endif
        return(Error);
    }

    index = HANDLE2INDEX( Handle );
    Inst = STTUNER_GetDrvInst();

    /* ---------- Check the parameters ---------- */
    Error = STTUNER_Util_CheckPtrNull(BandList_p);
    if( Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s fail BandList not valid\n", identity));
#endif
        return(Error);
    }

    /* ---------- select device ---------- */

    switch(Inst[index].Device)
    {

        case STTUNER_DEVICE_SATELLITE:
#ifdef STTUNER_USE_SAT
            Error = STTUNER_SAT_GetBandList(index, BandList_p);
#else
            Error = STTUNER_ERROR_ID;
#endif
            break;

        case STTUNER_DEVICE_CABLE:
#ifdef STTUNER_USE_CAB
            Error = STTUNER_CABLE_GetBandList(index, BandList_p);
#else
            Error = STTUNER_ERROR_ID;
#endif
            break;

        case STTUNER_DEVICE_TERR:
#if defined(STTUNER_USE_TER) || defined(STTUNER_USE_ATSC_VSB)
            Error = STTUNER_TERR_GetBandList(index, BandList_p);
#else
            Error = STTUNER_ERROR_ID;
#endif
            break;


            default:
#ifdef STTUNER_DEBUG_MODULE_STTUNER
                STTBX_Print(("%s no valid device %d\n", identity, Inst[index].Device));
#endif
                Error = STTUNER_ERROR_ID;
                break;
                    
    } /* switch */

    if (Error == ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s done ok \n", identity));
#endif
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s fail\n", identity));
#endif
    }


    return(Error);

} /* STTUNER_GetBandList() */
#endif
#ifndef STTUNER_MINIDRIVER

/* ----------------------------------------------------------------------------
Name: STTUNER_GetScanList()

Description:
    
Parameters:
    
Return Value:
    ST_NO_ERROR, the operation was carried out without error.
    
See Also:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_GetScanList(STTUNER_Handle_t Handle, STTUNER_ScanList_t *ScanList_p)
{
#ifdef STTUNER_DEBUG_USERSPACE_TO_KERNEL
  printk("STTUNER_TEST: ------Display the STTUNER_GetScanList ----- \n");
  printk( "STTUNER_TEST: Set the value In the STAPI \n");
  printk( "STTUNER_TEST: ScanList.NumElements is 1\n");
  printk( "STTUNER_TEST: ScanList.ScanList->Band is 5 \n");
  ScanList_p->NumElements = 1;
  ScanList_p->ScanList->Band = 5;
#endif

#ifdef STTUNER_DEBUG_MODULE_STTUNER
    const char *identity = "STTUNER sttuner.c STTUNER_GetScanList()";
#endif
    ST_ErrorCode_t Error;
    STTUNER_InstanceDbase_t *Inst;
    U32 index;


    /* ---------- validate handle ---------- */

    Error = STTUNER_CheckOpenHandle(Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s fail\n", identity));
#endif
        return(Error);
    }

    index = HANDLE2INDEX( Handle );
    Inst = STTUNER_GetDrvInst();

    /* ---------- Check the parameters ---------- */
    Error = STTUNER_Util_CheckPtrNull(ScanList_p);
    if( Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s fail ScanList not valid\n", identity));
#endif
        return(Error);
    }

    /* ---------- select device ---------- */

    switch(Inst[index].Device)
    {

        case STTUNER_DEVICE_SATELLITE:
#ifdef STTUNER_USE_SAT
            Error = STTUNER_SAT_GetScanList(index, ScanList_p);
#else
            Error = STTUNER_ERROR_ID;
#endif
            break;

        case STTUNER_DEVICE_CABLE:
#ifdef STTUNER_USE_CAB
            Error = STTUNER_CABLE_GetScanList(index, ScanList_p);
#else
            Error = STTUNER_ERROR_ID;
#endif
            break;

        case STTUNER_DEVICE_TERR:
#if defined(STTUNER_USE_TER) || defined(STTUNER_USE_ATSC_VSB)
            Error = STTUNER_TERR_GetScanList(index, ScanList_p);
#else
            Error = STTUNER_ERROR_ID;
#endif
            break;


            default:
#ifdef STTUNER_DEBUG_MODULE_STTUNER
                STTBX_Print(("%s no valid device %d\n", identity, Inst[index].Device));
#endif
                Error = STTUNER_ERROR_ID;
                break;
                    
    } /* switch */

    if (Error == ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s done ok \n", identity));
#endif
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s fail\n", identity));
#endif
    }


    return(Error);

} /* STTUNER_GetScanList() */
#endif


/* ----------------------------------------------------------------------------
Name: STTUNER_GetStatus()

Description:
    
Parameters:
    
Return Value:
    ST_NO_ERROR, the operation was carried out without error.
    
See Also:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_GetStatus(STTUNER_Handle_t Handle, STTUNER_Status_t *Status_p)
{
#ifdef STTUNER_DEBUG_USERSPACE_TO_KERNEL
  printk("STTUNER_TEST: ------Display the STTUNER_GetStatus ----- \n");
  printk("STTUNER_TEST: Set Status as STTUNER_STATUS_LOCKED (2) \n");
  *Status_p =  STTUNER_STATUS_LOCKED;
#endif

#ifdef STTUNER_DEBUG_MODULE_STTUNER
    const char *identity = "STTUNER sttuner.c STTUNER_GetStatus()";
#endif
    ST_ErrorCode_t Error;
    STTUNER_InstanceDbase_t *Inst;
    U32 index;


    /* ---------- validate handle ---------- */

    Error = STTUNER_CheckOpenHandle(Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s fail\n", identity));
#endif
        return(Error);
    }

    index = HANDLE2INDEX( Handle );
    Inst = STTUNER_GetDrvInst();

#ifndef STTUNER_MINIDRIVER
    /* ---------- Check the parameters ---------- */
    Error = STTUNER_Util_CheckPtrNull(Status_p);
    if( Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s fail Status not valid\n", identity));
#endif
        return(Error);
    }

    /* ---------- select device ---------- */

    switch(Inst[index].Device)
    {

        case STTUNER_DEVICE_SATELLITE:
#ifdef STTUNER_USE_SAT
            Error = STTUNER_SAT_GetStatus(index, Status_p);
#else
            Error = STTUNER_ERROR_ID;
#endif
            break;

        case STTUNER_DEVICE_CABLE:
#ifdef STTUNER_USE_CAB
            Error = STTUNER_CABLE_GetStatus(index, Status_p);
#else
            Error = STTUNER_ERROR_ID;
#endif
            break;

        case STTUNER_DEVICE_TERR:
#if defined(STTUNER_USE_TER) || defined(STTUNER_USE_ATSC_VSB)
            Error = STTUNER_TERR_GetStatus(index, Status_p);
#else
            Error = STTUNER_ERROR_ID;
#endif
            break;


            default:
#ifdef STTUNER_DEBUG_MODULE_STTUNER
                STTBX_Print(("%s no valid device %d\n", identity, Inst[index].Device));
#endif
                Error = STTUNER_ERROR_ID;
                break;
                    
    } /* switch */
#endif

#ifdef STTUNER_MINIDRIVER

#ifdef STTUNER_USE_SAT
	Error = STTUNER_SAT_GetStatus(index, Status_p);
#endif

#ifdef STTUNER_USE_TER
	Error = STTUNER_TERR_GetStatus(index, Status_p);
#endif

#endif

    if (Error == ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s done ok \n", identity));
#endif
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s fail\n", identity));
#endif
    }


    return(Error);

} /* STTUNER_GetStatus() */

#ifndef STTUNER_MINIDRIVER


/* ----------------------------------------------------------------------------
Name: STTUNER_GetThresholdList()

Description:
    
Parameters:
    
Return Value:
    ST_NO_ERROR, the operation was carried out without error.
    
See Also:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_GetThresholdList(STTUNER_Handle_t Handle, STTUNER_ThresholdList_t *ThresholdList_p, STTUNER_QualityFormat_t *QualityFormat_p)
{
#ifdef STTUNER_DEBUG_USERSPACE_TO_KERNEL
  printk("STTUNER_TEST: ------Display the STTUNER_GetThresholdList ----- \n");
  printk("STTUNER_TEST: Set ThresholdList_p->NumElements is 11\n");
  printk("STTUNER_TEST: Set ThresholdList_p->ThresholdList->SignalLow is 22\n");  ThresholdList_p->NumElements = 11;
  ThresholdList_p->ThresholdList->SignalLow = 22;
#endif

#ifdef STTUNER_DEBUG_MODULE_STTUNER
    const char *identity = "STTUNER sttuner.c STTUNER_GetThresholdList()";
#endif
    ST_ErrorCode_t Error;
    STTUNER_InstanceDbase_t *Inst;
    U32 index;



    /* ---------- validate handle ---------- */

    Error = STTUNER_CheckOpenHandle(Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s fail\n", identity));
#endif
        return(Error);
    }

    index = HANDLE2INDEX( Handle );
    Inst = STTUNER_GetDrvInst();

    /* ---------- Check the parameters ---------- */
    Error = STTUNER_Util_CheckPtrNull(ThresholdList_p);
    if( Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s fail ThresholdList_p not valid\n", identity));
#endif
        return(Error);
    }

    /* ---------- Check the parameters ---------- */
    Error = STTUNER_Util_CheckPtrNull(QualityFormat_p);
    if( Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s fail QualityFormat_p not valid\n", identity));
#endif
        return(Error);
    }

    /* ---------- select device ---------- */

    switch(Inst[index].Device)
    {

        case STTUNER_DEVICE_SATELLITE:
#ifdef STTUNER_USE_SAT
            Error = STTUNER_SAT_GetThresholdList(index, ThresholdList_p, QualityFormat_p);
#else
            Error = STTUNER_ERROR_ID;
#endif
            break;

        case STTUNER_DEVICE_CABLE:
#ifdef STTUNER_USE_CAB
            Error = STTUNER_CABLE_GetThresholdList(index, ThresholdList_p, QualityFormat_p);
#else
            Error = STTUNER_ERROR_ID;
#endif
            break;

        case STTUNER_DEVICE_TERR:
#if defined(STTUNER_USE_TER) || defined(STTUNER_USE_ATSC_VSB)
            Error = STTUNER_TERR_GetThresholdList(index, ThresholdList_p, QualityFormat_p);
#else
            Error = STTUNER_ERROR_ID;
#endif
            break;


            default:
#ifdef STTUNER_DEBUG_MODULE_STTUNER
                STTBX_Print(("%s no valid device %d\n", identity, Inst[index].Device));
#endif
                Error = STTUNER_ERROR_ID;
                break;
                    
    } /* switch */



    if (Error == ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s done ok \n", identity));
#endif
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s fail\n", identity));
#endif
    }


    return(Error);

} /* STTUNER_GetThresholdList() */
#endif


/* ----------------------------------------------------------------------------
Name: STTUNER_GetTunerInfo()

Description:
    
Parameters:
    
Return Value:
    ST_NO_ERROR, the operation was carried out without error.
    
See Also:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_GetTunerInfo(STTUNER_Handle_t Handle, STTUNER_TunerInfo_t *TunerInfo_p)
{
#ifdef STTUNER_DEBUG_USERSPACE_TO_KERNEL
  printk("STTUNER_TEST: ------Display the STTUNER_GetTunerInfo ----- \n");
  printk("STTUNER_TEST: Set TunerInfo_p->Frequency is 9600\n");
  TunerInfo_p->Frequency = 9600;
#endif

#ifdef STTUNER_DEBUG_MODULE_STTUNER
    const char *identity = "STTUNER sttuner.c STTUNER_GetTunerInfo()";
#endif
    ST_ErrorCode_t Error;
    STTUNER_InstanceDbase_t *Inst;
    U32 index;


    /* ---------- validate handle ---------- */

    Error = STTUNER_CheckOpenHandle(Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s fail\n", identity));
#endif
        return(Error);
    }

    index = HANDLE2INDEX( Handle );
    Inst = STTUNER_GetDrvInst();

#ifndef STTUNER_MINIDRIVER
    /* ---------- Check the parameters ---------- */
    Error = STTUNER_Util_CheckPtrNull(TunerInfo_p);
    if( Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s fail TunerInfo not valid\n", identity));
#endif
        return(Error);
    }

    /* ---------- select device ---------- */

    switch(Inst[index].Device)
    {

        case STTUNER_DEVICE_SATELLITE:
#ifdef STTUNER_USE_SAT

            Error = STTUNER_SAT_GetTunerInfo(index, TunerInfo_p);
#else
            Error = STTUNER_ERROR_ID;
#endif
            break;

        case STTUNER_DEVICE_CABLE:
#ifdef STTUNER_USE_CAB
            Error = STTUNER_CABLE_GetTunerInfo(index, TunerInfo_p);
#else
            Error = STTUNER_ERROR_ID;
#endif
            break;

        case STTUNER_DEVICE_TERR:
#if defined(STTUNER_USE_TER) || defined(STTUNER_USE_ATSC_VSB)
            Error = STTUNER_TERR_GetTunerInfo(index, TunerInfo_p);
#else
            Error = STTUNER_ERROR_ID;
#endif
            break;


            default:
#ifdef STTUNER_DEBUG_MODULE_STTUNER
                STTBX_Print(("%s no valid device %d\n", identity, Inst[index].Device));
#endif
                Error = STTUNER_ERROR_ID;
                break;
                    
    } /* switch */
#endif
#ifdef STTUNER_MINIDRIVER

#ifdef STTUNER_USE_SAT
	Error = STTUNER_SAT_GetTunerInfo(index, TunerInfo_p);
#endif

#ifdef STTUNER_USE_TER
	Error = STTUNER_TERR_GetTunerInfo(index, TunerInfo_p);
#endif

#endif

    if (Error == ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s done ok \n", identity));
#endif
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s fail\n", identity));
#endif
    }
 
    return(Error);

} /* STTUNER_GetTunerInfo() */



/* ----------------------------------------------------------------------------
Name: STTUNER_SetBandList()

Description:
    
Parameters:
    
Return Value:
    ST_NO_ERROR, the operation was carried out without error.
    
See Also:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_SetBandList(STTUNER_Handle_t Handle, const STTUNER_BandList_t *BandList_p)
{
#ifdef STTUNER_DEBUG_USERSPACE_TO_KERNEL
  printk("STTUNER_TEST: ------Display the STTUNER_SetBandList ----- \n");
  printk("STTUNER_TEST: BandList_p->NumElements is %d\n",BandList_p->NumElements);
  printk("STTUNER_TEST: BandList_p->BandList->BandStart is %d\n",BandList_p->BandList->BandStart);
#endif

#ifdef STTUNER_DEBUG_MODULE_STTUNER
    const char *identity = "STTUNER sttuner.c STTUNER_SetBandList()";
#endif
    ST_ErrorCode_t Error;
    STTUNER_InstanceDbase_t *Inst;
    U32 index;


    /* ---------- validate handle ---------- */

    Error = STTUNER_CheckOpenHandle(Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s fail\n", identity));
#endif
        return(Error);
    }

    index = HANDLE2INDEX( Handle );
    Inst = STTUNER_GetDrvInst();
#ifndef STTUNER_MINIDRIVER
    /* ---------- Check the parameters ---------- */
    Error = STTUNER_Util_CheckPtrNull((void *)BandList_p);
    if( Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s fail BandList not valid\n", identity));
#endif
        return(Error);
    }

    /* ---------- select device ---------- */

    switch(Inst[index].Device)
    {

        case STTUNER_DEVICE_SATELLITE:
#ifdef STTUNER_USE_SAT
            Error = STTUNER_SAT_SetBandList(index, BandList_p);
#else
            Error = STTUNER_ERROR_ID;
#endif
            break;

        case STTUNER_DEVICE_CABLE:
#ifdef STTUNER_USE_CAB
            Error = STTUNER_CABLE_SetBandList(index, BandList_p);
#else
            Error = STTUNER_ERROR_ID;
#endif
            break;

        case STTUNER_DEVICE_TERR:
#if defined(STTUNER_USE_TER) || defined(STTUNER_USE_ATSC_VSB)
            Error = STTUNER_TERR_SetBandList(index, BandList_p);
#else
            Error = STTUNER_ERROR_ID;
#endif
            break;


            default:
#ifdef STTUNER_DEBUG_MODULE_STTUNER
                STTBX_Print(("%s no valid device %d\n", identity, Inst[index].Device));
#endif
                Error = STTUNER_ERROR_ID;
                break;
                    
    } /* switch */
#endif

#ifdef STTUNER_MINIDRIVER
#ifdef STTUNER_USE_SAT
	Error = STTUNER_SAT_SetBandList(index, BandList_p);
#endif

#ifdef STTUNER_USE_TER
	Error = STTUNER_TERR_SetBandList(index, BandList_p);
#endif
#endif


    if (Error == ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s done ok \n", identity));
#endif
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s fail\n", identity));
#endif
    }


    return(Error);

} /* STTUNER_SetBandList() */



/* ----------------------------------------------------------------------------
Name: STTUNER_SetFrequency()

Description:
    
Parameters:
    
Return Value:
    ST_NO_ERROR, the operation was carried out without error.
    
See Also:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_SetFrequency(STTUNER_Handle_t Handle, U32 Frequency, STTUNER_Scan_t *ScanParams_p, U32 Timeout)
{
#ifdef STTUNER_DEBUG_USERSPACE_TO_KERNEL
  printk("STTUNER_TEST: ------Display the STTUNER_SetFrequency ----- \n");
  printk("STTUNER_TEST: Frequency is %d\n",Frequency);
  printk("STTUNER_TEST: ScanParams_p->Modulation is %d\n",ScanParams_p->Modulation);
#endif

#ifdef STTUNER_DEBUG_MODULE_STTUNER
    const char *identity = "STTUNER sttuner.c STTUNER_SetFrequency()";
#endif
    ST_ErrorCode_t Error;
    STTUNER_InstanceDbase_t *Inst;
    U32 index;


    /* ---------- validate handle ---------- */

    Error = STTUNER_CheckOpenHandle(Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s fail\n", identity));
#endif
        return(Error);
    }

    Inst = STTUNER_GetDrvInst();
    index = HANDLE2INDEX( Handle );
#ifndef STTUNER_MINIDRIVER

    /* ---------- Check the parameters ---------- */
    Error = STTUNER_Util_CheckPtrNull(ScanParams_p);
    if( Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s fail ScanParams not valid\n", identity));
#endif
        return(Error);
    }


    /* ---------- select device ---------- */

    switch(Inst[index].Device)
    {

        case STTUNER_DEVICE_SATELLITE:
#ifdef STTUNER_USE_SAT
	    ScanParams_p->DishPositioning_ToneOnOff = FALSE;
            Error = STTUNER_SAT_SetFrequency(index, Frequency, ScanParams_p, Timeout);
#else
            Error = STTUNER_ERROR_ID;
#endif
            break;

        case STTUNER_DEVICE_CABLE:
#ifdef STTUNER_USE_CAB
            Error = STTUNER_CABLE_SetFrequency(index, Frequency, ScanParams_p, Timeout);
#else
            Error = STTUNER_ERROR_ID;
#endif
            break;

        case STTUNER_DEVICE_TERR:
#if defined(STTUNER_USE_TER) || defined(STTUNER_USE_ATSC_VSB)
            Error = STTUNER_TERR_SetFrequency(index, Frequency, ScanParams_p, Timeout);
#else
            Error = STTUNER_ERROR_ID;
#endif
            break;


            default:
#ifdef STTUNER_DEBUG_MODULE_STTUNER
                STTBX_Print(("%s no valid device %d\n", identity, Inst[index].Device));
#endif
                Error = STTUNER_ERROR_ID;
                break;
                    
    } /* switch */
#endif

#ifdef STTUNER_MINIDRIVER
	/*ScanParams_p->DishPositioning_ToneOnOff = FALSE;*/
#ifdef STTUNER_USE_SAT
 	Error = STTUNER_SAT_SetFrequency(index, Frequency, ScanParams_p, Timeout);
#endif
#ifdef STTUNER_USE_TER
	Error = STTUNER_TERR_SetFrequency(index, Frequency, ScanParams_p, Timeout);
#endif
#endif


    if (Error == ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s done ok \n", identity));
#endif
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s fail\n", identity));
#endif
    }


    return(Error);

} /* STTUNER_SetFrequency() */

/******************************************************************/



#ifndef STTUNER_MINIDRIVER
/* ----------------------------------------------------------------------------
Name: STTUNER_StandByMode()

Description:
    
Parameters:
    
Return Value:
    ST_NO_ERROR, the operation was carried out without error.
    
See Also:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_StandByMode(STTUNER_Handle_t Handle,STTUNER_StandByMode_t PowerMode)
{
#ifdef STTUNER_DEBUG_MODULE_STTUNER
    const char *identity =  "STTUNER sttuner.c STTUNER_StandByMode()";
#endif
    ST_ErrorCode_t Error;
    STTUNER_InstanceDbase_t *Inst;
    U32 index;


    /* ---------- validate handle ---------- */

    Error = STTUNER_CheckOpenHandle(Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s fail\n", identity));
#endif
        return(Error);
    }

    index = HANDLE2INDEX( Handle );
    Inst = STTUNER_GetDrvInst();

    /* ---------- select device ---------- */

    switch(Inst[index].Device)
    {

        case STTUNER_DEVICE_TERR:
#if defined(STTUNER_USE_TER) || defined(STTUNER_USE_ATSC_VSB)
            Error = STTUNER_TERR_StandByMode (index,PowerMode);
     
#else
            Error = STTUNER_ERROR_ID;
#endif
            break;

       case STTUNER_DEVICE_SATELLITE:
#if defined STTUNER_USE_SAT
     
            Error = STTUNER_SAT_StandByMode (index,PowerMode);
     
#else
            Error = STTUNER_ERROR_ID;
#endif
            break;
       
             case STTUNER_DEVICE_CABLE:
#ifdef STTUNER_USE_CAB
     
            Error = STTUNER_CABLE_StandByMode (index,PowerMode);
     
#else
            Error = STTUNER_ERROR_ID;
#endif

            break;
            

      

            default:
#ifdef STTUNER_DEBUG_MODULE_STTUNER
                STTBX_Print(("%s no valid device %d\n", identity, Inst[index].Device));
#endif
                Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
                break;
                    
    } /* switch */

    if (Error == ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s done ok \n", identity));
#endif
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s fail\n", identity));
#endif
    }


    return(Error);
}
#endif

#ifndef STTUNER_MINIDRIVER
/* ----------------------------------------------------------------------------
Name: STTUNER_DishPositioning()

Description:
This API is used for positioning of Dish Antenna. This will send a tone depending upon the signal strength on coaxial cable and user can hear the tone by connecting a 
audio socket in the coaxil cable near the dish and can track the satellite.
Only available with 399 & 399E drivers.
   
Parameters:
    
Return Value:
    ST_NO_ERROR, the operation was carried out without error.
    
See Also:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_DishPositioning(STTUNER_Handle_t Handle, U32 Frequency, STTUNER_Scan_t *ScanParams_p, BOOL DishPositioning_ToneOnOff, U32 Timeout)
{
#ifdef STTUNER_DEBUG_USERSPACE_TO_KERNEL
  printk("STTUNER_TEST: ------Display the STTUNER_DishPositioning ----- \n");
  printk("STTUNER_TEST: Frequency is %d\n",Frequency);
  printk("STTUNER_TEST: ScanParams_p->Modulation is %d\n",ScanParams_p->Modulation);
#endif

#ifdef STTUNER_DEBUG_MODULE_STTUNER
    const char *identity = "STTUNER sttuner.c STTUNER_DishPositioning()";
#endif
    ST_ErrorCode_t Error;
    STTUNER_InstanceDbase_t *Inst;
    U32 index;


    /* ---------- validate handle ---------- */

    Error = STTUNER_CheckOpenHandle(Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s fail\n", identity));
#endif
        return(Error);
    }

    Inst = STTUNER_GetDrvInst();
    index = HANDLE2INDEX( Handle );

    /* ---------- Check the parameters ---------- */
    Error = STTUNER_Util_CheckPtrNull(ScanParams_p);
    if( Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s fail ScanParams not valid\n", identity));
#endif
        return(Error);
    }

    /* ---------- select device ---------- */
    #ifdef STTUNER_USE_SAT
	ScanParams_p->DishPositioning_ToneOnOff = DishPositioning_ToneOnOff;
    #endif

    switch(Inst[index].Device)
    {

        case STTUNER_DEVICE_SATELLITE:
#ifdef STTUNER_USE_SAT
            Error = STTUNER_SAT_SetFrequency(index, Frequency, ScanParams_p, Timeout);
#else
            Error = STTUNER_ERROR_ID;
#endif
            break;

            default:
#ifdef STTUNER_DEBUG_MODULE_STTUNER
                STTBX_Print(("%s Dish Positioning is available only for Satellite.  %d\n", identity, Inst[index].Device));
#endif
                Error = STTUNER_ERROR_ID;
                break;
                    
    } /* switch */

    if (Error == ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s done ok \n", identity));
#endif
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s fail\n", identity));
#endif
    }


    return(Error);

} /* STTUNER_DishPositioning() */
#endif




#ifndef STTUNER_MINIDRIVER	 
/* ----------------------------------------------------------------------------
Name: STTUNER_SetLNBConfig()

Description:
    satellite specific
Parameters:
    
Return Value:
    ST_NO_ERROR, the operation was carried out without error.
    
See Also:
---------------------------------------------------------------------------- */
 ST_ErrorCode_t STTUNER_SetLNBConfig(STTUNER_Handle_t Handle, STTUNER_LNBToneState_t LNBToneState, STTUNER_Polarization_t LNBVoltage)
{
#ifdef STTUNER_DEBUG_USERSPACE_TO_KERNEL
  printk("STTUNER_TEST: ------Display the STTUNER_SetLNBConfig ----- \n");
  printk("STTUNER_TEST: LNBToneState is %d\n",LNBState);
#endif

#ifdef STTUNER_DEBUG_MODULE_STTUNER
    const char *identity = "STTUNER sttuner.c STTUNER_SetLNBConfig()";
#endif
    ST_ErrorCode_t Error;
    STTUNER_InstanceDbase_t *Inst;
    U32 index;


    /* ---------- validate handle ---------- */

    Error = STTUNER_CheckOpenHandle(Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s fail\n", identity));
#endif
        return(Error);
    }

    index = HANDLE2INDEX( Handle );
    Inst = STTUNER_GetDrvInst();

    /* ---------- select device ---------- */

    switch(Inst[index].Device)
    {

        case STTUNER_DEVICE_SATELLITE:
#ifdef STTUNER_USE_SAT
            Error = STTUNER_SAT_SetLNBConfig(index, LNBToneState,LNBVoltage);
#else
            Error = STTUNER_ERROR_ID;
#endif
            break;

            default:
#ifdef STTUNER_DEBUG_MODULE_STTUNER
                STTBX_Print(("%s no valid device %d\n", identity, Inst[index].Device));
#endif
                Error = STTUNER_ERROR_ID;
                break;
                    
    } /* switch */

    if (Error == ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s done ok \n", identity));
#endif
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s fail\n", identity));
#endif
    }


    return(Error);

} /* STTUNER_SetLNBConfig() */
#endif	 


#ifdef STTUNER_USE_SAT 
/*~~~~~~~~~~~~~~~~DiSEqC API~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* --------------------------------------------------------------------------
Name: STTUNER_DiSEqCSendReceive()

Description:
    satellite specific .For DiSEqC API implementation
Parameters:
   1. STTUNER_Handle_t Handle, 
   2. STTUNER_DiSEqCSendPacket_t *pDiSEqCSendPacket - Pointer to the command to be sent 
   3. STTUNER_DiSEqCResponsePacket_t *pDiSEqCResponsePacket -pointer to data 
	received frompreipheral equipment for DiSEqC 2.0

Return Value:
    ST_NO_ERROR, the operation was carried out without error.
    
See Also: STTUNER_SetLNBConfig()
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_DiSEqCSendReceive (
										  STTUNER_Handle_t Handle, 
										  STTUNER_DiSEqCSendPacket_t *pDiSEqCSendPacket,
									      STTUNER_DiSEqCResponsePacket_t *pDiSEqCResponsePacket
									     )
{
#ifdef STTUNER_DEBUG_USERSPACE_TO_KERNEL
  printk("STTUNER_TEST: ------Display the STTUNER_DiSEqCSendReceive ----- \n");
  printk("STTUNER_TEST: pDiSEqcSendPacket->uc_TotalNoOfByetes is %d\n",pDiSEqCSendPacket->uc_TotalNoOfBytes);
#endif

#ifdef STTUNER_DEBUG_MODULE_STTUNER
    const char *identity =  "STTUNER sttuner.c STTUNER_DiSEqC_SendReceive()";
#endif
    ST_ErrorCode_t Error;
    STTUNER_InstanceDbase_t *Inst;
    U32 index;


    /* ---------- validate handle ---------- */

    Error = STTUNER_CheckOpenHandle(Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s fail\n", identity));
#endif
        return(Error);
    }

    index = HANDLE2INDEX( Handle );
    Inst = STTUNER_GetDrvInst();

    /* ---------- select device ---------- */

    switch(Inst[index].Device)
    {

        case STTUNER_DEVICE_SATELLITE:
#ifdef STTUNER_USE_SAT
           Error = STTUNER_SAT_DiSEqCSendReceive (index, 
									   pDiSEqCSendPacket,
									   pDiSEqCResponsePacket
									   );
#else
            Error = STTUNER_ERROR_ID;
#endif
            break;

            default:
#ifdef STTUNER_DEBUG_MODULE_STTUNER
                STTBX_Print(("%s no valid device %d\n", identity, Inst[index].Device));
#endif
                Error = STTUNER_ERROR_ID;
                break;
                    
    } /* switch */

    if (Error == ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s done ok \n", identity));
#endif
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s fail\n", identity));
#endif
    }


    return(Error);

} /*STTUNER_DiSEqCSendReceive()*/

/*~~~~~~~~~~~~~~~end~~~~~~~~~~~~~~~~~~~~~~~~~*/
#endif

#ifdef STTUNER_USE_TER 
#ifndef STTUNER_MINIDRIVER
/* ----------------------------------------------------------------------------
Name: STTUNER_GetTPSCellId()

Description:
    
Parameters:
    
Return Value:
    ST_NO_ERROR, the operation was carried out without error.
    
See Also:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_GetTPSCellId(STTUNER_Handle_t Handle, U16 *TPSCellID)
{
#ifdef STTUNER_DEBUG_MODULE_STTUNER
    const char *identity =  "STTUNER sttuner.c STTUNER_GetTPSCellID()";
#endif
    ST_ErrorCode_t Error;
    STTUNER_InstanceDbase_t *Inst;
    U32 index;


    /* ---------- validate handle ---------- */

    Error = STTUNER_CheckOpenHandle(Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s fail\n", identity));
#endif
        return(Error);
    }

    index = HANDLE2INDEX( Handle );
    Inst = STTUNER_GetDrvInst();

    /* ---------- select device ---------- */

    switch(Inst[index].Device)
    {

        case STTUNER_DEVICE_TERR:
#ifdef STTUNER_USE_TER
           Error = STTUNER_TERR_GetTPSCellId (index,TPSCellID);
#else
            Error = STTUNER_ERROR_ID;
#endif
            break;

            default:
#ifdef STTUNER_DEBUG_MODULE_STTUNER
                STTBX_Print(("%s no valid device %d\n", identity, Inst[index].Device));
#endif
                Error = STTUNER_ERROR_ID;
                break;
                    
    } /* switch */

    if (Error == ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s done ok \n", identity));
#endif
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s fail\n", identity));
#endif
    }


    return(Error);
}
#endif	   
#endif


#ifndef STTUNER_MINIDRIVER
/* ----------------------------------------------------------------------------
Name: STTUNER_SetScanList()

Description:
    
Parameters:
    
Return Value:
    ST_NO_ERROR, the operation was carried out without error.
    
See Also:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_SetScanList(STTUNER_Handle_t Handle, const STTUNER_ScanList_t *ScanList_p)
{
#ifdef STTUNER_DEBUG_USERSPACE_TO_KERNEL
  printk("STTUNER_TEST: ------Display the STTUNER_SetScanList ----- \n");
  printk("STTUNER_TEST: ScanList_p->NumElements is %d\n",ScanList_p->NumElements);
  printk("STTUNER_TEST: ScanList_p->ScanList->Modulation is %d\n",ScanList_p->ScanList->Modulation);
#endif

#ifdef STTUNER_DEBUG_MODULE_STTUNER
    const char *identity = "STTUNER sttuner.c STTUNER_SetScanList()";
#endif
    ST_ErrorCode_t Error;
    STTUNER_InstanceDbase_t *Inst;
    U32 index;


    /* ---------- validate handle ---------- */

    Error = STTUNER_CheckOpenHandle(Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s fail\n", identity));
#endif
        return(Error);
    }

    index = HANDLE2INDEX( Handle );
    Inst = STTUNER_GetDrvInst();

    /* ---------- Check the parameters ---------- */
    Error = STTUNER_Util_CheckPtrNull((void *)ScanList_p);
    if( Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s fail ScanList not valid\n", identity));
#endif
        return(Error);
    }

    /* ---------- select device ---------- */

    switch(Inst[index].Device)
    {

        case STTUNER_DEVICE_SATELLITE:
#ifdef STTUNER_USE_SAT
            Error = STTUNER_SAT_SetScanList(index, ScanList_p);
#else
            Error = STTUNER_ERROR_ID;
#endif
            break;

        case STTUNER_DEVICE_CABLE:
#ifdef STTUNER_USE_CAB
            Error = STTUNER_CABLE_SetScanList(index, ScanList_p);
#else
            Error = STTUNER_ERROR_ID;
#endif
            break;

        case STTUNER_DEVICE_TERR:
#if defined(STTUNER_USE_TER) || defined(STTUNER_USE_ATSC_VSB)
            Error = STTUNER_TERR_SetScanList(index, ScanList_p);
#else
            Error = STTUNER_ERROR_ID;
#endif
            break;


            default:
#ifdef STTUNER_DEBUG_MODULE_STTUNER
                STTBX_Print(("%s no valid device %d\n", identity, Inst[index].Device));
#endif
                Error = STTUNER_ERROR_ID;
                break;
                    
    } /* switch */

    if (Error == ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s done ok \n", identity));
#endif
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s fail\n", identity));
#endif
    }


    return(Error);

} /* STTUNER_SetScanList() */
#endif



#ifndef STTUNER_MINIDRIVER
/* ----------------------------------------------------------------------------
Name: STTUNER_SetThresholdList()

Description:
    
Parameters:
    
Return Value:
    ST_NO_ERROR, the operation was carried out without error.
    
See Also:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_SetThresholdList(STTUNER_Handle_t Handle, const STTUNER_ThresholdList_t *ThresholdList_p, STTUNER_QualityFormat_t QualityFormat)
{
#ifdef STTUNER_DEBUG_USERSPACE_TO_KERNEL
  printk("STTUNER_TEST: ------Display the STTUNER_SetThresholdList ----- \n");
  printk("STTUNER_TEST: ThresholdList_p->NumElements is %d\n",ThresholdList_p->NumElements);
  printk("STTUNER_TEST: ThresholdList_p->ThresholdList->SignalLow is %d\n",ThresholdList_p->ThresholdList->SignalLow);
#endif

#ifdef STTUNER_DEBUG_MODULE_STTUNER
    const char *identity = "STTUNER sttuner.c STTUNER_SetThresholdList()";
#endif
    ST_ErrorCode_t Error;
    STTUNER_InstanceDbase_t *Inst;
    U32 index;

    /* ---------- validate handle ---------- */

    Error = STTUNER_CheckOpenHandle(Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s fail\n", identity));
#endif
        return(Error);
    }

    index = HANDLE2INDEX( Handle );
    Inst = STTUNER_GetDrvInst();

    /* ---------- Check the parameters ---------- */
    Error = STTUNER_Util_CheckPtrNull((void *)ThresholdList_p);
    if( Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s fail ThresholdList not valid\n", identity));
#endif
        return(Error);
    }

    /* ---------- select device ---------- */

    switch(Inst[index].Device)
    {

        case STTUNER_DEVICE_SATELLITE:
#ifdef STTUNER_USE_SAT
            Error = STTUNER_SAT_SetThresholdList(index, ThresholdList_p, QualityFormat);
#else
            Error = STTUNER_ERROR_ID;
#endif
            break;

        case STTUNER_DEVICE_CABLE:
#ifdef STTUNER_USE_CAB
            Error = STTUNER_CABLE_SetThresholdList(index, ThresholdList_p, QualityFormat);
#else
            Error = STTUNER_ERROR_ID;
#endif
            break;

        case STTUNER_DEVICE_TERR:
#if defined(STTUNER_USE_TER) || defined(STTUNER_USE_ATSC_VSB)
            Error = STTUNER_TERR_SetThresholdList(index, ThresholdList_p, QualityFormat);
#else
            Error = STTUNER_ERROR_ID;
#endif
            break;


            default:
#ifdef STTUNER_DEBUG_MODULE_STTUNER
                STTBX_Print(("%s no valid device %d\n", identity, Inst[index].Device));
#endif
                Error = STTUNER_ERROR_ID;
                break;
                    
    } /* switch */

    if (Error == ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s done ok \n", identity));
#endif
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s fail\n", identity));
#endif
    }


    return(Error);

} /* STTUNER_() */
#endif


/* ------------------------------------------------------------------------- */
/* /\/\/\/\/\/\/\/\/\/\/\/\/\UTILITY Functions/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ */
/* ------------------------------------------------------------------------- */



ST_ErrorCode_t STTUNER_FreeLists(U32 index)
{
#ifdef STTUNER_DEBUG_MODULE_STTUNER
    const char *identity = "STTUNER sttuner.c STTUNER_FreeLists()";
#endif
    STTUNER_InstanceDbase_t *Inst;

    Inst = STTUNER_GetDrvInst();
#ifndef STTUNER_MINIDRIVER
    if (Inst[index].ThresholdHits != NULL)  
    {
        STOS_MemoryDeallocate(Inst[index].DriverPartition, Inst[index].ThresholdHits);
    }

    if (Inst[index].BandList.BandList != NULL)  
    {
        STOS_MemoryDeallocate(Inst[index].DriverPartition, Inst[index].BandList.BandList);
    }

    if (Inst[index].ThresholdList.ThresholdList != NULL)  
    {
        STOS_MemoryDeallocate(Inst[index].DriverPartition, Inst[index].ThresholdList.ThresholdList);
    }

    if (Inst[index].ScanList.ScanList != NULL)  
    {
        STOS_MemoryDeallocate(Inst[index].DriverPartition, Inst[index].ScanList.ScanList);
    }
#endif
#ifdef STTUNER_MINIDRIVER

	if (Inst[index].BandList.BandList != NULL)  
	    {
	        memory_deallocate(Inst[index].DriverPartition, Inst[index].BandList.BandList);
	    }
#endif

#ifdef STTUNER_DEBUG_MODULE_STTUNER
    STTBX_Print(("%s lists FREED for handle index %d\n", identity, index));
#endif    

    return(ST_NO_ERROR);
}



ST_ErrorCode_t STTUNER_CheckOpenHandle(STTUNER_Handle_t Handle)
{
#ifdef STTUNER_DEBUG_MODULE_STTUNER
    const char *identity = "STTUNER sttuner.c GetInstanceFromValidHandle()";
#endif
    STTUNER_InstanceDbase_t *Inst;
    U32 index;
    
    /* can do nothing if not initalized */
    if (STTUNER_Initalized == FALSE)
    {

#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s nothing initalized\n", identity));
#endif
        return(ST_ERROR_UNKNOWN_DEVICE);
    }

    /* validate handle range */
    index = HANDLE2INDEX( Handle );
    if ( (index >= STTUNER_MAX_HANDLES) || ((Handle & HANDLE_INV_MASK) != HANDLE_OR_MASK) )
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("\n%s fail index (%d) to bad handle (%d)\n", identity, index, Handle));
#endif
        return( ST_ERROR_INVALID_HANDLE );
    }

    /* check instance is open */
    Inst = STTUNER_GetDrvInst();
    if( Inst[index].Status != STTUNER_DASTATUS_OPEN)
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s fail handle %d not open\n", identity, Handle));
#endif
        return(ST_ERROR_INVALID_HANDLE);
    }    

    return(ST_NO_ERROR);

}   /* STTUNER_CheckOpenHandle() */

#ifndef STTUNER_MINIDRIVER
#ifdef STTUNER_DRV_SAT_SCR


/* ----------------------------------------------------------------------------
Name: STTUNER_SCRParams_Update()

Description: Change the SCRIndex / output band frequencies.
    
Parameters: DeviceMap, SCR Index, SCR BPF frequencies.
    
Return Value:
    ST_NO_ERROR, the operation was carried out without error.
    
See Also:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_SCRParams_Update(const ST_DeviceName_t DeviceName, STTUNER_SCRBPF_t SCR,U16* SCRBPFFrequencies)
{
    #ifdef STTUNER_DEBUG_MODULE_STTUNER
    const char *identity = "STTUNER sttuner.c STTUNER_UpdateUB()";
    #endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_InstanceDbase_t    *Inst;
    U32 index = 0;
     /* ---------- now safe to lock ---------- */
    SEM_LOCK(Lock_InitTermOpenClose);
    /* ---------- find handle from name ---------- */
    Inst  = STTUNER_GetDrvInst();
    index = 0;
    #ifdef STTUNER_DEBUG_MODULE_STTUNER
    STTBX_Print(("%s looking (%s) ", identity, Inst[index].Name));
    #endif
    while(strcmp(Inst[index].Name, DeviceName) != 0)
    {
        index++;
        if (index >= STTUNER_MAX_HANDLES)
        {
	    #ifdef STTUNER_DEBUG_MODULE_STTUNER
	            STTBX_Print(("\n%s fail no block named '%s' before end of list\n", identity, DeviceName));
	    #endif
            SEM_UNLOCK(Lock_InitTermOpenClose);
            return(ST_ERROR_UNKNOWN_DEVICE);
        }
        #ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("(%s)", Inst[index].Name));
        #endif
    }    
    #ifdef STTUNER_DEBUG_MODULE_STTUNER
    STTBX_Print(("\n"));
    #endif


    /* ---------- make sure state is initalized ---------- */
    if (Inst[index].Status != STTUNER_DASTATUS_INIT)
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s fail Status is not STTUNER_DASTATUS_INIT for handle %d\n", identity, index));
#endif    
        SEM_UNLOCK(Lock_InitTermOpenClose);

    }

    /* ---------- open an instance of a specific device ---------- */
    switch(Inst[index].Device)
    {

        case STTUNER_DEVICE_SATELLITE:
            #ifdef STTUNER_USE_SAT
            Error = STTUNER_SAT_SCRPARAM_UPDATE(index,SCR,SCRBPFFrequencies);
            #else
            Error = STTUNER_ERROR_ID;
            #endif
            break;

       default:
	    #ifdef STTUNER_DEBUG_MODULE_STTUNER
	            STTBX_Print(("%s no valid device %d\n", identity, Inst[index].Device));
	    #endif
            Error = STTUNER_ERROR_ID;
            break;
                    
    } /* switch */

    SEM_UNLOCK(Lock_InitTermOpenClose);
    return(Error);   
}
#endif
#endif
#ifdef STTUNER_DISEQC2_SWDECODE_VIA_PIO

ST_ErrorCode_t STTUNER_PIOHandleInInstDbase(U32 index,U32 HandlePIO)
{
#ifdef STTUNER_DEBUG_MODULE_STTUNER
    const char *identity = "STTUNER sttuner.c STTUNER_PIOHandleInInstDbase()";
#endif
    STTUNER_InstanceDbase_t *Inst;

    /* Get index from Handle */
     index =  HANDLE2INDEX(index);
    /* Check whether the TunerHandle is valid or not */
    if (index >= STTUNER_MAX_HANDLES)
    {
#ifdef STTUNER_DEBUG_MODULE_STTUNER
        STTBX_Print(("%s fail inavlid handle\n", identity));
#endif
        return(ST_ERROR_INVALID_HANDLE);
    }
    Inst = STTUNER_GetDrvInst();
    
    /* Set the PIOHandle into Instance Database */
    Inst[index].Sat.DiSEqC.HandlePIO = HandlePIO;    
    
#ifdef STTUNER_DEBUG_MODULE_STTUNER
    STTBX_Print(("%s PIOHandle is copied into instance Database\n", identity));
#endif    

    return(ST_NO_ERROR);
}/* STTUNER_PIOHandleInInstDbase */
#endif 

/****************Utility API To Check All SatCR Tones********************/
#ifndef STTUNER_MINIDRIVER
#ifdef STTUNER_DRV_SAT_SCR
ST_ErrorCode_t STTUNER_ScrTone_Enable(STTUNER_Handle_t Handle)
{
    ST_ErrorCode_t Error;
    STTUNER_InstanceDbase_t *Inst;
    U32 index;
    
    /* ---------- validate handle ---------- */
    Error = STTUNER_CheckOpenHandle(Handle);
    if (Error != ST_NO_ERROR)
    return(Error);
    
    index = HANDLE2INDEX( Handle );
    Inst = STTUNER_GetDrvInst();

    /* ---------- select device ---------- */
    switch(Inst[index].Device)
    {

        case STTUNER_DEVICE_SATELLITE:
		#ifdef STTUNER_USE_SAT
	            Error = STTUNER_SAT_SetScrTone(index);
		#else
	            Error = STTUNER_ERROR_ID;
		#endif
	break;

        default:
		#ifdef STTUNER_DEBUG_MODULE_STTUNER
	                STTBX_Print(("%s no valid device %d\n", identity, Inst[index].Device));
		#endif
                Error = STTUNER_ERROR_ID;
        break;
                    
    } /* switch */

    return(Error);

} /* STTUNER_ScrTone_Enable() */
#endif
#endif
/* End of sttuner.c */
