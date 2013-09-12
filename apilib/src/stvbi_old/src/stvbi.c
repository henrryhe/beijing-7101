/*******************************************************************************

File name   : stvbi.c

Description : VBI driver module API functions source file

COPYRIGHT (C) STMicroelectronics 2003.

Date               Modification                                     Name
----               ------------                                     ----
27 Jul 2000        Created                                           JG
16 Oct 2000        1.0.0A2                                           JG
11 Dec 2000        Improve error tracking                            HSdLM
22 Feb 2001        Use STDENC mutual exclusion register accesses     HSdLM
 *                 Move HAL functions & code to new file vbi_hal.c,
 *                 add capabilities test at STVBI_Open() call
04 Jul 2001        Terminate as much as possible. Non-API exported   HSdLM
 *                 symbols begin with stvbi_. Remove VBI exclusion
 *                 New Closed-caption API parameter. Restrict to 16
 *                 TTX lines premgramming. New dynamic parameters
 *                 checking.
14 Nov 2001        Select TTX lines individually (DDTS GNBvd08671)   HSdLM
 *                 New DeviceType STVBI_DEVICE_TYPE_GX1 for ST40GX1
02 Jan 2005        Add Support of OS21/ST40                          AC
*******************************************************************************/

/* Includes ----------------------------------------------------------------- */

#if !defined(ST_OSLINUX)
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#endif


#include "stdenc.h"

#include "vbi_rev.h"

#include "stvbi.h"
#include "vbi_drv.h"
#include "vbi_hal.h"
#include "vbi_vpreg.h"






/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */

#define STVBI_MAX_UNIT    (STVBI_MAX_OPEN * STVBI_MAX_DEVICE)
#define INVALID_DEVICE_INDEX (-1)

/* #define VBI_EXCLUSIVE 1 */
/* to make sharing lines VBI exclusive, uncomment define below and add STVBI_ERROR_VBI_CONFLICT error code*/

/* CLOSED CAPTION CONSTANTS */
/* Line numbers follow 525-SMPTE line number convention and 625-CCIR/ITU-R line number convention */
#define CC_DEFAULT_LINE_F1  21    /* default closed caption/extended data line insertion for field 1 */
#define CC_DEFAULT_LINE_F2  284   /* default closed caption/extended data line insertion for field 2 */

/* TELETEXT CONSTANTS, EN 300 472 V1.2.2 */
#define TTX_LINES_MAX       18         /* 6 to 23 or 318 to 335 */
#define TTX_LINE_MASK_MIN   0x40       /*  */
#define TTX_LINE_MASK_MAX   0xFFFFC0   /*  */

#if defined(ST_OSLINUX)
#define VBI_PARTITION_AVMEM  0
#endif

/* Private Variables (static)------------------------------------------------ */

static VBIDevice_t VBIDeviceArray[STVBI_MAX_DEVICE];
static VBI_t VBIArray[STVBI_MAX_UNIT];
static semaphore_t* InstancesAccessControl_p;   /* Init/Open/Close/Term protection semaphore */
static BOOL FirstInitDone = FALSE;

/* Global Variables --------------------------------------------------------- */

#ifdef DEBUG
VBIDevice_t *stvbi_DbgDev = VBIDeviceArray;
VBI_t *stvbi_DbgPtr = VBIArray;
#endif

/* Private Macros ----------------------------------------------------------- */

/* Passing a (STVBI_Handle_t), returns TRUE if the handle is valid, FALSE otherwise */
#define IsValidHandle(Handle) ((((VBI_t *) (Handle)) >= VBIArray) &&                    \
                               (((VBI_t *) (Handle)) < (VBIArray + STVBI_MAX_UNIT)) &&  \
                               (((VBI_t *) (Handle))->UnitValidity == STVBI_VALID_UNIT))


/* Private Function prototypes ---------------------------------------------- */
static void EnterCriticalSection(void);
static void LeaveCriticalSection(void);
static S32 GetIndexOfDeviceNamed(const ST_DeviceName_t WantedName);
static ST_ErrorCode_t Init(VBIDevice_t * const Device_p, const STVBI_InitParams_t * const InitParams_p);
static ST_ErrorCode_t Open(VBI_t * const Unit_p, const STVBI_OpenParams_t * const OpenParams_p);
static ST_ErrorCode_t Close(VBI_t * const Unit_p);
static ST_ErrorCode_t Term(VBIDevice_t * const Device_p);

static void GetCapability(const VBIDevice_t * const Device_p, STVBI_Capability_t * const Capability_p);
static ST_ErrorCode_t CheckVbiPriority( const VBI_t * const Unit_p, const STVBI_VbiType_t VbiType);
static ST_ErrorCode_t CheckVbiOpenParams(const STVBI_OpenParams_t * const OpenParams_p);
static ST_ErrorCode_t CheckVbiDynamicParams( const VBI_t * const Unit_p,const STVBI_DynamicParams_t* const DynamicParams_p);
static BOOL IsABlock(const U32 Mask);
static BOOL IsLengthOk( VBI_t * const Unit_p, const U8 Length);
static void UnitSetDynamicParams( VBI_t * const Unit_p, const STVBI_DynamicParams_t * const DynamicParams_p);
static void Report( const char * const String, const ST_ErrorCode_t ErrorCode);



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
        /* Initialise the Init/Open/Close/Term protection semaphore
          Caution: this semaphore is never deleted. (Not an issue) */
        InstancesAccessControl_p = STOS_SemaphoreCreateFifo(NULL,1);
        InstancesAccessControlInitialized = TRUE;
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
Name        : Init
Description : API specific initialisations
Parameters  : pointer on device and initialisation parameters
Assumptions : pointer on device is not NULL
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_BAD_PARAMETER, STVBI_ERROR_DENC_OPEN
*******************************************************************************/
static ST_ErrorCode_t Init(VBIDevice_t * const Device_p, const STVBI_InitParams_t * const InitParams_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    const STDENC_OpenParams_t DencOpenParams;
#if defined (ST_7710) || defined(ST_7100) || defined(ST_7109)||defined (ST_7200)
    const STLAYER_OpenParams_t LayerOpenParams;
    STLAYER_Capability_t LayerCapability;
    ST_DeviceName_t LayerName;
#endif
    STDENC_Capability_t DencCapability;
    ST_DeviceName_t DencName;



    switch (InitParams_p->DeviceType)
    {
        case STVBI_DEVICE_TYPE_GX1 :

            if (    (InitParams_p->Target.FullCell.BaseAddress_p == NULL)
                 || (strlen(InitParams_p->Target.FullCell.DencName) > ST_MAX_DEVICE_NAME - 1))
            {
                return(ST_ERROR_BAD_PARAMETER);
            }
            Device_p->BaseAddress_p = (void *) (  ((U8 *) InitParams_p->Target.FullCell.DeviceBaseAddress_p)
                                                + ((U32)  InitParams_p->Target.FullCell.BaseAddress_p)
                                               );
            strcpy(DencName, InitParams_p->Target.FullCell.DencName);
            break;

        case STVBI_DEVICE_TYPE_DENC :
            if (strlen(InitParams_p->Target.DencName) > ST_MAX_DEVICE_NAME - 1)
            {
                return(ST_ERROR_BAD_PARAMETER);
            }
            strcpy(DencName, InitParams_p->Target.DencName);
            break;

#if defined (ST_7710)||defined (ST_7109)||defined (ST_7100)||defined (ST_7200)

        case STVBI_DEVICE_TYPE_VIEWPORT :

                /* ------------ Memory  Allocation ------------ */

                /* need to be done after stvbi_AllocResource () */

            #if defined(ST_OSLINUX)
                        Device_p->AVMemPartitionHandle = STAVMEM_GetPartitionHandle( VBI_PARTITION_AVMEM );
            #else
                        Device_p->AVMemPartitionHandle = InitParams_p->Target.Viewport.AVMemPartitionHandle;
            #endif


             if ((strlen(InitParams_p->Target.Viewport.LayerName) > ST_MAX_DEVICE_NAME - 1)
                    ||(strlen(InitParams_p->Target.Viewport.LayerName) < 1))
            {
                return(ST_ERROR_BAD_PARAMETER);
            }
            strcpy(LayerName, InitParams_p->Target.Viewport.LayerName);
            break;
#endif


        default :
            return(ST_ERROR_BAD_PARAMETER);
            break;
    }

    switch (InitParams_p->DeviceType)
    {
        case STVBI_DEVICE_TYPE_GX1 : /* no break; */
        case STVBI_DEVICE_TYPE_DENC :
            ErrorCode = STDENC_Open(  DencName
                                    , &DencOpenParams
                                    , (STDENC_Handle_t * const)&(Device_p->DencHandle));
            if (ErrorCode == ST_NO_ERROR)
            {
                ErrorCode = STDENC_GetCapability( DencName, &DencCapability);
            }
            if (ErrorCode == ST_NO_ERROR)
            {
                Device_p->DencCellId = DencCapability.CellId;
            }
            else
            {
                ErrorCode = STVBI_ERROR_DENC_OPEN;
            }
            break;
#if defined (ST_7710) || defined(ST_7100) || defined(ST_7109)||defined (ST_7200)
        case STVBI_DEVICE_TYPE_VIEWPORT :

                       /* Check the LayerType */

                ErrorCode = STLAYER_GetCapability( LayerName, &LayerCapability);

                if (ErrorCode == ST_NO_ERROR)
                    {
                       if(LayerCapability.LayerType!=STLAYER_GAMMA_GDPVBI)
                            {
                                return(ST_ERROR_BAD_PARAMETER);
                            }
                    }

                if (ErrorCode == ST_NO_ERROR)
                       {
                            /* Layer Open */
                         ErrorCode = STLAYER_Open(  LayerName
                                                , &LayerOpenParams
                                                , (STLAYER_Handle_t * const)&(Device_p->LayerHandle));
                       }
                ErrorCode = stvbi_HalAllocResource(Device_p);
                if ( ErrorCode != ST_NO_ERROR)
                {
                    return (ErrorCode);
                }


             break;
#endif
        default :
            /* yet filtered, just above */
            break;
    }

    if ( ErrorCode != ST_NO_ERROR)
    {
        STTBX_Print(("Error > STVBI Init Failed ! \n"));
    }
    else
    {

        Device_p->DeviceType = InitParams_p->DeviceType;
        Device_p->TeletextSource = STVBI_TELETEXT_SOURCE_DMA;
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Denc version %d", Device_p->DencCellId));

    }
    return(ErrorCode);
} /* End of Init() function */

/*******************************************************************************
Name        : Open
Description : API specific actions just before opening
Parameters  : pointer on unit and open parameters
Assumptions : pointer on unit and OpenParams_p are not NULL
Limitations :
Returns     : ST_NO_ERROR, STVBI_ERROR_VBI_ALREADY_REGISTERED
 *            STVBI_ERROR_UNSUPPORTED_VBI, STVBI_ERROR_DENC_ACCESS, ST_ERROR_BAD_PARAMETER
*******************************************************************************/
static ST_ErrorCode_t Open(VBI_t * const Unit_p, const STVBI_OpenParams_t * const OpenParams_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* check VbiType asked can be set : not already registered, no priority conflict */
    ErrorCode = CheckVbiPriority( Unit_p, OpenParams_p->Configuration.VbiType);
    if (ErrorCode == ST_NO_ERROR)
    {
        SET_VBI_TYPE(  Unit_p, OpenParams_p->Configuration.VbiType);  /* set this type */
        REGISTER_VBI(Unit_p, OpenParams_p->Configuration.VbiType);  /* Vbi is registered */

        /* CheckVbiPriority has already checked OpenParams_p->Configuration.VbiType is OK */
        Unit_p->Params.Conf.VbiType = OpenParams_p->Configuration.VbiType;
        Unit_p->Params.Dyn.VbiType  = OpenParams_p->Configuration.VbiType;
        switch (Unit_p->Params.Conf.VbiType)
        {
            case STVBI_VBI_TYPE_TELETEXT : /* ok */
                Unit_p->Params.Conf.Type.TTX.Latency   = OpenParams_p->Configuration.Type.TTX.Latency;
                Unit_p->Params.Conf.Type.TTX.FullPage  = OpenParams_p->Configuration.Type.TTX.FullPage;
                Unit_p->Params.Conf.Type.TTX.Amplitude = OpenParams_p->Configuration.Type.TTX.Amplitude;
                Unit_p->Params.Conf.Type.TTX.Standard  = OpenParams_p->Configuration.Type.TTX.Standard;
                ErrorCode = stvbi_HALSetTeletextConfiguration(Unit_p);
                break;
            case STVBI_VBI_TYPE_CLOSEDCAPTION : /* ok */
                Unit_p->Params.Conf.Type.CC.Mode = OpenParams_p->Configuration.Type.CC.Mode;
                Unit_p->Params.Dyn.Type.CC.Mode  = OpenParams_p->Configuration.Type.CC.Mode;
                Unit_p->Params.Dyn.Type.CC.LinesField1 = CC_DEFAULT_LINE_F1;
                Unit_p->Params.Dyn.Type.CC.LinesField2 = CC_DEFAULT_LINE_F2;
                /* no hardware configuration */
                break;
            case STVBI_VBI_TYPE_WSS : /* tbd */
                break;
            case STVBI_VBI_TYPE_CGMS : /* tbd */
                break;
            case STVBI_VBI_TYPE_VPS : /* tbd */
                break;
        #ifdef COPYPROTECTION
            case STVBI_VBI_TYPE_MACROVISION : /* ok */
                SET_VBI_STATUS(Unit_p, TRUE); /* Macrovision is enabled at reset */
                Unit_p->Params.Conf.Type.MV.Algorithm = OpenParams_p->Configuration.Type.MV.Algorithm;
                ErrorCode = stvbi_HALInitCopyProtection(Unit_p);
                break;
        #endif /* #ifdef COPYPROTECTION */
           case STVBI_VBI_TYPE_CGMSFULL : /* ok */

#if defined (ST_7710) ||defined (ST_7109)||defined (ST_7100)||defined (ST_7200)

                    /* need to verfie CGMSFULLStandard is spported or not */
                Unit_p->Params.Conf.Type.CGMSFULL.CGMSFULLStandard   = OpenParams_p->Configuration.Type.CGMSFULL.CGMSFULLStandard;
                       /* Initialisation of of VBI Viewport */
                ErrorCode = stvbi_HalInitFullCgmsViewport(Unit_p);

#endif
            default:
                /* not possible, already filtered, see above */
                break;
        }


    }
    return(ErrorCode);
} /* End of Open() function */

/*******************************************************************************
Name        : Close
Description : API specific actions just before closing
Parameters  : pointer on unit
Assumptions : pointer on unit is not NULL
Limitations :
Returns     : ST_NO_ERROR, STVBI_ERROR_DENC_ACCESS
*******************************************************************************/
static ST_ErrorCode_t Close(VBI_t * const Unit_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

        switch (Unit_p->Device_p->DeviceType)
                {
                    case STVBI_DEVICE_TYPE_VIEWPORT :

#if defined (ST_7710)||defined (ST_7109)||defined (ST_7100)||defined (ST_7200)

                        ErrorCode =stvbi_HalCloseFullCgmsViewport(Unit_p);
#endif
                        break;

                     case STVBI_DEVICE_TYPE_DENC : /* no break; */
                     case STVBI_DEVICE_TYPE_GX1 :
                                         ErrorCode = stvbi_HALDisable(Unit_p);
                        break;
                     default :           /* not handled, see assumptions */
                        break;


                 }


    return(ErrorCode);
} /* End of Close() function */


/*******************************************************************************
Name        : Term
Description : API specific terminations
Parameters  : pointer on device and termination parameters
Assumptions : pointer on device is not NULL
Limitations :
Returns     : ST_NO_ERROR, STVBI_ERROR_DENC_OPEN
*******************************************************************************/
static ST_ErrorCode_t Term(VBIDevice_t * const Device_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    switch (Device_p->DeviceType)
    {
        case STVBI_DEVICE_TYPE_DENC : /* no break; */
        case STVBI_DEVICE_TYPE_GX1 :
            if (STDENC_Close(Device_p->DencHandle) != ST_NO_ERROR)
            {
                ErrorCode = STVBI_ERROR_DENC_OPEN;
            }
            break;
       case STVBI_DEVICE_TYPE_VIEWPORT :

    #if defined (ST_7710) ||defined (ST_7109)||defined (ST_7100)||defined (ST_7200)
                                    /* Dealllocate Memory */
                 ErrorCode = stvbi_HalFreeResource(Device_p);

                if ( ErrorCode == ST_NO_ERROR)
                {
                                            /* Layer Close */
                    ErrorCode =STLAYER_Close(Device_p->LayerHandle);
                }
   #endif

      break;
        default :
            /* filtered at init */
            break;
    }
    return(ErrorCode);
} /* End of Term() function */


/*******************************************************************************
Name        : GetIndexOfDeviceNamed
Description : Get the index in VBIDeviceArray where a device has been
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
        if (strcmp(VBIDeviceArray[Index].DeviceName, WantedName) == 0)
        {
            /* Name found in the initialised devices */
            WantedIndex = Index;
        }
        Index++;
    } while ((WantedIndex == INVALID_DEVICE_INDEX) && (Index < STVBI_MAX_DEVICE));

    return(WantedIndex);
} /* End of GetIndexOfDeviceNamed() function */

/*******************************************************************************
Name        : GetCapability
Description : Get capability of device
Parameters  : IN : Device_p : device aimed
 *            OUT : Capability_p : capability of device
Assumptions : pointer are not null and accurate
Limitations :
Returns     : none
*******************************************************************************/
static void GetCapability(const VBIDevice_t * const Device_p, STVBI_Capability_t * const Capability_p)
{
    switch (Device_p->DeviceType)
    {
        case STVBI_DEVICE_TYPE_DENC : /* no break */
        case STVBI_DEVICE_TYPE_GX1 :
        case STVBI_DEVICE_TYPE_VIEWPORT :

            Capability_p->SupportedTtxStd =
                STVBI_TELETEXT_STANDARD_B ;

            Capability_p->SupportedMvAlgo = 0;
            #ifdef COPYPROTECTION
                Capability_p->SupportedMvAlgo =
                    STVBI_COPY_PROTECTION_MV6 |
                    STVBI_COPY_PROTECTION_MV7 ;
            #endif /* #ifdef COPYPROTECTION */


            Capability_p->SupportedVbi =
                STVBI_VBI_TYPE_CLOSEDCAPTION |
                STVBI_VBI_TYPE_CGMS          |
                STVBI_VBI_TYPE_TELETEXT      ;

            #if defined (ST_7710)||defined (ST_7109)||defined (ST_7100)||defined (ST_7200)

            Capability_p->SupportedVbi |=  STVBI_VBI_TYPE_CGMSFULL ;

            #endif

            #ifdef COPYPROTECTION
                Capability_p->SupportedVbi |= STVBI_VBI_TYPE_MACROVISION;
            #endif /* #ifdef COPYPROTECTION */

            Capability_p->SupportedTeletextSource = STVBI_TELETEXT_SOURCE_DMA;

            switch (Device_p->DencCellId)
            {
                case 3 : /* no more */
                    break;
                case 5 :   /* no more */
                    break;
                case 6 :
                    Capability_p->SupportedVbi |=
                        STVBI_VBI_TYPE_WSS   |
                        STVBI_VBI_TYPE_VPS   ;
                    break;
                case 7 :
                case 8 :
                case 9 :
                case 10 :
                case 11 :
                case 12 :
                case 13 :
                    Capability_p->SupportedVbi |=
                        STVBI_VBI_TYPE_WSS   |
                        STVBI_VBI_TYPE_VPS   ;
                    Capability_p->SupportedTtxStd |=
                        STVBI_TELETEXT_STANDARD_C ;
                    break;

                default :
                    /* see assumptions */
                    break;
            }
            if (Device_p->DeviceType == STVBI_DEVICE_TYPE_GX1)
            {
                Capability_p->SupportedTeletextSource |= STVBI_TELETEXT_SOURCE_PIN;
            }
            break;
        default :
            /* not handled, see assumptions */
            break;
    }
} /* End of GetCapability() function */


/*******************************************************************************
Name        : CheckVbiPriority
Description : Not all VBI features can be used at the same time, and there is a
 *            priority between them. This routine returns a error if this rule is
 *            not followed.
Parameters  : IN : Unit_p : device aimed
 *            IN : VbiType : kind of VBI feature
Assumptions :
Limitations :
Returns     : ST_NO_ERROR, STVBI_ERROR_VBI_ALREADY_REGISTERED
 *            STVBI_ERROR_UNSUPPORTED_VBI
*******************************************************************************/
static ST_ErrorCode_t CheckVbiPriority( const VBI_t * const Unit_p, const STVBI_VbiType_t VbiType)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STVBI_Capability_t Capability;

    if ( (Unit_p->Device_p->VbiRegistered & VbiType) != 0)
    {
#ifndef VBI_MULT_CC
        return(STVBI_ERROR_VBI_ALREADY_REGISTERED);
#endif
    }
    GetCapability(Unit_p->Device_p, &Capability);
    if ((Capability.SupportedVbi & VbiType) == 0)
    {
        return(STVBI_ERROR_UNSUPPORTED_VBI);
    }
#ifdef VBI_EXCLUSIVE
    switch (VbiType)
    {
        case STVBI_VBI_TYPE_TELETEXT :       /* teletext don't accept other vbi */
            if ( (Unit_p->Device_p->VbiRegistered & ~STVBI_VBI_TYPE_TELETEXT) != 0 )
            {
                ErrorCode = STVBI_ERROR_VBI_CONFLICT;
            }
            break;
        case STVBI_VBI_TYPE_CLOSEDCAPTION : /* closed caption don't accept other vbi */
            if ( (Unit_p->Device_p->VbiRegistered & ~STVBI_VBI_TYPE_CLOSEDCAPTION) != 0 )
            {
                ErrorCode = STVBI_ERROR_VBI_CONFLICT;
            }
            break;
        case STVBI_VBI_TYPE_CGMS : /* cgms accept Wss, Vps and Macrovision */
            if ( (Unit_p->Device_p->VbiRegistered & ~(STVBI_VBI_TYPE_WSS | STVBI_VBI_TYPE_CGMS | STVBI_VBI_TYPE_VPS |STVBI_VBI_TYPE_MACROVISION)) != 0 )
            {
                ErrorCode = STVBI_ERROR_VBI_CONFLICT;
            }
            break;
        case STVBI_VBI_TYPE_WSS    :  /* Wss, Vps are compatible and accept Cgms but, */
        case STVBI_VBI_TYPE_VPS    :  /* don't accept other vbi */
                    if ( (Unit_p->Device_p->VbiRegistered & ~(STVBI_VBI_TYPE_WSS | STVBI_VBI_TYPE_CGMS | STVBI_VBI_TYPE_VPS)) != 0 )
            {
                ErrorCode = STVBI_ERROR_VBI_CONFLICT;
            }
            break;
         case STVBI_VBI_TYPE_CGMSFULL :  /* don't accept other vbi */
            if ( (Unit_p->Device_p->VbiRegistered & ~(STVBI_VBI_TYPE_CGMSFULL)) != 0 )
            {
                ErrorCode = STVBI_ERROR_VBI_CONFLICT;
            }
            break;

    #ifdef COPYPROTECTION
        case STVBI_VBI_TYPE_MACROVISION : /* macrovision only compatible with Cgms */
            if ( (Unit_p->Device_p->VbiRegistered & ~(STVBI_VBI_TYPE_MACROVISION | STVBI_VBI_TYPE_CGMS)) !=0 )
            {
                ErrorCode = STVBI_ERROR_VBI_CONFLICT;
            }
            break;
    #endif /* #ifdef COPYPROTECTION */
        default:
            ErrorCode = STVBI_ERROR_UNSUPPORTED_VBI;
            break;
    } /* end of switch (VbiType) */
#endif /* #ifdef VBI_EXCLUSIVE */
    return (ErrorCode);
} /* End of CheckVbiPriority */


/*******************************************************************************
Name        : CheckVbiOpenParams
Description : Check open parameters for STVBI_Open standard API routine.
Parameters  : IN : OpenParams_p : pointer on Open parameters
Assumptions : OpenParams_p is not NULL (checked before function call)
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_BAD_PARAMETER
*******************************************************************************/
static ST_ErrorCode_t CheckVbiOpenParams(const STVBI_OpenParams_t* const OpenParams_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    const STVBI_TeletextConfiguration_t * Ttx_p;
    const STVBI_CCConfiguration_t *       Cc_p;
    const STVBI_CGMSFULLConfiguration_t * CgmsFull_p;
#ifdef COPYPROTECTION
    const STVBI_MacrovisionConfiguration_t * Mv_p;
#endif /* #ifdef COPYPROTECTION */

    switch (OpenParams_p->Configuration.VbiType)
    {
        case STVBI_VBI_TYPE_TELETEXT :
            Ttx_p = &(OpenParams_p->Configuration.Type.TTX);
            /* Latency = 2 to 9 */
            if (    (Ttx_p->Latency < 2)
                 || (Ttx_p->Latency > 9)
               )
            {
                ErrorCode = ST_ERROR_BAD_PARAMETER;
            }
            /* Amplitude = 70 or 100IRE */
            if (   (Ttx_p->Amplitude != STVBI_TELETEXT_AMPLITUDE_70IRE)
                && (Ttx_p->Amplitude != STVBI_TELETEXT_AMPLITUDE_100IRE)
               )
            {
                ErrorCode = ST_ERROR_BAD_PARAMETER;
            }
            /* Standard = A,B,C or D */
            if (   (Ttx_p->Standard != STVBI_TELETEXT_STANDARD_A)
                && (Ttx_p->Standard != STVBI_TELETEXT_STANDARD_B)
                && (Ttx_p->Standard != STVBI_TELETEXT_STANDARD_C)
                && (Ttx_p->Standard != STVBI_TELETEXT_STANDARD_D)
               )
            {
                ErrorCode = ST_ERROR_BAD_PARAMETER;
            }
                break;
        case STVBI_VBI_TYPE_CLOSEDCAPTION :
            Cc_p = &(OpenParams_p->Configuration.Type.CC);
            if (    (Cc_p->Mode != STVBI_CCMODE_NONE)
                 && (Cc_p->Mode != STVBI_CCMODE_FIELD1)
                 && (Cc_p->Mode != STVBI_CCMODE_FIELD2)
                 && (Cc_p->Mode != STVBI_CCMODE_BOTH)
               )
            {
                ErrorCode = ST_ERROR_BAD_PARAMETER;
            }
                break;
        case STVBI_VBI_TYPE_CGMSFULL :
             CgmsFull_p = &(OpenParams_p->Configuration.Type.CGMSFULL);

                       /* Mode  = 1080I / 720P or 480P */
            if (   (CgmsFull_p->CGMSFULLStandard != STVBI_CGMS_HD_1080I60000)
                && (CgmsFull_p->CGMSFULLStandard != STVBI_CGMS_HD_720P60000)
                && (CgmsFull_p->CGMSFULLStandard != STVBI_CGMS_SD_480P60000)
                && (CgmsFull_p->CGMSFULLStandard != STVBI_CGMS_TYPE_B_HD_1080I60000)
                && (CgmsFull_p->CGMSFULLStandard != STVBI_CGMS_TYPE_B_HD_720P60000)
                && (CgmsFull_p->CGMSFULLStandard != STVBI_CGMS_TYPE_B_SD_480P60000)

               )
            {
                ErrorCode = ST_ERROR_BAD_PARAMETER;
            }
            break;

        case STVBI_VBI_TYPE_WSS :  /* no parameter */
        case STVBI_VBI_TYPE_CGMS : /* no parameter */
        case STVBI_VBI_TYPE_VPS :  /* no parameter */
                   break;
    #ifdef COPYPROTECTION
        case STVBI_VBI_TYPE_MACROVISION :
            Mv_p = &(OpenParams_p->Configuration.Type.MV);
            if (    (Mv_p->Algorithm != STVBI_COPY_PROTECTION_MV6)
                 && (Mv_p->Algorithm != STVBI_COPY_PROTECTION_MV7)
               )
            {
                ErrorCode = ST_ERROR_BAD_PARAMETER;
            }
            break;
    #endif /* #ifdef COPYPROTECTION */

        default :
            ErrorCode = ST_ERROR_BAD_PARAMETER;
            break;
    }
    return (ErrorCode);
} /* End of CheckVbiOpenParams() function */


/*******************************************************************************
Name        : CheckVbiDynamicParams
Description : Check dynamic parameters for STVBI_SetDynamicParams API routine.
 *            Two checking level possible : API level, HAL level.
Parameters  : IN : Unit_p : device aimed
 *            IN : DynamicParams_p : pointer on dynamic parameters
Assumptions : DynamicParams_p is not NULL (checked before function call)
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_BAD_PARAMETER, ST_ERROR_FEATURE_NOT_SUPPORTED
*******************************************************************************/
static ST_ErrorCode_t CheckVbiDynamicParams( const VBI_t * const Unit_p,
                                             const STVBI_DynamicParams_t* const DynamicParams_p
                                           )
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    if (Unit_p->Params.Conf.VbiType != DynamicParams_p->VbiType)
    {
        ErrorCode = ST_ERROR_BAD_PARAMETER;
    }
    else
    {
        /* API level check */
        switch ( DynamicParams_p->VbiType)
        {
            case STVBI_VBI_TYPE_TELETEXT :
                if (    (Unit_p->Device_p->DencCellId <= 6)
                     && (DynamicParams_p->Type.TTX.LineCount == 0)
                     && (!IsABlock(DynamicParams_p->Type.TTX.LineMask)))
                {
                    /* DENC v3 to 6 don't support individual line selection but only blocks */
                    ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
                }
                else if (   (DynamicParams_p->Type.TTX.LineCount > TTX_LINES_MAX)
                         || (    (DynamicParams_p->Type.TTX.LineCount == 0)
                              && (   (DynamicParams_p->Type.TTX.LineMask < TTX_LINE_MASK_MIN)
                                  || (DynamicParams_p->Type.TTX.LineMask > TTX_LINE_MASK_MAX))))
                {
                    ErrorCode = ST_ERROR_BAD_PARAMETER;
                }
                break;
            case STVBI_VBI_TYPE_CLOSEDCAPTION : /* no break */
            case STVBI_VBI_TYPE_WSS :           /* no break */
            case STVBI_VBI_TYPE_CGMS :          /* no break */
            case STVBI_VBI_TYPE_CGMSFULL :      /* no break */
            case STVBI_VBI_TYPE_VPS :
                /* no API check */
                break;
        #ifdef COPYPROTECTION
            case STVBI_VBI_TYPE_MACROVISION :
                if (   DynamicParams_p->Type.MV.PredefinedMode < STVBI_MV_PREDEFINED_0
                     || DynamicParams_p->Type.MV.PredefinedMode > STVBI_MV_PREDEFINED_5 )
                {
                    ErrorCode = ST_ERROR_BAD_PARAMETER;
                }
               if ( DynamicParams_p->Type.MV.UserMode < STVBI_MV_USER_DEFINED_0
                    || DynamicParams_p->Type.MV.UserMode > (STVBI_MV_USER_DEFINED_0 |
                                                            STVBI_MV_USER_DEFINED_1 |
                                                            STVBI_MV_USER_DEFINED_2 |
                                                            STVBI_MV_USER_DEFINED_3 |
                                                            STVBI_MV_USER_DEFINED_4 |
                                                            STVBI_MV_USER_DEFINED_5 |
                                                            STVBI_MV_USER_DEFINED_6 ) )
                {

                    ErrorCode = ST_ERROR_BAD_PARAMETER; /* overwrite previous ErrorCode but with same value ! */

                }

                if ( (Unit_p->Device_p->DencCellId < 6) && (ErrorCode == ST_NO_ERROR)
                     && (DynamicParams_p->Type.MV.PredefinedMode > STVBI_MV_PREDEFINED_1)
                     && (DynamicParams_p->Type.MV.PredefinedMode < STVBI_MV_PREDEFINED_5))
                {
                    ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
                }
                break;
        #endif /* #ifdef COPYPROTECTION */
            default :
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                break;
        } /* end switch ( DynamicParams_p->VbiType)*/

        if (ErrorCode == ST_NO_ERROR)
        {
            /* HAL level check */
            ErrorCode = stvbi_HALCheckDynamicParams(Unit_p, DynamicParams_p);
        }
    }
    return( ErrorCode);
} /* end of CheckVbiDynamicParams() function */


/*******************************************************************************
Name        : IsABlock
Description : check all bits set in Mask are contiguous. Needed for TTX, DENC v<=6.
Parameters  : IN : Mask : TTX mask to test
Assumptions :
Limitations :
Returns     : TRUE (bits set are contiguous) or FALSE (bits set are not contiguous)
*******************************************************************************/
static BOOL IsABlock(const U32 Mask)
{
    U8 i;
    U8 NbRisingEdge=0;
    BOOL IsBitSet, IsPreviousBitSet;

    IsPreviousBitSet = ((Mask & 0x01) == 0x01);
    i=1;
    while ( (i<32) && (NbRisingEdge < 2))
    {
        IsBitSet = (((Mask>>i) & 0x01) == 0x01);
        if (IsBitSet && !IsPreviousBitSet)
        {
            NbRisingEdge++;
        }
        IsPreviousBitSet = IsBitSet;
        i++;
    }
    return(NbRisingEdge==1);
} /* end IsABlock() */


/*******************************************************************************
Name        : IsLengthOk
Description : check buffer length for STVBI_WriteData .
Parameters  : IN : Unit_p : device aimed
Assumptions : Unit_p is not NULL (checked before function call)
 *            Unit_p is opened, VbiType is OK.
Limitations :
Returns     : TRUE (length is ok) or FALSE (length is bad)
*******************************************************************************/
static BOOL IsLengthOk( VBI_t * const Unit_p, const U8 Length)
{
    BOOL IsNbBytesOk = FALSE;
    /* check :
    - 4 bytes for Closed Caption,
    - 14/17 bytes for Copy Protection (Macrovision V6, V7)
    - 2 bytes for WSS,
    - 6 bytes for VPS,
    - 3 bytes for CGMS.
    */
    switch (Unit_p->Params.Conf.VbiType)
    {
        case STVBI_VBI_TYPE_TELETEXT : /* no data to write */
            IsNbBytesOk = (Length == 0);
            break;
        case STVBI_VBI_TYPE_CLOSEDCAPTION :
            switch (Unit_p->Params.Dyn.Type.CC.Mode)
            {
                case STVBI_CCMODE_NONE :
                    IsNbBytesOk = (Length == 0);
                    break;
                case STVBI_CCMODE_FIELD1 :
                case STVBI_CCMODE_FIELD2 :
                    IsNbBytesOk = (Length == 2);
                    break;
                case STVBI_CCMODE_BOTH :
                    IsNbBytesOk = (Length == 4);
                    break;
                default : /* IsNbBytesOk remains FALSE */
                    break;
            } /* end of switch (Unit_p->Params.Conf.Type.CC.Mode) */
            break;
        case STVBI_VBI_TYPE_WSS :
            IsNbBytesOk = (Length == 2);
            break;
        case STVBI_VBI_TYPE_CGMS :

            IsNbBytesOk = (Length == 3);
             break;

        case STVBI_VBI_TYPE_CGMSFULL :

                        switch (Unit_p->Params.Conf.Type.CGMSFULL.CGMSFULLStandard) {

                                    case STVBI_CGMS_HD_1080I60000 :
                                    case STVBI_CGMS_HD_720P60000  :
                                    case STVBI_CGMS_SD_480P60000  :
                                                IsNbBytesOk = (Length == 3);
                                                break;
                                    case STVBI_CGMS_TYPE_B_HD_1080I60000 :
                                    case STVBI_CGMS_TYPE_B_HD_720P60000  :
                                    case STVBI_CGMS_TYPE_B_SD_480P60000  :
                                                IsNbBytesOk = (Length == 17);
                                                break;


                                     default:
                                                break;

                    }


            break;
        case STVBI_VBI_TYPE_VPS :
            IsNbBytesOk = (Length == 6);
            break;

    #ifdef COPYPROTECTION
        case STVBI_VBI_TYPE_MACROVISION :
            if (Unit_p->Params.Conf.Type.MV.Algorithm == STVBI_COPY_PROTECTION_MV6)
            {
                IsNbBytesOk = (Length == 14);
            }
            if (Unit_p->Params.Conf.Type.MV.Algorithm == STVBI_COPY_PROTECTION_MV7)
            {
                IsNbBytesOk = (Length == 17);
            }
            break;
    #endif /* #ifdef COPYPROTECTION */
        default :
            /* not handled, see assumptions. IsNbBytesOk remains FALSE */
            break;
    } /* end of switch (Unit_p->Params.Conf.VbiType) */
    return(IsNbBytesOk);
} /* end of IsLengthOk() function */


/*******************************************************************************
Name        : UnitSetDynamicParams
Description : Copy dynamic parameters into Unit structure
Parameters  : IN : Unit_p : device aimed
 *            IN : DynamicParams_p : access to data to set
Assumptions : Unit_p is not NULL (checked before function call)
 *            Unit_p is opened, VbiType is OK.
Limitations :
Returns     : none
*******************************************************************************/
static void UnitSetDynamicParams( VBI_t * const Unit_p, const STVBI_DynamicParams_t * const DynamicParams_p)
{
    switch (Unit_p->Params.Conf.VbiType)
    {
        case STVBI_VBI_TYPE_TELETEXT :
            STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Teletext dynamic configuration"));
            Unit_p->Params.Dyn.Type.TTX.Mask      = DynamicParams_p->Type.TTX.Mask;
            Unit_p->Params.Dyn.Type.TTX.LineCount = DynamicParams_p->Type.TTX.LineCount;
            Unit_p->Params.Dyn.Type.TTX.LineMask  = DynamicParams_p->Type.TTX.LineMask;
            Unit_p->Params.Dyn.Type.TTX.Field     = DynamicParams_p->Type.TTX.Field;
            break;
        case STVBI_VBI_TYPE_CLOSEDCAPTION :
            STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Closed caption dynamic configuration"));
            Unit_p->Params.Dyn.Type.CC.Mode = DynamicParams_p->Type.CC.Mode;
            Unit_p->Params.Dyn.Type.CC.LinesField1 = DynamicParams_p->Type.CC.LinesField1;
            Unit_p->Params.Dyn.Type.CC.LinesField2 = DynamicParams_p->Type.CC.LinesField2;
            break;
        case STVBI_VBI_TYPE_WSS : /* no configuration */
            STTBX_Report((STTBX_REPORT_LEVEL_INFO, "no Wss dynamic configuration"));
            break;
        case STVBI_VBI_TYPE_CGMS : /* no configuration */
            STTBX_Report((STTBX_REPORT_LEVEL_INFO, "no Cgms dynamic configuration"));
            break;
        case STVBI_VBI_TYPE_CGMSFULL : /* no configuration */
            STTBX_Report((STTBX_REPORT_LEVEL_INFO, "no CgmsFull dynamic configuration"));
            break;
        case STVBI_VBI_TYPE_VPS : /* no configuration */
            STTBX_Report((STTBX_REPORT_LEVEL_INFO, "no Vps dynamic configuration"));
            break;
    #ifdef COPYPROTECTION
        case STVBI_VBI_TYPE_MACROVISION : /* ok */
            STTBX_Report((STTBX_REPORT_LEVEL_INFO, "MV dynamic configuration"));
            Unit_p->Params.Dyn.Type.MV.PredefinedMode = DynamicParams_p->Type.MV.PredefinedMode;
            Unit_p->Params.Dyn.Type.MV.UserMode       = DynamicParams_p->Type.MV.UserMode;
            break;
    #endif /* #ifdef COPYPROTECTION */
        default :
            /* not handled, see assumptions */
            break;
    } /* end of switch (Unit_p->Params.Conf.VbiType) */
} /* End of UnitSetDynamicParams() function */

/*******************************************************************************
Name        : Report
Description : STTBX_REPORT information or error
Parameters  : IN : String : additional text to display,
              IN : ErrorCode : error number to report
Assumptions :
Limitations :
Returns     : none
*******************************************************************************/
static void Report( const char * const String, const ST_ErrorCode_t ErrorCode)
{
    UNUSED_PARAMETER(String);
    if (ErrorCode != ST_NO_ERROR)
    {
        STTBX_Print(("Error > %s failed, error %x \n", String, ErrorCode ));
    }
    else
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "%sDone", String));
    }
} /* end of Report */


/*
******************************************************************************
Public Functions
******************************************************************************
*/

/*
--------------------------------------------------------------------------------
Get revision of STVBI API
--------------------------------------------------------------------------------
*/

ST_Revision_t STVBI_GetRevision(void)
{

    /* Revision string format: STXXX-REL_x.x.x
                                 /       \ \ \__patch release number  }
                            API name      \ \__minor release number  } revision number
                                           \__major release number  }
    */
    return(VBI_Revision);
} /* End of STVBI_GetRevision() function */


/*
--------------------------------------------------------------------------------
Get capabilities of STVBI API
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STVBI_GetCapability(const ST_DeviceName_t DeviceName, STVBI_Capability_t * const Capability_p)
{
    S32 DeviceIndex = INVALID_DEVICE_INDEX;
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;

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
            GetCapability(&VBIDeviceArray[DeviceIndex], Capability_p);
        }
    }
    LeaveCriticalSection();

    Report("STVBI_GetCapability : ", ErrorCode);
    return(ErrorCode);
} /* End of STVBI_GetCapability() function */


/*
--------------------------------------------------------------------------------
Initialise STVBI driver
(Standard instances management. Driver specific implementations should be put in Init() function.)
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STVBI_Init (const ST_DeviceName_t DeviceName, const STVBI_InitParams_t* const InitParams_p)
{
    S32            Index = 0;
    VBI_t *        Unit_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if ((InitParams_p == NULL) ||                        /* There must be parameters */
        (InitParams_p->MaxOpen > STVBI_MAX_OPEN) ||      /* No more than MAX_OPEN open handles supported */
        (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1) || /* Device name length should be respected */
        ((((const char *) DeviceName)[0]) == '\0')       /* Device name should not be empty */
       )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    EnterCriticalSection();

    /* First time: initialise devices and units as empty */
    if ( !FirstInitDone)
    {
        for (Index = 0; Index < STVBI_MAX_DEVICE; Index++)
        {
            VBIDeviceArray[Index].DeviceName[0] = '\0';
            VBIDeviceArray[Index].VbiRegistered = 0;  /* unregistered all types */
            VBIDeviceArray[Index].DeviceType = 0;
        }

        for (Index = 0; Index < STVBI_MAX_UNIT; Index++)
        {
            VBIArray[Index].UnitValidity = 0;
            Unit_p = &VBIArray[Index];
            SET_VBI_TYPE(Unit_p, 0);         /* invalid this type */
            SET_VBI_STATUS(Unit_p, FALSE);   /* status : disabled */
            Unit_p->AreDynamicParamsWaiting = FALSE;
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
        while ((VBIDeviceArray[Index].DeviceName[0] != '\0') && (Index < STVBI_MAX_DEVICE))
        {
            Index++;
        }
        if (Index >= STVBI_MAX_DEVICE)
        {
            /* All devices initialised */
            ErrorCode = ST_ERROR_NO_MEMORY;
        }
        else
        {
            /* API specific initialisations */
            ErrorCode = Init(&VBIDeviceArray[Index], InitParams_p);

            if (ErrorCode == ST_NO_ERROR)
            {
                /* Device available and successfully initialised: register device name */
                strcpy(VBIDeviceArray[Index].DeviceName, DeviceName);
                VBIDeviceArray[Index].DeviceName[ST_MAX_DEVICE_NAME - 1] = '\0';
                VBIDeviceArray[Index].MaxOpen = InitParams_p->MaxOpen;

                STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Device '%s' initialised", VBIDeviceArray[Index].DeviceName));
            }
        } /* End exists device not initialised */
    } /* End device not already initialised */




    LeaveCriticalSection();
    return(ErrorCode);
} /* End of STVBI_Init() function */



/*
--------------------------------------------------------------------------------
STVBI_Open
(Standard instances management. Driver specific implementations should be put in Open() function.)
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STVBI_Open( const ST_DeviceName_t       DeviceName,
                           const STVBI_OpenParams_t*   const OpenParams_p,
                           STVBI_Handle_t*             const Handle_p
                         )
{
    S32 DeviceIndex = INVALID_DEVICE_INDEX, UnitIndex, Index;
    U32 OpenedUnitForThisInit;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if ((OpenParams_p == NULL) ||                       /* There must be parameters ! */
        (Handle_p == NULL) ||                           /* Pointer to handle should be valid ! */
        (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1) || /* Device name length should be respected */
        ((((const char *) DeviceName)[0]) == '\0')
       )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
    /* OpenParams_p is not NULL, we can now check pointed structure values */
    if (CheckVbiOpenParams(OpenParams_p) != ST_NO_ERROR)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    EnterCriticalSection();

    if (! FirstInitDone)
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
            UnitIndex = STVBI_MAX_UNIT;
            OpenedUnitForThisInit = 0;
            for (Index = 0; Index < STVBI_MAX_UNIT; Index++)
            {
                if ((VBIArray[Index].UnitValidity == STVBI_VALID_UNIT) && (VBIArray[Index].Device_p == &VBIDeviceArray[DeviceIndex]))
                {
                    OpenedUnitForThisInit ++;
                }
                if (VBIArray[Index].UnitValidity != STVBI_VALID_UNIT)
                {
                    /* Found a free handle structure */
                    UnitIndex = Index;
                }
            }
            if ((OpenedUnitForThisInit >= VBIDeviceArray[DeviceIndex].MaxOpen) || (UnitIndex >= STVBI_MAX_UNIT))
            {
                /* None of the units is free or MaxOpen reached */
                ErrorCode = ST_ERROR_NO_FREE_HANDLES;
            }
            else
            {
                *Handle_p = (STVBI_Handle_t) &VBIArray[UnitIndex];
                VBIArray[UnitIndex].Device_p = &VBIDeviceArray[DeviceIndex];

                /* API specific actions after opening */
                ErrorCode = Open(&VBIArray[UnitIndex], OpenParams_p);

                if (ErrorCode == ST_NO_ERROR)
                {
                    /* Register opened handle */
                    VBIArray[UnitIndex].UnitValidity = STVBI_VALID_UNIT;

                    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Handle opened on device '%s'", DeviceName));
                }
            } /* End found unit unused */
        } /* End device valid */
    } /* End FirstInitDone */

    LeaveCriticalSection();

    return(ErrorCode);
} /* End fo STVBI_Open() function */

/*
--------------------------------------------------------------------------------
Close ...
(Standard instances management. Driver specific implementations should be put in Close() function.)
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t  STVBI_Close(const STVBI_Handle_t Handle)
{
    VBI_t * Unit_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    EnterCriticalSection();

    if (! FirstInitDone)
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
            Unit_p = (VBI_t *) Handle;

            /* API specific actions before closing */
            ErrorCode = Close(Unit_p);

            /* Close only if no errors */
            if (ErrorCode == ST_NO_ERROR)
            {
                /* Un-register opened handle */
                Unit_p->UnitValidity = 0;
                UNREGISTER_VBI(Unit_p);                     /* unregistered this vbi */
                SET_VBI_TYPE(Unit_p, STVBI_VBI_TYPE_NONE);  /* invalid this type */
                SET_VBI_STATUS(Unit_p, FALSE);              /* vbi disabled by default */

                STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Handle closed on device '%s'", Unit_p->Device_p->DeviceName));
            }
        } /* End handle valid */
    } /* End FirstInitDone */

    LeaveCriticalSection();

    return(ErrorCode);
} /* End of STVBI_Close() function */


/*
--------------------------------------------------------------------------------
Terminate STVBI driver
(Standard instances management. Driver specific implementations should be put in Term() function.)
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t  STVBI_Term(const ST_DeviceName_t DeviceName, const STVBI_TermParams_t* const TermParams_p)
{
    VBI_t *Unit_p;
    S32 DeviceIndex = INVALID_DEVICE_INDEX, UnitIndex;
    BOOL Found = FALSE;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if ((TermParams_p == NULL) ||                        /* There must be parameters ! */
        (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1) || /* Device name length should be respected */
        ((((const char *) DeviceName)[0]) == '\0')       /* Device name should not be empty */
       )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    EnterCriticalSection();
    if (! FirstInitDone)
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
            /* Found = FALSE;*/
            UnitIndex = 0;
            Unit_p = VBIArray;
            while ((UnitIndex < STVBI_MAX_UNIT) && (!Found))
            {
                Found = ((Unit_p->Device_p == &VBIDeviceArray[DeviceIndex]) && (Unit_p->UnitValidity == STVBI_VALID_UNIT));
                Unit_p++;
                UnitIndex++;
            }

            if (Found)
            {
                if (TermParams_p->ForceTerminate)
                {
                    UnitIndex = 0;
                    Unit_p = VBIArray;
                    while (UnitIndex < STVBI_MAX_UNIT)
                    {
                        if ((Unit_p->Device_p == &VBIDeviceArray[DeviceIndex]) && (Unit_p->UnitValidity == STVBI_VALID_UNIT))
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
                            UNREGISTER_VBI(Unit_p);                     /* unregistered this vbi */
                            SET_VBI_TYPE(Unit_p, STVBI_VBI_TYPE_NONE);  /* invalid this type */
                            SET_VBI_STATUS(Unit_p, FALSE);              /* vbi disabled by default */

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
                ErrorCode = Term(&VBIDeviceArray[DeviceIndex]);
                /* Don't leave instance semi-terminated: terminate as much as possible */
                /* free device */
                VBIDeviceArray[DeviceIndex].DeviceName[0] = '\0';

                STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Device '%s' terminated", DeviceName));
            } /* End terminate OK */
        } /* End valid device */
    } /* End FirstInitDone */

    LeaveCriticalSection();

    return(ErrorCode);
} /* End of STVBI_Term() function */


/*****************************************************************************
 * VBI specific functions
 *****************************************************************************/

/*
--------------------------------------------------------------------------------
STVBI_Disable
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STVBI_Disable(const STVBI_Handle_t Handle)
{
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;
    VBI_t           *Unit_p;

    if (!IsValidHandle(Handle))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        Unit_p = (VBI_t *)Handle;
        if ( !GET_VBI_STATUS(Unit_p) )
        {
            ErrorCode = STVBI_ERROR_VBI_NOT_ENABLE;
        }
        else
        {
            SET_VBI_STATUS(Unit_p, FALSE);
            ErrorCode = stvbi_HALDisable(Unit_p);

        }
    }
    Report("STVBI_Disable : ", ErrorCode);
    return(ErrorCode);
}

/*
--------------------------------------------------------------------------------
STVBI_Enable
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STVBI_Enable(const STVBI_Handle_t Handle)
{
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;
    VBI_t           *Unit_p;

    if (!IsValidHandle(Handle))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        Unit_p = (VBI_t *)Handle;
        if (GET_VBI_STATUS(Unit_p) == TRUE)
        {
            ErrorCode = STVBI_ERROR_VBI_ENABLE;
        }
        else
        {
            SET_VBI_STATUS(Unit_p, TRUE);

                        ErrorCode = stvbi_HALEnable(Unit_p);
                        /* if needed, configure hardware now service is enabled */
                        if ( (ErrorCode == ST_NO_ERROR) && (Unit_p->AreDynamicParamsWaiting))
                        {
                            ErrorCode = stvbi_HALSetDynamicParams(Unit_p);
                            Unit_p->AreDynamicParamsWaiting = FALSE;
                        }


        }
    }
    Report("STVBI_Enable : ", ErrorCode);
    return(ErrorCode);
} /* End of STVBI_Enable() function */

/*
--------------------------------------------------------------------------------
STVBI_GetConfiguration
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STVBI_GetConfiguration(const STVBI_Handle_t Handle, STVBI_Configuration_t * const Configuration_p)
{
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;
    VBI_t           *Unit_p;

    if (!IsValidHandle(Handle))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else if (Configuration_p == NULL)
    {
        ErrorCode = ST_ERROR_BAD_PARAMETER;
    }

    if (ErrorCode == ST_NO_ERROR)
    {
        Unit_p = (VBI_t *)Handle;

        Configuration_p->VbiType = Unit_p->Params.Conf.VbiType;
        switch (Unit_p->Params.Conf.VbiType)
        {
            case STVBI_VBI_TYPE_TELETEXT : /* ok */
                STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Teletext configuration"));
                Configuration_p->Type.TTX.Latency   = Unit_p->Params.Conf.Type.TTX.Latency;
                Configuration_p->Type.TTX.FullPage  = Unit_p->Params.Conf.Type.TTX.FullPage;
                Configuration_p->Type.TTX.Amplitude = Unit_p->Params.Conf.Type.TTX.Amplitude;
                Configuration_p->Type.TTX.Standard  = Unit_p->Params.Conf.Type.TTX.Standard;
                break;
            case STVBI_VBI_TYPE_CLOSEDCAPTION : /* ok */
                STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Closed caption configuration"));
                Configuration_p->Type.CC.Mode = Unit_p->Params.Conf.Type.CC.Mode;
                break;
            case STVBI_VBI_TYPE_WSS : /* no configuration */
                STTBX_Report((STTBX_REPORT_LEVEL_INFO, "no Wss configuration"));
                break;
            case STVBI_VBI_TYPE_CGMS : /* no configuration */
                STTBX_Report((STTBX_REPORT_LEVEL_INFO, "no Cgms configuration"));
                break;
            case STVBI_VBI_TYPE_CGMSFULL : /* no configuration */
                Configuration_p->Type.CGMSFULL.CGMSFULLStandard  = Unit_p->Params.Conf.Type.CGMSFULL.CGMSFULLStandard;
                break;
            case STVBI_VBI_TYPE_VPS : /* no configuration */
                STTBX_Report((STTBX_REPORT_LEVEL_INFO, "no Vps configuration"));
                break;
        #ifdef COPYPROTECTION
            case STVBI_VBI_TYPE_MACROVISION : /* ok */
                STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Mv configuration"));
                Configuration_p->Type.MV.Algorithm = Unit_p->Params.Conf.Type.MV.Algorithm;
                break;
        #endif /* #ifdef COPYPROTECTION */
            default :
                ErrorCode = ST_ERROR_BAD_PARAMETER; /* should not be possible */
                break;
        }
    }
    Report("STVBI_GetConfiguration : ", ErrorCode);
    return(ErrorCode);
} /* end of STVBI_GetConfiguration */


/*
--------------------------------------------------------------------------------
STVBI_SetDynamicParams
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STVBI_SetDynamicParams(const STVBI_Handle_t Handle, const STVBI_DynamicParams_t* const DynamicParams_p)
{
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;
    VBI_t           *Unit_p;

    if (!IsValidHandle(Handle))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else if (DynamicParams_p == NULL)
    {
        ErrorCode = ST_ERROR_BAD_PARAMETER;
    }

    if (ErrorCode == ST_NO_ERROR)
    {
        Unit_p = (VBI_t *)Handle;
        ErrorCode = CheckVbiDynamicParams( Unit_p, DynamicParams_p);
        if ( ErrorCode == ST_NO_ERROR)
        {
            UnitSetDynamicParams(Unit_p, DynamicParams_p);
            /* configure hardware only if service enabled */
            if (Unit_p->IsVbiEnabled)
            {
                ErrorCode = stvbi_HALSetDynamicParams(Unit_p);
            }
            else
            {
                Unit_p->AreDynamicParamsWaiting = TRUE;
            }
        }
    }
    Report("STVBI_SetDynamicParams : ", ErrorCode);
    return(ErrorCode);
} /* End of STVBI_SetDynamicParams() function */

/*
--------------------------------------------------------------------------------
STVBI_WriteData
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STVBI_WriteData (const STVBI_Handle_t Handle, const U8* const Data_p, const U8 Length)
{
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;
    VBI_t *         Unit_p;

    if (!IsValidHandle(Handle))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        Unit_p = (VBI_t *)Handle;
        if ( (Data_p == NULL) || !IsLengthOk(Unit_p, Length))
        {
            ErrorCode = ST_ERROR_BAD_PARAMETER;
        }
        else
        {
            ErrorCode = stvbi_HALWriteData(Unit_p, Data_p);
        }
    }
    Report("STVBI_WriteData : ", ErrorCode);
    return(ErrorCode);
} /* end of STVBI_WriteData() function */


/*
--------------------------------------------------------------------------------
STVBI_SetTeletextSource
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t  STVBI_SetTeletextSource (
                const STVBI_Handle_t          Handle,
                const STVBI_TeletextSource_t  TeletextSource)
{
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;
    VBI_t           *Unit_p = NULL;

    if (!IsValidHandle(Handle))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        Unit_p = (VBI_t *)Handle;
        switch (TeletextSource)
        {
            case STVBI_TELETEXT_SOURCE_DMA : /* no break */
                switch (Unit_p->Device_p->DeviceType)
                {
                    case STVBI_DEVICE_TYPE_DENC :
                        /* nothing to do */
                        break;
                    case STVBI_DEVICE_TYPE_GX1 :
                        ErrorCode = stvbi_HALSetTeletextSource( Unit_p, TeletextSource);
                        break;
                    default :
                        /* filtered at init */
                        break;
                }
                break;
            case STVBI_TELETEXT_SOURCE_PIN :
                switch (Unit_p->Device_p->DeviceType)
                {
                    case STVBI_DEVICE_TYPE_DENC :
                        ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
                        break;
                    case STVBI_DEVICE_TYPE_GX1 :
                        ErrorCode = stvbi_HALSetTeletextSource( Unit_p, TeletextSource);
                        break;
                    default :
                        /* filtered at init */
                        break;
                }
                break;
            default :
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                break;
        } /* switch (TeletextSource) */
    }
    if (ErrorCode == ST_NO_ERROR)
    {
        Unit_p->Device_p->TeletextSource = TeletextSource;
    }
    return(ErrorCode);
} /* end of STVBI_SetTeletextSource() function */


/*
--------------------------------------------------------------------------------
STVBI_GetTeletextSource
--------------------------------------------------------------------------------
*/

ST_ErrorCode_t  STVBI_GetTeletextSource (
                const STVBI_Handle_t          Handle,
                STVBI_TeletextSource_t *      const TeletextSource_p)
{
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;
    VBI_t           *Unit_p;

    if (!IsValidHandle(Handle))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else if (TeletextSource_p == NULL)
    {
        ErrorCode = ST_ERROR_BAD_PARAMETER;
    }

    if (ErrorCode == ST_NO_ERROR)
    {
        Unit_p = (VBI_t *)Handle;
        *TeletextSource_p = Unit_p->Device_p->TeletextSource;
    }
    return(ErrorCode);

} /* end of STVBI_GetTeletextSource() function */

/* End of stvbi.c */
