/*******************************************************************************

File name   : reg_rbus.h

Description : Denc registers access by RBUS header file

COPYRIGHT (C) STMicroelectronics 2003.

Date               Modification                                     Name
----               ------------                                     ----
07 Feb 2001        Created                                          HSdLM
07 Mar 2001        remove function table exports and add routines   HSdLM
 *                 to return function table addresses
22 Jun 2001        Exported symbols => stdenc_...                   HSdLM
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __DENC_REG_RBUS_H
#define __DENC_REG_RBUS_H

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

const stdenc_RegFunctionTable_t * stdenc_GetRBUSFunctionTableAddress(void);
const stdenc_RegFunctionTable_t * stdenc_GetRBUSShiftedFunctionTableAddress(void);

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __DENC_REG_RBUS_H */

/* End of reg_rbus.h */
