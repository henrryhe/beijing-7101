/*
 * callee.c
 *
 * Copyright (C) STMicroelectronics Limited 2003. All rights reserved.
 *
 * Called side of the RPC tricks example
 */

#include <stdio.h>
#include <string.h>

#include "tricks.h"
#include "harness.h"

volatile int finished = 0;

int main()
{
	rpcRuntimeInit();

	/* no real system should busy wait but for this example we
	 * don't want the clutter of code that works on all the OSs
	 */
	while (!finished) {}

	return 0;
}

void closeDown(void)
{
	finished = 1;
}

void decodeJTAG(jtagcode_t j, unsigned int *m, unsigned int *d, unsigned int *r)
{
	printf("decodeJTAG: device code 0x%x -> "
	       "manufacturer %x, device %x, revision %x\n",
	       j.id, j.jtag.manufacturer, j.jtag.device_code, j.jtag.revision);

	*m = j.jtag.manufacturer;
	*d = j.jtag.device_code;
	*r = j.jtag.revision;
}

void printLengthPrefixed(char *prefix)
{
	char t, u, v, w;

	/* record the trailing garbage (so we can show its garbage) */
	t = prefix[*prefix + 1];
	u = prefix[*prefix + 2];
	v = prefix[*prefix + 3];
	w = prefix[*prefix + 4];

	/* add a terminator */
	prefix[*prefix + 1] = '\0';

	/* now print the passed string and the trailing garbage */
	printf("printLengthPrefixed: prefix = '%s'\n", prefix+1);
	printf("printLengthPrefixed: trailing garbage = '%c%c%c%c'\n",
	       t, u, v, w);
}
