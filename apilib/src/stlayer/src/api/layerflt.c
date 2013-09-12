/*******************************************************************************

File name   : layerflt.c

Description : STLAYER API: filter driver on top layer driver

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                               Name
----               ------------                               ----
2002-05-02         Created                                    Michel Bruant
*******************************************************************************/

/* Includes ----------------------------------------------------------------- */

#include "stlayer.h"
#include "layerapi.h"

/* Public Functions --------------------------------------------------------- */

#ifdef GFX_GAMMA

#include "layergfx.h"

/*******************************************************************************
Name        : stlayer_EnableViewPortFilter
Description : !!! Defined only if GFX_GAMMA is defined (7015, 7020, GX1)
Parameters  :
Assumptions :
Limitations : not exported through API
Returns     :
*******************************************************************************/
ST_ErrorCode_t stlayer_EnableViewPortFilter(STLAYER_ViewPortHandle_t VPHandle,
                                                STLAYER_Handle_t FilterHandle)
{
    ST_ErrorCode_t              ErrorCode;
    STLAYER_ViewPortParams_t    ViewPortParams;
    STLAYER_ViewPortSource_t    Source;
    STGXOBJ_Bitmap_t            BitMap;
    STGXOBJ_Palette_t           Palette;
    STLAYER_ViewPortHandle_t    FilterVPHandle;

     #if defined (ST_7109)
    U32 CutId;
    #endif


    /* Get the viewport parameters */
    ViewPortParams.Source_p = &Source;
    Source.Data.BitMap_p    = &BitMap;
    Source.Palette_p        = &Palette;

   #if defined (ST_7109)
   CutId = ST_GetCutRevision();
   #endif

  #if defined (ST_7109) || defined (ST_7200)

   #if defined (ST_7109)

   if (CutId >= 0xC0)
    {
   #endif

        ErrorCode = LAYERGFX_EnableFlickerFilter(VPHandle);
        return(ErrorCode);

    #if defined (ST_7109)
    }
    #endif


#endif



   ErrorCode = STLAYER_GetViewPortParams(VPHandle,&ViewPortParams);
    if(ErrorCode != ST_NO_ERROR)
    {
        return(ErrorCode);
    }



    /* Open a viewport on the filter layer with equal parameters */
    ErrorCode = STLAYER_OpenViewPort((STLAYER_Handle_t)FilterHandle,
                                      &ViewPortParams,
                                      &FilterVPHandle);
    if(ErrorCode != ST_NO_ERROR)
    {
        return(ErrorCode);
    }

    /* Attach the filter VP to the filtered VP */
    ErrorCode = stlayer_AttachFilterViewPort(FilterVPHandle,VPHandle);
    if(ErrorCode != ST_NO_ERROR)
    {
        STLAYER_CloseViewPort(FilterVPHandle);
        return(ErrorCode);
    }

    /* Enable the filter-viewport */
    ErrorCode = STLAYER_EnableViewPort(FilterVPHandle);
    if(ErrorCode != ST_NO_ERROR)
    {
        STLAYER_CloseViewPort(FilterVPHandle);
        return(ErrorCode);
    }

    /* record the user-viewport as 'flitered' */
    stlayer_SetVPFilter(VPHandle,FilterVPHandle);

    return(ST_NO_ERROR);
}
/*******************************************************************************
Name        : stlayer_DisableViewPortFilter
Description : !!! Defined only if GFX_GAMMA is defined (7015, 7020, GX1)
Parameters  :
Assumptions :
Limitations : not exported through API
Returns     :
*******************************************************************************/
ST_ErrorCode_t stlayer_DisableViewPortFilter(STLAYER_ViewPortHandle_t VPHandle)
{
    ST_ErrorCode_t              ErrorCode;
    STLAYER_ViewPortHandle_t    FilterVPHandle;


    #if defined (ST_7109)
    U32 CutId;
    #endif


   #if defined (ST_7109)
    CutId = ST_GetCutRevision();
   #endif


#if defined (ST_7109) || defined (ST_7200)

  #if defined (ST_7109)

   if (CutId >= 0xC0)
    {
   #endif

        ErrorCode = LAYERGFX_DisableFlickerFilter(VPHandle);
        return(ErrorCode);

  #if defined (ST_7109)
    }
  #endif


#endif


    FilterVPHandle = stlayer_GetVPFilter(VPHandle);

    /* Disable the filter-viewport */
    ErrorCode = STLAYER_DisableViewPort(FilterVPHandle);
    if(ErrorCode != ST_NO_ERROR)
    {
        return(ErrorCode);
    }

    /* Close the filter-viewport */
    ErrorCode = STLAYER_CloseViewPort(FilterVPHandle);
    if(ErrorCode != ST_NO_ERROR)
    {
        return(ErrorCode);
    }

    /* record the user-viewport as 'NO flitered' */
    stlayer_SetVPFilter(VPHandle,0);

    return(ST_NO_ERROR);
}
#endif /* GFX_GAMMA */

/* end of layerflt.c */
/******************************************************************************/

