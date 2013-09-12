/*$Id: mixer_tools.c,v 1.10 2004/03/15 16:43:10 clapies Exp $*/


#include "mixer_tools.h"
#include "mixer_defines.h"

#ifdef _USE_16BIT_
#define __MULHHS				st20_f16_mulr_fract16_fract16
#else
#define __MULHHS				st20_Q30_mulQ31xQ31
#define st20_Q30_mulQ31xQ31		st20_Q30_mulQ31xQ31
#endif
#ifdef _ST20_C_
#include "st20_utils.h"
__inline short st20_f16_mulr_fract16_fract16(short r0, short r1)
{
#ifndef _ST20_ASM_
    int t0 = EXT16(r0, 32);
    int t1 = EXT16(r1, 32);
    int t2,t3;

    if (((-r0) == 0x8000) && ((-r1) == 0x8000)){
        t3 = 0x7FFF;
    }
    else{
        t2 = t0 * t1;
        t2 = t2 + (1 << 14);
        t3 = t2 >> 15;
/*      t3 = t2;*/

    }
#else
    int t3;
    __optasm{
        ldab r0,r1;
        smul;
        ldc 16  ;
        ashr;
        stl t3;
    }
#endif
    return t3;

}
__inline int st20_Q30_mulQ31xQ31(int a,int b)
{
#ifndef _ST20_ASM_
    long d1,d2,d3,d4,d5,d6,d7,d8,d9,d10,d11;
    d1 = (long)(a>>16);
    d2 = (long)(b>>16);
    {
        d3 = d1*d2;
        d4 = (long)((unsigned short)a);
        d5 = (long)((unsigned short)b);
        d6 = d2*d4;
        d7 = d1*d5;
        d8 = d6>>16;
        d9 = d7>>16;
        d10 = d3;
        d11 = d8+d9+d10;

    }
#else
    int d11;
    d11=0;
    __optasm{
        ldabc a,b,d11;
        mac;
        rev;
        stl d11;
    }
#endif
    return (int)d11;
}
#endif
// Note to Gael....
//chzscale
//... MULHHS(0x7fff, 0x7fff) => 0x3fff (at least on pc simulation)
// so there is a built in >>1 (normal for integer processors, but dsp often have a built in <<1 to compensate)
// to compensate you may want to use scale_shift-1 instead of scale_shift...
// but maybe you expected this ???? (but then saturation compare code might need looked at) ???
//
// Also 0x7fff * 0x7fffffff -> 0x7ffefffe here which is only transparent for 15 bits.. is this good enough?
/***********************************************************************************/
/************* 8 Bit to 16 bit sample conversion Ayaz 17/09/2007********************/
/***********************************************************************************/
#ifdef STANDALONE
enum  eErrorCode
#else
enum eAccBoolean
#endif
mixer_8bit_16bit_buffer(tPcmBuffer * pcmin,tPcmBuffer * pcmout , int nbspl ,
                        int processed_spl )
{
#ifdef STANDALONE
    enum  eErrorCode status = ACC_TRUE ;
    enum eMainChannelIdx   ch_in, ch_out;
#else
    enum eAccBoolean          status = ACC_MME_TRUE ;
    enum eAccMainChannelIdx   ch_in;
#endif

    int                       nch_in, nch_out, nch, spl;
    short	* spl_out;
    short     input;
    char	* spl_in,in_byte;

    int       in_idx   , out_idx;
    int       in_offset, out_offset;

    nch_in  = pcmin->NbChannels;
    nch_out = pcmout->NbChannels;
    nch     = nch_in ;

//	ch_out  = first_out;
    in_offset = pcmin->Offset;
    out_offset = pcmout->Offset;
#ifndef STANDALONE
    ch_in = ACC_MAIN_LEFT;
#else
    ch_in = MAIN_LEFT;
#endif
    for (; ch_in < nch ; ch_in++ )
    {
        spl_in  = (char *)pcmin->SamplesPtr[ch_in] + ( processed_spl * in_offset ) ;
        spl_out = (short *)pcmout->SamplesPtr[ch_in] + ( processed_spl * out_offset ) ;

        in_idx = 0;
        out_idx = 0;
#ifndef STANDALONE
        pcmout->ChannelExist[ch_in] = ACC_MME_TRUE;
#else
        pcmout->ChannelExist[ch_in] = ACC_TRUE;
#endif
        for ( spl = 0; spl < nbspl; spl++ )
        {
            in_byte = spl_in[in_idx];
            in_idx  += in_offset;
            input = in_byte<<0x8;
            spl_out[out_idx] = input;
            out_idx += out_offset;
        }
    }
    return ( status );
}
#ifdef STANDALONE
enum  eErrorCode
#else
enum eAccBoolean
#endif
mixer_swap_input(tPcmBuffer *pcmin,tPcmBuffer *pcmout, int nbspl ,
                 int processed_spl )
{
#ifdef STANDALONE
    enum  eErrorCode status = ACC_TRUE ;
    enum  eMainChannelIdx   ch_in, ch_out;
#else
    enum eAccBoolean          status = ACC_MME_TRUE ;
    enum eAccMainChannelIdx   ch_in;
#endif
    int                       nch_in, nch_out, nch, spl,tmp;


    short	* spl_out;
    short     input ;
    short	* spl_in,in_byte;
    int       in_idx   , out_idx;
    int       in_offset, out_offset;
    nch_in  = pcmin->NbChannels;
    nch_out = pcmin->NbChannels ;
    nch     = pcmin->NbChannels;
 //   ch_out  = first_out;
    in_offset = pcmin->Offset;
    out_offset = pcmout->Offset;
#ifndef STANDALONE
	ch_in = ACC_MAIN_LEFT;
#else
	ch_in = MAIN_LEFT;
#endif
    for (; ch_in < nch ; ch_in++ )
    {
        spl_in  = (short *)pcmin->SamplesPtr[ch_in] + ( processed_spl * in_offset ) ;
        spl_out = (short *)pcmout->SamplesPtr[ch_in] + ( processed_spl * out_offset ) ;
        in_idx = 0;
        out_idx = 0;
#ifndef STANDALONE
        pcmout->ChannelExist[ch_in] = ACC_MME_TRUE;
#else
		pcmout->ChannelExist[ch_in] = ACC_TRUE;
#endif
        for ( spl = 0; spl < nbspl; spl++ )
        {
            in_byte = spl_in[in_idx];
            in_idx  += in_offset;
            input = in_byte<<0x8;
            tmp = (unsigned short)in_byte>>0x8;
            input = input|tmp;
            spl_out[out_idx] = input;
            out_idx += out_offset;
        }
    }
    return ( status );
}
/********************************************************************************/
/********************Mixer_8bit_mix_saturate Ayaz 17/09/2007*********************/
/********************************************************************************/
#ifdef STANDALONE
enum  eErrorCode
mixer_8bit_mix_saturate( tPcmBuffer * pcmin0,tPcmBuffer * pcmin1,
                         tPcmBuffer * pcmout, short * AlphaMain,
                         short * AlphaSec,
                         enum eMainChannelIdx first_out_main,
                         enum eMainChannelIdx first_out_sec,
                         int scale_shift, int nbspl,
                         int processed_spl)
#else
enum eAccBoolean
mixer_8bit_mix_saturate( tPcmBuffer * pcmin0,tPcmBuffer * pcmin1,
                         tPcmBuffer * pcmout, short * AlphaMain,
                         short * AlphaSec,
                         enum eAccMainChannelIdx first_out_main,
                         enum eAccMainChannelIdx first_out_sec,
                         int scale_shift, int nbspl,
                         int processed_spl)
#endif
{
#ifdef STANDALONE
    enum  eErrorCode status = ACC_TRUE ;
    enum eMainChannelIdx ch_in, ch_out;
#else
    enum eAccBoolean          status = ACC_MME_TRUE ;
    enum eAccMainChannelIdx ch_in;
#endif
    int                  in_offset,out_offset;
    int                  nch_in, nch_out, nch,tmp1,tmp2,alpha_main,alpha_sec;


    char * spl_in0, *spl_in1,in1,in2;
    short	* spl_out;
    short   out;
    int       in_idx, out_idx,length;
    short   min, max,max_l,min_l;
    short   input0,input1, output;

    nch_in  = pcmin0->NbChannels;
    nch_out = pcmout->NbChannels - first_out_main;
    nch     = ( nch_in < nch_out ) ? nch_in : nch_out;
    in_offset = pcmin0->Offset;
    out_offset = pcmout->Offset;
    max = ( int ) MIXER_MAX_Q31 >> (scale_shift+16) ;
#ifndef _ST20_ASM_
    min = ( int ) MIXER_MIN_Q31 >> (scale_shift+16) ;
#else
    tmp1= MIXER_MIN_Q31;
    tmp2 = (scale_shift+16);
    __optasm{
        ldab tmp2,tmp1;
        ashr;
        st min;
    }
#endif

/* Mix input to output */
#ifdef _STANDALONE_
    for ( ch_in = MAIN_LEFT; ch_in < nch ; ch_in++ )
#else
	for ( ch_in = ACC_MAIN_LEFT; ch_in < nch ; ch_in++ )
#endif
    {
//this part only
        alpha_main=AlphaMain[ch_in];
        alpha_sec =AlphaSec[ch_in];
        max_l=(st20_f16_mulr_fract16_fract16(max,AlphaSec[ch_in+2])<<1);
        min_l=(st20_f16_mulr_fract16_fract16(min,AlphaSec[ch_in+2])<<1);
        spl_in0  = (char*)(pcmin0->SamplesPtr[ch_in]) + ( processed_spl * in_offset )  ;
        spl_in1  = (char*)(pcmin1->SamplesPtr[ch_in]) + ( processed_spl * in_offset )  ;
        spl_out = (short*)(pcmout->SamplesPtr[ch_in]) + ( processed_spl * out_offset )  ;
        in_idx = 0;
        out_idx = 0;
#ifdef _STANDALONE_
        pcmout->ChannelExist[ch_in] = ACC_TRUE;
#else
		pcmout->ChannelExist[ch_in] = ACC_MME_TRUE;
#endif
        length = nbspl/8;
        if(in_offset==1)
        {
            do
            {

                in1 = spl_in0[0];
                input0 =(short)in1<<8;
                out =  __MULHHS ( input0, alpha_main );
                in2 = spl_in1[0];
                input1 =(short)in2<<8;
                output=__MULHHS ( input1, alpha_sec );
#ifndef _ST20_ASM_
                out = ( out >> scale_shift ) +  (output>>scale_shift);
                out = ( out < min_l ) ? min_l : out;
                out = ( out > max_l ) ? max_l : out;
                out = ( out << scale_shift )  ;
#else
                out=out+output;
                __optasm{
                    ldab out,min_l;
                    order;
                    rot;
                    ld max_l;
                    order;
                    ldc 1;
                    shl;
                    st out;
                }
#endif
                spl_out[0]= out;

                in1 = spl_in0[1];
                input0 =(short)in1<<8;

                out =  __MULHHS ( input0, alpha_main );
                in2 = spl_in1[1];
                input1 =(short)in2<<8;

                output=__MULHHS ( input1, alpha_sec );
#ifndef _ST20_ASM_
                out = ( out >> scale_shift ) +  (output>>scale_shift);								out = ( out < min_l ) ? min_l : out;
                out = ( out > max_l ) ? max_l : out;
                out = ( out << scale_shift )  ;

#else
                out=out+output;
                __optasm{
                    ldab out,min_l;
                    order;
                    rot;
                    ld max_l;
                    order;
                    ldc 1;
                    shl;
                    st out;
                }

#endif
                spl_out[1]=out;

                in1 = spl_in0[2];
                input0 =(short)in1<<8;
                out =  __MULHHS ( input0, alpha_main );

                in2 = spl_in1[2];
                input1 =(short)in2<<8;
                output=__MULHHS ( input1, alpha_sec );
#ifndef _ST20_ASM_
                out = ( out >> scale_shift ) +  (output>>scale_shift);
                out = ( out < min_l ) ? min_l : out;
                out = ( out > max_l ) ? max_l : out;
                out = ( out << scale_shift )  ;
#else
                out = out+output;
                __optasm{
                    ldab out,min_l;
                    order;
                    rot;
                    ld max_l;
                    order;
                    ldc 1;
                    shl;
                    st out;
                }

#endif


                spl_out[2]=out;
                in1 = spl_in0[3];
                input0 =(short)in1<<8;
                out =  __MULHHS ( input0, alpha_main );
                in2 = spl_in1[3];
                input1 =(short)in2<<8;
                output=__MULHHS ( input1, alpha_sec );
#ifndef _ST20_ASM_
                out = ( out >> scale_shift ) +  (output>>scale_shift);
                out = ( out < min_l ) ? min_l : out;
                out = ( out > max_l ) ? max_l : out;
                out = ( out << scale_shift )  ;
#else
                out=out+output;
                __optasm{
                    ldab out,min_l;
                    order;
                    rot;
                    ld max_l;
                    order;
                    ldc 1;
                    shl;
                    st out;
                }
#endif

                spl_out[3] = out;

                in1 = spl_in0[4];
                input0 =(short)in1<<8;

                out =  __MULHHS ( input0, alpha_main );
                in2 = spl_in1[4];
                input1 =(short)in2<<8;
                output=__MULHHS ( input1, alpha_sec );
#ifndef _ST20_ASM_
                out = ( out >> scale_shift ) +  (output>>scale_shift);
                out = ( out < min_l ) ? min_l : out;
                out = ( out > max_l ) ? max_l : out;
                out = ( out << scale_shift )  ;
#else
                out = out+output;
                __optasm{
                    ldab out,min_l;
                    order;
                    rot;
                    ld max_l;
                    order;
                    ldc 1;
                    shl;
                    st out;
                }
#endif

                spl_out[4] = out;
                in1 = spl_in0[5];
                input0 =(short)in1<<8;
                out =  __MULHHS ( input0, alpha_main );
                in2 = spl_in1[5];
                input1 =(short)in2<<8;
                output=__MULHHS ( input1, alpha_sec );
#ifndef _ST20_ASM_
                out = ( out >> scale_shift ) +  (output>>scale_shift);
                out = ( out < min_l ) ? min_l : out;
                out = ( out > max_l ) ? max_l : out;
                out = ( out << scale_shift )  ;
#else
                out=out+output;
                __optasm{
                    ldab out,min_l;
                    order;
                    rot;
                    ld max_l;
                    order;
                    ldc 1;
                    shl;
                    st out;
                }
#endif


                spl_out[5]=out;
                in1 = spl_in0[6];
                input0 =(short)in1<<8;
                out =  __MULHHS ( input0, alpha_main );
                in2 = spl_in1[6];
                input1 =(short)in2<<8;
                output=__MULHHS ( input1, alpha_sec );
#ifndef _ST20_ASM_
                out = ( out >> scale_shift ) +  (output>>scale_shift);
                out = ( out < min_l ) ? min_l : out;
                out = ( out > max_l ) ? max_l : out;
                out = ( out << scale_shift )  ;
#else
                out=out+output;
                __optasm{
                    ldab out,min_l;
                    order;
                    rot;
                    ld max_l;
                    order;
                    ldc 1;
                    shl;
                    st out;
                }
#endif


                spl_out[6]=out;
                in1 = spl_in0[7];
                input0 =(short)in1<<8;
                out =  __MULHHS ( input0, alpha_main );
                in2 = spl_in1[7];
                input1 =(short)in2<<8;
                output=__MULHHS ( input1, alpha_sec );
#ifndef _ST20_ASM_
                out = ( out >> scale_shift ) +  (output>>scale_shift);
                out = ( out < min_l ) ? min_l : out;
                out = ( out > max_l ) ? max_l : out;
                out = ( out << scale_shift )  ;
#else
                out=out+output;
                __optasm{
                    ldab out,min_l;
                    order;
                    rot;
                    ld max_l;
                    order;
                    ldc 1;
                    shl;
                    st out;
                }
#endif


                spl_out[7]=out;
                spl_out=&spl_out[8];
                spl_in0 = &spl_in0[8];
                spl_in1 = &spl_in1[8];
            }while(--length); /* End for all samples */
        }
        else
        {

            do
            {
                in1 = spl_in0[0];
                input0 =(short)in1<<8;
                out =  __MULHHS ( input0, alpha_main );
                in2 = spl_in1[0];
                input1 =(short)in2<<8;
                output=__MULHHS ( input1, alpha_sec );
#ifndef _ST20_ASM_
                out = ( out >> scale_shift ) +  (output>>scale_shift);
#else
                __optasm{
                    ldab out,output;
                    add;
                    ldc 1;
                    ashr;
                    st out;
                }

#endif
                out = ( out < min_l ) ? min_l : out;
                out = ( out > max_l ) ? max_l : out;
                out = ( out << scale_shift )  ;

                spl_out[0]= out;
                in1 = spl_in0[2];
                input0 =(short)in1<<8;
                out =  __MULHHS ( input0, alpha_main );
                in2 = spl_in1[2];
                input1 =(short)in2<<8;
                output=__MULHHS ( input1, alpha_sec );
#ifndef _ST20_ASM_
                out = ( out >> scale_shift ) +  (output>>scale_shift);
#else
                __optasm{
                    ldab out,output;
                    add;
                    ldc 1;

                    ashr;
                    st out;

                }

#endif
                out = ( out < min_l ) ? min_l : out;
                out = ( out > max_l ) ? max_l : out;
                out = ( out << scale_shift )  ;

                spl_out[2]=out;
                in1 = spl_in0[4];
                input0 =(short)in1<<8;
                out =  __MULHHS ( input0, alpha_main );
                in2 = spl_in1[4];
                input1 =(short)in2<<8;
                output=__MULHHS ( input1, alpha_sec );
#ifndef _ST20_ASM_
                out = ( out >> scale_shift ) +  (output>>scale_shift);
#else
                __optasm{
                    ldab out,output;
                    add;
                    ldc 1;

                    ashr;
                    st out;

                }

#endif
                out = ( out < min_l ) ? min_l : out;
                out = ( out > max_l ) ? max_l : out;
                out = ( out << scale_shift )  ;

                spl_out[4]=out;
                in1 = spl_in0[6];
                input0 =(short)in1<<8;
                out =  __MULHHS ( input0, alpha_main );
                in2 = spl_in1[6];
                input1 =(short)in2<<8;
                output=__MULHHS ( input1, alpha_sec );
#ifndef _ST20_ASM_
                out = ( out >> scale_shift ) +  (output>>scale_shift);
#else
                __optasm{
                    ldab out,output;
                    add;
                    ldc 1;

                    ashr;
                    st out;

                }

#endif
                out = ( out < min_l ) ? min_l : out;
                out = ( out > max_l ) ? max_l : out;
                out = ( out << scale_shift )  ;

                spl_out[6] = out;

                in1 = spl_in0[8];
                input0 =(short)in1<<8;
                out =  __MULHHS ( input0, alpha_main );
                in2 = spl_in1[8];
                input1 =(short)in2<<8;
                output=__MULHHS ( input1, alpha_sec );
#ifndef _ST20_ASM_
                out = ( out >> scale_shift ) +  (output>>scale_shift);
#else
                __optasm{
                    ldab out,output;
                    add;
                    ldc 1;

                    ashr;
                    st out;

                }

#endif
                out = ( out < min_l ) ? min_l : out;
                out = ( out > max_l ) ? max_l : out;
                out = ( out << scale_shift )  ;

                spl_out[8] = out;
                in1 = spl_in0[10];
                input0 =(short)in1<<8;
                out =  __MULHHS ( input0, alpha_main );
                in2 = spl_in1[10];
                input1 =(short)in2<<8;
                output=__MULHHS ( input1, alpha_sec );
#ifndef _ST20_ASM_
                out = ( out >> scale_shift ) +  (output>>scale_shift);
#else
                __optasm{
                    ldab out,output;
                    add;
                    ldc 1;

                    ashr;
                    st out;

                }

#endif
                out = ( out < min_l ) ? min_l : out;
                out = ( out > max_l ) ? max_l : out;
                out = ( out << scale_shift )  ;

                spl_out[10]=out;
                in1 = spl_in0[12];
                input0 =(short)in1<<8;
                out =  __MULHHS ( input0, alpha_main );
                in2 = spl_in1[12];
                input1 =(short)in2<<8;
                output=__MULHHS ( input1, alpha_sec );
#ifndef _ST20_ASM_
                out = ( out >> scale_shift ) +  (output>>scale_shift);
#else
                __optasm{
                    ldab out,output;
                    add;
                    ldc 1;

                    ashr;
                    st out;

                }

#endif
                out = ( out < min_l ) ? min_l : out;
                out = ( out > max_l ) ? max_l : out;
                out = ( out << scale_shift )  ;

                spl_out[12]=out;
                in1 = spl_in0[14];
                input0 =(short)in1<<8;
                out =  __MULHHS ( input0, alpha_main );
                in2 = spl_in1[14];
                input1 =(short)in2<<8;
                output=__MULHHS ( input1, alpha_sec );
#ifndef _ST20_ASM_
                out = ( out >> scale_shift ) +  (output>>scale_shift);
#else
                __optasm{
                    ldab out,output;
                    add;
                    ldc 1;
                    ashr;
                    st out;
                }
#endif
                out = ( out < min_l ) ? min_l : out;
                out = ( out > max_l ) ? max_l : out;
                out = ( out << scale_shift )  ;

                spl_out[14]=out;
                spl_out=&spl_out[16];
                spl_in0 = &spl_in0[16];
                spl_in1 = &spl_in1[16];
            }while(--length); /* End for all samples */
        }
    }
    return ( status );
}

/************************************************************************/
/********** Mixer_swap_mix_saturate Ayaz 17/09/2007 *********************/
/************************************************************************/
#ifdef STANDALONE
enum  eErrorCode mixer_swap_mix_saturate( tPcmBuffer * pcmin0,tPcmBuffer * pcmin1,
                         tPcmBuffer * pcmout, short * AlphaMain,
                         short * AlphaSec,
                         enum eMainChannelIdx first_out_main,
                         enum eMainChannelIdx first_out_sec,
                         int scale_shift, int nbspl,
                         int processed_spl)
#else
enum eAccBoolean mixer_swap_mix_saturate( tPcmBuffer * pcmin0,tPcmBuffer * pcmin1,
                         tPcmBuffer * pcmout, short * AlphaMain,
                         short * AlphaSec,
                         enum eAccMainChannelIdx first_out_main,
                         enum eAccMainChannelIdx first_out_sec,
                         int scale_shift, int nbspl,
                         int processed_spl)
#endif
{
#ifdef STANDALONE
    enum  eErrorCode status = ACC_TRUE ;
        enum eMainChannelIdx ch_in, ch_out;
#else
    enum eAccBoolean          status = ACC_MME_TRUE ;
        enum eAccMainChannelIdx ch_in;
#endif
    int                  in_offset,out_offset;
    int                  nch_in, nch_out, nch,tmp1,tmp2,alpha_main,alpha_sec;

    short * spl_in0, *spl_in1,in1,in2;
     short	* spl_out,ins1,ins2;
    short   out;
    int       in_idx, out_idx,length;
    short   min, max,max_l,min_l;
    short   input0,input1, output;

    nch_in  = pcmin0->NbChannels;
    nch_out = pcmout->NbChannels - first_out_main;
    nch     = ( nch_in < nch_out ) ? nch_in : nch_out;

    in_offset = pcmin0->Offset;
    out_offset = pcmout->Offset;
    max = ( int ) MIXER_MAX_Q31 >> (scale_shift+16) ;
#ifndef _ST20_ASM_
    min = ( int ) MIXER_MIN_Q31 >> (scale_shift+16) ;
#else
    tmp1= MIXER_MIN_Q31;
    tmp2 = (scale_shift+16);
    __optasm{
        ldab tmp2,tmp1;
        ashr;
        st min;
    }
#endif

/* Mix input to output */
    #ifdef _STANDALONE_
    for ( ch_in = MAIN_LEFT; ch_in < nch ; ch_in++ )
#else
    for ( ch_in = ACC_MAIN_LEFT; ch_in < nch ; ch_in++ )
#endif
    {
//this part only
        alpha_main=AlphaMain[ch_in];
        alpha_sec =AlphaSec[ch_in];
        max_l=(st20_f16_mulr_fract16_fract16(max,AlphaSec[ch_in+2])<<1);
        min_l=(st20_f16_mulr_fract16_fract16(min,AlphaSec[ch_in+2])<<1);
        spl_in0  = (short*)(pcmin0->SamplesPtr[ch_in]) + ( processed_spl * in_offset )  ;
        spl_in1  = (short*)(pcmin1->SamplesPtr[ch_in]) + ( processed_spl * in_offset )  ;
        spl_out = (short *)(pcmout->SamplesPtr[ch_in]) + ( processed_spl * out_offset )  ;
        in_idx = 0;
        out_idx = 0;
#ifdef _STANDALONE_
        pcmout->ChannelExist[ch_in] = ACC_TRUE;
#else
		pcmout->ChannelExist[ch_in] = ACC_MME_TRUE;
#endif
        length = nbspl/8;
        if(in_offset==1)
        {
            do
            {

                in1 = spl_in0[0];
                ins1    = (unsigned short)in1>>8;
                input0 = in1<<8;
                input0 = input0|ins1;
                out =  __MULHHS ( input0, alpha_main );
                in2 = spl_in1[0];
                ins2    = (unsigned short)in2>>8;
                input1 =in2<<8;
                input1 =input1 |ins2;
                output=__MULHHS ( input1, alpha_sec );
#ifndef _ST20_ASM_
                out = ( out >> scale_shift ) +  (output>>scale_shift);
                out = ( out < min_l ) ? min_l : out;
                out = ( out > max_l ) ? max_l : out;
                out = ( out << scale_shift )  ;
#else
                out=out+output;
                __optasm{
                    ldab out,min_l;
                    order;
                    rot;
                    ld max_l;
                    order;
                    ldc 1;
                    shl;
                    st out;
                }
#endif
                spl_out[0]= out;

                in1 = spl_in0[1];
                ins1    = (unsigned short)in1>>8;
                input0 = in1<<8;
                input0 = input0|ins1;
                out =  __MULHHS ( input0, alpha_main );
                in2 = spl_in1[1];
                ins2    = (unsigned short)in2>>8;
                input1 =in2<<8;
                input1 =input1|ins2;

                output=__MULHHS ( input1, alpha_sec );
#ifndef _ST20_ASM_
                out = ( out >> scale_shift ) +  (output>>scale_shift);								out = ( out < min_l ) ? min_l : out;
                out = ( out > max_l ) ? max_l : out;
                out = ( out << scale_shift )  ;

#else
                out=out+output;
                __optasm{
                    ldab out,min_l;
                    order;
                    rot;
                    ld max_l;
                    order;
                    ldc 1;
                    shl;
                    st out;
                }

#endif
                spl_out[1]=out;

                in1 = spl_in0[2];
                ins1    = (unsigned short)in1>>8;
                input0 = in1<<8;
                input0 = input0|ins1;
                out =  __MULHHS ( input0, alpha_main );

                in2 = spl_in1[2];
                ins2    = (unsigned short)in2>>8;
                input1 =in2<<8;
                input1 =input1|ins2;
                output=__MULHHS ( input1, alpha_sec );
#ifndef _ST20_ASM_
                out = ( out >> scale_shift ) +  (output>>scale_shift);
                out = ( out < min_l ) ? min_l : out;
                out = ( out > max_l ) ? max_l : out;
                out = ( out << scale_shift )  ;
#else
                out = out+output;
                __optasm{
                    ldab out,min_l;
                    order;
                    rot;
                    ld max_l;
                    order;
                    ldc 1;
                    shl;
                    st out;
                }

#endif


                spl_out[2]=out;
                in1 = spl_in0[3];
                ins1    = (unsigned short)in1>>8;
                input0 = in1<<8;
                input0 = input0|ins1;
                out =  __MULHHS ( input0, alpha_main );
                in2 = spl_in1[3];
                ins2    = (unsigned short)in2>>8;
                input1 =in2<<8;
                input1 =input1|ins2;
                output=__MULHHS ( input1, alpha_sec );
#ifndef _ST20_ASM_
                out = ( out >> scale_shift ) +  (output>>scale_shift);
                out = ( out < min_l ) ? min_l : out;
                out = ( out > max_l ) ? max_l : out;
                out = ( out << scale_shift )  ;
#else
                out=out+output;
                __optasm{
                    ldab out,min_l;
                    order;
                    rot;
                    ld max_l;
                    order;
                    ldc 1;
                    shl;
                    st out;
                }
#endif

                spl_out[3] = out;

                in1 = spl_in0[4];
                ins1    = (unsigned short)in1>>8;
                input0 = in1<<8;
                input0 = input0|ins1;

                out =  __MULHHS ( input0, alpha_main );
                in2 = spl_in1[4];
                ins2    = (unsigned short)in2>>8;
                input1 =in2<<8;
                input1 =input1|ins2;
                output=__MULHHS ( input1, alpha_sec );
#ifndef _ST20_ASM_
                out = ( out >> scale_shift ) +  (output>>scale_shift);
                out = ( out < min_l ) ? min_l : out;
                out = ( out > max_l ) ? max_l : out;
                out = ( out << scale_shift )  ;
#else
                out = out+output;
                __optasm{
                    ldab out,min_l;
                    order;
                    rot;
                    ld max_l;
                    order;
                    ldc 1;
                    shl;
                    st out;
                }
#endif

                spl_out[4] = out;
                in1 = spl_in0[5];
                ins1    = (unsigned short)in1>>8;
                input0 = in1<<8;
                input0 = input0|ins1;
                out =  __MULHHS ( input0, alpha_main );
                in2 = spl_in1[5];
                ins2    = (unsigned short)in2>>8;
                input1 =in2<<8;
                input1 =input1|ins2;
                output=__MULHHS ( input1, alpha_sec );
#ifndef _ST20_ASM_
                out = ( out >> scale_shift ) +  (output>>scale_shift);
                out = ( out < min_l ) ? min_l : out;
                out = ( out > max_l ) ? max_l : out;
                out = ( out << scale_shift )  ;
#else
                out=out+output;
                __optasm{
                    ldab out,min_l;
                    order;
                    rot;
                    ld max_l;
                    order;
                    ldc 1;
                    shl;
                    st out;
                }
#endif


                spl_out[5]=out;
                in1 = spl_in0[6];
                ins1    = (unsigned short)in1>>8;
                input0 = in1<<8;
                input0 = input0|ins1;
                out =  __MULHHS ( input0, alpha_main );
                in2 = spl_in1[6];
                ins2    = (unsigned short)in2>>8;
                input1 =in2<<8;
                input1 =input1|ins2;
                output=__MULHHS ( input1, alpha_sec );
#ifndef _ST20_ASM_
                out = ( out >> scale_shift ) +  (output>>scale_shift);
                out = ( out < min_l ) ? min_l : out;
                out = ( out > max_l ) ? max_l : out;
                out = ( out << scale_shift )  ;
#else
                out=out+output;
                __optasm{
                    ldab out,min_l;
                    order;
                    rot;
                    ld max_l;
                    order;
                    ldc 1;
                    shl;
                    st out;
                }
#endif


                spl_out[6]=out;
                in1 = spl_in0[7];
                ins1    = (unsigned short)in1>>8;
                input0 = in1<<8;
                input0 = input0|ins1;
                out =  __MULHHS ( input0, alpha_main );
                in2 = spl_in1[7];
                ins2    = (unsigned short)in2>>8;
                input1 =in2<<8;
                input1 =input1|ins2;
                output=__MULHHS ( input1, alpha_sec );
#ifndef _ST20_ASM_
                out = ( out >> scale_shift ) +  (output>>scale_shift);
                out = ( out < min_l ) ? min_l : out;
                out = ( out > max_l ) ? max_l : out;
                out = ( out << scale_shift )  ;
#else
                out=out+output;
                __optasm{
                    ldab out,min_l;
                    order;
                    rot;
                    ld max_l;
                    order;
                    ldc 1;
                    shl;
                    st out;
                }
#endif


                spl_out[7]=out;
                spl_out=&spl_out[8];
                spl_in0 = &spl_in0[8];
                spl_in1 = &spl_in1[8];
            }while(--length); /* End for all samples */
        }
        else
        {

            do
            {
                in1 = spl_in0[0];
                ins1    = (unsigned short)in1>>8;
                input0 = in1<<8;
                input0 = input0|ins1;
                out =  __MULHHS ( input0, alpha_main );
                in2 = spl_in1[0];
                ins2    = (unsigned short)in2>>8;
                input1 =in2<<8;
                input1 =input1|ins2;
                output=__MULHHS ( input1, alpha_sec );
#ifndef _ST20_ASM_
                out = ( out >> scale_shift ) +  (output>>scale_shift);
#else
                __optasm{
                    ldab out,output;
                    add;
                    ldc 1;
//				ldab scale_shift,out;
                    ashr;
                    st out;
//				ldab scale_shift,output;
//				ashr;
//				st tmp2;
                }
//				out=tmp1+tmp2;
#endif
                out = ( out < min_l ) ? min_l : out;
                out = ( out > max_l ) ? max_l : out;
                out = ( out << scale_shift )  ;

                spl_out[0]= out;
                in1 = spl_in0[2];
                ins1    = (unsigned short)in1>>8;
                input0 = in1<<8;
                input0 = input0|ins1;
                out =  __MULHHS ( input0, alpha_main );
                in2 = spl_in1[2];
                ins2    = (unsigned short)in2>>8;
                input1 =in2<<8;
                input1 =input1|ins2;
                output=__MULHHS ( input1, alpha_sec );
#ifndef _ST20_ASM_
                out = ( out >> scale_shift ) +  (output>>scale_shift);
#else
                __optasm{
                    ldab out,output;
                    add;
                    ldc 1;
//				ldab scale_shift,out;
                    ashr;
                    st out;
//				ldab scale_shift,output;
//				ashr;
//				st tmp2;
                }
//				out=tmp1+tmp2;
#endif
                out = ( out < min_l ) ? min_l : out;
                out = ( out > max_l ) ? max_l : out;
                out = ( out << scale_shift )  ;

                spl_out[2]=out;
                in1 = spl_in0[4];
                ins1    = (unsigned short)in1>>8;
                input0 = in1<<8;
                input0 = input0|ins1;
                out =  __MULHHS ( input0, alpha_main );
                in2 = spl_in1[4];
                ins2    = (unsigned short)in2>>8;
                input1 =in2<<8;
                input1 =input1|ins2;
                output=__MULHHS ( input1, alpha_sec );
#ifndef _ST20_ASM_
                out = ( out >> scale_shift ) +  (output>>scale_shift);
#else
                __optasm{
                    ldab out,output;
                    add;
                    ldc 1;
//				ldab scale_shift,out;
                    ashr;
                    st out;
//				ldab scale_shift,output;
//				ashr;
//				st tmp2;
                }
//				out=tmp1+tmp2;
#endif
                out = ( out < min_l ) ? min_l : out;
                out = ( out > max_l ) ? max_l : out;
                out = ( out << scale_shift )  ;

                spl_out[4]=out;
                in1 = spl_in0[6];
                ins1    = (unsigned short)in1>>8;
                input0 = in1<<8;
                input0 = input0|ins1;
                out =  __MULHHS ( input0, alpha_main );
                in2 = spl_in1[6];
                ins2    = (unsigned short)in2>>8;
                input1 =in2<<8;
                input1 =input1|ins2;
                output=__MULHHS ( input1, alpha_sec );
#ifndef _ST20_ASM_
                out = ( out >> scale_shift ) +  (output>>scale_shift);
#else
                __optasm{
                    ldab out,output;
                    add;
                    ldc 1;
//				ldab scale_shift,out;
                    ashr;
                    st out;
//				ldab scale_shift,output;
//				ashr;
//				st tmp2;
                }
//				out=tmp1+tmp2;
#endif
                out = ( out < min_l ) ? min_l : out;
                out = ( out > max_l ) ? max_l : out;
                out = ( out << scale_shift )  ;

                spl_out[6] = out;

                in1 = spl_in0[8];
                ins1    = (unsigned short)in1>>8;
                input0 = in1<<8;
                input0 = input0|ins1;
                out =  __MULHHS ( input0, alpha_main );
                in2 = spl_in1[8];
                ins2    = (unsigned short)in2>>8;
                input1 =in2<<8;
                input1 =input1|ins2;
                output=__MULHHS ( input1, alpha_sec );
#ifndef _ST20_ASM_
                out = ( out >> scale_shift ) +  (output>>scale_shift);
#else
                __optasm{
                    ldab out,output;
                    add;
                    ldc 1;
//				ldab scale_shift,out;
                    ashr;
                    st out;
//				ldab scale_shift,output;
//				ashr;
//				st tmp2;
                }
//				out=tmp1+tmp2;
#endif
                out = ( out < min_l ) ? min_l : out;
                out = ( out > max_l ) ? max_l : out;
                out = ( out << scale_shift )  ;

                spl_out[8] = out;
                in1 = spl_in0[10];
                ins1    = (unsigned short)in1>>8;
                input0 = in1<<8;
                input0 = input0|ins1;
                out =  __MULHHS ( input0, alpha_main );
                in2 = spl_in1[10];
                ins2    = (unsigned short)in2>>8;
                input1 =in2<<8;
                input1 =input1|ins2;
                output=__MULHHS ( input1, alpha_sec );
#ifndef _ST20_ASM_
                out = ( out >> scale_shift ) +  (output>>scale_shift);
#else
                __optasm{
                    ldab out,output;
                    add;
                    ldc 1;
//				ldab scale_shift,out;
                    ashr;
                    st out;
//				ldab scale_shift,output;
//				ashr;
//				st tmp2;
                }
//				out=tmp1+tmp2;
#endif
                out = ( out < min_l ) ? min_l : out;
                out = ( out > max_l ) ? max_l : out;
                out = ( out << scale_shift )  ;

                spl_out[10]=out;
                in1 = spl_in0[12];
                ins1    = (unsigned short)in1>>8;
                input0 = in1<<8;
                input0 = input0|ins1;
                out =  __MULHHS ( input0, alpha_main );
                in2 = spl_in1[12];
                ins2    = (unsigned short)in2>>8;
                input1 =in2<<8;
                input1 =input1|ins2;
                output=__MULHHS ( input1, alpha_sec );
#ifndef _ST20_ASM_
                out = ( out >> scale_shift ) +  (output>>scale_shift);
#else
                __optasm{
                    ldab out,output;
                    add;
                    ldc 1;
//				ldab scale_shift,out;
                    ashr;
                    st out;
//				ldab scale_shift,output;
//				ashr;
//				st tmp2;
                }
//				out=tmp1+tmp2;
#endif
                out = ( out < min_l ) ? min_l : out;
                out = ( out > max_l ) ? max_l : out;
                out = ( out << scale_shift )  ;

                spl_out[12]=out;
                in1 = spl_in0[14];
                ins1    = (unsigned short)in1>>8;
                input0 = in1<<8;
                input0 = input0|ins1;
                out =  __MULHHS ( input0, alpha_main );
                in2 = spl_in1[14];
                ins2    = (unsigned short)in2>>8;
                input1 =in2<<8;
                input1 =input1|ins2;;
                output=__MULHHS ( input1, alpha_sec );
#ifndef _ST20_ASM_
                out = ( out >> scale_shift ) +  (output>>scale_shift);
#else
                __optasm{
                    ldab out,output;
                    add;
                    ldc 1;
//				ldab scale_shift,out;
                    ashr;
                    st out;
//				ldab scale_shift,output;
//				ashr;
//				st tmp2;
                }
//				out=tmp1+tmp2;
#endif
                out = ( out < min_l ) ? min_l : out;
                out = ( out > max_l ) ? max_l : out;
                out = ( out << scale_shift )  ;

                spl_out[14]=out;
                spl_out=&spl_out[16];
                spl_in0 = &spl_in0[16];
                spl_in1 = &spl_in1[16];
            }while(--length); /* End for all samples */
        }
    }
    return ( status );
}
#ifndef _USE_16BIT_
enum eAccBoolean mixer_attenuate_buffer ( tPcmBuffer * pcmin, tPcmBuffer * pcmout, int *AlphaNew, enum eAccMainChannelIdx first_out, int scale_shift, int nbspl, int processed_spl)
{
    enum eAccBoolean          status = ACC_MME_TRUE ;
    int                       nch_in, nch_out, nch,tmp,alpha;
    enum eAccMainChannelIdx   ch_in, ch_out;

    tSample * spl_in, * spl_out;
    tSample   input ,   out;

    int       in_idx   , out_idx;
    int       in_offset, out_offset;

    nch_in  = pcmin->NbChannels;
    nch_out = pcmout->NbChannels - first_out;
    nch     = ( nch_in < nch_out ) ? nch_in : nch_out;

    ch_out  = first_out;
    in_offset = pcmin->Offset;
    out_offset = pcmout->Offset;

    for ( ch_in = ACC_MAIN_LEFT; ch_in < nch ; ch_in++ )
    {
//only this part is executed
        alpha = AlphaNew[ch_in];
        spl_in  = pcmin->SamplesPtr[ch_in] + ( processed_spl * in_offset ) ;
        spl_out = pcmout->SamplesPtr[ch_in] + ( processed_spl * out_offset ) ;

        in_idx = 0;
        out_idx = 0;
        pcmout->ChannelExist[ch_in] = ACC_MME_TRUE;

        for ( spl = 0; spl < nbspl; spl++ )
        {
            input = spl_in[in_idx];
            in_idx  += in_offset;

            out =  __MULHHS ( input, alpha );
            out = out << 1;
#ifndef _ST20_ASM_
            spl_out[out_idx] = ( out >> scale_shift );
#else
            __optasm{
                ldab scale_shift,out;
                ashr;
                st tmp;
            }
            spl_out[out_idx] = tmp;
#endif
            out_idx += out_offset;

        } /* End for all samples */
    }
    return ( status );
}
enum eAccBoolean mixer_mix_buffer_saturate ( tPcmBuffer * pcmin, tPcmBuffer * pcmout, int * AlphaNew, enum eAccMainChannelIdx first_out, int scale_shift, int nbspl, int processed_spl)
{
    enum eAccBoolean     status = ACC_MME_TRUE ;
    int                  in_offset,out_offset;
    int                  nch_in, nch_out, nch, spl,tmp1,tmp2,alpha;
    enum eAccMainChannelIdx ch_in, ch_out;

    tSample * spl_in, * spl_out;
    tSample   out;
    int       in_idx, out_idx;
    tSample   min, max,max_l,min_l;
    tSample   input, output;

    nch_in  = pcmin->NbChannels;
    nch_out = pcmout->NbChannels - first_out;
    nch     = ( nch_in < nch_out ) ? nch_in : nch_out;

    in_offset = pcmin->Offset;
    out_offset = pcmout->Offset;

    max = ( int ) MIXER_MAX_Q31 >> scale_shift ;
#ifndef _ST20_ASM_
    min = ( int ) MIXER_MIN_Q31 >> scale_shift ;
#else
    tmp1= MIXER_MIN_Q31;
    __optasm{
        ldab scale_shift,tmp1;
        ashr;
        st min;
    }
#endif

/* Mix input to output */
    for ( ch_in = ACC_MAIN_LEFT; ch_in < nch ; ch_in++ )
    {
//this part only
        alpha=AlphaNew[ch_in];
        max_l=st20_Q31_mulQ31xQ31(max,AlphaNew[ch_in+2]);
        min_l=st20_Q31_mulQ31xQ31(min,AlphaNew[ch_in+2]);
        spl_in  = pcmin->SamplesPtr[ch_in] + ( processed_spl * in_offset )  ;
        spl_out = pcmout->SamplesPtr[ch_in] + ( processed_spl * out_offset )  ;
        in_idx = 0;
        out_idx = 0;
        pcmout->ChannelExist[ch_in] = ACC_MME_TRUE;

        for ( spl = 0; spl < nbspl; spl++ )
        {
            input = spl_in[in_idx];
            in_idx +=  in_offset;
            output = spl_out[out_idx];

            out = __MULHHS ( input, alpha ) << 1 ;
#ifndef _ST20_ASM_
            out = ( out >> scale_shift ) +  output;
#else
            __optasm{
                ldab scale_shift,out;
                ashr;
                st tmp1;
            }
            out=tmp1+output;
#endif
            out = ( out < min_l ) ? min_l : out;
            out = ( out > max_l ) ? max_l : out;
            out = ( out << scale_shift )  ;
            spl_out[out_idx] = out;
            out_idx += out_offset;
        } /* End for all samples */
    }
    return ( status );
}
#else //_USE_16bit_
#ifdef _STANDALONE_
enum  eErrorCode mixer_attenuate_buffer ( tPcmBuffer * pcmin, tPcmBuffer * pcmout, short *AlphaNew, enum eMainChannelIdx first_out, int scale_shift, int nbspl, int processed_spl)
#else
enum  eAccBoolean  mixer_attenuate_buffer ( tPcmBuffer * pcmin, tPcmBuffer * pcmout, short *AlphaNew, enum eAccMainChannelIdx first_out, int scale_shift, int nbspl, int processed_spl)
#endif
{
#ifdef _STANDALONE_
    enum  eErrorCode     status = ACC_EC_OK ;
    enum eMainChannelIdx   ch_in, ch_out;
#else
 	enum  eAccBoolean     status = ACC_MME_TRUE;
 	enum eAccMainChannelIdx   ch_in;
#endif
    int                       nch_in, nch_out, nch, spl;


    short * spl_in, * spl_out;
    short   input ,   out,alpha;

    int       in_idx   , out_idx;
    int       in_offset, out_offset;

    nch_in  = pcmin->NbChannels;
    nch_out = pcmout->NbChannels - first_out;
    nch     = ( nch_in < nch_out ) ? nch_in : nch_out;

   // ch_out  = first_out;
    in_offset = pcmin->Offset;
    out_offset = pcmout->Offset;

#ifdef _STANDALONE_
    for ( ch_in = MAIN_LEFT; ch_in < nch ; ch_in++ )
#else
	for ( ch_in = ACC_MAIN_LEFT; ch_in < nch ; ch_in++ )
#endif
    {
//only this part is executed
        alpha = AlphaNew[ch_in];
        spl_in  = (short*)(pcmin->SamplesPtr[ch_in]) + ( processed_spl * in_offset ) ;
        spl_out = (short*)(pcmout->SamplesPtr[ch_in]) + ( processed_spl * out_offset ) ;

        in_idx = 0;
        out_idx = 0;
#ifdef _STANDALONE_
        pcmout->ChannelExist[ch_in] = ACC_TRUE;
#else
        pcmout->ChannelExist[ch_in] = ACC_MME_TRUE;
#endif

        for ( spl = 0; spl < nbspl; spl++ )
        {
            input = spl_in[in_idx];
            in_idx  += in_offset;

            out =  __MULHHS ( input, alpha );
//	out = out << 1; //use lsh inside multiplier
//#ifndef _ST20_ASM_
            spl_out[out_idx] = ( out );
/*#else
            __optasm{
                ldab scale_shift,out;
                ashr;
                st tmp;
            }
            spl_out[out_idx] = tmp;
#endif*/
            out_idx += out_offset;

        } /* End for all samples */
    }
    return ( status );
}
#ifdef _STANDALONE_
enum  eErrorCode mixer_mix_buffer_saturate ( tPcmBuffer * pcmin, tPcmBuffer * pcmout, short * AlphaNew, enum eMainChannelIdx first_out, int scale_shift, int nbspl, int processed_spl)
#else
enum  eAccBoolean mixer_mix_buffer_saturate ( tPcmBuffer * pcmin, tPcmBuffer * pcmout, short * AlphaNew, enum eAccMainChannelIdx first_out, int scale_shift, int nbspl, int processed_spl)
#endif
{
#ifdef _STANDALONE_
    enum  eErrorCode     status = ACC_EC_OK ;
    enum eMainChannelIdx ch_in, ch_out;

#else
	enum  eAccBoolean     status = ACC_MME_TRUE;
	enum eAccMainChannelIdx ch_in;

#endif
    int                  in_offset,out_offset;
    int                  nch_in, nch_out, nch, spl,tmp1,tmp2,alpha;


    short * spl_in, * spl_out;
    short   out;
    int       in_idx, out_idx;
    short   min, max,max_l,min_l;
    short   input, output;

    nch_in  = pcmin->NbChannels;
    nch_out = pcmout->NbChannels - first_out;
    nch     = ( nch_in < nch_out ) ? nch_in : nch_out;

    in_offset = pcmin->Offset;
    out_offset = pcmout->Offset;

    max = ( int ) MIXER_MAX_Q31 >> (scale_shift+16) ;
#ifndef _ST20_ASM_
    min = ( int ) MIXER_MIN_Q31 >> (scale_shift+16) ;
#else
    tmp1= MIXER_MIN_Q31;
    tmp2 = (scale_shift+16);
    __optasm{
        ldab tmp2,tmp1;
        ashr;
        st min;
    }
#endif

/* Mix input to output */
#ifdef _STANDALONE_
    for ( ch_in = MAIN_LEFT; ch_in < nch ; ch_in++ )
#else
	for ( ch_in = ACC_MAIN_LEFT; ch_in < nch ; ch_in++ )
#endif
    {
//this part only
        alpha=AlphaNew[ch_in];
        max_l=(st20_f16_mulr_fract16_fract16(max,AlphaNew[ch_in+2]));
        min_l=(st20_f16_mulr_fract16_fract16(min,AlphaNew[ch_in+2]));
        spl_in  = (short*)(pcmin->SamplesPtr[ch_in]) + ( processed_spl * in_offset )  ;
        spl_out = (short *)(pcmout->SamplesPtr[ch_in]) + ( processed_spl * out_offset )  ;
        in_idx = 0;
        out_idx = 0;
#ifdef _STANDALONE_
        pcmout->ChannelExist[ch_in] = ACC_TRUE;
#else
        pcmout->ChannelExist[ch_in] = ACC_MME_TRUE;
#endif

        for ( spl = 0; spl < nbspl; spl++ )
        {
            input = spl_in[in_idx];
            in_idx +=  in_offset;
            output = spl_out[out_idx];

//				out = __MULHHS ( input, alpha ) << 1 ;
            out = __MULHHS ( input, alpha );
#ifndef _ST20_ASM_
            out = ( out >> scale_shift ) +  output;
#else
            __optasm{
                ldab scale_shift,out;
                ashr;
                st tmp1;
            }
            out=tmp1+output;
#endif
            out = ( out < min_l ) ? min_l : out;
            out = ( out > max_l ) ? max_l : out;
            out = ( out << scale_shift )  ;
            spl_out[out_idx] = out;
            out_idx += out_offset;
        } /* End for all samples */
    }
    return ( status );
}
#ifdef _STANDALONE_
enum  eErrorCode mixer_mix_saturate ( tPcmBuffer * pcmin0,tPcmBuffer * pcmin1, tPcmBuffer * pcmout, short * AlphaMain,short * AlphaSec,enum eMainChannelIdx first_out_main, enum eMainChannelIdx first_out_sec, int scale_shift, int nbspl, int processed_spl)
#else
enum  eAccBoolean mixer_mix_saturate ( tPcmBuffer * pcmin0,tPcmBuffer * pcmin1, tPcmBuffer * pcmout, short * AlphaMain,short * AlphaSec,enum eAccMainChannelIdx first_out_main, enum eAccMainChannelIdx first_out_sec, int scale_shift, int nbspl, int processed_spl)
#endif
{
#ifdef _STANDALONE_
    enum  eErrorCode     status = ACC_EC_OK ;
    enum eMainChannelIdx ch_in, ch_out;

#else
	enum  eAccBoolean     status = ACC_MME_TRUE;
    enum eAccMainChannelIdx ch_in;

#endif
    int                  in_offset,out_offset;
    int                  nch_in, nch_out, nch,tmp1,tmp2,alpha_main,alpha_sec;

    short * spl_in0, *spl_in1, * spl_out;
    short   out;
    int       in_idx, out_idx,length;
    short   min, max,max_l,min_l;
    short   input0,input1, output;

    nch_in  = pcmin0->NbChannels;
    nch_out = pcmout->NbChannels - first_out_main;
    nch     = ( nch_in < nch_out ) ? nch_in : nch_out;

    in_offset = pcmin0->Offset;
    out_offset = pcmout->Offset;
    max = ( int ) MIXER_MAX_Q31 >> (scale_shift+16) ;
#ifndef _ST20_ASM_
    min = ( int ) MIXER_MIN_Q31 >> (scale_shift+16) ;
#else
    tmp1= MIXER_MIN_Q31;
    tmp2 = (scale_shift+16);
    __optasm{
        ldab tmp2,tmp1;
        ashr;
        st min;
    }
#endif

/* Mix input to output */
#ifdef _STANDALONE_
    for ( ch_in = MAIN_LEFT; ch_in < nch ; ch_in++ )
#else
    for ( ch_in = ACC_MAIN_LEFT; ch_in < nch ; ch_in++ )
#endif
    {
//this part only
        alpha_main=AlphaMain[ch_in];
        alpha_sec =AlphaSec[ch_in];
        max_l=(st20_f16_mulr_fract16_fract16(max,AlphaSec[ch_in+2])<<1);
        min_l=(st20_f16_mulr_fract16_fract16(min,AlphaSec[ch_in+2])<<1);
        spl_in0  = (short*)(pcmin0->SamplesPtr[ch_in]) + ( processed_spl * in_offset )  ;
        spl_in1  = (short*)(pcmin1->SamplesPtr[ch_in]) + ( processed_spl * in_offset )  ;
        spl_out = (short *)(pcmout->SamplesPtr[ch_in]) + ( processed_spl * out_offset )  ;
        in_idx = 0;
        out_idx = 0;
#ifdef _STANDALONE_
        pcmout->ChannelExist[ch_in] = ACC_TRUE;
#else
		pcmout->ChannelExist[ch_in] = ACC_MME_TRUE;
#endif
        length = nbspl/8;
        if(in_offset==1)
        {
            do
            {
//				input0 = spl_in0[in_idx];
                input0 = spl_in0[0];
//				out = __MULHHS ( input, alpha ) << 1 ;
                out =  __MULHHS ( input0, alpha_main );
//	input1 = spl_in1[in_idx];
//	in_idx +=  in_offset;
                input1 = spl_in1[0];
                output=__MULHHS ( input1, alpha_sec );
#ifndef _ST20_ASM_
                out = ( out >> scale_shift ) +  (output>>scale_shift);
                out = ( out < min_l ) ? min_l : out;
                out = ( out > max_l ) ? max_l : out;
                out = ( out << scale_shift )  ;
#else
                out=out+output;
                __optasm{
                    ldab out,min_l;
                    order;
                    rot;
                    ld max_l;
                    order;
                    ldc 1;
                    shl;
                    st out;
                }
#endif

//		spl_out[out_idx] = out;
//		out_idx += out_offset;
                spl_out[0]= out;
//input0 = spl_in0[in_idx];
                input0 = spl_in0[1];
//				out = __MULHHS ( input, alpha ) << 1 ;
                out =  __MULHHS ( input0, alpha_main );
//input1 = spl_in1[in_idx];
                input1 = spl_in1[1];
//in_idx +=  in_offset;
                output=__MULHHS ( input1, alpha_sec );
#ifndef _ST20_ASM_
                out = ( out >> scale_shift ) +  (output>>scale_shift);								out = ( out < min_l ) ? min_l : out;
                out = ( out > max_l ) ? max_l : out;
                out = ( out << scale_shift )  ;

#else
                out=out+output;
                __optasm{
                    ldab out,min_l;
                    order;
                    rot;
                    ld max_l;
                    order;
                    ldc 1;
                    shl;
                    st out;
                }

#endif
                spl_out[1]=out;
//	input0 = spl_in0[in_idx];
                input0 = spl_in0[2];
//				out = __MULHHS ( input, alpha ) << 1 ;
                out =  __MULHHS ( input0, alpha_main );
//input1 = spl_in1[in_idx];
//in_idx +=  in_offset;
                input1=spl_in1[2];
                output=__MULHHS ( input1, alpha_sec );
#ifndef _ST20_ASM_
                out = ( out >> scale_shift ) +  (output>>scale_shift);
                out = ( out < min_l ) ? min_l : out;
                out = ( out > max_l ) ? max_l : out;
                out = ( out << scale_shift )  ;
#else
                out = out+output;
                __optasm{
                    ldab out,min_l;
                    order;
                    rot;
                    ld max_l;
                    order;
                    ldc 1;
                    shl;
                    st out;
                }

#endif

//	spl_out[out_idx] = out;
//	out_idx += out_offset;
                spl_out[2]=out;
//	input0 = spl_in0[in_idx];
                input0 = spl_in0[3];
//				out = __MULHHS ( input, alpha ) << 1 ;
                out =  __MULHHS ( input0, alpha_main );
//input1 = spl_in1[in_idx];
                input1 = spl_in1[3];
//in_idx +=  in_offset;
                output=__MULHHS ( input1, alpha_sec );
#ifndef _ST20_ASM_
                out = ( out >> scale_shift ) +  (output>>scale_shift);
                out = ( out < min_l ) ? min_l : out;
                out = ( out > max_l ) ? max_l : out;
                out = ( out << scale_shift )  ;
#else
                out=out+output;
                __optasm{
                    ldab out,min_l;
                    order;
                    rot;
                    ld max_l;
                    order;
                    ldc 1;
                    shl;
                    st out;
                }
#endif

//	spl_out[out_idx] = out;
                spl_out[3] = out;

//	out_idx += out_offset;
//	input0 = spl_in0[in_idx];
                input0 = spl_in0[4];
//				out = __MULHHS ( input, alpha ) << 1 ;
                out =  __MULHHS ( input0, alpha_main );
//input1 = spl_in1[in_idx];
                input1 = spl_in1[4];
//in_idx +=  in_offset;
                output=__MULHHS ( input1, alpha_sec );
#ifndef _ST20_ASM_
                out = ( out >> scale_shift ) +  (output>>scale_shift);
                out = ( out < min_l ) ? min_l : out;
                out = ( out > max_l ) ? max_l : out;
                out = ( out << scale_shift )  ;
#else
                out = out+output;
                __optasm{
                    ldab out,min_l;
                    order;
                    rot;
                    ld max_l;
                    order;
                    ldc 1;
                    shl;
                    st out;
                }
#endif
//spl_out[out_idx] = out;
//out_idx += out_offset;
                spl_out[4] = out;
//	input0 = spl_in0[in_idx];
                input0=spl_in0[5];
//				out = __MULHHS ( input, alpha ) << 1 ;
                out =  __MULHHS ( input0, alpha_main );
//input1 = spl_in1[in_idx];
//in_idx +=  in_offset;
                input1 = spl_in1[5];
                output=__MULHHS ( input1, alpha_sec );
#ifndef _ST20_ASM_
                out = ( out >> scale_shift ) +  (output>>scale_shift);
                out = ( out < min_l ) ? min_l : out;
                out = ( out > max_l ) ? max_l : out;
                out = ( out << scale_shift )  ;
#else
                out=out+output;
                __optasm{
                    ldab out,min_l;
                    order;
                    rot;
                    ld max_l;
                    order;
                    ldc 1;
                    shl;
                    st out;
                }
#endif

//	spl_out[out_idx] = out;
                spl_out[5]=out;
//	out_idx += out_offset;
//	input0 = spl_in0[in_idx];
                input0 = spl_in0[6];
//				out = __MULHHS ( input, alpha ) << 1 ;
                out =  __MULHHS ( input0, alpha_main );
//input1 = spl_in1[in_idx];
//in_idx +=  in_offset;
                input1=spl_in1[6];
                output=__MULHHS ( input1, alpha_sec );
#ifndef _ST20_ASM_
                out = ( out >> scale_shift ) +  (output>>scale_shift);
                out = ( out < min_l ) ? min_l : out;
                out = ( out > max_l ) ? max_l : out;
                out = ( out << scale_shift )  ;
#else
                out=out+output;
                __optasm{
                    ldab out,min_l;
                    order;
                    rot;
                    ld max_l;
                    order;
                    ldc 1;
                    shl;
                    st out;
                }
#endif

//spl_out[out_idx] = out;
                spl_out[6]=out;
//out_idx += out_offset;
//	input0 = spl_in0[in_idx];
                input0=spl_in0[7];
//				out = __MULHHS ( input, alpha ) << 1 ;
                out =  __MULHHS ( input0, alpha_main );
//input1 = spl_in1[in_idx];
                input1 = spl_in1[7];
                in_idx +=  in_offset;
                output=__MULHHS ( input1, alpha_sec );
#ifndef _ST20_ASM_
                out = ( out >> scale_shift ) +  (output>>scale_shift);
                out = ( out < min_l ) ? min_l : out;
                out = ( out > max_l ) ? max_l : out;
                out = ( out << scale_shift )  ;
#else
                out=out+output;
                __optasm{
                    ldab out,min_l;
                    order;
                    rot;
                    ld max_l;
                    order;
                    ldc 1;
                    shl;
                    st out;
                }
#endif

//spl_out[out_idx] = out;
//out_idx += out_offset;
                spl_out[7]=out;
                spl_out=&spl_out[8];
                spl_in0 = &spl_in0[8];
                spl_in1 = &spl_in1[8];
            }while(--length); /* End for all samples */
        }
        else
        {
//			for ( spl = 0; spl < nbspl; spl++ )
            do
            {
//				input0 = spl_in0[in_idx];
                input0 = spl_in0[0];
//				out = __MULHHS ( input, alpha ) << 1 ;
                out =  __MULHHS ( input0, alpha_main );
//	input1 = spl_in1[in_idx];
//	in_idx +=  in_offset;
                input1 = spl_in1[0];
                output=__MULHHS ( input1, alpha_sec );
#ifndef _ST20_ASM_
                out = ( out >> scale_shift ) +  (output>>scale_shift);
#else
                __optasm{
                    ldab out,output;
                    add;
                    ldc 1;
//				ldab scale_shift,out;
                    ashr;
                    st out;
//				ldab scale_shift,output;
//				ashr;
//				st tmp2;
                }
//				out=tmp1+tmp2;
#endif
                out = ( out < min_l ) ? min_l : out;
                out = ( out > max_l ) ? max_l : out;
                out = ( out << scale_shift )  ;
//		spl_out[out_idx] = out;
//		out_idx += out_offset;
                spl_out[0]= out;
//input0 = spl_in0[in_idx];
                input0 = spl_in0[2];
//				out = __MULHHS ( input, alpha ) << 1 ;
                out =  __MULHHS ( input0, alpha_main );
//input1 = spl_in1[in_idx];
                input1 = spl_in1[2];
//in_idx +=  in_offset;
                output=__MULHHS ( input1, alpha_sec );
#ifndef _ST20_ASM_
                out = ( out >> scale_shift ) +  (output>>scale_shift);
#else
                __optasm{
                    ldab out,output;
                    add;
                    ldc 1;
//				ldab scale_shift,out;
                    ashr;
                    st out;
//				ldab scale_shift,output;
//				ashr;
//				st tmp2;
                }
//				out=tmp1+tmp2;
#endif
                out = ( out < min_l ) ? min_l : out;
                out = ( out > max_l ) ? max_l : out;
                out = ( out << scale_shift )  ;
//spl_out[out_idx] = out;
//out_idx += out_offset;
                spl_out[2]=out;
//	input0 = spl_in0[in_idx];
                input0 = spl_in0[4];
//				out = __MULHHS ( input, alpha ) << 1 ;
                out =  __MULHHS ( input0, alpha_main );
//input1 = spl_in1[in_idx];
//in_idx +=  in_offset;
                input1=spl_in1[4];
                output=__MULHHS ( input1, alpha_sec );
#ifndef _ST20_ASM_
                out = ( out >> scale_shift ) +  (output>>scale_shift);
#else
                __optasm{
                    ldab out,output;
                    add;
                    ldc 1;
//				ldab scale_shift,out;
                    ashr;
                    st out;
//				ldab scale_shift,output;
//				ashr;
//				st tmp2;
                }
//				out=tmp1+tmp2;
#endif
                out = ( out < min_l ) ? min_l : out;
                out = ( out > max_l ) ? max_l : out;
                out = ( out << scale_shift )  ;
//	spl_out[out_idx] = out;
//	out_idx += out_offset;
                spl_out[4]=out;
//	input0 = spl_in0[in_idx];
                input0 = spl_in0[6];
//				out = __MULHHS ( input, alpha ) << 1 ;
                out =  __MULHHS ( input0, alpha_main );
//input1 = spl_in1[in_idx];
                input1 = spl_in1[6];
//in_idx +=  in_offset;
                output=__MULHHS ( input1, alpha_sec );
#ifndef _ST20_ASM_
                out = ( out >> scale_shift ) +  (output>>scale_shift);
#else
                __optasm{
                    ldab out,output;
                    add;
                    ldc 1;
//				ldab scale_shift,out;
                    ashr;
                    st out;
//				ldab scale_shift,output;
//				ashr;
//				st tmp2;
                }
//				out=tmp1+tmp2;
#endif
                out = ( out < min_l ) ? min_l : out;
                out = ( out > max_l ) ? max_l : out;
                out = ( out << scale_shift )  ;
//	spl_out[out_idx] = out;
                spl_out[6] = out;

//	out_idx += out_offset;
//	input0 = spl_in0[in_idx];
                input0 = spl_in0[8];
//				out = __MULHHS ( input, alpha ) << 1 ;
                out =  __MULHHS ( input0, alpha_main );
//input1 = spl_in1[in_idx];
                input1 = spl_in1[8];
//in_idx +=  in_offset;
                output=__MULHHS ( input1, alpha_sec );
#ifndef _ST20_ASM_
                out = ( out >> scale_shift ) +  (output>>scale_shift);
#else
                __optasm{
                    ldab out,output;
                    add;
                    ldc 1;
//				ldab scale_shift,out;
                    ashr;
                    st out;
//				ldab scale_shift,output;
//				ashr;
//				st tmp2;
                }
//				out=tmp1+tmp2;
#endif
                out = ( out < min_l ) ? min_l : out;
                out = ( out > max_l ) ? max_l : out;
                out = ( out << scale_shift )  ;
//spl_out[out_idx] = out;
//out_idx += out_offset;
                spl_out[8] = out;
//	input0 = spl_in0[in_idx];
                input0=spl_in0[10];
//				out = __MULHHS ( input, alpha ) << 1 ;
                out =  __MULHHS ( input0, alpha_main );
//input1 = spl_in1[in_idx];
//in_idx +=  in_offset;
                input1 = spl_in1[10];
                output=__MULHHS ( input1, alpha_sec );
#ifndef _ST20_ASM_
                out = ( out >> scale_shift ) +  (output>>scale_shift);
#else
                __optasm{
                    ldab out,output;
                    add;
                    ldc 1;
//				ldab scale_shift,out;
                    ashr;
                    st out;
//				ldab scale_shift,output;
//				ashr;
//				st tmp2;
                }
//				out=tmp1+tmp2;
#endif
                out = ( out < min_l ) ? min_l : out;
                out = ( out > max_l ) ? max_l : out;
                out = ( out << scale_shift )  ;
//	spl_out[out_idx] = out;
                spl_out[10]=out;
//	out_idx += out_offset;
//	input0 = spl_in0[in_idx];
                input0 = spl_in0[12];
//				out = __MULHHS ( input, alpha ) << 1 ;
                out =  __MULHHS ( input0, alpha_main );
//input1 = spl_in1[in_idx];
//in_idx +=  in_offset;
                input1=spl_in1[12];
                output=__MULHHS ( input1, alpha_sec );
#ifndef _ST20_ASM_
                out = ( out >> scale_shift ) +  (output>>scale_shift);
#else
                __optasm{
                    ldab out,output;
                    add;
                    ldc 1;
//				ldab scale_shift,out;
                    ashr;
                    st out;
//				ldab scale_shift,output;
//				ashr;
//				st tmp2;
                }
//				out=tmp1+tmp2;
#endif
                out = ( out < min_l ) ? min_l : out;
                out = ( out > max_l ) ? max_l : out;
                out = ( out << scale_shift )  ;
//spl_out[out_idx] = out;
                spl_out[12]=out;
//out_idx += out_offset;
//	input0 = spl_in0[in_idx];
                input0=spl_in0[14];
//				out = __MULHHS ( input, alpha ) << 1 ;
                out =  __MULHHS ( input0, alpha_main );
//input1 = spl_in1[in_idx];
                input1 = spl_in1[14];
                in_idx +=  in_offset;
                output=__MULHHS ( input1, alpha_sec );
#ifndef _ST20_ASM_
                out = ( out >> scale_shift ) +  (output>>scale_shift);
#else
                __optasm{
                    ldab out,output;
                    add;
                    ldc 1;
//				ldab scale_shift,out;
                    ashr;
                    st out;
//				ldab scale_shift,output;
//				ashr;
//				st tmp2;
                }
//				out=tmp1+tmp2;
#endif
                out = ( out < min_l ) ? min_l : out;
                out = ( out > max_l ) ? max_l : out;
                out = ( out << scale_shift )  ;
//spl_out[out_idx] = out;
//out_idx += out_offset;
                spl_out[14]=out;
                spl_out=&spl_out[16];
                spl_in0 = &spl_in0[16];
                spl_in1 = &spl_in1[16];
            }while(--length); /* End for all samples */
        }
    }
    return ( status );
}
#endif
