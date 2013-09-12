/*
 * shakedown/roundtrip/st40.c
 *
 * Copyright (C) STMicroelectronics Ltd. 2001
 *
 * ST40 side of simple primative types shakedown test
 */

#include "primative.h"

static char  nextExpectedChar;
static short nextExpectedShort;
static int   nextExpectedInt;
static long  nextExpectedLong;

int main(void)
{
	/* ensure the machine and RPC are booted and initialised */
	harness_boot();

	/* we have nothing to do to just wait around */
	harness_waitForShutdown();

	return 0;
}

void st40_reset_expected()
{
	VERBOSE(printf("st40_reset_expected()\n"));

	nextExpectedChar = 0;
	nextExpectedShort = 0;
	nextExpectedInt = 0;
	nextExpectedLong = 0;
}

int st40_test_char_short_int_long(char c, short s, int i, long l)
{
	VERBOSE(printf("st40_test_char_short_int_long(%d, %d, %d, %ld)\n", 
	                c, s, i, l));

	/* check against the expected values */
	assert(c == nextExpectedChar);
	assert(s == nextExpectedShort);
	assert(i == nextExpectedInt);
	assert(l == nextExpectedLong);

	/* update the expected values (note use of asymetric values to test
	 * for endianess problems)
	 */ 
	nextExpectedChar += CHAR_INCR;
	nextExpectedShort += SHORT_INCR;
	nextExpectedInt += INT_INCR;
	nextExpectedLong += LONG_INCR;

	return nextExpectedInt;
}

void st40_test_char_short_int_long_arrays(
	char ca[NUM_ARRAY_TESTS], short sa[NUM_ARRAY_TESTS],
	int  ia[NUM_ARRAY_TESTS], long  la[NUM_ARRAY_TESTS])
{
	int loop;
	char c  = 0; 
	short s = 0;
	int i   = 0;
	long l  = 0;

	VERBOSE(printf("st40_test_char_short_int_long_arrays(...)\n"));

	for (loop=0; loop<NUM_ARRAY_TESTS; loop++) {
		assert(c == ca[loop]);
		assert(s == sa[loop]);
		assert(i == ia[loop]);
		assert(l == la[loop]);

		c += CHAR_INCR;
		s += SHORT_INCR;
		i += INT_INCR;
		l += LONG_INCR;
	}
}

void st40_run_tests()
{
	int res, loop;

	VERBOSE(printf("st40_run_tests()\n"));

	st20_reset_expected();

	{
		char  c = 0;
		short s = 0;
		int   i = 0;
		long  l = 0;

		for (loop=0; loop < NUM_INT_TESTS; loop++) {
			res = st20_test_char_short_int_long(c, s, i, l);

			c += CHAR_INCR;
			s += SHORT_INCR;
			i += INT_INCR;
			l += LONG_INCR;

			assert(res == i);
		}
	}

	{
		char  c=0, ca[NUM_ARRAY_TESTS];
		short s=0, sa[NUM_ARRAY_TESTS];
		int   i=0, ia[NUM_ARRAY_TESTS];
		long  l=0, la[NUM_ARRAY_TESTS];

		for(loop=0; loop < NUM_ARRAY_TESTS; loop++) {
			ca[loop] = c;
			sa[loop] = s;
			ia[loop] = i;
			la[loop] = l;

			c += CHAR_INCR;
			s += SHORT_INCR;
			i += INT_INCR;
			l += LONG_INCR;
		}

		st20_test_char_short_int_long_arrays(ca, sa, ia, la);
	}
}
