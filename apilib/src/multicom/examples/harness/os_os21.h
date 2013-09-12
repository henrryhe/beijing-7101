/*
 * os_os21.h
 *
 * Copyright (C) STMicroelectronics Limited 2003. All rights reserved.
 *
 * Code to get OS21 up and running.
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h> /* for NULL */
#include <os21.h>

static void osRuntimeInit(void)
{
	kernel_initialize(NULL);
	kernel_start();

	task_priority_set(NULL, 0);
}
