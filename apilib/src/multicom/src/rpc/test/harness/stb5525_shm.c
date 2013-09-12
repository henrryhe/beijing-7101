/*
 * stb5525_mbox.c
 *
 * Copyright (C) STMicroelectronics Limited 2005. All rights reserved.
 *
 * Interrupt knocking code for STb5525
 */

#if defined __OS21__ && defined __ST231__

#include <assert.h>
#include <malloc.h>
#include <stdio.h>

#include <os21.h>
#include <os21/st200.h>

extern interrupt_name_t OS21_INTERRUPT_MAILBOX_0 __attribute__ ((weak));
extern interrupt_name_t OS21_INTERRUPT_MAILBOX_1 __attribute__ ((weak));

#include <embx.h>
#include <embxshm.h>

struct stb5525_ctx {
	EMBX_UINT cpuID;
	interrupt_t *handle;
	EMBXSHM_GenericConfig_InstallIsr_t interruptParameters;
};

/* macros to work out which processor we are running on */
#define IS_HOST() (0x1ef00000 == *((unsigned int *) 0xffffffb0))
#define US        (IS_HOST() ? 0 : 1)
#define THEM	  (IS_HOST() ? 1 : 0)

#define VERBOSE(x) ((void) (x))

/* macros used for ILC based interrupt knocking */
#define ILC_BASE 0x1bc00000
#define ILC_MAILBOX_CLEAR 0
#define ILC_MAILBOX_RAISE  2
#define ILC_MAILBOX_BOOTSIG 6

/* registers used for ILC based interrupt knocking */
static volatile unsigned int *ilc_mailbox[2] = { 
	(unsigned int *) (ILC_BASE + 0x800 + (79 * 8) + 4),
	(unsigned int *) (ILC_BASE + 0x800 + (80 * 8) + 4)
};

static struct stb5525_ctx STb5525_Context;

static int STb5525_InterruptHandler(void *data)
{
	struct stb5525_ctx *ctx = data;
	ctx->interruptParameters.handler(ctx->interruptParameters.param);
	return OS21_SUCCESS;
}

static EMBX_ERROR STb5525_NotifyFunction(void *data, EMBX_UINT opcode, void *param)
{
	struct stb5525_ctx *ctx = data;
	EMBXSHM_GenericConfig_InstallIsr_t *installIsr = param;
	EMBX_ERROR res = EMBX_SUCCESS;
	int err;

	switch (opcode) {
	case EMBXSHM_OPCODE_RENDEZVOUS:
		printf("Waiting for %s processor to boot ... ",
		       (IS_HOST() ? "audio" : "host"));
		fflush(stdout);

		/* the ILC trigger mode registers go to zero on reset so it is
		 * very easy to implement a rendezvous with them.
		 */
		*(ilc_mailbox[US]) = ILC_MAILBOX_BOOTSIG;	
		while (*(ilc_mailbox[THEM]) != ILC_MAILBOX_BOOTSIG) {}
		*(ilc_mailbox[THEM]) = ILC_MAILBOX_CLEAR;
		while (*(ilc_mailbox[US]) == ILC_MAILBOX_BOOTSIG) {}

		printf("done\n");
		break;
	case EMBXSHM_OPCODE_INSTALL_ISR:
		ctx->cpuID = installIsr->cpuID;
		ctx->interruptParameters = (*installIsr);

		ctx->handle = interrupt_handle(IS_HOST() ? OS21_INTERRUPT_MAILBOX_0 :
		                                           OS21_INTERRUPT_MAILBOX_1);
		assert(ctx->handle);

		err = interrupt_install_shared(ctx->handle,
					       STb5525_InterruptHandler, ctx);
		assert(0 == err);

		err = interrupt_enable(ctx->handle);
		assert(0 == err);

		/* when we installed the interrupt we potentially clobbered a
		 * pending interrupt from our partner. for this reason we assume
		 * we did clobber an interrupt and generate one just in case.
		 */
		*(ilc_mailbox[US]) = ILC_MAILBOX_RAISE;
		break;
	case EMBXSHM_OPCODE_REMOVE_ISR:
		interrupt_uninstall(ctx->handle);
		ctx->handle = 0;
		break;
	case EMBXSHM_OPCODE_RAISE_INTERRUPT:
		*(ilc_mailbox[(EMBX_UINT) param]) = ILC_MAILBOX_RAISE;
		break;
	case EMBXSHM_OPCODE_CLEAR_INTERRUPT:
		*(ilc_mailbox[US]) = ILC_MAILBOX_CLEAR;
		break;
	case EMBXSHM_OPCODE_ENABLE_INTERRUPT:
		(void) interrupt_enable(ctx->handle);
		break;
	case EMBXSHM_OPCODE_DISABLE_INTERRUPT:
		(void) interrupt_disable(ctx->handle);
		break;
	case EMBXSHM_OPCODE_BUFFER_FLUSH:
	default:
		res = EMBX_SYSTEM_ERROR;
	}
	
	return res;
}

void STb5525_ReserveSharedMemory(void *addr, unsigned int sz)
{
	static void *reserved;

	/* the memory map in the BSP does not reserve any space for the EMBX
	 * shared area but ideally we want both processors to know the address
	 * of this area at compile time. to allow this we call memalign()
	 * knowing that the result it gives us will be deterministic this
	 * early in the boot. this allows us to use a compiled in constant
	 * and 'reserve' later by causing it to be allocated.
	 */

	if (0 == reserved) {
		reserved = memalign(sz, sz);
	}

	if (reserved != addr) {
		fprintf(stderr, "ERROR: Cannot reserve memory at 0x%p "
		                "(got 0x%p instead)\n", addr, reserved);
		free(reserved);
		reserved = 0;
		abort();

	}
}

EMBX_ERROR STb5525_RegisterTransport(const char *name, EMBX_FACTORY *phFactory)
{
	void *physicalSharedAddr = (void *) 0x84200000;
	void *virtualSharedAddr = mmap_translate_virtual(physicalSharedAddr);
	void *uncachedSharedAddr = mmap_translate_uncached(virtualSharedAddr);

	EMBXSHM_GenericConfig_t config = {
		"shm",		/* name */
		0,			/* cpuID */
		{ 1, 1, 0, 0 },		/* participants */
		0,			/* pointerWarp */
		(void *) 0,		/* sharedAddr */
		2 * 1024 * 1024,	/* sharedSize */
		STb5525_NotifyFunction, /* interrupt knocking function */
		&STb5525_Context,	/* data structure used by above */
		0,			/* maxPorts */
		0,			/* maxObjects */
		0			/* freeListSize */
	};

	strcpy(config.name, name);
	config.sharedAddr = uncachedSharedAddr;

	if (IS_HOST()) {
		config.maxObjects = 64;
		config.freeListSize = 64;
	} else {
		config.cpuID = 1;

		STb5525_ReserveSharedMemory(physicalSharedAddr,
					    config.sharedSize);
	}
	
	return EMBX_RegisterTransport(EMBXSHM_generic_factory,
				      &config, sizeof(config), phFactory);
	
}

#else
extern void warning_suppression(void);
#endif
