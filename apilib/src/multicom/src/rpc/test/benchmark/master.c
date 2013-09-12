/*
 * shakedown/roundtrip/st20.c
 *
 * Copyright (C) STMicroelectronics Ltd. 2001
 *
 * ST20 side of the simple primative types shakedown test.
 */

#include "benchmark.h"

#ifdef __ST20__
#include <ostime.h>
typedef clock_t osclock_t;
#else
#include <os21.h>
#endif


static osclock_t hpp_time_now(void);

int main()
{
#ifdef ENABLE_SOAK_TEST
	int iterations = 0;
#endif
	osclock_t start, end, total;

	/* ensure that OS/20 and RPC are booted properly */
	harness_boot();

	/* make a trivial RPC call (wait for other side load fully) */
	st40_no_data();

#define DO_TEST(test) \
	{\
		start = hpp_time_now();\
		st40_dotest(test);\
		end = hpp_time_now();\
		total = time_minus(end, start);\
		printf("%-30s%10d\n", "st20_" #test, total/NUM_ITERATIONS);\
		\
		start = hpp_time_now();\
		st20_dotest(test);\
		end = hpp_time_now();\
		total = time_minus(end, start);\
		printf("%-30s%10d\n", "st40_" #test, total/NUM_ITERATIONS);\
	}

#ifdef ENABLE_SOAK_TEST
	soak_test:
#endif

	DO_TEST(no_data);
#ifndef ENABLE_PROFILE_TRICKS
	DO_TEST(int_1_word);
	DO_TEST(struct_4_word);
	DO_TEST(struct_16_word);
	DO_TEST(struct_64_word);
#ifndef REDUCE_COMPLEXITY
	DO_TEST(struct_256_word);
#ifdef EXTRA_COMPLEXITY
	DO_TEST(struct_1024_word);
#endif
#endif /* REDUCE_COMPLEXITY */
	DO_TEST(array_4_word);
	DO_TEST(array_16_word);
	DO_TEST(array_64_word);
	DO_TEST(array_256_word);
	DO_TEST(array_1024_word);
	DO_TEST(struct_10_word);
	DO_TEST(func_10_param);
#endif

#ifdef ENABLE_SOAK_TEST
	printf("\n******** Passed %d times ********\n\n", ++iterations);
	goto soak_test;
#endif

#undef DO_TEST

	harness_passed();
	return 0;
}

void st20_no_data          (void)                 {}
void st20_int_1_word       (int d)                {}
void st20_struct_4_word    (struct_4_word_t d)    {}
void st20_struct_16_word   (struct_16_word_t d)   {}
void st20_struct_64_word   (struct_64_word_t d)   {}
void st20_struct_256_word  (struct_256_word_t d)  {}
void st20_struct_1024_word (struct_1024_word_t d) {}
void st20_array_4_word     (array_4_word_t d)     {}
void st20_array_16_word    (array_16_word_t d)    {}
void st20_array_64_word    (array_64_word_t d)    {}
void st20_array_256_word   (array_256_word_t d)   {}
void st20_array_1024_word  (array_1024_word_t d)  {}
void st20_struct_10_word   (inout struct_10_word_t *p) {}
void st20_func_10_param    (
	inout int *a, inout int *b, inout int *c, inout int *d, inout int *e,
	inout int *f, inout int *g, inout int *h, inout int *i, inout int *j)
	{}

void st20_dotest(test_type_t test) {
	int i;

	VERBOSE(printf("st20_dotest(%d) ... ", test));
	VERBOSE(fflush(stdout));

	switch (test) {
		case no_data:
		{
			for (i=0; i<NUM_ITERATIONS; i++) { st40_no_data(); }
			break;
		}

		case int_1_word:
		{
			int i1;
			for (i=0; i<NUM_ITERATIONS; i++) { st40_int_1_word(i1); }
			break;
		}

		case struct_4_word:
		{
			struct_4_word_t s;
			for (i=0; i<NUM_ITERATIONS; i++) { st40_struct_4_word(s); }
			break;
		}

		case struct_16_word:
		{
			struct_16_word_t s;
			for (i=0; i<NUM_ITERATIONS; i++) { st40_struct_16_word(s); }
			break;
		}

		case struct_64_word:
		{
			struct_64_word_t s;
			for (i=0; i<NUM_ITERATIONS; i++) { st40_struct_64_word(s); }
			break;
		}

#ifndef REDUCE_COMPLEXITY
		case struct_256_word:
		{
			struct_256_word_t s;
			for (i=0; i<NUM_ITERATIONS; i++) { st40_struct_256_word(s); }
			break;
		}

#ifdef EXTRA_COMPLEXITY
		case struct_1024_word:
		{
			struct_1024_word_t s;
			for (i=0; i<NUM_ITERATIONS; i++) { st40_struct_1024_word(s); }
			break;
		}
#endif
#endif /* REDUCE_COMPLEXITY */

		case array_4_word:
		{
			array_4_word_t a;
			for (i=0; i<NUM_ITERATIONS; i++) { st40_array_4_word(a); }
			break;
		}

		case array_16_word:
		{
			array_16_word_t a;
			for (i=0; i<NUM_ITERATIONS; i++) { st40_array_16_word(a); }
			break;
		}

		case array_64_word:
		{
			array_64_word_t a;
			for (i=0; i<NUM_ITERATIONS; i++) { st40_array_64_word(a); }
			break;
		}

		case array_256_word:
		{
			array_256_word_t a;
			for (i=0; i<NUM_ITERATIONS; i++) { st40_array_256_word(a); }
			break;
		}

		case array_1024_word:
		{
			array_1024_word_t a;
			for (i=0; i<NUM_ITERATIONS; i++) { st40_array_1024_word(a); }
			break;
		}

		case struct_10_word:
		{
			struct_10_word_t a;
			for (i=0; i<NUM_ITERATIONS; i++) { st40_struct_10_word(&a); }
			break;
		}

		case func_10_param:
		{
			int a, b, c, d, e, f, g, h, i, j;
			for (i=0; i<NUM_ITERATIONS; i++) { st40_func_10_param(&a, &b, &c, &d, &e, &f, &g, &h, &i, &j); }
			break;
		}
		
		default:
			assert(0);
			break;
	}

	VERBOSE(printf("done\n"));
}

static osclock_t hpp_time_now()
{
#ifdef __ST20__
#if 1 == __CORE__
	return (osclock_t) (((double) harness_getTime() * 1000000.0) / (double) harness_getTicksPerSecond());
#else
	osclock_t t;
	__optasm {
		ldc 0;
		ldclock;
		st t;
	};

	return t;
#endif
#else
	return ((time_now() * 1000000) / time_ticks_per_sec());
#endif
}
