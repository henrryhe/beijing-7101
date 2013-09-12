




#ifndef H_MAP361



	#define H_MAP361




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
 #define SDAT_NOD 0x10001

/* TOPCTRL */
 #define R_TOPCTRL 0x2
 #define STDBY 0x20080
 #define STDBY_FEC 0x20040
 #define STDBY_CORE 0x20020
 #define DIR_CLK 0x20010
 #define TS_DIS 0x20008
 #define DUAL_CKDIR 0x20004
 #define BYP_BUFFER 0x20002
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

/* SDFR */
 #define R_SDFR 0x8
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
 #define AUXFEC_CTL 0xa00c0
 #define DIS_CKX4 0xa0020
 #define CKSEL 0xa0018
 #define CKDIV_PROG 0xa0006
 #define AUXCLK_ENA 0xa0001

/* RESERVED_1 */
 #define R_RESERVED_1 0xb

/* RESERVED_2 */
 #define R_RESERVED_2 0xc



/* RESERVED_3 */
 #define R_RESERVED_3 0xd


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

/* AGC12C */
 #define R_AGC12C 0x16
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
 #define INVERT_AGC12 0x170040
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
 #define INS_BYPASS 0x200010
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

/* PPM_CPAMP_DIR */
 #define R_PPM_CPAMP_DIR 0x24
 #define PPM_CPAMP_DIRECT 0x2400ff

/* PPM_CPAMP_INV */
 #define R_PPM_CPAMP_INV 0x25
 #define PPM_CPAMP_INV 0x2500ff

/* FREESTFE_1 */
 #define R_FREESTFE_1 0x26
 #define SYMBOL_NUMBER_INC 0x2600c0
 #define SEL_LSB 0x260004
 #define AVERAGE_ON 0x260002
 #define DC_ADJ 0x260001


/* FREESTFE_2 */
 #define R_FREESTFE_2 0x27
 #define SEL_SRCOUT 0x2700c0
 #define SEL_SYRTHR 0x27001f


/* DCOFFSET */
 #define R_DCOFFSET 0x28
 #define DC_OFFSET 0x28007f


/* EN_PROCESS */
 #define R_EN_PROCESS 0x29
 #define INS_NIN_INDEX 0x2900f0
 #define ENAB_MANUAL 0x290001

/* RESERVED_4 */
 #define R_RESERVED_4 0x2a
 #define DIS_SMOOTH 0x2a0080
 #define SDI_INC_SMOOTHER 0x2a007f


/* RESERVED_5 */
 #define R_RESERVED_5 0x2b
 #define TRL_LOOP_OP 0x2b0002
 #define CRL_LOOP_OP 0x2b0001


/* RESERVED_6 */
 #define R_RESERVED_6 0x2c
 #define FREQ_OFFSET_LOOP_OPEN_VHI 0x2c00ff

/* RESERVED_7 */
 #define R_RESERVED_7 0x2d
 #define FREQ_OFFSET_LOOP_OPEN_HI 0x2d00ff

/* RESERVED_8 */
 #define R_RESERVED_8 0x2e
 #define FREQ_OFFSET_LOOP_OPEN_LO 0x2e00ff

/* RESERVED_9 */
 #define R_RESERVED_9 0x2f
 #define TIM_OFFSET_LOOP_OPEN_HI 0x2f00ff

/* RESERVED_10 */
 #define R_RESERVED_10 0x30
 #define TIM_OFFSET_LOOP_OPEN_LO 0x3000ff

/* EPQ */
 #define R_EPQ 0x31
 #define EPQ 0x3100ff

/* EPQAUTO */
 #define R_EPQAUTO 0x32
 #define EPQ2 0x3200ff

/* CHP_TAPS */
 #define R_CHP_TAPS 0x33
 #define SCAT_FILT_EN 0x330002
 #define TAPS_EN 0x330001


/* CHP_DYN_COEFF */
 #define R_CHP_DYN_COEFF 0x34
 #define CHP_DYNAM_COEFFCIENT 0x340080


/* PPM_STATE_MAC */
 #define R_PPM_STATE_MAC 0x35
 #define PPM_STATE_MACHINE_DECODER 0x350020



/* INR_THRESHOLD */
 #define R_INR_THRESHOLD 0x36
 #define INR_THRESHOLD 0x360080



/* COR_CTL */
 #define R_COR_CTL 0x80
 #define CORE_ACTIVE 0x800020
 #define HOLD 0x800010
 #define CORE_STATE_CTL 0x80000f


/* COR_STAT */
 #define R_COR_STAT 0x81
 #define SCATT_LOCKED 0x810080
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
 #define AGC_TIMING_FACTOR 0x8500c0
 #define AGC_LAST 0x850010
 #define AGC_GAIN 0x850008
 #define AGC_NEG 0x850004
 #define AGC_SET 0x850002



/* RESERVED_11 */
 #define R_RESERVED_11 0x86
 #define AGC_VAL_LO 0x8600ff

/* RESERVED_12 */
 #define R_RESERVED_12 0x87
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


/* RESERVED_13 */
 #define R_RESERVED_13 0x8b
 #define ITB_INVERT 0x8b0001


/* RESERVED_14 */
 #define R_RESERVED_14 0x8c
 #define ITB_FREQ_LO 0x8c00ff

/* RESERVED_15 */
 #define R_RESERVED_15 0x8d
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
 #define SICTH_ENABLE 0x910080
 #define LONG_ECHO 0x910078
 #define AUTO_LE_EN 0x910004
 #define SYR_BYPASS 0x910002
 #define SYR_TR_DIS 0x910001

/* SYR_STAT */
 #define R_SYR_STAT 0x92
 #define SYR_LOCKED_STAT 0x920010
 #define SYR_MODE 0x920004
 #define SYR_GUARD 0x920003



/* RESERVED_16 */
 #define R_RESERVED_16 0x93
 #define SYR_NCO_LO 0x9300ff

/* RESERVED_17 */
 #define R_RESERVED_17 0x94
 #define SYR_NCO_HI 0x94003f


/* SYR_OFFSET1 */
 #define R_SYR_OFFSET1 0x95
 #define SYR_OFFSET_LO 0x9500ff

/* SYR_OFFSET2 */
 #define R_SYR_OFFSET2 0x96
 #define SYR_OFFSET_HI 0x96003f


/* RESERVED_18 */
 #define R_RESERVED_18 0x97
 #define SHIFT_FFT_TRIG 0x970018
 #define FFT_TRIGGER 0x970004
 #define FFT_MANUAL 0x970002
 #define IFFT_MODE 0x970001


/* SCR_CTL */
 #define R_SCR_CTL 0x98
 #define SYRADJDECAY 0x980070
 #define SCR_CPEDIS 0x980002
 #define SCR_DIS 0x980001



/* PPM_CTL1 */
 #define R_PPM_CTL1 0x99
 #define MEAN_OFF 0x990080
 #define GRAD_OFF 0x990040
 #define PPM_MAXFREQ 0x990030
 #define PPM_MAXTIM 0x990008
 #define PPM_INVSEL 0x990004
 #define PPM_SCATDIS 0x990002
 #define PPM_BYP 0x990001

/* TRL_CTL */
 #define R_TRL_CTL 0x9a
 #define TRL_NOMRATE_LSB 0x9a0080
 #define TRL_GAIN_FACTOR 0x9a0078
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
 #define CRL_GAIN_FACTOR 0x9f0078
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
 #define MANMEANP 0xa30010
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
 #define DMP_SCALING_FACTOR 0xa6001e
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



/* TPS_ID_CELL1 */
 #define R_TPS_ID_CELL1 0xab
 #define TPS_ID_CELL_LO 0xab00ff

/* TPS_ID_CELL2 */
 #define R_TPS_ID_CELL2 0xac
 #define TPS_ID_CELL_HI 0xac00ff

/* RESERVED_19 */
 #define R_RESERVED_19 0xad
 #define TPS_NA 0xad00fc
 #define TPS_SETFRAME 0xad0003

/* RESERVED_20 */
 #define R_RESERVED_20 0xae
 #define TPS_SETHIERMODE 0xae0070
 #define TPS_SETCONST 0xae0003



/* RESERVED_21 */
 #define R_RESERVED_21 0xaf
 #define TPS_SETLPCODE 0xaf0070
 #define TPS_SETHPCODE 0xaf0007



/* TPS_CTL */
 #define R_TPS_CTL 0xb0
 #define TPS_IMM 0xb00004
 #define TPS_BCHDIS 0xb00002
 #define TPS_UPDDIS 0xb00001


/* CTL_FFTOSNUM */
 #define R_CTL_FFTOSNUM 0xb1
 #define SYMBOL_NUMBER 0xb1007f


/* TESTSELECT */
 #define R_TESTSELECT 0xb2
 #define TESTSELECT 0xb2001f


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


/* RESERVED_22 */
 #define R_RESERVED_22 0xb7
 #define PPM_CPC 0xb700ff

/* RESERVED_23 */
 #define R_RESERVED_23 0xb8
 #define ADDRESS_BYTE_0 0xb800ff

/* RESERVED_24 */
 #define R_RESERVED_24 0xb9
 #define ADDRESS_BYTE_1 0xb900ff

/* RESERVED_25 */
 #define R_RESERVED_25 0xba
 #define DATA_BYTE_0 0xba00ff

/* RESERVED_26 */
 #define R_RESERVED_26 0xbb
 #define DATA_BYTE_1 0xbb00ff

/* RESERVED_27 */
 #define R_RESERVED_27 0xbc
 #define DATA_BYTE_2 0xbc00ff

/* RESERVED_28 */
 #define R_RESERVED_28 0xbd
 #define DATA_BYTE_3 0xbd00ff

/* RESERVED_29 */
 #define R_RESERVED_29 0xbe


/* RESERVED_30 */
 #define R_RESERVED_30 0xbf
 #define MEM_SELECT_BYTE 0xbf00ff


/* RESERVED_31 */
 #define R_RESERVED_31 0x40
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


/* RESERVED_32 */
 #define R_RESERVED_32 0x45
 #define VTH4 0x45007f


/* VTH5 */
 #define R_VTH5 0x46
 #define VTH5 0x46007f


/* RESERVED_33 */
 #define R_RESERVED_33 0x47


/* VITPROG */
 #define R_VITPROG 0x49
 #define FORCE_ROTA 0x4900c0
 #define AUTO_FREEZE 0x490030
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

/* RESERVED_34 */
 #define R_RESERVED_34 0x54


/* VERROR */
 #define R_VERROR 0x55
 #define ERROR_VALUE 0x5500ff

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

/* RESERVED_35 */
 #define R_RESERVED_35 0x59
 #define DILSTK_HI 0x5900ff

/* RESERVED_36 */
 #define R_RESERVED_36 0x5a
 #define DILSTK_LO 0x5a00ff

/* RESERVED_37 */
 #define R_RESERVED_37 0x5b


/* RESERVED_38 */
 #define R_RESERVED_38 0x5c


/* LNBRX */
 #define R_LNBRX 0x5d
 #define LINE_OK 0x5d0080
 #define OCCURRED_ERR 0x5d0040
 #define RSOV_DATAIN 0x5d0008
 #define LNBTX_CHIPADDR 0x5d0007


/* RESERVED_39 */
 #define R_RESERVED_39 0x5e
 #define DEINTTC 0x5e0080
 #define DIL64_ON 0x5e0040
 #define RSTC 0x5e0020
 #define DESCRAMTC 0x5e0010
 #define MODSYNCBYT 0x5e0004
 #define LOWP_DIS 0x5e0002
 #define HIGHP_DIS 0x5e0001


/* RESERVED_40 */
 #define R_RESERVED_40 0x5f
 #define RAND_RAMP 0x5f0040
 #define NOISE_LEVEL 0x5f0038
 #define PR_VIT_BIST 0x5f0007


/* RESERVED_41 */
 #define R_RESERVED_41 0xc0
 #define FRES_DISPLAY 0xc00080
 #define FRESRS 0xc00040
 #define FRESACS 0xc00020
 #define FRES_PRIF 0xc00010
 #define FRESFEC1_2 0xc00008
 #define FRESFEC 0xc00004
 #define FRESCORE1_2 0xc00002
 #define FRESCORE 0xc00001

/* ANACTRL */
 #define R_ANACTRL 0xc1
 #define STDBY_PLL2X4 0xc10080
 #define BYPASS_XTAL 0xc10040
 #define STDBY_BUFFER 0xc10020
 #define STDBY_BIAS 0xc10010
 #define STDBY_ADPIPE 0xc10008
 #define BYPASS_PLL 0xc10004
 #define DIS_PAD_OSC 0xc10002
 #define STDBY_PLLXN 0xc10001

/* RESERVED_42 */
 #define R_RESERVED_42 0xc2
 #define EXT_TESTIN 0xc20080
 #define EXT_ADC 0xc20040
 #define TEST_IN 0xc20038
 #define TS 0xc20007

/* RESERVED_43 */
 #define R_RESERVED_43 0xc3
 #define CKFECEXT 0xc30080
 #define FORCERATE1 0xc30008
 #define TSTCKRS 0xc30004
 #define TSTCKDIL 0xc30002
 #define DIRCKINT 0xc30001


/* RESERVED_44 */
 #define R_RESERVED_44 0xc4
 #define EN_VI2C 0xc40080
 #define TI2C 0xc40060
 #define BFAIL_BAD 0xc40010
 #define TST_PRIF 0xc40007


/* RESERVED_45 */
 #define R_RESERVED_45 0xc5
 #define SELADR1 0xc50080
 #define FSELRAM1 0xc50040
 #define FSELDEC 0xc50020
 #define FOEB 0xc5001c
 #define FADR 0xc50003

/* RESERVED_46 */
 #define R_RESERVED_46 0xc6
 #define FORCEPHA 0xc60080
 #define FNEWPHA 0xc60010
 #define FROT90 0xc60008
 #define FR 0xc60007


/* RESERVED_47 */
 #define R_RESERVED_47 0xc7
 #define EN_VLOG 0xc70080
 #define SELVIT60 0xc70040
 #define SELSYN3 0xc70020
 #define SELSYN2 0xc70010
 #define SELSYN1 0xc70008
 #define SELLIFO 0xc70004
 #define SELFIFO 0xc70002
 #define TSTFIFO 0xc70001

/* RESERVED_48 */
 #define R_RESERVED_48 0xc8
 #define SEL_VITDATAIN 0xc80080
 #define FORCE_ACS 0xc80040
 #define TSTSYN 0xc80020
 #define TSTRAM64 0xc80010
 #define TSTRAM 0xc80008
 #define TSTERR2 0xc80004
 #define TSTERR 0xc80002
 #define TSTACS 0xc80001

/* RESERVED_49 */
 #define R_RESERVED_49 0xc9
 #define FORMSB 0xc90004
 #define FORLSB 0xc90002
 #define TSTFIFO_TSTFIFO 0xc90001


/* RESERVED_50 */
 #define R_RESERVED_50 0xca
 #define TST_SCRA 0xca0080
 #define OLDRS6 0xca0040
 #define ADCT 0xca0030
 #define DILT 0xca000c
 #define SCARBIT 0xca0002
 #define TSTRS_EN 0xca0001

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
 #define ICONSTEL 0xce00ff

/* QCONSTEL */
 #define R_QCONSTEL 0xcf
 #define QCONSTEL 0xcf00ff

/* RESERVED_51 */
 #define R_RESERVED_51 0xd0
 #define BEND_BDI 0xd00080
 #define BBAD_BDI 0xd00040
 #define BEND_PPM 0xd00020
 #define BBAD_PPM 0xd00010
 #define BEND_SDI 0xd00008
 #define BBAD_SDI 0xd00004
 #define BEND_INS 0xd00002
 #define BBAD_INS 0xd00001

/* RESERVED_52 */
 #define R_RESERVED_52 0xd1
 #define BEND_CHC2B 0xd10080
 #define BBAD_CHC2B 0xd10040
 #define BEND_CHC3 0xd10020
 #define BBAD_CHC3 0xd10010
 #define BEND_FFTI 0xd10008
 #define BBAD_FFTI 0xd10004
 #define BEND_FFTW 0xd10002
 #define BBAD_FFTW 0xd10001

/* RESERVED_53 */
 #define R_RESERVED_53 0xd2
 #define BEND_RS 0xd20080
 #define BBAD_RS 0xd20040
 #define BEND_SYR 0xd20020
 #define BBAD_SYR 0xd20010
 #define BEND_CHC1 0xd20008
 #define BBAD_CHC1 0xd20004
 #define BEND_CHC2 0xd20002
 #define BBAD_CHC2 0xd20001

/* RESERVED_54 */
 #define R_RESERVED_54 0xd3
 #define BEND_FIFO 0xd30080
 #define BBAD_FIFO 0xd30040
 #define BEND_VIT2 0xd30020
 #define BBAD_VIT2 0xd30010
 #define BEND_VIT1 0xd30008
 #define BBAD_VIT1 0xd30004
 #define BEND_DIL 0xd30002
 #define BBAD_DIL 0xd30001

/* RF_AGC1 */
 #define R_RF_AGC1 0xd4
 #define RF_AGC1_LEVEL_HI 0xd400ff

/* RF_AGC2 */
 #define R_RF_AGC2 0xd5
 #define START_ADC 0xd50080
 #define TEST_ADCGP 0xd50040
 #define STDBY_ADCGP 0xd50020
 #define ADCGP_OVF 0xd50004
 #define RF_AGC1_LEVEL_LO 0xd50003


/* RESERVED_55 */
 #define R_RESERVED_55 0xd6
 #define BUSIN5 0xd600ff

/* ANACTRL2 */
 #define R_ANACTRL2 0xdb
 #define ANA_CTRL2_BUS 0xdb00ff

/* PLLMDIV */
 #define R_PLLMDIV 0xd8
 #define PLL_MDIV 0xd800ff

/* PLLNDIV */
 #define R_PLLNDIV 0xd9
 #define PLL_NDIV 0xd900ff

/* PLLSETUP */
 #define R_PLLSETUP 0xda
 #define PLL_PDIV 0xda0070
 #define PLL_SETUP 0xda000f


/* ANADIGCTRL */
 #define R_ANADIGCTRL 0xd7
 #define BUFFER_INCM 0xd700c0
 #define SEL_CLKDEM 0xd70020
 #define SEL_PLL 0xd70010
 #define BYPASS_ADC 0xd70008
 #define ADC_RIS_EGDE 0xd70004
 #define SGN_ADC 0xd70002
 #define TEST_ADC 0xd70001

/* TSTBIST */
 #define R_TSTBIST 0xdc
 #define TST_COLLAR_W 0xdc0020
 #define TST_COLLAR_CS 0xdc0010
 #define TST_COLLAR 0xdc0008
 #define TST_BIST_MODE 0xdc0006
 #define RBACT 0xdc0001






#define		STV361_NBREGS	177 

#define		STV361_NBFIELDS	524 
#endif 

