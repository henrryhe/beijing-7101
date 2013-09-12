/*
 * callee.c
 *
 * Copyright (C) STMicroelectronics Limited 2003. All rights reserved.
 *
 * Called side of the RPC tricks example
 */

#include <stdio.h>
#include <string.h>

#include "strategy.h"
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

void reverseString(char *s)
{
	int i, j;

	printf("callee: executing reverseString()\n");

	for (i=0, j=strlen(s)-1; j - i > 0; i++, j--) {
		/* on modern machines xor swaps are slower than using tempories 
		 * but they are much more fun!
		 */
		s[i] ^= s[j];
		s[j] ^= s[i];
		s[i] ^= s[j];
	}
}

void readData(int length, int *data, int *actualLength)
{
	static const int sineWave[] = {
		40, 46, 52, 58, 64, 68, 72, 75, 77, 78,
		79, 78, 77, 75, 72, 68, 64, 58, 52, 46,
		40, 34, 28, 22, 16, 12,  8,  5,  3,  2,
		 1,  2,  3,  5,  8, 12, 16, 22, 28, 34,
		40
	};

	int bytes = (4*length < sizeof(sineWave) ? 4*length : sizeof(sineWave));

	printf("callee: executing readData()\n");
	memcpy(data, sineWave, bytes);
	*actualLength = bytes / 4;
}

void printWaveform(waveform_t *w)
{
	int i;

	printf("callee: executing printWaveform()\n");

	for (i=0; i<w->length; i++) {
		printf("%*c\n", w->data[i], '#');
	}
}

void printAddresses(addressRecord_t *addr)
{
	printf("callee: executing printAddresses()\n");
	printf("%-39s| %-38s\n", "Name", "E-Mail");
	printf("---------------------------------------+---------------------------------------\n");
	for ( ; addr->name[0] != '\0'; addr++) {
		printf("%-39s| %-38s\n", addr->name, addr->email);
	}
}

callback_fn_t callback;
void *callbackData;

void registerCallback(callback_fn_t cb, void *data)
{
	printf("callee: executing registerCallback()\n");
	callback = cb;
	callbackData = data;
}

void makeCallback(void)
{
	int res;

	printf("callee: executing registerCallback()\n");
	printf("callee: calling callback function pointer\n");
	res = callback(callbackData);
	printf("callee: callback return %d\n", res);
}
