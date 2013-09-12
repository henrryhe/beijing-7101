/*******************************************************************************

File name   : stvmix.c

Description : video mixer module source file

COPYRIGHT (C) STMicroelectronics 2003.

Date               Modification                                     Name
----               ------------                                     ----
21 July 2000        Created                                          BS
26 Oct  2000        Release A2                                       BS
08 Nov  2000        Add support for Z ordering in layer              BS
21 June 2001        Coding rule compliant. Increase checks           BS
22 Oct  2002        Update revision number 1.3.1                     BS
18 Feb  2005        Update revision number 1.5.0                     NM
******************************************************************************/

/* Includes ----------------------------------------------------------------- */
#if !defined(ST_OSLINUX)
#include <string.h>
#include <stdlib.h>
#endif

#include "sttbx.h"
#include "vmix_drv.h"
#include "vmix_rev.h"

/* Private Constants -------------------------------------------------------- */
#if defined(ST_7200)
#define STVMIX_MAX_DEVICE      3          /* Maximum device */
#else
#define STVMIX_MAX_DEVICE      2          /* Maximum device */
#endif
#define STVMIX_MAX_OPEN        2          /* Maximum handle per device */
#define STVMIX_MAX_OUTPUT      5          /* Number of max connected output */

#define STVMIX_MAX_UNIT        (STVMIX_MAX_DEVICE * STVMIX_MAX_OPEN)  /* Max unit to reserved */
#define STVMIX_INVALID_HANDLE  ((STVMIX_Handle_t) NULL)

/* Any 'uncommon' value could be chosen, different from other module */
/* To prevent same number with other module, syntaxe is 0xXY0ZY0ZX   */
/* where XYZ is driver id number                                     */
#define STVMIX_VALID_UNIT      0x0A00A000

#define INVALID_DEVICE_INDEX   (-1)

#if defined(ST_OSLINUX)
#define STVMIX_MAPPING_WIDTH        0x1000
#endif

/* Private Variables (static)------------------------------------------------ */
static semaphore_t * FctAccessCtrl_p;   /* Init/Open/Close/Term protection semaphore */
static BOOL FirstInitDone = FALSE;
static stvmix_Unit_t UnitArray[STVMIX_MAX_UNIT];
static stvmix_Device_t DeviceArray[STVMIX_MAX_DEVICE];


#define UNUSED(x) (void)(x)


/*Trace Dynamic Data Size---------------------------------------------------- */
#ifdef TRACE_DYNAMIC_DATASIZE
     extern U32  DynamicDataSize ;
    #define AddDynamicDataSize(x)       DynamicDataSize += (x)
#else
    #define AddDynamicDataSize(x)
#endif


/* Global Variables --------------------------------------------------------- */


/* Private Macros ----------------------------------------------------------- */
/* Returns TRUE if the handle is valid, FALSE otherwise */
#define IsValidHandle(Handle) ((((stvmix_Unit_t *) (Handle)) >= UnitArray) &&                    \
                               (((stvmix_Unit_t *) (Handle)) < (UnitArray + STVMIX_MAX_UNIT)) &&  \
                               (((stvmix_Unit_t *) (Handle))->UnitValidity == STVMIX_VALID_UNIT))

/* Private Function prototypes ---------------------------------------------- */
static void EnterCriticalSection(void);
static void LeaveCriticalSection(void);
static STLAYER_Handle_t GetHandleOfLayerName(const stvmix_Device_t* Device_p, \
                                             const ST_DeviceName_t WantedName, U16 LayerOrder);
static ST_ErrorCode_t RestoreConnectedLayerContext(const stvmix_Device_t* Device_p);
static void FillUpdateParams(const stvmix_Device_t* Device_p, STLAYER_OutputParams_t * UpdateParams_p);
static ST_ErrorCode_t InformLayerAboutUpdate(stvmix_Device_t* Device_p, STLAYER_UpdateReason_t UpdateReason);
static ST_ErrorCode_t CheckConfigNewLayer(const stvmix_Device_t* Device_p, STLAYER_OutputParams_t * UpdateParams_p);
static ST_ErrorCode_t CheckConfigNewParams(stvmix_Device_t* Device_p, STLAYER_OutputParams_t * UpdateParams_p);
static ST_ErrorCode_t CheckLayerOnOtherDevice(const ST_DeviceName_t DeviceName);
static ST_ErrorCode_t Init(stvmix_Device_t * const Device_p, const STVMIX_InitParams_t * const InitParams_p);
static ST_ErrorCode_t Open(void);
static ST_ErrorCode_t Close(void);
static ST_ErrorCode_t Term(stvmix_Device_t * const Device_p);


/*******************************************************************************
*                           PRIVATE FUNCTIONS                                  *
*******************************************************************************/

/*******************************************************************************
Name        : EnterCriticalSection
Description : Used to protect critical sections of Init/Open/Close/Term
Parameters  : None
Returns     : Nothing
*******************************************************************************/
static void EnterCriticalSection(void)
{
    static BOOL FctAccessCtrlInitialized = FALSE;

    STOS_TaskLock ();

    if (!FctAccessCtrlInitialized)
    {
        FctAccessCtrlInitialized = TRUE;
        /* Initialise the Init/Open/Close/Term protection semaphore
           Caution: this semaphore is never deleted. (Not an issue) */
        FctAccessCtrl_p = STOS_SemaphoreCreateFifo(NULL,1);
    }

    STOS_TaskUnlock();

    /* Wait for the Init/Open/Close/Term protection semaphore */
    STOS_SemaphoreWait(FctAccessCtrl_p);
}


/*******************************************************************************
Name        : LeaveCriticalSection
Description : Used to release protection of critical sections of Init/Open/Close/Term
Parameters  : None
Returns     : Nothing
*******************************************************************************/
static void LeaveCriticalSection(void)
{
    /* Release the Init/Open/Close/Term protection semaphore */
    STOS_SemaphoreSignal(FctAccessCtrl_p);
}


/*******************************************************************************
Name        : GetHandleOfLayerName
Description : Get the handle of an already open layer in device
Parameters  : the name to look for in the given device
Assumptions : Device_p & WantedName not Null. Layer.NbConnect initialised (Init func)
Returns     : NULL if no handle found, or handle
*******************************************************************************/
static STLAYER_Handle_t GetHandleOfLayerName(const stvmix_Device_t* Device_p, const ST_DeviceName_t WantedName, U16 LayerOrder)
{
    U16    ConnectNb;

    ConnectNb = Device_p->Layers.NbConnect;

    /* Until no layer found */
    while(ConnectNb-- != 0)
    {
        if(strcmp(Device_p->Layers.ConnectArray_p[ConnectNb].Params.DeviceName, WantedName) == 0)
        {
            Device_p->Layers.ConnectArray_p[ConnectNb].HdlUsed = TRUE;
            Device_p->Layers.TmpArray_p[LayerOrder].HdlUsed = TRUE;
            /* Return handle */
            return(Device_p->Layers.ConnectArray_p[ConnectNb].Handle);
        }
    }
    Device_p->Layers.TmpArray_p[LayerOrder].HdlUsed = FALSE;
    return(0);
}


/*******************************************************************************
Name        : RestoreConnectedLayerContext
Description : Close all temporary open handle and restore connected layers context
Parameters  : Device structure
Assumptions : Device_p not Null. Layers.NbTmp initialised (Connect func)
Returns     : ST_NO_ERROR, STVMIX_ERROR_LAYER_ACCESS
*******************************************************************************/
static ST_ErrorCode_t RestoreConnectedLayerContext(const stvmix_Device_t* Device_p)
{
    ST_ErrorCode_t ErrorCode, RetCode = ST_NO_ERROR;
    U16 NbLayer;

    /* Disconnect all temporary open layer connection */
    for(NbLayer = 0; NbLayer<Device_p->Layers.NbTmp ; NbLayer++)
    {
        if(Device_p->Layers.TmpArray_p[NbLayer].HdlUsed == FALSE)
        {
            ErrorCode = STLAYER_Close(Device_p->Layers.TmpArray_p[NbLayer].Handle);
            /* Error should not occurs. Continue the process */
            if( ErrorCode != ST_NO_ERROR)
                RetCode = STVMIX_ERROR_LAYER_ACCESS;
        }
        Device_p->Layers.TmpArray_p[NbLayer].HdlUsed = FALSE;
    }

    /* Clean connected structure */
    for(NbLayer = 0; NbLayer<Device_p->Layers.NbConnect ; NbLayer++)
    {
        Device_p->Layers.ConnectArray_p[NbLayer].HdlUsed = FALSE;
    }
    return(RetCode);
}


/*******************************************************************************
Name        : FillUpdateParams
Description : Fill update param structure for LAYER
Parameters  : Device structure
Returns     : None
*******************************************************************************/
static void FillUpdateParams(const stvmix_Device_t* Device_p, STLAYER_OutputParams_t * UpdateParams_p)
{
    /* Fill structure from layer according to update reason */
    if(UpdateParams_p->UpdateReason & STLAYER_OFFSET_REASON)
    {
        UpdateParams_p->XOffset     = Device_p->XOffset;
        UpdateParams_p->YOffset     = Device_p->YOffset;
    }
    if(UpdateParams_p->UpdateReason & STLAYER_SCREEN_PARAMS_REASON)
    {
        UpdateParams_p->AspectRatio = Device_p->ScreenParams.AspectRatio;
        UpdateParams_p->ScanType    = Device_p->ScreenParams.ScanType;
        UpdateParams_p->FrameRate   = Device_p->ScreenParams.FrameRate;
        UpdateParams_p->Width       = Device_p->ScreenParams.Width;
        UpdateParams_p->Height      = Device_p->ScreenParams.Height;
        UpdateParams_p->XStart      = Device_p->ScreenParams.XStart;
        UpdateParams_p->YStart      = Device_p->ScreenParams.YStart;
    }
    if(UpdateParams_p->UpdateReason & STLAYER_VTG_REASON)
    {
        strcpy(UpdateParams_p->VTGName , Device_p->VTG);
    }
}


/*******************************************************************************
Name        : InformLayerAboutUpdate
Description : Inform connected layers about udate and reason
Parameters  : Device structure, reason why are only disconnection & changeid
Assumptions : Device_p not NULL, UpdateReason contains correct value
Returns     : ST_NO_ERROR, ST_ERROR_BAD_PARAMETER, STVMIX_ERROR_LAYER_ACCESS
*******************************************************************************/
static ST_ErrorCode_t InformLayerAboutUpdate(stvmix_Device_t* Device_p, STLAYER_UpdateReason_t UpdateReason)
{
    U16            LayerCnt;
    ST_ErrorCode_t ErrCode;
    ST_ErrorCode_t RetError = ST_NO_ERROR;
    BOOL ChangeId = FALSE;
    stvmix_Device_t*          Device1_p;

    /* LAYER specific */
    STLAYER_OutputParams_t LayerParams;


    /* Reason is screen parameter update */
    LayerParams.UpdateReason = UpdateReason;

    /*Remove Warning */
    Device1_p = Device_p;

    if( UpdateReason == STLAYER_CHANGE_ID_REASON)
    {
        /* Already device id fill */
        /* Update layers */
        for(LayerCnt=0; LayerCnt<Device_p->Layers.NbTmp; LayerCnt++)
        {
            LayerParams.DeviceId = Device_p->Layers.TmpArray_p[LayerCnt].DeviceId;
            /* Inform layer only of id has changed */
            if(Device_p->Layers.TmpArray_p[LayerCnt].ChangedId == TRUE)
            {
                if(ChangeId == FALSE)
                {
                    ChangeId = TRUE;
                    stvmix_Disable(Device1_p);
                }

                /* Should always be no error!! Continue to process even if an error occcured */
                ErrCode = STLAYER_UpdateFromMixer(Device_p->Layers.TmpArray_p[LayerCnt].Handle, &LayerParams);
                if(ErrCode != ST_NO_ERROR)
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error in changing IDs for layer\n"));
                    RetError = STVMIX_ERROR_LAYER_ACCESS;
                }
            }
        }
    }
    else if( UpdateReason == STLAYER_DISCONNECT_REASON)
    {
        /* Fill structure for layers */
        FillUpdateParams(Device_p, &LayerParams);

        /* Update layers */
        for(LayerCnt=0; (LayerCnt<Device_p->Layers.NbConnect); LayerCnt++)
        {
            /* Should always be no error!! Continue to process even if an error occcured */
            ErrCode = STLAYER_UpdateFromMixer(Device_p->Layers.ConnectArray_p[LayerCnt].Handle, &LayerParams);
            if(ErrCode != ST_NO_ERROR)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Inform failed when disconnecting layer\n"));
                RetError = STVMIX_ERROR_LAYER_ACCESS;
            }
        }
    }
    else
    {
        /* Reason not coded */
        RetError = ST_ERROR_BAD_PARAMETER;
    }
    return (RetError);
}


/*******************************************************************************
Name        : CheckConfigNewLayer
Description : Check if config suits new connected layers
Parameters  : Device structure, Update parameter
Returns     : Error of UpdateFromMixer function
*******************************************************************************/
static ST_ErrorCode_t CheckConfigNewLayer(const stvmix_Device_t* Device_p, STLAYER_OutputParams_t * UpdateParams_p)
{
    U16            LayerCnt, LayerSup;
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;

    /* Try to update layers */
    for(LayerCnt=0; (LayerCnt<Device_p->Layers.NbTmp)&&(ErrCode == ST_NO_ERROR); LayerCnt++)
    {
        if(Device_p->Layers.TmpArray_p[LayerCnt].HdlUsed == FALSE)
        {
            ErrCode = STLAYER_UpdateFromMixer(Device_p->Layers.TmpArray_p[LayerCnt].Handle, UpdateParams_p);
        }
    }

    /* An error occured => Disconnect layers informed */
    if (ErrCode != ST_NO_ERROR)
    {
        UpdateParams_p->UpdateReason = STLAYER_DISCONNECT_REASON;

        /* No need to inform layer that failed */
        LayerCnt--;
        for(LayerSup=0; (LayerSup<LayerCnt); LayerSup++)
        {
            if(Device_p->Layers.TmpArray_p[LayerSup].HdlUsed == FALSE)
            {
                /* Check not necessary. Handle valid because done just upper with same handle */
                /* and disconnect reason is informative. We don't need this layer anyway */
                STLAYER_UpdateFromMixer(Device_p->Layers.TmpArray_p[LayerSup].Handle, UpdateParams_p);
            }
        }
    }
    return(ErrCode);
}


/*******************************************************************************
Name        : CheckConfigNewParams
Description : Check if config suits connected layers
Parameters  : Device structure, Update parameter
Returns     : STVMIX_ERROR_LAYER_UPDATE_PARAMETERS, ST_ERROR_BAD_PARAMETER
              STVMIX_ERROR_LAYER_ACCESS
*******************************************************************************/
static ST_ErrorCode_t CheckConfigNewParams(stvmix_Device_t* Device_p, STLAYER_OutputParams_t * UpdateParams_p)
{

    U16            LayerCnt, LayerSup;
    ST_ErrorCode_t ErrCode = ST_NO_ERROR, RetErr = ST_NO_ERROR;
    STVMIX_ScreenParams_t PreviousScreenParams;
    S8 PreviousXOffset = 0, PreviousYOffset = 0;

    if(UpdateParams_p->UpdateReason & STLAYER_OFFSET_REASON)
    {
        PreviousXOffset = Device_p->XOffset;
        PreviousYOffset = Device_p->YOffset;
        Device_p->XOffset = UpdateParams_p->XOffset;
        Device_p->YOffset = UpdateParams_p->YOffset;
    }

    if(UpdateParams_p->UpdateReason & STLAYER_SCREEN_PARAMS_REASON)
    {
        PreviousScreenParams = Device_p->ScreenParams;
        Device_p->ScreenParams.AspectRatio = UpdateParams_p->AspectRatio;
        Device_p->ScreenParams.ScanType = UpdateParams_p->ScanType;
        Device_p->ScreenParams.FrameRate = UpdateParams_p->FrameRate;
        Device_p->ScreenParams.Width = UpdateParams_p->Width;
        Device_p->ScreenParams.Height= UpdateParams_p->Height;
        Device_p->ScreenParams.XStart = UpdateParams_p->XStart;
        Device_p->ScreenParams.YStart = UpdateParams_p->YStart;
    }

    if((UpdateParams_p->UpdateReason & STLAYER_SCREEN_PARAMS_REASON) ||
       (UpdateParams_p->UpdateReason & STLAYER_OFFSET_REASON))
    {
        /* Test new values in mixer Hal */
        ErrCode = stvmix_SetScreen(Device_p);
        if(ErrCode != ST_NO_ERROR)
            RetErr = ST_ERROR_BAD_PARAMETER;
    }

    /* Try to update layers */
    for(LayerCnt=0; (LayerCnt<Device_p->Layers.NbConnect)&&(ErrCode == ST_NO_ERROR); LayerCnt++)
    {
        ErrCode = STLAYER_UpdateFromMixer(Device_p->Layers.ConnectArray_p[LayerCnt].Handle, UpdateParams_p);
    }

    /* An error occured => Undo with layers updated */
    if (ErrCode != ST_NO_ERROR)
    {
        /* Restore context */
        if(UpdateParams_p->UpdateReason & STLAYER_SCREEN_PARAMS_REASON)
        {
            Device_p->ScreenParams = PreviousScreenParams;
        }
        if(UpdateParams_p->UpdateReason & STLAYER_OFFSET_REASON)
        {
            Device_p->XOffset = PreviousXOffset;
            Device_p->YOffset = PreviousYOffset;
        }

        /* Previous parameters */
        FillUpdateParams(Device_p, UpdateParams_p);

        if(LayerCnt != 0){
            /* Need to restore layers that don't failed */
            LayerCnt--;
            for(LayerSup=0; (LayerSup<LayerCnt); LayerSup++)
            {
                /* No check function already called upper */
                STLAYER_UpdateFromMixer(Device_p->Layers.ConnectArray_p[LayerSup].Handle, UpdateParams_p);
            }
        }

        if((UpdateParams_p->UpdateReason & STLAYER_SCREEN_PARAMS_REASON) ||
           (UpdateParams_p->UpdateReason & STLAYER_OFFSET_REASON))
        {
            /* Hal back to previous state => No error */
            stvmix_SetScreen(Device_p);
        }

        /* Error already managed ? */
        if(RetErr == ST_NO_ERROR)
        {
            if (ErrCode == ST_ERROR_INVALID_HANDLE)
           {
               /* Layer may have been destroyed */
               RetErr = STVMIX_ERROR_LAYER_ACCESS;
           }
           else
           {
               RetErr = STVMIX_ERROR_LAYER_UPDATE_PARAMETERS;
           }
        }
    }
    else
    {
        /* No error occured */
        /* Update internal structure according to reason(s) */
        if(UpdateParams_p->UpdateReason & STLAYER_VTG_REASON)
        {
            strcpy(Device_p->VTG, UpdateParams_p->VTGName);
        }
    }
    return(RetErr);
}


/*******************************************************************************
Name        : CheckLayerOnOtherDevice
Description : Check if layer already connected on other layers
Parameters  : Layer Name
Assumptions : DeviceName not NULL, Layers.NbConnect initialised (init)
Returns     : ST_NO_ERROR, STVMIX_ERROR_LAYER_ALREADY_CONNECTED
*******************************************************************************/
static ST_ErrorCode_t CheckLayerOnOtherDevice(const ST_DeviceName_t DeviceName)
{

    U16  LayerCnt, DeviceCnt = 0;

    /* Access to global structure of device */
    while(DeviceCnt < STVMIX_MAX_DEVICE)
    {
        /* Check if device initialised */
        if(DeviceArray[DeviceCnt].DeviceName[0] != '\0')
        {
            /* Layers connected to this device ? */
            if(DeviceArray[DeviceCnt].Status & API_CONNECTED_LAYER)
            {
                /* Check each connected layers */
                for(LayerCnt=0; LayerCnt<DeviceArray[DeviceCnt].Layers.NbConnect ;LayerCnt++)
                {
                    /* Connect layer matches with devicename ? */
                    if(strcmp(DeviceArray[DeviceCnt].Layers.ConnectArray_p[LayerCnt].Params.DeviceName, DeviceName) == 0)
                        /* Layer is found */
                        return(STVMIX_ERROR_LAYER_ALREADY_CONNECTED);
                }
            }
        }
        DeviceCnt++;
    }
    /* No layer found with same name */
    return(ST_NO_ERROR);
}


/*******************************************************************************
Name        : CheckOutputOnOtherDevice
Description : Check if layer already connected on other layers
Parameters  : Layer Name
Returns     : ST_NO_ERROR, STVMIX_ERROR_VOUT_ALREADY_CONNECTED
*******************************************************************************/
static ST_ErrorCode_t CheckOuputOnOtherDevice(const ST_DeviceName_t DeviceName)
{
    U16  LayerCnt, DeviceCnt = 0;

    /* Access to global structure of device */
    while(DeviceCnt < STVMIX_MAX_DEVICE)
    {
        /* Check if device initialised */
        if(DeviceArray[DeviceCnt].DeviceName[0] != '\0')
        {
            /* Check each output connected */
            for(LayerCnt=0; LayerCnt<DeviceArray[DeviceCnt].Outputs.NbConnect; LayerCnt++)
            {
                /* Connect layer matches with devicename ? */
                if(strcmp(DeviceArray[DeviceCnt].Outputs.ConnectArray_p[LayerCnt].Name, DeviceName) == 0)
                    /* Layer is found */
                    return(STVMIX_ERROR_VOUT_ALREADY_CONNECTED);
            }
        }
        DeviceCnt++;
    }
    /* No layer found with same name */
    return(ST_NO_ERROR);
}


/*******************************************************************************
Name        : GetIndexOfDeviceNamed
Description : Get the index in DeviceArray where a device has been
              initialised with the wanted name, if it exists
Parameters  : the name to look for
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
    } while ((WantedIndex == INVALID_DEVICE_INDEX) && (Index < STVMIX_MAX_DEVICE));

    return(WantedIndex);
}

/*******************************************************************************
Name        : Init
Description : Device specific initialisation
Parameters  : None
Assumptions : Device_p & InitParams_p not NULL
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_BAD_PARAMETER, FEATURE_NOT_SUPPORTED
*******************************************************************************/
static ST_ErrorCode_t Init(stvmix_Device_t * const Device_p, const STVMIX_InitParams_t * const InitParams_p)
{

    ST_DeviceName_t*     VoutName_p;
    STVOUT_Capability_t  VoutCapa;
    STVOUT_Handle_t      VoutHandle;
    STVOUT_OpenParams_t  VoutOpenParams;
    U8                   Val=0;
    U8                   DacSelect=0;
    ST_ErrorCode_t       Error=ST_NO_ERROR;

    STVTG_Handle_t       VtgHandle;
    STVTG_OpenParams_t   VtgOpenParams;
    STVTG_ModeParams_t   ModeParams;
    STVTG_TimingMode_t   TimingMode;




    /* common stuff initialization*/
    Device_p->InitParams.DeviceType = InitParams_p->DeviceType;
    Device_p->InitParams.CPUPartition_p = InitParams_p->CPUPartition_p;
    Device_p->InitParams.BaseAddress_p = InitParams_p->BaseAddress_p;
    Device_p->InitParams.DeviceBaseAddress_p = InitParams_p->DeviceBaseAddress_p;
    Device_p->InitParams.MaxLayer = InitParams_p->MaxLayer;
    Device_p->Layers.NbConnect = 0;
    Device_p->Status = API_SCREEN_PARAMS;         /* Screen params are initialised with default value PAL mode */
    Device_p->Background.Color.R = 0;
    Device_p->Background.Color.G = 0;
    Device_p->Background.Color.B = 0;
    Device_p->Background.Enable = 0;
      /* VTG name is not NULL ? */
    strcpy(Device_p->VTG, InitParams_p->VTGName);
    if(Device_p->VTG[0] != '\0')
        Device_p->Status |= API_STVTG_CONNECT;


    /* Save event handler name even if unsignificant for Omega1 */
    strncpy(Device_p->InitParams.EvtHandlerName, InitParams_p->EvtHandlerName, sizeof(ST_DeviceName_t));

    /* Check init params for Vout and Get Type */
    Device_p->Outputs.NbConnect = 0;
    VoutName_p = InitParams_p->OutputArray_p;
    while((Device_p->Outputs.NbConnect < STVMIX_MAX_OUTPUT) && (VoutName_p != NULL))
    {
        if ((*VoutName_p)[0] == '\0')

            break;

        Error = STVOUT_GetCapability(*VoutName_p, &VoutCapa);
        if(Error == ST_NO_ERROR)
        {
            /* Check that another mixer is not connected on same output */
            Error = CheckOuputOnOtherDevice(*VoutName_p);
            if(Error != ST_NO_ERROR)

                return(Error);

            Device_p->Outputs.ConnectArray_p[Device_p->Outputs.NbConnect].Type = VoutCapa.SelectedOutput;
            strcpy(Device_p->Outputs.ConnectArray_p[Device_p->Outputs.NbConnect].Name , *VoutName_p);
            VoutName_p++;
            Device_p->Outputs.NbConnect++;
        }
        else
        {
            /* Only possible error is device unknown */
            return(STVMIX_ERROR_VOUT_UNKNOWN);
        }
    }

     /* Check the selected dacs for all vout connection */
    VoutName_p=InitParams_p->OutputArray_p;
    while((VoutName_p!= NULL) && (Error == ST_NO_ERROR))
    {
           if ((*VoutName_p)[0]== '\0')
           {
              break;
           }
           Error = STVOUT_Open(*VoutName_p, &VoutOpenParams, &VoutHandle);
           Error = STVOUT_GetDacSelect(VoutHandle,&DacSelect);
           if(Error==ST_NO_ERROR)
           {
               if((Val & DacSelect)!= 0)
               {
                   Error = ST_ERROR_BAD_PARAMETER;
               }
               else
               {
                   Val |= DacSelect;
               }
               Error = STVOUT_Close(VoutHandle);
               VoutName_p++;
           }
           else
           {
                  Error=ST_ERROR_NO_FREE_HANDLES;
           }
    }

#if defined(ST_OSLINUX)
    if ( Error == ST_NO_ERROR)
	{
         Device_p->VmixMappedWidth = (U32)(STVMIX_MAPPING_WIDTH);
         Device_p->VmixMappedBaseAddress_p = STLINUX_MapRegion( (void *) (Device_p->InitParams.BaseAddress_p), (U32) (Device_p->VmixMappedWidth), "VMIX");

        if ( Device_p->VmixMappedBaseAddress_p )
	    {
            /** affecting VMIX Base Adress with mapped Base Adress **/
             Device_p->InitParams.BaseAddress_p = Device_p->VmixMappedBaseAddress_p;
	    }
        else
        {
             Error =  ST_ERROR_BAD_PARAMETER;
        }


    }


#endif /* ST_OSLINUX */

/*retreive screen parameters from stvtg mode*/
    Error = STVTG_Open( Device_p->VTG, &VtgOpenParams, &VtgHandle);
    if(Error != ST_NO_ERROR)
    {
        return(Error);
    }
    Error = STVTG_GetMode(VtgHandle, &TimingMode, &ModeParams);
    if(Error != ST_NO_ERROR)
    {
        return(Error);
    }
    if (TimingMode != STVTG_TIMING_MODE_SLAVE)    /* VTG is SLAVE */
    {
        Device_p->ScreenParams.AspectRatio = STGXOBJ_ASPECT_RATIO_4TO3;
        Device_p->ScreenParams.FrameRate   = ModeParams.FrameRate;
        Device_p->ScreenParams.ScanType    =  ModeParams.ScanType;
        Device_p->ScreenParams.Width       = ModeParams.ActiveAreaWidth;
        Device_p->ScreenParams.Height      = ModeParams.ActiveAreaHeight;
        Device_p->ScreenParams.XStart      = ModeParams.DigitalActiveAreaXStart;
        Device_p->ScreenParams.YStart      = ModeParams.DigitalActiveAreaYStart;
    }
    else
    {
        /*default for PAL*/
         /* Record some info in Device structure */
         Device_p->ScreenParams.AspectRatio  = STGXOBJ_ASPECT_RATIO_4TO3;
         Device_p->ScreenParams.ScanType     = STGXOBJ_INTERLACED_SCAN;
         Device_p->ScreenParams.FrameRate    = 50000;
         Device_p->ScreenParams.Width        = 720;
         #ifdef SERVICE_DIRECTV
             Device_p->ScreenParams.Height       = 480;
         #else
             Device_p->ScreenParams.Height       = 576;
         #endif
             Device_p->ScreenParams.XStart       = 132;
             Device_p->ScreenParams.YStart       = 23;
    }

    Error = STVTG_Close(VtgHandle);
     if(Error != ST_NO_ERROR)
    {
        return(Error);
    }

    /* Other inits with default values */
    Device_p->XOffset = 0;
    Device_p->YOffset = 0;

    /* Call device init Layer */
    Error = stvmix_Init(Device_p, InitParams_p);

    return(Error);
 }


/*******************************************************************************
Name        : Open
Description : Device specific opening
Parameters  : Device_p
Assumptions : Device_p not NULL
Limitations :
Returns     : ST_NO_ERROR
*******************************************************************************/
static ST_ErrorCode_t Open(void)
{
    /* Nothing to do */
    return(ST_NO_ERROR);
}


/*******************************************************************************
Name        : Close
Description : Device specific opening
Parameters  : Device_p
Assumptions : Device_p not NULL
Limitations :
Returns     : ST_NO_ERROR
*******************************************************************************/
static ST_ErrorCode_t Close(void)
{
    /* Nothing to do */
    return(ST_NO_ERROR);
}


/*******************************************************************************
Name        : Term
Description : Device specific termination
Parameters  : Device_p
Assumptions : Device_p not NULL and Layers.NbConnect set (init, connect, dconnnect)
Limitations :
Returns     : ST_NO_ERROR
*******************************************************************************/
static ST_ErrorCode_t Term(stvmix_Device_t * const Device_p)
{
    U16   NbLayer;
    /* Close all open handles, even if some layers fails */
    for(NbLayer = 0; NbLayer<Device_p->Layers.NbConnect; NbLayer++)
    {
        STLAYER_Close(Device_p->Layers.ConnectArray_p[NbLayer].Handle);
    }
    /* Call layer termination */
    stvmix_Term(Device_p);

#if defined(ST_OSLINUX)
    /* releasing VMIX linux device driver */
    /** VMIX region **/
    STLINUX_UnmapRegion(Device_p->VmixMappedBaseAddress_p, (U32) (Device_p->VmixMappedWidth));

#endif

    return(ST_NO_ERROR);
}


/*******************************************************************************
*                               API FUNCTIONS                                  *
*******************************************************************************/

/*******************************************************************************
Name        : STVMIX_ConnectLayers
Description : Connect layers to a mixer
Parameters  : Handle, pointer to LayerDisplayParams structure, size of this structure
Assumptions : None
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_INVALID_HANDLE, ST_ERROR_BAD_PARAMETER,
              STVMIX_ERROR_LAYER_UNKNOWN, STVMIX_ERROR_LAYER_ACCESS,
              ST_ERROR_FEATURE_NOT_SUPPORTED, STVMIX_ERROR_NO_VSYNC
*******************************************************************************/
ST_ErrorCode_t STVMIX_ConnectLayers(const STVMIX_Handle_t Handle,
                                    const STVMIX_LayerDisplayParams_t * const LayerDisplayParams[],
                                    const U16 NbLayerParams)
{
    U16                   LayerOrder = 0, NbLayer;
    ST_ErrorCode_t        Error = ST_NO_ERROR;
    stvmix_Unit_t *       MixerUnit_p;
    stvmix_Device_t*      Device_p;

    /* Connection variables to STLayer */
    STLAYER_OpenParams_t   LayerOpenParam;
    STLAYER_Handle_t       LayerHandle;
    STLAYER_Capability_t   LayerCapability;
    STLAYER_OutputParams_t UpdateParams;
    BOOL                   LayerAccessError = FALSE;

    MixerUnit_p =  (stvmix_Unit_t *) Handle ;
    /* Check Handle Validity */
    if (!IsValidHandle(Handle))

        return ST_ERROR_INVALID_HANDLE;



    /* Various inits */
    Device_p = MixerUnit_p->Device_p;
    NbLayer = NbLayerParams;
    Device_p->Layers.NbTmp = 0;

    /* Check if max layers reached */
    if(NbLayer > Device_p->InitParams.MaxLayer)
    {
        /* Report */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Too many layers to connect\n"));
        return (ST_ERROR_BAD_PARAMETER);
    }

    /* Protect access */
    STOS_SemaphoreWait(Device_p->CtrlAccess_p);

    /* 1 - First, extract all layer type & handle in array */
    while(NbLayer-- != 0)
    {
        if ((LayerDisplayParams[LayerOrder] == NULL) || (LayerDisplayParams[LayerOrder]->DeviceName == NULL))
        {
            /* Disconnect all temporary open layer connection */
            Error = RestoreConnectedLayerContext(Device_p);

            /* Free access */
            STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

            /* Access problem to layer */
            if (Error != ST_NO_ERROR)

                return (Error);

            return (ST_ERROR_BAD_PARAMETER);
        }

        /* Check if already an open handle on layer */
        LayerHandle = GetHandleOfLayerName(Device_p, LayerDisplayParams[LayerOrder]->DeviceName, LayerOrder);

        if(LayerHandle == 0)
        {
            /* Check if layer connected on other devices */
            Error = CheckLayerOnOtherDevice(LayerDisplayParams[LayerOrder]->DeviceName);

            if(Error == ST_NO_ERROR)
            {
                /* Connection to STLayer */
                Error = STLAYER_Open(((const STVMIX_LayerDisplayParams_t *) (LayerDisplayParams[LayerOrder]))->DeviceName,
                                     &LayerOpenParam, &LayerHandle);
            }

            if(Error != ST_NO_ERROR)
            {
                /* Disconnect all temporary open layer connection */
                /* Don't send this error. Already error returned below */
                RestoreConnectedLayerContext(Device_p);

                /* Free access */
                STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

                /* Manage first error code returned of check on other device, trace it */
                if(Error == STVMIX_ERROR_LAYER_ALREADY_CONNECTED)
                {
                    /* Report */
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Layer '%s' already connected on other device\n", \
                                  LayerDisplayParams[LayerOrder]->DeviceName));
                    return(STVMIX_ERROR_LAYER_ALREADY_CONNECTED);
                }

                /* Report */
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Unable to open device layer '%s'\n", \
                              LayerDisplayParams[LayerOrder]->DeviceName));
                return (STVMIX_ERROR_LAYER_UNKNOWN);
            }
        }

        /* GetCapability of a given layer. Should be no error unless layer have been destroyed. */
        /* This is the reason why we don't store the type, a connection is kept with layer */
        /* If an error occurs, back to previous state */
        Error = STLAYER_GetCapability(((const STVMIX_LayerDisplayParams_t *) (LayerDisplayParams[LayerOrder]))->DeviceName,\
                                      &LayerCapability);
        if(Error != ST_NO_ERROR)
        {
            /* Disconnect all temporary open layer connection */
            Error = RestoreConnectedLayerContext(Device_p);

            /* Free access */
            STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Unable to get the capability of device layer '%s'\n", \
                          LayerDisplayParams[LayerOrder]->DeviceName));
            return (STVMIX_ERROR_LAYER_ACCESS);
        }

        /* Store tmp layer type and deviceId */
        Device_p->Layers.TmpArray_p[LayerOrder].Type = LayerCapability.LayerType;
        Device_p->Layers.TmpArray_p[LayerOrder].DeviceId = LayerCapability.DeviceId;

        /* Store tmp layer handle */
        Device_p->Layers.TmpArray_p[LayerOrder].Handle = LayerHandle;

        /* Next Layer */
        LayerOrder++;
        Device_p->Layers.NbTmp++;
    }

    /* 2 - Then, connect Layer to Hal to check if hardware can support */
    /* Check if error occurs on order, repetitition of same layer and type of layers supported by hardware */
    Error = stvmix_ConnectLayer(MixerUnit_p->Device_p);

    /* Exit if error occurs */
    if(Error == ST_NO_ERROR)
    {
        /* Default value for reason : offset, connect */
        UpdateParams.UpdateReason = STLAYER_OFFSET_REASON | STLAYER_CONNECT_REASON;

        /* Set display latency and buffers hold time (always 1 for non blitter based devices) */
        UpdateParams.FrameBufferDisplayLatency      = 1;
        UpdateParams.FrameBufferHoldTime = 1;

        /* First connection, update the reason for new connected layers */
        if(Device_p->Status & API_STVTG_CONNECT)
            UpdateParams.UpdateReason |= STLAYER_VTG_REASON;
        if(Device_p->Status & API_SCREEN_PARAMS)
            UpdateParams.UpdateReason |= STLAYER_SCREEN_PARAMS_REASON;

        /* Fill structure to given to layers */
        FillUpdateParams(Device_p, &UpdateParams);

        /* 3 - Then, Check now if config of new connected layers is accepted by layers */
        /* Check occurs for the moment on aspect ratio, scan type */
        /* (if wanted other screen params & VTG can be cheked in the layer */
        Error = CheckConfigNewLayer(Device_p, &UpdateParams);

        /* No error occured => Close open handle on previous layers connected & store connected layers */
        if(Error == ST_NO_ERROR)
        {
            /* From now, no more errors should occurs! Terminate the process */

            /* 4 - Then, Disconnect all open handle on layers not reconnected in this context */
            if(Device_p->Status & API_CONNECTED_LAYER)
            {
                /* Information of disconnection is given to previous connected layers */
                /* As the result, layer can unsubscribe to VTG events */
                UpdateParams.UpdateReason = STLAYER_DISCONNECT_REASON;
                NbLayer = Device_p->Layers.NbConnect;

                /* For Omega1 mixer, also inform about disabling */
                UpdateParams.UpdateReason |= STLAYER_DISPLAY_REASON;
                UpdateParams.DisplayEnable = FALSE;

                while(NbLayer-- != 0)
                {
                    if(Device_p->Layers.ConnectArray_p[NbLayer].HdlUsed == FALSE)
                    {
                        /* Should always return NO_ERROR !!! */
                        /* Error occured if the layer have been destroyed. Update was already checked */
                        Error = STLAYER_UpdateFromMixer(Device_p->Layers.ConnectArray_p[NbLayer].Handle, &UpdateParams);
                        if(Error != ST_NO_ERROR)
                        {
                            LayerAccessError = TRUE;
                            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error when informing layer %s about disconnection !!\n",
                                          Device_p->Layers.ConnectArray_p[NbLayer].Params.DeviceName));
                        }
                        Error = STLAYER_Close(Device_p->Layers.ConnectArray_p[NbLayer].Handle);
                        if(Error != ST_NO_ERROR){
                            LayerAccessError = TRUE;
                            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error when closing layer %s !!\n",
                                          Device_p->Layers.ConnectArray_p[NbLayer].Params.DeviceName));
                        }
                    }
                }
            }

            /* 5 - Inform that layer should switch each other */
            /* No error should occurs. Continue the process if failure */
            Error = InformLayerAboutUpdate(Device_p, STLAYER_CHANGE_ID_REASON);
            if(Error != ST_NO_ERROR)
            {
                LayerAccessError = TRUE;
            }

            /* 6 -Save structure of all connected layer */
            NbLayer = NbLayerParams;
            LayerOrder = 0;
            while(NbLayer-- != 0)
            {
                /* Store structure of connected layers */
                Device_p->Layers.ConnectArray_p[LayerOrder].Params = *LayerDisplayParams[LayerOrder];
                Device_p->Layers.ConnectArray_p[LayerOrder].Handle = Device_p->Layers.TmpArray_p[LayerOrder].Handle;
                Device_p->Layers.ConnectArray_p[LayerOrder].Type = Device_p->Layers.TmpArray_p[LayerOrder].Type;
                Device_p->Layers.ConnectArray_p[LayerOrder].DeviceId = Device_p->Layers.TmpArray_p[LayerOrder].DeviceId;

                /* Back to init */
                Device_p->Layers.ConnectArray_p[LayerOrder].HdlUsed = FALSE;
                Device_p->Layers.TmpArray_p[LayerOrder].HdlUsed = FALSE;
                LayerOrder++;
            }

            /* Layers connected, store info */
            Device_p->Layers.NbConnect = NbLayerParams;
            Device_p->Status |= API_CONNECTED_LAYER;

            /* Hal Enable Output Now */
            stvmix_Enable(Device_p);

            /* Report if ok */
            if(LayerAccessError == FALSE)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Layers connected successfully\n"));
            }
        }
        else
        {
            /* Disconnect all already temporary open layer connection */
            RestoreConnectedLayerContext(Device_p);

            /* Report */
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Layer cannot support mixer parameters\n"));
            Error = STVMIX_ERROR_LAYER_UPDATE_PARAMETERS;
        }
    }
    else
    {
        /* Disconnect all already temporary open layer connection */
        RestoreConnectedLayerContext(Device_p);

        /* Report */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Hardware cannot support layer type or order\n"));
    }

    /* Free access */
    STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

    /* If unexpected error => layer access problem with layer */
    if(LayerAccessError == TRUE)
        Error = STVMIX_ERROR_LAYER_ACCESS;

    return(Error);
}


/*******************************************************************************
Name        : STVMIX_Close
Description : Close connection to a mixer
Parameters  : Handle on a device
Assumptions : None
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_UNKNOWN_DEVICE, ST_ERROR_INVALID_HANDLE
*******************************************************************************/
ST_ErrorCode_t STVMIX_Close(const STVMIX_Handle_t Handle)
{
    stvmix_Unit_t * Unit_p;
    ST_ErrorCode_t  Err = ST_NO_ERROR;

    EnterCriticalSection();
    if (!FirstInitDone)
    {
        Err = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        if (!IsValidHandle(Handle))
            Err = ST_ERROR_INVALID_HANDLE;
        else
        {
            Unit_p = (stvmix_Unit_t *) Handle;

            if (Unit_p->UnitValidity != STVMIX_VALID_UNIT)
            {
                Err = ST_ERROR_INVALID_HANDLE;
            }
            else
            {
                /* API specific actions before closing */
                Err = Close(); /* For Gcc compilation Unit_p->Device_p not used */

                /* Close only if no errors */
                if (Err == ST_NO_ERROR)
                {
                    /* Un-register opened handle */
                    Unit_p->Handle = STVMIX_INVALID_HANDLE;
                    Unit_p->UnitValidity = (U32)~STVMIX_VALID_UNIT;

                    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Handle closed on device '%s'\n",\
                                  Unit_p->Device_p->DeviceName));
                }
            }
        }
    }

    LeaveCriticalSection();
    return(Err);
}


/*******************************************************************************
Name        : STVMIX_Disable
Description : Disable output of mixer
Parameters  : Handle on a device
Assumptions : None
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_INVALID_HANDLE
*******************************************************************************/
ST_ErrorCode_t STVMIX_Disable(const STVMIX_Handle_t Handle)
{
    stvmix_Unit_t *   MixerUnit_p;
    stvmix_Device_t*  Device_p;

    /* Check Handle Validity */
    if (!IsValidHandle(Handle))
        return ST_ERROR_INVALID_HANDLE;

    MixerUnit_p =  (stvmix_Unit_t *) Handle ;
    Device_p = MixerUnit_p->Device_p;

    /* Protect access */
    STOS_SemaphoreWait(Device_p->CtrlAccess_p);

    /* Disable Device Output. Always sucessfull from hal */
    stvmix_Disable(Device_p);

    /* Update API status */
    Device_p->Status &= ~(API_ENABLE);

    /* Free access */
    STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

    /* Report */
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Mixer disabled successfully\n"));

    return(ST_NO_ERROR);
}


/*******************************************************************************
Name        : STVMIX_DisconnectLayers
Description : Disconnect layers from a mixer
Parameters  : Handle on a device
Assumptions : None
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_INVALID_HANDLE, STVMIX_ERROR_LAYER_ACCESS
*******************************************************************************/
ST_ErrorCode_t STVMIX_DisconnectLayers(const STVMIX_Handle_t Handle)
{
    stvmix_Unit_t *   MixerUnit_p;
    stvmix_Device_t*  Device_p;
    U16               NbLayer;
    ST_ErrorCode_t    ErrorCode;
    BOOL              LayerError = FALSE;

    /* Check Handle Validity */
    if (!IsValidHandle(Handle))
        return ST_ERROR_INVALID_HANDLE;

    MixerUnit_p =  (stvmix_Unit_t *) Handle ;
    Device_p = MixerUnit_p->Device_p;

    /* Protect access */
    STOS_SemaphoreWait(Device_p->CtrlAccess_p);

    /* Disable Device Output */
    /* If in function disconnect, pb occurs, it comes from layer disconnection */
    /* Same failure will be discovered below */
    stvmix_DisconnectLayer(Device_p);

    /* Inform layers about disconnection */
    ErrorCode = InformLayerAboutUpdate(Device_p, STLAYER_DISCONNECT_REASON);
    if (ErrorCode != ST_NO_ERROR)
        LayerError = TRUE;

    /* Close open handle of all connected layers */
    for(NbLayer = 0; NbLayer<Device_p->Layers.NbConnect; NbLayer++)
    {
        ErrorCode = STLAYER_Close(Device_p->Layers.ConnectArray_p[NbLayer].Handle);
        if (ErrorCode != ST_NO_ERROR)
            LayerError = TRUE;
    }

    /* No more layer connected */
    Device_p->Layers.NbConnect = 0;

    /* Set status: layers disconnected */
    Device_p->Status &= ~(API_CONNECTED_LAYER);

    /* Free access */
    STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

    /* Report */
    if (LayerError == FALSE)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Layers disconnected successfully\n"));
        ErrorCode = ST_NO_ERROR;
    }
    else
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Disconnected failed with some layers\n"));
        ErrorCode = STVMIX_ERROR_LAYER_ACCESS;
    }
    return(ErrorCode);
}


/*******************************************************************************
Name        : STVMIX_Enable
Description : Enable output of a mixer
Parameters  : Handle on a device
Assumptions : None
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_INVALID_HANDLE, STVMIX_ERROR_LAYER_ACCESS,
              STVMIX_ERROR_NO_VSYNC
*******************************************************************************/
ST_ErrorCode_t STVMIX_Enable(const STVMIX_Handle_t Handle)
{
    stvmix_Unit_t *   MixerUnit_p;
    stvmix_Device_t*  Device_p;
    ST_ErrorCode_t    ErrorCode;

    /* Check Handle Validity */
    if (!IsValidHandle(Handle))
        return ST_ERROR_INVALID_HANDLE;

    MixerUnit_p =  (stvmix_Unit_t *) Handle ;
    Device_p = MixerUnit_p->Device_p;

    /* Protect access */
    STOS_SemaphoreWait(Device_p->CtrlAccess_p);

    /* Update Status */
    Device_p->Status |= API_ENABLE;

    /* Enable Device Output */
    ErrorCode = stvmix_Enable(MixerUnit_p->Device_p);

    /* Error occured ? */
    if( ErrorCode != ST_NO_ERROR)
    {
        /* Not enabled because of error */
        Device_p->Status &= ~(API_ENABLE);
    }
    else
    {
        /* Report */
         STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Mixer enabled successfully\n"));
    }

    /* Free access */
    STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

    return(ErrorCode);
}


/*******************************************************************************
Name        : STVMIX_GetBackgroundColor
Description : Get the Background color of a driver
Parameters  : Handle on a device, pointer to color structure and enable
Assumptions : None
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_FEATURE_NOT_SUPPORTED
*******************************************************************************/
ST_ErrorCode_t STVMIX_GetBackgroundColor(const STVMIX_Handle_t     Handle, \
                                         STGXOBJ_ColorRGB_t* const RGB888_p, BOOL* const Enable_p)
{
    stvmix_Unit_t *   MixerUnit_p;
    stvmix_Device_t*  Device_p;
    ST_ErrorCode_t    Err = ST_NO_ERROR;

    /* Check Handle Validity */
    if (!IsValidHandle(Handle))
        return ST_ERROR_INVALID_HANDLE;

    MixerUnit_p =  (stvmix_Unit_t *) Handle ;
    Device_p = MixerUnit_p->Device_p;

    /* Check parameters */
    if((RGB888_p == NULL) || (Enable_p == NULL))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
    /* Get info from hal */
    Err = stvmix_GetBackGroundColor(Device_p, RGB888_p, Enable_p);

    /* Report */
    if(Err == ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Background color get successfully\n"));
    }

    return(Err);
}


/*******************************************************************************
Name        : STVMIX_GetCapability
Description : Get the capability of a driver
Parameters  : Handle on a device, pointer to capabilities structure
Assumptions : None
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_UNKNOWN_DEVICE, ST_ERROR_BAD_PARAMETER
              ST_ERROR_FEATURE_NOT_SUPPORTED
*******************************************************************************/
ST_ErrorCode_t STVMIX_GetCapability(const ST_DeviceName_t DeviceName, STVMIX_Capability_t* const Capability_p)
{
    S32 DeviceIndex = INVALID_DEVICE_INDEX;
    ST_ErrorCode_t    ErrCode = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if ((Capability_p == NULL) ||                        /* There must be parameters ! */
        (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1) || /* Device name length should be respected */
        ((((const char *) DeviceName)[0]) == '\0')             /* Device Name should not be NULL */
        )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    EnterCriticalSection();

    /* Init done */
    if (!FirstInitDone)
    {
        ErrCode = ST_ERROR_UNKNOWN_DEVICE;
    }
    else
    {
        /* Check if device already initialised and return error if not so */
        DeviceIndex = GetIndexOfDeviceNamed(DeviceName);
        if (DeviceIndex == INVALID_DEVICE_INDEX)
        {
            /* Device name not found */
            ErrCode = ST_ERROR_UNKNOWN_DEVICE;
        }
        else
        {
            /* Fill structure with current info */
            Capability_p->ScreenOffsetHorizontalMin = (&DeviceArray[DeviceIndex])->Capability.ScreenOffsetHorizontalMin;
            Capability_p->ScreenOffsetHorizontalMax = (&DeviceArray[DeviceIndex])->Capability.ScreenOffsetHorizontalMax;
            Capability_p->ScreenOffsetVerticalMin = (&DeviceArray[DeviceIndex])->Capability.ScreenOffsetVerticalMin ;
            Capability_p->ScreenOffsetVerticalMax = (&DeviceArray[DeviceIndex])->Capability.ScreenOffsetVerticalMax;
        }
    }

    LeaveCriticalSection();

    return(ErrCode);
}

/*******************************************************************************
Name        : STVMIX_GetConnectedLayers
Description : Ability to retrieve connected layers
Parameters  : Handle on a device,
              Layer Number starting from 1 and the farthest from the eyes
              Pointer to ST_DeviceName_t
Assumptions : None
Limitations :
Returns     : ST_ERROR_INVALID_HANDLE, ST_ERROR_BAD_PARAMETER,
              STVMIX_ERROR_LAYER_UNKNOWN
*******************************************************************************/
ST_ErrorCode_t STVMIX_GetConnectedLayers(const STVMIX_Handle_t Handle,
                                        const U16 LayerPosition,
                                        STVMIX_LayerDisplayParams_t* const LayerParams_p)
{
    ST_ErrorCode_t    ErrCode = ST_NO_ERROR;
    stvmix_Device_t*  Device_p;
    stvmix_Unit_t *   MixerUnit_p;

    /* Check Handle Validity */
    if (!IsValidHandle(Handle))
        return ST_ERROR_INVALID_HANDLE;

    MixerUnit_p =  (stvmix_Unit_t *) Handle ;
    Device_p = MixerUnit_p->Device_p;

    /* Check Params */
    if ((LayerPosition == 0) ||
        (LayerParams_p == NULL) ||
        (Device_p->InitParams.MaxLayer < LayerPosition))
    {
        return (ST_ERROR_BAD_PARAMETER);
    }

    /* Protect access */
    STOS_SemaphoreWait(Device_p->CtrlAccess_p);

    /* Layers are connected & layer number exists */
    if((Device_p->Status | API_CONNECTED_LAYER) &&
       (Device_p->Layers.NbConnect >= LayerPosition))
    {
        *LayerParams_p = Device_p->Layers.ConnectArray_p[LayerPosition-1].Params;
    }
    else
    {
        ErrCode = STVMIX_ERROR_LAYER_UNKNOWN;
    }

    /* Free access */
    STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

    return(ErrCode);
}


/*******************************************************************************
Name        : STVMIX_GetRevision
Description : Give the Revision number of the STVMIX driver
Parameters  : None
Assumptions : None
Limitations :
Returns     : Revision number
*******************************************************************************/
ST_Revision_t STVMIX_GetRevision(void)
{
    /* Revision string format: STXXX-REL_x.x.x
                                 /       \ \ \__patch release number  }
                            API name      \ \__minor release number  } revision number
                                           \__major release number  }
       Example: STVMIX-REL_1.2.3 */

    return(VMIX_Revision);
} /* End of STVMIX_GetRevision() function */


/*******************************************************************************
Name        : STVMIX_GetScreenOffset
Description : Get the screen offset
Parameters  : Handle on a device, pointer to horizontal and vertical offset
Assumptions : None
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_INVALID_HANDLE, ST_ERROR_BAD_PARAMETER
*******************************************************************************/
ST_ErrorCode_t STVMIX_GetScreenOffset(const STVMIX_Handle_t Handle, S8* const Horizontal_p, S8* const Vertical_p)
{
    stvmix_Device_t*    Device_p;
    stvmix_Unit_t *     MixerUnit_p;

    /* Check Handle Validity */
    if (!IsValidHandle(Handle))
        return ST_ERROR_INVALID_HANDLE;

    MixerUnit_p =  (stvmix_Unit_t *) Handle ;
    Device_p = MixerUnit_p->Device_p;

    /* Check Params */
    if((Horizontal_p == NULL) || (Vertical_p == NULL))
        return (ST_ERROR_BAD_PARAMETER);

    /* Protect access */
    STOS_SemaphoreWait(Device_p->CtrlAccess_p);

    /* Store values */
    *Horizontal_p = Device_p->XOffset;
    *Vertical_p = Device_p->YOffset;

    /* Free access */
    STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

    return (ST_NO_ERROR);
}


/*******************************************************************************
Name        : STVMIX_GetScreenParams
Description : Get the screen paramameters
Parameters  : Handle on a device, pointer to screen parameters
Assumptions : None
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_INVALID_HANDLE, ST_ERROR_BAD_PARAMETER
*******************************************************************************/
ST_ErrorCode_t STVMIX_GetScreenParams(const STVMIX_Handle_t Handle, STVMIX_ScreenParams_t* const ScreenParams_p)
{
    stvmix_Device_t*    Device_p;
    stvmix_Unit_t *     MixerUnit_p;

    /* Check Handle Validity */
    if (!IsValidHandle(Handle))
        return ST_ERROR_INVALID_HANDLE;

    MixerUnit_p =  (stvmix_Unit_t *) Handle ;
    Device_p = MixerUnit_p->Device_p;

    /* Check Params */
    if(ScreenParams_p == NULL)
        return (ST_ERROR_BAD_PARAMETER);

    /* Protect access */
    STOS_SemaphoreWait(Device_p->CtrlAccess_p);

    /* Give values of screen params */
    ScreenParams_p->AspectRatio = Device_p->ScreenParams.AspectRatio;
    ScreenParams_p->ScanType    = Device_p->ScreenParams.ScanType;
    ScreenParams_p->FrameRate   = Device_p->ScreenParams.FrameRate;
    ScreenParams_p->Width       = Device_p->ScreenParams.Width;
    ScreenParams_p->Height      = Device_p->ScreenParams.Height;
    ScreenParams_p->XStart      = Device_p->ScreenParams.XStart;
    ScreenParams_p->YStart      = Device_p->ScreenParams.YStart;

    /* Free access */
    STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

    return(ST_NO_ERROR);
}


/*******************************************************************************
Name        : STVMIX_GetTimeBase
Description : Give back VTG driver name attached to the mixer device
Parameters  : Handle on a device, Pointer to allocated device name
Assumptions : None
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_INVALID_HANDLE, ST_ERROR_BAD_PARAMETER
*******************************************************************************/
ST_ErrorCode_t STVMIX_GetTimeBase(const STVMIX_Handle_t Handle, ST_DeviceName_t* const VTGDriver_p)
{
    stvmix_Device_t*    Device_p;
    stvmix_Unit_t *     MixerUnit_p;

    /* Check Handle Validity */
    if (!IsValidHandle(Handle))
        return ST_ERROR_INVALID_HANDLE;

    MixerUnit_p =  (stvmix_Unit_t *) Handle ;
    Device_p = MixerUnit_p->Device_p;

    /* Check Params */
    if(VTGDriver_p == NULL)
        return (ST_ERROR_BAD_PARAMETER);

    if(Device_p->Status & API_STVTG_CONNECT)
    {
        strcpy(*VTGDriver_p, Device_p->VTG);
    }
    else
    {
        *VTGDriver_p[0] = '\0';
    }
    return(ST_NO_ERROR);
}


/*******************************************************************************
Name        : STVMIX_Init
Description : Init device function
Parameters  : Name of a device mixer to Open, pointer to initialisation parameters
Assumptions : None
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_BAD_PARAMETER, ST_ERROR_ALREADY_INITIALIZED, ST_ERROR_NO_MEMORY
*******************************************************************************/
ST_ErrorCode_t STVMIX_Init(const ST_DeviceName_t DeviceName, const STVMIX_InitParams_t* const InitParams_p)
{
    S32                            Index = 0;
    ST_ErrorCode_t                 Err = ST_NO_ERROR;
    stvmix_LayerConnect_t*         LayerConnectedArray_p;
    stvmix_LayerTmp_t*             LayerTmpArray_p;
    stvmix_Outputs_Params_t*       OutputArray_p;

    /* Exit now if parameters are bad */
    if ((InitParams_p == NULL) ||                        /* There must be parameters */
        (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1) || /* Device name length should be respected */
        ((((const char *) DeviceName)[0]) == '\0')             /* Device Name should not be NULL */
        )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
    if ((InitParams_p->MaxOpen > STVMIX_MAX_OPEN) || (InitParams_p->MaxOpen == 0))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    EnterCriticalSection();

    /* First time: initialise devices and units as empty */
    if (!FirstInitDone)
    {
        for (Index= 0; Index < STVMIX_MAX_DEVICE; Index++)
        {
            DeviceArray[Index].DeviceName[0] = '\0';
        }

        for (Index= 0; Index < STVMIX_MAX_UNIT; Index++)
        {
            UnitArray[Index].Handle = STVMIX_INVALID_HANDLE;
            UnitArray[Index].UnitValidity = (U32)~STVMIX_VALID_UNIT;
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
        while ((DeviceArray[Index].DeviceName[0] != '\0') && (Index < STVMIX_MAX_DEVICE))
        {
            Index++;
        }

        if (Index >= STVMIX_MAX_DEVICE)
        {
            /* All devices initialised */
            Err = ST_ERROR_NO_MEMORY;
        }
        else
        {
            /* Store max open */
            DeviceArray[Index].MaxOpen = InitParams_p->MaxOpen;

            /* Allocated for layer order */
            LayerConnectedArray_p = (stvmix_LayerConnect_t*) STOS_MemoryAllocate (InitParams_p->CPUPartition_p, \
                                                                              InitParams_p->MaxLayer * \
                                                                              sizeof(stvmix_LayerConnect_t));
            AddDynamicDataSize(InitParams_p->MaxLayer * sizeof(stvmix_LayerConnect_t));
            /* Allocated array to check layer order */
            LayerTmpArray_p = (stvmix_LayerTmp_t*) STOS_MemoryAllocate(InitParams_p->CPUPartition_p,        \
                                                                   InitParams_p->MaxLayer * sizeof(stvmix_LayerTmp_t));
           AddDynamicDataSize(InitParams_p->MaxLayer * sizeof(stvmix_LayerTmp_t));

            /* Allocated array for vout connection */
            OutputArray_p = (stvmix_Outputs_Params_t*) STOS_MemoryAllocate(InitParams_p->CPUPartition_p,        \
                                                              STVMIX_MAX_OUTPUT * sizeof(stvmix_Outputs_Params_t));
            AddDynamicDataSize(STVMIX_MAX_OUTPUT * sizeof(stvmix_Outputs_Params_t));

             if ((LayerTmpArray_p == NULL) || (LayerConnectedArray_p == NULL) || (OutputArray_p == NULL))
            {
                if(LayerConnectedArray_p != NULL)
                    STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, LayerConnectedArray_p);
                if(LayerTmpArray_p != NULL)
                    STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, LayerTmpArray_p);
                if(OutputArray_p != NULL)
                    STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, OutputArray_p);

                Err = ST_ERROR_NO_MEMORY;  /* Not enough memory to initialize mixer */
            }

            if (Err == ST_NO_ERROR)
            {
                /* Save allocated structure */
                DeviceArray[Index].Layers.ConnectArray_p = LayerConnectedArray_p;
                DeviceArray[Index].Layers.TmpArray_p = LayerTmpArray_p;
                DeviceArray[Index].Outputs.ConnectArray_p = OutputArray_p;

                /* API specific initialisations */
                Err = Init(&DeviceArray[Index], InitParams_p);

                if (Err == ST_NO_ERROR)
                {
                    /* Semaphore mixer device access creation */
                    DeviceArray[Index].CtrlAccess_p = STOS_SemaphoreCreateFifo(NULL,1);

                    /* Register device name */
                    strcpy(DeviceArray[Index].DeviceName, DeviceName);
                    DeviceArray[Index].DeviceName[ST_MAX_DEVICE_NAME - 1] = '\0';

                    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Device '%s' initialised\n", \
                                  DeviceArray[Index].DeviceName));
                }
                else   /* Free allocated memory. Not NULL checked upper */
                {
                    STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, LayerTmpArray_p);
                    STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, LayerConnectedArray_p);
                    STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, OutputArray_p);
                }
            }
        }
    }

    LeaveCriticalSection();

    return(Err);
}


/*******************************************************************************
Name        : STVMIX_Open
Description : Open device function
Parameters  : Name of the device to open, pointer to open parameters
Assumptions : None
Limitations :
Returns     : ST_NO_ERROR, INVALID_DEVICE_INDEX, ST_ERROR_BAD_PARAMETER,
              ST_ERROR_UNKNOWN_DEVICE, ST_ERROR_NO_FREE_HANDLES
*******************************************************************************/
ST_ErrorCode_t STVMIX_Open(const ST_DeviceName_t DeviceName, const STVMIX_OpenParams_t* const OpenParams_p, \
                           STVMIX_Handle_t* Handle_p)
{
    S32 DeviceIndex = INVALID_DEVICE_INDEX, UnitIndex = 0, Index;
    U32 OpenedUnitForThisInit;
    ST_ErrorCode_t Err = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if ((OpenParams_p == NULL) ||                       /* There must be parameters ! */
        (Handle_p == NULL) ||                           /* Pointer to handle should be valid ! */
        (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1)|| /* Device name length should be respected */
        ((((const char *) DeviceName)[0]) == '\0')            /* Device name NULL */
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
            /* Look for a free unit and return error if none is free */
            UnitIndex = STVMIX_MAX_UNIT;
            OpenedUnitForThisInit = 0;
            for (Index = 0; Index < STVMIX_MAX_UNIT; Index++)
            {
                if((UnitArray[Index].UnitValidity == STVMIX_VALID_UNIT) && \
                   (UnitArray[Index].Device_p == &DeviceArray[DeviceIndex]))
                {
                    OpenedUnitForThisInit++;
                }
                if (UnitArray[Index].UnitValidity != STVMIX_VALID_UNIT)
                {
                    /* Found a free handle structure */
                    UnitIndex = Index;
                }
            }
            if ((UnitIndex >= STVMIX_MAX_UNIT) || (OpenedUnitForThisInit >= DeviceArray[DeviceIndex].MaxOpen))
            {
                /* None of the units is free */
                Err = ST_ERROR_NO_FREE_HANDLES;
            }
            else
            {
                *Handle_p = (STVMIX_Handle_t) &UnitArray[UnitIndex];
                UnitArray[UnitIndex].Device_p = &DeviceArray[DeviceIndex]; /* 'inherits' device characteristics */
                /* API specific actions after opening */
                Err = Open(); /* For Gcc compilation UnitArray[UnitIndex].Device_p not used */

                if (Err == ST_NO_ERROR)
                {
                    /* Register opened handle */
                    UnitArray[UnitIndex].Handle = *Handle_p;
                    UnitArray[UnitIndex].UnitValidity = STVMIX_VALID_UNIT;

                    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Handle opened on device '%s'\n", DeviceName));
                }
            }
        }
    }

    LeaveCriticalSection();

    return(Err);
}


/*******************************************************************************
Name        : STVMIX_SetBackgroundColor
Description : Set the background color of a driver
Parameters  : Handle on a device, color structure and enable
Assumptions : None
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_FEATURE_NOT_SUPPORTED
*******************************************************************************/
ST_ErrorCode_t STVMIX_SetBackgroundColor(const STVMIX_Handle_t Handle, \
                                         STGXOBJ_ColorRGB_t const RGB888, BOOL const Enable)
{
    ST_ErrorCode_t Err = ST_NO_ERROR;
    stvmix_Unit_t *   MixerUnit_p;
    stvmix_Device_t*  Device_p;

    /* Check Handle Validity */
    if (!IsValidHandle(Handle))
        return ST_ERROR_INVALID_HANDLE;
    MixerUnit_p =  (stvmix_Unit_t *) Handle ;
    Device_p = MixerUnit_p->Device_p;

    /* Protect access */
    STOS_SemaphoreWait(Device_p->CtrlAccess_p);

    /* Call Hal function */
    Err = stvmix_SetBackGroundColor(Device_p, RGB888, Enable);

    /* Free access */
    STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

    /* Report */
    if(Err == ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Background color set successfully\n"));
    }

    return(Err);
}


/*******************************************************************************
Name        : STVMIX_SetScreenOffset
Description : Set the screen offset
Parameters  : Handle on a device, Horizontal & Vertical Offset
Assumptions : None
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_INVALID_HANDLE
*******************************************************************************/
ST_ErrorCode_t STVMIX_SetScreenOffset(const STVMIX_Handle_t Handle, const S8 Horizontal, const S8 Vertical)
{
    stvmix_Device_t*        Device_p;
    stvmix_Unit_t *         MixerUnit_p;
    STLAYER_OutputParams_t  UpdateParams;
    ST_ErrorCode_t          Err;

    /* Check Handle Validity */
    if (!IsValidHandle(Handle))
        return ST_ERROR_INVALID_HANDLE;

    MixerUnit_p =  (stvmix_Unit_t *) Handle ;
    Device_p = MixerUnit_p->Device_p;

    /* Values inside the range ? */
    if ((Vertical < Device_p->Capability.ScreenOffsetVerticalMin) ||
        (Vertical > Device_p->Capability.ScreenOffsetVerticalMax) ||
        (Horizontal < Device_p->Capability.ScreenOffsetHorizontalMin) ||
        (Horizontal > Device_p->Capability.ScreenOffsetHorizontalMax))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Protect access */
    STOS_SemaphoreWait(Device_p->CtrlAccess_p);

    /* New values for offset */
    UpdateParams.XOffset = Horizontal;
    UpdateParams.YOffset = Vertical;
    UpdateParams.UpdateReason = STLAYER_OFFSET_REASON;

    /* Inform Hal and then layers */
    Err = CheckConfigNewParams(Device_p, &UpdateParams);

    /* Report */
    if(Err == ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Screen Offset set successfully\n"));
    }

    /* Free access */
    STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

    return (Err);
}


/*******************************************************************************
Name        : STVMIX_SetScreenParams
Description : Set the screen parameters
Parameters  : Handle on a device, pointer to screen parameter structure
Assumptions : None
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_BAD_PARAMETER, STVMIX_ERROR_LAYER_UPDATE_PARAMETERS
              STVMIX_ERROR_LAYER_ACCESS
*******************************************************************************/
ST_ErrorCode_t STVMIX_SetScreenParams(const STVMIX_Handle_t Handle, const STVMIX_ScreenParams_t* const ScreenParams_p)
{
    stvmix_Device_t*        Device_p;
    stvmix_Unit_t *         MixerUnit_p;
    ST_ErrorCode_t          Err;
    STLAYER_OutputParams_t  UpdateParams;

    /* Check Handle Validity */
    if (!IsValidHandle(Handle))
        return ST_ERROR_INVALID_HANDLE;

    MixerUnit_p =  (stvmix_Unit_t *) Handle ;
    Device_p = MixerUnit_p->Device_p;

    /* Check Params */
    if(ScreenParams_p == NULL)
        return (ST_ERROR_BAD_PARAMETER);

    /* Check parameters validity */
    if((ScreenParams_p->AspectRatio > STGXOBJ_ASPECT_RATIO_SQUARE) ||
       (ScreenParams_p->ScanType > STGXOBJ_INTERLACED_SCAN))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Protect access */
    STOS_SemaphoreWait(Device_p->CtrlAccess_p);

    /* Store new values of screen params */
    UpdateParams.AspectRatio = ScreenParams_p->AspectRatio;
    UpdateParams.ScanType    = ScreenParams_p->ScanType;
    UpdateParams.FrameRate   = ScreenParams_p->FrameRate;
    UpdateParams.Width       = ScreenParams_p->Width;
    UpdateParams.Height      = ScreenParams_p->Height;
    UpdateParams.XStart       = ScreenParams_p->XStart;
    UpdateParams.YStart      = ScreenParams_p->YStart;
    UpdateParams.UpdateReason = STLAYER_SCREEN_PARAMS_REASON;

    /* Inform Hal and then layers */
    Err = CheckConfigNewParams(Device_p, &UpdateParams);

    /* Free access */
    STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

    if (Err == ST_NO_ERROR)
    {
        /* Report */
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Screen params set successfully\n"));
    }
    return(Err);
}


/*******************************************************************************
Name        : STVMIX_SetTimeBase
Description : Attach a VTG driver to a mixer, then to the layers connected
Parameters  : Handle on a device, Name of a VTG driver
Assumptions : None
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_BAD_PARAMETER, STVMIX_ERROR_LAYER_ACCESS,
              STVMIX_ERROR_NO_VSYNC
*******************************************************************************/
ST_ErrorCode_t STVMIX_SetTimeBase(const STVMIX_Handle_t Handle, const ST_DeviceName_t VTGDriver)
{
    stvmix_Device_t*          Device_p;
    stvmix_Unit_t *           MixerUnit_p;
    STLAYER_OutputParams_t    UpdateParams;
    ST_ErrorCode_t            Err = ST_NO_ERROR, RetErr = ST_NO_ERROR;
    ST_DeviceName_t           OldVTG;

    /* Check Handle Validity */
    if (!IsValidHandle(Handle))
        return ST_ERROR_INVALID_HANDLE;

    MixerUnit_p =  (stvmix_Unit_t *) Handle ;
    Device_p = MixerUnit_p->Device_p;

    /* Check Params */
    if(VTGDriver == NULL)
        return (ST_ERROR_BAD_PARAMETER);
    if(strlen(VTGDriver) > ST_MAX_DEVICE_NAME - 1)
        return (ST_ERROR_BAD_PARAMETER);
    if(VTGDriver[0] == '\0')
        return (ST_ERROR_BAD_PARAMETER);

    /* Protect access */
    STOS_SemaphoreWait(Device_p->CtrlAccess_p);

    /* Save old VTG name */
    strcpy(OldVTG, Device_p->VTG);

    /* Send info to layers if connected */
    if(Device_p->Status & API_CONNECTED_LAYER)
    {
        /* Store new value for VTG */
        strcpy(UpdateParams.VTGName, VTGDriver);
        UpdateParams.UpdateReason = STLAYER_VTG_REASON;

        /* Inform connected layers about the update */
        Err = CheckConfigNewParams(Device_p, &UpdateParams);
    }
    else
    {
        /* Store new value for VTG */
        strcpy(Device_p->VTG, VTGDriver);
    }

    if(Err == ST_NO_ERROR)
    {
        /* Update status VTG => VTG given name to layer & not null */
        Device_p->Status |= API_STVTG_CONNECT;

        RetErr = stvmix_SetTimeBase(Device_p, OldVTG);
        if(RetErr == ST_NO_ERROR)
        {
            /* Enable the mixer if possible */
            RetErr = stvmix_Enable(Device_p);

            if(RetErr == ST_NO_ERROR)
            {
                /* Report */
                STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Time base set successfully\n"));
            }
        }
    }
    else
    {
        /* Assign new error */
        RetErr = Err;
        if(RetErr == ST_ERROR_BAD_PARAMETER)
        {
            /* VTG name doesn't exist */
            Device_p->Status &= (~API_STVTG_CONNECT);

            /* Report */
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Time base problem with name !!!\n"));
        }
    }

    /* Free access */
    STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

    return(RetErr);
}


/*******************************************************************************
Name        : STVMIX_Term
Description : Termination mixer device function
Parameters  : Name of a mixer device, pointer to termination structure
Assumptions : None
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_BAD_PARAMETER, ST_ERROR_UNKNOWN_DEVICE, ST_ERROR_OPEN_HANDLE
*******************************************************************************/
ST_ErrorCode_t STVMIX_Term(const ST_DeviceName_t DeviceName, const STVMIX_TermParams_t* const TermParams_p)
{
    stvmix_Unit_t *Unit_p;
    S32 DeviceIndex = INVALID_DEVICE_INDEX, UnitIndex = 0;
    BOOL Found = FALSE;
    ST_ErrorCode_t Err = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if ((TermParams_p == NULL) ||                        /* There must be parameters ! */
        (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1) || /* Device name length should be respected */
        ((((const char *) DeviceName)[0]) == '\0')             /* Device Name should not be NULL */
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
            UnitIndex = 0;
            Found = FALSE;
            Unit_p = UnitArray;
            while ((UnitIndex < STVMIX_MAX_UNIT) && (!Found))
            {
                Found = ((Unit_p->Device_p == &DeviceArray[DeviceIndex]) && \
                         (Unit_p->Handle != STVMIX_INVALID_HANDLE));
                Unit_p++;
                UnitIndex++;
            }

            if (Found)
            {
                if (TermParams_p->ForceTerminate)
                {
                    /* Close all remaining open handle */
                    UnitIndex = 0;
                    Unit_p = UnitArray;
                    while (UnitIndex < STVMIX_MAX_UNIT)
                    {
                        if(Unit_p->Device_p == &DeviceArray[DeviceIndex])
                        {
                            Unit_p->Handle = STVMIX_INVALID_HANDLE;
                            Unit_p->UnitValidity = (U32)~STVMIX_VALID_UNIT;
                        }
                        Unit_p++;
                        UnitIndex++;
                    }

                    /* API specific terminations */
                    Err = Term(&DeviceArray[DeviceIndex]);

                    if (Err == ST_NO_ERROR)
                    {
                        /* Free allocated memory */
                        STOS_MemoryDeallocate(DeviceArray[DeviceIndex].InitParams.CPUPartition_p, \
                                          DeviceArray[DeviceIndex].Layers.ConnectArray_p);
                        STOS_MemoryDeallocate(DeviceArray[DeviceIndex].InitParams.CPUPartition_p,\
                                          DeviceArray[DeviceIndex].Layers.TmpArray_p);
                        STOS_MemoryDeallocate(DeviceArray[DeviceIndex].InitParams.CPUPartition_p,\
                                          DeviceArray[DeviceIndex].Outputs.ConnectArray_p);

                        /* Get semaphore before deleting it */
                        STOS_SemaphoreWait(DeviceArray[DeviceIndex].CtrlAccess_p);

                        /* Free device semaphore */
                        STOS_SemaphoreDelete(NULL,DeviceArray[DeviceIndex].CtrlAccess_p);

                        /* Reset values */
                        DeviceArray[DeviceIndex].Layers.ConnectArray_p = NULL;
                        DeviceArray[DeviceIndex].Layers.TmpArray_p = NULL;
                        DeviceArray[DeviceIndex].Outputs.ConnectArray_p = NULL;

                        /* free device */
                        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Device '%s' forced terminated\n", DeviceName));
                        DeviceArray[DeviceIndex].DeviceName[0] = '\0';
                    }
                }
                else
                {
                    Err = ST_ERROR_OPEN_HANDLE;
                }
            }
            else /* No more instance. Close the driver */
            {
                /* API specific terminations */
                Err = Term(&DeviceArray[DeviceIndex]);

                if (Err == ST_NO_ERROR)
                {
                    /* Free allocated memory */
                    STOS_MemoryDeallocate(DeviceArray[DeviceIndex].InitParams.CPUPartition_p,\
                                      DeviceArray[DeviceIndex].Layers.ConnectArray_p);
                    STOS_MemoryDeallocate(DeviceArray[DeviceIndex].InitParams.CPUPartition_p,\
                                      DeviceArray[DeviceIndex].Layers.TmpArray_p);
                    STOS_MemoryDeallocate(DeviceArray[DeviceIndex].InitParams.CPUPartition_p,\
                                      DeviceArray[DeviceIndex].Outputs.ConnectArray_p);

                    /* Get semaphore before deleting it */
                    STOS_SemaphoreWait(DeviceArray[DeviceIndex].CtrlAccess_p);

                    /* Free device semaphore */
                    STOS_SemaphoreDelete(NULL,DeviceArray[DeviceIndex].CtrlAccess_p);

                    /* Device found: free device */
                    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Device '%s' terminated\n", DeviceName));
                    DeviceArray[DeviceIndex].DeviceName[0] = '\0';
                }
            }
        }
    }

    LeaveCriticalSection();

    return(Err);
}



/*******************************************************************************
Name        : STVMIX_OpenVBIViewPort
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STVMIX_OpenVBIViewPort(STVMIX_Handle_t     Handle,
                                      STVMIX_VBIViewPortType_t    VBIType,
                                      STVMIX_VBIViewPortHandle_t*   VPHandle_p)
{

 ST_ErrorCode_t              Err                 = ST_ERROR_FEATURE_NOT_SUPPORTED;
UNUSED(Handle);
UNUSED(VBIType);
UNUSED(VPHandle_p);



return(Err);
}




/*******************************************************************************
Name        : STVMIX_SetVBIViewPortParams
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STVMIX_SetVBIViewPortParams(STVMIX_VBIViewPortHandle_t  VPHandle,
                                          STVMIX_VBIViewPortParams_t* Params_p)
{

 ST_ErrorCode_t              Err                 = ST_ERROR_FEATURE_NOT_SUPPORTED;

UNUSED(VPHandle);
UNUSED(Params_p);




return(Err);
}

/*******************************************************************************
Name        : STVMIX_EnableVBIViewPort
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STVMIX_EnableVBIViewPort(STVMIX_VBIViewPortHandle_t  VPHandle)
{

  ST_ErrorCode_t              Err                 = ST_ERROR_FEATURE_NOT_SUPPORTED;

UNUSED(VPHandle);


return(Err);
}
/*******************************************************************************
Name        : STVMIX_DisbleVBIViewPort
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STVMIX_DisableVBIViewPort(STVMIX_VBIViewPortHandle_t  VPHandle)
{

 ST_ErrorCode_t              Err                 = ST_ERROR_FEATURE_NOT_SUPPORTED;

  UNUSED(VPHandle);


return(Err);
}

/*******************************************************************************
Name        : STVMIX_CloseVBIViewPort
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STVMIX_CloseVBIViewPort(STVMIX_VBIViewPortHandle_t  VPHandle)
{

    ST_ErrorCode_t              Err         = ST_ERROR_FEATURE_NOT_SUPPORTED;
    UNUSED(VPHandle);



return(Err);
}

/* Global Flicker Filter management */

/*******************************************************************************
Name        : STVMIX_EnableGlobalFlickerFilter
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STVMIX_EnableGlobalFlickerFilter(const STVMIX_Handle_t Handle)
{
	ST_ErrorCode_t Err = ST_ERROR_FEATURE_NOT_SUPPORTED;

	UNUSED(Handle);

	return(Err);
}

/*******************************************************************************
Name        : STVMIX_DisableGlobalFlickerFilter
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STVMIX_DisableGlobalFlickerFilter(const STVMIX_Handle_t Handle)
{
	ST_ErrorCode_t Err = ST_ERROR_FEATURE_NOT_SUPPORTED;

	UNUSED(Handle);

	return(Err);
}

