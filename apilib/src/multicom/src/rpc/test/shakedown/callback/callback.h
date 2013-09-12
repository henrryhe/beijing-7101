/*
 * shakedown/callback/callback.h
 *
 * Copyright (C) STMicroelectronics Ltd. 2001
 *
 * Generic header for a test of function pointer based callbacks
 * (this test is not semetric an simulates the ST20 as a smart peripheral
 * view of the world).
 */

#ifndef rpc_callback_h
#define rpc_callback_h

#define DEBUG
#include "harness.h"

/* types */
typedef void (*callback_ptr_t)(int);
typedef void callback_fn_t(int);

/* prototypes */
void st20_set_alarm(unsigned int delay, int msg, void (*func)(int));
void st20_set_alarm_fn(unsigned int delay, int msg, callback_fn_t *func);
void st20_set_alarm_ptr(unsigned int delay, int msg, callback_ptr_t func);
void st40_callback(int msg);
void st40_run_tests(void);

arenas {
	{ slve, OS_FOR_SLAVE, CPU_FOR_SLAVE },
	{ mstr, OS_FOR_MASTER, CPU_FOR_MASTER }
};

transport {{ mstr, slve, TRANS_EMBX }};

headers {
	{ mstr, { "\"callback.h\"" } },
	{ slve, { "\"callback.h\"" } }
};

import {
	{ st20_set_alarm,                        slve, mstr },
	{ st20_set_alarm_fn,                     slve, mstr },
	{ st20_set_alarm_ptr,                    slve, mstr },
	{ st40_callback,                         mstr, slve },
	{ st40_run_tests,                        mstr, slve }
};

#endif /* rpc_callback_h */
