/*******************************************************************************

File name   : blt_sw.h

Description : Software optimisation routines for STBLIT

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
Apr 2002           Created                                           GS
May 2002           Modified                                          TM
******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __BLT_SW_H
#define __BLT_SW_H

/* Includes ----------------------------------------------------------------- */
#include "stddefs.h"
#include "stblit.h"
#include "stgxobj.h"
#include "blt_init.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */

/* Exported Types ----------------------------------------------------------- */

/* Exported Variables ------------------------------------------------------- */

/* Exported Macros ---------------------------------------------------------- */

/* Exported Functions ------------------------------------------------------- */

ST_ErrorCode_t stblit_sw_fillrect(  stblit_Device_t*      Device_p,
                                    STGXOBJ_Bitmap_t*     Bitmap_p,
                                    STGXOBJ_Rectangle_t*  Rectangle_p,
                                    STGXOBJ_Rectangle_t*  MaskRectangle_p,
                                    STGXOBJ_Color_t*      Color_p,
                                    STBLIT_BlitContext_t* BlitContext_p);

ST_ErrorCode_t stblit_sw_xyl(       stblit_Device_t*      Device_p,
                                    STGXOBJ_Bitmap_t*     Bitmap_p,
                                    STBLIT_XYL_t*         XYLArray_p,
                                    U32                   SegmentsNumber,
                                    STGXOBJ_Color_t*      Color_p,
                                    STBLIT_BlitContext_t* BlitContext_p );


ST_ErrorCode_t stblit_sw_copyrect(  stblit_Device_t*      Device_p,
                                    STGXOBJ_Bitmap_t*     SrcBitmap_p,
                                    STGXOBJ_Rectangle_t*  SrcRectangle_p,
                                    STGXOBJ_Bitmap_t*     DstBitmap_p,
                                    S32                   DstPositionX,
                                    S32                   DstPositionY,
                                    STBLIT_BlitContext_t* BlitContext_p );

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __BLT_SW_H */

/* End of blt_sw.h */

