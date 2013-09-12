#ifndef __H_KERNEL_WRAPPER
#define __H_KERNEL_WRAPPER

#include <linux/kernel.h>
#include <linux/time.h>     /* struct timeval           */
#include <linux/delay.h>    /* udelay etc.              */
#include <linux/sched.h>    /* jiffies (why here?)      */
#include <linux/interrupt.h>    /* Guess.               */

#include <asm/semaphore.h>  /* semaphores, up/down etc. */
#include <asm/param.h>      /* HZ                       */
#include <asm/string.h>     /* memcpy                   */
#include <asm/current.h>    /* 'current' variable       */

/* This file should have been changed to include no external header
 * files when building kernel code.
 */
#include "stddefs.h"

/* Compiler wrapping */
#ifndef __inline
#define __inline            __inline__
#endif

/* OS wrapping */

typedef struct semaphore_s
{
    wait_queue_head_t   wait_queue;
    long                Count;
} semaphore_t;

typedef struct SemaphoreList_s
{
    struct SemaphoreList_s    * Next_p;
    semaphore_t               * Semaphore_p;
} SemaphoreList_t;

/* task_t already mapped in sched.h */
typedef int                 tdesc_t;        /* unused */

#define TIMEOUT_INFINITY    ((clock_t*)0xFFFFFFFF)
#define TIMEOUT_IMMEDIATE   ((clock_t*)0x00000000)

/* Has no meaning */
#define MAX_USER_PRIORITY   0

#if 0 && defined(NO_TASK_T)
typedef struct task         task_t;
#endif

/* STTBX wrapping */
#define STTBX_REPORT_LEVEL_INFO     0
#define STTBX_REPORT_LEVEL_ERROR    1
#define STTBX_REPORT_LEVEL_WARNING  2

/* STEVT wrapping */
#if 0
typedef int     STEVT_Handle_t;
typedef int     STEVT_EventID_t;
typedef int     STEVT_EventConstant_t;
typedef int     STEVT_CallReason_t;
#endif

#if 0
/* STAVMEM wrapping */
typedef int     STAVMEM_PartitionHandle_t;
typedef int     STAVMEM_BlockHandle_t;
#endif

/*********** Prototypes *********/

/* OS wrapping */
int     semaphore_init_fifo (semaphore_t* Semaphore, int Count);
int     semaphore_init_fifo_timeout (semaphore_t* Semaphore, int Count);
int     semaphore_wait (semaphore_t* Semaphore);
int     semaphore_wait_timeout (semaphore_t* Semaphore, 
                                const clock_t* timeout);
int     semaphore_signal (semaphore_t* Semaphore);
void    semaphore_delete (semaphore_t* Semaphore);
void*   memory_allocate (partition_t* Partition, size_t Requested);
void    memory_deallocate (partition_t* Partition, void* Block);
void*   memory_allocate_clear (
    partition_t*    Partition, 
    size_t          num_elems, 
    size_t          element_size);
#define time_plus(a, b)     ((a) + (b))
#define time_minus(a, b)    ((a) - (b))
/* Can you do this?? */
#define task_lock()         task_lock(current)
#define task_unlock()       task_unlock(current)
#define time_now()          jiffies
/* XXX Temporary */
#define task_delay          udelay
#define interrupt_lock      local_irq_disable
#define interrupt_unlock    local_irq_enable

/* General STAPI wrapping */
#define ST_GetClocksPerSecond()     HZ
/* Means any parameter in clock ticks is actually in milliseconds */
/* #define ST_GetClocksPerSecond()     1000 */
#define STTBX_WaitMicroseconds      udelay

/* STSYS wrapping */
u8 STSYS_ReadRegDev8(u32 address);
void STSYS_WriteRegDev8(u32 address, u8 value);
u16 STSYS_ReadRegDev16LE(u32 address);
void STSYS_WriteRegDev16LE(u32 address, u16 value);
void STSYS_WriteRegDev32LE(u32 address, u32 value);
u32 STSYS_ReadRegDev32LE(u32 address);

/* Sometimes I hate C. */
void sttbx_Print(const char *format, ...);
void sttbx_Report(int level, const char *format, ...);

/* STTBX wrapping */
/* #define STTBX_Report(x) sttbx_Report x */
#if defined STTBX_PRINT
#define STTBX_Report(x)                                                 \
            {                                                               \
              /* sttbx_ReportFileLine has to be called here
                otherwise FILE and LINE would always be the same */         \
              printk(KERN_ALERT "%s %i:\n", __FILE__, __LINE__);    \
              sttbx_Report x;                                        \
            }

#define STTBX_Print(x)  sttbx_Print x
#else
#define STTBX_Report(x)
#define STTBX_Print(x)
#endif

/* STEVT wrapping */
#if 0
void STEVT_Notify(STEVT_Handle_t eventhandle, 
                  STEVT_EventID_t eventid, void *ptr);
ST_ErrorCode_t STEVT_UnsubscribeDeviceEvent (STEVT_Handle_t Handle,
                                             const ST_DeviceName_t RegistrantName,
                                             STEVT_EventConstant_t Event);
ST_ErrorCode_t STEVT_Close (STEVT_Handle_t Handle);
#endif
#endif

