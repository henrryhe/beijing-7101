/*****************************************************************************

File name   : osd_vp.h

Description : 

COPYRIGHT (C) STMicroelectronics 2001.

Date               Modification                                 Name
----               ------------                                 ----
2001-03-19          Creation                                    Michel Bruant

*****************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __OSD_VP_H
#define __OSD_VP_H

#include "stgxobj.h"

/* Exported Types --------------------------------------------------------- */

typedef struct
{
    U32 word1;
    U32 word2;
}osd_RegionHeader_t;


typedef struct stosd_ViewportDesc_s
{
    osd_RegionHeader_t *        PaletteFirstHeader_p;
    osd_RegionHeader_t *        PaletteSecondHeader_p;
    osd_RegionHeader_t *        BitmapFirstHeader_p;
    osd_RegionHeader_t *        BitmapSecondHeader_p;
    osd_RegionHeader_t *        BitmapPrevLastHeader_p;
    osd_RegionHeader_t *        BitmapLastHeader_p;
    STGXOBJ_Bitmap_t            Bitmap;
    STGXOBJ_Palette_t           Palette;
    U32                         BitmapY;   /* input (X=0 anyway) */
    U32                         PositionX; /* output */
    U32                         PositionY; /* output */
    U32                         Width;     /* output-input (no resize) */
    U32                         Height;    /* output-input (no resize) */
    U32                         Pitch;
    U32                         Alpha;
    BOOL                        TopHeaderUpdate;
    BOOL                        BotHeaderUpdate;
    struct stosd_ViewportDesc_s * Next_p; /* NULL=Last */
    struct stosd_ViewportDesc_s * Prev_p; /* NULL=First */
    BOOL                        Enabled;       /* flag  enabled  */
    BOOL                        Open;
    BOOL                        Recordable;
} stosd_ViewportDesc;


#endif /* #ifndef __OSD_VP_H */
