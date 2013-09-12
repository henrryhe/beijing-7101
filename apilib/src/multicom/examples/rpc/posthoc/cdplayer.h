/*
 * cdplayer.h
 *
 * Copyright (C) STMicroelectronics Limited 2003. All rights reserved.
 *
 * Getting Starting Example - Header file
 */

#ifndef CDPLAYER_H
#define CDPLAYER_H

#define ENABLE_RPC 1

typedef struct CD_TrackInfo {
	int length;
	char name[256];
} CD_TrackInfo_t; 

/* by sharing the console log function the output from both CPUs will appear
 * in the same window
 */
void log(const char*msg);

/* a very simple API to control a CD player (with track lookup) */
void CD_PlayTrack(int);
void CD_Stop(void);
void CD_Eject(void);
int  CD_GetTrackCount(void);
int  CD_GetTrackInfo(int count, CD_TrackInfo_t *info);

#endif /* CDPLAYER_H */
