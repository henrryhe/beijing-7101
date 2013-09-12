/*******************************************************************************

File name   : stilutil.c
Description : It provides the STILL

COPYRIGHT (C) STMicroelectronics 2004

Date               Modification                                 Name
----               ------------                                 ----
2000-12-20         Created                                    Michel Bruant


*******************************************************************************/

/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#include "lay_stil.h"
#include "still_cm.h"
#include "stillhal.h"

/* Functions ---------------------------------------------------------------- */
ST_ErrorCode_t LAYERSTILL_GetInitAllocParams(STLAYER_Layer_t LayerType,
                                          U32             ViewPortsNumber,
                                          STLAYER_AllocParams_t * params)
{
    params->ViewPortNodesInSharedMemory   = FALSE;
    params->ViewPortNodesBufferAlignment  = 0;
    params->ViewPortDescriptorsBufferSize = 0;
    params->ViewPortNodesBufferSize       = 0;
    return(STLAYER_ERROR_USER_ALLOCATION_NOT_ALLOWED);
}

/******************************************************************************/

ST_ErrorCode_t LAYERSTILL_GetBitmapAllocParams(STLAYER_Handle_t LayerHandle,
                                            STGXOBJ_Bitmap_t*     Bitmap_p,
                                    STGXOBJ_BitmapAllocParams_t*  Params1_p,
                                    STGXOBJ_BitmapAllocParams_t*  Params2_p)
{

    if (Bitmap_p->ColorType != STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422)
    {
        return (ST_ERROR_FEATURE_NOT_SUPPORTED);
    }
    Params1_p->AllocBlockParams.Alignment = 128;
    Params1_p->Pitch = 2 * Bitmap_p->Width ;
    if (Params1_p->Pitch %128 != 0)
    {
        Params1_p->Pitch = Params1_p->Pitch - (Params1_p->Pitch %128) + 128;

    }
    Params1_p->AllocBlockParams.Size = Params1_p->Pitch * Bitmap_p->Height;
    Params1_p->Offset = 0;

    if (Bitmap_p->BitmapType == STGXOBJ_BITMAP_TYPE_RASTER_PROGRESSIVE)
    {
        Params2_p->AllocBlockParams.Alignment = 0;
        Params2_p->AllocBlockParams.Size      = 0;
        Params2_p->Pitch                      = 0;
        Params2_p->Offset                     = 0;
    }
    else if(Bitmap_p->BitmapType == STGXOBJ_BITMAP_TYPE_RASTER_TOP_BOTTOM)
    {
        Params1_p->AllocBlockParams.Size = Params1_p->AllocBlockParams.Size / 2
                                            + 1;
        *Params2_p = *Params1_p;
    }
    else
    {
        return (ST_ERROR_FEATURE_NOT_SUPPORTED);
    }
    return(ST_NO_ERROR);
}

/******************************************************************************/
ST_ErrorCode_t LAYERSTILL_GetBitmapHeaderSize(STLAYER_Handle_t  LayerHandle,
                                           STGXOBJ_Bitmap_t* Bitmap_p,
                                           U32 *             HeaderSize_p)

{
    * HeaderSize_p = 0;
    return(ST_NO_ERROR);
}
/******************************************************************************/
ST_ErrorCode_t LAYERSTILL_UpdateFromMixer(STLAYER_Handle_t         LayerHandle,
                                       STLAYER_OutputParams_t * OutputParams_p)
{
    semaphore_wait(stlayer_still_context.AccessProtect_p);
    /* Disconnect the VTG synchro */
    if ((OutputParams_p->UpdateReason & STLAYER_DISCONNECT_REASON) != 0)
    {
        /* Don't care */
    }
    /* Connect a VTG synchro */
    if ((OutputParams_p->UpdateReason & STLAYER_VTG_REASON) != 0)
    {
        /* Don't care */
    }
    /* Change the pipeline */
    if ((OutputParams_p->UpdateReason & STLAYER_CHANGE_ID_REASON) != 0)
    {
        /* Don't care */
    }
    /* Update screen params */
    if ((OutputParams_p->UpdateReason & STLAYER_SCREEN_PARAMS_REASON) != 0)
    {
        stlayer_still_context.XStart = OutputParams_p->XStart;
        stlayer_still_context.YStart = OutputParams_p->YStart  /* top lines */
                                     + OutputParams_p->YStart; /* bot lines */
        stlayer_still_context.FrameRate = OutputParams_p->FrameRate;
        if (OutputParams_p->Width < stlayer_still_context.LayerParams.Width)
        {
            semaphore_signal(stlayer_still_context.AccessProtect_p);
            layerstill_Trace("LAYER - Still : Output Width is too small. \n");
            layerstill_Defense(STLAYER_ERROR_INSIDE_LAYER);
            return(STLAYER_ERROR_INSIDE_LAYER);
        }
        if (OutputParams_p->Height < stlayer_still_context.LayerParams.Height)
        {
            semaphore_signal(stlayer_still_context.AccessProtect_p);
            layerstill_Trace("LAYER - Still : Output Height is too small. \n");
            layerstill_Defense(STLAYER_ERROR_INSIDE_LAYER);
            return(STLAYER_ERROR_INSIDE_LAYER);
        }
         stlayer_still_context.Width = OutputParams_p->Width;
         stlayer_still_context.Height = OutputParams_p->Height;
    }
    /* Update Offset params */
    if ((OutputParams_p->UpdateReason & STLAYER_OFFSET_REASON) != 0)
    {
       stlayer_still_context.XOffset = OutputParams_p->XOffset;
       stlayer_still_context.YOffset = OutputParams_p->YOffset;
    }
    /* Enable / Disable the display */
    if ((OutputParams_p->UpdateReason & STLAYER_DISPLAY_REASON) != 0)
    {
       if (OutputParams_p->DisplayEnable == TRUE)
       {
           stlayer_still_context.OutputEnable = TRUE;
       }
       else
       {
           stlayer_still_context.OutputEnable = FALSE;
       }
    }

    /* dynamic output params change  */
    stlayer_StillHardUpdate();
    semaphore_signal(stlayer_still_context.AccessProtect_p);

    return(ST_NO_ERROR);
}

/* end of stilutil.c */
