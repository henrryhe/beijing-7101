/*
 * shakedown/callback/st40.c
 *
 * Copyright (C) STMicroelectronics Ltd. 2001
 *
 * ST40 side of the callback shakedown test
 */

#include "callback.h"

static volatile int alarmFired = 0;
static volatile int alarmMsg = 42;

int main(void)
{
	/* ensure the machine and RPC are booted and initialised */
	harness_boot();

	/* we have nothing to do to just wait around */
	harness_waitForShutdown();

	return 0;
}

void st40_callback(int msg)
{
	VERBOSE(printf("st40_callback(%d)\n", msg));

	assert(0 == alarmFired);
	assert(msg == alarmMsg);

	alarmFired = 1;
}

void st40_run_tests()
{
	VERBOSE(printf("st40_run_tests()\n"));

	alarmFired = 0;
	st20_set_alarm(7000, alarmMsg, st40_callback);
	while (!alarmFired) { harness_sleep(1); }

	alarmFired = 0;
	st20_set_alarm_fn(7000, alarmMsg, st40_callback);
	while (!alarmFired) { harness_sleep(1); }

	alarmFired = 0;
	st20_set_alarm_ptr(7000, alarmMsg, st40_callback);
	while (!alarmFired) { harness_sleep(1); }
}
