
#ifndef AUDDVB_H
#define AUDDVB_H

#include "stddefs.h"
#include "staudlx.h" 

#define AUDIO_DEVICE_NAME_PREFIX "audio"
#define AUDIO_BIT_BUFFER_LOW_LEVEL 100
#define AUDIO_BIT_BUFFER_SIZE	(32*1024)
#define STAUD_DIGITAL_OUTPUT_OBJECT STAUD_OBJECT_OUTPUT_SPDIF0

#define AUDDVB_CAP_AC3		(1 << 0)
#define AUDDVB_CAP_DTS		(1 << 1)
#define AUDDVB_CAP_MPEG1	(1 << 2)
#define AUDDVB_CAP_MPEG2	(1 << 3)
#define AUDDVB_CAP_CDDA	(1 << 4)
#define AUDDVB_CAP_PCM		(1 << 5)
#define AUDDVB_CAP_LPCM	(1 << 6)
#define AUDDVB_CAP_PINK_NOISE  (1 << 7)
#define AUDDVB_CAP_MP3		(1 << 8)
#define AUDDVB_CAP_MLP		(1 << 9)
#define AUDDVB_CAP_BEEP_TONE	(1 << 10)
#define AUDDVB_CAP_MPEG_AAC	(1 << 11)
#define AUDDVB_CAP_WMA		(1 << 12)
#define AUDDVB_CAP_DV		(1 << 13)
#define AUDDVB_CAP_CDDA_DTS	(1 << 14)
#define AUDDVB_CAP_LPCM_DVDA   (1 << 15)
#define AUDDVB_CAP_HE_AAC	(1 << 16)
#define AUDDVB_CAP_DDPLUS	(1 << 17)
#define AUDDVB_CAP_WMAPROLSL	(1 << 18)
#define AUDDVB_CAP_NULL	(1 << 19)

/*??? not existing in STAPI*/
#define AUDDVB_CAP_OGG           (1<<20)
#define AUDDVB_CAP_SDDS          (1<<21)

#define AUDDVB_PES        0xffff
#define AUDDVB_ES         0xcccc

/* Shared memory size useful for audio write manager*/
#define AUDDVB_SHARED_BUFFER_SIZE 	16384
/* Audio source */

#define AUDDVB_SOURCE_MEMORY 		0
#define AUDDVB_SOURCE_DEMUX  		1

#define AUDDVB_CHANNEL_SELECT_MODE_STEREO          		0
#define AUDDVB_CHANNEL_SELECT_MODE_PROLOGIC        	1
#define AUDDVB_CHANNEL_SELECT_MODE_DUAL_LEFT       	2
#define AUDDVB_CHANNEL_SELECT_MODE_DUAL_RIGHT      	3
#define AUDDVB_CHANNEL_SELECT_MODE_DUAL_MONO       	4
#define AUDDVB_CHANNEL_SELECT_MODE_AUTO	    		5

/* ************************************************************************
   D A T A   T Y P E S
   ************************************************************************ */




typedef struct aud_shared_buffer_s {
	spinlock_t *lock;
	U32 bufsize;
	U32 size;
	U32 head;
	U32 tail;
	U8  *buffer;
} aud_shared_buffer_t;


ST_ErrorCode_t AUDIO_GetBitBufferParams(int AudioDeviceNum, U8 **Base_p, int *RequiredSize_p); 
ST_ErrorCode_t AUDIO_SetInterface(U32 DeviceNum, GetWriteAddress_t  PTIWritePointerFunction,
	InformReadAddress_t  PTIReadPointerFunction, U32 PTIHandle);

#endif 
