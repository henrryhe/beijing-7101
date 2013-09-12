/*
 * shakedown/roundtrip/st40.c
 *
 * Copyright (C) STMicroelectronics Ltd. 2001
 *
 * ST40 side of simple roundtrip shakedown test
 */

#include "roundtrip.h"

int main(void)
{
	harness_boot();

	/* we have nothing to do to just wait around */
	harness_waitForShutdown();
	return 0;
}

void remoteLastInt(int i) {
	VERBOSE(printf("remoteLastInt(%d)\n", i););
	setLastInt(i);
}
