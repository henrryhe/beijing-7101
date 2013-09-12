/*
 * shakedown/vararray/vararray.h
 *
 * Copyright (C) STMicroelectronics Ltd. 2001
 *
 * Fully symetric shared pointer tests (ST20 is master)
 */

#include "shared.h"

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
	/* Initialize the EMBX library (c.f. INSbl14282) */
	/*EMBXLIB_Init();*/

        /* ensure the machine and RPC are booted and initialised */
	harness_boot();
	harness_waitForShutdown();
        return 0;
}
#endif

void DEFN(fillshared) (int len, int seed, int *buf)
{
	int i;

	VERBOSE(printf(PREFIX "fillshared(%d, %d, 0x%08x)\n", len, seed, (unsigned int) buf));

	for (i=seed; i<(seed+len-1); i++, buf++) {
		*buf = i;
	}

	*buf = 0; /* set up a null terminator */
}

int DEFN(testshared) (int seed, const int *buf)
{
	int i;

	VERBOSE(printf(PREFIX "testshared(%d, 0x%08x)\n", seed, (unsigned int) buf));

	for (i=seed; *buf != 0; i++, buf++) {
		assert(*buf == i);
	}

	return i-seed+1;
}

void DEFN(runtests) (void)
{
	EMBX_TPINFO tpinfo;
	EMBX_TRANSPORT handle;

	int i, err, len, *buf;
        /* errno is undefined on entry to an RPC function */
        errno = 0;

	VERBOSE(printf(PREFIX "runtests()\n"));

	EMBX(GetFirstTransport(&tpinfo));
	EMBX(OpenTransport(tpinfo.name, &handle));

	for (i=8; i<128; i+=8) {
		/* allocate shared memory */
		EMBX(Alloc(handle, i * sizeof(int), (void **) &buf));

		/* test fill on st20 and test on st40 */
		memset(buf, 127, i * sizeof(int));
		st20_fillshared(i, i*i*i, buf);
		assert(0 == errno);
		assert(0 == buf[i-1]); /* check terminator */

		len = st40_testshared(i*i*i, buf);
		assert(0 == errno);
		assert(i == len);

		/* test fill on st40 and test on st20 */
		memset(buf, 127, i * sizeof(int));
		st40_fillshared(i, i*i*i, buf);
		assert(0 == errno);
		assert(0 == buf[i-1]); /* check terminator */

		len = st20_testshared(i*i*i, buf);
		assert(0 == errno);
		assert(i == len);

		/* free shared memory */
		EMBX(Free(buf));
	}

	EMBX(CloseTransport(handle));
}
