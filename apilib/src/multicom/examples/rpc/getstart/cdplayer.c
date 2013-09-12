/*
 * cdplayer.c
 *
 * Copyright (C) STMicroelectronics Limited 2003. All rights reserved.
 *
 * Getting Started Example - A Text Based CD Player!
 */

#include <stdio.h>

#include "cdplayer.h"
#include "harness.h"

typedef struct CD_TrackList {
	CD_TrackInfo_t info;
	char *music;
} CD_TrackList_t;

CD_TrackList_t trackList[5] = {
	{ 
		{ 500, "Beethoven - Symphony Number 5" }, 
		"## Dum Dum Dum Duuuuummmmm ##" 
	},
	{
		{ 3*60 + 36, "Lizzy Borden - Master of Disguise" },
		"I'm the master of disguise, "
		"I can see you watching from the corner of my eyes."
	},
	{
		{ 2*60 + 58, "Yesterday's Kids - Nothing Gold Can Stay" },
		"When your life is on a roll, "
		"when you think you're in complete control."
	},
	{
		{ 3*60 + 11, "They Might Be Giants - Birdhouse in Your Soul" },
		"Not to put to fine a point on it, "
		"say I'm the only bee in your bonnet."
	},
	{
		{ 4*60 + 40, "Eric Burdon - House of the Rising Sun" },
		"There is a house in New Orleans they call the Rising Sun."
	}
};

volatile int currentTrack = 0;
volatile int finished = 0;

void CD_PlayTrack(int track)
{
	char s[256];

	currentTrack = track;

	sprintf(s, "CD: Playing: %s (%d:%d)\n", trackList[track].info.name,
					    trackList[track].info.length / 60,
					    trackList[track].info.length % 60);
	log(s);
}

void CD_Stop(void)
{
	currentTrack = -1;
	log("CD: Stopped\n");
}

int  CD_GetTrackCount(void)
{
	return sizeof(trackList)/sizeof(trackList[0]);
}

int CD_GetTrackInfo(int count, CD_TrackInfo_t *info)
{
	int i;

	/* fail if we do not have enough memory to copy the list */
	if (count <= CD_GetTrackCount()) {
		return -1;
	}

	/* copy the list */
	for (i=0; i < CD_GetTrackCount(); i++) {
		info[i] = trackList[i].info;
	}

	/* terminate the list (so it can be copied back ok) */
	info[i].length = 0;

	return 0;
}

void CD_Eject(void)
{
	log("CD: Ejecting Disc\n");
	finished = 1;
}

int main()
{
	int i;

	rpcRuntimeInit();

	CD_PlayTrack(0);

	/* start 'playing' the CD */
	while (!finished) {
		int track = currentTrack;
		if (track >= 0) {
			printf("%s", trackList[currentTrack].music);

			for (i=0; i<26; i++) {
				while (currentTrack < 0 && !finished) {}
				printf("\n");
				fflush(stdout);
			}
		}
	}

	return 0;
}
