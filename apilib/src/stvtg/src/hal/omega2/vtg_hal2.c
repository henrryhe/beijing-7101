/************************************************************************

File Name   : vtg_hal2.c

Description : VTG driver module. common HAL for omega2 VTG programmation

COPYRIGHT (C) STMicroelectronics 2003.

Date               Modification                                     Name
----               ------------                                     ----
01 Sep 2000        Creation                                         HSdLM
22 Nov 2000        Add EVT multi-instance support                   HSdLM
22 Jan 2001        Use STINTMR with STEVT callbacks                 HSdLM
21 Mar 2001        add pixclk config : HALVTG_SetPixelClock         HSdLM
 *                 remove useless standards
 *                 add HALVTG_SetOutput() to inhibit signal
28 Jun 2001        Split file for Video Front End (VFE) support     HSdLM
04 Sep 2001        Set target conditional build, move interrupt     HSdLM
 *                 init and handler routines in 'vos' and 'vfe'
08 Oct 2001        Use compilation flags related to device type     HSdLM
                   Add WA_GNBvd08957.
09 Oct 2001        Use STINTMR for VFE (ST40GX1)                    HSdLM
16 Apr 2002        New DeviceType for STi7020                       HSdLM
04 Jul 2002        Free interrupt at the end of IT handler instead  HSdLM
 *                 of the end of awakened task. Add compilation
 *                 flag to notify VSync under interrupt. Remove
 *                 useless mutex. Add debug statistic.
20 Jan 2003        Change flag STVTG_NOTIFY_VSYNC_UNDER_IT into     BS
                   STVTG_NOTIFY_VSYNC_IN_TASK
16 Apr 2003        Add support of STi5528 (VFE2).                   HSdLM
13 Sep 2004        Add support for ST40/OS21                        MH
************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */

/* Includes --------------------------------------------------------------- */
#if !defined(ST_OSLINUX)
#include <string.h>
#endif
#include "sttbx.h"
#include "stsys.h"

#include "stddefs.h"
#include "stgxobj.h"


#include "stevt.h"
#include "stvtg.h"
#include "vtg_drv.h"
#include "vtg_hal2.h"

#ifdef STVTG_VOS
#include "hal_vos.h"
#include "sync_in.h"
#include "vos_reg.h"
#endif /* #ifdef STVTG_VOS */

#if defined (STVTG_VFE) || defined (STVTG_VFE2)
#include "hal_vfe.h"
#include "vfe_reg.h"
#endif /* #ifdef STVTG_VFE || VFE2 */
/*#include "..\..\..\dvdgr-prj-stapigat\src\trace.h"*/

/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */

#ifdef STVTG_NOTIFY_VSYNC_IN_TASK
#ifndef STVTG_TASK_PRIORITY_IT_PROCESSING
#define STVTG_TASK_PRIORITY_IT_PROCESSING 12   /* from 0 (lowest priority) to 15 (highest priority) */
#endif
#ifdef STVTG_VOS
#define INTERRUPT_TASK_NAME_VOS_1     "STVTG_InterruptProcessingVos1"
#define INTERRUPT_TASK_NAME_VOS_2     "STVTG_InterruptProcessingVos2"
#endif /* #ifdef STVTG_VOS */
#if defined (STVTG_VFE) || defined (STVTG_VFE2)
#define INTERRUPT_TASK_NAME_VFE       "STVTG_InterruptProcessingVfe"
#endif /* #ifdef STVTG_VFE || VFE2 */
#define INTERRUPT_TASK_NAME_MAX_SIZE  35
#endif /* #ifdef STVTG_NOTIFY_VSYNC_IN_TASK */

#define NTSC_QUARTER_LINE        215
#define NTSC_THREE_QUARTER_LINE  644

#define VTG1_ITS 0x2c

/* Private Variables ------------------------------------------------------ */

typedef struct WA_GNBvd47682_VTimer_s
{
    BOOL Installed;
    U32  AVMuteAddress;
}WA_GNBvd47682_VTimer_t;

WA_GNBvd47682_VTimer_t stvtg_WA_GNBvd47682 = {0,0};  /* Added initilizer to remove a warning:no impact on software */

/* Global Variables --------------------------------------------------------- */

#if defined (ST_7200)|| defined (ST_7111)|| defined (ST_7105)
/*!!!!!!!!!only validated modes are in this tables : other modes will be added according to their validation delivery!!!!!!!!!*/
/* {Mode},

 * VTGn [1-3]

 * {HSyncRising, HSyncFalling, {VSyncTopRising.Vert,VSyncTopRising.Horiz},{ VSyncTopFalling.Vert,VSyncTopFalling.Horiz},
  {VSyncBotRising.Vert,VSyncBotRising.Hor},{VSyncBotFalling.Vert, VSyncBotFalling.Hor}

 * {SyncEnabled}
   HSYNC_VSYNC_REF = 0x1, HSYNC_VSYNC_1 = 0x2, HSYNC_VSYNC_2 = 0x4,  HSYNC_VSYNC_3 = 0x8, HSYNC_VSYNC_PAD  = 0x10.
   For example 1080I:  HSYNC_VSYNC_1| HSYNC_VSYNC_3 = 0x2|0x8 = 0xA

 */

static VideoActiveParams_t ModeVideoActiveTable[] =
{
     /* HD & ED video formats */
{ STVTG_TIMING_MODE_1080I60000_74250 , {2147, 2163, {563,1047}, {563,1063}, {562,2147}, {562, 2163}}, /*VTG1 : HSYNC1&VSYNC1*/
                                       {0,       0, { 0 ,   0}, { 0 ,   0}, {0  ,   0}, { 0 ,    0}}, /*VTG1 : HSYNC2&VSYNC2*/
                                       {0  ,    44, { 1 ,   0}, {  6 ,  0}, {0  ,1100}, { 5 , 1100}}, /*VTG1 : HSYNC3&VSYNC3*/
                                       9},  /*HSYNC_VSYNC_REF | HSYNC_VSYNC_3 */

{ STVTG_TIMING_MODE_1080I59940_74176 , {2147, 2163, {563,1047}, {563,1063}, {562,2147}, {562,2163}}, /*VTG1 : HSYNC1&VSYNC1*/
                                       {0   ,    0, { 0 ,   0}, { 0 ,   0}, {0  ,   0}, {0  ,   0}}, /*VTG1 : HSYNC2&VSYNC2*/
                                       { 0  ,   44, {1 ,    0}, {6 ,    0}, {0  ,1100}, {5  ,1100}}, /*VTG1 : HSYNC3&VSYNC3*/
                                        9},  /*HSYNC_VSYNC_REF | HSYNC_VSYNC_3 */

{ STVTG_TIMING_MODE_1080P60000_148500,  {0,      0, { 0 ,   0}, { 0 ,   0}, {0  ,   0}, { 0 , 0  }},
                                        {0,      0, { 0 ,   0}, { 0 ,   0}, {0  ,   0}, { 0 , 0  }},
                                        {0,      0, { 0 ,   0}, { 0 ,   0}, {0  ,   0}, { 0 , 0  }},9},

{ STVTG_TIMING_MODE_1080I50000_74250_1, {2634,10,{563,1314}, {563,1300}, {562,2628}, {562,2613}},
                                        {0,   0, { 0 ,   0}, { 0 ,   0}, {0  ,   0}, {0  ,   0}},
                                        {0,      44, { 1 ,   0}, { 6 ,   0}, {0  ,1320},  { 5 ,1320}},9},

{ STVTG_TIMING_MODE_1080I50000_74250,   {2634,10,{563,1314}, {563,1300}, {562,2628}, {562,2613}},
                                        {0,   0, { 0 ,   0}, { 0 ,   0}, {0  ,   0}, {0  ,   0}},
                                        {0,      44, { 1 ,   0}, { 6 ,   0}, {0  ,1320},  { 5 ,1320}},9},

{ STVTG_TIMING_MODE_720P60000_74250,    {0,       0, { 0 ,   0}, { 0 ,   0}, {0  ,   0},  { 0 , 0  }},
                                        {1635, 1594, {750, 1635},{750 ,1594 }, {750,1635},{750,1594}},
                                        {0,      40, { 1 ,   0}, { 6 ,   0}, {1  ,   0},  { 6 , 0  }},12},

{ STVTG_TIMING_MODE_720P59940_74176,    {0,       0, { 0 ,   0}, { 0 ,   0}, {0  ,   0},  { 0 , 0  }},
                                        {1635, 1594, {750, 1635},{750 ,1594 }, {750,1635},{750,1594}},
                                        {0,      40, { 1 ,   0}, { 6 ,   0}, {1  ,   0},  { 6 , 0  }},12},

{ STVTG_TIMING_MODE_720P50000_74250,   {0,   0,    { 0 ,   0},{0  ,   0},{0  ,   0},{0  ,   0}},
                                       {1965,1924, {750,1965},{750,1924},{750,1965},{750,1924}},
                                       {12,    56, { 1 ,   0},{ 5 ,   0},{1  ,   0},{5  ,   0}},12},


 /* SD video formats*/
 { STVTG_TIMING_MODE_480I60000_13514,   {1,       2, { 1 ,   0}, { 1 ,   2}, {0  ,   27}, { 0 , 27  }},
                                        {0,       0, { 0 ,   0}, { 0 ,   0}, {0  ,   0},  { 0 , 0  }},
                                        {0,      62, { 1 ,   0}, { 4 ,   0}, {0  , 429},  { 3 , 429}},10},

 { STVTG_TIMING_MODE_480I59940_13500,   {1,       2, { 1 ,   0}, { 1 ,   2}, {0  ,   27}, { 0 , 27  }},
                                        {0,       0, { 0 ,   0}, { 0 ,   0}, {0  ,   0},  { 0 , 0  }},
                                        {0,      62, { 1 ,   0}, { 4 ,   0}, {0  , 429},  { 3 , 429}},10},

 { STVTG_TIMING_MODE_576I50000_13500,   {1,       2, { 1 ,   0}, { 1 ,   2}, {0  ,   27}, { 0 , 27  }},
                                        {0,       0, { 0 ,   0}, { 0 ,   0}, {0  ,   0},  { 0 , 0  }},
                                        {0,      63, { 1 ,   0}, { 4 ,   0}, {0  , 432},  { 3 , 432}},10},

 { STVTG_TIMING_MODE_480P60000_27027,   {0,       0, { 0 ,   0}, { 0 ,   0}, {0  ,   0},  { 0 , 0  }},
                                        {848,  53, { 525 , 848}, { 1 , 53}, {525  , 848},  { 1 , 53 }},
                                        /*{851,  56, { 525 , 851}, { 1 , 56}, {525  , 851},  { 1 , 56 }},*/
                                        {0  ,    62, { 1 ,   0}, { 7 ,  0}, {1   ,   0},  { 7 , 0  }},
                                        12},

 { STVTG_TIMING_MODE_480P59940_27000,   {0,       0, { 0 ,   0}, { 0 ,   0}, {0  ,   0},  { 0 , 0  }},
                                        {848,  53, { 525 , 848}, { 1 , 53}, {525  , 848},  { 1 , 53 }},
                                        /*{851,  56, { 525 , 851}, { 1 , 56}, {525  , 851},  { 1 , 56 }},*/
                                        {0  ,    62, { 1 ,   0}, { 7 ,  0}, {1   ,   0},  { 7 , 0  }},12},

 /*Test 640*480p = Mode VGA*/
 { STVTG_TIMING_MODE_480P60000_25200,   {0,       0, { 0 ,   0}, { 0 ,   0}, {0  ,   0},  { 0 , 0  }},
                                        {0,       0, { 0 ,   0}, { 0 ,   0}, {0  ,   0},  { 0 , 0  }},
                                        {0,      96, { 1,    0}, { 3 ,   0}, {1  ,   0},  { 3 , 0  }},9},

 { STVTG_TIMING_MODE_480P59940_25175,   {0,       0, { 0 ,   0}, { 0 ,   0}, {0  ,   0},  { 0 , 0  }},
                                        {0,       0, { 0 ,   0}, { 0 ,   0}, {0  ,   0},  { 0 , 0  }},
                                        {0,      96, { 1 ,   0}, { 3 ,   0}, {1  ,   0},  { 3 , 0  }},9},
 /*1024*768P XGA modes */
 { STVTG_TIMING_MODE_768P75000_78750,   {0,       0, { 0 ,   0}, { 0 ,   0}, {0  ,   0},  { 0 , 0  }},
                                        {0,       0, { 0 ,   0}, { 0 ,   0}, {0  ,   0},  { 0 , 0  }},
                                        {0,       0, { 0 ,   0}, { 0 ,   0}, {0  ,   0},  { 0 , 0  }},1},
 { STVTG_TIMING_MODE_768P60000_65000,   {0,       0, { 0 ,   0}, { 0 ,   0}, {0  ,   0},  { 0 , 0  }},
                                        {0,       0, { 0 ,   0}, { 0 ,   0}, {0  ,   0},  { 0 , 0  }},
                                        {0,       0, { 0 ,   0}, { 0 ,   0}, {0  ,   0},  { 0 , 0  }},1},
 { STVTG_TIMING_MODE_768P85000_94500,   {0,       0, { 0 ,   0}, { 0 ,   0}, {0  ,   0},  { 0 , 0  }},
                                        {0,       0, { 0 ,   0}, { 0 ,   0}, {0  ,   0},  { 0 , 0  }},
                                        {0,       0, { 0 ,   0}, { 0 ,   0}, {0  ,   0},  { 0 , 0  }},1},
  /*Australian modes*/
 { STVTG_TIMING_MODE_1080I50000_72000,  {0,       0, { 0 ,   0}, { 0 ,   0}, {0  ,   0},  { 0 , 0  }},
                                        {0,       0, { 0 ,   0}, { 0 ,   0}, {0  ,   0},  { 0 , 0  }},
                                        {0,       0, { 0 ,   0}, { 0 ,   0}, {0  ,   0},  { 0 , 0  }},1},

 { STVTG_TIMING_MODE_576P50000_27000,   {0,       0, { 0 ,   0}, { 0 ,   0}, {0  ,   0},  { 0 , 0  }},
                                        {859,    61, {625 ,859}, { 1 ,  61}, {625, 859},  { 1 , 61 }},
										{0,      64, { 1,    0}, { 6 ,   0}, {1  ,   0},  { 6 , 0  }},
                                        /*{0  ,    64, {1   ,  0}, { 6 ,   0}, {  0,   0},  { 0 ,  0}},*/
                                        12},

 { STVTG_TIMING_MODE_1152I50000_48000,  {0,       0, { 0 ,   0}, { 0 ,   0}, {0  ,   0},  { 0 , 0  }},
                                        {0,       0, { 0 ,   0}, { 0 ,   0}, {0  ,   0},  { 0 , 0  }},
                                        {0,       0, { 0 ,   0}, { 0 ,   0}, {0  ,   0},  { 0 , 0  }},1},
 { STVTG_TIMING_MODE_COUNT,             {0,       0, { 0 ,   0}, { 0 ,   0}, {0  ,   0},  { 0 , 0  }},
                                        {0,       0, { 0 ,   0}, { 0 ,   0}, {0  ,   0},  { 0 , 0  }},
                                        {0,       0, { 0 ,   0}, { 0 ,   0}, {0  ,   0},  { 0 , 0  }},1}/*should stay last*/
 };
#endif
/* Private Macros --------------------------------------------------------- */
#define stvtg_Write32(a, v)     STSYS_WriteRegDev32LE((void *)(a), (v))
#define stvtg_Read32(a)         STSYS_ReadRegDev32LE((void *)(a))

/* Private Function prototypes ---------------------------------------------- */
static void MaskIT   (  const stvtg_Device_t * const Device_p, const U32 Mask);
static void UnMaskIT (  const stvtg_Device_t * const Device_p, const U32 Mask);

#if defined(ST_OSLINUX)
    static irqreturn_t stvtg_InterruptHandler(int irq, void * Param, struct pt_regs * regs);
#endif

#if defined(ST_OS21) || defined(ST_OSWINCE)
    static int stvtg_InterruptHandler(void * Param);
#endif  /* ST_OS21 || ST_OSWINCE */

#ifdef ST_OS20
	static void stvtg_InterruptHandler(void * Param);
#endif /* ST_OS20 */

void stvtg_InterruptEventHandler(  STEVT_CallReason_t Reason
                                  , const ST_DeviceName_t RegistrantName
                                  , STEVT_EventConstant_t Event
                                  , void *EventData_p
                                  , void *SubscriberData_p);
static ST_ErrorCode_t InterruptInit ( stvtg_Device_t * const Device_p);

#ifdef STVTG_NOTIFY_VSYNC_IN_TASK
static void           ProcessVTGInterrupt ( stvtg_Device_t * const Device_p);
#endif /* STVTG_NOTIFY_VSYNC_IN_TASK */

#ifdef WA_IT_TRIGGER_MODE_LEVEL
#pragma ST_translate(interrupt_trigger_mode_level, "interrupt_trigger_mode_level%os")
interrupt_trigger_mode_t interrupt_trigger_mode_level(int Level, interrupt_trigger_mode_t trigger_mode);
#endif /* #ifdef WA_IT_TRIGGER_MODE_LEVEL */


/* Functions (private)------------------------------------------------------- */

/*************************************************************************
 Name : MaskIT
 Description : Masks (disable) interrupt(s).
 Parameters : Device_p (IN) : pointer on VTG device to access
              Mask (IN) : interrupt mask
 Return : none
**************************************************************************/
static void MaskIT(  const stvtg_Device_t * const Device_p
                   , const U32 Mask
                  )
{
    stvtg_Write32( ((U32)Device_p->BaseAddress_p + Device_p->ItReg.ITM),
                   stvtg_Read32(((U32)Device_p->BaseAddress_p) + Device_p->ItReg.ITM) & ~Mask);
}

/*************************************************************************
 Name : UnMaskIT
 Description : Un-Masks (enable) interrupt(s).
 Parameters : Device_p (IN) : pointer on VTG device to access
              Mask (IN) : interrupt mask
 Return : none
**************************************************************************/
static void UnMaskIT(  const stvtg_Device_t * const Device_p
                     , const U32 Mask
                    )
{
    stvtg_Write32( ((U32)Device_p->BaseAddress_p + Device_p->ItReg.ITM),
                   stvtg_Read32(((U32)Device_p->BaseAddress_p) + Device_p->ItReg.ITM) | Mask);
}

/*******************************************************************************
Name        : stvtg_InterruptHandler
Description : When installing interrupt handler, interrupt function
Parameters  : Pointer to device
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
#if defined(ST_OSLINUX)
    static irqreturn_t stvtg_InterruptHandler(int irq, void * Param, struct pt_regs * regs)
#endif

#if defined(ST_OS21) || defined(ST_OSWINCE)
    static int stvtg_InterruptHandler(void * Param)
#endif  /* ST_OS21 || ST_OSWINCE */

#ifdef ST_OS20
    static void stvtg_InterruptHandler(void * Param)
#endif /* ST_OS20 */
{
    stvtg_Device_t * const Device_p = (stvtg_Device_t * )Param;
#if (defined (ST_7710) || defined (ST_7100)|| defined (ST_7109))
    U32 InterruptVtgStatus;
#endif
#if defined(ST_OSLINUX)
    irqreturn_t     IrqValid = IRQ_NONE;
#endif

#if !defined(STVTG_NOTIFY_VSYNC_IN_TASK)
    U8 VsyncItType;
    BOOL VSyncTopRaised;
#endif /* !STVTG_NOTIFY_VSYNC_IN_TASK */
    U32 stvtg_WA_GNBvd47682_AVMuteVal;
    /* it is mandatory here to read IT status */

    /* Get Interrupt status (ITS register), resets it. Returns only wanted interrupts (not masked).*/
    /*Device_p->InterruptStatus = stvtg_Read32(((U32)Device_p->BaseAddress_p + Device_p->ItReg.ITS)) & Device_p->InterruptMask;*/
#if defined (ST_7710) || defined (ST_7100)|| defined (ST_7109)
    Device_p->InterruptStatus = stvtg_Read32(((U32)Device_p->BaseAddress_p + Device_p->ItReg.ITS));
    /* HDCP Workarround */
    if(( stvtg_WA_GNBvd47682.Installed == TRUE) && (Device_p->InterruptStatus & VTG_IT_PDVS))
    {
        /* Disable interrupt now */
        MaskIT(Device_p, VTG_IT_PDVS);
        #if defined (ST_OSWINCE)
            /* In the following loop, we poll VTG until we see it up (either top or bottom frame)
             It has been observed that, when VTG goes up, we loose control (to whom ?).
             We return 10 micor-s later, detect VTG up, and proceed, but it is to late.
             We have only 7 micro-s to set the HDCP register.
             Locking interrupts prevents this mishap.*/
		    interrupt_lock_WinCE();
        #endif
        stvtg_WA_GNBvd47682.Installed = FALSE;
        InterruptVtgStatus = stvtg_Read32( (U32)Device_p->BaseAddress_p+ VTG1_ITS );

        while((!(InterruptVtgStatus & 0x2))&&(!(InterruptVtgStatus & 0x4)))
        {
                InterruptVtgStatus = stvtg_Read32( (U32)Device_p->BaseAddress_p+ VTG1_ITS );


        }
        if ((InterruptVtgStatus & 0x2)|| (InterruptVtgStatus & 0x4))
        {

            stvtg_WA_GNBvd47682_AVMuteVal = STSYS_ReadRegDev32LE((void *) stvtg_WA_GNBvd47682.AVMuteAddress );
            STSYS_WriteRegDev32LE((void *) stvtg_WA_GNBvd47682.AVMuteAddress, ~(U32)0x4 & stvtg_WA_GNBvd47682_AVMuteVal);
        }

        #if defined (ST_OSWINCE)
            /* CR : unable IT again*/
		    interrupt_unlock_WinCE();
        #endif
        Device_p->InterruptStatus = InterruptVtgStatus;
   }
#else
    UNUSED_PARAMETER(stvtg_WA_GNBvd47682_AVMuteVal);
    Device_p->InterruptStatus = stvtg_Read32(((U32)Device_p->BaseAddress_p + Device_p->ItReg.ITS)) & Device_p->InterruptMask;
#endif /*7710-7100-7109*/
#ifdef STVTG_NOTIFY_VSYNC_IN_TASK
    /* Interrupt is already unmasked here, so InterruptStatus may be corrupted while being used
     * in task if spurious interrupts occur. */
    STOS_SemaphoreSignal(Device_p->InterruptSemaphore_p);
#else
    VSyncTopRaised = FALSE;

    for (VsyncItType = STVTG_TOP; VsyncItType <= STVTG_BOTTOM; VsyncItType++ )
    {
        if (Device_p->InterruptStatus & Device_p->ItReg.VSType[VsyncItType])
        {
            Device_p->InterruptStatus ^= Device_p->ItReg.VSType[VsyncItType];
#ifdef WA_GNBvd08957
            if (Device_p->DeviceType == STVTG_DEVICE_TYPE_VFE)
            {
                Device_p->InterruptMask |= Device_p->ItReg.VSType[(VsyncItType+1)%2];
                Device_p->InterruptMask &= (U32)~Device_p->ItReg.VSType[VsyncItType];
            }
#endif /* #ifdef WA_GNBvd08957 */
            /* GNBvd20417: sometimes after reset both Top & Bottom it are raised, in this case
            * 'Top' event should be notified, not 'Bottom' */
            if (VsyncItType == STVTG_TOP)
            {
                VSyncTopRaised = TRUE;
                Device_p->VSyncType = VsyncItType;
            }
            else if (!VSyncTopRaised) /* only BOTTOM interrupt raised */
            {

                Device_p->VSyncType = VsyncItType;
            }
#ifdef STVTG_DEBUG_STATISTICS
            else
            {
                Device_p->Statistics.CountSimultaneousTopAndBottom ++;
            }
#endif /* STVTG_DEBUG_STATISTICS */
        }
    }

    STEVT_Notify(  Device_p->EvtHandle
                 , Device_p->EventID[STVTG_VSYNC_ID]
                 , &(Device_p->VSyncType)
                );

#endif /* #ifdef STVTG_NOTIFY_VSYNC_IN_TASK */
#if defined ST_OSLINUX
    IrqValid = IRQ_HANDLED;

    return (IrqValid);
#endif

#if defined(ST_OS21) || defined(ST_OSWINCE)
    return(OS21_SUCCESS);
#endif /* ST_OS21 */

} /* End of stvtg_InterruptHandler() function */

/*******************************************************************************
Name        : stvtg_InterruptEventHandler
Description : Recept interrupt event. Routine inside interrupt context.
Parameters  : EVT standard prototype
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
void stvtg_InterruptEventHandler(STEVT_CallReason_t Reason,
                                 const ST_DeviceName_t RegistrantName,
                                 STEVT_EventConstant_t Event,
                                 void *EventData_p,
                                 void *SubscriberData_p)
{
    UNUSED_PARAMETER(Reason);
    UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(Event);
    UNUSED_PARAMETER(EventData_p);
#ifdef ST_OSLINUX
    stvtg_InterruptHandler(0, (void *)EventData_p, NULL);
#else
    stvtg_InterruptHandler((void *)SubscriberData_p);
#endif

} /* end of stvtg_InterruptEventHandler */

/*******************************************************************************
Name        : InterruptInit
Description : link interrupt with callback function notified by interrupt event
Parameters  : Device_p (IN/OUT) : pointer on VTG device to access
Assumptions :
Limitations :
Returns     : ST_NO_ERROR or STVTG_ERROR_EVT_SUBSCRIBE
*******************************************************************************/
static ST_ErrorCode_t InterruptInit(stvtg_Device_t * const Device_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STEVT_DeviceSubscribeParams_t STEVT_SubscribeParams;
    const char VTG_string[4];
#ifdef WA_GNBvd35956
	U32	InterruptNumber;
#endif /* WA_GNBvd35956 */

    switch (Device_p->DeviceType)
    {
#if defined (STVTG_VOS) || defined (STVTG_VFE) || defined (STVTG_VFE2)
        case STVTG_DEVICE_TYPE_VTG_CELL_7015 : /* no break */
        case STVTG_DEVICE_TYPE_VTG_CELL_7020 : /* no break */
        case STVTG_DEVICE_TYPE_VFE :
            /* Subscribe to events */
            STEVT_SubscribeParams.NotifyCallback = (STEVT_DeviceCallbackProc_t)stvtg_InterruptEventHandler;
            STEVT_SubscribeParams.SubscriberData_p = (void *) (Device_p);

            ErrorCode = STEVT_SubscribeDeviceEvent( Device_p->EvtHandle,
                                                    Device_p->InterruptEventName,
                                                    Device_p->InterruptEvent,
                                                    &STEVT_SubscribeParams);
            if (ErrorCode != ST_NO_ERROR)
            {
                ErrorCode = STVTG_ERROR_EVT_SUBSCRIBE;
            }
            break;
#endif /* #ifdef STVTG_VOS || VFE || VFE2*/
#if defined (STVTG_VFE) || defined (STVTG_VFE2) || defined (STVTG_VOS)
        case STVTG_DEVICE_TYPE_VFE2 :
        case STVTG_DEVICE_TYPE_VTG_CELL_7710 :
        case STVTG_DEVICE_TYPE_VTG_CELL_7100 :
        case STVTG_DEVICE_TYPE_VTG_CELL_7200 :
            STOS_InterruptLock();
            strncpy((char *)VTG_string,"VTG", 4);

            if ( STOS_InterruptInstall(Device_p->InterruptNumber, Device_p->InterruptLevel,
                             stvtg_InterruptHandler,(char *)VTG_string,(void *) Device_p) != STOS_SUCCESS )

#ifdef ST_OS20
#ifdef WA_GNBvd35956
            /*Configure DVP register to capture MIX1 output.*/
            *(int *)( ST7710_DVP_BASE_ADDRESS ) = 0x000d0000;

			InterruptNumber = Device_p->InterruptNumber;

			if (InterruptNumber == ST7710_VOS_0_INTERRUPT)
			{
                /* Install DVP Interrupt number instead of VOS1 one.*/
                InterruptNumber = ST7710_DVP_INTERRUPT;
			}
            if ( STOS_InterruptInstall(InterruptNumber, Device_p->InterruptLevel,
                             stvtg_InterruptHandler,(char *)VTG_string,(void *) Device_p) != STOS_SUCCESS)


#else
            if ( STOS_InterruptInstall(Device_p->InterruptNumber, Device_p->InterruptLevel,
                             stvtg_InterruptHandler,(char *)VTG_string,(void *) Device_p) != STOS_SUCCESS)

#endif /* WA_GNBvd35956 */
#endif /* ST_OS20 */
            {
                ErrorCode = ST_ERROR_INTERRUPT_INSTALL;
            }

/* Work around */
#ifdef WA_IT_TRIGGER_MODE_LEVEL
            interrupt_trigger_mode_level(Device_p->InterruptLevel, interrupt_trigger_mode_high_level);
#endif /* #ifdef WA_IT_TRIGGER_MODE_LEVEL */
            STOS_InterruptUnlock();

            if (ErrorCode == ST_NO_ERROR)
            {
                  if ( STOS_InterruptEnable(Device_p->InterruptNumber,Device_p->InterruptLevel) !=  STOS_SUCCESS)
                {
                    ErrorCode = ST_ERROR_INTERRUPT_INSTALL;
                }
            }
            break;

#endif /* #ifdef STVTG_VFE || VFE2*/
        case STVTG_DEVICE_TYPE_DENC : /* no break */
        default :
            ErrorCode = ST_ERROR_BAD_PARAMETER;
            break;
    }
    return (ErrorCode);
} /* End of InterruptInit */


#ifdef STVTG_NOTIFY_VSYNC_IN_TASK
/*******************************************************************************
Name        : ProcessVTGInterrupt
Description : VTG interrupt routine
Parameters  : Device_p (IN/OUT)  : VTG device to work on.
Assumptions :
Limitations : InterruptStatus may be corrupted while being used in task if spurious
 *            interrupts occur.
Returns     : none
*******************************************************************************/
static void ProcessVTGInterrupt(stvtg_Device_t * const Device_p)
{
    U8 VsyncItType; /* one instance of the task per Device_p => one instance of VsyncItType variable per Device_p */
    BOOL VSyncTopRaised;

    while (!Device_p->ItTask.ToBeDeleted)
    {
        /* wait for an interrupt signal */

            STOS_SemaphoreWait(Device_p->InterruptSemaphore_p);

        if (!Device_p->ItTask.ToBeDeleted)
        {
            /* stay into interrupt routine until all the bits have not been tested */
            while (Device_p->InterruptStatus != 0)
            {
                VSyncTopRaised = FALSE;

                /* GNBvd20417: sometimes after reset both Top & Bottom it are raised, in this case
                * 'Top' event should be notified, not 'Bottom', so make loop decrement
                *  but do not break: clear both bits of Device_p->InterruptStatus */
                for (VsyncItType = STVTG_TOP; VsyncItType <= STVTG_BOTTOM; VsyncItType++ )
                {
                    if (Device_p->InterruptStatus & Device_p->ItReg.VSType[VsyncItType])
                    {
                        Device_p->InterruptStatus ^= Device_p->ItReg.VSType[VsyncItType];
#ifdef WA_GNBvd08957
                        Device_p->InterruptMask |= Device_p->ItReg.VSType[(VsyncItType+1)%2];
                        Device_p->InterruptMask &= (U32)~Device_p->ItReg.VSType[VsyncItType];
#endif /* #ifdef WA_GNBvd08957 */
                        /* GNBvd20417: sometimes after reset both Top & Bottom it are raised, in this case
                        * 'Top' event should be notified, not 'Bottom' */
                        if (VsyncItType == STVTG_TOP)
                        {
                            VSyncTopRaised = TRUE;
                            Device_p->VSyncType = VsyncItType;
                        }
                        else if (!VSyncTopRaised) /* only BOTTOM interrupt raised */
                        {
                            Device_p->VSyncType = VsyncItType;
                        }
#ifdef STVTG_DEBUG_STATISTICS
                        else
                        {
                            Device_p->Statistics.CountSimultaneousTopAndBottom ++;
                        }
#endif /* STVTG_DEBUG_STATISTICS */
                    }
                }
                STEVT_Notify(  Device_p->EvtHandle
                             , Device_p->EventID[STVTG_VSYNC_ID]
                             , &(Device_p->VSyncType)
                            );
#ifdef STVTG_DEBUG_STATISTICS
                /* InterruptStatus should be reset to 0, if not that means new interrupts have been raised during
                * all VSYNC callbacks processing. */
                if (Device_p->InterruptStatus != 0)
                {
                    Device_p->Statistics.CountVsyncItDuringNotify ++;
                }
#endif /* STVTG_DEBUG_STATISTICS */
            }  /* end of while (Device_p->InterruptStatus != 0) */

        } /* end of if (!Device_p->ItTask.ToBeDeleted) */
    } /*  end of while (!Device_p->ItTask.ToBeDeleted) */
} /* end of ProcessVTGInterrupt */
#endif /* #ifdef STVTG_NOTIFY_VSYNC_IN_TASK */

/* Functions (public)------------------------------------------------------- */

/*************************************************************************
 Name : stvtg_HALInit
 Description : Initialize interrupt for VTG device
 Parameters : Device_p (IN/OUT) : pointer on VTG device to access and update
 Return : ST_NO_ERROR, ST_ERROR_BAD_PARAMETER, ST_ERROR_INTERRUPT_INSTALL
 *        STVTG_ERROR_EVT_SUBSCRIBE
**************************************************************************/
ST_ErrorCode_t stvtg_HALInit(stvtg_Device_t * const Device_p, const STVTG_InitParams_t * const InitParams_p)
{
    ST_ErrorCode_t ErrorCode= ST_NO_ERROR;
#ifdef STVTG_NOTIFY_VSYNC_IN_TASK
    char InterruptTaskName[INTERRUPT_TASK_NAME_MAX_SIZE];
#endif /* #ifdef STVTG_NOTIFY_VSYNC_IN_TASK */
#ifdef STVTG_VOS
    STVTG_VtgCellSlaveParams_t *Params_p;
#endif /* #ifdef STVTG_VOS */

    /* Set registers & optional configuration init parameters */
    Device_p->SlaveModeParams.SlaveMode = STVTG_SLAVE_MODE_H_AND_V;
    Device_p->IsHSyncPositive           = FALSE;
    Device_p->IsVSyncPositive           = FALSE;
    Device_p->OutEdges.HSyncRising      = 0;
    Device_p->OutEdges.HSyncFalling     = 0;
    Device_p->OutEdges.VSyncRising      = 0;
    Device_p->OutEdges.VSyncFalling     = 0;
    Device_p->PixelsPerLine             = 0;

    switch (Device_p->DeviceType)
    {
#if defined (STVTG_VOS) || defined (STVTG_VFE) || defined (STVTG_VFE2)
        case STVTG_DEVICE_TYPE_VTG_CELL_7015 : /* no break */
        case STVTG_DEVICE_TYPE_VTG_CELL_7020 : /* no break */
        case STVTG_DEVICE_TYPE_VFE :
            /* Copy init parameters into device structure */

#if !defined(ST_OSLINUX)
            Device_p->InterruptEvent       = InitParams_p->Target.VtgCell.InterruptEvent;
            strncpy(Device_p->InterruptEventName, InitParams_p->Target.VtgCell.InterruptEventName, sizeof(Device_p->InterruptEventName));
            Device_p->InterruptEventName[ST_MAX_DEVICE_NAME - 1] = '\0';
#endif
            break;
#endif /* ##if defined STVTG_VOS || VFE || VFE2*/

#if defined (STVTG_VFE) || defined (STVTG_VFE2) || defined (STVTG_VOS)
        case STVTG_DEVICE_TYPE_VFE2 :
            /* Copy init parameters into device structure */
            Device_p->InterruptNumber      = InitParams_p->Target.VtgCell2.InterruptNumber;
            Device_p->InterruptLevel       = InitParams_p->Target.VtgCell2.InterruptLevel;

            break;

        case STVTG_DEVICE_TYPE_VTG_CELL_7710 : /* no break */
        case STVTG_DEVICE_TYPE_VTG_CELL_7100 :
        case STVTG_DEVICE_TYPE_VTG_CELL_7200 :
            /* Copy init parameters into device structure */
            Device_p->InterruptNumber      = InitParams_p->Target.VtgCell3.InterruptNumber;
            Device_p->InterruptLevel       = InitParams_p->Target.VtgCell3.InterruptLevel;
            break;

#endif /* #ifdef STVTG_VFE || VFE2*/
        case STVTG_DEVICE_TYPE_DENC : /* no break */
        default :
            return(ST_ERROR_BAD_PARAMETER);
            break;
    }

    switch (Device_p->DeviceType)
    {
#ifdef STVTG_VOS
        case STVTG_DEVICE_TYPE_VTG_CELL_7015 : /* no break */
        case STVTG_DEVICE_TYPE_VTG_CELL_7020 :
        case STVTG_DEVICE_TYPE_VTG_CELL_7710 :
        case STVTG_DEVICE_TYPE_VTG_CELL_7100 :
        case STVTG_DEVICE_TYPE_VTG_CELL_7200 :

            Device_p->ItReg.ITS                  = VTG_VOS_ITS;
            Device_p->ItReg.ITM                  = VTG_VOS_ITM;
            Device_p->ItReg.MASK                 = VTG_VOS_IT_MASK;
            Device_p->ItReg.VSType[STVTG_TOP]    = VTG_VOS_IT_VST;
            Device_p->ItReg.VSType[STVTG_BOTTOM] = VTG_VOS_IT_VSB;


#if defined (ST_7710) || defined(ST_7100)|| defined (ST_7109)
            if ((Device_p->VtgId == STVTG_VTG_ID_2)&&(( Device_p->DeviceType == STVTG_DEVICE_TYPE_VTG_CELL_7710 )|| ( Device_p->DeviceType == STVTG_DEVICE_TYPE_VTG_CELL_7100 )))
            {
                Device_p->ItReg.ITS              = VTG_VOS_ITS + VTG2_IT_OFFSET;
                Device_p->ItReg.ITM              = VTG_VOS_ITM + VTG2_IT_OFFSET;
            }
#ifdef WA_GNBvd35956
            else if (Device_p->VtgId == STVTG_VTG_ID_1)
            {
                Device_p->ItReg.ITS                   =(ST7710_DVP_BASE_ADDRESS - ST7710_VOS_BASE_ADDRESS + GAM_DVP_ITS);
                Device_p->ItReg.ITM                   =(ST7710_DVP_BASE_ADDRESS - ST7710_VOS_BASE_ADDRESS + GAM_DVP_ITM);
                Device_p->ItReg.MASK                  = GAM_DVP_IT_MASK;
                Device_p->ItReg.VSType[STVTG_BOTTOM]  = GAM_DVP_IT_VSB;
                Device_p->ItReg.VSType[STVTG_TOP]     = GAM_DVP_IT_VST ;
            }
#endif
#endif
            Device_p->EmbeddedSynchro            = FALSE;
            Device_p->NoOutputSignal             = FALSE;
            Device_p->OldNoOutputSignal          = FALSE;
            Device_p->HVDriveSave                = 0;
            Params_p = (STVTG_VtgCellSlaveParams_t *)&(Device_p->SlaveModeParams.Target.VtgCell);

            switch (Device_p->VtgId)
            {
                case STVTG_VTG_ID_1 :
                    if (( Device_p->DeviceType != STVTG_DEVICE_TYPE_VTG_CELL_7710 ) &&
                        ( Device_p->DeviceType != STVTG_DEVICE_TYPE_VTG_CELL_7100 ) &&
                        ( Device_p->DeviceType != STVTG_DEVICE_TYPE_VTG_CELL_7200 )) /* VTG1 can not be slave */
                    {
                        Params_p->HSlaveOf = STVTG_SLAVE_OF_PIN; /* 7015/20 : cannot be REF */
                        Params_p->VSlaveOf = STVTG_SLAVE_OF_PIN; /* 7015/20 : cannot be REF */
                    }
#ifdef STVTG_NOTIFY_VSYNC_IN_TASK
                    strncpy(InterruptTaskName, INTERRUPT_TASK_NAME_VOS_1, sizeof(InterruptTaskName));
#endif /* #ifdef STVTG_NOTIFY_VSYNC_IN_TASK */
                    break;
                case STVTG_VTG_ID_2 :
                    Params_p->HSlaveOf = STVTG_SLAVE_OF_REF; /* 7015/20 : cannot be PIN */
                    Params_p->VSlaveOf = STVTG_SLAVE_OF_REF; /* 7015/20 : cannot be PIN */
#ifdef STVTG_NOTIFY_VSYNC_IN_TASK
                    strncpy(InterruptTaskName, INTERRUPT_TASK_NAME_VOS_2, sizeof(InterruptTaskName));
#endif /* #ifdef STVTG_NOTIFY_VSYNC_IN_TASK */
                    break;
#if defined (ST_7200)|| defined (ST_7111)|| defined (ST_7105)
                case STVTG_VTG_ID_3 :
                    Params_p->HSlaveOf = STVTG_SLAVE_OF_REF;
                    Params_p->VSlaveOf = STVTG_SLAVE_OF_REF;
#endif
                default : /* filtered before */
                    break;
            }
            Params_p->BottomFieldDetectMin = NTSC_QUARTER_LINE;
            Params_p->BottomFieldDetectMax = NTSC_THREE_QUARTER_LINE;
            Params_p->ExtVSyncShape        = STVTG_EXT_VSYNC_SHAPE_PULSE;
            Params_p->IsExtVSyncRisingEdge = TRUE;
            Params_p->IsExtHSyncRisingEdge = TRUE;
            break;

#endif /* #ifdef STVTG_VOS */
#if defined (STVTG_VFE) || defined (STVTG_VFE2)
        case STVTG_DEVICE_TYPE_VFE : /* no break */
        case STVTG_DEVICE_TYPE_VFE2:
            Device_p->ItReg.ITS                  = VTG_VFE_ITS;
            Device_p->ItReg.ITM                  = VTG_VFE_ITM;
            Device_p->ItReg.MASK                 = VTG_VFE_IT_MASK;
            Device_p->ItReg.VSType[STVTG_TOP]    = VTG_VFE_IT_VST;
            Device_p->ItReg.VSType[STVTG_BOTTOM] = VTG_VFE_IT_VSB;
            Device_p->SlaveModeParams.Target.VfeSlaveOf = STVTG_SLAVE_OF_EXT0;
#ifdef STVTG_NOTIFY_VSYNC_IN_TASK
            strncpy(InterruptTaskName, INTERRUPT_TASK_NAME_VFE, sizeof(InterruptTaskName));
#endif /* #ifdef STVTG_NOTIFY_VSYNC_IN_TASK */
            break;
#endif /* #ifdef STVTG_VFE || VFE2*/
        case STVTG_DEVICE_TYPE_DENC : /* no break */
        default :
            return(ST_ERROR_BAD_PARAMETER);
            break;
    }

    /* ------  Interrupt handling initialization -------  */
    /* Initialize interrupt Mask */
    Device_p->InterruptMask = 0;    /* All interrupts Masked */

#ifdef STVTG_NOTIFY_VSYNC_IN_TASK
    InterruptTaskName[INTERRUPT_TASK_NAME_MAX_SIZE-1] = '\0';
    /* Setup interrupt task */
    Device_p->InterruptSemaphore_p = STOS_SemaphoreCreateFifo(NULL,0);
    Device_p->ItTask.ToBeDeleted = FALSE;

    ErrorCode = STOS_TaskCreate ((void (*) (void*))ProcessVTGInterrupt,
        (void*)Device_p,
        NULL,
        INTERRUPT_TASK_STACK_SIZE,
        NULL,
        NULL,
        &(Device_p->ItTask.Task_p),
        NULL,
        STVTG_TASK_PRIORITY_IT_PROCESSING,
        InterruptTaskName,
        0 /*task_flags_no_min_stack_size*/);
    if (ErrorCode != ST_NO_ERROR)    /* error */
    {
        STOS_SemaphoreDelete(NULL,Device_p->InterruptSemaphore_p);
        ErrorCode = ST_ERROR_INTERRUPT_INSTALL;
    }
    else
    {
        ErrorCode = InterruptInit(Device_p);
        if (ErrorCode == ST_NO_ERROR)
        {
            Device_p->ItTask.IsRunning = TRUE;
        }
    }
#else /* #ifdef STVTG_NOTIFY_VSYNC_IN_TASK */
    ErrorCode = InterruptInit(Device_p);
#endif /* #ifdef STVTG_NOTIFY_VSYNC_IN_TASK */

#ifdef STVTG_DEBUG_STATISTICS
    Device_p->Statistics.CountSimultaneousTopAndBottom = 0;
    Device_p->Statistics.CountVsyncItDuringNotify      = 0;
#endif

    return (ErrorCode);
} /* end of stvtg_HALInit */


/*************************************************************************
 Name : stvtg_HALSetInterrupt
 Description : Set Interrupts.
 Parameters : Device_p (IN/OUT) : pointer on VTG device to access and update
 Return : none
**************************************************************************/
void stvtg_HALSetInterrupt(stvtg_Device_t * const Device_p)
{
    Device_p->InterruptMask |= Device_p->ItReg.VSType[STVTG_TOP] | Device_p->ItReg.VSType[STVTG_BOTTOM];

#ifdef WA_GNBvd08957
    Device_p->InterruptMask &= (U32)~Device_p->ItReg.VSType[STVTG_BOTTOM]; /* keep only one, e.g. VST */
#endif /* #ifdef WA_GNBvd08957 */

    UnMaskIT(Device_p, Device_p->InterruptMask);

} /* end of stvtg_HALSetInterrupt */

/*************************************************************************
Name        : stvtg_HALSetOptionalConfiguration
Description : set optional configuration (modify default values set at init)
Parameters  : Device_p (IN/OUT) : pointer onto VTG device to access and update.
Assumptions : Device_p is reliable (not NULL and good address)
 *            HVDRIVE value has already been saved. Option has been checked.
Limitations :
Return      : ST_NO_ERROR, ST_ERROR_BAD_PARAMETER, ST_ERROR_FEATURE_NOT_SUPPORTED
**************************************************************************/
ST_ErrorCode_t stvtg_HALSetOptionalConfiguration( stvtg_Device_t * const Device_p)
{
    ST_ErrorCode_t ErrorCode= ST_NO_ERROR;

    switch (Device_p->Option)
    {
#ifdef STVTG_VOS
        case STVTG_OPTION_EMBEDDED_SYNCHRO :
            /* so DeviceType is STVTG_DEVICE_TYPE_VTG_CELL_7015 or 7020 or 7710 */
            if (Device_p->EmbeddedSynchro)
            {
                ErrorCode = stvtg_HALSetSyncInSettings(Device_p);
            }
            break;
        case STVTG_OPTION_NO_OUTPUT_SIGNAL :
#if !(defined (ST_7200)|| defined (ST_7111)|| defined (ST_7105))
            /* so DeviceType is STVTG_DEVICE_TYPE_VTG_CELL_7015 or 7020 */
            /* Device_p->OldNoOutputSignal : old value => status. TRUE => output disabled */
            /* Device_p->NoOutputSignal    : new value => action. TRUE => disable output */
            if (!Device_p->OldNoOutputSignal && Device_p->NoOutputSignal && Device_p->VtgId == STVTG_VTG_ID_1)
            {
                /* Set VSync and HSync signal direction as inputs */
                stvtg_Write32(((U32)Device_p->BaseAddress_p + VTG_HVDRIVE), 0);
            }
            if (Device_p->OldNoOutputSignal && !Device_p->NoOutputSignal && Device_p->VtgId == STVTG_VTG_ID_1)
            {
                stvtg_Write32(((U32)Device_p->BaseAddress_p + VTG_HVDRIVE), Device_p->HVDriveSave);
            }
#endif
           break;
#endif /* #ifdef STVTG_VOS */
        case STVTG_OPTION_HSYNC_POLARITY :    /* no break */
        case STVTG_OPTION_VSYNC_POLARITY :    /* no break */
        case STVTG_OPTION_OUT_EDGE_POSITION :
            stvtg_HALSetSyncEdges(Device_p);
            break;
        default :
            ErrorCode = ST_ERROR_BAD_PARAMETER;
            break;
    }

    return (ErrorCode);
} /* end of stvtg_HALSetOptionalConfiguration */

/*************************************************************************
Name : stvtg_HALSetSyncEdges
Description : Sets HSync/VSync polarities and edges positions
Parameters : Device_p (IN)        : pointer on VTG device to access
Assumptions : pointer on device is not NULL, and values are checked yet.
Limitations :
Return : none
**************************************************************************/
void stvtg_HALSetSyncEdges( const stvtg_Device_t * const Device_p)
{
    U16 Hdo, Hds, Vdo, Vds;
#if defined (STVTG_VFE) || defined (STVTG_VFE2)
    U16 VHdo;
#endif /* #ifdef STVTG_VFE || VFE2*/
#if defined (ST_7200)|| defined (ST_7111)|| defined (ST_7105)
    OutEdgePosition_t EdgePosition;
    U8 Index=0;
#endif
    if (Device_p->IsHSyncPositive)
    {
        Hdo = Device_p->OutEdges.HSyncRising;
        Hds = Device_p->OutEdges.HSyncFalling;
    }
    else
    {
        Hdo = Device_p->OutEdges.HSyncFalling;
        Hds = Device_p->OutEdges.HSyncRising;
    }

    if (Device_p->IsVSyncPositive)
    {
        Vdo = Device_p->OutEdges.VSyncRising;
        Vds = Device_p->OutEdges.VSyncFalling;
    }
    else
    {
        Vdo = Device_p->OutEdges.VSyncFalling;
        Vds = Device_p->OutEdges.VSyncRising;
    }

    switch (Device_p->DeviceType)
    {
#if defined (STVTG_VFE) || defined (STVTG_VFE2)
        case STVTG_DEVICE_TYPE_VFE :
            stvtg_Write32( ((U32)Device_p->BaseAddress_p + VTG_VFE_H_HD),
                           (Hdo  & VTG_VFE_H_HDO_MASK) | (Hds << 16 & VTG_VFE_H_HDS_MASK));

            VHdo = Hdo + Device_p->PixelsPerLine;
            stvtg_Write32( ((U32)Device_p->BaseAddress_p + VTG_VFE_TOP_V_VD),
                           (Vdo  & VTG_VFE_TOP_V_VD0_MASK) | (Vds << 16 & VTG_VFE_TOP_V_VDS_MASK));
            stvtg_Write32( ((U32)Device_p->BaseAddress_p + VTG_VFE_BOT_V_VD),
                           (Vdo  & VTG_VFE_BOT_V_VD0_MASK) | (Vds << 16 & VTG_VFE_BOT_V_VDS_MASK));
            stvtg_Write32( ((U32)Device_p->BaseAddress_p + VTG_VFE_TOP_V_HD),
                           (0    & VTG_VFE_TOP_V_HD0_MASK) | (0   << 16 & VTG_VFE_TOP_V_HDS_MASK));
            stvtg_Write32( ((U32)Device_p->BaseAddress_p + VTG_VFE_BOT_V_HD),
                           (VHdo & VTG_VFE_BOT_V_HD0_MASK) | (0   << 16 & VTG_VFE_BOT_V_HDS_MASK));
            break;

        case STVTG_DEVICE_TYPE_VFE2:

            stvtg_Write32( ((U32)Device_p->BaseAddress_p + VTG_VFE_H_HD),
                           (Hdo  & VTG_VFE_H_HDO_MASK) | (Hds << 16 & VTG_VFE_H_HDS_MASK));

            VHdo = Hdo + Device_p->PixelsPerLine;
            stvtg_Write32( ((U32)Device_p->BaseAddress_p + VTG_VFE_TOP_V_VD),
                           (Vdo  & VTG_VFE_TOP_V_VD0_MASK) | (Vds << 16 & VTG_VFE_TOP_V_VDS_MASK));
            stvtg_Write32( ((U32)Device_p->BaseAddress_p + VTG_VFE_BOT_V_VD),
                           (0  & VTG_VFE_BOT_V_VD0_MASK) | (0 << 16 & VTG_VFE_BOT_V_VDS_MASK));
            stvtg_Write32( ((U32)Device_p->BaseAddress_p + VTG_VFE_TOP_V_HD),
                           (0    & VTG_VFE_TOP_V_HD0_MASK) | (0   << 16 & VTG_VFE_TOP_V_HDS_MASK));

#if defined(ST_5528)|| defined(ST_4629)|| defined(ST_5100)|| defined(ST_5105)|| defined(ST_5301)|| defined(ST_5188) \
 || defined(ST_5525)|| defined(ST_5107)|| defined(ST_5162)
          stvtg_Write32( ((U32)Device_p->BaseAddress_p + VTG_VFE_BOT_V_HD),
                           (0 & VTG_VFE_BOT_V_HD0_MASK) | (0   << 16 & VTG_VFE_BOT_V_HDS_MASK));

#else

            stvtg_Write32( ((U32)Device_p->BaseAddress_p + VTG_VFE_BOT_V_HD),
                           (VHdo & VTG_VFE_BOT_V_HD0_MASK) | (0   << 16 & VTG_VFE_BOT_V_HDS_MASK));

#endif
#if defined(ST_5105)|| defined(ST_5188)|| defined(ST_5525)|| defined(ST_5107)|| defined(ST_5162)

            stvtg_Write32( ((U32)Device_p->BaseAddress_p + VTG_VFE_H_HD_2),
                           (VTG_VFE_H_HD_2_HO_PAL  & VTG_VFE_H_HDO_MASK) | (VTG_VFE_H_HD_2_HS_PAL << 16 & VTG_VFE_H_HDS_MASK));
            stvtg_Write32( ((U32)Device_p->BaseAddress_p + VTG_VFE_TOP_V_VD_2),
                           (VTG_VFE_TOP_V_VD_2_VO_PAL & VTG_VFE_TOP_V_VD0_MASK) | (VTG_VFE_TOP_V_VD_2_VS_PAL << 16 & VTG_VFE_TOP_V_VDS_MASK));
            stvtg_Write32( ((U32)Device_p->BaseAddress_p + VTG_VFE_BOT_V_VD_2),
                           (VTG_VFE_BOT_V_VD_2_VO_PAL  & VTG_VFE_BOT_V_VD0_MASK) | (VTG_VFE_BOT_V_VD_2_VS_PAL << 16 & VTG_VFE_BOT_V_VDS_MASK));
            stvtg_Write32( ((U32)Device_p->BaseAddress_p + VTG_VFE_TOP_V_HD_2),
                           (0    & VTG_VFE_TOP_V_HD0_MASK) | (0   << 16 & VTG_VFE_TOP_V_HDS_MASK));
            stvtg_Write32( ((U32)Device_p->BaseAddress_p + VTG_VFE_BOT_V_HD_2),
                           (0 & VTG_VFE_BOT_V_HD0_MASK) | (0   << 16 & VTG_VFE_BOT_V_HDS_MASK));




#endif

        break;

#endif /* #ifdef STVTG_VFE || VFE2*/
#ifdef STVTG_VOS
        case STVTG_DEVICE_TYPE_VTG_CELL_7015 : /* no break */
        case STVTG_DEVICE_TYPE_VTG_CELL_7020 :
        case STVTG_DEVICE_TYPE_VTG_CELL_7710 :
        case STVTG_DEVICE_TYPE_VTG_CELL_7100 :
            #if !(defined(ST_7200)||defined(ST_7111)|| defined (ST_7105))
                stvtg_Write32(((U32)Device_p->BaseAddress_p + VTG_VOS_HDO), Hdo);
                stvtg_Write32(((U32)Device_p->BaseAddress_p + VTG_VOS_HDS), Hds);
                stvtg_Write32(((U32)Device_p->BaseAddress_p + VTG_VOS_VDO), Vdo);
                stvtg_Write32(((U32)Device_p->BaseAddress_p + VTG_VOS_VDS), Vds);
            #endif
            break;
#if defined (ST_7200)|| defined (ST_7111)|| defined (ST_7105)
        case STVTG_DEVICE_TYPE_VTG_CELL_7200 :
            while(ModeVideoActiveTable[Index].TimingMode != STVTG_TIMING_MODE_COUNT)
            {
                if (ModeVideoActiveTable[Index].TimingMode == Device_p->TimingMode)
                {

                   if (((ModeVideoActiveTable[Index].SyncEnabled)&HSYNC_VSYNC_1)== HSYNC_VSYNC_1)
                   {

                    EdgePosition  = ModeVideoActiveTable[Index].OutEdgePosition1;

                    stvtg_Write32( ((U32)Device_p->BaseAddress_p + VTG_H_HD1),
                                   ( EdgePosition.HSyncRising & VTG_VOS_H_HDO_MASK) |
                                   ( EdgePosition.HSyncFalling << 16 & VTG_VOS_H_HDS_MASK));

                    stvtg_Write32( ((U32)Device_p->BaseAddress_p + VTG_TOP_V_VD1),
                                   (EdgePosition.VSyncTopRising.VerticalPositionEdge  & VTG_VOS_TOP_V_VD0_MASK) |
                                   (EdgePosition.VSyncTopFalling.VerticalPositionEdge << 16 & VTG_VOS_TOP_V_VDS_MASK));

                    stvtg_Write32( ((U32)Device_p->BaseAddress_p + VTG_BOT_V_VD1),
                                   (EdgePosition.VSyncBotRising.VerticalPositionEdge  & VTG_VOS_BOT_V_VD0_MASK) |
                                   (EdgePosition.VSyncBotFalling.VerticalPositionEdge << 16 & VTG_VOS_BOT_V_VDS_MASK));

                    stvtg_Write32( ((U32)Device_p->BaseAddress_p + VTG_TOP_V_HD1),
                                   (EdgePosition.VSyncTopRising.HorizontalPositionEdge  & VTG_VOS_TOP_V_HD0_MASK) |
                                   (EdgePosition.VSyncTopFalling.HorizontalPositionEdge << 16 & VTG_VOS_TOP_V_HDS_MASK));

                    stvtg_Write32( ((U32)Device_p->BaseAddress_p + VTG_BOT_V_HD1),
                                   (EdgePosition.VSyncBotRising.HorizontalPositionEdge  & VTG_VOS_BOT_V_HD0_MASK) |
                                   (EdgePosition.VSyncBotFalling.HorizontalPositionEdge  << 16 & VTG_VOS_BOT_V_HDS_MASK));
                    }
                    if (((ModeVideoActiveTable[Index].SyncEnabled)&HSYNC_VSYNC_2)==HSYNC_VSYNC_2)
                    {
                      EdgePosition  = ModeVideoActiveTable[Index].OutEdgePosition2;

                      stvtg_Write32( ((U32)Device_p->BaseAddress_p + VTG_H_HD2),
                                   ( EdgePosition.HSyncRising & VTG_VOS_H_HDO_MASK) |
                                   ( EdgePosition.HSyncFalling << 16 & VTG_VOS_H_HDS_MASK));

                      stvtg_Write32( ((U32)Device_p->BaseAddress_p + VTG_TOP_V_VD2),
                                   (EdgePosition.VSyncTopRising.VerticalPositionEdge  & VTG_VOS_TOP_V_VD0_MASK) |
                                   (EdgePosition.VSyncTopFalling.VerticalPositionEdge << 16 & VTG_VOS_TOP_V_VDS_MASK));

                      stvtg_Write32( ((U32)Device_p->BaseAddress_p + VTG_BOT_V_VD2),
                                   (EdgePosition.VSyncBotRising.VerticalPositionEdge  & VTG_VOS_BOT_V_VD0_MASK) |
                                   (EdgePosition.VSyncBotFalling.VerticalPositionEdge << 16 & VTG_VOS_BOT_V_VDS_MASK));

                      stvtg_Write32( ((U32)Device_p->BaseAddress_p + VTG_TOP_V_HD2),
                                   (EdgePosition.VSyncTopRising.HorizontalPositionEdge  & VTG_VOS_TOP_V_HD0_MASK) |
                                   (EdgePosition.VSyncTopFalling.HorizontalPositionEdge << 16 & VTG_VOS_TOP_V_HDS_MASK));

                      stvtg_Write32( ((U32)Device_p->BaseAddress_p + VTG_BOT_V_HD2),
                                   (EdgePosition.VSyncBotRising.HorizontalPositionEdge  & VTG_VOS_BOT_V_HD0_MASK) |
                                   (EdgePosition.VSyncBotFalling.HorizontalPositionEdge  << 16 & VTG_VOS_BOT_V_HDS_MASK));
                    }
                    if (((ModeVideoActiveTable[Index].SyncEnabled)&HSYNC_VSYNC_3)== HSYNC_VSYNC_3)
                    {
                      EdgePosition  = ModeVideoActiveTable[Index].OutEdgePosition3;

                      stvtg_Write32( ((U32)Device_p->BaseAddress_p + VTG_H_HD3),
                                   ( EdgePosition.HSyncRising & VTG_VOS_H_HDO_MASK) |
                                   ( EdgePosition.HSyncFalling << 16 & VTG_VOS_H_HDS_MASK));

                      stvtg_Write32( ((U32)Device_p->BaseAddress_p + VTG_TOP_V_VD3),
                                   (EdgePosition.VSyncTopRising.VerticalPositionEdge  & VTG_VOS_TOP_V_VD0_MASK) |
                                   (EdgePosition.VSyncTopFalling.VerticalPositionEdge << 16 & VTG_VOS_TOP_V_VDS_MASK));

                      stvtg_Write32( ((U32)Device_p->BaseAddress_p + VTG_BOT_V_VD3),
                                   (EdgePosition.VSyncBotRising.VerticalPositionEdge  & VTG_VOS_BOT_V_VD0_MASK) |
                                   (EdgePosition.VSyncBotFalling.VerticalPositionEdge << 16 & VTG_VOS_BOT_V_VDS_MASK));

                      stvtg_Write32( ((U32)Device_p->BaseAddress_p + VTG_TOP_V_HD3),
                                   (EdgePosition.VSyncTopRising.HorizontalPositionEdge  & VTG_VOS_TOP_V_HD0_MASK) |
                                   (EdgePosition.VSyncTopFalling.HorizontalPositionEdge << 16 & VTG_VOS_TOP_V_HDS_MASK));

                      stvtg_Write32( ((U32)Device_p->BaseAddress_p + VTG_BOT_V_HD3),
                                   (EdgePosition.VSyncBotRising.HorizontalPositionEdge  & VTG_VOS_BOT_V_HD0_MASK) |
                                   (EdgePosition.VSyncBotFalling.HorizontalPositionEdge  << 16 & VTG_VOS_BOT_V_HDS_MASK));
                    }
                }
              Index++;
           }
         break;
#endif
#endif /* #ifdef STVTG_VOS */
        case STVTG_DEVICE_TYPE_DENC : /* no break */
        default : /* checked should not fall here */
            break;
    }
} /* end of stvtg_HALSetSyncEdges */

/*************************************************************************
 Name : stvtg_HALInterruptTerm
 Description : Terminate interrupt for VTG device
 Parameters : Device_p (IN/OUT) : pointer on VTG device to access and update
 Return : ST_NO_ERROR, ST_ERROR_INTERRUPT_UNINSTALL
**************************************************************************/
ST_ErrorCode_t stvtg_HALInterruptTerm(stvtg_Device_t * const Device_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
#ifdef STVTG_NOTIFY_VSYNC_IN_TASK
    /*task_t *TaskPointer_p;*/
    int Status =-1;
#endif /* #ifdef STVTG_NOTIFY_VSYNC_IN_TASK */

    /* Masks (disable) interrupt(s) */
    MaskIT(Device_p, Device_p->ItReg.MASK);
STOS_InterruptUninstall(Device_p->InterruptNumber,Device_p->InterruptLevel,(void *)Device_p);
#ifdef STVTG_NOTIFY_VSYNC_IN_TASK
    /* Stop the interrupt processing task */
    Device_p->ItTask.ToBeDeleted = TRUE;
    STOS_SemaphoreSignal(Device_p->InterruptSemaphore_p);

    switch (Device_p->DeviceType)
    {
#if defined (STVTG_VOS) || defined (STVTG_VFE) || defined (STVTG_VFE2)
        case STVTG_DEVICE_TYPE_VTG_CELL_7015 : /* no break */
        case STVTG_DEVICE_TYPE_VTG_CELL_7020 : /* no break */
        case STVTG_DEVICE_TYPE_VTG_CELL_7710 : /* no break */
        case STVTG_DEVICE_TYPE_VTG_CELL_7100 : /* no break */
        case STVTG_DEVICE_TYPE_VTG_CELL_7200 : /* no break */
        case STVTG_DEVICE_TYPE_VFE2 :
            Status = STOS_TaskWait (&(Device_p->ItTask.Task_p),TIMEOUT_INFINITY);
            break;
#endif /* #ifdef STVTG_VOS || VFE || VFE2*/
#if defined (STVTG_VFE) || defined (STVTG_VFE2)
        case STVTG_DEVICE_TYPE_VFE :
            Status = 0; /* task_wait(&TaskPointer_p, 1, TIMEOUT_INFINITY); hangs on ST40GX1 */
            break;
#endif /* #ifdef STVTG_VFE || VFE2*/
        case STVTG_DEVICE_TYPE_DENC : /* no break */
        default : /* checked should not fall here */
            break;
    }

    if (Status == 0)
    {
        Status = STOS_TaskDelete(Device_p->ItTask.Task_p,NULL,NULL,NULL);

    }

    if (Status != 0)
    {
        ErrorCode = ST_ERROR_INTERRUPT_UNINSTALL;
    }
    else
    {
        Device_p->ItTask.IsRunning = FALSE;
        STOS_SemaphoreDelete(NULL,Device_p->InterruptSemaphore_p);
    }
#endif /* #ifdef STVTG_NOTIFY_VSYNC_IN_TASK */

    return (ErrorCode);
} /* end of stvtg_HALInterruptTerm */
#if defined (ST_7710) || defined (ST_7100)|| defined (ST_7109)
/*************************************************************************
 Name : stvtg_WA_GNBvd47682_VTimer_Raise
 Description : Program the interrupt related to the VTimer
 Parameters : Handle, ItNumberToRaise
 Return : ST_ErrorCode_t
**************************************************************************/
ST_ErrorCode_t stvtg_WA_GNBvd47682_VTimer_Raise(STVTG_Handle_t Handle, U32 AVMuteAddress)
{
    stvtg_Device_t *Device_p;
    ModeParamsLine_t *Line_p;
    if (!IsValidHandle(Handle))
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    Device_p = (stvtg_Device_t *) (((stvtg_Unit_t *)Handle)->Device_p);

    /* Get corresponding mode parameters in table */
    Line_p = stvtg_GetModeParamsLine(Device_p->TimingMode);

    /* Program wake-up before end of VSync */


    stvtg_Write32(((U32)Device_p->BaseAddress_p + VTG_VTIMER),
                  Line_p->RegParams.PixelsPerLine * (Line_p->RegParams.LinesByFrame-5) /
                  ((Line_p->ModeParams.ScanType == STGXOBJ_INTERLACED_SCAN) ? 2 : 1));


    /* Program interrupt to raise */
    stvtg_WA_GNBvd47682.AVMuteAddress = AVMuteAddress;
    stvtg_WA_GNBvd47682.Installed = TRUE;

    /* Enable interrupt now */

    UnMaskIT(Device_p, VTG_IT_PDVS);
    return(ST_NO_ERROR);
}
#endif
/* ----------------------------- End of file ------------------------------ */





