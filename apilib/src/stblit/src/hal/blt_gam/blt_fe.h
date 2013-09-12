/*******************************************************************************

File name   : blt_fe.h

Description : front_end header file

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
20 Jun 2000        Created                                           TM
******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __BLT_FE_H
#define __BLT_FE_H


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#include "blt_init.h"
#include "blt_com.h"
#include "stblit.h"
#include "stgxobj.h"


/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */


/* Exported Types ----------------------------------------------------------- */


/* Exported Variables ------------------------------------------------------- */
extern const U16 stblit_TableROP[28][28];
extern const U16 stblit_TableBlend[28][28];
extern const U16 stblit_TableSingleSrc2[28][28];
extern const U16 stblit_TableMB[28][28];
extern const U16 stblit_TableSingleSrc1[28][28];


/* Exported Macros ---------------------------------------------------------- */
#define stblit_PixelArea(Rectangle_p) ((((STGXOBJ_Rectangle_t*) Rectangle_p)->Width) * (((STGXOBJ_Rectangle_t*) Rectangle_p)->Height))

/* Exported Functions ------------------------------------------------------- */
ST_ErrorCode_t stblit_GetIndex (STGXOBJ_ColorType_t TypeIn, U8* Index, STGXOBJ_ColorSpaceConversionMode_t Mode);
ST_ErrorCode_t stblit_GetTmpIndex (U8 SrcIndex, U8* Index_p, STGXOBJ_ColorType_t* ColorType_p, U8* NbBitPerPixel_p);

ST_ErrorCode_t stblit_OneInputBlit( stblit_Device_t*        Device_p,
                                    STBLIT_Source_t*        Src_p,
                                    STBLIT_Destination_t*   Dst_p,
                                    STBLIT_BlitContext_t*   BlitContext_p);

ST_ErrorCode_t stblit_TwoInputBlit(stblit_Device_t*        Device_p,
                                    STBLIT_Source_t*        Src1_p,
                                    STBLIT_Source_t*        Src2_p,
                                    STBLIT_Destination_t*   Dst_p,
                                    STBLIT_BlitContext_t*   BlitContext_p);


/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __BLT_FE_H */

/* End of blt_fe.h */
