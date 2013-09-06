/*******************************************************************************

File name   : gfx_copy.c

Description : STGFX copy area source file

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
2000-11-09         Created                                          AM
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */


/* Includes ----------------------------------------------------------------- */

#include "gfx_tools.h" /* includes stgfx_init.h, ... */

/* Private Types ------------------------------------------------------------ */


/* Private Constants -------------------------------------------------------- */


/* Private Variables (static)------------------------------------------------ */


/* Global Variables --------------------------------------------------------- */


/* Private Macros ----------------------------------------------------------- */


/* Private Function prototypes ---------------------------------------------- */


/* Functions ---------------------------------------------------------------- */


/*******************************************************************************
Name        : STGFX_CopyArea
Description : 1:1 Copy within a bitmap, or from one bitmap to another
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t EIS_STGFX_CopyArea(
  STGFX_Handle_t                    Handle,
  const STGXOBJ_Bitmap_t*           SrcBitmap_p,
  STGXOBJ_Bitmap_t*                 DstBitmap_p,
  const STGFX_GC_t*                 GC_p,
  const STGXOBJ_Rectangle_t*        SrcRectangle_p,
  const STGXOBJ_Rectangle_t*        DstRectangle_p,
  STBLIT_BlitContext_t*       BlitContext_p
)
{
	ST_ErrorCode_t        Err;
	STBLIT_BlitContext_t  BlitContext;
	STBLIT_Handle_t       BlitHandle;
	char  Area_Room=1;
  /* check those parameters stgfx_InitBlitContext won't */
  if((SrcBitmap_p == NULL) || (SrcRectangle_p == NULL))
  {
    return(ST_ERROR_BAD_PARAMETER);
  }
  if (NULL == stgfx_HandleToUnit(Handle))
  {
    return(ST_ERROR_INVALID_HANDLE);
  }
  
  BlitHandle = stgfx_GetBlitterHandle(Handle);

  /* copy pertinent Graphic context params to the blitting context */
  Err = stgfx_InitBlitContext(BlitHandle, &BlitContext, GC_p, DstBitmap_p);
  if (Err != ST_NO_ERROR)
  {
      return(Err);
  }
	if(  (SrcRectangle_p->PositionX== DstRectangle_p->PositionX)&&
 (SrcRectangle_p->PositionY== DstRectangle_p->PositionY)&&
 ( SrcRectangle_p->Width==DstRectangle_p->Width)&&
  (SrcRectangle_p->Height==DstRectangle_p->Height)
)
	{
	Area_Room=0;
	}
	#if 0
	//memcpy(&(BlitContext.ColorKey),&(BlitContext_p->ColorKey),sizeof(STGXOBJ_ColorKey_t));
		BlitContext.ColorKeyCopyMode 			= BlitContext_p->ColorKeyCopyMode;
		BlitContext.ColorKey.Type 				= BlitContext_p->ColorKey.Type;
		BlitContext.ColorKey.Value.RGB888.RMin 	=BlitContext_p->ColorKey.Value.RGB888.RMin ;
		BlitContext.ColorKey.Value.RGB888.RMax 	= BlitContext_p->ColorKey.Value.RGB888.RMax ;
		BlitContext.ColorKey.Value.RGB888.ROut 	=BlitContext_p->ColorKey.Value.RGB888.ROut 	;
		BlitContext.ColorKey.Value.RGB888.REnable 	= BlitContext_p->ColorKey.Value.RGB888.REnable ;
		BlitContext.ColorKey.Value.RGB888.GMin 	=BlitContext_p->ColorKey.Value.RGB888.GMin 	;
		BlitContext.ColorKey.Value.RGB888.GMax 	= BlitContext_p->ColorKey.Value.RGB888.GMax 	;
		BlitContext.ColorKey.Value.RGB888.GOut 	=BlitContext_p->ColorKey.Value.RGB888.GOut 	;
		BlitContext.ColorKey.Value.RGB888.GEnable 	=BlitContext_p->ColorKey.Value.RGB888.GEnable ;
		BlitContext.ColorKey.Value.RGB888.BMin 	= BlitContext_p->ColorKey.Value.RGB888.BMin ;
		BlitContext.ColorKey.Value.RGB888.BMax 	=BlitContext_p->ColorKey.Value.RGB888.BMax ;
		BlitContext.ColorKey.Value.RGB888.BOut 	= BlitContext_p->ColorKey.Value.RGB888.BOut 	;
		BlitContext.ColorKey.Value.RGB888.BEnable 	= BlitContext_p->ColorKey.Value.RGB888.BEnable ;
	#endif
	BlitContext.GlobalAlpha=BlitContext_p->GlobalAlpha;
	BlitContext.AluMode=BlitContext_p->AluMode;
  /* STBLIT_CopyRectangl is quicker (I think),
    but does not support colour keying */
  if ((0==Area_Room) &&(GC_p->ColorKeyCopyMode == STBLIT_COLOR_KEY_MODE_NONE) &&
      (GC_p->AluMode == STBLIT_ALU_COPY) &&
      (SrcBitmap_p->ColorType == DstBitmap_p->ColorType))
  {
    Err = STBLIT_CopyRectangle(BlitHandle,
                               (STGXOBJ_Bitmap_t*)SrcBitmap_p,
                               (STGXOBJ_Rectangle_t*)SrcRectangle_p,
                               DstBitmap_p,
                               SrcRectangle_p->PositionX,
                               SrcRectangle_p->PositionY,
                               &BlitContext);
  }
  else
  {
    STBLIT_Source_t        Source;
    STBLIT_Destination_t   Destination;

    /* source & dest preparation */
    Source.Type          = STBLIT_SOURCE_TYPE_BITMAP;
    Source.Data.Bitmap_p = (STGXOBJ_Bitmap_t*)SrcBitmap_p;
    Source.Rectangle     = *SrcRectangle_p;
    Source.Palette_p     = NULL;

    Destination.Bitmap_p            = DstBitmap_p;
    Destination.Palette_p           = NULL;
    Destination.Rectangle = *DstRectangle_p;

    Err = STBLIT_Blit(BlitHandle, &Source, NULL, &Destination, &BlitContext);
  }
  
  if(Err != ST_NO_ERROR)
  {
    Err = STGFX_ERROR_STBLIT_BLIT;
  }
  
  return(Err);
}

/* Private Types ------------------------------------------------------------ */


/* Private Constants -------------------------------------------------------- */


/* Private Variables (static)------------------------------------------------ */


/* Global Variables --------------------------------------------------------- */


/* Private Macros ----------------------------------------------------------- */


/* Private Function prototypes ---------------------------------------------- */


/* Functions ---------------------------------------------------------------- */


/*******************************************************************************
Name        : STGFX_CopyArea
Description : 1:1 Copy within a bitmap, or from one bitmap to another
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STGFX_CopyArea(
  STGFX_Handle_t                    Handle,
  const STGXOBJ_Bitmap_t*           SrcBitmap_p,
  STGXOBJ_Bitmap_t*                 DstBitmap_p,
  const STGFX_GC_t*                 GC_p,
  const STGXOBJ_Rectangle_t*        SrcRectangle_p,
  S32                               DstPositionX,
  S32                               DstPositionY
)
{
	ST_ErrorCode_t        Err;
	STBLIT_BlitContext_t  BlitContext;
	STBLIT_Handle_t       BlitHandle;

  /* check those parameters stgfx_InitBlitContext won't */
  if((SrcBitmap_p == NULL) || (SrcRectangle_p == NULL))
  {
    return(ST_ERROR_BAD_PARAMETER);
  }
  if (NULL == stgfx_HandleToUnit(Handle))
  {
    return(ST_ERROR_INVALID_HANDLE);
  }
  
  BlitHandle = stgfx_GetBlitterHandle(Handle);

  /* copy pertinent Graphic context params to the blitting context */
  Err = stgfx_InitBlitContext(BlitHandle, &BlitContext, GC_p, DstBitmap_p);
  if (Err != ST_NO_ERROR)
  {
      return(Err);
  }
#if 0
  /* nothing to free, so turn of completion notification */
  BlitContext.NotifyBlitCompletion = true;
  
  BlitContext.EnableFlickerFilter=FALSE/*zxg change true to false*/;/*yxl 2007-06-07 add this line*/

#endif


  



  /* STBLIT_CopyRectangl is quicker (I think),
    but does not support colour keying */
  if ((GC_p->ColorKeyCopyMode == STBLIT_COLOR_KEY_MODE_NONE) &&
      (GC_p->AluMode == STBLIT_ALU_COPY) &&
      (SrcBitmap_p->ColorType == DstBitmap_p->ColorType))
  {
    Err = STBLIT_CopyRectangle(BlitHandle,
                               (STGXOBJ_Bitmap_t*)SrcBitmap_p,
                               (STGXOBJ_Rectangle_t*)SrcRectangle_p,
                               DstBitmap_p,
                               DstPositionX,
                               DstPositionY,
                               &BlitContext);
  }
  else
  {
    STBLIT_Source_t        Source;
    STBLIT_Destination_t   Destination;

    /* source & dest preparation */
    Source.Type          = STBLIT_SOURCE_TYPE_BITMAP;
    Source.Data.Bitmap_p = (STGXOBJ_Bitmap_t*)SrcBitmap_p;
    Source.Rectangle     = *SrcRectangle_p;
    Source.Palette_p     = NULL;

    Destination.Bitmap_p            = DstBitmap_p;
    Destination.Palette_p           = NULL;
    Destination.Rectangle.PositionX = DstPositionX;
    Destination.Rectangle.PositionY = DstPositionY;
    Destination.Rectangle.Width     = SrcRectangle_p->Width;
    Destination.Rectangle.Height    = SrcRectangle_p->Height;

    Err = STBLIT_Blit(BlitHandle, &Source, NULL, &Destination, &BlitContext);
  }
  
  if(Err != ST_NO_ERROR)
  {
    Err = STGFX_ERROR_STBLIT_BLIT;
  }
  
  return(Err);
}


#if 1 /*yxl 2005-10-24 add below function,yxl 2007-06-18 temp modify below section*/

/*******************************************************************************
Name        : STGFX_Blit
Description : 
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STGFX_CopyAreaA(
  STGFX_Handle_t                    Handle,
  const STGXOBJ_Bitmap_t*           SrcBitmap_p,
  STGXOBJ_Bitmap_t*                 DstBitmap_p,
  const STGFX_GC_t*                 GC_p,
  const STGXOBJ_Rectangle_t*        SrcRectangle_p,
  const STGXOBJ_Rectangle_t*        DstRectangle_p
)
{
	ST_ErrorCode_t        Err;
	STBLIT_BlitContext_t  BlitContext;
	STBLIT_Handle_t       BlitHandle;
	
	/* check those parameters stgfx_InitBlitContext won't */
	if((SrcBitmap_p == NULL) || (SrcRectangle_p == NULL)||(DstRectangle_p == NULL))
	{
		return(ST_ERROR_BAD_PARAMETER);
	}
	if (NULL == stgfx_HandleToUnit(Handle))
	{
		return(ST_ERROR_INVALID_HANDLE);
	}
	
	BlitHandle = stgfx_GetBlitterHandle(Handle);
	
	/* copy pertinent Graphic context params to the blitting context */
	Err = stgfx_InitBlitContext(BlitHandle, &BlitContext, GC_p, DstBitmap_p);
	if (Err != ST_NO_ERROR)
	{
		return(Err);
	}
#if 0
	
	/* nothing to free, so turn of completion notification */
	BlitContext.NotifyBlitCompletion = true;
#ifdef ST_7710 /*yxl 2007-06-02 add this line*/	
	BlitContext.EnableFlickerFilter=TRUE;/*yxl 2006-03-23 add this line*/
#endif 
#if 0/*yxl 2007-06-07 modify below section*/
	BlitContext.EnableFlickerFilter=FALSE;/*yxl 2007-06-02 add this line*/
#else
	BlitContext.EnableFlickerFilter=FALSE/*zxg change true to false*/;

#endif/*end yxl 2007-06-07 modify below section*/

  memset((void *)&BlitContext,0,sizeof(STBLIT_BlitContext_t));
  BlitContext.Trigger.EnableTrigger   = FALSE;
  BlitContext.JobHandle               = STBLIT_NO_JOB_HANDLE;
  BlitContext.WriteInsideClipRectangle= FALSE;
  BlitContext.EnableClipRectangle     = FALSE;
  BlitContext.NotifyBlitCompletion    = TRUE; /*FALSE;	*/
  BlitContext.AluMode                 = STBLIT_ALU_COPY;
  BlitContext.EnableMaskWord          = FALSE;
  BlitContext.MaskWord                = 0xFFFFFFFF;
  BlitContext.GlobalAlpha             = 128;
#endif

	/* STBLIT_CopyRectangl is quicker (I think),
    but does not support colour keying */
#if 0
	if ((GC_p->ColorKeyCopyMode == STBLIT_COLOR_KEY_MODE_NONE) &&
		(GC_p->AluMode == STBLIT_ALU_COPY) &&
		(SrcBitmap_p->ColorType == DstBitmap_p->ColorType))
	{
		Err = STBLIT_CopyRectangle(BlitHandle,
			(STGXOBJ_Bitmap_t*)SrcBitmap_p,
			(STGXOBJ_Rectangle_t*)SrcRectangle_p,
			DstBitmap_p,
			DstRectangle_p->PositionX,
			DstRectangle_p->PositionY,
			&BlitContext);
	}
	else
#endif
	{
		STBLIT_Source_t        Source;
		STBLIT_Destination_t   Destination;
		
		/* source & dest preparation */
		Source.Type          = STBLIT_SOURCE_TYPE_BITMAP;
		Source.Data.Bitmap_p = (STGXOBJ_Bitmap_t*)SrcBitmap_p;
		Source.Rectangle     = *SrcRectangle_p;
		Source.Palette_p     = NULL;
		
		Destination.Bitmap_p            = DstBitmap_p;
		Destination.Palette_p           = NULL;
		Destination.Rectangle.PositionX = 0;
		Destination.Rectangle.PositionY = 0;
		Destination.Rectangle.Width     = DstRectangle_p->Width;
		Destination.Rectangle.Height    = DstRectangle_p->Height;
		
		Err = STBLIT_Blit(BlitHandle, NULL,&Source, &Destination, &BlitContext);
	}
	
	if(Err != ST_NO_ERROR)
	{
		Err = STGFX_ERROR_STBLIT_BLIT;
	}
	
	return(Err);
}

#else /*yxl 2007-06-18 temp modify below section*/


ST_ErrorCode_t STGFX_CopyAreaA(
  STGFX_Handle_t                    Handle,
  const STGXOBJ_Bitmap_t*           SrcBitmap_p,
  STGXOBJ_Bitmap_t*                 DstBitmap_p,
  const STGFX_GC_t*                 GC_p,
  const STGXOBJ_Rectangle_t*        SrcRectangle_p,
  const STGXOBJ_Rectangle_t*        DstRectangle_p
)
{
	ST_ErrorCode_t        Err;
	STBLIT_BlitContext_t  BlitContext;
	STBLIT_Handle_t       BlitHandle;
	
	/* check those parameters stgfx_InitBlitContext won't */
	if((SrcBitmap_p == NULL) || (SrcRectangle_p == NULL)||(DstRectangle_p == NULL))
	{
		return(ST_ERROR_BAD_PARAMETER);
	}
	if (NULL == stgfx_HandleToUnit(Handle))
	{
		return(ST_ERROR_INVALID_HANDLE);
	}
	
	BlitHandle = stgfx_GetBlitterHandle(Handle);
	
  	memset((void *)&BlitContext,0,sizeof(STBLIT_BlitContext_t));

	BlitContext.ColorKeyCopyMode        = STBLIT_COLOR_KEY_MODE_NONE;
	BlitContext.AluMode                 = STBLIT_ALU_COPY;
	BlitContext.EnableMaskWord          = FALSE;
	BlitContext.EnableMaskBitmap        = FALSE;
	BlitContext.EnableColorCorrection   = FALSE;
	BlitContext.Trigger.EnableTrigger   = FALSE;
	BlitContext.GlobalAlpha             = 50;
	BlitContext.EnableClipRectangle     = FALSE;
	BlitContext.EnableFlickerFilter     = FALSE;
	BlitContext.JobHandle               = STBLIT_NO_JOB_HANDLE;
	BlitContext.NotifyBlitCompletion    = TRUE;

{
		STBLIT_Source_t        Source;
		STBLIT_Destination_t   Destination;
		
		/* source & dest preparation */
		Source.Type          = STBLIT_SOURCE_TYPE_BITMAP;
		Source.Data.Bitmap_p = (STGXOBJ_Bitmap_t*)SrcBitmap_p;
		Source.Rectangle     = *SrcRectangle_p;
		Source.Palette_p     = NULL;
		
		Destination.Bitmap_p            = DstBitmap_p;
		Destination.Palette_p           = NULL;
		Destination.Rectangle.PositionX = 0;
		Destination.Rectangle.PositionY = 0;
		Destination.Rectangle.Width     = DstRectangle_p->Width;
		Destination.Rectangle.Height    = DstRectangle_p->Height;
		
		Err = STBLIT_Blit(BlitHandle, NULL,&Source, &Destination, &BlitContext);
	}
	
	if(Err != ST_NO_ERROR)
	{
		Err = STGFX_ERROR_STBLIT_BLIT;
	}
	
	return(Err);
}

#endif/*end yxl 2005-10-24 add below function,,yxl 2007-06-18 temp modify below section*/


/*
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
*/


/* End of gfx_copy.c */
