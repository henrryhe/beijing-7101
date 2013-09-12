/*******************************************************************************

File name   : vtg_vsit.c

Description : Video Vsync interrupt handler source file, used to simulate STVID handling
 * of VTG interrupts on omega1 chips family

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                                     Name
----               ------------                                     ----
06 Mar 2002        Created                                          HG
04 Jul 2002        Replace ST_55xx by BE_55xx for "Back End"        HSdLM
11 Oct 2002        Add support for STi5517                          HSdLM
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */

/* Includes ----------------------------------------------------------------- */
#if !defined(ST_OSLINUX)
#include <stdio.h>
#include <string.h>
#endif

#include "sttbx.h"

#include "stddefs.h"
#include "stsys.h"
#include "stcommon.h"
#include "stevt.h"
#include "stvtg.h"
#include "vtg_drv.h"

/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */

enum
{
    /* Reserved: should not be subscribed ! Just here for reservation of the event constant !      */
    /* #define STVTG_DRIVER_ID      42 (0x2A)                                                      */
    /* #define STVTG_DRIVER_BASE    (STVTG_DRIVER_ID << 16)                                        */
    STVTG_VSYNC_IT_SELF_INSTALLED_EVT = 0x2A0100
};

#define VID_ITS2        0x3D  /* Video interrupt status [23:16]                 */
#define VID_ITS1        0x62  /* Video interrupt status [15:8]                  */
#define VID_ITS0        0x63  /* Video interrupt status [7:0]                   */
#define VID_ITS16       0x62  /* Video interrupt status [15:0]                  */
#define VID_ITM2        0x3C  /* Video interrupt mask [23:16]                   */
#define VID_ITM16       0x60  /* Video interrupt mask [15:0]                    */
#define VID_ITX_VST   0x0040       /* Top(Odd) Vsync                        */
#define VID_ITX_VSB   0x0020       /* Bottom(Even) Vsync                    */

#define VID_CTL         0x02  /* control of the video decoder  [7:0]            */
#define VID_CTL_EDC     0x01 /* enable video decoding                         */
#define VID_CTL_ERS     0x40 /* enable pipeline reset on severe error         */
#define VID_CTL_ERU     0x80 /* enable pipeline reset on picture decode error */

/* CAUTION: Register CFG_CCF is also used in STBOOT. Therefore, access to it should be protected. */
#define CFG_CCF               0x01   /* Clock configuration [7:0] */
#define CFG_CCF_EOU           0x40 /* Enable Ovf/Udf errors */

#define VID_TP_LDP      0x3F  /* Video Load internal pointer [2:0] : 5508/18    */

/* Private Variables (static)------------------------------------------------ */

/* Global Variables --------------------------------------------------------- */

/* Private Macros ----------------------------------------------------------- */

#define VTGVSIT_Read8(Address)     STSYS_ReadRegDev8((void *) (Address))

#define VTGVSIT_Write8(Address, Value)  STSYS_WriteRegDev8((void *) (Address), (Value))
#define VTGVSIT_Write16(Address, Value) STSYS_WriteRegDev16BE((void *) (Address), (Value))

/* Private Function prototypes ---------------------------------------------- */

/* Used to generate VSync event (patch omega1 which has no VTG hardware) */
extern void stvtg_SignalVSYNC(STVTG_VSYNC_t Type);

static U32 GetInterruptStatus(const stvtg_Device_t * const Device_p);
static void SetInterruptMask(const stvtg_Device_t * const Device_p);
static void VideoInterruptHandler(void * Param_p);

/* Public Functions --------------------------------------------------------- */

/*******************************************************************************
Name        : stvtg_VideoVsyncInterruptHandler
Description : Recept interrupt event
Parameters  : EVT standard prototype
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
void stvtg_VideoVsyncInterruptHandler(STEVT_CallReason_t Reason,
                                      const ST_DeviceName_t RegistrantName,
                                      STEVT_EventConstant_t Event,
                                      void *EventData_p,
                                      void *SubscriberData_p)
{
    stvtg_Device_t * const Device_p = (stvtg_Device_t * )EventData_p;

    GetInterruptStatus(Device_p);
} /* End of stvtg_VideoVsyncInterruptHandler() function */

/*******************************************************************************
Name        : stvtg_ActivateVideoVsyncInterrupt
Description : Initialise interrupt handler for Video Vsync interrupt
Parameters  : Device_p (IN_OUT) : pointer on VTG device to access and update
 *            InitParams_p(IN) : pointer on STVTG API init parameters
Assumptions :
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_INTERRUPT_INSTALL
*******************************************************************************/
ST_ErrorCode_t stvtg_ActivateVideoVsyncInterrupt( stvtg_Device_t * const Device_p,
                                                  const STVTG_InitParams_t * const InitParams_p)
{
    STEVT_DeviceSubscribeParams_t STEVT_SubscribeParams;
    U8 Tmp8;
    ST_ErrorCode_t ErrorCode;

    Device_p->VideoBaseAddress_p    =    (U8 *)InitParams_p->VideoDeviceBaseAddress_p
                                       +  (U32)InitParams_p->VideoBaseAddress_p;
    Device_p->VideoInterruptNumber  = InitParams_p->VideoInterruptNumber;
    Device_p->VideoInterruptLevel   = InitParams_p->VideoInterruptLevel;

    VTGVSIT_Write8(Device_p->VideoBaseAddress_p + VID_CTL, VID_CTL_ERS | VID_CTL_ERU | VID_CTL_EDC);

    /* Set interrupt mask to enable VSYNC interrupts ! (patch omega1 which has no VTG hardware) */
    SetInterruptMask(Device_p);

    /* Clear pending interrupts. Most significant byte must be read last, because it clears the register */
    Tmp8 = VTGVSIT_Read8(Device_p->VideoBaseAddress_p + VID_ITS2);
    Tmp8 = VTGVSIT_Read8(Device_p->VideoBaseAddress_p + VID_ITS0);
    Tmp8 = VTGVSIT_Read8(Device_p->VideoBaseAddress_p + VID_ITS1);

#if defined (BE_5510) || defined (BE_5512)
    Tmp8 = VTGVSIT_Read8(Device_p->VideoBaseAddress_p + CFG_CCF);
    VTGVSIT_Write8(Device_p->VideoBaseAddress_p + CFG_CCF, Tmp8 &~ CFG_CCF_EOU);

#elif    defined (BE_5508) || defined (BE_5518) \
      || defined (BE_5514) || defined (BE_5516) || defined (BE_5517)
    VTGVSIT_Write8(Device_p->VideoBaseAddress_p + VID_TP_LDP, 0);

#else
#error Simulation of STVID handling of VTG interrupts manages only omega1 chips family
#endif

    /* Install interrupt handler : invent an event number that should not be used anywhere else ! */
    STEVT_SubscribeParams.NotifyCallback = (STEVT_DeviceCallbackProc_t)stvtg_VideoVsyncInterruptHandler;
    STEVT_SubscribeParams.SubscriberData_p = NULL;
    ErrorCode = STEVT_SubscribeDeviceEvent( Device_p->EvtHandle,
                                            Device_p->DeviceName,
                                            STVTG_VSYNC_IT_SELF_INSTALLED_EVT,
                                            &STEVT_SubscribeParams);
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: subscribe failed, undo initialisations done */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "stvtg_ActivateVideoVsyncInterrupt: could not subscribe to internal interrupt event, error = 0x%0x\n", ErrorCode));
        ErrorCode = STVTG_ERROR_EVT_REGISTER;
    }
    else
    {
        /* Register ourselves interrupt event */
        ErrorCode = STEVT_RegisterDeviceEvent( Device_p->EvtHandle,
                                               Device_p->DeviceName,
                                               STVTG_VSYNC_IT_SELF_INSTALLED_EVT,
                                               &(Device_p->EventID[STVTG_VSYNC_IT_SELF_INSTALLED_ID]));
        if (ErrorCode != ST_NO_ERROR)
        {
            /* Error: notify failed, undo initialisations done */
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "stvtg_ActivateVideoVsyncInterrupt: could not notify internal interrupt event, error = 0x%0x\n", ErrorCode));
            ErrorCode = STVTG_ERROR_EVT_REGISTER;
        }
        else
        {
            STOS_InterruptLock();
            if ( STOS_InterruptInstall(Device_p->VideoInterruptNumber, Device_p->VideoInterruptLevel,
                             VideoInterruptHandler,"VTG",(void *) Device_p) != STOS_SUCCESS)

            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "stvtg_ActivateVideoVsyncInterrupt: Problem installing interrupt !\n"));
                ErrorCode = ST_ERROR_INTERRUPT_INSTALL;
            }
            STOS_InterruptUnlock();
            if (ErrorCode == ST_NO_ERROR)
            {

                if ( STOS_InterruptEnable(Device_p->VideoInterruptNumber,Device_p->VideoInterruptLevel) !=  STOS_SUCCESS)
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "stvtg_ActivateVideoVsyncInterrupt: Problem enabling interrupt !\n"));
                    ErrorCode = ST_ERROR_INTERRUPT_INSTALL;
                }
            }
        } /* End interrupt event register OK */
    } /* end interrupt event subscribe OK */

    return(ErrorCode);
} /* end of stvtg_ActivateVideoVsyncInterrupt() function */


/*******************************************************************************
Name        : stvtg_DeactivateVideoVsyncInterrupt
Description : disable and uninstall video VSync interrupt
Parameters  : Device_p (IN) : pointer on VTG device to access
Assumptions :
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_INTERRUPT_UNINSTALL
*******************************************************************************/
ST_ErrorCode_t stvtg_DeactivateVideoVsyncInterrupt(const stvtg_Device_t * const Device_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    if (STOS_InterruptDisable(Device_p->VideoInterruptNumber,Device_p->VideoInterruptLevel) != STOS_SUCCESS)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "stvtg_DeactivateVideoVsyncInterrupt: Problem disabling interrupt !\n"));
        ErrorCode = ST_ERROR_INTERRUPT_UNINSTALL;
    }
    STOS_InterruptLock();
    if (STOS_InterruptUninstal(Device_p->VideoInterruptNumber, Device_p->VideoInterruptLevel) != STOS_SUCCESS)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "stvtg_DeactivateVideoVsyncInterrupt: Problem un-installing interrupt !\n"));
        ErrorCode = ST_ERROR_INTERRUPT_UNINSTALL;
    }
    STOS_InterruptUnlock();
    return(ErrorCode);

} /* end of stvtg_DeactivateVideoVsyncInterrupt() function */


/* Private Functions -------------------------------------------------------- */


/*******************************************************************************
Name        : VideoInterruptHandler
Description : When installing interrupt handler ourselves, interrupt function
Parameters  : Device_p (IN) : pointer on VTG device to access
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void VideoInterruptHandler(void * Param_p)
{
    stvtg_Device_t * const Device_p = (stvtg_Device_t * )Param_p;
    ST_ErrorCode_t ErrorCode;

    ErrorCode = STEVT_Notify( Device_p->EvtHandle,
                              Device_p->EventID[STVTG_VSYNC_IT_SELF_INSTALLED_ID],
                              (const void *) Device_p);
} /* End of VideoInterruptHandler() function */


/*******************************************************************************
Name        : GetInterruptStatus
Description : Get interrupt status information
Parameters  : Device_p (IN) : pointer on VTG device to access
Assumptions : Must be called inside interrupt context
Limitations :
Returns     : interrupt status
*******************************************************************************/
static U32 GetInterruptStatus(const stvtg_Device_t * const Device_p)
{
    U32 HWStatus;

    /* Most significant byte must be read last, because it clears the register */
    HWStatus  = ((U32) VTGVSIT_Read8(Device_p->VideoBaseAddress_p + VID_ITS2)) << 16;
    HWStatus |= ((U32) VTGVSIT_Read8(Device_p->VideoBaseAddress_p + VID_ITS0));
    HWStatus |= ((U32) VTGVSIT_Read8(Device_p->VideoBaseAddress_p + VID_ITS1)) <<  8;

    /* Call hidden VTG function which generates VSync events (patch omega1 which has no VTG hardware) */
    if ((HWStatus & VID_ITX_VST) != 0)
    {
        stvtg_SignalVSYNC(STVTG_TOP);
    }
    if ((HWStatus & VID_ITX_VSB) != 0)
    {
        stvtg_SignalVSYNC(STVTG_BOTTOM);
    }

    return(0);
} /* End of GetInterruptStatus() function */


/*******************************************************************************
Name        : SetInterruptMask
Description : Set interrupt mask
Parameters  : Device_p (IN) : pointer on VTG device to access
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetInterruptMask(const stvtg_Device_t * const Device_p)
{
    U32 HWMask = 0;

    VTGVSIT_Write8( Device_p->VideoBaseAddress_p + VID_ITM2, HWMask >> 16);
    VTGVSIT_Write16(Device_p->VideoBaseAddress_p + VID_ITM16, (U16) HWMask | VID_ITX_VST | VID_ITX_VSB);
} /* End of SetInterruptMask() function */


/* End of vtg_vsit.c */
