/*******************************************************************************

File name   : tbx_init.c

Description : Source of standard API features of toolbox API

COPYRIGHT (C) STMicroelectronics 2003.

Date               Modification                                     Name
----               ------------                                     ----
11 Mar 1999         Created                                          HG
29 Apr 1999         New architecture of API functions                HG
11 Aug 1999         Change Semaphores structure (now one semaphore
                    for each I/O device).                            HG
                    Implement UART input/output                      HG
20 Aug 1999         Added SupportedDevices                           HG
 7 Oct 1999         Add Init/Term semaphore protection               HG
19 Oct 1999         Change Semaphores structure (now two sem
                    for each I/O device: one In and one Out)         HG
 3 Mar 2000         Corrected problem with too long device names     HG
28 Jan 2002         DDTS GNBvd10702 "Use ST_GetClocksPerSecond       HS
 *                  instead of Hardcoded value"
 *                  Add STTBX_NO_UART compilation flag.
02 Jul 2002         Reset sttbx_GlobalCurrentOutputDevice and
 *                  sttbx_GlobalCurrentInputDevice at termination    HS
*******************************************************************************/
#if !defined ST_OSLINUX
/* Under Linux, STTBX functions are defined in STOS vob */

/* Includes ----------------------------------------------------------------- */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#ifdef ST_OS20
#include <debug.h>
#endif /* ST_OS20*/
#if defined(ST_OS21) && !defined(ST_OSWINCE)
#include <os21debug.h>
#endif	/* ST_OS21 && !ST_OSWINCE*/
#include "stddefs.h"
#include "stcommon.h"
#include "sttbx.h"
#include "tbx_init.h"


/* Private Types ------------------------------------------------------------ */


/* Private Constants -------------------------------------------------------- */

#define STTBX_MAX_DEVICE 1

#define INVALID_DEVICE_INDEX (-1)

#define TASK_DESCHEDULING_TIME    (sttbx_ClocksPerSecond / 10) /* 0.1s */

/* Private Variables (static)------------------------------------------------ */

static sttbx_CallingDevice_t DeviceArray[STTBX_MAX_DEVICE];
static semaphore_t *FctAccessCtrl_p;   /* Init/Term protection semaphore */
static BOOL FirstInitDone = FALSE;       /* first Toolbox initialization */
/* Supported devices according to the initialisation */

/* Global Variables --------------------------------------------------------- */

/* print table */
sttbx_PrintTable_t **sttbx_PrintTable_p;
U8 sttbx_PrintTableSize;
semaphore_t *sttbx_LockPrintTable_p;

 /* GlobalPrintID semaphore */
semaphore_t * sttbx_GlobalPrintIdMutex_p;


/* Report locking semaphore: reports, done with 2 calls, cannot be cut in 2 ! */
semaphore_t *sttbx_LockReport_p;

/* Flushing buffer locking semaphore */
semaphore_t *sttbx_WakeUpFlushingBuffer_p;

/* Input/Output devices locking semaphores:
  -one for input and one for output if input and output are independent (simultaneously supported)
  -one for both input and output if they are not independent (they don't work simultaneously) */
semaphore_t *sttbx_LockDCUIn_p;    /* DCU input is independant from output */
semaphore_t *sttbx_LockDCUOut_p;   /* DCU output is independant from input */
#ifndef STTBX_NO_UART
semaphore_t *sttbx_LockUartIn_p;   /* UART should be full-duplex so that input is independant from output */
semaphore_t *sttbx_LockUartOut_p;  /* UART should be full-duplex so that output is independant from input */
semaphore_t *sttbx_LockUart2In_p;  /* UART2 should be full-duplex so that input is independant from output */
semaphore_t *sttbx_LockUart2Out_p; /* UART2 should be full-duplex so that output is independant from input */
#endif /* #ifndef STTBX_NO_UART  */
#if 0 /* Not supported yet */
semaphore_t *sttbx_LockI1284_p;
#endif /* Not supported yet */

/* Handles on opened drivers for I/O devices which require some */
#ifndef STTBX_NO_UART
STUART_Handle_t sttbx_GlobalUartHandle;
STUART_Handle_t sttbx_GlobalUart2Handle;
#endif /* #ifndef STTBX_NO_UART  */
#if 0 /* Not supported yet */
STI1284_Handle_t GlobalI1284Handle;
#endif /* Not supported yet */


/* Current input/output devices, seen in all the toolbox at any time */
STTBX_Device_t sttbx_GlobalCurrentOutputDevice = STTBX_DEVICE_NULL;
STTBX_Device_t sttbx_GlobalCurrentInputDevice = STTBX_DEVICE_NULL;

void *sttbx_GlobalCPUPartition_p;

#ifdef ST_OS21
osclock_t sttbx_ClocksPerSecond; /* for task_delay() */
#endif
#ifdef ST_OS20
U32 sttbx_ClocksPerSecond;
#endif

/* Private Macros ----------------------------------------------------------- */


/* Private Function prototypes ---------------------------------------------- */

#ifdef ST_OSWINCE
/* EnterCriticalSection & LeaveCriticalSection are WinCE system calls */
#define EnterCriticalSection myEnterCriticalSection
#define LeaveCriticalSection myLeaveCriticalSection
#endif

static void EnterCriticalSection(void);
static void LeaveCriticalSection(void);
static S32 GetIndexOfDeviceNamed(const ST_DeviceName_t WantedName);
static ST_ErrorCode_t Init(sttbx_CallingDevice_t *Device_p,  const STTBX_InitParams_t *InitParams_p);
static ST_ErrorCode_t Term(sttbx_CallingDevice_t *Device_p,  const STTBX_TermParams_t *TermParams_p);


extern ST_ErrorCode_t sttbx_InitOutputBuffers(sttbx_CallingDevice_t *Device_p);
extern ST_ErrorCode_t sttbx_RunOutputBuffers(sttbx_CallingDevice_t *Device_p);
extern ST_ErrorCode_t sttbx_TermOutputBuffers(sttbx_CallingDevice_t *Device_p);
extern ST_ErrorCode_t sttbx_DeleteOutputBuffers(sttbx_CallingDevice_t *Device_p);

extern void sttbx_InitFiltersTables(void);

/* Functions ---------------------------------------------------------------- */


/*******************************************************************************
Name        : EnterCriticalSection
Description : Used to protect critical sections of Init/Open/Close/Term
Parameters  : None
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void EnterCriticalSection(void)
{
    static BOOL FctAccessCtrlInitialized = FALSE;

    task_lock();
    if (!FctAccessCtrlInitialized)
    {
        /* Initialise the Init/Open/Close/Term protection semaphore
          Caution: this semaphore is never deleted. (Not an issue) */
        FctAccessCtrl_p = semaphore_create_fifo(1);
        FctAccessCtrlInitialized = TRUE;
    }
    task_unlock();

    /* Wait for the Init/Open/Close/Term protection semaphore */
    semaphore_wait(FctAccessCtrl_p);
}

/*******************************************************************************
Name        : LeaveCriticalSection
Description : Used to release protection of critical sections of Init/Open/Close/Term
Parameters  : None
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void LeaveCriticalSection(void)
{
    /* Release the Init/Open/Close/Term protection semaphore */
    semaphore_signal(FctAccessCtrl_p);
}

/*******************************************************************************
Name        : Init
Description : API specific initialisations
Parameters  : pointer on device and initialisation parameters
Assumptions : pointer on device is not NULL
Limitations :
Returns     : ST_ErrorCode_t for the API Init function
*******************************************************************************/
static ST_ErrorCode_t Init(sttbx_CallingDevice_t *Device_p,  const STTBX_InitParams_t *InitParams_p)
{
    ST_ErrorCode_t Err = ST_NO_ERROR;
#ifndef STTBX_NO_UART
    STUART_OpenParams_t UartOpenParams;
#endif /* #ifndef STTBX_NO_UART  */
    U8 i;

    /* Initialization of device parameters */
    Device_p->SupportedDevices = InitParams_p->SupportedDevices;
    Device_p->CPUPartition_p = InitParams_p->CPUPartition_p;
    Err = sttbx_InitOutputBuffers(Device_p);
    if (Err != ST_NO_ERROR)
    {
#ifdef STTBX_PRINT
        if (InitParams_p->DefaultOutputDevice == STTBX_DEVICE_DCU )
        {
            debugmessage("STTBX failed to allocate memory for output data !\n");
        }
#endif
        return(Err);
    }
    sttbx_PrintTable_p = (sttbx_PrintTable_t **)memory_allocate(Device_p->CPUPartition_p, sizeof(sttbx_PrintTable_t*)*STTBX_PRINT_TABLE_INIT_SIZE);
    if (sttbx_PrintTable_p == NULL)
    {
        Err = ST_ERROR_NO_MEMORY;
    }
    else
    {
        sttbx_PrintTableSize = STTBX_PRINT_TABLE_INIT_SIZE;
        for (i=0; i < STTBX_PRINT_TABLE_INIT_SIZE; i++)
        {
            sttbx_PrintTable_p[i] = (sttbx_PrintTable_t *)memory_allocate(Device_p->CPUPartition_p, sizeof(sttbx_PrintTable_t));
            if (sttbx_PrintTable_p[i] == NULL)
            {
                Err = ST_ERROR_NO_MEMORY;
                break;
            }
            else
            {
                sttbx_PrintTable_p[i]->String = (char *)memory_allocate(Device_p->CPUPartition_p, STTBX_MAX_LINE_SIZE);
                if (sttbx_PrintTable_p[i]->String == NULL)
                {
                    Err = ST_ERROR_NO_MEMORY;
                    break;
                }
            }
            sttbx_PrintTable_p[i]->IsSlotFree=TRUE;
            sttbx_PrintTable_p[i]->String[0]='\0';
        }
    }
    if (Err == ST_ERROR_NO_MEMORY)
    {
#ifdef STTBX_PRINT
        if (InitParams_p->DefaultOutputDevice == STTBX_DEVICE_DCU )
        {
            debugmessage("STTBX failed to allocate memory for print !\n");
        }
#endif
        return(Err);
    }
    /* Initialise PrintTable locking semaphore */
    sttbx_LockPrintTable_p = semaphore_create_fifo(1);

    /* Initialise Flushing buffer locking semaphore */
    sttbx_WakeUpFlushingBuffer_p = semaphore_create_fifo(0);

    /* Initialise report locking semaphore */
    sttbx_LockReport_p = semaphore_create_fifo(1);

    /* Initialise Input/Output devices locking semaphores */
    sttbx_LockDCUIn_p = semaphore_create_fifo(1);
    sttbx_LockDCUOut_p = semaphore_create_fifo(1);

    /* Initialise sttbx_GlobalPrintID semaphore */
    sttbx_GlobalPrintIdMutex_p = semaphore_create_fifo(1);

#ifndef STTBX_NO_UART
    sttbx_LockUartIn_p = semaphore_create_fifo(1);
    sttbx_LockUartOut_p = semaphore_create_fifo(1);
    sttbx_LockUart2In_p = semaphore_create_fifo(1);
    sttbx_LockUart2Out_p = semaphore_create_fifo(1);
#endif /* #ifndef STTBX_NO_UART  */
#if 0 /* Not supported yet */
    sttbx_LockI1284_p = semaphore_create_fifo(1);
#endif /* Not supported yet */

#ifndef STTBX_NO_UART
    /* Open UART if support is required */
    UartOpenParams.LoopBackEnabled = FALSE; /* NO internal loopback */
    UartOpenParams.FlushIOBuffers = TRUE;   /* Flush before open */
    UartOpenParams.DefaultParams = NULL;    /* Don't overwrite default parameters set at init */
    if (((Device_p->SupportedDevices) & STTBX_DEVICE_UART) != 0)
    {
        Err = STUART_Open(InitParams_p->UartDeviceName, &UartOpenParams, &(Device_p->UartHandle));
        if (Err != ST_NO_ERROR)
        {
            /* error handling: UART failed to open in toolbox ! */
#ifdef STTBX_PRINT
            if (InitParams_p->DefaultOutputDevice == STTBX_DEVICE_DCU )
            {
                debugmessage("UART failed to open in toolbox initialisation !\n");
            }
#endif /* #ifdef STTBX_PRINT */
            /* Device name given was not good ? */
            return(ST_ERROR_BAD_PARAMETER);
        }
#ifdef STTBX_PRINT
        else
        {
            if (InitParams_p->DefaultOutputDevice == STTBX_DEVICE_DCU )
            {
                debugmessage("UART opened in toolbox initialisation.\n");
            }
        }
#endif /* #ifdef STTBX_PRINT */
    }

    /* Open UART2 if support is required */
    UartOpenParams.LoopBackEnabled = FALSE; /* NO internal loopback */
    UartOpenParams.FlushIOBuffers = TRUE;   /* Flush before open */
    UartOpenParams.DefaultParams = NULL;    /* Don't overwrite default parameters set at init */
    if (((Device_p->SupportedDevices) & STTBX_DEVICE_UART2) != 0)
    {
        Err = STUART_Open(InitParams_p->Uart2DeviceName, &UartOpenParams, &(Device_p->Uart2Handle));
        if (Err != ST_NO_ERROR)
        {
            /* error handling: UART failed to open in toolbox ! */
#ifdef STTBX_PRINT
            if (InitParams_p->DefaultOutputDevice == STTBX_DEVICE_DCU )
            {
                debugmessage("UART2 failed to open in toolbox initialisation !\n");
            }
#endif /* #ifdef STTBX_PRINT */
            /* Device name given was not good ? */
            return(ST_ERROR_BAD_PARAMETER);
        }
#ifdef STTBX_PRINT
        else
        {
            if (InitParams_p->DefaultOutputDevice == STTBX_DEVICE_DCU )
            {
                debugmessage("UART2 opened in toolbox initialisation.\n");
            }
        }
#endif /* #ifdef STTBX_PRINT */
    }
#endif /* #ifndef STTBX_NO_UART  */

    /* Set current input and output devices */
    Device_p->CurrentOutputDevice = InitParams_p->DefaultOutputDevice;
    Device_p->CurrentInputDevice = InitParams_p->DefaultInputDevice;
    sttbx_InitFiltersTables();

    /* As STTBX functions don't have an open handle as parameter, it is impossible to
    retrieve any data from the sttbx_CallingDevice_t structure except in init and term functions.
    So we need to use globals for variables we need the value at any time...
    Remark: So we should not do multiple init. Anyway, we don't support it ! */
#ifndef STTBX_NO_UART
    sttbx_GlobalUartHandle = Device_p->UartHandle;
    sttbx_GlobalUart2Handle = Device_p->Uart2Handle;
#endif /* #ifndef STTBX_NO_UART  */
#if 0 /* Not supported yet */
    GlobalI1284Handle = Device_p->I1284Handle;
#endif /* Not supported yet */
    sttbx_GlobalCurrentOutputDevice = Device_p->CurrentOutputDevice;
    sttbx_GlobalCurrentInputDevice = Device_p->CurrentInputDevice;
    sttbx_GlobalCPUPartition_p = Device_p->CPUPartition_p;
    sttbx_ClocksPerSecond = ST_GetClocksPerSecond();

    /* Start new task */
    Err = sttbx_RunOutputBuffers(Device_p);

    return(Err);
}


/*******************************************************************************
Name        : Term
Description : API specific terminations
Parameters  : pointer on device and termination parameters
Assumptions : pointer on device is not NULL
Limitations :
Returns     : ST_ErrorCode_t for the API Term function
*******************************************************************************/
static ST_ErrorCode_t Term(sttbx_CallingDevice_t *Device_p,  const STTBX_TermParams_t *TermParams_p)
{
    ST_ErrorCode_t Err = ST_NO_ERROR;
    U8 i;
    UNUSED_PARAMETER(TermParams_p);
    /* Terminates the task */
    Err = sttbx_TermOutputBuffers(Device_p);

    /* TermParams exploitation and other actions */

#ifndef STTBX_NO_UART
    /* Close UART if it was opened */
    if (((Device_p->SupportedDevices) & STTBX_DEVICE_UART) != 0)
    {
        Err = STUART_Close(Device_p->UartHandle);
        if(Err != ST_NO_ERROR)
        {
            /* error handling: UART failed to open in toolbox ! */
#ifdef STTBX_PRINT
            debugmessage("UART failed to close in toolbox termination !\n");
#endif /* #ifdef STTBX_PRINT */
        }
#ifdef STTBX_PRINT
        else
        {
            debugmessage("UART closed in toolbox termination.\n");
        }
#endif /* #ifdef STTBX_PRINT */
    }

    /* Close UART2 if it was opened */
    if (((Device_p->SupportedDevices) & STTBX_DEVICE_UART2) != 0)
    {
        Err = STUART_Close(Device_p->Uart2Handle);
        if(Err != ST_NO_ERROR)
        {
            /* error handling: UART2 failed to open in toolbox ! */
#ifdef STTBX_PRINT
            debugmessage("UART2 failed to close in toolbox termination !\n");
#endif /* #ifdef STTBX_PRINT */
        }
#ifdef STTBX_PRINT
        else
        {
            debugmessage("UART2 closed in toolbox termination.\n");
        }
#endif /* #ifdef STTBX_PRINT */
    }
#endif /* #ifndef STTBX_NO_UART  */

    /* Delete PrintTable locking semaphore */
    semaphore_delete(sttbx_LockPrintTable_p);

    /* Initialise Flushing buffer locking semaphore */
    semaphore_delete(sttbx_WakeUpFlushingBuffer_p);

    /* Delete report locking semaphore */
    semaphore_delete(sttbx_LockReport_p);

    /* Delete Input/Output devices locking semaphores */
    semaphore_delete(sttbx_LockDCUIn_p);
    semaphore_delete(sttbx_LockDCUOut_p);

  /* Delete GlobalPrintID semaphores */
    semaphore_delete(sttbx_GlobalPrintIdMutex_p);

#ifndef STTBX_NO_UART
    semaphore_delete(sttbx_LockUartIn_p);
    semaphore_delete(sttbx_LockUartOut_p);
    semaphore_delete(sttbx_LockUart2In_p);
    semaphore_delete(sttbx_LockUart2Out_p);
#endif /* #ifndef STTBX_NO_UART  */
#if 0 /* Not supported yet */
    semaphore_delete(sttbx_LockI1284_p);
#endif /* Not supported yet */

    for (i=0; i < STTBX_PRINT_TABLE_INIT_SIZE; i++)
    {
        memory_deallocate(Device_p->CPUPartition_p, sttbx_PrintTable_p[i]->String);
        memory_deallocate(Device_p->CPUPartition_p, sttbx_PrintTable_p[i]);
    }
    memory_deallocate(Device_p->CPUPartition_p, sttbx_PrintTable_p);
    sttbx_PrintTable_p = NULL;
    sttbx_GlobalCurrentOutputDevice = STTBX_DEVICE_NULL;
    sttbx_GlobalCurrentInputDevice  = STTBX_DEVICE_NULL;

    return(ST_NO_ERROR);
} /* end of Term() */


/*******************************************************************************
Name        : GetIndexOfDeviceNamed
Description : Get the index in DeviceArray where a device has been
              initialised with the wanted name, if it exists
Parameters  : the name to look for
Assumptions :
Limitations :
Returns     : the index if the name was found, INVALID_DEVICE_INDEX otherwise
*******************************************************************************/
static S32 GetIndexOfDeviceNamed(const ST_DeviceName_t WantedName)
{
    S32 WantedIndex = INVALID_DEVICE_INDEX, Index = 0;

    do
    {
        /* Device initialised: check if name is matching */
        if (strcmp(DeviceArray[Index].DeviceName, WantedName) == 0)
        {
            /* Name found in the initialised devices */
            WantedIndex = Index;
        }
        Index++;
    } while ((WantedIndex == INVALID_DEVICE_INDEX) && (Index < STTBX_MAX_DEVICE));

    return(WantedIndex);
}


/***************************************************************
Name : STTBX_GetRevision
Description : Retrieve the driver revision
Parameters : none
Return : driver revision
****************************************************************/
ST_Revision_t STTBX_GetRevision(void)
{
    static const char Revision[]="STTBX-REL_2.3.12";

    /* Revision string format: STXXX-REL_x.x.x
                                 /       \ \ \__patch release number  }
                            API name      \ \__minor release number  } revision number
                                           \__major release number  }
    */

    return((ST_Revision_t)Revision);
} /* End of STTBX_GetRevision() */

/*
--------------------------------------------------------------------------------
Get capabilities of toolbox API
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STTBX_GetCapability(const ST_DeviceName_t DeviceName, STTBX_Capability_t  *Capability_p)
{
    /* Exit now if parameters are bad */
    if ((Capability_p == NULL) ||                       /* Pointer to capability structure should be valid ! */
        (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1)   /* Device name length should be respected */
       )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
}

/*
--------------------------------------------------------------------------------
Initialise toolbox API
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STTBX_Init(const ST_DeviceName_t DeviceName, const STTBX_InitParams_t *InitParams_p)
{
    S32 Index = 0;
    ST_ErrorCode_t Err = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if ((InitParams_p == NULL) || /* Bar pointer */
        (InitParams_p->SupportedDevices == 0) || /* No supported device specified */
        (((InitParams_p->SupportedDevices) & (InitParams_p->DefaultOutputDevice)) == 0) || /* Default output device is NOT requested to be supported */
        (((InitParams_p->SupportedDevices) & (InitParams_p->DefaultInputDevice)) == 0) ||  /* Default input device is NOT requested to be supported */
        (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1)       /* Device name length should be respected */
       )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Exit if I1284 support required: it is not supported yet */
    if ((InitParams_p->SupportedDevices & STTBX_DEVICE_I1284) != 0)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    EnterCriticalSection();

    /* First time: initialise devices as empty */
    if (!FirstInitDone)
    {
        for (Index= 0; Index < STTBX_MAX_DEVICE; Index++)
        {
            DeviceArray[Index].DeviceName[0] = '\0';
        }
        /* Process this only once */
        FirstInitDone = TRUE;
    }

    /* Check if device already initialised and return error if so */
    if (GetIndexOfDeviceNamed(DeviceName) != INVALID_DEVICE_INDEX)
    {
        /* Device name found */
        Err = ST_ERROR_ALREADY_INITIALIZED;
    }
    else
    {
        /* Look for a non-initialised device and return error if none is available*/
        Index = 0;
        while ((DeviceArray[Index].DeviceName[0] != '\0') && (Index < STTBX_MAX_DEVICE))
        {
            Index++;
        }
        if (Index >= STTBX_MAX_DEVICE)
        {
            /* All devices initialised */
            Err = ST_ERROR_NO_MEMORY;
        }
        else
        {
            /* API specific initialisations */
            Err = Init(&DeviceArray[Index], InitParams_p);

            if (Err == ST_NO_ERROR)
            {
                /* Device available and successfully initialised: register device name */
                strcpy(DeviceArray[Index].DeviceName, DeviceName);
                DeviceArray[Index].DeviceName[ST_MAX_DEVICE_NAME - 1] = '\0';

            } /* Otherwise the init has failed */
        }
    }

    LeaveCriticalSection();

    return(Err);
}


/*
--------------------------------------------------------------------------------
Terminate toolbox API
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STTBX_Term(const ST_DeviceName_t DeviceName, const STTBX_TermParams_t *TermParams_p)
{
    S32 DeviceIndex = INVALID_DEVICE_INDEX;
    ST_ErrorCode_t Err = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if ((TermParams_p == NULL) ||                       /* There should be term params */
        (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1)   /* Device name length should be respected */
       )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    EnterCriticalSection();

    if (!FirstInitDone)
    {
        Err = ST_ERROR_UNKNOWN_DEVICE;
    }
    else
    {
        /* Check if device already initialised and return error if NOT so */
        DeviceIndex = GetIndexOfDeviceNamed(DeviceName);
        if (DeviceIndex == INVALID_DEVICE_INDEX)
        {
            /* Device name not found */
            Err = ST_ERROR_UNKNOWN_DEVICE;
        }
        else
        {
            /* Device found: desallocate memory, free device */
            DeviceArray[DeviceIndex].DeviceName[0] = '\0';

            /* API specific terminations */
            Err = Term(&DeviceArray[DeviceIndex], TermParams_p);
        }
    }

    LeaveCriticalSection();

    sttbx_DeleteOutputBuffers(&DeviceArray[DeviceIndex]);

    return(Err);
}


#endif  /* #ifndef ST_OSLINUX */
/* End of tbx_init.c */


