/*
 * harness_linux.c
 *
 * Copyright (C) STMicroelectronics Limited 2003. All rights reserved.
 *
 * 
 */

#if defined __LINUX__

#ifdef __KERNEL__
#include <linux/module.h>
#include <linux/version.h>
#include <linux/config.h>
#include <linux/init.h>
#include <linux/stddef.h>
#include <linux/sched.h>
#include <linux/param.h>
#include <linux/smp_lock.h>
#include <linux/proc_fs.h>
#include <linux/wait.h>
#include <asm/semaphore.h>
#else
/* while we don't have platform.h */
#include <semaphore.h>
#include <unistd.h>
#endif

#include "harness.h"

#if !defined(__LINUX__) && !defined(__KERNEL__)
/* forcibly include the non-RPC version of assert */
#include <assert.h>
#endif

void rpcServerInit(void);
int rpcStubsInit(void *);

static void harness_initialize(void);

#ifdef MEDIAREF
char *rpcTransportName = "shm_mediaref";
#else
char *rpcTransportName = "shm";
#endif

#if defined __LINUX__ && defined __KERNEL__
int main(char *argv[], int argc);

int harness_read_proc(char *buf, char **start, off_t offset,
		       int count, int *eof, void *data)
{
	int res, len;

	/* run the test */
	printk("******** Booting "MACHINE" ********\n");
	res = main(NULL, 0);

	/* return the result to be printed in user mode */
	len = sprintf(buf, "******** Test returned %d ********\n", res);
	*eof = 1;
	return len;
}

int __init harness_init(void)
{
	struct proc_dir_entry *pde;
	
	/* the test will run when user space reads /proc/rpctest */
	pde = create_proc_read_entry("rpctest",
		0 /* default mode */,
		NULL /* parent dir */,
		harness_read_proc,
		NULL /* client data */);

	return (pde ? 0 : -ENOMEM);
}

void __exit harness_deinit(void)
{
	/* TODO: the stubs deinit is dubiously reliable */
	rpcStubsDeinit();
	printk("******** Unloaded test module ********\n");
}

module_init(harness_init);
module_exit(harness_deinit);
#endif

void harness_boot(void)
{
	int err;

#if defined __LINUX__ && !defined __KERNEL__
	printf("******** Booting "MACHINE" ********\n");
#endif

	/* kernel is alread booted */

	/* initialize any board specific components and register transports */
	harness_initialize();

#ifdef __STRPC__
	/* boot the RPC server and initialize the stubs */
	err = rpcStubsInit(NULL);
	assert(0 == err);
#else
#error not yet ported
#endif

	/* ensure errno is set to zero before the tests start */
	errno = 0;

	printf("******** %s ready for RPC ********\n", CPU);
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

#ifdef CONF_MASTER
void harness_assertfail(in.string(127) char * e, in.string(63) char * f, int l)
{
#ifdef __KERNEL__
	panic("ERROR: assertion failure %s at line %d of %s\n", e, l, f);
#else
	__assert(f, l, e);
#endif
}
#endif /* CONF_MASTER */

char *harness_getTransportName(void)
{
        return rpcTransportName;
}

void harness_sleep(int seconds)
{
#ifdef __KERNEL__
	set_current_state(TASK_INTERRUPTIBLE); 
	schedule_timeout (seconds*HZ);
#else
	sleep(seconds);
#endif
}

static void harness_initialize(void)
{
}

void harness_sync(void)
{
#ifdef __KERNEL__
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
	sync_dev(0);
#else /* 2.6.x */
	printf(MACHINE "cannot sync on 2.6.x\n");
#endif /* LINUX_VERSION_CODE */
#else
	sync();
#endif
}

#else /* __LINUX__ */

extern void warning_suppression(void);

#endif /* __LINUX__ */
