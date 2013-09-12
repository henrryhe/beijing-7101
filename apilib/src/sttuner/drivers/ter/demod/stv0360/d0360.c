/* ----------------------------------------------------------------------------
File Name: d0360.c

Description:

    stv0360 demod driver.


Copyright (C) 1999-2001 STMicroelectronics

History:

   date: 12-Sept-2001
version: 3.1.0
 author: GB from work by GJP
comment: rewrite for terrestrial.

Reference:

    ST API Definition "TUNER Driver API" DVD-API-06
---------------------------------------------------------------------------- */


/* Includes ---------------------------------------------------------------- */
#ifndef ST_OSLINUX
   
/* C libs */
#include <string.h>
#endif
#include "stlite.h"     /* Standard includes */

/* STAPI */
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
#include "360_echo.h"

#include "360_drv.h"    /* misc driver functions */
#include "d0360.h"      /* header for this file */
#include "360_map.h"

#include "tioctl.h"     /* data structure typedefs for all the ter ioctl functions */
#include "mdemod.h"



 
 
 U16 STV0360_Address[STV360_NBREGS]={
 /*R_ID*/             0x00, 
/*R_I2CRPT*/         0x01, 
/*R_TOPCTRL*/        0x02 ,
/*R_IOCFG0*/         0x03, 
/*R_DAC0R*/          0x04, 

/*R_IOCFG1*/         0x05, 
/*R_DAC1R*/          0x06, 
/*R_IOCFG2*/         0x07, 
/*R_PWMFR*/          0x08,
/*R_STATUS*/         0x09,

/*R_AUX_CLK*/        0x0a,
/*R_FREESYS1*/       0x0b,
/*R_FREESYS2*/       0x0c,
/*R_FREESYS3*/       0x0d,
/*R_AGC2MAX*/        0x10, 

/*R_AGC2MIN*/        0x11, 
/*R_AGC1MAX*/        0x12, 
/*R_AGC1MIN*/        0x13, 
/*R_AGCR*/           0x14, 
/*R_AGC2TH*/         0x15, 

/*R_AGC12C3*/        0x16,
/*R_AGCCTRL1*/       0x17, 
/*R_AGCCTRL2*/       0x18,
/*R_AGC1VAL1*/       0x19,
/*R_AGC1VAL2*/       0x1a,

/*R_AGC2VAL1*/       0x1b, 
/*R_AGC2VAL2*/       0x1c, 
/*R_AGC2PGA*/        0x1d, 
/*R_OVF_RATE1*/      0x1e, 
/*R_OVF_RATE2*/      0x1f, 

/*R_GAIN_SRC1*/      0x20, 
/*R_GAIN_SRC2*/      0x21,
/*R_INC_DEROT1*/     0x22,
/*R_INC_DEROT2*/     0x23,
/*R_FREESTFE_1*/     0x26,

/*R_SYR_THR*/        0x27,
/*R_INR*/            0x28, 
/*R_EN_PROCESS*/     0x29, 
/*R_SDI_SMOOTHER*/   0x2A, 
/*R_FE_LOOP_OPEN*/   0x2B, 

/*R_EPQ*/            0x31, 
/*R_EPQ2*/           0x32,
/*R_COR_CTL*/        0x80,
/*R_COR_STAT*/       0x81,
/*R_COR_INTEN*/      0x82,

/*R_COR_INTSTAT*/    0x83,
/*R_COR_MODEGUARD*/  0x84,
/*R_AGC_CTL*/        0x85,
/*R_AGC_MANUAL1*/    0x86,
/*R_AGC_MANUAL2*/    0x87,

/*R_AGC_TARGET*/     0x88,
/*R_AGC_GAIN1*/      0x89,
/*R_AGC_GAIN2*/      0x8a,
/*R_ITB_CTL*/        0x8b,
/*R_ITB_FREQ1*/      0x8c,

/*R_ITB_FREQ2*/      0x8d,
/*R_CAS_CTL*/        0x8e,
/*R_CAS_FREQ*/       0x8f,
/*R_CAS_DAGCGAIN*/   0x90,
/*R_SYR_CTL*/        0x91,

/*R_SYR_STAT*/       0x92,
/*R_SYR_NC01*/       0x93,
/*R_SYR_NC02*/       0x94,
/*R_SYR_OFFSET1*/    0x95,
/*R_SYR_OFFSET2*/    0x96,

/*R_FFT_CTL*/        0x97,
/*R_SCR_CTL*/        0x98,
/*R_PPM_CTL1*/       0x99,
/*R_TRL_CTL*/        0x9a,
/*R_TRL_NOMRATE1*/   0x9b,

/*R_TRL_NOMRATE2*/   0x9c,
/*R_TRL_TIME1*/      0x9d,
/*R_TRL_TIME2*/      0x9e,
/*R_CRL_CTL*/        0x9f,
/*R_CRL_FREQ1*/      0xa0,

/*R_CRL_FREQ2*/      0xa1,
/*R_CRL_FREQ3*/      0xa2,
/*R_CHC_CTL1*/       0xa3,
/*R_CHC_SNR*/        0xa4,
/*R_BDI_CTL*/        0xa5,

/*R_DMP_CTL*/        0xa6,
/*R_TPS_RCVD1*/      0xa7,
/*R_TPS_RCVD2*/      0xa8,
/*R_TPS_RCVD3*/      0xa9,
/*R_TPS_RCVD4*/      0xaa,

/*R_TPS_CELLID*/     0xab,
/*R_TPS_FREE2*/      0xac,
/*R_TPS_SET1*/       0xad,
/*R_TPS_SET2*/       0xae,
/*R_TPS_SET3*/       0xaf,

/*R_TPS_CTL*/        0xb0,
/*R_CTL_FFTOSNUM*/   0xb1,
/*R_CAR_DISP_SEL*/   0xb2,
/*R_MSC_REV*/        0xb3,
/*R_PIR_CTL*/        0xb4,

/*R_SNR_CARRIER1*/   0xb5,
/*R_SNR_CARRIER2*/   0xb6,
/*R_PPM_CPAMP*/      0xb7,
/*R_TSM_AP0*/        0xb8,
/*R_TSM_AP1*/        0xb9,

/*R_TSM_AP2*/        0xba,
/*R_TSM_AP3*/        0xbb,
/*R_TSM_AP4*/        0xbc,
/*R_TSM_AP5*/        0xbd,
/*R_TSM_AP6*/        0xbe,

/*R_TSM_AP7*/        0xbf,
/*R_CONSTMODE*/      0xcb,
/*R_CONSTCARR1*/     0xcc,
/*R_CONSTCARR2*/     0xcd,
/*R_ICONSTEL*/       0xce,

/*R_QCONSTEL*/       0xcf,
/*R_AGC1RF*/         0xD4,
/*R_EN_RF_AGC1*/     0xD5,
/*R_FECM*/           0x40,
/*R_VTH0*/           0x41,

/*R_VTH1*/           0x42,
/*R_VTH2*/           0x43,
/*R_VTH3*/           0x44,
/*R_VTH4*/           0x45,
/*R_VTH5*/           0x46,

/*R_FREEVIT*/        0x47,
/*R_VITPROG*/        0x49,
/*R_PR*/             0x4a,
/*R_VSEARCH*/        0x4b,
/*R_RS*/             0x4c,

/*R_RSOUT*/          0x4d,
/*R_ERRCTRL1*/       0x4e,
/*R_ERRCNTM1*/       0x4f,
/*R_ERRCNTL1*/       0x50,
/*R_ERRCTRL2*/       0x51,

/*R_ERRCNTM2*/       0x52,
/*R_ERRCNTL2*/       0x53,
/*R_ERRCTRL3*/       0x56,
/*R_ERRCNTM3*/       0x57,
/*R_ERRCNTL3*/       0x58,

/*R_DILSTK1*/        0x59,
/*R_DILSTK0*/        0x5A,
/*R_DILBWSTK1*/      0x5B,
/*R_DILBWST0*/       0x5C,
/*R_LNBRX*/          0x5D,

/*R_RSTC*/           0x5E,
/*R_VIT_BIST*/       0x5F,
/*R_FREEDRS*/        0x54,
/*R_VERROR*/         0x55,
/*R_TSTRES*/         0xc0,

/*R_ANACTRL*/        0xc1,
/*R_TSTBUS*/         0xc2,
/*R_TSTCK*/          0xc3,
/*R_TSTI2C*/         0xc4,
/*R_TSTRAM1*/        0xc5,

/*R_TSTRATE*/        0xc6,
/*R_SELOUT*/         0xc7,
/*R_FORCEIN*/        0xc8,
/*R_TSTFIFO*/        0xc9,
/*R_TSTRS*/          0xca,

/*R_TSTBISTRES0*/    0xd0,
/*R_TSTBISTRES1*/    0xd1,
/*R_TSTBISTRES2*/    0xd2,
/*R_TSTBISTRES3*/    0xd3

	        };/** end of array**/
 
 
/*******Never change the order of this array elements as it is accordance with******
******360_init.c settings of register.*********************/
/****** Register settings for DemodSTV0360*************/
#ifdef STTUNER_TERR_TMM_6MHZ
U8 Def360Val[STV360_NBREGS]={
/*R_ID*/             0x21, 
/*R_I2CRPT*/         0x27, 
/*R_TOPCTRL*/        0x2 ,
/*R_IOCFG0*/         0x0, 
/*R_DAC0R*/          0x00, 

/*R_IOCFG1*/         0x00, 
/*R_DAC1R*/          0x00, 
/*R_IOCFG2*/         0x70, 
/*R_PWMFR*/          0x00,
/*R_STATUS*/         0xf9,

/*R_AUX_CLK*/        0x1c,
/*R_FREESYS1*/       0x00,
/*R_FREESYS2*/       0x00,
/*R_FREESYS3*/       0x00,
/*R_AGC2MAX*/        0xff, 

/*R_AGC2MIN*/        0x0, 
/*R_AGC1MAX*/        0x0, 
/*R_AGC1MIN*/        0x0, 
/*R_AGCR*/           0x0, 
/*R_AGC2TH*/         0x0, 

/*R_AGC12C3*/        0x00,
/*R_AGCCTRL1*/       0x85, 
/*R_AGCCTRL2*/       0x7,
/*R_AGC1VAL1*/       0xf,
/*R_AGC1VAL2*/       0x0,

/*R_AGC2VAL1*/       0xfc, 
/*R_AGC2VAL2*/       0x07, 
/*R_AGC2PGA*/        0x00, 
/*R_OVF_RATE1*/      0x00, 
/*R_OVF_RATE2*/      0x00, 

/*R_GAIN_SRC1*/      0xca, 
/*R_GAIN_SRC2*/      0x8c,
/*R_INC_DEROT1*/     0x54,
/*R_INC_DEROT2*/     0xbb,
/*R_FREESTFE_1*/     0x03,

/*R_SYR_THR*/        0x1c,
/*R_INR*/            0xff, 
/*R_EN_PROCESS*/     0x1, 
/*R_SDI_SMOOTHER*/   0xff, 
/*R_FE_LOOP_OPEN*/   0x00, 

/*R_EPQ*/            0x12, 
/*R_EPQ2*/           0xd,
/*R_COR_CTL*/        0x20,
/*R_COR_STAT*/       0xf6,
/*R_COR_INTEN*/      0x00,

/*R_COR_INTSTAT*/    0x3f,
/*R_COR_MODEGUARD*/  0x3,
/*R_AGC_CTL*/        0x18,
/*R_AGC_MANUAL1*/    0x00,
/*R_AGC_MANUAL2*/    0x00,

/*R_AGC_TARGET*/     0x20,
/*R_AGC_GAIN1*/      0xf9,
/*R_AGC_GAIN2*/      0x1f,
/*R_ITB_CTL*/        0x00,
/*R_ITB_FREQ1*/      0x00,

/*R_ITB_FREQ2*/      0x00,
/*R_CAS_CTL*/        0x85,
/*R_CAS_FREQ*/       0xB3,
/*R_CAS_DAGCGAIN*/   0xf,
/*R_SYR_CTL*/        0x00,

/*R_SYR_STAT*/       0x17,
/*R_SYR_NC01*/       0x00,
/*R_SYR_NC02*/       0x00,
/*R_SYR_OFFSET1*/    0x01,
/*R_SYR_OFFSET2*/    0x00,

/*R_FFT_CTL*/        0x00,
/*R_SCR_CTL*/        0x00,
/*R_PPM_CTL1*/       0x30,
/*R_TRL_CTL*/        0x14,
/*R_TRL_NOMRATE1*/   0x4,

/*R_TRL_NOMRATE2*/   0x41,
/*R_TRL_TIME1*/      0x7c,
/*R_TRL_TIME2*/      0xfc,
/*R_CRL_CTL*/        0x4F,
/*R_CRL_FREQ1*/      0xfe,

/*R_CRL_FREQ2*/      0xb4,
/*R_CRL_FREQ3*/      0x2,
/*R_CHC_CTL1*/       0xb1,
/*R_CHC_SNR*/        0xdb,
/*R_BDI_CTL*/        0x00,

/*R_DMP_CTL*/        0x00,
/*R_TPS_RCVD1*/      0x33,
/*R_TPS_RCVD2*/      0x2,
/*R_TPS_RCVD3*/      0x01,
/*R_TPS_RCVD4*/      0x31,

/*R_TPS_CELLID*/     0x00,
/*R_TPS_FREE2*/      0x00,
/*R_TPS_SET1*/       0x01,
/*R_TPS_SET2*/       0x02,
/*R_TPS_SET3*/       0x4,

/*R_TPS_CTL*/        0x00,
/*R_CTL_FFTOSNUM*/   0x2b,
/*R_CAR_DISP_SEL*/   0x0C,
/*R_MSC_REV*/        0x0A,
/*R_PIR_CTL*/        0x00,

/*R_SNR_CARRIER1*/   0xA8,
/*R_SNR_CARRIER2*/   0x86,
/*R_PPM_CPAMP*/      0xc1,
/*R_TSM_AP0*/        0x00,
/*R_TSM_AP1*/        0x00,

/*R_TSM_AP2*/        0x00,
/*R_TSM_AP3*/        0x00,
/*R_TSM_AP4*/        0x00,
/*R_TSM_AP5*/        0x00,
/*R_TSM_AP6*/        0x00,

/*R_TSM_AP7*/        0x00,
/*R_CONSTMODE*/      0x02,
/*R_CONSTCARR1*/     0xD2,
/*R_CONSTCARR2*/     0x04,
/*R_ICONSTEL*/       0x17,

/*R_QCONSTEL*/       0x18,
/*R_AGC1RF*/         0xff,
/*R_EN_RF_AGC1*/     0x83,
/*R_FECM*/           0x00,
/*R_VTH0*/           0x1E,

/*R_VTH1*/           0x1e,
/*R_VTH2*/           0x0F,
/*R_VTH3*/           0x09,
/*R_VTH4*/           0x00,
/*R_VTH5*/           0x05,

/*R_FREEVIT*/        0x00,
/*R_VITPROG*/        0x92,
/*R_PR*/             0x2,
/*R_VSEARCH*/        0xb0,
/*R_RS*/             0xbc,

/*R_RSOUT*/          0x15,
/*R_ERRCTRL1*/       0x12,
/*R_ERRCNTM1*/       0x00,
/*R_ERRCNTL1*/       0x00,
/*R_ERRCTRL2*/       0xb2,

/*R_ERRCNTM2*/       0x00,
/*R_ERRCNTL2*/       0x00,
/*R_ERRCTRL3*/       0x12,
/*R_ERRCNTM3*/       0x00,
/*R_ERRCNTL3*/       0x00,

/*R_DILSTK1*/        0x00,
/*R_DILSTK0*/        0x03,
/*R_DILBWSTK1*/      0x00,
/*R_DILBWST0*/       0x03,
/*R_LNBRX*/          0x80,

/*R_RSTC*/           0xB0,
/*R_VIT_BIST*/       0x07,
/*R_FREEDRS*/        0x00,
/*R_VERROR*/         0x00,
/*R_TSTRES*/         0x00,

/*R_ANACTRL*/        0x00,
/*R_TSTBUS*/         0x00,
/*R_TSTCK*/          0x00,
/*R_TSTI2C*/         0x00,
/*R_TSTRAM1*/        0x00,

/*R_TSTRATE*/        0x00,
/*R_SELOUT*/         0x00,
/*R_FORCEIN*/        0x00,
/*R_TSTFIFO*/        0x00,
/*R_TSTRS*/          0x00,

/*R_TSTBISTRES0*/    0x00,
/*R_TSTBISTRES1*/    0x00,
/*R_TSTBISTRES2*/    0x00,
/*R_TSTBISTRES3*/    0x00

	        };/** end of array TMM6MHZ**/
#elif defined (STTUNER_TERR_TMM_7MHZ)
 U8 Def360Val[STV360_NBREGS]={
/*R_ID*/             0x21, 
/*R_I2CRPT*/         0x27, 
/*R_TOPCTRL*/        0x2 ,
/*R_IOCFG0*/         0x0, 
/*R_DAC0R*/          0x00, 

/*R_IOCFG1*/         0x40, 
/*R_DAC1R*/          0x00, 
/*R_IOCFG2*/         0x60, 
/*R_PWMFR*/          0x00,
/*R_STATUS*/         0xf9,

/*R_AUX_CLK*/        0x1c,
/*R_FREESYS1*/       0x00,
/*R_FREESYS2*/       0x00,
/*R_FREESYS3*/       0x00,
/*R_AGC2MAX*/        0xff, 

/*R_AGC2MIN*/        0x0, 
/*R_AGC1MAX*/        0x0, 
/*R_AGC1MIN*/        0x0, 
/*R_AGCR*/           0xbc, 
/*R_AGC2TH*/         0xfe, 

/*R_AGC12C3*/        0x00,
/*R_AGCCTRL1*/       0x85, 
/*R_AGCCTRL2*/       0x1f,
/*R_AGC1VAL1*/       0x00,
/*R_AGC1VAL2*/       0x0,

/*R_AGC2VAL1*/       0x1b, 
/*R_AGC2VAL2*/       0x08, 
/*R_AGC2PGA*/        0x00, 
/*R_OVF_RATE1*/      0x00, 
/*R_OVF_RATE2*/      0x00, 

/*R_GAIN_SRC1*/      0xcb, 
/*R_GAIN_SRC2*/      0xb8,
/*R_INC_DEROT1*/     0x55,
/*R_INC_DEROT2*/     0x4b,
/*R_FREESTFE_1*/     0x03,

/*R_SYR_THR*/        0x1c,
/*R_INR*/            0xff, 
/*R_EN_PROCESS*/     0x1, 
/*R_SDI_SMOOTHER*/   0xff, 
/*R_FE_LOOP_OPEN*/   0x00, 

/*R_EPQ*/            0x12, 
/*R_EPQ2*/           0xd,
/*R_COR_CTL*/        0x20,
/*R_COR_STAT*/       0xf6,
/*R_COR_INTEN*/      0x00,

/*R_COR_INTSTAT*/    0x39,
/*R_COR_MODEGUARD*/  0x3,
/*R_AGC_CTL*/        0x18,
/*R_AGC_MANUAL1*/    0x00,
/*R_AGC_MANUAL2*/    0x00,

/*R_AGC_TARGET*/     0x20,
/*R_AGC_GAIN1*/      0xfc,
/*R_AGC_GAIN2*/      0x10,
/*R_ITB_CTL*/        0x00,
/*R_ITB_FREQ1*/      0x00,

/*R_ITB_FREQ2*/      0x00,
/*R_CAS_CTL*/        0x85,
/*R_CAS_FREQ*/       0xB3,
/*R_CAS_DAGCGAIN*/   0x10,
/*R_SYR_CTL*/        0x00,

/*R_SYR_STAT*/       0x17,
/*R_SYR_NC01*/       0x00,
/*R_SYR_NC02*/       0x00,
/*R_SYR_OFFSET1*/    0x00,
/*R_SYR_OFFSET2*/    0x00,

/*R_FFT_CTL*/        0x00,
/*R_SCR_CTL*/        0x00,
/*R_PPM_CTL1*/       0x30,
/*R_TRL_CTL*/        0x94,
/*R_TRL_NOMRATE1*/   0xda,

/*R_TRL_NOMRATE2*/   0x4b,
/*R_TRL_TIME1*/      0x40,
/*R_TRL_TIME2*/      0xfd,
/*R_CRL_CTL*/        0x4F,
/*R_CRL_FREQ1*/      0x5f,

/*R_CRL_FREQ2*/      0xb1,
/*R_CRL_FREQ3*/      0x0,
/*R_CHC_CTL1*/       0xb1,
/*R_CHC_SNR*/        0xda,
/*R_BDI_CTL*/        0x00,

/*R_DMP_CTL*/        0x00,
/*R_TPS_RCVD1*/      0x30,
/*R_TPS_RCVD2*/      0x2,
/*R_TPS_RCVD3*/      0x01,
/*R_TPS_RCVD4*/      0x31,

/*R_TPS_CELLID*/     0x00,
/*R_TPS_FREE2*/      0x00,
/*R_TPS_SET1*/       0x01,
/*R_TPS_SET2*/       0x02,
/*R_TPS_SET3*/       0x4,

/*R_TPS_CTL*/        0x00,
/*R_CTL_FFTOSNUM*/   0x2b,
/*R_CAR_DISP_SEL*/   0x0C,
/*R_MSC_REV*/        0x0A,
/*R_PIR_CTL*/        0x00,

/*R_SNR_CARRIER1*/   0xA8,
/*R_SNR_CARRIER2*/   0x86,
/*R_PPM_CPAMP*/      0xbe,
/*R_TSM_AP0*/        0x00,
/*R_TSM_AP1*/        0x00,

/*R_TSM_AP2*/        0x00,
/*R_TSM_AP3*/        0x00,
/*R_TSM_AP4*/        0x00,
/*R_TSM_AP5*/        0x00,
/*R_TSM_AP6*/        0x00,

/*R_TSM_AP7*/        0x00,
/*R_CONSTMODE*/      0x02,
/*R_CONSTCARR1*/     0xD2,
/*R_CONSTCARR2*/     0x04,
/*R_ICONSTEL*/       0xf7,

/*R_QCONSTEL*/       0xf8,
/*R_AGC1RF*/         0xff,
/*R_EN_RF_AGC1*/     0x83,
/*R_FECM*/           0x00,
/*R_VTH0*/           0x1E,

/*R_VTH1*/           0x1e,
/*R_VTH2*/           0x0F,
/*R_VTH3*/           0x09,
/*R_VTH4*/           0x00,
/*R_VTH5*/           0x05,

/*R_FREEVIT*/        0x00,
/*R_VITPROG*/        0x92,
/*R_PR*/             0x2,
/*R_VSEARCH*/        0xb0,
/*R_RS*/             0xbc,

/*R_RSOUT*/          0x15,
/*R_ERRCTRL1*/       0x12,
/*R_ERRCNTM1*/       0x00,
/*R_ERRCNTL1*/       0x00,
/*R_ERRCTRL2*/       0xb2,

/*R_ERRCNTM2*/       0x00,
/*R_ERRCNTL2*/       0x00,
/*R_ERRCTRL3*/       0x12,
/*R_ERRCNTM3*/       0x00,
/*R_ERRCNTL3*/       0x00,

/*R_DILSTK1*/        0x00,
/*R_DILSTK0*/        0x03,
/*R_DILBWSTK1*/      0x00,
/*R_DILBWST0*/       0x03,
/*R_LNBRX*/          0x80,

/*R_RSTC*/           0xB0,
/*R_VIT_BIST*/       0x07,
/*R_FREEDRS*/        0x00,
/*R_VERROR*/         0x00,
/*R_TSTRES*/         0x00,

/*R_ANACTRL*/        0x00,
/*R_TSTBUS*/         0x00,
/*R_TSTCK*/          0x00,
/*R_TSTI2C*/         0x00,
/*R_TSTRAM1*/        0x00,

/*R_TSTRATE*/        0x00,
/*R_SELOUT*/         0x00,
/*R_FORCEIN*/        0x00,
/*R_TSTFIFO*/        0x00,
/*R_TSTRS*/          0x00,

/*R_TSTBISTRES0*/    0x00,
/*R_TSTBISTRES1*/    0x00,
/*R_TSTBISTRES2*/    0x00,
/*R_TSTBISTRES3*/    0x00

	        };/** end of array TMM7MHZ**/
#elif defined (STTUNER_TERR_TMM_8MHZ)
 U8 Def360Val[STV360_NBREGS]={
/*R_ID*/             0x21, 
/*R_I2CRPT*/         0x27, 
/*R_TOPCTRL*/        0x2 ,
/*R_IOCFG0*/         0x40, 
/*R_DAC0R*/          0x00, 

/*R_IOCFG1*/         0x40, 
/*R_DAC1R*/          0x00, 
/*R_IOCFG2*/         0x60, 
/*R_PWMFR*/          0x00,
/*R_STATUS*/         0xf9,

/*R_AUX_CLK*/        0x1c,
/*R_FREESYS1*/       0x00,
/*R_FREESYS2*/       0x00,
/*R_FREESYS3*/       0x00,
/*R_AGC2MAX*/        0xff, 

/*R_AGC2MIN*/        0x0, 
/*R_AGC1MAX*/        0x0, 
/*R_AGC1MIN*/        0x0, 
/*R_AGCR*/           0xbc, 
/*R_AGC2TH*/         0xfe, 

/*R_AGC12C3*/        0x00,
/*R_AGCCTRL1*/       0x85, 
/*R_AGCCTRL2*/       0x1f,
/*R_AGC1VAL1*/       0x00,
/*R_AGC1VAL2*/       0x0,

/*R_AGC2VAL1*/       0x2, 
/*R_AGC2VAL2*/       0x08, 
/*R_AGC2PGA*/        0x00, 
/*R_OVF_RATE1*/      0x00, 
/*R_OVF_RATE2*/      0x00, 

/*R_GAIN_SRC1*/      0xce, 
/*R_GAIN_SRC2*/      0x10,
/*R_INC_DEROT1*/     0x55,
/*R_INC_DEROT2*/     0x4b,
/*R_FREESTFE_1*/     0x03,

/*R_SYR_THR*/        0x1c,
/*R_INR*/            0xff, 
/*R_EN_PROCESS*/     0x1, 
/*R_SDI_SMOOTHER*/   0xff, 
/*R_FE_LOOP_OPEN*/   0x00, 

/*R_EPQ*/            0x11, 
/*R_EPQ2*/           0xd,
/*R_COR_CTL*/        0x20,
/*R_COR_STAT*/       0xf6,
/*R_COR_INTEN*/      0x00,

/*R_COR_INTSTAT*/    0x3d,
/*R_COR_MODEGUARD*/  0x3,
/*R_AGC_CTL*/        0x18,
/*R_AGC_MANUAL1*/    0x00,
/*R_AGC_MANUAL2*/    0x00,

/*R_AGC_TARGET*/     0x20,
/*R_AGC_GAIN1*/      0x9,
/*R_AGC_GAIN2*/      0x10,
/*R_ITB_CTL*/        0x00,
/*R_ITB_FREQ1*/      0x00,

/*R_ITB_FREQ2*/      0x00,
/*R_CAS_CTL*/        0x85,
/*R_CAS_FREQ*/       0xB3,
/*R_CAS_DAGCGAIN*/   0x10,
/*R_SYR_CTL*/        0x00,

/*R_SYR_STAT*/       0x17,
/*R_SYR_NC01*/       0x00,
/*R_SYR_NC02*/       0x00,
/*R_SYR_OFFSET1*/    0x00,
/*R_SYR_OFFSET2*/    0x00,

/*R_FFT_CTL*/        0x00,
/*R_SCR_CTL*/        0x00,
/*R_PPM_CTL1*/       0x30,
/*R_TRL_CTL*/        0x94,
/*R_TRL_NOMRATE1*/   0xb0,

/*R_TRL_NOMRATE2*/   0x56,
/*R_TRL_TIME1*/      0xee,
/*R_TRL_TIME2*/      0xfd,
/*R_CRL_CTL*/        0x4F,
/*R_CRL_FREQ1*/      0x90,

/*R_CRL_FREQ2*/      0x9b,
/*R_CRL_FREQ3*/      0x0,
/*R_CHC_CTL1*/       0xb1,
/*R_CHC_SNR*/        0xd7,
/*R_BDI_CTL*/        0x00,

/*R_DMP_CTL*/        0x00,
/*R_TPS_RCVD1*/      0x32,
/*R_TPS_RCVD2*/      0x2,
/*R_TPS_RCVD3*/      0x01,
/*R_TPS_RCVD4*/      0x31,

/*R_TPS_CELLID*/     0x00,
/*R_TPS_FREE2*/      0x00,
/*R_TPS_SET1*/       0x01,
/*R_TPS_SET2*/       0x02,
/*R_TPS_SET3*/       0x4,

/*R_TPS_CTL*/        0x00,
/*R_CTL_FFTOSNUM*/   0x2b,
/*R_CAR_DISP_SEL*/   0x0C,
/*R_MSC_REV*/        0x0A,
/*R_PIR_CTL*/        0x00,

/*R_SNR_CARRIER1*/   0xA8,
/*R_SNR_CARRIER2*/   0x86,
/*R_PPM_CPAMP*/      0xcc,
/*R_TSM_AP0*/        0x00,
/*R_TSM_AP1*/        0x00,

/*R_TSM_AP2*/        0x00,
/*R_TSM_AP3*/        0x00,
/*R_TSM_AP4*/        0x00,
/*R_TSM_AP5*/        0x00,
/*R_TSM_AP6*/        0x00,

/*R_TSM_AP7*/        0x00,
/*R_CONSTMODE*/      0x02,
/*R_CONSTCARR1*/     0xD2,
/*R_CONSTCARR2*/     0x04,
/*R_ICONSTEL*/       0xf7,

/*R_QCONSTEL*/       0xf8,
/*R_AGC1RF*/         0xff,
/*R_EN_RF_AGC1*/     0x83,
/*R_FECM*/           0x00,
/*R_VTH0*/           0x1E,

/*R_VTH1*/           0x1e,
/*R_VTH2*/           0x0F,
/*R_VTH3*/           0x09,
/*R_VTH4*/           0x00,
/*R_VTH5*/           0x05,

/*R_FREEVIT*/        0x00,
/*R_VITPROG*/        0x92,
/*R_PR*/             0x2,
/*R_VSEARCH*/        0xb0,
/*R_RS*/             0xbc,

/*R_RSOUT*/          0x15,
/*R_ERRCTRL1*/       0x12,
/*R_ERRCNTM1*/       0x00,
/*R_ERRCNTL1*/       0x00,
/*R_ERRCTRL2*/       0x12,

/*R_ERRCNTM2*/       0x00,
/*R_ERRCNTL2*/       0x00,
/*R_ERRCTRL3*/       0x12,
/*R_ERRCNTM3*/       0x00,
/*R_ERRCNTL3*/       0x00,

/*R_DILSTK1*/        0x00,
/*R_DILSTK0*/        0x03,
/*R_DILBWSTK1*/      0x00,
/*R_DILBWST0*/       0x03,
/*R_LNBRX*/          0x80,

/*R_RSTC*/           0xB0,
/*R_VIT_BIST*/       0x07,
/*R_FREEDRS*/        0x00,
/*R_VERROR*/         0x00,
/*R_TSTRES*/         0x00,

/*R_ANACTRL*/        0x00,
/*R_TSTBUS*/         0x00,
/*R_TSTCK*/          0x00,
/*R_TSTI2C*/         0x00,
/*R_TSTRAM1*/        0x00,

/*R_TSTRATE*/        0x00,
/*R_SELOUT*/         0x00,
/*R_FORCEIN*/        0x00,
/*R_TSTFIFO*/        0x00,
/*R_TSTRS*/          0x00,

/*R_TSTBISTRES0*/    0x00,
/*R_TSTBISTRES1*/    0x00,
/*R_TSTBISTRES2*/    0x00,
/*R_TSTBISTRES3*/    0x00

	        };/** end of array TMM8MHZ**/
#else
U8 Def360Val[STV360_NBREGS]={
/*R_ID*/             0x21, 
/*R_I2CRPT*/         0x27, 
/*R_TOPCTRL*/        0x2 ,
/*R_IOCFG0*/         0x40, 
/*R_DAC0R*/          0x00, 

/*R_IOCFG1*/         0x00, 
/*R_DAC1R*/          0x00, 
/*R_IOCFG2*/         0x80, 
/*R_PWMFR*/          0x00,
/*R_STATUS*/         0xf9,

/*R_AUX_CLK*/        0x1c,
/*R_FREESYS1*/       0x00,
/*R_FREESYS2*/       0x00,
/*R_FREESYS3*/       0x00,
/*R_AGC2MAX*/        0xFF, 

/*R_AGC2MIN*/        0x28, 
/*R_AGC1MAX*/        0xff, 
/*R_AGC1MIN*/        0x6b, 
/*R_AGCR*/           0xbc, 
/*R_AGC2TH*/         0x0c, 

/*R_AGC12C3*/        0x00,
/*R_AGCCTRL1*/       0x85, 
/*R_AGCCTRL2*/       0x1f,
/*R_AGC1VAL1*/       0xff,
/*R_AGC1VAL2*/       0xf,

/*R_AGC2VAL1*/       0xff, 
/*R_AGC2VAL2*/       0x0f, 
/*R_AGC2PGA*/        0x00, 
/*R_OVF_RATE1*/      0x00, 
/*R_OVF_RATE2*/      0x00, 

/*R_GAIN_SRC1*/      0xca, 
/*R_GAIN_SRC2*/      0x41,
/*R_INC_DEROT1*/     0x55,
/*R_INC_DEROT2*/     0x53,
/*R_FREESTFE_1*/     0x03,

/*R_SYR_THR*/        0x1c,
/*R_INR*/            0xff, 
/*R_EN_PROCESS*/     0x1, 
/*R_SDI_SMOOTHER*/   0xff, 
/*R_FE_LOOP_OPEN*/   0x00, 

/*R_EPQ*/            0x00, 
/*R_EPQ2*/           0x15,
/*R_COR_CTL*/        0x20,
/*R_COR_STAT*/       0xf6,
/*R_COR_INTEN*/      0x00,

/*R_COR_INTSTAT*/    0x3F,
/*R_COR_MODEGUARD*/  0x3,
/*R_AGC_CTL*/        0x18,
/*R_AGC_MANUAL1*/    0x00,
/*R_AGC_MANUAL2*/    0x00,

/*R_AGC_TARGET*/     0x28,
/*R_AGC_GAIN1*/      0xFF,
/*R_AGC_GAIN2*/      0x17,
/*R_ITB_CTL*/        0x00,
/*R_ITB_FREQ1*/      0x00,

/*R_ITB_FREQ2*/      0x00,
/*R_CAS_CTL*/        0x40,
/*R_CAS_FREQ*/       0xB3,
/*R_CAS_DAGCGAIN*/   0x10,
/*R_SYR_CTL*/        0x00,

/*R_SYR_STAT*/       0x13,
/*R_SYR_NC01*/       0x00,
/*R_SYR_NC02*/       0x00,
/*R_SYR_OFFSET1*/    0x00,
/*R_SYR_OFFSET2*/    0x00,

/*R_FFT_CTL*/        0x00,
/*R_SCR_CTL*/        0x00,
/*R_PPM_CTL1*/       0x30,
/*R_TRL_CTL*/        0x94,
/*R_TRL_NOMRATE1*/   0x4d,

/*R_TRL_NOMRATE2*/   0x55,
/*R_TRL_TIME1*/      0xc1,
/*R_TRL_TIME2*/      0xF8,
/*R_CRL_CTL*/        0x4F,
/*R_CRL_FREQ1*/      0xdc,

/*R_CRL_FREQ2*/      0xf1,
/*R_CRL_FREQ3*/      0xff,
/*R_CHC_CTL1*/       0x01,
/*R_CHC_SNR*/        0xE8,
/*R_BDI_CTL*/        0x60,

/*R_DMP_CTL*/        0x00,
/*R_TPS_RCVD1*/      0x32,
/*R_TPS_RCVD2*/      0x2,
/*R_TPS_RCVD3*/      0x01,
/*R_TPS_RCVD4*/      0x30,

/*R_TPS_CELLID*/     0x00,
/*R_TPS_FREE2*/      0x00,
/*R_TPS_SET1*/       0x01,
/*R_TPS_SET2*/       0x02,
/*R_TPS_SET3*/       0x01,

/*R_TPS_CTL*/        0x00,
/*R_CTL_FFTOSNUM*/   0x27,
/*R_CAR_DISP_SEL*/   0x0C,
/*R_MSC_REV*/        0x0A,
/*R_PIR_CTL*/        0x00,

/*R_SNR_CARRIER1*/   0xA8,
/*R_SNR_CARRIER2*/   0x86,
/*R_PPM_CPAMP*/      0x2C,
/*R_TSM_AP0*/        0x00,
/*R_TSM_AP1*/        0x00,

/*R_TSM_AP2*/        0x00,
/*R_TSM_AP3*/        0x00,
/*R_TSM_AP4*/        0x00,
/*R_TSM_AP5*/        0x00,
/*R_TSM_AP6*/        0x00,

/*R_TSM_AP7*/        0x00,
/*R_CONSTMODE*/      0x02,
/*R_CONSTCARR1*/     0xD2,
/*R_CONSTCARR2*/     0x04,
/*R_ICONSTEL*/       0xDC,

/*R_QCONSTEL*/       0xDB,
/*R_AGC1RF*/         0xAB,
/*R_EN_RF_AGC1*/     0x03,
/*R_FECM*/           0x00,
/*R_VTH0*/           0x1E,

/*R_VTH1*/           0x14,
/*R_VTH2*/           0x0F,
/*R_VTH3*/           0x09,
/*R_VTH4*/           0x00,
/*R_VTH5*/           0x05,

/*R_FREEVIT*/        0x00,
/*R_VITPROG*/        0x92,
/*R_PR*/             0xAF,
/*R_VSEARCH*/        0x30,
/*R_RS*/             0xFE,

/*R_RSOUT*/          0x15,
/*R_ERRCTRL1*/       0x12,
/*R_ERRCNTM1*/       0x00,
/*R_ERRCNTL1*/       0x00,
/*R_ERRCTRL2*/       0x12,

/*R_ERRCNTM2*/       0x00,
/*R_ERRCNTL2*/       0x00,
/*R_ERRCTRL3*/       0x12,
/*R_ERRCNTM3*/       0x00,
/*R_ERRCNTL3*/       0x00,

/*R_DILSTK1*/        0x00,
/*R_DILSTK0*/        0x0C,
/*R_DILBWSTK1*/      0x00,
/*R_DILBWST0*/       0x03,
/*R_LNBRX*/          0x80,

/*R_RSTC*/           0xB0,
/*R_VIT_BIST*/       0x07,
/*R_FREEDRS*/        0x00,
/*R_VERROR*/         0x00,
/*R_TSTRES*/         0x00,

/*R_ANACTRL*/        0x00,
/*R_TSTBUS*/         0x00,
/*R_TSTCK*/          0x00,
/*R_TSTI2C*/         0x00,
/*R_TSTRAM1*/        0x00,

/*R_TSTRATE*/        0x00,
/*R_SELOUT*/         0x00,
/*R_FORCEIN*/        0x00,
/*R_TSTFIFO*/        0x00,
/*R_TSTRS*/          0x00,

/*R_TSTBISTRES0*/    0x00,
/*R_TSTBISTRES1*/    0x00,
/*R_TSTBISTRES2*/    0x00,
/*R_TSTBISTRES3*/    0x00

	        };/** end of array**/
#endif	        
/* Private types/constants ------------------------------------------------ */

#define ANALOG_CARRIER_DETECT_SYMBOL_RATE   5000000
#define ANALOG_CARRIER_DETECT_AGC2_VALUE    25

/* Device capabilities */
#define MAX_AGC                         4095 /*fix for DDTS 33979 "AGC calculations for STV0360"*/
#define MAX_SIGNAL_QUALITY              100
#define MAX_BER                         200000



#define WAIT_N_MS(X)     STOS_TaskDelayUs(X * 1000)

#if defined (ST_OS21) || defined (ST_OSLINUX)
#define STTUNER_TaskDelay(x) STOS_TaskDelay((signed int)x)
#else
#define STTUNER_TaskDelay(x) STOS_TaskDelay((unsigned int)x)
#endif

/* private variables ------------------------------------------------------- */

static semaphore_t *Lock_InitTermOpenClose; /* guard calls to the functions */

static BOOL        Installed = FALSE;



/* instance chain, the default boot value is invalid, to catch errors */
static D0360_InstanceData_t *InstanceChainTop = (D0360_InstanceData_t *)0x7fffffff;

/* functions --------------------------------------------------------------- */

int FE_360_PowOf2(int number);

/* API */
ST_ErrorCode_t demod_d0360_Init(ST_DeviceName_t *DeviceName, DEMOD_InitParams_t *InitParams);
ST_ErrorCode_t demod_d0360_Term(ST_DeviceName_t *DeviceName, DEMOD_TermParams_t *TermParams);

ST_ErrorCode_t demod_d0360_Open (ST_DeviceName_t *DeviceName, DEMOD_OpenParams_t  *OpenParams, DEMOD_Capability_t *Capability, DEMOD_Handle_t *Handle);
ST_ErrorCode_t demod_d0360_Close(DEMOD_Handle_t  Handle, DEMOD_CloseParams_t *CloseParams);

ST_ErrorCode_t demod_d0360_GetTunerInfo    (DEMOD_Handle_t Handle, STTUNER_TunerInfo_t *TunerInfo_p);
ST_ErrorCode_t demod_d0360_GetSignalQuality(DEMOD_Handle_t Handle, U32  *SignalQuality_p, U32 *Ber);
ST_ErrorCode_t demod_d0360_GetModulation   (DEMOD_Handle_t Handle, STTUNER_Modulation_t *Modulation);
ST_ErrorCode_t demod_d0360_GetAGC          (DEMOD_Handle_t Handle, S16                  *Agc);
ST_ErrorCode_t demod_d0360_GetMode         (DEMOD_Handle_t Handle, STTUNER_Mode_t       *Mode);
ST_ErrorCode_t demod_d0360_GetGuard        (DEMOD_Handle_t Handle, STTUNER_Guard_t      *Guard);
ST_ErrorCode_t demod_d0360_GetFECRates     (DEMOD_Handle_t Handle, STTUNER_FECRate_t    *FECRates);
ST_ErrorCode_t demod_d0360_IsLocked        (DEMOD_Handle_t Handle, BOOL                 *IsLocked);
ST_ErrorCode_t demod_d0360_Tracking        (DEMOD_Handle_t Handle, BOOL ForceTracking,   U32 *NewFrequency, BOOL *SignalFound);
ST_ErrorCode_t demod_d0360_ScanFrequency   (DEMOD_Handle_t  Handle,
                                            U32  InitialFrequency,
                                            U32  SymbolRate,
                                            U32  MaxOffset,
                                            U32  TunerStep,
                                            U8   DerotatorStep,
                                            BOOL *ScanSuccess,
                                            U32  *NewFrequency,
                                            U32  Mode,
                                            U32  Guard,
                                            U32  Force,
                                            U32  Hierarchy,
                                            U32  Spectrum,
                                            U32  FreqOff,
                                            U32  ChannelBW,
                                            S32  EchoPos);
ST_ErrorCode_t demod_d0360_ioctl           (DEMOD_Handle_t Handle, U32 Function, void *InParams, void *OutParams,
                                             STTUNER_Da_Status_t *Status);
ST_ErrorCode_t demod_d0360_GetRFLevel(DEMOD_Handle_t Handle, S32  *Rflevel);

ST_ErrorCode_t demod_d0360_GetTPSCellId(DEMOD_Handle_t Handle, U16  *TPSCellId);

/* For STANDBY API mode */ 
ST_ErrorCode_t demod_d0360_StandByMode(DEMOD_Handle_t Handle, STTUNER_StandByMode_t PowerMode);

/* I/O API */
ST_ErrorCode_t demod_d0360_ioaccess(DEMOD_Handle_t Handle, IOARCH_Handle_t IOHandle,
                  STTUNER_IOARCH_Operation_t Operation, U16 SubAddr, U8 *Data, U32 TransferSize, U32 Timeout);
                  
ST_ErrorCode_t demod_d0360_TRL_ioctl_set(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,void *InParams);                  
ST_ErrorCode_t demod_d0360_TRL_ioctl_get(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,void *OutParams);

/* local functions --------------------------------------------------------- */

D0360_InstanceData_t *D0360_GetInstFromHandle(DEMOD_Handle_t Handle);

/* ----------------------------------------------------------------------------
Name: STTUNER_DRV_DEMOD_STV0360_Install()

Description:
    install a terrestrial device driver into the demod database.

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_DRV_DEMOD_STV0360_Install(STTUNER_demod_dbase_t *Demod)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
   const char *identity = "STTUNER d0360.c STTUNER_DRV_DEMOD_STV0360_Install()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if(Installed == TRUE)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
        STTBX_Print(("%s fail driver already installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
    STTBX_Print(("%s installing ter:demod:STV0360...", identity));
#endif

    /* mark ID in database */
    Demod->ID = STTUNER_DEMOD_STV0360;

    /* map API */
    Demod->demod_Init = demod_d0360_Init;
    Demod->demod_Term = demod_d0360_Term;

    Demod->demod_Open  = demod_d0360_Open;
    Demod->demod_Close = demod_d0360_Close;

    Demod->demod_IsAnalogCarrier  = NULL;
    Demod->demod_GetTunerInfo     = demod_d0360_GetTunerInfo;
    Demod->demod_GetSignalQuality = demod_d0360_GetSignalQuality;
    Demod->demod_GetModulation    = demod_d0360_GetModulation;
    Demod->demod_GetAGC           = demod_d0360_GetAGC;
    Demod->demod_GetMode          = demod_d0360_GetMode;
    Demod->demod_GetGuard         = demod_d0360_GetGuard;
    Demod->demod_GetFECRates      = demod_d0360_GetFECRates;
    Demod->demod_IsLocked         = demod_d0360_IsLocked ;
    Demod->demod_SetFECRates      = NULL;
    Demod->demod_Tracking         = demod_d0360_Tracking;
    Demod->demod_ScanFrequency    = demod_d0360_ScanFrequency;

    Demod->demod_ioaccess = demod_d0360_ioaccess;
    Demod->demod_ioctl    = demod_d0360_ioctl;
    Demod->demod_GetRFLevel	  = demod_d0360_GetRFLevel;
    Demod->demod_GetTPSCellId = demod_d0360_GetTPSCellId;
    Demod->demod_StandByMode =  demod_d0360_StandByMode;
    

    InstanceChainTop = NULL;


    Lock_InitTermOpenClose = STOS_SemaphoreCreateFifo(NULL,1);


    Installed = TRUE;

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
    STTBX_Print(("ok\n"));
#endif

    return(Error);
}



/* ----------------------------------------------------------------------------
Name: STTUNER_DRV_DEMOD_STV0360_UnInstall()

Description:
    install a terrestrial device driver into the demod database.

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_DRV_DEMOD_STV0360_UnInstall(STTUNER_demod_dbase_t *Demod)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
   const char *identity = "STTUNER d0360.c STTUNER_DRV_DEMOD_STV0360_UnInstall()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

    if(Demod->ID != STTUNER_DEMOD_STV0360)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
        STTBX_Print(("%s fail incorrect driver type\n", identity));
#endif
        return(STTUNER_ERROR_ID);
    }

    /* has all memory been freed, by Term() */
    if(InstanceChainTop != NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
        STTBX_Print(("%s fail at least one instance not terminated\n", identity));
#endif
        return(ST_ERROR_OPEN_HANDLE);
    }


#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
    STTBX_Print(("%s uninstalling ter:demod:STV0360...", identity));
#endif

    /* mark ID in database */
    Demod->ID = STTUNER_NO_DRIVER;

    /* unmap API */
    Demod->demod_Init = NULL;
    Demod->demod_Term = NULL;

    Demod->demod_Open  = NULL;
    Demod->demod_Close = NULL;

    Demod->demod_IsAnalogCarrier  = NULL;
    Demod->demod_GetTunerInfo     = NULL;
    Demod->demod_GetSignalQuality = NULL;
    Demod->demod_GetModulation    = NULL;
    Demod->demod_GetAGC           = NULL;
    Demod->demod_GetMode          = NULL;
    Demod->demod_GetFECRates      = NULL;
    Demod->demod_GetGuard         = NULL;
    Demod->demod_IsLocked         = NULL;
    Demod->demod_SetFECRates      = NULL;
    Demod->demod_Tracking         = NULL;
    Demod->demod_ScanFrequency    = NULL;

    Demod->demod_ioaccess = NULL;
    Demod->demod_ioctl    = NULL;
    Demod->demod_GetRFLevel	  = NULL;
    Demod->demod_GetTPSCellId     = NULL;
     Demod->demod_StandByMode     = NULL ;

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
        STTBX_Print(("<"));
#endif


        STOS_SemaphoreDelete(NULL,Lock_InitTermOpenClose);

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
        STTBX_Print((">"));
#endif

    InstanceChainTop = (D0360_InstanceData_t *)0x7ffffffe;
    Installed        = FALSE;

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
        STTBX_Print(("ok\n"));
#endif

    return(Error);
}



/* ------------------------------------------------------------------------- */
/* /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ API /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ */
/* ------------------------------------------------------------------------- */



/* ----------------------------------------------------------------------------
Name: demod_d0360_Init()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0360_Init(ST_DeviceName_t *DeviceName, DEMOD_InitParams_t *InitParams)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
    const char *identity = "STTUNER d0360.c demod_d0360_Init()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0360_InstanceData_t *InstanceNew, *Instance;


    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
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
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
        STTBX_Print(("%s fail MemoryPartition not valid\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    Error = STTUNER_Util_CheckPtrNull(DeviceName);
    if( Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
        STTBX_Print(("%s fail DeviceName not valid\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    InstanceNew = STOS_MemoryAllocateClear(InitParams->MemoryPartition, 1, sizeof( D0360_InstanceData_t ));
    if (InstanceNew == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
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

    InstanceNew->ExternalClock       = InitParams->ExternalClock;
    InstanceNew->TSOutputMode        = InitParams->TSOutputMode;
    InstanceNew->SerialDataMode      = InitParams->SerialDataMode;
    InstanceNew->SerialClockSource   = InitParams->SerialClockSource;
    InstanceNew->FECMode             = InitParams->FECMode;
    InstanceNew->ClockPolarity       = InitParams->ClockPolarity;


    InstanceNew->DeviceMap.Timeout   = IOREG_DEFAULT_TIMEOUT;
    InstanceNew->DeviceMap.Registers = STV360_NBREGS;
    InstanceNew->DeviceMap.Fields    = STV360_NBFIELDS;
    InstanceNew->DeviceMap.Mode      = IOREG_MODE_SUBADR_8; /* i/o addressing mode to use */
    InstanceNew->DeviceMap.MemoryPartition = InitParams->MemoryPartition;
    InstanceNew->DeviceMap.DefVal= (U32 *)&Def360Val[0];
    /* Allocate  memory for register mapping */
    Error = STTUNER_IOREG_Open(&InstanceNew->DeviceMap);
     if (Error != ST_NO_ERROR)
    {
	#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
        STTBX_Print(("%s fail setup new register database\n", identity));
	#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error); 
            }

    InstanceNew->StandBy_Flag = 0 ;
    
    switch(InstanceNew->TSOutputMode)
    {
        case STTUNER_TS_MODE_DEFAULT:
        case STTUNER_TS_MODE_PARALLEL:
            InstanceNew->FE_360_InitParams.Clock = FE_PARALLEL_CLOCK;
            break;

        case STTUNER_TS_MODE_SERIAL:
            switch(InstanceNew->SerialClockSource) /* Set serial clock source */
            {
                case STTUNER_SCLK_VCODIV6:
                    InstanceNew->FE_360_InitParams.Clock = FE_SERIAL_VCODIV6_CLOCK;
                    break;
		default:
                case STTUNER_SCLK_DEFAULT:
                case STTUNER_SCLK_MASTER:
                    InstanceNew->FE_360_InitParams.Clock = FE_SERIAL_MASTER_CLOCK;
                    break;
            }

	case STTUNER_TS_MODE_DVBCI:
	break;

    }

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
    STTBX_Print(("%s allocated & initalized block named '%s' at 0x%08x (%d bytes)\n", identity, InstanceNew->DeviceName, (U32)InstanceNew, sizeof( D0360_InstanceData_t ) ));
#endif
    SEM_UNLOCK(Lock_InitTermOpenClose);
    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0360_Term()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0360_Term(ST_DeviceName_t *DeviceName, DEMOD_TermParams_t *TermParams)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
   const char *identity = "STTUNER d0360.c demod_d0360_Term()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0360_InstanceData_t *Instance, *InstancePrev, *InstanceNext;

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
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
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
        STTBX_Print(("%s fail TermParams not valid\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    /* ---------- check that at least one init has taken place ---------- */
    if(InstanceChainTop == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
        STTBX_Print(("%s fail nothing initalized\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(STTUNER_ERROR_INITSTATE);
    }


    /* reap next matching DeviceName */
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
    STTBX_Print(("%s looking for first free handle[\n", identity));
#endif

    Instance = InstanceChainTop;
    while(1)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
        STTBX_Print(("(%s)", Instance->DeviceName));
#endif
        if ( strcmp( (char *)Instance->DeviceName, (char *)DeviceName) == 0)
        {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
            STTBX_Print(("<-- ]\n"));
#endif
            /* found so now xlink prev and next(if applicable) and deallocate memory */
            InstancePrev = Instance->InstanceChainPrev;
            InstanceNext = Instance->InstanceChainNext;

            /* if instance to delete is first in chain */
            if (Instance->InstanceChainPrev == NULL)
            {
                InstanceChainTop = InstanceNext;        /* which would be NULL if last block to be term'd */
                if (InstanceNext != NULL)
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


 /* Added for removing memory related problem in API test
                 memory for register mapping */
           Error = STTUNER_IOREG_Close(&Instance->DeviceMap);
           if (Error != ST_NO_ERROR)
            {
	   #ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
            STTBX_Print(("%s fail deallocate register database\n", identity));
	   #endif
           SEM_UNLOCK(Lock_InitTermOpenClose);
           return(Error);           
            }


            STOS_MemoryDeallocate(Instance->MemoryPartition, Instance);

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
            STTBX_Print(("%s freed block at %0x%08x\n", identity, (U32)Instance ));
#endif

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
            STTBX_Print(("%s terminated ok\n", identity));
#endif
            SEM_UNLOCK(Lock_InitTermOpenClose);
            return(Error);
        }
        else if(Instance->InstanceChainNext == NULL)
        {       /* error we should have found a matching name before the end of the list */
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
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


#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
    STTBX_Print(("%s FAIL! this point should NEVER be reached\n", identity));
#endif
    SEM_UNLOCK(Lock_InitTermOpenClose);
    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0360_Open()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0360_Open(ST_DeviceName_t *DeviceName, DEMOD_OpenParams_t  *OpenParams, DEMOD_Capability_t *Capability, DEMOD_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
   const char *identity = "STTUNER d0360.c demod_d0360_Open()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0360_InstanceData_t     *Instance;
    U8 ChipID;


    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

    /* now safe to lock semaphore */
    SEM_LOCK(Lock_InitTermOpenClose);

    /* ---------- check that at least one init has taken place ---------- */
    if(InstanceChainTop == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
        STTBX_Print(("%s fail nothing initalized\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(STTUNER_ERROR_INITSTATE);
    }

    /* find  handle from name */
    Instance = InstanceChainTop;
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
    STTBX_Print(("%s looking (%s)", identity, Instance->DeviceName));
#endif
    while(strcmp((char *)Instance->DeviceName, (char *)DeviceName) != 0)
    {
        /* error, should have found matching DeviceName before end of list */
        if(Instance->InstanceChainNext == NULL)
        {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
            STTBX_Print(("\n%s fail no block named '%s' before end of list\n", identity, DeviceName));
#endif
            SEM_UNLOCK(Lock_InitTermOpenClose);
            return(STTUNER_ERROR_INITSTATE);
        }
        Instance = Instance->InstanceChainNext;   /* next block */
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
        STTBX_Print(("(%s)", Instance->DeviceName));
#endif
    }
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
        STTBX_Print((" found ok\n"));
#endif

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
    STTBX_Print(("%s using block at 0x%08x\n", identity, (U32)Instance));
#endif

    /* check handle IS actually free */
    if(Instance->TopLevelHandle != STTUNER_MAX_HANDLES)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
        STTBX_Print(("%s fail driver instance already open\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_OPEN_HANDLE);
    }
    /* else now got pointer to free (and valid) data block */

   

     ChipID = STTUNER_IOREG_GetRegister( &(Instance->DeviceMap), Instance->IOHandle, R_ID);













    if ( (ChipID & 0x30) ==  0x00)   /* Accept cuts 1 & 2 (0x10 or 0x20) */
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
        STTBX_Print(("%s fail device not found (0x%02x)\n", identity, ChipID));
#endif
       











        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_UNKNOWN_DEVICE);
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
        STTBX_Print(("%s device found, release/revision=%u\n", identity, ChipID & 0x30));
#endif
    }
/* reset all chip registers */
      Error = STTUNER_IOREG_Reset(&(Instance->DeviceMap), Instance->IOHandle,Def360Val,STV0360_Address);



    /* Set serial/parallel data mode */
    if (Instance->TSOutputMode == STTUNER_TS_MODE_SERIAL)
    {
        STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle, OUTRS_SP, 1);
    }
    else
    {
        STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle, OUTRS_SP, 0);
    }
    
    /*set data clock polarity mode (rising/falling)*/
    switch(Instance->ClockPolarity)
    {
       case STTUNER_DATA_CLOCK_POLARITY_RISING:
    	 STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle, CLK_POL, 1);
    	 break;
       case STTUNER_DATA_CLOCK_POLARITY_FALLING:
    	 STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle, CLK_POL, 0);
    	 break;
       case STTUNER_DATA_CLOCK_POLARITY_DEFAULT:
    	 STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle, CLK_POL, 1);
         break;
    	default:
    	break;
    }
   
    /* Set capabilties */
    Capability->FECAvail        = STTUNER_FEC_ALL;  /* direct mapping to STTUNER_FECRate_t    */
    Capability->ModulationAvail = STTUNER_MOD_ALL;  /* direct mapping to STTUNER_Modulation_t */
    Capability->AGCControl      = FALSE;
    Capability->SymbolMin       =  1000000; /*   1 MegaSymbols/sec */
    Capability->SymbolMax       = 50000000; /*  50 MegaSymbols/sec */

    Capability->BerMax           = MAX_BER;
    Capability->SignalQualityMax = MAX_SIGNAL_QUALITY;
    Capability->AgcMax           = MAX_AGC;

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
    STTBX_Print(("%s no EVALMAX tuner driver, F0360_IAGC=0\n", identity));
#endif


    /* finally (as nor more errors to check for, allocate handle */
    Instance->TopLevelHandle = OpenParams->TopLevelHandle;
    *Handle = (U32)Instance;

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
    STTBX_Print(("%s opened ok\n", identity));
#endif

    SEM_UNLOCK(Lock_InitTermOpenClose);
    


    
    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0360_Close()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0360_Close(DEMOD_Handle_t Handle, DEMOD_CloseParams_t *CloseParams)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
   const char *identity = "STTUNER d0360.c demod_d0360_Close()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0360_InstanceData_t     *Instance;

    /* private driver instance data */
    Instance = D0360_GetInstFromHandle(Handle);

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

    SEM_LOCK(Lock_InitTermOpenClose);

    /* ---------- check that at least one init has taken place ---------- */
    if(InstanceChainTop == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
        STTBX_Print(("%s fail nothing initalized\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(STTUNER_ERROR_INITSTATE);
    }

    if(Instance->TopLevelHandle == STTUNER_MAX_HANDLES)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
        STTBX_Print(("%s fail driver instance not open\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_OPEN_HANDLE);
    }
















    /* indicate instance is closed */
    Instance->TopLevelHandle = STTUNER_MAX_HANDLES;

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
    STTBX_Print(("%s closed\n", identity));
#endif

    SEM_UNLOCK(Lock_InitTermOpenClose);
    return(Error);
}


/* ----------------------------------------------------------------------------
Name: demod_d0360_GetTunerInfo()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0360_GetTunerInfo(DEMOD_Handle_t Handle, STTUNER_TunerInfo_t *TunerInfo_p)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
   const char *identity = "STTUNER d0360.c demod_d0360_GetTunerInfo()";
#endif
    ST_ErrorCode_t       Error = ST_NO_ERROR;
    ST_ErrorCode_t       TunerError ;
    D0360_InstanceData_t *Instance;
    STTUNER_InstanceDbase_t     *Inst;
    U8                   Data;
    U32                  CurFrequency, CurSignalQuality, CurBitErrorRate;
    STTUNER_Modulation_t CurModulation;
    STTUNER_Mode_t       CurMode;
    STTUNER_FECRate_t    CurFECRates;
    STTUNER_Guard_t      CurGuard;
    STTUNER_Spectrum_t   CurSpectrum;
    /*STTUNER_Hierarchy_t  CurHierarchy;*/
    STTUNER_Hierarchy_Alpha_t CurHierMode;
    S32                  CurEchoPos;
   
    int offset=0;
    int offsetvar=0;
    char signvalue=0;
    int offset_type=0;
    /* private driver instance data */
    Instance = D0360_GetInstFromHandle(Handle);
    
    /* top level public instance data */
    Inst = STTUNER_GetDrvInst();
    
    /* Read noise estimations for C/N and BER */
    FE_360_GetNoiseEstimator(&(Instance->DeviceMap), Instance->IOHandle, &CurSignalQuality, &CurBitErrorRate);
  
  /******The Error field of TunerInfo in Instaneous database is updated ******/
    
    TunerError= ST_NO_ERROR; 
       
    /*************************************************************************/
    /* Get the modulation type */
    #ifndef ST_OSLINUX
    STOS_TaskLock();
    #ifdef PROCESSOR_C1 /***For C1 core this delay needed otherwise STTUNER_task_lock(); not working fine ****/ 
    /*task_delay(5);*/
    #endif
    #endif
    Data=STTUNER_IOREG_GetField(&(Instance->DeviceMap), Instance->IOHandle,  TPS_CONST);
    #ifndef ST_OSLINUX
    STOS_TaskUnlock();
    #endif
   /* Data = STTUNER_IOREG_FieldGetVal(&(Instance->DeviceMap), TPS_CONST);*/
    switch (Data)
    {
        case 0:  CurModulation = STTUNER_MOD_QPSK;  break;
        case 1:  CurModulation = STTUNER_MOD_16QAM; break;
        case 2:  CurModulation = STTUNER_MOD_64QAM; break;
        default: 
        /*CurModulation = STTUNER_MOD_ALL;*/
        CurModulation = Data ;
        STTUNER_TaskDelay(5);
        Data=STTUNER_IOREG_GetField(&(Instance->DeviceMap), Instance->IOHandle,  TPS_CONST);
        switch (Data)
        {
        case 0:  CurModulation = STTUNER_MOD_QPSK;  break;
        case 1:  CurModulation = STTUNER_MOD_16QAM; break;
        case 2:  CurModulation = STTUNER_MOD_64QAM; break;
        default:
        CurModulation = Data ;
        TunerError=ST_ERROR_BAD_PARAMETER;
        break;
        }
        break;
    }

    /* Get the mode */
    #ifndef ST_OSLINUX
    STOS_TaskLock();
    #ifdef PROCESSOR_C1 /***For C1 core this delay needed otherwise STTUNER_task_lock(); not working fine ****/ 
   /* task_delay(5);*/
    #endif
    #endif
    Data=STTUNER_IOREG_GetField(&(Instance->DeviceMap), Instance->IOHandle,  TPS_MODE);
    #ifndef ST_OSLINUX
    STOS_TaskUnlock();
    #endif
    /*Data = STTUNER_IOREG_FieldGetVal(&(Instance->DeviceMap), TPS_MODE);*/
    switch (Data)
    {
        case 0:  CurMode = STTUNER_MODE_2K; break;
        case 1:  CurMode = STTUNER_MODE_8K; break;
        default: 
        CurMode =Data;
        STTUNER_TaskDelay(5);
        Data=STTUNER_IOREG_GetField(&(Instance->DeviceMap), Instance->IOHandle,  TPS_MODE);
        switch (Data)
        {
        case 0:  CurMode = STTUNER_MODE_2K; break;
        case 1:  CurMode = STTUNER_MODE_8K; break;
        default:
        CurMode =Data;
        TunerError=ST_ERROR_BAD_PARAMETER;
        break;
        }
        break; /* error */
    }

    /* Get the Hierarchical Mode */
    #ifndef ST_OSLINUX
    STOS_TaskLock();
    #ifdef PROCESSOR_C1 /***For C1 core this delay needed otherwise STTUNER_task_lock(); not working fine ****/ 
    /*task_delay(5);*/
    #endif
    #endif
    Data=STTUNER_IOREG_GetField(&(Instance->DeviceMap), Instance->IOHandle,  TPS_HIERMODE);
    #ifndef ST_OSLINUX
    STOS_TaskUnlock();
    #endif
    switch(Data)
    {
    	case 0 : CurHierMode=STTUNER_HIER_ALPHA_NONE; break;
    	case 1 : CurHierMode=STTUNER_HIER_ALPHA_1; break;
    	case 2 : CurHierMode=STTUNER_HIER_ALPHA_2; break;
    	case 3 : CurHierMode=STTUNER_HIER_ALPHA_4; break;
    	default :
    	CurHierMode=Data;
    	TunerError=ST_ERROR_BAD_PARAMETER;
        break; /* error */
    }
    
     /* Get the FEC Rate */
    /*Data = STTUNER_IOREG_FieldGetVal(&(Instance->DeviceMap), TPS_HPCODE);*/
     #ifndef ST_OSLINUX
     STOS_TaskLock();
    #ifdef PROCESSOR_C1 /***For C1 core this delay needed otherwise STTUNER_task_lock(); not working fine ****/ 
    /*task_delay(5);*/
    #endif
    #endif
   if((Instance->Result).hier==STTUNER_HIER_LOW_PRIO)
    {
       Data=STTUNER_IOREG_GetField(&(Instance->DeviceMap), Instance->IOHandle,  TPS_LPCODE);
    
    }
    
    
    else
    {
    Data=STTUNER_IOREG_GetField(&(Instance->DeviceMap), Instance->IOHandle,  TPS_HPCODE);
    }
   #ifndef ST_OSLINUX
    STOS_TaskUnlock();
    #endif
    switch (Data)
    {
        case 0:  CurFECRates = STTUNER_FEC_1_2; break;
        case 1:  CurFECRates = STTUNER_FEC_2_3; break;
        case 2:  CurFECRates = STTUNER_FEC_3_4; break;
        case 3:  CurFECRates = STTUNER_FEC_5_6; break;
        case 4:  CurFECRates = STTUNER_FEC_7_8; break;
        default: 
        CurFECRates = Data;
        TunerError=ST_ERROR_BAD_PARAMETER;
        break; /* error */
    }

    /* Get the Guard time */
    #ifndef ST_OSLINUX
    STOS_TaskLock();
    #ifdef PROCESSOR_C1 /***For C1 core this delay needed otherwise STTUNER_task_lock(); not working fine ****/ 
    /*task_delay(5);*/
    #endif
    #endif
    Data = STTUNER_IOREG_GetField(&(Instance->DeviceMap), Instance->IOHandle,  TPS_GUARD);
    #ifndef ST_OSLINUX
    STOS_TaskUnlock();
    #endif
    switch (Data)
    {
        case 0:  CurGuard = STTUNER_GUARD_1_32; break;
        case 1:  CurGuard = STTUNER_GUARD_1_16; break;
        case 2:  CurGuard = STTUNER_GUARD_1_8;  break;
        case 3:  CurGuard = STTUNER_GUARD_1_4;  break;
        default: 
        CurGuard = Data;
        TunerError=ST_ERROR_BAD_PARAMETER;
        break; /* error */
    }

    /* Get the spectrum sense */
    Data = STTUNER_IOREG_GetField(&(Instance->DeviceMap), Instance->IOHandle,  INV_SPECTR);
    switch (Data)
    {
        case 0:  CurSpectrum = STTUNER_INVERSION;      break;
        default: CurSpectrum = STTUNER_INVERSION_NONE; break;
    }

    /* Get the correct frequency */
    CurFrequency = TunerInfo_p->Frequency;
    
    /********Frequency offset calculation done here*******************/
    signvalue=STTUNER_IOREG_GetField(&(Instance->DeviceMap), Instance->IOHandle, SEXT);
    offset=STTUNER_IOREG_GetRegister(&(Instance->DeviceMap), Instance->IOHandle, R_CRL_FREQ3);
    offset <<=16;
    offsetvar=STTUNER_IOREG_GetRegister(&(Instance->DeviceMap), Instance->IOHandle, R_CRL_FREQ2);
    offsetvar <<=8;
    offset = offset | offsetvar;
    offsetvar=0;
    offsetvar=STTUNER_IOREG_GetRegister(&(Instance->DeviceMap), Instance->IOHandle, R_CRL_FREQ1);
    offset =offset | offsetvar ;
    if(signvalue==1)
    {
       offset |=0xff000000;/*** Here sign extension made for the negative number**/
    }
       offset =offset /16384;/***offset calculation done according to data sheet of STV0360***/
       
       Data = STTUNER_IOREG_GetField(&(Instance->DeviceMap), Instance->IOHandle,  TPS_MODE);
       if(Data==0)
       {
          /*******it is 2k mode***************/
          offset=((offset*4464)/1000);/*** 1 FFT BIN=4.464khz***/
       }
       else
       {
          /*******it is 8k mode***************/
          offset=((offset*11)/10);/*** 1 FFT BIN=1.1khz***/
       }
       /*********** Spectrum Inversion Taken Care**************/
       Data = STTUNER_IOREG_GetField(&(Instance->DeviceMap), Instance->IOHandle,  INV_SPECTR);
       if((offset>=140) && (offset<=180))
       {
       	 offset -=166;
       	 if(Data ==0)
       	 {
       	    offset_type =STTUNER_OFFSET_POSITIVE;   
       	 }
       	 else
       	 {
              offset_type =STTUNER_OFFSET_NEGATIVE;
          }   
       }
       else if((offset<=-140) && (offset>=-180))
       {
       	 offset +=166;
       	 if(Data==0)
       	 {
             offset_type =STTUNER_OFFSET_NEGATIVE;
          }
          else
          {
             offset_type =STTUNER_OFFSET_POSITIVE;
          }
       }
       else
       {
       	 offset_type=STTUNER_OFFSET_NONE;
       }
       
       /*** After fine tunning with +/- 166 khz , a small offset in relation to +/-166khz
            is returned to user which it has to add with returned frequency to get the exact
            carrier frequency****/
       TunerInfo_p->ScanInfo.FreqOff = offset_type;
       if(Data==0)
       {
          TunerInfo_p->ScanInfo.ResidualOffset = offset;
       }
       else
       {
           TunerInfo_p->ScanInfo.ResidualOffset = offset*(-1);
       }
      
    /************************************************/

    /* Get the echo position */
    CurEchoPos=STTUNER_IOREG_GetField(&(Instance->DeviceMap), Instance->IOHandle,  LONG_ECHO);

    #ifndef ST_OSLINUX
    STOS_TaskLock();
    
    #ifdef PROCESSOR_C1 /***For C1 core this delay needed otherwise STTUNER_task_lock(); not working fine ****/ 
   /* task_delay(5);*/
    #endif
    #endif
    TunerInfo_p->FrequencyFound      = CurFrequency;
    TunerInfo_p->SignalQuality       = CurSignalQuality;
    TunerInfo_p->BitErrorRate        = CurBitErrorRate;
    TunerInfo_p->ScanInfo.Modulation = CurModulation;
    TunerInfo_p->ScanInfo.Mode       = CurMode;
    TunerInfo_p->ScanInfo.FECRates   = CurFECRates;
    TunerInfo_p->ScanInfo.Guard      = CurGuard;
    TunerInfo_p->ScanInfo.Spectrum   = CurSpectrum;
    TunerInfo_p->ScanInfo.EchoPos    = CurEchoPos;
    TunerInfo_p->Hierarchy_Alpha     = CurHierMode;
    Inst[Instance->TopLevelHandle].TunerInfoError = TunerError;
     TunerInfo_p->ScanInfo.Hierarchy =    (Instance->Result).hier;
    #ifndef ST_OSLINUX
    STOS_TaskUnlock();
    #endif
  
 #ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
    STTBX_Print(("F=%d SQ=%u BER=%u Modul=%u Mode=%u FR=%u G=%u Sp=%u\n",CurFrequency,CurSignalQuality,CurBitErrorRate,CurModulation,CurMode,CurFECRates,CurGuard,CurSpectrum));
 #endif
    


    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0360_GetSignalQuality()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0360_GetSignalQuality(DEMOD_Handle_t Handle, U32  *SignalQuality_p, U32 *Ber)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
   const char *identity = "STTUNER d0360.c demod_d0360_GetSignalQuality()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0360_InstanceData_t *Instance;

    /* private driver instance data */
    Instance = D0360_GetInstFromHandle(Handle);

    /* Read noise estimations for C/N and BER */
    FE_360_GetNoiseEstimator(&(Instance->DeviceMap), Instance->IOHandle,  SignalQuality_p, Ber);

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
    STTBX_Print(("%s SignalQuality=%u Ber=%u\n", identity, *SignalQuality_p, *Ber));
#endif
Error = Instance->DeviceMap.Error;

    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0360_GetModulation()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0360_GetModulation(DEMOD_Handle_t Handle, STTUNER_Modulation_t *Modulation)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
   const char *identity = "STTUNER d0360.c demod_d0360_GetModulation()";
#endif
    U8 Data;
    STTUNER_Modulation_t CurModulation;
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0360_InstanceData_t *Instance;

    /* private driver instance data */
    Instance = D0360_GetInstFromHandle(Handle);

    /* Get the modulation type */
     Data = STTUNER_IOREG_GetField(&(Instance->DeviceMap), Instance->IOHandle,  TPS_CONST);
   

    switch (Data)
    {
        case 0:
            CurModulation = STTUNER_MOD_QPSK;
            break;

        case 1:
            CurModulation = STTUNER_MOD_16QAM;
            break;

        case 2:
            CurModulation = STTUNER_MOD_64QAM;
            break;

        default:
            CurModulation = STTUNER_MOD_ALL;
            break;
    }

    *Modulation = CurModulation;

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
    STTBX_Print(("%s Modulation=%u\n", identity, *Modulation));
#endif

   


    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0360_GetMode()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0360_GetMode(DEMOD_Handle_t Handle, STTUNER_Mode_t *Mode)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
   const char *identity = "STTUNER d0360.c demod_d0360_GetMode()";
#endif
    U8 Data;
    STTUNER_Mode_t CurMode;
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0360_InstanceData_t *Instance;

    /* private driver instance data */
    Instance = D0360_GetInstFromHandle(Handle);

    /* Get the mode type */
    Data=STTUNER_IOREG_GetField(&(Instance->DeviceMap), Instance->IOHandle, TPS_MODE /*R_TPS_RCVD1*/);
   

    switch (Data)
    {
        case 0:
            CurMode = STTUNER_MODE_2K;
            break;

        case 1:
            CurMode = STTUNER_MODE_8K;
            break;

        default:
            CurMode = 0xff; /* error */
            break;
    }

    *Mode = CurMode;

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
    STTBX_Print(("%s Mode=%u\n", identity, *Mode));
#endif

    


    return(Error);
}

/* ----------------------------------------------------------------------------
Name: demod_d0360_GetRFLevel()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0360_GetRFLevel(DEMOD_Handle_t Handle, S32  *Rflevel)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
   const char *identity = "STTUNER d0360.c demod_d0360_GetRFLevel()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    U32 Data1=0;
    U32 Data2=0;
    D0360_InstanceData_t *Instance;
    U8	Rfregval;
    S32 RFleveldBm = 0;
          
    /* private driver instance data */
    Instance = D0360_GetInstFromHandle(Handle);
     
    /* Set the START_ADC field of register EN_RF_AGC1 to Enable the RF clock */
    STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle,START_ADC,0);
    STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle,START_ADC,1);
  
    /* Now read the RF_AGC1_LEVEL field of the register AGC1RF */
    Rfregval = STTUNER_IOREG_GetField(&(Instance->DeviceMap), Instance->IOHandle,  RF_AGC1_LEVEL);
    if (Rfregval >= 157)
    {
    	Data1 = STTUNER_IOREG_GetField(&(Instance->DeviceMap), Instance->IOHandle,  AGC2_VAL_LO);
   	Data2 = STTUNER_IOREG_GetField(&(Instance->DeviceMap), Instance->IOHandle,  AGC2_VAL_HI);   
    	Data2 <<=8;
    	Data2 &= 0x0F00;
    	Data2 |= Data1 ;
    	if ( Data2 >= 1035 && Data2 <= 1126)
   	 {
    		RFleveldBm = -((Data2 -1000)/25 + 42);
   	 }
   	 else if ( Data2 >= 1127 && Data2 <= 1201)
   	 {
    		RFleveldBm = -((Data2 -1000)/25 + 43);
    	 }
    	else if ( Data2 >= 1202 && Data2 <= 1259)
    	 {
    		RFleveldBm = -((Data2 -1000)/25 + 44);
     	 }
    
    	else if ( Data2 >= 1260 && Data2 <= 1340)
   	 {
    		RFleveldBm = -((Data2 -1000)/25 + 45);
   	 }    
   	else  
   	 {
   		RFleveldBm = STTUNER_LOW_RF;
   		STTBX_Print(("RF level is  < -60dBm \n"));	
    	}  
    }
   
    else if (Rfregval >= 143 && Rfregval <= 157 )
    	{
		RFleveldBm = -((Rfregval /8 )+ 24) ;
    	}
  
    else if (Rfregval >= 95 && Rfregval <= 142 )
    	{
    	      RFleveldBm = -((Rfregval - 100)/2 + 21) ;
   	} 
    else  
	{
    	     RFleveldBm = STTUNER_HIGH_RF;	
    	     STTBX_Print(("RF level is  > -20dBm \n"));
        }  
    *Rflevel = RFleveldBm;
    
    


    return(Error);
}




/* ----------------------------------------------------------------------------
Name: demod_d0360_GetTPSCellId()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */

ST_ErrorCode_t demod_d0360_GetTPSCellId(DEMOD_Handle_t Handle, U16  *TPSCellId)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
   const char *identity = "STTUNER d0360.c demod_d0360_GetTPSCellId()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0360_InstanceData_t *Instance;
    BOOL  Cell_ID_FOUND  ;
    int TPSCellIDTemp=0;
    U32 Counter=0;
    Cell_Id_Value cell_id;
    int Cell_ID1_MSB =0 ,Cell_ID2_MSB = 0 , Cell_ID1_LSB = 0 , Cell_ID2_LSB =0 ;
   
    Instance = D0360_GetInstFromHandle(Handle);
    Cell_ID_FOUND = FALSE ;
    do
    {
		
       Error=FE_360_GETCELLID(&(Instance->DeviceMap),Instance->IOHandle,&cell_id);	
       if (Error == ST_NO_ERROR)
       {
          Cell_ID1_MSB= cell_id.msb;
          Cell_ID1_LSB= cell_id.lsb;  
       }
       else
       {
          Error = ST_ERROR_TIMEOUT ;
          return(Error);
       }
       WAIT_N_MS(15);							
       Error=FE_360_GETCELLID(&(Instance->DeviceMap),Instance->IOHandle, &cell_id);
       if (Error == ST_NO_ERROR)
       {	
          Cell_ID2_MSB= cell_id.msb;
          Cell_ID2_LSB= cell_id.lsb; 
       }
       else
       {
          Error = ST_ERROR_TIMEOUT ;
          return(Error);
       } 							
       if(((Cell_ID2_MSB==Cell_ID1_MSB)&& (Cell_ID2_LSB==Cell_ID1_LSB)) )
       Cell_ID_FOUND=TRUE;
       Counter += 1;
        
    }while ((Cell_ID_FOUND==FALSE) && (Counter < 600));
 
            
      TPSCellIDTemp = Cell_ID1_MSB;
      TPSCellIDTemp <<= 8;
      TPSCellIDTemp |= Cell_ID1_LSB;
      *TPSCellId = TPSCellIDTemp;
       
   if (Counter >= 600)
       {
        Error = ST_ERROR_TIMEOUT;
       }
   


    return(Error);
}
/* ----------------------------------------------------------------------------
Name: demod_d0360_GetAGC()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0360_GetAGC(DEMOD_Handle_t Handle, S16  *Agc)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
   const char *identity = "STTUNER d0360.c demod_d0360_GetAGC()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    S16 Data1=0;
    S16 Data2=0;
    D0360_InstanceData_t *Instance;
     /** fix done here for the bug GNBvd20972 **
     ** where body of the function demod_d0360_GetAGC written **
     ** and the function now returns the core gain **/
        
    /* private driver instance data */
    Instance = D0360_GetInstFromHandle(Handle); 
    /* Get the mode type */
    Data1 = STTUNER_IOREG_GetField(&(Instance->DeviceMap), Instance->IOHandle,  AGC_GAIN_LO);
    
    /*Data1=STTUNER_IOREG_GetRegister(&(Instance->DeviceMap), Instance->IOHandle,  R_AGC_GAIN1);*/
       
    Data2 = STTUNER_IOREG_GetField(&(Instance->DeviceMap), Instance->IOHandle,  AGC_GAIN_HI);
   
   /* Data2=STTUNER_IOREG_GetRegister(&(Instance->DeviceMap), Instance->IOHandle,  R_AGC_GAIN2);*/    
    Data2 <<=8;
    Data2 &= 0x0F00;
    Data2 |= Data1 ;
    *Agc=Data2 ;
    
      


    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0360_GetFECRates()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0360_GetFECRates(DEMOD_Handle_t Handle, STTUNER_FECRate_t *FECRates)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
   const char *identity = "STTUNER d0360.c demod_d0360_GetFECRates()";
#endif
    U8 Data;
    STTUNER_FECRate_t CurFecRate;
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0360_InstanceData_t *Instance;

    /* private driver instance data */
    Instance = D0360_GetInstFromHandle(Handle);

    CurFecRate = 0;
    STTUNER_IOREG_GetField(&(Instance->DeviceMap), Instance->IOHandle,  TPS_HPCODE);
  
    Data = STTUNER_IOREG_GetField(&(Instance->DeviceMap),Instance->IOHandle, TPS_HPCODE);

    /* Convert venrate value to a STTUNER fecrate */
    switch (Data)
    {
        case 0:
            CurFecRate = STTUNER_FEC_1_2;
            break;

        case 1:
            CurFecRate = STTUNER_FEC_2_3;
            break;

        case 2:
            CurFecRate = STTUNER_FEC_3_4;
            break;

        case 3:
            CurFecRate = STTUNER_FEC_5_6;
            break;

        case 4:
            CurFecRate = STTUNER_FEC_7_8;
            break;

        default:
            CurFecRate = 0; /* error */
            break;
    }

    *FECRates = CurFecRate;  /* Copy back for caller */

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
    STTBX_Print(("%s FECRate=%u\n", identity, CurFecRate));
#endif

    


    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0360_GetGuard()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0360_GetGuard(DEMOD_Handle_t Handle, STTUNER_Guard_t *Guard)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
   const char *identity = "STTUNER d0360.c demod_d0360_GetGuard()";
#endif
    U8 Data;
    STTUNER_Guard_t CurGuard;
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0360_InstanceData_t *Instance;

    /* private driver instance data */
    Instance = D0360_GetInstFromHandle(Handle);

    CurGuard = 0;
    Data = STTUNER_IOREG_GetField(&(Instance->DeviceMap), Instance->IOHandle,  TPS_GUARD);

    /* Convert venrate value to a STTUNER fecrate */
    switch (Data)
    {
        case 0:
            CurGuard = STTUNER_GUARD_1_32;
            break;

        case 1:
            CurGuard = STTUNER_GUARD_1_16;
            break;

        case 2:
            CurGuard = STTUNER_GUARD_1_8;
            break;

        case 3:
            CurGuard = STTUNER_GUARD_1_4;
            break;

        default:
            CurGuard = 0xff; /* error */
            break;
    }

    *Guard = CurGuard;  /* Copy back for caller */

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
    STTBX_Print(("%s Guard=%u\n", identity, CurGuard));
#endif

   


    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0360_IsLocked()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0360_IsLocked(DEMOD_Handle_t Handle, BOOL *IsLocked)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
   const char *identity = "STTUNER d0360.c demod_d0360_IsLocked()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0360_InstanceData_t *Instance;

    /* private driver instance data */
    Instance = D0360_GetInstFromHandle(Handle);

    *IsLocked = STTUNER_IOREG_GetField(&(Instance->DeviceMap), Instance->IOHandle,  LK) ? TRUE : FALSE;


#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
    STTBX_Print(("%s IsLocked=%u\n", identity, *IsLocked));
#endif

   


    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0360_Tracking()

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
ST_ErrorCode_t demod_d0360_Tracking(DEMOD_Handle_t Handle, BOOL ForceTracking, U32 *NewFrequency, BOOL *SignalFound)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
   const char *identity = "STTUNER d0360.c demod_d0360_Tracking()";
#endif
    FE_360_Error_t           Error = ST_NO_ERROR;
    D0360_InstanceData_t    *Instance;
    

    Instance = D0360_GetInstFromHandle(Handle);

    

    FE_360_Tracking(&(Instance->DeviceMap), Instance->IOHandle,&(Instance->Result));

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
    STTBX_Print(("%s SignalFound=%u NewFrequency=%u\n", identity, *SignalFound, *NewFrequency));
#endif
    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0360_ScanFrequency()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0360_ScanFrequency   (DEMOD_Handle_t Handle,
                                            U32  InitialFrequency,
                                            U32  SymbolRate,
                                            U32  MaxOffset,
                                            U32  TunerStep,
                                            U8   DerotatorStep,
                                            BOOL *ScanSuccess,
                                            U32  *NewFrequency,
                                            U32  Mode,
                                            U32  Guard,
                                            U32  Force,
                                            U32  Hierarchy,
                                            U32  Spectrum,
                                            U32  FreqOff,
                                            U32  ChannelBW,
                                            S32  EchoPos)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
   const char *identity = "STTUNER d0360.c demod_d0360_ScanFrequency()";
#endif
    ST_ErrorCode_t              Error = ST_NO_ERROR;
    FE_360_Error_t error = FE_NO_ERROR;
    STTUNER_tuner_instance_t    *TunerInstance;
    STTUNER_InstanceDbase_t     *Inst;
    D0360_InstanceData_t        *Instance;
    
    FE_360_SearchParams_t       Search;
   
    /* private driver instance data */
    Instance = D0360_GetInstFromHandle(Handle);

    /* top level public instance data */
    Inst = STTUNER_GetDrvInst();

    /* get the tuner instance for this driver from the top level handle */
    TunerInstance = &Inst[Instance->TopLevelHandle].Terr.Tuner;

    




    Search.Frequency = (U32)(InitialFrequency);
    Search.Mode      = (STTUNER_Mode_t)Mode;
    Search.Guard     = (STTUNER_Guard_t)Guard;
    Search.Force     = (STTUNER_Force_t)Force;
    Search.Inv       = (STTUNER_Spectrum_t)Spectrum;
    Search.Offset    = (STTUNER_FreqOff_t)FreqOff;
    Search.ChannelBW = ChannelBW;
    Search.EchoPos   = EchoPos;
    Search.Hierarchy= (STTUNER_Hierarchy_t)Hierarchy;

    /** error checking is done here for the fix of the bug GNBvd20315 **/
    error=FE_360_LookFor(&(Instance->DeviceMap), Instance->IOHandle, &Search, &(Instance->Result), TunerInstance);
    if(error ==  FE_BAD_PARAMETER)
    {
       
       Error= ST_ERROR_BAD_PARAMETER;
      
       return Error;
    }
    
   else if (error == FE_SEARCH_FAILED)
    {
    	Error= ST_NO_ERROR;  
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
           STTBX_Print(("%s  FE_361_Search() == FE_361_SEARCH_FAILED\n", identity ));
#endif
	return Error;
    	
    }
    
     *ScanSuccess = (Instance->Result).Locked;
     if (*ScanSuccess == TRUE)
     {
     	*NewFrequency = (Instance->Result).Frequency;
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
        STTBX_Print(("%s NewFrequency=%u\t", identity, *NewFrequency));

#endif


#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
    STTBX_Print(("%s ScanSuccess=%u\n", identity, *ScanSuccess));
#endif

}
    return(ST_NO_ERROR);


}



/* ----------------------------------------------------------------------------
Name: demod_d0360_ioctl()

Description:
    access device specific low-level functions

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0360_ioctl(DEMOD_Handle_t Handle, U32 Function, void *InParams, void *OutParams, STTUNER_Da_Status_t *Status)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
   const char *identity = "STTUNER d0360.c demod_d0360_ioctl()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0360_InstanceData_t *Instance;
    STTUNER_InstanceDbase_t     *Inst;
Inst = STTUNER_GetDrvInst();

    /* private driver instance data */
    Instance = D0360_GetInstFromHandle(Handle);

    if(STTUNER_Util_CheckPtrNull(Instance) != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
        STTBX_Print(("%s invalid handle\n", identity));
#endif
        return(ST_ERROR_INVALID_HANDLE);
    }

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
        STTBX_Print(("%s demod driver instance handle=%d\n", identity, Handle));
        STTBX_Print(("%s                     function=%d\n", identity, Function));
        STTBX_Print(("%s                   I/O handle=%d\n", identity, Instance->IOHandle));
#endif

    /* ---------- select what to do ---------- */
    switch(Function){

        case STTUNER_IOCTL_REG_IN: /* read device register */
            *(int *)OutParams = STTUNER_IOREG_GetRegister(&(Instance->DeviceMap), Instance->IOHandle,  *(int *)InParams);
            break;

        case STTUNER_IOCTL_REG_OUT: /* write device register */
            Error =   STTUNER_IOREG_SetRegister(&(Instance->DeviceMap),Instance->IOHandle,
                  ((TERIOCTL_SETREG_InParams_t *)InParams)->RegIndex,
                  ((TERIOCTL_SETREG_InParams_t *)InParams)->Value );
            break;
	case STTUNER_IOCTL_SETTRL: /* set TRL registers through ioctl */
			Inst->TRL_IOCTL_Set_Flag=FALSE;
     			Error = demod_d0360_TRL_ioctl_set(&(Instance->DeviceMap),Instance->IOHandle,InParams);
			Inst->TRL_IOCTL_Set_Flag=TRUE;
			break;
			
	case STTUNER_IOCTL_GETTRL: /* get TRL registers through ioctl */
			Error = demod_d0360_TRL_ioctl_get(&(Instance->DeviceMap),Instance->IOHandle,OutParams);
			break;		
        default:
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
            STTBX_Print(("%s function %d not found\n", identity, Function));
#endif
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }


#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
    STTBX_Print(("%s called ok\n", identity, Function));    /* signal that function came back */
#endif

    


    return(Error);

}




/* ----------------------------------------------------------------------------
Name: demod_d0360_TRL_ioctl_set()

Description:
    access device specific low-level functions

Parameters:

Return Value:
---------------------------------------------------------------------------- */


ST_ErrorCode_t demod_d0360_TRL_ioctl_set(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,void *InParams)
   
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
   const char *identity = "STTUNER d0360.c demod_d0360_TRL_ioctl_set()";
#endif

       
    STTUNER_demod_TRL_IOCTL_t   *TRLSet;
	U8 trl_ctl[3]={0},R_trlctl;
	 ST_ErrorCode_t Error = ST_NO_ERROR;
	 
   TRLSet = (STTUNER_demod_TRL_IOCTL_t *)InParams;
  
   

R_trlctl=STTUNER_IOREG_GetRegister(DeviceMap,IOHandle,R_TRL_CTL);

trl_ctl[0]=((TRLSet->TRLNORMRATELSB)<<7) | (R_trlctl & 0x7f);
trl_ctl[1]=TRLSet->TRLNORMRATELO;
trl_ctl[2]=TRLSet->TRLNORMRATEHI;
 STTUNER_IOREG_SetContigousRegisters(DeviceMap,IOHandle,R_TRL_CTL,trl_ctl,3);	




#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
    STTBX_Print(("%s called ok\n", identity, Function));    /* signal that function came back */
#endif
    return(Error);

}


/* ----------------------------------------------------------------------------
Name: demod_d0360_TRL_ioctl_get()

Description:
    access device specific low-level functions

Parameters:

Return Value:
---------------------------------------------------------------------------- */

ST_ErrorCode_t demod_d0360_TRL_ioctl_get(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,void *OutParams)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
   const char *identity = "STTUNER d0360.c demod_d0360_TRL_ioctl_get()";
#endif
ST_ErrorCode_t Error = ST_NO_ERROR;
    
    STTUNER_demod_TRL_IOCTL_t   *TRLGet;
    
    
	    
   TRLGet = (STTUNER_demod_TRL_IOCTL_t *)OutParams;
   
  
 TRLGet->TRLNORMRATEHI= STTUNER_IOREG_GetField(DeviceMap,IOHandle,TRL_NOMRATE_HI);
 TRLGet->TRLNORMRATELO= STTUNER_IOREG_GetField(DeviceMap,IOHandle,TRL_NOMRATE_LO);
 TRLGet->TRLNORMRATELSB= STTUNER_IOREG_GetField(DeviceMap,IOHandle,TRL_NOMRATE_LSB) ;


#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
    STTBX_Print(("%s called ok\n", identity, Function));    /* signal that function came back */
#endif

    return(Error);

}





/* ----------------------------------------------------------------------------
Name: demod_d0360_ioaccess()

Description:
    called from ioarch.c when some driver does I/O but with the repeater/passthru
    set to point to this function.
Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0360_ioaccess(DEMOD_Handle_t Handle, IOARCH_Handle_t IOHandle, STTUNER_IOARCH_Operation_t Operation, U16 SubAddr, U8 *Data, U32 TransferSize, U32 Timeout)

{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
   const char *identity = "STTUNER d0360.c demod_d0360_ioaccess()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    IOARCH_Handle_t ThisIOHandle;
    D0360_InstanceData_t *Instance;
    STTUNER_InstanceDbase_t     *Inst;
    STTUNER_tuner_instance_t    *TunerInstance;
    
    Instance = D0360_GetInstFromHandle(Handle);
    
    Inst = STTUNER_GetDrvInst();

    /* get the tuner instance for this driver from the top level handle */
    TunerInstance = &Inst[Instance->TopLevelHandle].Terr.Tuner;

    if(STTUNER_Util_CheckPtrNull(Instance) != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
        STTBX_Print(("%s invalid handle\n", identity));
#endif
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* this (demod) drivers I/O handle */
    ThisIOHandle = Instance->IOHandle;

    /* if STTUNER_IOARCH_MAX_HANDLES then passthrough function required using our device handle/address */
    if (IOHandle == STTUNER_IOARCH_MAX_HANDLES)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
        STTBX_Print(("%s write passthru\n", identity));
#endif
        Error = STTUNER_IOARCH_ReadWrite(ThisIOHandle, Operation, SubAddr, Data, TransferSize, Timeout);
    }
    else    /* repeater */
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
        STTBX_Print(("%s write repeater\n", identity));
#endif
        #ifndef ST_OSLINUX
        STOS_TaskLock();    /* prevent any other activity on this bus (no task must preempt us) */
               /* enable repeater then send the data  using I/O handle supplied through ioarch.c */
        #ifdef PROCESSOR_C1 /***For C1 core this delay needed otherwise STTUNER_task_lock(); not working fine ****/ 
       /* task_delay(5); */
        #endif
        #endif
        Error = STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle,STOP_ENABLE, 1);
        Error = STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle,I2CT_ON, 1);

        /* send callers data using their address. (nb: subaddress == 0)
         Function 'STTUNER_IOARCH_ReadWriteNoRep' is called as calling the normal 'STTUNER_IOARCH_ReadWrite'
        function would cause it to call the redirection function which is THIS function, and around we
        would go forever. */
       
       
        	Error = STTUNER_IOARCH_ReadWriteNoRep(IOHandle, Operation, 0, Data, TransferSize, Timeout);
	
	#ifndef ST_OSLINUX
        STOS_TaskUnlock();
        #endif
    }

    return(Error);
}
/* ----------------------------------------------------------------------------
Name: demod_d0360_StandByMode()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */

ST_ErrorCode_t demod_d0360_StandByMode(DEMOD_Handle_t Handle, STTUNER_StandByMode_t PowerMode)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
   const char *identity = "STTUNER d0360.c demod_d0360_StandByMode()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0360_InstanceData_t *Instance;
       
    /* private driver instance data */
    Instance = D0360_GetInstFromHandle(Handle);
     
    switch ( PowerMode)
    {
       case STTUNER_NORMAL_POWER_MODE :
       if(Instance->StandBy_Flag == 1)
       {
          Error|= STTUNER_IOREG_SetRegister(&(Instance->DeviceMap),Instance->IOHandle,R_TOPCTRL,0x02);
       }
       Error|=STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle,CORE_ACTIVE,0);
       WAIT_N_MS(2);
       Error|=STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle,CORE_ACTIVE,1);
       
       if(Error==ST_NO_ERROR)
       {
          Instance->StandBy_Flag = 0 ;
       }
       
       break;
       case STTUNER_STANDBY_POWER_MODE :
       if(Instance->StandBy_Flag == 0)
       {
          Error|= STTUNER_IOREG_SetRegister(&(Instance->DeviceMap),Instance->IOHandle,R_TOPCTRL,0x82);
          if(Error==ST_NO_ERROR)
          {
             Instance->StandBy_Flag = 1 ;
          }
       }
       break ;
    }/* Switch statement */ 
  
    
       

   


    return(Error);
}


/* ------------------------------------------------------------------------- */
/* /\/\/\/\/\/\/\/\/\/\/\/\/\UTILITY Functions/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ */
/* ------------------------------------------------------------------------- */

D0360_InstanceData_t *D0360_GetInstFromHandle(DEMOD_Handle_t Handle)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360_HANDLES
   const char *identity = "STTUNER d0360.c D0360_GetInstFromHandle()";
#endif
    D0360_InstanceData_t *Instance;

    Instance = (D0360_InstanceData_t *)Handle;
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360_HANDLES
    STTBX_Print(("%s block at 0x%08x\n", identity, Instance));
#endif

    return(Instance);
}




/* End of d0360.c */
