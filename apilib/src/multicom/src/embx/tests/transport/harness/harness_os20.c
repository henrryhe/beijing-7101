/*
 * harness_os20.c
 *
 * Copyright (C) STMicroelectronics Limited 2002. All rights reserved.
 *
 * 
 */

#ifdef __OS20__

#include <cache.h>
#include <interrup.h>
#include <kernel.h>
#include <task.h>
#include <ostime.h>

#include "harness.h"

#include "embxshm.h"
#include "embxmailbox.h"

#ifdef MB385
#define TICKS_PER_SECOND 3773000
#else
#define TICKS_PER_SECOND 15625
#endif


static void harness_initialize(void);

char *transportName = "shm";

char intStack[16*1024];


void harness_boot(void)
{
	int err;

	printf("******** Booting "CPU" ********\n");

	/* these is a set of requirements of the interrupt system for
	 * the factory function to work correctly
	 */
	err = interrupt_init(7, intStack, sizeof(intStack), interrupt_trigger_mode_high_level, 0);
	assert(0 == err);
	interrupt_enable_global();

#if 1 == __CORE__
{
	extern void c1_timer_initialize(void);
	printf("******** Initializing ST20-C1 timer ********\n");
	c1_timer_initialize();
}
#endif

        /* setup the instruction cache (on most devices we expect
         * cache_config_instruction() to fail so we ignore return code
         */
        cache_config_instruction((void*) 0x80000000, (void*) 0x7fffffff,
                                 cache_config_enable);
        err = cache_enable_instruction();
        assert(0 == err);

        /* cache the whole of memory - harness_initialize is responsible
	 * for disabling the cache for shared memory
	 */
        err = cache_config_data((void*) 0x80000000, (void*) 0x7fffffff,
                                cache_config_enable);
        assert(0 == err);

	/* initialize any board specific components and register transports */
	harness_initialize();

	/* we can not safely enable the data cache */
        err = cache_enable_data();
        assert(0 == err);

	VERBOSE(printf(MACHINE"EMBX_Init()\n"));
	err = EMBX_Init();
	assert(EMBX_SUCCESS == err);

	printf("******** "CPU" booted OS20 %s ********\n", kernel_version());
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
#if 1 == __CORE__
	return TICKS_PER_SECOND;
#else
	return (TICKS_PER_SECOND * 64);
#endif
}

/* use the high priority timer since it has a finer resolution */
unsigned int harness_getTime(void)
{
	unsigned int t;
#if __CORE__ == 1
	t = time_now();
#else
	__optasm { ldc 0; ldclock; st t; };
#endif
	return t;
}

void harness_sleep(int seconds)
{
	task_delay(TICKS_PER_SECOND * seconds);
}

#ifdef MB376
static void harness_initialize(void)
{
	int res;
	EMBX_FACTORY hFactory;

	EMBXSHM_MailboxConfig_t config = { 
		"shm",		/* name */
		1,		/* cpuID */
		{ 1, 1, 0, 0 },	/* participants */
#ifdef ENABLE_EMBXSHMC
                0x44000000      /* pointerWarp */
#else
                0               /* pointerWarp */
#endif
	};

	/* register the mailbox with embxmailbox */
	EMBX(Mailbox_Init());
	EMBX(Mailbox_Register((void *) 0x19160000, 35, 7, EMBX_MAILBOX_FLAGS_ST20 |
	                                                  EMBX_MAILBOX_FLAGS_LOCKS));

	/* register the transport */
	res = EMBX_RegisterTransport(
			EMBXSHM_mailbox_factory,
			&config, sizeof(config),
			&hFactory);
	assert(EMBX_SUCCESS == res);
}
#endif /* MB376 */

#ifdef MB385
static void harness_initialize(void)
{
	int res;
	EMBX_FACTORY hFactory;

	EMBXSHM_MailboxConfig_t config = { 
		"shm",			/* name */
                0,			/* cpuID */
                { 1, 1, 0, 0 },         /* participants */
		0,			/* pointerWarp */
                0,                      /* maxPorts */
                16,                     /* maxObjects */
                16,                     /* freeListSize */
                (void *) 0xc0000000,    /* sharedAddr */
                (2*1024*1024)           /* sharedSize */
	};

        /* make sure the shared memory is not cached */
	res = cache_config_data((void *) config.sharedAddr,
	                        (void *) ((char *) config.sharedAddr + config.sharedSize),
				cache_config_disable);
        assert(0 == res);

	/* register the mailbox with embxmailbox */
	EMBX(Mailbox_Init());
	EMBX(Mailbox_Register((void *) 0x18404800, 44, 7, EMBX_MAILBOX_FLAGS_ST20));

	/* register the transport */
	EMBX(RegisterTransport(EMBXSHM_mailbox_factory, &config, sizeof(config), &hFactory));
}
#endif

#ifdef MEDIAREF
static void harness_initialize(void)
{
	int res;
	EMBX_FACTORY hFactory;

	/* this configuration forms a regression test for INSbl20113,
	 * ensuring that discontiguous participant maps work correctly
	 */
	EMBXSHM_EMPIMailboxConfig_t config = { 
		{
			"shm",		/* name */
			2,		/* cpuID */
			{ 1, 0, 1, 0 },	/* participants */
			0		/* pointerWarp */
		},
		(void *) 0x7b130000	/* empiAddr */
	};

        /* make sure the shared memory is not cached */
        res = cache_config_data((void*) 0x60000000, (void*) 0x7fffffff,
                                cache_config_disable);
        assert(0 == res);

	/* register the mailbox with embxmailbox */
	EMBX(Mailbox_Init());
	EMBX(Mailbox_Register((void *) 0x7b150000, 48, 7, EMBX_MAILBOX_FLAGS_ST20 |
	                                                  EMBX_MAILBOX_FLAGS_PASSIVE));

	/* register the transport */
	res = EMBX_RegisterTransport(
			EMBXSHM_empi_mailbox_factory,
			&config, sizeof(config),
			&hFactory);
	assert(EMBX_SUCCESS == res);
}
#endif /* MEDIAREF */

#ifndef HAS_DEINIT
void harness_deinit(void) {}
#endif

#else /* __OS20__ */

extern void warning_suppression(void);

#endif /* __OS20__ */
