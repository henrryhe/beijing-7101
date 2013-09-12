/************************************************************************

File Name   : vtg_hal1.c

Description : VTG driver module for omega1.

COPYRIGHT (C) STMicroelectronics 2002

Date               Modification                                     Name
----               ------------                                     ----
24 Sep 1999        Created                                           JG
01 Sep 2000        Updated when 7015 HAL added                      HSdLM
08 Dec 2000        Add DENC software reset                          HSdLM
08 Feb 2001        Use new Denc register access                     HSdLM
09 Mar 2001        include denc_reg.h for DENC registers            HSdLM
*                  add square pixel modes if CellId>=6 in
*                  capabilities, use stdencra_Mask/Or.
28 Jun 2001        return STVTG_ERROR_DENC_ACCESS insted of STDENC  HSdLM
*                  error code
20 Feb 2002        Modify ST40GX1 init configuration following      HSdLM
 *                 DDTS GNBvd10951 HW defect.
16 Apr 2002        New DENC DeviceType for STi7020                  HSdLM
************************************************************************/

/* Includes ---------------------------------------------------------------- */
#if !defined(ST_OSLINUX)
#include <string.h>
#endif

#include "stsys.h"
#include "sttbx.h"

#include "stdenc.h"
#include "stvtg.h"
#include "vtg_drv.h"
#include "vtg_hal1.h"
#include "denc_reg.h"

/* Private Types ----------------------------------------------------------- */

/* Private Constants ------------------------------------------------------- */

/* Private Variables -------------------------------------------------------- */

/* Private Macros ----------------------------------------------------------- */

#define IS_VALID_SLAVEOF(so)  (   ((so) == STVTG_DENC_SLAVE_OF_ODDEVEN) \
                               || ((so) == STVTG_DENC_SLAVE_OF_VSYNC)    \
                               || ((so) == STVTG_DENC_SLAVE_OF_SYNC_IN_DATA))

#define OK(ErrorCode) ((ErrorCode) == ST_NO_ERROR)

/* Private Function prototypes ---------------------------------------------- */

/* Functions ---------------------------------------------------------------- */

/*
******************************************************************************
Public Functions
******************************************************************************
*/

/*******************************************************************************
Name        : stvtg_HALInitVtgInDenc
Description : init VTG in DENC cell on chip
Parameters  : Device_p : IN/OUT : Device associated data to be updated
Assumptions : Device_p is OK, Device_p->DencDeviceType is ok.
Limitations :
Returns     : None
*******************************************************************************/
void stvtg_HALInitVtgInDenc( stvtg_Device_t * const Device_p)
{
    STVTG_DencSlaveParams_t *Params_p;

    Params_p = (STVTG_DencSlaveParams_t *)&(Device_p->SlaveModeParams.Target.Denc);

    /* Set registers init parameters */

    switch (Device_p->DencDeviceType)
    {
        case STDENC_DEVICE_TYPE_DENC:
            Device_p->IsHSyncPositive           = FALSE;
            Device_p->SlaveModeParams.SlaveMode = STVTG_SLAVE_MODE_V_ONLY;
            break;
        case STDENC_DEVICE_TYPE_7015: /* no break */
        case STDENC_DEVICE_TYPE_7020:
            Device_p->IsHSyncPositive           = TRUE;
            Device_p->SlaveModeParams.SlaveMode = STVTG_SLAVE_MODE_V_ONLY;
            break;
        case STDENC_DEVICE_TYPE_GX1:
            Device_p->IsHSyncPositive           = TRUE;
            Device_p->SlaveModeParams.SlaveMode = STVTG_SLAVE_MODE_H_AND_V; /* GNBvd10951 */
            break;
        case STDENC_DEVICE_TYPE_4629 :
            Device_p->IsHSyncPositive           = TRUE;
            Device_p->SlaveModeParams.SlaveMode = STVTG_SLAVE_MODE_H_AND_V;
            break;
        default :
            /* see assumptions */
            break;
    }
    if (Device_p->DencDeviceType!=STDENC_DEVICE_TYPE_4629 )
    {
        Params_p->DencSlaveOf      = STVTG_DENC_SLAVE_OF_ODDEVEN;
    }
    else
    {
        Params_p->DencSlaveOf      = STVTG_DENC_SLAVE_OF_SYNC_IN_DATA;
    }
    Device_p->IsVSyncPositive  = FALSE;
    Params_p->FreeRun          = FALSE;
    Params_p->OutSyncAvailable = FALSE;
    Params_p->SyncInAdjust     = 1;

} /* End of stvtg_HALInitVtgInDenc() function */


/*******************************************************************************
Name        : stvtg_HALSetCapabilityOnDenc
Description : Fill capability structure
Parameters  : Device_p : IN/OUT : Device associated data to be updated
Assumptions : Device_p not NULL and OK
Limitations :
Returns     : None
*******************************************************************************/
void stvtg_HALSetCapabilityOnDenc( stvtg_Device_t * const Device_p)
{
    Device_p->CapabilityBank1 =   (1<<STVTG_TIMING_MODE_SLAVE)
                                | (1<<STVTG_TIMING_MODE_480I59940_13500)
                                | (1<<STVTG_TIMING_MODE_576I50000_13500);

    if (Device_p->DencCellId >= 6) /* Square pixel not supported by V3 and V5 */
    {
        /* These timings need specific DENC CKREF frequencies of 24.5454MHz or 29.5MHz */
        Device_p->CapabilityBank1 |=  (1<<STVTG_TIMING_MODE_480I59940_12273)
                                    | (1<<STVTG_TIMING_MODE_576I50000_14750);
    }
    Device_p->CapabilityBank2 = 0;
}

/*******************************************************************************
Name        : stvtg_HALCheckDencSlaveModeParams
Description : check slave mode parameters are OK
Parameters  : Device_p (IN) : pointer on VTG device to access
              SlaveModeParams_p (IN) : slave mode parameters to check
Assumptions : Device_p is OK
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_BAD_PARAMETER
*******************************************************************************/
ST_ErrorCode_t stvtg_HALCheckDencSlaveModeParams( const STVTG_SlaveModeParams_t * const SlaveModeParams_p)
{
    const STVTG_DencSlaveParams_t * Params_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    Params_p = (const STVTG_DencSlaveParams_t *)&(SlaveModeParams_p->Target.Denc);

    if (    (!IS_VALID_SLAVEMODE(SlaveModeParams_p->SlaveMode))
         || (!IS_VALID_SLAVEOF(Params_p->DencSlaveOf))
         || (Params_p->SyncInAdjust > 3)
       )
    {
        ErrorCode = ST_ERROR_BAD_PARAMETER;
    }
    return(ErrorCode);
} /* end of stvtg_HALCheckDencSlaveModeParams */


/*************************************************************************
 Name : stvtg_HALUpdateTimingParameters
 Description : Sets VTG Timing Parameters.
 Parameters : Device_p (IN/OUT) : pointer onto VTG device to access and update.
              Mode (IN) : New mode to set. Should be reliable.
 Return : ST_NO_ERROR, ST_ERROR_FEATURE_NOT_SUPPORTED, STVTG_ERROR_DENC_ACCESS
**************************************************************************/
ST_ErrorCode_t stvtg_HALUpdateTimingParameters(  stvtg_Device_t *            const Device_p
                                               , const STVTG_TimingMode_t Mode
                                              )
{
    ST_ErrorCode_t Ec = ST_NO_ERROR;
    U8 OrMaskCfg0, OrMaskCfg1, OrMaskCfg4;
    STDENC_Handle_t DencHnd;
    STVTG_DencSlaveParams_t *DencSlaveParams_p;
    STVTG_SlaveMode_t SlaveMode;

    SlaveMode = Device_p->SlaveModeParams.SlaveMode;
    DencSlaveParams_p = (STVTG_DencSlaveParams_t *)&(Device_p->SlaveModeParams.Target.Denc);
    DencHnd = Device_p->DencHandle;

    Ec = STDENC_CheckHandle(DencHnd);
    if (!OK(Ec)) return (STVTG_ERROR_DENC_ACCESS);

    OrMaskCfg0 = 0;
    OrMaskCfg1 = 0;
    OrMaskCfg4 = 0;

    OrMaskCfg0 |= (Device_p->IsHSyncPositive ? DENC_CFG0_HSYNC_POL : 0);
    OrMaskCfg0 |= (Device_p->IsVSyncPositive ? DENC_CFG0_ODD_POL : 0);
    OrMaskCfg1 |= (DencSlaveParams_p->OutSyncAvailable ? DENC_CFG1_SYNC_OK : 0);

    if (Mode == STVTG_TIMING_MODE_SLAVE)
    {
        switch (SlaveMode)
        {
            case STVTG_SLAVE_MODE_H_AND_V:             /* full slave */
                switch (DencSlaveParams_p->DencSlaveOf)
                {
                    case STVTG_DENC_SLAVE_OF_ODDEVEN :
                        OrMaskCfg0 |= DENC_CFG0_ODHS_SLV;
                        break;
                    case STVTG_DENC_SLAVE_OF_VSYNC :
                        OrMaskCfg0 |= DENC_CFG0_VSHS_SLV;
                        break;
                    case STVTG_DENC_SLAVE_OF_SYNC_IN_DATA :
                        OrMaskCfg0 |= DENC_CFG0_FRHS_SLV;
                        break;
                    default :
                        /* should not occur, filtered above */
                        break;
                }
                break;
            case STVTG_SLAVE_MODE_V_ONLY:             /* half slave */
                switch (DencSlaveParams_p->DencSlaveOf)
                {
                    case STVTG_DENC_SLAVE_OF_ODDEVEN :
                        OrMaskCfg0 |= DENC_CFG0_ODDE_SLV;
                        break;
                    case STVTG_DENC_SLAVE_OF_VSYNC :
                        OrMaskCfg0 |= DENC_CFG0_VSYNC_SLV;
                        break;
                    case STVTG_DENC_SLAVE_OF_SYNC_IN_DATA :
                        OrMaskCfg0 |= DENC_CFG0_FRM_SLV;
                        break;
                    default :
                        /* should not occur, filtered above */
                        break;
                }
                break;
            default:                                  /* unknown slave mode */
                /* should not occur, filtered above */
                break;
        } /* End of switch (SlaveMode)*/
        if (DencSlaveParams_p->FreeRun)
        {
            OrMaskCfg0 |= DENC_CFG0_FREE_RUN;
        }
        if ( Device_p->DencDeviceType==STDENC_DEVICE_TYPE_4629 )
        {
            OrMaskCfg4 |= DENC_CFG4_ALINE; /* active line duration follows ITU-R/SMPTE analog standard requirements */
        }
        else
        {
            switch (DencSlaveParams_p->SyncInAdjust)
            {
                case 0 :
                    OrMaskCfg4 = DENC_CFG4_SYIN_0;
                    break;
                case 1 :
                    OrMaskCfg4 = DENC_CFG4_SYIN_P1;
                    break;
                case 2 :
                    OrMaskCfg4 = DENC_CFG4_SYIN_P2;
                    break;
                case 3 :
                    OrMaskCfg4 = DENC_CFG4_SYIN_P3;
                    break;
                default :
                    /* should not occur, filtered above */
                    break;
            } /* end of switch (DencSlaveParams_p->SyncInAdjust) */
        }
    }
    else /* Mode is a master one  */
    {
        OrMaskCfg0 |= DENC_CFG0_MASTER;
    } /* end of if (Mode == STVTG_TIMING_MODE_SLAVE) */

    STDENC_RegAccessLock(DencHnd);
    Ec = STDENC_MaskReg8(DencHnd, DENC_CFG0, (U8)~DENC_CFG0_MASK_STD, OrMaskCfg0);
    if (OK(Ec)) Ec = STDENC_MaskReg8(DencHnd, DENC_CFG1, (U8)~DENC_CFG1_SYNC_OK, OrMaskCfg1);
    if ( Device_p->DencDeviceType==STDENC_DEVICE_TYPE_4629 )
    {
        if (OK(Ec)) Ec = STDENC_MaskReg8(DencHnd, DENC_CFG4, DENC_CFG4_MASK_SYIN&~DENC_CFG4_ALINE, OrMaskCfg4);
	    Ec = STDENC_MaskReg8(DencHnd, DENC_CFG0, (U8)~DENC_CFG0_MASK_STD, 0x16);

    }
    else
    {
        if (OK(Ec)) Ec = STDENC_MaskReg8(DencHnd, DENC_CFG4, DENC_CFG4_MASK_SYIN, OrMaskCfg4);
    }
    if (OK(Ec)) Ec = STDENC_OrReg8(DencHnd, DENC_CFG6, DENC_CFG6_RST_SOFT); /* DENC software reset */
    STDENC_RegAccessUnlock(DencHnd);
    if (OK(Ec))
    {
        /* Set new timing mode of device */
        Device_p->TimingMode = Mode;
    }
    if (!OK(Ec))
    {
        Ec = STVTG_ERROR_DENC_ACCESS;
    }
    return (Ec);
} /* End of stvtg_HALUpdateTimingParameters() function */

/* ----------------------------- End of file ------------------------------ */
