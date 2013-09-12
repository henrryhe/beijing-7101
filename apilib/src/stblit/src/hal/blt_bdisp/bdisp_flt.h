/*******************************************************************************

File name   : bdisp_flt.h

Description : Contains filters tables

COPYRIGHT (C) STMicroelectronics 2001.

Date               Modification                                     Name
----               ------------                                     ----
01 Jun 2004        Created                                          HE
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __BLT_FLT_H
#define __BLT_FLT_H


/* Includes ----------------------------------------------------------------- */
#include "stddefs.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */
#define  STBLIT_DEFAULT_NUMBER_FILTERS        12
#define  STBLIT_HFILTER_COEFFICIENTS_SIZE     64 /* 64 bytes */
#define  STBLIT_VFILTER_COEFFICIENTS_SIZE     40 /* 40 bytes */
#define  STBLIT_DEFAULT_HFILTER_BUFFER_SIZE   (STBLIT_HFILTER_COEFFICIENTS_SIZE  * STBLIT_DEFAULT_NUMBER_FILTERS)
#define  STBLIT_DEFAULT_VFILTER_BUFFER_SIZE   (STBLIT_VFILTER_COEFFICIENTS_SIZE * STBLIT_DEFAULT_NUMBER_FILTERS)
#define  STBLIT_DEFAULT_FILTER_BUFFER_SIZE    (STBLIT_DEFAULT_HFILTER_BUFFER_SIZE + STBLIT_DEFAULT_VFILTER_BUFFER_SIZE)


/* Exported Types ----------------------------------------------------------- */

/* Exported Variables ------------------------------------------------------- */
extern const U8 HFilterBuffer[STBLIT_HFILTER_COEFFICIENTS_SIZE * STBLIT_DEFAULT_NUMBER_FILTERS];

extern const U8 VFilterBuffer[STBLIT_VFILTER_COEFFICIENTS_SIZE * STBLIT_DEFAULT_NUMBER_FILTERS];



/* Exported Macros ---------------------------------------------------------- */

/* Exported Functions ------------------------------------------------------- */


/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __BLT_FLT_H */
