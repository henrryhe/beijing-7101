/*******************************************************************************

File name   : osd_util.c

Description : 

COPYRIGHT (C) STMicroelectronics 1999.

Date               Modification                                 Name
----               ------------                                 ----
2001-12-01          Creation                                    Michel Bruant

*******************************************************************************/

/* Includes ----------------------------------------------------------------- */


#include "stlayer.h"
#include "stddefs.h"
#include "layerosd.h"
#include "osd_vp.h"
#include "osd_task.h"

/* Functions ---------------------------------------------------------------- */

ST_ErrorCode_t LAYEROSD_GetBitmapAllocParams(STLAYER_Handle_t      LayerHandle,
                                            STGXOBJ_Bitmap_t*     Bitmap_p,
                                    STGXOBJ_BitmapAllocParams_t*  Params1_p,
                                    STGXOBJ_BitmapAllocParams_t*  Params2_p)
{
    /*  LayerHandle and Bitmap_p can be tested but dummies */
    Params1_p->AllocBlockParams.Alignment = 8;
    Params1_p->Offset = 0; /* because we don't write headers in bitmap data */
    switch(Bitmap_p->ColorType)
    {
        case STGXOBJ_COLOR_TYPE_CLUT8:
            Params1_p->Pitch = Bitmap_p->Width;
            break;
        case STGXOBJ_COLOR_TYPE_CLUT4:
            Params1_p->Pitch = Bitmap_p->Width / 2;
            break;
        case STGXOBJ_COLOR_TYPE_CLUT2:
            Params1_p->Pitch = Bitmap_p->Width / 4;
            break;
#ifdef HW_5517
        case STGXOBJ_COLOR_TYPE_ARGB1555:
        case STGXOBJ_COLOR_TYPE_ARGB4444:
        case STGXOBJ_COLOR_TYPE_RGB565:
            Params1_p->Pitch = Bitmap_p->Width * 2;
            break;
#endif
        default:
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
            break;
    }
    if (Params1_p->Pitch % 8 != 0)
    {
        Params1_p->Pitch = Params1_p->Pitch + 8 - (Params1_p->Pitch % 8);
    }
    Params1_p->AllocBlockParams.Size = Params1_p->Pitch * Bitmap_p->Height;

    if (Bitmap_p->BitmapType == STGXOBJ_BITMAP_TYPE_RASTER_PROGRESSIVE)
    {
        Params2_p->AllocBlockParams.Alignment = 0;
        Params2_p->AllocBlockParams.Size      = 0;
        Params2_p->Pitch                      = 0;
        Params2_p->Offset                     = 0;
    }
    else
    {
        return (ST_ERROR_FEATURE_NOT_SUPPORTED);
    }
    return(ST_NO_ERROR);
}

/******************************************************************************/

ST_ErrorCode_t LAYEROSD_GetBitmapHeaderSize(STLAYER_Handle_t  LayerHandle,
                                           STGXOBJ_Bitmap_t* Bitmap_p,
                                           U32 *             HeaderSize_p)
{
    /*  LayerHandle and Bitmap_p can be tested but dummies */
    * HeaderSize_p = 0;
    return(ST_NO_ERROR);
} 
/******************************************************************************/


ST_ErrorCode_t LAYEROSD_GetInitAllocParams(STLAYER_Layer_t LayerType,
                                          U32             ViewPortsNumber,
                                          STLAYER_AllocParams_t * Params)
{
    U32 HeaderSize;

    /* LayerType is dummy (can be tested) */

    HeaderSize =  sizeof(STGXOBJ_ColorUnsignedAYCbCr_t) * 256;
    HeaderSize += sizeof(osd_RegionHeader_t);
    if(HeaderSize % 256 != 0)
    {
        HeaderSize = HeaderSize + 256 - (HeaderSize % 256);
    }
    /* nodes allocation */
    Params->ViewPortNodesInSharedMemory   = TRUE;
    Params->ViewPortNodesBufferAlignment  = 256;
    Params->ViewPortNodesBufferSize       = HeaderSize * 2 * ViewPortsNumber
                    + sizeof(osd_RegionHeader_t); /* for ghost header */
    /* Viewport allocation */ 
    Params->ViewPortDescriptorsBufferSize = ViewPortsNumber *
                                     (sizeof(stosd_ViewportDesc));
    return(ST_NO_ERROR);
}


/******************************************************************************/
ST_ErrorCode_t LAYEROSD_GetPaletteAllocParams(STLAYER_Handle_t LayerHandle,
                                        STGXOBJ_Palette_t*     Palette_p,
                                        STGXOBJ_PaletteAllocParams_t* Params_p)
{
    /* the color format of a palette can be only one */
    if(Palette_p->ColorType != STGXOBJ_COLOR_TYPE_UNSIGNED_AYCBCR6888_444)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
    Params_p->AllocBlockParams.Alignment = 4; 
    Params_p->AllocBlockParams.Size =  sizeof(STGXOBJ_ColorUnsignedAYCbCr_t)
                            * (1 << Palette_p->ColorDepth);
    return(ST_NO_ERROR);
}

/******************************************************************************/

