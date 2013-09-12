/* ----------------------------------------------------------------------------
File Name: d0mini360.c

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

/* C libs */
#include <string.h>

#include "stlite.h"     /* Standard includes */

/* STAPI */
#include "sttbx.h"

#include "sttuner.h"

#include "sysdbase.h"
#include "stcommon.h" 

/* LLA */
#include "360_echo.h"
#include "360_init.h"    /* register mappings for the stv0360 */
#include "360_drv.h"    /* misc driver functions */
#include "d0360.h"      /* header for this file */
#include "360_map.h"

#include "tioctl.h"     /* data structure typedefs for all the ter ioctl functions */
#include "iodirect.h"    
/*******Never change the order of this array elements as it is accordance with******
******360_init.c settings of register.*********************/
/****** Register settings for DemodSTV0360*************/
#ifdef STTUNER_TERR_TMM_6MHZ
const U8 Def360Val[STV360_NBREGS]={
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
const U8 Def360Val[STV360_NBREGS]={
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
const U8 Def360Val[STV360_NBREGS]={
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
const U8 Def360Val[STV360_NBREGS]={
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

#ifdef STTUNER_MINIDRIVER
const U8 SubAddrIndex[13]={
0x00,0x10,0x26,0x31,0x80,0xCB,0xD4,0x40,0x49,0x56,0x54,0xC0,0xD0 };

const U8 SizeIndex[13]={
14,20,6,2,64,5,2,8,11,10,2,11,4 }; 
#endif        
/* Private types/constants ------------------------------------------------ */

#define ANALOG_CARRIER_DETECT_SYMBOL_RATE   5000000
#define ANALOG_CARRIER_DETECT_AGC2_VALUE    25

/* Device capabilities */
#define MAX_AGC                         4095 /*fix for DDTS 33979 "AGC calculations for STV0360"*/
#define MAX_SIGNAL_QUALITY              100
#define MAX_BER                         200000
#define STCHIP_HANDLE(x) ((STCHIP_InstanceData_t *)x)

#ifdef ST_OS21
#define WAIT_N_MS(X)     task_delay( (signed int)(X * (ST_GetClocksPerSecond() / 1000)) )   /*task_delay(X)*/
#else
#define WAIT_N_MS(X)     task_delay( (unsigned int)(X * (ST_GetClocksPerSecond() / 1000)) )   /*task_delay(X)*/
#endif

/* private variables ------------------------------------------------------- */
#ifdef ST_OS21
static semaphore_t *Lock_InitTermOpenClose; /* guard calls to the functions */
#else
static semaphore_t Lock_InitTermOpenClose; /* guard calls to the functions */
#endif
/*ifndef STTUNER_MINIDRIVER
static BOOL        Installed = FALSE;	*/



/* instance chain, the default boot value is invalid, to catch errors */
/* static D0360_InstanceData_t *InstanceChainTop = (D0360_InstanceData_t *)0x7fffffff;
endif	  */

D0360_InstanceData_t *DEMODInstance;

/* functions --------------------------------------------------------------- */

int FE_360_PowOf2(int number);

/* API */
/*ST_ErrorCode_t demod_d0360_Init(ST_DeviceName_t *DeviceName, DEMOD_InitParams_t *InitParams);
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
                                            S32  EchoPos);*/
ST_ErrorCode_t demod_d0360_ioctl           (DEMOD_Handle_t Handle, U32 Function, void *InParams, void *OutParams,
                                             STTUNER_Da_Status_t *Status);
ST_ErrorCode_t demod_d0360_GetRFLevel(DEMOD_Handle_t Handle, S32  *Rflevel);

ST_ErrorCode_t demod_d0360_GetTPSCellId(DEMOD_Handle_t Handle, U16  *TPSCellId);

/* For STANDBY API mode */ 
ST_ErrorCode_t demod_d0360_StandByMode(DEMOD_Handle_t Handle, STTUNER_StandByMode_t PowerMode);

/* I/O API */
ST_ErrorCode_t demod_d0360_ioaccess(DEMOD_Handle_t Handle, IOARCH_Handle_t IOHandle,
                  STTUNER_IOARCH_Operation_t Operation, U16 SubAddr, U8 *Data, U32 TransferSize, U32 Timeout);

/* local functions --------------------------------------------------------- */

D0360_InstanceData_t *D0360_GetInstFromHandle(DEMOD_Handle_t Handle);


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
    #ifdef ST_OS21    
	    Lock_InitTermOpenClose = semaphore_create_fifo(1);
	#else
	    semaphore_init_fifo(&Lock_InitTermOpenClose, 1);
	#endif   
/* now safe to lock semaphore */
    SEM_LOCK(Lock_InitTermOpenClose);
    DEMODInstance = memory_allocate_clear(InitParams->MemoryPartition, 1, sizeof( D0360_InstanceData_t ));
    if (DEMODInstance == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
        STTBX_Print(("%s fail memory allocation InstanceNew\n", identity));
#endif    
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_NO_MEMORY);           
    }

    /*DEMODInstance->DeviceName          = DeviceName;*/
    DEMODInstance->TopLevelHandle      = STTUNER_MAX_HANDLES;
    DEMODInstance->IOHandle            = InitParams->IOHandle;
    DEMODInstance->MemoryPartition     = InitParams->MemoryPartition;
    /*DEMODInstance->InstanceChainNext   = NULL; Checknab*/ /* always last in the chain */

    DEMODInstance->ExternalClock       = InitParams->ExternalClock;
    DEMODInstance->TSOutputMode        = InitParams->TSOutputMode;
    DEMODInstance->SerialDataMode      = InitParams->SerialDataMode;
    DEMODInstance->SerialClockSource   = InitParams->SerialClockSource;
    DEMODInstance->FECMode             = InitParams->FECMode;
    DEMODInstance->ClockPolarity       = InitParams->ClockPolarity;


    
    switch(DEMODInstance->TSOutputMode)
    {
        case STTUNER_TS_MODE_DEFAULT:
        case STTUNER_TS_MODE_PARALLEL:
            DEMODInstance->FE_360_InitParams.Clock = FE_PARALLEL_CLOCK;
            break;

        case STTUNER_TS_MODE_SERIAL:
            switch(DEMODInstance->SerialClockSource) /* Set serial clock source */
            {
                case STTUNER_SCLK_VCODIV6:
                    DEMODInstance->FE_360_InitParams.Clock = FE_SERIAL_VCODIV6_CLOCK;
                    break;

                default:
                case STTUNER_SCLK_DEFAULT:
                case STTUNER_SCLK_MASTER:
                    DEMODInstance->FE_360_InitParams.Clock = FE_SERIAL_MASTER_CLOCK;
                    break;
            }

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
       /* now safe to lock semaphore */
    SEM_LOCK(Lock_InitTermOpenClose);

    memory_deallocate(DEMODInstance->MemoryPartition, DEMODInstance);

    SEM_UNLOCK(Lock_InitTermOpenClose);
	    #ifdef ST_OS21
	    semaphore_delete(Lock_InitTermOpenClose);
	    #else
	    semaphore_delete(&Lock_InitTermOpenClose);
	    #endif 
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
 /*   D0360_InstanceData_t     *Instance;*/
      U32  ChipID; 
    U8 RegWriteindex=0;
    U8 * Def360ValTemp ;

    /* now safe to lock semaphore */
    SEM_LOCK(Lock_InitTermOpenClose);


    
    /* check handle IS actually free */
    if(DEMODInstance->TopLevelHandle != STTUNER_MAX_HANDLES)
    {
		SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_OPEN_HANDLE);
    }
    /* else now got pointer to free (and valid) data block */
     Def360ValTemp= (U8*)Def360Val;
     for(RegWriteindex=0 ; RegWriteindex < 13 ; RegWriteindex++)
     {
     
      STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, SubAddrIndex[RegWriteindex], 0, 0, Def360ValTemp , SizeIndex[RegWriteindex], FALSE);  
      Def360ValTemp +=  SizeIndex[RegWriteindex];
        
     }
     DEMODInstance->FE_360_Handle = FE_360_Init();
	
	 ChipID = ChipDemodGetField(  R_ID );
    if ( (ChipID & 0x30) ==  0x00)   /* Accept cuts 1 & 2 (0x10 or 0x20) */
    {
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_UNKNOWN_DEVICE);
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
        STTBX_Print(("%s device found, release/revision=%u\n", identity, ChipID & 0x0F));
#endif
    }

    /* Set serial/parallel data mode */
    if (DEMODInstance->TSOutputMode == STTUNER_TS_MODE_SERIAL)
    {
        ChipDemodSetField( OUTRS_SP, 1);
    }
    else
    {
        ChipDemodSetField( OUTRS_SP, 0);
    }
    
    /*set data clock polarity mode (rising/falling)*/
    switch(DEMODInstance->ClockPolarity)
    {
       case STTUNER_DATA_CLOCK_POLARITY_RISING:
    	 ChipDemodSetField( CLK_POL, 1);
    	 break;
       case STTUNER_DATA_CLOCK_POLARITY_FALLING:
    	 ChipDemodSetField( CLK_POL, 0);
    	 break;
       case STTUNER_DATA_CLOCK_POLARITY_DEFAULT:
    	 ChipDemodSetField( CLK_POL, 1);
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


    /* finally (as nor more errors to check for, allocate handle */
    DEMODInstance->TopLevelHandle = OpenParams->TopLevelHandle;
    *Handle = (U32)DEMODInstance;

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
   
	DEMODInstance->TopLevelHandle = STTUNER_MAX_HANDLES;
	FE_360_Term();
	return(ST_NO_ERROR);
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
   /* D0360_InstanceData_t *Instance;*/
    STTUNER_InstanceDbase_t     *Inst;
    U8                   Data;
    U32                  CurFrequency, CurSignalQuality, CurBitErrorRate;
    STTUNER_Modulation_t CurModulation;
    STTUNER_Mode_t       CurMode;
    STTUNER_FECRate_t    CurFECRates;
    STTUNER_Guard_t      CurGuard;
    STTUNER_Spectrum_t   CurSpectrum;
    STTUNER_Hierarchy_t  CurHierMode;
    S32                  CurEchoPos;
    int offset=0;
    int offsetvar=0;
    char signvalue=0;
    int offset_type=0;
    /* private driver instance data 
    Instance = D0360_GetInstFromHandle(Handle);*/
    
    /* top level public instance data */
    Inst = STTUNER_GetDrvInst(); 

    /* Read noise estimations for C/N and BER */
    FE_360_GetNoiseEstimator( &CurSignalQuality, &CurBitErrorRate); /* checknab temp*/
  
  /******The Error field of TunerInfo in Instaneous database is updated ******/
    
    TunerError= ST_NO_ERROR; 
       
    /*************************************************************************/
    /* Get the modulation type */
    STTUNER_task_lock();
    #ifdef PROCESSOR_C1 /***For C1 core this delay needed otherwise STTUNER_task_lock(); not working fine ****/ 
    task_delay(5);
    #endif
    Data=ChipDemodGetField( TPS_CONST);
    STTUNER_task_unlock();
   /* Data = ChipGetFieldImage(FE2CHIP(Instance->FE_360_Handle), TPS_CONST);*/
    switch (Data)
    {
        case 0:  CurModulation = STTUNER_MOD_QPSK;  break;
        case 1:  CurModulation = STTUNER_MOD_16QAM; break;
        case 2:  CurModulation = STTUNER_MOD_64QAM; break;
        default: 
        /*CurModulation = STTUNER_MOD_ALL;*/
        CurModulation = Data ;
        task_delay(5);
        Data=ChipDemodGetField( TPS_CONST);
        switch (Data)
        {
        case 0:  CurModulation = STTUNER_MOD_QPSK;  break;
        case 1:  CurModulation = STTUNER_MOD_16QAM; break;
        case 2:  CurModulation = STTUNER_MOD_64QAM; break;
        default:
        CurModulation = Data ;
        TunerError=ST_ERROR_BAD_PARAMETER;
        }
        break;
    }

    /* Get the mode */
    STTUNER_task_lock();
    #ifdef PROCESSOR_C1 /***For C1 core this delay needed otherwise STTUNER_task_lock(); not working fine ****/ 
    task_delay(5);
    #endif
    Data=ChipDemodGetField( TPS_MODE);
    STTUNER_task_unlock();
    /*Data = ChipGetFieldImage(FE2CHIP(Instance->FE_360_Handle), TPS_MODE);*/
    switch (Data)
    {
        case 0:  CurMode = STTUNER_MODE_2K; break;
        case 1:  CurMode = STTUNER_MODE_8K; break;
        default: 
        CurMode =Data;
        task_delay(5);
        Data=ChipDemodGetField( TPS_MODE);
        switch (Data)
        {
        case 0:  CurMode = STTUNER_MODE_2K; break;
        case 1:  CurMode = STTUNER_MODE_8K; break;
        default:
        CurMode =Data;
        TunerError=ST_ERROR_BAD_PARAMETER;
        }
        break; /* error */
    }

    /* Get the Hierarchical Mode */
    STTUNER_task_lock();
    #ifdef PROCESSOR_C1 /***For C1 core this delay needed otherwise STTUNER_task_lock(); not working fine ****/ 
    task_delay(5);
    #endif
    Data=ChipDemodGetField ( TPS_HIERMODE);
   STTUNER_task_unlock();
    switch(Data)
    {
    	case 0 : CurHierMode=STTUNER_HIER_NONE; break;
    	case 1 : CurHierMode=STTUNER_HIER_1; break;
    	case 2 : CurHierMode=STTUNER_HIER_2; break;
    	case 3 : CurHierMode=STTUNER_HIER_4; break;
    	default :
    	CurHierMode=Data;
    	TunerError=ST_ERROR_BAD_PARAMETER;
        break; /* error */
    }
    
     /* Get the FEC Rate */
    /*Data = ChipGetFieldImage(FE2CHIP(Instance->FE_360_Handle), TPS_HPCODE);*/
    STTUNER_task_lock();
    #ifdef PROCESSOR_C1 /***For C1 core this delay needed otherwise STTUNER_task_lock(); not working fine ****/ 
    task_delay(5);
    #endif
    Data=ChipDemodGetField( TPS_HPCODE);
    STTUNER_task_unlock();
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
    STTUNER_task_lock();
    #ifdef PROCESSOR_C1 /***For C1 core this delay needed otherwise STTUNER_task_lock(); not working fine ****/ 
    task_delay(5);
    #endif
    Data = ChipDemodGetField( TPS_GUARD);
    STTUNER_task_unlock();
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
    Data = ChipDemodGetField( INV_SPECTR);
    switch (Data)
    {
        case 0:  CurSpectrum = STTUNER_INVERSION;      break;
        default: CurSpectrum = STTUNER_INVERSION_NONE; break;
    }

    /* Get the correct frequency */
    CurFrequency = TunerInfo_p->Frequency;
    
    /********Frequency offset calculation done here*******************/
    signvalue=ChipDemodGetField(SEXT);
    offset=ChipDemodGetField(R_CRL_FREQ3);
    offset <<=16;
    offsetvar=ChipDemodGetField(R_CRL_FREQ2);
    offsetvar <<=8;
    offset = offset | offsetvar;
    offsetvar=0;
    offsetvar=ChipDemodGetField(R_CRL_FREQ1);
    offset =offset | offsetvar ;
    if(signvalue==1)
    {
       offset |=0xff000000;/*** Here sign extension made for the negative number**/
    }
       offset =offset /16384;/***offset calculation done according to data sheet of STV0360***/
      /* Data = ChipFieldImage(FE2CHIP(Instance->FE_360_Handle), TPS_MODE);*/
		Data = ChipDemodGetField(TPS_MODE);
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
       Data = ChipDemodGetField(INV_SPECTR);
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
    CurEchoPos=ChipDemodGetField( LONG_ECHO);

    STTUNER_task_lock();
    #ifdef PROCESSOR_C1 /***For C1 core this delay needed otherwise STTUNER_task_lock(); not working fine ****/ 
    task_delay(5);
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
    TunerInfo_p->ScanInfo.Hierarchy  = CurHierMode;
    Inst[DEMODInstance->TopLevelHandle].TunerInfoError = TunerError;
    STTUNER_task_unlock();
  
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
    
    /* private driver instance data */
    /* Read noise estimations for C/N and BER */
    FE_360_GetNoiseEstimator( SignalQuality_p, Ber);

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
    STTBX_Print(("%s SignalQuality=%u Ber=%u\n", identity, *SignalQuality_p, *Ber));
#endif
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
    Data =ChipDemodGetField( TPS_CONST);
   /* Data = ChipGetFieldImage( TPS_CONST);*/

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
    Data=ChipDemodGetField(TPS_MODE /*R_TPS_RCVD1*/);
   /* Data = ChipGetFieldImage(FE2CHIP(Instance->FE_360_Handle), TPS_MODE);*/

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

    /*Resolve Bug GNBvd17801 - For proper I2C error handling inside the driver*/
    
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
   /* D0360_InstanceData_t *Instance;*/
     /** fix done here for the bug GNBvd20972 **
     ** where body of the function demod_d0360_GetAGC written **
     ** and the function now returns the core gain **/
        
    /* private driver instance data */
  /*  Instance = D0360_GetInstFromHandle(Handle);*/ 
    /* Get the mode type */
    Data1 = ChipDemodGetField(AGC_GAIN_LO);
    
    /*Data1=ChipGetOneRegister(FE2CHIP(Instance->FE_360_Handle), R_AGC_GAIN1);*/
       
    Data2 = ChipDemodGetField(AGC_GAIN_HI);
   
   /* Data2=ChipGetOneRegister(FE2CHIP(Instance->FE_360_Handle), R_AGC_GAIN2);*/    
    Data2 <<=8;
    Data2 &= 0x0F00;
    Data2 |= Data1 ;
    *Agc=Data2 ;
    
    /*Resolve Bug GNBvd17801 - For proper I2C error handling inside the driver*/
    /*Error |= STCHIP_HANDLE(FE2CHIP(DEMODInstance->FE_360_Handle))->Chip.Error;
    STCHIP_HANDLE(FE2CHIP(DEMODInstance->FE_360_Handle))->Chip.Error = CHIPERR_NO_ERROR;   */
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
   /* D0360_InstanceData_t *Instance;*/

    /* private driver instance data */
  /*  Instance = D0360_GetInstFromHandle(Handle);*/

    CurFecRate = 0;
    Data =ChipDemodGetField( TPS_HPCODE);
    /* ChipGetFieldImage(FE2CHIP(Instance->FE_360_Handle), TPS_HPCODE);*/


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

    /*Resolve Bug GNBvd17801 - For proper I2C error handling inside the driver*/
   /* Error |= STCHIP_HANDLE(FE2CHIP(DEMODInstance->FE_360_Handle))->Chip.Error;
    STCHIP_HANDLE(FE2CHIP(DEMODInstance->FE_360_Handle))->Chip.Error = CHIPERR_NO_ERROR;*/
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
    Data = ChipDemodGetField( TPS_GUARD);

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

    /*Resolve Bug GNBvd17801 - For proper I2C error handling inside the driver*/
   /* Error |= STCHIP_HANDLE(FE2CHIP(Instance->FE_360_Handle))->Chip.Error;
    STCHIP_HANDLE(FE2CHIP(Instance->FE_360_Handle))->Chip.Error = CHIPERR_NO_ERROR;*/
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
   /* D0360_InstanceData_t *Instance;*/

    /* private driver instance data */
   /* Instance = D0360_GetInstFromHandle(Handle);*/

    *IsLocked = ChipDemodGetField( LK) ? TRUE : FALSE;


#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
    STTBX_Print(("%s IsLocked=%u\n", identity, *IsLocked));
#endif

    /*Resolve Bug GNBvd17801 - For proper I2C error handling inside the driver*/
 /*   Error |= STCHIP_HANDLE(FE2CHIP(DEMODInstance->FE_360_Handle))->Chip.Error;
    STCHIP_HANDLE(FE2CHIP(DEMODInstance->FE_360_Handle))->Chip.Error = CHIPERR_NO_ERROR;*/
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
    /*D0360_InstanceData_t    *Instance;*/
    FE_360_InternalParams_t *pInternalParams;

   /* Instance = D0360_GetInstFromHandle(Handle);*/

    pInternalParams = (FE_360_InternalParams_t *)DEMODInstance->FE_360_Handle;

    FE_360_Tracking(pInternalParams);

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
   /* D0360_InstanceData_t        *Instance;*/
    FE_360_InternalParams_t     *pInternalParams;
    FE_360_SearchParams_t       Search;
    FE_360_SearchResult_t       Result;
    /* private driver instance data */
  /*  Instance = D0360_GetInstFromHandle(Handle);*/

    /* top level public instance data */
    Inst = STTUNER_GetDrvInst(); 
    
    /* get the tuner instance for this driver from the top level handle */
    TunerInstance = &Inst[DEMODInstance->TopLevelHandle].Terr.Tuner;

    pInternalParams            = (FE_360_InternalParams_t *)DEMODInstance->FE_360_Handle;
    /*pInternalParams->hChip     = FE2CHIP(Instance->FE_360_Handle);   */
    pInternalParams->Quartz    = (FE_360_Quarz_t)ChipDemodGetField( ENA_27);
    /*pInternalParams->Tuner     = TunerInstance->Driver->ID;	 */
    /******* For Minituner tuner type added here for Driver layer***************/
    pInternalParams->Tuner= STTUNER_TUNER_DTT7592;
    Search.Frequency = (U32)(InitialFrequency);
    Search.Mode      = (STTUNER_Mode_t)Mode;
    Search.Guard     = (STTUNER_Guard_t)Guard;
    Search.Force     = (STTUNER_Force_t)Force;
    Search.Inv       = (STTUNER_Spectrum_t)Spectrum;
    Search.Offset    = (STTUNER_FreqOff_t)FreqOff;
    Search.ChannelBW = ChannelBW;
    Search.EchoPos   = EchoPos;

    /** error checking is done here for the fix of the bug GNBvd20315 **/
    error=FE_360_LookFor(pInternalParams, &Search, &Result, TunerInstance);
    if(error != FE_NO_ERROR)
    {
       
       Error= ST_ERROR_BAD_PARAMETER;
      
       return Error;
    }
    
    if (Result.SignalStatus == LOCK_OK)
    {
    	  
        Echo_Init ( &(pInternalParams->Echo));
        /* Pass new frequency to caller */
        *NewFrequency = Result.Frequency/*InitialFrequency*/;
        

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
        STTBX_Print(("%s NewFrequency=%u\n", identity, *NewFrequency));
#endif
    }

    *ScanSuccess = (Result.SignalStatus == LOCK_OK);

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0360
    STTBX_Print(("%s ScanSuccess=%u\n", identity, *ScanSuccess));
#endif

    
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
