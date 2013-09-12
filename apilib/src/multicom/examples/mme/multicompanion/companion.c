/*
 * companion.c
 *
 * Copyright (C) STMicroelectronics Limited 2004. All rights reserved.
 *
 * Remote component of the getstart example.
 */

#include <os21.h>
#include <os21/st200.h>
#include <os21/st200/stm8000_audioenc.h>

#include <assert.h>

#include <embx.h>
#include <embxshm.h>
#include <embxmailbox.h>
#include <mme.h>
#include "mixer.h"

/*
 * The ST220 core being used can be determined at runtime by
 * examining the peripheral base core register. 
 */
#define VIDEOENC_PBASE 0x14100000
#define AUDIOENC_PBASE 0x14200000
#define AUDIODEC_PBASE 0x18000000

static volatile unsigned long *pbasereg = (volatile unsigned long *)0xffffffb0;


EMBXSHM_MailboxConfig_t config = {
   "shm",             /* Local transport name                                */
   0,                 /* CPU id, determined at runtime                       */
   { 1, 1, 1, 0 },    /* The participants map must be the same on all CPUs   */
   0x00000000         /* No pointer warp as ST220 addresses are bus addresses*/
                      /* in this setup.                                      */
};


char *cpu_names[] = { "videoenc", "audioenc","audiodec" };

#define CPU_NAME (config.cpuID==0)?"videoenc": \
                 (config.cpuID==1)?"audioenc": \
                 (config.cpuID==2)?"audiodec":"Invalid CPU"

int init_mailboxes(void)
{
	EMBX_ERROR err;

	if((err = EMBX_Mailbox_Init()) != EMBX_SUCCESS)	{
		printf("Failed to intialise mailbox library err = %d\n", err);
		return -1;
	}

	/*
	 * Register the first hardware mailbox on the STm8000. This will be
	 * used to signal the ST40
	 */
	if((err = EMBX_Mailbox_Register((void *)0x10200000, -1, -1, 0)) != EMBX_SUCCESS) {
		printf("Failed to register mbox 0 err = %d\n",err);
		return -1;
	}

	/*
	 * Register the second hardware mailbox on the STm8000. The audio encoder
	 * will own the first half of the register set and the audio decoder will
	 * own the second half of the register set. The interrupt level parameter
	 * is ignored when using OS21.
	 */ 
	err = EMBX_Mailbox_Register((void *)0x10210000,
				    (config.cpuID==1)?OS21_INTERRUPT_MBOX2_0:OS21_INTERRUPT_MBOX2_1,
				    0,
				    (config.cpuID==1)?EMBX_MAILBOX_FLAGS_SET1:EMBX_MAILBOX_FLAGS_SET2);

	if(err != EMBX_SUCCESS)	{
		printf("Failed to register mbox 1 err = %d\n",err);
		return -1;
	}

	printf("Initialized mailbox library\n");

	return 0;
}



int init_comms(EMBX_FACTORY *pFact, EMBX_TRANSPORT *pTrans, EMBX_PORT *pPort)
{
	EMBX_ERROR err;
	EMBX_TPINFO tpinfo;

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
	if((err = EMBX_Init()) != EMBX_SUCCESS)	{
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
		printf("Transport Factory failed err = %d\n",err);
		return -1;
	}

	return 0;
}

static int setupEMBXAndOS() 
{
	EMBX_FACTORY   hFact;
	EMBX_TRANSPORT hTrans;
	EMBX_PORT      myPort;
	int status = 0;

	/*
	 * Start OS21
	 */
	kernel_initialize(NULL);
	kernel_start();
	kernel_timeslice(OS21_TRUE);

	/*
	 * Determine which ST220 we are running on
	 */
	switch(*pbasereg) {
	case VIDEOENC_PBASE:
		printf("This example does not run on the video encoder\n");
		return 1;
	case AUDIOENC_PBASE:
		printf("On the audio encoder CPU\n");
		config.cpuID = 1;
		break;
	case AUDIODEC_PBASE:
		printf("On the audio decoder CPU\n");
		config.cpuID = 2;
		break;
	}

	if(init_mailboxes() < 0) {
		status = 1;
		goto exit;
	}

	if(init_comms(&hFact, &hTrans, &myPort) < 0) {
		status = 1;
		goto exit;
	}
 exit:
	return status;
}

#define TRANSFORMER_BASENAME "mme.8bit_mixer_"

int main(int argc, char* argv[])
{
	MME_ERROR err;
	char transformerName[MME_MAX_TRANSFORMER_NAME] = "mme.8bit_mixer_";

	/* initialize the underlying EMBX transport */
	setupEMBXAndOS();

	/* initialize the MME API */
	err = MME_Init();
	if (MME_SUCCESS != err) {
		printf("%s: ERROR: cannot initialize the MME API (%d)\n", CPU_NAME, err);
	}
	printf("%s: About to register transport shm\n", CPU_NAME);
	err = MME_RegisterTransport("shm");
	if (MME_SUCCESS != err) {
		printf("%s: ERROR: cannot register transport to external processors (%d)\n", CPU_NAME, err);
	}

	sprintf(transformerName, TRANSFORMER_BASENAME "%d", config.cpuID );

	printf("%s: About to register transformer %s\n", CPU_NAME, transformerName);

	err = Mixer_RegisterTransformer(transformerName);

	if (MME_SUCCESS != err) {
		printf("%s: ERROR: cannot register %s (%d)\n", CPU_NAME, transformerName, err);	}

        /* enter the main MME execution loop */
	err = MME_Run();
	if (MME_SUCCESS != err) {
		printf("%s: ERROR: MME execution loop reported an error (%d)\n", CPU_NAME, err);
	}

	err = MME_Term();
	if (MME_SUCCESS != err && MME_DRIVER_NOT_INITIALIZED != err) {
		printf("%s: ERROR: cannot terminate the MME API (%d)\n", CPU_NAME, err);
	}

	(void) EMBX_Deinit();

	return 0;
}
