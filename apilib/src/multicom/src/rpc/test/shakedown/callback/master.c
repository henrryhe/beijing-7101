/*
 * shakedown/callback/st20.c
 *
 * Copyright (C) STMicroelectronics Ltd. 2001
 *
 * ST20 side of the callback shakedown test.
 */

#include <assert.h>
#include <stdio.h>

#ifdef __OS20__
#include <ostime.h>
#include <semaphor.h>
#include <task.h>
#else
#include <os21.h>
#endif

#include "callback.h"

static task_t       *alarmTask;
static semaphore_t  *alarmSem;
static int          alarmMsg;
static unsigned int alarmDelay;
static callback_ptr_t   alarmCallback;

void alarm_task(void *p);

int main()
{
	/* ensure that OS/20 and RPC are booted properly */
	harness_boot();

	alarmSem = semaphore_create_fifo(0);
	assert(alarmSem);
	alarmTask = task_create(alarm_task, NULL, 32*1024, MAX_USER_PRIORITY, "alarm_task", 0);
	assert(alarmTask);

	st40_run_tests();

	harness_passed();
	return 0;
}

void alarm_task(void* p)
{
	VERBOSE(printf("alarm_task()\n"));

	while (1) {
		semaphore_wait(alarmSem);
		VERBOSE(printf("alarm_task signaled\n"));
		task_delay(alarmDelay);
		VERBOSE(printf("alarm_task delay ended\n"));
		alarmCallback(alarmMsg);
		VERBOSE(printf("alarm_task complete callback\n"));
	}
}

void st20_do_alarm(unsigned int delay, int msg, callback_ptr_t func)
{
	/* tell me what thread-safety is again */
	alarmDelay    = delay;
	alarmMsg      = msg;
	alarmCallback = func;

#ifdef SIM231
	alarmDelay = delay / 100;
#endif

	semaphore_signal(alarmSem);
}


void st20_set_alarm(unsigned int delay, int msg, void (*func)(int))
{
	VERBOSE(printf("st20_set_alarm(%d, %d, 0x%08x)\n", 
		delay, msg, (unsigned int) func));
	st20_do_alarm(delay, msg, func);
}

void st20_set_alarm_fn(unsigned int delay, int msg, callback_fn_t *func)
{
	VERBOSE(printf("st20_set_alarm_fn(%d, %d, 0x%08x)\n", 
		delay, msg, (unsigned int) func));
	st20_do_alarm(delay, msg, func);
}

void st20_set_alarm_ptr(unsigned int delay, int msg, callback_ptr_t func)
{
	VERBOSE(printf("st20_set_alarm_ptr(%d, %d, 0x%08x)\n", 
		delay, msg, (unsigned int) func));
	st20_do_alarm(delay, msg, func);
}
