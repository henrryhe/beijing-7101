/*
 * tricks.h
 *
 * Copyright (C) STMicroelectronics Limited 2003. All rights reserved.
 *
 * 
 */

#ifndef TRICKS_H
#define TRICKS_H

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
	{ cllr, { "tricks.h" } },
	{ clle, { "tricks.h" } }
};

void closeDown(void);
import {{ closeDown, cllr, clle }};

#include "union.h"
#include "prefix.h"

#endif /* TRICKS_H */
