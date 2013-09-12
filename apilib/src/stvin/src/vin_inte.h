/*******************************************************************************

File name   : vin_inte.h

Description : Video input interrupt header file

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
10 Aou 2000        Created                                          JA
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __STVIN_INTERRUPT_H
#define __STVIN_INTERRUPT_H

/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#include "stvin.h"
#include "vin_init.h"
#include "stevt.h"
#include "vin_drv.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Functions ------------------------------------------------------- */

#if !(defined(ST_7710) || defined(ST_5528) || defined(ST_7100) || defined(ST_7109))
void stvin_InterruptHandler(STEVT_CallReason_t Reason,
        const ST_DeviceName_t RegistrantName,
        STEVT_EventConstant_t Event,
        const void *EventData_p,
        const void *SubscriberData_p);
#else
STOS_INTERRUPT_DECLARE(stvin_InterruptHandler, Param);
#endif /* defined(ST_7710) || defined(ST_7100) defined(ST_7109) */

void stvin_ProcessInterrupt(stvin_Device_t * const InputHandle,
        const U32 InterruptStatus);
ST_ErrorCode_t UpdatePictureBuffer(stvin_Device_t * const Device_p,
        const STVID_PictureStructure_t PictureStructure,
        STVID_PictureInfos_t *PictureInfos_p,
        const STVID_PictureBufferDataParams_t * const PictureBufferParams_p);

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __STVIN_INTERRUPT_H */

/* End of vin_inte.h */




