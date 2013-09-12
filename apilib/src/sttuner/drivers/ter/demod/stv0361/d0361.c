/* ----------------------------------------------------------------------------
File Name: d0361.c

Description:

    stv0361 demod driver.


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
#ifndef ST_OSLINUX
 
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
/*#include "chip.h"*/

/* LLA */
#include "361_echo.h"

#include "361_drv.h"    /* misc driver functions */
#include "d0361.h"      /* header for this file */
#include "361_map.h"

#include "tioctl.h"     /* data structure typedefs for all the ter ioctl functions */

#define WAIT_N_MS(X)     STOS_TaskDelayUs(X * 1000)  


U16 STV0361_address[STV361_NBREGS]=
{
0x0000,
0x0001,
0x0002,
0x0003,
0x0004,
0x0005,
0x0006,
0x0007,
0x0008,
0x0009,
0x000a,
0x000b,
0x000c,
0x000d,
0x0010,
0x0011,
0x0012,
0x0013,
0x0014,
0x0015,
0x0016,
0x0017,
0x0018,
0x0019,
0x001a,
0x001b,
0x001c,
0x001d,
0x001e,
0x001f,
0x0020,
0x0021,
0x0022,
0x0023,
0x0024,
0x0025,
0x0026,
0x0027,
0x0028,
0x0029,
0x002a,
0x002b,
0x002c,
0x002d,
0x002e,
0x002f,
0x0030,
0x0031,
0x0032,
0x0033,
0x0034,
0x0035,
0x0036,
0x0080,
0x0081,
0x0082,
0x0083,
0x0084,
0x0085,
0x0086,
0x0087,
0x0088,
0x0089,
0x008a,
0x008b,
0x008c,
0x008d,
0x008e,
0x008f,
0x0090,
0x0091,
0x0092,
0x0093,
0x0094,
0x0095,
0x0096,
0x0097,
0x0098,
0x0099,
0x009a,
0x009b,
0x009c,
0x009d,
0x009e,
0x009f,
0x00a0,
0x00a1,
0x00a2,
0x00a3,
0x00a4,
0x00a5,
0x00a6,
0x00a7,
0x00a8,
0x00a9,
0x00aa,
0x00ab,
0x00ac,
0x00ad,
0x00ae,
0x00af,
0x00b0,
0x00b1,
0x00b2,
0x00b3,
0x00b4,
0x00b5,
0x00b6,
0x00b7,
0x00b8,
0x00b9,
0x00ba,
0x00bb,
0x00bc,
0x00bd,
0x00be,
0x00bf,
0x0040,
0x0041,
0x0042,
0x0043,
0x0044,
0x0045,
0x0046,
0x0047,
0x0049,
0x004a,
0x004b,
0x004c,
0x004d,
0x004e,
0x004f,
0x0050,
0x0051,
0x0052,
0x0053,
0x0054,
0x0055,
0x0056,
0x0057,
0x0058,
0x0059,
0x005a,
0x005b,
0x005c,
0x005d,
0x005e,
0x005f,
0x00c0,
0x00c1,
0x00c2,
0x00c3,
0x00c4,
0x00c5,
0x00c6,
0x00c7,
0x00c8,
0x00c9,
0x00ca,
0x00cb,
0x00cc,
0x00cd,
0x00ce,
0x00cf,
0x00d0,
0x00d1,
0x00d2,
0x00d3,
0x00d4,
0x00d5,
0x00d6,
0x00db,
0x00d8,
0x00d9,
0x00da,
0x00d7,
0x00dc
};



/*******Never change the order of this array elements as it is accordance with******
******361_init.c settings of register.*********************/
/****** Register settings for DemodSTV0361*************/
#ifdef STTUNER_TERR_TMM_6MHZ
U8 Def361Val[STV361_NBREGS]={
/*R_ID*/             0x30, 
/*R_I2CRPT*/         0x22,
/*R_TOPCTRL*/        0x02,
/*R_IOCFG0*/         0x40,
/*R_DAC0R*/          0x00,


/*R_IOCFG1*/         0x40,
/*R_DAC1R*/          0x00,
/*R_IOCFG2*/         0x80,
/*R_SDFR*/           0x00,
/*R_STATUS*/         0xF9,


/*R_AUX_CLK*/        0x1C,
/*R_RESERVED_1*/     0x00,
/*R_RESERVED_2*/     0x00,
/*R_RESERVED_3*/     0x00,
/*R_AGC2MAX*/        0x8C,


/*R_AGC2MIN*/        0x14,
/*R_AGC1MAX*/        0xFF,
/*R_AGC1MIN*/        0x6B,
/*R_AGCR*/           0xBC,
/*R_AGC2TH*/         0x50,


/*R_AGC12C*/         0x40,
/*R_AGCCTRL1*/       0x85,
/*R_AGCCTRL2*/       0x18,
/*R_AGC1VAL1*/       0xFF,
/*R_AGC1VAL2*/       0x0F,



/*R_AGC2VAL1*/       0xFA,
/*R_AGC2VAL2*/       0x01,
/*R_AGC2PGA*/        0x00,
/*R_OVF_RATE1*/      0x00,
/*R_OVF_RATE2*/      0x00,


/*R_GAIN_SRC1*/      0xFD,
/*R_GAIN_SRC2*/      0xAC,
/*R_INC_DEROT1*/     0x55,
/*R_INC_DEROT2*/     0x4B,
/*R_PPM_CPAMP_DIR*/  0x2C, /*0x5D*/



/*R_PPM_CPAMP_INV*/  0x00,
/*R_FREESTFE_1*/     0x03,
/*R_FREESTFE_2*/     0x1C,
/*R_DCOFFSET*/       0x06,
/*R_EN_PROCESS*/     0x01,
/*R_RESERVED_4*/     0xFF,


/*R_RESERVED_5*/     0x00,
/*R_RESERVED_6*/     0x80,
/*R_RESERVED_7*/     0x1D,
/*R_RESERVED_8*/     0x29,
/*R_RESERVED_9*/     0x14,


/*R_RESERVED_10*/    0xFE,
/*R_EPQ*/            0x02,
/*R_EPQAUTO*/        0x01,
/*R_CHP_TAPS*/       0x03,/* 0x01 for 2k mode and 0x03 for 8K mode*/
/*R_CHP_DYN_COEFF*/  0x00,


/*R_PPM_STATE_MAC*/  0x23,
/*R_INR_THRESHOLD*/  0x09,
/*R_COR_CTL*/        0x20,
/*R_COR_STAT*/       0xF6,
/*R_COR_INTEN*/      0x00,


/*R_COR_INTSTAT*/    0x3D,
/*R_COR_MODEGUARD*/  0x00,
/*R_AGC_CTL*/        0x18,
/*R_RESERVED_11*/    0x00,
/*R_RESERVED_12*/    0x00,


/*R_AGC_TARGET*/     0x1E,
/*R_AGC_GAIN1*/      0xFB,
/*R_AGC_GAIN2*/      0x1A,
/*R_RESERVED_13*/    0x00,
/*R_RESERVED_14*/    0x00,


/*R_RESERVED_15*/    0x00,
/*R_CAS_CTL*/        0x40,
/*R_CAS_FREQ*/       0xB3,
/*R_CAS_DAGCGAIN*/   0x10,
/*R_SYR_CTL*/        0x04,


/*R_SYR_STAT*/       0x13,
/*R_RESERVED_16*/    0x00,
/*R_RESERVED_17*/    0x00,
/*R_SYR_OFFSET1*/    0x00,
/*R_SYR_OFFSET2*/    0x00,


/*R_RESERVED_18*/    0x00,
/*R_SCR_CTL*/        0x00,
/*R_PPM_CTL1*/       0x38,
/*R_TRL_CTL*/        0x94,/* 0x14 */
/*R_TRL_NOMRATE1*/   0xAB, /*0xAC*/



/*R_TRL_NOMRATE2*/   0x56,
/*R_TRL_TIME1*/      0x3C,
/*R_TRL_TIME2*/      0x01,
/*R_CRL_CTL*/        0x4F,
/*R_CRL_FREQ1*/      0x30,


/*R_CRL_FREQ2*/      0xBE,
/*R_CRL_FREQ3*/      0xFE,
/*R_CHC_CTL1*/       0x11,
/*R_CHC_SNR*/        0xC3,
/*R_BDI_CTL*/        0x00,


/*R_DMP_CTL*/        0x00,
/*R_TPS_RCVD1*/      0x31,
/*R_TPS_RCVD2*/      0x02,
/*R_TPS_RCVD3*/      0x01,
/*R_TPS_RCVD4*/      0x30,


/*R_TPS_ID_CELL1*/   0x00,
/*R_TPS_ID_CELL2*/   0x00,
/*R_RESERVED_19*/    0x01,
/*R_RESERVED_20*/    0x02,
/*R_RESERVED_21*/    0x02,


/*R_TPS_CTL*/        0x00,
/*R_CTL_FFTOSNUM*/   0x10,
/*R_TESTSELECT*/     0x0C,
/*R_MSC_REV*/        0x0A,
/*R_PIR_CTL*/        0x00,


/*R_SNR_CARRIER1*/   0xA9,
/*R_SNR_CARRIER2*/   0x86,
/*R_RESERVED_22*/    0x30,
/*R_RESERVED_23*/    0x00,
/*R_RESERVED_24*/    0x00,


/*R_RESERVED_25*/    0x00,
/*R_RESERVED_26*/    0x00,
/*R_RESERVED_27*/    0x00,
/*R_RESERVED_28*/    0x00,
/*R_RESERVED_29*/    0x00,


/*R_RESERVED_30*/    0x00,
/*R_RESERVED_31*/    0x00,
/*R_VTH0*/           0x1E,
/*R_VTH1*/           0x14,
/*R_VTH2*/           0x0F,


/*R_VTH3*/           0x09,
/*R_RESERVED_32*/    0x00,
/*R_VTH5*/           0x05,
/*R_RESERVED_33*/    0x00,
/*R_VITPROG*/        0x92,


/*R_PR*/             0xAF,
/*R_VSEARCH*/        0x30,
/*R_RS*/             0xBC,
/*R_RSOUT*/          0x05,
/*R_ERRCTRL1*/       0x12,


/*R_ERRCNTM1*/       0x00,
/*R_ERRCNTL1*/       0x00,
/*R_ERRCTRL2*/       0xB3,
/*R_ERRCNTM2*/       0x00,
/*R_ERRCNTL2*/       0x00,


/*R_RESERVED_34*/    0x00,
/*R_VERROR*/         0x00,
/*R_ERRCTRL3*/       0x12,
/*R_ERRCNTM3*/       0x00,
/*R_ERRCNTL3*/       0x00,


/*R_RESERVED_35*/    0x00,
/*R_RESERVED_36*/    0x03,
/*R_RESERVED_37*/    0x00,
/*R_RESERVED_38*/    0x03,
/*R_LNBRX*/          0x80, /*0xC0*/


/*R_RESERVED_39*/    0xB0,
/*R_RESERVED_40*/    0x07,
/*R_RESERVED_41*/    0x00,
/*R_ANACTRL*/        0x00,
/*R_RESERVED_42*/    0x00,
 

/*R_RESERVED_43*/    0x00,
/*R_RESERVED_44*/    0x00,
/*R_RESERVED_45*/    0x00,
/*R_RESERVED_46*/    0x00,
/*R_RESERVED_47*/    0x00,


/*R_RESERVED_48*/    0x00,
/*R_RESERVED_49*/    0x00,
/*R_RESERVED_50*/    0x00,
/*R_CONSTMODE*/      0x02,
/*R_CONSTCARR1*/     0xD2,


/*R_CONSTCARR2*/     0x04,
/*R_ICONSTEL*/       0xDB,
/*R_QCONSTEL*/       0x8,
/*R_RESERVED_51*/    0x00,
/*R_RESERVED_52*/    0x00,



/*R_RESERVED_53*/    0x00,
/*R_RESERVED_54*/    0x00,
/*R_RF_AGC1*/        0x81,
/*R_RF_AGC2*/        0x80,
/*R_RESERVED_55*/    0x08,         


/*R_ANACTRL2*/       0x00,
/*R_PLLMDIV*/        0x00,
/*R_PLLNDIV*/        0x03,
/*R_PLLSETUP*/       0x17,
/*R_ANADIGCTRL*/     0x00,
/*R_TSTBIST*/        0x00

	        };/** end of array TMM6MHZ**/
#elif defined (STTUNER_TERR_TMM_7MHZ)
 U8 Def361Val[STV361_NBREGS]={
/*R_ID*/             0x30, 
/*R_I2CRPT*/         0x22,
/*R_TOPCTRL*/        0x02,
/*R_IOCFG0*/         0x40,
/*R_DAC0R*/          0x00,


/*R_IOCFG1*/         0x40,
/*R_DAC1R*/          0x00,
/*R_IOCFG2*/         0x80,
/*R_SDFR*/           0x00,
/*R_STATUS*/         0xF9,


/*R_AUX_CLK*/        0x1C,
/*R_RESERVED_1*/     0x00,
/*R_RESERVED_2*/     0x00,
/*R_RESERVED_3*/     0x00,
/*R_AGC2MAX*/        0x8C,


/*R_AGC2MIN*/        0x14,
/*R_AGC1MAX*/        0xFF,
/*R_AGC1MIN*/        0x6B,
/*R_AGCR*/           0xBC,
/*R_AGC2TH*/         0x50,


/*R_AGC12C*/         0x40,
/*R_AGCCTRL1*/       0x85,
/*R_AGCCTRL2*/       0x18,
/*R_AGC1VAL1*/       0xFF,
/*R_AGC1VAL2*/       0x0F,



/*R_AGC2VAL1*/       0xFA,
/*R_AGC2VAL2*/       0x01,
/*R_AGC2PGA*/        0x00,
/*R_OVF_RATE1*/      0x00,
/*R_OVF_RATE2*/      0x00,


/*R_GAIN_SRC1*/      0xFD,
/*R_GAIN_SRC2*/      0xAC,
/*R_INC_DEROT1*/     0x55,
/*R_INC_DEROT2*/     0x4B,
/*R_PPM_CPAMP_DIR*/  0x2C, /*0x5D*/



/*R_PPM_CPAMP_INV*/  0x00,
/*R_FREESTFE_1*/     0x03,
/*R_FREESTFE_2*/     0x1C,
/*R_DCOFFSET*/       0x06,
/*R_EN_PROCESS*/     0x01,
/*R_RESERVED_4*/     0xFF,


/*R_RESERVED_5*/     0x00,
/*R_RESERVED_6*/     0x80,
/*R_RESERVED_7*/     0x1D,
/*R_RESERVED_8*/     0x29,
/*R_RESERVED_9*/     0x14,


/*R_RESERVED_10*/    0xFE,
/*R_EPQ*/            0x02,
/*R_EPQAUTO*/        0x01,
/*R_CHP_TAPS*/       0x03,/* 0x01 for 2k mode and 0x03 for 8K mode*/
/*R_CHP_DYN_COEFF*/  0x00,


/*R_PPM_STATE_MAC*/  0x23,
/*R_INR_THRESHOLD*/  0x09,
/*R_COR_CTL*/        0x20,
/*R_COR_STAT*/       0xF6,
/*R_COR_INTEN*/      0x00,


/*R_COR_INTSTAT*/    0x3D,
/*R_COR_MODEGUARD*/  0x00,
/*R_AGC_CTL*/        0x18,
/*R_RESERVED_11*/    0x00,
/*R_RESERVED_12*/    0x00,


/*R_AGC_TARGET*/     0x1E,
/*R_AGC_GAIN1*/      0xFB,
/*R_AGC_GAIN2*/      0x1A,
/*R_RESERVED_13*/    0x00,
/*R_RESERVED_14*/    0x00,


/*R_RESERVED_15*/    0x00,
/*R_CAS_CTL*/        0x40,
/*R_CAS_FREQ*/       0xB3,
/*R_CAS_DAGCGAIN*/   0x10,
/*R_SYR_CTL*/        0x04,


/*R_SYR_STAT*/       0x13,
/*R_RESERVED_16*/    0x00,
/*R_RESERVED_17*/    0x00,
/*R_SYR_OFFSET1*/    0x00,
/*R_SYR_OFFSET2*/    0x00,


/*R_RESERVED_18*/    0x00,
/*R_SCR_CTL*/        0x00,
/*R_PPM_CTL1*/       0x38,
/*R_TRL_CTL*/        0x94,/* 0x14 */
/*R_TRL_NOMRATE1*/   0xAB, /*0xAC*/



/*R_TRL_NOMRATE2*/   0x56,
/*R_TRL_TIME1*/      0x3C,
/*R_TRL_TIME2*/      0x01,
/*R_CRL_CTL*/        0x4F,
/*R_CRL_FREQ1*/      0x30,


/*R_CRL_FREQ2*/      0xBE,
/*R_CRL_FREQ3*/      0xFE,
/*R_CHC_CTL1*/       0x11,
/*R_CHC_SNR*/        0xC3,
/*R_BDI_CTL*/        0x00,


/*R_DMP_CTL*/        0x00,
/*R_TPS_RCVD1*/      0x31,
/*R_TPS_RCVD2*/      0x02,
/*R_TPS_RCVD3*/      0x01,
/*R_TPS_RCVD4*/      0x30,


/*R_TPS_ID_CELL1*/   0x00,
/*R_TPS_ID_CELL2*/   0x00,
/*R_RESERVED_19*/    0x01,
/*R_RESERVED_20*/    0x02,
/*R_RESERVED_21*/    0x02,


/*R_TPS_CTL*/        0x00,
/*R_CTL_FFTOSNUM*/   0x10,
/*R_TESTSELECT*/     0x0C,
/*R_MSC_REV*/        0x0A,
/*R_PIR_CTL*/        0x00,


/*R_SNR_CARRIER1*/   0xA9,
/*R_SNR_CARRIER2*/   0x86,
/*R_RESERVED_22*/    0x30,
/*R_RESERVED_23*/    0x00,
/*R_RESERVED_24*/    0x00,


/*R_RESERVED_25*/    0x00,
/*R_RESERVED_26*/    0x00,
/*R_RESERVED_27*/    0x00,
/*R_RESERVED_28*/    0x00,
/*R_RESERVED_29*/    0x00,


/*R_RESERVED_30*/    0x00,
/*R_RESERVED_31*/    0x00,
/*R_VTH0*/           0x1E,
/*R_VTH1*/           0x14,
/*R_VTH2*/           0x0F,


/*R_VTH3*/           0x09,
/*R_RESERVED_32*/    0x00,
/*R_VTH5*/           0x05,
/*R_RESERVED_33*/    0x00,
/*R_VITPROG*/        0x92,


/*R_PR*/             0xAF,
/*R_VSEARCH*/        0x30,
/*R_RS*/             0xBC,
/*R_RSOUT*/          0x05,
/*R_ERRCTRL1*/       0x12,


/*R_ERRCNTM1*/       0x00,
/*R_ERRCNTL1*/       0x00,
/*R_ERRCTRL2*/       0xB3,
/*R_ERRCNTM2*/       0x00,
/*R_ERRCNTL2*/       0x00,


/*R_RESERVED_34*/    0x00,
/*R_VERROR*/         0x00,
/*R_ERRCTRL3*/       0x12,
/*R_ERRCNTM3*/       0x00,
/*R_ERRCNTL3*/       0x00,


/*R_RESERVED_35*/    0x00,
/*R_RESERVED_36*/    0x03,
/*R_RESERVED_37*/    0x00,
/*R_RESERVED_38*/    0x03,
/*R_LNBRX*/          0x80, /*0xC0*/


/*R_RESERVED_39*/    0xB0,
/*R_RESERVED_40*/    0x07,
/*R_RESERVED_41*/    0x00,
/*R_ANACTRL*/        0x00,
/*R_RESERVED_42*/    0x00,
 

/*R_RESERVED_43*/    0x00,
/*R_RESERVED_44*/    0x00,
/*R_RESERVED_45*/    0x00,
/*R_RESERVED_46*/    0x00,
/*R_RESERVED_47*/    0x00,


/*R_RESERVED_48*/    0x00,
/*R_RESERVED_49*/    0x00,
/*R_RESERVED_50*/    0x00,
/*R_CONSTMODE*/      0x02,
/*R_CONSTCARR1*/     0xD2,


/*R_CONSTCARR2*/     0x04,
/*R_ICONSTEL*/       0xDB,
/*R_QCONSTEL*/       0x8,
/*R_RESERVED_51*/    0x00,
/*R_RESERVED_52*/    0x00,



/*R_RESERVED_53*/    0x00,
/*R_RESERVED_54*/    0x00,
/*R_RF_AGC1*/        0x81,
/*R_RF_AGC2*/        0x80,
/*R_RESERVED_55*/    0x08,         


/*R_ANACTRL2*/       0x00,
/*R_PLLMDIV*/        0x00,
/*R_PLLNDIV*/        0x03,
/*R_PLLSETUP*/       0x17,
/*R_ANADIGCTRL*/     0x00,
/*R_TSTBIST*/        0x00

	        };/** end of array TMM7MHZ**/
#elif defined (STTUNER_TERR_TMM_8MHZ)
 U8 Def361Val[STV361_NBREGS]={
/*R_ID*/             0x30, 
/*R_I2CRPT*/         0x22,
/*R_TOPCTRL*/        0x02,
/*R_IOCFG0*/         0x40,
/*R_DAC0R*/          0x00,


/*R_IOCFG1*/         0x40,
/*R_DAC1R*/          0x00,
/*R_IOCFG2*/         0x80,
/*R_SDFR*/           0x00,
/*R_STATUS*/         0xF9,


/*R_AUX_CLK*/        0x1C,
/*R_RESERVED_1*/     0x00,
/*R_RESERVED_2*/     0x00,
/*R_RESERVED_3*/     0x00,
/*R_AGC2MAX*/        0x8C,


/*R_AGC2MIN*/        0x14,
/*R_AGC1MAX*/        0xFF,
/*R_AGC1MIN*/        0x6B,
/*R_AGCR*/           0xBC,
/*R_AGC2TH*/         0x50,


/*R_AGC12C*/         0x40,
/*R_AGCCTRL1*/       0x85,
/*R_AGCCTRL2*/       0x18,
/*R_AGC1VAL1*/       0xFF,
/*R_AGC1VAL2*/       0x0F,



/*R_AGC2VAL1*/       0xFA,
/*R_AGC2VAL2*/       0x01,
/*R_AGC2PGA*/        0x00,
/*R_OVF_RATE1*/      0x00,
/*R_OVF_RATE2*/      0x00,


/*R_GAIN_SRC1*/      0xFD,
/*R_GAIN_SRC2*/      0xAC,
/*R_INC_DEROT1*/     0x55,
/*R_INC_DEROT2*/     0x4B,
/*R_PPM_CPAMP_DIR*/  0x2C, /*0x5D*/



/*R_PPM_CPAMP_INV*/  0x00,
/*R_FREESTFE_1*/     0x03,
/*R_FREESTFE_2*/     0x1C,
/*R_DCOFFSET*/       0x06,
/*R_EN_PROCESS*/     0x01,
/*R_RESERVED_4*/     0xFF,


/*R_RESERVED_5*/     0x00,
/*R_RESERVED_6*/     0x80,
/*R_RESERVED_7*/     0x1D,
/*R_RESERVED_8*/     0x29,
/*R_RESERVED_9*/     0x14,


/*R_RESERVED_10*/    0xFE,
/*R_EPQ*/            0x02,
/*R_EPQAUTO*/        0x01,
/*R_CHP_TAPS*/       0x03,/* 0x01 for 2k mode and 0x03 for 8K mode*/
/*R_CHP_DYN_COEFF*/  0x00,


/*R_PPM_STATE_MAC*/  0x23,
/*R_INR_THRESHOLD*/  0x09,
/*R_COR_CTL*/        0x20,
/*R_COR_STAT*/       0xF6,
/*R_COR_INTEN*/      0x00,


/*R_COR_INTSTAT*/    0x3D,
/*R_COR_MODEGUARD*/  0x00,
/*R_AGC_CTL*/        0x18,
/*R_RESERVED_11*/    0x00,
/*R_RESERVED_12*/    0x00,


/*R_AGC_TARGET*/     0x1E,
/*R_AGC_GAIN1*/      0xFB,
/*R_AGC_GAIN2*/      0x1A,
/*R_RESERVED_13*/    0x00,
/*R_RESERVED_14*/    0x00,


/*R_RESERVED_15*/    0x00,
/*R_CAS_CTL*/        0x40,
/*R_CAS_FREQ*/       0xB3,
/*R_CAS_DAGCGAIN*/   0x10,
/*R_SYR_CTL*/        0x04,


/*R_SYR_STAT*/       0x13,
/*R_RESERVED_16*/    0x00,
/*R_RESERVED_17*/    0x00,
/*R_SYR_OFFSET1*/    0x00,
/*R_SYR_OFFSET2*/    0x00,


/*R_RESERVED_18*/    0x00,
/*R_SCR_CTL*/        0x00,
/*R_PPM_CTL1*/       0x38,
/*R_TRL_CTL*/        0x94,/* 0x14 */
/*R_TRL_NOMRATE1*/   0xAB, /*0xAC*/



/*R_TRL_NOMRATE2*/   0x56,
/*R_TRL_TIME1*/      0x3C,
/*R_TRL_TIME2*/      0x01,
/*R_CRL_CTL*/        0x4F,
/*R_CRL_FREQ1*/      0x30,


/*R_CRL_FREQ2*/      0xBE,
/*R_CRL_FREQ3*/      0xFE,
/*R_CHC_CTL1*/       0x11,
/*R_CHC_SNR*/        0xC3,
/*R_BDI_CTL*/        0x00,


/*R_DMP_CTL*/        0x00,
/*R_TPS_RCVD1*/      0x31,
/*R_TPS_RCVD2*/      0x02,
/*R_TPS_RCVD3*/      0x01,
/*R_TPS_RCVD4*/      0x30,


/*R_TPS_ID_CELL1*/   0x00,
/*R_TPS_ID_CELL2*/   0x00,
/*R_RESERVED_19*/    0x01,
/*R_RESERVED_20*/    0x02,
/*R_RESERVED_21*/    0x02,


/*R_TPS_CTL*/        0x00,
/*R_CTL_FFTOSNUM*/   0x10,
/*R_TESTSELECT*/     0x0C,
/*R_MSC_REV*/        0x0A,
/*R_PIR_CTL*/        0x00,


/*R_SNR_CARRIER1*/   0xA9,
/*R_SNR_CARRIER2*/   0x86,
/*R_RESERVED_22*/    0x30,
/*R_RESERVED_23*/    0x00,
/*R_RESERVED_24*/    0x00,


/*R_RESERVED_25*/    0x00,
/*R_RESERVED_26*/    0x00,
/*R_RESERVED_27*/    0x00,
/*R_RESERVED_28*/    0x00,
/*R_RESERVED_29*/    0x00,


/*R_RESERVED_30*/    0x00,
/*R_RESERVED_31*/    0x00,
/*R_VTH0*/           0x1E,
/*R_VTH1*/           0x14,
/*R_VTH2*/           0x0F,


/*R_VTH3*/           0x09,
/*R_RESERVED_32*/    0x00,
/*R_VTH5*/           0x05,
/*R_RESERVED_33*/    0x00,
/*R_VITPROG*/        0x92,


/*R_PR*/             0xAF,
/*R_VSEARCH*/        0x30,
/*R_RS*/             0xBC,
/*R_RSOUT*/          0x05,
/*R_ERRCTRL1*/       0x12,


/*R_ERRCNTM1*/       0x00,
/*R_ERRCNTL1*/       0x00,
/*R_ERRCTRL2*/       0xB3,
/*R_ERRCNTM2*/       0x00,
/*R_ERRCNTL2*/       0x00,


/*R_RESERVED_34*/    0x00,
/*R_VERROR*/         0x00,
/*R_ERRCTRL3*/       0x12,
/*R_ERRCNTM3*/       0x00,
/*R_ERRCNTL3*/       0x00,


/*R_RESERVED_35*/    0x00,
/*R_RESERVED_36*/    0x03,
/*R_RESERVED_37*/    0x00,
/*R_RESERVED_38*/    0x03,
/*R_LNBRX*/          0x80, /*0xC0*/


/*R_RESERVED_39*/    0xB0,
/*R_RESERVED_40*/    0x07,
/*R_RESERVED_41*/    0x00,
/*R_ANACTRL*/        0x00,
/*R_RESERVED_42*/    0x00,
 

/*R_RESERVED_43*/    0x00,
/*R_RESERVED_44*/    0x00,
/*R_RESERVED_45*/    0x00,
/*R_RESERVED_46*/    0x00,
/*R_RESERVED_47*/    0x00,


/*R_RESERVED_48*/    0x00,
/*R_RESERVED_49*/    0x00,
/*R_RESERVED_50*/    0x00,
/*R_CONSTMODE*/      0x02,
/*R_CONSTCARR1*/     0xD2,


/*R_CONSTCARR2*/     0x04,
/*R_ICONSTEL*/       0xDB,
/*R_QCONSTEL*/       0x8,
/*R_RESERVED_51*/    0x00,
/*R_RESERVED_52*/    0x00,



/*R_RESERVED_53*/    0x00,
/*R_RESERVED_54*/    0x00,
/*R_RF_AGC1*/        0x81,
/*R_RF_AGC2*/        0x80,
/*R_RESERVED_55*/    0x08,         


/*R_ANACTRL2*/       0x00,
/*R_PLLMDIV*/        0x00,
/*R_PLLNDIV*/        0x03,
/*R_PLLSETUP*/       0x17,
/*R_ANADIGCTRL*/     0x00,
/*R_TSTBIST*/        0x00

	        };/** end of array TMM8MHZ**/
#else
 U8 Def361Val[STV361_NBREGS]={
/*R_ID*/             0x30, 
/*R_I2CRPT*/         0x22,
/*R_TOPCTRL*/        0x02,
/*R_IOCFG0*/         0x40,
/*R_DAC0R*/          0x00,


/*R_IOCFG1*/         0x40,
/*R_DAC1R*/          0x00,
/*R_IOCFG2*/         0x80,
/*R_SDFR*/           0x00,
/*R_STATUS*/         0xF9,


/*R_AUX_CLK*/        0x1C,
/*R_RESERVED_1*/     0x00,
/*R_RESERVED_2*/     0x00,
/*R_RESERVED_3*/     0x00,
/*R_AGC2MAX*/        0x8C,


/*R_AGC2MIN*/        0x14,
/*R_AGC1MAX*/        0xFF,
/*R_AGC1MIN*/        0x6B,
/*R_AGCR*/           0xBC,
/*R_AGC2TH*/         0x50,


/*R_AGC12C*/         0x40,
/*R_AGCCTRL1*/       0x85,
/*R_AGCCTRL2*/       0x18,
/*R_AGC1VAL1*/       0xFF,
/*R_AGC1VAL2*/       0x0F,



/*R_AGC2VAL1*/       0xFA,
/*R_AGC2VAL2*/       0x01,
/*R_AGC2PGA*/        0x00,
/*R_OVF_RATE1*/      0x00,
/*R_OVF_RATE2*/      0x00,


/*R_GAIN_SRC1*/      0xFD,
/*R_GAIN_SRC2*/      0xAC,
/*R_INC_DEROT1*/     0x55,
/*R_INC_DEROT2*/     0x4B,
/*R_PPM_CPAMP_DIR*/  0x2C, /*0x5D*/



/*R_PPM_CPAMP_INV*/  0x00,
/*R_FREESTFE_1*/     0x03,
/*R_FREESTFE_2*/     0x1C,
/*R_DCOFFSET*/       0x06,
/*R_EN_PROCESS*/     0x01,
/*R_RESERVED_4*/     0xFF,


/*R_RESERVED_5*/     0x00,
/*R_RESERVED_6*/     0x80,
/*R_RESERVED_7*/     0x1D,
/*R_RESERVED_8*/     0x29,
/*R_RESERVED_9*/     0x14,


/*R_RESERVED_10*/    0xFE,
/*R_EPQ*/            0x02,
/*R_EPQAUTO*/        0x01,
/*R_CHP_TAPS*/       0x03,/* 0x01 for 2k mode and 0x03 for 8K mode*/
/*R_CHP_DYN_COEFF*/  0x00,


/*R_PPM_STATE_MAC*/  0x23,
/*R_INR_THRESHOLD*/  0x09,
/*R_COR_CTL*/        0x20,
/*R_COR_STAT*/       0xF6,
/*R_COR_INTEN*/      0x00,


/*R_COR_INTSTAT*/    0x3D,
/*R_COR_MODEGUARD*/  0x00,
/*R_AGC_CTL*/        0x18,
/*R_RESERVED_11*/    0x00,
/*R_RESERVED_12*/    0x00,


/*R_AGC_TARGET*/     0x1E,
/*R_AGC_GAIN1*/      0xFB,
/*R_AGC_GAIN2*/      0x1A,
/*R_RESERVED_13*/    0x00,
/*R_RESERVED_14*/    0x00,


/*R_RESERVED_15*/    0x00,
/*R_CAS_CTL*/        0x40,
/*R_CAS_FREQ*/       0xB3,
/*R_CAS_DAGCGAIN*/   0x10,
/*R_SYR_CTL*/        0x04,/*** to switch on the automic epq****/


/*R_SYR_STAT*/       0x13,
/*R_RESERVED_16*/    0x00,
/*R_RESERVED_17*/    0x00,
/*R_SYR_OFFSET1*/    0x00,
/*R_SYR_OFFSET2*/    0x00,


/*R_RESERVED_18*/    0x00,
/*R_SCR_CTL*/        0x00,
/*R_PPM_CTL1*/       0x38,
/*R_TRL_CTL*/        0x94,/* 0x14 */
/*R_TRL_NOMRATE1*/   0xAB, /*0xAC*/



/*R_TRL_NOMRATE2*/   0x56,
/*R_TRL_TIME1*/      0x3C,
/*R_TRL_TIME2*/      0x01,
/*R_CRL_CTL*/        0x4F,
/*R_CRL_FREQ1*/      0x30,


/*R_CRL_FREQ2*/      0xBE,
/*R_CRL_FREQ3*/      0xFE,
/*R_CHC_CTL1*/       0x11,
/*R_CHC_SNR*/        0xC3,
/*R_BDI_CTL*/        0x00,


/*R_DMP_CTL*/        0x00,
/*R_TPS_RCVD1*/      0x31,
/*R_TPS_RCVD2*/      0x02,
/*R_TPS_RCVD3*/      0x01,
/*R_TPS_RCVD4*/      0x30,


/*R_TPS_ID_CELL1*/   0x00,
/*R_TPS_ID_CELL2*/   0x00,
/*R_RESERVED_19*/    0x01,
/*R_RESERVED_20*/    0x02,
/*R_RESERVED_21*/    0x02,


/*R_TPS_CTL*/        0x00,
/*R_CTL_FFTOSNUM*/   0x10,
/*R_TESTSELECT*/     0x0C,
/*R_MSC_REV*/        0x0A,
/*R_PIR_CTL*/        0x00,


/*R_SNR_CARRIER1*/   0xA9,
/*R_SNR_CARRIER2*/   0x86,
/*R_RESERVED_22*/    0x30,
/*R_RESERVED_23*/    0x00,
/*R_RESERVED_24*/    0x00,


/*R_RESERVED_25*/    0x00,
/*R_RESERVED_26*/    0x00,
/*R_RESERVED_27*/    0x00,
/*R_RESERVED_28*/    0x00,
/*R_RESERVED_29*/    0x00,


/*R_RESERVED_30*/    0x00,
/*R_RESERVED_31*/    0x00,
/*R_VTH0*/           0x1E,
/*R_VTH1*/           0x14,
/*R_VTH2*/           0x0F,


/*R_VTH3*/           0x09,
/*R_RESERVED_32*/    0x00,
/*R_VTH5*/           0x05,
/*R_RESERVED_33*/    0x00,
/*R_VITPROG*/        0x92,


/*R_PR*/             0xAF,
/*R_VSEARCH*/        0x30,
/*R_RS*/             0xBC,
/*R_RSOUT*/          0x05,
/*R_ERRCTRL1*/       0x13,/**** To make byte event at a rate of 2*10e+18***/


/*R_ERRCNTM1*/       0x00,
/*R_ERRCNTL1*/       0x00,
/*R_ERRCTRL2*/       0xB3,
/*R_ERRCNTM2*/       0x00,
/*R_ERRCNTL2*/       0x00,


/*R_RESERVED_34*/    0x00,
/*R_VERROR*/         0x00,
/*R_ERRCTRL3*/       0x12,
/*R_ERRCNTM3*/       0x00,
/*R_ERRCNTL3*/       0x00,


/*R_RESERVED_35*/    0x00,
/*R_RESERVED_36*/    0x03,
/*R_RESERVED_37*/    0x00,
/*R_RESERVED_38*/    0x03,
/*R_LNBRX*/          0x80, /*0xC0*/


/*R_RESERVED_39*/    0xB0,
/*R_RESERVED_40*/    0x07,
/*R_RESERVED_41*/    0x00,
/*R_ANACTRL*/        0x00,
/*R_RESERVED_42*/    0x00,
 

/*R_RESERVED_43*/    0x00,
/*R_RESERVED_44*/    0x00,
/*R_RESERVED_45*/    0x00,
/*R_RESERVED_46*/    0x00,
/*R_RESERVED_47*/    0x00,


/*R_RESERVED_48*/    0x00,
/*R_RESERVED_49*/    0x00,
/*R_RESERVED_50*/    0x00,
/*R_CONSTMODE*/      0x02,
/*R_CONSTCARR1*/     0xD2,


/*R_CONSTCARR2*/     0x04,
/*R_ICONSTEL*/       0xDB,
/*R_QCONSTEL*/       0x8,
/*R_RESERVED_51*/    0x00,
/*R_RESERVED_52*/    0x00,



/*R_RESERVED_53*/    0x00,
/*R_RESERVED_54*/    0x00,
/*R_RF_AGC1*/        0x81,
/*R_RF_AGC2*/        0x80,
/*R_RESERVED_55*/    0x08,         


/*R_ANACTRL2*/       0x00,
/*R_PLLMDIV*/        0x00,
/*R_PLLNDIV*/        0x03,
/*R_PLLSETUP*/       0x17,
/*R_ANADIGCTRL*/     0x00,
/*R_TSTBIST*/        0x00

	        };/** end of array**/
#endif	        
/* Private types/constants ------------------------------------------------ */


#if defined (ST_OS21) || defined (ST_OSLINUX)
#define STTUNER_TaskDelay(x) STOS_TaskDelay((signed int)x)
#else
#define STTUNER_TaskDelay(x) STOS_TaskDelay((unsigned int)x)
#endif


#define ANALOG_CARRIER_DETECT_SYMBOL_RATE   5000000
#define ANALOG_CARRIER_DETECT_AGC2_VALUE    25

/* Device capabilities */
#define MAX_AGC                         255
#define MAX_SIGNAL_QUALITY              100
#define MAX_BER                         200000







/* private variables ------------------------------------------------------- */

static semaphore_t *Lock_InitTermOpenClose; /* guard calls to the functions */

static BOOL        Installed = FALSE;



/* instance chain, the default boot value is invalid, to catch errors */
static D0361_InstanceData_t *InstanceChainTop = (D0361_InstanceData_t *)0x7fffffff;

/* functions --------------------------------------------------------------- */

int FE_361_PowOf2(int number);

/* API */
ST_ErrorCode_t demod_d0361_Init(ST_DeviceName_t *DeviceName, DEMOD_InitParams_t *InitParams);
ST_ErrorCode_t demod_d0361_Term(ST_DeviceName_t *DeviceName, DEMOD_TermParams_t *TermParams);

ST_ErrorCode_t demod_d0361_Open (ST_DeviceName_t *DeviceName, DEMOD_OpenParams_t  *OpenParams, DEMOD_Capability_t *Capability, DEMOD_Handle_t *Handle);
ST_ErrorCode_t demod_d0361_Close(DEMOD_Handle_t  Handle, DEMOD_CloseParams_t *CloseParams);

ST_ErrorCode_t demod_d0361_GetTunerInfo    (DEMOD_Handle_t Handle, STTUNER_TunerInfo_t *TunerInfo_p);
ST_ErrorCode_t demod_d0361_GetSignalQuality(DEMOD_Handle_t Handle, U32  *SignalQuality_p, U32 *Ber);
ST_ErrorCode_t demod_d0361_GetModulation   (DEMOD_Handle_t Handle, STTUNER_Modulation_t *Modulation);
ST_ErrorCode_t demod_d0361_GetAGC          (DEMOD_Handle_t Handle, S16                  *Agc);
ST_ErrorCode_t demod_d0361_GetMode         (DEMOD_Handle_t Handle, STTUNER_Mode_t       *Mode);
ST_ErrorCode_t demod_d0361_GetGuard        (DEMOD_Handle_t Handle, STTUNER_Guard_t      *Guard);
ST_ErrorCode_t demod_d0361_GetFECRates     (DEMOD_Handle_t Handle, STTUNER_FECRate_t    *FECRates);
ST_ErrorCode_t demod_d0361_IsLocked        (DEMOD_Handle_t Handle, BOOL                 *IsLocked);
ST_ErrorCode_t demod_d0361_Tracking        (DEMOD_Handle_t Handle, BOOL ForceTracking,   U32 *NewFrequency, BOOL *SignalFound);
ST_ErrorCode_t demod_d0361_ScanFrequency   (DEMOD_Handle_t  Handle,
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
ST_ErrorCode_t demod_d0361_ioctl           (DEMOD_Handle_t Handle, U32 Function, void *InParams, void *OutParams,
                                             STTUNER_Da_Status_t *Status);

/* I/O API */
ST_ErrorCode_t demod_d0361_ioaccess(DEMOD_Handle_t Handle, IOARCH_Handle_t IOHandle,
                  STTUNER_IOARCH_Operation_t Operation, U16 SubAddr, U8 *Data, U32 TransferSize, U32 Timeout);
/* For TPS CellID extraction */
ST_ErrorCode_t demod_d0361_GetTPSCellId(DEMOD_Handle_t Handle, U16  *TPSCellId);

/* For STANDBY API mode */ 
ST_ErrorCode_t demod_d0361_StandByMode(DEMOD_Handle_t Handle, STTUNER_StandByMode_t PowerMode);

/* local functions --------------------------------------------------------- */

D0361_InstanceData_t *D0361_GetInstFromHandle(DEMOD_Handle_t Handle);

/* ----------------------------------------------------------------------------
Name: STTUNER_DRV_DEMOD_STV0361_Install()

Description:
    install a terrestrial device driver into the demod database.

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_DRV_DEMOD_STV0361_Install(STTUNER_demod_dbase_t *Demod)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
   const char *identity = "STTUNER d0361.c STTUNER_DRV_DEMOD_STV0361_Install()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if(Installed == TRUE)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
        STTBX_Print(("%s fail driver already installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
    STTBX_Print(("%s installing ter:demod:STV0361...", identity));
#endif

    /* mark ID in database */
    Demod->ID = STTUNER_DEMOD_STV0361;

    /* map API */
    Demod->demod_Init = demod_d0361_Init;
    Demod->demod_Term = demod_d0361_Term;

    Demod->demod_Open  = demod_d0361_Open;
    Demod->demod_Close = demod_d0361_Close;

    Demod->demod_IsAnalogCarrier  = NULL;
    Demod->demod_GetTunerInfo     = demod_d0361_GetTunerInfo;
    Demod->demod_GetSignalQuality = demod_d0361_GetSignalQuality;
    Demod->demod_GetModulation    = demod_d0361_GetModulation;
    Demod->demod_GetAGC           = demod_d0361_GetAGC;
    Demod->demod_GetMode          = demod_d0361_GetMode;
    Demod->demod_GetGuard         = demod_d0361_GetGuard;
    Demod->demod_GetFECRates      = demod_d0361_GetFECRates;
    Demod->demod_IsLocked         = demod_d0361_IsLocked ;
    Demod->demod_SetFECRates      = NULL;
    Demod->demod_Tracking         = demod_d0361_Tracking;
    Demod->demod_ScanFrequency    = demod_d0361_ScanFrequency;

    Demod->demod_ioaccess = demod_d0361_ioaccess;
    Demod->demod_ioctl    = demod_d0361_ioctl;
    Demod->demod_GetTPSCellId = demod_d0361_GetTPSCellId;
    Demod->demod_StandByMode = demod_d0361_StandByMode;

    InstanceChainTop = NULL;

    Lock_InitTermOpenClose = STOS_SemaphoreCreateFifo(NULL,1);


    Installed = TRUE;

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
    STTBX_Print(("ok\n"));
#endif

    return(Error);
}



/* ----------------------------------------------------------------------------
Name: STTUNER_DRV_DEMOD_STV0361_UnInstall()

Description:
    install a terrestrial device driver into the demod database.

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_DRV_DEMOD_STV0361_UnInstall(STTUNER_demod_dbase_t *Demod)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
   const char *identity = "STTUNER d0361.c STTUNER_DRV_DEMOD_STV0361_UnInstall()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

    if(Demod->ID != STTUNER_DEMOD_STV0361)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
        STTBX_Print(("%s fail incorrect driver type\n", identity));
#endif
        return(STTUNER_ERROR_ID);
    }

    /* has all memory been freed, by Term() */
    if(InstanceChainTop != NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
        STTBX_Print(("%s fail at least one instance not terminated\n", identity));
#endif
        return(ST_ERROR_OPEN_HANDLE);
    }


#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
    STTBX_Print(("%s uninstalling ter:demod:STV0361...", identity));
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
    Demod->demod_GetTPSCellId = NULL;
    Demod->demod_StandByMode = NULL ;

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
        STTBX_Print(("<"));
#endif

  STOS_SemaphoreDelete(NULL,Lock_InitTermOpenClose);

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
        STTBX_Print((">"));
#endif

    InstanceChainTop = (D0361_InstanceData_t *)0x7ffffffe;
    Installed        = FALSE;

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
        STTBX_Print(("ok\n"));
#endif

    return(Error);
}



/* ------------------------------------------------------------------------- */
/* /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ API /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ */
/* ------------------------------------------------------------------------- */



/* ----------------------------------------------------------------------------
Name: demod_d0361_Init()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0361_Init(ST_DeviceName_t *DeviceName, DEMOD_InitParams_t *InitParams)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
    const char *identity = "STTUNER d0361.c demod_d0361_Init()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0361_InstanceData_t *InstanceNew, *Instance;


    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
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
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
        STTBX_Print(("%s fail MemoryPartition not valid\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    Error = STTUNER_Util_CheckPtrNull(DeviceName);
    if( Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
        STTBX_Print(("%s fail DeviceName not valid\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    InstanceNew = STOS_MemoryAllocateClear(InitParams->MemoryPartition, 1, sizeof( D0361_InstanceData_t ));
    if (InstanceNew == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
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
    InstanceNew->DeviceMap.Registers = STV361_NBREGS;
    InstanceNew->DeviceMap.Fields    = STV361_NBFIELDS;
    InstanceNew->DeviceMap.Mode      = IOREG_MODE_SUBADR_8; /* i/o addressing mode to use */
    InstanceNew->DeviceMap.MemoryPartition = InitParams->MemoryPartition;
    InstanceNew->DeviceMap.DefVal= (U32 *)&Def361Val[0];







    InstanceNew->StandBy_Flag = 0 ;
    /* Allocate  memory for register mapping */
    Error = STTUNER_IOREG_Open(&InstanceNew->DeviceMap);
     if (Error != ST_NO_ERROR)
    {
	#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
        STTBX_Print(("%s fail setup new register database\n", identity));
	#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error); 
            }

    
    switch(InstanceNew->TSOutputMode)
    {
        case STTUNER_TS_MODE_DEFAULT:
        case STTUNER_TS_MODE_PARALLEL:
            InstanceNew->FE_361_InitParams.Clock = FE_361_PARALLEL_CLOCK;
            break;

        case STTUNER_TS_MODE_SERIAL:
            switch(InstanceNew->SerialClockSource) /* Set serial clock source */
            {
                case STTUNER_SCLK_VCODIV6:
                    InstanceNew->FE_361_InitParams.Clock = FE_361_SERIAL_VCODIV6_CLOCK;
                    break;

                default:
                case STTUNER_SCLK_DEFAULT:
                case STTUNER_SCLK_MASTER:
                    InstanceNew->FE_361_InitParams.Clock = FE_361_SERIAL_MASTER_CLOCK;
                    break;
            }
	case STTUNER_TS_MODE_DVBCI:
		   break;
    }

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
    STTBX_Print(("%s allocated & initalized block named '%s' at 0x%08x (%d bytes)\n", identity, InstanceNew->DeviceName, (U32)InstanceNew, sizeof( D0361_InstanceData_t ) ));
#endif
    SEM_UNLOCK(Lock_InitTermOpenClose);
    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0361_Term()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0361_Term(ST_DeviceName_t *DeviceName, DEMOD_TermParams_t *TermParams)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
   const char *identity = "STTUNER d0361.c demod_d0361_Term()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0361_InstanceData_t *Instance, *InstancePrev, *InstanceNext;

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
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
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
        STTBX_Print(("%s fail TermParams not valid\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    /* ---------- check that at least one init has taken place ---------- */
    if(InstanceChainTop == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
        STTBX_Print(("%s fail nothing initalized\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(STTUNER_ERROR_INITSTATE);
    }


    /* reap next matching DeviceName */
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
    STTBX_Print(("%s looking for first free handle[\n", identity));
#endif

    Instance = InstanceChainTop;
    while(1)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
        STTBX_Print(("(%s)", Instance->DeviceName));
#endif
        if ( strcmp( (char *)Instance->DeviceName, (char *)DeviceName) == 0)
        {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
            STTBX_Print(("<-- ]\n"));
#endif
            /* found so now xlink prev and next(if applicable) and deallocate memory */
            InstancePrev = Instance->InstanceChainPrev;
            InstanceNext = Instance->InstanceChainNext;

            /* if instance to delete is first in chain */
            if (Instance->InstanceChainPrev == NULL)
            {
                InstanceChainTop = InstanceNext;        /* which would be NULL if last block to be term'd */
             if ( InstanceNext != NULL)
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
	   #ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
            STTBX_Print(("%s fail deallocate register database\n", identity));
	   #endif
           SEM_UNLOCK(Lock_InitTermOpenClose);
           return(Error);           
            }

            STOS_MemoryDeallocate(Instance->MemoryPartition, Instance);

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
            STTBX_Print(("%s freed block at %0x%08x\n", identity, (U32)Instance ));
#endif

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
            STTBX_Print(("%s terminated ok\n", identity));
#endif
            SEM_UNLOCK(Lock_InitTermOpenClose);
            return(Error);
        }
        else if(Instance->InstanceChainNext == NULL)
        {       /* error we should have found a matching name before the end of the list */
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
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


#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
    STTBX_Print(("%s FAIL! this point should NEVER be reached\n", identity));
#endif
    SEM_UNLOCK(Lock_InitTermOpenClose);
    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0361_Open()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0361_Open(ST_DeviceName_t *DeviceName, DEMOD_OpenParams_t  *OpenParams, DEMOD_Capability_t *Capability, DEMOD_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
   const char *identity = "STTUNER d0361.c demod_d0361_Open()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0361_InstanceData_t     *Instance;
    U8 ChipID;


    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

    /* now safe to lock semaphore */
    SEM_LOCK(Lock_InitTermOpenClose);

    /* ---------- check that at least one init has taken place ---------- */
    if(InstanceChainTop == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
        STTBX_Print(("%s fail nothing initalized\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(STTUNER_ERROR_INITSTATE);
    }

    /* find  handle from name */
    Instance = InstanceChainTop;
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
    STTBX_Print(("%s looking (%s)", identity, Instance->DeviceName));
#endif
    while(strcmp((char *)Instance->DeviceName, (char *)DeviceName) != 0)
    {
        /* error, should have found matching DeviceName before end of list */
        if(Instance->InstanceChainNext == NULL)
        {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
            STTBX_Print(("\n%s fail no block named '%s' before end of list\n", identity, DeviceName));
#endif
            SEM_UNLOCK(Lock_InitTermOpenClose);
            return(STTUNER_ERROR_INITSTATE);
        }
        Instance = Instance->InstanceChainNext;   /* next block */
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
        STTBX_Print(("(%s)", Instance->DeviceName));
#endif
    }
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
        STTBX_Print((" found ok\n"));
#endif

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
    STTBX_Print(("%s using block at 0x%08x\n", identity, (U32)Instance));
#endif

    /* check handle IS actually free */
    if(Instance->TopLevelHandle != STTUNER_MAX_HANDLES)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
        STTBX_Print(("%s fail driver instance already open\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_OPEN_HANDLE);
    }
    /* else now got pointer to free (and valid) data block */

    
     
     ChipID = STTUNER_IOREG_GetRegister( &(Instance->DeviceMap), Instance->IOHandle, R_ID);
      if ( (ChipID & 0x30) ==  0x00)   /* Accept cuts 1 (0x30) */
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
        STTBX_Print(("%s fail device not found (0x%02x)\n", identity, ChipID));
#endif
       SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_UNKNOWN_DEVICE);
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
        STTBX_Print(("%s device found, release/revision=%u\n", identity, ChipID & 0x30));
#endif
    }
 /* reset all chip registers */
      Error = STTUNER_IOREG_Reset(&(Instance->DeviceMap), Instance->IOHandle,Def361Val,STV0361_address);


    /* Set serial/parallel data mode */
    if (Instance->TSOutputMode == STTUNER_TS_MODE_SERIAL)
    {
        
          Error |= STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle, OUTRS_SP, 1);
    }
    else
    {
         Error |= STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle, OUTRS_SP, 0);
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
    	 STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle, CLK_POL, 0);
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
    Instance->TopLevelHandle = OpenParams->TopLevelHandle;
    *Handle = (U32)Instance;

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
    STTBX_Print(("%s opened ok\n", identity));
#endif

    SEM_UNLOCK(Lock_InitTermOpenClose);
    



        return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0361_Close()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0361_Close(DEMOD_Handle_t Handle, DEMOD_CloseParams_t *CloseParams)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
   const char *identity = "STTUNER d0361.c demod_d0361_Close()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0361_InstanceData_t     *Instance;

    /* private driver instance data */
    Instance = D0361_GetInstFromHandle(Handle);

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

    SEM_LOCK(Lock_InitTermOpenClose);

    /* ---------- check that at least one init has taken place ---------- */
    if(InstanceChainTop == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
        STTBX_Print(("%s fail nothing initalized\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(STTUNER_ERROR_INITSTATE);
    }

    if(Instance->TopLevelHandle == STTUNER_MAX_HANDLES)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
        STTBX_Print(("%s fail driver instance not open\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_OPEN_HANDLE);
    }














 

    /* indicate instance is closed */
    Instance->TopLevelHandle = STTUNER_MAX_HANDLES;

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
    STTBX_Print(("%s closed\n", identity));
#endif

    SEM_UNLOCK(Lock_InitTermOpenClose);
    return(Error);
}


/* ----------------------------------------------------------------------------
Name: demod_d0361_GetTunerInfo()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0361_GetTunerInfo(DEMOD_Handle_t Handle, STTUNER_TunerInfo_t *TunerInfo_p)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
   const char *identity = "STTUNER d0361.c demod_d0361_GetTunerInfo()";
#endif
    ST_ErrorCode_t       Error = ST_NO_ERROR;
    ST_ErrorCode_t       TunerError ;
    D0361_InstanceData_t *Instance;
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
    /* private driver instance data */
    Instance = D0361_GetInstFromHandle(Handle);
    
    /* top level public instance data */
    Inst = STTUNER_GetDrvInst();
  
    /* Read noise estimations for C/N and BER */
   
    FE_361_GetNoiseEstimator(&(Instance->DeviceMap), Instance->IOHandle, &CurSignalQuality, &CurBitErrorRate);

	
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
     Data=STTUNER_IOREG_GetField(&(Instance->DeviceMap), Instance->IOHandle, TPS_CONST);

        
     #ifndef ST_OSLINUX
    STOS_TaskUnlock();
    #endif
   /* Data = ChipGetFieldImage(FE2CHIP(Instance->FE_360_Handle), TPS_CONST);*/
    switch (Data)
    {
        case 0:  CurModulation = STTUNER_MOD_QPSK;  break;
        case 1:  CurModulation = STTUNER_MOD_16QAM; break;
        case 2:  CurModulation = STTUNER_MOD_64QAM; break;
        default: 
        /*CurModulation = STTUNER_MOD_ALL;*/
        CurModulation = Data ;
        STTUNER_TaskDelay(5);
         Data=STTUNER_IOREG_GetField(&(Instance->DeviceMap), Instance->IOHandle, TPS_CONST);

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
     #ifndef ST_OSLINUX
     STOS_TaskLock();
    #ifdef PROCESSOR_C1 /***For C1 core this delay needed otherwise STTUNER_task_lock(); not working fine ****/ 
    /*task_delay(5);*/
    #endif
    #endif
    Data=STTUNER_IOREG_GetField(&(Instance->DeviceMap), Instance->IOHandle, TPS_MODE);

    #ifndef ST_OSLINUX
     STOS_TaskUnlock();
    #endif
    /*Data = ChipGetFieldImage(FE2CHIP(Instance->FE_360_Handle), TPS_MODE);*/
    switch (Data)
    {
        case 0:  CurMode = STTUNER_MODE_2K; break;
        case 1:  CurMode = STTUNER_MODE_8K; break;
        default: 
        CurMode =Data;
        STTUNER_TaskDelay(5);
          Data=STTUNER_IOREG_GetField(&(Instance->DeviceMap), Instance->IOHandle, TPS_MODE);

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
        
     #ifndef ST_OSLINUX
     STOS_TaskLock();
    #ifdef PROCESSOR_C1 /***For C1 core this delay needed otherwise STTUNER_task_lock(); not working fine ****/ 
    /*task_delay(5);*/
    #endif
    #endif
    Data=STTUNER_IOREG_GetField(&(Instance->DeviceMap), Instance->IOHandle, TPS_HIERMODE);

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
   
     #ifndef ST_OSLINUX
    STOS_TaskLock();
    #ifdef PROCESSOR_C1 /***For C1 core this delay needed otherwise STTUNER_task_lock(); not working fine ****/ 
    /*task_delay(5);*/
    #endif
    #endif
   if((Instance->Result).hier==STTUNER_HIER_LOW_PRIO)
    {
       
       Data=STTUNER_IOREG_GetField(&(Instance->DeviceMap), Instance->IOHandle, TPS_LPCODE);

    }
    else
    {
    Data=STTUNER_IOREG_GetField(&(Instance->DeviceMap), Instance->IOHandle, TPS_HPCODE);
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
    Data = STTUNER_IOREG_GetField(&(Instance->DeviceMap), Instance->IOHandle, TPS_GUARD);
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
    Data = STTUNER_IOREG_GetField(&(Instance->DeviceMap), Instance->IOHandle, INV_SPECTR);

    switch (Data)
    {
        case 0:  CurSpectrum = STTUNER_INVERSION;      break;
        default: CurSpectrum = STTUNER_INVERSION_NONE; break;
    }

    /* Get the correct frequency */
    CurFrequency = TunerInfo_p->Frequency;
    
/***********************/

 /********Frequency offset calculation done here*******************/
    signvalue=STTUNER_IOREG_GetField(&(Instance->DeviceMap), Instance->IOHandle,SEXT);
    
    offset=STTUNER_IOREG_GetRegister(&(Instance->DeviceMap), Instance->IOHandle, R_CRL_FREQ3);
    offset <<=16;
    
    offsetvar=STTUNER_IOREG_GetRegister(&(Instance->DeviceMap), Instance->IOHandle, R_CRL_FREQ2);
    offsetvar <<=8;
    offset = offset | offsetvar;
    offsetvar=0;
    offsetvar=STTUNER_IOREG_GetRegister(&(Instance->DeviceMap), Instance->IOHandle,R_CRL_FREQ1);
    offset =offset | offsetvar ;
    if(signvalue==1)
    {
       offset |=0xff000000;/*** Here sign extension made for the negative number**/
    }
       offset =offset /16384;/***offset calculation done according to data sheet of STV0360***/
       /*Data = STTUNER_IOREG_FieldGetVal(&(Instance->DeviceMap),TPS_MODE);*/
       Data = STTUNER_IOREG_GetField(&(Instance->DeviceMap), Instance->IOHandle,TPS_MODE);
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
       offset= offset*2; /*** In 361 actual offset shown is half***/
       /*********** Spectrum Inversion Taken Care**************/
       Data = STTUNER_IOREG_GetField(&(Instance->DeviceMap), Instance->IOHandle,INV_SPECTR);
       if((offset>=140) && (offset<=180))
       {
       	 offset -=166;





/***************/

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
     CurEchoPos=STTUNER_IOREG_GetField(&(Instance->DeviceMap), Instance->IOHandle, LONG_ECHO);


     #ifndef ST_OSLINUX
     STOS_TaskLock();
    
    #ifdef PROCESSOR_C1 /***For C1 core this delay needed otherwise STTUNER_task_lock(); not working fine ****/ 
    /*task_delay(5);*/
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
  
 #ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
    STTBX_Print(("F=%d SQ=%u BER=%u Modul=%u Mode=%u FR=%u G=%u Sp=%u\n",CurFrequency,CurSignalQuality,CurBitErrorRate,CurModulation,CurMode,CurFECRates,CurGuard,CurSpectrum));
 #endif
    
    


    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0361_GetSignalQuality()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0361_GetSignalQuality(DEMOD_Handle_t Handle, U32  *SignalQuality_p, U32 *Ber)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
   const char *identity = "STTUNER d0361.c demod_d0361_GetSignalQuality()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0361_InstanceData_t *Instance;

    /* private driver instance data */
    Instance = D0361_GetInstFromHandle(Handle);

    /* Read noise estimations for C/N and BER */
     FE_361_GetNoiseEstimator(&(Instance->DeviceMap), Instance->IOHandle, SignalQuality_p, Ber);


#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
    STTBX_Print(("%s SignalQuality=%u Ber=%u\n", identity, *SignalQuality_p, *Ber));
#endif
     Error = Instance->DeviceMap.Error;

    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0361_GetModulation()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0361_GetModulation(DEMOD_Handle_t Handle, STTUNER_Modulation_t *Modulation)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
   const char *identity = "STTUNER d0361.c demod_d0361_GetModulation()";
#endif
    U32 Data;
    STTUNER_Modulation_t CurModulation;
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0361_InstanceData_t *Instance;

    /* private driver instance data */
    Instance = D0361_GetInstFromHandle(Handle);

    /* Get the modulation type */
   /*Use IOREG Call instead of chip call */
    STTUNER_IOREG_GetField(&(Instance->DeviceMap), Instance->IOHandle, TPS_CONST);
   
Data = STTUNER_IOREG_GetField(&(Instance->DeviceMap),Instance->IOHandle, TPS_CONST);

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

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
    STTBX_Print(("%s Modulation=%u\n", identity, *Modulation));
#endif
   


    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0361_GetMode()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0361_GetMode(DEMOD_Handle_t Handle, STTUNER_Mode_t *Mode)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
   const char *identity = "STTUNER d0361.c demod_d0361_GetMode()";
#endif
    U8 Data;
    STTUNER_Mode_t CurMode;
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0361_InstanceData_t *Instance;

    /* private driver instance data */
    Instance = D0361_GetInstFromHandle(Handle);

    /* Get the mode type */
    
      Data=STTUNER_IOREG_GetField(&(Instance->DeviceMap),Instance->IOHandle,TPS_MODE);
   

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

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
    STTBX_Print(("%s Mode=%u\n", identity, *Mode));
#endif
    


    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0361_GetAGC()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0361_GetAGC(DEMOD_Handle_t Handle, S16  *Agc)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
   const char *identity = "STTUNER d0361.c demod_d0361_GetAGC()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    S16 Data1=0;
    S16 Data2=0;
    D0361_InstanceData_t *Instance;

    /** fix done here for the bug GNBvd20972 **
     ** where body of the function demod_d0361_GetAGC written **
     ** and the function now returns the core gain **/
        
    /* private driver instance data */
    Instance = D0361_GetInstFromHandle(Handle);
    Data1 =  STTUNER_IOREG_GetField(&(Instance->DeviceMap), Instance->IOHandle, AGC_GAIN_LO);
    
    Data2 = STTUNER_IOREG_GetField(&(Instance->DeviceMap), Instance->IOHandle, AGC_GAIN_HI);





   
    Data2 <<=8;
    Data2 &= 0x0F00;
    Data2 |= Data1 ;
      
    *Agc=Data2 ;
    


        
    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0361_GetFECRates()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0361_GetFECRates(DEMOD_Handle_t Handle, STTUNER_FECRate_t *FECRates)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
   const char *identity = "STTUNER d0361.c demod_d0361_GetFECRates()";
#endif
    U8 Data;
    STTUNER_FECRate_t CurFecRate;
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0361_InstanceData_t *Instance;

    /* private driver instance data */
    Instance = D0361_GetInstFromHandle(Handle);

    CurFecRate = 0;
     STTUNER_IOREG_GetField(&(Instance->DeviceMap), Instance->IOHandle, TPS_HPCODE);
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

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
    STTBX_Print(("%s FECRate=%u\n", identity, CurFecRate));
#endif
    


    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0361_GetGuard()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0361_GetGuard(DEMOD_Handle_t Handle, STTUNER_Guard_t *Guard)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
   const char *identity = "STTUNER d0361.c demod_d0361_GetGuard()";
#endif
    U8 Data;
    STTUNER_Guard_t CurGuard;
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0361_InstanceData_t *Instance;

    /* private driver instance data */
    Instance = D0361_GetInstFromHandle(Handle);

    CurGuard = 0;
     Data = STTUNER_IOREG_GetField(&(Instance->DeviceMap),Instance->IOHandle, TPS_GUARD);


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

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
    STTBX_Print(("%s Guard=%u\n", identity, CurGuard));
#endif
    


    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0361_IsLocked()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0361_IsLocked(DEMOD_Handle_t Handle, BOOL *IsLocked)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
   const char *identity = "STTUNER d0361.c demod_d0361_IsLocked()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0361_InstanceData_t *Instance;

    /* private driver instance data */
    Instance = D0361_GetInstFromHandle(Handle);

      *IsLocked = STTUNER_IOREG_GetField(&(Instance->DeviceMap),Instance->IOHandle, LK) ? TRUE : FALSE;



#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
    STTBX_Print(("%s IsLocked=%u\n", identity, *IsLocked));
#endif
    


    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0361_Tracking()

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
ST_ErrorCode_t demod_d0361_Tracking(DEMOD_Handle_t Handle, BOOL ForceTracking, U32 *NewFrequency, BOOL *SignalFound)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
   const char *identity = "STTUNER d0361.c demod_d0361_Tracking()";
#endif
    FE_361_Error_t           Error = ST_NO_ERROR;
    D0361_InstanceData_t    *Instance;
   

    Instance = D0361_GetInstFromHandle(Handle);

   FE_361_Tracking(&(Instance->DeviceMap), Instance->IOHandle,&Instance->Result);



#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
    STTBX_Print(("%s SignalFound=%u NewFrequency=%u\n", identity, *SignalFound, *NewFrequency));
#endif
    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0361_ScanFrequency()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0361_ScanFrequency   (DEMOD_Handle_t Handle,
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
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
   const char *identity = "STTUNER d0361.c demod_d0361_ScanFrequency()";
#endif
    ST_ErrorCode_t              Error = ST_NO_ERROR;
    FE_361_Error_t error = FE_361_NO_ERROR;
    STTUNER_tuner_instance_t    *TunerInstance;
    STTUNER_InstanceDbase_t     *Inst;
    D0361_InstanceData_t        *Instance;
    
    FE_361_SearchParams_t       Search;
    
        
    /* private driver instance data */
    Instance = D0361_GetInstFromHandle(Handle);

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
    error=FE_361_LookFor(&(Instance->DeviceMap), Instance->IOHandle, &Search, &Instance->Result, TunerInstance);
    

     
     if(error == FE_361_BAD_PARAMETER)
    {


       
       Error= ST_ERROR_BAD_PARAMETER;
      
       return Error;
    }
    
    else if (error == FE_361_SEARCH_FAILED)
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
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
        STTBX_Print(("%s NewFrequency=%u\t", identity, *NewFrequency));

#endif

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
    STTBX_Print(("%s ScanSuccess=%u\n", identity, *ScanSuccess));
#endif
	}
     return(ST_NO_ERROR);


}



/* ----------------------------------------------------------------------------
Name: demod_d0361_ioctl()

Description:
    access device specific low-level functions

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0361_ioctl(DEMOD_Handle_t Handle, U32 Function, void *InParams, void *OutParams, STTUNER_Da_Status_t *Status)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
   const char *identity = "STTUNER d0361.c demod_d0361_ioctl()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0361_InstanceData_t *Instance;

    /* private driver instance data */
    Instance = D0361_GetInstFromHandle(Handle);

    if(STTUNER_Util_CheckPtrNull(Instance) != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
        STTBX_Print(("%s invalid handle\n", identity));
#endif
        return(ST_ERROR_INVALID_HANDLE);
    }

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
        STTBX_Print(("%s demod driver instance handle=%d\n", identity, Handle));
        STTBX_Print(("%s                     function=%d\n", identity, Function));
        STTBX_Print(("%s                   I/O handle=%d\n", identity, Instance->IOHandle));
#endif

    /* ---------- select what to do ---------- */
    switch(Function){

        case STTUNER_IOCTL_REG_IN: /* read device register */
             *(int *)OutParams = STTUNER_IOREG_GetRegister(&(Instance->DeviceMap),Instance->IOHandle, *(int *)InParams);

            break;

        case STTUNER_IOCTL_REG_OUT: /* write device register */
             Error =  STTUNER_IOREG_SetRegister(&(Instance->DeviceMap),Instance->IOHandle,
                  ((TERIOCTL_SETREG_InParams_t *)InParams)->RegIndex,
                  ((TERIOCTL_SETREG_InParams_t *)InParams)->Value );
            break;

        default:
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
            STTBX_Print(("%s function %d not found\n", identity, Function));
#endif
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }


#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
    STTBX_Print(("%s called ok\n", identity, Function));    /* signal that function came back */
#endif
    


    return(Error);

}



/* ----------------------------------------------------------------------------
Name: demod_d0361_ioaccess()

Description:
    called from ioarch.c when some driver does I/O but with the repeater/passthru
    set to point to this function.
Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0361_ioaccess(DEMOD_Handle_t Handle, IOARCH_Handle_t IOHandle, STTUNER_IOARCH_Operation_t Operation, U16 SubAddr, U8 *Data, U32 TransferSize, U32 Timeout)

{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
   const char *identity = "STTUNER d0361.c demod_d0361_ioaccess()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    IOARCH_Handle_t ThisIOHandle;
    D0361_InstanceData_t *Instance;
    STTUNER_InstanceDbase_t     *Inst;
    STTUNER_tuner_instance_t    *TunerInstance;

    Instance = D0361_GetInstFromHandle(Handle);
    Inst = STTUNER_GetDrvInst();

    /* get the tuner instance for this driver from the top level handle */
    TunerInstance = &Inst[Instance->TopLevelHandle].Terr.Tuner;
    if(STTUNER_Util_CheckPtrNull(Instance) != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
        STTBX_Print(("%s invalid handle\n", identity));
#endif
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* this (demod) drivers I/O handle */
    ThisIOHandle = Instance->IOHandle;

    /* if STTUNER_IOARCH_MAX_HANDLES then passthrough function required using our device handle/address */
    if (IOHandle == STTUNER_IOARCH_MAX_HANDLES)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
        STTBX_Print(("%s write passthru\n", identity));
#endif
        Error = STTUNER_IOARCH_ReadWrite(ThisIOHandle, Operation, SubAddr, Data, TransferSize, Timeout);
    }
    else    /* repeater */
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
        STTBX_Print(("%s write repeater\n", identity));
#endif

         #ifndef ST_OSLINUX
         STOS_TaskLock();    /* prevent any other activity on this bus (no task must preempt us) */
        
        /* enable repeater then send the data  using I/O handle supplied through ioarch.c */
        #ifdef PROCESSOR_C1 /***For C1 core this delay needed otherwise STTUNER_task_lock(); not working fine ****/ 
            /*task_delay(5);*/
         #endif
         #endif
       Error = STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle, STOP_ENABLE, 1);
         Error = STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle, I2CT_ON, 1);           
       

        /* send callers data using their address. (nb: subaddress == 0)
         Function 'STTUNER_IOARCH_ReadWriteNoRep' is called as calling the normal 'STTUNER_IOARCH_ReadWrite'
        function would cause it to call the redirection function which is THIS function, and around we
        would go forever. */
        
       
       	Error = STTUNER_IOARCH_ReadWriteNoRep(IOHandle, Operation,0, Data, TransferSize, Timeout);
	

       #ifndef ST_OSLINUX
       STOS_TaskUnlock();
       #endif
    }

    return(Error);
}

/* ----------------------------------------------------------------------------
Name: demod_d0361_GetTPSCellId()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */

ST_ErrorCode_t demod_d0361_GetTPSCellId(DEMOD_Handle_t Handle, U16  *TPSCellId)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
   const char *identity = "STTUNER d0361.c demod_d0361_GetTPSCellId()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0361_InstanceData_t *Instance;
    BOOL CELL_ID_FOUND_MSB ;
    BOOL CELL_ID_FOUND_LSB ;
    int Cell_ID_LSB=0 , Cell_ID_MSB=0  ;
    int Counter=0;
    int Tempbuf_MSB[2] = {0};
    int Tempbuf_LSB[2] = {0}; 
    int Counter_MSB =0 , Counter_LSB =0 , Tempbuf_MSB_Fill_Flag =0 ,Tempbuf_LSB_Fill_Flag=0;
    int Var_CellID_MSB = 0 ,Var_CellID_LSB = 0 ;
    int TPSCellIDTemp =0;
    int FrameId , TPS_Id_Cell_HI =0 , TPS_Id_Cell_LO = 0;
	U8 tps_rcvd[7];     

    /* private driver instance data */
    Instance = D0361_GetInstFromHandle(Handle);
 
   
    CELL_ID_FOUND_MSB=FALSE;
    CELL_ID_FOUND_LSB = FALSE;
    for(Counter =0 ;Counter <= 600 ; Counter++)
    {    
      #ifndef ST_OSLINUX
      STOS_TaskLock();    /* prevent any other activity on this bus (no task must preempt us) */
                         /* enable repeater then send the data  using I/O handle supplied through ioarch.c */
    #ifdef PROCESSOR_C1 /***For C1 core this delay needed otherwise STTUNER_task_lock(); not working fine ****/ 
   /* task_delay(5); */
    #endif
    #endif
       
      STTUNER_IOREG_GetContigousRegisters(&(Instance->DeviceMap),Instance->IOHandle,R_TPS_RCVD1,7,tps_rcvd);
        FrameId =((tps_rcvd[0]&0x03));
       
        TPS_Id_Cell_HI =tps_rcvd[5];
       
        TPS_Id_Cell_LO =tps_rcvd[4];
         

     #ifndef ST_OSLINUX
       STOS_TaskUnlock();
    #endif
       if (CELL_ID_FOUND_MSB ==FALSE)
       {
          if((FrameId==2) && (TPS_Id_Cell_HI == TPS_Id_Cell_LO))
          {
             Var_CellID_MSB = TPS_Id_Cell_HI; 
             if ( Counter_MSB == 0)
             {
               if(Tempbuf_MSB_Fill_Flag ==0)
               {
                  Tempbuf_MSB[0]= Var_CellID_MSB;
                  Tempbuf_MSB_Fill_Flag ++;
             
               }
               else
               {
                  Tempbuf_MSB[1] = Var_CellID_MSB;
                  if (Tempbuf_MSB[0] == Tempbuf_MSB[1] )
                  {
                     Counter_MSB ++;
                  
                  }
                  else
                  {
                     Counter_MSB = 0;
                     Tempbuf_MSB_Fill_Flag = 0 ;
                
                  }
              
               }
         
         }/* end of Counter_MSB == 0 */
         else
         {
            Tempbuf_MSB[1] = Var_CellID_MSB;
            if (Tempbuf_MSB[0] == Tempbuf_MSB[1] )
               {
                  
                  Counter_MSB ++;
                  if ( Counter_MSB > 3)
                  {
                     CELL_ID_FOUND_MSB = TRUE ;
                     Cell_ID_MSB = Tempbuf_MSB[1];
                  }
                  
               }
               else
               {
                  Counter_MSB = 0;
                  Tempbuf_MSB_Fill_Flag = 0 ;
               }
               
         }
         
      }/* end of frameid if */
    }/* end of if (CELL_ID_FOUND_MSB ==FALSE) */
    if (CELL_ID_FOUND_LSB ==FALSE)
    {
      if((FrameId==3) && (TPS_Id_Cell_HI == TPS_Id_Cell_LO))
      {
         Var_CellID_LSB = TPS_Id_Cell_HI;
         if ( Counter_LSB == 0)
         {
            if(Tempbuf_LSB_Fill_Flag ==0)
            {
               Tempbuf_LSB[0]= Var_CellID_LSB;
               Tempbuf_LSB_Fill_Flag ++;
            }
            else
            {
               Tempbuf_LSB[1] = Var_CellID_LSB;
               if (Tempbuf_LSB[0] == Tempbuf_LSB[1] )
               {
                  Counter_LSB ++;
                  
               }
               else
               {
                  Counter_LSB = 0;
                  Tempbuf_LSB_Fill_Flag = 0 ;
               }
              
            }
         
         }/* end of Counter_LSB == 0 */
         else
         {
            Tempbuf_LSB[1] = Var_CellID_LSB;
            if (Tempbuf_LSB[0] == Tempbuf_LSB[1] )
               {
                  Counter_LSB ++;
                  if ( Counter_LSB > 3 )
                  {
                     CELL_ID_FOUND_LSB = TRUE ;
                     Cell_ID_LSB = Tempbuf_LSB[1];
                    
                  }
                   
               }
               else
               {
                  Counter_LSB = 0;
                  Tempbuf_LSB_Fill_Flag = 0 ;
               }
               
         }
       }/* end if FrameId */
    }/* end of if (CELL_ID_FOUND_LSB ==FALSE) */
    




   
    if ((CELL_ID_FOUND_MSB ==TRUE ) && (CELL_ID_FOUND_LSB ==TRUE )) 
    {
            break;
    }
         
   }/* for loop */
   TPSCellIDTemp = Cell_ID_MSB;
   TPSCellIDTemp <<= 8;
   TPSCellIDTemp |= Cell_ID_LSB;
   *TPSCellId = TPSCellIDTemp;
   if(Counter >600)
   {
      Error = ST_ERROR_TIMEOUT;
   }

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
    STTBX_Print(("%s IsLocked=%u\n", identity, *IsLocked));
#endif

    


    return(Error);
}

/* ----------------------------------------------------------------------------
Name: demod_d0361_StandByMode()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */

ST_ErrorCode_t demod_d0361_StandByMode(DEMOD_Handle_t Handle, STTUNER_StandByMode_t PowerMode)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361
   const char *identity = "STTUNER d0361.c demod_d0361_GetTPSCellId()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0361_InstanceData_t *Instance;
       
    /* private driver instance data */
    Instance = D0361_GetInstFromHandle(Handle);
     
    switch ( PowerMode)
    {
       case STTUNER_NORMAL_POWER_MODE :
       if(Instance->StandBy_Flag == 1)
       {
          
            
            Error|=STTUNER_IOREG_SetField(&(Instance->DeviceMap),Instance->IOHandle,STDBY,0);
  
               
            
           Error|= STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle, CORE_ACTIVE, 0);
           WAIT_N_MS(2);
                      
           Error|= STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle, CORE_ACTIVE, 1);
  
           
         if(Error==ST_NO_ERROR)
          {
             Instance->StandBy_Flag = 0 ;
          }
          break;
       }
       
       case STTUNER_STANDBY_POWER_MODE :
       	
       	if(Instance->StandBy_Flag == 0)
        {
       	  
       	   Error|= STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle, STDBY, 1);
          
            if(Error==ST_NO_ERROR)
            {
               Instance->StandBy_Flag = 1 ;
            }
           break ;
        }
       
    }/* Switch statement */ 
  
   


    return(Error);
}

/* ------------------------------------------------------------------------- */
/* /\/\/\/\/\/\/\/\/\/\/\/\/\UTILITY Functions/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ */
/* ------------------------------------------------------------------------- */

D0361_InstanceData_t *D0361_GetInstFromHandle(DEMOD_Handle_t Handle)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361_HANDLES
   const char *identity = "STTUNER d0361.c D0361_GetInstFromHandle()";
#endif
    D0361_InstanceData_t *Instance;

    Instance = (D0361_InstanceData_t *)Handle;
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0361_HANDLES
    STTBX_Print(("%s block at 0x%08x\n", identity, Instance));
#endif

    return(Instance);
}




/* End of d0361.c */
