/************************************************************************

File Name   : hal_vos.c

Description : VTG driver module. HAL for Video Output Stage VTG

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                                     Name
----               ------------                                     ----
28 Jun 2001        Creation from vtg_hal2.c                         HSdLM
04 Sep 2001        Move interrupt init and handler routines here,   HSdLM
 *                 add NULL pointer test for DDTS GNBvd08642.
08 Oct 2001        Remove PixelsPerLineDiv2                         HSdLM
19 Oct 2001        Move interrupt init and handler back to          HSdLM
 *                 vtg_hal2.c file (Use STINTMR for VFE)
 *                 Fix DDTS GNBvd09294.
26 Apr 2002        Capability limitation for 7020 cut1 on pixclk    HSdLM
17 Sep 2002        Fix DDTS GNBvd15700 "VSync output signal is not  HSdLM
 *                 set properly for output mode 720P60000_74250"
 *                 Now supports 7020 cut2, remove cut1 limitation
17 Jan 2003        Correct DDTS GNBvd17725.                          BS
 *                 Remove fix of DDTS GNBvd09294
************************************************************************/

/* Includes --------------------------------------------------------------- */
#if !defined(ST_OSLINUX)
#include <string.h>
#endif

#include "stsys.h"

#include "stddefs.h"


#include "stgxobj.h"

#include "stvtg.h"
#include "vtg_drv.h"
#include "vtg_hal2.h"
#include "clk_gen.h"
#include "sync_in.h"
#include "vos_reg.h"
#include "hal_vos.h"

/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */
#if defined(ST_7710) || defined(ST_7100)|| defined (ST_7109)
#define AWG_NUMBER_OF_MODE   2
/* This value need to be adjust in function of output stage */
#if defined (ST_7710)
#define AWG_BLANK            0x0000027D
#elif defined (ST_7100)|| defined (ST_7109)
#define AWG_BLANK            0x00000272
#endif

typedef struct AWG_Timings_s
{
  STVTG_TimingMode_t  TimingMode;
  U32                 AWGRamContent[AWG_RAM_SIZE];
}AWG_Timings_t;

static const AWG_Timings_t AWG_TimingMode[AWG_NUMBER_OF_MODE]=
{
  { STVTG_TIMING_MODE_1152I50000_48000 ,
  { 0x00000600, 0x00000E8F, AWG_BLANK,  0x00000C6F, 0x00001409,
    0x00000600, 0x00000C6F, AWG_BLANK,  0x00000E8F, 0x00001401,
    0x00000A00, 0x00000C6F, 0x00000128, 0x00001A69, 0x00000200,
    0x00000C6f, AWG_BLANK,  0x00000E8F, 0x00000600, 0x00000E8F,
    AWG_BLANK,  0x00000C6F, 0x00001409, 0x00000200, 0x00000C6F,
    AWG_BLANK,  0x00000E8F, 0x00000A00, 0x00000C6F, 0x00000128,
    0x00001A6B, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000}
  },
  { STVTG_TIMING_MODE_1080I50000_72000 ,
  { 0x00000200, 0x00000FD7, AWG_BLANK, 0x00000CA7, 0x00000200,
    0x00000CA7, AWG_BLANK,  0x00000FD7, 0x00000A00, 0x00000CA7,
    0x00000128, 0x00001A6E, 0x00000200, 0x00000CA7, AWG_BLANK,
    0x00000FD7, 0x00000200, 0x00000FD7, AWG_BLANK,  0x00000CA7,
    0x00000A00, 0x00000CA7, 0x00000128, 0x00001A70, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000 }
  }
};

#endif /*7710-710x-7200*/

/* Private Variables ------------------------------------------------------ */

/* Private Macros --------------------------------------------------------- */
#define stvtg_Write32(a, v)     STSYS_WriteRegDev32LE((a), (v))
#ifdef ST_OSLINUX
#define stvtg_Read32(a)         STSYS_ReadRegDev32LE((void*)(a))
#else
#define stvtg_Read32(a)         STSYS_ReadRegDev32LE((a))
#endif /*ST_OSLINUX*/

/* check valid parameters according to 7015/20 features */
#if defined WA_GNBvd07864
#define IS_VALID_SLAVEOF_VTG(so,id) (   ( ((so) == STVTG_SLAVE_OF_PIN) && ((id) == STVTG_VTG_ID_1))   \
                                     || ( ((so) == STVTG_SLAVE_OF_REF) && ((id) == STVTG_VTG_ID_2))   \
                                     || ( ((so) == STVTG_SLAVE_OF_SDIN1) && ((id) == STVTG_VTG_ID_1)) \
                                     || ( ((so) == STVTG_SLAVE_OF_SDIN2) && ((id) == STVTG_VTG_ID_1)) \
                                     || ((so) == STVTG_SLAVE_OF_HDIN))
#else
#define IS_VALID_SLAVEOF_VTG(so,id) (   ( ((so) == STVTG_SLAVE_OF_PIN) && ((id) == STVTG_VTG_ID_1))   \
                                     || ( ((so) == STVTG_SLAVE_OF_REF) && ((id) == STVTG_VTG_ID_2))   \
                                     || ((so) == STVTG_SLAVE_OF_SDIN1)    \
                                     || ((so) == STVTG_SLAVE_OF_SDIN2)    \
                                     || ((so) == STVTG_SLAVE_OF_HDIN))
#endif /*WA_GNBvd07864*/


#define IS_VALID_EXTVSYNCSHAPE(evs)  (((evs) == STVTG_EXT_VSYNC_SHAPE_BNOTT) || ((evs) == STVTG_EXT_VSYNC_SHAPE_PULSE))

/* Private Function prototypes ---------------------------------------------- */
static void SetSlaveModeParams  (  stvtg_Device_t * const Device_p
                                 , U32 *            const VtgMod_p
                                );
static void SetMasterModeParams (  stvtg_Device_t * const Device_p
                                 , U32 *            const VtgMod_p
                                );

/* Functions (private)------------------------------------------------------- */

/*************************************************************************
Name : SetSlaveModeParams
Description : Sets Slave Mode parameters to registers.
Parameters : Device_p (IN/OUT) : pointer on VTG device to access/update
             VtgMod_p (IN/OUT) : slave mode parameters register to update
Assumptions : pointer on device and variable to update are not NULL.
              slave mode parameters are reliable. (already checked)
Limitations :
Return : none
**************************************************************************/
static void SetSlaveModeParams(   stvtg_Device_t * const Device_p
                                , U32 *         const VtgMod_p
                              )
{
  #if !(defined (ST_7200)|| defined (ST_7111)|| defined (ST_7105))
    U32 Hvdrive = 0;    /*configure VSync and HSync as inputs */
    U32 HTmp,VTmp;
  #endif
  const STVTG_VtgCellSlaveParams_t *Params_p;
  STVTG_SlaveMode_t SlaveMode;

  SlaveMode = Device_p->SlaveModeParams.SlaveMode;
  Params_p = (const STVTG_VtgCellSlaveParams_t *)&(Device_p->SlaveModeParams.Target.VtgCell);

  #if defined (ST_7200)|| defined (ST_7111)|| defined (ST_7105)

    switch (Params_p->VSlaveOf) /* Set External VSync selection */
    {
        case STVTG_SLAVE_OF_DVP0: /* no break */
        case STVTG_SLAVE_OF_EXT0:
            *VtgMod_p |= VTG_VTGMOD_MODE_SLAVE_EXT0;
            break;
        case STVTG_SLAVE_OF_DVP1: /* no break */
        case STVTG_SLAVE_OF_EXT1:
            *VtgMod_p |= VTG_VTGMOD_MODE_SLAVE_EXT1;
            break;
        default:
            /* Should not be possible (see Assumptions in function header)*/
            break;
    }
  #else


    /*****************************/
    /* SET HVDRIVE               */
    /*****************************/
    /* if VTG is slave of Sync pin, Sync should be input, otherwise output.*/
    /* set Hvdrive for VSYNC slave */
    VTmp = 0;
    if (Params_p->VSlaveOf != STVTG_SLAVE_OF_PIN)
    {
        VTmp |= VTG_HVDRIVE_VSYNC;   /* VSync ouput */
    }
    /* set Hvdrive for HSYNC slave */
    HTmp = 0;
    if (Params_p->HSlaveOf != STVTG_SLAVE_OF_PIN)
    {
        HTmp |= VTG_HVDRIVE_HSYNC;   /* HSync ouput */
    }
    switch (SlaveMode)
    {
        case STVTG_SLAVE_MODE_H_AND_V:             /* full slave */
            Hvdrive = (VTmp | HTmp);
            break;
        case STVTG_SLAVE_MODE_V_ONLY:             /* slave on VSync, master on HSync : HSync ouput*/
            Hvdrive = (VTmp | VTG_HVDRIVE_HSYNC);
            break;
        case STVTG_SLAVE_MODE_H_ONLY :            /* 7015/20 not supported */
        default:                                  /* unknown slave mode */
            /* Should not be possible (see Assumptions in function header)*/
            break;
    }
    Device_p->HVDriveSave = Hvdrive;

    if ((!Device_p->NoOutputSignal)&&(Device_p->VtgId == STVTG_VTG_ID_1))
    {
        stvtg_Write32(((U32)Device_p->BaseAddress_p + VTG_HVDRIVE), Hvdrive);
    }
    /*****************************/
    /* Update *VtgMod_p          */
    /*****************************/
    VTmp = 0;
    HTmp = 0;

    /* Set External HSync / VSync selection if in slave mode */
    switch (Params_p->VSlaveOf) /* Set External VSync selection  */
    {
        case STVTG_SLAVE_OF_PIN:
            if (Device_p->VtgId == STVTG_VTG_ID_1)
            {
                /* 7015/20 : VTG1 can only be slave of pin, not of VTG2 */
                VTmp |= VTG_VTGMOD_VSYNC_PIN;
            }
            break;
        case STVTG_SLAVE_OF_REF:
            if (   (Device_p->VtgId == STVTG_VTG_ID_2)
                && (Params_p->VRefVtgIn == STVTG_VTG_ID_1))
            {
                /* 7015/20 : VTG2 can only be slave of VTG1, not of pin */
                VTmp |= VTG_VTGMOD_VSYNC_VTG1;
            }
            break;
        case STVTG_SLAVE_OF_SDIN1:
            VTmp |= VTG_VTGMOD_VSYNC_SDIN1;
            break;
        case STVTG_SLAVE_OF_SDIN2:
            VTmp |= VTG_VTGMOD_VSYNC_SDIN2;
            break;
        case STVTG_SLAVE_OF_HDIN:
            VTmp |= VTG_VTGMOD_VSYNC_HDIN;
            break;
        default:            /* Unknown Master */
            /* Should not be possible (see Assumptions in function header)*/
            break;
    } /* end of switch (Params_p->VSlaveOf) */

    switch (Params_p->HSlaveOf) /* Set External HSync selection  */
    {
        case STVTG_SLAVE_OF_PIN:
            if (Device_p->VtgId == STVTG_VTG_ID_1)
            {
                /* 7015/20 : VTG1 can only be slave of pin, not of VTG2 */
                HTmp |= VTG_VTGMOD_HSYNC_PIN;
            }
            break;
        case STVTG_SLAVE_OF_REF:
            if (   (Device_p->VtgId == STVTG_VTG_ID_2)
                && (Params_p->HRefVtgIn == STVTG_VTG_ID_1))
            {
                /* 7015/20 : VTG2 can only be slave of VTG1, not of pin */
                HTmp |= VTG_VTGMOD_HSYNC_VTG1;
            }
            break;
        case STVTG_SLAVE_OF_SDIN1:
            HTmp |= VTG_VTGMOD_HSYNC_SDIN1;
            break;
        case STVTG_SLAVE_OF_SDIN2:
            HTmp |= VTG_VTGMOD_HSYNC_SDIN2;
            break;
        case STVTG_SLAVE_OF_HDIN:
            HTmp |= VTG_VTGMOD_HSYNC_HDIN;
            break;
        default:            /* Unknown Master */
            /* Should not be possible (see Assumptions in function header)*/
            break;
    } /* end of switch (Params_p->HSlaveOf) */

    /* for VSYNC slave */
    if (Params_p->ExtVSyncShape == STVTG_EXT_VSYNC_SHAPE_BNOTT)
    {
        VTmp |= VTG_VTGMOD_EXT_VSYNC_BNOTT;
    }
    if (Params_p->IsExtVSyncRisingEdge)
    {
        VTmp |= VTG_VTGMOD_EXT_VSYNC_HIGH;
    }
    /* for HSYNC slave */
    if (Params_p->IsExtHSyncRisingEdge)
    {
        HTmp |= VTG_VTGMOD_EXT_HSYNC_HIGH;
    }

    switch (SlaveMode)
    {
        case STVTG_SLAVE_MODE_H_AND_V:             /* full slave */
            *VtgMod_p |= VTG_VTGMOD_SLAVE_MODE_EXTERNAL;
            *VtgMod_p |= (HTmp | VTmp);
            break;
        case STVTG_SLAVE_MODE_V_ONLY:             /* half slave */
            *VtgMod_p |= VTG_VTGMOD_SLAVE_MODE_EXT_VREF;
            *VtgMod_p |= VTmp;
            break;
        case STVTG_SLAVE_MODE_H_ONLY :            /* 7015/20 not supported */
        default:                                  /* unknown slave mode */
            /* Should not be possible (see Assumptions in function header)*/
            break;
    }
#if defined (ST_7710)||defined (ST_7100)||defined (ST_7109)
    if (Device_p->VtgId == STVTG_VTG_ID_2)
    {
        /* Set the limits of detection of a bottom field*/
        stvtg_Write32(((U32)Device_p->BaseAddress_p + VTG_RG1), Params_p->BottomFieldDetectMin);
        stvtg_Write32(((U32)Device_p->BaseAddress_p + VTG_RG2), Params_p->BottomFieldDetectMax);
    }
#else

    /*7015/20*/
    /* Set the limits of detection of a bottom field*/
    stvtg_Write32(((U32)Device_p->BaseAddress_p + VTG_RG1), Params_p->BottomFieldDetectMin);
    stvtg_Write32(((U32)Device_p->BaseAddress_p + VTG_RG2), Params_p->BottomFieldDetectMax);
#endif
#endif /*!(ST_7200||ST_7111||ST_7105)*/

} /* end of SetSlaveModeParams */

/*************************************************************************
Name : SetMasterModeParams
Description : Sets Master Mode parameters to registers.
Parameters : Device_p (IN/OUT) : pointer on VTG device to access/update
             VtgMod_p (IN/OUT) : master mode parameters register to update
Assumptions : pointer on device and variable to update are not NULL, Mode is
              accurate.
Limitations :
Return : none
**************************************************************************/
static void SetMasterModeParams(   stvtg_Device_t *         const Device_p
                                 , U32 *                    const VtgMod_p
                               )
{
    STVTG_ModeParams_t ModeParams;
    RegParams_t *RegParams_p;
    ModeParamsLine_t *Line_p;
    U32 LinesByFrame;
    #if defined (ST_7100)|| defined (ST_7109)
        U32 Value;
    #endif

    #if defined (ST_7100)
        U32 CutId;
    #endif

    /* Reach line in Table matching TimingMode */
    Line_p = stvtg_GetModeParamsLine(Device_p->TimingMode);

    /* Get corresponding mode parameters in that line */
    ModeParams = Line_p->ModeParams;

    /* Get corresponding register parameters in that line */
    RegParams_p = &Line_p->RegParams;
    #if !(defined(ST_7200)|| defined(ST_7111)|| defined (ST_7105))
        /* Set VSync and HSync signal direction as outputs */
        Device_p->HVDriveSave = VTG_HVDRIVE_VSYNC | VTG_HVDRIVE_HSYNC;
        if ( Device_p->DeviceType == STVTG_DEVICE_TYPE_VTG_CELL_7710 )
        {
            #ifdef STI7710_CUT2x
                if ((!Device_p->NoOutputSignal)&&(Device_p->VtgId == STVTG_VTG_ID_1))
                {
                    stvtg_Write32(((U32)Device_p->BaseAddress_p + VTG_HVDRIVE), VTG_HVDRIVE_VSYNC | VTG_HVDRIVE_HSYNC);
                }
            #else
                /* Manage only VTG1 case outputing the HSync & VSync (DVO signals) */
                if ((!Device_p->NoOutputSignal)&&(Device_p->VtgId == STVTG_VTG_ID_1))
                {
                    stvtg_Write32(((U32)Device_p->BaseAddress_p + VTG_HVDRIVE), 0x10000);
                }
            #endif
        }
        else
        {
            #if defined (ST_7100)
                if (!Device_p->NoOutputSignal)
                {
                    if (Device_p->VtgId == STVTG_VTG_ID_1)
                    {
                       CutId = ST_GetCutRevision();
                       if ( CutId  >= 0xB0 )
                       {
                         /* Set the three bits DVOSYNC[18:16] as following : 001 since the description of this register has
                         * changed from cut2.0.*/
                         Value = stvtg_Read32((U32)Device_p->BaseAddress_p + VTG_HVDRIVE );
                         Value = Value & 0xFFF8FFFF;
                         Value = Value | 0x10000;
                         stvtg_Write32(((U32)Device_p->BaseAddress_p + VTG_HVDRIVE), Value);  /* output main Hsync and Vsync */
                       }
                       else
                       {
                         stvtg_Write32(((U32)Device_p->BaseAddress_p + VTG_HVDRIVE),  VTG_HVDRIVE_VSYNC | VTG_HVDRIVE_HSYNC);
                       }
                    }
                }

            #elif defined (ST_7109)
                if ((!Device_p->NoOutputSignal)&&(Device_p->VtgId == STVTG_VTG_ID_1))
                {
                        /* Set the three bits DVOSYNC[18:16] as following : 001 */
                         Value = stvtg_Read32((U32)Device_p->BaseAddress_p + VTG_HVDRIVE );
                         Value = Value & 0xFFF8FFFF;
                         Value = Value | 0x10000;
                         stvtg_Write32(((U32)Device_p->BaseAddress_p + VTG_HVDRIVE), Value); /* output main Hsync and Vsync */
                }
            #else
              if ((!Device_p->NoOutputSignal)&&(Device_p->VtgId == STVTG_VTG_ID_1))
              {
                  stvtg_Write32(((U32)Device_p->BaseAddress_p + VTG_HVDRIVE), VTG_HVDRIVE_VSYNC | VTG_HVDRIVE_HSYNC);
              }
            #endif /*7100-7109*/
        }
    #endif
    /* Set mode as master */
    *VtgMod_p |= VTG_VTGMOD_SLAVE_MODE_INTERNAL;

    /* detect mode SMPTE295M or similar : even number of lines but interlaced : interlacing must be forced*/
    if (   (((RegParams_p->LinesByFrame) & 0x1) == 0)
        && (ModeParams.ScanType == STGXOBJ_INTERLACED_SCAN)
        )
    {
        *VtgMod_p |= VTG_VTGMOD_FORCE_INTERLACED;
    }
    else
    {
        *VtgMod_p &= (U32)~VTG_VTGMOD_FORCE_INTERLACED;
    }

    /* set CLKLN to PixelsPerLine */
    stvtg_Write32(((U32)Device_p->BaseAddress_p + VTG_CLKLN), RegParams_p->PixelsPerLine);

    #if defined(ST_7200)|| defined(ST_7111)|| defined (ST_7105)
        Device_p->PixelsPerLine = RegParams_p->PixelsPerLine;
    #endif

    /* set Half Line Per Field */
    LinesByFrame = RegParams_p->LinesByFrame;

    /* SD and ED display modes were defined with negative synchronization*/
    Device_p->IsHSyncPositive       = RegParams_p->HSyncPolarity;
    Device_p->IsVSyncPositive       = RegParams_p->VSyncPolarity;

    switch (Device_p->TimingMode)
    {
       case STVTG_TIMING_MODE_1080I60000_74250: /*no break*/
       case STVTG_TIMING_MODE_1080I59940_74176 : /*no break*/
       case STVTG_TIMING_MODE_1080I50000_74250:  /*no break*/
       case STVTG_TIMING_MODE_1080I50000_74250_1:
                 Device_p->OutEdges.HSyncRising  = 0x0;
                 Device_p->OutEdges.HSyncFalling = 0x2C;
                 Device_p->OutEdges.VSyncRising  = 0x0;
                 Device_p->OutEdges.VSyncFalling = 0xA;
                 break;

       case STVTG_TIMING_MODE_720P60000_74250:
       case STVTG_TIMING_MODE_720P59940_74176:
       case STVTG_TIMING_MODE_720P50000_74250 :
            Device_p->OutEdges.HSyncRising  = 0x0;
            Device_p->OutEdges.HSyncFalling = 0x28;
            Device_p->OutEdges.VSyncRising  = 0x0;
            Device_p->OutEdges.VSyncFalling = 0xA;
            break;

       case STVTG_TIMING_MODE_480P60000_27027: /* no break */
       case STVTG_TIMING_MODE_480P60000_24570: /* no break */
       case STVTG_TIMING_MODE_480P59940_27000: /* no break */
       case STVTG_TIMING_MODE_480P59940_24545:
            Device_p->OutEdges.HSyncRising  = 0x3E;
            Device_p->OutEdges.HSyncFalling = 0x0;
            Device_p->OutEdges.VSyncRising  = 0xC;
            Device_p->OutEdges.VSyncFalling = 0x0;
            break;

       case STVTG_TIMING_MODE_480P60000_25200 :
       case STVTG_TIMING_MODE_480P59940_25175 :
            Device_p->OutEdges.HSyncRising  = 0x60;
            Device_p->OutEdges.HSyncFalling = 0x0;
            Device_p->OutEdges.VSyncRising  = 0x4;
            Device_p->OutEdges.VSyncFalling = 0x0;
            break;

       case STVTG_TIMING_MODE_480I60000_13514: /* no break */
       case STVTG_TIMING_MODE_480I60000_12285: /* no break */
       case STVTG_TIMING_MODE_480I59940_13500: /* no break */
       case STVTG_TIMING_MODE_480I59940_12273:
            Device_p->OutEdges.HSyncRising  = 0x3E;
            Device_p->OutEdges.HSyncFalling = 0x0;
            Device_p->OutEdges.VSyncRising  = 0x6;
            Device_p->OutEdges.VSyncFalling = 0x0;
            break;

       case STVTG_TIMING_MODE_576P50000_27000 :
            Device_p->OutEdges.HSyncRising  = 0x40;
            Device_p->OutEdges.HSyncFalling = 0x0;
            Device_p->OutEdges.VSyncRising  = 0xA;
            Device_p->OutEdges.VSyncFalling = 0x0;
            break;

       case STVTG_TIMING_MODE_576I50000_13500: /* no break*/
       case STVTG_TIMING_MODE_576I50000_14750:
            Device_p->OutEdges.HSyncRising  = 0x3F;
            Device_p->OutEdges.HSyncFalling = 0x0;
            Device_p->OutEdges.VSyncRising  = 0x6;
            Device_p->OutEdges.VSyncFalling = 0x0;
            break;

       default :
           Device_p->OutEdges.HSyncRising  = 0;
           Device_p->OutEdges.HSyncFalling = RegParams_p->HSyncPulseWidth;
           Device_p->OutEdges.VSyncRising  = 0;
           Device_p->OutEdges.VSyncFalling = RegParams_p->VSyncPulseWidth;
           break;
        }
    /* DDTS GNBvd17725 : HD 1080i active video is placed too low, last line over the VSync pulse (Soft bug) */
    /* Code simplification by removing DDTS GNBvd09294 */
    if (ModeParams.ScanType == STGXOBJ_PROGRESSIVE_SCAN)
    {
        LinesByFrame = 2 * LinesByFrame;
    }

    stvtg_Write32(((U32)Device_p->BaseAddress_p + VTG_HLFLN), LinesByFrame);

    stvtg_HALSetSyncEdges( Device_p);
} /* end of SetMasterModeParams */

/*************************************************************************
 Name : stvtg_HALGetVtgId
 Description : Gives back the VtgId associated to a VTG base address
              , that is the  offset of the VTG registers within the register map
 Parameters : BaseAddress_p (IN) : base address of the VTG
              VtgId_p (IN/OUT)   : memory location to give back information
 Return : ST_NO_ERROR or ST_ERROR_BAD_PARAMETER
**************************************************************************/
ST_ErrorCode_t stvtg_HALGetVtgId(  const void *    const BaseAddress_p
                                 , STVTG_VtgId_t * const VtgId_p
                                )
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    if (VtgId_p == NULL)
    {
        ErrorCode = ST_ERROR_BAD_PARAMETER;
    }
    else
    {
        if ((U32)(BaseAddress_p) == VTG1_BASE_ADDRESS)
        {
                *VtgId_p = STVTG_VTG_ID_1;
        }
        else if ((U32)(BaseAddress_p) == VTG2_BASE_ADDRESS)
        {
                *VtgId_p = STVTG_VTG_ID_2;
        }
        #if defined(ST_7200)
            else if ((U32)(BaseAddress_p) == VTG3_BASE_ADDRESS)
            {
                    *VtgId_p = STVTG_VTG_ID_3;
            }
        #endif
        else
        {
                ErrorCode = ST_ERROR_BAD_PARAMETER;
        }
    }
    return (ErrorCode);
} /* end of stvtg_HALGetVtgId*/


/*******************************************************************************
Name        : stvtg_HALVosSetCapability
Description : Fill capability structure
Parameters  : Device_p : IN/OUT : Device associated data to be updated
Assumptions : Device_p not NULL and OK
Limitations :
Returns     : None
*******************************************************************************/
void stvtg_HALVosSetCapability( stvtg_Device_t * const Device_p)
{
    Device_p->CapabilityBank1 =   (U32)CAPABILITY_1_ALL
                                & (U32)~(1<<(STVTG_TIMING_MODE_1080P60000_148500))    /* Pixel clock unsupported */
                                & (U32)~(1<<(STVTG_TIMING_MODE_1080P59940_148352))    /* Pixel clock unsupported */
                                & (U32)~(1<<(STVTG_TIMING_MODE_1080P50000_148500));   /* Pixel clock unsupported */

    Device_p->CapabilityBank2 =   (U32)CAPABILITY_2_ALL;
                               /* & (U32)~(1<<(STVTG_TIMING_MODE_720P50000_74250-32))*/   /* not a standard */
                                /*& (U32)~(1<<(STVTG_TIMING_MODE_1152I50000_74250-32));*/ /* not a standard */
    /* -32 : because on second bank. Be aware of this if these formats are moved inside enum over 32th threshold */

} /* End of stvtg_HALVosSetCapability() function */

/*******************************************************************************
Name        : stvtg_HALVosCheckSlaveModeParams
Description : check slave mode parameters are OK
Parameters  : Device_p (IN) : pointer on VTG device to access
              SlaveModeParams_p (IN) : slave mode parameters to check
Assumptions : Device_p is OK
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_BAD_PARAMETER
*******************************************************************************/
ST_ErrorCode_t stvtg_HALVosCheckSlaveModeParams(   const STVTG_SlaveModeParams_t * const SlaveModeParams_p
                                              , const STVTG_VtgId_t VtgId
                                            )
{
    const STVTG_VtgCellSlaveParams_t *Params_p;
    ST_ErrorCode_t ErrorCode= ST_NO_ERROR;

    Params_p = (const STVTG_VtgCellSlaveParams_t *)&(SlaveModeParams_p->Target.VtgCell);

    #ifdef WA_GNBvd06999
        if (   (SlaveModeParams_p->SlaveMode != STVTG_SLAVE_MODE_H_AND_V)
    #else
        if (   (!IS_VALID_SLAVEMODE(SlaveModeParams_p->SlaveMode))
    #endif
            || (!IS_VALID_EXTVSYNCSHAPE(Params_p->ExtVSyncShape))
            || (!IS_VALID_SLAVEOF_VTG(Params_p->HSlaveOf, VtgId))
            || (!IS_VALID_SLAVEOF_VTG(Params_p->VSlaveOf, VtgId))
            )
        {
            ErrorCode = ST_ERROR_BAD_PARAMETER;
        }
        return(ErrorCode);
} /* end of stvtg_HALVosCheckSlaveModeParams */


/*************************************************************************
Name        : stvtg_HALVosSetModeParams
Description : Sets VTG Timing Parameters.
Parameters  : Device_p (IN/OUT) : pointer onto VTG device to access and update.
              Mode (IN) : New mode to set. Should be reliable.
Assumptions : Device_p is reliable (not NULL and good address), Mode is accurate.
 *            If EmbeddedSynchro, Mode supports it.
Limitations :
Return      : ST_NO_ERROR, ST_ERROR_FEATURE_NOT_SUPPORTED, ST_ERROR_BAD_PARAMETER
**************************************************************************/
ST_ErrorCode_t stvtg_HALVosSetModeParams(  stvtg_Device_t *         const Device_p
                                         , const STVTG_TimingMode_t Mode
                                        )
{
    U32 VtgMod;
    STVTG_TimingMode_t OldMode;
    ST_ErrorCode_t ErrorCode= ST_NO_ERROR;

    /* Add NULL pointer test, useless but asked by DDTS GNBvd08642 */
    if (Device_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    OldMode = Device_p->TimingMode;
    /* Set new timing mode of device */
    Device_p->TimingMode = Mode;

    if (Device_p->EmbeddedSynchro)
    {
        if (OldMode != Mode) /* Mode as changed */
        {
            ErrorCode = stvtg_HALSetSyncInSettings(Device_p); /* be careful : use Device_p->TimingMode !*/
        }
    }

    /* get current mode and remove bits that will be changed */
    #if defined (ST_7200)|| defined (ST_7111)|| defined (ST_7105)
        VtgMod =  stvtg_Read32(((U32)Device_p->BaseAddress_p + VTG_VTGMOD)) & (U32)~VTG_MOD_MASK;
        VtgMod &= (U32)~VTG_MOD_DISABLE;
        /* Disable VTG */
        stvtg_Write32(((U32)Device_p->BaseAddress_p + VTG_VTGMOD), VTG_MOD_DISABLE);
    #else
        VtgMod =  stvtg_Read32(((U32)Device_p->BaseAddress_p + VTG_VTGMOD))
                & (U32)~VTG_VTGMOD_SLAVEMODE_MASK
                & (U32)~VTG_VTGMOD_HVSYNC_CMD_MASK;

    #endif
    if (Mode == STVTG_TIMING_MODE_SLAVE)    /* VTG is SLAVE */
    {
        SetSlaveModeParams(Device_p, &VtgMod);
    }
    else                                    /* VTG is MASTER */
    {
        SetMasterModeParams(Device_p, &VtgMod);
        stvtg_HALSetPixelClock(Device_p);
    }   /* end of if (Mode == STVTG_TIMING_MODE_SLAVE) */

    /* write new VtgMod value to register */
    stvtg_Write32(((U32)Device_p->BaseAddress_p + VTG_VTGMOD), VtgMod);

    /* Set Vsync Timer ? */
    stvtg_Write32(((U32)Device_p->BaseAddress_p + VTG_VTIMER), 0);

    #if defined (ST_7200)|| defined (ST_7111)|| defined (ST_7105)
        stvtg_Write32(((U32)Device_p->BaseAddress_p + VTG_DRST), 0xffff);
    #else
        /* Resetting the VTG sometimes makes GDP crash on 7020 but if not VTG Vsync it may not start ... */
        stvtg_Write32(((U32)Device_p->BaseAddress_p + VTG_DRST), 1);
    #endif
    return(ErrorCode);
} /* end of stvtg_HALVosSetModeParams */

/*************************************************************************
Name        : stvtg_HALVosSetAWGSettings
Description : Sets AWG ram code.
Parameters  : Device_p (IN/OUT) : pointer onto VTG device to access and update.
Assumptions :
Limitations :
Return      : ST_NO_ERROR.
**************************************************************************/
ST_ErrorCode_t stvtg_HALVosSetAWGSettings(const stvtg_Device_t * const Device_p)
{
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;
  #if defined (ST_7710)||defined (ST_7100)|| defined (ST_7109)
    U32 Dspcfg_Val;


    U32 * BaseAddress_p;
    U32 Cnt = 0, Mode = 0 ;
    if (Device_p->VtgId == STVTG_VTG_ID_1)
    {
       while((AWG_TimingMode[Mode].TimingMode != Device_p->TimingMode) &&
            (Mode < AWG_NUMBER_OF_MODE))
      {
              Mode++;
      }
      if(Mode != AWG_NUMBER_OF_MODE)
      {
          BaseAddress_p =(U32*)(AWG_BASE_ADDRESS + AWG_RAM_OFFSET);
          for (Cnt=0; (Cnt < AWG_RAM_SIZE); Cnt++)
          {
              stvtg_Write32(BaseAddress_p, AWG_TimingMode[Mode].AWGRamContent[Cnt]);
              BaseAddress_p++;
          }

            /*AWG_CTRL*/
            stvtg_Write32((U32)AWG_BASE_ADDRESS + AWG_CTRL, 0x19 );
            /* Activate AWG formatter */
            Dspcfg_Val = stvtg_Read32((U32)VOS_BASE_ADDRESS + DSPCFG_ANA);
            Dspcfg_Val = Dspcfg_Val | (1 << DSPCFG_ANA_AWG) ;
            STSYS_WriteRegDev32LE((U32)VOS_BASE_ADDRESS + DSPCFG_ANA, Dspcfg_Val);

     }
      else
      {
         /* No AWG mode found back to default formatter */
          Dspcfg_Val = stvtg_Read32((U32)VOS_BASE_ADDRESS + DSPCFG_ANA);
          Dspcfg_Val = Dspcfg_Val & (U32)~(1 << DSPCFG_ANA_AWG);
          STSYS_WriteRegDev32LE((U32)VOS_BASE_ADDRESS + DSPCFG_ANA, Dspcfg_Val);
      }
    }
  #else
    UNUSED_PARAMETER(Device_p);
  #endif
  return( ErrorCode);
}


/* ----------------------------- End of file ------------------------------ */
