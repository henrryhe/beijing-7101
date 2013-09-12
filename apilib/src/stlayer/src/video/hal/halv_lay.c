/*******************************************************************************

File name   : Halv_lay.c

Description : Video HAL (Hardware Abstraction Layer) access to hardware source file

COPYRIGHT (C) STMicroelectronics 2002-2007

Date               Modification                                     Name
----               ------------                                     ----
23 Feb 2000        Created                                          HG
31 Jui 2001        Rename the functions                             AN
18 jan 2002        Added LAYERVID_GetVTGParams()                    HG
23 Aug 2006        Moved stlayer_GetDecimationFactorFromDecimation  DG
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */

/* Includes ----------------------------------------------------------------- */

#ifndef ST_OSLINUX
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#endif


#ifdef ST_OSLINUX
    #include "stddefs.h"
    #include "stlayer_core.h"
#endif  /* ST_OSLINUX */
#include "sttbx.h"
#include "halv_lay.h"


#if defined (HW_5508) || \
    defined (HW_5518) || \
    defined (HW_5510) || \
    defined (HW_5512) || \
    defined (HW_5514) || \
    defined (HW_5516) || \
    defined (HW_5517) || \
    defined (HW_5578)
    #include "omega1/hv_layer.h"
#endif /* defined (HW_5508) ... defined (HW_5578) */

#if defined (HW_7015) || defined (HW_7020)
    #include "omega2/hv_layer.h"
#endif /* defined (HW_7015) || defined (HW_7020) */

#if defined (HW_5528) || defined (HW_7710) || defined (HW_7100)
    #include "sddispo2/hv_lay8.h"
#endif /* defined (HW_5528) || defined (HW_7710) || defined (HW_7100) */

#if defined (HW_7109)
    #include "displaypipe/hv_vdplay.h"  /* For Main display */
    #include "sddispo2/hv_lay8.h"       /* For Aux display */
#endif  /* defined (HW_7109) */

#if defined (HW_7200)
    #include "displaypipe/hv_vdplay.h"
#endif  /* defined (HW_7200) */


/* Private Types ------------------------------------------------------------ */


/* Private Constants -------------------------------------------------------- */

#define HALBAD_INDEX           0x00ffffff

/* Dummy value that we set to check that a handle is valid, only one chance out of 2^32 to get it by accident ! */
#define HALLAYER_VALID_HANDLE 0xde48a925

/* Private Variables (static)------------------------------------------------ */


/* ------------------------------ Debug ------------------------------------- */


/* #define DEBUG */
/* #define DEBUG_RECORD_TIME */

#ifdef DEBUG_RECORD_TIME
    extern void layer_incr_tv(volatile STLAYER_Record_Time_t **tvp);
    extern STLAYER_Record_Time_t tv_data_tab[]; /* too lazy to allocate it */
    extern volatile STLAYER_Record_Time_t *tv_head;
    extern volatile STLAYER_Record_Time_t *tv_tail;
    extern struct timeval old_tv_data;
    extern spinlock_t layer_time_lock;
#endif

/* Private functions prototypes --------------------------------------------- */
static void LayerVideo_VSync(STEVT_CallReason_t Reason,
                                const ST_DeviceName_t RegistrantName,
                                STEVT_EventConstant_t Event,
                                void *EventData,
                                void *SubscriberData_p);
#ifdef STLAYER_VIDEO_TASK
static void STLAYER_VideoTask(void * Context_p);
#endif
/* Private Functions -------------------------------------------------------- */
#ifdef ST_OSLINUX
static U32 stlayer_HalIndexOfNamedLayer(const ST_DeviceName_t DeviceName)
{
    BOOL Found = FALSE;
    U32  i     = 0;
    do
    {
        if (strcmp(DeviceName,stlayer_LayerArray[i].LayerName) == 0)
        {
            Found = TRUE;
        }
        else
        {
            i++;
        }
    } while ((i<TOTAL_MAX_LAYER) && (Found==FALSE));

    if (Found)
    {
        return(i);
    }
    else
    {
        return(HALBAD_INDEX);
    }
}
#endif /* ST_OSLINUX */

/* Global Variables --------------------------------------------------------- */

/* Private Macros ----------------------------------------------------------- */

/*******************************************************************************
Name        : LAYERVID_Init
Description : Inits The Hal video Layer
Parameters  :
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success, ST_ERROR_BAD_PARAMETER, STLAYER_ERROR_EVENT_REGISTRATION
*******************************************************************************/
ST_ErrorCode_t LAYERVID_Init(const ST_DeviceName_t DeviceName, const STLAYER_InitParams_t * const LayerInitParams_p, STLAYER_Handle_t * const LayerHandle_p)
{
    HALLAYER_LayerProperties_t    * HALLAYER_Data_p;
    STEVT_OpenParams_t              STEVT_OpenParams;
    ST_DeviceName_t                 LayerName;
    ST_ErrorCode_t                  ErrorCode = ST_NO_ERROR;
#ifdef ST_OSLINUX
    STLAYERMod_Param_t              VideoLayerModuleParam;           /* layer modules parameters */
    STLAYERMod_Param_t              CompositorModuleParam;
#endif

#ifdef DEBUG_RECORD_TIME
	memset(tv_data_tab, 0xFF, sizeof(STLAYER_Record_Time_t) * NR_TIMEVAL);
#endif

    /* Memory allocation. */
    HALLAYER_Data_p = (HALLAYER_LayerProperties_t *)STOS_MemoryAllocate(LayerInitParams_p->CPUPartition_p, sizeof(HALLAYER_LayerProperties_t));

    /* If allocation fails returns an error. */
    if (HALLAYER_Data_p == NULL)
    {
        return (ST_ERROR_NO_MEMORY);
    }

    /* local LayerName variable to be compliant with STEVT_RegisterDeviceEvent */
    strncpy(LayerName, DeviceName, sizeof(ST_DeviceName_t)-1);

    /* Open EVT handle. */
    ErrorCode = STEVT_Open(LayerInitParams_p->EventHandlerName, &STEVT_OpenParams, &(HALLAYER_Data_p->EventsHandle));

    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: open failed, undo initialisations done */
        /* De-allocate last: data inside cannot be used after ! */
        STOS_MemoryDeallocate(HALLAYER_Data_p->CPUPartition_p, (void *) HALLAYER_Data_p);
        return(ST_ERROR_BAD_PARAMETER);
    }

    ErrorCode = STEVT_RegisterDeviceEvent(HALLAYER_Data_p->EventsHandle, LayerName, STLAYER_UPDATE_PARAMS_EVT, &(HALLAYER_Data_p->RegisteredEventsID[HALLAYER_UPDATE_PARAMS_EVT_ID]));
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Close EVT handle. */
        ErrorCode = STEVT_Close(HALLAYER_Data_p->EventsHandle);
        /* De-allocate last: data inside cannot be used after ! */
        STOS_MemoryDeallocate(HALLAYER_Data_p->CPUPartition_p, (void *) HALLAYER_Data_p);
        return(STLAYER_ERROR_EVENT_REGISTRATION);
    }

    ErrorCode = STEVT_RegisterDeviceEvent(HALLAYER_Data_p->EventsHandle, LayerName, STLAYER_UPDATE_DECIMATION_EVT, &(HALLAYER_Data_p->RegisteredEventsID[HALLAYER_UPDATE_DECIMATION_EVT_ID]));
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Close EVT handle. */
        ErrorCode = STEVT_Close(HALLAYER_Data_p->EventsHandle);
        /* De-allocate last: data inside cannot be used after ! */
        STOS_MemoryDeallocate(HALLAYER_Data_p->CPUPartition_p, (void *) HALLAYER_Data_p);
        return(STLAYER_ERROR_EVENT_REGISTRATION);
    }


#ifdef STLAYER_USE_FMD
    ErrorCode = STEVT_RegisterDeviceEvent(HALLAYER_Data_p->EventsHandle, LayerName, STLAYER_NEW_FMD_REPORTED_EVT, &(HALLAYER_Data_p->RegisteredEventsID[HALLAYER_NEW_FMD_REPORTED_EVT_ID]));
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Close EVT handle. */
        ErrorCode = STEVT_Close(HALLAYER_Data_p->EventsHandle);
        /* De-allocate last: data inside cannot be used after ! */
        STOS_MemoryDeallocate(HALLAYER_Data_p->CPUPartition_p, (void *) HALLAYER_Data_p);
        return(STLAYER_ERROR_EVENT_REGISTRATION);
    }
#endif

    /* return LAYER Handle */
    *LayerHandle_p = (STLAYER_Handle_t) HALLAYER_Data_p;

    HALLAYER_Data_p->ValidityCheck = HALLAYER_VALID_HANDLE;

       /* Initialise parameters */
       HALLAYER_Data_p->VTGSettings.Name[0]      = '\0';
       HALLAYER_Data_p->VTGSettings.FrameRate    = 0;
       HALLAYER_Data_p->VTGSettings.VSyncType    = STVTG_TOP;
       HALLAYER_Data_p->VTGSettings.IsConnected     = FALSE;
#ifdef STLAYER_VIDEO_TASK
       HALLAYER_Data_p->TaskTerminate               = FALSE;
#endif  /* #ifdef STLAYER_VIDEO_TASK */
   HALLAYER_Data_p->DisplayParamsForVideo.FrameBufferDisplayLatency = 1;
   HALLAYER_Data_p->DisplayParamsForVideo.FrameBufferHoldTime       = 1;

    /* Store parameters */
    HALLAYER_Data_p->LayerType              = LayerInitParams_p->LayerType;
    HALLAYER_Data_p->CPUPartition_p         = LayerInitParams_p->CPUPartition_p;
#ifdef ST_XVP_ENABLE_FLEXVP
    HALLAYER_Data_p->NCachePartition_p      = LayerInitParams_p->NCachePartition_p;
#endif /* ST_XVP_ENABLE_FLEXVP */
#ifdef ST_XVP_ENABLE_FGT
    HALLAYER_Data_p->IsFGTBypassed          = TRUE; /* default value : FGT is bypassed */
#endif /* ST_XVP_ENABLE_FGT */

#ifdef ST_OSLINUX
    /* Mapping LAYER Registers for Video */
    if (ErrorCode == ST_NO_ERROR)
    {
        HALLAYER_Data_p->MappedVideoDisplayWidth = (unsigned long)0x1000;

        /* Determinating length and base address of layer of mapping */
        if ((LayerInitParams_p->LayerType == STLAYER_SDDISPO2_VIDEO1)||(LayerInitParams_p->LayerType == STLAYER_HDDISPO2_VIDEO1)
           ||(LayerInitParams_p->LayerType == STLAYER_DISPLAYPIPE_VIDEO1)
           ||(LayerInitParams_p->LayerType == STLAYER_DISPLAYPIPE_VIDEO2))

        {
              VideoLayerModuleParam.LayerBaseAddress = (unsigned long)DISP0BaseAddress;
              VideoLayerModuleParam.LayerAddressWidth = (unsigned long)HALLAYER_Data_p->MappedVideoDisplayWidth;
        }

        else if ((LayerInitParams_p->LayerType == STLAYER_SDDISPO2_VIDEO2)
                ||(LayerInitParams_p->LayerType == STLAYER_HDDISPO2_VIDEO2)
                ||(LayerInitParams_p->LayerType == STLAYER_DISPLAYPIPE_VIDEO3))
        {
              VideoLayerModuleParam.LayerBaseAddress = (unsigned long)DISP1BaseAddress;
              VideoLayerModuleParam.LayerAddressWidth = (unsigned long)HALLAYER_Data_p->MappedVideoDisplayWidth;
        }
        else if (LayerInitParams_p->LayerType == STLAYER_DISPLAYPIPE_VIDEO4)
        {
              VideoLayerModuleParam.LayerBaseAddress = (unsigned long)DISP2BaseAddress;
              VideoLayerModuleParam.LayerAddressWidth = (unsigned long)HALLAYER_Data_p->MappedVideoDisplayWidth;
        }

        else
        {
             STTBX_Print(("Layer mapping problem: Unknown display base address!\n"));
             return (ST_ERROR_BAD_PARAMETER);
        }

        HALLAYER_Data_p->MappedVideoDisplayBaseAddress_p = (unsigned long *) STLINUX_MapRegion( (void*)VideoLayerModuleParam.LayerBaseAddress,
                    VideoLayerModuleParam.LayerAddressWidth, "VIDEO LAYER");

        if (HALLAYER_Data_p->MappedVideoDisplayBaseAddress_p == NULL)
        {
            /*ErrorCode =  STLAYER_ERROR_MAP_LAYER;*/
            release_mem_region(VideoLayerModuleParam.LayerBaseAddress | REGION2, VideoLayerModuleParam.LayerAddressWidth);
            STTBX_Print(("Video Layer: ioremap failed ->  STLAYER_ERROR_MAP_LAYER!!!! \n"));
        }

        STTBX_Print(("Video LAYER virtual io kernel address of phys %lX = %lX\n", (unsigned long)(VideoLayerModuleParam.LayerBaseAddress | REGION2), (unsigned long)(HALLAYER_Data_p->MappedVideoDisplayBaseAddress_p)));
    }

    /* Mapping LAYER Registers for Compositor */

    if (ErrorCode == ST_NO_ERROR)
    {
        HALLAYER_Data_p->MappedGammaWidth = 0xC00 ;

        /* Determinating length of mapping */
        CompositorModuleParam.LayerBaseAddress = (unsigned long)LAYER_COMPOSITOR_MAPPING;
        CompositorModuleParam.LayerAddressWidth = (unsigned long)HALLAYER_Data_p->MappedGammaWidth;
        CompositorModuleParam.LayerMappingIndex = 1; /* Compositor register mapping */

        HALLAYER_Data_p->MappedGammaBaseAddress_p = (unsigned long *)STLINUX_MapRegion( (void*)CompositorModuleParam.LayerBaseAddress, CompositorModuleParam.LayerAddressWidth, "COMPOSITOR LAYER");

        if (HALLAYER_Data_p->MappedGammaBaseAddress_p == NULL)
        {
            /*ErrorCode =  STLAYER_ERROR_MAP_LAYER;*/
            release_mem_region(CompositorModuleParam.LayerBaseAddress | REGION2, CompositorModuleParam.LayerAddressWidth);
            STTBX_Print(("STLAYER - VIDEO :Compositor Layer: ioremap failed ->  STLAYER_ERROR_MAP_LAYER!!!! \n"));
        }

        STTBX_Print(("STLAYER - VIDEO :Compositor LAYER virtual io kernel address of phys %lX = %lX\n", (unsigned long)(CompositorModuleParam.LayerBaseAddress | REGION2), (unsigned long)(HALLAYER_Data_p->MappedGammaBaseAddress_p)));
    }
#endif /* ST_OSLINUX */

    if (ErrorCode == ST_NO_ERROR)
    {
        if (  (LayerInitParams_p->LayerType == STLAYER_OMEGA2_VIDEO1) || (LayerInitParams_p->LayerType == STLAYER_OMEGA2_VIDEO2)
            ||(LayerInitParams_p->LayerType == STLAYER_7020_VIDEO1)   || (LayerInitParams_p->LayerType == STLAYER_7020_VIDEO2)
            ||(LayerInitParams_p->LayerType == STLAYER_SDDISPO2_VIDEO1) || (LayerInitParams_p->LayerType == STLAYER_SDDISPO2_VIDEO2)
            ||(LayerInitParams_p->LayerType == STLAYER_HDDISPO2_VIDEO1) || (LayerInitParams_p->LayerType == STLAYER_HDDISPO2_VIDEO2)
            ||(LayerInitParams_p->LayerType == STLAYER_DISPLAYPIPE_VIDEO1)||(LayerInitParams_p->LayerType == STLAYER_DISPLAYPIPE_VIDEO2)
            ||(LayerInitParams_p->LayerType == STLAYER_DISPLAYPIPE_VIDEO3)||(LayerInitParams_p->LayerType == STLAYER_DISPLAYPIPE_VIDEO4)

            )
        {
            /* for Omega2 chips, BaseAddress is the base address of the gamma compositor video registers */
            /* and BaseAddress2 is the base address of the video dislay registers (DIS_XX and DIP_XX) */

#ifdef ST_OSLINUX
            HALLAYER_Data_p->VideoDisplayBaseAddress_p = (void *)((U32) HALLAYER_Data_p->MappedVideoDisplayBaseAddress_p + (U32) LayerInitParams_p->BaseAddress2_p - (U32) VideoLayerModuleParam.LayerBaseAddress);
            HALLAYER_Data_p->GammaBaseAddress_p = (void *)((U32) HALLAYER_Data_p->MappedGammaBaseAddress_p + (U32) LayerInitParams_p->BaseAddress_p - (U32) CompositorModuleParam.LayerBaseAddress);

#else
            HALLAYER_Data_p->VideoDisplayBaseAddress_p = (void *) ((U8 *) LayerInitParams_p->DeviceBaseAddress_p + (U32) LayerInitParams_p->BaseAddress2_p);
            HALLAYER_Data_p->GammaBaseAddress_p = (void *) ((U8 *) LayerInitParams_p->DeviceBaseAddress_p + (U32) LayerInitParams_p->BaseAddress_p);
#endif  /* ST_OSLINUX */

        }
        else if (LayerInitParams_p->LayerType == STLAYER_OMEGA1_VIDEO)
        {
            /* for Omega1 chips, BaseAddress is the base address of the video dislay registers (VID_XX) */
            HALLAYER_Data_p->VideoDisplayBaseAddress_p = (void *) ((U8 *) LayerInitParams_p->DeviceBaseAddress_p + (U32) LayerInitParams_p->BaseAddress_p);
            HALLAYER_Data_p->GammaBaseAddress_p = NULL;
        }
        else
        {
            /* Close EVT handle. */
            ErrorCode = STEVT_Close(HALLAYER_Data_p->EventsHandle);
            /* De-allocate last: data inside cannot be used after ! */
            STOS_MemoryDeallocate(HALLAYER_Data_p->CPUPartition_p, (void *) HALLAYER_Data_p);
        return (ST_ERROR_BAD_PARAMETER);
        }
    }

    if (ErrorCode == ST_NO_ERROR)
    {
        switch(LayerInitParams_p->LayerType)
        {
            case STLAYER_OMEGA1_VIDEO :
                #if defined (HW_5508) || \
                    defined (HW_5518) || \
                    defined (HW_5510) || \
                    defined (HW_5512) || \
                    defined (HW_5514) || \
                    defined (HW_5516) || \
                    defined (HW_5517) || \
                    defined (HW_5578)
                    /* Functions Table Attribution */
                    HALLAYER_Data_p->FunctionsTable_p = &HALLAYER_Omega1Functions;
                #endif /* defined (HW_5508) ... defined (HW_5578) */
                break;
            case STLAYER_OMEGA2_VIDEO1 :
            case STLAYER_7020_VIDEO1   :
            case STLAYER_OMEGA2_VIDEO2 :
            case STLAYER_7020_VIDEO2   :
                #if defined (HW_7015) || defined (HW_7020)
                    /* Functions Table Attribution */
                    HALLAYER_Data_p->FunctionsTable_p = &HALLAYER_Omega2Functions;
                #endif /* defined (HW_7015) || defined (HW_7020) */
                break;

            case STLAYER_SDDISPO2_VIDEO1 :
            case STLAYER_SDDISPO2_VIDEO2 :
            case STLAYER_HDDISPO2_VIDEO1 :
                #if defined (HW_5528) || defined (HW_7710) || defined (HW_7100)
                    /* Functions Table Attribution */
                    HALLAYER_Data_p->FunctionsTable_p = &HALLAYER_Xddispo2Functions;
                #endif /* HW_5528 || HW_7710 || HW_7100 */
                break;

            case STLAYER_HDDISPO2_VIDEO2 :
                #if defined (HW_5528) || defined (HW_7710) || defined (HW_7100) || defined(HW_7109)
                    /* Functions Table Attribution */
                    HALLAYER_Data_p->FunctionsTable_p = &HALLAYER_Xddispo2Functions;
                #endif /* HW_5528 || HW_7710 || HW_7100 || HW_7109 */
                break;

            case STLAYER_DISPLAYPIPE_VIDEO1 :
                #if defined (HW_7109)
                    /* Functions Table Attribution */
                    HALLAYER_Data_p->FunctionsTable_p = &HALLAYER_DisplayPipeFunctions;
                #endif /* defined (HW_7109) */
                break;
            case STLAYER_DISPLAYPIPE_VIDEO2 :
            case STLAYER_DISPLAYPIPE_VIDEO3 :
            case STLAYER_DISPLAYPIPE_VIDEO4 :
                #if defined (HW_7200)
                /* Functions Table Attribution */
                HALLAYER_Data_p->FunctionsTable_p = &HALLAYER_DisplayPipeFunctions;
                #endif /* defined (HW_7200) */
                break;

           default :
                /* Close EVT handle. */
                ErrorCode = STEVT_Close(HALLAYER_Data_p->EventsHandle);
                /* De-allocate last: data inside cannot be used after ! */
                STOS_MemoryDeallocate(HALLAYER_Data_p->CPUPartition_p, (void *) HALLAYER_Data_p);
                return(ST_ERROR_BAD_PARAMETER);
                break;
        }
    }

    if (ErrorCode == ST_NO_ERROR)
    {
        /* Initialise layer */
        ErrorCode = (HALLAYER_Data_p->FunctionsTable_p->Init)(*LayerHandle_p, LayerInitParams_p);
    }

    if (ErrorCode != ST_NO_ERROR)
    {
        /* Close EVT handle. */
        STEVT_Close(HALLAYER_Data_p->EventsHandle);
        /* De-allocate last: data inside cannot be used after ! */
        STOS_MemoryDeallocate(HALLAYER_Data_p->CPUPartition_p,
                           (void *) HALLAYER_Data_p);
        return(ST_ERROR_BAD_PARAMETER);
    }
#ifdef STLAYER_VIDEO_TASK
    ErrorCode = STOS_TaskCreate ((void (*) (void*))STLAYER_VideoTask,
                        (void*)HALLAYER_Data_p,
                        HALLAYER_Data_p->CPUPartition_p,
                        HALLAYER_VIDEO_TASK_STACK_SIZE,
                        &(HALLAYER_Data_p->TaskStack),
                        HALLAYER_Data_p->CPUPartition_p,
                        &(HALLAYER_Data_p->Task_p),
                        &(HALLAYER_Data_p->TaskDesc),
                        STLAYER_VIDEO_HAL_TASK_PRIORITY,
                        STLAYER_VIDEO_NAME,
                        task_flags_no_min_stack_size);

    if(ErrorCode != ST_NO_ERROR)
    {
        STTBX_Print(("Error in call to STOS_TaskCreate() ."));
        return(!ST_NO_ERROR);
    }
#endif  /* #ifdef STLAYER_VIDEO_TASK */

    return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : LAYERVID_Term
Description : Terminates a hal video layer
Parameters  :
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if succes, or errors returned by STEVT_Close or OMEGAn Term function.
*******************************************************************************/
#ifdef ST_OSLINUX
    ST_ErrorCode_t LAYERVID_Term(const ST_DeviceName_t DeviceName,
                             const STLAYER_TermParams_t *  TermParams_p)
#else
    ST_ErrorCode_t LAYERVID_Term(const STLAYER_Handle_t        LayerHandle,
                             const STLAYER_TermParams_t *  TermParams_p)
#endif /* ST_OSLINUX */
{
    ST_ErrorCode_t     ErrorCode = ST_NO_ERROR;

#ifdef ST_OSLINUX
    U32                   Index;
#endif

#ifdef STLAYER_VIDEO_TASK
    task_t *           Task_p;
#endif  /* #ifdef STLAYER_VIDEO_TASK */
    HALLAYER_LayerProperties_t * HALLAYER_Data_p;

#ifdef ST_OSLINUX
    Index = stlayer_HalIndexOfNamedLayer(DeviceName);

    /* Find structure */
    HALLAYER_Data_p = (HALLAYER_LayerProperties_t *) (stlayer_LayerArray[Index].VideoHandle);
#else
    HALLAYER_Data_p = (HALLAYER_LayerProperties_t *) LayerHandle;
#endif /* ST_OSLINUX */

    if (HALLAYER_Data_p->ValidityCheck != HALLAYER_VALID_HANDLE)
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
#ifdef ST_OSLINUX
        /* releasing LAYER linux memory device device driver */

        /* Layer Video Display Unmap*/
        STLINUX_UnmapRegion( (void*)HALLAYER_Data_p->MappedVideoDisplayBaseAddress_p, (U32)HALLAYER_Data_p->MappedVideoDisplayWidth);
        STLINUX_UnmapRegion( (void*)HALLAYER_Data_p->MappedGammaBaseAddress_p, (U32)HALLAYER_Data_p->MappedGammaWidth);
#endif /* ST_OSLINUX */

#ifdef STLAYER_VIDEO_TASK

    /* Termination and deletion of the task: set the flag */
    HALLAYER_Data_p->TaskTerminate = TRUE;

    /* schedule the task and wait for its term */
    STOS_SemaphoreSignal(HALLAYER_Data_p->HALOrderSemaphore_p);

    Task_p = HALLAYER_Data_p->Task_p;

#ifndef WA_taskwait_on_os20emu
    STOS_TaskWait( &Task_p, TIMEOUT_INFINITY );
#endif

    ErrorCode = STOS_TaskDelete ( Task_p,
            HALLAYER_Data_p->CPUPartition_p,
            HALLAYER_Data_p->TaskStack,
            HALLAYER_Data_p->CPUPartition_p );
    if( ErrorCode != ST_NO_ERROR)
    {
        STTBX_Print(("Error in call to STOS_TaskDelete()\n\r"));
        return(ErrorCode);
    }
    STOS_SemaphoreDelete(NULL,HALLAYER_Data_p->HALOrderSemaphore_p);

#endif /* #ifdef STLAYER_VIDEO_TASK */

#ifdef ST_OSLINUX
        /* Term Layer */
        ErrorCode = (HALLAYER_Data_p->FunctionsTable_p->Term)(stlayer_LayerArray[Index].VideoHandle, TermParams_p);
#else
        ErrorCode = (HALLAYER_Data_p->FunctionsTable_p->Term)(LayerHandle, TermParams_p);
#endif /* ST_OSLINUX */

        if (ErrorCode == ST_NO_ERROR)
        {
            /* De-validate structure */
            HALLAYER_Data_p->ValidityCheck = 0; /* not HALLAYER_VALID_HANDLE */

            ErrorCode = STEVT_Close(HALLAYER_Data_p->EventsHandle);

            STOS_MemoryDeallocate(HALLAYER_Data_p->CPUPartition_p, (void *) HALLAYER_Data_p);
        }
    }

    return(ErrorCode);
}


/*******************************************************************************
Name        : LAYERVID_GetVTGName
Description : Give the layer name.
Parameters  : IN : LayerHandle
              OUT : VTGName
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
void LAYERVID_GetVTGName( const STLAYER_Handle_t  LayerHandle, ST_DeviceName_t * const VTGName_p)
{
    HALLAYER_LayerProperties_t * HALLAYER_Data_p;

    /* Find structure */
    HALLAYER_Data_p = (HALLAYER_LayerProperties_t *) LayerHandle;

    if (HALLAYER_Data_p->ValidityCheck != HALLAYER_VALID_HANDLE)
    {
        return;
    }

    strncpy(*VTGName_p, HALLAYER_Data_p->VTGSettings.Name, sizeof(ST_DeviceName_t)-1);
}


/*******************************************************************************
Name        : LAYERVID_GetVTGParams
Description : Give the VTG parameters.
Parameters  : IN : LayerHandle
              OUT : VTGParams
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
void LAYERVID_GetVTGParams(const STLAYER_Handle_t LayerHandle, STLAYER_VTGParams_t * const VTGParams_p)
{
    HALLAYER_LayerProperties_t * HALLAYER_Data_p;

    /* Find structure */
    HALLAYER_Data_p = (HALLAYER_LayerProperties_t *) LayerHandle;

    if (HALLAYER_Data_p->ValidityCheck != HALLAYER_VALID_HANDLE)
    {
        return;
    }

    strncpy(VTGParams_p->VTGName, HALLAYER_Data_p->VTGSettings.Name, sizeof(ST_DeviceName_t)-1);
    VTGParams_p->VTGFrameRate = HALLAYER_Data_p->VTGSettings.FrameRate;

} /* end of LAYERVID_GetVTGParams() function */


/*******************************************************************************
Name        : LAYERVID_UpdateFromMixer
Description : Update parameters from mixer called by STMIXER.
Parameters  : ViewportHandle
              Params
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
void LAYERVID_UpdateFromMixer(const STLAYER_Handle_t LayerHandle, STLAYER_OutputParams_t * const OutputParams_p)
{
    HALLAYER_LayerProperties_t *    HALLAYER_Data_p;
    STLAYER_UpdateParams_t          UpdateParams;
    STEVT_DeviceSubscribeParams_t   SubscribeParams;
    ST_ErrorCode_t  Error;

    /* Find structure */
    HALLAYER_Data_p = (HALLAYER_LayerProperties_t *) LayerHandle;

    if (HALLAYER_Data_p->ValidityCheck != HALLAYER_VALID_HANDLE)
    {
        return;
    }

    /* Initialise event data */
    UpdateParams.UpdateReason = 0;

    /* The reason of the call is a new connection to a mixer */
    if ((OutputParams_p->UpdateReason & STLAYER_CONNECT_REASON) != 0)
    {
        HALLAYER_Data_p->DisplayParamsForVideo.FrameBufferDisplayLatency   = OutputParams_p->FrameBufferDisplayLatency;
        HALLAYER_Data_p->DisplayParamsForVideo.FrameBufferHoldTime         = OutputParams_p->FrameBufferHoldTime;
        UpdateParams.DisplayParamsForVideo.FrameBufferDisplayLatency       = OutputParams_p->FrameBufferDisplayLatency;
        UpdateParams.DisplayParamsForVideo.FrameBufferHoldTime             = OutputParams_p->FrameBufferHoldTime;
        UpdateParams.UpdateReason |= STLAYER_CONNECT_REASON;
    }

    /* The reason of the call is a new VTG. */
    if (((OutputParams_p->UpdateReason & STLAYER_VTG_REASON) != 0) || /* VTG name changed */
        (((OutputParams_p->UpdateReason & STLAYER_SCREEN_PARAMS_REASON) != 0) && /* or the VTG FrameRate screen param changed */
          (HALLAYER_Data_p->VTGSettings.FrameRate != OutputParams_p->FrameRate))
       )
    {
        /* Notify event HALLAYER_UPDATE_PARAMS_EVT_ID with reason VTG for both VTG name and VTG frame rate change.
        This is because FrameRate is inside screen params, but video doesn't care about screen params... */
        UpdateParams.UpdateReason |= STLAYER_VTG_REASON;

        /* Assign a VTG name: either new one if available, or previous one */
        if ((OutputParams_p->UpdateReason & STLAYER_VTG_REASON) != 0)
        {
            /* Top Level HAL must subscribe to this VTG */
            if(HALLAYER_Data_p->VTGSettings.IsConnected)
            {
                /* unsubscribe the previous one */
                STEVT_UnsubscribeDeviceEvent(HALLAYER_Data_p->EventsHandle,
                                     HALLAYER_Data_p->VTGSettings.Name,
                                     STVTG_VSYNC_EVT);
            }
            /* Reason VTG of this event, take new VTG name */
            strncpy(UpdateParams.VTG_Name, OutputParams_p->VTGName, sizeof(ST_DeviceName_t)-1); /* old field, kept for backwards compatibility */
            strncpy(UpdateParams.VTGParams.VTGName, OutputParams_p->VTGName, sizeof(ST_DeviceName_t)-1);
            /* Store the new VTG Name for the future. Could be asked by the video driver with STLAYER_GetVTGName. */
            strncpy(HALLAYER_Data_p->VTGSettings.Name, OutputParams_p->VTGName, sizeof(ST_DeviceName_t)-1);

            /* Top Level HAL must subscribe to this VTG */
            HALLAYER_Data_p->VTGSettings.IsConnected    = TRUE;
            SubscribeParams.NotifyCallback     = (STEVT_DeviceCallbackProc_t)LayerVideo_VSync;
            SubscribeParams.SubscriberData_p   = (void*)HALLAYER_Data_p;
            Error = STEVT_SubscribeDeviceEvent(HALLAYER_Data_p->EventsHandle,
                                   HALLAYER_Data_p->VTGSettings.Name,
                                   STVTG_VSYNC_EVT,
                                   &SubscribeParams);
            if (Error != ST_NO_ERROR)
            {
                STTBX_Print(("STLAYER : Synchro KO: layer not synchro"));
                return;
            }
        }
        else
        {
            /* NOT reason VTG of this event, take previous VTG name */
            strncpy(UpdateParams.VTG_Name, HALLAYER_Data_p->VTGSettings.Name, sizeof(ST_DeviceName_t)-1); /* old field, kept for backwards compatibility */
            strncpy(UpdateParams.VTGParams.VTGName, HALLAYER_Data_p->VTGSettings.Name, sizeof(ST_DeviceName_t)-1);
        }

        /* Assign a VTG frame rate: either new one if available, or previous one */
        if ((OutputParams_p->UpdateReason & STLAYER_SCREEN_PARAMS_REASON) != 0)
        {
            /* Reason screen params of this event, take new VTG frame rate */
            UpdateParams.VTGParams.VTGFrameRate = OutputParams_p->FrameRate;
            /* Store the VTG frame rate for the future. */
            HALLAYER_Data_p->VTGSettings.FrameRate = OutputParams_p->FrameRate;
        }
        else
        {
            /* NOT reason screen params of this event, take previous VTG frame rate */
            UpdateParams.VTGParams.VTGFrameRate = HALLAYER_Data_p->VTGSettings.FrameRate;
        }
    }

    /* Reason of the call is screen params change */
/*    if ((OutputParams_p->UpdateReason & STLAYER_SCREEN_PARAMS_REASON) != 0)*/
/*    {*/
        /* VTG FrameRate treated with VTG reason, nothing to do with other parameters... */
/*    }*/

    if ((OutputParams_p->UpdateReason & STLAYER_DISCONNECT_REASON) != 0)
    {
        HALLAYER_Data_p->DisplayParamsForVideo.FrameBufferDisplayLatency = 1;
        HALLAYER_Data_p->DisplayParamsForVideo.FrameBufferHoldTime       = 1;
        UpdateParams.DisplayParamsForVideo.FrameBufferDisplayLatency     = 1;
        UpdateParams.DisplayParamsForVideo.FrameBufferHoldTime           = 1;
        /* Inform drivers above */
        UpdateParams.UpdateReason |= STLAYER_DISCONNECT_REASON;
    }

    /* Notify update event if required */
    if (UpdateParams.UpdateReason != 0)
    {
        STEVT_Notify(((HALLAYER_LayerProperties_t *) LayerHandle)->EventsHandle, ((HALLAYER_LayerProperties_t *) LayerHandle)->RegisteredEventsID[HALLAYER_UPDATE_PARAMS_EVT_ID], (const void *) (&UpdateParams));
    }

    /* Update HAL Layer */
    (((HALLAYER_LayerProperties_t*)LayerHandle)->FunctionsTable_p->UpdateFromMixer)(LayerHandle, OutputParams_p);

} /* end of LAYERVID_UpdateFromMixer() */

/*******************************************************************************/
#ifdef DEBUG_RECORD_TIME
clock_t get_time_convert(struct timeval *tv)
{
    clock_t          Clk;

    tv->tv_sec %= 1000;
	Clk = tv->tv_sec*1000000+tv->tv_usec;

    return (Clk);
}
#endif /* DEBUG_RECORD_TIME */

#ifdef STLAYER_VIDEO_TASK
/*******************************************************************************
Name        : STLAYER_VideoTask
Description : Calls the low level HAL on VSync if needed
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void STLAYER_VideoTask(void * Context_p)
{
    HALLAYER_LayerProperties_t * LayerProperties_p;

    STOS_TaskEnter(NULL);

    /* initialize the context pointer */
    LayerProperties_p = (HALLAYER_LayerProperties_t*) Context_p;

    LayerProperties_p->HALOrderSemaphore_p = STOS_SemaphoreCreateFifo(NULL,0);

    while ( LayerProperties_p->TaskTerminate == FALSE )
    {
        STOS_SemaphoreWait(LayerProperties_p->HALOrderSemaphore_p);

#ifdef DEBUG_RECORD_TIME
/*        STTBX_Print(("h-t:%x\n",tv_head - tv_tail));*/

		while (tv_head != tv_tail)
		{
		    unsigned long diff_time;

		    diff_time = (unsigned long) (get_time_convert((struct timeval *) &(tv_tail->tv_data)))
		            - (unsigned long) (get_time_convert((struct timeval *) &(old_tv_data)));

            switch(tv_tail->where)
            {
    		    case 0x00:
                    /* Top of VTG handler */
                    STTBX_Print(("---> L:%x T:%06lu\n", tv_tail->where, (unsigned long) (diff_time) % 1000000)); /* in us */
/*
                    STTBX_Print(("---> T:%lu Time:%03lu.%03lu D:%06lu\n", tv_tail->irq_nb,
        					(unsigned long) (get_time_convert((struct timeval *) &(tv_tail->tv_data))) / 1000000,
        					(unsigned long) (get_time_convert((struct timeval *) &(tv_tail->tv_data))) % 1000000,
                            (unsigned long) (diff_time) % 1000000));
*/

                    /* Ref time if at this location */
    		        old_tv_data = tv_tail->tv_data;

                    break;

    		    case 0x0F:
                    /* End of VTG handler */
/*                  STTBX_Print(("     D:%lu T:%06lu sync:%lu\n", tv_tail->data2,
        					(unsigned long) (diff_time) % 1000000,
                            (unsigned long) (tv_tail->data2)));*/
/*
                    STTBX_Print(("     T:%2x Time:%03lu.%03lu D:%06lu\n", tv_tail->irq_nb,
        					(unsigned long) (get_time_convert((struct timeval *) &(tv_tail->tv_data))) / 1000000,
        					(unsigned long) (get_time_convert((struct timeval *) &(tv_tail->tv_data))) % 1000000,
                            (unsigned long) (diff_time) % 1000000));
*/
                    break;

                default:
                    break;

            }

/*		    old_tv_data = tv_tail->tv_data;*/
			layer_incr_tv(&tv_tail);
		}
#endif /* DEBUG_RECORD_TIME */

        LayerProperties_p->FunctionsTable_p->SynchronizedUpdate((STLAYER_Handle_t)LayerProperties_p);
    }

    /* if this point is reached , it is the task termination */
    /* the callback must be no more called */
    if(LayerProperties_p->VTGSettings.IsConnected)
    {
        LayerProperties_p->VTGSettings.IsConnected = FALSE;
        STEVT_UnsubscribeDeviceEvent(LayerProperties_p->EventsHandle,
                                     LayerProperties_p->VTGSettings.Name,
                                     STVTG_VSYNC_EVT);
    }

    STOS_TaskExit(NULL);

}/* End of STLAYER_VideoTask */
#endif  /* #ifdef STLAYER_VIDEO_TASK */

/*******************************************************************************
Name        : LayerVideo_VSync
Description :
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void LayerVideo_VSync(STEVT_CallReason_t Reason,
                                const ST_DeviceName_t RegistrantName,
                                STEVT_EventConstant_t Event,
                                void *EventData,
                                void *SubscriberData_p)
{
    HALLAYER_LayerProperties_t * LayerProperties_p;
    BOOL                         IsUpdateNeeded;

    LayerProperties_p = (HALLAYER_LayerProperties_t*)SubscriberData_p;

    /* Unused parameters */
    UNUSED_PARAMETER(Reason);
    UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(Event);

    /* We are in interrupts here */
#ifdef DEBUG_RECORD_TIME
	spin_lock(&layer_time_lock);

    do_gettimeofday((struct timeval *) &(tv_head->tv_data)); /* cast to stop 'volatile' warning */
	tv_head->where = 0x10; /* inside interrupt */
	tv_head->irq_nb = 0xAA; /* Top of Top Half */
    layer_incr_tv(&tv_head);

    spin_unlock(&layer_time_lock);
#endif /* DEBUG_RECORD_TIME */
#ifdef STLAYER_VIDEO_TASK
    /* Ask the HAL if synchronized update is needed */
    IsUpdateNeeded = LayerProperties_p->FunctionsTable_p->IsUpdateNeeded((STLAYER_Handle_t)LayerProperties_p);
    if( IsUpdateNeeded == TRUE)
    {
        /* if an update is needed, schedules the HAL-task */
        LayerProperties_p->VTGSettings.VSyncType = *(STVTG_VSYNC_t*)EventData;

        if ( LayerProperties_p->TaskTerminate == FALSE )
        {
            STOS_SemaphoreSignal(LayerProperties_p->HALOrderSemaphore_p);
        }
    }
#else
    /* IsUpdateNeeded() should be call even if this information is not needed. It is indeed used by the HAL to know that a VSync has occured. */
    IsUpdateNeeded = LayerProperties_p->FunctionsTable_p->IsUpdateNeeded((STLAYER_Handle_t)LayerProperties_p);

    /* Save the new VSyncType */
    LayerProperties_p->VTGSettings.VSyncType = *(STVTG_VSYNC_t*)EventData;
#endif  /* #ifdef STLAYER_VIDEO_TASK */
} /* End of LayerVideo_VSync */

/*******************************************************************************
Name        : LAYERVID_GetCapability
Description : Give the layer capabilities
Parameters  : IN : LayerHandle
              OUT : Layer capabilities
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYERVID_GetCapability(const STLAYER_Handle_t LayerHandle, STLAYER_Capability_t * Capability_p)
{
    HALLAYER_LayerProperties_t *    HALLAYER_Data_p;
    ST_ErrorCode_t                  ErrorCode = ST_NO_ERROR;

    /* Find structure */
    HALLAYER_Data_p = (HALLAYER_LayerProperties_t *) LayerHandle;

    if (HALLAYER_Data_p->ValidityCheck != HALLAYER_VALID_HANDLE)
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        /* Set common capability params */
        /* Fill capability structure */
        switch(HALLAYER_Data_p->LayerType)
        {
            case STLAYER_HDDISPO2_VIDEO1:
            case STLAYER_DISPLAYPIPE_VIDEO1:
                Capability_p->DeviceId = 0x700;
                break;

             case STLAYER_HDDISPO2_VIDEO2:
                Capability_p->DeviceId = 0x800;
                break;

#if defined (HW_7200)
            case STLAYER_DISPLAYPIPE_VIDEO2 :
            case STLAYER_DISPLAYPIPE_VIDEO4 :


                    Capability_p->DeviceId = 0x700;
                break;

            case STLAYER_DISPLAYPIPE_VIDEO3 :

                    Capability_p->DeviceId = 0x800;
                break;
#endif
            default:
                Capability_p->DeviceId = 0x0;
                break;
        }

        Capability_p->FrameBufferDisplayLatency = HALLAYER_Data_p->DisplayParamsForVideo.FrameBufferDisplayLatency;
        Capability_p->FrameBufferHoldTime       = HALLAYER_Data_p->DisplayParamsForVideo.FrameBufferHoldTime;
        /* Get specific capability params from Hal */
        ((HALLAYER_LayerProperties_t *)(LayerHandle))->FunctionsTable_p->GetCapability(LayerHandle, Capability_p);
    }

    return(ErrorCode);
} /* End of LAYERVID_GetCapability */

#ifdef ST_XVP_ENABLE_FGT
/*******************************************************************************
Name        : LAYERVID_SetFGTState
Description : set IsFGTBypassed local value
Parameters  : IN : LayerHandle
              IN : State
Assumptions :
Limitations :
Returns     : decimation factor.
*******************************************************************************/
ST_ErrorCode_t LAYERVID_SetFGTState(const STLAYER_Handle_t LayerHandle, BOOL State)
{
    HALLAYER_LayerProperties_t  *HALLAYER_Data_p;
    ST_ErrorCode_t              ErrorCode = ST_NO_ERROR;

    /* Find structure */
    HALLAYER_Data_p = (HALLAYER_LayerProperties_t *) LayerHandle;

    if (HALLAYER_Data_p->ValidityCheck != HALLAYER_VALID_HANDLE)
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        HALLAYER_Data_p->IsFGTBypassed = State;
    }
    return ErrorCode;
}
#endif /* ST_XVP_ENABLE_FGT */

/*******************************************************************************
Name        : stlayer_GetDecimationFactorFromDecimation
Description : Convert a given decimation into a decimation factor that can be
              applied to picture sizes.
Parameters  : Decimation enumerate Decimation.
              Converted decimation factor : DecimationFactor.
Assumptions :
Limitations :
Returns     : decimation factor.
*******************************************************************************/
U32 stlayer_GetDecimationFactorFromDecimation (STLAYER_DecimationFactor_t Decimation)
{
    switch (Decimation)
    {
        case STLAYER_DECIMATION_FACTOR_NONE :
            return( 1 );
            break;

        case STLAYER_DECIMATION_FACTOR_2 :
            return( 2 );
            break;

        case STLAYER_DECIMATION_FACTOR_4 :
            return( 4 );
            break;

        default :
            return( 1 );
            break;
    }
} /* End of stlayer_GetDecimationFactorFromDecimation */

/*******************************************************************************
Name        : stlayer_GetDecimationFromDecimationFactor
Description : Convert a given decimation factor into a decimation.
Parameters  :
Assumptions :
Limitations :
Returns     : decimation.
*******************************************************************************/
STLAYER_DecimationFactor_t stlayer_GetDecimationFromDecimationFactor (U32 DecimationFactor)
{
    switch (DecimationFactor)
    {
        case 1 :
            return( STLAYER_DECIMATION_FACTOR_NONE );
            break;

        case 2 :
            return( STLAYER_DECIMATION_FACTOR_2 );
            break;

        case 4 :
            return( STLAYER_DECIMATION_FACTOR_4 );
            break;

        default :
            return( STLAYER_DECIMATION_FACTOR_NONE );
            break;
    }
} /* End of stlayer_GetDecimationFromDecimationFactor */

/* end of halv_lay.c */
