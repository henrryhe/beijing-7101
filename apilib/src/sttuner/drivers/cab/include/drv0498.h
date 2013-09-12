#ifndef __STTUNER_CAB_DRV0498_H
#define __STTUNER_CAB_DRV0498_H

#ifdef __cplusplus
extern "C"
{
#endif                          /* __cplusplus */

/* includes ---------------------------------------------------------------- */

#include "sttuner.h"
#include "tcdrv.h"

/*MPX mapped addresses*/
#define QAM0_BASE_ADDR  0xa3fff000
#define QAM1_BASE_ADDR  0xa3c00000
#define QAM2_BASE_ADDR  0xa3c01000

/*actual base addresses on 498*/
/*#define QAM0_BASE_ADDR  0xfd0ff000
#define QAM1_BASE_ADDR  0xfe000000
#define QAM2_BASE_ADDR  0xfe001000*/
 
#define QAM_BASE_ADDR QAM2_BASE_ADDR

#define F498_EQU_IQ_addr			(QAM_BASE_ADDR + 0x0080)
#define F498_EQU_IQ_size			32
#define F498_EQU_IQ_offset			0x00
#define F498_EQU_IQ_def			 F498_EQU_IQ_addr, F498_EQU_IQ_size, F498_EQU_IQ_offset


#define F498_STANDBY_addr			(QAM_BASE_ADDR + 0x0000)
#define F498_SOFT_RST_addr			(QAM_BASE_ADDR + 0x0000)
#define F498_EQU_RST_addr			(QAM_BASE_ADDR + 0x0000)
#define F498_CRL_RST_addr			(QAM_BASE_ADDR + 0x0000)
#define F498_TRL_RST_addr			(QAM_BASE_ADDR + 0x0000)
#define F498_AGC_RST_addr			(QAM_BASE_ADDR + 0x0000)
#define F498_FEC_NHW_RESET_addr		(QAM_BASE_ADDR + 0x0000)
#define F498_DEINT_RST_addr			(QAM_BASE_ADDR + 0x0000)
#define F498_FECAC_RS_RST_addr			(QAM_BASE_ADDR + 0x0000)
#define F498_FECB_RS_RST_addr			(QAM_BASE_ADDR + 0x0000)
#define F498_VITERBI_RST_addr			(QAM_BASE_ADDR + 0x0000)
#define F498_SWEEP_OUT_addr			(QAM_BASE_ADDR + 0x0008)
#define F498_FSM_CRL_addr			(QAM_BASE_ADDR + 0x0008)
#define F498_CRL_LOCK_addr			(QAM_BASE_ADDR + 0x0008)
#define F498_MFSM_addr				(QAM_BASE_ADDR + 0x0008)
#define F498_TRL_LOCK_addr			(QAM_BASE_ADDR + 0x0008)
#define F498_TRL_AGC_LIMIT_addr		(QAM_BASE_ADDR + 0x0008)
#define F498_ADJ_AGC_LOCK_addr			(QAM_BASE_ADDR + 0x0008)
#define F498_AGC_LOCK_addr			(QAM_BASE_ADDR + 0x0008)
#define F498_TSMF_CNT_addr			(QAM_BASE_ADDR + 0x0008)
#define F498_TSMF_EOF_addr			(QAM_BASE_ADDR + 0x0008)
#define F498_TSMF_RDY_addr			(QAM_BASE_ADDR + 0x0008)
#define F498_FEC_NOCORR_addr			(QAM_BASE_ADDR + 0x0008)
#define F498_FEC_LOCK_addr			(QAM_BASE_ADDR + 0x0008)
#define F498_DEINT_LOCK_addr			(QAM_BASE_ADDR + 0x0008)
#define F498_TAPMON_ALARM_addr			(QAM_BASE_ADDR + 0x0008)
#define F498_SWEEP_OUTE_addr			(QAM_BASE_ADDR + 0x0008)
#define F498_FSM_CRLE_addr			(QAM_BASE_ADDR + 0x0008)
#define F498_CRL_LOCKE_addr			(QAM_BASE_ADDR + 0x0008)
#define F498_MFSME_addr			(QAM_BASE_ADDR + 0x0008)
#define F498_TRL_LOCKE_addr			(QAM_BASE_ADDR + 0x0008)
#define F498_TRL_AGC_LIMITE_addr			(QAM_BASE_ADDR + 0x0008)
#define F498_ADJ_AGC_LOCKE_addr			(QAM_BASE_ADDR + 0x0008)
#define F498_AGC_LOCKE_addr			(QAM_BASE_ADDR + 0x0008)
#define F498_TSMF_CNTE_addr			(QAM_BASE_ADDR + 0x0008)
#define F498_TSMF_EOFE_addr			(QAM_BASE_ADDR + 0x0008)
#define F498_TSMF_RDYE_addr			(QAM_BASE_ADDR + 0x0008)
#define F498_FEC_NOCORRE_addr			(QAM_BASE_ADDR + 0x0008)
#define F498_FEC_LOCKE_addr			(QAM_BASE_ADDR + 0x0008)
#define F498_DEINT_LOCKE_addr			(QAM_BASE_ADDR + 0x0008)
#define F498_TAPMON_ALARME_addr			(QAM_BASE_ADDR + 0x0008)
#define F498_QAMFEC_LOCK_addr			(QAM_BASE_ADDR + 0x000C)
#define F498_TSMF_LOCK_addr			(QAM_BASE_ADDR + 0x000C)
#define F498_TSMF_ERROR_addr			(QAM_BASE_ADDR + 0x000C)
#define F498_TST_BUS_SEL_addr			(QAM_BASE_ADDR + 0x000C)
#define F498_AGC_LCK_TH_addr			(QAM_BASE_ADDR + 0x0010)
#define F498_AGC_ACCUMRSTSEL_addr			(QAM_BASE_ADDR + 0x0010)
#define F498_AGC_IF_BWSEL_addr			(QAM_BASE_ADDR + 0x0010)
#define F498_AGC_IF_FREEZE_addr			(QAM_BASE_ADDR + 0x0010)
#define F498_AGC_RF_BWSEL_addr			(QAM_BASE_ADDR + 0x0010)
#define F498_AGC_RF_FREEZE_addr			(QAM_BASE_ADDR + 0x0010)
#define F498_AGC_RF_PWM_TST_addr			(QAM_BASE_ADDR + 0x0010)
#define F498_AGC_RF_PWM_INV_addr			(QAM_BASE_ADDR + 0x0010)
#define F498_AGC_IF_PWM_TST_addr			(QAM_BASE_ADDR + 0x0010)
#define F498_AGC_IF_PWM_INV_addr			(QAM_BASE_ADDR + 0x0010)
#define F498_AGC_PWM_CLKDIV_addr			(QAM_BASE_ADDR + 0x0010)
#define F498_AGC_PWRREF_LO_addr			(QAM_BASE_ADDR + 0x0014)
#define F498_AGC_PWRREF_HI_addr			(QAM_BASE_ADDR + 0x0014)
#define F498_AGC_RF_TH_LO_addr			(QAM_BASE_ADDR + 0x0014)
#define F498_AGC_RF_TH_HI_addr			(QAM_BASE_ADDR + 0x0014)
#define F498_AGC_IF_THLO_LO_addr			(QAM_BASE_ADDR + 0x0018)
#define F498_AGC_IF_THLO_HI_addr			(QAM_BASE_ADDR + 0x0018)
#define F498_AGC_IF_THHI_LO_addr			(QAM_BASE_ADDR + 0x0018)
#define F498_AGC_IF_THHI_HI_addr			(QAM_BASE_ADDR + 0x0018)
#define F498_AGC_PWR_WORD_LO_addr			(QAM_BASE_ADDR + 0x001C)
#define F498_AGC_PWR_WORD_ME_addr			(QAM_BASE_ADDR + 0x001C)
#define F498_AGC_PWR_WORD_HI_addr			(QAM_BASE_ADDR + 0x001C)
#define F498_AGC_IF_PWMCMD_LO_addr			(QAM_BASE_ADDR + 0x0020)
#define F498_AGC_IF_PWMCMD_HI_addr			(QAM_BASE_ADDR + 0x0020)
#define F498_AGC_RF_PWMCMD_LO_addr			(QAM_BASE_ADDR + 0x0020)
#define F498_AGC_RF_PWMCMD_HI_addr			(QAM_BASE_ADDR + 0x0020)
#define F498_IQDEM_CLK_SEL_addr			(QAM_BASE_ADDR + 0x0024)
#define F498_IQDEM_INVIQ_addr			(QAM_BASE_ADDR + 0x0024)
#define F498_IQDEM_A2DTYPE_addr			(QAM_BASE_ADDR + 0x0024)
#define F498_MIX_NCO_INC_LL_addr			(QAM_BASE_ADDR + 0x0024)
#define F498_MIX_NCO_INC_HL_addr			(QAM_BASE_ADDR + 0x0024)
#define F498_MIX_NCO_INVCNST_addr			(QAM_BASE_ADDR + 0x0024)
#define F498_MIX_NCO_INC_HH_addr			(QAM_BASE_ADDR + 0x0024)
#define F498_SRC_NCO_INC_LL_addr			(QAM_BASE_ADDR + 0x0028)
#define F498_SRC_NCO_INC_LH_addr			(QAM_BASE_ADDR + 0x0028)
#define F498_SRC_NCO_INC_HL_addr			(QAM_BASE_ADDR + 0x0028)
#define F498_SRC_NCO_INC_HH_addr			(QAM_BASE_ADDR + 0x0028)
#define F498_GAIN_SRC_LO_addr			(QAM_BASE_ADDR + 0x002C)
#define F498_GAIN_SRC_HI_addr			(QAM_BASE_ADDR + 0x002C)
#define F498_DCRM0_DCIN_L_addr			(QAM_BASE_ADDR + 0x0030)
#define F498_DCRM1_I_DCIN_L_addr			(QAM_BASE_ADDR + 0x0030)
#define F498_DCRM0_DCIN_H_addr			(QAM_BASE_ADDR + 0x0030)
#define F498_DCRM1_Q_DCIN_L_addr			(QAM_BASE_ADDR + 0x0030)
#define F498_DCRM1_I_DCIN_H_addr			(QAM_BASE_ADDR + 0x0030)
#define F498_DCRM1_FRZ_addr			(QAM_BASE_ADDR + 0x0030)
#define F498_DCRM0_FRZ_addr			(QAM_BASE_ADDR + 0x0030)
#define F498_DCRM1_Q_DCIN_H_addr			(QAM_BASE_ADDR + 0x0030)
#define F498_ADJIIR_COEFF10_L_addr			(QAM_BASE_ADDR + 0x0034)
#define F498_ADJIIR_COEFF11_L_addr			(QAM_BASE_ADDR + 0x0034)
#define F498_ADJIIR_COEFF10_H_addr			(QAM_BASE_ADDR + 0x0034)
#define F498_ADJIIR_COEFF12_L_addr			(QAM_BASE_ADDR + 0x0034)
#define F498_ADJIIR_COEFF11_H_addr			(QAM_BASE_ADDR + 0x0034)
#define F498_ADJIIR_COEFF20_L_addr			(QAM_BASE_ADDR + 0x0034)
#define F498_ADJIIR_COEFF12_H_addr			(QAM_BASE_ADDR + 0x0034)
#define F498_ADJIIR_COEFF20_H_addr			(QAM_BASE_ADDR + 0x0038)
#define F498_ADJIIR_COEFF21_L_addr			(QAM_BASE_ADDR + 0x0038)
#define F498_ADJIIR_COEFF22_L_addr			(QAM_BASE_ADDR + 0x0038)
#define F498_ADJIIR_COEFF21_H_addr			(QAM_BASE_ADDR + 0x0038)
#define F498_ADJIIR_COEFF22_H_addr			(QAM_BASE_ADDR + 0x0038)
#define F498_ALLPASSFILT_EN_addr			(QAM_BASE_ADDR + 0x003C)
#define F498_ADJ_AGC_EN_addr			(QAM_BASE_ADDR + 0x003C)
#define F498_ADJ_COEFF_FRZ_addr			(QAM_BASE_ADDR + 0x003C)
#define F498_ADJ_EN_addr			(QAM_BASE_ADDR + 0x003C)
#define F498_ADJ_AGC_REF_addr			(QAM_BASE_ADDR + 0x003C)
#define F498_ALLPASSFILT_COEFF1_LO_addr			(QAM_BASE_ADDR + 0x0040)
#define F498_ALLPASSFILT_COEFF1_ME_addr			(QAM_BASE_ADDR + 0x0040)
#define F498_ALLPASSFILT_COEFF2_LO_addr			(QAM_BASE_ADDR + 0x0040)
#define F498_ALLPASSFILT_COEFF1_HI_addr			(QAM_BASE_ADDR + 0x0040)
#define F498_ALLPASSFILT_COEFF2_ME_addr			(QAM_BASE_ADDR + 0x0040)
#define F498_ALLPASSFILT_COEFF2_HI_addr			(QAM_BASE_ADDR + 0x0044)
#define F498_ALLPASSFILT_COEFF3_LO_addr			(QAM_BASE_ADDR + 0x0044)
#define F498_ALLPASSFILT_COEFF2_HH_addr			(QAM_BASE_ADDR + 0x0044)
#define F498_ALLPASSFILT_COEFF3_ME_addr			(QAM_BASE_ADDR + 0x0044)
#define F498_ALLPASSFILT_COEFF3_HI_addr			(QAM_BASE_ADDR + 0x0044)
#define F498_ALLPASSFILT_COEFF4_LO_addr			(QAM_BASE_ADDR + 0x0048)
#define F498_ALLPASSFILT_COEFF3_HH_addr			(QAM_BASE_ADDR + 0x0048)
#define F498_ALLPASSFILT_COEFF4_ME_addr			(QAM_BASE_ADDR + 0x0048)
#define F498_ALLPASSFILT_COEFF4_HI_addr			(QAM_BASE_ADDR + 0x0048)
#define F498_TRL_AGC_FREEZE_addr			(QAM_BASE_ADDR + 0x0050)
#define F498_TRL_AGC_REF_addr			(QAM_BASE_ADDR + 0x0050)
#define F498_NYQPOINT_INV_addr			(QAM_BASE_ADDR + 0x0054)
#define F498_TRL_SHIFT_addr			(QAM_BASE_ADDR + 0x0054)
#define F498_NYQ_COEFF_SEL_addr			(QAM_BASE_ADDR + 0x0054)
#define F498_TRL_LPF_FREEZE_addr			(QAM_BASE_ADDR + 0x0054)
#define F498_TRL_LPF_CRT_addr			(QAM_BASE_ADDR + 0x0054)
#define F498_TRL_GDIR_ACQ_addr			(QAM_BASE_ADDR + 0x0054)
#define F498_TRL_GINT_ACQ_addr			(QAM_BASE_ADDR + 0x0054)
#define F498_TRL_GDIR_TRK_addr			(QAM_BASE_ADDR + 0x0054)
#define F498_TRL_GINT_TRK_addr			(QAM_BASE_ADDR + 0x0054)
#define F498_TRL_GAIN_OUT_addr			(QAM_BASE_ADDR + 0x0054)
#define F498_TRL_LCK_THLO_addr			(QAM_BASE_ADDR + 0x0058)
#define F498_TRL_LCK_THHI_addr			(QAM_BASE_ADDR + 0x0058)
#define F498_TRL_LCK_TRG_addr			(QAM_BASE_ADDR + 0x0058)
#define F498_CRL_DFE_addr			(QAM_BASE_ADDR + 0x0060)
#define F498_DFE_START_addr			(QAM_BASE_ADDR + 0x0060)
#define F498_CTRLG_START_addr			(QAM_BASE_ADDR + 0x0060)
#define F498_FSM_FORCESTATE_addr			(QAM_BASE_ADDR + 0x0060)
#define F498_FEC2_EN_addr			(QAM_BASE_ADDR + 0x0060)
#define F498_SIT_EN_addr			(QAM_BASE_ADDR + 0x0060)
#define F498_TRL_AHEAD_addr			(QAM_BASE_ADDR + 0x0060)
#define F498_TRL2_EN_addr			(QAM_BASE_ADDR + 0x0060)
#define F498_FSM_RCA_EN_addr			(QAM_BASE_ADDR + 0x0060)
#define F498_FSM_BKP_DIS_addr			(QAM_BASE_ADDR + 0x0060)
#define F498_FSM_FORCE_EN_addr			(QAM_BASE_ADDR + 0x0060)
#define F498_FSM_STATUS_addr			(QAM_BASE_ADDR + 0x0060)
#define F498_SNR0_HTH_addr			(QAM_BASE_ADDR + 0x0060)
#define F498_SNR1_HTH_addr			(QAM_BASE_ADDR + 0x0064)
#define F498_SNR2_HTH_addr			(QAM_BASE_ADDR + 0x0064)
#define F498_SNR0_LTH_addr			(QAM_BASE_ADDR + 0x0064)
#define F498_SNR1_LTH_addr			(QAM_BASE_ADDR + 0x0064)
#define F498_SNR3_HTH_L_addr			(QAM_BASE_ADDR + 0x0068)
#define F498_RCA_HTH_addr			(QAM_BASE_ADDR + 0x0068)
#define F498_SIT_addr			(QAM_BASE_ADDR + 0x0068)
#define F498_WST_addr			(QAM_BASE_ADDR + 0x0068)
#define F498_ELT_addr			(QAM_BASE_ADDR + 0x0068)
#define F498_SNR3_HTH_H_addr			(QAM_BASE_ADDR + 0x0068)
#define F498_FEC2_DFEOFF_addr			(QAM_BASE_ADDR + 0x0068)
#define F498_PNT_STATE_addr			(QAM_BASE_ADDR + 0x0068)
#define F498_MODMAP_STATE_addr			(QAM_BASE_ADDR + 0x0068)
#define F498_I_TEST_TAP_L_addr			(QAM_BASE_ADDR + 0x0074)
#define F498_I_TEST_TAP_M_addr			(QAM_BASE_ADDR + 0x0074)
#define F498_I_TEST_TAP_H_addr			(QAM_BASE_ADDR + 0x0074)
#define F498_TEST_FFE_DFE_SEL_addr			(QAM_BASE_ADDR + 0x0074)
#define F498_TEST_TAP_SELECT_addr			(QAM_BASE_ADDR + 0x0074)
#define F498_Q_TEST_TAP_L_addr			(QAM_BASE_ADDR + 0x0078)
#define F498_Q_TEST_TAP_M_addr			(QAM_BASE_ADDR + 0x0078)
#define F498_Q_TEST_TAP_H_addr			(QAM_BASE_ADDR + 0x0078)
#define F498_MTAP_FRZ_addr			(QAM_BASE_ADDR + 0x0078)
#define F498_PRE_FREEZE_addr			(QAM_BASE_ADDR + 0x0078)
#define F498_DFE_TAPMON_EN_addr			(QAM_BASE_ADDR + 0x0078)
#define F498_FFE_TAPMON_EN_addr			(QAM_BASE_ADDR + 0x0078)
#define F498_MTAP_ONLY_addr			(QAM_BASE_ADDR + 0x0078)
#define F498_EQU_CTR_CRL_CONTROL_LO_addr			(QAM_BASE_ADDR + 0x007C)
#define F498_EQU_CTR_CRL_CONTROL_HI_addr			(QAM_BASE_ADDR + 0x007C)
#define F498_CTR_HIPOW_L_addr			(QAM_BASE_ADDR + 0x007C)
#define F498_CTR_HIPOW_H_addr			(QAM_BASE_ADDR + 0x007C)
#define F498_EQU_I_EQU_L_addr			(QAM_BASE_ADDR + 0x0080)
#define F498_EQU_I_EQU_H_addr			(QAM_BASE_ADDR + 0x0080)
#define F498_EQU_Q_EQU_L_addr			(QAM_BASE_ADDR + 0x0080)
#define F498_EQU_Q_EQU_H_addr			(QAM_BASE_ADDR + 0x0080)
#define F498_QUAD_AUTO_addr			(QAM_BASE_ADDR + 0x0084)
#define F498_QUAD_INV_addr			(QAM_BASE_ADDR + 0x0084)
#define F498_QAM_MODE_addr			(QAM_BASE_ADDR + 0x0084)
#define F498_SNR_PER_addr			(QAM_BASE_ADDR + 0x0084)
#define F498_SWEEP_RATE_addr			(QAM_BASE_ADDR + 0x0084)
#define F498_SNR_LO_addr			(QAM_BASE_ADDR + 0x0084)
#define F498_SNR_HI_addr			(QAM_BASE_ADDR + 0x0084)
#define F498_GAMMA_LO_addr			(QAM_BASE_ADDR + 0x0088)
#define F498_GAMMA_ME_addr			(QAM_BASE_ADDR + 0x0088)
#define F498_RCAMU_addr			(QAM_BASE_ADDR + 0x0088)
#define F498_CMAMU_addr			(QAM_BASE_ADDR + 0x0088)
#define F498_GAMMA_HI_addr			(QAM_BASE_ADDR + 0x0088)
#define F498_RADIUS_addr			(QAM_BASE_ADDR + 0x0088)
#define F498_FFE_MAINTAP_INIT_addr			(QAM_BASE_ADDR + 0x008C)
#define F498_LEAK_PER_addr			(QAM_BASE_ADDR + 0x008C)
#define F498_EQU_OUTSEL_addr			(QAM_BASE_ADDR + 0x008C)
#define F498_PNT2DFE_addr			(QAM_BASE_ADDR + 0x008C)
#define F498_FFE_LEAK_EN_addr			(QAM_BASE_ADDR + 0x008C)
#define F498_DFE_LEAK_EN_addr			(QAM_BASE_ADDR + 0x008C)
#define F498_FFE_MAINTAP_POS_addr			(QAM_BASE_ADDR + 0x008C)
#define F498_DFE_GAIN_WIDE_addr			(QAM_BASE_ADDR + 0x0090)
#define F498_FFE_GAIN_WIDE_addr			(QAM_BASE_ADDR + 0x0090)
#define F498_DFE_GAIN_NARROW_addr			(QAM_BASE_ADDR + 0x0090)
#define F498_FFE_GAIN_NARROW_addr			(QAM_BASE_ADDR + 0x0090)
#define F498_CTR_GTO_addr			(QAM_BASE_ADDR + 0x0090)
#define F498_CTR_GDIR_addr			(QAM_BASE_ADDR + 0x0090)
#define F498_SWEEP_EN_addr			(QAM_BASE_ADDR + 0x0090)
#define F498_CTR_GINT_addr			(QAM_BASE_ADDR + 0x0090)
#define F498_CRL_GTO_addr			(QAM_BASE_ADDR + 0x0090)
#define F498_CRL_GDIR_addr			(QAM_BASE_ADDR + 0x0090)
#define F498_SWEEP_DIR_addr			(QAM_BASE_ADDR + 0x0090)
#define F498_CRL_GINT_addr			(QAM_BASE_ADDR + 0x0090)
#define F498_CRL_GAIN_addr			(QAM_BASE_ADDR + 0x0094)
#define F498_CTR_INC_GAIN_addr			(QAM_BASE_ADDR + 0x0094)
#define F498_CTR_FRAC_addr			(QAM_BASE_ADDR + 0x0094)
#define F498_CTR_BADPOINT_EN_addr			(QAM_BASE_ADDR + 0x0094)
#define F498_CTR_GAIN_addr			(QAM_BASE_ADDR + 0x0094)
#define F498_LIMANEN_addr			(QAM_BASE_ADDR + 0x0094)
#define F498_CRL_LD_SEN_addr			(QAM_BASE_ADDR + 0x0094)
#define F498_CRL_BISTH_LIMIT_addr			(QAM_BASE_ADDR + 0x0094)
#define F498_CARE_EN_addr			(QAM_BASE_ADDR + 0x0094)
#define F498_CRL_LD_PER_addr			(QAM_BASE_ADDR + 0x0094)
#define F498_CRL_LD_WST_addr			(QAM_BASE_ADDR + 0x0094)
#define F498_CRL_LD_TFS_addr			(QAM_BASE_ADDR + 0x0094)
#define F498_CRL_LD_TFR_addr			(QAM_BASE_ADDR + 0x0094)
#define F498_CRL_BISTH_LO_addr			(QAM_BASE_ADDR + 0x0098)
#define F498_CRL_BISTH_HI_addr			(QAM_BASE_ADDR + 0x0098)
#define F498_SWEEP_RANGE_LO_addr			(QAM_BASE_ADDR + 0x0098)
#define F498_SWEEP_RANGE_HI_addr			(QAM_BASE_ADDR + 0x0098)
#define F498_BISECTOR_EN_addr			(QAM_BASE_ADDR + 0x009C)
#define F498_PHEST128_EN_addr			(QAM_BASE_ADDR + 0x009C)
#define F498_CRL_LIM_addr			(QAM_BASE_ADDR + 0x009C)
#define F498_PNT_DEPTH_addr			(QAM_BASE_ADDR + 0x009C)
#define F498_MODULUS_CMP_addr			(QAM_BASE_ADDR + 0x009C)
#define F498_PNT_EN_addr			(QAM_BASE_ADDR + 0x009C)
#define F498_MODULUSMAP_EN_addr			(QAM_BASE_ADDR + 0x009C)
#define F498_PNT_GAIN_addr			(QAM_BASE_ADDR + 0x009C)
#define F498_RST_RS_addr			(QAM_BASE_ADDR + 0x00A8)
#define F498_RST_DI_addr			(QAM_BASE_ADDR + 0x00A8)
#define F498_BE_BYPASS_addr			(QAM_BASE_ADDR + 0x00A8)
#define F498_REFRESH47_addr			(QAM_BASE_ADDR + 0x00A8)
#define F498_CT_NBST_addr			(QAM_BASE_ADDR + 0x00A8)
#define F498_TEI_ENA_addr			(QAM_BASE_ADDR + 0x00A8)
#define F498_DS_ENA_addr			(QAM_BASE_ADDR + 0x00A8)
#define F498_TSMF_EN_addr			(QAM_BASE_ADDR + 0x00A8)
#define F498_DEINT_DEPTH_addr			(QAM_BASE_ADDR + 0x00A8)
#define F498_DEINT_M_addr			(QAM_BASE_ADDR + 0x00A8)
#define F498_DIS_UNLOCK_addr			(QAM_BASE_ADDR + 0x00A8)
#define F498_DESCR_MODE_addr			(QAM_BASE_ADDR + 0x00A8)
#define F498_DI_UNLOCK_addr			(QAM_BASE_ADDR + 0x00A8)
#define F498_DI_FREEZE_addr			(QAM_BASE_ADDR + 0x00A8)
#define F498_MISMATCH_addr			(QAM_BASE_ADDR + 0x00A8)
#define F498_ACQ_MODE_addr			(QAM_BASE_ADDR + 0x00A8)
#define F498_TRK_MODE_addr			(QAM_BASE_ADDR + 0x00A8)
#define F498_DEINT_SMCNTR_addr			(QAM_BASE_ADDR + 0x00AC)
#define F498_DEINT_SYNCSTATE_addr			(QAM_BASE_ADDR + 0x00AC)
#define F498_DEINT_SYNLOST_addr			(QAM_BASE_ADDR + 0x00AC)
#define F498_DESCR_SYNCSTATE_addr			(QAM_BASE_ADDR + 0x00AC)
#define F498_BK_CT_L_addr			(QAM_BASE_ADDR + 0x00AC)
#define F498_BK_CT_H_addr			(QAM_BASE_ADDR + 0x00AC)
#define F498_CORR_CT_L_addr			(QAM_BASE_ADDR + 0x00B0)
#define F498_CORR_CT_H_addr			(QAM_BASE_ADDR + 0x00B0)
#define F498_UNCORR_CT_L_addr			(QAM_BASE_ADDR + 0x00B0)
#define F498_UNCORR_CT_H_addr			(QAM_BASE_ADDR + 0x00B0)
#define F498_RS_NOCORR_addr			(QAM_BASE_ADDR + 0x00B4)
#define F498_CT_HOLD_addr			(QAM_BASE_ADDR + 0x00B4)
#define F498_CT_CLEAR_addr			(QAM_BASE_ADDR + 0x00B4)
#define F498_BERT_ON_addr			(QAM_BASE_ADDR + 0x00B4)
#define F498_BERT_ERR_SRC_addr			(QAM_BASE_ADDR + 0x00B4)
#define F498_BERT_ERR_MODE_addr			(QAM_BASE_ADDR + 0x00B4)
#define F498_BERT_NBYTE_addr			(QAM_BASE_ADDR + 0x00B4)
#define F498_BERT_ERRCOUNT_L_addr			(QAM_BASE_ADDR + 0x00B4)
#define F498_BERT_ERRCOUNT_H_addr			(QAM_BASE_ADDR + 0x00B4)
#define F498_CLK_POLARITY_addr			(QAM_BASE_ADDR + 0x00B8)
#define F498_FEC_TYPE_addr			(QAM_BASE_ADDR + 0x00B8)
#define F498_SYNC_STRIP_addr			(QAM_BASE_ADDR + 0x00B8)
#define F498_TS_SWAP_addr			(QAM_BASE_ADDR + 0x00B8)
#define F498_OUTFORMAT_addr			(QAM_BASE_ADDR + 0x00B8)
#define F498_CI_DIVRANGE_addr			(QAM_BASE_ADDR + 0x00B8)
#define F498_SMOOTHER_CNT_L_addr			(QAM_BASE_ADDR + 0x00BC)
#define F498_SMOOTHERDIV_EN_L_addr			(QAM_BASE_ADDR + 0x00BC)
#define F498_SMOOTHER_CNT_H_addr			(QAM_BASE_ADDR + 0x00BC)
#define F498_SMOOTH_BYPASS_addr			(QAM_BASE_ADDR + 0x00BC)
#define F498_SWAP_IQ_addr			(QAM_BASE_ADDR + 0x00BC)
#define F498_SMOOTHERDIV_EN_H_addr			(QAM_BASE_ADDR + 0x00BC)
#define F498_TS_NUMBER_addr			(QAM_BASE_ADDR + 0x00C0)
#define F498_SEL_MODE_addr			(QAM_BASE_ADDR + 0x00C0)
#define F498_CHECK_ERROR_BIT_addr			(QAM_BASE_ADDR + 0x00C0)
#define F498_CHCK_F_SYNC_addr			(QAM_BASE_ADDR + 0x00C0)
#define F498_H_MODE_addr			(QAM_BASE_ADDR + 0x00C0)
#define F498_D_V_MODE_addr			(QAM_BASE_ADDR + 0x00C0)
#define F498_MODE_addr			(QAM_BASE_ADDR + 0x00C0)
#define F498_SYNC_IN_COUNT_addr			(QAM_BASE_ADDR + 0x00C0)
#define F498_SYNC_OUT_COUNT_addr			(QAM_BASE_ADDR + 0x00C0)
#define F498_TS_ID_L_addr			(QAM_BASE_ADDR + 0x00C4)
#define F498_TS_ID_H_addr			(QAM_BASE_ADDR + 0x00C4)
#define F498_ON_ID_L_addr			(QAM_BASE_ADDR + 0x00C4)
#define F498_ON_ID_H_addr			(QAM_BASE_ADDR + 0x00C4)
#define F498_RECEIVE_STATUS_L_addr			(QAM_BASE_ADDR + 0x00C8)
#define F498_RECEIVE_STATUS_LH_addr			(QAM_BASE_ADDR + 0x00C8)
#define F498_RECEIVE_STATUS_HL_addr			(QAM_BASE_ADDR + 0x00C8)
#define F498_RECEIVE_STATUS_HH_addr			(QAM_BASE_ADDR + 0x00C8)
#define F498_TS_STATUS_L_addr			(QAM_BASE_ADDR + 0x00CC)
#define F498_TS_STATUS_H_addr			(QAM_BASE_ADDR + 0x00CC)
#define F498_ERROR_addr			(QAM_BASE_ADDR + 0x00CC)
#define F498_EMERGENCY_addr			(QAM_BASE_ADDR + 0x00CC)
#define F498_CRE_TS_addr			(QAM_BASE_ADDR + 0x00CC)
#define F498_VER_addr			(QAM_BASE_ADDR + 0x00CC)
#define F498_M_LOCK_addr			(QAM_BASE_ADDR + 0x00CC)
#define F498_UPDATE_READY_addr			(QAM_BASE_ADDR + 0x00CC)
#define F498_END_FRAME_HEADER_addr			(QAM_BASE_ADDR + 0x00CC)
#define F498_CONTCNT_addr			(QAM_BASE_ADDR + 0x00CC)
#define F498_TS_IDENTIFIER_SEL_addr			(QAM_BASE_ADDR + 0x00CC)
#define F498_ON_ID_I_L_addr			(QAM_BASE_ADDR + 0x00D0)
#define F498_ON_ID_I_H_addr			(QAM_BASE_ADDR + 0x00D0)
#define F498_TS_ID_I_L_addr			(QAM_BASE_ADDR + 0x00D0)
#define F498_TS_ID_I_H_addr			(QAM_BASE_ADDR + 0x00D0)
#define F498_MPEG_DIS_addr			(QAM_BASE_ADDR + 0x00E0)
#define F498_MPEG_HDR_DIS_addr			(QAM_BASE_ADDR + 0x00E0)
#define F498_RS_FLAG_addr			(QAM_BASE_ADDR + 0x00E0)
#define F498_MPEG_FLAG_addr			(QAM_BASE_ADDR + 0x00E0)
#define F498_MPEG_GET_addr			(QAM_BASE_ADDR + 0x00E0)
#define F498_MPEG_DROP_addr			(QAM_BASE_ADDR + 0x00E0)
#define F498_VITERBI_SYNC_GET_addr			(QAM_BASE_ADDR + 0x00E4)
#define F498_VITERBI_SYNC_DROP_addr			(QAM_BASE_ADDR + 0x00E4)
#define F498_VITERBI_SYN_GO_addr			(QAM_BASE_ADDR + 0x00E4)
#define F498_VITERBI_SYN_STOP_addr			(QAM_BASE_ADDR + 0x00E8)
#define F498_FRAME_SYNC_DROP_addr			(QAM_BASE_ADDR + 0x00E8)
#define F498_FRAME_SYNC_GET_addr			(QAM_BASE_ADDR + 0x00E8)
#define F498_INTERLEAVER_DEPTH_addr			(QAM_BASE_ADDR + 0x00E8)
#define F498_RS_CORR_CNT_CLEAR_addr			(QAM_BASE_ADDR + 0x00E8)
#define F498_RS_UNERR_CNT_CLEAR_addr			(QAM_BASE_ADDR + 0x00E8)
#define F498_RS_4_ERROR_addr			(QAM_BASE_ADDR + 0x00E8)
#define F498_RS_CLR_ERR_addr			(QAM_BASE_ADDR + 0x00E8)
#define F498_RS_CLR_UNC_addr			(QAM_BASE_ADDR + 0x00E8)
#define F498_RS_RATE_ADJ_addr			(QAM_BASE_ADDR + 0x00E8)
#define F498_DEPTH_AUTOMODE_addr			(QAM_BASE_ADDR + 0x00EC)
#define F498_FRAME_SYNC_COUNT_addr			(QAM_BASE_ADDR + 0x00EC)
#define F498_MPEG_SYNC_addr			(QAM_BASE_ADDR + 0x00EC)
#define F498_VITERBI_I_SYNC_addr			(QAM_BASE_ADDR + 0x00EC)
#define F498_VITERBI_Q_SYNC_addr			(QAM_BASE_ADDR + 0x00EC)
#define F498_COMB_STATE_addr			(QAM_BASE_ADDR + 0x00EC)
#define F498_VITERBI_RATE_I_addr			(QAM_BASE_ADDR + 0x00EC)
#define F498_VITERBI_RATE_Q_addr			(QAM_BASE_ADDR + 0x00EC)
#define F498_RS_CORRECTED_COUNT_LO_addr			(QAM_BASE_ADDR + 0x00F0)
#define F498_RS_CORRECTED_COUNT_HI_addr			(QAM_BASE_ADDR + 0x00F0)
#define F498_RS_UNERRORED_COUNT_LO_addr			(QAM_BASE_ADDR + 0x00F0)
#define F498_RS_UNERRORED_COUNT_HI_addr			(QAM_BASE_ADDR + 0x00F0)
#define F498_RS_UNC_COUNT_LO_addr			(QAM_BASE_ADDR + 0x00F4)
#define F498_RS_UNC_COUNT_HI_addr			(QAM_BASE_ADDR + 0x00F4)
#define F498_RS_RATE_LO_addr			(QAM_BASE_ADDR + 0x00F4)
#define F498_RS_RATE_HI_addr			(QAM_BASE_ADDR + 0x00F4)
#define F498_TX_INTERLEAVE_DEPTH_addr			(QAM_BASE_ADDR + 0x00F8)
#define F498_RS_ERROR_COUNT_L_addr			(QAM_BASE_ADDR + 0x00F8)
#define F498_RS_ERROR_COUNT_H_addr			(QAM_BASE_ADDR + 0x00F8)
#define F498_IQ_SWAP_addr			(QAM_BASE_ADDR + 0x00FC)
#define F498_OOB_CLK_INV_addr			(QAM_BASE_ADDR + 0x00FC)
#define F498_CRX_RATIO_addr			(QAM_BASE_ADDR + 0x00FC)
#define F498_DRX_DELAY_addr			(QAM_BASE_ADDR + 0x00FC)
#define R498_CTRL_0_addr			(QAM_BASE_ADDR + 0x0000)
#define R498_CTRL_1_addr			(QAM_BASE_ADDR + 0x0000)
#define R498_CTRL_2_addr			(QAM_BASE_ADDR + 0x0000)
#define R498_IT_STATUS1_addr			(QAM_BASE_ADDR + 0x0008)
#define R498_IT_STATUS2_addr			(QAM_BASE_ADDR + 0x0008)
#define R498_IT_EN1_addr			(QAM_BASE_ADDR + 0x0008)
#define R498_IT_EN2_addr			(QAM_BASE_ADDR + 0x0008)
#define R498_CTRL_STATUS_addr			(QAM_BASE_ADDR + 0x000C)
#define R498_TEST_CTL_addr			(QAM_BASE_ADDR + 0x000C)
#define R498_AGC_CTL_addr			(QAM_BASE_ADDR + 0x0010)
#define R498_AGC_IF_CFG_addr			(QAM_BASE_ADDR + 0x0010)
#define R498_AGC_RF_CFG_addr			(QAM_BASE_ADDR + 0x0010)
#define R498_AGC_PWM_CFG_addr			(QAM_BASE_ADDR + 0x0010)
#define R498_AGC_PWR_REF_L_addr			(QAM_BASE_ADDR + 0x0014)
#define R498_AGC_PWR_REF_H_addr			(QAM_BASE_ADDR + 0x0014)
#define R498_AGC_RF_TH_L_addr			(QAM_BASE_ADDR + 0x0014)
#define R498_AGC_RF_TH_H_addr			(QAM_BASE_ADDR + 0x0014)
#define R498_AGC_IF_LTH_L_addr			(QAM_BASE_ADDR + 0x0018)
#define R498_AGC_IF_LTH_H_addr			(QAM_BASE_ADDR + 0x0018)
#define R498_AGC_IF_HTH_L_addr			(QAM_BASE_ADDR + 0x0018)
#define R498_AGC_IF_HTH_H_addr			(QAM_BASE_ADDR + 0x0018)
#define R498_AGC_PWR_RD_L_addr			(QAM_BASE_ADDR + 0x001C)
#define R498_AGC_PWR_RD_M_addr			(QAM_BASE_ADDR + 0x001C)
#define R498_AGC_PWR_RD_H_addr			(QAM_BASE_ADDR + 0x001C)
#define R498_AGC_PWM_IFCMD_L_addr			(QAM_BASE_ADDR + 0x0020)
#define R498_AGC_PWM_IFCMD_H_addr			(QAM_BASE_ADDR + 0x0020)
#define R498_AGC_PWM_RFCMD_L_addr			(QAM_BASE_ADDR + 0x0020)
#define R498_AGC_PWM_RFCMD_H_addr			(QAM_BASE_ADDR + 0x0020)
#define R498_IQDEM_CFG_addr			(QAM_BASE_ADDR + 0x0024)
#define R498_MIX_NCO_LL_addr			(QAM_BASE_ADDR + 0x0024)
#define R498_MIX_NCO_HL_addr			(QAM_BASE_ADDR + 0x0024)
#define R498_MIX_NCO_HH_addr			(QAM_BASE_ADDR + 0x0024)
#define R498_MIX_NCO_addr			(QAM_BASE_ADDR + 0x0024)
#define R498_SRC_NCO_LL_addr			(QAM_BASE_ADDR + 0x0028)
#define R498_SRC_NCO_LH_addr			(QAM_BASE_ADDR + 0x0028)
#define R498_SRC_NCO_HL_addr			(QAM_BASE_ADDR + 0x0028)
#define R498_SRC_NCO_HH_addr			(QAM_BASE_ADDR + 0x0028)
#define R498_SRC_NCO_addr			(QAM_BASE_ADDR + 0x0028)
#define R498_IQDEM_GAIN_SRC_L_addr			(QAM_BASE_ADDR + 0x002C)
#define R498_IQDEM_GAIN_SRC_H_addr			(QAM_BASE_ADDR + 0x002C)
#define R498_IQDEM_DCRM_CFG_LL_addr			(QAM_BASE_ADDR + 0x0030)
#define R498_IQDEM_DCRM_CFG_LH_addr			(QAM_BASE_ADDR + 0x0030)
#define R498_IQDEM_DCRM_CFG_HL_addr			(QAM_BASE_ADDR + 0x0030)
#define R498_IQDEM_DCRM_CFG_HH_addr			(QAM_BASE_ADDR + 0x0030)
#define R498_IQDEM_ADJ_COEFF0_addr			(QAM_BASE_ADDR + 0x0034)
#define R498_IQDEM_ADJ_COEFF1_addr			(QAM_BASE_ADDR + 0x0034)
#define R498_IQDEM_ADJ_COEFF2_addr			(QAM_BASE_ADDR + 0x0034)
#define R498_IQDEM_ADJ_COEFF3_addr			(QAM_BASE_ADDR + 0x0034)
#define R498_IQDEM_ADJ_COEFF4_addr			(QAM_BASE_ADDR + 0x0038)
#define R498_IQDEM_ADJ_COEFF5_addr			(QAM_BASE_ADDR + 0x0038)
#define R498_IQDEM_ADJ_COEFF6_addr			(QAM_BASE_ADDR + 0x0038)
#define R498_IQDEM_ADJ_COEFF7_addr			(QAM_BASE_ADDR + 0x0038)
#define R498_IQDEM_ADJ_EN_addr			(QAM_BASE_ADDR + 0x003C)
#define R498_IQDEM_ADJ_AGC_REF_addr			(QAM_BASE_ADDR + 0x003C)
#define R498_ALLPASSFILT1_addr			(QAM_BASE_ADDR + 0x0040)
#define R498_ALLPASSFILT2_addr			(QAM_BASE_ADDR + 0x0040)
#define R498_ALLPASSFILT3_addr			(QAM_BASE_ADDR + 0x0040)
#define R498_ALLPASSFILT4_addr			(QAM_BASE_ADDR + 0x0040)
#define R498_ALLPASSFILT5_addr			(QAM_BASE_ADDR + 0x0044)
#define R498_ALLPASSFILT6_addr			(QAM_BASE_ADDR + 0x0044)
#define R498_ALLPASSFILT7_addr			(QAM_BASE_ADDR + 0x0044)
#define R498_ALLPASSFILT8_addr			(QAM_BASE_ADDR + 0x0044)
#define R498_ALLPASSFILT9_addr			(QAM_BASE_ADDR + 0x0048)
#define R498_ALLPASSFILT10_addr			(QAM_BASE_ADDR + 0x0048)
#define R498_ALLPASSFILT11_addr			(QAM_BASE_ADDR + 0x0048)
#define R498_TRL_AGC_CFG_addr			(QAM_BASE_ADDR + 0x0050)
#define R498_TRL_LPF_CFG_addr			(QAM_BASE_ADDR + 0x0054)
#define R498_TRL_LPF_ACQ_GAIN_addr			(QAM_BASE_ADDR + 0x0054)
#define R498_TRL_LPF_TRK_GAIN_addr			(QAM_BASE_ADDR + 0x0054)
#define R498_TRL_LPF_OUT_GAIN_addr			(QAM_BASE_ADDR + 0x0054)
#define R498_TRL_LOCKDET_LTH_addr			(QAM_BASE_ADDR + 0x0058)
#define R498_TRL_LOCKDET_HTH_addr			(QAM_BASE_ADDR + 0x0058)
#define R498_TRL_LOCKDET_TRGVAL_addr			(QAM_BASE_ADDR + 0x0058)
#define R498_FSM_STATE_addr			(QAM_BASE_ADDR + 0x0060)
#define R498_FSM_CTL_addr			(QAM_BASE_ADDR + 0x0060)
#define R498_FSM_STS_addr			(QAM_BASE_ADDR + 0x0060)
#define R498_FSM_SNR0_HTH_addr			(QAM_BASE_ADDR + 0x0060)
#define R498_FSM_SNR1_HTH_addr			(QAM_BASE_ADDR + 0x0064)
#define R498_FSM_SNR2_HTH_addr			(QAM_BASE_ADDR + 0x0064)
#define R498_FSM_SNR0_LTH_addr			(QAM_BASE_ADDR + 0x0064)
#define R498_FSM_SNR1_LTH_addr			(QAM_BASE_ADDR + 0x0064)
#define R498_FSM_RCA_HTH_addr			(QAM_BASE_ADDR + 0x0068)
#define R498_FSM_TEMPO_addr			(QAM_BASE_ADDR + 0x0068)
#define R498_FSM_CONFIG_addr			(QAM_BASE_ADDR + 0x0068)
#define R498_EQU_I_TESTTAP_L_addr			(QAM_BASE_ADDR + 0x0074)
#define R498_EQU_I_TESTTAP_M_addr			(QAM_BASE_ADDR + 0x0074)
#define R498_EQU_I_TESTTAP_H_addr			(QAM_BASE_ADDR + 0x0074)
#define R498_EQU_TESTAP_CFG_addr			(QAM_BASE_ADDR + 0x0074)
#define R498_EQU_Q_TESTTAP_L_addr			(QAM_BASE_ADDR + 0x0078)
#define R498_EQU_Q_TESTTAP_M_addr			(QAM_BASE_ADDR + 0x0078)
#define R498_EQU_Q_TESTTAP_H_addr			(QAM_BASE_ADDR + 0x0078)
#define R498_EQU_TAP_CTRL_addr			(QAM_BASE_ADDR + 0x0078)
#define R498_EQU_CTR_CRL_CONTROL_L_addr			(QAM_BASE_ADDR + 0x007C)
#define R498_EQU_CTR_CRL_CONTROL_H_addr			(QAM_BASE_ADDR + 0x007C)
#define R498_EQU_CTR_HIPOW_L_addr			(QAM_BASE_ADDR + 0x007C)
#define R498_EQU_CTR_HIPOW_H_addr			(QAM_BASE_ADDR + 0x007C)
#define R498_EQU_I_EQU_LO_addr			(QAM_BASE_ADDR + 0x0080)
#define R498_EQU_I_EQU_HI_addr			(QAM_BASE_ADDR + 0x0080)
#define R498_EQU_Q_EQU_LO_addr			(QAM_BASE_ADDR + 0x0080)
#define R498_EQU_Q_EQU_HI_addr			(QAM_BASE_ADDR + 0x0080)
#define R498_EQU_MAPPER_addr			(QAM_BASE_ADDR + 0x0084)
#define R498_EQU_SWEEP_RATE_addr			(QAM_BASE_ADDR + 0x0084)
#define R498_EQU_SNR_LO_addr			(QAM_BASE_ADDR + 0x0084)
#define R498_EQU_SNR_HI_addr			(QAM_BASE_ADDR + 0x0084)
#define R498_EQU_GAMMA_LO_addr			(QAM_BASE_ADDR + 0x0088)
#define R498_EQU_GAMMA_HI_addr			(QAM_BASE_ADDR + 0x0088)
#define R498_EQU_ERR_GAIN_addr			(QAM_BASE_ADDR + 0x0088)
#define R498_EQU_RADIUS_addr			(QAM_BASE_ADDR + 0x0088)
#define R498_EQU_FFE_MAINTAP_addr			(QAM_BASE_ADDR + 0x008C)
#define R498_EQU_FFE_LEAKAGE_addr			(QAM_BASE_ADDR + 0x008C)
#define R498_EQU_FFE_MAINTAP_POS_addr			(QAM_BASE_ADDR + 0x008C)
#define R498_EQU_GAIN_WIDE_addr			(QAM_BASE_ADDR + 0x0090)
#define R498_EQU_GAIN_NARROW_addr			(QAM_BASE_ADDR + 0x0090)
#define R498_EQU_CTR_LPF_GAIN_addr			(QAM_BASE_ADDR + 0x0090)
#define R498_EQU_CRL_LPF_GAIN_addr			(QAM_BASE_ADDR + 0x0090)
#define R498_EQU_GLOBAL_GAIN_addr			(QAM_BASE_ADDR + 0x0094)
#define R498_EQU_CRL_LD_SEN_addr			(QAM_BASE_ADDR + 0x0094)
#define R498_EQU_CRL_LD_VAL_addr			(QAM_BASE_ADDR + 0x0094)
#define R498_EQU_CRL_TFR_addr			(QAM_BASE_ADDR + 0x0094)
#define R498_EQU_CRL_BISTH_LO_addr			(QAM_BASE_ADDR + 0x0098)
#define R498_EQU_CRL_BISTH_HI_addr			(QAM_BASE_ADDR + 0x0098)
#define R498_EQU_SWEEP_RANGE_LO_addr			(QAM_BASE_ADDR + 0x0098)
#define R498_EQU_SWEEP_RANGE_HI_addr			(QAM_BASE_ADDR + 0x0098)
#define R498_EQU_CRL_LIMITER_addr			(QAM_BASE_ADDR + 0x009C)
#define R498_EQU_MODULUS_MAP_addr			(QAM_BASE_ADDR + 0x009C)
#define R498_EQU_PNT_GAIN_addr			(QAM_BASE_ADDR + 0x009C)
#define R498_FEC_AC_CTR_0_addr			(QAM_BASE_ADDR + 0x00A8)
#define R498_FEC_AC_CTR_1_addr			(QAM_BASE_ADDR + 0x00A8)
#define R498_FEC_AC_CTR_2_addr			(QAM_BASE_ADDR + 0x00A8)
#define R498_FEC_AC_CTR_3_addr			(QAM_BASE_ADDR + 0x00A8)
#define R498_FEC_STATUS_addr			(QAM_BASE_ADDR + 0x00AC)
#define R498_RS_COUNTER_0_addr			(QAM_BASE_ADDR + 0x00AC)
#define R498_RS_COUNTER_1_addr			(QAM_BASE_ADDR + 0x00AC)
#define R498_RS_COUNTER_2_addr			(QAM_BASE_ADDR + 0x00B0)
#define R498_RS_COUNTER_3_addr			(QAM_BASE_ADDR + 0x00B0)
#define R498_RS_COUNTER_4_addr			(QAM_BASE_ADDR + 0x00B0)
#define R498_RS_COUNTER_5_addr			(QAM_BASE_ADDR + 0x00B0)
#define R498_BERT_0_addr			(QAM_BASE_ADDR + 0x00B4)
#define R498_BERT_1_addr			(QAM_BASE_ADDR + 0x00B4)
#define R498_BERT_2_addr			(QAM_BASE_ADDR + 0x00B4)
#define R498_BERT_3_addr			(QAM_BASE_ADDR + 0x00B4)
#define R498_OUTFORMAT_0_addr			(QAM_BASE_ADDR + 0x00B8)
#define R498_OUTFORMAT_1_addr			(QAM_BASE_ADDR + 0x00B8)
#define R498_SMOOTHER_0_addr			(QAM_BASE_ADDR + 0x00BC)
#define R498_SMOOTHER_1_addr			(QAM_BASE_ADDR + 0x00BC)
#define R498_SMOOTHER_2_addr			(QAM_BASE_ADDR + 0x00BC)
#define R498_TSMF_CTRL_0_addr			(QAM_BASE_ADDR + 0x00C0)
#define R498_TSMF_CTRL_1_addr			(QAM_BASE_ADDR + 0x00C0)
#define R498_TSMF_CTRL_3_addr			(QAM_BASE_ADDR + 0x00C0)
#define R498_TS_ON_ID_0_addr			(QAM_BASE_ADDR + 0x00C4)
#define R498_TS_ON_ID_1_addr			(QAM_BASE_ADDR + 0x00C4)
#define R498_TS_ON_ID_2_addr			(QAM_BASE_ADDR + 0x00C4)
#define R498_TS_ON_ID_3_addr			(QAM_BASE_ADDR + 0x00C4)
#define R498_RE_STATUS_0_addr			(QAM_BASE_ADDR + 0x00C8)
#define R498_RE_STATUS_1_addr			(QAM_BASE_ADDR + 0x00C8)
#define R498_RE_STATUS_2_addr			(QAM_BASE_ADDR + 0x00C8)
#define R498_RE_STATUS_3_addr			(QAM_BASE_ADDR + 0x00C8)
#define R498_TS_STATUS_0_addr			(QAM_BASE_ADDR + 0x00CC)
#define R498_TS_STATUS_1_addr			(QAM_BASE_ADDR + 0x00CC)
#define R498_TS_STATUS_2_addr			(QAM_BASE_ADDR + 0x00CC)
#define R498_TS_STATUS_3_addr			(QAM_BASE_ADDR + 0x00CC)
#define R498_T_O_ID_0_addr			(QAM_BASE_ADDR + 0x00D0)
#define R498_T_O_ID_1_addr			(QAM_BASE_ADDR + 0x00D0)
#define R498_T_O_ID_2_addr			(QAM_BASE_ADDR + 0x00D0)
#define R498_T_O_ID_3_addr			(QAM_BASE_ADDR + 0x00D0)
#define R498_MPEG_CTL_addr			(QAM_BASE_ADDR + 0x00E0)
#define R498_MPEG_SYNC_ACQ_addr			(QAM_BASE_ADDR + 0x00E0)
#define R498_FECB_MPEG_SYNC_LOSS_addr			(QAM_BASE_ADDR + 0x00E0)
#define R498_VITERBI_SYNC_ACQ_addr			(QAM_BASE_ADDR + 0x00E4)
#define R498_VITERBI_SYNC_LOSS_addr			(QAM_BASE_ADDR + 0x00E4)
#define R498_VITERBI_SYNC_GO_addr			(QAM_BASE_ADDR + 0x00E4)
#define R498_VITERBI_SYNC_STOP_addr			(QAM_BASE_ADDR + 0x00E8)
#define R498_FS_SYNC_addr			(QAM_BASE_ADDR + 0x00E8)
#define R498_IN_DEPTH_addr			(QAM_BASE_ADDR + 0x00E8)
#define R498_RS_CONTROL_addr			(QAM_BASE_ADDR + 0x00E8)
#define R498_DEINT_CONTROL_addr			(QAM_BASE_ADDR + 0x00EC)
#define R498_SYNC_STAT_addr			(QAM_BASE_ADDR + 0x00EC)
#define R498_VITERBI_I_RATE_addr			(QAM_BASE_ADDR + 0x00EC)
#define R498_VITERBI_Q_RATE_addr			(QAM_BASE_ADDR + 0x00EC)
#define R498_RS_CORR_COUNT_L_addr			(QAM_BASE_ADDR + 0x00F0)
#define R498_RS_CORR_COUNT_H_addr			(QAM_BASE_ADDR + 0x00F0)
#define R498_RS_UNERR_COUNT_L_addr			(QAM_BASE_ADDR + 0x00F0)
#define R498_RS_UNERR_COUNT_H_addr			(QAM_BASE_ADDR + 0x00F0)
#define R498_RS_UNC_COUNT_L_addr			(QAM_BASE_ADDR + 0x00F4)
#define R498_RS_UNC_COUNT_H_addr			(QAM_BASE_ADDR + 0x00F4)
#define R498_RS_RATE_L_addr			(QAM_BASE_ADDR + 0x00F4)
#define R498_RS_RATE_H_addr			(QAM_BASE_ADDR + 0x00F4)
#define R498_TX_INTERLEAVER_DEPTH_addr			(QAM_BASE_ADDR + 0x00F8)
#define R498_RS_ERR_CNT_L_addr			(QAM_BASE_ADDR + 0x00F8)
#define R498_RS_ERR_CNT_H_addr			(QAM_BASE_ADDR + 0x00F8)
#define R498_OOB_DUTY_addr			(QAM_BASE_ADDR + 0x00FC)
#define R498_OOB_DELAY_addr			(QAM_BASE_ADDR + 0x00FC)


#define F498_STANDBY_size			0x01
#define F498_SOFT_RST_size			0x01
#define F498_EQU_RST_size			0x01
#define F498_CRL_RST_size			0x01
#define F498_TRL_RST_size			0x01
#define F498_AGC_RST_size			0x01
#define F498_FEC_NHW_RESET_size			0x01
#define F498_DEINT_RST_size			0x01
#define F498_FECAC_RS_RST_size			0x01
#define F498_FECB_RS_RST_size			0x01
#define F498_VITERBI_RST_size			0x01
#define F498_SWEEP_OUT_size			0x01
#define F498_FSM_CRL_size			0x01
#define F498_CRL_LOCK_size			0x01
#define F498_MFSM_size			0x01
#define F498_TRL_LOCK_size			0x01
#define F498_TRL_AGC_LIMIT_size			0x01
#define F498_ADJ_AGC_LOCK_size			0x01
#define F498_AGC_LOCK_size			0x01
#define F498_TSMF_CNT_size			0x01
#define F498_TSMF_EOF_size			0x01
#define F498_TSMF_RDY_size			0x01
#define F498_FEC_NOCORR_size			0x01
#define F498_FEC_LOCK_size			0x01
#define F498_DEINT_LOCK_size			0x01
#define F498_TAPMON_ALARM_size			0x01
#define F498_SWEEP_OUTE_size			0x01
#define F498_FSM_CRLE_size			0x01
#define F498_CRL_LOCKE_size			0x01
#define F498_MFSME_size			0x01
#define F498_TRL_LOCKE_size			0x01
#define F498_TRL_AGC_LIMITE_size			0x01
#define F498_ADJ_AGC_LOCKE_size			0x01
#define F498_AGC_LOCKE_size			0x01
#define F498_TSMF_CNTE_size			0x01
#define F498_TSMF_EOFE_size			0x01
#define F498_TSMF_RDYE_size			0x01
#define F498_FEC_NOCORRE_size			0x01
#define F498_FEC_LOCKE_size			0x01
#define F498_DEINT_LOCKE_size			0x01
#define F498_TAPMON_ALARME_size			0x01
#define F498_QAMFEC_LOCK_size			0x01
#define F498_TSMF_LOCK_size			0x01
#define F498_TSMF_ERROR_size			0x01
#define F498_TST_BUS_SEL_size			0x05
#define F498_AGC_LCK_TH_size			0x04
#define F498_AGC_ACCUMRSTSEL_size			0x03
#define F498_AGC_IF_BWSEL_size			0x04
#define F498_AGC_IF_FREEZE_size			0x01
#define F498_AGC_RF_BWSEL_size			0x03
#define F498_AGC_RF_FREEZE_size			0x01
#define F498_AGC_RF_PWM_TST_size			0x01
#define F498_AGC_RF_PWM_INV_size			0x01
#define F498_AGC_IF_PWM_TST_size			0x01
#define F498_AGC_IF_PWM_INV_size			0x01
#define F498_AGC_PWM_CLKDIV_size			0x02
#define F498_AGC_PWRREF_LO_size			0x08
#define F498_AGC_PWRREF_HI_size			0x02
#define F498_AGC_RF_TH_LO_size			0x08
#define F498_AGC_RF_TH_HI_size			0x04
#define F498_AGC_IF_THLO_LO_size			0x08
#define F498_AGC_IF_THLO_HI_size			0x04
#define F498_AGC_IF_THHI_LO_size			0x08
#define F498_AGC_IF_THHI_HI_size			0x04
#define F498_AGC_PWR_WORD_LO_size			0x08
#define F498_AGC_PWR_WORD_ME_size			0x08
#define F498_AGC_PWR_WORD_HI_size			0x02
#define F498_AGC_IF_PWMCMD_LO_size			0x08
#define F498_AGC_IF_PWMCMD_HI_size			0x04
#define F498_AGC_RF_PWMCMD_LO_size			0x08
#define F498_AGC_RF_PWMCMD_HI_size			0x04
#define F498_IQDEM_CLK_SEL_size			0x01
#define F498_IQDEM_INVIQ_size			0x01
#define F498_IQDEM_A2DTYPE_size			0x01
#define F498_MIX_NCO_INC_LL_size			0x08
#define F498_MIX_NCO_INC_HL_size			0x08
#define F498_MIX_NCO_INVCNST_size			0x01
#define F498_MIX_NCO_INC_HH_size			0x07
#define F498_SRC_NCO_INC_LL_size			0x08
#define F498_SRC_NCO_INC_LH_size			0x08
#define F498_SRC_NCO_INC_HL_size			0x08
#define F498_SRC_NCO_INC_HH_size			0x07
#define F498_GAIN_SRC_LO_size			0x08
#define F498_GAIN_SRC_HI_size			0x02
#define F498_DCRM0_DCIN_L_size			0x08
#define F498_DCRM1_I_DCIN_L_size			0x06
#define F498_DCRM0_DCIN_H_size			0x02
#define F498_DCRM1_Q_DCIN_L_size			0x04
#define F498_DCRM1_I_DCIN_H_size			0x04
#define F498_DCRM1_FRZ_size			0x01
#define F498_DCRM0_FRZ_size			0x01
#define F498_DCRM1_Q_DCIN_H_size			0x06
#define F498_ADJIIR_COEFF10_L_size			0x08
#define F498_ADJIIR_COEFF11_L_size			0x06
#define F498_ADJIIR_COEFF10_H_size			0x02
#define F498_ADJIIR_COEFF12_L_size			0x04
#define F498_ADJIIR_COEFF11_H_size			0x04
#define F498_ADJIIR_COEFF20_L_size			0x02
#define F498_ADJIIR_COEFF12_H_size			0x06
#define F498_ADJIIR_COEFF20_H_size			0x08
#define F498_ADJIIR_COEFF21_L_size			0x08
#define F498_ADJIIR_COEFF22_L_size			0x06
#define F498_ADJIIR_COEFF21_H_size			0x02
#define F498_ADJIIR_COEFF22_H_size			0x04
#define F498_ALLPASSFILT_EN_size			0x01
#define F498_ADJ_AGC_EN_size			0x01
#define F498_ADJ_COEFF_FRZ_size			0x01
#define F498_ADJ_EN_size			0x01
#define F498_ADJ_AGC_REF_size			0x08
#define F498_ALLPASSFILT_COEFF1_LO_size			0x08
#define F498_ALLPASSFILT_COEFF1_ME_size			0x08
#define F498_ALLPASSFILT_COEFF2_LO_size			0x02
#define F498_ALLPASSFILT_COEFF1_HI_size			0x06
#define F498_ALLPASSFILT_COEFF2_ME_size			0x08
#define F498_ALLPASSFILT_COEFF2_HI_size			0x08
#define F498_ALLPASSFILT_COEFF3_LO_size			0x04
#define F498_ALLPASSFILT_COEFF2_HH_size			0x04
#define F498_ALLPASSFILT_COEFF3_ME_size			0x08
#define F498_ALLPASSFILT_COEFF3_HI_size			0x08
#define F498_ALLPASSFILT_COEFF4_LO_size			0x06
#define F498_ALLPASSFILT_COEFF3_HH_size			0x02
#define F498_ALLPASSFILT_COEFF4_ME_size			0x08
#define F498_ALLPASSFILT_COEFF4_HI_size			0x08
#define F498_TRL_AGC_FREEZE_size			0x01
#define F498_TRL_AGC_REF_size			0x07
#define F498_NYQPOINT_INV_size			0x01
#define F498_TRL_SHIFT_size			0x02
#define F498_NYQ_COEFF_SEL_size			0x03
#define F498_TRL_LPF_FREEZE_size			0x01
#define F498_TRL_LPF_CRT_size			0x01
#define F498_TRL_GDIR_ACQ_size			0x03
#define F498_TRL_GINT_ACQ_size			0x03
#define F498_TRL_GDIR_TRK_size			0x03
#define F498_TRL_GINT_TRK_size			0x03
#define F498_TRL_GAIN_OUT_size			0x03
#define F498_TRL_LCK_THLO_size			0x03
#define F498_TRL_LCK_THHI_size			0x08
#define F498_TRL_LCK_TRG_size			0x08
#define F498_CRL_DFE_size			0x01
#define F498_DFE_START_size			0x01
#define F498_CTRLG_START_size			0x02
#define F498_FSM_FORCESTATE_size			0x04
#define F498_FEC2_EN_size			0x01
#define F498_SIT_EN_size			0x01
#define F498_TRL_AHEAD_size			0x01
#define F498_TRL2_EN_size			0x01
#define F498_FSM_RCA_EN_size			0x01
#define F498_FSM_BKP_DIS_size			0x01
#define F498_FSM_FORCE_EN_size			0x01
#define F498_FSM_STATUS_size			0x04
#define F498_SNR0_HTH_size			0x08
#define F498_SNR1_HTH_size			0x08
#define F498_SNR2_HTH_size			0x08
#define F498_SNR0_LTH_size			0x08
#define F498_SNR1_LTH_size			0x08
#define F498_SNR3_HTH_L_size			0x04
#define F498_RCA_HTH_size			0x04
#define F498_SIT_size			0x02
#define F498_WST_size			0x03
#define F498_ELT_size			0x02
#define F498_SNR3_HTH_H_size			0x01
#define F498_FEC2_DFEOFF_size			0x01
#define F498_PNT_STATE_size			0x01
#define F498_MODMAP_STATE_size			0x01
#define F498_I_TEST_TAP_L_size			0x08
#define F498_I_TEST_TAP_M_size			0x08
#define F498_I_TEST_TAP_H_size			0x05
#define F498_TEST_FFE_DFE_SEL_size			0x01
#define F498_TEST_TAP_SELECT_size			0x06
#define F498_Q_TEST_TAP_L_size			0x08
#define F498_Q_TEST_TAP_M_size			0x08
#define F498_Q_TEST_TAP_H_size			0x05
#define F498_MTAP_FRZ_size			0x01
#define F498_PRE_FREEZE_size			0x01
#define F498_DFE_TAPMON_EN_size			0x01
#define F498_FFE_TAPMON_EN_size			0x01
#define F498_MTAP_ONLY_size			0x01
#define F498_EQU_CTR_CRL_CONTROL_LO_size			0x08
#define F498_EQU_CTR_CRL_CONTROL_HI_size			0x08
#define F498_CTR_HIPOW_L_size			0x08
#define F498_CTR_HIPOW_H_size			0x08
#define F498_EQU_I_EQU_L_size			0x08
#define F498_EQU_I_EQU_H_size			0x02
#define F498_EQU_Q_EQU_L_size			0x08
#define F498_EQU_Q_EQU_H_size			0x02
#define F498_QUAD_AUTO_size			0x01
#define F498_QUAD_INV_size			0x01
#define F498_QAM_MODE_size			0x03
#define F498_SNR_PER_size			0x02
#define F498_SWEEP_RATE_size			0x06
#define F498_SNR_LO_size			0x08
#define F498_SNR_HI_size			0x08
#define F498_GAMMA_LO_size			0x08
#define F498_GAMMA_ME_size			0x08
#define F498_RCAMU_size			0x03
#define F498_CMAMU_size			0x03
#define F498_GAMMA_HI_size			0x01
#define F498_RADIUS_size			0x08
#define F498_FFE_MAINTAP_INIT_size			0x08
#define F498_LEAK_PER_size			0x04
#define F498_EQU_OUTSEL_size			0x01
#define F498_PNT2DFE_size			0x01
#define F498_FFE_LEAK_EN_size			0x01
#define F498_DFE_LEAK_EN_size			0x01
#define F498_FFE_MAINTAP_POS_size			0x06
#define F498_DFE_GAIN_WIDE_size			0x04
#define F498_FFE_GAIN_WIDE_size			0x04
#define F498_DFE_GAIN_NARROW_size			0x04
#define F498_FFE_GAIN_NARROW_size			0x04
#define F498_CTR_GTO_size			0x01
#define F498_CTR_GDIR_size			0x03
#define F498_SWEEP_EN_size			0x01
#define F498_CTR_GINT_size			0x03
#define F498_CRL_GTO_size			0x01
#define F498_CRL_GDIR_size			0x03
#define F498_SWEEP_DIR_size			0x01
#define F498_CRL_GINT_size			0x03
#define F498_CRL_GAIN_size			0x05
#define F498_CTR_INC_GAIN_size			0x01
#define F498_CTR_FRAC_size			0x02
#define F498_CTR_BADPOINT_EN_size			0x01
#define F498_CTR_GAIN_size			0x03
#define F498_LIMANEN_size			0x01
#define F498_CRL_LD_SEN_size			0x03
#define F498_CRL_BISTH_LIMIT_size			0x01
#define F498_CARE_EN_size			0x01
#define F498_CRL_LD_PER_size			0x02
#define F498_CRL_LD_WST_size			0x02
#define F498_CRL_LD_TFS_size			0x02
#define F498_CRL_LD_TFR_size			0x08
#define F498_CRL_BISTH_LO_size			0x08
#define F498_CRL_BISTH_HI_size			0x08
#define F498_SWEEP_RANGE_LO_size			0x08
#define F498_SWEEP_RANGE_HI_size			0x08
#define F498_BISECTOR_EN_size			0x01
#define F498_PHEST128_EN_size			0x01
#define F498_CRL_LIM_size			0x06
#define F498_PNT_DEPTH_size			0x03
#define F498_MODULUS_CMP_size			0x05
#define F498_PNT_EN_size			0x01
#define F498_MODULUSMAP_EN_size			0x01
#define F498_PNT_GAIN_size			0x06
#define F498_RST_RS_size			0x01
#define F498_RST_DI_size			0x01
#define F498_BE_BYPASS_size			0x01
#define F498_REFRESH47_size			0x01
#define F498_CT_NBST_size			0x01
#define F498_TEI_ENA_size			0x01
#define F498_DS_ENA_size			0x01
#define F498_TSMF_EN_size			0x01
#define F498_DEINT_DEPTH_size			0x08
#define F498_DEINT_M_size			0x05
#define F498_DIS_UNLOCK_size			0x01
#define F498_DESCR_MODE_size			0x02
#define F498_DI_UNLOCK_size			0x01
#define F498_DI_FREEZE_size			0x01
#define F498_MISMATCH_size			0x02
#define F498_ACQ_MODE_size			0x02
#define F498_TRK_MODE_size			0x02
#define F498_DEINT_SMCNTR_size			0x03
#define F498_DEINT_SYNCSTATE_size			0x02
#define F498_DEINT_SYNLOST_size			0x01
#define F498_DESCR_SYNCSTATE_size			0x01
#define F498_BK_CT_L_size			0x08
#define F498_BK_CT_H_size			0x08
#define F498_CORR_CT_L_size			0x08
#define F498_CORR_CT_H_size			0x08
#define F498_UNCORR_CT_L_size			0x08
#define F498_UNCORR_CT_H_size			0x08
#define F498_RS_NOCORR_size			0x01
#define F498_CT_HOLD_size			0x01
#define F498_CT_CLEAR_size			0x01
#define F498_BERT_ON_size			0x01
#define F498_BERT_ERR_SRC_size			0x01
#define F498_BERT_ERR_MODE_size			0x01
#define F498_BERT_NBYTE_size			0x03
#define F498_BERT_ERRCOUNT_L_size			0x08
#define F498_BERT_ERRCOUNT_H_size			0x08
#define F498_CLK_POLARITY_size			0x01
#define F498_FEC_TYPE_size			0x01
#define F498_SYNC_STRIP_size			0x01
#define F498_TS_SWAP_size			0x01
#define F498_OUTFORMAT_size			0x02
#define F498_CI_DIVRANGE_size			0x08
#define F498_SMOOTHER_CNT_L_size			0x08
#define F498_SMOOTHERDIV_EN_L_size			0x05
#define F498_SMOOTHER_CNT_H_size			0x03
#define F498_SMOOTH_BYPASS_size			0x01
#define F498_SWAP_IQ_size			0x01
#define F498_SMOOTHERDIV_EN_H_size			0x04
#define F498_TS_NUMBER_size			0x04
#define F498_SEL_MODE_size			0x01
#define F498_CHECK_ERROR_BIT_size			0x01
#define F498_CHCK_F_SYNC_size			0x01
#define F498_H_MODE_size			0x01
#define F498_D_V_MODE_size			0x01
#define F498_MODE_size			0x02
#define F498_SYNC_IN_COUNT_size			0x04
#define F498_SYNC_OUT_COUNT_size			0x04
#define F498_TS_ID_L_size			0x08
#define F498_TS_ID_H_size			0x08
#define F498_ON_ID_L_size			0x08
#define F498_ON_ID_H_size			0x08
#define F498_RECEIVE_STATUS_L_size			0x08
#define F498_RECEIVE_STATUS_LH_size			0x08
#define F498_RECEIVE_STATUS_HL_size			0x08
#define F498_RECEIVE_STATUS_HH_size			0x06
#define F498_TS_STATUS_L_size			0x08
#define F498_TS_STATUS_H_size			0x07
#define F498_ERROR_size			0x01
#define F498_EMERGENCY_size			0x01
#define F498_CRE_TS_size			0x02
#define F498_VER_size			0x03
#define F498_M_LOCK_size			0x01
#define F498_UPDATE_READY_size			0x01
#define F498_END_FRAME_HEADER_size			0x01
#define F498_CONTCNT_size			0x01
#define F498_TS_IDENTIFIER_SEL_size			0x04
#define F498_ON_ID_I_L_size			0x08
#define F498_ON_ID_I_H_size			0x08
#define F498_TS_ID_I_L_size			0x08
#define F498_TS_ID_I_H_size			0x08
#define F498_MPEG_DIS_size			0x01
#define F498_MPEG_HDR_DIS_size			0x01
#define F498_RS_FLAG_size			0x01
#define F498_MPEG_FLAG_size			0x01
#define F498_MPEG_GET_size			0x04
#define F498_MPEG_DROP_size			0x06
#define F498_VITERBI_SYNC_GET_size			0x08
#define F498_VITERBI_SYNC_DROP_size			0x08
#define F498_VITERBI_SYN_GO_size			0x06
#define F498_VITERBI_SYN_STOP_size			0x06
#define F498_FRAME_SYNC_DROP_size			0x04
#define F498_FRAME_SYNC_GET_size			0x04
#define F498_INTERLEAVER_DEPTH_size			0x04
#define F498_RS_CORR_CNT_CLEAR_size			0x01
#define F498_RS_UNERR_CNT_CLEAR_size			0x01
#define F498_RS_4_ERROR_size			0x01
#define F498_RS_CLR_ERR_size			0x01
#define F498_RS_CLR_UNC_size			0x01
#define F498_RS_RATE_ADJ_size			0x03
#define F498_DEPTH_AUTOMODE_size			0x01
#define F498_FRAME_SYNC_COUNT_size			0x06
#define F498_MPEG_SYNC_size			0x01
#define F498_VITERBI_I_SYNC_size			0x01
#define F498_VITERBI_Q_SYNC_size			0x01
#define F498_COMB_STATE_size			0x02
#define F498_VITERBI_RATE_I_size			0x08
#define F498_VITERBI_RATE_Q_size			0x08
#define F498_RS_CORRECTED_COUNT_LO_size			0x08
#define F498_RS_CORRECTED_COUNT_HI_size			0x08
#define F498_RS_UNERRORED_COUNT_LO_size			0x08
#define F498_RS_UNERRORED_COUNT_HI_size			0x08
#define F498_RS_UNC_COUNT_LO_size			0x08
#define F498_RS_UNC_COUNT_HI_size			0x04
#define F498_RS_RATE_LO_size			0x08
#define F498_RS_RATE_HI_size			0x02
#define F498_TX_INTERLEAVE_DEPTH_size			0x04
#define F498_RS_ERROR_COUNT_L_size			0x08
#define F498_RS_ERROR_COUNT_H_size			0x04
#define F498_IQ_SWAP_size			0x01
#define F498_OOB_CLK_INV_size			0x01
#define F498_CRX_RATIO_size			0x06
#define F498_DRX_DELAY_size			0x06
#define R498_CTRL_0_size			0x08
#define R498_CTRL_1_size			0x08
#define R498_CTRL_2_size			0x08
#define R498_IT_STATUS1_size			0x08
#define R498_IT_STATUS2_size			0x08
#define R498_IT_EN1_size			0x08
#define R498_IT_EN2_size			0x08
#define R498_CTRL_STATUS_size			0x08
#define R498_TEST_CTL_size			0x08
#define R498_AGC_CTL_size			0x08
#define R498_AGC_IF_CFG_size			0x08
#define R498_AGC_RF_CFG_size			0x08
#define R498_AGC_PWM_CFG_size			0x08
#define R498_AGC_PWR_REF_L_size			0x08
#define R498_AGC_PWR_REF_H_size			0x08
#define R498_AGC_RF_TH_L_size			0x08
#define R498_AGC_RF_TH_H_size			0x08
#define R498_AGC_IF_LTH_L_size			0x08
#define R498_AGC_IF_LTH_H_size			0x08
#define R498_AGC_IF_HTH_L_size			0x08
#define R498_AGC_IF_HTH_H_size			0x08
#define R498_AGC_PWR_RD_L_size			0x08
#define R498_AGC_PWR_RD_M_size			0x08
#define R498_AGC_PWR_RD_H_size			0x08
#define R498_AGC_PWM_IFCMD_L_size			0x08
#define R498_AGC_PWM_IFCMD_H_size			0x08
#define R498_AGC_PWM_RFCMD_L_size			0x08
#define R498_AGC_PWM_RFCMD_H_size			0x08
#define R498_IQDEM_CFG_size			0x08
#define R498_MIX_NCO_LL_size			0x08
#define R498_MIX_NCO_HL_size			0x08
#define R498_MIX_NCO_HH_size			0x08
#define R498_MIX_NCO_size			0x18
#define R498_SRC_NCO_LL_size			0x08
#define R498_SRC_NCO_LH_size			0x08
#define R498_SRC_NCO_HL_size			0x08
#define R498_SRC_NCO_HH_size			0x08
#define R498_SRC_NCO_size			0x20
#define R498_IQDEM_GAIN_SRC_L_size			0x08
#define R498_IQDEM_GAIN_SRC_H_size			0x08
#define R498_IQDEM_DCRM_CFG_LL_size			0x08
#define R498_IQDEM_DCRM_CFG_LH_size			0x08
#define R498_IQDEM_DCRM_CFG_HL_size			0x08
#define R498_IQDEM_DCRM_CFG_HH_size			0x08
#define R498_IQDEM_ADJ_COEFF0_size			0x08
#define R498_IQDEM_ADJ_COEFF1_size			0x08
#define R498_IQDEM_ADJ_COEFF2_size			0x08
#define R498_IQDEM_ADJ_COEFF3_size			0x08
#define R498_IQDEM_ADJ_COEFF4_size			0x08
#define R498_IQDEM_ADJ_COEFF5_size			0x08
#define R498_IQDEM_ADJ_COEFF6_size			0x08
#define R498_IQDEM_ADJ_COEFF7_size			0x08
#define R498_IQDEM_ADJ_EN_size			0x08
#define R498_IQDEM_ADJ_AGC_REF_size			0x08
#define R498_ALLPASSFILT1_size			0x08
#define R498_ALLPASSFILT2_size			0x08
#define R498_ALLPASSFILT3_size			0x08
#define R498_ALLPASSFILT4_size			0x08
#define R498_ALLPASSFILT5_size			0x08
#define R498_ALLPASSFILT6_size			0x08
#define R498_ALLPASSFILT7_size			0x08
#define R498_ALLPASSFILT8_size			0x08
#define R498_ALLPASSFILT9_size			0x08
#define R498_ALLPASSFILT10_size			0x08
#define R498_ALLPASSFILT11_size			0x08
#define R498_TRL_AGC_CFG_size			0x08
#define R498_TRL_LPF_CFG_size			0x08
#define R498_TRL_LPF_ACQ_GAIN_size			0x08
#define R498_TRL_LPF_TRK_GAIN_size			0x08
#define R498_TRL_LPF_OUT_GAIN_size			0x08
#define R498_TRL_LOCKDET_LTH_size			0x08
#define R498_TRL_LOCKDET_HTH_size			0x08
#define R498_TRL_LOCKDET_TRGVAL_size			0x08
#define R498_FSM_STATE_size			0x08
#define R498_FSM_CTL_size			0x08
#define R498_FSM_STS_size			0x08
#define R498_FSM_SNR0_HTH_size			0x08
#define R498_FSM_SNR1_HTH_size			0x08
#define R498_FSM_SNR2_HTH_size			0x08
#define R498_FSM_SNR0_LTH_size			0x08
#define R498_FSM_SNR1_LTH_size			0x08
#define R498_FSM_RCA_HTH_size			0x08
#define R498_FSM_TEMPO_size			0x08
#define R498_FSM_CONFIG_size			0x08
#define R498_EQU_I_TESTTAP_L_size			0x08
#define R498_EQU_I_TESTTAP_M_size			0x08
#define R498_EQU_I_TESTTAP_H_size			0x08
#define R498_EQU_TESTAP_CFG_size			0x08
#define R498_EQU_Q_TESTTAP_L_size			0x08
#define R498_EQU_Q_TESTTAP_M_size			0x08
#define R498_EQU_Q_TESTTAP_H_size			0x08
#define R498_EQU_TAP_CTRL_size			0x08
#define R498_EQU_CTR_CRL_CONTROL_L_size			0x08
#define R498_EQU_CTR_CRL_CONTROL_H_size			0x08
#define R498_EQU_CTR_HIPOW_L_size			0x08
#define R498_EQU_CTR_HIPOW_H_size			0x08
#define R498_EQU_I_EQU_LO_size			0x08
#define R498_EQU_I_EQU_HI_size			0x08
#define R498_EQU_Q_EQU_LO_size			0x08
#define R498_EQU_Q_EQU_HI_size			0x08
#define R498_EQU_MAPPER_size			0x08
#define R498_EQU_SWEEP_RATE_size			0x08
#define R498_EQU_SNR_LO_size			0x08
#define R498_EQU_SNR_HI_size			0x08
#define R498_EQU_GAMMA_LO_size			0x08
#define R498_EQU_GAMMA_HI_size			0x08
#define R498_EQU_ERR_GAIN_size			0x08
#define R498_EQU_RADIUS_size			0x08
#define R498_EQU_FFE_MAINTAP_size			0x08
#define R498_EQU_FFE_LEAKAGE_size			0x08
#define R498_EQU_FFE_MAINTAP_POS_size			0x08
#define R498_EQU_GAIN_WIDE_size			0x08
#define R498_EQU_GAIN_NARROW_size			0x08
#define R498_EQU_CTR_LPF_GAIN_size			0x08
#define R498_EQU_CRL_LPF_GAIN_size			0x08
#define R498_EQU_GLOBAL_GAIN_size			0x08
#define R498_EQU_CRL_LD_SEN_size			0x08
#define R498_EQU_CRL_LD_VAL_size			0x08
#define R498_EQU_CRL_TFR_size			0x08
#define R498_EQU_CRL_BISTH_LO_size			0x08
#define R498_EQU_CRL_BISTH_HI_size			0x08
#define R498_EQU_SWEEP_RANGE_LO_size			0x08
#define R498_EQU_SWEEP_RANGE_HI_size			0x08
#define R498_EQU_CRL_LIMITER_size			0x08
#define R498_EQU_MODULUS_MAP_size			0x08
#define R498_EQU_PNT_GAIN_size			0x08
#define R498_FEC_AC_CTR_0_size			0x08
#define R498_FEC_AC_CTR_1_size			0x08
#define R498_FEC_AC_CTR_2_size			0x08
#define R498_FEC_AC_CTR_3_size			0x08
#define R498_FEC_STATUS_size			0x08
#define R498_RS_COUNTER_0_size			0x08
#define R498_RS_COUNTER_1_size			0x08
#define R498_RS_COUNTER_2_size			0x08
#define R498_RS_COUNTER_3_size			0x08
#define R498_RS_COUNTER_4_size			0x08
#define R498_RS_COUNTER_5_size			0x08
#define R498_BERT_0_size			0x08
#define R498_BERT_1_size			0x08
#define R498_BERT_2_size			0x08
#define R498_BERT_3_size			0x08
#define R498_OUTFORMAT_0_size			0x08
#define R498_OUTFORMAT_1_size			0x08
#define R498_SMOOTHER_0_size			0x08
#define R498_SMOOTHER_1_size			0x08
#define R498_SMOOTHER_2_size			0x08
#define R498_TSMF_CTRL_0_size			0x08
#define R498_TSMF_CTRL_1_size			0x08
#define R498_TSMF_CTRL_3_size			0x08
#define R498_TS_ON_ID_0_size			0x08
#define R498_TS_ON_ID_1_size			0x08
#define R498_TS_ON_ID_2_size			0x08
#define R498_TS_ON_ID_3_size			0x08
#define R498_RE_STATUS_0_size			0x08
#define R498_RE_STATUS_1_size			0x08
#define R498_RE_STATUS_2_size			0x08
#define R498_RE_STATUS_3_size			0x08
#define R498_TS_STATUS_0_size			0x08
#define R498_TS_STATUS_1_size			0x08
#define R498_TS_STATUS_2_size			0x08
#define R498_TS_STATUS_3_size			0x08
#define R498_T_O_ID_0_size			0x08
#define R498_T_O_ID_1_size			0x08
#define R498_T_O_ID_2_size			0x08
#define R498_T_O_ID_3_size			0x08
#define R498_MPEG_CTL_size			0x08
#define R498_MPEG_SYNC_ACQ_size			0x08
#define R498_FECB_MPEG_SYNC_LOSS_size			0x08
#define R498_VITERBI_SYNC_ACQ_size			0x08
#define R498_VITERBI_SYNC_LOSS_size			0x08
#define R498_VITERBI_SYNC_GO_size			0x08
#define R498_VITERBI_SYNC_STOP_size			0x08
#define R498_FS_SYNC_size			0x08
#define R498_IN_DEPTH_size			0x08
#define R498_RS_CONTROL_size			0x08
#define R498_DEINT_CONTROL_size			0x08
#define R498_SYNC_STAT_size			0x08
#define R498_VITERBI_I_RATE_size			0x08
#define R498_VITERBI_Q_RATE_size			0x08
#define R498_RS_CORR_COUNT_L_size			0x08
#define R498_RS_CORR_COUNT_H_size			0x08
#define R498_RS_UNERR_COUNT_L_size			0x08
#define R498_RS_UNERR_COUNT_H_size			0x08
#define R498_RS_UNC_COUNT_L_size			0x08
#define R498_RS_UNC_COUNT_H_size			0x08
#define R498_RS_RATE_L_size			0x08
#define R498_RS_RATE_H_size			0x08
#define R498_TX_INTERLEAVER_DEPTH_size			0x08
#define R498_RS_ERR_CNT_L_size			0x08
#define R498_RS_ERR_CNT_H_size			0x08
#define R498_OOB_DUTY_size			0x08
#define R498_OOB_DELAY_size			0x08



#define F498_STANDBY_offset			0x08
#define F498_SOFT_RST_offset			0x17
#define F498_EQU_RST_offset			0x13
#define F498_CRL_RST_offset			0x12
#define F498_TRL_RST_offset			0x11
#define F498_AGC_RST_offset			0x10
#define F498_FEC_NHW_RESET_offset			0x1F
#define F498_DEINT_RST_offset			0x1B
#define F498_FECAC_RS_RST_offset			0x1A
#define F498_FECB_RS_RST_offset			0x19
#define F498_VITERBI_RST_offset			0x18
#define F498_SWEEP_OUT_offset			0x07
#define F498_FSM_CRL_offset			0x06
#define F498_CRL_LOCK_offset			0x05
#define F498_MFSM_offset			0x04
#define F498_TRL_LOCK_offset			0x03
#define F498_TRL_AGC_LIMIT_offset			0x02
#define F498_ADJ_AGC_LOCK_offset			0x01
#define F498_AGC_LOCK_offset			0x00
#define F498_TSMF_CNT_offset			0x0F
#define F498_TSMF_EOF_offset			0x0E
#define F498_TSMF_RDY_offset			0x0D
#define F498_FEC_NOCORR_offset			0x0C
#define F498_FEC_LOCK_offset			0x0B
#define F498_DEINT_LOCK_offset			0x0A
#define F498_TAPMON_ALARM_offset			0x08
#define F498_SWEEP_OUTE_offset			0x17
#define F498_FSM_CRLE_offset			0x16
#define F498_CRL_LOCKE_offset			0x15
#define F498_MFSME_offset			0x14
#define F498_TRL_LOCKE_offset			0x13
#define F498_TRL_AGC_LIMITE_offset			0x12
#define F498_ADJ_AGC_LOCKE_offset			0x11
#define F498_AGC_LOCKE_offset			0x10
#define F498_TSMF_CNTE_offset			0x1F
#define F498_TSMF_EOFE_offset			0x1E
#define F498_TSMF_RDYE_offset			0x1D
#define F498_FEC_NOCORRE_offset			0x1C
#define F498_FEC_LOCKE_offset			0x1B
#define F498_DEINT_LOCKE_offset			0x1A
#define F498_TAPMON_ALARME_offset			0x18
#define F498_QAMFEC_LOCK_offset			0x02
#define F498_TSMF_LOCK_offset			0x01
#define F498_TSMF_ERROR_offset			0x00
#define F498_TST_BUS_SEL_offset			0x18
#define F498_AGC_LCK_TH_offset			0x04
#define F498_AGC_ACCUMRSTSEL_offset			0x00
#define F498_AGC_IF_BWSEL_offset			0x0C
#define F498_AGC_IF_FREEZE_offset			0x09
#define F498_AGC_RF_BWSEL_offset			0x14
#define F498_AGC_RF_FREEZE_offset			0x11
#define F498_AGC_RF_PWM_TST_offset			0x1F
#define F498_AGC_RF_PWM_INV_offset			0x1E
#define F498_AGC_IF_PWM_TST_offset			0x1B
#define F498_AGC_IF_PWM_INV_offset			0x1A
#define F498_AGC_PWM_CLKDIV_offset			0x18
#define F498_AGC_PWRREF_LO_offset			0x00
#define F498_AGC_PWRREF_HI_offset			0x08
#define F498_AGC_RF_TH_LO_offset			0x10
#define F498_AGC_RF_TH_HI_offset			0x18
#define F498_AGC_IF_THLO_LO_offset			0x00
#define F498_AGC_IF_THLO_HI_offset			0x08
#define F498_AGC_IF_THHI_LO_offset			0x10
#define F498_AGC_IF_THHI_HI_offset			0x18
#define F498_AGC_PWR_WORD_LO_offset			0x00
#define F498_AGC_PWR_WORD_ME_offset			0x08
#define F498_AGC_PWR_WORD_HI_offset			0x10
#define F498_AGC_IF_PWMCMD_LO_offset			0x00
#define F498_AGC_IF_PWMCMD_HI_offset			0x08
#define F498_AGC_RF_PWMCMD_LO_offset			0x10
#define F498_AGC_RF_PWMCMD_HI_offset			0x18
#define F498_IQDEM_CLK_SEL_offset			0x02
#define F498_IQDEM_INVIQ_offset			0x01
#define F498_IQDEM_A2DTYPE_offset			0x00
#define F498_MIX_NCO_INC_LL_offset			0x08
#define F498_MIX_NCO_INC_HL_offset			0x10
#define F498_MIX_NCO_INVCNST_offset			0x1F
#define F498_MIX_NCO_INC_HH_offset			0x18
#define F498_SRC_NCO_INC_LL_offset			0x00
#define F498_SRC_NCO_INC_LH_offset			0x08
#define F498_SRC_NCO_INC_HL_offset			0x10
#define F498_SRC_NCO_INC_HH_offset			0x18
#define F498_GAIN_SRC_LO_offset			0x00
#define F498_GAIN_SRC_HI_offset			0x08
#define F498_DCRM0_DCIN_L_offset			0x00
#define F498_DCRM1_I_DCIN_L_offset			0x0A
#define F498_DCRM0_DCIN_H_offset			0x08
#define F498_DCRM1_Q_DCIN_L_offset			0x14
#define F498_DCRM1_I_DCIN_H_offset			0x10
#define F498_DCRM1_FRZ_offset			0x1F
#define F498_DCRM0_FRZ_offset			0x1E
#define F498_DCRM1_Q_DCIN_H_offset			0x18
#define F498_ADJIIR_COEFF10_L_offset			0x00
#define F498_ADJIIR_COEFF11_L_offset			0x0A
#define F498_ADJIIR_COEFF10_H_offset			0x08
#define F498_ADJIIR_COEFF12_L_offset			0x14
#define F498_ADJIIR_COEFF11_H_offset			0x10
#define F498_ADJIIR_COEFF20_L_offset			0x1E
#define F498_ADJIIR_COEFF12_H_offset			0x18
#define F498_ADJIIR_COEFF20_H_offset			0x00
#define F498_ADJIIR_COEFF21_L_offset			0x08
#define F498_ADJIIR_COEFF22_L_offset			0x12
#define F498_ADJIIR_COEFF21_H_offset			0x10
#define F498_ADJIIR_COEFF22_H_offset			0x18
#define F498_ALLPASSFILT_EN_offset			0x03
#define F498_ADJ_AGC_EN_offset			0x02
#define F498_ADJ_COEFF_FRZ_offset			0x01
#define F498_ADJ_EN_offset			0x00
#define F498_ADJ_AGC_REF_offset			0x08
#define F498_ALLPASSFILT_COEFF1_LO_offset			0x00
#define F498_ALLPASSFILT_COEFF1_ME_offset			0x08
#define F498_ALLPASSFILT_COEFF2_LO_offset			0x16
#define F498_ALLPASSFILT_COEFF1_HI_offset			0x10
#define F498_ALLPASSFILT_COEFF2_ME_offset			0x18
#define F498_ALLPASSFILT_COEFF2_HI_offset			0x00
#define F498_ALLPASSFILT_COEFF3_LO_offset			0x0C
#define F498_ALLPASSFILT_COEFF2_HH_offset			0x08
#define F498_ALLPASSFILT_COEFF3_ME_offset			0x10
#define F498_ALLPASSFILT_COEFF3_HI_offset			0x18
#define F498_ALLPASSFILT_COEFF4_LO_offset			0x02
#define F498_ALLPASSFILT_COEFF3_HH_offset			0x00
#define F498_ALLPASSFILT_COEFF4_ME_offset			0x08
#define F498_ALLPASSFILT_COEFF4_HI_offset			0x10
#define F498_TRL_AGC_FREEZE_offset			0x07
#define F498_TRL_AGC_REF_offset			0x00
#define F498_NYQPOINT_INV_offset			0x07
#define F498_TRL_SHIFT_offset			0x05
#define F498_NYQ_COEFF_SEL_offset			0x02
#define F498_TRL_LPF_FREEZE_offset			0x01
#define F498_TRL_LPF_CRT_offset			0x00
#define F498_TRL_GDIR_ACQ_offset			0x0C
#define F498_TRL_GINT_ACQ_offset			0x08
#define F498_TRL_GDIR_TRK_offset			0x14
#define F498_TRL_GINT_TRK_offset			0x10
#define F498_TRL_GAIN_OUT_offset			0x18
#define F498_TRL_LCK_THLO_offset			0x00
#define F498_TRL_LCK_THHI_offset			0x08
#define F498_TRL_LCK_TRG_offset			0x10
#define F498_CRL_DFE_offset			0x07
#define F498_DFE_START_offset			0x06
#define F498_CTRLG_START_offset			0x04
#define F498_FSM_FORCESTATE_offset			0x00
#define F498_FEC2_EN_offset			0x0E
#define F498_SIT_EN_offset			0x0D
#define F498_TRL_AHEAD_offset			0x0C
#define F498_TRL2_EN_offset			0x0B
#define F498_FSM_RCA_EN_offset			0x0A
#define F498_FSM_BKP_DIS_offset			0x09
#define F498_FSM_FORCE_EN_offset			0x08
#define F498_FSM_STATUS_offset			0x10
#define F498_SNR0_HTH_offset			0x18
#define F498_SNR1_HTH_offset			0x00
#define F498_SNR2_HTH_offset			0x08
#define F498_SNR0_LTH_offset			0x10
#define F498_SNR1_LTH_offset			0x18
#define F498_SNR3_HTH_L_offset			0x04
#define F498_RCA_HTH_offset			0x00
#define F498_SIT_offset			0x0E
#define F498_WST_offset			0x0B
#define F498_ELT_offset			0x09
#define F498_SNR3_HTH_H_offset			0x08
#define F498_FEC2_DFEOFF_offset			0x12
#define F498_PNT_STATE_offset			0x11
#define F498_MODMAP_STATE_offset			0x10
#define F498_I_TEST_TAP_L_offset			0x00
#define F498_I_TEST_TAP_M_offset			0x08
#define F498_I_TEST_TAP_H_offset			0x10
#define F498_TEST_FFE_DFE_SEL_offset			0x1E
#define F498_TEST_TAP_SELECT_offset			0x18
#define F498_Q_TEST_TAP_L_offset			0x00
#define F498_Q_TEST_TAP_M_offset			0x08
#define F498_Q_TEST_TAP_H_offset			0x10
#define F498_MTAP_FRZ_offset			0x1C
#define F498_PRE_FREEZE_offset			0x1B
#define F498_DFE_TAPMON_EN_offset			0x1A
#define F498_FFE_TAPMON_EN_offset			0x19
#define F498_MTAP_ONLY_offset			0x18
#define F498_EQU_CTR_CRL_CONTROL_LO_offset			0x00
#define F498_EQU_CTR_CRL_CONTROL_HI_offset			0x08
#define F498_CTR_HIPOW_L_offset			0x10
#define F498_CTR_HIPOW_H_offset			0x18
#define F498_EQU_I_EQU_L_offset			0x00
#define F498_EQU_I_EQU_H_offset			0x08
#define F498_EQU_Q_EQU_L_offset			0x10
#define F498_EQU_Q_EQU_H_offset			0x18
#define F498_QUAD_AUTO_offset			0x07
#define F498_QUAD_INV_offset			0x06
#define F498_QAM_MODE_offset			0x00
#define F498_SNR_PER_offset			0x0E
#define F498_SWEEP_RATE_offset			0x08
#define F498_SNR_LO_offset			0x10
#define F498_SNR_HI_offset			0x18
#define F498_GAMMA_LO_offset			0x00
#define F498_GAMMA_ME_offset			0x08
#define F498_RCAMU_offset			0x14
#define F498_CMAMU_offset			0x11
#define F498_GAMMA_HI_offset			0x10
#define F498_RADIUS_offset			0x18
#define F498_FFE_MAINTAP_INIT_offset			0x00
#define F498_LEAK_PER_offset			0x14
#define F498_EQU_OUTSEL_offset			0x11
#define F498_PNT2DFE_offset			0x10
#define F498_FFE_LEAK_EN_offset			0x1F
#define F498_DFE_LEAK_EN_offset			0x1E
#define F498_FFE_MAINTAP_POS_offset			0x18
#define F498_DFE_GAIN_WIDE_offset			0x04
#define F498_FFE_GAIN_WIDE_offset			0x00
#define F498_DFE_GAIN_NARROW_offset			0x0C
#define F498_FFE_GAIN_NARROW_offset			0x08
#define F498_CTR_GTO_offset			0x17
#define F498_CTR_GDIR_offset			0x14
#define F498_SWEEP_EN_offset			0x13
#define F498_CTR_GINT_offset			0x10
#define F498_CRL_GTO_offset			0x1F
#define F498_CRL_GDIR_offset			0x1C
#define F498_SWEEP_DIR_offset			0x1B
#define F498_CRL_GINT_offset			0x18
#define F498_CRL_GAIN_offset			0x03
#define F498_CTR_INC_GAIN_offset			0x02
#define F498_CTR_FRAC_offset			0x00
#define F498_CTR_BADPOINT_EN_offset			0x0F
#define F498_CTR_GAIN_offset			0x0C
#define F498_LIMANEN_offset			0x0B
#define F498_CRL_LD_SEN_offset			0x08
#define F498_CRL_BISTH_LIMIT_offset			0x17
#define F498_CARE_EN_offset			0x16
#define F498_CRL_LD_PER_offset			0x14
#define F498_CRL_LD_WST_offset			0x12
#define F498_CRL_LD_TFS_offset			0x10
#define F498_CRL_LD_TFR_offset			0x18
#define F498_CRL_BISTH_LO_offset			0x00
#define F498_CRL_BISTH_HI_offset			0x08
#define F498_SWEEP_RANGE_LO_offset			0x10
#define F498_SWEEP_RANGE_HI_offset			0x18
#define F498_BISECTOR_EN_offset			0x07
#define F498_PHEST128_EN_offset			0x06
#define F498_CRL_LIM_offset			0x00
#define F498_PNT_DEPTH_offset			0x0D
#define F498_MODULUS_CMP_offset			0x08
#define F498_PNT_EN_offset			0x17
#define F498_MODULUSMAP_EN_offset			0x16
#define F498_PNT_GAIN_offset			0x10
#define F498_RST_RS_offset			0x07
#define F498_RST_DI_offset			0x06
#define F498_BE_BYPASS_offset			0x05
#define F498_REFRESH47_offset			0x04
#define F498_CT_NBST_offset			0x03
#define F498_TEI_ENA_offset			0x02
#define F498_DS_ENA_offset			0x01
#define F498_TSMF_EN_offset			0x00
#define F498_DEINT_DEPTH_offset			0x08
#define F498_DEINT_M_offset			0x13
#define F498_DIS_UNLOCK_offset			0x12
#define F498_DESCR_MODE_offset			0x10
#define F498_DI_UNLOCK_offset			0x1F
#define F498_DI_FREEZE_offset			0x1E
#define F498_MISMATCH_offset			0x1C
#define F498_ACQ_MODE_offset			0x1A
#define F498_TRK_MODE_offset			0x18
#define F498_DEINT_SMCNTR_offset			0x05
#define F498_DEINT_SYNCSTATE_offset			0x03
#define F498_DEINT_SYNLOST_offset			0x02
#define F498_DESCR_SYNCSTATE_offset			0x01
#define F498_BK_CT_L_offset			0x10
#define F498_BK_CT_H_offset			0x18
#define F498_CORR_CT_L_offset			0x00
#define F498_CORR_CT_H_offset			0x08
#define F498_UNCORR_CT_L_offset			0x10
#define F498_UNCORR_CT_H_offset			0x18
#define F498_RS_NOCORR_offset			0x02
#define F498_CT_HOLD_offset			0x01
#define F498_CT_CLEAR_offset			0x00
#define F498_BERT_ON_offset			0x0D
#define F498_BERT_ERR_SRC_offset			0x0C
#define F498_BERT_ERR_MODE_offset			0x0B
#define F498_BERT_NBYTE_offset			0x08
#define F498_BERT_ERRCOUNT_L_offset			0x10
#define F498_BERT_ERRCOUNT_H_offset			0x18
#define F498_CLK_POLARITY_offset			0x07
#define F498_FEC_TYPE_offset			0x06
#define F498_SYNC_STRIP_offset			0x03
#define F498_TS_SWAP_offset			0x02
#define F498_OUTFORMAT_offset			0x00
#define F498_CI_DIVRANGE_offset			0x08
#define F498_SMOOTHER_CNT_L_offset			0x00
#define F498_SMOOTHERDIV_EN_L_offset			0x0B
#define F498_SMOOTHER_CNT_H_offset			0x08
#define F498_SMOOTH_BYPASS_offset			0x15
#define F498_SWAP_IQ_offset			0x14
#define F498_SMOOTHERDIV_EN_H_offset			0x10
#define F498_TS_NUMBER_offset			0x01
#define F498_SEL_MODE_offset			0x00
#define F498_CHECK_ERROR_BIT_offset			0x0F
#define F498_CHCK_F_SYNC_offset			0x0E
#define F498_H_MODE_offset			0x0B
#define F498_D_V_MODE_offset			0x0A
#define F498_MODE_offset			0x08
#define F498_SYNC_IN_COUNT_offset			0x1C
#define F498_SYNC_OUT_COUNT_offset			0x18
#define F498_TS_ID_L_offset			0x00
#define F498_TS_ID_H_offset			0x08
#define F498_ON_ID_L_offset			0x10
#define F498_ON_ID_H_offset			0x18
#define F498_RECEIVE_STATUS_L_offset			0x00
#define F498_RECEIVE_STATUS_LH_offset			0x08
#define F498_RECEIVE_STATUS_HL_offset			0x10
#define F498_RECEIVE_STATUS_HH_offset			0x18
#define F498_TS_STATUS_L_offset			0x00
#define F498_TS_STATUS_H_offset			0x08
#define F498_ERROR_offset			0x17
#define F498_EMERGENCY_offset			0x16
#define F498_CRE_TS_offset			0x14
#define F498_VER_offset			0x11
#define F498_M_LOCK_offset			0x10
#define F498_UPDATE_READY_offset			0x1F
#define F498_END_FRAME_HEADER_offset			0x1E
#define F498_CONTCNT_offset			0x1D
#define F498_TS_IDENTIFIER_SEL_offset			0x18
#define F498_ON_ID_I_L_offset			0x00
#define F498_ON_ID_I_H_offset			0x08
#define F498_TS_ID_I_L_offset			0x10
#define F498_TS_ID_I_H_offset			0x18
#define F498_MPEG_DIS_offset			0x0D
#define F498_MPEG_HDR_DIS_offset			0x0A
#define F498_RS_FLAG_offset			0x09
#define F498_MPEG_FLAG_offset			0x08
#define F498_MPEG_GET_offset			0x10
#define F498_MPEG_DROP_offset			0x18
#define F498_VITERBI_SYNC_GET_offset			0x08
#define F498_VITERBI_SYNC_DROP_offset			0x10
#define F498_VITERBI_SYN_GO_offset			0x18
#define F498_VITERBI_SYN_STOP_offset			0x00
#define F498_FRAME_SYNC_DROP_offset			0x0C
#define F498_FRAME_SYNC_GET_offset			0x08
#define F498_INTERLEAVER_DEPTH_offset			0x10
#define F498_RS_CORR_CNT_CLEAR_offset			0x1F
#define F498_RS_UNERR_CNT_CLEAR_offset			0x1E
#define F498_RS_4_ERROR_offset			0x1D
#define F498_RS_CLR_ERR_offset			0x1C
#define F498_RS_CLR_UNC_offset			0x1B
#define F498_RS_RATE_ADJ_offset			0x18
#define F498_DEPTH_AUTOMODE_offset			0x06
#define F498_FRAME_SYNC_COUNT_offset			0x00
#define F498_MPEG_SYNC_offset			0x0C
#define F498_VITERBI_I_SYNC_offset			0x0B
#define F498_VITERBI_Q_SYNC_offset			0x0A
#define F498_COMB_STATE_offset			0x08
#define F498_VITERBI_RATE_I_offset			0x10
#define F498_VITERBI_RATE_Q_offset			0x18
#define F498_RS_CORRECTED_COUNT_LO_offset			0x00
#define F498_RS_CORRECTED_COUNT_HI_offset			0x08
#define F498_RS_UNERRORED_COUNT_LO_offset			0x10
#define F498_RS_UNERRORED_COUNT_HI_offset			0x18
#define F498_RS_UNC_COUNT_LO_offset			0x00
#define F498_RS_UNC_COUNT_HI_offset			0x08
#define F498_RS_RATE_LO_offset			0x10
#define F498_RS_RATE_HI_offset			0x18
#define F498_TX_INTERLEAVE_DEPTH_offset			0x00
#define F498_RS_ERROR_COUNT_L_offset			0x08
#define F498_RS_ERROR_COUNT_H_offset			0x10
#define F498_IQ_SWAP_offset			0x07
#define F498_OOB_CLK_INV_offset			0x06
#define F498_CRX_RATIO_offset			0x00
#define F498_DRX_DELAY_offset			0x08
#define R498_CTRL_0_offset			0x08
#define R498_CTRL_1_offset			0x10
#define R498_CTRL_2_offset			0x18
#define R498_IT_STATUS1_offset			0x00
#define R498_IT_STATUS2_offset			0x08
#define R498_IT_EN1_offset			0x10
#define R498_IT_EN2_offset			0x18
#define R498_CTRL_STATUS_offset			0x00
#define R498_TEST_CTL_offset			0x18
#define R498_AGC_CTL_offset			0x00
#define R498_AGC_IF_CFG_offset			0x08
#define R498_AGC_RF_CFG_offset			0x10
#define R498_AGC_PWM_CFG_offset			0x18
#define R498_AGC_PWR_REF_L_offset			0x00
#define R498_AGC_PWR_REF_H_offset			0x08
#define R498_AGC_RF_TH_L_offset			0x10
#define R498_AGC_RF_TH_H_offset			0x18
#define R498_AGC_IF_LTH_L_offset			0x00
#define R498_AGC_IF_LTH_H_offset			0x08
#define R498_AGC_IF_HTH_L_offset			0x10
#define R498_AGC_IF_HTH_H_offset			0x18
#define R498_AGC_PWR_RD_L_offset			0x00
#define R498_AGC_PWR_RD_M_offset			0x08
#define R498_AGC_PWR_RD_H_offset			0x10
#define R498_AGC_PWM_IFCMD_L_offset			0x00
#define R498_AGC_PWM_IFCMD_H_offset			0x08
#define R498_AGC_PWM_RFCMD_L_offset			0x10
#define R498_AGC_PWM_RFCMD_H_offset			0x18
#define R498_IQDEM_CFG_offset			0x00
#define R498_MIX_NCO_LL_offset			0x08
#define R498_MIX_NCO_HL_offset			0x10
#define R498_MIX_NCO_HH_offset			0x18
#define R498_MIX_NCO_offset			0x08
#define R498_SRC_NCO_LL_offset			0x00
#define R498_SRC_NCO_LH_offset			0x08
#define R498_SRC_NCO_HL_offset			0x10
#define R498_SRC_NCO_HH_offset			0x18
#define R498_SRC_NCO_offset			0x00
#define R498_IQDEM_GAIN_SRC_L_offset			0x00
#define R498_IQDEM_GAIN_SRC_H_offset			0x08
#define R498_IQDEM_DCRM_CFG_LL_offset			0x00
#define R498_IQDEM_DCRM_CFG_LH_offset			0x08
#define R498_IQDEM_DCRM_CFG_HL_offset			0x10
#define R498_IQDEM_DCRM_CFG_HH_offset			0x18
#define R498_IQDEM_ADJ_COEFF0_offset			0x00
#define R498_IQDEM_ADJ_COEFF1_offset			0x08
#define R498_IQDEM_ADJ_COEFF2_offset			0x10
#define R498_IQDEM_ADJ_COEFF3_offset			0x18
#define R498_IQDEM_ADJ_COEFF4_offset			0x00
#define R498_IQDEM_ADJ_COEFF5_offset			0x08
#define R498_IQDEM_ADJ_COEFF6_offset			0x10
#define R498_IQDEM_ADJ_COEFF7_offset			0x18
#define R498_IQDEM_ADJ_EN_offset			0x00
#define R498_IQDEM_ADJ_AGC_REF_offset			0x08
#define R498_ALLPASSFILT1_offset			0x00
#define R498_ALLPASSFILT2_offset			0x08
#define R498_ALLPASSFILT3_offset			0x10
#define R498_ALLPASSFILT4_offset			0x18
#define R498_ALLPASSFILT5_offset			0x00
#define R498_ALLPASSFILT6_offset			0x08
#define R498_ALLPASSFILT7_offset			0x10
#define R498_ALLPASSFILT8_offset			0x18
#define R498_ALLPASSFILT9_offset			0x00
#define R498_ALLPASSFILT10_offset			0x08
#define R498_ALLPASSFILT11_offset			0x10
#define R498_TRL_AGC_CFG_offset			0x00
#define R498_TRL_LPF_CFG_offset			0x00
#define R498_TRL_LPF_ACQ_GAIN_offset			0x08
#define R498_TRL_LPF_TRK_GAIN_offset			0x10
#define R498_TRL_LPF_OUT_GAIN_offset			0x18
#define R498_TRL_LOCKDET_LTH_offset			0x00
#define R498_TRL_LOCKDET_HTH_offset			0x08
#define R498_TRL_LOCKDET_TRGVAL_offset			0x10
#define R498_FSM_STATE_offset			0x00
#define R498_FSM_CTL_offset			0x08
#define R498_FSM_STS_offset			0x10
#define R498_FSM_SNR0_HTH_offset			0x18
#define R498_FSM_SNR1_HTH_offset			0x00
#define R498_FSM_SNR2_HTH_offset			0x08
#define R498_FSM_SNR0_LTH_offset			0x10
#define R498_FSM_SNR1_LTH_offset			0x18
#define R498_FSM_RCA_HTH_offset			0x00
#define R498_FSM_TEMPO_offset			0x08
#define R498_FSM_CONFIG_offset			0x10
#define R498_EQU_I_TESTTAP_L_offset			0x00
#define R498_EQU_I_TESTTAP_M_offset			0x08
#define R498_EQU_I_TESTTAP_H_offset			0x10
#define R498_EQU_TESTAP_CFG_offset			0x18
#define R498_EQU_Q_TESTTAP_L_offset			0x00
#define R498_EQU_Q_TESTTAP_M_offset			0x08
#define R498_EQU_Q_TESTTAP_H_offset			0x10
#define R498_EQU_TAP_CTRL_offset			0x18
#define R498_EQU_CTR_CRL_CONTROL_L_offset			0x00
#define R498_EQU_CTR_CRL_CONTROL_H_offset			0x08
#define R498_EQU_CTR_HIPOW_L_offset			0x10
#define R498_EQU_CTR_HIPOW_H_offset			0x18
#define R498_EQU_I_EQU_LO_offset			0x00
#define R498_EQU_I_EQU_HI_offset			0x08
#define R498_EQU_Q_EQU_LO_offset			0x10
#define R498_EQU_Q_EQU_HI_offset			0x18
#define R498_EQU_MAPPER_offset			0x00
#define R498_EQU_SWEEP_RATE_offset			0x08
#define R498_EQU_SNR_LO_offset			0x10
#define R498_EQU_SNR_HI_offset			0x18
#define R498_EQU_GAMMA_LO_offset			0x00
#define R498_EQU_GAMMA_HI_offset			0x08
#define R498_EQU_ERR_GAIN_offset			0x10
#define R498_EQU_RADIUS_offset			0x18
#define R498_EQU_FFE_MAINTAP_offset			0x00
#define R498_EQU_FFE_LEAKAGE_offset			0x10
#define R498_EQU_FFE_MAINTAP_POS_offset			0x18
#define R498_EQU_GAIN_WIDE_offset			0x00
#define R498_EQU_GAIN_NARROW_offset			0x08
#define R498_EQU_CTR_LPF_GAIN_offset			0x10
#define R498_EQU_CRL_LPF_GAIN_offset			0x18
#define R498_EQU_GLOBAL_GAIN_offset			0x00
#define R498_EQU_CRL_LD_SEN_offset			0x08
#define R498_EQU_CRL_LD_VAL_offset			0x10
#define R498_EQU_CRL_TFR_offset			0x18
#define R498_EQU_CRL_BISTH_LO_offset			0x00
#define R498_EQU_CRL_BISTH_HI_offset			0x08
#define R498_EQU_SWEEP_RANGE_LO_offset			0x10
#define R498_EQU_SWEEP_RANGE_HI_offset			0x18
#define R498_EQU_CRL_LIMITER_offset			0x00
#define R498_EQU_MODULUS_MAP_offset			0x08
#define R498_EQU_PNT_GAIN_offset			0x10
#define R498_FEC_AC_CTR_0_offset			0x00
#define R498_FEC_AC_CTR_1_offset			0x08
#define R498_FEC_AC_CTR_2_offset			0x10
#define R498_FEC_AC_CTR_3_offset			0x18
#define R498_FEC_STATUS_offset			0x00
#define R498_RS_COUNTER_0_offset			0x10
#define R498_RS_COUNTER_1_offset			0x18
#define R498_RS_COUNTER_2_offset			0x00
#define R498_RS_COUNTER_3_offset			0x08
#define R498_RS_COUNTER_4_offset			0x10
#define R498_RS_COUNTER_5_offset			0x18
#define R498_BERT_0_offset			0x00
#define R498_BERT_1_offset			0x08
#define R498_BERT_2_offset			0x10
#define R498_BERT_3_offset			0x18
#define R498_OUTFORMAT_0_offset			0x00
#define R498_OUTFORMAT_1_offset			0x08
#define R498_SMOOTHER_0_offset			0x00
#define R498_SMOOTHER_1_offset			0x08
#define R498_SMOOTHER_2_offset			0x10
#define R498_TSMF_CTRL_0_offset			0x00
#define R498_TSMF_CTRL_1_offset			0x08
#define R498_TSMF_CTRL_3_offset			0x18
#define R498_TS_ON_ID_0_offset			0x00
#define R498_TS_ON_ID_1_offset			0x08
#define R498_TS_ON_ID_2_offset			0x10
#define R498_TS_ON_ID_3_offset			0x18
#define R498_RE_STATUS_0_offset			0x00
#define R498_RE_STATUS_1_offset			0x08
#define R498_RE_STATUS_2_offset			0x10
#define R498_RE_STATUS_3_offset			0x18
#define R498_TS_STATUS_0_offset			0x00
#define R498_TS_STATUS_1_offset			0x08
#define R498_TS_STATUS_2_offset			0x10
#define R498_TS_STATUS_3_offset			0x18
#define R498_T_O_ID_0_offset			0x00
#define R498_T_O_ID_1_offset			0x08
#define R498_T_O_ID_2_offset			0x10
#define R498_T_O_ID_3_offset			0x18
#define R498_MPEG_CTL_offset			0x08
#define R498_MPEG_SYNC_ACQ_offset			0x10
#define R498_FECB_MPEG_SYNC_LOSS_offset			0x18
#define R498_VITERBI_SYNC_ACQ_offset			0x08
#define R498_VITERBI_SYNC_LOSS_offset			0x10
#define R498_VITERBI_SYNC_GO_offset			0x18
#define R498_VITERBI_SYNC_STOP_offset			0x00
#define R498_FS_SYNC_offset			0x08
#define R498_IN_DEPTH_offset			0x10
#define R498_RS_CONTROL_offset			0x18
#define R498_DEINT_CONTROL_offset			0x00
#define R498_SYNC_STAT_offset			0x08
#define R498_VITERBI_I_RATE_offset			0x10
#define R498_VITERBI_Q_RATE_offset			0x18
#define R498_RS_CORR_COUNT_L_offset			0x00
#define R498_RS_CORR_COUNT_H_offset			0x08
#define R498_RS_UNERR_COUNT_L_offset			0x10
#define R498_RS_UNERR_COUNT_H_offset			0x18
#define R498_RS_UNC_COUNT_L_offset			0x00
#define R498_RS_UNC_COUNT_H_offset			0x08
#define R498_RS_RATE_L_offset			0x10
#define R498_RS_RATE_H_offset			0x18
#define R498_TX_INTERLEAVER_DEPTH_offset			0x00
#define R498_RS_ERR_CNT_L_offset			0x08
#define R498_RS_ERR_CNT_H_offset			0x10
#define R498_OOB_DUTY_offset			0x00
#define R498_OOB_DELAY_offset			0x08



#define F498_STANDBY			 F498_STANDBY_addr, F498_STANDBY_size, F498_STANDBY_offset
#define F498_SOFT_RST			 F498_SOFT_RST_addr, F498_SOFT_RST_size, F498_SOFT_RST_offset
#define F498_EQU_RST			 F498_EQU_RST_addr, F498_EQU_RST_size, F498_EQU_RST_offset
#define F498_CRL_RST			 F498_CRL_RST_addr, F498_CRL_RST_size, F498_CRL_RST_offset
#define F498_TRL_RST			 F498_TRL_RST_addr, F498_TRL_RST_size, F498_TRL_RST_offset
#define F498_AGC_RST			 F498_AGC_RST_addr, F498_AGC_RST_size, F498_AGC_RST_offset
#define F498_FEC_NHW_RESET			 F498_FEC_NHW_RESET_addr, F498_FEC_NHW_RESET_size, F498_FEC_NHW_RESET_offset
#define F498_DEINT_RST			 F498_DEINT_RST_addr, F498_DEINT_RST_size, F498_DEINT_RST_offset
#define F498_FECAC_RS_RST			 F498_FECAC_RS_RST_addr, F498_FECAC_RS_RST_size, F498_FECAC_RS_RST_offset
#define F498_FECB_RS_RST			 F498_FECB_RS_RST_addr, F498_FECB_RS_RST_size, F498_FECB_RS_RST_offset
#define F498_VITERBI_RST			 F498_VITERBI_RST_addr, F498_VITERBI_RST_size, F498_VITERBI_RST_offset
#define F498_SWEEP_OUT			 F498_SWEEP_OUT_addr, F498_SWEEP_OUT_size, F498_SWEEP_OUT_offset
#define F498_FSM_CRL			 F498_FSM_CRL_addr, F498_FSM_CRL_size, F498_FSM_CRL_offset
#define F498_CRL_LOCK			 F498_CRL_LOCK_addr, F498_CRL_LOCK_size, F498_CRL_LOCK_offset
#define F498_MFSM			 	 F498_MFSM_addr, F498_MFSM_size, F498_MFSM_offset
#define F498_TRL_LOCK			 F498_TRL_LOCK_addr, F498_TRL_LOCK_size, F498_TRL_LOCK_offset
#define F498_TRL_AGC_LIMIT			 F498_TRL_AGC_LIMIT_addr, F498_TRL_AGC_LIMIT_size, F498_TRL_AGC_LIMIT_offset
#define F498_ADJ_AGC_LOCK			 F498_ADJ_AGC_LOCK_addr, F498_ADJ_AGC_LOCK_size, F498_ADJ_AGC_LOCK_offset
#define F498_AGC_LOCK			 F498_AGC_LOCK_addr, F498_AGC_LOCK_size, F498_AGC_LOCK_offset
#define F498_TSMF_CNT			 F498_TSMF_CNT_addr, F498_TSMF_CNT_size, F498_TSMF_CNT_offset
#define F498_TSMF_EOF			 F498_TSMF_EOF_addr, F498_TSMF_EOF_size, F498_TSMF_EOF_offset
#define F498_TSMF_RDY			 F498_TSMF_RDY_addr, F498_TSMF_RDY_size, F498_TSMF_RDY_offset
#define F498_FEC_NOCORR			 F498_FEC_NOCORR_addr, F498_FEC_NOCORR_size, F498_FEC_NOCORR_offset
#define F498_FEC_LOCK			 F498_FEC_LOCK_addr, F498_FEC_LOCK_size, F498_FEC_LOCK_offset
#define F498_DEINT_LOCK			 F498_DEINT_LOCK_addr, F498_DEINT_LOCK_size, F498_DEINT_LOCK_offset
#define F498_TAPMON_ALARM			 F498_TAPMON_ALARM_addr, F498_TAPMON_ALARM_size, F498_TAPMON_ALARM_offset
#define F498_SWEEP_OUTE			 F498_SWEEP_OUTE_addr, F498_SWEEP_OUTE_size, F498_SWEEP_OUTE_offset
#define F498_FSM_CRLE			 F498_FSM_CRLE_addr, F498_FSM_CRLE_size, F498_FSM_CRLE_offset
#define F498_CRL_LOCKE			 F498_CRL_LOCKE_addr, F498_CRL_LOCKE_size, F498_CRL_LOCKE_offset
#define F498_MFSME			 	 F498_MFSME_addr, F498_MFSME_size, F498_MFSME_offset
#define F498_TRL_LOCKE			 F498_TRL_LOCKE_addr, F498_TRL_LOCKE_size, F498_TRL_LOCKE_offset
#define F498_TRL_AGC_LIMITE		 F498_TRL_AGC_LIMITE_addr, F498_TRL_AGC_LIMITE_size, F498_TRL_AGC_LIMITE_offset
#define F498_ADJ_AGC_LOCKE			 F498_ADJ_AGC_LOCKE_addr, F498_ADJ_AGC_LOCKE_size, F498_ADJ_AGC_LOCKE_offset
#define F498_AGC_LOCKE			 F498_AGC_LOCKE_addr, F498_AGC_LOCKE_size, F498_AGC_LOCKE_offset
#define F498_TSMF_CNTE			 F498_TSMF_CNTE_addr, F498_TSMF_CNTE_size, F498_TSMF_CNTE_offset
#define F498_TSMF_EOFE			 F498_TSMF_EOFE_addr, F498_TSMF_EOFE_size, F498_TSMF_EOFE_offset
#define F498_TSMF_RDYE			 F498_TSMF_RDYE_addr, F498_TSMF_RDYE_size, F498_TSMF_RDYE_offset
#define F498_FEC_NOCORRE			 F498_FEC_NOCORRE_addr, F498_FEC_NOCORRE_size, F498_FEC_NOCORRE_offset
#define F498_FEC_LOCKE			 F498_FEC_LOCKE_addr, F498_FEC_LOCKE_size, F498_FEC_LOCKE_offset
#define F498_DEINT_LOCKE			 F498_DEINT_LOCKE_addr, F498_DEINT_LOCKE_size, F498_DEINT_LOCKE_offset
#define F498_TAPMON_ALARME			 F498_TAPMON_ALARME_addr, F498_TAPMON_ALARME_size, F498_TAPMON_ALARME_offset
#define F498_QAMFEC_LOCK			 F498_QAMFEC_LOCK_addr, F498_QAMFEC_LOCK_size, F498_QAMFEC_LOCK_offset
#define F498_TSMF_LOCK			 F498_TSMF_LOCK_addr, F498_TSMF_LOCK_size, F498_TSMF_LOCK_offset
#define F498_TSMF_ERROR			 F498_TSMF_ERROR_addr, F498_TSMF_ERROR_size, F498_TSMF_ERROR_offset
#define F498_TST_BUS_SEL			 F498_TST_BUS_SEL_addr, F498_TST_BUS_SEL_size, F498_TST_BUS_SEL_offset
#define F498_AGC_LCK_TH			 F498_AGC_LCK_TH_addr, F498_AGC_LCK_TH_size, F498_AGC_LCK_TH_offset
#define F498_AGC_ACCUMRSTSEL		 F498_AGC_ACCUMRSTSEL_addr, F498_AGC_ACCUMRSTSEL_size, F498_AGC_ACCUMRSTSEL_offset
#define F498_AGC_IF_BWSEL			 F498_AGC_IF_BWSEL_addr, F498_AGC_IF_BWSEL_size, F498_AGC_IF_BWSEL_offset
#define F498_AGC_IF_FREEZE			 F498_AGC_IF_FREEZE_addr, F498_AGC_IF_FREEZE_size, F498_AGC_IF_FREEZE_offset
#define F498_AGC_RF_BWSEL			 F498_AGC_RF_BWSEL_addr, F498_AGC_RF_BWSEL_size, F498_AGC_RF_BWSEL_offset
#define F498_AGC_RF_FREEZE			 F498_AGC_RF_FREEZE_addr, F498_AGC_RF_FREEZE_size, F498_AGC_RF_FREEZE_offset
#define F498_AGC_RF_PWM_TST			 F498_AGC_RF_PWM_TST_addr, F498_AGC_RF_PWM_TST_size, F498_AGC_RF_PWM_TST_offset
#define F498_AGC_RF_PWM_INV			 F498_AGC_RF_PWM_INV_addr, F498_AGC_RF_PWM_INV_size, F498_AGC_RF_PWM_INV_offset
#define F498_AGC_IF_PWM_TST			 F498_AGC_IF_PWM_TST_addr, F498_AGC_IF_PWM_TST_size, F498_AGC_IF_PWM_TST_offset
#define F498_AGC_IF_PWM_INV			 F498_AGC_IF_PWM_INV_addr, F498_AGC_IF_PWM_INV_size, F498_AGC_IF_PWM_INV_offset
#define F498_AGC_PWM_CLKDIV			 F498_AGC_PWM_CLKDIV_addr, F498_AGC_PWM_CLKDIV_size, F498_AGC_PWM_CLKDIV_offset
#define F498_AGC_PWRREF_LO			 F498_AGC_PWRREF_LO_addr, F498_AGC_PWRREF_LO_size, F498_AGC_PWRREF_LO_offset
#define F498_AGC_PWRREF_HI			 F498_AGC_PWRREF_HI_addr, F498_AGC_PWRREF_HI_size, F498_AGC_PWRREF_HI_offset
#define F498_AGC_RF_TH_LO			 F498_AGC_RF_TH_LO_addr, F498_AGC_RF_TH_LO_size, F498_AGC_RF_TH_LO_offset
#define F498_AGC_RF_TH_HI			 F498_AGC_RF_TH_HI_addr, F498_AGC_RF_TH_HI_size, F498_AGC_RF_TH_HI_offset
#define F498_AGC_IF_THLO_LO			 F498_AGC_IF_THLO_LO_addr, F498_AGC_IF_THLO_LO_size, F498_AGC_IF_THLO_LO_offset
#define F498_AGC_IF_THLO_HI			 F498_AGC_IF_THLO_HI_addr, F498_AGC_IF_THLO_HI_size, F498_AGC_IF_THLO_HI_offset
#define F498_AGC_IF_THHI_LO			 F498_AGC_IF_THHI_LO_addr, F498_AGC_IF_THHI_LO_size, F498_AGC_IF_THHI_LO_offset
#define F498_AGC_IF_THHI_HI			 F498_AGC_IF_THHI_HI_addr, F498_AGC_IF_THHI_HI_size, F498_AGC_IF_THHI_HI_offset
#define F498_AGC_PWR_WORD_LO			 F498_AGC_PWR_WORD_LO_addr, F498_AGC_PWR_WORD_LO_size, F498_AGC_PWR_WORD_LO_offset
#define F498_AGC_PWR_WORD_ME			 F498_AGC_PWR_WORD_ME_addr, F498_AGC_PWR_WORD_ME_size, F498_AGC_PWR_WORD_ME_offset
#define F498_AGC_PWR_WORD_HI			 F498_AGC_PWR_WORD_HI_addr, F498_AGC_PWR_WORD_HI_size, F498_AGC_PWR_WORD_HI_offset
#define F498_AGC_IF_PWMCMD_LO			 F498_AGC_IF_PWMCMD_LO_addr, F498_AGC_IF_PWMCMD_LO_size, F498_AGC_IF_PWMCMD_LO_offset
#define F498_AGC_IF_PWMCMD_HI			 F498_AGC_IF_PWMCMD_HI_addr, F498_AGC_IF_PWMCMD_HI_size, F498_AGC_IF_PWMCMD_HI_offset
#define F498_AGC_RF_PWMCMD_LO			 F498_AGC_RF_PWMCMD_LO_addr, F498_AGC_RF_PWMCMD_LO_size, F498_AGC_RF_PWMCMD_LO_offset
#define F498_AGC_RF_PWMCMD_HI			 F498_AGC_RF_PWMCMD_HI_addr, F498_AGC_RF_PWMCMD_HI_size, F498_AGC_RF_PWMCMD_HI_offset
#define F498_IQDEM_CLK_SEL			 F498_IQDEM_CLK_SEL_addr, F498_IQDEM_CLK_SEL_size, F498_IQDEM_CLK_SEL_offset
#define F498_IQDEM_INVIQ			 F498_IQDEM_INVIQ_addr, F498_IQDEM_INVIQ_size, F498_IQDEM_INVIQ_offset
#define F498_IQDEM_A2DTYPE			 F498_IQDEM_A2DTYPE_addr, F498_IQDEM_A2DTYPE_size, F498_IQDEM_A2DTYPE_offset
#define F498_MIX_NCO_INC_LL			 F498_MIX_NCO_INC_LL_addr, F498_MIX_NCO_INC_LL_size, F498_MIX_NCO_INC_LL_offset
#define F498_MIX_NCO_INC_HL			 F498_MIX_NCO_INC_HL_addr, F498_MIX_NCO_INC_HL_size, F498_MIX_NCO_INC_HL_offset
#define F498_MIX_NCO_INVCNST			 F498_MIX_NCO_INVCNST_addr, F498_MIX_NCO_INVCNST_size, F498_MIX_NCO_INVCNST_offset
#define F498_MIX_NCO_INC_HH			 F498_MIX_NCO_INC_HH_addr, F498_MIX_NCO_INC_HH_size, F498_MIX_NCO_INC_HH_offset
#define F498_SRC_NCO_INC_LL			 F498_SRC_NCO_INC_LL_addr, F498_SRC_NCO_INC_LL_size, F498_SRC_NCO_INC_LL_offset
#define F498_SRC_NCO_INC_LH			 F498_SRC_NCO_INC_LH_addr, F498_SRC_NCO_INC_LH_size, F498_SRC_NCO_INC_LH_offset
#define F498_SRC_NCO_INC_HL			 F498_SRC_NCO_INC_HL_addr, F498_SRC_NCO_INC_HL_size, F498_SRC_NCO_INC_HL_offset
#define F498_SRC_NCO_INC_HH			 F498_SRC_NCO_INC_HH_addr, F498_SRC_NCO_INC_HH_size, F498_SRC_NCO_INC_HH_offset
#define F498_GAIN_SRC_LO			 F498_GAIN_SRC_LO_addr, F498_GAIN_SRC_LO_size, F498_GAIN_SRC_LO_offset
#define F498_GAIN_SRC_HI			 F498_GAIN_SRC_HI_addr, F498_GAIN_SRC_HI_size, F498_GAIN_SRC_HI_offset
#define F498_DCRM0_DCIN_L			 F498_DCRM0_DCIN_L_addr, F498_DCRM0_DCIN_L_size, F498_DCRM0_DCIN_L_offset
#define F498_DCRM1_I_DCIN_L			 F498_DCRM1_I_DCIN_L_addr, F498_DCRM1_I_DCIN_L_size, F498_DCRM1_I_DCIN_L_offset
#define F498_DCRM0_DCIN_H			 F498_DCRM0_DCIN_H_addr, F498_DCRM0_DCIN_H_size, F498_DCRM0_DCIN_H_offset
#define F498_DCRM1_Q_DCIN_L			 F498_DCRM1_Q_DCIN_L_addr, F498_DCRM1_Q_DCIN_L_size, F498_DCRM1_Q_DCIN_L_offset
#define F498_DCRM1_I_DCIN_H			 F498_DCRM1_I_DCIN_H_addr, F498_DCRM1_I_DCIN_H_size, F498_DCRM1_I_DCIN_H_offset
#define F498_DCRM1_FRZ			 F498_DCRM1_FRZ_addr, F498_DCRM1_FRZ_size, F498_DCRM1_FRZ_offset
#define F498_DCRM0_FRZ			 F498_DCRM0_FRZ_addr, F498_DCRM0_FRZ_size, F498_DCRM0_FRZ_offset
#define F498_DCRM1_Q_DCIN_H			 F498_DCRM1_Q_DCIN_H_addr, F498_DCRM1_Q_DCIN_H_size, F498_DCRM1_Q_DCIN_H_offset
#define F498_ADJIIR_COEFF10_L			 F498_ADJIIR_COEFF10_L_addr, F498_ADJIIR_COEFF10_L_size, F498_ADJIIR_COEFF10_L_offset
#define F498_ADJIIR_COEFF11_L			 F498_ADJIIR_COEFF11_L_addr, F498_ADJIIR_COEFF11_L_size, F498_ADJIIR_COEFF11_L_offset
#define F498_ADJIIR_COEFF10_H			 F498_ADJIIR_COEFF10_H_addr, F498_ADJIIR_COEFF10_H_size, F498_ADJIIR_COEFF10_H_offset
#define F498_ADJIIR_COEFF12_L			 F498_ADJIIR_COEFF12_L_addr, F498_ADJIIR_COEFF12_L_size, F498_ADJIIR_COEFF12_L_offset
#define F498_ADJIIR_COEFF11_H			 F498_ADJIIR_COEFF11_H_addr, F498_ADJIIR_COEFF11_H_size, F498_ADJIIR_COEFF11_H_offset
#define F498_ADJIIR_COEFF20_L			 F498_ADJIIR_COEFF20_L_addr, F498_ADJIIR_COEFF20_L_size, F498_ADJIIR_COEFF20_L_offset
#define F498_ADJIIR_COEFF12_H			 F498_ADJIIR_COEFF12_H_addr, F498_ADJIIR_COEFF12_H_size, F498_ADJIIR_COEFF12_H_offset
#define F498_ADJIIR_COEFF20_H			 F498_ADJIIR_COEFF20_H_addr, F498_ADJIIR_COEFF20_H_size, F498_ADJIIR_COEFF20_H_offset
#define F498_ADJIIR_COEFF21_L			 F498_ADJIIR_COEFF21_L_addr, F498_ADJIIR_COEFF21_L_size, F498_ADJIIR_COEFF21_L_offset
#define F498_ADJIIR_COEFF22_L			 F498_ADJIIR_COEFF22_L_addr, F498_ADJIIR_COEFF22_L_size, F498_ADJIIR_COEFF22_L_offset
#define F498_ADJIIR_COEFF21_H			 F498_ADJIIR_COEFF21_H_addr, F498_ADJIIR_COEFF21_H_size, F498_ADJIIR_COEFF21_H_offset
#define F498_ADJIIR_COEFF22_H			 F498_ADJIIR_COEFF22_H_addr, F498_ADJIIR_COEFF22_H_size, F498_ADJIIR_COEFF22_H_offset
#define F498_ALLPASSFILT_EN			 F498_ALLPASSFILT_EN_addr, F498_ALLPASSFILT_EN_size, F498_ALLPASSFILT_EN_offset
#define F498_ADJ_AGC_EN			 F498_ADJ_AGC_EN_addr, F498_ADJ_AGC_EN_size, F498_ADJ_AGC_EN_offset
#define F498_ADJ_COEFF_FRZ			 F498_ADJ_COEFF_FRZ_addr, F498_ADJ_COEFF_FRZ_size, F498_ADJ_COEFF_FRZ_offset
#define F498_ADJ_EN			 F498_ADJ_EN_addr, F498_ADJ_EN_size, F498_ADJ_EN_offset
#define F498_ADJ_AGC_REF			 F498_ADJ_AGC_REF_addr, F498_ADJ_AGC_REF_size, F498_ADJ_AGC_REF_offset
#define F498_ALLPASSFILT_COEFF1_LO			 F498_ALLPASSFILT_COEFF1_LO_addr, F498_ALLPASSFILT_COEFF1_LO_size, F498_ALLPASSFILT_COEFF1_LO_offset
#define F498_ALLPASSFILT_COEFF1_ME			 F498_ALLPASSFILT_COEFF1_ME_addr, F498_ALLPASSFILT_COEFF1_ME_size, F498_ALLPASSFILT_COEFF1_ME_offset
#define F498_ALLPASSFILT_COEFF2_LO			 F498_ALLPASSFILT_COEFF2_LO_addr, F498_ALLPASSFILT_COEFF2_LO_size, F498_ALLPASSFILT_COEFF2_LO_offset
#define F498_ALLPASSFILT_COEFF1_HI			 F498_ALLPASSFILT_COEFF1_HI_addr, F498_ALLPASSFILT_COEFF1_HI_size, F498_ALLPASSFILT_COEFF1_HI_offset
#define F498_ALLPASSFILT_COEFF2_ME			 F498_ALLPASSFILT_COEFF2_ME_addr, F498_ALLPASSFILT_COEFF2_ME_size, F498_ALLPASSFILT_COEFF2_ME_offset
#define F498_ALLPASSFILT_COEFF2_HI			 F498_ALLPASSFILT_COEFF2_HI_addr, F498_ALLPASSFILT_COEFF2_HI_size, F498_ALLPASSFILT_COEFF2_HI_offset
#define F498_ALLPASSFILT_COEFF3_LO			 F498_ALLPASSFILT_COEFF3_LO_addr, F498_ALLPASSFILT_COEFF3_LO_size, F498_ALLPASSFILT_COEFF3_LO_offset
#define F498_ALLPASSFILT_COEFF2_HH			 F498_ALLPASSFILT_COEFF2_HH_addr, F498_ALLPASSFILT_COEFF2_HH_size, F498_ALLPASSFILT_COEFF2_HH_offset
#define F498_ALLPASSFILT_COEFF3_ME			 F498_ALLPASSFILT_COEFF3_ME_addr, F498_ALLPASSFILT_COEFF3_ME_size, F498_ALLPASSFILT_COEFF3_ME_offset
#define F498_ALLPASSFILT_COEFF3_HI			 F498_ALLPASSFILT_COEFF3_HI_addr, F498_ALLPASSFILT_COEFF3_HI_size, F498_ALLPASSFILT_COEFF3_HI_offset
#define F498_ALLPASSFILT_COEFF4_LO			 F498_ALLPASSFILT_COEFF4_LO_addr, F498_ALLPASSFILT_COEFF4_LO_size, F498_ALLPASSFILT_COEFF4_LO_offset
#define F498_ALLPASSFILT_COEFF3_HH			 F498_ALLPASSFILT_COEFF3_HH_addr, F498_ALLPASSFILT_COEFF3_HH_size, F498_ALLPASSFILT_COEFF3_HH_offset
#define F498_ALLPASSFILT_COEFF4_ME			 F498_ALLPASSFILT_COEFF4_ME_addr, F498_ALLPASSFILT_COEFF4_ME_size, F498_ALLPASSFILT_COEFF4_ME_offset
#define F498_ALLPASSFILT_COEFF4_HI			 F498_ALLPASSFILT_COEFF4_HI_addr, F498_ALLPASSFILT_COEFF4_HI_size, F498_ALLPASSFILT_COEFF4_HI_offset
#define F498_TRL_AGC_FREEZE			 F498_TRL_AGC_FREEZE_addr, F498_TRL_AGC_FREEZE_size, F498_TRL_AGC_FREEZE_offset
#define F498_TRL_AGC_REF			 F498_TRL_AGC_REF_addr, F498_TRL_AGC_REF_size, F498_TRL_AGC_REF_offset
#define F498_NYQPOINT_INV			 F498_NYQPOINT_INV_addr, F498_NYQPOINT_INV_size, F498_NYQPOINT_INV_offset
#define F498_TRL_SHIFT			 F498_TRL_SHIFT_addr, F498_TRL_SHIFT_size, F498_TRL_SHIFT_offset
#define F498_NYQ_COEFF_SEL			 F498_NYQ_COEFF_SEL_addr, F498_NYQ_COEFF_SEL_size, F498_NYQ_COEFF_SEL_offset
#define F498_TRL_LPF_FREEZE			 F498_TRL_LPF_FREEZE_addr, F498_TRL_LPF_FREEZE_size, F498_TRL_LPF_FREEZE_offset
#define F498_TRL_LPF_CRT			 F498_TRL_LPF_CRT_addr, F498_TRL_LPF_CRT_size, F498_TRL_LPF_CRT_offset
#define F498_TRL_GDIR_ACQ			 F498_TRL_GDIR_ACQ_addr, F498_TRL_GDIR_ACQ_size, F498_TRL_GDIR_ACQ_offset
#define F498_TRL_GINT_ACQ			 F498_TRL_GINT_ACQ_addr, F498_TRL_GINT_ACQ_size, F498_TRL_GINT_ACQ_offset
#define F498_TRL_GDIR_TRK			 F498_TRL_GDIR_TRK_addr, F498_TRL_GDIR_TRK_size, F498_TRL_GDIR_TRK_offset
#define F498_TRL_GINT_TRK			 F498_TRL_GINT_TRK_addr, F498_TRL_GINT_TRK_size, F498_TRL_GINT_TRK_offset
#define F498_TRL_GAIN_OUT			 F498_TRL_GAIN_OUT_addr, F498_TRL_GAIN_OUT_size, F498_TRL_GAIN_OUT_offset
#define F498_TRL_LCK_THLO			 F498_TRL_LCK_THLO_addr, F498_TRL_LCK_THLO_size, F498_TRL_LCK_THLO_offset
#define F498_TRL_LCK_THHI			 F498_TRL_LCK_THHI_addr, F498_TRL_LCK_THHI_size, F498_TRL_LCK_THHI_offset
#define F498_TRL_LCK_TRG			 F498_TRL_LCK_TRG_addr, F498_TRL_LCK_TRG_size, F498_TRL_LCK_TRG_offset
#define F498_CRL_DFE			 F498_CRL_DFE_addr, F498_CRL_DFE_size, F498_CRL_DFE_offset
#define F498_DFE_START			 F498_DFE_START_addr, F498_DFE_START_size, F498_DFE_START_offset
#define F498_CTRLG_START			 F498_CTRLG_START_addr, F498_CTRLG_START_size, F498_CTRLG_START_offset
#define F498_FSM_FORCESTATE			 F498_FSM_FORCESTATE_addr, F498_FSM_FORCESTATE_size, F498_FSM_FORCESTATE_offset
#define F498_FEC2_EN			 F498_FEC2_EN_addr, F498_FEC2_EN_size, F498_FEC2_EN_offset
#define F498_SIT_EN			 F498_SIT_EN_addr, F498_SIT_EN_size, F498_SIT_EN_offset
#define F498_TRL_AHEAD			 F498_TRL_AHEAD_addr, F498_TRL_AHEAD_size, F498_TRL_AHEAD_offset
#define F498_TRL2_EN			 F498_TRL2_EN_addr, F498_TRL2_EN_size, F498_TRL2_EN_offset
#define F498_FSM_RCA_EN			 F498_FSM_RCA_EN_addr, F498_FSM_RCA_EN_size, F498_FSM_RCA_EN_offset
#define F498_FSM_BKP_DIS			 F498_FSM_BKP_DIS_addr, F498_FSM_BKP_DIS_size, F498_FSM_BKP_DIS_offset
#define F498_FSM_FORCE_EN			 F498_FSM_FORCE_EN_addr, F498_FSM_FORCE_EN_size, F498_FSM_FORCE_EN_offset
#define F498_FSM_STATUS			 F498_FSM_STATUS_addr, F498_FSM_STATUS_size, F498_FSM_STATUS_offset
#define F498_SNR0_HTH			 F498_SNR0_HTH_addr, F498_SNR0_HTH_size, F498_SNR0_HTH_offset
#define F498_SNR1_HTH			 F498_SNR1_HTH_addr, F498_SNR1_HTH_size, F498_SNR1_HTH_offset
#define F498_SNR2_HTH			 F498_SNR2_HTH_addr, F498_SNR2_HTH_size, F498_SNR2_HTH_offset
#define F498_SNR0_LTH			 F498_SNR0_LTH_addr, F498_SNR0_LTH_size, F498_SNR0_LTH_offset
#define F498_SNR1_LTH			 F498_SNR1_LTH_addr, F498_SNR1_LTH_size, F498_SNR1_LTH_offset
#define F498_SNR3_HTH_L			 F498_SNR3_HTH_L_addr, F498_SNR3_HTH_L_size, F498_SNR3_HTH_L_offset
#define F498_RCA_HTH			 F498_RCA_HTH_addr, F498_RCA_HTH_size, F498_RCA_HTH_offset
#define F498_SIT			 F498_SIT_addr, F498_SIT_size, F498_SIT_offset
#define F498_WST			 F498_WST_addr, F498_WST_size, F498_WST_offset
#define F498_ELT			 F498_ELT_addr, F498_ELT_size, F498_ELT_offset
#define F498_SNR3_HTH_H			 F498_SNR3_HTH_H_addr, F498_SNR3_HTH_H_size, F498_SNR3_HTH_H_offset
#define F498_FEC2_DFEOFF			 F498_FEC2_DFEOFF_addr, F498_FEC2_DFEOFF_size, F498_FEC2_DFEOFF_offset
#define F498_PNT_STATE			 F498_PNT_STATE_addr, F498_PNT_STATE_size, F498_PNT_STATE_offset
#define F498_MODMAP_STATE			 F498_MODMAP_STATE_addr, F498_MODMAP_STATE_size, F498_MODMAP_STATE_offset
#define F498_I_TEST_TAP_L			 F498_I_TEST_TAP_L_addr, F498_I_TEST_TAP_L_size, F498_I_TEST_TAP_L_offset
#define F498_I_TEST_TAP_M			 F498_I_TEST_TAP_M_addr, F498_I_TEST_TAP_M_size, F498_I_TEST_TAP_M_offset
#define F498_I_TEST_TAP_H			 F498_I_TEST_TAP_H_addr, F498_I_TEST_TAP_H_size, F498_I_TEST_TAP_H_offset
#define F498_TEST_FFE_DFE_SEL			 F498_TEST_FFE_DFE_SEL_addr, F498_TEST_FFE_DFE_SEL_size, F498_TEST_FFE_DFE_SEL_offset
#define F498_TEST_TAP_SELECT			 F498_TEST_TAP_SELECT_addr, F498_TEST_TAP_SELECT_size, F498_TEST_TAP_SELECT_offset
#define F498_Q_TEST_TAP_L			 F498_Q_TEST_TAP_L_addr, F498_Q_TEST_TAP_L_size, F498_Q_TEST_TAP_L_offset
#define F498_Q_TEST_TAP_M			 F498_Q_TEST_TAP_M_addr, F498_Q_TEST_TAP_M_size, F498_Q_TEST_TAP_M_offset
#define F498_Q_TEST_TAP_H			 F498_Q_TEST_TAP_H_addr, F498_Q_TEST_TAP_H_size, F498_Q_TEST_TAP_H_offset
#define F498_MTAP_FRZ			 F498_MTAP_FRZ_addr, F498_MTAP_FRZ_size, F498_MTAP_FRZ_offset
#define F498_PRE_FREEZE			 F498_PRE_FREEZE_addr, F498_PRE_FREEZE_size, F498_PRE_FREEZE_offset
#define F498_DFE_TAPMON_EN			 F498_DFE_TAPMON_EN_addr, F498_DFE_TAPMON_EN_size, F498_DFE_TAPMON_EN_offset
#define F498_FFE_TAPMON_EN			 F498_FFE_TAPMON_EN_addr, F498_FFE_TAPMON_EN_size, F498_FFE_TAPMON_EN_offset
#define F498_MTAP_ONLY			 F498_MTAP_ONLY_addr, F498_MTAP_ONLY_size, F498_MTAP_ONLY_offset
#define F498_EQU_CTR_CRL_CONTROL_LO			 F498_EQU_CTR_CRL_CONTROL_LO_addr, F498_EQU_CTR_CRL_CONTROL_LO_size, F498_EQU_CTR_CRL_CONTROL_LO_offset
#define F498_EQU_CTR_CRL_CONTROL_HI			 F498_EQU_CTR_CRL_CONTROL_HI_addr, F498_EQU_CTR_CRL_CONTROL_HI_size, F498_EQU_CTR_CRL_CONTROL_HI_offset
#define F498_CTR_HIPOW_L			 F498_CTR_HIPOW_L_addr, F498_CTR_HIPOW_L_size, F498_CTR_HIPOW_L_offset
#define F498_CTR_HIPOW_H			 F498_CTR_HIPOW_H_addr, F498_CTR_HIPOW_H_size, F498_CTR_HIPOW_H_offset
#define F498_EQU_I_EQU_L			 F498_EQU_I_EQU_L_addr, F498_EQU_I_EQU_L_size, F498_EQU_I_EQU_L_offset
#define F498_EQU_I_EQU_H			 F498_EQU_I_EQU_H_addr, F498_EQU_I_EQU_H_size, F498_EQU_I_EQU_H_offset
#define F498_EQU_Q_EQU_L			 F498_EQU_Q_EQU_L_addr, F498_EQU_Q_EQU_L_size, F498_EQU_Q_EQU_L_offset
#define F498_EQU_Q_EQU_H			 F498_EQU_Q_EQU_H_addr, F498_EQU_Q_EQU_H_size, F498_EQU_Q_EQU_H_offset
#define F498_QUAD_AUTO			 F498_QUAD_AUTO_addr, F498_QUAD_AUTO_size, F498_QUAD_AUTO_offset
#define F498_QUAD_INV			 F498_QUAD_INV_addr, F498_QUAD_INV_size, F498_QUAD_INV_offset
#define F498_QAM_MODE			 F498_QAM_MODE_addr, F498_QAM_MODE_size, F498_QAM_MODE_offset
#define F498_SNR_PER			 F498_SNR_PER_addr, F498_SNR_PER_size, F498_SNR_PER_offset
#define F498_SWEEP_RATE			 F498_SWEEP_RATE_addr, F498_SWEEP_RATE_size, F498_SWEEP_RATE_offset
#define F498_SNR_LO			 F498_SNR_LO_addr, F498_SNR_LO_size, F498_SNR_LO_offset
#define F498_SNR_HI			 F498_SNR_HI_addr, F498_SNR_HI_size, F498_SNR_HI_offset
#define F498_GAMMA_LO			 F498_GAMMA_LO_addr, F498_GAMMA_LO_size, F498_GAMMA_LO_offset
#define F498_GAMMA_ME			 F498_GAMMA_ME_addr, F498_GAMMA_ME_size, F498_GAMMA_ME_offset
#define F498_RCAMU			 F498_RCAMU_addr, F498_RCAMU_size, F498_RCAMU_offset
#define F498_CMAMU			 F498_CMAMU_addr, F498_CMAMU_size, F498_CMAMU_offset
#define F498_GAMMA_HI			 F498_GAMMA_HI_addr, F498_GAMMA_HI_size, F498_GAMMA_HI_offset
#define F498_RADIUS			 F498_RADIUS_addr, F498_RADIUS_size, F498_RADIUS_offset
#define F498_FFE_MAINTAP_INIT			 F498_FFE_MAINTAP_INIT_addr, F498_FFE_MAINTAP_INIT_size, F498_FFE_MAINTAP_INIT_offset
#define F498_LEAK_PER			 F498_LEAK_PER_addr, F498_LEAK_PER_size, F498_LEAK_PER_offset
#define F498_EQU_OUTSEL			 F498_EQU_OUTSEL_addr, F498_EQU_OUTSEL_size, F498_EQU_OUTSEL_offset
#define F498_PNT2DFE			 F498_PNT2DFE_addr, F498_PNT2DFE_size, F498_PNT2DFE_offset
#define F498_FFE_LEAK_EN			 F498_FFE_LEAK_EN_addr, F498_FFE_LEAK_EN_size, F498_FFE_LEAK_EN_offset
#define F498_DFE_LEAK_EN			 F498_DFE_LEAK_EN_addr, F498_DFE_LEAK_EN_size, F498_DFE_LEAK_EN_offset
#define F498_FFE_MAINTAP_POS			 F498_FFE_MAINTAP_POS_addr, F498_FFE_MAINTAP_POS_size, F498_FFE_MAINTAP_POS_offset
#define F498_DFE_GAIN_WIDE			 F498_DFE_GAIN_WIDE_addr, F498_DFE_GAIN_WIDE_size, F498_DFE_GAIN_WIDE_offset
#define F498_FFE_GAIN_WIDE			 F498_FFE_GAIN_WIDE_addr, F498_FFE_GAIN_WIDE_size, F498_FFE_GAIN_WIDE_offset
#define F498_DFE_GAIN_NARROW			 F498_DFE_GAIN_NARROW_addr, F498_DFE_GAIN_NARROW_size, F498_DFE_GAIN_NARROW_offset
#define F498_FFE_GAIN_NARROW			 F498_FFE_GAIN_NARROW_addr, F498_FFE_GAIN_NARROW_size, F498_FFE_GAIN_NARROW_offset
#define F498_CTR_GTO			 F498_CTR_GTO_addr, F498_CTR_GTO_size, F498_CTR_GTO_offset
#define F498_CTR_GDIR			 F498_CTR_GDIR_addr, F498_CTR_GDIR_size, F498_CTR_GDIR_offset
#define F498_SWEEP_EN			 F498_SWEEP_EN_addr, F498_SWEEP_EN_size, F498_SWEEP_EN_offset
#define F498_CTR_GINT			 F498_CTR_GINT_addr, F498_CTR_GINT_size, F498_CTR_GINT_offset
#define F498_CRL_GTO			 F498_CRL_GTO_addr, F498_CRL_GTO_size, F498_CRL_GTO_offset
#define F498_CRL_GDIR			 F498_CRL_GDIR_addr, F498_CRL_GDIR_size, F498_CRL_GDIR_offset
#define F498_SWEEP_DIR			 F498_SWEEP_DIR_addr, F498_SWEEP_DIR_size, F498_SWEEP_DIR_offset
#define F498_CRL_GINT			 F498_CRL_GINT_addr, F498_CRL_GINT_size, F498_CRL_GINT_offset
#define F498_CRL_GAIN			 F498_CRL_GAIN_addr, F498_CRL_GAIN_size, F498_CRL_GAIN_offset
#define F498_CTR_INC_GAIN			 F498_CTR_INC_GAIN_addr, F498_CTR_INC_GAIN_size, F498_CTR_INC_GAIN_offset
#define F498_CTR_FRAC			 F498_CTR_FRAC_addr, F498_CTR_FRAC_size, F498_CTR_FRAC_offset
#define F498_CTR_BADPOINT_EN			 F498_CTR_BADPOINT_EN_addr, F498_CTR_BADPOINT_EN_size, F498_CTR_BADPOINT_EN_offset
#define F498_CTR_GAIN			 F498_CTR_GAIN_addr, F498_CTR_GAIN_size, F498_CTR_GAIN_offset
#define F498_LIMANEN			 F498_LIMANEN_addr, F498_LIMANEN_size, F498_LIMANEN_offset
#define F498_CRL_LD_SEN			 F498_CRL_LD_SEN_addr, F498_CRL_LD_SEN_size, F498_CRL_LD_SEN_offset
#define F498_CRL_BISTH_LIMIT			 F498_CRL_BISTH_LIMIT_addr, F498_CRL_BISTH_LIMIT_size, F498_CRL_BISTH_LIMIT_offset
#define F498_CARE_EN			 F498_CARE_EN_addr, F498_CARE_EN_size, F498_CARE_EN_offset
#define F498_CRL_LD_PER			 F498_CRL_LD_PER_addr, F498_CRL_LD_PER_size, F498_CRL_LD_PER_offset
#define F498_CRL_LD_WST			 F498_CRL_LD_WST_addr, F498_CRL_LD_WST_size, F498_CRL_LD_WST_offset
#define F498_CRL_LD_TFS			 F498_CRL_LD_TFS_addr, F498_CRL_LD_TFS_size, F498_CRL_LD_TFS_offset
#define F498_CRL_LD_TFR			 F498_CRL_LD_TFR_addr, F498_CRL_LD_TFR_size, F498_CRL_LD_TFR_offset
#define F498_CRL_BISTH_LO			 F498_CRL_BISTH_LO_addr, F498_CRL_BISTH_LO_size, F498_CRL_BISTH_LO_offset
#define F498_CRL_BISTH_HI			 F498_CRL_BISTH_HI_addr, F498_CRL_BISTH_HI_size, F498_CRL_BISTH_HI_offset
#define F498_SWEEP_RANGE_LO			 F498_SWEEP_RANGE_LO_addr, F498_SWEEP_RANGE_LO_size, F498_SWEEP_RANGE_LO_offset
#define F498_SWEEP_RANGE_HI			 F498_SWEEP_RANGE_HI_addr, F498_SWEEP_RANGE_HI_size, F498_SWEEP_RANGE_HI_offset
#define F498_BISECTOR_EN			 F498_BISECTOR_EN_addr, F498_BISECTOR_EN_size, F498_BISECTOR_EN_offset
#define F498_PHEST128_EN			 F498_PHEST128_EN_addr, F498_PHEST128_EN_size, F498_PHEST128_EN_offset
#define F498_CRL_LIM			 F498_CRL_LIM_addr, F498_CRL_LIM_size, F498_CRL_LIM_offset
#define F498_PNT_DEPTH			 F498_PNT_DEPTH_addr, F498_PNT_DEPTH_size, F498_PNT_DEPTH_offset
#define F498_MODULUS_CMP			 F498_MODULUS_CMP_addr, F498_MODULUS_CMP_size, F498_MODULUS_CMP_offset
#define F498_PNT_EN			 F498_PNT_EN_addr, F498_PNT_EN_size, F498_PNT_EN_offset
#define F498_MODULUSMAP_EN			 F498_MODULUSMAP_EN_addr, F498_MODULUSMAP_EN_size, F498_MODULUSMAP_EN_offset
#define F498_PNT_GAIN			 F498_PNT_GAIN_addr, F498_PNT_GAIN_size, F498_PNT_GAIN_offset
#define F498_RST_RS			 F498_RST_RS_addr, F498_RST_RS_size, F498_RST_RS_offset
#define F498_RST_DI			 F498_RST_DI_addr, F498_RST_DI_size, F498_RST_DI_offset
#define F498_BE_BYPASS			 F498_BE_BYPASS_addr, F498_BE_BYPASS_size, F498_BE_BYPASS_offset
#define F498_REFRESH47			 F498_REFRESH47_addr, F498_REFRESH47_size, F498_REFRESH47_offset
#define F498_CT_NBST			 F498_CT_NBST_addr, F498_CT_NBST_size, F498_CT_NBST_offset
#define F498_TEI_ENA			 F498_TEI_ENA_addr, F498_TEI_ENA_size, F498_TEI_ENA_offset
#define F498_DS_ENA			 F498_DS_ENA_addr, F498_DS_ENA_size, F498_DS_ENA_offset
#define F498_TSMF_EN			 F498_TSMF_EN_addr, F498_TSMF_EN_size, F498_TSMF_EN_offset
#define F498_DEINT_DEPTH			 F498_DEINT_DEPTH_addr, F498_DEINT_DEPTH_size, F498_DEINT_DEPTH_offset
#define F498_DEINT_M			 F498_DEINT_M_addr, F498_DEINT_M_size, F498_DEINT_M_offset
#define F498_DIS_UNLOCK			 F498_DIS_UNLOCK_addr, F498_DIS_UNLOCK_size, F498_DIS_UNLOCK_offset
#define F498_DESCR_MODE			 F498_DESCR_MODE_addr, F498_DESCR_MODE_size, F498_DESCR_MODE_offset
#define F498_DI_UNLOCK			 F498_DI_UNLOCK_addr, F498_DI_UNLOCK_size, F498_DI_UNLOCK_offset
#define F498_DI_FREEZE			 F498_DI_FREEZE_addr, F498_DI_FREEZE_size, F498_DI_FREEZE_offset
#define F498_MISMATCH			 F498_MISMATCH_addr, F498_MISMATCH_size, F498_MISMATCH_offset
#define F498_ACQ_MODE			 F498_ACQ_MODE_addr, F498_ACQ_MODE_size, F498_ACQ_MODE_offset
#define F498_TRK_MODE			 F498_TRK_MODE_addr, F498_TRK_MODE_size, F498_TRK_MODE_offset
#define F498_DEINT_SMCNTR			 F498_DEINT_SMCNTR_addr, F498_DEINT_SMCNTR_size, F498_DEINT_SMCNTR_offset
#define F498_DEINT_SYNCSTATE			 F498_DEINT_SYNCSTATE_addr, F498_DEINT_SYNCSTATE_size, F498_DEINT_SYNCSTATE_offset
#define F498_DEINT_SYNLOST			 F498_DEINT_SYNLOST_addr, F498_DEINT_SYNLOST_size, F498_DEINT_SYNLOST_offset
#define F498_DESCR_SYNCSTATE			 F498_DESCR_SYNCSTATE_addr, F498_DESCR_SYNCSTATE_size, F498_DESCR_SYNCSTATE_offset
#define F498_BK_CT_L			 F498_BK_CT_L_addr, F498_BK_CT_L_size, F498_BK_CT_L_offset
#define F498_BK_CT_H			 F498_BK_CT_H_addr, F498_BK_CT_H_size, F498_BK_CT_H_offset
#define F498_CORR_CT_L			 F498_CORR_CT_L_addr, F498_CORR_CT_L_size, F498_CORR_CT_L_offset
#define F498_CORR_CT_H			 F498_CORR_CT_H_addr, F498_CORR_CT_H_size, F498_CORR_CT_H_offset
#define F498_UNCORR_CT_L			 F498_UNCORR_CT_L_addr, F498_UNCORR_CT_L_size, F498_UNCORR_CT_L_offset
#define F498_UNCORR_CT_H			 F498_UNCORR_CT_H_addr, F498_UNCORR_CT_H_size, F498_UNCORR_CT_H_offset
#define F498_RS_NOCORR			 F498_RS_NOCORR_addr, F498_RS_NOCORR_size, F498_RS_NOCORR_offset
#define F498_CT_HOLD			 F498_CT_HOLD_addr, F498_CT_HOLD_size, F498_CT_HOLD_offset
#define F498_CT_CLEAR			 F498_CT_CLEAR_addr, F498_CT_CLEAR_size, F498_CT_CLEAR_offset
#define F498_BERT_ON			 F498_BERT_ON_addr, F498_BERT_ON_size, F498_BERT_ON_offset
#define F498_BERT_ERR_SRC			 F498_BERT_ERR_SRC_addr, F498_BERT_ERR_SRC_size, F498_BERT_ERR_SRC_offset
#define F498_BERT_ERR_MODE			 F498_BERT_ERR_MODE_addr, F498_BERT_ERR_MODE_size, F498_BERT_ERR_MODE_offset
#define F498_BERT_NBYTE			 F498_BERT_NBYTE_addr, F498_BERT_NBYTE_size, F498_BERT_NBYTE_offset
#define F498_BERT_ERRCOUNT_L			 F498_BERT_ERRCOUNT_L_addr, F498_BERT_ERRCOUNT_L_size, F498_BERT_ERRCOUNT_L_offset
#define F498_BERT_ERRCOUNT_H			 F498_BERT_ERRCOUNT_H_addr, F498_BERT_ERRCOUNT_H_size, F498_BERT_ERRCOUNT_H_offset
#define F498_CLK_POLARITY			 F498_CLK_POLARITY_addr, F498_CLK_POLARITY_size, F498_CLK_POLARITY_offset
#define F498_FEC_TYPE			 F498_FEC_TYPE_addr, F498_FEC_TYPE_size, F498_FEC_TYPE_offset
#define F498_SYNC_STRIP			 F498_SYNC_STRIP_addr, F498_SYNC_STRIP_size, F498_SYNC_STRIP_offset
#define F498_TS_SWAP			 F498_TS_SWAP_addr, F498_TS_SWAP_size, F498_TS_SWAP_offset
#define F498_OUTFORMAT			 F498_OUTFORMAT_addr, F498_OUTFORMAT_size, F498_OUTFORMAT_offset
#define F498_CI_DIVRANGE			 F498_CI_DIVRANGE_addr, F498_CI_DIVRANGE_size, F498_CI_DIVRANGE_offset
#define F498_SMOOTHER_CNT_L			 F498_SMOOTHER_CNT_L_addr, F498_SMOOTHER_CNT_L_size, F498_SMOOTHER_CNT_L_offset
#define F498_SMOOTHERDIV_EN_L			 F498_SMOOTHERDIV_EN_L_addr, F498_SMOOTHERDIV_EN_L_size, F498_SMOOTHERDIV_EN_L_offset
#define F498_SMOOTHER_CNT_H			 F498_SMOOTHER_CNT_H_addr, F498_SMOOTHER_CNT_H_size, F498_SMOOTHER_CNT_H_offset
#define F498_SMOOTH_BYPASS			 F498_SMOOTH_BYPASS_addr, F498_SMOOTH_BYPASS_size, F498_SMOOTH_BYPASS_offset
#define F498_SWAP_IQ			 F498_SWAP_IQ_addr, F498_SWAP_IQ_size, F498_SWAP_IQ_offset
#define F498_SMOOTHERDIV_EN_H			 F498_SMOOTHERDIV_EN_H_addr, F498_SMOOTHERDIV_EN_H_size, F498_SMOOTHERDIV_EN_H_offset
#define F498_TS_NUMBER			 F498_TS_NUMBER_addr, F498_TS_NUMBER_size, F498_TS_NUMBER_offset
#define F498_SEL_MODE			 F498_SEL_MODE_addr, F498_SEL_MODE_size, F498_SEL_MODE_offset
#define F498_CHECK_ERROR_BIT			 F498_CHECK_ERROR_BIT_addr, F498_CHECK_ERROR_BIT_size, F498_CHECK_ERROR_BIT_offset
#define F498_CHCK_F_SYNC			 F498_CHCK_F_SYNC_addr, F498_CHCK_F_SYNC_size, F498_CHCK_F_SYNC_offset
#define F498_H_MODE			 F498_H_MODE_addr, F498_H_MODE_size, F498_H_MODE_offset
#define F498_D_V_MODE			 F498_D_V_MODE_addr, F498_D_V_MODE_size, F498_D_V_MODE_offset
#define F498_MODE			 F498_MODE_addr, F498_MODE_size, F498_MODE_offset
#define F498_SYNC_IN_COUNT			 F498_SYNC_IN_COUNT_addr, F498_SYNC_IN_COUNT_size, F498_SYNC_IN_COUNT_offset
#define F498_SYNC_OUT_COUNT			 F498_SYNC_OUT_COUNT_addr, F498_SYNC_OUT_COUNT_size, F498_SYNC_OUT_COUNT_offset
#define F498_TS_ID_L			 F498_TS_ID_L_addr, F498_TS_ID_L_size, F498_TS_ID_L_offset
#define F498_TS_ID_H			 F498_TS_ID_H_addr, F498_TS_ID_H_size, F498_TS_ID_H_offset
#define F498_ON_ID_L			 F498_ON_ID_L_addr, F498_ON_ID_L_size, F498_ON_ID_L_offset
#define F498_ON_ID_H			 F498_ON_ID_H_addr, F498_ON_ID_H_size, F498_ON_ID_H_offset
#define F498_RECEIVE_STATUS_L			 F498_RECEIVE_STATUS_L_addr, F498_RECEIVE_STATUS_L_size, F498_RECEIVE_STATUS_L_offset
#define F498_RECEIVE_STATUS_LH			 F498_RECEIVE_STATUS_LH_addr, F498_RECEIVE_STATUS_LH_size, F498_RECEIVE_STATUS_LH_offset
#define F498_RECEIVE_STATUS_HL			 F498_RECEIVE_STATUS_HL_addr, F498_RECEIVE_STATUS_HL_size, F498_RECEIVE_STATUS_HL_offset
#define F498_RECEIVE_STATUS_HH			 F498_RECEIVE_STATUS_HH_addr, F498_RECEIVE_STATUS_HH_size, F498_RECEIVE_STATUS_HH_offset
#define F498_TS_STATUS_L			 F498_TS_STATUS_L_addr, F498_TS_STATUS_L_size, F498_TS_STATUS_L_offset
#define F498_TS_STATUS_H			 F498_TS_STATUS_H_addr, F498_TS_STATUS_H_size, F498_TS_STATUS_H_offset
#define F498_ERROR			 F498_ERROR_addr, F498_ERROR_size, F498_ERROR_offset
#define F498_EMERGENCY			 F498_EMERGENCY_addr, F498_EMERGENCY_size, F498_EMERGENCY_offset
#define F498_CRE_TS			 F498_CRE_TS_addr, F498_CRE_TS_size, F498_CRE_TS_offset
#define F498_VER			 F498_VER_addr, F498_VER_size, F498_VER_offset
#define F498_M_LOCK			 F498_M_LOCK_addr, F498_M_LOCK_size, F498_M_LOCK_offset
#define F498_UPDATE_READY			 F498_UPDATE_READY_addr, F498_UPDATE_READY_size, F498_UPDATE_READY_offset
#define F498_END_FRAME_HEADER			 F498_END_FRAME_HEADER_addr, F498_END_FRAME_HEADER_size, F498_END_FRAME_HEADER_offset
#define F498_CONTCNT			 F498_CONTCNT_addr, F498_CONTCNT_size, F498_CONTCNT_offset
#define F498_TS_IDENTIFIER_SEL			 F498_TS_IDENTIFIER_SEL_addr, F498_TS_IDENTIFIER_SEL_size, F498_TS_IDENTIFIER_SEL_offset
#define F498_ON_ID_I_L			 F498_ON_ID_I_L_addr, F498_ON_ID_I_L_size, F498_ON_ID_I_L_offset
#define F498_ON_ID_I_H			 F498_ON_ID_I_H_addr, F498_ON_ID_I_H_size, F498_ON_ID_I_H_offset
#define F498_TS_ID_I_L			 F498_TS_ID_I_L_addr, F498_TS_ID_I_L_size, F498_TS_ID_I_L_offset
#define F498_TS_ID_I_H			 F498_TS_ID_I_H_addr, F498_TS_ID_I_H_size, F498_TS_ID_I_H_offset
#define F498_MPEG_DIS			 F498_MPEG_DIS_addr, F498_MPEG_DIS_size, F498_MPEG_DIS_offset
#define F498_MPEG_HDR_DIS			 F498_MPEG_HDR_DIS_addr, F498_MPEG_HDR_DIS_size, F498_MPEG_HDR_DIS_offset
#define F498_RS_FLAG			 F498_RS_FLAG_addr, F498_RS_FLAG_size, F498_RS_FLAG_offset
#define F498_MPEG_FLAG			 F498_MPEG_FLAG_addr, F498_MPEG_FLAG_size, F498_MPEG_FLAG_offset
#define F498_MPEG_GET			 F498_MPEG_GET_addr, F498_MPEG_GET_size, F498_MPEG_GET_offset
#define F498_MPEG_DROP			 F498_MPEG_DROP_addr, F498_MPEG_DROP_size, F498_MPEG_DROP_offset
#define F498_VITERBI_SYNC_GET			 F498_VITERBI_SYNC_GET_addr, F498_VITERBI_SYNC_GET_size, F498_VITERBI_SYNC_GET_offset
#define F498_VITERBI_SYNC_DROP			 F498_VITERBI_SYNC_DROP_addr, F498_VITERBI_SYNC_DROP_size, F498_VITERBI_SYNC_DROP_offset
#define F498_VITERBI_SYN_GO			 F498_VITERBI_SYN_GO_addr, F498_VITERBI_SYN_GO_size, F498_VITERBI_SYN_GO_offset
#define F498_VITERBI_SYN_STOP			 F498_VITERBI_SYN_STOP_addr, F498_VITERBI_SYN_STOP_size, F498_VITERBI_SYN_STOP_offset
#define F498_FRAME_SYNC_DROP			 F498_FRAME_SYNC_DROP_addr, F498_FRAME_SYNC_DROP_size, F498_FRAME_SYNC_DROP_offset
#define F498_FRAME_SYNC_GET			 F498_FRAME_SYNC_GET_addr, F498_FRAME_SYNC_GET_size, F498_FRAME_SYNC_GET_offset
#define F498_INTERLEAVER_DEPTH			 F498_INTERLEAVER_DEPTH_addr, F498_INTERLEAVER_DEPTH_size, F498_INTERLEAVER_DEPTH_offset
#define F498_RS_CORR_CNT_CLEAR			 F498_RS_CORR_CNT_CLEAR_addr, F498_RS_CORR_CNT_CLEAR_size, F498_RS_CORR_CNT_CLEAR_offset
#define F498_RS_UNERR_CNT_CLEAR			 F498_RS_UNERR_CNT_CLEAR_addr, F498_RS_UNERR_CNT_CLEAR_size, F498_RS_UNERR_CNT_CLEAR_offset
#define F498_RS_4_ERROR			 F498_RS_4_ERROR_addr, F498_RS_4_ERROR_size, F498_RS_4_ERROR_offset
#define F498_RS_CLR_ERR			 F498_RS_CLR_ERR_addr, F498_RS_CLR_ERR_size, F498_RS_CLR_ERR_offset
#define F498_RS_CLR_UNC			 F498_RS_CLR_UNC_addr, F498_RS_CLR_UNC_size, F498_RS_CLR_UNC_offset
#define F498_RS_RATE_ADJ			 F498_RS_RATE_ADJ_addr, F498_RS_RATE_ADJ_size, F498_RS_RATE_ADJ_offset
#define F498_DEPTH_AUTOMODE			 F498_DEPTH_AUTOMODE_addr, F498_DEPTH_AUTOMODE_size, F498_DEPTH_AUTOMODE_offset
#define F498_FRAME_SYNC_COUNT			 F498_FRAME_SYNC_COUNT_addr, F498_FRAME_SYNC_COUNT_size, F498_FRAME_SYNC_COUNT_offset
#define F498_MPEG_SYNC			 F498_MPEG_SYNC_addr, F498_MPEG_SYNC_size, F498_MPEG_SYNC_offset
#define F498_VITERBI_I_SYNC			 F498_VITERBI_I_SYNC_addr, F498_VITERBI_I_SYNC_size, F498_VITERBI_I_SYNC_offset
#define F498_VITERBI_Q_SYNC			 F498_VITERBI_Q_SYNC_addr, F498_VITERBI_Q_SYNC_size, F498_VITERBI_Q_SYNC_offset
#define F498_COMB_STATE			 F498_COMB_STATE_addr, F498_COMB_STATE_size, F498_COMB_STATE_offset
#define F498_VITERBI_RATE_I			 F498_VITERBI_RATE_I_addr, F498_VITERBI_RATE_I_size, F498_VITERBI_RATE_I_offset
#define F498_VITERBI_RATE_Q			 F498_VITERBI_RATE_Q_addr, F498_VITERBI_RATE_Q_size, F498_VITERBI_RATE_Q_offset
#define F498_RS_CORRECTED_COUNT_LO			 F498_RS_CORRECTED_COUNT_LO_addr, F498_RS_CORRECTED_COUNT_LO_size, F498_RS_CORRECTED_COUNT_LO_offset
#define F498_RS_CORRECTED_COUNT_HI			 F498_RS_CORRECTED_COUNT_HI_addr, F498_RS_CORRECTED_COUNT_HI_size, F498_RS_CORRECTED_COUNT_HI_offset
#define F498_RS_UNERRORED_COUNT_LO			 F498_RS_UNERRORED_COUNT_LO_addr, F498_RS_UNERRORED_COUNT_LO_size, F498_RS_UNERRORED_COUNT_LO_offset
#define F498_RS_UNERRORED_COUNT_HI			 F498_RS_UNERRORED_COUNT_HI_addr, F498_RS_UNERRORED_COUNT_HI_size, F498_RS_UNERRORED_COUNT_HI_offset
#define F498_RS_UNC_COUNT_LO			 F498_RS_UNC_COUNT_LO_addr, F498_RS_UNC_COUNT_LO_size, F498_RS_UNC_COUNT_LO_offset
#define F498_RS_UNC_COUNT_HI			 F498_RS_UNC_COUNT_HI_addr, F498_RS_UNC_COUNT_HI_size, F498_RS_UNC_COUNT_HI_offset
#define F498_RS_RATE_LO			 F498_RS_RATE_LO_addr, F498_RS_RATE_LO_size, F498_RS_RATE_LO_offset
#define F498_RS_RATE_HI			 F498_RS_RATE_HI_addr, F498_RS_RATE_HI_size, F498_RS_RATE_HI_offset
#define F498_TX_INTERLEAVE_DEPTH			 F498_TX_INTERLEAVE_DEPTH_addr, F498_TX_INTERLEAVE_DEPTH_size, F498_TX_INTERLEAVE_DEPTH_offset
#define F498_RS_ERROR_COUNT_L			 F498_RS_ERROR_COUNT_L_addr, F498_RS_ERROR_COUNT_L_size, F498_RS_ERROR_COUNT_L_offset
#define F498_RS_ERROR_COUNT_H			 F498_RS_ERROR_COUNT_H_addr, F498_RS_ERROR_COUNT_H_size, F498_RS_ERROR_COUNT_H_offset
#define F498_IQ_SWAP			 F498_IQ_SWAP_addr, F498_IQ_SWAP_size, F498_IQ_SWAP_offset
#define F498_OOB_CLK_INV			 F498_OOB_CLK_INV_addr, F498_OOB_CLK_INV_size, F498_OOB_CLK_INV_offset
#define F498_CRX_RATIO			 F498_CRX_RATIO_addr, F498_CRX_RATIO_size, F498_CRX_RATIO_offset
#define F498_DRX_DELAY			 F498_DRX_DELAY_addr, F498_DRX_DELAY_size, F498_DRX_DELAY_offset
#define R498_CTRL_0			 R498_CTRL_0_addr, R498_CTRL_0_size, R498_CTRL_0_offset
#define R498_CTRL_1			 R498_CTRL_1_addr, R498_CTRL_1_size, R498_CTRL_1_offset
#define R498_CTRL_2			 R498_CTRL_2_addr, R498_CTRL_2_size, R498_CTRL_2_offset
#define R498_IT_STATUS1			 R498_IT_STATUS1_addr, R498_IT_STATUS1_size, R498_IT_STATUS1_offset
#define R498_IT_STATUS2			 R498_IT_STATUS2_addr, R498_IT_STATUS2_size, R498_IT_STATUS2_offset
#define R498_IT_EN1			 R498_IT_EN1_addr, R498_IT_EN1_size, R498_IT_EN1_offset
#define R498_IT_EN2			 R498_IT_EN2_addr, R498_IT_EN2_size, R498_IT_EN2_offset
#define R498_CTRL_STATUS			 R498_CTRL_STATUS_addr, R498_CTRL_STATUS_size, R498_CTRL_STATUS_offset
#define R498_TEST_CTL			 R498_TEST_CTL_addr, R498_TEST_CTL_size, R498_TEST_CTL_offset
#define R498_AGC_CTL			 R498_AGC_CTL_addr, R498_AGC_CTL_size, R498_AGC_CTL_offset
#define R498_AGC_IF_CFG			 R498_AGC_IF_CFG_addr, R498_AGC_IF_CFG_size, R498_AGC_IF_CFG_offset
#define R498_AGC_RF_CFG			 R498_AGC_RF_CFG_addr, R498_AGC_RF_CFG_size, R498_AGC_RF_CFG_offset
#define R498_AGC_PWM_CFG			 R498_AGC_PWM_CFG_addr, R498_AGC_PWM_CFG_size, R498_AGC_PWM_CFG_offset
#define R498_AGC_PWR_REF_L			 R498_AGC_PWR_REF_L_addr, R498_AGC_PWR_REF_L_size, R498_AGC_PWR_REF_L_offset
#define R498_AGC_PWR_REF_H			 R498_AGC_PWR_REF_H_addr, R498_AGC_PWR_REF_H_size, R498_AGC_PWR_REF_H_offset
#define R498_AGC_RF_TH_L			 R498_AGC_RF_TH_L_addr, R498_AGC_RF_TH_L_size, R498_AGC_RF_TH_L_offset
#define R498_AGC_RF_TH_H			 R498_AGC_RF_TH_H_addr, R498_AGC_RF_TH_H_size, R498_AGC_RF_TH_H_offset
#define R498_AGC_IF_LTH_L			 R498_AGC_IF_LTH_L_addr, R498_AGC_IF_LTH_L_size, R498_AGC_IF_LTH_L_offset
#define R498_AGC_IF_LTH_H			 R498_AGC_IF_LTH_H_addr, R498_AGC_IF_LTH_H_size, R498_AGC_IF_LTH_H_offset
#define R498_AGC_IF_HTH_L			 R498_AGC_IF_HTH_L_addr, R498_AGC_IF_HTH_L_size, R498_AGC_IF_HTH_L_offset
#define R498_AGC_IF_HTH_H			 R498_AGC_IF_HTH_H_addr, R498_AGC_IF_HTH_H_size, R498_AGC_IF_HTH_H_offset
#define R498_AGC_PWR_RD_L			 R498_AGC_PWR_RD_L_addr, R498_AGC_PWR_RD_L_size, R498_AGC_PWR_RD_L_offset
#define R498_AGC_PWR_RD_M			 R498_AGC_PWR_RD_M_addr, R498_AGC_PWR_RD_M_size, R498_AGC_PWR_RD_M_offset
#define R498_AGC_PWR_RD_H			 R498_AGC_PWR_RD_H_addr, R498_AGC_PWR_RD_H_size, R498_AGC_PWR_RD_H_offset
#define R498_AGC_PWM_IFCMD_L			 R498_AGC_PWM_IFCMD_L_addr, R498_AGC_PWM_IFCMD_L_size, R498_AGC_PWM_IFCMD_L_offset
#define R498_AGC_PWM_IFCMD_H			 R498_AGC_PWM_IFCMD_H_addr, R498_AGC_PWM_IFCMD_H_size, R498_AGC_PWM_IFCMD_H_offset
#define R498_AGC_PWM_RFCMD_L			 R498_AGC_PWM_RFCMD_L_addr, R498_AGC_PWM_RFCMD_L_size, R498_AGC_PWM_RFCMD_L_offset
#define R498_AGC_PWM_RFCMD_H			 R498_AGC_PWM_RFCMD_H_addr, R498_AGC_PWM_RFCMD_H_size, R498_AGC_PWM_RFCMD_H_offset
#define R498_IQDEM_CFG			 R498_IQDEM_CFG_addr, R498_IQDEM_CFG_size, R498_IQDEM_CFG_offset
#define R498_MIX_NCO_LL			 R498_MIX_NCO_LL_addr, R498_MIX_NCO_LL_size, R498_MIX_NCO_LL_offset
#define R498_MIX_NCO_HL			 R498_MIX_NCO_HL_addr, R498_MIX_NCO_HL_size, R498_MIX_NCO_HL_offset
#define R498_MIX_NCO_HH			 R498_MIX_NCO_HH_addr, R498_MIX_NCO_HH_size, R498_MIX_NCO_HH_offset
#define R498_MIX_NCO			 R498_MIX_NCO_addr, R498_MIX_NCO_size, R498_MIX_NCO_offset
#define R498_SRC_NCO_LL			 R498_SRC_NCO_LL_addr, R498_SRC_NCO_LL_size, R498_SRC_NCO_LL_offset
#define R498_SRC_NCO_LH			 R498_SRC_NCO_LH_addr, R498_SRC_NCO_LH_size, R498_SRC_NCO_LH_offset
#define R498_SRC_NCO_HL			 R498_SRC_NCO_HL_addr, R498_SRC_NCO_HL_size, R498_SRC_NCO_HL_offset
#define R498_SRC_NCO_HH			 R498_SRC_NCO_HH_addr, R498_SRC_NCO_HH_size, R498_SRC_NCO_HH_offset
#define R498_SRC_NCO			 R498_SRC_NCO_addr, R498_SRC_NCO_size, R498_SRC_NCO_offset
#define R498_IQDEM_GAIN_SRC_L			 R498_IQDEM_GAIN_SRC_L_addr, R498_IQDEM_GAIN_SRC_L_size, R498_IQDEM_GAIN_SRC_L_offset
#define R498_IQDEM_GAIN_SRC_H			 R498_IQDEM_GAIN_SRC_H_addr, R498_IQDEM_GAIN_SRC_H_size, R498_IQDEM_GAIN_SRC_H_offset
#define R498_IQDEM_DCRM_CFG_LL			 R498_IQDEM_DCRM_CFG_LL_addr, R498_IQDEM_DCRM_CFG_LL_size, R498_IQDEM_DCRM_CFG_LL_offset
#define R498_IQDEM_DCRM_CFG_LH			 R498_IQDEM_DCRM_CFG_LH_addr, R498_IQDEM_DCRM_CFG_LH_size, R498_IQDEM_DCRM_CFG_LH_offset
#define R498_IQDEM_DCRM_CFG_HL			 R498_IQDEM_DCRM_CFG_HL_addr, R498_IQDEM_DCRM_CFG_HL_size, R498_IQDEM_DCRM_CFG_HL_offset
#define R498_IQDEM_DCRM_CFG_HH			 R498_IQDEM_DCRM_CFG_HH_addr, R498_IQDEM_DCRM_CFG_HH_size, R498_IQDEM_DCRM_CFG_HH_offset
#define R498_IQDEM_ADJ_COEFF0			 R498_IQDEM_ADJ_COEFF0_addr, R498_IQDEM_ADJ_COEFF0_size, R498_IQDEM_ADJ_COEFF0_offset
#define R498_IQDEM_ADJ_COEFF1			 R498_IQDEM_ADJ_COEFF1_addr, R498_IQDEM_ADJ_COEFF1_size, R498_IQDEM_ADJ_COEFF1_offset
#define R498_IQDEM_ADJ_COEFF2			 R498_IQDEM_ADJ_COEFF2_addr, R498_IQDEM_ADJ_COEFF2_size, R498_IQDEM_ADJ_COEFF2_offset
#define R498_IQDEM_ADJ_COEFF3			 R498_IQDEM_ADJ_COEFF3_addr, R498_IQDEM_ADJ_COEFF3_size, R498_IQDEM_ADJ_COEFF3_offset
#define R498_IQDEM_ADJ_COEFF4			 R498_IQDEM_ADJ_COEFF4_addr, R498_IQDEM_ADJ_COEFF4_size, R498_IQDEM_ADJ_COEFF4_offset
#define R498_IQDEM_ADJ_COEFF5			 R498_IQDEM_ADJ_COEFF5_addr, R498_IQDEM_ADJ_COEFF5_size, R498_IQDEM_ADJ_COEFF5_offset
#define R498_IQDEM_ADJ_COEFF6			 R498_IQDEM_ADJ_COEFF6_addr, R498_IQDEM_ADJ_COEFF6_size, R498_IQDEM_ADJ_COEFF6_offset
#define R498_IQDEM_ADJ_COEFF7			 R498_IQDEM_ADJ_COEFF7_addr, R498_IQDEM_ADJ_COEFF7_size, R498_IQDEM_ADJ_COEFF7_offset
#define R498_IQDEM_ADJ_EN			 R498_IQDEM_ADJ_EN_addr, R498_IQDEM_ADJ_EN_size, R498_IQDEM_ADJ_EN_offset
#define R498_IQDEM_ADJ_AGC_REF			 R498_IQDEM_ADJ_AGC_REF_addr, R498_IQDEM_ADJ_AGC_REF_size, R498_IQDEM_ADJ_AGC_REF_offset
#define R498_ALLPASSFILT1			 R498_ALLPASSFILT1_addr, R498_ALLPASSFILT1_size, R498_ALLPASSFILT1_offset
#define R498_ALLPASSFILT2			 R498_ALLPASSFILT2_addr, R498_ALLPASSFILT2_size, R498_ALLPASSFILT2_offset
#define R498_ALLPASSFILT3			 R498_ALLPASSFILT3_addr, R498_ALLPASSFILT3_size, R498_ALLPASSFILT3_offset
#define R498_ALLPASSFILT4			 R498_ALLPASSFILT4_addr, R498_ALLPASSFILT4_size, R498_ALLPASSFILT4_offset
#define R498_ALLPASSFILT5			 R498_ALLPASSFILT5_addr, R498_ALLPASSFILT5_size, R498_ALLPASSFILT5_offset
#define R498_ALLPASSFILT6			 R498_ALLPASSFILT6_addr, R498_ALLPASSFILT6_size, R498_ALLPASSFILT6_offset
#define R498_ALLPASSFILT7			 R498_ALLPASSFILT7_addr, R498_ALLPASSFILT7_size, R498_ALLPASSFILT7_offset
#define R498_ALLPASSFILT8			 R498_ALLPASSFILT8_addr, R498_ALLPASSFILT8_size, R498_ALLPASSFILT8_offset
#define R498_ALLPASSFILT9			 R498_ALLPASSFILT9_addr, R498_ALLPASSFILT9_size, R498_ALLPASSFILT9_offset
#define R498_ALLPASSFILT10			 R498_ALLPASSFILT10_addr, R498_ALLPASSFILT10_size, R498_ALLPASSFILT10_offset
#define R498_ALLPASSFILT11			 R498_ALLPASSFILT11_addr, R498_ALLPASSFILT11_size, R498_ALLPASSFILT11_offset
#define R498_TRL_AGC_CFG			 R498_TRL_AGC_CFG_addr, R498_TRL_AGC_CFG_size, R498_TRL_AGC_CFG_offset
#define R498_TRL_LPF_CFG			 R498_TRL_LPF_CFG_addr, R498_TRL_LPF_CFG_size, R498_TRL_LPF_CFG_offset
#define R498_TRL_LPF_ACQ_GAIN			 R498_TRL_LPF_ACQ_GAIN_addr, R498_TRL_LPF_ACQ_GAIN_size, R498_TRL_LPF_ACQ_GAIN_offset
#define R498_TRL_LPF_TRK_GAIN			 R498_TRL_LPF_TRK_GAIN_addr, R498_TRL_LPF_TRK_GAIN_size, R498_TRL_LPF_TRK_GAIN_offset
#define R498_TRL_LPF_OUT_GAIN			 R498_TRL_LPF_OUT_GAIN_addr, R498_TRL_LPF_OUT_GAIN_size, R498_TRL_LPF_OUT_GAIN_offset
#define R498_TRL_LOCKDET_LTH			 R498_TRL_LOCKDET_LTH_addr, R498_TRL_LOCKDET_LTH_size, R498_TRL_LOCKDET_LTH_offset
#define R498_TRL_LOCKDET_HTH			 R498_TRL_LOCKDET_HTH_addr, R498_TRL_LOCKDET_HTH_size, R498_TRL_LOCKDET_HTH_offset
#define R498_TRL_LOCKDET_TRGVAL			 R498_TRL_LOCKDET_TRGVAL_addr, R498_TRL_LOCKDET_TRGVAL_size, R498_TRL_LOCKDET_TRGVAL_offset
#define R498_FSM_STATE			 R498_FSM_STATE_addr, R498_FSM_STATE_size, R498_FSM_STATE_offset
#define R498_FSM_CTL			 R498_FSM_CTL_addr, R498_FSM_CTL_size, R498_FSM_CTL_offset
#define R498_FSM_STS			 R498_FSM_STS_addr, R498_FSM_STS_size, R498_FSM_STS_offset
#define R498_FSM_SNR0_HTH			 R498_FSM_SNR0_HTH_addr, R498_FSM_SNR0_HTH_size, R498_FSM_SNR0_HTH_offset
#define R498_FSM_SNR1_HTH			 R498_FSM_SNR1_HTH_addr, R498_FSM_SNR1_HTH_size, R498_FSM_SNR1_HTH_offset
#define R498_FSM_SNR2_HTH			 R498_FSM_SNR2_HTH_addr, R498_FSM_SNR2_HTH_size, R498_FSM_SNR2_HTH_offset
#define R498_FSM_SNR0_LTH			 R498_FSM_SNR0_LTH_addr, R498_FSM_SNR0_LTH_size, R498_FSM_SNR0_LTH_offset
#define R498_FSM_SNR1_LTH			 R498_FSM_SNR1_LTH_addr, R498_FSM_SNR1_LTH_size, R498_FSM_SNR1_LTH_offset
#define R498_FSM_RCA_HTH			 R498_FSM_RCA_HTH_addr, R498_FSM_RCA_HTH_size, R498_FSM_RCA_HTH_offset
#define R498_FSM_TEMPO			 R498_FSM_TEMPO_addr, R498_FSM_TEMPO_size, R498_FSM_TEMPO_offset
#define R498_FSM_CONFIG			 R498_FSM_CONFIG_addr, R498_FSM_CONFIG_size, R498_FSM_CONFIG_offset
#define R498_EQU_I_TESTTAP_L			 R498_EQU_I_TESTTAP_L_addr, R498_EQU_I_TESTTAP_L_size, R498_EQU_I_TESTTAP_L_offset
#define R498_EQU_I_TESTTAP_M			 R498_EQU_I_TESTTAP_M_addr, R498_EQU_I_TESTTAP_M_size, R498_EQU_I_TESTTAP_M_offset
#define R498_EQU_I_TESTTAP_H			 R498_EQU_I_TESTTAP_H_addr, R498_EQU_I_TESTTAP_H_size, R498_EQU_I_TESTTAP_H_offset
#define R498_EQU_TESTAP_CFG			 R498_EQU_TESTAP_CFG_addr, R498_EQU_TESTAP_CFG_size, R498_EQU_TESTAP_CFG_offset
#define R498_EQU_Q_TESTTAP_L			 R498_EQU_Q_TESTTAP_L_addr, R498_EQU_Q_TESTTAP_L_size, R498_EQU_Q_TESTTAP_L_offset
#define R498_EQU_Q_TESTTAP_M			 R498_EQU_Q_TESTTAP_M_addr, R498_EQU_Q_TESTTAP_M_size, R498_EQU_Q_TESTTAP_M_offset
#define R498_EQU_Q_TESTTAP_H			 R498_EQU_Q_TESTTAP_H_addr, R498_EQU_Q_TESTTAP_H_size, R498_EQU_Q_TESTTAP_H_offset
#define R498_EQU_TAP_CTRL			 R498_EQU_TAP_CTRL_addr, R498_EQU_TAP_CTRL_size, R498_EQU_TAP_CTRL_offset
#define R498_EQU_CTR_CRL_CONTROL_L			 R498_EQU_CTR_CRL_CONTROL_L_addr, R498_EQU_CTR_CRL_CONTROL_L_size, R498_EQU_CTR_CRL_CONTROL_L_offset
#define R498_EQU_CTR_CRL_CONTROL_H			 R498_EQU_CTR_CRL_CONTROL_H_addr, R498_EQU_CTR_CRL_CONTROL_H_size, R498_EQU_CTR_CRL_CONTROL_H_offset
#define R498_EQU_CTR_HIPOW_L			 R498_EQU_CTR_HIPOW_L_addr, R498_EQU_CTR_HIPOW_L_size, R498_EQU_CTR_HIPOW_L_offset
#define R498_EQU_CTR_HIPOW_H			 R498_EQU_CTR_HIPOW_H_addr, R498_EQU_CTR_HIPOW_H_size, R498_EQU_CTR_HIPOW_H_offset
#define R498_EQU_I_EQU_LO			 R498_EQU_I_EQU_LO_addr, R498_EQU_I_EQU_LO_size, R498_EQU_I_EQU_LO_offset
#define R498_EQU_I_EQU_HI			 R498_EQU_I_EQU_HI_addr, R498_EQU_I_EQU_HI_size, R498_EQU_I_EQU_HI_offset
#define R498_EQU_Q_EQU_LO			 R498_EQU_Q_EQU_LO_addr, R498_EQU_Q_EQU_LO_size, R498_EQU_Q_EQU_LO_offset
#define R498_EQU_Q_EQU_HI			 R498_EQU_Q_EQU_HI_addr, R498_EQU_Q_EQU_HI_size, R498_EQU_Q_EQU_HI_offset
#define R498_EQU_MAPPER			 R498_EQU_MAPPER_addr, R498_EQU_MAPPER_size, R498_EQU_MAPPER_offset
#define R498_EQU_SWEEP_RATE			 R498_EQU_SWEEP_RATE_addr, R498_EQU_SWEEP_RATE_size, R498_EQU_SWEEP_RATE_offset
#define R498_EQU_SNR_LO			 R498_EQU_SNR_LO_addr, R498_EQU_SNR_LO_size, R498_EQU_SNR_LO_offset
#define R498_EQU_SNR_HI			 R498_EQU_SNR_HI_addr, R498_EQU_SNR_HI_size, R498_EQU_SNR_HI_offset
#define R498_EQU_GAMMA_LO			 R498_EQU_GAMMA_LO_addr, R498_EQU_GAMMA_LO_size, R498_EQU_GAMMA_LO_offset
#define R498_EQU_GAMMA_HI			 R498_EQU_GAMMA_HI_addr, R498_EQU_GAMMA_HI_size, R498_EQU_GAMMA_HI_offset
#define R498_EQU_ERR_GAIN			 R498_EQU_ERR_GAIN_addr, R498_EQU_ERR_GAIN_size, R498_EQU_ERR_GAIN_offset
#define R498_EQU_RADIUS			 R498_EQU_RADIUS_addr, R498_EQU_RADIUS_size, R498_EQU_RADIUS_offset
#define R498_EQU_FFE_MAINTAP			 R498_EQU_FFE_MAINTAP_addr, R498_EQU_FFE_MAINTAP_size, R498_EQU_FFE_MAINTAP_offset
#define R498_EQU_FFE_LEAKAGE			 R498_EQU_FFE_LEAKAGE_addr, R498_EQU_FFE_LEAKAGE_size, R498_EQU_FFE_LEAKAGE_offset
#define R498_EQU_FFE_MAINTAP_POS			 R498_EQU_FFE_MAINTAP_POS_addr, R498_EQU_FFE_MAINTAP_POS_size, R498_EQU_FFE_MAINTAP_POS_offset
#define R498_EQU_GAIN_WIDE			 R498_EQU_GAIN_WIDE_addr, R498_EQU_GAIN_WIDE_size, R498_EQU_GAIN_WIDE_offset
#define R498_EQU_GAIN_NARROW			 R498_EQU_GAIN_NARROW_addr, R498_EQU_GAIN_NARROW_size, R498_EQU_GAIN_NARROW_offset
#define R498_EQU_CTR_LPF_GAIN			 R498_EQU_CTR_LPF_GAIN_addr, R498_EQU_CTR_LPF_GAIN_size, R498_EQU_CTR_LPF_GAIN_offset
#define R498_EQU_CRL_LPF_GAIN			 R498_EQU_CRL_LPF_GAIN_addr, R498_EQU_CRL_LPF_GAIN_size, R498_EQU_CRL_LPF_GAIN_offset
#define R498_EQU_GLOBAL_GAIN			 R498_EQU_GLOBAL_GAIN_addr, R498_EQU_GLOBAL_GAIN_size, R498_EQU_GLOBAL_GAIN_offset
#define R498_EQU_CRL_LD_SEN			 R498_EQU_CRL_LD_SEN_addr, R498_EQU_CRL_LD_SEN_size, R498_EQU_CRL_LD_SEN_offset
#define R498_EQU_CRL_LD_VAL			 R498_EQU_CRL_LD_VAL_addr, R498_EQU_CRL_LD_VAL_size, R498_EQU_CRL_LD_VAL_offset
#define R498_EQU_CRL_TFR			 R498_EQU_CRL_TFR_addr, R498_EQU_CRL_TFR_size, R498_EQU_CRL_TFR_offset
#define R498_EQU_CRL_BISTH_LO			 R498_EQU_CRL_BISTH_LO_addr, R498_EQU_CRL_BISTH_LO_size, R498_EQU_CRL_BISTH_LO_offset
#define R498_EQU_CRL_BISTH_HI			 R498_EQU_CRL_BISTH_HI_addr, R498_EQU_CRL_BISTH_HI_size, R498_EQU_CRL_BISTH_HI_offset
#define R498_EQU_SWEEP_RANGE_LO			 R498_EQU_SWEEP_RANGE_LO_addr, R498_EQU_SWEEP_RANGE_LO_size, R498_EQU_SWEEP_RANGE_LO_offset
#define R498_EQU_SWEEP_RANGE_HI			 R498_EQU_SWEEP_RANGE_HI_addr, R498_EQU_SWEEP_RANGE_HI_size, R498_EQU_SWEEP_RANGE_HI_offset
#define R498_EQU_CRL_LIMITER			 R498_EQU_CRL_LIMITER_addr, R498_EQU_CRL_LIMITER_size, R498_EQU_CRL_LIMITER_offset
#define R498_EQU_MODULUS_MAP			 R498_EQU_MODULUS_MAP_addr, R498_EQU_MODULUS_MAP_size, R498_EQU_MODULUS_MAP_offset
#define R498_EQU_PNT_GAIN			 R498_EQU_PNT_GAIN_addr, R498_EQU_PNT_GAIN_size, R498_EQU_PNT_GAIN_offset
#define R498_FEC_AC_CTR_0			 R498_FEC_AC_CTR_0_addr, R498_FEC_AC_CTR_0_size, R498_FEC_AC_CTR_0_offset
#define R498_FEC_AC_CTR_1			 R498_FEC_AC_CTR_1_addr, R498_FEC_AC_CTR_1_size, R498_FEC_AC_CTR_1_offset
#define R498_FEC_AC_CTR_2			 R498_FEC_AC_CTR_2_addr, R498_FEC_AC_CTR_2_size, R498_FEC_AC_CTR_2_offset
#define R498_FEC_AC_CTR_3			 R498_FEC_AC_CTR_3_addr, R498_FEC_AC_CTR_3_size, R498_FEC_AC_CTR_3_offset
#define R498_FEC_STATUS			 R498_FEC_STATUS_addr, R498_FEC_STATUS_size, R498_FEC_STATUS_offset
#define R498_RS_COUNTER_0			 R498_RS_COUNTER_0_addr, R498_RS_COUNTER_0_size, R498_RS_COUNTER_0_offset
#define R498_RS_COUNTER_1			 R498_RS_COUNTER_1_addr, R498_RS_COUNTER_1_size, R498_RS_COUNTER_1_offset
#define R498_RS_COUNTER_2			 R498_RS_COUNTER_2_addr, R498_RS_COUNTER_2_size, R498_RS_COUNTER_2_offset
#define R498_RS_COUNTER_3			 R498_RS_COUNTER_3_addr, R498_RS_COUNTER_3_size, R498_RS_COUNTER_3_offset
#define R498_RS_COUNTER_4			 R498_RS_COUNTER_4_addr, R498_RS_COUNTER_4_size, R498_RS_COUNTER_4_offset
#define R498_RS_COUNTER_5			 R498_RS_COUNTER_5_addr, R498_RS_COUNTER_5_size, R498_RS_COUNTER_5_offset
#define R498_BERT_0			 R498_BERT_0_addr, R498_BERT_0_size, R498_BERT_0_offset
#define R498_BERT_1			 R498_BERT_1_addr, R498_BERT_1_size, R498_BERT_1_offset
#define R498_BERT_2			 R498_BERT_2_addr, R498_BERT_2_size, R498_BERT_2_offset
#define R498_BERT_3			 R498_BERT_3_addr, R498_BERT_3_size, R498_BERT_3_offset
#define R498_OUTFORMAT_0			 R498_OUTFORMAT_0_addr, R498_OUTFORMAT_0_size, R498_OUTFORMAT_0_offset
#define R498_OUTFORMAT_1			 R498_OUTFORMAT_1_addr, R498_OUTFORMAT_1_size, R498_OUTFORMAT_1_offset
#define R498_SMOOTHER_0			 R498_SMOOTHER_0_addr, R498_SMOOTHER_0_size, R498_SMOOTHER_0_offset
#define R498_SMOOTHER_1			 R498_SMOOTHER_1_addr, R498_SMOOTHER_1_size, R498_SMOOTHER_1_offset
#define R498_SMOOTHER_2			 R498_SMOOTHER_2_addr, R498_SMOOTHER_2_size, R498_SMOOTHER_2_offset
#define R498_TSMF_CTRL_0			 R498_TSMF_CTRL_0_addr, R498_TSMF_CTRL_0_size, R498_TSMF_CTRL_0_offset
#define R498_TSMF_CTRL_1			 R498_TSMF_CTRL_1_addr, R498_TSMF_CTRL_1_size, R498_TSMF_CTRL_1_offset
#define R498_TSMF_CTRL_3			 R498_TSMF_CTRL_3_addr, R498_TSMF_CTRL_3_size, R498_TSMF_CTRL_3_offset
#define R498_TS_ON_ID_0			 R498_TS_ON_ID_0_addr, R498_TS_ON_ID_0_size, R498_TS_ON_ID_0_offset
#define R498_TS_ON_ID_1			 R498_TS_ON_ID_1_addr, R498_TS_ON_ID_1_size, R498_TS_ON_ID_1_offset
#define R498_TS_ON_ID_2			 R498_TS_ON_ID_2_addr, R498_TS_ON_ID_2_size, R498_TS_ON_ID_2_offset
#define R498_TS_ON_ID_3			 R498_TS_ON_ID_3_addr, R498_TS_ON_ID_3_size, R498_TS_ON_ID_3_offset
#define R498_RE_STATUS_0			 R498_RE_STATUS_0_addr, R498_RE_STATUS_0_size, R498_RE_STATUS_0_offset
#define R498_RE_STATUS_1			 R498_RE_STATUS_1_addr, R498_RE_STATUS_1_size, R498_RE_STATUS_1_offset
#define R498_RE_STATUS_2			 R498_RE_STATUS_2_addr, R498_RE_STATUS_2_size, R498_RE_STATUS_2_offset
#define R498_RE_STATUS_3			 R498_RE_STATUS_3_addr, R498_RE_STATUS_3_size, R498_RE_STATUS_3_offset
#define R498_TS_STATUS_0			 R498_TS_STATUS_0_addr, R498_TS_STATUS_0_size, R498_TS_STATUS_0_offset
#define R498_TS_STATUS_1			 R498_TS_STATUS_1_addr, R498_TS_STATUS_1_size, R498_TS_STATUS_1_offset
#define R498_TS_STATUS_2			 R498_TS_STATUS_2_addr, R498_TS_STATUS_2_size, R498_TS_STATUS_2_offset
#define R498_TS_STATUS_3			 R498_TS_STATUS_3_addr, R498_TS_STATUS_3_size, R498_TS_STATUS_3_offset
#define R498_T_O_ID_0			 R498_T_O_ID_0_addr, R498_T_O_ID_0_size, R498_T_O_ID_0_offset
#define R498_T_O_ID_1			 R498_T_O_ID_1_addr, R498_T_O_ID_1_size, R498_T_O_ID_1_offset
#define R498_T_O_ID_2			 R498_T_O_ID_2_addr, R498_T_O_ID_2_size, R498_T_O_ID_2_offset
#define R498_T_O_ID_3			 R498_T_O_ID_3_addr, R498_T_O_ID_3_size, R498_T_O_ID_3_offset
#define R498_MPEG_CTL			 R498_MPEG_CTL_addr, R498_MPEG_CTL_size, R498_MPEG_CTL_offset
#define R498_MPEG_SYNC_ACQ			 R498_MPEG_SYNC_ACQ_addr, R498_MPEG_SYNC_ACQ_size, R498_MPEG_SYNC_ACQ_offset
#define R498_FECB_MPEG_SYNC_LOSS			 R498_FECB_MPEG_SYNC_LOSS_addr, R498_FECB_MPEG_SYNC_LOSS_size, R498_FECB_MPEG_SYNC_LOSS_offset
#define R498_VITERBI_SYNC_ACQ			 R498_VITERBI_SYNC_ACQ_addr, R498_VITERBI_SYNC_ACQ_size, R498_VITERBI_SYNC_ACQ_offset
#define R498_VITERBI_SYNC_LOSS			 R498_VITERBI_SYNC_LOSS_addr, R498_VITERBI_SYNC_LOSS_size, R498_VITERBI_SYNC_LOSS_offset
#define R498_VITERBI_SYNC_GO			 R498_VITERBI_SYNC_GO_addr, R498_VITERBI_SYNC_GO_size, R498_VITERBI_SYNC_GO_offset
#define R498_VITERBI_SYNC_STOP			 R498_VITERBI_SYNC_STOP_addr, R498_VITERBI_SYNC_STOP_size, R498_VITERBI_SYNC_STOP_offset
#define R498_FS_SYNC			 R498_FS_SYNC_addr, R498_FS_SYNC_size, R498_FS_SYNC_offset
#define R498_IN_DEPTH			 R498_IN_DEPTH_addr, R498_IN_DEPTH_size, R498_IN_DEPTH_offset
#define R498_RS_CONTROL			 R498_RS_CONTROL_addr, R498_RS_CONTROL_size, R498_RS_CONTROL_offset
#define R498_DEINT_CONTROL			 R498_DEINT_CONTROL_addr, R498_DEINT_CONTROL_size, R498_DEINT_CONTROL_offset
#define R498_SYNC_STAT			 R498_SYNC_STAT_addr, R498_SYNC_STAT_size, R498_SYNC_STAT_offset
#define R498_VITERBI_I_RATE			 R498_VITERBI_I_RATE_addr, R498_VITERBI_I_RATE_size, R498_VITERBI_I_RATE_offset
#define R498_VITERBI_Q_RATE			 R498_VITERBI_Q_RATE_addr, R498_VITERBI_Q_RATE_size, R498_VITERBI_Q_RATE_offset
#define R498_RS_CORR_COUNT_L			 R498_RS_CORR_COUNT_L_addr, R498_RS_CORR_COUNT_L_size, R498_RS_CORR_COUNT_L_offset
#define R498_RS_CORR_COUNT_H			 R498_RS_CORR_COUNT_H_addr, R498_RS_CORR_COUNT_H_size, R498_RS_CORR_COUNT_H_offset
#define R498_RS_UNERR_COUNT_L			 R498_RS_UNERR_COUNT_L_addr, R498_RS_UNERR_COUNT_L_size, R498_RS_UNERR_COUNT_L_offset
#define R498_RS_UNERR_COUNT_H			 R498_RS_UNERR_COUNT_H_addr, R498_RS_UNERR_COUNT_H_size, R498_RS_UNERR_COUNT_H_offset
#define R498_RS_UNC_COUNT_L			 R498_RS_UNC_COUNT_L_addr, R498_RS_UNC_COUNT_L_size, R498_RS_UNC_COUNT_L_offset
#define R498_RS_UNC_COUNT_H			 R498_RS_UNC_COUNT_H_addr, R498_RS_UNC_COUNT_H_size, R498_RS_UNC_COUNT_H_offset
#define R498_RS_RATE_L			 R498_RS_RATE_L_addr, R498_RS_RATE_L_size, R498_RS_RATE_L_offset
#define R498_RS_RATE_H			 R498_RS_RATE_H_addr, R498_RS_RATE_H_size, R498_RS_RATE_H_offset
#define R498_TX_INTERLEAVER_DEPTH			 R498_TX_INTERLEAVER_DEPTH_addr, R498_TX_INTERLEAVER_DEPTH_size, R498_TX_INTERLEAVER_DEPTH_offset
#define R498_RS_ERR_CNT_L			 R498_RS_ERR_CNT_L_addr, R498_RS_ERR_CNT_L_size, R498_RS_ERR_CNT_L_offset
#define R498_RS_ERR_CNT_H			 R498_RS_ERR_CNT_H_addr, R498_RS_ERR_CNT_H_size, R498_RS_ERR_CNT_H_offset
#define R498_OOB_DUTY			 R498_OOB_DUTY_addr, R498_OOB_DUTY_size, R498_OOB_DUTY_offset
#define R498_OOB_DELAY			 R498_OOB_DELAY_addr, R498_OOB_DELAY_size, R498_OOB_DELAY_offset




/* Number of registers */
	#define		STB0498_NBREGS	190

	/* Number of fields */
	#define		STB0498_NBFIELDS	364
	long BinaryFloatDiv(long n1, long n2, int precision);
	U32 PowOf2(int number);

	/****************************************************************
						COMMON STRUCTURES AND TYPEDEF
	 ****************************************************************/
	typedef int FE_498_Handle_t;

	/*	Signal type enum	*/
	typedef enum
	{
		FE_498_NOTUNER,
		FE_498_NOAGC,
		FE_498_NOSIGNAL,
		FE_498_NOTIMING,
		FE_498_TIMINGOK,
		FE_498_NOCARRIER,
		FE_498_CARRIEROK,
		FE_498_NOBLIND,
		FE_498_BLINDOK,
		FE_498_NODEMOD,
		FE_498_DEMODOK,
		FE_498_DATAOK
	} FE_498_SIGNALTYPE_t;

	typedef enum
	{
		FE_498_NO_ERROR,
	    FE_498_INVALID_HANDLE,
	    FE_498_ALLOCATION,
	    FE_498_BAD_PARAMETER,
	    FE_498_ALREADY_INITIALIZED,
	    FE_498_I2C_ERROR,
	    FE_498_SEARCH_FAILED,
	    FE_498_TRACKING_FAILED,
	    FE_498_TERM_FAILED
	} FE_498_Error_t;


	typedef enum
	{
	  	FE_498_MOD_QAM4,
	  	FE_498_MOD_QAM16,
	  	FE_498_MOD_QAM32,
	  	FE_498_MOD_QAM64,
	  	FE_498_MOD_QAM128,
	  	FE_498_MOD_QAM256,
	  	FE_498_MOD_QAM512,
	  	FE_498_MOD_QAM1024
	} FE_498_Modulation_t;

    typedef enum
    {
		FE_498_MOD_FECAC,
		FE_498_MOD_FECB
		
	} FE_498_Fec_t;

	typedef enum
	{
		FE_498_SPECT_NORMAL,
		FE_498_SPECT_INVERTED
	}FE_498_SpectInv_t;




	/****************************************************************
						INIT STRUCTURES
	 ****************************************************************/
	/*
		structure passed to the FE_498_Init() function
	*/
	typedef enum
	{
		FE_498_PARALLEL_NORMCLOCK,
		FE_498_PARALLEL_INVCLOCK,
		FE_498_DVBCI_NORMCLOCK,
		FE_498_DVBCI_INVCLOCK,
		FE_498_SERIAL_NORMCLOCK,
		FE_498_SERIAL_INVCLOCK,
		FE_498_SERIAL_NORMFIFOCLOCK,
		FE_498_SERIAL_INVFIFOCLOCK
	} FE_498_Clock_t;

	typedef enum
	{
		FE_498_PARITY_ON,
		FE_498_PARITY_OFF
	} FE_498_DataParity_t;


	typedef struct
	{
		char 					DemodName[20];	/* STB0498 name */
		U8						DemodI2cAddr;	/* STB0498 I2c address */
        /*TUNER_Model_t			TunerModel;*/		/* Tuner model */
        char 					TunerName[20];  /* Tuner name */
		U8						TunerI2cAddr;	/* Tuner I2c address */
		FE_498_DataParity_t	Parity;    	    /* Parity bytes */
		FE_498_Clock_t			Clock;			/* TS Format */
	} FE_498_InitParams_t;


	/****************************************************************
						SEARCH STRUCTURES
	 ****************************************************************/

	typedef struct
	{
		U32 Frequency_Khz;					/* channel frequency (in KHz)		*/
		U32 SymbolRate_Bds;					/* channel symbol rate  (in bds)	*/
		U32 SearchRange_Hz;					/* range of the search (in Hz) 		*/
		U32 SweepRate_Hz;					/* Sweep Rate (in Hz)		 		*/
		FE_498_Modulation_t Modulation;	/* modulation						*/
		FE_498_Fec_t FecType;                  /* FEC Type                         */
		U32 ExternalClock;
	} FE_498_SearchParams_t;


	typedef struct
	{
		BOOL Locked;						/* channel found	 					*/
		U32	Frequency_Khz;					/* found frequency (in Khz)				*/
		U32 SymbolRate_Bds;					/* found symbol rate (in Bds)			*/
		FE_498_Modulation_t Modulation;	/* modulation							*/
		FE_498_SpectInv_t SpectInv;		/* Spectrum Inversion					*/
	} FE_498_SearchResult_t;

	/************************
		INFO STRUCTURE
	************************/
	typedef struct
	{
		BOOL Locked;						/* Channel locked						*/
		U32 Frequency;						/* Channel frequency (in KHz)			*/
		U32 SymbolRate;						/* Channel symbol rate  (in Mbds)		*/
		FE_498_Modulation_t Modulation;	/* modulation							*/
		FE_498_SpectInv_t SpectInversion;	/* Spectrum Inversion					*/
		S32 Power_dBmx10;					/* Power of the RF signal (dBm x 10)	*/
		U32	CN_dBx10;						/* Carrier to noise ratio (dB x 10)		*/
		U32	BER;							/* Bit error rate (x 10000)				*/
		
	} FE_498_SignalInfo_t;

	#define DEMOD_CLK 27000000


	/****************************************************************
						API FUNCTIONS
	****************************************************************/

        FE_498_Error_t  FE_498_EstimatePower(U32* power);
        
	FE_498_Error_t	FE_498_Search(FE_498_SearchParams_t *pParams,FE_498_SearchResult_t *pResult, STTUNER_Handle_t   TopLevelHandle);

	FE_498_Error_t	FE_498_Status(FE_498_Handle_t Handle,FE_498_SIGNALTYPE_t *SignalType);

	FE_498_Error_t	FE_498_GetSignalInfo(FE_498_SignalInfo_t *pInfo);



void Set498QAMReg(U32 addr, U32 size, U32 offset, U32 value);
U32 Get498QAMReg(U32 addr, U32 size, U32 offset);








/**********NQAM defines**********************/
/*MPX mapped addresses*/
#define VIDEO_MISC_CONFIG_BASE_ADDR         0xa3c02000 
#define CABLE_MISC_CONFIG_BASE_ADDR         0xa3ffe000

/*actual base addresses on 498*/
/*#define VIDEO_MISC_CONFIG_BASE_ADDR         0xfe002000 
#define CABLE_MISC_CONFIG_BASE_ADDR         0xfd0fe000*/

#define NQAM_v_ts0_str			        "v_ts0"
#define NQAM_v_ts1_str			        "v_ts1"
#define NQAM_v_ts2_str			        "v_ts2"
#define NQAM_oob_sel_str			"oob_sel"
#define NQAM_FRAMING_MODE_str			"FRAMING_MODE"
#define NQAM_MCLK_MODE_str			"MCLK_MODE"
#define NQAM_TST_SEL_str			"TST_SEL"
#define NQAM_adctoqam0_str			"adctoqam0"
#define NQAM_adctoqam1_str			"adctoqam1"
#define NQAM_adctoqam2_str			"adctoqam2"
#define NQAM_adctosync_str			"adctosync"
#define NQAM_m_ts0_str			        "m_ts0"
#define NQAM_m_ts1_str			        "m_ts1"
#define NQAM_m_ts2_str			        "m_ts2"
#define NQAM_src_kRx_str			"src_kRx"
#define NQAM_src_kRx_receip_str			"src_kRx_receip"
#define NQAM_src_kLO_str			"src_kLO"
#define NQAM_src_kLO_scale_str			"src_kLO_scale"
#define NQAM_src_kLO_receip_str			"src_kLO_receip"
#define NQAM_src_kL1_str			"src_kL1"
#define NQAM_src_kM1_str			"src_kM1"
#define NQAM_src_gain_str			"src_gain"
#define NQAM_src_track_str			"src_track"
#define NQAM_src_kTx_str			"src_kTx"
#define NQAM_lin_gain_mant_str			"lin_gain_mant"
#define NQAM_lin_gain_expt_str			"lin_gain_expt"
#define NQAM_int_gain_mant_str			"int_gain_mant"
#define NQAM_int_gain_expt_str			"int_gain_expt"
#define NQAM_timing_polarity_str		"timing_polarity"
#define NQAM_timing_gain_str			"timing_gain"
#define NQAM_timing_break_str			"timing_break"
#define NQAM_timing_clear_str			"timing_clear"
#define NQAM_timing_cond_str			"timing_cond"
#define NQAM_lpf2_freq_str			"lpf2_freq"
#define NQAM_lpf2_clear_str			"lpf2_clear"
#define NQAM_lpf2_enable_str			"lpf2_enable"
#define NQAM_lpf2_gain_str			"lpf2_gain"
#define NQAM_nyq_feed_zero_str			"nyq_feed_zero"
#define NQAM_phase_symbol_str			"phase_symbol"
#define NQAM_dc_seed_str			"dc_seed"
#define NQAM_dc_active_str			"dc_active"
#define NQAM_dc_clear_str			"dc_clear"
#define NQAM_dc_load_str			"dc_load"
#define NQAM_dc_trace_str			"dc_trace"
#define NQAM_format_clear_str			"format_clear"
#define NQAM_format_twos_str			"format_twos"


#define NQAM_v_ts0_addr			        (VIDEO_MISC_CONFIG_BASE_ADDR + 0x0004)
#define NQAM_v_ts1_addr			        (VIDEO_MISC_CONFIG_BASE_ADDR + 0x0004)
#define NQAM_v_ts2_addr			        (VIDEO_MISC_CONFIG_BASE_ADDR + 0x0004)
#define NQAM_oob_sel_addr			(VIDEO_MISC_CONFIG_BASE_ADDR + 0x0004)
#define NQAM_FRAMING_MODE_addr			(CABLE_MISC_CONFIG_BASE_ADDR + 0x0000)
#define NQAM_MCLK_MODE_addr			(CABLE_MISC_CONFIG_BASE_ADDR + 0x0004)
#define NQAM_TST_SEL_addr			(CABLE_MISC_CONFIG_BASE_ADDR + 0x000C)
#define NQAM_adctoqam0_addr			(CABLE_MISC_CONFIG_BASE_ADDR + 0x0010)
#define NQAM_adctoqam1_addr			(CABLE_MISC_CONFIG_BASE_ADDR + 0x0010)
#define NQAM_adctoqam2_addr			(CABLE_MISC_CONFIG_BASE_ADDR + 0x0010)
#define NQAM_adctosync_addr			(CABLE_MISC_CONFIG_BASE_ADDR + 0x0010)
#define NQAM_m_ts0_addr			        (CABLE_MISC_CONFIG_BASE_ADDR + 0x0010)
#define NQAM_m_ts1_addr			        (CABLE_MISC_CONFIG_BASE_ADDR + 0x0010)
#define NQAM_m_ts2_addr			        (CABLE_MISC_CONFIG_BASE_ADDR + 0x0010)
#define NQAM_src_kRx_addr			(CABLE_MISC_CONFIG_BASE_ADDR + 0x0100)
#define NQAM_src_kRx_receip_addr		(CABLE_MISC_CONFIG_BASE_ADDR + 0x0100)
#define NQAM_src_kLO_addr			(CABLE_MISC_CONFIG_BASE_ADDR + 0x0104)
#define NQAM_src_kLO_scale_addr			(CABLE_MISC_CONFIG_BASE_ADDR + 0x0104)
#define NQAM_src_kLO_receip_addr		(CABLE_MISC_CONFIG_BASE_ADDR + 0x0104)                                        
#define NQAM_src_kL1_addr			(CABLE_MISC_CONFIG_BASE_ADDR + 0x0108)                                        
#define NQAM_src_kM1_addr			(CABLE_MISC_CONFIG_BASE_ADDR + 0x0108)                                        
#define NQAM_src_gain_addr			(CABLE_MISC_CONFIG_BASE_ADDR + 0x010C)                                        
#define NQAM_src_track_addr			(CABLE_MISC_CONFIG_BASE_ADDR + 0x010C)                                        
#define NQAM_src_kTx_addr			(CABLE_MISC_CONFIG_BASE_ADDR + 0x0110)                                        
#define NQAM_lin_gain_mant_addr			(CABLE_MISC_CONFIG_BASE_ADDR + 0x0114)                                        
#define NQAM_lin_gain_expt_addr			(CABLE_MISC_CONFIG_BASE_ADDR + 0x0114)                                        
#define NQAM_int_gain_mant_addr			(CABLE_MISC_CONFIG_BASE_ADDR + 0x0114)                                        
#define NQAM_int_gain_expt_addr			(CABLE_MISC_CONFIG_BASE_ADDR + 0x0114)                                        
#define NQAM_timing_polarity_addr		(CABLE_MISC_CONFIG_BASE_ADDR + 0x0114)                                        
#define NQAM_timing_gain_addr			(CABLE_MISC_CONFIG_BASE_ADDR + 0x0114)                                        
#define NQAM_timing_break_addr			(CABLE_MISC_CONFIG_BASE_ADDR + 0x0114)                                        
#define NQAM_timing_clear_addr			(CABLE_MISC_CONFIG_BASE_ADDR + 0x0114)                                        
#define NQAM_timing_cond_addr			(CABLE_MISC_CONFIG_BASE_ADDR + 0x0118)                                        
#define NQAM_lpf2_freq_addr			(CABLE_MISC_CONFIG_BASE_ADDR + 0x011C)                                        
#define NQAM_lpf2_clear_addr			(CABLE_MISC_CONFIG_BASE_ADDR + 0x011C)                                        
#define NQAM_lpf2_enable_addr			(CABLE_MISC_CONFIG_BASE_ADDR + 0x011C)                                        
#define NQAM_lpf2_gain_addr			(CABLE_MISC_CONFIG_BASE_ADDR + 0x011C)                                        
#define NQAM_nyq_feed_zero_addr			(CABLE_MISC_CONFIG_BASE_ADDR + 0x011C)                                        
#define NQAM_phase_symbol_addr			(CABLE_MISC_CONFIG_BASE_ADDR + 0x011C)                                        
#define NQAM_dc_seed_addr			(CABLE_MISC_CONFIG_BASE_ADDR + 0x0120)                                        
#define NQAM_dc_active_addr			(CABLE_MISC_CONFIG_BASE_ADDR + 0x0120)                                        
#define NQAM_dc_clear_addr			(CABLE_MISC_CONFIG_BASE_ADDR + 0x0120)                                        
#define NQAM_dc_load_addr			(CABLE_MISC_CONFIG_BASE_ADDR + 0x0120)                                        
#define NQAM_dc_trace_addr			(CABLE_MISC_CONFIG_BASE_ADDR + 0x0120)                                        
#define NQAM_format_clear_addr			(CABLE_MISC_CONFIG_BASE_ADDR + 0x0124)                                        
#define NQAM_format_twos_addr			(CABLE_MISC_CONFIG_BASE_ADDR + 0x0124) 


#define NQAM_v_ts0_size			        0x02
#define NQAM_v_ts1_size			        0x02
#define NQAM_v_ts2_size			        0x02
#define NQAM_oob_sel_size			0x02
#define NQAM_FRAMING_MODE_size			0x03
#define NQAM_MCLK_MODE_size			0x01
#define NQAM_TST_SEL_size			0x04
#define NQAM_adctoqam0_size			0x02
#define NQAM_adctoqam1_size			0x02
#define NQAM_adctoqam2_size			0x02
#define NQAM_adctosync_size			0x02
#define NQAM_m_ts0_size			        0x02
#define NQAM_m_ts1_size			        0x02
#define NQAM_m_ts2_size			        0x02
#define NQAM_src_kRx_size			0x18
#define NQAM_src_kRx_receip_size		0x08
#define NQAM_src_kLO_size			0x09
#define NQAM_src_kLO_scale_size			0x02
#define NQAM_src_kLO_receip_size		0x08
#define NQAM_src_kL1_size			0x0A
#define NQAM_src_kM1_size			0x0B
#define NQAM_src_gain_size			0x0A
#define NQAM_src_track_size			0x01
#define NQAM_src_kTx_size			0x18
#define NQAM_lin_gain_mant_size			0x02
#define NQAM_lin_gain_expt_size			0x03
#define NQAM_int_gain_mant_size			0x02
#define NQAM_int_gain_expt_size			0x03
#define NQAM_timing_polarity_size		0x02
#define NQAM_timing_gain_size			0x02
#define NQAM_timing_break_size			0x01
#define NQAM_timing_clear_size			0x01
#define NQAM_timing_cond_size			0x18
#define NQAM_lpf2_freq_size			0x18
#define NQAM_lpf2_clear_size			0x01
#define NQAM_lpf2_enable_size			0x01
#define NQAM_lpf2_gain_size			0x01
#define NQAM_nyq_feed_zero_size			0x01
#define NQAM_phase_symbol_size			0x01
#define NQAM_dc_seed_size			0x0D
#define NQAM_dc_active_size			0x01
#define NQAM_dc_clear_size			0x01
#define NQAM_dc_load_size			0x01
#define NQAM_dc_trace_size			0x0D
#define NQAM_format_clear_size			0x01
#define NQAM_format_twos_size			0x01


#define NQAM_v_ts0_offset			0x00
#define NQAM_v_ts1_offset			0x02
#define NQAM_v_ts2_offset			0x04
#define NQAM_oob_sel_offset			0x06
#define NQAM_FRAMING_MODE_offset		0x00
#define NQAM_MCLK_MODE_offset			0x00
#define NQAM_TST_SEL_offset			0x00
#define NQAM_adctoqam0_offset			0x00
#define NQAM_adctoqam1_offset			0x02
#define NQAM_adctoqam2_offset			0x04
#define NQAM_adctosync_offset			0x06
#define NQAM_m_ts0_offset			0x08
#define NQAM_m_ts1_offset			0x0A
#define NQAM_m_ts2_offset			0x0C
#define NQAM_src_kRx_offset			0x00
#define NQAM_src_kRx_receip_offset		0x18
#define NQAM_src_kLO_offset			0x00
#define NQAM_src_kLO_scale_offset		0x09
#define NQAM_src_kLO_receip_offset		0x10
#define NQAM_src_kL1_offset			0x00
#define NQAM_src_kM1_offset			0x10
#define NQAM_src_gain_offset			0x00
#define NQAM_src_track_offset			0x10
#define NQAM_src_kTx_offset			0x00
#define NQAM_lin_gain_mant_offset		0x00
#define NQAM_lin_gain_expt_offset		0x02
#define NQAM_int_gain_mant_offset		0x08
#define NQAM_int_gain_expt_offset		0x0A
#define NQAM_timing_polarity_offset		0x10
#define NQAM_timing_gain_offset			0x12
#define NQAM_timing_break_offset		0x14
#define NQAM_timing_clear_offset		0x15
#define NQAM_timing_cond_offset			0x00
#define NQAM_lpf2_freq_offset			0x00
#define NQAM_lpf2_clear_offset			0x18
#define NQAM_lpf2_enable_offset			0x19
#define NQAM_lpf2_gain_offset			0x1A
#define NQAM_nyq_feed_zero_offset		0x1B
#define NQAM_phase_symbol_offset		0x1C
#define NQAM_dc_seed_offset			0x00
#define NQAM_dc_active_offset			0x0D
#define NQAM_dc_clear_offset			0x0E
#define NQAM_dc_load_offset			0x0F
#define NQAM_dc_trace_offset			0x10
#define NQAM_format_clear_offset		0x00
#define NQAM_format_twos_offset			0x01


#define NQAM_v_ts0			        NQAM_v_ts0_str, NQAM_v_ts0_addr, NQAM_v_ts0_size, NQAM_v_ts0_offset
#define NQAM_v_ts1			        NQAM_v_ts1_str, NQAM_v_ts1_addr, NQAM_v_ts1_size, NQAM_v_ts1_offset
#define NQAM_v_ts2		        	NQAM_v_ts2_str, NQAM_v_ts2_addr, NQAM_v_ts2_size, NQAM_v_ts2_offset
#define NQAM_oob_sel			 	NQAM_oob_sel_str, NQAM_oob_sel_addr, NQAM_oob_sel_size, NQAM_oob_sel_offset
#define NQAM_FRAMING_MODE			NQAM_FRAMING_MODE_str, NQAM_FRAMING_MODE_addr, NQAM_FRAMING_MODE_size, NQAM_FRAMING_MODE_offset
#define NQAM_MCLK_MODE			 	NQAM_MCLK_MODE_str, NQAM_MCLK_MODE_addr, NQAM_MCLK_MODE_size, NQAM_MCLK_MODE_offset
#define NQAM_TST_SEL			 	NQAM_TST_SEL_str, NQAM_TST_SEL_addr, NQAM_TST_SEL_size, NQAM_TST_SEL_offset
#define NQAM_adctoqam0			 	NQAM_adctoqam0_str, NQAM_adctoqam0_addr, NQAM_adctoqam0_size, NQAM_adctoqam0_offset
#define NQAM_adctoqam1			 	NQAM_adctoqam1_str, NQAM_adctoqam1_addr, NQAM_adctoqam1_size, NQAM_adctoqam1_offset
#define NQAM_adctoqam2			 	NQAM_adctoqam2_str, NQAM_adctoqam2_addr, NQAM_adctoqam2_size, NQAM_adctoqam2_offset
#define NQAM_adctosync			 	NQAM_adctosync_str, NQAM_adctosync_addr, NQAM_adctosync_size, NQAM_adctosync_offset
#define NQAM_m_ts0			        NQAM_m_ts0_str, NQAM_m_ts0_addr, NQAM_m_ts0_size, NQAM_m_ts0_offset
#define NQAM_m_ts1			        NQAM_m_ts1_str, NQAM_m_ts1_addr, NQAM_m_ts1_size, NQAM_m_ts1_offset
#define NQAM_m_ts2			        NQAM_m_ts2_str, NQAM_m_ts2_addr, NQAM_m_ts2_size, NQAM_m_ts2_offset
#define NQAM_src_kRx			 	NQAM_src_kRx_str, NQAM_src_kRx_addr, NQAM_src_kRx_size, NQAM_src_kRx_offset 
#define NQAM_src_kRx_receip			NQAM_src_kRx_receip_str, NQAM_src_kRx_receip_addr, NQAM_src_kRx_receip_size, NQAM_src_kRx_receip_offset 
#define NQAM_src_kLO			 	NQAM_src_kLO_str, NQAM_src_kLO_addr, NQAM_src_kLO_size, NQAM_src_kLO_offset 
#define NQAM_src_kLO_scale			NQAM_src_kLO_scale_str, NQAM_src_kLO_scale_addr, NQAM_src_kLO_scale_size, NQAM_src_kLO_scale_offset 
#define NQAM_src_kLO_receip			NQAM_src_kLO_receip_str, NQAM_src_kLO_receip_addr, NQAM_src_kLO_receip_size, NQAM_src_kLO_receip_offset 
#define NQAM_src_kL1			 	NQAM_src_kL1_str, NQAM_src_kL1_addr, NQAM_src_kL1_size, NQAM_src_kL1_offset 
#define NQAM_src_kM1			 	NQAM_src_kM1_str, NQAM_src_kM1_addr, NQAM_src_kM1_size, NQAM_src_kM1_offset 
#define NQAM_src_gain			 	NQAM_src_gain_str, NQAM_src_gain_addr, NQAM_src_gain_size, NQAM_src_gain_offset 
#define NQAM_src_track			 	NQAM_src_track_str, NQAM_src_track_addr, NQAM_src_track_size, NQAM_src_track_offset 
#define NQAM_src_kTx			 	NQAM_src_kTx_str, NQAM_src_kTx_addr, NQAM_src_kTx_size, NQAM_src_kTx_offset 
#define NQAM_lin_gain_mant			NQAM_lin_gain_mant_str, NQAM_lin_gain_mant_addr, NQAM_lin_gain_mant_size, NQAM_lin_gain_mant_offset 
#define NQAM_lin_gain_expt			NQAM_lin_gain_expt_str, NQAM_lin_gain_expt_addr, NQAM_lin_gain_expt_size, NQAM_lin_gain_expt_offset 
#define NQAM_int_gain_mant			NQAM_int_gain_mant_str, NQAM_int_gain_mant_addr, NQAM_int_gain_mant_size, NQAM_int_gain_mant_offset 
#define NQAM_int_gain_expt			NQAM_int_gain_expt_str, NQAM_int_gain_expt_addr, NQAM_int_gain_expt_size, NQAM_int_gain_expt_offset 
#define NQAM_timing_polarity		NQAM_timing_polarity_str, NQAM_timing_polarity_addr, NQAM_timing_polarity_size, NQAM_timing_polarity_offset 
#define NQAM_timing_gain			NQAM_timing_gain_str, NQAM_timing_gain_addr, NQAM_timing_gain_size, NQAM_timing_gain_offset 
#define NQAM_timing_break			NQAM_timing_break_str, NQAM_timing_break_addr, NQAM_timing_break_size, NQAM_timing_break_offset 
#define NQAM_timing_clear			NQAM_timing_clear_str, NQAM_timing_clear_addr, NQAM_timing_clear_size, NQAM_timing_clear_offset 
#define NQAM_timing_cond			NQAM_timing_cond_str, NQAM_timing_cond_addr, NQAM_timing_cond_size, NQAM_timing_cond_offset 
#define NQAM_lpf2_freq			 	NQAM_lpf2_freq_str, NQAM_lpf2_freq_addr, NQAM_lpf2_freq_size, NQAM_lpf2_freq_offset 
#define NQAM_lpf2_clear			 	NQAM_lpf2_clear_str, NQAM_lpf2_clear_addr, NQAM_lpf2_clear_size, NQAM_lpf2_clear_offset 
#define NQAM_lpf2_enable			NQAM_lpf2_enable_str, NQAM_lpf2_enable_addr, NQAM_lpf2_enable_size, NQAM_lpf2_enable_offset 
#define NQAM_lpf2_gain			 	NQAM_lpf2_gain_str, NQAM_lpf2_gain_addr, NQAM_lpf2_gain_size, NQAM_lpf2_gain_offset 
#define NQAM_nyq_feed_zero			NQAM_nyq_feed_zero_str, NQAM_nyq_feed_zero_addr, NQAM_nyq_feed_zero_size, NQAM_nyq_feed_zero_offset 
#define NQAM_phase_symbol			NQAM_phase_symbol_str, NQAM_phase_symbol_addr, NQAM_phase_symbol_size, NQAM_phase_symbol_offset 
#define NQAM_dc_seed			 	NQAM_dc_seed_str, NQAM_dc_seed_addr, NQAM_dc_seed_size, NQAM_dc_seed_offset 
#define NQAM_dc_active			 	NQAM_dc_active_str, NQAM_dc_active_addr, NQAM_dc_active_size, NQAM_dc_active_offset 
#define NQAM_dc_clear			 	NQAM_dc_clear_str, NQAM_dc_clear_addr, NQAM_dc_clear_size, NQAM_dc_clear_offset 
#define NQAM_dc_load			 	NQAM_dc_load_str, NQAM_dc_load_addr, NQAM_dc_load_size, NQAM_dc_load_offset 
#define NQAM_dc_trace			 	NQAM_dc_trace_str, NQAM_dc_trace_addr, NQAM_dc_trace_size, NQAM_dc_trace_offset 
#define NQAM_format_clear			NQAM_format_clear_str, NQAM_format_clear_addr, NQAM_format_clear_size, NQAM_format_clear_offset 
#define NQAM_format_twos			NQAM_format_twos_str, NQAM_format_twos_addr, NQAM_format_twos_size, NQAM_format_twos_offset 


int GetNQAMMask(int size, int offset);
void SetNQAMReg(char*toto1, int addr, int size, int offset, int value);
int GetNQAMReg(char*toto1, int addr, int size, int offset);




/*PIO base addresses*/
/*MPX mapped addresses*/
#define PIO0_address  0xa3e20000
#define PIO1_address  0xa3e21000
#define PIO2_address  0xa3e22000
#define PIO3_address  0xa3f20000
#define PIO4_address  0xa3f21000

/*actual base addresses on 498*/
/*
#define PIO0_address  0xfD220000
#define PIO1_address  0xfD221000
#define PIO2_address  0xfD222000
#define PIO3_address  0xfE220000
#define PIO4_address  0xfE221000
*/


#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif                          /* __STTUNER_CAB_DRV0498_H */


/* End of drv0498.h */

