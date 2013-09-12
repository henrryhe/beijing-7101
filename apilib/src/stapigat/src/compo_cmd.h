/*******************************************************************************

File name   : compo_cmd.h

Description : COMPO commands configuration initialisation header file

COPYRIGHT (C) STMicroelectronics 2004.

Date                     Modification                                 Name
----                     ------------                                 ----
20 februay 2005          Created                                      HE
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __COMPO_CMD_H
#define __COMPO_CMD_H

/* Includes ----------------------------------------------------------------- */
#include "stddefs.h"
#include "stdevice.h"
#include "testtool.h"
#include "stcommon.h"
#include "sttbx.h"
#include "compo.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif


/* Exported Constants ------------------------------------------------------- */

/* Exported Types ----------------------------------------------------------- */

/* Exported Variables ------------------------------------------------------- */

/* Exported Macros ---------------------------------------------------------- */

/* Exported Functions ------------------------------------------------------- */
BOOL COMPO_RegisterCmd(void);


#endif /* #ifndef __COMPO_CMD_H */

/* End of compo_cmd.h */
