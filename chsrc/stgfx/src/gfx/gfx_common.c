/******************************************************************************

    File Name   : gfx_common.c

    Description : Common API and internal utility functions

******************************************************************************/

/* Includes ---------------------------------------------------------------- */

#include "gfx_tools.h" /* includes stgfx_init.h, ... */

#ifdef STGFX_TIME_OPS
#include <stdio.h> /* printf */
#endif

/* Private Types ----------------------------------------------------------- */

/* Private Constants ------------------------------------------------------- */

/* Private Variables ------------------------------------------------------- */

/* Private Macros ---------------------------------------------------------- */

/* Private Function Prototypes --------------------------------------------- */

/* Functions --------------------------------------------------------------- */


/*******************************************************************************
Name        : STGFX_Clear()
Description : Clear a bitmap, filling it with a given color
Parameters  : Handle
              Bitmap_p,
              Color_p
Assumptions : The target bitmap, the GC and the parameters are correct
Limitations : 
Returns     : error if a parameter is not correct
*******************************************************************************/
ST_ErrorCode_t STGFX_Clear(
    STGFX_Handle_t    Handle,
    STGXOBJ_Bitmap_t* Bitmap_p,
    STGXOBJ_Color_t*  Color_p
)
{
  ST_ErrorCode_t       Err;
  STGXOBJ_Rectangle_t  Rectangle;
  STBLIT_BlitContext_t BlitContext;
  stgfx_Unit_t *       Unit_p;
    
  /* check handle validity */
  Unit_p = stgfx_HandleToUnit(Handle);
  if(NULL == Unit_p)
  {
    return(ST_ERROR_INVALID_HANDLE);
  }
  if(Bitmap_p == NULL)
  {
    return(ST_ERROR_BAD_PARAMETER);
  }
    
  Rectangle.PositionX = 0;
  Rectangle.PositionY = 0;
  Rectangle.Width     = Bitmap_p->Width;
  Rectangle.Height    = Bitmap_p->Height;

  BlitContext.ColorKeyCopyMode         = STBLIT_COLOR_KEY_MODE_NONE;
  /* BlitContext.ColorKey */
  BlitContext.AluMode                  = STBLIT_ALU_COPY;
  BlitContext.EnableMaskWord           = FALSE;
  /* BlitContext.MaskWord */
  BlitContext.EnableMaskBitmap         = FALSE;
  /* BlitContext.MaskBitmap_p */
  /* BlitContext.MaskRectangle */
  /* BlitContext.WorkBuffer_p */
  BlitContext.EnableColorCorrection    = FALSE;
  /* BlitContext.ColorCorrectionTable_p */
  BlitContext.Trigger.EnableTrigger    = FALSE;
  BlitContext.GlobalAlpha              = 255;
  BlitContext.EnableClipRectangle      = FALSE;
  /* BlitContext.ClipRectangle */
  /* BlitContext.WriteInsideClipRectangle */
  BlitContext.EnableFlickerFilter      = FALSE;
  BlitContext.JobHandle                = STBLIT_NO_JOB_HANDLE;
  BlitContext.UserTag_p                = NULL;
  BlitContext.NotifyBlitCompletion     = FALSE;
  /* BlitContext.EventSubscriberID */
    
  Err = STBLIT_FillRectangle(Unit_p->Device_p->BlitHandle, Bitmap_p,
                             &Rectangle, Color_p, &BlitContext); 
  if(Err != ST_NO_ERROR)
  {
    return(STGFX_ERROR_STBLIT_BLIT);
  }  

  return(ST_NO_ERROR);
} /* End of STGFX_Clear() */


/*******************************************************************************
Name        : STGFX_ReadPixel()
Description : gets the value of a pixel of given coordinates
Parameters  : 
Assumptions : The target bitmap, the GC and the parameters are correct
Limitations : 
Returns     : error if a parameter is not correct
*******************************************************************************/
ST_ErrorCode_t STGFX_ReadPixel(
  STGFX_Handle_t            Handle,
  STGXOBJ_Bitmap_t*         Bitmap_p,
  S32                       X,
  S32                       Y,
  STGXOBJ_Color_t*          PixelValue_p
)
{
  ST_ErrorCode_t    Err;
  stgfx_Unit_t *    Unit_p;
    
  /* check handle validity */
  Unit_p = stgfx_HandleToUnit(Handle);
  if(NULL == Unit_p)
  {
    return(ST_ERROR_INVALID_HANDLE);
  }
  /* rely on STBLIT checking ((Bitmap_p != NULL) && (PixelValue_p != NULL)),
    for speed */

  Err = STBLIT_GetPixel(Unit_p->Device_p->BlitHandle,
                        Bitmap_p, X, Y,PixelValue_p);
  if((Err != ST_NO_ERROR) && (Err != ST_ERROR_BAD_PARAMETER))
  {
    Err = STGFX_ERROR_STBLIT_BLIT;
  }
  
  return Err;
}



/******************************************************************************
Function Name : stgfx_InitBlitContext
  Description : Provide standard initialisation of a BlitContext, and checking
                of GC & Bitmap pointers
   Parameters : GFX handle, BlitContext to fill, GC & Bitmap to copy from
  Assumptions : Handle & BlitContext_p valid
      Returns : ST_ERROR_BAD_PARAMETER / STGFX_ERROR_INVALID_GC
******************************************************************************/
ST_ErrorCode_t stgfx_InitBlitContext(STGFX_Handle_t Handle,
                                     STBLIT_BlitContext_t * BlitContext_p,
                                     const STGFX_GC_t * GC_p,
                                     const STGXOBJ_Bitmap_t * Bitmap_p)
{
    if ((Bitmap_p == NULL) || (GC_p == NULL))
    {
        return ST_ERROR_BAD_PARAMETER;
    }

    BlitContext_p->ColorKeyCopyMode          = GC_p->ColorKeyCopyMode;
    BlitContext_p->ColorKey                  = GC_p->ColorKey;
    BlitContext_p->AluMode                   = GC_p->AluMode;
    BlitContext_p->EnableMaskWord            = FALSE;
    /* BlitContext_p->MaskWord */
    BlitContext_p->EnableMaskBitmap          = FALSE;
    /* BlitContext_p->MaskBitmap_p */
    /* BlitContext_p->MaskRectangle */
    /* BlitContext_p->WorkBuffer_p = ((stgfx_Unit_t*)Handle)->BdfFgndWB_p; */
    BlitContext_p->EnableColorCorrection     = FALSE;
    /* BlitContext_p->ColorCorrectionTable_p */
    BlitContext_p->Trigger.EnableTrigger     = FALSE;
    BlitContext_p->GlobalAlpha               = GC_p->GlobalAlpha;
        /* previously 255, ignored except for STBLIT_ALU_ALPHA_BLEND */
    
    BlitContext_p->EnableClipRectangle = TRUE;
    if (GC_p->ClipMode == STGFX_NO_CLIPPING)
    {
        /* clipping still needs to be turned on to avoid problems if we try
          to raster outside the bitmap (eg end of Robustness test) */
          
        BlitContext_p->ClipRectangle.PositionX = 0;
        BlitContext_p->ClipRectangle.PositionY = 0;
        BlitContext_p->ClipRectangle.Width = Bitmap_p->Width;
        BlitContext_p->ClipRectangle.Height = Bitmap_p->Height;
        BlitContext_p->WriteInsideClipRectangle = TRUE;
    }
    else
    {
        BlitContext_p->ClipRectangle       = GC_p->ClipRectangle;
        
        /* restrict internal clip rect to be no larger than bitmap?
          What about external case? */
        
        if(GC_p->ClipMode == STGFX_EXTERNAL_CLIPPING)
        {
            BlitContext_p->WriteInsideClipRectangle = FALSE;
        }
        else if(GC_p->ClipMode == STGFX_INTERNAL_CLIPPING)
        {
            BlitContext_p->WriteInsideClipRectangle = TRUE;
        }
        else
        {
            return STGFX_ERROR_INVALID_GC;
        }
    }
    
    BlitContext_p->EnableFlickerFilter       = TRUE/*FALSE 打开防闪烁功能 sqzow20100709*/;
    BlitContext_p->JobHandle                 = STBLIT_NO_JOB_HANDLE;
    BlitContext_p->UserTag_p                 = NULL; /* set later to mark avmem to free */
    BlitContext_p->NotifyBlitCompletion      = TRUE;
    BlitContext_p->EventSubscriberID         = stgfx_GetEventSubscriberID(Handle);

    return ST_NO_ERROR;
}

/******************************************************************************
Function Name : DumpXYL
  Description : Debug function to dump an XYL array
   Parameters : array, number of elements, prefix (without trailing \n)
      Returns : 
******************************************************************************/
void DumpXYL(STBLIT_XYL_t* XYL_p, U32 NumXYL, char* Prefix_p)
{
#ifdef STTBX_PRINT
    /* Debug output an XYL array */
    int i;
    
    STTBX_Print(("%s", Prefix_p));
    for(i = 0; i < NumXYL; ++i)
    {
        if(i % 6 == 0) STTBX_Print(("\n"));
        STTBX_Print((" (%u, %u, %u)",
            XYL_p[i].PositionX, XYL_p[i].PositionY, XYL_p[i].Length));
    }
    STTBX_Print(("\n"));
#endif
}

#ifdef STGFX_TIME_OPS
int stgfx_NextTimeSlot = STGFX_STOP_TIMING;
stgfx_Timing_t stgfx_Timings[STGFX_MAX_TIMINGS];

/* begin recording at internal time points */
void STGFX_StartTiming(void)
{
    START_TIMING();
}

/* dump currently stored timings, and either stop recording or start again */
void STGFX_DumpTimings(BOOL Restart)
{
    int i, NumTimings = GET_NUM_TIMINGS();
    clock_t dt, dt2, time_prev = stgfx_Timings[0].Time;
    
    for(i=0; i < NumTimings; ++i)
    {
        dt = time_minus(stgfx_Timings[i].Time, stgfx_Timings[0].Time);
        dt2 = time_minus(stgfx_Timings[i].Time, time_prev);
    
        /* use printf to always output regardless of STTBX_PRINT definition */
        printf("[%2i] %5u ticks (+%5u): %s\n", i, dt, dt2,
               stgfx_Timings[i].Descr_p);
        
        time_prev = stgfx_Timings[i].Time;
    }

    CLEAR_TIMINGS()
    if(Restart) START_TIMING() else STOP_TIMING()
}
#endif


#ifdef STGFX_INCLUDE_TEXT
/******************************************************************************
Function Name : stgfx_AlphaExtendColor
  Description : Translate the given colour to one with an alpha component.
                For all destination types, you can read/write the alpha
                component as the first 8 bytes of the colour value
                Currently only needed for olf, but could have wider application
   Parameters : source colour, and slot for destination one.
                Safe for Dst_p == Src_p, but that's very dependent on the
                exact struct layouts
      Returns : ST_NO_ERROR / ST_ERROR_BAD_PARAMETER for invalid Src_p->Type
******************************************************************************/
ST_ErrorCode_t stgfx_AlphaExtendColor(const STGXOBJ_Color_t* Src_p,
                                      STGXOBJ_Color_t* Dst_p)
{
    switch(Src_p->Type)
    {
    /* these already have an alpha component: */
    /* (alternative, treat invalid types the same and group under default) */
    case STGXOBJ_COLOR_TYPE_ARGB8888:
    case STGXOBJ_COLOR_TYPE_ARGB8565:
    case STGXOBJ_COLOR_TYPE_ARGB1555:
    case STGXOBJ_COLOR_TYPE_ARGB4444:
    
    case STGXOBJ_COLOR_TYPE_ACLUT88:
    case STGXOBJ_COLOR_TYPE_ACLUT44:
    
    case STGXOBJ_COLOR_TYPE_UNSIGNED_AYCBCR6888_444:
    
    case STGXOBJ_COLOR_TYPE_ALPHA1:
    case STGXOBJ_COLOR_TYPE_ALPHA4:
    case STGXOBJ_COLOR_TYPE_ALPHA8:
    case STGXOBJ_COLOR_TYPE_BYTE: /* treat as ALPHA8 and scale */
        *Dst_p = *Src_p;
        break;

    /* for the rest, copy the colour value and specify a full strength
      alpha (alternative, take scaling factor as a parameter) */
      
    case STGXOBJ_COLOR_TYPE_RGB888:
        Dst_p->Type = STGXOBJ_COLOR_TYPE_ARGB8888;
        Dst_p->Value.ARGB8888.B = Src_p->Value.RGB888.B;
        Dst_p->Value.ARGB8888.G = Src_p->Value.RGB888.G;
        Dst_p->Value.ARGB8888.R = Src_p->Value.RGB888.R;
        Dst_p->Value.ARGB8888.Alpha = 0xff;
        break;
    case STGXOBJ_COLOR_TYPE_RGB565:
        Dst_p->Type = STGXOBJ_COLOR_TYPE_ARGB8565;
        Dst_p->Value.ARGB8565.B = Src_p->Value.ARGB8565.B;
        Dst_p->Value.ARGB8565.G = Src_p->Value.ARGB8565.G;
        Dst_p->Value.ARGB8565.R = Src_p->Value.ARGB8565.R;
        Dst_p->Value.ARGB8565.Alpha = 0xff;
        break;

    case STGXOBJ_COLOR_TYPE_CLUT8:
        Dst_p->Type = STGXOBJ_COLOR_TYPE_ACLUT88;
        Dst_p->Value.ACLUT88.PaletteEntry = Src_p->Value.CLUT8;
        Dst_p->Value.ACLUT88.Alpha = 0xff;
        break;
    case STGXOBJ_COLOR_TYPE_CLUT4:
    case STGXOBJ_COLOR_TYPE_CLUT2:
    case STGXOBJ_COLOR_TYPE_CLUT1:
        /* There isn't an ACLUT type < 44, so for CLUT2/1 expand as if
          indexing a palette larger than we will actually touch. That
          memory may not exist, however */
        Dst_p->Type = STGXOBJ_COLOR_TYPE_ACLUT44;
        Dst_p->Value.ACLUT44.PaletteEntry = Src_p->Value.CLUT4;
        Dst_p->Value.ACLUT44.Alpha = 0x0f;
        break;

    case STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_444:
    case STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_422:
    case STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_420:
        Dst_p->Type = STGXOBJ_COLOR_TYPE_UNSIGNED_AYCBCR6888_444;
        Dst_p->Value.UnsignedAYCbCr6888_444.Cr
         = (U8) (0x80 + (S16)Src_p->Value.SignedYCbCr888_444.Cr);
        Dst_p->Value.UnsignedAYCbCr6888_444.Cb
         = (U8) (0x80 + (S16)Src_p->Value.SignedYCbCr888_444.Cb);
        Dst_p->Value.UnsignedAYCbCr6888_444.Y
         = (U8) (0x80 + (S16)Src_p->Value.SignedYCbCr888_444.Y);
        Dst_p->Value.UnsignedAYCbCr6888_444.Alpha = 0x3f;
        break;
    case STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_444:
    case STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422:
    case STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_420:
        Dst_p->Type = STGXOBJ_COLOR_TYPE_UNSIGNED_AYCBCR6888_444;
        Dst_p->Value.UnsignedAYCbCr6888_444.Cr
         = Src_p->Value.UnsignedYCbCr888_444.Cr;
        Dst_p->Value.UnsignedAYCbCr6888_444.Cb
         = Src_p->Value.UnsignedYCbCr888_444.Cb;
        Dst_p->Value.UnsignedAYCbCr6888_444.Y
         = Src_p->Value.UnsignedYCbCr888_444.Y;
        Dst_p->Value.UnsignedAYCbCr6888_444.Alpha = 0x3f;
        break;

    default: /* not in current stgxobj.h list */
        return (ST_ERROR_BAD_PARAMETER);
    }
    
    return (ST_NO_ERROR);
}
#endif /* STGFX_INCLUDE_TEXT */

#if defined(STGFX_INCLUDE_FLOODFILL) || defined(STGFX_INCLUDE_LINES)
/* this function checks the equality of two color values
  (originally in gfx_seedfill.c, but now used by DrawRectangle too) */
/* Note that the invalid bits are also compared. This matters for
  gfx_seedfill.c, which uses TrimColor to zero those bits */
BOOL stgfx_AreColorsEqual(const STGXOBJ_Color_t* ColA_p,
                          const STGXOBJ_Color_t* ColB_p)
{
  switch(ColA_p->Type)
  {
    case STGXOBJ_COLOR_TYPE_ARGB8888_255:
    case STGXOBJ_COLOR_TYPE_ARGB8565_255:
    case STGXOBJ_COLOR_TYPE_ARGB8888:
    case STGXOBJ_COLOR_TYPE_ARGB8565:
    case STGXOBJ_COLOR_TYPE_ARGB1555:
    case STGXOBJ_COLOR_TYPE_ARGB4444:
      if(ColA_p->Value.ARGB8888.Alpha !=  ColB_p->Value.ARGB8888.Alpha)
        return(FALSE);
      if(ColA_p->Value.ARGB8888.R !=  ColB_p->Value.ARGB8888.R)
        return(FALSE);
      if(ColA_p->Value.ARGB8888.G !=  ColB_p->Value.ARGB8888.G)
        return(FALSE);
      if(ColA_p->Value.ARGB8888.B !=  ColB_p->Value.ARGB8888.B)
        return(FALSE);
      break;
    
    case STGXOBJ_COLOR_TYPE_RGB888:
    case STGXOBJ_COLOR_TYPE_RGB565:
      if(ColA_p->Value.RGB888.R !=  ColB_p->Value.RGB888.R)
        return(FALSE);
      if(ColA_p->Value.RGB888.G !=  ColB_p->Value.RGB888.G)
        return(FALSE);
      if(ColA_p->Value.RGB888.B !=  ColB_p->Value.RGB888.B)
        return(FALSE);
      break;
    
    case STGXOBJ_COLOR_TYPE_ACLUT88_255:
    case STGXOBJ_COLOR_TYPE_ACLUT88: /* alphaclut values */
    case STGXOBJ_COLOR_TYPE_ACLUT44:
      if(ColA_p->Value.ACLUT88.Alpha !=  ColB_p->Value.ACLUT88.Alpha)
        return(FALSE);
      if(ColA_p->Value.ACLUT88.PaletteEntry!=ColB_p->Value.ACLUT88.PaletteEntry)
        return(FALSE);
      break; 
    
    case STGXOBJ_COLOR_TYPE_CLUT8: /* U8 values */ 
    case STGXOBJ_COLOR_TYPE_CLUT4:
    case STGXOBJ_COLOR_TYPE_CLUT2:
    case STGXOBJ_COLOR_TYPE_CLUT1:
    case STGXOBJ_COLOR_TYPE_ALPHA1:
    case STGXOBJ_COLOR_TYPE_ALPHA4: /* JF added */
    case STGXOBJ_COLOR_TYPE_ALPHA8:
    case STGXOBJ_COLOR_TYPE_ALPHA8_255:
    case STGXOBJ_COLOR_TYPE_BYTE:
      if(ColA_p->Value.CLUT8 !=  ColB_p->Value.CLUT8)
        return(FALSE);
      break;
    
    case STGXOBJ_COLOR_TYPE_SIGNED_AYCBCR8888:
    case STGXOBJ_COLOR_TYPE_UNSIGNED_AYCBCR8888:
    case STGXOBJ_COLOR_TYPE_UNSIGNED_AYCBCR6888_444:
      if(ColA_p->Value.UnsignedAYCbCr8888.Alpha !=
         ColB_p->Value.UnsignedAYCbCr8888.Alpha)
        return(FALSE);
      if(ColA_p->Value.UnsignedAYCbCr8888.Y !=
         ColB_p->Value.UnsignedAYCbCr8888.Y)
        return(FALSE);
      if(ColA_p->Value.UnsignedAYCbCr8888.Cb !=
         ColB_p->Value.UnsignedAYCbCr8888.Cb)
        return(FALSE);
      if(ColA_p->Value.UnsignedAYCbCr8888.Cr !=
         ColB_p->Value.UnsignedAYCbCr8888.Cr)
        return(FALSE);
      break; 
      
    /* these first four aren't useful to seedfill,
      but could in principle appear for DrawRectangle */
    case STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_422:
    case STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422:
    case STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_420:
    case STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_420:
    case STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_444:
    case STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_444:
      if(ColA_p->Value.UnsignedYCbCr888_444.Y !=
         ColB_p->Value.UnsignedYCbCr888_444.Y)
        return(FALSE);
      if(ColA_p->Value.UnsignedYCbCr888_444.Cb !=
         ColB_p->Value.UnsignedYCbCr888_444.Cb)
        return(FALSE);
      if(ColA_p->Value.UnsignedYCbCr888_444.Cr !=
         ColB_p->Value.UnsignedYCbCr888_444.Cr)
        return(FALSE);
      break; 
    
    default:
      return(FALSE);
      break;
  }
  return(TRUE);
}
#endif /* FLOODFILL or LINES */
