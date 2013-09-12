/*$Id: st220_ecolib.c,v 1.8 2004/03/19 17:28:43 lassureg Exp $*/
/*####################################################################*/
/* Copyright  STMicroelectonics, ACC Singapore                        */
/* This program is property of Audio Competence Centre Singapore      */
/* from STMicroelectronics.                                           */
/* It should not be communicated outside STMicroelectronics           */
/* without authorization.                                             */
/*                                                                    */
/* Author:        Charles Averty                                      */
/* Organization:  ACC Singapore                                       */
/* Contact:       Charles.Averty@st.com                               */
/* Created:       July 2002                                           */
/*                                                                    */
/* This file contains equivalent to the new instructions for the core */
/* of ST220                                                           */
/*                                                                    */
/* Modified:   2002                                                   */
/* By:      Charles Averty (Charles.Averty@st.com)                    */
/* Purpose: Simplifications                                           */
/*          Bugs solved                                               */
/*          Code also cleaned: init and comput separated              */
/*####################################################################*/
/* Modified:   July 2002                                              */
/* By:      Charles Averty (Charles.Averty@st.com)                    */
/* Purpose: Integrate with LX API for SRC                             */
/*####################################################################*/

#include "mixer_defines.h"
#include "st20_utils.h"

#define _ST20_
#ifdef _ST20_


#if 0
int st20_checknshift(int data, int value)
{
	int maxdata,mindata;
	maxdata = MAXPOSI>>value;
#ifndef _ST20_ASM_
	mindata = (int)MAXNEGI>>value;
#else
__optasm{
	ldc MAXNEGI;
	ldl value;
	ashr;
	stl mindata;
}
#endif

	if(data>maxdata)
		data = MAXPOSI;
	else if(data<mindata)
		data = MAXNEGI;
	else
		data = data<<value;
	return data;
}

short st20_fract16_mulr_fract16_fract16(short r0, short r1)
{
#ifndef _ST20_ASM_
    int t0 = EXT16(r0, 32);
    int t1 = EXT16(r1, 32);
    int t2,t3;

    if (((-r0) == 0x8000) && ((-r1) == 0x8000))
    {
        t3 = 0x7FFF;
    }
    else
    {
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
   ldc 15;
   ashr;
   stl t3;
   }
 #endif
    return t3;

}
int st20_int32_mul_int16_int16(short r0, short r1)
{
    #ifndef _ST20_ASM_
 	int t0 = EXT16(r0, 16);
    int t1 = EXT16(r1, 16);
    int t2 = t0*t1;
    return t2 ;
    #else
    int t;
    __optasm{
    ldab r0,r1;
    smul;
    stl t;
    }
     return t;
    #endif
}

int st20_int32_abs_int32(int r0)
{
	int t1;
/* int t0 = -r0;*/
  t1 = r0 < 0 ? -r0 : r0;
  /*  long_long t2 = EXT(t1, 32);*/
   /* __ssv |= t1 != t2;*/
    return t1;
}
i64 st20_c64(int hi, int lo)
{
	i64 c;
	__hi(c) = hi;
	__lo(c) = lo;
	return c;
}
int st20_ltowr(i64 x)
{
	return (__lo(x)+1);
}

int st20_ltow(i64 x)
{
	return (__lo(x));
}

/* Constant constructor for global variables and local statics */


 i64  st20_addl(i64 a, i64 b)
{
	i64 c;
	int carry;
	ST20_ADDCG(__lo(c),carry,__lo(a),__lo(b),0);
	ST20_ADDCG(__hi(c),carry,__hi(a),__hi(b),carry);
	return c;
}
 i64 st20_shll(i64 a, unsigned int count)
{
	unsigned t_lo = __lo(a);
	unsigned t_hi = __hi(a);
	unsigned r_hi = t_hi;
	unsigned r_lo = t_lo;

	if (count)
	{
		if (count < 32)
		{
			r_lo = t_lo << count;
			r_hi = (t_hi << count) | (t_lo >> (32 - count));
		}
		else
		{
			r_lo = 0;
			r_hi = t_lo << (count & 31);
		}
	}
	return st20_c64(r_hi,r_lo);
}
 i64 st20_shrl(i64 a, unsigned int count)
{
	unsigned t_lo = __lo(a);
	unsigned t_hi = __hi(a);
	unsigned r_hi = t_hi;
	unsigned r_lo = t_lo;
#ifdef _ST20_ASM_
	unsigned r_hi1;
#endif

	if (count)
	{
		if (count < 32)
		{
#ifndef _ST20_ASM_
			r_hi = (signed) t_hi >> count;
#else
			__optasm{
			ldab count,t_hi;
			ashr;
			stl r_hi;
			}
#endif
				r_lo = (t_hi << (32 - count)) | (t_lo >> count);
		}
		else
		{
#ifndef _ST20_ASM_
			r_lo = (signed) t_hi >> (count & 31);
			r_hi = (signed) t_hi >> 31;
#else
			r_hi1 = (count & 31);
			__optasm{
			ldab r_hi1,t_hi;
			ashr;
			stl r_lo;
			ldl t_hi;
			ldc 31;
			ashr;
			stl r_hi;
			}
#endif
		}
	}
	return st20_c64(r_hi,r_lo);
}
 i64 st20_muln(unsigned int a, unsigned int b)
{
	/* Code from Fred Homewood - ST */
	i64 c;
	int i,w,x;
#ifndef _ST20_ASM_
	int carry;
	int y,z,d,e,f,g,h,j;
	w=(long)((int)a>>16);
	x=(long)((int)b>>16);
	y=(long)((unsigned short)a);
	z=(long)((unsigned short)b);
	d= w*x;
	e= y*z;
	f= w*z;
	g= x*y;
    ST20_ADDCG(h,carry,e,(int)(f<<16),0);
	ST20_ADDCG(__lo(c),carry,h,(int)(g<<16),carry);

    ST20_ADDCG(i,carry,d,(int)(f>>16),carry);
	ST20_ADDCG(__hi(c),carry,i,(int)(g>>16),carry);
 #else
        i=0;
 __optasm{
 	ldabc a,b,i;
 	mac;
 	stab w,x;
 	}
    c.__s20.lo = w;
    c.__s20.hi = x;
  #endif
	return c;
}

#endif

/*(Al*Bh + Ah*Bh)32 higher part*/
/*
long __MULHHS(long AhAl, long BhBl)
{
	__n64 c;
	c = (__n64)(BhBl >> 16) * (__n64)AhAl;
	c = c >> 16;
	return (long)c;
}
*/
/*
int32 __int32_mulh_int32_int32(int32 r0, int32 r1)
{
	long_long t0 = EXT(r0, 32);
	long_long t1 = EXT(r1, 32);
	long_long t2 = t0 * t1 >> 32;
	long_long t3 = EXT(t2, 32);
	return t3;
}
*/
#ifndef _ST20_C_
int st20_Q30_mulQ31xQ31(int a,int b)
{
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
	return (int)d11;
}
#endif
#if 0
 INLINE int st220_Q30_mulQ31xQ31(int a, int b)
{

#if 0

	unsigned int tl0, tl1;
	int th0, th1;
	int carry;
#ifndef __LXMFC__

	unsigned int lo;
	int hi;
#endif /* __LXMFC__ */

	__n64 r;

	tl0 = __MULLU(a,b);
	tl1 = __MULHS(a,b);
	th0 = __MULLHUS(a,b);
	th1 = __MULHHS(a,b);
#ifdef __LXMFC__

	__ADDCG(__lo(r),carry,tl0,tl1,0);
	__ADDCG(__hi(r),carry,th0,th1,carry);
#else

	__ADDCG(lo, carry, tl0, tl1, 0);
	__ADDCG(hi,carry,th0,th1,carry);
	r = hi;
	r = r << 32;
	r = r | (unsigned int) lo;
#endif /* __LXMFC__ */

	return __hi(r);

#endif

    return __int32_mulh_int32_int32(a,b);            // Bitexact with ST220 and ST230 above                   >> 1 Bundles with mul64h

}
int  st20_Q31_mulQ31xQ31(int r0, int r1)
{
#ifndef _ST20_ASM_
	long d1,d2,d3,d4,d5,d6,d7,d8,d9,d10,d11;
	d1 = (long)(r0>>16);
    d2 = (long)(r1>>16);
    {
		d3 = d1*d2;
		d4 = (long)((unsigned short)r0);
		d5 = (long)((unsigned short)r1);
		d6 = d2*d4;
		d7 = d1*d5;
		d8 = d6>>15;
		d9 = d7>>15;
		d10 = d3<<1;
		d11 = d8+d9+d10;

    }
#else
int d11;
	d11=0;
__optasm{
 	ldabc r0,r1,d11;
 	mac;
 	rev;
 	ldc 1;
 	shl;
 	stl d11;
 	}
#endif
	return (int)d11;
}
#endif

#ifdef _VOL_

#define MIXER_0dB_Q31 0x7FFFFFFF
#define MIXER_M0p5dB  0x78D6FC9E
#define MIXER_ATTENUATION_MUTE 70
#define MIXER_MAX_ATTENUATION (6 * 24)

int mixer_convert_volume(int dB)
{
	/* Convert Volume Command in dB into linear command
	 *
	 *     linear(-0.5db)^ dB
	 */

	int linear = MIXER_0dB_Q31;
	int refq31 = MIXER_M0p5dB;        /* Q31 */

	// mute if attenuation < LimitQ15
	if (dB > MIXER_ATTENUATION_MUTE) return (0);

	dB = dB << 1;

	if (dB != 0)
	{
		// limit the attenuation
		dB = ((dB > MIXER_MAX_ATTENUATION) | ( dB < 0 )) ? MIXER_MAX_ATTENUATION : dB;

		// Compute the corresponding linear attenuation
		do
		{
			if (dB & 1)
			{
				linear = st20_Q31_mulQ31xQ31(linear, refq31); /* Q31 */
			}
			dB >>= 1;
			refq31 = st20_Q31_mulQ31xQ31(refq31, refq31); /* Q31 */
		}
		while (dB != 0);
	}

	return(linear);
}
#endif

#endif /*  UNIX */
