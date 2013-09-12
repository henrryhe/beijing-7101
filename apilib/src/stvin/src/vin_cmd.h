
/*******************************************************************************

File name   : vin_cmd.h

Description :

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
18 Dec 2001        Created                                      Michel Bruant
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __VIN_CMD_H
#define __VIN_CMD_H


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

typedef enum STVIN_FrameRate_e
{
    STVIN_FRAME_RATE_50I = 25000,
    STVIN_FRAME_RATE_60I = 30000,
    STVIN_FRAME_RATE_50P = 50000,
    STVIN_FRAME_RATE_60P = 60000
} STVIN_FrameRate_t;

/* Exported Functions ------------------------------------------------------- */
void stvin_IOWindowsChange(STEVT_CallReason_t Reason,
                 const ST_DeviceName_t RegistrantName,
                 STEVT_EventConstant_t Event,
                 const void *EventData_p,
                 void *SubscriberData_p);

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __VIN_CMD_H */

/* End of vin_cmd.h */
