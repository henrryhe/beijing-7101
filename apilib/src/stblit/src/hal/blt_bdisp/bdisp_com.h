/*******************************************************************************

File name   : bdisp_com.h

Description : blit_common header file

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                       Name
----               ------------                                       ----
01 Jun 2000        Created											  HE
27 sep 2006        - Added BlendOp & PreMultipliedColor to job blit.  WinCE Noida Team
                   - All changes are under ST_OSWINCE option.
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __BLT_COM_H
#define __BLT_COM_H


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#include "stblit.h"

#ifdef ST_OSLINUX
#ifdef STBLIT_LINUX_FULL_USER_VERSION
#include "stblit_ioctl.h"
#endif
#endif

#if !defined(ST_OSLINUX) || defined(STBLIT_LINUX_FULL_USER_VERSION)
#include "stsys.h"
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
U32     PhysicWorkBuffer_p;
#endif


#ifndef ST_ZEUS

#ifdef STBLIT_LINUX_FULL_USER_VERSION

#if defined(ST_7100) || defined(ST_7109)

#define PHYSICAL_LMI_SYS_START                      0x7000000  /* Start LMI SYS */
#define KERNEL_LMI_SYS_START                        0xa7000000  /* Start LMI SYS */
#define LMI_SYS_WIDTH                               0x00C00000

#elif defined(ST_7200)

#define PHYSICAL_LMI_SYS_START                      0xBA00000  /* Start LMI SYS */
#define KERNEL_LMI_SYS_START                        0xABA00000  /* Start LMI SYS */
#define LMI_SYS_WIDTH                               0x063F1000

#endif


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

#endif                                  /* STBLIT_LINUX_FULL_USER_VERSION */

/* Blitter status Register */
#define BLT_STA                   0xA08
#define BLT_STA_IDLE_MASK         0x1

/* BLT Status Register 1 */
#define BLT_STA1_OFFSET           0x4
#define BLT_STA1_MASK             0xFFFFFFFF
#define BLT_STA1_SHIFT            0

/* BLT Status Register 2 */
#define BLT_STA2_OFFSET           0x8
#define BLT_STA2_MASK             0xFFF
#define BLT_STA2_SHIFT            0

/* BLT Status Register 3 */
#define BLT_STA3_OFFSET           0xC
#define BLT_STA3_MASK             0xF
#define BLT_STA3_SHIFT            0
#define BLT_STA3_READY            1
#define BLT_STA3_COMPLETED        2
#define BLT_STA3_READY2START      4
#define BLT_STA3_SUSPENDED        8

/* BLT next instruction pointer */
#define BLT_NIP_OFFSET            0x10

/* BLT packet size control */
#define BLT_PKZ_OFFSET            0xFC
#define BLT_PKZ_ENDIANNESS_SHIFT  5

/* PORT-5100 */
#define BLT_CTL             0xA00    /* BDisp control */
#define BLT_SSBA1           0xA10    /* BDisp static source base address 1 */
#define BLT_SSBA9           0xAA0    /* BDisp static source base address 9 */
#define BLT_STBA1           0xA30    /* BDisp static target base address 1 */
#define BLT_CQ1_TRIG_IP     0xA40    /* BDisp Composition Queue 1 trigger instruction pointer */
#define BLT_CQ1_TRIG_CTL    0xA44    /* BDisp Composition Queue 1 trigger control */
#if defined(ST_OSWINCE)
#define BLT_CQ1_PACE_CTL    0xA48    /* BDisp Composition Queue 1 pace control */
#define BLT_CQ1_IP			0xA4C    /* BDisp Composition Queue 1 instruction pointer */
#endif

#define BLT_ITS             0xA04    /* BDisp Interrup status */
#define BLT_ITS_AQ1_NODE_NOTIF_MASK   0X8000
#define BLT_ITS_AQ1_NODE_NOTIF_SHIFT  15
#define BLT_ITS_AQ1_STOPPED_MASK  0X2000
#define BLT_ITS_AQ1_STOPPED_SHIFT 13
#define BLT_ITS_AQ1_LNA_REACHED_MASK  0X1000
#define BLT_ITS_AQ1_LNA_REACHED_SHIFT 12
#define BLT_ITS_AQ1_RESET_INTERRUPTION 0xF000

#define BLT_STA             0xA08    /* BDisp status */

#define BLT_AQ1_CTL         0xA60    /* BDisp Application Queue 1 Control */
#define BLT_AQ2_CTL         0xA70    /* BDisp Application Queue 2 Control */
#define BLT_AQ3_CTL         0xA80    /* BDisp Application Queue 3 Control */
#define BLT_AQ4_CTL         0xA90    /* BDisp Application Queue 4 Control */

#define BLT_AQ1_IP          0xA64    /* BDisp Application Queue 1 Instruction pointer */
#define BLT_AQ2_IP          0xA74    /* BDisp Application Queue 2 Instruction pointer */
#define BLT_AQ3_IP          0xA84    /* BDisp Application Queue 3 Instruction pointer */
#define BLT_AQ4_IP          0xA94    /* BDisp Application Queue 4 Instruction pointer */

#define BLT_AQ1_LNA         0xA68    /* BDisp Application Queue 1 Last Node Address */
#define BLT_AQ2_LNA         0xA78    /* BDisp Application Queue 2 Last Node Address */
#define BLT_AQ3_LNA         0xA88    /* BDisp Application Queue 3 Last Node Address */
#define BLT_AQ4_LNA         0xA98    /* BDisp Application Queue 4 Last Node Address */

#define BLT_AQ1_STA         0xA6C    /* BDisp Application Queue 1 Last Interrupt Node Address */
#define BLT_AQ2_STA         0xA7C    /* BDisp Application Queue 2 Last Interrupt Node Address */
#define BLT_AQ3_STA         0xA8C    /* BDisp Application Queue 3 Last Interrupt Node Address */
#define BLT_AQ4_STA         0xA9C    /* BDisp Application Queue 4 Last Interrupt Node Address */

#define PRIORITY_1                     1     /* AQ1 Control Register */
#define STBLIT_AQCTL_PRIORITY_MASK     0x3
#define STBLIT_AQCTL_PRIORITY_SHIFT    0
#define COMPLETED_INT_ENA              0x8
#define LNA_REACHED_INT_ENA            0x4
#if defined(ST_OSWINCE)
#define QUEUE_STOPPED_INT_ENA          0x2
#define NODE_REPEAT_INT_ENA			   0x1
#endif
#define STBLIT_AQCTL_IRQ_MASK          0xF
#define STBLIT_AQCTL_IRQ_SHIFT         20
#define QUEUE_EN                       1
#define STBLIT_AQCTL_QUEUE_EN_MASK     0x1
#define STBLIT_AQCTL_QUEUE_EN_SHIFT    31
#define ABORT_CUURENT_NODE             2
#define STBLIT_EVENT_BEHAV_MASK        0x7
#define STBLIT_EVENT_BEHAV_SHIFT       24

#else /* ST_ZEUS */


#ifdef ST_ZEUS_PTV_MEM

#define stblit_CpuToDevice(Address)  (void*)((U32)gfx_UserToBus((void *)Address))
#define stblit_DeviceToCpu(Address)  gfx_BusToUser((unsigned long) Address)

#else	/*ST_ZEUS_PTV_MEM*/

#define stblit_CpuToDevice(Address)  (void*)(((U32)STLAYER_UserToKernel((U32)Address)) - (U32)0x91900000)
#define stblit_DeviceToCpu(Address)  (void*) STLAYER_KernelToUser((U32)(Address) +(U32)0x91900000)
#endif	/*ST_ZEUS_PTV_MEM*/


/* Blitter status Register */
#define BLT_STA                   0x008
#define BLT_STA_IDLE_MASK         0x1

/* BLT Status Register 1 */
#define BLT_STA1_OFFSET           0x4
#define BLT_STA1_MASK             0xFFFFFFFF
#define BLT_STA1_SHIFT            0

/* BLT Status Register 2 */
#define BLT_STA2_OFFSET           0x8
#define BLT_STA2_MASK             0xFFF
#define BLT_STA2_SHIFT            0

/* BLT Status Register 3 */
#define BLT_STA3_OFFSET           0xC
#define BLT_STA3_MASK             0xF
#define BLT_STA3_SHIFT            0
#define BLT_STA3_READY            1
#define BLT_STA3_COMPLETED        2
#define BLT_STA3_READY2START      4
#define BLT_STA3_SUSPENDED        8

/* BLT next instruction pointer */
#define BLT_NIP_OFFSET            0x10

/* BLT packet size control */
#define BLT_PKZ_OFFSET            0xFC
#define BLT_PKZ_ENDIANNESS_SHIFT  5

/* PORT-5100 */
#define BLT_CTL             0x000    /* BDisp control */
#define BLT_SSBA1           0x010    /* BDisp static source base address 1 */
#define BLT_SSBA9           0x0A0    /* BDisp static source base address 9 */
#define BLT_STBA1           0x030    /* BDisp static target base address 1 */
#define BLT_CQ1_TRIG_IP     0x040    /* BDisp Composition Queue 1 trigger instruction pointer */
#define BLT_CQ1_TRIG_CTL    0x044    /* BDisp Composition Queue 1 trigger control */


#define BLT_ITS             0x004    /* BDisp Interrup status */
#define BLT_ITS_AQ1_NODE_NOTIF_MASK   0X8000
#define BLT_ITS_AQ1_NODE_NOTIF_SHIFT  15
#define BLT_ITS_AQ1_STOPPED_MASK  0X2000
#define BLT_ITS_AQ1_STOPPED_SHIFT 13
#define BLT_ITS_AQ1_LNA_REACHED_MASK  0X1000
#define BLT_ITS_AQ1_LNA_REACHED_SHIFT 12
#define BLT_ITS_AQ1_RESET_INTERRUPTION 0xF000

#define BLT_STA             0x008    /* BDisp status */

#define BLT_AQ1_CTL         0x060    /* BDisp Application Queue 1 Control */
#define BLT_AQ2_CTL         0x070    /* BDisp Application Queue 2 Control */
#define BLT_AQ3_CTL         0x080    /* BDisp Application Queue 3 Control */
#define BLT_AQ4_CTL         0x090    /* BDisp Application Queue 4 Control */

#define BLT_AQ1_IP          0x064    /* BDisp Application Queue 1 Instruction pointer */
#define BLT_AQ2_IP          0x074    /* BDisp Application Queue 2 Instruction pointer */
#define BLT_AQ3_IP          0x084    /* BDisp Application Queue 3 Instruction pointer */
#define BLT_AQ4_IP          0x094    /* BDisp Application Queue 4 Instruction pointer */

#define BLT_AQ1_LNA         0x068    /* BDisp Application Queue 1 Last Node Address */
#define BLT_AQ2_LNA         0x078    /* BDisp Application Queue 2 Last Node Address */
#define BLT_AQ3_LNA         0x088    /* BDisp Application Queue 3 Last Node Address */
#define BLT_AQ4_LNA         0x098    /* BDisp Application Queue 4 Last Node Address */

#define BLT_AQ1_STA         0x06C    /* BDisp Application Queue 1 Last Interrupt Node Address */
#define BLT_AQ2_STA         0x07C    /* BDisp Application Queue 2 Last Interrupt Node Address */
#define BLT_AQ3_STA         0x08C    /* BDisp Application Queue 3 Last Interrupt Node Address */
#define BLT_AQ4_STA         0x09C    /* BDisp Application Queue 4 Last Interrupt Node Address */

#define PRIORITY_1                     1     /* AQ1 Control Register */
#define STBLIT_AQCTL_PRIORITY_MASK     0x3
#define STBLIT_AQCTL_PRIORITY_SHIFT    0
#define COMPLETED_INT_ENA              0x8
#define LNA_REACHED_INT_ENA            0x4
#define STBLIT_AQCTL_IRQ_MASK          0xF
#define STBLIT_AQCTL_IRQ_SHIFT         20
#define QUEUE_EN                       1
#define STBLIT_AQCTL_QUEUE_EN_MASK     0x1
#define STBLIT_AQCTL_QUEUE_EN_SHIFT    31
#define ABORT_CUURENT_NODE             2
#define STBLIT_EVENT_BEHAV_MASK        0x7
#define STBLIT_EVENT_BEHAV_SHIFT       24

#endif /* ST_ZEUS */


/* Blitter reset related */
#define BLT_CTL_RESET_SEQ1              0x80000000
#define BLT_CTL_RESET_SEQ2              0x00000000




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
    /* Group 0 */
    U32    BLT_NIP;                               /* Next Instruction Pointer */
    U32    BLT_CIC;                               /* Current Instruction Content  */
    U32    BLT_INS;                               /* Instruction  */
    U32    BLT_ACK;                               /* ALU and Color Key Control */

    /* Group 1 */
    U32    BLT_TBA;                               /* Target Base Address */
    U32    BLT_TTY;                               /* Target Type */
    U32    BLT_TXY;                               /* Target XY Location */
    U32    BLT_T_S1_SZ;                               /* Target Window size */

    /* Group 2 */
    U32    BLT_S1CF;                              /* Source1 Color Fill */
    U32    BLT_S2CF;                              /* Source2 Color Fill */

    /* Group 3 */
    U32    BLT_S1BA;                              /* Source1 Base Address */
    U32    BLT_S1TY;                              /* Source1 Type */
    U32    BLT_S1XY;                              /* Source1 XY Location */
    U32    BLT_STUFF_1;                           /* Stuffing */

    /* Group 4 */
    U32    BLT_S2BA;                              /* Source2 Base Address */
    U32    BLT_S2TY;                              /* Source2 Type */
    U32    BLT_S2XY;                              /* Source2 XY Location */
    U32    BLT_S2SZ;                              /* Source2 Size */

    /* Group 5 */
    U32    BLT_S3BA;                              /* Source3 Base Address */
    U32    BLT_S3TY;                              /* Source3 Type */
    U32    BLT_S3XY;                              /* Source3 XY Location */
    U32    BLT_S3SZ;                              /* Source3 Size */

    /* Group 6 */
    U32    BLT_CWO;                               /* Clipping Window Offset */
    U32    BLT_CWS;                               /* Clipping Window Stop */

    /* Group 7 */
    U32    BLT_CCO;                               /* Color Conversion Operators */
    U32    BLT_CML;                               /* CLUT Memory Location */

    /* Group 8 */
    U32    BLT_F_RZC_CTL;                         /* Filter control */
    U32    BLT_PMK;                               /* Plane Mask */

    /* Group 9 */
    U32    BLT_RSF;                               /* Resize Scaling Factor */
    U32    BLT_RZI;                               /* Resizer Initialization */
    U32    BLT_HFP;                               /* H Filter Coefficients Pointer */
    U32    BLT_VFP;                               /* V Filter Coefficients Pointer */

    /* Group 10 */
    U32    BLT_Y_RSF;                             /* Luma Resize Scaling Factor */
    U32    BLT_Y_RZI;                             /* Luma Resizer Initialization */
    U32    BLT_Y_HFP;                             /* Luma H Filter Coefficients Pointer */
    U32    BLT_Y_VFP;                             /* Luma V Filter Coefficients Pointer */

    /* Group 11 */
    U32    BLT_FF0;                               /* Flicker Filter 0 */
    U32    BLT_FF1;                               /* Flicker Filter 1 */
    U32    BLT_FF2;                               /* Flicker Filter 2 */
    U32    BLT_FF3;                               /* Flicker Filter 3 */

    /* Group 12 */
    U32    BLT_KEY1;                              /* Color Key 1 */
    U32    BLT_KEY2;                              /* Color Key 2 */

    /* Group 13 */
    U32    BLT_XYL;                               /* XYL */
    U32    BLT_XYP;                               /* XY Pointer */

    /* Group 14 */
    U32    BLT_SAR;                               /* Static adress register */
    U32    BLT_USR;                               /* User register */

#if defined (ST_7109) || defined (ST_7200)
    /* Group 15 */
    U32    BLT_IVMX0;                             /* Input Versatile Matrix 0 */
    U32    BLT_IVMX1;                             /* Input Versatile Matrix 1 */
    U32    BLT_IVMX2;                             /* Input Versatile Matrix 2 */
    U32    BLT_IVMX3;                             /* Input Versatile Matrix 3 */

    /* Group 16 */
    U32    BLT_OVMX0;                             /* Output Versatile Matrix 0 */
    U32    BLT_OVMX1;                             /* Output Versatile Matrix 1 */
    U32    BLT_OVMX2;                             /* Output Versatile Matrix 2 */
    U32    BLT_OVMX3;                             /* Output Versatile Matrix 3 */

    /* Group 17 */
    U32    BLT_DOT;                               /* BDisp Dot Arrangement Register */
    U32    BLT_PACE;                              /* Pace Counter Update */

    /* Group 18 */
    U32    BLT_VC1R;                               /* VC1 Range Register */

#endif
} stblit_HWNode_t;                              /* Node is 176 Byte wide and 64bit multiple (8 bytes) */

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
    STEVT_SubscriberID_t      SubscriberID;      /* STBLIT_BLIT_COMPLETED_EVT  subscriber*/                    /* 4 bytes */
    U32                       ITOpcode;          /* Interrupt Opcode */                                        /* 4 bytes */
    stblit_NodeHandle_t       PreviousNode;      /* Previous node  */                                          /* 4 bytes */
    STBLIT_JobHandle_t        JobHandle;         /* Parent Job Handle if any. If not STBLIT_NO_JOB_HANDLE */   /* 4 bytes */
    U32                       Reserved;          /* reserved 4 bytes */                                        /* 4 bytes */
                                                 /* Note that in case of s/W emulation the Reserved field is used for optimization */
    BOOL                      IsContinuationNode;  /*Identify Continuation Node */
    void*                       UserTag_p;           /* reserved 4 bytes */                                        /* 4 bytes */

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
#ifdef ST_OSWINCE
    BOOL                     BlendOp;                /* TRUE if ALPHA_BLEND op */
	BOOL                     PreMultipliedColor;     /* TRUE if source2 is PreMultipliedColor */
#endif
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
  U32   F_RZC_CTL;
  U32   RZI;
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
