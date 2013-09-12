/*---------------------------------------------------------------
File Name: reg0297.h (was reg0299.h)

Description: 

    STV0297 register map and functions

Copyright (C) 1999-2001 STMicroelectronics

   date: 01-October-2001
version: 3.2.0
 author: from STV0299 and MB validation drivers.
comment: Write for multi-instance/multi-FrontEnd.

Revision History:
    
Reference:
    ST API Definition "TUNER Driver API" DVD-API-06
---------------------------------------------------------------------------- */

/* define to prevent recursive inclusion */
#ifndef __STTUNER_DEMOD_R0297_H
#define __STTUNER_DEMOD_R0297_H


/* includes --------------------------------------------------------------- */

#include "stlite.h"
#include "ioarch.h"
#include "ioreg.h"


#ifdef __cplusplus
extern "C"
{
#endif                          /* __cplusplus */



/* register mappings ------------------------------------------------------- */

/*
 REGISTERS
*/

/* EQU_0 */
 #define R0297_EQU_0 0x0
 #define  F0297_U_THRESHOLD 0xf
 #define  F0297_MODE_SELECT 0x70

/* EQU_1 */
 #define R0297_EQU_1 0x1
 #define  F0297_BLIND_U 0x1000f
 #define  F0297_INITIAL_U 0x100f0

/* EQU_3 */
 #define R0297_EQU_3 0x3
 #define  F0297_EQ_FSM_CTL 0x30003
 #define  F0297_EQ_COEFF_CTL 0x3000c

/* EQU_4 */
 #define R0297_EQU_4 0x4
 #define  F0297_NBLIND 0x40001

/* EQU_7 */
 #define R0297_EQU_7 0x7
 #define  F0297_NOISE_EST_LO 0x700ff

/* EQU_8 */
 #define R0297_EQU_8 0x8
 #define  F0297_NOISE_EST_HI 0x800ff

/* INITDEM_0 */
 #define R0297_INITDEM_0 0x20
 #define  F0297_DEM_FQCY_LO 0x2000ff

/* INITDEM_1 */
 #define R0297_INITDEM_1 0x21
 #define  F0297_DEM_FQCY_HI 0x2100ff

/* INITDEM_2 */
 #define R0297_INITDEM_2 0x22
 #define  F0297_LATENCY 0x2200ff

/* INITDEM_3 */
 #define R0297_INITDEM_3 0x23
 #define  F0297_SCAN_STEP_LO 0x2300ff

/* INITDEM_4 */
 #define R0297_INITDEM_4 0x24
 #define  F0297_CHSCANITEN 0x240080
 #define  F0297_CHSCANITSOFT 0x240040
 #define  F0297_SCAN_STEP_HI 0x24003f

/* INITDEM_5 */
 #define R0297_INITDEM_5 0x25
 #define  F0297_IN_DEMOD_EN 0x250080
 #define  F0297_SCAN_ON 0x250040
 #define  F0297_AUTOSTOP 0x250020
 #define  F0297_SCALE_A 0x250010
 #define  F0297_SCALE_B 0x25000c

/* DELAGC_0 */
 #define R0297_DELAGC_0 0x30
 #define  F0297_AGC2MAX 0x3000ff

/* DELAGC_1 */
 #define R0297_DELAGC_1 0x31
 #define  F0297_AGC2MIN 0x3100ff

/* DELAGC_2 */
 #define R0297_DELAGC_2 0x32
 #define  F0297_AGC1MAX 0x3200ff

/* DELAGC_3 */
 #define R0297_DELAGC_3 0x33
 #define  F0297_AGC1MIN 0x3300ff

/* DELAGC_4 */
 #define R0297_DELAGC_4 0x34
 #define  F0297_RATIO_A 0x3400e0
 #define  F0297_RATIO_B 0x340018
 #define  F0297_RATIO_C 0x340007

/* DELAGC_5 */
 #define R0297_DELAGC_5 0x35
 #define  F0297_AGC2_THRES 0x3500ff

/* DELAGC_6 */
 #define R0297_DELAGC_6 0x36
 #define  F0297_DAGC_ON 0x360080
 #define  F0297_FRZ2_CTRL 0x360060
 #define  F0297_FRZ1_CTRL 0x360018

/* DELAGC_7 */
 #define R0297_DELAGC_7 0x37
 #define  F0297_TIME_CST 0x370070
 #define  F0297_OVF_RATE_LO 0x37000f
 #define  F0297_CORNER_RATE_LO 0x37000f

/* DELAGC_8 */
 #define R0297_DELAGC_8 0x38
 #define  F0297_OVF_RATE_HI 0x3800ff
 #define  F0297_CORNER_RATE_HI 0x3800ff

/* WBAGC_0 */
 #define R0297_WBAGC_0 0x40
 #define  F0297_I_REF 0x4000ff

/* WBAGC_1 */
 #define R0297_WBAGC_1 0x41
 #define  F0297_AGC2SD_LO 0x4100ff

/* WBAGC_2 */
 #define R0297_WBAGC_2 0x42
 #define  F0297_AGC2SD_HI 0x420003
 #define  F0297_ACQ_THRESH 0x42003c

/* WBAGC_3 */
 #define R0297_WBAGC_3 0x43
 #define  F0297_WAGC_CLR 0x430040
 #define  F0297_WAGC_INV 0x430020
 #define  F0297_WAGC_EN 0x430010
 #define  F0297_WAGC_ACQ 0x430008
 #define  F0297_SWAP 0x430004

/* WBAGC_4 */
 #define R0297_WBAGC_4 0x44
 #define  F0297_ROLL_LO 0x4400ff

/* WBAGC_5 */
 #define R0297_WBAGC_5 0x45
 #define  F0297_ACQ_COUNT_LO 0x4500ff

/* WBAGC_6 */
 #define R0297_WBAGC_6 0x46
 #define  F0297_ACQ_COUNT_HI 0x4600ff

/* WBAGC_9 */
 #define R0297_WBAGC_9 0x49
 #define  F0297_ROLL_HI 0x4900ff

/* WBAGC_10 */
 #define R0297_WBAGC_10 0x4a
 #define  F0297_IF_PWM_LO 0x4a00ff
 #define  F0297_TARGET_RATE_LO 0x4a00ff

/* WBAGC_11 */
 #define R0297_WBAGC_11 0x4b
 #define  F0297_IF_PWM_HI 0x4b00ff
 #define  F0297_TARGET_RATE_HI 0x4b00ff

/* STLOOP_2 */
 #define R0297_STLOOP_2 0x52
 #define  F0297_GAIN_SCALE_PATH0 0x5200e0
 #define  F0297_GAIN_SCALE_PATH1 0x52001c
 #define  F0297_INTEGRAL_GAIN_HI 0x520003

/* STLOOP_3 */
 #define R0297_STLOOP_3 0x53
 #define  F0297_DIRECT_GAIN_LO 0x5300ff

/* STLOOP_5 */
 #define R0297_STLOOP_5 0x55
 #define  F0297_SYMB_RATE_0 0x5500ff

/* STLOOP_6 */
 #define R0297_STLOOP_6 0x56
 #define  F0297_SYMB_RATE_1 0x5600ff

/* STLOOP_7 */
 #define R0297_STLOOP_7 0x57
 #define  F0297_SYMB_RATE_2 0x5700ff

/* STLOOP_8 */
 #define R0297_STLOOP_8 0x58
 #define  F0297_SYMB_RATE_3 0x5800ff

/* STLOOP_9 */
 #define R0297_STLOOP_9 0x59
 #define  F0297_INTEGRAL_GAIN_LO 0x59001f
 #define  F0297_DIRECT_GAIN_HI 0x5900e0

/* STLOOP_10 */
 #define R0297_STLOOP_10 0x5a
 #define  F0297_PHASE_EN 0x5a0040
 #define  F0297_PHASE_CLR 0x5a0020
 #define  F0297_ERR_RANGE 0x5a001f

/* STLOOP_11 */
 #define R0297_STLOOP_11 0x5b
 #define  F0297_ALGOSEL 0x5b0030
 #define  F0297_ERR_CLR 0x5b0002
 #define  F0297_ERR_EN 0x5b0001

/* CRL_0 */
 #define R0297_CRL_0 0x60
 #define  F0297_SWEEP_LO 0x6000ff

/* CRL_1 */
 #define R0297_CRL_1 0x61
 #define  F0297_GAIN_INT 0x61000f
 #define  F0297_GAIN_DIR 0x610070

/* CRL_2 */
 #define R0297_CRL_2 0x62
 #define  F0297_GAIN_INT_ADJ 0x620003
 #define  F0297_GAIN_DIR_ADJ 0x62000c

/* CRL_3 */
 #define R0297_CRL_3 0x63
 #define  F0297_APHASE_0 0x6300ff

/* CRL_4 */
 #define R0297_CRL_4 0x64
 #define  F0297_APHASE_1 0x6400ff

/* CRL_5 */
 #define R0297_CRL_5 0x65
 #define  F0297_APHASE_2 0x6500ff

/* CRL_6 */
 #define R0297_CRL_6 0x66
 #define  F0297_IPHASE_0 0x6600ff

/* CRL_7 */
 #define R0297_CRL_7 0x67
 #define  F0297_IPHASE_1 0x6700ff

/* CRL_8 */
 #define R0297_CRL_8 0x68
 #define  F0297_IPHASE_2 0x6800ff

/* CRL_9 */
 #define R0297_CRL_9 0x69
 #define  F0297_IPHASE_3 0x69000f
 #define  F0297_SWEEP_HI 0x6900f0

/* CRL_10 */
 #define R0297_CRL_10 0x6a
 #define  F0297_SWEEP_EN 0x6a0001
 #define  F0297_PH_EN 0x6a0002
 #define  F0297_DIR_EN 0x6a0004
 #define  F0297_INT_EN 0x6a0008
 #define  F0297_DIR_DIS 0x6a0010
 #define  F0297_INT_DIS 0x6a0020

/* CRL_11 */
 #define R0297_CRL_11 0x6b
 #define  F0297_CRL_SNAPSHOT 0x6b00ff

/* PMFAGC_0 */
 #define R0297_PMFAGC_0 0x70
 #define  F0297_LOCK_THRES_LO 0x7000ff

/* PMFAGC_1 */
 #define R0297_PMFAGC_1 0x71
 #define  F0297_PMFA_F_UNLOCK 0x710080
 #define  F0297_PMFA_F_LOCK 0x710040
 #define  F0297_WBAGC_F_LOCK 0x710020
 #define  F0297_UP_STOP 0x710010
 #define  F0297_LOCK_THRES_HI 0x71000f

/* PMFAGC_2 */
 #define R0297_PMFAGC_2 0x72
 #define  F0297_PMFA_ACC0 0x7200ff

/* PMFAGC_3 */
 #define R0297_PMFAGC_3 0x73
 #define  F0297_PMFA_ACC1 0x7300ff

/* PMFAGC_4 */
 #define R0297_PMFAGC_4 0x74
 #define  F0297_PMFA_LOCK_STATE 0x740080
 #define  F0297_PMFA_ACC2 0x74000f

/* CTRL_0 */
 #define R0297_CTRL_0 0x80
 #define  F0297_SOFT_RESET 0x800001
 #define  F0297_VERSION 0x800070

/* CTRL_1 */
 #define R0297_CTRL_1 0x81
 #define  F0297_RESET_DI 0x810001

/* CTRL_2 */
 #define R0297_CTRL_2 0x82
 #define  F0297_RS_UNCORR 0x820020
 #define  F0297_CORNER_LOCK 0x820010
 #define  F0297_EQU_LMS2 0x820008
 #define  F0297_EQU_LMS1 0x820004
 #define  F0297_PMFAGC_IT 0x820002
 #define  F0297_WBAGC_IT 0x820001

/* CTRL_3 */
 #define R0297_CTRL_3 0x83
 #define  F0297_J83C 0x830001
 #define  F0297_DFS 0x830002
 #define  F0297_SPEC_INV 0x830008
 #define  F0297_RESET_RS 0x830010
 #define  F0297_TEST_SEL 0x8300e0

/* CTRL_4 */
 #define R0297_CTRL_4 0x84
 #define  F0297_RESET_EQL 0x840001
 #define  F0297_CKX2SEL 0x840002
 #define  F0297_CKX2DIS 0x840004
 #define  F0297_INVADCLK 0x840008
 #define  F0297_M_OEN 0x840010
 #define  F0297_AGC_OD 0x840020

/* CTRL_5 */
 #define R0297_CTRL_5 0x85
 #define  F0297_LOCKPOL 0x850080
 #define  F0297_DI_SY_MASK 0x850040
 #define  F0297_DI_SY_EV 0x850020
 #define  F0297_DI_SY_DIR 0x850010
 #define  F0297_SYNC_MSK 0x850004
 #define  F0297_SYNC_EV 0x850002
 #define  F0297_SYNC_DIR 0x850001

/* CTRL_6 */
 #define R0297_CTRL_6 0x86
 #define  F0297_I2CT_EN 0x860080
 #define  F0297_SCLT_OD 0x860040
 #define  F0297_EXTADCLK_EN 0x860020
 #define  F0297_ITLOCKSEL 0x860010
 #define  F0297_ITPWMSEL 0x860008
 #define  F0297_LOCKSCE 0x860006
 #define  F0297_TWB_ACT 0x860001

/* CTRL_7 */
 #define R0297_CTRL_7 0x87
 #define  F0297_SOURCESEL 0x870080
 #define  F0297_PRGCLKDIV 0x870070
 #define  F0297_AUXCLKSEL 0x870008
 #define  F0297_ITLOCK_OD 0x870002
 #define  F0297_ITPWM_OD 0x870001

/* CTRL_8 */
 #define R0297_CTRL_8 0x88
 #define  F0297_AGC12SEL 0x880080
 #define  F0297_AGC12B_EN 0x880040
 #define  F0297_SIGMA_INV_1 0x880020
 #define  F0297_SIGMA_INV_2 0x880010
 #define  F0297_EN_CORNER_DET 0x880008

/* CTRL_9 */
 #define R0297_CTRL_9 0x89
 #define  F0297_AUTOQAMMODE_SEL 0x890080
 #define  F0297_AUTOCONSTEL_TIMER 0x890078
 #define  F0297_AUTOSTOP_CONSTEL 0x890004
 #define  F0297_AUTOCONSTEL_ON 0x890002

/* DEINT_SYNC_0 */
 #define R0297_DEINT_SYNC_0 0x90
 #define  F0297_DI_UNLOCK 0x900080
 #define  F0297_DI_FREEZE 0x900040
 #define  F0297_MISMATCH 0x900030
 #define  F0297_ACQ_MODE 0x90000c
 #define  F0297_TRKMODE 0x900003

/* DEINT_SYNC_1 */
 #define R0297_DEINT_SYNC_1 0x91
 #define  F0297_SYNLOST 0x910020

/* BERT_0 */
 #define R0297_BERT_0 0xa0
 #define  F0297_BERT_ON 0xa00080
 #define  F0297_ERR_SOURCE 0xa00010
 #define  F0297_ERR_MODE 0xa00008
 #define  F0297_NBYTE 0xa00007

/* BERT_1 */
 #define R0297_BERT_1 0xa1
 #define  F0297_ERRCOUNT_LO 0xa100ff

/* BERT_2 */
 #define R0297_BERT_2 0xa2
 #define  F0297_ERRCOUNT_HI 0xa200ff

/* DEINT_0 */
 #define R0297_DEINT_0 0xb0
 #define  F0297_USEINT 0xb00080
 #define  F0297_DAVIC 0xb00040
 #define  F0297_M 0xb0001f

/* DEINT_1 */
 #define R0297_DEINT_1 0xb1
 #define  F0297_DEPTH 0xb100ff

/* OUTFORMAT_0 */
 #define R0297_OUTFORMAT_0 0xc0
 #define  F0297_REFRESH47 0xc00040
 #define  F0297_BE_BYPASS 0xc00020
 #define  F0297_CKOUTPAR 0xc00010
 #define  F0297_CT_NBST 0xc00008
 #define  F0297_S_NP 0xc00004
 #define  F0297_TEI_ENA 0xc00002
 #define  F0297_DS_ENA 0xc00001

/* OUTFORMAT_1 */
 #define R0297_OUTFORMAT_1 0xc1
 #define  F0297_SYNC_STRIP 0xc10080
 #define  F0297_CI_EN 0xc10040
 #define  F0297_CICLK_POL 0xc10020
 #define  F0297_CICLK_BASE 0xc10010

/* OUTFORMAT_2 */
 #define R0297_OUTFORMAT_2 0xc2
 #define  F0297_CI_DIVRANGE 0xc2003f

/* RS_DESC_0 */
 #define R0297_RS_DESC_0 0xd0
 #define  F0297_BK_CT_LO 0xd000ff

/* RS_DESC_1 */
 #define R0297_RS_DESC_1 0xd1
 #define  F0297_BK_CT_HI 0xd100ff

/* RS_DESC_2 */
 #define R0297_RS_DESC_2 0xd2
 #define  F0297_CORR_CT_LO 0xd200ff

/* RS_DESC_3 */
 #define R0297_RS_DESC_3 0xd3
 #define  F0297_CORR_CT_HI 0xd300ff

/* RS_DESC_4 */
 #define R0297_RS_DESC_4 0xd4
 #define  F0297_UNCORR_CT_LO 0xd400ff

/* RS_DESC_5 */
 #define R0297_RS_DESC_5 0xd5
 #define  F0297_UNCORR_CT_HI 0xd500ff

/* RS_DESC_14 */
 #define R0297_RS_DESC_14 0xde
 #define  F0297_DIS_UNLOCK 0xde0004
 #define  F0297_MODE 0xde0003

/* RS_DESC_15 */
 #define R0297_RS_DESC_15 0xdf
 #define  F0297_CT_CLEAR 0xdf0001
 #define  F0297_CT_HOLD 0xdf0002
 #define  F0297_RS_NOCORR 0xdf0004
 #define  F0297_SYNCSTATE 0xdf0080
/* defines ----------------------------------------------------------------- */

/* Modulations */
#define D0297_QAM16	                        0
#define D0297_QAM32                         1
#define D0297_QAM64	                        4
#define D0297_QAM128                        2
#define D0297_QAM256	                    3

/* AGC */
#define D0297_WBAGCOFF                      0x60
#define D0297_WBAGCON                       0x30

/* MACRO definitions */
#define MAC0297_ABS(X)                      ((X)<0 ?   (-X) : (X))
#define MAC0297_MAX(X,Y)                    ((X)>=(Y) ? (X) : (Y))
#define MAC0297_MIN(X,Y)                    ((X)<=(Y) ? (X) : (Y)) 
#define MAC0297_MAKEWORD(X,Y)               ((X<<8)+(Y))
#define MAC0297_B0(X)                       ((X & 0xFF))
#define MAC0297_B1(Y)                       ((Y>>8)& 0xFF)
#define MAC0297_B2(Y)                       ((Y>>16)& 0xFF)
#define MAC0297_B3(Y)                       ((Y>>24)& 0xFF)


/* public types ------------------------------------------------------------ */



/* functions --------------------------------------------------------------- */


ST_ErrorCode_t          Reg0297_Install             (STTUNER_IOREG_DeviceMap_t *DeviceMap);
ST_ErrorCode_t          Reg0297_Open                (STTUNER_IOREG_DeviceMap_t *DeviceMap, U32 ExternalClock);
ST_ErrorCode_t          Reg0297_Close               (STTUNER_IOREG_DeviceMap_t *DeviceMap);
ST_ErrorCode_t          Reg0297_UnInstall           (STTUNER_IOREG_DeviceMap_t *DeviceMap);

/******** EQUALIZER ****************************************/
void                    Reg0297_SetQAMSize          (STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, STTUNER_Modulation_t Value);
STTUNER_Modulation_t    Reg0297_GetQAMSize          (STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle);
/********** WBAGC functions********/
void                    Reg0297_WBAGCOff            (STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle);
void                    Reg0297_WBAGCOn             (STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle);
void                    Reg0297_SetAGC              (STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, U16 Value);
U16                     Reg0297_GetAGC              (STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle);
/********** STL functions *********/
void                    Reg0297_SetSymbolRate       (STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, unsigned long Value);
unsigned long           Reg0297_GetSymbolRate       (STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle);
void                    Reg0297_SetSweepRate        (STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, unsigned long Value);
void                    Reg0297_SetFrequencyOffset  (STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, long Value);
long                    Reg0297_GetFrequencyOffset  (STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle);
U8                      Reg0297_GetSTV0297Id        (STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle);
/********** ERR functions *********/
void                    Reg0297_StartBlkCounter     (STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle);
void                    Reg0297_StopBlkCounter      (STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle);
U16                     Reg0297_GetBlkCounter       (STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle);
U16                     Reg0297_GetCorrBlk          (STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle);
U16                     Reg0297_GetUncorrBlk        (STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle);

#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif                          /* __STTUNER_DEMOD_R0297_H */

/* End of reg0297.h */
