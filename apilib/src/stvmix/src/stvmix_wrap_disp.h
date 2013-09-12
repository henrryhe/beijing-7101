/*******************************************************************************

File name   : stvmix_wrap_disp.h

Description : video mixer driver module header file

COPYRIGHT (C) STMicroelectronics 2003.

Date               Modification                                     Name
----               ------------                                     ----
241103                                                               TM
******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __STVMIX_WRAP_DISP_H
#define __STVMIX_WRAP_DISP_H


/* Includes ----------------------------------------------------------------- */

#include <stdlib.h>
#include <string.h>
#include "stddefs.h"
#include "stvmix.h"
#include "stgxobj.h"
#include "stdisp.h"
#include "stlayer.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */

/*  Supported planes
 * -----------------
 *  FRAME : BackGround + STILL + VIDEO + PIP + OSD
 *  FIELD : BackGround + STILL + VIDEO + OSD
 *  SDRAM : BackGround + VIDEO + OSD
 *
 * General assumptions for viewports use
 * -------------------------------------
 *	The total amount of accumulated pixels of different viewports in source must not exceed the maximum dimension defined for this plane.
 *  The total amount of accumulated pixels of different viewports in destination must not exceed the maximum dimension defined for this plane.
 *	Memory overlap feature not supported.
 *	Existing viewports in a given plane must not be partly covered.
 */



#ifdef MAXVIEWPORTS
    #define STVMIX_FRAME_MODE_MAX_VIEWPORTS      MAXVIEWPORTS
#else
    #define STVMIX_FRAME_MODE_MAX_VIEWPORTS      32
#endif
#define STVMIX_FIELD_MODE_MAX_VIEWPORTS          32
#define STVMIX_FIELD_SDRAM_MODE_MAX_VIEWPORTS    32

#define STVMIX_FRAME_MODE_NODE_NUMBER        1100
#define STVMIX_FIELD_MODE_NODE_NUMBER        800
#define STVMIX_FIELD_SDRAM_MODE_NODE_NUMBER  400

#define STVMIX_GDMA_NODE_NUMBER   8
#define STVMIX_VALID_UNIT         0x0A00A000   /* 0xXYnZYnZX with the driver ID XYZ in hexa
                                                * STVMIX Id = 160 => 0x0A0
                                                 * n = 0 */
 /*Offset are arbitrary values!!!! */
 #define STVMIX_COMPO_MIN_HORIZONTAL_OFFSET (-40)
 #define STVMIX_COMPO_MAX_HORIZONTAL_OFFSET 10
 #define STVMIX_COMPO_VERTICAL_OFFSET   10

/* Exported Types ----------------------------------------------------------- */

typedef struct
{
    STLAYER_Handle_t      Handle;       /* Handle of connected layer */
    ST_DeviceName_t       DeviceName;   /* Layer Name */
} stvmix_LayerConnect_t;

typedef struct
{
    STLAYER_Handle_t      Handle;       /* Handle of layer to be connected */
    BOOL                  IsTmp;        /* TRUE  : Layer not yet connected
                                           FALSE : Layer already connected */
} stvmix_LayerTmp_t;


typedef struct
{
    U32                     NbConnect;       /* Number of connected layers */
    stvmix_LayerConnect_t*  ConnectArray_p;  /* Dynamically allocated in API init function */
} stvmix_Layer_t;


typedef struct
{
    /* Common */
    ST_Partition_t*                         CPUPartition_p;
    ST_DeviceName_t                         DeviceName;
    U32                                     MaxHandles;
    U16                                     MaxLayer;
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;
    STAVMEM_PartitionHandle_t               AVMEM_Partition;
    STVMIX_DeviceType_t                     DeviceType;

    /* Screen related */
    S8                                      XOffset;
    S8                                      YOffset;
    STVMIX_ScreenParams_t                   ScreenParams;

    /* VTG related */
    ST_DeviceName_t                         VTGName;

    /* Protection related */
    semaphore_t*                            CtrlAccess_p;            /* Control access to internal data */

    /* Layer related */
    stvmix_Layer_t                          Layers;                /* Layers information */

    /* Output bitmap related */
    STGXOBJ_Bitmap_t                        OutputBitmap[2];
    STAVMEM_BlockHandle_t                   OutputBitmapAVMEMHandle[2];

    /* Background color related */
    STGXOBJ_ColorUnsignedYCbCr_t            DefaultYCbCr;      /* default is Black */
    STGXOBJ_Color_t                         BackgroundColor;
    BOOL                                    IsBackgroundEnable;

    /* Nb bitmap buffer : 2 for frame mode, 1 for field mode */
    int                                     NbBitmapBuffer;

} stvmix_Device_t;


typedef struct
{
    stvmix_Device_t*            Device_p;          /* Device structure (relative to init) */
    STDISP_Handle_t             Handle;            /* Handle on STDISP */
    U32                         UnitValidity;      /* Only the value STVMIX_VALID_UNIT means the unit is valid */
} stvmix_Unit_t;


/* Exported Variables ------------------------------------------------------- */

/* Exported Macros ---------------------------------------------------------- */

/* Exported Functions ------------------------------------------------------- */

#ifdef __cplusplus
}
#endif

#endif /* #ifndef __STVMIX_WRAP_DISP_H */

/* End of stvmix_wrap_disp.h */






