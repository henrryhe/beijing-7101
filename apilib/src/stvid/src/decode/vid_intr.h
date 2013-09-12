/*******************************************************************************

File name   : vid_intr.h

Description : Video decode interrupt header file

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
18 Jul 2000        Created                                          HG
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __VIDDEC_INTERRUPT_H
#define __VIDDEC_INTERRUPT_H


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"

#include "vid_dec.h"

#include "stevt.h"


/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */


/* Exported Types ----------------------------------------------------------- */


/* Exported Variables ------------------------------------------------------- */


/* Exported Macros ---------------------------------------------------------- */


/* Exported Functions ------------------------------------------------------- */


void viddec_InterruptHandler(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
        STEVT_EventConstant_t Event, const void *EventData_p, const void *SubscriberData_p);
void viddec_ProcessInterrupt(const VIDDEC_Handle_t DecodeHandle, const U32 InterruptStatus);


/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __VIDDEC_INTERRUPT_H */

/* End of vid_intr.h */
