/*---------------------------------------------------------------
File Name: reg0299.h

Description: 

    STV0299 register map and functions

Copyright (C) 1999-2001 STMicroelectronics

History:

   04/02/00        Code based on original implementation by CBB.

    date: 27-June-2001
version: 3.1.0
 author: GJP
comment: adapted from older reg0299.h
    
---------------------------------------------------------------------------- */

/* define to prevent recursive inclusion */
#ifndef __STTUNER_DEMOD_R0299_H
#define __STTUNER_DEMOD_R0299_H


/* includes --------------------------------------------------------------- */

#include "stlite.h"
#include "ioarch.h"
#include "ioreg.h"


#ifdef __cplusplus
extern "C"
{
#endif                          /* __cplusplus */

#ifndef STTUNER_MINIDRIVER
/* register mappings ------------------------------------------------------- */

/* Registers (indexes not addresses) */

/* ID */
 #define R0299_ID 0x0
 #define  F0299_ID 0xff

/* RCR */
 #define R0299_RCR 0x1
 #define  F0299_K 0x100c0
 #define  F0299_DIRCLK 0x10020
 #define  F0299_M 0x1001f

/* MCR */
 #define R0299_MCR 0x2
 #define  F0299_STDBY 0x20080
 #define  F0299_VCO 0x20040
 #define  F0299_SERCLK 0x20008
 #define  F0299_P 0x20007

/* ACR */
 #define R0299_ACR 0x3
 #define  F0299_ACR 0x300ff

/* F22FR */
 #define R0299_F22FR 0x4
 #define  F0299_F22FR 0x400ff

/* I2CRPT */
 #define R0299_I2CRPT 0x5
 #define  F0299_I2CT 0x50080
 #define  F0299_SCLT 0x50004
 #define  F0299_SDAT 0x50001

/* DACR1 */
 #define R0299_DACR1 0x6
 #define  F0299_DACMODE 0x600e0
 #define  F0299_DACMSB 0x6000f

/* DACR2 */
 #define R0299_DACR2 0x7
 #define  F0299_DACLSB 0x700ff

/* DISEQC */
 #define R0299_DISEQC 0x8
 #define  F0299_LOCKOUTPUT 0x800c0
 #define  F0299_LOCKCONFIGURATION 0x80020
 #define  F0299_UNMODULATEDBURST 0x80004
 #define  F0299_DISEQCMODE 0x80003

/* DISEQCFIFO */
 #define R0299_DISEQCFIFO 0x9
 #define  F0299_DISEQCFIFO 0x900ff

/* DISEQCSTATUS */
 #define R0299_DISEQCSTATUS 0xa
 #define  F0299_INPORT 0xa0080
 #define  F0299_SDATINPUTSTATE 0xa0040
 #define  F0299_FIFOEMPTY 0xa0002
 #define  F0299_FIFOFULL 0xa0001

/* RES */
 #define R0299_RES 0xb
 #define  F0299_RESERVED 0xb00ff

/* IOCFG */
 #define R0299_IOCFG 0xc
 #define  F0299_OP1CONTROL 0xc0080
 #define  F0299_OP1VALUE 0xc0040
 #define  F0299_OP0CONTROL 0xc0020
 #define  F0299_OP0VALUE 0xc0010
 #define  F0299_NYQUISTFILTER 0xc0006
 #define  F0299_IQ 0xc0001

/* AGC1C */
 #define R0299_AGC1C 0xd
 #define  F0299_DCADJ 0xd0080
 #define  F0299_BETA_AGC1 0xd0007

/* RTC */
 #define R0299_RTC 0xe
 #define  F0299_ALPHA_TMG 0xe0070
 #define  F0299_BETA_TMG 0xe0007

/* AGC1R */
 #define R0299_AGC1R 0xf
 #define  F0299_IAGC 0xf0080
 #define  F0299_AGC1_REF 0xf003f

/* AGC2O */
 #define R0299_AGC2O 0x10
 #define  F0299_AGC2COEF 0x1000e0
 #define  F0299_AGC2_REF 0x10001f

/* TLSR */
 #define R0299_TLSR 0x11
 #define  F0299_STEP_MINUS 0x1100f0
 #define  F0299_STEP_PLUS 0x11000f

/* CFD */
 #define R0299_CFD 0x12
 #define  F0299_CFD_ALGO 0x120080
 #define  F0299_BETA_FC 0x120070
 #define  F0299_FDTC 0x12000c
 #define  F0299_LDL 0x120003

/* ACLC */
 #define R0299_ACLC 0x13
 #define  F0299_DEROTATOR 0x130080
 #define  F0299_NOISEESTIMATORTIMECONSTANT 0x130030
 #define  F0299_ALPHA_CAR 0x13000f

/* BCLC */
 #define R0299_BCLC 0x14
 #define  F0299_PHASE_DETECTOR0299_ALGO 0x1400c0
 #define  F0299_BETA_CAR 0x14003f

/* CLDT */
 #define R0299_CLDT 0x15
 #define  F0299_CLDT 0x1501ff

/* AGC1I */
 #define R0299_AGC1I 0x16
 #define  F0299_AGCINTEGRATORVALUE 0x1601ff

/* TLIR */
 #define R0299_TLIR 0x17
 #define  F0299_TLIR 0x1700ff

/* AGC2I1 */
 #define R0299_AGC2I1 0x18
 #define  F0299_AGC2INTEGRATORMSB 0x1800ff

/* AGC2I2 */
 #define R0299_AGC2I2 0x19
 #define  F0299_AGC2INTEGRATORLSB 0x1900ff

/* RTF */
 #define R0299_RTF 0x1a
 #define  F0299_RTF 0x1a01ff

/* VSTATUS */
 #define R0299_VSTATUS 0x1b
 #define  F0299_CF 0x1b0080
 #define  F0299_PRF 0x1b0010
 #define  F0299_LK 0x1b0008
 #define  F0299_CPR 0x1b0007

/* CLDI */
 #define R0299_CLDI 0x1c
 #define  F0299_CLDI 0x1c01ff

/* ERRCNT_HIGH */
 #define R0299_ERRCNT_HIGH 0x1d
 #define  F0299_ERRCNTMSB 0x1d00ff

/* ERRCNT_LOW */
 #define R0299_ERRCNT_LOW 0x1e
 #define  F0299_ERRCNTLSB 0x1e00ff

/* SFRH */
 #define R0299_SFRH 0x1f
 #define  F0299_SYMB_FREQH 0x1f00ff

/* SFRM */
 #define R0299_SFRM 0x20
 #define  F0299_SYMB_FREQM 0x2000ff

/* SFRL */
 #define R0299_SFRL 0x21
 #define  F0299_SYMB_FREQL 0x2100f0

/* CFRM */
 #define R0299_CFRM 0x22
 #define  F0299_DEROTATORFREQUENCYMSB 0x2200ff

/* CFRL */
 #define R0299_CFRL 0x23
 #define  F0299_DEROTATORFREQUENCYLSB 0x2300ff

/* NIRH */
 #define R0299_NIRH 0x24
 #define  F0299_NOISEINDICATORMSB 0x2400ff

/* NIRL */
 #define R0299_NIRL 0x25
 #define  F0299_NOISEINDICATORLSB 0x2500ff

/* VERROR */
 #define R0299_VERROR 0x26
 #define  F0299_ERRORRATE 0x2600ff

/* FECM */
 #define R0299_FECM 0x28
 #define  F0299_FECMODE 0x2800f0
 #define  F0299_OUTPUTTYPE 0x280002
 #define  F0299_OUTPUTIMPEDANCE 0x280001

/* VTH0 */
 #define R0299_VTH0 0x29
 #define  F0299_VTH0 0x2900ff

/* VTH1 */
 #define R0299_VTH1 0x2a
 #define  F0299_VTH1 0x2a00ff

/* VTH2 */
 #define R0299_VTH2 0x2b
 #define  F0299_VTH2 0x2b00ff

/* VTH3 */
 #define R0299_VTH3 0x2c
 #define  F0299_VTH3 0x2c00ff

/* VTH4 */
 #define R0299_VTH4 0x2d
 #define  F0299_VTH4 0x2d00ff

/* PR */
 #define R0299_PR 0x31
 #define  F0299_RATE 0x3100ff

/* VSEARCH */
 #define R0299_VSEARCH 0x32
 #define  F0299_SEARCHMODE 0x320080
 #define  F0299_FREEZE 0x320040
 #define  F0299_SN 0x320030
 #define  F0299_TO 0x32000c
 #define  F0299_H 0x320003

/* RS */
 #define R0299_RS 0x33
 #define  F0299_DEINTERLEAVER 0x330080
 #define  F0299_SYNCHRO 0x330040
 #define  F0299_REEDSOLOMON 0x330020
 #define  F0299_DESCRAMBLER 0x330010
 #define  F0299_WRITEERRORBIT 0x330008
 #define  F0299_BLOCKSYNCHRO 0x330004
 #define  F0299_OUTPUTCLOCKPOLARITY 0x330002
 #define  F0299_OUTPUTCLOCKCONFIG 0x330001

/* ERRCNT */
 #define R0299_ERRCNT 0x34
 #define  F0299_ERRORMODE 0x340080
 #define  F0299_ERRORSOURCE 0x340030
 #define  F0299_NOE 0x340003

/* TFEC1 */
 #define R0299_TFEC1 0x40
 #define  F0299_TFEC1 0x4000ff

/* TFEC2 */
 #define R0299_TFEC2 0x41
 #define  F0299_TFEC2 0x4100ff

/* TSTRAM1 */
 #define R0299_TSTRAM1 0x42
 #define  F0299_TSTRAM1 0x4200ff

/* TSTRATE */
 #define R0299_TSTRATE 0x43
 #define  F0299_TSTRATE 0x4300ff

/* SELOUT */
 #define R0299_SELOUT 0x44
 #define  F0299_SELOUT 0x4400ff

/* FORCEIN */
 #define R0299_FORCEIN 0x45
 #define  F0299_FORCEIN 0x4500ff

/* TSRTESCK */
 #define R0299_TSRTESCK 0x46
 #define  F0299_TSRTESCK 0x4600ff

/* TSTOUT */
 #define R0299_TSTOUT 0x47
 #define  F0299_TSTOUT 0x4700ff

/* TSTR */
 #define R0299_TSTR 0x48
 #define  F0299_TSTR 0x4800ff

/* TAGC2 */
 #define R0299_TAGC2 0x49
 #define  F0299_TAGC2 0x4900ff

/* TCTL */
 #define R0299_TCTL 0x4a
 #define  F0299_TCTL 0x4a00ff

/* TCTL2 */
 #define R0299_TCTL2 0x4b
 #define  F0299_TCTL2 0x4b00ff

/* TAGC1 */
 #define R0299_TAGC1 0x4c
 #define  F0299_TAGC1 0x4c00ff

/* TSTFIFO */
 #define R0299_TSTFIFO 0x4d
 #define  F0299_TSTFIFO 0x4d00ff

/* TSTVCO */
 #define R0299_TSTVCO 0x4e
 #define  F0299_TSTVCO 0x4e00ff

/* GHOST */
 #define R0299_GHOST 0x4f
 #define  F0299_GHOST 0x4f00ff


/* public types ------------------------------------------------------------ */

#define     STV0299_NBREGS   65

/* functions --------------------------------------------------------------- */


ST_ErrorCode_t Reg0299_Install(STTUNER_IOREG_DeviceMap_t *DeviceMap);
ST_ErrorCode_t Reg0299_Open(STTUNER_IOREG_DeviceMap_t *DeviceMap, U32 ExternalClock);

void Reg0299_SetExtClk(STTUNER_IOREG_DeviceMap_t *DeviceMap, long Value);
long Reg0299_GetExtClk(STTUNER_IOREG_DeviceMap_t *DeviceMap);

long Reg0299_CalcRefFrequency(STTUNER_IOREG_DeviceMap_t *DeviceMap, int k);
long Reg0299_CalcVCOFrequency(STTUNER_IOREG_DeviceMap_t *DeviceMap, int k, int m);
long Reg0299_RegGetVCOFreq(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle);

long Reg0299_CalcF22Frequency(STTUNER_IOREG_DeviceMap_t *DeviceMap, int k, int m, int f22);
long Reg0299_RegGetF22Freq(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle);

long Reg0299_CalcMasterClkFrequency(STTUNER_IOREG_DeviceMap_t *DeviceMap, int stdby, int dirclk, int k, int m, int p);
long Reg0299_RegGetMasterFreq(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle);

long Reg0299_CalcDerotatorFrequency(int derotmsb, int derotlsb, long fm);
long Reg0299_RegGetDerotatorFreq(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle);

long Reg0299_CalcAuxClkFrequency(STTUNER_IOREG_DeviceMap_t *DeviceMap, int k, int m, int acr);

int  Reg0299_RegGetErrorCount(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle);
void Reg0299_RegSetErrorCount(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, int Value);

int  Reg0299_GetRollOff(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle);
void Reg0299_RegSetRollOff(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, int Value);

long Reg0299_CalcCarrierNaturalFrequency(int m2 , int betacar , long fm , long fs);
long Reg0299_RegGetCarrierNaturalFrequency(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle);

long Reg0299_CalcCarrierDampingFactor(int m2, int alphacar, int betacar, long fm, long fs);
long Reg0299_RegGetCarrierDampingFactor(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle);

long Reg0299_CalcTimingNaturalFrequency(STTUNER_IOREG_DeviceMap_t *DeviceMap, int m2, int betatmg, long fs);
long Reg0299_RegGetTimingNaturalFrequency(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle);

long Reg0299_CalcTimingDampingFactor(int m2, int alphatmg, int betatmg);
long Reg0299_RegGetTimingDampingFactor(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle);

long Reg0299_CalcSymbolRate(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, int Hbyte, int Mbyte, int Lbyte);
long Reg0299_RegSetSymbolRate(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, long SymbolRate);
long Reg0299_RegGetSymbolRate(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle);
long Reg0299_RegIncrSymbolRate(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, long correction);

long Reg0299_CalcAGC1TimeConstant(int m1, long fm, int betaagc1);
long Reg0299_RegGetAGC1TimeConstant(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle);

long Reg0299_CalcAGC2TimeConstant(long agc2coef, long m1, long fm);
long Reg0299_RegGetAGC2TimeConstant(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle);


void Reg0299_RegF22On (STTUNER_IOREG_DeviceMap_t *DeviceMap);
void Reg0299_RegF22Off(STTUNER_IOREG_DeviceMap_t *DeviceMap);

void Reg0299_RegTriggerOn(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle);
void Reg0299_RegTriggerOff(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle);

STTUNER_IOREG_Flag_t Reg0299_RegGetTrigger(STTUNER_IOREG_DeviceMap_t *DeviceMap);
void                 Reg0299_RegSetTrigger(STTUNER_IOREG_DeviceMap_t *DeviceMap, STTUNER_IOREG_Flag_t Trigger);

#endif

#ifdef STTUNER_MINIDRIVER
#define R0299_CFRM    34
#define R0299_SFRH    31
#define R0299_ACLC    19
#define R0299_PR      45
#define R0299_VSEARCH 46
#define R0299_FECM  0x28
#define F0299_OUTPUTTYPE     1
#define F0299_OUTPUTTYPE_L   1
#define F0299_FECMODE        4
#define F0299_FECMODE_L      4
#define R0299_IOCFG   0x0C
#define F0299_NYQUISTFILTER  1
#define F0299_NYQUISTFILTER_L 2
#define R0299_RS  0x33
#define F0299_DESCRAMBLER   4
#define F0299_DESCRAMBLER_L 1
#define R0299_MCR 0x02
#define F0299_SERCLK      3
#define F0299_SERCLK_L    1
#define F0299_OUTPUTCLOCKPOLARITY   1
#define F0299_OUTPUTCLOCKPOLARITY_L 1
#define F0299_OUTPUTCLOCKCONFIG     0
#define F0299_OUTPUTCLOCKCONFIG_L   1
#define F0299_BLOCKSYNCHRO          2
#define F0299_BLOCKSYNCHRO_L        1

#define F0299_IAGC    7
#define F0299_IAGC_L  1
#define R0299_VSTATUS   0x1B
#define F0299_CPR     0
#define F0299_CPR_L     3
#define F0299_LK   3
#define F0299_LK_L   1
#define F0299_RATE   0
#define F0299_RATE_L   8
#define R0299_TLIR  0x17
#define F0299_TLIR    0
#define F0299_TLIR_L    8
#define R0299_RTF   0x1A
#define F0299_RTF      0
#define F0299_RTF_L      8
#define R0299_CFD  0x12
#define F0299_CFD_ALGO    7
#define F0299_CFD_ALGO_L  1
#define F0299_CF     7
#define F0299_CF_L     1
#define F0299_OP1CONTROL   7
#define F0299_OP1CONTROL_L   1
#define F0299_OP1VALUE    6
#define F0299_OP1VALUE_L    1
#define F0299_IQ   0
#define F0299_IQ_L   1
#define R0299_BCLC   0x14
#define F0299_BETA_CAR  0
#define F0299_BETA_CAR_L  6
#define R0299_AGC1R   0x0F
#define F0299_AGC1_REF  0
#define F0299_AGC1_REF_L  6
#define F0299_OP1VALUE     6
#define F0299_OP1VALUE_L   1
#define F0299_OP0VALUE  4
#define F0299_OP0VALUE_L  1
#define F0299_LOCKOUTPUT     6
#define F0299_LOCKOUTPUT_L   2
#define R0299_DISEQC 0x08

#define R0299_DISEQCSTATUS 0x0A
#define F0299_FIFOFULL 0
#define F0299_FIFOFULL_L 1

#define R0299_DISEQC 0x08
#define F0299_DISEQCMODE 0
#define F0299_DISEQCMODE_L 2
#define F0299_UNMODULATEDBURST 2
#define F0299_UNMODULATEDBURST_L 1

#define R0299_DISEQCFIFO 0x09

#define R0299_NIRH  0x24

#endif

#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif                          /* __STTUNER_DEMOD_R0299_H */

/* End of reg0299.h */

