/*******************************************************************************

File name   : blt_init.h

Description : blit_init init header file (device relative definitions)

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
13 Jun 2000        Created                                          TM
29 May 2006        Modified for WinCE platform						WinCE Team-ST Noida
19 Jun 2006        stos.h not reqd for WinCE platform 				WinCE Team-ST Noida
******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __BLT_INIT_H
#define __BLT_INIT_H


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#include "stblit.h"
#include "blt_com.h"
#if !defined(STBLIT_LINUX_FULL_USER_VERSION)
#include "stavmem.h"
#endif
#include "bltswcfg.h"

#if !defined(ST_OSWINCE)
#if !defined(STBLIT_LINUX_FULL_USER_VERSION)
#include "stos.h"
#endif
#endif

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */
#define STBLIT_INVALID_HANDLE  ((STBLIT_Handle_t) NULL)
#define STBLIT_VALID_UNIT      0x090D90D0   /* 0xXYnZYnZX with the driver ID XYZ in hexa
                                             * STBLIT Id = 157 => 0x09D
                                             * n = 0 for STBLIT open handle*/

#define STBLIT_BLIT_COMPLETED_ID      0
#define STBLIT_JOB_COMPLETED_ID       1
#define STBLIT_NB_REGISTERED_EVENTS   2

#ifdef ST_7015                  /* 7015 h/w bug list related */

#define WA_GNBvd06529                           /* 3.4.5 : After "suspend after current blit" IT the NIP is not evaluated => Soft reset*/
#define WA_GNBvd06530                           /* 3.4.6 : After "blit completed" it the NIP is not evaluated => Soft reset*/
#define WA_GNBvd06658                           /* 3.4.8 : VResize/FF (W2 width>= 128) and clipping window couln't be use in the same blit operation => error returned*/
#define WA_VRESIZE_OR_FLICKER_AND_YCRCB_TARGET  /* 3.4.13 : V+HResize (S2 width >= 128) or flicker filter + YCrCb 422Raster Target => error returned*/
#define WA_VRESIZE_OR_FLICKER_AND_SRC1          /* 3.4.9 : Vertical resize (S2 width >= 128) or flicker filter + src1 activ => error returned */
#define WA_HORIZONTAL_POSITION_ODD              /* 3.4.14 : Src2 42X Raster/MB > 128 and FF or VResize, horizontal start position has to be odd */
#define WA_FILL_OPERATION                       /* 3.4.102 : Soft reset the blitter after each fill color*/
#define WA_DIRECT_COPY_FILL_OPERATION           /* 3.4.103 : Direct fill/copy problem with data alignment => Remove DF */
#define WA_WIDTH_8176_BYTES_LIMITATION          /* 3.4.106 : Width >= 8176 bytes => error returned */
#define WA_SOFT_RESET_EACH_NODE                 /* 3.4.107 : Perform soft reset after every node */
#endif

#ifdef ST_GX1                   /* STGX1 Cut 1.1 h/w bug workaround */

#define WA_VRESIZE_OR_FLICKER_AND_YCRCB_TARGET  /* 3.4.13 : V+HResize (S2 width >= 128) or flicker filter + YCrCb 422Raster Target => error returned*/
#define WA_HORIZONTAL_POSITION_ODD              /* 3.4.14 : Src2 42X Raster/MB > 128 and FF or VResize, horizontal start position has to be odd */
#define WA_WIDTH_8176_BYTES_LIMITATION          /* 3.4.106 : Width >= 8176 bytes => error returned */
#define WA_GNBvd12051                           /* The engine take into account an additional subinstruction from array in XYL mode */
#define WA_GNBvd10454                           /* VF/FF without HF, too many pixels out => WA is to enable HF with scaling factor of 1024 (x1), when VF or FF */
#define WA_GNBvd07516                           /* 2D rescale operator disabling */                  /* Ok, WA Implicely implemented in this code */
#define WA_GNBvd13605                           /* IT ready2start not working with trigger events */ /* Ok => IT not used in the code */
#define WA_GNBvd14732                           /* S2 Copy (src1 disabled) with src1 in macroblock type crashes */  /* Ok, WA Implicely implemented in this code => When programming Src2 copy (Sr1 is disabled), S1TY is never programmed MB */
#define WA_GNBvd08376                           /* Src in MB, Dst in Clut4/2/1 not working => Error returned */
#define WA_GNBvd09730                           /* Color fill in CLUT4/2/1 not functionnal => Error returned */
#define WA_GNBvd13604                           /* Color expansion not working in ACLUT44 => Error returned */
#define WA_GNBvd13838                           /* V+HResize (S2 width >= 128) or flicker filter and Dst CLUT1 generates bad pixels => Error returned */
#define WA_GNBvd15279                           /* Bad color fill after a MB blit => Soft reset */
#define WA_GNBvd15280                           /* MB blits generate transparent pixels when Dst has alpha per pix => Error returned */
#define WA_GNBvd15283                           /* Color reduction is enabled even if the clut operator is disable in INS */ /* Ok, WA Implicely implemented in this code */
#define WA_GNBvd15285                           /* Copy from CLUT to ACLUT give transparent pixel */  /* Ok, WA Implicely implemented in this code. Taken into account in color conversion table */

/* Bug supposed to be fixed but still pb encountered ! */
#define WA_FILL_OPERATION                       /* 3.4.102 : Soft reset the blitter after each fill color*/
#define WA_SOFT_RESET_EACH_NODE                 /* 3.4.107 : Perform soft reset after every node */
#define WA_DIRECT_COPY_FILL_OPERATION           /* 3.4.103 : Direct fill/copy problem with data alignment => Remove DF */



/* Bugs fixed on cut 1.1 */
/* #define WA_VRESIZE_OR_FLICKER_AND_SRC1 */         /* 3.4.9 : Vertical resize (S2 width >= 128) or flicker filter + src1 activ => error returned */
/* #define WA_GNBvd08068 */                          /* Stall when performing a node S2 on 42X mode after a node S2 not in 42X */
/* #define WA_GNBvd08373 */                          /* Vertical filtering may stall depending of stbus traffic */
/* #define WA_GNBvd08374 */                          /* Horizontal resize node corrupt following blit per column */
/* #define WA_GNBvd06529 */                          /* 3.4.5 : After "suspend after current blit" IT the NIP is not evaluated => Soft reset*/
/* #define WA_GNBvd06530 */                          /* 3.4.6 : After "blit completed" it the NIP is not evaluated => Soft reset*/
/* #define WA_GNBvd06658 */                          /* 3.4.8 : VResize/FF (W2 width>= 128) and clipping window couln't be use in the same blit operation => error returned*/

/* DDTS entries correspondance in ST40GX1 bug list
#define WA_GNBvd08497 WA_GNBvd10454
#define WA_GNBvd07515 WA_VRESIZE_OR_FLICKER_AND_SRC1
#define WA_GNBvd07517 WA_VRESIZE_OR_FLICKER_AND_YCRCB_TARGET
#define WA_GNBvd13625 WA_VRESIZE_OR_FLICKER_AND_YCRCB_TARGET
#define WA_GNBvd13626 WA_GNBvd09730
#define WA_GNBvd13627 WA_GNBvd13604
#define WA_GNBvd14976 WA_GNBvd13838
#define WA_GNBvd08064 WA_HORIZONTAL_POSITION_ODD
#define WA_GNBvd08065 WA_FILL_OPERATION
#define WA_GNBvd08066 WA_DIRECT_COPY_FILL_OPERATION
#define WA_GNBvd08098 WA_WIDTH_8176_BYTES_LIMITATION
*/
#endif


#if defined(ST_7020) || defined(ST_5528) || defined(ST_7710) || defined(ST_7100)        /* sti7020/sti5528 h/w bug workaround */

#define WA_VRESIZE_OR_FLICKER_AND_YCRCB_TARGET  /* 3.4.13 : V+HResize (S2 width >= 128) or flicker filter + YCrCb 422Raster Target => error returned*/
#define WA_GNBvd12051                           /* The engine take into account an additional subinstruction from array in XYL mode */
#define WA_GNBvd09730                           /* Color fill in CLUT4/2/1 not functionnal => Error returned */
#define WA_GNBvd13604                           /* Color expansion not working in ACLUT44 => Error returned */
#define WA_GNBvd13838                           /* V+HResize (S2 width >= 128) or flicker filter and Dst CLUT1 generates bad pixels => Error returned */
#define WA_GNBvd13605                           /* IT ready2start not working with trigger events */ /* Ok => IT not used in the code */
#define WA_GNBvd15279                           /* Bad color fill after a MB blit => Soft reset */
#define WA_GNBvd15280                           /* MB blits generate transparent pixels when Dst has alpha per pix => Error returned */
#define WA_GNBvd15283                           /* Color reduction is enabled even if the clut operator is disable in INS */ /* Ok, WA Implicely implemented in this code */
#define WA_GNBvd15285                           /* Copy from CLUT to ACLUT give transparent pixel */  /* Ok, WA Implicely implemented in this code. Taken into account in color conversion table */
#define WA_GNBvd49348                           /* wrong behavior when alpha blending and FF => WA is to enable HF with scaling factor of 1024 (x1) when this case*/

/* Bug supposed to be fixed but still pb encountered ! */
#define WA_FILL_OPERATION                       /* 3.4.102 : Soft reset the blitter after each fill color*/
#define WA_SOFT_RESET_EACH_NODE                 /* 3.4.107 : Perform soft reset after every node */
#define WA_DIRECT_COPY_FILL_OPERATION           /* 3.4.103 : Direct fill/copy problem with data alignment => Remove DF */

/* Bugs fixed on 7020 */
/* #define WA_GNBvd08376 */                          /* Src in MB, Dst in Clut4/2/1 not working */
/* #define WA_GNBvd07516 */                          /* 2D rescale operator disabling */                  /* Ok, WA Implicely implemented in this code */
/* #define WA_GNBvd14732 */                          /* S2 Copy (src1 disabled) with src1 in macroblock type crashes */  /* Ok, WA Implicely implemented in this code => When programming Src2 copy (Sr1 is disabled), S1TY is never programmed MB */
/* #define WA_GNBvd10454 */                          /* VF/FF without HF, too many pixels out => WA is to enable HF with scaling factor of 1024 (x1), when VF or FF */
/* #define WA_HORIZONTAL_POSITION_ODD */             /* 3.4.14 : Src2 42X Raster/MB > 128 and FF or VResize, horizontal start position has to be odd */
/* #define WA_WIDTH_8176_BYTES_LIMITATION */         /* 3.4.106 : Width >= 8176 bytes => error returned */
/* #define WA_VRESIZE_OR_FLICKER_AND_SRC1 */         /* 3.4.9 : Vertical resize (S2 width >= 128) or flicker filter + src1 activ => error returned */
/* #define WA_GNBvd08068 */                          /* Stall when performing a node S2 on 42X mode after a node S2 not in 42X */
/* #define WA_GNBvd08373 */                          /* Vertical filtering may stall depending of stbus traffic */
/* #define WA_GNBvd08374 */                          /* Horizontal resize node corrupt following blit per column */

/* DDTS entries correspondance in 7020 bug list
#define WA_GNBvd13625 WA_VRESIZE_OR_FLICKER_AND_YCRCB_TARGET
#define WA_GNBvd13626 WA_GNBvd09730
#define WA_GNBvd13627 WA_GNBvd13604
#define WA_GNBvd13645 WA_GNBvd13605
#define WA_GNBvd13891 WA_GNBvd12051
#define WA_GNBvd14976 WA_GNBvd13838
*/

#endif






#if defined(ST_OSLINUX) && !defined(STBLIT_LINUX_FULL_USER_VERSION)



static DECLARE_MUTEX_LOCKED(stblit_MasterProcess_sem);
static DECLARE_MUTEX_LOCKED(stblit_SlaveProcess_sem);
static DECLARE_MUTEX_LOCKED(stblit_InterruptProcess_sem);

#endif



/* Exported Types ----------------------------------------------------------- */

typedef struct
{
  U32                   NbFreeElem;           /* Number of Free elements in Data Pool */
  U32                   NbElem;               /* Number of elements */
  U32                   ElemSize;             /* Size of an element in Byte */
  void*                 ElemArray_p;          /* Array of elements */
  void**                HandleArray_p;        /* Array of handle (i.e array of pointers to elements)*/
} stblit_DataPoolDesc_t;

typedef struct
{
    /* common stuff */
    STBLIT_DeviceType_t                     DeviceType;
    ST_DeviceName_t                         DeviceName;
    ST_Partition_t*                         CPUPartition_p;
    void*                                   BaseAddress_p;
#ifdef STBLIT_LINUX_FULL_USER_VERSION
    void*                                   SingleBlitNodeBuffer_p ;
    void*                                   JobBlitNodeBuffer_p ;
#else
    STAVMEM_PartitionHandle_t               AVMEMPartition;
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;
#endif
    void*                                   SharedMemoryBaseAddress_p;
    U32                                     MaxHandles;
    semaphore_t*                            AccessControl;    /* Semaphore used for the device structure access control */

#ifdef STBLIT_LINUX_FULL_USER_VERSION
    FILE                        * BLIT_DevFile;            /* Linux Device file */
    U32                           BlitMappedWidth;
    void *                        BlitMappedBaseAddress_p; /* Address where will be mapped the driver registers */
#endif

    /* Events & IT related */
    ST_DeviceName_t             EventHandlerName;
    ST_DeviceName_t             BlitInterruptEventName;
#ifndef STBLIT_LINUX_FULL_USER_VERSION
    STEVT_EventConstant_t       BlitInterruptEvent;
#endif
    U32                         BlitInterruptNumber;
    U32                         BlitInterruptLevel;
#ifndef STBLIT_LINUX_FULL_USER_VERSION
    STEVT_Handle_t              EvtHandle;
    STEVT_EventID_t             EventID[STBLIT_NB_REGISTERED_EVENTS];
#endif

    /* Single Blit node related */
    U32                         SBlitNodeNumber;                    /* Number of Single blit nodes */
    stblit_DataPoolDesc_t       SBlitNodeDataPool;                  /* Data Pool for nodes used for single
                                                                     * blit operations => CPU MEMORY MAPPING */
#ifndef STBLIT_LINUX_FULL_USER_VERSION
    STAVMEM_BlockHandle_t       SBlitNodeBufferHandle;              /* AVMEM Handle if driver allocation for node buffer.
                                                                     * STAVMEM_INVALID_BLOCK_HANDLE if user allocated */
#endif
    /* Job descriptor related */
    U32                         JobNumber;                          /* Number of job descriptors */
    stblit_DataPoolDesc_t       JobDataPool;                        /* Data Pool for Job descriptors*/
    BOOL                        JobBufferUserAllocated;             /* TRUE if Job buffer allocated by
                                                                     * the user. Used for deallocation */
    /* Job Blit node related */
    U32                         JBlitNodeNumber;                    /* Number of Job blit nodes */
    stblit_DataPoolDesc_t       JBlitNodeDataPool;                  /* Data Pool for nodes used for blit operations
                                                                     * which are part of a job  => CPU MEMORY MAPPING*/
#ifndef STBLIT_LINUX_FULL_USER_VERSION
    STAVMEM_BlockHandle_t       JBlitNodeBufferHandle;              /* AVMEM Handle if driver allocation for node buffer.
                                                                     * STAVMEM_INVALID_BLOCK_HANDLE if user allocated */
#endif
    /* Job Blit descriptor related */
    U32                         JBlitNumber;                        /* Number of job blit */
    stblit_DataPoolDesc_t       JBlitDataPool;                      /* Data Pool for objects recording the context
                                                                     * of blit operations which are part of a job */
    BOOL                        JBlitBufferUserAllocated;           /* TRUE if JBlit buffer allocated by
                                                                     * the user. Used for deallocation */

    /* Work buffer related */
    U32                         WorkBufferSize;                     /* Size of the work buffer in byte */
    void*                       WorkBuffer_p;                       /* Buffer used by the driver */
#ifndef STBLIT_LINUX_FULL_USER_VERSION
    STAVMEM_BlockHandle_t       WorkBufferHandle;                   /* AVMEM Handle if driver allocation.
                                                                     * STAVMEM_INVALID_BLOCK_HANDLE if user allocated */
#endif

    /* Vertical & Horizontal 5-TAP filter coefficients buffer related */
    U8*                         FilterBuffer_p;                     /* Pointer to first coeff in filter buffer */

    /* 16 entries Palette to convert Alpha4 to Alpha8 format related */
    U8*                         Alpha4To8Palette_p;                 /* Pointer to first coeff in palette */

    /* Message Queue related */
    void*                       LowPriorityQueueBuffer_p;           /* Data buffer for Low priority queue */
    void*                       HighPriorityQueueBuffer_p;          /* Data buffer for High priority queue */
    #ifdef STBLIT_LINUX_FULL_USER_VERSION
            pthread_t               MasterTask;                         /* Master task used for High priority requests */
            pthread_t               SlaveTask;                          /* Slave task used for Low priority requests */
            pthread_t               InterruptProcessTask;               /* task used for Interrupt Process */

            pthread_t               InterruptEventTask;                 /* task used to notify event when interrupt is generetad */
            pthread_t               InterruptHandlerTask;               /* task used to signal interrupt process  */
    #endif

    /* Master task used for High priority requests */
    task_t*                         MasterTask_p;
    void*                           MasterTaskStack;
    tdesc_t                         MasterTaskDesc;

    /* Slave task used for Low priority requests */
    task_t*                         SlaveTask_p;
    void*                           SlaveTaskStack;
    tdesc_t                         SlaveTaskDesc;

    /* task used for Interrupt Process */
    task_t*                         InterruptProcessTask_p;
    void*                           InterruptProcessTaskStack;
    tdesc_t                         InterruptProcessTaskDesc;

    STOS_MessageQueue_t             *LowPriorityQueue;                  /* Back-end Queue for Low priority messages */
    STOS_MessageQueue_t             *HighPriorityQueue;                 /* Back-end Queue for High priority messages*/
    BOOL                            LowPriorityLockSent;                /* LOCK message already sent in the low priority queue */

    semaphore_t*                    BackEndQueueControl;                /* Semaphore used for the 2 Back-end message queues
                                                                        * control */
    semaphore_t*                    SlaveTerminatedSemaphore;           /* Semaphore used to tell when the Slave Queue mechanism
                                                                        * is terminated so that the termination of the master queue
                                                                        * can be done */
    #if defined(ST_OSLINUX) && !defined(STBLIT_LINUX_FULL_USER_VERSION)
        semaphore_t*                 MasterTerminatedSemaphore;
    #endif
    /* Back_end / Hardware blitter device related */
    BOOL                        HWUnLocked;                         /* HW Lock Status */
    semaphore_t*                 HWUnlockedSemaphore;                /* Semaphore used to signal when the Hardware blitter
                                                                        * device is unlocked */
    BOOL                        LockRequest;                        /* TRUE if a Lock mechanism has to be install during an interrupt */
    semaphore_t*                 LockInstalledSemaphore;             /* Semaphore used to signal when a lock mechanism has been installed
                                                                        *  by the interrupt routine */
    BOOL                        UnlockRequest;                      /* TRUE if a Unlock mechanism has to be install during an interrupt */
    semaphore_t*                 UnlockInstalledSemaphore;           /* Semaphore used to signal when a Unlock mechanism has been installed
                                                                        *  by the interrupt routine */
    BOOL                        InsertionRequest;                   /* TRUE if a node insertion has to be done during an interrupt */
    semaphore_t*                 InsertionDoneSemaphore;             /* Semaphore used to signal when a node insertion has been done by
                                                                        * the interrupt routine */
    stblit_Node_t*              FirstNode_p;                        /* Address of the first node to be inserted in case of request*/
    stblit_Node_t*              LastNode_p;                         /* Address of the last node to be inserted in case of request */
    BOOL                        InsertAtFront;                      /* FALSE : prepend node insertion in the HW queue
                                                                     * TRUE : append node insertion in the HW queue */
    BOOL                        LockBeforeFirstNode;                /* TRUE only if there is a lock before the first node to insert
                                                                     * (Used in case of job node) */
    stblit_Node_t*              LastNodeInBlitList_p;               /* Last node in the hardware list of the blitter which has
                                                                     * not been processed yet */
    BOOL                        BlitterIdle;                        /* If TRUE, the blitter finished to process the full node
                                                                     * list and wait for nodes to be inserted */
    U32                         Status1;                            /* Copy of Status 1 register  => CPU MEMORY MAPPING */
    U32                         Status3;                            /* Copy of Status 3 register */
    semaphore_t*                InterruptProcessSemaphore;          /* Semaphore used to signal when to start the Blitter
                                                                    * Interrupt process*/
    #ifdef STBLIT_LINUX_FULL_USER_VERSION
        semaphore_t*            BlitInterruptHandlerSemaphore;       /* Semaphore used to signal when to start the Blitter
                                                                      * Interrupt task handler*/
    #endif

    semaphore_t*                AllBlitsCompleted_p;                /* Used for sync chip */
    BOOL                        StartWaitBlitComplete;              /* Used for sync chip */

	#ifdef STBLIT_EMULATOR    /* Blitter emulator related */
        semaphore_t*            EmulatorStartISRSemaphore;          /* Semaphore to start Interrupt Service routine emulation */
        semaphore_t*            EmulatorStopISRSemaphore;           /* Semaphore to tell Interrupt Service routine is done  */
        semaphore_t*            EmulatorStartSemaphore;
        semaphore_t*            EmulatorSuspendSemaphore;

        #if defined(ST_OS21) || defined(ST_OSLINUX) || defined(ST_OSWINCE)
			task_t*                 EmulatorISRTask;
			task_t*                 EmulatorTask;
        #endif
        #ifdef ST_OS20
			task_t                  EmulatorISRTask;
			tdesc_t                 EmulatorISRTaskDesc;
			task_t                  EmulatorTask;
			tdesc_t                 EmulatorTaskDesc;
        #endif
	    int                         StackEmulatorISRTask[STBLIT_EMULATOR_ISR_TASK_STACK_SIZE/ sizeof(int)];
		int                         StackEmulatorTask[STBLIT_EMULATOR_TASK_STACK_SIZE/ sizeof(int)];
#ifndef STBLIT_LINUX_FULL_USER_VERSION
        STAVMEM_BlockHandle_t       EmulatorRegisterHandle;
#endif
	#endif

    /* Tasks related */
    BOOL                        TaskTerminate;                      /* TRUE if task must be terminated*/
    int                         StackMasterTask[STBLIT_MASTER_TASK_STACK_SIZE/ sizeof(int)];               /* Master stack */
    int                         StackSlaveTask[STBLIT_SLAVE_TASK_STACK_SIZE/ sizeof(int)];                 /* Slave stack */
    int                         StackInterruptProcessTask[STBLIT_INTERRUPT_PROCESS_TASK_STACK_SIZE/ sizeof(int)];      /* Interrupt Process stack */

#if defined(ST_OSLINUX) && !defined(STBLIT_LINUX_FULL_USER_VERSION)
    U32                         BlitMappedWidth;
    void*                       BlitMappedBaseAddress_p; /* Address where will be mapped the driver registers */
#endif

   /* FlickerFilter Mode */
    STBLIT_FlickerFilterMode_t       FlickerFilterMode;

    BOOL    AntiFlutterOn;   /* Used to control the AntiFlutter feature of blit operations*/
    BOOL    AntiAliasingOn;  /* Used to control the AntiAliasing feature*/


} stblit_Device_t;

 typedef struct
{
    stblit_Device_t*      Device_p;
    STBLIT_Handle_t       Handle;
    STBLIT_JobHandle_t    FirstCreatedJob;  /* Handle of the first created job in list. STBLIT_NO_JOB_HANDLE if none*/
    STBLIT_JobHandle_t    LastCreatedJob;   /* Handle of the last created job in list. STBLIT_NO_JOB_HANDLE if none*/
    U32                   NbCreatedJobs;    /* Number of created jobs in list (job in used by application)*/
    U32                   UnitValidity;     /* Only the value BLIT_VALID_UNIT means the unit is valid */
} stblit_Unit_t;

/* Exported Variables ------------------------------------------------------- */


/* Exported Macros ---------------------------------------------------------- */


/* Exported Functions ------------------------------------------------------- */


/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __BLT_INIT_H */

/* End of blt_init.h  */
