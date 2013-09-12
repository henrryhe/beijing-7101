/*
 * shakedown/termarray/termarray.h
 *
 * Copyright (C) STMicroelectronics Ltd. 2001
 *
 * Fully symetric terminated array test (ST20 is master)
 */

#include "termarray.h"

/* this file is compiled on both ST20 and ST40, the functions defined
 * using the DEFN macro will be prefixed with the processor name
 */
#ifdef CONF_MASTER
#define PREFIX   "st20_"
#define DEFN(fn) st20_ ## fn
#else
#define PREFIX   "st40_"
#define DEFN(fn) st40_ ## fn
#endif

#ifdef CONF_MASTER
int main()
{
	harness_boot();

	st20_runtests();
	st40_runtests();

	harness_passed();
	return 0;
}
#else
int main(void)
{
	harness_boot();
	harness_waitForShutdown();
        return 0;
}
#endif


void DEFN(chararray_in) (unsigned char* data)
{
	int i;

	VERBOSE(printf(PREFIX "chararray_in(...)\n"));

	/* assert that the data contains what we expect it contain */
	for (i=0; data[i]!=0; i++) {
		assert(data[i] == (char) (127-i));
	}
}

void DEFN(chararray_out) (int len, unsigned char* data)
{
	int i;

	VERBOSE(printf(PREFIX "chararray_out(%d, ...)\n", len));
	
	/* fill the array */
	for (i=0; i<len; i++) {
		data[i] = (char) (127-i);
	}
	data[i-1] = (char) 0;
}

void DEFN(chararray_inout) (unsigned char* data)
{
	int i;

	VERBOSE(printf(PREFIX "chararray_inout(...)\n"));

	/* not the array */
	for (i=0; data[i]!=0; i++) {
		data[i] = ~data[i];
	}
}

void DEFN(structarray_in) (struct_t *data)
{
	int i;

	VERBOSE(printf(PREFIX "structarray_in(...)\n"));

	/* assert that the data contains what we expect it to contain */
	for (i=0; data[i].c!=0; i++) {
		assert(data[i].c == (char) (127-i));
		assert(data[i].i == (127-i) + ((127-i) << 16));
	}
}

void DEFN(structarray_out) (int len, struct_t* data)
{
	int i;

	VERBOSE(printf(PREFIX "structarray_out(%d, ...)\n", len));
	
	/* fill the array */
	for (i=0; i<len; i++) {
		data[i].c = (char) (127-i);
		data[i].i = (127 - i) + ((127-i) << 16);
	}
	data[i-1].c = (char) 0;
}

void DEFN(structarray_inout) (struct_t* data)
{
	int i;

	VERBOSE(printf(PREFIX "structarray_inout(...)\n"));

	/* not the array */
	for (i=0; data[i].c!=0; i++) {
		data[i].c = ~(data[i].c);
		data[i].i = ~(data[i].i);
	}
}

volatile int *_ST_errno(void);

void DEFN(runtests) (void)
{
	unsigned char i, *buf;
	struct_t *str;
        /* errno is undefined on entry to an RPC function */
        errno = 0;

	VERBOSE(printf(PREFIX "runtests()\n"));

	buf = malloc(61);
	for (i=10; i<=60; i+=10) {
		/* test in and out (where only one of in or out is remote) */
		memset(buf, 31, 61);
		st20_chararray_out(i, buf);
		memset(buf+i, 63, 61-i);
		st40_chararray_in(buf);

		assert(0 == errno);
		assert(63 == buf[i]); /* check for write past buffer */

		/* invert previous test for each device */
		memset(buf, 31, 61);
		st40_chararray_out(i, buf);
		memset(buf+i, 63, 61-i);
		st20_chararray_in(buf);
		assert(0 == errno);
		assert(63 == buf[i]); /* check for write past buffer */

		/* test inout */
		assert(127 == buf[0]);
		st20_chararray_inout(buf);
		assert(127 == (unsigned char) ~buf[0]);
		st40_chararray_inout(buf);
		assert(127 == buf[0]);
		assert(0 == errno);
		assert(63 == buf[i]); /* check for write past buffer */

		/* check that buffer is undamaged by the inout */
		st40_chararray_in(buf);
		st20_chararray_in(buf);
		assert(0 == errno);
	}
	free(buf);

	str = malloc(61 * sizeof(struct_t));
	for (i=10; i<=60; i+=10) {
		/* test in and out (where only one of in or out is remote) */
		memset(str, 31, 61 * sizeof(struct_t));
		st20_structarray_out(i, str);
		memset(str+i, 63, (61-i) * sizeof(struct_t));
		st40_structarray_in(str);
		assert(0 == errno);
		assert(63 == str[i].c); /* check for write past buffer */

		/* invert previous test for each device */
		memset(str, 31, 61 * sizeof(struct_t));
		st40_structarray_out(i, str);
		memset(str+i, 63, (61-i) * sizeof(struct_t));
		st20_structarray_in(str);
		assert(0 == errno);
		assert(63 == str[i].c); /* check for write past buffer */

		/* test inout */
		assert(127 == str[0].c);
		st20_structarray_inout(str);
		assert(127 == (unsigned char) ~(str[0].c));
		st40_structarray_inout(str);
		assert(127 == str[0].c);
		assert(0 == errno);
		assert(63 == str[i].c); /* check for write past buffer */

		/* check that buffer is undamaged by the inout */
		st40_structarray_in(str);
		st20_structarray_in(str);
		assert(0 == errno);
	}
	free(str);

}
