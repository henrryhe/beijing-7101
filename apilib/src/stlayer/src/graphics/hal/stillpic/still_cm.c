/*******************************************************************************

File name   : still_cm.c
Description : It provides the STILL initialization and termination

COPYRIGHT (C) STMicroelectronics 2004

Date               Modification                                 Name
----               ------------                                 ----


*******************************************************************************/

/* Includes ----------------------------------------------------------------- */

#include <string.h>

#include "stlayer.h"
#include "layerapi.h"
#include "stddefs.h"
#include "lay_stil.h"
#define VARIABLE_STILL_RESERVATION
#include "still_cm.h"
#undef VARIABLE_STILL_RESERVATION
#include "stillhal.h"

/* Private Constants -------------------------------------------------------- */

#define INVALID_DEVICE_INDEX    0xFFFF
#define MAX_DEVICE              1

/* Private Types ------------------------------------------------------------ */

typedef struct
{
    ST_DeviceName_t           DeviceName;
    U32                       Open;
} still_Device_t;

/* Private Variables (static)------------------------------------------------ */

static still_Device_t   DeviceArray[MAX_DEVICE];
static U32              InitDone = 0;
static BOOL             AccessProtectInit;

/* Macros ------------------------------------------------------------------- */

#define TestLayHandle(Handle)           \
{                                       \
    if(Handle != 0)                     \
    {                                   \
        return(ST_ERROR_INVALID_HANDLE);\
    }                                   \
}

/* Functions ---------------------------------------------------------------- */

static S32 GetIndexOfDeviceNamed(const ST_DeviceName_t WantedName);
static ST_ErrorCode_t Term(const STLAYER_TermParams_t* TermParams_p,U32 Index);
static ST_ErrorCode_t Init(const STLAYER_InitParams_t* InitParams_p);

/*
--------------------------------------------------------------------------------
Initialise still API
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t LAYERSTILL_Init(const ST_DeviceName_t DeviceName,
                             const STLAYER_InitParams_t* const InitParams_p)
{
    S32 Index = 0;
    ST_ErrorCode_t Err = ST_NO_ERROR;

    if ((strlen(DeviceName) > ST_MAX_DEVICE_NAME)
            ||(InitParams_p == NULL)
            ||(InitParams_p->CPUPartition_p == NULL))
    {
        layerstill_Defense(ST_ERROR_BAD_PARAMETER);
        return(ST_ERROR_BAD_PARAMETER);
    }

    interrupt_lock();
    if (!AccessProtectInit)
    {
        /* Initialise the Init/Term protection semaphore */
        stlayer_still_context.AccessProtect_p = semaphore_create_fifo(1);
        AccessProtectInit = TRUE;
    }
    interrupt_unlock();
    semaphore_wait(stlayer_still_context.AccessProtect_p);

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
                InitDone ++;
            }
        }
    }

    semaphore_signal(stlayer_still_context.AccessProtect_p);

    if (Err != ST_NO_ERROR)
    {
        layerstill_Defense(Err);
    }
    return(Err);
}
/*
--------------------------------------------------------------------------------
Terminate still API
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t LAYERSTILL_Term(const ST_DeviceName_t     DeviceName,
                             const STLAYER_TermParams_t* const TermParams_p)
{
    S32 DeviceIndex = INVALID_DEVICE_INDEX;
    ST_ErrorCode_t Err = ST_NO_ERROR;

    if (TermParams_p == NULL)
    {
        layerstill_Defense(ST_ERROR_BAD_PARAMETER);
        return (ST_ERROR_BAD_PARAMETER);
    }

    semaphore_wait(stlayer_still_context.AccessProtect_p);

    /* Check if device already initialised and return error if NOT so */
    DeviceIndex = GetIndexOfDeviceNamed(DeviceName);
    if (DeviceIndex == INVALID_DEVICE_INDEX)
    {
        /* Device name not found */
        layerstill_Defense(ST_ERROR_UNKNOWN_DEVICE);
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

    semaphore_signal(stlayer_still_context.AccessProtect_p);

    return(Err);
}
/******************************************************************************/

ST_ErrorCode_t LAYERSTILL_Open(const ST_DeviceName_t DeviceName,
                            const STLAYER_OpenParams_t * const  Params,
                            STLAYER_Handle_t *      Handle)
{
    ST_ErrorCode_t  Err;
    U32             Index;

    if(Params == NULL)
    {
        layerstill_Defense(ST_ERROR_BAD_PARAMETER);
        return(ST_ERROR_BAD_PARAMETER);
    }

    Index = GetIndexOfDeviceNamed(DeviceName);
    if (Index == INVALID_DEVICE_INDEX)
    {
        /* Device name not found */
        layerstill_Defense(ST_ERROR_UNKNOWN_DEVICE);
        Err = ST_ERROR_UNKNOWN_DEVICE;
        *Handle = 0;
    }
    else
    {
        if (DeviceArray[Index].Open == stlayer_still_context.MaxHandles)
        {
            layerstill_Defense(ST_ERROR_NO_FREE_HANDLES);
            return(ST_ERROR_NO_FREE_HANDLES);
        }
        /* Device name OK */
        DeviceArray[Index].Open ++;
        Err  = ST_NO_ERROR;
        *Handle = Index;
    }
    return(Err);
}
/******************************************************************************/
ST_ErrorCode_t LAYERSTILL_Close(STLAYER_Handle_t  Handle)
{
    if(DeviceArray[Handle].Open >= 1)
    {
        DeviceArray[Handle].Open --;
        return(ST_NO_ERROR);
    }
    else
    {
        layerstill_Defense(ST_ERROR_INVALID_HANDLE);
        return(ST_ERROR_INVALID_HANDLE);
    }
}
/* -------------------------------------------------------------------------- */

ST_ErrorCode_t LAYERSTILL_GetCapability(const ST_DeviceName_t DeviceName,
                                     STLAYER_Capability_t * const Capability_p)

{
    S32 DeviceIndex = INVALID_DEVICE_INDEX;
    ST_ErrorCode_t Err = ST_NO_ERROR;

    if (Capability_p == NULL)
    {
        layerstill_Defense(ST_ERROR_BAD_PARAMETER);
        return (ST_ERROR_BAD_PARAMETER);
    }

    semaphore_wait(stlayer_still_context.AccessProtect_p);

    /* Check if device already initialised and return error if NOT so */
    DeviceIndex = GetIndexOfDeviceNamed(DeviceName);
    if (DeviceIndex == INVALID_DEVICE_INDEX)
    {
        /* Device name not found */
        layerstill_Defense(ST_ERROR_UNKNOWN_DEVICE);
        Err = ST_ERROR_UNKNOWN_DEVICE;
    }
    else
    {
        Capability_p->MultipleViewPorts                  = FALSE;
        Capability_p->HorizontalResizing                 = TRUE;
        Capability_p->AlphaBorder                        = FALSE;
        Capability_p->GlobalAlpha                        = TRUE;
        Capability_p->ColorKeying                        = FALSE;
        Capability_p->MultipleViewPortsOnScanLineCapable = FALSE;
        Capability_p->LayerType                          = STLAYER_OMEGA1_STILL;
    }

    semaphore_signal(stlayer_still_context.AccessProtect_p);

    return(Err);
}

/******************************************************************************/
ST_ErrorCode_t LAYERSTILL_GetLayerParams(STLAYER_Handle_t  Handle,
                                        STLAYER_LayerParams_t *  LayerParams_p)
{
    TestLayHandle(Handle);
     if(LayerParams_p == 0)
    {
        layerstill_Defense(ST_ERROR_BAD_PARAMETER);
        return(ST_ERROR_BAD_PARAMETER);
    }
    semaphore_wait(stlayer_still_context.AccessProtect_p);
   *LayerParams_p = stlayer_still_context.LayerParams;
    semaphore_signal(stlayer_still_context.AccessProtect_p);
    return(ST_NO_ERROR);
}
/******************************************************************************/
ST_ErrorCode_t LAYERSTILL_SetLayerParams(STLAYER_Handle_t  Handle,
                                        STLAYER_LayerParams_t *  LayerParams_p)
{
    TestLayHandle(Handle);
    if(LayerParams_p == 0)
    {
        layerstill_Defense(ST_ERROR_BAD_PARAMETER);
        return(ST_ERROR_BAD_PARAMETER);
    }
    if ((LayerParams_p->Width > stlayer_still_context.Width)
            || (LayerParams_p->Height > stlayer_still_context.Height))
    {
        /* the layer overflow the output */
        layerstill_Defense(ST_ERROR_BAD_PARAMETER);
        return(ST_ERROR_BAD_PARAMETER);
    }
    if ((stlayer_still_context.ViewPort.Open == TRUE)
       && ((stlayer_still_context.ViewPort.OutputRectangle.Width
              + stlayer_still_context.ViewPort.OutputRectangle.PositionX
                             > LayerParams_p->Width)
    || (stlayer_still_context.ViewPort.OutputRectangle.Height
              + stlayer_still_context.ViewPort.OutputRectangle.PositionY
                            > LayerParams_p->Height)))
    {
        /* the viewport overflow the layer */
        layerstill_Defense(ST_ERROR_BAD_PARAMETER);
        return(ST_ERROR_BAD_PARAMETER);
    }

    semaphore_wait(stlayer_still_context.AccessProtect_p);
    stlayer_still_context.LayerParams = *LayerParams_p;
    semaphore_signal(stlayer_still_context.AccessProtect_p);
    return(ST_NO_ERROR);
}
/******************************************************************************/
ST_ErrorCode_t LAYERSTILL_AdjustIORectangle(STLAYER_Handle_t      Handle,
                                         STGXOBJ_Rectangle_t * InputRectangle,
                                         STGXOBJ_Rectangle_t * OutputRectangle)
{
    ST_ErrorCode_t  Error;
    STGXOBJ_Rectangle_t SaveInput,SaveOutput;

    TestLayHandle(Handle);
    SaveInput = * InputRectangle;
    SaveOutput = * OutputRectangle;

    /* Adjust Input Rectangle */
    if (InputRectangle->PositionX < 0)
    {
        InputRectangle->PositionX = 0;
        Error = STLAYER_SUCCESS_IORECTANGLES_ADJUSTED;
    }
    if (InputRectangle->PositionX % 64 != 0)
    {
        InputRectangle->PositionX -= InputRectangle->PositionX % 64;
        Error = STLAYER_SUCCESS_IORECTANGLES_ADJUSTED;
    }
    if (InputRectangle->PositionY < 0)
    {
        InputRectangle->PositionY = 0;
        Error = STLAYER_SUCCESS_IORECTANGLES_ADJUSTED;
    }
    if (InputRectangle->PositionY % 2 != 0)
    {
        InputRectangle->PositionY -= InputRectangle->PositionY % 64;
        Error = STLAYER_SUCCESS_IORECTANGLES_ADJUSTED;
    }
    /* Adjust Output Recangle */
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
            > stlayer_still_context.LayerParams.Width)
    {
        OutputRectangle->Width
            =  stlayer_still_context.LayerParams.Width - OutputRectangle->PositionX ;
        Error = STLAYER_SUCCESS_IORECTANGLES_ADJUSTED;
    }
    if(OutputRectangle->PositionY + OutputRectangle->Height
            > stlayer_still_context.LayerParams.Height)
    {
        OutputRectangle-> Height
            = stlayer_still_context.LayerParams.Height - OutputRectangle->PositionY ;
        Error = STLAYER_SUCCESS_IORECTANGLES_ADJUSTED;
    }

    /* Eval V resize */
    if (OutputRectangle->Height != InputRectangle->Height)
    {
        if (InputRectangle->Height * 2 < OutputRectangle->Height)
        {
            OutputRectangle->Height = 2 * InputRectangle->Height;
            Error = STLAYER_SUCCESS_IORECTANGLES_ADJUSTED;
        }
        else if(OutputRectangle->Height * 2 < InputRectangle->Height)
        {
            InputRectangle->Height = OutputRectangle->Height / 2;
            Error = STLAYER_SUCCESS_IORECTANGLES_ADJUSTED;
        }
    }
    return(ST_NO_ERROR);
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
    if (InitParams_p->MaxViewPorts != 1)
    {
        layerstill_Defense(ST_ERROR_BAD_PARAMETER);
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Init of  globals */
    stlayer_still_context.AVMEMPartition        = InitParams_p->AVMEM_Partition;
    stlayer_still_context.CPUPartition_p        = InitParams_p->CPUPartition_p;
    stlayer_still_context.MaxViewPorts          = InitParams_p->MaxViewPorts;
    stlayer_still_context.MaxHandles            = InitParams_p->MaxHandles;
    stlayer_still_context.BaseAddress           = (void*)((U32)InitParams_p
                     ->BaseAddress_p + (U32)InitParams_p->DeviceBaseAddress_p);
    stlayer_still_context.LayerParams           = *InitParams_p->LayerParams_p;
    /* default output (should be overwrite byxer) */
    stlayer_still_context.XOffset               = 0;
    stlayer_still_context.YOffset               = 0;
    stlayer_still_context.XStart                = 112;
    stlayer_still_context.YStart                = 20+20; /* top + bot */
    stlayer_still_context.OutputEnable          = FALSE;
    stlayer_still_context.Width      = stlayer_still_context.LayerParams.Width;
    stlayer_still_context.Height     = stlayer_still_context.LayerParams.Height;
   STAVMEM_GetSharedMemoryVirtualMapping(&stlayer_still_context.VirtualMapping);
    /* declare the viexport closed */
    stlayer_still_context.ViewPort.Open = FALSE;

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
static ST_ErrorCode_t Term(const STLAYER_TermParams_t* TermParams_p,
                            U32 DeviceIndex)
{
    ST_ErrorCode_t Err = ST_NO_ERROR;

    if ((!(TermParams_p->ForceTerminate))
        && (DeviceArray[DeviceIndex].Open > 0))
    {
        layerstill_Defense(ST_ERROR_BAD_PARAMETER);
        return(ST_ERROR_BAD_PARAMETER); /* error */
    }
    if ((!(TermParams_p->ForceTerminate))
        && (stlayer_still_context.ViewPort.Open > 0))
    {
        layerstill_Defense(ST_ERROR_BAD_PARAMETER);
        return(ST_ERROR_BAD_PARAMETER); /* error */
    }
    if ((!(TermParams_p->ForceTerminate))
        && (stlayer_still_context.ViewPort.Enabled > 0))
    {
        layerstill_Defense(ST_ERROR_BAD_PARAMETER);
        return(ST_ERROR_BAD_PARAMETER); /* error */
    }
    /* At this point; we can term the layer */
    stlayer_StillHardTerm();

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

/******************************************************************************/
U32 stlayer_LayOpen(U32 Device)
{
    return (DeviceArray[Device].Open);
}
/******************************************************************************/


/* end of still_cm.c */
