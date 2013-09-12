/* ----------------------------------------------------------------------------
File Name: drv0297e.h

Description: 

    stv0297E demod driver internal low-level functions.

Copyright (C) 1999-2006 STMicroelectronics

---------------------------------------------------------------------------- */

/* define to prevent recursive inclusion */
#ifndef __STTUNER_CAB_DRV0297E_H
#define __STTUNER_CAB_DRV0297E_H

#ifdef __cplusplus
extern "C"
{
#endif                          /* __cplusplus */

/* includes ---------------------------------------------------------------- */

#include "sttuner.h"
#include "tcdrv.h"
	
/* Register map constants */
/*ANA_PLL_CTL2*/
#define R297e_ANA_PLL_CTL2       0x0006
#define F297e_PLL_NDIV           0x000600FF

/*MIS_ID*/
#define R297e_MIS_ID             0x0000
#define F297e_MSB_CUT            0x000000F0
#define F297e_LSB_CUT            0x0000000F

/*CTRL_0*/
#define R297e_CTRL_0             0x0002
#define F297e_SEL_ADCLK          0x000200C0
#define F297e_OSC_BYP            0x00020004
#define F297e_STANDBY            0x00020002

/*CTRL_1*/
#define R297e_CTRL_1             0x0003
#define F297e_SOFT_RST           0x00030080
#define F297e_DCREG_POFF         0x00030040
#define F297e_RS_RST             0x00030020
#define F297e_AGC_RST            0x00030010
#define F297e_CRL_RST            0x00030008
#define F297e_TRL_RST            0x00030004
#define F297e_DEINT_RST          0x00030002
#define F297e_EQU_RST            0x00030001

/*CTRL_2*/
#define R297e_CTRL_2             0x0004
#define F297e_I2CT_EN            0x00040080
#define F297e_AUXCLK_SEL         0x00040040
#define F297e_TST_FUNC           0x00040020
#define F297e_PRGCLKDIV          0x0004001E
#define F297e_M_OEN              0x00040001

/*ANA_PLL_CTL1*/
#define R297e_ANA_PLL_CTL1       0x0005
#define F297e_PLL_LOCK           0x00050080
#define F297e_SEL_TSTCLK         0x00050060
#define F297e_TST_CLKOUT_EN      0x00050010
#define F297e_PLL_BYPASS         0x00050008
#define F297e_PLL_POFF           0x00050004
#define F297e_BKP_MODE           0x00050002
#define F297e_PLL_CLKSEL         0x00050001

/*AD_CFG*/
#define R297e_AD_CFG             0x0007
#define F297e_AD_INMODE          0x00070040
#define F297e_AD_FSMIN           0x00070008
#define F297e_AD_FSMAX           0x00070004
#define F297e_AD_POFFREF         0x00070002
#define F297e_AD_POFF            0x00070001

/*IT_STATUS1*/
#define R297e_IT_STATUS1         0x0008
#define F297e_SWEEP_OUT          0x00080080
#define F297e_FSM_CRL            0x00080040
#define F297e_CRL_LOCK           0x00080020
#define F297e_MFSM               0x00080010
#define F297e_TRL_LOCK           0x00080008
#define F297e_TRL_AGC_LIMIT      0x00080004
#define F297e_ADJ_AGC_LOCK       0x00080002
#define F297e_AGC_LOCK           0x00080001

/*IT_STATUS2*/
#define R297e_IT_STATUS2         0x0009
#define F297e_TSMF_CNT           0x00090080
#define F297e_TSMF_EOF           0x00090040
#define F297e_TSMF_RDY           0x00090020
#define F297e_FEC_NOCORR         0x00090010
#define F297e_SYNCSTATE          0x00090008
#define F297e_DEINT_LOCK         0x00090004
#define F297e_FADDING_FRZ                0x00090002
#define F297e_TAPMON_ALARM               0x00090001

/*IT_EN1*/
#define R297e_IT_EN1             0x000A
#define F297e_SWEEP_OUTE         0x000A0080
#define F297e_FSM_CRLE           0x000A0040
#define F297e_CRL_LOCKE          0x000A0020
#define F297e_MFSME              0x000A0010
#define F297e_TRL_LOCKE          0x000A0008
#define F297e_TRL_AGC_LIMITE             0x000A0004
#define F297e_ADJ_AGC_LOCKE              0x000A0002
#define F297e_AGC_LOCKE          0x000A0001

/*IT_EN2*/
#define R297e_IT_EN2             0x000B
#define F297e_TSMF_CNTE          0x000B0080
#define F297e_TSMF_EOFE          0x000B0040
#define F297e_TSMF_RDYE          0x000B0020
#define F297e_FEC_NOCORRE                0x000B0010
#define F297e_SYNCSTATEE         0x000B0008
#define F297e_DEINT_LOCKE                0x000B0004
#define F297e_FADDING_FRZE               0x000B0002
#define F297e_TAPMON_ALARME              0x000B0001

/*CTRL_STATUS*/
#define R297e_CTRL_STATUS                0x000C
#define F297e_QAMFEC_LOCK                0x000C0004
#define F297e_TSMF_LOCK          0x000C0002
#define F297e_TSMF_ERROR         0x000C0001

/*LAB_DBG*/
#define R297e_LAB_DBG            0x000E
#define F297e_LAB_DEBUG          0x000E00FF

/*TEST_CTL*/
#define R297e_TEST_CTL           0x000F
#define F297e_EXT_AD             0x000F0080
#define F297e_TST_BLK_SEL                0x000F0060
#define F297e_TST_BUS_SEL                0x000F001F

/*AGC_CTL*/
#define R297e_AGC_CTL            0x0010
#define F297e_AGC_LCK_TH         0x001000F0
#define F297e_AGC_ACCUMRSTSEL            0x00100007

/*AGC_IF_CFG*/
#define R297e_AGC_IF_CFG         0x0011
#define F297e_AGC_IF_BWSEL               0x001100F0
#define F297e_AGC_IF_FREEZE              0x00110002

/*AGC_RF_CFG*/
#define R297e_AGC_RF_CFG         0x0012
#define F297e_AGC_RF_BWSEL               0x00120070
#define F297e_AGC_RF_FREEZE              0x00120002

/*AGC_PWM_CFG*/
#define R297e_AGC_PWM_CFG                0x0013
#define F297e_AGC_RF_PWM_TST             0x00130080
#define F297e_AGC_RF_PWM_INV             0x00130040
#define F297e_AGC_IF_PWM_TST             0x00130008
#define F297e_AGC_IF_PWM_INV             0x00130004
#define F297e_AGC_PWM_CLKDIV             0x00130003

/*AGC_PWR_REF_L*/
#define R297e_AGC_PWR_REF_L              0x0014
#define F297e_AGC_PWRREF_LO              0x001400FF

/*AGC_PWR_REF_H*/
#define R297e_AGC_PWR_REF_H              0x0015
#define F297e_AGC_PWRREF_HI              0x00150003

/*AGC_RF_TH_L*/
#define R297e_AGC_RF_TH_L                0x0016
#define F297e_AGC_RF_TH_LO               0x001600FF

/*AGC_RF_TH_H*/
#define R297e_AGC_RF_TH_H                0x0017
#define F297e_AGC_RF_TH_HI               0x0017000F

/*AGC_IF_LTH_L*/
#define R297e_AGC_IF_LTH_L               0x0018
#define F297e_AGC_IF_THLO_LO             0x001800FF

/*AGC_IF_LTH_H*/
#define R297e_AGC_IF_LTH_H               0x0019
#define F297e_AGC_IF_THLO_HI             0x0019000F

/*AGC_IF_HTH_L*/
#define R297e_AGC_IF_HTH_L               0x001A
#define F297e_AGC_IF_THHI_LO             0x001A00FF

/*AGC_IF_HTH_H*/
#define R297e_AGC_IF_HTH_H               0x001B
#define F297e_AGC_IF_THHI_HI             0x001B000F

/*AGC_PWR_RD_L*/
#define R297e_AGC_PWR_RD_L               0x001C
#define F297e_AGC_PWR_WORD_LO            0x001C00FF

/*AGC_PWR_RD_M*/
#define R297e_AGC_PWR_RD_M               0x001D
#define F297e_AGC_PWR_WORD_ME            0x001D00FF

/*AGC_PWR_RD_H*/
#define R297e_AGC_PWR_RD_H               0x001E
#define F297e_AGC_PWR_WORD_HI            0x001E0003

/*AGC_PWM_IFCMD_L*/
#define R297e_AGC_PWM_IFCMD_L            0x0020
#define F297e_AGC_IF_PWMCMD_LO           0x002000FF

/*AGC_PWM_IFCMD_H*/
#define R297e_AGC_PWM_IFCMD_H            0x0021
#define F297e_AGC_IF_PWMCMD_HI           0x0021000F

/*AGC_PWM_RFCMD_L*/
#define R297e_AGC_PWM_RFCMD_L            0x0022
#define F297e_AGC_RF_PWMCMD_LO           0x002200FF

/*AGC_PWM_RFCMD_H*/
#define R297e_AGC_PWM_RFCMD_H            0x0023
#define F297e_AGC_RF_PWMCMD_HI           0x0023000F

/*IQDEM_CFG*/
#define R297e_IQDEM_CFG          0x0024
#define F297e_IQDEM_CLK_SEL              0x00240004
#define F297e_IQDEM_INVIQ                0x00240002
#define F297e_IQDEM_A2DTYPE              0x00240001

/*MIX_NCO_LL*/
#define R297e_MIX_NCO_LL         0x0025
#define F297e_MIX_NCO_INC_LL             0x002500FF

/*MIX_NCO_HL*/
#define R297e_MIX_NCO_HL         0x0026
#define F297e_MIX_NCO_INC_HL             0x002600FF

/*MIX_NCO_HH*/
#define R297e_MIX_NCO_HH         0x0027
#define F297e_MIX_NCO_INVCNST            0x00270080
#define F297e_MIX_NCO_INC_HH             0x0027007F

/*SRC_NCO_LL*/
#define R297e_SRC_NCO_LL         0x0028
#define F297e_SRC_NCO_INC_LL             0x002800FF

/*SRC_NCO_LH*/
#define R297e_SRC_NCO_LH         0x0029
#define F297e_SRC_NCO_INC_LH             0x002900FF

/*SRC_NCO_HL*/
#define R297e_SRC_NCO_HL         0x002A
#define F297e_SRC_NCO_INC_HL             0x002A00FF

/*SRC_NCO_HH*/
#define R297e_SRC_NCO_HH         0x002B
#define F297e_SRC_NCO_INC_HH             0x002B007F

/*IQDEM_GAIN_SRC_L*/
#define R297e_IQDEM_GAIN_SRC_L           0x002C
#define F297e_GAIN_SRC_LO                0x002C00FF

/*IQDEM_GAIN_SRC_H*/
#define R297e_IQDEM_GAIN_SRC_H           0x002D
#define F297e_GAIN_SRC_HI                0x002D0003

/*IQDEM_DCRM_CFG_LL*/
#define R297e_IQDEM_DCRM_CFG_LL          0x0030
#define F297e_DCRM0_DCIN_L               0x003000FF

/*IQDEM_DCRM_CFG_LH*/
#define R297e_IQDEM_DCRM_CFG_LH          0x0031
#define F297e_DCRM1_I_DCIN_L             0x003100FC
#define F297e_DCRM0_DCIN_H               0x00310003

/*IQDEM_DCRM_CFG_HL*/
#define R297e_IQDEM_DCRM_CFG_HL          0x0032
#define F297e_DCRM1_Q_DCIN_L             0x003200F0
#define F297e_DCRM1_I_DCIN_H             0x0032000F

/*IQDEM_DCRM_CFG_HH*/
#define R297e_IQDEM_DCRM_CFG_HH          0x0033
#define F297e_DCRM1_FRZ          0x00330080
#define F297e_DCRM0_FRZ          0x00330040
#define F297e_DCRM1_Q_DCIN_H             0x0033003F

/*IQDEM_ADJ_COEFF0*/
#define R297e_IQDEM_ADJ_COEFF0           0x0034
#define F297e_ADJIIR_COEFF10_L           0x003400FF

/*IQDEM_ADJ_COEFF1*/
#define R297e_IQDEM_ADJ_COEFF1           0x0035
#define F297e_ADJIIR_COEFF11_L           0x003500FC
#define F297e_ADJIIR_COEFF10_H           0x00350003

/*IQDEM_ADJ_COEFF2*/
#define R297e_IQDEM_ADJ_COEFF2           0x0036
#define F297e_ADJIIR_COEFF12_L           0x003600F0
#define F297e_ADJIIR_COEFF11_H           0x0036000F

/*IQDEM_ADJ_COEFF3*/
#define R297e_IQDEM_ADJ_COEFF3           0x0037
#define F297e_ADJIIR_COEFF20_L           0x003700C0
#define F297e_ADJIIR_COEFF12_H           0x0037003F

/*IQDEM_ADJ_COEFF4*/
#define R297e_IQDEM_ADJ_COEFF4           0x0038
#define F297e_ADJIIR_COEFF20_H           0x003800FF

/*IQDEM_ADJ_COEFF5*/
#define R297e_IQDEM_ADJ_COEFF5           0x0039
#define F297e_ADJIIR_COEFF21_L           0x003900FF

/*IQDEM_ADJ_COEFF6*/
#define R297e_IQDEM_ADJ_COEFF6           0x003A
#define F297e_ADJIIR_COEFF22_L           0x003A00FC
#define F297e_ADJIIR_COEFF21_H           0x003A0003

/*IQDEM_ADJ_COEFF7*/
#define R297e_IQDEM_ADJ_COEFF7           0x003B
#define F297e_ADJIIR_COEFF22_H           0x003B000F

/*IQDEM_ADJ_EN*/
#define R297e_IQDEM_ADJ_EN               0x003C
#define F297e_ALLPASSFILT_EN             0x003C0008
#define F297e_ADJ_AGC_EN         0x003C0004
#define F297e_ADJ_COEFF_FRZ              0x003C0002
#define F297e_ADJ_EN             0x003C0001

/*IQDEM_ADJ_AGC_REF*/
#define R297e_IQDEM_ADJ_AGC_REF          0x003D
#define F297e_ADJ_AGC_REF                0x003D00FF

/*ALLPASSFILT1_LO*/
#define R297e_ALLPASSFILT1_LO            0x0040
#define F297e_ALLPASSFILT_COEFF1_LO              0x004000FF

/*ALLPASSFILT1_ME*/
#define R297e_ALLPASSFILT1_ME            0x0041
#define F297e_ALLPASSFILT_COEFF1_ME              0x004100FF

/*ALLPASSFILT1_HI*/
#define R297e_ALLPASSFILT1_HI            0x0042
#define F297e_ALLPASSFILT_COEFF1_HI              0x004200FF

/*ALLPASSFILT2_LO*/
#define R297e_ALLPASSFILT2_LO            0x0043
#define F297e_ALLPASSFILT_COEFF2_LO              0x004300FF

/*ALLPASSFILT2_ME*/
#define R297e_ALLPASSFILT2_ME            0x0044
#define F297e_ALLPASSFILT_COEFF2_ME              0x004400FF

/*ALLPASSFILT2_HI*/
#define R297e_ALLPASSFILT2_HI            0x0045
#define F297e_ALLPASSFILT_COEFF2_HI              0x004500FF

/*ALLPASSFILT3_LO*/
#define R297e_ALLPASSFILT3_LO            0x0046
#define F297e_ALLPASSFILT_COEFF3_LO              0x004600FC
#define F297e_ALLPASSFILT_COEFF2_HI2             0x00460003

/*ALLPASSFILT3_ME*/
#define R297e_ALLPASSFILT3_ME            0x0047
#define F297e_ALLPASSFILT_COEFF3_ME              0x004700FF

/*ALLPASSFILT3_HI*/
#define R297e_ALLPASSFILT3_HI            0x0048
#define F297e_ALLPASSFILT_COEFF3_HI              0x004800FF

/*ALLPASSFILT4_LO*/
#define R297e_ALLPASSFILT4_LO            0x0049
#define F297e_ALLPASSFILT_COEFF4_LO              0x004900F0
#define F297e_ALLPASSFILT_COEFF3_HI2             0x0049000F

/*ALLPASSFILT4_ME*/
#define R297e_ALLPASSFILT4_ME            0x004A
#define F297e_ALLPASSFILT_COEFF4_ME              0x004A00FF

/*ALLPASSFILT4_HI*/
#define R297e_ALLPASSFILT4_HI            0x004B
#define F297e_ALLPASSFILT_COEFF4_HI              0x004B00FF

/*ALLPASSFILT5_LO*/
#define R297e_ALLPASSFILT5_LO            0x004C
#define F297e_ALLPASSFILT_COEFF5_LO              0x004C00C0
#define F297e_ALLPASSFILT_COEFF4_HI2             0x004C003F

/*ALLPASSFILT5_ME*/
#define R297e_ALLPASSFILT5_ME            0x004D
#define F297e_ALLPASSFILT_COEFF5_ME              0x004D00FF

/*ALLPASSFILT5_HI*/
#define R297e_ALLPASSFILT5_HI            0x004E
#define F297e_ALLPASSFILT_COEFF5_HI              0x004E00FF

/*ALLPASSFILT5_HI2*/
#define R297e_ALLPASSFILT5_HI2           0x004F
#define F297e_ALLPASSFILT_COEFF5_HI2             0x004F00FF

/*ALLPASSFILT6_LO*/
#define R297e_ALLPASSFILT6_LO            0x0050
#define F297e_ALLPASSFILT_COEFF6_LO              0x005000FF

/*ALLPASSFILT6_ME*/
#define R297e_ALLPASSFILT6_ME            0x0051
#define F297e_ALLPASSFILT_COEFF6_ME              0x005100FF

/*ALLPASSFILT6_HI*/
#define R297e_ALLPASSFILT6_HI            0x0052
#define F297e_ALLPASSFILT_COEFF6_HI              0x005200FF

/*ALLPASSFILT7_LO*/
#define R297e_ALLPASSFILT7_LO            0x0053
#define F297e_ALLPASSFILT_COEFF7_LO              0x005300FF

/*ALLPASSFILT7_ME*/
#define R297e_ALLPASSFILT7_ME            0x0054
#define F297e_ALLPASSFILT_COEFF7_ME              0x005400FF

/*ALLPASSFILT7_HI*/
#define R297e_ALLPASSFILT7_HI            0x0055
#define F297e_ALLPASSFILT_COEFF7_HI              0x005500FF

/*ALLPASSFILT8_LO*/
#define R297e_ALLPASSFILT8_LO            0x0056
#define F297e_ALLPASSFILT_COEFF8_LO              0x005600FF

/*ALLPASSFILT8_ME*/
#define R297e_ALLPASSFILT8_ME            0x0057
#define F297e_ALLPASSFILT_COEFF8_ME              0x005700FF

/*ALLPASSFILT8_HI*/
#define R297e_ALLPASSFILT8_HI            0x0058
#define F297e_ALLPASSFILT_COEFF8_HI              0x005800FF

/*TRL_AGC_CFG*/
#define R297e_TRL_AGC_CFG                0x0064
#define F297e_TRL_AGC_FREEZE             0x00640080
#define F297e_TRL_AGC_REF                0x0064007F

/*TRL_LPF_CFG*/
#define R297e_TRL_LPF_CFG                0x0070
#define F297e_NYQPOINT_INV               0x00700040
#define F297e_TRL_SHIFT          0x00700030
#define F297e_NYQ_COEFF_SEL              0x0070000C
#define F297e_TRL_LPF_FREEZE             0x00700002
#define F297e_TRL_LPF_CRT                0x00700001

/*TRL_LPF_ACQ_GAIN*/
#define R297e_TRL_LPF_ACQ_GAIN           0x0071
#define F297e_TRL_GDIR_ACQ               0x00710070
#define F297e_TRL_GINT_ACQ               0x00710007

/*TRL_LPF_TRK_GAIN*/
#define R297e_TRL_LPF_TRK_GAIN           0x0072
#define F297e_TRL_GDIR_TRK               0x00720070
#define F297e_TRL_GINT_TRK               0x00720007

/*TRL_LPF_OUT_GAIN*/
#define R297e_TRL_LPF_OUT_GAIN           0x0073
#define F297e_TRL_GAIN_OUT               0x00730007

/*TRL_LOCKDET_LTH*/
#define R297e_TRL_LOCKDET_LTH            0x0074
#define F297e_TRL_LCK_THLO               0x00740007

/*TRL_LOCKDET_HTH*/
#define R297e_TRL_LOCKDET_HTH            0x0075
#define F297e_TRL_LCK_THHI               0x007500FF

/*TRL_LOCKDET_TRGVAL*/
#define R297e_TRL_LOCKDET_TRGVAL         0x0076
#define F297e_TRL_LCK_TRG                0x007600FF

/*FSM_STATE*/
#define R297e_FSM_STATE          0x0080
#define F297e_CRL_DFE            0x00800080
#define F297e_DFE_START          0x00800040
#define F297e_CTRLG_START                0x00800030
#define F297e_FSM_FORCESTATE             0x0080000F

/*FSM_CTL*/
#define R297e_FSM_CTL            0x0081
#define F297e_FEC2_EN            0x00810040
#define F297e_SIT_EN             0x00810020
#define F297e_TRL_AHEAD          0x00810010
#define F297e_TRL2_EN            0x00810008
#define F297e_FSM_EQA1_EN                0x00810004
#define F297e_FSM_BKP_DIS                0x00810002
#define F297e_FSM_FORCE_EN               0x00810001

/*FSM_STS*/
#define R297e_FSM_STS            0x0082
#define F297e_FSM_STATUS         0x0082000F

/*FSM_SNR0_HTH*/
#define R297e_FSM_SNR0_HTH               0x0083
#define F297e_SNR0_HTH           0x008300FF

/*FSM_SNR1_HTH*/
#define R297e_FSM_SNR1_HTH               0x0084
#define F297e_SNR1_HTH           0x008400FF

/*FSM_SNR2_HTH*/
#define R297e_FSM_SNR2_HTH               0x0085
#define F297e_SNR2_HTH           0x008500FF

/*FSM_SNR0_LTH*/
#define R297e_FSM_SNR0_LTH               0x0086
#define F297e_SNR0_LTH           0x008600FF

/*FSM_SNR1_LTH*/
#define R297e_FSM_SNR1_LTH               0x0087
#define F297e_SNR1_LTH           0x008700FF

/*FSM_EQA1_HTH*/
#define R297e_FSM_EQA1_HTH               0x0088
#define F297e_SNR3_HTH           0x008800F0
#define F297e_EQA1_HTH           0x0088000F

/*FSM_TEMPO*/
#define R297e_FSM_TEMPO          0x0089
#define F297e_SIT                0x00890060
#define F297e_WST                0x0089001C
#define F297e_ELT                0x00890003

/*EQU_I_TESTTAP_L*/
#define R297e_EQU_I_TESTTAP_L            0x0094
#define F297e_I_TEST_TAP_L               0x009400FF

/*EQU_I_TESTTAP_M*/
#define R297e_EQU_I_TESTTAP_M            0x0095
#define F297e_I_TEST_TAP_M               0x009500FF

/*EQU_I_TESTTAP_H*/
#define R297e_EQU_I_TESTTAP_H            0x0096
#define F297e_I_TEST_TAP_H               0x0096001F

/*EQU_TESTAP_CFG*/
#define R297e_EQU_TESTAP_CFG             0x0097
#define F297e_TEST_FFE_DFE_SEL           0x00970040
#define F297e_TEST_TAP_SELECT            0x0097003F

/*EQU_Q_TESTTAP_L*/
#define R297e_EQU_Q_TESTTAP_L            0x0098
#define F297e_Q_TEST_TAP_L               0x009800FF

/*EQU_Q_TESTTAP_M*/
#define R297e_EQU_Q_TESTTAP_M            0x0099
#define F297e_Q_TEST_TAP_M               0x009900FF

/*EQU_Q_TESTTAP_H*/
#define R297e_EQU_Q_TESTTAP_H            0x009A
#define F297e_Q_TEST_TAP_H               0x009A001F

/*EQU_TAP_CTRL*/
#define R297e_EQU_TAP_CTRL               0x009B
#define F297e_MTAP_FRZ           0x009B0010
#define F297e_PRE_FREEZE         0x009B0008
#define F297e_DFE_TAPMON_EN              0x009B0004
#define F297e_FFE_TAPMON_EN              0x009B0002
#define F297e_MTAP_ONLY          0x009B0001

/*EQU_CTR_CRL_CONTROL_L*/
#define R297e_EQU_CTR_CRL_CONTROL_L              0x009C
#define F297e_EQU_CTR_CRL_CONTROL_LO             0x009C00FF

/*EQU_CTR_CRL_CONTROL_H*/
#define R297e_EQU_CTR_CRL_CONTROL_H              0x009D
#define F297e_EQU_CTR_CRL_CONTROL_HI             0x009D00FF

/*EQU_CTR_HIPOW_L*/
#define R297e_EQU_CTR_HIPOW_L            0x009E
#define F297e_CTR_HIPOW_L                0x009E00FF

/*EQU_CTR_HIPOW_H*/
#define R297e_EQU_CTR_HIPOW_H            0x009F
#define F297e_CTR_HIPOW_H                0x009F00FF

/*EQU_I_EQU_LO*/
#define R297e_EQU_I_EQU_LO               0x00A0
#define F297e_EQU_I_EQU_L                0x00A000FF

/*EQU_I_EQU_HI*/
#define R297e_EQU_I_EQU_HI               0x00A1
#define F297e_EQU_I_EQU_H                0x00A10003

/*EQU_Q_EQU_LO*/
#define R297e_EQU_Q_EQU_LO               0x00A2
#define F297e_EQU_Q_EQU_L                0x00A200FF

/*EQU_Q_EQU_HI*/
#define R297e_EQU_Q_EQU_HI               0x00A3
#define F297e_EQU_Q_EQU_H                0x00A30003

/*EQU_MAPPER*/
#define R297e_EQU_MAPPER         0x00A4
#define F297e_QUAD_AUTO          0x00A40080
#define F297e_QUAD_INV           0x00A40040
#define F297e_QAM_MODE           0x00A40007

/*EQU_SWEEP_RATE*/
#define R297e_EQU_SWEEP_RATE             0x00A5
#define F297e_SNR_PER            0x00A500C0
#define F297e_SWEEP_RATE         0x00A5003F

/*EQU_SNR_LO*/
#define R297e_EQU_SNR_LO         0x00A6
#define F297e_SNR_LO             0x00A600FF

/*EQU_SNR_HI*/
#define R297e_EQU_SNR_HI         0x00A7
#define F297e_SNR_HI             0x00A700FF

/*EQU_GAMMA_LO*/
#define R297e_EQU_GAMMA_LO               0x00A8
#define F297e_GAMMA_LO           0x00A800FF

/*EQU_GAMMA_HI*/
#define R297e_EQU_GAMMA_HI               0x00A9
#define F297e_GAMMA_ME           0x00A900FF

/*EQU_ERR_GAIN*/
#define R297e_EQU_ERR_GAIN               0x00AA
#define F297e_EQA1MU             0x00AA0070
#define F297e_CRL2MU             0x00AA000E
#define F297e_GAMMA_HI           0x00AA0001

/*EQU_RADIUS*/
#define R297e_EQU_RADIUS         0x00AB
#define F297e_RADIUS             0x00AB00FF

/*EQU_FFE_MAINTAP*/
#define R297e_EQU_FFE_MAINTAP            0x00AC
#define F297e_FFE_MAINTAP_INIT           0x00AC00FF

/*EQU_FFE_LEAKAGE*/
#define R297e_EQU_FFE_LEAKAGE            0x00AE
#define F297e_LEAK_PER           0x00AE00F0
#define F297e_EQU_OUTSEL         0x00AE0002
#define F297e_PNT2DFE            0x00AE0001

/*EQU_FFE_MAINTAP_POS*/
#define R297e_EQU_FFE_MAINTAP_POS                0x00AF
#define F297e_FFE_LEAK_EN                0x00AF0080
#define F297e_DFE_LEAK_EN                0x00AF0040
#define F297e_FFE_MAINTAP_POS            0x00AF003F

/*EQU_GAIN_WIDE*/
#define R297e_EQU_GAIN_WIDE              0x00B0
#define F297e_DFE_GAIN_WIDE              0x00B000F0
#define F297e_FFE_GAIN_WIDE              0x00B0000F

/*EQU_GAIN_NARROW*/
#define R297e_EQU_GAIN_NARROW            0x00B1
#define F297e_DFE_GAIN_NARROW            0x00B100F0
#define F297e_FFE_GAIN_NARROW            0x00B1000F

/*EQU_CTR_LPF_GAIN*/
#define R297e_EQU_CTR_LPF_GAIN           0x00B2
#define F297e_CTR_GTO            0x00B20080
#define F297e_CTR_GDIR           0x00B20070
#define F297e_SWEEP_EN           0x00B20008
#define F297e_CTR_GINT           0x00B20007

/*EQU_CRL_LPF_GAIN*/
#define R297e_EQU_CRL_LPF_GAIN           0x00B3
#define F297e_CRL_GTO            0x00B30080
#define F297e_CRL_GDIR           0x00B30070
#define F297e_SWEEP_DIR          0x00B30008
#define F297e_CRL_GINT           0x00B30007

/*EQU_GLOBAL_GAIN*/
#define R297e_EQU_GLOBAL_GAIN            0x00B4
#define F297e_CRL_GAIN           0x00B400F8
#define F297e_CTR_INC_GAIN               0x00B40004
#define F297e_CTR_FRAC           0x00B40003

/*EQU_CRL_LD_SEN*/
#define R297e_EQU_CRL_LD_SEN             0x00B5
#define F297e_CTR_BADPOINT_EN            0x00B50080
#define F297e_CTR_GAIN           0x00B50070
#define F297e_LIMANEN            0x00B50008
#define F297e_CRL_LD_SEN         0x00B50007

/*EQU_CRL_LD_VAL*/
#define R297e_EQU_CRL_LD_VAL             0x00B6
#define F297e_CRL_BISTH_LIMIT            0x00B60080
#define F297e_CARE_EN            0x00B60040
#define F297e_CRL_LD_PER         0x00B60030
#define F297e_CRL_LD_WST         0x00B6000C
#define F297e_CRL_LD_TFS         0x00B60003

/*EQU_CRL_TFR*/
#define R297e_EQU_CRL_TFR                0x00B7
#define F297e_CRL_LD_TFR         0x00B700FF

/*EQU_CRL_BISTH_LO*/
#define R297e_EQU_CRL_BISTH_LO           0x00B8
#define F297e_CRL_BISTH_LO               0x00B800FF

/*EQU_CRL_BISTH_HI*/
#define R297e_EQU_CRL_BISTH_HI           0x00B9
#define F297e_CRL_BISTH_HI               0x00B900FF

/*EQU_SWEEP_RANGE_LO*/
#define R297e_EQU_SWEEP_RANGE_LO         0x00BA
#define F297e_SWEEP_RANGE_LO             0x00BA00FF

/*EQU_SWEEP_RANGE_HI*/
#define R297e_EQU_SWEEP_RANGE_HI         0x00BB
#define F297e_SWEEP_RANGE_HI             0x00BB00FF

/*EQU_CRL_LIMITER*/
#define R297e_EQU_CRL_LIMITER            0x00BC
#define F297e_BISECTOR_EN                0x00BC0080
#define F297e_PHEST128_EN                0x00BC0040
#define F297e_CRL_LIM            0x00BC003F

/*EQU_MODULUS_MAP*/
#define R297e_EQU_MODULUS_MAP            0x00BD
#define F297e_PNT_DEPTH          0x00BD00E0
#define F297e_MODULUS_CMP                0x00BD001F

/*EQU_PNT_GAIN*/
#define R297e_EQU_PNT_GAIN               0x00BE
#define F297e_PNT_EN             0x00BE0080
#define F297e_MODULUSMAP_EN              0x00BE0040
#define F297e_PNT_GAIN           0x00BE003F

/*GPIO_0_CFG*/
#define R297e_GPIO_0_CFG         0x00C0
#define F297e_GPIO0_OD           0x00C00080
#define F297e_GPIO0_RD_VAL               0x00C00040
#define F297e_GPIO0_SEL          0x00C0003E
#define F297e_GPIO0_INV          0x00C00001

/*GPIO_1_CFG*/
#define R297e_GPIO_1_CFG         0x00C1
#define F297e_GPIO1_OD           0x00C10080
#define F297e_GPIO1_RD_VAL               0x00C10040
#define F297e_GPIO1_SEL          0x00C1003E
#define F297e_GPIO1_INV          0x00C10001

/*GPIO_2_CFG*/
#define R297e_GPIO_2_CFG         0x00C2
#define F297e_GPIO2_OD           0x00C20080
#define F297e_GPIO2_RD_VAL               0x00C20040
#define F297e_GPIO2_SEL          0x00C2003E
#define F297e_GPIO2_INV          0x00C20001

/*GPIO_3_CFG*/
#define R297e_GPIO_3_CFG         0x00C3
#define F297e_GPIO3_OD           0x00C30080
#define F297e_GPIO3_RD_VAL               0x00C30040
#define F297e_GPIO3_SEL          0x00C3003E
#define F297e_GPIO3_INV          0x00C30001

/*GPIO_4_CFG*/
#define R297e_GPIO_4_CFG         0x00C4
#define F297e_GPIO4_OD           0x00C40080
#define F297e_GPIO4_RD_VAL               0x00C40040
#define F297e_GPIO4_SEL          0x00C4003E
#define F297e_GPIO4_INV          0x00C40001

/*GPIO_5_CFG*/
#define R297e_GPIO_5_CFG         0x00C5
#define F297e_GPIO5_OD           0x00C50080
#define F297e_GPIO5_RD_VAL               0x00C50040
#define F297e_GPIO5_SEL          0x00C5003E
#define F297e_GPIO5_INV          0x00C50001

/*GPIO_6_CFG*/
#define R297e_GPIO_6_CFG         0x00C6
#define F297e_GPIO6_OD           0x00C60080
#define F297e_GPIO6_RD_VAL               0x00C60040
#define F297e_GPIO6_SEL          0x00C6003E
#define F297e_GPIO6_INV          0x00C60001

/*GPIO_7_CFG*/
#define R297e_GPIO_7_CFG         0x00C7
#define F297e_GPIO7_OD           0x00C70080
#define F297e_GPIO7_RD_VAL               0x00C70040
#define F297e_GPIO7_SEL          0x00C7003E
#define F297e_GPIO7_INV          0x00C70001

/*GPIO_8_CFG*/
#define R297e_GPIO_8_CFG         0x00C8
#define F297e_GPIO8_OD           0x00C80080
#define F297e_GPIO8_RD_VAL               0x00C80040
#define F297e_GPIO8_SEL          0x00C8003E
#define F297e_GPIO8_INV          0x00C80001

/*GPIO_9_CFG*/
#define R297e_GPIO_9_CFG         0x00C9
#define F297e_GPIO9_OD           0x00C90080
#define F297e_GPIO9_RD_VAL               0x00C90040
#define F297e_GPIO9_SEL          0x00C9003E
#define F297e_GPIO9_INV          0x00C90001

/*FEC_AC_CTR_0*/
#define R297e_FEC_AC_CTR_0               0x00D8
#define F297e_BE_BYPASS          0x00D80020
#define F297e_REFRESH47          0x00D80010
#define F297e_CT_NBST            0x00D80008
#define F297e_TEI_ENA            0x00D80004
#define F297e_DS_ENA             0x00D80002
#define F297e_TSMF_EN            0x00D80001

/*FEC_AC_CTR_1*/
#define R297e_FEC_AC_CTR_1               0x00D9
#define F297e_DEINT_DEPTH                0x00D900FF

/*FEC_AC_CTR_2*/
#define R297e_FEC_AC_CTR_2               0x00DA
#define F297e_DEINT_M            0x00DA00F8
#define F297e_DIS_UNLOCK         0x00DA0004
#define F297e_DESCR_MODE         0x00DA0003

/*FEC_AC_CTR_3*/
#define R297e_FEC_AC_CTR_3               0x00DB
#define F297e_DI_UNLOCK          0x00DB0080
#define F297e_DI_FREEZE          0x00DB0040
#define F297e_MISMATCH           0x00DB0030
#define F297e_ACQ_MODE           0x00DB000C
#define F297e_TRK_MODE           0x00DB0003

/*FEC_STATUS*/
#define R297e_FEC_STATUS         0x00DC
#define F297e_DEINT_SMCNTR               0x00DC00E0
#define F297e_DEINT_SYNCSTATE            0x00DC0018
#define F297e_DEINT_SYNLOST              0x00DC0004
#define F297e_DESCR_SYNCSTATE            0x00DC0002

/*RS_COUNTER_0*/
#define R297e_RS_COUNTER_0               0x00DE
#define F297e_BK_CT_L            0x00DE00FF

/*RS_COUNTER_1*/
#define R297e_RS_COUNTER_1               0x00DF
#define F297e_BK_CT_H            0x00DF00FF

/*RS_COUNTER_2*/
#define R297e_RS_COUNTER_2               0x00E0
#define F297e_CORR_CT_L          0x00E000FF

/*RS_COUNTER_3*/
#define R297e_RS_COUNTER_3               0x00E1
#define F297e_CORR_CT_H          0x00E100FF

/*RS_COUNTER_4*/
#define R297e_RS_COUNTER_4               0x00E2
#define F297e_UNCORR_CT_L                0x00E200FF

/*RS_COUNTER_5*/
#define R297e_RS_COUNTER_5               0x00E3
#define F297e_UNCORR_CT_H                0x00E300FF

/*BERT_0*/
#define R297e_BERT_0             0x00E4
#define F297e_RS_NOCORR          0x00E40004
#define F297e_CT_HOLD            0x00E40002
#define F297e_CT_CLEAR           0x00E40001

/*BERT_1*/
#define R297e_BERT_1             0x00E5
#define F297e_BERT_ON            0x00E50020
#define F297e_BERT_ERR_SRC               0x00E50010
#define F297e_BERT_ERR_MODE              0x00E50008
#define F297e_BERT_NBYTE         0x00E50007

/*BERT_2*/
#define R297e_BERT_2             0x00E6
#define F297e_BERT_ERRCOUNT_L            0x00E600FF

/*BERT_3*/
#define R297e_BERT_3             0x00E7
#define F297e_BERT_ERRCOUNT_H            0x00E700FF

/*OUTFORMAT_0*/
#define R297e_OUTFORMAT_0                0x00E8
#define F297e_CLK_POLARITY               0x00E80080
#define F297e_FEC_TYPE           0x00E80040
#define F297e_SYNC_STRIP         0x00E80008
#define F297e_TS_SWAP            0x00E80004
#define F297e_OUTFORMAT          0x00E80003

/*OUTFORMAT_1*/
#define R297e_OUTFORMAT_1                0x00E9
#define F297e_CI_DIVRANGE                0x00E900FF

/*OUTFORMAT_2*/
#define R297e_OUTFORMAT_2                0x00EA
#define F297e_SMOOTH_BYPASS              0x00EA0008
#define F297e_SEL_FEC_MUX                0x00EA0007

/*TSMF_CTRL_0*/
#define R297e_TSMF_CTRL_0                0x00EC
#define F297e_TS_NUMBER          0x00EC001E
#define F297e_SEL_MODE           0x00EC0001

/*TSMF_CTRL_1*/
#define R297e_TSMF_CTRL_1                0x00ED
#define F297e_CHECK_ERROR_BIT            0x00ED0080
#define F297e_CHCK_F_SYNC                0x00ED0040
#define F297e_H_MODE             0x00ED0008
#define F297e_D_V_MODE           0x00ED0004
#define F297e_MODE               0x00ED0003

/*TSMF_CTRL_3*/
#define R297e_TSMF_CTRL_3                0x00EF
#define F297e_SYNC_IN_COUNT              0x00EF00F0
#define F297e_SYNC_OUT_COUNT             0x00EF000F

/*TS_ON_ID_0*/
#define R297e_TS_ON_ID_0         0x00F0
#define F297e_TS_ID_L            0x00F000FF

/*TS_ON_ID_1*/
#define R297e_TS_ON_ID_1         0x00F1
#define F297e_TS_ID_H            0x00F100FF

/*TS_ON_ID_2*/
#define R297e_TS_ON_ID_2         0x00F2
#define F297e_ON_ID_L            0x00F200FF

/*TS_ON_ID_3*/
#define R297e_TS_ON_ID_3         0x00F3
#define F297e_ON_ID_H            0x00F300FF

/*RE_STATUS_0*/
#define R297e_RE_STATUS_0                0x00F4
#define F297e_RECEIVE_STATUS_L           0x00F400FF

/*RE_STATUS_1*/
#define R297e_RE_STATUS_1                0x00F5
#define F297e_RECEIVE_STATUS_LH          0x00F500FF

/*RE_STATUS_2*/
#define R297e_RE_STATUS_2                0x00F6
#define F297e_RECEIVE_STATUS_HL          0x00F600FF

/*RE_STATUS_3*/
#define R297e_RE_STATUS_3                0x00F7
#define F297e_RECEIVE_STATUS_HH          0x00F7003F

/*TS_STATUS_0*/
#define R297e_TS_STATUS_0                0x00F8
#define F297e_TS_STATUS_L                0x00F800FF

/*TS_STATUS_1*/
#define R297e_TS_STATUS_1                0x00F9
#define F297e_TS_STATUS_H                0x00F9007F

/*TS_STATUS_2*/
#define R297e_TS_STATUS_2                0x00FA
#define F297e_ERROR              0x00FA0080
#define F297e_EMERGENCY          0x00FA0040
#define F297e_CRE_TS             0x00FA0030
#define F297e_VER                0x00FA000E
#define F297e_M_LOCK             0x00FA0001

/*TS_STATUS_3*/
#define R297e_TS_STATUS_3                0x00FB
#define F297e_UPDATE_READY               0x00FB0080
#define F297e_END_FRAME_HEADER           0x00FB0040
#define F297e_CONTCNT            0x00FB0020
#define F297e_TS_IDENTIFIER_SEL          0x00FB000F

/*T_O_ID_0*/
#define R297e_T_O_ID_0           0x00FC
#define F297e_TS_ID_I_L          0x00FC00FF

/*T_O_ID_1*/
#define R297e_T_O_ID_1           0x00FD
#define F297e_TS_ID_I_H          0x00FD00FF

/*T_O_ID_2*/
#define R297e_T_O_ID_2           0x00FE
#define F297e_ON_ID_I_L          0x00FE00FF

/*T_O_ID_3*/
#define R297e_T_O_ID_3           0x00FF
#define F297e_ON_ID_I_H          0x00FF00FF

/*RESERVED*/
#define R297e_RESERVED           0x006D
#define F297e_RESERVED           0x006D00FF


	/* Number of registers */
	#define		STB0297E_NBREGS	    190
	/* Number of fields */
	#define		STB0297E_NBFIELDS	392
	
	
	long BinaryFloatDiv(long n1, long n2, int precision);	
	U32 PowOf2(int number);


	/****************************************************************
						COMMON STRUCTURES AND TYPEDEF
	 ****************************************************************/		
	typedef int FE_297e_Handle_t;
	
	/*	Signal type enum	*/
	typedef enum
	{
		FE_297e_NOTUNER,
		FE_297e_NOAGC,
		FE_297e_NOSIGNAL,
		FE_297e_NOTIMING,
		FE_297e_TIMINGOK,
		FE_297e_NOCARRIER,
		FE_297e_CARRIEROK,
		FE_297e_NOBLIND,
		FE_297e_BLINDOK,
		FE_297e_NODEMOD,
		FE_297e_DEMODOK,
		FE_297e_DATAOK
	} FE_297e_SIGNALTYPE_t;

	typedef enum
	{
		FE_297e_NO_ERROR,
	    FE_297e_INVALID_HANDLE,
	    FE_297e_ALLOCATION,
	    FE_297e_BAD_PARAMETER,
	    FE_297e_ALREADY_INITIALIZED,
	    FE_297e_I2C_ERROR,
	    FE_297e_SEARCH_FAILED,
	    FE_297e_TRACKING_FAILED,
	    FE_297e_TERM_FAILED
	} FE_297e_Error_t;


	typedef enum
	{
	  	FE_297e_MOD_QAM4,
	  	FE_297e_MOD_QAM16,
	  	FE_297e_MOD_QAM32,
	  	FE_297e_MOD_QAM64,
	  	FE_297e_MOD_QAM128,
	  	FE_297e_MOD_QAM256,
	  	FE_297e_MOD_QAM512,
	  	FE_297e_MOD_QAM1024
	} FE_297e_Modulation_t;

	typedef enum
	{
		FE_297e_SPECT_NORMAL,
		FE_297e_SPECT_INVERTED
	}FE_297e_SpectInv_t;
	
	


	/****************************************************************
						INIT STRUCTURES
	 ****************************************************************/
	/*
		structure passed to the FE_297e_Init() function
	*/		
	typedef enum
	{								
		FE_297e_PARALLEL_NORMCLOCK,	
		FE_297e_PARALLEL_INVCLOCK,	
		FE_297e_DVBCI_NORMCLOCK,	
		FE_297e_DVBCI_INVCLOCK,
		FE_297e_SERIAL_NORMCLOCK,
		FE_297e_SERIAL_INVCLOCK,
		FE_297e_SERIAL_NORMFIFOCLOCK,
		FE_297e_SERIAL_INVFIFOCLOCK
	} FE_297e_Clock_t;

	typedef enum
	{
		FE_297e_PARITY_ON,
		FE_297e_PARITY_OFF
	} FE_297e_DataParity_t;


	typedef struct
	{
		char 					DemodName[20];	/* STB0297E name */
		U8						DemodI2cAddr;	/* STB0297E I2c address */
        STTUNER_TunerType_t		TunerModel;		/* Tuner model */
        char 					TunerName[20];  /* Tuner name */
		U8						TunerI2cAddr;	/* Tuner I2c address */
		FE_297e_DataParity_t	Parity;    	    /* Parity bytes */
		FE_297e_Clock_t			Clock;			/* TS Format */
	} FE_297e_InitParams_t;


	/****************************************************************
						SEARCH STRUCTURES
	 ****************************************************************/

	typedef struct
	{
		U32 Frequency_Khz;					/* channel frequency (in KHz)		*/
		U32 SymbolRate_Bds;					/* channel symbol rate  (in bds)	*/
		U32 SearchRange_Hz;					/* range of the search (in Hz) 		*/	
		U32 SweepRate_Hz;					/* Sweep Rate (in Hz)		 		*/	
		FE_297e_Modulation_t Modulation;	/* modulation						*/
		U32 ExternalClock;
	} FE_297e_SearchParams_t;


	typedef struct
	{
		BOOL Locked;						/* channel found	 					*/
		U32	Frequency_Khz;					/* found frequency (in Khz)				*/
		U32 SymbolRate_Bds;					/* found symbol rate (in Bds)			*/
		FE_297e_Modulation_t Modulation;	/* modulation							*/
		FE_297e_SpectInv_t SpectInv;		/* Spectrum Inversion					*/
	} FE_297e_SearchResult_t;

	/************************
		INFO STRUCTURE
	************************/
	typedef struct
	{
		BOOL Locked;						/* Channel locked						*/
		U32 Frequency;						/* Channel frequency (in KHz)			*/
		U32 SymbolRate;						/* Channel symbol rate  (in Mbds)		*/
		FE_297e_Modulation_t Modulation;	/* modulation							*/
		FE_297e_SpectInv_t SpectInversion;	/* Spectrum Inversion					*/
		S32 Power_dBmx10;					/* Power of the RF signal (dBm x 10)	*/			
		U32	CN_dBx10;						/* Carrier to noise ratio (dB x 10)		*/
		U32	BER;							/* Bit error rate (x 10000)				*/
	} FE_297e_SignalInfo_t;
	
	

	
	/****************************************************************
						LOW LEVEL FUNCTIONS
	****************************************************************/
	
	
	/****************************************************************
						API FUNCTIONS
	****************************************************************/
    FE_297e_Error_t	FE_297e_Search( 
    								IOARCH_Handle_t IOHandle,
    								FE_297e_SearchParams_t	*pSearch,
    								FE_297e_SearchResult_t	*pResult,
    								STTUNER_Handle_t   TopLevelHandle);
    
    FE_297e_Error_t FE_297e_Status(FE_297e_Handle_t	Handle,FE_297e_SIGNALTYPE_t *SignalType);

	FE_297e_Error_t	FE_297e_GetSignalInfo(
										 IOARCH_Handle_t IOHandle,
										 FE_297e_SignalInfo_t	*pInfo);


#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif                          /* __STTUNER_CAB_DRV0297E_H */


/* End of drv0297e.h */
