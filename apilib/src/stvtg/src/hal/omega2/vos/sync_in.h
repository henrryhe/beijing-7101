/************************************************************************
File Name   : sync_in.h

Description : Header file of embbeded syncs Output Stage HAL

COPYRIGHT (C) ST Microelectronics 2001

Date               Modification                                     Name
----               ------------                                     ----
06 Mar 2001        Created                                          HSdLM
28 Jun 2001        Set prefix stvtg_ on exported symbols            HSdLM
************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __VTG_SYNC_IN_H
#define __VTG_SYNC_IN_H

/* Includes -----------------------------------------------------------*/

#include <stddefs.h>
#include "vtg_drv.h"


/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ----------------------------------------------------- */

/* Exported Variables ----------------------------------------------------- */

/* Exported Types -----------------------------------------------------*/

/* Exported Macros ----------------------------------------------------*/

/* Exported Functions -------------------------------------------------*/
ST_ErrorCode_t stvtg_HALSetSyncInSettings(const stvtg_Device_t * const Device_p);

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __VTG_SYNC_IN_H */

/* ------------------------------- End of file ---------------------- */
