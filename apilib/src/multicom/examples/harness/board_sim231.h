/*
 * board_sim231.h
 *
 * Copyright (C) STMicroelectronics Limited 2003. All rights reserved.
 *
 * Code to initialize EMBX2 on a multi-core ST231 simulator
 */

#if defined(__OS21__)

#include <os21.h>

extern interrupt_name_t OS21_INTERRUPT_TEST;

/* we will determine our configuration at runtime using our peripheral base address */
#define ST231_VIDEO (0x1a000000 == *((int *) 0xffffffb0))

#if defined(__ST231__)
static void embxPlatformInit(void)
{
	EMBXSHM_MailboxConfig_t config_master = {
		"shm",			/* name */
		0,			/* cpuID */
		{ 1, 1, 0, 0 },		/* participants */
		0,			/* pointerWarp */
		0,			/* maxPorts */
		16,			/* maxObjects */
		16,			/* freeListSize */
		0,			/* sharedAddr */
		2 * 1024 * 1024		/* sharedSize */
	};

	EMBXSHM_MailboxConfig_t config_slave = {
		"shm",			/* name */
		1,			/* cpuID */
		{ 1, 1, 0, 0 },		/* participants */
		0			/* pointerWarp */
	};

        EMBX_ERROR err;
        EMBX_FACTORY hFactory;

        err = EMBX_Mailbox_Init();
	assert (EMBX_SUCCESS == err); 

        err = EMBX_Mailbox_Register((void *) 0x19210000, OS21_INTERRUPT_TEST, 0, 
			(ST231_VIDEO ? EMBX_MAILBOX_FLAGS_SET1 : EMBX_MAILBOX_FLAGS_SET2));
	assert (EMBX_SUCCESS == err); 

        /* register the transport */
        err = EMBX_RegisterTransport( EMBXSHM_mailbox_factory,
			(ST231_VIDEO ? &config_master : &config_slave),
			sizeof(EMBXSHM_MailboxConfig_t), &hFactory);
	assert (EMBX_SUCCESS == err); 


	err = EMBX_Init();
	assert (EMBX_SUCCESS == err); 
}

#endif /* __ST231__ */

#endif /* __OS21__ */
