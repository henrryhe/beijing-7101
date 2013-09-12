/* ----------------------------------------------------------------------------
File Name: d0498.c

Description:

    stv0498 demod driver.

Copyright (C) 1999-2007 STMicroelectronics

---------------------------------------------------------------------------- */


/* Includes ---------------------------------------------------------------- */
#ifndef ST_OSLINUX
/* C libs */
#include <string.h>
#include "sttbx.h"
#include "stlite.h"     /* Standard includes */
#endif
#include "stos.h"
#include "stcommon.h"

/* STAPI */

#include "stevt.h"
#include "sttuner.h"
#include "stsys.h"
/* local to sttuner */
#include "dbtypes.h"    /* data types for databases */
#include "sysdbase.h"   /* functions to accesss system data */
#include "ioarch.h"     /* I/O for this driver */
#include "ioreg.h"      /* I/O for this driver */
#include "util.h"       /* generic utility functions for sttuner */
#include "drv0498.h"   /* misc driver functions */
#include "d0498.h"     /* header for this file */
#include "util498.h"   /* utility functions */

#include "cioctl.h"     /* data structure typedefs for all the the cable ioctl functions */


/* Private types/constants ------------------------------------------------ */

/* Device capabilities */
#define STV0498_MAX_AGC                     1023
#define STV0498_MAX_SIGNAL_QUALITY          100
#define STCHIP_HANDLE(x) ((STCHIP_InstanceData_t *)x)

/* Device ID */
#define STV0498_DEVICE_VERSION              0x10
#define STV0498_SYMBOLMIN                   870000;     /* # 1  MegaSymbols/sec */
#define STV0498_SYMBOLMAX                   11700000;   /* # 11 MegaSymbols/sec */

/* private variables ------------------------------------------------------- */

static semaphore_t *Lock_InitTermOpenClose; /* guard calls to the functions */

static BOOL        Installed = FALSE;


/* ---------- per instance of driver ---------- */
typedef struct
{
    ST_DeviceName_t           *DeviceName;           /* unique name for opening under */
    STTUNER_Handle_t          TopLevelHandle;       /* access tuner, lnb etc. using this */
    FE_498_Modulation_t      FE_498_Modulation;
    ST_Partition_t            *MemoryPartition;     /* which partition this data block belongs to */
    void                      *InstanceChainPrev;   /* previous data block in chain or NULL if not */
    void                      *InstanceChainNext;   /* next data block in chain or NULL if last */

    U32                         ExternalClock;  /* External VCO */
    STTUNER_TSOutputMode_t      TSOutputMode;
    STTUNER_SerialDataMode_t    SerialDataMode;
    STTUNER_SerialClockSource_t SerialClockSource;
    STTUNER_FECMode_t           FECMode;
    STTUNER_DataClockPolarity_t ClockPolarity;
    STTUNER_SyncStrip_t         SyncStripping;
    STTUNER_BlockSyncMode_t     BlockSyncMode;
    U32 Symbolrate;
    U32                         StandBy_Flag; /*** To take care same Standby Mode doesnot
                                                   execute more than once ***/
    FE_498_Fec_t FecType;                                                   
 }D0498_InstanceData_t;

/* instance chain, the default boot value is invalid, to catch errors */
static D0498_InstanceData_t *InstanceChainTop = (D0498_InstanceData_t *)0x7fffffff;

/* functions --------------------------------------------------------------- */

/* API */
ST_ErrorCode_t demod_d0498_Init(ST_DeviceName_t *DeviceName, DEMOD_InitParams_t *InitParams);
ST_ErrorCode_t demod_d0498_Term(ST_DeviceName_t *DeviceName, DEMOD_TermParams_t *TermParams);

ST_ErrorCode_t demod_d0498_Open (ST_DeviceName_t *DeviceName, DEMOD_OpenParams_t  *OpenParams, DEMOD_Capability_t *Capability, DEMOD_Handle_t *Handle);
ST_ErrorCode_t demod_d0498_Close(DEMOD_Handle_t  Handle, DEMOD_CloseParams_t *CloseParams);


ST_ErrorCode_t demod_d0498_IsAnalogCarrier (DEMOD_Handle_t Handle, BOOL *IsAnalog);
ST_ErrorCode_t demod_d0498_GetSignalQuality(DEMOD_Handle_t Handle, U32  *SignalQuality_p, U32 *Ber_p);
ST_ErrorCode_t demod_d0498_GetModulation   (DEMOD_Handle_t Handle, STTUNER_Modulation_t *Modulation);
ST_ErrorCode_t demod_d0498_SetModulation   (DEMOD_Handle_t Handle, STTUNER_Modulation_t Modulation);
ST_ErrorCode_t demod_d0498_GetAGC          (DEMOD_Handle_t Handle, S16                  *Agc);
ST_ErrorCode_t demod_d0498_GetFECRates     (DEMOD_Handle_t Handle, STTUNER_FECRate_t    *FECRates);
ST_ErrorCode_t demod_d0498_IsLocked        (DEMOD_Handle_t Handle, BOOL                 *IsLocked);
ST_ErrorCode_t demod_d0498_SetFECRates     (DEMOD_Handle_t Handle, STTUNER_FECRate_t     FECRates);
ST_ErrorCode_t demod_d0498_Tracking        (DEMOD_Handle_t Handle, BOOL ForceTracking,   U32 *NewFrequency, BOOL *SignalFound);
ST_ErrorCode_t demod_d0498_StandByMode(DEMOD_Handle_t Handle, STTUNER_StandByMode_t PowerMode);

ST_ErrorCode_t demod_d0498_ScanFrequency   (DEMOD_Handle_t Handle, U32  InitialFrequency, U32   SymbolRate,    U32   MaxOffset,
                                                                   U32  TunerStep,        U8    DerotatorStep, BOOL *ScanSuccess,
                                                                   U32 *NewFrequency,     U32   Mode,          U32   Guard,
                                                                   U32  Force,            U32   Hierarchy,     U32   Spectrum,
                                                                   U32  FreqOff,          U32   ChannelBW,     S32   EchoPos);
/* access device specific low-level functions */
ST_ErrorCode_t demod_d0498_ioctl           (DEMOD_Handle_t Handle, U32 Function, void *InParams, void *OutParams,
                                             STTUNER_Da_Status_t *Status);

/* repeater/passthrough port for other drivers to use, type: STTUNER_IOARCH_RedirFn_t */
ST_ErrorCode_t demod_d0498_ioaccess         (DEMOD_Handle_t Handle, IOARCH_Handle_t IOHandle,
                                             STTUNER_IOARCH_Operation_t Operation, U16 SubAddr,
                                             U8 *Data, U32 TransferSize, U32 Timeout);

/* local functions --------------------------------------------------------- */

D0498_InstanceData_t *D0498_GetInstFromHandle(DEMOD_Handle_t Handle);

U8 Def498Val[STB0498_NBREGS]=
{
0x00,  /* CTRL_0*/
0x8f,  /* CTRL_1*/
0x0C,  /* CTRL_2*/
0x2B,  /* IT_STATUS1*/
0x08,  /* IT_STATUS2*/
0x00,  /* IT_EN1*/
0x00,  /* IT_EN2*/
0x04,  /* CTRL_STATUS*/
0x00,  /* TEST_CTL*/
0x77,  /* AGC_CTL*/
0x50,  /* AGC_IF_CFG*/
0x00,  /* AGC_RF_CFG*/
0x03,  /* AGC_PWM_CFG*/
0x5A,  /* AGC_PWR_REF_L*/
0x00,  /* AGC_PWR_REF_H*/
0xc1,  /* AGC_RF_TH_L*/
0x01,  /* AGC_RF_TH_H*/
0x99,  /* AGC_IF_LTH_L*/
0x09,  /* AGC_IF_LTH_H*/
0x00,  /* AGC_IF_HTH_L*/
0x00,  /* AGC_IF_HTH_H*/
0x00,  /* AGC_PWR_RD_L*/
0xE9,  /* AGC_PWR_RD_M*/
0x03,  /* AGC_PWR_RD_H*/
0x00,  /* AGC_PWM_IFCMD_L*/
0x0F,  /* AGC_PWM_IFCMD_H*/
0x00,  /* AGC_PWM_RFCMD_L*/
0x00,  /* AGC_PWM_RFCMD_H*/
0x00,  /* IQDEM_CFG*/
0x39,  /* MIX_NCO_LL*/
0x94,  /* MIX_NCO_HL*/
0x67,  /* MIX_NCO_HH*/
0x21,  /* SRC_NCO_LL*/
0x1E,  /* SRC_NCO_LH*/
0xF9,  /* SRC_NCO_HL*/
0x17,  /* SRC_NCO_HH*/
0xC5,  /* IQDEM_GAIN_SRC_L*/
0x00,  /* IQDEM_GAIN_SRC_H*/
0x05,  /* IQDEM_DCRM_CFG_LL*/
0x00,  /* IQDEM_DCRM_CFG_LH*/
0xF0,  /* IQDEM_DCRM_CFG_HL*/
0x3F,  /* IQDEM_DCRM_CFG_HH*/
0x8E,  /* IQDEM_ADJ_COEFF0*/
0xF2,  /* IQDEM_ADJ_COEFF1*/
0xD7,  /* IQDEM_ADJ_COEFF2*/
0x6F,  /* IQDEM_ADJ_COEFF3*/
0xB5,  /* IQDEM_ADJ_COEFF4*/
0x82,  /* IQDEM_ADJ_COEFF5*/
0xD7,  /* IQDEM_ADJ_COEFF6*/
0x0F,  /* IQDEM_ADJ_COEFF7*/
0x00,  /* IQDEM_ADJ_EN*/
0x82,  /* IQDEM_ADJ_AGC_REF*/
0xCE,  /* ALLPASSFILT1*/
0xAD,  /* ALLPASSFILT2*/
0xA3,  /* ALLPASSFILT3*/
0x78,  /* ALLPASSFILT4*/
0x7D,  /* ALLPASSFILT5*/
0x58,  /* ALLPASSFILT6*/
0x95,  /* ALLPASSFILT7*/
0x53,  /* ALLPASSFILT8*/
0xB9,  /* ALLPASSFILT9*/
0x12,  /* ALLPASSFILT10*/
0xD0,  /* ALLPASSFILT11*/
0x20,  /* TRL_AGC_CFG*/
0x4C,  /* TRL_LPF_CFG*/
0x44,  /* TRL_LPF_ACQ_GAIN*/
0x22,  /* TRL_LPF_TRK_GAIN*/
0x03,  /* TRL_LPF_OUT_GAIN*/
0x04,  /* TRL_LOCKDET_LTH*/
0x11,  /* TRL_LOCKDET_HTH*/
0x20,  /* TRL_LOCKDET_TRGVAL*/
0xB0,  /* FSM_STATE*/
0x08,  /* FSM_CTL*/
0x0C,  /* FSM_STS*/
0x00,  /* FSM_SNR0_HTH*/
0x00,  /* FSM_SNR1_HTH*/
0x00,  /* FSM_SNR2_HTH*/
0x00,  /* FSM_SNR0_LTH*/
0x00,  /* FSM_SNR1_LTH*/
0x00,  /* FSM_RCA_HTH*/
0x64,  /* FSM_TEMPO*/
0x03,  /* FSM_CONFIG*/
0x00,  /* EQU_I_TESTTAP_L*/
0x00,  /* EQU_I_TESTTAP_M*/
0x00,  /* EQU_I_TESTTAP_H*/
0x00,  /* EQU_TESTAP_CFG*/
0x00,  /* EQU_Q_TESTTAP_L*/
0xFF,  /* EQU_Q_TESTTAP_M*/
0x1F,  /* EQU_Q_TESTTAP_H*/
0x06,  /* EQU_TAP_CTRL*/
0x00,  /* EQU_CTR_CRL_CONTROL_L*/
0x00,  /* EQU_CTR_CRL_CONTROL_H*/
0x00,  /* EQU_CTR_HIPOW_L*/
0x00,  /* EQU_CTR_HIPOW_H*/
0x00,  /* EQU_I_EQU_LO*/
0x00,  /* EQU_I_EQU_HI*/
0x9D,  /* EQU_Q_EQU_LO*/
0x00,  /* EQU_Q_EQU_HI*/
0x83,  /* EQU_MAPPER*/
0x80,  /* EQU_SWEEP_RATE*/
0xE6,  /* EQU_SNR_LO*/
0x00,  /* EQU_SNR_HI*/
0x00,  /* EQU_GAMMA_LO*/
0x00,  /* EQU_GAMMA_HI*/
0x36,  /* EQU_ERR_GAIN*/
0xAA,  /* EQU_RADIUS*/
0x00,  /* EQU_FFE_MAINTAP*/
0x63,  /* EQU_FFE_LEAKAGE*/
0xD0,  /* EQU_FFE_MAINTAP_POS*/
0x88,  /* EQU_GAIN_WIDE*/
0x22,  /* EQU_GAIN_NARROW*/
0xC1,  /* EQU_CTR_LPF_GAIN*/
0xA7,  /* EQU_CRL_LPF_GAIN*/
0x06,  /* EQU_GLOBAL_GAIN*/
0x95,  /* EQU_CRL_LD_SEN*/
0xE2,  /* EQU_CRL_LD_VAL*/
0x17,  /* EQU_CRL_TFR*/
0x00,  /* EQU_CRL_BISTH_LO*/
0x00,  /* EQU_CRL_BISTH_HI*/
0x00,  /* EQU_SWEEP_RANGE_LO*/
0x00,  /* EQU_SWEEP_RANGE_HI*/
0x40,  /* EQU_CRL_LIMITER*/
0x90,  /* EQU_MODULUS_MAP*/
0x19,  /* EQU_PNT_GAIN*/
0x16,  /* FEC_AC_CTR_0*/
0x0B,  /* FEC_AC_CTR_1*/
0x88,  /* FEC_AC_CTR_2*/
0x02,  /* FEC_AC_CTR_3*/
0x00,  /* FEC_STATUS*/
0x00,  /* RS_COUNTER_0*/
0x00,  /* RS_COUNTER_1*/
0x00,  /* RS_COUNTER_2*/
0x00,  /* RS_COUNTER_3*/
0x00,  /* RS_COUNTER_4*/
0x00,  /* RS_COUNTER_5*/
0x01,  /* BERT_0*/
0x26,  /* BERT_1*/
0x00,  /* BERT_2*/
0x00,  /* BERT_3*/
0x80,  /* OUTFORMAT_0*/
0x22,  /* OUTFORMAT_1*/
0x8e,  /*0xD7,  */ /* SMOOTHER_0 */ /*smoother updated*/
0x03,  /* SMOOTHER_1*/
0x00,  /* SMOOTHER_2*/
0x01,  /* TSMF_CTRL_0*/
0xC6,  /* TSMF_CTRL_1*/
0x43,  /* TSMF_CTRL_3*/
0x00,  /* TS_ON_ID_0*/
0x00,  /* TS_ON_ID_1*/
0x00,  /* TS_ON_ID_2*/
0x00,  /* TS_ON_ID_3*/
0x00,  /* RE_STATUS_0*/
0x00,  /* RE_STATUS_1*/
0x00,  /* RE_STATUS_2*/
0x00,  /* RE_STATUS_3*/
0x00,  /* TS_STATUS_0*/
0x00,  /* TS_STATUS_1*/
0xA0,  /* TS_STATUS_2*/
0x00,  /* TS_STATUS_3*/
0x00,  /* T_O_ID_0*/
0x00,  /* T_O_ID_1*/
0x00,  /* T_O_ID_2*/
0x00,  /* T_O_ID_3*/
0x01,  /* MPEG_CTL*/
0x03,  /* MPEG_SYNC_ACQ*/
0x03,  /* FECB_MPEG_SYNC_LOSS*/
0x0D,  /* VITERBI_SYNC_ACQ*/
0xDC,  /* VITERBI_SYNC_LOSS*/
0x08,  /* VITERBI_SYNC_GO*/
0x08,  /* VITERBI_SYNC_STOP*/
0xA6,  /* FS_SYNC*/
0x01,  /* IN_DEPTH*/
0x20,  /* RS_CONTROL*/
0x44,  /* DEINT_CONTROL*/
0x1E,  /* SYNC_STAT*/
0x00,  /* VITERBI_I_RATE*/
0x00,  /* VITERBI_Q_RATE*/
0xA7,  /* RS_CORR_COUNT_L*/
0x02,  /* RS_CORR_COUNT_H*/
0xDD,  /* RS_UNERR_COUNT_L*/
0xD5,  /* RS_UNERR_COUNT_H*/
0x7E,  /* RS_UNC_COUNT_L*/
0x0E,  /* RS_UNC_COUNT_H*/
0x00,  /* RS_RATE_L*/
0x00,  /* RS_RATE_H*/
0x05,  /* TX_INTERLEAVER_DEPTH*/
0xFF,  /* RS_ERR_CNT_L*/
0x0F,  /* RS_ERR_CNT_H*/
0x02,  /* OOB_DUTY*/
0x01  /* OOB_DELAY*/
};


U32 Addressarray[STB0498_NBREGS][3]=
{
{R498_CTRL_0},
{R498_CTRL_1},
{R498_CTRL_2},
{R498_IT_STATUS1},
{R498_IT_STATUS2},
{R498_IT_EN1},
{R498_IT_EN2},
{R498_CTRL_STATUS},
{R498_TEST_CTL},
{R498_AGC_CTL},
{R498_AGC_IF_CFG},
{R498_AGC_RF_CFG},
{R498_AGC_PWM_CFG},
{R498_AGC_PWR_REF_L},
{R498_AGC_PWR_REF_H},
{R498_AGC_RF_TH_L},
{R498_AGC_RF_TH_H},
{R498_AGC_IF_LTH_L},
{R498_AGC_IF_LTH_H},
{R498_AGC_IF_HTH_L},
{R498_AGC_IF_HTH_H},
{R498_AGC_PWR_RD_L},
{R498_AGC_PWR_RD_M},
{R498_AGC_PWR_RD_H},
{R498_AGC_PWM_IFCMD_L},
{R498_AGC_PWM_IFCMD_H},
{R498_AGC_PWM_RFCMD_L},
{R498_AGC_PWM_RFCMD_H},
{R498_IQDEM_CFG},
{R498_MIX_NCO_LL},
{R498_MIX_NCO_HL},
{R498_MIX_NCO_HH},/*should be LH pg 317*/
/*{R498_MIX_NCO},*/
{R498_SRC_NCO_LL},
{R498_SRC_NCO_LH},
{R498_SRC_NCO_HL},
{R498_SRC_NCO_HH},
/*{R498_SRC_NCO},*/
{R498_IQDEM_GAIN_SRC_L},
{R498_IQDEM_GAIN_SRC_H},
{R498_IQDEM_DCRM_CFG_LL},
{R498_IQDEM_DCRM_CFG_LH},
{R498_IQDEM_DCRM_CFG_HL},
{R498_IQDEM_DCRM_CFG_HH},
{R498_IQDEM_ADJ_COEFF0},
{R498_IQDEM_ADJ_COEFF1},
{R498_IQDEM_ADJ_COEFF2},
{R498_IQDEM_ADJ_COEFF3},
{R498_IQDEM_ADJ_COEFF4},
{R498_IQDEM_ADJ_COEFF5},
{R498_IQDEM_ADJ_COEFF6},
{R498_IQDEM_ADJ_COEFF7},
{R498_IQDEM_ADJ_EN},
{R498_IQDEM_ADJ_AGC_REF},
{R498_ALLPASSFILT1},
{R498_ALLPASSFILT2},
{R498_ALLPASSFILT3},
{R498_ALLPASSFILT4},
{R498_ALLPASSFILT5},
{R498_ALLPASSFILT6},
{R498_ALLPASSFILT7},
{R498_ALLPASSFILT8},
{R498_ALLPASSFILT9},
{R498_ALLPASSFILT10},
{R498_ALLPASSFILT11},
{R498_TRL_AGC_CFG},
{R498_TRL_LPF_CFG},
{R498_TRL_LPF_ACQ_GAIN},
{R498_TRL_LPF_TRK_GAIN},
{R498_TRL_LPF_OUT_GAIN},
{R498_TRL_LOCKDET_LTH},
{R498_TRL_LOCKDET_HTH},
{R498_TRL_LOCKDET_TRGVAL},
{R498_FSM_STATE},
{R498_FSM_CTL},
{R498_FSM_STS},
{R498_FSM_SNR0_HTH},
{R498_FSM_SNR1_HTH},
{R498_FSM_SNR2_HTH},
{R498_FSM_SNR0_LTH},
{R498_FSM_SNR1_LTH},
{R498_FSM_RCA_HTH},
{R498_FSM_TEMPO},
{R498_FSM_CONFIG},
{R498_EQU_I_TESTTAP_L},
{R498_EQU_I_TESTTAP_M},
{R498_EQU_I_TESTTAP_H},
{R498_EQU_TESTAP_CFG},
{R498_EQU_Q_TESTTAP_L},
{R498_EQU_Q_TESTTAP_M},
{R498_EQU_Q_TESTTAP_H},
{R498_EQU_TAP_CTRL},
{R498_EQU_CTR_CRL_CONTROL_L},
{R498_EQU_CTR_CRL_CONTROL_H},
{R498_EQU_CTR_HIPOW_L},
{R498_EQU_CTR_HIPOW_H},
{R498_EQU_I_EQU_LO},
{R498_EQU_I_EQU_HI},
{R498_EQU_Q_EQU_LO},
{R498_EQU_Q_EQU_HI},
{R498_EQU_MAPPER},
{R498_EQU_SWEEP_RATE},
{R498_EQU_SNR_LO},
{R498_EQU_SNR_HI},
{R498_EQU_GAMMA_LO},
{R498_EQU_GAMMA_HI},
{R498_EQU_ERR_GAIN},
{R498_EQU_RADIUS},
{R498_EQU_FFE_MAINTAP},
{R498_EQU_FFE_LEAKAGE},
{R498_EQU_FFE_MAINTAP_POS},
{R498_EQU_GAIN_WIDE},
{R498_EQU_GAIN_NARROW},
{R498_EQU_CTR_LPF_GAIN},
{R498_EQU_CRL_LPF_GAIN},
{R498_EQU_GLOBAL_GAIN},
{R498_EQU_CRL_LD_SEN},
{R498_EQU_CRL_LD_VAL},
{R498_EQU_CRL_TFR},
{R498_EQU_CRL_BISTH_LO},
{R498_EQU_CRL_BISTH_HI},
{R498_EQU_SWEEP_RANGE_LO},
{R498_EQU_SWEEP_RANGE_HI},
{R498_EQU_CRL_LIMITER},
{R498_EQU_MODULUS_MAP},
{R498_EQU_PNT_GAIN},
{R498_FEC_AC_CTR_0},
{R498_FEC_AC_CTR_1},
{R498_FEC_AC_CTR_2},
{R498_FEC_AC_CTR_3},
{R498_FEC_STATUS},
{R498_RS_COUNTER_0},
{R498_RS_COUNTER_1},
{R498_RS_COUNTER_2},
{R498_RS_COUNTER_3},
{R498_RS_COUNTER_4},
{R498_RS_COUNTER_5},
{R498_BERT_0},
{R498_BERT_1},
{R498_BERT_2},
{R498_BERT_3},
{R498_OUTFORMAT_0},
{R498_OUTFORMAT_1},
{R498_SMOOTHER_0},
{R498_SMOOTHER_1},
{R498_SMOOTHER_2},
{R498_TSMF_CTRL_0},
{R498_TSMF_CTRL_1},
{R498_TSMF_CTRL_3},
{R498_TS_ON_ID_0},
{R498_TS_ON_ID_1},
{R498_TS_ON_ID_2},
{R498_TS_ON_ID_3},
{R498_RE_STATUS_0},
{R498_RE_STATUS_1},
{R498_RE_STATUS_2},
{R498_RE_STATUS_3},
{R498_TS_STATUS_0},
{R498_TS_STATUS_1},
{R498_TS_STATUS_2},
{R498_TS_STATUS_3},
{R498_T_O_ID_0},
{R498_T_O_ID_1},
{R498_T_O_ID_2},
{R498_T_O_ID_3},
{R498_MPEG_CTL},
{R498_MPEG_SYNC_ACQ},
{R498_FECB_MPEG_SYNC_LOSS},
{R498_VITERBI_SYNC_ACQ},
{R498_VITERBI_SYNC_LOSS},
{R498_VITERBI_SYNC_GO},
{R498_VITERBI_SYNC_STOP},
{R498_FS_SYNC},
{R498_IN_DEPTH},
{R498_RS_CONTROL},
{R498_DEINT_CONTROL},
{R498_SYNC_STAT},
{R498_VITERBI_I_RATE},
{R498_VITERBI_Q_RATE},
{R498_RS_CORR_COUNT_L},
{R498_RS_CORR_COUNT_H},
{R498_RS_UNERR_COUNT_L},
{R498_RS_UNERR_COUNT_H},
{R498_RS_UNC_COUNT_L},
{R498_RS_UNC_COUNT_H},
{R498_RS_RATE_L},
{R498_RS_RATE_H},
{R498_TX_INTERLEAVER_DEPTH},
{R498_RS_ERR_CNT_L},
{R498_RS_ERR_CNT_H},
{R498_OOB_DUTY},
{R498_OOB_DELAY}
};


/* ----------------------------------------------------------------------------
Name: STTUNER_DRV_DEMOD_STV0498_Install()

Description:
    install a cable device driver into the demod database.

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_DRV_DEMOD_STV0498_Install(STTUNER_demod_dbase_t *Demod)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
   const char *identity = "STTUNER d0498.c STTUNER_DRV_DEMOD_STV0498_Install()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if(Installed == TRUE)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
        STTBX_Print(("%s fail driver already installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
    STTBX_Print(("%s installing cable:demod:STV0498...", identity));
#endif

    /* mark ID in database */
    Demod->ID = STTUNER_DEMOD_STV0498;

    /* map API */
    Demod->demod_Init = demod_d0498_Init;
    Demod->demod_Term = demod_d0498_Term;

    Demod->demod_Open  = demod_d0498_Open;
    Demod->demod_Close = demod_d0498_Close;

    Demod->demod_IsAnalogCarrier  = demod_d0498_IsAnalogCarrier;
    Demod->demod_GetSignalQuality = demod_d0498_GetSignalQuality;
    Demod->demod_GetModulation    = demod_d0498_GetModulation;
    Demod->demod_SetModulation    = demod_d0498_SetModulation;
    Demod->demod_GetAGC           = demod_d0498_GetAGC;
    Demod->demod_GetFECRates      = demod_d0498_GetFECRates;
    Demod->demod_IsLocked         = demod_d0498_IsLocked ;
    Demod->demod_SetFECRates      = demod_d0498_SetFECRates;
    Demod->demod_Tracking         = demod_d0498_Tracking;
    Demod->demod_ScanFrequency    = demod_d0498_ScanFrequency;
    Demod->demod_StandByMode      = demod_d0498_StandByMode;
    Demod->demod_ioaccess		  = demod_d0498_ioaccess;
    /*Demod->demod_ioctl			  = demod_d0498_ioctl;*/

    InstanceChainTop = NULL;

	Lock_InitTermOpenClose = STOS_SemaphoreCreateFifo(NULL, 1);

    Installed = TRUE;

#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
    STTBX_Print(("ok\n"));
#endif

    return(Error);
}



/* ----------------------------------------------------------------------------
Name: STTUNER_DRV_DEMOD_STV0498_UnInstall()

Description:
    install a cable device driver into the demod database.

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_DRV_DEMOD_STV0498_UnInstall(STTUNER_demod_dbase_t *Demod)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
   const char *identity = "STTUNER d0498.c STTUNER_DRV_DEMOD_STV0498_UnInstall()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

    if(Demod->ID != STTUNER_DEMOD_STV0498)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
        STTBX_Print(("%s fail incorrect driver type\n", identity));
#endif
        return(STTUNER_ERROR_ID);
    }

    /* has all memory been freed, by Term() */
    if(InstanceChainTop != NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
        STTBX_Print(("%s fail at least one instance not terminated\n", identity));
#endif
        return(ST_ERROR_OPEN_HANDLE);
    }


#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
    STTBX_Print(("%s uninstalling cable:demod:STV0498...", identity));
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
    Demod->demod_SetModulation    = NULL;
    Demod->demod_GetAGC           = NULL;
    Demod->demod_GetFECRates      = NULL;
    Demod->demod_IsLocked         = NULL;
    Demod->demod_SetFECRates      = NULL;
    Demod->demod_Tracking         = NULL;
    Demod->demod_ScanFrequency    = NULL;
    Demod->demod_StandByMode      = NULL;
    Demod->demod_ioaccess = NULL;
    Demod->demod_ioctl    = NULL;

#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
        STTBX_Print(("<"));
#endif

STOS_SemaphoreDelete(NULL, Lock_InitTermOpenClose);

#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
        STTBX_Print((">"));
#endif

    InstanceChainTop = (D0498_InstanceData_t *)0x7ffffffe;
    Installed        = FALSE;

#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
        STTBX_Print(("ok\n"));
#endif

    return(Error);
}



/* ------------------------------------------------------------------------- */
/* /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ API /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ */
/* ------------------------------------------------------------------------- */



/* ----------------------------------------------------------------------------
Name: demod_d0498_Init()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0498_Init(ST_DeviceName_t *DeviceName, DEMOD_InitParams_t *InitParams)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
    const char *identity = "STTUNER d0498.c demod_d0498_Init()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0498_InstanceData_t *InstanceNew, *Instance;

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
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
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
        STTBX_Print(("%s fail MemoryPartition not valid\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    Error = STTUNER_Util_CheckPtrNull(DeviceName);
    if( Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
        STTBX_Print(("%s fail DeviceName not valid\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    InstanceNew = STOS_MemoryAllocateClear(InitParams->MemoryPartition, 1, sizeof( D0498_InstanceData_t ));
    if (InstanceNew == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
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
    InstanceNew->MemoryPartition     = InitParams->MemoryPartition;


    InstanceNew->InstanceChainNext   = NULL; /* always last in the chain */

    InstanceNew->ExternalClock     = InitParams->ExternalClock;          /* Unit Hz */
    InstanceNew->TSOutputMode      = InitParams->TSOutputMode;
    InstanceNew->SerialDataMode    = InitParams->SerialDataMode;
    InstanceNew->SerialClockSource = InitParams->SerialClockSource;
    InstanceNew->FECMode           = InitParams->FECMode;
    InstanceNew->ClockPolarity     = InitParams->ClockPolarity;
    InstanceNew->SyncStripping     = InitParams->SyncStripping;
    InstanceNew->BlockSyncMode     = InitParams->BlockSyncMode;
    InstanceNew->FE_498_Modulation = FE_498_MOD_QAM64; /*default 64QAM*/ /*will be filled from scanparams during scan/set frequency*/

    /* reserve memory for register mapping */
    /*Error = STTUNER_IOREG_Open(&InstanceNew->DeviceMap);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
        STTBX_Print(("%s fail setup new register database\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }*/

#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
    STTBX_Print(("%s allocated & initalized block named '%s' at 0x%08x (%d bytes)\n", identity, InstanceNew->DeviceName, (U32)InstanceNew, sizeof( D0498_InstanceData_t ) ));
#endif
    SEM_UNLOCK(Lock_InitTermOpenClose);
    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0498_Term()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0498_Term(ST_DeviceName_t *DeviceName, DEMOD_TermParams_t *TermParams)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
   const char *identity = "STTUNER d0498.c demod_d0498_Term()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0498_InstanceData_t *Instance, *InstancePrev, *InstanceNext;

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
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
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
        STTBX_Print(("%s fail TermParams not valid\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    /* ---------- check that at least one init has taken place ---------- */
    if(InstanceChainTop == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
        STTBX_Print(("%s fail nothing initalized\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(STTUNER_ERROR_INITSTATE);
    }


    /* reap next matching DeviceName */
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
    STTBX_Print(("%s looking for first free handle[\n", identity));
#endif

    Instance = InstanceChainTop;
    while(1)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
        STTBX_Print(("(%s)", Instance->DeviceName));
#endif
        if ( strcmp( (char *)Instance->DeviceName, (char *)DeviceName) == 0)
        {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
            STTBX_Print(("]\n"));
#endif		

            /*Error = STTUNER_IOREG_Close(&Instance->DeviceMap);
            if (Error != ST_NO_ERROR)
            {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
                STTBX_Print(("%s fail close register database\n", identity));
#endif
            }*/

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
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
            STTBX_Print(("%s freed block at %0x%08x\n", identity, (U32)Instance ));
#endif

#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
            STTBX_Print(("%s terminated ok\n", identity));
#endif
            SEM_UNLOCK(Lock_InitTermOpenClose);
            return(Error);
        }
        else if(Instance->InstanceChainNext == NULL)
        {       /* error we should have found a matching name before the end of the list */
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
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


#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
    STTBX_Print(("%s FAIL! this point should NEVER be reached\n", identity));
#endif
    SEM_UNLOCK(Lock_InitTermOpenClose);
    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0498_Open()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0498_Open(ST_DeviceName_t *DeviceName, DEMOD_OpenParams_t  *OpenParams, DEMOD_Capability_t *Capability, DEMOD_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
   const char *identity = "STTUNER d0498.c demod_d0498_Open()";
#endif
    ST_ErrorCode_t          Error = ST_NO_ERROR;
    D0498_InstanceData_t   *Instance;
    STTUNER_InstanceDbase_t *Inst;
    U8 index;
    U32 tempwriteback=0;

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

    /* now safe to lock semaphore */
    SEM_LOCK(Lock_InitTermOpenClose);

    /* ---------- check that at least one init has taken place ---------- */
    if(InstanceChainTop == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
        STTBX_Print(("%s fail nothing initalized\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(STTUNER_ERROR_INITSTATE);
    }

    /* find  handle from name */
    Instance = InstanceChainTop;
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
    STTBX_Print(("%s looking (%s)", identity, Instance->DeviceName));
#endif
    while(strcmp((char *)Instance->DeviceName, (char *)DeviceName) != 0)
    {
        /* error, should have found matching DeviceName before end of list */
        if(Instance->InstanceChainNext == NULL)
        {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
            STTBX_Print(("\n%s fail no block named '%s' before end of list\n", identity, DeviceName));
#endif
            SEM_UNLOCK(Lock_InitTermOpenClose);
            return(STTUNER_ERROR_INITSTATE);
        }
        Instance = Instance->InstanceChainNext;   /* next block */
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
        STTBX_Print(("(%s)", Instance->DeviceName));
#endif
    }
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
        STTBX_Print((" found ok\n"));
#endif

#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
    STTBX_Print(("%s using block at 0x%08x\n", identity, (U32)Instance));
#endif

    /* check handle IS actually free */
    if(Instance->TopLevelHandle != STTUNER_MAX_HANDLES)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
        STTBX_Print(("%s fail driver instance already open\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_OPEN_HANDLE);
    }

    /* now got pointer to free (and valid) data block */
	Inst = STTUNER_GetDrvInst();    /* pointer to instance database */
	
	
	/*Programming of PIO (1 1 0 Alternative function output Push-pull) for Tuner IF/RF AGC*/
	#if 0
	/*For PIO0 pins 2 & 3 for tuner 1(C0) IF & RF AGC - QAM0*/
	tempwriteback = STSYS_ReadRegDev32LE(PIO0_address+0x20);
	tempwriteback = tempwriteback & 0xFFFFFFF3; /*xxxx00xx*/ /*bit 2 & 3 ->0*/
    STSYS_WriteRegDev32LE(PIO0_address+0x20, tempwriteback);
    tempwriteback = STSYS_ReadRegDev32LE(PIO0_address+0x30);
    tempwriteback = tempwriteback | 0xC; /*xxxx11xx*/ /*bit 2 & 3 ->1*/
    STSYS_WriteRegDev32LE(PIO0_address+0x30, tempwriteback);
    tempwriteback = STSYS_ReadRegDev32LE(PIO0_address+0x40);
    tempwriteback = tempwriteback | 0xC; /*xxxx11xx*/ /*bit 2 & 3 ->1*/
    STSYS_WriteRegDev32LE(PIO0_address+0x40, tempwriteback);
    #endif
    
    #if 0
	/*For PIO1 pins 0 & 1 for tuner 2(C2) IF & RF AGC - QAM1*/
	tempwriteback = STSYS_ReadRegDev32LE(PIO1_address+0x20);
	tempwriteback = tempwriteback & 0xFFFFFFFC; /*xxxxxx00*/ /*bit 0 & 1 ->0*/
    STSYS_WriteRegDev32LE(PIO1_address+0x20, tempwriteback);
    tempwriteback = STSYS_ReadRegDev32LE(PIO1_address+0x30);
    tempwriteback = tempwriteback | 0x3; /*xxxxxx11*/ /*bit 0 & 1 ->1*/
    STSYS_WriteRegDev32LE(PIO1_address+0x30, tempwriteback);
    tempwriteback = STSYS_ReadRegDev32LE(PIO1_address+0x40);
    tempwriteback = tempwriteback | 0x3; /*xxxxxx11*/ /*bit 0 & 1 ->1*/
    STSYS_WriteRegDev32LE(PIO1_address+0x40, tempwriteback);
    #endif
    
    #if 1
	/*For PIO4 pins 2 & 3 for tuner 3(C4) IF & RF AGC - QAM2*/
	tempwriteback = STSYS_ReadRegDev32LE(PIO4_address+0x20);
	tempwriteback = tempwriteback & 0xFFFFFFF3; /*xxxx00xx*/ /*bit 2 & 3 ->0*/
    STSYS_WriteRegDev32LE(PIO4_address+0x20, tempwriteback);
    tempwriteback = STSYS_ReadRegDev32LE(PIO4_address+0x30);
    tempwriteback = tempwriteback | 0xC; /*xxxx11xx*/ /*bit 2 & 3 ->1*/
    STSYS_WriteRegDev32LE(PIO4_address+0x30, tempwriteback);
    tempwriteback = STSYS_ReadRegDev32LE(PIO4_address+0x40);
    tempwriteback = tempwriteback | 0xC; /*xxxx11xx*/ /*bit 2 & 3 ->1*/
    STSYS_WriteRegDev32LE(PIO4_address+0x40, tempwriteback);
    #endif
    /*End programming of PIO for Tuner IF/RF AGC*/
    
	/*Programming NQAM Cable Misc registers*/
	SetNQAMReg(NQAM_adctoqam0,    	3); // ADC C -> QAM0
	SetNQAMReg(NQAM_adctoqam1,    	2); // ADC B -> QAM1
	SetNQAMReg(NQAM_adctoqam2,    	1); // ADC A -> QAM2
	SetNQAMReg(NQAM_adctosync,    	1); // ADC A -> SYNC      
	SetNQAMReg(NQAM_m_ts0,        	2); // QAM2 -> Modem TS0
	SetNQAMReg(NQAM_m_ts1,        	1); // QAM1 -> Modem TS1
	SetNQAMReg(NQAM_m_ts2,        	0); // QAM0 -> Modem TS2
	SetNQAMReg(NQAM_v_ts0,        	2); // QAM2 -> Video TS0
	SetNQAMReg(NQAM_v_ts1,        	1); // QAM1 -> Video TS1
	SetNQAMReg(NQAM_v_ts2,        	0); // QAM0 -> Video TS2
	SetNQAMReg(NQAM_oob_sel, 		0);
	SetNQAMReg(NQAM_FRAMING_MODE, 	4);
	SetNQAMReg(NQAM_MCLK_MODE, 		0);	
	/*End programming NQAM Cable Misc registers*/


	/*
    --- wake up chip if in standby, Reset
    */
    /*  soft reset of the whole chip, 498 starts in standby mode by default */
    
    Set498QAMReg( F498_STANDBY, 1);
    
    /**************************/
    Set498QAMReg( F498_STANDBY, 0);

    /* reset all chip registers */
    
    for(index=0;index<STB0498_NBREGS;index++)
    {
   		Set498QAMReg(Addressarray[index][0],Addressarray[index][1],Addressarray[index][2], Def498Val[index]);
    }
   

    /* Set capabilties */
    Capability->FECAvail        = 0;
    Capability->ModulationAvail = STTUNER_MOD_QAM    |
                                  STTUNER_MOD_4QAM   |
                                  STTUNER_MOD_16QAM  |
                                  STTUNER_MOD_32QAM  |
                                  STTUNER_MOD_64QAM  |
                                  STTUNER_MOD_128QAM |
                                  STTUNER_MOD_256QAM;   /* direct mapping to STTUNER_Modulation_t */

    Capability->J83Avail        = STTUNER_J83_A |
                                  STTUNER_J83_C;

    Capability->AGCControl      = FALSE;
    Capability->SymbolMin       = STV0498_SYMBOLMIN;        /*   1 MegaSymbols/sec (e.g) */
    Capability->SymbolMax       = STV0498_SYMBOLMAX;        /*  11 MegaSymbols/sec (e.g) */

    Capability->BerMax           = 0;
    Capability->SignalQualityMax = STV0498_MAX_SIGNAL_QUALITY;
    Capability->AgcMax           = STV0498_MAX_AGC;


    /* TS output mode */
    switch (Instance->TSOutputMode)
    {
         case STTUNER_TS_MODE_SERIAL:
            Set498QAMReg(R498_OUTFORMAT_0, 0x09);
            Set498QAMReg(R498_FEC_AC_CTR_0, 0x1E);
            Set498QAMReg(R498_OUTFORMAT_1, 0x22);
            break;

        case STTUNER_TS_MODE_DVBCI:
            Set498QAMReg(R498_OUTFORMAT_0, 0x0A);
            Set498QAMReg(R498_FEC_AC_CTR_0, 0x16);
            Set498QAMReg(R498_OUTFORMAT_1, 0x22);
            break;

        case STTUNER_TS_MODE_PARALLEL:
        case STTUNER_TS_MODE_DEFAULT:
            Set498QAMReg(R498_OUTFORMAT_0, 0x08);
            Set498QAMReg(R498_FEC_AC_CTR_0, 0x1E);            
            Set498QAMReg(R498_OUTFORMAT_1, 0x22);

        default:
            break;
    }

    /*set data clock polarity mode (rising/falling)*/
    switch(Instance->ClockPolarity)
    {
       case STTUNER_DATA_CLOCK_POLARITY_RISING:
    	 Set498QAMReg( F498_CLK_POLARITY, 0);
    	 break;
       case STTUNER_DATA_CLOCK_POLARITY_FALLING:
    	 Set498QAMReg( F498_CLK_POLARITY, 1);
    	 break;
       case STTUNER_DATA_CLOCK_POLARITY_DEFAULT:
       default:
    	 Set498QAMReg( F498_CLK_POLARITY, 0);
    	break;
    }
    
    /*set Sync Stripping (On/Off)*/
    switch(Instance->SyncStripping)
    {
       case STTUNER_SYNC_STRIP_OFF:
    	 Set498QAMReg(F498_SYNC_STRIP, 0);
    	 break;
       case STTUNER_SYNC_STRIP_ON:
    	 Set498QAMReg( F498_SYNC_STRIP, 1);
    	 break;
	   case STTUNER_SYNC_STRIP_DEFAULT:
       default: /*OFF*/
    	 Set498QAMReg( F498_SYNC_STRIP, 0);
    	break;
    }
    
    /*set Sync Stripping (On/Off)*/
    switch(Instance->BlockSyncMode)
    {
       case STTUNER_SYNC_NORMAL:
    	 Set498QAMReg(F498_REFRESH47, 0);
    	 break;
       case STTUNER_SYNC_FORCED:
    	 Set498QAMReg(F498_REFRESH47, 1);
    	 break;
    	case STTUNER_SYNC_DEFAULT:
       default: /*FORCED*/
    	 Set498QAMReg(F498_REFRESH47, 1);
    	break;
    }

    /* Set default error count mode, bit error rate */
    Set498QAMReg( F498_CT_HOLD, 1); /* holds the counters from being updated */

    /* finally (as nor more errors to check for, allocate handle */
    Instance->TopLevelHandle = OpenParams->TopLevelHandle;
    *Handle = (U32)Instance;

#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
    STTBX_Print(("%s opened ok\n", identity));
#endif

    SEM_UNLOCK(Lock_InitTermOpenClose);
    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0498_Close()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0498_Close(DEMOD_Handle_t Handle, DEMOD_CloseParams_t *CloseParams)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
   const char *identity = "STTUNER d0498.c demod_d0498_Close()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0498_InstanceData_t     *Instance;

    /* private driver instance data */
    Instance = D0498_GetInstFromHandle(Handle);

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

    SEM_LOCK(Lock_InitTermOpenClose);

    /* ---------- check that at least one init has taken place ---------- */
    if(InstanceChainTop == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
        STTBX_Print(("%s fail nothing initalized\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(STTUNER_ERROR_INITSTATE);
    }

    if(Instance->TopLevelHandle == STTUNER_MAX_HANDLES)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
        STTBX_Print(("%s fail driver instance not open\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_OPEN_HANDLE);
    }

    /* indicate instance is closed */
    Instance->TopLevelHandle = STTUNER_MAX_HANDLES;

#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
    STTBX_Print(("%s closed\n", identity));
#endif

    SEM_UNLOCK(Lock_InitTermOpenClose);
    return(Error);
}


/* ----------------------------------------------------------------------------
Name: demod_d0498_IsAnalogCarrier()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0498_IsAnalogCarrier(DEMOD_Handle_t Handle, BOOL *IsAnalog)
{
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
    return(Error);
}


/* ----------------------------------------------------------------------------
Name: demod_d0498_GetSignalQuality()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0498_GetSignalQuality(DEMOD_Handle_t Handle, U32  *SignalQuality_p, U32 *Ber_p)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
   const char *identity = "STTUNER d0498.c demod_d0498_GetSignalQuality()";
#endif
    ST_ErrorCode_t              Error = ST_NO_ERROR;
    STTUNER_tuner_instance_t    *TunerInstance;
    STTUNER_InstanceDbase_t     *Inst;
    D0498_InstanceData_t       *Instance;
    FE_498_SignalInfo_t        pInfo;
    BOOL                        IsLocked;
    *SignalQuality_p = 0;
    *Ber_p = 0;
    
    pInfo.BER = 0;
    pInfo.CN_dBx10 = 0;

    /* private driver instance data */
    Instance = D0498_GetInstFromHandle(Handle);
    /* top level public instance data */
    Inst = STTUNER_GetDrvInst();
    /* get the tuner instance for this driver from the top level handle */
    TunerInstance = &Inst[Instance->TopLevelHandle].Cable.Tuner;

    /*
    --- Get Carrier And Data (TS) status
    */
    Error = demod_d0498_IsLocked(Handle, &IsLocked);
    if ( Error == ST_NO_ERROR && IsLocked )
    {
        /*
        --- Read Blk Counter
        */
        pInfo.SymbolRate = FE_498_GetSymbolRate(FE_498_GetMclkFreq(Instance->ExternalClock) );
        Set498QAMReg( F498_CT_HOLD, 1); /* holds the counters from being updated */
		Error = FE_498_GetSignalInfo(&pInfo);
		*Ber_p = pInfo.BER;
		*SignalQuality_p = pInfo.CN_dBx10;
		Set498QAMReg( F498_CT_HOLD, 0);
        Set498QAMReg( F498_CT_CLEAR, 0);
        Set498QAMReg( F498_CT_CLEAR, 1);
	}


    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0498_GetModulation()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0498_GetModulation(DEMOD_Handle_t Handle, STTUNER_Modulation_t *Modulation)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
   const char *identity = "STTUNER d0498.c demod_d0498_GetModulation()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0498_InstanceData_t *Instance;

    /* private driver instance data */
    Instance = D0498_GetInstFromHandle(Handle);

    switch( Get498QAMReg(F498_QAM_MODE) )
    {
    	case 0 /*000*/:
    		*Modulation = STTUNER_MOD_4QAM;
    		#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
            STTBX_Print(("%s QAM4\n", identity));
			#endif
    		break;
    		
    	case 1 /*001*/:
    		*Modulation = STTUNER_MOD_16QAM;
    		#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
            STTBX_Print(("%s QAM16\n", identity));
			#endif
    		break;
    		
    	case 2 /*010*/:
    		*Modulation = STTUNER_MOD_32QAM;
    		#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
            STTBX_Print(("%s QAM32\n", identity));
			#endif
    		break;
    		
    	case 3 /*011*/:
    		*Modulation = STTUNER_MOD_64QAM;
    		#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
            STTBX_Print(("%s QAM64\n", identity));
			#endif
    		break;
    		
    	case 4 /*100*/:
    		*Modulation = STTUNER_MOD_128QAM;
    		#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
            STTBX_Print(("%s QAM128\n", identity));
			#endif
    		break;
    		
    	case 5 /*101*/:
    		*Modulation = STTUNER_MOD_256QAM;
    		#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
            STTBX_Print(("%s QAM256\n", identity));
			#endif
    		break;
    		
    	default:
    		*Modulation = STTUNER_MOD_64QAM;
    		#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
            STTBX_Print(("%s QAM64-DEFAULT\n", identity));
			#endif
    		break;
    }

#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
    STTBX_Print(("%s Modulation=%u\n", identity, *Modulation));
#endif


    return(Error);
}


/* ----------------------------------------------------------------------------
Name: demod_d0498_GetFecType()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0498_GetFecType(DEMOD_Handle_t Handle, STTUNER_J83_t *FecType)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
   const char *identity = "STTUNER d0498.c demod_d0498_GetFecType()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0498_InstanceData_t *Instance;

    /* private driver instance data */
    Instance = D0498_GetInstFromHandle(Handle);

    switch( Get498QAMReg(F498_FEC_TYPE) )
    {
    	case 0 :
    		*FecType = FE_498_MOD_FECB;
    		#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
            STTBX_Print(("%s FEC_B\n", identity));
			#endif
    		break;
    		
    	case 1 :
    		*FecType = FE_498_MOD_FECAC;
    		#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
            STTBX_Print(("%s FEC_AC\n", identity));
			#endif
    		break;
    		
    		
    	default:
    		*FecType = FE_498_MOD_FECB;
    		#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
            STTBX_Print(("%s FEC_B-DEFAULT\n", identity));
			#endif
    		break;
    }

#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
    STTBX_Print(("%s FecType=%u\n", identity, *FecType));
#endif


    return(Error);
}


/* ----------------------------------------------------------------------------
Name: demod_d0498_StandByMode()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */



ST_ErrorCode_t demod_d0498_StandByMode(DEMOD_Handle_t Handle, STTUNER_StandByMode_t PowerMode)
{
	
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
   const char *identity = "STTUNER d0498.c demod_d0498_StandByMode()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0498_InstanceData_t *Instance;
 
    /* private driver instance data */
    Instance = D0498_GetInstFromHandle(Handle);
   
      
    switch ( PowerMode)
    {
       case STTUNER_NORMAL_POWER_MODE :
       if(Instance->StandBy_Flag == 1)
       {
          Set498QAMReg(F498_STANDBY, 0);
          
       }
          
          Instance->StandBy_Flag = 0 ;
       
       
       break;
       case STTUNER_STANDBY_POWER_MODE :
       if(Instance->StandBy_Flag == 0)
       {
          Set498QAMReg(F498_STANDBY, 1);
              
             Instance->StandBy_Flag = 1 ;
             
          
       }
       break ;
	   /* Switch statement */ 
  
    }
   
    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0498_SetModulation()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0498_SetModulation(DEMOD_Handle_t Handle, STTUNER_Modulation_t Modulation)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
   const char *identity = "STTUNER d0498.c demod_d0498_SetModulation()";
#endif
    STTUNER_InstanceDbase_t  *Inst;
    D0498_InstanceData_t   *Instance;
    ST_ErrorCode_t Error = ST_NO_ERROR;
    
    Instance = D0498_GetInstFromHandle(Handle);
    Inst = STTUNER_GetDrvInst();
        
    switch(Modulation)
    {
        case STTUNER_MOD_4QAM:
            Instance->FE_498_Modulation = FE_498_MOD_QAM4;
            break;

        case STTUNER_MOD_16QAM:
            Instance->FE_498_Modulation = FE_498_MOD_QAM16;
            break;

        case STTUNER_MOD_32QAM:
            Instance->FE_498_Modulation = FE_498_MOD_QAM32;
            break;

        case STTUNER_MOD_64QAM:
            Instance->FE_498_Modulation = FE_498_MOD_QAM64;
            break;

        case STTUNER_MOD_128QAM:
            Instance->FE_498_Modulation = FE_498_MOD_QAM128;
            break;

        case STTUNER_MOD_256QAM:
            Instance->FE_498_Modulation = FE_498_MOD_QAM256;
            break;  
            
        case STTUNER_MOD_QAM:
            Instance->FE_498_Modulation = FE_498_MOD_QAM64;
            break;
            
        default:
            return(ST_ERROR_BAD_PARAMETER);
    }

	Set498QAMReg(F498_QAM_MODE, Instance->FE_498_Modulation);

	#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
	    STTBX_Print(("%s Modulation=%u\n", identity, Modulation));
	#endif

    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0498_SetFecType()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0498_SetFecType(DEMOD_Handle_t Handle, STTUNER_J83_t FecType)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
   const char *identity = "STTUNER d0498.c demod_d0498_SetFecType()";
#endif
    STTUNER_InstanceDbase_t  *Inst;
    D0498_InstanceData_t   *Instance;
    ST_ErrorCode_t Error = ST_NO_ERROR;
    
    Instance = D0498_GetInstFromHandle(Handle);
    Inst = STTUNER_GetDrvInst();
        
    switch(FecType)
    {
        case STTUNER_J83_B:
            Instance->FecType = FE_498_MOD_FECB;
            Set498QAMReg(F498_FEC_TYPE, 0);
            break;

        case STTUNER_J83_A:
            Instance->FecType = FE_498_MOD_FECAC;
            Set498QAMReg(F498_FEC_TYPE, 1);
            break;

        case STTUNER_J83_C:
            Instance->FecType = FE_498_MOD_FECAC;
            Set498QAMReg(F498_FEC_TYPE, 1);
            break;   
            
        default:
            return(ST_ERROR_BAD_PARAMETER);
    }

	

	#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
	    STTBX_Print(("%s Fec Type=%u\n", identity, FecType));
	#endif

    return(Error);
}


/* ----------------------------------------------------------------------------
Name: demod_d0498_GetAGC()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0498_GetAGC(DEMOD_Handle_t Handle, S16  *Agc)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
   const char *identity = "STTUNER d0498.c demod_d0498_GetAGC()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0498_InstanceData_t *Instance;

    /* private driver instance data */
    Instance = D0498_GetInstFromHandle(Handle);

    *Agc = FE_498_GetRFLevel();

#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
    STTBX_Print(("%s Agc=%u\n", identity, *Agc));
#endif
    
    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0498_GetFECRates()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0498_GetFECRates(DEMOD_Handle_t Handle, STTUNER_FECRate_t *FECRates)
{
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0498_IsLocked()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0498_IsLocked(DEMOD_Handle_t Handle, BOOL *IsLocked)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
   const char *identity = "STTUNER d0498.c demod_d0498_IsLocked()";
#endif
    ST_ErrorCode_t          Error = ST_NO_ERROR;
    D0498_InstanceData_t    *Instance;

    /* private driver instance data */
    Instance = D0498_GetInstFromHandle(Handle);

    *IsLocked = Get498QAMReg(F498_QAMFEC_LOCK) ? TRUE : FALSE;
    #ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
    STTBX_Print(("%s IsLocked=%u\n", identity, *IsLocked));
    #endif    

#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
    STTBX_Print(("%s TUNER IS %s\n", identity, (*IsLocked)?"LOCKED":"UNLOCKED"));
#endif


    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0498_SetFECRates()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0498_SetFECRates(DEMOD_Handle_t Handle, STTUNER_FECRate_t FECRates)
{
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0498_Tracking()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0498_Tracking(DEMOD_Handle_t Handle, BOOL ForceTracking, U32 *NewFrequency, BOOL *SignalFound)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
   const char *identity = "STTUNER d0498.c demod_d0498_Tracking()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

    STTUNER_tuner_instance_t    *TunerInstance;     /* don't really need all these variables, done for clarity */
    STTUNER_InstanceDbase_t     *Inst;              /* pointer to instance database */
    STTUNER_Handle_t            TopHandle;          /* instance that contains the demods & tuner driver set */
    D0498_InstanceData_t       *Instance;
    BOOL                        IsLocked;
    
    TUNER_Status_t  TunerStatus; 
    U32  MasterClock;
    FE_498_SignalInfo_t	pInfo;
    

    /* private driver instance data */
    Instance = D0498_GetInstFromHandle(Handle);
    /* top level public instance data */
    Inst = STTUNER_GetDrvInst();

    /* this driver knows what to level instance it belongs to */
    TopHandle = Instance->TopLevelHandle;

    /* get the tuner instance for this driver from the top level handle */
    TunerInstance = &Inst[TopHandle].Cable.Tuner;

    Error = demod_d0498_IsLocked(Handle, &IsLocked);
    *SignalFound = IsLocked;
    if ( (Error == ST_NO_ERROR) && IsLocked )
    {
        Error = (TunerInstance->Driver->tuner_GetStatus)(TunerInstance->DrvHandle, &TunerStatus);
	   
	    MasterClock = FE_498_GetMclkFreq(Instance->ExternalClock);

	    Inst[TopHandle].CurrentTunerInfo.ScanInfo.SymbolRate = FE_498_GetSymbolRate(MasterClock);
	    pInfo.SymbolRate = Inst[TopHandle].CurrentTunerInfo.ScanInfo.SymbolRate; /*for passing to FE_498_GetSignalInfo*/
	    FE_498_GetSignalInfo(&pInfo);
	    *NewFrequency = TunerInstance->realfrequency;
	    Inst[TopHandle].CurrentTunerInfo.BitErrorRate = pInfo.BER;
	    Inst[TopHandle].CurrentTunerInfo.SignalQuality = pInfo.CN_dBx10;
	    Inst[TopHandle].CurrentTunerInfo.ScanInfo.AGC = pInfo.Power_dBmx10;
	    demod_d0498_GetModulation(Handle, &(Inst[TopHandle].CurrentTunerInfo.ScanInfo.Modulation) );
    }

#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
    STTBX_Print(("%s SignalFound=%u NewFrequency=%u\n", identity, *SignalFound, *NewFrequency));
#endif

    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0498_ScanFrequency()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0498_ScanFrequency(DEMOD_Handle_t Handle, U32  InitialFrequency, U32   SymbolRate,    U32   MaxOffset,
                                                                U32  TunerStep,        U8    DerotatorStep, BOOL *ScanSuccess,
                                                                U32 *NewFrequency,     U32   Mode,          U32   Guard,
                                                                U32  Force,            U32   Hierarchy,     U32   Spectrum,
                                                                U32  FreqOff,          U32   ChannelBW,     S32   EchoPos)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
    clock_t                     time_start_lock, time_end_lock;
    const char *identity = "STTUNER d0498.c demod_d0498_ScanFrequency()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FE_498_SearchParams_t Params;
    FE_498_SearchResult_t Result;
    STTUNER_InstanceDbase_t     *Inst;
    D0498_InstanceData_t        *Instance;
    STTUNER_Handle_t            TopHandle;
    STTUNER_tuner_instance_t    *TunerInstance;
    FE_498_SignalInfo_t	pInfo;

    /* private driver instance data */
    Instance = D0498_GetInstFromHandle(Handle);
    /* top level public instance data */
    Inst = STTUNER_GetDrvInst();
    /* this driver knows what to level instance it belongs to */
    TopHandle = Instance->TopLevelHandle;

    /* get the tuner instance for this driver from the top level handle */
    TunerInstance = &Inst[Instance->TopLevelHandle].Cable.Tuner;
    
    Params.ExternalClock = Instance->ExternalClock;
    Params.Frequency_Khz   = InitialFrequency;              /* demodulator (output from LNB) frequency (in KHz) */
    Params.SymbolRate_Bds  = SymbolRate;                    /* transponder symbol rate  (in bds) */
    Params.SearchRange_Hz = 5000000;  /*nominal 280Khz*/
    Error = demod_d0498_SetModulation(Handle, Inst[TopHandle].CurrentTunerInfo.ScanInfo.Modulation); /*fills in Instance->FE_498_Modulation*/
    Params.Modulation  = Instance->FE_498_Modulation;
    Params.SweepRate_Hz = 0;
    Error = demod_d0498_SetFecType(Handle, Inst[TopHandle].CurrentTunerInfo.ScanInfo.J83);/*fills in Instance->FE_498_MOD_FEC*/
    Params.FecType =  Instance->FecType;

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
            Inst[Instance->TopLevelHandle].Cable.Modulation = Instance->FE_498_Modulation;
            break;
    }

    Error |= FE_498_Search( &Params, &Result, Instance->TopLevelHandle );

    if (Error == FE_498_BAD_PARAMETER)  /* element(s) of Params bad */
    {
        #ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
        STTBX_Print(("%s fail, scan not done: bad parameter(s) to FE_498_Search() == FE_498_BAD_PARAMETER\n", identity ));
        #endif
        return(ST_ERROR_BAD_PARAMETER);
    }
    else if (Error == FE_498_SEARCH_FAILED)  /* found no signal within limits */
    {
	#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
        STTBX_Print(("%s FE_498_Search() == FE_498_SEARCH_FAILED\n", identity ));
 	#endif
        return(ST_NO_ERROR);    /* no error so try next F or stop if band limits reached */
    }
    *ScanSuccess = Result.Locked;

    if (Result.Locked == TRUE)
    {        
        Instance->Symbolrate = Result.SymbolRate_Bds;
        
        *NewFrequency = 1000*(Result.Frequency_Khz);
        /*lines for agc/rf level, ber,quality etc updation during initial scan/set freq before tracking is in effect, below*/
	    pInfo.SymbolRate = Result.SymbolRate_Bds; /*for passing to FE_498_GetSignalInfo*/
	    /*FE_498_GetSignalInfo(&Instance->DeviceMap, Instance->IOHandle, &pInfo);*/ /*To speedup aquisition, BER reporting only done while tracking*/
	    Inst[TopHandle].CurrentTunerInfo.BitErrorRate = 0/*pInfo.BER*/;
	    /*Inst[TopHandle].CurrentTunerInfo.SignalQuality = pInfo.CN_dBx10*/;
	    Inst[TopHandle].CurrentTunerInfo.SignalQuality = FE_498_GetCarrierToNoiseRatio_u32(Instance->FE_498_Modulation);
	    /*Inst[TopHandle].CurrentTunerInfo.ScanInfo.AGC = pInfo.Power_dBmx10;*/
	    Inst[TopHandle].CurrentTunerInfo.ScanInfo.SymbolRate=Instance->Symbolrate;
      }


#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
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

    return(Error);
}

#if 0
/* ----------------------------------------------------------------------------
Name: demod_d0498_ioctl()

Description:
    access device specific low-level functions

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0498_ioctl(DEMOD_Handle_t Handle, U32 Function, void *InParams, void *OutParams, STTUNER_Da_Status_t *Status)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
   const char *identity = "STTUNER d0498.c demod_d0498_ioctl()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0498_InstanceData_t *Instance;
    STTUNER_InstanceDbase_t  *Inst;             /* pointer to instance database */

    /* private driver instance data */
    Instance = D0498_GetInstFromHandle(Handle);
    /* top level public instance data */ 
    Inst = STTUNER_GetDrvInst();

    if(STTUNER_Util_CheckPtrNull(Instance) != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
        STTBX_Print(("%s invalid handle\n", identity));
#endif
        return(ST_ERROR_INVALID_HANDLE);
    }

#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
        STTBX_Print(("%s demod driver instance handle=%d\n", identity, Handle));
        STTBX_Print(("%s                     function=%d\n", identity, Function));
        STTBX_Print(("%s                   I/O handle=%d\n", identity, Instance->IOHandle));
#endif

    /* ---------- select what to do ---------- */
    switch(Function){

        case STTUNER_IOCTL_RAWIO: /* read/write device register (actual write to stv0498) */
            Error =  STTUNER_IOARCH_ReadWrite( Instance->IOHandle, 
                                                 ((CABIOCTL_IOARCH_Params_t *)InParams)->Operation,
                                                 ((CABIOCTL_IOARCH_Params_t *)InParams)->SubAddr,
                                                 ((CABIOCTL_IOARCH_Params_t *)InParams)->Data,
                                                 ((CABIOCTL_IOARCH_Params_t *)InParams)->TransferSize,
                                                 ((CABIOCTL_IOARCH_Params_t *)InParams)->Timeout );
            break;

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
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
            STTBX_Print(("%s function %d not found\n", identity, Function));
#endif
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }


#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
    STTBX_Print(("%s called ok\n", identity, Function));    /* signal that function came back */
#endif

    Error |= Instance->DeviceMap.Error; /* Also accumulate I2C error */
    Instance->DeviceMap.Error = ST_NO_ERROR;

    return(Error);

}
#endif

/* ----------------------------------------------------------------------------
Name: demod_d0498_ioaccess()

Description:
    called from ioarch.c when some driver does I/O but with the repeater/passthru
    set to point to this function.
Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0498_ioaccess(DEMOD_Handle_t Handle, IOARCH_Handle_t IOHandle, STTUNER_IOARCH_Operation_t Operation, U16 SubAddr, U8 *Data, U32 TransferSize, U32 Timeout)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
   const char *identity = "STTUNER d0498.c demod_d0498_ioaccess()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0498_InstanceData_t *Instance;

    Instance = D0498_GetInstFromHandle(Handle);

    if(STTUNER_Util_CheckPtrNull(Instance) != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498
        STTBX_Print(("%s invalid handle\n", identity));
#endif
        return(ST_ERROR_INVALID_HANDLE);
    }

    return(Error);
}



/* ------------------------------------------------------------------------- */
/* /\/\/\/\/\/\/\/\/\/\/\/\/\UTILITY Functions/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ */
/* ------------------------------------------------------------------------- */

D0498_InstanceData_t *D0498_GetInstFromHandle(DEMOD_Handle_t Handle)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498_HANDLES
   const char *identity = "STTUNER d0498.c D0498_GetInstFromHandle()";
#endif
    D0498_InstanceData_t *Instance;

    Instance = (D0498_InstanceData_t *)Handle;
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0498_HANDLES
    STTBX_Print(("%s block at 0x%08x\n", identity, Instance));
#endif

    return(Instance);
}


/* End of d0498.c */

