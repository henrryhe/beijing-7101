/*******************************************************************************

File name   :osd_cm.c
Description : It provides the OSD initialization and termination

COPYRIGHT (C) STMicroelectronics 2004

Date               Modification                                 Name
----               ------------                                 ----
2001-12-20          Creation                                    Michel Bruant

*******************************************************************************/

/* Includes ----------------------------------------------------------------- */

#include <string.h>

#include "stlayer.h"
#include "layerosd.h"
#include "stddefs.h"
#define RESERV
#include "osd_cm.h"
#undef RESERV
#include "osd_task.h"
#include "osdswcfg.h"
#include "stsys.h"
#include "layerapi.h"

/* Private Constants -------------------------------------------------------- */

#define OSD_TASK_STACK_SIZE     200
#define MAX_DEVICE              2 /* one layer recordable, the other not */
#define INVALID_DEVICE_INDEX    0xFFFF

/* Private Types ------------------------------------------------------------ */

typedef struct
{
    ST_DeviceName_t           DeviceName;
    U32                       Open;
    U32                       MaxOpen;
} stosd_Device_t;

/* Private Variables (static)------------------------------------------------ */

static stosd_Device_t   DeviceArray[MAX_DEVICE];
static semaphore_t      *FctAccessCtrl_p;
static U32              InitDone = 0;
static task_t *         Task_p;

/* Functions ---------------------------------------------------------------- */
/* static functions */
static void EnterCriticalSection(void);
static void LeaveCriticalSection(void);
static S32 GetIndexOfDeviceNamed(const ST_DeviceName_t WantedName);
static ST_ErrorCode_t Term(const STLAYER_TermParams_t* TermParams_p,U32 Index);
static ST_ErrorCode_t Init(const STLAYER_InitParams_t* InitParams_p);


/*******************************************************************************
Name        : LAYEROSD_Init
Description : Init a Layer
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYEROSD_Init(const ST_DeviceName_t DeviceName,
                             const STLAYER_InitParams_t* const InitParams_p)
{
    S32 Index = 0;
    ST_ErrorCode_t Err = ST_NO_ERROR;

    if ((strlen(DeviceName) > ST_MAX_DEVICE_NAME)
            ||(InitParams_p == NULL)
            ||(InitParams_p->CPUPartition_p == NULL))
    {
        layerosd_Defense(ST_ERROR_BAD_PARAMETER);
        return(ST_ERROR_BAD_PARAMETER);
    }

    EnterCriticalSection();

    /* First time: initialise devices as empty */
    if (!InitDone)
    {
        for (Index= 0; Index < MAX_DEVICE; Index++)
        {
            DeviceArray[Index].DeviceName[0] = '\0';
        }
    }

    /* Check if device already initialised and return error if so */
    if (GetIndexOfDeviceNamed(DeviceName) != INVALID_DEVICE_INDEX)
    {
        /* Device name found */
        Err = ST_ERROR_ALREADY_INITIALIZED;
    }
    else
    {
        /* Look for a non-initialised device */
        Index = 0;
        while((DeviceArray[Index].DeviceName[0] != '\0') && (Index <MAX_DEVICE))
        {
            Index++;
        }
        if (Index >= MAX_DEVICE)
        {
            /* All devices initialised */
            Err = ST_ERROR_NO_MEMORY;
        }
        else
        {
            /* API specific initialisations */
            Err = ST_NO_ERROR;
            if (InitDone == 0)
            {
                Err = Init(InitParams_p);
            }
            if (Err == ST_NO_ERROR)
            {
                /* Device available and successfully initialised: */
                strcpy(DeviceArray[Index].DeviceName, DeviceName);
                DeviceArray[Index].Open = 0;
                DeviceArray[Index].MaxOpen = InitParams_p->MaxHandles;
                InitDone ++;
            }
        }
    }

    LeaveCriticalSection();
    if(Err != ST_NO_ERROR)
    {
       layerosd_Defense(Err);
    }

    return(Err);
}
/*******************************************************************************
Name        : LAYEROSD_Term
Description : Term a Layer
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYEROSD_Term(const ST_DeviceName_t     DeviceName,
                             const STLAYER_TermParams_t* const TermParams_p)
{
    S32 DeviceIndex = INVALID_DEVICE_INDEX;
    ST_ErrorCode_t Err = ST_NO_ERROR;

    if (TermParams_p == NULL)
    {
        layerosd_Defense(ST_ERROR_BAD_PARAMETER);
        return (ST_ERROR_BAD_PARAMETER);
    }

    EnterCriticalSection();

    /* Check if device is initialised */
    DeviceIndex = GetIndexOfDeviceNamed(DeviceName);
    if (DeviceIndex == INVALID_DEVICE_INDEX)
    {
        /* Device name not found */
        layerosd_Defense(ST_ERROR_UNKNOWN_DEVICE);
        Err = ST_ERROR_UNKNOWN_DEVICE;
    }
    else
    {
        /* API specific terminations */
        Err = ST_NO_ERROR;
        if (InitDone == 1)
        {
            Err = Term(TermParams_p,DeviceIndex);
        }
        if (Err == ST_NO_ERROR)
        {
            /* Device found: desallocate memory, free device */
            DeviceArray[DeviceIndex].DeviceName[0] = '\0';
            InitDone --;
        }
    }

    LeaveCriticalSection();

    return(Err);
}

/*******************************************************************************
Name        : LAYEROSD_Open
Description : Open a Layer
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYEROSD_Open(const ST_DeviceName_t DeviceName,
                            const STLAYER_OpenParams_t * const  Params,
                            STLAYER_Handle_t *      Handle)
{
    ST_ErrorCode_t  Err;
    U32             Index;

    if(Params == NULL)
    {
        layerosd_Defense(ST_ERROR_BAD_PARAMETER);
        return(ST_ERROR_BAD_PARAMETER);
    }
    Index = GetIndexOfDeviceNamed(DeviceName);
    if (Index == INVALID_DEVICE_INDEX)
    {
        /* Device name not found */
        layerosd_Defense(ST_ERROR_UNKNOWN_DEVICE);
        Err = ST_ERROR_UNKNOWN_DEVICE;
        *Handle = 0;
    }
    else
    {
        if(DeviceArray[Index].Open < DeviceArray[Index].MaxOpen)
        {
            /* Device name OK */
            DeviceArray[Index].Open ++;
            Err  = ST_NO_ERROR;
            *Handle = Index;
        }
        else
        {
            layerosd_Defense(ST_ERROR_NO_FREE_HANDLES);
            return(ST_ERROR_NO_FREE_HANDLES);
        }
    }
    return(Err);
}
/*******************************************************************************
Name        : LAYEROSD_Close
Description : Close a Layer
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYEROSD_Close(STLAYER_Handle_t  Handle)
{
    if(DeviceArray[Handle].Open >= 1)
    {
        DeviceArray[Handle].Open --;
        return(ST_NO_ERROR);
    }
    else
    {
        layerosd_Defense(ST_ERROR_INVALID_HANDLE);
        return(ST_ERROR_INVALID_HANDLE);
    }
}
/*******************************************************************************
Name        : LAYEROSD_GetLayerParams
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYEROSD_GetLayerParams(STLAYER_Handle_t  Handle,
                                        STLAYER_LayerParams_t *  LayerParams_p)
{
     if(LayerParams_p == 0)
    {
        layerosd_Defense(ST_ERROR_BAD_PARAMETER);
        return(ST_ERROR_BAD_PARAMETER);
    }
   *LayerParams_p = stlayer_osd_context.LayerParams;
    return(ST_NO_ERROR);
}
/*******************************************************************************
Name        : LAYEROSD_SetLayerParams
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYEROSD_SetLayerParams(STLAYER_Handle_t  Handle,
                                        STLAYER_LayerParams_t *  LayerParams_p)
{
    stosd_ViewportDesc *    CurrentViewport_p;

    if(LayerParams_p == 0)
    {
        layerosd_Defense(ST_ERROR_BAD_PARAMETER);
        return(ST_ERROR_BAD_PARAMETER);
    }
    if ((LayerParams_p->Width > stlayer_osd_context.Width)
            || (LayerParams_p->Height > stlayer_osd_context.Height))
    {
        /* the layer overflow the output */
        layerosd_Defense(ST_ERROR_BAD_PARAMETER);
        return(ST_ERROR_BAD_PARAMETER);
    }
    /* scan the viewports */
    CurrentViewport_p = stlayer_osd_context.FirstViewPortLinked;
    while(CurrentViewport_p != 0)
    {
        if((CurrentViewport_p->OutputRectangle.Height
                    + CurrentViewport_p->OutputRectangle.PositionY
                            > LayerParams_p->Height)
            ||(CurrentViewport_p->OutputRectangle.Width
                    + CurrentViewport_p->OutputRectangle.PositionX
                            >   LayerParams_p->Width))
        {
            /* the viewport overflow the layer */
            layerosd_Defense(STLAYER_ERROR_OUT_OF_LAYER);
            return(STLAYER_ERROR_OUT_OF_LAYER);
        }
        CurrentViewport_p = CurrentViewport_p->Next_p;
    }

    stlayer_osd_context.LayerParams = *LayerParams_p;
    return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : LAYEROSD_GetCapability
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYEROSD_GetCapability(const ST_DeviceName_t DeviceName,
                               STLAYER_Capability_t * const Capability_p)
{
    S32 DeviceIndex = INVALID_DEVICE_INDEX;
    ST_ErrorCode_t Err = ST_NO_ERROR;

    if (Capability_p == NULL)
    {
        layerosd_Defense(ST_ERROR_BAD_PARAMETER);
        return (ST_ERROR_BAD_PARAMETER);
    }

    EnterCriticalSection();

    /* Check if device already initialised and return error if NOT so */
    DeviceIndex = GetIndexOfDeviceNamed(DeviceName);
    if (DeviceIndex == INVALID_DEVICE_INDEX)
    {
        /* Device name not found */
        layerosd_Defense(ST_ERROR_UNKNOWN_DEVICE);
        Err = ST_ERROR_UNKNOWN_DEVICE;
    }
    else
    {
        Capability_p->MultipleViewPorts                  = TRUE;
        Capability_p->HorizontalResizing                 = FALSE;
        Capability_p->AlphaBorder                        = FALSE;
        Capability_p->GlobalAlpha                        = TRUE;
        Capability_p->ColorKeying                        = FALSE;
        Capability_p->MultipleViewPortsOnScanLineCapable = FALSE;
        Capability_p->LayerType                          = STLAYER_OMEGA1_OSD;
    }

    LeaveCriticalSection();

    return(Err);
}
/*******************************************************************************
Name        : LAYEROSD_AdjustIORectangle
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/

ST_ErrorCode_t LAYEROSD_AdjustIORectangle(STLAYER_Handle_t      Handle,
                                         STGXOBJ_Rectangle_t * InputRectangle,
                                         STGXOBJ_Rectangle_t * OutputRectangle)
{
    ST_ErrorCode_t               Error;

    Error = ST_NO_ERROR;

    /* Selection tests */
    if(OutputRectangle->PositionX > ((S32)stlayer_osd_context.LayerParams.Width))
    {
       return(STLAYER_ERROR_IORECTANGLES_NOT_ADJUSTABLE);
    }
    if(OutputRectangle->PositionY > ((S32)stlayer_osd_context.LayerParams.Height))
    {
       return(STLAYER_ERROR_IORECTANGLES_NOT_ADJUSTABLE);
    }

    /* Adjust Input Rectangle */
    if (InputRectangle->PositionX < 0)
    {
        InputRectangle->PositionX = 0;
        Error = STLAYER_SUCCESS_IORECTANGLES_ADJUSTED;
    }
    if (InputRectangle->PositionY < 0)
    {
        InputRectangle->PositionY = 0;
        Error = STLAYER_SUCCESS_IORECTANGLES_ADJUSTED;
    }
    /* Adjust Output Rectangle */
    if (OutputRectangle->PositionX < 0)
    {
        OutputRectangle->PositionX = 0;
        Error = STLAYER_SUCCESS_IORECTANGLES_ADJUSTED;
    }
    if (OutputRectangle->PositionY < 0)
    {
        OutputRectangle->PositionY = 0;
        Error = STLAYER_SUCCESS_IORECTANGLES_ADJUSTED;
    }
    if(OutputRectangle->PositionX + OutputRectangle->Width
                                > stlayer_osd_context.LayerParams.Width)
    {
        OutputRectangle->Width = stlayer_osd_context.LayerParams.Width
                                                - OutputRectangle->PositionX ;
        Error = STLAYER_SUCCESS_IORECTANGLES_ADJUSTED;
    }
    if(OutputRectangle->PositionY + OutputRectangle->Height
                                > stlayer_osd_context.LayerParams.Height)
    {
        OutputRectangle-> Height = stlayer_osd_context.LayerParams.Height
                                                - OutputRectangle->PositionY ;
        Error = STLAYER_SUCCESS_IORECTANGLES_ADJUSTED;
    }
    /* Adjust Resize */
    /* test Vresize */
    if ( OutputRectangle->Height > 2 * InputRectangle->Height)
    {
        OutputRectangle->Height = 2 * InputRectangle->Height;
        Error = STLAYER_SUCCESS_IORECTANGLES_ADJUSTED;
    }
    else if (InputRectangle->Height > 2 * OutputRectangle->Height)
    {
        InputRectangle->Height = 2 * OutputRectangle->Height;
        Error = STLAYER_SUCCESS_IORECTANGLES_ADJUSTED;
    }
    else if (InputRectangle->Height != OutputRectangle->Height)
    {
        Error = STLAYER_SUCCESS_IORECTANGLES_ADJUSTED;
        if (InputRectangle->Height > OutputRectangle->Height)
        {
            InputRectangle->Height = OutputRectangle->Height;
        }
        else
        {
            OutputRectangle->Height = InputRectangle->Height;
        }
    }
    /* Test Hresize */
    if ( OutputRectangle->Width > 2 * InputRectangle->Width)
    {
        OutputRectangle->Width = 2 * InputRectangle->Width;
        Error = STLAYER_SUCCESS_IORECTANGLES_ADJUSTED;
    }
    else if (InputRectangle->Width != OutputRectangle->Width)
    {
        Error = STLAYER_SUCCESS_IORECTANGLES_ADJUSTED;
        if (InputRectangle->Width > OutputRectangle->Width)
        {
            InputRectangle->Width = OutputRectangle->Width;
        }
        else
        {
            OutputRectangle->Width = InputRectangle->Width;
        }
    }

    return(Error);

}
/*******************************************************************************
Name        : EnterCriticalSection
Description : Used to protect critical sections
Parameters  : None
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void EnterCriticalSection(void)
{
    static BOOL FctAccessCtrlInitialized = FALSE;

    interrupt_lock();
    if (!FctAccessCtrlInitialized)
    {
        /* Initialise the  protection semaphore */
        FctAccessCtrl_p = semaphore_create_fifo(1);
        FctAccessCtrlInitialized = TRUE;
    }
    interrupt_unlock();

    /* Wait for the  protection semaphore */
    semaphore_wait(FctAccessCtrl_p);
}
/*******************************************************************************
Name        : LeaveCriticalSection
Description : Used to release protection of critical sections
Parameters  : None
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void LeaveCriticalSection(void)
{
    /* Release the  protection semaphore */
    semaphore_signal(FctAccessCtrl_p);
}
/*******************************************************************************
Name        : Init
Description : API specific initialisations
Parameters  : pointer on device and initialisation parameters
Assumptions : pointer on device is not NULL
Limitations :
Returns     : ST_ErrorCode_t for the API Init function
*******************************************************************************/
static ST_ErrorCode_t Init(const STLAYER_InitParams_t* InitParams_p)
{
    STAVMEM_MemoryRange_t       ForbiddenArea[1];
    STAVMEM_AllocBlockParams_t  AllocBlockParams;
    STEVT_OpenParams_t          OpenParams;
    ST_ErrorCode_t              Error;
    U32                         HeaderSize;
    void *                      VirtualAddress;
    ST_DeviceName_t Empty = "\0\0";

    /* Init of  globals */
    stlayer_osd_context.DriverTermination = FALSE;
    stlayer_osd_context.stosd_AVMEMPartition    = InitParams_p->AVMEM_Partition;
    stlayer_osd_context.CPUPartition_p          = InitParams_p->CPUPartition_p;
    stlayer_osd_context.MaxViewPorts            = InitParams_p->MaxViewPorts;
    stlayer_osd_context.stosd_BaseAddress       = (U32)InitParams_p
                                    ->BaseAddress_p
                                    + (U32)InitParams_p->DeviceBaseAddress_p;
    strcpy(stlayer_osd_context.EvtHandlerName,InitParams_p->EventHandlerName);
    strcpy(stlayer_osd_context.VTGName,Empty);
    stlayer_osd_context.LayerParams             = *InitParams_p->LayerParams_p;
    STAVMEM_GetSharedMemoryVirtualMapping(&stlayer_osd_context.VirtualMapping);

    /* default output (should be overwrite by mixer) */
    stlayer_osd_context.XOffset                 = 0;
    stlayer_osd_context.YOffset                 = 0;
    stlayer_osd_context.XStart                  = 112;
    stlayer_osd_context.YStart                  = 20+20; /* top + bot */
    stlayer_osd_context.Width       = stlayer_osd_context.LayerParams.Width;
    stlayer_osd_context.Height      = stlayer_osd_context.LayerParams.Height;
    stlayer_osd_context.LayerRecordable[0]      = TRUE;
    stlayer_osd_context.LayerRecordable[1]      = FALSE;
    stlayer_osd_context.OutputEnabled           = FALSE;
    stlayer_osd_context.EnableRGB16Mode         = FALSE;
    stlayer_osd_context.NumViewPortLinked       = 0;
    stlayer_osd_context.NumViewPortEnabled      = 0;

    /* Memory allocation for viewports */
    if (!(InitParams_p->ViewPortBufferUserAllocated))
    {
        stlayer_osd_context.OSD_ViewPort = (stosd_ViewportDesc *)
              memory_allocate(stlayer_osd_context.CPUPartition_p,
                stlayer_osd_context.MaxViewPorts  * sizeof(stosd_ViewportDesc));
        stlayer_osd_context.ViewPortsUserAllocated = FALSE;
    }
    else
    {
        stlayer_osd_context.OSD_ViewPort = (stosd_ViewportDesc *)
                                                 InitParams_p->ViewPortBuffer_p;
        stlayer_osd_context.ViewPortsUserAllocated = TRUE;
    }
    /* Memory allocation for headers (+ palettes) */
    HeaderSize =  sizeof(STGXOBJ_ColorUnsignedAYCbCr_t) * 256;
    HeaderSize += sizeof(osd_RegionHeader_t);
    if(HeaderSize % 256 != 0)
    {
        HeaderSize = HeaderSize + 256 - (HeaderSize % 256);
    }
    if (!(InitParams_p->NodeBufferUserAllocated))
    {
        U32 Size;
        Size = HeaderSize * 2 * stlayer_osd_context.MaxViewPorts
                    + sizeof(osd_RegionHeader_t); /* for ghost header */
        ForbiddenArea[0].StartAddr_p = (void *)
               ((U32)(stlayer_osd_context.VirtualMapping.VirtualBaseAddress_p));
        ForbiddenArea[0].StopAddr_p  =  (void *)
                ((U32)(stlayer_osd_context.VirtualMapping.VirtualBaseAddress_p)
                 + 3);
        ForbiddenArea[1].StartAddr_p = (void *)
                ((U32)(stlayer_osd_context.VirtualMapping.VirtualBaseAddress_p)
                + (U32)(stlayer_osd_context.VirtualMapping.VirtualWindowOffset)
                + (U32)(stlayer_osd_context.VirtualMapping.VirtualWindowSize));
        ForbiddenArea[1].StopAddr_p  =  (void *)
                ((U32)(stlayer_osd_context.VirtualMapping.VirtualBaseAddress_p)
                + (U32)(stlayer_osd_context.VirtualMapping.VirtualSize));
        AllocBlockParams.PartitionHandle
                                     = stlayer_osd_context.stosd_AVMEMPartition;
        AllocBlockParams.Size        = Size;
        AllocBlockParams.Alignment   = 256;
        AllocBlockParams.AllocMode   = STAVMEM_ALLOC_MODE_BOTTOM_TOP;
        AllocBlockParams.NumberOfForbiddenRanges  = 2;
        AllocBlockParams.ForbiddenRangeArray_p    = &ForbiddenArea[0];
        AllocBlockParams.NumberOfForbiddenBorders = 0;
        AllocBlockParams.ForbiddenBorderArray_p   = NULL;
        Error = STAVMEM_AllocBlock(&AllocBlockParams,
                                          &(stlayer_osd_context.HeadersHandle));
        Error = STAVMEM_GetBlockAddress(stlayer_osd_context.HeadersHandle,
                                            (void**)&VirtualAddress);
        stlayer_osd_context.HeadersBuffer = (osd_RegionHeader_t *)
               STAVMEM_VirtualToCPU(VirtualAddress,
                                    &stlayer_osd_context.VirtualMapping);
        stlayer_osd_context.HeadersUserAllocated = FALSE;
    }
    else
    {
        stlayer_osd_context.HeadersBuffer = (osd_RegionHeader_t *)
                      STAVMEM_VirtualToCPU(InitParams_p->ViewPortNodeBuffer_p,
                                          &stlayer_osd_context.VirtualMapping);
        stlayer_osd_context.HeadersUserAllocated = TRUE;
    }

    /* Link the viewports to their headers */
    {
        U8 * Pointer;
        U32 i;
        Pointer = (U8*)stlayer_osd_context.HeadersBuffer;
        for(i=0; i< stlayer_osd_context.MaxViewPorts; i++)
        {
            stlayer_osd_context.OSD_ViewPort[i].Top_header
                            = (osd_RegionHeader_t *)Pointer;
            Pointer = Pointer + HeaderSize;
            stlayer_osd_context.OSD_ViewPort[i].Bot_header
                            = (osd_RegionHeader_t *)Pointer;
            Pointer = Pointer + HeaderSize;
            stlayer_osd_context.OSD_ViewPort[i].Enabled = 0;
            stlayer_osd_context.OSD_ViewPort[i].Open    = 0;
        }
        stlayer_osd_context.GhostHeader_p = (osd_RegionHeader_t *) Pointer;
    }
    /* semaphore init */
    stlayer_osd_context.stosd_VSync_p = semaphore_create_fifo(0);
    stlayer_osd_context.VPDecriptorAccess_p = semaphore_create_fifo(1);

    /* Create Task */
     Task_p = task_create((void(*)(void*))stlayer_ProcessEvtVSync,NULL,
            OSD_TASK_STACK_SIZE,STLAYER_OSDFRAME_TASK_PRIORITY , "STLAYER_OsdFrame",task_flags_no_min_stack_size );
    /*  Event */
    STEVT_Open(stlayer_osd_context.EvtHandlerName, &OpenParams,
          &(stlayer_osd_context.EvtHandle));

    STSYS_WriteRegMem32BE(&stlayer_osd_context.GhostHeader_p->word1,0xFFFFFFFF);
    STSYS_WriteRegMem32BE(&stlayer_osd_context.GhostHeader_p->word2,0xFFFFFFFF);
    STSYS_WriteRegMem32BE(&stlayer_osd_context.GhostHeader_p->word3,0xFFFFFFFF);
    STSYS_WriteRegMem32BE(&stlayer_osd_context.GhostHeader_p->word4,0xFFFFFFFF);
    stlayer_HardInit();
    return(ST_NO_ERROR);
}
/*******************************************************************************
Name        : Term
Description : API specific terminations
Parameters  : pointer on device and termination parameters
Assumptions : pointer on device is not NULL
Limitations :
Returns     : ST_ErrorCode_t for the API Term function
*******************************************************************************/
static ST_ErrorCode_t Term(const STLAYER_TermParams_t* TermParams_p,U32 Index)
{
    ST_ErrorCode_t Err = ST_NO_ERROR;
    STAVMEM_FreeBlockParams_t   FreeParams;

    if (!(TermParams_p->ForceTerminate))
    {
        if (DeviceArray[Index].Open > 0)
        {
            layerosd_Defense(ST_ERROR_BAD_PARAMETER);
            return(ST_ERROR_BAD_PARAMETER); /* error still opened */
        }
        if (stlayer_osd_context.NumViewPortLinked > 0)
        {
            layerosd_Defense(ST_ERROR_BAD_PARAMETER);
            return(ST_ERROR_BAD_PARAMETER); /* error still opened */
        }
    }
    /* Force Open to be abble to close all viewport */
    DeviceArray[Index].Open ++;
    while(stlayer_osd_context.FirstViewPortLinked != 0)
    {
        LAYEROSD_DisableViewPort((STLAYER_ViewPortHandle_t)
                                stlayer_osd_context.FirstViewPortLinked);
    }
    /* Close all the sessions */
    while (DeviceArray[Index].Open > 0)
    {
        /* Close it */
        LAYEROSD_Close(Index);
    }

    /* Indicates termination to tasks
     * wait for end of task  before deleting it*/
    stlayer_osd_context.DriverTermination = TRUE;

    Err = STEVT_Close(stlayer_osd_context.EvtHandle);
    if (Err != ST_NO_ERROR)
    {
        layerosd_Defense(Err);
    }

    /* delete the task */
    semaphore_signal(stlayer_osd_context.stosd_VSync_p);
    task_wait(&Task_p,1,TIMEOUT_INFINITY);
    task_delete(Task_p);

    /* semaphore delete */
    semaphore_delete(stlayer_osd_context.stosd_VSync_p);
    semaphore_delete(stlayer_osd_context.VPDecriptorAccess_p);

    /* deallocate memory */
    if (!(stlayer_osd_context.ViewPortsUserAllocated))
    {
        memory_deallocate(stlayer_osd_context.CPUPartition_p,
                            (void*)stlayer_osd_context.OSD_ViewPort);
        stlayer_osd_context.OSD_ViewPort = 0;
    }
    FreeParams.PartitionHandle = stlayer_osd_context.stosd_AVMEMPartition;
    if (!(stlayer_osd_context.HeadersUserAllocated))
    {
        STAVMEM_FreeBlock(&FreeParams,&stlayer_osd_context.HeadersHandle);
        stlayer_osd_context.GhostHeader_p = 0;
        stlayer_osd_context.HeadersBuffer = 0;
    }

    return(Err);
}
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
        if (strcmp(DeviceArray[Index].DeviceName, WantedName ) == 0)
        {
            /* Name found in the initialised devices */
            WantedIndex = Index;
        }
        Index++;
    } while ((WantedIndex == INVALID_DEVICE_INDEX) && (Index < MAX_DEVICE));

    return(WantedIndex);
}

/*******************************************************************************
Name        : LAYEROSD_UpdateFromMixer
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYEROSD_UpdateFromMixer(STLAYER_Handle_t         LayerHandle,
                                       STLAYER_OutputParams_t * OutputParams_p)
{
    /* Connect a VTG synchro */
    if ((OutputParams_p->UpdateReason & STLAYER_VTG_REASON) != 0)
    {
        stlayer_SubscribeEvents(OutputParams_p->VTGName);
    }
    /* Disonnect a VTG synchro */
    if ((OutputParams_p->UpdateReason & STLAYER_DISCONNECT_REASON) != 0)
    {
        stlayer_SubscribeEvents(0);
    }
    /* Update Offset params */
    if ((OutputParams_p->UpdateReason & STLAYER_OFFSET_REASON) != 0)
    {
       stlayer_osd_context.XOffset = OutputParams_p->XOffset;
       stlayer_osd_context.YOffset = OutputParams_p->YOffset;
    }
    /* Update screen params */
    if ((OutputParams_p->UpdateReason & STLAYER_SCREEN_PARAMS_REASON) != 0)
    {
       if (OutputParams_p->Width < stlayer_osd_context.LayerParams.Width)
       {
            layerosd_Trace("LAYER - OSD : Output Width is too small. \n");
            layerosd_Defense(STLAYER_ERROR_INSIDE_LAYER);
            return(STLAYER_ERROR_INSIDE_LAYER);
       }
       if (OutputParams_p->Height < stlayer_osd_context.LayerParams.Height)
       {
            layerosd_Trace("LAYER - OSD : Output Height is too small. \n");
            layerosd_Defense(STLAYER_ERROR_INSIDE_LAYER);
            return(STLAYER_ERROR_INSIDE_LAYER);
       }
       stlayer_osd_context.Width  = OutputParams_p->Width;
       stlayer_osd_context.Height = OutputParams_p->Height;
       stlayer_osd_context.XStart = OutputParams_p->XStart;
       stlayer_osd_context.YStart = OutputParams_p->YStart  /* top lines */
                                  + OutputParams_p->YStart; /* bot lines */
       stlayer_osd_context.FrameRate = OutputParams_p->FrameRate;
    }
    /* Change the pipeline */
    if ((OutputParams_p->UpdateReason & STLAYER_CHANGE_ID_REASON) != 0)
    {
        /* dont care */
    }
    /* Enable / Disable the display */
    if ((OutputParams_p->UpdateReason & STLAYER_DISPLAY_REASON) != 0)
    {
        stlayer_osd_context.OutputEnabled = OutputParams_p->DisplayEnable;
        if((stlayer_osd_context.OutputEnabled)
                &&(stlayer_osd_context.NumViewPortEnabled !=0))
        {
            stlayer_HardEnableDisplay();
        }
        else
        {
            stlayer_HardDisableDisplay();
        }
    }

    /* dynamic output params change */
    stlayer_BuildHardImage();

    return(ST_NO_ERROR);
}

/* end of osd_cm.c */
