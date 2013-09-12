/*******************************************************************************

File name   : vid_xcode.c

Description : Encode/Transcode common functions commands source file

COPYRIGHT (C) STMicroelectronics 2006

Date               Modification                                     Name
----               ------------                                     ----
 28 Feb 2007        Created                                         LM
*******************************************************************************/

/* Includes ----------------------------------------------------------------- */
#if !defined ST_OSLINUX
#include <stdlib.h>
#include <string.h>
#endif

#include "stddefs.h"
#include "stos.h"

#include "api.h"
#include "vid_rev.h"
#include "vid_com.h"
#include "vid_xcode.h"
#include "transcoder.h"

/* Private Constants -------------------------------------------------------- */

#define INVALID_DEVICE_INDEX (-1)

#define MAX_DEVICE_TYPES    5

/* Private Types ------------------------------------------------------------ */

/* Stores pairs of (transcoder types - number of instances of this type) */
typedef struct InstancesOfTrancoderDeviceType_s
{
/*    STVID_TranscoderType_t  TranscoderType;*/
    U8                      NumberOfInitialisedInstances;
    /* Below : handles to instances of modules that are required only ONCE per TranscoderType whatever the number of Transcode instances. */
} InstancesOfTrancoderDeviceType_t;

/* Type used to allocate number of structures necessary for the instance */
typedef struct TranscoderParameters_s
{
    U32                     Dummy;
} TranscoderParameters_t;

/* Private Variables (static)------------------------------------------------ */
static TranscoderDevice_t TranscoderDeviceArray[STVID_MAX_DEVICE];
/* Init/Open/Close/Term/GetCapability/GetStatistics/ResetStatistics protection semaphore */
static semaphore_t * InterTranscoderInstancesLockingSemaphore_p = NULL;
static BOOL FirstTranscoderInitDone = FALSE;
#if 0
static InstancesOfTrancoderDeviceType_t TranscoderInstancesSummary[MAX_DEVICE_TYPES];
#endif

/* Global Variables --------------------------------------------------------- */

/* not static because used in IsValidHandle macro */
TranscoderUnit_t stvid_TranscoderUnitArray[STVID_MAX_UNIT];

/* Private Macros ----------------------------------------------------------- */

/* Private Function prototypes ---------------------------------------------- */
static S32 GetIndexOfTranscoderDeviceNamed(const ST_DeviceName_t WantedName);
#if 0
static ST_ErrorCode_t FindInstancesOfSameTranscoderDeviceType(const STVID_TranscoderDeviceType_t DeviceType, InstancesOfTranscoderDeviceType_t ** const InstancesOfTranscoderDeviceType_p);
#endif
static ST_ErrorCode_t TranscoderInit(TranscoderDevice_t * const Device_p, const STVID_TranscoderInitParams_t * const TranscoderInitParams_p, const ST_DeviceName_t DeviceName);
static ST_ErrorCode_t TranscoderTerm(TranscoderDevice_t * const Device_p, const STVID_TranscoderTermParams_t * const TermParams_p);
static ST_ErrorCode_t TranscoderOpen(TranscoderUnit_t * const Unit_p, const STVID_TranscoderOpenParams_t * const TranscoderOpenParams_p);
static ST_ErrorCode_t TranscoderClose(TranscoderUnit_t * const Unit_p);

/* Private functions -------------------------------------------------------- */

/*******************************************************************************
Name        : GetIndexOfTranscoderDeviceNamed
Description : Get the index in DeviceArray where a device has been
              initialised with the wanted name, if it exists
Parameters  : the name to look for
Assumptions :
Limitations :
Returns     : the index if the name was found, INVALID_DEVICE_INDEX otherwise
*******************************************************************************/
static S32 GetIndexOfTranscoderDeviceNamed(const ST_DeviceName_t WantedName)
{
    S32 WantedIndex = INVALID_DEVICE_INDEX, Index = 0;

    do
    {
        /* Device initialised: check if name is matching */
        if (strcmp(TranscoderDeviceArray[Index].DeviceName, WantedName) == 0)
        {
            /* Name found in the initialised devices */
            WantedIndex = Index;
        }
        Index++;
    } while ((WantedIndex == INVALID_DEVICE_INDEX) && (Index < STVID_MAX_DEVICE));

    return(WantedIndex);
} /* End of GetIndexOfTranscoderDeviceNamed() function */

#if 0
/*******************************************************************************
Name        : FindInstancesOfSameTranscoderDeviceType
Description : Look for instances of the same device type that are already initialised. If not found, take a free index
Parameters  : Device type
              pointer to return row in table of all instances, the row where this device types are counted
Assumptions : if NumberOfInitialisedInstances is 0, this means that there is no instance of the same device
Limitations :
Returns     : ST_NO_ERROR if success
*******************************************************************************/
static ST_ErrorCode_t FindInstancesOfSameTranscoderDeviceType(const STVID_TranscoderDeviceType_t TranscoderDeviceType, InstancesOfTranscoderDeviceType_t ** const InstancesOfTranscoderDeviceType_p)
{
    S8 Index, IndexOfSameInstances;

    /* Look for instances of the same device type that are already initialised. If not found, take a free index. */
    IndexOfSameInstances = MAX_DEVICE_TYPES; /* Outside the table, means invalid... */
    for (Index = (MAX_DEVICE_TYPES - 1); Index >= 0; Index--)
    {
        if (TranscoderInstancesSummary[Index].NumberOfInitialisedInstances == 0)
        {
            /* Index of the table is not used, so eventually take it if no other instances of same device type is found */
            IndexOfSameInstances = Index;
        }
        /* CL quick fix to deal with 7100_H264 & 7100_MPEG instances which share the same VID_INJ instance */
        else if ((TranscoderInstancesSummary[Index].DeviceType == DeviceType) ||
                (((DeviceType == STVID_DEVICE_TYPE_7100_MPEG) || (DeviceType == STVID_DEVICE_TYPE_7100_H264) || (DeviceType == STVID_DEVICE_TYPE_7109_VC1) || (DeviceType == STVID_DEVICE_TYPE_7109_MPEG) || (DeviceType == STVID_DEVICE_TYPE_7109_H264) || (DeviceType == STVID_DEVICE_TYPE_7109D_MPEG) || DeviceType == STVID_DEVICE_TYPE_7100_MPEG4P2 || DeviceType == STVID_DEVICE_TYPE_7109_MPEG4P2) &&
                  ((TranscoderInstancesSummary[Index].DeviceType == STVID_DEVICE_TYPE_7100_MPEG)||(TranscoderInstancesSummary[Index].DeviceType == STVID_DEVICE_TYPE_7100_H264)|| (TranscoderInstancesSummary[Index].DeviceType == STVID_DEVICE_TYPE_7109_MPEG) || (TranscoderInstancesSummary[Index].DeviceType == STVID_DEVICE_TYPE_7109D_MPEG) || (TranscoderInstancesSummary[Index].DeviceType == STVID_DEVICE_TYPE_7109_H264) ||(TranscoderInstancesSummary[Index].DeviceType == STVID_DEVICE_TYPE_7109_VC1) ||
                  (TranscoderInstancesSummary[Index].DeviceType == STVID_DEVICE_TYPE_7100_MPEG4P2) || (TranscoderInstancesSummary[Index].DeviceType == STVID_DEVICE_TYPE_7109_MPEG4P2))) )
        {
            /* Found some instances of same device type ! End the 'for' by breaking. */
            IndexOfSameInstances = Index;
            break;
        }
    }
    /* Take un-used or confirm used index of table */
    TranscoderInstancesSummary[IndexOfSameInstances].DeviceType = TranscoderDeviceType;
    *InstancesOfTranscoderDeviceType_p = &(TranscoderInstancesSummary[IndexOfSameInstances]);

    return(ST_NO_ERROR);
} /* end of FindInstancesOfSameTranscoderDeviceType() function */

#endif

/*******************************************************************************
Name        : stvid_EnterTranscoderCriticalSection
Description : Enter in a section protected against mutual access of the same object
              (data base of Devices and Units) by different processes.
Parameters  : * TranscoderDevice_p, pointer to a transcoder device, to protect access to device data.
              * NULL, to protect access to static table of devices and units.
Assumptions : If TranscoderDevice_p is not NULL, the pointer is supposed to be
              correct (Valid TranscoderDevice_t pointed).
Limitations :
Returns     : Nothing.
*******************************************************************************/
void stvid_EnterTranscoderCriticalSection(TranscoderDevice_t * TranscoderDevice_p)
{
    if (TranscoderDevice_p == NULL)
    {
        /* Enter this section only if before first Init (InterTranscoderInstancesLockingSemaphore_p default set at NULL) */
        if (InterTranscoderInstancesLockingSemaphore_p == NULL)
        {
            semaphore_t * TemporaryInterTranscoderInstancesLockingSemaphore_p;
            BOOL          TemporarySemaphoreNeedsToBeDeleted;

            /* We can't use the instance's CPUPartition_p as it resides inside the structure that we want to protect
             * and may not even exist when stvid_EnterCriticalFunction() is called. So we pass NULL as a partition
             * and STOS_SemaphoreCreatePriority will know how to react depending on OS. */
            TemporaryInterTranscoderInstancesLockingSemaphore_p = STOS_SemaphoreCreatePriority(NULL, 1);

            /* This case is to protect general access of video functions, like init/open/close and term */
            /* We do not want to call semaphore_create within the task_lock/task_unlock boundaries because
             * semaphore_create internally calls semaphore_wait and can cause normal scheduling to resume,
             * therefore unlocking the critical region */
            STOS_TaskLock();
            if (InterTranscoderInstancesLockingSemaphore_p == NULL)
            {
                InterTranscoderInstancesLockingSemaphore_p = TemporaryInterTranscoderInstancesLockingSemaphore_p;
                TemporarySemaphoreNeedsToBeDeleted = FALSE;
            }
            else
            {
                /* Remember to delete the temporary semaphore, if the InterTranscoderInstancesLockingSemaphore_p
                 * was already created by another video instance */
                TemporarySemaphoreNeedsToBeDeleted = TRUE;
            }
            STOS_TaskUnlock();
            /* Delete the temporary semaphore */
            if(TemporarySemaphoreNeedsToBeDeleted)
            {
                STOS_SemaphoreDelete(NULL, TemporaryInterTranscoderInstancesLockingSemaphore_p);
            }
        }
        STOS_SemaphoreWait(InterTranscoderInstancesLockingSemaphore_p);
    }
    else
    {
        /* This case is for a specific function of the device (start, stop, ...) */
        STOS_SemaphoreWait(TranscoderDevice_p->DeviceControlLockingSemaphore_p);
    }

} /* end of stvid_EnterTranscoderCriticalSection function */


/*******************************************************************************
Name        : stvid_LeaveTranscoderCriticalSection
Description : Leave the section protected against mutual access of the same object
              (database of Devices and Units) by different processes.
Parameters  : TranscoderDevice_p, pointer to a transcoder device.
Assumptions : If TranscoderDevice_p is not NULL, the pointer is supposed to be
              correct (Valid TranscoderDevice_t pointed).
Limitations :
Returns     : Nothing.
*******************************************************************************/
void stvid_LeaveTranscoderCriticalSection(TranscoderDevice_t * TranscoderDevice_p)
{
    if (TranscoderDevice_p == NULL)
    {
        /* Release the Init/Open/Close/Term protection semaphore */
        STOS_SemaphoreSignal(InterTranscoderInstancesLockingSemaphore_p);
    }
    else
    {
        STOS_SemaphoreSignal(TranscoderDevice_p->DeviceControlLockingSemaphore_p);
    }
} /* end of stvid_LeaveTranscoderCriticalSection function */

/*******************************************************************************
Name        : TranscoderInit
Description : API specific initialisations
Parameters  : pointer on device and initialisation parameters
Assumptions : pointer on device is not NULL
Limitations :
Returns     : ST_ErrorCode_t for the API InitTranscoder function
*******************************************************************************/
static ST_ErrorCode_t TranscoderInit(TranscoderDevice_t * const Device_p, const STVID_TranscoderInitParams_t * const TranscoderInitParams_p, const ST_DeviceName_t DeviceName)
{
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;
    VIDTRAN_Handle_t        TranscoderHandle;
    VIDTRAN_InitParams_t    TranscoderInitParams;

    /* Test initialisation parameters and exit if some are invalid. */
    if (TranscoderInitParams_p->CPUPartition_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Allocate dynamic data structure */
    SAFE_MEMORY_ALLOCATE( Device_p->DeviceData_p, TranscoderDeviceData_t *, TranscoderInitParams_p->CPUPartition_p, sizeof(TranscoderDeviceData_t) );
    if (Device_p->DeviceData_p == NULL)
    {
        /* Error of allocation */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Unable to allocate memory for structure !"));
    }

    Device_p->DeviceData_p->TranscoderHandleIsValid = FALSE;

    /* Copy the init structure */
    strcpy(TranscoderInitParams.TranscoderName, DeviceName);
    strcpy(TranscoderInitParams.EvtHandlerName, TranscoderInitParams_p->EvtHandlerName);
    strcpy(TranscoderInitParams.TargetDeviceName, TranscoderInitParams_p->TargetDeviceName);
    TranscoderInitParams.CPUPartition_p          = TranscoderInitParams_p->CPUPartition_p;
    TranscoderInitParams.AVMEMPartition          = TranscoderInitParams_p->AVMEMPartition;
    TranscoderInitParams.OutputStreamProfile     = TranscoderInitParams_p->OutputStreamProfile;
    TranscoderInitParams.Xcode_InitParams        = TranscoderInitParams_p->Xcode_InitParams;         /* All structure copied */
    TranscoderInitParams.OutputBufferAllocated   = TranscoderInitParams_p->IsEncoderOutputDataBufferAllocated;
    TranscoderInitParams.OutputBufferAddress_p   = TranscoderInitParams_p->EncoderOutputDataBufferAddress_p;
    TranscoderInitParams.OutputBufferSize        = TranscoderInitParams_p->EncoderOutputDataBufferSize;

    ErrorCode = VIDTRAN_Init(&TranscoderInitParams, &TranscoderHandle);
    if (ErrorCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Transcoder module failed to initialize !"));
        /* Undo initializations done. */
        /* TODO */
    }
    Device_p->DeviceData_p->CPUPartition_p          = TranscoderInitParams_p->CPUPartition_p;
    Device_p->DeviceData_p->TranscoderHandle        = TranscoderHandle;
    Device_p->DeviceData_p->TranscoderHandleIsValid = TRUE;

    return (ErrorCode);
} /* TranscoderInit */

/*******************************************************************************
Name        : TranscoderTerm
Description : API specific terminations: should go-on as much as possible even if failures
Parameters  : pointer on device and termination parameters
Assumptions : pointer on device is not NULL
Limitations :
Returns     : ST_ErrorCode_t for the API Term function
*******************************************************************************/
static ST_ErrorCode_t TranscoderTerm(TranscoderDevice_t * const Device_p, const STVID_TranscoderTermParams_t * const TranscoderTermParams_p)
{
    ST_ErrorCode_t                          ErrorCode = ST_NO_ERROR;
#if 0
    InstancesOfTranscoderDeviceType_t *     InstancesOfDeviceType_p;
#endif

    UNUSED_PARAMETER(TranscoderTermParams_p);

    if (Device_p->DeviceData_p == NULL)
    {
        /* Error case ! */
        return(ST_ERROR_BAD_PARAMETER);
    }

    stvid_EnterTranscoderCriticalSection(Device_p);

    /* TO DO */
#if 0
    ErrorCode = FindInstancesOfSameTranscoderDeviceType(Device_p->DeviceData_p->DeviceType, &InstancesOfDeviceType_p); *//* no error possible */
#endif

    /* Before terminating: stop the transcoder (if not already the case). */
    /* TO DO */
    /* end decoder not stopped */

    /* Remove protection to terminate modules which delete some tasks: indeed, if
       we are inside a protected area, we may go to a blocking state: the task may
       be inside a STEVT_Notify(), inside our callback which can be also protected,
       so waiting on the protection semaphore. In that case, we will wait for ever !*/

    stvid_LeaveTranscoderCriticalSection(Device_p);

    /* Termination of the Transcoder */
    if (VIDTRAN_Term(Device_p->DeviceData_p->TranscoderHandle) != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Transcoder termination failed !"));
        ErrorCode = ST_ERROR_BAD_PARAMETER;
    }

#if 0
    /* Close event handler. This also un-registers and un-subscribes all events. */
    ErrorCode = STEVT_Close(Device_p->DeviceData_p->EventsHandle);
    if (ErrorCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error while closing event handler"));
        ErrorCode = ST_ERROR_BAD_PARAMETER;
    }
#endif

    stvid_EnterTranscoderCriticalSection(Device_p);

    /* We have succeded to terminate: consider transcoder number no more in use */
#if 0
    Device_p->TranscoderNumber = STVID_INVALID_DECODER_NUMBER;
#endif

    Device_p->DeviceData_p->TranscoderHandleIsValid = FALSE;

    /* De-allocate dynamic data structure */
    SAFE_MEMORY_DEALLOCATE(Device_p->DeviceData_p, Device_p->DeviceData_p->CPUPartition_p,  sizeof(TranscoderDeviceData_t));
    Device_p->DeviceData_p = NULL;

#if 0
    /* Aknowledge that there is now one less instance of this device type */
    InstancesOfDeviceType_p->NumberOfInitialisedInstances -= 1;
#endif

    stvid_LeaveTranscoderCriticalSection(Device_p);

    return(ErrorCode);

} /* End of TranscoderTerm() function */

/*******************************************************************************
Name        : TranscoderOpen
Description : API specific actions just before opening
Parameters  : pointer on unit
Assumptions : pointer on unit is not NULL
Limitations :
Returns     : ST_ErrorCode_t for the API Close function
              If not ST_NO_ERROR, the Handle would not be closed afterwards
*******************************************************************************/
static ST_ErrorCode_t TranscoderOpen(TranscoderUnit_t * const Unit_p, const STVID_TranscoderOpenParams_t * const TranscoderOpenParams_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    if (Unit_p->Device_p->DeviceData_p == NULL)
    {
        /* Error case ! */
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Open source and target decoders */
    ErrorCode = VIDTRAN_Open(Unit_p->Device_p->DeviceData_p->TranscoderHandle, TranscoderOpenParams_p);

    /* If success of Open(): increment number of open handles */
    if (ErrorCode == ST_NO_ERROR)
    {
        /* Increment number of open handles */
        Unit_p->Device_p->DeviceData_p->NumberOfOpenSinceInit ++;
    }

    return(ErrorCode);
} /* End of TranscoderOpen() function */

/*******************************************************************************
Name        : TranscoderClose
Description : API specific actions just before closing
Parameters  : pointer on unit
Assumptions : pointer on unit is not NULL
Limitations :
Returns     : ST_ErrorCode_t for the API Close function
              If not ST_NO_ERROR, the Handle would not be closed afterwards
*******************************************************************************/
static ST_ErrorCode_t TranscoderClose(TranscoderUnit_t * const Unit_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    if (Unit_p->Device_p->DeviceData_p == NULL)
    {
        /* Error case ! */
        return(ST_ERROR_BAD_PARAMETER);
    }

    return(ErrorCode);
} /* End of TranscoderClose() function */

/* Exported functions ------------------------------------------------------- */
/*******************************************************************************
Name        : STVID_TranscoderInit
Description : Initializes a Video Transcoder and gives it the name DeviceName.
Parameters  :
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t for the API InitTranscoder function
*******************************************************************************/
ST_ErrorCode_t STVID_TranscoderInit(const ST_DeviceName_t DeviceName,
    const STVID_TranscoderInitParams_t * const TranscoderInitParams_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    S32 Index = 0;

    /* Exit now if parameters are bad */
    if ((TranscoderInitParams_p == NULL) ||                       /* There must be parameters */
        (TranscoderInitParams_p->Xcode_InitParams.MaxOpen > STVID_MAX_OPEN) ||     /* No more than MAX_OPEN open handles supported */
        (TranscoderInitParams_p->Xcode_InitParams.MaxOpen == 0) ||                 /* Cannot be 0 ! */
        (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1) ||          /* Device name length should be respected */
        ((((const char *) DeviceName)[0]) == '\0') ||
        ((((const char *) TranscoderInitParams_p->TargetDeviceName)[0]) == '\0')
       )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Software protection : the application cannot call a 2nd STVID_TranscoderInit()
    while the first one is not finished. It cannot either begin a STVID_TranscoderTerm function */
    stvid_EnterTranscoderCriticalSection(NULL);

    /* First time: initialise devices and units as empty */
    if (!(FirstTranscoderInitDone))
    {
        for (Index = 0; Index < STVID_MAX_DEVICE; Index++)
        {
            TranscoderDeviceArray[Index].DeviceName[0] = '\0';
            TranscoderDeviceArray[Index].DeviceData_p = NULL;
        }

        for (Index = 0; Index < STVID_MAX_UNIT; Index++)
        {
            stvid_TranscoderUnitArray[Index].UnitValidity = 0;
        }

        /* Process this only once */
        FirstTranscoderInitDone = TRUE;
    }

    /* Check if device already initialised and return error if so */
    if ((GetIndexOfTranscoderDeviceNamed(DeviceName) != INVALID_DEVICE_INDEX) && (ErrorCode == ST_NO_ERROR ))
    {
        /* Device name found */
        ErrorCode = ST_ERROR_ALREADY_INITIALIZED;
    }
    else
    {
        /* Look for a non-initialised device and return error if none is available */
        Index = 0;
        while ((TranscoderDeviceArray[Index].DeviceName[0] != '\0') && (Index < STVID_MAX_DEVICE))
        {
            Index++;
        }
        if (Index >= STVID_MAX_DEVICE)
        {
            /* All devices initialised */
            ErrorCode = ST_ERROR_NO_MEMORY;
        }
        else
        {
            /* API specific initialisations */
            ErrorCode = TranscoderInit(&TranscoderDeviceArray[Index], TranscoderInitParams_p, DeviceName);


            if (ErrorCode == ST_NO_ERROR)
            {
                strcpy(TranscoderDeviceArray[Index].DeviceName, DeviceName);
                TranscoderDeviceArray[Index].DeviceName[ST_MAX_DEVICE_NAME - 1] = '\0';
                TranscoderDeviceArray[Index].DeviceData_p->MaxOpen = TranscoderInitParams_p->MaxOpen;

                /* Initialise semaphore used to protect device data. */
                TranscoderDeviceArray[Index].DeviceControlLockingSemaphore_p =
                STOS_SemaphoreCreatePriority(TranscoderDeviceArray[Index].DeviceData_p->CPUPartition_p,1);
                STTBX_Report((STTBX_REPORT_LEVEL_INFO, "TranscodeDevice '%s' initialised", TranscoderDeviceArray[Index].DeviceName));
            }
        }
    }

    stvid_LeaveTranscoderCriticalSection(NULL);

    return(ErrorCode);

} /* STVID_TranscoderInit */

/*******************************************************************************
Name        : STVID_TranscoderTerm
Description : Terminate Encoder driver
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STVID_TranscoderTerm(const ST_DeviceName_t DeviceName,
        const STVID_TranscoderTermParams_t * const TermParams_p)
{
    TranscoderUnit_t *Unit_p;
    S32 DeviceIndex = INVALID_DEVICE_INDEX, UnitIndex;
    BOOL Found = FALSE;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    ST_Partition_t *      Partition_p ;

    /* Exit now if parameters are bad */
    if ((TermParams_p == NULL) ||                       /* There must be parameters ! */
        (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1) || /* Device name length should be respected */
        ((((const char *) DeviceName)[0]) == '\0')            /* Device name should not be empty */
        )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Software protection : the application cannot call a 2nd STVID_Term()
       while the first one is not finished. */
    stvid_EnterTranscoderCriticalSection(NULL);

    /* Check the init has been done prior to STVID_Term. */
    if (! FirstTranscoderInitDone)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STVID_Init should be "
                      "called prior to STVID_Term"));
        ErrorCode = ST_ERROR_UNKNOWN_DEVICE;
    }
    else
    {
        /* Check if device already initialised and return error if NOT so */
        DeviceIndex = GetIndexOfTranscoderDeviceNamed(DeviceName);
        if (DeviceIndex == INVALID_DEVICE_INDEX)
        {
            /* Device name not found */
            ErrorCode = ST_ERROR_UNKNOWN_DEVICE;
        }
        else
        {
            /* Check if there is still 'open' on this device */
/*            Found = FALSE;*/
            UnitIndex = 0;
            Unit_p = stvid_TranscoderUnitArray;
            while ((UnitIndex < STVID_MAX_UNIT) && (!Found))
            {
                Found = ((Unit_p->Device_p == &TranscoderDeviceArray[DeviceIndex]) && (Unit_p->UnitValidity == STVID_VALID_UNIT));
                Unit_p++;
                UnitIndex++;
            }

            if (Found)
            {
                if (TermParams_p->ForceTerminate)
                {
                    UnitIndex = 0;
                    Unit_p = stvid_TranscoderUnitArray;
                    while (UnitIndex < STVID_MAX_UNIT)
                    {
                        if ((Unit_p->Device_p == &TranscoderDeviceArray[DeviceIndex]) && (Unit_p->UnitValidity == STVID_VALID_UNIT))
                        {
                            /* Found an open instance: close it ! */
                            /* To do ?? */
                            ErrorCode = ST_NO_ERROR;

                            if (ErrorCode != ST_NO_ERROR)
                            {
                                /* If error: don't care, force close to force terminate... */
                                ErrorCode = ST_NO_ERROR;
                            }

                            /* Un-register opened handle whatever the error */
                            Unit_p->UnitValidity = 0;

                            STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Transcoder handle closed on device '%s'", Unit_p->Device_p->DeviceName));
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
                /* save  DeviceArray[DeviceIndex].DeviceData_p->CPUPartition_p
                 * in an auxilary variable to be used for STOS_SemaphoreDelete */
                Partition_p = TranscoderDeviceArray[DeviceIndex].DeviceData_p->CPUPartition_p;
                /* API specific terminations */
                ErrorCode = TranscoderTerm(&TranscoderDeviceArray[DeviceIndex], TermParams_p);

/* Don't leave instance semi-terminated: terminate as much as possible, to enable next Init() to work ! */
/*                if (ErrorCode == ST_NO_ERROR)*/
/*                {*/
                    /* Device found: desallocate memory, free device */
                    TranscoderDeviceArray[DeviceIndex].DeviceName[0] = '\0';

                    /* Take the section semaphore not to block an other task by destructing the semaphore */

                    stvid_EnterTranscoderCriticalSection(&TranscoderDeviceArray[DeviceIndex]);

                STOS_SemaphoreDelete(Partition_p,
                            TranscoderDeviceArray[DeviceIndex].DeviceControlLockingSemaphore_p);

                    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Device '%s' terminated", DeviceName));
/*                }*/
            } /* End terminate OK */
        } /* End valid device */
    } /* End FirstInitDone */

    /* End of the critical section. */
    stvid_LeaveTranscoderCriticalSection(NULL);

    return(ErrorCode);
} /* End of STVID_TranscoderTerm() function */

/*******************************************************************************
Name        : TranscoderOpen
Description :
Parameters  :
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t for the API InitTranscoder function
*******************************************************************************/
ST_ErrorCode_t STVID_TranscoderOpen(const ST_DeviceName_t DeviceName,
        const STVID_TranscoderOpenParams_t * const TranscoderOpenParams_p,
        STVID_TranscoderHandle_t * const TranscoderOpenHdl_p)
{
    S32 DeviceIndex = INVALID_DEVICE_INDEX, UnitIndex, Index;
    U32 OpenedUnitForThisInit;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if ((TranscoderOpenParams_p == NULL) ||                       /* There must be parameters ! */
        (TranscoderOpenHdl_p == NULL) ||                           /* Pointer to handle should be valid ! */
        (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1) || /* Device name length should be respected */
        ((((const char *) DeviceName)[0]) == '\0')
        )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Check the init has been done prior to STVID_OpenTranscoder. */
    if (!FirstTranscoderInitDone)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                      "STVID_InitTranscoder should be called prior to STVID_OpenTranscoder"));
        ErrorCode = ST_ERROR_UNKNOWN_DEVICE;
    }
    else
    {
        /* Check if device already initialised and return error if not so */
        DeviceIndex = GetIndexOfTranscoderDeviceNamed(DeviceName);
        if (DeviceIndex == INVALID_DEVICE_INDEX)
        {
            /* Device name not found */
            ErrorCode = ST_ERROR_UNKNOWN_DEVICE;
        }
        else
        {
            stvid_EnterTranscoderCriticalSection(&TranscoderDeviceArray[DeviceIndex]);
            /* Look for a free unit and check all opened units to check if MaxOpen is not reached */
            UnitIndex = STVID_MAX_UNIT;
            OpenedUnitForThisInit = 0;
            for (Index = 0; Index < STVID_MAX_UNIT; Index++)
            {
                if ((stvid_TranscoderUnitArray[Index].UnitValidity == STVID_VALID_UNIT) && (stvid_TranscoderUnitArray[Index].Device_p == &TranscoderDeviceArray[DeviceIndex]))
                {
                    OpenedUnitForThisInit ++;
                }
                if (stvid_TranscoderUnitArray[Index].UnitValidity != STVID_VALID_UNIT)
                {
                    /* Found a free handle structure */
                    UnitIndex = Index;
                }
            }
            if ((OpenedUnitForThisInit >= TranscoderDeviceArray[DeviceIndex].DeviceData_p->MaxOpen) || (UnitIndex >= STVID_MAX_UNIT))
            {
                /* None of the units is free or MaxOpen reached */
                ErrorCode = ST_ERROR_NO_FREE_HANDLES;
            }
            else
            {
                *TranscoderOpenHdl_p = (STVID_TranscoderHandle_t) &stvid_TranscoderUnitArray[UnitIndex];
                stvid_TranscoderUnitArray[UnitIndex].Device_p = &TranscoderDeviceArray[DeviceIndex];

                ErrorCode = TranscoderOpen(&stvid_TranscoderUnitArray[UnitIndex], TranscoderOpenParams_p);

                if (ErrorCode == ST_NO_ERROR)
                {
                    /* Register opened handle */
                    stvid_TranscoderUnitArray[UnitIndex].UnitValidity = STVID_VALID_UNIT;

                    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Video handle opened on device '%s'", DeviceName));
                }
            } /* End found unit unused */
            stvid_LeaveTranscoderCriticalSection(&TranscoderDeviceArray[DeviceIndex]);
        } /* End device valid */
    } /* End FirstInitDone */

    return(ErrorCode);
} /* End of STVID_TranscoderOpen() function */

/*******************************************************************************
Name        : STVID_TranscoderClose
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STVID_TranscoderClose(const STVID_TranscoderHandle_t Handle)
{
    TranscoderUnit_t * Unit_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Software protection : the application cannot call a 2nd STVID_Close()
     while the first one is not finished. */
    stvid_EnterTranscoderCriticalSection(NULL);

    /* Check if an init has been done. */
    if (!(FirstTranscoderInitDone))
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
            "STVID_TrancoderInit should be called prior to STVID_TranscoderClose"));
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        if (!IsValidTranscoderHandle(Handle))
        {
            ErrorCode = ST_ERROR_INVALID_HANDLE;
        }
        else
        {
            Unit_p = (TranscoderUnit_t *) Handle;

            stvid_EnterTranscoderCriticalSection(Unit_p->Device_p);

            /* API specific actions before closing */
            ErrorCode = TranscoderClose(Unit_p);

            /* Close only if no errors */
            if (ErrorCode == ST_NO_ERROR)
            {
                /* Un-register opened handle */
                Unit_p->UnitValidity = 0;

                STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Transcoder handle closed on device '%s'", Unit_p->Device_p->DeviceName));
            }
            stvid_LeaveTranscoderCriticalSection(Unit_p->Device_p);
        }
    }

    /* End of the critical section.                                            */
    stvid_LeaveTranscoderCriticalSection(NULL);

    return(ErrorCode);
} /* End of STVID_TranscoderClose() function */


/*******************************************************************************
Name        : STVID_TranscoderStart
Description : Start the video transcode
Parameters  : Transcoder handle
              video transcoder start parameters
Assumptions : The memory profile has been correctly set up by application
              (use of STVID_SetMemoryProfile).
Limitations :
Returns     : ST_NO_ERROR               if decode successfully started.
              ST_ERROR_INVALID_HANDLE   if the handle is not valid.
              ST_ERROR_BAD_PARAMETER    if at least one input parameter or
                    the memory profile is not valid.
              ST_ERROR_NO_MEMORY        if STAVMEM allocation failed
*******************************************************************************/
ST_ErrorCode_t STVID_TranscoderStart(const STVID_TranscoderHandle_t TranscoderHandle,
                             const STVID_TranscoderStartParams_t * const Params_p)
{
    TranscoderDevice_t *  Device_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if (Params_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!(IsValidTranscoderHandle(TranscoderHandle)))
    {
        /* Handle given is invalid */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Handle is valid: point to device */
    Device_p = ((TranscoderUnit_t *)TranscoderHandle)->Device_p;

    stvid_EnterTranscoderCriticalSection(Device_p);

    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = VIDTRAN_Start(Device_p->DeviceData_p->TranscoderHandle, Params_p);
    }

    stvid_LeaveTranscoderCriticalSection(Device_p);

    return(ErrorCode);
} /* STVID_TranscoderStart */

/*******************************************************************************
Name        : STVID_TranscoderStop
Description : Stop the video transcode
Parameters  : Transcoder handle
              video transcoder stop parameters
Assumptions : The memory profile has been correctly set up by application
              (use of STVID_SetMemoryProfile).
Limitations :
Returns     : ST_NO_ERROR               if decode successfully started.
              ST_ERROR_INVALID_HANDLE   if the handle is not valid.
              ST_ERROR_BAD_PARAMETER    if at least one input parameter or
                    the memory profile is not valid.
              ST_ERROR_NO_MEMORY        if STAVMEM allocation failed
*******************************************************************************/
ST_ErrorCode_t STVID_TranscoderStop(const STVID_TranscoderHandle_t TranscoderHandle,
                             const STVID_TranscoderStopParams_t * const StopParams_p)
{
    TranscoderDevice_t * Device_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if (StopParams_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!(IsValidTranscoderHandle(TranscoderHandle)))
    {
        /* Handle given is invalid */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Handle is valid: point to device */
    Device_p = ((TranscoderUnit_t *)TranscoderHandle)->Device_p;

    stvid_EnterTranscoderCriticalSection(Device_p);

    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = VIDTRAN_Stop(Device_p->DeviceData_p->TranscoderHandle, StopParams_p);
    }

    stvid_LeaveTranscoderCriticalSection(Device_p);

    return(ErrorCode);
} /* STVID_TranscoderStop */


/*******************************************************************************
Name        : STVID_TranscoderLink
Description : Links the video and the transcoder
Parameters  : Transcoder handle
              Video Handle
Limitations :
Returns     : ST_NO_ERROR               if decode successfully started.
              ST_ERROR_INVALID_HANDLE   if the handle is not valid.
              ST_ERROR_BAD_PARAMETER    if at least one input parameter or
                    the memory profile is not valid.
              ST_ERROR_NO_MEMORY        if STAVMEM allocation failed
*******************************************************************************/
ST_ErrorCode_t STVID_TranscoderLink(const STVID_TranscoderHandle_t TranscoderHandle, const STVID_TranscoderLinkParams_t * const TranscoderLinkParams_p)
{
    TranscoderDevice_t *            Device_p;
    ST_ErrorCode_t                  ErrorCode = ST_NO_ERROR;

    if (!(IsValidTranscoderHandle(TranscoderHandle)))
    {
        /* Handle given is invalid */
        return(ST_ERROR_INVALID_HANDLE);
    }

    if ((((const char *) TranscoderLinkParams_p->SourceDeviceName)[0]) == '\0')
    {
        /* DeviceName given is invalid */
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Handle is valid: point to device */
    Device_p = ((TranscoderUnit_t *)TranscoderHandle)->Device_p;

    stvid_EnterTranscoderCriticalSection(Device_p);

    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = VIDTRAN_Link(Device_p->DeviceData_p->TranscoderHandle, TranscoderLinkParams_p);
    }

    stvid_LeaveTranscoderCriticalSection(Device_p);

    return(ErrorCode);
}

/* End of STVID_TranscoderLink */

/*******************************************************************************
Name        : STVID_TranscoderUnlink
Description : Unlinks the video and the transccoder
Parameters  : Transcoder handle
Limitations :
Returns     : ST_NO_ERROR               if decode successfully started.
              ST_ERROR_INVALID_HANDLE   if the handle is not valid.
              ST_ERROR_BAD_PARAMETER    if at least one input parameter or
                    the memory profile is not valid.
              ST_ERROR_NO_MEMORY        if STAVMEM allocation failed
*******************************************************************************/
ST_ErrorCode_t STVID_TranscoderUnlink(const STVID_TranscoderHandle_t TranscoderHandle)
{
    TranscoderDevice_t * Device_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    if (!(IsValidTranscoderHandle(TranscoderHandle)))
    {
        /* Handle given is invalid */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Handle is valid: point to device */
    Device_p = ((TranscoderUnit_t *)TranscoderHandle)->Device_p;

    stvid_EnterTranscoderCriticalSection(Device_p);

    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = VIDTRAN_Unlink(Device_p->DeviceData_p->TranscoderHandle);
    }

    stvid_LeaveTranscoderCriticalSection(Device_p);

    return(ErrorCode);
}

/* End of STVID_TranscoderUnlink */

/*******************************************************************************
Name        : STVID_TranscoderSetParams
Description : Sets on the fly parameters for the transcoder
Parameters  : Transcoder handle
              Display Mode
Limitations :
Returns     : ST_NO_ERROR               Always
*******************************************************************************/
ST_ErrorCode_t STVID_TranscoderSetParams(const STVID_TranscoderHandle_t TranscoderHandle, const STVID_TranscoderSetParams_t * const SetParams_p)
{
    TranscoderDevice_t * Device_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    if (!(IsValidTranscoderHandle(TranscoderHandle)))
    {
        /* Handle given is invalid */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Handle is valid: point to device */
    Device_p = ((TranscoderUnit_t *)TranscoderHandle)->Device_p;

    if (Device_p->DeviceData_p->TranscoderHandleIsValid)
    {
        /* Disregard invalid rate control mode */
        switch (SetParams_p->CompressionParams.RateControlMode)
        {
            case STVID_RATE_CONTROL_MODE_NONE :
            case STVID_RATE_CONTROL_MODE_VBR :
            case STVID_RATE_CONTROL_MODE_CBR :
                break;

            default :
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                              "STVID_TranscoderSetParams() : Invalid rate control mode !"));
                return(ST_ERROR_BAD_PARAMETER);
        }

    }

    stvid_EnterTranscoderCriticalSection(Device_p);
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = VIDTRAN_SetParams(Device_p->DeviceData_p->TranscoderHandle, SetParams_p);
    }
    stvid_LeaveTranscoderCriticalSection(Device_p);

    return(ErrorCode);
}

/* End of STVID_TranscoderSetParams */


/*******************************************************************************
Name        : STVID_TranscoderSetMode
Description : Sets the mode for the transcoder
Parameters  : Transcoder handle
              Transcoder Mode
Limitations :
Returns     : ST_NO_ERROR               Always
*******************************************************************************/
ST_ErrorCode_t STVID_TranscoderSetMode(const STVID_TranscoderHandle_t TranscoderHandle, const STVID_TranscoderSetMode_t * const SetMode_p)
{
    TranscoderDevice_t * Device_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    if (!(IsValidTranscoderHandle(TranscoderHandle)))
    {
        /* Handle given is invalid */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Handle is valid: point to device */
    Device_p = ((TranscoderUnit_t *)TranscoderHandle)->Device_p;

    if (Device_p->DeviceData_p->TranscoderHandleIsValid)
    {
        /* Disregard invalid rate control mode */
        switch (*SetMode_p)
        {
            case STVID_ENCODER_MODE_SLAVE :
            case STVID_ENCODER_MODE_MASTER :
                break;

            default :
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                              "STVID_TranscoderSetMode() : Invalid transcoder mode !"));
                return(ST_ERROR_BAD_PARAMETER);
        }
    }

    stvid_EnterTranscoderCriticalSection(Device_p);
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = VIDTRAN_SetMode(Device_p->DeviceData_p->TranscoderHandle, SetMode_p);
    }
    stvid_LeaveTranscoderCriticalSection(Device_p);

    return(ErrorCode);
}

/* End of STVID_TranscoderSetMode */

/*******************************************************************************
Name        : STVID_TranscoderSetErrorRecoveryMode
Description : Defines the behavior of the transcoder driver regarding error recovery.
Parameters  : Transcoder handle
              Error Recovery Mode
Limitations :
Returns     : ST_NO_ERROR if no error
*******************************************************************************/
ST_ErrorCode_t STVID_TranscoderSetErrorRecoveryMode(const STVID_TranscoderHandle_t TranscoderHandle, const STVID_TranscoderErrorRecoveryMode_t Mode)
{
    TranscoderDevice_t * Device_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    if (!(IsValidTranscoderHandle(TranscoderHandle)))
    {
        /* Handle given is invalid */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Handle is valid: point to device */
    Device_p = ((TranscoderUnit_t *)TranscoderHandle)->Device_p;

    if (Device_p->DeviceData_p->TranscoderHandleIsValid)
    {
        switch(Mode)
        {
            case STVID_ENCODER_MODE_ERROR_RECOVERY_FULL :
            case STVID_ENCODER_MODE_ERROR_RECOVERY_NONE :
            break;

        default :
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STVID_TranscoderSetErrorRecoveryMode : Invalid mode"));
            ErrorCode = ST_ERROR_BAD_PARAMETER;
            break;
        }
    }

    stvid_EnterTranscoderCriticalSection(Device_p);
    ErrorCode = VIDTRAN_SetErrorRecoveryMode(Device_p->DeviceData_p->TranscoderHandle, Mode);
    stvid_LeaveTranscoderCriticalSection(Device_p);

    if (ErrorCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "VIDTRAN_SetErrorRecoveryMode failed"));
        ErrorCode = ST_ERROR_BAD_PARAMETER;
    }
    else
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Error recovery mode set to %d.", Mode));
    }

    return(ErrorCode);

} /* End of STVID_TranscoderSetErrorRecoveryMode() function */

/*******************************************************************************
Name        : STVID_TranscoderGetErrorRecoveryMode
Description : Retrieves the current error recovery mode for the specified transcoder.
Parameters  : Transcoder handle
              Error Recovery Mode
Limitations :
Returns     : ST_NO_ERROR if no error
*******************************************************************************/
ST_ErrorCode_t STVID_TranscoderGetErrorRecoveryMode(const STVID_TranscoderHandle_t TranscoderHandle, STVID_TranscoderErrorRecoveryMode_t * const Mode_p)
{
    TranscoderDevice_t * Device_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if (Mode_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!(IsValidTranscoderHandle(TranscoderHandle)))
    {
        /* Handle given is invalid */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Handle is valid: point to device */
    Device_p = ((TranscoderUnit_t *)TranscoderHandle)->Device_p;

    stvid_EnterTranscoderCriticalSection(Device_p);
    ErrorCode = VIDTRAN_GetErrorRecoveryMode(Device_p->DeviceData_p->TranscoderHandle, Mode_p);
    stvid_LeaveTranscoderCriticalSection(Device_p);

    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Error recovery mode is %d.", *Mode_p));

    return(ErrorCode);

} /* End of STVID_TranscoderGetErrorRecoveryMode() function */


#ifdef STVID_DEBUG_GET_STATISTICS
/*******************************************************************************
Name        : STVID_TranscoderGetStatistics
Description :
Parameters  : IN  : Transcoder Handle.
              OUT : Data Interface Parameters.
Assumptions :
Limitations :
Returns     : ST_NO_ERROR.
*******************************************************************************/
ST_ErrorCode_t STVID_TranscoderGetStatistics(const ST_DeviceName_t DeviceName, STVID_TranscoderStatistics_t  * const Statistics_p)
{
    S32 DeviceIndex = INVALID_DEVICE_INDEX;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    TranscoderDevice_t * TranscoderDevice_p;

    /* Exit now if parameters are bad */
    if ((Statistics_p == NULL) ||                       /* Pointer to capability structure should be valid ! */
        (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1) || /* Device name length should be respected */
        ((((const char *) DeviceName)[0]) == '\0')
        )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    memset (Statistics_p, 0x0, sizeof(STVID_TranscoderStatistics_t)); /* fill in data with 0x0 by default */

    stvid_EnterTranscoderCriticalSection(NULL);

    if (! FirstTranscoderInitDone)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STVID_EncoderInit should be called prior to STVID_EncoderGetStatistics !"));
        ErrorCode = ST_ERROR_UNKNOWN_DEVICE;
    }
    else
    {
        /* Check if device already initialised and return error if not so */
        DeviceIndex = GetIndexOfTranscoderDeviceNamed(DeviceName);
        if (DeviceIndex == INVALID_DEVICE_INDEX)
        {
            /* Device name not found */
            ErrorCode = ST_ERROR_UNKNOWN_DEVICE;
        }
        else
        {
            /* Fill statistics structure */
            TranscoderDevice_p = &(TranscoderDeviceArray[DeviceIndex]);
            stvid_EnterTranscoderCriticalSection(TranscoderDevice_p);

            /* Transcoder */
            if (TranscoderDevice_p->DeviceData_p->TranscoderHandleIsValid)
            {
                ErrorCode = VIDTRAN_GetStatistics(TranscoderDevice_p->DeviceData_p->TranscoderHandle, Statistics_p);
            }
            if (ErrorCode != ST_NO_ERROR)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "VIDTRAN_GetStatistics() : failed !"));
                ErrorCode = ST_ERROR_BAD_PARAMETER;
            }

            stvid_LeaveTranscoderCriticalSection(TranscoderDevice_p);
        } /* End is valid device */
    }

    stvid_LeaveTranscoderCriticalSection(NULL);

    return(ErrorCode);
} /* End of STVID_TranscoderGetStatistics() function */

/*******************************************************************************
Name        : STVID_TranscoderResetStatistics
Description :
Parameters  : IN  : Video Handle.
              OUT : Data Interface Parameters.
Assumptions :
Limitations :
Returns     : ST_NO_ERROR.
*******************************************************************************/
ST_ErrorCode_t STVID_TranscoderResetAllStatistics(const ST_DeviceName_t DeviceName, STVID_TranscoderStatistics_t * const Statistics_p)
{
    S32 DeviceIndex = INVALID_DEVICE_INDEX;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    TranscoderDevice_t * TranscoderDevice_p;

    /* Exit now if parameters are bad */
    if ((Statistics_p == NULL) ||                       /* Pointer to capability structure should be valid ! */
        (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1) || /* Device name length should be respected */
        ((((const char *) DeviceName)[0]) == '\0')
        )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    stvid_EnterTranscoderCriticalSection(NULL);

    if (! FirstTranscoderInitDone)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STVID_EncoderInit should be called prior to STVID_EncoderResetStatistics !"));
        ErrorCode = ST_ERROR_UNKNOWN_DEVICE;
    }
    else
    {
        /* Check if device already initialised and return error if not so */
        DeviceIndex = GetIndexOfTranscoderDeviceNamed(DeviceName);
        if (DeviceIndex == INVALID_DEVICE_INDEX)
        {
            /* Device name not found */
            ErrorCode = ST_ERROR_UNKNOWN_DEVICE;
        }
        else
        {
            /* Fill statistics structure */
            TranscoderDevice_p = &(TranscoderDeviceArray[DeviceIndex]);
            stvid_EnterTranscoderCriticalSection(TranscoderDevice_p);

            /* Producer */
            if (TranscoderDevice_p->DeviceData_p->TranscoderHandleIsValid)
            {
                ErrorCode = VIDTRAN_ResetAllStatistics(TranscoderDevice_p->DeviceData_p->TranscoderHandle, Statistics_p);
            }
            if (ErrorCode != ST_NO_ERROR)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "VIDTRAN_ResetStatistics() : failed !"));
                ErrorCode = ST_ERROR_BAD_PARAMETER;
            }
            stvid_LeaveTranscoderCriticalSection(TranscoderDevice_p);
        } /* End is valid device */
    }

    stvid_LeaveTranscoderCriticalSection(NULL);

    return(ErrorCode);
} /* End of STVID_TranscoderResetStatistics() function */
#endif /* STVID_DEBUG_GET_STATISTICS */

/*******************************************************************************
Name        : STVID_TranscoderInformReadAddress
Description : Inform the transcoder about progress in reading the output buffer
Parameters  : Transcoder handle
              Read pointer
Limitations :
Returns     : ST_NO_ERROR               Always
*******************************************************************************/
ST_ErrorCode_t STVID_TranscoderInformReadAddress(const STVID_Handle_t TranscoderHandle, void * const Read_p)
{
    TranscoderDevice_t * Device_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if (Read_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!(IsValidTranscoderHandle(TranscoderHandle)))
    {
        /* Handle given is invalid */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Handle is valid: point to device */
    Device_p = ((TranscoderUnit_t *)TranscoderHandle)->Device_p;

    stvid_EnterTranscoderCriticalSection(Device_p);

    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = VIDTRAN_InformReadAddress(Device_p->DeviceData_p->TranscoderHandle, Read_p);
    }

    stvid_LeaveTranscoderCriticalSection(Device_p);

    return(ErrorCode);

} /* End of STVID_TranscoderInformReadAddress() function */

/* end vid_xcode.c */


