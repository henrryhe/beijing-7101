/*---------------------------------------------------------------
File Name: reg0297j.h (was reg0297.h)

Description:

    STV0297J register map and functions

Copyright (C) 1999-2001 STMicroelectronics

   date: 15-May-2002
version: 3.5.0
 author: from STV0297 and MB validation drivers.
comment: Write for multi-instance/multi-FrontEnd.

Revision History:

Reference:
    ST API Definition "TUNER Driver API" DVD-API-06
---------------------------------------------------------------------------- */

/* define to prevent recursive inclusion */
#ifndef __STTUNER_DEMOD_R0297J_H
#define __STTUNER_DEMOD_R0297J_H


/* includes --------------------------------------------------------------- */

#include "stlite.h"
#include "ioarch.h"
#include "ioreg.h"


#ifdef __cplusplus
extern "C"
{
#endif                          /* __cplusplus */

/* register mappings ------------------------------------------------------- */

/* REGISTERS*/


/* _EQU_0 */
 #define R0297J_EQU_0 0x0
 #define            F0297J_MODE_SELECT 0xf0
 #define            F0297J_U_THRESHOLD 0xf

/* _EQU_1 */
 #define R0297J_EQU_1 0x1
 #define            F0297J_INITIAL_U 0x100f0
 #define            F0297J_BLIND_U 0x1000f

/* _EQU_3 */
 #define R0297J_EQU_3 0x3
 #define            F0297J_NBLIND 0x30020
 #define            F0297J_EQ_COEFF_CTL 0x3000c
 #define            F0297J_EQ_FSM_CTL 0x30003

/* _EQU_4 */
 #define R0297J_EQU_4 0x4
 #define            F0297J_SPEC_INV 0x40010
 #define            F0297J_EN_CORNER_DET 0x40008
 #define            F0297J_TIME 0x40007

/* _EQU_5 */
 #define R0297J_EQU_5 0x5
 #define            F0297J_TARGET_RATE_LSB 0x500ff

/* _EQU_6 */
 #define R0297J_EQU_6 0x6
 #define            F0297J_CORNER_RATE_LSB 0x600f0
 #define            F0297J_TARGET_RATE_MSB 0x6000f

/* _EQU_7 */
 #define R0297J_EQU_7 0x7
 #define            F0297J_CORNER_RATE_MSB 0x700ff

/* _EQU_8 */
 #define R0297J_EQU_8 0x8
 #define            F0297J_NOISE_EST_LO 0x800ff

/* _EQU_9 */
 #define R0297J_EQU_9 0x9
 #define            F0297J_NOISE_EST_HI 0x9003f

/* _EQU_10 */
 #define R0297J_EQU_10 0xa
 #define           F0297J_Q_CONST 0xa00ff

/* _EQU_11 */
 #define R0297J_EQU_11 0xb
 #define           F0297J_I_CONST 0xb00ff

/* _INITDEM_0 */
 #define R0297J_INITDEM_0 0xc
 #define        F0297J_DEM_FQCY_LO 0xc00ff

/* _INITDEM_1 */
 #define R0297J_INITDEM_1 0xd
 #define        F0297J_DEM_FQCY_HI 0xd00ff

/* _INITDEM_2 */
 #define R0297J_INITDEM_2 0xe
 #define        F0297J_LATENCY 0xe00ff

/* _INITDEM_3 */
 #define R0297J_INITDEM_3 0x10
 #define        F0297J_SCAN_STEP_LO 0x1000ff

/* _INITDEM_4 */
 #define R0297J_INITDEM_4 0x11
 #define        F0297J_CHSCANITEN 0x110080
 #define        F0297J_CHSCANITSOFT 0x110040
 #define        F0297J_SCAN_STEP_HI 0x11003f

/* _INITDEM_5 */
 #define R0297J_INITDEM_5 0x12
 #define        F0297J_IN_DEMOD_EN 0x120080
 #define        F0297J_SCAN_ON 0x120040
 #define        F0297J_AUTOSTOP 0x120020
 #define        F0297J_SCALE_A 0x120010
 #define        F0297J_SCALE_B 0x12000c

/* _DELAGC_0 */
 #define R0297J_DELAGC_0 0x14
 #define         F0297J_AGC2MAX 0x1400ff

/* _DELAGC_1 */
 #define R0297J_DELAGC_1 0x15
 #define         F0297J_AGC2MIN 0x1500ff

/* _DELAGC_2 */
 #define R0297J_DELAGC_2 0x16
 #define         F0297J_AGC1MAX 0x1600ff

/* _DELAGC_3 */
 #define R0297J_DELAGC_3 0x17
 #define         F0297J_AGC1MIN 0x1700ff

/* _DELAGC_4 */
 #define R0297J_DELAGC_4 0x18
 #define         F0297J_RATIO_A 0x1800e0
 #define         F0297J_RATIO_B 0x180018
 #define         F0297J_RATIO_C 0x180007

/* _DELAGC_5 */
 #define R0297J_DELAGC_5 0x19
 #define         F0297J_AGC2_THRES 0x1900ff

/* _DELAGC_6 */
 #define R0297J_DELAGC_6 0x1a
 #define         F0297J_DAGC_ON 0x1a0080
 #define         F0297J_FRZ2_CTRL 0x1a0060
 #define         F0297J_FRZ1_CTRL 0x1a0018

/* _DELAGC_7 */
 #define R0297J_DELAGC_7 0x1c
 #define         F0297J_AD_AVERAGE_LO 0x1c00f0
 #define         F0297J_TIME_CST 0x1c000e

/* _DELAGC_8 */
 #define R0297J_DELAGC_8 0x1d
 #define         F0297J_AD_AVERAGE_HI 0x1d00ff

/* _DELAGC_10 */
 #define R0297J_DELAGC_10 0x20
 #define        F0297J_AGC2SD1_LO 0x2000ff

/* _DELAGC_11 */
 #define R0297J_DELAGC_11 0x21
 #define        F0297J_AGC2SD1_HI 0x210003

/* _DELAGC_12 */
 #define R0297J_DELAGC_12 0x22
 #define        F0297J_AGC2SD2_LO 0x2200ff

/* _DELAGC_13 */
 #define R0297J_DELAGC_13 0x23
 #define        F0297J_AGC2SD2_HI 0x230003

/* _WBAGC_0 */
 #define R0297J_WBAGC_0 0x24
 #define          F0297J_AGC2SD_LO 0x2400ff

/* _WBAGC_1 */
 #define R0297J_WBAGC_1 0x25
 #define          F0297J_ACQ_THRESH 0x25003c
 #define          F0297J_AGC2SD_HI 0x250003

/* _WBAGC_2 */
 #define R0297J_WBAGC_2 0x26
 #define          F0297J_I_REF 0x26007f

/* _WBAGC_3 */
 #define R0297J_WBAGC_3 0x27
 #define          F0297J_WAGC_CLR 0x270040
 #define          F0297J_WAGC_INV 0x270020
 #define          F0297J_WAGC_EN 0x270010
 #define          F0297J_WAGC_ACQ 0x270008
 #define          F0297J_SWAP 0x270004

/* _WBAGC_4 */
 #define R0297J_WBAGC_4 0x28
 #define          F0297J_ACQ_COUNT_LO 0x2800ff

/* _WBAGC_5 */
 #define R0297J_WBAGC_5 0x29
 #define          F0297J_ACQ_COUNT_HI 0x2900ff

/* _WBAGC_6 */
 #define R0297J_WBAGC_6 0x2a
 #define          F0297J_ROLL_LO 0x2a00ff

/* _WBAGC_7 */
 #define R0297J_WBAGC_7 0x2b
 #define          F0297J_ROLL_HI 0x2b00ff

/* _STLOOP_1 */
 #define R0297J_STLOOP_1 0x2c
 #define         F0297J_DIRECT_GAIN_LO 0x2c00ff

/* _STLOOP_2 */
 #define R0297J_STLOOP_2 0x2d
 #define         F0297J_DIRECT_GAIN_HI 0x2d0007

/* _STLOOP_3 */
 #define R0297J_STLOOP_3 0x2e
 #define         F0297J_INTEGRAL_GAIN_LO 0x2e00ff

/* _STLOOP_4 */
 #define R0297J_STLOOP_4 0x2f
 #define         F0297J_GAIN_SCALE_PATH0 0x2f00e0
 #define         F0297J_GAIN_SCALE_PATH1 0x2f001c
 #define         F0297J_INTEGRAL_GAIN_HI 0x2f0003

/* _STLOOP_5 */
 #define R0297J_STLOOP_5 0x30
 #define         F0297J_SYMB_RATE_0 0x3000ff

/* _STLOOP_6 */
 #define R0297J_STLOOP_6 0x31
 #define         F0297J_SYMB_RATE_1 0x3100ff

/* _STLOOP_7 */
 #define R0297J_STLOOP_7 0x32
 #define         F0297J_SYMB_RATE_2 0x3200ff

/* _STLOOP_8 */
 #define R0297J_STLOOP_8 0x33
 #define         F0297J_SYMB_RATE_3 0x3300ff

/* _STLOOP_9 */
 #define R0297J_STLOOP_9 0x34
 #define         F0297J_PHASE_EN 0x340040
 #define         F0297J_PHASE_CLR 0x340020
 #define         F0297J_ERR_RANGE 0x34001f

/* _STLOOP_10 */
 #define R0297J_STLOOP_10 0x35
 #define        F0297J_ROLLOFF 0x3500c0
 #define        F0297J_ALGOSEL 0x350030
 #define        F0297J_DIR 0x350008
 #define        F0297J_EN_DIR 0x350004
 #define        F0297J_ERR_CLR 0x350002
 #define        F0297J_ERR_EN 0x350001

/* _CRL_1 */
 #define R0297J_CRL_1 0x38
 #define            F0297J_SECOND_LOOP_BYPASS 0x380080
 #define            F0297J_GAIN_DIR 0x380070
 #define            F0297J_GAIN_INT 0x38000f

/* _CRL_2 */
 #define R0297J_CRL_2 0x39
 #define            F0297J_GAIN_DIR_PN 0x3900f0
 #define            F0297J_GAIN_DIR_ADJ 0x39000c
 #define            F0297J_GAIN_INT_ADJ 0x390003

/* _CRL_3 */
 #define R0297J_CRL_3 0x3a
 #define            F0297J_PN_LOOP_SEL 0x3a0040
 #define            F0297J_INT_DIS 0x3a0020
 #define            F0297J_DIR_DIS 0x3a0010
 #define            F0297J_INT_EN 0x3a0008
 #define            F0297J_DIR_EN 0x3a0004
 #define            F0297J_PH_EN 0x3a0002
 #define            F0297J_SWEEP_EN 0x3a0001

/* _CRL_4 */
 #define R0297J_CRL_4 0x3b
 #define            F0297J_CRL_SNAPSHOT 0x3b00ff

/* _CRL_5 */
 #define R0297J_CRL_5 0x3c
 #define            F0297J_APHASE_0 0x3c00ff

/* _CRL_6 */
 #define R0297J_CRL_6 0x3d
 #define            F0297J_APHASE_1 0x3d00ff

/* _CRL_7 */
 #define R0297J_CRL_7 0x3e
 #define            F0297J_APHASE_2 0x3e00ff

/* _CRL_8 */
 #define R0297J_CRL_8 0x3f
 #define            F0297J_PN_CRL_TH 0x3f0020
 #define            F0297J_PN_CRL_COEFF 0x3f001f

/* _CRL_9 */
 #define R0297J_CRL_9 0x40
 #define            F0297J_IPHASE_0 0x4000ff

/* _CRL_10 */
 #define R0297J_CRL_10 0x41
 #define           F0297J_IPHASE_1 0x4100ff

/* _CRL_11 */
 #define R0297J_CRL_11 0x42
 #define           F0297J_IPHASE_2 0x4200ff

/* _CRL_12 */
 #define R0297J_CRL_12 0x43
 #define           F0297J_IPHASE_3 0x43000f

/* _CRL_13 */
 #define R0297J_CRL_13 0x44
 #define           F0297J_SWEEP_LO 0x4400ff

/* _CRL_14 */
 #define R0297J_CRL_14 0x45
 #define           F0297J_SWEEP_HI 0x45000f

/* _PMFAGC_0 */
 #define R0297J_PMFAGC_0 0x48
 #define         F0297J_LOCK_THRES_LO 0x4800ff

/* _PMFAGC_1 */
 #define R0297J_PMFAGC_1 0x49
 #define         F0297J_LOCK_THRES_HI 0x49000f

/* _PMFAGC_2 */
 #define R0297J_PMFAGC_2 0x4a
 #define         F0297J_PMFA_LOCK_STATE 0x4a0010
 #define         F0297J_PMFA_F_UNLOCK 0x4a0008
 #define         F0297J_PMFA_F_LOCK 0x4a0004
 #define         F0297J_WBAGC_F_LOCK 0x4a0002
 #define         F0297J_UP_STOP 0x4a0001

/* _PMFAGC_3 */
 #define R0297J_PMFAGC_3 0x4c
 #define         F0297J_PMFA_ACC0 0x4c00ff

/* _PMFAGC_4 */
 #define R0297J_PMFAGC_4 0x4d
 #define         F0297J_PMFA_ACC1 0x4d00ff

/* _PMFAGC_5 */
 #define R0297J_PMFAGC_5 0x4e
 #define         F0297J_PMFA_ACC2 0x4e000f

/* _INTER_0 */
 #define R0297J_INTER_0 0x50
 #define          F0297J_MPEGA_MASK 0x500080
 #define          F0297J_UNCORRA_MASK 0x500040
 #define          F0297J_DI_LOCKA_MASK 0x500020
 #define          F0297J_CARRIER_LOCK_MASK 0x500010
 #define          F0297J_LMS2_MASK 0x500008
 #define          F0297J_LMS1_MASK 0x500004
 #define          F0297J_WBAGC_LOCK_EN_MASK 0x500002
 #define          F0297J_MPEGB_MASK 0x500001

/* _INTER_1 */
 #define R0297J_INTER_1 0x51
 #define          F0297J_UNCORRB_MASK 0x510080
 #define          F0297J_SYNCQ_MASK 0x510040
 #define          F0297J_SYNCI_MASK 0x510020
 #define          F0297J_ENFADDET_MASK 0x510010
 #define          F0297J_ENCRL_UL_IT_MASK 0x510008
 #define          F0297J_UPDATE_READY_MASK 0x510004
 #define          F0297J_END_FRAME_HEADER_MASK 0x510002
 #define          F0297J_CONTCNT_EVENT_MASK 0x510001

/* _INTER_2 */
 #define R0297J_INTER_2 0x52
 #define          F0297J_MPEGA_LOCK 0x520080
 #define          F0297J_UNCORRA 0x520040
 #define          F0297J_DI_LOCKA 0x520020
 #define          F0297J_CORNER_LOCK 0x520010
 #define          F0297J_EQU_LMS2 0x520008
 #define          F0297J_EQU_LMS1 0x520004
 #define          F0297J_WBAGC_IT 0x520002
 #define          F0297J_MPEGB 0x520001

/* _INTER_3 */
 #define R0297J_INTER_3 0x53
 #define          F0297J_UNCORRB 0x530080
 #define          F0297J_SYNCQ 0x530040
 #define          F0297J_SYNCI 0x530020
 #define          F0297J_ENFADDET_IT 0x530010
 #define          F0297J_ENCRL_UL_IT 0x530008
 #define          F0297J_UPDATE_READY 0x530004
 #define          F0297J_END_FRAME_HEADER 0x530002
 #define          F0297J_CONTCNT_EVENT 0x530001

/* _SIG_FAD_0 */
 #define R0297J_SIG_FAD_0 0x58
 #define        F0297J_MAGMEAN_LENGHT 0x58001e
 #define        F0297J_ENFADDET 0x580001

/* _SIG_FAD_1 */
 #define R0297J_SIG_FAD_1 0x59
 #define        F0297J_MAGMEAN 0x5900ff

/* _SIG_FAD_2 */
 #define R0297J_SIG_FAD_2 0x5a
 #define        F0297J_MAGMEAN_TH1 0x5a00ff

/* _SIG_FAD_3 */
 #define R0297J_SIG_FAD_3 0x5b
 #define        F0297J_MAGMEAN_TH2 0x5b00ff

/* _NEW_CRL_0 */
 #define R0297J_NEW_CRL_0 0x5c
 #define        F0297J_GAIN_DIR_BLIND 0x5c001f

/* _NEW_CRL_1 */
 #define R0297J_NEW_CRL_1 0x5d
 #define        F0297J_GAIN_INT_BLIND 0x5d001f

/* _NEW_CRL_2 */
 #define R0297J_NEW_CRL_2 0x5e
 #define        F0297J_GAIN_DIR_LMS1 0x5e001f

/* _NEW_CRL_3 */
 #define R0297J_NEW_CRL_3 0x5f
 #define        F0297J_GAIN_INT_LMS1 0x5f001f

/* _NEW_CRL_4 */
 #define R0297J_NEW_CRL_4 0x60
 #define        F0297J_GAIN_DIR_LMS2 0x60001f

/* _NEW_CRL_5 */
 #define R0297J_NEW_CRL_5 0x61
 #define        F0297J_GAIN_INT_LMS2 0x61001f

/* _NEW_CRL_6 */
 #define R0297J_NEW_CRL_6 0x62
 #define        F0297J_HIGHLOOPGAIN_EN 0x620008
 #define        F0297J_CORRECTIONFLAG_F 0x620006
 #define        F0297J_CRLLOCKEDFLAG 0x620001

/* _FREQ_0 */
 #define R0297J_FREQ_0 0x64
 #define           F0297J_MAXDIST_TH 0x64003f
 #define           F0297J_PHASEOFF_TH_LSB 0x6400c0

/* _FREQ_1 */
 #define R0297J_FREQ_1 0x65
 #define           F0297J_PHASEOFF_TH_MSB 0x6500ff

/* _FREQ_2 */
 #define R0297J_FREQ_2 0x66
 #define           F0297J_MEANLENGTHDOPPLER_BLIND 0x66003f

/* _FREQ_3 */
 #define R0297J_FREQ_3 0x67
 #define           F0297J_MEANLENGTHDOPPLER_LMS1 0x67003f

/* _FREQ_4 */
 #define R0297J_FREQ_4 0x68
 #define           F0297J_MEANLENGTHDOPPLER_LMS2 0x68003f

/* _FREQ_5 */
 #define R0297J_FREQ_5 0x69
 #define           F0297J_CORNETPTS_LOW_TH_LSB 0x6900ff

/* _FREQ_6 */
 #define R0297J_FREQ_6 0x6a
 #define           F0297J_CORNETPTS_LOW_TH_MSB 0x6a0003
 #define           F0297J_STD_EST_CRL_SHIFT 0x6a003c
 #define           F0297J_CORNETPTS_HIGH_TH_LSB 0x6a00c0

/* _FREQ_7 */
 #define R0297J_FREQ_7 0x6b
 #define           F0297J_CORNETPTS_HIGH_TH_MSB 0x6b00ff

/* _FREQ_8 */
 #define R0297J_FREQ_8 0x6c
 #define           F0297J_FREQESTCURRENT_0 0x6c00ff

/* _FREQ_9 */
 #define R0297J_FREQ_9 0x6d
 #define           F0297J_FREQESTCURRENT_1 0x6d00ff

/* _FREQ_10 */
 #define R0297J_FREQ_10 0x6e
 #define          F0297J_FREQESTCURRENT_2 0x6e00ff

/* _FREQ_11 */
 #define R0297J_FREQ_11 0x6f
 #define          F0297J_NSTDEST_FREQEST 0x6f00f0
 #define          F0297J_NSTDEST_INTPATH 0x6f000f

/* _FREQ_12 */
 #define R0297J_FREQ_12 0x70
 #define          F0297J_STDEST_TH1_0 0x7000ff

/* _FREQ_13 */
 #define R0297J_FREQ_13 0x71
 #define          F0297J_STDEST_TH1_1 0x7100ff

/* _FREQ_14 */
 #define R0297J_FREQ_14 0x72
 #define          F0297J_STDEST_TH1_2 0x7200ff

/* _FREQ_15 */
 #define R0297J_FREQ_15 0x73
 #define          F0297J_CRL_UL_F 0x7300c0
 #define          F0297J_FREQESTCORR_BLIND_EN 0x730020
 #define          F0297J_FREQESTCORR_LMS1_EN 0x730010
 #define          F0297J_FREQESTCORR_LMS2_EN 0x730008
 #define          F0297J_FREQ_OFFSET 0x730002
 #define          F0297J_RESET_FREQEST 0x730001

/* _FREQ_16 */
 #define R0297J_FREQ_16 0x74
 #define          F0297J_STDEST_TH2_0 0x7400ff

/* _FREQ_17 */
 #define R0297J_FREQ_17 0x75
 #define          F0297J_STDEST_TH2_1 0x7500ff

/* _FREQ_18 */
 #define R0297J_FREQ_18 0x76
 #define          F0297J_STDEST_TH2_2 0x7600ff

/* _FREQ_19 */
 #define R0297J_FREQ_19 0x77
 #define          F0297J_FFE_BLIND_IN_UNLOCKED 0x770070
 #define          F0297J_FFE_BLIND_IN_LOCKED 0x77000e
 #define          F0297J_EQ_UP_CRL_UL 0x770001

/* _FREQ_20 */
 #define R0297J_FREQ_20 0x78
 #define          F0297J_STDEST_CURRENT_0 0x7800ff

/* _FREQ_21 */
 #define R0297J_FREQ_21 0x79
 #define          F0297J_STDEST_CURRENT_1 0x7900ff

/* _FREQ_22 */
 #define R0297J_FREQ_22 0x7a
 #define          F0297J_STDEST_CURRENT_2 0x7a00ff

/* _FREQ_23 */
 #define R0297J_FREQ_23 0x7b
 #define          F0297J_FFE_LMS1_IN_UNLOCKED 0x7b00e0
 #define          F0297J_FFE_LMS1_IN_LOCKED 0x7b001c

/* _FREQ_24 */
 #define R0297J_FREQ_24 0x7c
 #define          F0297J_FFE_LMS2_IN_UNLOCKED 0x7c00e0
 #define          F0297J_FFE_LMS2_IN_LOCKED 0x7c001c

/* _DEINT_SYNC_0 */
 #define R0297J_DEINT_SYNC_0 0x80
 #define     F0297J_DI_UNLOCK 0x800080
 #define     F0297J_DI_FREEZE 0x800040
 #define     F0297J_MISMATCH 0x800030
 #define     F0297J_ACQ_MODE 0x80000c
 #define     F0297J_TRKMODE 0x800003

/* _DEINT_SYNC_1 */
 #define R0297J_DEINT_SYNC_1 0x81
 #define     F0297J_SYNLOST 0x810020
 #define     F0297J_SMCNTR 0x81001c
 #define     F0297J_SYNCSTATE_BIS 0x810003

/* _BERT_0 */
 #define R0297J_BERT_0 0x84
 #define           F0297J_BERT_ON 0x840080
 #define           F0297J_ERR_SOURCE 0x840010
 #define           F0297J_ERR_MODE 0x840008
 #define           F0297J_NBYTE 0x840007

/* _BERT_1 */
 #define R0297J_BERT_1 0x85
 #define           F0297J_ERRCOUNT_LO 0x8500ff

/* _BERT_2 */
 #define R0297J_BERT_2 0x86
 #define           F0297J_ERRCOUNT_HI 0x8600ff

/* _DEINT_0 */
 #define R0297J_DEINT_0 0x88
 #define          F0297J_DEINT_MEMORY 0x880080
 #define          F0297J_DAVIC 0x880040
 #define          F0297J_M 0x88001f

/* _DEINT_1 */
 #define R0297J_DEINT_1 0x89
 #define          F0297J_DEPTH 0x8900ff

/* _OUTFORMAT_0 */
 #define R0297J_OUTFORMAT_0 0x8c
 #define      F0297J_TSMF_EN 0x8c0080
 #define      F0297J_REFRESH47 0x8c0040
 #define      F0297J_BE_BYPASS 0x8c0020
 #define      F0297J_CKOUTPAR 0x8c0010
 #define      F0297J_CT_NBST 0x8c0008
 #define      F0297J_S_NP 0x8c0004
 #define      F0297J_TEI_ENA 0x8c0002
 #define      F0297J_DS_ENA 0x8c0001

/* _OUTFORMAT_1 */
 #define R0297J_OUTFORMAT_1 0x90
 #define      F0297J_SYNC_STRIP 0x900080
 #define      F0297J_CI_EN 0x900040
 #define      F0297J_CICLK_POL 0x900020
 #define      F0297J_TS_SWAP 0x900010

/* _OUTFORMAT_2 */
 #define R0297J_OUTFORMAT_2 0x91
 #define      F0297J_CI_DIVRANGE 0x9100ff

/* _RS_DESC_0 */
 #define R0297J_RS_DESC_0 0x94
 #define        F0297J_BK_CT_LO 0x9400ff

/* _RS_DESC_1 */
 #define R0297J_RS_DESC_1 0x95
 #define        F0297J_BK_CT_HI 0x9500ff

/* _RS_DESC_2 */
 #define R0297J_RS_DESC_2 0x96
 #define        F0297J_CORR_CT_LO 0x9600ff

/* _RS_DESC_3 */
 #define R0297J_RS_DESC_3 0x97
 #define        F0297J_CORR_CT_HI 0x9700ff

/* _RS_DESC_4 */
 #define R0297J_RS_DESC_4 0x98
 #define        F0297J_UNCORR_CT_LO 0x9800ff

/* _RS_DESC_5 */
 #define R0297J_RS_DESC_5 0x99
 #define        F0297J_UNCORR_CT_HI 0x9900ff

/* _RS_DESC_6 */
 #define R0297J_RS_DESC_6 0x9a
 #define        F0297J_SYNCSTATE 0x9a0080

/* _RS_DESC_7 */
 #define R0297J_RS_DESC_7 0x9c
 #define        F0297J_DIS_UNLOCK 0x9c0004
 #define        F0297J_MODE 0x9c0003

/* _RS_DESC_8 */
 #define R0297J_RS_DESC_8 0x9d
 #define        F0297J_RS_NOCORR 0x9d0004
 #define        F0297J_CT_HOLD 0x9d0002
 #define        F0297J_CT_CLEAR 0x9d0001

/* _CTRL_0 */
 #define R0297J_CTRL_0 0xa0
 #define           F0297J_FEC_AC_OR_B 0xa00080
 #define           F0297J_PAGE_MODE 0xa00040
 #define           F0297J_IRQ_CLEAR 0xa00002
 #define           F0297J_SOFT_RESET 0xa00001

/* _CTRL_1 */
 #define R0297J_CTRL_1 0xa1
 #define           F0297J_RESET_EQL 0xa10020
 #define           F0297J_RESET_RS 0xa10010
 #define           F0297J_RESET_PMFAGC 0xa10008
 #define           F0297J_RESET_STL 0xa10004
 #define           F0297J_RESET_CRL 0xa10002
 #define           F0297J_RESET_DI 0xa10001

/* _CTRL_2 */
 #define R0297J_CTRL_2 0xa2
 #define           F0297J_IT_OD 0xa20080
 #define           F0297J_LOCK_OD 0xa20040
 #define           F0297J_AGC_OD 0xa20020
 #define           F0297J_M_OEN 0xa20010
 #define           F0297J_IT_STATUS 0xa20008
 #define           F0297J_M_ERR_STATUS 0xa20004
 #define           F0297J_OSCIBYPASS 0xa20002

/* _CTRL_3 */
 #define R0297J_CTRL_3 0xa3
 #define           F0297J_LOCKPOL 0xa30080
 #define           F0297J_DI_SY_MASK 0xa30040
 #define           F0297J_DI_SY_EV 0xa30020
 #define           F0297J_DI_SY_DIR 0xa30010
 #define           F0297J_SYNC_MSK 0xa30004
 #define           F0297J_SYNC_EV 0xa30002
 #define           F0297J_SYNC_DIR 0xa30001

/* _CTRL_4 */
 #define R0297J_CTRL_4 0xa6
 #define           F0297J_I2CT_EN 0xa60080
 #define           F0297J_LOCKSCE 0xa60060
 #define           F0297J_PRGCLKDIV 0xa6001e
 #define           F0297J_AUXCLKSEL 0xa60001

/* _CTRL_5 */
 #define R0297J_CTRL_5 0xa8
 #define           F0297J_SPEC_INVERSION 0xa80008
 #define           F0297J_GMAP_SEL 0xa80004
 #define           F0297J_DFS 0xa80002

/* _CTRL_6 */
 #define R0297J_CTRL_6 0xa9
 #define           F0297J_SIGMA_INV_1 0xa90020
 #define           F0297J_SIGMA_INV_2 0xa90010

/* _CTRL_7 */
 #define R0297J_CTRL_7 0xaa
 #define           F0297J_AUTO_QAMMODE_SEL 0xaa0080
 #define           F0297J_AUTOCONSTEL_TIMER 0xaa0078
 #define           F0297J_AUTOSTOP_CONSTEL 0xaa0004
 #define           F0297J_AUTOCONSTEL_ON 0xaa0002

/* _CTRL_8 */
 #define R0297J_CTRL_8 0xab
 #define           F0297J_TSMF_HEADER 0xab0080
 #define           F0297J_TSMF_ERROR 0xab0040
 #define           F0297J_TSMF_LOCK 0xab0020
 #define           F0297J_MPEG_LOCK_FECA 0xab0010
 #define           F0297J_MPEG_LOCK_FECB 0xab0008
 #define           F0297J_LMS2_LOCK 0xab0004
 #define           F0297J_LMS1_LOCK 0xab0002
 #define           F0297J_WAGC_LOCK 0xab0001

/* _CTRL_9 */
 #define R0297J_CTRL_9 0xac
 #define           F0297J_TESTSEL 0xac00ff

/* _CTRL_10 */
 #define R0297J_CTRL_10 0xad
 #define          F0297J_SOURCESEL 0xad0080
 #define          F0297J_DC_LEVEL_BYPASS 0xad0040
 #define          F0297J_SYNTH_DISABLE 0xad0020
 #define          F0297J_COARE_FS_SEL 0xad001f

/* _CTRL_11 */
 #define R0297J_CTRL_11 0xae
 #define          F0297J_FINE_FS_SEL_LO 0xae00ff

/* _CTRL_12 */
 #define R0297J_CTRL_12 0xaf
 #define          F0297J_FINE_FS_SEL_HI 0xaf00ff

/* _MPEG_CTRL */
 #define R0297J_MPEG_CTRL 0xc0
 #define        F0297J_MODE_64_256 0xc00080
 #define        F0297J_PARAM_DIS 0xc00040
 #define        F0297J_MPEG_DIS 0xc00020
 #define        F0297J_RS_EN 0xc00010
 #define        F0297J_VIT_RESET 0xc00008
 #define        F0297J_MPEG_HDR_DIS 0xc00004
 #define        F0297J_RS_FLAG 0xc00002
 #define        F0297J_MPEG_FLAG 0xc00001

/* _MPEG_SYNC_ACQ */
 #define R0297J_MPEG_SYNC_ACQ 0xc1
 #define    F0297J_MPEG_GET 0xc1000f

/* _MPEG_SYNC_LOSS */
 #define R0297J_MPEG_SYNC_LOSS 0xc2
 #define   F0297J_MPEG_DROP 0xc2003f

/* _DATA_OUT_CTRL */
 #define R0297J_DATA_OUT_CTRL 0xc3
 #define    F0297J_STI5518 0xc30004
 #define    F0297J_OUTPUT_CLK_POL 0xc30002
 #define    F0297J_OUTPUT_DATA_MODE 0xc30001

/* _VIT_SYNC_ACQ */
 #define R0297J_VIT_SYNC_ACQ 0xc4
 #define     F0297J_VIT_SYNC_GET 0xc400ff

/* _VIT_SYNC_LOSS */
 #define R0297J_VIT_SYNC_LOSS 0xc5
 #define    F0297J_VIT_SYNC_DROP 0xc500ff

/* _VIT_SYNC_GO */
 #define R0297J_VIT_SYNC_GO 0xc6
 #define      F0297J_VIT_SYNC_BEGIN 0xc6003f

/* _VIT_SYNC_STOP */
 #define R0297J_VIT_SYNC_STOP 0xc7
 #define    F0297J_VIT_SYNC_END 0xc7003f

/* _FS_SYNC */
 #define R0297J_FS_SYNC 0xc8
 #define          F0297J_FRAME_SYNC_DROP 0xc800f0
 #define          F0297J_FRAME_SYNC_GET 0xc8000f

/* _IN_DEPTH */
 #define R0297J_IN_DEPTH 0xc9
 #define         F0297J_FEC_RESET 0xc90080
 #define         F0297J_INT_DEPTH 0xc9000f

/* _RS_CONTROL */
 #define R0297J_RS_CONTROL 0xca
 #define       F0297J_RS_CORRECTED_CNT_CLEAR 0xca0080
 #define       F0297J_RS_UNERRORED_CNT_CLEAR 0xca0040
 #define       F0297J_RS_4_ERROR 0xca0020
 #define       F0297J_RS_CLR_ERR 0xca0010
 #define       F0297J_RS_CLR_UNC 0xca0008
 #define       F0297J_RS_RATE_ADJ 0xca0007

/* _DEINT_CONTROL */
 #define R0297J_DEINT_CONTROL 0xcb
 #define    F0297J_DEINT_DEPTH_AUTO_MODE 0xcb0040
 #define    F0297J_FRAME_SYNC_COUNT 0xcb003f

/* _SYNC_STAT */
 #define R0297J_SYNC_STAT 0xcc
 #define        F0297J_MPEG_SYNC 0xcc0010
 #define        F0297J_VIT_I_SYNC 0xcc0008
 #define        F0297J_VIT_Q_SYNC 0xcc0004
 #define        F0297J_COMB_STATE 0xcc0003

/* _VIT_I_RATE */
 #define R0297J_VIT_I_RATE 0xcd
 #define       F0297J_VIT_RATE_I 0xcd00ff

/* _VIT_Q_RATE */
 #define R0297J_VIT_Q_RATE 0xce
 #define       F0297J_VIT_RATE_Q 0xce00ff

/* _TX_INTERLEAVER */
 #define R0297J_TX_INTERLEAVER 0xd0
 #define   F0297J_TX_INT_DEPTH 0xd0000f

/* _RS_ERR_CNT_0 */
 #define R0297J_RS_ERR_CNT_0 0xd1
 #define     F0297J_RS_ERR_CNT_LO 0xd100ff

/* _RS_ERR_CNT_1 */
 #define R0297J_RS_ERR_CNT_1 0xd2
 #define     F0297J_RS_ERR_CNT_HI 0xd2000f

/* _RS_UNC_CNT_0 */
 #define R0297J_RS_UNC_CNT_0 0xd4
 #define     F0297J_RS_UNC_CNT_LO 0xd400ff

/* _RS_UNC_CNT_1 */
 #define R0297J_RS_UNC_CNT_1 0xd5
 #define     F0297J_RS_UNC_CNT_HI 0xd5000f

/* _RS_RATE_0 */
 #define R0297J_RS_RATE_0 0xd6
 #define        F0297J_RS_RATE_LO 0xd600ff

/* _RS_RATE_1 */
 #define R0297J_RS_RATE_1 0xd7
 #define        F0297J_RS_RATE_HI 0xd70003

/* _RS_CORR_CNT_0 */
 #define R0297J_RS_CORR_CNT_0 0xd8
 #define    F0297J_RS_CORR_CNT_LO 0xd800ff

/* _RS_CORR_CNT_1 */
 #define R0297J_RS_CORR_CNT_1 0xd9
 #define    F0297J_RS_CORR_CNT_HI 0xd900ff

/* _RS_UNERR_CNT_0 */
 #define R0297J_RS_UNERR_CNT_0 0xda
 #define   F0297J_RS_UNERR_CNT_LO 0xda00ff

/* _RS_UNERR_CNT_1 */
 #define R0297J_RS_UNERR_CNT_1 0xdb
 #define   F0297J_RS_UNERR_CNT_HI 0xdb00ff

/* _TSMF_SEL */
 #define R0297J_TSMF_SEL 0xb1
 #define         F0297J_TS_NUMBER 0xb1001e
 #define         F0297J_SEL_MODE 0xb10001

/* _TSMF_CTRL */
 #define R0297J_TSMF_CTRL 0xb2
 #define        F0297J_CHECK_ERROR_BIT 0xb20080
 #define        F0297J_CHECK_F_SYNC 0xb20040
 #define        F0297J_H_MODE 0xb20008
 #define        F0297J_D_V_MODE 0xb20004
 #define        F0297J_TS_MODE 0xb20003

/* _AUTOMATIC */
 #define R0297J_AUTOMATIC 0xb3
 #define        F0297J_SYNC_IN_COUNT 0xb300f0
 #define        F0297J_SYNC_OUT_COUNT 0xb3000f

/* _TS_ID_L */
 #define R0297J_TS_ID_L 0xb4
 #define          F0297J_TS_ID_LSB 0xb400ff

/* _TS_ID_H */
 #define R0297J_TS_ID_H 0xb5
 #define          F0297J_TS_ID_MSB 0xb500ff

/* _ON_ID_L */
 #define R0297J_ON_ID_L 0xb6
 #define          F0297J_ON_ID_LSB 0xb600ff

/* _ON_ID_H */
 #define R0297J_ON_ID_H 0xb7
 #define          F0297J_ON_ID_MSB 0xb700ff

/* _GAL_STATUS */
 #define R0297J_GAL_STATUS 0xb9
 #define       F0297J_ERROR 0xb90080
 #define       F0297J_EMERGENCY 0xb90040
 #define       F0297J_CRE_TS 0xb90030
 #define       F0297J_VER 0xb9000e
 #define       F0297J_M_LOCK 0xb90001

/* _TS_ST_L */
 #define R0297J_TS_ST_L 0xba
 #define          F0297J_TS_STATUS_LSB 0xba00ff

/* _TS_ST_H */
 #define R0297J_TS_ST_H 0xbb
 #define          F0297J_TS_STATUS_MSB 0xbb007f

/* _RE_ST_L */
 #define R0297J_RE_ST_L 0xbc
 #define          F0297J_RX_STATUS_0 0xbc00ff

/* _RE_ST_2 */
 #define R0297J_RE_ST_2 0xbd
 #define          F0297J_RX_STATUS_1 0xbd00ff

/* _RE_ST_1 */
 #define R0297J_RE_ST_1 0xbe
 #define          F0297J_RX_STATUS_2 0xbe00ff

/* _RE_ST_H */
 #define R0297J_RE_ST_H 0xbf
 #define          F0297J_RX_STATUS_3 0xbf003f

/* _T_ID_L1 */
 #define R0297J_T_ID_L1 0xc0
 #define          F0297J_TS_ID_1_LSB 0xc000ff

/* _T_ID_H1 */
 #define R0297J_T_ID_H1 0xc1
 #define          F0297J_TS_ID_1_MSB 0xc100ff

/* _O_ID_L1 */
 #define R0297J_O_ID_L1 0xc2
 #define          F0297J_ON_ID_1_LSB 0xc200ff

/* _O_ID_H1 */
 #define R0297J_O_ID_H1 0xc3
 #define          F0297J_ON_ID_1_MSB 0xc300ff

/* _T_ID_L2 */
 #define R0297J_T_ID_L2 0xc4
 #define          F0297J_TS_ID_2_LSB 0xc400ff

/* _T_ID_H2 */
 #define R0297J_T_ID_H2 0xc5
 #define          F0297J_TS_ID_2_MSB 0xc500ff

/* _O_ID_L2 */
 #define R0297J_O_ID_L2 0xc6
 #define          F0297J_ON_ID_2_LSB 0xc600ff

/* _O_ID_H2 */
 #define R0297J_O_ID_H2 0xc7
 #define          F0297J_ON_ID_2_MSB 0xc700ff

/* _T_ID_L3 */
 #define R0297J_T_ID_L3 0xc8
 #define          F0297J_TS_ID_3_LSB 0xc800ff

/* _T_ID_H3 */
 #define R0297J_T_ID_H3 0xc9
 #define          F0297J_TS_ID_3_MSB 0xc900ff

/* _O_ID_L3 */
 #define R0297J_O_ID_L3 0xca
 #define          F0297J_ON_ID_3_LSB 0xca00ff

/* _O_ID_H3 */
 #define R0297J_O_ID_H3 0xcb
 #define          F0297J_ON_ID_3_MSB 0xcb00ff

/* _T_ID_L4 */
 #define R0297J_T_ID_L4 0xcc
 #define          F0297J_TS_ID_4_LSB 0xcc00ff

/* _T_ID_H4 */
 #define R0297J_T_ID_H4 0xcd
 #define          F0297J_TS_ID_4_MSB 0xcd00ff

/* _O_ID_L4 */
 #define R0297J_O_ID_L4 0xce
 #define          F0297J_ON_ID_4_LSB 0xce00ff

/* _O_ID_H4 */
 #define R0297J_O_ID_H4 0xcf
 #define          F0297J_ON_ID_4_MSB 0xcf00ff

/* _T_ID_L5 */
 #define R0297J_T_ID_L5 0xd0
 #define          F0297J_TS_ID_5_LSB 0xd000ff

/* _T_ID_H5 */
 #define R0297J_T_ID_H5 0xd1
 #define          F0297J_TS_ID_5_MSB 0xd100ff

/* _O_ID_L5 */
 #define R0297J_O_ID_L5 0xd2
 #define          F0297J_ON_ID_5_LSB 0xd200ff

/* _O_ID_H5 */
 #define R0297J_O_ID_H5 0xd3
 #define          F0297J_ON_ID_5_MSB 0xd300ff

/* _T_ID_L6 */
 #define R0297J_T_ID_L6 0xd4
 #define          F0297J_TS_ID_6_LSB 0xd400ff

/* _T_ID_H6 */
 #define R0297J_T_ID_H6 0xd5
 #define          F0297J_TS_ID_6_MSB 0xd500ff

/* _O_ID_L6 */
 #define R0297J_O_ID_L6 0xd6
 #define          F0297J_ON_ID_6_LSB 0xd600ff

/* _O_ID_H6 */
 #define R0297J_O_ID_H6 0xd7
 #define          F0297J_ON_ID_6_MSB 0xd700ff

/* _T_ID_L7 */
 #define R0297J_T_ID_L7 0xd8
 #define          F0297J_TS_ID_7_LSB 0xd800ff

/* _T_ID_H7 */
 #define R0297J_T_ID_H7 0xd9
 #define          F0297J_TS_ID_7_MSB 0xd900ff

/* _O_ID_L7 */
 #define R0297J_O_ID_L7 0xda
 #define          F0297J_ON_ID_7_LSB 0xda00ff

/* _O_ID_H7 */
 #define R0297J_O_ID_H7 0xdb
 #define          F0297J_ON_ID_7_MSB 0xdb00ff

/* _T_ID_L8 */
 #define R0297J_T_ID_L8 0xdc
 #define          F0297J_TS_ID_8_LSB 0xdc00ff

/* _T_ID_H8 */
 #define R0297J_T_ID_H8 0xdd
 #define          F0297J_TS_ID_8_MSB 0xdd00ff

/* _O_ID_L8 */
 #define R0297J_O_ID_L8 0xde
 #define          F0297J_ON_ID_8_LSB 0xde00ff

/* _O_ID_H8 */
 #define R0297J_O_ID_H8 0xdf
 #define          F0297J_ON_ID_8_MSB 0xdf00ff

/* _T_ID_L9 */
 #define R0297J_T_ID_L9 0xe0
 #define          F0297J_TS_ID_9_LSB 0xe000ff

/* _T_ID_H9 */
 #define R0297J_T_ID_H9 0xe1
 #define          F0297J_TS_ID_9_MSB 0xe100ff

/* _O_ID_L9 */
 #define R0297J_O_ID_L9 0xe2
 #define          F0297J_ON_ID_9_LSB 0xe200ff

/* _O_ID_H9 */
 #define R0297J_O_ID_H9 0xe3
 #define          F0297J_ON_ID_9_MSB 0xe300ff

/* _T_ID_L10 */
 #define R0297J_T_ID_L10 0xe4
 #define         F0297J_TS_ID_10_LSB 0xe400ff

/* _T_ID_H10 */
 #define R0297J_T_ID_H10 0xe5
 #define         F0297J_TS_ID_10_MSB 0xe500ff

/* _O_ID_L10 */
 #define R0297J_O_ID_L10 0xe6
 #define         F0297J_ON_ID_10_LSB 0xe600ff

/* _O_ID_H10 */
 #define R0297J_O_ID_H10 0xe7
 #define         F0297J_ON_ID_10_MSB 0xe700ff

/* _T_ID_L11 */
 #define R0297J_T_ID_L11 0xe8
 #define         F0297J_TS_ID_11_LSB 0xe800ff

/* _T_ID_H11 */
 #define R0297J_T_ID_H11 0xe9
 #define         F0297J_TS_ID_11_MSB 0xe900ff

/* _O_ID_L11 */
 #define R0297J_O_ID_L11 0xea
 #define         F0297J_ON_ID_11_LSB 0xea00ff

/* _O_ID_H11 */
 #define R0297J_O_ID_H11 0xeb
 #define         F0297J_ON_ID_11_MSB 0xeb00ff

/* _T_ID_L12 */
 #define R0297J_T_ID_L12 0xec
 #define         F0297J_TS_ID_12_LSB 0xec00ff

/* _T_ID_H12 */
 #define R0297J_T_ID_H12 0xed
 #define         F0297J_TS_ID_12_MSB 0xed00ff

/* _O_ID_L12 */
 #define R0297J_O_ID_L12 0xee
 #define         F0297J_ON_ID_12_LSB 0xee00ff

/* _O_ID_H12 */
 #define R0297J_O_ID_H12 0xef
 #define         F0297J_ON_ID_12_MSB 0xef00ff

/* _T_ID_L13 */
 #define R0297J_T_ID_L13 0xf0
 #define         F0297J_TS_ID_13_LSB 0xf000ff

/* _T_ID_H13 */
 #define R0297J_T_ID_H13 0xf1
 #define         F0297J_TS_ID_13_MSB 0xf100ff

/* _O_ID_L13 */
 #define R0297J_O_ID_L13 0xf2
 #define         F0297J_ON_ID_13_LSB 0xf200ff

/* _O_ID_H13 */
 #define R0297J_O_ID_H13 0xf3
 #define         F0297J_ON_ID_13_MSB 0xf300ff

/* _T_ID_L14 */
 #define R0297J_T_ID_L14 0xf4
 #define         F0297J_TS_ID_14_LSB 0xf400ff

/* _T_ID_H14 */
 #define R0297J_T_ID_H14 0xf5
 #define         F0297J_TS_ID_14_MSB 0xf500ff

/* _O_ID_L14 */
 #define R0297J_O_ID_L14 0xf6
 #define         F0297J_ON_ID_14_LSB 0xf600ff

/* _O_ID_H14 */
 #define R0297J_O_ID_H14 0xf7
 #define         F0297J_ON_ID_14_MSB 0xf700ff

/* _T_ID_L15 */
 #define R0297J_T_ID_L15 0xf8
 #define         F0297J_TS_ID_15_LSB 0xf800ff

/* _T_ID_H15 */
 #define R0297J_T_ID_H15 0xf9
 #define         F0297J_TS_ID_15_MSB 0xf900ff

/* _O_ID_L15 */
 #define R0297J_O_ID_L15 0xfa
 #define         F0297J_ON_ID_15_LSB 0xfa00ff

/* _O_ID_H15 */
 #define R0297J_O_ID_H15 0xfb
 #define         F0297J_ON_ID_15_MSB 0xfb00ff
/* defines ----------------------------------------------------------------- */

/* Modulations */
#define QAM16                                0
#define QAM32                                1
#define QAM64	                             4
#define QAM128                               2
#define QAM256	                             3

/* MACRO definitions */
#define MAC0297J_ABS(X)                      ((X)<0 ?   (-X) : (X))
#define MAC0297J_MAX(X,Y)                    ((X)>=(Y) ? (X) : (Y))
#define MAC0297J_MIN(X,Y)                    ((X)<=(Y) ? (X) : (Y))
#define MAC0297J_MAKEWORD(X,Y)               ((X<<8)+(Y))
#define MAC0297J_B0(X)                       ((X & 0xFF))
#define MAC0297J_B1(Y)                       ((Y>>8)& 0xFF)
#define MAC0297J_B2(Y)                       ((Y>>16)& 0xFF)
#define MAC0297J_B3(Y)                       ((Y>>24)& 0xFF)

#define MAC0297J_B0N0(X)                     ((X & 0x0F))
#define MAC0297J_B1N0(Y)                     ((Y>>8)& 0x0F)
#define MAC0297J_B2N0(Y)                     ((Y>>16)& 0x0F)
#define MAC0297J_B3N0(Y)                     ((Y>>24)& 0x0F)

#define MAC0297J_B0N1(X)                     ((X & 0xF0))
#define MAC0297J_B1N1(Y)                     ((Y>>8)& 0xF0)
#define MAC0297J_B2N1(Y)                     ((Y>>16)& 0xF0)
#define MAC0297J_B3N1(Y)                     ((Y>>24)& 0xF0)

/* public types ------------------------------------------------------------ */



/* functions --------------------------------------------------------------- */


ST_ErrorCode_t          Reg0297J_Install             (STTUNER_IOREG_DeviceMap_t *DeviceMap);
ST_ErrorCode_t          Reg0297J_Open                (STTUNER_IOREG_DeviceMap_t *DeviceMap, U32 ExternalClock);
ST_ErrorCode_t          Reg0297J_Close               (STTUNER_IOREG_DeviceMap_t *DeviceMap);
ST_ErrorCode_t          Reg0297J_UnInstall           (STTUNER_IOREG_DeviceMap_t *DeviceMap);

/******** EQUALIZER ***************/
void                    Reg0297J_SetQAMSize         (STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, STTUNER_Modulation_t Value);
STTUNER_Modulation_t    Reg0297J_GetQAMSize         (STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle);
/********** WBAGC functions********/
void                    Reg0297J_SetWBAGCloop       (STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, unsigned short loop);
U16                     Reg0297J_GetWBAGCloop       (STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle);
void                    Reg0297J_SetAGC             (STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, U16 Value);
U16                     Reg0297J_GetAGC             (STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle);
/*********** Gle ******************/
void                    Reg0297J_STV0297Jreset      (STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle);
/********** Functions *************/
unsigned long	        Reg0297J_GetSymbolRate      (STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle);
void	                Reg0297J_SetSymbolRate      (STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, unsigned long _SymbolRate);
void                    Reg0297J_SetSpectrumInversion(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, STTUNER_Spectrum_t SpectrumInversion);
void                    Reg0297J_SetSweepRate       (STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, short _FShift);
void	                Reg0297J_SetFrequencyOffset (STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, long _CarrierOffset);
/********** ERR functions *********/
void                    Reg0297J_StartBlkCounter    (STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle);
void                    Reg0297J_StopBlkCounter     (STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle);
unsigned short	        Reg0297J_GetBlkCounter      (STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle);
unsigned short	        Reg0297J_GetCorrBlk         (STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle);
unsigned short	        Reg0297J_GetUncorrBlk       (STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle);
U8                      Reg0297J_GetSTV0297JId      (STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle);

#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif                          /* __STTUNER_DEMOD_R0297J_H */

/* End of reg0297J.h */
