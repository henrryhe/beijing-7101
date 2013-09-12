/*****************************************************************************

File name   : lc_vp.c

Description : Layer compo Viewport source file

COPYRIGHT (C) STMicroelectronics 2004

Date               Modification                                     Name
----               ------------                                     ----
Sept 2003        Created                                           TM
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */


/* Includes ----------------------------------------------------------------- */

#include <string.h>
#include "stddefs.h"
#include "stlayer.h"
#include "lc_vp.h"
#include "stgxobj.h"
#include "lc_init.h"
#include "lc_pool.h"
#include "sttbx.h"
#include "laycompo.h"
#include "stsys.h"
#include "stvtg.h"


/* Private Types ------------------------------------------------------------ */


/* Private Constants -------------------------------------------------------- */

/* Private Variables (static)------------------------------------------------ */


/* Global Variables --------------------------------------------------------- */


/* Private Macros ----------------------------------------------------------- */


/* Private Function prototypes ---------------------------------------------- */
static ST_ErrorCode_t RemoveViewPortFromList(STLAYER_ViewPortHandle_t VPHandle);
static ST_ErrorCode_t AddViewPortToList(STLAYER_ViewPortHandle_t VPHandle);
static ST_ErrorCode_t CheckViewPortParams(laycompo_Device_t* Device_p, STLAYER_ViewPortParams_t* Params_p);
static ST_ErrorCode_t CheckViewPortRectangle(laycompo_Device_t* Device_p,STGXOBJ_Bitmap_t* InputBitmap_p,
                                             STGXOBJ_Rectangle_t*  InputRectangle_p, STGXOBJ_Rectangle_t*  OutputRectangle_p);
static ST_ErrorCode_t CheckViewPortSource(STLAYER_ViewPortSource_t* Source_p);


/* Private Function --------------------------------------------------------- */

/*******************************************************************************
Name        : CheckViewPortRectangle
Description :
Parameters  :
Assumptions : Non NULL Params
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t CheckViewPortRectangle(laycompo_Device_t*    Device_p,
                                             STGXOBJ_Bitmap_t*     InputBitmap_p,
                                             STGXOBJ_Rectangle_t*  InputRectangle_p,
                                             STGXOBJ_Rectangle_t*  OutputRectangle_p)
{
    UNUSED_PARAMETER(Device_p);
    UNUSED_PARAMETER(InputBitmap_p);
    UNUSED_PARAMETER(InputRectangle_p);
    UNUSED_PARAMETER(OutputRectangle_p);

    return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : CheckViewPortSource
Description :
Parameters  :
Assumptions : Non NULL Source_p
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t CheckViewPortSource(STLAYER_ViewPortSource_t* Source_p)
{
    UNUSED_PARAMETER(Source_p);
    return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : CheckViewPortParams
Description :
Parameters  :
Assumptions : Non NULL Params_p
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t CheckViewPortParams(laycompo_Device_t* Device_p, STLAYER_ViewPortParams_t* Params_p)
{
    ST_ErrorCode_t          Err = ST_NO_ERROR;

    /* Check Source */
    if (Params_p->Source_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
    Err = CheckViewPortSource(Params_p->Source_p);
    if (Err != ST_NO_ERROR)
    {
        return(Err);
    }

    /* Check Rectangles */
    Err = CheckViewPortRectangle(Device_p,Params_p->Source_p->Data.BitMap_p,&(Params_p->InputRectangle), &(Params_p->OutputRectangle));
    if (Err != ST_NO_ERROR)
    {
        return(Err);
    }

    return(Err);
}


/*******************************************************************************
Name        : CreateAllVPDisplayFromList
Description :
Parameters  :
Assumptions : Recursive function:
                + Need to be called from LastOpenViewport in order to have FirstOpenViewPort at the Back and LastOpenViewport
                  at the Front in STDISP/COMPO VPDisplay/Overlay lists.
                  This is important that the viewport list follows the VPDisplay/Overlay list (see STDISP/COMPO) for Order mgnt reasons
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t CreateAllVPDisplayFromList(STLAYER_ViewPortHandle_t VPHandle)
{
    ST_ErrorCode_t              Err           = ST_NO_ERROR;
    laycompo_ViewPort_t*        ViewPort_p    = (laycompo_ViewPort_t*) VPHandle;
	laycompo_Device_t* 		    Device_p      = (laycompo_Device_t*)(((laycompo_ViewPort_t*)VPHandle)->Device_p);
    STDISP_ViewPortParams_t     VPDisplayParams;
    STDISP_ViewPortSource_t     VPSource;
    STDISP_StreamingVideo_t     VideoStream;
    STDISP_CompositionRecurrence_t    DispCompositionRecurrence;
    STDISP_FlickerFilterMode_t        DispFlickerFilterMode;


    if (ViewPort_p->PreviousViewPort != LAYCOMPO_NO_VIEWPORT_HANDLE)
    {
        CreateAllVPDisplayFromList(ViewPort_p->PreviousViewPort);
    }

    /* Set ViewPort Display params */
    VPSource.SourceType = ViewPort_p->Source.SourceType;
    if (VPSource.SourceType == STLAYER_STREAMING_VIDEO)
    {
        VideoStream.Bitmap           = ViewPort_p->Source.Data.VideoStream_p->BitmapParams;
        VideoStream.ScanType         = ViewPort_p->Source.Data.VideoStream_p->ScanType;
        VideoStream.FieldInversion   = ViewPort_p->Source.Data.VideoStream_p->PresentedFieldInverted;
        VPSource.Data.VideoStream_p  = &VideoStream;
        VPSource.Palette_p           = NULL;
    }
    else  /* STLAYER_GRAPHIC_BITMAP */
    {
        VPSource.Data.Bitmap_p = ViewPort_p->Source.Data.BitMap_p;
        VPSource.Palette_p     = ViewPort_p->Source.Palette_p;
    }
    VPDisplayParams.Source_p        = &VPSource;
    VPDisplayParams.InputRectangle  = ViewPort_p->InputRectangle;
    VPDisplayParams.OutputRectangle = ViewPort_p->OutputRectangle;

    /* Open ViewPort Display : It is open at front of current list */
    Err = STDISP_OpenViewPort(Device_p->DisplayHandle,&VPDisplayParams,&(ViewPort_p->VPDisplay));
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_ERROR,"Err STDISP_OpenViewPort : %d\n",Err));
        return(Err);
    }

    if (ViewPort_p->GlobalAlpha.A0 != 128)    /* Use only A0 for the moment? */
    {
        STDISP_GlobalAlpha_t VPDisplayAlpha;

        VPDisplayAlpha.A0 = ViewPort_p->GlobalAlpha.A0;
        VPDisplayAlpha.A1 = ViewPort_p->GlobalAlpha.A1;

        Err = STDISP_SetViewPortAlpha(ViewPort_p->VPDisplay,&VPDisplayAlpha);
        if (Err != ST_NO_ERROR)
        {
            STTBX_Report ((STTBX_REPORT_LEVEL_ERROR,"Err STDISP_SetViewPortAlpha : %d\n",Err));
            return(Err);
        }
    }

    /* Set Composition Recurrence mode */
    switch (ViewPort_p->CompositionRecurrence)
    {
        case STLAYER_COMPOSITION_RECURRENCE_EVERY_VSYNC :
            DispCompositionRecurrence = STDISP_COMPOSITION_RECURRENCE_EVERY_VSYNC;
            break;

        case STLAYER_COMPOSITION_RECURRENCE_MANUAL_OR_VIEWPORT_PARAMS_CHANGES :
            DispCompositionRecurrence = STDISP_COMPOSITION_RECURRENCE_MANUAL_OR_VIEWPORT_PARAMS_CHANGES;
            break;

        default:
            DispCompositionRecurrence = STDISP_COMPOSITION_RECURRENCE_EVERY_VSYNC;
            break;
    }

    Err = STDISP_SetViewPortCompositionRecurrence(ViewPort_p->VPDisplay, DispCompositionRecurrence);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_ERROR,"Err STDISP_SetViewPortCompositionRecurrence : %d\n",Err));
        return(Err);
    }

    /* Set Flicker Filter mode */
    switch (ViewPort_p->FlickerFilterMode)
    {
        case STLAYER_FLICKER_FILTER_MODE_SIMPLE :
            DispFlickerFilterMode = STDISP_FLICKER_FILTER_MODE_SIMPLE;
            break;

        case STLAYER_FLICKER_FILTER_MODE_USING_BLITTER :
            DispFlickerFilterMode = STDISP_FLICKER_FILTER_MODE_USING_BLITTER;
            break;

        default:
            DispFlickerFilterMode = STDISP_FLICKER_FILTER_MODE_SIMPLE;
            break;
    }

    Err = STDISP_SetViewPortFlickerFilterMode(ViewPort_p->VPDisplay, DispFlickerFilterMode);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_ERROR,"Err STDISP_SetViewPortFlickerFilterMode : %d\n",Err));
        return(Err);
    }

    if (ViewPort_p->Enable)
    {
        Err = STDISP_EnableViewPort(ViewPort_p->VPDisplay);
        if (Err != ST_NO_ERROR)
        {
            STTBX_Report ((STTBX_REPORT_LEVEL_ERROR,"Err STDISP_EnableViewPort : %d\n",Err));
            return(Err);
        }
    }

    return(Err);
}

/*******************************************************************************
Name        : DestroyAllVPDisplayFromList
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t DestroyAllVPDisplayFromList(STLAYER_ViewPortHandle_t VPHandle)
{
    ST_ErrorCode_t              Err           = ST_NO_ERROR;
    laycompo_ViewPort_t*        ViewPort_p    = (laycompo_ViewPort_t*) VPHandle;

    if (ViewPort_p->NextViewPort != LAYCOMPO_NO_VIEWPORT_HANDLE)
    {
        DestroyAllVPDisplayFromList(ViewPort_p->NextViewPort);
    }

    /* Close Viewport Display */
    Err = STDISP_CloseViewPort(ViewPort_p->VPDisplay);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_ERROR,"Err STDISP_CloseViewPort : %d\n",Err));
        return(Err);
    }

    /* Update Viewport descriptor */
    ViewPort_p->VPDisplay = LAYCOMPO_NO_VPDISPLAY_HANDLE;

    return(Err);
}

/*******************************************************************************
Name        : AddViewPortToList
Description : Add viewport in list of open viewports contained in laycompo_Device_t
Parameters  :
Assumptions : VPHandle valid and non NULL
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t AddViewPortToList(STLAYER_ViewPortHandle_t VPHandle)
{
    ST_ErrorCode_t          Err         = ST_NO_ERROR;
    laycompo_ViewPort_t*    ViewPort_p  = (laycompo_ViewPort_t*) VPHandle;
	laycompo_Device_t* 		Device_p    = (laycompo_Device_t*)(((laycompo_ViewPort_t*)VPHandle)->Device_p);

    /* Update viewport descriptor */
    ViewPort_p->NextViewPort        = LAYCOMPO_NO_VIEWPORT_HANDLE;
    ViewPort_p->PreviousViewPort    = Device_p->LastOpenViewPort;

    /* Update laycompo_unit_t */
    if (Device_p->NbOpenViewPorts == 0)
    {
        Device_p->FirstOpenViewPort = VPHandle;
    }
    else /* LastOpenViewPort != LAYCOMPO_NO_VIEWPORT_HANDLE */
    {
        ((laycompo_ViewPort_t*)(Device_p->LastOpenViewPort))->NextViewPort = VPHandle;
    }
    (Device_p->NbOpenViewPorts)++;
    Device_p->LastOpenViewPort = VPHandle;

    return(Err);
}

/*******************************************************************************
Name        : RemoveViewPortFromList
Description : Remove viewport in list of open viewports contained in laycompo_Unit_t
Parameters  :
Assumptions : Viewport is in list and only once!
              VPHandle non null and valid
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t RemoveViewPortFromList(STLAYER_ViewPortHandle_t VPHandle)
{
    ST_ErrorCode_t             Err                 = ST_NO_ERROR;
    laycompo_ViewPort_t*       ViewPort_p          = (laycompo_ViewPort_t*) VPHandle;
	laycompo_Device_t* 		   Device_p            = (laycompo_Device_t*)(((laycompo_ViewPort_t*)VPHandle)->Device_p);
    STLAYER_ViewPortHandle_t   PreviousViewPort;
    STLAYER_ViewPortHandle_t   NextViewPort;

    /* Update previous and next viewport descriptor */
    PreviousViewPort = ViewPort_p->PreviousViewPort ;
    NextViewPort     = ViewPort_p->NextViewPort ;
    if (PreviousViewPort != LAYCOMPO_NO_VIEWPORT_HANDLE)
    {
        ((laycompo_ViewPort_t*)PreviousViewPort)->NextViewPort = NextViewPort;
    }
    else
    {
        /* Update laycompo_unit_t */
        Device_p->FirstOpenViewPort  = NextViewPort;
    }

    if (NextViewPort != LAYCOMPO_NO_VIEWPORT_HANDLE)
    {
        ((laycompo_ViewPort_t*)NextViewPort)->PreviousViewPort = PreviousViewPort;
    }
    else
    {
        /* Update laycompo_unit_t */
        Device_p->LastOpenViewPort  = PreviousViewPort;
    }

    (Device_p->NbOpenViewPorts)--;

    return(Err);
}

/*******************************************************************************
Name        : SetAllVPDisplayAtFront
Description :
Parameters  :
Assumptions : Recursive function:
                  Need to be called from LastOpenViewport in order to have FirstOpenViewPort at the Back and LastOpenViewport
                  at the Front in STDISP/COMPO VPDisplay/Overlay lists.
                  This is important that the viewport list follows the VPDisplay/Overlay list (see STDISP/COMPO) for Order mgnt reasons
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t SetAllVPDisplayAtFront(STLAYER_ViewPortHandle_t VPHandle)
{
    ST_ErrorCode_t              Err                    = ST_NO_ERROR;
    laycompo_ViewPort_t*        ViewPort_p             = (laycompo_ViewPort_t*) VPHandle;
    STDISP_ViewPortHandle_t     ReferenceVPDisplayHandle = (STDISP_ViewPortHandle_t) NULL;  /* dummy reference not used within STDISP */

    if (ViewPort_p->PreviousViewPort != LAYCOMPO_NO_VIEWPORT_HANDLE)
    {
        SetAllVPDisplayAtFront(ViewPort_p->PreviousViewPort);
    }

    /* Set ViewPort Display Order to Front */
    Err = STDISP_SetViewPortOrder(ViewPort_p->VPDisplay,STDISP_VIEWPORT_ORDER_SEND_TO_FRONT,ReferenceVPDisplayHandle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_ERROR,"Err STDISP_SetViewPortOrder : %d\n",Err));
        return(Err);
    }

    return(Err);
}


/* Functions ---------------------------------------------------------------- */

/*******************************************************************************
Name        : laycompo_CloseAllViewPortFromList
Description : Close all viewports from list of open viewports contained in laycompo_Device_t
              Recursive function.
Parameters  :
Assumptions : VPHandle != LAYCOMPO_NO_VIEWPORT_HANDLE and valid
              The update of Device_p open ViewPort list must be done at upper level
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t laycompo_CloseAllViewPortFromList(STLAYER_ViewPortHandle_t VPHandle)
{
    ST_ErrorCode_t              Err           = ST_NO_ERROR;
    laycompo_ViewPort_t*        ViewPort_p    = (laycompo_ViewPort_t*) VPHandle;
	laycompo_Device_t* 		    Device_p      = (laycompo_Device_t*)(((laycompo_ViewPort_t*)VPHandle)->Device_p);

    if (ViewPort_p->NextViewPort != LAYCOMPO_NO_VIEWPORT_HANDLE)
    {
        laycompo_CloseAllViewPortFromList(ViewPort_p->NextViewPort);
    }

    if (Device_p->MixerConnected)
    {
        Err = STDISP_CloseViewPort(ViewPort_p->VPDisplay);
        if (Err != ST_NO_ERROR)
        {
            STTBX_Report ((STTBX_REPORT_LEVEL_ERROR,"Err STDISP_CloseViewPort : %d\n",Err));
            return(Err);
        }
    }

    if (ViewPort_p->Enable)
    {
        /* Disable Viewport */
        ViewPort_p->Enable       = FALSE;
    }

    /* Update Viewport descriptor */
    ViewPort_p->ViewPortValidity = (U32)~LAYCOMPO_VALID_VIEWPORT;

    /* Release Viewport */
    laycompo_ReleaseElement(&(Device_p->ViewPortDataPool), (void*) VPHandle);

    return(Err);
}

/*******************************************************************************
Name        : LAYCOMPO_OpenViewPort
Description :
Parameters  :
Assumptions : Output PositionY is always even, No parity cheching : First implementation.
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYCOMPO_OpenViewPort(STLAYER_Handle_t            Handle,
                                     STLAYER_ViewPortParams_t*   Params_p,
                                     STLAYER_ViewPortHandle_t*   VPHandle_p)
{
    ST_ErrorCode_t              Err         		= ST_NO_ERROR;
    laycompo_Unit_t*            Unit_p      		= (laycompo_Unit_t*)Handle;
    laycompo_Device_t*          Device_p;
    laycompo_ViewPort_t*        ViewPort_p;

    /* Check Handle validity */
    if (Handle == (STLAYER_Handle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    if (Unit_p->UnitValidity != LAYCOMPO_VALID_UNIT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Check general params */
    if ((Params_p == NULL) || (VPHandle_p == NULL))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Get Device structreu */
    Device_p = Unit_p->Device_p ;

    /* Protect access */
    STOS_SemaphoreWait(Device_p->CtrlAccess_p);

    /* Check ViewPort params */
    Err = CheckViewPortParams(Device_p, Params_p);
    if (Err != ST_NO_ERROR)
    {
        /* Free access */
        STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

        return(Err);
    }

    /* Look if Parameters are adjusted */

    /* Get ViewPort descriptor handle*/
    Err = laycompo_GetElement(&(Device_p->ViewPortDataPool), (void**) VPHandle_p);
    if (Err != ST_NO_ERROR)
    {
        /* Free access */
        STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

        return(STLAYER_ERROR_NO_FREE_HANDLES);
    }

    ViewPort_p = (laycompo_ViewPort_t*) *VPHandle_p;

    /* Set ViewPort descriptor */
    ViewPort_p->InputRectangle      = Params_p->InputRectangle;
    ViewPort_p->OutputRectangle     = Params_p->OutputRectangle;
    ViewPort_p->GlobalAlpha.A0      = 128; /* Opaque */
    ViewPort_p->Device_p            = Device_p;
    ViewPort_p->ViewPortValidity    = LAYCOMPO_VALID_VIEWPORT;
    ViewPort_p->Enable              = FALSE;
    ViewPort_p->Source.SourceType   = Params_p->Source_p->SourceType;
    ViewPort_p->FlickerFilterOn     = FALSE;
    ViewPort_p->CompositionRecurrence     = STDISP_COMPOSITION_RECURRENCE_EVERY_VSYNC;
    ViewPort_p->FlickerFilterMode         = STLAYER_FLICKER_FILTER_MODE_SIMPLE;


    if (Params_p->Source_p->SourceType == STLAYER_STREAMING_VIDEO)
    {
        /* Allocate for Viewport VideoStream data */
        ViewPort_p->Source.Data.VideoStream_p = (STLAYER_StreamingVideo_t*) STOS_MemoryAllocate(Device_p->CPUPartition_p,sizeof(STLAYER_StreamingVideo_t));
        if (ViewPort_p->Source.Data.VideoStream_p == NULL)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "CPU memory allocation failed"));

            /* Free access */
            STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

            return(ST_ERROR_NO_MEMORY);
        }
        *(ViewPort_p->Source.Data.VideoStream_p) = *(Params_p->Source_p->Data.VideoStream_p);
        ViewPort_p->Source.Palette_p             = NULL;
    }
    else  /* STLAYER_GRAPHIC_BITMAP */
    {
        /* Allocate for Viewport Bitmap data */
        ViewPort_p->Source.Data.BitMap_p = (STGXOBJ_Bitmap_t*) STOS_MemoryAllocate(Device_p->CPUPartition_p,sizeof(STGXOBJ_Bitmap_t));
        if (ViewPort_p->Source.Data.BitMap_p == NULL)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "CPU memory allocation failed"));

            /* Free access */
            STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

            return(ST_ERROR_NO_MEMORY);
        }
        *(ViewPort_p->Source.Data.BitMap_p) = *(Params_p->Source_p->Data.BitMap_p);

        /* Allocate for Viewport Palette Data */
        if (Params_p->Source_p->Palette_p != NULL)
        {
            ViewPort_p->Source.Palette_p = (STGXOBJ_Palette_t*) STOS_MemoryAllocate(Device_p->CPUPartition_p,sizeof(STGXOBJ_Palette_t));
            if (ViewPort_p->Source.Palette_p == NULL)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "CPU memory allocation failed"));

                /* Deallocate Viewport Bitmtp Data */
                STOS_MemoryDeallocate(Device_p->CPUPartition_p, ViewPort_p->Source.Data.BitMap_p);

                /* Free access */
                STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

                return(ST_ERROR_NO_MEMORY);
            }

            *(ViewPort_p->Source.Palette_p) = *(Params_p->Source_p->Palette_p);
        }
        else
        {
            ViewPort_p->Source.Palette_p = NULL;
        }
    }

    /* Update Open Viewport list */
    AddViewPortToList(*VPHandle_p);

    if (Device_p->MixerConnected)
    {
        STDISP_ViewPortParams_t     VPDisplayParams;
        STDISP_ViewPortSource_t     VPSource;
        STDISP_StreamingVideo_t     VideoStream;

        /* Set ViewPort Display params */
        VPSource.SourceType = ViewPort_p->Source.SourceType;
        if (VPSource.SourceType == STLAYER_STREAMING_VIDEO)
        {
            VideoStream.Bitmap           = ViewPort_p->Source.Data.VideoStream_p->BitmapParams;
            VideoStream.ScanType         = ViewPort_p->Source.Data.VideoStream_p->ScanType;
            VideoStream.FieldInversion   = ViewPort_p->Source.Data.VideoStream_p->PresentedFieldInverted;
            VPSource.Data.VideoStream_p  = &VideoStream;
            VPSource.Palette_p           = NULL;
        }
        else  /* STLAYER_GRAPHIC_BITMAP */
        {
            VPSource.Data.Bitmap_p = ViewPort_p->Source.Data.BitMap_p;
            VPSource.Palette_p     = ViewPort_p->Source.Palette_p;
        }
        VPDisplayParams.Source_p        = &VPSource;
        VPDisplayParams.InputRectangle  = ViewPort_p->InputRectangle;
        VPDisplayParams.OutputRectangle = ViewPort_p->OutputRectangle;

        /* Open ViewPort Display */
        Err = STDISP_OpenViewPort(Device_p->DisplayHandle,&VPDisplayParams,&(ViewPort_p->VPDisplay));
        if (Err != ST_NO_ERROR)
        {
            STTBX_Report ((STTBX_REPORT_LEVEL_ERROR,"Err STDISP_OpenViewPort : %d\n",Err));

            /* Update Open Viewport list */
            RemoveViewPortFromList(*VPHandle_p);

            /* Update Viewport descriptor */
            ViewPort_p->ViewPortValidity = (U32)~LAYCOMPO_VALID_VIEWPORT;

            /* Deallocate Viewport Bitmap/VideoStream data & Palette if any */
            if (Params_p->Source_p->SourceType == STLAYER_STREAMING_VIDEO)
            {
                /* Deallocate for Viewport VideoStream data */
                STOS_MemoryDeallocate(Device_p->CPUPartition_p, ViewPort_p->Source.Data.VideoStream_p);
            }
            else  /* STLAYER_GRAPHIC_BITMAP */
            {
                /* Deallocate for Viewport Bitmap data */
                STOS_MemoryDeallocate(Device_p->CPUPartition_p, ViewPort_p->Source.Data.BitMap_p);

                if (Params_p->Source_p->Palette_p != NULL)
                {
                    /* Deallocate Viewport Palette Data */
                    STOS_MemoryDeallocate(Device_p->CPUPartition_p, ViewPort_p->Source.Palette_p);
                }
            }

            /* Release Viewport */
            laycompo_ReleaseElement(&(Device_p->ViewPortDataPool), (void*) *VPHandle_p);

            /* Free access */
            STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

            return (Err);
        }

        /* By default Viewport is Open at Front. Need to place ViewPort at the right Z-position (according to Layer connection) */
        if (Device_p->FrontLayer != LAYCOMPO_NO_LAYER_HANDLE)
        {
            STDISP_ViewPortHandle_t     ReferenceVPDisplayHandle = (STDISP_ViewPortHandle_t)NULL;
            STDISP_ViewPortOrder_t      Where;

            if (Device_p->BackLayer == LAYCOMPO_NO_LAYER_HANDLE)
            {
                Where = STDISP_VIEWPORT_ORDER_SEND_TO_BACK;
            }
            else
            {
                /* Find reference Viewport :
                   Be carefull because Back/Front layers can have 0 Viewport inside!! Need to go through all connected layers. */

                if (((laycompo_Unit_t*)(Device_p->BackLayer))->Device_p->NbOpenViewPorts != 0)
                {
                    STLAYER_ViewPortHandle_t  ReferenceViewPort = ((laycompo_Unit_t*)(Device_p->BackLayer))->Device_p->LastOpenViewPort;

                    Where = STDISP_VIEWPORT_ORDER_INFRONT;
                    ReferenceVPDisplayHandle = ((laycompo_ViewPort_t*)ReferenceViewPort)->VPDisplay;
                }
                else if ((((laycompo_Unit_t*)(Device_p->FrontLayer))->Device_p->NbOpenViewPorts != 0))
                {
                    STLAYER_ViewPortHandle_t  ReferenceViewPort = ((laycompo_Unit_t*)(Device_p->FrontLayer))->Device_p->FirstOpenViewPort;

                    Where = STDISP_VIEWPORT_ORDER_BEHIND;
                    ReferenceVPDisplayHandle = ((laycompo_ViewPort_t*)ReferenceViewPort)->VPDisplay;
                }
                else
                {
                    laycompo_Unit_t* ReferenceUnit_p  = (laycompo_Unit_t*)Device_p->BackLayer;

                    for (ReferenceUnit_p = (laycompo_Unit_t*)Device_p->BackLayer;
                         ReferenceUnit_p != NULL;
                         ReferenceUnit_p = (laycompo_Unit_t*) ReferenceUnit_p->Device_p->BackLayer )
                    {
                        if (ReferenceUnit_p->Device_p->NbOpenViewPorts != 0)
                        {
                            Where = STDISP_VIEWPORT_ORDER_INFRONT;
                            ReferenceVPDisplayHandle = ((laycompo_ViewPort_t*)ReferenceUnit_p->Device_p->LastOpenViewPort)->VPDisplay;
                            break;
                        }
                    }
                    if (ReferenceUnit_p == NULL)
                    {
                        Where = STDISP_VIEWPORT_ORDER_SEND_TO_BACK;
                    }
                }
            }

            Err = STDISP_SetViewPortOrder(ViewPort_p->VPDisplay,Where,ReferenceVPDisplayHandle);
            if (Err != ST_NO_ERROR)
            {
                STTBX_Report ((STTBX_REPORT_LEVEL_ERROR,"Err STDISP_SetViewPortOrder : %d\n",Err));

                /* Free access */
                STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

                return(Err);
            }
        }
    }

    /* Free access */
    STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

    return(Err);
}


/*******************************************************************************
Name        : LAYCOMPO_CloseViewPort
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYCOMPO_CloseViewPort(STLAYER_ViewPortHandle_t VPHandle)
{
    ST_ErrorCode_t              Err 		= ST_NO_ERROR;
    laycompo_Device_t*          Device_p;
    laycompo_ViewPort_t*        ViewPort_p  = (laycompo_ViewPort_t*) VPHandle ;

    /* Check Handle validity */
    if (VPHandle == (STLAYER_ViewPortHandle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (ViewPort_p->ViewPortValidity != LAYCOMPO_VALID_VIEWPORT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

	/* Get device structure */
	Device_p = (laycompo_Device_t*)ViewPort_p->Device_p;

    /* Protect access */
    STOS_SemaphoreWait(Device_p->CtrlAccess_p);

    if (Device_p->MixerConnected)
    {
        Err = STDISP_CloseViewPort(ViewPort_p->VPDisplay);
        if (Err != ST_NO_ERROR)
        {
            STTBX_Report ((STTBX_REPORT_LEVEL_ERROR,"Err STDISP_CloseViewPort : %d\n",Err));

            /* Free access */
            STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

            return(Err);
        }
    }

    if (ViewPort_p->Enable)
    {
        /* Disable Viewport */
        ViewPort_p->Enable = FALSE;
    }

    /* Update Open Viewport list */
    RemoveViewPortFromList(VPHandle);

    /* Update Viewport descriptor */
    ViewPort_p->ViewPortValidity = (U32)~LAYCOMPO_VALID_VIEWPORT;

    /* Deallocate Viewport Bitmap/VideoStream data & Palette if any */
    if (ViewPort_p->Source.SourceType == STLAYER_STREAMING_VIDEO)
    {
        /* Deallocate for Viewport VideoStream data */
        STOS_MemoryDeallocate(Device_p->CPUPartition_p, ViewPort_p->Source.Data.VideoStream_p);
    }
    else  /* STLAYER_GRAPHIC_BITMAP */
    {
        /* Deallocate for Viewport Bitmap data */
        STOS_MemoryDeallocate(Device_p->CPUPartition_p, ViewPort_p->Source.Data.BitMap_p);

        if (ViewPort_p->Source.Palette_p != NULL)
        {
            /* Deallocate Viewport Palette Data */
            STOS_MemoryDeallocate(Device_p->CPUPartition_p, ViewPort_p->Source.Palette_p);
        }
    }

    /* Release Viewport */
    laycompo_ReleaseElement(&(Device_p->ViewPortDataPool), (void*) VPHandle);

    /* Free access */
    STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

    return(Err);
}

/*******************************************************************************
Name        : LAYCOMPO_EnableViewPort
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYCOMPO_EnableViewPort(STLAYER_ViewPortHandle_t VPHandle)
{
    ST_ErrorCode_t          Err         = ST_NO_ERROR;
    laycompo_Device_t*      Device_p;
    laycompo_ViewPort_t*    ViewPort_p  = (laycompo_ViewPort_t*) VPHandle ;

    /* Check Handle validity */
    if (VPHandle == (STLAYER_ViewPortHandle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (ViewPort_p->ViewPortValidity != LAYCOMPO_VALID_VIEWPORT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (ViewPort_p->Enable)
    {
        return(ST_NO_ERROR);
    }

    /* Get device structure */
    Device_p = (laycompo_Device_t*)ViewPort_p->Device_p;

    /* Protect access */
    STOS_SemaphoreWait(Device_p->CtrlAccess_p);

    if (Device_p->MixerConnected)
    {
        Err = STDISP_EnableViewPort(ViewPort_p->VPDisplay);
        if (Err != ST_NO_ERROR)
        {
            STTBX_Report ((STTBX_REPORT_LEVEL_ERROR,"Err STDISP_EnableViewPort : %d\n",Err));

            /* Free access */
            STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

            return(Err);
        }
    }

    /* Enable ViewPort */
    ViewPort_p->Enable = TRUE;

    /* Free access */
    STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

    return(Err);
}

/*******************************************************************************
Name        : LAYCOMPO_DisableViewPort
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYCOMPO_DisableViewPort(STLAYER_ViewPortHandle_t VPHandle)
{
    ST_ErrorCode_t          Err         = ST_NO_ERROR;
    laycompo_ViewPort_t*    ViewPort_p  = (laycompo_ViewPort_t*) VPHandle ;
    laycompo_Device_t*      Device_p;

    /* Check Handle validity */
    if (VPHandle == (STLAYER_ViewPortHandle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (ViewPort_p->ViewPortValidity != LAYCOMPO_VALID_VIEWPORT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    if (!(ViewPort_p->Enable))
    {
        return(ST_NO_ERROR);
    }

    /* Get device structure */
    Device_p = (laycompo_Device_t*)ViewPort_p->Device_p;

    /* Protect access */
    STOS_SemaphoreWait(Device_p->CtrlAccess_p);

    if (Device_p->MixerConnected)
    {
        Err = STDISP_DisableViewPort(ViewPort_p->VPDisplay);
        if (Err != ST_NO_ERROR)
        {
            STTBX_Report ((STTBX_REPORT_LEVEL_ERROR,"Err STDISP_DisableViewPort : %d\n",Err));

            /* Free access */
            STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

            return(Err);
        }
    }

    /* Disable ViewPort */
    ViewPort_p->Enable       = FALSE;

    /* Free access */
    STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

    return(Err);
}

/*******************************************************************************
Name        : LAYCOMPO_UpdateFromMixer
Description :
Parameters  :
Assumptions : +Supported UpdateReasons are:
                + STLAYER_CONNECT_REASON        => If Viewport, Assume connect at Front of every layers!
                + STLAYER_DISCONNECT_REASON
                + STLAYER_SCREEN_PARAMS_REASON  => Assume Device_p->MixerConnected already be TRUE  (connection done in a previous call)
                + STLAYER_VTG_REASON            => Assume Device_p->MixerConnected already be TRUE  (connection done in a previous call)

              + If Layer already connected and STLAYER_CONNECT_REASON occurs -> It is for the same Mixer connection! Check is done by
                mixer driver to ensure that layer already connected to a mixer, can not be connected to a new one without deconnection!

Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYCOMPO_UpdateFromMixer(STLAYER_Handle_t Handle, STLAYER_OutputParams_t* OutputParams_p)
{
    ST_ErrorCode_t                  Err         = ST_NO_ERROR;
    laycompo_Unit_t*                Unit_p      = (laycompo_Unit_t*)Handle;
    laycompo_Device_t*              Device_p;
    STLAYER_UpdateParams_t          UpdateParams;
    STLAYER_LayerParams_t           LayerParams;
    BOOL                            IsEvtToBeNotified = FALSE;

    /* Check Handle validity */
    if (Handle == (STLAYER_Handle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (Unit_p->UnitValidity != LAYCOMPO_VALID_UNIT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Check Output params */
    if (OutputParams_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
    /*  Check Output params ....(ScanType, AspectRatio ...)*/

    Device_p = Unit_p->Device_p ;

    /* Some Init */
    UpdateParams.UpdateReason = 0;

    /* Protect access */
    STOS_SemaphoreWait(Device_p->CtrlAccess_p);

    if ((OutputParams_p->UpdateReason & 0x2) == STLAYER_SCREEN_PARAMS_REASON)
    {
        /* Check if device layer is fully inside Mixer plane (layer is always at (0,0) in mixer plane) */
        if ((OutputParams_p->Width < Device_p->LayerWidth) ||
            (OutputParams_p->Height < Device_p->LayerHeight))
        {
            /* Free access */
            STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

             return(STLAYER_ERROR_INSIDE_LAYER);
        }

        /* Update Device Layer */
        Device_p->AspectRatio = OutputParams_p->AspectRatio;
        Device_p->ScanType    = OutputParams_p->ScanType;
        Device_p->MixerWidth  = OutputParams_p->Width;
        Device_p->MixerHeight = OutputParams_p->Height;

        /* Record data for STLAYER_UPDATE_PARAMS_EVT */
        UpdateParams.UpdateReason  |= STLAYER_LAYER_PARAMS_REASON;
        LayerParams.AspectRatio     = OutputParams_p->AspectRatio;
        LayerParams.ScanType        = OutputParams_p->ScanType;
        LayerParams.Width           = OutputParams_p->Width;
        LayerParams.Height          = OutputParams_p->Height;
        UpdateParams.LayerParams_p  = &LayerParams;
        IsEvtToBeNotified           = TRUE;
    }

    if ((OutputParams_p->UpdateReason & 0x8) == STLAYER_VTG_REASON)
    {
        /* Update device structure */
        Device_p->IsEvtVsyncSubscribed  = TRUE;
        strcpy(Device_p->VTGParams.VTGName,OutputParams_p->VTGName);
        Device_p->VTGParams.VTGFrameRate = OutputParams_p->FrameRate;

        /* Record data for STLAYER_UPDATE_PARAMS_EVT */
        UpdateParams.UpdateReason          |= STLAYER_VTG_REASON;
        UpdateParams.VTGParams.VTGFrameRate = OutputParams_p->FrameRate;
        IsEvtToBeNotified                   = TRUE;
        strcpy(UpdateParams.VTGParams.VTGName,OutputParams_p->VTGName);
        strcpy(UpdateParams.VTG_Name,OutputParams_p->VTGName);
    }

    if ((OutputParams_p->UpdateReason & 0x200) == STLAYER_CONNECT_REASON)
    {
        if (!(Device_p->MixerConnected))
        {
            /* Set Display Handle */
            Device_p->DisplayHandle  = (STDISP_Handle_t)OutputParams_p->DisplayHandle;

            /* Create ViewPort Displays */
            if (Device_p->NbOpenViewPorts != 0)
            {
                CreateAllVPDisplayFromList(Device_p->LastOpenViewPort);
            }

            /* Connection */
            Device_p->MixerConnected = TRUE;
        }
        else  /*  assumption is that the layer remains connected to same mixer (See STVMIX_ConnectLayers function)!! */
        {

            if (Device_p->NbOpenViewPorts != 0)
            {
                /* Assumption is that the UpdatefromMixer function calls start connection from Back layer to front layer:
                   We need to put all viewports at front positionn starting from last open viewport in the layer
                   (In each layer all viewports have the same z-ordering. However for commodity reason, first open viewport is
                   said to be at the back and last open viewport to be at the front )*/
                SetAllVPDisplayAtFront(Device_p->LastOpenViewPort);
            }
        }
        /* Update Front and Back Layers */
        Device_p->BackLayer   = OutputParams_p->BackLayerHandle;
        Device_p->FrontLayer  = OutputParams_p->FrontLayerHandle;
        /* Update the display params for the video */
        Device_p->DisplayParamsForVideo.FrameBufferDisplayLatency    = OutputParams_p->FrameBufferDisplayLatency;
        Device_p->DisplayParamsForVideo.FrameBufferHoldTime          = OutputParams_p->FrameBufferHoldTime;
        UpdateParams.DisplayParamsForVideo.FrameBufferDisplayLatency = OutputParams_p->FrameBufferDisplayLatency;
        UpdateParams.DisplayParamsForVideo.FrameBufferHoldTime       = OutputParams_p->FrameBufferHoldTime;

        IsEvtToBeNotified                   = TRUE;
        UpdateParams.UpdateReason          |= STLAYER_CONNECT_REASON;
    }

    if ((OutputParams_p->UpdateReason & 0x1) == STLAYER_DISCONNECT_REASON)
    {
        if (Device_p->MixerConnected)
        {
            /* Destroy ViewPort Displays */
            if (Device_p->NbOpenViewPorts != 0)
            {
                DestroyAllVPDisplayFromList(Device_p->FirstOpenViewPort);
            }

            /* Reset Device */
            Device_p->DisplayHandle     = LAYCOMPO_NO_DISPLAY_HANDLE;
            Device_p->MixerConnected    = FALSE;
            Device_p->MixerWidth        = Device_p->LayerWidth;
            Device_p->MixerHeight       = Device_p->LayerHeight;
            Device_p->BackLayer         = LAYCOMPO_NO_LAYER_HANDLE;
            Device_p->FrontLayer        = LAYCOMPO_NO_LAYER_HANDLE;
        }

        /* Record data for STLAYER_UPDATE_PARAMS_EVT */
        Device_p->DisplayParamsForVideo.FrameBufferDisplayLatency = 1;
        Device_p->DisplayParamsForVideo.FrameBufferHoldTime       = 1;
        UpdateParams.UpdateReason = STLAYER_DISCONNECT_REASON;
        IsEvtToBeNotified         = TRUE;
    }

    /* Free access */
    STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

    /* Send STLAYER_UPDATE_PARAMS_EVT */
    if (IsEvtToBeNotified)
    {
        STEVT_Notify(Device_p->EvtHandle,Device_p->EventID[LAYCOMPO_UPDATE_PARAMS_EVT_ID],
                    (const void *) (&UpdateParams));
    }

    return(Err);
}

/*******************************************************************************
Name        : LAYCOMPO_SetViewPortPosition
Description :
Parameters  :
Assumptions : Output PositionY is always even
              Compositor is First Field  TOP
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYCOMPO_SetViewPortPosition(STLAYER_ViewPortHandle_t VPHandle,
                                            S32                       PositionX,
                                            S32                       PositionY)
{
    ST_ErrorCode_t          Err                 = ST_NO_ERROR;
    laycompo_ViewPort_t*    ViewPort_p          = (laycompo_ViewPort_t*) VPHandle ;
    laycompo_Device_t*      Device_p;

    /* Check Handle validity */
    if (VPHandle == (STLAYER_ViewPortHandle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (ViewPort_p->ViewPortValidity != LAYCOMPO_VALID_VIEWPORT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Get device structure */
	Device_p = (laycompo_Device_t*)ViewPort_p->Device_p;

    /* Protect access */
    STOS_SemaphoreWait(Device_p->CtrlAccess_p);

    /* Check ViewPort fit in Device layer */
    if ((PositionX < 0) || (PositionY  < 0))
    {
        /* Free access */
        STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

        return(STLAYER_ERROR_OUT_OF_LAYER);
    }
    if (((U32)(PositionX + ViewPort_p->OutputRectangle.Width - 1) > Device_p->LayerWidth) ||
        ((U32)(PositionY + ViewPort_p->OutputRectangle.Height - 1) > Device_p->LayerHeight))
    {
        /* Free access */
        STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

        return(STLAYER_ERROR_OUT_OF_LAYER);
    }

    /* Adjust positions? */

    /* Update ViewPort display if any */
    if (Device_p->MixerConnected)
    {
        Err = STDISP_SetViewPortPosition(ViewPort_p->VPDisplay,PositionX, PositionY);
        if (Err != ST_NO_ERROR)
        {
            STTBX_Report ((STTBX_REPORT_LEVEL_ERROR,"Err STDISP_SetViewPortPosition : %d\n",Err));

            /* Free access */
            STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

            return(Err);
        }
    }

    /* Update ViewPort output rectangle related information. */
    ViewPort_p->OutputRectangle.PositionX = PositionX;
    ViewPort_p->OutputRectangle.PositionY = PositionY;

    /* Free access */
    STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

    return(Err);
}

/*******************************************************************************
Name        : LAYCOMPO_SetViewPortIORectangle
Description :
Parameters  :
Assumptions : Output PositionY is always even
              Compositor is First Field  TOP
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYCOMPO_SetViewPortIORectangle(STLAYER_ViewPortHandle_t  VPHandle,
                                               STGXOBJ_Rectangle_t*      InputRectangle_p,
                                               STGXOBJ_Rectangle_t*      OutputRectangle_p)
{
    ST_ErrorCode_t              Err         					= ST_NO_ERROR;
    laycompo_ViewPort_t*        ViewPort_p  					= (laycompo_ViewPort_t*) VPHandle ;
    laycompo_Device_t*          Device_p;
    STGXOBJ_Bitmap_t*           Bitmap_p;

    /* Check Handle validity */
    if (VPHandle == (STLAYER_ViewPortHandle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (ViewPort_p->ViewPortValidity != LAYCOMPO_VALID_VIEWPORT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Check Params */
    if ((InputRectangle_p == NULL) ||
        (OutputRectangle_p == NULL))
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    if (ViewPort_p->Source.SourceType == STLAYER_STREAMING_VIDEO)
    {
        Bitmap_p = &(ViewPort_p->Source.Data.VideoStream_p->BitmapParams);
    }
    else   /* STLAYER_GRAPHIC_BITMAP */
    {
        Bitmap_p = ViewPort_p->Source.Data.BitMap_p;
    }

    /* Get device structure */
	Device_p = (laycompo_Device_t*)ViewPort_p->Device_p;

    /* Protect access */
    STOS_SemaphoreWait(Device_p->CtrlAccess_p);

    /* Check Rectangles */
    Err = CheckViewPortRectangle(Device_p,Bitmap_p,InputRectangle_p,OutputRectangle_p);
    if (Err != ST_NO_ERROR)
    {
        /* Free access */
        STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

        return(Err);
    }

    /* Adjust position and size */

    /* Update ViewPort display if any */
    if (Device_p->MixerConnected)
    {
        Err = STDISP_SetViewPortIORectangle(ViewPort_p->VPDisplay,InputRectangle_p, OutputRectangle_p);
        if (Err != ST_NO_ERROR)
        {
            STTBX_Report ((STTBX_REPORT_LEVEL_ERROR,"Err STDISP_SetViewPortIORectangle : %d\n",Err));

            /* Free access */
            STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

            return(Err);
        }
    }

    /* Update ViewPortrectangle related information. */
    ViewPort_p->InputRectangle.PositionX    = InputRectangle_p->PositionX;
    ViewPort_p->InputRectangle.PositionY    = InputRectangle_p->PositionY;
    ViewPort_p->InputRectangle.Width        = InputRectangle_p->Width;
    ViewPort_p->InputRectangle.Height       = InputRectangle_p->Height;
    ViewPort_p->OutputRectangle.PositionX   = OutputRectangle_p->PositionX;
    ViewPort_p->OutputRectangle.PositionY   = OutputRectangle_p->PositionY;
    ViewPort_p->OutputRectangle.Width       = OutputRectangle_p->Width;
    ViewPort_p->OutputRectangle.Height      = OutputRectangle_p->Height;

    /* Free access */
    STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

    return(Err);
}

/*******************************************************************************
Name        : LAYCOMPO_SetViewPortSource
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYCOMPO_SetViewPortSource(STLAYER_ViewPortHandle_t    VPHandle,
                                          STLAYER_ViewPortSource_t*    VPSource_p)
{
    ST_ErrorCode_t          Err         = ST_NO_ERROR;
    laycompo_ViewPort_t*    ViewPort_p  = (laycompo_ViewPort_t*) VPHandle ;
    laycompo_Device_t*      Device_p;

    /* Check Handle validity */
    if (VPHandle == (STLAYER_ViewPortHandle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (ViewPort_p->ViewPortValidity != LAYCOMPO_VALID_VIEWPORT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Check Params */
    if (VPSource_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Check Source */
    Err = CheckViewPortSource(VPSource_p);
    if (Err != ST_NO_ERROR)
    {
        return(Err);
    }

    /* Get device structure */
    Device_p = (laycompo_Device_t*)ViewPort_p->Device_p;

    /* Protect access */
    STOS_SemaphoreWait(Device_p->CtrlAccess_p);

    /* Update Viewport Display if any */
    if (Device_p->MixerConnected)
    {
        STDISP_ViewPortSource_t     VPSource;
        STDISP_StreamingVideo_t     VideoStream;

        /* Set ViewPort Display params */
        VPSource.SourceType = VPSource_p->SourceType;
        if (VPSource.SourceType == STLAYER_STREAMING_VIDEO)
        {
            VideoStream.Bitmap           = VPSource_p->Data.VideoStream_p->BitmapParams;
            VideoStream.ScanType         = VPSource_p->Data.VideoStream_p->ScanType;
            VideoStream.FieldInversion   = VPSource_p->Data.VideoStream_p->PresentedFieldInverted;
            VPSource.Data.VideoStream_p  = &VideoStream;
            VPSource.Palette_p           = NULL;
        }
        else  /* STLAYER_GRAPHIC_BITMAP */
        {
            VPSource.Data.Bitmap_p = VPSource_p->Data.BitMap_p;
            VPSource.Palette_p     = VPSource_p->Palette_p;
        }

        Err = STDISP_SetViewPortSource(ViewPort_p->VPDisplay,&VPSource);
        if (Err != ST_NO_ERROR)
        {
            STTBX_Report ((STTBX_REPORT_LEVEL_ERROR,"Err STDISP_SetViewPortSource : %d\n",Err));

            /* Free access */
            STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

            return(Err);
        }
    }

    /* Update ViewPort descriptor :
       Don't care about VPSource_p->Palette_p if NULL because ViewPort_p->Source.Palette_p already NULL by default */
    if (VPSource_p->SourceType == STLAYER_STREAMING_VIDEO)
    {
        /* Set Bitmap */
        *(ViewPort_p->Source.Data.VideoStream_p) = *(VPSource_p->Data.VideoStream_p);
    }
    else  /* STLAYER_GRAPHIC_BITMAP */
    {
        /* Set Palette */
        if (VPSource_p->Palette_p != NULL)
        {
            if (ViewPort_p->Source.Palette_p == NULL)
            {
                /* Allocate for Viewport palette */
                ViewPort_p->Source.Palette_p = (STGXOBJ_Palette_t*) STOS_MemoryAllocate(Device_p->CPUPartition_p,sizeof(STGXOBJ_Palette_t));
                if (ViewPort_p->Source.Palette_p == NULL)
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "CPU memory allocation failed"));

                    /* Free access */
                    STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

                    return(ST_ERROR_NO_MEMORY);
                }
             }
            *(ViewPort_p->Source.Palette_p) = *(VPSource_p->Palette_p);
        }
        else  /* VPSource_p->Palette_p == NULL */
        {
            if (ViewPort_p->Source.Palette_p != NULL)
            {
                /* Deallocate Viewport Palette Data */
                STOS_MemoryDeallocate(Device_p->CPUPartition_p, ViewPort_p->Source.Palette_p);
            }
            ViewPort_p->Source.Palette_p = NULL;
        }

        /* Set Bitmap */
        *(ViewPort_p->Source.Data.BitMap_p) = *(VPSource_p->Data.BitMap_p);
    }
    ViewPort_p->Source.SourceType   = VPSource_p->SourceType;

    /* Free access */
    STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

    return(Err);
}

/*******************************************************************************
Name        : LAYCOMPO_SetViewPortParams
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYCOMPO_SetViewPortParams(STLAYER_ViewPortHandle_t  VPHandle,
                                          STLAYER_ViewPortParams_t* Params_p)
{
    ST_ErrorCode_t              Err                             = ST_NO_ERROR;
    laycompo_Device_t*          Device_p;
    laycompo_ViewPort_t*        ViewPort_p                      = (laycompo_ViewPort_t*) VPHandle ;


    /* Check Handle validity */
    if (VPHandle == (STLAYER_ViewPortHandle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (ViewPort_p->ViewPortValidity != LAYCOMPO_VALID_VIEWPORT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (Params_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Get device structure */
    Device_p = (laycompo_Device_t*)ViewPort_p->Device_p;

    /* Protect access */
    STOS_SemaphoreWait(Device_p->CtrlAccess_p);

    /* Check ViewPort params */
    Err = CheckViewPortParams(Device_p, Params_p);
    if (Err != ST_NO_ERROR)
    {
        /* Free access */
        STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

        return(Err);
    }

    /* Update Viewport Display if any */
    if (Device_p->MixerConnected)
    {
        STDISP_ViewPortParams_t     VPDisplayParams;
        STDISP_ViewPortSource_t     VPSource;
        STDISP_StreamingVideo_t     VideoStream;

        /* Set ViewPort Display params */
        VPSource.SourceType = Params_p->Source_p->SourceType;
        if (VPSource.SourceType == STLAYER_STREAMING_VIDEO)
        {
            VideoStream.Bitmap           = Params_p->Source_p->Data.VideoStream_p->BitmapParams;
            VideoStream.ScanType         = Params_p->Source_p->Data.VideoStream_p->ScanType;
            VideoStream.FieldInversion   = Params_p->Source_p->Data.VideoStream_p->PresentedFieldInverted;
            VPSource.Data.VideoStream_p  = &VideoStream;
            VPSource.Palette_p           = NULL;
        }
        else  /* STLAYER_GRAPHIC_BITMAP */
        {
            VPSource.Data.Bitmap_p = Params_p->Source_p->Data.BitMap_p;
            VPSource.Palette_p     = Params_p->Source_p->Palette_p;
        }

        VPDisplayParams.Source_p        = &VPSource;
        VPDisplayParams.InputRectangle  = Params_p->InputRectangle;
        VPDisplayParams.OutputRectangle = Params_p->OutputRectangle;

        Err = STDISP_SetViewPortParams(ViewPort_p->VPDisplay, &VPDisplayParams);
        if (Err != ST_NO_ERROR)
        {
            STTBX_Report ((STTBX_REPORT_LEVEL_ERROR,"Err STDISP_SetViewPortParams : %d\n",Err));

            /* Free access */
            STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

            return(Err);
        }
    }

    /* Update ViewPort descriptor :
       Don't care about Params_p->Source_p->Palette_p if NULL because ViewPort_p->Source.Palette_p already NULL by default */
    if (Params_p->Source_p->SourceType == STLAYER_STREAMING_VIDEO)
    {
        /* Set Bitmap */
        *(ViewPort_p->Source.Data.VideoStream_p) = *(Params_p->Source_p->Data.VideoStream_p);
    }
    else  /* STLAYER_GRAPHIC_BITMAP */
    {
        /* Set Palette */
        if (Params_p->Source_p->Palette_p != NULL)
        {
            if (ViewPort_p->Source.Palette_p == NULL)
            {
                /* Allocate for Viewport palette */
                ViewPort_p->Source.Palette_p = (STGXOBJ_Palette_t*) STOS_MemoryAllocate(Device_p->CPUPartition_p,sizeof(STGXOBJ_Palette_t));
                if (ViewPort_p->Source.Palette_p == NULL)
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "CPU memory allocation failed"));

                    /* Free access */
                    STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

                    return(ST_ERROR_NO_MEMORY);
                }
             }
            *(ViewPort_p->Source.Palette_p) = *(Params_p->Source_p->Palette_p);
        }
        else  /* Params_p->Source_p->Palette_p == NULL */
        {
            if (ViewPort_p->Source.Palette_p != NULL)
            {
                /* Deallocate Viewport Palette Data */
                STOS_MemoryDeallocate(Device_p->CPUPartition_p, ViewPort_p->Source.Palette_p);
            }
            ViewPort_p->Source.Palette_p = NULL;
        }

        /* Set Bitmap */
        *(ViewPort_p->Source.Data.BitMap_p) = *(Params_p->Source_p->Data.BitMap_p);
    }
    ViewPort_p->Source.SourceType           = Params_p->Source_p->SourceType;
    ViewPort_p->InputRectangle              = Params_p->InputRectangle;
    ViewPort_p->OutputRectangle             = Params_p->OutputRectangle;

    /* Free access */
    STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

    return(Err);
}

/*******************************************************************************
Name        : LAYCOMPO_SetViewPortAlpha
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYCOMPO_SetViewPortAlpha(STLAYER_ViewPortHandle_t  VPHandle,
                                         STLAYER_GlobalAlpha_t*    Alpha_p)
{
    ST_ErrorCode_t              Err         = ST_NO_ERROR;
    laycompo_ViewPort_t*        ViewPort_p  = (laycompo_ViewPort_t*) VPHandle ;
    laycompo_Device_t*          Device_p;

    /* Check Handle validity */
    if (VPHandle == (STLAYER_ViewPortHandle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (ViewPort_p->ViewPortValidity != LAYCOMPO_VALID_VIEWPORT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (Alpha_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if ((Alpha_p->A0 > 128) || (Alpha_p->A1 > 128))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Get device structure */
    Device_p = (laycompo_Device_t*)ViewPort_p->Device_p;

    /* Protect access */
    STOS_SemaphoreWait(Device_p->CtrlAccess_p);

    /* Update Viewport display if any */
    if (Device_p->MixerConnected)
    {
        STDISP_GlobalAlpha_t VPDisplayAlpha;

        VPDisplayAlpha.A0 = Alpha_p->A0;
        VPDisplayAlpha.A1 = Alpha_p->A1;

        Err = STDISP_SetViewPortAlpha(ViewPort_p->VPDisplay,&VPDisplayAlpha);
        if (Err != ST_NO_ERROR)
        {
            STTBX_Report ((STTBX_REPORT_LEVEL_ERROR,"Err STDISP_SetViewPortAlpha : %d\n",Err));

            /* Free access */
            STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

            return(Err);
        }
    }

    /* Update ViewPort descriptor */
    ViewPort_p->GlobalAlpha  = *Alpha_p;

    /* Free access */
    STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

    return(Err);
}

/*******************************************************************************
Name        : LAYCOMPO_GetViewPortAlpha
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYCOMPO_GetViewPortAlpha(STLAYER_ViewPortHandle_t  VPHandle,
                                         STLAYER_GlobalAlpha_t*    Alpha_p)
{
    ST_ErrorCode_t              Err         = ST_NO_ERROR;
    laycompo_ViewPort_t*        ViewPort_p  = (laycompo_ViewPort_t*) VPHandle ;

    /* Check Handle validity */
    if (VPHandle == (STLAYER_ViewPortHandle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (ViewPort_p->ViewPortValidity != LAYCOMPO_VALID_VIEWPORT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (Alpha_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    *Alpha_p = ViewPort_p->GlobalAlpha;


    return(Err);
}

/*******************************************************************************
Name        : LAYCOMPO_GetViewPortIORectangle
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYCOMPO_GetViewPortIORectangle(STLAYER_ViewPortHandle_t VPHandle,
                                               STGXOBJ_Rectangle_t*      InputRectangle_p,
                                               STGXOBJ_Rectangle_t*      OutputRectangle_p)
{
    ST_ErrorCode_t              Err         = ST_NO_ERROR;
    laycompo_ViewPort_t*        ViewPort_p  = (laycompo_ViewPort_t*) VPHandle ;

    /* Check Handle validity */
    if (VPHandle == (STLAYER_ViewPortHandle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (ViewPort_p->ViewPortValidity != LAYCOMPO_VALID_VIEWPORT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Check Params */
    if ((InputRectangle_p == NULL) ||
        (OutputRectangle_p == NULL))
    {
        return(ST_ERROR_INVALID_HANDLE);
    }


    /* return value */
    *InputRectangle_p  = ViewPort_p->InputRectangle;
    *OutputRectangle_p = ViewPort_p->OutputRectangle;

    return(Err);
}

/*******************************************************************************
Name        : LAYCOMPO_GetViewPortParams
Description :
Parameters  :
Assumptions : Params_p points to a valid structure
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYCOMPO_GetViewPortParams(STLAYER_ViewPortHandle_t  VPHandle,
                                          STLAYER_ViewPortParams_t* Params_p)
{
    ST_ErrorCode_t              Err         = ST_NO_ERROR;
    laycompo_ViewPort_t*        ViewPort_p  = (laycompo_ViewPort_t*) VPHandle ;

    /* Check Handle validity */
    if (VPHandle == (STLAYER_ViewPortHandle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (ViewPort_p->ViewPortValidity != LAYCOMPO_VALID_VIEWPORT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (Params_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
    if (Params_p->Source_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }


    *(Params_p->Source_p)     = ViewPort_p->Source;
    Params_p->InputRectangle  = ViewPort_p->InputRectangle;
    Params_p->OutputRectangle = ViewPort_p->OutputRectangle;

    return(Err);
}

/*******************************************************************************
Name        : LAYCOMPO_GetViewPortSource
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYCOMPO_GetViewPortSource(STLAYER_ViewPortHandle_t  VPHandle,
                                          STLAYER_ViewPortSource_t*  VPSource_p)
{
    ST_ErrorCode_t          Err         = ST_NO_ERROR;
    laycompo_ViewPort_t*    ViewPort_p  = (laycompo_ViewPort_t*) VPHandle ;

    /* Check Handle validity */
    if (VPHandle == (STLAYER_ViewPortHandle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (ViewPort_p->ViewPortValidity != LAYCOMPO_VALID_VIEWPORT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Check Params */
    if (VPSource_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Return value */
    *VPSource_p = ViewPort_p->Source;

    return(Err);
}

/*******************************************************************************
Name        : LAYCOMPO_GetViewPortPosition
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYCOMPO_GetViewPortPosition(STLAYER_ViewPortHandle_t VPHandle,
                                            S32*                     PositionX_p,
                                            S32*                     PositionY_p)
{
    ST_ErrorCode_t          Err                 = ST_NO_ERROR;
    laycompo_ViewPort_t*    ViewPort_p          = (laycompo_ViewPort_t*) VPHandle ;


    /* Check Handle validity */
    if (VPHandle == (STLAYER_ViewPortHandle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (ViewPort_p->ViewPortValidity != LAYCOMPO_VALID_VIEWPORT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    if ((PositionX_p == NULL) || (PositionY_p == NULL))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    *PositionX_p = ViewPort_p->OutputRectangle.PositionX;
    *PositionY_p = ViewPort_p->OutputRectangle.PositionY;


    return(Err);
}

/*******************************************************************************
Name        : LAYCOMPO_AdjustViewPortParams
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYCOMPO_AdjustViewPortParams(STLAYER_Handle_t           Handle,
                                             STLAYER_ViewPortParams_t*  Params_p)
{
    ST_ErrorCode_t      Err = ST_NO_ERROR;
    laycompo_Unit_t*    Unit_p      = (laycompo_Unit_t*)Handle;
    laycompo_Device_t*  Device_p;
    STGXOBJ_Bitmap_t*   Bitmap_p;
    S32                 ClipedInStart;
    S32                 ClipedInStop;
    S32                 ClipedOutStart;
    S32                 ClipedOutStop;
    /*U32                 Resize;*/
    U32                 StartTrunc,StopTrunc;
    S32                 InStop, OutStop;

    /* Check Handle validity */
    if (Handle == (STLAYER_Handle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    if (Unit_p->UnitValidity != LAYCOMPO_VALID_UNIT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Check params & Source  */
    if (Params_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
    if (Params_p->Source_p->SourceType == STLAYER_GRAPHIC_BITMAP)
    {
        if (Params_p->Source_p->Data.BitMap_p == NULL)
        {
            return(ST_ERROR_BAD_PARAMETER);
        }
        else
        {
            Bitmap_p = Params_p->Source_p->Data.BitMap_p;
        }
    }
    else   /*  STLAYER_STREAMING_VIDEO */
    {
        if (Params_p->Source_p->Data.VideoStream_p == NULL)
        {
            return(ST_ERROR_BAD_PARAMETER);
        }
        else
        {
            Bitmap_p = &(Params_p->Source_p->Data.VideoStream_p->BitmapParams);
        }
    }

    Device_p = Unit_p->Device_p ;

    /* Protect access */
    STOS_SemaphoreWait(Device_p->CtrlAccess_p);

    /* Clipping Horizontal Computing */
    /*-------------------------------*/
    InStop  = Params_p->InputRectangle.PositionX/16 + Params_p->InputRectangle.Width;
    OutStop = Params_p->OutputRectangle.PositionX + Params_p->OutputRectangle.Width;
    /*Resize  = 1024 * (Params_p->OutputRectangle.Width - 1) / (Params_p->InputRectangle.Width - 1);*/
    /* (factor 1024 to keep precision in integer div) */

    /* easy clipping in bitmap selection */
    /*-----------------------------------*/
    if(Params_p->InputRectangle.PositionX  < 0 )
    {
        StartTrunc    = -Params_p->InputRectangle.PositionX/16;
        ClipedInStart = 0;
    }
    else
    {
        StartTrunc    = 0;
        ClipedInStart = Params_p->InputRectangle.PositionX/16;
    }
    if(InStop > (S32)(Bitmap_p->Width))
    {
        StopTrunc    = InStop - Bitmap_p->Width;
        ClipedInStop = Bitmap_p->Width;
    }
    else
    {
        StopTrunc    = 0;
        ClipedInStop = InStop;
    }
    /* screen feedback clipping */
    /*--------------------------*/
    /*ClipedOutStart = Params_p->OutputRectangle.PositionX + (StartTrunc * Resize / 1024);*/
    ClipedOutStart = Params_p->OutputRectangle.PositionX + (S32)((StartTrunc * (Params_p->OutputRectangle.Width - 1)) / (Params_p->InputRectangle.Width - 1));
    /*ClipedOutStop  = OutStop - (StopTrunc * Resize / 1024);*/
    ClipedOutStop  = OutStop - (S32)((StopTrunc * (Params_p->OutputRectangle.Width - 1)) / (Params_p->InputRectangle.Width - 1));

    /* easy clipping in output selection */
    /*-----------------------------------*/
    if(ClipedOutStart < 0)
    {
        StartTrunc     = -ClipedOutStart;
        ClipedOutStart = 0;
    }
    else
    {
        StartTrunc = 0;
    }
    if(ClipedOutStop > (S32)Device_p->LayerWidth)
    {
        StopTrunc       = ClipedOutStop - Device_p->LayerWidth;
        ClipedOutStop   = Device_p->LayerWidth;
    }
    else
    {
        StopTrunc = 0;
    }
    /* bitmap feedback clipping */
    /*--------------------------*/
    /*ClipedInStart = ClipedInStart + ( 1024 * StartTrunc / Resize);*/
    ClipedInStart = ClipedInStart + (S32)((StartTrunc * (Params_p->InputRectangle.Width - 1)) / (Params_p->OutputRectangle.Width - 1));
    /*ClipedInStop  = ClipedInStop - (1024 * StopTrunc / Resize);*/
    ClipedInStop  = ClipedInStop - (S32)((StopTrunc * (Params_p->InputRectangle.Width - 1)) / (Params_p->OutputRectangle.Width - 1));

    /* Update Input/Output PositionX/Width */
    Params_p->InputRectangle.PositionX  = ClipedInStart*16;
    Params_p->InputRectangle.Width      = ClipedInStop - ClipedInStart;
    Params_p->OutputRectangle.PositionX = ClipedOutStart;
    Params_p->OutputRectangle.Width     = ClipedOutStop - ClipedOutStart;


    /* Clipping Vertical Computing */
    /*-----------------------------*/

    InStop  = Params_p->InputRectangle.PositionY/16 + Params_p->InputRectangle.Height;
    OutStop = Params_p->OutputRectangle.PositionY + Params_p->OutputRectangle.Height;
    /*Resize =  1024 * (Params_p->OutputRectangle.Height - 1) / (Params_p->InputRectangle.Height - 1);*/

    /* easy clipping in bitmap selection */
    /*-----------------------------------*/
    if(Params_p->InputRectangle.PositionY  < 0 )
    {
        StartTrunc      = -Params_p->InputRectangle.PositionY/16;
        ClipedInStart   = 0;
    }
    else
    {
        StartTrunc      = 0;
        ClipedInStart   = Params_p->InputRectangle.PositionY/16;
    }
    if(InStop > (S32)(Bitmap_p->Height))
    {
        StopTrunc    = InStop - Bitmap_p->Height;
        ClipedInStop = Bitmap_p->Height;
    }
    else
    {
        StopTrunc    = 0;
        ClipedInStop = InStop;
    }
    /* screen feedback clipping */
    /*--------------------------*/
    /*ClipedOutStart = Params_p->OutputRectangle.PositionY + (StartTrunc * Resize / 1024);*/
    ClipedOutStart = Params_p->OutputRectangle.PositionY + (S32)((StartTrunc * (Params_p->OutputRectangle.Height - 1)) / (Params_p->InputRectangle.Height - 1));
    /*ClipedOutStop  = OutStop - (StopTrunc * Resize / 1024);*/
    ClipedOutStop  = OutStop - (S32)((StopTrunc * (Params_p->OutputRectangle.Height - 1)) / (Params_p->InputRectangle.Height - 1));

    /* easy clipping in output selection */
    /*-----------------------------------*/
    if(ClipedOutStart < 0)
    {
        StartTrunc     = -ClipedOutStart;
        ClipedOutStart = 0;
    }
    else
    {
        StartTrunc = 0;
    }
    if(ClipedOutStop >(S32)Device_p->LayerHeight)
    {
        StopTrunc     = ClipedOutStop - Device_p->LayerHeight;
        ClipedOutStop = Device_p->LayerHeight;
    }
    else
    {
        StopTrunc = 0;
    }
    /* bitmap feedback clipping */
    /*--------------------------*/
    /*ClipedInStart = ClipedInStart + (1024 * StartTrunc / Resize );*/
    ClipedInStart = ClipedInStart + (S32)((StartTrunc * (Params_p->InputRectangle.Height - 1)) / (Params_p->OutputRectangle.Height - 1) );
    /*ClipedInStop  = ClipedInStop - (1024 * StopTrunc / Resize );*/
    ClipedInStop  = ClipedInStop - (S32)((StopTrunc * (Params_p->InputRectangle.Height - 1)) / (Params_p->OutputRectangle.Height - 1) );

    /* Update Input/Output PositionY/Height */
    Params_p->InputRectangle.PositionY   = ClipedInStart*16;
    Params_p->InputRectangle.Height      = ClipedInStop - ClipedInStart;
    Params_p->OutputRectangle.PositionY  = ClipedOutStart;
    Params_p->OutputRectangle.Height     = ClipedOutStop - ClipedOutStart;

    /* Free access */
    STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

    return(Err);

}


/*******************************************************************************
Name        : LAYCOMPO_AdjustIORectangle
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYCOMPO_AdjustIORectangle(STLAYER_Handle_t       Handle,
                                          STGXOBJ_Rectangle_t*   InputRectangle_p,
                                          STGXOBJ_Rectangle_t*   OutputRectangle_p)
{
    ST_ErrorCode_t      Err         = ST_NO_ERROR;
    laycompo_Unit_t*    Unit_p      = (laycompo_Unit_t*)Handle;
    laycompo_Device_t*  Device_p;
    S32                 ClipedInStart;
    S32                 ClipedInStop;
    S32                 ClipedOutStart;
    S32                 ClipedOutStop;
    /*U32                 Resize;*/
    U32                 StartTrunc,StopTrunc;
    S32                 InStop, OutStop;

    /* Check Handle validity */
    if (Handle == (STLAYER_Handle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    if (Unit_p->UnitValidity != LAYCOMPO_VALID_UNIT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Check params & Source  */
    if ((InputRectangle_p == NULL) ||
        (OutputRectangle_p == NULL))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    Device_p = Unit_p->Device_p ;

    /* Protect access */
    STOS_SemaphoreWait(Device_p->CtrlAccess_p);


    /* Clipping Horizontal Computing */
    /*-------------------------------*/
    InStop  = InputRectangle_p->PositionX/16 + InputRectangle_p->Width;
    OutStop = OutputRectangle_p->PositionX + OutputRectangle_p->Width;
    /*Resize  = 1024 * (OutputRectangle_p->Width - 1) / (InputRectangle_p->Width - 1);*/
    /* (factor 1024 to keep precision in integer div) */

    /* easy clipping in bitmap selection */
    /*-----------------------------------*/
    if(OutputRectangle_p->PositionX  < 0 )
    {
        StartTrunc     = -OutputRectangle_p->PositionX;
        ClipedOutStart = 0;
    }
    else
    {
        StartTrunc     = 0;
        ClipedOutStart = OutputRectangle_p->PositionX;
    }
    if(OutStop > (S32)(Device_p->LayerWidth))
    {
        StopTrunc     = OutStop - Device_p->LayerWidth;
        ClipedOutStop = Device_p->LayerWidth;
    }
    else
    {
        StopTrunc     = 0;
        ClipedOutStop = OutStop;
    }

    /* bitmap feedback clipping */
    /*--------------------------*/
    /*ClipedInStart = InputRectangle_p->PositionX + (1024 * StartTrunc / Resize);*/
    ClipedInStart = InputRectangle_p->PositionX/16 + (S32)((StartTrunc * (InputRectangle_p->Width - 1)) / (OutputRectangle_p->Width - 1));
    /*ClipedInStop  = InStop - (1024 * StopTrunc / Resize);*/
    ClipedInStop  = InStop - (S32)((StopTrunc * (InputRectangle_p->Width - 1)) / (OutputRectangle_p->Width - 1));

    /* Update Input/Output PositionX/Width */
    InputRectangle_p->PositionX   = ClipedInStart*16;
    InputRectangle_p->Width       = ClipedInStop - ClipedInStart;
    OutputRectangle_p->PositionX  = ClipedOutStart;
    OutputRectangle_p->Width      = ClipedOutStop - ClipedOutStart;


    /* Clipping Vertical Computing */
    /*-----------------------------*/
    InStop  = InputRectangle_p->PositionY/16 + InputRectangle_p->Height;
    OutStop = OutputRectangle_p->PositionY + OutputRectangle_p->Height;
    /*Resize  = 1024 * (OutputRectangle_p->Height - 1) / (InputRectangle_p->Height - 1);*/
    /* (factor 1024 to keep precision in integer div) */

    /* easy clipping in bitmap selection */
    /*-----------------------------------*/
    if(OutputRectangle_p->PositionY  < 0 )
    {
        StartTrunc     = -OutputRectangle_p->PositionY;
        ClipedOutStart = 0;
    }
    else
    {
        StartTrunc     = 0;
        ClipedOutStart = OutputRectangle_p->PositionY;
    }
    if(OutStop > (S32)(Device_p->LayerHeight))
    {
        StopTrunc     = OutStop - Device_p->LayerHeight;
        ClipedOutStop = Device_p->LayerHeight;
    }
    else
    {
        StopTrunc     = 0;
        ClipedOutStop = OutStop;
    }

    /* bitmap feedback clipping */
    /*--------------------------*/
    /*ClipedInStart = InputRectangle_p->PositionY + (1024 * StartTrunc / Resize);*/
    ClipedInStart = InputRectangle_p->PositionY/16 + (S32) ((StartTrunc * (InputRectangle_p->Height - 1)) / (OutputRectangle_p->Height - 1));
    /*ClipedInStop  = InStop - (1024 * StopTrunc / Resize);*/
    ClipedInStop  = InStop - (S32)((StopTrunc * (InputRectangle_p->Height - 1)) / (OutputRectangle_p->Height - 1));

    /* Update Input/Output PositionY/Height */
    InputRectangle_p->PositionY   = ClipedInStart*16;
    InputRectangle_p->Height      = ClipedInStop - ClipedInStart;
    OutputRectangle_p->PositionY  = ClipedOutStart;
    OutputRectangle_p->Height     = ClipedOutStop - ClipedOutStart;

    /* Free access */
    STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

    return(Err);
}

/*******************************************************************************
Name        :  LAYCOMPO_DisableBorderAlpha
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYCOMPO_DisableBorderAlpha(STLAYER_ViewPortHandle_t VPHandle)
{
    UNUSED_PARAMETER(VPHandle);
    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
}

/*******************************************************************************
Name        :  LAYCOMPO_EnableBorderAlpha
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYCOMPO_EnableBorderAlpha(STLAYER_ViewPortHandle_t VPHandle)
{
    UNUSED_PARAMETER(VPHandle);
    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
}

/*******************************************************************************
Name        :  LAYCOMPO_SetViewPortRecordable
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYCOMPO_SetViewPortRecordable(STLAYER_ViewPortHandle_t VPHandle,BOOL Recordable)
{
    UNUSED_PARAMETER(VPHandle);
    UNUSED_PARAMETER(Recordable);
    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
}

/*******************************************************************************
Name        :  LAYCOMPO_GetViewPortRecordable
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYCOMPO_GetViewPortRecordable(STLAYER_ViewPortHandle_t VPHandle,BOOL* Recordable_p)
{
    UNUSED_PARAMETER(VPHandle);
    UNUSED_PARAMETER(Recordable_p);
    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
}

/*******************************************************************************
Name        :  LAYCOMPO_EnableViewPortFilter
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYCOMPO_EnableViewPortFilter(STLAYER_ViewPortHandle_t VPHandle)
{
    ST_ErrorCode_t              Err         = ST_NO_ERROR;
    laycompo_ViewPort_t*        ViewPort_p  = (laycompo_ViewPort_t*) VPHandle ;
    laycompo_Device_t*          Device_p;

    /* Check Handle validity */
    if (VPHandle == (STLAYER_ViewPortHandle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (ViewPort_p->ViewPortValidity != LAYCOMPO_VALID_VIEWPORT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    if (ViewPort_p->FlickerFilterOn)
    {
        return(ST_NO_ERROR);
    }

    /* Get device structure */
    Device_p = (laycompo_Device_t*)ViewPort_p->Device_p;

    /* Protect access */
    STOS_SemaphoreWait(Device_p->CtrlAccess_p);

    /* Update Viewport display if any */
    if (Device_p->MixerConnected)
    {
        Err = STDISP_EnableViewPortFlickerFilter(ViewPort_p->VPDisplay);
        if (Err != ST_NO_ERROR)
        {
            STTBX_Report ((STTBX_REPORT_LEVEL_ERROR,"Err STDISP_EnableViewPortFilter : %d\n",Err));

            /* Free access */
            STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

            return(Err);
        }
    }

    /* Update ViewPort descriptor */
    ViewPort_p->FlickerFilterOn  = TRUE;

    /* Free access */
    STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

    return(Err);
}

/*******************************************************************************
Name        :  LAYCOMPO_DisableViewPortFilter
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYCOMPO_DisableViewPortFilter(STLAYER_ViewPortHandle_t VPHandle)
{
    ST_ErrorCode_t              Err         = ST_NO_ERROR;
    laycompo_ViewPort_t*        ViewPort_p  = (laycompo_ViewPort_t*) VPHandle ;
    laycompo_Device_t*          Device_p;

    /* Check Handle validity */
    if (VPHandle == (STLAYER_ViewPortHandle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (ViewPort_p->ViewPortValidity != LAYCOMPO_VALID_VIEWPORT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    if (!(ViewPort_p->FlickerFilterOn))
    {
        return(ST_NO_ERROR);
    }

    /* Get device structure */
    Device_p = (laycompo_Device_t*)ViewPort_p->Device_p;

    /* Protect access */
    STOS_SemaphoreWait(Device_p->CtrlAccess_p);

    /* Update Viewport display if any */
    if (Device_p->MixerConnected)
    {
        Err = STDISP_DisableViewPortFlickerFilter(ViewPort_p->VPDisplay);
        if (Err != ST_NO_ERROR)
        {
            STTBX_Report ((STTBX_REPORT_LEVEL_ERROR,"Err STDISP_DisableViewPortFilter : %d\n",Err));

            /* Free access */
            STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

            return(Err);
        }
    }

    /* Update ViewPort descriptor */
    ViewPort_p->FlickerFilterOn  = FALSE;

    /* Free access */
    STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

    return(Err);
}

/*******************************************************************************
Name        : LAYCOMPO_SetViewPortCompositionRecurrence
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYCOMPO_SetViewPortCompositionRecurrence(STLAYER_ViewPortHandle_t        VPHandle,
                                                   const STLAYER_CompositionRecurrence_t CompositionRecurrence)
{
    ST_ErrorCode_t              Err         = ST_NO_ERROR;
    laycompo_ViewPort_t*        ViewPort_p  = (laycompo_ViewPort_t*) VPHandle ;
    laycompo_Device_t*          Device_p;

    /* Check Handle validity */
    if (VPHandle == (STLAYER_ViewPortHandle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (ViewPort_p->ViewPortValidity != LAYCOMPO_VALID_VIEWPORT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    if ((CompositionRecurrence != STLAYER_COMPOSITION_RECURRENCE_EVERY_VSYNC) &&
        (CompositionRecurrence != STLAYER_COMPOSITION_RECURRENCE_MANUAL_OR_VIEWPORT_PARAMS_CHANGES))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Get device structure */
    Device_p = (laycompo_Device_t*)ViewPort_p->Device_p;

    /* Protect access */
    STOS_SemaphoreWait(Device_p->CtrlAccess_p);

    /* Update Viewport display if any */
    if (Device_p->MixerConnected)
    {
        STDISP_CompositionRecurrence_t DispCompositionRecurrence;

        switch (CompositionRecurrence)
        {
            case STLAYER_COMPOSITION_RECURRENCE_EVERY_VSYNC :
                DispCompositionRecurrence = STDISP_COMPOSITION_RECURRENCE_EVERY_VSYNC;
                break;

            case STLAYER_COMPOSITION_RECURRENCE_MANUAL_OR_VIEWPORT_PARAMS_CHANGES :
                DispCompositionRecurrence = STDISP_COMPOSITION_RECURRENCE_MANUAL_OR_VIEWPORT_PARAMS_CHANGES;
                break;

           default:
                DispCompositionRecurrence = STDISP_COMPOSITION_RECURRENCE_EVERY_VSYNC;
                break;
        }

        Err = STDISP_SetViewPortCompositionRecurrence(ViewPort_p->VPDisplay, DispCompositionRecurrence);
        if (Err != ST_NO_ERROR)
        {
            STTBX_Report ((STTBX_REPORT_LEVEL_ERROR,"Err STDISP_SetViewPortCompositionRecurrence : %d\n",Err));

            /* Free access */
            STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

            return(Err);
        }
    }

    /* Update ViewPort descriptor */
    ViewPort_p->CompositionRecurrence  = CompositionRecurrence;


    /* Free access */
    STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

    return(Err);
}

/*******************************************************************************
Name        :  LAYCOMPO_PerformViewPortComposition
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYCOMPO_PerformViewPortComposition(STLAYER_ViewPortHandle_t VPHandle)
{
    ST_ErrorCode_t              Err         = ST_NO_ERROR;
    laycompo_ViewPort_t*        ViewPort_p  = (laycompo_ViewPort_t*) VPHandle ;
    laycompo_Device_t*          Device_p;

    /* Check Handle validity */
    if (VPHandle == (STLAYER_ViewPortHandle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (ViewPort_p->ViewPortValidity != LAYCOMPO_VALID_VIEWPORT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Get device structure */
    Device_p = (laycompo_Device_t*)ViewPort_p->Device_p;

    /* Protect access */
    STOS_SemaphoreWait(Device_p->CtrlAccess_p);

    /* Update Viewport display if any */
    if (Device_p->MixerConnected)
    {
        Err = STDISP_PerformViewPortComposition(ViewPort_p->VPDisplay);
        if (Err != ST_NO_ERROR)
        {
            STTBX_Report ((STTBX_REPORT_LEVEL_ERROR,"Err STDISP_DisableViewPortFilter : %d\n",Err));

            /* Free access */
            STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

            return(Err);
        }
    }

    /* Free access */
    STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

    return(Err);
}

/*******************************************************************************
Name        : LAYCOMPO_SetViewPortFlickerFilterMode
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYCOMPO_SetViewPortFlickerFilterMode(STLAYER_ViewPortHandle_t     VPHandle,
                                                     STLAYER_FlickerFilterMode_t  FlickerFilterMode)
{
    ST_ErrorCode_t              Err         = ST_NO_ERROR;
    laycompo_ViewPort_t*        ViewPort_p  = (laycompo_ViewPort_t*) VPHandle ;
    laycompo_Device_t*          Device_p;

    /* Check Handle validity */
    if (VPHandle == (STLAYER_ViewPortHandle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (ViewPort_p->ViewPortValidity != LAYCOMPO_VALID_VIEWPORT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    if ((FlickerFilterMode != STLAYER_FLICKER_FILTER_MODE_SIMPLE) &&
        (FlickerFilterMode != STLAYER_FLICKER_FILTER_MODE_USING_BLITTER))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Get device structure */
    Device_p = (laycompo_Device_t*)ViewPort_p->Device_p;

    /* Protect access */
    STOS_SemaphoreWait(Device_p->CtrlAccess_p);

    /* Update Viewport display if any */
    if (Device_p->MixerConnected)
    {
        STDISP_FlickerFilterMode_t DispFlickerFilterMode;

        switch (FlickerFilterMode)
        {
            case STLAYER_FLICKER_FILTER_MODE_SIMPLE :
                DispFlickerFilterMode = STDISP_FLICKER_FILTER_MODE_SIMPLE;
                break;

            case STLAYER_FLICKER_FILTER_MODE_USING_BLITTER :
                DispFlickerFilterMode = STDISP_FLICKER_FILTER_MODE_USING_BLITTER;
                break;

            default:
                DispFlickerFilterMode = STDISP_FLICKER_FILTER_MODE_SIMPLE;
                break;
        }

        Err = STDISP_SetViewPortFlickerFilterMode(ViewPort_p->VPDisplay, DispFlickerFilterMode);
        if (Err != ST_NO_ERROR)
        {
            STTBX_Report ((STTBX_REPORT_LEVEL_ERROR,"Err STDISP_SetViewPortFlickerFilterMode : %d\n",Err));

            /* Free access */
            STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

            return(Err);
        }
    }

    /* Update ViewPort descriptor */
    ViewPort_p->FlickerFilterMode  = FlickerFilterMode;


    /* Free access */
    STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

    return(Err);
}

/*******************************************************************************
Name        : LAYCOMPO_GetViewPortFlickerFilterMode
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYCOMPO_GetViewPortFlickerFilterMode(STLAYER_ViewPortHandle_t      VPHandle,
                                                     STLAYER_FlickerFilterMode_t*  FlickerFilterMode_p)
{
    ST_ErrorCode_t              Err         = ST_NO_ERROR;
    laycompo_ViewPort_t*        ViewPort_p  = (laycompo_ViewPort_t*) VPHandle ;

    /* Check Handle validity */
    if (VPHandle == (STLAYER_ViewPortHandle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (ViewPort_p->ViewPortValidity != LAYCOMPO_VALID_VIEWPORT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (FlickerFilterMode_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    *FlickerFilterMode_p = ViewPort_p->FlickerFilterMode;


    return(Err);
}

/*******************************************************************************
Name        : LAYCOMPO_InformPictureToBeDecoded
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYCOMPO_InformPictureToBeDecoded(STLAYER_ViewPortHandle_t   VPHandle,
                                                 STGXOBJ_PictureInfos_t*    PictureInfos_p)
{
    ST_ErrorCode_t          Err         = ST_NO_ERROR;
    laycompo_ViewPort_t*    ViewPort_p  = (laycompo_ViewPort_t*) VPHandle ;

    /* Check Handle validity */
    if (VPHandle == (STLAYER_ViewPortHandle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (ViewPort_p->ViewPortValidity != LAYCOMPO_VALID_VIEWPORT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Check Params */
    if (PictureInfos_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    Err = STDISP_InformPictureToBeDecoded(ViewPort_p->VPDisplay, PictureInfos_p);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_ERROR,"Err STDISP_SetViewPortSource : %d\n",Err));

        return(Err);
    }

    return(Err);
}


/*******************************************************************************
Name        : LAYCOMPO_CommitViewPortParams
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYCOMPO_CommitViewPortParams(STLAYER_ViewPortHandle_t VPHandle)
{
    ST_ErrorCode_t          Err         = ST_NO_ERROR;
    laycompo_ViewPort_t*    ViewPort_p  = (laycompo_ViewPort_t*) VPHandle ;

    /* Check Handle validity */
    if (VPHandle == (STLAYER_ViewPortHandle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (ViewPort_p->ViewPortValidity != LAYCOMPO_VALID_VIEWPORT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    Err = STDISP_CommitViewPortParams(ViewPort_p->VPDisplay);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_ERROR,"Err STDISP_CommitViewPortParams : %d\n",Err));

        return(Err);
    }

    return(Err);
}


/* End of lc_vp.c */
