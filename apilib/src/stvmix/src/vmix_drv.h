/*******************************************************************************

File name   : vmix_drv.h

Description : video mixer driver module header file

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
21 July 2000        Created                                          BS
26 Oct  2000        Release A2                                       BS
08 Nov  2000        Re-organizing functions & add GFX support        BS
******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __VMIX_DRV_H
#define __VMIX_DRV_H


/* Includes ----------------------------------------------------------------- */
#if !defined(ST_OSLINUX)
#include <stdlib.h>
#include <string.h>
#endif
#include "stddefs.h"

#include "sttbx.h"

#include "stvmix.h"
#include "stlayer.h"
#include "stvout.h"
#include "stcommon.h"
#include "stos.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */
#define STVMIX_COMMON_HORIZONTAL_OFFSET   10
#define STVMIX_COMMON_VERTICAL_OFFSET     10

/* The slowest Vsync is a progressive 24 hertz vtg */
#define STVMIX_MAX_VSYNC_DURATION ((U32)(ST_GetClocksPerSecond()/10))

/* Exported Types ----------------------------------------------------------- */
typedef enum
{
    API_ENABLE          = 0x01,
    API_STVTG_CONNECT   = 0x02,
    API_CONNECTED_LAYER = 0x04,
    API_SCREEN_PARAMS   = 0x08,
    API_DISPLAY_ALL_SET = 0x0D
} stvmix_ApiStatus_t;

typedef enum
{
    LAYER_CHECK,
    LAYER_SET_REGISTER
} stvmix_HalLayerConnect;

typedef enum
{
    VSYNC_SUBSCRIBE,
    VSYNC_UNSUBSCRIBE,
    CHANGE_VTG
} stvmix_HalActionEvt_e;

typedef struct
{
    ST_Partition_t*         CPUPartition_p;
    void*                   BaseAddress_p;
    void*                   DeviceBaseAddress_p;
    STVMIX_DeviceType_t     DeviceType;
    U16                     MaxLayer;
    ST_DeviceName_t         EvtHandlerName;
} stvmix_InitParams_t;

typedef struct
{
    STLAYER_Handle_t            Handle;      /* Handle of connected layer */
    STLAYER_Layer_t             Type;        /* Type of connected layer */
    STVMIX_LayerDisplayParams_t Params;      /* Params connected layer (Name & futur info)*/
    BOOL                        HdlUsed;     /* If handle is used by temporary structure*/
    U32                         DeviceId;    /* Device number of layer, usefull when types are identicals */
} stvmix_LayerConnect_t;

typedef struct
{
    STLAYER_Handle_t            Handle;      /* Handle of temporary layer */
    STLAYER_Layer_t             Type;        /* Type of temporary layer */
    BOOL                        HdlUsed;     /* If handle is used by temporary structure*/
    U32                         DeviceId;    /* Device number of layer, usefull when types are identicals */
    BOOL                        ChangedId;   /* Id changed when switching planes */
} stvmix_LayerTmp_t;

typedef struct
{
    U32                     NbConnect;       /* Number of connected layers */
    stvmix_LayerConnect_t*  ConnectArray_p;  /* Dynamically allocated in API init function */
    U32                     NbTmp;           /* Number of temporary connected layers */
    stvmix_LayerTmp_t*      TmpArray_p;      /* Dynamically allocated in API init function */
} stvmix_Layer_t;

typedef struct
{
    ST_DeviceName_t         Name;
    STVOUT_OutputType_t     Type;
} stvmix_Outputs_Params_t;

typedef struct
{
    stvmix_Outputs_Params_t*  ConnectArray_p; /* Array of ouputs parameters */
    U32                       NbConnect;      /* Number of connected outputs */
} stvmix_Outputs_t;

typedef struct
{
    STGXOBJ_ColorRGB_t        Color;       /* Background color */
    BOOL                      Enable;      /* Color enabled */
} stvmix_Background_t;

typedef struct
{
    /* common informations for all open on a particular device */
    ST_DeviceName_t               DeviceName;            /* Device Name */

#ifdef ST_OSLINUX
    U32                           VmixMappedWidth;
    void *                        VmixMappedBaseAddress_p; /* Address where will be mapped the driver registers */
    U32                           GammaMappedWidth;
    void *                        GammaMappedBaseAddress_p; /* Address where will be mapped the driver registers */
#endif

    U32                           MaxOpen;               /* Max open permitted for this device */
    stvmix_InitParams_t           InitParams;            /* Init parameters */
    semaphore_t *                 CtrlAccess_p;            /* Control access to internal data */
    stvmix_ApiStatus_t            Status;                /* Internal status of the API */
    stvmix_Layer_t                Layers;                /* Layers information */
    stvmix_Outputs_t              Outputs;               /* Outputs information */
    stvmix_Background_t           Background;
    ST_DeviceName_t               VTG;                   /* VTG connected to the mixer*/
    STVMIX_ScreenParams_t         ScreenParams;          /* Screen parameters */
    S8                            XOffset;               /* X offset */
    S8                            YOffset;               /* Y offset */
    STVMIX_Capability_t           Capability;            /* Capability infos */
    void*                         Hal_p;                 /* Hal specific infos */
} stvmix_Device_t;

typedef struct
{
    stvmix_Device_t*    Device_p;
    STVMIX_Handle_t     Handle;
    U32                 UnitValidity;   /* Only the value STVMIX_VALID_UNIT means the unit is valid */
} stvmix_Unit_t;

/* Exported Variables ------------------------------------------------------- */

/* Exported Macros ---------------------------------------------------------- */

/* Exported Functions ------------------------------------------------------- */
ST_ErrorCode_t stvmix_ConnectLayer(stvmix_Device_t* const Device_p);
void stvmix_Disable(stvmix_Device_t* const Device_p);
void stvmix_DisconnectLayer(stvmix_Device_t* const Device_p);
ST_ErrorCode_t stvmix_Enable(stvmix_Device_t* const Device_p);
ST_ErrorCode_t stvmix_GetBackGroundColor(stvmix_Device_t* const Device_p,
                                         STGXOBJ_ColorRGB_t* const RGB888_p,
                                         BOOL* const Enable_p);
ST_ErrorCode_t stvmix_Init(stvmix_Device_t* const Device_p,
                           const STVMIX_InitParams_t* const InitParams_p);
ST_ErrorCode_t stvmix_SetBackGroundColor(stvmix_Device_t* const Device_p,
                                         STGXOBJ_ColorRGB_t const RGB888,
                                         BOOL const Enable);
ST_ErrorCode_t stvmix_SetScreen(stvmix_Device_t* const Device_p);
ST_ErrorCode_t stvmix_SetTimeBase(stvmix_Device_t * const Device_p,
                                  ST_DeviceName_t OldVTG);
void stvmix_Term(stvmix_Device_t* const Device_p);
/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __VMIX_DRV_H */

/* End of vmix_drv.h */







