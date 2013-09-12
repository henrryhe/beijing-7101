
/* ----------------------------------------------------------------------------
File Name: ioarch.c

Description: 

     Simple I/O manager. This module handles low-level driver functions for I2c etc.


Copyright (C) 1999-2001 STMicroelectronics

   date: 18-June-2001
version: 3.1.0
 author: GJP from work by LW
comment: write for multi-instance.
    
   date: 17-August-2001
version: 3.1.1
 author: GJP
comment: Update SubAddr to U16.
         GNBvd07229 - request I2C bus access timeout set to 20mS rather than 1mS

   date: 10-October-2001
version: 3.1.1
 author: GJP
comment: Use STI2C_Write() instead of STI2C_WriteNoStop() for STTUNER_IO_SA_READ for
         compatability with 297 chip

   date: 22-Feb-2002
version: 3.2.0
 author: GJP
comment: Changed STTUNER_IO_SA_WRITE_NS to use STI2C_WriteNoStop() instead of STI2C_Write()
         so now it conforms to its intended effect (write with no I2C stop at the end)

   date: 4-April-2002
version: 3.4.0
 author: GJP
comment: Add functions to support changing default target function and routing (for dual STEM 299 support)

Reference:
---------------------------------------------------------------------------- */


/* Includes ---------------------------------------------------------------- */

/* C libs */
#ifndef ST_OSLINUX
   
#include <string.h>
#endif
/* Standard includes */
#include "stlite.h"

/* STAPI */
#include "stsys.h"
#include "sttbx.h"


#include "sti2c.h"
#include "stevt.h"
#include "sttuner.h"                    

/* local to sttuner */
#include "util.h"       /* generic utility functions for sttuner */
#include "dbtypes.h"    /* data types for databases */
#include "sysdbase.h"   /* functions to accesss system data */

#include "ioarch.h"     /* header for this file */

/* for casting void* to a U32 */
#define I2C_HANDLE(x)      (*((STI2C_Handle_t *)x))

/* ST20 boot address (FLASH memory/MPX boot) */
#define BAD_RAM_ADDRESS (0x7ffffffe)



/* buffer for subaddress & 'nostop' subaddress writes */
#define NSBUFFER_SIZE 100

/*#define STTUNER_DEBUG_MODULE_IOARCH*/

/*static IOARCH_HandleData_t IOARCH_Handle[STTUNER_IOARCH_MAX_HANDLES];*/
extern IOARCH_HandleData_t IOARCH_Handle[STTUNER_IOARCH_MAX_HANDLES];


static semaphore_t *Lock_OpenClose;
static semaphore_t *Lock_ReadWrite_Dbg;



static BOOL Initalized = FALSE;

#ifdef STTUNER_DRV_SAT_DUAL_STEM_299
extern U32 LnbHandleOne;
extern U32 LnbHandleTwo;
#endif

U8 PCF8574RegisterValue = 0;

/* ---------- private functions ---------- */

/* memory */
ST_ErrorCode_t Mem_OpenAddress (void *ExtHandle, ST_DeviceName_t ExtDriverName, U32 BaseAddress);
ST_ErrorCode_t Mem_CloseAddress(void *ExtHandle);
ST_ErrorCode_t Mem_ReadWrite   (void *Handle, STTUNER_IOARCH_Operation_t Operation, U16 SubAddr, U8 *Data, U32 TransferSize, U32 Timeout);

/* null */
ST_ErrorCode_t Nul_OpenAddress (void *ExtHandle, ST_DeviceName_t ExtDriverName, U32 Address);
ST_ErrorCode_t Nul_CloseAddress(void *ExtHandle);
ST_ErrorCode_t Nul_ReadWrite   (void *Handle, STTUNER_IOARCH_Operation_t Operation, U16 SubAddr, U8 *Data, U32 TransferSize, U32 Timeout);

/* I2c */
ST_ErrorCode_t I2C_OpenAddress (void *I2CHandle, ST_DeviceName_t I2CDriverName, U32 Address);
ST_ErrorCode_t I2C_CloseAddress(void *I2CHandle);
ST_ErrorCode_t I2C_ReadWrite   (void *I2CHandle, STTUNER_IOARCH_Operation_t Operation, U16 SubAddr, U8 *Data, U32 TransferSize, U32 Timeout);

/* debug */
ST_ErrorCode_t Dbg_OpenAddress (void *ExtHandle, ST_DeviceName_t ExtDriverName, U32 Address);
ST_ErrorCode_t Dbg_CloseAddress(void *ExtHandle);
ST_ErrorCode_t Dbg_ReadWrite   (void *Handle, STTUNER_IOARCH_Operation_t Operation, U16 SubAddr, U8 *Data, U32 TransferSize, U32 Timeout);


/* core function to select I/O to perform */
ST_ErrorCode_t IOARCH_ReadWrite(IOARCH_Handle_t Handle, STTUNER_IOARCH_Operation_t Operation, U16 SubAddr, U8 *Data, U32 TransferSize, U32 Timeout, BOOL Route);

/* Added for TraceDump 11 July 2k2 */
#ifdef  USE_TRACE_BUFFER_FOR_DEBUG 
  #undef USE_TRACE_BUFFER_FOR_DEBUG
#endif


#ifdef USE_TRACE_BUFFER_FOR_DEBUG
  #define DMP
  char Trace_PrintBuffer[40]; /* global variable to be seen by modules */
  #define ARR_SIZE   300
  #define ARR_LENGTH 40
  char trace_buffer[ARR_SIZE][ARR_LENGTH]; 
  static U32 uiDump_Idx = 0;
#endif /* ifdef USE_TRACE_BUFFER_FOR_DEBUG */
/* Added for TraceDump 11 July 2k2 */

	
#ifdef USE_TRACE_BUFFER_FOR_DEBUG
void DUMP_DATA(char *info)   
{ 
	if( uiDump_Idx >= (ARR_SIZE-1) )
	{	
		memset( trace_buffer, '\0', sizeof( trace_buffer) );			
	  	uiDump_Idx=0;
	}
	
	strcpy( trace_buffer[uiDump_Idx++], info ); 
}
#endif /* #ifdef USE_TRACE_BUFFER_FOR_DEBUG */




/* ----------------------------------------------------------------------------
Name: STTUNER_IOARCH_Init()

Description:
    Initalize simple I/O manager subsystem.
    
Parameters:
    dummy params at this time.
    
Return Value:
    STTUNER_ERROR_INITSTATE - Already initalized.
    ST_NO_ERROR             - No error.
    ST_ERROR_BAD_PARAMETER  - repeater or passthrough mode selected but no target function specified

See Also:
    STTUNER_IOARCH_Term()
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_IOARCH_Init(STTUNER_IOARCH_InitParams_t *InitParams)
{
#ifdef STTUNER_DEBUG_MODULE_IOARCH
    const char *identity = "STTUNER ioarch.c STTUNER_IOARCH_Init()";
    
#endif
    U32 loop;

    if (Initalized == TRUE)
    {
#ifdef STTUNER_DEBUG_MODULE_IOARCH
        STTBX_Print(("%s already initalized\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

    /* init handles */
    for(loop = 0; loop < STTUNER_IOARCH_MAX_HANDLES; loop++)
    {
        IOARCH_Handle[loop].IsFree         = TRUE;
        IOARCH_Handle[loop].ExtDev         = BAD_RAM_ADDRESS;       /* set to invalid value */;
        IOARCH_Handle[loop].IODriver       = STTUNER_IODRV_NULL;
        IOARCH_Handle[loop].IORoute        = STTUNER_IO_DIRECT;
        IOARCH_Handle[loop].hInstance      = NULL;                  /* set to invalid value */
        IOARCH_Handle[loop].TargetFunction = NULL;
        IOARCH_Handle[loop].Address        = 0;
    }


    Lock_OpenClose = STOS_SemaphoreCreateFifo(NULL,1);
    Lock_ReadWrite_Dbg = STOS_SemaphoreCreateFifo(NULL,1);



    
      

    Initalized = TRUE;
#ifdef STTUNER_DEBUG_MODULE_IOARCH
    
    STTBX_Print(("%s initalized ok \n", identity));
#endif
    return(ST_NO_ERROR);
}



/* ----------------------------------------------------------------------------
Name: STTUNER_IOARCH_Open()

Description:
    Open a connection to a specific hardware device using a specific (base) address and I/O mode.
    
Parameters:
    OpenParams - configure this I/O instance

Return Value:
    ST_ERROR_NO_FREE_HANDLES - All handles used.
    STTUNER_ERROR_ID         - Unknown target hardware.
    ST_NO_ERROR              - No error.

See Also:
    STTUNER_IOARCH_Close()
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_IOARCH_Open(IOARCH_Handle_t *Handle, STTUNER_IOARCH_OpenParams_t  *OpenParams)
{
#ifdef STTUNER_DEBUG_MODULE_IOARCH
    const char *identity = "STTUNER ioarch.c STTUNER_IOARCH_Open()";
#endif
    U32 index;
    ST_ErrorCode_t Error = ST_NO_ERROR;

    SEM_LOCK(Lock_OpenClose);
    index = 0;

    while(IOARCH_Handle[index].IsFree != TRUE)
    {
        index++;
        if (index >= STTUNER_IOARCH_MAX_HANDLES)
        {
#ifdef STTUNER_DEBUG_MODULE_IOARCH
            STTBX_Print(("%s fail no free handles\n", identity));
#endif
            SEM_UNLOCK(Lock_OpenClose);
            return(ST_ERROR_NO_FREE_HANDLES);
        }

    } /* while */

#ifdef STTUNER_DEBUG_MODULE_IOARCH
    STTBX_Print(("%s next free handle=%d\n", identity, index));
#endif

    /* Pass to device driver the index to this instance to open in case it might need it,
     the return will either be this value or a handle to an external (hardware) instance of a driver. */
    IOARCH_Handle[index].ExtDev = index;

    /* DIRECT and REPEATER routes require an I/O connection to be open,
       PASSTHRU uses someone elses connection, so incorrect to open one for it */

    if (OpenParams->IORoute != STTUNER_IO_PASSTHRU)
    {
        switch(OpenParams->IODriver)
        {
            case STTUNER_IODRV_NULL:    /* null driver */
                Error = Nul_OpenAddress( &IOARCH_Handle[index].ExtDev, OpenParams->DriverName, OpenParams->Address);
#ifdef STTUNER_DEBUG_MODULE_IOARCH
                STTBX_Print(("%s STTUNER_IODRV_NULL\n", identity));
#endif
                break;
            
            case STTUNER_IODRV_I2C:     /* I2C */
                Error = I2C_OpenAddress( &IOARCH_Handle[index].ExtDev, OpenParams->DriverName, OpenParams->Address);
                #ifdef STTUNER_DRV_SET_HIGH_I2C_BAUDRATE
                Error |= STI2C_SetBaudRate(IOARCH_Handle[index].ExtDev, OpenParams->BaudRate);
                #endif
#ifdef STTUNER_DEBUG_MODULE_IOARCH
                STTBX_Print(("%s STTUNER_IODRV_I2C\n", identity));
#endif
                break;
            
            case STTUNER_IODRV_DEBUG:   /* screen */
                Error = Dbg_OpenAddress( &IOARCH_Handle[index].ExtDev, OpenParams->DriverName, OpenParams->Address);
#ifdef STTUNER_DEBUG_MODULE_IOARCH
                STTBX_Print(("%s STTUNER_IODRV_DEBUG\n", identity));
#endif
                break;
            
            case STTUNER_IODRV_MEM:     /* memory */
                Error = Mem_OpenAddress( &IOARCH_Handle[index].ExtDev, OpenParams->DriverName, OpenParams->Address);
#ifdef STTUNER_DEBUG_MODULE_IOARCH
                STTBX_Print(("%s STTUNER_IODRV_MEM\n", identity));
#endif
                break;

            default:                    /* unknown, return error */
#ifdef STTUNER_DEBUG_MODULE_IOARCH
                STTBX_Print(("%s I/O ID not known %d \n", identity, OpenParams->IODriver));
#endif
                Error = STTUNER_ERROR_ID;
                break;
            
            }   /* switch */

        if (Error != ST_NO_ERROR)
        {
#ifdef STTUNER_DEBUG_MODULE_IOARCH
            STTBX_Print(("%s fail I/O driver open() error=%d\n", identity, Error));
#endif
            SEM_UNLOCK(Lock_OpenClose);
            return(ST_ERROR_NO_FREE_HANDLES);
        }

    }   /* if(!STTUNER_IO_PASSTHRU) */


    /* if repeater or passthru then check function() to call for I/O operations */
    if ( (OpenParams->IORoute == STTUNER_IO_REPEATER) ||
         (OpenParams->IORoute == STTUNER_IO_PASSTHRU) )
    {            
        if (OpenParams->TargetFunction == NULL)
        {
#ifdef STTUNER_DEBUG_MODULE_IOARCH
            STTBX_Print(("%s fail invalid function\n", identity));
#endif
            SEM_UNLOCK(Lock_OpenClose);
            return(ST_ERROR_BAD_PARAMETER);
        }
    }


#ifdef STTUNER_DEBUG_MODULE_IOARCH

    STTBX_Print(("%s ", identity));
    switch(OpenParams->IORoute)
    {
        case STTUNER_IO_REPEATER:
                STTBX_Print(("repeater"));
                break;
            
        case STTUNER_IO_PASSTHRU:
                STTBX_Print(("passthru"));
                break;

        case STTUNER_IO_DIRECT:
                STTBX_Print(("direct"));
                break;
        default:
                STTBX_Print(("<UNKNOWN>"));
                break;
    }
    STTBX_Print((" on handle %d\n", index));
#endif

    *Handle = index;
    
    IOARCH_Handle[index].TargetFunction  = OpenParams->TargetFunction;
    IOARCH_Handle[index].hInstance       = OpenParams->hInstance;
    IOARCH_Handle[index].IORoute         = OpenParams->IORoute; 
    IOARCH_Handle[index].IODriver        = OpenParams->IODriver; 
    IOARCH_Handle[index].IsFree          = FALSE;
    IOARCH_Handle[index].Address         = OpenParams->Address;
#ifdef STTUNER_DEBUG_MODULE_IOARCH
    STTBX_Print(("%s I/O opened ok\n", identity));
#endif
    SEM_UNLOCK(Lock_OpenClose);

    return(ST_NO_ERROR);
}
    
    
    
/* ----------------------------------------------------------------------------
Name: STTUNER_IOARCH_Close()

Description:
    Close connection to hardware.

Parameters:

Return Value:
    ST_ERROR_INVALID_HANDLE - Bad handle value or handle not in use.
    ST_NO_ERROR             - No error.
    STTUNER_ERROR_ID        - Target hardware not known (could mean possible memory corruption)
    
    Other errors are  returned by the Close() function of specific (e.g. I2c) drivers.

See Also:
    STTUNER_IOARCH_Open()
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_IOARCH_Close(IOARCH_Handle_t Handle, STTUNER_IOARCH_CloseParams_t *CloseParams)
{
#ifdef STTUNER_DEBUG_MODULE_IOARCH
    const char *identity = "STTUNER ioarch.c STTUNER_IOARCH_Close()";
#endif
    U32 index;
    ST_ErrorCode_t Error = ST_NO_ERROR;


    SEM_LOCK(Lock_OpenClose);
    index = Handle;

    if (index >= STTUNER_IOARCH_MAX_HANDLES)
    {
#ifdef STTUNER_DEBUG_MODULE_IOARCH
        STTBX_Print(("%s fail bad handle\n", identity));
#endif
        SEM_UNLOCK(Lock_OpenClose);
        return(ST_ERROR_INVALID_HANDLE);
    }

    if (IOARCH_Handle[index].IsFree == TRUE)
    {
#ifdef STTUNER_DEBUG_MODULE_IOARCH
        STTBX_Print(("%s fail handle not used\n", identity));
#endif
        SEM_UNLOCK(Lock_OpenClose);
        return(ST_ERROR_INVALID_HANDLE);

    }

    switch(IOARCH_Handle[index].IODriver)
    {
        case STTUNER_IODRV_NULL:    /* null driver */
             Error = Nul_CloseAddress(&IOARCH_Handle[index].ExtDev);
#ifdef STTUNER_DEBUG_MODULE_IOARCH
            STTBX_Print(("%s STTUNER_IODRV_NULL\n", identity));
#endif
            break;
            
        case STTUNER_IODRV_I2C:     /* I2C */
            if (IOARCH_Handle[index].IORoute != STTUNER_IO_PASSTHRU)
            {
                Error = I2C_CloseAddress(&IOARCH_Handle[index].ExtDev);
            }
            else
            {
                Error = ST_NO_ERROR; /* PASSTHRU does not open an I2C address */
            }
#ifdef STTUNER_DEBUG_MODULE_IOARCH
            STTBX_Print(("%s STTUNER_IODRV_I2C\n", identity));
#endif
            break;

        case STTUNER_IODRV_DEBUG:   /* screen */
             Error = Dbg_CloseAddress(&IOARCH_Handle[index].ExtDev);
#ifdef STTUNER_DEBUG_MODULE_IOARCH
            STTBX_Print(("%s STTUNER_IODRV_DEBUG\n", identity));
#endif
            break;
            
        case STTUNER_IODRV_MEM:     /* memory */
            Error = Mem_CloseAddress(&IOARCH_Handle[index].ExtDev);
#ifdef STTUNER_DEBUG_MODULE_IOARCH
            STTBX_Print(("%s STTUNER_IODRV_MEM\n", identity));
#endif
            break;

        default:                    /* unknown, return error */
#ifdef STTUNER_DEBUG_MODULE_IOARCH
            STTBX_Print(("%s I/O ID not known %d \n", identity, IOARCH_Handle[index].IODriver));
#endif
            Error = STTUNER_ERROR_ID;
            break;
            
    }   /* switch */


    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_IOARCH
        STTBX_Print(("%s failed error=%d\n", identity, Error));
#endif
        SEM_UNLOCK(Lock_OpenClose);
        return(Error);
    }

    IOARCH_Handle[index].ExtDev   = BAD_RAM_ADDRESS;
    IOARCH_Handle[index].IODriver = STTUNER_IODRV_NULL;
    IOARCH_Handle[index].IsFree   = TRUE;

#ifdef STTUNER_DEBUG_MODULE_IOARCH
    STTBX_Print(("%s I/O closed\n", identity));
#endif
    SEM_UNLOCK(Lock_OpenClose);

    return(ST_NO_ERROR);
}
    
    
    
/* ----------------------------------------------------------------------------
Name: STTUNER_IOARCH_Term()

Description:
    Terminate I/O manager.

Parameters:
    dummy params at this time.

Return Value:
    ST_NO_ERROR             - No error.
    STTUNER_ERROR_INITSTATE - Not initalized so cannot terminate.
See Also:
    STTUNER_IOARCH_Init()
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_IOARCH_Term(const STTUNER_IOARCH_TermParams_t *TermParams)
{
#ifdef STTUNER_DEBUG_MODULE_IOARCH
    const char *identity = "STTUNER ioarch.c STTUNER_IOARCH_Term()";
#endif
    ST_ErrorCode_t RetErr, Error = ST_NO_ERROR;
    U32 loop;
    STTUNER_IOARCH_CloseParams_t CloseParams;

    if (Initalized == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_IOARCH
        STTBX_Print(("%s not initalized\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }


    /* close I/O connections, if open */
    for(loop = 0; loop < STTUNER_IOARCH_MAX_HANDLES; loop++)
    {
        if (IOARCH_Handle[loop].IsFree == FALSE)   /* found open handle */
        {
#ifdef STTUNER_DEBUG_MODULE_IOARCH
            STTBX_Print(("%s close I/O handle %d...", identity, loop));
#endif
            RetErr = STTUNER_IOARCH_Close(loop, &CloseParams);
            if (RetErr != ST_NO_ERROR)
            {
#ifdef STTUNER_DEBUG_MODULE_IOARCH
                STTBX_Print(("fail\n"));
#endif
                Error = RetErr;
            }
            else
            {
#ifdef STTUNER_DEBUG_MODULE_IOARCH
                STTBX_Print(("ok\n"));
#endif
            }
        }
    }    

    /* remove semaphores */
    STOS_SemaphoreDelete(NULL,Lock_OpenClose);
    STOS_SemaphoreDelete(NULL,Lock_ReadWrite_Dbg);


#ifdef STTUNER_DEBUG_MODULE_IOARCH
    STTBX_Print(("%s I/O terminated\n", identity));
#endif
    Initalized = FALSE;
    return(Error);
}



/* ----------------------------------------------------------------------------
Name: STTUNER_IOARCH_ChangeTarget()

Description:
    change instance IOARCH_HandleData_t TargetFunction.
Parameters:

Return Value:
    ST_NO_ERROR - No error.
    
See Also:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_IOARCH_ChangeTarget(IOARCH_Handle_t Handle, STTUNER_IOARCH_RedirFn_t TargetFunction, IOARCH_ExtHandle_t *hInstance)
{
#ifdef STTUNER_DEBUG_MODULE_IOARCH
    const char *identity = "STTUNER ioarch.c STTUNER_IOARCH_ChangeTarget()";
#endif

    if (Handle >= STTUNER_IOARCH_MAX_HANDLES)
    {
#ifdef STTUNER_DEBUG_MODULE_IOARCH
        STTBX_Print(("%s fail bad handle\n", identity));
#endif
        return( ST_ERROR_INVALID_HANDLE );
    }

    if (IOARCH_Handle[Handle].IsFree == TRUE)
    {
#ifdef STTUNER_DEBUG_MODULE_IOARCH
        STTBX_Print(("%s fail handle not used\n", identity));
#endif
        return( ST_ERROR_INVALID_HANDLE );
    }

    if (TargetFunction == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_IOARCH
        STTBX_Print(("%s fail invalid TargetFunction\n", identity));
#endif
        return( ST_ERROR_BAD_PARAMETER );
    }

    IOARCH_Handle[Handle].TargetFunction = TargetFunction;
    IOARCH_Handle[Handle].hInstance = hInstance;

    return( ST_NO_ERROR );
}



/* ----------------------------------------------------------------------------
Name: STTUNER_IOARCH_ChangeRoute()

Description:
    change instance IOARCH_HandleData_t IORoute.
Parameters:

Return Value:
    ST_NO_ERROR - No error.
    
See Also:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_IOARCH_ChangeRoute(IOARCH_Handle_t Handle, STTUNER_IORoute_t IORoute)
{
#ifdef STTUNER_DEBUG_MODULE_IOARCH
    const char *identity = "STTUNER ioarch.c STTUNER_IOARCH_ChangeRoute()";
#endif

    if (Handle >= STTUNER_IOARCH_MAX_HANDLES)
    {
#ifdef STTUNER_DEBUG_MODULE_IOARCH
        STTBX_Print(("%s fail bad handle\n", identity));
#endif
        return( ST_ERROR_INVALID_HANDLE );
    }

    switch(IORoute)
    {
        case STTUNER_IO_REPEATER:
        case STTUNER_IO_PASSTHRU:
        case STTUNER_IO_DIRECT:
            IOARCH_Handle[Handle].IORoute = IORoute; 
            break;

        default:
#ifdef STTUNER_DEBUG_MODULE_IOARCH
            STTBX_Print(("%s fail, IORoute not valid (%u)\n", identity, IORoute));
#endif
            return( ST_ERROR_BAD_PARAMETER );
    }

    return( ST_NO_ERROR );
}



/* ----------------------------------------------------------------------------
Name: STTUNER_IOARCH_ReadWrite()

Description:
    Perform I/O to the target with I/O routing enabled
    
Parameters:

Return Value:
      ST_NO_ERROR - No error.
  
See Also:
    IOARCH_ReadWrite()
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_IOARCH_ReadWrite(IOARCH_Handle_t Handle, STTUNER_IOARCH_Operation_t Operation, U16 SubAddr, U8 *Data, U32 TransferSize, U32 Timeout)
{
  return( IOARCH_ReadWrite(Handle, Operation, SubAddr, Data, TransferSize, Timeout, TRUE) );  
}


/* ----------------------------------------------------------------------------
Name: STTUNER_IOARCH_ReadWriteNoRep()

Description:
    Perform I/O DIRECT to the target ( I/O routing disabled)
    
Parameters:

Return Value:
     ST_NO_ERROR - No error.
   
See Also:
    IOARCH_ReadWrite()
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_IOARCH_ReadWriteNoRep(IOARCH_Handle_t Handle, STTUNER_IOARCH_Operation_t Operation, U16 SubAddr, U8 *Data, U32 TransferSize, U32 Timeout)
{
    return( IOARCH_ReadWrite(Handle, Operation, SubAddr, Data, TransferSize, Timeout, FALSE) );
}



/* ----------------------------------------------------------------------------
Name: IOARCH_ReadWrite()

Description:
    Perform I/O

Parameters:

Return Value:
      ST_NO_ERROR - No error.
   
See Also:
    STTUNER_IOARCH_ReadWrite()
    STTUNER_IOARCH_ReadWriteNoRep()
---------------------------------------------------------------------------- */
ST_ErrorCode_t IOARCH_ReadWrite(IOARCH_Handle_t Handle, STTUNER_IOARCH_Operation_t Operation, U16 SubAddr, U8 *Data, U32 TransferSize, U32 Timeout, BOOL Route)
{
#ifdef STTUNER_DEBUG_MODULE_IOARCH
    const char *identity = "STTUNER ioarch.c IOARCH_ReadWrite()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    U32 index, hInstance = 0;
    index = Handle;
    
   
 
   if (IOARCH_Handle[index].hInstance != NULL)
   {
    hInstance = *IOARCH_Handle[index].hInstance; /* if REPEATER or PASSTHRU retrieve instance of fn() to call */
    }
  
   
 
    if (index >= STTUNER_IOARCH_MAX_HANDLES)
    {
#ifdef STTUNER_DEBUG_MODULE_IOARCH
        STTBX_Print(("%s fail bad handle\n", identity));
#endif
        return(ST_ERROR_INVALID_HANDLE);
    }

    if (IOARCH_Handle[index].IsFree == TRUE)
    {
#ifdef STTUNER_DEBUG_MODULE_IOARCH
        STTBX_Print(("%s fail handle not used\n", identity));
#endif
        return(ST_ERROR_INVALID_HANDLE);

    }
 
    /* obey routing path, in general the 'TargetFunction' will be call back (task recurse into fn())
     here so its a very bad idea to put any semaphore locking in this function */
    if (Route == TRUE)  
    {
        /* if repeater then pass registered instance (CallerHandle, so that the caller can pick its own IO address up) as well */
        if(IOARCH_Handle[index].IORoute == STTUNER_IO_REPEATER)
        {
            Error = (*IOARCH_Handle[index].TargetFunction)(hInstance, Handle, Operation, SubAddr, Data, TransferSize, Timeout);
            if (Error != ST_NO_ERROR)
            {
#ifdef STTUNER_DEBUG_MODULE_IOARCH
                STTBX_Print(("%s fail repeater fn() error=%d\n", identity, Error));
#endif
            }
            return(Error);
        }

        /* if passthrough then as above but no handle to I/O so pass invalid value to indicate
           passthru wanted */
        else if(IOARCH_Handle[index].IORoute == STTUNER_IO_PASSTHRU)
        {
          
            Error = (*IOARCH_Handle[index].TargetFunction)(hInstance, STTUNER_IOARCH_MAX_HANDLES, Operation, SubAddr, Data, TransferSize, Timeout);
            if (Error != ST_NO_ERROR)
            {
#ifdef STTUNER_DEBUG_MODULE_IOARCH
                STTBX_Print(("%s fail passthrough fn() error=%d\n", identity, Error));
#endif
            }
            return(Error);
        }
    }   /* if(Route) */


#ifdef DMP
   	memset( Trace_PrintBuffer, '\0', ARR_LENGTH );	
	sprintf(Trace_PrintBuffer, "CritSTRT Handle[%d] Data[%d]",Handle,*Data);  
	DUMP_DATA (Trace_PrintBuffer );
#endif
  
  

    /* ---------- if direct I/O or not obey route ---------- */

    switch(IOARCH_Handle[index].IODriver)
    {
        case STTUNER_IODRV_I2C:     /* I2C */
           Error = I2C_ReadWrite(&IOARCH_Handle[index].ExtDev, Operation, SubAddr,Data, TransferSize, Timeout);
           
#ifdef DMP                 
            memset( Trace_PrintBuffer, '\0', ARR_LENGTH );	
            sprintf(Trace_PrintBuffer, "CritEND Handle[%d] Data[%d] ",Handle,*Data);  
            DUMP_DATA (Trace_PrintBuffer );
#endif
            break;

        case STTUNER_IODRV_DEBUG:   /* screen */
            Error = Dbg_ReadWrite(&IOARCH_Handle[index].ExtDev, Operation, SubAddr, Data, TransferSize, Timeout);
            break;
        case STTUNER_IODRV_NULL:    /* null */
            Error = Nul_ReadWrite(&IOARCH_Handle[index].ExtDev, Operation, SubAddr, Data, TransferSize, Timeout);
            break;
        case STTUNER_IODRV_MEM:     /* memory */
            Error = Mem_ReadWrite(&IOARCH_Handle[index].ExtDev, Operation, SubAddr, Data, TransferSize, Timeout);
            break;

        default:                    /* unknown, return error */
#ifdef STTUNER_DEBUG_MODULE_IOARCH
            STTBX_Print(("%s I/O ID not known %d \n", identity, IOARCH_Handle[index].IODriver));
#endif
            Error = STTUNER_ERROR_ID;
            break;
            
    }   /* switch */
    


    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_IOARCH
        STTBX_Print(("%s failed error=%d\n", identity, Error));
#endif
    }

    return(Error);
 
    
}



/* ------------------------------------------------------------------------- */
/* /\/\/\/\/\/\/\/\/\/\/\/\/\/DRIVER Functions/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ */
/* ------------------------------------------------------------------------- */



/* ----------------------------------------------------------------------------
Name: I2C_OpenAddress()

Description:
    Open an instance of STI2C (STI2C must have been initalized first)

Parameters:

Return Value:
    ST_NO_ERROR - No error.
    STI2C error code

See Also:
    I2C_CloseAddress()
---------------------------------------------------------------------------- */
ST_ErrorCode_t I2C_OpenAddress(void *I2CHandle, ST_DeviceName_t I2CDriverName, U32 Address)
{
#ifdef STTUNER_DEBUG_MODULE_IOARCH
    const char *identity = "STTUNER ioarch.c I2C_OpenAddress()";
#endif
    ST_ErrorCode_t     Error;
    STI2C_OpenParams_t I2cOpenParams;

    memset(&I2cOpenParams, '\0', sizeof( STI2C_OpenParams_t ) );

    /* ---------- Open an existing, Initialized I2C driver ---------- */
    I2cOpenParams.AddressType      = STI2C_ADDRESS_7_BITS;
    I2cOpenParams.BusAccessTimeOut = 200; /* GNBvd07229 */
    I2cOpenParams.I2cAddress       = Address;

#ifdef STTUNER_DEBUG_MODULE_IOARCH
    STTBX_Print(("%s Opening I2C device '%s' address 0x%04x\n", identity, I2CDriverName, Address));
#endif

    Error = STI2C_Open(I2CDriverName, &I2cOpenParams, (STI2C_Handle_t *)I2CHandle);
    
#ifdef STTUNER_DEBUG_MODULE_IOARCH
    STTBX_Print(("%s handle=0x%08x\n", identity, I2C_HANDLE(I2CHandle) ));
    if (Error == ST_NO_ERROR)
    {
        STTBX_Print(("%s I2C opened ok\n", identity));
    }
#endif
    return(Error);
}



/* ----------------------------------------------------------------------------
Name: I2C_CloseAddress()

Description:
    Close STI2C instance.
Parameters:

Return Value:
    ST_NO_ERROR - No error.
    STI2C error code
    
See Also:
    I2C_OpenAddress()
---------------------------------------------------------------------------- */
ST_ErrorCode_t I2C_CloseAddress(void *I2CHandle)
{
#ifdef STTUNER_DEBUG_MODULE_IOARCH
    const char *identity = "STTUNER ioarch.c I2C_CloseAddress()";
#endif
    ST_ErrorCode_t Error;

    /* Clean-up previously allocated resources */
    Error = STI2C_Close( I2C_HANDLE(I2CHandle) );

#ifdef STTUNER_DEBUG_MODULE_IOARCH
    if (Error == ST_NO_ERROR)
    {
        STTBX_Print(("%s I2C handle 0x%08x closed ok\n", identity, I2C_HANDLE(I2CHandle) ));
    }
#endif
    return(Error);
}



/* ----------------------------------------------------------------------------
Name: I2C_ReadWrite()

Description:
    Perform I/O using STI2C driver.
    
Parameters:

Return Value:
    ST_NO_ERROR - No error.
    STI2C error code

See Also:
---------------------------------------------------------------------------- */
ST_ErrorCode_t I2C_ReadWrite(void *I2CHandle, STTUNER_IOARCH_Operation_t Operation, U16 SubAddr, U8 *Data, U32 TransferSize, U32 Timeout)
{
#ifdef STTUNER_DEBUG_MODULE_IOARCH
    const char *identity = "STTUNER ioarch.c I2C_ReadWrite()";
    U32 loop;
#endif

    ST_ErrorCode_t Error = ST_NO_ERROR;
    U32 Size;
    U8  SubAddress, SubAddress16bit[2]={0};
    U8  nsbuffer[NSBUFFER_SIZE];
    BOOL ADR16_FLAG=FALSE;
    
    if(SubAddr & 0xFF00)
    ADR16_FLAG = TRUE;
    else
    ADR16_FLAG=FALSE;
    

#ifdef STTUNER_DEBUG_MODULE_IOARCH
    STTBX_Print(("%s h[0x%08x] saddr[0x%04x] ts[%d] to[%d] op[", identity, I2C_HANDLE(I2CHandle), SubAddr , TransferSize, Timeout));
    switch (Operation)
    {
        case STTUNER_IO_SA_READ_NS:
            STTBX_Print(("NS, "));
            /* fallthrough (no break;) */
        case STTUNER_IO_SA_READ:
            STTBX_Print(("SA READ, "));
            /* fallthrough (no break;) */

        case STTUNER_IO_READ:
            STTBX_Print(("READ\n"));
            break;

        case STTUNER_IO_SA_WRITE_NS:
            STTBX_Print(("NS, "));
            /* fallthrough (no break;) */
        case STTUNER_IO_SA_WRITE:
            STTBX_Print(("SA WRITE, "));
            /* fallthrough (no break;) */

        case STTUNER_IO_WRITE:
            STTBX_Print(("WRITE\n"));
            break;

        default:
            STTBX_Print(("??? "));
            break;
    }

    STTBX_Print(("] data in["));
    if (TransferSize > 0)
    {
        for(loop = 0; loop < TransferSize; loop++)
        {
            STTBX_Print(("0x%02x ", (U8)Data[loop] ));
        }
    }
    STTBX_Print(("]\n"));
#endif

    if(ADR16_FLAG == FALSE)
    {SubAddress = (U8)(SubAddr & 0xFF);}
    else
    {
    	SubAddress16bit[0]=(U8)((SubAddr & 0xFF00)>>8);
    	SubAddress16bit[1]=(U8)(SubAddr & 0xFF);
    }

    switch (Operation)
    {
        /* ---------- Read ---------- */
        case STTUNER_IO_SA_READ_NS:
            Error = STI2C_WriteNoStop(I2C_HANDLE(I2CHandle),&SubAddress, 1, Timeout, &Size);
#ifdef STTUNER_DEBUG_DTV_I2C_QUIET_MODE
            if (Error == STTUNER_ERROR_DTV_I2C_QUIET_MODE)
            {
            	Error =ST_NO_ERROR;
            }
#endif
            #if defined(ST_OSLINUX) && defined(STTUNER_TUNER_MT2060)/*MT2060 requires NO STOP condition which is not supported in LINUX I2C*/
            #else
            if (Error == ST_NO_ERROR)
            {
                Error = STI2C_Read(I2C_HANDLE(I2CHandle), Data,TransferSize, Timeout, &Size);
#ifdef STTUNER_DEBUG_DTV_I2C_QUIET_MODE
            if (Error == STTUNER_ERROR_DTV_I2C_QUIET_MODE)
            {
            	Error =ST_NO_ERROR;
            }
#endif
                }
            #endif    
            break;

        case STTUNER_IO_SA_READ:
            if(ADR16_FLAG == FALSE)
            {Error = STI2C_Write(I2C_HANDLE(I2CHandle), &SubAddress, 1, Timeout, &Size); /* fix for cable (297 chip) */}
            else
            {Error = STI2C_Write(I2C_HANDLE(I2CHandle), SubAddress16bit, 2, Timeout, &Size); /* To handle 16 bit subaddress */}
#ifdef STTUNER_DEBUG_DTV_I2C_QUIET_MODE
            if (Error == STTUNER_ERROR_DTV_I2C_QUIET_MODE)
            {
            	Error =ST_NO_ERROR;
            }
#endif
            /* fallthrough (no break;) */
        case STTUNER_IO_READ:
            if (Error == ST_NO_ERROR)
                Error = STI2C_Read(I2C_HANDLE(I2CHandle), Data,TransferSize, Timeout, &Size);
#ifdef STTUNER_DEBUG_DTV_I2C_QUIET_MODE
            if (Error == STTUNER_ERROR_DTV_I2C_QUIET_MODE)
            {
            	Error =ST_NO_ERROR;
            }
#endif                
            break;

        /* ---------- Write ---------- */
        case STTUNER_IO_SA_WRITE_NS:
            if (TransferSize >= NSBUFFER_SIZE)  /* use auto buffer for speed */
            {
#ifdef STTUNER_DEBUG_MODULE_IOARCH  /* the down side is of course in the size you can send */
                STTBX_Print(("%s fail NSBUFFER_SIZE=%d TransferSize=%d\n", identity, NSBUFFER_SIZE, TransferSize));
#endif
                return(STTUNER_ERROR_UNSPECIFIED);
            }
            if(ADR16_FLAG == FALSE)
            {nsbuffer[0] = SubAddress;
            memcpy( (nsbuffer + 1), Data, TransferSize);
            Error = STI2C_WriteNoStop(  I2C_HANDLE(I2CHandle), nsbuffer, TransferSize+1, Timeout, &Size);
            }
            else
            {
            	nsbuffer[0] = SubAddress16bit[0];
             	nsbuffer[1] = SubAddress16bit[1];
             	memcpy( (nsbuffer + 2), Data, TransferSize);
             	Error = STI2C_WriteNoStop(  I2C_HANDLE(I2CHandle), nsbuffer, TransferSize+2, Timeout, &Size);
            }
            /*memcpy( (nsbuffer + 1), Data, TransferSize);*/
            /*Error = STI2C_WriteNoStop(  I2C_HANDLE(I2CHandle), nsbuffer, TransferSize+1, Timeout, &Size);*/
#ifdef STTUNER_DEBUG_DTV_I2C_QUIET_MODE
            if (Error == STTUNER_ERROR_DTV_I2C_QUIET_MODE)
            {
            	Error =ST_NO_ERROR;
            }
#endif
            break;

        case STTUNER_IO_SA_WRITE:
            if (TransferSize >= NSBUFFER_SIZE)  /* use auto buffer for speed */
            {
#ifdef STTUNER_DEBUG_MODULE_IOARCH  /* the down side is of course in the size you can send */
                STTBX_Print(("%s fail NSBUFFER_SIZE=%d TransferSize=%d\n", identity, NSBUFFER_SIZE, TransferSize));
#endif
                return(STTUNER_ERROR_UNSPECIFIED);
            }
            if(ADR16_FLAG == FALSE)
            {
            	nsbuffer[0] = SubAddress;
	        memcpy( (nsbuffer + 1), Data, TransferSize);
	        Error = STI2C_Write(  I2C_HANDLE(I2CHandle), nsbuffer, TransferSize+1, Timeout, &Size);
            }
            else
            {
            	nsbuffer[0] = SubAddress16bit[0];
             	nsbuffer[1] = SubAddress16bit[1];
             	memcpy( (nsbuffer + 2), Data, TransferSize);
             	Error = STI2C_Write(  I2C_HANDLE(I2CHandle), nsbuffer, TransferSize+2, Timeout, &Size);
            }
            /*nsbuffer[0] = SubAddress;
            memcpy( (nsbuffer + 1), Data, TransferSize);
            Error = STI2C_Write(  I2C_HANDLE(I2CHandle), nsbuffer, TransferSize+1, Timeout, &Size);*/
#ifdef STTUNER_DEBUG_DTV_I2C_QUIET_MODE
            if (Error == STTUNER_ERROR_DTV_I2C_QUIET_MODE)
            {
            	Error =ST_NO_ERROR;
            }
#endif
            break;

        case STTUNER_IO_WRITE:
            if (Error == ST_NO_ERROR)
                Error = STI2C_Write(  I2C_HANDLE(I2CHandle), Data, TransferSize, Timeout, &Size);
#ifdef STTUNER_DEBUG_DTV_I2C_QUIET_MODE
            if (Error == STTUNER_ERROR_DTV_I2C_QUIET_MODE)
            {
            	Error =ST_NO_ERROR;
            }
#endif                
            break;

        /* ---------- Error ---------- */
        default:
#ifdef STTUNER_DEBUG_MODULE_IOARCH
            STTBX_Print(("%s not in STTUNER_IOARCH_Operation_t %d\n", identity, Operation));
#endif
            Error = STTUNER_ERROR_ID;
            break;
    }

#ifdef STTUNER_DEBUG_MODULE_IOARCH
    STTBX_Print(("%s data out[",identity ));
    if (TransferSize > 0)
    {
        for(loop = 0; loop < TransferSize; loop++)
        {
            STTBX_Print(("0x%02x ", (U8)Data[loop] ));
        }
    }
    STTBX_Print(("]\n"));
#endif

#ifdef STTUNER_DEBUG_MODULE_IOARCH
    switch(Error)
    {
        case ST_NO_ERROR:
           STTBX_Print(("%s ST_NO_ERROR\n", identity ));
           break;

        case STI2C_ERROR_BASE:
           STTBX_Print(("%s STI2C_ERROR_BASE\n", identity ));
           break;

        case STI2C_ERROR_LINE_STATE:
           STTBX_Print(("%s STI2C_ERROR_LINE_STATE\n", identity ));
           break;

        case STI2C_ERROR_STATUS:
           STTBX_Print(("%s STI2C_ERROR_STATUS\n", identity ));
           break;

        case STI2C_ERROR_ADDRESS_ACK:
           STTBX_Print(("%s STI2C_ERROR_ADDRESS_ACK\n", identity ));
           break;

        case STI2C_ERROR_WRITE_ACK:
           STTBX_Print(("%s STI2C_ERROR_WRITE_ACK\n", identity ));
           break;

        case STI2C_ERROR_NO_DATA:
           STTBX_Print(("%s STI2C_ERROR_NO_DATA\n", identity ));
           break;

        case STI2C_ERROR_PIO:
           STTBX_Print(("%s STI2C_ERROR_PIO\n", identity ));
           break;

        case STI2C_ERROR_BUS_IN_USE:
           STTBX_Print(("%s STI2C_ERROR_BUS_IN_USE\n", identity ));
           break;

        case STI2C_ERROR_EVT_REGISTER:
           STTBX_Print(("%s STI2C_ERROR_EVT_REGISTER\n", identity ));
           break;

        case ST_ERROR_BAD_PARAMETER:
           STTBX_Print(("%s ST_ERROR_BAD_PARAMETER\n", identity ));
           break;

        case ST_ERROR_NO_MEMORY:
           STTBX_Print(("%s ST_ERROR_NO_MEMORY\n", identity ));
           break;

        case ST_ERROR_UNKNOWN_DEVICE:
           STTBX_Print(("%s ST_ERROR_UNKNOWN_DEVICE\n", identity ));
           break;

        case ST_ERROR_ALREADY_INITIALIZED:
           STTBX_Print(("%s ST_ERROR_ALREADY_INITIALIZED\n", identity ));
           break;

        case ST_ERROR_NO_FREE_HANDLES:
           STTBX_Print(("%s ST_ERROR_NO_FREE_HANDLES\n", identity ));
           break;

        case ST_ERROR_OPEN_HANDLE:
           STTBX_Print(("%s ST_ERROR_OPEN_HANDLE\n", identity ));
           break;

        case ST_ERROR_INVALID_HANDLE:
           STTBX_Print(("%s ST_ERROR_INVALID_HANDLE\n", identity ));
           break;

        case ST_ERROR_FEATURE_NOT_SUPPORTED:
           STTBX_Print(("%s ST_ERROR_FEATURE_NOT_SUPPORTED\n", identity ));
           break;

        case ST_ERROR_INTERRUPT_INSTALL:
           STTBX_Print(("%s ST_ERROR_INTERRUPT_INSTALL\n", identity ));
           break;

        case ST_ERROR_INTERRUPT_UNINSTALL:
           STTBX_Print(("%s ST_ERROR_INTERRUPT_UNINSTALL\n", identity ));
           break;

        case ST_ERROR_TIMEOUT:
           STTBX_Print(("%s ST_ERROR_TIMEOUT\n", identity ));
           break;

        case ST_ERROR_DEVICE_BUSY:
           STTBX_Print(("%s ST_ERROR_DEVICE_BUSY\n", identity ));
           break;

        default:
           STTBX_Print(("%s Not I2C error code\n", identity ));
           break;

    }
#endif

    return(Error);
}



/* ----------------------------------------------------------------------------
Names:  Nul_OpenAddress()
        Nul_CloseAddress()
        Nul_ReadWrite()

Description:
    Null driver functions.
    
Parameters:

Return Value:
     ST_NO_ERROR - No error.
   
See Also:
---------------------------------------------------------------------------- */
ST_ErrorCode_t Nul_OpenAddress(void *ExtHandle, ST_DeviceName_t ExtDriverName, U32 Address)
{
    return(ST_NO_ERROR);
}

ST_ErrorCode_t Nul_CloseAddress(void *ExtHandle)
{
    return(ST_NO_ERROR);
}

ST_ErrorCode_t Nul_ReadWrite(void *Handle, STTUNER_IOARCH_Operation_t Operation, U16 SubAddr, U8 *Data, U32 TransferSize, U32 Timeout)
{
    return(ST_NO_ERROR);
}



/* ----------------------------------------------------------------------------
Name: Dbg_OpenAddress()

Description:
    Open debug connection (faked instance open)
    
Parameters:

Return Value:
    ST_NO_ERROR - No error.

See Also:
---------------------------------------------------------------------------- */
ST_ErrorCode_t Dbg_OpenAddress(void *ExtHandle, ST_DeviceName_t ExtDriverName, U32 Address)
{
#ifdef STTUNER_DEBUG_MODULE_IOARCH
    const char *identity = "STTUNER ioarch.c Dbg_OpenAddress()";

    STTBX_Print(("%s Opened:\n", identity ));
    STTBX_Print(("%s IOARCH handle = %d\n",     identity, *(U32 *)ExtHandle     ));
    STTBX_Print(("%s Driver name   = %s\n",     identity,         ExtDriverName ));
    STTBX_Print(("%s Address       = 0x%04x\n", identity,         Address       ));
#endif
    return(ST_NO_ERROR);
}



/* ----------------------------------------------------------------------------
Name: Dbg_CloseAddress()

Description:
    Close faked debug handle.
Parameters:

Return Value:
    ST_NO_ERROR - No error.

See Also:
---------------------------------------------------------------------------- */
ST_ErrorCode_t Dbg_CloseAddress(void *ExtHandle)
{
#ifdef STTUNER_DEBUG_MODULE_IOARCH
    const char *identity = "STTUNER ioarch.c Dbg_CloseAddress()";

    STTBX_Print(("%s Closed:\n", identity ));
    STTBX_Print(("%s IOARCH handle = %d\n", identity, *(U32 *)ExtHandle));
#endif
    return(ST_NO_ERROR);
}



/* ----------------------------------------------------------------------------
Name: Dbg_ReadWrite()

Description:
    
Parameters:

Return Value:
    ST_NO_ERROR - No error.

See Also:
---------------------------------------------------------------------------- */
ST_ErrorCode_t Dbg_ReadWrite(void *Handle, STTUNER_IOARCH_Operation_t Operation, U16 SubAddr, U8 *Data, U32 TransferSize, U32 Timeout)
{
#ifdef STTUNER_DEBUG_MODULE_IOARCH
    const char *identity = "STTUNER ioarch.c Dbg_ReadWrite()";
#else
    const char *identity = "I/O ==>";
#endif
    U32 loop, index;

    if(identity)
    {
        /* get rid of annoying warning if no o/p to sttbx, will be optimized out anyway */
    }

    SEM_LOCK(Lock_ReadWrite_Dbg);

    index = *(U32 *)Handle;

    STTBX_Print(("%s Handle=%d ", identity, index));
    STTBX_Print(("Address=%d (0x%04x) ", IOARCH_Handle[index].Address, IOARCH_Handle[index].Address));
    STTBX_Print(("SubAddr=%d (0x%04x) ", SubAddr, SubAddr));
    STTBX_Print(("Operation="));
    switch (Operation)
    {
        case STTUNER_IO_SA_READ_NS:
            STTBX_Print(("NS, "));
            /* fallthrough (no break;) */
        case STTUNER_IO_SA_READ:
            STTBX_Print(("SA READ, "));
            /* fallthrough (no break;) */

        case STTUNER_IO_READ:
            STTBX_Print(("READ\n"));
            break;

        case STTUNER_IO_SA_WRITE_NS:
            STTBX_Print(("NS, "));
            /* fallthrough (no break;) */
        case STTUNER_IO_SA_WRITE:
            STTBX_Print(("SA WRITE, "));
            /* fallthrough (no break;) */

        case STTUNER_IO_WRITE:
            STTBX_Print(("WRITE\n"));
            break;

        default:
            STTBX_Print(("???\n"));
            break;
    }

    STTBX_Print(("Data (%d) ", TransferSize));

    if (TransferSize > 0)
    {
        for(loop = 0; loop < TransferSize; loop++)
        {
            STTBX_Print(("0x%02x ", (U8)Data[loop] ));
        }
    }
    STTBX_Print(("\n"));

    SEM_UNLOCK(Lock_ReadWrite_Dbg);    
    return(ST_NO_ERROR);
}



/* ----------------------------------------------------------------------------
Name: Mem_OpenAddress()

Description:
    Dummy function to open address to basic flat memory access.

Parameters:
    ExtHandle     - Not used.
    ExtDriverName - Not used.
    BaseAddress   - Not used.

Return Value:
    ST_NO_ERROR - No error.

See Also:
---------------------------------------------------------------------------- */
ST_ErrorCode_t Mem_OpenAddress(void *ExtHandle, ST_DeviceName_t ExtDriverName, U32 BaseAddress)
{
#ifdef STTUNER_DEBUG_MODULE_IOARCH
    const char *identity = "STTUNER ioarch.c Mem_OpenAddress()";

    STTBX_Print(("%s Opened:\n", identity ));
    STTBX_Print(("%s IOARCH handle = %d\n",     identity, *(U32 *)ExtHandle     ));
    STTBX_Print(("%s Driver name   = %s\n",     identity,         ExtDriverName ));
    STTBX_Print(("%s BaseAddress   = 0x%04x\n", identity,         BaseAddress   ));
#endif
    return(ST_NO_ERROR);
}



/* ----------------------------------------------------------------------------
Name: Mem_CloseAddress()

Description:
    Dummy function to close address

Parameters:
    ExtHandle   - Not used.

Return Value:
    ST_NO_ERROR - No error.

See Also:
---------------------------------------------------------------------------- */
ST_ErrorCode_t Mem_CloseAddress(void *ExtHandle)
{
#ifdef STTUNER_DEBUG_MODULE_IOARCH
    const char *identity = "STTUNER ioarch.c Mem_CloseAddress()";

    STTBX_Print(("%s Closed IOARCH handle=%d\n", identity, *(U32 *)ExtHandle));
#endif
    return(ST_NO_ERROR);
}



/* ----------------------------------------------------------------------------
Name: Mem_ReadWrite()

Description:
    
Parameters:

Return Value:
    ST_NO_ERROR - No error.
    ST_ERROR_BAD_PARAMETER - wrong transfer size or unknown operation selected

See Also:
---------------------------------------------------------------------------- */
ST_ErrorCode_t Mem_ReadWrite(void *Handle, STTUNER_IOARCH_Operation_t Operation, U16 SubAddr, U8 *Data, U32 TransferSize, U32 Timeout)
{
#ifdef STTUNER_DEBUG_MODULE_IOARCH
    const char *identity = "STTUNER ioarch.c Mem_ReadWrite()";
#endif
    U32 index, address=0;
    U8 *Data1;
    U16 *Data2;
    U32 *Data4;

    /* FYI: from IOARCH_Open(): IOARCH_Handle[index].ExtDev = index to instance = *(U32 *)Handle */
    index = *(U32 *)Handle;

    if (Operation != STTUNER_IO_WRITE)
    {
        switch(TransferSize)
        {
            case 1:
            case 2:
            case 4:
                address = IOARCH_Handle[index].Address;
#ifdef STTUNER_DEBUG_MODULE_IOARCH
                STTBX_Print(("%s address:0x%08x SubAddr:0x%04x\n", identity, address, SubAddr));
#endif
                address = address + SubAddr;
                break;

            default:
#ifdef STTUNER_DEBUG_MODULE_IOARCH
                STTBX_Print(("%s fail transfer size must be 1, 2 or 4 bytes\n", identity));
#endif
                return(ST_ERROR_BAD_PARAMETER);

        }  /* switch(TransferSize) */
    }

    switch(Operation)
    {
        case STTUNER_IO_SA_READ:
        case STTUNER_IO_READ:
            switch(TransferSize)
            {
                case 1:
                    *(U8  *)Data = STSYS_ReadRegDev8(address);
#ifdef STTUNER_DEBUG_MODULE_IOARCH
                    STTBX_Print(("%s read8 0x%02x from address 0x%08x\n", identity,  *(U8  *)Data, address));
#endif
                    break;

                case 2:
                    *(U16 *)Data = STSYS_ReadRegDev16LE(address);
#ifdef STTUNER_DEBUG_MODULE_IOARCH
                    STTBX_Print(("%s read16 0x%04x from address 0x%08x\n", identity, *(U16 *)Data, address));
#endif
                    break;

                case 4:
                    *(U32 *)Data = STSYS_ReadRegDev32LE(address);
#ifdef STTUNER_DEBUG_MODULE_IOARCH
                    STTBX_Print(("%s read32 0x%08x from address 0x%08x\n", identity, *(U32 *)Data, address));
#endif
                    break;
            }
            break;  /* Operation */

        case STTUNER_IO_SA_WRITE:
            switch(TransferSize)
            {
                case 1:
                    STSYS_WriteRegDev8(address, *(U8  *)Data);
#ifdef STTUNER_DEBUG_MODULE_IOARCH
                    STTBX_Print(("%s sa write8 0x%02x to address 0x%08x\n", identity, *(U8  *)Data, address));
#endif
                    break;

                case 2:
                    STSYS_WriteRegDev16LE(address, *(U16 *)Data);
#ifdef STTUNER_DEBUG_MODULE_IOARCH
                    STTBX_Print(("%s sa write16 0x%04x to address 0x%08x\n", identity, *(U16  *)Data, address));
#endif
                    break;

                case 4:
                    STSYS_WriteRegDev32LE(address, *(U32 *)Data);
#ifdef STTUNER_DEBUG_MODULE_IOARCH
                    STTBX_Print(("%s sa write32 0x%08x to address 0x%08x\n", identity, *(U32  *)Data, address));
#endif
                    break;
            }
            break;  /* Operation */


        case STTUNER_IO_WRITE:
            switch(TransferSize)
            {
                case 2:
                    Data1 = (U8 *) Data;
                    /*address = IOARCH_Handle[index].Address + *((U8 *)Data[0]);
                    STSYS_WriteRegDev8(address, *((U8  *)Data[1]));*/
                    address = IOARCH_Handle[index].Address + *Data1++;
                    STSYS_WriteRegDev8(address, *Data1);
#ifdef STTUNER_DEBUG_MODULE_IOARCH
                    STTBX_Print(("%s write8 0x%02x to address 0x%08x\n", identity,  *(U8  *)Data[1], address));
#endif
                    break;

                case 4:
                    Data2 = (U16 *) Data;
                    /*address = IOARCH_Handle[index].Address + *(U16  *)Data[0];
                    STSYS_WriteRegDev16LE(address, *(U16  *)Data[2]);*/
                    address = IOARCH_Handle[index].Address + *Data2++;
                    STSYS_WriteRegDev8(address, *Data2);
#ifdef STTUNER_DEBUG_MODULE_IOARCH
                    STTBX_Print(("%s write16 0x%04x to address 0x%08x\n", identity, *(U16 *)Data[2], address));
#endif
                    break;

                case 8:
                    Data4 = (U32 *) Data;
                   /* address = IOARCH_Handle[index].Address + *(U32 *)Data[0];
                    STSYS_WriteRegDev32LE(address, *(U32  *)Data[4]);*/
                    address = IOARCH_Handle[index].Address + *Data4++;
                    STSYS_WriteRegDev8(address, *Data4);
#ifdef STTUNER_DEBUG_MODULE_IOARCH
                    STTBX_Print(("%s write32 0x%08x to address 0x%08x\n", identity, *(U32 *)Data[4], address));
#endif
                    break;

                default:
#ifdef STTUNER_DEBUG_MODULE_IOARCH
                    STTBX_Print(("%s fail transfer size (%d) must be 2, 4 or 8 bytes for STTUNER_IO_WRITE\n", identity, TransferSize));
#endif
                    return(ST_ERROR_BAD_PARAMETER);
            }
            break;  /* Operation */


        default:
#ifdef STTUNER_DEBUG_MODULE_IOARCH
            STTBX_Print(("%s fail unknown operation\n", identity));
#endif
            return(ST_ERROR_BAD_PARAMETER);
    }        

    return(ST_NO_ERROR);
}

/* ----------------------------------------------------------------------------
Name: STTUNER_IOARCH_DirectI2C()

Description:This function STTUNER_IOARCH_DirectToI2C()for dual stem 
   added to enable the scanning algorithm work properly when demod
   is in repeater mode , instead of STTUNER_IOARCH_ReadWriteNoRep().
   Here direct I2C readwrite is done 
    
Parameters: Handle, Operation mode, SubAddr, Data, TransferSize, Timeout

Return Value:
    ST_NO_ERROR - No error.
    Error return by I2CReadWrite()- when it is not ST_NO_ERROR

See Also:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_IOARCH_DirectToI2C(IOARCH_Handle_t Handle,
                                          STTUNER_IOARCH_Operation_t Operation,
                                          U16 SubAddr,
                                          U8 *Data,
                                          U32 TransferSize,
                                          U32 Timeout)
{
#ifdef STTUNER_DEBUG_MODULE_IOARCH
    const char *identity = "STTUNER ioarch.c STTUNER_IOARCH_DirectToI2C()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    U32 index = Handle;
    
    /******make I2C readwrite for demod****************/
    Error = I2C_ReadWrite(&IOARCH_Handle[index].ExtDev, Operation, SubAddr, Data, TransferSize, Timeout);

    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_IOARCH
        STTBX_Print(("%s failed error=%d\n", identity, Error));
#endif
    }
    return(Error);
}

/* End of ioarch.c */
