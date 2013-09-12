/*$Id: st220_ecolib.h,v 1.7 2004/01/23 11:01:55 lassureg Exp $*/
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
/* This files contains the sampling rate conversion routines required */
/* for the DVD-RAM application                                        */
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

/* GL - MARCH 2003 -  update             */
/* Prototypes from OPEN64 Compiler Group */
/*
   int32 __st220mulhhs(int32,int32) ;
   int32 __st220mulhs(int32,int32) ;
   int32 __st220mullhus(int32,int32) ;
   uint32 __st220mullu(uint32,uint32) ;

   MULTIFLOW intrinsics == >
   #define __MULHHS(a,b)   __st220mulhhs((a),(b))
   #define __MULLHUS(a,b)  __st220mullhus((a),(b))
   #define __MULHS(a,b)    __st220mulhs((a),(b))
   #define __MULLU(a,b)    __st220mullu((a),(b))

*/

#ifndef _ST20_UTILS_H
#define _ST20_UTILS_H


typedef union {
	struct
	{
#ifndef _little_endian
		unsigned int hi;
		unsigned int lo;
#else

		unsigned int lo;
		unsigned int hi;
#endif

	}
	__s20;
	double __d; /* force 0 mod 8 alignment */
} i64;
#define EXT16(x, n) (((int)(x)<<(32-(n)))>>(32-(n)))
#define MAXPOSI		2147483647
#define MAXNEGI		-2147483648
#define SCALE 32768
/* Private utilities */
#define __hi(x) ((x).__s20.hi)
#define __lo(x) ((x).__s20.lo)
#define ST20_ADDCG(s,cout,a,b,cin) { \
    unsigned __addcg_c = ((cin) & 1); \
    unsigned __addcg_t = (a) + (b) + __addcg_c; \
    (cout) = ((cin)&1) ? ((unsigned)__addcg_t <= (a)) : ((unsigned)__addcg_t < (a)); \
    (s) = __addcg_t; \
}
extern int st20_checknshift(int data, int value);
extern short st20_fract16_mulr_fract16_fract16(short r0, short r1);
extern int st20_int32_mul_int16_int16(short r0, short r1);
extern i64 st20_c64(int hi, int lo);
extern int st20_ltow(i64 x);
extern i64 st20_addl(i64 a, i64 b);
extern i64 st20_shll(i64 a, unsigned int count);
extern i64 st20_shrl(i64 a, unsigned int count);
extern i64 st20_muln(unsigned int a, unsigned int b);
extern unsigned int __CLZ(unsigned int input);
extern int st20_int32_abs_int32(int r0);
int  st20_Q31_mulQ31xQ31(int r0, int r1);
#endif /* _ST20_UTILS_H */
