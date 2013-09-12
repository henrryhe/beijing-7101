/*
 * host.c
 *
 * Copyright (C) STMicroelectronics Limited 2004. All rights reserved.
 *
 * 
 */

#include <os21.h>
#include <os21/st40.h>
#include <os21/st40/stm8000.h>

#include <assert.h>
#include <string.h>

#include <embx.h>
#include <embxshm.h>
#include <embxmailbox.h>
#include <mme.h>
#include <os_abstractions.h>

#include "mixer.h"

/*
 * Shared memory transport factory configuration structure
 */
EMBXSHM_MailboxConfig_t config = {
	"shm",                   /* Local transport name                      */
	0,                       /* ST40/master is always CPU 0               */
	{ 1, 1, 1, 0 },          /* CPU 0,1,2 will be participating           */
	0x60000000,              /* Standard pointer warp for ST40 P2 address */
	                         /* to a bus address.                         */
	0,
	16,
	16,
	(EMBX_VOID *)0xa9800000, /* Hardcoded location in LMI for the shared heap */
                                 /* in an area known to be unused by the          */
                                 /* standard ST220 toolset configurations.        */
	1*1024*1024              /* Set a 1Mb heap */
};

int init_mailboxes(void)
{
	EMBX_ERROR err;

	if((err = EMBX_Mailbox_Init()) != EMBX_SUCCESS) {
		printf("Failed to intialise mailbox library err = %d\n",err);
		return -1;
	}

	/*
	 * Register the first hardware mailbox on the STm8000 as the
	 * one the ST40 will be interrupted by. This will use the first
	 * half of the register set when the mailbox library allocates
	 * mailbox registers to us. The interrupt level parameter is
	 * ignored when using OS21.
	 */
	err = EMBX_Mailbox_Register((void *)0xb0200000,
				    OS21_INTERRUPT_MBOX1_0,
				    0,
				    EMBX_MAILBOX_FLAGS_SET1);

	if(err != EMBX_SUCCESS) {
		printf("Failed to register mbox 0 err = %d\n",err);
		return -1;
	}

	/*
	 * Register the second hardware mailbox on the STm8000. This
	 * will be used to signal the other CPUs.
	 */
	if((err = EMBX_Mailbox_Register((void *)0xb0210000, -1, -1, 0)) != EMBX_SUCCESS) {
		printf("Failed to register mbox 1 err = %d\n",err);
		return -1;
	}

	printf("Initialized mailbox library\n");

	return 0;
}


int init_comms(EMBX_FACTORY *pFact,EMBX_TRANSPORT *pTrans,EMBX_PORT *pPort)
{
	EMBX_ERROR err;
	EMBX_TPINFO tpinfo;

	*pFact  = 0;
	*pTrans = 0;
	*pPort  = 0;

	/*
	 * Register the transport factory for the shared memory
	 * transport configuration we require. 
	 */
	err = EMBX_RegisterTransport(EMBXSHM_mailbox_factory,
				     &config, sizeof(config), pFact);

	if(err != EMBX_SUCCESS) {
		printf("Factory registration failed err = %d\n",err);
		return -1;
	}

	/*
	 * Initialise the EMBX library, this will attempt to create
	 * a transport based on the factory we just registered.
	 */
	if((err = EMBX_Init()) != EMBX_SUCCESS) {
		printf("EMBX initialisation failed err = %d\n",err);
		return -1;
	}

	/*
	 * Try and get the information structure for the transport
	 * we hope has just been created. If this returns an error
	 * no transports were created during EMBX_Init, which means
	 * something went wrong in the transport factory we registered.
	 */
	if((err = EMBX_GetFirstTransport(&tpinfo)) != EMBX_SUCCESS) {
		printf("Querying transport failed err = %d\n",err);
		return -1;
	}

	return 0;
}

static int setupEMBXAndOS() 
{
	EMBX_FACTORY hFact;
	EMBX_TRANSPORT hTrans;
	EMBX_PORT masterPort;
	int status = 0;

	/*
	 * Start OS21
	 */
	kernel_initialize(NULL);
	kernel_start();
	kernel_timeslice(OS21_TRUE);

	if(init_mailboxes() < 0) { 
		printf("Failed to initialize mailboxes\n");
		status = 1;
		goto exit;
	}

	if(init_comms(&hFact,&hTrans,&masterPort) < 0) {
		printf("Failed to initialize embx comms\n");
		status = 1;
	}
 exit:
	return status;
}


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

static MME_ERROR simpleMix(MME_TransformerHandle_t hdl[2])
{
	MME_ERROR err;
	MME_DataBuffer_t *bufs[2][3] = {{ 0, 0, 0 }, { 0, 0, 0 }};
	MME_Command_t cmd[2] = {{ sizeof(MME_Command_t) }, { sizeof(MME_Command_t) }};

	/* allocate databuffers using MME_ALLOCATION_PHYSICAL to guarantee
	 * a single scatter page per allocation */

	err = MME_AllocDataBuffer(hdl[0], sizeof(sample1), MME_ALLOCATION_PHYSICAL, &bufs[0][0]);
	if (MME_SUCCESS != err) goto error_recovery;
	memcpy(bufs[0][0]->ScatterPages_p->Page_p, sample1, sizeof(sample1));

	err = MME_AllocDataBuffer(hdl[0], sizeof(sample2), MME_ALLOCATION_PHYSICAL, &bufs[0][1]);
	if (MME_SUCCESS != err) goto error_recovery;
	memcpy(bufs[0][1]->ScatterPages_p->Page_p, sample2, sizeof(sample2));

	err = MME_AllocDataBuffer(hdl[0], sizeof(sample1), MME_ALLOCATION_PHYSICAL, &bufs[0][2]);
	if (MME_SUCCESS != err) goto error_recovery;

	/* fill in the non-zero parts of the command structure and issue the command.
	 * 
	 * Note that setting constant due times will cause transforms to happen in 
	 * strict FIFO order within a transformer (and round-robin within multiple 
	 * transformers) 
	 */
	cmd[0].CmdCode = MME_TRANSFORM;
	cmd[0].CmdEnd = MME_COMMAND_END_RETURN_NOTIFY;
	cmd[0].DueTime = 0;
	cmd[0].NumberInputBuffers = 2;
	cmd[0].NumberOutputBuffers = 1;
	cmd[0].DataBuffers_p = &bufs[0][0];

	/* Now allocate for the second transformer on the second companion */

	err = MME_AllocDataBuffer(hdl[1], sizeof(sample1), MME_ALLOCATION_PHYSICAL, &bufs[1][0]);
	if (MME_SUCCESS != err) goto error_recovery;
	memcpy(bufs[1][0]->ScatterPages_p->Page_p, sample1, sizeof(sample1));

	err = MME_AllocDataBuffer(hdl[1], sizeof(sample2), MME_ALLOCATION_PHYSICAL, &bufs[1][1]);
	if (MME_SUCCESS != err) goto error_recovery;
	memcpy(bufs[1][1]->ScatterPages_p->Page_p, sample2, sizeof(sample2));

	err = MME_AllocDataBuffer(hdl[1], sizeof(sample1), MME_ALLOCATION_PHYSICAL, &bufs[1][2]);
	if (MME_SUCCESS != err) goto error_recovery;

	cmd[1].CmdCode = MME_TRANSFORM;
	cmd[1].CmdEnd = MME_COMMAND_END_RETURN_NOTIFY;
	cmd[1].DueTime = 0;
	cmd[1].NumberInputBuffers = 2;
	cmd[1].NumberOutputBuffers = 1;
	cmd[1].DataBuffers_p = &bufs[1][0];

	/* Submit a transform to the 1st transformer on 1st companion */
	err = MME_SendCommand(hdl[0], &cmd[0]);
	if (MME_SUCCESS != err) goto error_recovery;

	/* Submit a transform to the 2nd transformer on 2nd companion */
	err = MME_SendCommand(hdl[1], &cmd[1]);
	if (MME_SUCCESS != err) goto error_recovery;

	OS_SIGNAL_WAIT(&callbackSignal);
	OS_SIGNAL_WAIT(&callbackSignal);

	/* in normal use we would send many more commands before returning (at which
	 * point the transformer will be terminated.
	 */

	/* show the 1st mixed waveform */
	printf("Output from 1st mixer\n");
	printSignal(bufs[0][2]->ScatterPages_p->Page_p, sizeof(sample1));

	/* show the 2nd mixed waveform */
	printf("Output from 2nd mixer\n");
	printSignal(bufs[1][2]->ScatterPages_p->Page_p, sizeof(sample1));
	
	/*FALLTHRU*/
    error_recovery:

	if (bufs[0][0]) (void) MME_FreeDataBuffer(bufs[0][0]);
	if (bufs[0][1]) (void) MME_FreeDataBuffer(bufs[0][1]);
	if (bufs[0][2]) (void) MME_FreeDataBuffer(bufs[0][2]);
	if (bufs[1][0]) (void) MME_FreeDataBuffer(bufs[1][0]);
	if (bufs[1][1]) (void) MME_FreeDataBuffer(bufs[1][1]);
	if (bufs[1][2]) (void) MME_FreeDataBuffer(bufs[1][2]);

	return err;
}

int main(void)
{
	MME_TransformerInitParams_t initParams[2] = {{ sizeof(MME_TransformerInitParams_t) },
                                                     { sizeof(MME_TransformerInitParams_t) }};
	MixerParamsInit_t mixerInit[2];
	MME_TransformerHandle_t remoteHdl[2];

	MME_ERROR err;

	/* initialize the underlying EMBX transport */
	setupEMBXAndOS();

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
	initParams[0].Priority = MME_PRIORITY_NORMAL;
	initParams[0].Callback = mixerCallback;
	initParams[0].TransformerInitParamsSize = sizeof(MixerParamsInit_t);
	initParams[0].TransformerInitParams_p = &mixerInit[0];
	MME_INDEXED_PARAM(mixerInit[0], MixerParamsInit_volume, 0) = 1.0;
	MME_INDEXED_PARAM(mixerInit[0], MixerParamsInit_volume, 1) = 4.0/3.0;

	initParams[1].Priority = MME_PRIORITY_NORMAL;
	initParams[1].Callback = mixerCallback;
	initParams[1].TransformerInitParamsSize = sizeof(MixerParamsInit_t);
	initParams[1].TransformerInitParams_p = &mixerInit[1];
	MME_INDEXED_PARAM(mixerInit[1], MixerParamsInit_volume, 0) = 0.4;
	MME_INDEXED_PARAM(mixerInit[1], MixerParamsInit_volume, 1) = 0.7;
	
#if !(defined(__LINUX__) && !defined(__KERNEL__))
	/* register the transport for the remote transformer (at this point we will
	 * block until the remote processor initializes itself)
	 */
	printf("Trying to register transport shm\n");

	err = MME_RegisterTransport("shm");
	if (MME_SUCCESS != err) {
		printf("ERROR: cannot register transport to external processors (%d)\n", err);
	}
#endif

	/* repeatly attempt to connect to the remote transformer until it is registered or
	 * a serious error occurs.
	 */
	do {
		task_delay(time_ticks_per_sec() * 2);
		printf("Trying to init mme.8bit_mix_1\n");
		err = MME_InitTransformer("mme.8bit_mixer_1", &initParams[0], &remoteHdl[0]);
	} while (err == MME_UNKNOWN_TRANSFORMER);

	if (MME_SUCCESS != err) {
		printf("ERROR: cannot instantiate mme.8bit_mixer_1 (%d)\n", err);
	}

	do {
		task_delay(time_ticks_per_sec() * 2);
		printf("Trying to init mme.8bit_mix_2\n");
		err = MME_InitTransformer("mme.8bit_mixer_2", &initParams[1], &remoteHdl[1]);
	} while (err == MME_UNKNOWN_TRANSFORMER);

	if (MME_SUCCESS != err) {
		printf("ERROR: cannot instantiate mme.8bit_mixer_2 (%d)\n", err);
	}

	/* display the waveforms that will be mixed */
	printf("ORIGINAL WAVEFORMS\n"
	       "------------------\n\n");
	printTwinSignal(sample1, sample2, sizeof(sample1));

	printf("\nSIMPLE MIXING USING TWO REMOTELY REGISTERED TRANSFORMERS\n"
	         "--------------------------------------------------------\n\n");

	/* perform the mix as above using remotely registered transformer. the 
	 * results should be identical assuming both processors implement IEEE single
	 * precision floating point maths.
	 */
	err = simpleMix(remoteHdl);
	if (MME_SUCCESS != err) {
		printf("ERROR: cannot perform mixer transformer on mme.8bit_mixer (%d)\n", err);
	}

	/* clean up prior to shutting down */
	err = MME_TermTransformer(remoteHdl[0]);
	if (MME_SUCCESS != err) {
		printf("ERROR: cannot terminate mme.8bit_mixer (%d)\n", err);
	}

	err = MME_TermTransformer(remoteHdl[1]);
	if (MME_SUCCESS != err) {
		printf("ERROR: cannot terminate mme.8bit_mixer (%d)\n", err);
	}

	err = MME_Term();
	if (MME_SUCCESS != err) {
		printf("ERROR: cannot terminate the MME API (%d)\n", err);
	}

	(void) EMBX_Deinit();

	return 0;
}
