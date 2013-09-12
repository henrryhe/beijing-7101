/*******************************************************************************

File name   : bdisp_init.h

Description : blit_init init header file (device relative definitions)

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
01 Jun 2000        Created                                          HE
25 May 2006        Modified for WinCE platfor.                      WinCE Noida):-Abhishek
                   All changes are under ST_OSWINCE flag
19 Jun 2006        stos.h not reqd for WinCE platform 				WinCE Team-ST Noida
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __BLT_INIT_H
#define __BLT_INIT_H


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#include "stblit.h"
#include "bdisp_com.h"
#if !defined(STBLIT_LINUX_FULL_USER_VERSION)
#include "stavmem.h"
#endif


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
#define MAX_NUMBER_OF_SECURE_AREAS                     16
#define STBLIT_BLIT_COMPLETED_ID        0
#define STBLIT_JOB_COMPLETED_ID         1

#ifdef STBLIT_HW_BLITTER_RESET_DEBUG
#define STBLIT_HARDWARE_ERROR_ID        2
#define STBLIT_NB_REGISTERED_EVENTS     3
#else
#define STBLIT_NB_REGISTERED_EVENTS     2
#endif /*STBLIT_HW_BLITTER_RESET_DEBUG*/

#ifdef ST_OSLINUX
#define STBLIT_TASK_STACK_SIZE       (16*1024)
#endif

/* Max 25 IT's before any of them are processed ? */
#define INTERRUPTS_BUFFER_SIZE          1000

#if defined(ST_OSLINUX) && !defined(STBLIT_LINUX_FULL_USER_VERSION)

static DECLARE_MUTEX_LOCKED(stblit_InterruptProcess_sem);

#endif

#define BLIT_MAX_AQ 4

/* Task Stack size related */
#ifndef STBLIT_INTERRUPT_PROCESS_TASK_STACK_SIZE
    #if defined(ST_OS21) || defined(ST_OSLINUX) || defined(ST_OSWINCE)                                                /* OS21 accepts 16K stack. */
        #if defined(ST_5301)                                                                                          /*For 5301(SD platform) with ST200 CPU, 5k stack is sufficient */
            #define STBLIT_INTERRUPT_PROCESS_TASK_STACK_SIZE      5 *1024
        #else
            #define STBLIT_INTERRUPT_PROCESS_TASK_STACK_SIZE      16 *1024
        #endif
    #endif
    #ifdef ST_OS20
		#define STBLIT_INTERRUPT_PROCESS_TASK_STACK_SIZE      5 *1024
    #endif
#endif

/* Task priority related */
#ifndef STBLIT_INTERRUPT_PROCESS_TASK_STACK_PRIORITY
#ifdef ST_OSLINUX
#define STBLIT_INTERRUPT_PROCESS_TASK_STACK_PRIORITY STBLIT_INTERRUPT_THREAD_PRIORITY
#else
#define STBLIT_INTERRUPT_PROCESS_TASK_STACK_PRIORITY  (10)
#endif
#endif

#ifndef STBLIT_BLITTER_CRASH_CHECK_PROCESS_TASK_STACK_PRIORITY
#ifdef ST_OSLINUX
#define STBLIT_BLITTER_CRASH_CHECK_PROCESS_TASK_STACK_PRIORITY STBLIT_INTERRUPT_THREAD_PRIORITY
#else
#define STBLIT_BLITTER_CRASH_CHECK_PROCESS_TASK_STACK_PRIORITY  (10)
#endif
#endif

#ifdef ST_OSLINUX
#define BLITINTERRUPT_NAME KERNEL_THREAD_BLITINTERRUPT_NAME
#else
#define BLITINTERRUPT_NAME "STBLIT_InterruptProcess"
#endif

#ifdef ST_OSLINUX
#define BLITTER_CRASH_CHECK_NAME KERNEL_THREAD_BLITINTERRUPT_NAME
#else
#define BLITTER_CRASH_CHECK_NAME "STBLIT_BlitterCrashCheckProcess"
#endif

/* Exported Types ----------------------------------------------------------- */

typedef struct
{
  U32                   NbFreeElem;           /* Number of Free elements in Data Pool */
  U32                   NbElem;               /* Number of elements */
  U32                   ElemSize;             /* Size of an element in Byte */
  void*                 ElemArray_p;          /* Array of elements */
  void**                HandleArray_p;        /* Array of handle (i.e array of pointers to elements)*/

  /* Semaphore used for data pool access control */
   semaphore_t*        AccessControl;

#ifdef STBLIT_ENABLE_DATA_POOL_FREE_ELEMENT_CHECK
  /* Semaphore used for data pool elements control */
    semaphore_t*        FreeElementSemaphore;
#endif /* STBLIT_ENABLE_DATA_POOL_FREE_ELEMENT_CHECK */

} stblit_DataPoolDesc_t;

typedef struct
{
    U32                         ITS;                                /* Copy of ITS register */
    U32                         AQ1_STA;                            /* Copy of AQ1_STA register */
} stblit_ItStatus;

typedef struct {
    stblit_ItStatus * Data_p;
    U32  TotalSize;
    U32  UsedSize;
    U32  MaxUsedSize;       /* To monitor max size used in commands buffer */
    stblit_ItStatus * BeginPointer_p;    /* This is a circular buffer, with BeginPointer_p and UsedSize we know what is in */
} stblit_ItStatusBuffer;

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
    void *                        SecuredBlitMappedBaseAddress_p;
#endif

    /* Events & IT related */
    ST_DeviceName_t             EventHandlerName;
    ST_DeviceName_t             BlitInterruptEventName;
#ifndef STBLIT_LINUX_FULL_USER_VERSION
    STEVT_EventConstant_t       BlitInterruptEvent;
#endif
    U32                         BlitInterruptNumber;
    U32                         BlitInterruptLevel;

    STEVT_Handle_t              EvtHandle;
    STEVT_EventID_t             EventID[STBLIT_NB_REGISTERED_EVENTS];

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

    /* Vertical & Horizontal 8-TAP filter coefficients buffer related */
    U8*                                  HFilterBuffer_p;          /* Pointer to first coeff in horizontal filter buffer */
    U8*                                  VFilterBuffer_p;          /* Pointer to first coeff in vertical filter buffer */

    /* 16 entries Palette to convert Alpha4 to Alpha8 format related */
    U8*                         Alpha4To8Palette_p;                 /* Pointer to first coeff in palette */


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

    semaphore_t*                AQInsertionSemaphore[4];            /* Semaphore used to synchronise AQ1 node insertion */

    semaphore_t*                AllBlitsCompleted_p;                /* Used for sync chip */
    BOOL                        StartWaitBlitComplete;              /* Used for sync blitter */

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

    stblit_Node_t*              FirstNodeTab[BLIT_MAX_AQ];          /* Address of first nodes in AQ lists*/
    stblit_Node_t*              ContinuationNodeTab[BLIT_MAX_AQ];
    U32                         AQ_LNA_ADDRESS[BLIT_MAX_AQ];        /* Address of AQ LNAs */
    U32                         AQ_CTL_ADDRESS[BLIT_MAX_AQ];        /* Address of AQ CTLs */
    U32                         AQ_IP_ADDRESS[BLIT_MAX_AQ];         /* Address of AQ IPs */
    BOOL                        AQFirstInsertion[BLIT_MAX_AQ];      /* First insertion in this queue */

    semaphore_t*                InterruptProcessSemaphore;          /* Semaphore used to signal when to start the Blitter
                                                                     * Interrupt process*/

    semaphore_t*                    IPSemaphore;          /* Semaphore used to synchronise between Blitter Interrupt process and
                                                         Post Submit Message function */




    stblit_ItStatus InterruptsData[INTERRUPTS_BUFFER_SIZE];

    stblit_ItStatusBuffer   InterruptsBuffer;                   /* Data concerning the interrupts status queue */

    /* Tasks related */
    BOOL                        TaskTerminate;                      /* TRUE if task must be terminated*/

    task_t*                     InterruptProcessTask_p;
    void*                       InterruptProcessTaskStack;
    tdesc_t                     InterruptProcessTaskDesc;           /* Interrupt Process task descriptor */


#ifdef STBLIT_LINUX_FULL_USER_VERSION
    pthread_t                   InterruptEventTask;                 /* task used to notify event when interrupt is generetad */
    pthread_t                   ErrorCheckProcessTask;              /* task used to check blitter hardware error */
#endif

    int                         StackInterruptProcessTask[STBLIT_INTERRUPT_PROCESS_TASK_STACK_SIZE/ sizeof(int)];      /* Interrupt Process stack */


#if defined(ST_OSLINUX) && !defined(STBLIT_LINUX_FULL_USER_VERSION)
    U32                         BlitMappedWidth;
    void*                       BlitMappedBaseAddress_p; /* Address where will be mapped the driver registers */
#endif
 /* Overwrite Support */
    void*                       UserTag_p;

#ifdef STBLIT_ENABLE_BASIC_TRACE
    U32                         NbBltSub;
    U32                         NbBltInt;
#endif


#ifdef STBLIT_HW_BLITTER_RESET_DEBUG
    BOOL                        BlitterErrorCheck_WaitingForInterrupt;
    BOOL                        BlitterErrorCheck_GotOneInterrupt;

    semaphore_t*                BlitterErrorCheck_AccessControlSemaphore;
    semaphore_t*                BlitterErrorCheck_InterruptSemaphore;
    semaphore_t*                BlitterErrorCheck_SubmissionSemaphore;

    task_t*                     BlitterErrorCheckProcessTask_p;
    void*                       BlitterErrorCheckProcessTaskStack;
    tdesc_t                     BlitterErrorCheckProcessTaskDesc;           /* Blitter error Check Process task descriptor */

    int                         StackBlitterErrorCheckProcessTask[STBLIT_BLITTER_CRASH_CHECK_PROCESS_TASK_STACK_SIZE/ sizeof(int)];      /* Blitter Crash Check Process stack */
#endif /* STBLIT_HW_BLITTER_RESET_DEBUG */

#ifdef STBLIT_ENABLE_HW_BLITTER_RESET_SIMULATION
    U32                         NbSubmission;
#endif /* STBLIT_ENABLE_HW_BLITTER_RESET_SIMULATION */

#ifdef STBLIT_DEBUG_GET_STATISTICS
    semaphore_t*                StatisticsAccessControl_p;
    STOS_Clock_t                GenerationStartTime, GenerationEndTime, ExecutionStartTime, ExecutionEndTime;
    U32                         TotalGenerationTime, TotalExecutionTime, TotalProcessingTime, TotalExecutionRate, LatestBlitWidth, LatestBlitHeight;
    STBLIT_Statistics_t         stblit_Statistics;
#ifdef STBLIT_DEBUG_STATISTICS_USE_FIRST_AND_LAST_TIME_VALUE_ONLY
    BOOL                        ExecutionRateCalculationStarted;
#endif /* STBLIT_DEBUG_STATISTICS_USE_FIRST_AND_LAST_TIME_VALUE_ONLY */

#endif
    BOOL    AntiFlutterOn;  /*to control the anti-flutter of target bitmaps*/
    BOOL    AntiAliasingOn;  /*to control the anti-aliasing of target bitmaps*/

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
