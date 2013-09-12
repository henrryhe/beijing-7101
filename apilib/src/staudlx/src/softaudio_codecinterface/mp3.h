/* $Id: mp3.h,v 1.5 2004/01/16 15:52:02 clapies Exp $ */
#ifndef _MP3_H_
#define _MP3_H_

#include "stdio.h"
/*#include "acc_system.h"*/
#include "acc_api.h"

typedef struct {
    enum eBoolean FreeFormat;
	enum eBoolean DecodeLastFrame;
	enum eBoolean MuteIfError;
//	#ifdef _VCONTROL_
	int mp3_left_vol;
	int mp3_right_vol;
//	#endif
} tMp3Config;
/*
extern void               mp3_set_default(void);

extern enum eErrorCode    mp3_allocate(tDecoderInterface * dec_if);
extern enum eErrorCode    mp3_free(tDecoderInterface * dec_if);
extern enum eErrorCode    mp3_show_help(void);
extern int                mp3_parse_command(int argc, char ** argv, int i );

extern tSystemDecoding    mp3_processing;

extern enum eErrorCode    mp3_decode(tDecoderInterface *mp3_interface);
*/
#endif /* _MP3_H_ */
