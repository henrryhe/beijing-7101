/*
 * harness_linux.c
 *
 * Copyright (C) STMicroelectronics Limited 2002. All rights reserved.
 *
 * 
 */

#if defined(__LINUX__)

#ifdef __KERNEL__
#include <linux/sched.h>
#else
#include <semaphore.h>
#include <unistd.h>
#include <stdarg.h>
#endif

#include "harness.h"
#include "harness_linux.h"


/* force the non-RPC version of assert */
#include <assert.h>

/* while we don't have platform.h */
void rpcServerInit(void);
int rpcStubsInit(void *);

#ifdef MEDIAREF
char *rpcTransportName = "empi_mediaref";
#endif

void harness_boot(void)
{
#ifdef __KERNEL__
	printf("******** Booting %s/%s ********\n", OS, CPU);

	/* kernel is alread booted */
#endif

#if 0
	/* initialize any board specific components and register transports */
	harness_initialize();

	/* boot the RPC server and initialize the stubs */
	err = rpcStubsInit(NULL);
	assert(0 == err);

#endif
}

void harness_block(void)
{
#ifdef __KERNEL__
	DECLARE_MUTEX_LOCKED(sem);
	down_interruptible(&sem);
#else
	sem_t sem;
	sem_init(&sem, 0, 0);
	sem_wait(&sem);
#endif
}

#if 0
#ifdef MASTER
void harness_assertfail(in.string(127) char * e, in.string(63) char * f, int l)
{
	__assert(f, l, e);
}
#endif /* MASTER */
#endif


#if 0
#ifdef MASTER
void harness_passed()
{
	int res;
	pthread_t t;

	printf("******** Passed ********\n");

	res = pthread_create(&t, NULL, 
	                     (void(*)(void*)) harness_remoteshutdown, NULL);
	if (res < 0) {
		printf("WARNING - cannot close down remote side\n");
	}

	sleep(2);
	exit(0);
}
#endif

#ifdef SLAVE
void harness_remoteshutdown(int e)
{
	exit(e);
}
#endif
#endif

unsigned int harness_getTicksPerSecond(void)
{
#ifdef __KERNEL__
        return HZ;
#else
#endif
}

/* use the high priority timer since it has a finer resolution */
unsigned int harness_getTime(void)
{
	/* converts a 64-bit to 32-bit type but this is OK because
	 * the tests will not run for long enough for this to roll
	 * over
	 */
#ifdef __KERNEL__
	return jiffies;
#else
#endif
}

void harness_sleep(int seconds)
{
#ifdef __KERNEL__
        set_current_state(TASK_INTERRUPTIBLE);
        schedule_timeout(seconds*HZ);
#else
#endif
}


char *harness_getTransportName(void)
{
	return "shm";
}

void harness_exit(int code, char* str, ...)
{
#ifdef __KERNEL__
#else
        exit(code);
#endif
}

#else /* __LINUX__ */

extern void warning_suppression(void);

#endif /* __LINUX__ */
