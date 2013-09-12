/*
 * shakedown/primative/primative.h
 *
 * Copyright (C) STMicroelectronics Ltd. 2001
 *
 * Generic headerr for simple primative shakedown
 */

#ifndef rpc_primative_h
#define rpc_primative_h

#include "harness.h"

#define NUM_INT_TESTS   (255/3)
#define NUM_ARRAY_TESTS 10
#define CHAR_INCR       0x03
#define SHORT_INCR      0x0030
#define INT_INCR        0x00300303
#define LONG_INCR       0x03030030

/* prototypes */
void st20_reset_expected(void);
/* regression test for INSbl19429 - problems with extern prototypes */
extern int  st20_test_char_short_int_long(char c, short s, int i, long l);
void st20_test_char_short_int_long_arrays(
	char ca[NUM_ARRAY_TESTS], short sa[NUM_ARRAY_TESTS],
	int  ia[NUM_ARRAY_TESTS], long  la[NUM_ARRAY_TESTS]);
void st20_run_tests(void);

void st40_reset_expected(void);
/* regression test for INSbl19429 - problems with extern prototypes */
extern int  st40_test_char_short_int_long(char, short, int, long);
void st40_test_char_short_int_long_arrays(
	char ca[NUM_ARRAY_TESTS], short sa[NUM_ARRAY_TESTS],
	int  ia[NUM_ARRAY_TESTS], long  la[NUM_ARRAY_TESTS]);
void st40_run_tests(void);

arenas {
	{ slve, OS_FOR_SLAVE,  CPU_FOR_SLAVE },
	{ mstr, OS_FOR_MASTER, CPU_FOR_MASTER }
};

transport {{ slve, mstr, TRANS_EMBX }};

headers {
	{ slve, { "\"primative.h\"" } },
	{ mstr, { "\"primative.h\"" } }
};

import {
	{ st20_reset_expected,                   slve, mstr },
	{ st20_test_char_short_int_long,         slve, mstr },
	{ st20_test_char_short_int_long_arrays,  slve, mstr },
	{ st40_reset_expected,                   mstr, slve },
	{ st40_test_char_short_int_long,         mstr, slve },
	{ st40_test_char_short_int_long_arrays,  mstr, slve },
	{ st40_run_tests,                        mstr, slve }
};

#endif
