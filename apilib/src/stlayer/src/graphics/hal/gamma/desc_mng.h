/*******************************************************************************

File name   : desc_mng.h

Description : 

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                               Name
----               ------------                               ----
2000-05-30         Created                                    Michel Bruant
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __DESCMANAGER_H
#define __DESCMANAGER_H


/* Includes ----------------------------------------------------------------- */
#include "back_end.h"


/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */

enum
{
    DESC_MANAGER_OK,
    DESC_MANAGER_BAD_VIEWPORT,
    DESC_MANAGER_IMPOSSIBLE_INSERTION,
    DESC_MANAGER_IMPOSSIBLE_RESIZE_MOVE,
    DESC_MANAGER_MOVE_INVERSION_LIST
};

/* Exported Functions ------------------------------------------------------- */
ST_ErrorCode_t stlayer_InsertVPDescriptor(
  stlayer_ViewPortDescriptor_t* NewVPDesc
);

ST_ErrorCode_t stlayer_ExtractVPDescriptor(
  stlayer_ViewPortDescriptor_t* OldVPDesc
);

ST_ErrorCode_t stlayer_CheckVPDescriptor(
  stlayer_ViewPortDescriptor_t* NewVP
);

ST_ErrorCode_t stlayer_TestMove(
            STLAYER_ViewPortHandle_t VPHandle,
           STGXOBJ_Rectangle_t * OutputRectangle
);


ST_ErrorCode_t stlayer_GetViewPortHandle(
  stlayer_ViewPortHandle_t*   PtViewPortHandle,
  STLAYER_Handle_t            LayerHandle
);

ST_ErrorCode_t stlayer_FreeViewPortHandle(
  stlayer_ViewPortHandle_t    ViewPortHandle,
  STLAYER_Handle_t            LayerHandle
);

ST_ErrorCode_t stlayer_FillViewPort(
  STLAYER_Handle_t            LayerHandle,
  STLAYER_ViewPortParams_t*   Params,
  stlayer_ViewPortHandle_t    VPHandleCasted
);

    
/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __DESC_MANAGER_H */

/* End of desc_mng.h */
