/* ----------------------------------------------------------------------------
File Name: d0362.c

Description:

    stv0362 Terrestrial COFDM driver


Copyright (C) 2006-2007 STMicroelectronics

History:

   date:
version:
 author:
comment:

Reference:


---------------------------------------------------------------------------- */


/* Includes ---------------------------------------------------------------- */

/* C libs */

#ifndef ST_OSLINUX
#include <string.h>

/* STAPI */
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
#include "chip.h"
/* LLA */


#include "d0362_echo.h"
#include "d0362_drv.h"    /* misc driver functions */
#include "d0362.h"      /* header for this file */
#include "d0362_map.h"
#include "tioctl.h"     /* data structure typedefs for all the ter ioctl functions */

 U16  STV0362_Address[STV0362_NBREGS]=
{
0x00d9,
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
0x0037,
0x0038,
0x0039,
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
0x0060,
0x0061,
0x0062,
0x0063,
0x0064,
0x0065,
0x0066,
0x0067,
0x0068,
0x0069,
0x006a,
0x006b,
0x006c,
0x006d,
0x006e,
0x0070,
0x0071,
0x0072,
0x0073,
0x0074,
0x0075,
0x0076,
0x0077,
0x0078,
0x0079,
0x007a,
0x007b,
0x007c,
0x007d,
0x007e,
0x007f,
0x006f,
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
0x00c0,
0x00c1,
0x00c2,
0x00c6,
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
0x00d7,
0x00d8,
0x00da,
0x00db,
0x00dc,
0x00dd,
0x00de,
0x00df,
0x0000
};

#define MAX_ADDRESS 0x00df

 U8  STV0362_DefVal[STV0362_NBREGS]=
{
#ifdef STTUNER_DRV_TER_TUN_MT2266
/*0x0f,0x41,0x22,0x02,0x40,0x00,0x00,0x00,0x00,0x00,0xfa,0x11,0x00,0x00,0x00,0xf8,0x50,0xf8,0x50,0xbc,0x78,0x90,0x85,0x00,0x8f,0x0f,0x14,0x09,0x00,0x00,0x00,0xd8,0x98,0x55,0xa0,0x58,0x00,0x00,0x1c,0x00,0xb1,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x0b,0x09,0x03,0x07,0x23,0xc0,0x89,0x00,0x02,0x00,0x1e,0x14,0x0f,0x09,0x00,0x05,0x00,0x92,0x2f,0x34,0xbc,0x15,0x12,0x00,0x00,0x32,0x00,0x00,0x00,0x00,0x12,0x00,0x00,0x00,0x03,0x00,0x03,0x80,0xb0,0x07,0x85,0x00,0x00,0x0e,0xcc,0x0e,0xcc,0x00,0x00,0x36,0x47,0x01,0x25,0x00,0x00,0xec,0x0f,0xa1,0xb3,0x39,0x10,0x01,0x40,0xf4,0xf2,0x23,0x00,0x02,0x00,0x00,0x00,0xc0,0x20,0xf6,0x00,0x3d,0x00,0x18,0x00,0x00,0x16,0x18,0x11,0x00,0x00,0x00,0x40,0xb3,0x10,0x04,0x15,0x00,0x00,0x00,0x00,0x00,0x00,0x38,0x14,0xb2,0x56,0x20,0x02,0x4f,0x94,0xf6,0xfd,0x11,0xea,0x00,0x00,0x33,0x01,0x02,0x11,0x00,0x00,0x00,0x01,0x02,0x00,0x08,0x00,0x0a,0x00,0x00,0x80,0xbb,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x02,0x00,0x00,0x00,0x3d,0x83,0x01,0x00,0x10,0x08,0x00,0x00,0x00,0xe0,0x41*/
0x0f,0x41,0x22,0x02,0x40,0x00,0x00,0x00,0x00,0x00,0xf1,0x11,0x00,0x00,0x00,0xf8,0x50,0xf8,0x50,0xbc,0x78,0x90,0x85,0x00,0x56,0x0c,0x30,0x06,0x00,0x00,0x00,0xd8,0x98,0x55,0xa0,0x5e,0x00,0x00,0x1c,0x00,0xb1,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xff,0x03,0x05,0x23,0xc0,0x89,0x00,0x22,0x00,0x1e,0x14,0x0f,0x09,0x00,0x05,0x00,0x92,0xaf,0x30,0xbc,0x15,0x12,0x00,0x00,0x32,0x00,0x00,0x00,0x12,0x12,0x00,0x00,0x00,0x03,0x00,0x03,0x40,0xb0,0x07,0x85,0x00,0x00,0x0e,0xcc,0x0e,0xcc,0x00,0x00,0x36,0x47,0x01,0x25,0x00,0x00,0xec,0x0f,0xa1,0xb3,0x39,0x10,0x01,0x40,0xf4,0xf2,0x23,0x00,0x02,0x00,0x00,0x00,0xc0,0x20,0xf6,0x00,0x3d,0x00,0x18,0x00,0x00,0x16,0x3c,0x1e,0x00,0x00,0x00,0x40,0xb3,0x10,0x04,0x14,0x00,0x00,0x00,0x00,0x00,0x00,0x38,0x14,0xb2,0x56,0x17,0x03,0x4f,0xe5,0xfe,0xfd,0x11,0x78,0x00,0x00,0x33,0x02,0x01,0x01,0x00,0x00,0x02,0x02,0x01,0x00,0x34,0x00,0x0a,0x00,0x00,0x80,0xc5,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xc0,0x02,0x00,0x00,0x00,0x4c,0x81,0x01,0x00,0x10,0x08,0x00,0x00,0x00,0xc0,0x41/*default from GUI*/
#else
0x0f,/*	 PLLNDIV            	0x00d9*/
0x40,/*	 ID                 	0x0000*/
0x22,/*	 I2CRPT             	0x0001*/
0x01,/*	 TOPCTRL            	0x0002*/
0x40,/*	 IOCFG0             	0x0003*/
0x00,/*	 DAC0R              	0x0004*/
0x00,/*	 IOCFG1             	0x0005*/
0x00,/*	 DAC1R              	0x0006*/
0x00,/*	 IOCFG2             	0x0007*/
0x00,/*	 SDFR               	0x0008*/
0xfa,/*	 STATUS             	0x0009*/
0x0b,/*	 AUX_CLK            	0x000a*/
0x00,/*	 FREESYS1           	0x000b*/
0x00,/*	 FREESYS2           	0x000c*/
0x00,/*	 FREESYS3           	0x000d*/
0xff,/*	 AGC2MAX            	0x0010*/
0x00,/*	 AGC2MIN            	0x0011*/
0xff,/*	 AGC1MAX            	0x0012*/
0x00,/*	 AGC1MIN            	0x0013*/
0xbc,/*	 AGCR               	0x0014*/
0x00,/*	 AGC2TH             	0x0015*/
0x40,/*	 AGC12C             	0x0016*/
0x85,/*	 AGCCTRL1           	0x0017*/
0x18,/*	 AGCCTRL2           	0x0018*/
0xff,/*	 AGC1VAL1           	0x0019*/
0x0f,/*	 AGC1VAL2           	0x001a*/
0x8d,/*	 AGC2VAL1           	0x001b*/
0x02,/*	 AGC2VAL2           	0x001c*/
0x00,/*	 AGC2PGA            	0x001d*/
0x00,/*	 OVF_RATE1          	0x001e*/
0x00,/*	 OVF_RATE2          	0x001f*/
0xfe,/*	 GAIN_SRC1          	0x0020*/
0xd8,/*	 GAIN_SRC2          	0x0021*/
0x55,/*	 INC_DEROT1         	0x0022*/
0x4c,/*	 INC_DEROT2         	0x0023*/
0x2c,/*	 PPM_CPAMP_DIR      	0x0024*/
0x00,/*	 PPM_CPAMP_INV      	0x0025*/
0x00,/*	 FREESTFE_1         	0x0026*/
0x1c,/*	 FREESTFE_2         	0x0027*/
0x00,/*	 DCOFFSET           	0x0028*/
0xb1,/*	 EN_PROCESS         	0x0029*/
0xff,/*	 SDI_SMOOTHER       	0x002a*/
0x00,/*	 FE_LOOP_OPEN       	0x002b*/
0x00,/*	 FREQOFF1           	0x002c*/
0x00,/*	 FREQOFF2           	0x002d*/
0x00,/*	 FREQOFF3           	0x002e*/
0x00,/*	 TIMOFF1            	0x002f*/
0x00,/*	 TIMOFF2            	0x0030*/
0x01,/*	 EPQ                	0x0031*/
0x01,/*	 EPQAUTO            	0x0032*/
0x01,/*	 CHP_TAPS           	0x0033*/
0x02,/*	 CHP_DYN_COEFF      	0x0034*/
0x23,/*	 PPM_STATE_MAC      	0x0035*/
0xff,/*	 INR_THRESHOLD      	0x0036*/
0x89,/*	 EPQ_TPS_ID_CELL    	0x0037*/
0x00,/*	 EPQ_CFG            	0x0038*/
0xfe,/*	 EPQ_STATUS         	0x0039*/
0x00,/*	 FECM               	0x0040*/
0x1e,/*	 VTH0               	0x0041*/
0x1e,/*	 VTH1               	0x0042*/
0x0f,/*	 VTH2               	0x0043*/
0x09,/*	 VTH3               	0x0044*/
0x00,/*	 VTH4               	0x0045*/
0x05,/*	 VTH5               	0x0046*/
0x00,/*	 FREEVIT            	0x0047*/
0x92,/*	 VITPROG            	0x0049*/
/*0x04*/0xAF,/*	 PR                 	0x004a*/
/*0xb0*/0x30,/*	 VSEARCH            	0x004b*/
0xbc,/*	 RS                 	0x004c*/
0x05,/*	 RSOUT              	0x004d*/
0x12,/*	 ERRCTRL1           	0x004e*/
0x00,/*	 ERRCNTM1           	0x004f*/
0x00,/*	 ERRCNTL1           	0x0050*/
0xb3,/*	 ERRCTRL2           	0x0051*/
0x00,/*	 ERRCNTM2           	0x0052*/
0x00,/*	 ERRCNTL2           	0x0053*/
0x00,/*	 FREEDRS            	0x0054*/
0x00,/*	 VERROR             	0x0055*/
0x12,/*	 ERRCTRL3           	0x0056*/
0x00,/*	 ERRCNTM3           	0x0057*/
0x00,/*	 ERRCNTL3           	0x0058*/
0x00,/*	 DILSTK1            	0x0059*/
0x03,/*	 DILSTK0            	0x005a*/
0x00,/*	 DILBWSTK1          	0x005b*/
0x03,/*	 DILBWST0           	0x005c*/
0x80,/*	 LNBRX              	0x005d*/
0xb0,/*	 RSTC               	0x005e*/
0x07,/*	 VIT_BIST           	0x005f*/
0x8d,/*	 IIR_CELL_NB        	0x0060*/
0x00,/*	 IIR_CX_COEFF1_MSB  	0x0061*/
0x00,/*	 IIR_CX_COEFF1_LSB  	0x0062*/
0x0e,/*	 IIR_CX_COEFF2_MSB  	0x0063*/
0xcc,/*	 IIR_CX_COEFF2_LSB  	0x0064*/
0x0e,/*	 IIR_CX_COEFF3_MSB  	0x0065*/
0xcc,/*	 IIR_CX_COEFF3_LSB  	0x0066*/
0x00,/*	 IIR_CX_COEFF4_MSB  	0x0067*/
0x00,/*	 IIR_CX_COEFF4_LSB  	0x0068*/
0x36,/*	 IIR_CX_COEFF5_MSB  	0x0069*/
0x47,/*	 IIR_CX_COEFF5_LSB  	0x006a*/
0x00,/*	 FEPATH_CFG         	0x006b*/
0x25,/*	 PMC1_FUNC          	0x006c*/
0x00,/*	 PMC1_FORCE         	0x006d*/
0x00,/*	 PMC2_FUNC          	0x006e*/
0xf8,/*	 DIG_AGC_R          	0x0070*/
0x0d,/*	 COMAGC_TARMSB      	0x0071*/
0xc1,/*	 COM_AGC_TAR_ENMODE 	0x0072*/
0x3b,/*	 COM_AGC_CFG        	0x0073*/
0x39,/*	 COM_AGC_GAIN1      	0x0074*/
0x10,/*	 AUT_AGC_TARGET_MSB 	0x0075*/
0x01,/*	 LOCK_DETECT_MSB    	0x0076*/
0x00,/*	 AGCTAR_LOCK_LSBS   	0x0077*/
0xf4,/*	 AUT_GAIN_EN        	0x0078*/
0xf0,/*	 AUT_CFG            	0x0079*/
0x23,/*	 LOCKN              	0x007a*/
0x00,/*	 INT_X_3            	0x007b*/
0x05,/*	 INT_X_2            	0x007c*/
0x60,/*	 INT_X_1            	0x007d*/
0xc0,/*	 INT_X_0            	0x007e*/
0x01,/*	 MIN_ERR_X_MSB      	0x007f*/
0xec,/*	 STATUS_ERR_DA      	0x006f*/
0x20,/*	 COR_CTL            	0x0080*/
0xf6,/*	 COR_STAT           	0x0081*/
0x00,/*	 COR_INTEN          	0x0082*/
0x39,/*	 COR_INTSTAT        	0x0083*/
0x0a,/*	 COR_MODEGUARD      	0x0084*/
0x18,/*	 AGC_CTL            	0x0085*/
0x00,/*	 AGC_MANUAL1        	0x0086*/
0x00,/*	 AGC_MANUAL2        	0x0087*/
0x1e,/*	 AGC_TARGET         	0x0088*/
0x97,/*	 AGC_GAIN1          	0x0089*/
0x1a,/*	 AGC_GAIN2          	0x008a*/
0x00,/*	 RESERVED_1         	0x008b*/
0x00,/*	 RESERVED_2         	0x008c*/
0x00,/*	 RESERVED_3         	0x008d*/
0x40,/*	 CAS_CTL            	0x008e*/
0xb3,/*	 CAS_FREQ           	0x008f*/
0x0f,/*	 CAS_DAGCGAIN       	0x0090*/
0x04,/*	 SYR_CTL            	0x0091*/
0x13,/*	 SYR_STAT           	0x0092*/
0x00,/*	 SYR_NCO1           	0x0093*/
0x00,/*	 SYR_NCO2           	0x0094*/
0x00,/*	 SYR_OFFSET1        	0x0095*/
0x00,/*	 SYR_OFFSET2        	0x0096*/
0x00,/*	 FFT_CTL            	0x0097*/
0x00,/*	 SCR_CTL            	0x0098*/
0x38,/*	 PPM_CTL1           	0x0099*/
0x14,/*	 TRL_CTL            	0x009a*/
0xac,/*	 TRL_NOMRATE1       	0x009b*/
0x56,/*	 TRL_NOMRATE2       	0x009c*/
0x87,/*	 TRL_TIME1          	0x009d*/
0xfd,/*	 TRL_TIME2          	0x009e*/
0x4f,/*	 CRL_CTL            	0x009f*/
0xa0,/*	 CRL_FREQ1          	0x00a0*/
0xb6,/*	 CRL_FREQ2          	0x00a1*/
0xff,/*	 CRL_FREQ3          	0x00a2*/
0x11,/*	 CHC_CTL1           	0x00a3*/
0xee,/*	 CHC_SNR            	0x00a4*/
0x00,/*	 BDI_CTL            	0x00a5*/
0x00,/*	 DMP_CTL            	0x00a6*/
0x33,/*	 TPS_RCVD1          	0x00a7*/
0x02,/*	 TPS_RCVD2          	0x00a8*/
0x02,/*	 TPS_RCVD3          	0x00a9*/
0x30,/*	 TPS_RCVD4          	0x00aa*/
0x00,/*	 TPS_ID_CELL1       	0x00ab*/
0x00,/*	 TPS_ID_CELL2       	0x00ac*/
0x00,/*	 TPS_RCVD5_SET1     	0x00ad*/
0x02,/*	 TPS_SET2           	0x00ae*/
0x02,/*	 TPS_SET3           	0x00af*/
0x00,/*	 TPS_CTL            	0x00b0*/
0x2b,/*	 CTL_FFTOSNUM       	0x00b1*/
0x09,/*	 TESTSELECT         	0x00b2*/
0x0a,/*	 MSC_REV            	0x00b3*/
0x00,/*	 PIR_CTL            	0x00b4*/
0xa9,/*	 SNR_CARRIER1       	0x00b5*/
0x86,/*	 SNR_CARRIER2       	0x00b6*/
0x31,/*	 PPM_CPAMP          	0x00b7*/
0x00,/*	 TSM_AP0            	0x00b8*/
0x00,/*	 TSM_AP1            	0x00b9*/
0x00,/*	 TSM_AP2            	0x00ba*/
0x00,/*	 TSM_AP3            	0x00bb*/
0x00,/*	 TSM_AP4            	0x00bc*/
0x00,/*	 TSM_AP5            	0x00bd*/
0x00,/*	 TSM_AP6            	0x00be*/
0x00,/*	 TSM_AP7            	0x00bf*/
0x00,/*	 TSTRES             	0x00c0*/
0x00,/*	 ANACTRL            	0x00c1*/
0x00,/*	 TSTBUS             	0x00c2*/
0x00,/*	 TSTRATE            	0x00c6*/
0x00,/*	 CONSTMODE          	0x00cb*/
0x00,/*	 CONSTCARR1         	0x00cc*/
0x00,/*	 CONSTCARR2         	0x00cd*/
0x00,/*	 ICONSTEL           	0x00ce*/
0x00,/*	 QCONSTEL           	0x00cf*/
0x02,/*	 TSTBISTRES0        	0x00d0*/
0x00,/*	 TSTBISTRES1        	0x00d1*/
0x00,/*	 TSTBISTRES2        	0x00d2*/
0x00,/*	 TSTBISTRES3        	0x00d3*/
0xe6,/*	 RF_AGC1            	0x00d4*/
0x81,/*	 RF_AGC2            	0x00d5*/
0x01,/*	 ANADIGCTRL         	0x00d7*/
0x00,/*	 PLLMDIV            	0x00d8*/
0x10,/*	 PLLSETUP           	0x00da*/
0x08,/*	 DUAL_AD12          	0x00db*/
0x00,/*	 TSTBIST            	0x00dc*/
0x00,/*	 PAD_COMP_CTRL      	0x00dd*/
0x00,/*	 PAD_COMP_WR        	0x00de*/
0xe0,/*	 PAD_COMP_RD        	0x00df*/
0x40 /*	 GHOSTREG           	0x0000*/
#endif
};






/* Private types/constants ------------------------------------------------ */



/* Device capabilities */
#define MAX_AGC                         255
#define MAX_SIGNAL_QUALITY              100
#define MAX_BER                         200000

/* private variables ------------------------------------------------------- */


static semaphore_t *Lock_InitTermOpenClose; /* guard calls to the functions */


static BOOL        Installed = FALSE;


#define WAIT_N_MS_362(X)     STOS_TaskDelayUs(X * 1000)


#if defined (ST_OS21) || defined (ST_OSLINUX)
#define STTUNER_TaskDelay(x) STOS_TaskDelay((signed int)x)
#else
#define STTUNER_TaskDelay(x) STOS_TaskDelay((unsigned int)x)
#endif


extern IOARCH_HandleData_t IOARCH_Handle[STTUNER_IOARCH_MAX_HANDLES];



 
/* instance chain, the default boot value is invalid, to catch errors */
static D0362_InstanceData_t *InstanceChainTop = (D0362_InstanceData_t *)0x7fffffff;

/* functions --------------------------------------------------------------- */


/* API */
ST_ErrorCode_t demod_d0362_Init(ST_DeviceName_t *DeviceName, DEMOD_InitParams_t *InitParams);
ST_ErrorCode_t demod_d0362_Term(ST_DeviceName_t *DeviceName, DEMOD_TermParams_t *TermParams);

ST_ErrorCode_t demod_d0362_Open (ST_DeviceName_t *DeviceName, DEMOD_OpenParams_t  *OpenParams, DEMOD_Capability_t *Capability, DEMOD_Handle_t *Handle);
ST_ErrorCode_t demod_d0362_Close(DEMOD_Handle_t  Handle, DEMOD_CloseParams_t *CloseParams);

ST_ErrorCode_t demod_d0362_GetTunerInfo    (DEMOD_Handle_t Handle, STTUNER_TunerInfo_t *TunerInfo_p);
ST_ErrorCode_t demod_d0362_GetSignalQuality(DEMOD_Handle_t Handle, U32  *SignalQuality_p, U32 *Ber);
ST_ErrorCode_t demod_d0362_GetModulation   (DEMOD_Handle_t Handle, STTUNER_Modulation_t *Modulation);
ST_ErrorCode_t demod_d0362_GetAGC          (DEMOD_Handle_t Handle, S16                  *Agc);
ST_ErrorCode_t demod_d0362_GetFECRates     (DEMOD_Handle_t Handle, STTUNER_FECRate_t    *FECRates);
ST_ErrorCode_t demod_d0362_GetMode         (DEMOD_Handle_t Handle, STTUNER_Mode_t       *Mode);
ST_ErrorCode_t demod_d0362_GetGuard        (DEMOD_Handle_t Handle, STTUNER_Guard_t      *Guard);
ST_ErrorCode_t demod_d0362_GetRFLevel(DEMOD_Handle_t Handle, S32  *Rflevel);
ST_ErrorCode_t demod_d0362_IsLocked        (DEMOD_Handle_t Handle, BOOL                 *IsLocked);
ST_ErrorCode_t demod_d0362_Tracking        (DEMOD_Handle_t Handle, BOOL ForceTracking,   U32 *NewFrequency, BOOL *SignalFound);

ST_ErrorCode_t demod_d0362_ScanFrequency   (DEMOD_Handle_t  Handle,
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
ST_ErrorCode_t demod_d0362_ioctl           (DEMOD_Handle_t Handle, U32 Function, void *InParams, void *OutParams,
                                             STTUNER_Da_Status_t *Status);



/* For TPS CellID extraction */
ST_ErrorCode_t demod_d0362_GetTPSCellId(DEMOD_Handle_t Handle, U16  *TPSCellId);

/* For STANDBY API mode */
ST_ErrorCode_t demod_d0362_StandByMode(DEMOD_Handle_t Handle, STTUNER_StandByMode_t PowerMode);

/* local functions --------------------------------------------------------- */

D0362_InstanceData_t *d0362_GetInstFromHandle(DEMOD_Handle_t Handle);
ST_ErrorCode_t demod_d0362_PLLDIV_TRL_INC_ioctl_set( IOARCH_Handle_t IOHandle,void *InParams);
ST_ErrorCode_t demod_d0362_PLLDIV_TRL_INC_ioctl_get(IOARCH_Handle_t IOHandle,void *OutParams);
ST_ErrorCode_t demod_d0362_IF_IQMODE_ioctl_set(STTUNER_IF_IQ_Mode *IFIQMode,void *InParams);
ST_ErrorCode_t demod_d0362_IF_IQMODE_ioctl_get(STTUNER_IF_IQ_Mode IFIQMode,void *OutParams);
ST_ErrorCode_t demod_d0362_repeateraccess(DEMOD_Handle_t Handle,BOOL REPEATER_STATUS);


/* ----------------------------------------------------------------------------
Name: STTUNER_DRV_DEMOD_STV0362_Install()

Description:
    install a terrestrial device driver (362) into the demod database.

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_DRV_DEMOD_STV0362_Install(STTUNER_demod_dbase_t *Demod)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
   const char *identity = "STTUNER d0362.c STTUNER_DRV_DEMOD_STV0362_Install()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if(Installed == TRUE)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
        STTBX_Print(("%s fail driver already installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
    STTBX_Print(("%s installing ter:demod:STV0362...", identity));
#endif

    /* mark ID in database */
    Demod->ID = STTUNER_DEMOD_STV0362;

    /* map API */
    Demod->demod_Init = demod_d0362_Init;
    Demod->demod_Term = demod_d0362_Term;

    Demod->demod_Open  = demod_d0362_Open;
    Demod->demod_Close = demod_d0362_Close;

    Demod->demod_IsAnalogCarrier  = NULL;
    Demod->demod_GetTunerInfo     = demod_d0362_GetTunerInfo;
    Demod->demod_GetSignalQuality = demod_d0362_GetSignalQuality;
    Demod->demod_GetModulation    = demod_d0362_GetModulation;
    Demod->demod_GetAGC           = demod_d0362_GetAGC;
    Demod->demod_GetMode          = demod_d0362_GetMode;
    Demod->demod_GetGuard         = demod_d0362_GetGuard;
    Demod->demod_GetFECRates      = demod_d0362_GetFECRates;
    Demod->demod_GetRFLevel       =demod_d0362_GetRFLevel;
    Demod->demod_IsLocked         = demod_d0362_IsLocked ;
    Demod->demod_SetFECRates      = NULL;
    Demod->demod_Tracking         = demod_d0362_Tracking;
    Demod->demod_ScanFrequency    = demod_d0362_ScanFrequency;
    Demod->demod_repeateraccess = demod_d0362_repeateraccess;
    Demod->demod_ioaccess = NULL;
    Demod->demod_ioctl    = demod_d0362_ioctl;
    /*Demod->demod_GetRFLevel	  = NULL;*/
    Demod->demod_GetTPSCellId = demod_d0362_GetTPSCellId;
    Demod->demod_StandByMode = demod_d0362_StandByMode;

    InstanceChainTop = NULL;

    Lock_InitTermOpenClose = STOS_SemaphoreCreateFifo(NULL,1);


    Installed = TRUE;

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
    STTBX_Print(("ok\n"));
#endif

    return(Error);
}



/* ----------------------------------------------------------------------------
Name: STTUNER_DRV_DEMOD_STV0362_UnInstall()

Description:
    uninstall a terrestrial device driver(COFDM) into the demod database.

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_DRV_DEMOD_STV0362_UnInstall(STTUNER_demod_dbase_t *Demod)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
   const char *identity = "STTUNER d0362.c STTUNER_DRV_DEMOD_STV0362_UnInstall()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

    if(Demod->ID != STTUNER_DEMOD_STV0362)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
        STTBX_Print(("%s fail incorrect driver type\n", identity));
#endif
        return(STTUNER_ERROR_ID);
    }

    /* has all memory been freed, by Term() */
    if(InstanceChainTop != NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
        STTBX_Print(("%s fail at least one instance not terminated\n", identity));
#endif
        return(ST_ERROR_OPEN_HANDLE);
    }


#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
    STTBX_Print(("%s uninstalling ter:demod:STV0362...", identity));
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
    Demod->demod_GetRFLevel       =NULL;
    Demod->demod_IsLocked         = NULL;
    Demod->demod_SetFECRates      = NULL;
    Demod->demod_Tracking         = NULL;
    Demod->demod_ScanFrequency    = NULL;
    Demod->demod_repeateraccess = NULL;
    Demod->demod_ioaccess = NULL;
    Demod->demod_ioctl    = NULL;
    Demod->demod_GetTPSCellId = NULL;
    Demod->demod_StandByMode = NULL ;

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
        STTBX_Print(("<"));
#endif


  STOS_SemaphoreDelete(NULL,Lock_InitTermOpenClose);


#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
        STTBX_Print((">"));
#endif

    InstanceChainTop = (D0362_InstanceData_t *)0x7ffffffe;
    Installed        = FALSE;

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
        STTBX_Print(("ok\n"));
#endif

    return(Error);
}



/* ------------------------------------------------------------------------- */
/* /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ API /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ */
/* ------------------------------------------------------------------------- */



/* ----------------------------------------------------------------------------
Name: demod_d0362_Init()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0362_Init(ST_DeviceName_t *DeviceName, DEMOD_InitParams_t *InitParams)
{



#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
    const char *identity = "STTUNER d0362.c demod_d0362_Init()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0362_InstanceData_t *InstanceNew, *Instance;

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
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
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
        STTBX_Print(("%s fail MemoryPartition not valid\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    Error = STTUNER_Util_CheckPtrNull(DeviceName);
    if( Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
        STTBX_Print(("%s fail DeviceName not valid\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

  InstanceNew = STOS_MemoryAllocateClear(InitParams->MemoryPartition, 1, sizeof( D0362_InstanceData_t ));
    if (InstanceNew == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
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
  
    InstanceNew->StandBy_Flag = 0 ;
    
    
    IOARCH_Handle[InstanceNew->IOHandle].DeviceMap.Timeout   = IOREG_DEFAULT_TIMEOUT;
    IOARCH_Handle[InstanceNew->IOHandle].DeviceMap.Registers = STV0362_NBREGS;
    IOARCH_Handle[InstanceNew->IOHandle].DeviceMap.Fields    = STV0362_NBFIELDS;
    IOARCH_Handle[InstanceNew->IOHandle].DeviceMap.Mode      = IOREG_MODE_SUBADR_8; /* i/o addressing mode to use */
    IOARCH_Handle[InstanceNew->IOHandle].DeviceMap.MemoryPartition = InitParams->MemoryPartition;
    IOARCH_Handle[InstanceNew->IOHandle].DeviceMap.DefVal= (U32 *)&STV0362_DefVal[0];    
    IOARCH_Handle[InstanceNew->IOHandle].DeviceMap.ByteStream = STOS_MemoryAllocateClear(IOARCH_Handle[InstanceNew->IOHandle].DeviceMap.MemoryPartition,MAX_ADDRESS,sizeof(U8));





            
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
    STTBX_Print(("%s allocated & initalized block named '%s' at 0x%08x (%d bytes)\n", identity, InstanceNew->DeviceName, (U32)InstanceNew, sizeof( D0362_InstanceData_t ) ));
#endif
    SEM_UNLOCK(Lock_InitTermOpenClose);
 


    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0362_Term()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0362_Term(ST_DeviceName_t *DeviceName, DEMOD_TermParams_t *TermParams)
{


#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
   const char *identity = "STTUNER d0362.c demod_d0362_Term()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0362_InstanceData_t *Instance, *InstancePrev, *InstanceNext;

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
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
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
        STTBX_Print(("%s fail TermParams not valid\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    /* ---------- check that at least one init has taken place ---------- */
    if(InstanceChainTop == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
        STTBX_Print(("%s fail nothing initalized\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(STTUNER_ERROR_INITSTATE);
    }


    /* reap next matching DeviceName */
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
    STTBX_Print(("%s looking for first free handle[\n", identity));
#endif

    Instance = InstanceChainTop;
    while(1)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
        STTBX_Print(("(%s)", Instance->DeviceName));
#endif
        if ( strcmp( (char *)Instance->DeviceName, (char *)DeviceName) == 0)
        {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
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


             
            STOS_MemoryDeallocate(Instance->MemoryPartition, Instance);


#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
            STTBX_Print(("%s freed block at %0x%08x\n", identity, (U32)Instance ));
#endif

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
            STTBX_Print(("%s terminated ok\n", identity));
#endif


STOS_MemoryDeallocate(IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition,IOARCH_Handle[Instance->IOHandle].DeviceMap.ByteStream);


#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
            STTBX_Print(("%s freed block at %0x%08x\n", identity, (U32)Instance ));
#endif

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
            STTBX_Print(("%s terminated ok\n", identity));
#endif

            SEM_UNLOCK(Lock_InitTermOpenClose);
            return(Error);
        }
        else if(Instance->InstanceChainNext == NULL)
        {       /* error we should have found a matching name before the end of the list */
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
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


#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
    STTBX_Print(("%s FAIL! this point should NEVER be reached\n", identity));
#endif
    SEM_UNLOCK(Lock_InitTermOpenClose);
    

    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0362_Open()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0362_Open(ST_DeviceName_t *DeviceName, DEMOD_OpenParams_t  *OpenParams, DEMOD_Capability_t *Capability, DEMOD_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
   const char *identity = "STTUNER d0362.c demod_d0362_Open()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0362_InstanceData_t     *Instance;
    U8 ChipID = 0;
    U8 index;

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

    /* now safe to lock semaphore */
    SEM_LOCK(Lock_InitTermOpenClose);

    /* ---------- check that at least one init has taken place ---------- */
    if(InstanceChainTop == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
        STTBX_Print(("%s fail nothing initalized\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(STTUNER_ERROR_INITSTATE);
    }

    /* find  handle from name */
    Instance = InstanceChainTop;
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
    STTBX_Print(("%s looking (%s)", identity, Instance->DeviceName));
#endif
    while(strcmp((char *)Instance->DeviceName, (char *)DeviceName) != 0)
    {
        /* error, should have found matching DeviceName before end of list */
        if(Instance->InstanceChainNext == NULL)
        {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
            STTBX_Print(("\n%s fail no block named '%s' before end of list\n", identity, DeviceName));
#endif
            SEM_UNLOCK(Lock_InitTermOpenClose);
            return(STTUNER_ERROR_INITSTATE);
        }
        Instance = Instance->InstanceChainNext;   /* next block */
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
        STTBX_Print(("(%s)", Instance->DeviceName));
#endif
    }
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
        STTBX_Print((" found ok\n"));
#endif

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
    STTBX_Print(("%s using block at 0x%08x\n", identity, (U32)Instance));
#endif



    /* check handle IS actually free */
    if(Instance->TopLevelHandle != STTUNER_MAX_HANDLES)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
        STTBX_Print(("%s fail driver instance already open\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_OPEN_HANDLE);
    }
    /* else now got pointer to free (and valid) data block */



   ChipID = ChipGetOneRegister( Instance->IOHandle, R0362_ID);
    
    
    
    
    

    if ( (ChipID & 0x40) !=  0x40)
    {

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
        STTBX_Print(("%s fail device not found (0x%02x)\n", identity, ChipID));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_UNKNOWN_DEVICE);
    }
    else
    {

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
        STTBX_Print(("%s device found, release/revision=%u\n", identity, ChipID & 0x40));
#endif
    }
    
 /* reset all chip registers */

    for (index=0;index < STV0362_NBREGS;index++)
     {
     ChipSetOneRegister(Instance->IOHandle,STV0362_Address[index],STV0362_DefVal[index]);
     }
              
   /* Set serial/parallel data mode */
    if (Instance->TSOutputMode == STTUNER_TS_MODE_SERIAL)
    {
        Error |= ChipSetField( Instance->IOHandle, F0362_OUTRS_SP, 1);
    }
    else
    {
        Error |= ChipSetField( Instance->IOHandle, F0362_OUTRS_SP, 0);
    }


    /*set data clock polarity inversion mode (rising/falling)*/
    switch(Instance->ClockPolarity)
    {
       case STTUNER_DATA_CLOCK_POLARITY_RISING:
    	 ChipSetField( Instance->IOHandle, F0362_CLK_POL, 1);
    	 break;
       case STTUNER_DATA_CLOCK_POLARITY_FALLING:
    	 ChipSetField( Instance->IOHandle, F0362_CLK_POL, 0);
    	 break;
       case STTUNER_DATA_CLOCK_POLARITY_DEFAULT:
    	 ChipSetField( Instance->IOHandle, F0362_CLK_POL, 0);
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

    Instance->UnlockCounter = 2; /* unlock counter made 0 initially*/
    Instance->IFIQMode = STTUNER_NORMAL_IF_TUNER;/*Normal IF mode by defaul*/

    *Handle = (U32)Instance;



#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
    STTBX_Print(("%s opened ok\n", identity));
#endif

    SEM_UNLOCK(Lock_InitTermOpenClose);
    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0362_Close()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0362_Close(DEMOD_Handle_t Handle, DEMOD_CloseParams_t *CloseParams)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
   const char *identity = "STTUNER d0362.c demod_d0362_Close()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0362_InstanceData_t     *Instance;

    /* private driver instance data */
    Instance = d0362_GetInstFromHandle(Handle);

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

    SEM_LOCK(Lock_InitTermOpenClose);

    /* ---------- check that at least one init has taken place ---------- */
    if(InstanceChainTop == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
        STTBX_Print(("%s fail nothing initalized\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(STTUNER_ERROR_INITSTATE);
    }

    if(Instance->TopLevelHandle == STTUNER_MAX_HANDLES)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
        STTBX_Print(("%s fail driver instance not open\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_OPEN_HANDLE);
    }


    /* indicate instance is closed */
    Instance->TopLevelHandle = STTUNER_MAX_HANDLES;

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
    STTBX_Print(("%s closed\n", identity));
#endif

    SEM_UNLOCK(Lock_InitTermOpenClose);
    return(Error);
}


/* ----------------------------------------------------------------------------
Name: demod_d0362_GetTunerInfo()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0362_GetTunerInfo(DEMOD_Handle_t Handle, STTUNER_TunerInfo_t *TunerInfo_p)
{

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
   const char *identity = "STTUNER d0362.c demod_d0362_GetTunerInfo()";
#endif
    ST_ErrorCode_t       Error = ST_NO_ERROR;
    ST_ErrorCode_t       TunerError ;
    D0362_InstanceData_t *Instance;
    STTUNER_InstanceDbase_t     *Inst;
    U8                   Data;
    U32                  CurFrequency, CurSignalQuality, CurBitErrorRate;
    STTUNER_Modulation_t CurModulation;
    STTUNER_Mode_t       CurMode;
    STTUNER_FECRate_t    CurFECRates;
    STTUNER_Guard_t      CurGuard;
    STTUNER_Spectrum_t   CurSpectrum;
    STTUNER_Hierarchy_Alpha_t CurHierMode;/*Added for HM*/
    S32                  CurEchoPos;
    S32 offset=0;
    #ifdef STTUNER_DRV_TER_TUN_MT2266
      STTUNER_tuner_instance_t    *TunerInstance;
     #endif
   /* S32 offset_type=0;*/
    U8 crl_freq1[3];
    /* private driver instance data */
    Instance = d0362_GetInstFromHandle(Handle);
 #ifdef STTUNER_DRV_TER_TUN_MT2266
 TunerInstance = &Inst[Instance->TopLevelHandle].Terr.Tuner;
 #endif

    /* top level public instance data */
    Inst = STTUNER_GetDrvInst();
    /* Read noise estimations for C/N and BER */
    FE_362_GetNoiseEstimator( Instance->IOHandle, &CurSignalQuality, &CurBitErrorRate);

  /******The Error field of TunerInfo in Instaneous database is updated ******/

    TunerError= ST_NO_ERROR;

    /*************************************************************************/
    /* Get the modulation type */
#ifdef STTUNER_DRV_TER_TUN_MT2266
/*(TunerInstance->Driver->pdcontrol_loop)(TunerInstance->DrvHandle,&(Instance->DeviceMap), Instance->IOHandle);*/

MT2266_pdcontrol_loop(&TunerInstance->DrvHandle, Instance->IOHandle);
#endif


    Data=ChipGetField( Instance->IOHandle, F0362_TPS_CONST);


    switch (Data)
    {
        case 0:  CurModulation = STTUNER_MOD_QPSK;  break;
        case 1:  CurModulation = STTUNER_MOD_16QAM; break;
        case 2:  CurModulation = STTUNER_MOD_64QAM; break;
        default:
        /*CurModulation = STTUNER_MOD_ALL;*/
        CurModulation = Data ;
        STTUNER_TaskDelay(5);
        Data=ChipGetField( Instance->IOHandle, F0362_TPS_CONST);
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


    Data=ChipGetField( Instance->IOHandle, F0362_TPS_MODE);

    switch (Data)
    {
        case 0:  CurMode = STTUNER_MODE_2K; break;
        case 1:  CurMode = STTUNER_MODE_8K; break;
        default:
        CurMode =Data;
        STTUNER_TaskDelay(5);
        Data=ChipGetField( Instance->IOHandle, F0362_TPS_MODE);
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


    Data=ChipGetField( Instance->IOHandle, F0362_TPS_HIERMODE);

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
    if((Instance->Result).hier==STTUNER_HIER_LOW_PRIO)
    {
       Data=ChipGetField( Instance->IOHandle,  F0362_TPS_LPCODE);

    }
    else
    {
    Data = ChipGetField(Instance->IOHandle, F0362_TPS_HPCODE);
    }

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


    Data = ChipGetField( Instance->IOHandle, F0362_TPS_GUARD);

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

    /* Get the spectrum.Register is not read as spectrum inversion code added*/
     CurSpectrum = Instance->ResultSpectrum;

    /* Get the correct frequency */
    CurFrequency = TunerInfo_p->Frequency;
    /********Frequency offset calculation done here*******************/

        ChipSetField( Instance->IOHandle,F0362_FREEZE,1);
	ChipGetRegisters(Instance->IOHandle,R0362_CRL_FREQ1,3,crl_freq1);
	ChipSetField( Instance->IOHandle,F0362_FREEZE,0);



offset=Instance->Result.offset;

   
     TunerInfo_p->ScanInfo.FreqOff =Instance->Result.offset_type;
     TunerInfo_p->ScanInfo.ResidualOffset = offset;
    /************************************************/

    /* Get the echo position */
    CurEchoPos=ChipGetField( Instance->IOHandle, F0362_LONG_ECHO);



    TunerInfo_p->FrequencyFound      = CurFrequency;
    TunerInfo_p->SignalQuality       = CurSignalQuality;
    TunerInfo_p->BitErrorRate        = CurBitErrorRate;
    TunerInfo_p->ScanInfo.Modulation = CurModulation;
    TunerInfo_p->ScanInfo.Mode       = CurMode;
    TunerInfo_p->ScanInfo.FECRates   = CurFECRates;
    TunerInfo_p->ScanInfo.Guard      = CurGuard;
    TunerInfo_p->ScanInfo.Spectrum   = CurSpectrum;
    TunerInfo_p->ScanInfo.EchoPos    = CurEchoPos;
    TunerInfo_p->Hierarchy_Alpha     = CurHierMode;/*added for Hierarchical modulation*/
    Inst[Instance->TopLevelHandle].TunerInfoError = TunerError;

     TunerInfo_p->ScanInfo.Hierarchy =    (Instance->Result).hier;/*added for Hierarchical modulation*/


 #ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
    STTBX_Print(("F=%d SQ=%u BER=%u Modul=%u Mode=%u FR=%u G=%u Sp=%u\n",CurFrequency,CurSignalQuality,CurBitErrorRate,CurModulation,CurMode,CurFECRates,CurGuard,CurSpectrum));
 #endif

    /*Error handling from device map*/
    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0362_GetSignalQuality()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0362_GetSignalQuality(DEMOD_Handle_t Handle, U32  *SignalQuality_p, U32 *Ber)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
   const char *identity = "STTUNER d0362.c demod_d0362_GetSignalQuality()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0362_InstanceData_t *Instance;

    /* private driver instance data */
    Instance = d0362_GetInstFromHandle(Handle);

    /* Read noise estimations for C/N and BER */
    FE_362_GetNoiseEstimator(Instance->IOHandle, SignalQuality_p, Ber);
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
    STTBX_Print(("%s SignalQuality=%u Ber=%u\n", identity, *SignalQuality_p, *Ber));
#endif
    Error = IOARCH_Handle[Instance->IOHandle].DeviceMap.Error;
    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0362_GetModulation()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0362_GetModulation(DEMOD_Handle_t Handle, STTUNER_Modulation_t *Modulation)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
   const char *identity = "STTUNER d0362.c demod_d0362_GetModulation()";
#endif
    STTUNER_Modulation_t CurModulation;
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0362_InstanceData_t *Instance;
    U32 Data;
    /* private driver instance data */
    Instance = d0362_GetInstFromHandle(Handle);

    /* Get the modulation type */
    /*Use IOREG Call instead of chip call */
    ChipGetField( Instance->IOHandle, F0362_TPS_CONST);
    Data = ChipGetField(Instance->IOHandle, F0362_TPS_CONST);

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

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
    STTBX_Print(("%s Modulation=%u\n", identity, *Modulation));
#endif
   /*add Error handling from device map*/
    return(Error);
}

/* ----------------------------------------------------------------------------
Name: demod_d0362_GetAGC()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0362_GetAGC(DEMOD_Handle_t Handle, S16  *Agc)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
   const char *identity = "STTUNER d0362.c demod_d0362_GetAGC()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    S16 Data1=0;
    S16 Data2=0;
    D0362_InstanceData_t *Instance;

    /** fix done here for the bug GNBvd20972 **
     ** where body of the function demod_d0362_GetAGC written **
     ** and the function now returns the core gain **/

    /* private driver instance data */
    Instance = d0362_GetInstFromHandle(Handle);
    /* Get the mode type */
    Data1 =  ChipGetField( Instance->IOHandle, F0362_AGC_GAIN_LO);

    Data2 = ChipGetField( Instance->IOHandle, F0362_AGC_GAIN_HI);
    Data2 <<=8;
    Data2 &= 0x0F00;
    Data2 |= Data1 ;

    *Agc=Data2 ;

   /*add error handling from device map*/
    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0362_GetFECRates()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0362_GetFECRates(DEMOD_Handle_t Handle, STTUNER_FECRate_t *FECRates)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
   const char *identity = "STTUNER d0362.c demod_d0362_GetFECRates()";
#endif
    U8 Data;
    STTUNER_FECRate_t CurFecRate;
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0362_InstanceData_t *Instance;

    /* private driver instance data */
    Instance = d0362_GetInstFromHandle(Handle);

    CurFecRate = 0;
    ChipGetField( Instance->IOHandle, F0362_TPS_HPCODE);
    Data = ChipGetField(Instance->IOHandle, F0362_TPS_HPCODE);


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

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
    STTBX_Print(("%s FECRate=%u\n", identity, CurFecRate));
#endif


  /*Add error handling from device map*/
    return(Error);
}

/* ----------------------------------------------------------------------------
Name: demod_d0362_GetMode()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0362_GetMode(DEMOD_Handle_t Handle, STTUNER_Mode_t *Mode)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
   const char *identity = "STTUNER d0362.c demod_d0362_GetMode()";
#endif
    U8 Data;
    STTUNER_Mode_t CurMode;
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0362_InstanceData_t *Instance;

    /* private driver instance data */
    Instance = d0362_GetInstFromHandle(Handle);

    /* Get the mode type */
    Data=ChipGetField(Instance->IOHandle,F0362_TPS_MODE /*R_TPS_RCVD1*/);
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

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
    STTBX_Print(("%s Mode=%u\n", identity, *Mode));
#endif
     /*Add error handling from device map*/
    return(Error);
}

/* ----------------------------------------------------------------------------
Name: demod_d0362_GetGuard()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0362_GetGuard(DEMOD_Handle_t Handle, STTUNER_Guard_t *Guard)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
   const char *identity = "STTUNER d0362.c demod_d0362_GetGuard()";
#endif
    U8 Data;
    STTUNER_Guard_t CurGuard;
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0362_InstanceData_t *Instance;

    /* private driver instance data */
    Instance = d0362_GetInstFromHandle(Handle);

    CurGuard = 0;
    Data = ChipGetField(Instance->IOHandle, F0362_TPS_GUARD);

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

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
    STTBX_Print(("%s Guard=%u\n", identity, CurGuard));
#endif

    /*Resolve Bug GNBvd17801 - For proper I2C error handling inside the driver*/
    /*Add error handling from device map*/
    return(Error);
}
/* ----------------------------------------------------------------------------
Name: demod_d0362_IsLocked()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0362_IsLocked(DEMOD_Handle_t Handle, BOOL *IsLocked)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
   const char *identity = "STTUNER d0362.c demod_d0362_IsLocked()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0362_InstanceData_t *Instance;

    /* private driver instance data */
    Instance = d0362_GetInstFromHandle(Handle);

    *IsLocked = ChipGetField(Instance->IOHandle, F0362_LK) ? TRUE : FALSE;
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
    STTBX_Print(("%s IsLocked=%u\n", identity, *IsLocked));
#endif

    /*Resolve Bug GNBvd17801 - For proper I2C error handling inside the driver*/

    return(Error);
}




/* ----------------------------------------------------------------------------
Name: demod_d0362_ScanFrequency()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0362_ScanFrequency   (DEMOD_Handle_t Handle,
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
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
   const char *identity = "STTUNER d0362.c demod_d0362_ScanFrequency()";
#endif

    ST_ErrorCode_t              Error = ST_NO_ERROR;
    FE_362_Error_t error = FE_362_NO_ERROR;
    STTUNER_tuner_instance_t    *TunerInstance;
    STTUNER_InstanceDbase_t     *Inst;
    D0362_InstanceData_t        *Instance;

    FE_362_SearchParams_t       Search;
    FE_362_SearchResult_t       Result;
    /* private driver instance data */
    Instance = d0362_GetInstFromHandle(Handle);

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

    Search.IF_IQ_Mode =Instance->IFIQMode;/*This should come from application*/
    error = FE_362_LookFor(Handle,Instance->IOHandle, &Search, &Result,Instance->TopLevelHandle);

   if(error == FE_362_BAD_PARAMETER)
    {
       Error= ST_ERROR_BAD_PARAMETER;
 	 #ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
           STTBX_Print(("%s fail, scan not done: bad parameter(s) to FE_362_Search() == FE_BAD_PARAMETER\n", identity ));
	 #endif
       return Error;
    }

    else if (error == FE_362_SEARCH_FAILED)
    {
    	Error= ST_NO_ERROR;
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
           STTBX_Print(("%s  FE_362_Search() == FE_362_SEARCH_FAILED\n", identity ));
#endif
	return Error;

    }


     *ScanSuccess = Result.Locked;
     if (*ScanSuccess == TRUE)
     {
     	*NewFrequency = Result.Frequency;


#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
        STTBX_Print(("%s NewFrequency=%u\t", identity, *NewFrequency));
    STTBX_Print(("%s ScanSuccess=%u\n", identity, *ScanSuccess));
#endif

     	/*Update Hierarchical parameters into strucuture*/
     	  (Instance->Result).hier = Result.hier;
     	  (Instance->Result).Hprate=Result.Hprate;
     	  (Instance->Result).Lprate=Result.Lprate;
     	   (Instance->Result).offset = Result.offset;
	      (Instance->Result).offset_type = Result.offset_type;


     }
     	 return(ST_NO_ERROR);


}



/* ----------------------------------------------------------------------------
Name: demod_d0362_ioctl()

Description:
    access device specific low-level functions

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0362_ioctl(DEMOD_Handle_t Handle, U32 Function, void *InParams, void *OutParams, STTUNER_Da_Status_t *Status)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
   const char *identity = "STTUNER d0362.c demod_d0362_ioctl()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0362_InstanceData_t *Instance;
 STTUNER_InstanceDbase_t     *Inst;
Inst = STTUNER_GetDrvInst();
    /* private driver instance data */
    Instance = d0362_GetInstFromHandle(Handle);

    if(STTUNER_Util_CheckPtrNull(Instance) != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
        STTBX_Print(("%s invalid handle\n", identity));
#endif
        return(ST_ERROR_INVALID_HANDLE);
    }

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
        STTBX_Print(("%s demod driver instance handle=%d\n", identity, Handle));
        STTBX_Print(("%s                     function=%d\n", identity, Function));
        STTBX_Print(("%s                   I/O handle=%d\n", identity, Instance->IOHandle));
#endif

    /* ---------- select what to do ---------- */
    switch(Function){

        case STTUNER_IOCTL_REG_IN: /* read device register */
            *(int *)OutParams = ChipGetOneRegister(Instance->IOHandle, *(int *)InParams);
            break;

        case STTUNER_IOCTL_REG_OUT: /* write device register */
            Error =  ChipSetOneRegister(Instance->IOHandle,
                  ((TERIOCTL_SETREG_InParams_t *)InParams)->RegIndex,
                  ((TERIOCTL_SETREG_InParams_t *)InParams)->Value );
            break;

	case STTUNER_IOCTL_SET_30MHZ_REG: /* set TRL registers through ioctl */
			Inst->IOCTL_Set_30MZ_REG_Flag=FALSE;
    			Error = demod_d0362_PLLDIV_TRL_INC_ioctl_set(Instance->IOHandle,InParams);
			Inst->IOCTL_Set_30MZ_REG_Flag=TRUE;
			break;

	case STTUNER_IOCTL_GET_30MHZ_REG: /* get TRL registers through ioctl */
			Error = demod_d0362_PLLDIV_TRL_INC_ioctl_get(Instance->IOHandle,OutParams);
			break;
	case STTUNER_IOCTL_SET_IF_IQMODE:/* Set IFIQ mode for the 362 NIM */
		  Error = demod_d0362_IF_IQMODE_ioctl_set(&(Instance->IFIQMode),InParams);
		  break;
	case STTUNER_IOCTL_GET_IF_IQMODE:/* Get IFIQ mode for the 362 NIM */
		  Error = demod_d0362_IF_IQMODE_ioctl_get(Instance->IFIQMode,OutParams);
		  break;
        default:
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
            STTBX_Print(("%s function %d not found\n", identity, Function));
#endif
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }


#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
    STTBX_Print(("%s called ok\n", identity));    /* signal that function came back */
#endif

    return(Error);

}






/* ----------------------------------------------------------------------------
Name: demod_d0362_PLLDIV_TRL_INC_ioctl_set()

Description:
    This fuction is used for setting from application some registers for 30 MHZ crystal

Parameters:

Return Value:
---------------------------------------------------------------------------- */


ST_ErrorCode_t demod_d0362_PLLDIV_TRL_INC_ioctl_set( IOARCH_Handle_t IOHandle,void *InParams)

{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
   const char *identity = "STTUNER d0362.c demod_d0362_PLLDIV_TRL_INC_ioctl_set()";
#endif


    STTUNER_demod_IOCTL_30MHZ_REG_t   *Reg30MHZSet;
	U8 trl_ctl[3]={0},R_trlctl,plldiv;
	U8 incderot[2]={0};
	 ST_ErrorCode_t Error = ST_NO_ERROR;


   Reg30MHZSet = (STTUNER_demod_IOCTL_30MHZ_REG_t *)InParams;


    /* private driver instance data */



plldiv=Reg30MHZSet->RPLLDIV;
ChipSetOneRegister(IOHandle,R0362_PLLNDIV,plldiv);/*PLLNDIV set for 30 MHZ crystal*/
R_trlctl=ChipGetOneRegister(IOHandle,R0362_TRL_CTL);

trl_ctl[0]=((Reg30MHZSet->TRLNORMRATELSB)<<7) | (R_trlctl & 0x7f);
trl_ctl[1]=Reg30MHZSet->TRLNORMRATELO;
trl_ctl[2]=Reg30MHZSet->TRLNORMRATEHI;
 ChipSetRegisters(IOHandle,R0362_TRL_CTL,trl_ctl,3);/*TRL registers setting for 30 MHz crystal*/

incderot[0]=Reg30MHZSet->INCDEROT1;
incderot[1]=Reg30MHZSet->INCDEROT2;
ChipSetRegisters(IOHandle,R0362_INC_DEROT1,incderot,2);/*iNCDEROT register set for 30 MHZ crystal*/
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
    STTBX_Print(("%s called ok\n", identity));    /* signal that function came back */
#endif
    return(Error);

}


/* ----------------------------------------------------------------------------
Name: demod_d0362_PLLDIV_TRL_INC_ioctl_get()

Description:
    This fuction is used for setting from application some registers for 30 MHZ crystal

Parameters:

Return Value:
---------------------------------------------------------------------------- */

ST_ErrorCode_t demod_d0362_PLLDIV_TRL_INC_ioctl_get( IOARCH_Handle_t IOHandle,void *OutParams)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
   const char *identity = "STTUNER d0362.c demod_d0362_PLLDIV_TRL_INC_ioctl_get()";
#endif
ST_ErrorCode_t Error = ST_NO_ERROR;

    STTUNER_demod_IOCTL_30MHZ_REG_t   *Reg30MHZGet;


   Reg30MHZGet = (STTUNER_demod_IOCTL_30MHZ_REG_t *)OutParams;


 /* private driver instance data */


 Reg30MHZGet->RPLLDIV =ChipGetOneRegister(IOHandle,R0362_PLLNDIV);
 Reg30MHZGet->TRLNORMRATEHI= ChipGetField(IOHandle,F0362_TRL_NOMRATE_HI);
 Reg30MHZGet->TRLNORMRATELO= ChipGetField(IOHandle,F0362_TRL_NOMRATE_LO);
 Reg30MHZGet->TRLNORMRATELSB= ChipGetField(IOHandle,F0362_TRL_NOMRATE_LSB) ;
Reg30MHZGet->INCDEROT1=ChipGetOneRegister(IOHandle,R0362_INC_DEROT1);
Reg30MHZGet->INCDEROT2=ChipGetOneRegister(IOHandle,R0362_INC_DEROT2);

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
    STTBX_Print(("%s called ok\n", identity));    /* signal that function came back */
#endif

    return(Error);

}

/* ----------------------------------------------------------------------------
Name: demod_d0362_IF_IQMODE_ioctl_set()

Description:
    This fuction is used for setting IFIQ mode for 362 driver as set in application

Parameters:

Return Value:
---------------------------------------------------------------------------- */

ST_ErrorCode_t demod_d0362_IF_IQMODE_ioctl_set(STTUNER_IF_IQ_Mode *IFIQMode,void *InParams)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
   const char *identity = "STTUNER d0362.c demod_d0362_IF_IQMODE_ioctl_set()";
#endif
ST_ErrorCode_t Error = ST_NO_ERROR;


#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
     STTBX_Print(("%s IFIQMode %d \n", identity,*IFIQMode));
#endif


    *IFIQMode= *(STTUNER_IF_IQ_Mode *)InParams;

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
    STTBX_Print(("%s called ok \n", identity));    /* signal that function came back */
#endif

    return(Error);

}


/* ----------------------------------------------------------------------------
Name: demod_d0362_IF_IQMODE_ioctl_get()

Description:
    This fuction is used for setting IFIQ mode for 362 driver as set in application

Parameters:

Return Value:
---------------------------------------------------------------------------- */

ST_ErrorCode_t demod_d0362_IF_IQMODE_ioctl_get(STTUNER_IF_IQ_Mode IFIQMode,void *OutParams)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
   const char *identity = "STTUNER d0362.c demod_d0362_IF_IQMODE_ioctl_get()";
#endif
ST_ErrorCode_t Error = ST_NO_ERROR;


#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
     STTBX_Print(("%s IFIQMode %d \n", identity,IFIQMode));
#endif


    *(STTUNER_IF_IQ_Mode *)OutParams= IFIQMode;

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
    STTBX_Print(("%s called ok \n", identity));    /* signal that function came back */
#endif

    return(Error);

}

/* ----------------------------------------------------------------------------
Name: demod_d0362_repeateraccess()

Description:
    called from ioarch.c when some driver does I/O but with the repeater/passthru
    set to point to this function.
Parameters:

Return Value:
---------------------------------------------------------------------------- */


ST_ErrorCode_t demod_d0362_repeateraccess(DEMOD_Handle_t Handle,BOOL REPEATER_STATUS)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0362_InstanceData_t *Instance;
    Instance = d0362_GetInstFromHandle(Handle);  
       if (REPEATER_STATUS)
       {
         Error = ChipSetField( Instance->IOHandle, F0362_I2CT_ON, 1);
       }

    return(Error);
}


/* ----------------------------------------------------------------------------
Name: demod_d0362_Tracking()

Description:
    This routine checks the carrier against a certain threshold value and will
    perform derotator centering, if necessary -- using the ForceTracking
    option ensures that derotator centering is always performed when
    this routine is called.

    This routine should be periodically called once a lock has been
    established in order to maintain the lock.
    This routine checks the carrier against a certain threshold value and will
Parameters:
    ForceTracking, boolean to control whether to always perform
                   derotator centering, regardless of the carrier.

    NewFrequency,  pointer to area where to store the new frequency
                   value -- it may be changed when trying to optimize
                   the derotator.

    SignalFound,   indicates that whether or not we're still locked

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0362_Tracking(DEMOD_Handle_t Handle, BOOL ForceTracking, U32 *NewFrequency, BOOL *SignalFound)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
   const char *identity = "STTUNER d0362.c demod_d0362_Tracking()";
#endif
    FE_362_Error_t           Error = ST_NO_ERROR;
    D0362_InstanceData_t    *Instance;

    Instance = d0362_GetInstFromHandle(Handle);

/****************************************
Tracking should check the lock status. Temporary disabled for testing
*****************************************/
    FE_362_Tracking(Instance->IOHandle,&Instance->UnlockCounter);

    /*Error handling*/
    return(Error);
}

ST_ErrorCode_t demod_d0362_GetTPSCellId(DEMOD_Handle_t Handle, U16  *TPSCellId)
{
	return ST_NO_ERROR;

}


/* ----------------------------------------------------------------------------
Name: demod_d0362_GetRFLevel()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0362_GetRFLevel(DEMOD_Handle_t Handle, S32  *Rflevel)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
   const char *identity = "STTUNER d0362.c demod_d0362_GetRFLevel()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    U32 Data1=0;
    U32 Data2=0;
    D0362_InstanceData_t    *Instance;
    U8	Rfregval;
    S32 RFleveldBm = 0;
          
    /* private driver instance data */
   Instance = d0362_GetInstFromHandle(Handle);
     
    /* Set the START_ADC field of register EN_RF_AGC1 to Enable the RF clock */
    
  
    /* Now read the RF_AGC1_LEVEL field of the register AGC1RF */
    Rfregval = ChipGetField( Instance->IOHandle,  F0362_RF_AGC1_LEVEL_HI);
      
   if (Rfregval >= 255)
    {
    	
    	Data1 = ChipGetField( Instance->IOHandle,  F0362_AGC2_VAL_LO);
   	Data2 = ChipGetField( Instance->IOHandle,  F0362_AGC2_VAL_HI);   
    	Data2 <<=8;
    	Data2 &= 0x0F00;
    	Data2 |= Data1 ;
    	    	
    	
    	if ( Data2 >= 670 && Data2 <= 730)
   	 {
    		RFleveldBm = -((Data2 -100)/20 + 21);
   	 }
   	
    	else if ( Data2 >= 730 && Data2 <= 769)
    	 {
    		RFleveldBm = -((Data2 -100)/20 + 22);
     	 }
   
   else if ( Data2 >= 770 && Data2 <= 820)
    	 {
    		RFleveldBm = -((Data2 -100)/20 + 22);
     	 }
     	 else if ( Data2 >= 821 && Data2 <= 900)
    	 {
    		RFleveldBm = -((Data2 -100)/20 + 22);
     	 }
     	 else  
   	 {
   		RFleveldBm = STTUNER_LOW_RF;
   		STTBX_Print(("RF level is  < -60dBm \n"));	
    	}  
}
   
   
   else if (Rfregval >= 160 && Rfregval <= 170 )
    	{
		RFleveldBm = -(((Rfregval - 50)/2)-39) ;
    	}
  
  else if (Rfregval >= 171 && Rfregval <= 220 )
    	{
    	      RFleveldBm = -(((Rfregval - 50)/2)-38) ;
   	} 
   	 else if (Rfregval >= 221 && Rfregval <= 230 )
    	{
    	      RFleveldBm = -(((Rfregval - 50)/2)-41) ;
   	} 
   	
   	 else if (Rfregval >= 231 && Rfregval <= 245 )
    	{
    	      RFleveldBm = -(((Rfregval - 50)/2)-44) ;
   	} 
   	 else if (Rfregval >= 246 && Rfregval <= 253 )
    	{
    	      RFleveldBm = -(((Rfregval - 50)/2)-51) ;
   	} 
   	 else if (Rfregval == 254  )
    	{
    	      RFleveldBm = -(((Rfregval - 50)/2)-48) ;
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
Name: demod_d0360_StandByMode()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */



ST_ErrorCode_t demod_d0362_StandByMode(DEMOD_Handle_t Handle, STTUNER_StandByMode_t PowerMode)
{

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362
   const char *identity = "STTUNER d0362.c demod_d0362_StandByMode()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0362_InstanceData_t *Instance;

    /* private driver instance data */
    Instance = d0362_GetInstFromHandle(Handle);

    switch ( PowerMode)
    {
       case STTUNER_NORMAL_POWER_MODE :
       if(Instance->StandBy_Flag == 1)
       {

          Error|= ChipSetField( Instance->IOHandle, F0362_STDBY, 0);
          Error|=ChipSetField( Instance->IOHandle, F0362_CORE_ACTIVE, 0);
          WAIT_N_MS_362(2);
          Error|=ChipSetField( Instance->IOHandle, F0362_CORE_ACTIVE, 1);


       if(Error==ST_NO_ERROR)
       {
          Instance->StandBy_Flag = 0 ;
       }

       break;
      }
       case STTUNER_STANDBY_POWER_MODE :
       if(Instance->StandBy_Flag == 0)
       {
          Error|=ChipSetField( Instance->IOHandle, F0362_STDBY, 1);
          if(Error==ST_NO_ERROR)
          {
             Instance->StandBy_Flag = 1 ;

          }
           break ;
       }

	   /* Switch statement */

    }

    return(Error);
}
/* ------------------------------------------------------------------------- */
/* /\/\/\/\/\/\/\/\/\/\/\/\/\UTILITY Functions/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ */
/* ------------------------------------------------------------------------- */

D0362_InstanceData_t *d0362_GetInstFromHandle(DEMOD_Handle_t Handle)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362_HANDLES
   const char *identity = "STTUNER d0362.c d0362_GetInstFromHandle()";
#endif
    D0362_InstanceData_t *Instance;

    Instance = (D0362_InstanceData_t *)Handle;
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362_HANDLES
    STTBX_Print(("%s block at 0x%08x\n", identity, Instance));
#endif

    return(Instance);
}




/* End of d0362.c */
