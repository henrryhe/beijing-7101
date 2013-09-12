/*******************************************************************************

File name   : osd_util.c

Description : 

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                 Name
----               ------------                                 ----
2001-12-01          Creation                                    Michel Bruant

*******************************************************************************/

/* Includes ----------------------------------------------------------------- */


#include "stlayer.h"
#include "stddefs.h"
#include "layerosd.h"
#include "osd_vp.h"

/* Functions ---------------------------------------------------------------- */
ST_ErrorCode_t LAYEROSD_GetBitmapAllocParams(STLAYER_Handle_t      LayerHandle,
                                            STGXOBJ_Bitmap_t*     Bitmap_p,
                                    STGXOBJ_BitmapAllocParams_t*  Params1_p,
                                    STGXOBJ_BitmapAllocParams_t*  Params2_p)
{
    /*  LayerHandle and Bitmap_p can be tested but dummies */

    Params2_p->AllocBlockParams.Alignment = 0;
    Params2_p->AllocBlockParams.Size      = 0;
    Params2_p->Pitch                      = 0;
    Params2_p->Offset                     = 0;

    Params1_p->AllocBlockParams.Alignment = 128;
    Params1_p->Pitch = Bitmap_p->Width + sizeof(osd_RegionHeader_t);
    if (Params1_p->Pitch % 128 != 0)
    {
        Params1_p->Pitch = Params1_p->Pitch + 128 - (Params1_p->Pitch % 128);
    }
    Params1_p->AllocBlockParams.Size = Params1_p->Pitch * Bitmap_p->Height;
    Params1_p->Offset = sizeof(osd_RegionHeader_t);
    return(ST_NO_ERROR);
}

/******************************************************************************/

ST_ErrorCode_t LAYEROSD_GetBitmapHeaderSize(STLAYER_Handle_t  LayerHandle,
                                           STGXOBJ_Bitmap_t* Bitmap_p,
                                           U32 *             HeaderSize_p)
{
    /*  LayerHandle and Bitmap_p can be tested but dummies */
    * HeaderSize_p = sizeof(osd_RegionHeader_t);
    return(ST_NO_ERROR);
} 
/******************************************************************************/


ST_ErrorCode_t LAYEROSD_GetInitAllocParams(STLAYER_Layer_t LayerType,
                                          U32             ViewPortsNumber,
                                          STLAYER_AllocParams_t * Params)
{
    /* LayerType is dummy (can be tested) */
    /* No nodes to allocate : headers take place in user bitmap */
    Params->ViewPortNodesInSharedMemory   = FALSE;
    Params->ViewPortNodesBufferAlignment  = 0;
    Params->ViewPortNodesBufferSize       = 0; 
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


