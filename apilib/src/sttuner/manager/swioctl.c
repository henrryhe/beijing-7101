/* ----------------------------------------------------------------------------
File Name: swioctl.c

Description: 

    This module handles the selection of function calls when the ioctl 
   device is STTUNER_SOFT_ID and after sat,cable or terrestrial have
   processed the call first.
    

Copyright (C) 1999-2001 STMicroelectronics

History:
 
   date: 17-July-2001
version: 3.1.0 
 author: GJP
comment: Initial version.
    
   date: 23-August-2001
version: 3.1.0 
 author: GJP
comment: Added Probe function.
    
Reference:
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

#include "stevt.h"
#include "sttuner.h"

/* local to sttuner */
#include "util.h"       /* generic utility functions for sttuner */
#include "dbtypes.h"    /* data types for databases */
#include "sysdbase.h"   /* functions to accesss system data */


/* shared devices */
#ifdef STTUNER_USE_SHARED
 /* n/a */
#endif

/* driver API */
#include "drivers.h"
#include "ioarch.h"
#include "ioreg.h"
#include "chip.h"

#include "swioctl.h"    /* header for this file */

#define I2C_HANDLE(x)      (*((STI2C_Handle_t *)x))



/* constants --------------------------------------------------------------- */


/* ioctl functions --------------------------------------------------------- */

/* function index: STTUNER_IOCTL_ST_TEST_01 */
ST_ErrorCode_t SWIOCTL_Test_01(STTUNER_Handle_t Handle, void *InParams, void *OutParams, STTUNER_Da_Status_t *Status);
ST_ErrorCode_t SWIOCTL_Probe  (STTUNER_Handle_t Handle, void *InParams, void *OutParams, STTUNER_Da_Status_t *Status);

extern IOARCH_HandleData_t IOARCH_Handle[STTUNER_IOARCH_MAX_HANDLES];
/* ----------------------------------------------------------------------------
Name: STTUNER_SWIOCTL_Default()

Description:
    select access to specific functions common to all services
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_SWIOCTL_Default(STTUNER_Handle_t Handle, U32 Function, void *InParams, void *OutParams, STTUNER_Da_Status_t *Status)
{
#ifdef STTUNER_DEBUG_MODULE_SWIOCTL
   const char *identity = "STTUNER swioctl.c STTUNER_SWIOCTL_Default()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_InstanceDbase_t *Inst;
    BOOL NonInstance = FALSE;

    /* if STTUNER_MAX_HANDLES then the ioctl function is non-instance based */
    if (Handle >= STTUNER_MAX_HANDLES)
    {
        NonInstance = TRUE;
    }
    else
    {
        Inst = STTUNER_GetDrvInst();
        if( Inst[Handle].Status != STTUNER_DASTATUS_OPEN)
        {
#ifdef STTUNER_DEBUG_MODULE_SWIOCTL
            STTBX_Print(("%s handle %d not open\n", identity, Handle));
#endif
            return(ST_ERROR_INVALID_HANDLE);
        }
    }

#ifdef STTUNER_DEBUG_MODULE_SWIOCTL
    STTBX_Print(("%s  STTUNER handle=%d\n", identity, Handle));
    STTBX_Print(("%s        function=%d\n", identity, Function));
#endif

    /* ---------- select what to do ---------- */
    switch(Function){

        case STTUNER_IOCTL_TEST_01: /* demo test */
            if(NonInstance == FALSE)
            {
                Error = SWIOCTL_Test_01(Handle, InParams, OutParams, Status);
            }
            else
            {
                Error = ST_ERROR_INVALID_HANDLE;
            }
            break;

        case STTUNER_IOCTL_PROBE:   /* probe hardware register */
            Error = SWIOCTL_Probe(Handle, InParams, OutParams, Status);
            break;

        default:
#ifdef STTUNER_DEBUG_MODULE_SWIOCTL
            STTBX_Print(("%s function %d not found\n", identity, Function));
#endif
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

#ifdef STTUNER_DEBUG_MODULE_SWIOCTL
    STTBX_Print(("%s called ok\n", identity));  /* signal that function came back */
#endif
    return(Error);

}   /* STTUNER_SWIOCTL_Default() */



/* ========================================================================= */
/* /\/\/\/\/\/\/\/\/\/\/\/\/\IOCTL Functions/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ */
/* ========================================================================= */



/* ----------------------------------------------------------------------------
Name: SWIOCTL_Test_01()

Description:
    print message (if debugging turned on for this file) then return
    with no error.
    
Parameters:
    
Return Value:
    ST_NO_ERROR
---------------------------------------------------------------------------- */
ST_ErrorCode_t SWIOCTL_Test_01(STTUNER_Handle_t Handle, void *InParams, void *OutParams, STTUNER_Da_Status_t *Status)
{
#ifdef STTUNER_DEBUG_MODULE_SWIOCTL
   const char *identity = "STTUNER swioctl.c SWIOCTL_Test_01()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;


#ifdef STTUNER_DEBUG_MODULE_SWIOCTL
    STTBX_Print(("%s called ok\n", identity));
#endif
    return(Error);

}   /* SWIOCTL_Test_01() */



/* ----------------------------------------------------------------------------
Name: SWIOCTL_Probe()

Description:
    Probe specifies address to recover ID register value.

Parameters:
    
Return Value:
    ST_NO_ERROR
---------------------------------------------------------------------------- */
ST_ErrorCode_t SWIOCTL_Probe(STTUNER_Handle_t Handle, void *InParams, void *OutParams, STTUNER_Da_Status_t *Status)
{
    /* device I/O selection */
    typedef struct
    {
        STTUNER_IODriver_t Driver;
        ST_DeviceName_t    DriverName;
        U32                Address;
        U32                SubAddress;
        U32                Value;
        U32                XferSize;
        U32                TimeOut; 
    } SWIOCTL_ProbeParams_t;
    

#ifdef STTUNER_DEBUG_MODULE_SWIOCTL
   const char *identity = "STTUNER swioctl.c SWIOCTL_Probe()";
#endif

    ST_ErrorCode_t Err= ST_NO_ERROR, Error = ST_NO_ERROR;

    STTUNER_IOARCH_OpenParams_t  OpenParams;

    IOARCH_Handle_t              IOHandle;
    SWIOCTL_ProbeParams_t       *ProbeParams;   
    U32 Size, TransferSize;
    U8 SubAddress[2];
    


    /* init I/O manager */
    Error = ChipInit();
    
    if ((Error != ST_NO_ERROR) && (Error != STTUNER_ERROR_INITSTATE))
    {
#ifdef STTUNER_DEBUG_MODULE_SWIOCTL
        STTBX_Print(("%s fail init I/O manager\n", identity));
#endif    
        return(Error);
    }

    ProbeParams = (SWIOCTL_ProbeParams_t *)InParams;

    /* open I/O manager to address */
    OpenParams.IORoute        = STTUNER_IO_DIRECT;
    OpenParams.IODriver       = ProbeParams->Driver;
    OpenParams.Address        = ProbeParams->Address;
    OpenParams.hInstance      = NULL;
    OpenParams.TargetFunction = NULL;
    #ifdef STTUNER_DRV_SET_HIGH_I2C_BAUDRATE
    OpenParams.BaudRate = 100000;
    #endif
    strcpy(OpenParams.DriverName, ProbeParams->DriverName);

#ifdef STTUNER_DEBUG_MODULE_SWIOCTL
    STTBX_Print(("%s   OpenParams.IODriver: %d\n",     identity, OpenParams.IODriver));
    STTBX_Print(("%s    OpenParams.Address: 0x%08x\n", identity, OpenParams.Address));
    STTBX_Print(("%s OpenParams.DriverName: '%s'\n",   identity, OpenParams.DriverName));
#endif


    Error=  ChipOpen(&IOHandle, &OpenParams);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SWIOCTL
        STTBX_Print(("%s fail open I/O Error=%d\n", identity, Error));
#endif
        return(Error);
    }


if (ProbeParams->SubAddress & 0xFF00)
  {
  SubAddress[0]=(U8)(((ProbeParams->SubAddress) & 0xFF00)>>8);
  SubAddress[1]=(U8)((ProbeParams->SubAddress) & 0xFF);
  TransferSize=2;
  } 
else
 {
 SubAddress[0] = (U8)(ProbeParams->SubAddress & 0xFF);
 TransferSize=1;
 }
 
Err = STI2C_Write(I2C_HANDLE(&IOARCH_Handle[IOHandle].ExtDev),SubAddress,TransferSize,ProbeParams->TimeOut,&Size);
Err |= STI2C_Read(I2C_HANDLE(&IOARCH_Handle[IOHandle].ExtDev),(U8 *)&ProbeParams->Value,TransferSize,ProbeParams->TimeOut, &Size);

      if (Err != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SWIOCTL
        STTBX_Print(("%s fail read I/O Error=%d\n", identity, Err));
#endif
    }

    /*  close I/O instance */
    Error=ChipClose(IOHandle);
   
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SWIOCTL
        STTBX_Print(("%s fail close I/O Error=%d\n", identity, Error));
#endif
    }


#ifdef STTUNER_DEBUG_MODULE_SWIOCTL
    STTBX_Print(("%s called ok\n", identity));
#endif
    return(Error | Err);

}   /* SWIOCTL_Probe() */



/* ------------------------------------------------------------------------- */
/* /\/\/\/\/\/\/\/\/\/\/\/\/\UTILITY Functions/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ */
/* ------------------------------------------------------------------------- */



/* End of swioctl.c */



