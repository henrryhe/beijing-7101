/*
 * mailbox.c
 *
 * Copyright (C) STMicroelectronics Limited 2005. All rights reserved.
 *
 * Implementation of the mailbox peripheral in C.
 */

/*
 * Notes:
 *
 * The simulator demonstrates its preference for C++ by exporting very few useful
 * header files for C (and filling even these with C++ comments). However this
 * particular plugin is deliberatly implemented in C so that we can compile it with
 * an arbitary version of GCC and expect it to conform to the ABI used by the 
 * simulator (which is compiled with a fairly old version of GCC).
 *
 * This plugin really doesn't care about host/target endianness problems. Since its
 * registers are never interpretted as numeric values we can simply leave all registers
 * in target-endian form.
 *
 * Being a software-only simulation this plugin uses an optimised clocking strategy and
 * is explicitly clocked after every register write. This keeps calls to DevClock() off
 * the simulators critical paths.
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#ifdef DEBUG
#define VERBOSE(x) x
#else
#define VERBOSE(x) ((void) 0)
#endif

#define DevClock OptimisedDevClock

struct SPinout {
	/* Set an interrupt (don't forget to clear before you re-enable interrupts) */
	void (*m_interrupt)(void *pinout, int interrupt);
	void (*m_interruptLine)(void *pinout, unsigned int line, int signal);

	/* there are loads more pinout functions but we don't need them for the mailbox */
};

static unsigned int s_mailboxAddress;

static struct mailbox {
	unsigned int st20_interrupt_status[4];
	unsigned int st20_interrupt_enable[4];

	unsigned int st40_interrupt_status[4];
	unsigned int st40_interrupt_enable[4];
} s_mailbox;

static struct core {
	void *initiator;
	void *pinout;
	struct SPinout *pinoutC;

} s_core[2];

void DevInitialise(void *initiator, void *pinout, struct SPinout *pinoutC, char *args)
{
	static int i = 0;
	assert(i < 2);

	VERBOSE(printf("mailbox: DevInitialize(%p, %p, %p, \"%s\")\n", 
	                initiator, pinout, (void *) pinoutC, args));

	/* store the pinout pointers for each core */
	s_core[i].initiator = initiator;
	s_core[i].pinout = pinout;
	s_core[i].pinoutC = pinoutC;
	i++;

	memset(&s_mailbox, 0, sizeof(s_mailbox));

	s_mailboxAddress = 0x19210000;
}

void DevTerminate(void *initiator)
{
	VERBOSE(printf("mailbox: DevTerminate(%p)\n", initiator));
}

void DevClock(void *initiator)
{
	int i, interrupt[2];

	/* update the interrupt state */
	interrupt[0] = interrupt[1] = 0;
	for (i=0; i<3; i++) {
		interrupt[0] |= s_mailbox.st20_interrupt_status[i] & 
		                s_mailbox.st20_interrupt_enable[i];
		interrupt[1] |= s_mailbox.st40_interrupt_status[i] & 
		                s_mailbox.st40_interrupt_enable[i];
	}

	if (s_core[0].initiator == initiator) {
		s_core[0].pinoutC->m_interruptLine(s_core[0].pinout, 63, interrupt[1]);
	} else if (s_core[1].initiator == initiator) {
		s_core[1].pinoutC->m_interruptLine(s_core[1].pinout, 63, interrupt[0]);
	}
}

int DevRead(void *initiator, unsigned char *to, unsigned int address, 
					         unsigned int numBytes)
{
	int offset, res = 1;

	if (4 != numBytes || 0 != (address&3)) {
		return 0;
	}

	offset = address - s_mailboxAddress;
	switch(offset) {
	case 0x0000: /* version */
		to[0] = to[1] = to[2] = to[3] = 0;
		break;
	case 0x0004: /* st20_interrupt_status */
	case 0x0008:
	case 0x000c:
	case 0x0010:
		memcpy(to, &s_mailbox.st20_interrupt_status[(offset-0x0004)/4], 4);
		break;
	case 0x0064: /* st20_interrupt_enable */
	case 0x0068:
	case 0x006c:
	case 0x0070:
		memcpy(to, &s_mailbox.st20_interrupt_enable[(offset-0x0064)/4], 4);
		break;
	case 0x0104: /* st40_interrupt_status */
	case 0x0108:
	case 0x010c:
	case 0x0110:
		memcpy(to, &s_mailbox.st40_interrupt_status[(offset-0x0104)/4], 4);
		break;
	case 0x0164: /* st40_interrupt_enable */
	case 0x0168:
	case 0x016c:
	case 0x0170:
		memcpy(to, &s_mailbox.st40_interrupt_enable[(offset-0x0164)/4], 4);
		break;
	default:
		res = 0;
	}

	if (res) {
		VERBOSE(printf("mailbox: read 0x%02x%02x%02x%02x from 0x%08x\n",
		               to[0], to[1], to[2], to[3], address));
	}

	return res;
}

int DevWrite(void *initiator, const unsigned char *from, unsigned int address, 
						 unsigned int numBytes, const char* byteEnables)
{
	int offset, res = 1;

	if (4 != numBytes || 0 != (address&3)) {
		return 0;
	}

	offset = address - s_mailboxAddress;
	switch(offset) {
		unsigned int tmp;
	case 0x0004: /* st20_interrupt_status */
	case 0x0008:
	case 0x000c:
	case 0x0010:
		memcpy(&s_mailbox.st20_interrupt_status[(offset-0x0004)/4], from, 4);
		break;
	case 0x0024: /* st20_interrupt_status_set */
	case 0x0028:
	case 0x002c:
	case 0x0030:
		memcpy(&tmp, from, 4);
		s_mailbox.st20_interrupt_status[(offset-0x0024)/4] |= tmp;
		break;
	case 0x0044: /* st20_interrupt_status_clr */
	case 0x0048:
	case 0x004c:
	case 0x0050:
		memcpy(&tmp, from, 4);
		s_mailbox.st20_interrupt_status[(offset-0x0044)/4] &= ~tmp;
		break;
	case 0x0064: /* st20_interrupt_enable */
	case 0x0068:
	case 0x006c:
	case 0x0070:
		memcpy(&s_mailbox.st20_interrupt_enable[(offset-0x0064)/4], from, 4);
		break;
	case 0x0084: /* st20_interrupt_enable_set */
	case 0x0088:
	case 0x008c:
	case 0x0090:
		memcpy(&tmp, from, 4);
		s_mailbox.st20_interrupt_enable[(offset-0x0084)/4] |= tmp;
		break;
	case 0x00a4: /* st20_interrupt_enable_clr */
	case 0x00a8:
	case 0x00ac:
	case 0x00b0:
		memcpy(&tmp, from, 4);
		s_mailbox.st20_interrupt_enable[(offset-0x00a4)/4] &= ~tmp;
		break;
	case 0x0104: /* st40_interrupt_status */
	case 0x0108:
	case 0x010c:
	case 0x0110:
		memcpy(&s_mailbox.st40_interrupt_status[(offset-0x0104)/4], from, 4);
		break;
	case 0x0124: /* st40_interrupt_status_set */
	case 0x0128:
	case 0x012c:
	case 0x0130:
		memcpy(&tmp, from, 4);
		s_mailbox.st40_interrupt_status[(offset-0x0124)/4] |= tmp;
		break;
	case 0x0144: /* st40_interrupt_status_clr */
	case 0x0148:
	case 0x014c:
	case 0x0150:
		memcpy(&tmp, from, 4);
		s_mailbox.st40_interrupt_status[(offset-0x0144)/4] &= ~tmp;
		break;
	case 0x0164: /* st40_interrupt_enable */
	case 0x0168:
	case 0x016c:
	case 0x0170:
		memcpy(&s_mailbox.st40_interrupt_enable[(offset-0x0164)/4], from, 4);
		break;
	case 0x0184: /* st40_interrupt_enable_set */
	case 0x0188:
	case 0x018c:
	case 0x0190:
		memcpy(&tmp, from, 4);
		s_mailbox.st40_interrupt_enable[(offset-0x0184)/4] |= tmp;
		break;
	case 0x01a4: /* st40_interrupt_enable_clr */
	case 0x01a8:
	case 0x01ac:
	case 0x01b0:
		memcpy(&tmp, from, 4);
		s_mailbox.st40_interrupt_enable[(offset-0x01a4)/4] &= ~tmp;
		break;
	default:
		res = 0;
	}

	if (res) {
		int i;
		VERBOSE(printf("mailbox: write 0x%02x%02x%02x%02x from 0x%08x\n",
		               from[0], from[1], from[2], from[3], address));

		for (i=0; i<4; i++) {
			VERBOSE(printf("mailbox: st20_interrupt_status[%d] = %8x\n", 
			        i, s_mailbox.st20_interrupt_status[i]));
			VERBOSE(printf("mailbox: st20_interrupt_enable[%d] = %8x\n", 
			        i, s_mailbox.st20_interrupt_enable[i]));
			VERBOSE(printf("mailbox: st40_interrupt_status[%d] = %8x\n", 
			        i, s_mailbox.st40_interrupt_status[i]));
			VERBOSE(printf("mailbox: st40_interrupt_enable[%d] = %8x\n", 
			        i, s_mailbox.st40_interrupt_enable[i]));
		}

#ifdef DevClock
		/* optimised clocking - we only clock the peripheral after register writes */
		if (s_core[0].initiator) DevClock(s_core[0].initiator);
		if (s_core[1].initiator) DevClock(s_core[1].initiator);
#endif
	}

	return res;
}
