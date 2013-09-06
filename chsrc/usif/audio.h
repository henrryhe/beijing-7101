
#ifndef AUDIO_USIF
#include "staud.h"
#define AUDIO_USIF

#ifndef OSD_COLOR_TYPE_RGB16  
#define MUTE_FILL_COLOR     0xfff09000
#define UNMUTE_FILL_COLOR   0xff80b0e0
#define VOLUME_BACK_COLOR    0xff506090
#else/*ARGB1555 */
#define MUTE_FILL_COLOR     0xFA40
#define UNMUTE_FILL_COLOR   0xC2DC
#define VOLUME_BACK_COLOR    0xA992
#endif
#define VOLUME_STEP             2
#define MAX_VOL_LEVEL           63   /*64*/
#define MIN_VOL_LEVEL           17 /*48*/

#define VOLUME_BCK_X 		(475-8)
#define VOLUME_BCK_Y 		/*48*/45+5
#define VOLUME_SUB_X		(VOLUME_BCK_X+5)
#define VOLUME_SUB_Y 		(VOLUME_BCK_Y+5)
#define VOLUME_ADD_X		(644-8)
#define VOLUME_ADD_Y		VOLUME_SUB_Y
#define VOLUME_START_X 	    ( 505-8)
#define VOLUME_START_Y 	    /* 53*/49+5

#define VOLUME_SPEAKER_X 	(442-8)
#define VOLUME_SPEAKER_Y 	VOLUME_BCK_Y

#define VOLUME_WIDTH  		188
#define VOLUME_HEIGHT		24

#define VOLUME_PER_STEP   	8

#define MOSAIC_MUTE_OFFSETX   -20 /* wz ADD 20070111*/
#define MOSAIC_MUTE_OFFSETY   50
#define STOCK_MUTE_OFFSETX    -35

extern  void CH6_ShowSpeakerIcon(U32 StartX,U32 StartY,char MuteMode);
extern  int CH6_Volume(int iKeyScanCode);
extern  int CH6_VolumeSet(char AudioMode);
extern  void CH6_MuteAudio(void);
extern  void CH6_ChangeVolume(char  DrawFirstly , char Volume ,char MuteMode,char AudioMode);
extern  void mpeg_set_audio_volume_LR ( U16 left_volume, U16 right_volume );

extern void CH_CheckMute(void);
extern void CH6_MosaicChangeVolume(char DrawFirstly , char Volume ,char MuteMode,char AudioMode);
extern int  game_set_volume(int iKeyRead);

void CH_SetAudOut(char AMode,STAUD_StreamContent_t streamtype);
extern void CH6_StockChangeVolume(char DrawFirstly , char Volume ,char MuteMode,char AudioMode);
#endif

/*--eof----------------------------------------------------------------------------*/


