/*******************************************************************************

File name   : vtgi_hal.c

Description : vtg interface with hal source file

COPYRIGHT (C) STMicroelectronics 2003.

Date               Modification                                     Name
----               ------------                                     ----
27 Sep 2000        Created                                          HSdLM
22 Nov 2000        New routine for error tracking                   HSdLM
                   and remove path in header includes.
15 Feb 2001        Use new DeviceType values                        HSdLM
21 Mar 2001        Remove HALIVTG_SetInterrupt(), move interrupt    HSdLM
 *                 init in HALIVTG_Init(), call HALVTG_InitClockGen()
 *                 and HALVTG_SetSyncInSettings()
 *                 New HALIVTG_SetOutput() to disable signals
28 Jun 2001        Re-organization : split vtg_hal2 in vtg_hal2,    HSdLM
 *                 vos_hal and vfe_hal. Set prefix stvtg_ on
 *                 exported symbols
04 Sep 2001        Set target conditional build                     HSdLM
16 Apr 2002        New DeviceType for STi7020                       HSdLM
 *                 Add STVTG_VSYNC_WITHOUT_VIDEO option
16 Apr 2003        New DeviceType for STi5528                       HSdLM
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */


/* Includes ----------------------------------------------------------------- */

#include "stvtg.h"
#include "vtg_drv.h"
#include "vtg_ihal.h"
#include "vtg_hal1.h"

#if defined(STVTG_VOS) || defined(STVTG_VFE) || defined (STVTG_VFE2)
#include "vtg_hal2.h"
#endif /* #if defined(STVTG_VOS) || defined(STVTG_VFE) || defined (STVTG_VFE2)*/

#ifdef STVTG_VOS
#include "hal_vos.h"
#include "clk_gen.h"
#endif /* #ifdef STVTG_VOS */

#if defined (STVTG_VFE) || defined (STVTG_VFE2)
#include "hal_vfe.h"
#endif /* #ifdef STVTG_VFE || VFE2*/

#ifdef STVTG_VSYNC_WITHOUT_VIDEO
#include "vtg_vsit.h"
#endif /* #ifdef STVTG_VSYNC_WITHOUT_VIDEO */


/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */
 #ifdef ST_5528
 #define ENA_PWM
 #endif
/* Private Variables (static)------------------------------------------------ */

/* Global Variables --------------------------------------------------------- */

/* Private Macros ----------------------------------------------------------- */

/* Private Function prototypes ---------------------------------------------- */

/* Functions ---------------------------------------------------------------- */

/*******************************************************************************
Name        : stvtg_IHALInit
Description : make device dependant initializations
Parameters  : Device_p : IN/OUT : Device associated data to be updated
 *            InitParams_p : IN : Init parameters used for HAL init.
Assumptions : Device_p not NULL
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_BAD_PARAMETER, STVTG_ERROR_DENC_ACCESS
 *            ST_ERROR_INTERRUPT_INSTALL, STVTG_ERROR_EVT_SUBSCRIBE
*******************************************************************************/
ST_ErrorCode_t stvtg_IHALInit(  stvtg_Device_t * const Device_p, const STVTG_InitParams_t * const InitParams_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    switch (Device_p->DeviceType)
    {
        case STVTG_DEVICE_TYPE_DENC :
            stvtg_HALInitVtgInDenc(Device_p);
            stvtg_HALSetCapabilityOnDenc(Device_p);
            ErrorCode = stvtg_HALUpdateTimingParameters(Device_p, STVTG_TIMING_MODE_SLAVE);
#ifdef STVTG_VSYNC_WITHOUT_VIDEO
            if (ErrorCode== ST_NO_ERROR)
            {
                ErrorCode = stvtg_ActivateVideoVsyncInterrupt(Device_p, InitParams_p);
            }
#endif /* #ifdef STVTG_VSYNC_WITHOUT_VIDEO */
            break;
#if defined (STVTG_VFE) || defined(STVTG_VFE2)
        case STVTG_DEVICE_TYPE_VFE : /* no break */
        case STVTG_DEVICE_TYPE_VFE2:
            if ((ErrorCode = stvtg_HALInit(Device_p, InitParams_p)) == ST_NO_ERROR)
            {
                #ifdef ENA_PWM
                stvtg_HALEnablePWMclock(Device_p);
                #endif
                stvtg_HALVfeSetCapability(Device_p);
                stvtg_HALVfeSetModeParams(Device_p, STVTG_TIMING_MODE_576I50000_13500);
                stvtg_HALSetInterrupt(Device_p);
            }
            break;
#endif /* #ifdef STVTG_VFE || VFE2*/
#ifdef STVTG_VOS
        case STVTG_DEVICE_TYPE_VTG_CELL_7015 : /* no break */
        case STVTG_DEVICE_TYPE_VTG_CELL_7020 :
        case STVTG_DEVICE_TYPE_VTG_CELL_7710 :
        case STVTG_DEVICE_TYPE_VTG_CELL_7100 :
        case STVTG_DEVICE_TYPE_VTG_CELL_7200 :
            if ((ErrorCode = stvtg_HALInit(Device_p, InitParams_p)) == ST_NO_ERROR)
            {
                stvtg_HALInitClockGen(Device_p);
                stvtg_HALVosSetCapability(Device_p);
                if ((ErrorCode = stvtg_HALVosSetModeParams(Device_p, STVTG_TIMING_MODE_SLAVE)) == ST_NO_ERROR)
                {
                    stvtg_HALSetInterrupt(Device_p);
                }
            }
            break;
#endif /* #ifdef STVTG_VOS */
        default :
            ErrorCode = ST_ERROR_BAD_PARAMETER;
            break;
    }
    return(ErrorCode);
} /* end of stvtg_IHALInit() function */

/*******************************************************************************
Name        : stvtg_IHALGetVtgId
Description : stvtg_IHALGetVtgId gets device Id matching base address of the VTG
 *            registers, that is the offset of the VTG registers within the register
 *            map. That's why InitParams_p->Target.VtgCell.BaseAddress_p is given
 *            and DeviceBaseAddress is not taken into account.
Parameters  : InitParams_p : IN : Init parameters
 *            VtgId_p : OUT : VtgId to give back
Assumptions :
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_BAD_PARAMETER
*******************************************************************************/
ST_ErrorCode_t stvtg_IHALGetVtgId(  const STVTG_InitParams_t * const InitParams_p
                                  , STVTG_VtgId_t *            const VtgId_p
                                 )
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    switch (InitParams_p->DeviceType)
    {
        case STVTG_DEVICE_TYPE_DENC :
            *VtgId_p = STVTG_VTG_ID_DENC;
            break;
#if defined (STVTG_VFE) || defined (STVTG_VFE2)
        case STVTG_DEVICE_TYPE_VFE : /* no break */
        case STVTG_DEVICE_TYPE_VFE2:
            ErrorCode = stvtg_HALVfeGetVtgId(InitParams_p->Target.VtgCell2.BaseAddress_p, VtgId_p);
            break;
#endif /* #ifdef STVTG_VFE || VFE2*/
#ifdef STVTG_VOS
        case STVTG_DEVICE_TYPE_VTG_CELL_7015 : /* no break */
        case STVTG_DEVICE_TYPE_VTG_CELL_7020 :
             ErrorCode = stvtg_HALGetVtgId(InitParams_p->Target.VtgCell.BaseAddress_p, VtgId_p);
            break;
       case STVTG_DEVICE_TYPE_VTG_CELL_7710 :
       case STVTG_DEVICE_TYPE_VTG_CELL_7100 :
       case STVTG_DEVICE_TYPE_VTG_CELL_7200 :
            ErrorCode = stvtg_HALGetVtgId(InitParams_p->Target.VtgCell3.BaseAddress_p, VtgId_p);
            break;

#endif /* #ifdef STVTG_VOS */
        default :
            ErrorCode = ST_ERROR_BAD_PARAMETER;
            break;
    }
    return(ErrorCode);
} /* end of stvtg_IHALGetVtgId() function */

/*******************************************************************************
Name        : stvtg_IHALUpdateTimingParameters
Description : call appropriates HALVTG routine
Parameters  : Device_p : IN/OUT : Device associated data to be updated
 *            Mode : IN : Timing Mode to set
Assumptions : Device_p not NULL
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_BAD_PARAMETER, STVTG_ERROR_DENC_ACCESS
 *            ST_ERROR_FEATURE_NOT_SUPPORTED
*******************************************************************************/
ST_ErrorCode_t stvtg_IHALUpdateTimingParameters(  stvtg_Device_t *            const Device_p
                                                , const STVTG_TimingMode_t Mode
                                               )
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    switch (Device_p->DeviceType)
    {
        case STVTG_DEVICE_TYPE_DENC :
            ErrorCode = stvtg_HALUpdateTimingParameters(Device_p, Mode);
            break;
#if defined (STVTG_VFE) || defined(STVTG_VFE2)
        case STVTG_DEVICE_TYPE_VFE : /* no break */
        case STVTG_DEVICE_TYPE_VFE2:
            stvtg_HALVfeSetModeParams(Device_p, Mode);
            break;
#endif /* #ifdef STVTG_VFE || VFE2*/
#ifdef STVTG_VOS
        case STVTG_DEVICE_TYPE_VTG_CELL_7015 : /* no break */
        case STVTG_DEVICE_TYPE_VTG_CELL_7020 :
            ErrorCode = stvtg_HALVosSetModeParams(Device_p, Mode);
            break;
        case STVTG_DEVICE_TYPE_VTG_CELL_7710 :
        case STVTG_DEVICE_TYPE_VTG_CELL_7100 :
        case STVTG_DEVICE_TYPE_VTG_CELL_7200 :
            ErrorCode = stvtg_HALVosSetModeParams(Device_p, Mode);
            if( ErrorCode == ST_NO_ERROR)
                ErrorCode = stvtg_HALVosSetAWGSettings(Device_p);
            break;
#endif /* #ifdef STVTG_VOS */
        default :
            ErrorCode = ST_ERROR_BAD_PARAMETER;
            break;
    }
    return(ErrorCode);
} /* end of stvtg_IHALUpdateTimingParameters() function */

/*******************************************************************************
Name        : stvtg_IHALSetOptionalConfiguration
Description : call appropriates HALVTG routine
Parameters  : Device_p : IN/OUT : Device associated data to be updated
Assumptions : Device_p not NULL. Option has been checked.
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_BAD_PARAMETER, ST_ERROR_FEATURE_NOT_SUPPORTED
*******************************************************************************/
ST_ErrorCode_t stvtg_IHALSetOptionalConfiguration( stvtg_Device_t * const Device_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    switch (Device_p->DeviceType)
    {
        case STVTG_DEVICE_TYPE_DENC :
            /* just modify option, TimingMode don't change */
            ErrorCode = stvtg_HALUpdateTimingParameters(Device_p, Device_p->TimingMode);
            break;
#if defined (STVTG_VFE) || defined (STVTG_VFE2)
        case STVTG_DEVICE_TYPE_VFE : /* no break */
        case STVTG_DEVICE_TYPE_VFE2:
            ErrorCode = stvtg_HALSetOptionalConfiguration(Device_p);
            break;
#endif /* #ifdef STVTG_VFE || VFE2*/
#ifdef STVTG_VOS
        case STVTG_DEVICE_TYPE_VTG_CELL_7015 : /* no break */
        case STVTG_DEVICE_TYPE_VTG_CELL_7020 :
        case STVTG_DEVICE_TYPE_VTG_CELL_7710 :
        case STVTG_DEVICE_TYPE_VTG_CELL_7100 :
        case STVTG_DEVICE_TYPE_VTG_CELL_7200 :
            ErrorCode = stvtg_HALSetOptionalConfiguration(Device_p);
            break;
#endif /* #ifdef STVTG_VOS */
        default :
            ErrorCode = ST_ERROR_BAD_PARAMETER;
            break;
    }
    return(ErrorCode);
} /* end of stvtg_IHALSetOptionalConfiguration() function */


/*******************************************************************************
Name        : stvtg_IHALCheckOptionValidity
Description : check option validity
Parameters  : Device_p : IN/OUT : Device associated data to be updated
Assumptions : Device_p not NULL.
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_BAD_PARAMETER, ST_ERROR_FEATURE_NOT_SUPPORTED
*******************************************************************************/
ST_ErrorCode_t stvtg_IHALCheckOptionValidity( stvtg_Device_t * const Device_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    switch (Device_p->DeviceType)
    {
        case STVTG_DEVICE_TYPE_DENC :
            switch (Device_p->Option)
            {
                case STVTG_OPTION_HSYNC_POLARITY :     /* no break */
                case STVTG_OPTION_VSYNC_POLARITY :
                    break;
                case STVTG_OPTION_EMBEDDED_SYNCHRO :   /* no break */
                case STVTG_OPTION_NO_OUTPUT_SIGNAL :   /* no break */
                case STVTG_OPTION_OUT_EDGE_POSITION :
                    ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
                    break;
                default :
                    ErrorCode = ST_ERROR_BAD_PARAMETER;
                    break;
            }
            break;
#if defined (STVTG_VFE) || defined (STVTG_VFE2)
        case STVTG_DEVICE_TYPE_VFE : /* no break */
        case STVTG_DEVICE_TYPE_VFE2:
            switch (Device_p->Option)
            {
                case STVTG_OPTION_HSYNC_POLARITY :     /* no break */
                case STVTG_OPTION_VSYNC_POLARITY :     /* no break */
                case STVTG_OPTION_OUT_EDGE_POSITION :
                    break;
                case STVTG_OPTION_EMBEDDED_SYNCHRO :   /* no break */
                case STVTG_OPTION_NO_OUTPUT_SIGNAL :
                    ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
                    break;
                default :
                    ErrorCode = ST_ERROR_BAD_PARAMETER;
                    break;
            }
            break;
#endif /* #ifdef STVTG_VFE || VFE2*/
#ifdef STVTG_VOS
        case STVTG_DEVICE_TYPE_VTG_CELL_7015 : /* no break */
        case STVTG_DEVICE_TYPE_VTG_CELL_7020 :
        case STVTG_DEVICE_TYPE_VTG_CELL_7710 :
        case STVTG_DEVICE_TYPE_VTG_CELL_7100 :
        case STVTG_DEVICE_TYPE_VTG_CELL_7200 :
            switch (Device_p->Option)
            {
                case STVTG_OPTION_EMBEDDED_SYNCHRO :   /* no break */
                case STVTG_OPTION_NO_OUTPUT_SIGNAL :   /* no break */
                case STVTG_OPTION_HSYNC_POLARITY :     /* no break */
                case STVTG_OPTION_VSYNC_POLARITY :     /* no break */
                case STVTG_OPTION_OUT_EDGE_POSITION :
                    break;
                default :
                    ErrorCode = ST_ERROR_BAD_PARAMETER;
                    break;
            }
            break;
#endif /* #ifdef STVTG_VOS */
        default :
            ErrorCode = ST_ERROR_BAD_PARAMETER;
            break;
    }
    return(ErrorCode);
} /* end of stvtg_IHALCheckOptionValidity() function */


/*******************************************************************************
Name        : stvtg_IHALCheckSlaveModeParams
Description : call appropriates HALVTG routine
Parameters  : Device_p : IN/OUT : Device associated data to be updated
 *            SlaveModeParams_p : Slave Mode Params to check
Assumptions : Device_p not NULL
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_BAD_PARAMETER;
*******************************************************************************/
ST_ErrorCode_t stvtg_IHALCheckSlaveModeParams(  const stvtg_Device_t * const Device_p
                                              , const STVTG_SlaveModeParams_t * const SlaveModeParams_p
                                             )
{
    switch (Device_p->DeviceType)
    {
        case STVTG_DEVICE_TYPE_DENC :
            return(stvtg_HALCheckDencSlaveModeParams(SlaveModeParams_p));
            break;
#if defined (STVTG_VFE) || defined (STVTG_VFE2)
        case STVTG_DEVICE_TYPE_VFE : /* no break */
        case STVTG_DEVICE_TYPE_VFE2:
            return(stvtg_HALVfeCheckSlaveModeParams(SlaveModeParams_p));
            break;
#endif /* #ifdef STVTG_VFE || VFE2 */
#ifdef STVTG_VOS
        case STVTG_DEVICE_TYPE_VTG_CELL_7015 : /* no break */
        case STVTG_DEVICE_TYPE_VTG_CELL_7020 :
        case STVTG_DEVICE_TYPE_VTG_CELL_7710 :
        case STVTG_DEVICE_TYPE_VTG_CELL_7100 :
        case STVTG_DEVICE_TYPE_VTG_CELL_7200 :
            return(stvtg_HALVosCheckSlaveModeParams(SlaveModeParams_p, Device_p->VtgId));
            break;
#endif /* #ifdef STVTG_VOS */
        default :
            return(ST_ERROR_BAD_PARAMETER);
            break;
    }
} /* end of stvtg_IHALCheckSlaveModeParams() function */

/*******************************************************************************
Name        : stvtg_IHALTerm
Description : call appropriates HALVTG routine
Parameters  : Device_p : IN/OUT : Device associated data to be updated
Assumptions : Device_p not NULL
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_INTERRUPT_UNINSTALL
*******************************************************************************/
ST_ErrorCode_t stvtg_IHALTerm(stvtg_Device_t * const Device_p)
{
    switch (Device_p->DeviceType)
    {
        case STVTG_DEVICE_TYPE_DENC :
#ifdef STVTG_VSYNC_WITHOUT_VIDEO
            return(stvtg_DeactivateVideoVsyncInterrupt(Device_p));
#else
            return(ST_NO_ERROR);
#endif /* #ifdef STVTG_VSYNC_WITHOUT_VIDEO */
            break;
#if defined (STVTG_VFE) || defined (STVTG_VFE2)
        case STVTG_DEVICE_TYPE_VFE : /* no break */
        case STVTG_DEVICE_TYPE_VFE2:
            return(stvtg_HALInterruptTerm(Device_p));
            break;
#endif /* #ifdef STVTG_VFE || VFE2*/
#ifdef STVTG_VOS
        case STVTG_DEVICE_TYPE_VTG_CELL_7015 : /* no break */
        case STVTG_DEVICE_TYPE_VTG_CELL_7020 :
        case STVTG_DEVICE_TYPE_VTG_CELL_7710 :
        case STVTG_DEVICE_TYPE_VTG_CELL_7100 :
        case STVTG_DEVICE_TYPE_VTG_CELL_7200 :
            return(stvtg_HALInterruptTerm(Device_p));
            break;
#endif /* #ifdef STVTG_VOS */
        default :
            /* not possible, however C code needs a return */
            return(ST_ERROR_BAD_PARAMETER);
            break;
    }
} /* End of stvtg_IHALTerm() function */

/* End of vtgi_hal.c */
