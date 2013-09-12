/*******************************************************************************

File name   : Sub_init.h

Description : Sub init header file (device relative definitions)

COPYRIGHT (C) STMicroelectronics 2004

Date               Modification                                     Name
----               ------------                                     ----
15 Dec 2000        Created                                           TM
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __SUB_INIT_H
#define __SUB_INIT_H


/* Includes ----------------------------------------------------------------- */

#include <stdlib.h>
#include "stddefs.h"
#include "stevt.h"
#include "stavmem.h"
#include "stlayer.h"
#include "sub_com.h"
#include "subswcfg.h"


/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */

#define LAYERSUB_VALID_UNIT   0x091E91E0   /* 0xXYnZYnZX with the driver ID XYZ in hexa
                                            * STLAYER Id = 158 => 0x09E
                                            * n = 1 open handle sub picture TBD!!!!!!*/

/* Exported Types ----------------------------------------------------------- */

typedef enum
{
  LAYERSUB_DISPLAY_STATUS_ENABLE,
  LAYERSUB_DISPLAY_STATUS_DISABLE
} layersub_DisplayStatus_t;

typedef struct
{
  U32                   NbFreeElem;           /* Number of Free elements in Data Pool */
  U32                   NbElem;               /* Number of elements */
  U32                   ElemSize;             /* Size of an element in Byte */
  void*                 ElemArray_p;          /* Array of elements */
  void**                HandleArray_p;        /* Array of handle (i.e array of pointers to elements)*/
} layersub_DataPoolDesc_t;

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

    /* Event related */
    ST_DeviceName_t                 EventHandlerName;
    STEVT_Handle_t                  EvtHandle;

    /* ViewPort related*/
    layersub_DataPoolDesc_t         ViewPortDataPool;       /* Data Pool for ViewPort descriptors*/
    BOOL                            ViewPortBufferUserAllocated;  /* TRUE if ViewPort buffer allocated by the user. Used for deallocation */
    STLAYER_ViewPortHandle_t        ActiveViewPort;   /* Handle of the enabled viewport(only one). LAYERSUB_NO_VIEWPORT_HANDLE if none*/

    /* Mixer-display related */
    layersub_DisplayStatus_t        DisplayStatus;   /* Display Status */
    BOOL                            MixerConnected;  /* Mixel connected to layer */
    U32                             MixerWidth;
    U32                             MixerHeight;

    /* HW layer params */
    STGXOBJ_ScanType_t              ScanType;
    STGXOBJ_AspectRatio_t           AspectRatio;
    U32                             LayerWidth;     /* Width of device layer */
    U32                             LayerHeight;    /* Height of device layer */
    U32                             XStart;    /* X Offset relative to norm (PAL, NTSC...) */
    U32                             YStart;    /* Y Offset relative to norm */
    U32                             FrameRate;
    S8                              XOffset;   /* X Offset correction (relative to TV set) */
    S8                              YOffset;   /* Y Offset correction */

    /* Tasks related */
    BOOL                        TaskTerminate;                       /* TRUE if task must be terminated*/
    task_t *                    ProcessEvtVSyncTask_p;                 /* Task used for Vsync process*/
    int                         StackProcessEvtVSyncTask[PROCESS_EVT_VSYNC_TASK_STACK_SIZE/ sizeof(int)];  /* Task stack */

    /* Swap SPU related */
    BOOL                        SwapActiveSPU;                      /* TRUE if a new SPU has to be swapped with the active one */

    /* semaphore related */
    semaphore_t *               ProcessEvtVSyncSemaphore_p;            /* Semaphore used for Vsync process*/

    /* VTG related */
    BOOL                        IsEvtVsyncSubscribed;                 /* TRUE if a VTG name has been given to the driver by
                                                                      * the UpdateFromMixer function in order to subscribe
                                                                      * to Vsync event*/
    ST_DeviceName_t              VTGName;                             /* VTG Name used for  VSYNC event subscription */


} layersub_Device_t;


/* Structure describing an open connection on one HW layer */
typedef struct
{
    layersub_Device_t*          Device_p;          /* Device structure (relative to init) */
    STLAYER_ViewPortHandle_t    FirstOpenViewPort; /* Handle of the first open viewport in list. LAYERSUB_NO_VIEWPORT_HANDLE if none*/
    STLAYER_ViewPortHandle_t    LastOpenViewPort;  /* Handle of the last open viewport in list. LAYERSUB_NO_VIEWPORT_HANDLE if none*/
    U32                         NbOpenViewPorts;   /* Number of open viewports in list */
    U32                         UnitValidity;      /* Only the value LAYERSUB_VALID_UNIT means the unit is valid */
} layersub_Unit_t;


/* Exported Variables ------------------------------------------------------- */


/* Exported Macros ---------------------------------------------------------- */


/* Exported Functions ------------------------------------------------------- */


/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __SUB_INIT_H */

/* End of Sub_init.h */
