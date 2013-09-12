/*$Id: mixer_process.c,v 1.11 2004/03/10 18:43:52 lassureg Exp $*/

#include "mixer_tools.h"
#include "st20_utils.h"
#ifndef _STANDALONE_
#define WS32 ACC_WS32
#define WS16 ACC_WS16
#define WS8 ACC_WS8
#endif

void mixer_output_status ( tPcmProcessingInterface * pcm_if )
{
    tPcmBuffer * pcmin0, * pcmout, * pcmin1, *pcmin;
    pcmin0  = pcm_if->PcmIn[0];
    pcmin1  = pcm_if->PcmIn[1];
    pcmout = pcm_if->PcmOut[0];

    if ( pcmin1 != 0x0)
    {
        if (pcmin0->NbSamples == 0)
        {
            pcmin = pcmin1;
            pcmout->NbSamples       = pcmin->NbSamples;
        }
        else
        {
            pcmin = pcmin0;
            if(pcmin0->NbSamples>=pcmin1->NbSamples)
                pcmout->NbSamples =pcmin0->NbSamples;
            else
                pcmout->NbSamples =pcmin1->NbSamples;
        }
    }
    else
    {
        pcmin = pcmin0;
        pcmout->NbSamples       = pcmin->NbSamples;
    }

    pcmout->SamplingFreq    = pcmin->SamplingFreq;
    pcm_if->NbNewOutSamples = pcmout->NbSamples;

    pcmout->AudioMode       = pcmin->AudioMode;

    pcmout->Flags = pcmin->Flags;
    pcmout->PTS   = pcmin->PTS;
}
#ifdef _STANDALONE_
enum  eErrorCode
#else
enum eAccBoolean
#endif
mixer_process ( tPcmProcessingInterface * pcm_if )
{
#ifdef _STANDALONE_
    enum eErrorCode status = ACC_EC_OK ;
    enum eMainChannelIdx firstout;
    enum eBoolean 	swap;
#else
    MME_ERROR status = MME_SUCCESS ;
    enum eAccMainChannelIdx firstout;

    enum eAccBoolean 	swap;
#endif
    tMixerConfig *  config = ( tMixerConfig * ) pcm_if->Config;
    tPcmBuffer   ** pcmin  = pcm_if->PcmIn;
    tPcmBuffer   ** pcmout = pcm_if->PcmOut;
    int          scale_shift ;
	int diff_sample;
    short * AlphaNew;
    tPcmBuffer * InPcm;

#ifdef _VOL_
#ifdef _USE_16BIT_
    short AlphaMainNew[2],AlphaSecNew[4];
#else
    int AlphaMainNew[2],AlphaSecNew[4];
#endif
#endif
    int          nspl, processed_samples, remaining_samples;

#ifdef _VOL_
#ifdef _USE_16BIT_
    AlphaSecNew[2]=(mixer_convert_volume(config->Volume[0])>>16);
    AlphaSecNew[3]=(mixer_convert_volume(config->Volume[1])>>16);
    AlphaMainNew[0]=st20_fract16_mulr_fract16_fract16((config->Alpha[0]>>16),(mixer_convert_volume(config->Volume[0])>>16));
    AlphaMainNew[1]=st20_fract16_mulr_fract16_fract16((config->Alpha[0]>>16),(mixer_convert_volume(config->Volume[1]))>>16);
    AlphaSecNew[0]=st20_fract16_mulr_fract16_fract16((config->Alpha[1]>>16),AlphaSecNew[2]);
    AlphaSecNew[1]=st20_fract16_mulr_fract16_fract16((config->Alpha[1]>>16),AlphaSecNew[3]);
#else //_USE_16BIT_
    AlphaSecNew[2]=mixer_convert_volume(config->Volume[0]);
    AlphaSecNew[3]=mixer_convert_volume(config->Volume[1]);
    AlphaMainNew[0]=st20_Q31_mulQ31xQ31(config->Alpha[0],mixer_convert_volume(config->Volume[0]));
    AlphaMainNew[1]=st20_Q31_mulQ31xQ31(config->Alpha[0],mixer_convert_volume(config->Volume[1]));
    AlphaSecNew[0]=st20_Q31_mulQ31xQ31(config->Alpha[1],AlphaSecNew[2]);
    AlphaSecNew[1]=st20_Q31_mulQ31xQ31(config->Alpha[1],AlphaSecNew[3]);
#endif
#endif
#ifdef _STANDALONE_
    if ( pcm_if->FirstTime == ACC_TRUE )
    {
        pcm_if->FirstTime = ACC_FALSE;
    }
#else
	if ( pcm_if->FirstTime == ACC_MME_TRUE )
    {
        pcm_if->FirstTime = ACC_MME_FALSE;
    }
#endif
	/* Scale the mixer according to the number of input */
    scale_shift = 1;
/* #if 0   if(pcmin[1]==0x0)*/ /* Means single input only may work as PCM bypass also for 16 bit only */
/*
      {
      remaining_samples = pcmin[0]->NbSamples ;
      processed_samples = 0;

      while ( remaining_samples )
      {
          *//* Process the only one  input stream *//*
             nspl = ( remaining_samples < MIXER_MAX_BLOCK_SIZE ) ? remaining_samples : MIXER_MAX_BLOCK_SIZE;
             mixer_attenuate_buffer ( pcmin[0], pcmout[0], AlphaMainNew, config->FirstOutChan[0], scale_shift, nspl, processed_samples );
             remaining_samples -= nspl;
             processed_samples += nspl;
             }

             }
             else
             #endif*/
    if( pcmin[0]->NbSamples==0)
    { // No first input So process second input only may work as
        remaining_samples = pcmin[1]->NbSamples ;
        processed_samples = 0;
#ifdef _STANDALONE_
        if(config->Swap[1]==ACC_FALSE)
#else
		if(config->Swap[1]==ACC_MME_FALSE)
#endif
        {
            if(pcmin[1]->WordSize==WS16)
            {
                while ( remaining_samples )
                {
                        /* Process the first input stream */
                    nspl = ( remaining_samples < MIXER_MAX_BLOCK_SIZE ) ? remaining_samples : MIXER_MAX_BLOCK_SIZE;
                    mixer_attenuate_buffer(pcmin[1], pcmout[0], AlphaSecNew, config->FirstOutChan[1], scale_shift, nspl, processed_samples );
                    remaining_samples -= nspl;
                    processed_samples += nspl;
                }

            }
            else if(pcmin[1]->WordSize==WS8)
            {
                while ( remaining_samples )
                {
                        /* Process the first input stream */
                    nspl = ( remaining_samples < MIXER_MAX_BLOCK_SIZE ) ? remaining_samples : MIXER_MAX_BLOCK_SIZE;
                    mixer_8bit_16bit_buffer ( pcmin[1], pcmout[0], nspl, processed_samples );
                    mixer_attenuate_buffer ( pcmout[0], pcmout[0], AlphaSecNew, config->FirstOutChan[1], scale_shift, nspl, processed_samples );
                    remaining_samples -= nspl;
                    processed_samples += nspl;
                }
            }
            else
            {
#ifdef _DEBUG_
                printf("ERROR:Unsuported PCM Resolution\n");
#endif
#ifdef _STANDALONE_
                return(ACC_EC_ERROR);
#else
				return(ACC_MME_FALSE);
#endif
            }
        }
        else
        {
            if(pcmin[1]->WordSize==WS16)
            {
                while ( remaining_samples )
                {
                        /* Process the first input stream */
                    nspl = ( remaining_samples < MIXER_MAX_BLOCK_SIZE ) ? remaining_samples : MIXER_MAX_BLOCK_SIZE;
                    mixer_swap_input(pcmin[1],pcmout[0],nspl, processed_samples);
                    mixer_attenuate_buffer(pcmout[0], pcmout[0], AlphaSecNew, config->FirstOutChan[1], scale_shift, nspl, processed_samples );
                    remaining_samples -= nspl;
                    processed_samples += nspl;
                }
            }
            else if(pcmin[1]->WordSize==WS8)
            {
                while ( remaining_samples )
                {
                        /* Process the first input stream */
                    nspl = ( remaining_samples < MIXER_MAX_BLOCK_SIZE ) ? remaining_samples : MIXER_MAX_BLOCK_SIZE;
                    mixer_8bit_16bit_buffer ( pcmin[1], pcmout[0], nspl, processed_samples );
                    mixer_attenuate_buffer ( pcmout[0], pcmout[0], AlphaSecNew, config->FirstOutChan[1], scale_shift, nspl, processed_samples );
                    remaining_samples -= nspl;
                    processed_samples += nspl;
                }
            }
            else
            {
#ifdef _DEBUG_
                printf("ERROR:Unsuported PCM Resolution\n");
#endif
#ifdef _STANDALONE_
                return(ACC_EC_ERROR);
#else
				return(ACC_MME_FALSE);
#endif
				}
        }
    }
    else if((pcmin[1]==0x0)||(pcmin[1]->NbSamples==0))
    {
        remaining_samples = pcmin[0]->NbSamples ;
        processed_samples = 0;
#ifdef _STANDALONE_
        if(config->Swap[0]==ACC_FALSE)
#else
		if(config->Swap[0]==ACC_MME_FALSE)
#endif
        {
            if(pcmin[0]->WordSize==WS16)
            {
                while ( remaining_samples )
                {
				/* Process the first input stream */
                    nspl = ( remaining_samples < MIXER_MAX_BLOCK_SIZE ) ? remaining_samples : MIXER_MAX_BLOCK_SIZE;
                    mixer_attenuate_buffer ( pcmin[0], pcmout[0], AlphaMainNew, config->FirstOutChan[0], scale_shift, nspl, processed_samples );
                    remaining_samples -= nspl;
                    processed_samples += nspl;
                }
            }
            else if(pcmin[0]->WordSize==WS8)
            {
                while ( remaining_samples )
                {
				/* Process the first input stream */
                    nspl = ( remaining_samples < MIXER_MAX_BLOCK_SIZE ) ? remaining_samples : MIXER_MAX_BLOCK_SIZE;
                    mixer_8bit_16bit_buffer ( pcmin[0], pcmout[0], nspl, processed_samples );
                    mixer_attenuate_buffer ( pcmout[0], pcmout[0], AlphaMainNew, config->FirstOutChan[1], scale_shift, nspl, processed_samples );
                    remaining_samples -= nspl;
                    processed_samples += nspl;
                }
            }
            else
            {
#ifdef _DEBUG_
                printf("ERROR:Unsuported PCM Resolution\n");
#endif
#ifdef _STANDALONE_
                return(ACC_EC_ERROR);
#else
				return(ACC_MME_FALSE);
#endif
            }
        }
        else
        {
            if(pcmin[0]->WordSize==WS16)
            {
                while ( remaining_samples )
                {
				/* Process the first input stream */
                    nspl = ( remaining_samples < MIXER_MAX_BLOCK_SIZE ) ? remaining_samples : MIXER_MAX_BLOCK_SIZE;
                    mixer_swap_input(pcmin[0],pcmout[0],nspl, processed_samples);
                    mixer_attenuate_buffer ( pcmout[0], pcmout[0], AlphaMainNew, config->FirstOutChan[0], scale_shift, nspl, processed_samples );
                    remaining_samples -= nspl;
                    processed_samples += nspl;
                }
            }
            else if(pcmin[0]->WordSize==WS8)
            {
                while ( remaining_samples )
                {
				/* Process the first input stream */
                    nspl = ( remaining_samples < MIXER_MAX_BLOCK_SIZE ) ? remaining_samples : MIXER_MAX_BLOCK_SIZE;
                    mixer_8bit_16bit_buffer ( pcmin[0], pcmout[0], nspl, processed_samples );
                    mixer_attenuate_buffer ( pcmout[0], pcmout[0], AlphaMainNew, config->FirstOutChan[1], scale_shift, nspl, processed_samples );
                    remaining_samples -= nspl;
                    processed_samples += nspl;
                }
            }
            else
            {
#ifdef _DEBUG_
                printf("ERROR:Unsuported PCM Resolution\n");
#endif
#ifdef _STANDALONE_
                return(ACC_EC_ERROR);
#else
				return(ACC_MME_FALSE);
#endif
				}
        }
	}


    else // Means both input 0 and 1 is present
    {

        if(pcmin[0]->NbSamples>=pcmin[1]->NbSamples)
        {
            remaining_samples = pcmin[1]->NbSamples ;
            diff_sample=pcmin[0]->NbSamples-pcmin[1]->NbSamples;
            AlphaNew = AlphaMainNew;
            InPcm =pcmin[0];
            swap = config->Swap[0];
            firstout = config->FirstOutChan[0];

        }
        else
        {
            remaining_samples = pcmin[0]->NbSamples ;
            diff_sample=pcmin[1]->NbSamples-pcmin[0]->NbSamples;
            AlphaNew = AlphaSecNew;
            InPcm =pcmin[1];
            swap = config->Swap[1];
            firstout = config->FirstOutChan[1];
        }
        processed_samples = 0;
#ifdef _STANDALONE_
        if((config->Swap[0]==ACC_FALSE)&&(config->Swap[1]==ACC_FALSE))
#else
		if((config->Swap[0]==ACC_MME_FALSE)&&(config->Swap[1]==ACC_MME_FALSE))
#endif
        {
            if((pcmin[0]->WordSize==WS16)&&(pcmin[1]->WordSize==WS16))
            {
                while ( remaining_samples )
                {
                                /* Process the first input stream */
                    nspl = ( remaining_samples < MIXER_MAX_BLOCK_SIZE ) ? remaining_samples : MIXER_MAX_BLOCK_SIZE;
                    mixer_mix_saturate ( pcmin[0],pcmin[1], pcmout[0], AlphaMainNew,AlphaSecNew, config->FirstOutChan[0],config->FirstOutChan[1], scale_shift, nspl, processed_samples );
                    remaining_samples -= nspl;
                    processed_samples += nspl;
                }
            }
            else if((pcmin[0]->WordSize==WS8)&&(pcmin[1]->WordSize==WS8))
            {
                while ( remaining_samples )
                {
                                /* Process the first input stream */
                    nspl = ( remaining_samples < MIXER_MAX_BLOCK_SIZE ) ? remaining_samples : MIXER_MAX_BLOCK_SIZE;
                    mixer_8bit_mix_saturate( pcmin[0],pcmin[1], pcmout[0], AlphaMainNew,AlphaSecNew, config->FirstOutChan[0],config->FirstOutChan[1], scale_shift, nspl, processed_samples );
                    remaining_samples -= nspl;
                    processed_samples += nspl;
                }
            }
            else if(pcmin[0]->WordSize==WS8)
            {
                while ( remaining_samples )
                {
                                /* Process the first input stream */
                    nspl = ( remaining_samples < MIXER_MAX_BLOCK_SIZE ) ? remaining_samples : MIXER_MAX_BLOCK_SIZE;
                    mixer_8bit_16bit_buffer ( pcmin[0], pcmout[0], nspl, processed_samples );
                    mixer_mix_saturate ( pcmout[0],pcmin[1], pcmout[0], AlphaMainNew,AlphaSecNew, config->FirstOutChan[0],config->FirstOutChan[1], scale_shift, nspl, processed_samples );
                    remaining_samples -= nspl;
                    processed_samples += nspl;
                }
            }
            else if(pcmin[1]->WordSize==WS8)
            {
                while ( remaining_samples )
                {
                                /* Process the first input stream */
                    nspl = ( remaining_samples < MIXER_MAX_BLOCK_SIZE ) ? remaining_samples : MIXER_MAX_BLOCK_SIZE;
                    mixer_8bit_16bit_buffer ( pcmin[1], pcmout[0], nspl, processed_samples );
                    mixer_mix_saturate ( pcmin[0],pcmout[0], pcmout[0], AlphaMainNew,AlphaSecNew, config->FirstOutChan[0],config->FirstOutChan[1], scale_shift, nspl, processed_samples );
                    remaining_samples -= nspl;
                    processed_samples += nspl;
                }
            }
            else
            {
#ifdef _DEBUG_
                printf("ERROR:Unsuported PCM Resolution\n");
#endif
#ifdef _STANDALONE_
                return(ACC_EC_ERROR);
#else
				return(ACC_MME_FALSE);
#endif
            }
        }
#ifdef _STANDALONE_
        else if((config->Swap[0]==ACC_TRUE)&&(config->Swap[1]==ACC_TRUE))
#else
		else if((config->Swap[0]==ACC_MME_TRUE)&&(config->Swap[1]==ACC_MME_TRUE))
#endif
        {
            if((pcmin[0]->WordSize==WS16)&&(pcmin[1]->WordSize==WS16))
            {
                while ( remaining_samples )
                {
                                /* Process the first input stream */
                    nspl = ( remaining_samples < MIXER_MAX_BLOCK_SIZE ) ? remaining_samples : MIXER_MAX_BLOCK_SIZE;
                    mixer_swap_mix_saturate ( pcmin[0],pcmin[1], pcmout[0], AlphaMainNew,AlphaSecNew, config->FirstOutChan[0],config->FirstOutChan[1], scale_shift, nspl, processed_samples );
                    remaining_samples -= nspl;
                    processed_samples += nspl;
                }
            }
            else if((pcmin[0]->WordSize==WS8)&&(pcmin[0]->WordSize==WS8))
            {
                while ( remaining_samples )
                {
                                /* Process the first input stream */
                    nspl = ( remaining_samples < MIXER_MAX_BLOCK_SIZE ) ? remaining_samples : MIXER_MAX_BLOCK_SIZE;
                    mixer_8bit_mix_saturate( pcmin[0],pcmin[1], pcmout[0], AlphaMainNew,AlphaSecNew, config->FirstOutChan[0],config->FirstOutChan[1], scale_shift, nspl, processed_samples );
                    remaining_samples -= nspl;
                    processed_samples += nspl;
                }
            }
            else if(pcmin[0]->WordSize==WS8)
            {
                while ( remaining_samples )
                {
                                /* Process the first input stream */
                    nspl = ( remaining_samples < MIXER_MAX_BLOCK_SIZE ) ? remaining_samples : MIXER_MAX_BLOCK_SIZE;
                    mixer_8bit_16bit_buffer ( pcmin[0], pcmout[0], nspl, processed_samples );
                    mixer_swap_input(pcmin[1],pcmin[1], nspl, processed_samples );
                    mixer_mix_saturate ( pcmout[0],pcmin[1], pcmout[0], AlphaMainNew,AlphaSecNew, config->FirstOutChan[0],config->FirstOutChan[1], scale_shift, nspl, processed_samples );
                    remaining_samples -= nspl;
                    processed_samples += nspl;
                }
            }
            else if(pcmin[1]->WordSize==WS8)
            {
                while ( remaining_samples )
                {
                                /* Process the first input stream */
                    nspl = ( remaining_samples < MIXER_MAX_BLOCK_SIZE ) ? remaining_samples : MIXER_MAX_BLOCK_SIZE;
                    mixer_8bit_16bit_buffer ( pcmin[1], pcmout[0], nspl, processed_samples );
                    mixer_swap_input(pcmin[0],pcmin[0], nspl, processed_samples );
                    mixer_mix_saturate ( pcmin[0],pcmout[0], pcmout[0], AlphaMainNew,AlphaSecNew, config->FirstOutChan[0],config->FirstOutChan[1], scale_shift, nspl, processed_samples );
                    remaining_samples -= nspl;
                    processed_samples += nspl;
                }
            }
            else
            {
#ifdef _DEBUG_
                printf("ERROR:Unsuported PCM Resolution\n");
#endif
#ifdef _STANDALONE_
                return(ACC_EC_ERROR);
#else
				return(ACC_MME_FALSE);
#endif
            }
        }
#ifdef _STANDALONE_
        else if(config->Swap[0]==ACC_TRUE)
#else
		else if(config->Swap[0]==ACC_MME_TRUE)
#endif
        {
            if((pcmin[0]->WordSize==WS16)&&(pcmin[1]->WordSize==WS16))
            {
                while ( remaining_samples )
                {
                                /* Process the first input stream */
                    nspl = ( remaining_samples < MIXER_MAX_BLOCK_SIZE ) ? remaining_samples : MIXER_MAX_BLOCK_SIZE;
                    mixer_swap_input(pcmin[0],pcmout[0], nspl, processed_samples );
                    mixer_mix_saturate ( pcmout[0],pcmin[1], pcmout[0], AlphaMainNew,AlphaSecNew, config->FirstOutChan[0],config->FirstOutChan[1], scale_shift, nspl, processed_samples );
                    remaining_samples -= nspl;
                    processed_samples += nspl;
                }
            }
            else if((pcmin[0]->WordSize==WS8)&&(pcmin[0]->WordSize==WS8))
            {
                while ( remaining_samples )
                {
                                /* Process the first input stream */
                    nspl = ( remaining_samples < MIXER_MAX_BLOCK_SIZE ) ? remaining_samples : MIXER_MAX_BLOCK_SIZE;
                    mixer_8bit_mix_saturate( pcmin[0],pcmin[1], pcmout[0], AlphaMainNew,AlphaSecNew, config->FirstOutChan[0],config->FirstOutChan[1], scale_shift, nspl, processed_samples );
                    remaining_samples -= nspl;
                    processed_samples += nspl;
                }
            }
            else if(pcmin[0]->WordSize==WS8)
            {
                while ( remaining_samples )
                {
                                /* Process the first input stream */
                    nspl = ( remaining_samples < MIXER_MAX_BLOCK_SIZE ) ? remaining_samples : MIXER_MAX_BLOCK_SIZE;
                    mixer_8bit_16bit_buffer ( pcmin[0], pcmout[0], nspl, processed_samples );
                    mixer_mix_saturate ( pcmout[0],pcmin[1], pcmout[0], AlphaMainNew,AlphaSecNew, config->FirstOutChan[0],config->FirstOutChan[1], scale_shift, nspl, processed_samples );
                    remaining_samples -= nspl;
                    processed_samples += nspl;
                }
            }
            else if(pcmin[1]->WordSize==WS8)
            {
                while ( remaining_samples )
                {
                                /* Process the first input stream */
                    nspl = ( remaining_samples < MIXER_MAX_BLOCK_SIZE ) ? remaining_samples : MIXER_MAX_BLOCK_SIZE;
                    mixer_8bit_16bit_buffer ( pcmin[1], pcmout[0], nspl, processed_samples );
                    mixer_swap_input(pcmin[0],pcmin[0], nspl, processed_samples );
                    mixer_mix_saturate ( pcmin[0],pcmout[0], pcmout[0], AlphaMainNew,AlphaSecNew, config->FirstOutChan[0],config->FirstOutChan[1], scale_shift, nspl, processed_samples );
                    remaining_samples -= nspl;
                    processed_samples += nspl;
                }
            }
            else
            {
#ifdef _DEBUG_
                printf("ERROR:Unsuported PCM Resolution\n");
#endif
#ifdef _STANDALONE_
                return(ACC_EC_ERROR);
#else
				return(ACC_MME_FALSE);
#endif
            }
        }
        else //Swap of PCM Input 1 is requested
        {
            if((pcmin[0]->WordSize==WS16)&&(pcmin[1]->WordSize==WS16))
            {
                while ( remaining_samples )
                {
                                /* Process the first input stream */
                    nspl = ( remaining_samples < MIXER_MAX_BLOCK_SIZE ) ? remaining_samples : MIXER_MAX_BLOCK_SIZE;
                    mixer_swap_input(pcmin[1],pcmout[0], nspl, processed_samples );
                    mixer_mix_saturate ( pcmin[0],pcmout[0], pcmout[0], AlphaMainNew,AlphaSecNew, config->FirstOutChan[0],config->FirstOutChan[1], scale_shift, nspl, processed_samples );
                    remaining_samples -= nspl;
                    processed_samples += nspl;
                }
            }
            else if((pcmin[0]->WordSize==WS8)&&(pcmin[1]->WordSize==WS8))
            {
                while ( remaining_samples )
                {
                                /* Process the first input stream */
                    nspl = ( remaining_samples < MIXER_MAX_BLOCK_SIZE ) ? remaining_samples : MIXER_MAX_BLOCK_SIZE;
                    mixer_8bit_mix_saturate( pcmin[0],pcmin[1], pcmout[0], AlphaMainNew,AlphaSecNew, config->FirstOutChan[0],config->FirstOutChan[1], scale_shift, nspl, processed_samples );
                    remaining_samples -= nspl;
                    processed_samples += nspl;
                }
            }
            else if(pcmin[0]->WordSize==WS8)
            {
                while ( remaining_samples )
                {
                                /* Process the first input stream */
                    nspl = ( remaining_samples < MIXER_MAX_BLOCK_SIZE ) ? remaining_samples : MIXER_MAX_BLOCK_SIZE;
                    mixer_swap_input(pcmin[1],pcmin[1],nspl, processed_samples );
                    mixer_8bit_16bit_buffer ( pcmin[0], pcmout[0], nspl, processed_samples );
                    mixer_mix_saturate ( pcmout[0],pcmin[1], pcmout[0], AlphaMainNew,AlphaSecNew, config->FirstOutChan[0],config->FirstOutChan[1], scale_shift, nspl, processed_samples );
                    remaining_samples -= nspl;
                    processed_samples += nspl;
                }
            }
            else if(pcmin[1]->WordSize==WS8)
            {
                while ( remaining_samples )
                {
                                /* Process the first input stream */
                    nspl = ( remaining_samples < MIXER_MAX_BLOCK_SIZE ) ? remaining_samples : MIXER_MAX_BLOCK_SIZE;
				//	mixer_swap_buffer(pcmin[0],pcmin[0],nspl, processed_samples );
                    mixer_8bit_16bit_buffer ( pcmin[1], pcmout[0], nspl, processed_samples );
                    mixer_mix_saturate ( pcmin[0],pcmout[0], pcmout[0], AlphaMainNew,AlphaSecNew, config->FirstOutChan[0],config->FirstOutChan[1], scale_shift, nspl, processed_samples );
                    remaining_samples -= nspl;
                    processed_samples += nspl;
                }
            }
            else
            {
#ifdef _DEBUG_
                printf("ERROR:Unsuported PCM Resolution\n");
#endif
#ifdef _STANDALONE_
                return(ACC_EC_ERROR);
#else
				return(ACC_MME_FALSE);
#endif
            }
        }
        remaining_samples = diff_sample ;
#ifdef _STANALONE_
        if(swap==ACC_FALSE)
 #else
 		if(swap==ACC_MME_FALSE)
 #endif
        {
            if(InPcm->WordSize==WS16)
            {
                while ( remaining_samples )
                {
                                /* Process the first input stream */
                    nspl = ( remaining_samples < MIXER_MAX_BLOCK_SIZE ) ? remaining_samples : MIXER_MAX_BLOCK_SIZE;
                    mixer_attenuate_buffer(InPcm, pcmout[0], AlphaNew, firstout, scale_shift, nspl, processed_samples );
                    remaining_samples -= nspl;
                    processed_samples += nspl;
                }
            }
            else if(InPcm->WordSize==WS8)
            {
                while ( remaining_samples )
                {
                                /* Process the first input stream */
                    nspl = ( remaining_samples < MIXER_MAX_BLOCK_SIZE ) ? remaining_samples : MIXER_MAX_BLOCK_SIZE;
                    mixer_8bit_16bit_buffer ( InPcm, pcmout[0], nspl, processed_samples );
                    mixer_attenuate_buffer ( pcmout[0], pcmout[0], AlphaNew, firstout, scale_shift, nspl, processed_samples );
                    remaining_samples -= nspl;
                    processed_samples += nspl;
                }
            }
            else
            {
#ifdef _DEBUG_
                printf("ERROR:Unsuported PCM Resolution\n");
#endif
#ifdef _STANDALONE_
                return(ACC_EC_ERROR);
#else
				return(ACC_MME_FALSE);
#endif
            }
        }
        else
        {
            if(InPcm->WordSize==WS16)
            {
                while ( remaining_samples )
                {
                                /* Process the first input stream */
                    nspl = ( remaining_samples < MIXER_MAX_BLOCK_SIZE ) ? remaining_samples : MIXER_MAX_BLOCK_SIZE;
                    mixer_swap_input(InPcm,pcmout[0],nspl, processed_samples);
                    mixer_attenuate_buffer(pcmout[0], pcmout[0], AlphaNew, firstout, scale_shift, nspl, processed_samples );
                    remaining_samples -= nspl;
                    processed_samples += nspl;
                }
            }
            else if(InPcm->WordSize==WS8)
            {
                while ( remaining_samples )
                {
                                /* Process the first input stream */
                    nspl = ( remaining_samples < MIXER_MAX_BLOCK_SIZE ) ? remaining_samples : MIXER_MAX_BLOCK_SIZE;
                    mixer_8bit_16bit_buffer ( InPcm, pcmout[0], nspl, processed_samples );
                    mixer_attenuate_buffer ( pcmout[0], pcmout[0], AlphaNew, firstout, scale_shift, nspl, processed_samples );
                    remaining_samples -= nspl;
                    processed_samples += nspl;
                }
            }
            else
            {
#ifdef _DEBUG_
                printf("ERROR:Unsuported PCM Resolution\n");
#endif
#ifdef _STANDALONE_
                return(ACC_EC_ERROR);
#else
				return(ACC_MME_FALSE);
#endif
            }
        }
    }
    mixer_output_status ( pcm_if );

    return ( status );
}
