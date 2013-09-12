/*$Id: mixer_tools.h,v 1.4 2004/03/10 18:43:52 lassureg Exp $*/

#ifndef _MIXER_TOOLS_H_
#define _MIXER_TOOLS_H_

#include "acc_mmedefines.h"

#include "mixer_defines.h"
#include "mixer_config.h"
#include "mixer_multicom_heap.h"
#ifdef _STANALONE_
extern  enum  eAccBoolean  mixer_8bit_16bit_buffer(tPcmBuffer *pcmin,tPcmBuffer *pcmout, int nbspl ,
												  int processed_spl );
extern  enum  eAccBoolean  mixer_swap_input(tPcmBuffer *,tPcmBuffer *, int nbspl ,
										   int processed_spl );
extern  enum  eAccBoolean  mixer_8bit_mix_saturate( tPcmBuffer * pcmin0,tPcmBuffer * pcmin1,
												  tPcmBuffer * pcmout, short * AlphaMain,
												  short * AlphaSec,
												  enum eMainChannelIdx first_out_main,
												  enum eMainChannelIdx first_out_sec,
												  int scale_shift, int nbspl,
												  int processed_spl);
extern  enum  eAccBoolean  mixer_swap_mix_saturate( tPcmBuffer * pcmin0,tPcmBuffer * pcmin1,
												  tPcmBuffer * pcmout, short * AlphaMain,
												  short * AlphaSec,
												  enum eMainChannelIdx first_out_main,
												  enum eMainChannelIdx first_out_sec,
												  int scale_shift, int nbspl,
												  int processed_spl);
#else
extern  enum  eAccBoolean  mixer_8bit_16bit_buffer(tPcmBuffer *pcmin,tPcmBuffer *pcmout, int nbspl ,
												  int processed_spl );
extern  enum  eAccBoolean  mixer_swap_input(tPcmBuffer *,tPcmBuffer *, int nbspl ,
										   int processed_spl );
extern  enum  eAccBoolean  mixer_8bit_mix_saturate( tPcmBuffer * pcmin0,tPcmBuffer * pcmin1,
												  tPcmBuffer * pcmout, short * AlphaMain,
												  short * AlphaSec,
												  enum eAccMainChannelIdx first_out_main,
												  enum eAccMainChannelIdx first_out_sec,
												  int scale_shift, int nbspl,
												  int processed_spl);
extern  enum  eAccBoolean  mixer_swap_mix_saturate( tPcmBuffer * pcmin0,tPcmBuffer * pcmin1,
												  tPcmBuffer * pcmout, short * AlphaMain,
												  short * AlphaSec,
												  enum eAccMainChannelIdx first_out_main,
												  enum eAccMainChannelIdx first_out_sec,
												  int scale_shift, int nbspl,
												  int processed_spl);
#endif
extern int mixer_convert_volume(int dB);
#ifndef _VOL_
extern enum eAccBoolean mixer_attenuate_buffer    ( tPcmBuffer * pcmin, tPcmBuffer * pcmout, tCoeff32 alpha, enum eAccMainChannelIdx first_out, int scale_shift,
	        int nbspl, int processed_spl);
extern enum eAccBoolean mixer_mix_buffer_saturate ( tPcmBuffer * pcmin, tPcmBuffer * pcmout, tCoeff32 alpha, enum eAccMainChannelIdx first_out, int scale_shift,
	        int nbspl, int processed_spl);
#else
#ifdef _USE_16BIT_
extern enum eAccBoolean mixer_attenuate_buffer    ( tPcmBuffer * pcmin, tPcmBuffer * pcmout, short *alpha, enum eAccMainChannelIdx first_out, int scale_shift,
	        int nbspl, int processed_spl);
extern enum eAccBoolean mixer_mix_buffer_saturate ( tPcmBuffer * pcmin, tPcmBuffer * pcmout, short *alpha, enum eAccMainChannelIdx first_out, int scale_shift,
	        int nbspl, int processed_spl);
extern enum eAccBoolean mixer_mix_saturate ( tPcmBuffer * pcmin0,tPcmBuffer * pcmin1, tPcmBuffer * pcmout, short * AlphaMain,short * AlphaSec,enum eAccMainChannelIdx first_out_main, enum eAccMainChannelIdx first_out_sec, int scale_shift, int nbspl, int processed_spl);

#else
extern enum eAccBoolean mixer_attenuate_buffer    ( tPcmBuffer * pcmin, tPcmBuffer * pcmout, short *alpha, enum eAccMainChannelIdx first_out, int scale_shift,
	        int nbspl, int processed_spl);
extern enum eAccBoolean mixer_mix_buffer_saturate ( tPcmBuffer * pcmin, tPcmBuffer * pcmout, short *alpha, enum eAccMainChannelIdx first_out, int scale_shift,
	        int nbspl, int processed_spl);
#endif
#endif

#endif /* _MIXER_TOOLS_H_ */
