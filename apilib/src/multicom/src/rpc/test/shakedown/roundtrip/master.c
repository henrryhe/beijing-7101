/*
 * shakedown/roundtrip/master.c
 *
 * Copyright (C) STMicroelectronics Ltd. 2001
 *
 * ST20 side of the simple roundtrip shakedown test.
 */

#include "roundtrip.h"

static int lastInt = -1;

int main()
{
	int i;

	/* ensure that OS/20 and RPC are booted properly */
	harness_boot();

	for (i=0; i<25; i++) {
		remoteLastInt(i);
		assert(i == lastInt);
	}

	harness_passed();
	return 0;
}

void setLastInt(int i) {
	VERBOSE(printf("setLastInt(%d)\n", i));
	lastInt = i;
}
