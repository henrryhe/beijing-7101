/*******************************************************************************

File name   : sub_spu.h

Description : sub picture unit header file

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
18 Dec 2000        Created                                           TM
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __SUB_SPU_H
#define __SUB_SPU_H


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#include "sub_vp.h"


/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */
#define LAYERSUB_SPU_HEADER_SIZE    4
#define LAYERSUB_SPU_DCSQ_SIZE      24
#define LAYERSUB_SPU_MAX_DATA_SIZE  4096     /* max is 128 * 128 pixel in CLUT2 */
#define LAYERSUB_SPU_MAX_SIZE   (LAYERSUB_SPU_MAX_DATA_SIZE + LAYERSUB_SPU_DCSQ_SIZE + LAYERSUB_SPU_HEADER_SIZE)

/* Exported Types ----------------------------------------------------------- */


/* Exported Variables ------------------------------------------------------- */


/* Exported Macros ---------------------------------------------------------- */


/* Exported Functions ------------------------------------------------------- */
ST_ErrorCode_t layersub_SetSPUHeader(layersub_Device_t* Device_p, layersub_ViewPort_t* ViewPort_p);
ST_ErrorCode_t layersub_SetData(layersub_Device_t* Device_p, layersub_ViewPort_t* ViewPort_p);
ST_ErrorCode_t layersub_SetDCSQ(layersub_Device_t* Device_p, layersub_ViewPort_t* ViewPort_p);
ST_ErrorCode_t layersub_UpdateContrast(layersub_Device_t* Device_p, layersub_ViewPort_t* ViewPort_p);
ST_ErrorCode_t layersub_SwapActiveSPU(layersub_Device_t* Device_p, layersub_ViewPort_t* ViewPort_p);

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __SUB_SPU_H */

/* End of sub_spu.h */


