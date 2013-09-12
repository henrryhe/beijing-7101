/*
 * board_mb376.h
 *
 * Copyright (C) STMicroelectronics Limited 2003. All rights reserved.
 *
 * Code to initialize EMBX2 on STi5528/MB376.
 */


#if defined(__ST20__)
static void embxPlatformInit(void)
{
        EMBXSHM_MailboxConfig_t config = { 
		"shm",          /* name */
		1,              /* cpuID */
		{ 1, 1, 0, 0 }, /* participants */
		0               /* pointerWarp */
        };

	EMBX_ERROR err;
        EMBX_FACTORY hFactory;

	err = EMBX_Mailbox_Init();
	assert (EMBX_SUCCESS == err); 


	err = EMBX_Mailbox_Register((void *) 0x19160000, 35, 7,
			EMBX_MAILBOX_FLAGS_ST20);
	assert (EMBX_SUCCESS == err); 


	err = EMBX_RegisterTransport(EMBXSHM_mailbox_factory,
			&config, sizeof(config), &hFactory);
	assert (EMBX_SUCCESS == err); 


	err = EMBX_Init();
	assert (EMBX_SUCCESS == err);
}
#endif /* __ST20__ */

#if defined(__SH4__)

#if defined(__OS21__)
#include <os21/st40/sti5528.h>

static void embxPlatformInit(void)
{
        EMBXSHM_MailboxConfig_t config = { 
		"shm",          /* name */
		0,              /* cpuID */
		{ 1, 1, 0, 0 }, /* participants */
		0x60000000,     /* pointerWarp */
		0,              /* maxPorts */
		16,             /* maxObjects */
		16,             /* freeListSize */
		0,              /* sharedAddr */
		(2*1024*1024)   /* sharedSize */
	};

        EMBX_ERROR err;
        EMBX_FACTORY hFactory;

        err = EMBX_Mailbox_Init();
	assert (EMBX_SUCCESS == err); 


        EMBX_Mailbox_Register((void *) 0xb9160000, OS21_INTERRUPT_MBOX, 0, 
			EMBX_MAILBOX_FLAGS_ST40);
	assert (EMBX_SUCCESS == err); 


        /* register the transport */
        err = EMBX_RegisterTransport( EMBXSHM_mailbox_factory,
                        &config, sizeof(config), &hFactory);
	assert (EMBX_SUCCESS == err); 


	err = EMBX_Init();
	assert (EMBX_SUCCESS == err);
}

#elif defined (__LINUX__)

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
