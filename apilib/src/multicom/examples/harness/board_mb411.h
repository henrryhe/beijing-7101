/*
 * board_mb411.h
 *
 * Copyright (C) STMicroelectronics Limited 2003. All rights reserved.
 *
 * Code to initialize EMBX2 on an MB411.
 */

#if defined(__SH4__)

#if defined(__OS21__)
#include <os21/st40/stb7100.h>

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
	assert (EMBX_SUCCESS == err || EMBX_ALREADY_INITIALIZED == err); 

	if (err != EMBX_ALREADY_INITIALIZED) {
		err = EMBX_Mailbox_Register((void *) 0xb9211000, OS21_INTERRUPT_MB_LX_DPHI, 0, 
				EMBX_MAILBOX_FLAGS_SET2);
		assert (EMBX_SUCCESS == err); 

		err = EMBX_Mailbox_Register((void *) 0xb9212000, OS21_INTERRUPT_MB_LX_AUDIO, 0, 
				EMBX_MAILBOX_FLAGS_SET2);
		assert (EMBX_SUCCESS == err); 

		/* register the transport */
		err = EMBX_RegisterTransport( EMBXSHM_mailbox_factory,
				              &config, sizeof(config), &hFactory);
		assert (EMBX_SUCCESS == err); 
	}



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

#elif defined(__WINCE__)
#include <nkintr.h>
#include <SH4202T_intc2_irq.h>

static void embxPlatformInit(void)
{
	EMBXSHM_MailboxConfig_t config = { 
		"shm",		/* name */
		0,		/* cpuID */
		{ 1, 1, 0, 0 },	/* participants */
                0x60000000,     /* pointerWarp */
		0,		/* maxPorts */
		16,		/* maxObjects */
		16,		/* freeListSize */
		(void *) 0xa6000000, /* sharedAddr */
		2 * 1024 * 1024 /* sharedSize */
	};

        EMBX_ERROR err;
        EMBX_FACTORY hFactory;

	UINT32 irq;
	DWORD sysintr, out;

        err = EMBX_Mailbox_Init();
	assert (EMBX_SUCCESS == err); 

	/* Obtain an logical interrupt number from the hardware derived one.
	 * When this interrupt is no longer required it should be released using
	 * IOCTL_HAL_RELEASE_SYSINTR but this is not implemented because the
	 * Multicom example framework does not support de-initialization.
	 */
	irq = IRQ_ST231_DELPHI;
	sysintr = SYSINTR_NOP;
	err = KernelIoControl(IOCTL_HAL_REQUEST_SYSINTR, 
	                      &irq, sizeof(irq),
			      &sysintr, sizeof(sysintr), &out);
	assert(err && out == sizeof(sysintr));

        err = EMBX_Mailbox_Register((void *) 0xb9211000, sysintr, 0, 
			      EMBX_MAILBOX_FLAGS_SET2 | EMBX_MAILBOX_FLAGS_LOCKS);
	assert (EMBX_SUCCESS == err); 

	/* strictly speaking we don't need the audio interrupt so we'll register
	 * a mailbox as though it is not capable of interrupt us.
	 */
	err = EMBX_Mailbox_Register((void *) 0xb9212000, -1, -1, EMBX_MAILBOX_FLAGS_LOCKS);
	assert (EMBX_SUCCESS == err); 

        /* register the transport */
        err = EMBX_RegisterTransport( EMBXSHM_mailbox_factory,
                        &config, sizeof(config), &hFactory);
	assert (EMBX_SUCCESS == err); 

	err = EMBX_Init();
	assert (EMBX_SUCCESS == err); 
}

#else
#error Unsupported operating system
#endif /* __OS21__ */

#endif /* __SH4__ */


#if defined(__ST200__)

#if defined(__OS21__)
#include <os21/st200/stb7100_audio.h>
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


	err = EMBX_Mailbox_Register((void *) 0x19211000, -1, -1, 0);
	assert (EMBX_SUCCESS == err);
        err = EMBX_Mailbox_Register((void *) 0x19212000, OS21_INTERRUPT_MBOX_SH4, 0, 
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
