/*
 * os_abstractions.h
 *
 * Copyright (C) STMicroelectronics Limited 2001. All rights reserved.
 *
 * Types and functions to provide an OS independant view of multi-thread
 * issues.
 */

#ifndef _STRPC_OS_ABSTRACTIONS_H_
#define _STRPC_OS_ABSTRACTIONS_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
 * Initialization
 *
 * Perform any initialization needed for the abstraction layer
 *
 * Provides:
 *   OS_INIT
 *   OS_DEINIT
 */

#if defined OS20_OPTIMIZATIONS

void os_init(void);
void os_deinit(void);
#define OS_INIT   os_init
#define OS_DEINIT os_deinit

#else

#define OS_INIT()
#define OS_DEINIT()

#endif

/*
 * OS_MUTEX
 *
 * A simple mutex type that cannot (necessarily) be taken more than once. 
 *
 * Provides:
 *   OS_MUTEX_INIT    - Initialize the mutex
 *   OS_MUTEX_DESTROY - Uninitialize the mutex and return any memory used
 *   OS_MUTEX_TAKE    - Obtain/lock the mutex
 *   OS_MUTEX_RELEASE - Release/unlock the mutex
 */

#if defined __OS20__

#include <semaphor.h>

typedef semaphore_t OS_MUTEX;

#define OS_MUTEX_INIT(pMutex)   	semaphore_init_fifo((pMutex), 1)
#define OS_MUTEX_DESTROY(pMutex)	semaphore_delete((pMutex))
#define OS_MUTEX_TAKE(pMutex)		semaphore_wait((pMutex))
#define OS_MUTEX_RELEASE(pMutex)	semaphore_signal((pMutex))

#elif defined __OS21__

#include <os21/semaphore.h>

typedef semaphore_t *OS_MUTEX;

#define OS_MUTEX_INIT(pMutex)		*(pMutex) = semaphore_create_fifo(1)
#define OS_MUTEX_DESTROY(pMutex)	semaphore_delete(*(pMutex));
#define OS_MUTEX_TAKE(pMutex)		semaphore_wait(*(pMutex));
#define OS_MUTEX_RELEASE(pMutex)	semaphore_signal(*(pMutex));

#elif defined __LINUX__ && defined __KERNEL__

/* Linux kernel space OS abstractions */

#include <linux/module.h>
#include <linux/version.h>
#include <linux/config.h>
#include <linux/stddef.h>
#include <linux/wait.h>
#include <asm/semaphore.h>

typedef struct semaphore OS_MUTEX;

#define OS_MUTEX_INIT(pMutex)		sema_init((pMutex), 1)
#define OS_MUTEX_DESTROY(pMutex)
#define OS_MUTEX_TAKE(pMutex)		down_interruptible((pMutex))
#define OS_MUTEX_RELEASE(pMutex)	up((pMutex))

#elif defined __LINUX__

/* Linux user space OS abstractions */

#include <semaphore.h>

typedef sem_t OS_MUTEX;

#define OS_MUTEX_INIT(pMutex)		sem_init((pMutex), 0, 1)
#define OS_MUTEX_DESTROY(pMutex)	sem_destroy((pMutex));
#define OS_MUTEX_TAKE(pMutex)		sem_wait((pMutex))
#define OS_MUTEX_RELEASE(pMutex)	sem_post((pMutex))

#elif defined __WINCE__

#include <windows.h>

typedef HANDLE OS_MUTEX;

#define OS_MUTEX_INIT(pMutex)		*(pMutex) = CreateSemaphore(NULL, 1, 0xffff, NULL)
#define OS_MUTEX_DESTROY(pMutex)	CloseHandle(*(pMutex));
#define OS_MUTEX_TAKE(pMutex)		WaitForSingleObject(*(pMutex), INFINITE)
#define OS_MUTEX_RELEASE(pMutex)	ReleaseSemaphore(*(pMutex), 1, NULL)

#else
#error OS_MUTEX not yet ported to this operating system
#endif


/*
 * OS_SIGNAL
 *
 * A simple signal type that provides a means for a task to block whilst
 * waiting for a particular event.
 *
 * Provides:
 *   OS_SIGNAL_INIT    - Initialize the signal (can appear in critical paths)
 *   OS_SIGNAL_DESTROY - Uninitialize the mutex and return any memory used
 *   OS_SIGNAL_WAIT    - Wait to the signal to be posted
 *   OS_SIGNAL_POST    - Wave the flag, signal the signal
 */

#if defined OS20_OPTIMIZATIONS

/* An implementation partially based on OS_MUTEX that avoids OS_MUTEX_INIT
 * for every OS_SIGNAL_INIT.
 *
 * semaphore_init is very expensive under OS20 (150us) and OS_SIGNAL_INIT
 * appears in time critical code (but OS_MUTEX_INIT does not).
 */

typedef struct OS_SIGNAL {
	OS_MUTEX mutex;
	struct OS_SIGNAL *next;
} *OS_SIGNAL;

void os_signal_init(OS_SIGNAL * pSignal);
void os_signal_destroy(OS_SIGNAL * pSignal);
#define OS_SIGNAL_INIT			os_signal_init
#define OS_SIGNAL_DESTROY		os_signal_destroy
#define OS_SIGNAL_WAIT(pSignal)		OS_MUTEX_TAKE(&((*(pSignal))->mutex))
#define OS_SIGNAL_POST(pSignal)		OS_MUTEX_RELEASE(&((*(pSignal))->mutex))

#else

/* A default implementation based on OS_MUTEX. This only works if OS_MUTEX
 * cannot be taken twice by the same process.
 */

typedef OS_MUTEX OS_SIGNAL;
#define OS_SIGNAL_INIT(pSignal) \
	{ \
		OS_MUTEX_INIT((pSignal)); \
		OS_MUTEX_TAKE((pSignal)); \
	}
#define OS_SIGNAL_DESTROY(pSignal)	OS_MUTEX_DESTROY((pSignal))
#define OS_SIGNAL_WAIT(pSignal)		OS_MUTEX_TAKE((pSignal))
#define OS_SIGNAL_POST(pSignal)		OS_MUTEX_RELEASE((pSignal))

#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _STRPC_OS_ABSTRACTIONS_H_ */
