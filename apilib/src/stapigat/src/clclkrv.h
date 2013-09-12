/*******************************************************************************

File name   : clclkrv.h

Description : CLKRV configuration initialisation header file

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                                     Name
----               ------------                                     ----
30 Apr 2002        Created                                           BS
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __CLCLKRV_H
#define __CLCLKRV_H

/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#include "stclkrv.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */
#if defined(ST_7200)
#define STCLKRV_DEVICE_NAME  "CLKRV"        /* Same name as with only one CLKRV to be backward compatible with current scripts */
#define STCLKRV0_DEVICE_NAME "CLKRV"
#define STCLKRV1_DEVICE_NAME "CLKRV1"
#else  /* #if !defined(ST_7200) ... */
#define STCLKRV_DEVICE_NAME "CLKRV"
#endif /* #if !defined(ST_7200) ... else ... */

/* Exported Types ----------------------------------------------------------- */

/* Exported Variables ------------------------------------------------------- */

/* Exported Macros ---------------------------------------------------------- */

/* Exported Functions ------------------------------------------------------- */

#if defined(ST_7200)
BOOL CLKRV_Init(ST_DeviceName_t DeviceName, void * CRUBaseAddress_p, STCLKRV_Decode_t DecodeType);
#else  /* #if defined(ST_7200) ... */
BOOL CLKRV_Init(void);
#endif /* #if !defined(ST_7200) ... else ... */
void CLKRV_Term(void);

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __CLCLKRV_H */

/* End of clclkrv.h */
