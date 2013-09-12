/*******************************************************************************

File name   : blt_com.h

Description : blit_common header file

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
13 Jun 2000        Created                                           TM
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __BLT_COM_H
#define __BLT_COM_H


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#include "stblit.h"
#if !defined(ST_OSLINUX) || defined(STBLIT_LINUX_FULL_USER_VERSION)
#include "stsys.h"
#endif

#ifdef STBLIT_LINUX_FULL_USER_VERSION
#include "stlayer.h"
#endif

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */

enum
{
    STBLIT_IT_OPCODE_BLIT_COMPLETION_MASK = 1,
    STBLIT_IT_OPCODE_JOB_COMPLETION_MASK  = 2,
    STBLIT_IT_OPCODE_LOCK_MASK            = 4,
    STBLIT_IT_OPCODE_UNLOCK_MASK          = 8,
    STBLIT_IT_OPCODE_SW_WORKAROUND_MASK   = 16
};

#define STBLIT_VALID_JOB          0x091D91D0   /* 0xXYnZYnZX with the driver ID XYZ in hexa
                                                * STBLIT Id = 157 => 0x09D
                                                * n = 1 for job handle*/

#define STBLIT_VALID_JOB_BLIT      0x092D92D0   /* 0xXYnZYnZX with the driver ID XYZ in hexa
                                                 * STBLIT Id = 157 => 0x09D
                                                 * n = 2 for Job Blit handle*/

#define STBLIT_NO_JOB_BLIT_HANDLE ((STBLIT_JobBlitHandle_t) NULL)

#ifdef STBLIT_LINUX_FULL_USER_VERSION
U32     PhysicJobBlitNodeBuffer_p;
U32     PhysicSingleBlitNodeBuffer_p;


#define PHYSICAL_LMI_SYS_START                      0x7000000  /* Start LMI SYS */
#define KERNEL_LMI_SYS_START                        0xa7000000  /* Start LMI SYS */
#define LMI_SYS_WIDTH                               0x00C00000

#define stblit_DeviceToCpu(Address)  (void*) STLAYER_KernelToUser((U32)(Address) + (U32)(KERNEL_LMI_SYS_START-PHYSICAL_LMI_SYS_START) )

#else

#define stblit_CpuToDevice(Address, SharedMemoryVirtualMapping_p)                                                 \
                          ((void*)((U32)(Address)                                                                 \
                                 + (U32)((SharedMemoryVirtualMapping_p)->PhysicalAddressSeenFromDevice_p)         \
                                 - (U32)((SharedMemoryVirtualMapping_p)->PhysicalAddressSeenFromCPU_p)))

#define stblit_DeviceToCpu(Address, SharedMemoryVirtualMapping_p)                                                 \
                          ((void*)((U32)(Address)                                                                 \
                                 + (U32)((SharedMemoryVirtualMapping_p)->PhysicalAddressSeenFromCPU_p)         \
                                 - (U32)((SharedMemoryVirtualMapping_p)->PhysicalAddressSeenFromDevice_p)))
#endif

/* BLT Control Register */
#define BLT_CTL_OFFSET            0x00
#define BLT_CTL_MASK              0x0000000F
#define BLT_CTL_SHIFT             0
#define BLT_CTL_RESET             1
#define BLT_CTL_START             2
#define BLT_CTL_SUSPEND           4
#define BLT_CTL_SUSPEND_ASAP      8

/* BLT Status Register 1 */
#define BLT_STA1_OFFSET           0x04/*0x104*/
#define BLT_STA1_MASK             0xFFFFFFFF
#define BLT_STA1_SHIFT            0

/* BLT Status Register 2 */
#define BLT_STA2_OFFSET           0x08/*0x108*/
#define BLT_STA2_MASK             0xFFF
#define BLT_STA2_SHIFT            0

/* BLT Status Register 3 */
#define BLT_STA3_OFFSET           0x0C/*0x10C*/
#define BLT_STA3_MASK             0x0000000F
#define BLT_STA3_SHIFT            0
#define BLT_STA3_READY            1
#define BLT_STA3_COMPLETED        2
#define BLT_STA3_READY2START      4
#define BLT_STA3_SUSPENDED        8

/* BLT next instruction pointer */
#define BLT_NIP_OFFSET            0x10/*0x110*/

/* BLT packet size control */
#define BLT_PKZ_OFFSET            0xFC
#define BLT_PKZ_ENDIANNESS_SHIFT  5


/* Exported Types ----------------------------------------------------------- */
typedef enum
{
    STBLIT_NO_MASK          = 0,
    STBLIT_MASK_FIRST_PASS  = 1,
    STBLIT_MASK_SECOND_PASS = 2
} stblit_MaskMode_t;

typedef enum
{
    STBLIT_SOURCE1_MODE_NORMAL,
    STBLIT_SOURCE1_MODE_DIRECT_COPY,
    STBLIT_SOURCE1_MODE_DIRECT_FILL
} stblit_Source1Mode_t;

typedef enum
{
    STBLIT_JOB_BLIT_XYL_TYPE_NONE,     /* No XYL mode */
    STBLIT_JOB_BLIT_XYL_TYPE_DEVICE,   /* Single node XYL performed by HW device */
    STBLIT_JOB_BLIT_XYL_TYPE_EMULATED  /* Emulated XYL , multi nodes */
} stblit_JBlitXYLType_t;

typedef enum
{
   STBLIT_INFO_TYPE_NONE,
   STBLIT_INFO_TYPE_COLOR,
   STBLIT_INFO_TYPE_BITMAP
} stblit_InfoType_t;


typedef struct
{
    U32    BLT_NIP;                               /* Next Instruction Pointer */
    U32    BLT_USR;                               /* User specific  */
    U32    BLT_INS;                               /* Instruction  */
    U32    BLT_S1BA;                              /* Source1 Base Address */

    U32    BLT_S1TY;                              /* Source1 Type */
    U32    BLT_S1XY;                              /* Source1 XY Location */
    U32    BLT_S1SZ;                              /* Source1 Size */
    U32    BLT_S1CF;                              /* Source1 Color Fill */

    U32    BLT_S2BA;                              /* Source2 Base Address */
    U32    BLT_S2TY;                              /* Source2 Type */
    U32    BLT_S2XY;                              /* Source2 XY Location */
    U32    BLT_S2SZ;                              /* Source2 Size */

    U32    BLT_S2CF;                              /* Source2 Color Fill */
    U32    BLT_TBA;                               /* Target Base Address */
    U32    BLT_TTY;                               /* Target Type */
    U32    BLT_TXY;                               /* Target XY Location */

    U32    BLT_CWO;                               /* Clipping Window Offset */
    U32    BLT_CWS;                               /* Clipping Window Stop */
    U32    BLT_CCO;                               /* Color Conversion Operators */
    U32    BLT_CML;                               /* CLUT Memory Location */

    U32    BLT_RZC;                               /* 2D Resize Control */
    U32    BLT_HFP;                               /* H Filter Coefficients Pointer */
    U32    BLT_VFP;                               /* V Filter Coefficients Pointer */
    U32    BLT_RSF;                               /* Resize Scaling Factor */

    U32    BLT_FF0;                               /* Flicker Filter 0 */
    U32    BLT_FF1;                               /* Flicker Filter 1 */
    U32    BLT_FF2;                               /* Flicker Filter 2 */
    U32    BLT_FF3;                               /* Flicker Filter 3 */

    U32    BLT_ACK;                               /* ALU and Color Key Control */
    U32    BLT_KEY1;                              /* Color Key 1 */
    U32    BLT_KEY2;                              /* Color Key 2 */
    U32    BLT_PMK;                               /* Plane Mask */

    U32    BLT_RST;                               /* Raster Scan Trigger */
    U32    BLT_XYL;                               /* XYL */
    U32    BLT_XYP;                               /* XY Pointer */
} stblit_HWNode_t;

typedef U32 stblit_NodeHandle_t;                  /* Handle of Node */
#define STBLIT_NO_NODE_HANDLE ((stblit_NodeHandle_t) NULL)

/* stblit_Node_t must be aligned on 128 bit word boundary.
 * A pool of stblit_Node_t is allocated at Init time by AVMEM taking into account this constraint. Unfortunately this alignement
 * concerns only the first node and not the others since there are all together allocated as one single buffer (1 STAVMEM_Alloc call for the pool).
 * The only way to keep the 128bit alignemnt is either to allocate each node separately or to make the node size be multiple
 * of 128bit word. This last solution is the one chosen! */
typedef struct
{
    stblit_HWNode_t           HWNode;            /* HW Node description */                                     /* 140 bytes */
#ifndef STBLIT_LINUX_FULL_USER_VERSION
    STEVT_SubscriberID_t      SubscriberID;      /* STBLIT_BLIT_COMPLETED_EVT  subscriber*/                    /* 4 bytes */
#endif
    U32                       ITOpcode;          /* Interrupt Opcode */                                        /* 4 bytes */
    stblit_NodeHandle_t       PreviousNode;      /* Previous node  */                                          /* 4 bytes */
    STBLIT_JobHandle_t        JobHandle;         /* Parent Job Handle if any. If not STBLIT_NO_JOB_HANDLE */   /* 4 bytes */
    U32                       Reserved;          /* reserved 4 bytes */                                        /* 4 bytes */
                                                 /* Note that in case of s/W emulation the Reserved field is used for optimization */
    /* Make sure the size 128 bit multiple (16 bytes)*/
} stblit_Node_t;

typedef struct
{
    STBLIT_Handle_t         Handle;              /* Handle of the open connection associated with the Job */
    stblit_NodeHandle_t     FirstNodeHandle;     /* Fisrt node in the Job list */
    stblit_NodeHandle_t     LastNodeHandle;      /* Last node in the Job list */
#ifndef STBLIT_LINUX_FULL_USER_VERSION
    STEVT_SubscriberID_t    SubscriberID;        /* STBLIT_JOB_COMPLETED_EVT subscriber */
#endif
    BOOL                    NotifyCompletion;    /* TRUE if user notify for Job completion */
    U32                     NbNodes;             /* Number of nodes in the Job list */
    U32                     JobValidity;         /* Only the value STBLIT_VALID_JOB means the Job is valid */
    BOOL                    Closed;              /* Job has been already submitted. No further nodes accepted*/
    BOOL                    UseLockingMechanism; /* TRUE if Job contains lock/unlock commands */
    BOOL                    PendingLock;         /* TRUE if lock has been added during Job built and there is not any unlock
                                                  * yet */
    BOOL                    LockBeforeFirstNode; /* TRUE if Job contains a LOCK before its first node */
    STBLIT_JobHandle_t      NextJob;             /* Handle of the next created job. STBLIT_NO_JOB_HANDLE if none*/
    STBLIT_JobHandle_t      PreviousJob;         /* Handle of the previous created job. STBLIT_NO_JOB_HANDLE if none*/

    STBLIT_JobBlitHandle_t  FirstJBlitHandle;    /* Handle of the first Job Blit in the job */
    STBLIT_JobBlitHandle_t  LastJBlitHandle;     /* Handle of the last Job Blit in the job */
    U32                     NbJBlit;             /* Number of Job Blit in the Job  */

} stblit_Job_t;

typedef struct
{
    STGXOBJ_ColorType_t      ColorType;            /* Color Type of the bitmap */
    STGXOBJ_BitmapType_t     BitmapType;           /* Bitmap type (Raster, MB...) */
    BOOL                     IsSubByteFormat;      /* TRUE if Sub Byte format */
    STGXOBJ_SubByteFormat_t  SubByteFormat;        /* If Sub Byte Format, tells which one */
    U32                      BitmapWidth;          /* Width of the bitmap */
    U32                      BitmapHeight;         /* Height of the bitmap */
    STGXOBJ_Rectangle_t      Rectangle;            /* Rectangle within the bitmap */
} stblit_BitmapInfo_t;

typedef struct
{
    STGXOBJ_ColorType_t      ColorType;            /* Color Type of the color */
} stblit_ColorInfo_t;

typedef struct
{
    stblit_InfoType_t       Type;                 /* Type of the source info. Either None, Color or Bitmap */
    union
    {
      stblit_BitmapInfo_t   Bitmap;               /* Bitmap info if Type if STBLIT_SOURCE_TYPE_BITMAP */
      stblit_ColorInfo_t    Color;                /* Color info if Type is STBLIT_SOURCE_TYPE_COLOR */
    } Info;
} stblit_SrcInfo_t;

typedef struct
{
    BOOL                     ConcatMode;             /* TRUE if Concat operation */
    BOOL                     SrcMB;                  /* TRUE if Src Macro Block */
    BOOL                     IdenticalDstSrc1;       /* Foreground = Destination (Rectangle + bitmap) */
    stblit_Source1Mode_t     OpMode;                 /* Operation Mode. Either Normal, Direct Fill or Direct Copy */
    stblit_JBlitXYLType_t    XYLMode;                /* XYL Mode */
    U8                       ScanConfig;             /* Scan direction, memory overlap related */
    BOOL                     MaskEnabled;            /* TRUE if mask mode */
    stblit_BitmapInfo_t      MaskInfo;               /* If Mask mode, mask info related */
    stblit_SrcInfo_t         Foreground;             /* Foreground info related */
    stblit_SrcInfo_t         Background;             /* Background info related */
    stblit_BitmapInfo_t      Destination;            /* Destination info related */

} stblit_OperationConfiguration_t;

typedef struct
{
    /* Job Blit management */
    U32                     JBlitValidity;         /* Only the value STBLIT_VALID_JOB_BLIT means the Job Blit is valid */
    STBLIT_JobHandle_t      JobHandle;             /* Parent Job Handle */
    stblit_NodeHandle_t     FirstNodeHandle;       /* Fisrt node of the Job Blit */
    stblit_NodeHandle_t     LastNodeHandle;        /* Last node of the Job Blit */
    U32                     NbNodes;               /* Number of nodes in the Job Blit */
    STBLIT_JobBlitHandle_t  NextJBlit;             /* Handle of the next Job Blit. STBLIT_NO_JOB_BLIT_HANDLE if none*/

   /* Operation configuration related */
   stblit_OperationConfiguration_t  Cfg;           /* Operation description */

} stblit_JobBlit_t;

/* Structure for common hw node field */
typedef struct
{
  U32   INS;
  U32   ACK;
  U32   RZC;
  U32   CCO;
  U32   CML;
} stblit_CommonField_t;




/* Exported Variables ------------------------------------------------------- */


/* Exported Macros ---------------------------------------------------------- */
#define stblit_OrWriteRegMem32(Address_p,Value) STSYS_WriteRegMem32LE((void*)Address_p,((STSYS_ReadRegMem32LE((void*)Address_p)) | (Value)))
#define stblit_AndWriteRegMem32(Address_p,Value) STSYS_WriteRegMem32LE((void*)Address_p,((STSYS_ReadRegMem32LE((void*)Address_p)) & (Value)))

/* Exported Functions ------------------------------------------------------- */



/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __BLT_COM_H  */

/* End of blt_com.h */
