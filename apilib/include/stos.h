/*****************************************************************************

File name   :  stos.h

Description :  Operating system independence file.


COPYRIGHT (C) STMicroelectronics 2005.

Date               Modification                                          Name
----               ------------                                          ----
04/05/2005         Added support for semaphore calls                     DG
05/01/2005         Created                                               MH

*****************************************************************************/

/*#define STOS_CHECK_PARAMETERS*/

/* Define to prevent recursive inclusion */
#if !defined __STOS_H
#define __STOS_H

/* Includes ----------------------------------------------------------------- */
#include "stddefs.h"

#if !defined __STLITE_H                  /* This part below was taken from stlite.h */

#ifdef ST_OS20
    #include <interrup.h>
    #include <kernel.h>
    #include <message.h>
    #include <mutex.h>
    #include <partitio.h>
    #include <semaphor.h>
    #include <task.h>
    #include <ostime.h>
    #include <cache.h>
#endif

#if defined (ST_OS21) && !defined(ST_OSWINCE)
    #include <os21.h>
    #include <time.h>
    #if defined (ARCHITECTURE_ST40)
        #include <os21/st40.h>
        #include <os21/st40/cache.h>
    #endif /*#if defined ARCHITECTURE_ST40*/
#endif

#endif  /* #if! defined __STLITE_H */    /* This part above was taken from stlite.h */

#ifdef ST_OSLINUX
#include "linuxwrapper.h"
#endif

/* C++ support */
#if defined __cplusplus
    extern "C" {
#endif

/* Types ------------------------------------------------------------------- */
#if defined (ST_OS21) && !defined(ST_OSWINCE)
    typedef void *  tdesc_t;
#endif

/* Defines ----------------------------------------------------------------- */
#if !defined __OS_20TO21_MAP_H && !defined(ST_OSWINCE)     /* This part below was taken from os_20to21_map.h */
#if defined(ST_OS20)
    #define task_flags_no_min_stack_size    0   /* Should be unused: only for compilation */

    #define osclock_t  clock_t
#elif defined(ST_OS21)
	#define semaphore_create_fifo_timeout(count) semaphore_create_fifo(count)
	#define semaphore_create_priority_timeout(count) semaphore_create_priority(count)
	#define interrupt_install(a,b,c,d) interrupt_install(interrupt_handle(a),(interrupt_handler_t )c,d)
	#define interrupt_install_shared(a,b,c,d) interrupt_install_shared(interrupt_handle(a),(interrupt_handler_t )c,d)
	#define interrupt_uninstall(a,b) interrupt_uninstall(interrupt_handle(a))
	#define interrupt_enable_number(number) interrupt_enable(interrupt_handle(number))
	#define interrupt_disable_number(number) interrupt_disable(interrupt_handle(number))
	#define interrupt_clear_number(number) interrupt_clear(interrupt_handle(number))
	#define interrupt_raise_number(number) interrupt_raise(interrupt_handle(number))

	#define task_flags_high_priority_process    0 /* does not exist for ST40 */
	#define clock_t  osclock_t

#elif defined(ST_OSLINUX)
    #define task_flags_no_min_stack_size        0   /* Should be unused: only for compilation */
    #define task_flags_suspended                1   /* Should be unused: only for compilation */
    #define task_flags_high_priority_process    2   /* Should be unused: only for compilation */

#endif  /* ST_OS21 */
#endif  /* #if ! defined __OS_20TO21_MAP_H */

typedef int STOS_Error_t;
#if defined(ST_OSLINUX)
    #define STOS_SUCCESS    (0)
    #define STOS_FAILURE    (-1)

#elif defined(ST_OS20)
    #define STOS_SUCCESS    (0)
    #define STOS_FAILURE    (-1)

#elif defined(ST_OS21)
    #define STOS_SUCCESS    (OS21_SUCCESS)
    #define STOS_FAILURE    (OS21_FAILURE)

#endif

/* Time management */
#if defined(ST_OSLINUX)
#if defined(MODULE)
/* LM: In kernel mode, jiffies is the right clock definition but may
introduce warnings during compilations as long as function are not
ported to stos.c. jiffies are unsigned long */
    #define STOS_Clock_t            unsigned long
#else
    #define STOS_Clock_t            clock_t
#endif

#elif defined(ST_OS20)
    #define STOS_Clock_t            clock_t

#elif defined(ST_OS21)
    #define STOS_Clock_t            osclock_t

#endif


/* Mutex management */
typedef mutex_t             STOS_Mutex_t;


/* Message Queue management */
typedef message_queue_t     STOS_MessageQueue_t;


/* Interrupt type management */
typedef enum
{
    STOS_FAST_IRQ           = 0x01,
    STOS_SHARED_IRQ         = 0x02,
    STOS_RANDOM_SEED_IRQ    = 0x04
} STOS_InterruptType_t;

typedef struct STOS_InterruptParam_s
{
    STOS_InterruptType_t    InterruptType;
} STOS_InterruptParam_t;

#if !defined __STLITE_H                /* This part below was taken from stlite.h */

#if defined (ARCHITECTURE_ST20)
    /* fix for DCU 1.8.1 - different number of parameters */
    #if OS20_VERSION_MAJOR >= 2 && OS20_VERSION_MINOR >= 7
      #define interrupt_status(a,b) interrupt_status(a,b,0)
    #endif
#endif  /* ARCHITECTURE_ST20 */

#ifndef ST_OSWINCE
/* The following is required for (legacy) PTI and TestTool compatibility,
  which require the identifiers boolean, true, and false */
#if defined __cplusplus
/* true and false are already defined. Define the type the same size
  as under C compilation below (DDTS 18417) */
    typedef int boolean;
    #else
        #if !defined(true) && !defined(false)
            typedef enum {false = 0, true = 1} boolean;
        #endif
#endif  /* __cplusplus */

/***********************************
 Toolset compatibility issues
***********************************/
/* ST20/ST40 __inline compatibility */
#if !defined __inline
    #if defined (ARCHITECTURE_ST40)
        #define __inline __inline__
    #endif
#endif

#endif /* !ST_OSWINCE */

#endif  /* #if !defined __STLITE_H */    /* This part above was taken from stlite.h */



/* Exported Functions ------------------------------------------------------- */
ST_Revision_t   STOS_GetRevision(void);

/* Tasks related functions */
ST_ErrorCode_t  STOS_TaskCreate (void (*Function)(void* Param),
                                       void* Param,
                                       partition_t* StackPartition,
                                       size_t StackSize,
                                       void** Stack,
                                       partition_t* TaskPartition,
                                       task_t** Task,
                                       tdesc_t* Tdesc,
                                       int Priority,
                                       const char* Name,
                                       task_flags_t Flags );
ST_ErrorCode_t  STOS_TaskWait   ( task_t** Task, const STOS_Clock_t * TimeOutValue_p );
ST_ErrorCode_t  STOS_TaskEnter  ( void * Param );
ST_ErrorCode_t  STOS_TaskExit   ( void * Param );
ST_ErrorCode_t  STOS_TaskDelete (task_t* Task,
                                 partition_t* TaskPartition,
                                 void* Stack,
                                 partition_t* StackPartition );


/* Semaphores related functions */
semaphore_t * STOS_SemaphoreCreateFifo           (partition_t * Partition_p, const int InitialValue);
semaphore_t * STOS_SemaphoreCreateFifoTimeOut    (partition_t * Partition_p, const int InitialValue);
semaphore_t * STOS_SemaphoreCreatePriority       (partition_t * Partition_p, const int InitialValue);
semaphore_t * STOS_SemaphoreCreatePriorityTimeOut(partition_t * Partition_p, const int InitialValue);
int  STOS_SemaphoreDelete(partition_t * Partition_p, semaphore_t * Semaphore_p);
#if !defined WINCE_USE_INLINE
void STOS_SemaphoreSignal(semaphore_t * Semaphore_p);
int  STOS_SemaphoreWait(semaphore_t * Semaphore_p);
int  STOS_SemaphoreWaitTimeOut(semaphore_t * Semaphore_p, const STOS_Clock_t * TimeOutValue);
#endif

/* Uncached memory access functions */
void    STOS_memsetUncached(const void* const PhysicalAddress, U8 Pattern, U32 Size);
void    STOS_memcpyUncachedToUncached(const void* const uncached_dest, const void* const uncached_src, U32 Size);
void    STOS_memcpyCachedToUncached(const void* const uncached_dest, const void* const uncached_src, U32 Size);
void    STOS_memcpyUncachedToCached(void* const uncached_dest, const void* const uncached_src, U32 Size);
#define STOS_memcpyCachedToCached(cached_dest, cached_src, Size) memcpy(cached_dest, cached_src, size)

#if defined(ST_OS21) || defined (ST_OSWINCE)
void * STOS_memcpy(void * dst, const void * src, size_t count);

#else
#define STOS_memcpy(cached_dest, cached_src, size)      memcpy(cached_dest, cached_src, size)
#endif /*defined ST_OS21 || defined ST_OSWINCE*/


/* Memory Management */
#define STOS_MemoryAllocate(partition,size)                 memory_allocate(partition,size)
#define STOS_MemoryDeallocate(partition,ptr)                memory_deallocate(partition,ptr)
#define STOS_MemoryAllocateClear(partition,nelem,elsize)    memory_allocate_clear(partition,nelem,elsize)
#ifdef ST_OSLINUX
#ifdef MODULE
#define STOS_MemoryReallocate(partition,ptr,size,oldsize)   memory_reallocate(partition, ptr, size, oldsize)
#else
#define STOS_MemoryReallocate(partition,ptr,size,oldsize)   memory_reallocate(partition, ptr, size)
#endif

#else
#define STOS_MemoryReallocate(partition,ptr,size,oldsize)   memory_reallocate(partition, ptr, size)
#endif


/* Regions management related functions */
#if defined(ST_OSLINUX)
#if defined(MODULE)
#define STOS_VirtToPhys(vAddr)                      ((void *)virt_to_phys((volatile void *)(vAddr)))

#define STOS_InvalidateRegion(Addr, size)           dma_cache_inv((void *)(Addr), (int)(size))
#define STOS_FlushRegion(Addr, size)                dma_cache_wback_inv((void *)(Addr), (int)(size))

#ifdef MAP_NO_REGION_REQUEST
#define STOS_MapRegisters(pAddr, Length, Name)      ioremap_nocache((unsigned long)(pAddr), Length)
#define STOS_UnmapRegisters(vAddr, Length)          iounmap((void*)(vAddr))
#else
#define STOS_MapRegisters(pAddr, Length, Name)      STLINUX_MapRegion((void *)(pAddr), Length, Name)
#define STOS_UnmapRegisters(vAddr, Length)          STLINUX_UnmapRegion((void *)(vAddr), Length)
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,12)
    /* LDDE 2.2 */
#define STOS_MapPhysToCached(pAddr, Length)         ioremap_cache((unsigned long)(pAddr), Length)
#else
    /* LDDE 2.0 */
#define STOS_MapPhysToCached(pAddr, Length)         ioremap_cached((unsigned long)(pAddr), Length)
#endif  /* LDDE 2.2 */
#define STOS_MapPhysToUncached(pAddr, Length)       ioremap_nocache((unsigned long)(pAddr), Length)
#define STOS_UnmapPhysToCached(vAddr, Length)       iounmap((void*)(vAddr))
#define STOS_UnmapPhysToUncached(vAddr, Length)     iounmap((void*)(vAddr))

#else
#define STOS_VirtToPhys(vAddr)                      ((void *)NULL)  /* Not available in User */

#define STOS_InvalidateRegion(Addr, size)                           /* Not available in User */
#define STOS_FlushRegion(Addr, size)                                /* Not available in User */

#define STOS_MapRegisters(pAddr, Length, Name)      (NULL)          /* Not available in User */
#define STOS_UnmapRegisters(vAddr, Length)                          /* Not available in User */

#define STOS_MapPhysToCached(pAddr, Length)         (NULL)          /* Not available in User */
#define STOS_MapPhysToUncached(pAddr, Length)       (NULL)          /* Not available in User */
#define STOS_UnmapPhysToCached(vAddr, Length)                       /* not available in User */
#define STOS_UnmapPhysToUncached(vAddr, Length)                     /* not available in User */
#endif  /* MODULE */

#elif defined(ST_OS21)
#ifdef VIRTUAL_MEMORY_SUPPORT
void  * STOS_VirtToPhys(void * vAddr);
#else

#ifdef NATIVE_CORE
/* On host (PC or workstation) running the VSOC simulator no map needed and map could't be done by modifiyng  address MSB */
#define STOS_VirtToPhys(vAddr)                      (vAddr)
#else
#define STOS_VirtToPhys(vAddr)                      ((void *)((U32)(vAddr) & 0x1FFFFFFF))
#endif  /* NATIVE_CORE */
#endif  /* VIRTUAL_MEMORY_SUPPORT */

#ifdef NATIVE_CORE
/* On host (PC or workstation) running the VSOC simulator no map needed and map could't be done by modifiyng  address MSB */
#define STOS_MapRegisters(pAddr, Length, Name)      (pAddr)
#define STOS_UnmapRegisters(vAddr, Length)

#define STOS_MapPhysToUncached(pAddr, Length)       (pAddr)
#define STOS_MapPhysToCached(pAddr, Length)         (pAddr)
#define STOS_UnmapPhysToCached(vAddr, Length)
#define STOS_UnmapPhysToUncached(vAddr, Length)
#else

#define STOS_MapRegisters(pAddr, Length, Name)      ((void *)((U32)(pAddr) | 0xA0000000))
#define STOS_UnmapRegisters(vAddr, Length)

#define STOS_MapPhysToUncached(pAddr, Length)       ((void *)((U32)(pAddr) | 0xA0000000))
#define STOS_MapPhysToCached(pAddr, Length)         ((void *)((((U32)pAddr) & 0xDFFFFFFF) | 0x80000000))
#define STOS_UnmapPhysToCached(vAddr, Length)
#define STOS_UnmapPhysToUncached(vAddr, Length)
#endif  /* NATIVE_CORE */

#define STOS_InvalidateRegion(Addr, size)           do {  if (size) {cache_invalidate_data((void *)(Addr), (unsigned int)(size)); } } while(0)
#define STOS_FlushRegion(Addr, size)                do {  if (size) {cache_flush_data((void *)(Addr), (unsigned int)(size)); } } while(0)

#elif defined(ST_OS20)
#define STOS_VirtToPhys(vAddr)                      (vAddr)

#define STOS_InvalidateRegion(Addr, size)           (void)cache_invalidate_data(0, 0)      /* Invalidates ALL cache lines */
#define STOS_FlushRegion(Addr, size)                                                                \
                do {                                                                                \
                    (void)cache_flush_data(0, 0);         /* Flushes ALL cache lines */             \
                    (void)cache_invalidate_data(0, 0);    /* Then invalidates ALL cache lines */    \
                } while(0)

#define STOS_MapRegisters(pAddr, Length, Name)      (pAddr)
#define STOS_UnmapRegisters(vAddr, Length)

#define STOS_MapPhysToUncached(pAddr, Length)       (pAddr)
#define STOS_MapPhysToCached(pAddr, Length)         (pAddr)
#define STOS_UnmapPhysToCached(vAddr, Length)
#define STOS_UnmapPhysToUncached(vAddr, Length)
#endif
/* Same implementation for all OSes */
#define STOS_AddressTranslate(newbase,oldbase,oldaddr)      ((void *)((U32)(newbase) + ((U32)(oldaddr) - (U32)(oldbase))))



/* Race conditions Management */
#if defined(ST_OSLINUX)
#if defined(MODULE)
    #define STOS_TaskLock()        task_lock(current)
    #define STOS_TaskUnlock()      task_unlock(current)
#else
    /* In user space, task_lock/unlock() are mapped on pthread_mutex_lock/unlock() */
    #define STOS_TaskLock()        task_lock()
    #define STOS_TaskUnlock()      task_unlock()
#endif

#elif defined(ST_OS20) || defined(ST_OS21) || defined(ST_OSWINCE)
    #define STOS_TaskLock()        task_lock()
    #define STOS_TaskUnlock()      task_unlock()
#endif


/* Time management */
#if defined(ST_OSLINUX)
    #define STOS_time_now()         time_now()
    #define STOS_time_minus(t1,t2)  time_minus(t1,t2)
    #define STOS_time_plus(t1,t2)   time_plus(t1,t2)
    #define STOS_time_after(t1,t2)  time_after_STAPI(t1,t2)

#elif defined(ST_OS20)
    #define STOS_time_now()         time_now()
    #define STOS_time_minus(t1,t2)  time_minus(t1,t2)
    #define STOS_time_plus(t1,t2)   time_plus(t1,t2)
    #define STOS_time_after(t1,t2)  time_after(t1,t2)

#elif defined(ST_OS21) || defined(ST_OSWINCE)
    #define STOS_time_now()         time_now()
    #define STOS_time_minus(t1,t2)  time_minus(t1,t2)
    #define STOS_time_plus(t1,t2)   time_plus(t1,t2)
    #define STOS_time_after(t1,t2)  time_after(t1,t2)
#endif


/* Mutex related functions */

#if defined(ST_OSLINUX)
#define STOS_MutexCreateFifo()      mutex_create_fifo()
#define STOS_MutexCreatePriority()  mutex_create_fifo()
#define STOS_MutexDelete(mutex)     mutex_delete(mutex)
#define STOS_MutexLock(mutex)       mutex_lockit(mutex)
#define STOS_MutexRelease(mutex)    mutex_release(mutex)


#elif defined(ST_OS21) || defined(ST_OS20)
#define STOS_MutexCreateFifo()      mutex_create_fifo()
#define STOS_MutexCreatePriority()  mutex_create_priority()
#define STOS_MutexDelete(mutex)     mutex_delete(mutex)
#define STOS_MutexLock(mutex)       mutex_lock(mutex)
#define STOS_MutexRelease(mutex)    mutex_release(mutex)

#endif


/* Delays related functions */
#ifdef ST_OSLINUX
  #ifdef MODULE
    #ifdef STOS_CHECK_PARAMETERS
        #define Check_TaskDelay_Param(test,param)  do {  if (test) {printk("%s(%d): WARNING !!! Inadequate parameter (0x%.8x), this may hide problem !\n", __FUNCTION__, __LINE__, (U32)(param));}  } while (0)
    #else
        #define Check_TaskDelay_Param(test,param)
    #endif

    #define STOS_TaskDelay(ticks)                                                             \
              do {                                                                            \
                  Check_TaskDelay_Param(((ticks) <= 0), ticks);                               \
                                                                                              \
                  if ((ticks) >= 0)                                                           \
                  {                                                                           \
                      /* Will perform rescheduling with delay of about the ms */              \
                      /* Transforming jiffies (actually HZ) in ms */                          \
                      msleep(((ticks) * 1000) / (unsigned int) STLINUX_GetClocksPerSecond()); \
                  }                                                                           \
              } while (0)
    #define STOS_TaskDelayUs(microsec)                                                        \
              do {                                                                            \
                  Check_TaskDelay_Param(((microsec) <= 1000000/STLINUX_GetClocksPerSecond()), microsec);    \
                                                                                              \
                  if ((microsec) >= 0)                                                        \
                  {                                                                           \
                      msleep((microsec)/1000);                                                \
                  }                                                                           \
              } while (0)
    #define STOS_TaskDelayUntil(timeout)                                                      \
              do {                                                                            \
                  int task_delay_until_timeout = STOS_time_minus((timeout), STOS_time_now()); \
                  STOS_TaskDelay(task_delay_until_timeout);                                   \
              } while (0)

  #else
    void STOS_TaskDelay(int ticks);
    void STOS_TaskDelayUs(int microsec);
    void STOS_TaskDelayUntil(STOS_Clock_t timeout);
  #endif  /* MODULE */

#elif defined (ST_OS21) || defined (ST_OS20) && !defined(ST_OSWINCE)

#define STOS_TaskDelay(ticks)           task_delay(ticks)
#define STOS_TaskDelayUntil(timeout)    task_delay_until(timeout)
#define STOS_TaskDelayUs(microsec)                                                              \
                do {                                                                            \
                    U32     ClksPerSecs = ST_GetClocksPerSecond();                              \
                    U32     Divisor = ((microsec) / (0xFFFFFFFF/ClksPerSecs)) + 1;              \
                                                                                                \
                    /* Original formula is: (microsec*ClksPerSecs)/1000000 which     */         \
                    /* translates microseconds into ticks                            */         \
                    /* but because of U32 calculus limitations, Divisor has been     */         \
                    /* introduced in order to be sure that                           */         \
                    /* (microsec*ClksPerSecs) does not exceeds U32 limit (0xFFFFFFFF)*/         \
                    /* On 7100: from 1 to 16000 microsecs, no loss of accuracy in delays */     \
                    /*          above 16000 microsecs, acceptable loss of accuracy may happen */\
                    task_delay( ((((U32)(microsec)/Divisor))*ClksPerSecs)/(1000000/Divisor) );  \
                } while (0)

#elif defined(ST_OSWINCE)
#define STOS_TaskDelay(ticks)           task_delay(ticks)
#define STOS_TaskDelayUntil(timeout)    task_delay_until(timeout)
#define STOS_TaskDelayUs(microsec)      task_delay(((unsigned long long)(microsec)*ST_GetClocksPerSecond())/1000000)

#endif


/* Task scheduling related functions */
#ifdef ST_OSLINUX
#ifdef MODULE
#define STOS_TaskSchedule()             do { schedule(); } while(0)
#define STOS_TaskYield()                do { yield(); } while(0)
#else
#define STOS_TaskSchedule()             do { sched_yield(); } while(0)
#define STOS_TaskYield()                do { sched_yield(); } while(0)
#endif

#elif defined (ST_OS21)
#define STOS_TaskSchedule()             do { task_reschedule(); } while(0)
#define STOS_TaskYield()                do { task_yield(); } while(0)

#elif defined (ST_OS20)
#define STOS_TaskSchedule()             do { task_reschedule(); } while(0)
#define STOS_TaskYield()                do { task_reschedule(); } while(0)
#endif



/* Interrupts management */
#define STOS_InterruptLock()                    interrupt_lock()
#define STOS_InterruptUnlock()                  interrupt_unlock()

#ifdef ST_OSLINUX
#ifdef MODULE
#define STOS_INTERRUPT_DECLARE(Fct, Param)      irqreturn_t Fct(int irq, void* Param, struct pt_regs* regs)
#define STOS_INTERRUPT_PROTOTYPE(Fct)           irqreturn_t(*Fct)(int, void* , struct pt_regs*)
#define STOS_INTERRUPT_CAST(Fct)                (irqreturn_t(*)(int, void* , struct pt_regs* ))Fct
#define STOS_INTERRUPT_EXIT(ret)                return (ret == STOS_SUCCESS ? IRQ_HANDLED : IRQ_NONE)

#define STOS_InterruptEnable(Number,Level)          (STOS_SUCCESS)
#define STOS_InterruptDisable(Number,Level)         (STOS_SUCCESS)

#else
#define STOS_INTERRUPT_DECLARE(Fct, Param)      void Fct(void * Param)
#define STOS_INTERRUPT_PROTOTYPE(Fct)           void(*Fct)(void*)
#define STOS_INTERRUPT_CAST(Fct)                (void(*)(void*))Fct
#define STOS_INTERRUPT_EXIT(ret)                return

#define STOS_InterruptEnable(Number,Level)          (STOS_FAILURE)
#define STOS_InterruptDisable(Number,Level)         (STOS_FAILURE)
#endif

#elif defined ST_OS21
#define STOS_INTERRUPT_DECLARE(Fct, Param)      int Fct(void * Param)
#define STOS_INTERRUPT_PROTOTYPE(Fct)           int(*Fct)(void*)
#define STOS_INTERRUPT_CAST(Fct)                (int(*)(void*))Fct
#define STOS_INTERRUPT_EXIT(ret)                return(ret == STOS_SUCCESS ? OS21_SUCCESS : OS21_FAILURE)

#define STOS_InterruptEnable(Number,Level)          (interrupt_enable(interrupt_handle(Number)) == 0 ? STOS_SUCCESS : STOS_FAILURE)
#define STOS_InterruptDisable(Number,Level)         (interrupt_disable(interrupt_handle(Number)) == 0 ? STOS_SUCCESS : STOS_FAILURE)

#elif defined ST_OS20
#define STOS_INTERRUPT_DECLARE(Fct, Param)      void Fct(void * Param)

#define STOS_INTERRUPT_PROTOTYPE(Fct)           void(*Fct)(void*)
#define STOS_INTERRUPT_CAST(Fct)                (void(*)(void*))Fct
#define STOS_INTERRUPT_EXIT(ret)                return

#if defined(STAPI_INTERRUPT_BY_NUMBER)
#define STOS_InterruptEnable(Number,Level)          (interrupt_enable_number(Number) == 0 ? STOS_SUCCESS : STOS_FAILURE)
#define STOS_InterruptDisable(Number,Level)         (interrupt_disable_number(Number) == 0 ? STOS_SUCCESS : STOS_FAILURE)
#else
#define STOS_InterruptEnable(Number,Level)          (interrupt_enable(Level) == 0 ? STOS_SUCCESS : STOS_FAILURE)
#define STOS_InterruptDisable(Number,Level)         (interrupt_disable(Level) == 0 ? STOS_SUCCESS : STOS_FAILURE)
#endif

#endif
STOS_Error_t STOS_InterruptInstallConfigurable( U32 InterruptNumber,
                                    U32 InterruptLevel,
                                    STOS_INTERRUPT_PROTOTYPE(Fct),
                                    STOS_InterruptParam_t * InterruptParam_p,
                                    char * IrqName,
                                    void * Params);
STOS_Error_t STOS_InterruptInstall( U32 InterruptNumber,
                                    U32 InterruptLevel,
                                    STOS_INTERRUPT_PROTOTYPE(Fct),
                                    char * IrqName,
                                    void * Params);
STOS_Error_t STOS_InterruptUninstall(U32 InterruptNumber,
                                     U32 InterruptLevel,
                                     void * Params);



/* Message Queue related functions */
#define STOS_MESSAGEQUEUE_MEMSIZE(Size, Nbr)            ((Size)*(Nbr))
#ifdef ST_OSLINUX
#define STOS_MessageQueueCreate(Size, Nbr)              message_create_queue_timeout(Size, Nbr)

#elif defined ST_OS21
#define STOS_MessageQueueCreate(Size, Nbr)              message_create_queue(Size, Nbr)

#elif defined ST_OS20
#define STOS_MessageQueueCreate(Size, Nbr)              message_create_queue_timeout(Size, Nbr)

#endif
#define STOS_MessageQueueDelete(Queue)                  message_delete_queue(Queue)
#define STOS_MessageQueueClaim(Queue)                   (void *)message_claim(Queue)
#define STOS_MessageQueueClaimTimeout(Queue, Timeout)   (void *)message_claim_timeout(Queue, Timeout)
#define STOS_MessageQueueRelease(Queue, Msg)            message_release(Queue, Msg)
#define STOS_MessageQueueSend(Queue, Msg)               (void)message_send(Queue, Msg)
#define STOS_MessageQueueReceive(Queue)                 message_receive(Queue)
#define STOS_MessageQueueReceiveTimeout(Queue, Timeout) message_receive_timeout(Queue, Timeout)



/* C++ support */
#if defined __cplusplus
}
#endif

#endif /* #if !defined __STOS_H */

/* End of stos.h */

