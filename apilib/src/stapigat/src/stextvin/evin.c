/*******************************************************************************

File name   : evin.c

Description : ExtVin module standard API functions source file

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
28 Nov 2000        Created                                           BD
23 Nov 2001        Terminate as much as possible                     HS
*******************************************************************************/

/* Includes ----------------------------------------------------------------- */

#include <stdlib.h>
#include <string.h>

#include "stddefs.h"
#include "sttbx.h"

#include "stextvin.h"
#include "evin.h"
#include "extvi_pr.h"
#include "sti2c.h"
#include "stos.h"

/* Private Types ------------------------------------------------------------ */


/* Private Constants -------------------------------------------------------- */

#define INVALID_DEVICE_INDEX (-1)

/* Private Variables (static)------------------------------------------------ */

static stextvin_Device_t DeviceArray[STEXTVIN_MAX_DEVICE];
static semaphore_t * InstancesAccessControl_p;   /* Init/Open/Close/Term protection semaphore */
static BOOL FirstInitDone = FALSE;


/* Global Variables --------------------------------------------------------- */
stextvin_Unit_t STEXTVIN_UnitArray[STEXTVIN_MAX_UNIT];
STI2C_Handle_t  I2cHandle_SAA7114;
STI2C_Handle_t  I2cHandle_SAA7111;
STI2C_Handle_t  I2cHandle_TDA8752;
STI2C_Handle_t  I2cHandle_STV2310;


/* Private Macros ----------------------------------------------------------- */



/* Private Function prototypes ---------------------------------------------- */

static void EnterCriticalSection(void);
static void LeaveCriticalSection(void);
static S32 GetIndexOfDeviceNamed(const ST_DeviceName_t WantedName);
static ST_ErrorCode_t Init(stextvin_Device_t * const Device_p, const STEXTVIN_InitParams_t * const InitParams_p);
static ST_ErrorCode_t Open(stextvin_Unit_t * const Unit_p, const STEXTVIN_OpenParams_t * const OpenParams_p);
static ST_ErrorCode_t Close(stextvin_Unit_t * const Unit_p);
static ST_ErrorCode_t Term(stextvin_Device_t * const Device_p, const STEXTVIN_TermParams_t * const TermParams_p);


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

    STOS_InterruptLock();
    if (!InstancesAccessControlInitialized)
    {
        /* Initialise the Init/Open/Close/Term protection semaphore
          Caution: this semaphore is never deleted. (Not an issue) */
        InstancesAccessControl_p = STOS_SemaphoreCreateFifoTimeOut(NULL, 1);
        InstancesAccessControlInitialized = TRUE;
    }
    STOS_InterruptUnlock();

    /* Wait for the Init/Open/Close/Term protection semaphore */
    STOS_SemaphoreWait(InstancesAccessControl_p);
} /* End of EnterCriticalSection() function */


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
    STOS_SemaphoreSignal(InstancesAccessControl_p);
} /* End of LeaveCriticalSection() function */


/*******************************************************************************
Name        : Init
Description : API specific initialisations
Parameters  : pointer on device and initialisation parameters
Assumptions : pointer on device is not NULL
Limitations :
Returns     : ST_ErrorCode_t for the API Init function
*******************************************************************************/
static ST_ErrorCode_t Init(stextvin_Device_t * const Device_p, const STEXTVIN_InitParams_t * const InitParams_p)
{

    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STI2C_OpenParams_t I2cOpenParams;

    I2cOpenParams.I2cAddress        = InitParams_p->I2CAddress; /* address of remote I2C device */
    I2cOpenParams.AddressType       = STI2C_ADDRESS_7_BITS;
    I2cOpenParams.BusAccessTimeOut  = STI2C_TIMEOUT_INFINITY;

    switch (InitParams_p->DeviceType)
    {
        case STEXTVIN_SAA7114 :

        ErrorCode = STI2C_Open (InitParams_p->I2CDeviceName, &I2cOpenParams, &I2cHandle_SAA7114);
        if (ErrorCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Cannot open SAA7114 I2C device driver"));
            ErrorCode = STEXTVIN_ERROR_I2C;
        }
        else
        {
            ErrorCode = EXTVIN_SAA7114Init(I2cHandle_SAA7114);
        }
            break;

        case STEXTVIN_STV2310 :

        ErrorCode = STI2C_Open (InitParams_p->I2CDeviceName, &I2cOpenParams, &I2cHandle_STV2310);
        if (ErrorCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Cannot open STV2310 I2C device driver"));
            ErrorCode = STEXTVIN_ERROR_I2C;
        }
        else
        {
            ErrorCode = EXTVIN_STV2310Init(I2cHandle_STV2310);
        }
            break;

        case STEXTVIN_TDA8752 :
        ErrorCode = STI2C_Open (InitParams_p->I2CDeviceName, &I2cOpenParams, &I2cHandle_TDA8752);
        if (ErrorCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Cannot open TDA8752 I2C device driver"));
            ErrorCode = STEXTVIN_ERROR_I2C;
        }
        else
        {
            ErrorCode = EXTVIN_TDA8752Init(I2cHandle_TDA8752);
        }
            break;

        case STEXTVIN_DB331 :
        ErrorCode = STI2C_Open (InitParams_p->I2CDeviceName, &I2cOpenParams, &I2cHandle_SAA7111);
        if (ErrorCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Cannot open SAA7111 I2C device driver"));
            ErrorCode = STEXTVIN_ERROR_I2C;
        }
        else
        {
            ErrorCode = EXTVIN_SAA7111Init(I2cHandle_SAA7111);
        }
            break;

        default :
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Cannot Init the device: parameters invalid"));
            ErrorCode = ST_ERROR_BAD_PARAMETER;
            break;
    }

    Device_p->DeviceType = InitParams_p->DeviceType;
    strcpy((char *) Device_p->I2CDeviceName,(char *) InitParams_p->I2CDeviceName);
    Device_p->I2CAddress = InitParams_p->I2CAddress;

    return(ErrorCode);
} /* End of Init() function */


/*******************************************************************************
Name        : Open
Description : API specific actions just before opening
Parameters  : pointer on unit and open parameters
Assumptions : pointer on unit is not NULL
Limitations :
Returns     : ST_ErrorCode_t for the API Open function
*******************************************************************************/
static ST_ErrorCode_t Open(stextvin_Unit_t * const Unit_p, const STEXTVIN_OpenParams_t * const OpenParams_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    UNUSED_PARAMETER( Unit_p);
    UNUSED_PARAMETER( OpenParams_p);

    /* OpenParams exploitation and other actions */

    return(ErrorCode);
} /* End of Open() function */

/*******************************************************************************
Name        : Close
Description : API specific actions just before closing
Parameters  : pointer on unit
Assumptions : pointer on unit is not NULL
Limitations :
Returns     : ST_ErrorCode_t for the API Close function
              If not ST_NO_ERROR, the Handle would not be closed afterwards
*******************************************************************************/
static ST_ErrorCode_t Close(stextvin_Unit_t * const Unit_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    UNUSED_PARAMETER( Unit_p);

    /* Other actions */

    return(ErrorCode);
} /* End of Close() function */

/*******************************************************************************
Name        : Term
Description : API specific terminations
Parameters  : pointer on device and termination parameters
Assumptions : pointer on device is not NULL
Limitations :
Returns     : ST_ErrorCode_t for the API Term function
*******************************************************************************/
static ST_ErrorCode_t Term(stextvin_Device_t * const Device_p, const STEXTVIN_TermParams_t * const TermParams_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    UNUSED_PARAMETER( TermParams_p);

    switch (Device_p->DeviceType)
    {
        case STEXTVIN_SAA7114 :
            ErrorCode = STI2C_Close (I2cHandle_SAA7114);
            if (ErrorCode != ST_NO_ERROR)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Cannot Close SAA7114 I2C device driver"));
                ErrorCode = STEXTVIN_ERROR_I2C;
            }
            break;

        case STEXTVIN_STV2310 :
            ErrorCode = STI2C_Close (I2cHandle_STV2310);
            if (ErrorCode != ST_NO_ERROR)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Cannot Close STV2310 I2C device driver"));
                ErrorCode = STEXTVIN_ERROR_I2C;
            }
            break;

        case STEXTVIN_TDA8752 :
            ErrorCode = STI2C_Close (I2cHandle_TDA8752);
            if (ErrorCode != ST_NO_ERROR)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Cannot close TDA8752 I2C device driver"));
                ErrorCode = STEXTVIN_ERROR_I2C;
            }
            break;

        case STEXTVIN_DB331 :
            ErrorCode = STI2C_Close (I2cHandle_SAA7111);
            if (ErrorCode != ST_NO_ERROR)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Cannot close SAA7111 I2C device driver"));
                ErrorCode = STEXTVIN_ERROR_I2C;
            }
            break;

        default :
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "The specified device has never been initialized"));
            ErrorCode = ST_ERROR_UNKNOWN_DEVICE;
            break;
    }
    return(ErrorCode);
} /* End of Term() function */



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
    } while ((WantedIndex == INVALID_DEVICE_INDEX) && (Index < STEXTVIN_MAX_DEVICE));

    return(WantedIndex);
} /* End of GetIndexOfDeviceNamed() function */


/*
--------------------------------------------------------------------------------
Get revision of EXTVIN API
--------------------------------------------------------------------------------
*/
ST_Revision_t STEXTVIN_GetRevision(void)
{
    static const char Revision[] = "STEXTVIN-REL_1.0.0";

    /* Revision string format: STEXTVIN-REL_x.x.x
                                 /       \ \ \__patch release number  }
                            API name      \ \__minor release number  } revision number
                                           \__major release number  }
    */

    return((ST_Revision_t) Revision);
} /* End of STEXTVIN_GetRevision() function */


/*
--------------------------------------------------------------------------------
Get capabilities of EXTVIN API
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STEXTVIN_GetCapability(const ST_DeviceName_t DeviceName, STEXTVIN_Capability_t * const Capability_p)
{
    S32 DeviceIndex = INVALID_DEVICE_INDEX;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if ((Capability_p == NULL) ||                       /* Pointer to capability structure should be valid ! */
        (strlen(DeviceName) > (ST_MAX_DEVICE_NAME - 1)) || /* Device name length should be respected */
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
            /* Fill capability structure */

            /* Eventually not supported */
            ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
        }
    }

    LeaveCriticalSection();

    return(ErrorCode);
} /* End of STEXTVIN_GetCapability() function */


/*
--------------------------------------------------------------------------------
Initialise EXTVIN driver
(Standard instances management. Driver specific implementations should be put in Init() function.)
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STEXTVIN_Init(const ST_DeviceName_t DeviceName, const STEXTVIN_InitParams_t * const InitParams_p)
{
    S32 Index = 0;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if ((InitParams_p == NULL) ||                       /* There must be parameters */
        (InitParams_p->MaxOpen > STEXTVIN_MAX_OPEN) ||     /* No more than MAX_OPEN open handles supported */
        (strlen(DeviceName) > (ST_MAX_DEVICE_NAME - 1)) || /* Device name length should be respected */
        ((((const char *) DeviceName)[0]) == '\0')            /* Device name should not be empty */
       )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    EnterCriticalSection();

    /* First time: initialise devices and units as empty */
    if (!FirstInitDone)
    {
        for (Index = 0; Index < STEXTVIN_MAX_DEVICE; Index++)
        {
            DeviceArray[Index].DeviceName[0] = '\0';
        }

        for (Index = 0; Index < STEXTVIN_MAX_UNIT; Index++)
        {
            STEXTVIN_UnitArray[Index].UnitValidity = 0;
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
        while ((DeviceArray[Index].DeviceName[0] != '\0') && (Index < STEXTVIN_MAX_DEVICE))
        {
            Index++;
        }
        if (Index >= STEXTVIN_MAX_DEVICE)
        {
            /* All devices initialised */
            ErrorCode = ST_ERROR_NO_MEMORY;
        }
        else
        {
            /* API specific initialisations */
            ErrorCode = Init(&DeviceArray[Index], InitParams_p);

            if (ErrorCode == ST_NO_ERROR)
            {
                /* Device available and successfully initialised: register device name */
                strcpy(DeviceArray[Index].DeviceName, DeviceName);
                DeviceArray[Index].DeviceName[ST_MAX_DEVICE_NAME - 1] = '\0';
                DeviceArray[Index].MaxOpen = InitParams_p->MaxOpen;
                STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Device '%s' initialised", DeviceArray[Index].DeviceName));
            }
        } /* End exists device not initialised */
    } /* End device not already initialised */

    LeaveCriticalSection();

    return(ErrorCode);
} /* End of STEXTVIN_Init() function */


/*
--------------------------------------------------------------------------------
open ...
(Standard instances management. Driver specific implementations should be put in Open() function.)
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STEXTVIN_Open(const ST_DeviceName_t DeviceName, const STEXTVIN_OpenParams_t * const OpenParams_p, STEXTVIN_Handle_t *Handle_p)
{
    S32 DeviceIndex = INVALID_DEVICE_INDEX, UnitIndex, Index;
    U32 OpenedUnitForThisInit;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if ((OpenParams_p == NULL) ||                       /* There must be parameters ! */
        (Handle_p == NULL) ||                           /* Pointer to handle should be valid ! */
        (strlen(DeviceName) > (ST_MAX_DEVICE_NAME - 1)) || /* Device name length should be respected */
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
            /* Look for a free unit and check all opened units to check if MaxOpen is not reached */
            UnitIndex = STEXTVIN_MAX_UNIT;
            OpenedUnitForThisInit = 0;
            for (Index = 0; Index < STEXTVIN_MAX_UNIT; Index++)
            {
                if ((STEXTVIN_UnitArray[Index].UnitValidity == STEXTVIN_VALID_UNIT) && (STEXTVIN_UnitArray[Index].Device_p == &DeviceArray[DeviceIndex]))
                {
                    OpenedUnitForThisInit ++;
                }
                if (STEXTVIN_UnitArray[Index].UnitValidity != STEXTVIN_VALID_UNIT)
                {
                    /* Found a free handle structure */
                    UnitIndex = Index;
                }
            }
            if ((OpenedUnitForThisInit >= DeviceArray[DeviceIndex].MaxOpen) || (UnitIndex >= STEXTVIN_MAX_UNIT))
            {
                /* None of the units is free or MaxOpen reached */
                ErrorCode = ST_ERROR_NO_FREE_HANDLES;
            }
            else
            {
                *Handle_p = (STEXTVIN_Handle_t) &STEXTVIN_UnitArray[UnitIndex];
                STEXTVIN_UnitArray[UnitIndex].Device_p = &DeviceArray[DeviceIndex];

                /* API specific actions after opening */
                ErrorCode = Open(&STEXTVIN_UnitArray[UnitIndex], OpenParams_p);

                if (ErrorCode == ST_NO_ERROR)
                {
                    /* Register opened handle */
                    STEXTVIN_UnitArray[UnitIndex].UnitValidity = STEXTVIN_VALID_UNIT;

                    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Handle opened on device '%s'", DeviceName));
                }
            } /* End found unit unused */
        } /* End device valid */
    } /* End FirstInitDone */

    LeaveCriticalSection();

    return(ErrorCode);
} /* End fo STEXTVIN_Open() function */


/*
--------------------------------------------------------------------------------
Close ...
(Standard instances management. Driver specific implementations should be put in Close() function.)
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STEXTVIN_Close(STEXTVIN_Handle_t Handle)
{
    stextvin_Unit_t *Unit_p;
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
            Unit_p = (stextvin_Unit_t *) Handle;

            /* API specific actions before closing */
            ErrorCode = Close(Unit_p);

            /* Close only if no errors */
            if (ErrorCode == ST_NO_ERROR)
            {
                /* Un-register opened handle */
                Unit_p->UnitValidity = 0;

                STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Handle closed on device '%s'", Unit_p->Device_p->DeviceName));
            }
        } /* End handle valid */
    } /* End FirstInitDone */

    LeaveCriticalSection();

    return(ErrorCode);
} /* End of STEXTVIN_Close() function */


/*
--------------------------------------------------------------------------------
Terminate EXTVIN driver
(Standard instances management. Driver specific implementations should be put in Term() function.)
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STEXTVIN_Term(const ST_DeviceName_t DeviceName, const STEXTVIN_TermParams_t *const TermParams_p)
{
    stextvin_Unit_t *Unit_p;
    S32 DeviceIndex = INVALID_DEVICE_INDEX, UnitIndex;
    BOOL Found = FALSE;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if ((TermParams_p == NULL) ||                       /* There must be parameters ! */
        (strlen(DeviceName) > (ST_MAX_DEVICE_NAME - 1)) || /* Device name length should be respected */
        ((((const char *) DeviceName)[0]) == '\0')            /* Device name should not be empty */
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
/*            Found = FALSE;*/
            UnitIndex = 0;
            Unit_p = STEXTVIN_UnitArray;
            while ((UnitIndex < STEXTVIN_MAX_UNIT) && (!Found))
            {
                Found = ((Unit_p->Device_p == &DeviceArray[DeviceIndex]) && (Unit_p->UnitValidity == STEXTVIN_VALID_UNIT));
                Unit_p++;
                UnitIndex++;
            }

            if (Found)
            {
                if (TermParams_p->ForceTerminate)
                {
                    UnitIndex = 0;
                    Unit_p = STEXTVIN_UnitArray;
                    while (UnitIndex < STEXTVIN_MAX_UNIT)
                    {
                        if ((Unit_p->Device_p == &DeviceArray[DeviceIndex]) && (Unit_p->UnitValidity == STEXTVIN_VALID_UNIT))
                        {
                            /* Found an open instance: close it ! */
                            ErrorCode = Close(Unit_p);
                            if (ErrorCode != ST_NO_ERROR)
                            {
                                /* If error: don't care, force close to force terminate... */
                                ErrorCode = ST_NO_ERROR;
                            }
                            /* Un-register opened handle whatever the error */
                            Unit_p->UnitValidity = 0;

                            STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Handle closed on device '%s'", Unit_p->Device_p->DeviceName));
                        }
                        Unit_p++;
                        UnitIndex++;
                    }
                } /* End ForceTerminate: closed all opened handles */
                else
                {
                    /* Can't term if there are handles still opened, and ForceTerminate not set */
                    ErrorCode = ST_ERROR_OPEN_HANDLE;
                }
            } /* End found handle not closed */

            /* Terminate if OK */
            if (ErrorCode == ST_NO_ERROR)
            {
                /* API specific terminations */
                ErrorCode = Term(&DeviceArray[DeviceIndex], TermParams_p);
                /* Don't leave instance semi-terminated: terminate as much as possible */
                /* free device */
                DeviceArray[DeviceIndex].DeviceName[0] = '\0';

                STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Device '%s' terminated", DeviceName));
            } /* End terminate OK */
        } /* End valid device */
    } /* End FirstInitDone */

    LeaveCriticalSection();

    return(ErrorCode);
} /* End of STEXTVIN_Term() function */


/* End of evin.c */
