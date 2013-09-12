/*******************************************************************************

File name   : stcctask.h

Description :

COPYRIGHT (C) STMicroelectronics 2001.

Date               Modification                                     Name
----               ------------                                     ----
10 Mar 2000        Created                                     Michel Bruant
*******************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __STCC_TASK_H
#define __STCC_TASK_H

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

#include "stevt.h"
#include "stccinit.h"

void stcc_ProcessCaption(stcc_Device_t * const Device_p);
void stcc_VSyncCallBack(STEVT_CallReason_t CC_Reason,
                        const ST_DeviceName_t RegistrantName,
                        STEVT_EventConstant_t Event,
                        void* EventData,
                        void *SubscriberData_p);
void stcc_UserDataCallBack(STEVT_CallReason_t CC_Reason,
                           const ST_DeviceName_t RegistrantName,
                           STEVT_EventConstant_t Event,
                           void* EventData,
                           void *SubscriberData_p);
/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __STCC_TASK_H */

/* End of stcctask.h */
