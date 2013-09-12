/*******************************************************************************

File name   : denc_hal.h

Description : hardware abstraction layer for STDENC

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
06 Jun 2000        Created                                           JG
16 Oct 2000        multi-init ..., hal for STV0119 : function        JG
 *                 renamed for STV0119
15 Nov 2000        improve error tracability                         HSdLM
06 Feb 2001        Reorganize HAL for providing mutual exclusion on  HSdLM
 *                 registers access
22 Jun 2001        Exported symbols => stdenc_...                    HSdLM
30 Aou 2001        Remove dependency upon STI2C if not needed, to support   HSdLM
 *                 ST40GX1
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __DENC_HAL_H
#define __DENC_HAL_H

/* Includes ----------------------------------------------------------------- */

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */

/* Exported Types ----------------------------------------------------------- */

/* Exported Variables ------------------------------------------------------- */

/* Exported Macros ---------------------------------------------------------- */

/* Exported Functions ------------------------------------------------------- */

#ifdef STDENC_I2C
ST_ErrorCode_t stdenc_HALInitI2CConnection(    DENCDevice_t * const Device_p,
                                               const STDENC_I2CAccess_t *   const I2CAccess_p
                                          );
ST_ErrorCode_t stdenc_HALTermI2CConnection(    DENCDevice_t * const Device_p);
#endif /* #ifdef STDENC_I2C */
ST_ErrorCode_t stdenc_HALSetOn(                DENCDevice_t * const Device_p);
ST_ErrorCode_t stdenc_HALSetOff(               DENCDevice_t * const Device_p);
ST_ErrorCode_t stdenc_HALVersion(              DENCDevice_t * const Device_p);
ST_ErrorCode_t stdenc_HALSetDencId(            DENCDevice_t * const Device_p);
ST_ErrorCode_t stdenc_HALSetEncodingMode(      DENCDevice_t * const Device_p);
ST_ErrorCode_t stdenc_HALSetColorBarPattern(   DENCDevice_t * const Device_p);
ST_ErrorCode_t stdenc_HALRestoreConfiguration0(DENCDevice_t * const Device_p);


/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __DENC_HAL_H */

/* ------------------------------- End of file ---------------------------- */
