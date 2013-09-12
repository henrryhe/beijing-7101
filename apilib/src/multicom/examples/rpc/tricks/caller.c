/*
 * caller.c
 *
 * Copyright (C) STMicroelectronics Limited 2003. All rights reserved.
 *
 * Calling side of the RPC tricks example
 */

#include <stdio.h>
#include <string.h>

#include "tricks.h"
#include "harness.h"

int main()
{
	jtagcode_t j;
	unsigned int m, d, r;

	char prefix[] = "^This is length prefixed!!!!!";

	rpcRuntimeInit();

	/* this is a real device code ... but for which part */
	j.id = 0x2d40a041;
	decodeJTAG(j, &m, &d, &r);
	printf("caller: device code 0x%x -> "
	       "manufacturer %x, device %x, revision %x\n",
	       j.id, m, d, r);

	/* calculate the prefix (-1 for prefix and -4 to remove four '!'s */
	prefix[0] = strlen(prefix) - 1 - 4;
	printf("caller: prefix = '%s'\n", prefix+1);
	printLengthPrefixed(prefix);

	closeDown();

	return 0;
}
