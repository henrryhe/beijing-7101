/* -------------------------------------------------------------------------
File Name: iocom.c

Description: Present an alternate register based API to hardware
             connected on an I2C bus, this is to support shared PC/embedded
             satellite low-level algorithms.

Copyright (C) 1999-2001 STMicroelectronics

History:
   date: 10-October-2001
version: 1.0.0
 author: GP
comment: interface to ioreg.h

   date: 22-July-2004
version: 4.9.0
 author: RV
comment: made independent from ioreg.c

---------------------------------------------------------------------------- */
/* Includes ---------------------------------------------------------------- */
#ifndef ST_OSLINUX

#include <string.h>     /* for memcpy() */
#endif
#include "sttbx.h"


#include "stddef.h"     /* TOOLSET Standard definitions */
#include "stddefs.h"    /* STAPI Standard definitions */

/* STAPI */
#include "sttuner.h"
/* local to sttuner */
#include "util.h"       /* generic utility functions for sttuner */
#include "ioarch.h"     /* STTUNER driver */
#include "ioreg.h"
#include "chip.h"

#define I2C_HANDLE(x)      (*((STI2C_Handle_t *)x))


/* ST20 boot address (FLASH memory/MPX boot) */
#define BAD_RAM_ADDRESS (0x7ffffffe)
static semaphore_t* Lock_OpenClose;
static BOOL         Initalized  = FALSE;

IOARCH_HandleData_t IOARCH_Handle[STTUNER_IOARCH_MAX_HANDLES];


/* ----------------------------------------------------------------------------
Name: Chipopen()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t
ChipOpen(IOARCH_Handle_t* hChip, STTUNER_IOARCH_OpenParams_t* OpenParams)
{
   #ifdef STTUNER_DEBUG_MODULE_IO
   const char*          identity    = "STTUNER chip.c STTUNER_IOARCH_Open()";    
   #endif


   STI2C_OpenParams_t   I2cOpenParams;    
   U32                  index;
   ST_ErrorCode_t       Error       = ST_NO_ERROR;;



   
   SEM_LOCK(Lock_OpenClose);
   index = 0;

   while (IOARCH_Handle[index].IsFree != TRUE)
   {
      index++;
      if (index >= STTUNER_IOARCH_MAX_HANDLES)
      {
         #ifdef STTUNER_DEBUG_MODULE_IO
         STTBX_Print(("%s Failed.. No free handles\n", identity));
         #endif
         SEM_UNLOCK(Lock_OpenClose);
         return(ST_ERROR_NO_FREE_HANDLES);
      }
   } /* while */

   #ifdef STTUNER_DEBUG_MODULE_IO
   STTBX_Print(("%s Next free handle is %d\n", identity, index));
   #endif

   /* Pass to device driver the index to this instance to open in case it might need it,
    the return will either be this value or a handle to an external (hardware) instance of a driver. */
   IOARCH_Handle[index].ExtDev = index;

   /* DIRECT and REPEATER routes require an I/O connection to be open,
      PASSTHRU uses someone elses connection, so incorrect to open one for it */

   if (OpenParams->IORoute != STTUNER_IO_PASSTHRU)
   {
      switch (OpenParams->IODriver)
      {
         case STTUNER_IODRV_I2C:
            /* I2C */
            memset(&I2cOpenParams, '\0', sizeof(STI2C_OpenParams_t));

            /* ---------- Open an existing, Initialized I2C driver ---------- */
            I2cOpenParams.AddressType = STI2C_ADDRESS_7_BITS;
            I2cOpenParams.BusAccessTimeOut = 200; /* GNBvd07229 */
            I2cOpenParams.I2cAddress = OpenParams->Address;

            Error = STI2C_Open(OpenParams->DriverName, &I2cOpenParams, (STI2C_Handle_t *) &IOARCH_Handle[index].ExtDev);

            #ifdef STTUNER_DEBUG_MODULE_IO
            STTBX_Print(("%s handle=0x%08x\n", identity, I2C_HANDLE(IOARCH_Handle[index].ExtDev)));
            if (Error == ST_NO_ERROR)
            {
               STTBX_Print(("%s I2C opened ok\n", identity));
            }
            #endif

            #ifdef STTUNER_DRV_SET_HIGH_I2C_BAUDRATE
            Error |=    STI2C_SetBaudRate   (IOARCH_Handle[index].ExtDev, OpenParams->BaudRate);
            #endif
            #ifdef STTUNER_DEBUG_MODULE_IO
            STTBX_Print(("%s STTUNER_IODRV_I2C\n", identity));
            #endif
            break;

         case STTUNER_IODRV_DEBUG:
            /* screen */
            #ifdef STTUNER_DEBUG_MODULE_IO
            STTBX_Print(("%s Opened:\n", identity));
            STTBX_Print(("%s IOARCH handle = %d\n", identity, *(U32 *) IOARCH_Handle[index].ExtDev));
            STTBX_Print(("%s Address       = 0x%04x\n", identity, IOARCH_Handle[index].Address));
            #endif
            break;

         default:
            /* unknown, return error */
            #ifdef STTUNER_DEBUG_MODULE_IO
            STTBX_Print(("%s I/O ID not known %d \n", identity, OpenParams->IODriver));
            #endif
            
           Error = STTUNER_ERROR_ID;
            SEM_UNLOCK(Lock_OpenClose);
             return(Error);
            break;
      }   /* switch */

      if (Error != ST_NO_ERROR)
      {
         #ifdef STTUNER_DEBUG_MODULE_IO
         STTBX_Print(("%sI/O driver open() Failed! Error=%d\n", identity, Error));
         #endif
         SEM_UNLOCK(Lock_OpenClose);
         return(ST_ERROR_NO_FREE_HANDLES);
      }
   }   /* if(!STTUNER_IO_PASSTHRU) */


   /* if repeater or passthru then check function() to call for I/O operations */
   if ((OpenParams->IORoute == STTUNER_IO_REPEATER) || (OpenParams->IORoute == STTUNER_IO_PASSTHRU))
   {
      if ((OpenParams->TargetFunction == NULL) && (OpenParams->RepeaterFn == NULL))
      {
         #ifdef STTUNER_DEBUG_MODULE_IO
         STTBX_Print(("%s Failed.. invalid target function\n", identity));
         #endif
         SEM_UNLOCK(Lock_OpenClose);
         return(ST_ERROR_BAD_PARAMETER);
      }
   }

   #ifdef STTUNER_DEBUG_MODULE_IO
   STTBX_Print(("%s ", identity));
   switch (OpenParams->IORoute)
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

   *hChip = index;

   IOARCH_Handle[index].TargetFunction = OpenParams->TargetFunction;
   IOARCH_Handle[index].RepeaterFn = OpenParams->RepeaterFn;
   IOARCH_Handle[index].hInstance = OpenParams->hInstance;
   IOARCH_Handle[index].IORoute = OpenParams->IORoute; 
   IOARCH_Handle[index].IODriver = OpenParams->IODriver; 
   IOARCH_Handle[index].IsFree = FALSE;
   IOARCH_Handle[index].Address = OpenParams->Address;

   /* Handle_Pool[index]                   = IOARCH_Handle[index].ExtDev;*/

   #ifdef STTUNER_DEBUG_MODULE_IO
   STTBX_Print(("%s I/O opened ok\n", identity));
   #endif

   SEM_UNLOCK(Lock_OpenClose);

   return(Error);
}
/* ----------------------------------------------------------------------------
Name: ChipInit()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */

ST_ErrorCode_t
ChipInit(void)
{
    #ifdef STTUNER_DEBUG_MODULE_IO
   const char*      identity    = "STTUNER iocom.c ChipInit()";
   #endif
   U32  loop;

   if (Initalized == TRUE)
   {
      #ifdef STTUNER_DEBUG_MODULE_IO
      STTBX_Print((" %s Already initalized!\n",identity));
      #endif
      return(STTUNER_ERROR_INITSTATE);
   }
   /* init handles */
   for (loop = 0; loop < STTUNER_IOARCH_MAX_HANDLES; loop++)
   {
      IOARCH_Handle[loop].IsFree = TRUE;
      IOARCH_Handle[loop].ExtDev = BAD_RAM_ADDRESS;       /* set to invalid value */;
      IOARCH_Handle[loop].IODriver = STTUNER_IODRV_NULL;
      IOARCH_Handle[loop].IORoute = STTUNER_IO_DIRECT;
      IOARCH_Handle[loop].hInstance = NULL;                  /* set to invalid value */
      IOARCH_Handle[loop].TargetFunction = NULL;
      IOARCH_Handle[loop].RepeaterFn = NULL;
      IOARCH_Handle[loop].Address = 0;
      IOARCH_Handle[loop].DeviceMap.Timeout = IOREG_DEFAULT_TIMEOUT;
   }

   Lock_OpenClose = STOS_SemaphoreCreateFifo(NULL, 1);
   Initalized = TRUE;

   #ifdef STTUNER_DEBUG_MODULE_IO
   STTBX_Print(("%s Initalize success\n", identity));
   #endif

   return(ST_NO_ERROR);
}
/* ----------------------------------------------------------------------------
Name: ChipClose()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */

ST_ErrorCode_t
ChipClose(IOARCH_Handle_t hChip)
{
   #ifdef STTUNER_DEBUG_MODULE_IO
   const char*      identity    = "STTUNER ioarch.c STTUNER_IOARCH_Close()";
   #endif
   U32              index;

   ST_ErrorCode_t   Error       = ST_NO_ERROR;


   SEM_LOCK(Lock_OpenClose);
   index = hChip;

   if (index >= STTUNER_IOARCH_MAX_HANDLES)
   {
      #ifdef STTUNER_DEBUG_MODULE_IO
      STTBX_Print(("%s failed bad handle\n", identity));
      #endif
      SEM_UNLOCK(Lock_OpenClose);
      return(ST_ERROR_INVALID_HANDLE);
   }

   if (IOARCH_Handle[index].IsFree == TRUE)
   {
      #ifdef STTUNER_DEBUG_MODULE_IO
      STTBX_Print(("%s fail handle not used\n", identity));
      #endif
      SEM_UNLOCK(Lock_OpenClose);
      return(ST_ERROR_INVALID_HANDLE);
   }

   switch (IOARCH_Handle[index].IODriver)
   {
      case STTUNER_IODRV_I2C:
         /* I2C */
         if (IOARCH_Handle[index].IORoute != STTUNER_IO_PASSTHRU)
         {
            /* Clean-up previously allocated resources */
            Error = STI2C_Close(I2C_HANDLE(&IOARCH_Handle[index].ExtDev));

            #ifdef STTUNER_DEBUG_MODULE_IOARCH
            if (Error == ST_NO_ERROR)
            {
               STTBX_Print(("%s I2C handle 0x%08x closed ok\n", identity, I2C_HANDLE(I2CHandle)));
            }
            #endif
         }
         else
         {
            Error = ST_NO_ERROR; /* PASSTHRU does not open an I2C address */
         }
         #ifdef STTUNER_DEBUG_MODULE_IO
         STTBX_Print(("%s STTUNER_IODRV_I2C\n", identity));
         #endif
         break;

      case STTUNER_IODRV_DEBUG:
         /* screen */

         #ifdef STTUNER_DEBUG_MODULE_IO
         STTBX_Print(("%s STTUNER_IODRV_DEBUG\n", identity));
         #endif
         break;

      default:
         /* unknown, return error */
         #ifdef STTUNER_DEBUG_MODULE_IOARCH
         STTBX_Print(("%s Closed:\n", identity));
         STTBX_Print(("%s IOARCH handle = %d\n", identity, *(U32 *) ExtHandle));
         #endif
         break;
   }   /* switch */


   if (Error != ST_NO_ERROR)
   {
      #ifdef STTUNER_DEBUG_MODULE_IO
      STTBX_Print(("%s failed error=%d\n", identity, Error));
      #endif
      SEM_UNLOCK(Lock_OpenClose);
      return(Error);
   }

   IOARCH_Handle[index].ExtDev = BAD_RAM_ADDRESS;
   IOARCH_Handle[index].IODriver = STTUNER_IODRV_NULL;
   IOARCH_Handle[index].IsFree = TRUE;

   /* remove semaphores */

   SEM_UNLOCK(Lock_OpenClose);

 

   #ifdef STTUNER_DEBUG_MODULE_IO
   STTBX_Print(("%s I/O closed\n", identity));
   #endif

   return(ST_NO_ERROR);
}
/* ----------------------------------------------------------------------------
Name: ChipTerm()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t
ChipTerm()
{
   #ifdef STTUNER_DEBUG_MODULE_IO
   const char*      identity        = "STTUNER ioarch.c STTUNER_IOARCH_Term()";
   #endif
   ST_ErrorCode_t   RetErr, Error   = ST_NO_ERROR;
   U32              loop;

   if (Initalized == FALSE)
   {
      #ifdef STTUNER_DEBUG_MODULE_IO
      STTBX_Print(("%s Not initalized!\n", identity));
      #endif
      return(STTUNER_ERROR_INITSTATE);
   }


   /* close I/O connections, if open */
   for (loop = 0; loop < STTUNER_IOARCH_MAX_HANDLES; loop++)
   {
      if (IOARCH_Handle[loop].IsFree == FALSE)   /* found open handle */
      {
         #ifdef STTUNER_DEBUG_MODULE_IO
         STTBX_Print(("%s close I/O handle %d...", identity, loop));
         #endif
         RetErr = ChipClose(loop);
         if (RetErr != ST_NO_ERROR)
         {
            #ifdef STTUNER_DEBUG_MODULE_IO
            STTBX_Print(("failed closing I/O handle\n"));
            #endif
            Error = RetErr;
         }
         else
         {
            #ifdef STTUNER_DEBUG_MODULE_IO
            STTBX_Print(("I/O handles closed..ok\n"));
            #endif
         }
      }
   }    

   /* remove semaphores */
   STOS_SemaphoreDelete(NULL, Lock_OpenClose);



   #ifdef STTUNER_DEBUG_MODULE_IO
   STTBX_Print(("%s I/O terminated\n", identity));
   #endif
   Initalized = FALSE;
   return(Error);
}































