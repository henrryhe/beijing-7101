/*
 * harness/harness.c
 *
 * Copyright (C) STMicroelectronics Limited 2003. All rights reserved.
 *
 * 
 */

#if defined __OS20__
#include <cache.h>
#include <interrup.h>
#include <task.h>
#include <ostime.h>
#endif

#if defined __OS21__
#include <os21.h>
#if defined __SH4__
#include <os21/st40.h>
#endif
#endif

#if defined __LINUX__ && !defined(__KERNEL__)
#ifndef DISABLE_EMBX_CALLS
#define DISABLE_EMBX_CALLS
#endif
#include <sys/reboot.h>
#include <unistd.h>
#endif

#include <stdarg.h>
#include "harness.h"

/* make a (conservative) guess about how long it tasks to stop using EMBX */
#define SHUTDOWN_TIME 3

int lastError;

#ifdef CONF_MASTER

void harness_passed()
{
	harness_sendShutdown();

	harness_sleep(2*SHUTDOWN_TIME);
	printf("******** "MACHINE" about to rip down RPC ********\n");
	rpcStubsDeinit();
	printf("******** "MACHINE" has ripped down RPC ********\n");
#ifndef DISABLE_EMBX_CALLS
	EMBX(Deinit());
#else
	harness_sleep(SHUTDOWN_TIME);
#endif

	printf("******** Passed ********\n");

	/* send halt signal to init */
#if defined __LINUX__ && !defined(DISABLE_HALT)
#if defined(__KERNEL__)
	harness_sync();
	lock_kernel();
	machine_restart(0);
#else	
	reboot(RB_HALT_SYSTEM);
#endif
#endif

	exit(0);
}

#else /* not CONF_MASTER */

volatile int receivedShutdownRequest = 0;

void harness_sendShutdown()
{
	receivedShutdownRequest = 1;
}
	
void harness_waitForShutdown()
{
	printf("******** "MACHINE" waiting for shutdown request ********\n");
	while (!receivedShutdownRequest) {
		harness_sleep(1);
	}

	harness_sleep(SHUTDOWN_TIME);
	printf("******** "MACHINE" about to rip down RPC ********\n");
	rpcStubsDeinit();
	printf("******** "MACHINE" has ripped down RPC ********\n");
#ifndef DISABLE_EMBX_CALLS
	EMBX(Deinit());
#else
	harness_sleep(SHUTDOWN_TIME);
#endif

	printf("******** "MACHINE" shutdown OK ********\n");

	/* send halt signal to init */
#if defined __LINUX__ && !defined(DISABLE_HALT)
#if defined(__KERNEL__)
	harness_sync();
	lock_kernel();
	machine_restart(0);
#else	
	reboot(RB_HALT_SYSTEM);
#endif
#endif
	exit(0);
}

#endif /* CONF_MASTER */
