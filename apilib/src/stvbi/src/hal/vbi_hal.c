/*******************************************************************************

File name   : vbi_hal.c

Description : vbi interface with hal source file

COPYRIGHT (C) STMicroelectronics 2001.

Date               Modification                                      Name
----               ------------                                      ----
22 Feb 2001        Created                                           HSdLM
04 Jul 2001        Non-API exported symbols begin with stvbi_        HSdLM
 *                 New function to check dynamic parameters
14 Nov 2001        New DeviceType STVBI_DEVICE_TYPE_GX1 for ST40GX1  HSdLM
 *                 New function to select TTX source on ST40GX1
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */


/* Includes ----------------------------------------------------------------- */
/*#if !defined(ST_OSLINUX)*/
#include "stsys.h"
/*#endif*/

#include "stvbi.h"
#include "vbi_drv.h"
#include "vbi_hal.h"
#include "ondenc.h"

#if defined (ST_7710)||defined (ST_7109)||defined (ST_7100)||defined (ST_7200)
#include "vbi_vpreg.h"
#endif





/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */

/* Datasheet ST40GFX1 Video Subsystem Output Functional Specification
 * ADCS 7216440                                                               */
#define PIXCLKCON_CONTROL           0x08 /* register offset from PIXCLKCON base address */
#define PIXCLKCON_CONTROL_TTXTSRC   0x02 /* TTXTSRC bit mask in CONTROL register */

/* Private Variables (static)------------------------------------------------ */

/* Global Variables --------------------------------------------------------- */

/* Private Macros ----------------------------------------------------------- */

/* Private Function prototypes ---------------------------------------------- */

/* Functions ---------------------------------------------------------------- */

#ifdef COPYPROTECTION
/*******************************************************************************
Name        : stvbi_HALInitCopyProtection
Description : Init Copy Protection feature
Parameters  : pointer on unit
Assumptions : pointer on unit is not NULL
Limitations :
Returns     : ST_NO_ERROR, STVBI_ERROR_DENC_ACCESS
*******************************************************************************/
ST_ErrorCode_t stvbi_HALInitCopyProtection(VBI_t * const Unit_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    switch (Unit_p->Device_p->DeviceType)
    {
        case STVBI_DEVICE_TYPE_DENC : /* no break; */
        case STVBI_DEVICE_TYPE_GX1 :
            ErrorCode = stvbi_HALInitCopyProtectionOnDenc(Unit_p);
            break;
        default :
            /* not handled, see assumptions */
            break;
    }
    return(ErrorCode);
} /* end of stvbi_HALInitCopyProtection() function */
#endif /* #ifdef COPYPROTECTION */

/*******************************************************************************
Name        : stvbi_HALSetTeletextConfiguration
Description : Set teletext configuration
Parameters  : pointer on unit
Assumptions : pointer on unit is not NULL
Limitations :
Returns     : ST_NO_ERROR, STVBI_ERROR_DENC_ACCESS
*******************************************************************************/
ST_ErrorCode_t stvbi_HALSetTeletextConfiguration(VBI_t * const Unit_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    switch (Unit_p->Device_p->DeviceType)
    {
        case STVBI_DEVICE_TYPE_DENC : /* no break; */
        case STVBI_DEVICE_TYPE_GX1 :
            ErrorCode = stvbi_HALSetTeletextConfigurationOnDenc(Unit_p);
            break;
        default :
            /* not handled, see assumptions */
            break;
    }
    return(ErrorCode);
} /* end of stvbi_HALSetTeletextConfiguration() function */


/*******************************************************************************
Name        : stvbi_HALEnable
Description : Enable VBI.
Parameters  : IN : Unit_p : device aimed
Assumptions : Unit_p is not NULL (checked before function call)
 *            Unit_p is opened, DeviceType and VbiType are OK.
Limitations :
Returns     : ST_NO_ERROR, STVBI_ERROR_DENC_ACCESS
*******************************************************************************/
ST_ErrorCode_t stvbi_HALEnable( VBI_t * const Unit_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "enable type %d", Unit_p->Params.Conf.VbiType));
    switch (Unit_p->Device_p->DeviceType)
    {
        case STVBI_DEVICE_TYPE_DENC : /* no break; */
        case STVBI_DEVICE_TYPE_GX1 :
            switch (Unit_p->Params.Conf.VbiType)
            {
                case STVBI_VBI_TYPE_TELETEXT : /* ok */
                    ErrorCode = stvbi_HALEnableTeletextOnDenc(Unit_p);
                    break;
                case STVBI_VBI_TYPE_CLOSEDCAPTION : /* ok */
                    ErrorCode = stvbi_HALEnableClosedCaptionOnDenc(Unit_p);
                    break;
                case STVBI_VBI_TYPE_WSS : /* ok */
                    ErrorCode = stvbi_HALEnableWssOnDenc(Unit_p);
                    break;
                case STVBI_VBI_TYPE_CGMS : /* ok */
                    ErrorCode = stvbi_HALEnableCgmsOnDenc(Unit_p);
                    break;
                case STVBI_VBI_TYPE_VPS : /* ok */
                    ErrorCode = stvbi_HALEnableVpsOnDenc(Unit_p);
                    break;
            #ifdef COPYPROTECTION
                case STVBI_VBI_TYPE_MACROVISION : /* ok */
                    ErrorCode = stvbi_HALEnableCopyProtectionOnDenc(Unit_p);
                    break;
            #endif /* #ifdef COPYPROTECTION */
                default :
                    /* not handled, see assumptions */
                    break;
            } /* end of switch (Unit_p->Params.Conf.VbiType) */
            break;

        case STVBI_DEVICE_TYPE_VIEWPORT :
            #if defined (ST_7710)||defined (ST_7109)||defined (ST_7100)||defined (ST_7200)
               ErrorCode = STLAYER_EnableViewPort(Unit_p->Device_p->VPHandle) ;
            #endif
            break;

        default :
            /* not handled, see assumptions */
            break;
    } /* end of switch (Unit_p->Device_p->DeviceType) */
    return( ErrorCode);
} /* End of stvbi_HALEnable() function */


/*******************************************************************************
Name        : stvbi_HALDisable
Description : Disable VBI.
Parameters  : IN : Unit_p : device aimed
Assumptions : Unit_p is not NULL (checked before function call)
 *            Unit_p is opened, DeviceType and VbiType are OK.
Limitations :
Returns     : ST_NO_ERROR, STVBI_ERROR_DENC_ACCESS
*******************************************************************************/
ST_ErrorCode_t stvbi_HALDisable( VBI_t * const Unit_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "disable type %d", Unit_p->Params.Conf.VbiType));
    switch (Unit_p->Device_p->DeviceType)
    {
        case STVBI_DEVICE_TYPE_DENC : /* no break; */
        case STVBI_DEVICE_TYPE_GX1 :
            switch (Unit_p->Params.Conf.VbiType)
            {
                case STVBI_VBI_TYPE_TELETEXT : /* ok */
                    ErrorCode = stvbi_HALDisableTeletextOnDenc(Unit_p);
                    break;
                case STVBI_VBI_TYPE_CLOSEDCAPTION : /* ok */
                    ErrorCode = stvbi_HALDisableClosedCaptionOnDenc(Unit_p);
                    break;
                case STVBI_VBI_TYPE_WSS : /* ok */
                    ErrorCode = stvbi_HALDisableWssOnDenc(Unit_p);
                    break;
                case STVBI_VBI_TYPE_CGMS : /* ok */
                    ErrorCode = stvbi_HALDisableCgmsOnDenc(Unit_p);
                    break;
                case STVBI_VBI_TYPE_VPS : /* ok */
                    ErrorCode = stvbi_HALDisableVpsOnDenc(Unit_p);
                    break;
            #ifdef COPYPROTECTION
                case STVBI_VBI_TYPE_MACROVISION : /* ok */
                    ErrorCode = stvbi_HALDisableCopyProtectionOnDenc(Unit_p);
                    break;
            #endif /* #ifdef COPYPROTECTION */
                default :
                    /* not handled, see assumptions */
                    break;
            } /* end of switch (Unit_p->Params.Conf.VbiType) */
            break;

        case STVBI_DEVICE_TYPE_VIEWPORT :
            #if defined (ST_7710)||defined (ST_7109)||defined (ST_7100)||defined (ST_7200)
               ErrorCode = STLAYER_DisableViewPort(Unit_p->Device_p->VPHandle) ;
            #endif

            break;
        default :
            /* not handled, see assumptions */
            break;
    } /* end of switch (Unit_p->Device_p->DeviceType) */
    return( ErrorCode);
} /* End of stvbi_HALDisable() function */

/*******************************************************************************
Name        : stvbi_HALCheckDynamicParams
Description : check if parameters given are valid.
Parameters  : IN : Unit_p : device aimed
 *            IN : DynamicParams_p : parameters to be checked
Assumptions : Unit_p and DynamicParams_p are not NULL (checked before function call)
 *            Unit_p is opened, VbiType and DeviceType are OK.
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_BAD_PARAMETER
*******************************************************************************/
ST_ErrorCode_t stvbi_HALCheckDynamicParams( const VBI_t * const Unit_p, const STVBI_DynamicParams_t * const DynamicParams_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    switch (Unit_p->Device_p->DeviceType)
    {
        case STVBI_DEVICE_TYPE_DENC : /* no break; */
        case STVBI_DEVICE_TYPE_GX1 :
            switch (Unit_p->Params.Conf.VbiType)
            {
                case STVBI_VBI_TYPE_CLOSEDCAPTION : /* ok */
                    ErrorCode = stvbi_HALCheckClosedCaptionDynamicParamsOnDenc( Unit_p, DynamicParams_p);
                    break;
            #ifdef COPYPROTECTION
                case STVBI_VBI_TYPE_MACROVISION : /* no break */
            #endif /* #ifdef COPYPROTECTION */
                case STVBI_VBI_TYPE_TELETEXT :    /* no break */
                case STVBI_VBI_TYPE_WSS :         /* no break */
                case STVBI_VBI_TYPE_CGMS :        /* no break */
                case STVBI_VBI_TYPE_VPS :         /* no break */
                default :
                    /* not handled, see assumptions */
                    break;
            } /* end of switch (Unit_p->Params.Conf.VbiType) */
            break;


        default :
            /* not handled, see assumptions */
            break;
    } /* end of switch (Unit_p->Device_p->DeviceType) */
    return( ErrorCode);
} /* End of stvbi_HALCheckDynamicParams() function */

/*******************************************************************************
Name        : stvbi_HALSetDynamicParams
Description : Copy dynamic parameters into hardware registers
Parameters  : IN : Unit_p : device aimed
Assumptions : Unit_p is not NULL (checked before function call)
 *            Unit_p is opened, VbiType and DeviceType are OK.
Limitations :
Returns     : ST_NO_ERROR, STVBI_ERROR_DENC_ACCESS
*******************************************************************************/
ST_ErrorCode_t stvbi_HALSetDynamicParams( VBI_t * const Unit_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    switch (Unit_p->Device_p->DeviceType)
    {
        case STVBI_DEVICE_TYPE_DENC : /* no break; */
        case STVBI_DEVICE_TYPE_GX1 :
            switch (Unit_p->Params.Conf.VbiType)
            {
                case STVBI_VBI_TYPE_TELETEXT : /* ok */
                    ErrorCode = stvbi_HALSetTeletextDynamicParamsOnDenc( Unit_p);
                    break;
                case STVBI_VBI_TYPE_CLOSEDCAPTION : /* ok */
                    ErrorCode = stvbi_HALSetClosedCaptionDynamicParamsOnDenc( Unit_p);
                    break;
                case STVBI_VBI_TYPE_WSS : /* no configuration */
                    break;
                case STVBI_VBI_TYPE_CGMS : /* no configuration */
                    break;
                case STVBI_VBI_TYPE_VPS : /* no configuration */
                    break;
            #ifdef COPYPROTECTION
                case STVBI_VBI_TYPE_MACROVISION : /* ok */
                    ErrorCode = stvbi_HALSetCopyProtectionDynamicParamsOnDenc( Unit_p);
                    break;
            #endif /* #ifdef COPYPROTECTION */
                default :
                    /* not handled, see assumptions */
                    break;
            } /* end of switch (Unit_p->Params.Conf.VbiType) */
            break;
        default :
            /* not handled, see assumptions */
            break;
    } /* end of switch (Unit_p->Device_p->DeviceType) */
    return( ErrorCode);
} /* End of stvbi_HALSetDynamicParams() function */


/*******************************************************************************
Name        : stvbi_HALWriteData
Description : Write data into hardware registers
Parameters  : IN : Unit_p : device aimed
 *            IN : Data_p : data to be written
Assumptions : Unit_p is not NULL (checked before function call)
 *            Unit_p is opened, VbiType and DeviceType are OK.
Limitations :
Returns     : ST_NO_ERROR, STVBI_ERROR_DENC_ACCESS
*******************************************************************************/
ST_ErrorCode_t stvbi_HALWriteData( VBI_t * const Unit_p, const U8* const Data_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    switch (Unit_p->Device_p->DeviceType)
    {
        case STVBI_DEVICE_TYPE_DENC : /* no break; */
        case STVBI_DEVICE_TYPE_GX1 :
            switch (Unit_p->Params.Conf.VbiType)
            {
                case STVBI_VBI_TYPE_TELETEXT : /* no data to write */
                    break;
                case STVBI_VBI_TYPE_CLOSEDCAPTION : /* ok */
                    ErrorCode = stvbi_HALWriteClosedCaptionDataOnDenc(Unit_p, Data_p);
                    break;
                case STVBI_VBI_TYPE_WSS : /* ok */
                    ErrorCode = stvbi_HALWriteWssDataOnDenc(Unit_p, Data_p);
                    break;
                case STVBI_VBI_TYPE_CGMS : /* ok */
                    ErrorCode = stvbi_HALWriteCgmsDataOnDenc(Unit_p, Data_p);
                    break;
                case STVBI_VBI_TYPE_VPS : /* ok */
                    ErrorCode = stvbi_HALWriteVpsDataOnDenc(Unit_p, Data_p);
                    break;
            #ifdef COPYPROTECTION
                case STVBI_VBI_TYPE_MACROVISION : /* ok */
                    ErrorCode = stvbi_HALWriteCopyProtectionDataOnDenc(Unit_p, Data_p);
                    break;
            #endif /* #ifdef COPYPROTECTION */

                default :
                    /* not handled, see assumptions */
                    break;
            } /* end of switch (Unit_p->Params.Conf.VbiType) */
            break;
          case STVBI_DEVICE_TYPE_VIEWPORT :

            #if defined (ST_7710)||defined (ST_7109)||defined (ST_7100)||defined (ST_7200)

                switch (Unit_p->Params.Conf.VbiType)
                {
                    case STVBI_VBI_TYPE_CGMSFULL : /* Write data on Viewport */
                        switch (Unit_p->Params.Conf.Type.CGMSFULL.CGMSFULLStandard)
                        {
                            case STVBI_CGMS_HD_1080I60000 :
                            case STVBI_CGMS_HD_720P60000  :
                            case STVBI_CGMS_SD_480P60000  :
                                ErrorCode = stvbi_HalWriteDataOnFullCgmsViewport(Unit_p, Data_p);
                                break;

                            case STVBI_CGMS_TYPE_B_HD_1080I60000 :
                            case STVBI_CGMS_TYPE_B_HD_720P60000  :
                            case STVBI_CGMS_TYPE_B_SD_480P60000  :
                                ErrorCode = stvbi_HalWriteDataOnFullCgmsViewportTypeB(Unit_p, Data_p);
                                break;

                            default:
                                break;
                        }
                        break;
                    default :
                        /* not handled, see assumptions */
                        break;
                }
            #endif
            break;

          default :
            /* not handled, see assumptions */
            break;
    }/* end of switch (Unit_p->Device_p->DeviceType) */

    return( ErrorCode);
} /* End of stvbi_HALWriteData() function */


/*******************************************************************************
Name        : stvbi_HALSetTeletextSource
Description :
Parameters  : IN : Unit_p : device aimed
 *            IN : TeletextSource : select source of teletext incoming data
Assumptions : Unit_p is not NULL (checked before function call)
 *            Unit_p is opened, TeletextSource is OK.
Limitations :
Returns     : ST_NO_ERROR
*******************************************************************************/
ST_ErrorCode_t stvbi_HALSetTeletextSource(   const VBI_t *                 const Unit_p,
                                             const STVBI_TeletextSource_t  TeletextSource)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    U32 RegAddr, RegValue;

    switch (Unit_p->Device_p->DeviceType)
    {
        case STVBI_DEVICE_TYPE_DENC :
            ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
            break;
        case STVBI_DEVICE_TYPE_GX1 :
            switch (Unit_p->Params.Conf.VbiType)
            {
                case STVBI_VBI_TYPE_TELETEXT :
                    RegAddr = (U32)Unit_p->Device_p->BaseAddress_p + PIXCLKCON_CONTROL;
                    switch (TeletextSource)
                    {
                        case STVBI_TELETEXT_SOURCE_DMA :
                            RegValue = STSYS_ReadRegDev32LE((void*)RegAddr);
                            STSYS_WriteRegDev32LE(RegAddr, (RegValue & (U32)~PIXCLKCON_CONTROL_TTXTSRC));
                            break;
                        case STVBI_TELETEXT_SOURCE_PIN :
                            RegValue = STSYS_ReadRegDev32LE((void*)RegAddr);
                            STSYS_WriteRegDev32LE(RegAddr, (RegValue | PIXCLKCON_CONTROL_TTXTSRC));
                            break;
                        default :
                            ErrorCode = ST_ERROR_BAD_PARAMETER;
                            break;
                    } /* switch (TeletextSource) */
                    break;
                case STVBI_VBI_TYPE_CLOSEDCAPTION : /* no break */
                case STVBI_VBI_TYPE_WSS :           /* no break */
                case STVBI_VBI_TYPE_CGMS :          /* no break */
                case STVBI_VBI_TYPE_VPS :           /* no break */
            #ifdef COPYPROTECTION
                case STVBI_VBI_TYPE_MACROVISION :   /* no break */
            #endif /* #ifdef COPYPROTECTION */
                default :
                    ErrorCode = ST_ERROR_BAD_PARAMETER;
                    break;
            } /* end of switch (Unit_p->Params.Conf.VbiType) */
            break;
        default :
            ErrorCode = ST_ERROR_BAD_PARAMETER;
            break;
    } /* end of switch (Unit_p->Device_p->DeviceType) */
    return( ErrorCode);

} /* end stvbi_HALSetTeletextSource() */


/* End of vbi_hal.c */
