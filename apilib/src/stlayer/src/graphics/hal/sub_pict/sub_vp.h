/*******************************************************************************

File name   : Sub_vp.h

Description : Sub Picture viewport header file

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
15 Dec 2000        Created                                           TM
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __SUB_VP_H
#define __SUB_VP_H


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#include "stavmem.h"
#include "stgxobj.h"
#include "sub_com.h"
#include "sub_init.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */
#define LAYERSUB_VALID_VIEWPORT   0x092E92E0   /* 0xXYnZYnZX with the driver ID XYZ in hexa
                                            * STLAYER Id = 158 => 0x09E
                                            * n = 2 Viewport handle sub picture TBD!!!!!!*/


/* Exported Types ----------------------------------------------------------- */

typedef struct
{

    STGXOBJ_Rectangle_t             InputRectangle;
    STGXOBJ_Rectangle_t             OutputRectangle;
    STGXOBJ_Bitmap_t                Bitmap;
    STLAYER_GlobalAlpha_t           GlobalAlpha;
    U8                              Alpha[4];
    STGXOBJ_Palette_t               Palette;
    STAVMEM_BlockHandle_t           SubPictureUnitMemoryHandle;
    void*                           Header_p;
    void*                           RLETop_p;         /* Input data first line  address */
    void*                           RLEBottom_p;      /* Input data second line address */
    void*                           DCSQ_p;
    STLAYER_Handle_t                Handle;           /* Handle of the open connection of the layer */
    U32                             ViewPortValidity; /* Only the value LAYERSUB_VALID_VIEWPORT means the it is valid */
    BOOL                            Enable;           /* If Viewport is enabled */
    STLAYER_ViewPortHandle_t        NextViewPort;     /* Handle of the next open viewport. LAYERSUB_NO_VIEWPORT_HANDLE if none*/
    STLAYER_ViewPortHandle_t        PreviousViewPort; /* Handle of the previous open viewport. LAYERSUB_NO_VIEWPORT_HANDLE if none*/
} layersub_ViewPort_t;


/* Exported Variables ------------------------------------------------------- */


/* Exported Macros ---------------------------------------------------------- */


/* Exported Functions ------------------------------------------------------- */
ST_ErrorCode_t layersub_CloseAllViewPortFromList(layersub_Unit_t* Unit_p,STLAYER_ViewPortHandle_t VPHandle);

int MXYCbCr_Pattern (int mix, int red, int green, int blue);


/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __SUB_VP_H */

