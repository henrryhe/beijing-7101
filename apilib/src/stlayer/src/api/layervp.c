/*******************************************************************************

File name   : layervp.c

Description : STLAYER API: viewport wrapper

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                               Name
----               ------------                               ----
2000-07-22         Created                                    Michel Bruant
2000-dec           Add 55xx gfx layers                        Michel Bruant
Oct 2003           Added compositor                           Thomas Meyer
*******************************************************************************/

/* Includes ----------------------------------------------------------------- */
#if !defined (ST_OSLINUX)
    #include <string.h>
#endif
#include "sttbx.h"
#include "stlayer.h"
#include "layerapi.h"
#ifdef VIDEO
#include "halv_lay.h"
#endif

#ifdef GFX_GAMMA
#include "layergfx.h"
#endif

#ifdef GFX_55XX
#include "layerosd.h"
#include "lay_sub.h"
#ifdef GFX_STILL
#include "lay_stil.h"
#endif
#endif

#ifdef LAYER_COMPO
#include "laycompo.h"
#endif

#ifdef ST_XVP_ENABLE_FLEXVP
#include "hxvp_main.h"
#endif /* ST_XVP_ENABLE_FLEXVP */

/* Constants ---------------------------------------------------------------- */

#define TOTAL_MAX_VP        256          /* to be tuned */
#define KEY_HANDLE_USED     0x9456acd4



/* Types -------------------------------------------------------------------- */

typedef struct
{
    STLAYER_ViewPortHandle_t    LowLevelHandle;             /* low level */
    STLAYER_Handle_t            AssociatedLayerHandle;      /* low level */
    STLAYER_ViewPortHandle_t    AssociatedFilterVPHandle;   /* api level */
    U32                         Used;
    stlayer_LayerElement_t *    LayerDevice;
#ifdef ST_XVP_ENABLE_FLEXVP
    STLAYER_XVPHandle_t         XVPHandle;
#endif /* ST_XVP_ENABLE_FLEXVP */
} stlayer_ViewportElement_t;

/* Variables ---------------------------------------------------------------- */

static stlayer_ViewportElement_t stlayer_ViewportArray[TOTAL_MAX_VP];
static BOOL FirstOpenDone = FALSE;

/* Private Macro ------------------------------------------------------------ */

#define TestHandle(ApiHandle)                                                \
    if ((stlayer_ViewportElement_t *)ApiHandle == NULL)                      \
    {                                                                        \
        STTBX_Report((STTBX_REPORT_LEVEL_INFO,"STLAYER - API : Bad Handle"));\
        return(ST_ERROR_INVALID_HANDLE);                                     \
    }                                                                        \
    if(((stlayer_ViewportElement_t *)ApiHandle)->Used != KEY_HANDLE_USED)    \
    {                                                                        \
        STTBX_Report((STTBX_REPORT_LEVEL_INFO,"STLAYER - API : Bad Handle"));\
        return(ST_ERROR_INVALID_HANDLE);                                     \
    }

#define GetHandle(ApiHandle)       ((stlayer_ViewportElement_t*)(ApiHandle))\
                                    ->LowLevelHandle
#define GetLayerHandle(ApiHandle)  ((stlayer_ViewportElement_t*)(ApiHandle))\
                                    ->AssociatedLayerHandle
#define GetType(ApiHandle)         ((stlayer_ViewportElement_t*)(ApiHandle))\
                                    ->LayerDevice->LayerType
#define GetFilterHandle(ApiHandle) ((stlayer_ViewportElement_t*)\
                                  (((stlayer_ViewportElement_t*)(ApiHandle))\
                                   ->AssociatedFilterVPHandle))
#ifdef ST_XVP_ENABLE_FLEXVP
#define GetXVPHandle(ApiHandle)     ((stlayer_ViewportElement_t*)(ApiHandle))\
                                    ->XVPHandle
#endif /* ST_XVP_ENABLE_FLEXVP */

/* Public Functions --------------------------------------------------------- */

/*******************************************************************************
Name        : STLAYER_OpenViewPort
Description :
Parameters  : LayerHandle IN: Handle of the layer which will support the
    viewport, Params IN: Pointer to an allocated parameters structure for the
    desiredviewport, VPHandle IN: Pointer to an allocated viewport handle,
    OUT: Pointer to the returned ViewPort handle
Assumptions :
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_INVALID_HANDLE, ST_ERROR_BAD_PARAMETER,
    STLAYER_ERROR_NO_FREE_HANDLE
*******************************************************************************/
ST_ErrorCode_t STLAYER_OpenViewPort(
  STLAYER_Handle_t            LayerHandle,
  STLAYER_ViewPortParams_t*   Params_p,
  STLAYER_ViewPortHandle_t*   VPHandle_p)
{
    ST_ErrorCode_t              Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
    STLAYER_Handle_t            LowLevelLayerHandle;
    STLAYER_ViewPortHandle_t    LowLevelViewportHandle;
    STLAYER_Layer_t             LayerType;

    if((stlayer_OpenElement*)LayerHandle == NULL)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO,"STLAYER - API : Bad Handle"));
        return(ST_ERROR_INVALID_HANDLE);
    }
    if(((stlayer_OpenElement*)LayerHandle)->LayerDevice == NULL)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO,"STLAYER - API : Bad Handle"));
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (strcmp(((stlayer_OpenElement*)(LayerHandle))->LayerDevice->LayerName,
                "") == 0)
    {
        /* The layer is not init or open */
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (FirstOpenDone == FALSE)
    {
        U32 Index;
        FirstOpenDone = TRUE;
        for(Index = 0; Index < TOTAL_MAX_VP; Index++)
        {
            stlayer_ViewportArray[Index].LowLevelHandle             = 0;
            stlayer_ViewportArray[Index].AssociatedLayerHandle      = 0;
            stlayer_ViewportArray[Index].AssociatedFilterVPHandle   = 0;
            stlayer_ViewportArray[Index].Used                       = 0;
            stlayer_ViewportArray[Index].LayerDevice                = NULL;
#ifdef ST_XVP_ENABLE_FLEXVP
            stlayer_ViewportArray[Index].XVPHandle                  = 0;
#endif /* ST_XVP_ENABLE_FLEXVP */
        }
    }
    LowLevelLayerHandle = ((stlayer_OpenElement*)(LayerHandle))->LowLevelHandle;
    LayerType = ((stlayer_OpenElement*)(LayerHandle))->LayerDevice->LayerType;
    switch(LayerType)
    {
#if defined LAYER_COMPO
        case STLAYER_COMPOSITOR:
            Error = LAYCOMPO_OpenViewPort(LowLevelLayerHandle,Params_p,
                                         &LowLevelViewportHandle);
            break;
#endif
#if defined GFX_GAMMA
        case STLAYER_GAMMA_CURSOR:
        case STLAYER_GAMMA_GDP:
        case STLAYER_GAMMA_BKL:
        case STLAYER_GAMMA_GDPVBI:
        case STLAYER_GAMMA_ALPHA:
        case STLAYER_GAMMA_FILTER:
            Error = LAYERGFX_OpenViewPort(LowLevelLayerHandle,Params_p,
                                         &LowLevelViewportHandle);
            break;
#endif
#if defined GFX_55XX
        case STLAYER_OMEGA1_OSD:
            Error = LAYEROSD_OpenViewPort(LowLevelLayerHandle,Params_p,
                                         &LowLevelViewportHandle);
            break;
        case STLAYER_OMEGA1_CURSOR:
            Error = LAYERSUB_OpenViewPort(LowLevelLayerHandle,Params_p,
                                         &LowLevelViewportHandle);
            break;
#if defined GFX_STILL
        case STLAYER_OMEGA1_STILL:
            Error = LAYERSTILL_OpenViewPort(LowLevelLayerHandle,Params_p,
                                         &LowLevelViewportHandle);
            break;
#endif
#endif
#ifdef VIDEO
        case STLAYER_OMEGA2_VIDEO1:
        case STLAYER_OMEGA2_VIDEO2:
        case STLAYER_7020_VIDEO1  :
        case STLAYER_7020_VIDEO2  :
        case STLAYER_OMEGA1_VIDEO :
        case STLAYER_SDDISPO2_VIDEO1 :
        case STLAYER_SDDISPO2_VIDEO2 :
        case STLAYER_HDDISPO2_VIDEO1 :
        case STLAYER_HDDISPO2_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO1 :
        case STLAYER_DISPLAYPIPE_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO3 :
        case STLAYER_DISPLAYPIPE_VIDEO4 :

            Error = LAYERVID_OpenViewport(LowLevelLayerHandle,Params_p,
                                             &LowLevelViewportHandle);
            break;
#endif
        default:
            Error = ST_ERROR_BAD_PARAMETER;
            break;
    }
    if(!Error) /* record vp handle */
    {
        U32 i = 0;
        BOOL Found = FALSE;
        do
        {
            if (stlayer_ViewportArray[i].Used == 0) /* eq free index */
            {
                stlayer_ViewportArray[i].LowLevelHandle =LowLevelViewportHandle;
                stlayer_ViewportArray[i].Used =KEY_HANDLE_USED ;
                stlayer_ViewportArray[i].AssociatedLayerHandle
                                              =LowLevelLayerHandle ;
                stlayer_ViewportArray[i].LayerDevice =
                        ((stlayer_OpenElement*)(LayerHandle))->LayerDevice;
                *VPHandle_p=(STLAYER_ViewPortHandle_t)&stlayer_ViewportArray[i];
                Found = TRUE;
            }
            else
            {
                i ++;
            }
        } while((i < TOTAL_MAX_VP) && (Found == FALSE));
        if (Found == FALSE)
        {
            Error = ST_ERROR_NO_FREE_HANDLES;
        }
    }
    return(Error);
}

/*******************************************************************************
Name        : STLAYER_CloseViewPort
Description :
Parameters  : Handle IN: Handle of the viewport to be closed
Assumptions :
Limitations :
Returns     : ST_ERROR_INVALID_HANDLE, ST_NO_ERROR Success
*******************************************************************************/
ST_ErrorCode_t STLAYER_CloseViewPort(STLAYER_ViewPortHandle_t VPHandle)
{
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;

    TestHandle(VPHandle);

    switch(GetType(VPHandle))
    {
#if defined LAYER_COMPO
        case STLAYER_COMPOSITOR:
            Error = LAYCOMPO_CloseViewPort(GetHandle(VPHandle));
            break;
#endif
#if defined GFX_GAMMA
        case STLAYER_GAMMA_CURSOR:
        case STLAYER_GAMMA_GDP:
        case STLAYER_GAMMA_BKL:
        case STLAYER_GAMMA_GDPVBI:
        case STLAYER_GAMMA_ALPHA:
        case STLAYER_GAMMA_FILTER:
            if(GetFilterHandle(VPHandle) != 0)
            {
                Error = LAYERGFX_CloseViewPort(GetHandle(GetFilterHandle
                                                                   (VPHandle)));
            }
            Error = LAYERGFX_CloseViewPort(GetHandle(VPHandle));
            break;
#endif
#if defined GFX_55XX
        case STLAYER_OMEGA1_OSD:
            Error = LAYEROSD_CloseViewPort(GetHandle(VPHandle));
            break;
        case STLAYER_OMEGA1_CURSOR:
            Error = LAYERSUB_CloseViewPort(GetHandle(VPHandle));
            break;
#if defined GFX_STILL
        case STLAYER_OMEGA1_STILL:
            Error = LAYERSTILL_CloseViewPort(GetHandle(VPHandle));
            break;
#endif
#endif
#ifdef VIDEO
        case STLAYER_OMEGA2_VIDEO1:
        case STLAYER_OMEGA2_VIDEO2:
        case STLAYER_7020_VIDEO1  :
        case STLAYER_7020_VIDEO2  :
        case STLAYER_OMEGA1_VIDEO :
        case STLAYER_SDDISPO2_VIDEO1 :
        case STLAYER_SDDISPO2_VIDEO2 :
        case STLAYER_HDDISPO2_VIDEO1 :
        case STLAYER_HDDISPO2_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO1 :
        case STLAYER_DISPLAYPIPE_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO3 :
        case STLAYER_DISPLAYPIPE_VIDEO4 :
            Error = LAYERVID_CloseViewport(GetLayerHandle(VPHandle),
                                           GetHandle(VPHandle));
            break;
#endif
        default:
            Error = ST_ERROR_INVALID_HANDLE;
            break;
    }
    if(!Error)/* unrecord vp handle */
    {
        ((stlayer_ViewportElement_t*)(VPHandle))->Used                      = 0;
        ((stlayer_ViewportElement_t*)(VPHandle))->LowLevelHandle            = 0;
        ((stlayer_ViewportElement_t*)(VPHandle))->AssociatedLayerHandle     = 0;
        ((stlayer_ViewportElement_t*)(VPHandle))->AssociatedFilterVPHandle  = 0;
        ((stlayer_ViewportElement_t*)(VPHandle))->LayerDevice            = NULL;
    }
    return(Error);
}

/*******************************************************************************
Name        : STLAYER_EnableViewPort()
Description : Enables a specified viewport, showing it on the layer on which
        is open.
Parameters  : Handle IN: Handle of the viewport.
Assumptions :
Limitations :
Returns     : ST_ERROR_INVALID_HANDLE, STLAYER_ERROR_OVERLAP_VIEWPORT,
        ST_NO_ERROR
*******************************************************************************/
ST_ErrorCode_t STLAYER_EnableViewPort(STLAYER_ViewPortHandle_t VPHandle)
{
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;

    TestHandle(VPHandle);
    switch(GetType(VPHandle))
    {
#if defined LAYER_COMPO
        case STLAYER_COMPOSITOR:
            Error = LAYCOMPO_EnableViewPort(GetHandle(VPHandle));
            break;
#endif
#if defined GFX_GAMMA
        case STLAYER_GAMMA_CURSOR:
        case STLAYER_GAMMA_GDP:
        case STLAYER_GAMMA_GDPVBI:
        case STLAYER_GAMMA_BKL:
        case STLAYER_GAMMA_ALPHA:
        case STLAYER_GAMMA_FILTER:
            Error = ST_NO_ERROR;
            if(GetFilterHandle(VPHandle) != 0)
            {
                Error = LAYERGFX_EnableViewPort(GetHandle(GetFilterHandle
                                                                (VPHandle)));
            }
            if(Error == ST_NO_ERROR)
            {
                Error = LAYERGFX_EnableViewPort(GetHandle(VPHandle));
            }
            break;
#endif
#if defined GFX_55XX
        case STLAYER_OMEGA1_OSD:
            Error = LAYEROSD_EnableViewPort(GetHandle(VPHandle));
            break;
        case STLAYER_OMEGA1_CURSOR:
            Error = LAYERSUB_EnableViewPort(GetHandle(VPHandle));
            break;
#if defined GFX_STILL
        case STLAYER_OMEGA1_STILL:
            Error = LAYERSTILL_EnableViewPort(GetHandle(VPHandle));
            break;
#endif
#endif
#ifdef VIDEO
        case STLAYER_OMEGA2_VIDEO1:
        case STLAYER_OMEGA2_VIDEO2:
        case STLAYER_7020_VIDEO1  :
        case STLAYER_7020_VIDEO2  :
        case STLAYER_OMEGA1_VIDEO :
        case STLAYER_SDDISPO2_VIDEO1 :
        case STLAYER_SDDISPO2_VIDEO2 :
        case STLAYER_HDDISPO2_VIDEO1 :
        case STLAYER_HDDISPO2_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO1 :
        case STLAYER_DISPLAYPIPE_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO3 :
        case STLAYER_DISPLAYPIPE_VIDEO4 :
            LAYERVID_EnableVideoViewport(GetLayerHandle(VPHandle),
                                         GetHandle(VPHandle));
            Error = ST_NO_ERROR;
            break;
#endif
        default:
            Error = ST_ERROR_INVALID_HANDLE;
            break;
    }
    return(Error);
}

/*******************************************************************************
Name        : STLAYER_DisableViewPort()
Description : Disables display of a viewport, hiding it fro m the screen.
Parameters  : Handle IN: Handle of the viewport.
Assumptions :
Limitations :
Returns     : ST_ERROR_INVALID_HANDLE, ST_NO_ERROR
*******************************************************************************/
ST_ErrorCode_t STLAYER_DisableViewPort(STLAYER_ViewPortHandle_t VPHandle)
{
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;

    TestHandle(VPHandle);
    switch(GetType(VPHandle))
    {
#if defined LAYER_COMPO
        case STLAYER_COMPOSITOR:
            Error = LAYCOMPO_DisableViewPort(GetHandle(VPHandle));
            break;
#endif
#if defined GFX_GAMMA
        case STLAYER_GAMMA_CURSOR:
        case STLAYER_GAMMA_GDP:
        case STLAYER_GAMMA_GDPVBI:
        case STLAYER_GAMMA_BKL:
        case STLAYER_GAMMA_ALPHA:
        case STLAYER_GAMMA_FILTER:
            Error = ST_NO_ERROR;
            if(GetFilterHandle(VPHandle) != 0)
            {
                Error = LAYERGFX_DisableViewPort(GetHandle(GetFilterHandle
                                                                (VPHandle)));
            }
            if(Error == ST_NO_ERROR)
            {
                Error = LAYERGFX_DisableViewPort(GetHandle(VPHandle));
            }
            break;
#endif
#if defined GFX_55XX
        case STLAYER_OMEGA1_OSD:
            Error = LAYEROSD_DisableViewPort(GetHandle(VPHandle));
            break;
        case STLAYER_OMEGA1_CURSOR:
            Error = LAYERSUB_DisableViewPort(GetHandle(VPHandle));
            break;
#if defined GFX_STILL
        case STLAYER_OMEGA1_STILL:
            Error = LAYERSTILL_DisableViewPort(GetHandle(VPHandle));
            break;
#endif
#endif
#ifdef VIDEO
        case STLAYER_OMEGA2_VIDEO1:
        case STLAYER_OMEGA2_VIDEO2:
        case STLAYER_7020_VIDEO1  :
        case STLAYER_7020_VIDEO2  :
        case STLAYER_OMEGA1_VIDEO :
        case STLAYER_SDDISPO2_VIDEO1 :
        case STLAYER_SDDISPO2_VIDEO2 :
        case STLAYER_HDDISPO2_VIDEO1 :
        case STLAYER_HDDISPO2_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO1 :
        case STLAYER_DISPLAYPIPE_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO3 :
        case STLAYER_DISPLAYPIPE_VIDEO4 :
            LAYERVID_DisableVideoViewport(GetLayerHandle(VPHandle),
                                          GetHandle(VPHandle));
            Error = ST_NO_ERROR;
            break;
#endif
        default:
            Error = ST_ERROR_INVALID_HANDLE;
            break;
    }
    return(Error);
}


/*******************************************************************************
Name        : STLAYER_SetViewPortParams()
Description : Store the parameter of the source.
Parameters  : VPHandle IN: Handle of the ViewPort, VPParam IN: Pointer to an
              allocated structure which contains the parameters.
Assumptions :
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_INVALID_HANDLE, ST_ERROR_BAD_PARAMETER
*******************************************************************************/
ST_ErrorCode_t STLAYER_SetViewPortParams(
                    const STLAYER_ViewPortHandle_t  VPHandle,
                    STLAYER_ViewPortParams_t*       Params_p)
{
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;

    TestHandle(VPHandle);
    switch(GetType(VPHandle))
    {
#if defined LAYER_COMPO
        case STLAYER_COMPOSITOR:
            Error = LAYCOMPO_SetViewPortParams(GetHandle(VPHandle),
                                                            Params_p);
            break;
#endif
#if defined GFX_GAMMA
        case STLAYER_GAMMA_CURSOR:
        case STLAYER_GAMMA_GDP:
        case STLAYER_GAMMA_BKL:
        case STLAYER_GAMMA_GDPVBI:
        case STLAYER_GAMMA_ALPHA:
            Error = ST_NO_ERROR;
            if(GetFilterHandle(VPHandle) != 0)
            {
                Error = LAYERGFX_SetViewPortParams(GetHandle(GetFilterHandle
                                                      (VPHandle)),Params_p);
            }
            if(Error == ST_NO_ERROR)
            {
                Error = LAYERGFX_SetViewPortParams(GetHandle(VPHandle),
                                                            Params_p);
            }
            break;
#endif
#if defined GFX_55XX
        case STLAYER_OMEGA1_OSD:
            Error = LAYEROSD_SetViewPortParams(GetHandle(VPHandle),Params_p);
            break;
        case STLAYER_OMEGA1_CURSOR:
            Error = LAYERSUB_SetViewPortParams(GetHandle(VPHandle),Params_p);
            break;
#if defined GFX_STILL
        case STLAYER_OMEGA1_STILL:
            Error = LAYERSTILL_SetViewPortParams(GetHandle(VPHandle),Params_p);
            break;
#endif
#endif
#ifdef VIDEO
        case STLAYER_OMEGA2_VIDEO1:
        case STLAYER_OMEGA2_VIDEO2:
        case STLAYER_7020_VIDEO1  :
        case STLAYER_7020_VIDEO2  :
        case STLAYER_OMEGA1_VIDEO :
        case STLAYER_SDDISPO2_VIDEO1 :
        case STLAYER_SDDISPO2_VIDEO2 :
        case STLAYER_HDDISPO2_VIDEO1 :
        case STLAYER_HDDISPO2_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO1 :
        case STLAYER_DISPLAYPIPE_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO3 :
        case STLAYER_DISPLAYPIPE_VIDEO4 :
            Error = LAYERVID_SetViewportParams(GetLayerHandle(VPHandle),
                    GetHandle(VPHandle),Params_p);
            break;
#endif
        default:
            Error = ST_ERROR_INVALID_HANDLE;
            break;
    }
    return(Error);
}


/*******************************************************************************
Name        : STLAYER_GetViewPortParams()
Description : Retrieve the parameter of the source.
Parameters  : VPHandle IN: Handle of the ViewPort, VPParam OUT: Pointer to an
              allocated structure which will contain the parameters.
Assumptions :
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_INVALID_HANDLE, ST_ERROR_BAD_PARAMETER
*******************************************************************************/
ST_ErrorCode_t STLAYER_GetViewPortParams(
                    const STLAYER_ViewPortHandle_t  VPHandle,
                    STLAYER_ViewPortParams_t*       Params_p)
{
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;

    TestHandle(VPHandle);
    switch(GetType(VPHandle))
    {
#if defined LAYER_COMPO
        case STLAYER_COMPOSITOR:
            Error = LAYCOMPO_GetViewPortParams(GetHandle(VPHandle),Params_p);
            break;
#endif
#if defined GFX_GAMMA
        case STLAYER_GAMMA_CURSOR:
        case STLAYER_GAMMA_GDP:
        case STLAYER_GAMMA_GDPVBI:
        case STLAYER_GAMMA_BKL:
        case STLAYER_GAMMA_ALPHA:
            Error = LAYERGFX_GetViewPortParams(GetHandle(VPHandle),Params_p);
            break;
#endif
#if defined GFX_55XX
        case STLAYER_OMEGA1_OSD:
            Error = LAYEROSD_GetViewPortParams(GetHandle(VPHandle),Params_p);
            break;
        case STLAYER_OMEGA1_CURSOR:
            Error = LAYERSUB_GetViewPortParams(GetHandle(VPHandle),Params_p);
            break;
#if defined GFX_STILL
        case STLAYER_OMEGA1_STILL:
            Error = LAYERSTILL_GetViewPortParams(GetHandle(VPHandle),Params_p);
            break;
#endif
#endif
#ifdef VIDEO
        case STLAYER_OMEGA2_VIDEO1:
        case STLAYER_OMEGA2_VIDEO2:
        case STLAYER_7020_VIDEO1  :
        case STLAYER_7020_VIDEO2  :
        case STLAYER_OMEGA1_VIDEO :
        case STLAYER_SDDISPO2_VIDEO1 :
        case STLAYER_SDDISPO2_VIDEO2 :
        case STLAYER_HDDISPO2_VIDEO1 :
        case STLAYER_HDDISPO2_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO1 :
        case STLAYER_DISPLAYPIPE_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO3 :
        case STLAYER_DISPLAYPIPE_VIDEO4 :
            LAYERVID_GetViewportParams(GetLayerHandle(VPHandle),
                    GetHandle(VPHandle),Params_p);
            Error = ST_NO_ERROR;
            break;
#endif
        default:
            Error = ST_ERROR_INVALID_HANDLE;
            break;
    }
    return(Error);
}


/*******************************************************************************
Name        : STLAYER_SetViewportSource()
Description : Sets the source attached to a viewport.
Parameters  : VPHandle IN: Handle of the ViewPort, VPSource IN: Pointer to an
        allocated structure which contains the source informations.
Assumptions :
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_INVALID_HANDLE, ST_ERROR_BAD_PARAMETER
*******************************************************************************/
ST_ErrorCode_t STLAYER_SetViewPortSource(const STLAYER_ViewPortHandle_t
                    VPHandle,  STLAYER_ViewPortSource_t*   VPSource)
{
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;

    TestHandle(VPHandle);
    switch(GetType(VPHandle))
    {
#if defined LAYER_COMPO
        case STLAYER_COMPOSITOR:
            Error = LAYCOMPO_SetViewPortSource(GetHandle(VPHandle),VPSource);
            break;
#endif
#if defined GFX_GAMMA
        case STLAYER_GAMMA_CURSOR:
        case STLAYER_GAMMA_GDP:
        case STLAYER_GAMMA_GDPVBI:
        case STLAYER_GAMMA_BKL:
        case STLAYER_GAMMA_ALPHA:
            Error = ST_NO_ERROR;
            if(GetFilterHandle(VPHandle) != 0)
            {
                Error = LAYERGFX_SetViewPortSource(GetHandle(GetFilterHandle
                                                        (VPHandle)),VPSource);
            }
            if(Error == ST_NO_ERROR)
            {
                Error = LAYERGFX_SetViewPortSource(GetHandle(VPHandle),
                                                                VPSource);
            }
            break;
#endif
#if defined GFX_55XX
        case STLAYER_OMEGA1_OSD:
            Error = LAYEROSD_SetViewPortSource(GetHandle(VPHandle),VPSource);
            break;
        case STLAYER_OMEGA1_CURSOR:
            Error = LAYERSUB_SetViewPortSource(GetHandle(VPHandle),VPSource);
            break;
#if defined GFX_STILL
        case STLAYER_OMEGA1_STILL:
            Error = LAYERSTILL_SetViewPortSource(GetHandle(VPHandle),VPSource);
            break;
#endif
#endif
#ifdef VIDEO
        case STLAYER_OMEGA2_VIDEO1:
        case STLAYER_OMEGA2_VIDEO2:
        case STLAYER_7020_VIDEO1  :
        case STLAYER_7020_VIDEO2  :
        case STLAYER_OMEGA1_VIDEO :
        case STLAYER_SDDISPO2_VIDEO1 :
        case STLAYER_SDDISPO2_VIDEO2 :
        case STLAYER_HDDISPO2_VIDEO1 :
        case STLAYER_HDDISPO2_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO1 :
        case STLAYER_DISPLAYPIPE_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO3 :
        case STLAYER_DISPLAYPIPE_VIDEO4 :
            Error = LAYERVID_SetViewportSource(GetLayerHandle(VPHandle),
                                         GetHandle(VPHandle),VPSource);
            break;
#endif
        default:
            Error = ST_ERROR_INVALID_HANDLE;
            break;
    }
    return(Error);
}

/*******************************************************************************
Name        : STLAYER_GetViewportSource
Description :
Parameters  : Handle IN: Handle of the ViewPort, VPSource OUT: Pointere to
                the source of the viewport
Assumptions :
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_INVALID_HANDLE
*******************************************************************************/
ST_ErrorCode_t STLAYER_GetViewPortSource(STLAYER_ViewPortHandle_t VPHandle,
                    STLAYER_ViewPortSource_t*   VPSource)
{
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;

    TestHandle(VPHandle);
    switch(GetType(VPHandle))
    {
#if defined LAYER_COMPO
        case STLAYER_COMPOSITOR:
            Error = LAYCOMPO_GetViewPortSource(GetHandle(VPHandle),VPSource);
            break;
#endif
#if defined GFX_GAMMA
        case STLAYER_GAMMA_CURSOR:
        case STLAYER_GAMMA_GDP:
        case STLAYER_GAMMA_GDPVBI:
        case STLAYER_GAMMA_BKL:
        case STLAYER_GAMMA_ALPHA:
            Error = LAYERGFX_GetViewPortSource(GetHandle(VPHandle),VPSource);
            break;
#endif
#if defined GFX_55XX
        case STLAYER_OMEGA1_OSD:
            Error = LAYEROSD_GetViewPortSource(GetHandle(VPHandle),VPSource);
            break;
        case STLAYER_OMEGA1_CURSOR:
            Error = LAYERSUB_GetViewPortSource(GetHandle(VPHandle),VPSource);
            break;
#if defined GFX_STILL
        case STLAYER_OMEGA1_STILL:
            Error = LAYERSTILL_GetViewPortSource(GetHandle(VPHandle),VPSource);
            break;
#endif
#endif
#ifdef VIDEO
        case STLAYER_OMEGA2_VIDEO1:
        case STLAYER_OMEGA2_VIDEO2:
        case STLAYER_7020_VIDEO1  :
        case STLAYER_7020_VIDEO2  :
        case STLAYER_OMEGA1_VIDEO :
        case STLAYER_SDDISPO2_VIDEO1 :
        case STLAYER_SDDISPO2_VIDEO2 :
        case STLAYER_HDDISPO2_VIDEO1 :
        case STLAYER_HDDISPO2_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO1 :
        case STLAYER_DISPLAYPIPE_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO3 :
        case STLAYER_DISPLAYPIPE_VIDEO4 :
            /* STLAYER_ViewPortSource_t is not enough defined to call */
            /* this function */
            break;
#endif
        default:
            Error = ST_ERROR_INVALID_HANDLE;
            break;
    }
    return(Error);
}

/*******************************************************************************
Name        : STLAYER_SetViewPortIORectangle()
Description : Sets Input and Output viewport rectangles.
Parameters  : Handle IN: Handle of the viewport, InputRectangle IN: Pointer
        to the new input rectangle, OutputRectangle IN: Pointer to the new
        output rectagnle
Assumptions :
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_BAD_PARAMETER, ST_ERROR_INVALID_HANDLE,
                STLAYER_ERROR_INVALID_INPUT_RECTANGLE,
                STLAYER_ERROR_INVALID_OUTPUT_RECTANGLE
*******************************************************************************/
ST_ErrorCode_t STLAYER_SetViewPortIORectangle(STLAYER_ViewPortHandle_t VPHandle,
                                STGXOBJ_Rectangle_t*           InputRectangle,
                                STGXOBJ_Rectangle_t*           OutputRectangle)
{
    ST_ErrorCode_t Error ;

    TestHandle(VPHandle);
    switch(GetType(VPHandle))
    {
#if defined LAYER_COMPO
        case STLAYER_COMPOSITOR:
            Error = LAYCOMPO_SetViewPortIORectangle(GetHandle(VPHandle),InputRectangle,OutputRectangle);
            break;
#endif
#if defined GFX_GAMMA
        case STLAYER_GAMMA_CURSOR:
        case STLAYER_GAMMA_GDP:
        case STLAYER_GAMMA_GDPVBI:
        case STLAYER_GAMMA_BKL:
        case STLAYER_GAMMA_ALPHA:
            Error = ST_NO_ERROR;
            if(GetFilterHandle(VPHandle) != 0)
            {
                Error = LAYERGFX_SetViewPortIORectangle(
                      GetHandle(GetFilterHandle(VPHandle)),InputRectangle,
                                                           OutputRectangle);
            }
            if(Error == ST_NO_ERROR)
            {
                Error = LAYERGFX_SetViewPortIORectangle(
                        GetHandle(VPHandle),InputRectangle,OutputRectangle);
            }
            break;
#endif
#if defined GFX_55XX
        case STLAYER_OMEGA1_OSD:
            Error = LAYEROSD_SetViewPortIORectangle(GetHandle(VPHandle),
                                                InputRectangle,OutputRectangle);
            break;
        case STLAYER_OMEGA1_CURSOR:
            Error = LAYERSUB_SetViewPortIORectangle(GetHandle(VPHandle),
                                                InputRectangle,OutputRectangle);
            break;
#if defined GFX_STILL
        case STLAYER_OMEGA1_STILL:
            Error = LAYERSTILL_SetViewPortIORectangle(GetHandle(VPHandle),
                                                InputRectangle,OutputRectangle);
            break;
#endif
#endif
#ifdef VIDEO
        case STLAYER_OMEGA2_VIDEO1:
        case STLAYER_OMEGA2_VIDEO2:
        case STLAYER_7020_VIDEO1  :
        case STLAYER_7020_VIDEO2  :
        case STLAYER_OMEGA1_VIDEO :
        case STLAYER_SDDISPO2_VIDEO1 :
        case STLAYER_SDDISPO2_VIDEO2 :
        case STLAYER_HDDISPO2_VIDEO1 :
        case STLAYER_HDDISPO2_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO1 :
        case STLAYER_DISPLAYPIPE_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO3 :
        case STLAYER_DISPLAYPIPE_VIDEO4 :
            Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
            break;
#endif
        default:
            Error = ST_ERROR_INVALID_HANDLE;
            break;
    }
    return(Error);
}

/*******************************************************************************
Name        : STLAYER_GetViewPortIORectangle()
Description : Gets the input and output rectangle of a given viewport.
Parameters  : Handle IN: Handle of the viewport, InputRectangle IN: Pointer to
    an allocated rectangle structure, OUT: Input rectangle values,
    OutputRectangle IN: Pointer to an allocated rectangle structure,
    OUT: Output rectangle values
Assumptions :
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_INVALID_HANDLE, ST_ERROR_BAD_PARAMETER,
*******************************************************************************/
ST_ErrorCode_t STLAYER_GetViewPortIORectangle(STLAYER_ViewPortHandle_t VPHandle,
                                STGXOBJ_Rectangle_t*           InputRectangle,
                                STGXOBJ_Rectangle_t*           OutputRectangle)
{
    ST_ErrorCode_t Error;

    TestHandle(VPHandle);
    switch(GetType(VPHandle))
    {
#if defined LAYER_COMPO
        case STLAYER_COMPOSITOR:
            Error = LAYCOMPO_GetViewPortIORectangle(GetHandle(VPHandle),InputRectangle, OutputRectangle);
            break;
#endif
#if defined GFX_GAMMA
        case STLAYER_GAMMA_CURSOR:
        case STLAYER_GAMMA_GDP:
        case STLAYER_GAMMA_GDPVBI:
        case STLAYER_GAMMA_BKL:
        case STLAYER_GAMMA_ALPHA:
            Error = LAYERGFX_GetViewPortIORectangle(GetHandle(VPHandle),
                                             InputRectangle, OutputRectangle);
            break;
#endif
#if defined GFX_55XX
        case STLAYER_OMEGA1_OSD:
            Error = LAYEROSD_GetViewPortIORectangle(GetHandle(VPHandle),
                                             InputRectangle, OutputRectangle);
            break;
        case STLAYER_OMEGA1_CURSOR:
            Error = LAYERSUB_GetViewPortIORectangle(GetHandle(VPHandle),
                                             InputRectangle, OutputRectangle);
            break;
#if defined GFX_STILL
        case STLAYER_OMEGA1_STILL:
            Error = LAYERSTILL_GetViewPortIORectangle(GetHandle(VPHandle),
                                             InputRectangle, OutputRectangle);
            break;
#endif
#endif
#ifdef VIDEO
        case STLAYER_OMEGA2_VIDEO1:
        case STLAYER_OMEGA2_VIDEO2:
        case STLAYER_7020_VIDEO1  :
        case STLAYER_7020_VIDEO2  :
        case STLAYER_OMEGA1_VIDEO :
        case STLAYER_SDDISPO2_VIDEO1 :
        case STLAYER_SDDISPO2_VIDEO2 :
        case STLAYER_HDDISPO2_VIDEO1 :
        case STLAYER_HDDISPO2_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO1 :
        case STLAYER_DISPLAYPIPE_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO3 :
        case STLAYER_DISPLAYPIPE_VIDEO4 :
            Error = LAYERVID_GetViewportIORectangle(GetLayerHandle(VPHandle),
                                            GetHandle(VPHandle),InputRectangle,
                                            OutputRectangle);
            break;
#endif
        default:
            Error = ST_ERROR_INVALID_HANDLE;
            break;
    }
    return(Error);
}

/*******************************************************************************
Name        : STLAYER_SetViewPortPosition()
Description : Sets the position of an open viewport on the layer on which is
            opened.
Parameters  : Handle IN: Handle of the viewport to be moved,
                PositionX IN: New XPosition, PositionX IN: New Yposition
Assumptions :
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_INVALID_HANDLE, ST_ERROR_BAD_PARAMETER
*******************************************************************************/
ST_ErrorCode_t STLAYER_SetViewPortPosition(STLAYER_ViewPortHandle_t VPHandle,
                                          S32                      XPosition,
                                          S32                      YPosition)
{
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;

    TestHandle(VPHandle);
    switch(GetType(VPHandle))
    {
#if defined LAYER_COMPO
        case STLAYER_COMPOSITOR:
            Error = LAYCOMPO_SetViewPortPosition(
                                GetHandle(VPHandle),XPosition,YPosition);
            break;
#endif
#if defined GFX_GAMMA
        case STLAYER_GAMMA_CURSOR:
        case STLAYER_GAMMA_GDP:
        case STLAYER_GAMMA_GDPVBI:
        case STLAYER_GAMMA_BKL:
        case STLAYER_GAMMA_ALPHA:
            Error = ST_NO_ERROR;
            if(GetFilterHandle(VPHandle) != 0)
            {
                Error = LAYERGFX_SetViewPortPosition(
                                GetHandle(GetFilterHandle(VPHandle)),XPosition,
                                                                    YPosition);
            }
            if(Error == ST_NO_ERROR)
            {
                Error =LAYERGFX_SetViewPortPosition(
                                GetHandle(VPHandle),XPosition,YPosition);
            }
            break;
#endif
#if defined GFX_55XX
        case STLAYER_OMEGA1_OSD:
           Error =LAYEROSD_SetViewPortPosition(GetHandle(VPHandle),XPosition,
                                                    YPosition);
            break;
        case STLAYER_OMEGA1_CURSOR:
            Error =LAYERSUB_SetViewPortPosition(GetHandle(VPHandle),XPosition,
                                                    YPosition);
            break;
#if defined GFX_STILL
        case STLAYER_OMEGA1_STILL:
           Error =LAYERSTILL_SetViewPortPosition(GetHandle(VPHandle),XPosition,
                                                    YPosition);
            break;
#endif
#endif
#ifdef VIDEO
        case STLAYER_OMEGA2_VIDEO1:
        case STLAYER_OMEGA2_VIDEO2:
        case STLAYER_7020_VIDEO1  :
        case STLAYER_7020_VIDEO2  :
        case STLAYER_OMEGA1_VIDEO :
        case STLAYER_SDDISPO2_VIDEO1 :
        case STLAYER_SDDISPO2_VIDEO2 :
        case STLAYER_HDDISPO2_VIDEO1 :
        case STLAYER_HDDISPO2_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO1 :
        case STLAYER_DISPLAYPIPE_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO3 :
        case STLAYER_DISPLAYPIPE_VIDEO4 :
            LAYERVID_SetViewportPosition(GetLayerHandle(VPHandle),
                                     GetHandle(VPHandle), XPosition, YPosition);
            Error = ST_NO_ERROR;
            break;
#endif
        default:
            Error = ST_ERROR_INVALID_HANDLE;
            break;
    }
    return(Error);
}

/*******************************************************************************
Name        : STLAYER_GetViewPortPosition()
Description : Gets the position of a viewport on the layer on which is
                displayed.
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STLAYER_GetViewPortPosition(STLAYER_ViewPortHandle_t VPHandle,
                                          S32  *                   XPosition,
                                          S32  *                   YPosition)
{
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;

    TestHandle(VPHandle);
    switch(GetType(VPHandle))
    {
#if defined LAYER_COMPO
        case STLAYER_COMPOSITOR:
            Error = LAYCOMPO_GetViewPortPosition(GetHandle(VPHandle),XPosition,
                                                    YPosition);
        break;
#endif
#if defined GFX_GAMMA
        case STLAYER_GAMMA_CURSOR:
        case STLAYER_GAMMA_GDP:
        case STLAYER_GAMMA_GDPVBI:
        case STLAYER_GAMMA_BKL:
        case STLAYER_GAMMA_ALPHA:
            Error = LAYERGFX_GetViewPortPosition(GetHandle(VPHandle),XPosition,
                                                    YPosition);
            break;
#endif
#if defined GFX_55XX
        case STLAYER_OMEGA1_OSD:
            Error = LAYEROSD_GetViewPortPosition(GetHandle(VPHandle),XPosition,
                                                    YPosition);
            break;
        case STLAYER_OMEGA1_CURSOR:
            Error = LAYERSUB_GetViewPortPosition(GetHandle(VPHandle),XPosition,
                                                    YPosition);
            break;
#if defined GFX_STILL
        case STLAYER_OMEGA1_STILL:
            Error =LAYERSTILL_GetViewPortPosition(GetHandle(VPHandle),XPosition,
                                                    YPosition);
            break;
#endif
#endif
#ifdef VIDEO
        case STLAYER_OMEGA2_VIDEO1:
        case STLAYER_OMEGA2_VIDEO2:
        case STLAYER_7020_VIDEO1  :
        case STLAYER_7020_VIDEO2  :
        case STLAYER_OMEGA1_VIDEO :
        case STLAYER_SDDISPO2_VIDEO1 :
        case STLAYER_SDDISPO2_VIDEO2 :
        case STLAYER_HDDISPO2_VIDEO1 :
        case STLAYER_HDDISPO2_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO1 :
        case STLAYER_DISPLAYPIPE_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO3 :
        case STLAYER_DISPLAYPIPE_VIDEO4 :
            LAYERVID_GetViewportPosition(GetLayerHandle(VPHandle),
                                        GetHandle(VPHandle), (U32 *)XPosition,
                                        (U32 *)YPosition);
            Error = ST_NO_ERROR;
            break;
#endif
        default:
            Error = ST_ERROR_INVALID_HANDLE;
            break;
    }
    return(Error);
}

/*******************************************************************************
Name        : STLAYER_SetViewportPSI()
Description : Sets the PSI for the designated viewport.
Parameters  : VPHandle IN: Handle of the ViewPort, VPPSI IN: Pointer to
            an allocated structure which contains the source informations.
Assumptions :
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_INVALID_HANDLE, ST_ERROR_BAD_PARAMETER
*******************************************************************************/
ST_ErrorCode_t STLAYER_SetViewPortPSI(STLAYER_ViewPortHandle_t VPHandle,
                                      STLAYER_PSI_t*            VPPSI_p)
{
    BOOL IsParameterUsed=FALSE;
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;

    TestHandle(VPHandle);
    switch(GetType(VPHandle))
    {
#if defined LAYER_COMPO
        case STLAYER_COMPOSITOR:
            break;
#endif
#if defined GFX_GAMMA
        case STLAYER_GAMMA_CURSOR:
        case STLAYER_GAMMA_GDP:
        case STLAYER_GAMMA_GDPVBI:
        case STLAYER_GAMMA_BKL:
        case STLAYER_GAMMA_ALPHA:
            Error = ST_NO_ERROR;
            if(GetFilterHandle(VPHandle) != 0)
            {
                Error = LAYERGFX_SetViewPortPSI(GetHandle(GetFilterHandle
                                                (VPHandle)),VPPSI_p);
            }
            if(Error == ST_NO_ERROR)
            {
                Error = LAYERGFX_SetViewPortPSI(GetHandle(VPHandle),VPPSI_p);
            }
            IsParameterUsed=TRUE;
            break;
#endif
#if defined GFX_55XX
        case STLAYER_OMEGA1_OSD:
            /* No PSI in osd */
            break;
        case STLAYER_OMEGA1_CURSOR:
            /* No PSI in sub */
            break;
#if defined GFX_STILL
        case STLAYER_OMEGA1_STILL:
            /* No PSI in still */
            break;
#endif
#endif
#ifdef VIDEO
        case STLAYER_OMEGA1_VIDEO:
            /* No PSI in omega 1 video */
            break;
        case STLAYER_HDDISPO2_VIDEO1 :
        case STLAYER_HDDISPO2_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO1 :
        case STLAYER_DISPLAYPIPE_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO3 :
        case STLAYER_DISPLAYPIPE_VIDEO4 :
        case STLAYER_OMEGA2_VIDEO1:
        case STLAYER_OMEGA2_VIDEO2:
        case STLAYER_7020_VIDEO1  :
        case STLAYER_7020_VIDEO2  :
            Error = LAYERVID_SetViewPortPSI(GetLayerHandle(VPHandle),
                                            GetHandle(VPHandle), VPPSI_p);
            IsParameterUsed=TRUE;
            break;
#endif
        default:
            Error = ST_ERROR_INVALID_HANDLE;
            break;
    }
    if ( IsParameterUsed==FALSE)
    {
        UNUSED_PARAMETER(VPPSI_p);
    }


    return(Error);
}

/*******************************************************************************
Name        : STLAYER_GetViewportPSI()
Description :
Assumptions :
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_INVALID_HANDLE, ST_ERROR_BAD_PARAMETER
*******************************************************************************/
ST_ErrorCode_t STLAYER_GetViewPortPSI(STLAYER_ViewPortHandle_t VPHandle,
                                      STLAYER_PSI_t*            VPPSI_p)
{
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;

    TestHandle(VPHandle);
    switch(GetType(VPHandle))
    {
        case STLAYER_COMPOSITOR :
        case STLAYER_GAMMA_CURSOR:
        case STLAYER_GAMMA_GDP:
        case STLAYER_GAMMA_GDPVBI:
        case STLAYER_GAMMA_BKL:
        case STLAYER_GAMMA_ALPHA:
        case STLAYER_OMEGA1_OSD:
        case STLAYER_OMEGA1_CURSOR:
        case STLAYER_OMEGA1_STILL:
        case STLAYER_OMEGA1_VIDEO:
        case STLAYER_SDDISPO2_VIDEO1 :
        case STLAYER_SDDISPO2_VIDEO2 :
            UNUSED_PARAMETER(VPPSI_p);
            break;
#ifdef VIDEO
        case STLAYER_HDDISPO2_VIDEO1 :
        case STLAYER_HDDISPO2_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO1 :
        case STLAYER_DISPLAYPIPE_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO3 :
        case STLAYER_DISPLAYPIPE_VIDEO4 :
        case STLAYER_OMEGA2_VIDEO1:
        case STLAYER_OMEGA2_VIDEO2:
        case STLAYER_7020_VIDEO1  :
        case STLAYER_7020_VIDEO2  :
            Error = LAYERVID_GetViewPortPSI(GetLayerHandle(VPHandle),
                                            GetHandle(VPHandle), VPPSI_p);
            break;
#endif
        default:
            UNUSED_PARAMETER(VPPSI_p);
            Error = ST_ERROR_INVALID_HANDLE;
            break;
    }
    return(Error);
}

/*******************************************************************************
Name        : STLAYER_SetViewPortSpecialMode()
Description : Set special mode parameters (spectacle, mode, ...)
Parameters  : Handle IN: Handle of the viewport, Output mode to initialize
                and output mode parameters.
Assumptions :
Limitations :
Returns     : ST_ERROR_INVALID_HANDLE, ST_ERROR_FEATURE_NOT_SUPPORTED,
                ST_NO_ERROR
*******************************************************************************/
ST_ErrorCode_t STLAYER_SetViewPortSpecialMode (
  const STLAYER_ViewPortHandle_t                  VPHandle,
  const STLAYER_OutputMode_t OutputMode,
  const STLAYER_OutputWindowSpecialModeParams_t * const Params_p)
{
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;

    TestHandle(VPHandle);
    switch(GetType(VPHandle))
    {
        case STLAYER_COMPOSITOR :
        case STLAYER_GAMMA_CURSOR:
        case STLAYER_GAMMA_GDP:
        case STLAYER_GAMMA_GDPVBI:
        case STLAYER_GAMMA_BKL:
        case STLAYER_GAMMA_ALPHA:
        case STLAYER_OMEGA1_OSD:
        case STLAYER_OMEGA1_CURSOR:
        case STLAYER_OMEGA1_STILL:
        case STLAYER_OMEGA1_VIDEO:
            UNUSED_PARAMETER(OutputMode);
            UNUSED_PARAMETER(Params_p);
            break;
#ifdef VIDEO
        case STLAYER_OMEGA2_VIDEO1:
        case STLAYER_OMEGA2_VIDEO2:
        case STLAYER_7020_VIDEO1  :
        case STLAYER_7020_VIDEO2  :
        case STLAYER_SDDISPO2_VIDEO1 :
        case STLAYER_SDDISPO2_VIDEO2 :
        case STLAYER_HDDISPO2_VIDEO1 :
        case STLAYER_HDDISPO2_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO1 :
        case STLAYER_DISPLAYPIPE_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO3 :
        case STLAYER_DISPLAYPIPE_VIDEO4 :
            Error = LAYERVID_SetViewPortSpecialMode(GetLayerHandle(VPHandle),
                        GetHandle(VPHandle),
                        OutputMode,
                        Params_p);
            break;
#endif
        default:
            UNUSED_PARAMETER(OutputMode);
            UNUSED_PARAMETER(Params_p);
            Error = ST_ERROR_INVALID_HANDLE;
            break;
    }
    return(Error);

} /* End of STLAYER_SetViewPortSpecialMode() function. */

/*******************************************************************************
Name        : STLAYER_GetViewPortSpecialMode()
Description : Get special mode parameters (spectacle, mode, ...)
Parameters  : Handle IN: Handle of the viewport, Output mode to initialize
                and output mode parameters.
Assumptions :
Limitations :
Returns     : ST_ERROR_INVALID_HANDLE, ST_ERROR_FEATURE_NOT_SUPPORTED,
                ST_NO_ERROR
*******************************************************************************/
ST_ErrorCode_t STLAYER_GetViewPortSpecialMode (
  const STLAYER_ViewPortHandle_t                  VPHandle,
  STLAYER_OutputMode_t                    * const OutputMode_p,
  STLAYER_OutputWindowSpecialModeParams_t * const Params_p)
{
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;

    TestHandle(VPHandle);
    switch(GetType(VPHandle))
    {
        case STLAYER_COMPOSITOR :
        case STLAYER_GAMMA_CURSOR:
        case STLAYER_GAMMA_GDP:
        case STLAYER_GAMMA_GDPVBI:
        case STLAYER_GAMMA_BKL:
        case STLAYER_GAMMA_ALPHA:
        case STLAYER_OMEGA1_OSD:
        case STLAYER_OMEGA1_CURSOR:
        case STLAYER_OMEGA1_STILL:
        case STLAYER_OMEGA1_VIDEO:
            UNUSED_PARAMETER(OutputMode_p);
            UNUSED_PARAMETER(Params_p);
            break;
#ifdef VIDEO
        case STLAYER_OMEGA2_VIDEO1:
        case STLAYER_OMEGA2_VIDEO2:
        case STLAYER_7020_VIDEO1  :
        case STLAYER_7020_VIDEO2  :
        case STLAYER_SDDISPO2_VIDEO1 :
        case STLAYER_SDDISPO2_VIDEO2 :
        case STLAYER_HDDISPO2_VIDEO1 :
        case STLAYER_HDDISPO2_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO1 :
        case STLAYER_DISPLAYPIPE_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO3 :
        case STLAYER_DISPLAYPIPE_VIDEO4 :
            Error = LAYERVID_GetViewPortSpecialMode(GetLayerHandle(VPHandle),
                        GetHandle(VPHandle), OutputMode_p, Params_p);
            break;
#endif
        default:
            UNUSED_PARAMETER(OutputMode_p);
            UNUSED_PARAMETER(Params_p);
            Error = ST_ERROR_INVALID_HANDLE;
            break;
    }
    return(Error);

} /* End of STLAYER_GetViewPortSpecialMode() function. */

/*******************************************************************************
Name        : STLAYER_DisableColorKey()
Description : Disables the Color Key on a specified viewport
Parameters  : Handle IN: Handle of the viewport.
Assumptions :
Limitations :
Returns     : ST_ERROR_INVALID_HANDLE, ST_NO_ERROR
*******************************************************************************/
ST_ErrorCode_t STLAYER_DisableColorKey(STLAYER_ViewPortHandle_t VPHandle)
{
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;

    TestHandle(VPHandle);
    switch(GetType(VPHandle))
    {
#if defined LAYER_COMPO
        case STLAYER_COMPOSITOR:
            /* No color key in compo */
            break;
#endif
#if defined GFX_GAMMA
        case STLAYER_GAMMA_CURSOR:
        case STLAYER_GAMMA_GDP:
        case STLAYER_GAMMA_GDPVBI:
        case STLAYER_GAMMA_BKL:
            Error = ST_NO_ERROR;
            if(GetFilterHandle(VPHandle) != 0)
            {
                Error = LAYERGFX_DisableColorKey(GetHandle(GetFilterHandle
                                                                (VPHandle)));
            }
            if(Error == ST_NO_ERROR)
            {
                Error = LAYERGFX_DisableColorKey(GetHandle(VPHandle));
            }
            break;
        case STLAYER_GAMMA_ALPHA:
            break;
#endif
#if defined GFX_55XX
        case STLAYER_OMEGA1_OSD:
            /* no Color Key in osd */
            break;
        case STLAYER_OMEGA1_CURSOR:
            /* no Color Key in sub */
            break;
        case STLAYER_OMEGA1_STILL:
            /* no Color Key in still */
            break;
#endif
#ifdef VIDEO
        case STLAYER_OMEGA2_VIDEO1:
        case STLAYER_OMEGA2_VIDEO2:
        case STLAYER_7020_VIDEO1  :
        case STLAYER_7020_VIDEO2  :
        case STLAYER_OMEGA1_VIDEO :
        case STLAYER_SDDISPO2_VIDEO1 :
        case STLAYER_SDDISPO2_VIDEO2 :
        case STLAYER_HDDISPO2_VIDEO1 :
        case STLAYER_HDDISPO2_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO1 :
        case STLAYER_DISPLAYPIPE_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO3 :
        case STLAYER_DISPLAYPIPE_VIDEO4 :
            /* not supported */
            break;
#endif
        default:
            Error = ST_ERROR_INVALID_HANDLE;
            break;
    }
    return(Error);
}

/*******************************************************************************
Name        : STLAYER_EnableColorKey()
Description : Enable the Color Key on a specified viewport
Parameters  : Handle IN: Handle of the viewport.
Assumptions :
Limitations :
Returns     : ST_ERROR_INVALID_HANDLE, ST_NO_ERROR
*******************************************************************************/
ST_ErrorCode_t STLAYER_EnableColorKey(STLAYER_ViewPortHandle_t VPHandle)
{
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;

    TestHandle(VPHandle);
    switch(GetType(VPHandle))
    {
#if defined LAYER_COMPO
        case STLAYER_COMPOSITOR:
            /* No color key in compo */
            break;
#endif
#if defined GFX_GAMMA
        case STLAYER_GAMMA_CURSOR:
        case STLAYER_GAMMA_GDP:
        case STLAYER_GAMMA_GDPVBI:
        case STLAYER_GAMMA_BKL:
            Error = ST_NO_ERROR;
            if(GetFilterHandle(VPHandle) != 0)
            {
                Error = LAYERGFX_EnableColorKey(GetHandle(GetFilterHandle
                                                                (VPHandle)));
            }
            if(Error == ST_NO_ERROR)
            {
                Error = LAYERGFX_EnableColorKey(GetHandle(VPHandle));
            }
            break;
        case STLAYER_GAMMA_ALPHA:
            break;
#endif
#if defined GFX_55XX
        case STLAYER_OMEGA1_OSD:
            /* no Color Key in osd */
            break;
        case STLAYER_OMEGA1_STILL:
            /* no Color Key in still */
            break;
        case STLAYER_OMEGA1_CURSOR:
            /* no Color Key in sub */
            break;
#endif
#ifdef VIDEO
        case STLAYER_OMEGA2_VIDEO1:
        case STLAYER_OMEGA2_VIDEO2:
        case STLAYER_7020_VIDEO1  :
        case STLAYER_7020_VIDEO2  :
        case STLAYER_OMEGA1_VIDEO :
        case STLAYER_SDDISPO2_VIDEO1 :
        case STLAYER_SDDISPO2_VIDEO2 :
        case STLAYER_HDDISPO2_VIDEO1 :
        case STLAYER_HDDISPO2_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO1 :
        case STLAYER_DISPLAYPIPE_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO3 :
        case STLAYER_DISPLAYPIPE_VIDEO4 :
            /* not supported */
            break;
#endif
        default:
            Error = ST_ERROR_INVALID_HANDLE;
            break;
    }
    return(Error);
}

/*******************************************************************************
Name        : STLAYER_SetViewPortColorKey()
Description : Sets the color key associated with a specified viewport
Parameters  : Handle IN: Handle of the viewport, ColorKey IN: pointer to
                the desired color key
Assumptions :
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_BAD_PARAMETER, ST_ERROR_INVALID_HANDLE
*******************************************************************************/
ST_ErrorCode_t STLAYER_SetViewPortColorKey(STLAYER_ViewPortHandle_t VPHandle,
                                         STGXOBJ_ColorKey_t*        ColorKey)
{
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
    BOOL IsParameterUsed=FALSE;

    TestHandle(VPHandle);
    switch(GetType(VPHandle))
    {
#if defined LAYER_COMPO
        case STLAYER_COMPOSITOR:
            /* No color key in compo */
            break;
#endif
#if defined GFX_GAMMA
        case STLAYER_GAMMA_CURSOR:
        case STLAYER_GAMMA_GDP:
        case STLAYER_GAMMA_GDPVBI:
        case STLAYER_GAMMA_BKL:
            Error = ST_NO_ERROR;
            if(GetFilterHandle(VPHandle) != 0)
            {
                Error = LAYERGFX_SetViewPortColorKey(GetHandle(GetFilterHandle
                                                        (VPHandle)),ColorKey);
            }
            if(Error == ST_NO_ERROR)
            {
                Error = LAYERGFX_SetViewPortColorKey(GetHandle(VPHandle),
                                                        ColorKey);
            }
            IsParameterUsed=TRUE;
            break;
        case STLAYER_GAMMA_ALPHA:
            break;
#endif
#if defined GFX_55XX
        case STLAYER_OMEGA1_OSD:
            /* no Color Key in osd */
            break;
        case STLAYER_OMEGA1_STILL:
            /* no Color Key in still */
            break;
        case STLAYER_OMEGA1_CURSOR:
            /* no Color Key in sub */
            break;
#endif
#ifdef VIDEO
        case STLAYER_OMEGA2_VIDEO1:
        case STLAYER_OMEGA2_VIDEO2:
        case STLAYER_7020_VIDEO1  :
        case STLAYER_7020_VIDEO2  :
        case STLAYER_OMEGA1_VIDEO :
        case STLAYER_SDDISPO2_VIDEO1 :
        case STLAYER_SDDISPO2_VIDEO2 :
        case STLAYER_HDDISPO2_VIDEO1 :
        case STLAYER_HDDISPO2_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO1 :
        case STLAYER_DISPLAYPIPE_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO3 :
        case STLAYER_DISPLAYPIPE_VIDEO4 :
            /* not supported */
            break;
#endif
        default:
            Error = ST_ERROR_INVALID_HANDLE;
            break;
    }
    if ( IsParameterUsed==FALSE)
    {
        UNUSED_PARAMETER(ColorKey);
    }

    return(Error);
}

/*******************************************************************************
Name        : STLAYER_GetViewPortColorKey()
Description : Gets the color key associated with a specified viewport
Parameters  : Handle IN: Handle of the viewport, ColorKey IN: Pointer
            to an allocated color key data type, OUT: returned color key fields
Assumptions :
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_INVALID_HANDLE, ST_ERROR_BAD_PARAMETER
*******************************************************************************/
ST_ErrorCode_t STLAYER_GetViewPortColorKey(STLAYER_ViewPortHandle_t VPHandle,
                                         STGXOBJ_ColorKey_t*        ColorKey)
{
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
    BOOL IsParameterUsed=FALSE;

    TestHandle(VPHandle);
    switch(GetType(VPHandle))
    {
#if defined LAYER_COMPO
        case STLAYER_COMPOSITOR:
            /* No color key in compo */
            break;
#endif
#if defined GFX_GAMMA
        case STLAYER_GAMMA_CURSOR:
        case STLAYER_GAMMA_GDP:
        case STLAYER_GAMMA_GDPVBI:
        case STLAYER_GAMMA_BKL:
            Error = LAYERGFX_GetViewPortColorKey(GetHandle(VPHandle),ColorKey);
            IsParameterUsed=TRUE;
            break;
        case STLAYER_GAMMA_ALPHA:
            break;
#endif
#if defined GFX_55XX
        case STLAYER_OMEGA1_OSD:
            /* no Color Key in osd */
            break;
        case STLAYER_OMEGA1_STILL:
            /* no Color Key in still */
            break;
        case STLAYER_OMEGA1_CURSOR:
            /* no Color Key in sub */
            break;
#endif
#ifdef VIDEO
        case STLAYER_OMEGA2_VIDEO1:
        case STLAYER_OMEGA2_VIDEO2:
        case STLAYER_7020_VIDEO1  :
        case STLAYER_7020_VIDEO2  :
        case STLAYER_OMEGA1_VIDEO :
        case STLAYER_SDDISPO2_VIDEO1 :
        case STLAYER_SDDISPO2_VIDEO2 :
        case STLAYER_HDDISPO2_VIDEO1 :
        case STLAYER_HDDISPO2_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO1 :
        case STLAYER_DISPLAYPIPE_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO3 :
        case STLAYER_DISPLAYPIPE_VIDEO4 :
            /* not supported */
            break;
#endif
        default:
            Error = ST_ERROR_INVALID_HANDLE;
            break;
    }
     if ( IsParameterUsed==FALSE)
    {
        UNUSED_PARAMETER(ColorKey);
    }

    return(Error);
}

/*******************************************************************************
Name        : STLAYER_DisableBorderAlpha()
Description : Disables the Border Alpha on a specified viewport
Parameters  : Handle IN: Handle of the viewport.
Assumptions :
Limitations :
Returns     : ST_ERROR_INVALID_HANDLE, ST_NO_ERROR
*******************************************************************************/
ST_ErrorCode_t STLAYER_DisableBorderAlpha(STLAYER_ViewPortHandle_t VPHandle)
{
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;

    TestHandle(VPHandle);
    switch(GetType(VPHandle))
    {
#if defined LAYER_COMPO
        case STLAYER_COMPOSITOR:
            Error = LAYCOMPO_DisableBorderAlpha(GetHandle(VPHandle));
            break;
#endif
#if defined GFX_GAMMA
        case STLAYER_GAMMA_CURSOR:
        case STLAYER_GAMMA_GDP:
        case STLAYER_GAMMA_GDPVBI:
        case STLAYER_GAMMA_BKL:
            Error = ST_NO_ERROR;
            if(GetFilterHandle(VPHandle) != 0)
            {
                Error = LAYERGFX_DisableBorderAlpha(GetHandle(GetFilterHandle
                                                                   (VPHandle)));
            }
            if(Error == ST_NO_ERROR)
            {
                Error = LAYERGFX_DisableBorderAlpha(GetHandle(VPHandle));
            }
            break;
        case STLAYER_GAMMA_ALPHA:
            break;
#endif
#if defined GFX_55XX
        case STLAYER_OMEGA1_OSD:
            /* no Border Alpha in osd */
            break;
        case STLAYER_OMEGA1_STILL:
            /* no Border Alpha in still */
            break;
        case STLAYER_OMEGA1_CURSOR:
            /* no Border Alpha in sub */
            break;
#endif
#ifdef VIDEO
        case STLAYER_OMEGA2_VIDEO1:
        case STLAYER_OMEGA2_VIDEO2:
        case STLAYER_7020_VIDEO1  :
        case STLAYER_7020_VIDEO2  :
        case STLAYER_OMEGA1_VIDEO :
        case STLAYER_SDDISPO2_VIDEO1 :
        case STLAYER_SDDISPO2_VIDEO2 :
        case STLAYER_HDDISPO2_VIDEO1 :
        case STLAYER_HDDISPO2_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO1 :
        case STLAYER_DISPLAYPIPE_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO3 :
        case STLAYER_DISPLAYPIPE_VIDEO4 :
            /* not supported */
            break;
#endif
        default:
            Error = ST_ERROR_INVALID_HANDLE;
            break;
    }
    return(Error);
}

/*******************************************************************************
Name        : STLAYER_EnableBorderAlpha()
Description : Enable the Border Alpha on a specified viewport
Parameters  : Handle IN: Handle of the viewport.
Assumptions :
Limitations :
Returns     : ST_ERROR_INVALID_HANDLE, ST_NO_ERROR
*******************************************************************************/
ST_ErrorCode_t STLAYER_EnableBorderAlpha(STLAYER_ViewPortHandle_t VPHandle)
{
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;

    TestHandle(VPHandle);
    switch(GetType(VPHandle))
    {
#if defined LAYER_COMPO
        case STLAYER_COMPOSITOR:
            Error = LAYCOMPO_EnableBorderAlpha(GetHandle(VPHandle));
            break;
#endif
#if defined GFX_GAMMA
        case STLAYER_GAMMA_CURSOR:
        case STLAYER_GAMMA_GDP:
        case STLAYER_GAMMA_GDPVBI:
        case STLAYER_GAMMA_BKL:
            Error = ST_NO_ERROR;
            if(GetFilterHandle(VPHandle) != 0)
            {
                Error = LAYERGFX_EnableBorderAlpha(GetHandle(GetFilterHandle
                                                                (VPHandle)));
            }
            if(Error == ST_NO_ERROR)
            {
                Error = LAYERGFX_EnableBorderAlpha(GetHandle(VPHandle));
            }
            break;
        case STLAYER_GAMMA_ALPHA:
            break;
#endif
#if defined GFX_55XX
        case STLAYER_OMEGA1_OSD:
            /* no Border Alpha in osd */
            break;
        case STLAYER_OMEGA1_STILL:
            /* no Border Alpha in still */
            break;
        case STLAYER_OMEGA1_CURSOR:
            /* no Border Alpha in sub */
            break;
#endif
#ifdef VIDEO
        case STLAYER_OMEGA2_VIDEO1:
        case STLAYER_OMEGA2_VIDEO2:
        case STLAYER_7020_VIDEO1  :
        case STLAYER_7020_VIDEO2  :
        case STLAYER_OMEGA1_VIDEO :
        case STLAYER_SDDISPO2_VIDEO1 :
        case STLAYER_SDDISPO2_VIDEO2 :
        case STLAYER_HDDISPO2_VIDEO1 :
        case STLAYER_HDDISPO2_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO1 :
        case STLAYER_DISPLAYPIPE_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO3 :
        case STLAYER_DISPLAYPIPE_VIDEO4 :
            /* not supported */
            break;
#endif
        default:
            Error = ST_ERROR_INVALID_HANDLE;
            break;
    }
    return(Error);
}

/*******************************************************************************
Name        : STLAYER_GetViewPortAlpha()
Description : Gets the global alpha of a specified ViewPort.
Parameters  : Handle IN: Handle of the viewport, Alpha IN: Pointer to
            an allocated STXX_Alpha_t data field, OUT: returned alpha value(s)
Assumptions :
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_INVALID_HANDLE, ST_ERROR_BAD_PARAMETER
*******************************************************************************/
ST_ErrorCode_t STLAYER_GetViewPortAlpha(STLAYER_ViewPortHandle_t VPHandle,
                                        STLAYER_GlobalAlpha_t*    Alpha)
{
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;

    TestHandle(VPHandle);
    switch(GetType(VPHandle))
    {
#if defined LAYER_COMPO
        case STLAYER_COMPOSITOR:
            Error = LAYCOMPO_GetViewPortAlpha(GetHandle(VPHandle),Alpha);
            break;
#endif
#if defined GFX_GAMMA
        case STLAYER_GAMMA_CURSOR:
        case STLAYER_GAMMA_GDP:
        case STLAYER_GAMMA_GDPVBI:
        case STLAYER_GAMMA_BKL:
        case STLAYER_GAMMA_ALPHA:
            Error = LAYERGFX_GetViewPortAlpha(GetHandle(VPHandle),Alpha);
            break;
#endif
#if defined GFX_55XX
        case STLAYER_OMEGA1_OSD:
            Error = LAYEROSD_GetViewPortAlpha(GetHandle(VPHandle),Alpha);
            break;
        case STLAYER_OMEGA1_CURSOR:
            Error = LAYERSUB_GetViewPortAlpha(GetHandle(VPHandle),Alpha);
            break;
#if defined GFX_STILL
        case STLAYER_OMEGA1_STILL:
            Error = LAYERSTILL_GetViewPortAlpha(GetHandle(VPHandle),Alpha);
            break;
#endif
#endif
#ifdef VIDEO
        case STLAYER_OMEGA2_VIDEO1:
        case STLAYER_OMEGA2_VIDEO2:
        case STLAYER_7020_VIDEO1  :
        case STLAYER_7020_VIDEO2  :
        case STLAYER_OMEGA1_VIDEO :
        case STLAYER_SDDISPO2_VIDEO1 :
        case STLAYER_SDDISPO2_VIDEO2 :
        case STLAYER_HDDISPO2_VIDEO1 :
        case STLAYER_HDDISPO2_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO1 :
        case STLAYER_DISPLAYPIPE_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO3 :
        case STLAYER_DISPLAYPIPE_VIDEO4 :
            LAYERVID_GetViewportAlpha(GetLayerHandle(VPHandle),
                                      GetHandle(VPHandle), Alpha);
            Error = ST_NO_ERROR;
            break;
#endif
        default:
            Error = ST_ERROR_INVALID_HANDLE;
            break;
    }
    return(Error);
}

/*******************************************************************************
Name        : STLAYER_SetViewPortAlpha()
Description : Sets global alpha.
Parameters  : Handle IN: Handle of the viewport, Alpha IN: Pointer to
            the alpha value to be set
Assumptions :
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_INVALID_HANDLE, ST_ERROR_BAD_PARAMETER
*******************************************************************************/
ST_ErrorCode_t STLAYER_SetViewPortAlpha(STLAYER_ViewPortHandle_t VPHandle,
                                        STLAYER_GlobalAlpha_t*    Alpha)
{
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;

    TestHandle(VPHandle);
    switch(GetType(VPHandle))
    {
#if defined LAYER_COMPO
        case STLAYER_COMPOSITOR:
            Error = LAYCOMPO_SetViewPortAlpha(GetHandle(VPHandle),Alpha);
            break;
#endif
#if defined GFX_GAMMA
        case STLAYER_GAMMA_CURSOR:
        case STLAYER_GAMMA_GDP:
        case STLAYER_GAMMA_GDPVBI:
        case STLAYER_GAMMA_BKL:
        case STLAYER_GAMMA_ALPHA:
            Error = ST_NO_ERROR;
            if(GetFilterHandle(VPHandle) != 0)
            {
                Error = LAYERGFX_SetViewPortAlpha(GetHandle(GetFilterHandle
                                                            (VPHandle)),Alpha);
            }
            if(Error == ST_NO_ERROR)
            {
                Error = LAYERGFX_SetViewPortAlpha(GetHandle(VPHandle),Alpha);
            }
            break;
#endif
#if defined GFX_55XX
        case STLAYER_OMEGA1_OSD:
            Error = LAYEROSD_SetViewPortAlpha(GetHandle(VPHandle),Alpha);
            break;
        case STLAYER_OMEGA1_CURSOR:
            Error = LAYERSUB_SetViewPortAlpha(GetHandle(VPHandle),Alpha);
            break;
#if defined GFX_STILL
        case STLAYER_OMEGA1_STILL:
            Error = LAYERSTILL_SetViewPortAlpha(GetHandle(VPHandle),Alpha);
            break;
#endif
#endif
#ifdef VIDEO
        case STLAYER_OMEGA2_VIDEO1:
        case STLAYER_OMEGA2_VIDEO2:
        case STLAYER_7020_VIDEO1  :
        case STLAYER_7020_VIDEO2  :
        case STLAYER_OMEGA1_VIDEO :
        case STLAYER_SDDISPO2_VIDEO1 :
        case STLAYER_SDDISPO2_VIDEO2 :
        case STLAYER_HDDISPO2_VIDEO1 :
        case STLAYER_HDDISPO2_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO1 :
        case STLAYER_DISPLAYPIPE_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO3 :
        case STLAYER_DISPLAYPIPE_VIDEO4 :
            LAYERVID_SetViewportAlpha(GetLayerHandle(VPHandle),
                                      GetHandle(VPHandle), Alpha);
            Error = ST_NO_ERROR;
            break;
#endif
        default:
            Error = ST_ERROR_INVALID_HANDLE;
            break;
    }
    return(Error);
}

/*******************************************************************************
Name        : STLAYER_SetViewPortGain()
Description : Sets gain parameters.
Parameters  : Handle IN: Handle of the viewport, Params IN: Pointer
                to the desired gain parameters type
Assumptions :
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_BAD_PARAMETER, ST_ERROR_INVALID_HANDLE
*******************************************************************************/
ST_ErrorCode_t STLAYER_SetViewPortGain(STLAYER_ViewPortHandle_t VPHandle,
                                      STLAYER_GainParams_t  *    Gain_p)
{
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
    BOOL IsParameterUsed=FALSE;

    TestHandle(VPHandle);
    switch(GetType(VPHandle))
    {
#if defined LAYER_COMPO
        case STLAYER_COMPOSITOR:
            /* No gain in compo */
            break;
#endif
#if defined GFX_GAMMA
        case STLAYER_GAMMA_CURSOR:
        case STLAYER_GAMMA_GDP:
        case STLAYER_GAMMA_GDPVBI:
        case STLAYER_GAMMA_BKL:
            Error = ST_NO_ERROR;
            if(GetFilterHandle(VPHandle) != 0)
            {
                Error = LAYERGFX_SetViewPortGain(GetHandle(GetFilterHandle
                                                            (VPHandle)),Gain_p);
            }
            if(Error == ST_NO_ERROR)
            {
                Error = LAYERGFX_SetViewPortGain(GetHandle(VPHandle),Gain_p);
            }
            IsParameterUsed=TRUE;
            break;
        case STLAYER_GAMMA_ALPHA:
            break;
#endif
#if defined GFX_55XX
        case STLAYER_OMEGA1_OSD:
            /* No Gain in osd */
            break;
        case STLAYER_OMEGA1_CURSOR:
            /* No Gain in still */
            break;
        case STLAYER_OMEGA1_STILL:
            /* No Gain in sub */
            break;
#endif
#ifdef VIDEO
        case STLAYER_OMEGA2_VIDEO1:
        case STLAYER_OMEGA2_VIDEO2:
        case STLAYER_7020_VIDEO1  :
        case STLAYER_7020_VIDEO2  :
        case STLAYER_OMEGA1_VIDEO :
        case STLAYER_SDDISPO2_VIDEO1 :
        case STLAYER_SDDISPO2_VIDEO2 :
        case STLAYER_HDDISPO2_VIDEO1 :
        case STLAYER_HDDISPO2_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO1 :
        case STLAYER_DISPLAYPIPE_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO3 :
        case STLAYER_DISPLAYPIPE_VIDEO4 :
            /* not supported */
            break;
#endif
        default:
            Error = ST_ERROR_INVALID_HANDLE;
            break;
    }
    if ( IsParameterUsed==FALSE)
    {
        UNUSED_PARAMETER(Gain_p);
    }
    return(Error);
}

/*******************************************************************************
Name        : STLAYER_GetViewPortGain()
Description : Sets gain parameters.
Parameters  : Handle IN: Handle of the viewport, Params IN: Pointer
            to an allocated gain parameters structure, OUT: gain parameters
Assumptions :
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_INVALID_HANDLE, ST_ERROR_BAD_PARAMETER,
                ST_ERROR_FEATURE_NOT_SUPPORTED
*******************************************************************************/
ST_ErrorCode_t STLAYER_GetViewPortGain(STLAYER_ViewPortHandle_t VPHandle,
                                      STLAYER_GainParams_t  *    Gain_p)
{
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
    BOOL IsParameterUsed=FALSE;

    TestHandle(VPHandle);
    switch(GetType(VPHandle))
    {
#if defined LAYER_COMPO
        case STLAYER_COMPOSITOR:
            /* No gain in compo */
            break;
#endif
#if defined GFX_GAMMA
        case STLAYER_GAMMA_CURSOR:
        case STLAYER_GAMMA_GDP:
        case STLAYER_GAMMA_GDPVBI:
        case STLAYER_GAMMA_BKL:
            Error = LAYERGFX_GetViewPortGain(GetHandle(VPHandle),Gain_p);
            IsParameterUsed=TRUE;
            break;
        case STLAYER_GAMMA_ALPHA:
            break;
#endif
#if defined GFX_55XX
        case STLAYER_OMEGA1_OSD:
            /* No Gain in osd */
            break;
        case STLAYER_OMEGA1_STILL:
            /* No Gain in still */
            break;
        case STLAYER_OMEGA1_CURSOR:
            /* No Gain in sub */
            break;
#endif
#ifdef VIDEO
        case STLAYER_OMEGA2_VIDEO1:
        case STLAYER_OMEGA2_VIDEO2:
        case STLAYER_7020_VIDEO1  :
        case STLAYER_7020_VIDEO2  :
        case STLAYER_OMEGA1_VIDEO :
        case STLAYER_SDDISPO2_VIDEO1 :
        case STLAYER_SDDISPO2_VIDEO2 :
        case STLAYER_HDDISPO2_VIDEO1 :
        case STLAYER_HDDISPO2_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO1 :
        case STLAYER_DISPLAYPIPE_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO3 :
        case STLAYER_DISPLAYPIPE_VIDEO4 :
            /* not supported */
            break;
#endif
        default:
            Error = ST_ERROR_INVALID_HANDLE;
            break;
    }
    if ( IsParameterUsed==FALSE)
    {
        UNUSED_PARAMETER(Gain_p);
    }
    return(Error);
}
/*******************************************************************************
Name        : STLAYER_SetViewPortRecordable()
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STLAYER_SetViewPortRecordable(STLAYER_ViewPortHandle_t VPHandle,
                                             BOOL Recordable)
{
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;

    TestHandle(VPHandle);
    switch(GetType(VPHandle))
    {
#if defined LAYER_COMPO
        case STLAYER_COMPOSITOR:
            Error = LAYCOMPO_SetViewPortRecordable(GetHandle(VPHandle),
                                                       Recordable);
            break;
#endif
#if defined GFX_GAMMA
        case STLAYER_GAMMA_CURSOR:
        case STLAYER_GAMMA_GDP:
        case STLAYER_GAMMA_GDPVBI:
        case STLAYER_GAMMA_BKL:
        case STLAYER_GAMMA_ALPHA:
            Error = ST_NO_ERROR;
            if(GetFilterHandle(VPHandle) != 0)
            {
                Error = LAYERGFX_SetViewPortRecordable(GetHandle(GetFilterHandle
                                                       (VPHandle)),Recordable);
            }
            if(Error == ST_NO_ERROR)
            {
                Error = LAYERGFX_SetViewPortRecordable(GetHandle(VPHandle),
                                                       Recordable);
            }
            break;
#endif
#if defined GFX_55XX
        case STLAYER_OMEGA1_OSD:
            Error = LAYEROSD_SetViewPortRecordable(GetHandle(VPHandle),
                                                   Recordable);
            break;
        case STLAYER_OMEGA1_STILL:
        case STLAYER_OMEGA1_CURSOR:
            break;
#endif
#ifdef VIDEO
        case STLAYER_OMEGA2_VIDEO1:
        case STLAYER_OMEGA2_VIDEO2:
        case STLAYER_7020_VIDEO1  :
        case STLAYER_7020_VIDEO2  :
        case STLAYER_OMEGA1_VIDEO :
        case STLAYER_SDDISPO2_VIDEO1 :
        case STLAYER_SDDISPO2_VIDEO2 :
        case STLAYER_HDDISPO2_VIDEO1 :
        case STLAYER_HDDISPO2_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO1 :
        case STLAYER_DISPLAYPIPE_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO3 :
        case STLAYER_DISPLAYPIPE_VIDEO4 :
            /* not supported */
            break;
#endif
        default:
            Error = ST_ERROR_INVALID_HANDLE;
            break;
    }
    return(Error);
}
/*******************************************************************************
Name        : STLAYER_GetViewPortRecordable()
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STLAYER_GetViewPortRecordable(STLAYER_ViewPortHandle_t VPHandle,
                                             BOOL * Recordable_p)
{
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;

    TestHandle(VPHandle);
    switch(GetType(VPHandle))
    {
#if defined LAYER_COMPO
        case STLAYER_COMPOSITOR:
            Error = LAYCOMPO_GetViewPortRecordable(GetHandle(VPHandle),
                                                   Recordable_p);
            break;
#endif
#if defined GFX_GAMMA
        case STLAYER_GAMMA_CURSOR:
        case STLAYER_GAMMA_GDP:
        case STLAYER_GAMMA_GDPVBI:
        case STLAYER_GAMMA_BKL:
        case STLAYER_GAMMA_ALPHA:
            Error = LAYERGFX_GetViewPortRecordable(GetHandle(VPHandle),
                                                       Recordable_p);
            break;
#endif
#if defined GFX_55XX
        case STLAYER_OMEGA1_OSD:
            Error = LAYEROSD_GetViewPortRecordable(GetHandle(VPHandle),
                                                   Recordable_p);
            break;
        case STLAYER_OMEGA1_STILL:
        case STLAYER_OMEGA1_CURSOR:
            break;
#endif
#ifdef VIDEO
        case STLAYER_OMEGA2_VIDEO1:
        case STLAYER_OMEGA2_VIDEO2:
        case STLAYER_7020_VIDEO1  :
        case STLAYER_7020_VIDEO2  :
        case STLAYER_OMEGA1_VIDEO :
        case STLAYER_SDDISPO2_VIDEO1 :
        case STLAYER_SDDISPO2_VIDEO2 :
        case STLAYER_HDDISPO2_VIDEO1 :
        case STLAYER_HDDISPO2_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO1 :
        case STLAYER_DISPLAYPIPE_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO3 :
        case STLAYER_DISPLAYPIPE_VIDEO4 :
            break;
#endif
        default:
            Error = ST_ERROR_INVALID_HANDLE;
            break;
    }
    return(Error);
}
/*******************************************************************************
Name        : STLAYER_EnableViewPortFilter()
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STLAYER_EnableViewPortFilter(STLAYER_ViewPortHandle_t VPHandle,
                                                STLAYER_Handle_t FilterHandle )
{
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
    BOOL IsParameterUsed=FALSE;

    TestHandle(VPHandle);
    switch(GetType(VPHandle))
    {
#if defined LAYER_COMPO
        case STLAYER_COMPOSITOR:
            Error = LAYCOMPO_EnableViewPortFilter(GetHandle(VPHandle));
            break;
#endif
#if defined GFX_GAMMA
        case STLAYER_GAMMA_GDP:
        case STLAYER_GAMMA_GDPVBI:
            /* api part drive a 'twin viewport' */
            #if defined (ST_7109)||defined (ST_7200)
            Error = stlayer_EnableViewPortFilter(GetHandle(VPHandle), FilterHandle );
            #else
            Error = stlayer_EnableViewPortFilter(VPHandle, FilterHandle );
            #endif
            IsParameterUsed=TRUE;
            break;
#endif
#if defined GFX_55XX
        case STLAYER_OMEGA1_OSD:
            Error = LAYEROSD_EnableViewPortFilter();
            break;
        case STLAYER_OMEGA1_STILL:
        case STLAYER_OMEGA1_CURSOR:
            break;
#endif
#ifdef VIDEO
        case STLAYER_OMEGA2_VIDEO1:
        case STLAYER_OMEGA2_VIDEO2:
        case STLAYER_OMEGA1_VIDEO :
        case STLAYER_SDDISPO2_VIDEO1 :
        case STLAYER_SDDISPO2_VIDEO2 :
        case STLAYER_HDDISPO2_VIDEO1 :
        case STLAYER_HDDISPO2_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO1 :
        case STLAYER_DISPLAYPIPE_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO3 :
        case STLAYER_DISPLAYPIPE_VIDEO4 :
            /* not supported */
            break;
#endif
        default:
            Error = ST_ERROR_INVALID_HANDLE;
            break;
    }
    if ( IsParameterUsed==FALSE)
    {
        UNUSED_PARAMETER(FilterHandle);
    }
    return(Error);
}
/*******************************************************************************
Name        : STLAYER_DisableViewPortFilter()
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STLAYER_DisableViewPortFilter(STLAYER_ViewPortHandle_t VPHandle)
{
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;

    TestHandle(VPHandle);
    switch(GetType(VPHandle))
    {
#if defined LAYER_COMPO
        case STLAYER_COMPOSITOR:
            Error = LAYCOMPO_DisableViewPortFilter(GetHandle(VPHandle));
            break;
#endif
#if defined GFX_GAMMA
        case STLAYER_GAMMA_GDP:
        case STLAYER_GAMMA_GDPVBI:
            #if defined (ST_7109) || defined (ST_7200)
            Error = stlayer_DisableViewPortFilter(GetHandle(VPHandle) );
            #else
            Error = stlayer_DisableViewPortFilter(VPHandle);
            #endif

            break;
#endif
#if defined GFX_55XX
        case STLAYER_OMEGA1_OSD:
            Error = LAYEROSD_DisableViewPortFilter();
            break;
        case STLAYER_OMEGA1_STILL:
        case STLAYER_OMEGA1_CURSOR:
            break;
#endif
#ifdef VIDEO
        case STLAYER_OMEGA2_VIDEO1:
        case STLAYER_OMEGA2_VIDEO2:
        case STLAYER_OMEGA1_VIDEO :
        case STLAYER_SDDISPO2_VIDEO1 :
        case STLAYER_SDDISPO2_VIDEO2 :
        case STLAYER_HDDISPO2_VIDEO1 :
        case STLAYER_HDDISPO2_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO1 :
        case STLAYER_DISPLAYPIPE_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO3 :
        case STLAYER_DISPLAYPIPE_VIDEO4 :
            /* not supported */
            break;
#endif
        default:
            Error = ST_ERROR_INVALID_HANDLE;
            break;
    }
    return(Error);
}
/*******************************************************************************
Name        : STLAYER_AttachAlphaViewPort()
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STLAYER_AttachAlphaViewPort(STLAYER_ViewPortHandle_t VPHandle,
                                           STLAYER_Handle_t MaskedLayer)
{
    ST_ErrorCode_t          Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
    BOOL IsParameterUsed=FALSE;
#if defined GFX_GAMMA
    U32                     Ident;
    ST_DeviceName_t         DeviceName;
    STLAYER_Capability_t    Capability;
#endif

    TestHandle(VPHandle);
    switch(GetType(VPHandle))
    {
#if defined LAYER_COMPO
        case STLAYER_COMPOSITOR:
            /* No alpha plane in compo */
            break;
#endif
#if defined GFX_GAMMA
        case STLAYER_GAMMA_CURSOR:
        case STLAYER_GAMMA_GDP:
        case STLAYER_GAMMA_GDPVBI:
        case STLAYER_GAMMA_BKL:
        case STLAYER_GAMMA_FILTER:
            break;
        case STLAYER_GAMMA_ALPHA:
            /* 'Ident' can be done only in API level : get the target ident */
            strncpy(DeviceName,((stlayer_OpenElement*)(MaskedLayer))
                              ->LayerDevice->LayerName,sizeof(ST_DeviceName_t)-1);
            STLAYER_GetCapability(DeviceName,&Capability);
            switch((Capability.DeviceId)&(0x00000fff))/* use lsb to define */
            {
                /* this implementation is hardware dependant */
                case 0x100:    Ident = 3;  break;
                case 0x200:    Ident = 4;  break;
                case 0x300:    Ident = 5;  break;
                case 0x400:    Ident = 6;  break;
                case 0x700:    Ident = 1;  break;
                case 0x800:    Ident = 2;  break;

                default:       Ident = 0;  break;
            }
            Error = LAYERGFX_AttachViewPort(GetHandle(VPHandle),Ident);
            IsParameterUsed=TRUE;
            break;
#endif
#if defined GFX_55XX
        case STLAYER_OMEGA1_OSD:
            break;
        case STLAYER_OMEGA1_STILL:
            break;
#endif
#ifdef VIDEO

        case STLAYER_OMEGA2_VIDEO1:
        case STLAYER_OMEGA2_VIDEO2:
        case STLAYER_OMEGA1_VIDEO :
        case STLAYER_SDDISPO2_VIDEO1 :
        case STLAYER_SDDISPO2_VIDEO2 :
        case STLAYER_HDDISPO2_VIDEO1 :
        case STLAYER_HDDISPO2_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO1 :
        case STLAYER_DISPLAYPIPE_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO3 :
        case STLAYER_DISPLAYPIPE_VIDEO4 :
            break;
#endif
        default:
            Error = ST_ERROR_INVALID_HANDLE;
            break;
    }
    if(IsParameterUsed==FALSE)
    {
        UNUSED_PARAMETER(MaskedLayer);
    }
    return(Error);
}

/*******************************************************************************
Name        : stlayer_CleanLayerViewports
Description :
Parameters  :
Assumptions :
Limitations : not exported through API
Returns     :
*******************************************************************************/
U32 stlayer_CleanLayerViewports(stlayer_LayerElement_t * Layer)
{
    U32 i,Count;


    Count = 0;
    for (i=0; i<TOTAL_MAX_VP; i++)
    {
        if((stlayer_ViewportArray[i].Used == KEY_HANDLE_USED)
                &&(stlayer_ViewportArray[i].LayerDevice == Layer))
        {
            Count ++ ;
            /* if a filter-viewport exist, delete it */
            if(stlayer_ViewportArray[i].AssociatedFilterVPHandle != 0)
            {
                STLAYER_DisableViewPortFilter(
                        stlayer_ViewportArray[i].AssociatedFilterVPHandle);
            }
            stlayer_ViewportArray[i].Used = 0;
        }
    }
    return(Count);
}

/*******************************************************************************
Name        : stlayer_AttachFilterViewPort()
Description :
Parameters  :
Assumptions :
Limitations : not exported through API
Returns     :
*******************************************************************************/
ST_ErrorCode_t stlayer_AttachFilterViewPort(STLAYER_ViewPortHandle_t VPHandle,
                                        STLAYER_ViewPortHandle_t FilteredHandle)
{
    ST_ErrorCode_t          Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
#if defined GFX_GAMMA
    U32                     Ident;
    ST_DeviceName_t         DeviceName;
    STLAYER_Capability_t    Capability;
#endif

    TestHandle(VPHandle);
    TestHandle(FilteredHandle);
    switch(GetType(VPHandle))
    {

#if defined GFX_GAMMA
        case STLAYER_GAMMA_CURSOR:
        case STLAYER_GAMMA_GDP:
        case STLAYER_GAMMA_GDPVBI:
        case STLAYER_GAMMA_BKL:
        case STLAYER_GAMMA_ALPHA:
            break;
        case STLAYER_GAMMA_FILTER:
            /* 'Ident' can be done only in API level : get the target ident */
            strncpy(DeviceName,((stlayer_ViewportElement_t*)(FilteredHandle))
                                    ->LayerDevice->LayerName,sizeof(ST_DeviceName_t)-1);
            STLAYER_GetCapability(DeviceName,&Capability);
            switch((Capability.DeviceId)&(0x00000fff))/* use lsb to define */
            {
                /* this implementation is hardware dependant */
                case 0x100:    Ident = 1;  break;
                case 0x200:    Ident = 2;  break;
                case 0x300:    Ident = 3;  break;
                case 0x400:    Ident = 4;  break;
                default:       Ident = 0;  break;
            }
            Error = LAYERGFX_AttachViewPort(GetHandle(VPHandle),Ident);
            break;

#endif
#if defined GFX_55XX
        case STLAYER_OMEGA1_OSD:
            break;
        case STLAYER_OMEGA1_STILL:
            break;
#endif
#ifdef VIDEO
        case STLAYER_OMEGA2_VIDEO1:
        case STLAYER_OMEGA2_VIDEO2:
        case STLAYER_OMEGA1_VIDEO :
        case STLAYER_SDDISPO2_VIDEO1 :
        case STLAYER_SDDISPO2_VIDEO2 :
        case STLAYER_HDDISPO2_VIDEO1 :
        case STLAYER_HDDISPO2_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO1 :
        case STLAYER_DISPLAYPIPE_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO3 :
        case STLAYER_DISPLAYPIPE_VIDEO4 :
            break;
#endif
        default:
            Error = ST_ERROR_INVALID_HANDLE;
            break;
    }
    return(Error);
}

/*******************************************************************************
Name        : stlayer_SetVPFilter
Description : Record a filter handle associated with a viewport
Parameters  :
Assumptions :
Limitations : not exported through API
Returns     :
*******************************************************************************/
void stlayer_SetVPFilter(STLAYER_ViewPortHandle_t VPHandle,
                         STLAYER_ViewPortHandle_t FltHandle)
{
    ((stlayer_ViewportElement_t*)(VPHandle))->AssociatedFilterVPHandle
            = FltHandle;
}
/*******************************************************************************
Name        : stlayer_GetVPFilter
Description : Retrieve a filter handle associated with a viewport
Parameters  :
Assumptions :
Limitations : not exported through API
Returns     :
*******************************************************************************/
STLAYER_ViewPortHandle_t stlayer_GetVPFilter(STLAYER_ViewPortHandle_t VPHandle)
{
    return(((stlayer_ViewportElement_t*)(VPHandle))->AssociatedFilterVPHandle);
}

/*******************************************************************************
Name        : STLAYER_SetViewPortFlickerFilterMode
Description : Update The Viewport with a specified FlickerFilter Mode
Parameters  :
Assumptions :
Limitations : not exported through API
Returns     :
*******************************************************************************/


ST_ErrorCode_t STLAYER_SetViewPortFlickerFilterMode(
    STLAYER_ViewPortHandle_t          VPHandle,
    STLAYER_FlickerFilterMode_t       FlickerFilterMode)

    {

    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
    BOOL IsParameterUsed=FALSE;

    #if defined (ST_7109)
        U32 CutId;
    #endif

   #if defined (ST_7109)
    CutId = ST_GetCutRevision();
   #endif

    TestHandle(VPHandle);
    switch(GetType(VPHandle))
    {
#if defined LAYER_COMPO
        case STLAYER_COMPOSITOR:
            Error = LAYCOMPO_SetViewPortFlickerFilterMode(GetHandle(VPHandle),FlickerFilterMode);
            IsParameterUsed=TRUE;
        break;
#endif
#if defined GFX_GAMMA
        case STLAYER_GAMMA_GDP:
        case STLAYER_GAMMA_GDPVBI:
            /* api part drive a 'twin viewport' */

#if defined (ST_7109)
            if (CutId >= 0xC0)
            {
             Error = LAYERGFX_SetViewPortFlickerFilterMode(GetHandle(VPHandle),FlickerFilterMode);
             IsParameterUsed=TRUE;
             return Error ;
             }
#endif

#if defined (ST_7200)
            Error = LAYERGFX_SetViewPortFlickerFilterMode(GetHandle(VPHandle),FlickerFilterMode);
            IsParameterUsed=TRUE;
            return Error ;
#endif

            break;
#endif


#if defined GFX_55XX
        case STLAYER_OMEGA1_OSD:
            /* */
            break;
        case STLAYER_OMEGA1_STILL:
        case STLAYER_OMEGA1_CURSOR:
            break;
#endif
#ifdef VIDEO
        case STLAYER_OMEGA2_VIDEO1:
        case STLAYER_OMEGA2_VIDEO2:
        case STLAYER_OMEGA1_VIDEO :
        case STLAYER_SDDISPO2_VIDEO1 :
        case STLAYER_SDDISPO2_VIDEO2 :
        case STLAYER_HDDISPO2_VIDEO1 :
        case STLAYER_HDDISPO2_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO1 :
        case STLAYER_DISPLAYPIPE_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO3 :
        case STLAYER_DISPLAYPIPE_VIDEO4 :
            /* not supported */
            break;
#endif
        default:
            Error = ST_ERROR_INVALID_HANDLE;
            break;
    }
    if ( IsParameterUsed==FALSE)
    {
        UNUSED_PARAMETER(FlickerFilterMode);
    }

    return(Error);

   }

/*******************************************************************************
Name        : STLAYER_GetViewPortFlickerFilterMode
Description : Retrive  The FlickerFilter Mode of a specfied  Viewport
Parameters  :
Assumptions :
Limitations : not exported through API
Returns     :
*******************************************************************************/


ST_ErrorCode_t STLAYER_GetViewPortFlickerFilterMode(
    STLAYER_ViewPortHandle_t          VPHandle,
    STLAYER_FlickerFilterMode_t*      FlickerFilterMode_p)
    {

    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
    BOOL IsParameterUsed=FALSE;

    #if defined (ST_7109)
        U32 CutId;
    #endif

   #if defined (ST_7109)
    CutId = ST_GetCutRevision();
   #endif

    TestHandle(VPHandle);
    switch(GetType(VPHandle))
    {
#if defined LAYER_COMPO
        case STLAYER_COMPOSITOR:
            Error = LAYCOMPO_GetViewPortFlickerFilterMode(GetHandle(VPHandle),FlickerFilterMode_p);
            IsParameterUsed=TRUE;
        break;
#endif
#if defined GFX_GAMMA
        case STLAYER_GAMMA_GDP:
        case STLAYER_GAMMA_GDPVBI:
            /* api part drive a 'twin viewport' */

#if defined (ST_7109)
            if (CutId >= 0xC0)
            {
            Error = LAYERGFX_GetViewPortFlickerFilterMode(GetHandle(VPHandle),FlickerFilterMode_p);
            IsParameterUsed=TRUE;
            return Error;
            }
#endif

#if defined (ST_7200)
            Error = LAYERGFX_GetViewPortFlickerFilterMode(GetHandle(VPHandle),FlickerFilterMode_p);
            IsParameterUsed=TRUE;
            return Error;
#endif

            break;
#endif

#if defined GFX_55XX
        case STLAYER_OMEGA1_OSD:
            /* */
            break;
        case STLAYER_OMEGA1_STILL:
        case STLAYER_OMEGA1_CURSOR:
            break;
#endif
#ifdef VIDEO
        case STLAYER_OMEGA2_VIDEO1:
        case STLAYER_OMEGA2_VIDEO2:
        case STLAYER_OMEGA1_VIDEO :
        case STLAYER_SDDISPO2_VIDEO1 :
        case STLAYER_SDDISPO2_VIDEO2 :
        case STLAYER_HDDISPO2_VIDEO1 :
        case STLAYER_HDDISPO2_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO1 :
        case STLAYER_DISPLAYPIPE_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO3 :
        case STLAYER_DISPLAYPIPE_VIDEO4 :
            /* not supported */
            break;
#endif
        default:
            Error = ST_ERROR_INVALID_HANDLE;
            break;
    }
    if ( IsParameterUsed==FALSE)
    {
        UNUSED_PARAMETER(FlickerFilterMode_p);
    }

    return(Error);

   }


/*******************************************************************************
Name        : STLAYER_SetViewPortFlickerFilterMode
Description : Update The Viewport with a specified FlickerFilter Mode
Parameters  :
Assumptions :
Limitations : not exported through API
Returns     :
*******************************************************************************/


ST_ErrorCode_t STLAYER_EnableViewportColorFill(
    STLAYER_ViewPortHandle_t          VPHandle)

    {

    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;


    #if defined (ST_7109)
        U32 CutId;
    #endif

   #if defined (ST_7109)
    CutId = ST_GetCutRevision();
   #endif

    TestHandle(VPHandle);
    switch(GetType(VPHandle))
    {
#if defined LAYER_COMPO
        case STLAYER_COMPOSITOR:
            /*  */;
            break;
#endif
#if defined GFX_GAMMA
        case STLAYER_GAMMA_GDP:
        case STLAYER_GAMMA_GDPVBI:
            /* api part drive a 'twin viewport' */
#if defined (ST_7109)
            if (CutId >= 0xC0)
            {
            Error = LAYERGFX_EnableViewportColorFill(GetHandle(VPHandle));
            return Error ;
            }
#endif

#if defined (ST_7200)
            Error = LAYERGFX_EnableViewportColorFill(GetHandle(VPHandle));
            return Error ;
#endif

            break;
#endif
#if defined GFX_55XX
        case STLAYER_OMEGA1_OSD:
            /* */
            break;
        case STLAYER_OMEGA1_STILL:
        case STLAYER_OMEGA1_CURSOR:
            break;
#endif
#ifdef VIDEO
        case STLAYER_OMEGA2_VIDEO1:
        case STLAYER_OMEGA2_VIDEO2:
        case STLAYER_OMEGA1_VIDEO :
        case STLAYER_SDDISPO2_VIDEO1 :
        case STLAYER_SDDISPO2_VIDEO2 :
        case STLAYER_HDDISPO2_VIDEO1 :
        case STLAYER_HDDISPO2_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO1 :
        case STLAYER_DISPLAYPIPE_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO3 :
        case STLAYER_DISPLAYPIPE_VIDEO4 :
            /* not supported */
            break;
#endif
        default:
            Error = ST_ERROR_INVALID_HANDLE;
            break;
    }
    return(Error);

   }


/*******************************************************************************
Name        : STLAYER_DisableViewportColorFill
Description : Update The Viewport with a specified FlickerFilter Mode
Parameters  :
Assumptions :
Limitations : not exported through API
Returns     :
*******************************************************************************/


ST_ErrorCode_t STLAYER_DisableViewportColorFill(
    STLAYER_ViewPortHandle_t          VPHandle)

    {

    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;


    #if defined (ST_7109)
        U32 CutId;
    #endif

   #if defined (ST_7109)
    CutId = ST_GetCutRevision();
   #endif

    TestHandle(VPHandle);
    switch(GetType(VPHandle))
    {
#if defined LAYER_COMPO
        case STLAYER_COMPOSITOR:
            /*  */;
            break;
#endif
#if defined GFX_GAMMA
        case STLAYER_GAMMA_GDP:
        case STLAYER_GAMMA_GDPVBI:
            /* api part drive a 'twin viewport' */
#if defined (ST_7109)
            if (CutId >= 0xC0)
            {
            Error = LAYERGFX_DisableViewportColorFill(GetHandle(VPHandle));
            return Error ;
            }
#endif

#if defined (ST_7200)
             Error = LAYERGFX_DisableViewportColorFill(GetHandle(VPHandle));
            return Error ;
#endif

            break;
#endif
#if defined GFX_55XX
        case STLAYER_OMEGA1_OSD:
            /* */
            break;
        case STLAYER_OMEGA1_STILL:
        case STLAYER_OMEGA1_CURSOR:
            break;
#endif
#ifdef VIDEO
        case STLAYER_OMEGA2_VIDEO1:
        case STLAYER_OMEGA2_VIDEO2:
        case STLAYER_OMEGA1_VIDEO :
        case STLAYER_SDDISPO2_VIDEO1 :
        case STLAYER_SDDISPO2_VIDEO2 :
        case STLAYER_HDDISPO2_VIDEO1 :
        case STLAYER_HDDISPO2_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO1 :
        case STLAYER_DISPLAYPIPE_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO3 :
        case STLAYER_DISPLAYPIPE_VIDEO4 :
            /* not supported */
            break;
#endif
        default:
            Error = ST_ERROR_INVALID_HANDLE;
            break;
    }
    return(Error);

   }





/*******************************************************************************
Name        : STLAYER_SetViewportColorFill
Description : Update The Viewport with a specified FlickerFilter Mode
Parameters  :
Assumptions :
Limitations : not exported through API
Returns     :
*******************************************************************************/


ST_ErrorCode_t STLAYER_SetViewportColorFill(
    STLAYER_ViewPortHandle_t          VPHandle,STGXOBJ_ColorARGB_t*         ColorFill)

    {

    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
    BOOL IsParameterUsed=FALSE;

    #if defined (ST_7109)
        U32 CutId;
    #endif

   #if defined (ST_7109)
    CutId = ST_GetCutRevision();
   #endif

    TestHandle(VPHandle);
    switch(GetType(VPHandle))
    {
#if defined LAYER_COMPO
        case STLAYER_COMPOSITOR:
            /*  */;
            break;
#endif
#if defined GFX_GAMMA
        case STLAYER_GAMMA_GDP:
        case STLAYER_GAMMA_GDPVBI:
            /* api part drive a 'twin viewport' */
#if defined (ST_7109)
            if (CutId >= 0xC0)
            {
                Error = LAYERGFX_SetViewportColorFill(GetHandle(VPHandle),ColorFill);
                IsParameterUsed=TRUE;
                return Error ;
            }
#endif

#if defined (ST_7200)
                Error = LAYERGFX_SetViewportColorFill(GetHandle(VPHandle),ColorFill);
                IsParameterUsed=TRUE;
                return Error ;
#endif


            break;
#endif
#if defined GFX_55XX
        case STLAYER_OMEGA1_OSD:
            /* */
            break;
        case STLAYER_OMEGA1_STILL:
        case STLAYER_OMEGA1_CURSOR:
            break;
#endif
#ifdef VIDEO
        case STLAYER_OMEGA2_VIDEO1:
        case STLAYER_OMEGA2_VIDEO2:
        case STLAYER_OMEGA1_VIDEO :
        case STLAYER_SDDISPO2_VIDEO1 :
        case STLAYER_SDDISPO2_VIDEO2 :
        case STLAYER_HDDISPO2_VIDEO1 :
        case STLAYER_HDDISPO2_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO1 :
        case STLAYER_DISPLAYPIPE_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO3 :
        case STLAYER_DISPLAYPIPE_VIDEO4 :
            /* not supported */
            break;
#endif
        default:
            Error = ST_ERROR_INVALID_HANDLE;
            break;
    }
    if ( IsParameterUsed==FALSE)
    {
        UNUSED_PARAMETER(ColorFill);
    }

    return(Error);

   }

/*******************************************************************************
Name        : STLAYER_GetViewportColorFill()
Description : Gets the color fill associated with a specified viewport
Parameters  : Handle IN: Handle of the viewport, ColorFill IN: Pointer
            to an allocated color fill data type, OUT: returned color fill fields
Assumptions :
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_INVALID_HANDLE, ST_ERROR_BAD_PARAMETER
*******************************************************************************/
ST_ErrorCode_t STLAYER_GetViewportColorFill(STLAYER_ViewPortHandle_t VPHandle,
                                         STGXOBJ_ColorARGB_t*        ColorFill)
{
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
    BOOL IsParameterUsed=FALSE;

    #if defined (ST_7109)
        U32 CutId;
    #endif

   #if defined (ST_7109)
    CutId = ST_GetCutRevision();
   #endif


    TestHandle(VPHandle);
    switch(GetType(VPHandle))
    {
#if defined LAYER_COMPO
        case STLAYER_COMPOSITOR:
        /* No color key in compo */
        break;
#endif
#if defined GFX_GAMMA
        case STLAYER_GAMMA_CURSOR:
        case STLAYER_GAMMA_GDP:
        case STLAYER_GAMMA_GDPVBI:
        case STLAYER_GAMMA_BKL:
            /* api part drive a 'twin viewport' */

#if defined (ST_7109)
                if (CutId >= 0xC0)
                {
                Error = LAYERGFX_GetViewportColorFill(GetHandle(VPHandle),ColorFill);
                IsParameterUsed=TRUE;
                }
#endif
#if defined (ST_7200)
                Error = LAYERGFX_GetViewportColorFill(GetHandle(VPHandle),ColorFill);
                IsParameterUsed=TRUE;
#endif

        break;
        case STLAYER_GAMMA_ALPHA:
        break;

#endif

#if defined GFX_55XX
        case STLAYER_OMEGA1_OSD:
            /* no Color Key in osd */
            break;
        case STLAYER_OMEGA1_STILL:
            /* no Color Key in still */
            break;
        case STLAYER_OMEGA1_CURSOR:
            /* no Color Key in sub */
            break;
#endif
#ifdef VIDEO
        case STLAYER_OMEGA2_VIDEO1:
        case STLAYER_OMEGA2_VIDEO2:
        case STLAYER_7020_VIDEO1  :
        case STLAYER_7020_VIDEO2  :
        case STLAYER_OMEGA1_VIDEO :
        case STLAYER_SDDISPO2_VIDEO1 :
        case STLAYER_SDDISPO2_VIDEO2 :
        case STLAYER_HDDISPO2_VIDEO1 :
        case STLAYER_HDDISPO2_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO1 :
        case STLAYER_DISPLAYPIPE_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO3 :
        case STLAYER_DISPLAYPIPE_VIDEO4 :
            /* not supported */
            break;
#endif
        default:
            Error = ST_ERROR_INVALID_HANDLE;
            break;
    }

    if ( IsParameterUsed==FALSE)
    {
        UNUSED_PARAMETER(ColorFill);
    }
    return(Error);
}


/*******************************************************************************
Name        : STLAYER_SetViewPortCompositionRecurrence()
Description : Sets Composition Mode.
Parameters  :
Assumptions :
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_INVALID_HANDLE, ST_ERROR_BAD_PARAMETER
*******************************************************************************/
ST_ErrorCode_t STLAYER_SetViewPortCompositionRecurrence(STLAYER_ViewPortHandle_t        VPHandle,
                                                  const STLAYER_CompositionRecurrence_t CompositionRecurrence)
{
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;

    TestHandle(VPHandle);
    switch(GetType(VPHandle))
    {
#if defined LAYER_COMPO
        case STLAYER_COMPOSITOR:
            Error = LAYCOMPO_SetViewPortCompositionRecurrence(GetHandle(VPHandle), CompositionRecurrence);
            break;
#endif

        default:
            UNUSED_PARAMETER(CompositionRecurrence);
            Error = ST_ERROR_INVALID_HANDLE;
            break;
    }
    return(Error);
}

/*******************************************************************************
Name        : STLAYER_PerformViewPortComposition()
Description : Performs one viewport composition next Vsynch.
Parameters  :
Assumptions :
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_INVALID_HANDLE, ST_ERROR_BAD_PARAMETER
*******************************************************************************/
ST_ErrorCode_t STLAYER_PerformViewPortComposition(STLAYER_ViewPortHandle_t        VPHandle)
{
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;

    TestHandle(VPHandle);
    switch(GetType(VPHandle))
    {
#if defined LAYER_COMPO
        case STLAYER_COMPOSITOR:
            Error = LAYCOMPO_PerformViewPortComposition(GetHandle(VPHandle));
            break;
#else
            /* not supported */
            break;
#endif
        default:
            Error = ST_ERROR_INVALID_HANDLE;
            break;
    }
    return(Error);
}

/*******************************************************************************
Name        : STLAYER_InformPictureToBeDecoded()
Description :
Parameters  : VPHandle IN: Handle of the ViewPort, PictureType IN: Pointer to the
              Picture info structure.
Assumptions :
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_INVALID_HANDLE, ST_ERROR_FEATURE_NOT_SUPPORTED
*******************************************************************************/
ST_ErrorCode_t STLAYER_InformPictureToBeDecoded(const STLAYER_ViewPortHandle_t VPHandle,
                                                STGXOBJ_PictureInfos_t* PictureInfos_p)
{
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;

    TestHandle(VPHandle);
    switch(GetType(VPHandle))
    {
#if defined LAYER_COMPO
        case STLAYER_COMPOSITOR:
            Error = LAYCOMPO_InformPictureToBeDecoded(GetHandle(VPHandle),PictureInfos_p);
            break;
#endif
        default:
            UNUSED_PARAMETER(PictureInfos_p);
            Error = ST_ERROR_INVALID_HANDLE;
            break;
    }
    return(Error);
}


/*******************************************************************************
Name        : STLAYER_CommitViewPortParams
Description :
Parameters  : Handle IN: Handle of the ViewPort
Assumptions :
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_INVALID_HANDLE
*******************************************************************************/
ST_ErrorCode_t STLAYER_CommitViewPortParams(STLAYER_ViewPortHandle_t VPHandle)
{
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;

    TestHandle(VPHandle);
    switch(GetType(VPHandle))
    {
#if defined LAYER_COMPO
        case STLAYER_COMPOSITOR:
            Error = LAYCOMPO_CommitViewPortParams(GetHandle(VPHandle));
            break;
#endif
        default:
            Error = ST_ERROR_INVALID_HANDLE;
            break;
    }
    return(Error);
}


#ifdef STVID_DEBUG_GET_DISPLAY_PARAMS

/*******************************************************************************
Name        : STLAYER_GetVideoDisplayParams()
Description : This function retreive params used for VHSRC.
Parameters  : VPHandle IN: Handle of the viewport, Params_p OUT: Pointer
              to an allocated VHSRCParams data type
Assumptions :
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_BAD_PARAMETER
*******************************************************************************/
ST_ErrorCode_t STLAYER_GetVideoDisplayParams(STLAYER_ViewPortHandle_t VPHandle,
                                    STLAYER_VideoDisplayParams_t * Params_p)
{
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;

    TestHandle(VPHandle);

    switch(GetType(VPHandle))
    {
#ifdef VIDEO
        case STLAYER_HDDISPO2_VIDEO1 :
        case STLAYER_HDDISPO2_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO1 :
        case STLAYER_DISPLAYPIPE_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO3 :
        case STLAYER_DISPLAYPIPE_VIDEO4 :
            Error = LAYERVID_GetVideoDisplayParams(GetLayerHandle(VPHandle),
                                          GetHandle(VPHandle),
                                          Params_p);
            break;
#endif

        default:
            Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
            break;
    }
    return(Error);
}

#endif

#ifdef VIDEO_DEBUG_DEINTERLACING_MODE
/*******************************************************************************
Name        : STLAYER_GetRequestedDeinterlacingMode()
Description : This function can be used to know the currently requested Deinterlacing mode.
Parameters  : The Requested mode can be one of the following values:
              0: Off (nothing displayed)
              1: Reserved for future use
              2: Bypass
              3: Vertical Intperolation
              4: Directional Intperolation
              5: Field Merging
              6: Median Filter
              7: MLD Mode (=3D Mode) = Default value
              8: LMU Mode
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STLAYER_GetRequestedDeinterlacingMode(STLAYER_ViewPortHandle_t VPHandle,
                                               STLAYER_DeiMode_t * const RequestedMode_p)
{
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;

    TestHandle(VPHandle);

    switch(GetType(VPHandle))
    {
#ifdef VIDEO
        case STLAYER_HDDISPO2_VIDEO1 :
        case STLAYER_HDDISPO2_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO1 :
        case STLAYER_DISPLAYPIPE_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO3 :
        case STLAYER_DISPLAYPIPE_VIDEO4 :
            Error = LAYERVID_GetRequestedDeinterlacingMode(GetLayerHandle(VPHandle),
                                                     GetHandle(VPHandle),
                                                     RequestedMode_p);
            break;
#endif

        default:
            Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
            break;
    }
    return(Error);
}

/*******************************************************************************
Name        : STLAYER_SetRequestedDeinterlacingMode()
Description : This function can be used to request a Deinterlacing mode.

              This function is provided FOR TEST PURPOSE ONLY. By default the requested
              DEI mode should not be changed.

Parameters  : The Requested mode can be one of the following values:
              0: Off (nothing displayed)
              1: Reserved for future use
              2: Bypass
              3: Vertical Intperolation
              4: Directional Intperolation
              5: Field Merging
              6: Median Filter
              7: MLD Mode (=3D Mode) = Default value
              8: LMU Mode
Assumptions :
Limitations : The requested mode will be applied only if the necessary conditions are met
              (for example the availability of the Previous and Current fields)
Returns     :
*******************************************************************************/
ST_ErrorCode_t STLAYER_SetRequestedDeinterlacingMode(STLAYER_ViewPortHandle_t VPHandle,
                                               const STLAYER_DeiMode_t RequestedMode)
{
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;

    TestHandle(VPHandle);

    switch(GetType(VPHandle))
    {
#ifdef VIDEO
        case STLAYER_HDDISPO2_VIDEO1 :
        case STLAYER_HDDISPO2_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO1 :
        case STLAYER_DISPLAYPIPE_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO3 :
        case STLAYER_DISPLAYPIPE_VIDEO4 :
            Error = LAYERVID_SetRequestedDeinterlacingMode(GetLayerHandle(VPHandle),
                                                     GetHandle(VPHandle),
                                                     RequestedMode);
            break;
#endif

        default:
            Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
            break;
    }
    return(Error);
}
#endif /* VIDEO_DEBUG_DEINTERLACING_MODE */

#ifdef ST_XVP_ENABLE_FLEXVP
/*******************************************************************************
Name        : STLAYER_XVPInit()
Description : Init of flexVP
Parameters  : LayerHandle IN: Handle of the Layer,
              processId : identifier of the process to init
Assumptions :
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_INVALID_HANDLE, ST_ERROR_BAD_PARAMETER
*******************************************************************************/

ST_ErrorCode_t STLAYER_XVPInit( const STLAYER_ViewPortHandle_t      VPHandle,
                                U32                                 ProcessId)
{
    ST_ErrorCode_t          Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
    STLAYER_XVPHandle_t     XVPHandle;

    TestHandle(VPHandle);
    switch(GetType(VPHandle))
    {
        case STLAYER_DISPLAYPIPE_VIDEO2 :
            Error = XVP_Init(GetHandle(VPHandle), ProcessId, &XVPHandle);
            break;

        default:
            Error = ST_ERROR_INVALID_HANDLE;
            break;
    }
    if (!Error)
    {
        ((stlayer_ViewportElement_t*)(VPHandle))->XVPHandle = XVPHandle;
    }
    return(Error);
}

/*******************************************************************************
Name        : STLAYER_XVPTerm()
Description : Term of flexVP
Parameters  : LayerHandle IN: Handle of the Layer

Assumptions :
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_INVALID_HANDLE, ST_ERROR_BAD_PARAMETER
*******************************************************************************/

ST_ErrorCode_t STLAYER_XVPTerm(const STLAYER_ViewPortHandle_t VPHandle)
{
    ST_ErrorCode_t          Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
    STLAYER_XVPHandle_t     XVPHnd;

    TestHandle(VPHandle);
    switch(GetType(VPHandle))
    {
        case STLAYER_DISPLAYPIPE_VIDEO2 :
            XVPHnd = GetXVPHandle(VPHandle);
            if (XVPHnd)
            {
                Error = XVP_Term(XVPHnd);
            }
            break;

        default:
            Error = ST_ERROR_INVALID_HANDLE;
            break;
    }
    if (!Error)
    {
        ((stlayer_ViewportElement_t*)(VPHandle))->XVPHandle = 0;
    }
    return(Error);
}

/*******************************************************************************
Name        : STLAYER_XVPSetParam()
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/

ST_ErrorCode_t STLAYER_XVPSetParam( const STLAYER_ViewPortHandle_t  VPHandle,
                                    STLAYER_XVPParam_t              *Param_p)
{
    ST_ErrorCode_t          Error = ST_NO_ERROR;
    STLAYER_XVPHandle_t     XVPHnd;

    TestHandle(VPHandle);
    XVPHnd = GetXVPHandle(VPHandle);
    if (XVPHnd)
    {
        Error = XVP_SetParam(XVPHnd, Param_p);
    }
    return(Error);
}

/*******************************************************************************
Name        : STLAYER_XVPSetExtraParam()
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/

ST_ErrorCode_t STLAYER_XVPSetExtraParam(    const STLAYER_ViewPortHandle_t  VPHandle,
                                            STLAYER_XVPExtraParam_t         *ExtraParam_p)
{
    ST_ErrorCode_t          Error = ST_NO_ERROR;
    STLAYER_XVPHandle_t     XVPHnd;

    XVPHnd = GetXVPHandle(VPHandle);
    if (XVPHnd)
    {
        Error = XVP_SetExtraParam(XVPHnd, ExtraParam_p);
    }
    return(Error);
}

/*******************************************************************************
Name        : STLAYER_XVPCompute()
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/

ST_ErrorCode_t STLAYER_XVPCompute(const STLAYER_ViewPortHandle_t  VPHandle)
{
    ST_ErrorCode_t          Error = ST_NO_ERROR;
    STLAYER_XVPHandle_t     XVPHnd;

    TestHandle(VPHandle);
    XVPHnd = GetXVPHandle(VPHandle);
    if (XVPHnd)
    {
        Error = XVP_Compute(XVPHnd);
    }
    return(Error);
}
#endif /* ST_XVP_ENABLE_FLEXVP */

#ifdef STLAYER_USE_FMD
/*******************************************************************************
Name        : STLAYER_EnableFMDReporting()
Description : This function can be used to enable the FMD reporting.
Parameters  : VPHandle IN: Handle of the viewport
Assumptions :
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_BAD_PARAMETER
*******************************************************************************/
ST_ErrorCode_t STLAYER_EnableFMDReporting(STLAYER_ViewPortHandle_t VPHandle)
{
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;

    TestHandle(VPHandle);

    switch(GetType(VPHandle))
    {
#ifdef VIDEO
        case STLAYER_HDDISPO2_VIDEO1 :
        case STLAYER_HDDISPO2_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO1 :
        case STLAYER_DISPLAYPIPE_VIDEO2 :
            Error = LAYERVID_EnableFMDReporting(GetLayerHandle(VPHandle),
                                                GetHandle(VPHandle));
            break;
#endif

        default:
            Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
            break;
    }
    return(Error);
}

/*******************************************************************************
Name        : STLAYER_DisableFMDReporting()
Description : This function can be used to disable the FMD reporting.
Parameters  : VPHandle IN: Handle of the viewport
Assumptions :
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_BAD_PARAMETER
*******************************************************************************/
ST_ErrorCode_t STLAYER_DisableFMDReporting(STLAYER_ViewPortHandle_t VPHandle)
{
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;

    TestHandle(VPHandle);

    switch(GetType(VPHandle))
    {
#ifdef VIDEO
        case STLAYER_HDDISPO2_VIDEO1 :
        case STLAYER_HDDISPO2_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO1 :
        case STLAYER_DISPLAYPIPE_VIDEO2 :
            Error = LAYERVID_DisableFMDReporting(GetLayerHandle(VPHandle),
                                                 GetHandle(VPHandle));
            break;
#endif

        default:
            Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
            break;
    }
    return(Error);
}

/*******************************************************************************
Name        : STLAYER_GetFMDParams()
Description : This function retreive params used for FMD.
Parameters  : VPHandle IN: Handle of the viewport, Params_p OUT: Pointer
              to an allocated FMDParams data type
Assumptions :
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_BAD_PARAMETER
*******************************************************************************/
ST_ErrorCode_t STLAYER_GetFMDParams(STLAYER_ViewPortHandle_t VPHandle,
                                    STLAYER_FMDParams_t * Params_p)
{
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;

    TestHandle(VPHandle);

    switch(GetType(VPHandle))
    {
#ifdef VIDEO
        case STLAYER_HDDISPO2_VIDEO1 :
        case STLAYER_HDDISPO2_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO1 :
        case STLAYER_DISPLAYPIPE_VIDEO2 :
            Error = LAYERVID_GetFMDParams(GetLayerHandle(VPHandle),
                                          GetHandle(VPHandle),
                                          Params_p);
            break;
#endif

        default:
            Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
            break;
    }
    return(Error);
}

/*******************************************************************************
Name        : STLAYER_SetFMDParams()
Description : This function set params used for FMD.
Parameters  : VPHandle IN: Handle of the viewport, Params_p IN: Pointer
              to an allocated FMDParams data type
Assumptions :
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_BAD_PARAMETER
*******************************************************************************/
ST_ErrorCode_t STLAYER_SetFMDParams(STLAYER_ViewPortHandle_t VPHandle,
                                    const STLAYER_FMDParams_t * Params_p)
{
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;

    TestHandle(VPHandle);

    switch(GetType(VPHandle))
    {
#ifdef VIDEO
        case STLAYER_HDDISPO2_VIDEO1 :
        case STLAYER_HDDISPO2_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO1 :
        case STLAYER_DISPLAYPIPE_VIDEO2 :
            Error = LAYERVID_SetFMDParams(GetLayerHandle(VPHandle),
                                          GetHandle(VPHandle),
                                          Params_p);
            break;
#endif

        default:
            Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
            break;
    }
    return(Error);
}

#endif /* STLAYER_USE_FMD */

