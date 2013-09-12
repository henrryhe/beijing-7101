/*****************************************************************************

File name   : Sub_vp.c

Description : Sub picture Viewport source file

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
15 Dec 2000        Created                                           TM
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */


/* Includes ----------------------------------------------------------------- */

#include <string.h>
#include "stddefs.h"
#include "stlayer.h"
#include "sub_vp.h"
#include "stgxobj.h"
#include "sub_init.h"
#include "sub_pool.h"
#include "sttbx.h"
#include "sub_spu.h"
#include "sub_reg.h"
#include "sub_com.h"
#include "lay_sub.h"
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
static ST_ErrorCode_t CheckViewPortParams(layersub_Device_t* Device_p, STLAYER_ViewPortParams_t* Params_p);
static ST_ErrorCode_t CheckViewPortRectangle(layersub_Device_t* Device_p,STGXOBJ_Bitmap_t* InputBitmap_p,
                                             STGXOBJ_Rectangle_t*  InputRectangle_p, STGXOBJ_Rectangle_t*  OutputRectangle_p);
static ST_ErrorCode_t CheckViewPortSource(STLAYER_ViewPortSource_t* Source_p);


/* Functions ---------------------------------------------------------------- */





/*******************************************************************************
Name        : CheckViewPortRectangle
Description :
Parameters  :
Assumptions : Non NULL Params
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t CheckViewPortRectangle(layersub_Device_t*    Device_p,
                                             STGXOBJ_Bitmap_t*     InputBitmap_p,
                                             STGXOBJ_Rectangle_t*  InputRectangle_p,
                                             STGXOBJ_Rectangle_t*  OutputRectangle_p)
{
    ST_ErrorCode_t Err = ST_NO_ERROR;

    /* No resize suppported */
    if ((InputRectangle_p->Width != OutputRectangle_p->Width ) ||
        (InputRectangle_p->Height != OutputRectangle_p->Height))
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    /* Output Position Y must be even */
    if ((OutputRectangle_p->PositionY % 2) != 0 )
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    /* Output and Input rectangle height is even */
    if ((OutputRectangle_p->Height % 2) != 0)
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    /* Check Output Rectangle fit in Device Layer */
    if ((OutputRectangle_p->PositionX < 0) || (OutputRectangle_p->PositionY < 0))
    {
        return(STLAYER_ERROR_OUT_OF_LAYER);
    }
    if (((U32)(OutputRectangle_p->PositionX + OutputRectangle_p->Width - 1) > Device_p->LayerWidth) ||
        ((U32)(OutputRectangle_p->PositionY + OutputRectangle_p->Height - 1) > Device_p->LayerHeight))
    {
        return(STLAYER_ERROR_OUT_OF_LAYER);
    }

    /* Check Input Rectangle fit in Bitmap */
    if ((InputRectangle_p->PositionX < 0) || (InputRectangle_p->PositionY < 0))
    {
        return(STLAYER_ERROR_OUT_OF_BITMAP);
    }

    if (((U32)(InputRectangle_p->PositionX + InputRectangle_p->Width - 1) > InputBitmap_p->Width) ||
        ((U32)(InputRectangle_p->PositionY + InputRectangle_p->Height - 1) > InputBitmap_p->Height))
    {
        return(STLAYER_ERROR_OUT_OF_BITMAP);
    }

    /* Check max rectangle size (128*128) */
    if ((OutputRectangle_p->Width > 128) || (OutputRectangle_p->Height > 128))
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }


    return(Err);
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
    ST_ErrorCode_t Err = ST_NO_ERROR;
    STGXOBJ_Bitmap_t*       Bitmap_p;
    STGXOBJ_Palette_t*      Palette_p;

    if (Source_p->SourceType != STLAYER_GRAPHIC_BITMAP)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
    if (Source_p->Data.BitMap_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
    if (Source_p->Palette_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Check Bitmap params */
    Bitmap_p = Source_p->Data.BitMap_p;
    if ((Bitmap_p->ColorType   != STGXOBJ_COLOR_TYPE_CLUT2) ||
        ((Bitmap_p->BitmapType != STGXOBJ_BITMAP_TYPE_RASTER_PROGRESSIVE)))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Check Palette */
    Palette_p = Source_p->Palette_p;
    if ((Palette_p->ColorType != STGXOBJ_COLOR_TYPE_ARGB8888) ||
/*       (Palette_p->PaletteType != STGXOBJ_PALETTE_TYPE_DEVICE_DEPENDENT) ||*/
        (Palette_p->ColorDepth != 2))
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    return(Err);
}



/*******************************************************************************
Name        : CheckViewPortParams
Description :
Parameters  :
Assumptions : Non NULL Params_p
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t CheckViewPortParams(layersub_Device_t* Device_p, STLAYER_ViewPortParams_t* Params_p)
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
Name        : layersub_CloseAllViewPortFromList
Description : Close all viewports from list of open viewports contained in layersub_Unit_t
              Recursive function.
Parameters  :
Assumptions : VPHandle != LAYERSUB_NO_VIEWPORT_HANDLE , and non null Unit_p is valid
              The update of Unit_p open ViewPort list must be done at upper level
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t layersub_CloseAllViewPortFromList(layersub_Unit_t* Unit_p,STLAYER_ViewPortHandle_t VPHandle)
{
    ST_ErrorCode_t              Err           = ST_NO_ERROR;
    layersub_ViewPort_t*        ViewPort_p    = (layersub_ViewPort_t*) VPHandle;
    layersub_Device_t*          Device_p      = Unit_p->Device_p;
    STAVMEM_FreeBlockParams_t   FreeParams;

    if (ViewPort_p->NextViewPort != LAYERSUB_NO_VIEWPORT_HANDLE)
    {
        layersub_CloseAllViewPortFromList(Unit_p,ViewPort_p->NextViewPort);
    }

    if (ViewPort_p->Enable == TRUE)
    {
        /* Disable Viewport */
        ViewPort_p->Enable       = FALSE;
        Device_p->ActiveViewPort =  LAYERSUB_NO_VIEWPORT_HANDLE;

        if (Device_p->DisplayStatus == LAYERSUB_DISPLAY_STATUS_ENABLE) /* Mixer Implicitely connected */
        {
            layersub_StopSPU(Device_p);
        }
    }

    /* Free SPU buffer */
    FreeParams.PartitionHandle = Device_p->AVMEMPartition;

    Err = STAVMEM_FreeBlock(&FreeParams,&(ViewPort_p->SubPictureUnitMemoryHandle));
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error Free Shared memory : %d\n",Err));
         /* return (); */
    }

    /* Update Viewport descriptor */
    ViewPort_p->ViewPortValidity = (U32)~LAYERSUB_VALID_VIEWPORT;

    /* Release Viewport */
    layersub_ReleaseElement(&(Device_p->ViewPortDataPool), (void*) VPHandle);

    return(Err);
}

/*******************************************************************************
Name        : AddViewPortToList
Description : Add viewport in list of open viewports contained in layersub_Unit_t
Parameters  :
Assumptions : VPHandle valid and non NULL
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t AddViewPortToList(STLAYER_ViewPortHandle_t VPHandle)
{
    ST_ErrorCode_t          Err         = ST_NO_ERROR;
    layersub_Unit_t*        Unit_p;
    layersub_ViewPort_t*    ViewPort_p  = (layersub_ViewPort_t*) VPHandle;

    Unit_p = (layersub_Unit_t*)(ViewPort_p->Handle);

    /* Update viewport descriptor */
    ViewPort_p->NextViewPort        = LAYERSUB_NO_VIEWPORT_HANDLE;
    ViewPort_p->PreviousViewPort    = Unit_p->LastOpenViewPort;

    /* Update layersub_unit_t */
    if (Unit_p->NbOpenViewPorts == 0)
    {
        Unit_p->FirstOpenViewPort = VPHandle;
    }
    else /* LastOpenViewPort != LAYERSUB_NO_VIEWPORT_HANDLE */
    {
        ((layersub_ViewPort_t*)(Unit_p->LastOpenViewPort))->NextViewPort = VPHandle;
    }
    (Unit_p->NbOpenViewPorts)++;
    Unit_p->LastOpenViewPort = VPHandle;

    return(Err);
}

/*******************************************************************************
Name        : RemoveViewPortFromList
Description : Remove viewport in list of open viewports contained in layersub_Unit_t
Parameters  :
Assumptions : Viewport is in list and only once!
              VPHandle non null and valid
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t RemoveViewPortFromList(STLAYER_ViewPortHandle_t VPHandle)
{
    ST_ErrorCode_t              Err                 = ST_NO_ERROR;
    layersub_Unit_t*            Unit_p;
    layersub_ViewPort_t*        ViewPort_p          = (layersub_ViewPort_t*) VPHandle;
    STLAYER_ViewPortHandle_t   PreviousViewPort;
    STLAYER_ViewPortHandle_t   NextViewPort;

    Unit_p = (layersub_Unit_t*)(ViewPort_p->Handle);

    /* Update previous and next viewport descriptor */
    PreviousViewPort = ViewPort_p->PreviousViewPort ;
    NextViewPort     = ViewPort_p->NextViewPort ;
    if (PreviousViewPort != LAYERSUB_NO_VIEWPORT_HANDLE)
    {
        ((layersub_ViewPort_t*)PreviousViewPort)->NextViewPort = NextViewPort;
    }
    else
    {
        /* Update layersub_unit_t */
        Unit_p->FirstOpenViewPort  = NextViewPort;
    }

    if (NextViewPort != LAYERSUB_NO_VIEWPORT_HANDLE)
    {
        ((layersub_ViewPort_t*)NextViewPort)->PreviousViewPort = PreviousViewPort;
    }
    else
    {
        /* Update layersub_unit_t */
        Unit_p->LastOpenViewPort  = PreviousViewPort;
    }

    (Unit_p->NbOpenViewPorts)--;

    return(Err);
}


/*******************************************************************************
Name        : LAYERSUB_OpenViewPort
Description :
Parameters  :
Assumptions : Output PositionY is always even, No parity cheching : First implementation.
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYERSUB_OpenViewPort(STLAYER_Handle_t            Handle,
                                     STLAYER_ViewPortParams_t*   Params_p,
                                     STLAYER_ViewPortHandle_t*   VPHandle_p)
{
    ST_ErrorCode_t              Err         = ST_NO_ERROR;
    layersub_Unit_t*            Unit_p      = (layersub_Unit_t*)Handle;
    layersub_Device_t*          Device_p;
    layersub_ViewPort_t*        ViewPort_p;
    STAVMEM_AllocBlockParams_t  AllocBlockParams;
    U8                          NbForbiddenRange = 0;
    STAVMEM_MemoryRange_t       RangeArea[2];

    /* Check Handle validity */
    if (Handle == (STLAYER_Handle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    Device_p = Unit_p->Device_p ;

    if (Unit_p->UnitValidity != LAYERSUB_VALID_UNIT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Check general params */
    if ((Params_p == NULL) || (VPHandle_p == NULL))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Check ViewPort params */
    Err = CheckViewPortParams(Device_p, Params_p);
    if (Err != ST_NO_ERROR)
    {
        return(Err);
    }

    /* Look if Parameters are adjusted */

    /* Get ViewPort descriptor handle*/
    Err = layersub_GetElement(&(Device_p->ViewPortDataPool), (void**) VPHandle_p);
    if (Err != ST_NO_ERROR)
    {
        return(STLAYER_ERROR_NO_FREE_HANDLES);
    }

    ViewPort_p = (layersub_ViewPort_t*) *VPHandle_p;



    /* Set ViewPort descriptor */

    ViewPort_p->InputRectangle.PositionX    = Params_p->InputRectangle.PositionX;
    ViewPort_p->InputRectangle.PositionY    = Params_p->InputRectangle.PositionY;
    ViewPort_p->InputRectangle.Width        = Params_p->InputRectangle.Width;
    ViewPort_p->InputRectangle.Height       = Params_p->InputRectangle.Height;
    ViewPort_p->OutputRectangle.PositionX   = Params_p->OutputRectangle.PositionX;
    ViewPort_p->OutputRectangle.PositionY   = Params_p->OutputRectangle.PositionY;
    ViewPort_p->OutputRectangle.Width       = Params_p->OutputRectangle.Width;
    ViewPort_p->OutputRectangle.Height      = Params_p->OutputRectangle.Height;
    ViewPort_p->Bitmap                      = *(Params_p->Source_p->Data.BitMap_p);
    ViewPort_p->Palette                     = *(Params_p->Source_p->Palette_p);
    ViewPort_p->GlobalAlpha.A0              = 128; /* Opaque */
    ViewPort_p->Alpha[0]                    = 128; /* Opaque */
    ViewPort_p->Alpha[1]                    = 128; /* Opaque */
    ViewPort_p->Alpha[2]                    = 128; /* Opaque */
    ViewPort_p->Alpha[3]                    = 128; /* Opaque */
    ViewPort_p->Handle                      = Handle;                    /* Open handle of layer */
    ViewPort_p->ViewPortValidity            = LAYERSUB_VALID_VIEWPORT;
    ViewPort_p->Enable                      = FALSE;

    /* Update Open Viewport list */
    AddViewPortToList(*VPHandle_p);

    /* Allocation of a STU in shared memory
     * First implementation Size => Take max cursor size (128*128)*/
    /* Forbidden range : All Virtual memory range but Virtual window
     *  VirtualWindowOffset may be 0
     *  Virtual Size is always > 0
     *  Assumption : Physical base Address from device = 0 !!!! */
    if (Device_p->VirtualMapping.VirtualWindowOffset > 0)
    {
        RangeArea[0].StartAddr_p = (void *) Device_p->VirtualMapping.VirtualBaseAddress_p;
        RangeArea[0].StopAddr_p  = (void *) ((U32)(RangeArea[0].StartAddr_p) +
                                             (U32)(Device_p->VirtualMapping.VirtualWindowOffset) - 1);
        NbForbiddenRange++;
    }

    if ((Device_p->VirtualMapping.VirtualWindowOffset + Device_p->VirtualMapping.VirtualWindowSize) !=
         Device_p->VirtualMapping.VirtualSize)
    {
        RangeArea[1].StartAddr_p = (void *) ((U32)(RangeArea[0].StartAddr_p) +
                                             (U32)(Device_p->VirtualMapping.VirtualWindowOffset) +
                                             (U32)(Device_p->VirtualMapping.VirtualWindowSize));
        RangeArea[1].StopAddr_p  = (void *) ((U32)(Device_p->VirtualMapping.VirtualBaseAddress_p) +
                                                  (U32)(Device_p->VirtualMapping.VirtualSize) - 1);
        NbForbiddenRange++;
    }

    AllocBlockParams.PartitionHandle          = Device_p->AVMEMPartition;
    AllocBlockParams.Size                     = LAYERSUB_SPU_MAX_SIZE;
    AllocBlockParams.Alignment                = 2048;
    AllocBlockParams.AllocMode                = STAVMEM_ALLOC_MODE_BOTTOM_TOP;
    AllocBlockParams.NumberOfForbiddenRanges  = NbForbiddenRange;
    AllocBlockParams.ForbiddenRangeArray_p    = &RangeArea[0];
    AllocBlockParams.NumberOfForbiddenBorders = 0;
    AllocBlockParams.ForbiddenBorderArray_p   = NULL;

    Err = STAVMEM_AllocBlock(&AllocBlockParams,&(ViewPort_p->SubPictureUnitMemoryHandle));
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Shared memory allocation failed"));

        /*  release Viewport element */
        layersub_ReleaseElement(&(Device_p->ViewPortDataPool), (void*) *VPHandle_p);

        return(STLAYER_ERROR_NO_AV_MEMORY);
    }
    STAVMEM_GetBlockAddress(ViewPort_p->SubPictureUnitMemoryHandle,(void**)&(ViewPort_p->Header_p));

    ViewPort_p->RLETop_p    = (void*)((U32)ViewPort_p->Header_p + LAYERSUB_SPU_HEADER_SIZE);
    ViewPort_p->RLEBottom_p = (void*)((U32)ViewPort_p->RLETop_p + (LAYERSUB_SPU_MAX_DATA_SIZE / 2));
    ViewPort_p->DCSQ_p      = (void*)((U32)ViewPort_p->Header_p + LAYERSUB_SPU_HEADER_SIZE + LAYERSUB_SPU_MAX_DATA_SIZE);

    /* Write SPU Header */
    layersub_SetSPUHeader(Device_p, ViewPort_p);

    /* Write Top and Bottom data */
    layersub_SetData(Device_p, ViewPort_p);

    /* Write Display Control sequence commands */
    layersub_SetDCSQ(Device_p, ViewPort_p);

    return(Err);
}


/*******************************************************************************
Name        : LAYERSUB_CloseViewPort
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYERSUB_CloseViewPort(STLAYER_ViewPortHandle_t VPHandle)
{
    ST_ErrorCode_t              Err = ST_NO_ERROR;
    STAVMEM_FreeBlockParams_t   FreeParams;
    layersub_Unit_t*            Unit_p;
    layersub_Device_t*          Device_p;
    layersub_ViewPort_t*        ViewPort_p = (layersub_ViewPort_t*) VPHandle ;

    /* Check Handle validity */
    if (VPHandle == (STLAYER_ViewPortHandle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (ViewPort_p->ViewPortValidity != LAYERSUB_VALID_VIEWPORT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    Unit_p   = (layersub_Unit_t*)ViewPort_p->Handle;
    Device_p = Unit_p->Device_p;

    if (ViewPort_p->Enable == TRUE)
    {
        /* Disable Viewport */
        ViewPort_p->Enable       = FALSE;
        Device_p->ActiveViewPort = LAYERSUB_NO_VIEWPORT_HANDLE;

        if (Device_p->DisplayStatus == LAYERSUB_DISPLAY_STATUS_ENABLE) /* Mixer Implicitely connected */
        {
            layersub_StopSPU(Device_p);
        }
    }

    /* Free SPU buffer */
    FreeParams.PartitionHandle = Device_p->AVMEMPartition;

    Err = STAVMEM_FreeBlock(&FreeParams,&(ViewPort_p->SubPictureUnitMemoryHandle));
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error Free Shared memory : %d\n",Err));
         /* return (); */
    }

    /* Update Open Viewport list */
    RemoveViewPortFromList(VPHandle);

    /* Update Viewport descriptor */
    ViewPort_p->ViewPortValidity = (U32)~LAYERSUB_VALID_VIEWPORT;

    /* Release Viewport */
    layersub_ReleaseElement(&(Device_p->ViewPortDataPool), (void*) VPHandle);

    return(Err);
}

/*******************************************************************************
Name        : LAYERSUB_EnableViewPort
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYERSUB_EnableViewPort(STLAYER_ViewPortHandle_t VPHandle)
{
    ST_ErrorCode_t          Err         = ST_NO_ERROR;
    layersub_ViewPort_t*    ViewPort_p  = (layersub_ViewPort_t*) VPHandle ;
    layersub_Unit_t*        Unit_p;
    layersub_Device_t*      Device_p;

    /* Check Handle validity */
    if (VPHandle == (STLAYER_ViewPortHandle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (ViewPort_p->ViewPortValidity != LAYERSUB_VALID_VIEWPORT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }


    Unit_p   = (layersub_Unit_t*)ViewPort_p->Handle;
    Device_p = Unit_p->Device_p;

    /* Check wheter there is already an other Viewport enabled on the device */
    if (Device_p->ActiveViewPort != LAYERSUB_NO_VIEWPORT_HANDLE)
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    /* Enable ViewPort */
    ViewPort_p->Enable       = TRUE;
    Device_p->ActiveViewPort = VPHandle;

    /* Set HW Display if requested by mixer and start SPU */
    if ((Device_p->MixerConnected == TRUE) && (Device_p->DisplayStatus == LAYERSUB_DISPLAY_STATUS_ENABLE))
    {
        /* Start SPU */
        layersub_StartSPU(Device_p);
    }

    return(Err);
}

/*******************************************************************************
Name        : LAYERSUB_DisableViewPort
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYERSUB_DisableViewPort(STLAYER_ViewPortHandle_t VPHandle)
{
    ST_ErrorCode_t          Err         = ST_NO_ERROR;
    layersub_ViewPort_t*    ViewPort_p  = (layersub_ViewPort_t*) VPHandle ;
    layersub_Unit_t*        Unit_p;
    layersub_Device_t*      Device_p;

    /* Check Handle validity */
    if (VPHandle == (STLAYER_ViewPortHandle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (ViewPort_p->ViewPortValidity != LAYERSUB_VALID_VIEWPORT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    Unit_p   = (layersub_Unit_t*)ViewPort_p->Handle;
    Device_p = Unit_p->Device_p;

    if (ViewPort_p->Enable == FALSE)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Disable ViewPort */
    ViewPort_p->Enable       = FALSE;
    Device_p->ActiveViewPort = LAYERSUB_NO_VIEWPORT_HANDLE;

    /* Reset HW Display if requested by mixer and stop SPU*/
    if (Device_p->DisplayStatus == LAYERSUB_DISPLAY_STATUS_ENABLE) /* Mixer implicetely connected */
    {
        layersub_StopSPU(Device_p);
    }

    return(Err);
}

/*******************************************************************************
Name        : LAYERSUB_UpdateFromMixer
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYERSUB_UpdateFromMixer(STLAYER_Handle_t Handle, STLAYER_OutputParams_t* OutputParams_p)
{
    ST_ErrorCode_t                  Err         = ST_NO_ERROR;
    layersub_Unit_t*                Unit_p      = (layersub_Unit_t*)Handle;
    layersub_Device_t*              Device_p;
    STEVT_DeviceSubscribeParams_t   SubscribeParams;

    /* Check Handle validity */
    if (Handle == (STLAYER_Handle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (Unit_p->UnitValidity != LAYERSUB_VALID_UNIT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    Device_p = Unit_p->Device_p ;

    /* Check Output params */
    if (OutputParams_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
    /*  Check Output params ....(ScanType, AspectRatio ...)*/


    if ((OutputParams_p->UpdateReason & 0x20) == STLAYER_DISPLAY_REASON)
    {
        /* Update Device Layer */
        if (OutputParams_p->DisplayEnable == TRUE)
        {
             if (Device_p->DisplayStatus == LAYERSUB_DISPLAY_STATUS_DISABLE)
             {
                 /* enable display */
                 Device_p->DisplayStatus = LAYERSUB_DISPLAY_STATUS_ENABLE;

                 /* Automatical connection */
                 Device_p->MixerConnected = TRUE;

                 /* Update active viewport */
                 if (Device_p->ActiveViewPort != LAYERSUB_NO_VIEWPORT_HANDLE)
                 {
                     /* Start SPU */
                     layersub_StartSPU(Device_p);
                 }
             }
         }
         else /* DisplayEnable == FALSE */
         {
             if (Device_p->DisplayStatus == LAYERSUB_DISPLAY_STATUS_ENABLE) /* Mixer implicitely connected */
             {
                 Device_p->DisplayStatus = LAYERSUB_DISPLAY_STATUS_DISABLE;

                 /* Update Active Viewport */
                 if (Device_p->ActiveViewPort != LAYERSUB_NO_VIEWPORT_HANDLE)
                 {
                    layersub_StopSPU(Device_p);
                 }
             }
         }
    }

    if ((OutputParams_p->UpdateReason & 0x2) == STLAYER_SCREEN_PARAMS_REASON)
    {

         /* Check if device layer is fully inside Mixer plane (layer is always at (0,0) in mixer plane) */
         if ((OutputParams_p->Width < Device_p->LayerWidth) ||
             (OutputParams_p->Height < Device_p->LayerHeight))
         {
             return(STLAYER_ERROR_INSIDE_LAYER);   /* TBD */
         }

        /* Update Device Layer */
        Device_p->XStart      = OutputParams_p->XStart;
        Device_p->YStart      = OutputParams_p->YStart + OutputParams_p->YStart;
        Device_p->MixerWidth  = OutputParams_p->Width;
        Device_p->MixerHeight = OutputParams_p->Height;
        Device_p->FrameRate   = OutputParams_p->FrameRate;

        /* Update Register if layer connected to mixer, display enabled and Viewport enabled*/
        if ((Device_p->MixerConnected == TRUE) &&
            (Device_p->DisplayStatus == LAYERSUB_DISPLAY_STATUS_ENABLE) &&
            (Device_p->ActiveViewPort != LAYERSUB_NO_VIEWPORT_HANDLE))
        {
            /* Set Offsets registers */
            layersub_SetXDOYDO(Device_p);
        }

        /* Automatical connection */
        Device_p->MixerConnected = TRUE;
    }

    if ((OutputParams_p->UpdateReason & 0x8) == STLAYER_VTG_REASON)
    {
        if (Device_p->IsEvtVsyncSubscribed != FALSE)
        {
            Err = STEVT_UnsubscribeDeviceEvent(Device_p->EvtHandle, (char*)Device_p->VTGName, STVTG_VSYNC_EVT);
            if (Err != ST_NO_ERROR)
            {
                return(ST_ERROR_BAD_PARAMETER);
            }
        }

        /* Update device structure */
        Device_p->IsEvtVsyncSubscribed  = TRUE;
        strcpy(Device_p->VTGName,OutputParams_p->VTGName);

        /* Subscribe to Vsync event of VTG Name device */
        SubscribeParams.NotifyCallback      = layersub_ReceiveEvtVSync;
        SubscribeParams.SubscriberData_p    = (void*)Device_p;
        Err = STEVT_SubscribeDeviceEvent(Device_p->EvtHandle, (char*)Device_p->VTGName,
                                         STVTG_VSYNC_EVT,&SubscribeParams);
        if (Err != ST_NO_ERROR)
        {
            return(ST_ERROR_BAD_PARAMETER);
        }

        /* Automatical connection */
        Device_p->MixerConnected = TRUE;
    }


    if ((OutputParams_p->UpdateReason & 0x4) == STLAYER_OFFSET_REASON)
    {
         /* Update Device layer */
         Device_p->XOffset   = OutputParams_p->XOffset;
         Device_p->YOffset   = OutputParams_p->YOffset;

        /* Update Register if layer connected to mixer, display enabled and Viewport enabled*/
        if ((Device_p->MixerConnected == TRUE) &&
            (Device_p->DisplayStatus == LAYERSUB_DISPLAY_STATUS_ENABLE) &&
            (Device_p->ActiveViewPort != LAYERSUB_NO_VIEWPORT_HANDLE))
        {
            /* Set Offsets registers */
            layersub_SetXDOYDO(Device_p);
        }

         /* Automatical connection */
         Device_p->MixerConnected = TRUE;
    }


    if ((OutputParams_p->UpdateReason & 0x1) == STLAYER_DISCONNECT_REASON)
    {
        if (Device_p->MixerConnected == TRUE)
        {
            /* Update active viewport */
            if ((Device_p->ActiveViewPort != LAYERSUB_NO_VIEWPORT_HANDLE)  &&
                (Device_p->DisplayStatus == LAYERSUB_DISPLAY_STATUS_ENABLE))
            {
                layersub_StopSPU(Device_p);
            }

            /* Reset Device */
            Device_p->MixerConnected    = FALSE;
            Device_p->DisplayStatus     = LAYERSUB_DISPLAY_STATUS_DISABLE;
            Device_p->MixerWidth        = Device_p->LayerWidth;
            Device_p->MixerHeight       = Device_p->LayerHeight;
            Device_p->XStart            = 132;  /* PAL */
            Device_p->YStart            = 23+23;   /* PAL */
            Device_p->XOffset           = 0;
            Device_p->YOffset           = 0;
        }
    }


    return(Err);
}

/*******************************************************************************
Name        : LAYERSUB_SetViewPortPosition
Description :
Parameters  :
Assumptions : Output PositionY is always even, no parity cheching : First implementation
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYERSUB_SetViewPortPosition(STLAYER_ViewPortHandle_t VPHandle,
                                            S32                       PositionX,
                                            S32                       PositionY)
{
    ST_ErrorCode_t          Err                 = ST_NO_ERROR;
    layersub_ViewPort_t*    ViewPort_p          = (layersub_ViewPort_t*) VPHandle ;
    layersub_Unit_t*        Unit_p;
    layersub_Device_t*      Device_p;

    /* Check Handle validity */
    if (VPHandle == (STLAYER_ViewPortHandle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (ViewPort_p->ViewPortValidity != LAYERSUB_VALID_VIEWPORT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    Unit_p   = (layersub_Unit_t*)ViewPort_p->Handle;
    Device_p = Unit_p->Device_p;

    /* Check ViewPort fit in Device layer */
    if ((PositionX < 0) || (PositionY  < 0))
    {
        return(STLAYER_ERROR_OUT_OF_LAYER);
    }
    if (((U32)(PositionX + ViewPort_p->OutputRectangle.Width - 1) > Device_p->LayerWidth) ||
        ((U32)(PositionY + ViewPort_p->OutputRectangle.Height - 1) > Device_p->LayerHeight))
    {
        return(STLAYER_ERROR_OUT_OF_LAYER);
    }

    /* Output Position Y must be even */
    if ((PositionY % 2) != 0 )
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }


    /* Adjust positions ?*/

    /* Update ViewPort output rectangle related information. */
    ViewPort_p->OutputRectangle.PositionX = PositionX;
    ViewPort_p->OutputRectangle.PositionY = PositionY;

    if ((Device_p->MixerConnected == TRUE) &&
        (Device_p->DisplayStatus == LAYERSUB_DISPLAY_STATUS_ENABLE) &&
        (Device_p->ActiveViewPort == VPHandle))
    {

        /* Set Display area register */
        layersub_SetXDOYDO(Device_p);
    }

    return(Err);
}

/*******************************************************************************
Name        : LAYERSUB_SetViewPortIORectangle
Description :
Parameters  :
Assumptions : Output PositionY is always even, no parity cheching : First implementation
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYERSUB_SetViewPortIORectangle(STLAYER_ViewPortHandle_t VPHandle,
                                               STGXOBJ_Rectangle_t*      InputRectangle_p,
                                               STGXOBJ_Rectangle_t*      OutputRectangle_p)
{
    ST_ErrorCode_t              Err         = ST_NO_ERROR;
    layersub_ViewPort_t*        ViewPort_p  = (layersub_ViewPort_t*) VPHandle ;
    layersub_Unit_t*            Unit_p;
    layersub_Device_t*          Device_p;
    BOOL                        InputRectanglePositionChanged   = FALSE;
    BOOL                        OutputRectanglePositionChanged  = FALSE;
    BOOL                        RectangleSizeChanged            = FALSE;

    /* Check Handle validity */
    if (VPHandle == (STLAYER_ViewPortHandle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (ViewPort_p->ViewPortValidity != LAYERSUB_VALID_VIEWPORT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    Unit_p   = (layersub_Unit_t*)ViewPort_p->Handle;
    Device_p = Unit_p->Device_p;


    /* Check Params */
    if ((InputRectangle_p == NULL) ||
        (OutputRectangle_p == NULL))
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Check Rectangles */
    Err = CheckViewPortRectangle(Device_p,&(ViewPort_p->Bitmap),InputRectangle_p,OutputRectangle_p);
    if (Err != ST_NO_ERROR)
    {
        return(Err);
    }

    /* Adjust position and size */

    /* Check if Input rectangle position changed */
    if ((InputRectangle_p->PositionX != ViewPort_p->InputRectangle.PositionX) ||
        (InputRectangle_p->PositionY != ViewPort_p->InputRectangle.PositionY))
    {
        InputRectanglePositionChanged           = TRUE;
        ViewPort_p->InputRectangle.PositionX    = InputRectangle_p->PositionX;
        ViewPort_p->InputRectangle.PositionY    = InputRectangle_p->PositionY;
    }

    /* Check if Output rectangle position changed */
    if ((OutputRectangle_p->PositionX != ViewPort_p->OutputRectangle.PositionX) ||
        (OutputRectangle_p->PositionY != ViewPort_p->OutputRectangle.PositionY))
    {
        OutputRectanglePositionChanged          = TRUE;
        ViewPort_p->OutputRectangle.PositionX   = OutputRectangle_p->PositionX;
        ViewPort_p->OutputRectangle.PositionY   = OutputRectangle_p->PositionY;
    }

    /* Check if Input/Output rectangle changed */
    if ((ViewPort_p->InputRectangle.Width != InputRectangle_p->Width) ||
        (ViewPort_p->InputRectangle.Height != InputRectangle_p->Height))
    {
        RectangleSizeChanged                = TRUE;
        ViewPort_p->InputRectangle.Width    = InputRectangle_p->Width;
        ViewPort_p->InputRectangle.Height   = InputRectangle_p->Height;
        ViewPort_p->OutputRectangle.Width   = OutputRectangle_p->Width;
        ViewPort_p->OutputRectangle.Height  = OutputRectangle_p->Height;
    }

    if ((OutputRectanglePositionChanged == FALSE) &&
        (InputRectanglePositionChanged == FALSE) &&
        (RectangleSizeChanged == FALSE))
    {
        return(Err);
    }

    if ((Device_p->MixerConnected == TRUE) &&
        (Device_p->DisplayStatus == LAYERSUB_DISPLAY_STATUS_ENABLE) &&
        (Device_p->ActiveViewPort == VPHandle))
    {
        if ((OutputRectanglePositionChanged == TRUE) &&
            (InputRectanglePositionChanged == FALSE) &&
            (RectangleSizeChanged == FALSE))
        {
            /* Set Display area register */
            layersub_SetXDOYDO(Device_p);
        }
        else
        {
            if (OutputRectanglePositionChanged == TRUE)
            {
                /* Set Display area register */
                layersub_SetXDOYDO(Device_p);
            }

            /* Prepare new SPU, swap with old one, release old one  and update ViewPort Descriptor */
            Err = layersub_SwapActiveSPU(Device_p,ViewPort_p);
        }
    }


    return(Err);
}

/*******************************************************************************
Name        : LAYERSUB_SetViewPortSource
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYERSUB_SetViewPortSource(STLAYER_ViewPortHandle_t    VPHandle,
                                          STLAYER_ViewPortSource_t*    VPSource_p)
{
    ST_ErrorCode_t          Err         = ST_NO_ERROR;
    layersub_ViewPort_t*    ViewPort_p  = (layersub_ViewPort_t*) VPHandle ;
    layersub_Unit_t*        Unit_p;
    layersub_Device_t*      Device_p;

    /* Check Handle validity */
    if (VPHandle == (STLAYER_ViewPortHandle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (ViewPort_p->ViewPortValidity != LAYERSUB_VALID_VIEWPORT)
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

    Unit_p   = (layersub_Unit_t*)ViewPort_p->Handle;
    Device_p = Unit_p->Device_p;

    /* Update ViewPort descriptor */
    ViewPort_p->Bitmap    = *(VPSource_p->Data.BitMap_p);
    ViewPort_p->Palette   = *(VPSource_p->Palette_p);

    /* Fisrt implementation : Copy in current SPU (even if started).
     * This is possible because all SPU are allocated for max size !
     *
     * if SPU fit exactly the RLE bitmap rectangle size, then we have to create new SPU
     * set all its parmeters (Update ViewPort accordingly) and then stop the current one , and start the new one*/

    /* Write Top and Bottom  data */
    layersub_SetData(Device_p, ViewPort_p);

    if ((Device_p->MixerConnected == TRUE) &&
        (Device_p->DisplayStatus == LAYERSUB_DISPLAY_STATUS_ENABLE) &&
        (Device_p->ActiveViewPort == VPHandle))
    {
        /* Set Palette register if changed!*/
        layersub_SetPalette(Device_p);
    }

    return(Err);
}

/*******************************************************************************
Name        : LAYERSUB_SetViewPortParams
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYERSUB_SetViewPortParams(STLAYER_ViewPortHandle_t  VPHandle,
                                          STLAYER_ViewPortParams_t* Params_p)
{
    ST_ErrorCode_t              Err         = ST_NO_ERROR;
    layersub_ViewPort_t*        ViewPort_p  = (layersub_ViewPort_t*) VPHandle ;
    layersub_Unit_t*            Unit_p;
    layersub_Device_t*          Device_p;
    BOOL                        InputRectanglePositionChanged   = FALSE;
    BOOL                        OutputRectanglePositionChanged  = FALSE;
    BOOL                        RectangleSizeChanged            = FALSE;

    /* Check Handle validity */
    if (VPHandle == (STLAYER_ViewPortHandle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (ViewPort_p->ViewPortValidity != LAYERSUB_VALID_VIEWPORT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (Params_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    Unit_p   = (layersub_Unit_t*)ViewPort_p->Handle;
    Device_p = Unit_p->Device_p;

    /* Check ViewPort params */
    Err = CheckViewPortParams(Device_p, Params_p);
    if (Err != ST_NO_ERROR)
    {
        return(Err);
    }

    /* Update Bitmap and Palette Datas in ViewPort descriptor */
    ViewPort_p->Bitmap   = *(Params_p->Source_p->Data.BitMap_p);
    ViewPort_p->Palette  = *(Params_p->Source_p->Palette_p);

    /* Check if Input rectangle position changed */
    if ((Params_p->InputRectangle.PositionX != ViewPort_p->InputRectangle.PositionX) ||
        (Params_p->InputRectangle.PositionY != ViewPort_p->InputRectangle.PositionY))
    {
        InputRectanglePositionChanged           = TRUE;
        ViewPort_p->InputRectangle.PositionX    = Params_p->InputRectangle.PositionX;
        ViewPort_p->InputRectangle.PositionY    = Params_p->InputRectangle.PositionY;
    }

    /* Check if Output rectangle position changed */
    if ((Params_p->OutputRectangle.PositionX != ViewPort_p->OutputRectangle.PositionX) ||
        (Params_p->OutputRectangle.PositionY != ViewPort_p->OutputRectangle.PositionY))
    {
        OutputRectanglePositionChanged          = TRUE;
        ViewPort_p->OutputRectangle.PositionX   = Params_p->OutputRectangle.PositionX;
        ViewPort_p->OutputRectangle.PositionY   = Params_p->OutputRectangle.PositionY;
    }

    /* Check if Input/Output rectangle changed */
    if ((ViewPort_p->InputRectangle.Width != Params_p->InputRectangle.Width) ||
        (ViewPort_p->InputRectangle.Height != Params_p->InputRectangle.Height))
    {
        RectangleSizeChanged                = TRUE;
        ViewPort_p->InputRectangle.Width    = Params_p->InputRectangle.Width;
        ViewPort_p->InputRectangle.Height   = Params_p->InputRectangle.Height;
        ViewPort_p->OutputRectangle.Width   = Params_p->OutputRectangle.Width;
        ViewPort_p->OutputRectangle.Height  = Params_p->OutputRectangle.Height;
    }

    if ((OutputRectanglePositionChanged == FALSE) &&
        (InputRectanglePositionChanged == FALSE) &&
        (RectangleSizeChanged == FALSE))
    {
        return(Err);
    }

    if ((Device_p->MixerConnected == TRUE) &&
        (Device_p->DisplayStatus == LAYERSUB_DISPLAY_STATUS_ENABLE) &&
        (Device_p->ActiveViewPort == VPHandle))
    {
        if ((OutputRectanglePositionChanged == TRUE) &&
            (InputRectanglePositionChanged == FALSE) &&
            (RectangleSizeChanged == FALSE))
        {
            /* Write Top and Bottom  data */
            layersub_SetData(Device_p, ViewPort_p);

            /* Set Palette register if changed!*/
            layersub_SetPalette(Device_p);

            /* Set Display area register */
            layersub_SetXDOYDO(Device_p);
        }
        else
        {
            if (OutputRectanglePositionChanged == TRUE)
            {
                /* Set Display area register */
                layersub_SetXDOYDO(Device_p);
            }

            /* Prepare new SPU, swap with old one, release old one  and update ViewPort Descriptor */
            Err = layersub_SwapActiveSPU(Device_p,ViewPort_p);

        }
    }
    return(Err);
}

/*******************************************************************************
Name        : LAYERSUB_SetViewPortAlpha
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYERSUB_SetViewPortAlpha(STLAYER_ViewPortHandle_t  VPHandle,
                                         STLAYER_GlobalAlpha_t*    Alpha_p)
{
    ST_ErrorCode_t              Err         = ST_NO_ERROR;
    layersub_ViewPort_t*        ViewPort_p  = (layersub_ViewPort_t*) VPHandle ;
    layersub_Unit_t*            Unit_p;
    layersub_Device_t*          Device_p;

    /* Check Handle validity */
    if (VPHandle == (STLAYER_ViewPortHandle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (ViewPort_p->ViewPortValidity != LAYERSUB_VALID_VIEWPORT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (Alpha_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    Unit_p   = (layersub_Unit_t*)ViewPort_p->Handle;
    Device_p = Unit_p->Device_p;

    /* Update ViewPort descriptor */
    ViewPort_p->GlobalAlpha.A0 = Alpha_p->A0;

    /* Update Palette values  */
    layersub_UpdateContrast(Device_p, ViewPort_p);

    return(Err);
}


/*******************************************************************************
Name        : LAYERSUB_GetViewPortAlpha
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYERSUB_GetViewPortAlpha(STLAYER_ViewPortHandle_t  VPHandle,
                                         STLAYER_GlobalAlpha_t*    Alpha_p)
{
    ST_ErrorCode_t              Err         = ST_NO_ERROR;
    layersub_ViewPort_t*        ViewPort_p  = (layersub_ViewPort_t*) VPHandle ;

    /* Check Handle validity */
    if (VPHandle == (STLAYER_ViewPortHandle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (ViewPort_p->ViewPortValidity != LAYERSUB_VALID_VIEWPORT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (Alpha_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    Alpha_p->A0 = ViewPort_p->GlobalAlpha.A0;


    return(Err);
}

/*******************************************************************************
Name        : LAYERSUB_GetViewPortIORectangle
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYERSUB_GetViewPortIORectangle(STLAYER_ViewPortHandle_t VPHandle,
                                               STGXOBJ_Rectangle_t*      InputRectangle_p,
                                               STGXOBJ_Rectangle_t*      OutputRectangle_p)
{
    ST_ErrorCode_t              Err         = ST_NO_ERROR;
    layersub_ViewPort_t*        ViewPort_p  = (layersub_ViewPort_t*) VPHandle ;

    /* Check Handle validity */
    if (VPHandle == (STLAYER_ViewPortHandle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (ViewPort_p->ViewPortValidity != LAYERSUB_VALID_VIEWPORT)
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
    InputRectangle_p->PositionX = ViewPort_p->InputRectangle.PositionX;
    InputRectangle_p->PositionY = ViewPort_p->InputRectangle.PositionY;
    InputRectangle_p->Width     = ViewPort_p->InputRectangle.Width;
    InputRectangle_p->Height    = ViewPort_p->InputRectangle.Height;

    OutputRectangle_p->PositionX = ViewPort_p->OutputRectangle.PositionX;
    OutputRectangle_p->PositionY = ViewPort_p->OutputRectangle.PositionY;
    OutputRectangle_p->Width     = ViewPort_p->OutputRectangle.Width;
    OutputRectangle_p->Height    = ViewPort_p->OutputRectangle.Height;

    return(Err);
}

/*******************************************************************************
Name        : LAYERSUB_GetViewPortParams
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYERSUB_GetViewPortParams(STLAYER_ViewPortHandle_t  VPHandle,
                                          STLAYER_ViewPortParams_t* Params_p)
{
    ST_ErrorCode_t              Err         = ST_NO_ERROR;
    layersub_ViewPort_t*        ViewPort_p  = (layersub_ViewPort_t*) VPHandle ;

    /* Check Handle validity */
    if (VPHandle == (STLAYER_ViewPortHandle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (ViewPort_p->ViewPortValidity != LAYERSUB_VALID_VIEWPORT)
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
    if((Params_p->Source_p == NULL)||(Params_p->Source_p->Palette_p == NULL))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
    Params_p->Source_p->SourceType      = STLAYER_GRAPHIC_BITMAP;
    *(Params_p->Source_p->Data.BitMap_p)= (ViewPort_p->Bitmap);
    *(Params_p->Source_p->Palette_p)    = (ViewPort_p->Palette);
    Params_p->InputRectangle.PositionX  = ViewPort_p->InputRectangle.PositionX;
    Params_p->InputRectangle.PositionY  = ViewPort_p->InputRectangle.PositionY;
    Params_p->InputRectangle.Width      = ViewPort_p->InputRectangle.Width;
    Params_p->InputRectangle.Height     = ViewPort_p->InputRectangle.Height;

    Params_p->OutputRectangle.PositionX = ViewPort_p->OutputRectangle.PositionX;
    Params_p->OutputRectangle.PositionY = ViewPort_p->OutputRectangle.PositionY;
    Params_p->OutputRectangle.Width     = ViewPort_p->OutputRectangle.Width;
    Params_p->OutputRectangle.Height    = ViewPort_p->OutputRectangle.Height;

    return(Err);
}

/*******************************************************************************
Name        : LAYERSUB_GetViewPortSource
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYERSUB_GetViewPortSource(STLAYER_ViewPortHandle_t  VPHandle,
                                          STLAYER_ViewPortSource_t*  VPSource_p)
{
    ST_ErrorCode_t          Err         = ST_NO_ERROR;
    layersub_ViewPort_t*    ViewPort_p  = (layersub_ViewPort_t*) VPHandle ;

    /* Check Handle validity */
    if (VPHandle == (STLAYER_ViewPortHandle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (ViewPort_p->ViewPortValidity != LAYERSUB_VALID_VIEWPORT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Check Params */
    if (VPSource_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
    if((VPSource_p->Palette_p == NULL)||(VPSource_p->Data.BitMap_p == NULL))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
    /* Return value */
    VPSource_p->SourceType      = STLAYER_GRAPHIC_BITMAP;
    *(VPSource_p->Data.BitMap_p)= (ViewPort_p->Bitmap);
    *(VPSource_p->Palette_p)    = (ViewPort_p->Palette);

    return(Err);
}

/*******************************************************************************
Name        : LAYERSUB_GetViewPortPosition
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYERSUB_GetViewPortPosition(STLAYER_ViewPortHandle_t VPHandle,
                                            S32*                     PositionX_p,
                                            S32*                     PositionY_p)
{
    ST_ErrorCode_t          Err                 = ST_NO_ERROR;
    layersub_ViewPort_t*    ViewPort_p          = (layersub_ViewPort_t*) VPHandle ;


    /* Check Handle validity */
    if (VPHandle == (STLAYER_ViewPortHandle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (ViewPort_p->ViewPortValidity != LAYERSUB_VALID_VIEWPORT)
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
Name        : LAYERSUB_AdjustViewPortParams
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYERSUB_AdjustViewPortParams(STLAYER_Handle_t           Handle,
                                             STLAYER_ViewPortParams_t*  Params_p)
{
    ST_ErrorCode_t      Err = ST_NO_ERROR;
    layersub_Unit_t*    Unit_p      = (layersub_Unit_t*)Handle;
    layersub_Device_t*  Device_p;
    S32                 XOffset;
    S32                 YOffset;

    /* Check Handle validity */
    if (Handle == (STLAYER_Handle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    Device_p = Unit_p->Device_p ;

    if (Unit_p->UnitValidity != LAYERSUB_VALID_UNIT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Check params & Source  */
    if (Params_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
    if (Params_p->Source_p->SourceType != STLAYER_GRAPHIC_BITMAP)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
    if (Params_p->Source_p->Data.BitMap_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Check and Adjust Rectangles (adjusted rectangles will at least include original ones!)*/
    /* For test below, we take as reference the Input Width and Height. Output Width and height will be set to the input values !*/

    /* Check Input rectangle */
    if (Params_p->InputRectangle.PositionX < 0)
    {
        XOffset = -(Params_p->InputRectangle.PositionX);

        /* Update Input Width and position*/
        Params_p->InputRectangle.Width = (U32)(((S32)Params_p->InputRectangle.Width) -  XOffset);
        Params_p->InputRectangle.PositionX = 0;

        /* Update Output Position */
        Params_p->OutputRectangle.PositionX = Params_p->OutputRectangle.PositionX + XOffset;
    }
    if (Params_p->InputRectangle.PositionY < 0)
    {
        YOffset = -(Params_p->InputRectangle.PositionY);

        /* Update Input Heigth and position*/
        Params_p->InputRectangle.Height = (U32)(((S32)Params_p->InputRectangle.Height) -  YOffset);
        Params_p->InputRectangle.PositionY = 0;

        /* Update Output Position  */
        Params_p->OutputRectangle.PositionY = Params_p->OutputRectangle.PositionY + YOffset;
    }

    /* Check Output Rectangle*/
    if (Params_p->OutputRectangle.PositionX < 0)
    {
        XOffset = -(Params_p->OutputRectangle.PositionX);

        /* Update input params*/
        Params_p->InputRectangle.Width = (U32)(((S32)Params_p->InputRectangle.Width) - XOffset);
        Params_p->InputRectangle.PositionX = Params_p->InputRectangle.PositionX + XOffset;

        /* Update output position */
        Params_p->OutputRectangle.PositionX = 0;
    }
    if (Params_p->OutputRectangle.PositionY < 0)
    {
        YOffset = -(Params_p->OutputRectangle.PositionY);

        /* Update input params*/
        Params_p->InputRectangle.Height = (U32)(((S32)Params_p->InputRectangle.Height) - YOffset);
        Params_p->InputRectangle.PositionY = Params_p->InputRectangle.PositionY + YOffset;

        /* Update output position */
        Params_p->OutputRectangle.PositionY = 0;
    }

    /* Output Position Y must be even */
    if ((Params_p->OutputRectangle.PositionY % 2) != 0 )
    {
        /* Update Output params */
        Params_p->OutputRectangle.PositionY  = Params_p->OutputRectangle.PositionY + 1 ;
    }


    /* Check Input out of bitmap */
    if ((U32)(Params_p->InputRectangle.PositionX + Params_p->InputRectangle.Width) > Params_p->Source_p->Data.BitMap_p->Width)
    {
        XOffset = Params_p->InputRectangle.PositionX + (S32)(Params_p->InputRectangle.Width) -
                  (S32)(Params_p->Source_p->Data.BitMap_p->Width) ;

        /* Update Input width */
        Params_p->InputRectangle.Width = (U32)((S32)(Params_p->InputRectangle.Width) - XOffset );

    }
    if ((U32)(Params_p->InputRectangle.PositionY + Params_p->InputRectangle.Height) > Params_p->Source_p->Data.BitMap_p->Height)
    {
        YOffset = Params_p->InputRectangle.PositionY + (S32)(Params_p->InputRectangle.Height) -
                  (S32)(Params_p->Source_p->Data.BitMap_p->Height) ;

        /* Update Input height */
        Params_p->InputRectangle.Height = (U32)((S32)(Params_p->InputRectangle.Height) - YOffset );

    }

    /* Check Output out of layer (Use Input Width and Height !!!)*/
    if ((U32)(Params_p->OutputRectangle.PositionX + Params_p->InputRectangle.Width) > Device_p->LayerWidth)
    {
        XOffset = Params_p->OutputRectangle.PositionX + (S32)(Params_p->InputRectangle.Width) -
                  (S32)(Device_p->LayerWidth) ;

        /* Update Input width */
        Params_p->InputRectangle.Width = (U32)((S32)(Params_p->InputRectangle.Width) - XOffset );

    }
    if ((U32)(Params_p->OutputRectangle.PositionY + Params_p->InputRectangle.Height) > Device_p->LayerHeight)
    {
        YOffset = Params_p->OutputRectangle.PositionY + (S32)(Params_p->InputRectangle.Height) -
                  (S32)(Device_p->LayerHeight) ;

        /* Update Input height */
        Params_p->InputRectangle.Height = (U32)((S32)(Params_p->InputRectangle.Height) - YOffset );

    }

    /* Input rectangle height is even */
    if ((Params_p->InputRectangle.Height % 2) != 0)
    {
        Params_p->InputRectangle.Height = Params_p->InputRectangle.Height  + 1;
    }

    /* Check max rectangle size (128*128) */
    if (Params_p->InputRectangle.Width > 128)
    {
        Params_p->InputRectangle.Width = 128;
    }

    if (Params_p->InputRectangle.Height > 128)
    {
        Params_p->InputRectangle.Height = 128;
    }

    /* set Output Width and Height = Input ones*/
    Params_p->OutputRectangle.Height     = Params_p->InputRectangle.Height;
    Params_p->OutputRectangle.Width      = Params_p->InputRectangle.Width;

    return(Err);
}


/*******************************************************************************
Name        : LAYERSUB_AdjustIORectangle
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYERSUB_AdjustIORectangle(STLAYER_Handle_t       Handle,
                                          STGXOBJ_Rectangle_t*   InputRectangle_p,
                                          STGXOBJ_Rectangle_t*   OutputRectangle_p)
{
    ST_ErrorCode_t      Err = ST_NO_ERROR;
    layersub_Unit_t*    Unit_p      = (layersub_Unit_t*)Handle;
    layersub_Device_t*  Device_p;
    S32                 XOffset;
    S32                 YOffset;


    /* Check Handle validity */
    if (Handle == (STLAYER_Handle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    Device_p = Unit_p->Device_p ;

    if (Unit_p->UnitValidity != LAYERSUB_VALID_UNIT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Check params & Source  */
    if ((InputRectangle_p == NULL) ||
        (OutputRectangle_p == NULL))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
    /* Check and Adjust Rectangles (adjusted rectangles will at least include original ones!)*/
    /* For test below, we take as reference the Input Width and Height. Output Width and height will be set to the input values !*/

    /* Check Input rectangle */
    if (InputRectangle_p->PositionX < 0)
    {
        XOffset = -(InputRectangle_p->PositionX);

        /* Update Input Width and position*/
        InputRectangle_p->Width = (U32)(((S32)InputRectangle_p->Width) -  XOffset);
        InputRectangle_p->PositionX = 0;

        /* Update Output Position */
        OutputRectangle_p->PositionX = OutputRectangle_p->PositionX + XOffset;
    }
    if (InputRectangle_p->PositionY < 0)
    {
        YOffset = -(InputRectangle_p->PositionY);

        /* Update Input Heigth and position*/
        InputRectangle_p->Height = (U32)(((S32)InputRectangle_p->Height) -  YOffset);
        InputRectangle_p->PositionY = 0;

        /* Update Output Position  */
        OutputRectangle_p->PositionY = OutputRectangle_p->PositionY + YOffset;
    }

    /* Check Output Rectangle*/
    if (OutputRectangle_p->PositionX < 0)
    {
        XOffset = -(OutputRectangle_p->PositionX);

        /* Update input params*/
        InputRectangle_p->Width = (U32)(((S32)InputRectangle_p->Width) - XOffset);
        InputRectangle_p->PositionX = InputRectangle_p->PositionX + XOffset;

        /* Update output position */
        OutputRectangle_p->PositionX = 0;
    }
    if (OutputRectangle_p->PositionY < 0)
    {
        YOffset = -(OutputRectangle_p->PositionY);

        /* Update input params*/
        InputRectangle_p->Height = (U32)(((S32)InputRectangle_p->Height) - YOffset);
        InputRectangle_p->PositionY = InputRectangle_p->PositionY + YOffset;

        /* Update output position */
        OutputRectangle_p->PositionY = 0;
    }

    /* Output Position Y must be even */
    if ((OutputRectangle_p->PositionY % 2) != 0 )
    {
        /* Update Output params */
        OutputRectangle_p->PositionY  = OutputRectangle_p->PositionY + 1 ;
    }



    /* Input rectangle height is even */
    if ((InputRectangle_p->Height % 2) != 0)
    {
        InputRectangle_p->Height = InputRectangle_p->Height  + 1;
    }

    /* Check max rectangle size (128*128) */
    if (InputRectangle_p->Width > 128)
    {
        InputRectangle_p->Width = 128;
    }

    if (InputRectangle_p->Height > 128)
    {
        InputRectangle_p->Height = 128;
    }

    /* set Output Width and Height = Input ones*/
    OutputRectangle_p->Height     = InputRectangle_p->Height;
    OutputRectangle_p->Width      = InputRectangle_p->Width;



    return(Err);
}


/* End of Sub_vp.c */

