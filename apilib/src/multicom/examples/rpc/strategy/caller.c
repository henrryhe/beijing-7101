/*
 * caller.c
 *
 * Copyright (C) STMicroelectronics Limited 2003. All rights reserved.
 *
 * Calling side of the RPC tricks example
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "strategy.h"
#include "harness.h"

addressRecord_t addressDB[] = {
	{ "Tony Blair", "RtHonTonyBlairMP@hotmail.com" },
	{ "Queen Elizabeth II", "queen@royal.gov.uk" },
	{ "", "" }
};

char callbackMessage[] = "not very interesting";

int main()
{
	char *backwards;
	int data[64];
	waveform_t waveform;
	int i;

	rpcRuntimeInit();

	/* demonstrate reverseString */
	backwards = (char *) malloc(strlen(addressDB[0].name) + 1);
	assert(backwards);
	strcpy(backwards, addressDB[0].name);
	printf("caller: calling reverseString()\n");
	reverseString(backwards);
	printf("caller: '%s' backwards is '%s'\n", addressDB[0].name, backwards );
	free(backwards);

	/* demonstrate readData and printWaveform */
	printf("caller: calling readData()\n");
	readData(sizeof(data)/4, data, &waveform.length);

	/* format convert the data and print it */
	for (i=0; i<waveform.length; i++) {
		waveform.data[i] = data[i];
	}
	printf("caller: calling printWaveform()\n");
	printWaveform(&waveform);

	/* demonstrate printAddresses */
	printf("caller: calling printAddresses()\n");
	printAddresses(addressDB);

	/* demonstrate the callback support */
	printf("caller: calling registerCallback()\n");
	registerCallback(actualCallback, (void *) callbackMessage);
	printf("caller: calling makeCallback()\n");
	makeCallback();
	
	closeDown();

	return 0;
}

int actualCallback(void *data)
{
	char *msg = (char *) data;
	
	printf("caller: executing actualCallback()\n");
	printf("caller: callback message is '%s'\n", msg);

	return strlen(msg);
}
