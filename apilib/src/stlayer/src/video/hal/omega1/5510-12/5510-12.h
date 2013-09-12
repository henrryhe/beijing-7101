/*******************************************************************************

File name   : 5510-12.h

Description : HAL video decode omega1 family STi5510/12 specific header file

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
7 Aug 2000         Created                                           AN
*******************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __HAL_VIDEO_LAYER_OMEGA1_5510_5512_H
#define __HAL_VIDEO_LAYER_OMEGA1_5510_5512_H


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */
#define GNBvd01320

/* Exported Types ----------------------------------------------------------- */

/* Exported Variables ------------------------------------------------------- */

/* Exported Macros ---------------------------------------------------------- */

/* Exported Functions ------------------------------------------------------- */
/*void SetInputMargins(const STLAYER_Handle_t LayerHandle, S32 * const LeftMargin_p, S32 * const TopMargin_p, const BOOL const Apply);*/

#define VID_VFC_8         0x55
#define VID_VFC_MASK      0x1F
#define VID_VFL_8         0x54
#define VID_VFL_MASK      0x1F


/* Masks depending on chip */
#define VID_YD0_MASK     0x1FF
#define VID_YDS_MASK     0x1FF


/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __HAL_VIDEO_LAYER_OMEGA1_5510_5512_H */

/* End of 5510-12.h */
