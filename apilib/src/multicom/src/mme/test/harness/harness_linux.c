/*
 * harness_linux.c
 *
 * Copyright (C) STMicroelectronics Limited 2002. All rights reserved.
 *
 * 
 */

#if defined(__LINUX__)

#ifdef __KERNEL__
#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/smp_lock.h>
#include <linux/proc_fs.h>
#else
#include <semaphore.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/time.h>

#include <asm/unistd.h>
#include <linux/reboot.h>
#endif

#include "harness.h"
#include "harness_linux.h"
#include "mme.h"

#ifdef MEDIAREF
char *rpcTransportName = "empi_mediaref";
#endif

#if defined __LINUX__ && defined __KERNEL__

MODULE_LICENSE("GPL");

/*
 * This slightly esoteric means to run main() is designed to ensure that
 * insmod is able to display address information to be used to allow
 * this modules to be debuged with sh4gdb or kgdb *before* anything
 * is likely to go wrong.
 */
int main(char *argv[], int argc);

int harness_read_proc(char *buf, char **start, off_t offset,
		       int count, int *eof, void *data)
{
	int res, len;

	/* run the test */
	printk("******** Running main on "MACHINE" ********\n");
	res = main(NULL, 0);

	/* return the result to be printed in user mode */
	len = sprintf(buf, "******** Test returned %d ********\n", res);
	*eof = 1;
	return len;
}

int __init harness_init(void)
{
	struct proc_dir_entry *pde;
	int res;
	
	printk("******** Loading test module ********\n");

	/* the test will run when user space reads /proc/rpctest */
	pde = create_proc_read_entry("rpctest",
		0 /* default mode */,
		NULL /* parent dir */,
		harness_read_proc,
		NULL /* client data */);

	res = (pde ? 0 : -ENOMEM);
	printk("******** Load returned %d ********\n", res);
	return res;
}

void __exit harness_deinit(void)
{
	/* TODO: should deregister create_proc_read_entry() */
	printk("******** Unloaded test module ********\n");
}

module_init(harness_init);
module_exit(harness_deinit);
#endif

void harness_boot(void)
{
	printf("******** Booting "MACHINE" ********\n");

#if defined __KERNEL__
	/* MME is already initialized by the test suites assume that this is not the case */
	MME(Term());
#else
	/* workaround to gdb bug */
	{ volatile float f=2.3; f+=2.4; }
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

unsigned int harness_getTicksPerSecond(void)
{
#ifdef __KERNEL__
        return HZ;
#else
	return 1000000;
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
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_usec + 1000000 * tv.tv_sec;
#endif
}

void harness_sleep(int seconds)
{
#ifdef __KERNEL__
        set_current_state(TASK_INTERRUPTIBLE);
        schedule_timeout(seconds*HZ);
#else
	sleep(seconds);
#endif
}


char *harness_getTransportName(void)
{
	return "shm";

}

void harness_exit(int code, char* str, ...)
{
	static char buf[256];
	va_list args;

	/* generate the assertion string if required */
	if (str) {
		va_start(args, str);
		vsnprintf(buf, sizeof(buf), str, args);
		printf("%sERROR: lastError is %d\n", buf, lastError);
		va_end(args);
	}

#ifdef NOTERMINATE
	return 1;
#endif

#ifdef __KERNEL__
	harness_sync();
	lock_kernel();
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,12)
	machine_restart(str ? buf : 0);
#else
	if (str) {
		printf(str ? buf : "SUCCESS");
		while (1) {
			harness_sleep(1);
			/* we can no longer report failure properly ... just deadlock on failure */
		}
	} else
		kernel_halt();
#endif

#else

	/* use rmmod to force termination of the actual MME implementation */
	if (code || 0 == system("/sbin/rmmod mme_host.ko")) {
		harness_sleep(2);

		/* now perform the shutdown */
		harness_sync();
		if (0 != syscall(__NR_reboot, LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2,
                                 (code ? LINUX_REBOOT_CMD_RESTART2 : LINUX_REBOOT_CMD_RESTART),
			         (code ? buf : 0))) {
			printf("ERROR: failed to issue reboot command\n");
		 }
	} else {
		/* if we cannot remove the module there are either outstanding
		 * mmaps or the /dev/mme file handle is still open
		 */
		harness_exit(1, "ERROR: cannot uninstall Linux kernel module (mmap leak?)\n");
	}
#endif
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

#if ! defined(__KERNEL__) && ! defined (HAS_DEINIT)
void harness_deinit(void) {}
#endif

#else /* __LINUX__ */

extern void warning_suppression(void);

#endif /* __LINUX__ */
