/*******************************************************************************

File name   : lc_init.c

Description : laycompo init module standard API functions source file

COPYRIGHT (C) STMicroelectronics 2004

Date               Modification                                     Name
----               ------------                                     ----
Sept 2003        Created                                           TM
*******************************************************************************/

/* Includes ----------------------------------------------------------------- */

#include <stdlib.h>
#include <string.h>

#include "stddefs.h"
#include "sttbx.h"
#include "stlayer.h"
#include "lc_init.h"
#include "lc_vp.h"
#include "lc_pool.h"
#include "laycompo.h"
#include "stsys.h"
#include "lcswcfg.h"
#include "stvtg.h"


/* Private Types ------------------------------------------------------------ */


/* Private Constants -------------------------------------------------------- */

#define LAYCOMPO_MAX_DEVICE  5     /* Max number of Init() allowed */
#define LAYCOMPO_MAX_OPEN    5     /* Max number of Open() allowed per Init() */
#define LAYCOMPO_MAX_UNIT    (LAYCOMPO_MAX_OPEN * LAYCOMPO_MAX_DEVICE)

#define INVALID_DEVICE_INDEX (-1)

/* Private Variables (static)------------------------------------------------ */

static laycompo_Device_t DeviceArray[LAYCOMPO_MAX_DEVICE];
static laycompo_Unit_t UnitArray[LAYCOMPO_MAX_UNIT];
static semaphore_t *InstancesAccessControl_p;   /* Init/Open/Close/Term protection semaphore */
static BOOL FirstInitDone = FALSE;


/* Global Variables --------------------------------------------------------- */


/* Private Macros ----------------------------------------------------------- */

/* Passing a (STLAYER_Handle_t), returns TRUE if the handle is valid, FALSE otherwise */
#define IsValidHandle(Handle) ((((laycompo_Unit_t *) (Handle)) >= UnitArray) &&                    \
                               (((laycompo_Unit_t *) (Handle)) < (UnitArray + LAYCOMPO_MAX_UNIT)) &&  \
                               (((laycompo_Unit_t *) (Handle))->UnitValidity == LAYCOMPO_VALID_UNIT))


/* Private Function prototypes ---------------------------------------------- */

static void EnterCriticalSection(void);
static void LeaveCriticalSection(void);
static S32 GetIndexOfDeviceNamed(const ST_DeviceName_t WantedName);
static ST_ErrorCode_t Init(laycompo_Device_t * const Device_p, const STLAYER_InitParams_t * const InitParams_p, const ST_DeviceName_t DeviceName);
static ST_ErrorCode_t Open(laycompo_Unit_t * const Unit_p, const STLAYER_OpenParams_t * const OpenParams_p);
static ST_ErrorCode_t Close(laycompo_Unit_t * const Unit_p);
static ST_ErrorCode_t Term(laycompo_Device_t * const Device_p, const STLAYER_TermParams_t * const TermParams_p);


/* Functions ---------------------------------------------------------------- */


/*******************************************************************************
Name        : EnterCriticalSection
Description : Used to protect critical sections of Init/Open/Close/Term
Parameters  : None
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void EnterCriticalSection(void)
{
    static BOOL InstancesAccessControlInitialized = FALSE;

    STOS_TaskLock();
    if (!InstancesAccessControlInitialized)
    {
        /* Initialise the Init/Open/Close/Term protection semaphore
          Caution: this semaphore is never deleted. (Not an issue) */
        InstancesAccessControl_p =STOS_SemaphoreCreateFifo(NULL,1);
        InstancesAccessControlInitialized = TRUE;
    }
    task_unlock();

    /* Wait for the Init/Open/Close/Term protection semaphore */
    STOS_SemaphoreWait(InstancesAccessControl_p);
} /* End of EnterCriticalSection() function */


/*******************************************************************************
Name        : LeaveCriticalSection
Description : Used to release protection of critical sections of Init/Open/Close/Term
Parameters  : None
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void LeaveCriticalSection(void)
{
    /* Release the Init/Open/Close/Term protection semaphore */
    STOS_SemaphoreSignal(InstancesAccessControl_p);
} /* End of LeaveCriticalSection() function */


/*******************************************************************************
Name        : SetupEvent
Description : Open  event handler and register event.
Parameters  :
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t for the API Init function
*******************************************************************************/
static ST_ErrorCode_t SetupEvent(laycompo_Device_t* Device_p,const ST_DeviceName_t DeviceName)
{
    STEVT_OpenParams_t              OpenParams;
    ST_ErrorCode_t                  Err;

    /* Open Enent handler */
    Err = STEVT_Open(Device_p->EventHandlerName, &OpenParams, &(Device_p->EvtHandle));

    /* Register API event */
     if (Err == ST_NO_ERROR)
    {
        Err = STEVT_RegisterDeviceEvent(Device_p->EvtHandle,(char*) DeviceName,
                                        STLAYER_UPDATE_PARAMS_EVT, &(Device_p->EventID[LAYCOMPO_UPDATE_PARAMS_EVT_ID]));
    }

    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error %X while Opening EventHandler %s or registering events",
                 Err, Device_p->EventHandlerName));
        Err = ST_ERROR_BAD_PARAMETER;
    }

    return(Err);

}



/*******************************************************************************
Name        :  LAYCOMPO_SetLayerParams
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYCOMPO_SetLayerParams(STLAYER_Handle_t Handle, STLAYER_LayerParams_t* LayerParams_p)
{
    ST_ErrorCode_t      Err         = ST_NO_ERROR;
    laycompo_Unit_t*    Unit_p      = (laycompo_Unit_t*)Handle;
    laycompo_Device_t*  Device_p;
    laycompo_ViewPort_t* CurrentViewPort_p;
    U32                 i;

    /* Check Handle validity */
    if (Handle == (STLAYER_Handle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    Device_p = Unit_p->Device_p ;

    if (Unit_p->UnitValidity != LAYCOMPO_VALID_UNIT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (LayerParams_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Check if device layer is fully inside Mixer plane (layer is always at (0,0) in mixer plane) */
    if ((Device_p->MixerWidth < LayerParams_p->Width) ||
        (Device_p->MixerHeight < LayerParams_p->Height))
    {
        return(STLAYER_ERROR_INSIDE_LAYER);   /* TBD */
    }

    /* Check if Open viewPorts fit in new Layer width and height */
    CurrentViewPort_p = (laycompo_ViewPort_t*) Device_p->FirstOpenViewPort;
    for (i=0; i < Device_p->NbOpenViewPorts; i++ )
    {
        /* Output rectangle position of ViewPort is always > 0 */
        if (((U32)(CurrentViewPort_p->OutputRectangle.PositionX +
                   CurrentViewPort_p->OutputRectangle.Width - 1) > LayerParams_p->Width) ||
            ((U32)(CurrentViewPort_p->OutputRectangle.PositionY +
                   CurrentViewPort_p->OutputRectangle.Height - 1) > LayerParams_p->Height))
        {
            return(STLAYER_ERROR_OUT_OF_LAYER);
        }
        CurrentViewPort_p = (laycompo_ViewPort_t*)(CurrentViewPort_p->NextViewPort);
    }

    /* Update Device_p structure */
    Device_p->ScanType       = LayerParams_p->ScanType;
    Device_p->AspectRatio    = LayerParams_p->AspectRatio;
    Device_p->LayerWidth     = LayerParams_p->Width;
    Device_p->LayerHeight    = LayerParams_p->Height;

    return(Err);
}

/*******************************************************************************
Name        :  LAYCOMPO_GetLayerParams
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYCOMPO_GetLayerParams(STLAYER_Handle_t Handle, STLAYER_LayerParams_t* LayerParams_p)
{
    ST_ErrorCode_t      Err         = ST_NO_ERROR;
    laycompo_Unit_t*    Unit_p      = (laycompo_Unit_t*)Handle;
    laycompo_Device_t*  Device_p;


    /* Check Handle validity */
    if (Handle == (STLAYER_Handle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    Device_p = Unit_p->Device_p ;

    if (Unit_p->UnitValidity != LAYCOMPO_VALID_UNIT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (LayerParams_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Get params */
    LayerParams_p->ScanType            = Device_p->ScanType;
    LayerParams_p->AspectRatio         = Device_p->AspectRatio;
    LayerParams_p->Width               = Device_p->LayerWidth;
    LayerParams_p->Height              = Device_p->LayerHeight;

    return(Err);
}

/*******************************************************************************
Name        :  LAYCOMPO_GetInitAllocParams
Description :  Gets the init allocation parameters.
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYCOMPO_GetInitAllocParams(STLAYER_Layer_t         LayerType,
                                           U32                     ViewPortsNumber,
                                           STLAYER_AllocParams_t * Params_p)
{
    ST_ErrorCode_t      Err  = ST_NO_ERROR;
    UNUSED_PARAMETER(LayerType);
    if ( ( ViewPortsNumber == 0 ) || ( Params_p == NULL ) )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    Params_p->ViewPortDescriptorsBufferSize = ViewPortsNumber * sizeof(laycompo_ViewPort_t);
    Params_p->ViewPortNodesInSharedMemory   = FALSE;
    Params_p->ViewPortNodesBufferAlignment  = 0;
    Params_p->ViewPortNodesBufferSize       = 0;

    return(Err);
}

/*******************************************************************************
Name        :  LAYCOMPO_GetBitmapAllocParams
Description :  Gets the bitmap allocation parameters.
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYCOMPO_GetBitmapAllocParams(STLAYER_Handle_t             Handle,
                                             STGXOBJ_Bitmap_t*            Bitmap_p,
                                             STGXOBJ_BitmapAllocParams_t* Params1_p,
                                             STGXOBJ_BitmapAllocParams_t* Params2_p)
{
    UNUSED_PARAMETER(Handle);

    Params1_p->AllocBlockParams.Alignment = 0;
    Params1_p->AllocBlockParams.Size      = 0;
    Params1_p->Pitch                      = 0;
    Params1_p->Offset                     = 0;
    Params2_p->AllocBlockParams.Alignment = 0;
    Params2_p->AllocBlockParams.Size      = 0;
    Params2_p->Pitch                      = 0;
    Params2_p->Offset                     = 0;

    /* Offset is 0 because we dont write any header in the bitmap data */
    /* allocation for a progressive bitmap */
    if(Bitmap_p->BitmapType == STGXOBJ_BITMAP_TYPE_RASTER_PROGRESSIVE)
    {
        /* 2nd alloc ununsed */
        switch(Bitmap_p->ColorType)
        {
            /* 4-Byte format */
            case STGXOBJ_COLOR_TYPE_ARGB8888:
            case STGXOBJ_COLOR_TYPE_SIGNED_AYCBCR8888:
            case STGXOBJ_COLOR_TYPE_UNSIGNED_AYCBCR8888:
                Params1_p->AllocBlockParams.Alignment = 4;
                Params1_p->Pitch = 4 * Bitmap_p->Width;
                Params1_p->AllocBlockParams.Size = Params1_p->Pitch *
                                                Bitmap_p->Height;
            break;
            /* 3-Byte formats */
            case STGXOBJ_COLOR_TYPE_ARGB8565:              /* fall through */
            case STGXOBJ_COLOR_TYPE_RGB888:                /* fall through */
            case STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_444:   /* fall through */
            case STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_444:
                Params1_p->AllocBlockParams.Alignment = 4;
                Params1_p->Pitch = (Bitmap_p->Width * 3);
                if (Params1_p->Pitch %4 != 0)
                {
                    Params1_p->Pitch += (4 - Bitmap_p->Width % 4);
                }
                Params1_p->AllocBlockParams.Size = Params1_p->Pitch *
                                                    Bitmap_p->Height;
            break;
            /* 2-Byte formats */
            case STGXOBJ_COLOR_TYPE_RGB565:   /* fall through */
            case STGXOBJ_COLOR_TYPE_ARGB1555: /* fall through */
            case STGXOBJ_COLOR_TYPE_ARGB4444: /* fall through */
            case STGXOBJ_COLOR_TYPE_ACLUT88:
                Params1_p->AllocBlockParams.Alignment = 2;
                Params1_p->Pitch = 2 * Bitmap_p->Width;
                Params1_p->AllocBlockParams.Size = Params1_p->Pitch *
                                                Bitmap_p->Height;
            break;
            /* 1-Byte formats */
            case STGXOBJ_COLOR_TYPE_CLUT8:   /* fall through */
            case STGXOBJ_COLOR_TYPE_ACLUT44: /* fall through */
            case STGXOBJ_COLOR_TYPE_ALPHA8:
                Params1_p->AllocBlockParams.Alignment = 1;
                Params1_p->Pitch = Bitmap_p->Width;
                Params1_p->AllocBlockParams.Size = Params1_p->Pitch *
                                                Bitmap_p->Height;
            break;
            /* 1/2-Byte format */
            case STGXOBJ_COLOR_TYPE_CLUT4:
                Params1_p->AllocBlockParams.Alignment = 1;
                Params1_p->Pitch = (Bitmap_p->Width/2) +
                        ((Bitmap_p->Width & 0x1) ? 1 : 0);
                Params1_p->AllocBlockParams.Size = Params1_p->Pitch *
                                                Bitmap_p->Height;
            break;
            /* 1/4-Byte format */
            case STGXOBJ_COLOR_TYPE_CLUT2:
                Params1_p->AllocBlockParams.Alignment = 1;
                Params1_p->Pitch = (Bitmap_p->Width/4)
                            + ((Bitmap_p->Width%4) ? 1 : 0);
                Params1_p->AllocBlockParams.Size = Params1_p->Pitch *
                                                    Bitmap_p->Height;
            break;
            /* 1/8-Byte formats */
            case STGXOBJ_COLOR_TYPE_CLUT1: /* fall through */
            case STGXOBJ_COLOR_TYPE_ALPHA1:
                Params1_p->AllocBlockParams.Alignment = 1;
                Params1_p->Pitch = (Bitmap_p->Width/8)
                            + ((Bitmap_p->Width%8) ? 1 : 0);
                Params1_p->AllocBlockParams.Size = Params1_p->Pitch *
                                                 Bitmap_p->Height;
            break;
            case STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_422: /* fall through */
            case STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422:
                Params1_p->AllocBlockParams.Alignment = 4;
                Params1_p->Pitch = (Bitmap_p->Width*2) +
                          ((Bitmap_p->Width & 0x1) ? 2 : 0);
                Params1_p->AllocBlockParams.Size = Params1_p->Pitch *
                                                 Bitmap_p->Height;
            break;

            default:
                return(ST_ERROR_FEATURE_NOT_SUPPORTED);
            break;
        } /* end switch */

    } /* endif bitmap is progressive */
    else /* if bitmap is top/bottom or macro block */
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    return(ST_NO_ERROR);


}

/*******************************************************************************
Name        :  LAYCOMPO_GetPaletteAllocParams
Description :  Gets the palette allocation parameters.
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYCOMPO_GetPaletteAllocParams(STLAYER_Handle_t               Handle,
                                              STGXOBJ_Palette_t*             Palette_p,
                                              STGXOBJ_PaletteAllocParams_t*  Params_p)
{
    UNUSED_PARAMETER(Handle);

    /* the color format of a palette can be only one of these three */
    if(Palette_p->ColorType != STGXOBJ_COLOR_TYPE_ARGB8888  &&
        Palette_p->ColorType != STGXOBJ_COLOR_TYPE_ARGB4444 )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    switch(Palette_p->ColorType)
    {
        case STGXOBJ_COLOR_TYPE_ARGB8888:
            Params_p->AllocBlockParams.Alignment = 16;
            Params_p->AllocBlockParams.Size = 4 * (1 << Palette_p->ColorDepth);
        break;
        case STGXOBJ_COLOR_TYPE_ARGB4444:
            Params_p->AllocBlockParams.Alignment = 16;
            Params_p->AllocBlockParams.Size = 2 * (1 << Palette_p->ColorDepth);
        break;
        default:
        break;
    }


    return(ST_NO_ERROR);
}

/*******************************************************************************
Name        :  LAYCOMPO_GetVTGName
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYCOMPO_GetVTGName(STLAYER_Handle_t LayerHandle, ST_DeviceName_t* const VTGName_p)
{
    ST_ErrorCode_t      Err     = ST_NO_ERROR;
    laycompo_Unit_t*    Unit_p  = (laycompo_Unit_t*)LayerHandle;
    laycompo_Device_t*  Device_p;

    /* Check Handle validity */
    if (LayerHandle == (STLAYER_Handle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    if (Unit_p->UnitValidity != LAYCOMPO_VALID_UNIT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Check general params */
    if (VTGName_p== NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    Device_p = Unit_p->Device_p ;

    strcpy(*VTGName_p,Device_p->VTGParams.VTGName);

    return(Err);

}

/*******************************************************************************
Name        :  LAYCOMPO_GetVTGParams
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYCOMPO_GetVTGParams(STLAYER_Handle_t LayerHandle,STLAYER_VTGParams_t* const VTGParams_p)
{
    ST_ErrorCode_t      Err         = ST_NO_ERROR;
    laycompo_Unit_t*    Unit_p      = (laycompo_Unit_t*)LayerHandle;
    laycompo_Device_t*  Device_p;

    /* Check Handle validity */
    if (LayerHandle == (STLAYER_Handle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    if (Unit_p->UnitValidity != LAYCOMPO_VALID_UNIT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Check general params */
    if (VTGParams_p== NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    Device_p = Unit_p->Device_p ;

    strcpy(VTGParams_p->VTGName,Device_p->VTGParams.VTGName);
    VTGParams_p->VTGFrameRate = Device_p->VTGParams.VTGFrameRate;

    return(Err);
}


/*******************************************************************************
Name        :  LAYCOMPO_GetBitmapHeaderSize
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYCOMPO_GetBitmapHeaderSize(STLAYER_Handle_t             Handle,
                                            STGXOBJ_Bitmap_t*            Bitmap_p,
                                            U32 *                        HeaderSize_p)
{
    ST_ErrorCode_t      Err  = ST_NO_ERROR;
    laycompo_Unit_t*    Unit_p      = (laycompo_Unit_t*)Handle;

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
    if ((HeaderSize_p == NULL) ||
        (Bitmap_p == NULL))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    *HeaderSize_p = 0;

    return(Err);
}

/*******************************************************************************
Name        : Init
Description : API specific initialisations
Parameters  : pointer on device and initialisation parameters
Assumptions : pointer on device is not NULL
Limitations :
Returns     : ST_ErrorCode_t for the API Init function
*******************************************************************************/
static ST_ErrorCode_t Init(laycompo_Device_t * const Device_p, const STLAYER_InitParams_t * const InitParams_p,
                           const ST_DeviceName_t DeviceName)
{
    ST_ErrorCode_t          Err               = ST_NO_ERROR;
    void*                   Elembuffer_p;
    void**                  Handlebuffer_p;
    U32                     NumberElem;
    laycompo_ViewPort_t*    ViewPort_p;
    U32                     i;

    /* common stuff initialization*/
    Device_p->CPUPartition_p            = InitParams_p->CPUPartition_p;
    Device_p->BaseAddress_p             = (void*)((U32)(InitParams_p->BaseAddress_p) + (U32)(InitParams_p->DeviceBaseAddress_p));
    Device_p->AVMEMPartition            = InitParams_p->AVMEM_Partition;

    /* Mapping related */
    STAVMEM_GetSharedMemoryVirtualMapping(&Device_p->VirtualMapping);
    Device_p->SharedMemoryBaseAddress_p = InitParams_p->SharedMemoryBaseAddress_p;


    /* ViewPort descriptor management initialization */
    NumberElem = InitParams_p->MaxViewPorts ;
    if (InitParams_p->ViewPortBufferUserAllocated)  /* buffer which is user allocated */
    {
        Elembuffer_p = (void*) InitParams_p->ViewPortBuffer_p;
        Device_p->ViewPortBufferUserAllocated = TRUE ;
    }
    else
    {
        Elembuffer_p = (void*) STOS_MemoryAllocate(Device_p->CPUPartition_p,
                                              (NumberElem * sizeof(laycompo_ViewPort_t)));
        if (Elembuffer_p == NULL)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "CPU memory allocation failed"));
            return(ST_ERROR_NO_MEMORY);
        }
        Device_p->ViewPortBufferUserAllocated = FALSE ;
    }
    Handlebuffer_p = (void**) STOS_MemoryAllocate(Device_p->CPUPartition_p,
                                             (NumberElem * sizeof(laycompo_ViewPort_t*)));
    if (Handlebuffer_p == NULL)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "CPU memory allocation failed"));
        return(ST_ERROR_NO_MEMORY);
    }
    laycompo_InitDataPool(&(Device_p->ViewPortDataPool),
                        NumberElem,
                        sizeof(laycompo_ViewPort_t),
                        Elembuffer_p,
                        Handlebuffer_p);

    /* Initialize ViewPort Validity field in the ViewPortDataPool */
    ViewPort_p = (laycompo_ViewPort_t*)Elembuffer_p;
    for (i=0; i<NumberElem; i++)
    {
        ViewPort_p->ViewPortValidity = (U32)(~LAYCOMPO_VALID_VIEWPORT);
        ViewPort_p++;
    }

    /* Device initialization */
    Device_p->ScanType              = InitParams_p->LayerParams_p->ScanType;
    Device_p->AspectRatio           = InitParams_p->LayerParams_p->AspectRatio;
    Device_p->LayerWidth        	= InitParams_p->LayerParams_p->Width;
    Device_p->LayerHeight       	= InitParams_p->LayerParams_p->Height;
    Device_p->MixerConnected    	= FALSE;
    Device_p->MixerWidth        	= Device_p->LayerWidth;
    Device_p->MixerHeight       	= Device_p->LayerHeight;
    Device_p->BackLayer             = LAYCOMPO_NO_LAYER_HANDLE;
    Device_p->FrontLayer            = LAYCOMPO_NO_LAYER_HANDLE;

    Device_p->IsEvtVsyncSubscribed  = FALSE;
	Device_p->FirstOpenViewPort 	= LAYCOMPO_NO_VIEWPORT_HANDLE;
    Device_p->LastOpenViewPort  	= LAYCOMPO_NO_VIEWPORT_HANDLE;
    Device_p->NbOpenViewPorts   	= 0;
    Device_p->DisplayHandle         = LAYCOMPO_NO_DISPLAY_HANDLE;

    Device_p->DisplayParamsForVideo.FrameBufferDisplayLatency   = 1;
    Device_p->DisplayParamsForVideo.FrameBufferHoldTime         = 1;

    /* Event open/register related  */
    strcpy(Device_p->EventHandlerName,InitParams_p->EventHandlerName);
    if (SetupEvent(Device_p,DeviceName) != ST_NO_ERROR)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    return(Err);
} /* End of Init() function */
/*******************************************************************************
Name        : Open
Description : API specific actions just before opening
Parameters  : pointer on unit and open parameters
Assumptions : pointer on unit is not NULL
Limitations :
Returns     : ST_ErrorCode_t for the API Open function
*******************************************************************************/
static ST_ErrorCode_t Open(laycompo_Unit_t * const Unit_p, const STLAYER_OpenParams_t * const OpenParams_p)
{
    UNUSED_PARAMETER(Unit_p);
    UNUSED_PARAMETER(OpenParams_p);

    return(ST_NO_ERROR);
} /* End of Open() function */

/*******************************************************************************
Name        : Close
Description : API specific actions just before closing
Parameters  : pointer on unit
Assumptions : pointer on unit is not NULL
Limitations :
Returns     : ST_ErrorCode_t for the API Close function
              If not ST_NO_ERROR, the Handle would not be closed afterwards
*******************************************************************************/
static ST_ErrorCode_t Close(laycompo_Unit_t * const Unit_p)
{
    UNUSED_PARAMETER(Unit_p);
    return( ST_NO_ERROR);
} /* End of Close() function */

/*******************************************************************************
Name        : Term
Description : API specific terminations
Parameters  : pointer on device and termination parameters
Assumptions : pointer on device is not NULL
Limitations :
Returns     : ST_ErrorCode_t for the API Term function
*******************************************************************************/
static ST_ErrorCode_t Term(laycompo_Device_t * const Device_p, const STLAYER_TermParams_t * const TermParams_p)
{
    ST_ErrorCode_t Err = ST_NO_ERROR;
    UNUSED_PARAMETER(TermParams_p);
    /* Event Close */
    if (STEVT_Close(Device_p->EvtHandle) != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Evt Close failed"));
    }

    /* Close all viewports   */
	if (Device_p->NbOpenViewPorts != 0)
    {
        /*  Device_p->FirstOpenViewPort != LAYCOMPO_NO_VIEWPORT_HANDLE */
        laycompo_CloseAllViewPortFromList(Device_p->FirstOpenViewPort);
    }

    /* ViewPort Descriptor Data Pool deallocation  */
    STOS_MemoryDeallocate(Device_p->CPUPartition_p, Device_p->ViewPortDataPool.HandleArray_p);
    if (Device_p->ViewPortBufferUserAllocated != TRUE)   /* buffer has been allocated by driver */
    {
        STOS_MemoryDeallocate(Device_p->CPUPartition_p, Device_p->ViewPortDataPool.ElemArray_p);
    }

    return(Err);
} /* End of Term() function */



/*******************************************************************************
Name        : GetIndexOfDeviceNamed
Description : Get the index in DeviceArray where a device has been
              initialised with the wanted name, if it exists
Parameters  : the name to look for
Assumptions :
Limitations :
Returns     : the index if the name was found, INVALID_DEVICE_INDEX otherwise
*******************************************************************************/
static S32 GetIndexOfDeviceNamed(const ST_DeviceName_t WantedName)
{
    S32 WantedIndex = INVALID_DEVICE_INDEX, Index = 0;

    do
    {
        /* Device initialised: check if name is matching */
        if (strcmp(DeviceArray[Index].DeviceName, WantedName) == 0)
        {
            /* Name found in the initialised devices */
            WantedIndex = Index;
        }
        Index++;
    } while ((WantedIndex == INVALID_DEVICE_INDEX) && (Index < LAYCOMPO_MAX_DEVICE));

    return(WantedIndex);
} /* End of GetIndexOfDeviceNamed() function */


/*
--------------------------------------------------------------------------------
Get capabilities of LAYCOMPO_GetCapability API
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t LAYCOMPO_GetCapability(const ST_DeviceName_t DeviceName, STLAYER_Capability_t * const Capability_p)
{
    S32 DeviceIndex = INVALID_DEVICE_INDEX;
    ST_ErrorCode_t Err = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if ((Capability_p == NULL) ||                       /* Pointer to capability structure should be valid ! */
        (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1) || /* Device name length should be respected */
        (strcmp(&DeviceName[0], "\0")==0)
       )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    EnterCriticalSection();

    /* Check if device already initialised and return error if not so */
    DeviceIndex = GetIndexOfDeviceNamed(DeviceName);
    if (DeviceIndex == INVALID_DEVICE_INDEX)
    {
        /* Device name not found */
        Err = ST_ERROR_UNKNOWN_DEVICE;
    }
    else
    {
        /* Fill capability structure */
        Capability_p->MultipleViewPorts                     = TRUE;
        Capability_p->HorizontalResizing                    = TRUE;
        Capability_p->AlphaBorder                           = FALSE;
        Capability_p->GlobalAlpha                           = TRUE;
        Capability_p->ColorKeying                           = FALSE;
        Capability_p->MultipleViewPortsOnScanLineCapable    = FALSE;
        Capability_p->LayerType                             = STLAYER_COMPOSITOR;
        Capability_p->FrameBufferDisplayLatency             = DeviceArray[DeviceIndex].DisplayParamsForVideo.FrameBufferDisplayLatency;
        Capability_p->FrameBufferHoldTime                   = DeviceArray[DeviceIndex].DisplayParamsForVideo.FrameBufferHoldTime;
    }

    LeaveCriticalSection();

    return(Err);

} /* End of LAYCOMPO_GetCapability() function */


/*
--------------------------------------------------------------------------------
Initialise LAYCOMPO_Init driver
(Standard instances management. Driver specific implementations should be put in Init() function.)
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t LAYCOMPO_Init(const ST_DeviceName_t DeviceName, const STLAYER_InitParams_t * const InitParams_p)
{
    S32 Index = 0;
    ST_ErrorCode_t Err = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if (InitParams_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if ((InitParams_p->MaxHandles > LAYCOMPO_MAX_OPEN) ||     /* No more than MAX_OPEN open handles supported */
        (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1) || /* Device name length should be respected */
          (strcmp(&DeviceName[0], "\0")==0)            /* Device name should not be empty */
       )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }


    EnterCriticalSection();

    /* First time: initialise devices and units as empty */
    if (!FirstInitDone)
    {
        for (Index = 0; Index < LAYCOMPO_MAX_DEVICE; Index++)
        {
            DeviceArray[Index].DeviceName[0] = '\0';
        }

        for (Index = 0; Index < LAYCOMPO_MAX_UNIT; Index++)
        {
            UnitArray[Index].UnitValidity = 0;
        }
        /* Process this only once */
        FirstInitDone = TRUE;
    }

    /* Check if device already initialised and return error if so */
    if (GetIndexOfDeviceNamed(DeviceName) != INVALID_DEVICE_INDEX)
    {
        /* Device name found */
        Err = ST_ERROR_ALREADY_INITIALIZED;
    }
    else
    {
        /* Look for a non-initialised device and return error if none is available */
        Index = 0;
        while ((DeviceArray[Index].DeviceName[0] != '\0') && (Index < LAYCOMPO_MAX_DEVICE))
        {
            Index++;
        }
        if (Index >= LAYCOMPO_MAX_DEVICE)
        {
            /* All devices initialised */
            Err = ST_ERROR_NO_MEMORY;
        }
        else
        {
            /* Semaphore laycompo device access creation */
            DeviceArray[Index].CtrlAccess_p = STOS_SemaphoreCreateFifo(NULL,1);

            /* API specific initialisations */
            Err = Init(&DeviceArray[Index], InitParams_p,DeviceName );

            if (Err == ST_NO_ERROR)
            {
                /* Device available and successfully initialised: register device name */
                strcpy(DeviceArray[Index].DeviceName, DeviceName);
                DeviceArray[Index].DeviceName[ST_MAX_DEVICE_NAME - 1] = '\0';
                DeviceArray[Index].MaxHandles = InitParams_p->MaxHandles;

                STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Device '%s' initialised", DeviceArray[Index].DeviceName));
            }
        } /* End exists device not initialised */
    } /* End device not already initialised */

    LeaveCriticalSection();

    return(Err);
} /* End of LAYCOMPO_Init() function */


/*
--------------------------------------------------------------------------------
open ...
(Standard instances management. Driver specific implementations should be put in Open() function.)
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t LAYCOMPO_Open(const ST_DeviceName_t DeviceName, const STLAYER_OpenParams_t * const OpenParams_p, STLAYER_Handle_t *Handle_p)
{
    S32 DeviceIndex = INVALID_DEVICE_INDEX, UnitIndex, Index;
    U32 OpenedUnitForThisInit;
    ST_ErrorCode_t Err = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if ((OpenParams_p == NULL) ||                       /* There must be parameters ! */
        (Handle_p == NULL) ||                           /* Pointer to handle should be valid ! */
        (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1) || /* Device name length should be respected */
        (strcmp(&DeviceName[0], "\0")==0)
       )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    EnterCriticalSection();

    if (!FirstInitDone)
    {
        Err = ST_ERROR_UNKNOWN_DEVICE;
    }
    else
    {
        /* Check if device already initialised and return error if not so */
        DeviceIndex = GetIndexOfDeviceNamed(DeviceName);
        if (DeviceIndex == INVALID_DEVICE_INDEX)
        {
            /* Device name not found */
            Err = ST_ERROR_UNKNOWN_DEVICE;
        }
        else
        {
            /* Look for a free unit and check all opened units to check if MaxHandles is not reached */
            UnitIndex = LAYCOMPO_MAX_UNIT;
            OpenedUnitForThisInit = 0;
            for (Index = 0; Index < LAYCOMPO_MAX_UNIT; Index++)
            {
                if ((UnitArray[Index].UnitValidity == LAYCOMPO_VALID_UNIT) && (UnitArray[Index].Device_p == &DeviceArray[DeviceIndex]))
                {
                    OpenedUnitForThisInit ++;
                }
                if (UnitArray[Index].UnitValidity != LAYCOMPO_VALID_UNIT)
                {
                    /* Found a free handle structure */
                    UnitIndex = Index;
                }
            }
            if ((OpenedUnitForThisInit >= DeviceArray[DeviceIndex].MaxHandles) || (UnitIndex >= LAYCOMPO_MAX_UNIT))
            {
                /* None of the units is free or MaxHandles reached */
                Err = ST_ERROR_NO_FREE_HANDLES;
            }
            else
            {
                *Handle_p = (STLAYER_Handle_t) &UnitArray[UnitIndex];
                UnitArray[UnitIndex].Device_p = &DeviceArray[DeviceIndex];

                /* API specific actions after opening */
                Err = Open(&UnitArray[UnitIndex], OpenParams_p);

                if (Err == ST_NO_ERROR)
                {
                    /* Register opened handle */
                    UnitArray[UnitIndex].UnitValidity = LAYCOMPO_VALID_UNIT;

                    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Handle opened on device '%s'", DeviceName));
                }
            } /* End found unit unused */
        } /* End device valid */
    } /* End FirstInitDone */

    LeaveCriticalSection();

    return(Err);
} /* End fo LAYCOMPO_Open() function */


/*
--------------------------------------------------------------------------------
Close ...
(Standard instances management. Driver specific implementations should be put in Close() function.)
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t LAYCOMPO_Close(STLAYER_Handle_t Handle)
{
    laycompo_Unit_t *Unit_p;
    ST_ErrorCode_t Err = ST_NO_ERROR;

    EnterCriticalSection();

    if (!(FirstInitDone))
    {
        Err = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        if (!IsValidHandle(Handle))
        {
            Err = ST_ERROR_INVALID_HANDLE;
        }
        else
        {
            Unit_p = (laycompo_Unit_t *) Handle;

            /* API specific actions before closing */
            Err = Close(Unit_p);

            /* Close only if no errors */
            if (Err == ST_NO_ERROR)
            {
                /* Un-register opened handle */
                Unit_p->UnitValidity = 0;

                STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Handle closed on device '%s'", Unit_p->Device_p->DeviceName));
            }
        } /* End handle valid */
    } /* End FirstInitDone */

    LeaveCriticalSection();

    return(Err);
} /* End of LAYCOMPO_Close() function */


/*
--------------------------------------------------------------------------------
Terminate xxxxx driver
(Standard instances management. Driver specific implementations should be put in Term() function.)
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t LAYCOMPO_Term(const ST_DeviceName_t DeviceName, const STLAYER_TermParams_t *const TermParams_p)
{
    laycompo_Unit_t *Unit_p;
    S32 DeviceIndex = INVALID_DEVICE_INDEX, UnitIndex;
    BOOL Found = FALSE;
    ST_ErrorCode_t Err = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if ((TermParams_p == NULL) ||                       /* There must be parameters ! */
        (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1) || /* Device name length should be respected */
        (strcmp(&DeviceName[0], "\0")==0)            /* Device name should not be empty */
       )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    EnterCriticalSection();

    if (!FirstInitDone)
    {
        Err = ST_ERROR_UNKNOWN_DEVICE;
    }
    else
    {
        /* Check if device already initialised and return error if NOT so */
        DeviceIndex = GetIndexOfDeviceNamed(DeviceName);
        if (DeviceIndex == INVALID_DEVICE_INDEX)
        {
            /* Device name not found */
            Err = ST_ERROR_UNKNOWN_DEVICE;
        }
        else
        {
            /* Check if there is still 'open' on this device */
/*            Found = FALSE;*/
            UnitIndex = 0;
            Unit_p = UnitArray;
            while ((UnitIndex < LAYCOMPO_MAX_UNIT) && (!Found))
            {
                Found = ((Unit_p->Device_p == &DeviceArray[DeviceIndex]) && (Unit_p->UnitValidity == LAYCOMPO_VALID_UNIT));
                Unit_p++;
                UnitIndex++;
            }

            if (Found)
            {
                if (TermParams_p->ForceTerminate)
                {
                    UnitIndex = 0;
                    Unit_p = UnitArray;
                    while (UnitIndex < LAYCOMPO_MAX_UNIT)
                    {
                        if ((Unit_p->Device_p == &DeviceArray[DeviceIndex]) && (Unit_p->UnitValidity == LAYCOMPO_VALID_UNIT))
                        {
                            /* Found an open instance: close it ! */
                            Err = Close(Unit_p);
                            if (Err == ST_NO_ERROR)
                            {
                                /* Un-register opened handle */
                                Unit_p->UnitValidity = 0;

                                STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Handle closed on device '%s'", Unit_p->Device_p->DeviceName));
                            }
                            else
                            {
                                /* Problem: this should not happen
                                Fail to terminate ? No because of force */
                                Err = ST_NO_ERROR;
                            }
                        }
                        Unit_p++;
                        UnitIndex++;
                    }
                } /* End ForceTerminate: closed all opened handles */
                else
                {
                    /* Can't term if there are handles still opened, and ForceTerminate not set */
                    Err = ST_ERROR_OPEN_HANDLE;
                }
            } /* End found handle not closed */

            /* Terminate if OK */
            if (Err == ST_NO_ERROR)
            {
                /* API specific terminations */
                Err = Term(&DeviceArray[DeviceIndex], TermParams_p);

                if (Err == ST_NO_ERROR)
                {
                    /* Get semaphore before deleting it */
                    STOS_SemaphoreWait(DeviceArray[DeviceIndex].CtrlAccess_p);

                    /* Free device semaphore */
                    STOS_SemaphoreDelete(NULL,DeviceArray[DeviceIndex].CtrlAccess_p);

                    /* Device found: desallocate memory, free device */
                    DeviceArray[DeviceIndex].DeviceName[0] = '\0';

                    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Device '%s' terminated", DeviceName));
                }
            } /* End terminate OK */
        } /* End valid device */
    } /* End FirstInitDone */

    LeaveCriticalSection();

    return(Err);
} /* End of LAYCOMPO_Term() function */


/* End of lc_init.c */
