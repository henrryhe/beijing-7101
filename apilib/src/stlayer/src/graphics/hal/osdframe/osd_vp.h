/*****************************************************************************

File name   : osd_vp.h

Description : 

COPYRIGHT (C) STMicroelectronics 1999.

Date               Modification                                 Name
----               ------------                                 ----
2001-01-04         Created                                    Michel Bruant

*****************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __OSD_VP_H
#define __OSD_VP_H

/* Includes --------------------------------------------------------------- */

#include "stlayer.h"
#include "stgxobj.h"
#include "osd_hal.h"

/* Exported Types --------------------------------------------------------- */
typedef struct stosd_ViewportDesc_s
{
    osd_RegionHeader_t *        Top_header;
    osd_RegionHeader_t *        Bot_header;
    BOOL                        Enabled;       /* flag  enabled  */
    BOOL                        Open;
    BOOL                        Recordable;
    STLAYER_GlobalAlpha_t       Alpha;
    STGXOBJ_Rectangle_t         InputRectangle;
    STGXOBJ_Rectangle_t         OutputRectangle;
    STGXOBJ_Rectangle_t         ClippedIn;
    STGXOBJ_Rectangle_t         ClippedOut;
    BOOL                        TotalClipped;
    STGXOBJ_Bitmap_t            Bitmap;
    STGXOBJ_Palette_t           Palette;
    struct stosd_ViewportDesc_s * Next_p;
    struct stosd_ViewportDesc_s * Prev_p;
    U32                         HardImage1;
    U32                         HardImage2Top;
    U32                         HardImage2Bot;
    U32                         HardImage3Top;
    U32                         HardImage3Bot;
    U32                         HardImage4Top;
    U32                         HardImage4Bot;
} stosd_ViewportDesc;


#endif /* #ifndef __OSD_VP_H */
