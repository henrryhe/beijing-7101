/* ----------------------------------------------------------------------------
File Name: d0372.c

Description:

    stv0372 8VSB demod driver.


Copyright (C) 2004-2006 STMicroelectronics

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
#include "chip.h"

/* LLA */


#include "d0372_drv.h"    /* misc driver functions */
#include "d0372.h"      /* header for this file */
#include "d0372_map.h"

#include "tioctl.h"     /* data structure typedefs for all the ter ioctl functions */


U16  STV0372_Address[STV0372_NBREGS]=
{
0xf000,     /* ID */
0xf001 ,    /* I2C_PAGE */
0xf002,     /* I2CRPT1 */
0xf003,     /* I2CRPT2 */
0xf004,     /* CLK_CTRL */
0xf005,     /* STANDBY */
0xf006 ,    /* IO_CTRL */
0xf007,     /* GPIO_INFO */
0xf610,     /* PLL_CTRL */
0xf611,     /* RESERVED */
0xf61c,     /* AD10_CTRL */
0xf010,     /* DEMOD_CTRL */
0xf011,     /* SYNCCTRL */
0xf012,     /* AGCCTRL1 */
0xf013,     /* AGCCTRL2 */
0xf014,     /* AGCPWR0372_LSB */
0xf015,     /* AGCPWR0372_MSB */
0xf016,     /* AGCITHUP_LSB */
0xf017,     /* AGCITHUP_MSB */
0xf018,     /* AGCITHLOW_LSB */
0xf019,     /* AGCITHLOW_MSB */
0xf01a,     /* AGCTH_LSB */
0xf01b,     /* AGCTH_MSB */
0xf01c,     /* AGCBWSEL */
0xf01d,     /* TAGCBWSEL */
0xf01e,     /* TST_PWM1 */
0xf01f,     /* TST_PWM2 */
0xf020,     /* TST_PWM3 */
0xf021,     /* AGC_IND_LSB */
0xf022,     /* AGC_IND_MSB */
0xf023,     /* AGC_IND_MMSB */
0xf024,     /* VCXOOFFSET1 */
0xf025,     /* VCXOOFFSET2 */
0xf026,     /* VCXOOFFSET3 */
0xf027,     /* VCXOOFFSET4 */
0xf028,     /* GAINSRC_LSB */
0xf029,     /* GAINSRC_MSB */
0xf02a,     /* VCXOINITV */
0xf02b,     /* GAIN1ACQ1_LSB */
0xf02c,     /* GAIN1ACQ1_MSB */
0xf02d,     /* GAIN1ACQ2_LSB */
0xf02e,     /* GAIN1ACQ2_MSB */
0xf02f,     /* GAIN1TRACK_LSB */
0xf030,     /* GAIN1TRACK_MSB */
0xf031,     /* GAIN2ACQ1_LSB */
0xf032,     /* GAIN2ACQ1_MSB */
0xf033,     /* GAIN2ACQ2_LSB */
0xf034,     /* GAIN2ACQ2_MSB */
0xf035,     /* GAIN2TRK_LSB */
0xf036,     /* GAIN2TRK_MSB */
0xf037,     /* GAIN3ACQ */
0xf038,     /* GAIN3TRK */
0xf039,     /* VCXOERR0372_LSB */
0xf03a,     /* VCXOERR0372_MSB */
0xf03b,     /* VCXOERR0372_MMSB */
0xf03c,     /* TIMLOCKDETECT_LSB */
0xf03d,     /* TIMLOCKDETECT_MSB */
0xf03e,     /* TIMLOCKDETECT_MMSB */
0xf03f,     /* FREQLOCK_LSB */
0xf040,     /* FREQLOCK_MSB */
0xf041,     /* FREQLOCK_MMSB */
0xf042,     /* TIMINGAGCREF_LSB */
0xf043 ,    /* TIMINGAGCREF_MSB */
0xf044,     /* NCOCNST_LSB */
0xf045,     /* NCOCNST_MSB */
0xf046,     /* NCOCNST_MMSB */
0xf047,     /* NCOGAIN1ACQ */
0xf048,     /* NCOGAIN1TRACK */
0xf049,     /* NCOGAIN2ACQ */
0xf04a,     /* NCOGAIN2TRACK */
0xf04b,     /* NCOGAIN3 */
0xf04c,     /* NCOERR_LSB */
0xf04d,     /* NCOERR_MSB */
0xf04e,     /* NCOERR_MMSB */
0xf04f ,    /* CARLOCKDETECT1_LSB */
0xf050,     /* CARLOCKDETECT1_MSB */
0xf051,     /* CARLOCKDETECT1_MMSB */
0xf052,     /* CARLOCKDETECT2_LSB */
0xf053,     /* CARLOCKDETECT2_MSB */
0xf054,     /* CARLOCKDETECT2_MMSB */
0xf055,     /* CARRIERLOCKTH_LSB */
0xf056,     /* CARRIERLOCKTH_MSB */
0xf057,     /* CARRIERLOCKTH_MMSB */
0xf058,     /* CARRIERAGCREF_LSB */
0xf059,     /* CARRIERAGCREF_MSB */
0xf05a,     /* CARAGCMIXRATIO */
0xf05b,     /* FSM1 */
0xf05c,     /* FSM2 */
0xf05d,     /* MAINSMUP */
0xf05e,     /* EQSMUP */
0xf05f,     /* STATEVAL_LSB */
0xf060,     /* STATEVAL_MSB */
0xf061,     /* STATEVAL_MMSB */
0xf062,     /* UPDATEVCXO_LSB */
0xf063,     /* UPDATEVCXO_MSB */
0xf064,     /* UPDATEVCXO_MMSB */
0xf065 ,    /* MAXNBFRAMERCA */
0xf066,     /* MAXNBFRAMEDD_LSB */
0xf067 ,    /* MAXNBFRAMEDD_MSB */
0xf068 ,    /* NCO_TIMEOUT_LSB */
0xf069 ,    /* NCO_TIMEOUT_MSB */
0xf06a ,    /* NCO_TIMEOUT_MMSB */
0xf06b ,    /* DEMSTATUS */
0xf06c ,    /* SYSCTRL */
0xf06d ,    /* SEG_INIT_LSB */
0xf06e ,    /* SEG_INIT_MSB */
0xf06f ,    /* CENTROIDCALDONE_LSB */
0xf070 ,    /* CENTROIDCALDONE_MSB */
0xf071 ,    /* CENTROIDOFFSET_LSB */
0xf072 ,    /* CENTROIDOFFSET_MSB */
0xf073 ,    /* FFEGAINTRAIN */
0xf074 ,    /* FFEGAINRCA1 */
0xf075 ,    /* FFEGAINRCA2 */
0xf076 ,    /* FFEGAINDDM1 */
0xf077 ,    /* FFEGAINDDM2 */
0xf078 ,    /* DFEGAINTRAIN */
0xf079 ,    /* DFEGAINRCA1 */
0xf07a ,    /* DFEGAINRCA2 */
0xf07b ,    /* DFEGAINDDM1 */
0xf07c ,    /* DFEGAINDDM2 */
0xf07d ,    /* SNR0372_TH_DDM1 */
0xf07e ,    /* SNR0372_TH_DDM2 */
0xf07f,     /* SNR0372_TH_PT */
0xf080,     /* SNR0372_TH_RCA1 */
0xf081,     /* SNR0372_TH_RCA2 */
0xf082,     /* LEAKFFETRAIN */
0xf083,     /* LEAKFFERCA */
0xf084,     /* LEAKFFEDDM */
0xf085,     /* LEAKDFETRAIN */
0xf086,     /* LEAKDFERCA */
0xf087,     /* LEAKDFEDDM */
0xf088,     /* TRACK_REG */
0xf089,     /* DFE_CTRL */
0xf08a,     /* TAP_CTRL */
0xf08b,     /* TAP_POINTER_LSB */
0xf08c ,    /* TAP_POINTER_MSB */
0xf08d,     /* TAP_GETBYTE */
0xf08e,     /* TAP_PUTBYTE */
0xf08f,     /* DECCTRL */
0xf090,     /* DECSYNCCTRL */
0xf091,     /* SER_LSB */
0xf092,     /* SER_MSB */
0xf093,     /* SER_PERIOD_LSB */
0xf094,     /* SER_PERIOD_MSB */
0xf095,     /* DEC_STATUS */
0xf096,     /* SIGNAL_POWER_LSB */
0xf097,     /* SIGNAL_POWER_MSB */
0xf098,     /* ERROR_POWER_LSB */
0xf099,     /* ERROR_POWER_MSB */
0xf09a,     /* BERCTRL */
0xf09b,     /* BER0372_LSB */
0xf09c,     /* BER_MSB */
0xf09d,     /* BER_PERIOD_LLSB */
0xf09e,     /* BER_PERIOD_LSB */
0xf09f,     /* BER_PERIOD_MSB */
0xf0a0,     /* BER_PERIOD_MMSB */
0xf0a1,     /* FRAMECORTH_LSB */
0xf0a2,     /* FRAMECORTH_MSB */
0xf0a3,     /* FRAMELOCKDELOCK */
0xf0a4,     /* CI_BYTE_CLK */
0xf0a5,     /* TS_FORMAT */
0xf0a6,     /* ACI_CTRL */
0xf0a7,     /* ACI_CHANNEL */
0xf0a8,     /* ACI_PROGRAM_LSB */
0xf0a9     /* ACI_PROGRAM_MSB */ 

};

#define MAX_ADDRESS 0xf0a9

U8  STV0372_DefVal[STV0372_NBREGS]=
{ 
0x10,   /*R0372_ID          	     -   0xf000 */
0x00,   /*R0372_I2C_PAGE   	     -   0xf001 */
0x32,   /*R0372_I2CRPT1     	     -   0xf002 */ 
0x07,   /*R0372_I2CRPT2    	     -   0xf003 */
0x0a,   /*R0372_CLK_CTRL   	     -   0xf004 */
0xcd,   /*R0372_STANDBY   	     -   0xf005 */
0x58,   /*R0372_IO_CTRL    	     -   0xf006 *//*To disable TS O/P from high impedance mode and make TS enable */
0x18,   /*R0372_GPIO_INFO   	     -   0xf007 */ 
0xc7,   /*R0372_PLL_CTRL    	     -   0xf610 */
0x00,    /*R0372_RESERVED    	     -   0xf611 */ /*Given reset value to make compatible with old files*/
0x14,   /*R0372_AD10_CTRL            -   0xf61c */
0x00,   /*R0372_DEMOD_CTRL           -   0xf010 */
0x00,   /*R0372_SYNCCTRL             -   0xf011 */
0x80,   /*R0372_AGCCTRL1             -   0xf012 */
0x00,   /*R0372_AGCCTRL2	     -   0xf013 */
0x72,   /*R0372_AGCPWR_LSB           -   0xf014 */
0xf0,   /*R0372_AGCPWR_MSB           -   0xf015 */
0xff,   /*R0372_AGCITHUP_LSB             -   0xf016 */
0x07,   /*R0372_AGCITHUP_MSB             -   0xf017 */
0x00,   /*R0372_AGCITHLOW_LSB            -   0xf018 */
0x08,   /*R0372_AGCITHLOW_MSB            -   0xf019 */
0x80,   /*R0372_AGCTH_LSB                -   0xf01a */
0x03,   /*R0372_AGCTH_MSB                -   0xf01b */
0x06,   /*R0372_AGCBWSEL                 -   0xf01c */
0x01,   /*R0372_TAGCBWSEL                -   0xf01d */
0x81,   /*R0372_TST_PWM1                 -   0xf01e */
0x53,   /*R0372_TST_PWM2                 -   0xf01f */
0xef,   /*R0372_TST_PWM3                 -   0xf020 */
0xca,   /*R0372_AGC_IND_LSB              -   0xf021 */
0xf1,   /*R0372_AGC_IND_MSB              -   0xf022 */
0x03,   /*R0372_AGC_IND_MMSB             -   0xf023 */
0x98,   /*R0372_VCXOOFFSET1              -   0xf024 */
0x5e,   /*R0372_VCXOOFFSET2              -   0xf025 */
0x05,   /*R0372_VCXOOFFSET3              -   0xf026 */
0xb3,   /*R0372_VCXOOFFSET4              -   0xf027 */
0xba,   /*R0372_GAINSRC_LSB              -   0xf028 */
0x3f,   /*R0372_GAINSRC_MSB              -   0xf029 */
0x00,   /*R0372_VCXOINITV                -   0xf02a */
0x00,   /*R0372_GAIN1ACQ1_LSB            -   0xf02b */
0x7d,   /*R0372_GAIN1ACQ1_MSB            -   0xf02c */
0x20,   /*R0372_GAIN1ACQ2_LSB            -   0xf02d */
0xf4,   /*R0372_GAIN1ACQ2_MSB            -   0xf02e */
0x10,   /*R0372_GAIN1TRACK_LSB           -   0xf02f */
0xd8,   /*R0372_GAIN1TRACK_MSB           -   0xf030 */
0xff,   /*R0372_GAIN2ACQ1_LSB            -   0xf031 */
0x83,   /*R0372_GAIN2ACQ1_MSB            -   0xf032 */
0x00,   /*R0372_GAIN2ACQ2_LSB    	     -   0xf033 */
0x79,   /*R0372_GAIN2ACQ2_MSB            -   0xf034 */
0x80,   /*R0372_GAIN2TRK_LSB             -   0xf035 */
0xe4,   /*R0372_GAIN2TRK_MSB             -   0xf036 */
0x05,   /*R0372_GAIN3ACQ                 -   0xf037 */
0x05,   /*R0372_GAIN3TRK 		     -   0xf038 */
0x5e,   /*R0372_VCXOERR_LSB              -   0xf039 */
0xf0,   /*R0372_VCXOERR_MSB     	     -   0xf03a */
0xfe,   /*R0372_VCXOERR_MMSB             -   0xf03b */
0xa3,   /*R0372_TIMLOCKDETECT_LSB        -   0xf03c */
0xd8,   /*R0372_TIMLOCKDETECT_MSB        -   0xf03d */
0x05,   /*R0372_TIMLOCKDETECT_MMSB       -   0xf03e */
0x00,   /*R0372_FREQLOCK_LSB             -   0xf03f */
0x77,   /*R0372_FREQLOCK_MSB             -   0xf040 */
0x01,   /*R0372_FREQLOCK_MMSB            -   0xf041 */
0x00,   /*R0372_TIMINGAGCREF_LSB         -   0xf042 */
0x14,   /*R0372_TIMINGAGCREF_MSB         -   0xf043 */
0x4b,   /*R0372_NCOCNST_LSB		     -   0xf044 */
0x68,   /*R0372_NCOCNST_MSB              -   0xf045 */
0x2f,   /*R0372_NCOCNST_MMSB             -   0xf046 */
0x02,   /*R0372_NCOGAIN1ACQ              -   0xf047 */
0x02,   /*R0372_NCOGAIN1TRACK            -   0xf048 */
0x08,   /*R0372_NCOGAIN2ACQ              -   0xf049 */
0x08,   /*R0372_NCOGAIN2TRACK            -   0xf04a */
0x06,   /*R0372_NCOGAIN3 		     -   0xf04b */
0xf1,   /*R0372_NCOERR_LSB		     -   0xf04c */
0x0f,   /*R0372_NCOERR_MSB		     -   0xf04d */
0x00,   /*R0372_NCOERR_MMSB		     -   0xf04e */
0x13,   /*R0372_CARLOCKDETECT1_LSB       -   0xf04f */
0x45,   /*R0372_CARLOCKDETECT1_MSB       -   0xf050 */
0x05,   /*R0372_CARLOCKDETECT1_MMSB      -   0xf051 */
0x90,   /*R0372_CARLOCKDETECT2_LSB	     -   0xf052 */
0x8f,   /*R0372_CARLOCKDETECT2_MSB       -   0xf053 */
0x01,   /*R0372_CARLOCKDETECT2_MMSB      -   0xf054 */
0x00,   /*R0372_CARRIERLOCKTH_LSB        -   0xf055 */
0x77,   /*R0372_CARRIERLOCKTH_MSB        -   0xf056 */
0xc5,   /*R0372_CARRIERLOCKTH_MMSB       -   0xf057 */
0x00,   /*R0372_CARRIERAGCREF_LSB        -   0xf058 */
0xc2,   /*R0372_CARRIERAGCREF_MSB        -   0xf059 */
0x14,   /*R0372_CARAGCMIXRATIO 	     -   0xf05a */
0x00,   /*R0372_FSM1                     -   0xf05b */
0x00,   /*R0372_FSM2		     -   0xf05c */
0xff,   /*R0372_MAINSMUP                 -   0xf05d */
0xff,   /*R0372_EQSMUP		     -   0xf05e */
0xff,   /*R0372_STATEVAL_LSB             -   0xf05f */
0x0f,   /*R0372_STATEVAL_MSB             -   0xf060 */
0x90,   /*R0372_STATEVAL_MMSB            -   0xf061 */
0x00,   /*R0372_UPDATEVCXO_LSB           -   0xf062 */
0x00,   /*R0372_UPDATEVCXO_MSB           -   0xf063 */
0x8e,   /*R0372_UPDATEVCXO_MMSB          -   0xf064 */
0x1a,   /*R0372_MAXNBFRAMERCA            -   0xf065 */
0xc8,   /*R0372_MAXNBFRAMEDD_LSB         -   0xf066 */
0x00,   /*R0372_MAXNBFRAMEDD_MSB	     -   0xf067 */
0xff,   /*R0372_NCO_TIMEOUT_LSB          -   0xf068 */
0xff,   /*R0372_NCO_TIMEOUT_MSB          -   0xf069 */
0xff,   /*R0372_NCO_TIMEOUT_MMSB 	     -   0xf06a */
0x5a,   /*R0372_DEMSTATUS		     -   0xf06b */
0x70,   /*R0372_SYSCTRL    		     -   0xf06c */
0x00,   /*R0372_SEG_INIT_LSB	     -   0xf06d */
0xe1,   /*R0372_SEG_INIT_MSB	     -   0xf06e */
0x37,   /*R0372_CENTROIDCALDONE_LSB	     -   0xf06f */
0xf1,   /*R0372_CENTROIDCALDONE_MSB	     -   0xf070 */
0x00,   /*R0372_CENTROIDOFFSET_LSB 	     -   0xf071 */
0x00,   /*R0372_CENTROIDOFFSET_MSB	     -   0xf072 */
0x40,   /*R0372_FFEGAINTRAIN	     -   0xf073 */
0x09,   /*R0372_FFEGAINRCA1	             -   0xf074 */
0x05,   /*R0372_FFEGAINRCA2 	     -   0xf075 */
0x10,   /*R0372_FFEGAINDDM1		     -   0xf076 */
0x08,   /*R0372_FFEGAINDDM2		     -   0xf077 */
0x40,   /*R0372_DFEGAINTRAIN	     -   0xf078 */
0x20,   /*R0372_DFEGAINRCA1 	     -   0xf079 */
0x10,   /*R0372_DFEGAINRCA2 	     -   0xf07a */
0x20,   /*R0372_DFEGAINDDM1		     -   0xf07b */
0x20,   /*R0372_DFEGAINDDM2	             -   0xf07c */
0xca,   /*R0372_SNR_TH_DDM1	             -   0xf07d */
0xca,   /*R0372_SNR_TH_DDM2		     -   0xf07e */
0xd8,   /*R0372_SNR_TH_PT		     -   0xf07f */
0x53,   /*R0372_SNR_TH_RCA1		     -   0xf080 */
0x75,   /*R0372_SNR_TH_RCA2		     -   0xf081 */
0x0d,   /*R0372_LEAKFFETRAIN	     -   0xf082 */
0x77,   /*R0372_LEAKFFERCA		     -   0xf083 */
0x97,   /*R0372_LEAKFFEDDM		     -   0xf084 */
0x08,   /*R0372_LEAKDFETRAIN	     -   0xf085 */
0x44,   /*R0372_LEAKDFERCA		     -   0xf086 */
0x22,   /*R0372_LEAKDFEDDM		     -   0xf087 */
0x30,   /*R0372_TRACK_REG		     -   0xf088 */
0xb8,   /*R0372_DFE_CTRL		     -   0xf089 */
0x00,   /*R0372_TAP_CTRL                 -   0xf08a */
0x3f,   /*R0372_TAP_POINTER_LSB	     -   0xf08b */
0x01,   /*R0372_TAP_POINTER_MSB          -   0xf08c */
0x00,   /*R0372_TAP_GETBYTE 	     -   0xf08d */
0x00,   /*R0372_TAP_PUTBYTE		     -   0xf08e */
0x00,   /*R0372_DECCTRL		     -   0xf08f */
0x00,   /*R0372_DECSYNCCTRL		     -   0xf090 */
0x00,   /*R0372_SER_LSB		     -   0xf091 */
0x00,   /*R0372_SER_MSB		     -   0xf092 */
0x10,   /*R0372_SER_PERIOD_LSB           -   0xf093 */		
0x27,   /*R0372_SER_PERIOD_MSB	     -   0xf094 */  
0x3a,   /*R0372_DEC_STATUS		     -   0xf095 */
0xbe,   /*R0372_SIGNAL_POWER_LSB	     -   0xf096 */
0x03,   /*R0372_SIGNAL_POWER_MSB         -   0xf097 */
0x07,   /*R0372_ERROR_POWER_LSB	     -   0xf098 */
0x00,   /*R0372_ERROR_POWER_MSB	     -   0xf099 */
0x8b,   /*R0372_BERCTRL		     -   0xf09a */
0xff,   /*R0372_BER_LSB		     -   0xf09b */
0xff,   /*R0372_BER_MSB		     -   0xf09c */
0xbe,   /*R0372_BER_PERIOD_LLSB          -   0xf09d */  
0xc8,   /*R0372_BER_PERIOD_LSB 	     -   0xf09e */
0x24,   /*R0372_BER_PERIOD_MSB	     -   0xf09f */
0x00,   /*R0372_BER_PERIOD_MMSB	     -   0xf0a0 */
0x00,   /*R0372_FRAMECORTH_LSB	     -   0xf0a1 */
0x30,   /*R0372_FRAMECORTH_MSB	     -   0xf0a2 */
0x41,   /*R0372_FRAMELOCKDELOCK 	     -   0xf0a3 */
0x44,   /*R0372_CI_BYTE_CLK		     -   0xf0a4 */
0x02,   /*R0372_TS_FORMAT                -   0xf0a5 */	
0x00,   /*R0372_ACI_CTRL		     -   0xf0a6 */
0x00,   /*R0372_ACI_CHANNEL		     -   0xf0a7 */
0x01,   /*R0372_ACI_PROGRAM_LSB	     -   0xf0a8 */
0x00  /*R0372_ACI_PROGRAM_MSB	     -   0xf0a9 */
};

       
/* Private types/constants ------------------------------------------------ */



/* Device capabilities */
#define MAX_AGC                         4095 
#define MAX_SIGNAL_QUALITY              100
#define MAX_BER                         200000
#define STCHIP_HANDLE(x) ((STCHIP_InstanceData_t *)x)

/* private variables ------------------------------------------------------- */

static semaphore_t *Lock_InitTermOpenClose; /* guard calls to the functions */

static BOOL        Installed = FALSE;


extern IOARCH_HandleData_t IOARCH_Handle[STTUNER_IOARCH_MAX_HANDLES];



/* instance chain, the default boot value is invalid, to catch errors */
static D0372_InstanceData_t *InstanceChainTop = (D0372_InstanceData_t *)0x7fffffff;

/* functions --------------------------------------------------------------- */


/* API */
ST_ErrorCode_t demod_d0372_Init(ST_DeviceName_t *DeviceName, DEMOD_InitParams_t *InitParams);
ST_ErrorCode_t demod_d0372_Term(ST_DeviceName_t *DeviceName, DEMOD_TermParams_t *TermParams);

ST_ErrorCode_t demod_d0372_Open (ST_DeviceName_t *DeviceName, DEMOD_OpenParams_t  *OpenParams, DEMOD_Capability_t *Capability, DEMOD_Handle_t *Handle);
ST_ErrorCode_t demod_d0372_Close(DEMOD_Handle_t  Handle, DEMOD_CloseParams_t *CloseParams);

ST_ErrorCode_t demod_d0372_GetTunerInfo    (DEMOD_Handle_t Handle, STTUNER_TunerInfo_t *TunerInfo_p);
ST_ErrorCode_t demod_d0372_GetSignalQuality(DEMOD_Handle_t Handle, U32  *SignalQuality_p, U32 *Ber);
ST_ErrorCode_t demod_d0372_GetModulation   (DEMOD_Handle_t Handle, STTUNER_Modulation_t *Modulation);
ST_ErrorCode_t demod_d0372_GetAGC          (DEMOD_Handle_t Handle, S16                  *Agc);
ST_ErrorCode_t demod_d0372_GetFECRates     (DEMOD_Handle_t Handle, STTUNER_FECRate_t    *FECRates);
ST_ErrorCode_t demod_d0372_IsLocked        (DEMOD_Handle_t Handle, BOOL                 *IsLocked);
ST_ErrorCode_t demod_d0372_StandByMode     (DEMOD_Handle_t Handle, STTUNER_StandByMode_t PowerMode);

 ST_ErrorCode_t demod_d0372_ScanFrequency   (DEMOD_Handle_t  Handle,
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
ST_ErrorCode_t demod_d0372_ioctl           (DEMOD_Handle_t Handle, U32 Function, void *InParams, void *OutParams,
                                             STTUNER_Da_Status_t *Status);



/* local functions --------------------------------------------------------- */

D0372_InstanceData_t *d0372_GetInstFromHandle(DEMOD_Handle_t Handle);
ST_ErrorCode_t demod_d0372_repeateraccess(DEMOD_Handle_t Handle,BOOL REPEATER_STATUS);

/* ----------------------------------------------------------------------------
Name: STTUNER_DRV_DEMOD_STV0372_Install()

Description:
    install a terrestrial device driver(8VSB) into the demod database.

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_DRV_DEMOD_STV0372_Install(STTUNER_demod_dbase_t *Demod)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0372
   const char *identity = "STTUNER d0372.c STTUNER_DRV_DEMOD_STV0372_Install()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if(Installed == TRUE)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0372
        STTBX_Print(("%s fail driver already installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0372
    STTBX_Print(("%s installing ter:demod:STV0372...", identity));
#endif

    /* mark ID in database */
    Demod->ID = STTUNER_DEMOD_STV0372;

    /* map API */
    Demod->demod_Init = demod_d0372_Init;
    Demod->demod_Term = demod_d0372_Term;

    Demod->demod_Open  = demod_d0372_Open;
    Demod->demod_Close = demod_d0372_Close;

    Demod->demod_IsAnalogCarrier  = NULL;
    Demod->demod_GetTunerInfo     = demod_d0372_GetTunerInfo;
    Demod->demod_GetSignalQuality = demod_d0372_GetSignalQuality;
    Demod->demod_GetModulation    = demod_d0372_GetModulation;
    Demod->demod_GetAGC           = demod_d0372_GetAGC;
    Demod->demod_GetMode          = NULL;
    Demod->demod_GetGuard         = NULL;
    Demod->demod_GetFECRates      = demod_d0372_GetFECRates;
    Demod->demod_IsLocked         = demod_d0372_IsLocked ;
    Demod->demod_SetFECRates      = NULL;
    Demod->demod_Tracking         = NULL;
    Demod->demod_ScanFrequency    = demod_d0372_ScanFrequency;
    Demod->demod_StandByMode      = demod_d0372_StandByMode;
    Demod->demod_repeateraccess = demod_d0372_repeateraccess;
    Demod->demod_ioaccess = NULL;
    Demod->demod_ioctl    = demod_d0372_ioctl;
    Demod->demod_GetRFLevel	  = NULL;

    InstanceChainTop = NULL;


     Lock_InitTermOpenClose = STOS_SemaphoreCreateFifo(NULL,1);



    Installed = TRUE;

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0372
    STTBX_Print(("ok\n"));
#endif

    return(Error);
}



/* ----------------------------------------------------------------------------
Name: STTUNER_DRV_DEMOD_STV0372_UnInstall()

Description:
    uninstall a terrestrial device driver(8VSB) into the demod database.

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_DRV_DEMOD_STV0372_UnInstall(STTUNER_demod_dbase_t *Demod)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0372
   const char *identity = "STTUNER d0372.c STTUNER_DRV_DEMOD_STV0372_UnInstall()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0372
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

    if(Demod->ID != STTUNER_DEMOD_STV0372)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0372
        STTBX_Print(("%s fail incorrect driver type\n", identity));
#endif
        return(STTUNER_ERROR_ID);
    }

    /* has all memory been freed, by Term() */
    if(InstanceChainTop != NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0372
        STTBX_Print(("%s fail at least one instance not terminated\n", identity));
#endif
        return(ST_ERROR_OPEN_HANDLE);
    }


#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0372
    STTBX_Print(("%s uninstalling ter:demod:STV0372...", identity));
#endif

    /* mark ID in database */
    Demod->ID = STTUNER_NO_DRIVER;

    /* unmap API */
    Demod->demod_Init = NULL;
    Demod->demod_Term = NULL;

    Demod->demod_Open  = NULL;
    Demod->demod_Close = NULL;
    Demod->demod_StandByMode      = NULL;
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
    Demod->demod_repeateraccess = NULL;
    Demod->demod_ioaccess = NULL;
    Demod->demod_ioctl    = NULL;
    Demod->demod_GetRFLevel	  = NULL;

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0372
        STTBX_Print(("<"));
#endif


        STOS_SemaphoreDelete(NULL,Lock_InitTermOpenClose);


#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0372
        STTBX_Print((">"));
#endif

    InstanceChainTop = (D0372_InstanceData_t *)0x7ffffffe;
    Installed        = FALSE;

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0372
        STTBX_Print(("ok\n"));
#endif

    return(Error);
}



/* ------------------------------------------------------------------------- */
/* /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ API /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ */
/* ------------------------------------------------------------------------- */



/* ----------------------------------------------------------------------------
Name: demod_d0372_Init()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0372_Init(ST_DeviceName_t *DeviceName, DEMOD_InitParams_t *InitParams)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0372
    const char *identity = "STTUNER d0372.c demod_d0372_Init()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0372_InstanceData_t *InstanceNew, *Instance;
    
    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0372
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
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0372
        STTBX_Print(("%s fail MemoryPartition not valid\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    Error = STTUNER_Util_CheckPtrNull(DeviceName);
    if( Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0372
        STTBX_Print(("%s fail DeviceName not valid\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    InstanceNew = STOS_MemoryAllocateClear(InitParams->MemoryPartition, 1, sizeof( D0372_InstanceData_t ));
    if (InstanceNew == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0372
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
  
    
    IOARCH_Handle[InstanceNew->IOHandle].DeviceMap.Timeout   = IOREG_DEFAULT_TIMEOUT;
    IOARCH_Handle[InstanceNew->IOHandle].DeviceMap.Registers = STV0372_NBREGS;
    IOARCH_Handle[InstanceNew->IOHandle].DeviceMap.Fields    = STV0372_NBFIELDS;
    IOARCH_Handle[InstanceNew->IOHandle].DeviceMap.Mode      = IOREG_MODE_SUBADR_16; /* i/o addressing mode to use */
    IOARCH_Handle[InstanceNew->IOHandle].DeviceMap.MemoryPartition = InitParams->MemoryPartition;
    IOARCH_Handle[InstanceNew->IOHandle].DeviceMap.DefVal= (U32 *)&STV0372_DefVal[0];    
    IOARCH_Handle[InstanceNew->IOHandle].DeviceMap.ByteStream = STOS_MemoryAllocateClear(IOARCH_Handle[InstanceNew->IOHandle].DeviceMap.MemoryPartition,MAX_ADDRESS,sizeof(U8));


    
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0372
    STTBX_Print(("%s allocated & initalized block named '%s' at 0x%08x (%d bytes)\n", identity, InstanceNew->DeviceName, (U32)InstanceNew, sizeof( D0372_InstanceData_t ) ));
#endif
    SEM_UNLOCK(Lock_InitTermOpenClose);
    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0372_Term()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0372_Term(ST_DeviceName_t *DeviceName, DEMOD_TermParams_t *TermParams)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0372
   const char *identity = "STTUNER d0372.c demod_d0372_Term()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0372_InstanceData_t *Instance, *InstancePrev, *InstanceNext;

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0372
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
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0372
        STTBX_Print(("%s fail TermParams not valid\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    /* ---------- check that at least one init has taken place ---------- */
    if(InstanceChainTop == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0372
        STTBX_Print(("%s fail nothing initalized\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(STTUNER_ERROR_INITSTATE);
    }


    /* reap next matching DeviceName */
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0372
    STTBX_Print(("%s looking for first free handle[\n", identity));
#endif

    Instance = InstanceChainTop;
    while(1)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0372
        STTBX_Print(("(%s)", Instance->DeviceName));
#endif
        if ( strcmp( (char *)Instance->DeviceName, (char *)DeviceName) == 0)
        {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0372
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


           STOS_MemoryDeallocate(Instance->MemoryPartition, Instance);
           

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0372
            STTBX_Print(("%s freed block at %0x%08x\n", identity, (U32)Instance ));
#endif

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0372
            STTBX_Print(("%s terminated ok\n", identity));
#endif
						
STOS_MemoryDeallocate(IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition,IOARCH_Handle[Instance->IOHandle].DeviceMap.ByteStream);


#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0372
            STTBX_Print(("%s freed block at %0x%08x\n", identity, (U32)Instance ));
#endif

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0372
            STTBX_Print(("%s terminated ok\n", identity));
#endif

            SEM_UNLOCK(Lock_InitTermOpenClose);
            return(Error);
        }
        else if(Instance->InstanceChainNext == NULL)
        {       /* error we should have found a matching name before the end of the list */
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0372
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


#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0372
    STTBX_Print(("%s FAIL! this point should NEVER be reached\n", identity));
#endif
    SEM_UNLOCK(Lock_InitTermOpenClose);
    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0372_Open()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0372_Open(ST_DeviceName_t *DeviceName, DEMOD_OpenParams_t  *OpenParams, DEMOD_Capability_t *Capability, DEMOD_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0372
   const char *identity = "STTUNER d0372.c demod_d0372_Open()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0372_InstanceData_t     *Instance;
    U8 ChipID = 0;
    U8 index;


    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0372
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

    /* now safe to lock semaphore */
    SEM_LOCK(Lock_InitTermOpenClose);

    /* ---------- check that at least one init has taken place ---------- */
    if(InstanceChainTop == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0372
        STTBX_Print(("%s fail nothing initalized\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(STTUNER_ERROR_INITSTATE);
    }

    /* find  handle from name */
    Instance = InstanceChainTop;
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0372
    STTBX_Print(("%s looking (%s)", identity, Instance->DeviceName));
#endif
    while(strcmp((char *)Instance->DeviceName, (char *)DeviceName) != 0)
    {
        /* error, should have found matching DeviceName before end of list */
        if(Instance->InstanceChainNext == NULL)
        {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0372
            STTBX_Print(("\n%s fail no block named '%s' before end of list\n", identity, DeviceName));
#endif
            SEM_UNLOCK(Lock_InitTermOpenClose);
            return(STTUNER_ERROR_INITSTATE);
        }
        Instance = Instance->InstanceChainNext;   /* next block */
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0372
        STTBX_Print(("(%s)", Instance->DeviceName));
#endif
    }
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0372
        STTBX_Print((" found ok\n"));
#endif

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0372
    STTBX_Print(("%s using block at 0x%08x\n", identity, (U32)Instance));
#endif

    /* check handle IS actually free */
    if(Instance->TopLevelHandle != STTUNER_MAX_HANDLES)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0372
        STTBX_Print(("%s fail driver instance already open\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_OPEN_HANDLE);
    }
    /* else now got pointer to free (and valid) data block */

  
  
    ChipID = ChipGetOneRegister( Instance->IOHandle, R0372_ID);

    if ( (ChipID & 0x10) !=  0x10)   
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0372
        STTBX_Print(("%s fail device not found (0x%02x)\n", identity, ChipID));
#endif
        /* Term LLA */
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0372
        STTBX_Print(("%s FE_372_Term()\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_UNKNOWN_DEVICE);
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0372
        STTBX_Print(("%s device found, release/revision=%u\n", identity, ChipID & 0x10));       
#endif
     }

 /* reset all chip registers */
     for (index=0;index < STV0372_NBREGS;index++)
     {
     ChipSetOneRegister(Instance->IOHandle,STV0372_Address[index],STV0372_DefVal[index]);
     }
        				
    /* Set serial/parallel data mode */
    if (Instance->TSOutputMode == STTUNER_TS_MODE_SERIAL)
    {
        Error |= ChipSetField( Instance->IOHandle, F0372_SERIES, 0x01);
        Error |= ChipSetField( Instance->IOHandle, F0372_SWAP, 0x01);
    }
    else
    {
        Error |= ChipSetField( Instance->IOHandle, F0372_SERIES, 0x00);
        Error |= ChipSetField( Instance->IOHandle, F0372_SWAP, 0x00);
    }
    
    
    /*set data clock polarity inversion mode (rising/falling)*/
    switch(Instance->ClockPolarity)
    {
       case STTUNER_DATA_CLOCK_INVERTED:
    	 Error |= ChipSetField( Instance->IOHandle, F0372_POLARITY, 0x01);
    	 break;
       case STTUNER_DATA_CLOCK_NONINVERTED:
    	 Error |= ChipSetField( Instance->IOHandle, F0372_POLARITY, 0x00);
    	 break;
       case STTUNER_DATA_CLOCK_POLARITY_DEFAULT:            
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

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0372
    STTBX_Print(("%s no EVALMAX tuner driver, F0360_IAGC=0\n", identity));
#endif


    /* finally (as nor more errors to check for, allocate handle */
    Instance->TopLevelHandle = OpenParams->TopLevelHandle;
    *Handle = (U32)Instance;

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0372
    STTBX_Print(("%s opened ok\n", identity));
#endif

    SEM_UNLOCK(Lock_InitTermOpenClose); 
    return(Error);
}




/* ----------------------------------------------------------------------------
Name: demod_d0372_StandByMode()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */



ST_ErrorCode_t demod_d0372_StandByMode(DEMOD_Handle_t Handle, STTUNER_StandByMode_t PowerMode)
{
	
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0372
   const char *identity = "STTUNER d0372.c demod_d0372_StandByMode()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0372_InstanceData_t *Instance;
      
    /* private driver instance data */
    Instance = d0372_GetInstFromHandle(Handle);
   
      
    switch ( PowerMode)
    {
       case STTUNER_NORMAL_POWER_MODE :
       if(Instance->StandBy_Flag == 1)
       {
          Error|=ChipSetField( Instance->IOHandle, F0372_STANDBY_VSB, 0);
          Error|=ChipSetField( Instance->IOHandle, F0372_STANDBY_AD10, 0);
       }
          
             
       if(Error==ST_NO_ERROR)
       {
       	 
          Instance->StandBy_Flag = 0 ;
       }
       
       break;
       case STTUNER_STANDBY_POWER_MODE :
       if(Instance->StandBy_Flag == 0)
       {
          Error|=ChipSetField( Instance->IOHandle, F0372_STANDBY_VSB, 1);
          Error|=ChipSetField( Instance->IOHandle, F0372_STANDBY_AD10, 1);
          if(Error==ST_NO_ERROR)
          {
                    
             Instance->StandBy_Flag = 1 ;
             
          }
       }
       break ;
	   /* Switch statement */ 
  
    }
   
    return(Error);
}





/* ----------------------------------------------------------------------------
Name: demod_d0372_Close()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0372_Close(DEMOD_Handle_t Handle, DEMOD_CloseParams_t *CloseParams)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0372
   const char *identity = "STTUNER d0372.c demod_d0372_Close()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0372_InstanceData_t     *Instance;

    /* private driver instance data */
    Instance = d0372_GetInstFromHandle(Handle);

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0372
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

    SEM_LOCK(Lock_InitTermOpenClose);

    /* ---------- check that at least one init has taken place ---------- */
    if(InstanceChainTop == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0372
        STTBX_Print(("%s fail nothing initalized\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(STTUNER_ERROR_INITSTATE);
    }

    if(Instance->TopLevelHandle == STTUNER_MAX_HANDLES)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0372
        STTBX_Print(("%s fail driver instance not open\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_OPEN_HANDLE);
    }
   
    
    /* indicate instance is closed */
    Instance->TopLevelHandle = STTUNER_MAX_HANDLES;

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0372
    STTBX_Print(("%s closed\n", identity));
#endif

    SEM_UNLOCK(Lock_InitTermOpenClose);
    return(Error);
}


/* ----------------------------------------------------------------------------
Name: demod_d0372_GetTunerInfo()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0372_GetTunerInfo(DEMOD_Handle_t Handle, STTUNER_TunerInfo_t *TunerInfo_p)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0372
   const char *identity = "STTUNER d0372.c demod_d0372_GetTunerInfo()";
#endif
    ST_ErrorCode_t       Error = ST_NO_ERROR;
    D0372_InstanceData_t *Instance;
    STTUNER_Modulation_t CurModulation;
    STTUNER_FECRate_t    CurFECRates;
    U32                  CurFrequency,CurBitErrorRate;
    S32 CurSignalQuality;
     /* private driver instance data */    
    Instance = d0372_GetInstFromHandle(Handle);
    
    CurModulation = STTUNER_MOD_8VSB;
    
    CurFECRates = STTUNER_FEC_2_3;
   
    CurFrequency = TunerInfo_p->Frequency;
    
    /* Read noise estimations for C/N and BER */
    CurSignalQuality = FE_372_GetCarrierToNoiseRatio( Instance->IOHandle);
    CurBitErrorRate=FE_372_GetBitErrorRate( Instance->IOHandle);
    #ifndef ST_OSLINUX
    STOS_TaskLock();
    #endif
    CurSignalQuality = (CurSignalQuality * 10)/36; /*signal qaulity in % calculated considering 36dB as 100% signal-quality,. x*100/36 *10 as value should be devided by 10 */
    /*If it reaches maximum value then force it to max value*/
    if ( CurSignalQuality > MAX_SIGNAL_QUALITY)
    {
      	CurSignalQuality = MAX_SIGNAL_QUALITY;
    }
    TunerInfo_p->FrequencyFound      = CurFrequency;
    TunerInfo_p->SignalQuality       = CurSignalQuality;
    TunerInfo_p->BitErrorRate        = CurBitErrorRate;
    TunerInfo_p->ScanInfo.Modulation = CurModulation;  
    TunerInfo_p->ScanInfo.FECRates   = CurFECRates;
    
    /*Inst[Instance->TopLevelHandle].TunerInfoError = TunerError;)*/
    Error = IOARCH_Handle[Instance->IOHandle].DeviceMap.Error;
    #ifndef ST_OSLINUX
    STOS_TaskUnlock();
    #endif
    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0372_GetSignalQuality()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0372_GetSignalQuality(DEMOD_Handle_t Handle, U32  *SignalQuality_p, U32 *Ber)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0372
   const char *identity = "STTUNER d0372.c demod_d0372_GetSignalQuality()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0372_InstanceData_t *Instance;
       /* private driver instance data */
    Instance = d0372_GetInstFromHandle(Handle);
    
     /* Read noise estimations for C/N and BER */
    *SignalQuality_p = FE_372_GetCarrierToNoiseRatio( Instance->IOHandle);
    *Ber=FE_372_GetBitErrorRate( Instance->IOHandle);
    *SignalQuality_p = (*SignalQuality_p * 10)/36; /*signal qaulity in % calculated considering 36dB as 100% signal-quality,. x*100/36 *10 as value should be devided by 10 */
     
       /*If it reaches maximum value then force it to max value*/
    if ( *SignalQuality_p > MAX_SIGNAL_QUALITY)
    {
    	*SignalQuality_p = MAX_SIGNAL_QUALITY;
    }

   Error = IOARCH_Handle[Instance->IOHandle].DeviceMap.Error;

    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0372_GetModulation()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0372_GetModulation(DEMOD_Handle_t Handle, STTUNER_Modulation_t *Modulation)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0372
   const char *identity = "STTUNER d0372.c demod_d0372_GetModulation()";
#endif
    STTUNER_Modulation_t CurModulation;
    ST_ErrorCode_t Error = ST_NO_ERROR;
    
    CurModulation = STTUNER_MOD_8VSB;
    
    return(Error);
}
 
/* ----------------------------------------------------------------------------
Name: demod_d0372_GetAGC()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0372_GetAGC(DEMOD_Handle_t Handle, S16  *Agc)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0372
   const char *identity = "STTUNER d0372.c demod_d0372_GetAGC()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
   
    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0372_GetFECRates()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0372_GetFECRates(DEMOD_Handle_t Handle, STTUNER_FECRate_t *FECRates)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0372
   const char *identity = "STTUNER d0372.c demod_d0372_GetFECRates()";
#endif
    STTUNER_FECRate_t CurFecRate;
    ST_ErrorCode_t Error = ST_NO_ERROR;
    CurFecRate = STTUNER_FEC_2_3;
    return(Error);
}

/* ----------------------------------------------------------------------------
Name: demod_d0372_IsLocked()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0372_IsLocked(DEMOD_Handle_t Handle, BOOL *IsLocked)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0372
   const char *identity = "STTUNER d0372.c demod_d0372_IsLocked()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0372_InstanceData_t *Instance;
    U32 LockedStatus =0;

    /* private driver instance data */
    Instance = d0372_GetInstFromHandle(Handle);

    LockedStatus = ChipGetOneRegister(Instance->IOHandle, R0372_DEMSTATUS);
  if(LockedStatus == 0x5A)
    {
  	*IsLocked =1;
    }
    else
    {
	*IsLocked =0;
     }
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0372
    STTBX_Print(("%s IsLocked=%u\n", identity, *IsLocked));
#endif
  Error = IOARCH_Handle[Instance->IOHandle].DeviceMap.Error;
  return(Error);
}




/* ----------------------------------------------------------------------------
Name: demod_d0372_ScanFrequency()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0372_ScanFrequency   (DEMOD_Handle_t Handle,
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
                                            U32  IQMode,
                                            U32  FreqOff,
                                            U32  ChannelBW,
                                            S32  EchoPos)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0372
   const char *identity = "STTUNER d0372.c demod_d0372_ScanFrequency()";
#endif
    ST_ErrorCode_t              Error = ST_NO_ERROR;
    FE_372_Error_t        error = FE_372_NO_ERROR;
    STTUNER_tuner_instance_t    *TunerInstance;
    STTUNER_InstanceDbase_t     *Inst;
    D0372_InstanceData_t        *Instance;
  /*  FE_360_InternalParams_t     *pInternalParams;*/
    FE_372_SearchParams_t       Search;
    FE_372_SearchResult_t       Result;
  
    /* private driver instance data */
    Instance = d0372_GetInstFromHandle(Handle);
    
    /* top level public instance data */
    Inst = STTUNER_GetDrvInst();

    /* get the tuner instance for this driver from the top level handle */
    TunerInstance = &Inst[Instance->TopLevelHandle].Terr.Tuner;

    Search.Frequency = InitialFrequency;
    Search.SymbolRate = SymbolRate;
    Search.SearchRange = 0; 
    Search.IQMode  = IQMode;
    Search.FistWatchdog = ST_GetClocksPerSecond() *5 ; /*get number of ticks per 10 seconds*/
    Search.DefaultWatchdog = ST_GetClocksPerSecond() ;/*get number of ticks per  seconds*/
    		
    Search.LOWPowerTh = 90;
    Search.UPPowerTh = 130;
   
    error=FE_372_Search(Instance->IOHandle, &Search, &Result,Instance->TopLevelHandle);
  
    if(error == FE_372_BAD_PARAMETER)
    {
       Error= ST_ERROR_BAD_PARAMETER;     
 	 #ifdef STTUNER_DEBUG_MODULE_TERDRV_D0372
           STTBX_Print(("%s fail, scan not done: bad parameter(s) to FE_372_Search() == FE_BAD_PARAMETER\n", identity ));
	 #endif      
       return Error;
    }
    
    else if (error == FE_372_SEARCH_FAILED)
    {
    	Error= ST_NO_ERROR;  
    	#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0372
           STTBX_Print(("%s  FE_372_Search() == FE_372_SEARCH_FAILED\n", identity ));
	#endif  
	return Error;
    	
    }
    
     *ScanSuccess = Result.Locked;
     if (*ScanSuccess == TRUE)
     {
     
     	*NewFrequency = Result.Frequency;
     	#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0372
        STTBX_Print(("%s NewFrequency=%u\t", identity, *NewFrequency));
        STTBX_Print(("%s ScanSuccess=%u\n",  identity, *ScanSuccess));
        #endif
     	/* return(ST_NO_ERROR);*/
     	 
     }
     	 return(ST_NO_ERROR);

   
}



/* ----------------------------------------------------------------------------
Name: demod_d0372_ioctl()

Description:
    access device specific low-level functions

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0372_ioctl(DEMOD_Handle_t Handle, U32 Function, void *InParams, void *OutParams, STTUNER_Da_Status_t *Status)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0372
   const char *identity = "STTUNER d0372.c demod_d0372_ioctl()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0372_InstanceData_t *Instance;

    /* private driver instance data */
    Instance = d0372_GetInstFromHandle(Handle);

    if(STTUNER_Util_CheckPtrNull(Instance) != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0372
        STTBX_Print(("%s invalid handle\n", identity));
#endif
        return(ST_ERROR_INVALID_HANDLE);
    }

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0372
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

        default:
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0372
            STTBX_Print(("%s function %d not found\n", identity, Function));
#endif
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }


#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0372
    STTBX_Print(("%s called ok\n", identity, Function));    /* signal that function came back */
#endif
 
    return(Error);

}



/* ----------------------------------------------------------------------------
Name: demod_d0372_repeateraccess()

Description:
    called from ioarch.c when some driver does I/O but with the repeater/passthru
    set to point to this function.
Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0372_repeateraccess(DEMOD_Handle_t Handle,BOOL REPEATER_STATUS)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0372_InstanceData_t *Instance;
    Instance = d0372_GetInstFromHandle(Handle);  
       if (REPEATER_STATUS)
       {
         Error = ChipSetField( Instance->IOHandle, F0372_I2CT_ON_1, 1);
       }
      

    return(Error);
}


/* ------------------------------------------------------------------------- */
/* /\/\/\/\/\/\/\/\/\/\/\/\/\UTILITY Functions/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ */
/* ------------------------------------------------------------------------- */

D0372_InstanceData_t *d0372_GetInstFromHandle(DEMOD_Handle_t Handle)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0372_HANDLES
   const char *identity = "STTUNER d0372.c d0372_GetInstFromHandle()";
#endif
    D0372_InstanceData_t *Instance;

    Instance = (D0372_InstanceData_t *)Handle;
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0372_HANDLES
    STTBX_Print(("%s block at 0x%08x\n", identity, Instance));
#endif

    return(Instance);
}




/* End of d0372.c */
