/*
 * board_mb379.h
 *
 * Copyright (C) STMicroelectronics Limited 2003. All rights reserved.
 *
 * Code to initialize EMBX2 on an MB379.
 */


#if defined(__SH4__)

#if defined(__OS21__)
#include <os21/st40/stm8000.h>

static void embxPlatformInit(void)
{
	EMBXSHM_MailboxConfig_t config = {
		"shm",			/* name */
		0,			/* cpuID */
		{ 1, 1, 0, 0 },		/* participants */
		0x60000000,		/* pointerWarp */
		0,			/* maxPorts */
		16,			/* maxObjects */
		16,			/* freeListSize */
		0,			/* sharedAddr */
		2 * 1024 * 1024		/* sharedSize */
	};

        EMBX_ERROR err;
        EMBX_FACTORY hFactory;

        err = EMBX_Mailbox_Init();
	assert (EMBX_SUCCESS == err); 


        EMBX_Mailbox_Register((void *) 0xb0200000, OS21_INTERRUPT_MBOX1_0, 0, 
			EMBX_MAILBOX_FLAGS_SET1);
	assert (EMBX_SUCCESS == err); 
        EMBX_Mailbox_Register((void *) 0xb0210000, -1, -1, 0);
	assert (EMBX_SUCCESS == err); 


        /* register the transport */
        err = EMBX_RegisterTransport( EMBXSHM_mailbox_factory,
                        &config, sizeof(config), &hFactory);
	assert (EMBX_SUCCESS == err); 


	err = EMBX_Init();
	assert (EMBX_SUCCESS == err); 
}
#elif defined(__LINUX__)

static void embxPlatformInit(void)
{
	/* on Linux EMBX and the RPC uServer are setup using module
	 * parameters
	 */
}

#else
#error Unsupported operating system
#endif /* __OS21__ */

#endif /* __SH4__ */


#if defined(__ST200__)

#if defined(__OS21__)
#include <os21/st200/stm8000_audioenc.h>
#endif

static void embxPlatformInit(void)
{
	EMBXSHM_MailboxConfig_t config = {
		{ 's', 'h', 'm' },	/* name */
		1,			/* cpuID */
		{ 1, 1, 0, 0 },		/* participants */
		0,			/* pointerWarp */
	};

        EMBX_ERROR err;
        EMBX_FACTORY hFactory;

        err = EMBX_Mailbox_Init();
	assert (EMBX_SUCCESS == err); 


        EMBX_Mailbox_Register((void *) 0x10200000, -1, -1, 0);
	assert (EMBX_SUCCESS == err); 
        EMBX_Mailbox_Register((void *) 0x10210000, OS21_INTERRUPT_MBOX2_0, 0, 
			EMBX_MAILBOX_FLAGS_SET1);
	assert (EMBX_SUCCESS == err); 


        /* register the transport */
        err = EMBX_RegisterTransport( EMBXSHM_mailbox_factory,
                        &config, sizeof(config), &hFactory);
	assert (EMBX_SUCCESS == err); 


	err = EMBX_Init();
	assert (EMBX_SUCCESS == err);
}
#endif /* __ST200__ */
