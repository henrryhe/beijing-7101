/*******************************************************************************

File name   : frontend.h

Description : definitions used in layer context.

COPYRIGHT (C) STMicroelectronics 2004

Date               Modification                               Name
----               ------------                               ----
2000-05-30         Created                                    Michel Bruant
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __FRONT_END_H
#define __FRONT_END_H

/* Includes ----------------------------------------------------------------- */
#include "layergfx.h"
#include "back_end.h"
#include "gfxswcfg.h"
#include "stevt.h"

#include "stos.h"


/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Private Constants -------------------------------------------------------- */

/* Hardware limitation */
#define MAX_GDP_BKL   5
#define MAX_CURS      1
#define MAX_OTHER     0
#define MAX_LAYER     (MAX_GDP_BKL+MAX_CURS+MAX_OTHER)

/* Exported Types ----------------------------------------------------------- */

/* type for describe a layer */
typedef struct
{
    STLAYER_Layer_t                 LayerType;
    U32                             Open;
    U32                             DeviceId;
    U32                             MaxViewPorts;
    BOOL                            Initialised;
    BOOL                            VPDescriptorsUserAllocated;
    BOOL                            VPNodesUserAllocated;
    stlayer_DataPoolDesc_t          DataPoolDesc;
    stlayer_ViewPortDescriptor_t ** HandleArray;
    stlayer_ViewPortDescriptor_t *  ListViewPort;
    stlayer_Node_t *                NodeArray;
    stlayer_Node_t *                TmpTopNode_p;
    stlayer_Node_t *                TmpBotNode_p;
    STAVMEM_BlockHandle_t           BlockNodeArray;
    stlayer_ViewPortDescriptor_t *  LinkedViewPorts;
    U32                             NumberLinkedViewPorts;
    S8*                             FilterCoefs;
    S8*                             VFilterCoefs; /* Coeficient of Vertical filters params  */
    S8*                             FFilterCoefs; /* Coeficient of Flicker Filters params */
    ST_DeviceName_t                 DeviceName;
    STLAYER_LayerParams_t           LayerParams;
    STLAYER_OutputParams_t          OutputParams;
    ST_Partition_t *                CPUPartition_p;
    void *                          BaseAddress;
    STAVMEM_SharedMemoryVirtualMapping_t VirtualMapping;
    STAVMEM_PartitionHandle_t       AVMEM_Partition;
    ST_DeviceName_t                 EvtHandlerName;
    ST_DeviceName_t                 VTGName;
    STEVT_Handle_t                  EvtHandle;
    STEVT_EventID_t                 UpdateParamEvtIdent;
    semaphore_t *                   AccessProtect_p;
    BOOL                            TaskTerminate;
#ifdef ST_OSLINUX
    void *                          MappedGammaBaseAddress_p;		/* Mapped base Address of the VIn_ registers */
    U32                             MappedGammaWidth;		        /* Mapped width */
#endif  /* ST_OSLINUX */
    task_t*                         Task_p;
    void*                           TaskStack;
    tdesc_t                         TaskDesc;

    message_queue_t *               MsgQueueTop_p;
    message_queue_t *               MsgQueueBot_p;
    void*                           MsgQueueBufferTop_p;
    void*                           MsgQueueBufferBot_p;

    semaphore_t *                   VSyncSemaphore_p;
    BOOL                            VTG_TopNotBottom;
    BOOL                            VTG_TopNotBottomSimulation;
} stlayer_XLayerContext_t;

/* Global Variables --------------------------------------------------------- */

#ifdef VARIABLE_STLAYER_RESERVATION
#define VARIABLE_STLAYER           /* real reservation   */
#else
#define VARIABLE_STLAYER extern    /* extern reservation */
#endif

VARIABLE_STLAYER struct
{
    /* General Context */
    BOOL                          FirstInitDone;                /* flag */
    U32                           NumLayers;       /* for hw limitation */
    U32                           NumBKLGDPLayers; /* for hw limitation */
    U32                           NumCursLayers;   /* for hw limitation */
    U32                           NumOtherLayers;  /* for other layers  */
    /* each Layer Context */
    stlayer_XLayerContext_t       XContext[MAX_LAYER]; /* index eq handle */

}stlayer_context;

#undef VARIABLE_STLAYER

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __FRONT_END_H */

/* End of frontend.h */
