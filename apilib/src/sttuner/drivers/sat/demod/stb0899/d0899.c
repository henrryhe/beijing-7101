/* ----------------------------------------------------------------------------
File Name: d_0899.c

Description: 

    STB0899 demod driver.


Copyright (C) 2005-2006 STMicroelectronics

History:
  date: 18-Jan-2005
  version: 
  author: SD 
comment: Add support for SatCR.

date: 20-Mar-2006
  version: 
  author: SD 
comment: Linux integration and V3.6 integration.

Reference:
---------------------------------------------------------------------------- */

/* Includes ---------------------------------------------------------------- */
#ifndef ST_OSLINUX
   #include  <stdlib.h>
   #include <string.h>
#endif
#include "stlite.h"     /* Standard includes */
#include "sttbx.h"

#include "stevt.h"
#include "sttuner.h"                    

/* local to sttuner */
#include "util.h"       /* generic utility functions for sttuner */
#include "dbtypes.h"    /* data types for databases */
#include "sysdbase.h"   /* functions to accesss system data */

#include "ioarch.h"     /* I/O for this driver */
#include "ioreg.h"      /* I/O for this driver */

/* LLA */
#include "drv0899.h"   /* driver */
#include "d0899_util.h"  /* utility functions to support stb0899 LLA */
#include "d0899_dvbs2util.h"
#include "d0899_init.h" 
#include "d0899.h"      /* header for this file */

#include "sioctl.h"     /* data structure typedefs for all the the sat ioctl functions */

extern S32 FE_STB0899_GetRFLevel(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, STTUNER_FECType_t FECType, STTUNER_FECMode_t FECMode);
#ifdef STTUNER_DRV_SAT_SCR
extern ST_ErrorCode_t scr_scrdrv_SetFrequency   (STTUNER_Handle_t Handle, DEMOD_Handle_t DemodHandle, ST_DeviceName_t *DeviceName, U32  InitialFrequency,  U8 LNBIndex, U8 SCRBPF );
#endif

   int  ModCodLUT[29][3] = {
        {FE_DUMMY_PLF, STTUNER_MOD_NONE,    STTUNER_FEC_NONE},
		{FE_QPSK_14,   STTUNER_MOD_QPSK,    STTUNER_FEC_1_4},
		{FE_QPSK_13,   STTUNER_MOD_QPSK,    STTUNER_FEC_1_3},
		{FE_QPSK_25,    STTUNER_MOD_QPSK,   STTUNER_FEC_2_5},
		{FE_QPSK_12,    STTUNER_MOD_QPSK,   STTUNER_FEC_1_2},
		{FE_QPSK_35,    STTUNER_MOD_QPSK,   STTUNER_FEC_3_5},
		{FE_QPSK_23,    STTUNER_MOD_QPSK,   STTUNER_FEC_2_3},
		{FE_QPSK_34,    STTUNER_MOD_QPSK,   STTUNER_FEC_3_4},
		{FE_QPSK_45,    STTUNER_MOD_QPSK,   STTUNER_FEC_4_5},
		{FE_QPSK_56,    STTUNER_MOD_QPSK,   STTUNER_FEC_5_6},
		{FE_QPSK_89,    STTUNER_MOD_QPSK,   STTUNER_FEC_8_9},
		{FE_QPSK_910,   STTUNER_MOD_QPSK,   STTUNER_FEC_9_10},
		{FE_8PSK_35,    STTUNER_MOD_8PSK,   STTUNER_FEC_3_5},
		{FE_8PSK_23,    STTUNER_MOD_8PSK,   STTUNER_FEC_2_3},
		{FE_8PSK_34,    STTUNER_MOD_8PSK,   STTUNER_FEC_3_4},
		{FE_8PSK_56,    STTUNER_MOD_8PSK,   STTUNER_FEC_5_6},
		{FE_8PSK_89,    STTUNER_MOD_8PSK,   STTUNER_FEC_8_9},
		{FE_8PSK_910,   STTUNER_MOD_8PSK,   STTUNER_FEC_9_10},
		{FE_16APSK_23,  STTUNER_MOD_16APSK, STTUNER_FEC_2_3},
		{FE_16APSK_34,  STTUNER_MOD_16APSK, STTUNER_FEC_3_4},
		{FE_16APSK_45,  STTUNER_MOD_16APSK, STTUNER_FEC_4_5},
		{FE_16APSK_56,  STTUNER_MOD_16APSK, STTUNER_FEC_5_6},
		{FE_16APSK_89,  STTUNER_MOD_16APSK, STTUNER_FEC_8_9},
		{FE_16APSK_910, STTUNER_MOD_16APSK, STTUNER_FEC_9_10},
		{FE_32APSK_34,  STTUNER_MOD_32APSK, STTUNER_FEC_3_4},
		{FE_32APSK_45,  STTUNER_MOD_32APSK, STTUNER_FEC_4_5},
		{FE_32APSK_56,  STTUNER_MOD_32APSK, STTUNER_FEC_5_6},
		{FE_32APSK_89,  STTUNER_MOD_32APSK, STTUNER_FEC_8_9},
		{FE_32APSK_910, STTUNER_MOD_32APSK, STTUNER_FEC_9_10}
	};
	
	
U32 AddressArray[STB0899_NBREGS]=
{
 0x0000f000 ,	/* ID */
 0x0000f0a0 ,	/* TDISCNTRL1 */
 0x0000f0a1 ,	/* DISCNTRL2 */
 0x0000f0a4 ,	/* DISRX_ST0 */
 0x0000f0a5 ,	/* DISRX_ST1 */
 0x0000f0a6 ,	/* DISPARITY */
 0x0000f0a7 ,	/* DISFIFO */
 0x0000f0a8 ,	/* DISSTATUS */
 0x0000f0a9 ,	/* DISF22 */
 0x0000f0aa ,	/* DISF22RX */
 0x0000f101 ,	/* SYSREG */
 0x0000f110 ,	/* ACRPRESC */
 0x0000f111 ,	/* ACRDIV1 */
 0x0000f112 ,	/* ACRDIV2 */
 0x0000f113 ,	/* DACR1 */
 0x0000f114 ,	/* DACR2 */
 0x0000f11c ,	/* OUTCFG */
 0x0000f11d ,	/* MODECFG */
 0x0000f120 ,	/* IRQSTATUS3 */
 0x0000f121 ,	/* IRQSTATUS2 */
 0x0000f122 ,	/* IRQSTATUS1 */
 0x0000f123 ,	/* IRQSTATUS0 */
 0x0000f124 ,	/* IRQMSK3 */
 0x0000f125 ,	/* IRQMSK2 */
 0x0000f126 ,	/* IRQMSK1 */
 0x0000f127 ,	/* IRQMSK0 */
 0x0000f128 ,	/* IRQCFG */
 0x0000f129 ,	/* I2CCFG */
 0x0000f12a ,	/* I2CRPT */
 0x0000f139 ,	/* IOPVALUE5 */
 0x0000f13a ,	/* IOPVALUE4 */
 0x0000f13b ,	/* IOPVALUE3 */
 0x0000f13c ,	/* IOPVALUE2 */
 0x0000f13d ,	/* IOPVALUE1 */
 0x0000f13e ,	/* IOPVALUE0 */
 0x0000f140 ,	/* GPIO0CFG */
 0x0000f141 ,	/* GPIO1CFG */
 0x0000f142 ,	/* GPIO2CFG */
 0x0000f143 ,	/* GPIO3CFG */
 0x0000f144 ,	/* GPIO4CFG */
 0x0000f145 ,	/* GPIO5CFG */
 0x0000f146 ,	/* GPIO6CFG */
 0x0000f147 ,	/* GPIO7CFG */
 0x0000f148 ,	/* GPIO8CFG */
 0x0000f149 ,	/* GPIO9CFG */
 0x0000f14a ,	/* GPIO10CFG */
 0x0000f14b ,	/* GPIO11CFG */
 0x0000f14c ,	/* GPIO12CFG */
 0x0000f14d ,	/* GPIO13CFG */
 0x0000f14e ,	/* GPIO14CFG */
 0x0000f14f ,	/* GPIO15CFG */
 0x0000f150 ,	/* GPIO16CFG */
 0x0000f151 ,	/* GPIO17CFG */
 0x0000f152 ,	/* GPIO18CFG */
 0x0000f153 ,	/* GPIO19CFG */
 0x0000f154 ,	/* GPIO20CFG */
 0x0000f155 ,	/* SDATCFG */
 0x0000f156 ,	/* SCLTCFG */
 0x0000f157 ,	/* AGCRFCFG */
 0x0000f158 ,	/* AGCBB2CFG */
 0x0000f159 ,	/* AGCBB1CFG */
 0x0000f15a ,	/* DIRCLKCFG */
 0x0000f15b ,	/* CKOUT27CFG */
 0x0000f15c ,	/* STDBYCFG */
 0x0000f15d ,	/* CS0CFG */
 0x0000f15e ,	/* CS1CFG */
 0x0000f15f ,	/* DISEQCOCFG */
 0x0000f160 ,	/* GPIO32CFG */
 0x0000f161 ,	/* GPIO33CFG */
 0x0000f162 ,	/* GPIO34CFG */
 0x0000f163 ,	/* GPIO35CFG */
 0x0000f164 ,	/* GPIO36CFG */
 0x0000f165 ,	/* GPIO37CFG */
 0x0000f166 ,	/* GPIO38CFG */
 0x0000f167 ,	/* GPIO39CFG */
 0x0000f1b3 ,	/* NCOARSE */
 0x0000f1b6 ,	/* SYNTCTRL */
 0x0000f1b7 ,	/* FILTCTRL */
 0x0000f1b8 ,	/* SYSCTRL */
 0x0000f1c2 ,	/* STOPCLK1 */
 0x0000f1c3 ,	/* STOPCLK2 */
 0x0000f1e0 ,	/* TSTTNR1 */
 0x0000f1e1 ,	/* TSTTNR2 */
 0x0000f1e2 ,	/* TSTTNR3 */
 0x0000f200 ,	/* INTBUFSTATUS */
 0x0000f201 ,	/* INTBUFCTRL */
 0x8000f300 ,	/* DMDSTATUS */
 0x8000f304 ,	/* CRLFREQ */
 0x8000f308 ,	/* BTRFREQ */
 0x8000f30c ,	/* IFAGCGAIN */
 0x8000f310 ,	/* BBAGCGAIN */
 0x8000f314 ,	/* DCOFFSET */
 0x8000f31c ,	/* DMDCNTRL */
 0x8000f320 ,	/* IFAGCCNTRL */
 0x8000f324 ,	/* BBAGCCNTRL */
 0x8000f328 ,	/* CRLCNTRL */
 0x8000f32c ,	/* CRLPHSINIT */
 0x8000f330 ,	/* CRLFREQINIT */
 0x8000f334 ,	/* CRLLOOPGAIN */
 0x8000f338 ,	/* CRLNOMFREQ */
 0x8000f33c ,	/* CRLSWPRATE */
 0x8000f340 ,	/* CRLMAXSWP */
 0x8000f344 ,	/* CRLLKCNTRL */
 0x8000f348 ,	/* DECIMCNTRL */
 0x8000f34c ,	/* BTRCNTRL */
 0x8000f350 ,	/* BTRLOOPGAIN */
 0x8000f354 ,	/* BTRPHSINIT */
 0x8000f358 ,	/* BTRFREQINIT */
 0x8000f35c ,	/* BTRNOMFREQ */
 0x8000f360 ,	/* BTRLKCNTRL */
 0x8000f364 ,	/* DECNCNTRL */
 0x8000f368 ,	/* TPCNTRL */
 0x8000f36c ,	/* TPBUFSTATUS */
 0x8000f37c ,	/* DCESTIM */
 0x8020f310 ,	/* FLLCNTRL */
 0x8020f314 ,	/* FLLFREQWD */
 0x8020f358 ,	/* ANTIALIASSEL */
 0x8020f35c ,	/* RRCALPHA */
 0x8020f360 ,	/* DCADAPTISHFT */
 0x8020f364 ,	/* IMBOFFSET */
 0x8020f368 ,	/* IMBESTIMATE */
 0x8020f36c ,	/* IMBCNTRL */
 0x8020f374 ,	/* IFAGCCNTRL2 */
 0x8020f378 ,	/* DMDCNTRL2 */
 0x8040f300 ,	/* TPBUFFER */
 0x8040f304 ,	/* TPBUFFER1 */
 0x8040f308 ,	/* TPBUFFER2 */
 0x8040f30c ,	/* TPBUFFER3 */
 0x8040f310 ,	/* TPBUFFER4 */
 0x8040f314 ,	/* TPBUFFER5 */
 0x8040f318 ,	/* TPBUFFER6 */
 0x8040f31c ,	/* TPBUFFER7 */
 0x8040f320 ,	/* TPBUFFER8 */
 0x8040f324 ,	/* TPBUFFER9 */
 0x8040f328 ,	/* TPBUFFER10 */
 0x8040f32c ,	/* TPBUFFER11 */
 0x8040f330 ,	/* TPBUFFER12 */
 0x8040f334 ,	/* TPBUFFER13 */
 0x8040f338 ,	/* TPBUFFER14 */
 0x8040f33c ,	/* TPBUFFER15 */
 0x8040f340 ,	/* TPBUFFER16 */
 0x8040f344 ,	/* TPBUFFER17 */
 0x8040f348 ,	/* TPBUFFER18 */
 0x8040f34c ,	/* TPBUFFER19 */
 0x8040f350 ,	/* TPBUFFER20 */
 0x8040f354 ,	/* TPBUFFER21 */
 0x8040f358 ,	/* TPBUFFER22 */
 0x8040f35c ,	/* TPBUFFER23 */
 0x8040f360 ,	/* TPBUFFER24 */
 0x8040f364 ,	/* TPBUFFER25 */
 0x8040f368 ,	/* TPBUFFER26 */
 0x8040f36c ,	/* TPBUFFER27 */
 0x8040f370 ,	/* TPBUFFER28 */
 0x8040f374 ,	/* TPBUFFER29 */
 0x8040f378 ,	/* TPBUFFER30 */
 0x8040f37c ,	/* TPBUFFER31 */
 0x8060f300 ,	/* TPBUFFER32 */
 0x8060f304 ,	/* TPBUFFER33 */
 0x8060f308 ,	/* TPBUFFER34 */
 0x8060f30c ,	/* TPBUFFER35 */
 0x8060f310 ,	/* TPBUFFER36 */
 0x8060f314 ,	/* TPBUFFER37 */
 0x8060f318 ,	/* TPBUFFER38 */
 0x8060f31c ,	/* TPBUFFER39 */
 0x8060f320 ,	/* TPBUFFER40 */
 0x8060f324 ,	/* TPBUFFER41 */
 0x8060f328 ,	/* TPBUFFER42 */
 0x8060f32c ,	/* TPBUFFER43 */
 0x8060f330 ,	/* TPBUFFER44 */
 0x8060f334 ,	/* TPBUFFER45 */
 0x8060f338 ,	/* TPBUFFER46 */
 0x8060f33c ,	/* TPBUFFER47 */
 0x8060f340 ,	/* TPBUFFER48 */
 0x8060f344 ,	/* TPBUFFER49 */
 0x8060f348 ,	/* TPBUFFER50 */
 0x8060f34c ,	/* TPBUFFER51 */
 0x8060f350 ,	/* TPBUFFER52 */
 0x8060f354 ,	/* TPBUFFER53 */
 0x8060f358 ,	/* TPBUFFER54 */
 0x8060f35c ,	/* TPBUFFER55 */
 0x8060f360 ,	/* TPBUFFER56 */
 0x8060f364 ,	/* TPBUFFER57 */
 0x8060f368 ,	/* TPBUFFER58 */
 0x8060f36c ,	/* TPBUFFER59 */
 0x8060f370 ,	/* TPBUFFER60 */
 0x8060f374 ,	/* TPBUFFER61 */
 0x8060f378 ,	/* TPBUFFER62 */
 0x8060f37c ,	/* TPBUFFER63 */
 0x8400f300 ,	/* RESETCNTRL */
 0x8400f304 ,	/* ACMENABLE */
 0x8400f30c ,	/* DESCRCNTRL */
 0x8400f310 ,	/* CSMCNTRL1 */
 0x8400f314 ,	/* CSMCNTRL2 */
 0x8400f318 ,	/* CSMCNTRL3 */
 0x8400f31c ,	/* CSMCNTRL4 */
 0x8400f320 ,	/* UWPCNTRL1 */
 0x8400f324 ,	/* UWPCNTRL2 */
 0x8400f328 ,	/* UWPSTAT1 */
 0x8400f32c ,	/* UWPSTAT2 */
 0x8400f340 ,	/* DMDSTAT2 */
 0x8400f344 ,	/* FREQADJSCALE */
 0x8400f34c ,	/* UWPCNTRL3 */
 0x8400f350 ,	/* SYMCLKSEL */
 0x8400f354 ,	/* SOFSRCHTO */
 0x8400f358 ,	/* ACQCNTRL1 */
 0x8400f35c ,	/* ACQCNTRL2 */
 0x8400f360 ,	/* ACQCNTRL3 */
 0x8400f364 ,	/* FESETTLE */
 0x8400f368 ,	/* ACDWELL */
 0x8400f36c ,	/* ACQUIRETRIG */
 0x8400f370 ,	/* LOCKLOST */
 0x8400f374 ,	/* ACQSTAT1 */
 0x8400f378 ,	/* ACQTIMEOUT */
 0x8400f37c ,	/* ACQTIME */
 0x8440f308 ,	/* FINALAGCCNTRL */
 0x8440f30c ,	/* FINALAGCCGAIN */
 0x8440f310 ,	/* EQUILIZERINIT */
 0x8440f314 ,	/* EQCNTL */
 0x8440f320 ,	/* EQIINITCOEFF0 */
 0x8440f324 ,	/* EQIINITCOEFF1 */
 0x8440f328 ,	/* EQIINITCOEFF2 */
 0x8440f32c ,	/* EQIINITCOEFF3 */
 0x8440f330 ,	/* EQIINITCOEFF4 */
 0x8440f334 ,	/* EQIINITCOEFF5 */
 0x8440f338 ,	/* EQIINITCOEFF6 */
 0x8440f33c ,	/* EQIINITCOEFF7 */
 0x8440f340 ,	/* EQIINITCOEFF8 */
 0x8440f344 ,	/* EQIINITCOEFF9 */
 0x8440f348 ,	/* EQIINITCOEFF10 */
 0x8440f350 ,	/* EQQINITCOEFF0 */
 0x8440f354 ,	/* EQQINITCOEFF1 */
 0x8440f358 ,	/* EQQINITCOEFF2 */
 0x8440f35c ,	/* EQQINITCOEFF3 */
 0x8440f360 ,	/* EQQINITCOEFF4 */
 0x8440f364 ,	/* EQQINITCOEFF5 */
 0x8440f368 ,	/* EQQINITCOEFF6 */
 0x8440f36c ,	/* EQQINITCOEFF7 */
 0x8440f370 ,	/* EQQINITCOEFF8 */
 0x8440f374 ,	/* EQQINITCOEFF9 */
 0x8440f378 ,	/* EQQINITCOEFF10 */
 0x8460f300 ,	/* EQICOEFFSOUT0 */
 0x8460f304 ,	/* EQICOEFFSOUT1 */
 0x8460f308 ,	/* EQICOEFFSOUT2 */
 0x8460f30c ,	/* EQICOEFFSOUT3 */
 0x8460f310 ,	/* EQICOEFFSOUT4 */
 0x8460f314 ,	/* EQICOEFFSOUT5 */
 0x8460f318 ,	/* EQICOEFFSOUT6 */
 0x8460f31c ,	/* EQICOEFFSOUT7 */
 0x8460f320 ,	/* EQICOEFFSOUT8 */
 0x8460f324 ,	/* EQICOEFFSOUT9 */
 0x8460f328 ,	/* EQICOEFFSOUT10 */
 0x8460f330 ,	/* EQQCOEFFSOUT0 */
 0x8460f334 ,	/* EQQCOEFFSOUT1 */
 0x8460f338 ,	/* EQQCOEFFSOUT2 */
 0x8460f33c ,	/* EQQCOEFFSOUT3 */
 0x8460f340 ,	/* EQQCOEFFSOUT4 */
 0x8460f344 ,	/* EQQCOEFFSOUT5 */
 0x8460f348 ,	/* EQQCOEFFSOUT6 */
 0x8460f34c ,	/* EQQCOEFFSOUT7 */
 0x8460f350 ,	/* EQQCOEFFSOUT8 */
 0x8460f354 ,	/* EQQCOEFFSOUT9 */
 0x8460f358 ,	/* EQQCOEFFSOUT10 */
 0x0000f40e ,	/* DEMOD */
 0x0000f410 ,	/* RCOMPC */
 0x0000f412 ,	/* AGC1CN */
 0x0000f413 ,	/* AGC1REF */
 0x0000f417 ,	/* RTC */
 0x0000f418 ,	/* TMGCFG */
 0x0000f419 ,	/* AGC2REF */
 0x0000f41a ,	/* TLSR */
 0x0000f41b ,	/* CFD */
 0x0000f41c ,	/* ACLC */
 0x0000f41d ,	/* BCLC */
 0x0000f41e ,	/* EQON */
 0x0000f41f ,	/* LDT */
 0x0000f420 ,	/* LDT2 */
 0x0000f425 ,	/* EQUALREF */
 0x0000f426 ,	/* TMGRAMP */
 0x0000f427 ,	/* TMGTHD */
 0x0000f428 ,	/* IDCCOMP */
 0x0000f429 ,	/* QDCCOMP */
 0x0000f42a ,	/* POWERI */
 0x0000f42b ,	/* POWERQ */
 0x0000f42c ,	/* RCOMP */
 0x0000f42e ,	/* AGCIQIN */
 0x0000f436 ,	/* AGC2I1 */
 0x0000f437 ,	/* AGC2I2 */
 0x0000f438 ,	/* TLIR */
 0x0000f439 ,	/* RTF */
 0x0000f43a ,	/* DSTATUS */
 0x0000f43b ,	/* LDI */
 0x0000f43e ,	/* CFRM */
 0x0000f43f ,	/* CFRL */
 0x0000f440 ,	/* NIRM */
 0x0000f441 ,	/* NIRL */
 0x0000f444 ,	/* ISYMB */
 0x0000f445 ,	/* QSYMB */
 0x0000f446 ,	/* SFRH */
 0x0000f447 ,	/* SFRM */
 0x0000f448 ,	/* SFRL */
 0x0000f44c ,	/* SFRUPH */
 0x0000f44d ,	/* SFRUPM */
 0x0000f44e ,	/* SFRUPL */
 0x0000f4e0 ,	/* EQUAI1 */
 0x0000f4e1 ,	/* EQUAQ1 */
 0x0000f4e2 ,	/* EQUAI2 */
 0x0000f4e3 ,	/* EQUAQ2 */
 0x0000f4e4 ,	/* EQUAI3 */
 0x0000f4e5 ,	/* EQUAQ3 */
 0x0000f4e6 ,	/* EQUAI4 */
 0x0000f4e7 ,	/* EQUAQ4 */
 0x0000f4e8 ,	/* EQUAI5 */
 0x0000f4e9 ,	/* EQUAQ5 */
 0x0000f50c ,	/* DSTATUS2 */
 0x0000f50d ,	/* VSTATUS */
 0x0000f50f ,	/* VERROR */
 0x0000f523 ,	/* IQSWAP */
 0x0000f524 ,	/* ECNTM */
 0x0000f525 ,	/* ECNTL */
 0x0000f526 ,	/* ECNT2M */
 0x0000f527 ,	/* ECNT2L */
 0x0000f528 ,	/* ECNT3M */
 0x0000f529 ,	/* ECNT3L */
 0x0000f530 ,	/* FECAUTO1 */
 0x0000f533 ,	/* FECM */
 0x0000f534 ,	/* VTH12 */
 0x0000f535 ,	/* VTH23 */
 0x0000f536 ,	/* VTH34 */
 0x0000f537 ,	/* VTH56 */
 0x0000f538 ,	/* VTH67 */
 0x0000f539 ,	/* VTH78 */
 0x0000f53c ,	/* PRVIT */
 0x0000f53d ,	/* VITSYNC */
 0x0000f548 ,	/* RSULC */
 0x0000f549 ,	/* TSULC */
 0x0000f54a ,	/* RSLLC */
 0x0000f54b ,	/* TSLPL */
 0x0000f54c ,	/* TSCFGH */
 0x0000f54d ,	/* TSCFGM */
 0x0000f54e ,	/* TSCFGL */
 0x0000f54f ,	/* TSOUT */
 0x0000f550 ,	/* RSSYNC */
 0x0000f551 ,	/* TSINSDELH */
 0x0000f552 ,	/* TSINSDELM */
 0x0000f553 ,	/* TSINSDELL */
 0x0000f55a ,	/* TSLLSTKM */
 0x0000f55b ,	/* TSLLSTKL */
 0x0000f55c ,	/* TSULSTKM */
 0x0000f55d ,	/* TSULSTKL */
 0x0000f55e ,	/* PCKLENUL */
 0x0000f55f ,	/* PCKLENLL */
 0x0000f560 ,	/* RSPCKLEN */
 0x0000f561 ,	/* TSSTATUS */
 0x0000f574 ,	/* ERRCTRL1 */
 0x0000f575 ,	/* ERRCTRL2 */
 0x0000f576 ,	/* ERRCTRL3 */
 0x0000f57b ,	/* DMONMSK1 */
 0x0000f57c ,	/* DMONMSK0 */
 0x0000f583 ,	/* DEMAPVIT */
 0x0000f58c ,	/* PLPARM */
 0x0000f600 ,	/* PDELCTRL */
 0x0000f601 ,	/* PDELCTRL2 */
 0x0000f602 ,	/* BBHCTRL1 */
 0x0000f603 ,	/* BBHCTRL2 */
 0x0000f604 ,	/* HYSTTHRESH */
 0x0000f605 ,	/* MATCSTM */
 0x0000f606 ,	/* MATCSTL */
 0x0000f607 ,	/* UPLCSTM */
 0x0000f608 ,	/* UPLCSTL */
 0x0000f609 ,	/* DFLCSTM */
 0x0000f60a ,	/* DFLCSTL */
 0x0000f60b ,	/* SYNCCST */
 0x0000f60c ,	/* SYNCDCSTM */
 0x0000f60d ,	/* SYNCDCSTL */
 0x0000f60e ,	/* ISIENTRY */
 0x0000f60f ,	/* ISIBITEN */
 0x0000f610 ,	/* MATSTRM */
 0x0000f611 ,	/* MATSTRL */
 0x0000f612 ,	/* UPLSTRM */
 0x0000f613 ,	/* UPLSTRL */
 0x0000f614 ,	/* DFLSTRM */
 0x0000f615 ,	/* DFLSTRL */
 0x0000f616 ,	/* SYNCSTR */
 0x0000f617 ,	/* SYNCDSTRM */
 0x0000f618 ,	/* SYNCDSTRL */
 0x0000f619 ,	/* CFGPDELSTATUS1 */
 0x0000f61a ,	/* CFGPKTDELSTTS2 */
 0x0000f61b ,	/* BBFERRORM */
 0x0000f61c ,	/* BBFERRORL */
 0x0000f61d ,	/* UPKTERRORM */
 0x0000f61e ,	/* UPKTERRORL */
 0x8000fa04 ,	/* BLOCKLNGTH */
 0x8000fa08 ,	/* ROWSTR */
 0x8000fa10 ,	/* BNANDADDR */
 0x8000fa14 ,	/* CNANDADDR */
 0x8000fa1c ,	/* INFOLENGTH */
 0x8000fa20 ,	/* BOT_ADDR */
 0x8000fa24 ,	/* BCHBLKLN */
 0x8000fa28 ,	/* BCHT */
 0x8800fa00 ,	/* CNFGMODE */
 0x8800fa04 ,	/* LDPCSTAT */
 0x8800fa08 ,	/* ITERSCALE */
 0x8800fa0c ,	/* INPUTMODE */
 0x8800fa10 ,	/* LDPCDECRST */
 0x8800fa14 ,	/* CLKPERBYTE */
 0x8800fa18 ,	/* BCHERRORS */
 0x8800fa1c ,	/* LDPCERRORS */
 0x8800fa20 ,	/* BCHMODE */
 0x8800fa24 ,	/* ERRACCPER */
 0x8800fa28 ,	/* BCHERRACC */
 0x8800fa38 ,	/* FECTPSEL */
 0x0000ff10 ,	/* TSTCK */
 0x0000ff11 ,	/* TSTRES */
 0x0000ff12 ,	/* TSTOUT */
 0x0000ff13 ,	/* TSTIN */
 0x0000ff14 ,	/* TSTSYS */
 0x0000ff15 ,	/* TSTCHIP */
 0x0000ff16 ,	/* TSTFREE */
 0x0000ff17 ,	/* TSTI2C */
 0x0000ff1c ,	/* BITSPEEDM */
 0x0000ff1d ,	/* BITSPEEDL */
 0x0000ff1e ,	/* TBUSBIT */
 0x0000ff24 ,	/* TSTDIS */
 0x0000ff25 ,	/* TSTDISRX */
 0x0000ff28 ,	/* TSTJETON */
 0x0000ff40 ,	/* TSTDCADJ */
 0x0000ff41 ,	/* TSTAGC1 */
 0x0000ff42 ,	/* TSTAGC1N */
 0x0000ff48 ,	/* TSTPOLYPH */
 0x0000ff49 ,	/* TSTR */
 0x0000ff4a ,	/* TSTAGC2 */
 0x0000ff4b ,	/* TSTCTL1 */
 0x0000ff4c ,	/* TSTCTL2 */
 0x0000ff4d ,	/* TSTCTL3 */
 0x0000ff50 ,	/* TSTDEMAP */
 0x0000ff51 ,	/* TSTDEMAP2 */
 0x0000ff52 ,	/* TSTDEMMON */
 0x0000ff53 ,	/* TSTRATE */
 0x0000ff54 ,	/* TSTSELOUT */
 0x0000ff55 ,	/* TSYNC */
 0x0000ff56 ,	/* TSTERR */
 0x0000ff58 ,	/* TSTRAM1 */
 0x0000ff59 ,	/* TSTVSELOUT */
 0x0000ff5a ,	/* TSTFORCEIN */
 0x0000ff5c ,	/* TSTRS1 */
 0x0000ff5d ,	/* TSTRS2 */
 0x0000ff5e ,	/* TSTRS3 */
 0x0000f000 ,	/* GHOSTREG */

};
	
U32 STB0899DefVal[STB0899_NBREGS]=	/* Default values for Coder registers	*/
{
 0x00000081 ,	/* ID */
 0x00000032 ,	/* TDISCNTRL1 */
 0x00000000 ,/*0x00000080 ,*/	/* DISCNTRL2 */
 0x00000004 ,	/* DISRX_ST0 */
 0x00000000 ,	/* DISRX_ST1 */
 0x00000000 ,	/* DISPARITY */
 0x00000000 ,	/* DISFIFO */
 0x00000020 ,	/* DISSTATUS */
 0x0000008c ,	/* DISF22 */
 0x0000009a ,	/* DISF22RX */
 0x0000000b ,	/* SYSREG */
 0x00000011 ,	/* ACRPRESC */
 0x0000000a ,	/* ACRDIV1 */
 0x00000005 ,	/* ACRDIV2 */
 0x00000000 ,	/* DACR1 */
 0x00000000 ,	/* DACR2 */
 0x00000000 ,	/* OUTCFG */
 0x00000000 ,	/* MODECFG */
 0x00000032 ,	/* IRQSTATUS3 */
 0x00000000 ,	/* IRQSTATUS2 */
 0x00000000 ,	/* IRQSTATUS1 */
 0x00000000 ,	/* IRQSTATUS0 */
 0x000000f3 ,	/* IRQMSK3 */
 0x000000fc ,	/* IRQMSK2 */
 0x000000ff ,	/* IRQMSK1 */
 0x000000ff ,	/* IRQMSK0 */
 0x00000000 ,	/* IRQCFG */
 0x00000088 ,	/* I2CCFG */
 0x0000005c ,	/* I2CRPT */
 0x00000000 ,	/* IOPVALUE5 */
 0x00000031 ,	/* IOPVALUE4 */
 0x000000f1 ,	/* IOPVALUE3 */
 0x00000090 ,	/* IOPVALUE2 */
 0x00000040 ,	/* IOPVALUE1 */
 0x00000000 ,	/* IOPVALUE0 */
 0x00000082 ,	/* GPIO0CFG */
 0x00000082 ,	/* GPIO1CFG */
 0x00000082 ,	/* GPIO2CFG */
 0x00000082 ,	/* GPIO3CFG */
 0x00000082 ,	/* GPIO4CFG */
 0x00000082 ,	/* GPIO5CFG */
 0x00000082 ,	/* GPIO6CFG */
 0x00000082 ,	/* GPIO7CFG */
 0x00000082 ,	/* GPIO8CFG */
 0x00000082 ,	/* GPIO9CFG */
 0x00000082 ,	/* GPIO10CFG */
 0x00000082 ,	/* GPIO11CFG */
 0x00000082 ,	/* GPIO12CFG */
 0x00000082 ,	/* GPIO13CFG */
 0x00000082 ,	/* GPIO14CFG */
 0x00000082 ,	/* GPIO15CFG */
 0x00000082 ,	/* GPIO16CFG */
 0x00000082 ,	/* GPIO17CFG */
 0x00000082 ,	/* GPIO18CFG */
 0x00000082 ,	/* GPIO19CFG */
 0x00000082 ,	/* GPIO20CFG */
 0x000000b8 ,	/* SDATCFG */
 0x000000ba ,	/* SCLTCFG */
 0x0000001c ,	/* AGCRFCFG */
 0x00000082 ,	/* AGCBB2CFG */
 0x00000091 ,	/* AGCBB1CFG */
 0x00000082 ,	/* DIRCLKCFG */
 0x0000007e ,	/* CKOUT27CFG */
 0x00000082 ,	/* STDBYCFG */
 0x00000082 ,	/* CS0CFG */
 0x00000082 ,	/* CS1CFG */
 0x00000020 ,	/* DISEQCOCFG */
 0x00000082 ,	/* GPIO32CFG */
 0x00000082 ,	/* GPIO33CFG */
 0x00000082 ,	/* GPIO34CFG */
 0x00000082 ,	/* GPIO35CFG */
 0x00000082 ,	/* GPIO36CFG */
 0x00000082 ,	/* GPIO37CFG */
 0x00000082 ,	/* GPIO38CFG */
 0x00000082 ,	/* GPIO39CFG */
 0x00000015 ,	/* NCOARSE */
 0x00000002 ,	/* SYNTCTRL */
 0x00000000 ,	/* FILTCTRL */
 0x00000000 ,	/* SYSCTRL */
 0x00000020 ,	/* STOPCLK1 */
 0x00000000 ,	/* STOPCLK2 */
 0x0000000b ,	/* TSTTNR1 */
 0x00000000 ,	/* TSTTNR2 */
 0x00000000 ,	/* TSTTNR3 */
 0x00000000 ,	/* INTBUFSTATUS */
 0x0000000a ,	/* INTBUFCTRL */
 0x00000002 ,	/* DMDSTATUS */
 0x3ed097b6 ,	/* CRLFREQ */
 0x00003ff8 ,	/* BTRFREQ */
 0x00003fff ,	/* IFAGCGAIN */
 0x00000deb ,	/* BBAGCGAIN */
 0x000002ff ,	/* DCOFFSET */
 0x0000000f ,	/* DMDCNTRL */
 0x03fb4a20 ,	/* IFAGCCNTRL */
 0x00200c17 ,	/* BBAGCCNTRL */
 0x00000016 ,	/* CRLCNTRL */
 0x00000000 ,	/* CRLPHSINIT */
 0x00000000 ,	/* CRLFREQINIT */
 0x00000000 ,	/* CRLLOOPGAIN */
 0x3ed097b6 ,	/* CRLNOMFREQ */
 0x00000000 ,	/* CRLSWPRATE */
 0x00000000 ,	/* CRLMAXSWP */
 0x0f6cdc01 ,	/* CRLLKCNTRL */
 0x00000000 ,	/* DECIMCNTRL */
 0x00003993 ,	/* BTRCNTRL */
 0x000d3c6f ,	/* BTRLOOPGAIN */
 0x00000000 ,	/* BTRPHSINIT */
 0x00000000 ,	/* BTRFREQINIT */
 0x0238e38e ,	/* BTRNOMFREQ */
 0x00000000 ,	/* BTRLKCNTRL */
 0x00000000 ,	/* DECNCNTRL */
 0x00000000 ,	/* TPCNTRL */
 0x00000000 ,	/* TPBUFSTATUS */
 0x00000000 ,	/* DCESTIM */
 0x00000000 ,	/* FLLCNTRL */
 0x40ed8000 ,	/* FLLFREQWD */
 0x00000001 ,	/* ANTIALIASSEL */
 0x00000002 ,	/* RRCALPHA */
 0x00000000 ,	/* DCADAPTISHFT */
 0x000019de ,	/* IMBOFFSET */
 0x00000000 ,	/* IMBESTIMATE */
 0x00000001 ,	/* IMBCNTRL */
 0x00005007 ,	/* IFAGCCNTRL2 */
 0x00000002 ,	/* DMDCNTRL2 */
 0x00000000 ,	/* TPBUFFER */
 0x00000000 ,	/* TPBUFFER1 */
 0x00000000 ,	/* TPBUFFER2 */
 0x00000000 ,	/* TPBUFFER3 */
 0x00000000 ,	/* TPBUFFER4 */
 0x00000000 ,	/* TPBUFFER5 */
 0x00000000 ,	/* TPBUFFER6 */
 0x00000000 ,	/* TPBUFFER7 */
 0x00000000 ,	/* TPBUFFER8 */
 0x00000000 ,	/* TPBUFFER9 */
 0x00000000 ,	/* TPBUFFER10 */
 0x00000000 ,	/* TPBUFFER11 */
 0x00000000 ,	/* TPBUFFER12 */
 0x00000000 ,	/* TPBUFFER13 */
 0x00000000 ,	/* TPBUFFER14 */
 0x00000000 ,	/* TPBUFFER15 */
 0x00000000 ,	/* TPBUFFER16 */
 0x00000000 ,	/* TPBUFFER17 */
 0x0000ff00 ,	/* TPBUFFER18 */
 0x00000200 ,	/* TPBUFFER19 */
 0x0000fc00 ,	/* TPBUFFER20 */
 0x00000e02 ,	/* TPBUFFER21 */
 0x00001b17 ,	/* TPBUFFER22 */
 0x00001c12 ,	/* TPBUFFER23 */
 0x0000d72d ,	/* TPBUFFER24 */
 0x0000e7fe ,	/* TPBUFFER25 */
 0x0000d3e5 ,	/* TPBUFFER26 */
 0x0000d9f0 ,	/* TPBUFFER27 */
 0x0000f7d4 ,	/* TPBUFFER28 */
 0x0000d91e ,	/* TPBUFFER29 */
 0x0000cfc7 ,	/* TPBUFFER30 */
 0x0000c1f6 ,	/* TPBUFFER31 */
 0x0000f6b7 ,	/* TPBUFFER32 */
 0x0000bcf1 ,	/* TPBUFFER33 */
 0x00000ec8 ,	/* TPBUFFER34 */
 0x0000cbf9 ,	/* TPBUFFER35 */
 0x0000c3e4 ,	/* TPBUFFER36 */
 0x0000badc ,	/* TPBUFFER37 */
 0x0000dd34 ,	/* TPBUFFER38 */
 0x0000c3c6 ,	/* TPBUFFER39 */
 0x0000d038 ,	/* TPBUFFER40 */
 0x0000d1d2 ,	/* TPBUFFER41 */
 0x0000c307 ,	/* TPBUFFER42 */
 0x0000ddca ,	/* TPBUFFER43 */
 0x0000baea ,	/* TPBUFFER44 */
 0x0000e2ca ,	/* TPBUFFER45 */
 0x0000bcf7 ,	/* TPBUFFER46 */
 0x0000c404 ,	/* TPBUFFER47 */
 0x0000bed1 ,	/* TPBUFFER48 */
 0x0000c31f ,	/* TPBUFFER49 */
 0x0000bcc6 ,	/* TPBUFFER50 */
 0x0000ced7 ,	/* TPBUFFER51 */
 0x0000d8be ,	/* TPBUFFER52 */
 0x0000bce1 ,	/* TPBUFFER53 */
 0x0000c4b7 ,	/* TPBUFFER54 */
 0x0000c0e7 ,	/* TPBUFFER55 */
 0x0000c1b6 ,	/* TPBUFFER56 */
 0x0000c0e4 ,	/* TPBUFFER57 */
 0x0000c2bd ,	/* TPBUFFER58 */
 0x0000c1d8 ,	/* TPBUFFER59 */
 0x0000c2ce ,	/* TPBUFFER60 */
 0x0000c1bd ,	/* TPBUFFER61 */
 0x0000c1c2 ,	/* TPBUFFER62 */
 0x0000c1c2 ,	/* TPBUFFER63 */
 0x00000001 ,	/* RESETCNTRL */
 0x00005654 ,	/* ACMENABLE */
 0x00000000 ,	/* DESCRCNTRL */
 0x00020019 ,	/* CSMCNTRL1 */
 0x004b3237 ,	/* CSMCNTRL2 */
 0x0003dd17 ,	/* CSMCNTRL3 */
 0x00008008 ,	/* CSMCNTRL4 */
 0x002a3106 ,	/* UWPCNTRL1 */
 0x0006140a ,	/* UWPCNTRL2 */
 0x00008000 ,	/* UWPSTAT1 */
 0x00000000 ,	/* UWPSTAT2 */
 0x00000000 ,	/* DMDSTAT2 */
 0x00000471 ,	/* FREQADJSCALE */
 0x017b0465 ,	/* UWPCNTRL3 */
 0x00000002 ,	/* SYMCLKSEL */
 0x00196464 ,	/* SOFSRCHTO */
 0x00000603 ,	/* ACQCNTRL1 */
 0x02046666 ,	/* ACQCNTRL2 */
 0x10046583 ,	/* ACQCNTRL3 */
 0x00010404 ,	/* FESETTLE */
 0x0002aa8a ,	/* ACDWELL */
 0x00000000 ,	/* ACQUIRETRIG */
 0x00000001 ,	/* LOCKLOST */
 0x00000500 ,	/* ACQSTAT1 */
 0x0028a0a0 ,	/* ACQTIMEOUT */
 0x00000000 ,	/* ACQTIME */
 0x00800c17 ,	/* FINALAGCCNTRL */
 0x00000a08 ,	/* FINALAGCCGAIN */
 0x00000000 ,	/* EQUILIZERINIT */
 0x00054802 ,	/* EQCNTL */
 0x00000000 ,	/* EQIINITCOEFF0 */
 0x00000000 ,	/* EQIINITCOEFF1 */
 0x00000000 ,	/* EQIINITCOEFF2 */
 0x00000000 ,	/* EQIINITCOEFF3 */
 0x00000000 ,	/* EQIINITCOEFF4 */
 0x00000400 ,	/* EQIINITCOEFF5 */
 0x00000000 ,	/* EQIINITCOEFF6 */
 0x00000000 ,	/* EQIINITCOEFF7 */
 0x00000000 ,	/* EQIINITCOEFF8 */
 0x00000000 ,	/* EQIINITCOEFF9 */
 0x00000000 ,	/* EQIINITCOEFF10 */
 0x00000000 ,	/* EQQINITCOEFF0 */
 0x00000000 ,	/* EQQINITCOEFF1 */
 0x00000000 ,	/* EQQINITCOEFF2 */
 0x00000000 ,	/* EQQINITCOEFF3 */
 0x00000000 ,	/* EQQINITCOEFF4 */
 0x00000000 ,	/* EQQINITCOEFF5 */
 0x00000000 ,	/* EQQINITCOEFF6 */
 0x00000000 ,	/* EQQINITCOEFF7 */
 0x00000000 ,	/* EQQINITCOEFF8 */
 0x00000000 ,	/* EQQINITCOEFF9 */
 0x00000000 ,	/* EQQINITCOEFF10 */
 0x00000ffd ,	/* EQICOEFFSOUT0 */
 0x00000002 ,	/* EQICOEFFSOUT1 */
 0x00000ffe ,	/* EQICOEFFSOUT2 */
 0x00000006 ,	/* EQICOEFFSOUT3 */
 0x00000fdd ,	/* EQICOEFFSOUT4 */
 0x000002ee ,	/* EQICOEFFSOUT5 */
 0x0000000e ,	/* EQICOEFFSOUT6 */
 0x00000000 ,	/* EQICOEFFSOUT7 */
 0x00000fff ,	/* EQICOEFFSOUT8 */
 0x00000ffe ,	/* EQICOEFFSOUT9 */
 0x00000000 ,	/* EQICOEFFSOUT10 */
 0x00000fff ,	/* EQQCOEFFSOUT0 */
 0x00000000 ,	/* EQQCOEFFSOUT1 */
 0x00000ffc ,	/* EQQCOEFFSOUT2 */
 0x0000000e ,	/* EQQCOEFFSOUT3 */
 0x00000fd9 ,	/* EQQCOEFFSOUT4 */
 0x00000ffd ,	/* EQQCOEFFSOUT5 */
 0x00000008 ,	/* EQQCOEFFSOUT6 */
 0x00000ffc ,	/* EQQCOEFFSOUT7 */
 0x00000ffc ,	/* EQQCOEFFSOUT8 */
 0x00000002 ,	/* EQQCOEFFSOUT9 */
 0x00000ffd ,	/* EQQCOEFFSOUT10 */
 0x00000000 ,	/* DEMOD */
 0x000000c9 ,	/* RCOMPC */
 0x00000001 ,	/* AGC1CN */
 0x00000010 ,	/* AGC1REF */
 0x0000007a ,	/* RTC */
 0x0000004e ,	/* TMGCFG */
 0x00000034 ,	/* AGC2REF */
 0x00000084 ,	/* TLSR */
 0x000000c7 ,	/* CFD */
 0x00000087 ,	/* ACLC */
 0x00000094 ,	/* BCLC */
 0x00000041 ,	/* EQON */
 0x000000dd ,	/* LDT */
 0x000000c9 ,	/* LDT2 */
 0x000000b4 ,	/* EQUALREF */
 0x00000010 ,	/* TMGRAMP */
 0x00000030 ,	/* TMGTHD */
 0x000000fd ,	/* IDCCOMP */
 0x00000001 ,	/* QDCCOMP */
 0x00000008 ,	/* POWERI */
 0x00000009 ,	/* POWERQ */
 0x0000006f ,	/* RCOMP */
 0x00000080 ,	/* AGCIQIN */
 0x00000009 ,	/* AGC2I1 */
 0x000000d9 ,	/* AGC2I2 */
 0x00000030 ,	/* TLIR */
 0x0000007e ,	/* RTF */
 0x00000000 ,	/* DSTATUS */
 0x000000b0 ,	/* LDI */
 0x000000fd ,	/* CFRM */
 0x00000045 ,	/* CFRL */
 0x0000002c ,	/* NIRM */
 0x00000003 ,	/* NIRL */
 0x0000001d ,	/* ISYMB */
 0x000000ff ,	/* QSYMB */
 0x0000002f ,	/* SFRH */
 0x00000068 ,	/* SFRM */
 0x00000040 ,	/* SFRL */
 0x0000002f ,	/* SFRUPH */
 0x00000068 ,	/* SFRUPM */
 0x00000040 ,	/* SFRUPL */
 0x00000000 ,	/* EQUAI1 */
 0x000000fd ,	/* EQUAQ1 */
 0x00000001 ,	/* EQUAI2 */
 0x000000fd ,	/* EQUAQ2 */
 0x00000003 ,	/* EQUAI3 */
 0x000000fc ,	/* EQUAQ3 */
 0x00000002 ,	/* EQUAI4 */
 0x000000fe ,	/* EQUAQ4 */
 0x00000007 ,	/* EQUAI5 */
 0x000000fe ,	/* EQUAQ5 */
 0x00000000 ,	/* DSTATUS2 */
 0x00000000 ,	/* VSTATUS */
 0x000000ff ,	/* VERROR */
 0x00000020 ,	/* IQSWAP */
 0x00000000 ,	/* ECNTM */
 0x00000030 ,	/* ECNTL */
 0x00000000 ,	/* ECNT2M */
 0x00000021 ,	/* ECNT2L */
 0x0000002f ,	/* ECNT3M */
 0x00000009 ,	/* ECNT3L */
 0x00000006 ,	/* FECAUTO1 */
 0x00000000 ,	/* FECM */
 0x000000b0 ,	/* VTH12 */
 0x0000007a ,	/* VTH23 */
 0x00000058 ,	/* VTH34 */
 0x00000038 ,	/* VTH56 */
 0x00000034 ,	/* VTH67 */
 0x00000024 ,	/* VTH78 */
 0x000000ff ,	/* PRVIT */
 0x00000019 ,	/* VITSYNC */
 0x000000b1 ,	/* RSULC */
 0x00000042 ,	/* TSULC */
 0x00000041 ,	/* RSLLC */
 0x00000012 ,	/* TSLPL */
 0x0000000c ,	/* TSCFGH */
 0x00000000 ,	/* TSCFGM */
 0x00000000 ,	/* TSCFGL */
 0x00000073 ,	/* TSOUT */
 0x00000000 ,	/* RSSYNC */
 0x00000002 ,	/* TSINSDELH */
 0x00000000 ,	/* TSINSDELM */
 0x00000000 ,	/* TSINSDELL */
 0x00000013 ,	/* TSLLSTKM */
 0x000000f7 ,	/* TSLLSTKL */
 0x00000000 ,	/* TSULSTKM */
 0x00000000 ,	/* TSULSTKL */
 0x000000bc ,	/* PCKLENUL */
 0x000000cc ,	/* PCKLENLL */
 0x00000083 ,	/* RSPCKLEN */
 0x00000080 ,	/* TSSTATUS */
 0x000000b6 ,	/* ERRCTRL1 */
 0x00000096 ,	/* ERRCTRL2 */
 0x00000089 ,	/* ERRCTRL3 */
 0x00000027 ,	/* DMONMSK1 */
 0x00000003 ,	/* DMONMSK0 */
 0x0000005c ,	/* DEMAPVIT */
 0x0000000d ,	/* PLPARM */
 0x00000048 ,	/* PDELCTRL */
 0x00000000 ,	/* PDELCTRL2 */
 0x00000000 ,	/* BBHCTRL1 */
 0x00000000 ,	/* BBHCTRL2 */
 0x00000077 ,	/* HYSTTHRESH */
 0x00000000 ,	/* MATCSTM */
 0x00000000 ,	/* MATCSTL */
 0x00000000 ,	/* UPLCSTM */
 0x00000000 ,	/* UPLCSTL */
 0x00000000 ,	/* DFLCSTM */
 0x00000000 ,	/* DFLCSTL */
 0x00000000 ,	/* SYNCCST */
 0x00000000 ,	/* SYNCDCSTM */
 0x00000000 ,	/* SYNCDCSTL */
 0x00000000 ,	/* ISIENTRY */
 0x00000000 ,	/* ISIBITEN */
 0x0000001f ,	/* MATSTRM */
 0x000000ff ,	/* MATSTRL */
 0x000000da ,	/* UPLSTRM */
 0x0000001e ,	/* UPLSTRL */
 0x00000078 ,	/* DFLSTRM */
 0x0000002e ,	/* DFLSTRL */
 0x000000b1 ,	/* SYNCSTR */
 0x000000d9 ,	/* SYNCDSTRM */
 0x00000025 ,	/* SYNCDSTRL */
 0x0000001d ,	/* CFGPDELSTATUS1 */
 0x00000027 ,	/* CFGPKTDELSTTS2 */
 0x00000000 ,	/* BBFERRORM */
 0x00000007 ,	/* BBFERRORL */
 0x00000000 ,	/* UPKTERRORM */
 0x000000a3 ,	/* UPKTERRORL */
 0x00000008 ,	/* BLOCKLNGTH */
 0x000000b4 ,	/* ROWSTR */
 0x000004b5 ,	/* BNANDADDR */
 0x00000b4b ,	/* CNANDADDR */
 0x00000078 ,	/* INFOLENGTH */
 0x000001e0 ,	/* BOT_ADDR */
 0x0000a8c0 ,	/* BCHBLKLN */
 0x0000000c ,	/* BCHT */
 0x00000001 ,	/* CNFGMODE */
 0x0000021d ,	/* LDPCSTAT */
 0x00000040 ,	/* ITERSCALE */
 0x00000000 ,	/* INPUTMODE */
 0x00000000 ,	/* LDPCDECRST */
 0x00000008 ,	/* CLKPERBYTE */
 0x0000000c ,	/* BCHERRORS */
 0x0000083f ,	/* LDPCERRORS */
 0x00000000 ,	/* BCHMODE */
 0x00000008 ,	/* ERRACCPER */
 0x00000006 ,	/* BCHERRACC */
 0x00000000 ,	/* FECTPSEL */
 0x00000000 ,	/* TSTCK */
 0x00000000 ,	/* TSTRES */
 0x00000000 ,	/* TSTOUT */
 0x00000000 ,	/* TSTIN */
 0x00000000 ,	/* TSTSYS */
 0x00000000 ,	/* TSTCHIP */
 0x00000000 ,	/* TSTFREE */
 0x00000000 ,	/* TSTI2C */
 0x00000000 ,	/* BITSPEEDM */
 0x00000000 ,	/* BITSPEEDL */
 0x00000000 ,	/* TBUSBIT */
 0x00000000 ,	/* TSTDIS */
 0x00000000 ,	/* TSTDISRX */
 0x00000000 ,	/* TSTJETON */
 0x00000000 ,	/* TSTDCADJ */
 0x00000000 ,	/* TSTAGC1 */
 0x00000000 ,	/* TSTAGC1N */
 0x00000000 ,	/* TSTPOLYPH */
 0x000000c0 ,	/* TSTR */
 0x00000000 ,	/* TSTAGC2 */
 0x00000000 ,	/* TSTCTL1 */
 0x00000000 ,	/* TSTCTL2 */
 0x00000000 ,	/* TSTCTL3 */
 0x00000000 ,	/* TSTDEMAP */
 0x00000000 ,	/* TSTDEMAP2 */
 0x00000000 ,	/* TSTDEMMON */
 0x00000000 ,	/* TSTRATE */
 0x00000000 ,	/* TSTSELOUT */
 0x00000000 ,	/* TSYNC */
 0x00000000 ,	/* TSTERR */
 0x00000000 ,	/* TSTRAM1 */
 0x00000000 ,	/* TSTVSELOUT */
 0x00000000 ,	/* TSTFORCEIN */
 0x00000000 ,	/* TSTRS1 */
 0x00000000 ,	/* TSTRS2 */
 0x00000000 ,	/* TSTRS3 */
 0x00000081 ,	/* GHOSTREG */

};
	

#define STB0899_SYMBOL_RATE_MIN 1000000
#define STB0899_SYMBOL_RATE_MAX 45000000

#define STB0899_FREQ_MIN  950000
#define STB0899_FREQ_MAX  2150000

#define STB0899_MAX_BER 100

#define angle_threshold 1042
/* private variables ------------------------------------------------------- */

static semaphore_t *Lock_InitTermOpenClose; /* guard calls to the functions */


static BOOL        Installed = FALSE;

/* instance chain, the default boot value is invalid, to catch errors */
static STB0899_InstanceData_t *InstanceChainTop = (STB0899_InstanceData_t *)0x7fffffff;

/**************extern from  open.c************************/
#ifdef STTUNER_DRV_SAT_SCR
#ifdef STTUNER_DRV_SAT_SCR_LOOPTHROUGH
extern U32 DemodDrvHandleOne;
#endif
#endif

/* For DiSEqC2.0*/
#ifdef STTUNER_DISEQC2_SWDECODE_VIA_PIO	
static U32 IndexforISR;
STTUNER_InstanceDbase_t *InstforISR;
#endif 


/* functions --------------------------------------------------------------- */

/* API */
ST_ErrorCode_t demod_stb0899_Init(ST_DeviceName_t *DeviceName, DEMOD_InitParams_t *InitParams);
ST_ErrorCode_t demod_stb0899_Term(ST_DeviceName_t *DeviceName, DEMOD_TermParams_t *TermParams);

ST_ErrorCode_t demod_stb0899_Open (ST_DeviceName_t *DeviceName, DEMOD_OpenParams_t  *OpenParams, DEMOD_Capability_t *Capability, DEMOD_Handle_t *Handle);
ST_ErrorCode_t demod_stb0899_Close(DEMOD_Handle_t  Handle, DEMOD_CloseParams_t *CloseParams);

ST_ErrorCode_t demod_stb0899_IsAnalogCarrier (DEMOD_Handle_t Handle, BOOL *IsAnalog);        
ST_ErrorCode_t demod_stb0899_GetSignalQuality(DEMOD_Handle_t Handle, U32  *SignalQuality_p, U32 *Ber);
ST_ErrorCode_t demod_stb0899_GetModulation   (DEMOD_Handle_t Handle, STTUNER_Modulation_t *Modulation);
ST_ErrorCode_t demod_stb0899_SetModulation   (DEMOD_Handle_t Handle, STTUNER_Modulation_t  Modulation);
ST_ErrorCode_t demod_stb0899_GetModeCode     (DEMOD_Handle_t Handle, STTUNER_ModeCode_t  *ModeCode);
ST_ErrorCode_t demod_stb0899_GetAGC          (DEMOD_Handle_t Handle, S16                  *Agc);
ST_ErrorCode_t demod_stb0899_GetFECRates     (DEMOD_Handle_t Handle, STTUNER_FECRate_t    *FECRates);
ST_ErrorCode_t demod_stb0899_GetIQMode       (DEMOD_Handle_t Handle, STTUNER_IQMode_t     *IQMode); /*added for GNBvd26107->I2C failure due to direct access to demod device at API level*/
ST_ErrorCode_t demod_stb0899_IsLocked        (DEMOD_Handle_t Handle, BOOL                 *IsLocked);
ST_ErrorCode_t demod_stb0899_SetFECRates     (DEMOD_Handle_t Handle, STTUNER_FECRate_t     FECRates);
ST_ErrorCode_t demod_stb0899_Tracking        (DEMOD_Handle_t Handle, BOOL ForceTracking,   U32 *NewFrequency, BOOL *SignalFound);
ST_ErrorCode_t demod_stb899_StandByMode(DEMOD_Handle_t Handle, STTUNER_StandByMode_t PowerMode);

ST_ErrorCode_t demod_stb0899_ScanFrequency   (DEMOD_Handle_t Handle, U32  InitialFrequency, U32   SymbolRate,    U32   MaxOffset, 
                                                                   U32  TunerStep,        U8    DerotatorStep, BOOL *ScanSuccess,
                                                                   U32 *NewFrequency,     U32   Mode,          U32   Guard,
                                                                   U32  Force,            U32   Hierarchy,     U32   Spectrum,
                                                                   U32  FreqOff,          U32   ChannelBW,     S32   EchoPos);

ST_ErrorCode_t demod_stb0899_ioctl           (DEMOD_Handle_t Handle, U32 Function, void *InParams, void *OutParams, STTUNER_Da_Status_t *Status);

/* added for DiSEqC API support*/
ST_ErrorCode_t demod_stb0899_DiSEqC          (DEMOD_Handle_t Handle, 
									    STTUNER_DiSEqCSendPacket_t *pDiSEqCSendPacket,
									    STTUNER_DiSEqCResponsePacket_t *pDiSEqCResponsePacket
									    );

ST_ErrorCode_t demod_stb0899_DiSEqCGetConfig ( DEMOD_Handle_t Handle ,STTUNER_DiSEqCConfig_t * DiSEqCConfig);
ST_ErrorCode_t demod_stb0899_DiSEqCBurstOFF ( DEMOD_Handle_t Handle );

ST_ErrorCode_t demod_stb0899_Tonedetection(DEMOD_Handle_t Handle,U32 StartFreq, U32 StopFreq,U8  *NbTones,U32 *ToneList, U8 mode, int* power_detection_level);
/* I/O API */
ST_ErrorCode_t demod_stb0899_ioaccess(DEMOD_Handle_t Handle, IOARCH_Handle_t IOHandle,
                  STTUNER_IOARCH_Operation_t Operation, U16 SubAddr, U8 *Data, U32 TransferSize, U32 Timeout);
ST_ErrorCode_t demod_stb0899_DiseqcInit(DEMOD_Handle_t Handle);

/* local functions --------------------------------------------------------- */

STB0899_InstanceData_t *STB0899_GetInstFromHandle(DEMOD_Handle_t Handle);

BOOL checkLUT(int Value, U32 ModeCode)
{
	if(Value == ModeCode)
	return TRUE;
	else
	return FALSE;
}

/* ----------------------------------------------------------------------------
Name: STTUNER_DRV_DEMOD_STB0899_Install()

Description:
    install a satellite device driver into the demod database.
    
Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_DRV_DEMOD_STB0899_Install(STTUNER_demod_dbase_t *Demod)
{

    ST_ErrorCode_t Error = ST_NO_ERROR;

    if(Installed == TRUE)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_STB0899
        STTBX_Print(("%s fail driver already installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }
   
#ifdef STTUNER_DEBUG_MODULE_SATDRV_STB0899
    STTBX_Print(("%s installing sat:demod:STB0899...", identity));
#endif

    /* mark ID in database */
    Demod->ID = STTUNER_DEMOD_STB0899;

    /* map API */
    Demod->demod_Init = demod_stb0899_Init;
    Demod->demod_Term = demod_stb0899_Term;

    Demod->demod_Open  = demod_stb0899_Open;
    Demod->demod_Close = demod_stb0899_Close;

    Demod->demod_IsAnalogCarrier  = demod_stb0899_IsAnalogCarrier; 
    Demod->demod_GetSignalQuality = demod_stb0899_GetSignalQuality;
    Demod->demod_GetModulation    = demod_stb0899_GetModulation;   
    Demod->demod_SetModulation    = demod_stb0899_SetModulation;   
    Demod->demod_GetAGC           = demod_stb0899_GetAGC;          
    Demod->demod_GetIQMode        = demod_stb0899_GetIQMode; /*added for GNBvd26107->I2C failure due to direct access to demod device at API level*/
    Demod->demod_GetFECRates      = demod_stb0899_GetFECRates;      
    Demod->demod_IsLocked         = demod_stb0899_IsLocked ;   
    Demod->demod_SetFECRates      = demod_stb0899_SetFECRates;     
    Demod->demod_Tracking         = demod_stb0899_Tracking;        
    Demod->demod_ScanFrequency    = demod_stb0899_ScanFrequency;
    #ifdef STTUNER_DRV_SAT_STB0899
    Demod->demod_GetModeCode    = demod_stb0899_GetModeCode;
    #endif
    /*Added for DiSEqC Support*/
	Demod->demod_DiSEqC	  = demod_stb0899_DiSEqC;
	Demod->demod_GetConfigDiSEqC   = demod_stb0899_DiSEqCGetConfig;  
	Demod->demod_SetDiSEqCBurstOFF   = demod_stb0899_DiSEqCBurstOFF;

    Demod->demod_ioaccess = demod_stb0899_ioaccess;
    Demod->demod_ioctl    = demod_stb0899_ioctl;
    Demod->demod_tonedetection = demod_stb0899_Tonedetection;
    Demod->demod_StandByMode      = demod_stb899_StandByMode;

    InstanceChainTop = NULL;

    Lock_InitTermOpenClose = STOS_SemaphoreCreateFifo(NULL,1);

    Installed = TRUE;

#ifdef STTUNER_DEBUG_MODULE_SATDRV_STB0899
    STTBX_Print(("ok\n"));
#endif

    return(Error);
}



/* ----------------------------------------------------------------------------
Name: STTUNER_DRV_DEMOD_STB0899_UnInstall()

Description:
    install a satellite device driver into the demod database.
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_DRV_DEMOD_STB0899_UnInstall(STTUNER_demod_dbase_t *Demod)
{

    ST_ErrorCode_t Error = ST_NO_ERROR;

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_STB0899
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }
   
    if(Demod->ID != STTUNER_DEMOD_STB0899)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_STB0899
        STTBX_Print(("%s fail incorrect driver type\n", identity));
#endif
        return(STTUNER_ERROR_ID);
    }

    /* has all memory been freed, by Term() */
    if(InstanceChainTop != NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_STB0899
        STTBX_Print(("%s fail at least one instance not terminated\n", identity));
#endif
        return(ST_ERROR_OPEN_HANDLE);
    }


#ifdef STTUNER_DEBUG_MODULE_SATDRV_STB0899
    STTBX_Print(("%s uninstalling sat:demod:STB0899...", identity));
#endif

    /* mark ID in database */
    Demod->ID = STTUNER_NO_DRIVER;

    /* unmap API */
    Demod->demod_Init = NULL;
    Demod->demod_Term = NULL;

    Demod->demod_Open  = NULL;
    Demod->demod_Close = NULL;

    Demod->demod_IsAnalogCarrier  = NULL; 
    Demod->demod_GetSignalQuality = NULL;
    Demod->demod_GetModulation    = NULL;   
    Demod->demod_SetModulation    = NULL;   
    Demod->demod_GetAGC           = NULL;          
    Demod->demod_GetFECRates      = NULL;
    Demod->demod_GetIQMode        = NULL; /*added for GNBvd26107->I2C failure due to direct access to demod device at API level*/
    Demod->demod_IsLocked         = NULL;       
    Demod->demod_SetFECRates      = NULL;     
    Demod->demod_Tracking         = NULL;        
    Demod->demod_ScanFrequency    = NULL;
    
	Demod->demod_DiSEqC	          = NULL;
	Demod->demod_GetConfigDiSEqC  = NULL;  
	Demod->demod_SetDiSEqCBurstOFF= NULL;
	#ifdef STTUNER_DRV_SAT_STB0899
    Demod->demod_GetModeCode    = NULL;
    #endif

    Demod->demod_ioaccess = NULL;
    Demod->demod_ioctl    = NULL;
    Demod->demod_tonedetection = NULL;
    Demod->demod_StandByMode   = NULL;

#ifdef STTUNER_DEBUG_MODULE_SATDRV_STB0899
        STTBX_Print(("<"));
#endif

        STOS_SemaphoreDelete(NULL,Lock_InitTermOpenClose);
      
#ifdef STTUNER_DEBUG_MODULE_SATDRV_STB0899
        STTBX_Print((">"));
#endif

    InstanceChainTop = (STB0899_InstanceData_t *)0x7ffffffe;
    Installed        = FALSE;

#ifdef STTUNER_DEBUG_MODULE_SATDRV_STB0899
        STTBX_Print(("ok\n"));
#endif

    return(Error);
}

/* ----------------------------------------------------------------------------
Name: demod_stb0899_Init()

Description:
    
Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_stb0899_Init(ST_DeviceName_t *DeviceName, DEMOD_InitParams_t *InitParams)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_STB0899
    const char *identity = "STTUNER d_0899.c demod_stb0899_Init()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STB0899_InstanceData_t *InstanceNew, *Instance;

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_STB0899
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

    /* now safe to lock semaphore */
    SEM_LOCK(Lock_InitTermOpenClose);

    /* ---------- check params ---------- */
    Error = STTUNER_Util_CheckPtrNull(InitParams->MemoryPartition);
    if( Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_STB0899
        STTBX_Print(("%s fail MemoryPartition not valid\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    Error = STTUNER_Util_CheckPtrNull(DeviceName);
    if( Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_STB0899
        STTBX_Print(("%s fail DeviceName not valid\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    InstanceNew = STOS_MemoryAllocateClear(InitParams->MemoryPartition, 1, sizeof( STB0899_InstanceData_t ));
    if (InstanceNew == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_STB0899
        STTBX_Print(("%s fail memory allocation InstanceNew\n", identity));
#endif    
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_NO_MEMORY);           
    }

    /* slot into chain */
    if (InstanceChainTop == NULL)
    {
        InstanceNew->InstanceChainPrev = NULL; /* no previous instance */
        InstanceChainTop = InstanceNew;
    }
    else    /* tag onto last data block in chain */
    {
        Instance = InstanceChainTop;

        while(Instance->InstanceChainNext != NULL)
        {
            Instance = Instance->InstanceChainNext;   /* next block */
        }
        Instance->InstanceChainNext     = (void *)InstanceNew;
        InstanceNew->InstanceChainPrev  = (void *)Instance;
    }

    InstanceNew->DeviceName          = DeviceName;
    InstanceNew->TopLevelHandle      = STTUNER_MAX_HANDLES;
    InstanceNew->IOHandle            = InitParams->IOHandle;
    InstanceNew->MemoryPartition     = InitParams->MemoryPartition;
    InstanceNew->InstanceChainNext   = NULL; /* always last in the chain */

    InstanceNew->DeviceMap.Timeout   = IOREG_DEFAULT_TIMEOUT;
    InstanceNew->DeviceMap.Registers = STB0899_NBREGS;
    InstanceNew->DeviceMap.Fields    = STB0899_NBFIELDS;
    InstanceNew->DeviceMap.Mode      = IOREG_MODE_SUBADR_16_POINTED; /* NEW as of 3.4.0: i/o addressing mode to use */
    InstanceNew->DeviceMap.MemoryPartition = InitParams->MemoryPartition;
    InstanceNew->DeviceMap.DefVal    = (U32 *)&STB0899DefVal[0];
    InstanceNew->ExternalClock       = InitParams->ExternalClock;  
    InstanceNew->TSOutputMode        = InitParams->TSOutputMode;
    /*InstanceNew->SerialDataMode      = InitParams->SerialDataMode;*/
    InstanceNew->SerialClockSource   = InitParams->SerialClockSource;
    InstanceNew->FE_STB0899_Modulation   = FE_899_MOD_QPSK; /* default to QPSK modulation */
    InstanceNew->FECMode = InitParams->FECMode;
    InstanceNew->FECType = STTUNER_FEC_MODE_DVBS2;
    InstanceNew->ClockPolarity       = InitParams->ClockPolarity;
    InstanceNew->DataClockAtParityBytes = InitParams->DataClockAtParityBytes;
    InstanceNew->TSDataOutputControl = InitParams->TSDataOutputControl;
    InstanceNew->BlockSyncMode       = InitParams->BlockSyncMode;
    InstanceNew->DataFIFOMode        = InitParams->DataFIFOMode;
    InstanceNew->OutputFIFOConfig    = InitParams->OutputFIFOConfig;
    InstanceNew->DiSEqC_RxFreq       = 22000;
    
    
    /********Added for DiSEqC***************************/
	InstanceNew->DiSEqCConfig.Command=STTUNER_DiSEqC_COMMAND;
    InstanceNew->DiSEqCConfig.ToneState=STTUNER_DiSEqC_TONE_CONTINUOUS_OFF;
	/**************************************************/  
	InstanceNew->SET_TRACKING_PARAMS = FALSE;
   /* reserve memory for register mapping */
    Error = STTUNER_IOREG_Open(&InstanceNew->DeviceMap);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_STB0899
        STTBX_Print(("%s fail setup new register database\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);           
    }
    
   
#ifdef STTUNER_DEBUG_MODULE_SATDRV_STB0899
    STTBX_Print(("%s allocated & initalized block named '%s' at 0x%08x (%d bytes)\n", identity, InstanceNew->DeviceName, (U32)InstanceNew, sizeof( D0399_InstanceData_t ) ));
#endif
    SEM_UNLOCK(Lock_InitTermOpenClose);
    return(Error);
}   



/* ----------------------------------------------------------------------------
Name: demod_stb0899_Term()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_stb0899_Term(ST_DeviceName_t *DeviceName, DEMOD_TermParams_t *TermParams)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_STB0899
   const char *identity = "STTUNER d0399.c demod_d0399_Term()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STB0899_InstanceData_t *Instance, *InstancePrev, *InstanceNext;

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_STB0899
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

    /* now safe to lock semaphore */
    SEM_LOCK(Lock_InitTermOpenClose);

    /* ---------- check params ---------- */
    Error = STTUNER_Util_CheckPtrNull(TermParams);
    if( Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_STB0899
        STTBX_Print(("%s fail TermParams not valid\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    /* ---------- check that at least one init has taken place ---------- */
    if(InstanceChainTop == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_STB0899
        STTBX_Print(("%s fail nothing initalized\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(STTUNER_ERROR_INITSTATE);
    }


    /* reap next matching DeviceName */
#ifdef STTUNER_DEBUG_MODULE_SATDRV_STB0899
    STTBX_Print(("%s reap next matching DeviceName[", identity));
#endif

    Instance = InstanceChainTop;
    while(1)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_STB0899
        STTBX_Print(("(%s)", Instance->DeviceName));
#endif
        if ( strcmp( (char *)Instance->DeviceName, (char *)DeviceName) == 0)
        {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_STB0899
            STTBX_Print(("<-- ]\n"));
#endif
            Error = STTUNER_IOREG_Close(&Instance->DeviceMap);
            if (Error != ST_NO_ERROR)
            {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
                STTBX_Print(("%s fail close register database\n", identity));
#endif
            }
            /* found so now xlink prev and next(if applicable) and deallocate memory */
            InstancePrev = Instance->InstanceChainPrev;
            InstanceNext = Instance->InstanceChainNext;

            /* if instance to delete is first in chain */
            if (Instance->InstanceChainPrev == NULL)
            {
                InstanceChainTop = InstanceNext;        /* which would be NULL if last block to be term'd */
                if(InstanceNext != NULL)
                { 
                InstanceNext->InstanceChainPrev = NULL; /* now top of chain, no previous instance */
                }
            }
            else
            {   /* safe to set value for prev instaance (because there IS one) */
                InstancePrev->InstanceChainNext = InstanceNext;
            }

            /* if there is a next block in the chain */            
            if (InstanceNext != NULL)
            {
                InstanceNext->InstanceChainPrev = InstancePrev;
            }   

            STOS_MemoryDeallocate(Instance->MemoryPartition, Instance);

#ifdef STTUNER_DEBUG_MODULE_SATDRV_STB0899
            STTBX_Print(("%s freed block at 0x%08x\n", identity, (U32)Instance ));
#endif

#ifdef STTUNER_DEBUG_MODULE_SATDRV_STB0899
            STTBX_Print(("%s terminated ok\n", identity));
#endif
            SEM_UNLOCK(Lock_InitTermOpenClose);
            return(Error);
        }
        else if(Instance->InstanceChainNext == NULL) 
        {       /* error we should have found a matching name before the end of the list */
#ifdef STTUNER_DEBUG_MODULE_SATDRV_STB0899
                STTBX_Print(("\n%s fail no free handle before end of list\n", identity));
#endif
                SEM_UNLOCK(Lock_InitTermOpenClose);
                return(STTUNER_ERROR_INITSTATE);
        }
        else
        {
            Instance = Instance->InstanceChainNext;   /* next block */
        }
        
    }


#ifdef STTUNER_DEBUG_MODULE_SATDRV_STB0899
    STTBX_Print(("%s FAIL! this point should NEVER be reached\n", identity));
#endif
    SEM_UNLOCK(Lock_InitTermOpenClose);
    return(Error);
}   



/* ----------------------------------------------------------------------------
Name: demod_stb0899_Open()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_stb0899_Open(ST_DeviceName_t *DeviceName, DEMOD_OpenParams_t  *OpenParams, DEMOD_Capability_t *Capability, DEMOD_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_STB0899
   const char *identity = "STTUNER d_0899.c demod_stb0899_Open()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STB0899_InstanceData_t *Instance;
    U8 ChipID = 0;
    int ENA8_LevelValue;
   
    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_STB0899
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

    /* now safe to lock semaphore */
    SEM_LOCK(Lock_InitTermOpenClose);

    /* ---------- check that at least one init has taken place ---------- */
    if(InstanceChainTop == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_STB0899
        STTBX_Print(("%s fail nothing initalized\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(STTUNER_ERROR_INITSTATE);
    }

    /* find  handle from name */
    Instance = InstanceChainTop;
#ifdef STTUNER_DEBUG_MODULE_SATDRV_STB0899
    STTBX_Print(("%s looking (%s)", identity, Instance->DeviceName));
#endif
    while(strcmp((char *)Instance->DeviceName, (char *)DeviceName) != 0)
    {
        /* error, should have found matching DeviceName before end of list */
        if(Instance->InstanceChainNext == NULL) 
        {       
#ifdef STTUNER_DEBUG_MODULE_SATDRV_STB0899
            STTBX_Print(("\n%s fail no block named '%s' before end of list\n", identity, DeviceName));
#endif
            SEM_UNLOCK(Lock_InitTermOpenClose);
            return(STTUNER_ERROR_INITSTATE);
        }
        Instance = Instance->InstanceChainNext;   /* next block */
#ifdef STTUNER_DEBUG_MODULE_SATDRV_STB0899
        STTBX_Print(("(%s)", Instance->DeviceName));
#endif
    }
#ifdef STTUNER_DEBUG_MODULE_SATDRV_STB0899
        STTBX_Print((" found ok\n"));
#endif

#ifdef STTUNER_DEBUG_MODULE_SATDRV_STB0899
    STTBX_Print(("%s using block at 0x%08x\n", identity, (U32)Instance));
#endif

    /* check handle IS actually free */
    if (Instance->TopLevelHandle != STTUNER_MAX_HANDLES)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_STB0899
        STTBX_Print(("%s fail driver instance already open\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_OPEN_HANDLE);
    }
    /* else now got pointer to free (and valid) data block */

     /* check to see if chip is of the expected type, this is done afer register setup with
    reset values because the standby/sleep mode may have been done and  Reg0399_Reset() wakes
    the device up (depending on the polarity of the STDBY pin - see stb0899 data sheet */

    /* nb: Instance->FE_399_Handle points to a  FE_399_InternalParams_t cast to a FE_399_Handle_t
           with STCHIP_Handle_t as the first element */     
    ChipID = STTUNER_IOREG_GetRegister(&(Instance->DeviceMap), Instance->IOHandle, RSTB0899_ID);

    if (((ChipID & 0xF0) !=  0x30)&&((ChipID & 0xF0) !=  0x80))   /* 0899 ID code 0x10 cut 1.0, 0x81 for Cut3.1 */
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_STB0899
        STTBX_Print(("%s fail device not found (0x%02x)\n", identity, ChipID));
#endif
       
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_UNKNOWN_DEVICE);
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_STB0899
        STTBX_Print(("%s device found, release/revision=%u\n", identity, ChipID & 0x0F));
#endif
    }
    
    /* reset all chip registers */
   Error = STTUNER_IOREG_Reset_SizeU32(&(Instance->DeviceMap), Instance->IOHandle,STB0899DefVal, AddressArray);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_STB0899
        STTBX_Print(("%s fail reset device\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);           
    }

    /* Set serial data mode */
    if (Instance->TSOutputMode == STTUNER_TS_MODE_SERIAL)   
    {     
    	Error |= STTUNER_IOREG_SetField_SizeU32(&(Instance->DeviceMap), Instance->IOHandle, FSTB0899_OUTRS_PS,FSTB0899_OUTRS_PS_INFO, 1);
    	Error |= STTUNER_IOREG_SetRegister(&(Instance->DeviceMap), Instance->IOHandle, RSTB0899_TSOUT,0x23); 
    }
    else
    {
    	Error |= STTUNER_IOREG_SetField_SizeU32(&(Instance->DeviceMap), Instance->IOHandle, FSTB0899_OUTRS_PS,FSTB0899_OUTRS_PS_INFO, 0);
    	Error |= STTUNER_IOREG_SetRegister(&(Instance->DeviceMap), Instance->IOHandle, RSTB0899_TSOUT,0x7f); 
    }
    
    if (Instance->ClockPolarity == STTUNER_DATA_CLOCK_POLARITY_FALLING)    /* Rising Edge */
    {
        Error |= STTUNER_IOREG_SetField_SizeU32(&(Instance->DeviceMap), Instance->IOHandle, FSTB0899_CLK_POL,FSTB0899_CLK_POL_INFO, 1);
    }
    else
    {
        Error |= STTUNER_IOREG_SetField_SizeU32(&(Instance->DeviceMap), Instance->IOHandle, FSTB0899_CLK_POL, FSTB0899_CLK_POL_INFO,0);
    }
        
    if (Instance->DataClockAtParityBytes == STTUNER_DATACLOCK_NULL_AT_PARITYBYTES)     /* Parity enable changes done for 5528 live decode issue*/
    {
        Error |= STTUNER_IOREG_SetField_SizeU32(&(Instance->DeviceMap), Instance->IOHandle, FSTB0899_FORCE0, FSTB0899_FORCE0_INFO,1);
    }
    else
    {
        Error |= STTUNER_IOREG_SetField_SizeU32(&(Instance->DeviceMap), Instance->IOHandle, FSTB0899_FORCE0,FSTB0899_FORCE0_INFO, 0);
    }
  
   if(Instance->BlockSyncMode == STTUNER_SYNC_FORCED)
   {
   		Error |= STTUNER_IOREG_SetField_SizeU32(&(Instance->DeviceMap), Instance->IOHandle, FSTB0899_MPEG, FSTB0899_MPEG_INFO,1);
   }
   else
   {
   		Error |= STTUNER_IOREG_SetField_SizeU32(&(Instance->DeviceMap), Instance->IOHandle, FSTB0899_MPEG,FSTB0899_MPEG_INFO, 0);
   }   	
   

    
   if(Instance->TSDataOutputControl == STTUNER_TSDATA_OUTPUT_CONTROL_MANUAL)  
   {
   	   Error |= STTUNER_IOREG_SetField_SizeU32(&(Instance->DeviceMap), Instance->IOHandle, FSTB0899_ENA_AUTO, FSTB0899_ENA_AUTO_INFO, 0);
	   
	   if(Instance->DataFIFOMode == STTUNER_DATAFIFO_ENABLED)
	   {
	   	if(Instance->OutputFIFOConfig.OutputRateCompensationMode == STTUNER_COMPENSATE_DATACLOCK)
	   	{
	   		Error |= STTUNER_IOREG_SetField_SizeU32(&(Instance->DeviceMap), Instance->IOHandle, FSTB0899_EN_STBACKEND, FSTB0899_EN_STBACKEND_INFO,   1);
	   	}
	   	else if(Instance->OutputFIFOConfig.OutputRateCompensationMode == STTUNER_COMPENSATE_DATAVALID)
	   	{
	   		Error |= STTUNER_IOREG_SetField_SizeU32(&(Instance->DeviceMap), Instance->IOHandle, FSTB0899_EN_STBACKEND, FSTB0899_EN_STBACKEND_INFO,   0);
	   	}
	   	
	   	if (Instance->TSOutputMode == STTUNER_TS_MODE_SERIAL)
	   	{
	   		ENA8_LevelValue = 0x01;/* use only ENA8_LEVEL[3:2] in serial mode*/
	   		ENA8_LevelValue |= ((Instance->OutputFIFOConfig.CLOCKPERIOD - 1)<<2);
	   		Error |= STTUNER_IOREG_SetField_SizeU32(&(Instance->DeviceMap), Instance->IOHandle, FSTB0899_ENA_LEVEL, FSTB0899_ENA_LEVEL_INFO,ENA8_LevelValue);
	   	}
	   	else if (Instance->TSOutputMode == STTUNER_TS_MODE_PARALLEL)
	   	{
	   		ENA8_LevelValue = (Instance->OutputFIFOConfig.CLOCKPERIOD) / 4;
	   		Error |= STTUNER_IOREG_SetField_SizeU32(&(Instance->DeviceMap), Instance->IOHandle, FSTB0899_ENA_LEVEL, FSTB0899_ENA_LEVEL_INFO,ENA8_LevelValue);
	   	}
	   }
    }
    else
    {
    	Error |= STTUNER_IOREG_SetField_SizeU32(&(Instance->DeviceMap), Instance->IOHandle, FSTB0899_ENA_AUTO,  FSTB0899_ENA_AUTO_INFO,1);
    }

    /* Set capabilties */
    Capability->FECAvail        = STTUNER_FEC_1_2 | STTUNER_FEC_2_3 | STTUNER_FEC_3_4 |
                                  STTUNER_FEC_5_6 | STTUNER_FEC_6_7 | STTUNER_FEC_7_8 |
                                  STTUNER_FEC_8_9 | STTUNER_FEC_1_4 | STTUNER_FEC_1_3 |
                                  STTUNER_FEC_2_5 | STTUNER_FEC_3_5 | STTUNER_FEC_9_10;  /* direct mapping to STTUNER_FECRate_t    */
    Capability->ModulationAvail = STTUNER_MOD_ALL;  /* direct mapping to STTUNER_Modulation_t */
    Capability->AGCControl      = FALSE;
    Capability->SymbolMin       = STB0899_SYMBOL_RATE_MIN; /*   1 MegaSymbols/sec */
    Capability->SymbolMax       = STB0899_SYMBOL_RATE_MAX; /*  45 MegaSymbols/sec */
    Capability->FreqMin         = STB0899_FREQ_MIN;  /* 950000 */
    Capability->FreqMax         = STB0899_FREQ_MAX;  /* 2150000 */
    
    /* finally (as nor more errors to check for, allocate handle */
    Instance->TopLevelHandle = OpenParams->TopLevelHandle;
       
    /* in loopthriugh mode the second demod which is connected by loopthrough of first cannot send disecq-st 
       command because of a DC block in the loop-through path. Presently it depends upon toplevelhandle but when tone detection 
       LLA of SatCR will be written "DISECQ_ST_ENABLE" will be initalized automatically depending upon the tone sent*/
    #ifdef STTUNER_DRV_SAT_SCR
		#ifdef STTUNER_DRV_SAT_SCR_LOOPTHROUGH
		    if(OpenParams->TopLevelHandle == 0)
		    Instance->DISECQ_ST_ENABLE = TRUE;
		    if(OpenParams->TopLevelHandle == 1)
		    Instance->DISECQ_ST_ENABLE = FALSE;
	    #endif
	#endif
   
	 *Handle = (U32)Instance;
	 /* for(;;)*/
/*   Error |= demod_stb0899_DiseqcInit(*Handle);*/
#ifdef STTUNER_DEBUG_MODULE_SATDRV_STB0899
    STTBX_Print(("%s opened ok\n", identity));
#endif

    SEM_UNLOCK(Lock_InitTermOpenClose);
    
    return(Error);
}   



/* ----------------------------------------------------------------------------
Name: demod_stb0899_Close()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_stb0899_Close(DEMOD_Handle_t Handle, DEMOD_CloseParams_t *CloseParams)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_STB0899
   const char *identity = "STTUNER d_0899.c demod_stb0899_Close()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STB0899_InstanceData_t     *Instance;

    /* private driver instance data */
    Instance = STB0899_GetInstFromHandle(Handle);

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_STB0899
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

    SEM_LOCK(Lock_InitTermOpenClose);

    /* ---------- check that at least one init has taken place ---------- */
    if(InstanceChainTop == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_STB0899
        STTBX_Print(("%s fail nothing initalized\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(STTUNER_ERROR_INITSTATE);
    }

    if(Instance->TopLevelHandle == STTUNER_MAX_HANDLES)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_STB0899
        STTBX_Print(("%s fail driver instance not open\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_OPEN_HANDLE);
    }
   
    /* indidcate insatance is closed */
    Instance->TopLevelHandle = STTUNER_MAX_HANDLES;

#ifdef STTUNER_DEBUG_MODULE_SATDRV_STB0899
    STTBX_Print(("%s closed\n", identity));
#endif

    SEM_UNLOCK(Lock_InitTermOpenClose);
    return(Error);
}   




/* ----------------------------------------------------------------------------
Name: demod_d0899_StandByMode()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */



ST_ErrorCode_t demod_stb899_StandByMode(DEMOD_Handle_t Handle, STTUNER_StandByMode_t PowerMode)
{
	
#ifdef STTUNER_DEBUG_MODULE_SATDRV_STB0899
   const char *identity = "STTUNER d0899.c demod_stb899_StandByMode()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STB0899_InstanceData_t *Instance;

   
    /* private driver instance data */
    Instance = STB0899_GetInstFromHandle(Handle);
    Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
   
    return(Error);
}




/* ----------------------------------------------------------------------------
Name: demod_stb0899_IsAnalogCarrier()

Description:
    This routine checks for an analog carrier on the current frequency
    by setting the symbol rate to 5M (never a digital signal).

Parameters:
    IsAnalog,   pointer to area to store result:
                TRUE - is analog
                FALSE - is not analog

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_stb0899_IsAnalogCarrier(DEMOD_Handle_t Handle, BOOL *IsAnalog)   /* TODO */
{

    ST_ErrorCode_t Error = ST_NO_ERROR;
    
    
    return(Error);
}   



/* ----------------------------------------------------------------------------
Name: demod_stb0899_GetSignalQuality()

Description:
  Obtains a signal quality setting for the current lock.
    
Parameters:
    SignalQuality_p,    pointer to area to store the signal quality value.
    Ber,                pointer to area to store the bit error rate.
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_stb0899_GetSignalQuality(DEMOD_Handle_t Handle, U32  *SignalQuality_p, U32 *Ber)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_STB0899
   const char *identity = "STTUNER d0899.c demod_stb0899_GetSignalQuality()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    
    STB0899_InstanceData_t     *Instance;
    /* private driver instance data */
    Instance = STB0899_GetInstFromHandle(Handle);
    *Ber = FE_STB0899_GetError(&Instance->DeviceMap, Instance->IOHandle, Instance->FECType, Instance->FECMode);
    *SignalQuality_p = FE_STB0899_GetCarrierQuality(&Instance->DeviceMap, Instance->IOHandle, Instance->FECType, Instance->FECMode);
    return(Error);
}   



/* ----------------------------------------------------------------------------
Name: demod_stb0899_GetModulation()

Description:
    This routine returns the modulation scheme in use by this device.
    
Parameters:
    Modulation, pointer to area to store modulation scheme.
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_stb0899_GetModulation(DEMOD_Handle_t Handle, STTUNER_Modulation_t *Modulation)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_STB0899
   const char *identity = "STTUNER d0899.c demod_stb0899_GetModulation()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    U8 Data,u8 = 0;
    U32 ModCod;
    STB0899_InstanceData_t     *Instance;

    /* private driver instance data */
    Instance = STB0899_GetInstFromHandle(Handle);
    if((Instance->FECType ==STTUNER_FEC_MODE_DVBS1) || (Instance->FECMode == STTUNER_FEC_MODE_DIRECTV))
    {
	    Data = STTUNER_IOREG_GetField_SizeU32(&(Instance->DeviceMap), Instance->IOHandle, FSTB0899_VIT_MAPPING, FSTB0899_VIT_MAPPING_INFO);
	
	    /* Convert 399 value to a STTUNER fecrate */
	    switch (Data)
	    {
	        case FE_899_MOD_BPSK:
	            *Modulation = STTUNER_MOD_BPSK;
	            break;
	
	        case FE_899_MOD_QPSK:
	            *Modulation = STTUNER_MOD_QPSK;
	            break;
	
	        case FE_899_MOD_8PSK:
	            *Modulation = STTUNER_MOD_8PSK;
	            break;
	
	        
	        default:
	            *Modulation = STTUNER_MOD_NONE;
	            break;
	    }
	}
	else
	{
		ModCod = STTUNER_IOREG_GetField_SizeU32(&(Instance->DeviceMap), Instance->IOHandle,FSTB0899_UWP_DECODED_MODCODE,FSTB0899_UWP_DECODED_MODCODE_INFO)>>2;
		while(!checkLUT(ModCodLUT[u8][0], ModCod)) u8++;
		*Modulation = ModCodLUT[u8][1];
		Error = Instance->DeviceMap.Error;
	}
	
    return(Error);
}   


ST_ErrorCode_t demod_stb0899_GetModeCode(DEMOD_Handle_t Handle, STTUNER_ModeCode_t  *ModeCode)
{
	 STB0899_InstanceData_t     *Instance;
	 ST_ErrorCode_t Error = ST_NO_ERROR;

     Instance = STB0899_GetInstFromHandle(Handle);
		
		*ModeCode = STTUNER_IOREG_GetField_SizeU32(&(Instance->DeviceMap), Instance->IOHandle,FSTB0899_UWP_DECODED_MODCODE,FSTB0899_UWP_DECODED_MODCODE_INFO)>>2;
		Error = Instance->DeviceMap.Error;
		return Error;
}


/* ----------------------------------------------------------------------------
Name: demod_stb0899_SetModulation()

Description:
    This routine sets the modulation scheme for use when scanning.
    
Parameters:
    Modulation.
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_stb0899_SetModulation(DEMOD_Handle_t Handle, STTUNER_Modulation_t Modulation)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_STB0899
   const char *identity = "STTUNER d0899.c demod_stb0899_SetModulation()";
#endif
    STTUNER_InstanceDbase_t  *Inst;
    STB0899_InstanceData_t   *Instance;
    ST_ErrorCode_t Error = ST_NO_ERROR;
    
    Instance = STB0899_GetInstFromHandle(Handle);
    Inst = STTUNER_GetDrvInst();
    
    switch(Modulation)
    {
        case STTUNER_MOD_BPSK:
            Instance->FE_STB0899_Modulation = FE_899_MOD_BPSK;
            break;

        case STTUNER_MOD_QPSK:
            Instance->FE_STB0899_Modulation = FE_899_MOD_QPSK;
            break;

        case STTUNER_MOD_8PSK:
            Instance->FE_STB0899_Modulation = FE_899_MOD_8PSK;
            break;

        default:
            return(ST_ERROR_BAD_PARAMETER);
    }
    if((Inst[Instance->TopLevelHandle].CurrentTunerInfo.ScanInfo.FECType == STTUNER_FEC_MODE_DVBS1) || ( Instance->FECMode == STTUNER_FEC_MODE_DIRECTV))
    {   
	   Error = STTUNER_IOREG_SetField_SizeU32( &(Instance->DeviceMap), Instance->IOHandle, FSTB0899_VIT_MAPPING, FSTB0899_VIT_MAPPING_INFO,Instance->FE_STB0899_Modulation);
    }

	#ifdef STTUNER_DEBUG_MODULE_SATDRV_STB0899
	    STTBX_Print(("%s Modulation=%u\n", identity, Modulation));
	#endif

    return(ST_NO_ERROR);
}   



/* ----------------------------------------------------------------------------
Name: demod_stb0899_GetAGC()

Description:
    Obtains the current RF signal power.

Parameters:
    AGC,    pointer to area to store power output value.
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_stb0899_GetAGC(DEMOD_Handle_t Handle, S16  *Agc)
{

	ST_ErrorCode_t Error = ST_NO_ERROR;
	STB0899_InstanceData_t *Instance;
	
        Instance = STB0899_GetInstFromHandle(Handle);
	
	*Agc = FE_STB0899_GetRFLevel(&(Instance->DeviceMap), Instance->IOHandle, Instance->FECType, Instance->FECMode);
	
	return(Error);
}   



/* ----------------------------------------------------------------------------
Name: demod_stb0899_GetFECRates()

Description:
     Checks the VEN rate register to deduce the forward error correction
    setting that is currently in use.
   
Parameters:
    FECRates,   pointer to area to store FEC rates in use.
  
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_stb0899_GetFECRates(DEMOD_Handle_t Handle, STTUNER_FECRate_t *FECRates)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_STB0899
   const char *identity = "STTUNER d0899.c demod_stb0899_GetFECRates()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_FECRate_t CurFecRate = 0;
    U8 Data, u8= 0;
    U32 ModCod;
    if((STB0899_GetInstFromHandle(Handle)->FECType == STTUNER_FEC_MODE_DVBS1) || (STB0899_GetInstFromHandle(Handle)->FECMode == STTUNER_FEC_MODE_DIRECTV))
    {
    	Data = STTUNER_IOREG_GetField_SizeU32( &(STB0899_GetInstFromHandle(Handle)->DeviceMap), STB0899_GetInstFromHandle(Handle)->IOHandle, FSTB0899_VIT_CURPUN,FSTB0899_VIT_CURPUN_INFO);

	    /* Convert 399 value to a STTUNER fecrate */
	    switch (Data)
	    {
	        case FE_899_1_2:
	            CurFecRate = STTUNER_FEC_1_2;
	            break;
	
	        case FE_899_2_3:
	            CurFecRate = STTUNER_FEC_2_3;
	            break;
	
	        case FE_899_3_4:
	            CurFecRate = STTUNER_FEC_3_4;
	            break;
	
	        case FE_899_5_6:
	            CurFecRate = STTUNER_FEC_5_6;
	            break;
	
	        case FE_899_6_7:
	            CurFecRate = STTUNER_FEC_6_7;
	            break;
	
	        case FE_899_7_8:
	            CurFecRate = STTUNER_FEC_7_8;
	            break;
	
	        
	        default:
	            CurFecRate = STTUNER_FEC_NONE;
	            break;
	    }
	}
	else
	{
		ModCod = STTUNER_IOREG_GetField_SizeU32(&(STB0899_GetInstFromHandle(Handle)->DeviceMap), STB0899_GetInstFromHandle(Handle)->IOHandle,FSTB0899_UWP_DECODED_MODCODE,FSTB0899_UWP_DECODED_MODCODE_INFO)>>2;
		while(!checkLUT(ModCodLUT[u8][0], ModCod)) u8++;
		CurFecRate = ModCodLUT[u8][2];
	}

    *FECRates = CurFecRate;  /* Copy back for caller */
    
#ifdef STTUNER_DEBUG_MODULE_SATDRV_STB0899
    STTBX_Print(("%s FECRate=%u\n", identity, CurFecRate));
#endif
    return(Error);
}   



/* ----------------------------------------------------------------------------
Name: demod_stb0899_GetIQMode()
Function added for GNBvd26107->I2C failure due to direct access to demod device at API level
(Problem:STTUNER_GetTunerInfo was calling STTUNER_SAT_GetTunerInfo, which was reading 
STTUNER_IOCTL_IQMODE directly. This was causing I2C failure due to simultaneous I2C access
to the same device.
Resolution:This functions reads the value for current IQMode from the register which is updated
in the CurrentTunerInfo structure. So now in STTUNER_SAT_GetTunerInfo() (in get.c) no separate
I2C access (due to Ioctl) is required for retrieving the IQ mode )

Description:
     Retrieves the IQMode from the FECM register(NORMAL or INVERTED)
   
Parameters In:
    IQMode, pointer to area to store the IQMode in use.

Parameters Out:
    IQMode
  
Return Value:
    Error
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_stb0899_GetIQMode(DEMOD_Handle_t Handle, STTUNER_IQMode_t *IQMode)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_STB0899
   const char *identity = "STTUNER d0899 demod_stb0899_GetIQMode()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    if((STB0899_GetInstFromHandle(Handle)->FECType == STTUNER_FEC_MODE_DVBS1) || (STB0899_GetInstFromHandle(Handle)->FECMode == STTUNER_FEC_MODE_DIRECTV))
    {
	    	if(STTUNER_IOREG_GetField_SizeU32(&(STB0899_GetInstFromHandle(Handle)->DeviceMap), STB0899_GetInstFromHandle(Handle)->IOHandle, FSTB0899_SYM,FSTB0899_SYM_INFO)==1)
	            *IQMode = STTUNER_IQ_MODE_INVERTED;
	    else
	            *IQMode = STTUNER_IQ_MODE_NORMAL;
    }      
    else
    {
    	if(STTUNER_IOREG_GetField_SizeU32(&(STB0899_GetInstFromHandle(Handle)->DeviceMap), STB0899_GetInstFromHandle(Handle)->IOHandle, FSTB0899_SPECTRUM_INVERT,FSTB0899_SPECTRUM_INVERT_INFO)==1)
	            *IQMode = STTUNER_IQ_MODE_INVERTED;
	    else
	            *IQMode = STTUNER_IQ_MODE_NORMAL;
    }
    
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0899
    STTBX_Print(("%s IQMode=%u\n", identity, *IQMode));
#endif

    return(Error);
}   



/* ----------------------------------------------------------------------------
Name: demod_stb0899_IsLocked()

Description:
     Checks the LK register i.e., are we demodulating a digital carrier.
   
Parameters:
    IsLocked,   pointer to area to store result (bool):
                TRUE -- we are locked.
                FALSE -- no lock.
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_stb0899_IsLocked(DEMOD_Handle_t Handle, BOOL *IsLocked)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0899
   const char *identity = "STTUNER d0899.c demod_d0899_IsLocked()";
#endif
ST_ErrorCode_t Error = ST_NO_ERROR;

    if((STB0899_GetInstFromHandle(Handle)->FECType == STTUNER_FEC_MODE_DVBS1) || (STB0899_GetInstFromHandle(Handle)->FECMode == STTUNER_FEC_MODE_DIRECTV))
    {
	if(STTUNER_IOREG_GetField_SizeU32(&(STB0899_GetInstFromHandle(Handle)->DeviceMap), STB0899_GetInstFromHandle(Handle)->IOHandle, FSTB0899_LOCKEDVIT,FSTB0899_LOCKEDVIT_INFO)==1)
	*IsLocked = TRUE;
	else
	*IsLocked = FALSE;
    }
    else
    {
    	if((((STTUNER_IOREG_GetField_SizeU32(&(STB0899_GetInstFromHandle(Handle)->DeviceMap), STB0899_GetInstFromHandle(Handle)->IOHandle, FSTB0899_LOCK, FSTB0899_LOCK_INFO))&&
				   	(STTUNER_IOREG_GetRegister(&(STB0899_GetInstFromHandle(Handle)->DeviceMap), STB0899_GetInstFromHandle(Handle)->IOHandle, RSTB0899_DMDSTAT2)==0x03))?1:0))
    	{
		*IsLocked = TRUE;
	
		}
		else
		*IsLocked = FALSE;	
    }

#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0899
    STTBX_Print(("%s IsLocked=%u\n", identity, *IsLocked));
#endif

    return(Error = STB0899_GetInstFromHandle(Handle)->DeviceMap.Error);
}   



/* ----------------------------------------------------------------------------
Name: demod_stb0899_SetFECRates()

Description:
    Sets the FEC rates to be used during demodulation.
    Parameters:
    
Parameters:
      FECRates, bitmask of FEC rates to be applied.
  
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_stb0899_SetFECRates(DEMOD_Handle_t Handle, STTUNER_FECRate_t FECRates)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_STB0899
   const char *identity = "STTUNER d0899.c demod_stb0899_SetFECRates()";
#endif
    STTUNER_InstanceDbase_t  *Inst;
    STB0899_InstanceData_t *Instance;
    ST_ErrorCode_t Error = ST_NO_ERROR;
    U32 Data = 0;
    Instance = STB0899_GetInstFromHandle(Handle);
    Inst = STTUNER_GetDrvInst();
    
   if((Inst[Instance->TopLevelHandle].CurrentTunerInfo.ScanInfo.FECType == STTUNER_FEC_MODE_DVBS1) || ( Instance->FECMode == STTUNER_FEC_MODE_DIRECTV))
   {
    	/* Convert STTUNER FEC rates (see sttuner.h) to a VENRATE value to be applied to the stb0899 */
	    if (FECRates & STTUNER_FEC_1_2) Data |= FE_899_1_2;
	    if (FECRates & STTUNER_FEC_2_3) Data |= FE_899_2_3;
	    if (FECRates & STTUNER_FEC_3_4) Data |= FE_899_3_4;
	    if (FECRates & STTUNER_FEC_5_6) Data |= FE_899_5_6;
	    if (FECRates & STTUNER_FEC_6_7) Data |= FE_899_6_7;
	    if (FECRates & STTUNER_FEC_7_8) Data |= FE_899_7_8;
	
	    Error = STTUNER_IOREG_SetField_SizeU32( &Instance->DeviceMap, Instance->IOHandle, FSTB0899_VIT_CURPUN, FSTB0899_VIT_CURPUN_INFO,Data);
	    if ( Error != ST_NO_ERROR)
	    {
	#ifdef STTUNER_DEBUG_MODULE_SATDRV_STB0899
	        STTBX_Print(("%s fail ChipSetField()\n", identity));
	#endif
	    }
	    else
	    {
	#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0999
	        STTBX_Print(("%s Field F0899_RATE = 0x%02x\n", identity, Data));
	#endif
	    }
	}
    return(Error);
}   



/* ----------------------------------------------------------------------------
Name: demod_stb0899_Tracking()

Description:
    This routine checks the carrier against a certain threshold value and will
    perform derotator centering, if necessary -- using the ForceTracking
    option ensures that derotator centering is always performed when
    this routine is called.

    This routine should be periodically called once a lock has been
    established in order to maintain the lock.
    
Parameters:
    ForceTracking, boolean to control whether to always perform
                   derotator centering, regardless of the carrier.

    NewFrequency,  pointer to area where to store the new frequency
                   value -- it may be changed when trying to optimize
                   the derotator.

    SignalFound,   indicates that whether or not we're still locked
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_stb0899_Tracking(DEMOD_Handle_t Handle, BOOL ForceTracking, U32 *NewFrequency, BOOL *SignalFound)
{

    ST_ErrorCode_t Error = ST_NO_ERROR;
    STB0899_InstanceData_t *Instance;
    STTUNER_InstanceDbase_t  *Inst;
    STTUNER_tuner_instance_t *TunerInstance;
    TUNER_Status_t  TunerStatus;  

    FE_STB0899_SignalInfo_t	pInfo; 
    
	Inst = STTUNER_GetDrvInst();
	Instance = STB0899_GetInstFromHandle(Handle);
	/* get the tuner instance for this driver from the top level handle */
        TunerInstance = &Inst[Instance->TopLevelHandle].Sat.Tuner;
	
	Error = (TunerInstance->Driver->tuner_GetStatus)(TunerInstance->DrvHandle, &TunerStatus);
	Error = demod_stb0899_IsLocked(Handle, SignalFound);
	if((Error == ST_NO_ERROR) && (*SignalFound == TRUE))
        {
		
		FE_STB0899_GetSignalInfo(&Instance->DeviceMap, Instance->IOHandle,  Instance->TopLevelHandle,&pInfo,Instance->FECType, Instance->FECMode);
	                
        	
		    *NewFrequency = pInfo.Frequency = TunerInstance->realfrequency;
		    Inst[Instance->TopLevelHandle].CurrentTunerInfo.ScanInfo.SymbolRate = pInfo.SymbolRate;
		    Inst[Instance->TopLevelHandle].CurrentTunerInfo.BitErrorRate = pInfo.BER;
		    Inst[Instance->TopLevelHandle].CurrentTunerInfo.SignalQuality = pInfo.C_N;
		    Inst[Instance->TopLevelHandle].CurrentTunerInfo.ScanInfo.AGC = pInfo.Power;
		    demod_stb0899_GetFECRates(Handle,&Inst[Instance->TopLevelHandle].CurrentTunerInfo.ScanInfo.FECRates);
		    Inst[Instance->TopLevelHandle].CurrentTunerInfo.ScanInfo.IQMode=pInfo.SpectralInv;
		    Inst[Instance->TopLevelHandle].CurrentTunerInfo.ScanInfo.ModeCode=pInfo.ModCode;
		    Inst[Instance->TopLevelHandle].CurrentTunerInfo.ScanInfo.Pilots = pInfo.Pilots;
	        
	        #ifdef STTUNER_DRV_SAT_SCR
	        if((Inst[Instance->TopLevelHandle].Capability.SCREnable)&& (Inst[Instance->TopLevelHandle].CurrentTunerInfo.ScanInfo.LNB_SignalRouting == STTUNER_VIA_SCRENABLED_LNB))
	    	{
	        	*NewFrequency = Inst[Instance->TopLevelHandle].ScanList.ScanList->ScrParams.SCRVCOFrequency - *NewFrequency;
		}
	        #endif
	}
	else
	{
		    *NewFrequency = TunerInstance->realfrequency;
		    Inst[Instance->TopLevelHandle].CurrentTunerInfo.BitErrorRate      = 0;
		    Inst[Instance->TopLevelHandle].CurrentTunerInfo.SignalQuality     = 0;
		    Inst[Instance->TopLevelHandle].CurrentTunerInfo.ScanInfo.AGC      = -1000;
		    Inst[Instance->TopLevelHandle].CurrentTunerInfo.ScanInfo.IQMode   = 0;
		    Inst[Instance->TopLevelHandle].CurrentTunerInfo.ScanInfo.ModeCode = 0;
		    Inst[Instance->TopLevelHandle].CurrentTunerInfo.ScanInfo.Pilots   = 0;
	
	}
	
	
#ifndef ST_OSLINUX
    if(Inst[Instance->TopLevelHandle].Sat.Ioctl.SignalNoise == TRUE)
    {
        {
            static BOOL SRand = TRUE;
            U32 r;

            if (SRand)
            {
                srand((unsigned int)STOS_time_now());
                SRand = FALSE;
            }

            /* Calculate percentage fluctuation from current position */
            r = rand() % 100;               /* Percent change */

            Inst[Instance->TopLevelHandle].CurrentTunerInfo.SignalQuality -= (
                (((Inst[Instance->TopLevelHandle].CurrentTunerInfo.SignalQuality << 7) * r) / 100) >> 7 );

            /* Estimate ber from signal quality based on a linear relationship */
            Inst[Instance->TopLevelHandle].CurrentTunerInfo.BitErrorRate = ( STB0899_MAX_BER - (
                (Inst[Instance->TopLevelHandle].CurrentTunerInfo.BitErrorRate * STB0899_MAX_BER) / 100 ));
        }
    }
#endif       
       return(Error);
}   


/* ----------------------------------------------------------------------------
Name: demod_stb0899_ScanFrequency()

Description:
    This routine will attempt to scan and find a QPSK signal based on the
    passed in parameters.
    
Parameters:
    InitialFrequency, IF value to commence scan (in kHz).
    SymbolRate,       required symbol bit rate (in Hz).
    MaxLNBOffset,     maximum allowed LNB offset (in Hz).
    TunerStep,        Tuner's step value -- enables override of
                      TNR device's internal setting.
    DerotatorStep,    derotator step (usually 6).
    ScanSuccess,      boolean that indicates search success.
    NewFrequency,     pointer to area to store locked frequency.
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_stb0899_ScanFrequency(DEMOD_Handle_t Handle, U32  InitialFrequency, U32   SymbolRate,    U32   MaxOffset, 
                                                                U32  TunerStep,        U8    DerotatorStep, BOOL *ScanSuccess,
                                                                U32 *NewFrequency,     U32   Mode,          U32   Guard,
                                                                U32  Force,            U32   Hierarchy,     U32   Spectrum,
                                                                U32  FreqOff,          U32   ChannelBW,     S32   EchoPos)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
   const char *identity = "STTUNER d0399.c demod_d0399_ScanFrequency()";
#endif
    FE_STB0899_Error_t Error = FE_899_NO_ERROR;
    FE_STB0899_SearchParams_t Params;
    FE_STB0899_SearchResult_t Result;
    STTUNER_InstanceDbase_t  *Inst;
    STB0899_InstanceData_t *Instance;
    #ifdef STTUNER_DRV_SAT_SCR
    	U32 SCRVCOFrequency, TunerFrequency;
    #endif
   
    	/* private driver instance data */
    Instance = STB0899_GetInstFromHandle(Handle);
	Inst = STTUNER_GetDrvInst();
	Params.Frequency   = InitialFrequency;              /* demodulator (output from LNB) frequency (in KHz) */
	Params.SymbolRate  = SymbolRate;                    /* transponder symbol rate  (in bds) */
	
	Params.SearchRange = MaxOffset*2;
	#ifdef STTUNER_DRV_SAT_SCR
	if((Inst[Instance->TopLevelHandle].Capability.SCREnable)&& (Inst[Instance->TopLevelHandle].CurrentTunerInfo.ScanInfo.LNB_SignalRouting == STTUNER_VIA_SCRENABLED_LNB))
    	{
	if(Params.SearchRange<20000000)
	Params.SearchRange = 20000000;
	}
	#endif
    Instance->FECType = Inst[Instance->TopLevelHandle].Sat.ScanInfo.Scan->FECType;	
    Params.Modulation  = Instance->FE_STB0899_Modulation;   /* previously setup modulation */
    Params.DemodIQMode=(STTUNER_IQMode_t) Spectrum;
    Params.Standard = Instance->FECType;
	Params.FECMode = Instance->FECMode;
	/*Params.Pilots = Inst[Instance->TopLevelHandle].Sat.ScanInfo.Scan->Pilots;*/
	if((Instance->FECMode == STTUNER_FEC_MODE_DIRECTV) || (Instance->FECType== STTUNER_FEC_MODE_DVBS1))
	{
		STTUNER_IOREG_SetField_SizeU32(&(Instance->DeviceMap), Instance->IOHandle, FSTB0899_SYNCBYTE, FSTB0899_SYNCBYTE_INFO,1);
	}
	else
	{
		STTUNER_IOREG_SetField_SizeU32(&(Instance->DeviceMap), Instance->IOHandle, FSTB0899_SYNCBYTE,FSTB0899_SYNCBYTE_INFO, 0);
	}   
	/* Set SCR frequency here */
    #ifdef STTUNER_DRV_SAT_SCR
    if((Inst[Instance->TopLevelHandle].Capability.SCREnable)&& (Inst[Instance->TopLevelHandle].CurrentTunerInfo.ScanInfo.LNB_SignalRouting == STTUNER_VIA_SCRENABLED_LNB))
    {
    	Error = scr_scrdrv_SetFrequency(Instance->TopLevelHandle, Handle, Instance->DeviceName, InitialFrequency/1000, Inst[Instance->TopLevelHandle].Sat.ScanInfo.Scan->ScrParams.LNBIndex,Inst[Instance->TopLevelHandle].Sat.ScanInfo.Scan->ScrParams.SCRBPF );
	TunerFrequency = (Inst[Instance->TopLevelHandle].Capability.SCRBPFFrequencies[Inst[Instance->TopLevelHandle].Sat.ScanInfo.Scan->ScrParams.SCRBPF])*1000;
	Inst[Instance->TopLevelHandle].Sat.ScanInfo.Scan->ScrParams.BPFFrequency = TunerFrequency;
	SCRVCOFrequency = TunerFrequency + InitialFrequency;
	Params.Frequency = TunerFrequency;
	Inst[Instance->TopLevelHandle].Sat.ScanInfo.Scan->ScrParams.SCRVCOFrequency = SCRVCOFrequency;
    }
    #endif
		     
   Error |= FE_STB0899_Search( &(Instance->DeviceMap),Instance->IOHandle, &Params, &Result, Instance->TopLevelHandle );
  		      	  
    if (Error == FE_899_BAD_PARAMETER)  /* element(s) of Params bad */
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
        STTBX_Print(("%s fail, scan not done: bad parameter(s) to FE_399_Search() == FE_899_BAD_PARAMETER\n", identity ));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }
    else if (Error == FE_899_SEARCH_FAILED)  /* found no signal within limits */
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
        STTBX_Print(("%s FE_399_Search() == FE_899_SEARCH_FAILED\n", identity ));
#endif
        return(ST_NO_ERROR);    /* no error so try next F or stop if band limits reached */
    }
    *ScanSuccess = Result.Locked;

    if (Result.Locked == TRUE)
    {
        Instance->SET_TRACKING_PARAMS = TRUE;
        #ifdef STTUNER_DRV_SAT_SCR
        if((Inst[Instance->TopLevelHandle].Capability.SCREnable)&& (Inst[Instance->TopLevelHandle].CurrentTunerInfo.ScanInfo.LNB_SignalRouting == STTUNER_VIA_SCRENABLED_LNB))
        {
        	*NewFrequency = (SCRVCOFrequency - TunerFrequency) + (TunerFrequency - Result.Frequency);
        }
        else
        {
        	*NewFrequency = Result.Frequency;
        }
        #else
        *NewFrequency = Result.Frequency;
        #endif

#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
        STTBX_Print(("%s NewFrequency=%u\t", identity, *NewFrequency));
        STTBX_Print(("%s ScanSuccess=%u\n",  identity, *ScanSuccess));
#endif
  return(ST_NO_ERROR); 
    }
 return(ST_NO_ERROR); 
}
/* ----------------------------------------------------------------------------
Name: demod_stb0899_DiseqcInit
Description:
    Selects the DiSEqC F22RX for maximum hits. THis function should be called when DiSEqC 2.0 Rx 
    Envelop mode is not used.
  Parameters:
    Handle,					Demod Handle
  Return Value:
    ST_NO_ERROR,                the operation was carried out without error.
  ---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_stb0899_DiseqcInit(DEMOD_Handle_t Handle)
{
	ST_ErrorCode_t Error = ST_NO_ERROR;      
	U8 f22Tx, f22Rx, sendData[3]={0xE2,0x00,0x00},/*diseqC command to send */
		         ReceveData[8]={0};
	STTUNER_DiSEqCSendPacket_t pDiSEqCSendPacket;
	STTUNER_DiSEqCResponsePacket_t pDiSEqCResponsePacket;
	STB0899_InstanceData_t *Instance;
	U32 mclk, txFreq=22000, RxFreq,
     	    recevedok=0,
    	    i,
    	    trial = 0,	/*try number maximum = 2 (try 20khz and 17.5 khz)*/
    	    sucess1=0,  /*20 Khz sucess */
    	    sucess2=0;  /*17.5 Khz sucess */
    	    
        Instance = STB0899_GetInstFromHandle(Handle);
        pDiSEqCSendPacket.DiSEqCCommandType = STTUNER_DiSEqC_COMMAND;
        pDiSEqCSendPacket.pFrmAddCmdData = &sendData[0];
	pDiSEqCSendPacket.uc_TotalNoOfBytes = 3;
	pDiSEqCResponsePacket.ReceivedFrmAddData = ReceveData;
	
	Error = STTUNER_IOREG_SetField_SizeU32(&Instance->DeviceMap, Instance->IOHandle, FSTB0899_ONECHIPTRX,FSTB0899_ONECHIPTRX_INFO,0); /*Disable Tx spy off */
	Error |= STTUNER_IOREG_SetField_SizeU32(&Instance->DeviceMap, Instance->IOHandle, FSTB0899_DISEQC_RESET,FSTB0899_DISEQC_RESET_INFO,1);
	Error |= STTUNER_IOREG_SetField_SizeU32(&Instance->DeviceMap, Instance->IOHandle, FSTB0899_DISEQC_RESET,FSTB0899_DISEQC_RESET_INFO,0);
	
	mclk=FE_STB0899_GetMclkFreq(&Instance->DeviceMap, Instance->IOHandle, 27000000); 
	f22Tx= mclk /(txFreq*32);
	Error |= STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, RSTB0899_DISF22, f22Tx);/*Set DiseqC Tx freq */
	
	RxFreq=20000;
	f22Rx= mclk /((RxFreq)*32);
	if(Error == ST_NO_ERROR)	
	{
		
		while ((recevedok <5) && (trial <2))
		{
			Instance->DiSEqC_RxFreq = RxFreq;
			STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, RSTB0899_DISF22RX, f22Rx);	/*Set Rx freq 2 possible value 20khz or 17.5 khz*/
			
			for(i=0;i<5;i++)
			{
				pDiSEqCResponsePacket.uc_TotalBytesReceived = 0;
				Error = demod_stb0899_DiSEqC(Handle, &pDiSEqCSendPacket, &pDiSEqCResponsePacket);
				if(Error != ST_NO_ERROR)
				{
					return Error;
				}
				if(pDiSEqCResponsePacket.uc_TotalBytesReceived >=1)
				{
					if((pDiSEqCResponsePacket.ReceivedFrmAddData[pDiSEqCResponsePacket.uc_TotalBytesReceived-1]==0xe4)||(pDiSEqCResponsePacket.ReceivedFrmAddData[pDiSEqCResponsePacket.uc_TotalBytesReceived-1]==0xe5))
					recevedok++;
				}
				
			}	
			
			if(trial==0)
				sucess1=recevedok;
			else
				sucess2=recevedok;
					
			trial++;
			RxFreq=17500;
			f22Rx= mclk /((RxFreq)*32);
		}
		if(sucess1>=sucess2)
		{
			RxFreq=20000;
			f22Rx= mclk /((RxFreq)*32);
		}
		else
		{
			RxFreq=17500;
			f22Rx= mclk /((RxFreq)*32);
		}
		
		Error = STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, RSTB0899_DISF22RX, f22Rx);
        }
	if((sucess1==0)&&(sucess2==0))
	RxFreq=0; /*No diseqC 2.x slave detected */

	Instance->DiSEqC_RxFreq = RxFreq;
	if(RxFreq != 0)
	{
		#ifndef NO_DISEQC_RX_ENVELOPMODE /* In Envelop Receiving Mode, F22Rx = 22000 Hz */
		Instance->DiSEqC_RxFreq = 22000;
		#endif
	}
	return Error;
}

/* ----------------------------------------------------------------------------
Name: demod_stb0899_DiSEqC
Description:
    Sends the DiSEqc command through F22

  Parameters:
    Handle,					handle to sttuner device
    pDiSEqCSendPacket		pointer to the command packet to be sent
	pDiSEqCResponsePacket	pointer to the  packet to be received; void for diseqc 1.2

  Return Value:
    ST_NO_ERROR,                the operation was carried out without error.
    ST_ERROR_BAD_PARAMETER      Handle NULL or incorrect tone setting.

  Calling function  

  See Also:
    Nothing.
---------------------------------------------------------------------------- */

ST_ErrorCode_t demod_stb0899_DiSEqC (DEMOD_Handle_t Handle, 
				     STTUNER_DiSEqCSendPacket_t *pDiSEqCSendPacket,
				     STTUNER_DiSEqCResponsePacket_t *pDiSEqCResponsePacket
				    )
{	
	int Error = ST_NO_ERROR;
	
	unsigned char No_of_Bytes, *pCurrent_Data;
	STB0899_InstanceData_t *Instance;
	unsigned int TimeoutCounter =0 ;
	U32 Delay=0;
	U32 DigFreq, F22_Freq;
	U8 F22, i=0;
	
#ifdef STTUNER_DISEQC2_SWDECODE_VIA_PIO	
	U8 NumofBytes =0,BitPattern[MAX_NUM_BITS_IN_REPLY];
	U8 DecodedOneByte,i,j,ParityCounter;
        U32 DelayCount = 0;
#endif 

#if defined (STTUNER_DRV_SAT_LNBH21) || defined (STTUNER_DRV_SAT_LNB21)
	LNB_Handle_t LHandle;
	STTUNER_InstanceDbase_t *Inst;	
#endif 
	/* If framing byte wants response (DiSEqC 2.0) report error*/
	pCurrent_Data = pDiSEqCSendPacket->pFrmAddCmdData;
       
	Instance = STB0899_GetInstFromHandle(Handle);
	

#if defined (STTUNER_DRV_SAT_LNBH21) || defined (STTUNER_DRV_SAT_LNB21)
	Inst = STTUNER_GetDrvInst();
	LHandle = Inst[Instance->TopLevelHandle].Sat.Lnb.DrvHandle;
	Error = (Inst[Instance->TopLevelHandle].Sat.Lnb.Driver->lnb_setttxmode)(LHandle,STTUNER_LNB_TX);
	if (Error != ST_NO_ERROR)
	{
		return(Error);		
	}
#endif 	
	
#ifdef STTUNER_DISEQC2_SWDECODE_VIA_PIO
	/* Get the top level handle */
	IndexforISR = Instance->TopLevelHandle;
	/* Get the instance database */
	InstforISR = STTUNER_GetDrvInst();	
#endif  	
	/*In loopthrough mode send DiSEqC-ST commands through first demod */
	#ifdef STTUNER_DRV_SAT_SCR
		#ifdef STTUNER_DRV_SAT_SCR_LOOPTHROUGH
		    if(Instance->DISECQ_ST_ENABLE == FALSE)
		    {
			    Handle = DemodDrvHandleOne;
			    Instance = STB0899_InstanceData_t(Handle);
		    }
		#endif
	#endif
	
	/* Set 22KHz before transmission */
	DigFreq = FE_STB0899_GetMclkFreq(&Instance->DeviceMap, Instance->IOHandle,27000000);     /*27000000->Quartz Frequency */
	F22 = STTUNER_IOREG_GetField_SizeU32(&Instance->DeviceMap, Instance->IOHandle, FSTB0899_F22, FSTB0899_F22_INFO);
	F22_Freq = FE_STB0899_F22Freq(DigFreq,F22);
	if(F22_Freq<= 21800 || F22_Freq>=22200)
	{
		F22 = DigFreq/(32*22000);
		STTUNER_IOREG_SetField_SizeU32(&Instance->DeviceMap, Instance->IOHandle, FSTB0899_F22, FSTB0899_F22_INFO,F22);
		WAIT_N_MS_899(2);
	}
	/*****************************************/
	/*Take care for receiving freq as it may vary w.r.t master clock frequency -> add Rx_Freq check here*/
	/*****************************************/
	if(Instance->DiSEqC_RxFreq !=0 )
	{
		STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, RSTB0899_GPIO0CFG, 0x82);
		STTUNER_IOREG_SetField_SizeU32(&Instance->DeviceMap, Instance->IOHandle, FSTB0899_PINSELECT, FSTB0899_PINSELECT_INFO,0);
		STTUNER_IOREG_SetField_SizeU32(&Instance->DeviceMap, Instance->IOHandle, FSTB0899_RECEIVER_ON, FSTB0899_RECEIVER_ON_INFO,1);
		STTUNER_IOREG_SetField_SizeU32(&Instance->DeviceMap, Instance->IOHandle, FSTB0899_ONECHIPTRX,FSTB0899_ONECHIPTRX_INFO,0); 
		F22 = STTUNER_IOREG_GetField_SizeU32(&Instance->DeviceMap, Instance->IOHandle, FSTB0899_F22RX,FSTB0899_F22RX_INFO);
		F22_Freq = FE_STB0899_F22Freq(DigFreq,F22);
		if((F22_Freq <= (Instance->DiSEqC_RxFreq - 500)) || (F22_Freq >= (Instance->DiSEqC_RxFreq + 500)))
		{
			F22 = DigFreq/(32*(Instance->DiSEqC_RxFreq));
			STTUNER_IOREG_SetField_SizeU32(&Instance->DeviceMap, Instance->IOHandle, FSTB0899_F22RX, FSTB0899_F22RX_INFO,F22);
			WAIT_N_MS_899(2);
		}		
	}
	/*wait for  FIFO empty to ensure previous execution is complete*/
	STTUNER_IOREG_SetField_SizeU32( &Instance->DeviceMap, Instance->IOHandle, FSTB0899_DISEQC_RESET, FSTB0899_DISEQC_RESET_INFO,1);
	STTUNER_IOREG_SetField_SizeU32( &Instance->DeviceMap, Instance->IOHandle, FSTB0899_DISEQC_RESET, FSTB0899_DISEQC_RESET_INFO,0);
	while((STTUNER_IOREG_GetField_SizeU32( &Instance->DeviceMap, Instance->IOHandle, FSTB0899_TXFIFOBYTES,FSTB0899_TXFIFOBYTES_INFO) != 0))				  			
	{
		STB0899_Delay(50);
		if ((++TimeoutCounter)>0xfff)
				{ 
					return(ST_ERROR_TIMEOUT);
				}
		TimeoutCounter =0;
	}
	/* Reset previos FIFO contents */
	
	switch (pDiSEqCSendPacket->DiSEqCCommandType)
		
		{ 
 		 		case	STTUNER_DiSEqC_TONE_BURST_SEND_0_UNMODULATED:   /*send of 0 for 12.5 ms ;continuous tone*/
				
				/*DiSEqC(1:0)=11  DiSEqC(2)=X DiSEqC FIFO=00  */
				Error = STTUNER_IOREG_SetField_SizeU32( &Instance->DeviceMap, Instance->IOHandle, FSTB0899_DISEQCMODE, FSTB0899_DISEQCMODE_INFO,0x3);
				/* Put Tx in WAITING State */
				Error = STTUNER_IOREG_SetField_SizeU32( &Instance->DeviceMap, Instance->IOHandle, FSTB0899_DISPRECHARGE, FSTB0899_DISPRECHARGE_INFO,1);
				Error = STTUNER_IOREG_SetField_SizeU32( &Instance->DeviceMap, Instance->IOHandle, FSTB0899_DISEQCFIFO, FSTB0899_DISEQCFIFO_INFO,00);
				/* Start transmission */
				Error = STTUNER_IOREG_SetField_SizeU32( &Instance->DeviceMap, Instance->IOHandle, FSTB0899_DISPRECHARGE,FSTB0899_DISPRECHARGE_INFO, 0);
				WAIT_N_MS_899(14);/* Allowing time to transmit one byte from FIFO to DiSEqC bus to avoid any disturbance on DiSEqC bus*/
				Instance->DiSEqCConfig.Command=STTUNER_DiSEqC_TONE_BURST_SEND_0_UNMODULATED;
                		Instance->DiSEqCConfig.ToneState=STTUNER_DiSEqC_TONE_CONTINUOUS_OFF;
											
				break;

		case	STTUNER_DiSEqC_TONE_BURST_SEND_0_MODULATED:     /*0-2/3 duty cycle tone*/
				
				/*DiSEqC(1:0)=10  DiSEqC FIFO=00  */
				Error = STTUNER_IOREG_SetField_SizeU32( &Instance->DeviceMap, Instance->IOHandle, FSTB0899_DISEQCMODE,FSTB0899_DISEQCMODE_INFO, 0x2);
				/* Put Tx in WAITING State */
				Error = STTUNER_IOREG_SetField_SizeU32( &Instance->DeviceMap, Instance->IOHandle, FSTB0899_DISPRECHARGE, FSTB0899_DISPRECHARGE_INFO,1);
				Error = STTUNER_IOREG_SetField_SizeU32( &Instance->DeviceMap, Instance->IOHandle, FSTB0899_DISEQCFIFO, FSTB0899_DISEQCFIFO_INFO,00);
				/* Start transmission */
				Error = STTUNER_IOREG_SetField_SizeU32( &Instance->DeviceMap, Instance->IOHandle, FSTB0899_DISPRECHARGE, FSTB0899_DISPRECHARGE_INFO,0);
				WAIT_N_MS_899(14);/* Allowing time to transmit one byte from FIFO to DiSEqC bus to avoid any disturbance on DiSEqC bus*/
				Instance->DiSEqCConfig.Command=STTUNER_DiSEqC_TONE_BURST_SEND_0_MODULATED;
                		Instance->DiSEqCConfig.ToneState=STTUNER_DiSEqC_TONE_CONTINUOUS_OFF;
				
				
				break;

		case	STTUNER_DiSEqC_TONE_BURST_SEND_1: 				/*1-1/3 duty cycle tone modulated*/
				
				/*DiSEqC(1:0)=10  DiSEqC FIFO=FF  */
				Error = STTUNER_IOREG_SetField_SizeU32( &Instance->DeviceMap, Instance->IOHandle, FSTB0899_DISEQCMODE, FSTB0899_DISEQCMODE_INFO,0x2);
				/* Put Tx in WAITING State */
				Error = STTUNER_IOREG_SetField_SizeU32( &Instance->DeviceMap, Instance->IOHandle, FSTB0899_DISPRECHARGE,FSTB0899_DISPRECHARGE_INFO, 1);
				Error = STTUNER_IOREG_SetField_SizeU32( &Instance->DeviceMap, Instance->IOHandle, FSTB0899_DISEQCFIFO, FSTB0899_DISEQCFIFO_INFO,0xFF);
				/* Start transmission */
				Error = STTUNER_IOREG_SetField_SizeU32( &Instance->DeviceMap, Instance->IOHandle, FSTB0899_DISPRECHARGE,FSTB0899_DISPRECHARGE_INFO, 0);
				WAIT_N_MS_899(14);/* Allowing time to transmit one byte from FIFO to DiSEqC bus to avoid any disturbance on DiSEqC bus*/
				Instance->DiSEqCConfig.Command=STTUNER_DiSEqC_TONE_BURST_SEND_1;
                		Instance->DiSEqCConfig.ToneState=STTUNER_DiSEqC_TONE_CONTINUOUS_OFF;
				
				
				break;

		case	STTUNER_DiSEqC_COMMAND:							/*DiSEqC (2/3)command */
				
				/*DiSEqC(1:0)=10  DiSEqC FIFO=data *//* USE 10 for 2/3 PWM DiSEqC signal mode EXTM pin of LNBH21 is used */
				Error = STTUNER_IOREG_SetField_SizeU32( &Instance->DeviceMap, Instance->IOHandle, FSTB0899_DISEQCMODE, FSTB0899_DISEQCMODE_INFO,2);
				/* Put Tx in WAITING State */
				Error = STTUNER_IOREG_SetField_SizeU32( &Instance->DeviceMap, Instance->IOHandle, FSTB0899_DISPRECHARGE, FSTB0899_DISPRECHARGE_INFO,1);
				
				for(No_of_Bytes = pDiSEqCSendPacket->uc_TotalNoOfBytes; No_of_Bytes>0; No_of_Bytes--)
					{
						Error = STTUNER_IOREG_SetRegister( &Instance->DeviceMap, Instance->IOHandle, RSTB0899_DISFIFO, *pCurrent_Data);
						
						pCurrent_Data++;
				
					}
				Error = STTUNER_IOREG_SetField_SizeU32( &Instance->DeviceMap, Instance->IOHandle, FSTB0899_DISPRECHARGE,FSTB0899_DISPRECHARGE_INFO, 0);
				Instance->DiSEqCConfig.Command = STTUNER_DiSEqC_COMMAND;
                Instance->DiSEqCConfig.ToneState = STTUNER_DiSEqC_TONE_CONTINUOUS_OFF;
                /* Make Delay value to 135 */
                Delay = 140;
                /* Wait for the DiSEqC command to be sent completely --(TicksPermilisec *13.5* Numberofbytes sent)*/
				WAIT_N_MS_899((Delay*pDiSEqCSendPacket->uc_TotalNoOfBytes)/10);
				
				break;
		
		default:
				Error=ST_ERROR_BAD_PARAMETER;
	        break;
	
		}/*end of Switch*/														
	
	if (Error !=ST_NO_ERROR)
		{	
			return(Error);
		}
	
	
	if(Instance->DiSEqC_RxFreq !=0 )
	{
		
		while((STTUNER_IOREG_GetField_SizeU32(&Instance->DeviceMap, Instance->IOHandle,FSTB0899_RXEND, FSTB0899_RXEND_INFO)!=1) && (i<10))
		{
			WAIT_N_MS_899(10); 
			i++;
		}
		i=0;
		while(STTUNER_IOREG_GetField_SizeU32(&Instance->DeviceMap, Instance->IOHandle,FSTB0899_RXEND, FSTB0899_RXEND_INFO) && (i<10))
		{
			WAIT_N_MS_899(10); 
			i++;
		}
			
		pDiSEqCResponsePacket->uc_TotalBytesReceived = STTUNER_IOREG_GetField_SizeU32(&Instance->DeviceMap, Instance->IOHandle, FSTB0899_FIFOBYTENBR, FSTB0899_FIFOBYTENBR_INFO);
		for(i=0;i<(pDiSEqCResponsePacket->uc_TotalBytesReceived);i++)
		pDiSEqCResponsePacket->ReceivedFrmAddData[i]=STTUNER_IOREG_GetRegister(&Instance->DeviceMap, Instance->IOHandle, RSTB0899_DISFIFO);
		STTUNER_IOREG_SetField_SizeU32( &Instance->DeviceMap, Instance->IOHandle, FSTB0899_DISEQC_RESET, FSTB0899_DISEQC_RESET_INFO,1);
		STTUNER_IOREG_SetField_SizeU32( &Instance->DeviceMap, Instance->IOHandle, FSTB0899_DISEQC_RESET,FSTB0899_DISEQC_RESET_INFO, 0);
		
	}		
 
#ifdef STTUNER_DISEQC2_SWDECODE_VIA_PIO
 

#if defined (STTUNER_DRV_SAT_LNBH21) || defined (STTUNER_DRV_SAT_LNB21)
	Error = (Inst[Instance->TopLevelHandle].Sat.Lnb.Driver->lnb_setttxmode)(LHandle,STTUNER_LNB_RX);
	if (Error != ST_NO_ERROR)
	{
		return(Error);		
	}
#endif	    
        /* make the pointer to the first location of the data array */
        pCurrent_Data -=pDiSEqCSendPacket->uc_TotalNoOfBytes; 
       /* Now check whether reply is required */
       if ((*pCurrent_Data == FB_COMMAND_REPLY)|
			(*(pDiSEqCSendPacket->pFrmAddCmdData) == FB_COMMAND_REPLY_REPEATED))
		{
		
		 /*Clear the counter before starting the interrupt */
		
		 InstforISR[IndexforISR].Sat.DiSEqC.PulseCounter = 0;
		 InstforISR[IndexforISR].Sat.DiSEqC.IgnoreFirstIntr =0; /* Reset the PIO interrupt workaround flag */
		 InstforISR[IndexforISR].Sat.DiSEqC.setCmpReg.ComparePattern =1;
		 InstforISR[IndexforISR].Sat.DiSEqC.FirstBitRcvdFlag = 0;
		 InstforISR[IndexforISR].Sat.DiSEqC.ReplyContinueFlag =0;
		 InstforISR[IndexforISR].Sat.DiSEqC.setCmpReg.CompareEnable = InstforISR[IndexforISR].Sat.DiSEqC.PIOPin;
		 Error = STPIO_SetCompare(InstforISR[IndexforISR].Sat.DiSEqC.HandlePIO, &InstforISR[IndexforISR].Sat.DiSEqC.setCmpReg);
		 
		 i=0;
		 /* Wait 150ms for the first bit of the reply to come */
		 while(i++ < 16 && InstforISR[IndexforISR].Sat.DiSEqC.FirstBitRcvdFlag == 0 )
		 {
		   WAIT_N_MS_899(10);
		 }
	  			
		if (i >= 16) /* If Interrupt is not called before time out is reached then return tmeout error*/
		{
			return STTUNER_ERROR_DISEQC_TIMEOUT;
		}
	

           /* Wait for the reply to stop */
           while(DelayCount < 200)
           {
           	if(InstforISR[IndexforISR].Sat.DiSEqC.ReplyContinueFlag == 0)
           	{
           		break; /* If reply is not coming then come out of the loop*/
           	}
           	else
           	{
           	 InstforISR[IndexforISR].Sat.DiSEqC.ReplyContinueFlag =0;
           	}
           	WAIT_N_MS_899(3); 
           	DelayCount +=3;
           }
	              
	       /* Now Disable interrupt */
	       InstforISR[IndexforISR].Sat.DiSEqC.setCmpReg.CompareEnable = 0x00;
	       Error = STPIO_SetCompare(InstforISR[IndexforISR].Sat.DiSEqC.HandlePIO, &InstforISR[IndexforISR].Sat.DiSEqC.setCmpReg);
	       
	       /* Clear the BitPattern[] array */
	       memset(BitPattern,0x00,MAX_NUM_BITS_IN_REPLY);
	       
	       /* Check whether any response is captured or not */
	       if (InstforISR[IndexforISR].Sat.DiSEqC.PulseCounter == 0)
	       {
	         return STTUNER_ERROR_DISEQC_TIMEOUT; /* if no reply is received
	         				 then return timeout error*/
	       }
	       /* Check whether the number of bits received is multile of 9 or not */
	     	if (InstforISR[IndexforISR].Sat.DiSEqC.PulseCounter%9 != 0)
	     	{
	     	 return STTUNER_ERROR_DISEQC_CORRUPTED_REPLY;
	        } 
	       /* Now decode the bit pattern */
	       for(i=0;i<InstforISR[IndexforISR].Sat.DiSEqC.PulseCounter;i++)
	       {
	       	if (InstforISR[IndexforISR].Sat.DiSEqC.PulseWidth[i] >=6 && InstforISR[IndexforISR].Sat.DiSEqC.PulseWidth[i] <=8 )
	       	{
	       		BitPattern[i] = 1;	
	       	}
	       	else if (InstforISR[IndexforISR].Sat.DiSEqC.PulseWidth[i] >=13 && InstforISR[IndexforISR].Sat.DiSEqC.PulseWidth[i] <=15)
	       	{
	       		BitPattern[i] = 0;
	       	}
	       	else
	       	{
	       	  /* For different pulse width return Corrupted DiSEqC reply error */
	       	  return STTUNER_ERROR_DISEQC_CORRUPTED_REPLY;
	       	}
	}
	
	
	     
	     /* Now check for Parity and make bytes to fill in response byte pointer */
	     NumofBytes = InstforISR[IndexforISR].Sat.DiSEqC.PulseCounter/9;
	     for (i=0;i<NumofBytes;i++)
	     {
	     	DecodedOneByte = 0;
	     	ParityCounter =0;
	     	for(j=0;j<8;j++)
	     	{
	     		if (BitPattern[i*9+j] == 1)
	     		{
	     			ParityCounter++;
	     		}
	     	}
	     	if (ParityCounter%2 == 0) /* If number of 1 in the byte is even then 9th bit 
	     				should be 1 */
	     	 {      
	     	 	if (BitPattern[i*9+j] != 1)
	     	 	{
	     	 	 return STTUNER_ERROR_DISEQC_PARITY;
	     	 	}  
	     	 }   
	     	 else	/* If number of 1 in the byte is odd then 9th bit 
	     				should be 0 */
	     	 {
	     	 	if (BitPattern[i*9+j] != 0)
	     	 	{
	     	 	  return STTUNER_ERROR_DISEQC_PARITY;
	     	 	} 
	     	  } 
	     	  /*Now form the bits into bytes */
	     	  for(j=0;j<8;j++)
	          {
	     		DecodedOneByte = (BitPattern[i*9+7-j]<< j)|DecodedOneByte;
	     	  }
	     	    pDiSEqCResponsePacket->ReceivedFrmAddData[i] =   DecodedOneByte;            
	     	               
	     	}
	     	/* Update the number of byte received field of the response byte structure*/
	     	pDiSEqCResponsePacket->uc_TotalBytesReceived = NumofBytes;
	     	
		
	}
	
#endif 
	
	/* wait for 'pDiSEqCSendPacket->uc_msecBeforeNextCommand' millisec. */
	/*WAIT_N_MS_899(pDiSEqCSendPacket->uc_msecBeforeNextCommand); */

	return(Error);
}

#ifdef STTUNER_DISEQC2_SWDECODE_VIA_PIO
/* ----------------------------------------------------------------------------
Name: CaptureReplyPulse1()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
void CaptureReplyPulse1(STPIO_Handle_t Handle, STPIO_BitMask_t ActiveBits)	
{
	if (InstforISR[IndexforISR].Sat.DiSEqC.IgnoreFirstIntr == 0)
	{
		InstforISR[IndexforISR].Sat.DiSEqC.IgnoreFirstIntr =1;
	}
	else
	{
		
	if(InstforISR[IndexforISR].Sat.DiSEqC.setCmpReg.ComparePattern ==1)
	{
	   InstforISR[IndexforISR].Sat.DiSEqC.FallingTime [InstforISR[IndexforISR].Sat.DiSEqC.PulseCounter] = STOS_time_now();
	}
	 else
	 {
	    InstforISR[IndexforISR].Sat.DiSEqC.PulseWidth[InstforISR[IndexforISR].Sat.DiSEqC.PulseCounter] = STOS_time_minus(STOS_time_now(),InstforISR[IndexforISR].Sat.DiSEqC.FallingTime [InstforISR[IndexforISR].Sat.DiSEqC.PulseCounter]);
	    InstforISR[IndexforISR].Sat.DiSEqC.PulseCounter++;
	     
	 }
	 
	InstforISR[IndexforISR].Sat.DiSEqC.setCmpReg.ComparePattern = (~(InstforISR[IndexforISR].Sat.DiSEqC.setCmpReg.ComparePattern)& 0x01);
	/* set FirstBitRcvdFlag to 1  */
	if (InstforISR[IndexforISR].Sat.DiSEqC.PulseCounter == 1)
	{
	InstforISR[IndexforISR].Sat.DiSEqC.FirstBitRcvdFlag=1;
	}
	/* Set ReplyContinueFlag to 1 */
	InstforISR[IndexforISR].Sat.DiSEqC.ReplyContinueFlag=1;	
	}
	
}
 
 
/* ----------------------------------------------------------------------------
Name: CaptureReplyPulse2()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
void CaptureReplyPulse2(STPIO_Handle_t Handle, STPIO_BitMask_t ActiveBits)	
{
	if (InstforISR[IndexforISR].Sat.DiSEqC.IgnoreFirstIntr == 0)
	{
		InstforISR[IndexforISR].Sat.DiSEqC.IgnoreFirstIntr =1;
	}
	else
	{
		
	if(InstforISR[IndexforISR].Sat.DiSEqC.setCmpReg.ComparePattern ==1)
	{
	   InstforISR[IndexforISR].Sat.DiSEqC.FallingTime [InstforISR[IndexforISR].Sat.DiSEqC.PulseCounter] = STOS_time_now();
	}
	 else
	 {
	    InstforISR[IndexforISR].Sat.DiSEqC.PulseWidth[InstforISR[IndexforISR].Sat.DiSEqC.PulseCounter] = STOS_time_minus(STOS_time_now(),InstforISR[IndexforISR].Sat.DiSEqC.FallingTime [InstforISR[IndexforISR].Sat.DiSEqC.PulseCounter]);
	    InstforISR[IndexforISR].Sat.DiSEqC.PulseCounter++;
	     
	 }
	 
	InstforISR[IndexforISR].Sat.DiSEqC.setCmpReg.ComparePattern = (~(InstforISR[IndexforISR].Sat.DiSEqC.setCmpReg.ComparePattern)& 0x01);
	/* set FirstBitRcvdFlag to 1  */
	if (InstforISR[IndexforISR].Sat.DiSEqC.PulseCounter == 1)
	{
	InstforISR[IndexforISR].Sat.DiSEqC.FirstBitRcvdFlag=1;
	}
	/* Set ReplyContinueFlag to 1 */
	InstforISR[IndexforISR].Sat.DiSEqC.ReplyContinueFlag=1;	
	}
	
}

#endif 

/* ----------------------------------------------------------------------------
Name: demod_stb0899_DiSEqCGetConfig()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_stb0899_DiSEqCGetConfig(DEMOD_Handle_t Handle,STTUNER_DiSEqCConfig_t *DiSEqCConfig)
{

    ST_ErrorCode_t Error = ST_NO_ERROR;
    STB0899_InstanceData_t *Instance;

    Instance = STB0899_GetInstFromHandle(Handle);

    DiSEqCConfig->Command=Instance->DiSEqCConfig.Command;
    DiSEqCConfig->ToneState=Instance->DiSEqCConfig.ToneState;
    
    return(Error);
}  

/* ----------------------------------------------------------------------------
Name: demod_stb0899_DiSEqCBurstOFF()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_stb0899_DiSEqCBurstOFF ( DEMOD_Handle_t Handle )
{
   ST_ErrorCode_t Error = ST_NO_ERROR;
  /* D0299_InstanceData_t *Instance;*/
   STTUNER_DiSEqCSendPacket_t DiSEqCSendPacket;
   void *pDiSEqCResponsePacket=NULL;
   
   DiSEqCSendPacket.DiSEqCCommandType = STTUNER_DiSEqC_TONE_BURST_OFF;

   DiSEqCSendPacket.uc_msecBeforeNextCommand =0;
   Error=demod_stb0899_DiSEqC(Handle,&DiSEqCSendPacket,pDiSEqCResponsePacket);

   return(Error);
}

/* ----------------------------------------------------------------------------
Name: demod_stb0899_ioctl()

Description:
    access device specific low-level functions
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_stb0899_ioctl(DEMOD_Handle_t Handle, U32 Function, void *InParams, void *OutParams, STTUNER_Da_Status_t *Status)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
   const char *identity = "STTUNER d0399.c demod_d0399_ioctl()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STB0899_InstanceData_t *Instance;

    Instance = STB0899_GetInstFromHandle(Handle);

    if(STTUNER_Util_CheckPtrNull(Instance) != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_STB0899
        STTBX_Print(("%s invalid handle\n", identity));
#endif
        return(ST_ERROR_INVALID_HANDLE);
    }

#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
        STTBX_Print(("%s demod driver instance handle=%d\n", identity, Handle));
        STTBX_Print(("%s                     function=%d\n", identity, Function));
        STTBX_Print(("%s                   I/O handle=%d\n", identity, Instance->IOHandle));
#endif

    /* ---------- select what to do ---------- */
    switch(Function){

        case STTUNER_IOCTL_RAWIO: /* read/write device register (actual write to stb0899) */
            Error =  STTUNER_IOARCH_ReadWrite( Instance->IOHandle, 
                                                 ((SATIOCTL_IOARCH_Params_t *)InParams)->Operation,
                                                 ((SATIOCTL_IOARCH_Params_t *)InParams)->SubAddr,
                                                 ((SATIOCTL_IOARCH_Params_t *)InParams)->Data,
                                                 ((SATIOCTL_IOARCH_Params_t *)InParams)->TransferSize,
                                                 ((SATIOCTL_IOARCH_Params_t *)InParams)->Timeout );
            break;

        case STTUNER_IOCTL_REG_IN: /* read device register */
            *(int *)OutParams = STTUNER_IOREG_GetRegister( &Instance->DeviceMap, Instance->IOHandle, *(int *)InParams);
            break;

        case STTUNER_IOCTL_REG_OUT: /* write device register. NOTE: RegIndex is <<INDEX>> TO register number. */
            Error =  STTUNER_IOREG_SetRegister( &Instance->DeviceMap, Instance->IOHandle,
                  ((SATIOCTL_SETREG_InParams_t *)InParams)->RegIndex,
                  ((SATIOCTL_SETREG_InParams_t *)InParams)->Value );
            break;

     
        case STTUNER_IOCTL_IQMODE: /* Get the IQ mode */  /* DS Modification to avoid NOT supported error !!!!! */

            if(STTUNER_IOREG_GetField_SizeU32( &Instance->DeviceMap, Instance->IOHandle,  FSTB0899_SYM,FSTB0899_SYM_INFO))
                *((STTUNER_IQMode_t *)OutParams)=STTUNER_IQ_MODE_INVERTED;
            else
                *((STTUNER_IQMode_t *)OutParams)=STTUNER_IQ_MODE_NORMAL;
            break;
           
           
        default:
#ifdef STTUNER_DEBUG_MODULE_SATDRV_STB0899
            STTBX_Print(("%s function %d not found\n", identity, Function));
#endif
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }


#ifdef STTUNER_DEBUG_MODULE_SATDRV_STB0899
    STTBX_Print(("%s called ok\n", identity, Function));    /* signal that function came back */
#endif

    return(Error);

}



/* ----------------------------------------------------------------------------
Name: demod_stb0899_ioaccess()

Description:
    called from ioarch.c when some driver does I/O but with the repeater/passthru
    set to point to this function.
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_stb0899_ioaccess(DEMOD_Handle_t Handle, IOARCH_Handle_t IOHandle, STTUNER_IOARCH_Operation_t Operation, U16 SubAddr, U8 *Data, U32 TransferSize, U32 Timeout)

{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
   const char *identity = "STTUNER d0399.c demod_d0399_ioaccess()";
#endif
    
    ST_ErrorCode_t Error = ST_NO_ERROR;
   
    IOARCH_Handle_t ThisIOHandle;
    STB0899_InstanceData_t *Instance;

    Instance = STB0899_GetInstFromHandle(Handle);

    if(STTUNER_Util_CheckPtrNull(Instance) != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0899
        STTBX_Print(("%s invalid handle\n", identity));
#endif
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* this (demod) drivers I/O handle */
    ThisIOHandle = Instance->IOHandle;

    /* if STTUNER_IOARCH_MAX_HANDLES then passthrough function required using our device handle/address */ 
    if (IOHandle == STTUNER_IOARCH_MAX_HANDLES)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0899
        STTBX_Print(("%s write passthru\n", identity));
#endif
        Error = STTUNER_IOARCH_ReadWrite(ThisIOHandle, Operation, SubAddr, Data, TransferSize, Timeout);
    }
    else    /* repeater */
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0899
        STTBX_Print(("%s write repeater\n", identity));
#endif
        /* enable repeater then send the data  using I/O handle supplied through ioarch.c */
        Error = STTUNER_IOREG_SetField_SizeU32( &(Instance->DeviceMap),Instance->IOHandle, FSTB0899_I2CTON, FSTB0899_I2CTON_INFO,1);

        /* send callers data using their address. (nb: subaddress == 0)
         Function 'STTUNER_IOARCH_ReadWriteNoRep' is called as calling the normal 'STTUNER_IOARCH_ReadWrite'
        function would cause it to call the redirection function which is THIS function, and around we 
        would go forever. */
        Error |= STTUNER_IOARCH_ReadWriteNoRep(IOHandle, Operation, SubAddr, Data, TransferSize, Timeout);
        Error |= STTUNER_IOREG_SetField_SizeU32( &(Instance->DeviceMap),Instance->IOHandle, FSTB0899_I2CTON, FSTB0899_I2CTON_INFO,0);
    
    }

    return(Error);
    
}
/* ----------------------------------------------------------------------------
Name: demod_stb0899_Tonedetection()

Description:
    called from scrdrv.c by a fn pointer to detect the tone frequency generated by SatCR LNB.
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_stb0899_Tonedetection(DEMOD_Handle_t Handle,U32 StartFreq, U32 StopFreq, U8  *NbTones,U32 *ToneList, U8 mode, int* power_detection_level)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    
    STB0899_InstanceData_t *Instance;
    Instance = STB0899_GetInstFromHandle(Handle);

    if(STTUNER_Util_CheckPtrNull(Instance) != ST_NO_ERROR)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }       
    *NbTones = FE_STB0899_ToneDetection(&(Instance->DeviceMap),Instance->TopLevelHandle,StartFreq,StopFreq,ToneList, mode);
    
    return(Error);
    
}   

/* ------------------------------------------------------------------------- */
/* /\/\/\/\/\/\/\/\/\/\/\/\/\UTILITY Functions/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ */
/* ------------------------------------------------------------------------- */

STB0899_InstanceData_t *STB0899_GetInstFromHandle(DEMOD_Handle_t Handle)
{
    return( (STB0899_InstanceData_t *)Handle );
}


/* End of d_0899.c */
