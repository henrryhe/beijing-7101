/*
 * shakedown/roundtrip/roundtrip.h
 *
 * Copyright (C) STMicroelectronics Ltd. 2001
 *
 * Generic headerr for simple roundtrip shakedown
 */

#ifndef rpc_roundtrip_h
#define rpc_roundtrip_h

#include "harness.h"

void setLastInt(int);
void remoteLastInt(int);

arenas {
	{ slve, OS_FOR_SLAVE,  CPU_FOR_SLAVE },
	{ mstr, OS_FOR_MASTER, CPU_FOR_MASTER }
};

transport {
/* check that the named transport language extension works */
#if defined __LINUX__ && defined __KERNEL__ && defined MEDIAREF
	{ slve, mstr, TRANS_EMBX, "shm_mediaref" }
#else
	{ slve, mstr, TRANS_EMBX }
#endif

};

headers {
	{ slve, { "\"roundtrip.h\"" } },
	{ mstr, { "\"roundtrip.h\"" } }
};

import {
	{ setLastInt, slve, mstr },
	{ remoteLastInt, mstr, slve }
};

#endif
