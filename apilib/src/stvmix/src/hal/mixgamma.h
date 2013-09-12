/*******************************************************************************

File name   : mixgamma.h

Description : Gamma video mixer driver module header file

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
08 Nov 2000         Created                                          BS
...
15 Nov 2001         Add TestVync for test Vsync generation to avoid 
                    blocking of the task used by mixer               BS
19 Mar  2002        Compilation chip selection exported in makefile  BS
******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __MIX_GAMMA_H
#define __MIX_GAMMA_H

/* Includes ----------------------------------------------------------------- */

#include "vmix_drv.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */
#define STVMIX_GAMMA_HORIZONTAL_OFFSET 40

/* Exported Types ----------------------------------------------------------- */
    
/* Exported Variables ------------------------------------------------------- */

/* Exported Macros ---------------------------------------------------------- */

/* Exported Functions ------------------------------------------------------- */

void stvmix_GammaConnectBackground(stvmix_Device_t * const Device_p);
ST_ErrorCode_t stvmix_GammaConnectLayerMix(stvmix_Device_t * const Device_p, 
                                           stvmix_HalLayerConnect Purpose);
void stvmix_GammaResetConfig(stvmix_Device_t * const Device_p);
ST_ErrorCode_t stvmix_GammaEvt(stvmix_Device_t * const Device_p, 
                               stvmix_HalActionEvt_e Purpose,
                               ST_DeviceName_t VTGName);
ST_ErrorCode_t stvmix_GammaInit(stvmix_Device_t * const Device_p);
ST_ErrorCode_t stvmix_GammaSetBackgroundParameters(stvmix_Device_t * const Device_p,
                                                   stvmix_HalLayerConnect Purpose);
ST_ErrorCode_t stvmix_GammaSetOutputPath(stvmix_Device_t * const Device_p);
ST_ErrorCode_t stvmix_GammaSetScreen(stvmix_Device_t * const Device_p);
void stvmix_GammaTerm(stvmix_Device_t * const Device_p);
/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __MIX_GAMMA_H */

/* End of mixgamma.h */







