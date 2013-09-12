/*
 * shakedown/primative/primative.h
 *
 * Copyright (C) STMicroelectronics Ltd. 2001
 *
 * Generic headerr for simple primative shakedown
 */

#ifndef rpc_str_h
#define rpc_str_h

#define  DEBUG
#include "harness.h"

#define MAX_LENGTH 40

/* prototypes */
void st20_strrev(inout.string(MAX_LENGTH) char *str);
void st40_strrev(inout.string(MAX_LENGTH) char *str);
void st20_strbang(in.string(MAX_LENGTH) char *str);
void st40_strbang(in.string(MAX_LENGTH) char *str);
void st20_run_tests(void);
void st40_run_tests(void);

arenas {
	{ slve, OS_FOR_SLAVE, CPU_FOR_SLAVE },
	{ mstr, OS_FOR_MASTER, CPU_FOR_MASTER }
};

transport {{ slve, mstr, TRANS_EMBX }};

headers {
	{ slve, { "\"str.h\"" } },
	{ mstr, { "\"str.h\"" } }
};

import {
	{ st20_strrev,                           slve, mstr },
	{ st40_strrev,                           mstr, slve },
	{ st20_strbang,                          slve, mstr },
	{ st40_strbang,                          mstr, slve },
	{ st20_run_tests,                        slve, mstr },
	{ st40_run_tests,                        mstr, slve },
};

#endif
