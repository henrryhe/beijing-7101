/* -------------------------------------------------------------------------
File Name: 399_init.h

Description: 399 register mappings.

Copyright (C) 1999-2001 STMicroelectronics

History:
   date: 10-October-2001
version: 1.0.0
 author: SA
comment: STAPIfied by GP

---------------------------------------------------------------------------- */


/* define to prevent recursive inclusion */
#ifndef H_399INIT
 #define H_399INIT


 /* common includes ---------------------------------------------------------------- */
#ifndef STTUNER_REG_INIT_OLD_METHOD /* this option will be removed later & only the new method will be left in file*/



/* ID */
 #define R399_ID 0x0
 #define F399_CHIP_IDENT 0xf0
 #define F399_RELEASE 0xf

/* I2CRPT */
 #define R399_I2CRPT 0x1
 #define F399_I2CT_ON 0x10080
 #define F399_ENARPT_LEVEL 0x10070
 #define F399_SCLT_DELAY 0x10008
 #define F399_SCLT_VALUE 0x10004
 #define F399_STOP_ENABLE 0x10002
 #define F399_SDAT_VALUE 0x10001

/* ACR */
 #define R399_ACR 0x2
 #define F399_PRESCALER 0x200e0
 #define F399_DIVIDER 0x2001f

/* F22FR */
 #define R399_F22FR 0x3
 #define F399_F_REG 0x300ff

/* DACR1 */
 #define R399_DACR1 0x4
 #define F399_DACMODE 0x400e0
 #define F399_DACR1_4 0x40010
 #define F399_DACMSB 0x4000f

/* DACR2 */
 #define R399_DACR2 0x5
 #define F399_DACLSB 0x500ff

/* DISEQC */
 #define R399_DISEQC 0x6
 #define F399_TIM_OFF 0x60080
 #define F399_DISEQC_6 0x60040
 #define F399_TIM_CMD 0x60030
 #define F399_CMD_ENV 0x60008
 #define F399_DISEQC 0x60004
 #define F399_DISEQCMODE 0x60003

/* DISEQCFIFO */
 #define R399_DISEQCFIFO 0x7
 #define F399_DISEQCFIFO1 0x700ff

/* DISEQCSTATUS */
 #define R399_DISEQCSTATUS 0x8
 #define F399_IP0 0x80080
 #define F399_ATT 0x80040
 #define F399_T5 0x80020
 #define F399_T4 0x80010
 #define F399_T3 0x80008
 #define F399_TIM 0x80004
 #define F399_FE 0x80002
 #define F399_FF 0x80001

/* DISEQC2 */
 #define R399_DISEQC2 0x9
 #define F399_DISEQC2_7 0x90080
 #define F399_DISEQC2_6 0x90040
 #define F399_DISEQC2_5 0x90020
 #define F399_DISEQC2_4 0x90010
 #define F399_DISEQC2_3 0x90008
 #define F399_DISEQC2_2 0x90004
 #define F399_DISEQC2_1 0x90002
 #define F399_DISEQC2_0 0x90001

/* IOCFG1 */
 #define R399_IOCFG1 0xa
 #define F399_LOCK_CONF 0xa00e0
 #define F399_LOCK_OPDRAIN 0xa0010
 #define F399_SERIAL_D0 0xa0008
 #define F399_TURBO_OUTMODE 0xa0007

/* IOCFG2 */
 #define R399_IOCFG2 0xb
 #define F399_OP1_OPDRAIN 0xb0080
 #define F399_OP1_1 0xb0040
 #define F399_OP0_OPDRAIN 0xb0020
 #define F399_OP0_1 0xb0010
 #define F399_DCHIP_ADDR 0xb000c
 #define F399_DIFF_MODE 0xb0002
 #define F399_OUTRS_HZ 0xb0001

/* AGC0C */
 #define R399_AGC0C 0xc
 #define F399_PROG0 0xc00f8
 #define F399_BETA0 0xc0007

/* AGC0R */
 #define R399_AGC0R 0xd
 #define F399_REF_AGC0 0xd00fc
 #define F399_LUT_ORD 0xd0002
 #define F399_DC0_ADJ 0xd0001

/* AGC1C */
 #define R399_AGC1C 0xe
 #define F399_DC1_ADJ 0xe0080
 #define F399_DBLE_AGC 0xe0040
 #define F399_AGC1_B2_EN 0xe0020
 #define F399_BETA2 0xe0018
 #define F399_AGC1C_BETA1 0xe0007

/* AGC1CN */
 #define R399_AGC1CN 0xf
 #define F399_AVERAGE_ON 0xf0080
 #define F399_SEL_AVERAGE_ADJ 0xf0040
 #define F399_AGC2GAIN 0xf0030
 #define F399_SEL_ADCOUT 0xf0008

/* RTC */
 #define R399_RTC 0x10
 #define F399_RTC_7 0x100080
 #define F399_ALPHA_TMG 0x100070
 #define F399_RTC_3 0x100008
 #define F399_BETA_TMG 0x100007

/* AGC1R */
 #define R399_AGC1R 0x11
 #define F399_IAGC1R 0x110080
 #define F399_AGC_BEF_DC 0x110040
 #define F399_AGC1R_REF 0x11003f

/* AGC1RN */
 #define R399_AGC1RN 0x12
 #define F399_IAGC1RN 0x120080
 #define F399_OPEN_DRAIN 0x120040
 #define F399_AGC1RN_REF 0x12003f

/* AGC2O */
 #define R399_AGC2O 0x13
 #define F399_AGC2COEFF 0x1300e0
 #define F399_AGC2_REF 0x13001f

/* TLSR */
 #define R399_TLSR 0x14
 #define F399_STEP_MINUS 0x1400f0
 #define F399_STEP_PLUS 0x14000f

/* CFD */
 #define R399_CFD 0x15
 #define F399_CFD_ON 0x150080
 #define F399_BETA_FC 0x150070
 #define F399_FDCT 0x15000c
 #define F399_LDL 0x150003

/* ACLC */
 #define R399_ACLC 0x16
 #define F399_DEROT_ON_OFF 0x160080
 #define F399_ACLC 0x160040
 #define F399_NOISE 0x160030
 #define F399_ALPHA 0x16000f

/* BCLC */
 #define R399_BCLC 0x17
 #define F399_ALGO 0x1700c0
 #define F399_BETA 0x17003f

/* R8PSK */
 #define R399_R8PSK 0x18
 #define F399_MODE_8PSK 0x180080
 #define F399_EGAL_ON 0x180040
 #define F399_OUT_IQ_8PSK 0x180020
 #define F399_R8PSK_4 0x180010
 #define F399_MODE_COEF 0x180008
 #define F399_MU_8PSK 0x180007

/* LDT */
 #define R399_LDT 0x19
 #define F399_LOCK_THRESHOLD 0x1901ff

/* LDT2 */
 #define R399_LDT2 0x1a
 #define F399_LOCK_THRESHOLD2 0x1a01ff

/* AGC0CMD */
 #define R399_AGC0CMD 0x1b
 #define F399_LOCK0_CMD 0x1b00c0
 #define F399_LOCK0_INFO 0x1b0020
 #define F399_AUTO_SPLIT 0x1b0010
 #define F399_AMPLI6DB 0x1b0008
 #define F399_AGC0CMD_2 0x1b0004
 #define F399_AGC0CMD_1 0x1b0002
 #define F399_AGC0CMD_0 0x1b0001

/* AGC0I */
 #define R399_AGC0I 0x1c
 #define F399_AGC0I_7 0x1c0080
 #define F399_AGC0I_6 0x1c0040
 #define F399_AGC0I_5 0x1c0020
 #define F399_AGC0I_4 0x1c0010
 #define F399_AGC0_INT 0x1c000f

/* AGC1S */
 #define R399_AGC1S 0x1d
 #define F399_AGC1_INT_SEC 0x1d01ff

/* AGC1P */
 #define R399_AGC1P 0x1e
 #define F399_AGC1_INT_PRIM 0x1e01ff

/* AGC1IN */
 #define R399_AGC1IN 0x1f
 #define F399_AGC1_VALUE 0x1f00ff

/* TLIR */
 #define R399_TLIR 0x20
 #define F399_TMG_LOCK_IND 0x2000ff

/* AGC2I1 */
 #define R399_AGC2I1 0x21
 #define F399_AGC2_INTEGRATOR_MSB 0x2100ff

/* AGC2I2 */
 #define R399_AGC2I2 0x22
 #define F399_AGC2_INTEGRATOR_LSB 0x2200ff

/* RTF */
 #define R399_RTF 0x23
 #define F399_TIMING_LOOP_FREQ 0x2301ff

/* VSTATUS */
 #define R399_VSTATUS 0x24
 #define F399_CF 0x240080
 #define F399_VSTATUS_6 0x240040
 #define F399_VSTATUS_5 0x240020
 #define F399_PRF 0x240010
 #define F399_LK 0x240008
 #define F399_PR 0x240007

/* LDI */
 #define R399_LDI 0x25
 #define F399_LOCK_DET_INTEGR 0x2501ff

/* ECNTM */
 #define R399_ECNTM 0x26
 #define F399_ERROR_COUNT_MSB 0x2600ff

/* ECNTL */
 #define R399_ECNTL 0x27
 #define F399_ERROR_COUNT_LSB 0x2700ff

/* SFRH */
 #define R399_SFRH 0x28
 #define F399_SYMB_FREQ_HSB 0x2800ff

/* SFRM */
 #define R399_SFRM 0x29
 #define F399_SYMB_FREQ_MSB 0x2900ff

/* SFRL */
 #define R399_SFRL 0x2a
 #define F399_SYMB_FREQ_LSB 0x2a00f0

/* CFRM */
 #define R399_CFRM 0x2b
 #define F399_CARRIER_FREQUENCY_MSB 0x2b00ff

/* CFRL */
 #define R399_CFRL 0x2c
 #define F399_CARRIER_FREQUENCY_LSB 0x2c00ff

/* NIRM */
 #define R399_NIRM 0x2d
 #define F399_NOISE_IND_MSB 0x2d00ff

/* NIRL */
 #define R399_NIRL 0x2e
 #define F399_NOISE_IND_LSB 0x2e00ff

/* VERROR */
 #define R399_VERROR 0x2f
 #define F399_ERROR_VAL 0x2f00ff

/* FECM */
 #define R399_FECM 0x30
 #define F399_FECMODE 0x3000f0
 #define F399_FECM3 0x300008
 #define F399_VIT_DIFF 0x300004
 #define F399_SYNC 0x300002
 #define F399_SYM 0x300001

/* VTH0 */
 #define R399_VTH0 0x31
 #define F399_VTH0 0x31007f

/* VTH1 */
 #define R399_VTH1 0x32
 #define F399_VTH1 0x32007f

/* VTH2 */
 #define R399_VTH2 0x33
 #define F399_VTH2 0x33007f

/* VTH3 */
 #define R399_VTH3 0x34
 #define F399_VTH3 0x34007f

/* VTH4 */
 #define R399_VTH4 0x35
 #define F399_VTH4 0x35007f

/* VTH5 */
 #define R399_VTH5 0x36
 #define F399_VTH5 0x36007f

/* PR */
 #define R399_PR 0x37
 #define F399_E7 0x370080
 #define F399_E6 0x370040
 #define F399_PR_7_8 0x370020
 #define F399_PR_6_7 0x370010
 #define F399_PR_5_6 0x370008
 #define F399_PR_3_4 0x370004
 #define F399_PR_2_3 0x370002
 #define F399_PR_1_2 0x370001

/* VAVSRCH */
 #define R399_VAVSRCH 0x38
 #define F399_AM 0x380080
 #define F399_F 0x380040
 #define F399_SN 0x380030
 #define F399_TO 0x38000c
 #define F399_H 0x380003

/* RS */
 #define R399_RS 0x39
 #define F399_DEINT 0x390080
 #define F399_OUTRS_PS 0x390040
 #define F399_RS 0x390020
 #define F399_DESCRAM 0x390010
 #define F399_ERR_BIT 0x390008
 #define F399_MPEG 0x390004
 #define F399_CLK_POL 0x390002
 #define F399_CLK_CFG 0x390001

/* RSOUT */
 #define R399_RSOUT 0x3a
 #define F399_INV_DVALID 0x3a0080
 #define F399_INV_DSTART 0x3a0040
 #define F399_INV_DERROR 0x3a0020
 #define F399_EN_STBACKEND 0x3a0010
 #define F399_ENA8_LEVEL 0x3a000f

/* ERRCTRL */
 #define R399_ERRCTRL 0x3b
 #define F399_ERRMODE 0x3b0080
 #define F399_TSTERS 0x3b0040
 #define F399_ERR_SOURCE 0x3b0030
 #define F399_ECOL3 0x3b0008
 #define F399_RESET_CNT 0x3b0004
 #define F399_NOE 0x3b0003

/* VITPROG */
 #define R399_VITPROG 0x3c
 #define F399_VITPROG_7 0x3c0080
 #define F399_VITPROG_6 0x3c0040
 #define F399_VITPROG_5 0x3c0020
 #define F399_VITPROG_4 0x3c0010
 #define F399_VITPROG_3 0x3c0008
 #define F399_VITPROG_2 0x3c0004
 #define F399_MDIVIDER 0x3c0003

/* ERRCTRL2 */
 #define R399_ERRCTRL2 0x3d
 #define F399_ERRMODE2 0x3d0080
 #define F399_TSTERS2 0x3d0040
 #define F399_ERR_SOURCE2 0x3d0030
 #define F399_ECOL3_2 0x3d0008
 #define F399_RESET_CNT2 0x3d0004
 #define F399_NOE2 0x3d0003

/* ECNTM2 */
 #define R399_ECNTM2 0x3e
 #define F399_ERROR_COUNT2_MSB 0x3e00ff

/* ECNTL2 */
 #define R399_ECNTL2 0x3f
 #define F399_ERROR_COUNT2_LSB 0x3f00ff

/* DCLK1 */
 #define R399_DCLK1 0x40
 #define F399_CMD_DIV_DIG 0x4000e0
 #define F399_PLL_DIV 0x40001f

/* LPF */
 #define R399_LPF 0x41
 #define F399_DC_3 0x410080
 #define F399_FILTER 0x41003f

/* DCLK2 */
 #define R399_DCLK2 0x42
 #define F399_CMD_DIV 0x4200ff

/* ACOARSE */
 #define R399_ACOARSE 0x43
 #define F399_MD 0x43011f

/* AFINEMSB */
 #define R399_AFINEMSB 0x44
 #define F399_PE_MSB 0x4400ff

/* AFINELSB */
 #define R399_AFINELSB 0x45
 #define F399_PE_LSB 0x4500ff

/* SYNTCTRL */
 #define R399_SYNTCTRL 0x46
 #define F399_STANDBY 0x460080
 #define F399_BYP 0x460040
 #define F399_DIV 0x460030
 #define F399_DIS 0x460008

/* SYNTCTRL2 */
 #define R399_SYNTCTRL2 0x47
 #define F399_CMD_MINUS 0x470020
 #define F399_CMD_PLUS 0x470010
 #define F399_PLL_FACTOR 0x470008
 #define F399_RESET_FILT 0x470004
 #define F399_RESET_SYNT 0x470002
 #define F399_RESET_PLL 0x470001

/* SYSCTRL */
 #define R399_SYSCTRL 0x48
 #define F399_EXT_ATT 0x480080
 #define F399_VHIGH 0x480020
 #define F399_CMD 0x480008
 #define F399_LPF_OLD 0x480004
 #define F399_VLOW 0x480002
 #define F399_CONT_LT 0x480001

/* AGC1EP */
 #define R399_AGC1EP 0x49
 #define F399_LOCK0_INFO_PRIM 0x490080
 #define F399_AGC1_ERROR_PRIM 0x49017f

/* AGC1ES */
 #define R399_AGC1ES 0x4a
 #define F399_LOCK0_INFO_SEC 0x4a0080
 #define F399_AGC1_ERROR_SEC 0x4a017f

/* TSTDCADJ */
 #define R399_TSTDCADJ 0x80
 #define F399_TSTDCADJ_7 0x800080
 #define F399_TSTDCADJ_6 0x800040
 #define F399_TSTDCADJ_5 0x800020
 #define F399_ADJTST 0x800018
 #define F399_SEL_AVERAGE 0x800004
 #define F399_DCADJ_Q 0x800002
 #define F399_DCADJ_I 0x800001

/* TAGC1 */
 #define R399_TAGC1 0x81
 #define F399_EN_AGC1 0x810080
 #define F399_SEC_AGC 0x810040
 #define F399_EN_AGC1OUT 0x810020
 #define F399_SEL_CLOCK 0x810010
 #define F399_EN_AGC1_MOD 0x810008
 #define F399_EN_AGC1_BETA 0x810004
 #define F399_EN_AGC1_PLF 0x810002
 #define F399_SEL_AGC1_TST 0x810001

/* TAGC1N */
 #define R399_TAGC1N 0x82
 #define F399_EN_AGC1N 0x820080
 #define F399_TAGC1N_6 0x820040
 #define F399_TAGC1N_5 0x820020
 #define F399_AGC_PWM 0x820010
 #define F399_EN_AGC1N_MOD 0x820008
 #define F399_EN_AGC1N_BETA 0x820004
 #define F399_EN_AGC1N_PLF 0x820002
 #define F399_SEL_AGC1N_TST 0x820001

/* TPOLYPH */
 #define R399_TPOLYPH 0x83
 #define F399_PPH_ITEST 0x830080
 #define F399_PPH_QTEST 0x830040
 #define F399_SEL_PPH 0x830020
 #define F399_SYMENA_BYP 0x830010
 #define F399_SEL_MULT 0x830008
 #define F399_EN_MULT 0x830004
 #define F399_EN_BYPASS 0x830002
 #define F399_SEL_CPT 0x830001

/* TSTR */
 #define R399_TSTR 0x84
 #define F399_EN_STRTST 0x840080
 #define F399_TSTR_6 0x840040
 #define F399_TSTR_5 0x840020
 #define F399_EN_FRAC 0x840010
 #define F399_EN_STR_PLF 0x840008
 #define F399_EN_STR_ERR 0x840004
 #define F399_EN_STR_GPD 0x840002
 #define F399_SEL_STR_TST 0x840001

/* TAGC2 */
 #define R399_TAGC2 0x85
 #define F399_EN_AGC2 0x850080
 #define F399_TAGC2_6 0x850040
 #define F399_EN_AGC2M 0x850020
 #define F399_EN_AGC2AC 0x850010
 #define F399_EN_AGC2_OUT 0x850008
 #define F399_TAGC2_2 0x850004
 #define F399_TAGC2_1 0x850002
 #define F399_TAGC2_0 0x850001

/* TCTL1 */
 #define R399_TCTL1 0x86
 #define F399_SEL_COR 0x860080
 #define F399_EN_CTL_NCO 0x860040
 #define F399_EN_CTL_PLF 0x860020
 #define F399_EN_CTL_CLM 0x860010
 #define F399_EN_CTL_DEROT 0x860008
 #define F399_TCTL1_2 0x860004
 #define F399_SEL_TETA 0x860002
 #define F399_SEL_CTL_PLF 0x860001

/* TCTL2 */
 #define R399_TCTL2 0x87
 #define F399_EN_CTLTST 0x870080
 #define F399_EN_CTL_LOC_IND 0x870040
 #define F399_EN_CLT_POLY 0x870020
 #define F399_EN_CTL_NOS 0x870010
 #define F399_EN_CTL_NOS_IND 0x870008
 #define F399_EN_CTL_LOC_COS 0x870004
 #define F399_TCTL2_1 0x870002
 #define F399_TCTL2_0 0x870001

/* TSTRAM1 */
 #define R399_TSTRAM1 0x88
 #define F399_SELADR1 0x880080
 #define F399_FSELRAM1 0x880040
 #define F399_FSELDEC 0x880020
 #define F399_FOEB 0x88001c
 #define F399_FADR 0x880003

/* TSTRATE */
 #define R399_TSTRATE 0x89
 #define F399_FORCEPHA 0x890080
 #define F399_TSTRATE_6 0x890040
 #define F399_TSTRATE_5 0x890020
 #define F399_FNEWPHA 0x890010
 #define F399_FROTA90 0x890008
 #define F399_FOFF 0x890004
 #define F399_FR1 0x890002
 #define F399_FR0 0x890001

/* SELOUT */
 #define R399_SELOUT 0x8a
 #define F399_EN_VLOG 0x8a0080
 #define F399_SELVIT60 0x8a0040
 #define F399_SELSYN3 0x8a0020
 #define F399_SELSYN2 0x8a0010
 #define F399_SELSYN1 0x8a0008
 #define F399_SELLIFO 0x8a0004
 #define F399_SELFIFO 0x8a0002
 #define F399_SELERR 0x8a0001

/* FORCEIN */
 #define R399_FORCEIN 0x8b
 #define F399_SEL_VITDATAIN 0x8b0080
 #define F399_FORCE_ACS 0x8b0040
 #define F399_TSTSYN 0x8b0020
 #define F399_TSTRAM64 0x8b0010
 #define F399_TSTRAM 0x8b0008
 #define F399_TSTERR2 0x8b0004
 #define F399_TSTERR 0x8b0002
 #define F399_TSTACS 0x8b0001

/* TSTFIFO */
 #define R399_TSTFIFO 0x8c
 #define F399_TSTFIFO_7 0x8c0080
 #define F399_TSTFIFO_6 0x8c0040
 #define F399_TSTFIFO_5 0x8c0020
 #define F399_TSTFIFO_4 0x8c0010
 #define F399_TSTFIFO_3 0x8c0008
 #define F399_FORMSB 0x8c0004
 #define F399_FORLSB 0x8c0002
 #define F399_TSTFIFO 0x8c0001

/* TSTRS */
 #define R399_TSTRS 0x8d
 #define F399_TSTSCRA 0x8d0080
 #define F399_OLDRS6 0x8d0040
 #define F399_ADCT 0x8d0030
 #define F399_DILT 0x8d000c
 #define F399_SCRABIT 0x8d0002
 #define F399_TSTRSEN 0x8d0001

/* TSTDIS */
 #define R399_TSTDIS 0x8e
 #define F399_EN_DIS 0x8e0080
 #define F399_EN_PTR 0x8e0040
 #define F399_TSTDIS_5 0x8e0020
 #define F399_TSTDISFIFO_4 0x8e0010
 #define F399_EN_DIS_FIFO 0x8e0008
 #define F399_TESTPRO 0x8e0004
 #define F399_TESTREG 0x8e0002
 #define F399_TESTPRE 0x8e0001

/* TSTI2C */
 #define R399_TSTI2C 0x8f
 #define F399_EN_VI2C 0x8f0080
 #define F399_TI2C 0x8f0060
 #define F399_TSTI2C_4 0x8f0010
 #define F399_TSTI2C_3 0x8f0008
 #define F399_TSTI2C_2 0x8f0004
 #define F399_TSTI2C_1 0x8f0002
 #define F399_TSTI2C_0 0x8f0001

/* TSTCK */
 #define R399_TSTCK 0x90
 #define F399_TSTCK_7 0x900080
 #define F399_TSTCKRS 0x900040
 #define F399_TSTCKDIL 0x900020
 #define F399_TSTCK_4 0x900010
 #define F399_FORCE60 0x900008
 #define F399_FORSYMHA 0x900004
 #define F399_FORSYMAX 0x900002
 #define F399_DIRCKINT 0x900001

/* TSTRES */
 #define R399_TSTRES 0x91
 #define F399_TSTRES_7 0x910080
 #define F399_FRESRS 0x910040
 #define F399_TSTRES_5 0x910020
 #define F399_FRESMAS1_2 0x910010
 #define F399_FRESACS 0x910008
 #define F399_FRESSYM 0x910004
 #define F399_FRESMAS 0x910002
 #define F399_FRESINT 0x910001

/* TSTOUT */
 #define R399_TSTOUT 0x92
 #define F399_EN_SIGNATURE 0x920080
 #define F399_RBACT 0x920040
 #define F399_MRSTB 0x920020
 #define F399_SGNRST_T12 0x920010
 #define F399_TS 0x92000e
 #define F399_CTEST 0x920001

/* TSTIN */
 #define R399_TSTIN 0x93
 #define F399_TEST_IN 0x930080
 #define F399_EN_ADC 0x930040
 #define F399_SGN_ADC 0x930020
 #define F399_BCLK_IN 0x930010
 #define F399_TP12_T12 0x930008
 #define F399_BCLK_VALUE 0x930004
 #define F399_TSTIN_1 0x930002
 #define F399_TSTIN_0 0x930001

/* READ */
 #define R399_READ 0x94
 #define F399_READREG 0x9400ff

/* TSTFREE */
 #define R399_TSTFREE 0x95
 #define F399_TSTFREE_7 0x950080
 #define F399_TSTFREE_6 0x950040
 #define F399_TSTFREE_5 0x950020
 #define F399_TSTFREE_4 0x950010
 #define F399_TSTFREE_3 0x950008
 #define F399_TSTFREE_2 0x950004
 #define F399_TSTFREE_1 0x950002
 #define F399_TSTFREE_0 0x950001

/* TSTTNR1 */
 #define R399_TSTTNR1 0x96
 #define F399_VISU_SYNTHE 0x960080
 #define F399_NUM_SYNTH 0x960040
 #define F399_TST_TNR_13 0x960020
 #define F399_A_BYP 0x960010
 #define F399_TEST_AD_I 0x960008
 #define F399_TEST_AD_Q 0x960004
 #define F399_OUT_IOUTM 0x960002
 #define F399_OUT_IOUTP 0x960001

/* TSTTNR2 */
 #define R399_TSTTNR2 0x97
 #define F399_INTERNAL_I 0x970080
 #define F399_INTERNAL_Q 0x970040
 #define F399_POWER_OFF 0x970020
 #define F399_TST_TNR_4 0x970010
 #define F399_VOIE_QDIRECT 0x970008
 #define F399_VOIE_IDIRECTE 0x970004
 #define F399_VOIE_IPAD 0x970002
 #define F399_VOIE_QPAD 0x970001

	/* Number of registers */
	#define     STV399_NBREGS   99


	/* Number of fields */
	#define     STV399_NBFIELDS 396
#endif


#ifdef __cplusplus
 extern "C"
 {
#endif                  /* __cplusplus */


#ifdef __cplusplus
 }
#endif                  /* __cplusplus */

 
#endif          /* H_399CHIP */

/* End of 399_init.h */








