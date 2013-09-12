/*
 * board_mediaref.h
 *
 * Copyright (C) STMicroelectronics Limited 2003. All rights reserved.
 *
 * Code to initialize EMBX2 on an MediaRef.
 */


#if defined(__ST20__)
static void embxPlatformInit(void)
{
        EMBXSHM_EMPIMailboxConfig_t config = { 
                {
                        "shm",          /* name */
                        1,              /* cpuID */
                        { 1, 1, 0, 0 }, /* participants */
                        0               /* pointerWarp */
                },
                (void *) 0x7b130000     /* empiAddr */
        };

	EMBX_ERROR err;
        EMBX_FACTORY hFactory;

	err = EMBX_Mailbox_Init();
	assert (EMBX_SUCCESS == err); 


	err = EMBX_Mailbox_Register((void *) 0x7b150000, 48, 7,
			EMBX_MAILBOX_FLAGS_ST20 | EMBX_MAILBOX_FLAGS_PASSIVE);
	assert (EMBX_SUCCESS == err); 


	err = EMBX_RegisterTransport(EMBXSHM_empi_mailbox_factory,
			&config, sizeof(config), &hFactory);
	assert (EMBX_SUCCESS == err); 


	err = EMBX_Init();
	assert (EMBX_SUCCESS == err); 
}
#endif /* __ST20__ */

#if defined(__SH4__)

#if defined(__OS21__)
#include <os21/st40/st40gx1.h>

static void embxPlatformInit(void)
{
        EMBXSHM_EMPIMailboxConfig_t config = { 
                {
                        "shm",          /* name */
                        0,              /* cpuID */
                        { 1, 1, 0, 0 }, /* participants */
                        0xc0000000,     /* pointerWarp */
                        0,              /* maxPorts */
                        16,             /* maxObjects */
                        16,             /* freeListSize */
                        0,              /* sharedAddr */
                        (2*1024*1024)   /* sharedSize */
                },
                (void *) 0xbb130000,    /* empiAddr */
                (void *) 0xbb150000,    /* mailboxAddr */
                (void *) 0xbb190000,    /* sysconfAddr */
                (1 << 19),              /* sysconfMaskSet */
                (1 << 18)               /* sysconfMaskClear */
        };

        EMBX_ERROR err;
        EMBX_FACTORY hFactory;

        err = EMBX_Mailbox_Init();
	assert (EMBX_SUCCESS == err); 


        EMBX_Mailbox_Register((void *) 0xbb150000, OS21_INTERRUPT_MBOX, 0, 
			EMBX_MAILBOX_FLAGS_ST40 | EMBX_MAILBOX_FLAGS_ACTIVATE);
	assert (EMBX_SUCCESS == err); 


        /* register the transport */
        err = EMBX_RegisterTransport( EMBXSHM_empi_mailbox_factory,
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
