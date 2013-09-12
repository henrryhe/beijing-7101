/*
 * os_os20.h
 *
 * Copyright (C) STMicroelectronics Limited 2003. All rights reserved.
 *
 * Code to get STLite/OS20 up and running.
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h> /* for NULL */
#include <interrup.h>
#include <task.h>

static void osRuntimeInit(void)
{
	static char intStack[16 * 1024];

	int res;

	res = interrupt_init(7, intStack, sizeof(intStack),
			interrupt_trigger_mode_high_level, 0);
	assert(0 == res);
	interrupt_enable_global();

	task_priority_set(NULL, 0);
}

