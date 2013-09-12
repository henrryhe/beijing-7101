/*******************************************************************************

File name   : hard_mng.h

Description :

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                               Name
----               ------------                               ----
2000-05-30         Created                                    Michel Bruant
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __HW_MANAGER_H
#define __HW_MANAGER_H


/* Includes ----------------------------------------------------------------- */
#include "frontend.h"
#include "stos.h"
/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */
#define STLAYER_MAX_HWTASK_NAME (ST_MAX_DEVICE_NAME + 20)

#define NON_VALID_REG 0xffffffff

/* Exported Types ----------------------------------------------------------- */
typedef struct
{
  stlayer_Node_t *  NodeToAffect_p;   /* address (register or memory) */
  stlayer_Node_t    NodeValue;        /* values to be written         */
} stlayer_HWManagerMsg_t;


/* Exported Functions ------------------------------------------------------- */

/* HW manager initialization function */
ST_ErrorCode_t stlayer_HardwareManagerInit(
  stlayer_XLayerContext_t*  LayerContext_p
);

/* HW manager termination function */
ST_ErrorCode_t stlayer_HardwareManagerTerm(
  stlayer_XLayerContext_t*  LayerContext_p
);

ST_ErrorCode_t stlayer_VTGSynchro(stlayer_XLayerContext_t * LayerContext_p,
                        ST_DeviceName_t NewVTG);
/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __HW_MANAGER_H */

/* End of hard_mng.h */
