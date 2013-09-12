/*
 * harness_os21.c
 *
 * Copyright (C) STMicroelectronics Limited 2002. All rights reserved.
 *
 * 
 */

#ifdef __OS21__

#include <assert.h>

#include <os21.h>

#if defined __SH4__
#include <os21/st40.h>
#endif

#if defined __ST200__
#include <os21/st200.h>
#endif

#include <embx.h>

#include "harness.h"

static void harness_initialize(void);

static char *transportName = "shm";

#if defined SIM231
extern int (*_sys_custom_sysconf)(int name);
static int (*old_sys_custom_sysconf)(int name);
static int harness_sys_custom_sysconf(int name)
{
	int res;
	
	/* intercept request for the bus frequency (in hz) */
	if (1001 == name) {
		return 100 * 1000;
	}

	res = old_sys_custom_sysconf(name);
	printf(MACHINE "%d = _sys_custom_sysconf(%d)\n", res, name);
	return res;
}
#endif

void harness_boot(void)
{
	/* cannot use printf before the kernel is initialized since this
	 * is routed through harness_printf which does a task_lock()
	 */
	fprintf(stdout, "******** Booting "MACHINE" ********\n");

#if defined SIM231
	old_sys_custom_sysconf = _sys_custom_sysconf;
	_sys_custom_sysconf = harness_sys_custom_sysconf;
#endif

	/* boot the kernel */
	kernel_initialize(NULL);
	kernel_start();
	kernel_timeslice(OS21_TRUE);

	/* initialize any board specific components and register transports */
	harness_initialize();

#ifdef __STRPC__
#ifdef ENABLE_RPC_SERVER
	rpcServerInit();
#else
	EMBX(Init());
	rpcStubsInit(NULL);
#endif
#else
	EMBX(Init());
#endif
	errno = 0;

	printf("******** "MACHINE" booted OS21 %s ********\n", kernel_version());
}

char *harness_getTransportName(void)
{
	return transportName;
}

void harness_block(void)
{
	task_suspend(NULL);
}

unsigned int harness_getTicksPerSecond(void)
{
        return time_ticks_per_sec();
}

/* use the high priority timer since it has a finer resolution */
unsigned int harness_getTime(void)
{
	/* converts a 64-bit to 32-bit type but this is OK because
	 * the tests will not run for long enough for this to roll
	 * over
	 */
	return (unsigned int) time_now();
}

void harness_sleep(int seconds)
{
	osclock_t t;

	t = time_plus(time_now(), time_ticks_per_sec() * seconds);
	VERBOSE(printf(MACHINE "sleeping until %lld\n", t));
	task_delay_until(t);
}



#include <embxmailbox.h>

#if defined MB376
#include <embxshm.h>
#if defined __SH4__
#include <os21/st40/sti5528.h>
static void harness_initialize(void)
{
	int res;
	EMBX_FACTORY hFactory;

	EMBXSHM_MailboxConfig_t config = { 
		"shm",		/* name */
		0,		/* cpuID */
		{ 1, 1, 0, 0 },	/* participants */
#if defined ENABLE_EMBXSHMC
		0x80000000,	/* pointerWarp */
#else
		0x60000000,	/* pointerWarp */
#endif
		0,		/* maxPorts */
		16,		/* maxObjects */
		16,		/* freeListSize */
		NULL,		/* sharedAddr */
		(2*1024*1024)	/* sharedSize */
	};

	EMBX(Mailbox_Init());
	EMBX(Mailbox_Register((void *) 0xb9160000, OS21_INTERRUPT_MBOX, 0, EMBX_MAILBOX_FLAGS_ST40 | EMBX_MAILBOX_FLAGS_LOCKS));

	/* register the transport */
	res = EMBX_RegisterTransport(
			EMBXSHM_mailbox_factory,
			&config, sizeof(config),
			&hFactory);
	assert(EMBX_SUCCESS == res);
}
#endif /* __SH4__ */
#endif /* MB376 */


#if defined MEDIAREF
#include <embxshm.h>
#if defined __SH4__ 
#include <os21/st40/st40gx1.h>
static void harness_initialize(void)
{
	int res;
	EMBX_FACTORY hFactory;

	EMBXSHM_EMPIMailboxConfig_t config = { 
		{
			"shm",		/* name */
			0,		/* cpuID */
			{ 1, 1, 0, 0 },	/* participants */
			0xc0000000,	/* pointerWarp */
			0,		/* maxPorts */
			16,		/* maxObjects */
			16,		/* freeListSize */
			NULL,		/* sharedAddr */
			(2*1024*1024)	/* sharedSize */
		},
		(void *) 0xbb130000,	/* empiAddr */
		(void *) 0xbb150000,	/* mailboxAddr */
		(void *) 0xbb190000,	/* sysconfAddr */
		(1 << 19),		/* sysconfMaskSet */
		(1 << 18)		/* sysconfMaskClear */
	};

	EMBX(Mailbox_Init());
	EMBX(Mailbox_Register((void *) 0xbb150000, OS21_INTERRUPT_MBOX, 0, 
	                      EMBX_MAILBOX_FLAGS_ST40 | EMBX_MAILBOX_FLAGS_ACTIVATE));

	/* register the transport */
	res = EMBX_RegisterTransport(
			EMBXSHM_empi_mailbox_factory,
			&config, sizeof(config),
			&hFactory);
	assert(EMBX_SUCCESS == res);
}
#endif /* __SH4__ */
#endif /* MEDIAREF */

#if defined MB379 || defined MB392
#include <embxshm.h>

#if defined __SH4__
#include <os21/st40/stm8000.h>
static void harness_initialize(void)
{
	EMBX_FACTORY hFactory;
	EMBXSHM_MailboxConfig_t config = { 
		"shm",			/* name */
		0,			/* cpuID */
		{ 1, 1, 0, 0 },		/* participants */
		0x60000000,		/* pointerWarp */
		0,			/* maxPorts */
		16,			/* maxObjects */
		16,			/* freeListSize */
		(void *) 0xaa600000,	/* sharedAddr */
		2 * 1024 * 1024		/* sharedSize */
	};

	EMBX(Mailbox_Init());
	EMBX(Mailbox_Register((void *) 0xb0200000, OS21_INTERRUPT_MBOX1_0, OS21_INTERRUPT_ILC_13,
	                      EMBX_MAILBOX_FLAGS_SET1 | EMBX_MAILBOX_FLAGS_LOCKS));
	EMBX(Mailbox_Register((void *) 0xb0210000, -1, -1, EMBX_MAILBOX_FLAGS_LOCKS));
	

	EMBX(RegisterTransport(EMBXSHM_mailbox_factory,
	                       &config, sizeof(config), &hFactory));
}
#endif /* __SH4__ */

#if defined __ST200__
#include <embxshm.h>
#include <os21/st200/stm8000_videoenc.h>
static void harness_initialize(void)
{
	EMBX_FACTORY hFactory;
	EMBXSHM_MailboxConfig_t config = { 
		{ 's', 'h', 'm' },	/* name */
		1,			/* cpuID */
		{ 1, 1, 0, 0 },		/* participants */
		0,			/* pointerWarp */
	};

	EMBX(Mailbox_Init());
	EMBX(Mailbox_Register((void *) 0x10200000, -1, -1, EMBX_MAILBOX_FLAGS_LOCKS));
	EMBX(Mailbox_Register((void *) 0x10210000, OS21_INTERRUPT_MBOX2_0, 0,
	                      EMBX_MAILBOX_FLAGS_SET1 | EMBX_MAILBOX_FLAGS_LOCKS));
	
	EMBX(RegisterTransport(EMBXSHM_mailbox_factory,
	                       &config, sizeof(config), &hFactory));
}
#endif /* __ST200__ */
#endif /* MB379 or MB392 */

#if defined MB385
#include <embxshm.h>
#include <rpc_mb385.h>
static void harness_initialize(void)
{
	EMBX_FACTORY hFactory;
	EMBXSHM_MailboxConfig_t config = { 
		{ 's', 'h', 'm' },	/* name */
		1,			/* cpuID */
		{ 1, 1, 0, 0 },		/* participants */
		0,			/* pointerWarp */
	};

	EMBX(Mailbox_Init());
	EMBX(Mailbox_Register((void *) 0x18404800, OS21_INTERRUPT_MBOX1_1, 0,
	                      EMBX_MAILBOX_FLAGS_ST40));

	EMBX(RegisterTransport(EMBXSHM_mailbox_factory,
	                       &config, sizeof(config), &hFactory));

	/* workaround iteration bug in dpu_config() */
	{
		protection_list_t INSbl21557 = { 0, 4, protection_attribute_enable };
		int res = dpu_config(&INSbl21557, 1, 0);
		assert(OS21_SUCCESS == res);
	}
}
#endif /* MB385 */

#if defined MB411
#include <embxshm.h>
extern interrupt_name_t OS21_INTERRUPT_MB_LX_DPHI;
extern interrupt_name_t OS21_INTERRUPT_MBOX_SH4;
#if defined __SH4__
static void harness_initialize(void)
{
	EMBX_FACTORY hFactory;
	EMBXSHM_MailboxConfig_t config = { 
		"shm",			/* name */
		0,			/* cpuID */
		{ 1, 1, 0, 0 },		/* participants */
#if defined ENABLE_EMBXSHMC
		0x80000000,		/* pointerWarp */
#else
		0x60000000,		/* pointerWarp */
#endif
		0,			/* maxPorts */
		16,			/* maxObjects */
		16,			/* freeListSize */
		(void *) 0x00000000,	/* sharedAddr */
		2 * 1024 * 1024,	/* sharedSize */
	};

	EMBX(Mailbox_Init());
	EMBX(Mailbox_Register((void *) 0xb9211000, OS21_INTERRUPT_MB_LX_DPHI, -1,
                              EMBX_MAILBOX_FLAGS_SET2 | EMBX_MAILBOX_FLAGS_LOCKS));
        EMBX(Mailbox_Register((void *) 0xb9212000, -1, -1, EMBX_MAILBOX_FLAGS_LOCKS));

	EMBX(RegisterTransport(EMBXSHM_mailbox_factory,
	                       &config, sizeof(config), &hFactory));
}
#endif /* __SH4__ */
#if defined __ST231__
static void harness_initialize(void)
{
	EMBX_FACTORY hFactory;
	EMBXSHM_MailboxConfig_t config = { 
		{ "shm" },		/* name */
		1,			/* cpuID */
		{ 1, 1, 0, 0 },		/* participants */
		0 			/* pointerWarp */
	};

	EMBX(Mailbox_Init());
	EMBX(Mailbox_Register((void *) 0x19211000, -1, -1, EMBX_MAILBOX_FLAGS_LOCKS));
	EMBX(Mailbox_Register((void *) 0x19212000, OS21_INTERRUPT_MBOX_SH4, -1,
	                      EMBX_MAILBOX_FLAGS_SET1 | EMBX_MAILBOX_FLAGS_LOCKS));

	EMBX(RegisterTransport(EMBXSHM_mailbox_factory,
	                       &config, sizeof(config), &hFactory));
}
#endif /* __ST231__ */
#endif /* MB411 */

#ifdef MB428
static void harness_initialize(void)
{
	extern EMBX_ERROR STb5525_RegisterTransport(const char *, EMBX_FACTORY *);
	EMBX_ERROR err;
	EMBX_FACTORY hFactory;

	err = STb5525_RegisterTransport("shm", &hFactory);
	assert(EMBX_SUCCESS == err);
}
#endif /* MB428 */

#if defined SIM231
#include <embxshm.h>
extern interrupt_name_t OS21_INTERRUPT_TEST;

static void harness_initialize(void)
{
	EMBX_FACTORY hFactory;
	EMBXSHM_MailboxConfig_t config = {
#ifdef CONF_MASTER
                "shm",                  /* name */
                0,                      /* cpuID */
                { 1, 1, 0, 0 },         /* participants */
                0,                      /* pointerWarp */
                0,                      /* maxPorts */
                16,                     /* maxObjects */
                16,                     /* freeListSize */
                0,                      /* sharedAddr */
                2 * 1024 * 1024         /* sharedSize */
#else
                "shm",                  /* name */
                1,                      /* cpuID */
                { 1, 1, 0, 0 },         /* participants */
                0                       /* pointerWarp */
#endif
        };

	EMBX(Mailbox_Init());
#ifdef CONF_MASTER
	EMBX(Mailbox_Register((void *) 0x19210000, OS21_INTERRUPT_TEST, 0, EMBX_MAILBOX_FLAGS_SET1));
#else
	EMBX(Mailbox_Register((void *) 0x19210000, OS21_INTERRUPT_TEST, 0, EMBX_MAILBOX_FLAGS_SET2));
#endif

	EMBX(RegisterTransport(EMBXSHM_mailbox_factory,
	                       &config, sizeof(config), &hFactory));
}

#endif /* SIM231 */

#if defined DB547
#include <embxdma.h>

#if defined __SH4__
#include <pci_driver.h>
#include <db547b.h>
static void harness_initialize(void)
{
	int res;

	EMBXDMA_PCI_Host_Config_t config = {
		"shm",          /* transport name */
		1,		/* DMA channel to use */
		0,		/* max num. object handler, 0 implies default */
		4*1024*1024,	/* heap size */
		0		/* card number of first PCI card */
	};
	EMBX_FACTORY hFactory;

	/* initialize the boards PCI */
	pci_initialize();
	res = db547b_init_driver();
	assert(0 == res);
	
	/* register the transport */
	res = EMBX_RegisterTransport(
			EMBXDMA_pci_host_for_db547b_factory,
			&config, sizeof(config),
			&hFactory);
	assert(EMBX_SUCCESS == res);
}
#endif /* __SH4__ */

#if defined __ST200__
#include <db547b_support.h>
static void harness_initialize(void)
{
	int res;

	EMBXDMA_DB547B_PCI_Config_t config;
	EMBX_FACTORY hFactory;

	strcpy(config.name, transportName);
	config.dmaChanNum = 1;
	config.maxObjects = 0;
	config.heapSize   = 32*1024*1024;
	config.heapRegion = (EMBX_VOID *) 0x0c000000;

	/* initialize the boards PCI */
	res = db547b_initialize(config.heapSize);
	assert(0 == res);
	res = db547b_sync_to_host();
	assert(0 == res);

	/* register the transport */
	res = EMBX_RegisterTransport(
			EMBXDMA_db547b_pci_factory,
			&config, sizeof(config),
			&hFactory);
	assert(EMBX_SUCCESS == res);
}
#endif /* __ST200__ */

#endif /* DB547 */

#else /* __OS21__ */

extern void warning_suppression(void);

#endif /* __OS21__ */
