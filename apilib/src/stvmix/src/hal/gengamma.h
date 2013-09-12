/*******************************************************************************

File name   : gengamma.h

Description : Generic gamma video mixer driver module header file

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
27 March 2001      Created                                          BS
******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __MIX_GENGAMMA_H
#define __MIX_GENGAMMA_H

/* Includes ----------------------------------------------------------------- */

#include "vmix_drv.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */

/* Exported Types ----------------------------------------------------------- */
    
/* Exported Variables ------------------------------------------------------- */

/* Exported Macros ---------------------------------------------------------- */

/* Exported Functions ------------------------------------------------------- */

void stvmix_GenGammaConnectBackground(stvmix_Device_t * const Device_p);
ST_ErrorCode_t stvmix_GenGammaConnectLayerMix(stvmix_Device_t * const Device_p, 
                                              stvmix_HalLayerConnect Purpose);
ST_ErrorCode_t stvmix_GenGammaEvt(stvmix_Device_t * const Device_p, 
                                  stvmix_HalActionEvt_e Action,
                                  ST_DeviceName_t VTGName);
ST_ErrorCode_t stvmix_GenGammaInit(stvmix_Device_t * const Device_p);
void           stvmix_GenGammaResetConfig(stvmix_Device_t * const Device_p);
ST_ErrorCode_t stvmix_GenGammaSetBackgroundParameters(stvmix_Device_t * const Device_p,
                                                      stvmix_HalLayerConnect Purpose);
ST_ErrorCode_t stvmix_GenGammaSetOutputPath(stvmix_Device_t * const Device_p);
ST_ErrorCode_t stvmix_GenGammaSetScreen(stvmix_Device_t * const Device_p);
void stvmix_GenGammaTerm(stvmix_Device_t * const Device_p);

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __MIX_GENGAMMA_H */

/* End of gengamma.h */







