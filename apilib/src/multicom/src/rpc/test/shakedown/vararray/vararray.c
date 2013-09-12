/*
 * shakedown/vararray/vararray.h
 *
 * Copyright (C) STMicroelectronics Ltd. 2001
 *
 * Fully symetric variable length array test (ST20 is master)
 */

#include "vararray.h"

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
        /* ensure the machine and RPC are booted and initialised */
	harness_boot();
        harness_waitForShutdown();
        return 0;
}
#endif


void DEFN(chararray_in) (int len, unsigned char* data)
{
	int i;

	VERBOSE(printf(PREFIX "chararray_in(%d, ...)\n", len));

	/* assert that the data contains what we expect it to contain */
	for (i=0; i<len; i++) {
		assert(data[i] == (char) len);
	}
}

void DEFN(chararray_out) (int len, unsigned char* data)
{
	int i;

	VERBOSE(printf(PREFIX "chararray_out(%d, ...)\n", len));
	
	/* fill the array */
	for (i=0; i<len; i++) {
		data[i] = (char) len;
	}
}

void DEFN(chararray_inout) (int len, unsigned char* data)
{
	int i;

	VERBOSE(printf(PREFIX "chararray_inout(%d, ...)\n", len));

	/* not the array */
	for (i=0; i<len; i++) {
		data[i] = ~data[i];
	}
}

void DEFN(structarray_in) (int len, struct_t* data)
{
	int i;

	VERBOSE(printf(PREFIX "structarray_in(%d, ...)\n", len));

	/* assert that the data contains what we expect it to contain */
	for (i=0; i<len; i++) {
		assert(data[i].c == (char) len);
		assert(data[i].i == len + (len << 16));
	}
}

void DEFN(structarray_out) (int len, struct_t* data)
{
	int i;

	VERBOSE(printf(PREFIX "structarray_out(%d, ...)\n", len));

	/* fill the array */
	for (i=0; i<len; i++) {
		data[i].c = (char) len;
		data[i].i = len + (len << 16);
	}
}

void DEFN(structarray_inout) (int len, struct_t* data)
{
	int i;

	VERBOSE(printf(PREFIX "structarray_inout(%d, ...)\n", len));

	/* not the array */
	for (i=0; i<len; i++) {
		data[i].c = ~data[i].c;
		data[i].i = ~data[i].i;
	}
}

void DEFN(runtests) (void)
{
	unsigned char i, *buf;
	struct_t *str;
        /* errno is undefined on entry to RPC func */
        errno = 0;

	VERBOSE(printf(PREFIX "runtests()\n"));

	/* test primative arrays */
	buf = malloc(61);
	for (i=10; i<=60; i+=10) {
		/* test in and out (where only one of in or out is remote) */
		memset(buf, 63, 61);
		st20_chararray_out(i, buf);
		memset(buf+i, 127, 61-i);
		st40_chararray_in(i, buf);
		assert(0 == errno);
		assert(127 == buf[i]); /* check for write past buffer */

		/* invert previous test for each device */
		memset(buf, 63, 61);
		st40_chararray_out(i, buf);
		memset(buf+i, 127, 61-i);
		st20_chararray_in(i, buf);
		assert(0 == errno);
		assert(127 == buf[i]); /* check for write past buffer */

		/* test inout */
		assert(i == buf[0]);
		st20_chararray_inout(i, buf);
		assert(i == (unsigned char) ~buf[0]);
		st40_chararray_inout(i, buf);
		assert(i == buf[0]);
		assert(0 == errno);
		assert(127 == buf[i]); /* check for write past buffer */

		/* check that buffer is undamaged by the inout */
		st40_chararray_in(i, buf);
		st20_chararray_in(i, buf);
		assert(0 == errno);
	}
	free(buf);

	/* repeat the test with more complex types */
	str = (struct_t *) malloc(61 * sizeof(struct_t));
	for (i=10; i<=60; i+=10) {
		/* test in and out (where only one of in or out is remote) */
		memset(str, 63, 61 * sizeof(struct_t));
		st20_structarray_out(i, str);
		memset(str+i, 127, (61-i) * sizeof(struct_t));
		st40_structarray_in(i, str);
		assert(0 == errno);
		assert(127 == str[i].c); /* check for write past buffer */

		/* invert previous test for each device */
		memset(str, 63, 61 * sizeof(struct_t));
		st40_structarray_out(i, str);
		memset(str+i, 127, (61-i) * sizeof(struct_t));
		st20_structarray_in(i, str);
		assert(0 == errno);
		assert(127 == str[i].c); /* check for write past buffer */

		/* test inout */
		assert(i == str[0].c);
		st20_structarray_inout(i, str);
		assert(i == (unsigned char) ~(str[0].c));
		st40_structarray_inout(i, str);
		assert(i == str[0].c);
		assert(0 == errno);
		assert(127 == str[i].c); /* check for write past buffer */

		/* check that buffer is undamaged by the inout */
		st40_structarray_in(i, str);
		st20_structarray_in(i, str);
		assert(0 == errno);
	}
	free((void *)str);
}
