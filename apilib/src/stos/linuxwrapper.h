#ifndef __H_LINUXWRAPPER
#define __H_LINUXWRAPPER

#include <linux/version.h>

#ifdef MODULE
#include <linux/config.h>

/* next line to check */
#include <linux/module.h>
#include <linux/kernel.h> /* Needed for KERN_ALERT */
#include <linux/init.h>   /* Needed for the macros */
/* end to check */

#include <linux/types.h>
#include <linux/fs.h>
#include <linux/mm.h>    /* for verify_area */
#include <linux/errno.h> /* for -EBUSY */

#include <asm/io.h>       /* for ioremap */
#include <asm/atomic.h>
#include <asm/system.h>

#include <linux/ioport.h> /* for ..._mem_region */

#include <linux/spinlock.h>

#include <linux/time.h>
#include <linux/delay.h>

#include <asm/irq.h>
#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/interrupt.h>

#include <linux/wait.h>
#include <linux/poll.h>

#include <linux/slab.h>
#include <linux/vmalloc.h>

#include <linux/netdevice.h>    /* ??? SET_MODULE_OWNER ??? */
#include <linux/cdev.h>         /* cdev_alloc */

#include <asm/param.h>
#include <asm/current.h>
#include <asm/uaccess.h>
#include <asm/unistd.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,12)  /* LDDE 2.2 */
#include <linux/mutex.h>
#endif
#include <asm/semaphore.h>

#include "stcommon.h"
#else   /* MODULE */

#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <sys/poll.h>
#include <stdarg.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <semaphore.h>
#endif  /* MODULE */

#include "stddefs.h"
#include "stcommon.h"
#include "linuxcommon.h"

/* Debug */

/* This is *only* okay because we never de-reference it. It'll need to
 * be fixed if we ever do.
 */
#define TIMEOUT_INFINITY    ((void *)NULL)
#define TIMEOUT_IMMEDIATE   ((clock_t *)-1)

/* To be adjusted */
#define MAX_USER_PRIORITY    99
#define MIN_USER_PRIORITY    0

/* Avoiding debug.h... */
#define DEBUG_NOT_CONNECTED 0

/* defining debug functions */
#define debugopen(x,y)				(long int)fileno(fopen(x,y))
#define debugread(x,y,z)			read((int)x,y,z)
#define debugwrite(x,y,z)			write((int)x,y,z)
#define debugclose(x)				close((int)x)

/* Parhaps this should go in a separate debug header file. */
#ifdef MODULE
#ifdef STAPI_ASSERT_ENABLED
#define assert(expr)                                                \
    do {                                                            \
        if(unlikely(!(expr))) {                                     \
            printk(KERN_ERR "Assertion failed! %s,%s,%s,line=%d\n", \
                #expr,__FILE__,__FUNCTION__,__LINE__);              \
        }                                                           \
    } while (0)
#else
#define assert(expr)
#endif
#endif

/* For register access */
#define REGION2 					0xa0000000
#define REGION4 					0xe0000000
#define MASK_STBUS_FROM_ST40		0x1fffffff

/* Linux specific directories */
#define STAPI_DEVICE_DIRECTORY              "/dev/stapi/"       /* Devices directory */
/* Statistics directory (root is /proc directory)
   This did not work the way people were using it: the same directory got created multiple times,
   which does not work right. We can use one subdirectory, but it must be created once.
 */
#define STAPI_STAT_DIRECTORY                (module_name(THIS_MODULE))	/* Module name */

/* Setting Linux paging values*/
#define LINUX_PAGE_SIZE         PAGE_SIZE
#define LINUX_PAGE_ALIGNMENT    PAGE_SIZE


/* ***************** */
/* partial limits.h  */
/* ***************** */

/* Limits constant values */
#if !defined(CHAR_BIT)
#define CHAR_BIT 8
#endif
#if !defined(SCHAR_MAX)
#define SCHAR_MAX 127
#endif
#if !defined(UCHAR_MAX)
#define UCHAR_MAX 255
#endif

#ifdef __SIGNED_CHAR__
#if !defined(CHAR_MIN)
#define CHAR_MIN (-128)
#endif
#if !defined(CHAR_MAX)
#define CHAR_MAX 127
#endif

#else   /* __SIGNED_CHAR__ */
#if !defined(CHAR_MIN)
#define CHAR_MIN 0
#endif
#if !defined(CHAR_MAX)
#define CHAR_MAX 255
#endif

#endif  /* __SIGNED_CHAR__ */

#if !defined(MB_LEN_MAX)
#define MB_LEN_MAX 1
#endif

#if !defined(SHRT_MIN)
#define SHRT_MIN  (~0x7fff)
#endif
#if !defined(SHRT_MAX)
#define SHRT_MAX  0x7fff
#endif
#if !defined(USHRT_MAX)
#define USHRT_MAX 65535U
#endif




/* *************** */
/* Partial sttbx.h */
/* *************** */
#ifndef STTBX_INPUT
#define STTBX_INPUT
#endif

#ifdef STTBX_REPORT
#define STTBX_Report(x)     STTBX_Report_arg x

#ifdef MODULE
#define STTBX_REPORT_NL		printk("\n");
#else
#define STTBX_REPORT_NL		printf("\n");
#endif

#else
#define STTBX_Report(x)
#define STTBX_REPORT_NL
#endif  /* STTBX_REPORT */

#ifdef STTBX_PRINT
#ifdef MODULE
#define STTBX_Print(x)		printk x
#else
#define STTBX_Print(x)		printf x
#endif

#else
#define STTBX_Print(x)
#endif  /* STTBX_PRINT */



/* ************** */



/* *************** */

/* Input functions */
#if !defined(MODULE)
void STTBX_InputChar(char * result);
BOOL STTBX_InputPollChar(char* buffer);
int STTBX_InputStr( char* answer, int size );
#endif  /* !MODULE */

/* Defines the levels of the report (type of message) */
typedef enum STTBX_ReportLevel_e
{                                   /* Use the given level to indicate: */
    STTBX_REPORT_LEVEL_FATAL = 0,   /* Imminent non recoverable system failure */
    STTBX_REPORT_LEVEL_ERROR,       /* Serious error, though recoverable */
    STTBX_REPORT_LEVEL_WARNING,     /* Warnings of unusual occurences */
    STTBX_REPORT_LEVEL_ENTER_LEAVE_FN, /* Entrance or exit of a function, and parameter display */
    STTBX_REPORT_LEVEL_INFO,        /* Status and other information - normal though */
    STTBX_REPORT_LEVEL_USER1,       /* User specific */
    STTBX_REPORT_LEVEL_USER2,       /* User specific */

    /* Keep as last one (Internal use only) */
    STTBX_NB_OF_REPORT_LEVEL        /* Last element has a value corresponding to the number of elements by default ! */
} STTBX_ReportLevel_t;

void STTBX_Report_arg(const STTBX_ReportLevel_t ReportLevel, const char *const Format_p, ...);

#ifndef STTBX_REPORT_LEVEL_MAX
#define STTBX_REPORT_LEVEL_MAX      STTBX_REPORT_LEVEL_ENTER_LEAVE_FN
#endif



/* *************** */
#ifndef PARTITION_T
#define PARTITION_T
typedef int partition_t;
#endif

#ifndef ST_PARTITION_T
#define ST_PARTITION_T
typedef int ST_Partition_t;
#endif

#ifdef MODULE
/* Memory Management */
void*   memory_allocate(partition_t *part, size_t size);
void    memory_deallocate(partition_t *part, void* block);
void*   memory_reallocate (partition_t* Partition, void* Block, size_t Requested, size_t OldSize);
void*   memory_allocate_clear(partition_t* Partition, size_t nelem, size_t elsize);

#else   /* MODULE */

#define memory_allocate(partition, amount)      malloc(amount)
#define memory_deallocate(partition, ptr)       free(ptr)
#define memory_reallocate(partition, ptr, size) realloc(ptr, size)
#define memory_allocate_clear(partition, nelem, size)       calloc(nelem, size)
#endif  /* MODULE */

typedef BOOL boolean;
#define false	FALSE
#define true	TRUE

typedef clock_t osclock_t;

#ifdef MODULE
#ifdef OPTIMIZED_SEMAPHORES
/* Just a Linux semaphore */
typedef struct semaphore semaphore_t;

#else  /* OPTIMIZED_SEMAPHORES */

typedef struct semaphore_s
{
    wait_queue_head_t   wait;
    atomic_t            count;
} semaphore_t;
#endif  /* OPTIMIZED_SEMAPHORES */

#ifdef DO_NOT_SUPPORT_RECURSIVE_MUTEX
typedef struct semaphore mutex_t;

#else   /* DO_NOT_SUPPORT_RECURSIVE_MUTEX */
typedef struct {
	task_t *owner;
	unsigned int count;
	wait_queue_head_t wait;
} mutex_t;

#endif   /* DO_NOT_SUPPORT_RECURSIVE_MUTEX */

#define OS21_SUCCESS     0
#define OS21_FAILURE    -1

#else /* MODULE */

typedef sem_t       semaphore_t;
typedef pthread_mutex_t mutex_t;
typedef pthread_t   task_t;
#endif  /* MODULE */

#define MESSAGE_MEMSIZE_QUEUE(size,nb) ((nb)*(size))

typedef struct
{
    semaphore_t	  * MsgSemaphore_p;
    semaphore_t	  * ClaimSemaphore_p;
    int				Index;
} message_queue_t;


typedef int tdesc_t;            /* Unused */
typedef int task_flags_t;       /* Unused */
typedef int task_context_t;
#define task_context_task       0
#define task_context_interrupt  1


#ifdef MODULE
static DECLARE_MUTEX_LOCKED(thread_trick_sem);
static DECLARE_MUTEX_LOCKED(thread_inj_sem);
static DECLARE_MUTEX_LOCKED(thread_vin_sem);
#endif

#ifdef DEBUG_LOCK

/* interrupt_lock / interrupt_unlock */
#define INT_LOCK_INDEX_MAX          (1024)
#define INT_LOCK_FILE_LENGTH_MAX    (80)

typedef struct {
    clock_t   time;
    U32       location;
    char      file[INT_LOCK_FILE_LENGTH_MAX];
} IntLockVar_t;

extern IntLockVar_t IntLockInVar[];
extern IntLockVar_t IntLockOutVar[];
extern U32 IntLockPrint;

#endif

#ifdef MODULE

#define SPIN_LOCK_DEPTH_MAX 15

extern spinlock_t    SpinIntLock;
extern U32           SpinIntFlagsIndex;
extern unsigned long SpinIntFlagsTab[];

#ifdef DEBUG_LOCK

extern U32 IntLockInIndex;
extern U32 IntLockOutIndex;
extern int IntLockI, IntLockJ;
extern unsigned long SpinIntFlags;
extern pid_t SpinIntPid;
extern int IntLockPrintSignal;

#define interrupt_lock() \
  do { \
    spin_lock_irqsave(&SpinIntLock, SpinIntFlags);\
    if (IntLockInIndex < (INT_LOCK_INDEX_MAX)) \
    { \
        IntLockInVar[IntLockInIndex].time = time_now(); \
        IntLockInVar[IntLockInIndex].location = __LINE__ ;\
        strcpy(IntLockInVar[IntLockInIndex].file, __FILE__);\
        IntLockInIndex++; \
        if (in_interrupt()) \
        {\
            /* We are in interrupt context. 'current' is not valid. Setting SpinIntPid to NULL */ \
        	SpinIntPid = 0; \
        }\
        else \
        { \
    	    SpinIntPid = current->pids->nr; \
    	}\
    } \
    else \
    { \
        /*printk("Max in index reached \n");  */       \
        IntLockPrint = 1; \
    } \
  } while (0)
#else
    #define interrupt_lock()    spin_lock_irqsave(&SpinIntLock, SpinIntFlagsTab[SpinIntFlagsIndex++])
#endif /* DEBUG_LOCK */

#ifdef DEBUG_LOCK
#define interrupt_unlock() \
  do { \
    if (IntLockOutIndex < (INT_LOCK_INDEX_MAX)) \
    { \
        IntLockOutVar[IntLockOutIndex].time = time_now();\
        IntLockOutVar[IntLockOutIndex].location = __LINE__ ;\
        strcpy(IntLockOutVar[IntLockOutIndex].file, __FILE__);\
        IntLockOutIndex++;\
    }\
    else\
    {\
        /*printk("Max out index reached \n");  */\
        IntLockPrint = 1;\
    }\
    spin_unlock_irqrestore(&SpinIntLock, SpinIntFlags);\
    if ((IntLockPrint) && (!IntLockPrintSignal))\
    {\
        printk("Table ready\n");\
        IntLockPrintSignal=1;\
    }\
  } while (0)
#else
   #define interrupt_unlock()       spin_unlock_irqrestore(&SpinIntLock, SpinIntFlagsTab[--SpinIntFlagsIndex])
#endif

#else
    #define interrupt_lock()
    #define interrupt_unlock()
#endif /* MODULE */

/* Semaphores functions */
#ifdef MODULE
#if defined OPTIMIZED_SEMAPHORES && !defined NO_MACRO_FOR_OPTIMIZED_SEMAPHORES
/* We have optimizations for kernel space code */
static inline int semaphore_init_fifo(semaphore_t *sem, int count) {
	sema_init(sem, count);
	return 0;
}
static int inline semaphore_wait(semaphore_t *sem) {
	down(sem);
	return 0;
}
static int inline semaphore_signal(semaphore_t *sem) {
	up(sem);
	return 0;
}
#else
int semaphore_init_fifo(semaphore_t *sem, int count);
int semaphore_wait(semaphore_t *sem);
int semaphore_signal(semaphore_t *sem);
#endif  /* OPTIMIZED_SEMAPHORES */

#else
int semaphore_init_fifo(semaphore_t *sem, int count);
int semaphore_wait(semaphore_t *sem);
int semaphore_signal(semaphore_t *sem);
#endif

int semaphore_wait_timeout(semaphore_t *sem, clock_t *timeout);
int semaphore_delete (semaphore_t* Semaphore);
semaphore_t * semaphore_create_fifo_timeout(int count);

/* These are not aliases because creating an alias to a function
 * means it won't be inlined, just localized. */
#define semaphore_init_fifo_timeout(s,c)    semaphore_init_fifo(s,c)
#define semaphore_init_priority(s,c)        semaphore_init_fifo(s,c)
#define semaphore_create_fifo(c)            semaphore_create_fifo_timeout(c)
#define semaphore_create_priority(c)        semaphore_create_fifo_timeout(c)

/* mutex recursive support */

mutex_t * mutex_create_fifo(void);
int mutex_init_fifo(mutex_t *mutex, int count);
int mutex_lockit(mutex_t *mutex);
int mutex_release(mutex_t *mutex);
int mutex_delete(mutex_t *mutex);
#define mutex_create_priority() mutex_create_fifo()


/* Race condition functions */
#ifdef MODULE
/* Already defined in <linux/sched.h> */
#else
void task_lock(void);
void task_unlock(void);
#endif  /* !MODULE */


/* Time functions */
#ifdef MODULE
    /* Jiffies */
#define STLINUX_GetClocksPerSecond()      (HZ)
#else
    /* In user mode, it is up to us to define a base here. Let's set it to ms */
#define STLINUX_GetClocksPerSecond()      (1000)
#endif

#define MILLISECOND_TO_TICKS(v)          (((unsigned int) STLINUX_GetClocksPerSecond()*((unsigned int)(v)))/1000)

int task_delay(int ticks);
#if defined MODULE
int task_delay_until(clock_t timeout);
#endif

/* Context functions */
task_context_t task_context(task_t **task, int* level);


/* Messages functions */

void message_init_queue_timeout(message_queue_t* MessageQueue,
                                void* memory, size_t ElementSize,
                                unsigned int NoElements);
void message_init_queue(message_queue_t* MessageQueue,
                        void* memory, size_t ElementSize,
                        unsigned int NoElements);
message_queue_t * message_create_queue_timeout(size_t ElementSize, unsigned int NoElements);
void message_delete_queue(message_queue_t* MessageQueue);
int message_claim_timeout(message_queue_t *queue, clock_t * time_end_p);
int message_claim(message_queue_t * MessageQueue);
void message_release (message_queue_t* MessageQueue, void* Message);
int message_send(message_queue_t *queue, void *message);
void *message_receive (message_queue_t* MessageQueue);
void *message_receive_timeout (message_queue_t* MessageQueue, clock_t * ticks);

/* Clock functions */
clock_t      time_now (void);
clock_t      time_minus (clock_t Time1, clock_t Time2);
clock_t      time_plus (clock_t Time1, clock_t Time2);

#if defined(MODULE)
int          time_after_STAPI (clock_t Time1, clock_t Time2);

#else

struct timeval  Clockt2Timeval(clock_t clock);
clock_t         get_time_convert(struct timeval *tv);
clock_t		 get_time_now (void);
int          time_after (clock_t Time1, clock_t Time2);
void         task_delay_until (unsigned int ExitTime);
#endif

/* STSYS functions */

#ifdef MODULE
U16 STSYS_ReadRegDev16BE(U32 Address_p);
U32 STSYS_ReadRegDev24BE(U32 Address_p);
U32 STSYS_ReadRegDev32BE(U32 Address_p);

#define STSYS_ReadRegDev8(Address_p)        readb(Address_p)
#define STSYS_ReadRegMem8(Address_p)        readb(Address_p)
#define STSYS_ReadRegDev16LE(address)       readw(address)
#define STSYS_ReadRegMem16LE(Address_p)     readw(Address_p)
#define STSYS_ReadRegMem16BE(Address_p)     STSYS_ReadRegDev16BE(Address_p)
#define STSYS_ReadRegDev24LE(Address_p)     ((U32) ((readb((U32)(Address_p) + 2)) << 16 | (readw(Address_p))))
#define STSYS_ReadRegMem24LE(Address_p)     STSYS_ReadRegDev24LE(Address_p)
#define STSYS_ReadRegMem24BE(Address_p)     STSYS_ReadRegDev24BE(Address_p)
#define STSYS_ReadRegDev32LE(address)       readl(address)
#define STSYS_ReadRegMem32LE(address)       readl(address)
#define STSYS_ReadRegMem32BE(Address_p)     STSYS_ReadRegDev32BE(Address_p)

#define STSYS_WriteRegDev8(Address_p, value)        writeb((u8)value, (void*)Address_p)
#define STSYS_WriteRegMem8(Address, value)          writeb((u8)value, (u32)Address)
#define STSYS_WriteRegDev16LE(Address_p, value)     writew((u16)value, (void*)Address_p)
#define STSYS_WriteRegMem16LE(Address_p, value)     writew((u16)value, (void*)Address_p)
#define STSYS_WriteRegDev32LE(Address_p, value)     writel((u32)value, (void*)Address_p)
#define STSYS_WriteRegMem32LE(Address, value)       writel((u32)value, (u32)Address)

#define STSYS_WriteRegDev16BE(Address_p, Value)                     \
    do {   U16 Reg16BEToLE =  (U16) ((((Value) & 0xFF00) >> 8) |    \
                                     (((Value) & 0x00FF) << 8));    \
           writew(Reg16BEToLE, (void*)Address_p);                   \
    } while (0)

#define STSYS_WriteRegDev24BE(Address_p, Value)                     \
    do {   U16 Reg24BEToLE = (U16) ((((Value) & 0xFF0000) >> 16) |  \
                                    (((Value) & 0x00FF00)      ));  \
            writew(Reg24BEToLE, (void*)Address_p);                  \
            writeb((U8)(Value), (void*)((U32)(Address_p) + 2));     \
    } while (0)

#define STSYS_WriteRegDev24LE(Address_p, Value)                                             \
    do {    writew((U16)(Value), (void*)Address_p);                                         \
            writeb((U8)(((Value) & 0xFF0000) >> 16), (void*)((U32)(Address_p) + 2));        \
    } while (0)

#define STSYS_WriteRegDev32BE(Address_p, Value)                         \
    do {    U32 Reg32BEToLE = (U32) ((((Value) & 0xFF000000) >> 24) |   \
                                     (((Value) & 0x00FF0000) >>  8) |   \
                                     (((Value) & 0x0000FF00) <<  8) |   \
                                     (((Value) & 0x000000FF) << 24));   \
            writel(Reg32BEToLE, (void*)Address_p);                      \
    } while (0)

#define STSYS_WriteRegMem16BE(Address_p, Value)     STSYS_WriteRegDev16BE(Address_p, Value)
#define STSYS_WriteRegMem24BE(Address_p, Value)     STSYS_WriteRegDev24BE(Address_p, Value)
#define STSYS_WriteRegMem24LE(Address_p, Value)     STSYS_WriteRegDev24LE(Address_p, Value)
#define STSYS_WriteRegMem32BE(Address_p, Value)     STSYS_WriteRegDev32BE(Address_p, Value)


#define STSYS_SetRegMask32LE(address, mask) \
    STSYS_WriteRegDev32LE(address, STSYS_ReadRegDev32LE(address) | (mask) )

#define STSYS_ClearRegMask32LE(address, mask) \
    STSYS_WriteRegDev32LE(address, STSYS_ReadRegDev32LE(address) & ~(mask) )

#define STSYS_memcpy_fromio(dest, source, num)    \
do {   \
    memcpy_fromio((dest), (u32)(source), (num));    \
    rmb();                               \
} while(0)

#define STSYS_ReadRegMemUncached8(Address_p)             STSYS_ReadRegDev8((void *)(((U32)(Address_p) & MASK_STBUS_FROM_ST40) | REGION2))
#define STSYS_WriteRegMemUncached8(Address_p, Value)     STSYS_WriteRegDev8((void *)(((U32)(Address_p) & MASK_STBUS_FROM_ST40) | REGION2), (Value))
#define STSYS_ReadRegMemUncached16LE(Address_p)          STSYS_ReadRegDev16LE((void *)(((U32)(Address_p) & MASK_STBUS_FROM_ST40) | REGION2))
#define STSYS_WriteRegMemUncached16LE(Address_p, Value)  STSYS_WriteRegDev16LE((void *)(((U32)(Address_p) & MASK_STBUS_FROM_ST40) | REGION2), (Value))
#define STSYS_ReadRegMemUncached16BE(Address_p)          STSYS_ReadRegDev16BE((void *)(((U32)(Address_p) & MASK_STBUS_FROM_ST40) | REGION2))
#define STSYS_WriteRegMemUncached16BE(Address_p, Value)  STSYS_WriteRegDev16BE((void *)(((U32)(Address_p) & MASK_STBUS_FROM_ST40) | REGION2), (Value))
#define STSYS_ReadRegMemUncached32LE(Address_p)          STSYS_ReadRegDev32LE((void *)(((U32)(Address_p) & MASK_STBUS_FROM_ST40) | REGION2))
#define STSYS_WriteRegMemUncached32LE(Address_p, Value)  STSYS_WriteRegDev32LE((void *)(((U32)(Address_p) & MASK_STBUS_FROM_ST40) | REGION2), (Value))
#define STSYS_ReadRegMemUncached32BE(Address_p)          STSYS_ReadRegDev32BE((void *)(((U32)(Address_p) & MASK_STBUS_FROM_ST40) | REGION2))
#define STSYS_WriteRegMemUncached32BE(Address_p, Value)  STSYS_WriteRegDev32BE((void *)(((U32)(Address_p) & MASK_STBUS_FROM_ST40) | REGION2), (Value))

#endif

/**********************************************************/
/*                  Kernel Functions                      */
/**********************************************************/
#ifdef MODULE
int STLINUX_sched_setscheduler(pid_t pid, int policy, const struct sched_param *p);
#endif

/**********************************************************/
/*                  STLINUX_task                          */
/**********************************************************/

#ifdef MODULE
ST_ErrorCode_t  STLINUX_TaskCreate (void (*Function)(void* Param),
                                    void* Param,
                                    struct semaphore * sem_p,
                                    int     Priority,
                                    BOOL    RealTime,
                                    const char* Name);
void STLINUX_TaskAskEnd (BOOL * EndFlag_p, semaphore_t * sem_p);
void STLINUX_TaskWaitEnd (struct semaphore * sem_p);
void STLINUX_TaskStartUp (char * const ThreadName);
void STLINUX_TaskEndUp (struct semaphore * sem_p);
#endif

/**********************************************************/
/*                 DEVICES REGISTRATION                   */
/**********************************************************/
#ifdef MODULE

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,12)
#define class_simple                                        class
#define class_simple_create(a,b)                            ((struct class *)1)
#define class_simple_destroy(a)
#define class_simple_device_add(a,b,c,d_args...)            ((struct class_device *)1)
#define class_simple_device_remove(a)
#define io_remap_page_range(vma, vaddr, pfn, size, prot)    io_remap_pfn_range(vma, vaddr, (pfn) >> PAGE_SHIFT, size, prot)
#endif

int STLINUX_DeviceRegister(struct file_operations *stdevice_fops,
                         U32                     nbdevices,
                         char                   *DeviceName,
                         unsigned int           *DeviceMajor_p,
                         struct cdev            **stdevice_cdev,
                         struct class_simple    **stdevice_class);
int STLINUX_DeviceUnregister(U32                     nbdevices,
                           char                   *DeviceName,
                           unsigned int            DeviceMajor,
                           struct cdev            *stdevice_cdev,
                           struct class_simple    *stdevice_class);
#endif  /* MODULE */



/* *****************  Random procedure ***************** */
#ifdef MODULE
unsigned long rand(void);
#endif




/**********************************************************/
/*                   REGISTERS MAPPING                    */
/**********************************************************/
#ifdef MODULE
void * STLINUX_MapRegion(void * Address_p, U32 Width, char * RegionName);
void   STLINUX_UnmapRegion(void * MappedAddress_p, U32 Width);
int    STLINUX_MMapMethod(struct file *filp, struct vm_area_struct *vma);
#endif

#endif  /* __H_LINUXWRAPPER */


/* End of File */
