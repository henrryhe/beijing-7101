/*******************************************************************************

File name   : sub_init.c

Description : Sub picture init module standard API functions source file

COPYRIGHT (C) STMicroelectronics 2004

Date               Modification                                     Name
----               ------------                                     ----
18 Dec 2000        Created                                           TM
*******************************************************************************/

/* Includes ----------------------------------------------------------------- */

#include <stdlib.h>
#include <string.h>

#include "stddefs.h"
#include "sttbx.h"

#include "stlayer.h"
#include "sub_init.h"
#include "sub_vp.h"
#include "sub_pool.h"
#include "sub_com.h"
#include "lay_sub.h"
#include "sub_reg.h"
#include "stsys.h"
#include "subswcfg.h"




/* Private Types ------------------------------------------------------------ */


/* Private Constants -------------------------------------------------------- */

#define LAYERSUB_MAX_DEVICE  1     /* Max number of Init() allowed */
#define LAYERSUB_MAX_OPEN    2     /* Max number of Open() allowed per Init() */
#define LAYERSUB_MAX_UNIT    (LAYERSUB_MAX_OPEN * LAYERSUB_MAX_DEVICE)

#define INVALID_DEVICE_INDEX (-1)

/* Private Variables (static)------------------------------------------------ */

static layersub_Device_t DeviceArray[LAYERSUB_MAX_DEVICE];
static layersub_Unit_t UnitArray[LAYERSUB_MAX_UNIT];
static semaphore_t * InstancesAccessControl_p;   /* Init/Open/Close/Term protection semaphore */
static BOOL FirstInitDone = FALSE;


/* Global Variables --------------------------------------------------------- */


/* Private Macros ----------------------------------------------------------- */

/* Passing a (STLAYER_Handle_t), returns TRUE if the handle is valid, FALSE otherwise */
#define IsValidHandle(Handle) ((((layersub_Unit_t *) (Handle)) >= UnitArray) &&                    \
                               (((layersub_Unit_t *) (Handle)) < (UnitArray + LAYERSUB_MAX_UNIT)) &&  \
                               (((layersub_Unit_t *) (Handle))->UnitValidity == LAYERSUB_VALID_UNIT))


/* Private Function prototypes ---------------------------------------------- */

static void EnterCriticalSection(void);
static void LeaveCriticalSection(void);
static S32 GetIndexOfDeviceNamed(const ST_DeviceName_t WantedName);
static ST_ErrorCode_t Init(layersub_Device_t * const Device_p, const STLAYER_InitParams_t * const InitParams_p);
static ST_ErrorCode_t Open(layersub_Unit_t * const Unit_p, const STLAYER_OpenParams_t * const OpenParams_p);
static ST_ErrorCode_t Close(layersub_Unit_t * const Unit_p);
static ST_ErrorCode_t Term(layersub_Device_t * const Device_p, const STLAYER_TermParams_t * const TermParams_p);


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

    interrupt_lock();
    if (!InstancesAccessControlInitialized)
    {
        /* Initialise the Init/Open/Close/Term protection semaphore
          Caution: this semaphore is never deleted. (Not an issue) */
        InstancesAccessControl_p = semaphore_create_fifo(1);
        InstancesAccessControlInitialized = TRUE;
    }
    interrupt_unlock();

    /* Wait for the Init/Open/Close/Term protection semaphore */
    semaphore_wait(InstancesAccessControl_p);
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
    semaphore_signal(InstancesAccessControl_p);
} /* End of LeaveCriticalSection() function */

/*******************************************************************************
Name        :  LAYERSUB_SetLayerParams
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYERSUB_SetLayerParams(STLAYER_Handle_t Handle, STLAYER_LayerParams_t* LayerParams_p)
{
    ST_ErrorCode_t      Err         = ST_NO_ERROR;
    layersub_Unit_t*    Unit_p      = (layersub_Unit_t*)Handle;
    layersub_Device_t*  Device_p;
    layersub_ViewPort_t* CurrentViewPort_p;
    U32                 i;

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
    CurrentViewPort_p = (layersub_ViewPort_t*) Unit_p->FirstOpenViewPort;
    for (i=0; i < Unit_p->NbOpenViewPorts; i++ )
    {
        /* Output rectangle position of ViewPort is always > 0 */
        if (((U32)(CurrentViewPort_p->OutputRectangle.PositionX +
                   CurrentViewPort_p->OutputRectangle.Width - 1) > LayerParams_p->Width) ||
            ((U32)(CurrentViewPort_p->OutputRectangle.PositionY +
                   CurrentViewPort_p->OutputRectangle.Height - 1) > LayerParams_p->Height))
        {
            return(STLAYER_ERROR_OUT_OF_LAYER);
        }
        CurrentViewPort_p = (layersub_ViewPort_t*)(CurrentViewPort_p->NextViewPort);
    }


    /* Update Device_p structure */
    Device_p->ScanType          = LayerParams_p->ScanType;
    Device_p->AspectRatio       = LayerParams_p->AspectRatio;
    Device_p->LayerWidth        = LayerParams_p->Width;
    Device_p->LayerHeight       = LayerParams_p->Height;


    return(Err);
}

/*******************************************************************************
Name        :  LAYERSUB_GetLayerParams
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYERSUB_GetLayerParams(STLAYER_Handle_t Handle, STLAYER_LayerParams_t* LayerParams_p)
{
    ST_ErrorCode_t      Err         = ST_NO_ERROR;
    layersub_Unit_t*    Unit_p      = (layersub_Unit_t*)Handle;
    layersub_Device_t*  Device_p;


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
    if (LayerParams_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Get params */
    LayerParams_p->ScanType      =  Device_p->ScanType;
    LayerParams_p->AspectRatio   =  Device_p->AspectRatio;
    LayerParams_p->Width         =  Device_p->LayerWidth;
    LayerParams_p->Height        =  Device_p->LayerHeight;


    return(Err);
}

/*******************************************************************************
Name        :  LAYERSUB_GetInitAllocParams
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYERSUB_GetInitAllocParams(STLAYER_Layer_t         LayerType,
                                           U32                     ViewPortsNumber,
                                           STLAYER_AllocParams_t * Params_p)
{
    ST_ErrorCode_t      Err  = ST_NO_ERROR;

    if ((ViewPortsNumber == 0) ||
        (Params_p == NULL))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    Params_p->ViewPortDescriptorsBufferSize = ViewPortsNumber * sizeof(layersub_ViewPort_t);
/*    Params->ViewPortNodesInSharedMemory   = FALSE;*/
/*    Params->ViewPortNodesBufferAlignment  = 0;*/
/*    Params->ViewPortNodesBufferSize       = 0;*/

    return(Err);
}

/*******************************************************************************
Name        :  LAYERSUB_GetBitmapAllocParams
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYERSUB_GetBitmapAllocParams(STLAYER_Handle_t             Handle,
                                             STGXOBJ_Bitmap_t*            Bitmap_p,
                                             STGXOBJ_BitmapAllocParams_t* Params1_p,
                                             STGXOBJ_BitmapAllocParams_t* Params2_p)
{
    ST_ErrorCode_t      Err  = ST_NO_ERROR;
    layersub_Unit_t*    Unit_p      = (layersub_Unit_t*)Handle;

    /* Check Handle validity */
    if (Handle == (STLAYER_Handle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    if (Unit_p->UnitValidity != LAYERSUB_VALID_UNIT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Check general params */
    if ((Params1_p == NULL) ||
        (Params2_p == NULL) ||
        (Bitmap_p == NULL))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if ((Bitmap_p->ColorType  != STGXOBJ_COLOR_TYPE_CLUT2) ||
        (Bitmap_p->BitmapType != STGXOBJ_BITMAP_TYPE_RASTER_PROGRESSIVE))
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    /* Set Params1_p
     * No particular constraints (apart from byte multiple pitch) since the driver is copying the bitmap data
     * in a well aligned SPU!*/
    if ((Bitmap_p->Width % 4) == 0)
    {
        Params1_p->Pitch  = Bitmap_p->Width / 4;
    }
    else
    {
        Params1_p->Pitch  = (Bitmap_p->Width / 4) + 1;
    }
    Params1_p->AllocBlockParams.Size        = Params1_p->Pitch  * Bitmap_p->Height;
    Params1_p->AllocBlockParams.Alignment   = 1;
    Params1_p->Offset                       = 0;

    return(Err);
}

/*******************************************************************************
Name        :  LAYERSUB_GetPaletteAllocParams
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYERSUB_GetPaletteAllocParams(STLAYER_Handle_t               Handle,
                                              STGXOBJ_Palette_t*             Palette_p,
                                              STGXOBJ_PaletteAllocParams_t*  Params_p)
{
    ST_ErrorCode_t      Err  = ST_NO_ERROR;
    layersub_Unit_t*    Unit_p      = (layersub_Unit_t*)Handle;

    /* Check Handle validity */
    if (Handle == (STLAYER_Handle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    if (Unit_p->UnitValidity != LAYERSUB_VALID_UNIT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Check general params */
    if ((Params_p == NULL) ||
        (Palette_p == NULL))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if ((Palette_p->ColorType == STGXOBJ_COLOR_TYPE_ARGB8888) &&
        ( Palette_p->ColorDepth == 2))
    {
        Params_p->AllocBlockParams.Alignment = 1;
        Params_p->AllocBlockParams.Size = 4 * (1 << Palette_p->ColorDepth);
    }
    else
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }


    return(Err);
}

/*******************************************************************************
Name        :  LAYERSUB_GetBitmapHeaderSize
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYERSUB_GetBitmapHeaderSize(STLAYER_Handle_t             Handle,
                                            STGXOBJ_Bitmap_t*            Bitmap_p,
                                            U32 *                        HeaderSize_p)
{
    ST_ErrorCode_t      Err  = ST_NO_ERROR;
    layersub_Unit_t*    Unit_p      = (layersub_Unit_t*)Handle;

    /* Check Handle validity */
    if (Handle == (STLAYER_Handle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    if (Unit_p->UnitValidity != LAYERSUB_VALID_UNIT)
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
static ST_ErrorCode_t Init(layersub_Device_t * const Device_p, const STLAYER_InitParams_t * const InitParams_p)
{
    ST_ErrorCode_t          Err               = ST_NO_ERROR;
    void*                   Elembuffer_p;
    void**                  Handlebuffer_p;
    U32                     NumberElem;
    layersub_ViewPort_t*    ViewPort_p;
    STEVT_OpenParams_t      OpenParams;
    U32                     i;
    U32                     BitBufferStart;
    U32                     BitBufferStop;
    U8*                     Address_p;

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
        Elembuffer_p = (void*) memory_allocate(Device_p->CPUPartition_p,
                                              (NumberElem * sizeof(layersub_ViewPort_t)));
        if (Elembuffer_p == NULL)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "CPU memory allocation failed"));
            return(ST_ERROR_NO_MEMORY);
        }
        Device_p->ViewPortBufferUserAllocated = FALSE ;
    }
    Handlebuffer_p = (void**) memory_allocate(Device_p->CPUPartition_p,
                                             (NumberElem * sizeof(layersub_ViewPort_t*)));
    if (Handlebuffer_p == NULL)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "CPU memory allocation failed"));
        return(ST_ERROR_NO_MEMORY);
    }
    layersub_InitDataPool(&(Device_p->ViewPortDataPool),
                        NumberElem,
                        sizeof(layersub_ViewPort_t),
                        Elembuffer_p,
                        Handlebuffer_p);

    /* Initialize ViewPort Validity field in the ViewPortDataPool */
    ViewPort_p = (layersub_ViewPort_t*)Elembuffer_p;
    for (i=0; i<NumberElem; i++)
    {
        ViewPort_p->ViewPortValidity = (U32)(~LAYERSUB_VALID_VIEWPORT);
        ViewPort_p++;
    }

    /* Device initialization */
    Device_p->ScanType          = InitParams_p->LayerParams_p->ScanType;
    Device_p->AspectRatio       = InitParams_p->LayerParams_p->AspectRatio;
    Device_p->LayerWidth        = InitParams_p->LayerParams_p->Width;
    Device_p->LayerHeight       = InitParams_p->LayerParams_p->Height;
    Device_p->XStart            = 132; /* PAL */
    Device_p->YStart            = 23+23;  /* PAL */
    Device_p->XOffset           = 0;
    Device_p->YOffset           = 0;
    Device_p->ActiveViewPort    = LAYERSUB_NO_VIEWPORT_HANDLE;
    Device_p->DisplayStatus     = LAYERSUB_DISPLAY_STATUS_DISABLE;
    Device_p->MixerConnected    = FALSE;
    Device_p->MixerWidth        = Device_p->LayerWidth;
    Device_p->MixerHeight       = Device_p->LayerHeight;
    Device_p->SwapActiveSPU     = FALSE;
    Device_p->IsEvtVsyncSubscribed  = FALSE;

    /* Sub picture decoder Soft reset */
    STSYS_WriteRegDev8((void*)((U32)Device_p->BaseAddress_p + LAYERSUB_SPD_SPR_OFFSET),LAYERSUB_SPD_SPR);
    STSYS_WriteRegDev8((void*)((U32)Device_p->BaseAddress_p + LAYERSUB_SPD_SPR_OFFSET),0);

    /* Reset auto increment address counter */
    STSYS_WriteRegDev8((void*)((U32)Device_p->BaseAddress_p + LAYERSUB_SPD_CTL2_OFFSET), LAYERSUB_SPD_CTL2_RC);
    STSYS_WriteRegDev8((void*)((U32)Device_p->BaseAddress_p + LAYERSUB_SPD_CTL2_OFFSET), 0);

    /* Set SubPicture Buffer = SDRAM window!
     *  + Translate from Virtual to CPU mapping
     *  + Address Offset in unit of 2 Kbyte relative to SDRAM base address
     *  + Set SPB, SPE */
    Address_p = (U8*)STAVMEM_VirtualToCPU((void*)((U32)Device_p->VirtualMapping.VirtualBaseAddress_p +
                                           Device_p->VirtualMapping.VirtualWindowOffset),&(Device_p->VirtualMapping));

    BitBufferStart = (U32)(Address_p) / 2048;
    BitBufferStop  = ((U32)(Address_p) + Device_p->VirtualMapping.VirtualWindowSize - 1) / 2048;

    /* Set SPB */
    STSYS_WriteRegDev8((void*)(LAYERSUB_VID_BASE_ADDRESS + LAYERSUB_VID_SPB_LESS_OFFSET),
                        BitBufferStart & LAYERSUB_VID_SPB_LESS_MASK);
    STSYS_WriteRegDev8((void*)(LAYERSUB_VID_BASE_ADDRESS + LAYERSUB_VID_SPB_MOST_OFFSET),
                       (BitBufferStart >> 8) & LAYERSUB_VID_SPB_MOST_MASK);

    /* Set SPE */
    STSYS_WriteRegDev8((void*)(LAYERSUB_VID_BASE_ADDRESS + LAYERSUB_VID_SPE_LESS_OFFSET),
                        BitBufferStop & LAYERSUB_VID_SPE_LESS_MASK);
    STSYS_WriteRegDev8((void*)(LAYERSUB_VID_BASE_ADDRESS + LAYERSUB_VID_SPE_MOST_OFFSET),
                       (BitBufferStop >> 8) & LAYERSUB_VID_SPE_MOST_MASK);

    /* semaphore init */
    Device_p->ProcessEvtVSyncSemaphore_p = semaphore_create_fifo(0);

    /* Task initialization */
    Device_p->TaskTerminate = FALSE;
    Device_p->ProcessEvtVSyncTask_p = task_create ((void(*)(void*))layersub_ProcessEvtVSync,
            (void*)Device_p, PROCESS_EVT_VSYNC_TASK_STACK_SIZE,
            STLAYER_SUBPICTURE_TASK_PRIORITY, "Process Evt Vsync",task_flags_no_min_stack_size );

    /* Events open (subscribe is done by UpdateFromMixer fct when VTG reason)*/
    strcpy(Device_p->EventHandlerName,InitParams_p->EventHandlerName);
    Err = STEVT_Open(Device_p->EventHandlerName, &OpenParams, &(Device_p->EvtHandle));
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error %X while Opening EventHandler",Err));
        Err = ST_ERROR_BAD_PARAMETER;
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
static ST_ErrorCode_t Open(layersub_Unit_t * const Unit_p, const STLAYER_OpenParams_t * const OpenParams_p)
{
    ST_ErrorCode_t Err = ST_NO_ERROR;

    Unit_p->FirstOpenViewPort   = LAYERSUB_NO_VIEWPORT_HANDLE;
    Unit_p->LastOpenViewPort    = LAYERSUB_NO_VIEWPORT_HANDLE;
    Unit_p->NbOpenViewPorts     = 0;

    return(Err);
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
static ST_ErrorCode_t Close(layersub_Unit_t * const Unit_p)
{
    ST_ErrorCode_t Err = ST_NO_ERROR;

    if (Unit_p->NbOpenViewPorts != 0)
    {
        /*  Unit_p->FirstOpenViewPort != LAYERSUB_NO_VIEWPORT_HANDLE */
        layersub_CloseAllViewPortFromList(Unit_p,Unit_p->FirstOpenViewPort);
    }

    /* Update Unit_p */
    Unit_p->FirstOpenViewPort   = LAYERSUB_NO_VIEWPORT_HANDLE;
    Unit_p->LastOpenViewPort    = LAYERSUB_NO_VIEWPORT_HANDLE;
    Unit_p->NbOpenViewPorts     = 0;

    return(Err);
} /* End of Close() function */

/*******************************************************************************
Name        : Term
Description : API specific terminations
Parameters  : pointer on device and termination parameters
Assumptions : pointer on device is not NULL
Limitations :
Returns     : ST_ErrorCode_t for the API Term function
*******************************************************************************/
static ST_ErrorCode_t Term(layersub_Device_t * const Device_p, const STLAYER_TermParams_t * const TermParams_p)
{
    ST_ErrorCode_t Err = ST_NO_ERROR;

    /* Close Event related */
    if (STEVT_Close(Device_p->EvtHandle) != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Evt Close failed"));
    }

    /* Task deletion process
     * TaskTerminate has to be set to FALSE before the TERMINATE Message to avoid the system to hang! */
    Device_p->TaskTerminate = TRUE;

    /* Signal any semaphore pending to go out of the waits (TBD) */
    semaphore_signal(Device_p->ProcessEvtVSyncSemaphore_p);

    /* delete task */
    task_wait(&Device_p->ProcessEvtVSyncTask_p,1,TIMEOUT_INFINITY);
    task_delete(Device_p->ProcessEvtVSyncTask_p);

    /* ViewPort Descriptor Data Pool deallocation  */
    memory_deallocate(Device_p->CPUPartition_p, Device_p->ViewPortDataPool.HandleArray_p);
    if (Device_p->ViewPortBufferUserAllocated != TRUE)   /* buffer has been allocated by driver */
    {
        memory_deallocate(Device_p->CPUPartition_p, Device_p->ViewPortDataPool.ElemArray_p);
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
    } while ((WantedIndex == INVALID_DEVICE_INDEX) && (Index < LAYERSUB_MAX_DEVICE));

    return(WantedIndex);
} /* End of GetIndexOfDeviceNamed() function */


/*
--------------------------------------------------------------------------------
Get capabilities of LAYERSUB_GetCapability API
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t LAYERSUB_GetCapability(const ST_DeviceName_t DeviceName, STLAYER_Capability_t * const Capability_p)
{
    S32 DeviceIndex = INVALID_DEVICE_INDEX;
    ST_ErrorCode_t Err = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if ((Capability_p == NULL) ||                       /* Pointer to capability structure should be valid ! */
        (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1) || /* Device name length should be respected */
        ((((char *) DeviceName)[0]) == '\0')
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
        Capability_p->MultipleViewPorts                     = FALSE;
        Capability_p->HorizontalResizing                    = FALSE;
        Capability_p->AlphaBorder                           = FALSE;
        Capability_p->GlobalAlpha                           = TRUE;
        Capability_p->ColorKeying                           = FALSE;
        Capability_p->MultipleViewPortsOnScanLineCapable    = FALSE;
        Capability_p->LayerType                             = STLAYER_OMEGA1_CURSOR;
    }

    LeaveCriticalSection();

    return(Err);
} /* End of LAYERSUB_GetCapability() function */


/*
--------------------------------------------------------------------------------
Initialise LAYERSUB_Init driver
(Standard instances management. Driver specific implementations should be put in Init() function.)
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t LAYERSUB_Init(const ST_DeviceName_t DeviceName, const STLAYER_InitParams_t * const InitParams_p)
{
    S32 Index = 0;
    ST_ErrorCode_t Err = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if (InitParams_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if ((InitParams_p->MaxHandles > LAYERSUB_MAX_OPEN) ||     /* No more than MAX_OPEN open handles supported */
        (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1) || /* Device name length should be respected */
        ((((char *) DeviceName)[0]) == '\0')            /* Device name should not be empty */
       )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }


    EnterCriticalSection();

    /* First time: initialise devices and units as empty */
    if (!FirstInitDone)
    {
        for (Index = 0; Index < LAYERSUB_MAX_DEVICE; Index++)
        {
            DeviceArray[Index].DeviceName[0] = '\0';
        }

        for (Index = 0; Index < LAYERSUB_MAX_UNIT; Index++)
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
        while ((DeviceArray[Index].DeviceName[0] != '\0') && (Index < LAYERSUB_MAX_DEVICE))
        {
            Index++;
        }
        if (Index >= LAYERSUB_MAX_DEVICE)
        {
            /* All devices initialised */
            Err = ST_ERROR_NO_MEMORY;
        }
        else
        {
            /* API specific initialisations */
            Err = Init(&DeviceArray[Index], InitParams_p);

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
} /* End of LAYERSUB_Init() function */


/*
--------------------------------------------------------------------------------
open ...
(Standard instances management. Driver specific implementations should be put in Open() function.)
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t LAYERSUB_Open(const ST_DeviceName_t DeviceName, const STLAYER_OpenParams_t * const OpenParams_p, STLAYER_Handle_t *Handle_p)
{
    S32 DeviceIndex = INVALID_DEVICE_INDEX, UnitIndex, Index;
    U32 OpenedUnitForThisInit;
    ST_ErrorCode_t Err = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if ((OpenParams_p == NULL) ||                       /* There must be parameters ! */
        (Handle_p == NULL) ||                           /* Pointer to handle should be valid ! */
        (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1) || /* Device name length should be respected */
        ((((char *) DeviceName)[0]) == '\0')
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
            UnitIndex = LAYERSUB_MAX_UNIT;
            OpenedUnitForThisInit = 0;
            for (Index = 0; Index < LAYERSUB_MAX_UNIT; Index++)
            {
                if ((UnitArray[Index].UnitValidity == LAYERSUB_VALID_UNIT) && (UnitArray[Index].Device_p == &DeviceArray[DeviceIndex]))
                {
                    OpenedUnitForThisInit ++;
                }
                if (UnitArray[Index].UnitValidity != LAYERSUB_VALID_UNIT)
                {
                    /* Found a free handle structure */
                    UnitIndex = Index;
                }
            }
            if ((OpenedUnitForThisInit >= DeviceArray[DeviceIndex].MaxHandles) || (UnitIndex >= LAYERSUB_MAX_UNIT))
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
                    UnitArray[UnitIndex].UnitValidity = LAYERSUB_VALID_UNIT;

                    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Handle opened on device '%s'", DeviceName));
                }
            } /* End found unit unused */
        } /* End device valid */
    } /* End FirstInitDone */

    LeaveCriticalSection();

    return(Err);
} /* End fo LAYERSUB_Open() function */


/*
--------------------------------------------------------------------------------
Close ...
(Standard instances management. Driver specific implementations should be put in Close() function.)
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t LAYERSUB_Close(STLAYER_Handle_t Handle)
{
    layersub_Unit_t *Unit_p;
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
            Unit_p = (layersub_Unit_t *) Handle;

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
} /* End of LAYERSUB_Close() function */


/*
--------------------------------------------------------------------------------
Terminate xxxxx driver
(Standard instances management. Driver specific implementations should be put in Term() function.)
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t LAYERSUB_Term(const ST_DeviceName_t DeviceName, const STLAYER_TermParams_t *const TermParams_p)
{
    layersub_Unit_t *Unit_p;
    S32 DeviceIndex = INVALID_DEVICE_INDEX, UnitIndex;
    BOOL Found = FALSE;
    ST_ErrorCode_t Err = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if ((TermParams_p == NULL) ||                       /* There must be parameters ! */
        (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1) || /* Device name length should be respected */
        ((((char *) DeviceName)[0]) == '\0')            /* Device name should not be empty */
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
            while ((UnitIndex < LAYERSUB_MAX_UNIT) && (!Found))
            {
                Found = ((Unit_p->Device_p == &DeviceArray[DeviceIndex]) && (Unit_p->UnitValidity == LAYERSUB_VALID_UNIT));
                Unit_p++;
                UnitIndex++;
            }

            if (Found)
            {
                if (TermParams_p->ForceTerminate)
                {
                    UnitIndex = 0;
                    Unit_p = UnitArray;
                    while (UnitIndex < LAYERSUB_MAX_UNIT)
                    {
                        if ((Unit_p->Device_p == &DeviceArray[DeviceIndex]) && (Unit_p->UnitValidity == LAYERSUB_VALID_UNIT))
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
                    /* Device found: desallocate memory, free device */
                    DeviceArray[DeviceIndex].DeviceName[0] = '\0';

                    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Device '%s' terminated", DeviceName));
                }
            } /* End terminate OK */
        } /* End valid device */
    } /* End FirstInitDone */

    LeaveCriticalSection();

    return(Err);
} /* End of LAYERSUB_Term() function */


/* End of sub_init.c */
