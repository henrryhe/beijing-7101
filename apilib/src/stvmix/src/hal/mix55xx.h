/*******************************************************************************

File name   : mix55xx.h

Description : 55XX video mixer driver module header file

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
08 Nov 2000         Created                                          BS
19 Mar  2002        Compilation chip selection exported in makefile  BS
******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __MIX_55xx_H
#define __MIX_55xx_H


/* Includes ----------------------------------------------------------------- */

#include "vmix_drv.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Types ----------------------------------------------------------- */
#if defined(ST_5518) || defined(ST_5508)
#define STVMIX_OMEGA1_HORIZONTAL_OFFSET   10
#define STVMIX_OMEGA1_VERTICAL_OFFSET     3
#else
#define STVMIX_OMEGA1_HORIZONTAL_OFFSET   STVMIX_COMMON_HORIZONTAL_OFFSET
#define STVMIX_OMEGA1_VERTICAL_OFFSET     STVMIX_COMMON_VERTICAL_OFFSET
#endif
/* Exported Variables ------------------------------------------------------- */

/* Exported Macros ---------------------------------------------------------- */

/* Exported Functions ------------------------------------------------------- */
ST_ErrorCode_t  stvmix_Omega1ConnectLayer(stvmix_Device_t * const Device_p, 
                                          stvmix_HalLayerConnect Purpose);
void stvmix_Omega1DefaultConfig(stvmix_Device_t * const Device_p);
ST_ErrorCode_t stvmix_Omega1SetBackgroundParameters(stvmix_Device_t * const Device_p,
                                                    stvmix_HalLayerConnect Purpose);


/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __MIX_55xx_H */

/* End of mixgamma.h */







