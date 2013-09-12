/*****************************************************************************

File name   : bltswcfg.h

COPYRIGHT (C) STMicroelectronics 1999.


****************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __BLTSWCFG_H
#define __BLTSWCFG_H


/* Includes --------------------------------------------------------------- */

#ifdef __cplusplus
extern "C" {
#endif


/* Exported Constants ----------------------------------------------------- */

/* Message queue related */
#ifndef STBLIT_MAX_NUMBER_MESSAGE
#define STBLIT_MAX_NUMBER_MESSAGE  20            /* To be tuned */
#endif

/* Task Stack size related */
#ifndef STBLIT_MASTER_TASK_STACK_SIZE
    #if defined(ST_OS21) || defined(ST_OSLINUX) || defined(ST_OSWINCE)                                                /* OS21 accepts 16K stack. */
		#define STBLIT_MASTER_TASK_STACK_SIZE     16 *1024
    #endif
    #ifdef ST_OS20
		#define STBLIT_MASTER_TASK_STACK_SIZE     5 *1024
    #endif
#endif

#ifndef STBLIT_SLAVE_TASK_STACK_SIZE
    #if defined(ST_OS21) || defined(ST_OSLINUX) || defined(ST_OSWINCE)                                                /* OS21 accepts 16K stack. */
		#define STBLIT_SLAVE_TASK_STACK_SIZE      16 *1024
    #endif
    #ifdef ST_OS20
		#define STBLIT_SLAVE_TASK_STACK_SIZE      5 *1024
    #endif
#endif

#ifndef STBLIT_INTERRUPT_PROCESS_TASK_STACK_SIZE
    #if defined(ST_OS21) || defined(ST_OSLINUX) || defined(ST_OSWINCE)                                                /* OS21 accepts 16K stack. */
		#define STBLIT_INTERRUPT_PROCESS_TASK_STACK_SIZE      16 *1024
    #endif
    #ifdef ST_OS20
		#define STBLIT_INTERRUPT_PROCESS_TASK_STACK_SIZE      5 *1024
    #endif
#endif

#ifndef STBLIT_BLITTER_CRASH_CHECK_PROCESS_TASK_STACK_SIZE
    #if defined(ST_OS21) || defined(ST_OSLINUX) || defined(ST_OSWINCE)                                                /* OS21 accepts 16K stack. */
        #define STBLIT_BLITTER_CRASH_CHECK_PROCESS_TASK_STACK_SIZE      16 *1024
    #endif
    #ifdef ST_OS20
        #define STBLIT_BLITTER_CRASH_CHECK_PROCESS_TASK_STACK_SIZE      5 *1024
    #endif
#endif


/* Task priority related */
#ifndef STBLIT_MASTER_TASK_STACK_PRIORITY
#ifdef ST_OSLINUX
#define STBLIT_MASTER_TASK_STACK_PRIORITY STBLIT_MASTER_THREAD_PRIORITY
#else
#define STBLIT_MASTER_TASK_STACK_PRIORITY (5)
#endif
#endif

#ifndef STBLIT_SLAVE_TASK_STACK_PRIORITY
#ifdef ST_OSLINUX
#define STBLIT_SLAVE_TASK_STACK_PRIORITY STBLIT_SLAVE_THREAD_PRIORITY
#else
#define STBLIT_SLAVE_TASK_STACK_PRIORITY  (5)
#endif
#endif

#ifndef STBLIT_INTERRUPT_PROCESS_TASK_STACK_PRIORITY
#ifdef ST_OSLINUX
#define STBLIT_INTERRUPT_PROCESS_TASK_STACK_PRIORITY STBLIT_INTERRUPT_THREAD_PRIORITY
#else
#define STBLIT_INTERRUPT_PROCESS_TASK_STACK_PRIORITY  (10)
#endif
#endif


/* S/W Emulator tasks related */
#ifdef STBLIT_EMULATOR

#ifndef STBLIT_EMULATOR_TASK_STACK_PRIORITY
#define STBLIT_EMULATOR_TASK_STACK_PRIORITY (5)
#endif

#ifndef STBLIT_EMULATOR_TASK_STACK_SIZE
        #if defined(ST_OS21) || defined(ST_OSLINUX) || defined(ST_OSWINCE)                                                /* OS21 accepts 16K stack. */
			#define STBLIT_EMULATOR_TASK_STACK_SIZE      16 * 1024  /* To be tuned */
        #endif
        #ifdef ST_OS20
			#define STBLIT_EMULATOR_TASK_STACK_SIZE      5 * 1024  /* To be tuned */
        #endif
#endif

#ifndef STBLIT_EMULATOR_ISR_TASK_STACK_SIZE
    #if defined(ST_OS21) || defined(ST_OSLINUX) || defined(ST_OSWINCE)                                                /* OS21 accepts 16K stack. */
		#define STBLIT_EMULATOR_ISR_TASK_STACK_SIZE  16 * 1024  /* To be tuned */
    #endif
    #ifdef ST_OS20
		#define STBLIT_EMULATOR_ISR_TASK_STACK_SIZE  5 * 1024  /* To be tuned */
    #endif
#endif

#endif


#ifdef ST_OSLINUX
#define BLITMASTER_NAME KERNEL_THREAD_BLITMASTER_NAME
#else
#define BLITMASTER_NAME "STBLIT_ProcessHighPriorityQueue"
#endif

#ifdef ST_OSLINUX
#define BLITSLAVE_NAME KERNEL_THREAD_BLITSLAVE_NAME
#else
#define BLITSLAVE_NAME "STBLIT_ProcessLowPriorityQueue"
#endif

#ifdef ST_OSLINUX
#define BLITINTERRUPT_NAME KERNEL_THREAD_BLITINTERRUPT_NAME
#else
#define BLITINTERRUPT_NAME "STBLIT_InterruptProcess"
#endif




/* Exported Types --------------------------------------------------------- */


/* Exported Variables ----------------------------------------------------- */


/* Exported Macros -------------------------------------------------------- */


/* Exported Functions ----------------------------------------------------- */


#ifdef __cplusplus
}
#endif

#endif /* #ifndef __BLTSWCFG_H */

/* End of bltswcfg.h */






