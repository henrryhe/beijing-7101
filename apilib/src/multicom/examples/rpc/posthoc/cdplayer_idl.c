/*
 * cdplayer_idl.c [must be name .c or .h so the compiler driver knows
 *                 what to do with this file]
 *
 * Copyright (C) STMicroelectronics Limited 2003. All rights reserved.
 *
 * Interface definition for the CD player application
 */

#include "cdplayer.h"

/* collect the definition of OS_FOR_RPC and CPU_FOR_RPC */
#include "os_cpu.h"

arenas {
	{ app, OS_FOR_RPC, CPU_FOR_RPC },
	{ cd,  OS_FOR_RPC, CPU_FOR_RPC }
};

transport {
	{ app, cd, TRANS_EMBX }
};

headers {
	{ app, { "cdplayer.h" } },
	{ cd,  { "cdplayer.h" } }
};

import {
	{ log,              cd, app },
	{ CD_PlayTrack,     app, cd },
	{ CD_Stop,          app, cd },
	{ CD_Eject,         app, cd },
	{ CD_GetTrackCount, app, cd },
	{ CD_GetTrackInfo,  app, cd }
};

rpc_info {
	/* types */
	typedef struct CD_TrackInfo {
		int length;
		out.string(256) char name[256];
	} CD_TrackInfo_t; 

	/* functions */
	void log(
		in.string(256) const char*msg
	);
	int  CD_GetTrackInfo(
		int count, 
		out.termarray(16, 0 == __element__.length) CD_TrackInfo_t *info
	);
};
