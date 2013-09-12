/*
 * benchmark/benchmark.h
 *
 * Copyright (C) STMicroelectronics Ltd. 2001
 *
 * Generic header for the RPC benchmark
 */

#ifndef rpc_benchmark_h
#define rpc_benchmark_h

#include "harness.h"

#ifdef ENABLE_PROFILE_TRICKS
#define NUM_ITERATIONS 100000
#else
#define NUM_ITERATIONS 1000
#endif

/* typedefs */
typedef struct struct_4_word    { int a, b, c, d; }               struct_4_word_t;
typedef struct struct_16_word   { struct_4_word_t a, b, c, d; }   struct_16_word_t;
typedef struct struct_64_word   { struct_16_word_t a, b, c, d; }  struct_64_word_t;
typedef struct struct_256_word  { struct_64_word_t a, b, c, d; }  struct_256_word_t;
typedef struct struct_1024_word { struct_256_word_t a, b, c, d; } struct_1024_word_t;

typedef int    array_4_word_t[4];
typedef int    array_16_word_t[16];
typedef int    array_64_word_t[64];
typedef int    array_256_word_t[256];
typedef int    array_1024_word_t[1024];

typedef struct struct_10_word   { int a, b, c, d, e, f, g, h, i, j; }
								  struct_10_word_t;

typedef enum test_type {
	no_data, int_1_word,
	struct_4_word, struct_16_word, struct_64_word, struct_256_word, struct_1024_word,
	array_4_word,  array_16_word,  array_64_word,  array_256_word,  array_1024_word,
	struct_10_word, func_10_param
} test_type_t;

/* prototypes */
void st20_assertfail       (in.string(127) char *, in.string(63) char *, int);
void st20_no_data          (void);
void st40_no_data          (void);
void st20_int_1_word       (int d);
void st40_int_1_word       (int d);
void st20_struct_4_word    (struct_4_word_t d);
void st40_struct_4_word    (struct_4_word_t d);
void st20_struct_16_word   (struct_16_word_t d);
void st40_struct_16_word   (struct_16_word_t d);
void st20_struct_64_word   (struct_64_word_t d);
void st40_struct_64_word   (struct_64_word_t d);
void st20_struct_256_word  (struct_256_word_t d);
void st40_struct_256_word  (struct_256_word_t d);
void st20_struct_1024_word (struct_1024_word_t d);
void st40_struct_1024_word (struct_1024_word_t d);
void st20_array_4_word     (array_4_word_t d);
void st40_array_4_word     (array_4_word_t d);
void st20_array_16_word    (array_16_word_t d);
void st40_array_16_word    (array_16_word_t d);
void st20_array_64_word    (array_64_word_t d);
void st40_array_64_word    (array_64_word_t d);
void st20_array_256_word   (array_256_word_t d);
void st40_array_256_word   (array_256_word_t d);
void st20_array_1024_word  (array_1024_word_t d);
void st40_array_1024_word  (array_1024_word_t d);
void st20_struct_10_word   (inout struct_10_word_t *p);
void st40_struct_10_word   (inout struct_10_word_t *p);
void st20_func_10_param    (inout int *a, inout int *b, inout int *c, inout int *d, inout int *e,
                            inout int *f, inout int *g, inout int *h, inout int *i, inout int *j);
void st40_func_10_param    (inout int *a, inout int *b, inout int *c, inout int *d, inout int *e,
                            inout int *f, inout int *g, inout int *h, inout int *i, inout int *j);
void st20_dotest           (test_type_t test);
void st40_dotest           (test_type_t test);

arenas {
	{ slve, OS_FOR_SLAVE,  CPU_FOR_SLAVE },
	{ mstr, OS_FOR_MASTER, CPU_FOR_MASTER }
};

transport {{ slve, mstr, TRANS_EMBX }};

headers {
	{ slve, { "\"benchmark.h\"" } },
	{ mstr, { "\"benchmark.h\"" } }
};

import {
	{ st20_no_data,                          slve, mstr },
	{ st40_no_data,                          mstr, slve },
	{ st20_int_1_word,                       slve, mstr },
	{ st40_int_1_word,                       mstr, slve },
	{ st20_struct_4_word,                    slve, mstr },
	{ st40_struct_4_word,                    mstr, slve },
	{ st20_struct_16_word,                   slve, mstr },
	{ st40_struct_16_word,                   mstr, slve },
	{ st20_struct_64_word,                   slve, mstr },
	{ st40_struct_64_word,                   mstr, slve },
/* gcc cannot complie generated code of this complexity */
#ifndef REDUCE_COMPLEXITY
	{ st20_struct_256_word,                  slve, mstr },
	{ st40_struct_256_word,                  mstr, slve },
#ifdef EXTRA_COMPLEXITY
	{ st20_struct_1024_word,                 slve, mstr },
	{ st40_struct_1024_word,                 mstr, slve },
#endif
#endif
	{ st20_array_4_word,                     slve, mstr },
	{ st40_array_4_word,                     mstr, slve },
	{ st20_array_16_word,                    slve, mstr },
	{ st40_array_16_word,                    mstr, slve },
	{ st20_array_64_word,                    slve, mstr },
	{ st40_array_64_word,                    mstr, slve },
	{ st20_array_256_word,                   slve, mstr },
	{ st40_array_256_word,                   mstr, slve },
	{ st20_array_1024_word,                  slve, mstr },
	{ st40_array_1024_word,                  mstr, slve },
	{ st20_struct_10_word,                   slve, mstr },
	{ st40_struct_10_word,                   mstr, slve },
	{ st20_func_10_param,                    slve, mstr },
	{ st40_func_10_param,                    mstr, slve },
	{ st20_dotest,                           slve, mstr },
	{ st40_dotest,                           mstr, slve }
};

#endif
