/* ************************************************************************
   Copyright (c) Jan 2002, STMicroelectronics, NV. All Rights Reserved
   
   Author:        
   ST Division:   STB
   ST Group:      CH-BU/Software Group
   Project:       Linux STB
   Purpose:       Linux DVB Audio driver - header file
   ************************************************************************ */

#ifndef DVBAUDIO_H
#define DVBAUDIO_H

#ifdef __KERNEL__
#include <linux/types.h>
#else
#include <stdint.h>
#include <time.h>
#endif

/* ************************************************************************
   D E F I N E S
   ************************************************************************ */
/* Forced by DVB Convergence (c) specs. */

#ifndef EINTERNAL
#define EINTERNAL    568
#endif /* EINTERNAL */

/* Custom defines */
/* By Convergence (c) DVB-API is not possibile to handle
   different audio/video stream type*/
/* PES and ES audio/video stream are supported by LinuxSTB Tree*/

#define AUDIO_PES    0xffff
#define AUDIO_ES     0xcccc

#ifndef EBUFFEROVERFLOW
#define EBUFFEROVERFLOW 769
#endif /*!EBUFFEROVERFLOW*/

/* Linux Convergence (c) DVB Defines */

#define AUDIO_CAP_DTS                          1
#define AUDIO_CAP_LPCM                         2
#define AUDIO_CAP_MP1                          4
#define AUDIO_CAP_MP2                          8
#define AUDIO_CAP_MP3                          16
#define AUDIO_CAP_AAC                          32
#define AUDIO_CAP_OGG                          64
#define AUDIO_CAP_SDDS                         128
#define AUDIO_CAP_AC3                          256

/*extra audio formats (not present in DVB API specs)*/
#define AUDIO_CAP_PCM                          512
#define AUDIO_CAP_HE_AAC                      1024

/* ************************************************************************
   D A T A   T Y P E S
   ************************************************************************ */

/* Linux Convergence (c) DVB Audio Data Types */

typedef enum {
	AUDIO_SOURCE_DEMUX,
	AUDIO_SOURCE_MEMORY
} audio_stream_source_t;


typedef enum {
	AUDIO_STOPPED,
	AUDIO_PLAYING,
	AUDIO_PAUSED
} audio_play_state_t;


typedef enum {
        AUDIO_STEREO,       /*stereo*/
	AUDIO_MONO,         /* left+right mixed to both channels*/
	AUDIO_MONO_LEFT,    /* left to both channels*/
	AUDIO_MONO_RIGHT,   /* right to both channels*/
	AUDIO_AUTO          /* automatic*/
} audio_channel_select_t;

typedef struct audio_mixer {
	unsigned int volume_left;
	unsigned int volume_right;
} audio_mixer_t;

typedef struct audio_status {
	int                    AV_sync_state;
	int                    mute_state;
	audio_play_state_t     play_state;
	audio_stream_source_t  stream_source;
	audio_channel_select_t channel_select;
	int                    bypass_mode;
	audio_mixer_t	       mixer_state;    
} audio_status_t;




typedef struct audio_karaoke {
	int vocal1;
	int vocal2;
	int melody;
} audio_karaoke_t;


typedef uint16_t audio_attributes_t;
/*   bits: descr. */
/*   15-13 audio coding mode (0=ac3, 2=mpeg1, 3=mpeg2ext, 4=LPCM, 6=DTS, */
/*   12    multichannel extension */
/*   11-10 audio type (0=not spec, 1=language included) */
/*    9- 8 audio application mode (0=not spec, 1=karaoke, 2=surround) */
/*    7- 6 Quantization / DRC (mpeg audio: 1=DRC exists)(lpcm: 0=16bit,  */
/*    5- 4 Sample frequency fs (0=48kHz, 1=96kHz) */
/*    2- 0 number of audio channels (n+1 channels) */

/* Linux Convergence (c) DVB ioctl declaration */


typedef struct pcm_format {
	int	sample_rate;
	int	channels;
	int	data_precision;
} audio_pcm_format_t;


#define AUDIO_STOP                  _IO ('o', 1) 
#define AUDIO_PLAY                  _IO ('o', 2)
#define AUDIO_PAUSE                 _IO ('o', 3)
#define AUDIO_CONTINUE              _IO ('o', 4)
#define AUDIO_SELECT_SOURCE         _IOW('o', 5,  audio_stream_source_t)
#define AUDIO_SET_MUTE              _IOW('o', 6,  int)
#define AUDIO_SET_AV_SYNC           _IOW('o', 7,  int)
#define AUDIO_SET_BYPASS_MODE       _IOW('o', 8,  int)
#define AUDIO_CHANNEL_SELECT        _IOW('o', 9,  audio_channel_select_t)
#define AUDIO_GET_STATUS            _IOR('o', 10, struct audio_status *)
#define AUDIO_SET_ID                _IOW('o', 11, int)
#define AUDIO_SET_MIXER             _IOW('o', 12, audio_mixer_t *)
#define AUDIO_SET_STREAMTYPE        _IOW('o', 13, int)
#define AUDIO_SET_ATTRIBUTES        _IOW('o', 14, audio_attributes_t)
#define AUDIO_GET_CAPABILITIES      _IOR('o', 15, unsigned int *)
#define AUDIO_CLEAR_BUFFER          _IO ('o', 16)
#define AUDIO_SET_EXT_ID            _IOW('o', 17, int)
#define AUDIO_SET_KARAOKE           _IOW('o', 18, audio_karaoke_t *)
#define AUDIO_PCM_SET_FORMAT        _IOW('o', 19, audio_pcm_format_t)
#endif /*DVBAUDIO_H*/
