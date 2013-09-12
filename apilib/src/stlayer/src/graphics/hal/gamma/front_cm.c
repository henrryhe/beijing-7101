/*******************************************************************************

File name   : front_cm.c

Description : API functions

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                               Name
----               ------------                               ----
2000-05-30         Created                                    Michel Bruant
                                                              Adriano Melis

    Common functions
    -----------------
    LAYERGFX_GetCapability
    LAYERGFX_GetInitAllocParams
    LAYERGFX_Init
    LAYERGFX_Term
    LAYERGFX_Open
    LAYERGFX_Close
    LAYERGFX_GetLayerParams
    LAYERGFX_SetLayerParams
    LAYERGFX_UpdateFromMixer
    LAYERGFX_GetVTGParams

 ******************************************************************************/

/* Includes ----------------------------------------------------------------- */

#ifndef ST_OSLINUX
    #include <string.h>
 #endif

#include "layerapi.h"
#define VARIABLE_STLAYER_RESERVATION
#include "filters.h"
#include "hard_mng.h"
#include "frontend.h"
#include "back_end.h"

#undef VARIABLE_STLAYER_RESERVATION
#include "stavmem.h"

#ifdef ST_OSLINUX
    #include "stlayer_core.h"
#else
    #include "string.h"
#endif

#include "stsys.h"
#include "stdevice.h"

#ifdef ST_OSLINUX
extern STAVMEM_PartitionHandle_t STAVMEM_GetPartitionHandle( U32 PartitionID );
STAVMEM_PartitionHandle_t PartitionHandle;
#endif

/* Macros ------------------------------------------------------------------- */

#define DeviceAdd(Add) ((U32)Add \
           + (U32)LayerContext->VirtualMapping.PhysicalAddressSeenFromDevice_p \
           - (U32)LayerContext->VirtualMapping.PhysicalAddressSeenFromCPU_p)


/* Constants ---------------------------------------------------------------- */

#define INVALID_DEVICE_INDEX (0xFFFF)


/* Private Function prototypes ---------------------------------------------- */

static U32 IndexOnNamedLayer(const ST_DeviceName_t DeviceName);
static void InitLayerContext(const STLAYER_InitParams_t *const ,
        const ST_DeviceName_t, U32 );
static void EnterCriticalSection(void);
static void LeaveCriticalSection(void);
static ST_ErrorCode_t InitLayerPool(const STLAYER_InitParams_t* const Params_p,
                          stlayer_XLayerContext_t * LayerContext);
static ST_ErrorCode_t ReleaseLayerPool(STLAYER_Handle_t  Handle );
#ifdef ST_OSLINUX
static ST_ErrorCode_t InitLayerModule(const STLAYER_InitParams_t* const Params_p,
                                            stlayer_XLayerContext_t * LayerContext_p);
#endif /* ST_OSLINUX */

/* Private Variables (static)------------------------------------------------ */
/* Init/Open/Close/Term protection semaphore */
static semaphore_t * InterInstancesLockingSemaphore_p = NULL;

#if defined(ST_OS21)
FILE * dumpstream;
#endif

/* Functions ---------------------------------------------------------------- */


/*******************************************************************************
Name        : LAYERGFX_GetCapability
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYERGFX_GetCapability(const ST_DeviceName_t DeviceName,
                                     STLAYER_Capability_t * const Capability_p)
{
    S32 DeviceIndex ;
    ST_ErrorCode_t Err = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if ((Capability_p == NULL)
         ||  (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1) )
    {
        layergfx_Defense(ST_ERROR_BAD_PARAMETER);
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Check if device already initialised and return error if not so */
    DeviceIndex = IndexOnNamedLayer(DeviceName);
    if (DeviceIndex == INVALID_DEVICE_INDEX)
    {
        /* Device name not found */
        layergfx_Defense(ST_ERROR_UNKNOWN_DEVICE);
        return(ST_ERROR_UNKNOWN_DEVICE);
    }

    /* Fill capability structure */
    Capability_p->DeviceId = stlayer_context.XContext[DeviceIndex].DeviceId;
    switch(stlayer_context.XContext[DeviceIndex].LayerType)
    {
        case STLAYER_GAMMA_CURSOR:
            Capability_p->MultipleViewPorts                     = FALSE;
            Capability_p->HorizontalResizing                    = FALSE;
            Capability_p->AlphaBorder                           = FALSE;
            Capability_p->GlobalAlpha                           = FALSE;
            Capability_p->ColorKeying                           = FALSE;
            Capability_p->MultipleViewPortsOnScanLineCapable    = FALSE;

            Capability_p->LayerType                     = STLAYER_GAMMA_CURSOR;
            Err = ST_NO_ERROR;
            break;
        case STLAYER_GAMMA_GDP:

            Capability_p->MultipleViewPorts                     = TRUE;
            Capability_p->HorizontalResizing                    = TRUE;
            Capability_p->AlphaBorder                           = TRUE;
            Capability_p->GlobalAlpha                           = TRUE;
            Capability_p->ColorKeying                           = TRUE;
            Capability_p->MultipleViewPortsOnScanLineCapable    = FALSE;
            Capability_p->LayerType                     = STLAYER_GAMMA_GDP;
            /* To initialize correctly the Counters[Layer].CounterInDisplay, */
            /* in the video function PutNewPictureOnDisplayForNextVSync() */
            Capability_p->FrameBufferHoldTime           = 1;
            Capability_p->FrameBufferDisplayLatency     = 1;

            Err = ST_NO_ERROR;
            break;
        case STLAYER_GAMMA_GDPVBI:
            Capability_p->MultipleViewPorts                     = TRUE;
            Capability_p->HorizontalResizing                    = TRUE;
            Capability_p->AlphaBorder                           = TRUE;
            Capability_p->GlobalAlpha                           = TRUE;
            Capability_p->ColorKeying                           = TRUE;
            Capability_p->MultipleViewPortsOnScanLineCapable    = FALSE;
            Capability_p->LayerType                     = STLAYER_GAMMA_GDPVBI;
            Err = ST_NO_ERROR;
            break;


        case STLAYER_GAMMA_FILTER:
            Capability_p->MultipleViewPorts                     = TRUE;
            Capability_p->HorizontalResizing                    = TRUE;
            Capability_p->AlphaBorder                           = TRUE;
            Capability_p->GlobalAlpha                           = TRUE;
            Capability_p->ColorKeying                           = TRUE;
            Capability_p->MultipleViewPortsOnScanLineCapable    = FALSE;
            Capability_p->LayerType                     = STLAYER_GAMMA_FILTER;
            Err = ST_NO_ERROR;
            break;
        case STLAYER_GAMMA_ALPHA:
            Capability_p->MultipleViewPorts                     = TRUE;
            Capability_p->HorizontalResizing                    = TRUE;
            Capability_p->AlphaBorder                           = TRUE;
            Capability_p->GlobalAlpha                           = TRUE;
            Capability_p->ColorKeying                           = TRUE;
            Capability_p->MultipleViewPortsOnScanLineCapable    = FALSE;
            Capability_p->LayerType                     = STLAYER_GAMMA_ALPHA;
            Err = ST_NO_ERROR;
            break;
        case STLAYER_GAMMA_BKL:
            Capability_p->MultipleViewPorts                     = TRUE;
            Capability_p->HorizontalResizing                    = TRUE;
            Capability_p->AlphaBorder                           = TRUE;
            Capability_p->GlobalAlpha                           = TRUE;
            Capability_p->ColorKeying                           = TRUE;
            Capability_p->MultipleViewPortsOnScanLineCapable    = FALSE;
            Capability_p->LayerType                     = STLAYER_GAMMA_BKL;
            Err = ST_NO_ERROR;
            break;
        default:
            layergfx_Defense(ST_ERROR_FEATURE_NOT_SUPPORTED);
            Err = ST_ERROR_FEATURE_NOT_SUPPORTED;
            break;
    }
    return(Err);

}
/*******************************************************************************
name        : LAYERGFX_GetInitAllocParams
description :
parameters  :
assumptions :
limitations :
returns     :
*******************************************************************************/
ST_ErrorCode_t LAYERGFX_GetInitAllocParams(STLAYER_Layer_t LayerType,
                                          U32             ViewPortsNumber,
                                          STLAYER_AllocParams_t * params)
{
    if(LayerType == STLAYER_GAMMA_CURSOR)
    {
        params->ViewPortNodesInSharedMemory   = TRUE;
        params->ViewPortNodesBufferAlignment  = 0;
        params->ViewPortDescriptorsBufferSize =(sizeof(stlayer_ViewPortHandle_t)
                + sizeof(stlayer_ViewPortDescriptor_t));
        params->ViewPortNodesBufferSize       = 0;
    }
    else
    {
        params->ViewPortNodesInSharedMemory   = TRUE;
        params->ViewPortNodesBufferAlignment  = 16;
        params->ViewPortDescriptorsBufferSize = ViewPortsNumber
                                        * (sizeof(stlayer_ViewPortHandle_t)
                                        + sizeof(stlayer_ViewPortDescriptor_t));
        params->ViewPortNodesBufferSize       = 2 * (ViewPortsNumber + 1)
                                              * sizeof(stlayer_Node_t);
    }
    return(ST_NO_ERROR);

}

/*******************************************************************************
Name        : LAYERGFX_Init
Description :Init a Layer
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYERGFX_Init(const ST_DeviceName_t        DeviceName,
                             const STLAYER_InitParams_t * const InitParams_p)
{
    ST_ErrorCode_t  Error;
    U32             i;
    ST_DeviceName_t Empty = "\0\0";

#if defined(ST_OS21)
#ifdef DUMP_GDPS_NODES

    if(stlayer_context.NumLayers==0)
            {
      /* Open stream */
    printf("\nSaving Dump file ...\n");
    dumpstream = fopen("GDP_DUMP.txt", "wb");

        fprintf(dumpstream,"---------------------------------------- \n");
        fprintf(dumpstream,"----  Compositor GDP node generation ----\n");
        fprintf(dumpstream,"---------------------------------------- \n");
            }
#endif
#endif

    EnterCriticalSection();
    /* Exit now if parameters are bad */
    if ((InitParams_p == NULL) ||
        (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1))
    {
        LeaveCriticalSection();
        layergfx_Defense(ST_ERROR_BAD_PARAMETER);
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* first init only */
    if(!stlayer_context.FirstInitDone)
    {
        /* Init of the general context */
        stlayer_context.FirstInitDone   = TRUE;
        stlayer_context.NumLayers       = 0;
        stlayer_context.NumBKLGDPLayers = 0;
        stlayer_context.NumCursLayers   = 0;
        stlayer_context.NumOtherLayers  = 0;
        for(i=0; i<MAX_LAYER; i++)
        {
            stlayer_context.XContext[i].LayerType                   = 0;
            stlayer_context.XContext[i].Open                        = 0;
            stlayer_context.XContext[i].Initialised                 = 0;
            stlayer_context.XContext[i].VPDescriptorsUserAllocated  = 0;
            stlayer_context.XContext[i].VPNodesUserAllocated        = 0;
            stlayer_context.XContext[i].HandleArray                 = 0;
            stlayer_context.XContext[i].ListViewPort                = 0;
            stlayer_context.XContext[i].NodeArray                   = 0;
            stlayer_context.XContext[i].LinkedViewPorts             = 0;
            stlayer_context.XContext[i].NumberLinkedViewPorts       = 0;
            stlayer_context.XContext[i].CPUPartition_p              = 0;
            stlayer_context.XContext[i].BaseAddress                 = 0;
#ifdef ST_OSLINUX
            stlayer_context.XContext[i].MappedGammaBaseAddress_p    = 0;
            stlayer_context.XContext[i].MappedGammaWidth            = 0;
#endif
            stlayer_context.XContext[i].AVMEM_Partition             = 0;
            stlayer_context.XContext[i].EvtHandle                   = 0;
        }
    }
#ifdef ST_OSLINUX
    PartitionHandle = STAVMEM_GetPartitionHandle( LAYER_PARTITION_AVMEM );
#endif


    /* then all inits */
    /* Check if device already initialised and return error if so */
    if (IndexOnNamedLayer(DeviceName) != INVALID_DEVICE_INDEX)
    {
        /* Device name found */
        LeaveCriticalSection();
        layergfx_Defense(ST_ERROR_ALREADY_INITIALIZED);
        return(ST_ERROR_ALREADY_INITIALIZED);
    }
    /* Find a free index */
    i = 0;
    while ((stlayer_context.XContext[i].Initialised == TRUE )
            && (i < MAX_LAYER))
    {
        i++;
    }
    if (i >= MAX_LAYER)
    {
        /* All devices initialised */
        LeaveCriticalSection();
        layergfx_Defense(ST_ERROR_NO_MEMORY);
        return(ST_ERROR_NO_MEMORY);
    }
    InitLayerContext(InitParams_p,DeviceName,i);
#ifdef ST_OSLINUX
    Error = InitLayerModule(InitParams_p, &(stlayer_context.XContext[i]));
    if (Error != ST_NO_ERROR)
	{
        strcpy(stlayer_context.XContext[i].DeviceName,Empty);
        layergfx_Defense(Error);
        LeaveCriticalSection();
        return(Error);
    }

#endif
	Error = InitLayerPool(InitParams_p,&(stlayer_context.XContext[i]));
    if (Error != ST_NO_ERROR)
    {
        strncpy(stlayer_context.XContext[i].DeviceName,Empty,sizeof(ST_DeviceName_t)-1);
        layergfx_Defense(Error);
        LeaveCriticalSection();
        return(Error);
    }

    Error = stlayer_HardwareManagerInit(&(stlayer_context.XContext[i]));
    if (Error != ST_NO_ERROR)
    {
        ReleaseLayerPool(i);
        strncpy(stlayer_context.XContext[i].DeviceName,Empty,sizeof(ST_DeviceName_t)-1);
        layergfx_Defense(Error);
        LeaveCriticalSection();
        return(Error);
    }

    switch (InitParams_p->LayerType)
    {
        case STLAYER_GAMMA_GDP:
        case STLAYER_GAMMA_GDPVBI:
        case STLAYER_GAMMA_BKL:
        case STLAYER_GAMMA_ALPHA:
        case STLAYER_GAMMA_FILTER:
            stlayer_context.NumBKLGDPLayers ++;
            if (stlayer_context.NumBKLGDPLayers > MAX_GDP_BKL)
            {
                ReleaseLayerPool(i);
                stlayer_HardwareManagerTerm(&(stlayer_context.XContext[i]));
                stlayer_context.NumBKLGDPLayers --;
                strncpy(stlayer_context.XContext[i].DeviceName, Empty, sizeof(ST_DeviceName_t)-1);
                layergfx_Defense(ST_ERROR_NO_FREE_HANDLES);
                LeaveCriticalSection();
                return(ST_ERROR_NO_FREE_HANDLES); /* HW limitation */
            }
        break;

        case STLAYER_GAMMA_CURSOR:
            stlayer_context.NumCursLayers ++;
            if (stlayer_context.NumCursLayers > MAX_CURS)
            {
                ReleaseLayerPool(i);
                stlayer_HardwareManagerTerm(&(stlayer_context.XContext[i]));
                stlayer_context.NumCursLayers --;
                strncpy(stlayer_context.XContext[i].DeviceName,Empty, sizeof(ST_DeviceName_t)-1);
                layergfx_Defense(ST_ERROR_NO_FREE_HANDLES);
                LeaveCriticalSection();
                return(ST_ERROR_NO_FREE_HANDLES); /* HW limitation */
            }
            if(InitParams_p->MaxViewPorts != 1)
            {
                ReleaseLayerPool(i);
                stlayer_HardwareManagerTerm(&(stlayer_context.XContext[i]));
                stlayer_context.NumCursLayers --;
                strncpy(stlayer_context.XContext[i].DeviceName, Empty, sizeof(ST_DeviceName_t)-1);
                layergfx_Defense(ST_ERROR_FEATURE_NOT_SUPPORTED);
                LeaveCriticalSection();
                return(ST_ERROR_FEATURE_NOT_SUPPORTED); /* HW limitation */
            }
        break;

        default:
        break;
    }
    stlayer_context.NumLayers ++;
    stlayer_context.XContext[i].Initialised = TRUE;
    LeaveCriticalSection();
    layergfx_Trace(( "STLAYER - GFX : Layer init OK" ));
    return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : LAYERGFX_Term
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYERGFX_Term(const ST_DeviceName_t        DeviceName,
                            const STLAYER_TermParams_t * const TermParams_p)
{
	U32             LayerIndex;
    ST_DeviceName_t Empty = "\0\0";
    ST_ErrorCode_t  Error;


#ifdef DUMP_GDPS_NODES
if(stlayer_context.NumLayers==1)
        {
        fclose (dumpstream);
    printf(("Dump file saved.\n"));
        }
#endif

    EnterCriticalSection();
    if(TermParams_p == 0)
    {
        LeaveCriticalSection();
        layergfx_Defense(ST_ERROR_BAD_PARAMETER);
        return(ST_ERROR_BAD_PARAMETER); /* error */
    }
    LayerIndex = IndexOnNamedLayer(DeviceName);
    if (LayerIndex == INVALID_DEVICE_INDEX)
    {
        /* Device name not found */
        LeaveCriticalSection();
        layergfx_Defense(ST_ERROR_UNKNOWN_DEVICE);
        return(ST_ERROR_UNKNOWN_DEVICE);
    }
    if (stlayer_context.XContext[LayerIndex].Initialised == FALSE)
    {
        LeaveCriticalSection();
        layergfx_Defense(ST_ERROR_UNKNOWN_DEVICE);
        return(ST_ERROR_UNKNOWN_DEVICE); /* error not init */
    }
    if ((stlayer_context.XContext[LayerIndex].Open > 0)
            &&(TermParams_p->ForceTerminate == FALSE))
    {
        LeaveCriticalSection();
        layergfx_Defense(ST_ERROR_BAD_PARAMETER);
        return(ST_ERROR_BAD_PARAMETER); /* error still opened */
    }
    if((stlayer_context.XContext[LayerIndex].NumberLinkedViewPorts != 0)
            &&(TermParams_p->ForceTerminate == FALSE))
    {
        LeaveCriticalSection();
        layergfx_Defense(ST_ERROR_BAD_PARAMETER);
        return(ST_ERROR_BAD_PARAMETER); /* error */
    }

    /* At this point; we can term the layer */
    /* Disable display */
    while (stlayer_context.XContext[LayerIndex].NumberLinkedViewPorts > 0)
    {
        /* force-open the layer : to be abble to disable the VPs */
        stlayer_context.XContext[LayerIndex].Open ++;
        LAYERGFX_DisableViewPort((STLAYER_ViewPortHandle_t)
                stlayer_context.XContext[LayerIndex].LinkedViewPorts);
    }
    /* Close all the sessions */
    while (stlayer_context.XContext[LayerIndex].Open > 0)
    {
        /* Close it */
        LeaveCriticalSection();
        LAYERGFX_Close(LayerIndex);
        EnterCriticalSection();
    }

    switch (stlayer_context.XContext[LayerIndex].LayerType)
    {
        case STLAYER_GAMMA_GDP:
        case STLAYER_GAMMA_GDPVBI:
        case STLAYER_GAMMA_BKL:
        case STLAYER_GAMMA_ALPHA:
        case STLAYER_GAMMA_FILTER:
            stlayer_context.NumBKLGDPLayers --;
        break;

        case STLAYER_GAMMA_CURSOR:
            stlayer_context.NumCursLayers --;
        break;

        default:
        break;
    }

#ifdef ST_OSLINUX
/* releasing LAYER GAMMA linux device driver */
    /** LAYER GAMMA region **/
    STLINUX_UnmapRegion( (void*)stlayer_context.XContext[LayerIndex].MappedGammaBaseAddress_p, (U32)stlayer_context.XContext[LayerIndex].MappedGammaWidth);




#endif
    Error = stlayer_HardwareManagerTerm(&stlayer_context.XContext[LayerIndex]);
    if(Error != ST_NO_ERROR)
    {
        LeaveCriticalSection();
        layergfx_Defense(Error);
        return(Error);
    }
    Error = ReleaseLayerPool(LayerIndex);
    if(Error != ST_NO_ERROR)
    {
        LeaveCriticalSection();
        layergfx_Defense(Error);
        return(Error);
    }


    stlayer_context.XContext[LayerIndex].Initialised = FALSE;
    strncpy(stlayer_context.XContext[LayerIndex].DeviceName,Empty,sizeof(ST_DeviceName_t)-1);
    stlayer_context.NumLayers --;
    LeaveCriticalSection();
    layergfx_Trace(( "STLAYER - GFX : Layer term OK" ));
    return(ST_NO_ERROR);

}

/*******************************************************************************
Name        : LAYERGFX_Open
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYERGFX_Open(const ST_DeviceName_t        DeviceName,
                            const STLAYER_OpenParams_t * const Params,
                            STLAYER_Handle_t *     Handle)
{
    U32 LayerIndex;

    if ((Params == NULL) ||
         (Handle == NULL) ||
         (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1))
    {
        layergfx_Defense(ST_ERROR_BAD_PARAMETER);
        return(ST_ERROR_BAD_PARAMETER);
    }

    EnterCriticalSection();

    LayerIndex = IndexOnNamedLayer(DeviceName);
    if (LayerIndex == INVALID_DEVICE_INDEX)
    {
        /* Device name not found */
        LeaveCriticalSection();
        layergfx_Defense(ST_ERROR_UNKNOWN_DEVICE);
        return(ST_ERROR_UNKNOWN_DEVICE);
    }
    if (stlayer_context.XContext[LayerIndex].Initialised == FALSE)
    {
        LeaveCriticalSection();
        layergfx_Defense(ST_ERROR_BAD_PARAMETER);
        return(ST_ERROR_BAD_PARAMETER); /* error not init */
    }

    stlayer_context.XContext[LayerIndex].Open ++;
    * Handle = LayerIndex;

    LeaveCriticalSection();
    layergfx_Trace(( "STLAYER - GFX : Layer open OK" ));
    return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : LAYERGFX_Close
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYERGFX_Close(STLAYER_Handle_t  Handle)
{
    U32 LayerIndex;

    EnterCriticalSection();
    if (!stlayer_context.FirstInitDone)
    {
        LeaveCriticalSection();
        layergfx_Defense(ST_ERROR_UNKNOWN_DEVICE);
        return(ST_ERROR_UNKNOWN_DEVICE);
    }
    LayerIndex = Handle;
    if ((LayerIndex == INVALID_DEVICE_INDEX)
            || (LayerIndex >= MAX_LAYER))
    {
        /* Device name not found */
        LeaveCriticalSection();
        layergfx_Defense(ST_ERROR_UNKNOWN_DEVICE);
        return(ST_ERROR_UNKNOWN_DEVICE);
    }
    if (stlayer_context.XContext[LayerIndex].Open == 0)
    {
        LeaveCriticalSection();
        layergfx_Defense(ST_ERROR_BAD_PARAMETER);
        return(ST_ERROR_BAD_PARAMETER); /* error already closed */
    }
    stlayer_context.XContext[LayerIndex].Open -- ;

    LeaveCriticalSection();
    layergfx_Trace(( "STLAYER - GFX : Layer close OK" ));
    return(ST_NO_ERROR);
}
/*******************************************************************************
Name        : LAYERGFX_SetLayerParams
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYERGFX_SetLayerParams(STLAYER_Handle_t  Handle,
        STLAYER_LayerParams_t *  LayerParams_p)
{
    STLAYER_ViewPortHandle_t        CurrentVP;
    STLAYER_ViewPortSource_t        Source;
    STGXOBJ_Palette_t               Palette;
    STGXOBJ_Bitmap_t                Bitmap;
    U32                             i;

    if(stlayer_context.XContext[Handle].Initialised == FALSE)
    {
        layergfx_Defense(ST_ERROR_INVALID_HANDLE);
        return(ST_ERROR_INVALID_HANDLE);
    }
    if(LayerParams_p == 0)
    {
        layergfx_Defense(ST_ERROR_BAD_PARAMETER);
        return(ST_ERROR_BAD_PARAMETER);
    }
    EnterCriticalSection();
    if((LayerParams_p->Width
                > stlayer_context.XContext[Handle].OutputParams.Width)
        ||(LayerParams_p->Height
                > stlayer_context.XContext[Handle].OutputParams.Height))
    {
        /* Layer overflow the output */
        LeaveCriticalSection();
        layergfx_Defense(ST_ERROR_BAD_PARAMETER);
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* set the new params */
    STOS_memcpy(&stlayer_context.XContext[Handle].LayerParams, LayerParams_p, sizeof(STLAYER_LayerParams_t));
    /* Update the node according to the new scan mode */
    /* and recompute the clipping                     */
    /* Affect the pointers for Source */
    Source.Palette_p    = &Palette;
    Source.Data.BitMap_p = &Bitmap;
    for (i=0; i<stlayer_context.XContext[Handle].MaxViewPorts; i++ )
    {
        CurrentVP =(STLAYER_ViewPortHandle_t)
           &((stlayer_context.XContext[Handle].ListViewPort[i]));
        if(((stlayer_ViewPortDescriptor_t*)CurrentVP)->Enabled)
        {
            /* this is a way to do */
            LAYERGFX_GetViewPortSource(CurrentVP,&Source);
            LAYERGFX_SetViewPortSource(CurrentVP,&Source);
        }
    }

    LeaveCriticalSection();
    return(ST_NO_ERROR);
}
/*******************************************************************************
Name        :  LAYERGFX_UpdateFromMixer
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYERGFX_UpdateFromMixer(STLAYER_Handle_t        Handle,
                                       STLAYER_OutputParams_t * OutputParams_p)
{
    stlayer_ViewPortDescriptor_t *  CurrentVP;
    ST_ErrorCode_t                  Error;
    STLAYER_UpdateParams_t          UpdateParams;

    if(stlayer_context.XContext[Handle].Initialised == FALSE)
    {
        layergfx_Defense(ST_ERROR_INVALID_HANDLE);
        return(ST_ERROR_INVALID_HANDLE);
    }
    UpdateParams.UpdateReason = 0;
    EnterCriticalSection();
    STOS_SemaphoreWait(stlayer_context.XContext[Handle].AccessProtect_p);

    /* Reason is : disconnect the layer from its VTG */
    if ((OutputParams_p->UpdateReason & STLAYER_DISCONNECT_REASON) != 0)
    {
        /* unsubscribe to the handled VTG_VSYNC event */
        stlayer_VTGSynchro(&stlayer_context.XContext[Handle],NULL);
        UpdateParams.UpdateReason |= STLAYER_DISCONNECT_REASON;
    }

    /* Reason is : the screen params have been changed */
    if ((OutputParams_p->UpdateReason & STLAYER_SCREEN_PARAMS_REASON) != 0)
    {
        /* for this version : layer scan type must be */
        /* equal to output scan type (hw constraint)  */
        if(OutputParams_p->ScanType !=
                stlayer_context.XContext[Handle].LayerParams.ScanType)
        {
            LeaveCriticalSection();
            STOS_SemaphoreSignal(stlayer_context.XContext[Handle].AccessProtect_p);
            layergfx_Defense(ST_ERROR_FEATURE_NOT_SUPPORTED);
            return (ST_ERROR_FEATURE_NOT_SUPPORTED);
        }
        if(OutputParams_p->Width
                < stlayer_context.XContext[Handle].LayerParams.Width)
        {
            LeaveCriticalSection();
            STOS_SemaphoreSignal(stlayer_context.XContext[Handle].AccessProtect_p);
            layergfx_Trace(( "LAYER - Gamma : Output Width is too small." ));
            return(ST_ERROR_BAD_PARAMETER);
        }
        if(OutputParams_p->Height
                < stlayer_context.XContext[Handle].LayerParams.Height)
        {
            LeaveCriticalSection();
            STOS_SemaphoreSignal(stlayer_context.XContext[Handle].AccessProtect_p);
            layergfx_Trace(( "LAYER - Gamma : Output Height is too small." ));
            return(ST_ERROR_BAD_PARAMETER);
        }
        stlayer_context.XContext[Handle].OutputParams.Width
                                                    = OutputParams_p->Width;
        stlayer_context.XContext[Handle].OutputParams.Height
                                                    = OutputParams_p->Height;
        stlayer_context.XContext[Handle].OutputParams.XStart
                                                    = OutputParams_p->XStart;
        stlayer_context.XContext[Handle].OutputParams.YStart
                                                    = OutputParams_p->YStart;
        stlayer_context.XContext[Handle].OutputParams.FrameRate
                                                    = OutputParams_p->FrameRate;
        strncpy(UpdateParams.VTG_Name, stlayer_context.XContext[Handle].VTGName, sizeof(ST_DeviceName_t)-1);
        strncpy(UpdateParams.VTGParams.VTGName, stlayer_context.XContext[Handle].VTGName, sizeof(ST_DeviceName_t)-1);
        UpdateParams.VTGParams.VTGFrameRate = OutputParams_p->FrameRate;
        UpdateParams.UpdateReason |= STLAYER_VTG_REASON;
    }
    /* Reason is : the screen offset have been changed */
    if ((OutputParams_p->UpdateReason & STLAYER_OFFSET_REASON) != 0)
    {
        stlayer_context.XContext[Handle].OutputParams.XOffset
                                                    = OutputParams_p->XOffset;
        stlayer_context.XContext[Handle].OutputParams.YOffset
                                                    = OutputParams_p->YOffset;
    }
    /* Reason is : the layer is connected with a new VTG */
    if ((OutputParams_p->UpdateReason & STLAYER_VTG_REASON) != 0)
    {
        Error = stlayer_VTGSynchro(&stlayer_context.XContext[Handle],
                            OutputParams_p->VTGName);
        if (Error != ST_NO_ERROR)
        {
            LeaveCriticalSection();
            STOS_SemaphoreSignal(stlayer_context.XContext[Handle].AccessProtect_p);
            layergfx_Defense(Error);
            return (Error);
        }
        strncpy(UpdateParams.VTG_Name, OutputParams_p->VTGName, sizeof(ST_DeviceName_t)-1);
        strncpy(UpdateParams.VTGParams.VTGName,OutputParams_p->VTGName, sizeof(ST_DeviceName_t)-1);
        UpdateParams.UpdateReason |= STLAYER_VTG_REASON;
    }
    /* Reason is the pipeline adress need to be changed */
    if ((OutputParams_p->UpdateReason & STLAYER_CHANGE_ID_REASON) != 0)
    {
        stlayer_RemapDevice(Handle, OutputParams_p->DeviceId);
    }

    /* dynamic output change */
    if(stlayer_context.XContext[Handle].NumberLinkedViewPorts != 0)
    {
        CurrentVP = stlayer_context.XContext[Handle].LinkedViewPorts;
        do
        {
            /* Update the node according to the new scan mode */
            stlayer_UpdateNodeGeneric(CurrentVP);
            CurrentVP = CurrentVP->Next_p;
        } while (CurrentVP != stlayer_context.XContext[Handle].LinkedViewPorts);
    }
    else if ((stlayer_context.XContext[Handle].LayerType
                             == STLAYER_GAMMA_CURSOR)
            && (stlayer_context.XContext[Handle].LinkedViewPorts != 0))
    {
        stlayer_UpdateNodeGeneric(stlayer_context.XContext[Handle].
                                LinkedViewPorts);
    }

    LeaveCriticalSection();
    STOS_SemaphoreSignal(stlayer_context.XContext[Handle].AccessProtect_p);
    /* Notify update event if required */
    if (UpdateParams.UpdateReason != 0)
    {
        STEVT_Notify(stlayer_context.XContext[Handle].EvtHandle,
                     stlayer_context.XContext[Handle].UpdateParamEvtIdent,
                     (const void *) (&UpdateParams));
    }
    return(ST_NO_ERROR);
}

/*******************************************************************************
Name        :  LAYERGFX_GetVTGParams
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYERGFX_GetVTGParams(STLAYER_Handle_t           Handle,
                                    STLAYER_VTGParams_t * const VTGParams_p)
{
    if(stlayer_context.XContext[Handle].Initialised == FALSE)
    {
        layergfx_Defense(ST_ERROR_INVALID_HANDLE);
        return(ST_ERROR_INVALID_HANDLE);
    }
    if(VTGParams_p == NULL)
    {
        layergfx_Defense(ST_ERROR_BAD_PARAMETER);
        return(ST_ERROR_BAD_PARAMETER);
    }
    strncpy(VTGParams_p->VTGName,stlayer_context.XContext[Handle].VTGName, sizeof(ST_DeviceName_t)-1);
    VTGParams_p->VTGFrameRate =
                    stlayer_context.XContext[Handle].OutputParams.FrameRate;
    return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : LAYERGFX_GetLayerParams
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYERGFX_GetLayerParams(STLAYER_Handle_t  Handle,
        STLAYER_LayerParams_t *  LayerParams_p)
{
    if(stlayer_context.XContext[Handle].Initialised == FALSE)
    {
        layergfx_Defense(ST_ERROR_INVALID_HANDLE);
        return(ST_ERROR_INVALID_HANDLE);
    }
    if(LayerParams_p == 0)
    {
        layergfx_Defense(ST_ERROR_BAD_PARAMETER);
        return(ST_ERROR_BAD_PARAMETER);
    }
    EnterCriticalSection();
    *LayerParams_p = stlayer_context.XContext[Handle].LayerParams;
    LeaveCriticalSection();
    return(ST_NO_ERROR);
}


/*******************************************************************************

  Private functions
  -----------------
*******************************************************************************/

/*******************************************************************************
Name        : IndexOnNamedLayer
Description : Private function
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static U32 IndexOnNamedLayer(const ST_DeviceName_t DeviceName)
{
    U32 WantedIndex = INVALID_DEVICE_INDEX;
    U32 Index       = 0;

    do
    {
        /* Device initialised: check if name is matching */
        if (strcmp(stlayer_context.XContext[Index].DeviceName,
                    DeviceName ) == 0)
        {
            /* Name found in the initialised devices */
            WantedIndex = Index;
        }
        Index++;
    } while ((WantedIndex == INVALID_DEVICE_INDEX)
              && (Index < MAX_LAYER));

    return(WantedIndex);
}

/*******************************************************************************
Name        : InitLayerContext
Description : Private function called by STLAYER_Init.
              Init a context for a layer and fill its parameters
              according to the init params.
Parameters  : InitParams_p : IN, DeviceName : IN
Assumptions :
Limitations : !! do not init the data pool used by the layer.
Returns     : void
*******************************************************************************/
static void InitLayerContext(const STLAYER_InitParams_t * const InitParams_p,
                  const ST_DeviceName_t DeviceName, U32 i)
{
    ST_DeviceName_t Empty = "\0\0";
    stlayer_context.XContext[i].Open                                    = 0;
    stlayer_context.XContext[i].ListViewPort                            = 0;
    stlayer_context.XContext[i].HandleArray                             = 0;
    stlayer_context.XContext[i].NodeArray                               = 0;
    stlayer_context.XContext[i].NumberLinkedViewPorts                   = 0;
    stlayer_context.XContext[i].LinkedViewPorts                         = 0;
    stlayer_context.XContext[i].LayerType       = InitParams_p->LayerType;
    stlayer_context.XContext[i].CPUPartition_p  = InitParams_p->CPUPartition_p;
    stlayer_context.XContext[i].BaseAddress     = (void * )
                                      ((U32)InitParams_p->BaseAddress_p
                                     + (U32)InitParams_p->DeviceBaseAddress_p);

#ifdef ST_OSLINUX
    stlayer_context.XContext[i].AVMEM_Partition = PartitionHandle;
#else
    stlayer_context.XContext[i].AVMEM_Partition = InitParams_p->AVMEM_Partition;
#endif
    STOS_memcpy(&stlayer_context.XContext[i].LayerParams, InitParams_p->LayerParams_p, sizeof(STLAYER_LayerParams_t));
    strncpy(stlayer_context.XContext[i].DeviceName,DeviceName, sizeof(ST_DeviceName_t)-1);
    strncpy(stlayer_context.XContext[i].EvtHandlerName,
            InitParams_p->EventHandlerName, sizeof(ST_DeviceName_t)-1);
    strncpy(stlayer_context.XContext[i].VTGName,Empty, sizeof(ST_DeviceName_t)-1);
    stlayer_context.XContext[i].DeviceId        = ((U32)InitParams_p
                                            ->BaseAddress_p);

    stlayer_context.XContext[i].AccessProtect_p = STOS_SemaphoreCreateFifo(NULL,1);

    /* Default output : no limitation */
    stlayer_context.XContext[i].OutputParams.Width
                                         = InitParams_p->LayerParams_p->Width;
    stlayer_context.XContext[i].OutputParams.Height
                                         = InitParams_p->LayerParams_p->Height;
    stlayer_context.XContext[i].OutputParams.XStart  = 200;
    stlayer_context.XContext[i].OutputParams.YStart  = 20;
    stlayer_context.XContext[i].OutputParams.XOffset = 0;
    stlayer_context.XContext[i].OutputParams.YOffset = 0;
    stlayer_context.XContext[i].OutputParams.FrameRate = 50000;

}

#ifdef ST_OSLINUX
/*******************************************************************************
Name        : InitLayerModule
Description : Mapping LAYER Registers for Compositor
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t InitLayerModule(const STLAYER_InitParams_t* const Params_p,
                                            stlayer_XLayerContext_t * LayerContext_p)
{
    ST_ErrorCode_t         ErrorCode = ST_NO_ERROR;
#ifdef ST_OSLINUX
    STLAYERMod_Param_t     LayerModuleParam;           /* layer modules parameters */
#endif

    /* base address of Layer mapping */
    LayerModuleParam.LayerBaseAddress = (unsigned long) LAYER_COMPOSITOR_MAPPING;
    LayerModuleParam.LayerAddressWidth = (unsigned long)STLAYER_MAPPED_WIDTH;
    LayerModuleParam.LayerMappingIndex = 1; /* Compositor register mapping */

#ifdef ST_OSLINUX
    /* kernel checking with memory range to access */
    LayerContext_p->MappedGammaBaseAddress_p = STLINUX_MapRegion( (void*)LayerModuleParam.LayerBaseAddress, LayerModuleParam.LayerAddressWidth, "LAYER");

    if ( LayerContext_p->MappedGammaBaseAddress_p == NULL )
    {
        STTBX_Print(("Layer: STLINUX_MapRegion() Error!!!! \n"));
    }


    /** affecting LAYER Base Adress with mapped Base Adress **/
    LayerContext_p->BaseAddress  = LayerContext_p->MappedGammaBaseAddress_p + (U32)Params_p->BaseAddress_p - LAYER_COMPOSITOR_MAPPING ;
    STTBX_Print(("LAYER virtual io kernel address of phys 0x%x = 0x%x\n", ((U32)Params_p->BaseAddress_p | REGION2),
            (U32)(LayerContext_p->MappedGammaBaseAddress_p)));
#endif

    return ErrorCode;
}
#endif

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
    /* Enter this section only if before first Init (InterInstancesLockingSemaphore_p default set at NULL) */
    if (InterInstancesLockingSemaphore_p == NULL)
    {
        semaphore_t * TemporaryInterInstancesLockingSemaphore_p;
        BOOL          TemporarySemaphoreNeedsToBeDeleted;

        TemporaryInterInstancesLockingSemaphore_p = STOS_SemaphoreCreatePriority(NULL,1);

        /* This case is to protect general access of video functions, like init/open/close and term */
        /* We do not want to call semaphore_create within the task_lock/task_unlock boundaries because
         * semaphore_create internally calls STOS_SemaphoreWait and can cause normal scheduling to resume,
         * therefore unlocking the critical region */
#ifndef ST_OSLINUX
        STOS_TaskLock();
#endif
        if (InterInstancesLockingSemaphore_p == NULL)
        {
            InterInstancesLockingSemaphore_p = TemporaryInterInstancesLockingSemaphore_p;
            TemporarySemaphoreNeedsToBeDeleted = FALSE;
        }
        else
        {
            /* Remember to delete the temporary semaphore, if the InterInstancesLockingSemaphore_p
             * was already created by another video instance */
            TemporarySemaphoreNeedsToBeDeleted = TRUE;
        }
#ifndef ST_OSLINUX
        STOS_TaskUnlock();
#endif
        /* Delete the temporary semaphore */
        if(TemporarySemaphoreNeedsToBeDeleted)
        {
            STOS_SemaphoreDelete(NULL,TemporaryInterInstancesLockingSemaphore_p);
#if defined (ST_OS20)
                        STOS_MemoryDeallocate(system_partition, TemporaryInterInstancesLockingSemaphore_p);
#endif
        }
    }
    /* Wait for the Init/Open/Close/Term protection semaphore */
    STOS_SemaphoreWait(InterInstancesLockingSemaphore_p);
}

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
    STOS_SemaphoreSignal(InterInstancesLockingSemaphore_p);
}

/*******************************************************************************
Name        : InitLayerPool
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t InitLayerPool(const STLAYER_InitParams_t* const Params_p,
                   stlayer_XLayerContext_t * LayerContext)
{
    U32                         i,j;
    U32                         Add,Val;
    STAVMEM_MemoryRange_t       ForbiddenArea[1];
    U32                         ViewPortSize;
    ST_ErrorCode_t              Error;
    stlayer_Node_t *            NodeToAffect_p;
    STAVMEM_AllocBlockParams_t  AllocBlockParams;
    void *                      VirtualAddress;
    #ifdef WINCE_MSTV_ENHANCED
      U32                         OffsetOfCoefs; /* offset of the tables of filter coefs in the NodeArray table */
    #endif
    #if defined (ST_7109)
        U32 CutId;
    #endif

#if defined (DVD_SECURED_CHIP)&& !defined(STLAYER_NO_STMES)
     unsigned int uMemoryStatus=0;
#endif


   #if defined (ST_7109)
    CutId = ST_GetCutRevision();
   #endif





#ifdef ST_OSLINUX
    STAVMEM_GetSharedMemoryVirtualMapping2( LAYER_PARTITION_AVMEM, &LayerContext->VirtualMapping );
#else
    STAVMEM_GetSharedMemoryVirtualMapping(&LayerContext->VirtualMapping );
#endif





    ViewPortSize = sizeof(stlayer_ViewPortDescriptor_t);
    /* allocation of viewport descriptor pool and its handles if needed */
    if (Params_p->ViewPortBufferUserAllocated == FALSE)
    {
        LayerContext->ListViewPort =
        (stlayer_ViewPortDescriptor_t*) STOS_MemoryAllocate
                (Params_p->CPUPartition_p,
                 Params_p->MaxViewPorts *
                        (ViewPortSize + sizeof(STLAYER_ViewPortHandle_t)) );

    memset(LayerContext->ListViewPort, 0, Params_p->MaxViewPorts *
             (ViewPortSize + sizeof(STLAYER_ViewPortHandle_t)));

        LayerContext->VPDescriptorsUserAllocated = FALSE;
    }
    else /* viewport descriptors were allocated by user */
    {
        LayerContext->ListViewPort = Params_p->ViewPortBuffer_p;
        LayerContext->VPDescriptorsUserAllocated = TRUE;
    }

    /* Affect the Handle Array Pointer */
    /* Handles location is behind the viewports */
    LayerContext->HandleArray  = (stlayer_ViewPortDescriptor_t **)
                    ((U32)LayerContext->ListViewPort
                           + ( ViewPortSize * Params_p->MaxViewPorts)) ;

    /* allocation of nodes in AV memory if needed */
    if(Params_p->NodeBufferUserAllocated == FALSE)
    {
        ForbiddenArea[0].StartAddr_p = (void *)
               ((U32)(LayerContext->VirtualMapping.VirtualBaseAddress_p)
              + (U32)(LayerContext->VirtualMapping.VirtualWindowOffset)
              + (U32)(LayerContext->VirtualMapping.VirtualWindowSize));
        ForbiddenArea[0].StopAddr_p  =  (void *)
               ((U32)(LayerContext->VirtualMapping.VirtualBaseAddress_p)
              + (U32)(LayerContext->VirtualMapping.VirtualSize));

#ifdef ST_OSLINUX
        AllocBlockParams.PartitionHandle = PartitionHandle;
#else
        AllocBlockParams.PartitionHandle = Params_p->AVMEM_Partition;
#endif
        /* part for the nodes : */
        AllocBlockParams.Size = 2 * sizeof(stlayer_Node_t)
                                    * (Params_p->MaxViewPorts);
        if(LayerContext->LayerType != STLAYER_GAMMA_CURSOR)
        {
            /* part for the two dummy nodes */
#ifdef WINCE_MSTV_ENHANCED
            AllocBlockParams.Size += 2 * sizeof(stlayer_Node_t);
            // fast update: needs 2 lists of nodes (1 current, 1 next)
            AllocBlockParams.Size *= 2;
#else
            AllocBlockParams.Size += 8 * sizeof(stlayer_Node_t);
#endif
            if(LayerContext->LayerType != STLAYER_GAMMA_BKL)
            {
                /* part for the filters tables */
#ifdef WINCE_MSTV_ENHANCED
                OffsetOfCoefs = AllocBlockParams.Size;
#endif
                AllocBlockParams.Size += sizeof(stlayer_HSRC_Coeffs);
                AllocBlockParams.Size += sizeof(stlayer_VSRC_Coeffs);
                AllocBlockParams.Size += sizeof(stlayer_FF_Coeffs);
            }
        }
        AllocBlockParams.Alignment = 16;
        AllocBlockParams.AllocMode = STAVMEM_ALLOC_MODE_BOTTOM_TOP;
        AllocBlockParams.NumberOfForbiddenRanges  = 1;
        AllocBlockParams.ForbiddenRangeArray_p    = &ForbiddenArea[0];
        AllocBlockParams.NumberOfForbiddenBorders = 0;
        AllocBlockParams.ForbiddenBorderArray_p   = NULL;
        Error = STAVMEM_AllocBlock(&AllocBlockParams,
                                   &(LayerContext->BlockNodeArray));
        if (Error != ST_NO_ERROR)
        {
            layergfx_Defense(Error);
            return(Error);
        }

        Error = STAVMEM_GetBlockAddress(LayerContext->BlockNodeArray,
                                      (void **)&VirtualAddress);

        LayerContext->NodeArray =
                    STAVMEM_VirtualToCPU(VirtualAddress,&LayerContext->VirtualMapping);
        LayerContext->VPNodesUserAllocated = FALSE;
        if (Error != ST_NO_ERROR)
        {
            layergfx_Defense(Error);
            return(Error);
        }
    }
    else /* viewport nodes were allocated by user */
    {
        VirtualAddress = Params_p->ViewPortNodeBuffer_p;
        LayerContext->NodeArray =
            STAVMEM_VirtualToCPU(VirtualAddress,&LayerContext->VirtualMapping);
        LayerContext->VPNodesUserAllocated = TRUE;
    }


    /* link the viewport to the hw register if the layer is the cursor one */
    if(LayerContext->LayerType == STLAYER_GAMMA_CURSOR)
    {
        /* Then use the hw register */
        LayerContext->ListViewPort[0].TopNode_p
                   = (stlayer_Node_t *)(LayerContext->BaseAddress);
        /* bottom node is not used */
        LayerContext->ListViewPort[0].BotNode_p = 0;
        /* default values to avoid pipeline crash if mix enable the curs */
        NodeToAffect_p = LayerContext->ListViewPort[0].TopNode_p;

        STSYS_WriteRegDev32LE(&(NodeToAffect_p->CurNode.VPO),((18 << 16)|300));
        STSYS_WriteRegDev32LE(&(NodeToAffect_p->CurNode.PML),0);
        STSYS_WriteRegDev32LE(&(NodeToAffect_p->CurNode.PMP),128);
        STSYS_WriteRegDev32LE(&(NodeToAffect_p->CurNode.SIZE),((2 << 16)|12));
        STSYS_WriteRegDev32LE(&(NodeToAffect_p->CurNode.CML),0);
        STSYS_WriteRegDev32LE(&(NodeToAffect_p->CurNode.AWS),0x002d0118);
        STSYS_WriteRegDev32LE(&(NodeToAffect_p->CurNode.AWE),0x04640897);
    }
    else
    {
         /* link the viewport descriptors to the nodes */
        for (i=0; i<Params_p->MaxViewPorts; i++)
        {
            (LayerContext->ListViewPort[i]).TopNode_p
                                    = &(LayerContext->NodeArray[2*i]);
            (LayerContext->ListViewPort[i]).BotNode_p
                                    = &(LayerContext->NodeArray[2*i+1]);
            (LayerContext->ListViewPort[i]).Enabled
                                    = 0;
        }

        if (LayerContext->LayerType != STLAYER_GAMMA_BKL)
        {
            /* set and fill the Horizontals filters tables */
#ifdef WINCE_MSTV_ENHANCED
            LayerContext->FilterCoefs=(S8*)((U32)(LayerContext->NodeArray) + OffsetOfCoefs);
#else
            LayerContext->FilterCoefs=(S8*)(&(LayerContext->NodeArray[2*i+2]));
#endif
    #if 1
            for (j=0; j<(NB_HSRC_FILTERS
                      * (NB_HSRC_COEFFS + FILTERS_ALIGN) /4); j++)
            {

                Add = (U32)LayerContext->FilterCoefs + (j*4);
                Val = ((U8)(stlayer_HSRC_Coeffs[(j*4) + 0]) <<  0)
                    | ((U8)(stlayer_HSRC_Coeffs[(j*4) + 1]) <<  8)
                    | ((U8)(stlayer_HSRC_Coeffs[(j*4) + 2]) << 16)
                    | ((U8)(stlayer_HSRC_Coeffs[(j*4) + 3]) << 24);
                    STSYS_WriteRegDev32LE((U32)Add,(U32)Val);
            }
    #endif

   #if defined (ST_7109) || defined (ST_7200)

    #if defined (ST_7109)

    if (CutId >= 0xC0)
    {
    #endif
                /* set and fill the Verticals filters tables */
#ifdef WINCE_MSTV_ENHANCED
                LayerContext->VFilterCoefs=(S8*)((U32)(LayerContext->NodeArray) + OffsetOfCoefs + sizeof(stlayer_HSRC_Coeffs));
#else
                LayerContext->VFilterCoefs=(S8*)(&(LayerContext->NodeArray[2*i+6]));
#endif
                for (j=0; j<(NB_VSRC_FILTERS
                        * (NB_VSRC_COEFFS + VFILTERS_ALIGN) /4); j++)
                {

                    Add = (U32)LayerContext->VFilterCoefs + (j*4);
                    Val = ((U8)(stlayer_VSRC_Coeffs[(j*4) + 0]) <<  0)
                        | ((U8)(stlayer_VSRC_Coeffs[(j*4) + 1]) <<  8)
                        | ((U8)(stlayer_VSRC_Coeffs[(j*4) + 2]) << 16)
                        | ((U8)(stlayer_VSRC_Coeffs[(j*4) + 3]) << 24);
                        STSYS_WriteRegDev32LE((U32)Add,(U32)Val);
                }

             /* set and fill the Flikcer Filters tables */
#ifdef WINCE_MSTV_ENHANCED
                LayerContext->FFilterCoefs=(S8*)((U32)(LayerContext->NodeArray) + OffsetOfCoefs + sizeof(stlayer_HSRC_Coeffs) + sizeof(stlayer_VSRC_Coeffs));
#else
                LayerContext->FFilterCoefs=(S8*)(&(LayerContext->NodeArray[2*i+8]));
#endif

                for (j=0; j<(NB_FF_FILTERS
                        * (NB_FF_COEFFS + FF_FILTERS_ALIGN) /4); j++)
                {

                    Add = (U32)LayerContext->FFilterCoefs + (j*4);
                    Val = ((U8)(stlayer_FF_Coeffs[(j*4) + 0]) <<  0)
                        | ((U8)(stlayer_FF_Coeffs[(j*4) + 1]) <<  8)
                        | ((U8)(stlayer_FF_Coeffs[(j*4) + 2]) << 16)
                        | ((U8)(stlayer_FF_Coeffs[(j*4) + 3]) << 24);
                        STSYS_WriteRegDev32LE((U32)Add,(U32)Val);
                }

  #if defined (ST_7109)
   }
  #endif

    #endif
        }

        /* Initialise two service nodes */
        LayerContext->TmpTopNode_p = &(LayerContext->NodeArray
                                                [2*Params_p->MaxViewPorts]);
        LayerContext->TmpBotNode_p = &(LayerContext->NodeArray
                                                [2*Params_p->MaxViewPorts + 1]);
        /* default values to avoid pipeline crash if mix enable the pipeline */
        /* Top node */
        NodeToAffect_p = LayerContext->TmpTopNode_p;

        STSYS_WriteRegMem32LE(&(NodeToAffect_p->BklNode.CTL),0x80000007);

#ifdef WA_GNBycbcr
        if(LayerContext->DeviceId == 0x200) /* Reserved for DVP */
        {
            STSYS_WriteRegMem32LE(&(NodeToAffect_p->BklNode.CTL),0x80000012);
        }
#endif
        STSYS_WriteRegMem32LE(&(NodeToAffect_p->BklNode.AGC),0x00808080);
        STSYS_WriteRegMem32LE(&(NodeToAffect_p->BklNode.HSRC),0x100);
        STSYS_WriteRegMem32LE(&(NodeToAffect_p->BklNode.VPO),((18<< 16)|300));
        STSYS_WriteRegMem32LE(&(NodeToAffect_p->BklNode.VPS),((19<< 16)|311));
/*        STSYS_WriteRegMem32LE(&(NodeToAffect_p->BklNode.PML),0x06000200);*/
        STSYS_WriteRegMem32LE(&(NodeToAffect_p->BklNode.PML),0x04000000);
/*        STSYS_WriteRegMem32LE(&(NodeToAffect_p->BklNode.PML),0x200);*/
        STSYS_WriteRegMem32LE(&(NodeToAffect_p->BklNode.PMP),0x100);
        if(LayerContext->LayerType != STLAYER_GAMMA_BKL)
        { /* gdp like case : manage the scan type */
         if(LayerContext->LayerParams.ScanType == STGXOBJ_INTERLACED_SCAN)
         {
             /* interlaced : 1line per node */
          STSYS_WriteRegMem32LE(&(NodeToAffect_p->BklNode.SIZE),((1 << 16)|12));
         }
         else
         {
             /* prog : 2lines per node */
          STSYS_WriteRegMem32LE(&(NodeToAffect_p->BklNode.SIZE),((2 << 16)|12));
         }

        }
        else
        {
          STSYS_WriteRegMem32LE(&(NodeToAffect_p->BklNode.SIZE),((2 << 16)|12));
        }
        STSYS_WriteRegMem32LE(&(NodeToAffect_p->BklNode.VSRC),0);
        STSYS_WriteRegMem32LE(&(NodeToAffect_p->BklNode.KEY1),0);
        STSYS_WriteRegMem32LE(&(NodeToAffect_p->BklNode.KEY2),0);
        STSYS_WriteRegMem32LE(&(NodeToAffect_p->BklNode.HFP),0);
        STSYS_WriteRegMem32LE(&(NodeToAffect_p->BklNode.PPT),0);
                                         /* ifdef cut3.0 */
        STSYS_WriteRegMem32LE(&(NodeToAffect_p->BklNode.CML),0);
        STSYS_WriteRegMem32LE(&(NodeToAffect_p->BklNode.VFP),0);


#if defined (DVD_SECURED_CHIP)&& !defined(STLAYER_NO_STMES)

        uMemoryStatus = STMES_IsMemorySecure((void *)DeviceAdd(LayerContext->TmpBotNode_p), (U32)(MAX_GDP_NODE_SIZE), 0);
        if(uMemoryStatus == SECURE_REGION)
        {
            STSYS_WriteRegMem32LE(&(NodeToAffect_p->BklNode.NVN), (U32)(DeviceAdd(LayerContext->TmpBotNode_p)| STLAYER_SECURE_ON));
         }
        else
        {
            STSYS_WriteRegMem32LE(&(NodeToAffect_p->BklNode.NVN), (U32)(DeviceAdd(LayerContext->TmpBotNode_p)& STLAYER_SECURE_MASK));
        }

#else
        STSYS_WriteRegMem32LE(&(NodeToAffect_p->BklNode.NVN),
                                    (U32)DeviceAdd(LayerContext->TmpBotNode_p));
#endif
        /* Bottom node */
        NodeToAffect_p = LayerContext->TmpBotNode_p;
        STOS_memcpy(NodeToAffect_p, LayerContext->TmpTopNode_p, sizeof(stlayer_Node_t));

#if defined (DVD_SECURED_CHIP)&& !defined(STLAYER_NO_STMES)
        uMemoryStatus = STMES_IsMemorySecure((void *)DeviceAdd(LayerContext->TmpTopNode_p), (U32)(MAX_GDP_NODE_SIZE), 0);
        if(uMemoryStatus == SECURE_REGION)
        {
            STSYS_WriteRegMem32LE(&(NodeToAffect_p->BklNode.NVN),
                                    (U32)(DeviceAdd(LayerContext->TmpTopNode_p)| STLAYER_SECURE_ON));
        }
        else
        {
            STSYS_WriteRegMem32LE(&(NodeToAffect_p->BklNode.NVN),
                                    (U32)(DeviceAdd(LayerContext->TmpTopNode_p)& STLAYER_SECURE_MASK));
        }

#else
        STSYS_WriteRegMem32LE(&(NodeToAffect_p->BklNode.NVN),
                                    (U32)DeviceAdd(LayerContext->TmpTopNode_p));
#endif
        /* start pipeline : PKZ  and NVN register in base address */
        NodeToAffect_p = (stlayer_Node_t *)(LayerContext->BaseAddress);
#ifdef HW_7020
        if(Params_p->CPUBigEndian)
        {
            STSYS_WriteRegDev32LE((U32)&(NodeToAffect_p->BklNode) + PKZ_OFFSET,
                    ((1<<5)) );
        }
        else
        {
            STSYS_WriteRegDev32LE((U32)&(NodeToAffect_p->BklNode) + PKZ_OFFSET,
                    ((0<<5)) );
        }
#endif /* HW_7020 */
#ifdef HW_5528
            STSYS_WriteRegDev32LE((U32)&(NodeToAffect_p->BklNode) + PKZ_OFFSET,
                    0x10 );
#endif /* HW_5528 */
#ifdef HW_7710
        if(Params_p->CPUBigEndian)
        {
            STSYS_WriteRegDev32LE((U32)&(NodeToAffect_p->BklNode) + PKZ_OFFSET,
                    0x20 );
        }
        else
        {
            STSYS_WriteRegDev32LE((U32)&(NodeToAffect_p->BklNode) + PKZ_OFFSET,
                    0x0 );
        }
#endif /* HW_7710 */
#if defined HW_7100
        if(Params_p->CPUBigEndian)
        {
            STSYS_WriteRegDev32LE((U32)&(NodeToAffect_p->BklNode) + PKZ_OFFSET,
                    0x20 );
        }
        else
        {
            STSYS_WriteRegDev32LE((U32)&(NodeToAffect_p->BklNode) + PKZ_OFFSET,
                    0x0 );
        }
#endif /* HW_7100 */
#if defined HW_7109

        if (CutId < 0xC0)
        {

        if(Params_p->CPUBigEndian)
        {
            STSYS_WriteRegDev32LE((U32)&(NodeToAffect_p->BklNode) + PKZ_OFFSET,
                    0x20 );
        }
        else
        {
            STSYS_WriteRegDev32LE((U32)&(NodeToAffect_p->BklNode) + PKZ_OFFSET,
                    0x0 );
        }

        }
        else
        {


     switch(LayerContext->LayerType)

            {

       case STLAYER_GAMMA_GDP :
            STSYS_WriteRegDev32LE((U32)&(NodeToAffect_p->BklNode) + PAS_OFFSET ,0x2 );
            STSYS_WriteRegDev32LE((U32)&(NodeToAffect_p->BklNode) + MAOS_OFFSET,0x5 );
            STSYS_WriteRegDev32LE((U32)&(NodeToAffect_p->BklNode) + MIOS_OFFSET,0x3 );
            STSYS_WriteRegDev32LE((U32)&(NodeToAffect_p->BklNode) + MACS_OFFSET,0x3 );
            STSYS_WriteRegDev32LE((U32)&(NodeToAffect_p->BklNode) + MAMS_OFFSET,0x0 );

            break;

       case STLAYER_GAMMA_CURSOR :
            STSYS_WriteRegDev32LE((U32)&(NodeToAffect_p->BklNode) + PAS_OFFSET ,0x2 );
            STSYS_WriteRegDev32LE((U32)&(NodeToAffect_p->BklNode) + MAOS_OFFSET,0x5 );
            STSYS_WriteRegDev32LE((U32)&(NodeToAffect_p->BklNode) + MIOS_OFFSET,0x3 );
            STSYS_WriteRegDev32LE((U32)&(NodeToAffect_p->BklNode) + MACS_OFFSET,0x2 );
            STSYS_WriteRegDev32LE((U32)&(NodeToAffect_p->BklNode) + MAMS_OFFSET,0x0 );

            break;

       case STLAYER_GAMMA_ALPHA :
            STSYS_WriteRegDev32LE((U32)&(NodeToAffect_p->BklNode) + PAS_OFFSET ,0x2 );
            STSYS_WriteRegDev32LE((U32)&(NodeToAffect_p->BklNode) + MAOS_OFFSET,0x5 );
            STSYS_WriteRegDev32LE((U32)&(NodeToAffect_p->BklNode) + MIOS_OFFSET,0x3 );
            STSYS_WriteRegDev32LE((U32)&(NodeToAffect_p->BklNode) + MACS_OFFSET,0x2 );
            STSYS_WriteRegDev32LE((U32)&(NodeToAffect_p->BklNode) + MAMS_OFFSET,0x0 );

            break;


      default:
            break;
            }


         }
#endif /* HW_7109 */


#if defined (DVD_SECURED_CHIP)&& !defined(STLAYER_NO_STMES)

        uMemoryStatus = STMES_IsMemorySecure((void *)DeviceAdd(LayerContext->TmpTopNode_p), (U32)(MAX_GDP_NODE_SIZE), 0);
        if(uMemoryStatus == SECURE_REGION)
        {
            STSYS_WriteRegDev32LE(&(NodeToAffect_p->BklNode.NVN),
                                    (U32)(DeviceAdd(LayerContext->TmpTopNode_p)| STLAYER_SECURE_ON));
         }
        else
         {
            STSYS_WriteRegDev32LE(&(NodeToAffect_p->BklNode.NVN),
                                    (U32)(DeviceAdd(LayerContext->TmpTopNode_p)& STLAYER_SECURE_MASK));
         }

#else
        STSYS_WriteRegDev32LE(&(NodeToAffect_p->BklNode.NVN),
                                    (U32)DeviceAdd(LayerContext->TmpTopNode_p));

#endif



    }

    /* Initialize handles buffer */
    LayerContext->MaxViewPorts  = Params_p->MaxViewPorts;
    stlayer_InitDataPool(&LayerContext->DataPoolDesc,Params_p->MaxViewPorts,
                        ViewPortSize,(void*)LayerContext->ListViewPort,
                       (Handle_t*)LayerContext->HandleArray);

    return (ST_NO_ERROR);
}


/*******************************************************************************
Name        : ReleaseLayerPool
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t ReleaseLayerPool(STLAYER_Handle_t  Handle )
{
    STAVMEM_FreeBlockParams_t   FreeParams;
    ST_ErrorCode_t              Error;

    /* release viewports and handles */
    if(stlayer_context.XContext[Handle].VPDescriptorsUserAllocated == FALSE)
    {
        /* The driver must deallocate the VP Pool */
        STOS_MemoryDeallocate
                    (stlayer_context.XContext[Handle].CPUPartition_p,
                    (void *)stlayer_context.XContext[Handle].ListViewPort);
        stlayer_context.XContext[Handle].ListViewPort = 0;
        stlayer_context.XContext[Handle].HandleArray  = 0;
    }
    else
    {
        /* The user is responsable of its VP Pool */
    }

    /* release nodes */
    if(stlayer_context.XContext[Handle].VPNodesUserAllocated == FALSE)
    {
        /* The driver must deallocate the nodes Pool */
        FreeParams.PartitionHandle =
                    stlayer_context.XContext[Handle].AVMEM_Partition;
        Error = STAVMEM_FreeBlock(&FreeParams,
                          &(stlayer_context.XContext[Handle].BlockNodeArray));
        if(Error != ST_NO_ERROR)
        {
            layergfx_Defense(Error);
            return(Error);
        }
        stlayer_context.XContext[Handle].NodeArray = 0;
    }
    else
    {
       /* The user is responsable of its VP nodes */
    }
    return (ST_NO_ERROR);
}
/******************************************************************************/






