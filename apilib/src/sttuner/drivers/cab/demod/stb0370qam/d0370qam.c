/* ----------------------------------------------------------------------------
File Name: d0370qam.c 
Description:

    stb0370 QAM demod driver.

Copyright (C) 2005-2006 STMicroelectronics

   date: 
version: 
 author: 
comment: Write for multi-instance/multi-FrontEnd.

Reference:
    ST API Definition "TUNER Driver API" DVD-API-06
---------------------------------------------------------------------------- */


/* Includes ---------------------------------------------------------------- */
#ifndef ST_OSLINUX
/* C libs */
#include <string.h>


#endif
#include "stlite.h"     /* Standard includes */
#include "sttbx.h"
#include "stcommon.h"

/* STAPI */

#include "stevt.h"
#include "sttuner.h"

/* local to sttuner */
#include "util.h"       /* generic utility functions for sttuner */
#include "dbtypes.h"    /* data types for databases */
#include "sysdbase.h"   /* functions to accesss system data */
#include "ioarch.h"     /* I/O for this driver */
#include "ioreg.h"      /* I/O for this driver */

#include "d0370qam.h"     /* header for this file */
#include "reg0370qam.h"   /* register mappings for the stb0370qam */
#include "drv0370qam.h"   /* misc driver functions */


#include "cioctl.h"     /* data structure typedefs for all the the cable ioctl functions */


/* Private types/constants ------------------------------------------------ */

/* Device capabilities */
#define STB0370QAM_MAX_AGC                     1023
#define STB0370QAM_MAX_SIGNAL_QUALITY          100
#define STB0370QAM_MAX_SIGNAL_QUALITY_IN_DB    50 /*Considering maximum signal quality as 50dB*/

/* Device ID */
#define STB0370QAM_DEVICE_VERSION              0x10        
#define STB0370QAM_SYMBOLMIN                   870000;     /*  # 1  MegaSymbols/sec */
#define STB0370QAM_SYMBOLMAX                   11700000;   /*  # 11 MegaSymbols/sec */

/* private variables ------------------------------------------------------- */


static semaphore_t *Lock_InitTermOpenClose; /* guard calls to the functions */

static BOOL        Installed = FALSE;
/*New changed */
U16 STB0370_Address_256QAM_TD1336[STB0370_QAM_NBREGS]=
{
0xf000,
0xf001,
0xf002,
0xf003,
0xf004,
0xf005,
0xf006,
0xf007,
0xf640,
0xf641,
0xf610,
0xf611,
0xf612,
0xf613,
0xf614,
0xf615,
0xf616,
0xf617,
0xf618,
0xf619,
0xf61c,
0xf410,
0xf411,
0xf412,
0xf413,
0xf415,
0xf414,
0xf416,
0xf417,
0xf419,
0xf418,
0xf41a,
0xf41b,
0xf41c,
0xf41d,
0xf41f,
0xf41e,
0xf420,
0xf421,
0xf422,
0xf423,
0xf424,
0xf425,
0xf426,
0xf427,
0xf428,
0xf429,
0xf42a,
0xf42b,
0xf42c,
0xf430,
0xf42f,
0xf432,
0xf431,
0xf433,
0xf434,
0xf436,
0xf435,
0xf438,
0xf437,
0xf439,
0xf43a,
0xf43b,
0xf43c,
0xf440,
0xf43f,
0xf43e,
0xf43d,
0xf441,
0xf442,
0xf443,
0xf444,
0xf445,
0xf446,
0xf449,
0xf448,
0xf447,
0xf44d,
0xf44c,
0xf44b,
0xf44a,
0xf44f,
0xf44e,
0xf450,
0xf451,
0xf452,
0xf453,
0xf454,
0xf455,
0xf456,
0xf457,
0xf458,
0xf459,
0xf45a,
0xf45b,
0xf45c,
0xf45d,
0xf45e,
0xf45f,
0xf460,
0xf461,
0xf462,
0xf463,
0xf464,
0xf465,
0xf466,
0xf467,
0xf468,
0xf469,
0xf46c,
0xf46b,
0xf46a,
0xf46d,
0xf46e,
0xf46f,
0xf470,
0xf471,
0xf472,
0xf473,
0xf474,
0xf475,
0xf478,
0xf477,
0xf476,
0xf479,
0xf47a,
0xf47e,
0xf47f,
0xf480,
0xf481,
0xf482,
0xf485,
0xf484,
0xf483,
0xf491,
0xf492,
0xf493,
0xf495,
0xf496,
0xf497,
0xf498,
0xf499,
0xf49a,
0xf49b,
0xf49c,
0xf49d,
0xf49e,
0xf49f,
0xf4a0,
0xf4a1,
0xf4a2,
0xf4a3,
0xf4a4,
0xf4a5,
0xf4a6,
0xf4a7,
0xf4a8,
0xf4a9,
0xf4aa,
0xf4ac,
0xf4ad,
0xf4ae,
0xf4af
};

U8 STB0370_DefVal_256QAM_TD1336[STB0370_QAM_NBREGS]=
{
0x0010, /*REGISTER ID                    - 0xf000 */
0x0000,	/*REGISTER I2C_PAGE              - 0xf001 */
0x0022,	/*REGISTER I2CRPT1               - 0xf002 */
0x0002,	/*REGISTER I2CRPT2               - 0xf003 */
0x000a,	/*REGISTER CLK_CTRL              - 0xf004 */
0x00ca,	/*REGISTER STANDBY               - 0xf005 */
0x00c6,	/*REGISTER IO_CTRL               - 0xf006 */
0x0000,	/*REGISTER GPIO_INFO             - 0xf007 */
0x0000,	/*REGISTER AD_INTERF             - 0xf640 */
0x0000,	/*REGISTER TEST_AD_IF            - 0xf641 */
0x0087,	/*REGISTER PLL_CTRL              - 0xf610 */
0x0000,	/*REGISTER SEL_CLKAD8            - 0xf611 */
0x0000,	/*REGISTER NCO_PLL1              - 0xf612 */
0x0000,	/*REGISTER NCO_PLL2              - 0xf613 */
0x0000,	/*REGISTER NCO_TST_PLL           - 0xf614 */
0x0003,	/*REGISTER NCO_CTRL              - 0xf615 */
0x0000,	/*REGISTER NCO_SYNTH_COARSE_FREQ - 0xf616 */
0x0000,	/*REGISTER NCO_SYNTH_FINE_FREQ_1 - 0xf617 */
0x0000,	/*REGISTER NCO_SYNTH_FINE_FREQ_0 - 0xf618 */
0x0008,	/*REGISTER OOB_TUNER_CTRL        - 0xf619 */
0x0030,	/*REGISTER AD10_CTRL             - 0xf61c */ /*For cut 2.0*/
0x0039,	/*REGISTER EQU_0                 - 0xf410 */
0x0069,	/*REGISTER EQU_1                 - 0xf411 */
0x0020,	/*REGISTER EQU_2                 - 0xf412 */
0x0016,	/*REGISTER EQU_3                 - 0xf413 */
0x0000,	/*REGISTER EQU_5                 - 0xf415 */
0x003f,	/*REGISTER EQU_4                 - 0xf414 */
0x0000,	/*REGISTER EQU_6                 - 0xf416 */
0x0000,	/*REGISTER EQU_7                 - 0xf417 */
0x0012,	/*REGISTER EQU_9                 - 0xf419 */
0x004a,	/*REGISTER EQU_8                 - 0xf418 */
0x00db,	/*REGISTER EQU_10                - 0xf41a */
0x001c,	/*REGISTER EQU_11                - 0xf41b */
0x0098,	/*REGISTER INITDEM_0             - 0xf41c */
0x00d0,	/*REGISTER INITDEM_1             - 0xf41d */
0x0000,	/*REGISTER INITDEM_2             - 0xf41f */
0x0000,	/*REGISTER INITDEM_3             - 0xf41e */
0x0000,	/*REGISTER INITDEM_4             - 0xf420 */
0x0084,	/*REGISTER INITDEM_5             - 0xf421 */
0x0057,	/*REGISTER DELAGC_0              - 0xf422 */
0x0000,	/*REGISTER DELAGC_1              - 0xf423 */
0x00ff,	/*REGISTER DELAGC_2              - 0xf424 */
0x0000,	/*REGISTER DELAGC_3              - 0xf425 */
0x0039,	/*REGISTER DELAGC_4              - 0xf426 */
0x001f,	/*REGISTER DELAGC_5              - 0xf427 */
0x0080,	/*REGISTER DELAGC_6              - 0xf428 */
0x002e,	/*REGISTER DELAGC_7              - 0xf429 */
0x0038,	/*REGISTER DELAGC_8              - 0xf42a */
0x00ff,	/*REGISTER DELAGC_9              - 0xf42b */
0x0001,	/*REGISTER DELAGC_10             - 0xf42c */
0x0003,	/*REGISTER DELAGC_13             - 0xf430 */
0x0043,	/*REGISTER DELAGC_12             - 0xf42f */
0x0025,	/*REGISTER WBAGC_1               - 0xf432 */
0x0042,	/*REGISTER WBAGC_0               - 0xf431 */
0x001a,	/*REGISTER WBAGC_2               - 0xf433 */
0x0018,	/*REGISTER WBAGC_3               - 0xf434 */
0x006f,	/*REGISTER WBAGC_5               - 0xf436 */
0x0097,	/*REGISTER WBAGC_4               - 0xf435 */
0x0013,	/*REGISTER WBAGC_7               - 0xf438 */
0x0088,	/*REGISTER WBAGC_6               - 0xf437 */
0x0008,	/*REGISTER STLOOP_0              - 0xf439 */
0x0000,	/*REGISTER STLOOP_1              - 0xf43a */
0x0008,	/*REGISTER STLOOP_2              - 0xf43b */
0x0030,	/*REGISTER STLOOP_3              - 0xf43c */
0x0019,	/*REGISTER STLOOP_7              - 0xf440 */
0x006a,	/*REGISTER STLOOP_6              - 0xf43f */
0x0026,	/*REGISTER STLOOP_5              - 0xf43e */
0x004c,	/*REGISTER STLOOP_4              - 0xf43d */
0x005e,	/*REGISTER STLOOP_8              - 0xf441 */
0x0084,	/*REGISTER STLOOP_9              - 0xf442 */
0x00ca,	/*REGISTER CRL_0                 - 0xf443 */
0x008b,	/*REGISTER CRL_1                 - 0xf444 */
0x0002,	/*REGISTER CRL_2                 - 0xf445 */
0x0000,	/*REGISTER CRL_3                 - 0xf446 */
0x0016,	/*REGISTER CRL_7                 - 0xf449 */
0x008c,	/*REGISTER CRL_5                 - 0xf448 */
0x00d4,	/*REGISTER CRL_4                 - 0xf447 */
0x000f,	/*REGISTER CRL_11                - 0xf44d */
0x00f7,	/*REGISTER CRL_10                - 0xf44c */
0x0051,	/*REGISTER CRL_9                 - 0xf44b */
0x00c0,	/*REGISTER CRL_8                 - 0xf44a */
0x0000,	/*REGISTER CRL_13                - 0xf44f */
0x0000,	/*REGISTER CRL_12                - 0xf44e */
0x0007,	/*REGISTER CRL_14                - 0xf450 */
0x00ff,	/*REGISTER PMFAGC_0              - 0xf451 */
0x0004,	/*REGISTER PMFAGC_1              - 0xf452 */
0x0012,	/*REGISTER PMFAGC_2              - 0xf453 */
0x0019,	/*REGISTER PMFAGC_3              - 0xf454 */
0x002c,	/*REGISTER PMFAGC_4              - 0xf455 */
0x0005,	/*REGISTER PMFAGC_5              - 0xf456 */
0x001b,	/*REGISTER SIG_FAD_0             - 0xf457 */
0x0038,	/*REGISTER SIG_FAD_1             - 0xf458 */
0x001e,	/*REGISTER SIG_FAD_2             - 0xf459 */
0x0037,	/*REGISTER SIG_FAD_3             - 0xf45a */
0x0004,	/*REGISTER NEW_CRL_0             - 0xf45b */
0x000a,	/*REGISTER NEW_CRL_1             - 0xf45c */
0x0004,	/*REGISTER NEW_CRL_2             - 0xf45d */
0x0007,	/*REGISTER NEW_CRL_3             - 0xf45e */
0x0002,	/*REGISTER NEW_CRL_4             - 0xf45f */
0x0006,	/*REGISTER NEW_CRL_5             - 0xf460 */
0x0009,	/*REGISTER NEW_CRL_6             - 0xf461 */
0x00c1,	/*REGISTER FREQ_0                - 0xf462 */
0x007b,	/*REGISTER FREQ_1                - 0xf463 */
0x0005,	/*REGISTER FREQ_2                - 0xf464 */
0x0006,	/*REGISTER FREQ_3                - 0xf465 */
0x0008,	/*REGISTER FREQ_4                - 0xf466 */
0x00ea,	/*REGISTER FREQ_5                - 0xf467 */
0x0081,	/*REGISTER FREQ_6                - 0xf468 */
0x0098,	/*REGISTER FREQ_7                - 0xf469 */
0x0000,	/*REGISTER FREQ_10               - 0xf46c */
0x000c,	/*REGISTER FREQ_9                - 0xf46b */
0x0030,	/*REGISTER FREQ_8                - 0xf46a */
0x0022,	/*REGISTER FREQ_11               - 0xf46d */
0x00b4,	/*REGISTER FREQ_12               - 0xf46e */
0x0000,	/*REGISTER FREQ_13               - 0xf46f */
0x0000,	/*REGISTER FREQ_14               - 0xf470 */
0x00b3,	/*REGISTER FREQ_15               - 0xf471 */
0x00c8,	/*REGISTER FREQ_16               - 0xf472 */
0x0000,	/*REGISTER FREQ_17               - 0xf473 */
0x0000,	/*REGISTER FREQ_18               - 0xf474 */
0x0000,	/*REGISTER FREQ_19               - 0xf475 */
0x0000,	/*REGISTER FREQ_22               - 0xf478 */
0x0000,	/*REGISTER FREQ_21               - 0xf477 */
0x0005,	/*REGISTER FREQ_20               - 0xf476 */
0x0000,	/*REGISTER FREQ_23               - 0xf479 */
0x0000,	/*REGISTER FREQ_24               - 0xf47a */
0x0000,	/*REGISTER CTRL_0                - 0xf47e */
0x0000,	/*REGISTER CTRL_1                - 0xf47f */
0x0000,	/*REGISTER CTRL_2                - 0xf480 */
0x0000, /*REGISTER CTRL_3                - 0xf481 */
0x002f,	/*REGISTER CTRL_4                - 0xf482 */
0x0000,	/*REGISTER CTRL_7                - 0xf485 */
0x00a8,	/*REGISTER CTRL_6                - 0xf484 */
0x002c,	/*REGISTER CTRL_5                - 0xf483 */
0x001b,	/*REGISTER MPEG_CTRL             - 0xf491 */
0x000f,	/*REGISTER MPEG_SYNC_ACQ         - 0xf492 */
0x003f,	/*REGISTER MPEG_SYNC_LOSS        - 0xf493 */
0x000d,	/*REGISTER VIT_SYNC_ACQ          - 0xf495 */
0x00dc,	/*REGISTER VIT_SYNC_LOSS         - 0xf496 */
0x0008,	/*REGISTER VIT_SYNC_GO           - 0xf497 */
0x0008,	/*REGISTER VIT_SYNC_STOP         - 0xf498 */
0x00a8,	/*REGISTER FS_SYNC               - 0xf499 */
0x0081,	/*REGISTER IN_DEPTH              - 0xf49a */
0x00fb,	/*REGISTER RS_CTRL               - 0xf49b */
0x0047,	/*REGISTER DEINTER_CTRL          - 0xf49c */
0x001e,	/*REGISTER SYNC_STAT             - 0xf49d */
0x0002,	/*REGISTER VITERBI_I_RATE        - 0xf49e */
0x0002,	/*REGISTER VITERBI_Q_RATE        - 0xf49f */
0x00de,	/*REGISTER RS_CORR_CNT_LSB       - 0xf4a0 */
0x0002,	/*REGISTER RS_CORR_CNT_MSB       - 0xf4a1 */
0x00ff,	/*REGISTER RS_UNERR_CNT_LSB      - 0xf4a2 */
0x00ff,	/*REGISTER RS_UNERR_CNT_MSB      - 0xf4a3 */
0x0005,	/*REGISTER RS_UNC_CNT_LSB        - 0xf4a4 */
0x000a,	/*REGISTER RS_UNC_CNT_MSB        - 0xf4a5 */
0x0000,	/*REGISTER RS_RATE_LSB           - 0xf4a6 */
0x0000,	/*REGISTER RS_RATE_MSB           - 0xf4a7 */
0x0005,	/*REGISTER TX_IN_DEPTH           - 0xf4a8 */
0x0000,	/*REGISTER RS_ERR_CNT_LSB        - 0xf4a9 */
0x000f,	/*REGISTER RS_ERR_CNT_MSB        - 0xf4aa */
0x0080,	/*REGISTER OUT_FORMAT_0          - 0xf4ac */
0x0022,	/*REGISTER OUT_FORMAT_1          - 0xf4ad */
0x0000,	/*REGISTER OUT_FORMAT_2          - 0xf4ae */
0x000c	/*REGISTER INTERRUPT_STAT        - 0xf4af */
};

U8 STB0370_DefVal_256QAM_DCT7050[STB0370_QAM_NBREGS]=
{
0x0010,  /*REGISTER ID                    - 0xf000 */
0x0000,	 /*REGISTER I2C_PAGE              - 0xf001 */
0x0012,	 /*REGISTER I2CRPT1               - 0xf002 */
0x0002,	 /*REGISTER I2CRPT2               - 0xf003 */
0x000a,	 /*REGISTER CLK_CTRL              - 0xf004 */
0x00ca,	 /*REGISTER STANDBY               - 0xf005 */
0x00c6,	 /*REGISTER IO_CTRL               - 0xf006 */
0x0000,	 /*REGISTER GPIO_INFO             - 0xf007 */
0x0000,	 /*REGISTER AD_INTERF             - 0xf640 */
0x0000,	 /*REGISTER TEST_AD_IF            - 0xf641 */
0x0087,	 /*REGISTER PLL_CTRL              - 0xf610 */
0x0000,	 /*REGISTER SEL_CLKAD8            - 0xf611 */
0x0000,	 /*REGISTER NCO_PLL1              - 0xf612 */
0x0000,	 /*REGISTER NCO_PLL2              - 0xf613 */
0x0000,	 /*REGISTER NCO_TST_PLL           - 0xf614 */
0x0003,	 /*REGISTER NCO_CTRL              - 0xf615 */
0x0000,	 /*REGISTER NCO_SYNTH_COARSE_FREQ - 0xf616 */
0x0000,	 /*REGISTER NCO_SYNTH_FINE_FREQ_1 - 0xf617 */
0x0000,	 /*REGISTER NCO_SYNTH_FINE_FREQ_0 - 0xf618 */
0x0008,	 /*REGISTER OOB_TUNER_CTRL        - 0xf619 */
0x0030,	 /*REGISTER AD10_CTRL             - 0xf61c */ /*Changed for 370 cut 2.0*/
0x0039,	 /*REGISTER EQU_0                 - 0xf410 */
0x0069,	 /*REGISTER EQU_1                 - 0xf411 */
0x0020,	 /*REGISTER EQU_2                 - 0xf412 */
0x0016,	 /*REGISTER EQU_3                 - 0xf413 */
0x0000,	 /*REGISTER EQU_5                 - 0xf415 */
0x003f,	 /*REGISTER EQU_4                 - 0xf414 */
0x0000,	 /*REGISTER EQU_6                 - 0xf416 */
0x0000,	 /*REGISTER EQU_7                 - 0xf417 */
0x000e,	 /*REGISTER EQU_9                 - 0xf419 */
0x009e,	 /*REGISTER EQU_8                 - 0xf418 */
0x0013,	 /*REGISTER EQU_10                - 0xf41a */
0x00c3,	 /*REGISTER EQU_11                - 0xf41b */
0x0098,	 /*REGISTER INITDEM_0             - 0xf41c */
0x00d0,	 /*REGISTER INITDEM_1             - 0xf41d */
0x0000,	 /*REGISTER INITDEM_2             - 0xf41f */
0x0000,	 /*REGISTER INITDEM_3             - 0xf41e */
0x0000,	 /*REGISTER INITDEM_4             - 0xf420 */
0x0084,	 /*REGISTER INITDEM_5             - 0xf421 */
0x0057,	 /*REGISTER DELAGC_0              - 0xf422 */
0x0000,	 /*REGISTER DELAGC_1              - 0xf423 */
0x00ff,	 /*REGISTER DELAGC_2              - 0xf424 */
0x0000,	 /*REGISTER DELAGC_3              - 0xf425 */
0x0039,	 /*REGISTER DELAGC_4              - 0xf426 */
0x001f,	 /*REGISTER DELAGC_5              - 0xf427 */
0x0080,	 /*REGISTER DELAGC_6              - 0xf428 */
0x004e,	 /*REGISTER DELAGC_7              - 0xf429 */
0x0038,	 /*REGISTER DELAGC_8              - 0xf42a */
0x00c9,	 /*REGISTER DELAGC_9              - 0xf42b */
0x0000,	 /*REGISTER DELAGC_10             - 0xf42c */
0x0002,	 /*REGISTER DELAGC_13             - 0xf430 */
0x003d,	 /*REGISTER DELAGC_12             - 0xf42f */
0x0024,	 /*REGISTER WBAGC_1               - 0xf432 */
0x003d,	 /*REGISTER WBAGC_0               - 0xf431 */
0x001a,	 /*REGISTER WBAGC_2               - 0xf433 */
0x0018,	 /*REGISTER WBAGC_3               - 0xf434 */
0x00d0,	 /*REGISTER WBAGC_5               - 0xf436 */
0x00fb,	 /*REGISTER WBAGC_4               - 0xf435 */
0x0013,	 /*REGISTER WBAGC_7               - 0xf438 */
0x0088,	 /*REGISTER WBAGC_6               - 0xf437 */
0x0008,	 /*REGISTER STLOOP_0              - 0xf439 */
0x0000,	 /*REGISTER STLOOP_1              - 0xf43a */
0x0008,	 /*REGISTER STLOOP_2              - 0xf43b */
0x0030,	 /*REGISTER STLOOP_3              - 0xf43c */
0x0019,	 /*REGISTER STLOOP_7              - 0xf440 */
0x006a,	 /*REGISTER STLOOP_6              - 0xf43f */
0x00d4,	 /*REGISTER STLOOP_5              - 0xf43e */
0x0000,	 /*REGISTER STLOOP_4              - 0xf43d */
0x005e,	 /*REGISTER STLOOP_8              - 0xf441 */
0x0084,	 /*REGISTER STLOOP_9              - 0xf442 */
0x00ca,	 /*REGISTER CRL_0                 - 0xf443 */
0x008b,	 /*REGISTER CRL_1                 - 0xf444 */
0x0002,	 /*REGISTER CRL_2                 - 0xf445 */
0x0000,	 /*REGISTER CRL_3                 - 0xf446 */
0x0003,	 /*REGISTER CRL_7                 - 0xf449 */
0x0085,	 /*REGISTER CRL_5                 - 0xf448 */
0x0038,	 /*REGISTER CRL_4                 - 0xf447 */
0x0000,	 /*REGISTER CRL_11                - 0xf44d */
0x000a,	 /*REGISTER CRL_10                - 0xf44c */
0x0008,	 /*REGISTER CRL_9                 - 0xf44b */
0x0040,	 /*REGISTER CRL_8                 - 0xf44a */
0x0000,	 /*REGISTER CRL_13                - 0xf44f */
0x0000,	 /*REGISTER CRL_12                - 0xf44e */
0x0007,	 /*REGISTER CRL_14                - 0xf450 */
0x00ff,	 /*REGISTER PMFAGC_0              - 0xf451 */
0x0004,	 /*REGISTER PMFAGC_1              - 0xf452 */
0x0012,	 /*REGISTER PMFAGC_2              - 0xf453 */
0x009e,	 /*REGISTER PMFAGC_3              - 0xf454 */
0x0031,	 /*REGISTER PMFAGC_4              - 0xf455 */
0x0005,	 /*REGISTER PMFAGC_5              - 0xf456 */
0x001b,	 /*REGISTER SIG_FAD_0             - 0xf457 */
0x0039,	 /*REGISTER SIG_FAD_1             - 0xf458 */
0x001e,	 /*REGISTER SIG_FAD_2             - 0xf459 */
0x0037,	 /*REGISTER SIG_FAD_3             - 0xf45a */
0x0004,	 /*REGISTER NEW_CRL_0             - 0xf45b */
0x000a,	 /*REGISTER NEW_CRL_1             - 0xf45c */
0x0004,	 /*REGISTER NEW_CRL_2             - 0xf45d */
0x0007,	 /*REGISTER NEW_CRL_3             - 0xf45e */
0x0002,	 /*REGISTER NEW_CRL_4             - 0xf45f */
0x0006,	 /*REGISTER NEW_CRL_5             - 0xf460 */
0x0008,	 /*REGISTER NEW_CRL_6             - 0xf461 */
0x00c1,	 /*REGISTER FREQ_0                - 0xf462 */
0x007b,	 /*REGISTER FREQ_1                - 0xf463 */
0x0005,	 /*REGISTER FREQ_2                - 0xf464 */
0x0006,	 /*REGISTER FREQ_3                - 0xf465 */
0x0008,	 /*REGISTER FREQ_4                - 0xf466 */
0x00ea,	 /*REGISTER FREQ_5                - 0xf467 */
0x0081,	 /*REGISTER FREQ_6                - 0xf468 */
0x0098,	 /*REGISTER FREQ_7                - 0xf469 */
0x000c,	 /*REGISTER FREQ_10               - 0xf46c */
0x0072,	 /*REGISTER FREQ_9                - 0xf46b */
0x0080,	 /*REGISTER FREQ_8                - 0xf46a */
0x0022,	 /*REGISTER FREQ_11               - 0xf46d */
0x00b4,	 /*REGISTER FREQ_12               - 0xf46e */
0x0000,	 /*REGISTER FREQ_13               - 0xf46f */
0x0000,	 /*REGISTER FREQ_14               - 0xf470 */
0x00b3,	 /*REGISTER FREQ_15               - 0xf471 */
0x00c8,	 /*REGISTER FREQ_16               - 0xf472 */
0x0000,	 /*REGISTER FREQ_17               - 0xf473 */
0x0000,	 /*REGISTER FREQ_18               - 0xf474 */
0x0000,	 /*REGISTER FREQ_19               - 0xf475 */
0x0000,	 /*REGISTER FREQ_22               - 0xf478 */
0x0000,	 /*REGISTER FREQ_21               - 0xf477 */
0x004a,	 /*REGISTER FREQ_20               - 0xf476 */
0x0000,	 /*REGISTER FREQ_23               - 0xf479 */
0x0000,	 /*REGISTER FREQ_24               - 0xf47a */
0x0000,	 /*REGISTER CTRL_0                - 0xf47e */
0x0000,	 /*REGISTER CTRL_1                - 0xf47f */
0x0000,	 /*REGISTER CTRL_2                - 0xf480 */
0x0000,	 /*REGISTER CTRL_3                - 0xf481 */
0x002f,	 /*REGISTER CTRL_4                - 0xf482 */
0x000f,	 /*REGISTER CTRL_7                - 0xf485 */
0x0040,	 /*REGISTER CTRL_6                - 0xf484 */
0x0032,	 /*REGISTER CTRL_5                - 0xf483 */
0x001b,	 /*REGISTER MPEG_CTRL             - 0xf491 */
0x000f,	 /*REGISTER MPEG_SYNC_ACQ         - 0xf492 */
0x003f,	 /*REGISTER MPEG_SYNC_LOSS        - 0xf493 */
0x000d,	 /*REGISTER VIT_SYNC_ACQ          - 0xf495 */
0x00dc,	 /*REGISTER VIT_SYNC_LOSS         - 0xf496 */
0x0008,	 /*REGISTER VIT_SYNC_GO           - 0xf497 */
0x0008,	 /*REGISTER VIT_SYNC_STOP         - 0xf498 */
0x00a8,	 /*REGISTER FS_SYNC               - 0xf499 */
0x0081,	 /*REGISTER IN_DEPTH              - 0xf49a */
0x00fb,	 /*REGISTER RS_CTRL               - 0xf49b */
0x0047,	 /*REGISTER DEINTER_CTRL          - 0xf49c */
0x001e,	 /*REGISTER SYNC_STAT             - 0xf49d */
0x000e,	 /*REGISTER VITERBI_I_RATE        - 0xf49e */
0x000d,	 /*REGISTER VITERBI_Q_RATE        - 0xf49f */
0x00f7,	 /*REGISTER RS_CORR_CNT_LSB       - 0xf4a0 */
0x0005,	 /*REGISTER RS_CORR_CNT_MSB       - 0xf4a1 */
0x00ff,	 /*REGISTER RS_UNERR_CNT_LSB      - 0xf4a2 */
0x00ff,	 /*REGISTER RS_UNERR_CNT_MSB      - 0xf4a3 */
0x0000,	 /*REGISTER RS_UNC_CNT_LSB        - 0xf4a4 */
0x0005,	 /*REGISTER RS_UNC_CNT_MSB        - 0xf4a5 */
0x0000,	 /*REGISTER RS_RATE_LSB           - 0xf4a6 */
0x0003,	 /*REGISTER RS_RATE_MSB           - 0xf4a7 */
0x0001,	 /*REGISTER TX_IN_DEPTH           - 0xf4a8 */
0x0000,	 /*REGISTER RS_ERR_CNT_LSB        - 0xf4a9 */
0x000f,	 /*REGISTER RS_ERR_CNT_MSB        - 0xf4aa */
0x0080,	 /*REGISTER OUT_FORMAT_0          - 0xf4ac */
0x0022,	 /*REGISTER OUT_FORMAT_1          - 0xf4ad */
0x0000,	 /*REGISTER OUT_FORMAT_2          - 0xf4ae */
0x000c	 /*REGISTER INTERRUPT_STAT        - 0xf4af */
};


U8 STB0370_DefVal_256QAM_DTT7600[STB0370_QAM_NBREGS]=
{
0x0010,/*REGISTER ID                    - 0xf000 */
0x0000,/*REGISTER I2C_PAGE              - 0xf001 */
0x0032,/*REGISTER I2CRPT1               - 0xf002 */
0x0002,/*REGISTER I2CRPT2               - 0xf003 */
0x000a,/*REGISTER CLK_CTRL              - 0xf004 */
0x00ca,/*REGISTER STANDBY               - 0xf005 */
0x00c6,/*REGISTER IO_CTRL               - 0xf006 */
0x0018,/*REGISTER GPIO_INFO             - 0xf007 */
0x0000,/*REGISTER AD_INTERF             - 0xf640 */
0x0000,/*REGISTER TEST_AD_IF            - 0xf641 */
0x0087,/*REGISTER PLL_CTRL              - 0xf610 */
0x0000,/*REGISTER SEL_CLKAD8            - 0xf611 */
0x0000,/*REGISTER NCO_PLL1              - 0xf612 */
0x0000,/*REGISTER NCO_PLL2              - 0xf613 */
0x0000,/*REGISTER NCO_TST_PLL           - 0xf614 */
0x0003,/*REGISTER NCO_CTRL              - 0xf615 */
0x0000,/*REGISTER NCO_SYNTH_COARSE_FREQ - 0xf616 */
0x0000,/*REGISTER NCO_SYNTH_FINE_FREQ_1 - 0xf617 */
0x0000,/*REGISTER NCO_SYNTH_FINE_FREQ_0 - 0xf618 */
0x0008,/*REGISTER OOB_TUNER_CTRL        - 0xf619 */
0x0030,/*REGISTER AD10_CTRL             - 0xf61c */ /*For cut 2.0*/
0x0039,/*REGISTER EQU_0                 - 0xf410 */
0x0069,/*REGISTER EQU_1                 - 0xf411 */
0x0020,/*REGISTER EQU_2                 - 0xf412 */
0x0016,/*REGISTER EQU_3                 - 0xf413 */
0x0000,/*REGISTER EQU_5                 - 0xf415 */
0x0041,/*REGISTER EQU_4                 - 0xf414 */
0x0000,/*REGISTER EQU_6                 - 0xf416 */
0x0000,/*REGISTER EQU_7                 - 0xf417 */
0x0013,/*REGISTER EQU_9                 - 0xf419 */
0x00dc,/*REGISTER EQU_8                 - 0xf418 */
0x00d3,/*REGISTER EQU_10                - 0xf41a */
0x00e6,/*REGISTER EQU_11                - 0xf41b */
0x0098,/*REGISTER INITDEM_0             - 0xf41c */
0x00d0,/*REGISTER INITDEM_1             - 0xf41d */
0x0000,/*REGISTER INITDEM_2             - 0xf41f */
0x0000,/*REGISTER INITDEM_3             - 0xf41e */
0x0000,/*REGISTER INITDEM_4             - 0xf420 */
0x0084,/*REGISTER INITDEM_5             - 0xf421 */
0x0077,/*REGISTER DELAGC_0              - 0xf422 */
0x0000,/*REGISTER DELAGC_1              - 0xf423 */
0x00ff,/*REGISTER DELAGC_2              - 0xf424 */
0x0000,/*REGISTER DELAGC_3              - 0xf425 */
0x001a,/*REGISTER DELAGC_4              - 0xf426 */
0x002d,/*REGISTER DELAGC_5              - 0xf427 */
0x0080,/*REGISTER DELAGC_6              - 0xf428 */
0x001e,/*REGISTER DELAGC_7              - 0xf429 */
0x0036,/*REGISTER DELAGC_8              - 0xf42a */
0x00ff,/*REGISTER DELAGC_9              - 0xf42b */
0x0001,/*REGISTER DELAGC_10             - 0xf42c */
0x0003,/*REGISTER DELAGC_13             - 0xf430 */
0x00df,/*REGISTER DELAGC_12             - 0xf42f */
0x0027,/*REGISTER WBAGC_1               - 0xf432 */
0x00ff,/*REGISTER WBAGC_0               - 0xf431 */
0x001a,/*REGISTER WBAGC_2               - 0xf433 */
0x0018,/*REGISTER WBAGC_3               - 0xf434 */
0x003d,/*REGISTER WBAGC_5               - 0xf436 */
0x00c9,/*REGISTER WBAGC_4               - 0xf435 */
0x0001,/*REGISTER WBAGC_7               - 0xf438 */
0x0080,/*REGISTER WBAGC_6               - 0xf437 */
0x0008,/*REGISTER STLOOP_0              - 0xf439 */
0x0000,/*REGISTER STLOOP_1              - 0xf43a */
0x0008,/*REGISTER STLOOP_2              - 0xf43b */
0x0030,/*REGISTER STLOOP_3              - 0xf43c */
0x0019,/*REGISTER STLOOP_7              - 0xf440 */
0x0069,/*REGISTER STLOOP_6              - 0xf43f */
0x0023,/*REGISTER STLOOP_5              - 0xf43e */
0x0024,/*REGISTER STLOOP_4              - 0xf43d */
0x005e,/*REGISTER STLOOP_8              - 0xf441 */
0x0084,/*REGISTER STLOOP_9              - 0xf442 */
0x00ca,/*REGISTER CRL_0                 - 0xf443 */
0x008b,/*REGISTER CRL_1                 - 0xf444 */
0x0002,/*REGISTER CRL_2                 - 0xf445 */
0x0000,/*REGISTER CRL_3                 - 0xf446 */
0x004a,/*REGISTER CRL_7                 - 0xf449 */
0x0041,/*REGISTER CRL_5                 - 0xf448 */
0x00c0,/*REGISTER CRL_4                 - 0xf447 */
0x000f,/*REGISTER CRL_11                - 0xf44d */
0x00fd,/*REGISTER CRL_10                - 0xf44c */
0x0067,/*REGISTER CRL_9                 - 0xf44b */
0x0000,/*REGISTER CRL_8                 - 0xf44a */
0x0000,/*REGISTER CRL_13                - 0xf44f */
0x0000,/*REGISTER CRL_12                - 0xf44e */
0x0007,/*REGISTER CRL_14                - 0xf450 */
0x00ff,/*REGISTER PMFAGC_0              - 0xf451 */
0x0004,/*REGISTER PMFAGC_1              - 0xf452 */
0x0012,/*REGISTER PMFAGC_2              - 0xf453 */
0x00f2,/*REGISTER PMFAGC_3              - 0xf454 */
0x0034,/*REGISTER PMFAGC_4              - 0xf455 */
0x0005,/*REGISTER PMFAGC_5              - 0xf456 */
0x001b,/*REGISTER SIG_FAD_0             - 0xf457 */
0x0037,/*REGISTER SIG_FAD_1             - 0xf458 */
0x001e,/*REGISTER SIG_FAD_2             - 0xf459 */
0x0037,/*REGISTER SIG_FAD_3             - 0xf45a */
0x0004,/*REGISTER NEW_CRL_0             - 0xf45b */
0x000a,/*REGISTER NEW_CRL_1             - 0xf45c */
0x0004,/*REGISTER NEW_CRL_2             - 0xf45d */
0x0007,/*REGISTER NEW_CRL_3             - 0xf45e */
0x0002,/*REGISTER NEW_CRL_4             - 0xf45f */
0x0006,/*REGISTER NEW_CRL_5             - 0xf460 */
0x0009,/*REGISTER NEW_CRL_6             - 0xf461 */
0x00c1,/*REGISTER FREQ_0                - 0xf462 */
0x007b,/*REGISTER FREQ_1                - 0xf463 */
0x0005,/*REGISTER FREQ_2                - 0xf464 */
0x0006,/*REGISTER FREQ_3                - 0xf465 */
0x0008,/*REGISTER FREQ_4                - 0xf466 */
0x00ea,/*REGISTER FREQ_5                - 0xf467 */
0x0081,/*REGISTER FREQ_6                - 0xf468 */
0x0098,/*REGISTER FREQ_7                - 0xf469 */
0x00ff,/*REGISTER FREQ_10               - 0xf46c */
0x00d5,/*REGISTER FREQ_9                - 0xf46b */
0x0080,/*REGISTER FREQ_8                - 0xf46a */
0x0022,/*REGISTER FREQ_11               - 0xf46d */
0x00b4,/*REGISTER FREQ_12               - 0xf46e */
0x0000,/*REGISTER FREQ_13               - 0xf46f */
0x0000,/*REGISTER FREQ_14               - 0xf470 */
0x00b3,/*REGISTER FREQ_15               - 0xf471 */
0x00c8,/*REGISTER FREQ_16               - 0xf472 */
0x0000,/*REGISTER FREQ_17               - 0xf473 */
0x0000,/*REGISTER FREQ_18               - 0xf474 */
0x0000,/*REGISTER FREQ_19               - 0xf475 */
0x0000,/*REGISTER FREQ_22               - 0xf478 */
0x0000,/*REGISTER FREQ_21               - 0xf477 */
0x0005,/*REGISTER FREQ_20               - 0xf476 */
0x0000,/*REGISTER FREQ_23               - 0xf479 */
0x0000,/*REGISTER FREQ_24               - 0xf47a */
0x0000,/*REGISTER CTRL_0                - 0xf47e */
0x0000,/*REGISTER CTRL_1                - 0xf47f */
0x0000,/*REGISTER CTRL_2                - 0xf480 */
0x0000,/*REGISTER CTRL_3                - 0xf481 */
0x002f,/*REGISTER CTRL_4                - 0xf482 */
0x0001,/*REGISTER CTRL_7                - 0xf485 */
0x00e0,/*REGISTER CTRL_6                - 0xf484 */
0x00f2,/*REGISTER CTRL_5                - 0xf483 */
0x001b,/*REGISTER MPEG_CTRL             - 0xf491 */
0x000f,/*REGISTER MPEG_SYNC_ACQ         - 0xf492 */
0x003f,/*REGISTER MPEG_SYNC_LOSS        - 0xf493 */
0x000d,/*REGISTER VIT_SYNC_ACQ          - 0xf495 */
0x00dc,/*REGISTER VIT_SYNC_LOSS         - 0xf496 */
0x0008,/*REGISTER VIT_SYNC_GO           - 0xf497 */
0x0008,/*REGISTER VIT_SYNC_STOP         - 0xf498 */
0x00a8,/*REGISTER FS_SYNC               - 0xf499 */
0x0081,/*REGISTER IN_DEPTH              - 0xf49a */
0x00fb,/*REGISTER RS_CTRL               - 0xf49b */
0x0047,/*REGISTER DEINTER_CTRL          - 0xf49c */
0x001e,/*REGISTER SYNC_STAT             - 0xf49d */
0x0007,/*REGISTER VITERBI_I_RATE        - 0xf49e */
0x0005,/*REGISTER VITERBI_Q_RATE        - 0xf49f */
0x003b,/*REGISTER RS_CORR_CNT_LSB       - 0xf4a0 */
0x0001,/*REGISTER RS_CORR_CNT_MSB       - 0xf4a1 */
0x00ff,/*REGISTER RS_UNERR_CNT_LSB      - 0xf4a2 */
0x003f,/*REGISTER RS_UNERR_CNT_MSB      - 0xf4a3 */
0x009d,/*REGISTER RS_UNC_CNT_LSB        - 0xf4a4 */
0x0007,/*REGISTER RS_UNC_CNT_MSB        - 0xf4a5 */
0x0000,/*REGISTER RS_RATE_LSB           - 0xf4a6 */
0x0003,/*REGISTER RS_RATE_MSB           - 0xf4a7 */
0x0001,/*REGISTER TX_IN_DEPTH           - 0xf4a8 */
0x0000,/*REGISTER RS_ERR_CNT_LSB        - 0xf4a9 */
0x000f,/*REGISTER RS_ERR_CNT_MSB        - 0xf4aa */
0x0080,/*REGISTER OUT_FORMAT_0          - 0xf4ac */
0x0022,/*REGISTER OUT_FORMAT_1          - 0xf4ad */
0x0000,/*REGISTER OUT_FORMAT_2          - 0xf4ae */
0x000c/*REGISTER INTERRUPT_STAT        - 0xf4af */
};



/* instance chain, the default boot value is invalid, to catch errors */
static D0370QAM_InstanceData_t *InstanceChainTop = (D0370QAM_InstanceData_t *)0x7fffffff;

/* functions --------------------------------------------------------------- */

/* API */
ST_ErrorCode_t demod_d0370QAM_Init(ST_DeviceName_t *DeviceName, DEMOD_InitParams_t *InitParams);
ST_ErrorCode_t demod_d0370QAM_Term(ST_DeviceName_t *DeviceName, DEMOD_TermParams_t *TermParams);

ST_ErrorCode_t demod_d0370QAM_Open (ST_DeviceName_t *DeviceName, DEMOD_OpenParams_t  *OpenParams, DEMOD_Capability_t *Capability, DEMOD_Handle_t *Handle);
ST_ErrorCode_t demod_d0370QAM_Close(DEMOD_Handle_t  Handle, DEMOD_CloseParams_t *CloseParams);


ST_ErrorCode_t demod_d0370QAM_IsAnalogCarrier (DEMOD_Handle_t Handle, BOOL *IsAnalog);
ST_ErrorCode_t demod_d0370QAM_GetSignalQuality(DEMOD_Handle_t Handle, U32  *SignalQuality_p, U32 *Ber_p);
ST_ErrorCode_t demod_d0370QAM_GetModulation   (DEMOD_Handle_t Handle, STTUNER_Modulation_t *Modulation);
ST_ErrorCode_t demod_d0370QAM_GetAGC          (DEMOD_Handle_t Handle, S16                  *Agc);
ST_ErrorCode_t demod_d0370QAM_GetFECRates     (DEMOD_Handle_t Handle, STTUNER_FECRate_t    *FECRates);
ST_ErrorCode_t demod_d0370QAM_IsLocked        (DEMOD_Handle_t Handle, BOOL                 *IsLocked);
ST_ErrorCode_t demod_d0370QAM_SetFECRates     (DEMOD_Handle_t Handle, STTUNER_FECRate_t     FECRates);
ST_ErrorCode_t demod_d0370QAM_Tracking        (DEMOD_Handle_t Handle, BOOL ForceTracking,   U32 *NewFrequency, BOOL *SignalFound);
ST_ErrorCode_t demod_d0370QAM_StandByMode(DEMOD_Handle_t Handle, STTUNER_StandByMode_t PowerMode);

ST_ErrorCode_t demod_d0370QAM_ScanFrequency   (DEMOD_Handle_t Handle, U32  InitialFrequency, U32   SymbolRate,    U32   MaxOffset,
                                                                   U32  TunerStep,        U8    DerotatorStep, BOOL *ScanSuccess,
                                                                   U32 *NewFrequency,     U32   Mode,          U32   Guard,
                                                                   U32  Force,            U32   Hierarchy,     U32   Spectrum,
                                                                   U32  FreqOff,          U32   ChannelBW,     S32   EchoPos);
/* access device specific low-level functions */
ST_ErrorCode_t demod_d0370QAM_ioctl           (DEMOD_Handle_t Handle, U32 Function, void *InParams, void *OutParams,
                                             STTUNER_Da_Status_t *Status);

/* repeater/passthrough port for other drivers to use, type: STTUNER_IOARCH_RedirFn_t */
ST_ErrorCode_t demod_d0370QAM_ioaccess         (DEMOD_Handle_t Handle, IOARCH_Handle_t IOHandle,
                                             STTUNER_IOARCH_Operation_t Operation, U16 SubAddr,
                                             U8 *Data, U32 TransferSize, U32 Timeout);

/* local functions --------------------------------------------------------- */

D0370QAM_InstanceData_t *D0370QAM_GetInstFromHandle(DEMOD_Handle_t Handle);

/* ----------------------------------------------------------------------------
Name: STTUNER_DRV_DEMOD_STB0370QAM_Install()

Description:
    install a cable device driver into the demod database.

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_DRV_DEMOD_STB0370QAM_Install(STTUNER_demod_dbase_t *Demod)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
   const char *identity = "STTUNER d0370QAM.c STTUNER_DRV_DEMOD_STB0370QAM_Install()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if(Installed == TRUE)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
        STTBX_Print(("%s fail driver already installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
    STTBX_Print(("%s installing cable:demod:STB0370VSB...", identity));
#endif

    /* mark ID in database */
    Demod->ID = STTUNER_DEMOD_STB0370QAM;

    /* map API */
    Demod->demod_Init = demod_d0370QAM_Init;
    Demod->demod_Term = demod_d0370QAM_Term;

    Demod->demod_Open  = demod_d0370QAM_Open;
    Demod->demod_Close = demod_d0370QAM_Close;

    Demod->demod_IsAnalogCarrier  = demod_d0370QAM_IsAnalogCarrier;
    Demod->demod_GetSignalQuality = demod_d0370QAM_GetSignalQuality;
    Demod->demod_GetModulation    = demod_d0370QAM_GetModulation;
    Demod->demod_GetAGC           = demod_d0370QAM_GetAGC;
    Demod->demod_GetFECRates      = demod_d0370QAM_GetFECRates;
    Demod->demod_IsLocked         = demod_d0370QAM_IsLocked ;
    Demod->demod_SetFECRates      = demod_d0370QAM_SetFECRates;
    Demod->demod_Tracking         = demod_d0370QAM_Tracking;
    Demod->demod_ScanFrequency    = demod_d0370QAM_ScanFrequency;
    Demod->demod_StandByMode      = demod_d0370QAM_StandByMode;
    Demod->demod_ioaccess = demod_d0370QAM_ioaccess;
    Demod->demod_ioctl    = demod_d0370QAM_ioctl;

    InstanceChainTop = NULL;

    Lock_InitTermOpenClose = STOS_SemaphoreCreateFifo(NULL,1);


    Installed = TRUE;

#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
    STTBX_Print(("ok\n"));
#endif

    return(Error);
}



/* ----------------------------------------------------------------------------
Name: STTUNER_DRV_DEMOD_STB0370QAM_UnInstall()

Description:
    install a cable device driver into the demod database.

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_DRV_DEMOD_STB0370QAM_UnInstall(STTUNER_demod_dbase_t *Demod)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
   const char *identity = "STTUNER d0370QAM.c STTUNER_DRV_DEMOD_STB0370QAM_UnInstall()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

    if(Demod->ID != STTUNER_DEMOD_STB0370QAM)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
        STTBX_Print(("%s fail incorrect driver type\n", identity));
#endif
        return(STTUNER_ERROR_ID);
    }

    /* has all memory been freed, by Term() */
    if(InstanceChainTop != NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
        STTBX_Print(("%s fail at least one instance not terminated\n", identity));
#endif
        return(ST_ERROR_OPEN_HANDLE);
    }


#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
    STTBX_Print(("%s uninstalling cable:demod:STB0370QAM...", identity));
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
    Demod->demod_GetAGC           = NULL;
    Demod->demod_GetFECRates      = NULL;
    Demod->demod_IsLocked         = NULL;
    Demod->demod_SetFECRates      = NULL;
    Demod->demod_Tracking         = NULL;
    Demod->demod_ScanFrequency    = NULL;
    Demod->demod_StandByMode      = NULL;
    Demod->demod_ioaccess = NULL;
    Demod->demod_ioctl    = NULL;

#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
        STTBX_Print(("<"));
#endif

        STOS_SemaphoreDelete(NULL,Lock_InitTermOpenClose);

#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
        STTBX_Print((">"));
#endif

    InstanceChainTop = (D0370QAM_InstanceData_t *)0x7ffffffe;
    Installed        = FALSE;

#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
        STTBX_Print(("ok\n"));
#endif

    return(Error);
}



/* ------------------------------------------------------------------------- */
/* /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ API /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ */
/* ------------------------------------------------------------------------- */



/* ----------------------------------------------------------------------------
Name: demod_d0370QAM_Init()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0370QAM_Init(ST_DeviceName_t *DeviceName, DEMOD_InitParams_t *InitParams)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
    const char *identity = "STTUNER d0370QAM.c demod_d0370QAM_Init()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0370QAM_InstanceData_t *InstanceNew, *Instance;

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
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
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
        STTBX_Print(("%s fail MemoryPartition not valid\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    Error = STTUNER_Util_CheckPtrNull(DeviceName);
    if( Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
        STTBX_Print(("%s fail DeviceName not valid\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    InstanceNew = STOS_MemoryAllocateClear(InitParams->MemoryPartition, 1, sizeof( D0370QAM_InstanceData_t ));
    if (InstanceNew == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
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
    InstanceNew->DeviceMap.Timeout   = IOREG_DEFAULT_TIMEOUT;
    InstanceNew->DeviceMap.Registers = STB0370_QAM_NBREGS;
    InstanceNew->DeviceMap.Fields    = STB0370_QAM_NBFIELDS;
    InstanceNew->DeviceMap.Mode      = IOREG_MODE_SUBADR_16;
    InstanceNew->DeviceMap.MemoryPartition = InitParams->MemoryPartition;
    InstanceNew->InstanceChainNext   = NULL; /* always last in the chain */

    InstanceNew->ExternalClock     = (InitParams->ExternalClock)/1000;          /* Unit KHz */
    InstanceNew->TSOutputMode      = InitParams->TSOutputMode;
    InstanceNew->SerialDataMode    = InitParams->SerialDataMode;
    InstanceNew->SerialClockSource = InitParams->SerialClockSource;
    InstanceNew->FECMode           = InitParams->FECMode;
    InstanceNew->Sti5518           = InitParams->Sti5518;
	
    InstanceNew->DeviceMap.DefVal= (U32*)&STB0370_DefVal_256QAM_TD1336[0];
    /*Used in C/N calculation*/
    InstanceNew->Driv0370QAMCNEstimation = 3000;
    InstanceNew->Driv0370QAMCNEstimatorOffset =0;

    /* reserve memory for register mapping */
    Error = STTUNER_IOREG_Open(&InstanceNew->DeviceMap);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
        STTBX_Print(("%s fail setup new register database\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
    STTBX_Print(("%s allocated & initalized block named '%s' at 0x%08x (%d bytes)\n", identity, InstanceNew->DeviceName, (U32)InstanceNew, sizeof( D0370QAM_InstanceData_t ) ));
#endif
    SEM_UNLOCK(Lock_InitTermOpenClose);
    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0370QAM_Term()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0370QAM_Term(ST_DeviceName_t *DeviceName, DEMOD_TermParams_t *TermParams)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
   const char *identity = "STTUNER d0370QAM.c demod_d0370QAM_Term()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0370QAM_InstanceData_t *Instance, *InstancePrev, *InstanceNext;

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
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
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
        STTBX_Print(("%s fail TermParams not valid\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    /* ---------- check that at least one init has taken place ---------- */
    if(InstanceChainTop == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
        STTBX_Print(("%s fail nothing initalized\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(STTUNER_ERROR_INITSTATE);
    }


    /* reap next matching DeviceName */
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
    STTBX_Print(("%s looking for first free handle[\n", identity));
#endif

    Instance = InstanceChainTop;
    while(1)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
        STTBX_Print(("(%s)", Instance->DeviceName));
#endif
        if ( strcmp( (char *)Instance->DeviceName, (char *)DeviceName) == 0)
        {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
            STTBX_Print(("]\n"));
#endif
            Error = STTUNER_IOREG_Close(&Instance->DeviceMap);
            if (Error != ST_NO_ERROR)
            {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
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
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
            STTBX_Print(("%s freed block at %0x%08x\n", identity, (U32)Instance ));
#endif

#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
            STTBX_Print(("%s terminated ok\n", identity));
#endif
            SEM_UNLOCK(Lock_InitTermOpenClose);
            return(Error);
        }
        else if(Instance->InstanceChainNext == NULL)
        {       /* error we should have found a matching name before the end of the list */
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
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


#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
    STTBX_Print(("%s FAIL! this point should NEVER be reached\n", identity));
#endif
    SEM_UNLOCK(Lock_InitTermOpenClose);
    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0370QAM_Open()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0370QAM_Open(ST_DeviceName_t *DeviceName, DEMOD_OpenParams_t  *OpenParams, DEMOD_Capability_t *Capability, DEMOD_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
   const char *identity = "STTUNER d0370QAM.c demod_d0370QAM_Open()";
#endif
    ST_ErrorCode_t          Error = ST_NO_ERROR;
    U8                      Version;
    D0370QAM_InstanceData_t    *Instance;
    STTUNER_InstanceDbase_t *Inst;
    STTUNER_tuner_instance_t *TunerInstance;


    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

    /* now safe to lock semaphore */
    SEM_LOCK(Lock_InitTermOpenClose);

    /* ---------- check that at least one init has taken place ---------- */
    if(InstanceChainTop == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
        STTBX_Print(("%s fail nothing initalized\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(STTUNER_ERROR_INITSTATE);
    }

    /* find  handle from name */
    Instance = InstanceChainTop;
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
    STTBX_Print(("%s looking (%s)", identity, Instance->DeviceName));
#endif
    while(strcmp((char *)Instance->DeviceName, (char *)DeviceName) != 0)
    {
        /* error, should have found matching DeviceName before end of list */
        if(Instance->InstanceChainNext == NULL)
        {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
            STTBX_Print(("\n%s fail no block named '%s' before end of list\n", identity, DeviceName));
#endif
            SEM_UNLOCK(Lock_InitTermOpenClose);
            return(STTUNER_ERROR_INITSTATE);
        }
        Instance = Instance->InstanceChainNext;   /* next block */
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
        STTBX_Print(("(%s)", Instance->DeviceName));
#endif
    }
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
        STTBX_Print((" found ok\n"));
#endif

#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
    STTBX_Print(("%s using block at 0x%08x\n", identity, (U32)Instance));
#endif

    /* check handle IS actually free */
    if(Instance->TopLevelHandle != STTUNER_MAX_HANDLES)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
        STTBX_Print(("%s fail driver instance already open\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_OPEN_HANDLE);
    }

    /* now got pointer to free (and valid) data block */

    Inst = STTUNER_GetDrvInst();    /* pointer to instance database */


    /*Assign the default register array according to the tuner type*/
     TunerInstance = &Inst[OpenParams->TopLevelHandle].Cable.Tuner; /* pointer to tuner for this instance */
     switch(TunerInstance->Driver->ID)
     {
     	case STTUNER_TUNER_TD1336:     	
     	    Instance->DeviceMap.DefVal= (U32*)&STB0370_DefVal_256QAM_TD1336[0];
     	   break;
     	case STTUNER_TUNER_DCT7050:  
     	   Instance->DeviceMap.DefVal= (U32*)&STB0370_DefVal_256QAM_DCT7050[0]; 
     	  break;
     	  case STTUNER_TUNER_DTT7600:  
     	     Instance->DeviceMap.DefVal= (U32*)&STB0370_DefVal_256QAM_DTT7600[0]; 

     	  break;
     	  default:
     	  /*Do nothing. In this case default value is STB0370_DefVal_256QAM_TD1336, as set in Init()*/
     	  break;
     }
    /*
    --- wake up chip if in standby, Reset
    */
    /*  soft reset of the whole chip */
    
    Error = STTUNER_IOREG_SetRegister( &Instance->DeviceMap, Instance->IOHandle, R0370QAM_CTRL_0, 1);/* check the what will be the actual  register ??*/
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
        STTBX_Print(("%s fail SOFT_RESET\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }
    Error = STTUNER_IOREG_SetRegister( &Instance->DeviceMap, Instance->IOHandle, R0370QAM_CTRL_0, 0); /* check the what will be the actual  register  ??*/
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
        STTBX_Print(("%s fail SOFT_RESET\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    /*
    --- Get chip ID
    */
    Version = Reg0370QAM_GetSTB0370QAMId(&Instance->DeviceMap, Instance->IOHandle);
    if ((Version & STB0370QAM_DEVICE_VERSION) !=  STB0370QAM_DEVICE_VERSION)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
        STTBX_Print(("%s fail device not found (0x%02x)\n", identity, Version));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_UNKNOWN_DEVICE);
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
        STTBX_Print(("%s device found, release/revision=%u\n", identity, Version));
#endif
    }

    /* reset all chip registers */
    Error = STTUNER_IOREG_Reset(&Instance->DeviceMap, Instance->IOHandle,(U8*)Instance->DeviceMap.DefVal,STB0370_Address_256QAM_TD1336);
  
     if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
        STTBX_Print(("%s fail reset device\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    /* Set capabilties */
    Capability->FECAvail        = 0;
    Capability->ModulationAvail = STTUNER_MOD_QAM    |
                                  STTUNER_MOD_16QAM  |
                                  STTUNER_MOD_32QAM  |
                                  STTUNER_MOD_64QAM  |
                                  STTUNER_MOD_128QAM |
                                  STTUNER_MOD_256QAM;   /* direct mapping to STTUNER_Modulation_t */

    Capability->J83Avail        =   STTUNER_J83_B;


    Capability->AGCControl      = FALSE;
    Capability->SymbolMin       = STB0370QAM_SYMBOLMIN;        /*   1 MegaSymbols/sec (e.g) */
    Capability->SymbolMax       = STB0370QAM_SYMBOLMAX;        /*  11 MegaSymbols/sec (e.g) */

    Capability->BerMax           = 0;
    Capability->SignalQualityMax = STB0370QAM_MAX_SIGNAL_QUALITY;
    Capability->AgcMax           = STB0370QAM_MAX_AGC;

    /* Setup stateblock */
    Instance->StateBlock.ScanMode     = DEF0370QAM_CHANNEL;
    Instance->StateBlock.lastAGC2Coef = 1;
    Instance->StateBlock.Ber[0] = 0;
    Instance->StateBlock.Ber[1] = 50;                       /* Default Ratio for BER */
    Instance->StateBlock.Ber[2] = 0;
    Instance->StateBlock.CNdB = 0;
    Instance->StateBlock.SignalQuality = 0;

/*Check the Output formatting accordingly*/
    /* TS output mode */
    switch (Instance->TSOutputMode)
    {
         case STTUNER_TS_MODE_SERIAL:
           STTUNER_IOREG_SetField(&Instance->DeviceMap, Instance->IOHandle,F0370QAM_OUT_FORMAT,0x01);
            break;

        case STTUNER_TS_MODE_DVBCI:
          STTUNER_IOREG_SetField(&Instance->DeviceMap, Instance->IOHandle,F0370QAM_OUT_FORMAT,0x02);
            break;

        case STTUNER_TS_MODE_PARALLEL:
        case STTUNER_TS_MODE_DEFAULT:
          STTUNER_IOREG_SetField(&Instance->DeviceMap, Instance->IOHandle,F0370QAM_OUT_FORMAT,0x00);

        default:
            break;
    }


    /* Set default error count mode, bit error rate */

    /* finally (as nor more errors to check for, allocate handle */
    Instance->TopLevelHandle = OpenParams->TopLevelHandle;
    *Handle = (U32)Instance;

#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
    STTBX_Print(("%s opened ok\n", identity));
#endif

    SEM_UNLOCK(Lock_InitTermOpenClose);
    Error |= Instance->DeviceMap.Error;

    Instance->DeviceMap.Error = ST_NO_ERROR;

    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0370QAM_Close()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0370QAM_Close(DEMOD_Handle_t Handle, DEMOD_CloseParams_t *CloseParams)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
   const char *identity = "STTUNER d0370QAM.c demod_d0370QAM_Close()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0370QAM_InstanceData_t     *Instance;

    /* private driver instance data */
    Instance = D0370QAM_GetInstFromHandle(Handle);

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

    SEM_LOCK(Lock_InitTermOpenClose);

    /* ---------- check that at least one init has taken place ---------- */
    if(InstanceChainTop == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
        STTBX_Print(("%s fail nothing initalized\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(STTUNER_ERROR_INITSTATE);
    }

    if(Instance->TopLevelHandle == STTUNER_MAX_HANDLES)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
        STTBX_Print(("%s fail driver instance not open\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_OPEN_HANDLE);
    }

    Instance->TopLevelHandle = STTUNER_MAX_HANDLES;

#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
    STTBX_Print(("%s closed\n", identity));
#endif

    SEM_UNLOCK(Lock_InitTermOpenClose);
    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0370QAM_StandByMode()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */



ST_ErrorCode_t demod_d0370QAM_StandByMode(DEMOD_Handle_t Handle, STTUNER_StandByMode_t PowerMode)
{
	
#ifdef STTUNER_DEBUG_MODULE_SATDRV_STB0899
   const char *identity = "STTUNER d0370QAM.c demod_d0370QAM_StandByMode()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0370QAM_InstanceData_t *Instance;
   
    /* private driver instance data */
    Instance = D0370QAM_GetInstFromHandle(Handle);
   
      
    switch ( PowerMode)
    {
       case STTUNER_NORMAL_POWER_MODE :
       if(Instance->StandBy_Flag == 1)
       {
          Error|=STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle, F0370QAM_STANDBY_TUNER, 0);
          Error|=STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle, F0370QAM_STANDBY_NCO, 0);
          Error|=STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle, F0370QAM_STANDBY_AD10, 0); 
          Error|=STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle, F0370QAM_STANDBY_QAM, 0);
       }
          
             
       if(Error==ST_NO_ERROR)
       {
       	 
          Instance->StandBy_Flag = 0 ;
       }
       
       break;
       case STTUNER_STANDBY_POWER_MODE :
       if(Instance->StandBy_Flag == 0)
       {
          Error|=STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle, F0370QAM_STANDBY_TUNER, 1);
          Error|=STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle, F0370QAM_STANDBY_NCO, 1);
          Error|=STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle, F0370QAM_STANDBY_AD10, 1); 
          Error|=STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle, F0370QAM_STANDBY_QAM, 1);
          if(Error==ST_NO_ERROR)
          {
                  
             Instance->StandBy_Flag = 1 ;
             
          }
       }
       break ;
	   /* Switch statement */ 
  
    }
       
 

    /*Resolve Bug GNBvd17801 - For proper I2C error handling inside the driver*/
   
    return(Error);
}


/* ----------------------------------------------------------------------------
Name: demod_d0370QAM_IsAnalogCarrier()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0370QAM_IsAnalogCarrier(DEMOD_Handle_t Handle, BOOL *IsAnalog)
{
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
    return(Error);
}


/* ----------------------------------------------------------------------------
Name: demod_d0370QAM_GetSignalQuality()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0370QAM_GetSignalQuality(DEMOD_Handle_t Handle, U32  *SignalQuality_p, U32 *Ber_p)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
   const char *identity = "STTUNER d0370QAM.c demod_d0370QAM_GetSignalQuality()";
#endif
    ST_ErrorCode_t              Error = ST_NO_ERROR;
    STTUNER_tuner_instance_t    *TunerInstance;
    STTUNER_InstanceDbase_t     *Inst;
    D0370QAM_InstanceData_t        *Instance;
    int                         Mean, CNdB100;
    int                         ApplicationBERCount;
    BOOL                        IsLocked;

    /*
    --- Set parameters
    */
    *SignalQuality_p = 0;
    *Ber_p = 0;
    Mean = 0;
    CNdB100 = 0;
    ApplicationBERCount = 0;    /* Bit Error counter */


    /* private driver instance data */
    Instance = D0370QAM_GetInstFromHandle(Handle);
    /* top level public instance data */
    Inst = STTUNER_GetDrvInst();
    /* get the tuner instance for this driver from the top level handle */
    TunerInstance = &Inst[Instance->TopLevelHandle].Cable.Tuner;

    /*
    --- Get Carrier And Data (TS) status
    */
    Error = demod_d0370QAM_IsLocked(Handle, &IsLocked);
    if ( Error == ST_NO_ERROR && IsLocked )
    {
 
      
        /*
        --- Read noise estimations and BER
        */
       
        Driv0370QAMBertCount(&Instance->DeviceMap, Instance->IOHandle, &Instance->StateBlock,
                            TunerInstance, &ApplicationBERCount, Instance->StateBlock.Ber);
        Driv0370QAMCNEstimator(Handle,&Instance->DeviceMap, Instance->IOHandle, &Mean, &CNdB100, &Instance->StateBlock.SignalQuality);

       
        /*
        --- Set results
        */
        /*calculate signal quality in % considering 50dB as theoretical maximum*/
        *SignalQuality_p = Instance->StateBlock.SignalQuality /STB0370QAM_MAX_SIGNAL_QUALITY_IN_DB;
       if ( Instance->StateBlock.Ber[2] == 0 && ApplicationBERCount != 0 )
        {        
            *Ber_p = ApplicationBERCount;           /* Set BER to counter (no time to count) */
        }
        else
        {        
            *Ber_p = Instance->StateBlock.Ber[2];
        }
    }

#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
    STTBX_Print(("%s Mean=%d CN=%d dBx100 (%d %%)\n", identity, Mean, CNdB100, Instance->StateBlock.SignalQuality));
    STTBX_Print(("%s BERCount=%d BER[0..2] (cnt %d sat %d rate %d 10E-6)\n", identity,
                ApplicationBERCount,
                Instance->StateBlock.Ber[0],
                Instance->StateBlock.Ber[1],
                Instance->StateBlock.Ber[2]));
    STTBX_Print(("%s SignalQuality %u %% Ber %u 10E-6\n", identity, *SignalQuality_p, *Ber_p));
    STTBX_Print(("%s BlkCounter %9d CorrBlk %9d UncorrBlk %9d\n", identity,
                Instance->StateBlock.BlkCounter,
                Instance->StateBlock.CorrBlk,
                Instance->StateBlock.UncorrBlk
                ));
#endif

    Error |= Instance->DeviceMap.Error;
    Instance->DeviceMap.Error = ST_NO_ERROR;

    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0370QAM_GetModulation()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0370QAM_GetModulation(DEMOD_Handle_t Handle, STTUNER_Modulation_t *Modulation)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
   const char *identity = "STTUNER d0370QAM.c demod_d0370QAM_GetModulation()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0370QAM_InstanceData_t *Instance;

    /* private driver instance data */
    Instance = D0370QAM_GetInstFromHandle(Handle);

    /* This implementation of the DEMOD device only supports one type of modulation */
    *Modulation = Reg0370QAM_GetQAMSize(&Instance->DeviceMap, Instance->IOHandle);

#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
    STTBX_Print(("%s Modulation=%u\n", identity, *Modulation));
#endif

    Error |= Instance->DeviceMap.Error;
    Instance->DeviceMap.Error = ST_NO_ERROR;

    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0370QAM_GetAGC()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0370QAM_GetAGC(DEMOD_Handle_t Handle, S16  *Agc)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
   const char *identity = "STTUNER d0370QAM.c demod_d0370QAM_GetAGC()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0370QAM_InstanceData_t *Instance;

    /* private driver instance data */
    Instance = D0370QAM_GetInstFromHandle(Handle);

    *Agc   = (S16)(0xFFFF & Reg0370QAM_GetAGC(&Instance->DeviceMap, Instance->IOHandle));

#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
    STTBX_Print(("%s Agc=%u\n", identity, *Agc));
#endif

    Error |= Instance->DeviceMap.Error;
    Instance->DeviceMap.Error = ST_NO_ERROR;

    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0370QAM_GetFECRates()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0370QAM_GetFECRates(DEMOD_Handle_t Handle, STTUNER_FECRate_t *FECRates)
{
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0370QAM_IsLocked()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0370QAM_IsLocked(DEMOD_Handle_t Handle, BOOL *IsLocked)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
   const char *identity = "STTUNER d0370QAM.c demod_d0370QAM_IsLocked()";
#endif
    ST_ErrorCode_t          Error = ST_NO_ERROR;
    U8                      Version;
    D0370QAM_InstanceData_t    *Instance;
    D0370QAM_SignalType_t      SignalType;

    /* private driver instance data */
    Instance = D0370QAM_GetInstFromHandle(Handle);

    *IsLocked = FALSE;

    /*
    --- Get Version Id (In fact to test I2C Access
    */
    Version = Reg0370QAM_GetSTB0370QAMId(&Instance->DeviceMap, Instance->IOHandle);
 if ((Version & STB0370QAM_DEVICE_VERSION) !=  STB0370QAM_DEVICE_VERSION)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
        STTBX_Print(("%s fail device not found (0x%02x)\n", identity, Version));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_UNKNOWN_DEVICE);
    }

    /*
    --- Get Information from demod
    */
    SignalType = Drv0370QAM_CheckAgc(&Instance->DeviceMap, Instance->IOHandle, &Instance->StateBlock.Params);
    if (SignalType == E370QAM_AGCOK)
    {
        SignalType = Drv0370QAM_CheckData(&Instance->DeviceMap, Instance->IOHandle, &Instance->StateBlock.Params);
        if (SignalType == E370QAM_DATAOK)
        {
            Instance->StateBlock.Result.SignalType = E370QAM_LOCKOK;
            *IsLocked = TRUE;
        }
        else
        {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
            STTBX_Print(("%s FRONT END IS UNLOCKED !!!! NO DATA\n", identity));
#endif
            /*
            --- Update Satus
            */
            Instance->StateBlock.Result.SignalType = SignalType;
        }
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
        STTBX_Print(("%s FRONT END IS UNLOCKED !!!! NO AGC\n", identity));
#endif
        /*
        --- Update Satus
        */
        Instance->StateBlock.Result.SignalType = SignalType;
    }


#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
    STTBX_Print(("%s TUNER IS %s\n", identity, (*IsLocked)?"LOCKED":"UNLOCKED"));
#endif


    Error |= Instance->DeviceMap.Error;
    Instance->DeviceMap.Error = ST_NO_ERROR;

    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0370QAM_SetFECRates()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0370QAM_SetFECRates(DEMOD_Handle_t Handle, STTUNER_FECRate_t FECRates)
{
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0370QAM_Tracking()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0370QAM_Tracking(DEMOD_Handle_t Handle, BOOL ForceTracking, U32 *NewFrequency, BOOL *SignalFound)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
   const char *identity = "STTUNER d0370QAM.c demod_d0370QAM_Tracking()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

    STTUNER_tuner_instance_t    *TunerInstance;     /* don't really need all these variables, done for clarity */
    STTUNER_InstanceDbase_t     *Inst;              /* pointer to instance database */
    STTUNER_Handle_t            TopHandle;          /* instance that contains the demos, tuner, lnb & diseqc driver set */
    D0370QAM_InstanceData_t        *Instance;
    BOOL                        IsLocked, DataFound;

    /* private driver instance data */
    Instance = D0370QAM_GetInstFromHandle(Handle);
    /* top level public instance data */
    Inst = STTUNER_GetDrvInst();

    /* this driver knows what to level instance it belongs to */
    TopHandle = Instance->TopLevelHandle;

    /* get the tuner instance for this driver from the top level handle */
    TunerInstance = &Inst[TopHandle].Cable.Tuner;

    /*
    --- Get Carrier And Data (TS) status
    */
    Error = demod_d0370QAM_IsLocked(Handle, &IsLocked);
    if ( Error == ST_NO_ERROR && IsLocked )
    {
        /*
        --- Tuner Is Locked
        */
        *SignalFound  = (Instance->StateBlock.Result.SignalType == E370QAM_LOCKOK)? TRUE : FALSE;
        *NewFrequency = 1000 * ((U32)Instance->StateBlock.Result.Frequency);
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
        STTBX_Print(("%s FRONT END IS LOCKED on %d Hz (CARRIER and DATA)\n", identity, *NewFrequency));
#endif
    }
    else
    {
        /*
        --- Tuner is UnLocked => Search Carrier and Data
        */
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
        STTBX_Print(("%s FRONT END IS UNLOCKED ==> SEARCH CARRIER AND DATA\n", identity));
#endif
        DataFound = FE_370QAM_Search(&Instance->DeviceMap, Instance->IOHandle, &Instance->StateBlock, Instance->TopLevelHandle);
        Error |= Instance->DeviceMap.Error;
        Instance->DeviceMap.Error = ST_NO_ERROR;
        if (Error != ST_NO_ERROR)
        {
            return(Error);
        }

        if (DataFound)
        {
            *SignalFound  = (Instance->StateBlock.Result.SignalType == E370QAM_LOCKOK)? TRUE : FALSE;
            *NewFrequency = 1000 * ((U32)Instance->StateBlock.Result.Frequency);
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
            STTBX_Print(("%s DATA on %d Hz\n", identity, *NewFrequency));
#endif
        }
        else
        {
            /*
            --- Tuner Is UnLocked
            */
            *SignalFound  = FALSE;
            *NewFrequency = 1000 * ((U32)Instance->StateBlock.Result.Frequency);
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
            STTBX_Print(("%s NO DATA on %d Hz !!!!\n", identity, *NewFrequency));
#endif
        }
    }

#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
    STTBX_Print(("%s %s, on %u Hz\n", identity, (*SignalFound)?"SIGNAL FOUND":"NO SIGNAL DOUND", *NewFrequency));
#endif

    Error |= Instance->DeviceMap.Error;
    Instance->DeviceMap.Error = ST_NO_ERROR;

    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0370QAM_ScanFrequency()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0370QAM_ScanFrequency(DEMOD_Handle_t Handle, U32  InitialFrequency, U32   SymbolRate,    U32   MaxOffset,
                                                                U32  TunerStep,        U8    DerotatorStep, BOOL *ScanSuccess,
                                                                U32 *NewFrequency,     U32   Mode,          U32   Guard,
                                                                U32  Force,            U32   Hierarchy,     U32   Spectrum,
                                                                U32  FreqOff,          U32   ChannelBW,     S32   EchoPos)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
    clock_t                     time_start_lock, time_end_lock;
   const char *identity = "STTUNER d0370QAM.c demod_d0370QAM_ScanFrequency()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_tuner_instance_t    *TunerInstance;
    STTUNER_InstanceDbase_t     *Inst;
    D0370QAM_InstanceData_t        *Instance;
    BOOL                        DataFound=FALSE;

    /* private driver instance data */
    Instance = D0370QAM_GetInstFromHandle(Handle);
    /* top level public instance data */
    Inst = STTUNER_GetDrvInst();

    /* get the tuner instance for this driver from the top level handle */
    TunerInstance = &Inst[Instance->TopLevelHandle].Cable.Tuner;

    /*STTUNER_task_lock();*/

    /*
    --- Spectrum inversion is set to
    */
    switch(Spectrum)
    {
        case STTUNER_INVERSION_NONE :
        case STTUNER_INVERSION :
        case STTUNER_INVERSION_AUTO :
            break;
        default:
            Inst[Instance->TopLevelHandle].Cable.SingleScan.Spectrum = STTUNER_INVERSION_NONE;
            break;
    }

    /*
    --- Modulation type is set to
    */
    switch(Inst[Instance->TopLevelHandle].Cable.Modulation)
    {
         case STTUNER_MOD_16QAM :
         case STTUNER_MOD_32QAM :
         case STTUNER_MOD_64QAM :
         case STTUNER_MOD_128QAM :
         case STTUNER_MOD_256QAM :
            break;
        case STTUNER_MOD_QAM :
        default:
            Inst[Instance->TopLevelHandle].Cable.Modulation = STTUNER_MOD_64QAM;
            break;
    }

    /*
    --- Start Init
    */
    Drv0370QAM_InitSearch(TunerInstance, &Instance->StateBlock,
                       Inst[Instance->TopLevelHandle].Cable.Modulation,
                       InitialFrequency,
                       SymbolRate,
                       Spectrum,
                       Inst[Instance->TopLevelHandle].Cable.ScanExact,
                       Inst[Instance->TopLevelHandle].Cable.SingleScan.J83);

    /*
    --- Try to lock
    */
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
    time_start_lock = STOS_time_now();
#endif
    DataFound = FE_370QAM_Search(&Instance->DeviceMap,Instance->IOHandle, &Instance->StateBlock, Instance->TopLevelHandle);
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
    time_end_lock = STOS_time_now();
#endif
    if (DataFound)
    {
        /*Instance->StateBlock.Result.SignalType = E297J_LOCKOK;*/
        /* Pass new frequency to caller */
        *NewFrequency = 1000 * ((U32)Instance->StateBlock.Result.Frequency);
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
        STTBX_Print(("%s  DATA FOUND on %u Hz\n", identity, *NewFrequency));
#endif
        *ScanSuccess = TRUE;
    }
    else
    {
        Instance->StateBlock.Result.SignalType = E370QAM_NODATA;
        *NewFrequency = InitialFrequency;
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
        STTBX_Print(("%s NO DATA on %u Hz\n", identity, *NewFrequency));
#endif
        *ScanSuccess = FALSE;
    }

    /*STTUNER_task_unlock();*/

#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
    switch(Inst[Instance->TopLevelHandle].Cable.Modulation)
    {        
        case STTUNER_MOD_16QAM :
            STTBX_Print(("%s QAM 16 : ", identity));
            break;
        case STTUNER_MOD_32QAM :
            STTBX_Print(("%s QAM 32 : ", identity));
            break;
        case STTUNER_MOD_64QAM :
            STTBX_Print(("%s QAM 64 : ", identity));
            break;
        case STTUNER_MOD_128QAM :
            STTBX_Print(("%s QAM 128 : ", identity));
            break;
        case STTUNER_MOD_256QAM :
            STTBX_Print(("%s QAM 256 : ", identity));
            break;
        case STTUNER_MOD_QAM :
            STTBX_Print(("%s QAM : ", identity));
            break;
    }
    STTBX_Print(("SR %d S/s TIME >> TUNER + DEMOD (%u ticks)(%u ms)\n", SymbolRate,
                STOS_time_minus(time_end_lock, time_start_lock),
                ((STOS_time_minus(time_end_lock, time_start_lock))*1000)/ST_GetClocksPerSecond()
                ));
#endif

    Error |= Instance->DeviceMap.Error;
    Instance->DeviceMap.Error = ST_NO_ERROR;

    return(Error);
}

/* ----------------------------------------------------------------------------
Name: demod_d0370QAM_ioctl()

Description:
    access device specific low-level functions

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0370QAM_ioctl(DEMOD_Handle_t Handle, U32 Function, void *InParams, void *OutParams, STTUNER_Da_Status_t *Status)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
   const char *identity = "STTUNER d0370QAM.c demod_d0370QAM_ioctl()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0370QAM_InstanceData_t *Instance;

    /* private driver instance data */
    Instance = D0370QAM_GetInstFromHandle(Handle);

    if(STTUNER_Util_CheckPtrNull(Instance) != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
        STTBX_Print(("%s invalid handle\n", identity));
#endif
        return(ST_ERROR_INVALID_HANDLE);
    }

#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
        STTBX_Print(("%s demod driver instance handle=%d\n", identity, Handle));
        STTBX_Print(("%s                     function=%d\n", identity, Function));
        STTBX_Print(("%s                   I/O handle=%d\n", identity, Instance->IOHandle));
#endif

    /* ---------- select what to do ---------- */
    switch(Function){

        case STTUNER_IOCTL_REG_IN: /* read device register */
            *(int *)OutParams = STTUNER_IOREG_GetRegister(&Instance->DeviceMap, Instance->IOHandle, *(int *)InParams);
            break;

        case STTUNER_IOCTL_REG_OUT: /* write device register */
            Error =  STTUNER_IOREG_SetRegister(  &Instance->DeviceMap,
                                                  Instance->IOHandle,
                  ((CABIOCTL_SETREG_InParams_t *)InParams)->RegIndex,
                  ((CABIOCTL_SETREG_InParams_t *)InParams)->Value );
            break;

        default:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
            STTBX_Print(("%s function %d not found\n", identity, Function));
#endif
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }


#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
    STTBX_Print(("%s called ok\n", identity, Function));    /* signal that function came back */
#endif

    Error |= Instance->DeviceMap.Error; /* Also accumulate I2C error */
    Instance->DeviceMap.Error = ST_NO_ERROR;

    return(Error);

}



/* ----------------------------------------------------------------------------
Name: demod_d0370QAM_ioaccess()

Description:
    called from ioarch.c when some driver does I/O but with the repeater/passthru
    set to point to this function.
Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0370QAM_ioaccess(DEMOD_Handle_t Handle, IOARCH_Handle_t IOHandle, STTUNER_IOARCH_Operation_t Operation, U16 SubAddr, U8 *Data, U32 TransferSize, U32 Timeout)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
   const char *identity = "STTUNER d0370QAM.c demod_d0370QAM_ioaccess()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    IOARCH_Handle_t ThisIOHandle;
    D0370QAM_InstanceData_t *Instance;

    Instance = D0370QAM_GetInstFromHandle(Handle);

    if(STTUNER_Util_CheckPtrNull(Instance) != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
        STTBX_Print(("%s invalid handle\n", identity));
#endif
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* this (demod) drivers I/O handle */
    ThisIOHandle = Instance->IOHandle;

    /* if STTUNER_IOARCH_MAX_HANDLES then passthrough function required using our device handle/address */
    if (IOHandle == STTUNER_IOARCH_MAX_HANDLES)
    {
/*
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
        STTBX_Print(("%s write passthru\n", identity));
#endif
*/
        Error = STTUNER_IOARCH_ReadWrite(ThisIOHandle, Operation, SubAddr, Data, TransferSize, Timeout);
    }
    else    /* repeater */
    {
/*
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM
        STTBX_Print(("%s write repeater\n", identity));
#endif
*/
        STTUNER_task_lock();    /* prevent any other activity on this bus (no task must preempt us) */
        #ifdef PROCESSOR_C1 /***For C1 core this delay needed otherwise STTUNER_task_lock(); not working fine ****/
        /*task_delay(5);*/
        #endif
        /* enable repeater then send the data  using I/O handle supplied through ioarch.c */
         Error = STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle, F0370QAM_STOP_ENABLE_1, 1); 
         Error = STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle, F0370QAM_I2CT_ON_1, 1);
        if ( Error == ST_NO_ERROR )
        {
            /*
            --- Send/Receive Data(s) to target at SubAddr
            */
            Error = STTUNER_IOARCH_ReadWriteNoRep(IOHandle, Operation, SubAddr, Data, TransferSize, Timeout*100);
        }
        STTUNER_task_unlock();
    }

    return(Error);
}



/* ------------------------------------------------------------------------- */
/* /\/\/\/\/\/\/\/\/\/\/\/\/\UTILITY Functions/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ */
/* ------------------------------------------------------------------------- */

D0370QAM_InstanceData_t *D0370QAM_GetInstFromHandle(DEMOD_Handle_t Handle)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM_HANDLES
   const char *identity = "STTUNER d0370QAM.c D0370QAM_GetInstFromHandle()";
#endif
    D0370QAM_InstanceData_t *Instance;

    Instance = (D0370QAM_InstanceData_t *)Handle;
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0370QAM_HANDLES
    STTBX_Print(("%s block at 0x%08x\n", identity, Instance));
#endif

    return(Instance);
}




/* End of d0370QAM.c */
