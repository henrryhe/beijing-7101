/*
 * harness_wince.c
 *
 * Copyright (C) STMicroelectronics Limited 2005. All rights reserved.
 *
 * 
 */

#ifdef __WINCE__

#include <windows.h>
#include <bsp.h>
#include <sh4_intc.h>
#include <nkintr.h>
#include <SH4202T_intc2_irq.h>

#include "harness.h"

#include <embx.h>
#include <embxshm.h>
#include <embxmailbox.h>

static void harness_initialize(void);

char *transportName = "shm";

void harness_boot(void)
{
	printf("******** Booting "CPU" ********\n");

	/* initialize any board specific components and register transports */
	harness_initialize();

	EMBX(Init());

	printf("******** "CPU" booted "OS" ********\n");
}

char *harness_getTransportName(void)
{
	return transportName;
}

void harness_block(void)
{
	Sleep(INFINITE);
}

unsigned int harness_getTicksPerSecond(void)
{
	/* milliseconds */
	return 1000;
}

unsigned int harness_getTime(void)
{
	return GetTickCount();
}

void harness_sleep(int seconds)
{
	Sleep(1000 * seconds);
}

#ifdef MB411
static DWORD sysintr;
static void harness_initialize(void)
{
	int res;
	EMBX_FACTORY hFactory;

	UINT32 irq;
	DWORD out;

	EMBXSHM_MailboxConfig_t config = { 
		"shm",		/* name */
		0,		/* cpuID */
		{ 1, 1, 0, 0 },	/* participants */
                0x60000000,     /* pointerWarp */
		0,		/* maxPorts */
		16,		/* maxObjects */
		16,		/* freeListSize */
		(void *) 0xa7000000, /* sharedAddr */
		2 * 1024 * 1024 /* sharedSize */
	};

	/* register the mailbox with embxmailbox */
	EMBX(Mailbox_Init());

	/* obtain a logical interrupt number from a hardware derived one. */
	irq = IRQ_ST231_DELPHI;
	sysintr = SYSINTR_NOP;
	res = KernelIoControl(IOCTL_HAL_REQUEST_SYSINTR,
			      &irq, sizeof(irq),
			      &sysintr, sizeof(sysintr), &out);
	assert(res && out == sizeof(sysintr));

	EMBX(Mailbox_Register((void *) 0xb9211000, sysintr, 0, EMBX_MAILBOX_FLAGS_SET2));

	/* register the transport */
	EMBX(RegisterTransport(EMBXSHM_mailbox_factory, &config, sizeof(config), &hFactory));
}

void harness_deinit(void)
#define HAS_DEINIT
{
	int res;
	DWORD out;

	res = KernelIoControl(IOCTL_HAL_RELEASE_SYSINTR, 
	                      &sysintr, sizeof(sysintr), NULL, 0, &out);
	assert(res);
}
#endif /* MB411 */

#ifndef HAS_DEINIT
void harness_deinit(void) {}
#endif

#else /* __WINCE__ */

extern void warning_suppression(void);

#endif /* __WINCE__ */
