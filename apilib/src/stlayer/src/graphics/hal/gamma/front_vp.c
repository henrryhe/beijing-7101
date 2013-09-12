/*******************************************************************************

File name   : front_vp.c

Description : API functions

COPYRIGHT (C) STMicroelectronics 2004

Date               Modification                               Name
----               ------------                               ----
2000-05-30         Created                                    Michel Bruant


    ViewPort functions
    ------------------
    LAYERGFX_OpenViewPort()
    LAYERGFX_CloseViewPort()
    LAYERGFX_EnableViewPort()
    LAYERGFX_DisableViewPort()
    LAYERGFX_SetViewPortSource()
    LAYERGFX_GetViewPortSource()
    LAYERGFX_SetViewPortIORectangle()
    LAYERGFX_AdjustIORectangle()
    LAYERGFX_GetViewPortIORectangle()
    LAYERGFX_SetViewPortPosition()
    LAYERGFX_GetViewPortPosition()
    LAYERGFX_SetViewPortPSI()
    LAYERGFX_DisableColorKey()
    LAYERGFX_EnableColorKey()
    LAYERGFX_SetViewPortColorKey()
    LAYERGFX_GetViewPortColorKey()
    LAYERGFX_DisableBorderAlpha()
    LAYERGFX_EnableBorderAlpha()
    LAYERGFX_SetViewPortAlpha()
    LAYERGFX_GetViewPortAlpha()
    LAYERGFX_SetViewPortGain()
    LAYERGFX_GetViewPortGain()
    LAYERGFX_AttachViewPort()
    LAYERGFX_GetViewPortRecordable()
    LAYERGFX_SetViewPortRecordable()

******************************************************************************/

/* Includes ----------------------------------------------------------------- */

#include "stlayer.h"
#include "hard_mng.h"
#include "frontend.h"
#include "desc_mng.h"
#include "stsys.h"
#if !defined (ST_OSLINUX)
    #include <string.h>
#endif

/* Functions ---------------------------------------------------------------- */

static ST_ErrorCode_t stlayer_TestViewPortParam(STLAYER_Handle_t LayerHandle,
                                        STGXOBJ_Rectangle_t * InputRect_p,
                                        STGXOBJ_Rectangle_t * OutputRect_p,
                                        STGXOBJ_Rectangle_t * ClippedIn_p,
                                        STGXOBJ_Rectangle_t * ClippedOut_p,
                                        BOOL                * TotalClipped,
                                        STGXOBJ_Bitmap_t    * Bitmap_p);

static ST_ErrorCode_t LAYERGFX_Show(STLAYER_ViewPortHandle_t VPHandle);
static ST_ErrorCode_t LAYERGFX_Hide(STLAYER_ViewPortHandle_t VPHandle);

/*******************************************************************************
Name        : LAYERGFX_OpenViewPort
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYERGFX_OpenViewPort(STLAYER_Handle_t           LayerHandle,
                                    STLAYER_ViewPortParams_t * Params,
                                    STLAYER_ViewPortHandle_t * VPHandle)
{
  ST_ErrorCode_t                Error;
  stlayer_ViewPortHandle_t *    VPHandleCasted;


    #if defined (ST_7109)
        U32 CutId;
    #endif


  /* check params */
  if ((Params == 0) || (VPHandle == 0))
  {
      layergfx_Defense(ST_ERROR_BAD_PARAMETER);
      return(ST_ERROR_BAD_PARAMETER);
  }
  /* default value */
  *VPHandle = 0;

  /* error checking: is LayerHandle valid? */
  if(! ((stlayer_context.XContext[LayerHandle].Initialised)
       &&(stlayer_context.XContext[LayerHandle].Open)))
  {
      layergfx_Defense(ST_ERROR_INVALID_HANDLE);
      return(ST_ERROR_INVALID_HANDLE);
  }
  VPHandleCasted = (stlayer_ViewPortHandle_t*)(VPHandle);
  STOS_SemaphoreWait(stlayer_context.XContext[LayerHandle].AccessProtect_p);
  Error = stlayer_GetViewPortHandle(VPHandleCasted, LayerHandle);
  if (Error)
  {
        *VPHandle = 0;
        STOS_SemaphoreSignal(stlayer_context.XContext[LayerHandle].AccessProtect_p);
        layergfx_Defense(STLAYER_ERROR_NO_FREE_HANDLES);
        return(STLAYER_ERROR_NO_FREE_HANDLES);
  }

   #if defined (ST_7109)
    CutId = ST_GetCutRevision();
   #endif

   /* Test Viewport params and clipping */
  Error = stlayer_TestViewPortParam(LayerHandle,
                                       &(Params->InputRectangle),
                                       &(Params->OutputRectangle),
                                       &(*VPHandleCasted)->ClippedIn,
                                       &(*VPHandleCasted)->ClippedOut,
                                       &(*VPHandleCasted)->TotalClipped,
                                       Params->Source_p->Data.BitMap_p );
  if (Error)
  {
        *VPHandle = 0;
        STOS_SemaphoreSignal(stlayer_context.XContext[LayerHandle].AccessProtect_p);
        stlayer_FreeViewPortHandle(*VPHandleCasted, LayerHandle);
        layergfx_Defense(ST_ERROR_BAD_PARAMETER);
        return(ST_ERROR_BAD_PARAMETER);
  }
  Error = stlayer_FillViewPort(LayerHandle,Params,* VPHandleCasted);
  if (Error)
  {
        *VPHandle = 0;
        STOS_SemaphoreSignal(stlayer_context.XContext[LayerHandle].AccessProtect_p);
        stlayer_FreeViewPortHandle(*VPHandleCasted, LayerHandle);
        layergfx_Defense(ST_ERROR_BAD_PARAMETER);
        return(ST_ERROR_BAD_PARAMETER);
  }

 #if defined (ST_7109) || defined (ST_7710) || defined (ST_7100)

      /* Test Vertical Resize and Horizontal Downsize */

  #if defined (ST_7109)

  if (CutId < 0xC0)
    {
  #endif


    if(Params->OutputRectangle.Height != Params->InputRectangle.Height)
    {
        STOS_SemaphoreSignal(stlayer_context.XContext[LayerHandle].AccessProtect_p);
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    if(Params->OutputRectangle.Width < Params->InputRectangle.Width)
    {
        STOS_SemaphoreSignal(stlayer_context.XContext[LayerHandle].AccessProtect_p);

        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

  #if defined (ST_7109)

   }
   else
   {
        if((((stlayer_ViewPortDescriptor_t*)VPHandle)->FFilterEnabled == TRUE) && (Params->OutputRectangle.Height != Params->InputRectangle.Height))
        {
           STOS_SemaphoreSignal(stlayer_context.XContext[LayerHandle].AccessProtect_p);
           return(ST_ERROR_FEATURE_NOT_SUPPORTED);
         }

   }

  #endif


 #endif


 #if defined (ST_7200)

 {
	if((((stlayer_ViewPortDescriptor_t*)VPHandle)->FFilterEnabled == TRUE) && (Params->OutputRectangle.Height != Params->InputRectangle.Height))
        	{
           STOS_SemaphoreSignal(stlayer_context.XContext[LayerHandle].AccessProtect_p);
		   return(ST_ERROR_FEATURE_NOT_SUPPORTED);
         	}
 }
 #endif



  layergfx_Trace("STLAYER - GFX : Viewport request");
  STOS_SemaphoreSignal(stlayer_context.XContext[LayerHandle].AccessProtect_p);
  return(ST_NO_ERROR);
}


/*******************************************************************************
Name        : LAYERGFX_CloseViewPort
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYERGFX_CloseViewPort(STLAYER_ViewPortHandle_t VPHandle)
{
    ST_ErrorCode_t              Error;
    stlayer_ViewPortHandle_t    VPHandleCasted;
    STLAYER_Handle_t            LayerHandle;

    /* error checking: is the VP handle valid?*/
    if(VPHandle == 0)
    {
        layergfx_Defense(ST_ERROR_BAD_PARAMETER);
        return(ST_ERROR_INVALID_HANDLE);
    }
    VPHandleCasted = (stlayer_ViewPortHandle_t)(VPHandle);
    LayerHandle = ((stlayer_ViewPortDescriptor_t*)VPHandle)->LayerHandle;

    /* if VP is enabled, disable it before free */
    if(((stlayer_ViewPortDescriptor_t*)VPHandle)->Enabled)
    {
        LAYERGFX_DisableViewPort(VPHandle);
    }

    STOS_SemaphoreWait(stlayer_context.XContext[LayerHandle].AccessProtect_p);
    Error = stlayer_FreeViewPortHandle(VPHandleCasted, LayerHandle);
    STOS_SemaphoreSignal(stlayer_context.XContext[LayerHandle].AccessProtect_p);
    layergfx_Trace("STLAYER - GFX : Viewport release");
    return(Error);
}
/*******************************************************************************
Name        : LAYERGFX_Show
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYERGFX_Show(STLAYER_ViewPortHandle_t VPHandle)
{
    ST_ErrorCode_t error;
    stlayer_ViewPortHandle_t    VPHandleCasted;
    STLAYER_Handle_t            LayerHandle;

    VPHandleCasted = (stlayer_ViewPortHandle_t)(VPHandle);
    LayerHandle = ((stlayer_ViewPortDescriptor_t*)VPHandle)->LayerHandle;
    if (stlayer_context.XContext[LayerHandle].LayerType != STLAYER_GAMMA_CURSOR)
    {
        /* Graphic Layer case */
        error = stlayer_CheckVPDescriptor(VPHandleCasted);
        if( error == DESC_MANAGER_OK)
        {
            /* link the VP to the VP linked list */
            stlayer_InsertVPDescriptor(VPHandleCasted);
            ((stlayer_ViewPortDescriptor_t *)VPHandle)->Enabled = TRUE;
            stlayer_UpdateNodeGeneric(VPHandleCasted);
            /* link the nodes to the nodes linked list = show the VP */
            stlayer_InsertNodeLayerType(VPHandleCasted);
        }
        else
        {
            layergfx_Defense(STLAYER_ERROR_OVERLAP_VIEWPORT);
            return (STLAYER_ERROR_OVERLAP_VIEWPORT);
        }
    }
    else
    {
        /* Cursor Layer case */
        error = stlayer_EnableCursor(VPHandleCasted);
    }
    return(error);
}
/*******************************************************************************
Name        : LAYERGFX_Hide
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYERGFX_Hide(STLAYER_ViewPortHandle_t VPHandle)
{
    stlayer_ViewPortHandle_t    VPHandleCasted;
    STLAYER_Handle_t            LayerHandle;

    VPHandleCasted = (stlayer_ViewPortHandle_t)(VPHandle);
    LayerHandle = ((stlayer_ViewPortDescriptor_t*)VPHandle)->LayerHandle;
    if (stlayer_context.XContext[LayerHandle].LayerType!= STLAYER_GAMMA_CURSOR)
    {
        /* Graphic Layer case */
        /* unlink the nodes */
        stlayer_ExtractNodeLayerType(VPHandleCasted);
        stlayer_ExtractVPDescriptor(VPHandleCasted);
    }
    else
    {
        /* Cursor Layer case */
        stlayer_DisableCursor(VPHandleCasted);
    }
    return(ST_NO_ERROR);
}
/*******************************************************************************
Name        : LAYERGFX_EnableViewPort
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYERGFX_EnableViewPort(STLAYER_ViewPortHandle_t VPHandle)
{
    U32            LayerHandle;
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STGXOBJ_Bitmap_t    * Bitmap_p;
    STGXOBJ_Rectangle_t   * InputRectangle;
    STGXOBJ_Rectangle_t   * OutputRectangle;
    STGXOBJ_Rectangle_t   ClippedIn;
    STGXOBJ_Rectangle_t   ClippedOut;
    BOOL                    TotalClipped;

    /* error checking: is the VP handle valid?*/
    if(VPHandle == 0)
    {
        layergfx_Defense(ST_ERROR_INVALID_HANDLE);
        return(ST_ERROR_INVALID_HANDLE);
    }
    if(((stlayer_ViewPortDescriptor_t *)VPHandle)->Enabled)
    {
        return(ST_NO_ERROR);
    }
    LayerHandle = ((stlayer_ViewPortDescriptor_t*)VPHandle)->LayerHandle;
    Bitmap_p    = &(((stlayer_ViewPortDescriptor_t*)VPHandle)->Bitmap);
    InputRectangle=&(((stlayer_ViewPortDescriptor_t*)VPHandle)->InputRect);
    OutputRectangle=&(((stlayer_ViewPortDescriptor_t*)VPHandle)->OutputRect);
    STOS_memcpy(&ClippedIn, &(((stlayer_ViewPortDescriptor_t*)VPHandle)->ClippedIn), sizeof(STGXOBJ_Rectangle_t));
    STOS_memcpy(&ClippedOut, &(((stlayer_ViewPortDescriptor_t*)VPHandle)->ClippedOut), sizeof(STGXOBJ_Rectangle_t));
    Error=stlayer_TestViewPortParam(LayerHandle,InputRectangle,
                                    OutputRectangle,
                                    &ClippedIn,&ClippedOut,&TotalClipped,
                                    Bitmap_p );
    if(Error != ST_NO_ERROR)
    {
        STOS_SemaphoreSignal(stlayer_context.XContext[LayerHandle].AccessProtect_p);
        layergfx_Defense(Error);
        return(Error);
    }
    STOS_memcpy(&((stlayer_ViewPortDescriptor_t*)VPHandle)->ClippedOut, &ClippedOut, sizeof(STGXOBJ_Rectangle_t));
    STOS_memcpy(&((stlayer_ViewPortDescriptor_t*)VPHandle)->ClippedIn, &ClippedIn, sizeof(STGXOBJ_Rectangle_t));
    ((stlayer_ViewPortDescriptor_t*)VPHandle)->TotalClipped = TotalClipped;

    STOS_SemaphoreWait(stlayer_context.XContext[LayerHandle].AccessProtect_p);
    if (!(((stlayer_ViewPortDescriptor_t *)VPHandle)->TotalClipped))
    {
        Error = LAYERGFX_Show(VPHandle);
    }
    if(!Error)
    {
        ((stlayer_ViewPortDescriptor_t *)VPHandle)->Enabled = TRUE;
    }
    STOS_SemaphoreSignal(stlayer_context.XContext[LayerHandle].AccessProtect_p);
    return(Error);
}
/*******************************************************************************
Name        : LAYERGFX_DisableViewPort
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYERGFX_DisableViewPort(STLAYER_ViewPortHandle_t VPHandle)
{
    U32             LayerHandle;

    /* error checking: is the VP handle valid?*/
    if(VPHandle == 0)
    {
        layergfx_Defense(ST_ERROR_INVALID_HANDLE);
        return(ST_ERROR_INVALID_HANDLE);
    }
    if(!((stlayer_ViewPortDescriptor_t *)VPHandle)->Enabled)
    {
        return(ST_NO_ERROR);
    }
    LayerHandle = ((stlayer_ViewPortDescriptor_t*)VPHandle)->LayerHandle;
    STOS_SemaphoreWait(stlayer_context.XContext[LayerHandle].AccessProtect_p);
    if (!(((stlayer_ViewPortDescriptor_t *)VPHandle)->TotalClipped))
    {
        LAYERGFX_Hide(VPHandle);
    }
    ((stlayer_ViewPortDescriptor_t *)VPHandle)->Enabled = FALSE;
    STOS_SemaphoreSignal(stlayer_context.XContext[LayerHandle].AccessProtect_p);
    return(ST_NO_ERROR);
}
/*******************************************************************************
Name        : LAYERGFX_SetViewPortSource
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYERGFX_SetViewPortSource(STLAYER_ViewPortHandle_t VPHandle,
                                         STLAYER_ViewPortSource_t * VPSource)
{
  U32                   LayerHandle;
  STGXOBJ_Rectangle_t   InputRectangle,OutputRectangle;
  STGXOBJ_Rectangle_t   ClippedIn;
  STGXOBJ_Rectangle_t   ClippedOut;
  ST_ErrorCode_t        Error;
  BOOL                  TotalClipped;

  /* error checking: is the VP handle valid?*/
  if(VPHandle == 0)
  {
      layergfx_Defense(ST_ERROR_INVALID_HANDLE);
      return(ST_ERROR_INVALID_HANDLE);
  }
  if(VPSource == 0)
  {
      layergfx_Defense(ST_ERROR_BAD_PARAMETER);
      return(ST_ERROR_BAD_PARAMETER);
  }
  LayerHandle     = ((stlayer_ViewPortDescriptor_t*)VPHandle)->LayerHandle;
  STOS_SemaphoreWait(stlayer_context.XContext[LayerHandle].AccessProtect_p);
  STOS_memcpy(&InputRectangle, &((stlayer_ViewPortDescriptor_t*)VPHandle)->InputRect, sizeof(STGXOBJ_Rectangle_t));
  STOS_memcpy(&OutputRectangle, &((stlayer_ViewPortDescriptor_t*)VPHandle)->OutputRect, sizeof(STGXOBJ_Rectangle_t));

    /* Out and In params TBD */
  if(VPSource->Data.BitMap_p == 0)
  {
      STOS_SemaphoreSignal(stlayer_context.XContext[LayerHandle].AccessProtect_p);
      layergfx_Defense(ST_ERROR_BAD_PARAMETER);
      return(ST_ERROR_BAD_PARAMETER);
  }

  Error = stlayer_TestViewPortParam(LayerHandle,&InputRectangle,
                                    &OutputRectangle,
                                    &ClippedIn,&ClippedOut,&TotalClipped,
                                    VPSource->Data.BitMap_p );
  if(Error != ST_NO_ERROR)
  {
      STOS_SemaphoreSignal(stlayer_context.XContext[LayerHandle].AccessProtect_p);
      layergfx_Defense(Error);
      return(Error);
  }

  STOS_memcpy(&((stlayer_ViewPortDescriptor_t*)VPHandle)->Bitmap, VPSource->Data.BitMap_p, sizeof(STGXOBJ_Bitmap_t));
  if(stlayer_context.XContext[LayerHandle].LayerType == STLAYER_GAMMA_CURSOR)
  {
     if(VPSource->Palette_p == 0)
    {
        STOS_SemaphoreSignal(stlayer_context.XContext[LayerHandle].AccessProtect_p);
        layergfx_Defense(ST_ERROR_BAD_PARAMETER);
        return(ST_ERROR_BAD_PARAMETER);
    }
    STOS_memcpy(&((stlayer_ViewPortDescriptor_t*)VPHandle)->Palette, VPSource->Palette_p, sizeof(STGXOBJ_Palette_t));
  }
  STOS_memcpy(&((stlayer_ViewPortDescriptor_t*)VPHandle)->ClippedIn, &ClippedIn, sizeof(STGXOBJ_Rectangle_t));
  STOS_memcpy(&((stlayer_ViewPortDescriptor_t*)VPHandle)->ClippedOut, &ClippedOut, sizeof(STGXOBJ_Rectangle_t));
  if(((stlayer_ViewPortDescriptor_t*)VPHandle)->Enabled)
  {
      if((((stlayer_ViewPortDescriptor_t*)VPHandle)->TotalClipped)
              && (!(TotalClipped)))
      {
          LAYERGFX_Show(VPHandle);
      }
      if (!((((stlayer_ViewPortDescriptor_t*)VPHandle)->TotalClipped))
              && (TotalClipped))
      {
          LAYERGFX_Hide(VPHandle);
      }
  }
  ((stlayer_ViewPortDescriptor_t*)VPHandle)->TotalClipped = TotalClipped;
  stlayer_UpdateNodeGeneric((stlayer_ViewPortHandle_t)VPHandle);
  STOS_SemaphoreSignal(stlayer_context.XContext[LayerHandle].AccessProtect_p);

  return(ST_NO_ERROR);
}
/*******************************************************************************
Name        : LAYERGFX_GetViewPortSource
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYERGFX_GetViewPortSource(STLAYER_ViewPortHandle_t VPHandle,
                                         STLAYER_ViewPortSource_t * VPSource)
{
  stlayer_ViewPortHandle_t    VPHandleCasted;
  STLAYER_Handle_t            LayerHandle;
  /* error checking: is the VP handle valid?*/
  if(VPHandle == 0)
  {
      layergfx_Defense(ST_ERROR_INVALID_HANDLE);
      return(ST_ERROR_INVALID_HANDLE);
  }
  if(VPSource == 0)
  {
      layergfx_Defense(ST_ERROR_BAD_PARAMETER);
      return(ST_ERROR_BAD_PARAMETER);
  }
  if((VPSource->Data.BitMap_p == 0) || (VPSource->Palette_p == 0))
  {
      layergfx_Defense(ST_ERROR_BAD_PARAMETER);
      return(ST_ERROR_BAD_PARAMETER);
  }
  VPHandleCasted = (stlayer_ViewPortHandle_t)(VPHandle);
  LayerHandle    = ((stlayer_ViewPortDescriptor_t*)VPHandle)->LayerHandle;

  STOS_SemaphoreWait(stlayer_context.XContext[LayerHandle].AccessProtect_p);
  VPSource->SourceType          = STLAYER_GRAPHIC_BITMAP;
  STOS_memcpy(VPSource->Data.BitMap_p, &(VPHandleCasted->Bitmap), sizeof(STGXOBJ_Bitmap_t));
  STOS_memcpy(VPSource->Palette_p, &(VPHandleCasted->Palette), sizeof(STGXOBJ_Palette_t));

  STOS_SemaphoreSignal(stlayer_context.XContext[LayerHandle].AccessProtect_p);
  return(ST_NO_ERROR);
}
/*******************************************************************************
Name        : LAYERGFX_SetViewPortIORectangle
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYERGFX_SetViewPortIORectangle(
                                STLAYER_ViewPortHandle_t VPHandle,
                                STGXOBJ_Rectangle_t * InputRectangle,
                                STGXOBJ_Rectangle_t * OutputRectangle)
{
    ST_ErrorCode_t        Error,Move;
    U32                   LayerHandle;
    STGXOBJ_Bitmap_t      * Bitmap_p;
    STGXOBJ_Rectangle_t   ClippedIn;
    STGXOBJ_Rectangle_t   ClippedOut;
    STGXOBJ_Rectangle_t   PrevInput;
    STGXOBJ_Rectangle_t   PrevOutput;
    BOOL                  TotalClipped;


    #if defined (ST_7109)
        U32 CutId;
    #endif


    /* error checking: is the VP handle valid?*/
    if(VPHandle == 0)
    {
        layergfx_Defense(ST_ERROR_INVALID_HANDLE);
        return(ST_ERROR_INVALID_HANDLE);
    }
    if ((InputRectangle == 0) || (OutputRectangle == 0))
    {
        layergfx_Defense(ST_ERROR_INVALID_HANDLE);
        return(ST_ERROR_INVALID_HANDLE);
    }


  #if defined (ST_7109)
    CutId = ST_GetCutRevision();
  #endif


 #if defined (ST_7109) || defined (ST_7710) || defined (ST_7100)

      /* Test Vertical Resize and Horizontal Downsize */

    #if defined (ST_7109)

     if (CutId < 0xC0)
        {
    #endif

    if(OutputRectangle->Height != InputRectangle->Height)
    {

        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    if(OutputRectangle->Width < InputRectangle->Width)
    {

        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    #if defined (ST_7109)
       }

   else
   {

        if (((OutputRectangle->Height) != (InputRectangle->Height)) &&
                 (((stlayer_ViewPortDescriptor_t*)VPHandle)->FFilterEnabled == TRUE))
        {


            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
        }
#if 0
        if((OutputRectangle->Width*2) < (InputRectangle->Width))
        {

            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
        }
#endif

   }

    #endif

#endif


   #if defined (ST_7200)

	if (((OutputRectangle->Height) != (InputRectangle->Height)) &&
	         (((stlayer_ViewPortDescriptor_t*)VPHandle)->FFilterEnabled == TRUE))
	{
	    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
	}
   #endif



#ifdef WA_GdpResizeOnGX1
    if(OutputRectangle->Width > 8 * InputRectangle->Width)
    {
        OutputRectangle->Width = 8 *InputRectangle->Width;
    }
    if(OutputRectangle->Height > 2 * InputRectangle->Height)
    {
        OutputRectangle->Height = 2 * InputRectangle->Height;
    }
#endif

    LayerHandle = ((stlayer_ViewPortDescriptor_t*)VPHandle)->LayerHandle;
    STOS_SemaphoreWait(stlayer_context.XContext[LayerHandle].AccessProtect_p);
    Bitmap_p    = &(((stlayer_ViewPortDescriptor_t*)VPHandle)->Bitmap);
    Error = stlayer_TestViewPortParam(LayerHandle,InputRectangle,
                                    OutputRectangle,
                                    &ClippedIn,&ClippedOut,&TotalClipped,
                                    Bitmap_p );
    if(Error != ST_NO_ERROR)
    {
        STOS_SemaphoreSignal(stlayer_context.XContext[LayerHandle].AccessProtect_p);
        layergfx_Defense(Error);
        return(Error);
    }

    /* Test output rectangle if enabled and update linked list */
    if(((stlayer_ViewPortDescriptor_t*)VPHandle)->Enabled)
    {
        if(TotalClipped) /* VP is now unvisible */
        {
            if (!(((stlayer_ViewPortDescriptor_t*)VPHandle)->TotalClipped))
            {
                LAYERGFX_Hide(VPHandle);
            }
            ((stlayer_ViewPortDescriptor_t*)VPHandle)->TotalClipped
                                                                = TotalClipped;
            STOS_memcpy(&((stlayer_ViewPortDescriptor_t*)VPHandle)->InputRect, InputRectangle, sizeof(STGXOBJ_Rectangle_t));
            STOS_memcpy(&((stlayer_ViewPortDescriptor_t*)VPHandle)->OutputRect, OutputRectangle, sizeof(STGXOBJ_Rectangle_t));
            STOS_memcpy(&((stlayer_ViewPortDescriptor_t*)VPHandle)->ClippedOut, &ClippedOut, sizeof(STGXOBJ_Rectangle_t));
            STOS_memcpy(&((stlayer_ViewPortDescriptor_t*)VPHandle)->ClippedIn, &ClippedIn, sizeof(STGXOBJ_Rectangle_t));

            STOS_SemaphoreSignal(stlayer_context.XContext[LayerHandle]
                                                                .AccessProtect_p);
            return(ST_NO_ERROR);
        }
        /* TotalClipped is FALSE : VP is now visible */
        if(((stlayer_ViewPortDescriptor_t*)VPHandle)->TotalClipped)
        {
            /* VP was unvisible */
            STOS_memcpy(&PrevInput, &((stlayer_ViewPortDescriptor_t*)VPHandle)->InputRect, sizeof(STGXOBJ_Rectangle_t));
            STOS_memcpy(&PrevOutput, &((stlayer_ViewPortDescriptor_t*)VPHandle)->OutputRect, sizeof(STGXOBJ_Rectangle_t));
            STOS_memcpy(&((stlayer_ViewPortDescriptor_t*)VPHandle)->InputRect, InputRectangle, sizeof(STGXOBJ_Rectangle_t));

            STOS_memcpy(&((stlayer_ViewPortDescriptor_t*)VPHandle)->OutputRect, OutputRectangle, sizeof(STGXOBJ_Rectangle_t));
            STOS_memcpy(&((stlayer_ViewPortDescriptor_t*)VPHandle)->ClippedOut, &ClippedOut, sizeof(STGXOBJ_Rectangle_t));
            STOS_memcpy(&((stlayer_ViewPortDescriptor_t*)VPHandle)->ClippedIn, &ClippedIn, sizeof(STGXOBJ_Rectangle_t));

            ((stlayer_ViewPortDescriptor_t*)VPHandle)->TotalClipped = FALSE;
            Error = LAYERGFX_Show(VPHandle);
            if (Error != ST_NO_ERROR)
            {
                STOS_memcpy(&((stlayer_ViewPortDescriptor_t*)VPHandle)->InputRect, &PrevInput, sizeof(STGXOBJ_Rectangle_t));
                STOS_memcpy(&((stlayer_ViewPortDescriptor_t*)VPHandle)->OutputRect, &PrevOutput, sizeof(STGXOBJ_Rectangle_t));

                STOS_SemaphoreSignal(stlayer_context.XContext[LayerHandle].
                                                            AccessProtect_p);
                layergfx_Defense(STLAYER_ERROR_OVERLAP_VIEWPORT);
                return(STLAYER_ERROR_OVERLAP_VIEWPORT);
            }
            STOS_SemaphoreSignal(stlayer_context.XContext[LayerHandle].
                                                            AccessProtect_p);
            return(ST_NO_ERROR);
        }
        /* VP is now visible and was already visible : this is a real move */
        if(stlayer_context.XContext[LayerHandle].LayerType
                                                != STLAYER_GAMMA_CURSOR)
        {
            Move = stlayer_TestMove(VPHandle,&ClippedOut);
            if(Move == DESC_MANAGER_OK)
            {
                STOS_memcpy(&((stlayer_ViewPortDescriptor_t*)VPHandle)->InputRect, InputRectangle, sizeof(STGXOBJ_Rectangle_t));
                STOS_memcpy(&((stlayer_ViewPortDescriptor_t*)VPHandle)->OutputRect, OutputRectangle, sizeof(STGXOBJ_Rectangle_t));
                STOS_memcpy(&((stlayer_ViewPortDescriptor_t*)VPHandle)->ClippedOut, &ClippedOut, sizeof(STGXOBJ_Rectangle_t));
                STOS_memcpy(&((stlayer_ViewPortDescriptor_t*)VPHandle)->ClippedIn, &ClippedIn, sizeof(STGXOBJ_Rectangle_t));
                stlayer_UpdateNodeGeneric((stlayer_ViewPortHandle_t)VPHandle);
            }
            else if(Move == DESC_MANAGER_MOVE_INVERSION_LIST)
            {
                STOS_memcpy(&((stlayer_ViewPortDescriptor_t*)VPHandle)->InputRect, InputRectangle, sizeof(STGXOBJ_Rectangle_t));
                STOS_memcpy(&((stlayer_ViewPortDescriptor_t*)VPHandle)->OutputRect, OutputRectangle, sizeof(STGXOBJ_Rectangle_t));
                STOS_memcpy(&((stlayer_ViewPortDescriptor_t*)VPHandle)->ClippedOut, &ClippedOut, sizeof(STGXOBJ_Rectangle_t));
                STOS_memcpy(&((stlayer_ViewPortDescriptor_t*)VPHandle)->ClippedIn, &ClippedIn, sizeof(STGXOBJ_Rectangle_t));
                /* rebuild the list */
                LAYERGFX_Hide(VPHandle);
                LAYERGFX_Show(VPHandle);
            }
            else /* Move == DESC_MANAGER_IMPOSSIBLE_RESIZE_MOVE */
            {
                STOS_SemaphoreSignal(stlayer_context.XContext[LayerHandle].
                                                                AccessProtect_p);
                layergfx_Defense(STLAYER_ERROR_OVERLAP_VIEWPORT);
                return(STLAYER_ERROR_OVERLAP_VIEWPORT);
            }
        }
        else  /* gamma cursor layer */
        {
            STOS_memcpy(&((stlayer_ViewPortDescriptor_t*)VPHandle)->InputRect, InputRectangle, sizeof(STGXOBJ_Rectangle_t));
            STOS_memcpy(&((stlayer_ViewPortDescriptor_t*)VPHandle)->OutputRect, OutputRectangle, sizeof(STGXOBJ_Rectangle_t));
            STOS_memcpy(&((stlayer_ViewPortDescriptor_t*)VPHandle)->ClippedIn, &ClippedIn, sizeof(STGXOBJ_Rectangle_t));
            STOS_memcpy(&((stlayer_ViewPortDescriptor_t*)VPHandle)->ClippedOut, &ClippedOut, sizeof(STGXOBJ_Rectangle_t));
            stlayer_UpdateNodeGeneric((stlayer_ViewPortHandle_t)VPHandle);
        }
    }
    else
    {
        /* VP is disabled : just set the new dim */
        STOS_memcpy(&((stlayer_ViewPortDescriptor_t*)VPHandle)->InputRect, InputRectangle, sizeof(STGXOBJ_Rectangle_t));
        STOS_memcpy(&((stlayer_ViewPortDescriptor_t*)VPHandle)->OutputRect, OutputRectangle, sizeof(STGXOBJ_Rectangle_t));
        STOS_memcpy(&((stlayer_ViewPortDescriptor_t*)VPHandle)->ClippedIn, &ClippedIn, sizeof(STGXOBJ_Rectangle_t));
        STOS_memcpy(&((stlayer_ViewPortDescriptor_t*)VPHandle)->ClippedOut, &ClippedOut, sizeof(STGXOBJ_Rectangle_t));
    }
    STOS_SemaphoreSignal(stlayer_context.XContext[LayerHandle].AccessProtect_p);
    return(ST_NO_ERROR);
}
/*******************************************************************************
Name        : LAYERGFX_AdjustIORectangle
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYERGFX_AdjustIORectangle(STLAYER_Handle_t Handle,
                                         STGXOBJ_Rectangle_t * InputRectangle,
                                         STGXOBJ_Rectangle_t * OutputRectangle)
{
    ST_ErrorCode_t Error;

    Error = ST_NO_ERROR;
    /* test param */
    if((Handle >= MAX_LAYER)
    || (!(stlayer_context.XContext[Handle].Open)))
    {
        layergfx_Defense(ST_ERROR_INVALID_HANDLE);
        return(ST_ERROR_INVALID_HANDLE);
    }

    STOS_SemaphoreWait(stlayer_context.XContext[Handle].AccessProtect_p);
    /* Selection tests */
    if(OutputRectangle->PositionX
            > (S32)(stlayer_context.XContext[Handle].LayerParams.Width))
    {
       STOS_SemaphoreSignal(stlayer_context.XContext[Handle].AccessProtect_p);
       return(STLAYER_ERROR_IORECTANGLES_NOT_ADJUSTABLE);
    }
    if(OutputRectangle->PositionY
            > (S32)(stlayer_context.XContext[Handle].LayerParams.Height))
    {
       STOS_SemaphoreSignal(stlayer_context.XContext[Handle].AccessProtect_p);
       return(STLAYER_ERROR_IORECTANGLES_NOT_ADJUSTABLE);
    }

    /* Adjust Resize */
    if ((stlayer_context.XContext[Handle].LayerType == STLAYER_GAMMA_CURSOR)
         || (stlayer_context.XContext[Handle].LayerType == STLAYER_GAMMA_BKL))
    {
        /* test Vresize */
        if (InputRectangle->Height != OutputRectangle->Height)
        {
            InputRectangle->Height = OutputRectangle->Height;
            Error = STLAYER_SUCCESS_IORECTANGLES_ADJUSTED;
        }
        /* Test Hresize */
        if(InputRectangle->Width != OutputRectangle->Width)
        {
            InputRectangle->Width = OutputRectangle->Width;
            Error = STLAYER_SUCCESS_IORECTANGLES_ADJUSTED;
        }
    }
    STOS_SemaphoreSignal(stlayer_context.XContext[Handle].AccessProtect_p);
    return(Error);
}

/*******************************************************************************
Name        : LAYERGFX_GetViewPortIORectangle
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYERGFX_GetViewPortIORectangle(
                                    STLAYER_ViewPortHandle_t VPHandle,
                                    STGXOBJ_Rectangle_t * InputRectangle,
                                    STGXOBJ_Rectangle_t * OutputRectangle)
{
  stlayer_ViewPortHandle_t    VPHandleCasted;
  STLAYER_Handle_t            LayerHandle;
  /* error checking: is the VP handle valid?*/
  if(VPHandle == 0)
  {
      layergfx_Defense(ST_ERROR_INVALID_HANDLE);
      return(ST_ERROR_INVALID_HANDLE);
  }
  if ((InputRectangle == 0) || (OutputRectangle == 0))
  {
      layergfx_Defense(ST_ERROR_BAD_PARAMETER);
      return(ST_ERROR_BAD_PARAMETER);
  }
  VPHandleCasted = (stlayer_ViewPortHandle_t)(VPHandle);
  LayerHandle    = VPHandleCasted->LayerHandle;
  STOS_SemaphoreWait(stlayer_context.XContext[LayerHandle].AccessProtect_p);
  STOS_memcpy(InputRectangle, &VPHandleCasted->InputRect, sizeof(STGXOBJ_Rectangle_t));
  STOS_memcpy(OutputRectangle, &VPHandleCasted->OutputRect, sizeof(STGXOBJ_Rectangle_t));
  STOS_SemaphoreSignal(stlayer_context.XContext[LayerHandle].AccessProtect_p);
  return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : LAYERGFX_SetViewPortPosition
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYERGFX_SetViewPortPosition(STLAYER_ViewPortHandle_t VPHandle,
                                           S32                      XPosition,
                                           S32                      YPosition)
{
  STGXOBJ_Rectangle_t    Input, Output;
  ST_ErrorCode_t      Error;
  stlayer_ViewPortHandle_t    VPHandleCasted;
  STLAYER_Handle_t            LayerHandle;

  /* error checking: is the VP handle valid?*/
  if(VPHandle == 0)
  {
      layergfx_Defense(ST_ERROR_INVALID_HANDLE);
      return(ST_ERROR_INVALID_HANDLE);
  }
  /* change output position */
  VPHandleCasted = (stlayer_ViewPortHandle_t)(VPHandle);
  LayerHandle    = VPHandleCasted->LayerHandle;
  STOS_SemaphoreWait(stlayer_context.XContext[LayerHandle].AccessProtect_p);
  STOS_memcpy(&Input, &VPHandleCasted->InputRect, sizeof(STGXOBJ_Rectangle_t));
  STOS_memcpy(&Output, &VPHandleCasted->OutputRect, sizeof(STGXOBJ_Rectangle_t));
  STOS_SemaphoreSignal(stlayer_context.XContext[LayerHandle].AccessProtect_p);
  Output.PositionX = XPosition;
  Output.PositionY = YPosition;
  Error = LAYERGFX_SetViewPortIORectangle(VPHandle,&Input,&Output);
  if (Error)
  {
      layergfx_Defense(Error);
      return(Error);
  }
  else
  {
    return(ST_NO_ERROR);
  }
}

/*******************************************************************************
Name        : LAYERGFX_GetViewPortPosition
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYERGFX_GetViewPortPosition(STLAYER_ViewPortHandle_t VPHandle,
                                           S32 *                    XPosition,
                                           S32 *                    YPosition)
{
  stlayer_ViewPortHandle_t    VPHandleCasted;
  STLAYER_Handle_t            LayerHandle;
  /* error checking: is the VP handle valid?*/
  if(VPHandle == 0)
  {
      layergfx_Defense(ST_ERROR_INVALID_HANDLE);
      return(ST_ERROR_INVALID_HANDLE);
  }
  if ((XPosition == 0) || (YPosition == 0))
  {
      layergfx_Defense(ST_ERROR_BAD_PARAMETER);
      return(ST_ERROR_BAD_PARAMETER);
  }
  VPHandleCasted = (stlayer_ViewPortHandle_t)(VPHandle);
  LayerHandle    = VPHandleCasted->LayerHandle;
  STOS_SemaphoreWait(stlayer_context.XContext[LayerHandle].AccessProtect_p);
  * XPosition = VPHandleCasted->OutputRect.PositionX;
  * YPosition = VPHandleCasted->OutputRect.PositionY;
  STOS_SemaphoreSignal(stlayer_context.XContext[LayerHandle].AccessProtect_p);
  return(ST_NO_ERROR);
}
/*******************************************************************************
Name        : LAYERGFX_SetViewPortPSI
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYERGFX_SetViewPortPSI(STLAYER_ViewPortHandle_t VPHandle,
                                      STLAYER_PSI_t *          VPPSI_p)
{
  STLAYER_PSI_t Dummy;
  /* error checking: is the VP handle valid?*/
  if(VPHandle == 0)
  {
      layergfx_Defense(ST_ERROR_INVALID_HANDLE);
      return(ST_ERROR_INVALID_HANDLE);
  }
  Dummy = *VPPSI_p;
  return(ST_ERROR_FEATURE_NOT_SUPPORTED);
}
/*******************************************************************************
Name        : LAYERGFX_DisableColorKey
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYERGFX_DisableColorKey(STLAYER_ViewPortHandle_t VPHandle)
{
  U32  LayerHandle;

  /* error checking: is the VP handle valid?*/
  if(VPHandle == 0)
  {
      layergfx_Defense(ST_ERROR_INVALID_HANDLE);
      return(ST_ERROR_INVALID_HANDLE);
  }
  LayerHandle = ((stlayer_ViewPortDescriptor_t*)VPHandle)->LayerHandle;
  if (stlayer_context.XContext[LayerHandle].LayerType == STLAYER_GAMMA_CURSOR )
  {
      layergfx_Defense(ST_ERROR_FEATURE_NOT_SUPPORTED);
      return(ST_ERROR_FEATURE_NOT_SUPPORTED);
  }
  STOS_SemaphoreWait(stlayer_context.XContext[LayerHandle].AccessProtect_p);
  ((stlayer_ViewPortDescriptor_t*)VPHandle)->ColorKeyEnabled = FALSE;
  stlayer_UpdateNodeGeneric((stlayer_ViewPortHandle_t)VPHandle);
  STOS_SemaphoreSignal(stlayer_context.XContext[LayerHandle].AccessProtect_p);
  return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : LAYERGFX_EnableColorKey
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYERGFX_EnableColorKey(STLAYER_ViewPortHandle_t VPHandle)
{
  U32  LayerHandle;
  /* error checking: is the VP handle valid?*/
  if(VPHandle == 0)
  {
      layergfx_Defense(ST_ERROR_INVALID_HANDLE);
      return(ST_ERROR_INVALID_HANDLE);
  }
  LayerHandle = ((stlayer_ViewPortDescriptor_t*)VPHandle)->LayerHandle;
  if (stlayer_context.XContext[LayerHandle].LayerType == STLAYER_GAMMA_CURSOR )
  {
      layergfx_Defense(ST_ERROR_FEATURE_NOT_SUPPORTED);
      return(ST_ERROR_FEATURE_NOT_SUPPORTED);
  }
  STOS_SemaphoreWait(stlayer_context.XContext[LayerHandle].AccessProtect_p);
  ((stlayer_ViewPortDescriptor_t*)VPHandle)->ColorKeyEnabled = TRUE;
  stlayer_UpdateNodeGeneric((stlayer_ViewPortHandle_t)VPHandle);
  STOS_SemaphoreSignal(stlayer_context.XContext[LayerHandle].AccessProtect_p);
  return(ST_NO_ERROR);
}


/*******************************************************************************
Name        : LAYERGFX_EnableFlickerFilter
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYERGFX_EnableFlickerFilter(STLAYER_ViewPortHandle_t VPHandle)
{
  U32  LayerHandle;
  /* error checking: is the VP handle valid?*/
    #if defined (ST_7109)
        U32 CutId;
    #endif


  #if defined (ST_7109)
    CutId = ST_GetCutRevision();
  #endif


  if(VPHandle == 0)
  {
      layergfx_Defense(ST_ERROR_INVALID_HANDLE);
      return(ST_ERROR_INVALID_HANDLE);
  }
  LayerHandle = ((stlayer_ViewPortDescriptor_t*)VPHandle)->LayerHandle;
  if (stlayer_context.XContext[LayerHandle].LayerType == STLAYER_GAMMA_CURSOR )    /* see with alpha */
  {
      layergfx_Defense(ST_ERROR_FEATURE_NOT_SUPPORTED);
      return(ST_ERROR_FEATURE_NOT_SUPPORTED);
  }


 #if defined (ST_7109) || defined (ST_7200)

    #if defined (ST_7109)

    if (CutId >= 0xC0)
    {
    #endif
       if(((stlayer_ViewPortDescriptor_t*)VPHandle)->InputRect.Height!=((stlayer_ViewPortDescriptor_t*)VPHandle)->OutputRect.Height)
             {
                layergfx_Defense(ST_ERROR_FEATURE_NOT_SUPPORTED);
                return(ST_ERROR_FEATURE_NOT_SUPPORTED);
             }
        if(stlayer_context.XContext[LayerHandle].LayerParams.ScanType == STGXOBJ_PROGRESSIVE_SCAN)
             {
                layergfx_Defense(ST_ERROR_FEATURE_NOT_SUPPORTED);
                return(ST_ERROR_FEATURE_NOT_SUPPORTED);
             }

  #if defined (ST_7109)
   }
  #endif

 #endif


  STOS_SemaphoreWait(stlayer_context.XContext[LayerHandle].AccessProtect_p);
  ((stlayer_ViewPortDescriptor_t*)VPHandle)->FFilterEnabled = TRUE;
  stlayer_UpdateNodeGeneric((stlayer_ViewPortHandle_t)VPHandle);
  STOS_SemaphoreSignal(stlayer_context.XContext[LayerHandle].AccessProtect_p);
  return(ST_NO_ERROR);
}



/*******************************************************************************
Name        : LAYERGFX_DisableFlickerFilter
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYERGFX_DisableFlickerFilter(STLAYER_ViewPortHandle_t VPHandle)
{
  U32  LayerHandle;

   #if defined (ST_7109)
        U32 CutId;
    #endif

  #if defined (ST_7109)
    CutId = ST_GetCutRevision();
  #endif


  /* error checking: is the VP handle valid?*/
  if(VPHandle == 0)
  {
      layergfx_Defense(ST_ERROR_INVALID_HANDLE);
      return(ST_ERROR_INVALID_HANDLE);
  }
  LayerHandle = ((stlayer_ViewPortDescriptor_t*)VPHandle)->LayerHandle;
  if (stlayer_context.XContext[LayerHandle].LayerType == STLAYER_GAMMA_CURSOR )    /* see with alpha */
  {
      layergfx_Defense(ST_ERROR_FEATURE_NOT_SUPPORTED);
      return(ST_ERROR_FEATURE_NOT_SUPPORTED);
  }


#if defined (ST_7109) || defined (ST_7200)

    #if defined (ST_7109)

    if (CutId >= 0xC0)
    {
    #endif
       if(((stlayer_ViewPortDescriptor_t*)VPHandle)->InputRect.Height!=((stlayer_ViewPortDescriptor_t*)VPHandle)->OutputRect.Height)
             {
                layergfx_Defense(ST_ERROR_FEATURE_NOT_SUPPORTED);
                return(ST_ERROR_FEATURE_NOT_SUPPORTED);
             }
  #if defined (ST_7109)
   }
  #endif

    #endif

   STOS_SemaphoreWait(stlayer_context.XContext[LayerHandle].AccessProtect_p);
  ((stlayer_ViewPortDescriptor_t*)VPHandle)->FFilterEnabled = FALSE;
  stlayer_UpdateNodeGeneric((stlayer_ViewPortHandle_t)VPHandle);
  STOS_SemaphoreSignal(stlayer_context.XContext[LayerHandle].AccessProtect_p);
  return(ST_NO_ERROR);
}


#if defined(ST_7109) || defined(ST_7200)

/*******************************************************************************
Name        : LAYERGFX_SetFlickerFilterMode
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYERGFX_SetViewPortFlickerFilterMode(STLAYER_ViewPortHandle_t VPHandle,
                                            STLAYER_FlickerFilterMode_t  FlickerFilterMode)
{
  U32  LayerHandle;
  /* error checking: is the VP handle valid?*/
  if(VPHandle == 0)
  {
      layergfx_Defense(ST_ERROR_INVALID_HANDLE);
      return(ST_ERROR_INVALID_HANDLE);
  }

  if ((FlickerFilterMode != STLAYER_FLICKER_FILTER_MODE_SIMPLE)  &&
      (FlickerFilterMode != STLAYER_FLICKER_FILTER_MODE_ADAPTIVE))
  {
      layergfx_Defense(ST_ERROR_FEATURE_NOT_SUPPORTED);
      return(ST_ERROR_FEATURE_NOT_SUPPORTED);
  }


  LayerHandle = ((stlayer_ViewPortDescriptor_t*)VPHandle)->LayerHandle;
  if (stlayer_context.XContext[LayerHandle].LayerType == STLAYER_GAMMA_CURSOR )    /* see with alpha */
  {
      layergfx_Defense(ST_ERROR_FEATURE_NOT_SUPPORTED);
      return(ST_ERROR_FEATURE_NOT_SUPPORTED);
  }

  STOS_SemaphoreWait(stlayer_context.XContext[LayerHandle].AccessProtect_p);
  ((stlayer_ViewPortDescriptor_t*)VPHandle)->FFilterMode = FlickerFilterMode;
  stlayer_UpdateNodeGeneric((stlayer_ViewPortHandle_t)VPHandle);
  STOS_SemaphoreSignal(stlayer_context.XContext[LayerHandle].AccessProtect_p);
  return(ST_NO_ERROR);
}


/*******************************************************************************
Name        : LAYERGFX_GetFlickerFilterMode
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYERGFX_GetViewPortFlickerFilterMode(STLAYER_ViewPortHandle_t VPHandle,
                                            STLAYER_FlickerFilterMode_t * FlickerFilterMode)
{
  U32  LayerHandle;
  /* error checking: is the VP handle valid?*/
  if(VPHandle == 0)
  {
      layergfx_Defense(ST_ERROR_INVALID_HANDLE);
      return(ST_ERROR_INVALID_HANDLE);
  }
  LayerHandle = ((stlayer_ViewPortDescriptor_t*)VPHandle)->LayerHandle;
  if (stlayer_context.XContext[LayerHandle].LayerType == STLAYER_GAMMA_CURSOR )    /* see with alpha */
  {
      layergfx_Defense(ST_ERROR_FEATURE_NOT_SUPPORTED);
      return(ST_ERROR_FEATURE_NOT_SUPPORTED);
  }
  STOS_SemaphoreWait(stlayer_context.XContext[LayerHandle].AccessProtect_p);
  *FlickerFilterMode = ((stlayer_ViewPortDescriptor_t*)VPHandle)->FFilterMode;
  stlayer_UpdateNodeGeneric((stlayer_ViewPortHandle_t)VPHandle);
  STOS_SemaphoreSignal(stlayer_context.XContext[LayerHandle].AccessProtect_p);
  return(ST_NO_ERROR);
}


/*******************************************************************************
Name        : LAYERGFX_EnableViewportColorFill
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYERGFX_EnableViewportColorFill(STLAYER_ViewPortHandle_t VPHandle)
{
  U32  LayerHandle;
  /* error checking: is the VP handle valid?*/
  if(VPHandle == 0)
  {
      layergfx_Defense(ST_ERROR_INVALID_HANDLE);
      return(ST_ERROR_INVALID_HANDLE);
  }
  LayerHandle = ((stlayer_ViewPortDescriptor_t*)VPHandle)->LayerHandle;
  if (stlayer_context.XContext[LayerHandle].LayerType == STLAYER_GAMMA_CURSOR )    /* see with alpha */
  {
      layergfx_Defense(ST_ERROR_FEATURE_NOT_SUPPORTED);
      return(ST_ERROR_FEATURE_NOT_SUPPORTED);
  }


  STOS_SemaphoreWait(stlayer_context.XContext[LayerHandle].AccessProtect_p);
  ((stlayer_ViewPortDescriptor_t*)VPHandle)->ColorFillEnabled = TRUE;
  stlayer_UpdateNodeGeneric((stlayer_ViewPortHandle_t)VPHandle);
  STOS_SemaphoreSignal(stlayer_context.XContext[LayerHandle].AccessProtect_p);
  return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : LAYERGFX_DisableViewportColorFill
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYERGFX_DisableViewportColorFill(STLAYER_ViewPortHandle_t VPHandle)
{
  U32  LayerHandle;
  /* error checking: is the VP handle valid?*/
  if(VPHandle == 0)
  {
      layergfx_Defense(ST_ERROR_INVALID_HANDLE);
      return(ST_ERROR_INVALID_HANDLE);
  }
  LayerHandle = ((stlayer_ViewPortDescriptor_t*)VPHandle)->LayerHandle;
  if (stlayer_context.XContext[LayerHandle].LayerType == STLAYER_GAMMA_CURSOR )    /* see with alpha */
  {
      layergfx_Defense(ST_ERROR_FEATURE_NOT_SUPPORTED);
      return(ST_ERROR_FEATURE_NOT_SUPPORTED);
  }
  STOS_SemaphoreWait(stlayer_context.XContext[LayerHandle].AccessProtect_p);
  ((stlayer_ViewPortDescriptor_t*)VPHandle)->ColorFillEnabled = FALSE;
  stlayer_UpdateNodeGeneric((stlayer_ViewPortHandle_t)VPHandle);
  STOS_SemaphoreSignal(stlayer_context.XContext[LayerHandle].AccessProtect_p);
  return(ST_NO_ERROR);
}





/*******************************************************************************
Name        : LAYERGFX_SetViewportColorFill
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYERGFX_SetViewportColorFill(STLAYER_ViewPortHandle_t VPHandle,
                                                STGXOBJ_ColorARGB_t*  ColorFill)
{
  U32  LayerHandle;
  /* error checking: is the VP handle valid?*/
  if(VPHandle == 0)
  {
      layergfx_Defense(ST_ERROR_INVALID_HANDLE);
      return(ST_ERROR_INVALID_HANDLE);
  }
  LayerHandle = ((stlayer_ViewPortDescriptor_t*)VPHandle)->LayerHandle;
  if (stlayer_context.XContext[LayerHandle].LayerType != STLAYER_GAMMA_GDP )    /* see with alpha */
  {
      layergfx_Defense(ST_ERROR_FEATURE_NOT_SUPPORTED);
      return(ST_ERROR_FEATURE_NOT_SUPPORTED);
  }
  STOS_SemaphoreWait(stlayer_context.XContext[LayerHandle].AccessProtect_p);
  ((stlayer_ViewPortDescriptor_t*)VPHandle)->ColorFillValue = *ColorFill;
  stlayer_UpdateNodeGeneric((stlayer_ViewPortHandle_t)VPHandle);
  STOS_SemaphoreSignal(stlayer_context.XContext[LayerHandle].AccessProtect_p);
  return(ST_NO_ERROR);
}


/*******************************************************************************
Name        : LAYERGFX_GetViewportColorFill
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYERGFX_GetViewportColorFill(STLAYER_ViewPortHandle_t VPHandle,
                                                STGXOBJ_ColorARGB_t*  ColorFill)
{
  U32  LayerHandle;
  /* error checking: is the VP handle valid?*/
  if(VPHandle == 0)
  {
      layergfx_Defense(ST_ERROR_INVALID_HANDLE);
      return(ST_ERROR_INVALID_HANDLE);
  }
  LayerHandle = ((stlayer_ViewPortDescriptor_t*)VPHandle)->LayerHandle;
  if (stlayer_context.XContext[LayerHandle].LayerType != STLAYER_GAMMA_GDP )    /* see with alpha */
  {
      layergfx_Defense(ST_ERROR_FEATURE_NOT_SUPPORTED);
      return(ST_ERROR_FEATURE_NOT_SUPPORTED);
  }
  STOS_SemaphoreWait(stlayer_context.XContext[LayerHandle].AccessProtect_p);
   * ColorFill = ((stlayer_ViewPortDescriptor_t*)VPHandle)->ColorFillValue;
  stlayer_UpdateNodeGeneric((stlayer_ViewPortHandle_t)VPHandle);
  STOS_SemaphoreSignal(stlayer_context.XContext[LayerHandle].AccessProtect_p);
  return(ST_NO_ERROR);
}



 #endif



/*******************************************************************************
Name        : LAYERGFX_SetViewPortColorKey
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYERGFX_SetViewPortColorKey(STLAYER_ViewPortHandle_t VPHandle,
                                            STGXOBJ_ColorKey_t *     ColorKey)
{
  U32  LayerHandle;
  /* error checking: is the VP handle valid?*/
  if(VPHandle == 0)
  {
      layergfx_Defense(ST_ERROR_INVALID_HANDLE);
      return(ST_ERROR_INVALID_HANDLE);
  }
  LayerHandle = ((stlayer_ViewPortDescriptor_t*)VPHandle)->LayerHandle;
  if (stlayer_context.XContext[LayerHandle].LayerType == STLAYER_GAMMA_CURSOR )
  {
      layergfx_Defense(ST_ERROR_FEATURE_NOT_SUPPORTED);
      return(ST_ERROR_FEATURE_NOT_SUPPORTED);
  }
  if(ColorKey == 0)
  {
      layergfx_Defense(ST_ERROR_BAD_PARAMETER);
      return(ST_ERROR_BAD_PARAMETER);
  }
  if((ColorKey->Type != STGXOBJ_COLOR_KEY_TYPE_RGB888)
      &&(ColorKey->Type != STGXOBJ_COLOR_KEY_TYPE_YCbCr888_UNSIGNED))
  {
      layergfx_Defense(ST_ERROR_FEATURE_NOT_SUPPORTED);
      return(ST_ERROR_FEATURE_NOT_SUPPORTED);
  }
  STOS_SemaphoreWait(stlayer_context.XContext[LayerHandle].AccessProtect_p);
  ((stlayer_ViewPortDescriptor_t*)VPHandle)->ColorKeyValue = *ColorKey;
  stlayer_UpdateNodeGeneric((stlayer_ViewPortHandle_t)VPHandle);
  STOS_SemaphoreSignal(stlayer_context.XContext[LayerHandle].AccessProtect_p);
  return(ST_NO_ERROR);
}




/*******************************************************************************
Name        : LAYERGFX_GetViewPortColorKey
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYERGFX_GetViewPortColorKey(STLAYER_ViewPortHandle_t VPHandle,
                                            STGXOBJ_ColorKey_t *     ColorKey)
{
  stlayer_ViewPortHandle_t    VPHandleCasted;
  STLAYER_Handle_t            LayerHandle;

  /* error checking: is the VP handle valid?*/
  if(VPHandle == 0)
  {
      layergfx_Defense(ST_ERROR_INVALID_HANDLE);
      return(ST_ERROR_INVALID_HANDLE);
  }
  if(ColorKey == 0)
  {
      layergfx_Defense(ST_ERROR_BAD_PARAMETER);
      return(ST_ERROR_BAD_PARAMETER);
  }
  VPHandleCasted = (stlayer_ViewPortHandle_t)(VPHandle);
  LayerHandle    = VPHandleCasted->LayerHandle;
  STOS_SemaphoreWait(stlayer_context.XContext[LayerHandle].AccessProtect_p);
  * ColorKey = VPHandleCasted->ColorKeyValue;
  STOS_SemaphoreSignal(stlayer_context.XContext[LayerHandle].AccessProtect_p);
  return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : LAYERGFX_DisableBorderAlpha
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYERGFX_DisableBorderAlpha(STLAYER_ViewPortHandle_t VPHandle)
{
  U32  LayerHandle;
  /* error checking: is the VP handle valid?*/
  if(VPHandle == 0)
  {
      layergfx_Defense(ST_ERROR_INVALID_HANDLE);
      return(ST_ERROR_INVALID_HANDLE);
  }
  LayerHandle = ((stlayer_ViewPortDescriptor_t*)VPHandle)->LayerHandle;
  if (stlayer_context.XContext[LayerHandle].LayerType == STLAYER_GAMMA_CURSOR )
  {
      layergfx_Defense(ST_ERROR_FEATURE_NOT_SUPPORTED);
      return(ST_ERROR_FEATURE_NOT_SUPPORTED);
  }
  STOS_SemaphoreWait(stlayer_context.XContext[LayerHandle].AccessProtect_p);
  ((stlayer_ViewPortDescriptor_t*)VPHandle)->BorderAlphaOn = FALSE;
  stlayer_UpdateNodeGeneric((stlayer_ViewPortHandle_t)VPHandle);
  STOS_SemaphoreSignal(stlayer_context.XContext[LayerHandle].AccessProtect_p);
  return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : LAYERGFX_EnableBorderAlpha
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYERGFX_EnableBorderAlpha(STLAYER_ViewPortHandle_t VPHandle)
{
  U32  LayerHandle;
  /* error checking: is the VP handle valid?*/
  if(VPHandle == 0)
  {
      layergfx_Defense(ST_ERROR_INVALID_HANDLE);
      return(ST_ERROR_INVALID_HANDLE);
  }
  LayerHandle = ((stlayer_ViewPortDescriptor_t*)VPHandle)->LayerHandle;
  if (stlayer_context.XContext[LayerHandle].LayerType == STLAYER_GAMMA_CURSOR )
  {
      layergfx_Defense(ST_ERROR_FEATURE_NOT_SUPPORTED);
      return(ST_ERROR_FEATURE_NOT_SUPPORTED);
  }
  STOS_SemaphoreWait(stlayer_context.XContext[LayerHandle].AccessProtect_p);
  ((stlayer_ViewPortDescriptor_t*)VPHandle)->BorderAlphaOn = TRUE;
  stlayer_UpdateNodeGeneric((stlayer_ViewPortHandle_t)VPHandle);
  STOS_SemaphoreSignal(stlayer_context.XContext[LayerHandle].AccessProtect_p);
  return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : LAYERGFX_GetViewPortAlpha
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYERGFX_GetViewPortAlpha(STLAYER_ViewPortHandle_t VPHandle,
                                        STLAYER_GlobalAlpha_t * Alpha)
{
    stlayer_ViewPortHandle_t    VPHandleCasted;
    STLAYER_Handle_t            LayerHandle;
    /* error checking: is the VP handle valid?*/
    if(VPHandle == 0)
    {
      layergfx_Defense(ST_ERROR_INVALID_HANDLE);
        return(ST_ERROR_INVALID_HANDLE);
    }
    if(Alpha == 0)
    {
      layergfx_Defense(ST_ERROR_BAD_PARAMETER);
        return(ST_ERROR_BAD_PARAMETER);
    }
    VPHandleCasted = (stlayer_ViewPortHandle_t)(VPHandle);
    LayerHandle = ((stlayer_ViewPortDescriptor_t*)VPHandle)->LayerHandle;
    STOS_SemaphoreWait(stlayer_context.XContext[LayerHandle].AccessProtect_p);
    * Alpha = VPHandleCasted->Alpha;
    STOS_SemaphoreSignal(stlayer_context.XContext[LayerHandle].AccessProtect_p);
    return(ST_NO_ERROR);
}
/*******************************************************************************
Name        : LAYERGFX_SetViewPortAlpha
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYERGFX_SetViewPortAlpha(STLAYER_ViewPortHandle_t VPHandle,
                                        STLAYER_GlobalAlpha_t *     Alpha)
{
    U32  LayerHandle;
    /* error checking: is the VP handle valid?*/
    if(VPHandle == 0)
    {
        layergfx_Defense(ST_ERROR_INVALID_HANDLE);
        return(ST_ERROR_INVALID_HANDLE);
    }
    LayerHandle = ((stlayer_ViewPortDescriptor_t*)VPHandle)->LayerHandle;
    if (stlayer_context.XContext[LayerHandle].LayerType == STLAYER_GAMMA_CURSOR)
    {
        layergfx_Defense(ST_ERROR_FEATURE_NOT_SUPPORTED);
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }
    if(Alpha == 0)
    {
        layergfx_Defense(ST_ERROR_BAD_PARAMETER);
        return(ST_ERROR_BAD_PARAMETER);
    }
    if ((Alpha->A0 > 128) || (Alpha->A1 > 128))
    {
        layergfx_Defense(ST_ERROR_BAD_PARAMETER);
        return(ST_ERROR_BAD_PARAMETER);
    }
    STOS_SemaphoreWait(stlayer_context.XContext[LayerHandle].AccessProtect_p);
    ((stlayer_ViewPortDescriptor_t*)VPHandle)->Alpha = *Alpha;
    stlayer_UpdateNodeGeneric((stlayer_ViewPortHandle_t)VPHandle);
    STOS_SemaphoreSignal(stlayer_context.XContext[LayerHandle].AccessProtect_p);
    return(ST_NO_ERROR);
}
/*******************************************************************************
Name        : LAYERGFX_GetViewPortRecordable
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYERGFX_GetViewPortRecordable(STLAYER_ViewPortHandle_t VPHandle,
                                              BOOL * Recordable_p)
{
    stlayer_ViewPortHandle_t    VPHandleCasted;
    STLAYER_Handle_t            LayerHandle;
    /* error checking: is the VP handle valid?*/
    if(VPHandle == 0)
    {
      layergfx_Defense(ST_ERROR_INVALID_HANDLE);
        return(ST_ERROR_INVALID_HANDLE);
    }
    if(Recordable_p == 0)
    {
      layergfx_Defense(ST_ERROR_BAD_PARAMETER);
        return(ST_ERROR_BAD_PARAMETER);
    }
    VPHandleCasted = (stlayer_ViewPortHandle_t)(VPHandle);
    LayerHandle = ((stlayer_ViewPortDescriptor_t*)VPHandle)->LayerHandle;
    STOS_SemaphoreWait(stlayer_context.XContext[LayerHandle].AccessProtect_p);
    if(VPHandleCasted->HideOnMix2)
    {
        *Recordable_p = FALSE;
    }
    else
    {
        *Recordable_p = TRUE;
    }
    STOS_SemaphoreSignal(stlayer_context.XContext[LayerHandle].AccessProtect_p);
    return(ST_NO_ERROR);
}
/*******************************************************************************
Name        : LAYERGFX_SetViewPortRecordable
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYERGFX_SetViewPortRecordable(STLAYER_ViewPortHandle_t VPHandle,
                                              BOOL Recordable)
{
    U32  LayerHandle;
    /* error checking: is the VP handle valid?*/
    if(VPHandle == 0)
    {
        layergfx_Defense(ST_ERROR_INVALID_HANDLE);
        return(ST_ERROR_INVALID_HANDLE);
    }
    LayerHandle = ((stlayer_ViewPortDescriptor_t*)VPHandle)->LayerHandle;
    if (stlayer_context.XContext[LayerHandle].LayerType == STLAYER_GAMMA_CURSOR)
    {
        layergfx_Defense(ST_ERROR_FEATURE_NOT_SUPPORTED);
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }
    STOS_SemaphoreWait(stlayer_context.XContext[LayerHandle].AccessProtect_p);
    if (!(Recordable))
    {
        ((stlayer_ViewPortDescriptor_t*)VPHandle)->HideOnMix2 = TRUE;
    }
    else
    {
        ((stlayer_ViewPortDescriptor_t*)VPHandle)->HideOnMix2 = FALSE;
    }
    stlayer_UpdateNodeGeneric((stlayer_ViewPortHandle_t)VPHandle);
    STOS_SemaphoreSignal(stlayer_context.XContext[LayerHandle].AccessProtect_p);
    return(ST_NO_ERROR);
}


/*******************************************************************************
Name        : LAYERGFX_SetViewPortGain
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYERGFX_SetViewPortGain(STLAYER_ViewPortHandle_t VPHandle,
                                       STLAYER_GainParams_t *   Params)
{
    U32  LayerHandle;
    /* error checking: is the VP handle valid?*/
    if(VPHandle == 0)
    {
        layergfx_Defense(ST_ERROR_INVALID_HANDLE);
        return(ST_ERROR_INVALID_HANDLE);
    }
    LayerHandle = ((stlayer_ViewPortDescriptor_t*)VPHandle)->LayerHandle;
    if (stlayer_context.XContext[LayerHandle].LayerType == STLAYER_GAMMA_CURSOR)
    {
        layergfx_Defense(ST_ERROR_FEATURE_NOT_SUPPORTED);
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }
    if(Params == 0)
    {
        layergfx_Defense(ST_ERROR_BAD_PARAMETER);
        return(ST_ERROR_BAD_PARAMETER);
    }
    /*if((Params->BlackLevel > 128) || (Params->GainLevel > 128))
    {
        layergfx_Defense(ST_ERROR_BAD_PARAMETER);
        return(ST_ERROR_BAD_PARAMETER);
    }*/
    STOS_SemaphoreWait(stlayer_context.XContext[LayerHandle].AccessProtect_p);
    ((stlayer_ViewPortDescriptor_t*)VPHandle)->Gain = *Params ;
    stlayer_UpdateNodeGeneric((stlayer_ViewPortHandle_t)VPHandle);
    STOS_SemaphoreSignal(stlayer_context.XContext[LayerHandle].AccessProtect_p);
    return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : LAYERGFX_GetViewPortGain
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYERGFX_GetViewPortGain(STLAYER_ViewPortHandle_t VPHandle,
                                       STLAYER_GainParams_t *      Params)
{
    stlayer_ViewPortHandle_t    VPHandleCasted;
    STLAYER_Handle_t            LayerHandle;
    /* error checking: is the VP handle valid?*/
    if(VPHandle == 0)
    {
      layergfx_Defense(ST_ERROR_INVALID_HANDLE);
        return(ST_ERROR_INVALID_HANDLE);
    }
    if(Params == 0)
    {
      layergfx_Defense(ST_ERROR_BAD_PARAMETER);
        return(ST_ERROR_BAD_PARAMETER);
    }
    VPHandleCasted = (stlayer_ViewPortHandle_t)(VPHandle);
    LayerHandle = ((stlayer_ViewPortDescriptor_t*)VPHandle)->LayerHandle;
    STOS_SemaphoreWait(stlayer_context.XContext[LayerHandle].AccessProtect_p);
    *Params = VPHandleCasted->Gain;
    STOS_SemaphoreSignal(stlayer_context.XContext[LayerHandle].AccessProtect_p);
    return(ST_NO_ERROR);
}


/*******************************************************************************
Name        : LAYERGFX_SetViewPortParams
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYERGFX_SetViewPortParams(STLAYER_ViewPortHandle_t VPHandle,
                                      STLAYER_ViewPortParams_t*       Params_p)
{
    ST_ErrorCode_t           Error;
    STGXOBJ_Rectangle_t      * InputRectangle;
    STGXOBJ_Rectangle_t      * OutputRectangle;
    STGXOBJ_Rectangle_t       ClippedIn;
    STGXOBJ_Rectangle_t       ClippedOut;
    STLAYER_ViewPortSource_t * Source_p;
    stlayer_ViewPortHandle_t    VPHandleCasted;
    STLAYER_Handle_t            LayerHandle;
    BOOL                        TotalClipped;

    #if defined (ST_7109)
        U32 CutId;
    #endif



    if(Params_p == 0)
    {
        layergfx_Defense(ST_ERROR_BAD_PARAMETER);
        return(ST_ERROR_BAD_PARAMETER);
    }

   #if defined (ST_7109)
    CutId = ST_GetCutRevision();
   #endif


       /* test the new params */
    InputRectangle  = &(Params_p->InputRectangle);
    OutputRectangle = &(Params_p->OutputRectangle);

 #if defined (ST_7109) || defined (ST_7710) || defined (ST_7100)

      /* Test Vertical Resize and Horizontal Downsize */

  #if defined (ST_7109)

  if (CutId < 0xC0)
    {
  #endif


    if(OutputRectangle->Height != InputRectangle->Height)
    {

        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    if(OutputRectangle->Width < InputRectangle->Width)
    {

        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

  #if defined (ST_7109)

   }
   else
   {
#if 0
        if((OutputRectangle->Width*2) < InputRectangle->Width)
        {

            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
        }
#endif

        if((((stlayer_ViewPortDescriptor_t*)VPHandle)->FFilterEnabled == TRUE) && (OutputRectangle->Height != OutputRectangle->Height))
        {
                      return(ST_ERROR_FEATURE_NOT_SUPPORTED);
         }

   }
  #endif


#endif

  #if defined (ST_7200)

	if((((stlayer_ViewPortDescriptor_t*)VPHandle)->FFilterEnabled == TRUE) && (OutputRectangle->Height != OutputRectangle->Height))
	{
       return(ST_ERROR_FEATURE_NOT_SUPPORTED);
	 }
   #endif



    Source_p        = Params_p->Source_p;
    VPHandleCasted  = (stlayer_ViewPortHandle_t)(VPHandle);
    LayerHandle     =  VPHandleCasted->LayerHandle;
    STOS_SemaphoreWait(stlayer_context.XContext[LayerHandle].AccessProtect_p);

    if(Source_p->Data.BitMap_p == 0)
    {
        STOS_SemaphoreSignal(stlayer_context.XContext[LayerHandle].AccessProtect_p);
        layergfx_Defense(ST_ERROR_BAD_PARAMETER);
        return(ST_ERROR_BAD_PARAMETER);
    }
    Error =stlayer_TestViewPortParam(LayerHandle,InputRectangle,OutputRectangle,
                                     &ClippedIn,&ClippedOut,&TotalClipped,
                                     Source_p->Data.BitMap_p);
    if (Error != ST_NO_ERROR)
    {
        STOS_SemaphoreSignal(stlayer_context.XContext[LayerHandle].AccessProtect_p);
        layergfx_Defense(Error);
        return(Error);
    }
    /* Set the source without test i/o rectangles */
    if(stlayer_context.XContext[LayerHandle].LayerType == STLAYER_GAMMA_CURSOR)
    {
        if(Source_p->Palette_p == 0)
        {
            STOS_SemaphoreSignal(stlayer_context.XContext[LayerHandle].
                                                AccessProtect_p);
            layergfx_Defense(ST_ERROR_BAD_PARAMETER);
            return(ST_ERROR_BAD_PARAMETER);
        }
        STOS_memcpy(&VPHandleCasted->Palette, Source_p->Palette_p, sizeof(STGXOBJ_Palette_t));
    }
    STOS_memcpy(&VPHandleCasted->Bitmap, Source_p->Data.BitMap_p, sizeof(STGXOBJ_Bitmap_t));

    STOS_SemaphoreSignal(stlayer_context.XContext[LayerHandle].AccessProtect_p);
    /* The set the i/o rectangles */
    LAYERGFX_SetViewPortIORectangle(VPHandle,InputRectangle,OutputRectangle);
    return(ST_NO_ERROR);
}
/*******************************************************************************
Name        : LAYERGFX_GetViewPortParams
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYERGFX_GetViewPortParams(STLAYER_ViewPortHandle_t VPHandle,
                                      STLAYER_ViewPortParams_t*       Params_p)
{
    ST_ErrorCode_t Error;

    if(Params_p == 0)
    {
        layergfx_Defense(ST_ERROR_BAD_PARAMETER);
        return(ST_ERROR_BAD_PARAMETER);
    }
    Error = LAYERGFX_GetViewPortSource(VPHandle,Params_p->Source_p);
    if (Error != ST_NO_ERROR)
    {
        return(Error);
    }
    else
    {
        Error = LAYERGFX_GetViewPortIORectangle(VPHandle,
                                                &(Params_p->InputRectangle),
                                                &(Params_p->OutputRectangle));
    }
    return(Error);
}
/*******************************************************************************
Name        : LAYERGFX_AdjustViewPortParams
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYERGFX_AdjustViewPortParams(STLAYER_Handle_t Handle,
                                            STLAYER_ViewPortParams_t * Params_p)
{
    ST_ErrorCode_t              ErrorSource, ErrorRect;
    STGXOBJ_Bitmap_t            *BitMap_p;
    STGXOBJ_Rectangle_t         InputRectangle;
    STGXOBJ_Rectangle_t         OutputRectangle;

    BitMap_p        = Params_p->Source_p->Data.BitMap_p;
    STOS_memcpy(&InputRectangle, &Params_p->InputRectangle, sizeof(STGXOBJ_Rectangle_t));
    STOS_memcpy(&OutputRectangle, &Params_p->OutputRectangle, sizeof(STGXOBJ_Rectangle_t));

    ErrorSource = ST_NO_ERROR;
    /* Selection test */
    if (InputRectangle.PositionX  > (S32)(BitMap_p->Width))
    {
        return(STLAYER_ERROR_IORECTANGLES_NOT_ADJUSTABLE);
    }
    if (InputRectangle.PositionY > (S32)(BitMap_p->Height))
    {
        return(STLAYER_ERROR_IORECTANGLES_NOT_ADJUSTABLE);
    }

    /* Adjust to the source */
    if (InputRectangle.PositionX + InputRectangle.Width > (BitMap_p->Width))
    {
        InputRectangle.Width = BitMap_p->Width - InputRectangle.PositionX;
        ErrorSource = STLAYER_SUCCESS_IORECTANGLES_ADJUSTED;
    }
    if (InputRectangle.PositionY + InputRectangle.Height > (BitMap_p->Height))
    {
        InputRectangle.Height = BitMap_p->Height - InputRectangle.PositionY;
        ErrorSource = STLAYER_SUCCESS_IORECTANGLES_ADJUSTED;
    }
    /* Adjust the io rectangles */
    ErrorRect = LAYERGFX_AdjustIORectangle(Handle,&InputRectangle,
                                                    &OutputRectangle);
    if ((ErrorSource == STLAYER_ERROR_IORECTANGLES_NOT_ADJUSTABLE)
            || (ErrorRect == STLAYER_ERROR_IORECTANGLES_NOT_ADJUSTABLE))
    {
        layergfx_Defense(STLAYER_ERROR_IORECTANGLES_NOT_ADJUSTABLE);
        return(STLAYER_ERROR_IORECTANGLES_NOT_ADJUSTABLE);
    }

    STOS_memcpy(&Params_p->InputRectangle, &InputRectangle, sizeof(STGXOBJ_Rectangle_t));
    STOS_memcpy(&Params_p->OutputRectangle, &OutputRectangle, sizeof(STGXOBJ_Rectangle_t));
    return(ErrorRect | ErrorSource);
}
/*******************************************************************************
Name        : LAYERGFX_AttachViewPort
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYERGFX_AttachViewPort(STLAYER_ViewPortHandle_t VPHandle,
                                       U32 Ident)
{
    U32  LayerHandle;
    /* error checking: is the VP handle valid?*/
    if(VPHandle == 0)
    {
        layergfx_Defense(ST_ERROR_INVALID_HANDLE);
        return(ST_ERROR_INVALID_HANDLE);
    }
    LayerHandle = ((stlayer_ViewPortDescriptor_t*)VPHandle)->LayerHandle;
    if (stlayer_context.XContext[LayerHandle].LayerType == STLAYER_GAMMA_CURSOR)
    {
        layergfx_Defense(ST_ERROR_FEATURE_NOT_SUPPORTED);
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }
    STOS_SemaphoreWait(stlayer_context.XContext[LayerHandle].AccessProtect_p);
    ((stlayer_ViewPortDescriptor_t*)VPHandle)->AssociatedToMainLayer = Ident;
    stlayer_UpdateNodeGeneric((stlayer_ViewPortHandle_t)VPHandle);
    STOS_SemaphoreSignal(stlayer_context.XContext[LayerHandle].AccessProtect_p);
    return(ST_NO_ERROR);
}


/*******************************************************************************
Name        : stlayer_TestViewPortParam
Description : Check validity and perform clipping
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t stlayer_TestViewPortParam(STLAYER_Handle_t     LayerHandle,
                                        STGXOBJ_Rectangle_t * InputRect_p,
                                        STGXOBJ_Rectangle_t * OutputRect_p,
                                        STGXOBJ_Rectangle_t * ClippedIn_p,
                                        STGXOBJ_Rectangle_t * ClippedOut_p,
                                        BOOL                * TotalClipped,
                                        STGXOBJ_Bitmap_t    * BitMap_p)
{
    ST_ErrorCode_t Error;
    S32 ClipedInStart;
    S32 ClipedInStop;
    S32 ClipedOutStart;
    S32 ClipedOutStop;
    U32 Resize;
    U32 StartTrunc,StopTrunc;
    S32 InStop, OutStop;

    #if defined (ST_7109)
        U32 CutId;
    #endif


    Error = ST_NO_ERROR;
    * TotalClipped = FALSE;


   #if defined (ST_7109)
    CutId = ST_GetCutRevision();
   #endif





    /* Test bitmap data storage */

    /* test the color type and the resize */
    switch(stlayer_context.XContext[LayerHandle].LayerType)
    {
        case STLAYER_GAMMA_GDP:


            #if defined (ST_7710) || defined (ST_7100)
                if (InputRect_p->Height != OutputRect_p->Height)
                {
                    layergfx_Trace("STLAYER - GFX :  Non Supported Vertical Resize");
                    Error = ST_ERROR_BAD_PARAMETER;
                }
            #endif
#if defined (ST_7109) || defined (ST_7100) || defined (ST_7710)  || defined (ST_7200)
        if( (BitMap_p->ColorSpaceConversion != STGXOBJ_ITU_R_BT601)&&
            (BitMap_p->ColorSpaceConversion != STGXOBJ_ITU_R_BT709))
            {
                      Error = ST_ERROR_BAD_PARAMETER;
             }

#endif

        case STLAYER_GAMMA_FILTER:
        if( (BitMap_p->ColorType != STGXOBJ_COLOR_TYPE_RGB565)&&
            (BitMap_p->ColorType != STGXOBJ_COLOR_TYPE_RGB888)&&
            (BitMap_p->ColorType != STGXOBJ_COLOR_TYPE_ARGB8565)&&
            (BitMap_p->ColorType != STGXOBJ_COLOR_TYPE_ARGB8565_255)&&
            (BitMap_p->ColorType != STGXOBJ_COLOR_TYPE_ARGB8888)&&
            (BitMap_p->ColorType != STGXOBJ_COLOR_TYPE_ARGB8888_255)&&
            (BitMap_p->ColorType != STGXOBJ_COLOR_TYPE_ARGB1555)&&
            (BitMap_p->ColorType != STGXOBJ_COLOR_TYPE_ARGB4444)&&
            (BitMap_p->ColorType != STGXOBJ_COLOR_TYPE_SIGNED_AYCBCR8888)&&
            (BitMap_p->ColorType != STGXOBJ_COLOR_TYPE_UNSIGNED_AYCBCR8888)&&
            (BitMap_p->ColorType != STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_444)&&
            (BitMap_p->ColorType != STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_444)&&
            (BitMap_p->ColorType != STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_422)&&
            (BitMap_p->ColorType != STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422))
        {
            layergfx_Trace("STLAYER - GFX :  Non Supported Color Type");


  	 #if defined (ST_7109) || defined (ST_7200)

		#if defined (ST_7109)

		if (CutId >= 0xC0)
		{
		#endif
                        if (BitMap_p->ColorType != STGXOBJ_COLOR_TYPE_CLUT8)
                        {
                            Error = ST_ERROR_BAD_PARAMETER;
                        }
         #if defined (ST_7109)
		 }
         else
         {
                         Error = ST_ERROR_BAD_PARAMETER;
          }
        #endif


      #else

            Error = ST_ERROR_BAD_PARAMETER;


      #endif

        }


        break;

        case STLAYER_GAMMA_CURSOR:
        if(BitMap_p->ColorType != STGXOBJ_COLOR_TYPE_CLUT8)
        {
            layergfx_Trace("STLAYER - GFX :  Non Supported Color Type");
            Error = ST_ERROR_BAD_PARAMETER;
        }
        if ((InputRect_p->Height != OutputRect_p->Height)
          ||(InputRect_p->Width  != OutputRect_p->Width))
        {
            layergfx_Trace("STLAYER - GFX :  Non Supported Resize");
            Error = ST_ERROR_BAD_PARAMETER;
        }
        break;

        case STLAYER_GAMMA_ALPHA:
        if((BitMap_p->ColorType != STGXOBJ_COLOR_TYPE_ALPHA1) &&
           (BitMap_p->ColorType != STGXOBJ_COLOR_TYPE_ALPHA8) &&
           (BitMap_p->ColorType != STGXOBJ_COLOR_TYPE_ALPHA8_255))
        {
            layergfx_Trace("STLAYER - GFX :  Non Supported Color Type");
            Error = ST_ERROR_BAD_PARAMETER;
        }
        break;

        case STLAYER_GAMMA_BKL:
        if( (BitMap_p->ColorType != STGXOBJ_COLOR_TYPE_RGB565)&&
            (BitMap_p->ColorType != STGXOBJ_COLOR_TYPE_ARGB1555)&&
            (BitMap_p->ColorType != STGXOBJ_COLOR_TYPE_ARGB4444)&&
            (BitMap_p->ColorType != STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_422 )&&
            (BitMap_p->ColorType != STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422 ))
        {
            layergfx_Trace("STLAYER - GFX :  Non Supported Color Type");
            Error = ST_ERROR_BAD_PARAMETER;
        }
        if (InputRect_p->Height != OutputRect_p->Height)
        {
            layergfx_Trace("STLAYER - GFX :  Non Supported Resize");
            Error = ST_ERROR_BAD_PARAMETER;
        }
#if defined WA_GNBvd06281
        if(InputRect_p->Width  != OutputRect_p->Width)
        {
            layergfx_Trace("STLAYER - GFX :  Non Supported Resize");
            Error = ST_ERROR_BAD_PARAMETER;
        }
#endif
#if defined WA_GNBvd06293 || defined WA_GNBvd06299
        if(InputRect_p->Height < 2)
        {
            layergfx_Trace("STLAYER - GFX :  Non Supported height");
            Error = ST_ERROR_BAD_PARAMETER;
        }
#endif

        break;
        default:
        /* Req for no warning */
        break;
    }

#if defined (NOT_DEFINED) /* for vin/video : don't test bitmap add */
    if(!STAVMEM_IsAddressVirtual(BitMap_p->Data1_p,
                &stlayer_context.XContext[LayerHandle].VirtualMapping))
    {
        layergfx_Trace("STLAYER - GFX :  Bitmap data is not in add range");
        Error = ST_ERROR_BAD_PARAMETER;
    }
#endif

    /* Clipping Horizontal Computing */
    /*-------------------------------*/
    InStop  = InputRect_p->PositionX + InputRect_p->Width;
    OutStop = OutputRect_p->PositionX + OutputRect_p->Width;
    Resize  =  256 * OutputRect_p->Width / InputRect_p->Width;
    /* (factor 256 to keep precision in integer div) */

    /* easy clipping in bitmap selection */
    /*-----------------------------------*/
    if((InStop < 0 ) || (InputRect_p->PositionX > (S32)BitMap_p->Width))
    {
        * TotalClipped = TRUE;
    }
    if(InputRect_p->PositionX  < 0 )
    {
        StartTrunc = -InputRect_p->PositionX;
        ClipedInStart = 0;
    }
    else
    {
        StartTrunc = 0;
        ClipedInStart = InputRect_p->PositionX;
    }
    if(InStop > (S32)(BitMap_p->Width))
    {
        StopTrunc = InStop - BitMap_p->Width;
        ClipedInStop = BitMap_p->Width;
    }
    else
    {
        StopTrunc = 0;
        ClipedInStop = InStop;
    }
    /* screen feedback clipping */
    /*--------------------------*/
    ClipedOutStart = OutputRect_p->PositionX + (StartTrunc * Resize / 256);
    ClipedOutStop = OutStop - (StopTrunc * Resize / 256);

    /* easy clipping in output selection */
    /*-----------------------------------*/
    if((OutStop < 0) || (OutputRect_p->PositionX
           > (S32)stlayer_context.XContext[LayerHandle].LayerParams.Width))
    {
        * TotalClipped = TRUE;
    }
    if(ClipedOutStart < 0)
    {
        StartTrunc = -ClipedOutStart;
        ClipedOutStart = 0;
    }
    else
    {
        StartTrunc = 0;
    }
    if(ClipedOutStop >
            (S32)(stlayer_context.XContext[LayerHandle].LayerParams.Width))
    {
        StopTrunc = ClipedOutStop -
            stlayer_context.XContext[LayerHandle].LayerParams.Width;
        ClipedOutStop =
            stlayer_context.XContext[LayerHandle].LayerParams.Width;
    }
    else
    {
        StopTrunc = 0;
    }
    /* bitmap feedback clipping */
    /*--------------------------*/
    ClipedInStart = ClipedInStart + (256 * StartTrunc / Resize);
    ClipedInStop = ClipedInStop - (256 * StopTrunc / Resize);

    ClippedIn_p->PositionX  = ClipedInStart;
    ClippedIn_p->Width      = ClipedInStop - ClipedInStart;
    ClippedOut_p->PositionX = ClipedOutStart;
    ClippedOut_p->Width     = ClipedOutStop - ClipedOutStart;

    /* Clipping Vertical Computing */
    /*-----------------------------*/

    InStop  = InputRect_p->PositionY + InputRect_p->Height;
    OutStop = OutputRect_p->PositionY + OutputRect_p->Height;
    Resize =  256 * OutputRect_p->Height / InputRect_p->Height;

    /* easy clipping in bitmap selection */
    /*-----------------------------------*/
    if((InStop < 0 ) || (InputRect_p->PositionY > (S32)(BitMap_p->Height)))
    {
        * TotalClipped = TRUE;
    }
    if(InputRect_p->PositionY  < 0 )
    {
        StartTrunc = -InputRect_p->PositionY;
        ClipedInStart = 0;
    }
    else
    {
        StartTrunc = 0;
        ClipedInStart = InputRect_p->PositionY;
    }
    if(InStop > (S32)(BitMap_p->Height))
    {
        StopTrunc = InStop - BitMap_p->Height;
        ClipedInStop = BitMap_p->Height;
    }
    else
    {
        StopTrunc = 0;
        ClipedInStop = InStop;
    }
    /* screen feedback clipping */
    /*--------------------------*/
    ClipedOutStart = OutputRect_p->PositionY + (StartTrunc * Resize / 256 );
    ClipedOutStop = OutStop - (StopTrunc * Resize / 256);

    /* easy clipping in output selection */
    /*-----------------------------------*/
    if((OutStop < 0) || (OutputRect_p->PositionY
           > (S32)stlayer_context.XContext[LayerHandle].LayerParams.Height))
    {
        * TotalClipped = TRUE;
    }
    if(ClipedOutStart < 0)
    {
        StartTrunc = -ClipedOutStart;
        ClipedOutStart = 0;
    }
    else
    {
        StartTrunc = 0;
    }
    if(ClipedOutStop >
            (S32)(stlayer_context.XContext[LayerHandle].LayerParams.Height))
    {
        StopTrunc = ClipedOutStop -
            stlayer_context.XContext[LayerHandle].LayerParams.Height;
        ClipedOutStop =
            stlayer_context.XContext[LayerHandle].LayerParams.Height;
    }
    else
    {
        StopTrunc = 0;
    }
    /* bitmap feedback clipping */
    /*--------------------------*/
    ClipedInStart = ClipedInStart + (256 * StartTrunc / Resize );
    ClipedInStop = ClipedInStop - (256 * StopTrunc / Resize );

    ClippedIn_p->PositionY  = ClipedInStart;
    ClippedIn_p->Height     = ClipedInStop - ClipedInStart;
    ClippedOut_p->PositionY = ClipedOutStart;
    ClippedOut_p->Height    = ClipedOutStop - ClipedOutStart;

    /* WA 7020 */
    ClippedIn_p->PositionX &= ~((U32)0x00000001);

    if(  (ClippedOut_p->Height <= 1)
       ||(ClippedOut_p->Width  <= 1)
       ||(ClippedIn_p->Height  <= 1)
       ||(ClippedIn_p->Width   <= 1))
    {
        *TotalClipped = TRUE;
    }
        /* Ignore Clipping test for GDPVBIViewport   */
    if (stlayer_context.XContext[LayerHandle].LayerType==STLAYER_GAMMA_GDPVBI)
            {
                STOS_memcpy(ClippedOut_p, OutputRect_p, sizeof(STGXOBJ_Rectangle_t));
                STOS_memcpy(ClippedIn_p, InputRect_p, sizeof(STGXOBJ_Rectangle_t));
                *TotalClipped = FALSE;
             }



    if (Error != ST_NO_ERROR)
    {
        layergfx_Defense(Error);
    }
    return(Error);
}

/*  end of front_vp.c */
