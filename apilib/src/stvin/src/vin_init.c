/*******************************************************************************

File name   : vin_init.c

Description : VIN module standard API functions source file

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
27 Sep 2000        Created                                           JA
*******************************************************************************/

/* Includes ----------------------------------------------------------------- */
#if !defined(ST_OSLINUX)
/* Include system files only if not in Kernel mode */
#include <stdlib.h>
#include <string.h>
#endif /*ST_OSLINUX*/

#include "sttbx.h"
#include "stddefs.h"
#include "stdevice.h"
#include "stvin.h"
#include "vin_inte.h"
#include "vin_init.h"
#include "vin_drv.h"
#include "halv_vin.h"
#include "stevt.h"
#include "vin_cmd.h"
#include "vin_rev.h"

/*#define TRACE_UART*/
#ifdef TRACE_UART
# include "../../tests/src/trace.h"
# define VIN_TraceBuffer(x) TraceBuffer(x)
extern void VIN_TracePictureBufferQueue (stvin_Device_t * const Device_p);
extern void VIN_TracePictureStructure (STVID_PictureStructure_t PictureStructure, U32 ExtendedTemporalReference);
#else
# define VIN_TraceBuffer(x)
# define VIN_TracePictureBufferQueue(x)
# define VIN_TracePictureStructure(x,y)
#endif

#ifdef ST_GX1
# define WA_TASK_WAIT_BUGGED_ON_OS20EMU 1
#endif

/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */
#ifndef INTERRUPTLEVEL
#define INTERRUPTLEVEL  4
#endif
/* Max VSync duration */
#define MAX_VSYNC_DURATION (313 * ST_GetClocksPerSecond() / 1000)

/* Max time waiting for an order: 0.33 field display time (130) ? */
#define MAX_WAIT_ORDER_TIME (MAX_VSYNC_DURATION / 3)

#define INVALID_DEVICE_INDEX (-1)

/* Private Variables (static)------------------------------------------------ */
static stvin_Device_t DeviceArray[STVIN_MAX_DEVICE];
/* Init/Open/Close/Term protection semaphore */
static semaphore_t * InterInstancesLockingSemaphore_p = NULL;
static BOOL FirstInitDone = FALSE;

/* Global Variables --------------------------------------------------------- */

/* not static because used in IsValidHandle macro */
stvin_Unit_t stvin_UnitArray[STVIN_MAX_UNIT];

/* Private Function prototypes ---------------------------------------------- */

static ST_ErrorCode_t StartInputTask(stvin_Device_t * const Device_p);
static ST_ErrorCode_t StopInputTask(stvin_Device_t * const Device_p);
static void CheckAndProcessInterrupt(stvin_Device_t * const Device_p);
static void InputTaskFunc(stvin_Device_t * const Device_p);
static void EnterCriticalSection(void);
static void LeaveCriticalSection(void);
static S32 GetIndexOfDeviceNamed(const ST_DeviceName_t WantedName);
static ST_ErrorCode_t Init(stvin_Device_t * const Device_p, const STVIN_InitParams_t * const InitParams_p, const ST_DeviceName_t DeviceName);
static ST_ErrorCode_t Open(stvin_Unit_t * const Unit_p, const STVIN_OpenParams_t * const OpenParams_p);
static ST_ErrorCode_t Close(stvin_Unit_t * const Unit_p);
static ST_ErrorCode_t Term(stvin_Device_t * const Device_p, const STVIN_TermParams_t * const TermParams_p);
static ST_ErrorCode_t ReleasePictureBuffers (stvin_Device_t * const Device_p);

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
    /* Enter this section only if before first Init (InterInstancesLockingSemaphore_p default set at NULL) */
    if (InterInstancesLockingSemaphore_p == NULL)
    {
        semaphore_t * TemporaryInterInstancesLockingSemaphore_p;
        BOOL          TemporarySemaphoreNeedsToBeDeleted;

        TemporaryInterInstancesLockingSemaphore_p = STOS_SemaphoreCreatePriority(NULL, 1);

        /* This case is to protect general access of video functions, like init/open/close and term */
        /* We do not want to call semaphore_create within the task_lock/task_unlock boundaries because
         * semaphore_create internally calls semaphore_wait and can cause normal scheduling to resume,
         * therefore unlocking the critical region */
        STOS_TaskLock();
        if (InterInstancesLockingSemaphore_p == NULL)
        {
            InterInstancesLockingSemaphore_p = TemporaryInterInstancesLockingSemaphore_p;
            TemporarySemaphoreNeedsToBeDeleted = FALSE;
        }
        else
        {
            /* Remember to delete the temporary semaphore, if the InterInstancesLockingSemaphore_p
             * was already created by another video instance */
            TemporarySemaphoreNeedsToBeDeleted = TRUE;
        }
        STOS_TaskUnlock();

        /* Delete the temporary semaphore */
        if(TemporarySemaphoreNeedsToBeDeleted)
        {
            STOS_SemaphoreDelete(NULL, TemporaryInterInstancesLockingSemaphore_p);
        }
    }
    /* Wait for the Init/Open/Close/Term protection semaphore */
    STOS_SemaphoreWait(InterInstancesLockingSemaphore_p);
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
    STOS_SemaphoreSignal(InterInstancesLockingSemaphore_p);
}


/*******************************************************************************
Name        : Init
Description : API specific initialisations
Parameters  : pointer on device and initialisation parameters
Assumptions : pointer on device is not NULL
Limitations :
Returns     : ST_ErrorCode_t for the API Init function
*******************************************************************************/
static ST_ErrorCode_t Init(stvin_Device_t * const Device_p,
        const STVIN_InitParams_t * const InitParams_p,
        const ST_DeviceName_t DeviceName)
{
#ifdef ST_stclkrv
    STCLKRV_OpenParams_t STCkrvOpenParams;
#endif
    STVID_OpenParams_t STVID_OpenParams;
    STEVT_OpenParams_t STEVT_OpenParams;
    HALVIN_InitParams_t HALInitParams;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STEVT_DeviceSubscribeParams_t STEVT_SubscribeParams;
    U8 NumBuffer;
#if (defined(ST_7710) || defined(ST_5528) || defined(ST_7100) || defined(ST_7109))
    char irqName[] = STVIN_IRQ_NAME;
#endif /*(ST_7710) || (ST_5528) || (ST_7100) || (ST_7109*/

#if !defined(ST_OSLINUX)
    /* Reset Device characteristics as it free.                     */
    memset ((U8 *)Device_p, 0, sizeof(stvin_Device_t));
#endif

    /* Test initialisation parameters and exit if some are invalid. */
#if !defined(ST_OSLINUX)
    if ((InitParams_p->CPUPartition_p == NULL) || (InitParams_p->AVMEMPartition == (STAVMEM_PartitionHandle_t) NULL))
#else /*!ST_OSLINUX*/
    if (InitParams_p->CPUPartition_p == NULL)
#endif /*!ST_OSLINUX*/
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Open Video handle.                                           */
    ErrorCode = STVID_Open(InitParams_p->VidHandlerName, &STVID_OpenParams, &(Device_p->VideoHandle));
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: open failed, undo initialisations done.           */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module VIN init: could not open Video !"));
        /* First step of the function : exit with error reporting.  */
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Init local parameters.                                       */
    strcpy(Device_p->VideoDeviceName,InitParams_p->VidHandlerName);

    /* Open EVT handle.                                             */
    ErrorCode = STEVT_Open(InitParams_p->EvtHandlerName, &STEVT_OpenParams, &(Device_p->EventsHandle));
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: open failed, undo initialisations done            */
        ErrorCode = STVID_Close(Device_p->VideoHandle);
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module VIN init: could not open EVT !"));
        /* First step of the function : exit with error reporting.  */
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Initialize here because if any error occur, a Term could     */
    /* lead to problem when calling semaphore_delete(..)            */
    Device_p->ForTask.InputOrder_p = STOS_SemaphoreCreateFifoTimeOut(NULL, 0);

    /* Register events for Ancillary Data */
    ErrorCode = STEVT_RegisterDeviceEvent(Device_p->EventsHandle, DeviceName,
                                            STVIN_USER_DATA_EVT,
                                            &(Device_p->RegisteredEventsID[STVIN_USER_DATA_EVT_ID]));

    STEVT_SubscribeParams.SubscriberData_p   = (void *) (Device_p);
#if !(defined(ST_7710) || defined(ST_5528) || defined(ST_7100) || defined(ST_7109))
    /* Subscribe to events */
    STEVT_SubscribeParams.NotifyCallback     =  stvin_InterruptHandler;

    /* Subscribe to InterruptHandler */
    ErrorCode |= STEVT_SubscribeDeviceEvent(Device_p->EventsHandle,
            ((STVIN_InitParams_t * const) InitParams_p)->InterruptEventName,
            InitParams_p->InterruptEvent, &STEVT_SubscribeParams);

#else
    STOS_InterruptLock();

    if(STOS_InterruptInstall( InitParams_p->InterruptEvent,
                                        INTERRUPTLEVEL,
                                        STOS_INTERRUPT_CAST( stvin_InterruptHandler ),
                                        irqName,
                                        (void *) Device_p) != STOS_SUCCESS)
    {
        STOS_InterruptUnlock();
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module VIN init: Problem installing Interrupt!"));
        ErrorCode = STVID_Close(Device_p->VideoHandle);
        ErrorCode = STEVT_Close(Device_p->EventsHandle);
        return(ST_ERROR_BAD_PARAMETER);
    }

    STOS_InterruptUnlock();

    if(STOS_InterruptEnable(InitParams_p->InterruptEvent,INTERRUPTLEVEL) != STOS_SUCCESS)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module VIN init: Problem enabling Interrupt!"));
        ErrorCode = STVID_Close(Device_p->VideoHandle);
        ErrorCode = STEVT_Close(Device_p->EventsHandle);
        return(ST_ERROR_BAD_PARAMETER);
    }
#endif /* (defined(ST_7710) || defined(ST_5528) || defined(ST_7100) defined(ST_7109)) */

    Device_p->InputWinAutoMode = TRUE;  /* Input Window Mode Automatically set from the video */
    Device_p->OutputWinAutoMode = TRUE;  /* Output Window Mode Automatically set from the video */
    strcpy(Device_p->IntmrDeviceName,InitParams_p->InterruptEventName);
    Device_p->InterruptEvent = InitParams_p->InterruptEvent;
    /* Subscribe to new windows event */
    STEVT_SubscribeParams.NotifyCallback     = (STEVT_DeviceCallbackProc_t) stvin_IOWindowsChange;
    ErrorCode |= STEVT_SubscribeDeviceEvent(Device_p->EventsHandle,
            InitParams_p->VidHandlerName,
            STVID_DIGINPUT_WIN_CHANGE_EVT,
            &STEVT_SubscribeParams);

    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: Event handling failed                             */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
            "Module VIN init: could not subscribe to video windows event !"));

        /* Error: Event handling failed, undo initialisations done. */
        ErrorCode = STVID_Close(Device_p->VideoHandle);
        ErrorCode = STEVT_Close(Device_p->EventsHandle);
        /* STEVT_Close() will unregister any already registerd evt. */
        return(ST_ERROR_BAD_PARAMETER);
    }

#ifdef ST_stclkrv
    /* Open clock recovery driver.                                  */
    ErrorCode = STCLKRV_Open(InitParams_p->ClockRecoveryName, &STCkrvOpenParams, &(Device_p->STClkrvHandle));
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: open failed, undo initialisations done.           */
        ErrorCode = STVID_Close(Device_p->VideoHandle);
        ErrorCode = STEVT_Close(Device_p->EventsHandle);
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module VIN init: could not open CLKRV !"));
        /* First step of the function : exit with error reporting.  */
        return(ST_ERROR_BAD_PARAMETER);
    }
#endif


    /* InitParams exploitation and other actions.                   */
    strcpy(Device_p->DeviceName, DeviceName);
    Device_p->CPUPartition_p    = InitParams_p->CPUPartition_p;
    Device_p->DeviceType        = InitParams_p->DeviceType;
    Device_p->InputMode         = InitParams_p->InputMode;

    /* Status.                                                      */
    Device_p->Status = VIN_DEVICE_STATUS_STOPPED;

    /* Get the input frame rate according of input mode.            */
    switch (InitParams_p->InputMode)
    {
            /* Standard Definition */
        case     STVIN_SD_PAL_720_576_I_CCIR:
        case     STVIN_SD_PAL_768_576_I_CCIR:
            Device_p->InputFrameRate = STVIN_FRAME_RATE_50I;
            break;
            /* Standard Definition */
        case     STVIN_SD_NTSC_720_480_I_CCIR:
        case     STVIN_SD_NTSC_640_480_I_CCIR:
            /* High definition */
        case     STVIN_HD_YCbCr_1920_1080_I_CCIR:
            Device_p->InputFrameRate = STVIN_FRAME_RATE_60I;
            break;

        case     STVIN_HD_YCbCr_1280_720_P_CCIR:
        case     STVIN_HD_YCbCr_720_480_P_CCIR :
        case     STVIN_HD_RGB_1024_768_P_EXT:
        case     STVIN_HD_RGB_800_600_P_EXT:
        case     STVIN_HD_RGB_640_480_P_EXT:
            Device_p->InputFrameRate = STVIN_FRAME_RATE_60P;
            break;

        default :
            Device_p->InputFrameRate = STVIN_FRAME_RATE_60I;
            break;
    }

    Device_p->ForTask.TimeCode.Hours    = 0;
    Device_p->ForTask.TimeCode.Minutes  = 0;
    Device_p->ForTask.TimeCode.Seconds  = 0;
    Device_p->ForTask.TimeCode.Frames   = 0;
    Device_p->ForTask.TimeCode.Interpolated     = TRUE;
    Device_p->ForTask.PictureBufferCurrentIndex = 0;
    Device_p->ForTask.ExtendedTemporalReference = 0;
    Device_p->ForTask.AncillaryDataCurrentIndex = 0;


    /* Frame Buffer Status */
    for (NumBuffer = 0; NumBuffer < STVIN_MAX_NB_FRAME_IN_VIDEO; NumBuffer++)
    {
        Device_p->PictureBufferHandle[NumBuffer] = (STVID_PictureBufferHandle_t) NULL;
    }

    Device_p->DummyProgOrTopFieldBufferHandle = (STVID_PictureBufferHandle_t) NULL;
    Device_p->DummyBotFieldBufferHandle       = (STVID_PictureBufferHandle_t) NULL;

    /* Call the HAL */
    HALInitParams.CPUPartition_p= InitParams_p->CPUPartition_p;
    HALInitParams.DeviceType    = InitParams_p->DeviceType;
    HALInitParams.InputMode     = InitParams_p->InputMode;
    HALInitParams.DeviceBaseAddress_p   = InitParams_p->DeviceBaseAddress_p;
    HALInitParams.RegistersBaseAddress_p= InitParams_p->RegistersBaseAddress_p;
    HALInitParams.VideoParams_p = &(Device_p->VideoParams);
#ifdef ST_OSLINUX
    HALInitParams.AVMEMPartionHandle = STAVMEM_GetPartitionHandle( InitParams_p->AVMEMPartition );
#else /*ST_OSLINUX*/
    HALInitParams.AVMEMPartionHandle = InitParams_p->AVMEMPartition;
#endif /*ST_OSLINUX*/

#ifdef ST_OSLINUX
    if ( ErrorCode == ST_NO_ERROR)
    {
        Device_p->VinMappedWidth = (U32)(STVIN_MAPPING_WIDTH);

        Device_p->VinMappedBaseAddress_p = STOS_MapRegisters( (void *) (InitParams_p->RegistersBaseAddress_p), (U32) (Device_p->VinMappedWidth), "VIN");

        if ( Device_p->VinMappedBaseAddress_p )
        {
            HALInitParams.RegistersBaseAddress_p = Device_p->VinMappedBaseAddress_p;
            HALInitParams.DeviceBaseAddress_p = (void *) 0;
        }
        else
        {
            ErrorCode =  ST_ERROR_BAD_PARAMETER;
        }
    }
#endif  /* ST_OSLINUX */

    ErrorCode = HALVIN_Init(&HALInitParams, &(Device_p->HALInputHandle));

    if (ErrorCode == ST_NO_ERROR)
    {
        if ((((HALVIN_Properties_t *)(Device_p->HALInputHandle))->DefInputMode == STVIN_INPUT_MODE_SD) ||
            (((((HALVIN_Properties_t *)(Device_p->HALInputHandle))->DefInputMode == STVIN_INPUT_MODE_HD))&&
             ((((HALVIN_Properties_t *)(Device_p->HALInputHandle))->DeviceType == STVIN_DEVICE_TYPE_ST7100_DVP_INPUT)  ||
              (((HALVIN_Properties_t *)(Device_p->HALInputHandle))->DeviceType == STVIN_DEVICE_TYPE_ST7109_DVP_INPUT))))
        {
            /* Input source is SD. We may manage ancillary data.    */

            /* After init, event are no allowed. Application should call    */
            /* STVIN_SetAncillaryParameters() function to allow capture.    */
            Device_p->AncillaryDataEventEnabled = FALSE;

            if ( (InitParams_p->UserDataBufferNumber != 0) && (InitParams_p->UserDataBufferSize != 0) )
            {
                ErrorCode = HALVIN_AllocateMemoryForAncillaryData(Device_p->HALInputHandle,
                        InitParams_p->UserDataBufferNumber,
                        InitParams_p->UserDataBufferSize,
                        Device_p->AncillaryDataTable);

                if (ErrorCode == ST_NO_ERROR)
                {
                    /* Ok, all buffers allocated.                               */
                    Device_p->AncillaryDataTableAllocated = TRUE;
                    Device_p->AncillaryDataBufferNumber = InitParams_p->UserDataBufferNumber;
                    Device_p->AncillaryDataBufferSize   = InitParams_p->UserDataBufferSize;
                }
            }
        }
        else
        {
            Device_p->AncillaryDataEventEnabled   = FALSE;
            Device_p->AncillaryDataTableAllocated = FALSE;
        }
    }

    if (ErrorCode == ST_NO_ERROR)
    {
        Device_p->VideoInterrupt.Mask      = 0;
        Device_p->VideoInterrupt.EnableCount = 0;

        /* Initialise commands queue */
        Device_p->CommandsBuffer.Data_p         = Device_p->CommandsData;
        Device_p->CommandsBuffer.TotalSize      = sizeof(Device_p->CommandsData);
        Device_p->CommandsBuffer.BeginPointer_p = Device_p->CommandsBuffer.Data_p;
        Device_p->CommandsBuffer.UsedSize       = 0;
        Device_p->CommandsBuffer.MaxUsedSize    = 0;

        /* Initialise commands queue with VSync */
        Device_p->VSyncCommandsBuffer.Data_p         = Device_p->VSyncCommandsData;
        Device_p->VSyncCommandsBuffer.TotalSize      = sizeof(Device_p->VSyncCommandsData);
        Device_p->VSyncCommandsBuffer.BeginPointer_p = Device_p->VSyncCommandsBuffer.Data_p;
        Device_p->VSyncCommandsBuffer.UsedSize       = 0;
        Device_p->VSyncCommandsBuffer.MaxUsedSize    = 0;

        /* Initialise interrupts queue */
        Device_p->InterruptsBuffer.Data_p        = Device_p->InterruptsData;
        Device_p->InterruptsBuffer.TotalSize     = sizeof(Device_p->InterruptsData) / sizeof(U32);
        Device_p->InterruptsBuffer.BeginPointer_p = Device_p->InterruptsBuffer.Data_p;
        Device_p->InterruptsBuffer.UsedSize      = 0;
        Device_p->InterruptsBuffer.MaxUsedSize   = 0;

        Device_p->InputTask.IsRunning  = FALSE;

        /* Enable video interrupt */
        /* GGi (19/07/2002)         */
        /* No more used. Omega2 doesn't need interrupt handler. */
    /*        stvin_EnableVideoInterrupt(Device_p);*/

        Device_p->VideoInterrupt.Mask = 0;

        HALVIN_SetInterruptMask(Device_p->HALInputHandle, Device_p->VideoInterrupt.Mask);

        ErrorCode = StartInputTask(Device_p);

    }

    if (ErrorCode != ST_NO_ERROR)
    {
        STVIN_TermParams_t TermParams;
        Term(Device_p, &TermParams);
    }

    return(ErrorCode);
} /* End of Init() function. */

/*******************************************************************************
Name        : Open
Description : API specific actions just before opening
Parameters  : pointer on unit and open parameters
Assumptions : pointer on unit is not NULL
Limitations :
Returns     : ST_ErrorCode_t for the API Open function
*******************************************************************************/
static ST_ErrorCode_t Open(stvin_Unit_t * const Unit_p, const STVIN_OpenParams_t * const OpenParams_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    UNUSED_PARAMETER(Unit_p);
    UNUSED_PARAMETER(OpenParams_p);

    return(ErrorCode);
}

/*******************************************************************************
Name        : Close
Description : API specific actions just before closing
Parameters  : pointer on unit
Assumptions : pointer on unit is not NULL
Limitations :
Returns     : ST_ErrorCode_t for the API Close function
              If not ST_NO_ERROR, the Handle would not be closed afterwards
*******************************************************************************/
static ST_ErrorCode_t Close(stvin_Unit_t * const Unit_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    UNUSED_PARAMETER(Unit_p);

    /* Other actions */

    return(ErrorCode);
}

/*******************************************************************************
Name        : Term
Description : API specific terminations
Parameters  : pointer on device and termination parameters
Assumptions : pointer on device is not NULL
Limitations :
Returns     : ST_ErrorCode_t for the API Term function
*******************************************************************************/
static ST_ErrorCode_t Term(stvin_Device_t * const Device_p, const STVIN_TermParams_t * const TermParams_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    U8 Loop;
    UNUSED_PARAMETER(TermParams_p);

    /* TermParams exploitation and other actions */
    Device_p->DeviceName[0] = '\0';

    /* desactivate the interface */
    HALVIN_DisableInterface(Device_p->HALInputHandle);

    Device_p->VideoInterrupt.Mask = 0;
    HALVIN_SetInterruptMask(Device_p->HALInputHandle, Device_p->VideoInterrupt.Mask);

    /* Disable video interrupt */
    /* 1GGi (19/07/2002)         */
    /* No more used. Omega2 doesn't need interrupt handler. */
/*    stvin_DisableVideoInterrupt(Device_p);*/


    /* Reset index of buffer */
    for (Loop = 0; Loop < STVIN_MAX_NB_FRAME_IN_VIDEO; Loop++)
    {
        if (Device_p->PictureBufferHandle[Loop] != (STVID_PictureBufferHandle_t) NULL)
        {
            ErrorCode = STVID_ReleasePictureBuffer(Device_p->VideoHandle,
                                                   Device_p->PictureBufferHandle[Loop]);
            Device_p->PictureBufferHandle[Loop] = (STVID_PictureBufferHandle_t) NULL;
        }
    }

    StopInputTask(Device_p);

    /* Release Ancillary Data memory */
    if (Device_p->AncillaryDataTableAllocated)
    {
        ErrorCode = HALVIN_DeAllocateMemoryForAncillaryData(Device_p->HALInputHandle,
                Device_p->AncillaryDataBufferNumber,
                Device_p->AncillaryDataTable);
        Device_p->AncillaryDataTableAllocated = FALSE;
    }

    ErrorCode = HALVIN_Term(Device_p->HALInputHandle);
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: HALTerm failed  */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module VIN init: could not Term HALVIN"));
        ErrorCode = ST_ERROR_BAD_PARAMETER;
    }

    STOS_SemaphoreDelete(NULL, Device_p->ForTask.InputOrder_p);

#ifdef ST_stclkrv
    /* Close Clock Recovery handle */
    ErrorCode = STCLKRV_Close(Device_p->STClkrvHandle);
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: close failed, undo initialisations done */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module VIN init: could not close CLKRV !"));
        ErrorCode = ST_ERROR_BAD_PARAMETER;
    }
#endif

    /* Close Video handle */
    ErrorCode = STVID_Close(Device_p->VideoHandle);
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: close failed, undo initialisations done */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module VIN init: could not close Video !"));
        ErrorCode = ST_ERROR_BAD_PARAMETER;
    }

    /* Unsubscribe events */
    STEVT_UnsubscribeDeviceEvent(Device_p->EventsHandle,
                                    Device_p->VideoDeviceName,
                                    STVID_DIGINPUT_WIN_CHANGE_EVT);
#if !(defined(ST_7710) || defined(ST_5528) || defined(ST_7100) || defined(ST_7109))
    STEVT_UnsubscribeDeviceEvent(Device_p->EventsHandle,
                                    Device_p->IntmrDeviceName,
                                    Device_p->InterruptEvent );
#else
    STOS_InterruptLock();
    if (STOS_InterruptDisable( Device_p->InterruptEvent, INTERRUPTLEVEL) != STOS_SUCCESS)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Problem disabling interrupt !"));
    }
    if (STOS_InterruptUninstall( Device_p->InterruptEvent, INTERRUPTLEVEL, (void *)Device_p ) != STOS_SUCCESS)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Problem un-installing interrupt !"));
    }

    if (STOS_InterruptEnable( Device_p->InterruptEvent, INTERRUPTLEVEL) != STOS_SUCCESS)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Problem re-enbling interrupt !"));
    }
    STOS_InterruptUnlock();
#endif /* (defined(ST_7710) || defined(ST_5528) || defined(ST_7100) defined(ST_7109)) */


    /* Close EVT handle */
    ErrorCode = STEVT_Close(Device_p->EventsHandle);
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: close failed, undo initialisations done */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module VIN init: could not close EVT !"));
        ErrorCode = ST_ERROR_BAD_PARAMETER;
    }

#if defined(ST_OSLINUX)
    /* releasing VIN linux device driver */
    /** VIN region **/
    STOS_UnmapRegisters(Device_p->VinMappedBaseAddress_p, (U32) (Device_p->VinMappedWidth));

#endif

    return(ErrorCode);
} /* End of Term() */

/*******************************************************************************
Name        : ReleasePictureBuffers
Description : Release all picture buffers got from video (STVID).
Parameters  : pointer on device
Assumptions : pointer on device is not NULL.
Limitations :
Returns     : the index if the name was found, INVALID_DEVICE_INDEX otherwise
*******************************************************************************/
static ST_ErrorCode_t ReleasePictureBuffers (stvin_Device_t * const Device_p)
{
    U8              Loop;
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;

    /* Reset index of buffer */
    for (Loop = 0; Loop < STVIN_MAX_NB_FRAME_IN_VIDEO; Loop++)
    {
        if (Device_p->PictureBufferHandle[Loop] != (STVID_PictureBufferHandle_t) NULL)
        {
            ErrorCode = STVID_ReleasePictureBuffer(Device_p->VideoHandle,
                                                   Device_p->PictureBufferHandle[Loop]);
            Device_p->PictureBufferHandle[Loop] = (STVID_PictureBufferHandle_t) NULL;
        }
    }

    if (Device_p->DummyProgOrTopFieldBufferHandle != (STVID_PictureBufferHandle_t) NULL)
    {
        ErrorCode = STVID_ReleasePictureBuffer(Device_p->VideoHandle,
                                                Device_p->DummyProgOrTopFieldBufferHandle);

        Device_p->DummyProgOrTopFieldBufferHandle = (STVID_PictureBufferHandle_t) NULL;
    }

    if (Device_p->DummyBotFieldBufferHandle != (STVID_PictureBufferHandle_t) NULL)
    {
        ErrorCode = STVID_ReleasePictureBuffer(Device_p->VideoHandle,
                                                Device_p->DummyBotFieldBufferHandle);
        Device_p->DummyBotFieldBufferHandle = (STVID_PictureBufferHandle_t) NULL;
    }

    return(ST_NO_ERROR);

} /* End of ReleasePictureBuffers() function. */

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
    } while ((WantedIndex == INVALID_DEVICE_INDEX) && (Index < STVIN_MAX_DEVICE));

    return(WantedIndex);
}


/*
--------------------------------------------------------------------------------
Get revision of vinxx API
--------------------------------------------------------------------------------
*/
ST_Revision_t STVIN_GetRevision(void)
{
    /* Revision string format: STVIN-REL_x.x.x
                                 /       \ \ \__patch release number  }
                            API name      \ \__minor release number  } revision number
                                           \__major release number  }
       Example: STAVMEM-REL_1.0.0 */

    return((ST_Revision_t) VIN_Revision);
}


/*
--------------------------------------------------------------------------------
Get capabilities of vinxx API
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STVIN_GetCapability(const ST_DeviceName_t DeviceName, STVIN_Capability_t * const Capability_p)
{
    S32 DeviceIndex = INVALID_DEVICE_INDEX;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if ((Capability_p == NULL) ||                       /* Pointer to capability structure should be valid ! */
        (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1)   /* DeAvice name length should be respected */
       )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Check if device already initialised and return error if not so */
    DeviceIndex = GetIndexOfDeviceNamed(DeviceName);
    if (DeviceIndex == INVALID_DEVICE_INDEX)
    {
        /* Device name not found */
        return(ST_ERROR_UNKNOWN_DEVICE);
    }

    /* Fill capability structure */
    switch (DeviceArray[DeviceIndex].DeviceType)
    {
        case STVIN_DEVICE_TYPE_7015_DIGITAL_INPUT :
        case STVIN_DEVICE_TYPE_7020_DIGITAL_INPUT :
        case STVIN_DEVICE_TYPE_ST40GX1_DVP_INPUT  :
        case STVIN_DEVICE_TYPE_ST5528_DVP_INPUT   :
        case STVIN_DEVICE_TYPE_ST7710_DVP_INPUT   :
        case STVIN_DEVICE_TYPE_ST7100_DVP_INPUT   :
        case STVIN_DEVICE_TYPE_ST7109_DVP_INPUT   :
            Capability_p->SupportedAncillaryData =
                            STVIN_ANC_DATA_PACKET_NIBBLE_ENCODED |
                            STVIN_ANC_DATA_PACKET_DIRECT         |
                            STVIN_ANC_DATA_RAW_CAPTURE;
            break;
        default :
            return(ST_ERROR_UNKNOWN_DEVICE);
            break;
    }

    return(ErrorCode);
}


/*
--------------------------------------------------------------------------------
Initialise vinxx API
(Standard instances management. Driver specific implementations should be put in Init() function.)
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STVIN_Init(const ST_DeviceName_t DeviceName, const STVIN_InitParams_t * const InitParams_p)
{
    S32 Index = 0;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if ((InitParams_p == NULL) ||                        /* There must be parameters */
        (InitParams_p->MaxOpen > STVIN_MAX_OPEN) ||      /* No more than MAX_OPEN open handles supported */
        (InitParams_p->MaxOpen == 0) ||                  /* Cannot be NULL ! */
        (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1) || /* Device name length should be respected */
        ((((const char *) DeviceName)[0]) == '\0')
       )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    EnterCriticalSection();

    /* First time: initialise devices and units as empty */
    if (!FirstInitDone)
    {
        for (Index= 0; Index < STVIN_MAX_DEVICE; Index++)
        {
            DeviceArray[Index].DeviceName[0] = '\0';
        }

        for (Index= 0; Index < STVIN_MAX_UNIT; Index++)
        {
            stvin_UnitArray[Index].UnitValidity = 0;
        }
        /* Process this only once */
        FirstInitDone = TRUE;
    }

    /* Check if device already initialised and return error if so */
    if (GetIndexOfDeviceNamed(DeviceName) != INVALID_DEVICE_INDEX)
    {
        /* Device name found */
        ErrorCode = ST_ERROR_ALREADY_INITIALIZED;
    }
    else
    {
        /* Look for a non-initialised device and return error if none is available */
        Index = 0;
        while ((DeviceArray[Index].DeviceName[0] != '\0') && (Index < STVIN_MAX_DEVICE))
        {
            Index++;
        }
        if (Index >= STVIN_MAX_DEVICE)
        {
            /* All devices initialised */
            ErrorCode = ST_ERROR_NO_MEMORY;
        }
        else
        {
            /* API specific initialisations */
            ErrorCode = Init(&DeviceArray[Index], InitParams_p, DeviceName);

            if (ErrorCode == ST_NO_ERROR)
            {
                /* Device available and successfully initialised: register device name */
                strcpy(DeviceArray[Index].DeviceName, DeviceName);
                DeviceArray[Index].DeviceName[ST_MAX_DEVICE_NAME - 1] = '\0';
                DeviceArray[Index].MaxOpen = InitParams_p->MaxOpen;

                STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Device '%s' initialised", DeviceArray[Index].DeviceName));
            }
        }
    }

    LeaveCriticalSection();

    return(ErrorCode);
}


/*
--------------------------------------------------------------------------------
open ...
(Standard instances management. Driver specific implementations should be put in Open() function.)
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STVIN_Open(const ST_DeviceName_t DeviceName, const STVIN_OpenParams_t * const OpenParams_p, STVIN_Handle_t *Handle_p)
{
    S32 DeviceIndex = INVALID_DEVICE_INDEX, UnitIndex = 0, Index;
    U32 OpenedUnitForThisInit;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if ((OpenParams_p == NULL) ||                        /* There must be parameters ! */
        (Handle_p == NULL) ||                            /* Pointer to handle should be valid ! */
        (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1) || /* Device name length should be respected */
        ((((const char *) DeviceName)[0]) == '\0')
        )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    EnterCriticalSection();

    if (!FirstInitDone)
    {
        ErrorCode = ST_ERROR_UNKNOWN_DEVICE;
    }
    else
    {
        /* Check if device already initialised and return error if not so */
        DeviceIndex = GetIndexOfDeviceNamed(DeviceName);
        if (DeviceIndex == INVALID_DEVICE_INDEX)
        {
            /* Device name not found */
            ErrorCode = ST_ERROR_UNKNOWN_DEVICE;
        }
        else
        {
            /* Look for a free unit and return error if none is free */
            UnitIndex = STVIN_MAX_UNIT;
            OpenedUnitForThisInit = 0;
            for (Index = 0; Index < STVIN_MAX_UNIT; Index++)
            {
                if ((stvin_UnitArray[Index].UnitValidity == STVIN_VALID_UNIT) && (stvin_UnitArray[Index].Device_p == &DeviceArray[DeviceIndex]))
                {
                    OpenedUnitForThisInit ++;
                }
                if (stvin_UnitArray[Index].UnitValidity != STVIN_VALID_UNIT)
                {
                    /* Found a free handle structure */
                    UnitIndex = Index;
                }
            }
            if ((OpenedUnitForThisInit >= DeviceArray[DeviceIndex].MaxOpen) || (UnitIndex >= STVIN_MAX_UNIT))
            {
                /* None of the units is free or MaxOpen reached */
                ErrorCode = ST_ERROR_NO_FREE_HANDLES;
            }
            else
            {
                *Handle_p = (STVIN_Handle_t) &stvin_UnitArray[UnitIndex];
                stvin_UnitArray[UnitIndex].Device_p = &DeviceArray[DeviceIndex]; /* 'inherits' device characteristics */

                /* API specific actions after opening */
                ErrorCode = Open(&stvin_UnitArray[UnitIndex], OpenParams_p);

                if (ErrorCode == ST_NO_ERROR)
                {
                    /* Register opened handle */
                    stvin_UnitArray[UnitIndex].UnitValidity = STVIN_VALID_UNIT;

                    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Handle opened on device '%s'", DeviceName));
                }
            }
        }
    }

    LeaveCriticalSection();

    return(ErrorCode);
}


/*
--------------------------------------------------------------------------------
Close ...
(Standard instances management. Driver specific implementations should be put in Close() function.)
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STVIN_Close(STVIN_Handle_t Handle)
{
    stvin_Unit_t *Unit_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    EnterCriticalSection();

    if (!(FirstInitDone))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        if (!IsValidHandle(Handle))
        {
            ErrorCode = ST_ERROR_INVALID_HANDLE;
        }
        else
        {
            Unit_p = (stvin_Unit_t *) Handle;

            /* API specific actions before closing */
            ErrorCode = Close(Unit_p);

            /* Close only if no errors */
            if (ErrorCode == ST_NO_ERROR)
            {
                /* Un-register opened handle */
                Unit_p->UnitValidity = 0;

                STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Handle closed on device '%s'", Unit_p->Device_p->DeviceName));
            }
        }
    }

    LeaveCriticalSection();

    return(ErrorCode);
}


/*
--------------------------------------------------------------------------------
Terminate vinxx API
(Standard instances management. Driver specific implementations should be put in Term() function.)
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STVIN_Term(const ST_DeviceName_t DeviceName, const STVIN_TermParams_t *const TermParams_p)
{
    stvin_Unit_t *Unit_p;
    S32 DeviceIndex = INVALID_DEVICE_INDEX, UnitIndex = 0;
    BOOL Found = FALSE;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if ((TermParams_p == NULL) ||                         /* There must be parameters ! */
        (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1) ||  /* Device name length should be respected */
        ((((const char *) DeviceName)[0]) == '\0')
       )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    EnterCriticalSection();

    if (!FirstInitDone)
    {
        ErrorCode = ST_ERROR_UNKNOWN_DEVICE;
    }
    else
    {
        /* Check if device already initialised and return error if NOT so */
        DeviceIndex = GetIndexOfDeviceNamed(DeviceName);
        if (DeviceIndex == INVALID_DEVICE_INDEX)
        {
            /* Device name not found */
            ErrorCode = ST_ERROR_UNKNOWN_DEVICE;
        }
        else
        {
            /* Check if there is still 'open' on this device */
        /*    UnitIndex = 0;
            Found = FALSE;*/
            Unit_p = stvin_UnitArray;
            while ((UnitIndex < STVIN_MAX_UNIT) && (!Found))
            {
                Found = ((Unit_p->Device_p == &DeviceArray[DeviceIndex]) && (Unit_p->UnitValidity == STVIN_VALID_UNIT));
                Unit_p++;
                UnitIndex++;
            }

            if (Found)
            {
                if (TermParams_p->ForceTerminate)
                {
                    UnitIndex = 0;
                    Unit_p = stvin_UnitArray;
                    while (UnitIndex < STVIN_MAX_UNIT)
                    {
                        if ((Unit_p->Device_p == &DeviceArray[DeviceIndex]) && (Unit_p->UnitValidity == STVIN_VALID_UNIT))
                        {
                            /* Found an open instance: close it ! */
                            ErrorCode = Close(Unit_p);
                            if (ErrorCode == ST_NO_ERROR)
                            {
                                /* Un-register opened handle */
                                Unit_p->UnitValidity = 0;

                                STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Handle closed on device '%s'", Unit_p->Device_p->DeviceName));
                            }
                            else
                            {
                                /* Problem: this should not happen
                                Fail to terminate ? No because of force */
                                ErrorCode = ST_NO_ERROR;
                            }
                        }
                        Unit_p++;
                        UnitIndex++;
                    }
                }
                else
                {
                    ErrorCode = ST_ERROR_OPEN_HANDLE;
                }
            }

            /* Terminate if OK */
            if (ErrorCode == ST_NO_ERROR)
            {
                /* API specific terminations */
                ErrorCode = Term(&DeviceArray[DeviceIndex], TermParams_p);

                if (ErrorCode == ST_NO_ERROR)
                {
                    /* Device found: desallocate memory, free device */
                    DeviceArray[DeviceIndex].DeviceName[0] = '\0';

                    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Device '%s' terminated", DeviceName));
                }
           }
        }
    }

    LeaveCriticalSection();

    return(ErrorCode);
}

/*******************************************************************************
Name        : STVIN_Start
Description : Start the video input
Parameters  : Video handle
              video input start parameters
Assumptions :
Limitations :
Returns     : ST_NO_ERROR               if decode successfully started.
              ST_ERROR_INVALID_HANDLE   if the handle is not valid.
              ST_ERROR_BAD_PARAMETER    if at least one input parameter or
                    the memory profile is not valid.
*******************************************************************************/
ST_ErrorCode_t STVIN_Start(const STVIN_Handle_t Handle,
                           const STVIN_StartParams_t * const Params_p)
{
    stvin_Unit_t *Unit_p;
    stvin_Device_t * Device_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STVID_MemoryProfile_t STVID_MemoryProfile;
    STVID_GetPictureBufferParams_t GetPictureBufferParams;
    U32 InterruptStatus;

#ifdef ST_stclkrv
    STCLKRV_ExtendedSTC_t ExtendedSTC;
#endif
    STVID_PTS_t CurrentSTC;

    VIN_TraceBuffer(("** STVIN_Start\r\n"));

    /* Exit now if parameters are bad */
    if (Params_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!IsValidHandle(Handle))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        Unit_p = (stvin_Unit_t *) Handle;

        /* Handle is valid: point to device */
        Device_p = ((stvin_Unit_t *)Handle)->Device_p;

        /* Check Status */
        if (Device_p->Status == VIN_DEVICE_STATUS_RUNNING)
        {
            STTBX_Print(("Already running !"));
            return(ST_NO_ERROR);
        }

        /* Check Memory Profile */
        /* Get the memory profile of THIS decode/display process. */
        ErrorCode = STVID_GetMemoryProfile(Device_p->VideoHandle, &STVID_MemoryProfile);
        if (ErrorCode!=ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                          "STVIN_Start() : Impossible to get memory profile from STVID !"));
            return(ST_ERROR_BAD_PARAMETER);
        }

        /* Memorize the current video memory profile.             */
        Device_p->CurrentVideoMemoryProfile = STVID_MemoryProfile;

        /* the decimation are the same for the width & the height */
        /* TODO: Decimation not supported! */
        ErrorCode = HALVIN_SetReconstructedStorageMemoryMode(
            Device_p->HALInputHandle,
            STVID_MemoryProfile.DecimationFactor & ~(STVID_DECIMATION_FACTOR_V2|STVID_DECIMATION_FACTOR_V4),
            STVID_MemoryProfile.DecimationFactor & ~(STVID_DECIMATION_FACTOR_H2|STVID_DECIMATION_FACTOR_H4),
            Device_p->VideoParams.ScanType);

        if (ErrorCode!=ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                          "STVIN_Start() : HALVIN_SetReconstructedStorageMemoryMode !"));
            return(ST_ERROR_BAD_PARAMETER);
        }

        switch (((HALVIN_Properties_t *)(Device_p->HALInputHandle))->DefInputMode)
        {
            case STVIN_INPUT_MODE_SD:
                if (STVID_MemoryProfile.MaxWidth < ((HALVIN_Properties_t *) ((HALVIN_Handle_t)Device_p->HALInputHandle))->InputWindow.OutputWinWidth)
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STVIN_Start() : memory profile -> FrameWidth"));
                    return(ST_ERROR_BAD_PARAMETER);
                }

                /* The below height limitaion was removed (old consideration) */
                /*   NTSC = 525 lines/frame but only 480 lines/frame in active area */
                /*   but the buffer *must* be enough to support ALL the lines */

                if (STVID_MemoryProfile.MaxHeight < ((HALVIN_Properties_t *) ((HALVIN_Handle_t)Device_p->HALInputHandle))->InputWindow.OutputWinHeight)
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STVIN_Start() : memory profile -> FrameHeight"));
                    return(ST_ERROR_BAD_PARAMETER);
                }

                break;
            case STVIN_INPUT_MODE_HD:
                if ((((HALVIN_Properties_t *)(Device_p->HALInputHandle))->DeviceType == STVIN_DEVICE_TYPE_ST7100_DVP_INPUT)  ||
                    (((HALVIN_Properties_t *)(Device_p->HALInputHandle))->DeviceType == STVIN_DEVICE_TYPE_ST7109_DVP_INPUT))
                {
                    if (STVID_MemoryProfile.MaxWidth < ((HALVIN_Properties_t *) ((HALVIN_Handle_t)Device_p->HALInputHandle))->InputWindow.OutputWinWidth)
                    {
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STVIN_Start() : memory profile -> FrameWidth"));
                        return(ST_ERROR_BAD_PARAMETER);
                    }

                    /* The below height limitaion was removed (old consideration) */
                    /*   NTSC = 525 lines/frame but only 480 lines/frame in active area */
                    /*   but the buffer *must* be enough to support ALL the lines */

                    if (STVID_MemoryProfile.MaxHeight < ((HALVIN_Properties_t *) ((HALVIN_Handle_t)Device_p->HALInputHandle))->InputWindow.OutputWinHeight)
                    {
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STVIN_Start() : memory profile -> FrameHeight"));
                        return(ST_ERROR_BAD_PARAMETER);
                    }
                }
                else
                {
                    if ((STVID_MemoryProfile.MaxWidth < Device_p->VideoParams.FrameWidth) ||
                        (STVID_MemoryProfile.MaxHeight < Device_p->VideoParams.FrameHeight))
                    {
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STVIN_Start() : memory profile -> FrameHeight / FrameWidth"));
                        return(ST_ERROR_BAD_PARAMETER);
                    }
                }

                break;
            case STVIN_INPUT_INVALID:
            default:
                return(ST_ERROR_BAD_PARAMETER);
                break;
        }


        /* Size of the Frame Width in unit in pixels */
        switch (Device_p->DeviceType)
        {
            case STVIN_DEVICE_TYPE_ST5528_DVP_INPUT   :
            case STVIN_DEVICE_TYPE_ST7710_DVP_INPUT   :
            case STVIN_DEVICE_TYPE_ST7100_DVP_INPUT   :
            case STVIN_DEVICE_TYPE_ST7109_DVP_INPUT   :
                switch (Device_p->InputMode) {
                    case STVIN_HD_YCbCr_1280_720_P_CCIR :
                    case STVIN_HD_YCbCr_720_480_P_CCIR :
                    case STVIN_HD_RGB_1024_768_P_EXT :
                    case STVIN_HD_RGB_800_600_P_EXT :
                    case STVIN_HD_RGB_640_480_P_EXT :
                        HALVIN_SetReconstructedFrameSize(Device_p->HALInputHandle,
                            2 * ((HALVIN_Properties_t *) ((HALVIN_Handle_t)Device_p->HALInputHandle))->InputWindow.OutputWinWidth,
                            Device_p->VideoParams.FrameHeight);
                        break;

                    case STVIN_SD_NTSC_720_480_I_CCIR :
                    case STVIN_SD_NTSC_640_480_I_CCIR :
                    case STVIN_SD_PAL_720_576_I_CCIR :
                    case STVIN_SD_PAL_768_576_I_CCIR :
                    case STVIN_HD_YCbCr_1920_1080_I_CCIR :
                    default :
                        HALVIN_SetReconstructedFrameSize(Device_p->HALInputHandle,
                            4 * ((HALVIN_Properties_t *) ((HALVIN_Handle_t)Device_p->HALInputHandle))->InputWindow.OutputWinWidth,
                            Device_p->VideoParams.FrameHeight);
                       break;
                }
                break;
            case STVIN_DEVICE_TYPE_ST40GX1_DVP_INPUT  :
            case STVIN_DEVICE_TYPE_7015_DIGITAL_INPUT :
            case STVIN_DEVICE_TYPE_7020_DIGITAL_INPUT :
            default :
                HALVIN_SetReconstructedFrameSize(Device_p->HALInputHandle,
                                                Device_p->VideoParams.FrameWidth,
                                                Device_p->VideoParams.FrameHeight);
                break;
        }

        /* Size of the Frame Width in unit in pixels */
#if defined(ST_7109)
        if ((Device_p->VideoParams.ScanType == STGXOBJ_INTERLACED_SCAN) &&
            (((Device_p->VideoParams.FrameHeight + Device_p->VideoParams.VerticalBlankingOffset) %2) == 0))
        {
            HALVIN_SetReconstructedFrameMaxSize(Device_p->HALInputHandle,
                                                Device_p->VideoParams.FrameWidth+Device_p->VideoParams.HorizontalBlankingOffset,
                                                Device_p->VideoParams.FrameHeight+Device_p->VideoParams.VerticalBlankingOffset + 1);
        }
        else
        {
            HALVIN_SetReconstructedFrameMaxSize(Device_p->HALInputHandle,
                                                Device_p->VideoParams.FrameWidth+Device_p->VideoParams.HorizontalBlankingOffset,
                                                Device_p->VideoParams.FrameHeight+Device_p->VideoParams.VerticalBlankingOffset);
        }
#else
        HALVIN_SetReconstructedFrameMaxSize(Device_p->HALInputHandle,
                                            Device_p->VideoParams.FrameWidth+Device_p->VideoParams.HorizontalBlankingOffset,
                                            Device_p->VideoParams.FrameHeight+Device_p->VideoParams.VerticalBlankingOffset);
#endif /* ST_7109 */

        /* Initialise interrupts queue */
        Device_p->InterruptsBuffer.Data_p        = Device_p->InterruptsData;
        Device_p->InterruptsBuffer.TotalSize     = sizeof(Device_p->InterruptsData) / sizeof(U32);
        Device_p->InterruptsBuffer.BeginPointer_p = Device_p->InterruptsBuffer.Data_p;
        Device_p->InterruptsBuffer.UsedSize      = 0;
        Device_p->InterruptsBuffer.MaxUsedSize   = 0;

        /* Want to get frame and display: get a frame first */
        GetPictureBufferParams.PictureStructure     = ((Device_p->VideoParams.ScanType == STGXOBJ_INTERLACED_SCAN) ?
                                                       STVID_PICTURE_STRUCTURE_TOP_FIELD : STVID_PICTURE_STRUCTURE_FRAME);
        GetPictureBufferParams.TopFieldFirst        = TRUE;
        GetPictureBufferParams.ExpectingSecondField = TRUE;
        GetPictureBufferParams.ExtendedTemporalReference = 0;
        GetPictureBufferParams.PictureWidth         = ((HALVIN_Properties_t *) ((HALVIN_Handle_t)Device_p->HALInputHandle))->InputWindow.OutputWinWidth;
        GetPictureBufferParams.PictureHeight        = ((HALVIN_Properties_t *) ((HALVIN_Handle_t)Device_p->HALInputHandle))->InputWindow.OutputWinHeight;

        /* Now, check if STVIN can use a "dummy" buffer in order to */
        /* prevent sVSync and VSync drift.                          */
        if (STVID_MemoryProfile.NbFrameStore >= STVIN_MIN_BUFFER_TO_USE_DUMMY_BUFFER)
        {
            /* Get Dummy buffers (Progressive or Top field buffer).     */
            ErrorCode = STVID_GetPictureBuffer(Device_p->VideoHandle,
                                            &GetPictureBufferParams,
                                            &(Device_p->DummyProgOrTopFieldBufferParams),
                                            &(Device_p->DummyProgOrTopFieldBufferHandle));
                /* Additional traces.                                       */
                VIN_TraceBuffer((">> GetDummy "));
                VIN_TracePictureStructure (GetPictureBufferParams.PictureStructure,
                        GetPictureBufferParams.ExtendedTemporalReference);
                VIN_TraceBuffer(("[%x]\r\n", Device_p->DummyProgOrTopFieldBufferParams.Data1_p));

            if (ErrorCode != ST_NO_ERROR)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Failed to get a free frame buffer"));
                return(ST_ERROR_NO_MEMORY);
            }

            if (GetPictureBufferParams.PictureStructure == STVID_PICTURE_STRUCTURE_TOP_FIELD)
            {
                /* Concerning field management, get also a dummy bottom field.  */
                GetPictureBufferParams.PictureStructure     = STVID_PICTURE_STRUCTURE_BOTTOM_FIELD;
                GetPictureBufferParams.TopFieldFirst        = FALSE;
                GetPictureBufferParams.ExpectingSecondField = FALSE;
                ErrorCode = STVID_GetPictureBuffer(Device_p->VideoHandle,
                                                &GetPictureBufferParams,
                                                &(Device_p->DummyBotFieldBufferParams),
                                                &(Device_p->DummyBotFieldBufferHandle));
                    /* Additional traces.                                       */
                    VIN_TraceBuffer((">> GetDummy "));
                    VIN_TracePictureStructure (GetPictureBufferParams.PictureStructure,
                            GetPictureBufferParams.ExtendedTemporalReference);
                    VIN_TraceBuffer(("[%x]\r\n", Device_p->DummyBotFieldBufferParams.Data1_p));

                /* restore informations for a top field.                        */
                GetPictureBufferParams.PictureStructure     = STVID_PICTURE_STRUCTURE_TOP_FIELD;
                GetPictureBufferParams.TopFieldFirst        = TRUE;
                GetPictureBufferParams.ExpectingSecondField = TRUE;
                /* Top field expected next STVIN's VSync.                       */
                Device_p->ForTask.NextPictureStructureExpected = STVID_PICTURE_STRUCTURE_TOP_FIELD;
            }
            else
            {
                /* A frame is expected next STVIN's VSync.                      */
                Device_p->ForTask.NextPictureStructureExpected = STVID_PICTURE_STRUCTURE_FRAME;
            }
        }
        else
        {
            Device_p->DummyProgOrTopFieldBufferHandle   = (STVID_PictureBufferHandle_t) NULL;
            Device_p->DummyBotFieldBufferHandle         = (STVID_PictureBufferHandle_t) NULL;
        }

        Device_p->DummyBufferUseStatus        = NONE;
        Device_p->PreviousBufferUseStatus     = NONE;

        /* Get a frame to input in */
        ErrorCode = STVID_GetPictureBuffer(Device_p->VideoHandle,
                                           &GetPictureBufferParams,
                                           &(Device_p->PictureBufferParams[0]),
                                           &(Device_p->PictureBufferHandle[0]));

        if (ErrorCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Failed to get a free frame buffer"));
            return(ST_ERROR_NO_MEMORY);
        }

        HALVIN_SetReconstructedFramePointer(Device_p->HALInputHandle,
                                            STVID_PICTURE_STRUCTURE_FRAME,
                                            Device_p->PictureBufferParams[0].Data1_p,
                                            Device_p->PictureBufferParams[0].Data2_p);

        Device_p->ForTask.PictureBufferCurrentIndex = 0;
        Device_p->ForTask.ExtendedTemporalReference = 0;
        Device_p->ForTask.FirstStart                = TRUE;
        Device_p->ForTask.SkipNextBufferAllocation  = FALSE;

        /* Update Picture buffer & Register */
        UpdatePictureBuffer(Device_p,
                (Device_p->VideoParams.ScanType == STGXOBJ_INTERLACED_SCAN) ?
                STVID_PICTURE_STRUCTURE_TOP_FIELD : STVID_PICTURE_STRUCTURE_FRAME,
                &(Device_p->PictureInfos[0]), &(Device_p->PictureBufferParams[0]));
        Device_p->ExtendedTemporalReference[0] = 0;

        /* Read the interrupt status register to clean all bits */
        InterruptStatus = HALVIN_GetInterruptStatus(Device_p->HALInputHandle);

        Device_p->VideoInterrupt.Mask = HALVIN_VERTICAL_SYNC_BOTTOM_MASK | HALVIN_VERTICAL_SYNC_TOP_MASK;
        HALVIN_SetInterruptMask(Device_p->HALInputHandle, Device_p->VideoInterrupt.Mask);

#ifdef ST_stclkrv
        /* Read STC/PCR to compare PTS to it */
        ErrorCode = STCLKRV_GetExtendedSTC(Device_p->STClkrvHandle, &ExtendedSTC);
        if (ErrorCode != ST_NO_ERROR)
        {
            /* No STC available or error: can't tell about synchronization !    */
            CurrentSTC.PTS      =  0;
            CurrentSTC.PTS33    =  FALSE;
            /* This not an error.                                               */
            ErrorCode = ST_NO_ERROR;
        }
        CurrentSTC.PTS      =  ExtendedSTC.BaseValue;
        CurrentSTC.PTS33    = (ExtendedSTC.BaseBit32 != 0);
#else
        /* No STC available : can't tell about synchronization !                */
        CurrentSTC.PTS      =  0;
        CurrentSTC.PTS33    =  FALSE;
#endif

        /* TimeStamp */
        Device_p->PictureInfos[0].VideoParams.PTS.PTS    = CurrentSTC.PTS;
        Device_p->PictureInfos[0].VideoParams.PTS.PTS33  = CurrentSTC.PTS33;
        Device_p->PictureInfos[0].VideoParams.PTS.Interpolated = TRUE;

        /* Set Status */
        Device_p->Status = VIN_DEVICE_STATUS_RUNNING;

        /* Manage also the ancillary data.                      */
        if (Device_p->AncillaryDataEventEnabled)
        {
            /* Set the first ancillary data buffer of the loop. */

            Device_p->EnableAncillaryDatatEventAfterStart = FALSE;

            HALVIN_SetAncillaryDataPointer(Device_p->HALInputHandle, Device_p->AncillaryDataTable[0].Address_p,
                    Device_p->AncillaryDataBufferSize);
            Device_p->ForTask.AncillaryDataCurrentIndex = 0;
        }

        /* Activate the interface */
        HALVIN_EnabledInterface(Device_p->HALInputHandle);
    }

    return(ErrorCode);

} /* End of STVIN_Start() funtcion. */

/*******************************************************************************
Name        : STVIN_Stop
Description : Stop the video input
Parameters  : Video input handle
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STVIN_Stop(const STVIN_Handle_t Handle,
                          const STVIN_Stop_t StopMode)
{
    stvin_Unit_t *Unit_p;
    stvin_Device_t * Device_p;
    U32 InterruptStatus;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    UNUSED_PARAMETER(StopMode);
    VIN_TraceBuffer(("** STVIN_Stop\r\n"));

    if (!IsValidHandle(Handle))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        Unit_p = (stvin_Unit_t *) Handle;

        /* Handle is valid: point to device */
        Device_p = ((stvin_Unit_t *)Handle)->Device_p;

        if (Device_p->Status == VIN_DEVICE_STATUS_STOPPED)
        {
            STTBX_Print(("Already Stopped !"));
            return(ST_NO_ERROR);
        }

        /* desactivate the interface */
        HALVIN_DisableInterface(Device_p->HALInputHandle);

        Device_p->VideoInterrupt.Mask = 0;
        HALVIN_SetInterruptMask(Device_p->HALInputHandle, Device_p->VideoInterrupt.Mask);
        while (PopInterruptCommand(Device_p, &InterruptStatus) == ST_NO_ERROR)
        {
            /* Loop until no more interrupt command in the pipe.    */
            ;
        }
        /* Release all picture buffers. */
        ReleasePictureBuffers (Device_p);

        Device_p->Status = VIN_DEVICE_STATUS_STOPPED;
    }
    return (ErrorCode);

} /* End of STVIN_Stop() funtcion. */


/*******************************************************************************
Name        : StartInputTask
Description : Start the task of decode control
Parameters  :
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
              ST_ERROR_ALREADY_INITIALIZED if task already running
              ST_ERROR_BAD_PARAMETER if problem of creation
*******************************************************************************/
static ST_ErrorCode_t StartInputTask(stvin_Device_t * const Device_p)
{
    VINCOM_Task_t * const Task_p = &(Device_p->InputTask);
    ST_ErrorCode_t Error;

    if (Task_p->IsRunning)
    {
        return(ST_ERROR_ALREADY_INITIALIZED);
    }

    Error = STOS_TaskCreate((void (*) (void*)) InputTaskFunc, (void *) Device_p,
                            Device_p->CPUPartition_p ,
                            STVIN_TASK_STACK_SIZE_INPUT,
                            &(Task_p->TaskStack),
                            Device_p->CPUPartition_p ,
                            &(Task_p->Task_p),
                            &(Task_p->TaskDesc),
                            STVIN_TASK_PRIORITY_INPUT,
                            "STVIN_DigitalInputTask",
                            0);

    if (Error != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Creation of input task failed: exiting..."));
        return(Error);
    }

    Task_p->IsRunning  = TRUE;
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Input task created."));
    return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : StopInputTask
Description : Stop the task of input control
Parameters  :
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if no problem,
              ST_ERROR_ALREADY_INITIALIZED if task not running
*******************************************************************************/
static ST_ErrorCode_t StopInputTask(stvin_Device_t * const Device_p)
{
    VINCOM_Task_t * const Task_p = &(Device_p->InputTask);
    task_t *TaskList_p = Task_p->Task_p;

    if (!(Task_p->IsRunning))
    {
        return(ST_ERROR_ALREADY_INITIALIZED);
    }

    /* End the function of the task here... */
    PushControllerCommand(Device_p, CONTROLLER_COMMAND_TERMINATE);

    /* End the function of the task here... */
    Task_p->ToBeDeleted = TRUE;

    /* Signal controller that a new command is given */
    STOS_SemaphoreSignal(Device_p->ForTask.InputOrder_p);

#ifndef WA_TASK_WAIT_BUGGED_ON_OS20EMU
    STOS_TaskWait(&TaskList_p, TIMEOUT_INFINITY);
#endif
    STOS_TaskDelete ( Task_p->Task_p,
                      Device_p->CPUPartition_p,
                      Task_p->TaskStack,
                      Device_p->CPUPartition_p );

    Task_p->IsRunning  = FALSE;

    return(ST_NO_ERROR);
}


/*******************************************************************************
Name        : InputTaskFunc
Description : Function fo the decode task
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void InputTaskFunc(stvin_Device_t * const Device_p)
{
    /* Overall variables of the task */
    U8   Tmp8;
    BOOL InputTaskFinished = FALSE;

    /* Big loop executed until InputTaskFinished is set */
    do
    {
        /* while not ToBeDeleted */
        /* At each loop: first check if there are interrupts to process. */
        CheckAndProcessInterrupt(Device_p);

        Device_p->ForTask.MaxWaitOrderTime = time_plus(time_now(), MAX_WAIT_ORDER_TIME);

        /* Check if an order is available */
        if (STOS_SemaphoreWaitTimeOut(Device_p->ForTask.InputOrder_p, &(Device_p->ForTask.MaxWaitOrderTime)) == 0)
        {
            /* Process all user commands together only if no start code commands... */
            while (PopControllerCommand(Device_p, &Tmp8/*, (void *) ControllerCommandParams*/) == ST_NO_ERROR)
            {
                switch ((ControllerCommandID_t) Tmp8)
                {
                    case CONTROLLER_COMMAND_TERMINATE :
                        InputTaskFinished = TRUE;
                        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Input task on the way to terminate..."));
                        break;
                    default:
                        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Unknown command!"));
                        break;
                } /* end switch(command) */
            } /* end while(controller command) */

            /* Check if there are interrupts to process after each command process */
            CheckAndProcessInterrupt(Device_p);
        } /* end there's orders, start code or command */
        else
        {
            /* No order */
        }
    } while (!InputTaskFinished);

} /* End of InputTaskFunc() function */

/* Private Functions -------------------------------------------------------- */


/*******************************************************************************
Name        : CheckAndProcessInterrupt
Description : Process interrupt the sooner if there is one
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void CheckAndProcessInterrupt(stvin_Device_t * const Device_p)
{
    U32 InterruptStatus;

    while (PopInterruptCommand(Device_p, &InterruptStatus) == ST_NO_ERROR)
    {
            stvin_ProcessInterrupt(Device_p, InterruptStatus);
    }
} /* End of CheckAndProcessInterrupt() function */


/* End of vin_init.c */



