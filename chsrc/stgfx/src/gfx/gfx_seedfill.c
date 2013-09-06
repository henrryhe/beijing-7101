/*******************************************************************************

File name   : gfx_seedfill.c

Description :  STGFX seed filling functions

COPYRIGHT (C) STMicroelectronics 2001.

*******************************************************************************/

/* Includes ----------------------------------------------------------------- */

#include "gfx_tools.h" /* includes stgfx_init.h, ... */


/* Private Types ------------------------------------------------------------ */
typedef struct{
	S32 X;
	S32 Y;
} stgfx_Point_t;

typedef struct{
	stgfx_Point_t LeftEnd;
	stgfx_Point_t RightEnd;
} stgfx_Span_t;

typedef struct StackElement
{
	struct StackElement* Next_p;
	stgfx_Span_t  Span;
} stgfx_StackOfPoints_t;

typedef enum
{
  LEFTWARD,
  RIGHTWARD
} stgfx_Verse_t;

/* Private Constants -------------------------------------------------------- */


/* Private Variables (static)------------------------------------------------ */


/* Global Variables --------------------------------------------------------- */


/* Private Macros ----------------------------------------------------------- */


/* Private Function prototypes ---------------------------------------------- */
/*static BOOL AreColorsEqual(STGXOBJ_Color_t* ColA_p, STGXOBJ_Color_t* ColB_p);
  - moved to gfx_common.c as DrawRectangle now needs it.
  Could determine size just once rather than have a switch for every pixel */

static stgfx_StackOfPoints_t* Push(
  STGFX_Handle_t Handle, 
  stgfx_StackOfPoints_t*  Top_p,
  U32               leftx,
  U32               lefty,
  U32               rightx,
  U32 righty
);

static stgfx_StackOfPoints_t* Pop(
  STGFX_Handle_t          Handle,
  stgfx_StackOfPoints_t*  Top_p
);

static ST_ErrorCode_t BoundFindEnd(
  STGFX_Handle_t        Handle,
  STGXOBJ_Bitmap_t*     Bitmap_p,
  STGXOBJ_Color_t*      Color, 
  S32                   beginPointX,
  S32                   beginPointY,
  S32                   LeftOrRight,
  S32*                  BoundEnd_p
);

static BOOL IsInStack(
  stgfx_StackOfPoints_t*  Top_p,
  S32               XSeed,
  S32 YSeed
);

static void TrimColor(STGXOBJ_Color_t* Color_p);

/* Functions ---------------------------------------------------------------- */

/*******************************************************************************
Name        : STGFX_FloodFill()
Description : fills with a given color until reaches boundaries in
              CountourColor
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STGFX_FloodFill(
  STGFX_Handle_t            Handle,
  STGXOBJ_Bitmap_t*         Bitmap_p,
  const STGFX_GC_t*         GC_p,
  S32                       XSeed,
  S32                       YSeed,
  const STGXOBJ_Color_t*    ContourColor_p
)
{
  ST_ErrorCode_t         Err = ST_NO_ERROR;
  STBLIT_Handle_t        BlitHandle;
  STBLIT_BlitContext_t   BlitContext;
  STGXOBJ_Color_t        ContourColor, FillColor;
  STGXOBJ_Color_t        Color;
  stgfx_StackOfPoints_t* NewTop_p;
  stgfx_StackOfPoints_t* Top_p = NULL;
  stgfx_Point_t          seed;
  stgfx_Point_t          newSeed;
  stgfx_Span_t           newSpan;
  S32                    Spans = 0;
  S32                    tempx2;


  if (NULL == stgfx_HandleToUnit(Handle))
  {
    return(ST_ERROR_INVALID_HANDLE);
  }
  BlitHandle = stgfx_GetBlitterHandle(Handle);
  
  if((Bitmap_p == NULL) || (GC_p == NULL) || (ContourColor_p == NULL))
  {
    return(ST_ERROR_BAD_PARAMETER);
  }
  if(GC_p->DrawTexture_p != NULL  || GC_p->FillTexture_p != NULL) 
  {
    /* you can't FloodFill with a texture */
    return(ST_ERROR_BAD_PARAMETER);
  }

  /* border & fill colors must be of the same type of the bmp colortype,
     otherwise there can be not correnspondence and the algorithm would fail */
  if((Bitmap_p->ColorType != ContourColor_p->Type) ||
     (Bitmap_p->ColorType != GC_p->FillColor.Type))
  {
    return(ST_ERROR_BAD_PARAMETER);
  }

  /* if the color type of the bitmap has undersampled chromas
     the color of a pixel is not univocally determined */
  switch(Bitmap_p->ColorType)
  {
    case STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_422: /* fall through */
    case STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422:
    case STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_420:
    case STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_420:
      return(ST_ERROR_BAD_PARAMETER);
      break;
    default:
      break;
  }
  
  /* copy blit params to blitting context */
  Err = stgfx_InitBlitContext(Handle, &BlitContext, GC_p, Bitmap_p);
  if (Err != ST_NO_ERROR)
  {
    return Err;
  }
  
  /* no memory to free, so disable blit complete notification */
  BlitContext.NotifyBlitCompletion      = FALSE;

  if(XSeed <0 || YSeed <0 || XSeed>=(Bitmap_p->Width-1) ||
     YSeed>=(Bitmap_p->Height-1))
  {
    return(ST_ERROR_BAD_PARAMETER);
  }
  
  /* zero invalid bits of colours to ensure we operate and terminate
    correctly regardless of what the user puts there */
  ContourColor = *ContourColor_p;
  TrimColor(&ContourColor);
  FillColor = GC_p->FillColor;
  TrimColor(&FillColor);
  
  seed.X = XSeed;
  seed.Y = YSeed;
    
  Err = STBLIT_GetPixel(BlitHandle, Bitmap_p, XSeed, YSeed, &Color);
  if(Err != ST_NO_ERROR)
  {
    return(STGFX_ERROR_STBLIT_BLIT);
  }
  
  if(stgfx_AreColorsEqual(&Color, (STGXOBJ_Color_t*)&ContourColor))
  {
    return(ST_ERROR_BAD_PARAMETER);
  }
  
  newSpan.LeftEnd.Y = YSeed;
  Err = BoundFindEnd(Handle, Bitmap_p, (STGXOBJ_Color_t*)&ContourColor,
                     seed.X, seed.Y, LEFTWARD, &(newSpan.LeftEnd.X));
  if(Err != ST_NO_ERROR)
  {
    return(Err);
  }
  
  newSpan.RightEnd.Y = YSeed;
  Err = BoundFindEnd(Handle, Bitmap_p, (STGXOBJ_Color_t*)&ContourColor,
                     seed.X, seed.Y, RIGHTWARD, &(newSpan.RightEnd.X));
  if(Err != ST_NO_ERROR)
  {
    return(Err);
  }
  
  /* seed is pushed onto stack */
  Top_p = Push(Handle, Top_p,newSpan.LeftEnd.X, newSpan.LeftEnd.Y,
               newSpan.RightEnd.X, newSpan.RightEnd.Y );
  if(Top_p == NULL)
  {
    return(ST_ERROR_NO_MEMORY);
  }
  Spans++;

  while((Top_p != NULL) && (Spans > 0)) /* main loop */
  {
    seed.X = Top_p->Span.LeftEnd.X;
    seed.Y = Top_p->Span.LeftEnd.Y;
    Err = STBLIT_DrawHLine(BlitHandle,
                           Bitmap_p,
                           seed.X,
                           seed.Y,
                           Top_p->Span.RightEnd.X - seed.X + 1,
                           (STGXOBJ_Color_t*)&FillColor,
                           &BlitContext);
    if(Err != ST_NO_ERROR)
    {
      while(Top_p!= NULL)
      {
        Top_p = Pop(Handle, Top_p); 
      }
      return(STGFX_ERROR_STBLIT_BLIT);
    }
    
    tempx2 = Top_p->Span.RightEnd.X;
    Spans--;
    Top_p = Pop(Handle, Top_p); 

    /* look for new seeds in the line above */
    newSeed.X = seed.X;
    newSeed.Y = seed.Y - 1;
    if(newSeed.Y>=0)
    {
      while (newSeed.X < tempx2)
      {
        Err = STBLIT_GetPixel(BlitHandle, Bitmap_p, newSeed.X, newSeed.Y,
                              &Color);
        if(Err != ST_NO_ERROR)
        {
          while(Top_p!= NULL)
          {
            Top_p = Pop(Handle, Top_p); 
          }
          return(STGFX_ERROR_STBLIT_BLIT);
        }
      
        /* if the color is boundary or is already set go on */
        if(stgfx_AreColorsEqual(&Color, (STGXOBJ_Color_t*)&ContourColor) ||
           stgfx_AreColorsEqual(&Color, (STGXOBJ_Color_t*)&FillColor))
        {
          newSeed.X++;
        }
        else
        {
          Err = BoundFindEnd(Handle, Bitmap_p,(STGXOBJ_Color_t*)&ContourColor,
                             newSeed.X, newSeed.Y, LEFTWARD,
                             &(newSpan.LeftEnd.X));
          if(Err != ST_NO_ERROR)
          { 
            while(Top_p!= NULL)
            {
              Top_p = Pop(Handle, Top_p); 
            }
            return(STGFX_ERROR_STBLIT_BLIT);
          }
          Err = BoundFindEnd(Handle, Bitmap_p, (STGXOBJ_Color_t*)&ContourColor,
                             newSeed.X, newSeed.Y, RIGHTWARD,
                             &(newSpan.RightEnd.X));
          if(Err != ST_NO_ERROR)
          { 
            while(Top_p!= NULL)
            {
              Top_p = Pop(Handle, Top_p); 
            }
            return(Err);
          }
          if(IsInStack(Top_p, newSpan.LeftEnd.X, newSeed.Y) == FALSE)
          {
            newSpan.LeftEnd.Y = newSeed.Y;
            newSpan.RightEnd.Y = newSeed.Y;
            NewTop_p = Push(Handle, Top_p, newSpan.LeftEnd.X, newSpan.LeftEnd.Y,
                            newSpan.RightEnd.X, newSpan.RightEnd.Y);
            if(NewTop_p == NULL)
            {
              while(Top_p != NULL)
              {
                Top_p = Pop(Handle, Top_p); 
              }
              return(ST_ERROR_NO_MEMORY);
            }
            Top_p = NewTop_p;
            Spans++;    
          }
          newSeed.X = newSpan.RightEnd.X + 1;
        }
      }
    }
    
    /* look for new seeds in the line below */
    newSeed.X = seed.X;
    newSeed.Y = seed.Y + 1;
    if(newSeed.Y<Bitmap_p->Height)
    {
      while (newSeed.X < tempx2)
      {
        Err = STBLIT_GetPixel(BlitHandle, Bitmap_p, newSeed.X,
                              newSeed.Y, &Color);
        if(Err != ST_NO_ERROR)
        {
          while(Top_p!= NULL)
          {
            Top_p = Pop(Handle, Top_p); 
          }
          return(STGFX_ERROR_STBLIT_BLIT);
        }
        if(stgfx_AreColorsEqual(&Color, (STGXOBJ_Color_t*)&ContourColor) ||
           stgfx_AreColorsEqual(&Color, (STGXOBJ_Color_t*)&FillColor))
        {
          newSeed.X++;
        }
        else
        {
          Err = BoundFindEnd(Handle, Bitmap_p, (STGXOBJ_Color_t*)&ContourColor,
                             newSeed.X, newSeed.Y, LEFTWARD,
                             &(newSpan.LeftEnd.X));
          Err = BoundFindEnd(Handle, Bitmap_p, (STGXOBJ_Color_t*)&ContourColor,
                             newSeed.X, newSeed.Y, RIGHTWARD,
                             &(newSpan.RightEnd.X));
          if(IsInStack(Top_p, newSpan.LeftEnd.X, newSeed.Y) == FALSE)
          {
            newSpan.LeftEnd.Y = newSeed.Y;
            newSpan.RightEnd.Y = newSeed.Y;
            NewTop_p = Push(Handle, Top_p, newSpan.LeftEnd.X, newSpan.LeftEnd.Y,
                            newSpan.RightEnd.X, newSpan.RightEnd.Y);
            if(NewTop_p == NULL)
            {
              while(Top_p != NULL)
              {
                Top_p = Pop(Handle, Top_p); 
              }
              return(ST_ERROR_NO_MEMORY);
            }
            Top_p = NewTop_p;
            Spans++;
          }
          newSeed.X = newSpan.RightEnd.X +1;
        }
      }
    }
  } /* main loop */
  
  return(ST_NO_ERROR);
}


/*
Zero out the inapplicable bits of a colour value, so we don't get false
match failures when the caller fills such bits. Without this, FloodFill
could generate spans forever if it never matches FillColor
*/
static void TrimColor(STGXOBJ_Color_t* Color_p)
{
  switch(Color_p->Type)
  {
    case STGXOBJ_COLOR_TYPE_ARGB8565:
    case STGXOBJ_COLOR_TYPE_ARGB8565_255:
      Color_p->Value.ARGB8565.R &= 0x1f;
      Color_p->Value.ARGB8565.G &= 0x3f;
      Color_p->Value.ARGB8565.B &= 0x1f;
      break;
      
    case STGXOBJ_COLOR_TYPE_ARGB1555:
      Color_p->Value.ARGB1555.Alpha &= 0x01;
      Color_p->Value.ARGB1555.R &= 0x1f;
      Color_p->Value.ARGB1555.G &= 0x1f;
      Color_p->Value.ARGB1555.B &= 0x1f;
      break;

    case STGXOBJ_COLOR_TYPE_ARGB4444:
      Color_p->Value.ARGB4444.Alpha &= 0x0f;
      Color_p->Value.ARGB4444.R &= 0x0f;
      Color_p->Value.ARGB4444.G &= 0x0f;
      Color_p->Value.ARGB4444.B &= 0x0f;
      break;
    
    case STGXOBJ_COLOR_TYPE_RGB565:
      Color_p->Value.RGB565.R &= 0x1f;
      Color_p->Value.RGB565.G &= 0x3f;
      Color_p->Value.RGB565.B &= 0x1f;
      break;
    
    case STGXOBJ_COLOR_TYPE_ACLUT44:
      Color_p->Value.ACLUT44.Alpha &= 0x0f;
      Color_p->Value.ACLUT44.PaletteEntry &= 0x0f;
      break; 
    
    case STGXOBJ_COLOR_TYPE_CLUT4:
      Color_p->Value.CLUT4 &= 0x0f;
      break; 

    case STGXOBJ_COLOR_TYPE_CLUT2:
      Color_p->Value.CLUT4 &= 0x03;
      break; 

    case STGXOBJ_COLOR_TYPE_CLUT1:
      Color_p->Value.CLUT4 &= 0x01;
      break; 

    case STGXOBJ_COLOR_TYPE_ALPHA1:
      Color_p->Value.CLUT4 &= 0x01;
      break; 

    case STGXOBJ_COLOR_TYPE_ALPHA4:
      Color_p->Value.CLUT4 &= 0x0f;
      break; 

    case STGXOBJ_COLOR_TYPE_UNSIGNED_AYCBCR6888_444:
      Color_p->Value.UnsignedAYCbCr6888_444.Alpha &= 0x3f;
      break;
      
    default:
      /* All bits used (or invalid colour type, but that's checked by FloodFill) */
      break;
  }
}

/*
The Push function add a new element on the top of the stack
Returns NULL for failed memory allocation
*/
static stgfx_StackOfPoints_t* Push(
  STGFX_Handle_t Handle, 
  stgfx_StackOfPoints_t* Top_p,
  U32 leftx,
  U32 lefty,
  U32 rightx,
  U32 righty
)
{
  stgfx_StackOfPoints_t* Stack_p;
	
  Stack_p = stgfx_DEVMEM_malloc(Handle, sizeof(stgfx_StackOfPoints_t));

  /* Other than the height of the bitmap (I think), which is only known at run-time,
    there is no upper limit on the depth the stack can grow to, and thus even with a
    pre-allocated device partition, no guarantee a memory allocation won't fail */
    
  /* Alternative: do one allocation of an array of stack elements once max size is known */
  
  if(Stack_p != NULL)
  {
    Stack_p->Next_p = Top_p;
    Stack_p->Span.LeftEnd.X = leftx;
    Stack_p->Span.LeftEnd.Y = lefty;
    Stack_p->Span.RightEnd.X = rightx;
    Stack_p->Span.RightEnd.Y = righty;
  }
  return Stack_p;
}

/*
The Pop function take the top element of the
stack (the last pushed, which is the Top_p)
making the previous one to point to NULL
*/
static stgfx_StackOfPoints_t* Pop(
  STGFX_Handle_t          Handle,
  stgfx_StackOfPoints_t*  Top_p
)
{
  stgfx_StackOfPoints_t* temp;
  
  temp = Top_p->Next_p;
  if(Top_p != NULL)
  {
    stgfx_DEVMEM_free(Handle, Top_p);
  }
  return temp;
}


/* if the xylArray already has the seed, it returns 'true' else it
   returns 'false' */
static BOOL IsInStack(stgfx_StackOfPoints_t* Top_p, S32 XSeed, S32 YSeed)
{
  stgfx_StackOfPoints_t* current;
	
  current = Top_p;
  while(current != NULL)
  {
    if((current->Span.LeftEnd.X == XSeed) &&
       (current->Span.LeftEnd.Y == YSeed))
    {
      return TRUE;			
    }
    else
    {
      current = current->Next_p;
    }
  }
  return FALSE;
}

/* 
The BoundFindEnd function finds the left or the right endpoint of
a span in case of boundary filling
*/
static ST_ErrorCode_t BoundFindEnd(
  STGFX_Handle_t    Handle,
  STGXOBJ_Bitmap_t* Bitmap_p,
  STGXOBJ_Color_t*  BoundaryColor, 
  S32               beginPointX,
  S32               beginPointY,
  S32               LeftOrRight,
  S32*              BoundEnd_p
)
{
  ST_ErrorCode_t Err = ST_NO_ERROR;
  STGXOBJ_Color_t NewColor;
  S32 XIncrement;
  S32 X;
  BOOL Stay;
  STBLIT_Handle_t BlitHandle;
	
  if(LeftOrRight == 0)
  {
    XIncrement = -1;
  }
  else if(LeftOrRight == 1)
  {
    XIncrement = 1;
  }
  else 
  {
    return(ST_ERROR_BAD_PARAMETER);
  }
  X = beginPointX + XIncrement;
	
  if(X>=(Bitmap_p->Width) || X<0)
  {
    *BoundEnd_p = (X - XIncrement);
    return(ST_NO_ERROR);
  }
  
  BlitHandle = stgfx_GetBlitterHandle(Handle);
  Stay = TRUE;
  do
  {
    Err = STBLIT_GetPixel(BlitHandle, Bitmap_p, X, beginPointY, &NewColor);
    if(Err != ST_NO_ERROR)
    {
      return(STGFX_ERROR_STBLIT_BLIT);
    }
    if(!stgfx_AreColorsEqual(&NewColor, BoundaryColor))
    {
      X += XIncrement;
    }
    else
      Stay = FALSE;
    if(X>=(Bitmap_p->Width) || X<0)
    {
      *BoundEnd_p = (X - XIncrement);
      return(ST_NO_ERROR);
    }
  } while(Stay);
	
  *BoundEnd_p = (X - XIncrement);
  return(ST_NO_ERROR);
}

/* End of gfx_seedfill.c */
