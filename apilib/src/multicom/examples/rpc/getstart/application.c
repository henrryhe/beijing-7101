/*
 * application.c
 *
 * Copyright (C) STMicroelectronics Limited 2003. All rights reserved.
 *
 * Getting Started Example - Master side
 */

#include <stdio.h>
#include <string.h>

#include "cdplayer.h"
#include "harness.h"

int main()
{
	int numTracks;
	CD_TrackInfo_t *trackList;

	rpcRuntimeInit();

	/* fetch the track list */
	log("App: fetching the track list\n");
	numTracks = CD_GetTrackCount();
	trackList = (CD_TrackInfo_t *) malloc(sizeof(CD_TrackInfo_t) * (numTracks + 1));
	if (0 != CD_GetTrackInfo(numTracks + 1, trackList)) {
		log("ERROR: could not fetch the track list\n");
		return 1;
	}

	/* now select the tracks */
	while (1) {
		char *p;
		char search[80];
		int i;

		log("App: please enter a string to search for ...\n");
		if (NULL == fgets(search, sizeof(search), stdin)) {
			break;
		}

		/* remove trailing new line */
		p = strchr(search, '\r');
		p = (NULL == p ? strchr(search, '\n') : p);
		if (NULL != p) {
			*p = '\0';
		}

		if (search[0] != '\0') {
			for (i=0; i<numTracks; i++) {
				if (NULL != strstr(trackList[i].name, search)) { break; }
			}

			if (i<numTracks) {
				CD_PlayTrack(i);
			} else {
				CD_Stop();
			}
		} else {
			CD_Eject();
			break;
		}
	}

	return 0;
}

void log(const char *msg)
{
	printf("%s", msg);
	fflush(stdout);
}
