/*
 * cdplayer.h
 *
 * Copyright (C) STMicroelectronics Limited 2003. All rights reserved.
 *
 * Getting Starting Example - Header file
 */

#ifndef CDPLAYER_H
#define CDPLAYER_H

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

typedef struct CD_TrackInfo {
	int length;
	out.string(256) char name[256];
} CD_TrackInfo_t; 

/* by sharing the console log function the output from both CPUs will appear
 * in the same window
 */
void log(in.string(256) const char*msg);


/* a very simple API to control a CD player (with track lookup) */
void CD_PlayTrack(int);
void CD_Stop(void);
void CD_Eject(void);
int  CD_GetTrackCount(void);
int  CD_GetTrackInfo(int count, out.termarray(16, 0 == __element__.length) CD_TrackInfo_t *info);

#endif /* CDPLAYER_H */
