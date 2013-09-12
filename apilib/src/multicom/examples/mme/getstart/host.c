/*
 * host.c
 *
 * Copyright (C) STMicroelectronics Limited 2004. All rights reserved.
 *
 * 
 */

#include <mme.h>
#include <os_abstractions.h>

#include "harness.h"
#include "mixer.h"

static OS_SIGNAL callbackSignal;

static char sample1[] = { 34, 38, 33, 19,  5,  8,  8, 23, 36, 38, 31, 16,
			   3, 13, 10, 25, 38, 39, 29, 13, 28, 20, 12, 28 };
static char sample2[] = {  4, 28, 18, 19,  4,  8,  8,  5, 12, 15, 15, 12,
			   8, 29, 13, 28, 20, 12, 28, 23, 26, 28, 21, 16 };

/* hashes returns a string of hashes based on the value of l. it is used
 * to graphically display amplitude.
 */
static const char *hashes(int l)
{
	static const char h[] = "########################################"
				"########################################";
	
	assert(l < 80);
	return h + sizeof(h) - l;
}

/* use hashes to display a single waveform */
static void printSignal(char *a, int n)
{
	char *c;

	for (c=a; c<a+n; c++) {
		printf("%s\n", hashes(*c));
	}
}

/* use hashes to display two waveforms (one on each side of the display) */
static void printTwinSignal(char *a, char *b, int n)
{
	char *c, *d;

	for (c=a, d=b; c<a+n; c++, d++) {
		printf("%-40s%40s\n", hashes(*c), hashes(*d));
	}
}

static void mixerCallback(MME_Event_t Event, MME_Command_t *CallbackData, void *UserData)
{
	OS_SIGNAL_POST(&callbackSignal);
}

static MME_ERROR simpleMix(MME_TransformerHandle_t hdl)
{
	MME_ERROR err;
	MME_DataBuffer_t *bufs[3] = { 0, 0, 0 };
	MME_Command_t cmd = { sizeof(MME_Command_t) };

	/* allocate databuffers using MME_ALLOCATION_PHYSICAL to guarantee
	 * a single scatter page per allocation */

	err = MME_AllocDataBuffer(hdl, sizeof(sample1), MME_ALLOCATION_PHYSICAL, bufs+0);
	if (MME_SUCCESS != err) goto error_recovery;
	memcpy(bufs[0]->ScatterPages_p->Page_p, sample1, sizeof(sample1));

	err = MME_AllocDataBuffer(hdl, sizeof(sample2), MME_ALLOCATION_PHYSICAL, bufs+1);
	if (MME_SUCCESS != err) goto error_recovery;
	memcpy(bufs[1]->ScatterPages_p->Page_p, sample2, sizeof(sample2));

	err = MME_AllocDataBuffer(hdl, sizeof(sample1), MME_ALLOCATION_PHYSICAL, bufs+2);
	if (MME_SUCCESS != err) goto error_recovery;

	/* fill in the non-zero parts of the command structure and issue the command.
	 * 
	 * Note that setting constant due times will cause transforms to happen in 
	 * strict FIFO order within a transformer (and round-robin within multiple 
	 * transformers) 
	 */
	cmd.CmdCode = MME_TRANSFORM;
	cmd.CmdEnd = MME_COMMAND_END_RETURN_NOTIFY;
	cmd.DueTime = 0;
	cmd.NumberInputBuffers = 2;
	cmd.NumberOutputBuffers = 1;
	cmd.DataBuffers_p = bufs;
	err = MME_SendCommand(hdl, &cmd);
	if (MME_SUCCESS != err) goto error_recovery;
	OS_SIGNAL_WAIT(&callbackSignal);

	/* in normal use we would send many more commands before returning (at which
	 * point the transformer will be terminated.
	 */

	/* show the mixed waveform */
	printSignal(bufs[2]->ScatterPages_p->Page_p, sizeof(sample1));
	
	/*FALLTHRU*/
    error_recovery:

	if (bufs[0]) (void) MME_FreeDataBuffer(bufs[0]);
	if (bufs[1]) (void) MME_FreeDataBuffer(bufs[1]);
	if (bufs[2]) (void) MME_FreeDataBuffer(bufs[2]);

	return err;
}

int main(void)
{
	MME_TransformerInitParams_t initParams = { sizeof(MME_TransformerInitParams_t) };
	MixerParamsInit_t mixerInit;
	MME_TransformerHandle_t localHdl, remoteHdl;

	MME_ERROR err;

	/* initialize the underlying EMBX transport */
	embxRuntimeInit();

	/* initialize the MME API */
	err = MME_Init();
	if (MME_SUCCESS != err) {
		printf("ERROR: cannot initialize the MME API (%d)\n", err);
	}

	/* other misc. initialization */
	OS_SIGNAL_INIT(&callbackSignal);

	/* pre-construct the data structure to be passed at transformer 
	 * initialization time
	 */
	initParams.Priority = MME_PRIORITY_NORMAL;
	initParams.Callback = mixerCallback;
	initParams.TransformerInitParamsSize = sizeof(MixerParamsInit_t);
	initParams.TransformerInitParams_p = &mixerInit;
	MME_INDEXED_PARAM(mixerInit, MixerParamsInit_volume, 0) = 1.0;
	MME_INDEXED_PARAM(mixerInit, MixerParamsInit_volume, 1) = 4.0/3.0;

	
	/* display the waveforms that will be mixed */
	printf("ORIGINAL WAVEFORMS\n"
	       "------------------\n\n");
	printTwinSignal(sample1, sample2, sizeof(sample1));


	printf("\nSIMPLE MIXING USING A LOCALLY REGISTERED TRANSFORMER\n"
	         "----------------------------------------------------\n\n");

	/* register a local transformer */
	err = Mixer_RegisterTransformer("local.8bit_mixer");
	if (MME_SUCCESS != err) {
		printf("ERROR: cannot register local.8bit_mixer (%d)\n", err);
	}

	/* initialize the local transformer */
	err = MME_InitTransformer("local.8bit_mixer", &initParams, &localHdl);
	if (MME_SUCCESS != err) {
		printf("ERROR: cannot instanciate local.8bit_mixer (%d)\n", err);
	}

	/* perform a simple mix using the local transformer */
	err = simpleMix(localHdl);
	if (MME_SUCCESS != err) {
		printf("ERROR: cannot perform mixer transformer on mme.8bit_mixer (%d)\n", err);
	}


	printf("\nSIMPLE MIXING USING A REMOTELY REGISTERED TRANSFORMER\n"
	         "-----------------------------------------------------\n\n");

#if !(defined(__LINUX__) && !defined(__KERNEL__))
	/* register the transport for the remote transformer (at this point we will
	 * block until the remote processor initializes itself)
	 */
	err = MME_RegisterTransport("shm");
	if (MME_SUCCESS != err) {
		printf("ERROR: cannot register transport to external processors (%d)\n", err);
	}
#endif

	/* repeatly attempt to connect to the remote transformer until it is registered or
	 * a serious error occurs.
	 */
	do {
		err = MME_InitTransformer("mme.8bit_mixer", &initParams, &remoteHdl);
	} while (err == MME_UNKNOWN_TRANSFORMER);
	if (MME_SUCCESS != err) {
		printf("ERROR: cannot instanciate mme.8bit_mixer (%d)\n", err);
	}

	/* perform the same mix as above using a remotely registered transformer. the 
	 * results should be identical assuming both processors implement IEEE single
	 * precision floating point maths.
	 */
	err = simpleMix(remoteHdl);
	if (MME_SUCCESS != err) {
		printf("ERROR: cannot perform mixer transformer on mme.8bit_mixer (%d)\n", err);
	}

	/* clean up prior to shutting down */
	err = MME_TermTransformer(remoteHdl);
	if (MME_SUCCESS != err) {
		printf("ERROR: cannot terminate mme.8bit_mixer (%d)\n", err);
	}

	err = MME_TermTransformer(localHdl);
	if (MME_SUCCESS != err) {
		printf("ERROR: cannot terminate local.8bit_mixer (%d)\n", err);
	}

	err = MME_Term();
	if (MME_SUCCESS != err) {
		printf("ERROR: cannot terminate the MME API (%d)\n", err);
	}

#ifndef __LINUX__
	(void) EMBX_Deinit();
#endif

	return 0;
}
