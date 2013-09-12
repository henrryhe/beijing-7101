/*
 * shakedown/vararray/vararray.h
 *
 * Copyright (C) STMicroelectronics Ltd. 2001
 *
 * Generic header for vararray shakedown
 */

#ifndef rpc_vararray_h
#define rpc_vararray_h

/* #define DEBUG */
#include "harness.h"

#include <embx.h>

/* prototypes */
void st20_assertfail(in.string(127) char *, in.string(63) char *, int);
void st20_fillshared(int, int, in.shared int *);
void st40_fillshared(int, int, in.shared int *);
int st20_testshared(int, const in.shared int *);
int st40_testshared(int, const in.shared int *);
void st20_runtests(void);
void st40_runtests(void);

arenas {
	{ slve, OS_FOR_SLAVE, CPU_FOR_SLAVE },
	{ mstr, OS_FOR_MASTER, CPU_FOR_MASTER }
};

transport {{ slve, mstr, TRANS_EMBX }};

headers {
	{ slve, { "\"shared.h\"" } },
	{ mstr, { "\"shared.h\"" } }
};

import {
	{ st20_fillshared,                       slve, mstr },
	{ st40_fillshared,                       mstr, slve },
	{ st20_testshared,                       slve, mstr },
	{ st40_testshared,                       mstr, slve },
	{ st20_runtests,                         slve, mstr },
	{ st40_runtests,                         mstr, slve },
};

#endif
