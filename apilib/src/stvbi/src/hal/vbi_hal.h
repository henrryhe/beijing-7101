/*****************************************************************************

File name : vbi_hal.h

Description :  HAL interface for VBI programmation. Exported routines declarations.

COPYRIGHT (C) STMicroelectronics 2001.

Date               Modification                                      Name
----               ------------                                      ----
22 Feb 2001        Created                                           HSdLM
04 Jul 2001        Non-API exported symbols begin with stvbi_        HSdLM
 *                 New function to check dynamic parameters
14 Nov 2001        New function to select TTX source on ST40GX1      HSdLM
*****************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __VBI_HAL_H
#define __VBI_HAL_H

/* Includes --------------------------------------------------------------- */
#include "stvbi.h"
#include "vbi_drv.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ----------------------------------------------------- */

/* Exported Variables ----------------------------------------------------- */

/* Exported Types --------------------------------------------------------- */

/* Exported Functions  -----------------------------------------------------*/

#ifdef COPYPROTECTION
ST_ErrorCode_t stvbi_HALInitCopyProtection (       VBI_t * const Unit_p );
#endif /* #ifdef COPYPROTECTION */
ST_ErrorCode_t stvbi_HALSetTeletextConfiguration ( VBI_t * const Unit_p );
ST_ErrorCode_t stvbi_HALEnable (                   VBI_t * const Unit_p );
ST_ErrorCode_t stvbi_HALDisable (                  VBI_t * const Unit_p );
ST_ErrorCode_t stvbi_HALCheckDynamicParams ( const VBI_t * const Unit_p, const STVBI_DynamicParams_t * const DynamicParams_p);
ST_ErrorCode_t stvbi_HALSetDynamicParams (         VBI_t * const Unit_p );
ST_ErrorCode_t stvbi_HALWriteData (                VBI_t * const Unit_p, const U8* const Data_p);
ST_ErrorCode_t stvbi_HALSetTeletextSource(   const VBI_t * const Unit_p, const STVBI_TeletextSource_t TeletextSource);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef __VBI_HAL_H */

/* ------------------------------- End of file ---------------------------- */
