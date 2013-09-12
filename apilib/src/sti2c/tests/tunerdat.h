/******************************************************************************

File name   : tunerdat.h

Description : Data definition for Tuner reg (299, 399, 360)	i2ctest.c code 
			  for I2C (SSC) interface

COPYRIGHT (C) STMicroelectronics 2004

******************************************************************************/
#ifndef __TUNERDAT_H
#define __TUNERDAT_H

/* In register values [1] specifies that these registers are either read only or 
value in these registers keep changing, so values read from these registers are
discarded when comparing. These values are taken from following paths 
dvdbr-prj-sttuner\drivers\sat\demod\stv0299\reg0299.c 
dvdbr-prj-sttuner\drivers\sat\demod\stv0399\d0399.c
dvdbr-prj-sttuner\drivers\ter\demod\stv0360\d0360.c
*/


U8 RegData299[][2]={{1,0xA1},
                    {0,0x15},
                    {0,0x00},
                    {0,0x00},
                    {0,0x7D},
                    {0,0x05},
                    {0,0x02},
                    {0,0x00},
                    {0,0x40},
                    {0,0x00},			
                    {1,0x82},   /* DiSEqC Status */
                    {0,0x00},
                    {0,0x51},
                    {0,0x81},
                    {0,0x23},
                    {0,0x1A},
                    {0,0x34},
                    {0,0x84},
                    {0,0xB9},
                    {0,0x9B},
                    {0,0x9E},
                    {0,0xE3},
                    {1,0x80},   /* AGC1I */
                    {1,0x12},
                    {1,0xFF},   /* ACG2I1 */
                    {1,0xFF},
                    {1,0x82},
                    {1,0x00},
                    {1,0x7F},
                    {1,0x00},
                    {1,0x00},
                    {0,0x00},
                    {0,0x00},
                    {0,0x00},
                    {1,0x80},
                    {1,0x0B},
                    {1,0x2B},
                    {1,0x75},
                    {1,0x1A},   /* VERROR */
                    {1,0x00},   /* res */
                    {0,0x00},   /* FECM */
                    {0,0x1E},   /* VTH0 */
                    {0,0x14},
                    {0,0x0F},
                    {0,0x09},
                    {0,0x05},
                    {1,0x00},   /* res */
                    {1,0x00},   /* res */
                    {1,0x00},   /* res */
                    {1,0x1F},   /* PR */
                    {0,0x19},
                    {0,0xFC},
                    {0,0x13},   /* ERRCNT */
};


/* Problem in few bytes such as 0x08,0x1d,0x1e,0x21,0x22 and 0x2d */

U8 RegData399[][2]={{1,0xb4},
                    {1,0x07},
                    {0,0x2a},   /*ACR */
                    {0,0x8e},   /*F22FR */
                    {0,0xa2},   /*DACR1*/
                    {0,0x00},   /*DACR2*/
                    {0,0x60},   /*DiSEqC*/
                    {1,0x00},   /*DiSEqCFIFO */
                    {1,0x02},   /* First row */ /*DiSEqCSTATUS*/
                    {1,0x00},
                    {1,0x00},   /*IOCFG1*//*Accesible in stand By Mode*/
                    {1,0x50},   /*IOCFG2*//*Accesible in stand By Mode*/ 
                    {0,0x01},   /*AGC0C*/
                    {0,0x51},   /*AGC0R*//*0x0D*/
                    {0,0xe9},   /*AGC1C*/
                    {0,0x81},   /*AGC1CN*/
                    {0,0x23},   /*RTC*/
                    {0,0x14},   /*second row */ /*0x11*//*AGC1R*/
                    {1,0x14},
                    {0,0x74},   /*AGC2O*//*0x13*/
                    {0,0x84},   /*0x14*//*TLSR*/
                    {0,0xd9},   /*CFD*/ /*0x15*/
                    {0,0x87},   /*ACLC*//*0x16*/
                    {0,0xa8},   /*BCLC*//*0x17*/
                    {1,0x03},   /*R8PSK*/ /*0x18*/
                    {0,0xdd},   /*LDT*/ /*0x19*/
                    {0,0xc9},   /*Third row *//*1A*//*LDT2*/
                    {0,0xa0},   /*AGC0CMD*/
                    {0,0x05},   /*AGC0I */
                    {1,0xc3},   /*AGC1S*//*0x1D*/
                    {1,0xbf},   /*AGC1P *//*0x1E*/
                    {1,0x80},
                    {1,0x78},   /*0x20*//*TLIR*/
                    {1,0x41},   /*0x21*//*AGC2I1*/
                    {1,0x7e},   /*AGC2I2*/ /*0x22*/
                    {0,0xff},   /*Fourth row *//*0x23*/ /*RTF */
                    {1,0x9a},   /*VSTATUS*//*0x24*//*read*/
                    {1,0x7f},   /*read*//*0x25*//*LDI*/
                    {1,0x00},   /*ECNTM*//*0x26*//*read*/
                    {1,0x00},   /*ECNTL*//*0x27*//*read*/
                    {0,0x41},   /*0x28*//*SFRH*/
                    {0,0x2f},   /*SFRM*//*0x29*/
                    {0,0x60},   /*SFRL*//*0x2A*/
                    {0,0xff},   /*CFRM*//*0x2b*/
                    {1,0x17},   /*fifth row *//*CFRL*//*0x2c*/
                    {1,0x0d},   /*NIRH*//*0x2D*//*Read*/
                    {1,0xc4},   /*NIRL*//*0x2E*//*Read*/
                    {1,0x00},   /*VERROR*//*0x2f*//*Read*/
                    {0,0x01},   /*FECM*//*0x30*/
                    {0,0x1e},   /*VTH0*//*0x31*/
                    {0,0x14},   /*VTH1*//*0x32*/
                    {0,0x0f},   /*VTH2*//*0x33*/
                    {0,0x09},   /*VTH3*//*0x34*/
                    {0,0x0c},   /*Sixth row *//*VTH4*//*0x35*/
                    {0,0x05},   /*VTH5*//*0x36*/
                    {0,0x2f},   /*PR*//*0x37*/
                    {0,0x19},   /*VSEARCH*//*0x38*/
                    {0,0xbc},   /*RS*//*0x39*/
                    {0,0x10},   /*RSOUT*//*0x3A*/
                    {0,0xb3},   /*ERRCTRL*//*0x3B*/
                    {0,0x01},   /*VITPROG*//*0x3c*/
                    {0,0x31},   /*ERRCTRL2*//*0x3d*/
                    {1,0x00},   /*Seventh row *//*ECNTM2*//*0x3e*//*Read*/
                    {1,0x00},   /*ECNTL2*//*0x3f*//*Read*/
                    {0,0x40},   /*DCLK1*//*0x40*/
                    {0,0x3f},   /*LPF*//*0x41*/
                    {0,0x08},   /*DCLK2*//*0x42*/
                    {0,0x7a},   /*ACOARSE*//*0x43*/
                    {0,0x03},   /*AFINEMSB*//*0x44*/
                    {0,0xff},   /*AFINELSB*//*0x45*/
                    {0,0x51},   /*SYNTCTRL*//*0x46*/
                    {0,0x00},   /*eight row */ /*SYNTCTRL2*//*0x47*/
                    {0,0x0e},   /*SYSCTRL*//*0x48*/
                    {1,0x8f},   /*AGC1EP*//*0x49*//*Read*/
                    {1,0xf6}    /*AGC1ES*//*0x4A*//*Read*/
};


/* Not all registers are tested for 360 */

U8 RegData360[][2]={{1,0x21},   /*ID*/
                    {0,0x27},   /*I2CRPT*/
                    {1,0x02},   /*TOPCTRL*/
                    {0,0x40},   /*IOCFG0 *//*0x04*/
                    {0,0x00},   /*DAC0R*/
                    {0,0x40},   /*IOCFG1*/
                    {0,0x00},   /*DAC1R*//*0x06*/
                    {0,0x60},   /*IOCFG2 */
                    {0,0x00},   /*PWMFR*//*,0x08*/
                    {0,0xf9},   /*STATUS*/
                    {0,0x1C},   /*AUX_CLK*//*0x0A*/
                    {1,0x00},   /*Res*//*FREESYS1*/
                    {1,0x00},   /*Res*//*FREESYS2*/
                    {1,0x00},   /*Res*//*FREESYS3*/
                    {1,0x55},   /* Check */
                    {1,0x55},   /* Check */
                    {0,0xff},   /*AGC2MAX*//*0x10*/
                    {0,0x00},   /*AGC2MIN*/
                    {0,0x00},   /*AGC1MAX*//*0x12*/
                    {0,0x00},   /*AGC1MIN*//*0x13*/
                    {0,0xbc},   /*AGCR*//*0x14*/
                    {0,0xfe},   /*AGC2TH*/
                    {0,0x00},   /*AGC12C3*//*0x16*/
                    {0,0x85},   /*AGCCTRL1*/
                    {0,0x1f},   /*AGCCTRL2*/
                    {0,0x00},   /*AGC1VAL1*//*0x19*/
                    {0,0x00},   /*AGC1VAL2*/
                    {0,0x02},   /*AGC2VAL1*/
                    {0,0x08},   /*AGC2VAL2*//*0x1C*/
                    {1,0x00},   /*AGC2PGA*//*Check*//*0x1D*/
                    {1,0x00},   /*OVF_RATE1*/
                    {1,0x00},   /*OVF_RATE2*//*0x1f*/
                    {0,0xce},   /*GAIN_SRC1*/
                    {0,0x10},   /*GAIN_SRC2*//*0x21*/
                    {0,0x55},   /*INC_DEROT1*//*0x22*/
                    {0,0x4b},   /*INC_DEROT2*//*0x23*/
                    {1,0x00},   /*0x24*/
                    {1,0x00},   /*0x25*/
                    {0,0x03},   /*FREE_ST_FE*/
                    {0,0x1c},   /*SYR_THR*//*0x27*/
                    {0,0xff},   /*INR*//*0x28*/
                    {1,0x01},   /*EN_PROCESS*/
                    {1,0xff},   /*0x2A*//*SDI_SMOOTHER*/
                    {1,0xff},   /*FE_LOOP_OPEN*//*0x2B*/
                    {1,0x00},
                    {1,0x00},
                    {1,0x00},
                    {1,0x00},   /*0x2f*/
                    {1,0x00},   /*0x30*/
                    {1,0x11},   /*EPQ*/
                    {1,0x0d},   /*EPQ2*//*0x32*/
                    {1,0x00},
                    {1,0x00},
                    {1,0x00},
                    {1,0x00},   /*0x36*/
                    {1,0x00},
                    {1,0x00},
                    {1,0x00},
                    {1,0x00},
                    {1,0x00},
                    {1,0x00},
                    {1,0x00},
                    {1,0x00},
                    {1,0x00},
                    {1,0x00},   /*0x40*//*FECM*/
                    {0,0x1E},   /*VTH0*/
                    {0,0x1E},   /*VTH1*/
                    {0,0x0F},   /*VTH2*/
                    {0,0x09},   /*VTH3*//*0x44*/
                    {0,0x00},   /*VTH4*/
                    {0,0x05},   /*0x46*//*VTH5*/
                    {1,0x00},   /*FREEVIT*/
                    {1,0x00},
                    {0,0x92},   /*VITPROG*/
                    {0,0x02},   /*PR*/
                    {0,0xb0},   /*VSEARCH*/
                    {0,0xBC},   /*RS*//*0x4C*/
                    {0,0x15},   /*RSOUT*/
                    {0,0x12},   /*ERRCTRL1*/
                    {1,0x00},   /*ERRCNTM1*//*0x4F*/
                    {1,0x00},   /*0x50*//*ERRCNTL1*/
                    {0,0x12},   /*ERRCTRL2*/
                    {1,0x00},   /*ERRCNTM2*/
                    {1,0x00},   /*ERRCNTL2*//*0x53*/
                    {1,0x00},   /*FREEDRS*/
                    {1,0x00},   /*VERROR*/
                    {0,0x12},   /*0x56*//*ERRCTRL3*/
                    {1,0x00},   /*ERRCNTM3*/
                    {1,0x00},   /*ERRCNTL3*//*0x58*/
                    {1,0x00},   /*DILSTK1*/
                    {1,0x03},   /*DILSTK0*//*0x5A*/
                    {1,0x00},   /*DILBWSTK1*/
                    {1,0x03},   /*DILBWST0*/
                    {0,0x80},   /*0x5D*//*LNBRX*/
                    {1,0xB0},   /*RSTC*/
                    {1,0x07},   /*VIT_BIST*/
                    {1,0x00},   /*0x60*/
                    {1,0x00},
                    {1,0x00},
                    {1,0x00},
                    {1,0x00},
                    {1,0x00},
                    {1,0x00},   /*0x66*/
                    {1,0x00},
                    {1,0x00},
                    {1,0x00},
                    {1,0x00},
                    {1,0x00},
                    {1,0x00},
                    {1,0x00},
                    {1,0x00},
                    {1,0x00},
                    {1,0x00},   /*0x70*/
                    {1,0x00},
                    {1,0x00},
                    {1,0x00},
                    {1,0x00},
                    {1,0x00},
                    {1,0x00},   /*0x76*/
                    {1,0x00},
                    {1,0x00},
                    {1,0x00},
                    {1,0x00},
                    {1,0x00},
                    {1,0x00},
                    {1,0x00},
                    {1,0x00},
                    {0,0x20},   /*0x80*//*COR_CTL*/
                    {1,0xf6},   /*COR_STAT*//*Read only*/
                    {0,0x00},   /*COR_INTEN*/
                    {1,0x3d},   /*COR_INTSTAT*//*Read Only*/
                    {0,0x03},   /*COR_MODEGUARD*/
                    {0,0x18},   /*AGC_CTL*//*0x85*/
                    {1,0x00},   /*0x86*//*AGC_MANUAL1*/
                    {1,0x00},   /*0x87*//*AGC_MANUAL2*/
                    {0,0x20},   /*AGC_TARGET*/
                    {1,0x09},   /*AGC_GAIN1 */
                    {1,0x10},   /*AGC_GAIN2 *//*0x8A*/
                    {1,0x00},   /*0x8B*//*ITB_CTL*/
                    {1,0x00},   /*ITB_FREQ1*/
                    {1,0x00},   /*ITB_FREQ2*/
                    {0,0x85},   /*CAS_CTL*//*0x8E*/
                    {0,0xB3},   /*CAS_FREQ*/
                    {1,0x10},   /*CAS_DAGCGAIN*//*Read Only*/
                    {0,0x00},   /*SYR_CTL*/
                    {1,0x17},   /*SYR_STAT*//*0x92*/
                    {1,0x00},   /*SYR_NCO1*/
                    {1,0x00},   /*SYR_NCO2*//*0x94*/
                    {0,0x00},   /*SYR_OFFSET1*/
                    {0,0x00},   /*SYR_OFFSET2*//*0x96*/
                    {1,0x00},   /*FFT_CTL*/
                    {0,0x00},   /*SCR_CTL*//*0x98*/
                    {0,0x30},   /*PPM_CTL1*/
                    {0,0x94},   /*TRL_CTL*//*0x9A*/
                    {0,0xb0},   /*TRL_NOMRATE1*/
                    {0,0x56},   /*TRL_NOMRATE2*//*0x9C*/
                    {1,0xee},   /*TRL_TIME1*/
                    {1,0xfd},   /*TRL_TIME2*/
                    {0,0x4f},   /*CRL_CTL*//*0x9f*/
                    {1,0x90},   /*CRL_FREQ1*//*0xA0*/
                    {1,0x9b},   /*CRL_FREQ2*/
                    {1,0x00},   /*CRL_FREQ3*/
                    {0,0xb1},   /*CHC_CTL1*/
                    {1,0xd7},   /*CHC_SNR*/
                    {0,0x00},   /*BDI_CTL*//*0xA5*/
                    {1,0x00},   /*DMP_CTL*/
                    {1,0x32},   /*TPS_RCVD1*/
                    {1,0x02},   /*TPS_RCVD2*/
                    {1,0x01},   /*TPS_RCVD3*/
                    {1,0x31},   /*TPS_RCVD4*//*0xAA*/
                    {1,0x00},   /*TPS_CELLID*/
                    {1,0x00},   /*TPS_FREE2*/
                    {1,0x01},   /*TPS_SET1*/
                    {1,0x02},   /*TPS_SET2*/
                    {1,0x04},   /*TPS_SET3*//*0xaf*/
                    {0,0x00},   /*0xB0*//*TPS_CTL*/
                    {0,0x2b},   /*CTL_FFTOSNUM*/
                    {0,0x0c},   /*CAR_DISP_SEL*/
                    {1,0x0A},   /*0xB3*//*MSC_REV*/
                    {0,0x00},   /*PIR_CTL*//*0xB4*/
                    {0,0xA8},   /*SNR_CARRIER1*/
                    {0,0x86},   /*SNR_CARRIER2*/
                    {1,0xcc},   /*PPM_CPAMP*//*0xB7*/
                    {1,0x00},   /*TSM_AP0*/
                    {1,0x00},   /*TSM_AP1*/
                    {1,0x00},   /*TSM_AP2*/
                    {1,0x00},   /*TSM_AP3*/
                    {1,0x00},   /*TSM_AP4*/
                    {1,0x00},   /*TSM_AP5*/
                    {1,0x00},   /*TSM_AP6*//*0xBE*/
                    {1,0x00},   /*0xBF*//*TSM_AP7*/
                    {1,0x00},   /*0xC0*//*TSTRES*/
                    {1,0x00},   /*ANACTRL*/
                    {1,0x00},   /*TSTBUS*/
                    {1,0x00},   /*TSTCK*//*0xC3*/
                    {1,0x00},   /*TSTI2C*/
                    {1,0x00},   /*TSTRAM1*/
                    {1,0x00},   /*TSTRATE*//*0xc6*/
                    {1,0x00},   /*SELOUT*/
                    {1,0x00},   /*FORCEIN*//*0xC8*/
                    {1,0x00},   /*TSTFIFO*/
                    {1,0x00},   /*0xCA*//*TSTRS*/
                    {0,0x02},   /*CONSTMODE*//*0xCB*/
                    {0,0xD2},   /*CONSTCARR1*/
                    {0,0x04},   /*CONSTCARR2*/
                    {1,0xf7},   /*ICONSTEL*//*0xCE*//*Read*/
                    {1,0xf8},   /*QCONSTEL*//*0xCF*//*Read*/
                    {1,0x00},   /*0xD0*//*TSTBISTRES0*/
                    {1,0x00},   /*TSTBISTRES1*/
                    {1,0x00},   /*TSTBISTRES2*/
                    {1,0x00},   /*0xD3*//*TSTBISTRES3*/
                    {1,0xff},   /*0xD4*//*RF_AGC1*/
                    {0,0x83},   /*EN_RF_AGC1*//*0xD5*/
                    {1,0x00},
                    {1,0x00},
                    {1,0x00},
                    {1,0x00},   /*0xD9*/
                    {1,0x00},   /*0xDA*/
                    {1,0x00},
                    {1,0x00},
                    {1,0x00},
                    {1,0x00},
                    {1,0x00}    /*0xDF*/
};

#ifdef __cplusplus
}
#endif

#endif /* __TUNERDAT_H */
/*EOF*/
