/*
 * os_linux.h
 *
 * Copyright (C) STMicroelectronics Limited 2003. All rights reserved.
 *
 * Code to get Linux up and running.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

static void osRuntimeInit(void)
{
	/* force line buffering even isatty() is zero (gdbserver) */
	setvbuf(stdout, NULL, _IOLBF, 0);
}

