/*******************************************************************************

File name   : halv_lay.h

Description : Video HAL (Hardware Abstraction Layer) access to hardware source file

COPYRIGHT (C) STMicroelectronics 2004

Date               Modification                                     Name
----               ------------                                     ----
23 Feb 2000        Created                                          PLe
01 Aug 2001        Rename the functions                             AN
18 jan 2002        Added LAYERVID_GetVTGParams()                    HG
*******************************************************************************/

#ifndef __HAL_LAYER_H
#define __HAL_LAYER_H

/* Private preliminary definitions (internal use only) ---------------------- */

/* Includes ----------------------------------------------------------------- */
#ifndef ST_OSLINUX
    #include <stdio.h>
#endif

#include "stddefs.h"
#include "stevt.h"
#include "stvtg.h"
#include "stlayer.h"

#ifdef ST_OSLINUX
    #include "layerapi.h"
#endif

#include "stos.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif


/* Exported Constants ------------------------------------------------------- */

/* Display mask IT : They must be the same for all the HALs */
#define       HALLAYER_END_ANALYSIS_WINDOW_MASK                            0x400
#define       HALLAYER_LMU_LUMA_WRITE_FIFO_FULL_MASK                       0x200
#define       HALLAYER_LMU_LUMA_READ_FIFO_EMPTY_MASK                       0x100
#define       HALLAYER_LMU_CHROMA_WRITE_FIFO_FULL_MASK                     0x080
#define       HALLAYER_LMU_CHROMA_READ_FIFO_EMPTY_MASK                     0x040
#define       HALLAYER_DISPLAY_LUMA_B_FIFO_EMPTY_MASK                      0x020
#define       HALLAYER_DISPLAY_LUMA_A_FIFO_EMPTY_MASK                      0x010
#define       HALLAYER_DISPLAY_CHROMA_FIFO_EMPTY_MASK                      0x008
#define       HALLAYER_PIXEL_FIFO_EMPTY_MASK                               0x004
#define       HALLAYER_LUMA_INPUT_FIELD_MASK                               0x002
#define       HALLAYER_CHROMA_INPUT_FIELD_MASK                             0x001

#define       HALLAYER_UPDATE_PARAMS_EVT_ID                                0
#define       HALLAYER_UPDATE_DECIMATION_EVT_ID                            1
#define       HALLAYER_NEW_FMD_REPORTED_EVT_ID                             2
#define       HALLAYER_NB_REGISTERED_EVENTS_IDS                            3

/* 1Ko of stack for the task */
#ifdef ST_OS20
#define       HALLAYER_VIDEO_TASK_STACK_SIZE                               0x400
#endif /* ST_OS20 */
#ifdef ST_OS21
#define       HALLAYER_VIDEO_TASK_STACK_SIZE                               0x4000
#endif /* ST_OS21 */
#ifdef ST_OSLINUX
#define       HALLAYER_VIDEO_TASK_STACK_SIZE                               0x400
#endif /* ST_OSLINUX */

#ifdef ST_OSLINUX
#ifndef STLAYER_VIDEO_HAL_TASK_PRIORITY
#define STLAYER_VIDEO_HAL_TASK_PRIORITY       STLAYER_VIDEO_THREAD_PRIORITY
#endif
#define STLAYER_VIDEO_NAME                    "STAPI_LAYER_VIDEO"
#else
#ifndef STLAYER_VIDEO_HAL_TASK_PRIORITY
#define STLAYER_VIDEO_HAL_TASK_PRIORITY       10
#endif
#define STLAYER_VIDEO_NAME                    "STLAYER_Video"
#endif /* LINUX */

/* Exported Types ----------------------------------------------------------- */

/*- enum used by display only --------------------------------------------*/

/* typedef enum */
/* { */
/* } HALLAYER_SourceId_t ; */

typedef enum
{
    HALLAYER_VIEWPORT_PRIORITY_0,
    HALLAYER_VIEWPORT_PRIORITY_1,
    HALLAYER_VIEWPORT_PRIORITY_2,
    HALLAYER_VIEWPORT_PRIORITY_3
} HALLAYER_ViewportPriority_t ;


typedef struct
{
    ST_ErrorCode_t (*AdjustViewportParams)(
                         const STLAYER_Handle_t             LayerHandle,
                         STLAYER_ViewPortParams_t * const   Params_p);
    ST_ErrorCode_t (*CloseViewport)(const STLAYER_ViewPortHandle_t ViewportHandle);
    ST_ErrorCode_t (*DisableFMDReporting)(const STLAYER_ViewPortHandle_t ViewportHandle);
    void (*DisableVideoViewport)(const STLAYER_ViewPortHandle_t ViewportHandle);
    ST_ErrorCode_t (*EnableFMDReporting)(const STLAYER_ViewPortHandle_t ViewportHandle);
    void (*EnableVideoViewport)(const STLAYER_ViewPortHandle_t ViewportHandle);
    void (*GetCapability)(
                         const STLAYER_Handle_t LayerHandle,
                         STLAYER_Capability_t * Capability);
    ST_ErrorCode_t (*GetFMDParams)(
                         const STLAYER_ViewPortHandle_t     ViewportHandle,
                         STLAYER_FMDParams_t * Params_p);
    void (*GetLayerParams)(
                         const STLAYER_Handle_t             LayerHandle,
                         STLAYER_LayerParams_t *            LayerParams_p);
    ST_ErrorCode_t (*GetRequestedDeinterlacingMode) (
                         const STLAYER_ViewPortHandle_t     ViewportHandle,
                         STLAYER_DeiMode_t * const          RequestedMode_p);
    ST_ErrorCode_t (*GetVideoDisplayParams)(
                         const STLAYER_ViewPortHandle_t     ViewportHandle,
                         STLAYER_VideoDisplayParams_t * Params_p);
    void (*GetViewportAlpha)(
                         const STLAYER_ViewPortHandle_t     ViewportHandle,
                         STLAYER_GlobalAlpha_t*             Alpha);
    ST_ErrorCode_t (*GetViewportParams)(
                         const STLAYER_ViewPortHandle_t     ViewportHandle,
                         STLAYER_ViewPortParams_t * const   Params_p);
    ST_ErrorCode_t (*GetViewportPSI)(
                         const STLAYER_Handle_t             LayerHandle,
                         const STLAYER_ViewPortHandle_t     ViewportHandle,
                         STLAYER_PSI_t * const        ViewportPSI_p);
    ST_ErrorCode_t (*GetViewportIORectangle)(
                         const STLAYER_ViewPortHandle_t     ViewportHandle,
                         STGXOBJ_Rectangle_t * InputRectangle_p,
                         STGXOBJ_Rectangle_t * OutputRectangle_p);
    void (*GetViewportPosition)(
                         const STLAYER_ViewPortHandle_t     ViewportHandle,
                         U32 *                              XPos_p,
                         U32 *                              YPos_p);
    void (*GetViewportSource)(
                         const STLAYER_ViewPortHandle_t     ViewportHandle,
                         U32                 *              SourceNumber_p);
    ST_ErrorCode_t (*GetViewportSpecialMode)(
                         const STLAYER_ViewPortHandle_t     ViewportPHandle,
                         STLAYER_OutputMode_t *       const OuputMode_p,
                         STLAYER_OutputWindowSpecialModeParams_t * Params_p);
    ST_ErrorCode_t (*Init)(
                         const STLAYER_Handle_t             LayerHandle_p,
                         const STLAYER_InitParams_t * const LayerInitParams_p);
    BOOL (*IsUpdateNeeded)(
                         const STLAYER_Handle_t             LayerHandle);
    ST_ErrorCode_t (*OpenViewport)(
                         const STLAYER_Handle_t             LayerHandle,
                         STLAYER_ViewPortParams_t * const   Params_p,
                         STLAYER_ViewPortHandle_t *         ViewportHandle_p);
    ST_ErrorCode_t (*SetFMDParams)(
                         const STLAYER_ViewPortHandle_t     ViewportHandle,
                         STLAYER_FMDParams_t * const   Params_p);
    void (*SetLayerParams)(
                         const STLAYER_Handle_t             LayerHandle,
                         STLAYER_LayerParams_t * const      LayerParams_p);
    ST_ErrorCode_t (*SetRequestedDeinterlacingMode) (
                         const STLAYER_ViewPortHandle_t     ViewportHandle,
                         const STLAYER_DeiMode_t            RequestedMode);
    void (*SetViewportAlpha)(
                         const STLAYER_ViewPortHandle_t     ViewportHandle,
                         STLAYER_GlobalAlpha_t*             Alpha);
    ST_ErrorCode_t (*SetViewportParams)(
                         const STLAYER_ViewPortHandle_t     ViewportHandle,
                         STLAYER_ViewPortParams_t * const   Params_p);
    ST_ErrorCode_t (*SetViewportPSI)(
                         const STLAYER_Handle_t             LayerHandle,
                         const STLAYER_ViewPortHandle_t     ViewportHandle,
                         const STLAYER_PSI_t * const        ViewportPSI_p);
    void (*SetViewportPosition)(
                         const STLAYER_ViewPortHandle_t     ViewportHandle,
                         const U32                          XPos,
                         const U32                          YPos);
    void (*SetViewportPriority)(
                         const STLAYER_ViewPortHandle_t     ViewportHandle,
                         const HALLAYER_ViewportPriority_t  Priority);
    ST_ErrorCode_t (*SetViewportSource)(
                         const STLAYER_ViewPortHandle_t     ViewportHandle,
                         const STLAYER_ViewPortSource_t *   Source_p);
    ST_ErrorCode_t (*SetViewportSpecialMode)(
                         const STLAYER_ViewPortHandle_t     ViewportPHandle,
                         const STLAYER_OutputMode_t         OuputMode,
                         const STLAYER_OutputWindowSpecialModeParams_t * const Params_p);
    void (*SynchronizedUpdate)(
                         const STLAYER_Handle_t             LayerHandle);
    void (*UpdateFromMixer)(
                         const STLAYER_Handle_t             LayerHandle,
                         STLAYER_OutputParams_t * const     OutputParams_p);
    ST_ErrorCode_t (*Term)(
                         const STLAYER_Handle_t             LayerHandle,
                         const STLAYER_TermParams_t *       TermParams_p);
} HALLAYER_FunctionsTable_t ;


typedef struct
{
    ST_DeviceName_t                     Name;
    U32                                 FrameRate; /* Frame rate of VTG in mHz */
    STVTG_VSYNC_t                       VSyncType; /* Latest VSync Type. */
    BOOL                                IsConnected;
} HALLAYER_VTGSettings_t ;

typedef struct
{
    const HALLAYER_FunctionsTable_t *   FunctionsTable_p;
    ST_Partition_t *                    CPUPartition_p;          /* Where the module can allocate memory for its internal usage */
    STLAYER_Layer_t                     LayerType;
    void *                              VideoDisplayBaseAddress_p;  /* Base address of the DIS_ registers */
    void *                              GammaBaseAddress_p;      /* Base Address of the VIn_ registers */
#ifdef ST_OSLINUX
    void *                              PhysicalDisplayBaseAddress_p;       /* Phys. base Address of the DIS_ registers (used for unmap) */

    void *                              MappedVideoDisplayBaseAddress_p;    /* Mapped base address of the DIS_ registers */
    U32                                 MappedVideoDisplayWidth;            /* Mapped width */

    void *                              MappedGammaBaseAddress_p;           /* Mapped base Address of the VIn_ registers */
    U32                                 MappedGammaWidth;                   /* Mapped width */

#endif
    U32                                 ValidityCheck;
    void *                              PrivateData_p;
    STEVT_Handle_t                      EventsHandle;
    STEVT_EventID_t                     RegisteredEventsID[HALLAYER_NB_REGISTERED_EVENTS_IDS];

    /* task and vtg sync mechanism */
    semaphore_t *                       HALOrderSemaphore_p;
    HALLAYER_VTGSettings_t              VTGSettings;
    BOOL                                TaskTerminate;

#if !defined(ST_OSLINUX)
    char                                Stack[HALLAYER_VIDEO_TASK_STACK_SIZE];
#endif
    task_t*                             Task_p;
    void*                               TaskStack;
    tdesc_t                             TaskDesc;


    /* Display params for video */
    struct
    {
        U8                          FrameBufferDisplayLatency; /* Number of VSyncs between video commit and framebuffer display */
        U8                          FrameBufferHoldTime;     /* Number of VSyncs that the display needs the framebuffer
                                                              * is kept unchanged for nodes generation and composition */
    } DisplayParamsForVideo;

#ifdef ST_XVP_ENABLE_FLEXVP
    ST_Partition_t *                    NCachePartition_p;
#endif /* ST_XVP_ENABLE_FLEXVP */
#ifdef ST_XVP_ENABLE_FGT
    BOOL                                IsFGTBypassed;
#endif /* ST_XVP_ENABLE_FGT */

} HALLAYER_LayerProperties_t ;


/* Exported Variables ------------------------------------------------------- */

/* Exported Macros ---------------------------------------------------------- */
#define LAYERVID_AdjustViewportParams(LayerHandle,Params_p)                                  ((HALLAYER_LayerProperties_t *)(LayerHandle))->FunctionsTable_p->AdjustViewportParams(LayerHandle,Params_p)
#define LAYERVID_CloseViewport(LayerHandle,ViewportHandle)                                   ((HALLAYER_LayerProperties_t *)(LayerHandle))->FunctionsTable_p->CloseViewport(ViewportHandle)
#define LAYERVID_DisableFMDReporting(LayerHandle,ViewportHandle)                             ((HALLAYER_LayerProperties_t *)(LayerHandle))->FunctionsTable_p->DisableFMDReporting(ViewportHandle)
#define LAYERVID_DisableVideoViewport(LayerHandle,ViewportHandle)                            ((HALLAYER_LayerProperties_t *)(LayerHandle))->FunctionsTable_p->DisableVideoViewport(ViewportHandle)
#define LAYERVID_EnableFMDReporting(LayerHandle,ViewportHandle)                              ((HALLAYER_LayerProperties_t *)(LayerHandle))->FunctionsTable_p->EnableFMDReporting(ViewportHandle)
#define LAYERVID_EnableVideoViewport(LayerHandle,ViewportHandle)                             ((HALLAYER_LayerProperties_t *)(LayerHandle))->FunctionsTable_p->EnableVideoViewport(ViewportHandle)
#define LAYERVID_GetFMDParams(LayerHandle,ViewportHandle,LayerParams_p)                      ((HALLAYER_LayerProperties_t *)(LayerHandle))->FunctionsTable_p->GetFMDParams(ViewportHandle,LayerParams_p)
#define LAYERVID_GetVideoDisplayParams(LayerHandle,ViewportHandle,LayerParams_p)             ((HALLAYER_LayerProperties_t *)(LayerHandle))->FunctionsTable_p->GetVideoDisplayParams(ViewportHandle,LayerParams_p)
#define LAYERVID_GetLayerParams(LayerHandle,LayerParams_p)                                   ((HALLAYER_LayerProperties_t *)(LayerHandle))->FunctionsTable_p->GetLayerParams(LayerHandle,LayerParams_p)
#define LAYERVID_GetViewportAlpha(LayerHandle,ViewportHandle,Alpha)                          ((HALLAYER_LayerProperties_t *)(LayerHandle))->FunctionsTable_p->GetViewportAlpha(ViewportHandle,Alpha)
#define LAYERVID_GetViewportIORectangle(LayerHandle,ViewportHandle,InputRectangle_p, OutputRectangle_p) ((HALLAYER_LayerProperties_t *)(LayerHandle))->FunctionsTable_p->GetViewportIORectangle(ViewportHandle,InputRectangle_p, OutputRectangle_p)
#define LAYERVID_GetViewportParams(LayerHandle,ViewportHandle,Params_p)                      ((HALLAYER_LayerProperties_t *)(LayerHandle))->FunctionsTable_p->GetViewportParams(ViewportHandle,Params_p)
#define LAYERVID_GetViewPortPSI(LayerHandle,ViewportHandle,ViewportPSI_p)                    ((HALLAYER_LayerProperties_t *)(LayerHandle))->FunctionsTable_p->GetViewportPSI(LayerHandle,ViewportHandle,ViewportPSI_p)
#define LAYERVID_GetViewportPosition(LayerHandle,ViewportHandle,XPos_p,YPos_p)               ((HALLAYER_LayerProperties_t *)(LayerHandle))->FunctionsTable_p->GetViewportPosition(ViewportHandle,XPos_p,YPos_p)
#define LAYERVID_GetViewportSource(LayerHandle,ViewportHandle,SourceNumber_p)                ((HALLAYER_LayerProperties_t *)(LayerHandle))->FunctionsTable_p->GetViewportSource(ViewportHandle,SourceNumber_p)
#define LAYERVID_GetViewPortSpecialMode(LayerHandle,ViewportHandle,Mode_p,Params_p)          ((HALLAYER_LayerProperties_t *)(LayerHandle))->FunctionsTable_p->GetViewportSpecialMode(ViewportHandle,Mode_p,Params_p)
#define LAYERVID_OpenViewport(LayerHandle,Params_p,ViewportHandle_p)                         ((HALLAYER_LayerProperties_t *)(LayerHandle))->FunctionsTable_p->OpenViewport(LayerHandle,Params_p,ViewportHandle_p)
#define LAYERVID_GetRequestedDeinterlacingMode(LayerHandle,ViewportHandle,RequestedMode_p)   ((HALLAYER_LayerProperties_t *)(LayerHandle))->FunctionsTable_p->GetRequestedDeinterlacingMode(ViewportHandle,RequestedMode_p)
#define LAYERVID_SetRequestedDeinterlacingMode(LayerHandle,ViewportHandle,RequestedMode)     ((HALLAYER_LayerProperties_t *)(LayerHandle))->FunctionsTable_p->SetRequestedDeinterlacingMode(ViewportHandle,RequestedMode)
#define LAYERVID_SetFMDParams(LayerHandle,ViewportHandle,LayerParams_p)                      ((HALLAYER_LayerProperties_t *)(LayerHandle))->FunctionsTable_p->SetFMDParams(ViewportHandle,LayerParams_p)
#define LAYERVID_SetLayerParams(LayerHandle,LayerParams_p)                                   ((HALLAYER_LayerProperties_t *)(LayerHandle))->FunctionsTable_p->SetLayerParams(LayerHandle,LayerParams_p)
#define LAYERVID_SetViewportAlpha(LayerHandle,ViewportHandle,Alpha)                          ((HALLAYER_LayerProperties_t *)(LayerHandle))->FunctionsTable_p->SetViewportAlpha(ViewportHandle,Alpha)
#define LAYERVID_SetViewportParams(LayerHandle,ViewportHandle,Params_p)                      ((HALLAYER_LayerProperties_t *)(LayerHandle))->FunctionsTable_p->SetViewportParams(ViewportHandle,Params_p)
#define LAYERVID_SetViewPortPSI(LayerHandle,ViewportHandle,ViewportPSI_p)                    ((HALLAYER_LayerProperties_t *)(LayerHandle))->FunctionsTable_p->SetViewportPSI(LayerHandle,ViewportHandle,ViewportPSI_p)
#define LAYERVID_SetViewportPosition(LayerHandle,ViewportHandle,XPos,YPos)                   ((HALLAYER_LayerProperties_t *)(LayerHandle))->FunctionsTable_p->SetViewportPosition(ViewportHandle,XPos,YPos)
#define LAYERVID_SetViewportPriority(LayerHandle,ViewportHandle,Priority)                    ((HALLAYER_LayerProperties_t *)(LayerHandle))->FunctionsTable_p->SetViewportPriority(ViewportHandle,Priority)
#define LAYERVID_SetViewportSource(LayerHandle,ViewportHandle,Source_p)                      ((HALLAYER_LayerProperties_t *)(LayerHandle))->FunctionsTable_p->SetViewportSource(ViewportHandle,Source_p)
#define LAYERVID_SetViewPortSpecialMode(LayerHandle,ViewportHandle,Mode,Params_p)            ((HALLAYER_LayerProperties_t *)(LayerHandle))->FunctionsTable_p->SetViewportSpecialMode(ViewportHandle,Mode,Params_p)


/* Exported Functions ------------------------------------------------------- */
void LAYERVID_GetVTGName(const STLAYER_Handle_t LayerHandle, ST_DeviceName_t * const VTGName_p);
void LAYERVID_GetVTGParams(const STLAYER_Handle_t LayerHandle, STLAYER_VTGParams_t * const VTGParams_p);
ST_ErrorCode_t LAYERVID_Init(const ST_DeviceName_t DeviceName, const STLAYER_InitParams_t * const LayerInitParams_p, STLAYER_Handle_t * const LayerHandle_p);
#ifdef ST_OSLINUX
    ST_ErrorCode_t LAYERVID_Term(const ST_DeviceName_t DeviceName, const STLAYER_TermParams_t * TermParams_p);
#else
    ST_ErrorCode_t LAYERVID_Term(const STLAYER_Handle_t LayerHandle, const STLAYER_TermParams_t * TermParams_p);
#endif
void LAYERVID_UpdateFromMixer(const STLAYER_Handle_t LayerHandle, STLAYER_OutputParams_t * const OutputParams_p);
ST_ErrorCode_t LAYERVID_GetCapability(const STLAYER_Handle_t LayerHandle, STLAYER_Capability_t * Capability_p);
U32 stlayer_GetDecimationFactorFromDecimation(STLAYER_DecimationFactor_t Decimation);
STLAYER_DecimationFactor_t stlayer_GetDecimationFromDecimationFactor (U32 DecimationFactor);

#ifdef ST_XVP_ENABLE_FGT
ST_ErrorCode_t LAYERVID_SetFGTState(const STLAYER_Handle_t LayerHandle, BOOL State);
#endif /* ST_XVP_ENABLE_FGT */

 /* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __VIDEO_HAL_TYPE_H */

/* End of halv_lay.h */
