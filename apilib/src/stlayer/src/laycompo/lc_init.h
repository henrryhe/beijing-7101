/*******************************************************************************

File name   : lc_init.h

Description : layer compo init header file (device relative definitions)

COPYRIGHT (C) STMicroelectronics 2003.

Date               Modification                                     Name
----               ------------                                     ----
Sept 2003        Created                                           TM
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __LC_INIT_H
#define __LC_INIT_H


/* Includes ----------------------------------------------------------------- */

#include <stdlib.h>
#include "stddefs.h"
#include "stevt.h"
#include "stavmem.h"
#include "stlayer.h"
#include "lcswcfg.h"
#include "stdisp.h"


/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */

#define LAYCOMPO_NO_DISPLAY_HANDLE ((STDISP_Handle_t) NULL)
#define LAYCOMPO_NO_LAYER_HANDLE ((STLAYER_Handle_t) NULL)

#define LAYCOMPO_VALID_UNIT   0x092E92E0   /* 0xXYnZYnZX with the driver ID XYZ in hexa
                                            * STLAYER Id = 158 => 0x09E
                                            * n = 2 */
#define LAYCOMPO_UPDATE_PARAMS_EVT_ID      0
#define LAYCOMPO_NB_REGISTERED_EVENTS      1

/* Exported Types ----------------------------------------------------------- */

typedef struct
{
  U32                   NbFreeElem;           /* Number of Free elements in Data Pool */
  U32                   NbElem;               /* Number of elements */
  U32                   ElemSize;             /* Size of an element in Byte */
  void*                 ElemArray_p;          /* Array of elements */
  void**                HandleArray_p;        /* Array of handle (i.e array of pointers to elements)*/
} laycompo_DataPoolDesc_t;

/* Structure describing one HW layer (=Device)*/
typedef struct
{
    /* common stuff */
    ST_DeviceName_t                         DeviceName;
    ST_Partition_t*                         CPUPartition_p;
    void*                                   BaseAddress_p;
    STAVMEM_PartitionHandle_t               AVMEMPartition;
    void*                                   SharedMemoryBaseAddress_p;
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;
    U32                                     MaxHandles;

    /* Evt related */
    ST_DeviceName_t                         EventHandlerName;
    STEVT_Handle_t                          EvtHandle;
    STEVT_EventID_t                         EventID[LAYCOMPO_NB_REGISTERED_EVENTS];

    /* ViewPort related*/
    laycompo_DataPoolDesc_t         ViewPortDataPool;       		/* Data Pool for ViewPort descriptors*/
    BOOL                            ViewPortBufferUserAllocated;  	/* TRUE if ViewPort buffer allocated by the user. Used for deallocation */

    /* Mixer-display related */
    BOOL                            MixerConnected;  /* Mixel connected to layer */
    U32                             MixerWidth;
    U32                             MixerHeight;
    STLAYER_Handle_t                BackLayer;
    STLAYER_Handle_t                FrontLayer;

	/* Viewport related */
	STLAYER_ViewPortHandle_t    	FirstOpenViewPort; /* Handle of the first open viewport in list. LAYCOMPO_NO_VIEWPORT_HANDLE if none*/
    STLAYER_ViewPortHandle_t    	LastOpenViewPort;  /* Handle of the last open viewport in list. LAYCOMPO_NO_VIEWPORT_HANDLE if none*/
    U32                         	NbOpenViewPorts;   /* Number of open viewports in list */

    /* Layer params */
    STGXOBJ_AspectRatio_t           AspectRatio;
    STGXOBJ_ScanType_t              ScanType;
    U32                             LayerWidth;     /* Width of device layer */
    U32                             LayerHeight;    /* Height of device layer */

    /* Compositor related */
    STDISP_Handle_t                 DisplayHandle;    /* First implmentation : Only one compo, if any, per layer */

    /* VTG related */
    BOOL                            IsEvtVsyncSubscribed;                 /* TRUE if a VTG name has been given to the driver by
                                                                           * the UpdateFromMixer function in order to subscribe
                                                                           * to Vsync event*/
    /* VTG related */
    STLAYER_VTGParams_t             VTGParams;

    /* Protection related */
    semaphore_t *                   CtrlAccess_p;            /* Control access to internal data */

    /* Display params for video */
    struct
    {
        U8                          FrameBufferDisplayLatency; /* Number of VSyncs between video commit and framebuffer display */
        U8                          FrameBufferHoldTime;     /* Number of VSyncs that the display needs the framebuffer
                                                              * is kept unchanged for nodes generation and composition */
    } DisplayParamsForVideo;

} laycompo_Device_t;


/* Structure describing an open connection on one HW layer */
typedef struct
{
    laycompo_Device_t*          Device_p;          /* Device structure (relative to init) */
    U32                         UnitValidity;      /* Only the value LAYCOMPO_VALID_UNIT means the unit is valid */
} laycompo_Unit_t;


/* Exported Variables ------------------------------------------------------- */


/* Exported Macros ---------------------------------------------------------- */


/* Exported Functions ------------------------------------------------------- */
void laycompo_ReceiveEvtVSync (STEVT_CallReason_t     Reason,
                               const ST_DeviceName_t  RegistrantName,
                               STEVT_EventConstant_t  Event,
                               const void*            EventData_p,
                               const void*            SubscriberData_p);

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __LC_INIT_H */

/* End of lc_init.h */
