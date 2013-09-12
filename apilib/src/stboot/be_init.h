/*******************************************************************************

File name : be_init.h

Description : Back end specific header of STBOOT

COPYRIGHT (C) STMicroelectronics 1999.

Date               Modification                 Name
----               ------------                 ----
26 july 99         Created                      HG
*******************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __BACKEND_INIT_H
#define __BACKEND_INIT_H


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


extern ST_ErrorCode_t stboot_BackendInit(STBOOT_Backend_t *BackendType);



/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __BACKEND_INIT_H */

/* End of be_init.h */
