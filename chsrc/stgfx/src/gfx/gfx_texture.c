/******************************************************************************

    File Name   : gfx_texture.c

    Description : STGFX texturing helpers

******************************************************************************/

/*#define STTBX_PRINT*/

/* Includes ---------------------------------------------------------------- */

#if defined(DEBUG) && !defined(NDEBUG)
/* turn off asserts unless DEBUG defined */
#define NDEBUG
#endif

#include <assert.h>
#include <string.h>

#include "sttbx.h"

#include "gfx_tools.h"

/* Private Types ----------------------------------------------------------- */

/* Private Constants ------------------------------------------------------- */

/* Private Variables ------------------------------------------------------- */

/* Private Macros ---------------------------------------------------------- */

/* Private Function Prototypes --------------------------------------------- */

/* Functions --------------------------------------------------------------- */


/******************************************************************************
Function Name : GetWorkBufferColorType
  Description : Copy stblit_GetTmpIndex in determining the colour type needed
                for the intermediate bitmap in a masked copy operation, needed
                to determine how to allocate the memory
   Parameters : maximum tile width and height, color type of the tiles and
                destination, place to put workbuffer address and handle
  Assumptions : valid handle and pointers
******************************************************************************/
static ST_ErrorCode_t GetWorkBufferColorType(STGXOBJ_ColorType_t  SrcColorType,
                                             STGXOBJ_ColorType_t* TmpColorType_p)
{
    switch (SrcColorType)
    {
    case STGXOBJ_COLOR_TYPE_CLUT8:
    case STGXOBJ_COLOR_TYPE_ACLUT88:
    case STGXOBJ_COLOR_TYPE_ACLUT88_255:
        *TmpColorType_p = STGXOBJ_COLOR_TYPE_ACLUT88;
        break;
        
    default: /* all others 32 bpp (even ARGB8565!);
               don't bother to validate the colour type */
        *TmpColorType_p = STGXOBJ_COLOR_TYPE_ARGB8888;
        break;
    }
    
    return ST_NO_ERROR;
}

/******************************************************************************
Function Name : stgfx_PrepareWorkBuffer
  Description : Allocate an STBLIT WorkBuffer suitable for texturing with tiles
                up to the size specified. Returns the handle and physical
                address of the avmem block obtained. Schedule this to be freed
                after all uses
   Parameters : maximum tile width and height, color type of the tiles and
                destination, place to put workbuffer address and handle
  Assumptions : valid handle and pointers
******************************************************************************/
ST_ErrorCode_t stgfx_PrepareWorkBuffer(STGFX_Handle_t Handle,
    U32 MaxWidth, U32 MaxHeight, STGXOBJ_ColorType_t ColorType,
    void** WorkBuffer_p_p, STAVMEM_BlockHandle_t* BlockHandle_p)
{
    ST_ErrorCode_t                Err;
    STBLIT_Handle_t               BlitHandle = stgfx_GetBlitterHandle(Handle);
    STGXOBJ_BitmapAllocParams_t   BitmapParams1, BitmapParams2;
    STGXOBJ_Bitmap_t              WorkBitmap;
    /* we don't actually supply it as a bitmap, but need to use
      STBLIT_GetBitmapAllocParams to get the size */
    
    WorkBitmap.BitmapType   = STGXOBJ_BITMAP_TYPE_RASTER_PROGRESSIVE;
    WorkBitmap.Width        = MaxWidth;
    WorkBitmap.Height       = MaxHeight;

    /* derive actual colour type of intermediate bitmap
      (CLUT formats are alpha-extended) */
      
    Err = GetWorkBufferColorType(ColorType, &WorkBitmap.ColorType);
    if(Err != ST_NO_ERROR) return STGFX_ERROR_STBLIT_DRV;
    
    Err = STBLIT_GetBitmapAllocParams(BlitHandle, &WorkBitmap,
                                      &BitmapParams1, &BitmapParams2);
    if(Err != ST_NO_ERROR) return STGFX_ERROR_STBLIT_DRV;
    
    /*STTBX_Print(("stgfx_PrepareWorkBuffer: MaxWidth %u, MaxHeight %u,"
                 " pitch %u, offset %u, alignment %u, allocation %u bytes\n",
                 MaxWidth, MaxHeight, BitmapParams1.Pitch, BitmapParams1.Offset,
                 BitmapParams1.AllocBlockParams.Alignment,
                 BitmapParams1.AllocBlockParams.Size));*/
                 
    /* BitmapParams2 won't be touched - not used for progressive format */
    assert(BitmapParams1.AllocBlockParams.Alignment <= 4);
    return stgfx_AVMEM_malloc(Handle, BitmapParams1.AllocBlockParams.Size,
                              BlockHandle_p, WorkBuffer_p_p);
}


/*******************************************************************************
Name        : stgfx_PrepareMaskBitmap()
Description : prepares the fields of a mask bitmap given the target bitmap
Parameters  : 
Assumptions : both bitmap pointers are valid,
              the Target bitmnap contains sensible data
              data pointers are initialized to NULL
Limitations :
Returns     :
*******************************************************************************/
void stgfx_PrepareMaskBitmap(
    STGXOBJ_Bitmap_t* TargetBitmap_p,
    STGXOBJ_Bitmap_t* MaskBitmap_p
)
{
#ifdef STGFX_MASK_ALPHA8
    /* needed to avoid hanging on ST40GX1 with STBLIT_DEVICE_TYPE_7015;
      doesn't work with STBLIT_DEVICE_TYPE_ST40GX1 */
      
    MaskBitmap_p->ColorType = STGXOBJ_COLOR_TYPE_ALPHA8;
    MaskBitmap_p->Pitch = TargetBitmap_p->Width;
#else
    /* This is what we really want to do, and works with
      STBLIT_DEVICE_TYPE_ST40GX1 */
      
    MaskBitmap_p->ColorType = STGXOBJ_COLOR_TYPE_ALPHA1;
    /*MaskBitmap_p->Pitch = (TargetBitmap_p->Width/8
                         + (TargetBitmap_p->Width%8 ? 1 : 0));*/
    MaskBitmap_p->Pitch = (TargetBitmap_p->Width+7)/8;
#endif
    MaskBitmap_p->BitmapType =  STGXOBJ_BITMAP_TYPE_RASTER_PROGRESSIVE;
    MaskBitmap_p->PreMultipliedColor =  FALSE;
    MaskBitmap_p->ColorSpaceConversion = STGXOBJ_CONVERSION_MODE_UNKNOWN;
    MaskBitmap_p->AspectRatio = STGXOBJ_ASPECT_RATIO_SQUARE;
    /* should we use the target aspect ratio instead? */
    MaskBitmap_p->Width = TargetBitmap_p->Width;
    MaskBitmap_p->Height = TargetBitmap_p->Height;
    /* MaskBitmap_p->Pitch above */
    MaskBitmap_p->Offset  = 0;
    MaskBitmap_p->Data1_p = NULL; /* set by PrepareTexture before use */
    MaskBitmap_p->Size1   = MaskBitmap_p->Pitch * TargetBitmap_p->Height;
    MaskBitmap_p->Data2_p = NULL; /* irrelevant */
    MaskBitmap_p->Size2   = 0; /* irrelevant */
    MaskBitmap_p->SubByteFormat = STGXOBJ_SUBBYTE_FORMAT_RPIX_LSB; /* MSB */
         /*LSB appears to be what the hardware wants */
    MaskBitmap_p->BigNotLittle = FALSE;
}


/******************************************************************************
Function Name : stgfx_InitTexture
  Description : Perform initial texturing analysis and setup, including
                checking the colour type, preparing the MaskBitmap structure
                and allocating the WorkBuffer
   Parameters : 
  Assumptions : valid handle and pointers; at least one of DrawTexture and
                FillTexture set
******************************************************************************/
ST_ErrorCode_t stgfx_InitTexture(STGFX_Handle_t Handle, const STGFX_GC_t* GC_p,
    STGXOBJ_Bitmap_t* TargetBitmap_p, STGXOBJ_Bitmap_t* MaskBitmap_p,
    void** WorkBuffer_p_p, STAVMEM_BlockHandle_t* WorkBlockHandle_p)
{
    U32 MaxWidth = 0, MaxHeight = 0;

    stgfx_PrepareMaskBitmap(TargetBitmap_p, MaskBitmap_p);

    if(GC_p->DrawTexture_p != NULL)
    {
        if(GC_p->DrawTexture_p->ColorType != TargetBitmap_p->ColorType)
            return ST_ERROR_BAD_PARAMETER;
            
        MaxWidth = GC_p->DrawTexture_p->Width;
        MaxHeight = GC_p->DrawTexture_p->Height;
    }

    if(GC_p->FillTexture_p != NULL)
    {
        if(GC_p->FillTexture_p->ColorType != TargetBitmap_p->ColorType)
            return ST_ERROR_BAD_PARAMETER;

        MaxWidth = GFXMAX(MaxWidth, GC_p->FillTexture_p->Width);
        MaxHeight = GFXMAX(MaxHeight, GC_p->FillTexture_p->Height);
    }

    return stgfx_PrepareWorkBuffer(Handle, MaxWidth, MaxHeight,
                                   TargetBitmap_p->ColorType,
                                   WorkBuffer_p_p, WorkBlockHandle_p);
}


/******************************************************************************
Function Name : stgfx_PrepareTexture
  Description : Set up the data region of the supplied mask bitmap. Returns
                the handle of the avmem block obtained - pass this to
                ApplyTexture for freeing, or free it yourself if an error occurs
                first. Also sets BlitContext ALU/ColorKey (restored by
                stgfx_ApplyTexture) and Color for drawing to the mask
   Parameters : span width and height, blit context and color to modify,
                place to put avmem block handle
  Assumptions : valid handle and pointers; all MaskBitmap members except
                Data1_p already set up (usually by stgfx_PrepareMaskBitmap)
******************************************************************************/
ST_ErrorCode_t stgfx_PrepareTexture(STGFX_Handle_t Handle,
    STGXOBJ_Bitmap_t* MaskBitmap_p, STAVMEM_BlockHandle_t* BlockHandle_p,
    STBLIT_BlitContext_t* BlitContext_p, STGXOBJ_Color_t* Color_p)
{
    ST_ErrorCode_t Err;
    
    Err = stgfx_AVMEM_malloc(Handle, MaskBitmap_p->Size1, BlockHandle_p,
                             (void**) &(MaskBitmap_p->Data1_p));
    if (Err != ST_NO_ERROR) return Err;
    memset(MaskBitmap_p->Data1_p, 0x0, MaskBitmap_p->Size1);

#ifdef STGFX_MASK_ALPHA8
    Color_p->Type         = STGXOBJ_COLOR_TYPE_ALPHA8;
    Color_p->Value.ALPHA8 = 255; /* not 100 */
#else
    Color_p->Type         = STGXOBJ_COLOR_TYPE_ALPHA1;
    Color_p->Value.ALPHA1 = 1;
#endif

    BlitContext_p->AluMode          = STBLIT_ALU_COPY;
    BlitContext_p->ColorKeyCopyMode = STBLIT_COLOR_KEY_MODE_NONE; 

    return ST_NO_ERROR;
}


/******************************************************************************
Function Name : stgfx_ApplyTexture
  Description : Replicate a texture using the supplied mask, BlitContext and
                target origin. We first restore the ALU and ColorKey settings
                from the supplied GC. We will schedule freeing the mask bitmap
                (but not the WorkBuffer)
   Parameters : span width and height, blit context and color to modify,
                place to put avmem block handle
  Assumptions : all the bits valid
******************************************************************************/
ST_ErrorCode_t stgfx_ApplyTexture(STGFX_Handle_t Handle, U32 OriginX,
    U32 OriginY, const STGFX_GC_t* GC_p, STGXOBJ_Bitmap_t* TileBitmap_p,
    STGXOBJ_Bitmap_t* MaskBitmap_p, STAVMEM_BlockHandle_t* MaskBlockHandle_p,
    STGXOBJ_Bitmap_t* TargetBitmap_p, STBLIT_BlitContext_t* BlitContext_p)
{
    BOOL OldNotifyBlitCompletion = BlitContext_p->NotifyBlitCompletion;
    ST_ErrorCode_t         Err;
    STBLIT_Handle_t        BlitHandle = stgfx_GetBlitterHandle(Handle);
    STBLIT_Source_t        Source;
    STBLIT_Destination_t   Destination;
    int                    i;

    /* debugging: see if intermittent halt is BLIT mishandling queue end hit */
    /*STGFX_Sync(Handle); - still occurs */
    
    /* source & dest preparation */
    Source.Type             = STBLIT_SOURCE_TYPE_BITMAP;
    Source.Data.Bitmap_p    = TileBitmap_p;
    Source.Palette_p        = NULL;

    Destination.Bitmap_p    = TargetBitmap_p;
    Destination.Palette_p   = NULL;

    /* BlitContext setup  */
    BlitContext_p->AluMode                 = GC_p->AluMode;
    BlitContext_p->ColorKeyCopyMode        = GC_p->ColorKeyCopyMode;
    BlitContext_p->EnableMaskBitmap        = TRUE;
    BlitContext_p->MaskBitmap_p            = MaskBitmap_p;
    BlitContext_p->NotifyBlitCompletion    = FALSE;
    /*BlitContext_p->WorkBuffer_p            = WorkBuffer_p; - set up by caller */

    STTBX_Print(("stgfx_ApplyTexture "));

#ifdef STGFX_TEXTURE_REPEAT
    /* JF DEBUG: try lots of times with static memory configuration */
    for(i = 0; i < 10; ++i)
#endif
    {
        /* Y geometry for first row */
        Source.Rectangle.PositionY = OriginY % TileBitmap_p->Width;
        Source.Rectangle.Height = TileBitmap_p->Height - Source.Rectangle.PositionY;
        BlitContext_p->MaskRectangle.PositionY = 0;
        Destination.Rectangle.PositionY = OriginY;

        if(i == 0) { STTBX_Print((".")); }

        while(BlitContext_p->MaskRectangle.PositionY < MaskBitmap_p->Height)
        {
            /* limit to the height of the tiled region */
            Source.Rectangle.Height = GFXMIN(Source.Rectangle.Height,
                MaskBitmap_p->Height - BlitContext_p->MaskRectangle.PositionY);

            /* X geometry for first tile in the row */
            Source.Rectangle.PositionX = OriginX % TileBitmap_p->Width;
            Source.Rectangle.Width = TileBitmap_p->Width - Source.Rectangle.PositionX;
            BlitContext_p->MaskRectangle.PositionX = 0;
            Destination.Rectangle.PositionX = OriginX;

            /* propogate Height of this row */
            BlitContext_p->MaskRectangle.Height = Source.Rectangle.Height;
            Destination.Rectangle.Height        = Source.Rectangle.Height;

            while(BlitContext_p->MaskRectangle.PositionX < MaskBitmap_p->Width)
            {
                /* limit to the width of the tiled region */
                Source.Rectangle.Width = GFXMIN(Source.Rectangle.Width,
                    MaskBitmap_p->Width - BlitContext_p->MaskRectangle.PositionX);

                /* propogate Width of this copy */
                BlitContext_p->MaskRectangle.Width = Source.Rectangle.Width;
                Destination.Rectangle.Width        = Source.Rectangle.Width;

                Err = STBLIT_Blit(BlitHandle, &Source, NULL,
                                  &Destination, BlitContext_p);
                if(Err != ST_NO_ERROR)
                {
                    STTBX_Print(("STBLIT_Blit error 0x%08X\n", Err));
                    stgfx_ScheduleAVMEM_free(Handle, MaskBlockHandle_p);

                    /* for completeness, though caller will probably bail out: */
                    BlitContext_p->EnableMaskBitmap        = FALSE;
                    BlitContext_p->NotifyBlitCompletion    = OldNotifyBlitCompletion;

                    return(STGFX_ERROR_STBLIT_BLIT);  
                }
                
    #ifdef STGFX_SYNC_EACH_TILE
                STGFX_Sync(Handle);
    #endif

    #ifdef TRACE_FIRST_TILE
                if((BlitContext_p->MaskRectangle.PositionX == 0) &&
                   (BlitContext_p->MaskRectangle.PositionY == 0))
                {
                    STTBX_Print((" first tile complete ..."));
                }
    #endif
    #ifdef TRACE_FIRST_ROW
                if(BlitContext_p->MaskRectangle.PositionY == 0)
                {
                    STTBX_Print(("."));
                }
    #endif          
                /* restore standard geometry for second and subsequent tiles */
                Source.Rectangle.PositionX   = 0;
                Source.Rectangle.Width       = TileBitmap_p->Width;

                /* advance to next position */
                BlitContext_p->MaskRectangle.PositionX += Destination.Rectangle.Width;
                Destination.Rectangle.PositionX        += Destination.Rectangle.Width;
            }

    #ifdef TRACE_FIRST_ROW
            if(BlitContext_p->MaskRectangle.PositionY == 0)
            {
                STTBX_Print((" first row complete\n"));
            }
    #endif          
            /* restore standard geometry for second and subsequent rows */
            Source.Rectangle.PositionY   = 0;
            Source.Rectangle.Height      = TileBitmap_p->Height;

            /* advance to next position */
            BlitContext_p->MaskRectangle.PositionY += Destination.Rectangle.Height;
            Destination.Rectangle.PositionY        += Destination.Rectangle.Height;
        }
    }

    /*STTBX_Print(("stgfx_ApplyTexture end\n"));*/
    
    /* restore default BlitContext */
    BlitContext_p->EnableMaskBitmap        = FALSE;
    BlitContext_p->NotifyBlitCompletion    = OldNotifyBlitCompletion;
    
    return stgfx_ScheduleAVMEM_free(Handle, MaskBlockHandle_p);
}
