/*******************************************************************************

File name   : stccinit.c

Description : STCC init manager source file

COPYRIGHT (C) STMicroelectronics 2001.

Date               Modification                                     Name
----               ------------                                     ----
25 June 2001       Created                                           BS
16 July 2001       Modification                                 Michel Bruant
08 Jan 2002        Change a semaphore type to be 'timeout'           HSdLM
*******************************************************************************/

/* Includes ----------------------------------------------------------------- */

#if !defined(ST_OSLINUX)
/* Include system files only if not in Kernel mode */
#include <stdlib.h>
#include <string.h>
#endif


#define CC_RESERV /* this will reserve the stcc variable as NOT extern */
#include "stddefs.h"
#include "stcctask.h"
#include "stcc.h"
#include "stccinit.h"
#include "cc_rev.h"
#include "stos.h"

#if !defined(ST_OSLINUX)
#include "sttbx.h"
#endif


/* Private Constants -------------------------------------------------------- */

#define INVALID_DEVICE_INDEX (-1)

#ifndef STCC_TASK_PRIORITY
#define STCC_TASK_PRIORITY       (MIN_USER_PRIORITY)
#endif

enum
{
    STCC_RECOVERY_ERROR_NONE,
    STCC_RECOVERY_ERROR_HANDLER,
    STCC_RECOVERY_ERROR_VBI_ACCESS,
    STCC_RECOVERY_ERROR_VIDEO_USER_SUB,
    STCC_RECOVERY_ERROR_VIDEO_FRAME_SUB,
    STCC_RECOVERY_ERROR_VTG_SUB,
    STCC_RECOVERY_ERROR_EVT_REGISTER,
    STCC_RECOVERY_ERROR_TASK_CREATE
};

/* Private Variables (static)------------------------------------------------ */

static stcc_Device_t    DeviceArray[STCC_MAX_DEVICE];
static semaphore_t*      InstancesAccessControl_p;
static BOOL             FirstInitDone = FALSE;

/* Global Variables ------------------------------------------------ */


/* Private Function prototypes ---------------------------------------------- */

static void EnterCriticalSection(void);
static void LeaveCriticalSection(void);
static S32 GetIndexOfDeviceNamed(const ST_DeviceName_t WantedName);
static ST_ErrorCode_t Init(stcc_Device_t * const Device_p,
                    const STCC_InitParams_t * const InitParams_p);
static ST_ErrorCode_t Open(stcc_Unit_t * const Unit_p);
static ST_ErrorCode_t Close(stcc_Unit_t * const Unit_p);
static ST_ErrorCode_t Term(stcc_Device_t * const Device_p,
                    const STCC_TermParams_t * const TermParams_p);

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
    static BOOL InstancesAccessControlInitialized = FALSE;

#if !defined(ST_OSLINUX)
    task_lock();
#endif

    if (!InstancesAccessControlInitialized)
    {

        InstancesAccessControlInitialized = TRUE;
        InstancesAccessControl_p = semaphore_create_fifo(1);

    }
#if !defined(ST_OSLINUX)
    task_unlock();
#endif


    semaphore_wait(InstancesAccessControl_p);
}

/*******************************************************************************
Name        : LeaveCriticalSection
Description : Used to release protection of critical  of Init/Open/Close/Term
Parameters  : None
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void LeaveCriticalSection(void)
{
    semaphore_signal(InstancesAccessControl_p);
}

/*******************************************************************************
Name        : Init
Description : API specific initialisations
Parameters  : pointer on device and initialisation parameters
Assumptions : pointer on device is not NULL
Limitations :
Returns     : ST_ErrorCode_t for the API Init function
*******************************************************************************/
static ST_ErrorCode_t Init(stcc_Device_t * const Device_p,
                    const STCC_InitParams_t * const InitParams_p)
{
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;
    STEVT_OpenParams_t EVTOpenParams;
    STVBI_OpenParams_t VBIOpenParams;
    STEVT_DeviceSubscribeParams_t SubscribeParams;
    U8 ErrorRecovery = STCC_RECOVERY_ERROR_NONE;
    U32 i,j;

    /* Test initialisation parameters and exit if some are invalid. */
    if (InitParams_p->CPUPartition_p == NULL)
    {
        CC_Defense(ST_ERROR_BAD_PARAMETER);
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Allocate dynamic data structure */
    Device_p->DeviceData_p = (stcc_DeviceData_t *)
       memory_allocate(InitParams_p->CPUPartition_p, sizeof(stcc_DeviceData_t));
    if (Device_p->DeviceData_p == NULL)
    {
        /* Error of allocation */
        CC_Defense(ST_ERROR_NO_MEMORY);
        return(ST_ERROR_NO_MEMORY);
    }

    memset(Device_p->DeviceData_p, 0x00 , sizeof(stcc_DeviceData_t));

    /* Initialise the allocated structure: most of variables are set-up by */
    /* init (handles, init parameters, etc), so just initialise the rest. */
    /* Default inits */
    Device_p->DeviceData_p->InitParams.CPUPartition_p =
                                                InitParams_p->CPUPartition_p;
    Device_p->DeviceData_p->Task.IsRunning                      = FALSE;
    Device_p->DeviceData_p->Task.ToBeDeleted                    = FALSE;
    Device_p->DeviceData_p->CCDecodeStarted                     = FALSE;
    Device_p->DeviceData_p->FirstSlot                           = BAD_SLOT;
    Device_p->DeviceData_p->NbUsedSlot                          = 0;
    Device_p->DeviceData_p->LastPacket708                       = FALSE;

    for(i =0; i<NB_SLOT; i++)
    {
        for(j =0; j<NB_CCDATA; j++)
        {
            Device_p->DeviceData_p->CaptionSlot[i].CC_Data[j].Field = 0x0;
            Device_p->DeviceData_p->CaptionSlot[i].CC_Data[j].Byte1 = 0x0;
            Device_p->DeviceData_p->CaptionSlot[i].CC_Data[j].Byte2 = 0x0;
        }
        Device_p->DeviceData_p->CaptionSlot[i].DataLength           = 0;
        Device_p->DeviceData_p->CaptionSlot[i].PTS.PTS              = 0xfffffff;
        Device_p->DeviceData_p->CaptionSlot[i].SlotAvailable        = 1;
        Device_p->DeviceData_p->CaptionSlot[i].Next                 = BAD_SLOT;
        Device_p->DeviceData_p->CaptionSlot[i].Previous             = BAD_SLOT;
    }

    for(i=0; i<NB_COPIES; i++)
    {
        Device_p->DeviceData_p->Copies[i].Used      = STCC_EMPTY;
        Device_p->DeviceData_p->Copies[i].Length    = 0;
    }

    /* Check bad parameter */
    if ((InitParams_p->MaxOpen > STCC_MAX_OPEN) ||
        (InitParams_p->EvtHandlerName[0] == '\0') ||
        (InitParams_p->VBIName[0] == '\0') ||
        (InitParams_p->VIDName[0] == '\0') ||
        (InitParams_p->VTGName[0] == '\0') ||
        (strlen(InitParams_p->EvtHandlerName) > sizeof(ST_DeviceName_t)) ||
        (strlen(InitParams_p->VBIName) > sizeof(ST_DeviceName_t)) ||
        (strlen(InitParams_p->VTGName) > sizeof(ST_DeviceName_t)) ||
        (strlen(InitParams_p->VIDName) > sizeof(ST_DeviceName_t))
        )
    {
        ErrCode = ST_ERROR_BAD_PARAMETER;
    }

    if (ErrCode == ST_NO_ERROR)
    {
        /* Save Max open & strings */
        Device_p->DeviceData_p->InitParams.MaxOpen = InitParams_p->MaxOpen;
        strcpy(Device_p->DeviceData_p->InitParams.EvtHandlerName,
                            InitParams_p->EvtHandlerName);
        strcpy(Device_p->DeviceData_p->InitParams.VBIName,
                            InitParams_p->VBIName);
        strcpy(Device_p->DeviceData_p->InitParams.VIDName,
                            InitParams_p->VIDName);
        strcpy(Device_p->DeviceData_p->InitParams.VTGName,
                            InitParams_p->VTGName);
    }

    if (ErrCode == ST_NO_ERROR)
    {
        /* Save ouput mode */
        Device_p->DeviceData_p->OutputMode = STCC_OUTPUT_MODE_NONE;
        Device_p->DeviceData_p->FormatMode = STCC_FORMAT_MODE_DETECT;
        ErrCode = STEVT_Open(InitParams_p->EvtHandlerName, &EVTOpenParams,
                             &Device_p->DeviceData_p->EVTHandle);
        if(ErrCode != ST_NO_ERROR)
        {
            ErrCode = ST_ERROR_BAD_PARAMETER;
            ErrorRecovery = STCC_RECOVERY_ERROR_HANDLER;
        }
        if(ErrCode == ST_NO_ERROR)
        {
            /* Open params for VBI */
            VBIOpenParams.Configuration.VbiType = STVBI_VBI_TYPE_CLOSEDCAPTION;
            VBIOpenParams.Configuration.Type.CC.Mode = STVBI_CCMODE_BOTH;
            ErrCode = STVBI_Open(Device_p->DeviceData_p->InitParams.VBIName,
                             &VBIOpenParams,&Device_p->DeviceData_p->VBIHandle);
            if(ErrCode != ST_NO_ERROR)
            {
                ErrCode = STCC_ERROR_VBI_UNKNOWN;
                ErrorRecovery = STCC_RECOVERY_ERROR_VBI_ACCESS;
            }
        }
        if(ErrCode == ST_NO_ERROR)
        {
            /* Subscribe to UserDataEvent */
            SubscribeParams.NotifyCallback = (STEVT_DeviceCallbackProc_t)stcc_UserDataCallBack;
            SubscribeParams.SubscriberData_p = Device_p;

            ErrCode = STEVT_SubscribeDeviceEvent(
                            Device_p->DeviceData_p->EVTHandle,
                            Device_p->DeviceData_p->InitParams.VIDName,
                            STVID_USER_DATA_EVT, &SubscribeParams);
            if(ErrCode != ST_NO_ERROR)
            {
                ErrCode = STCC_ERROR_EVT_SUBSCRIBE;
                ErrorRecovery = STCC_RECOVERY_ERROR_VIDEO_USER_SUB;
            }
        }
        if(ErrCode == ST_NO_ERROR)
        {
            /* Subscribe to VSyncEvent */
            SubscribeParams.NotifyCallback = (STEVT_DeviceCallbackProc_t)stcc_VSyncCallBack;
            ErrCode = STEVT_SubscribeDeviceEvent(
                            Device_p->DeviceData_p->EVTHandle,
                            Device_p->DeviceData_p->InitParams.VTGName,
                            STVTG_VSYNC_EVT, &SubscribeParams);
            if(ErrCode != ST_NO_ERROR)
            {
                ErrCode = STCC_ERROR_EVT_SUBSCRIBE;
                ErrorRecovery = STCC_RECOVERY_ERROR_VTG_SUB;
            }
        }
        if(ErrCode == ST_NO_ERROR)
        {
             ErrCode = STEVT_RegisterDeviceEvent(
                            Device_p->DeviceData_p->EVTHandle,
                            Device_p->DeviceName,
                            STCC_DATA_TO_BE_PRESENTED_EVT,
                            &Device_p->DeviceData_p->Notify_id);
            /* Event register */
            if(ErrCode != ST_NO_ERROR)
            {
                ErrCode = STCC_ERROR_EVT_REGISTER;
                ErrorRecovery = STCC_RECOVERY_ERROR_EVT_REGISTER;
            }

        }
        if(ErrCode == ST_NO_ERROR)
        {

            /* task creation */
            Device_p->DeviceData_p->Task.SemProcessCC_p = semaphore_create_fifo_timeout(0);
            Device_p->DeviceData_p->SemAccess_p = semaphore_create_fifo(1);


    /* CREATE TASK */
            ErrCode = STOS_TaskCreate ((void (*) (void*))stcc_ProcessCaption,
                                       (void *)Device_p,
                                        Device_p->DeviceData_p->InitParams.CPUPartition_p,
                                        STCC_TASK_STACK_SIZE,
                                        NULL,
                                        Device_p->DeviceData_p->InitParams.CPUPartition_p,
                                        &(Device_p->DeviceData_p->Task.Task_p),
                                        NULL,
                                        STCC_TASK_PRIORITY,
                                        "Closed_Caption_ProcessTask",
                                        task_flags_no_min_stack_size);

            if(ErrCode != ST_NO_ERROR)
            {
                return(ST_ERROR_BAD_PARAMETER);
            }
            else
            {
                /* Task created */
                Device_p->DeviceData_p->Task.IsRunning = TRUE;
            }
        }

        switch(ErrorRecovery)
        {
            /* Break are omited volontary to undo what we already done */
            /* No check to undo what succeeded */
            case STCC_RECOVERY_ERROR_TASK_CREATE:
          semaphore_delete((Device_p->DeviceData_p->Task.SemProcessCC_p));
          semaphore_delete((Device_p->DeviceData_p->SemAccess_p));

                STEVT_UnregisterDeviceEvent(
                                Device_p->DeviceData_p->EVTHandle,
                                Device_p->DeviceName,
                                STCC_DATA_TO_BE_PRESENTED_EVT);
            case STCC_RECOVERY_ERROR_EVT_REGISTER:
                STEVT_UnsubscribeDeviceEvent(
                                Device_p->DeviceData_p->EVTHandle,
                                Device_p->DeviceData_p->InitParams.VTGName,
                                STVTG_VSYNC_EVT);
            case STCC_RECOVERY_ERROR_VTG_SUB:
            case STCC_RECOVERY_ERROR_VIDEO_FRAME_SUB:
                STEVT_UnsubscribeDeviceEvent(
                                Device_p->DeviceData_p->EVTHandle,
                                Device_p->DeviceData_p->InitParams.VIDName,
                                STVID_USER_DATA_EVT);
            case STCC_RECOVERY_ERROR_VIDEO_USER_SUB:
                STVBI_Close(Device_p->DeviceData_p->VBIHandle);
            case STCC_RECOVERY_ERROR_VBI_ACCESS:
                STEVT_Close(Device_p->DeviceData_p->EVTHandle);
            case STCC_RECOVERY_ERROR_HANDLER: /* Nothing to undo */
            case STCC_RECOVERY_ERROR_NONE:
            default:
                break;
        }
    }
    if (ErrCode != ST_NO_ERROR)
    {
        CC_Defense(ErrCode);
        /* De-allocate dynamic data structure */
        memory_deallocate(InitParams_p->CPUPartition_p, (void *)
                Device_p->DeviceData_p);
        Device_p->DeviceData_p = NULL;

    }

    STCC_DebugStats.NbUserDataReceived         = 0;
    STCC_DebugStats.NbUserDataParsed           = 0;
    STCC_DebugStats.NBCaptionDataFound         = 0;
    STCC_DebugStats.NBCaptionDataPresented     = 0;

    return(ErrCode);
}

/*******************************************************************************
Name        : Open
Description : API specific actions just before opening
Parameters  : pointer on unit
Assumptions : pointer on unit is not NULL
Limitations :
Returns     : ST_NO_ERROR
*******************************************************************************/
static ST_ErrorCode_t Open(stcc_Unit_t * const Unit_p)
{
    UNUSED_PARAMETER(Unit_p);
    return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : Close
Description : API specific actions just before closing
Parameters  : pointer on unit
Assumptions : pointer on unit is not NULL
Limitations :
Returns     : ST_NO_ERROR
*******************************************************************************/
static ST_ErrorCode_t Close(stcc_Unit_t * const Unit_p)
{
    UNUSED_PARAMETER(Unit_p);
    return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : Term
Description : API specific terminations
Parameters  : pointer on device and termination parameters
Assumptions : pointer on device is not NULL
Limitations :
Returns     : ST_ErrorCode_t for the API Term function
*******************************************************************************/
static ST_ErrorCode_t Term(stcc_Device_t * const Device_p,
                    const STCC_TermParams_t * const TermParams_p)
{
    ST_ErrorCode_t RetCode, ErrorCode = ST_NO_ERROR;
    task_t *TaskPointer_p;

    if (Device_p->DeviceData_p == NULL)
    {
        CC_Defense(ST_ERROR_BAD_PARAMETER);
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Is the process stopped */
    if((Device_p->DeviceData_p->CCDecodeStarted)
            && (!TermParams_p->ForceTerminate))
    {
        CC_Defense(STCC_ERROR_DECODER_RUNNING);
        return(STCC_ERROR_DECODER_RUNNING);
    }

    if(Device_p->DeviceData_p->CCDecodeStarted)
    {
        RetCode = STVBI_Disable(Device_p->DeviceData_p->VBIHandle);
        if(RetCode != ST_NO_ERROR)
            ErrorCode = STCC_ERROR_VBI_ACCESS;
        Device_p->DeviceData_p->CCDecodeStarted = FALSE;
    }

    /* Delete CC Task */
    if(Device_p->DeviceData_p->Task.IsRunning)
    {
        Device_p->DeviceData_p->Task.ToBeDeleted = TRUE;

        semaphore_signal((Device_p->DeviceData_p->Task.SemProcessCC_p));
        TaskPointer_p = (Device_p->DeviceData_p->Task.Task_p);


        STOS_TaskWait( &TaskPointer_p, TIMEOUT_INFINITY );
        ErrorCode = STOS_TaskDelete ( TaskPointer_p,
                                      Device_p->DeviceData_p->InitParams.CPUPartition_p,
                                      NULL,
                                      Device_p->DeviceData_p->InitParams.CPUPartition_p);

        if (ErrorCode != ST_NO_ERROR)
        {
            ErrorCode = STCC_ERROR_TASK_CALL;
        }
        Device_p->DeviceData_p->Task.IsRunning = FALSE;
        semaphore_delete((Device_p->DeviceData_p->Task.SemProcessCC_p));
        semaphore_wait((Device_p->DeviceData_p->SemAccess_p));
        semaphore_delete((Device_p->DeviceData_p->SemAccess_p));
    }

    RetCode = STEVT_UnsubscribeDeviceEvent(Device_p->DeviceData_p->EVTHandle,
                                   Device_p->DeviceData_p->InitParams.VTGName,
                                   STVTG_VSYNC_EVT);
    if(RetCode != ST_NO_ERROR)
    {
        ErrorCode = STCC_ERROR_EVT_SUBSCRIBE;
    }
    if(RetCode != ST_NO_ERROR)
    {
        ErrorCode = STCC_ERROR_EVT_SUBSCRIBE;
    }

    RetCode = STEVT_UnsubscribeDeviceEvent(Device_p->DeviceData_p->EVTHandle,
                                   Device_p->DeviceData_p->InitParams.VIDName,
                                   STVID_USER_DATA_EVT);
    if(RetCode != ST_NO_ERROR)
    {
        ErrorCode = STCC_ERROR_EVT_SUBSCRIBE;
    }
    RetCode = STEVT_UnregisterDeviceEvent(Device_p->DeviceData_p->EVTHandle,
                                    Device_p->DeviceName,
                                    STCC_DATA_TO_BE_PRESENTED_EVT);
    if(RetCode != ST_NO_ERROR)
    {
        ErrorCode = STCC_ERROR_EVT_REGISTER;
    }
    RetCode = STVBI_Close(Device_p->DeviceData_p->VBIHandle);
    if(RetCode != ST_NO_ERROR)
    {
        ErrorCode = STCC_ERROR_VBI_ACCESS;
    }

    RetCode = STEVT_Close(Device_p->DeviceData_p->EVTHandle);
    if(RetCode != ST_NO_ERROR)
    {
        ErrorCode = STCC_ERROR_EVT_SUBSCRIBE;
    }

    /* We have succeded to terminate: de-allocate dynamic data structure */
    memory_deallocate(Device_p->DeviceData_p->InitParams.CPUPartition_p,
                    (void *) Device_p->DeviceData_p);
    Device_p->DeviceData_p = NULL;
    if(RetCode != ST_NO_ERROR)
    {
        CC_Defense(ErrorCode);
    }

    return(ErrorCode);
}

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
    } while ((WantedIndex == INVALID_DEVICE_INDEX)
            && (Index < STCC_MAX_DEVICE));

    return(WantedIndex);
}

/*******************************************************************************
*                               API FUNCTIONS                                  *
*******************************************************************************/

/*
--------------------------------------------------------------------------------
Get revision of stcc API
--------------------------------------------------------------------------------
*/
ST_Revision_t STCC_GetRevision(void)
{
/*    static const char Revision[] = "STCC-REL_2.0.9";
    return((ST_Revision_t) Revision);*/

    return(CC_Revision);
} /* End of STCC_GetRevision() function */



/*
--------------------------------------------------------------------------------
Initialise stcc driver
(Standard instances management. Driver specific implementations should be put in Init() function.)
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STCC_Init(const ST_DeviceName_t DeviceName,
        const STCC_InitParams_t * const InitParams_p)
{
    S32 Index = 0;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if ((InitParams_p == NULL)
        ||(InitParams_p->MaxOpen > STCC_MAX_OPEN)
        ||(strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1)
        || ((((const char *) DeviceName)[0]) == '\0'))
    {
        CC_Defense(ST_ERROR_BAD_PARAMETER);
        return(ST_ERROR_BAD_PARAMETER);
    }

    EnterCriticalSection();

    /* First time: initialise devices and units as empty */
    if (!FirstInitDone)
    {
        for (Index = 0; Index < STCC_MAX_DEVICE; Index++)
        {
            DeviceArray[Index].DeviceName[0] = '\0';
            DeviceArray[Index].DeviceData_p = NULL;
        }

        for (Index = 0; Index < STCC_MAX_UNIT; Index++)
        {
            stcc_UnitArray[Index].UnitValidity = 0;
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
        /* Look for a non-init device and return error if none is available */
        Index = 0;
        while ((DeviceArray[Index].DeviceName[0] != '\0')
                && (Index < STCC_MAX_DEVICE))
        {
            Index++;
        }
        if (Index >= STCC_MAX_DEVICE)
        {
            /* All devices initialised */
            ErrorCode = ST_ERROR_NO_MEMORY;
        }
        else
        {
            /* API specific initialisations */
            strcpy(DeviceArray[Index].DeviceName, DeviceName);
            ErrorCode = Init(&DeviceArray[Index], InitParams_p);

            if (ErrorCode == ST_NO_ERROR)
            {
                strcpy(DeviceArray[Index].DeviceName, DeviceName);
                DeviceArray[Index].DeviceName[ST_MAX_DEVICE_NAME - 1] = '\0';
                DeviceArray[Index].DeviceData_p->InitParams.MaxOpen
                                            = InitParams_p->MaxOpen;

                CC_Trace(("Device CC initialised"));
            }
            else
            {
                strcpy(DeviceArray[Index].DeviceName,"");
            }
        } /* End exists device not initialised */
    } /* End device not already initialised */

    LeaveCriticalSection();
    if(ErrorCode != ST_NO_ERROR)
    {
        CC_Defense(ErrorCode);
    }
    return(ErrorCode);
}

/*
--------------------------------------------------------------------------------
open ...
(Standard instances management. Driver specific implementations should be put in Open() function.)
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STCC_Open(const ST_DeviceName_t DeviceName,
         const STCC_OpenParams_t * const OpenParams_p, STCC_Handle_t *Handle_p)
{
    S32 DeviceIndex = INVALID_DEVICE_INDEX, UnitIndex, Index;
    U32 OpenedUnitForThisInit;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if ((OpenParams_p == NULL)
        || (Handle_p == NULL)
        || (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1)
        || ((((const char *) DeviceName)[0]) == '\0'))
    {
        CC_Defense(ST_ERROR_BAD_PARAMETER);
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
            UnitIndex = STCC_MAX_UNIT;
            OpenedUnitForThisInit = 0;
            for (Index = 0; Index < STCC_MAX_UNIT; Index++)
            {
                if ((stcc_UnitArray[Index].UnitValidity == STCC_VALID_UNIT)
               && (stcc_UnitArray[Index].Device_p == &DeviceArray[DeviceIndex]))
                {
                    OpenedUnitForThisInit ++;
                }
                if (stcc_UnitArray[Index].UnitValidity != STCC_VALID_UNIT)
                {
                    /* Found a free handle structure */
                    UnitIndex = Index;
                }
            }
            if ((OpenedUnitForThisInit
                   >= DeviceArray[DeviceIndex].DeviceData_p->InitParams.MaxOpen)
                || (UnitIndex >= STCC_MAX_UNIT))
            {
                /* None of the units is free or MaxOpen reached */
                ErrorCode = ST_ERROR_NO_FREE_HANDLES;
            }
            else
            {
                *Handle_p = (STCC_Handle_t) &stcc_UnitArray[UnitIndex];
                stcc_UnitArray[UnitIndex].Device_p = &DeviceArray[DeviceIndex];

                /* API specific actions after opening */
                ErrorCode = Open(&stcc_UnitArray[UnitIndex]);

                if (ErrorCode == ST_NO_ERROR)
                {
                    /* Register opened handle */
                    stcc_UnitArray[UnitIndex].UnitValidity = STCC_VALID_UNIT;
                    CC_Trace(("Handle opened on device CC"));
                }
            } /* End found unit unused */
        } /* End device valid */
    } /* End FirstInitDone */

    LeaveCriticalSection();
    if(ErrorCode != ST_NO_ERROR)
    {
        CC_Defense(ErrorCode);
    }

    return(ErrorCode);
}

/*
--------------------------------------------------------------------------------
Close ...
(Standard instances management. Driver specific implementations should be put in Close() function.)
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STCC_Close(const STCC_Handle_t Handle)
{
    stcc_Unit_t *Unit_p;
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
            Unit_p = (stcc_Unit_t *) Handle;

            /* API specific actions before closing */
            ErrorCode = Close(Unit_p);

            /* Close only if no errors */
            if (ErrorCode == ST_NO_ERROR)
            {
                /* Un-register opened handle */
                Unit_p->UnitValidity = 0;
                CC_Trace(("Handle closed on device CC"));
            }
        } /* End handle valid */
    } /* End FirstInitDone */

    LeaveCriticalSection();
    if(ErrorCode != ST_NO_ERROR)
    {
        CC_Defense(ErrorCode);
    }

    return(ErrorCode);
}

/*
--------------------------------------------------------------------------------
Terminate stcc driver
(Standard instances management. Driver specific implementations should be put in Term() function.)
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STCC_Term(const ST_DeviceName_t DeviceName,
        const STCC_TermParams_t *const TermParams_p)
{
    stcc_Unit_t *Unit_p;
    S32 DeviceIndex = INVALID_DEVICE_INDEX, UnitIndex;
    BOOL Found = FALSE;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if ((TermParams_p == NULL)
        || (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1)
        ||((((const char *) DeviceName)[0]) == '\0'))
    {
        CC_Defense(ST_ERROR_BAD_PARAMETER);
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
            Found = FALSE;
            UnitIndex = 0;
            Unit_p = stcc_UnitArray;
            while ((UnitIndex < STCC_MAX_UNIT) && (!Found))
            {
                Found = ((Unit_p->Device_p == &DeviceArray[DeviceIndex])
                        && (Unit_p->UnitValidity == STCC_VALID_UNIT));
                Unit_p++;
                UnitIndex++;
            }

            if (Found)
            {
                if (TermParams_p->ForceTerminate)
                {
                    UnitIndex = 0;
                    Unit_p = stcc_UnitArray;
                    while (UnitIndex < STCC_MAX_UNIT)
                    {
                        if ((Unit_p->Device_p == &DeviceArray[DeviceIndex])
                                && (Unit_p->UnitValidity == STCC_VALID_UNIT))
                        {
                            /* Found an open instance: close it ! */
                            ErrorCode = Close(Unit_p);
                            if (ErrorCode == ST_NO_ERROR)
                            {
                                /* Un-register opened handle */
                                Unit_p->UnitValidity = 0;

                                CC_Trace(("Handle closed on device CC"));
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
                } /* End ForceTerminate: closed all opened handles */
                else
                {
                    ErrorCode = ST_ERROR_OPEN_HANDLE;
                }
            } /* End found handle not closed */

            /* Terminate if OK */
            if (ErrorCode == ST_NO_ERROR)
            {
                /* API specific terminations */
                ErrorCode = Term(&DeviceArray[DeviceIndex], TermParams_p);
                if(ErrorCode != STCC_ERROR_DECODER_RUNNING)
                {
                    /* Device found: desallocate memory, free device */
                    DeviceArray[DeviceIndex].DeviceName[0] = '\0';
                    CC_Trace(("Device CC terminated"));
                }
            } /* End terminate OK */
        } /* End valid device */
    } /* End FirstInitDone */

    LeaveCriticalSection();
    if(ErrorCode != ST_NO_ERROR)
    {
        CC_Defense(ErrorCode);
    }

    return(ErrorCode);
}

