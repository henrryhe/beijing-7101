/*******************************************************************************

File name   :

Description : definitions used in layer context.

COPYRIGHT (C) STMicroelectronics 2004

Date               Modification                               Name
----               ------------                               ----
2000-05-30         Created                                    Michel Bruant
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __STILL_CM_H
#define __STILL_CM_H

/* Includes ----------------------------------------------------------------- */

#include "stavmem.h"
#include "stddefs.h"
#include "still_vp.h"

/* Macros ------------------------------------------------------------------- */

/* Variables ---------------------------------------------------------------- */

#ifdef VARIABLE_STILL_RESERVATION
#define VARIABLE_STILL           /* real reservation   */
#else
#define VARIABLE_STILL extern    /* extern reservation */
#endif

VARIABLE_STILL struct
{
    STAVMEM_PartitionHandle_t   AVMEMPartition;
    ST_Partition_t *            CPUPartition_p;
    U32                         MaxViewPorts;
    U32                         MaxHandles;
    STAVMEM_SharedMemoryVirtualMapping_t VirtualMapping;
    void *                      BaseAddress;
    still_ViewportDesc          ViewPort;
    STLAYER_LayerParams_t       LayerParams;
    semaphore_t *               AccessProtect_p;
    /* Output Params */
    S8                          XOffset;
    S8                          YOffset;
    U32                         XStart;
    U32                         YStart;
    U32                         FrameRate;
    BOOL                        OutputEnable;
    U32                         Width;
    U32                         Height;
}stlayer_still_context;

/* Functions ---------------------------------------------------------------- */

U32 stlayer_LayOpen(U32 Device);

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif


/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef  __STILL_CM_H */

/* End of .h */
