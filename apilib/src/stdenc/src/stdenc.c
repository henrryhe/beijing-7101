/**********************************************************************************

File Name   : stdenc.c

Description : API DENC driver module.

COPYRIGHT (C) STMicroelectronics 2003

Date               Modification                                             Name
----               ------------                                             ----
06 Jun 2000        Created                                                  JG
29 Aou 2000        Multi-init ..., hal for STV0119 and STI55xx,70xx         JG
15 Nov 2000        improve error tracability                                HSdLM
06 Feb 2001        Reorganize HAL for providing mutual exclusion on         HSdLM
 *                 registers access
07 Mar 2001        use new enum STDENC_Mode_t members                       HSdLM
 *                 use structure STDENC_EncodingMode_t
 *                 use new STDENC_Capability_t members
 *                 use new STDENC_<Set/Get>EncodingMode() 2nd arg
 *                 STDENC_Init() returns 'bad parameter' if bad DeviceType
 *                 move stdencra functions to denc_reg.c
 *                 use Dencra_xxx fct to get fonction table addresses
22 Mar 2001        check STDENC_SetEncodingMode pointer argument validity   HSdLM
22 Jun 2001        Move STDENC_<Set/Get>EncodingMode to new file            HSdLM
 *                 denc_drv.c. Update capabilities (new variables).
12 Jul 2001        Patch to fix DDTS GNBvd08283                             HSdLM
02 Aou 2001        Term() / ForceTerminate : Un-register opened handle      HSdLM
 *                 whatever the Close() error.
30 Aou 2001        Remove dependency upon STI2C if not needed, to support   HSdLM
 *                 ST40GX1
16 Apr 2002        Add support for STi7020                                  HSdLM
26 Jul 2002        Modify WA_GNBvd11019                                     BS
30 Oct 2002        STi7020 DENC input is 4:4:4                              HSdLM
16 Apr 2003        Disable trap filter by default                           HSdLM
27 Jun 2003        Add support for new cell version 12 (auxilary encoder)   HSdLM
27 Feb 2004        Add support for STi5100                                  MH
05 Jul 2004        Add 7710/mb391 support                                   TA
13 Sep 2004        Add support for ST40/OS21                                MH
***********************************************************************************/

/* Includes ----------------------------------------------------------------- */

#if !defined(ST_OSLINUX)
/* Include system files only if not in Kernel mode */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#endif
#include "sttbx.h"

#include "stddefs.h"

#ifdef ST_OSWINCE		
#include "stdevice.h"		/* Added for ST7100_VOS_ADDRESS_WIDTH definition. */
#endif	/* ST_OSWINCE */

#include "denc_rev.h"
#include "stdenc.h"
#include "denc_rev.h"
#include "denc_drv.h"
#include "denc_hal.h"
#include "reg_rbus.h"

#ifdef STDENC_I2C
#include "reg_i2c.h"
#endif /* #ifdef STDENC_I2C */

/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */

#define STDENC_MAX_UNIT    (STDENC_MAX_OPEN * STDENC_MAX_DEVICE)

#define INVALID_DEVICE_INDEX (-1)

#if defined(ST_OSLINUX)
#define STDENC_MAPPING_WIDTH        0x400   /* Maps DENC registers area only */
#endif /** ST_OSLINUX **/

/* Private Variables (static)------------------------------------------------ */
static DENCDevice_t DeviceArray[STDENC_MAX_DEVICE];
static DENC_t UnitArray[STDENC_MAX_UNIT];
static semaphore_t *InstancesAccessControl_p;   /* Init/Open/Close/Term protection semaphore */
static BOOL FirstInitDone = FALSE;

/* Global Variables --------------------------------------------------------- */

#ifdef DEBUG
DENCDevice_t * stdenc_DbgDev = DeviceArray;
DENC_t * stdenc_DbgPtr = UnitArray;
#endif

/* Private Macros ----------------------------------------------------------- */

/* Passing a (STDENC_Handle_t), returns TRUE if the handle is valid, FALSE otherwise */
#define IsValidHandle(Handle) ((((DENC_t *) (Handle)) >= UnitArray) &&                    \
                               (((DENC_t *) (Handle)) < (UnitArray + STDENC_MAX_UNIT)) &&  \
                               (((DENC_t *) (Handle))->UnitValidity == STDENC_VALID_UNIT))

/* Private Function prototypes ---------------------------------------------- */

static void EnterCriticalSection(void);
static void LeaveCriticalSection(void);
static S32 GetIndexOfDeviceNamed(const ST_DeviceName_t WantedName);
static ST_ErrorCode_t Init(DENCDevice_t * const Device_p, const STDENC_InitParams_t * const InitParams_p);
static ST_ErrorCode_t Open(DENC_t * const Unit_p, const STDENC_OpenParams_t * const OpenParams_p);
static ST_ErrorCode_t Close(DENC_t * const Unit_p);
static ST_ErrorCode_t Term(DENCDevice_t * const Device_p, const STDENC_TermParams_t * const TermParams_p);

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


    STOS_TaskLock();

    if (!InstancesAccessControlInitialized)
    {
        InstancesAccessControlInitialized = TRUE;
        /* Initialise the Init/Open/Close/Term protection semaphore
          Caution: this semaphore is never deleted. (Not an issue) */
        InstancesAccessControl_p = STOS_SemaphoreCreateFifo(NULL,1);

    }

    STOS_TaskUnlock();


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
    } while ((WantedIndex == INVALID_DEVICE_INDEX) && (Index < STDENC_MAX_DEVICE));

    return(WantedIndex);
} /* End of GetIndexOfDeviceNamed() function */


/*******************************************************************************
Name        : Init
Description : API specific initialisations
Parameters  : pointer on device and initialisation parameters
Assumptions : pointer on device is not NULL
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_BAD_PARAMETER or STDENC_ERROR_I2C
*******************************************************************************/
static ST_ErrorCode_t Init(DENCDevice_t * const Device_p, const STDENC_InitParams_t * const InitParams_p)
{
#ifdef STDENC_I2C
    const STDENC_I2CAccess_t * I2CAccess_p;
#endif /* #ifdef STDENC_I2C */
    const STDENC_EMIAccess_t * RBUSAccess_p;
    ST_ErrorCode_t             ErrorCode = ST_NO_ERROR;

	Device_p->IsAuxEncoderOn=FALSE;
	Device_p->IsExternal=FALSE;
    if (  (InitParams_p->AccessType == STDENC_ACCESS_TYPE_EMI)
        &&(InitParams_p->STDENC_Access.EMI.BaseAddress_p == NULL)
       )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    switch (InitParams_p->DeviceType)
    {
        case STDENC_DEVICE_TYPE_DENC :
        case STDENC_DEVICE_TYPE_4629 :
        case STDENC_DEVICE_TYPE_V13  : /* no break */
            Device_p->RegShift = 0; /* registers are spaced each 2^0=1 bytes */
            /* RegShift will be updated later, depending on access width 8 or 32 bits */
            break;

        case STDENC_DEVICE_TYPE_7015 : /* no break */
        case STDENC_DEVICE_TYPE_7020 : /* no break */
            Device_p->RegShift = 2; /* registers are spaced each 2^2=4 bytes */
            break;
        case STDENC_DEVICE_TYPE_GX1 :
            Device_p->RegShift = 3; /* registers are spaced each 2^3=8 bytes */
            break;
        default :
            return(ST_ERROR_BAD_PARAMETER);
            break;
    }
    Device_p->DeviceType = InitParams_p->DeviceType;

     /* init by default : */
    Device_p->EncodingMode.Mode     = STDENC_MODE_NONE;
    Device_p->SECAMSquarePixel      = FALSE;
    Device_p->LumaTrapFilter        = FALSE;
    Device_p->LumaTrapFilterAux     = FALSE;
    Device_p->BlackLevelPedestal    = FALSE;
    Device_p->BlackLevelPedestalAux = FALSE;

#ifdef WA_GNBvd11019
    Device_p->RamToDefault = FALSE;
#endif /* WA_GNBvd11019 */

    /* Setup mutex semaphore */
    Device_p->RegAccessMutex_p = STOS_SemaphoreCreateFifo(NULL,1);
    /* Setup accessing encoding mode data semaphore */
    Device_p->ModeCtrlAccess_p = STOS_SemaphoreCreateFifo(NULL,1);

    switch (InitParams_p->AccessType)
    {
        case STDENC_ACCESS_TYPE_EMI :
            RBUSAccess_p = &(InitParams_p->STDENC_Access.EMI);

            switch(RBUSAccess_p->Width)
            {
                case STDENC_EMI_ACCESS_WIDTH_8_BITS:
                    Device_p->FunctionsTable_p = stdenc_GetRBUSFunctionTableAddress();
                    break;
                case STDENC_EMI_ACCESS_WIDTH_32_BITS:
                    /* reminder: for STDENC_DEVICE_TYPE_GX1, RegShift = 3 */
                    if (Device_p->RegShift == 0)
                    {
                        Device_p->RegShift = 2; /* registers are spaced each 2^2=4 bytes */
                    }
                    Device_p->FunctionsTable_p = stdenc_GetRBUSShiftedFunctionTableAddress();
                    break;
                default  :
                    ErrorCode = ST_ERROR_BAD_PARAMETER;
                    break;
            }

            if (ErrorCode == ST_NO_ERROR)
            {
                Device_p->BaseAddress_p = (void *) (((U8 *) RBUSAccess_p->DeviceBaseAddress_p) + ((U32)RBUSAccess_p->BaseAddress_p));



#if !defined(ST_OSLINUX) /* it is not possible to access hardware at this point */
#ifdef ST_OSWINCE
			/* translate BaseAddress_p to virtual memory */
            Device_p->BaseAddress_p = MapPhysicalToVirtual(Device_p->BaseAddress_p, VOS_ADDRESS_WIDTH);
			if (Device_p->BaseAddress_p == NULL)
			{
				WCE_ERROR("MapPhysicalToVirtual(Device_p->BaseAddress_p)");
				ErrorCode = ST_ERROR_BAD_PARAMETER;
				break;
			}
#endif /* ST_OSWINCE */

                if ( (InitParams_p->DeviceType == STDENC_DEVICE_TYPE_7015) || (InitParams_p->DeviceType == STDENC_DEVICE_TYPE_7020) )
                {
                    stdenc_HALSetOn(Device_p);
                }
#endif
            }
            /* Now DENC register accesses are available, if RegShift as been set above */
            break;
#ifdef STDENC_I2C
        case STDENC_ACCESS_TYPE_I2C :
            Device_p->FunctionsTable_p = stdenc_GetI2CFunctionTableAddress();
            I2CAccess_p = &(InitParams_p->STDENC_Access.I2C);

#if !defined(ST_OSLINUX) /* it is not possible to access hardware at this point */
            ErrorCode = stdenc_HALInitI2CConnection(Device_p, I2CAccess_p);
#endif
            break;
#endif /* #ifdef STDENC_I2C */
        default:
            ErrorCode = ST_ERROR_BAD_PARAMETER;
            break;
    }

#ifdef ST_OSLINUX

	if ( ErrorCode == ST_NO_ERROR)
	{
         Device_p->DencMappedWidth = (U32)(STDENC_MAPPING_WIDTH);
         Device_p->DencMappedBaseAddress_p = STLINUX_MapRegion( (void *) (Device_p->BaseAddress_p), (U32) (Device_p->DencMappedWidth), "DENC");

        if ( Device_p->DencMappedBaseAddress_p )
	    {
            /** affecting DENC Base Adress with mapped Base Adress **/
            Device_p->BaseAddress_p = Device_p->DencMappedBaseAddress_p;
	    }
        else
        {
             ErrorCode =  ST_ERROR_BAD_PARAMETER;
        }

    }
#endif  /* ST_OSLINUX */

    if ( ErrorCode == ST_NO_ERROR)
    {
        Device_p->AccessType = InitParams_p->AccessType;
        /* set DencVersion */
        ErrorCode = stdenc_HALVersion(Device_p);
    }

    if ( ErrorCode == ST_NO_ERROR)
    {
        switch (InitParams_p->DeviceType)
        {

			case STDENC_DEVICE_TYPE_4629 :
				Device_p->YCbCr444Input  = TRUE;
                Device_p->ChromaDelayAux = -1;
				Device_p->IsExternal=TRUE;
				break;
			case STDENC_DEVICE_TYPE_DENC :
            case STDENC_DEVICE_TYPE_V13 :
                if (Device_p->DencVersion < 10)
                {
                    Device_p->ChromaDelay = 0; /* default for PAL/NTSC in 4:2:2 format on CVBS */
                }
                else
                {
                    Device_p->ChromaDelay = -1; /* default for PAL/NTSC in 4:2:2 format on CVBS */
                }
                if (Device_p->DencVersion < 12)
                {
                    Device_p->YCbCr444Input  = FALSE;
                }
                else
                {
                    Device_p->YCbCr444Input  = TRUE;
                    Device_p->ChromaDelayAux = -1;
					Device_p->IsAuxEncoderOn = TRUE;
                }
                break;
            case STDENC_DEVICE_TYPE_7015 :
                Device_p->ChromaDelay = -3; /* SwRWA of DDTS GNBvd07130, topic 4.3 of STi7015 Cut 1.1 validation report */
                Device_p->YCbCr444Input      = FALSE;
                break;
            case STDENC_DEVICE_TYPE_7020 :
                Device_p->ChromaDelay = -3; /* SwRWA of DDTS GNBvd07130, topic 4.3 of STi7015 Cut 1.1 validation report */
                Device_p->YCbCr444Input      = TRUE;
                break;
            case STDENC_DEVICE_TYPE_GX1 :
                Device_p->ChromaDelay = -1; /* default for PAL/NTSC in 4:2:2 format on CVBS */
                Device_p->YCbCr444Input      = TRUE;
                break;
           default : /* already checked before in this routine */
                break;
        }
        ErrorCode = stdenc_HALSetDencId(Device_p);
        if ( ErrorCode == ST_NO_ERROR)
        {
            ErrorCode = stdenc_HALSetEncodingMode(Device_p);
        }
    }


    if ( ErrorCode != ST_NO_ERROR)
    {
        STOS_SemaphoreDelete(NULL,Device_p->RegAccessMutex_p);
        STOS_SemaphoreDelete(NULL,Device_p->ModeCtrlAccess_p);

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "HALDENC Init Failed !"));
    }
    else
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "STDenc version %d", Device_p->DencVersion));
    }
    return(ErrorCode);
} /* end of Init() */

/*******************************************************************************
Name        : Open
Description : API specific actions just before opening
Parameters  : pointer on unit and open parameters
Assumptions : pointer on unit is not NULL
Limitations :
Returns     : ST_NO_ERROR
*******************************************************************************/
static ST_ErrorCode_t Open(DENC_t * const Unit_p, const STDENC_OpenParams_t * const OpenParams_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    UNUSED_PARAMETER(Unit_p);
    UNUSED_PARAMETER(OpenParams_p);

    /* Other actions */

    return(ErrorCode);

} /* end of Open() */

/*******************************************************************************
Name        : Close
Description : API specific actions just before closing
Parameters  : pointer on unit
Assumptions : pointer on unit is not NULL
Limitations :
Returns     : ST_NO_ERROR
              If not ST_NO_ERROR, the Handle would not be closed afterwards
*******************************************************************************/
static ST_ErrorCode_t Close(DENC_t * const Unit_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Wait pending configuration finish before closing instance */
    /* then release mutex for other instances */
    STOS_SemaphoreWait(Unit_p->Device_p->ModeCtrlAccess_p);
    STOS_SemaphoreSignal(Unit_p->Device_p->ModeCtrlAccess_p);

    return(ErrorCode);
} /* end of Close() */


/*******************************************************************************
Name        : Term
Description : API specific terminations
Parameters  : pointer on device and termination parameters
Assumptions : pointer on device is not NULL
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_BAD_PARAMETER, STDENC_ERROR_I2C
*******************************************************************************/
static ST_ErrorCode_t Term(DENCDevice_t * const Device_p, const STDENC_TermParams_t * const TermParams_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    UNUSED_PARAMETER(TermParams_p);
    /* Other actions */
    if ( (Device_p->DeviceType == STDENC_DEVICE_TYPE_7015) || (Device_p->DeviceType == STDENC_DEVICE_TYPE_7020) )
    {
        stdenc_HALSetOff(Device_p);
    }
#ifdef STDENC_I2C
    if (Device_p->AccessType == STDENC_ACCESS_TYPE_I2C)
    {
        ErrorCode = stdenc_HALTermI2CConnection(Device_p);
    }
#endif /* #ifdef STDENC_I2C */

    STOS_SemaphoreDelete(NULL,Device_p->RegAccessMutex_p);
    STOS_SemaphoreDelete(NULL,Device_p->ModeCtrlAccess_p);

#ifdef ST_OSWINCE
    /* unmap the address range */
    if (Device_p->BaseAddress_p != NULL)
        UnmapPhysicalToVirtual(Device_p->BaseAddress_p);
#endif /* ST_OSWINCE */

#ifdef ST_OSLINUX
    /** DENC region **/
    STLINUX_UnmapRegion(Device_p->DencMappedBaseAddress_p, (U32) (Device_p->DencMappedWidth));

#endif

    return(ErrorCode);
} /* end of Term() */


/*
--------------------------------------------------------------------------------
Get revision of DENC API
--------------------------------------------------------------------------------
*/
ST_Revision_t STDENC_GetRevision(void)
{
    /* Revision string format: STXXX-REL_x.x.x
                                 /       \ \ \__patch release number  }
                            API name      \ \__minor release number  } revision number
                                           \__major release number  }
    */

    return(DENC_Revision);
} /* End of STDENC_GetRevision() function */


/*
--------------------------------------------------------------------------------
Get capabilities of DENC API
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STDENC_GetCapability(const ST_DeviceName_t DeviceName, STDENC_Capability_t * const Capability_p)
{
    S32 DeviceIndex = INVALID_DEVICE_INDEX;
    DENCDevice_t * Device_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if ((Capability_p == NULL) ||                       /* Pointer to capability structure should be valid ! */
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
            Device_p = (DENCDevice_t * )&DeviceArray[DeviceIndex];
            /* Fill capability structure */
            Capability_p->Modes =   (1<<STDENC_MODE_NTSCM)
                                  | (1<<STDENC_MODE_NTSCM_J)
                                  | (1<<STDENC_MODE_NTSCM_443)
                                  | (1<<STDENC_MODE_PALBDGHI)
                                  | (1<<STDENC_MODE_PALM)
                                  | (1<<STDENC_MODE_PALN)
                                  | (1<<STDENC_MODE_PALN_C);
            Capability_p->Interlaced =   (1<<STDENC_MODE_NTSCM)
                                       | (1<<STDENC_MODE_NTSCM_J)
                                       | (1<<STDENC_MODE_NTSCM_443)
                                       | (1<<STDENC_MODE_PALBDGHI)
                                       | (1<<STDENC_MODE_PALM)
                                       | (1<<STDENC_MODE_PALN)
                                       | (1<<STDENC_MODE_PALN_C);
            Capability_p->FieldRate60Hz =   (1<<STDENC_MODE_NTSCM)
                                         | (1<<STDENC_MODE_NTSCM_J)
                                         | (1<<STDENC_MODE_NTSCM_443);
            Capability_p->SquarePixel = 0;

            if (Device_p->DencVersion >=6)
            {
                Capability_p->Modes |= (1<<STDENC_MODE_SECAM);
                /* Capability_p->Interlaced : no change */
                /* Capability_p->FieldRate60Hz : no change */
                Capability_p->SquarePixel =   (1<<STDENC_MODE_NTSCM)
                                            | (1<<STDENC_MODE_NTSCM_J)
                                            | (1<<STDENC_MODE_NTSCM_443)
                                            | (1<<STDENC_MODE_PALBDGHI)
                                            | (1<<STDENC_MODE_PALM)
                                            | (1<<STDENC_MODE_PALN)
                                            | (1<<STDENC_MODE_PALN_C);
            }
            if (Device_p->IsAuxEncoderOn)
            {
                Capability_p->Modes       |= (1<<STDENC_MODE_SECAM_AUX);
                Capability_p->SquarePixel |= (1<<STDENC_MODE_SECAM_AUX);
            }
            Capability_p->MinChromaDelay = STDENC_MIN_CHROMA_DELAY;
            if (Device_p->DencVersion >=10)
            {
                Capability_p->SquarePixel |=  (1<<STDENC_MODE_SECAM);
                Capability_p->MaxChromaDelay  = STDENC_MAX_CHROMA_DELAY_V10_MORE;
                Capability_p->StepChromaDelay = STDENC_STEP_CHROMA_DELAY_V10_MORE;
            }
            else
            {
                Capability_p->MaxChromaDelay  = STDENC_MAX_CHROMA_DELAY_V10_LESS;
                Capability_p->StepChromaDelay = STDENC_STEP_CHROMA_DELAY_V10_LESS;
            }

            Capability_p->BlackLevelPedestalAux = (Device_p->IsAuxEncoderOn);
            Capability_p->ChromaDelayAux        = (Device_p->IsAuxEncoderOn);
            Capability_p->LumaTrapFilterAux     = (Device_p->IsAuxEncoderOn);

            if (    (Device_p->DencVersion <  6)                        /* 4:2:2 only supported */
                 || (Device_p->DencVersion >= 12)                       /* 4:4:4 only supported */
                 || (Device_p->DeviceType == STDENC_DEVICE_TYPE_7015)   /* 4:2:2 only supported */
                 || (Device_p->DeviceType == STDENC_DEVICE_TYPE_7020)   /* 4:4:4 only supported */
                 || (Device_p->DeviceType == STDENC_DEVICE_TYPE_GX1))    /* 4:4:4 only supported */
            {
                Capability_p->YCbCr444Input = FALSE;
            }
            else
            {
                Capability_p->YCbCr444Input = TRUE;
            }
            Capability_p->CellId = (U8)Device_p->DencVersion;
            Capability_p->DeviceType = Device_p->DeviceType;
        } /* else of if (DeviceIndex == INVALID_DEVICE_INDEX) */
    } /* else of if (!FirstInitDone) */

    LeaveCriticalSection();

    return(ErrorCode);
} /* End of STDENC_GetCapability() function */

/*
--------------------------------------------------------------------------------
Initialise DENC driver
(Standard instances management. Driver specific implementations should be put in Init() function.)
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STDENC_Init(const ST_DeviceName_t DeviceName, const STDENC_InitParams_t * const InitParams_p)
{
    S32 Index = 0;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if ((InitParams_p == NULL) ||                        /* There must be parameters */
        (InitParams_p->MaxOpen > STDENC_MAX_OPEN) ||     /* No more than MAX_OPEN open handles supported */
        (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1) || /* Device name length should be respected */
        ((((const char *) DeviceName)[0]) == '\0')       /* Device name should not be empty */
       )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

	EnterCriticalSection();

    /* First time: initialise devices and units as empty */
    if (!FirstInitDone)
    {
        for (Index = 0; Index < STDENC_MAX_DEVICE; Index++)
        {
            DeviceArray[Index].DeviceName[0] = '\0';
        }

        for (Index = 0; Index < STDENC_MAX_UNIT; Index++)
        {
            UnitArray[Index].UnitValidity = 0;
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
        while ((DeviceArray[Index].DeviceName[0] != '\0') && (Index < STDENC_MAX_DEVICE))
        {
            Index++;
        }

        if (Index >= STDENC_MAX_DEVICE)
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
} /* End of STDENC_Init() function */


/*
--------------------------------------------------------------------------------
open ...
(Standard instances management. Driver specific implementations should be put in Open() function.)
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STDENC_Open(  const ST_DeviceName_t DeviceName
                           , const STDENC_OpenParams_t * const OpenParams_p
                           , STDENC_Handle_t * const Handle_p)
{
    S32 DeviceIndex = INVALID_DEVICE_INDEX, UnitIndex, Index;
    U32 OpenedUnitForThisInit;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if ((OpenParams_p == NULL) ||                       /* There must be parameters ! */
        (Handle_p == NULL) ||                           /* Pointer to handle should be valid ! */
        (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1)|| /* Device name length should be respected */
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
            UnitIndex = STDENC_MAX_UNIT;
            OpenedUnitForThisInit = 0;
            for (Index = 0; Index < STDENC_MAX_UNIT; Index++)
            {
                if ((UnitArray[Index].UnitValidity == STDENC_VALID_UNIT) && (UnitArray[Index].Device_p == &DeviceArray[DeviceIndex]))
                {
                    OpenedUnitForThisInit ++;
                }
                if (UnitArray[Index].UnitValidity != STDENC_VALID_UNIT)
                {
                    /* Found a free handle structure */
                    UnitIndex = Index;
                }
            }
            if ((OpenedUnitForThisInit >= DeviceArray[DeviceIndex].MaxOpen) || (UnitIndex >= STDENC_MAX_UNIT))
            {
                /* None of the units is free or MaxOpen reached */
                ErrorCode = ST_ERROR_NO_FREE_HANDLES;
            }
            else
            {
                *Handle_p = (STDENC_Handle_t) &UnitArray[UnitIndex];
                UnitArray[UnitIndex].Device_p = &DeviceArray[DeviceIndex];

                /* API specific actions after opening */
                ErrorCode = Open(&UnitArray[UnitIndex], OpenParams_p);

                if (ErrorCode == ST_NO_ERROR)
                {
                    /* Register opened handle */
                    UnitArray[UnitIndex].UnitValidity = STDENC_VALID_UNIT;

                    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Handle opened on device '%s'", DeviceName));
                }
            } /* End found unit unused */
        } /* End device valid */
    } /* End FirstInitDone */

    LeaveCriticalSection();

    return(ErrorCode);
} /* End fo STDENC_Open() function */


/*
--------------------------------------------------------------------------------
Close ...
(Standard instances management. Driver specific implementations should be put in Close() function.)
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STDENC_Close(STDENC_Handle_t Handle)
{
    DENC_t *Unit_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    EnterCriticalSection();

    if (!FirstInitDone)
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
            Unit_p = (DENC_t *) Handle;

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
} /* End of STDENC_Close() function */


/*
--------------------------------------------------------------------------------
Terminate DENC driver
(Standard instances management. Driver specific implementations should be put in Term() function.)
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STDENC_Term(const ST_DeviceName_t DeviceName, const STDENC_TermParams_t *const TermParams_p)
{
    DENC_t *Unit_p;
    S32 DeviceIndex = INVALID_DEVICE_INDEX, UnitIndex = 0;
    BOOL Found = FALSE;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if ((TermParams_p == NULL) ||                       /* There must be parameters ! */
        (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1) || /* Device name length should be respected */
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
            /* UnitIndex = 0;
            Found = FALSE;*/
            Unit_p = UnitArray;
            while ((UnitIndex < STDENC_MAX_UNIT) && (!Found))
            {
                Found = ((Unit_p->Device_p == &DeviceArray[DeviceIndex]) && (Unit_p->UnitValidity == STDENC_VALID_UNIT));
                Unit_p++;
                UnitIndex++;
            }

            if (Found)
            {
                if (TermParams_p->ForceTerminate)
                {
                    Unit_p = UnitArray;
                    UnitIndex = 0;
                    while (UnitIndex < STDENC_MAX_UNIT)
                    {
                        if ((Unit_p->Device_p == &DeviceArray[DeviceIndex]) && (Unit_p->UnitValidity == STDENC_VALID_UNIT))
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
} /* End of STDENC_Term() function */




/* REGISTER ACCESS EXPORTED FUNCTIONS  */

/*******************************************************************************
Name        : STDENC_CheckHandle
Description : check STDENC connection Handle
Parameters  : Handle  : IN  : Handle on DENC driver connection
Assumptions :
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_INVALID_HANDLE
*******************************************************************************/
ST_ErrorCode_t STDENC_CheckHandle(STDENC_Handle_t Handle)
{
    if (!IsValidHandle(Handle))
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    else
    {
        return(ST_NO_ERROR);
    }
} /* End of STDENC_CheckHandle() function */

/* ----------------------------- End of file ------------------------------ */




