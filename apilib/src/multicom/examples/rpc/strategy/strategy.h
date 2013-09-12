/*
 * strategy.h
 *
 * Copyright (C) STMicroelectronics Limited 2003. All rights reserved.
 *
 * Strategy Example - Header file
 */

#ifndef STRATEGY_H
#define STRATEGY_H

/* collect the definition of OS_FOR_RPC and CPU_FOR_RPC */
#include "os_cpu.h"

arenas {
        { cllr, OS_FOR_RPC, CPU_FOR_RPC },
        { clle, OS_FOR_RPC, CPU_FOR_RPC }
};

transport {
	{ cllr, clle, TRANS_EMBX }
};

headers {
	{ cllr, { "strategy.h" } },
	{ clle, { "strategy.h" } }
};

/* standard C strings (read/write) */
void reverseString(inout.string(256) char *s);

/* function taking a length and an array (write only) */
void readData(int length, out.array(256, length) int *data, out int *acutalLen);

/* structure containing a length and an array (read only) */
typedef struct waveform {
	int length;
	in.array(256, __struct__.length) unsigned short data[256];
} waveform_t;
void printWaveform(waveform_t *w);

/* terminated array of structures containing strings (read only) */
typedef struct addressRecord {
	/* using the string strategy reduces the amount of data transfered
	 * to only that which is required.
	 */
	in.string(256) char name[256];
	in.string(256) char email[256];
} addressRecord_t;
/* this could be decorated more clearly (but less efficiently as follows:
 *   in.termarray(64, 0 == strlen(__element__.name))
 */
void printAddresses(in.termarray(64, '\0' == __element__.name[0]) addressRecord_t *addr);

/* registering a callback with an opaque pointer argument (i.e. the
 * pointer can only be dereferenced on the originating CPU)
 */
typedef int (*callback_fn_t)(void *);
void registerCallback(callback_fn_t cb, in.opaque void *data);
void makeCallback(void);
int  actualCallback(in.opaque void *data);

/* a pointer to an address held in shared memory */
void scanBuffer(in.shared void *buffer);

void closeDown(void);

import {
	{ reverseString,    cllr, clle },
	{ readData,         cllr, clle },
	{ printAddresses,   cllr, clle },
	{ printWaveform,    cllr, clle },
	{ registerCallback, cllr, clle },
	{ makeCallback,     cllr, clle },
	{ actualCallback,   clle, cllr },
/*	{ scanBuffer,       cllr, clle }, */
	{ closeDown,        cllr, clle }
};

#endif /* STRATEGY_H */
