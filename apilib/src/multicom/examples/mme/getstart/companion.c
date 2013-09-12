/*
 * companion.c
 *
 * Copyright (C) STMicroelectronics Limited 2004. All rights reserved.
 *
 * Remote component of the getstart example.
 */

#include <mme.h>

#include "harness.h"
#include "mixer.h"

int main(void)
{
	MME_ERROR err;

	/* initialize the underlying EMBX transport */
	embxRuntimeInit();

	/* initialize the MME API */
	err = MME_Init();
	if (MME_SUCCESS != err) {
		printf("ERROR: cannot initialize the MME API (%d)\n", err);
	}
	err = MME_RegisterTransport("shm");
	if (MME_SUCCESS != err) {
		printf("ERROR: cannot register transport to external processors (%d)\n", err);
	}
	err = Mixer_RegisterTransformer("mme.8bit_mixer");
	if (MME_SUCCESS != err) {
		printf("ERROR: cannot register mme.8bit_mixer (%d)\n", err);
	}

        /* enter the main MME execution loop */
	err = MME_Run();
	if (MME_SUCCESS != err) {
		printf("ERROR: MME execution loop reported an error (%d)\n", err);
	}

	err = MME_Term();
	if (MME_SUCCESS != err) {
		printf("ERROR: cannot terminate the MME API (%d)\n", err);
	}

	(void) EMBX_Deinit();

	return 0;
}
