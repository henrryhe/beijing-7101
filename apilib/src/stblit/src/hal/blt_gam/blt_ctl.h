/*******************************************************************************

File name   : blt_ctl.h

Description : back-end control header file

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
20 Jun 2000        Created                                           TM
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __BLT_CTL_H
#define __BLT_CTL_H


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#include "blt_com.h"
#include "blt_init.h"


/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */


/* Exported Types ----------------------------------------------------------- */


/* Exported Variables ------------------------------------------------------- */


/* Exported Macros ---------------------------------------------------------- */


/* Exported Functions ------------------------------------------------------- */
ST_ErrorCode_t stblit_PostSubmitMessage(stblit_Device_t * const Device_p,
                                                void* FirstNode_p, void* LastNode_p,
                                                BOOL InsertAtFront, BOOL LockBeforeFirstNode);


/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __BLT_CTL_H  */

/* End of blt_ctl.h */
