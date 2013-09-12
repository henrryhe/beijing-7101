/*******************************************************************************
File name   : hdmi_int.h

Description : Header file for hdmi intrruptions

COPYRIGHT (C) STMicroelectronics 2004.

Date               Modification                                     Name
----               ------------                                     ----
3  Apr 2004        Created                                          AC
17 Nov 2004        Add Support of ST40/OS21                         AC
*******************************************************************************/

#ifndef __INTR_H
#define __INTR_H

/* Includes ----------------------------------------------------------------- */

#include "vout_drv.h"
#include "stddefs.h"
#include "hdmi.h"
#if defined (STVOUT_HDCP_PROTECTION)
#include "hdcp.h"
#endif
#include "hdmi_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Private Types ---------------------------------------------------------- */

/* Exported Types--------------------------------------------------------- */

/* Private Constants ------------------------------------------------------ */

/* Private variables (static) --------------------------------------------- */

/* Global Variables ------------------------------------------------------- */

/* Private Macros --------------------------------------------------------- */

/* Exported Functions ----------------------------------------------------- */
void stvout_EnableHdmiInterrupt        (const HDMI_Handle_t Handle);
void stvout_DisableHdmiInterrupt       (const HDMI_Handle_t Handle);
void stvout_InterruptHandler           (STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
                                        STEVT_EventConstant_t Event, void *EventData_p, void *SubscriberData_p);
void stvout_ProcessInterrupt           (const HDMI_Handle_t Handle, U32 InterruptStatus);
void stvout_InterruptProcessTask       (const HDMI_Handle_t Handle);

#if defined(ST_OS21) || defined(ST_OSWINCE)
   int SelfInstalledHdmiInterruptHandler (void * Param);
#endif /* ST_OS21 */
#ifdef ST_OS20
   void SelfInstalledHdmiInterruptHandler (void * Param);
#endif /* ST_OS20 */
#if defined(ST_OSLINUX)
   irqreturn_t SelfInstalledHdmiInterruptHandler(int irq, void * Param, struct pt_regs * regs);
#endif

#ifdef WA_GNBvd44290
ST_ErrorCode_t WA_GNBvd44290_Install(const HDMI_Handle_t Handle);
ST_ErrorCode_t WA_GNBvd44290_UnInstall(const HDMI_Handle_t Handle);
#endif /* WA_GNBvd44290 */

#if defined (WA_GNBvd54182) || defined (WA_GNBvd56512)
ST_ErrorCode_t WA_PWMInterruptInstall(const HDMI_Handle_t Handle);
ST_ErrorCode_t WA_PWMInterruptUnInstall(const HDMI_Handle_t Handle);
#endif /*WA_GNBvd54182||WA_GNBvd56512*/

/* ------------------------------- End of file ---------------------------- */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __HDMI_H */


