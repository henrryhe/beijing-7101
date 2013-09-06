/******************************************************************************
File name   : stblitwrap_cmd.c

Description : STBLIT wrapper module commands based on the STOSD driver

Assumptions : A valid OSD driver has been initialized and opened.
              Only 8 bpp regions are operated on by this STBLIT-like wrapper
              Here follows the list of the fields of a STGXOBJ_Bitmap_t
              structure with the corrensponding meaningful values for this
              implementation:

  ColorType                 STGXOBJ_COLOR_TYPE_CLUT8
  BitmapType                STGXOBJ_BITMAP_TYPE_RASTER_TOP_BOTTOM
  PreMultipliedColor        --->irrelevant<---
  ColorSpaceConversion      --->irrelevant<---
  AspectRatio               --->irrelevant<---
  Width                     Width of the OSD region 
  Height                    Height of the OSD region
  Pitch                     --->irrelevant<---
  Data1_p                   Handle of the OSD region, casted to (void*)
  Size1                     --->irrelevant<---
  Data2_p                   --->irrelevant<---
  Size2                     --->irrelevant<---
  SubByteFormat             --->irrelevant<---


COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                  Name
----               ------------                                  ----
2000-11-22         Adapted to API 1.1.0                          Adriano Melis
2000-07-06         Created                                       Adriano Melis
*******************************************************************************/

/* Includes ----------------------------------------------------------------- */
#include "stddefs.h"
#include "stosd.h"
#include "stblit.h"
#include "stgxobj.h"
#include <stdio.h>
#include <string.h>



/* Private Types ------------------------------------------------------------ */
/* this enum represents the direction of copying rectangles:
   specially important whren copying is performed between
   overlapped rectangles */
typedef enum
{
  NW, /* NorthWest */
  SE, /* SouthEast */
  SW, /* SouthWest */
  NE  /* NorthEast */
} stblit_CopyDirection;


/* Private Constants -------------------------------------------------------- */

/* Private Variables (static)------------------------------------------------ */
static U8 GliphBuffer[2500];
static STEVT_Handle_t EvtHandle;
static STEVT_EventID_t EvtId;
static ST_DeviceName_t BlitName;
static ST_DeviceName_t EvtName;


/* Global Variables --------------------------------------------------------- */


/* Private Macros ----------------------------------------------------------- */


/* Private Function prototypes ---------------------------------------------- */
static ST_ErrorCode_t stblit_Notify(STBLIT_BlitContext_t *Context);

/* Exported Functions ------------------------------------------------------- */

ST_ErrorCode_t STBLIT_Init(const ST_DeviceName_t DeviceName,
                           const STBLIT_InitParams_t * const InitParams_p)
{

    strcpy ( BlitName, DeviceName);
    strcpy ( EvtName, InitParams_p->EventHandlerName);

    return (ST_NO_ERROR);
}



ST_ErrorCode_t STBLIT_Open(const ST_DeviceName_t      DeviceName,
                           const STBLIT_OpenParams_t* const Params_p,
                           STBLIT_Handle_t*           Handle_p)
{
    STEVT_OpenParams_t OpenParms;
    ST_ErrorCode_t Error;
  
  
    Error = STEVT_Open( EvtName,
                        &OpenParms,
                        &EvtHandle);
    if (Error != ST_NO_ERROR)
    {
        return (Error);
    }


    Error = STEVT_RegisterDeviceEvent( EvtHandle, 
                                       BlitName, 
                                       (STEVT_EventConstant_t)STBLIT_BLIT_COMPLETED_EVT,
                                       &EvtId);
    if (Error != ST_NO_ERROR)
    {
        return (Error);
    }

    *Handle_p = 0;
    
    return(ST_NO_ERROR);
}


ST_ErrorCode_t STBLIT_Close(STBLIT_Handle_t  Handle)
{
  return(ST_NO_ERROR);
}

ST_ErrorCode_t STBLIT_SetPixel(STBLIT_Handle_t       Handle,
                               STGXOBJ_Bitmap_t*        Bitmap_p,
                               S32                   PositionX,
                               S32                   PositionY,
                               STGXOBJ_Color_t*         Color_p,
                               STBLIT_BlitContext_t* Context_p)
{
  STOSD_RegionHandle_t RegionHandle;
  STOSD_Color_t        DstColor;
  STOSD_Color_t        OSDColor;
  ST_ErrorCode_t       Err = ST_NO_ERROR;
  STGXOBJ_Rectangle_t  ClipRect = Context_p->ClipRectangle;

  /* back casting to region handle */
  RegionHandle = (STOSD_RegionHandle_t)(Bitmap_p->Data1_p);
  if(Color_p->Type != STGXOBJ_COLOR_TYPE_CLUT8 &&
     Color_p->Type != STGXOBJ_COLOR_TYPE_CLUT4 &&
     Color_p->Type != STGXOBJ_COLOR_TYPE_CLUT2)
  {
    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
  }
  
  /* ALU modes */
  if(Context_p->AluMode != STBLIT_ALU_COPY &&
     Context_p->AluMode != STBLIT_ALU_XOR)
  {
    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
  }    
  if(Context_p->EnableClipRectangle &&
     Context_p->WriteInsideClipRectangle == FALSE)
  {
    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
  }
  /* ColorKey Type  can be only CLUT8 */
  if(Context_p->ColorKeyCopyMode != STBLIT_COLOR_KEY_MODE_NONE &&
     Context_p->ColorKey.Type != STGXOBJ_COLOR_KEY_TYPE_CLUT8)
  {
    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
  }

  /* set the clipping rectangle according to blit context */
  if(Context_p->EnableClipRectangle)
  {
    if(ClipRect.PositionX<0)
    {
      ClipRect.Width -= ClipRect.PositionX;
      ClipRect.PositionX = 0;
    }
    if(ClipRect.PositionY<0)
    {
      ClipRect.Height -= ClipRect.PositionY;
      ClipRect.PositionY = 0;
    }
    if((ClipRect.PositionX+ClipRect.Width)>Bitmap_p->Width)
    {
      ClipRect.Width = Bitmap_p->Width - ClipRect.PositionX;
    }
    if((ClipRect.PositionY+ClipRect.Height)>Bitmap_p->Height)
    {
      ClipRect.Height = Bitmap_p->Height - ClipRect.PositionY;
    }
    Err = STOSD_SetDstClippingArea(RegionHandle,
                                   Context_p->ClipRectangle.PositionX,
                                   Context_p->ClipRectangle.PositionY,
                                   Context_p->ClipRectangle.Width,
                                   Context_p->ClipRectangle.Height);
    if(Err != ST_NO_ERROR)
    {
      return(Err);
    }
  }
  /* set the color type for drawing */
  OSDColor.Type = STOSD_COLOR_TYPE_PALETTE;
  /* set the color value for drawing */
  switch(Color_p->Type)
  {
    case STGXOBJ_COLOR_TYPE_CLUT8:
      OSDColor.Value.PaletteEntry = Color_p->Value.CLUT8;
      break;
    default:
      return(ST_ERROR_FEATURE_NOT_SUPPORTED);
      break;
  }

  /* if there is no colorkeying go on! */
  if(Context_p->ColorKeyCopyMode == STBLIT_COLOR_KEY_MODE_NONE)
  {
    switch(Context_p->AluMode)
    {
      case STBLIT_ALU_XOR:
        Err = STOSD_GetPixel(RegionHandle, PositionX, PositionY, &DstColor);
        if(Err != ST_NO_ERROR)
        {
          return(Err);
        }
        OSDColor.Value.PaletteEntry = ((*((U8*)(&(DstColor.Value)))) ^ 
                         (*((U8*)(&(OSDColor.Value)))) );
        break;
      default:
        break;
    }
    Err = STOSD_SetPixel(RegionHandle, PositionX, PositionY, OSDColor);
    if(Err != ST_NO_ERROR)
    {
      return(Err);
    }
  }
  else if(Context_p->ColorKeyCopyMode == STBLIT_COLOR_KEY_MODE_SRC &&
          Context_p->ColorKey.Value.CLUT8.PaletteEntryEnable == TRUE)
  {
    /* check if the color to be written is in range */
    if((Context_p->ColorKey.Value.CLUT8.PaletteEntryOut == FALSE &&
        Color_p->Value.CLUT8 >=
        Context_p->ColorKey.Value.CLUT8.PaletteEntryMin          &&
        Color_p->Value.CLUT8 <=
        Context_p->ColorKey.Value.CLUT8.PaletteEntryMax            ) ||
       (Context_p->ColorKey.Value.CLUT8.PaletteEntryOut == TRUE  &&
        (Color_p->Value.CLUT8 <
         Context_p->ColorKey.Value.CLUT8.PaletteEntryMin ||
         Color_p->Value.CLUT8 >
         Context_p->ColorKey.Value.CLUT8.PaletteEntryMax   )           ))
    {
      switch(Context_p->AluMode)
      {
        case STBLIT_ALU_XOR:
          Err = STOSD_GetPixel(RegionHandle, PositionX, PositionY, &DstColor);
          if(Err != ST_NO_ERROR)
          {
            return(Err);
          }
          OSDColor.Value.PaletteEntry = ((*((U8*)(&(DstColor.Value)))) ^ 
                           (*((U8*)(&(OSDColor.Value)))) );
          break;
        default:
          break;
      }
      Err = STOSD_SetPixel(RegionHandle, PositionX, PositionY, OSDColor);
      if(Err != ST_NO_ERROR)
      {
          return(Err);
      }
    }
  }
  else if(Context_p->ColorKeyCopyMode == STBLIT_COLOR_KEY_MODE_DST &&
          Context_p->ColorKey.Value.CLUT8.PaletteEntryEnable == TRUE)
  {
    /* retrieve the value of the pixel */
    Err = STOSD_GetPixel(RegionHandle, PositionX, PositionY, &DstColor);
    /* check if the pixel can be overwritten */
    if((Context_p->ColorKey.Value.CLUT8.PaletteEntryOut == FALSE &&
        DstColor.Value.PaletteEntry >=
        Context_p->ColorKey.Value.CLUT8.PaletteEntryMin          &&
        DstColor.Value.PaletteEntry <=
        Context_p->ColorKey.Value.CLUT8.PaletteEntryMax            ) ||
       (Context_p->ColorKey.Value.CLUT8.PaletteEntryOut == TRUE  &&
        (DstColor.Value.PaletteEntry <
         Context_p->ColorKey.Value.CLUT8.PaletteEntryMin ||
         DstColor.Value.PaletteEntry >
         Context_p->ColorKey.Value.CLUT8.PaletteEntryMax   )           ))
    {
      switch(Context_p->AluMode)
      {
        case STBLIT_ALU_XOR:
          OSDColor.Value.PaletteEntry = ((*((U8*)(&(DstColor.Value)))) ^ 
                                        (*((U8*)(&(OSDColor.Value)))) );
          break;
        default:
          break;
      }
      Err = STOSD_SetPixel(RegionHandle, PositionX, PositionY, OSDColor);
      if(Err != ST_NO_ERROR)
      {
          return(Err);
      }
    }
  }

  Err = stblit_Notify(Context_p);
  if(Err != ST_NO_ERROR)
  {
      return(Err);
  }
  
  return(ST_NO_ERROR);
} /* end of STBLIT_setPixel() */

ST_ErrorCode_t STBLIT_FillRectangle( STBLIT_Handle_t       Handle,
                                     STGXOBJ_Bitmap_t*        Bitmap_p,
                                     STGXOBJ_Rectangle_t*     Rectangle_p,
                                     STGXOBJ_Color_t*         Color_p,
                                     STBLIT_BlitContext_t* Context_p )
{
  STOSD_RegionHandle_t RegionHandle;
  STOSD_Color_t        OSDColor;
  STOSD_Color_t        OSDColorTmp;
  STOSD_Color_t        DstColor;
  ST_ErrorCode_t       Err = ST_NO_ERROR;
  STGXOBJ_Rectangle_t  ClipRect = Context_p->ClipRectangle;

  /* back casting to region handle */
  RegionHandle = (STOSD_RegionHandle_t)(Bitmap_p->Data1_p);
  if(Color_p->Type != STGXOBJ_COLOR_TYPE_CLUT8)
  {
    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
  }
  /* ALU modes */
  if(Context_p->AluMode != STBLIT_ALU_COPY &&
     Context_p->AluMode != STBLIT_ALU_XOR)
  {
    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
  }
  
  if(Context_p->EnableClipRectangle &&
     Context_p->WriteInsideClipRectangle == FALSE)
  {
    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
  }
  if(Context_p->WriteInsideClipRectangle == FALSE)
  {
    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
  }
  /* ColorKey Type  can be only CLUT8 */
  if(Context_p->ColorKeyCopyMode != STBLIT_COLOR_KEY_MODE_NONE &&
     Context_p->ColorKey.Type != STGXOBJ_COLOR_KEY_TYPE_CLUT8)
  {
    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
  }
  if(Context_p->MaskBitmap_p != NULL &&
     (Context_p->ColorKeyCopyMode != STBLIT_COLOR_KEY_MODE_NONE ||
      Context_p->AluMode != STBLIT_ALU_COPY))
  {
    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
  }

  /* set the clipping rectangle according to blit context */
  if(ClipRect.PositionX<0)
  {
    ClipRect.Width -= ClipRect.PositionX;
    ClipRect.PositionX = 0;
  }
  if(ClipRect.PositionY<0)
  {
    ClipRect.Height -= ClipRect.PositionY;
    ClipRect.PositionY = 0;
  }
  if((ClipRect.PositionX+ClipRect.Width)>Bitmap_p->Width)
  {
    ClipRect.Width = Bitmap_p->Width - ClipRect.PositionX;
  }
  if((ClipRect.PositionY+ClipRect.Height)>Bitmap_p->Height)
  {
    ClipRect.Height = Bitmap_p->Height - ClipRect.PositionY;
  }
  Err = STOSD_SetDstClippingArea(RegionHandle,
                                 Context_p->ClipRectangle.PositionX,
                                 Context_p->ClipRectangle.PositionY,
                                 Context_p->ClipRectangle.Width,
                                 Context_p->ClipRectangle.Height);
  if(Err != ST_NO_ERROR)
  {
    return(Err);
  }

  /* set the color type for drawing */
  OSDColor.Type = STOSD_COLOR_TYPE_PALETTE;
  /* set the color value for drawing */
  switch(Color_p->Type)
  {
    case STGXOBJ_COLOR_TYPE_CLUT8:
      OSDColor.Value.PaletteEntry = Color_p->Value.CLUT8;
      break;
    default:
      return(ST_ERROR_FEATURE_NOT_SUPPORTED);
      break;
  }

  OSDColorTmp = OSDColor;
  /* if there is no colorkeying go on! */
  if(Context_p->ColorKeyCopyMode == STBLIT_COLOR_KEY_MODE_NONE)
  {
    if(Context_p->AluMode == STBLIT_ALU_COPY)
    {
      if(Context_p->MaskBitmap_p == NULL)
      {
        Err = STOSD_FillRectangle(RegionHandle,
                                Rectangle_p->PositionX, Rectangle_p->PositionY,
                                Rectangle_p->Width, Rectangle_p->Height,
                                OSDColor);
        if(Err != ST_NO_ERROR)
        {
            return(Err);
        }
        else
        {
          Err = stblit_Notify(Context_p);
        }
        return(Err);
      }
      else /* mask bitmap is on */
      {
        STOSD_Bitmap_t OSDBmap;
        U8* Data1_p = Context_p->MaskBitmap_p->Data1_p;
        U8* bmp_p;
        S32 x; /* current x in mask */
        S32 X; /* current x in bitmap */
        S32 y; /* start address of current line in mask */
        S32 Y; /* start address of current line in bitmap */
        S32 xstart, ystart;
        bmp_p = OSDBmap.Data_p = (void*)GliphBuffer;
        
        /* get the rectangle to be modified on the target */
        Err = STOSD_GetRectangle(RegionHandle,
                                 Rectangle_p->PositionX,
                                 Rectangle_p->PositionY,
                                 Rectangle_p->Width,
                                 Rectangle_p->Height,
                                 &OSDBmap);
        if(Err != ST_NO_ERROR)
        {
          return(Err);
        }
        
        xstart = (Rectangle_p->PositionX<0) ? -Rectangle_p->PositionX : 0;
        ystart = (Rectangle_p->PositionY<0) ?
                 -Rectangle_p->PositionY*Context_p->MaskBitmap_p->Pitch : 0;
    
        for(y=ystart, Y=0;                     /* start */
            Y < OSDBmap.Width*OSDBmap.Height;     /* stop condition */
            y += Context_p->MaskBitmap_p->Pitch, /* y increment */
            Y += OSDBmap.Width)                      /* Y increment */
        {
          for(x=xstart, X=0;
              X<OSDBmap.Width;
              x++, X++)
          {
            if((0x80 >> (x & 0x7)) & Data1_p[(x>>3) + y])
            {
              bmp_p[X + Y] = OSDColor.Value.PaletteEntry;
            }
          }
        }

        /* parameters_adjusting */
        if(Rectangle_p->PositionX < 0)
        {
          Rectangle_p->PositionX = 0;
        }
        if(Rectangle_p->PositionY < 0)
        {
          Rectangle_p->PositionY = 0;
        }
        
        Err = STOSD_PutRectangle(RegionHandle, Rectangle_p->PositionX,
                                 Rectangle_p->PositionY, &OSDBmap);
        if(Err != ST_NO_ERROR)
        {
          return(Err);
        }
        else
        {
          Err = stblit_Notify(Context_p);
        }
        return(Err);
      }
    }
    else /* ALU mode is not COPY */
    {
      S32 x, y;
      STOSD_Color_t DstColor;
      for(y=Rectangle_p->PositionY;
          y<Rectangle_p->PositionY + Rectangle_p->Height; y++)
      {
         for(x=Rectangle_p->PositionX;
             x<Rectangle_p->PositionX + Rectangle_p->Width; x++)

        {
          /* retrieve the value of the pixel */
          Err = STOSD_GetPixel(RegionHandle, x, y, &DstColor);
          if(Err != ST_NO_ERROR)
          {
            return(Err);
          }
          OSDColor = OSDColorTmp;
          switch(Context_p->AluMode)
          {
            case STBLIT_ALU_XOR:
              OSDColor.Value.PaletteEntry = ((*((U8*)(&(DstColor.Value)))) ^ 
                                             (*((U8*)(&(OSDColor.Value)))) );
              break;
            default:
              break;
          }
          Err = STOSD_SetPixel(RegionHandle, x, y,
                               OSDColor);
        }
      }
      if(Err != ST_NO_ERROR)
      {
          return(Err);
      }
      else
      {
          Err = stblit_Notify(Context_p);
      }
      return(Err);
    }
  }
  else if(Context_p->ColorKeyCopyMode == STBLIT_COLOR_KEY_MODE_SRC &&
          Context_p->ColorKey.Value.CLUT8.PaletteEntryEnable == TRUE)
  {
    /* check if the color to be written is in range */
    if((Context_p->ColorKey.Value.CLUT8.PaletteEntryOut == FALSE &&
        Color_p->Value.CLUT8 >=
        Context_p->ColorKey.Value.CLUT8.PaletteEntryMin          &&
        Color_p->Value.CLUT8 <=
        Context_p->ColorKey.Value.CLUT8.PaletteEntryMax            ) ||
       (Context_p->ColorKey.Value.CLUT8.PaletteEntryOut == TRUE  &&
        (Color_p->Value.CLUT8 <
         Context_p->ColorKey.Value.CLUT8.PaletteEntryMin ||
         Color_p->Value.CLUT8 >
         Context_p->ColorKey.Value.CLUT8.PaletteEntryMax   )           ))
    {
      if(Context_p->AluMode == STBLIT_ALU_COPY)
      {
        Err = STOSD_FillRectangle(RegionHandle,
                                Rectangle_p->PositionX,
                                Rectangle_p->PositionY,
                                Rectangle_p->Width, Rectangle_p->Height,
                                OSDColor);

          if(Err != ST_NO_ERROR)
          {
              return(Err);
          }
          else
          {
              Err = stblit_Notify(Context_p);
          }
          return(Err);
      }
      else
      {
        S32 x, y;
        STOSD_Color_t DstColor;
        for(y=Rectangle_p->PositionY;
            y<(Rectangle_p->PositionY + Rectangle_p->Height); y++)

        {
          for(x=Rectangle_p->PositionX;
              x<Rectangle_p->PositionX + Rectangle_p->Width; x++)

          {
            /* retrieve the value of the pixel */
            Err = STOSD_GetPixel(RegionHandle, x, y, &DstColor);
            OSDColor = OSDColorTmp;
            switch(Context_p->AluMode)
            {
              case STBLIT_ALU_XOR:
                OSDColor.Value.PaletteEntry = ((*((U8*)(&(DstColor.Value)))) ^ 
                                               (*((U8*)(&(OSDColor.Value)))) );
                break;
              default:
                break;
            }
            Err = STOSD_SetPixel(RegionHandle, x, y, OSDColor);
          }
        }
        if(Err != ST_NO_ERROR)
        {
            return(Err);
        }
        else
        {
            Err = stblit_Notify(Context_p);
        }
        return(Err);
      }
    }
  }
  else if(Context_p->ColorKeyCopyMode == STBLIT_COLOR_KEY_MODE_DST &&
          Context_p->ColorKey.Value.CLUT8.PaletteEntryEnable == TRUE)
  {
    S32 x, y;
    for(y=Rectangle_p->PositionY;
        y<Rectangle_p->PositionY + Rectangle_p->Height; y++)
    {
      for(x=Rectangle_p->PositionX;
          x<Rectangle_p->PositionX + Rectangle_p->Width; x++)
      {
        /* retrieve the value of the pixel */
        Err = STOSD_GetPixel(RegionHandle, x, y, &DstColor);
        /* check if the pixel can be overwritten */
        if((Context_p->ColorKey.Value.CLUT8.PaletteEntryOut == FALSE &&
            DstColor.Value.PaletteEntry >=
            Context_p->ColorKey.Value.CLUT8.PaletteEntryMin          &&
            DstColor.Value.PaletteEntry <=
            Context_p->ColorKey.Value.CLUT8.PaletteEntryMax            ) ||
           (Context_p->ColorKey.Value.CLUT8.PaletteEntryOut == TRUE  &&
            (DstColor.Value.PaletteEntry <
             Context_p->ColorKey.Value.CLUT8.PaletteEntryMin ||
             DstColor.Value.PaletteEntry >
             Context_p->ColorKey.Value.CLUT8.PaletteEntryMax   )           ))
        {
          OSDColor = OSDColorTmp;
          switch(Context_p->AluMode)
          {
            case STBLIT_ALU_XOR:
              OSDColor.Value.PaletteEntry = ((*((U8*)(&(DstColor.Value)))) ^ 
                                            (*((U8*)(&(OSDColor.Value)))) );
              break;
            default:
              break;
          }
          Err = STOSD_SetPixel(RegionHandle, x, y, OSDColor);
        }
      }
    }
  }
  if(Err != ST_NO_ERROR)
  {
      return(Err);
  }
  else
  {
      Err = stblit_Notify(Context_p);
  }
  return(Err);
} /* end of FillRectangle() */


ST_ErrorCode_t STBLIT_DrawHLine( STBLIT_Handle_t       Handle,
                                 STGXOBJ_Bitmap_t*     Bitmap_p,
                                 S32                   PositionX,
                                 S32                   PositionY,
                                 U32                   Length,
                                 STGXOBJ_Color_t*      Color_p,
                                 STBLIT_BlitContext_t* Context_p )
{
  STOSD_RegionHandle_t RegionHandle;
  STOSD_Color_t        OSDColor;
  STOSD_Color_t        OSDColorTmp;
  STOSD_Color_t        DstColor;
  ST_ErrorCode_t       Err = ST_NO_ERROR;
  STGXOBJ_Rectangle_t  ClipRect = Context_p->ClipRectangle;

  /* back casting to region handle */
  RegionHandle = (STOSD_RegionHandle_t)(Bitmap_p->Data1_p);
  if(Color_p->Type != STGXOBJ_COLOR_TYPE_CLUT8 &&
     Color_p->Type != STGXOBJ_COLOR_TYPE_CLUT4 &&
     Color_p->Type != STGXOBJ_COLOR_TYPE_CLUT2)
  {
    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
  }
  /* ALU modes */
  if(Context_p->AluMode != STBLIT_ALU_COPY &&
     Context_p->AluMode != STBLIT_ALU_XOR)
  {
    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
  }
  /* ColorKey Type  can be only CLUT8 */
  if(Context_p->ColorKeyCopyMode != STBLIT_COLOR_KEY_MODE_NONE &&
     Context_p->ColorKey.Type != STGXOBJ_COLOR_KEY_TYPE_CLUT8)
  {
    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
  }
  /* clipmode is only inside */
  if(Context_p->WriteInsideClipRectangle == FALSE)
  {
    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
  }
  /* ColorKey Type, if set, can be only CLUT8 */
  if(Context_p->ColorKeyCopyMode != STBLIT_COLOR_KEY_MODE_NONE &&
     Context_p->ColorKey.Type != STGXOBJ_COLOR_KEY_TYPE_CLUT8)
  {
    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
  }

  /* set the clipping rectangle according to blit context */
  if(ClipRect.PositionX<0)
  {
    ClipRect.Width -= ClipRect.PositionX;
    ClipRect.PositionX = 0;
  }
  if(ClipRect.PositionY<0)
  {
    ClipRect.Height -= ClipRect.PositionY;
    ClipRect.PositionY = 0;
  }
  if((ClipRect.PositionX+ClipRect.Width)>Bitmap_p->Width)
  {
    ClipRect.Width = Bitmap_p->Width - ClipRect.PositionX;
  }
  if((ClipRect.PositionY+ClipRect.Height)>Bitmap_p->Height)
  {
    ClipRect.Height = Bitmap_p->Height - ClipRect.PositionY;
  }
  Err = STOSD_SetDstClippingArea(RegionHandle,
                                 Context_p->ClipRectangle.PositionX,
                                 Context_p->ClipRectangle.PositionY,
                                 Context_p->ClipRectangle.Width,
                                 Context_p->ClipRectangle.Height);

  /* set the color type for drawing */
  OSDColor.Type = STOSD_COLOR_TYPE_PALETTE;
  /* set the color value for drawing */
  switch(Color_p->Type)
  {
    case STGXOBJ_COLOR_TYPE_CLUT8:
      OSDColor.Value.PaletteEntry = Color_p->Value.CLUT8;
      break;
    default:
      return(ST_ERROR_FEATURE_NOT_SUPPORTED);
      break;
  }

  OSDColorTmp = OSDColor;
  /* if there is no colorkeying go on! */
  if(Context_p->ColorKeyCopyMode == STBLIT_COLOR_KEY_MODE_NONE)
  {
    if(Context_p->AluMode == STBLIT_ALU_COPY)
    {
      Err = STOSD_FillRectangle(RegionHandle,
                                PositionX, PositionY,
                                Length, 1,
                                OSDColor);
      if(Err != ST_NO_ERROR)
      {
          return(Err);
      }
      else
      {
          Err = stblit_Notify(Context_p);
      }
      return(Err);
    }
    else /* ALU mode is not COPY */
    {
      S32 x; 
      STOSD_Color_t DstColor;
      for(x=PositionX; x<(PositionX + Length); x++)
      {
        /* retrieve the value of the pixel */
        Err = STOSD_GetPixel(RegionHandle, x, PositionY, &DstColor);
        OSDColor = OSDColorTmp;
        switch(Context_p->AluMode)
        {
          case STBLIT_ALU_XOR:
            OSDColor.Value.PaletteEntry = ((*((U8*)(&(DstColor.Value)))) ^ 
                                           (*((U8*)(&(OSDColor.Value)))) );
            break;
          default:
            break;
        }
        Err = STOSD_SetPixel(RegionHandle, x, PositionY, OSDColor);
      }
      if(Err != ST_NO_ERROR)
      {
          return(Err);
      }
      else
      {
          Err = stblit_Notify(Context_p);
      }
      return(Err);
    }
  }
  else if(Context_p->ColorKeyCopyMode == STBLIT_COLOR_KEY_MODE_SRC &&
          Context_p->ColorKey.Value.CLUT8.PaletteEntryEnable == TRUE)
  {
    /* check if the color to be written is in range */
    if((Context_p->ColorKey.Value.CLUT8.PaletteEntryOut == FALSE &&
        Color_p->Value.CLUT8 >=
        Context_p->ColorKey.Value.CLUT8.PaletteEntryMin          &&
        Color_p->Value.CLUT8 <=
        Context_p->ColorKey.Value.CLUT8.PaletteEntryMax            ) ||
       (Context_p->ColorKey.Value.CLUT8.PaletteEntryOut == TRUE  &&
        (Color_p->Value.CLUT8 <
         Context_p->ColorKey.Value.CLUT8.PaletteEntryMin ||
         Color_p->Value.CLUT8 >
         Context_p->ColorKey.Value.CLUT8.PaletteEntryMax   )           ))
    {
      if(Context_p->AluMode == STBLIT_ALU_COPY)
      {
        Err = STOSD_FillRectangle(RegionHandle, PositionX, PositionY,
                                  Length, 1,
                                  OSDColor);

          if(Err != ST_NO_ERROR)
          {
              return(Err);
          }
          else
          {
              Err = stblit_Notify(Context_p);
          }
          return(Err);

      }
      else
      {
        S32 x;
        STOSD_Color_t DstColor;
        for(x=PositionX; x<(PositionX + Length); x++)
        {
          /* retrieve the value of the pixel */
          Err = STOSD_GetPixel(RegionHandle, x, PositionY, &DstColor);
          OSDColor = OSDColorTmp;
          switch(Context_p->AluMode)
          {
            case STBLIT_ALU_XOR:
              OSDColor.Value.PaletteEntry = ((*((U8*)(&(DstColor.Value)))) ^ 
                                             (*((U8*)(&(OSDColor.Value)))) );
              break;
            default:
              break;
          }
          Err = STOSD_SetPixel(RegionHandle, x, PositionY, OSDColor);
        }
        if(Err != ST_NO_ERROR)
        {
            return(Err);
        }
        else
        {
            Err = stblit_Notify(Context_p);
        }
        return(Err);
      }
    }
  }
  else if(Context_p->ColorKeyCopyMode == STBLIT_COLOR_KEY_MODE_DST &&
          Context_p->ColorKey.Value.CLUT8.PaletteEntryEnable == TRUE)
  {
    S32 x;
    for(x=PositionX; x<(PositionX + Length); x++)
    {
      /* retrieve the value of the pixel */
      Err = STOSD_GetPixel(RegionHandle, x, PositionY, &DstColor);
      /* check if the pixel can be overwritten */
      if((Context_p->ColorKey.Value.CLUT8.PaletteEntryOut == FALSE &&
          DstColor.Value.PaletteEntry >=
          Context_p->ColorKey.Value.CLUT8.PaletteEntryMin          &&
          DstColor.Value.PaletteEntry <=
          Context_p->ColorKey.Value.CLUT8.PaletteEntryMax            ) ||
         (Context_p->ColorKey.Value.CLUT8.PaletteEntryOut == TRUE  &&
          (DstColor.Value.PaletteEntry <
           Context_p->ColorKey.Value.CLUT8.PaletteEntryMin ||
           DstColor.Value.PaletteEntry >
           Context_p->ColorKey.Value.CLUT8.PaletteEntryMax   )           ))
      {
        OSDColor = OSDColorTmp;
        switch(Context_p->AluMode)
        {
          case STBLIT_ALU_XOR:
            OSDColor.Value.PaletteEntry = ((*((U8*)(&(DstColor.Value)))) ^ 
                                          (*((U8*)(&(OSDColor.Value)))) );
            break;
          default:
            break;
        }
        Err = STOSD_SetPixel(RegionHandle, x, PositionY, OSDColor);
      }
    }    
  }
  if(Err != ST_NO_ERROR)
  {
      return(Err);
  }
  else
  {
      Err = stblit_Notify(Context_p);
  }
  return(Err);
} /* end of STBLIT_DrawHLine */

ST_ErrorCode_t STBLIT_DrawVLine( STBLIT_Handle_t       Handle,
                                 STGXOBJ_Bitmap_t*        Bitmap_p,
                                 S32                   PositionX,
                                 S32                   PositionY,
                                 U32                   Length,
                                 STGXOBJ_Color_t*         Color_p,
                                 STBLIT_BlitContext_t* Context_p )
{
  STOSD_RegionHandle_t RegionHandle;
  STOSD_Color_t        OSDColor;
  STOSD_Color_t        OSDColorTmp;
  STOSD_Color_t        DstColor;
  ST_ErrorCode_t       Err = ST_NO_ERROR;
  STGXOBJ_Rectangle_t  ClipRect = Context_p->ClipRectangle;

  /* back casting to region handle */
  RegionHandle = (STOSD_RegionHandle_t)(Bitmap_p->Data1_p);
  if(Color_p->Type != STGXOBJ_COLOR_TYPE_CLUT8 &&
     Color_p->Type != STGXOBJ_COLOR_TYPE_CLUT4 &&
     Color_p->Type != STGXOBJ_COLOR_TYPE_CLUT2)
  {
    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
  }
  /* ALU modes */
  if(Context_p->AluMode != STBLIT_ALU_COPY &&
     Context_p->AluMode != STBLIT_ALU_XOR)
  {
    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
  }
  /* clipmode is only inside */
  if(Context_p->WriteInsideClipRectangle == FALSE)
  {
    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
  }
  /* ColorKey Type  can be only CLUT8 */
  if(Context_p->ColorKeyCopyMode != STBLIT_COLOR_KEY_MODE_NONE &&
     Context_p->ColorKey.Type != STGXOBJ_COLOR_KEY_TYPE_CLUT8)
  {
    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
  }
  /* ColorKey Type, if set, can be only CLUT8 */
  if(Context_p->ColorKey.Type != STGXOBJ_COLOR_KEY_TYPE_CLUT8)
  {
    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
  }

  /* set the clipping rectangle according to blit context */
  if(ClipRect.PositionX<0)
  {
    ClipRect.Width -= ClipRect.PositionX;
    ClipRect.PositionX = 0;
  }
  if(ClipRect.PositionY<0)
  {
    ClipRect.Height -= ClipRect.PositionY;
    ClipRect.PositionY = 0;
  }
  if((ClipRect.PositionX+ClipRect.Width)>Bitmap_p->Width)
  {
    ClipRect.Width = Bitmap_p->Width - ClipRect.PositionX;
  }
  if((ClipRect.PositionY+ClipRect.Height)>Bitmap_p->Height)
  {
    ClipRect.Height = Bitmap_p->Height - ClipRect.PositionY;
  }
  Err = STOSD_SetDstClippingArea(RegionHandle,
                                 Context_p->ClipRectangle.PositionX,
                                 Context_p->ClipRectangle.PositionY,
                                 Context_p->ClipRectangle.Width,
                                 Context_p->ClipRectangle.Height);

  /* set the color type for drawing */
  OSDColor.Type = STOSD_COLOR_TYPE_PALETTE;
  /* set the color value for drawing */
  switch(Color_p->Type)
  {
    case STGXOBJ_COLOR_TYPE_CLUT8:
      OSDColor.Value.PaletteEntry = Color_p->Value.CLUT8;
      break;
    default:
      return(ST_ERROR_FEATURE_NOT_SUPPORTED);
      break;
  }

  OSDColorTmp = OSDColor;
  /* if there is no colorkeying go on! */
  if(Context_p->ColorKeyCopyMode == STBLIT_COLOR_KEY_MODE_NONE)
  {
    if(Context_p->AluMode == STBLIT_ALU_COPY)
    {
      Err = STOSD_FillRectangle(RegionHandle,
                                PositionX, PositionY,
                                1, Length,
                                OSDColor);
      if(Err != ST_NO_ERROR)
      {
          return(Err);
      }
      else
      {
          Err = stblit_Notify(Context_p);
      }
      return(Err);
    }
    else /* ALU mode is not COPY */
    {
      S32 y; 
      STOSD_Color_t DstColor;
      for(y=PositionY; y<(PositionY + Length); y++)
      {
        /* retrieve the value of the pixel */
        Err = STOSD_GetPixel(RegionHandle, PositionX, y, &DstColor);
        OSDColor = OSDColorTmp;
        switch(Context_p->AluMode)
        {
          case STBLIT_ALU_XOR:
            OSDColor.Value.PaletteEntry = ((*((U8*)(&(DstColor.Value)))) ^ 
                                           (*((U8*)(&(OSDColor.Value)))) );
            break;
          default:
            break;
        }
        Err = STOSD_SetPixel(RegionHandle, PositionX, y, OSDColor);
      }
      if(Err != ST_NO_ERROR)
      {
          return(Err);
      }
      else
      {
          Err = stblit_Notify(Context_p);
      }
      return(Err);
    }
  }
  else if(Context_p->ColorKeyCopyMode == STBLIT_COLOR_KEY_MODE_SRC &&
          Context_p->ColorKey.Value.CLUT8.PaletteEntryEnable == TRUE)
  {
    /* check if the color to be written is in range */
    if((Context_p->ColorKey.Value.CLUT8.PaletteEntryOut == FALSE &&
        Color_p->Value.CLUT8 >=
        Context_p->ColorKey.Value.CLUT8.PaletteEntryMin          &&
        Color_p->Value.CLUT8 <=
        Context_p->ColorKey.Value.CLUT8.PaletteEntryMax            ) ||
       (Context_p->ColorKey.Value.CLUT8.PaletteEntryOut == TRUE  &&
        (Color_p->Value.CLUT8 <
         Context_p->ColorKey.Value.CLUT8.PaletteEntryMin ||
         Color_p->Value.CLUT8 >
         Context_p->ColorKey.Value.CLUT8.PaletteEntryMax   )           ))
    {
      if(Context_p->AluMode == STBLIT_ALU_COPY)
      {
        Err = STOSD_FillRectangle(RegionHandle, PositionX, PositionY,
                                  1, Length,
                                  OSDColor);

        if(Err != ST_NO_ERROR)
        {
            return(Err);
        }
        else
        {
            Err = stblit_Notify(Context_p);
        }
        return(Err);
      }
      else
      {
        S32 y;
        STOSD_Color_t DstColor;
        for(y=PositionY; y<(PositionY + Length); y++)
        {
          /* retrieve the value of the pixel */
          Err = STOSD_GetPixel(RegionHandle, PositionX, y, &DstColor);
          OSDColor = OSDColorTmp;
          switch(Context_p->AluMode)
          {
            case STBLIT_ALU_XOR:
              OSDColor.Value.PaletteEntry = ((*((U8*)(&(DstColor.Value)))) ^ 
                                             (*((U8*)(&(OSDColor.Value)))) );
              break;
            default:
              break;
          }
          Err = STOSD_SetPixel(RegionHandle, PositionX, y, OSDColor);
        }
        if(Err != ST_NO_ERROR)
        {
            return(Err);
        }
        else
        {
            Err = stblit_Notify(Context_p);
        }
        return(Err);
      }
    }
  }
  else if(Context_p->ColorKeyCopyMode == STBLIT_COLOR_KEY_MODE_DST &&
          Context_p->ColorKey.Value.CLUT8.PaletteEntryEnable == TRUE)
  {
    S32 y;
    for(y=PositionY; y<(PositionY + Length); y++)
    {
      /* retrieve the value of the pixel */
      Err = STOSD_GetPixel(RegionHandle, PositionX, y, &DstColor);
      /* check if the pixel can be overwritten */
      if((Context_p->ColorKey.Value.CLUT8.PaletteEntryOut == FALSE &&
          DstColor.Value.PaletteEntry >=
          Context_p->ColorKey.Value.CLUT8.PaletteEntryMin          &&
          DstColor.Value.PaletteEntry <=
          Context_p->ColorKey.Value.CLUT8.PaletteEntryMax            ) ||
         (Context_p->ColorKey.Value.CLUT8.PaletteEntryOut == TRUE  &&
          (DstColor.Value.PaletteEntry <
           Context_p->ColorKey.Value.CLUT8.PaletteEntryMin ||
           DstColor.Value.PaletteEntry >
           Context_p->ColorKey.Value.CLUT8.PaletteEntryMax   )           ))
      {
        OSDColor = OSDColorTmp;
        switch(Context_p->AluMode)
        {
          case STBLIT_ALU_XOR:
            OSDColor.Value.PaletteEntry = ((*((U8*)(&(DstColor.Value)))) ^ 
                                          (*((U8*)(&(OSDColor.Value)))) );
            break;
          default:
            break;
        }
        Err = STOSD_SetPixel(RegionHandle, PositionX, y, OSDColor);
      }
    }    
  }
  if(Err != ST_NO_ERROR)
  {
      return(Err);
  }
  else
  {
      Err = stblit_Notify(Context_p);
  }
  return(Err);
} /* end of STBLIT_DrawVLine */

ST_ErrorCode_t STBLIT_DrawXYArray( STBLIT_Handle_t       Handle,
                                   STGXOBJ_Bitmap_t*     Bitmap_p,
                                   STBLIT_XY_t*          XYArray_p,
                                   U32                   PixelsNumber,
                                   STGXOBJ_Color_t*      Color_p,
                                   STBLIT_BlitContext_t* Context_p )
{
  STOSD_RegionHandle_t RegionHandle;
  STOSD_Color_t        OSDColor;
  STOSD_Color_t        OSDColorTmp;
  ST_ErrorCode_t       Err = ST_NO_ERROR;
  U32                  i;
  STGXOBJ_Rectangle_t  ClipRect = Context_p->ClipRectangle;

  /* back casting to region handle */
  RegionHandle = (STOSD_RegionHandle_t)(Bitmap_p->Data1_p);
  if(Color_p->Type != STGXOBJ_COLOR_TYPE_CLUT8 &&
     Color_p->Type != STGXOBJ_COLOR_TYPE_CLUT4 &&
     Color_p->Type != STGXOBJ_COLOR_TYPE_CLUT2)
  {
    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
  }
  /* ALU modes */
  if(Context_p->AluMode != STBLIT_ALU_COPY &&
     Context_p->AluMode != STBLIT_ALU_XOR)
  {
    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
  }
  /* clipmode is only inside */
  if(Context_p->WriteInsideClipRectangle == FALSE)
  {
    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
  }
  /* ColorKey Type  can be only CLUT8 */
  if(Context_p->ColorKeyCopyMode != STBLIT_COLOR_KEY_MODE_NONE &&
     Context_p->ColorKey.Type != STGXOBJ_COLOR_KEY_TYPE_CLUT8)
  {
    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
  }
  /* ColorKey Type, if set, can be only CLUT8 */
  if(Context_p->ColorKey.Type != STGXOBJ_COLOR_KEY_TYPE_CLUT8)
  {
    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
  }

  /* set the clipping rectangle according to blit context */
  if(ClipRect.PositionX<0)
  {
    ClipRect.Width -= ClipRect.PositionX;
    ClipRect.PositionX = 0;
  }
  if(ClipRect.PositionY<0)
  {
    ClipRect.Height -= ClipRect.PositionY;
    ClipRect.PositionY = 0;
  }
  if((ClipRect.PositionX+ClipRect.Width)>Bitmap_p->Width)
  {
    ClipRect.Width = Bitmap_p->Width - ClipRect.PositionX;
  }
  if((ClipRect.PositionY+ClipRect.Height)>Bitmap_p->Height)
  {
    ClipRect.Height = Bitmap_p->Height - ClipRect.PositionY;
  }
  Err = STOSD_SetDstClippingArea(RegionHandle,
                                 Context_p->ClipRectangle.PositionX,
                                 Context_p->ClipRectangle.PositionY,
                                 Context_p->ClipRectangle.Width,
                                 Context_p->ClipRectangle.Height);

  /* set the color type for drawing */
  OSDColor.Type = STOSD_COLOR_TYPE_PALETTE;
  /* set the color value for drawing */
  switch(Color_p->Type)
  {
    case STGXOBJ_COLOR_TYPE_CLUT8:
      OSDColor.Value.PaletteEntry = Color_p->Value.CLUT8;
      break;
    default:
      return(ST_ERROR_FEATURE_NOT_SUPPORTED);
      break;
  }

  OSDColorTmp = OSDColor;
  /* no colorkeying */
  if(Context_p->ColorKeyCopyMode == STBLIT_COLOR_KEY_MODE_NONE)
  {
    if(Context_p->AluMode == STBLIT_ALU_COPY)
    {
      for(i=0; i<PixelsNumber; i++)
      {
        Err = STOSD_FillRectangle(RegionHandle,
                                  XYArray_p[i].PositionX,
                                  XYArray_p[i].PositionY,
                                  1,
                                  1,
                                  OSDColor);
      }
      if(Err != ST_NO_ERROR)
      {
          return(Err);
      }
      else
      {
          Err = stblit_Notify(Context_p);
      }
      return(Err);
    }
    else /* ALU mode is not COPY */
    {
      STOSD_Color_t DstColor;
      for(i=0; i<PixelsNumber; i++)
      {
        /* retrieve the value of the pixel */
        Err = STOSD_GetPixel(RegionHandle, XYArray_p[i].PositionX,
                             XYArray_p[i].PositionY,
                             &DstColor);
        OSDColor = OSDColorTmp;
        switch(Context_p->AluMode)
        {
          case STBLIT_ALU_XOR:
            OSDColor.Value.PaletteEntry = ((*((U8*)(&(DstColor.Value)))) ^ 
                                           (*((U8*)(&(OSDColor.Value)))) );
            break;
          default:
            break;
        }
        Err = STOSD_SetPixel(RegionHandle, XYArray_p[i].PositionX,
                             XYArray_p[i].PositionY,
                             OSDColor);
      }
      if(Err != ST_NO_ERROR)
      {
          return(Err);
      }
      else
      {
          Err = stblit_Notify(Context_p);
      }
      return(Err);
    }
  }
  else if(Context_p->ColorKeyCopyMode == STBLIT_COLOR_KEY_MODE_SRC &&
          Context_p->ColorKey.Value.CLUT8.PaletteEntryEnable == TRUE)
  {
    /* check if the color to be written is in range */
    if((Context_p->ColorKey.Value.CLUT8.PaletteEntryOut == FALSE &&
        Color_p->Value.CLUT8 >=
        Context_p->ColorKey.Value.CLUT8.PaletteEntryMin          &&
        Color_p->Value.CLUT8 <=
        Context_p->ColorKey.Value.CLUT8.PaletteEntryMax            ) ||
       (Context_p->ColorKey.Value.CLUT8.PaletteEntryOut == TRUE  &&
        (Color_p->Value.CLUT8 <
         Context_p->ColorKey.Value.CLUT8.PaletteEntryMin ||
         Color_p->Value.CLUT8 >
         Context_p->ColorKey.Value.CLUT8.PaletteEntryMax   )           ))
    {
      if(Context_p->AluMode == STBLIT_ALU_COPY)
      {
        for(i=0; i<PixelsNumber; i++)
        {
          Err = STOSD_FillRectangle(RegionHandle,
                                    XYArray_p[i].PositionX,
                                    XYArray_p[i].PositionY,
                                    1,
                                    1,
                                    OSDColor);
        }
        if(Err != ST_NO_ERROR)
        {
            return(Err);
        }
        else
        {
            Err = stblit_Notify(Context_p);
        }
        return(Err);
      }
      else
      {
        STOSD_Color_t DstColor;
        for(i=0; i<PixelsNumber; i++)
        {
          /* retrieve the value of the pixel */
          Err = STOSD_GetPixel(RegionHandle,
                               XYArray_p[i].PositionX,
                               XYArray_p[i].PositionY,
                               &DstColor);
          OSDColor = OSDColorTmp;
          switch(Context_p->AluMode)
          {
            case STBLIT_ALU_XOR:
              OSDColor.Value.PaletteEntry = ((*((U8*)(&(DstColor.Value)))) ^ 
                                            (*((U8*)(&(OSDColor.Value)))) );
              break;
            default:
              break;
          }
          Err = STOSD_SetPixel(RegionHandle, XYArray_p[i].PositionX,
                               XYArray_p[i].PositionY,
                               OSDColor);
        }
        if(Err != ST_NO_ERROR)
        {
            return(Err);
        }
        else
        {
            Err = stblit_Notify(Context_p);
        }
        return(Err);
      }
    }     
  }
  else if(Context_p->ColorKeyCopyMode == STBLIT_COLOR_KEY_MODE_DST &&
          Context_p->ColorKey.Value.CLUT8.PaletteEntryEnable == TRUE)
  {
    STOSD_Color_t DstColor;
    for(i=0; i<PixelsNumber; i++)
    {
      /* retrieve the value of the pixel */
      Err = STOSD_GetPixel(RegionHandle,
                           XYArray_p[i].PositionX,
                           XYArray_p[i].PositionY,
                           &DstColor);
      /* check if the pixel can be overwritten */
      if((Context_p->ColorKey.Value.CLUT8.PaletteEntryOut == FALSE &&
          DstColor.Value.PaletteEntry >=
         Context_p->ColorKey.Value.CLUT8.PaletteEntryMin          &&
         DstColor.Value.PaletteEntry <=
        Context_p->ColorKey.Value.CLUT8.PaletteEntryMax            ) ||
       (Context_p->ColorKey.Value.CLUT8.PaletteEntryOut == TRUE  &&
         (DstColor.Value.PaletteEntry <
          Context_p->ColorKey.Value.CLUT8.PaletteEntryMin ||
          DstColor.Value.PaletteEntry >
          Context_p->ColorKey.Value.CLUT8.PaletteEntryMax   )           ))
      {
        OSDColor = OSDColorTmp;
        switch(Context_p->AluMode)
        {
          case STBLIT_ALU_XOR:
            OSDColor.Value.PaletteEntry = ((*((U8*)(&(DstColor.Value)))) ^ 
                                          (*((U8*)(&(OSDColor.Value)))) );
            break;
          default:
            break;
        }
        Err = STOSD_SetPixel(RegionHandle, XYArray_p[i].PositionX,
                             XYArray_p[i].PositionY,
                             OSDColor);
      }
    }
    if(Err != ST_NO_ERROR)
    {
        return(Err);
    }
    else
    {
        Err = stblit_Notify(Context_p);
    }
    return(Err);
  }
  if(Err != ST_NO_ERROR)
  {
      return(Err);
  }
  else
  {
      Err = stblit_Notify(Context_p);
  }
  return(Err);
} /* End of STBLIT_DrawXYArray() */

ST_ErrorCode_t STBLIT_DrawXYLArray( STBLIT_Handle_t       Handle,
                                    STGXOBJ_Bitmap_t*        Bitmap_p,
                                    STBLIT_XYL_t*         XYLArray_p,
                                    U32                   SegmentsNumber,
                                    STGXOBJ_Color_t*         Color_p,
                                    STBLIT_BlitContext_t* Context_p )
{
  STOSD_RegionHandle_t RegionHandle;
  STOSD_Color_t        OSDColor;
  STOSD_Color_t        OSDColorTmp;
  ST_ErrorCode_t       Err = ST_NO_ERROR;
  U32                  i;
  STGXOBJ_Rectangle_t  ClipRect = Context_p->ClipRectangle;

  /* back casting to region handle */
  RegionHandle = (STOSD_RegionHandle_t)(Bitmap_p->Data1_p);
  if(Color_p->Type != STGXOBJ_COLOR_TYPE_CLUT8 &&
     Color_p->Type != STGXOBJ_COLOR_TYPE_CLUT4 &&
     Color_p->Type != STGXOBJ_COLOR_TYPE_CLUT2)
  {
    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
  }
  /*ALU modes */
  if(Context_p->AluMode != STBLIT_ALU_COPY &&
     Context_p->AluMode != STBLIT_ALU_XOR)
  {
    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
  }
  /* clipmode is only inside */
  if(Context_p->WriteInsideClipRectangle == FALSE)
  {
    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
  }
  /* ColorKey Type  can be only CLUT8 */
  if(Context_p->ColorKeyCopyMode != STBLIT_COLOR_KEY_MODE_NONE &&
     Context_p->ColorKey.Type != STGXOBJ_COLOR_KEY_TYPE_CLUT8)
  {
    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
  }

  /* set the clipping rectangle according to blit context */
  if(ClipRect.PositionX<0)
  {
    ClipRect.Width -= ClipRect.PositionX;
    ClipRect.PositionX = 0;
  }
  if(ClipRect.PositionY<0)
  {
    ClipRect.Height -= ClipRect.PositionY;
    ClipRect.PositionY = 0;
  }
  if((ClipRect.PositionX+ClipRect.Width)>Bitmap_p->Width)
  {
    ClipRect.Width = Bitmap_p->Width - ClipRect.PositionX;
  }
  if((ClipRect.PositionY+ClipRect.Height)>Bitmap_p->Height)
  {
    ClipRect.Height = Bitmap_p->Height - ClipRect.PositionY;
  }
  Err = STOSD_SetDstClippingArea(RegionHandle,
                                 Context_p->ClipRectangle.PositionX,
                                 Context_p->ClipRectangle.PositionY,
                                 Context_p->ClipRectangle.Width,
                                 Context_p->ClipRectangle.Height);
  if(Err != ST_NO_ERROR)
  {
    return(Err);
  }

  /* set the color type for drawing */
  OSDColor.Type = STOSD_COLOR_TYPE_PALETTE;
  /* set the color value for drawing */
  switch(Color_p->Type)
  {
    case STGXOBJ_COLOR_TYPE_CLUT8:
      OSDColor.Value.PaletteEntry = Color_p->Value.CLUT8;
      break;
    default:
      return(ST_ERROR_FEATURE_NOT_SUPPORTED);
      break;
  }
  OSDColorTmp = OSDColor;

  /* no colorkeying */
  if(Context_p->ColorKeyCopyMode == STBLIT_COLOR_KEY_MODE_NONE)
  {
    if(Context_p->AluMode == STBLIT_ALU_COPY)
    {
      for(i=0; i<SegmentsNumber; i++)
      {
        if(!XYLArray_p[i].Length)
        {
          continue;
        }
        Err = STOSD_FillRectangle(RegionHandle,
                                  XYLArray_p[i].PositionX,
                                  XYLArray_p[i].PositionY,
                                  XYLArray_p[i].Length,
                                  1,
                                  OSDColor);
        if(Err != ST_NO_ERROR)
        {
          return(Err);
        }
      }
      if(Err != ST_NO_ERROR)
      {
          return(Err);
      }
      else
      {
          Err = stblit_Notify(Context_p);
      }
      return(Err);
    }
    else /* ALU mode is not COPY */
    {
      S32 x, start;
      STOSD_Color_t DstColor;
      for(i=0; i<SegmentsNumber; i++)
      {
        if((XYLArray_p[i].PositionX + (S32)XYLArray_p[i].Length) <= 0)
          continue;
        start = XYLArray_p[i].PositionX>=0 ? XYLArray_p[i].PositionX : 0;
        for(x=start;
            x<(XYLArray_p[i].PositionX + XYLArray_p[i].Length);
            x++)
        {
          /* retrieve the value of the pixel */
          Err = STOSD_GetPixel(RegionHandle, x, XYLArray_p[i].PositionY,
                               &DstColor);
          if(Err != ST_NO_ERROR)
          {
            return(Err);
          }
          OSDColor = OSDColorTmp;
          switch(Context_p->AluMode)
          {
            case STBLIT_ALU_XOR:
              OSDColor.Value.PaletteEntry = ((*((U8*)(&(DstColor.Value)))) ^ 
                                             (*((U8*)(&(OSDColor.Value)))) );
              break;
            default:
              break;
          }
          Err = STOSD_SetPixel(RegionHandle, x, XYLArray_p[i].PositionY,
                               OSDColor);
          if(Err != ST_NO_ERROR)
          {
            return(Err);
          }
        }
      }
      if(Err != ST_NO_ERROR)
      {
          return(Err);
      }
      else
      {
          Err = stblit_Notify(Context_p);
      }
      return(Err);
    }
  }
  else if(Context_p->ColorKeyCopyMode == STBLIT_COLOR_KEY_MODE_SRC &&
          Context_p->ColorKey.Value.CLUT8.PaletteEntryEnable == TRUE)
  {
    /* check if the color to be written is in range */
    if((Context_p->ColorKey.Value.CLUT8.PaletteEntryOut == FALSE &&
        Color_p->Value.CLUT8 >=
        Context_p->ColorKey.Value.CLUT8.PaletteEntryMin          &&
        Color_p->Value.CLUT8 <=
        Context_p->ColorKey.Value.CLUT8.PaletteEntryMax            ) ||
       (Context_p->ColorKey.Value.CLUT8.PaletteEntryOut == TRUE  &&
        (Color_p->Value.CLUT8 <
         Context_p->ColorKey.Value.CLUT8.PaletteEntryMin ||
         Color_p->Value.CLUT8 >
         Context_p->ColorKey.Value.CLUT8.PaletteEntryMax   )           ))
    {
      if(Context_p->AluMode == STBLIT_ALU_COPY)
      {
        for(i=0; i<SegmentsNumber; i++)
        {
          if(!XYLArray_p[i].Length)
          {
            continue;
          }
          Err = STOSD_FillRectangle(RegionHandle,
                                    XYLArray_p[i].PositionX,
                                    XYLArray_p[i].PositionY,
                                    XYLArray_p[i].Length,
                                    1,
                                    OSDColor);
          if(Err != ST_NO_ERROR)
          {
            return(Err);
          }                          
        }
        if(Err != ST_NO_ERROR)
        {
            return(Err);
        }
        else
        {
            Err = stblit_Notify(Context_p);
        }
        return(Err);
      }
      else
      {
        S32 x, start;
        STOSD_Color_t DstColor;
        for(i=0; i<SegmentsNumber; i++)
        {
          if((XYLArray_p[i].PositionX + (S32)XYLArray_p[i].Length) <= 0)
            continue;
          start = XYLArray_p[i].PositionX>=0 ? XYLArray_p[i].PositionX : 0;
          for(x=start;
              x<(XYLArray_p[i].PositionX + XYLArray_p[i].Length);
              x++)
          {
            /* retrieve the value of the pixel */
            Err = STOSD_GetPixel(RegionHandle,
                                 x,
                                 XYLArray_p[i].PositionY,
                                 &DstColor);
            if(Err != ST_NO_ERROR)
            {
              return(Err);
            }
            OSDColor = OSDColorTmp;
            switch(Context_p->AluMode)
            {
              case STBLIT_ALU_XOR:
                OSDColor.Value.PaletteEntry = ((*((U8*)(&(DstColor.Value)))) ^ 
                                              (*((U8*)(&(OSDColor.Value)))) );
                break;
              default:
                break;
            }
            Err = STOSD_SetPixel(RegionHandle, x, XYLArray_p[i].PositionY,
                                 OSDColor);
            if(Err != ST_NO_ERROR)
            {
              return(Err);
            }
          }
        }
        if(Err != ST_NO_ERROR)
        {
            return(Err);
        }
        else
        {
            Err = stblit_Notify(Context_p);
        }
        return(Err);
      }
    }     
  }
  else if(Context_p->ColorKeyCopyMode == STBLIT_COLOR_KEY_MODE_DST &&
          Context_p->ColorKey.Value.CLUT8.PaletteEntryEnable == TRUE)
  {
    S32 x;
    STOSD_Color_t DstColor;
    for(i=0; i<SegmentsNumber; i++)
    {
      for(x=XYLArray_p[i].PositionX;
          x<XYLArray_p[i].PositionX + XYLArray_p[i].Length;
          x++)
      {
        /* retrieve the value of the pixel */
        Err = STOSD_GetPixel(RegionHandle,
                             x,
                             XYLArray_p[i].PositionY,
                             &DstColor);
        if(Err != ST_NO_ERROR)
        {
          return(Err);
        }
        /* check if the pixel can be overwritten */
        if((Context_p->ColorKey.Value.CLUT8.PaletteEntryOut == FALSE &&
            DstColor.Value.PaletteEntry >=
           Context_p->ColorKey.Value.CLUT8.PaletteEntryMin          &&
           DstColor.Value.PaletteEntry <=
          Context_p->ColorKey.Value.CLUT8.PaletteEntryMax            ) ||
         (Context_p->ColorKey.Value.CLUT8.PaletteEntryOut == TRUE  &&
           (DstColor.Value.PaletteEntry <
            Context_p->ColorKey.Value.CLUT8.PaletteEntryMin ||
            DstColor.Value.PaletteEntry >
            Context_p->ColorKey.Value.CLUT8.PaletteEntryMax   )           ))
        {
          OSDColor = OSDColorTmp;
          switch(Context_p->AluMode)
          {
            case STBLIT_ALU_XOR:
              OSDColor.Value.PaletteEntry = ((*((U8*)(&(DstColor.Value)))) ^ 
                                            (*((U8*)(&(OSDColor.Value)))) );
              break;
            default:
              break;
          }
          Err = STOSD_SetPixel(RegionHandle, x, XYLArray_p[i].PositionY,
                               OSDColor);
          if(Err != ST_NO_ERROR)
          {
            return(Err);
          }
        }
      }
    }
    if(Err != ST_NO_ERROR)
    {
        return(Err);
    }
    else
    {
        Err = stblit_Notify(Context_p);
    }
    return(Err);
  }
  if(Err != ST_NO_ERROR)
  {
      return(Err);
  }
  else
  {
      Err = stblit_Notify(Context_p);
  }
  return(Err);
} /* End of STBLIT_DrawXYLArray() */

ST_ErrorCode_t STBLIT_DrawXYCArray( STBLIT_Handle_t       Handle,
                                    STGXOBJ_Bitmap_t*     Bitmap_p,
                                    STBLIT_XYC_t*         XYArray_p,
                                    U32                   PixelsNumber,
                                    STGXOBJ_Color_t*      ColorArray_p,
                                    STBLIT_BlitContext_t* Context_p )
{
  U32 i;
  ST_ErrorCode_t       Err = ST_NO_ERROR;

  /* clipmode is only inside */
  if(Context_p->WriteInsideClipRectangle == FALSE)
  {
    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
  }
  for(i=0; i<PixelsNumber; i++)
  {
    Err = STBLIT_SetPixel(Handle,
                          Bitmap_p,
                          XYArray_p[i].PositionX,
                          XYArray_p[i].PositionY,
                          &ColorArray_p[i],
                          Context_p);
    if(Err != ST_NO_ERROR)
      return(Err);
  }
  if(Err != ST_NO_ERROR)
  {
      return(Err);
  }
  else
  {
      Err = stblit_Notify(Context_p);
  }
  return(Err);
} /* End of STBLIT_DrawXYCArray() */

ST_ErrorCode_t STBLIT_DrawXYLCArray( STBLIT_Handle_t   Handle,
                                     STGXOBJ_Bitmap_t* Bitmap_p,
                                     STBLIT_XYL_t*     XYLArray_p,
                                     U32               SegmentsNumber,
                                     STGXOBJ_Color_t*  ColorArray_p,
                                     STBLIT_BlitContext_t* Context_p )
{
  U32 i;
  ST_ErrorCode_t       Err = ST_NO_ERROR;

  /* clipmode is only inside */
  if(Context_p->WriteInsideClipRectangle == FALSE)
  {
    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
  }
  for(i=0; i<SegmentsNumber; i++)
  {
    Err = STBLIT_DrawHLine(Handle,
                           Bitmap_p,
                           XYLArray_p[i].PositionX,
                           XYLArray_p[i].PositionY,
                           XYLArray_p[i].Length,
                           &ColorArray_p[i],
                           Context_p);
    if(Err != ST_NO_ERROR)
      return(Err);
  }
  if(Err != ST_NO_ERROR)
  {
      return(Err);
  }
  else
  {
      Err = stblit_Notify(Context_p);
  }
  return(Err);

}




/*******************************************************************************
Name        : STBLIT_CopyRectangle()
Description : copies a rectangle between two bitmaps
Parameters  : see prototype
Assumptions : 
Limitations : source and destination rectangles are _completely_ inside the
              respective bitmaps
Returns     :
*******************************************************************************/
ST_ErrorCode_t STBLIT_CopyRectangle(STBLIT_Handle_t       Handle,
                                    STGXOBJ_Bitmap_t*        SrcBitmap_p,
                                    STGXOBJ_Rectangle_t*     SrcRectangle_p,
                                    STGXOBJ_Bitmap_t*        DstBitmap_p,
                                    S32                   DstPositionX,
                                    S32                   DstPositionY,
                                    STBLIT_BlitContext_t* Context_p )
{
  STOSD_RegionHandle_t SrcRegionHandle;
  STOSD_RegionHandle_t DstRegionHandle;
  ST_ErrorCode_t       Err = ST_NO_ERROR;
  stblit_CopyDirection Dir;
  STGXOBJ_Rectangle_t  ClipRect = Context_p->ClipRectangle;
  

  /* Copyrectangle cannot perform color/bmap type conversion */
  if(SrcBitmap_p->BitmapType != DstBitmap_p->BitmapType ||
     SrcBitmap_p->ColorType != DstBitmap_p->ColorType)
  {
    return(ST_ERROR_BAD_PARAMETER);
  }
  /* this version operates only on STOSD regions,
     the corrensponding bitmap  type must be
     STGXOBJ_BITMAP_TYPE_RASTER_TOP_BOTTOM */
  if(SrcBitmap_p->BitmapType != STGXOBJ_BITMAP_TYPE_RASTER_TOP_BOTTOM ||
     SrcBitmap_p->ColorType != STGXOBJ_COLOR_TYPE_CLUT8)
  {
    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
  }
  
  /* back casting to region handle */
  SrcRegionHandle = (STOSD_RegionHandle_t)(SrcBitmap_p->Data1_p);
  DstRegionHandle = (STOSD_RegionHandle_t)(DstBitmap_p->Data1_p);

  /* ALU modes */
  if(Context_p->AluMode != STBLIT_ALU_COPY &&
     Context_p->AluMode != STBLIT_ALU_XOR)
  {
    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
  }
  /* clipmode is only inside */
  if(Context_p->WriteInsideClipRectangle == FALSE)
  {
    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
  }
  /* ColorKey Type, if set, can be only CLUT8 */
  if(Context_p->ColorKeyCopyMode != STBLIT_COLOR_KEY_MODE_NONE &&
     Context_p->ColorKey.Type != STGXOBJ_COLOR_KEY_TYPE_CLUT8)
  {
    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
  }

  /* set the clipping rectangle, if needed, according to blit context */
  if(ClipRect.PositionX<0)
  {
    ClipRect.Width -= ClipRect.PositionX;
    ClipRect.PositionX = 0;
  }
  if(ClipRect.PositionY<0)
  {
    ClipRect.Height -= ClipRect.PositionY;
    ClipRect.PositionY = 0;
  }
  if((ClipRect.PositionX+ClipRect.Width)>DstBitmap_p->Width)
  {
    ClipRect.Width = DstBitmap_p->Width - ClipRect.PositionX;
  }
  if((ClipRect.PositionY+ClipRect.Height)>DstBitmap_p->Height)
  {
    ClipRect.Height = DstBitmap_p->Height - ClipRect.PositionY;
  }
  Err = STOSD_SetDstClippingArea(DstRegionHandle,
                                 ClipRect.PositionX,
                                 ClipRect.PositionY,
                                 ClipRect.Width,
                                 ClipRect.Height);
  if(Err != ST_NO_ERROR)
  {
    return(Err);
  }
  
  /* direction setting */
  if(DstPositionX>SrcRectangle_p->PositionX &&
     DstPositionY>SrcRectangle_p->PositionY)
  {
    Dir = SE;
  }
  else if(DstPositionX<=SrcRectangle_p->PositionX &&
          DstPositionY<=SrcRectangle_p->PositionY)
  {
    Dir = NW;
  }
  else if(DstPositionX>SrcRectangle_p->PositionX &&
          DstPositionY<=SrcRectangle_p->PositionY)
  {
    Dir = NE;
  }
  else if(DstPositionX<=SrcRectangle_p->PositionX &&
          DstPositionY>SrcRectangle_p->PositionY)
  {
    Dir = SW;
  }
  
  /* if there is no colorkeying go on! */
  if(Context_p->ColorKeyCopyMode == STBLIT_COLOR_KEY_MODE_NONE)
  {
    if(Context_p->AluMode == STBLIT_ALU_COPY)
    {
      Err = STOSD_CopyRectangle(SrcRegionHandle,
                                SrcRectangle_p->PositionX,
                                SrcRectangle_p->PositionY,
                                SrcRectangle_p->Width,
                                SrcRectangle_p->Height,
                                DstRegionHandle,
                                DstPositionX,
                                DstPositionY);
      if(Err != ST_NO_ERROR)
      {
          return(Err);
      }
      else
      {
          Err = stblit_Notify(Context_p);
      }
      return(Err);
    }
    else /* ALU mode is not copy */
    {
      S32 x, y;
      S32 StartX, StartY;
      STOSD_Color_t DstColor, SrcColor;
      
      switch(Dir)
      {
        case SE:
          StartX = SrcRectangle_p->Width - 1;
          StartY = SrcRectangle_p->Height - 1;
          break;
        case NW:
          StartX = 0;
          StartY = 0;
          break;
        case SW:
          StartX = 0;
          StartY = SrcRectangle_p->Height - 1;
          break;
        case NE:
          StartX = SrcRectangle_p->Width - 1;
          StartY = 0;
          break;
        default:
          break;
      }
      for(y=StartY; (y<SrcRectangle_p->Height) && (y>=0);)
      {
        for(x=StartX; (x<SrcRectangle_p->Width) && (x>=0);)
        {
          Err = STOSD_GetPixel(SrcRegionHandle,
                               x + SrcRectangle_p->PositionX,
                               y + SrcRectangle_p->PositionY,
                               &SrcColor);
          if(Err != ST_NO_ERROR)
            return(Err);
          Err = STOSD_GetPixel(DstRegionHandle,
                               x + DstPositionX,
                               y + DstPositionY,
                               &DstColor);
          if(Err != ST_NO_ERROR)
            return(Err);
          switch(Context_p->AluMode)
          {
            case STBLIT_ALU_XOR:
              DstColor.Value.PaletteEntry = DstColor.Value.PaletteEntry ^ 
                                            SrcColor.Value.PaletteEntry;
              break;
            default:
              break;
          }
          Err = STOSD_SetPixel(DstRegionHandle,
                             x + DstPositionX,
                             y + DstPositionY,
                             DstColor);
          if(Err != ST_NO_ERROR)
            return(Err);
          if(Dir == SE || Dir == NE)
            x--;
          else
            x++;
        }
        if(Dir == SE || Dir == SW)
          y--;
        else
          y++;
      }
    }
    if(Err != ST_NO_ERROR)
    {
        return(Err);
    }
    else
    {
        Err = stblit_Notify(Context_p);
    }
    return(Err);
  }
  /* else: source color keying */
  else if(Context_p->ColorKeyCopyMode == STBLIT_COLOR_KEY_MODE_SRC &&
          Context_p->ColorKey.Value.CLUT8.PaletteEntryEnable == TRUE)
  {
    S32 x, y;
    S32 StartX, StartY;
    STOSD_Color_t DstColor, SrcColor;
    
    switch(Dir)
    {
      case SE:
        StartX = SrcRectangle_p->Width - 1;
        StartY = SrcRectangle_p->Height - 1;
        break;
      case NW:
        StartX = 0;
        StartY = 0;
        break;
      case SW:
        StartX = 0;
        StartY = SrcRectangle_p->Height - 1;
        break;
      case NE:
        StartX = SrcRectangle_p->Width - 1;
        StartY = 0;
        break;
      default:
        break;
    }
    for(y=StartY; (y<SrcRectangle_p->Height) && (y>=0);)
    {
      for(x=StartX; (x<SrcRectangle_p->Width) && (x>=0);)
      {
        Err = STOSD_GetPixel(SrcRegionHandle,
                             x + SrcRectangle_p->PositionX,
                             y + SrcRectangle_p->PositionY,
                             &SrcColor);
        if(Err != ST_NO_ERROR)
          return(Err);
        if((Context_p->ColorKey.Value.CLUT8.PaletteEntryOut == FALSE &&
          SrcColor.Value.PaletteEntry >=
          Context_p->ColorKey.Value.CLUT8.PaletteEntryMin          &&
          SrcColor.Value.PaletteEntry <=
          Context_p->ColorKey.Value.CLUT8.PaletteEntryMax            ) ||
         (Context_p->ColorKey.Value.CLUT8.PaletteEntryOut == TRUE  &&
         (SrcColor.Value.PaletteEntry <
          Context_p->ColorKey.Value.CLUT8.PaletteEntryMin ||
          SrcColor.Value.PaletteEntry >
          Context_p->ColorKey.Value.CLUT8.PaletteEntryMax   )           ))
        {
          Err = STOSD_GetPixel(DstRegionHandle,
                               x + DstPositionX,
                               y + DstPositionY,
                               &DstColor);
        if(Err != ST_NO_ERROR)
          return(Err);
          switch(Context_p->AluMode)
          {
            case STBLIT_ALU_COPY:
              DstColor.Value.PaletteEntry = SrcColor.Value.PaletteEntry;
              break;
            case STBLIT_ALU_XOR:
              DstColor.Value.PaletteEntry = DstColor.Value.PaletteEntry ^ 
                                            SrcColor.Value.PaletteEntry;
              break;
            default:
              break;
          }
          Err = STOSD_SetPixel(DstRegionHandle,
                               x + DstPositionX,
                               y + DstPositionY,
                               DstColor);
          if(Err != ST_NO_ERROR)
            return(Err);
        }  
        if(Dir == SE || Dir == NE)
            x--;
        else
          x++;
      }
      if(Dir == SE || Dir == SW)
        y--;
      else
        y++;
    }
    if(Err != ST_NO_ERROR)
    {
        return(Err);
    }
    else
    {
        Err = stblit_Notify(Context_p);
    }
    return(Err);
  }
  /* else: destination color keying */
  else if(Context_p->ColorKeyCopyMode == STBLIT_COLOR_KEY_MODE_DST &&
          Context_p->ColorKey.Value.CLUT8.PaletteEntryEnable == TRUE)
  {
    S32 x, y;
    S32 StartX, StartY;
    STOSD_Color_t DstColor, SrcColor;
    
    switch(Dir)
    {
      case SE:
        StartX = SrcRectangle_p->Width - 1;
        StartY = SrcRectangle_p->Height - 1;
        break;
      case NW:
        StartX = 0;
        StartY = 0;
        break;
      case SW:
        StartX = 0;
        StartY = SrcRectangle_p->Height - 1;
        break;
      case NE:
        StartX = SrcRectangle_p->Width - 1;
        StartY = 0;
        break;
      default:
        break;
    }
    for(y=StartY; (y<SrcRectangle_p->Height) && (y>=0);)
    {
      for(x=StartX; (x<SrcRectangle_p->Width) && (x>=0);)
      {
        Err = STOSD_GetPixel(DstRegionHandle,
                             x + DstPositionX,
                             y + DstPositionY,
                             &DstColor);
        if((Context_p->ColorKey.Value.CLUT8.PaletteEntryOut == FALSE &&
          DstColor.Value.PaletteEntry >=
          Context_p->ColorKey.Value.CLUT8.PaletteEntryMin          &&
          DstColor.Value.PaletteEntry <=
          Context_p->ColorKey.Value.CLUT8.PaletteEntryMax            ) ||
         (Context_p->ColorKey.Value.CLUT8.PaletteEntryOut == TRUE  &&
         (DstColor.Value.PaletteEntry <
          Context_p->ColorKey.Value.CLUT8.PaletteEntryMin ||
          DstColor.Value.PaletteEntry >
          Context_p->ColorKey.Value.CLUT8.PaletteEntryMax   )           ))
        {
          Err = STOSD_GetPixel(SrcRegionHandle,
                               x + SrcRectangle_p->PositionX,
                               y + SrcRectangle_p->PositionY,
                               &SrcColor);
          switch(Context_p->AluMode)
          {
            case STBLIT_ALU_COPY:
              DstColor.Value.PaletteEntry = SrcColor.Value.PaletteEntry;
              break;
            case STBLIT_ALU_XOR:
              DstColor.Value.PaletteEntry = DstColor.Value.PaletteEntry ^ 
                                            SrcColor.Value.PaletteEntry;
              break;
            default:
              break;
          }
          Err = STOSD_SetPixel(DstRegionHandle,
                             x + DstPositionX,
                             y + DstPositionY,
                             DstColor);
        }
        if(Dir == SE || Dir == NE)
            x--;
        else
          x++;
      }
      if(Dir == SE || Dir == SW)
        y--;
      else
        y++;
    }
    if(Err != ST_NO_ERROR)
    {
        return(Err);
    }
    else
    {
        Err = stblit_Notify(Context_p);
    }
    return(Err);
  }
  if(Err != ST_NO_ERROR)
  {
      return(Err);
  }
  else
  {
      Err = stblit_Notify(Context_p);
  }
  return(Err);
} /*  end of STBLIT_CopyRectangle() */

/*******************************************************************************
Name        : STBLIT_Blit()
Description : copies a rectangle between two bitmaps of different type
Parameters  : see prototype
Assumptions : Destination is an 8bpp OSD region;
              Source can be:
              - a CLUT8 raster progressive bitmap
                (for copies from raster bitmaps to OSD regions)
                OR, for text rendering purposes:
                - a CLUT1 progressive bitmap (a glyph)
                  OR
                - a CLUT 8 color, and the Blit context has a bitmap mask
                  which is a CLUT1 bitmap (a glyph).
Limitations : source and destination rectangles are _completely_ inside the
              respective bitmaps
Returns     :
*******************************************************************************/
ST_ErrorCode_t STBLIT_Blit(STBLIT_Handle_t             Handle,
                           STBLIT_Source_t*            Src1_p,
                           STBLIT_Source_t*            Src2_p,
                           STBLIT_Destination_t*       Dst_p,
                           STBLIT_BlitContext_t*       BlitContext_p)
{
  ST_ErrorCode_t          Err = ST_NO_ERROR;
  STOSD_RegionHandle_t    DstRegionHandle;
  STOSD_Bitmap_t          OSDBmap = {STOSD_COLOR_TYPE_PALETTE,
                                       STOSD_BITMAP_TYPE_DIB_BYTE_ALIGN,
                                       8, /* bpp */
                                       1, /* width = 1 */
                                       1, /* height = 1 */
                                       NULL}; /* Data_p = NULL */
  STGXOBJ_Rectangle_t     ClipRect = BlitContext_p->ClipRectangle;

  /* this version can not have 2 sources */
  if(Src2_p != NULL)
    return(ST_ERROR_FEATURE_NOT_SUPPORTED);

  /* check if source 1 is a raster progressive bitmap CLUT 8 */
  if(Src1_p->Type == STBLIT_SOURCE_TYPE_BITMAP)
  {
    if(Src1_p->Data.Bitmap_p->BitmapType !=
         STGXOBJ_BITMAP_TYPE_RASTER_PROGRESSIVE)
    {
      return(ST_ERROR_FEATURE_NOT_SUPPORTED);  
    }
    if(Src1_p->Data.Bitmap_p->ColorType != STGXOBJ_COLOR_TYPE_CLUT8)
    {
      return(ST_ERROR_FEATURE_NOT_SUPPORTED);  
    }
  }
  
  /* check if destination is a top-bottom bmap, i.e. an OSD region */
  if(Dst_p->Bitmap_p->BitmapType != STGXOBJ_BITMAP_TYPE_RASTER_TOP_BOTTOM ||
     Dst_p->Bitmap_p->ColorType != STGXOBJ_COLOR_TYPE_CLUT8)
  {
    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
  }
    
  /* back casting to region handle */
  DstRegionHandle = (STOSD_RegionHandle_t)(Dst_p->Bitmap_p->Data1_p);

  /* ALU modes */
  if(BlitContext_p->AluMode != STBLIT_ALU_COPY)
  {
    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
  }
  
  /* clipmode is only inside */
  if(BlitContext_p->EnableClipRectangle == TRUE &&
     BlitContext_p->WriteInsideClipRectangle == FALSE)
  {
    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
  }
  
  /* ColorKey is partially supported */
  /* this portion of code as been written only for teletext fonts */
  if(BlitContext_p->ColorKeyCopyMode != STBLIT_COLOR_KEY_MODE_NONE &&
     BlitContext_p->ColorKey.Value.CLUT8.PaletteEntryEnable == TRUE)
  {
    S32 x, y;
    STOSD_Color_t DstColor;
    U8*  SrcColor = Src1_p->Data.Bitmap_p->Data1_p;

    if (BlitContext_p->ColorKeyCopyMode != STBLIT_COLOR_KEY_MODE_SRC ||
        BlitContext_p->ColorKey.Value.CLUT8.PaletteEntryOut != FALSE ||
        BlitContext_p->AluMode != STBLIT_ALU_COPY)
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);

    for (y=0; y<Src1_p->Rectangle.Height; ++y)
    {
      for (x=0; x<Src1_p->Rectangle.Width; ++x)
      {
        if (*SrcColor < BlitContext_p->ColorKey.Value.CLUT8.PaletteEntryMin ||
            *SrcColor > BlitContext_p->ColorKey.Value.CLUT8.PaletteEntryMax)
        {
            Err = STOSD_GetPixel(DstRegionHandle,
                                   x + Dst_p->Rectangle.PositionX,
                                   y + Dst_p->Rectangle.PositionY,
                                   &DstColor);
            if (Err != ST_NO_ERROR) return(Err);
            DstColor.Value.PaletteEntry = *SrcColor;
            Err = STOSD_SetPixel(DstRegionHandle,
                               x + Dst_p->Rectangle.PositionX,
                               y + Dst_p->Rectangle.PositionY,
                               DstColor);
            if (Err != ST_NO_ERROR) return(Err);
        }
        ++SrcColor;
      }
    }
    Err = stblit_Notify(BlitContext_p);
    return(Err);
  } 

  /* set the clipping rectangle, if needed, according to blit context */
  if(BlitContext_p->EnableClipRectangle == TRUE)
  {
    if(ClipRect.PositionX<0)
    {
      ClipRect.Width -= ClipRect.PositionX;
      ClipRect.PositionX = 0;
    }
    if(ClipRect.PositionY<0)
    {
      ClipRect.Height -= ClipRect.PositionY;
      ClipRect.PositionY = 0;
    }
    if((ClipRect.PositionX+ClipRect.Width)>Dst_p->Bitmap_p->Width)
    {
      ClipRect.Width = Dst_p->Bitmap_p->Width - ClipRect.PositionX;
    }
    if((ClipRect.PositionY+ClipRect.Height)>Dst_p->Bitmap_p->Height)
    {
      ClipRect.Height = Dst_p->Bitmap_p->Height - ClipRect.PositionY;
    }
    Err = STOSD_SetDstClippingArea(DstRegionHandle,
                                   ClipRect.PositionX,
                                   ClipRect.PositionY,
                                   ClipRect.Width,
                                  ClipRect.Height);
    if(Err != ST_NO_ERROR)
    {
      return(Err);
    }
  }

  OSDBmap.BitmapType = STOSD_BITMAP_TYPE_DIB_BYTE_ALIGN;
  OSDBmap.ColorType = STOSD_COLOR_TYPE_PALETTE;
  OSDBmap.BitsPerPel = 8;
  OSDBmap.Width  = Src1_p->Rectangle.Width;
  OSDBmap.Height = Src1_p->Rectangle.Height;
  
  if(Src1_p->Type == STBLIT_SOURCE_TYPE_BITMAP &&
     Src1_p->Data.Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_CLUT8)
  {
    OSDBmap.Data_p = Src1_p->Data.Bitmap_p->Data1_p;
    /* copy source bitmap onto region */
    Err = STOSD_PutRectangle(DstRegionHandle, Dst_p->Rectangle.PositionX,
                             Dst_p->Rectangle.PositionY, &OSDBmap);
    if(Err != ST_NO_ERROR)
    {
      return(Err);
    }
  }
  else /* source is a color */
  {
    U8* Data1_p = BlitContext_p->MaskBitmap_p->Data1_p;
    U8* bmp_p;
    S32 x; /* current x in mask */
    S32 X; /* current x in bitmap */
    S32 y; /* start address of current line in mask */
    S32 Y; /* start address of current line in bitmap */
    S32 xstart, ystart;

    
    bmp_p = OSDBmap.Data_p = (void*)GliphBuffer;
    /* get the rectangle to be modified on the target */
    if (Src1_p->Type == STBLIT_SOURCE_TYPE_COLOR) 
    {
      Err = STOSD_GetRectangle(DstRegionHandle,
                               Dst_p->Rectangle.PositionX,
                               Dst_p->Rectangle.PositionY,
                               Src1_p->Rectangle.Width,
                               Src1_p->Rectangle.Height,
                               &OSDBmap);
      if(Err != ST_NO_ERROR)
      {
        return(Err);
      }
    }
    
    xstart = (Dst_p->Rectangle.PositionX<0) ? -Dst_p->Rectangle.PositionX : 0;
    ystart = (Dst_p->Rectangle.PositionY<0) ?
             -Dst_p->Rectangle.PositionY*BlitContext_p->MaskBitmap_p->Pitch : 0;
    
    for(y=ystart, Y=0;                         /* start */
        Y < OSDBmap.Width*OSDBmap.Height;     /* stop condition */
        y+=BlitContext_p->MaskBitmap_p->Pitch, /* y increment */
        Y+=OSDBmap.Width)                      /* Y increment */
    {
      for(x=xstart, X=0;
          X<OSDBmap.Width;
          x++, X++)
      {
        if((0x80 >> (x & 0x7)) & Data1_p[(x>>3) + y])
        {
          bmp_p[X + Y] = Src1_p->Data.Color_p->Value.CLUT8;
        }
      }
    }

    /* parameters_adjusting */
    if(Dst_p->Rectangle.PositionX < 0)
    {
      Dst_p->Rectangle.PositionX = 0;
    }
    if(Dst_p->Rectangle.PositionY < 0)
    {
      Dst_p->Rectangle.PositionY = 0;
    }
    Err = STOSD_PutRectangle(DstRegionHandle, Dst_p->Rectangle.PositionX,
                             Dst_p->Rectangle.PositionY, &OSDBmap);
  }
  
  if(Err != ST_NO_ERROR)
  {
      return(Err);
  }
  else
  {
      Err = stblit_Notify(BlitContext_p);
  }
  return(Err);
} /*  end of STBLIT_Blit() */


ST_ErrorCode_t STBLIT_GetPixel(STBLIT_Handle_t       Handle,
                               STGXOBJ_Bitmap_t*        Bitmap_p,
                               S32                   PositionX,
                               S32                   PositionY,
                               STGXOBJ_Color_t*         Color_p)
{
  STOSD_RegionHandle_t RegionHandle;
  STOSD_Color_t        OSDColor;
  ST_ErrorCode_t       Err = ST_NO_ERROR;

  /* back casting to region handle */
  RegionHandle = (STOSD_RegionHandle_t)(Bitmap_p->Data1_p);
  Color_p->Type = Bitmap_p->ColorType;
  Err = STOSD_GetPixel(RegionHandle, PositionX, PositionY, &OSDColor);
  if(Err != ST_NO_ERROR)
  {
    return(Err);
  }
  (*Color_p).Value.CLUT8 = OSDColor.Value.PaletteEntry;
  return(ST_NO_ERROR);
} /* end of STBLIT_GetPixel() */
  

static ST_ErrorCode_t stblit_Notify(STBLIT_BlitContext_t *Context)
{
    ST_ErrorCode_t  Error;

    if ( Context->NotifyBlitCompletion == TRUE )
    {
        Error = STEVT_NotifySubscriber( EvtHandle, EvtId, 
                                        Context->UserTag_p, Context->EventSubscriberID);
        if(Error != ST_NO_ERROR)
        {
            return(Error);
        }
    }

    return (ST_NO_ERROR);
}
        

    

/* EOF: End of stblitwrap_cmd.c */







