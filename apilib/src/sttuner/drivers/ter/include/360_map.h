



#ifndef H_MAP360



	#define H_MAP360

#ifndef STTUNER_MINIDRIVER

/* ID */
 #define R_ID 0x0
 #define IDENTIFICATIONREGISTER 0xff

/* I2CRPT */
 #define R_I2CRPT 0x1
 #define I2CT_ON 0x10080
 #define ENARPT_LEVEL 0x10070
 #define SCLT_DELAY 0x10008
 #define SCLT_NOD 0x10004
 #define STOP_ENABLE 0x10002


/* TOPCTRL */
 #define R_TOPCTRL 0x2
 #define STDBY 0x20080
 #define STDBY_FEC 0x20040
 #define STDBY_CORE 0x20020
 #define DIR_CLK 0x20010
 #define TS_DIS 0x20008
 #define TQFP80 0x20004
 #define BYPASS_PGA 0x20002
 #define ENA_27 0x20001

/* IOCFG0 */
 #define R_IOCFG0 0x3
 #define OP0_SD 0x30080
 #define OP0_VAL 0x30040
 #define OP0_OD 0x30020
 #define OP0_INV 0x30010
 #define OP0_DACVALUE_HI 0x3000f

/* DAC0R */
 #define R_DAC0R 0x4
 #define OP0_DACVALUE_LO 0x400ff

/* IOCFG1 */
 #define R_IOCFG1 0x5
 #define IP0 0x50040
 #define OP1_OD 0x50020
 #define OP1_INV 0x50010
 #define OP1_DACVALUE_HI 0x5000f


/* DAC1R */
 #define R_DAC1R 0x6
 #define OP1_DACVALUE_LO 0x600ff

/* IOCFG2 */
 #define R_IOCFG2 0x7
 #define OP2_LOCK_CONF 0x700e0
 #define OP2_OD 0x70010
 #define OP2_VAL 0x70008
 #define OP1_LOCK_CONF 0x70007

/* PWMFR */
 #define R_PWMFR 0x8
 #define OP0_FREQ 0x800f0
 #define OP1_FREQ 0x8000f

/* STATUS */
 #define R_STATUS 0x9
 #define TPS_LOCK 0x90080
 #define SYR_LOCK 0x90040
 #define AGC_LOCK 0x90020
 #define PRF 0x90010
 #define LK 0x90008
 #define PR 0x90007

/* AUX_CLK */
 #define R_AUX_CLK 0xa
 #define AUXFEC12B 0xa0080
 #define AUXFEC_ENA 0xa0040
 #define DIS_CKX4 0xa0020
 #define CKSEL 0xa0018
 #define CKDIV_PROG 0xa0006
 #define AUXCLK_ENA 0xa0001

/* FREESYS1 */
 #define R_FREESYS1 0xb


/* FREESYS2 */
 #define R_FREESYS2 0xc


/* FREESYS3 */
 #define R_FREESYS3 0xd


/* AGC2MAX */
 #define R_AGC2MAX 0x10
 #define AGC2MAX 0x1000ff

/* AGC2MIN */
 #define R_AGC2MIN 0x11
 #define AGC2MIN 0x1100ff

/* AGC1MAX */
 #define R_AGC1MAX 0x12
 #define AGC1MAX 0x1200ff

/* AGC1MIN */
 #define R_AGC1MIN 0x13
 #define AGC1MIN 0x1300ff

/* AGCR */
 #define R_AGCR 0x14
 #define RATIO_A 0x1400e0
 #define RATIO_B 0x140018
 #define RATIO_C 0x140007

/* AGC2TH */
 #define R_AGC2TH 0x15
 #define AGC2_THRES 0x1500ff

/* AGC12C3 */
 #define R_AGC12C3 0x16
 #define AGC1_IV 0x160080
 #define AGC1_OD 0x160040
 #define AGC1_LOAD 0x160020
 #define AGC2_IV 0x160010
 #define AGC2_OD 0x160008
 #define AGC2_LOAD 0x160004
 #define AGC12_MODE 0x160003


/* AGCCTRL1 */
 #define R_AGCCTRL1 0x17
 #define DAGC_ON 0x170080
 #define AGC1_MODE 0x170008
 #define AGC2_MODE 0x170007


/* AGCCTRL2 */
 #define R_AGCCTRL2 0x18
 #define FRZ2_CTRL 0x180060
 #define FRZ1_CTRL 0x180018
 #define TIME_CST 0x180007


/* AGC1VAL1 */
 #define R_AGC1VAL1 0x19
 #define AGC1_VAL_LO 0x1900ff

/* AGC1VAL2 */
 #define R_AGC1VAL2 0x1a
 #define AGC1_VAL_HI 0x1a000f


/* AGC2VAL1 */
 #define R_AGC2VAL1 0x1b
 #define AGC2_VAL_LO 0x1b00ff

/* AGC2VAL2 */
 #define R_AGC2VAL2 0x1c
 #define AGC2_VAL_HI 0x1c000f


/* AGC2PGA */
 #define R_AGC2PGA 0x1d
 #define UAGC2PGA 0x1d003f


/* OVF_RATE1 */
 #define R_OVF_RATE1 0x1e
 #define OVF_RATE_HI 0x1e000f


/* OVF_RATE2 */
 #define R_OVF_RATE2 0x1f
 #define OVF_RATE_LO 0x1f00ff

/* GAIN_SRC1 */
 #define R_GAIN_SRC1 0x20
 #define INV_SPECTR 0x200080
 #define IQ_INVERT 0x200040
 #define INR_BYPASS 0x200020
 #define GAIN_SRC_HI 0x20000f


/* GAIN_SRC2 */
 #define R_GAIN_SRC2 0x21
 #define GAIN_SRC_LO 0x2100ff

/* INC_DEROT1 */
 #define R_INC_DEROT1 0x22
 #define INC_DEROT_HI 0x2200ff

/* INC_DEROT2 */
 #define R_INC_DEROT2 0x23
 #define INC_DEROT_LO 0x2300ff

/* FREESTFE_1 */
 #define R_FREESTFE_1 0x26
 #define AVERAGE_ON 0x260004
 #define DC_ADJ 0x260002
 #define SEL_LSB 0x260001




/* SYR_THR */
 #define R_SYR_THR 0x27
 #define SEL_SRCOUT 0x2700c0
 #define SEL_SYRTHR 0x27001f


/* INR */
 #define R_INR 0x28
 #define INR 0x2800ff

/* EN_PROCESS */
 #define R_EN_PROCESS 0x29
 #define ENAB 0x290001


/* SDI_SMOOTHER */
 #define R_SDI_SMOOTHER 0x2a
 #define DIS_SMOOTH 0x2a0080
 #define SDI_INC_SMOOTHER 0x2a003f


/* FE_LOOP_OPEN */
 #define R_FE_LOOP_OPEN 0x2b
 #define TRL_LOOP_OP 0x2b0002
 #define CRL_LOOP_OP 0x2b0001


/* EPQ */
 #define R_EPQ 0x31
 #define EPQ 0x3100ff

/* EPQ2 */
 #define R_EPQ2 0x32
 #define EPQ2 0x3200ff

/* COR_CTL */
 #define R_COR_CTL 0x80
 #define CORE_ACTIVE 0x800020
 #define HOLD 0x800010
 #define CORE_STATE_CTL 0x80000f


/* COR_STAT */
 #define R_COR_STAT 0x81
 #define TPS_LOCKED 0x810040
 #define SYR_LOCKED_COR 0x810020
 #define AGC_LOCKED_STAT 0x810010
 #define CORE_STATE_STAT 0x81000f


/* COR_INTEN */
 #define R_COR_INTEN 0x82
 #define INTEN 0x820080
 #define INTEN_SYR 0x820020
 #define INTEN_FFT 0x820010
 #define INTEN_AGC 0x820008
 #define INTEN_TPS1 0x820004
 #define INTEN_TPS2 0x820002
 #define INTEN_TPS3 0x820001


/* COR_INTSTAT */
 #define R_COR_INTSTAT 0x83
 #define INTSTAT_SYR 0x830020
 #define INTSTAT_FFT 0x830010
 #define INTSAT_AGC 0x830008
 #define INTSTAT_TPS1 0x830004
 #define INTSTAT_TPS2 0x830002
 #define INTSTAT_TPS3 0x830001


/* COR_MODEGUARD */
 #define R_COR_MODEGUARD 0x84
 #define FORCE 0x840008
 #define MODE 0x840004
 #define GUARD 0x840003


/* AGC_CTL */
 #define R_AGC_CTL 0x85
 #define DELAY_STARTUP 0x8500e0
 #define AGC_LAST 0x850010
 #define AGC_GAIN 0x85000c
 #define AGC_NEG 0x850002
 #define AGC_SET 0x850001

/* AGC_MANUAL1 */
 #define R_AGC_MANUAL1 0x86
 #define AGC_VAL_LO 0x8600ff


/* AGC_MANUAL2 */
 #define R_AGC_MANUAL2 0x87
 #define AGC_VAL_HI 0x87000f



/* AGC_TARGET */
 #define R_AGC_TARGET 0x88
 #define AGC_TARGET 0x8800ff

/* AGC_GAIN1 */
 #define R_AGC_GAIN1 0x89
 #define AGC_GAIN_LO 0x8900ff

/* AGC_GAIN2 */
 #define R_AGC_GAIN2 0x8a
 #define AGC_LOCKED_GAIN2 0x8a0010
 #define AGC_GAIN_HI 0x8a000f


/* ITB_CTL */
 #define R_ITB_CTL 0x8b
 #define ITB_INVERT 0x8b0001


/* ITB_FREQ1 */
 #define R_ITB_FREQ1 0x8c
 #define ITB_FREQ_LO 0x8c00ff

/* ITB_FREQ2 */
 #define R_ITB_FREQ2 0x8d
 #define ITB_FREQ_HI 0x8d003f


/* CAS_CTL */
 #define R_CAS_CTL 0x8e
 #define CCS_ENABLE 0x8e0080
 #define ACS_DISABLE 0x8e0040
 #define DAGC_DIS 0x8e0020
 #define DAGC_GAIN 0x8e0018
 #define CCSMU 0x8e0007

/* CAS_FREQ */
 #define R_CAS_FREQ 0x8f
 #define CCS_FREQ 0x8f00ff

/* CAS_DAGCGAIN */
 #define R_CAS_DAGCGAIN 0x90
 #define CAS_DAGC_GAIN 0x9000ff

/* SYR_CTL */
 #define R_SYR_CTL 0x91
 #define SICTHENABLE 0x910080
 #define LONG_ECHO 0x910178
 #define AUTO_LE_EN 0x910004
 #define SYR_BYPASS 0x910002
 #define SYR_TR_DIS 0x910001

/* SYR_STAT */
 #define R_SYR_STAT 0x92
 #define SYR_LOCKED_STAT 0x920010
 #define SYR_MODE 0x920004
 #define SYR_GUARD 0x920003



/* SYR_NCO1 */
 #define R_SYR_NCO1 0x93
 #define SYR_NCO_LO 0x9300ff

/* SYR_NCO2 */
 #define R_SYR_NCO2 0x94
 #define SYR_NCO_HI 0x94003f


/* SYR_OFFSET1 */
 #define R_SYR_OFFSET1 0x95
 #define SYR_OFFSET_LO 0x9500ff

/* SYR_OFFSET2 */
 #define R_SYR_OFFSET2 0x96
 #define SYR_OFFSET_HI 0x96003f


/* FFT_CTL */
 #define R_FFT_CTL 0x97
 #define FFT_TRIGGER 0x970004
 #define FFT_MANUAL 0x970002
 #define IFFT_MODE 0x970001


/* SCR_CTL */
 #define R_SCR_CTL 0x98
 #define SCR_CPDIS 0x980002
 #define SCR_DIS 0x980001


/* PPM_CTL1 */
 #define R_PPM_CTL1 0x99
 #define PPM_MAXFREQ 0x990030
 #define PPM_SCATDIS 0x990002
 #define PPM_BYP 0x990001



/* TRL_CTL */
 #define R_TRL_CTL 0x9a
 #define TRL_NOMRATE_LSB 0x9a0080
 #define TRL_TR_GAIN 0x9a0038
 #define TRL_LOOPGAIN 0x9a0007


/* TRL_NOMRATE1 */
 #define R_TRL_NOMRATE1 0x9b
 #define TRL_NOMRATE_LO 0x9b00ff

/* TRL_NOMRATE2 */
 #define R_TRL_NOMRATE2 0x9c
 #define TRL_NOMRATE_HI 0x9c00ff

/* TRL_TIME1 */
 #define R_TRL_TIME1 0x9d
 #define TRL_TOFFSET_LO 0x9d00ff

/* TRL_TIME2 */
 #define R_TRL_TIME2 0x9e
 #define TRL_TOFFSET_HI 0x9e00ff

/* CRL_CTL */
 #define R_CRL_CTL 0x9f
 #define CRL_DIS 0x9f0080
 #define CRL_TR_GAIN 0x9f0038
 #define CRL_LOOPGAIN 0x9f0007


/* CRL_FREQ1 */
 #define R_CRL_FREQ1 0xa0
 #define CRL_FOFFSET_LO 0xa000ff

/* CRL_FREQ2 */
 #define R_CRL_FREQ2 0xa1
 #define CRL_FOFFSET_HI 0xa100ff

/* CRL_FREQ3 */
 #define R_CRL_FREQ3 0xa2
 #define SEXT 0xa20080
 #define CRL_FOFFSET_VHI 0xa2007f

/* CHC_CTL1 */
 #define R_CHC_CTL1 0xa3
 #define MEAN_PILOT_GAIN 0xa300e0
 #define MMEANP 0xa30010
 #define DBADP 0xa30008
 #define DNOISEN 0xa30004
 #define DCHCPRED 0xa30002
 #define CHC_INT 0xa30001

/* CHC_SNR */
 #define R_CHC_SNR 0xa4
 #define CHC_SNR 0xa400ff

/* BDI_CTL */
 #define R_BDI_CTL 0xa5
 #define BDI_LPSEL 0xa50002
 #define BDI_SERIAL 0xa50001


/* DMP_CTL */
 #define R_DMP_CTL 0xa6
 #define DMP_SDDIS 0xa60001


/* TPS_RCVD1 */
 #define R_TPS_RCVD1 0xa7
 #define TPS_CHANGE 0xa70040
 #define BCH_OK 0xa70020
 #define TPS_SYNC 0xa70010
 #define TPS_FRAME 0xa70003



/* TPS_RCVD2 */
 #define R_TPS_RCVD2 0xa8
 #define TPS_HIERMODE 0xa80070
 #define TPS_CONST 0xa80003



/* TPS_RCVD3 */
 #define R_TPS_RCVD3 0xa9
 #define TPS_LPCODE 0xa90070
 #define TPS_HPCODE 0xa90007



/* TPS_RCVD4 */
 #define R_TPS_RCVD4 0xaa
 #define TPS_GUARD 0xaa0030
 #define TPS_MODE 0xaa0003



/* TPS_CELLID */
 #define R_TPS_CELLID 0xab
 #define TPS_CELLID 0xab00ff

/* TPS_FREE2 */
 #define R_TPS_FREE2 0xac


/* TPS_SET1 */
 #define R_TPS_SET1 0xad
 #define TPS_SETFRAME 0xad0003


/* TPS_SET2 */
 #define R_TPS_SET2 0xae
 #define TPS_SETHIERMODE 0xae0070
 #define TPS_SETCONST 0xae0003



/* TPS_SET3 */
 #define R_TPS_SET3 0xaf
 #define TPS_SETLPCODE 0xaf0070
 #define TPS_SETHPCODE 0xaf0007



/* TPS_CTL */
 #define R_TPS_CTL 0xb0
 #define TPS_IMM 0xb00004
 #define TPS_BCHDIS 0xb00002
 #define TPS_UPDDIS 0xb00001


/* CTL_FFTOSNUM */
 #define R_CTL_FFTOSNUM 0xb1
 #define SYMBOL_NUMBER 0xb100ff

/* CAR_DISP_SEL */
 #define R_CAR_DISP_SEL 0xb2
 #define CAR_DISP_SEL 0xb2000f


/* MSC_REV */
 #define R_MSC_REV 0xb3
 #define REV_NUMBER 0xb300ff

/* PIR_CTL */
 #define R_PIR_CTL 0xb4
 #define FREEZE 0xb40001


/* SNR_CARRIER1 */
 #define R_SNR_CARRIER1 0xb5
 #define SNR_CARRIER_LO 0xb500ff

/* SNR_CARRIER2 */
 #define R_SNR_CARRIER2 0xb6
 #define MEAN 0xb60080
 #define SNR_CARRIER_HI 0xb6001f



/* PPM_CPAMP */
 #define R_PPM_CPAMP 0xb7
 #define PPM_CPC 0xb700ff

/* TSM_AP0 */
 #define R_TSM_AP0 0xb8
 #define ADDRESS_BYTE_0 0xb800ff

/* TSM_AP1 */
 #define R_TSM_AP1 0xb9
 #define ADDRESS_BYTE_1 0xb900ff

/* TSM_AP2 */
 #define R_TSM_AP2 0xba
 #define DATA_BYTE_0 0xba00ff

/* TSM_AP3 */
 #define R_TSM_AP3 0xbb
 #define DATA_BYTE_1 0xbb00ff

/* TSM_AP4 */
 #define R_TSM_AP4 0xbc
 #define DATA_BYTE_2 0xbc00ff

/* TSM_AP5 */
 #define R_TSM_AP5 0xbd
 #define DATA_BYTE_3 0xbd00ff

/* TSM_AP6 */
 #define R_TSM_AP6 0xbe



/* TSM_AP7 */
 #define R_TSM_AP7 0xbf
 #define MEM_SELECT_BYTE 0xbf00ff

/* CONSTMODE */
 #define R_CONSTMODE 0xcb
 #define CAR_TYPE 0xcb0018
 #define IQ_RANGE 0xcb0004
 #define CONST_MODE 0xcb0003


/* CONSTCARR1 */
 #define R_CONSTCARR1 0xcc
 #define CONST_CARR_LO 0xcc00ff

/* CONSTCARR2 */
 #define R_CONSTCARR2 0xcd
 #define CONST_CARR_HI 0xcd001f


/* ICONSTEL */
 #define R_ICONSTEL 0xce
 #define ICONSTEL 0xce01ff

/* QCONSTEL */
 #define R_QCONSTEL 0xcf
 #define QCONSTEL 0xcf01ff

/* AGC1RF */
 #define R_AGC1RF 0xd4
 #define RF_AGC1_LEVEL 0xd400ff

/* EN_RF_AGC1 */
 #define R_EN_RF_AGC1 0xd5
 #define START_ADC 0xd50080


/* FECM */
 #define R_FECM 0x40
 #define FEC_MODE 0x4000f0
 #define VIT_DIFF 0x400004
 #define SYNC 0x400002
 #define SYM 0x400001


/* VTH0 */
 #define R_VTH0 0x41
 #define VTH0 0x41007f


/* VTH1 */
 #define R_VTH1 0x42
 #define VTH1 0x42007f


/* VTH2 */
 #define R_VTH2 0x43
 #define VTH2 0x43007f


/* VTH3 */
 #define R_VTH3 0x44
 #define VTH3 0x44007f


/* VTH4 */
 #define R_VTH4 0x45
 #define VTH4 0x45007f


/* VTH5 */
 #define R_VTH5 0x46
 #define VTH5 0x46007f


/* FREEVIT */
 #define R_FREEVIT 0x47


/* VITPROG */
 #define R_VITPROG 0x49
 #define FORCE_ROTA 0x4900c0
 #define MDIVIDER 0x490003



/* PR */
 #define R_PR 0x4a
 #define FRAPTCR 0x4a0080
 #define E7_8 0x4a0020
 #define E6_7 0x4a0010
 #define E5_6 0x4a0008
 #define E3_4 0x4a0004
 #define E2_3 0x4a0002
 #define E1_2 0x4a0001


/* VSEARCH */
 #define R_VSEARCH 0x4b
 #define PR_AUTO 0x4b0080
 #define PR_FREEZE 0x4b0040
 #define SAMPNUM 0x4b0030
 #define TIMEOUT 0x4b000c
 #define HYSTER 0x4b0003

/* RS */
 #define R_RS 0x4c
 #define DEINT_ENA 0x4c0080
 #define OUTRS_SP 0x4c0040
 #define RS_ENA 0x4c0020
 #define DESCR_ENA 0x4c0010
 #define ERRBIT_ENA 0x4c0008
 #define FORCE47 0x4c0004
 #define CLK_POL 0x4c0002
 #define CLK_CFG 0x4c0001

/* RSOUT */
 #define R_RSOUT 0x4d
 #define ENA_STBACKEND 0x4d0010
 #define ENA8_LEVEL 0x4d000f

/* ERRCTRL1 */
 #define R_ERRCTRL1 0x4e
 #define ERRMODE1 0x4e0080
 #define TESTERS1 0x4e0040
 #define ERR_SOURCE1 0x4e0030
 #define RESET_CNTR1 0x4e0004
 #define NUM_EVENT1 0x4e0003


/* ERRCNTM1 */
 #define R_ERRCNTM1 0x4f
 #define ERROR_COUNT1_HI 0x4f00ff

/* ERRCNTL1 */
 #define R_ERRCNTL1 0x50
 #define ERROR_COUNT1_LO 0x5000ff

/* ERRCTRL2 */
 #define R_ERRCTRL2 0x51
 #define ERRMODE2 0x510080
 #define TESTERS2 0x510040
 #define ERR_SOURCE2 0x510030
 #define RESET_CNTR2 0x510004
 #define NUM_EVENT2 0x510003


/* ERRCNTM2 */
 #define R_ERRCNTM2 0x52
 #define ERROR_COUNT2_HI 0x5200ff

/* ERRCNTL2 */
 #define R_ERRCNTL2 0x53
 #define ERROR_COUNT2_LO 0x5300ff

/* ERRCTRL3 */
 #define R_ERRCTRL3 0x56
 #define ERRMODE3 0x560080
 #define TESTERS3 0x560040
 #define ERR_SOURCE3 0x560030
 #define RESET_CNTR3 0x560004
 #define NUM_EVENT3 0x560003


/* ERRCNTM3 */
 #define R_ERRCNTM3 0x57
 #define ERROR_COUNT3_HI 0x5700ff

/* ERRCNTL3 */
 #define R_ERRCNTL3 0x58
 #define ERROR_COUNT3_LO 0x5800ff

/* DILSTK1 */
 #define R_DILSTK1 0x59
 #define DILSTK_HI 0x5900ff


/* DILSTK0 */
 #define R_DILSTK0 0x5a
 #define DILSTK_LO 0x5a00ff


/* DILBWSTK1 */
 #define R_DILBWSTK1 0x5b

/* DILBWST0 */
 #define R_DILBWST0 0x5c





/* LNBRX */
 #define R_LNBRX 0x5d
 #define LINE_OK 0x5d0080
 #define OCCURRED_ERR 0x5d0040


/* RSTC */
 #define R_RSTC 0x5e
 #define DEINNTE 0x5e0080
 #define DIL64_ON 0x5e0040
 #define RSTC 0x5e0020
 #define DESCRAMTC 0x5e0010
 #define MODSYNCBYTE 0x5e0004
 #define LOWP_DIS 0x5e0002
 #define HI 0x5e0001



/* VIT_BIST */
 #define R_VIT_BIST 0x5f
 #define RAND_RAMP 0x5f0040
 #define NOISE_LEVEL 0x5f0038
 #define PR_VIT_BIST 0x5f0007


/* FREEDRS */
 #define R_FREEDRS 0x54


/* VERROR */
 #define R_VERROR 0x55
 #define ERROR_VALUE 0x5500ff

/* TSTRES */
 #define R_TSTRES 0xc0
 #define FRESI2C 0xc00080
 #define FRESRS 0xc00040
 #define FRESACS 0xc00020
 #define FRES_PRIF 0xc00010
 #define FRESFEC1_2 0xc00008
 #define FRESFEC 0xc00004
 #define FRESCORE1_2 0xc00002
 #define FRESCORE 0xc00001


/* ANACTRL */
 #define R_ANACTRL 0xc1
 #define STDBY_PLL 0xc10080
 #define BYPASS_XTAL 0xc10040
 #define STDBY_PGA 0xc10020
 #define TEST_PGA 0xc10010
 #define STDBY_ADC 0xc10008
 #define BYPASS_ADC 0xc10004
 #define SGN_ADC 0xc10002
 #define TEST_ADC 0xc10001


/* TSTBUS */
 #define R_TSTBUS 0xc2
 #define EXT_TESTIN 0xc20080
 #define EXT_ADC 0xc20040
 #define TEST_IN 0xc20038
 #define TS 0xc20007


/* TSTCK */
 #define R_TSTCK 0xc3
 #define CKFECEXT 0xc30080
 #define FORCERATE1 0xc30008
 #define TSTCKRS 0xc30004
 #define TSTCKDIL 0xc30002
 #define DIRCKINT 0xc30001



/* TSTI2C */
 #define R_TSTI2C 0xc4
 #define EN_VI2C 0xc40080
 #define TI2C 0xc40060
 #define BFAIL_BAD 0xc40010
 #define RBACT 0xc40008
 #define TST_PRIF 0xc40007


/* TSTRAM1 */
 #define R_TSTRAM1 0xc5
 #define SELADR1 0xc50080
 #define FSELRAM1 0xc50040
 #define FSELDEC 0xc50020
 #define FOEB 0xc5001c
 #define FADR 0xc50003

/* TSTRATE */
 #define R_TSTRATE 0xc6
 #define FORCEPHA 0xc60080
 #define FNEWPHA 0xc60010
 #define FROT90 0xc60008
 #define FR 0xc60007


/* SELOUT */
 #define R_SELOUT 0xc7
 #define EN_VLOG 0xc70080
 #define SELVIT60 0xc70040
 #define SELSYN3 0xc70020
 #define SELSYN2 0xc70010
 #define SELSYN1 0xc70008
 #define SELLIFO 0xc70004
 #define SELFIFO 0xc70002
 #define TSTFIFO_SELOUT 0xc70001

/* FORCEIN */
 #define R_FORCEIN 0xc8
 #define SEL_VITDATAIN 0xc80080
 #define FORCE_ACS 0xc80040
 #define TSTSYN 0xc80020
 #define TSTRAM64 0xc80010
 #define TSTRAM 0xc80008
 #define TSTERR2 0xc80004
 #define TSTERR 0xc80002
 #define TSTACS 0xc80001

/* TSTFIFO */
 #define R_TSTFIFO 0xc9
 #define FORMSB 0xc90004
 #define FORLSB 0xc90002
 #define TSTFIFO_TSTFIFO 0xc90001


/* TSTRS */
 #define R_TSTRS 0xca
 #define TST_SCRA 0xca0080
 #define OLDRS6 0xca0040
 #define ADCT 0xca0030
 #define DILT 0xca000c
 #define SCARBIT 0xca0002
 #define TSTRS_EN 0xca0001

/* TSTBISTRES0 */
 #define R_TSTBISTRES0 0xd0
 #define BEND_CHC2 0xd00080
 #define BBAD_CHC2 0xd00040
 #define BEND_PPM 0xd00020
 #define BBAD_PPM 0xd00010
 #define BEND_FFTW 0xd00008
 #define BBAD_FFTW 0xd00004
 #define BEND_FFTI 0xd00002
 #define BBAD_FFTI 0xd00001

/* TSTBISTRES1 */
 #define R_TSTBISTRES1 0xd1
 #define BEND_CHC1 0xd10080
 #define BBAD_CHC1 0xd10040
 #define BEND_SYR 0xd10020
 #define BBAD_SYR 0xd10010
 #define BEND_SDI 0xd10008
 #define BBAD_SDI 0xd10004
 #define BEND_BDI 0xd10002
 #define BBAD_BDI 0xd10001

/* TSTBISTRES2 */
 #define R_TSTBISTRES2 0xd2
 #define BEND_VIT2 0xd20080
 #define BBAD_VIT2 0xd20040
 #define BEND_VIT1 0xd20020
 #define BBAD_VIT1 0xd20010
 #define BEND_DIL 0xd20008
 #define BBAD_DIL 0xd20004
 #define BEND_RS 0xd20002
 #define BBAD_RS 0xd20001

/* TSTBISTRES3 */
 #define R_TSTBISTRES3 0xd3
 #define BEND_FIFO 0xd30002
 #define BBAD_FIFO 0xd30001


	/* Number of registers  */
/*#else     DEBUG_VERSION*/
#define		STV360_NBREGS	159

#define		STV360_NBFIELDS	466 

#else      /* Mini driver definitions*/

/*  ID  */
/*(RX123)||(FX_S << 8)|(FX_L << 12)*/
#define R_ID 0x00 
#define F_R_ID_S 0
#define F_R_ID_L 8
#define F_R_ID  (R_ID)|(F_R_ID_S << 16)|(F_R_ID_L << 24)
#define IDENTIFICATIONREGISTER 0 

/*  I2CRPT  */
#define R_I2CRPT 0x01 
#define I2CT_ON_S 7 
#define ENARPT_LEVEL_S 4 
#define SCLT_DELAY_S 3 
#define SCLT_NOD_S 2 
#define STOP_ENABLE_S 1 
#define F_I2CRPT_RESERVED_1_S 0

#define I2CT_ON_L 1 
#define ENARPT_LEVEL_L 3 
#define SCLT_DELAY_L 1 
#define SCLT_NOD_L 1 
#define STOP_ENABLE_L 1 
#define F_I2CRPT_RESERVED_1_L 1

#define I2CT_ON (R_I2CRPT)|(I2CT_ON_S << 16)|(I2CT_ON_L << 24) 
#define ENARPT_LEVEL (R_I2CRPT)|(ENARPT_LEVEL_S << 16)|(ENARPT_LEVEL_L << 24)
#define SCLT_DELAY (R_I2CRPT)|(SCLT_DELAY_S << 16)|(SCLT_DELAY_L << 24) 
#define SCLT_NOD (R_I2CRPT)|(SCLT_NOD_S << 16)|(SCLT_NOD_L << 24) 
#define STOP_ENABLE (R_I2CRPT)|(STOP_ENABLE_S << 16)|(STOP_ENABLE_L << 24) 
#define F_I2CRPT_RESERVED_1 (R_I2CRPT)|(F_I2CRPT_RESERVED_1_S << 16)|(F_I2CRPT_RESERVED_1_L << 24)

/*  TOPCTRL */
#define R_TOPCTRL 0x02 
#define STDBY_S 7 
#define STDBY_FEC_S 6 
#define STDBY_CORE_S 5 
#define DIR_CLK_S 4 
#define TS_DIS_S 3 
#define TQFP80_S 2 
#define BYPASS_PGA_S 1 
#define ENA_27_S 0 

#define STDBY_L 1 
#define STDBY_FEC_L 1 
#define STDBY_CORE_L 1 
#define DIR_CLK_L 1 
#define TS_DIS_L 1 
#define TQFP80_L 1 
#define BYPASS_PGA_L 1 
#define ENA_27_L 1 

#define STDBY (R_TOPCTRL)|(STDBY_S << 16)|(STDBY_L << 24) 
#define STDBY_FEC (R_TOPCTRL)|(STDBY_FEC_S << 16)|(STDBY_FEC_L << 24) 
#define STDBY_CORE (R_TOPCTRL)|(STDBY_CORE_S << 16)|(STDBY_CORE_L << 24) 
#define DIR_CLK (R_TOPCTRL)|(DIR_CLK_S << 16)|(DIR_CLK_L << 24) 
#define TS_DIS (R_TOPCTRL)|(TS_DIS_S << 16)|(TS_DIS_L << 24) 
#define TQFP80 (R_TOPCTRL)|(TQFP80_S << 16)|(TQFP80_L << 24) 
#define BYPASS_PGA (R_TOPCTRL)|(BYPASS_PGA_S << 16)|(BYPASS_PGA_L << 24) 
#define ENA_27 (R_TOPCTRL)|(ENA_27_S << 16)|(ENA_27_L << 24) 

/*  IOCFG0  */
#define R_IOCFG0 0x03 
/*#define OP0_SD 15 
#define OP0_VAL 16 
#define OP0_OD 17 
#define OP0_INV 18 
#define OP0_DACVALUE_HI 19 
*/
/*  DAC0R   */
#define R_DAC0R 0x04 
/*#define OP0_DACVALUE_LO 20 
*/
/*  IOCFG1  */
#define R_IOCFG1 0x05 
/*
#define F_IOCFG1_RESERVED_1 21
#define IP0 22 
#define OP1_OD 23 
#define OP1_INV 24 
#define OP1_DACVALUE_HI 25
*/
/*  DAC1R   */
#define R_DAC1R 0x06 
/*#define OP1_DACVALUE_LO 26 
*/
/*  IOCFG2  */
#define R_IOCFG2 0x07 
#define OP2_LOCK_CONF_S 5 
#define OP2_OD_S 4 
#define OP2_VAL_S 3 
#define OP1_LOCK_CONF_S 0 

#define OP2_LOCK_CONF_L 3 
#define OP2_OD_L 1 
#define OP2_VAL_L 1 
#define OP1_LOCK_CONF_L 3 

#define OP2_LOCK_CONF (R_IOCFG2)|(OP2_LOCK_CONF_S << 16)|(OP2_LOCK_CONF_L << 24) 
#define OP2_OD (R_IOCFG2)|(OP2_OD_S << 16)|(OP2_OD_L << 24) 
#define OP2_VAL (R_IOCFG2)|(OP2_VAL_S << 16)|(OP2_VAL_L << 24) 
#define OP1_LOCK_CONF (R_IOCFG2)|(OP1_LOCK_CONF_S << 16)|(OP1_LOCK_CONF_L << 24) 
/*  PWMFR    */
#define R_PWMFR 0x08 
/*#define OP0_FREQ 31 
#define OP1_FREQ 32 
*/
/*  STATUS  */
#define R_STATUS 0x09 
#define TPS_LOCK_S 7
#define SYR_LOCK_S 6 
#define AGC_LOCK_S 5 
#define PRF_S 4 
#define LK_S 3 
#define PR_S 0 

#define TPS_LOCK_L 1
#define SYR_LOCK_L 1 
#define AGC_LOCK_L 1 
#define PRF_L 1 
#define LK_L 1 
#define PR_L 3

#define TPS_LOCK (R_STATUS)|(TPS_LOCK_S << 16)|(TPS_LOCK_L << 24)
#define SYR_LOCK (R_STATUS)|(SYR_LOCK_S << 16)|(SYR_LOCK_L << 24)
#define AGC_LOCK (R_STATUS)|(AGC_LOCK_S << 16)|(AGC_LOCK_L << 24)
#define PRF (R_STATUS)|(PRF_S << 16)|(PRF_L << 24)
#define LK (R_STATUS)|(LK_S << 16)|(LK_L << 24)
#define PR (R_STATUS)|(PR_S << 16)|(PR_L << 24)
/*  AUX_CLK */
#define R_AUX_CLK 0x0a 
/*#define AUXFEC_ENA 40 
#define DIS_CKX4 41 
#define CKSEL 42 
#define CKDIV_PROG 43 
#define AUXCLK_ENA 44 
*/
/*  FREESYS1    */
#define R_FREESYS1 0x0b 
/*#define F_FREESYS1_RESERVED_1 45
*/
/*  FREESYS2    */
#define R_FREESYS2 0x0c 
/*#define F_FREESYS2_RESERVED_1 46
*/
/*  FREESYS3    */
#define R_FREESYS3 0x0d 
/*#define F_FREESYS3_RESERVED_1 47
*/
/*  AGC2MAX */
#define R_AGC2MAX 0x10 
/*#define AGC2MAX 48 
*/
/*  AGC2MIN */
#define R_AGC2MIN 0x11 
/*#define AGC2MIN 49
*/
/*  AGC1MAX */
#define R_AGC1MAX 0x12 
/*#define AGC1MAX 50 
*/
/*  AGC1MIN */
#define R_AGC1MIN 0x13 
/*#define AGC1MIN 51
*/
/*  AGCR    */
#define R_AGCR 0x14 
/*#define RATIO_A 52 
#define RATIO_B 53 
#define RATIO_C 54
*/
/*  AGC2TH  */
#define R_AGC2TH 0x15 
/*#define AGC2_THRES 55 
*/
/*  AGC12C */
#define R_AGC12C3 0x16 
/*#define AGC1_IV 56 
#define AGC1_OD 57 
#define AGC1_LOAD 58 
#define AGC2_IV 59
#define AGC2_OD 60 
#define AGC2_LOAD 61 
#define AGC12_MODE 62 
*/

/*  AGCCTRL1    */
#define R_AGCCTRL1 0x17 
/*#define DAGC_ON 63 
#define F_AGCCTRL1_RESERVED_1 64
#define AGC1_MODE 65 
#define AGC2_MODE 66 
*/

/*  AGCCTRL2    */
#define R_AGCCTRL2 0x18 
/*#define F_AGCCTRL2_RESERVED_1 67
#define FRZ2_CTRL 68
#define FRZ1_CTRL 69 
#define TIME_CST 70
*/
/*  AGC1VAL1    */
#define R_AGC1VAL1 0x19 
#define AGC1_VAL_LO_S 0 

#define AGC1_VAL_LO_L 8

#define AGC1_VAL_LO (R_AGC1VAL1)|(AGC1_VAL_LO_S << 16)|(AGC1_VAL_LO_L << 24) 
/*  AGC1VAL2    */
#define R_AGC1VAL2 0x1a 

/*#define F_AGC1VAL2_RESERVED_1 72*/
#define AGC1_VAL_HI_S 0

#define AGC1_VAL_HI_L 4

#define AGC1_VAL_HI (R_AGC1VAL2)|(AGC1_VAL_HI_S << 16)|(AGC1_VAL_HI_L << 24)
  
/*  AGC2VAL1    */
#define R_AGC2VAL1 0x1b 

#define AGC2_VAL_LO_S 0
 
#define AGC2_VAL_LO_L 8 

#define AGC2_VAL_LO (R_AGC2VAL1)|(AGC2_VAL_LO_S << 16)|(AGC2_VAL_LO_L << 24) 

/*  AGC2VAL2    */
#define R_AGC2VAL2 0x1c
/*#define F_AGC2VAL2_RESERVED_1 75*/

#define AGC2_VAL_HI_S  0 

#define AGC2_VAL_HI_L 4

#define AGC2_VAL_HI (R_AGC2VAL2)|(AGC2_VAL_HI_S << 16)|(AGC2_VAL_HI_L << 24) 
/*  AGC2PGA */
#define R_AGC2PGA 0x1d 
/*#define F_AGC2PGA_RESERVED_1 77
#define UAGC2PGA 78  */

/*  OVF_RATE1   */
#define R_OVF_RATE1 0x1e
/*#define F_OVF_RESERVED_1 79
#define OVF_RATE_HI 80 */

/*  OVF_RATE2   */
#define R_OVF_RATE2 0x1f 
/*#define OVF_RATE_LO 81*/

/*  GAIN_SRC1   */
#define R_GAIN_SRC1 0x20 

#define INV_SPECTR_S 7
#define IQ_INVERT_S 6
#define INR_BYPASS_S 5 
#define GAIN_SRC_HI_S 0

#define INV_SPECTR_L 1
#define IQ_INVERT_L 1
#define INR_BYPASS_L 1 
#define GAIN_SRC_HI_L 4

#define INV_SPECTR (R_GAIN_SRC1)|(INV_SPECTR_S << 16)|(INV_SPECTR_L << 24)
#define IQ_INVERT (R_GAIN_SRC1)|(IQ_INVERT_S << 16)|(IQ_INVERT_L << 24)
#define INR_BYPASS (R_GAIN_SRC1)|(INR_BYPASS_S << 16)|(INR_BYPASS_L << 24)
#define GAIN_SRC_HI (R_GAIN_SRC1)|(GAIN_SRC_HI_S << 16)|(GAIN_SRC_HI_L << 24)
/*  GAIN_SRC2   */
#define R_GAIN_SRC2 0x21 
#define GAIN_SRC_LO_S 0
#define GAIN_SRC_LO_L 8

#define GAIN_SRC_LO (R_GAIN_SRC2)|(GAIN_SRC_LO_S << 16)|(GAIN_SRC_LO_L << 24)
/*  INC_DEROT1  */
#define R_INC_DEROT1 0x22 
/*#define INC_DEROT_HI 88 */

/*  INC_DEROT2  */
#define R_INC_DEROT2 0x23 
/*#define INC_DEROT_LO 89 */

/*  FREESTFE_1  */


#define R_FREESTFE_1 0x26 
/*#define F_FREESTFE_1_RESERVED_1 90
#define AVERAGE_ON 91
#define DC_ADJ 92
#define SEL_LSB 93 */

/*  SYR_THR  */
#define R_SYR_THR 0x27 
/*#define SEL_SRCOUT 94 
#define F_SYR_THR_RESERVED_1 95
#define SEL_SYRTHR 96*/

/* INR	*/
#define R_INR 0x28 
/*#define INR 97 */

/*  EN_PROCESS */
#define R_EN_PROCESS 0x29 
/*#define F_EN_PROCESS_RESERVED_1 98
#define ENAB 99*/

/* SDI_SMOOTHER */
#define R_SDI_SMOOTHER 0x2a 
/*#define DIS_SMOOTH 100 
#define F_SDI_SMOOTHER_RESERVED_1 101
#define SDI_INC_SMOOTHER 102*/

/* FE_LOOP_OPEN */
#define R_FE_LOOP_OPEN 0x2b 
/*#define F_FE_LOOP_OPEN_RESERVED_1 103
#define TRL_LOOP_OP 104
#define CRL_LOOP_OP 105 */

/* EPQ  */
#define R_EPQ 0x31 
 
#define EPQ_S 0

#define EPQ_L 8

#define EPQ (R_EPQ)|(EPQ_S << 16)|(EPQ_L << 24)

#define R_EPQ2 0x32 
/*#define EPQ2 107 */

/*  COR_CTL */
#define R_COR_CTL 0x80 
/*#define F_COR_CTL_RESERVED_1 108*/

#define CORE_ACTIVE_S 5
#define HOLD_S 4 
#define CORE_STATE_CTL_S 0 

#define CORE_ACTIVE_L 1
#define HOLD_L 1 
#define CORE_STATE_CTL_L 4

#define CORE_ACTIVE (R_COR_CTL)|(CORE_ACTIVE_S << 16)|(CORE_ACTIVE_L << 24) 
#define HOLD (R_COR_CTL)|(HOLD_S << 16)|(HOLD_L << 24)
#define CORE_STATE_CTL (R_COR_CTL)|(CORE_STATE_CTL_S << 16)|(CORE_STATE_CTL_L << 24)

/*  COR_STAT    */
#define R_COR_STAT 0x81 
/*#define F_COR_STAT_RESERVED_1 112*/

#define TPS_LOCKED_S 6
#define SYR_LOCKED_COR_S 5 
#define AGC_LOCKED_STAT_S 4 
#define CORE_STATE_STAT_S 0

 
#define TPS_LOCKED_L 1
#define SYR_LOCKED_COR_L 1 
#define AGC_LOCKED_STAT_L 1 
#define CORE_STATE_STAT_L 4

#define TPS_LOCKED (R_COR_STAT)|(TPS_LOCKED_S << 16)|(TPS_LOCKED_L << 24)
#define SYR_LOCKED_COR (R_COR_STAT)|(SYR_LOCKED_COR_S << 16)|(SYR_LOCKED_COR_L << 24)
#define AGC_LOCKED_STAT (R_COR_STAT)|(AGC_LOCKED_STAT_S << 16)|(AGC_LOCKED_STAT_L << 24)
#define CORE_STATE_STAT (R_COR_STAT)|(CORE_STATE_STAT_S << 16)|(CORE_STATE_STAT_L << 24) 
/*  COR_INTEN   */
#define R_COR_INTEN 0x82 
/*#define INTEN 117
#define F_COR_INTEN_RESERVED_1 118
#define INTEN_SYR 119
#define INTEN_FFT 120 
#define INTEN_AGC 121 
#define INTEN_TPS1 122 
#define INTEN_TPS2 123 
#define INTEN_TPS3 124 */

/*  COR_INTSTAT */
#define R_COR_INTSTAT 0x83 
/*#define F_COR_INTSTAT_RESERVED_1 125
#define INTSTAT_SYR 126
#define INTSTAT_FFT 127 
#define INTSAT_AGC 128
#define INTSTAT_TPS1 129 
#define INTSTAT_TPS2 130 
#define INTSTAT_TPS3 131 */

/*  COR_MODEGUARD   */
#define R_COR_MODEGUARD 0x84 
/*#define F_COR_MODEGUARD_RESERVED_1 132*/
#define FORCE_S 3
#define MODE_S 2 
#define GUARD_S 0
 
#define FORCE_L 1
#define MODE_L 1 
#define GUARD_L 2 

#define FORCE (R_COR_MODEGUARD)|(FORCE_S << 16)|(FORCE_L << 24)
#define MODE (R_COR_MODEGUARD)|(MODE_S << 16)|(MODE_L << 24)
#define GUARD (R_COR_MODEGUARD)|(GUARD_S << 16)|(GUARD_L << 24)
/*  AGC_CTL */
#define R_AGC_CTL 0x85 
/*#define DELAY_STARTUP 136
#define AGC_LAST 137
#define AGC_GAIN 138 
#define AGC_NEG 139
#define AGC_SET 140 */

/*  AGC_MANUAL1 */

#define R_AGC_MANUAL1 0x86 
/*#define AGC_VAL_LO 141 */

/*  AGC_MANUAL2 */

#define R_AGC_MANUAL2 0x87 
/*#define F_AGC_MANUAL2_RESERVED_1 142
#define AGC_VAL_HI 143*/

/*  AGC_TARGET  */
#define R_AGC_TARGET 0x88
/*#define AGC_TARGET 144 */

/*  AGC_GAIN1   */
#define R_AGC_GAIN1 0x89 
#define AGC_GAIN_LO_S 0

#define AGC_GAIN_LO_L 8

#define AGC_GAIN_LO (R_AGC_GAIN1)|(AGC_GAIN_LO_S << 16)|(AGC_GAIN_LO_L << 24)

/*  AGC_GAIN2   */
#define R_AGC_GAIN2 0x8a 
/*#define F_AGC_GAIN2_RESERVED_1 146*/
#define AGC_LOCKED_GAIN2_S 4
#define AGC_GAIN_HI_S 0
 
#define AGC_LOCKED_GAIN2_L 1
#define AGC_GAIN_HI_L 4 

#define AGC_LOCKED_GAIN2 (R_AGC_GAIN2)|(AGC_LOCKED_GAIN2_S << 16)|(AGC_LOCKED_GAIN2_L << 24)
#define AGC_GAIN_HI (R_AGC_GAIN2)|(AGC_GAIN_HI_S << 16)|(AGC_GAIN_HI_L << 24)

/*  ITB_CTL */
#define R_ITB_CTL 0x8b 
/*#define F_ITB_CTL_RESERVED_1 149
#define ITB_INVERT 150*/

/*  ITB_FREQ1   */
#define R_ITB_FREQ1 0x8c 
/*#define ITB_FREQ_LO 151 */

/*  ITB_FREQ2   */
#define R_ITB_FREQ2 0x8d
/*#define F_ITB_FREQ2_RESERVED_1 152
#define ITB_FREQ_HI 153 */

/*  CAS_CTL */
#define R_CAS_CTL 0x8e
/*#define CCS_ENABLE 154
#define ACS_DISABLE 155 
#define DAGC_DIS 156 
#define DAGC_GAIN 157 
#define CCSMU 158*/

/*  CAS_FREQ    */
#define R_CAS_FREQ 0x8f 
/*#define CCS_FREQ 159 */

/*  CAS_DAGCGAIN    */
#define R_CAS_DAGCGAIN 0x90 
/*#define CAS_DAGC_GAIN 160 */

/*  SYR_CTL */
#define R_SYR_CTL 0x91 

#define SICTHENABLE_S 7
#define LONG_ECHO_S 3 
#define AUTO_LE_EN_S 2 
#define SYR_BYPASS_S 1 
#define SYR_TR_DIS_S 0

 
#define SICTHENABLE_L 1
#define LONG_ECHO_L 4 
#define AUTO_LE_EN_L 1
#define SYR_BYPASS_L 1 
#define SYR_TR_DIS_L 1

#define SICTHENABLE (R_SYR_CTL)|(SICTHENABLE_S << 16)|(SICTHENABLE_L << 24)
#define LONG_ECHO (R_SYR_CTL)|(LONG_ECHO_S << 16)|(LONG_ECHO_L << 24)
#define AUTO_LE_EN (R_SYR_CTL)|(AUTO_LE_EN_S << 16)|(AUTO_LE_EN_L << 24)
#define SYR_BYPASS (R_SYR_CTL)|(SYR_BYPASS_S << 16)|(SYR_BYPASS_L << 24)
#define SYR_TR_DIS (R_SYR_CTL)|(SYR_TR_DIS_S << 16)|(SYR_TR_DIS_L << 24)


/*  SYR_STAT    */
#define R_SYR_STAT 0x92
/*#define F_SYR_STAT_RESERVED_1 166*/
#define SYR_LOCKED_STAT_S 4
#define SYR_MODE_S 2 
#define SYR_GUARD_S 0 

#define SYR_LOCKED_STAT_L 1
#define SYR_MODE_L 1 
#define SYR_GUARD_L 2 

#define SYR_LOCKED_STAT (R_SYR_STAT)|(SYR_LOCKED_STAT_S << 16)|(SYR_LOCKED_STAT_L << 24)
#define SYR_MODE (R_SYR_STAT)|(SYR_MODE_S << 16)|(SYR_MODE_L << 24)
#define SYR_GUARD (R_SYR_STAT)|(SYR_GUARD_S << 16)|(SYR_GUARD_L << 24)
/*  SYR_NCO1    */
#define R_SYR_NCO1 0x93 
/*#define SYR_NCO_LO 171 */

/*  SYR_NCO2    */
#define R_SYR_NCO2 0x94 
/*#define F_SYR_NCO2_RESERVED_1 172
#define SYR_NCO_HI 173*/

/*  SYR_OFFSET1 */
#define R_SYR_OFFSET1 0x95 
/*#define SYR_OFFSET_LO 174 */

/*  SYR_OFFSET2 */
#define R_SYR_OFFSET2 0x96 
/*#define F_SYR_OFFSET2_RESERVED_1 175
#define SYR_OFFSET_HI 176*/

/*  FFT_CTL */
#define R_FFT_CTL 0x97 
/*#define F_FFT_CTL_RESERVED_1 177
#define FFT_TRIGGER 178
#define FFT_MANUAL 179
#define IFFT_MODE 180*/

/*  SCR_CTL */
#define R_SCR_CTL 0x98 
/*#define F_SCR_CTL_RESERVED_1 181
#define SCR_CPDIS 182
#define SCR_DIS 183*/

/*  PPM_CTL1    */
#define R_PPM_CTL1 0x99 
/*#define F_PPM_CTL1_RESERVED_1 184
#define PPM_MAXFREQ 185 
#define F_PPM_CTL1_RESERVED_2 186
#define PPM_SCATDIS 187
#define PPM_BYP 188*/

/*  TRL_CTL */
#define R_TRL_CTL 0x9a 

#define TRL_NOMRATE_LSB_S 7
#define TRL_TR_GAIN_S 3
#define TRL_LOOPGAIN_S 0

 
#define TRL_NOMRATE_LSB_L 1
#define TRL_TR_GAIN_L 3
#define TRL_LOOPGAIN_L 1 

#define TRL_NOMRATE_LSB (R_TRL_CTL)|(TRL_NOMRATE_LSB_S << 16)|(TRL_NOMRATE_LSB_L << 24)
#define TRL_TR_GAIN (R_TRL_CTL)|(TRL_TR_GAIN_S << 16)|(TRL_TR_GAIN_L << 24)
#define TRL_LOOPGAIN (R_TRL_CTL)|(TRL_LOOPGAIN_S << 16)|(TRL_LOOPGAIN_L << 24)

/*  TRL_NOMRATE1    */
#define R_TRL_NOMRATE1 0x9b 

#define TRL_NOMRATE_LO_S 0 
#define TRL_NOMRATE_LO_L 8 

#define TRL_NOMRATE_LO (R_TRL_NOMRATE1)|(TRL_NOMRATE_LO_S << 16)|(TRL_NOMRATE_LO_L << 24)
/*  TRL_NOMRATE2    */
#define R_TRL_NOMRATE2 0x9c 

#define TRL_NOMRATE_HI_S 0 
#define TRL_NOMRATE_HI_L 8 

#define TRL_NOMRATE_HI (R_TRL_NOMRATE2)|(TRL_NOMRATE_HI_S << 16)|(TRL_NOMRATE_HI_L << 24)

/*  TRL_TIME1   */
#define R_TRL_TIME1 0x9d
 
#define TRL_TOFFSET_LO_S 0
#define TRL_TOFFSET_LO_L 8

#define TRL_TOFFSET_LO (R_TRL_TIME1)|(TRL_TOFFSET_LO_S << 16)|(TRL_TOFFSET_LO_L << 24)

/*  TRL_TIME2   */
#define R_TRL_TIME2 0x9e
 
#define TRL_TOFFSET_HI_S 0
#define TRL_TOFFSET_HI_L 8

#define TRL_TOFFSET_HI (R_TRL_TIME2)|(TRL_TOFFSET_HI_S << 16)|(TRL_TOFFSET_HI_L << 24)

/*  CRL_CTL */
#define R_CRL_CTL 0x9f 
/*#define CRL_DIS 197
#define F_CRL_CTL_RESERVED_1 198
#define CRL_TR_GAIN 199
#define CRL_LOOPGAIN 200 */

/*  CRL_FREQ1   */
#define R_CRL_FREQ1 0xa0
/*#define CRL_FOFFSET_LO 201 */

/*  CRL_FREQ2   */
#define R_CRL_FREQ2 0xa1 
/*#define CRL_FOFFSET_HI 202 */

/*  CRL_FREQ3   */
#define R_CRL_FREQ3 0xa2 
#define SEXT_S 7
#define CRL_FOFFSET_VHI_S 0

#define SEXT_L 1
#define CRL_FOFFSET_VHI_L 7

#define SEXT (R_CRL_FREQ3)|(SEXT_S << 16)|(SEXT_L << 24)
#define CRL_FOFFSET_VHI (R_CRL_FREQ3)|(CRL_FOFFSET_VHI_S << 16)|(CRL_FOFFSET_VHI_L << 24)
/*  CHC_CTL1    */
#define R_CHC_CTL1 0xa3
/*#define MEAN_PILOT_GAIN 205 
#define MMEANP 206
#define DBADP 207
#define DNOISEN 208 
#define DCHCPRED 209 
#define CHC_INT 210*/

/*  CHC_SNR */
#define R_CHC_SNR 0xa4 
 
#define CHC_SNR_S 0 
#define CHC_SNR_L 8 
#define CHC_SNR (R_CHC_SNR)|(CHC_SNR_S << 16)|(CHC_SNR_L << 24) 
/*  BDI_CTL */
#define R_BDI_CTL 0xa5 
/*#define F_BDI_CTL_RESERVED_1 212
#define BDI_LPSEL 213
#define BDI_SERIAL 214 */

/*  DMP_CTL */
#define R_DMP_CTL 0xa6
/*#define F_DMP_CTL_RESERVED_1 215
#define DMP_SDDIS 216*/

/*  TPS_RCVD1   */
#define R_TPS_RCVD1 0xa7 
/*#define F_TPS_RCVD1_RESERVED_1 217
#define TPS_CHANGE 218
#define BCH_OK 219
#define TPS_SYNC 220 
#define F_TPS_RCVD1_RESERVED_2 221
#define TPS_FRAME 222*/

/*  TPS_RCVD2   */
#define R_TPS_RCVD2 0xa8 
/*#define F_TPS_RCVD2_RESERVED_1 223*/
#define TPS_HIERMODE_S 4
#define TPS_CONST_S 0
#define TPS_HIERMODE_L 3
#define TPS_CONST_L 2

#define TPS_HIERMODE (R_TPS_RCVD2)|(TPS_HIERMODE_S << 16)|(TPS_HIERMODE_L << 24)
#define TPS_CONST (R_TPS_RCVD2)|(TPS_CONST_S << 16)|(TPS_CONST_L << 24)

/*  TPS_RCVD3   */
#define R_TPS_RCVD3 0xa9 
/*#define F_TPS_RCVD3_RESERVED_1 227*/
#define TPS_LPCODE_S 4
#define TPS_HPCODE_S 0

#define TPS_LPCODE_L 3
#define TPS_HPCODE_L 3

#define TPS_LPCODE (R_TPS_RCVD3)|(TPS_LPCODE_S << 16)|(TPS_LPCODE_L << 24)
#define TPS_HPCODE (R_TPS_RCVD3)|(TPS_HPCODE_S << 16)|(TPS_HPCODE_L << 24)

/*  TPS_RCVD4   */
#define R_TPS_RCVD4 0xaa
/*#define F_TPS_RCVD4_RESERVED_1  231*/

#define TPS_GUARD_S 4
/* #define F_TPS_RCVD4_RESERVED_2 233*/
#define TPS_MODE_S 0

#define TPS_GUARD_L 2
#define TPS_MODE_L 2

#define TPS_GUARD (R_TPS_RCVD4)|(TPS_GUARD_S << 16)|(TPS_GUARD_L << 24)
#define TPS_MODE (R_TPS_RCVD4)|(TPS_MODE_S << 16)|(TPS_MODE_L << 24)

/*  TPS_CELLID   */
#define R_TPS_CELLID 0xab 
 
#define TPS_CELLID_S 0
#define TPS_CELLID_L 8

#define TPS_CELLID (R_TPS_CELLID)|(TPS_CELLID_S << 16)|(TPS_CELLID_L << 24)

/*  TPS_FREE2   */
#define R_TPS_FREE2 0xac 

#define F_TPS_FREE2_RESERVED_1_S 0
#define F_TPS_FREE2_RESERVED_1_L 8

#define F_TPS_FREE2_RESERVED_1 (R_TPS_FREE2)|(F_TPS_FREE2_RESERVED_1_S << 16)|(F_TPS_FREE2_RESERVED_1_L << 24)

/*  TPS_SET1    */
#define R_TPS_SET1 0xad 
/*#define F_TPS_SET1_RESERVED_1 237*/
#define TPS_SETFRAME_S 0
#define TPS_SETFRAME_L 2

#define TPS_SETFRAME (R_TPS_SET1)|(TPS_SETFRAME_S << 16)|(TPS_SETFRAME_L << 24)

/*  TPS_SET2    */
#define R_TPS_SET2 0xae 
/*#define F_TPS_SET2_RESERVED_1 239
#define TPS_SETHIERMODE 240
#define F_TPS_SET2_RESERVED_2 241
#define TPS_SETCONST 242*/

/*  TPS_SET3    */
#define R_TPS_SET3 0xaf 
/*#define F_TPS_SET3_RESERVED_1 243
#define TPS_SETLPCODE 244
#define F_TPS_SET3_RESERVED_2 245
#define TPS_SETHPCODE 246*/

/*  TPS_CTL */
#define R_TPS_CTL 0xb0 
/*#define F_TPS_CTL_RESERVED_1 247
#define TPS_IMM 248
#define TPS_BCHDIS 249 
#define TPS_UPDDIS 250 */

/*  CTL_FFTOSNUM    */
#define R_CTL_FFTOSNUM 0xb1 
/*#define SYMBOL_NUMBER 251 */

/*  TESTSELECT  */
#define R_CAR_DISP_SEL 0xb2 
/*#define F_CAR_DISP_SEL_RESERVED_1 252
#define CAR_DISP_SEL 253*/

/*  MSC_REV */
#define R_MSC_REV 0xb3 
/*#define REV_NUMBER 254*/

/*  PIR_CTL */
#define R_PIR_CTL 0xb4 
/*#define F_PIR_CTL_RESERVED_1 255*/
#define FREEZE_S 0
#define FREEZE_L 1

#define FREEZE (R_PIR_CTL)|(FREEZE_S << 16)|(FREEZE_L << 24)
/*  SNR_CARRIER1    */
#define R_SNR_CARRIER1 0xb5 
/*#define SNR_CARRIER_LO 257*/

/*  SNR_CARRIER2    */

#define R_SNR_CARRIER2 0xb6 
/*#define MEAN 258 
#define F_SNR_CARRIER2_RESERVED_1 259
#define SNR_CARRIER_HI 260 */

/*  PPM_CPAMP       */
#define R_PPM_CPAMP 0xb7 
#define PPM_CPC_S 0 
#define PPM_CPC_L 8
 
#define PPM_CPC (R_PPM_CPAMP)|(PPM_CPC_S << 16)|(PPM_CPC_L << 24)
 
/*  TSM_AP0 */
#define R_TSM_AP0 0xb8 
/*#define ADDRESS_BYTE_0 262*/ 

/*  TSM_AP1 */
#define R_TSM_AP1 0xb9 
/*#define ADDRESS_BYTE_1 263*/

/*  TSM_AP2 */
#define R_TSM_AP2 0xba 
/*#define DATA_BYTE_0 264*/
/*  TSM_AP3 */
#define R_TSM_AP3 0xbb 
/*#define DATA_BYTE_1 265 */

/*  TSM_AP4 */
#define R_TSM_AP4 0xbc 
/*#define DATA_BYTE_2 266 */

/*  TSM_AP5 */
#define R_TSM_AP5 0xbd 
/*/#define DATA_BYTE_3 267 */

/*  TSM_AP6 */
#define R_TSM_AP6 0xbe 
/*#define F_TSM_AP6_RESERVED_1 268 */


/*  TSM_AP7 */
#define R_TSM_AP7 0xbf 
/*#define MEM_SELECT_BYTE 269*/

/*  CONSTMODE   */
#define R_CONSTMODE 0xcb 
/*#define F_CONSTMODE_RESERVED_1 270
#define CAR_TYPE 271
#define IQ_RANGE 272 
#define CONST_MODE 273 */

/*  CONSTCARR1  */
#define R_CONSTCARR1 0xcc 
/*#define CONST_CARR_LO 274 */

/*  CONSTCARR2  */
#define R_CONSTCARR2 0xcd
/*#define F_CONSTCARR2_RESERVED_1 275
#define CONST_CARR_HI 276*/

/*  ICONSTEL    */
#define R_ICONSTEL 0xce

#define ICONSTEL_S 0
#define ICONSTEL_L 8

#define ICONSTEL (R_ICONSTEL)|(ICONSTEL_S << 16)|(ICONSTEL_L << 24)

/*  QCONSTEL    */
#define R_QCONSTEL 0xcf 
#define QCONSTEL_S 0
#define QCONSTEL_L 8

#define QCONSTEL (R_QCONSTEL)|(QCONSTEL_S << 16)|(QCONSTEL_L << 24)

/* AGC1RF */
#define R_AGC1RF 0xd4 

#define RF_AGC1_LEVEL_S 0
#define RF_AGC1_LEVEL_L 8

#define RF_AGC1_LEVEL (R_AGC1RF)|(RF_AGC1_LEVEL_S << 16)|(RF_AGC1_LEVEL_L << 24)

/* EN_RF_AGC1 */
#define R_EN_RF_AGC1 0xd5

#define START_ADC_S 7
#define START_ADC_L 1
/*#define F_START_ADC_RESERVED_1 281*/

#define START_ADC (R_EN_RF_AGC1)|(START_ADC_S << 16)|(START_ADC_L << 24)
/*  FECM    */
#define R_FECM 0x40
/*#define FEC_MODE 282
#define F_FECM_RESERVED_1 283
#define VIT_DIFF 284
#define SYNC 285
#define SYM 286*/

/*  VTH0    */
#define R_VTH0 0x41
/*#define F_VTH0_RESERVED_1  287*/
#define VTH0_S 0
#define VTH0_L 7

#define VTH0 (R_VTH0)|(VTH0_S << 16)|(VTH0_L << 24)
/*  VTH1    */
#define R_VTH1 0x42 
#define VTH1_S 0
#define VTH1_L 7
#define VTH1 (R_VTH1)|(VTH1_S << 16)|(VTH1_L << 24)
/*  VTH2    */
#define R_VTH2 0x43
/*#define F_VTH2_RESERVED_1 291*/
#define VTH2_S 0 
#define VTH2_L 7 
 
#define VTH2 (R_VTH2)|(VTH2_S << 16)|(VTH2_L << 24)
 
/*  VTH3    */
#define R_VTH3 0x44
/*#define F_VTH3_RESERVED_1 293*/
#define VTH3_S 0 
#define VTH3_L 7 

#define VTH3 (R_VTH3)|(VTH3_S << 16)|(VTH3_L << 24)

/*  VTH4    */
#define R_VTH4 0x45 
/* #define F_VTH4_RESERVED_1 295*/
#define VTH4_S 0 
#define VTH4_L 7 

#define VTH4 (R_VTH4)|(VTH4_S << 16)|(VTH4_L << 24)
 
/*  VTH5    */
#define R_VTH5 0x46 
/* #define F_VTH5_RESERVED_1 297*/
#define VTH5_S 0 
#define VTH5_L 7

#define VTH5 (R_VTH5)|(VTH5_S << 16)|(VTH5_L << 24)

/*  FREEVIT */
#define R_FREEVIT 0x47 
/*#define F_FREEVIT_RESERVED_1 299*/

/*  VITPROG */
#define R_VITPROG 0x49
/*#define FORCE_ROTA 300
#define F_VITPROG_RESERVED_1 301
#define MDIVIDER 302*/


/*  PR  */
#define R_PR 0x4a

/*#define F_R_PR_S 0*/ /*Complete register*/
/*#define F_R_PR_L 8
#define F_R_PR  0*/ 

#define FRAPTCR_S 7
/*#define F_PR_RESERVED_1 304*/
#define E7_8_S 5
#define E6_7_S 4 
#define E5_6_S 3 
#define E3_4_S 2 
#define E2_3_S 1 
#define E1_2_s 0

#define FRAPTCR_L 1
/*#define F_PR_RESERVED_1 304*/
#define E7_8_L 1
#define E6_7_L 1 
#define E5_6_L 1 
#define E3_4_L 1 
#define E2_3_L 1 
#define E1_2_L 1

#define FRAPTCR (R_PR)|(FRAPTCR_S << 16)|(FRAPTCR_L << 24)
#define E7_8 (R_PR)|(E7_8_S << 16)|(E7_8_L << 24)
#define E6_7 (R_PR)|(E6_7_S << 16)|(E6_7_L << 24)
#define E5_6 (R_PR)|(E5_6_S << 16)|(E5_6_L << 24)
#define E3_4 (R_PR)|(E3_4_S << 16)|(E3_4_L << 24)
#define E2_3 (R_PR)|(E2_3_S << 16)|(E2_3_L << 24)
#define E1_2 (R_PR)|(E1_2_S << 16)|(E1_2_L << 24)

/*  VSEARCH */
#define R_VSEARCH 0x4b 
 
#define PR_AUTO_S 7
#define PR_FREEZE_S 6 
#define SAMPNUM_S 4
#define TIMEOUT_S 2
#define HYSTER_S 0

#define PR_AUTO_L 1
#define PR_FREEZE_L 1 
#define SAMPNUM_L 2
#define TIMEOUT_L 2
#define HYSTER_L 2	  

#define PR_AUTO_N_PR_FREEZE_S 6
#define PR_AUTO_N_PR_FREEZE_L 2
#define PR_AUTO_N_PR_FREEZE  (R_VSEARCH)|(PR_AUTO_N_PR_FREEZE_S << 16)|(PR_AUTO_N_PR_FREEZE_L << 24)


#define PR_AUTO (R_VSEARCH)|(PR_AUTO_S << 16)|(PR_AUTO_L << 24)
#define PR_FREEZE (R_VSEARCH)|(PR_FREEZE_S << 16)|(PR_FREEZE_L << 24)
#define SAMPNUM (R_VSEARCH)|(SAMPNUM_S << 16)|(SAMPNUM_L << 24)
#define TIMEOUT (R_VSEARCH)|(TIMEOUT_S << 16)|(TIMEOUT_L << 24)
#define HYSTER (R_VSEARCH)|(HYSTER_S << 16)|(HYSTER_L << 24)

/*  RS  */
#define R_RS 0x4c

#define DEINT_ENA_S 7 
#define OUTRS_SP_S  6
#define RS_ENA_S    5
#define DESCR_ENA_S 4 
#define ERRBIT_ENA_S 3 
#define FORCE47_S 2
#define CLK_POL_S 1 
#define CLK_CFG_S 0

 
#define DEINT_ENA_L 1 
#define OUTRS_SP_L 1
#define RS_ENA_L 1
#define DESCR_ENA_L 1 
#define ERRBIT_ENA_L 1 
#define FORCE47_L 1
#define CLK_POL_L 1 
#define CLK_CFG_L 1 

#define DEINT_ENA (R_RS)|(DEINT_ENA_S << 16)|(DEINT_ENA_L << 24)
#define OUTRS_SP (R_RS)|(OUTRS_SP_S << 16)|(OUTRS_SP_L << 24) 
#define RS_ENA (R_RS)|(RS_ENA_S << 16)|(RS_ENA_L << 24)
#define DESCR_ENA (R_RS)|(DESCR_ENA_S << 16)|(DESCR_ENA_L << 24)
#define ERRBIT_ENA (R_RS)|(ERRBIT_ENA_S << 16)|(ERRBIT_ENA_L << 24)
#define FORCE47 (R_RS)|(FORCE47_S << 16)|(FORCE47_L << 24)
#define CLK_POL (R_RS)|(CLK_POL_S << 16)|(CLK_POL_L << 24)
#define CLK_CFG (R_RS)|(CLK_CFG_S << 16)|(CLK_CFG_L << 24)


#define R_RSOUT 0x4d
/*#define F_RSOUT_RESERVED_1 324  
#define ENA_STBACKEND 325
#define ENA8_LEVEL 326 */

/*  ERRCTRL1    */
#define R_ERRCTRL1 0x4e 
 
#define ERRMODE1_S 7
#define TESTERS1_S 6 
#define ERR_SOURCE1_S 4
#define RESET_CNTR1_S 2
#define NUM_EVENT1_S 0 

#define ERRMODE1_L 1
#define TESTERS1_L 1 
#define ERR_SOURCE1_L 2
#define RESET_CNTR1_L 1
#define NUM_EVENT1_L 2 

 #define ERRMODE1 (R_ERRCTRL1)|(ERRMODE1_S << 16)|(ERRMODE1_L << 24)
 #define TESTERS1 (R_ERRCTRL1)|(TESTERS1_S << 16)|(TESTERS1_L << 24)
 #define ERR_SOURCE1 (R_ERRCTRL1)|(ERR_SOURCE1_S << 16)|(ERR_SOURCE1_L << 24)
 #define RESET_CNTR1 (R_ERRCTRL1)|(RESET_CNTR1_S << 16)|(RESET_CNTR1_L << 24)
 #define NUM_EVENT1 (R_ERRCTRL1)|(NUM_EVENT1_S << 16)|(NUM_EVENT1_L << 24)

/*  ERRCNTM1    */
#define R_ERRCNTM1 0x4f 

#define ERROR_COUNT1_HI_S 0
#define ERROR_COUNT1_HI_L 8
#define ERROR_COUNT1_HI (R_ERRCNTM1)|(ERROR_COUNT1_HI_S << 16)|(ERROR_COUNT1_HI_L << 24)

/*  ERRCNTL1    */
#define R_ERRCNTL1 0x50
#define ERROR_COUNT1_LO_S 0 
#define ERROR_COUNT1_LO_L 8 

#define ERROR_COUNT1_LO (R_ERRCNTL1)|(ERROR_COUNT1_LO_S << 16)|(ERROR_COUNT1_LO_L << 24)
 
/*  ERRCTRL2    */			   
#define R_ERRCTRL2 0x51
/*#define ERRMODE2 335
#define TESTERS2 336 
#define ERR_SOURCE2 337 
#define F_ERRCTRL2_RESERVED_1 338
#define RESET_CNTR2 339
#define NUM_EVENT2 340*/

/*  ERRCNTM2    */
#define R_ERRCNTM2 0x52 
/*#define ERROR_COUNT2_HI 341 */

/*  ERRCNTL2    */
#define R_ERRCNTL2 0x53
/*#define ERROR_COUNT2_LO 342 */

/*  ERRCTRL3    */
#define R_ERRCTRL3 0x56
#define ERRMODE3_S 7
#define TESTERS3_S 6 
#define ERR_SOURCE3_S 4 
#define RESET_CNTR3_S 2
#define NUM_EVENT3_S 0

#define ERRMODE3_L 1
#define TESTERS3_L 1 
#define ERR_SOURCE3_L 2 
#define RESET_CNTR3_L 1
#define NUM_EVENT3_L 2

#define ERRMODE3 (R_ERRCTRL3)|(ERRMODE3_S << 16)|(ERRMODE3_L << 24)
#define TESTERS3 (R_ERRCTRL3)|(TESTERS3_S << 16)|(TESTERS3_L << 24)
#define ERR_SOURCE3 (R_ERRCTRL3)|(ERR_SOURCE3_S << 16)|(ERR_SOURCE3_L << 24)
#define RESET_CNTR3 (R_ERRCTRL3)|(RESET_CNTR3_S << 16)|(RESET_CNTR3_L << 24)
#define NUM_EVENT3 (R_ERRCTRL3)|(NUM_EVENT3_S << 16)|(NUM_EVENT3_L << 24)


/*  ERRCNTM3    */
#define R_ERRCNTM3 0x57 
/*#define ERROR_COUNT3_HI 349 */

/*  ERRCNTL3    */
#define R_ERRCNTL3 0x58 
/*#define ERROR_COUNT3_LO 350 */

/* DILSTK1 */

#define R_DILSTK1 0x59 
/*#define DILSTK_HI 351*/

/* DILSTK0 */

#define R_DILSTK0 0x5a 
/*#define DILSTK_LO 352*/

/* DILBWSTK1 */

#define R_DILBWSTK1 0x5b 
/*#define F_DILBWSTK1_RESERVED_1 353*/

/* DILBWSTK0 */

#define R_DILBWST0 0x5c
/*#define F_DILBWSTK0_RESERVED_1 354 */

/*  LNBRX       */
#define R_LNBRX 0x5d 
 
#define LINE_OK_S 7
#define OCCURRED_ERR_S 6

#define LINE_OK_L 1
#define OCCURRED_ERR_L 1 

#define LINE_OK (R_LNBRX)|(LINE_OK_S << 16)|(LINE_OK_L << 24)
#define OCCURRED_ERR (R_LNBRX)|(OCCURRED_ERR_S << 16)|(OCCURRED_ERR_L << 24)
 

/* RSTC */

#define R_RSTC 0x5e 
/*#define DEINNTE 358
#define DIL64_ON 359 
#define RSTC 360
#define DESCRAMTC 361 
#define F_RSTC_RESERVED_1 362
#define MODSYNCBYTE 363
#define LOWP_DIS 364
#define HI 365 */

/* VIT_BIST */
/*#define R_VIT_BIST 0x5f 
#define F_VIT_BIST_RESERVED_1 366
#define RAND_RAMP 367
#define NOISE_LEVEL 368 
#define PR_VIT_BIST 369 */

/*  FREEDRS */
#define R_FREEDRS 0x54
/*#define F_FREEDRS_RESERVED_1 370*/

/*  VERROR  */
#define R_VERROR 0x55
/*#define ERROR_VALUE 371*/

/*  TSTRES  */

#define R_TSTRES 0xc0
/*#define FRESI2C 372
#define FRESRS 373
#define FRESACS 374 
#define FRES_PRIF 375 
#define FRESFEC1_2 376 
#define FRESFEC 377
#define FRESCORE1_2 378 
#define FRESCORE 379*/

/*  ANACTRL */

#define R_ANACTRL 0xc1 
/*#define STDBY_PLL 380 
#define BYPASS_XTAL 381 
#define STDBY_PGA 382
#define TEST_PGA 383
#define STDBY_ADC 384 
#define BYPASS_ADC 385 
#define SGN_ADC 386
#define TEST_ADC 387*/

/*  TSTBUS  */

#define R_TSTBUS 0xc2
/*#define EXT_TESTIN 388 
#define EXT_ADC 389
#define TEST_IN 390 
#define TS 391*/

/*  TSTCK   */

#define R_TSTCK 0xc3
/*#define CKFECEXT 392 
#define F_TSTCK_RESERVED_1 393
#define FORCERATE1 394
#define TSTCKRS 395
#define TSTCKDIL 396 
#define DIRCKINT 397*/

/*  TSTI2C  */

#define R_TSTI2C 0xc4 
/*#define EN_VI2C 398
#define TI2C 399
#define BFAIL_BAD 400 
#define RBACT 401
#define TST_PRIF 402*/

/*  TSTRAM1 */
#define R_TSTRAM1 0xc5 
/*#define SELADR1 403
#define FSELRAM1 404 
#define FSELDEC 405
#define FOEB 406
#define FADR 407*/

/*  TSTRATE */
#define R_TSTRATE 0xc5 
/*#define FORCEPHA 408
#define F_TSTRATE_RESERVED_1 409
#define FNEWPHA 410
#define FROT90 411
#define FR 412*/

/*  SELOUT  */
#define R_SELOUT 0xc7
/*#define EN_VLOG 413 
#define SELVIT60 414 
#define SELSYN3 415
#define SELSYN2 416 
#define SELSYN1 417 
#define SELLIFO 418 
#define SELFIFO 419 
#define TSTFIFO_SELOUT 420 */

/*  FORCEIN */
#define R_FORCEIN 0xc8 
/*#define SEL_VITDATAIN 421
#define FORCE_ACS 422
#define TSTSYN 423 
#define TSTRAM64 424 
#define TSTRAM 425
#define TSTERR2 426 
#define TSTERR 427 
#define TSTACS 428 */

/*  TSTFIFO */
#define R_TSTFIFO 0xc9
/*#define F_TSTFIFO_RESERVED_1  429
#define FORMSB 430
#define FORLSB 431 
#define TSTFIFO_TSTFIFO 432 */

/*  TSTRS   */
#define R_TSTRS 0xca
/*#define TST_SCRA 433
#define OLDRS6 434
#define ADCT 435
#define DILT 436 
#define SCARBIT 437 
#define TSTRS_EN 438 */

/*  TSTBISTRES0 */
#define R_TSTBISTRES0 0xd0 
/*#define BEND_CHC2 439 
#define BBAD_CHC2 440 
#define BEND_PPM 441
#define BBAD_PPM 442 
#define BEND_FFTW 443 
#define BBAD_FFTW 444 
#define BEND_FFTI 445 
#define BBAD_FFTI 446 */

/*  TSTBISTRES1 */
#define R_TSTBISTRES1 0xd1 
/*#define BEND_CHC1 447 
#define BBAD_CHC1 448 
#define BEND_SYR 449 
#define BBAD_SYR 450 
#define BEND_SDI 451 
#define BBAD_SDI 452 
#define BEND_BDI 453 
#define BBAD_BDI 454 */

/*  TSTBISTRES2 */
#define R_TSTBISTRES2 0xd2 
/*#define BEND_VIT2 455 
#define BBAD_VIT2 456 
#define BEND_VIT1 457 
#define BBAD_VIT1 458 
#define BEND_DIL 459
#define BBAD_DIL 460 
#define BEND_RS 461
#define BBAD_RS 462 */

/*  TSTBISTRES3 */
#define R_TSTBISTRES3 d3 
/*#define F_TSTBISTRES3_RESERVED_1 463
#define BEND_FIFO 464
#define BBAD_FIFO 465  */

	/* Number of registers  */
/*#else     DEBUG_VERSION*/
#define		STV360_NBREGS	159

#define		STV360_NBFIELDS	466 


#endif  /* else (minidriver)ends */

#endif 




