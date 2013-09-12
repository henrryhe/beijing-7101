/*
 * harness/harness.c
 *
 * Copyright (C) STMicroelectronics Limited 2003. All rights reserved.
 *
 * 
 */

#if defined __LINUX__ && !defined __KERNEL__
#define DISABLE_EMBX_CALLS
#endif

#include <stdarg.h>

#include "harness.h"
#include <mme.h>
#include "embx_osinterface.h"

int lastError;

void harness_passed()
{
	printf("******** "CPU"/"OS" is shutting down ********\n");

	/* check that the test has tidied up */
	MME_E(MME_DRIVER_NOT_INITIALIZED,Term());

#ifndef DISABLE_EMBX_CALLS
	EMBX(Deinit());
#endif

#ifndef __KERNEL__
	harness_deinit();
#endif

	/* allow time for our partners runtime loader to exit */
	harness_sleep(2);

	printf("******** Passed ********\n");
	exit(0);
}


	
void harness_waitForShutdown()
{
	printf("******** "CPU"/"OS" is shutting down ********\n");

	/* check that the test has tidied up */
	MME_E(MME_DRIVER_NOT_INITIALIZED,Term());

#ifndef DISABLE_EMBX_CALLS
	EMBX(Deinit());
#endif

#ifndef __KERNEL__
	harness_deinit();
#endif

	printf("******** "CPU"/"OS" shutdown OK ********\n");
	exit(0);
}
