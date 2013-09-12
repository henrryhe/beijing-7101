/************************************************************************

File Name   : hal_vfe.c

Description : VTG driver module. HAL for Video Front End VTG programmation

COPYRIGHT (C) STMicroelectronics 2002

Date               Modification                                     Name
----               ------------                                     ----
02 Jul 2001        Creation from vtg_hal2.c                         HSdLM
04 Sep 2001        New interrupt init and handler routines, fix     HSdLM
 *                 capabilities issue.
08 Oct 2001        Set CLKLN to twice PixelsPerLine, update         HSdLM
 *                 capabilities.
09 Oct 2001        Use STINTMR for VFE                              HSdLM
29 Nov 2001        Add WA for ST40GX1 DVP interrupt issue           HSdLM
20 Feb 2002        Remove useless code                              HSdLM
26 Mar 2003        Update for 5528                                  HSdLM
21 Jan 2004        Add 60hz and Square pixel modes on 5528          AC
27 Feb 2004        Update for 5528 & 4629 & 5100                    MH
 *
************************************************************************/

/* Includes --------------------------------------------------------------- */
#if !defined(ST_OSLINUX)
#include <string.h>
#endif

#include "stddefs.h"
#include "stgxobj.h"
#include "stdevice.h"

#include "stsys.h"
#include "sttbx.h"


#include "stevt.h"
#include "stvtg.h"
#include "vtg_drv.h"
#include "vtg_hal2.h"
#include "vfe_reg.h"
#include "hal_vfe.h"
#include "clk_gen.h"

/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */

/* Private Variables ------------------------------------------------------ */


/* Private Macros --------------------------------------------------------- */

/* check valid parameters according to Video Front End features */
#define IS_VALID_SLAVEOF_VTG(so) (     ((so) == STVTG_SLAVE_OF_DVP0) \
                                    || ((so) == STVTG_SLAVE_OF_DVP1) \
                                    || ((so) == STVTG_SLAVE_OF_EXT0) \
                                    || ((so) == STVTG_SLAVE_OF_EXT1))

#define stvtg_Write32(a, v)     STSYS_WriteRegDev32LE((a), (v))
#ifdef ST_OSLINUX
#define stvtg_Read32(a)         STSYS_ReadRegDev32LE((void*)(a))
#else
#define stvtg_Read32(a)         STSYS_ReadRegDev32LE((a))
#endif


/* Private Function prototypes ---------------------------------------------- */
static void SetSlaveModeParams    (  stvtg_Device_t * const Device_p
                                   , U32 *            const VtgMod_p
                                  );
static void SetMasterModeParams   (  stvtg_Device_t * const Device_p
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
                                , U32 *          const VtgMod_p
                              )
{
    /* Set External HSync / VSync selection if in slave mode */
    switch (Device_p->SlaveModeParams.Target.VfeSlaveOf) /* Set External VSync selection */
    {
        case STVTG_SLAVE_OF_DVP0: /* no break */
        case STVTG_SLAVE_OF_EXT0:
            *VtgMod_p |= VTG_VFE_MOD_MODE_SLAVE_EXT0;
            break;
        case STVTG_SLAVE_OF_DVP1: /* no break */
        case STVTG_SLAVE_OF_EXT1:
            *VtgMod_p |= VTG_VFE_MOD_MODE_SLAVE_EXT1;
            break;
        default:
            /* Should not be possible (see Assumptions in function header)*/
            break;
    }

} /* end of SetSlaveModeParams */

/*************************************************************************
Name : SetMasterModeParams
Description : Sets Master Mode parameters to registers.
Parameters : Device_p (IN/OUT) : pointer on VTG device to access/update
             VtgMod_p (IN/OUT) : master mode parameters register to update
Assumptions : pointer on device and variable to update are not NULL, Mode is
              accurate. DeviceType is OK.
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

    /* Reach line in Table matching TimingMode */
    Line_p = stvtg_GetModeParamsLine(Device_p->TimingMode);

    /* Get corresponding mode parameters in that line */
    ModeParams = Line_p->ModeParams;

    /* Get corresponding register parameters in that line */
    RegParams_p = &Line_p->RegParams;

    /* Set mode as master */
    *VtgMod_p |= VTG_VFE_MOD_MODE_MASTER;

    /* set CLKLN to twice PixelsPerLine */
    stvtg_Write32(((U32)Device_p->BaseAddress_p + VTG_VFE_CLKLN), 2*(RegParams_p->PixelsPerLine));
    Device_p->PixelsPerLine = RegParams_p->PixelsPerLine; /* must equal half CLKLN */

    /* set Half Line Per Field */
    if (ModeParams.ScanType == STGXOBJ_INTERLACED_SCAN)
    {
        stvtg_Write32(((U32)Device_p->BaseAddress_p + VTG_VFE_HLFLN), RegParams_p->LinesByFrame);
    }
    else /* STGXOBJ_PROGRESSIVE_SCAN */
    {
        stvtg_Write32(((U32)Device_p->BaseAddress_p + VTG_VFE_HLFLN), 2*RegParams_p->LinesByFrame);
    }

    Device_p->IsHSyncPositive       = RegParams_p->HSyncPolarity;
    Device_p->IsVSyncPositive       = RegParams_p->VSyncPolarity;
#if defined(ST_5528)||defined(ST_5100)||defined(ST_5105)||defined(ST_5301)||defined(ST_5188)||defined(ST_5525)\
  ||defined(ST_5107)||defined(ST_5162)
#if defined(ST_5528) || defined (ST_5188) || defined (ST_5525)
    Device_p->OutEdges.HSyncRising  = 128;
    Device_p->OutEdges.HSyncFalling = 5;
#else /*5100-5105-5301*/
#ifdef WA_SHIFT_VTG
    Device_p->OutEdges.HSyncRising  = 129;
    Device_p->OutEdges.HSyncFalling = 6;
 #else
    Device_p->OutEdges.HSyncRising  = 128;
    Device_p->OutEdges.HSyncFalling = 5;
#endif
#endif
    Device_p->OutEdges.VSyncRising  = 1;
    Device_p->OutEdges.VSyncFalling = (RegParams_p->LinesByFrame+1)/2;
#else
    Device_p->OutEdges.HSyncRising  = 0;
    Device_p->OutEdges.HSyncFalling = RegParams_p->HSyncPulseWidth;
    Device_p->OutEdges.VSyncRising  = 0;
    Device_p->OutEdges.VSyncFalling = RegParams_p->VSyncPulseWidth;
#endif
    stvtg_HALSetSyncEdges( Device_p);

} /* end of SetMasterModeParams */

/*******************************************************************************
Name        : stvtg_HALVfeSetCapability
Description : Fill capability structure
Parameters  : Device_p : IN/OUT : Device associated data to be updated
Assumptions : Device_p not NULL and OK
Limitations :
Returns     : None
*******************************************************************************/
void stvtg_HALVfeSetCapability( stvtg_Device_t * const Device_p)
{
    Device_p->CapabilityBank1 =   (1<<STVTG_TIMING_MODE_SLAVE)
                                | (1<<STVTG_TIMING_MODE_480I59940_13500)    /* NTSC, PAL M */
                                | (1<<STVTG_TIMING_MODE_576I50000_13500)   /* PAL B,D,G,H,I,N, SECAM */
                                | (1<<STVTG_TIMING_MODE_480I60000_13514)   /* NTSC 60Hz */
                                | (1<<STVTG_TIMING_MODE_480I60000_12285)   /* NTSC 60Hz square */
                                | (1<<STVTG_TIMING_MODE_480I59940_12273)   /* NTSC square, PAL M square */
                                | (1<<STVTG_TIMING_MODE_576I50000_14750);  /* PAL B,D,G,H,I,N, SECAM square */

    /* Following modes can be supported by ST40GX1 hardware, re-programming pixel clock.
     * See "ST40GX1 low power application note" ADCS 7224656B to know how stop-modify-start PLL
     * and ST40GX1 datasheet 7227220C to know value to set in CLKGENB.PLL2CR to set square pixel pixel clocks
     */
                             /*   | (1<<STVTG_TIMING_MODE_480I60000_13514) */   /* NTSC 60Hz */
                             /*   | (1<<STVTG_TIMING_MODE_480P30000_13514)   */ /* ATSC 30P */
                             /*   | (1<<STVTG_TIMING_MODE_480P24000_10811)   */ /* ATSC 24P */
                             /*   | (1<<STVTG_TIMING_MODE_480P29970_13500)   */ /* ATSC 30P/1.001 */
                             /*   | (1<<STVTG_TIMING_MODE_480P23976_10800)   */ /* ATSC 24P/1.001 */
                             /*   | (1<<STVTG_TIMING_MODE_480I60000_12285)   */ /* NTSC 60Hz square */
                             /*   | (1<<STVTG_TIMING_MODE_480P30000_12285)   */ /* ATSC 30P square */
                             /*   | (1<<STVTG_TIMING_MODE_480P24000_9828)    */ /* ATSC 24P square */
                             /*   | (1<<STVTG_TIMING_MODE_480I59940_12273)   */ /* NTSC square, PAL M square */
                             /*   | (1<<STVTG_TIMING_MODE_480P29970_12273)   */ /* ATSC 30P/1.001 square */
                             /*   | (1<<STVTG_TIMING_MODE_480P23976_9818)    */ /* ATSC 24P/1.001 square */
                             /*   | (1<<STVTG_TIMING_MODE_576I50000_14750);  */ /* PAL B,D,G,H,I,N, SECAM square */

    Device_p->CapabilityBank2 = 0;

} /* End of stvtg_HALVfeSetCapability() function */

/*******************************************************************************
Name        : stvtg_HALVfeCheckSlaveModeParams
Description : check slave mode parameters are OK
Parameters  : SlaveModeParams_p (IN) : slave mode parameters to check
Assumptions :
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_BAD_PARAMETER
*******************************************************************************/
ST_ErrorCode_t stvtg_HALVfeCheckSlaveModeParams(   const STVTG_SlaveModeParams_t * const SlaveModeParams_p)
{
    ST_ErrorCode_t ErrorCode= ST_NO_ERROR;

    if (    (SlaveModeParams_p->SlaveMode != STVTG_SLAVE_MODE_H_AND_V)
         || (!IS_VALID_SLAVEOF_VTG(SlaveModeParams_p->Target.VfeSlaveOf)))
    {
        ErrorCode = ST_ERROR_BAD_PARAMETER;
    }
    return(ErrorCode);
} /* end of stvtg_HALVfeCheckSlaveModeParams */


/*************************************************************************
Name        : stvtg_HALVfeSetModeParams
Description : Sets VTG Timing Parameters.
Parameters  : Device_p (IN/OUT) : pointer onto VTG device to access and update.
              Mode (IN) : New mode to set. Should be reliable.
Assumptions : Device_p is reliable (not NULL and good address), Mode is accurate.
Limitations :
Return      : None
**************************************************************************/
void stvtg_HALVfeSetModeParams(  stvtg_Device_t *            const Device_p
                               , const STVTG_TimingMode_t Mode
                              )
{
    U32 VtgMod;

    /* Set new timing mode of device */
    Device_p->TimingMode = Mode;

    /* get current mode and remove bits that will be changed */
    VtgMod =  stvtg_Read32(((U32)Device_p->BaseAddress_p + VTG_VFE_MOD)) & (U32)~VTG_VFE_MOD_MASK;
    VtgMod &= (U32)~VTG_VFE_MOD_DISABLE;
    /* Disable VTG */
    stvtg_Write32(((U32)Device_p->BaseAddress_p + VTG_VFE_MOD), VTG_VFE_MOD_DISABLE);

    if (Mode == STVTG_TIMING_MODE_SLAVE)    /* VTG is SLAVE */
    {
      SetSlaveModeParams(Device_p, &VtgMod);

    }
    else                                    /* VTG is MASTER */
    {
        SetMasterModeParams(Device_p, &VtgMod);



        if ((Mode==STVTG_TIMING_MODE_480I59940_12273)||(Mode==STVTG_TIMING_MODE_576I50000_14750)|| \
            (Mode==STVTG_TIMING_MODE_480I60000_12285))
        {
            stvtg_HALSetPixelClock(Device_p);
        }
      #if defined(ST_5528) ||defined(ST_5100) ||defined(ST_5105) || defined(ST_5301) || defined(ST_5188) ||defined(ST_5525) \
       || defined(ST_5107) ||defined(ST_5162)
        else
         {
            /** Reset Clock system to have the default pixel clock **/
             stvtg_HALSetPixelClockRst(Device_p);
         }
      #endif
    }   /* end of if (Mode == STVTG_TIMING_MODE_SLAVE) */

    /* write new VtgMod value to register( Enable VTG) */

        stvtg_Write32(((U32)Device_p->BaseAddress_p + VTG_VFE_MOD), VtgMod);


#ifdef WA_DVP_ST40GX1_HARD_INTERRUPT_BUG
    /* write GAM_DVP_HLL (from DVP 0 & 1) to a correct value before starting VTG
       This is a bug who impact DVP hardware block (interrupt)
     * DVP0 Base Address is 0x200 before VTG one
     * DVP1 Base Address is 0x100 before VTG one
     * Register targetted is 0x2C offset from this Base address */
    stvtg_Write32(((U32)Device_p->BaseAddress_p - 0x200 + 0x2C), 0x360);
    stvtg_Write32(((U32)Device_p->BaseAddress_p - 0x100 + 0x2C), 0x360);
#endif /* #ifdef WA_DVP_ST40GX1_HARD_INTERRUPT_BUG */

    /* Reset the VTG before leaving : to take into account the VTG registers values ? */
    stvtg_Write32(((U32)Device_p->BaseAddress_p + VTG_VFE_SRST), (0xffff));


} /* end of stvtg_HALVfeSetModeParams */

/*************************************************************************
 Name : stvtg_HALVfeGetVtgId
 Description : Gives back the VtgId associated to a VTG base address
              , that is the  offset of the VTG registers within the register map
 Parameters : BaseAddress_p (IN) : base address of the VTG
              VtgId_p (IN/OUT)   : memory location to give back information
 Return : ST_NO_ERROR or ST_ERROR_BAD_PARAMETER
**************************************************************************/

ST_ErrorCode_t stvtg_HALVfeGetVtgId(  const void *    const BaseAddress_p
                                 , STVTG_VtgId_t * const VtgId_p
                                )
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
#if defined (ST_5525)
    if (VtgId_p == NULL)
    {
        ErrorCode = ST_ERROR_BAD_PARAMETER;
    }
    else
    {
        if ((U32)(BaseAddress_p) == VTG_0_BASE_ADDRESS)
        {

                *VtgId_p = STVTG_VTG_ID_1;
        }
        else if ((U32)(BaseAddress_p) == VTG_1_BASE_ADDRESS)
        {
                *VtgId_p = STVTG_VTG_ID_2;
        }
        else
        {
                ErrorCode = ST_ERROR_BAD_PARAMETER;
        }
    }
#else
    UNUSED_PARAMETER(BaseAddress_p);
    *VtgId_p = STVTG_VTG_ID_1;
#endif
    return (ErrorCode);
} /* end of stvtg_HALGetVtgId*/

/* ----------------------------- End of file ------------------------------ */

