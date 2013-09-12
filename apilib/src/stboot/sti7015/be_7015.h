/*******************************************************************************

File name : be_7015.h

Description : Back end specific header of STBOOT

COPYRIGHT (C) STMicroelectronics 2001.

Date               Modification                 Name
----               ------------                 ----
26 july 99         Created                      HG
*******************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __BACKEND_INIT7015_H
#define __BACKEND_INIT7015_H


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"


/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */


/* Exported Types ----------------------------------------------------------- */


/* Exported Variables ------------------------------------------------------- */


/* Exported Macros ---------------------------------------------------------- */


/* Exported Functions ------------------------------------------------------- */
ST_ErrorCode_t stboot_BackendInit7015(U32 SDRAMFrequency);
BOOL stboot_BoardInit(void);
#ifdef STBOOT_INSTALL_BACKEND_INTMGR
void STBOOT_RegisterSemaphore(semaphore_t *Semaphore_p, U8 InterruptLine);
void STBOOT_UnRegisterSemaphore(U8 InterruptLine);
void STBOOT_DisableBackendIT(void);
void STBOOT_EnableBackendIT(void);
void BackendInterruptHandler(void);
#endif

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __BACKEND_INIT7015_H */

/* End of be_7015.h */
