/******************************************************************************

File name   : 5512_reg.h

Description : VIDEO registers specific STi5512

COPYRIGHT (C) ST-Microelectronics 2000.

Date               Modification                 Name
----               ------------                 ----
08/07/00           Created                     Alexandre Nabais
******************************************************************************/

#ifndef __HAL_LAYER_REG_5512_H
#define __HAL_LAYER_REG_5512_H

/* Includes ---------------------------------------------------------------- */
#include "stddefs.h"
#include "stsys.h"

#ifdef __cplusplus
extern "C" {
#endif

/*- Pan & Scan --------------------------------------------------------------*/
#define VID_SCN_YDO_8       0x6E  /* Video Scan vector [3:1] rows of line */
#define VID_SCN_YDO_MASK    0x0E

/* Zoom out x4 */
#define VID_DCF_STP_DEC_MASK    1
#define VID_DCF_STP_DEC_EMP     0


#ifdef __cplusplus
}
#endif

#endif /* __HAL_LAYER_REG_5512_H */
