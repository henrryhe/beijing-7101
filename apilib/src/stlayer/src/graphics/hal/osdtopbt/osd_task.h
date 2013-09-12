/*******************************************************************************

File name   : osd_task.h

Description : STLAYER API. GFX  OSD Part. Prototypes and types

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                               Name
----               ------------                               ----
2000-11-27         Created                                    Michel Bruant
*******************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __OSDTASK_H
#define __OSDTASK_H

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Includes ----------------------------------------------------------------- */

#include "stevt.h"

/* Functions ---------------------------------------------------------------- */
void            stlayer_UpdateHeadersTask(void);
ST_ErrorCode_t  stlayer_SubscribeEvents(ST_DeviceName_t NewVTG);

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __OSDTASK_H */

/* End of osd_task.h */
