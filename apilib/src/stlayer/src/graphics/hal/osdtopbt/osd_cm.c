/*******************************************************************************

File name   : osd_cm.c

Description : It provides the OSD initialization and termination

COPYRIGHT (C) STMicroelectronics 2004

Date               Modification                                 Name
----               ------------                                 ----
2001-03-13          Creation                                    Michel Bruant

*******************************************************************************/

/* Includes ----------------------------------------------------------------- */

#include <string.h>
#include "stlayer.h"
#include "layerapi.h"
#include "layerosd.h"
#include "stddefs.h"
#define RESERV
#include "osd_hal.h"
#undef RESERV
#include "osd_task.h"
#include "stvtg.h"
#include "osdswcfg.h"
#include "stsys.h"

/* Private Constants -------------------------------------------------------- */

#define OSD_TASK_STACK_SIZE     250
#define MAX_DEVICE              2 /* one layer recordable, the other not */
#define INVALID_DEVICE_INDEX    0xFFFF

/* Private Types ------------------------------------------------------------ */

typedef struct
{
    ST_DeviceName_t           DeviceName;
    U32                       Open;
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
static ST_ErrorCode_t Term(U32 Index,const STLAYER_TermParams_t* TermParams_p);
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
    if (InitDone == 0)
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
                InitDone ++;
            }
        }
    }

    LeaveCriticalSection();

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
        /* API specific terminations */
        Err = ST_NO_ERROR;
        if (InitDone == 1)
        {
            Err = Term(DeviceIndex,TermParams_p);
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
        /* Device name OK */
        if(DeviceArray[Index].Open == stlayer_osd_context.MaxHandles)
        {
            layerosd_Defense(ST_ERROR_NO_FREE_HANDLES);
            return(ST_ERROR_NO_FREE_HANDLES);
        }
        DeviceArray[Index].Open ++;
        Err  = ST_NO_ERROR;
        *Handle = Index;
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

    if (LayerParams_p == 0)
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
    CurrentViewport_p = stlayer_osd_context.FirstViewPortEnabled;
    while(CurrentViewport_p != 0)
    {
        if((CurrentViewport_p->Height
                    + CurrentViewport_p->PositionY
                            > LayerParams_p->Height)
            ||(CurrentViewport_p->Width
                    + CurrentViewport_p->PositionX
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
Name        : EnterCriticalSection
Description : Used to protect critical sections of Init/Term
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
        /* Initialise the Init/Term protection semaphore */
        FctAccessCtrl_p = semaphore_create_fifo(1);
        FctAccessCtrlInitialized = TRUE;
    }
    interrupt_unlock();

    /* Wait for the Init/Term protection semaphore */
    semaphore_wait(FctAccessCtrl_p);
}
/*******************************************************************************
Name        : LeaveCriticalSection
Description : Used to release protection of critical sections of Init/Term
Parameters  : None
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void LeaveCriticalSection(void)
{
    /* Release the Init/Term protection semaphore */
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
    U8 Value;
    STAVMEM_MemoryRange_t       ForbiddenArea[1];
    STAVMEM_AllocBlockParams_t  AllocBlockParams;
    U32                         HeaderSize;
    STEVT_OpenParams_t          OpenParams;
    void *                      VirtualAddress;
    ST_ErrorCode_t              Error;

    /* Init Private variables */
    stlayer_osd_context.DriverTermination = FALSE;

    /* Init of  globals */
    stlayer_osd_context.stosd_AVMEMPartition   = InitParams_p->AVMEM_Partition;
    stlayer_osd_context.CPUPartition_p         = InitParams_p->CPUPartition_p;
    stlayer_osd_context.MaxViewPorts           = InitParams_p->MaxViewPorts;
    stlayer_osd_context.MaxHandles             = InitParams_p->MaxHandles;
    stlayer_osd_context.Width                  = InitParams_p->LayerParams_p->Width;
    stlayer_osd_context.Height                 = InitParams_p->LayerParams_p->Height;
    stlayer_osd_context.MaxViewPorts           = InitParams_p->MaxViewPorts;
    stlayer_osd_context.stosd_BaseAddress      = (U32)InitParams_p->BaseAddress_p
                                       + (U32)InitParams_p->DeviceBaseAddress_p;
    strcpy(stlayer_osd_context.EvtHandlerName,InitParams_p->EventHandlerName);
    STEVT_Open(stlayer_osd_context.EvtHandlerName, &OpenParams, &(stlayer_osd_context.EvtHandle));
    stlayer_osd_context.LayerParams           = *(InitParams_p->LayerParams_p);
    /* Default values for output: */
    stlayer_osd_context.XOffset                        = 0;
    stlayer_osd_context.YOffset                        = 0;
    stlayer_osd_context.XStart                         = 120;
    stlayer_osd_context.YStart                         = 20+20; /* top + bot */
    STAVMEM_GetSharedMemoryVirtualMapping(&stlayer_osd_context.VirtualMapping);
    stlayer_osd_context.ContextAcess_p = semaphore_create_fifo(1);
    /* Memory allocation for viewports */
    if (!(InitParams_p->ViewPortBufferUserAllocated))
    {
        stlayer_osd_context.OSD_ViewPort = (stosd_ViewportDesc *)
              memory_allocate(stlayer_osd_context.CPUPartition_p,
                        stlayer_osd_context.MaxViewPorts * sizeof(stosd_ViewportDesc));
        stlayer_osd_context.ViewPortsUserAllocated = FALSE;
    }
    else
    {
        stlayer_osd_context.OSD_ViewPort = (stosd_ViewportDesc *) InitParams_p->ViewPortBuffer_p;
        stlayer_osd_context.ViewPortsUserAllocated = TRUE;
    }
    /* Memory allocation for palettes headers */
    /* (bitmap headers are allocated IN the bitmap) */
    HeaderSize  = sizeof(STGXOBJ_ColorUnsignedAYCbCr_t) * 256;
    HeaderSize += sizeof(osd_RegionHeader_t);
    if(HeaderSize % 256 != 0)
    {   /* alignment */
        HeaderSize = HeaderSize + 256 - (HeaderSize % 256);
    }

    if (!(InitParams_p->NodeBufferUserAllocated))
    {
        U32 Size;
        Size = HeaderSize * 2 * stlayer_osd_context.MaxViewPorts; /* top+bot */
        ForbiddenArea[0].StartAddr_p = (void *)
               ((U32)(stlayer_osd_context.VirtualMapping.VirtualBaseAddress_p));
        ForbiddenArea[0].StopAddr_p  =  (void *)
            ((U32)(stlayer_osd_context.VirtualMapping.VirtualBaseAddress_p)+ 3);
        ForbiddenArea[1].StartAddr_p = (void *)
                ((U32)(stlayer_osd_context.VirtualMapping.VirtualBaseAddress_p)
                + (U32)(stlayer_osd_context.VirtualMapping.VirtualWindowOffset)
                + (U32)(stlayer_osd_context.VirtualMapping.VirtualWindowSize));
        ForbiddenArea[1].StopAddr_p  =  (void *)
                ((U32)(stlayer_osd_context.VirtualMapping.VirtualBaseAddress_p)
                + (U32)(stlayer_osd_context.VirtualMapping.VirtualSize));
        AllocBlockParams.PartitionHandle
                                    = stlayer_osd_context.stosd_AVMEMPartition;
        AllocBlockParams.Size                   = Size;
        AllocBlockParams.Alignment              = 256;
        AllocBlockParams.AllocMode              = STAVMEM_ALLOC_MODE_BOTTOM_TOP;
        AllocBlockParams.NumberOfForbiddenRanges= 2;
        AllocBlockParams.ForbiddenRangeArray_p    = &ForbiddenArea[0];
        AllocBlockParams.NumberOfForbiddenBorders = 0;
        AllocBlockParams.ForbiddenBorderArray_p   = NULL;
        Error =STAVMEM_AllocBlock(&AllocBlockParams,
                                         &(stlayer_osd_context.HeadersHandle));
        Error = STAVMEM_GetBlockAddress(stlayer_osd_context.HeadersHandle,
                                            (void**)&VirtualAddress);
        stlayer_osd_context.HeadersBuffer = (osd_RegionHeader_t *)
               STAVMEM_VirtualToCPU(VirtualAddress
                       ,&stlayer_osd_context.VirtualMapping);
        stlayer_osd_context.HeadersUserAllocated = FALSE;
    }
    else
    {
        stlayer_osd_context.HeadersBuffer = (osd_RegionHeader_t *)
                      STAVMEM_VirtualToCPU(InitParams_p->ViewPortNodeBuffer_p,
                                        &stlayer_osd_context.VirtualMapping);
        stlayer_osd_context.HeadersUserAllocated = TRUE;
    }

    /* Link the viewports to theire palette headers */
    {
        U8 * Pointer;
        U32 i;
        Pointer = (U8*)stlayer_osd_context.HeadersBuffer;
        for(i=0; i< stlayer_osd_context.MaxViewPorts; i++)
        {
            stlayer_osd_context.OSD_ViewPort[i].PaletteFirstHeader_p
                                        = (osd_RegionHeader_t *)Pointer;
            Pointer = Pointer + HeaderSize;
            stlayer_osd_context.OSD_ViewPort[i].PaletteSecondHeader_p
                                        = (osd_RegionHeader_t *)Pointer;
            Pointer = Pointer + HeaderSize;
        }
    }
    /* declare the viexports closed */
    for (Value = 0; Value < stlayer_osd_context.MaxViewPorts; Value ++)
    {
        stlayer_osd_context.OSD_ViewPort[Value].Open = FALSE;
    }

    /* semaphore init */
    stlayer_osd_context.stosd_VSync_p = semaphore_create_fifo(0);

    /* Create Task */
    Task_p = task_create((void(*)(void*))stlayer_UpdateHeadersTask,NULL,
            OSD_TASK_STACK_SIZE, STLAYER_OSDTOPBOT_TASK_PRIORITY ,
            "STLAYER_OsdTopBot",task_flags_no_min_stack_size );

    /* Hardware Init */
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
static ST_ErrorCode_t Term(U32 Index,const STLAYER_TermParams_t* TermParams_p)
{
    ST_ErrorCode_t Err = ST_NO_ERROR;
    ST_DeviceName_t Empty = "\0\0";

    /* Force Open to be abble to close all viewport */
    DeviceArray[Index].Open ++;
    while(stlayer_osd_context.FirstViewPortEnabled != 0)
    {
        LAYEROSD_DisableViewPort((STLAYER_ViewPortHandle_t)
                                stlayer_osd_context.FirstViewPortEnabled);
    }
    /* Close all the sessions */
    while (DeviceArray[Index].Open > 0)
    {
        /* Close it */
        LAYEROSD_Close(Index);
    }
    if(strcmp(stlayer_osd_context.VTGName,Empty) != 0 )
    {
        /* A synchro was already in effect */
        /* -> No more synchro */
        STEVT_UnsubscribeDeviceEvent(stlayer_osd_context.EvtHandle,
                                     stlayer_osd_context.VTGName,
                                     STVTG_VSYNC_EVT);
    }

    /* Indicates termination to  "UpdateOsdRegion"  tasks
     * wait for end of task  before deleting it*/
    stlayer_osd_context.DriverTermination = TRUE;

    Err = STEVT_Close(stlayer_osd_context.EvtHandle);
    if (Err != ST_NO_ERROR)
    {
        layerosd_Defense(Err);
    }

    semaphore_signal(stlayer_osd_context.stosd_VSync_p);
    task_wait(&Task_p,1,TIMEOUT_INFINITY);
    task_delete(Task_p);

    /* semaphore delete */
    semaphore_delete(stlayer_osd_context.stosd_VSync_p);
    semaphore_delete(stlayer_osd_context.ContextAcess_p);

    /* memory */
    if (!(stlayer_osd_context.ViewPortsUserAllocated))
    {
        memory_deallocate(stlayer_osd_context.CPUPartition_p,(void *)
                            stlayer_osd_context.OSD_ViewPort);
    }
    if (!(stlayer_osd_context.HeadersUserAllocated))
    {
        STAVMEM_FreeBlockParams_t   FreeParams;
        FreeParams.PartitionHandle = stlayer_osd_context.stosd_AVMEMPartition;
        STAVMEM_FreeBlock(&FreeParams,&(stlayer_osd_context.HeadersHandle));
    }

    stlayer_HardTerm();

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

/* end of osd_cm.c */
