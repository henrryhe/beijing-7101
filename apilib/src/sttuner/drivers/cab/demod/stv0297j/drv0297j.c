/*----------------------------------------------------------------------------
File Name   : drv0297J.c (was drv0297.c)


Description : STV0297J front-end driver routines.

Copyright (C) 1999-2001 STMicroelectronics

   date: 01-October-2001
version: 3.2.0
 author: from STV0299 and MB validation drivers.
comment: Write for multi-instance/multi-FrontEnd.

Revision History:

Reference:
    ST API Definition "TUNER Driver API" DVD-API-06
----------------------------------------------------------------------------*/


/* Includes ---------------------------------------------------------------- */

#ifndef ST_OSLINUX
   
#include  <stdlib.h>
#include <string.h>
#endif
#include "sttbx.h"

/* C libs */


/* STAPI common includes */

/* STAPI */
#include "sttuner.h"
#include "sti2c.h"
#include "stcommon.h"
#include "stevt.h"

/* local to sttuner */
#include "util.h"       /* generic utility functions for sttuner */
#include "dbtypes.h"    /* data types for databases */
#include "sysdbase.h"   /* functions to accesss system data */

#include "ioarch.h"     /* I/O for this driver */
#include "ioreg.h"      /* I/O for this driver */

#include "reg0297j.h"    /* register mappings for the stv0297J */
#include "d0297j.h"      /* top level header for this driver */

#include "drv0297j.h"    /* header for this file */


/* defines ----------------------------------------------------------------- */

#define STV0297J_DEMOD_WAITING_TIME          5     /* In ms */
#define STV0297J_DEMOD_TRACKING_LOCK         5
/*#define STV0297J_RELOAD_DEMOD_REGISTER*/
#define STV0297J_AGC2_MAX_LEVEL              600

#define STV0297J_FAST_SWEEP_SPEED            750
#define STV0297J_SLOW_SWEEP_SPEED            400

#define STV0297J_SYMBOLRATE_LEVEL            3000000 /* When SR < STV0297J_SYMBOLRATE_LEVEL, Sweep = Sweep/2 */

/* private types ----------------------------------------------------------- */


/* constants --------------------------------------------------------------- */

/* Current LLA revision */

static ST_Revision_t Revision297J  = " STV0297J-LLA_REL_01.10 ";


/* Initialization of TDEE4 */
/* ======================= */

const U8 FECA_QAM16_TDEE4[] =
    {
    R0297J_EQU_0,          0x0a,
    0x00
    };

const U8 FECA_QAM32_TDEE4[] =
    {
    0x00
    };

const U8 FECA_QAM64_TDEE4[] =
    {
    R0297J_EQU_0,          0x48,
    R0297J_EQU_1,          0xa7,
    R0297J_EQU_3,          0x00,
    R0297J_EQU_4,          0x06,
    R0297J_EQU_5,          0x7a,
    R0297J_EQU_6,          0x00,
    R0297J_INITDEM_0,      0x51,
    R0297J_INITDEM_1,      0xb8,
    R0297J_INITDEM_2,      0x00,
    R0297J_INITDEM_3,      0x00,
    R0297J_INITDEM_4,      0x40,
    R0297J_INITDEM_5,      0x88,
    R0297J_DELAGC_0,       0xff,
    R0297J_DELAGC_1,       0x3f,
    R0297J_DELAGC_2,       0xff,
    R0297J_DELAGC_3,       0x00,
    R0297J_DELAGC_4,       0x29,
    R0297J_DELAGC_5,       0x4c,
    R0297J_DELAGC_6,       0x80,
    R0297J_DELAGC_7,       0x86,
    R0297J_WBAGC_0,        0x65,
    R0297J_WBAGC_1,        0x30,
    R0297J_WBAGC_2,        0x1b,
    R0297J_WBAGC_3,        0x18,
    R0297J_WBAGC_4,        0x99,
    R0297J_WBAGC_5,        0xb0,
    R0297J_WBAGC_6,        0x80,
    R0297J_WBAGC_7,        0x12,
    R0297J_STLOOP_1,       0x06,
    R0297J_STLOOP_2,       0x00,
    R0297J_STLOOP_3,       0x06,
    R0297J_STLOOP_4,       0x30,
    R0297J_STLOOP_5,       0x30,
    R0297J_STLOOP_6,       0x33,
    R0297J_STLOOP_7,       0x33,
    R0297J_STLOOP_8,       0x23,
    R0297J_STLOOP_9,       0x1e,
    R0297J_STLOOP_10,      0x04,
    R0297J_CRL_1,          0x48,
    R0297J_CRL_2,          0x8b,
    R0297J_CRL_3,          0x00,
    R0297J_CRL_4,          0xfe,
    R0297J_CRL_5,          0x00,
    R0297J_CRL_6,          0x00,
    R0297J_CRL_7,          0x00,
    R0297J_CRL_8,          0x26,
    R0297J_CRL_9,          0xe0,
    R0297J_CRL_10,         0x29,
    R0297J_CRL_11,         0xfe,
    R0297J_CRL_12,         0x0f,
    R0297J_CRL_13,         0x00,
    R0297J_CRL_14,         0x00,
    R0297J_PMFAGC_0,       0xff,
    R0297J_PMFAGC_1,       0x0f,
    R0297J_PMFAGC_2,       0x10,
    R0297J_PMFAGC_3,       0x00,
    R0297J_PMFAGC_4,       0x00,
    R0297J_PMFAGC_5,       0x00,
    R0297J_INTER_0,        0xff,
    R0297J_INTER_1,        0xff,
    R0297J_SIG_FAD_0,      0x17,
    R0297J_SIG_FAD_2,      0x1e,
    R0297J_SIG_FAD_3,      0x37,
    R0297J_NEW_CRL_0,      0x04,
    R0297J_NEW_CRL_1,      0x08,
    R0297J_NEW_CRL_2,      0x02,
    R0297J_NEW_CRL_3,      0x05,
    R0297J_NEW_CRL_4,      0x02,
    R0297J_NEW_CRL_5,      0x05,
    R0297J_NEW_CRL_6,      0x09,
    R0297J_FREQ_0,         0x82,
    R0297J_FREQ_1,         0x6b,
    R0297J_FREQ_2,         0x05,
    R0297J_FREQ_3,         0x06,
    R0297J_FREQ_4,         0x0a,
    R0297J_FREQ_5,         0xf0,
    R0297J_FREQ_6,         0x00,
    R0297J_FREQ_7,         0x50,
    R0297J_FREQ_11,        0x22,
    R0297J_FREQ_12,        0xb4,
    R0297J_FREQ_13,        0x00,
    R0297J_FREQ_14,        0x00,
    R0297J_FREQ_15,        0xb0,
    R0297J_FREQ_16,        0xc8,
    R0297J_FREQ_17,        0x00,
    R0297J_FREQ_18,        0x00,
    R0297J_FREQ_19,        0x00,
    R0297J_FREQ_23,        0x00,
    R0297J_FREQ_24,        0x00,
    R0297J_DEINT_SYNC_0,   0x02,
    R0297J_BERT_0,         0x85,
    R0297J_BERT_1,         0x00,
    R0297J_BERT_2,         0x00,
    R0297J_DEINT_0,        0x91,
    R0297J_DEINT_1,        0x0b,
    R0297J_RS_DESC_6,      0x00,
    R0297J_RS_DESC_7,      0x00,
    R0297J_RS_DESC_8,      0x01,
    R0297J_CTRL_0,         0xc0,
    R0297J_CTRL_1,         0x00,
    R0297J_CTRL_2,         0x20,
    R0297J_CTRL_3,         0x80,
    R0297J_CTRL_4,         0xa0,
    R0297J_CTRL_5,         0x00,
    R0297J_CTRL_6,         0x00,
    R0297J_CTRL_7,         0x00,
    R0297J_CTRL_9,         0x00,
    R0297J_CTRL_10,        0x1b, /* 58MHz */
    R0297J_CTRL_11,        0x00,
    R0297J_CTRL_12,        0x00,
    0x00
    };

const U8 FECA_QAM128_TDEE4[] =
    {
    R0297J_EQU_0,          0x29,
    0x00
    };

const U8 FECA_QAM256_TDEE4[] =
    {
    R0297J_EQU_0,          0x39,
    R0297J_EQU_1,          0x69,
    R0297J_EQU_3,          0x20,
    R0297J_EQU_4,          0x16,
    R0297J_EQU_5,          0x18,
    R0297J_EQU_6,          0xe0,
    R0297J_INITDEM_0,      0xf5,
    R0297J_INITDEM_1,      0xb8,
    R0297J_INITDEM_2,      0x00,
    R0297J_INITDEM_3,      0x00,
    R0297J_INITDEM_4,      0x00,
    R0297J_INITDEM_5,      0x88,
    R0297J_DELAGC_0,       0xff,
    R0297J_DELAGC_1,       0x3f,
    R0297J_DELAGC_2,       0xff,
    R0297J_DELAGC_3,       0x4c,
    R0297J_DELAGC_4,       0x29,
    R0297J_DELAGC_5,       0x8c,
    R0297J_DELAGC_6,       0x80,
    R0297J_DELAGC_7,       0xe0,
    R0297J_WBAGC_0,        0x0a,
    R0297J_WBAGC_1,        0x21,
    R0297J_WBAGC_2,        0x1c,
    R0297J_WBAGC_3,        0x18,
    R0297J_WBAGC_4,        0x00,
    R0297J_WBAGC_5,        0x11,
    R0297J_WBAGC_6,        0x80,
    R0297J_WBAGC_7,        0x12,
    R0297J_STLOOP_1,       0x06,
    R0297J_STLOOP_2,       0x00,
    R0297J_STLOOP_3,       0x06,
    R0297J_STLOOP_4,       0x30,
    R0297J_STLOOP_5,       0x79,
    R0297J_STLOOP_6,       0x26,
    R0297J_STLOOP_7,       0x77,
    R0297J_STLOOP_8,       0x17,
    R0297J_STLOOP_9,       0x1e,
    R0297J_STLOOP_10,      0x04,
    R0297J_CRL_1,          0x4a,
    R0297J_CRL_2,          0x0b,
    R0297J_CRL_3,          0x00,
    R0297J_CRL_4,          0xb0,
    R0297J_CRL_5,          0x00,
    R0297J_CRL_6,          0x00,
    R0297J_CRL_7,          0x00,
    R0297J_CRL_8,          0x00,
    R0297J_CRL_9,          0x00,
    R0297J_CRL_10,         0x80,
    R0297J_CRL_11,         0x04,
    R0297J_CRL_12,         0x00,
    R0297J_CRL_13,         0x00,
    R0297J_CRL_14,         0x00,
    R0297J_PMFAGC_0,       0xff,
    R0297J_PMFAGC_1,       0x04,
    R0297J_PMFAGC_2,       0x10,
    R0297J_PMFAGC_3,       0x00,
    R0297J_PMFAGC_4,       0x00,
    R0297J_PMFAGC_5,       0x00,
    R0297J_INTER_0,        0xff,
    R0297J_INTER_1,        0xff,
    R0297J_SIG_FAD_0,      0x17,
    R0297J_SIG_FAD_2,      0x1e,
    R0297J_SIG_FAD_3,      0x37,
    R0297J_NEW_CRL_0,      0x04,
    R0297J_NEW_CRL_1,      0x0a,
    R0297J_NEW_CRL_2,      0x04,
    R0297J_NEW_CRL_3,      0x07,
    R0297J_NEW_CRL_4,      0x01,
    R0297J_NEW_CRL_5,      0x06,
    R0297J_NEW_CRL_6,      0x09,
    R0297J_FREQ_0,         0xc1,
    R0297J_FREQ_1,         0x7b,
    R0297J_FREQ_2,         0x05,
    R0297J_FREQ_3,         0x06,
    R0297J_FREQ_4,         0x08,
    R0297J_FREQ_5,         0xea,
    R0297J_FREQ_6,         0x81,
    R0297J_FREQ_7,         0x98,
    R0297J_FREQ_11,        0x22,
    R0297J_FREQ_12,        0xb4,
    R0297J_FREQ_13,        0x00,
    R0297J_FREQ_14,        0x00,
    R0297J_FREQ_15,        0xb0,
    R0297J_FREQ_16,        0xc8,
    R0297J_FREQ_17,        0x00,
    R0297J_FREQ_18,        0x00,
    R0297J_FREQ_19,        0x00,
    R0297J_FREQ_23,        0x00,
    R0297J_FREQ_24,        0x00,
    R0297J_DEINT_SYNC_0,   0x02,
    R0297J_BERT_0,         0x85,
    R0297J_BERT_1,         0x1a,
    R0297J_BERT_2,         0x00,
    R0297J_DEINT_0,        0x91,
    R0297J_DEINT_1,        0x0b,
    R0297J_RS_DESC_6,      0x00,
    R0297J_RS_DESC_7,      0x00,
    R0297J_RS_DESC_8,      0x01,
    R0297J_CTRL_0,         0xc0,
    R0297J_CTRL_1,         0x00,
    R0297J_CTRL_2,         0x00,
    R0297J_CTRL_3,         0x80,
    R0297J_CTRL_4,         0x81,
    R0297J_CTRL_5,         0x00,
    R0297J_CTRL_6,         0x00,
    R0297J_CTRL_7,         0x00,
    R0297J_CTRL_9,         0x00,
    R0297J_CTRL_10,        0x1f,
    R0297J_CTRL_11,        0x00,
    R0297J_CTRL_12,        0x00,
    0x00
    };

const U8 FECB_QAM64_TDEE4[] =
    {
    R0297J_EQU_0,          0x48,
    R0297J_EQU_1,          0xa7,
    R0297J_EQU_3,          0x00,
    R0297J_EQU_4,          0x16,
    R0297J_EQU_5,          0x7a,
    R0297J_EQU_6,          0x00,
    R0297J_INITDEM_0,      0xf5,
    R0297J_INITDEM_1,      0xb8,
    R0297J_INITDEM_2,      0x00,
    R0297J_INITDEM_3,      0x00,
    R0297J_INITDEM_4,      0x40,
    R0297J_INITDEM_5,      0x88,
    R0297J_DELAGC_0,       0xff,
    R0297J_DELAGC_1,       0x3f,
    R0297J_DELAGC_2,       0xff,
    R0297J_DELAGC_3,       0x4c,
    R0297J_DELAGC_4,       0x29,
    R0297J_DELAGC_5,       0x8c,
    R0297J_DELAGC_6,       0x80,
    R0297J_DELAGC_7,       0x70,
    R0297J_WBAGC_0,        0xd4,
    R0297J_WBAGC_1,        0x20,
    R0297J_WBAGC_2,        0x1c,
    R0297J_WBAGC_3,        0x18,
    R0297J_WBAGC_4,        0x3f,
    R0297J_WBAGC_5,        0x0d,
    R0297J_WBAGC_6,        0x80,
    R0297J_WBAGC_7,        0x12,
    R0297J_STLOOP_1,       0x06,
    R0297J_STLOOP_2,       0x00,
    R0297J_STLOOP_3,       0x06,
    R0297J_STLOOP_4,       0x30,
    R0297J_STLOOP_5,       0xfa,
    R0297J_STLOOP_6,       0xee,
    R0297J_STLOOP_7,       0xe3,
    R0297J_STLOOP_8,       0x19,
    R0297J_STLOOP_9,       0x1e,
    R0297J_STLOOP_10,      0x84,
    R0297J_CRL_1,          0x48,
    R0297J_CRL_2,          0x0b,
    R0297J_CRL_3,          0x00,
    R0297J_CRL_4,          0xd1,
    R0297J_CRL_5,          0x00,
    R0297J_CRL_6,          0x00,
    R0297J_CRL_7,          0x00,
    R0297J_CRL_8,          0x00,
    R0297J_CRL_9,          0xa0,
    R0297J_CRL_10,         0x51,
    R0297J_CRL_11,         0xfa,
    R0297J_CRL_12,         0x0f,
    R0297J_CRL_13,         0x00,
    R0297J_CRL_14,         0x00,
    R0297J_PMFAGC_0,       0xff,
    R0297J_PMFAGC_1,       0x0f,
    R0297J_PMFAGC_2,       0x10,
    R0297J_PMFAGC_3,       0x00,
    R0297J_PMFAGC_4,       0x00,
    R0297J_PMFAGC_5,       0x00,
    R0297J_INTER_0,        0xff,
    R0297J_INTER_1,        0xff,
    R0297J_SIG_FAD_0,      0x17,
    R0297J_SIG_FAD_2,      0x1e,
    R0297J_SIG_FAD_3,      0x37,
    R0297J_NEW_CRL_0,      0x04,
    R0297J_NEW_CRL_1,      0x08,
    R0297J_NEW_CRL_2,      0x02,
    R0297J_NEW_CRL_3,      0x05,
    R0297J_NEW_CRL_4,      0x02,
    R0297J_NEW_CRL_5,      0x05,
    R0297J_NEW_CRL_6,      0x09,
    R0297J_FREQ_0,         0x82,
    R0297J_FREQ_1,         0x6b,
    R0297J_FREQ_2,         0x05,
    R0297J_FREQ_3,         0x06,
    R0297J_FREQ_4,         0x07,
    R0297J_FREQ_5,         0xf0,
    R0297J_FREQ_6,         0x00,
    R0297J_FREQ_7,         0x50,
    R0297J_FREQ_11,        0x22,
    R0297J_FREQ_12,        0xb4,
    R0297J_FREQ_13,        0x00,
    R0297J_FREQ_14,        0x00,
    R0297J_FREQ_15,        0xb0,
    R0297J_FREQ_16,        0xc8,
    R0297J_FREQ_17,        0x00,
    R0297J_FREQ_18,        0x00,
    R0297J_FREQ_19,        0x00,
    R0297J_FREQ_23,        0x00,
    R0297J_FREQ_24,        0x00,
    R0297J_DEINT_SYNC_0,   0x02,
    R0297J_BERT_0,         0x85,
    R0297J_BERT_1,         0xff,
    R0297J_BERT_2,         0xff,
    R0297J_DEINT_0,        0x91,
    R0297J_DEINT_1,        0x0b,
    R0297J_RS_DESC_6,      0x00,
    R0297J_RS_DESC_7,      0x00,
    R0297J_RS_DESC_8,      0xfd,
    R0297J_CTRL_0,         0x00,
    R0297J_CTRL_1,         0x11,
    R0297J_CTRL_2,         0x00,
    R0297J_CTRL_3,         0xa2,
    R0297J_CTRL_4,         0x81,
    R0297J_CTRL_5,         0x00,
    R0297J_CTRL_6,         0x00,
    R0297J_CTRL_7,         0x00,
    R0297J_CTRL_9,         0x00,
    R0297J_CTRL_10,        0x1f,
    R0297J_CTRL_11,        0x00,
    R0297J_CTRL_12,        0x00,
    R0297J_MPEG_CTRL,      0xdb,
    R0297J_MPEG_SYNC_ACQ,  0x0f,
    R0297J_MPEG_SYNC_LOSS, 0x3f,
    R0297J_VIT_SYNC_ACQ,   0x0d,
    R0297J_VIT_SYNC_LOSS,  0xdc,
    R0297J_VIT_SYNC_GO,    0x08,
    R0297J_VIT_SYNC_STOP,  0x08,
    R0297J_FS_SYNC,        0xa8,
    R0297J_IN_DEPTH,       0x81,
    R0297J_RS_CONTROL,     0xfb,
    R0297J_DEINT_CONTROL,  0x47,
    R0297J_TSMF_SEL,       0x00,
    R0297J_TSMF_CTRL,      0x00,
    R0297J_AUTOMATIC,      0x43,
    0x00
    };

const U8 FECB_QAM256_TDEE4[] =
    {
    R0297J_EQU_0,          0x39,
    R0297J_EQU_1,          0x69,
    R0297J_EQU_3,          0x20,
    R0297J_EQU_4,          0x16,
    R0297J_EQU_5,          0x18,
    R0297J_EQU_6,          0x00,
    R0297J_INITDEM_0,      0xf5,
    R0297J_INITDEM_1,      0xb8,
    R0297J_INITDEM_2,      0x00,
    R0297J_INITDEM_3,      0x00,
    R0297J_INITDEM_4,      0x00,
    R0297J_INITDEM_5,      0x88,
    R0297J_DELAGC_0,       0xff,
    R0297J_DELAGC_1,       0x3f,
    R0297J_DELAGC_2,       0xff,
    R0297J_DELAGC_3,       0x4c,
    R0297J_DELAGC_4,       0x29,
    R0297J_DELAGC_5,       0x8c,
    R0297J_DELAGC_6,       0x80,
    R0297J_DELAGC_7,       0x10,
    R0297J_WBAGC_0,        0xff,
    R0297J_WBAGC_1,        0x23,
    R0297J_WBAGC_2,        0x1c,
    R0297J_WBAGC_3,        0x18,
    R0297J_WBAGC_4,        0x47,
    R0297J_WBAGC_5,        0x03,
    R0297J_WBAGC_6,        0x80,
    R0297J_WBAGC_7,        0x12,
    R0297J_STLOOP_1,       0x06,
    R0297J_STLOOP_2,       0x00,
    R0297J_STLOOP_3,       0x06,
    R0297J_STLOOP_4,       0x30,
    R0297J_STLOOP_5,       0x7d,
    R0297J_STLOOP_6,       0x6f,
    R0297J_STLOOP_7,       0x01,
    R0297J_STLOOP_8,       0x00,
    R0297J_STLOOP_9,       0x1e,
    R0297J_STLOOP_10,      0x44,
    R0297J_CRL_1,          0x4a,
    R0297J_CRL_2,          0x0b,
    R0297J_CRL_3,          0x00,
    R0297J_CRL_4,          0x3b,
    R0297J_CRL_5,          0x48,
    R0297J_CRL_6,          0x7c,
    R0297J_CRL_7,          0xbd,
    R0297J_CRL_8,          0x00,
    R0297J_CRL_9,          0x00,
    R0297J_CRL_10,         0x2e,
    R0297J_CRL_11,         0xf4,
    R0297J_CRL_12,         0x0f,
    R0297J_CRL_13,         0x00,
    R0297J_CRL_14,         0x00,
    R0297J_PMFAGC_0,       0xff,
    R0297J_PMFAGC_1,       0x04,
    R0297J_PMFAGC_2,       0x10,
    R0297J_PMFAGC_3,       0x37,
    R0297J_PMFAGC_4,       0x02,
    R0297J_PMFAGC_5,       0x00,
    R0297J_INTER_0,        0xff,
    R0297J_INTER_1,        0xff,
    R0297J_SIG_FAD_0,      0x17,
    R0297J_SIG_FAD_2,      0x1e,
    R0297J_SIG_FAD_3,      0x37,
    R0297J_NEW_CRL_0,      0x04,
    R0297J_NEW_CRL_1,      0x0a,
    R0297J_NEW_CRL_2,      0x04,
    R0297J_NEW_CRL_3,      0x07,
    R0297J_NEW_CRL_4,      0x01,
    R0297J_NEW_CRL_5,      0x06,
    R0297J_NEW_CRL_6,      0x09,
    R0297J_FREQ_0,         0xc1,
    R0297J_FREQ_1,         0x7b,
    R0297J_FREQ_2,         0x05,
    R0297J_FREQ_3,         0x06,
    R0297J_FREQ_4,         0x08,
    R0297J_FREQ_5,         0xea,
    R0297J_FREQ_6,         0x81,
    R0297J_FREQ_7,         0x98,
    R0297J_FREQ_11,        0x22,
    R0297J_FREQ_12,        0xb4,
    R0297J_FREQ_13,        0x00,
    R0297J_FREQ_14,        0x00,
    R0297J_FREQ_15,        0xb0,
    R0297J_FREQ_16,        0xc8,
    R0297J_FREQ_17,        0x00,
    R0297J_FREQ_18,        0x00,
    R0297J_FREQ_19,        0x00,
    R0297J_FREQ_23,        0x00,
    R0297J_FREQ_24,        0x00,
    R0297J_DEINT_SYNC_0,   0x02,
    R0297J_BERT_0,         0x86,
    R0297J_BERT_1,         0xff,
    R0297J_BERT_2,         0xff,
    R0297J_DEINT_0,        0x91,
    R0297J_DEINT_1,        0x0b,
    R0297J_RS_DESC_6,      0x00,
    R0297J_RS_DESC_7,      0x00,
    R0297J_RS_DESC_8,      0x00,
    R0297J_CTRL_0,         0x00,
    R0297J_CTRL_1,         0x11,
    R0297J_CTRL_2,         0x00,
    R0297J_CTRL_3,         0x80,
    R0297J_CTRL_4,         0x81,
    R0297J_CTRL_5,         0x00,
    R0297J_CTRL_6,         0x00,
    R0297J_CTRL_7,         0x00,
    R0297J_CTRL_9,         0x00,
    R0297J_CTRL_10,        0x1f,
    R0297J_CTRL_11,        0x00,
    R0297J_CTRL_12,        0x00,
    R0297J_MPEG_CTRL,      0x1b,
    R0297J_MPEG_SYNC_ACQ,  0x0f,
    R0297J_MPEG_SYNC_LOSS, 0x3f,
    R0297J_VIT_SYNC_ACQ,   0x0d,
    R0297J_VIT_SYNC_LOSS,  0xdc,
    R0297J_VIT_SYNC_GO,    0x08,
    R0297J_VIT_SYNC_STOP,  0x08,
    R0297J_FS_SYNC,        0xa8,
    R0297J_IN_DEPTH,       0x81,
    R0297J_RS_CONTROL,     0xfd,
    R0297J_DEINT_CONTROL,  0x47,
    R0297J_TSMF_SEL,       0x00,
    R0297J_TSMF_CTRL,      0x00,
    R0297J_AUTOMATIC,      0x43,
    0x00
    };


/* Initialization of MT2050 */
/* ======================== */

#ifdef STTUNER_DRV_CAB_DOCSIS_FE
/* DOCSIS front-end board */
/* ---------------------- */

const U8 FECA_QAM16_MT2050[] =
    {
    R0297J_EQU_0,          0x0a,
    0x00
    };

const U8 FECA_QAM32_MT2050[] =
    {
    0x00
    };

const U8 FECA_QAM64_MT2050[] =
    {
    R0297J_EQU_0,          0x48,
    R0297J_EQU_1,          0xa7,
    R0297J_EQU_3,          0x00,
    R0297J_EQU_4,          0x06,
    R0297J_EQU_5,          0x7a,
    R0297J_EQU_6,          0x00,
    R0297J_INITDEM_0,      0xff,
    R0297J_INITDEM_1,      0xdf,
    R0297J_INITDEM_2,      0x00,
    R0297J_INITDEM_3,      0x00,
    R0297J_INITDEM_4,      0x40,
    R0297J_INITDEM_5,      0x88,
    R0297J_DELAGC_0,       0xff,
    R0297J_DELAGC_1,       0x00,
    R0297J_DELAGC_2,       0xff,
    R0297J_DELAGC_3,       0x27,
    R0297J_DELAGC_4,       0x21,
    R0297J_DELAGC_5,       0xff,
    R0297J_DELAGC_6,       0x80,
    R0297J_DELAGC_7,       0x90,
    R0297J_WBAGC_0,        0x3e,
    R0297J_WBAGC_1,        0x21,
    R0297J_WBAGC_2,        0x1a,
    R0297J_WBAGC_3,        0x18,
    R0297J_WBAGC_4,        0x2b,
    R0297J_WBAGC_5,        0x1f,
    R0297J_WBAGC_6,        0x80,
    R0297J_WBAGC_7,        0x12,
    R0297J_STLOOP_1,       0x06,
    R0297J_STLOOP_2,       0x00,
    R0297J_STLOOP_3,       0x08,
    R0297J_STLOOP_4,       0x30,
    R0297J_STLOOP_5,       0xac,
    R0297J_STLOOP_6,       0x25,
    R0297J_STLOOP_7,       0x32,
    R0297J_STLOOP_8,       0x23,
    R0297J_STLOOP_9,       0x1e,
    R0297J_STLOOP_10,      0x04,
    R0297J_CRL_1,          0x48,
    R0297J_CRL_2,          0x8b,
    R0297J_CRL_3,          0x00,
    R0297J_CRL_4,          0x59,
    R0297J_CRL_5,          0x00,
    R0297J_CRL_6,          0x00,
    R0297J_CRL_7,          0x00,
    R0297J_CRL_8,          0x26,
    R0297J_CRL_9,          0x00,
    R0297J_CRL_10,         0x50,
    R0297J_CRL_11,         0xf8,
    R0297J_CRL_12,         0x0f,
    R0297J_CRL_13,         0x00,
    R0297J_CRL_14,         0x00,
    R0297J_PMFAGC_0,       0xff,
    R0297J_PMFAGC_1,       0x0f,
    R0297J_PMFAGC_2,       0x10,
    R0297J_PMFAGC_3,       0x00,
    R0297J_PMFAGC_4,       0x00,
    R0297J_PMFAGC_5,       0x00,
    R0297J_INTER_0,        0xff,
    R0297J_INTER_1,        0xff,
    R0297J_SIG_FAD_0,      0x17,
    R0297J_SIG_FAD_2,      0x1e,
    R0297J_SIG_FAD_3,      0x37,
    R0297J_NEW_CRL_0,      0x04,
    R0297J_NEW_CRL_1,      0x08,
    R0297J_NEW_CRL_2,      0x02,
    R0297J_NEW_CRL_3,      0x05,
    R0297J_NEW_CRL_4,      0x02,
    R0297J_NEW_CRL_5,      0x05,
    R0297J_NEW_CRL_6,      0x09,
    R0297J_FREQ_0,         0x82,
    R0297J_FREQ_1,         0x6b,
    R0297J_FREQ_2,         0x05,
    R0297J_FREQ_3,         0x06,
    R0297J_FREQ_4,         0x0a,
    R0297J_FREQ_5,         0xf0,
    R0297J_FREQ_6,         0x00,
    R0297J_FREQ_7,         0x50,
    R0297J_FREQ_11,        0x22,
    R0297J_FREQ_12,        0xb4,
    R0297J_FREQ_13,        0x00,
    R0297J_FREQ_14,        0x00,
    R0297J_FREQ_15,        0xb0,
    R0297J_FREQ_16,        0xc8,
    R0297J_FREQ_17,        0x00,
    R0297J_FREQ_18,        0x00,
    R0297J_FREQ_19,        0x00,
    R0297J_FREQ_23,        0x00,
    R0297J_FREQ_24,        0x00,
    R0297J_DEINT_SYNC_0,   0x02,
    R0297J_BERT_0,         0x85,
    R0297J_BERT_1,         0x3e,
    R0297J_BERT_2,         0x03,
    R0297J_DEINT_0,        0x91,
    R0297J_DEINT_1,        0x0b,
    R0297J_RS_DESC_6,      0x00,
    R0297J_RS_DESC_7,      0x00,
    R0297J_RS_DESC_8,      0x01,
    R0297J_CTRL_0,         0xc0,
    R0297J_CTRL_1,         0x00,
    R0297J_CTRL_2,         0x20,
    R0297J_CTRL_3,         0x80,
    R0297J_CTRL_4,         0xa0,
    R0297J_CTRL_5,         0x00,
    R0297J_CTRL_6,         0x00,
    R0297J_CTRL_7,         0x00,
    R0297J_CTRL_9,         0x00,
    R0297J_CTRL_10,        0x1b, /* 58MHz */
    R0297J_CTRL_11,        0xee,
    R0297J_CTRL_12,        0x34,
    0x00
    };

const U8 FECA_QAM128_MT2050[] =
    {
    R0297J_EQU_0,          0x29,
    0x00
    };

const U8 FECA_QAM256_MT2050[] =
    {
    R0297J_EQU_0,          0x39,
    R0297J_EQU_1,          0x69,
    R0297J_EQU_3,          0x20,
    R0297J_EQU_4,          0x16,
    R0297J_EQU_5,          0x18,
    R0297J_EQU_6,          0x00,
    R0297J_INITDEM_0,      0xff,
    R0297J_INITDEM_1,      0xdf,
    R0297J_INITDEM_2,      0x00,
    R0297J_INITDEM_3,      0x00,
    R0297J_INITDEM_4,      0x00,
    R0297J_INITDEM_5,      0x88,
    R0297J_DELAGC_0,       0xff,
    R0297J_DELAGC_1,       0x00,
    R0297J_DELAGC_2,       0xff,
    R0297J_DELAGC_3,       0x28,
    R0297J_DELAGC_4,       0x21,
    R0297J_DELAGC_5,       0xff,
    R0297J_DELAGC_6,       0x80,
    R0297J_DELAGC_7,       0x36,
    R0297J_WBAGC_0,        0xd9,
    R0297J_WBAGC_1,        0x30,
    R0297J_WBAGC_2,        0x1b,
    R0297J_WBAGC_3,        0x18,
    R0297J_WBAGC_4,        0x72,
    R0297J_WBAGC_5,        0x70,
    R0297J_WBAGC_6,        0x80,
    R0297J_WBAGC_7,        0x12,
    R0297J_STLOOP_1,       0x06,
    R0297J_STLOOP_2,       0x00,
    R0297J_STLOOP_3,       0x08,
    R0297J_STLOOP_4,       0x30,
    R0297J_STLOOP_5,       0x64,
    R0297J_STLOOP_6,       0xd1,
    R0297J_STLOOP_7,       0x76,
    R0297J_STLOOP_8,       0x17,
    R0297J_STLOOP_9,       0x1e,
    R0297J_STLOOP_10,      0x04,
    R0297J_CRL_1,          0x4a,
    R0297J_CRL_2,          0x0b,
    R0297J_CRL_3,          0x00,
    R0297J_CRL_4,          0xe9,
    R0297J_CRL_5,          0x00,
    R0297J_CRL_6,          0x00,
    R0297J_CRL_7,          0x00,
    R0297J_CRL_8,          0x00,
    R0297J_CRL_9,          0x40,
    R0297J_CRL_10,         0x11,
    R0297J_CRL_11,         0x05,
    R0297J_CRL_12,         0x00,
    R0297J_CRL_13,         0x00,
    R0297J_CRL_14,         0x00,
    R0297J_PMFAGC_0,       0xff,
    R0297J_PMFAGC_1,       0x04,
    R0297J_PMFAGC_2,       0x10,
    R0297J_PMFAGC_3,       0x00,
    R0297J_PMFAGC_4,       0x00,
    R0297J_PMFAGC_5,       0x00,
    R0297J_INTER_0,        0xff,
    R0297J_INTER_1,        0xff,
    R0297J_SIG_FAD_0,      0x17,
    R0297J_SIG_FAD_2,      0x1e,
    R0297J_SIG_FAD_3,      0x37,
    R0297J_NEW_CRL_0,      0x04,
    R0297J_NEW_CRL_1,      0x0a,
    R0297J_NEW_CRL_2,      0x04,
    R0297J_NEW_CRL_3,      0x07,
    R0297J_NEW_CRL_4,      0x02,
    R0297J_NEW_CRL_5,      0x06,
    R0297J_NEW_CRL_6,      0x09,
    R0297J_FREQ_0,         0xc1,
    R0297J_FREQ_1,         0x7b,
    R0297J_FREQ_2,         0x05,
    R0297J_FREQ_3,         0x06,
    R0297J_FREQ_4,         0x08,
    R0297J_FREQ_5,         0xea,
    R0297J_FREQ_6,         0x81,
    R0297J_FREQ_7,         0x98,
    R0297J_FREQ_11,        0x22,
    R0297J_FREQ_12,        0xb4,
    R0297J_FREQ_13,        0x00,
    R0297J_FREQ_14,        0x00,
    R0297J_FREQ_15,        0xb0,
    R0297J_FREQ_16,        0xc8,
    R0297J_FREQ_17,        0x00,
    R0297J_FREQ_18,        0x00,
    R0297J_FREQ_19,        0x00,
    R0297J_FREQ_23,        0x00,
    R0297J_FREQ_24,        0x00,
    R0297J_DEINT_SYNC_0,   0x02,
    R0297J_BERT_0,         0x85,
    R0297J_BERT_1,         0x43,
    R0297J_BERT_2,         0x0e,
    R0297J_DEINT_0,        0x91,
    R0297J_DEINT_1,        0x0b,
    R0297J_RS_DESC_6,      0x00,
    R0297J_RS_DESC_7,      0x00,
    R0297J_RS_DESC_8,      0x01,
    R0297J_CTRL_0,         0xc0,
    R0297J_CTRL_1,         0x00,
    R0297J_CTRL_2,         0x20,
    R0297J_CTRL_3,         0x80,
    R0297J_CTRL_4,         0x81,
    R0297J_CTRL_5,         0x00,
    R0297J_CTRL_6,         0x00,
    R0297J_CTRL_7,         0x00,
    R0297J_CTRL_9,         0x00,
    R0297J_CTRL_10,        0x1b,
    R0297J_CTRL_11,        0xf3,
    R0297J_CTRL_12,        0x34,
    0x00
    };

const U8 FECB_QAM64_MT2050[] =
    {
    R0297J_EQU_0,          0x48,
    R0297J_EQU_1,          0xa7,
    R0297J_EQU_3,          0x00,
    R0297J_EQU_4,          0x06,
    R0297J_EQU_5,          0x7a,
    R0297J_EQU_6,          0x00,
    R0297J_INITDEM_0,      0xff,
    R0297J_INITDEM_1,      0xdf,
    R0297J_INITDEM_2,      0x00,
    R0297J_INITDEM_3,      0x00,
    R0297J_INITDEM_4,      0x40,
    R0297J_INITDEM_5,      0x88,
    R0297J_DELAGC_0,       0xff,
    R0297J_DELAGC_1,       0x00,
    R0297J_DELAGC_2,       0xff,
    R0297J_DELAGC_3,       0x27,
    R0297J_DELAGC_4,       0x21,
    R0297J_DELAGC_5,       0xff,
    R0297J_DELAGC_6,       0x80,
    R0297J_DELAGC_7,       0x20,
    R0297J_WBAGC_0,        0xc3,
    R0297J_WBAGC_1,        0x35,
    R0297J_WBAGC_2,        0x1a,
    R0297J_WBAGC_3,        0x18,
    R0297J_WBAGC_4,        0xb6,
    R0297J_WBAGC_5,        0xee,
    R0297J_WBAGC_6,        0x88,
    R0297J_WBAGC_7,        0x13,
    R0297J_STLOOP_1,       0x08,
    R0297J_STLOOP_2,       0x00,
    R0297J_STLOOP_3,       0x08,
    R0297J_STLOOP_4,       0x30,
    R0297J_STLOOP_5,       0xec,
    R0297J_STLOOP_6,       0x26,
    R0297J_STLOOP_7,       0x29,
    R0297J_STLOOP_8,       0x17,
    R0297J_STLOOP_9,       0x5e,
    R0297J_STLOOP_10,      0x84,
    R0297J_CRL_1,          0x48,
    R0297J_CRL_2,          0x8b,
    R0297J_CRL_3,          0x00,
    R0297J_CRL_4,          0xf3,
    R0297J_CRL_5,          0x00,
    R0297J_CRL_6,          0x00,
    R0297J_CRL_7,          0x00,
    R0297J_CRL_8,          0x26,
    R0297J_CRL_9,          0xc0,
    R0297J_CRL_10,         0x7a,
    R0297J_CRL_11,         0xf5,
    R0297J_CRL_12,         0x0f,
    R0297J_CRL_13,         0x00,
    R0297J_CRL_14,         0x00,
    R0297J_PMFAGC_0,       0xff,
    R0297J_PMFAGC_1,       0x0f,
    R0297J_PMFAGC_2,       0x10,
    R0297J_PMFAGC_3,       0x00,
    R0297J_PMFAGC_4,       0x00,
    R0297J_PMFAGC_5,       0x00,
    R0297J_INTER_0,        0xff,
    R0297J_INTER_1,        0xff,
    R0297J_SIG_FAD_0,      0x17,
    R0297J_SIG_FAD_2,      0x1e,
    R0297J_SIG_FAD_3,      0x37,
    R0297J_NEW_CRL_0,      0x04,
    R0297J_NEW_CRL_1,      0x08,
    R0297J_NEW_CRL_2,      0x02,
    R0297J_NEW_CRL_3,      0x05,
    R0297J_NEW_CRL_4,      0x02,
    R0297J_NEW_CRL_5,      0x05,
    R0297J_NEW_CRL_6,      0x09,
    R0297J_FREQ_0,         0x82,
    R0297J_FREQ_1,         0x6b,
    R0297J_FREQ_2,         0x05,
    R0297J_FREQ_3,         0x06,
    R0297J_FREQ_4,         0x0a,
    R0297J_FREQ_5,         0xf0,
    R0297J_FREQ_6,         0x00,
    R0297J_FREQ_7,         0x50,
    R0297J_FREQ_11,        0x22,
    R0297J_FREQ_12,        0xb4,
    R0297J_FREQ_13,        0x00,
    R0297J_FREQ_14,        0x00,
    R0297J_FREQ_15,        0xb0,
    R0297J_FREQ_16,        0xc8,
    R0297J_FREQ_17,        0x00,
    R0297J_FREQ_18,        0x00,
    R0297J_FREQ_19,        0x00,
    R0297J_FREQ_23,        0x00,
    R0297J_FREQ_24,        0x00,
    R0297J_DEINT_SYNC_0,   0x02,
    R0297J_BERT_0,         0x85,
    R0297J_BERT_1,         0x00,
    R0297J_BERT_2,         0x00,
    R0297J_DEINT_0,        0x91,
    R0297J_DEINT_1,        0x0b,
    R0297J_RS_DESC_6,      0x00,
    R0297J_RS_DESC_7,      0x00,
    R0297J_RS_DESC_8,      0x01,
    R0297J_CTRL_0,         0x00,
    R0297J_CTRL_1,         0x11,
    R0297J_CTRL_2,         0x20,
    R0297J_CTRL_3,         0xa2,
    R0297J_CTRL_4,         0x81,
    R0297J_CTRL_5,         0x00,
    R0297J_CTRL_6,         0x00,
    R0297J_CTRL_7,         0x00,
    R0297J_CTRL_9,         0x00,
    R0297J_CTRL_10,        0x1f,
    R0297J_CTRL_11,        0x00,
    R0297J_CTRL_12,        0x00,
    R0297J_MPEG_CTRL,      0xdb,
    R0297J_MPEG_SYNC_ACQ,  0x0f,
    R0297J_MPEG_SYNC_LOSS, 0x3f,
    R0297J_VIT_SYNC_ACQ,   0x0d,
    R0297J_VIT_SYNC_LOSS,  0xdc,
    R0297J_VIT_SYNC_GO,    0x08,
    R0297J_VIT_SYNC_STOP,  0x08,
    R0297J_FS_SYNC,        0xa8,
    R0297J_IN_DEPTH,       0x81,
    R0297J_RS_CONTROL,     0xfc,
    R0297J_TSMF_SEL,       0x00,
    R0297J_TSMF_CTRL,      0x00,
    R0297J_AUTOMATIC,      0x43,
    0x00
    };

const U8 FECB_QAM256_MT2050[] =
    {
    R0297J_EQU_0,          0x39,
    R0297J_EQU_1,          0x69,
    R0297J_EQU_3,          0x20,
    R0297J_EQU_4,          0x06,
    R0297J_EQU_5,          0x18,
    R0297J_EQU_6,          0x00,
    R0297J_INITDEM_0,      0xff,
    R0297J_INITDEM_1,      0xdf,
    R0297J_INITDEM_2,      0x00,
    R0297J_INITDEM_3,      0x00,
    R0297J_INITDEM_4,      0x00,
    R0297J_INITDEM_5,      0x88,
    R0297J_DELAGC_0,       0xff,
    R0297J_DELAGC_1,       0x00,
    R0297J_DELAGC_2,       0xff,
    R0297J_DELAGC_3,       0x30,
    R0297J_DELAGC_4,       0x21,
    R0297J_DELAGC_5,       0xff,
    R0297J_DELAGC_6,       0x80,
    R0297J_DELAGC_7,       0x86,
    R0297J_WBAGC_0,        0xad,
    R0297J_WBAGC_1,        0x35,
    R0297J_WBAGC_2,        0x1b,
    R0297J_WBAGC_3,        0x18,
    R0297J_WBAGC_4,        0x91,
    R0297J_WBAGC_5,        0x39,
    R0297J_WBAGC_6,        0x88,
    R0297J_WBAGC_7,        0x13,
    R0297J_STLOOP_1,       0x08,
    R0297J_STLOOP_2,       0x00,
    R0297J_STLOOP_3,       0x08,
    R0297J_STLOOP_4,       0x30,
    R0297J_STLOOP_5,       0xec,
    R0297J_STLOOP_6,       0x5e,
    R0297J_STLOOP_7,       0x71,
    R0297J_STLOOP_8,       0x1b,
    R0297J_STLOOP_9,       0x5e,
    R0297J_STLOOP_10,      0x44,
    R0297J_CRL_1,          0x4a,
    R0297J_CRL_2,          0x8b,
    R0297J_CRL_3,          0x00,
    R0297J_CRL_4,          0xd4,
    R0297J_CRL_5,          0x00,
    R0297J_CRL_6,          0x00,
    R0297J_CRL_7,          0x00,
    R0297J_CRL_8,          0x26,
    R0297J_CRL_9,          0xc0,
    R0297J_CRL_10,         0xbf,
    R0297J_CRL_11,         0x04,
    R0297J_CRL_12,         0x00,
    R0297J_CRL_13,         0x00,
    R0297J_CRL_14,         0x00,
    R0297J_PMFAGC_0,       0xff,
    R0297J_PMFAGC_1,       0x04,
    R0297J_PMFAGC_2,       0x10,
    R0297J_PMFAGC_3,       0x00,
    R0297J_PMFAGC_4,       0x00,
    R0297J_PMFAGC_5,       0x00,
    R0297J_INTER_0,        0xff,
    R0297J_INTER_1,        0xff,
    R0297J_SIG_FAD_0,      0x17,
    R0297J_SIG_FAD_2,      0x1e,
    R0297J_SIG_FAD_3,      0x37,
    R0297J_NEW_CRL_0,      0x04,
    R0297J_NEW_CRL_1,      0x0a,
    R0297J_NEW_CRL_2,      0x04,
    R0297J_NEW_CRL_3,      0x07,
    R0297J_NEW_CRL_4,      0x01,
    R0297J_NEW_CRL_5,      0x06,
    R0297J_NEW_CRL_6,      0x09,
    R0297J_FREQ_0,         0xc1,
    R0297J_FREQ_1,         0x7b,
    R0297J_FREQ_2,         0x05,
    R0297J_FREQ_3,         0x06,
    R0297J_FREQ_4,         0x08,
    R0297J_FREQ_5,         0xea,
    R0297J_FREQ_6,         0x81,
    R0297J_FREQ_7,         0x98,
    R0297J_FREQ_11,        0x22,
    R0297J_FREQ_12,        0xb4,
    R0297J_FREQ_13,        0x00,
    R0297J_FREQ_14,        0x00,
    R0297J_FREQ_15,        0xb0,
    R0297J_FREQ_16,        0xc8,
    R0297J_FREQ_17,        0x00,
    R0297J_FREQ_18,        0x00,
    R0297J_FREQ_19,        0x00,
    R0297J_FREQ_23,        0x00,
    R0297J_FREQ_24,        0x00,
    R0297J_DEINT_SYNC_0,   0x02,
    R0297J_BERT_0,         0x86,
    R0297J_BERT_1,         0xff,
    R0297J_BERT_2,         0xff,
    R0297J_DEINT_0,        0x91,
    R0297J_DEINT_1,        0x0b,
    R0297J_RS_DESC_6,      0x00,
    R0297J_RS_DESC_7,      0x00,
    R0297J_RS_DESC_8,      0x00,
    R0297J_CTRL_0,         0x00,
    R0297J_CTRL_1,         0x11,
    R0297J_CTRL_2,         0x20,
    R0297J_CTRL_3,         0x80,
    R0297J_CTRL_4,         0x81,
    R0297J_CTRL_5,         0x00,
    R0297J_CTRL_6,         0x00,
    R0297J_CTRL_7,         0x00,
    R0297J_CTRL_9,         0x00,
    R0297J_CTRL_10,        0x1f,
    R0297J_CTRL_11,        0x00,
    R0297J_CTRL_12,        0x00,
    R0297J_MPEG_CTRL,      0x1b,
    R0297J_MPEG_SYNC_ACQ,  0x0f,
    R0297J_MPEG_SYNC_LOSS, 0x3f,
    R0297J_VIT_SYNC_ACQ,   0x0d,
    R0297J_VIT_SYNC_LOSS,  0xdc,
    R0297J_VIT_SYNC_GO,    0x08,
    R0297J_VIT_SYNC_STOP,  0x08,
    R0297J_FS_SYNC,        0xa8,
    R0297J_IN_DEPTH,       0x81,
    R0297J_RS_CONTROL,     0xfb,
    R0297J_TSMF_SEL,       0x00,
    R0297J_TSMF_CTRL,      0x00,
    R0297J_AUTOMATIC,      0x43,
    0x00
    };

#else
/* Test board */
/* ---------- */

const U8 FECA_QAM16_MT2050[] =
    {
    R0297J_EQU_0,          0x0a,
    0x00
    };

const U8 FECA_QAM32_MT2050[] =
    {
    0x00
    };

const U8 FECA_QAM64_MT2050[] =
    {
    R0297J_EQU_0,          0x48,
    R0297J_EQU_1,          0xa7,
    R0297J_EQU_3,          0x00,
    R0297J_EQU_4,          0x06,
    R0297J_EQU_5,          0x7a,
    R0297J_EQU_6,          0x00,
    R0297J_INITDEM_0,      0x16,
    R0297J_INITDEM_1,      0xb9,
    R0297J_INITDEM_2,      0x00,
    R0297J_INITDEM_3,      0x00,
    R0297J_INITDEM_4,      0x40,
    R0297J_INITDEM_5,      0x88,
    R0297J_DELAGC_0,       0xf9,
    R0297J_DELAGC_1,       0x17,
    R0297J_DELAGC_2,       0xa6,
    R0297J_DELAGC_3,       0x1a,
    R0297J_DELAGC_4,       0x29,
    R0297J_DELAGC_5,       0x6b,
    R0297J_DELAGC_6,       0x80,
    R0297J_DELAGC_7,       0xb0,
    R0297J_WBAGC_0,        0x6d,
    R0297J_WBAGC_1,        0x21,
    R0297J_WBAGC_2,        0x1a,
    R0297J_WBAGC_3,        0x18,
    R0297J_WBAGC_4,        0x82,
    R0297J_WBAGC_5,        0xdb,
    R0297J_WBAGC_6,        0x80,
    R0297J_WBAGC_7,        0x12,
    R0297J_STLOOP_1,       0x06,
    R0297J_STLOOP_2,       0x00,
    R0297J_STLOOP_3,       0x08,
    R0297J_STLOOP_4,       0x30,
    R0297J_STLOOP_5,       0xc8,
    R0297J_STLOOP_6,       0xb9,
    R0297J_STLOOP_7,       0x32,
    R0297J_STLOOP_8,       0x23,
    R0297J_STLOOP_9,       0x1e,
    R0297J_STLOOP_10,      0x04,
    R0297J_CRL_1,          0x48,
    R0297J_CRL_2,          0x0b,
    R0297J_CRL_3,          0x00,
    R0297J_CRL_4,          0x00,
    R0297J_CRL_5,          0x00,
    R0297J_CRL_6,          0x00,
    R0297J_CRL_7,          0x00,
    R0297J_CRL_8,          0x00,
    R0297J_CRL_9,          0x00,
    R0297J_CRL_10,         0x9a,
    R0297J_CRL_11,         0x0a,
    R0297J_CRL_12,         0x00,
    R0297J_CRL_13,         0x00,
    R0297J_CRL_14,         0x00,
    R0297J_PMFAGC_0,       0xff,
    R0297J_PMFAGC_1,       0x0f,
    R0297J_PMFAGC_2,       0x10,
    R0297J_PMFAGC_3,       0x00,
    R0297J_PMFAGC_4,       0x00,
    R0297J_PMFAGC_5,       0x00,
    R0297J_INTER_0,        0xff,
    R0297J_INTER_1,        0xff,
    R0297J_SIG_FAD_0,      0x17,
    R0297J_SIG_FAD_2,      0x1e,
    R0297J_SIG_FAD_3,      0x37,
    R0297J_NEW_CRL_0,      0x04,
    R0297J_NEW_CRL_1,      0x08,
    R0297J_NEW_CRL_2,      0x02,
    R0297J_NEW_CRL_3,      0x05,
    R0297J_NEW_CRL_4,      0x02,
    R0297J_NEW_CRL_5,      0x05,
    R0297J_NEW_CRL_6,      0x09,
    R0297J_FREQ_0,         0x82,
    R0297J_FREQ_1,         0x6b,
    R0297J_FREQ_2,         0x05,
    R0297J_FREQ_3,         0x06,
    R0297J_FREQ_4,         0x0a,
    R0297J_FREQ_5,         0xf0,
    R0297J_FREQ_6,         0x00,
    R0297J_FREQ_7,         0x50,
    R0297J_FREQ_11,        0x22,
    R0297J_FREQ_12,        0xb4,
    R0297J_FREQ_13,        0x00,
    R0297J_FREQ_14,        0x00,
    R0297J_FREQ_15,        0xb0,
    R0297J_FREQ_16,        0xc8,
    R0297J_FREQ_17,        0x00,
    R0297J_FREQ_18,        0x00,
    R0297J_FREQ_19,        0x00,
    R0297J_FREQ_23,        0x00,
    R0297J_FREQ_24,        0x00,
    R0297J_DEINT_SYNC_0,   0x02,
    R0297J_BERT_0,         0x85,
    R0297J_BERT_1,         0x5e,
    R0297J_BERT_2,         0x00,
    R0297J_DEINT_0,        0x91,
    R0297J_DEINT_1,        0x0b,
    R0297J_RS_DESC_6,      0x80,
    R0297J_RS_DESC_7,      0x00,
    R0297J_RS_DESC_8,      0x01,
    R0297J_CTRL_0,         0xc0,
    R0297J_CTRL_1,         0x00,
    R0297J_CTRL_2,         0x20,
    R0297J_CTRL_3,         0x80,
    R0297J_CTRL_4,         0x80,
    R0297J_CTRL_5,         0x00,
    R0297J_CTRL_6,         0x00,
    R0297J_CTRL_7,         0x00,
    R0297J_CTRL_9,         0x00,
    R0297J_CTRL_10,        0x1b, /* 58MHz */
    R0297J_CTRL_11,        0xf3,
    R0297J_CTRL_12,        0x34,
    0x00
    };

const U8 FECA_QAM128_MT2050[] =
    {
    R0297J_EQU_0,          0x29,
    0x00
    };

const U8 FECA_QAM256_MT2050[] =
    {
    R0297J_EQU_0,          0x39,
    R0297J_EQU_1,          0x69,
    R0297J_EQU_3,          0x20,
    R0297J_EQU_4,          0x16,
    R0297J_EQU_5,          0x18,
    R0297J_EQU_6,          0xe0,
    R0297J_INITDEM_0,      0xf5,
    R0297J_INITDEM_1,      0xb8,
    R0297J_INITDEM_2,      0x00,
    R0297J_INITDEM_3,      0x00,
    R0297J_INITDEM_4,      0x00,
    R0297J_INITDEM_5,      0x88,
    R0297J_DELAGC_0,       0xff,
    R0297J_DELAGC_1,       0x6b,
    R0297J_DELAGC_2,       0xff,
    R0297J_DELAGC_3,       0x4c,
    R0297J_DELAGC_4,       0x29,
    R0297J_DELAGC_5,       0x8c,
    R0297J_DELAGC_6,       0x80,
    R0297J_DELAGC_7,       0xe0,
    R0297J_WBAGC_0,        0x0a,
    R0297J_WBAGC_1,        0x21,
    R0297J_WBAGC_2,        0x1c,
    R0297J_WBAGC_3,        0x18,
    R0297J_WBAGC_4,        0x00,
    R0297J_WBAGC_5,        0x11,
    R0297J_WBAGC_6,        0x80,
    R0297J_WBAGC_7,        0x12,
    R0297J_STLOOP_1,       0x06,
    R0297J_STLOOP_2,       0x00,
    R0297J_STLOOP_3,       0x06,
    R0297J_STLOOP_4,       0x30,
    R0297J_STLOOP_5,       0x79,
    R0297J_STLOOP_6,       0x26,
    R0297J_STLOOP_7,       0x77,
    R0297J_STLOOP_8,       0x17,
    R0297J_STLOOP_9,       0x1e,
    R0297J_STLOOP_10,      0x04,
    R0297J_CRL_1,          0x4a,
    R0297J_CRL_2,          0x0b,
    R0297J_CRL_3,          0x00,
    R0297J_CRL_4,          0xb0,
    R0297J_CRL_5,          0x00,
    R0297J_CRL_6,          0x00,
    R0297J_CRL_7,          0x00,
    R0297J_CRL_8,          0x00,
    R0297J_CRL_9,          0x00,
    R0297J_CRL_10,         0x80,
    R0297J_CRL_11,         0x04,
    R0297J_CRL_12,         0x00,
    R0297J_CRL_13,         0x00,
    R0297J_CRL_14,         0x00,
    R0297J_PMFAGC_0,       0xff,
    R0297J_PMFAGC_1,       0x04,
    R0297J_PMFAGC_2,       0x10,
    R0297J_PMFAGC_3,       0x00,
    R0297J_PMFAGC_4,       0x00,
    R0297J_PMFAGC_5,       0x00,
    R0297J_INTER_0,        0xff,
    R0297J_INTER_1,        0xff,
    R0297J_SIG_FAD_0,      0x17,
    R0297J_SIG_FAD_2,      0x1e,
    R0297J_SIG_FAD_3,      0x37,
    R0297J_NEW_CRL_0,      0x04,
    R0297J_NEW_CRL_1,      0x0a,
    R0297J_NEW_CRL_2,      0x04,
    R0297J_NEW_CRL_3,      0x07,
    R0297J_NEW_CRL_4,      0x01,
    R0297J_NEW_CRL_5,      0x06,
    R0297J_NEW_CRL_6,      0x09,
    R0297J_FREQ_0,         0xc1,
    R0297J_FREQ_1,         0x7b,
    R0297J_FREQ_2,         0x05,
    R0297J_FREQ_3,         0x06,
    R0297J_FREQ_4,         0x08,
    R0297J_FREQ_5,         0xea,
    R0297J_FREQ_6,         0x81,
    R0297J_FREQ_7,         0x98,
    R0297J_FREQ_11,        0x22,
    R0297J_FREQ_12,        0xb4,
    R0297J_FREQ_13,        0x00,
    R0297J_FREQ_14,        0x00,
    R0297J_FREQ_15,        0xb0,
    R0297J_FREQ_16,        0xc8,
    R0297J_FREQ_17,        0x00,
    R0297J_FREQ_18,        0x00,
    R0297J_FREQ_19,        0x00,
    R0297J_FREQ_23,        0x00,
    R0297J_FREQ_24,        0x00,
    R0297J_DEINT_SYNC_0,   0x02,
    R0297J_BERT_0,         0x85,
    R0297J_BERT_1,         0x1a,
    R0297J_BERT_2,         0x00,
    R0297J_DEINT_0,        0x91,
    R0297J_DEINT_1,        0x0b,
    R0297J_RS_DESC_6,      0x00,
    R0297J_RS_DESC_7,      0x00,
    R0297J_RS_DESC_8,      0x01,
    R0297J_CTRL_0,         0xc0,
    R0297J_CTRL_1,         0x00,
    R0297J_CTRL_2,         0x00,
    R0297J_CTRL_3,         0x80,
    R0297J_CTRL_4,         0x81,
    R0297J_CTRL_5,         0x00,
    R0297J_CTRL_6,         0x00,
    R0297J_CTRL_7,         0x00,
    R0297J_CTRL_9,         0x00,
    R0297J_CTRL_10,        0x1b, /* 58MHz */
    R0297J_CTRL_11,        0x00,
    R0297J_CTRL_12,        0x00,
    0x00
    };

const U8 FECB_QAM64_MT2050 [] =
    {
    R0297J_EQU_0,          0x48,
    0x00
    };

const U8 FECB_QAM256_MT2050 [] =
    {
    R0297J_EQU_0,          0x39,
    0x00
    };
#endif

/* Initialization of DCT7040 */
/* ========================= */

const U8 FECA_QAM16_DCT7040[] =
    {
    R0297J_EQU_0,          0x0a,
    R0297J_EQU_1,          0x58,
    R0297J_EQU_3,          0x00,
    R0297J_EQU_4,          0x16,
    R0297J_EQU_5,          0x99,
    R0297J_EQU_6,          0x31,
    R0297J_INITDEM_0,      0x51,
    R0297J_INITDEM_1,      0xb8,
    R0297J_INITDEM_2,      0x00,
    R0297J_INITDEM_3,      0x00,
    R0297J_INITDEM_4,      0x40,
    R0297J_INITDEM_5,      0x88,
    R0297J_DELAGC_0,       0xff,
    R0297J_DELAGC_1,       0x3f,
    R0297J_DELAGC_2,       0xff,
    R0297J_DELAGC_3,       0x00,
    R0297J_DELAGC_4,       0x29,
    R0297J_DELAGC_5,       0x4c,
    R0297J_DELAGC_6,       0x80,
    R0297J_DELAGC_7,       0xc6,
    R0297J_WBAGC_0,        0x43,
    R0297J_WBAGC_1,        0x30,
    R0297J_WBAGC_2,        0x1b,
    R0297J_WBAGC_3,        0x18,
    R0297J_WBAGC_4,        0x7d,
    R0297J_WBAGC_5,        0x38,
    R0297J_WBAGC_6,        0x80,
    R0297J_WBAGC_7,        0x12,
    R0297J_STLOOP_1,       0x06,
    R0297J_STLOOP_2,       0x00,
    R0297J_STLOOP_3,       0x06,
    R0297J_STLOOP_4,       0x30,
    R0297J_STLOOP_5,       0xb0,
    R0297J_STLOOP_6,       0x45,
    R0297J_STLOOP_7,       0x98,
    R0297J_STLOOP_8,       0x23,
    R0297J_STLOOP_9,       0x1e,
    R0297J_STLOOP_10,      0x04,
    R0297J_CRL_1,          0x48,
    R0297J_CRL_2,          0x0b,
    R0297J_CRL_3,          0x00,
    R0297J_CRL_4,          0x1a,
    R0297J_CRL_5,          0x00,
    R0297J_CRL_6,          0x00,
    R0297J_CRL_7,          0x00,
    R0297J_CRL_8,          0x00,
    R0297J_CRL_9,          0x38,
    R0297J_CRL_10,         0xf8,
    R0297J_CRL_11,         0xfa,
    R0297J_CRL_12,         0x0f,
    R0297J_CRL_13,         0x00,
    R0297J_CRL_14,         0x00,
    R0297J_PMFAGC_0,       0xff,
    R0297J_PMFAGC_1,       0x04,
    R0297J_PMFAGC_2,       0x10,
    R0297J_PMFAGC_3,       0x00,
    R0297J_PMFAGC_4,       0x00,
    R0297J_PMFAGC_5,       0x00,
    R0297J_INTER_0,        0xff,
    R0297J_INTER_1,        0xff,
    R0297J_SIG_FAD_0,      0x16,
    R0297J_SIG_FAD_2,      0x1e,
    R0297J_SIG_FAD_3,      0x37,
    R0297J_NEW_CRL_0,      0x04,
    R0297J_NEW_CRL_1,      0x08,
    R0297J_NEW_CRL_2,      0x02,
    R0297J_NEW_CRL_3,      0x05,
    R0297J_NEW_CRL_4,      0x01,
    R0297J_NEW_CRL_5,      0x05,
    R0297J_NEW_CRL_6,      0x09,
    R0297J_FREQ_0,         0x02,
    R0297J_FREQ_1,         0x64,
    R0297J_FREQ_2,         0x05,
    R0297J_FREQ_3,         0x05,
    R0297J_FREQ_4,         0x08,
    R0297J_FREQ_5,         0x62,
    R0297J_FREQ_6,         0x00,
    R0297J_FREQ_7,         0x23,
    R0297J_FREQ_11,        0x22,
    R0297J_FREQ_12,        0xb4,
    R0297J_FREQ_13,        0x00,
    R0297J_FREQ_14,        0x00,
    R0297J_FREQ_15,        0xb0,
    R0297J_FREQ_16,        0xc8,
    R0297J_FREQ_17,        0x00,
    R0297J_FREQ_18,        0x00,
    R0297J_FREQ_19,        0x00,
    R0297J_FREQ_23,        0x00,
    R0297J_FREQ_24,        0x00,
    R0297J_DEINT_SYNC_0,   0x02,
    R0297J_BERT_0,         0x85,
    R0297J_BERT_1,         0xfa,
    R0297J_BERT_2,         0x0c,
    R0297J_DEINT_0,        0x91,
    R0297J_DEINT_1,        0x0b,
    R0297J_RS_DESC_6,      0x00,
    R0297J_RS_DESC_7,      0x00,
    R0297J_RS_DESC_8,      0x01,
    R0297J_CTRL_0,         0xc0,
    R0297J_CTRL_1,         0x00,
    R0297J_CTRL_2,         0x20,
    R0297J_CTRL_3,         0x80,
    R0297J_CTRL_4,         0x81,
    R0297J_CTRL_5,         0x00,
    R0297J_CTRL_6,         0x00,
    R0297J_CTRL_7,         0x00,
    R0297J_CTRL_9,         0x00,
    R0297J_CTRL_10,        0x1f,
    R0297J_CTRL_11,        0x00,
    R0297J_CTRL_12,        0x00,
    0x00
    };

const U8 FECA_QAM32_DCT7040[] =
    {
    R0297J_EQU_0,          0x19,
    R0297J_EQU_1,          0x69,
    R0297J_EQU_3,          0x20,
    R0297J_EQU_4,          0x06,
    R0297J_EQU_5,          0xf5,
    R0297J_EQU_6,          0x10,
    R0297J_INITDEM_0,      0x51,
    R0297J_INITDEM_1,      0xb8,
    R0297J_INITDEM_2,      0x00,
    R0297J_INITDEM_3,      0x00,
    R0297J_INITDEM_4,      0x40,
    R0297J_INITDEM_5,      0x88,
    R0297J_DELAGC_0,       0xff,
    R0297J_DELAGC_1,       0x3f,
    R0297J_DELAGC_2,       0xff,
    R0297J_DELAGC_3,       0x00,
    R0297J_DELAGC_4,       0x29,
    R0297J_DELAGC_5,       0x4c,
    R0297J_DELAGC_6,       0x80,
    R0297J_DELAGC_7,       0xe6,
    R0297J_WBAGC_0,        0x42,
    R0297J_WBAGC_1,        0x30,
    R0297J_WBAGC_2,        0x1b,
    R0297J_WBAGC_3,        0x18,
    R0297J_WBAGC_4,        0x8a,
    R0297J_WBAGC_5,        0x5b,
    R0297J_WBAGC_6,        0x80,
    R0297J_WBAGC_7,        0x12,
    R0297J_STLOOP_1,       0x06,
    R0297J_STLOOP_2,       0x00,
    R0297J_STLOOP_3,       0x06,
    R0297J_STLOOP_4,       0x30,
    R0297J_STLOOP_5,       0x60,
    R0297J_STLOOP_6,       0x00,
    R0297J_STLOOP_7,       0x00,
    R0297J_STLOOP_8,       0x00,
    R0297J_STLOOP_9,       0x1e,
    R0297J_STLOOP_10,      0x04,
    R0297J_CRL_1,          0x5a,
    R0297J_CRL_2,          0x0b,
    R0297J_CRL_3,          0x02,
    R0297J_CRL_4,          0x00,
    R0297J_CRL_5,          0x00,
    R0297J_CRL_6,          0x00,
    R0297J_CRL_7,          0x00,
    R0297J_CRL_8,          0x00,
    R0297J_CRL_9,          0xa0,
    R0297J_CRL_10,         0x2c,
    R0297J_CRL_11,         0xfb,
    R0297J_CRL_12,         0x0f,
    R0297J_CRL_13,         0x00,
    R0297J_CRL_14,         0x00,
    R0297J_PMFAGC_0,       0xff,
    R0297J_PMFAGC_1,       0x04,
    R0297J_PMFAGC_2,       0x10,
    R0297J_PMFAGC_3,       0x00,
    R0297J_PMFAGC_4,       0x00,
    R0297J_PMFAGC_5,       0x00,
    R0297J_INTER_0,        0xff,
    R0297J_INTER_1,        0xff,
    R0297J_SIG_FAD_0,      0x16,
    R0297J_SIG_FAD_2,      0x1e,
    R0297J_SIG_FAD_3,      0x37,
    R0297J_NEW_CRL_0,      0x04,
    R0297J_NEW_CRL_1,      0x09,
    R0297J_NEW_CRL_2,      0x03,
    R0297J_NEW_CRL_3,      0x06,
    R0297J_NEW_CRL_4,      0x02,
    R0297J_NEW_CRL_5,      0x05,
    R0297J_NEW_CRL_6,      0x08,
    R0297J_FREQ_0,         0x02,
    R0297J_FREQ_1,         0x73,
    R0297J_FREQ_2,         0x04,
    R0297J_FREQ_3,         0x05,
    R0297J_FREQ_4,         0x07,
    R0297J_FREQ_5,         0x66,
    R0297J_FREQ_6,         0x00,
    R0297J_FREQ_7,         0x1c,
    R0297J_FREQ_11,        0x22,
    R0297J_FREQ_12,        0xb4,
    R0297J_FREQ_13,        0x00,
    R0297J_FREQ_14,        0x00,
    R0297J_FREQ_15,        0x81,
    R0297J_FREQ_16,        0xc8,
    R0297J_FREQ_17,        0x00,
    R0297J_FREQ_18,        0x00,
    R0297J_FREQ_19,        0x00,
    R0297J_FREQ_23,        0x00,
    R0297J_FREQ_24,        0x00,
    R0297J_DEINT_SYNC_0,   0x02,
    R0297J_BERT_0,         0x85,
    R0297J_BERT_1,         0x00,
    R0297J_BERT_2,         0x00,
    R0297J_DEINT_0,        0x91,
    R0297J_DEINT_1,        0x0b,
    R0297J_RS_DESC_6,      0x00,
    R0297J_RS_DESC_7,      0x00,
    R0297J_RS_DESC_8,      0x01,
    R0297J_CTRL_0,         0xc0,
    R0297J_CTRL_1,         0x00,
    R0297J_CTRL_2,         0x20,
    R0297J_CTRL_3,         0x80,
    R0297J_CTRL_4,         0x81,
    R0297J_CTRL_5,         0x00,
    R0297J_CTRL_6,         0x00,
    R0297J_CTRL_7,         0x00,
    R0297J_CTRL_9,         0x00,
    R0297J_CTRL_10,        0x1f,
    R0297J_CTRL_11,        0x00,
    R0297J_CTRL_12,        0x00,
    0x00
    };

const U8 FECA_QAM64_DCT7040[] =
    {
    R0297J_EQU_0,          0x48,
    R0297J_EQU_1,          0xa7,
    R0297J_EQU_3,          0x00,
    R0297J_EQU_4,          0x06,
    R0297J_EQU_5,          0x7a,
    R0297J_EQU_6,          0x00,
    R0297J_INITDEM_0,      0x7f,
    R0297J_INITDEM_1,      0xb8,
    R0297J_INITDEM_2,      0x00,
    R0297J_INITDEM_3,      0x00,
    R0297J_INITDEM_4,      0x40,
    R0297J_INITDEM_5,      0x88,
    R0297J_DELAGC_0,       0xff,
    R0297J_DELAGC_1,       0x3f,
    R0297J_DELAGC_2,       0xff,
    R0297J_DELAGC_3,       0x00,
    R0297J_DELAGC_4,       0x29,
    R0297J_DELAGC_5,       0x4c,
    R0297J_DELAGC_6,       0x80,
    R0297J_DELAGC_7,       0xd6,
    R0297J_WBAGC_0,        0x37,
    R0297J_WBAGC_1,        0x34,
    R0297J_WBAGC_2,        0x1b,
    R0297J_WBAGC_3,        0x18,
    R0297J_WBAGC_4,        0x99,
    R0297J_WBAGC_5,        0xb0,
    R0297J_WBAGC_6,        0x88,
    R0297J_WBAGC_7,        0x13,
    R0297J_STLOOP_1,       0x08,
    R0297J_STLOOP_2,       0x00,
    R0297J_STLOOP_3,       0x08,
    R0297J_STLOOP_4,       0x30,
    R0297J_STLOOP_5,       0xcc,
    R0297J_STLOOP_6,       0x50,
    R0297J_STLOOP_7,       0x33,
    R0297J_STLOOP_8,       0x23,
    R0297J_STLOOP_9,       0x5e,
    R0297J_STLOOP_10,      0x04,
    R0297J_CRL_1,          0x48,
    R0297J_CRL_2,          0x8b,
    R0297J_CRL_3,          0x00,
    R0297J_CRL_4,          0xa6,
    R0297J_CRL_5,          0x44,
    R0297J_CRL_6,          0xf6,
    R0297J_CRL_7,          0x74,
    R0297J_CRL_8,          0x26,
    R0297J_CRL_9,          0x80,
    R0297J_CRL_10,         0xee,
    R0297J_CRL_11,         0x04,
    R0297J_CRL_12,         0x00,
    R0297J_CRL_13,         0x00,
    R0297J_CRL_14,         0x00,
    R0297J_PMFAGC_0,       0xff,
    R0297J_PMFAGC_1,       0x0f,
    R0297J_PMFAGC_2,       0x10,
    R0297J_PMFAGC_3,       0x00,
    R0297J_PMFAGC_4,       0x00,
    R0297J_PMFAGC_5,       0x00,
    R0297J_INTER_0,        0xff,
    R0297J_INTER_1,        0xff,
    R0297J_SIG_FAD_0,      0x17,
    R0297J_SIG_FAD_2,      0x1e,
    R0297J_SIG_FAD_3,      0x37,
    R0297J_NEW_CRL_0,      0x04,
    R0297J_NEW_CRL_1,      0x08,
    R0297J_NEW_CRL_2,      0x02,
    R0297J_NEW_CRL_3,      0x05,
    R0297J_NEW_CRL_4,      0x02,
    R0297J_NEW_CRL_5,      0x05,
    R0297J_NEW_CRL_6,      0x09,
    R0297J_FREQ_0,         0x82,
    R0297J_FREQ_1,         0x6b,
    R0297J_FREQ_2,         0x05,
    R0297J_FREQ_3,         0x06,
    R0297J_FREQ_4,         0x0a,
    R0297J_FREQ_5,         0xf0,
    R0297J_FREQ_6,         0x00,
    R0297J_FREQ_7,         0x50,
    R0297J_FREQ_11,        0x22,
    R0297J_FREQ_12,        0xb4,
    R0297J_FREQ_13,        0x00,
    R0297J_FREQ_14,        0x00,
    R0297J_FREQ_15,        0xb0,
    R0297J_FREQ_16,        0xc8,
    R0297J_FREQ_17,        0x00,
    R0297J_FREQ_18,        0x00,
    R0297J_FREQ_19,        0x00,
    R0297J_FREQ_23,        0x00,
    R0297J_FREQ_24,        0x00,
    R0297J_DEINT_SYNC_0,   0x02,
    R0297J_BERT_0,         0x85,
    R0297J_BERT_1,         0x00,
    R0297J_BERT_2,         0x00,
    R0297J_DEINT_0,        0x91,
    R0297J_DEINT_1,        0x0b,
    R0297J_RS_DESC_6,      0x00,
    R0297J_RS_DESC_7,      0x00,
    R0297J_RS_DESC_8,      0x01,
    R0297J_CTRL_0,         0xc0,
    R0297J_CTRL_1,         0x00,
    R0297J_CTRL_2,         0x20,
    R0297J_CTRL_3,         0x80,
    R0297J_CTRL_4,         0xa0,
    R0297J_CTRL_5,         0x00,
    R0297J_CTRL_6,         0x00,
    R0297J_CTRL_7,         0x00,
    R0297J_CTRL_9,         0x00,
    R0297J_CTRL_10,        0x1b, /* 58MHz */
    R0297J_CTRL_11,        0xf7,
    R0297J_CTRL_12,        0x34,
    0x00
    };

const U8 FECA_QAM128_DCT7040[] =
    {
    R0297J_EQU_0,          0x29,
    R0297J_EQU_1,          0x89,
    R0297J_EQU_3,          0x00,
    R0297J_EQU_4,          0x06,
    R0297J_EQU_5,          0x1e,
    R0297J_EQU_6,          0x70,
    R0297J_INITDEM_0,      0x51,
    R0297J_INITDEM_1,      0xb8,
    R0297J_INITDEM_2,      0x00,
    R0297J_INITDEM_3,      0x00,
    R0297J_INITDEM_4,      0x40,
    R0297J_INITDEM_5,      0x88,
    R0297J_DELAGC_0,       0xff,
    R0297J_DELAGC_1,       0x3f,
    R0297J_DELAGC_2,       0xff,
    R0297J_DELAGC_3,       0x00,
    R0297J_DELAGC_4,       0x29,
    R0297J_DELAGC_5,       0x4c,
    R0297J_DELAGC_6,       0x80,
    R0297J_DELAGC_7,       0x96,
    R0297J_WBAGC_0,        0x43,
    R0297J_WBAGC_1,        0x30,
    R0297J_WBAGC_2,        0x1b,
    R0297J_WBAGC_3,        0x18,
    R0297J_WBAGC_4,        0x09,
    R0297J_WBAGC_5,        0x21,
    R0297J_WBAGC_6,        0x80,
    R0297J_WBAGC_7,        0x12,
    R0297J_STLOOP_1,       0x06,
    R0297J_STLOOP_2,       0x00,
    R0297J_STLOOP_3,       0x06,
    R0297J_STLOOP_4,       0x30,
    R0297J_STLOOP_5,       0x47,
    R0297J_STLOOP_6,       0xfc,
    R0297J_STLOOP_7,       0xe4,
    R0297J_STLOOP_8,       0x09,
    R0297J_STLOOP_9,       0x1e,
    R0297J_STLOOP_10,      0x04,
    R0297J_CRL_1,          0x4a,
    R0297J_CRL_2,          0x7b,
    R0297J_CRL_3,          0x00,
    R0297J_CRL_4,          0x2a,
    R0297J_CRL_5,          0x00,
    R0297J_CRL_6,          0x00,
    R0297J_CRL_7,          0x00,
    R0297J_CRL_8,          0x28,
    R0297J_CRL_9,          0x18,
    R0297J_CRL_10,         0xa6,
    R0297J_CRL_11,         0xff,
    R0297J_CRL_12,         0x0f,
    R0297J_CRL_13,         0x26,
    R0297J_CRL_14,         0x00,
    R0297J_PMFAGC_0,       0xff,
    R0297J_PMFAGC_1,       0x04,
    R0297J_PMFAGC_2,       0x10,
    R0297J_PMFAGC_3,       0x00,
    R0297J_PMFAGC_4,       0x00,
    R0297J_PMFAGC_5,       0x00,
    R0297J_INTER_0,        0xff,
    R0297J_INTER_1,        0xff,
    R0297J_SIG_FAD_0,      0x16,
    R0297J_SIG_FAD_2,      0x1e,
    R0297J_SIG_FAD_3,      0x37,
    R0297J_NEW_CRL_0,      0x04,
    R0297J_NEW_CRL_1,      0x0a,
    R0297J_NEW_CRL_2,      0x04,
    R0297J_NEW_CRL_3,      0x06,
    R0297J_NEW_CRL_4,      0x02,
    R0297J_NEW_CRL_5,      0x05,
    R0297J_NEW_CRL_6,      0x08,
    R0297J_FREQ_0,         0x01,
    R0297J_FREQ_1,         0x37,
    R0297J_FREQ_2,         0x04,
    R0297J_FREQ_3,         0x04,
    R0297J_FREQ_4,         0x06,
    R0297J_FREQ_5,         0x66,
    R0297J_FREQ_6,         0x80,
    R0297J_FREQ_7,         0x1d,
    R0297J_FREQ_11,        0x12,
    R0297J_FREQ_12,        0xb4,
    R0297J_FREQ_13,        0x00,
    R0297J_FREQ_14,        0x00,
    R0297J_FREQ_15,        0x81,
    R0297J_FREQ_16,        0xc8,
    R0297J_FREQ_17,        0x00,
    R0297J_FREQ_18,        0x00,
    R0297J_FREQ_19,        0x00,
    R0297J_FREQ_23,        0x00,
    R0297J_FREQ_24,        0x00,
    R0297J_DEINT_SYNC_0,   0x02,
    R0297J_BERT_0,         0x86,
    R0297J_BERT_1,         0xf5,
    R0297J_BERT_2,         0x14,
    R0297J_DEINT_0,        0x91,
    R0297J_DEINT_1,        0x0b,
    R0297J_RS_DESC_6,      0x00,
    R0297J_RS_DESC_7,      0x00,
    R0297J_RS_DESC_8,      0x01,
    R0297J_CTRL_0,         0xc0,
    R0297J_CTRL_1,         0x00,
    R0297J_CTRL_2,         0x20,
    R0297J_CTRL_3,         0x80,
    R0297J_CTRL_4,         0x81,
    R0297J_CTRL_5,         0x00,
    R0297J_CTRL_6,         0x00,
    R0297J_CTRL_7,         0x00,
    R0297J_CTRL_9,         0x00,
    R0297J_CTRL_10,        0x1f,
    R0297J_CTRL_11,        0x00,
    R0297J_CTRL_12,        0x00,
    0x00
    };

const U8 FECA_QAM256_DCT7040[] =
    {
    R0297J_EQU_0,          0x39,
    R0297J_EQU_1,          0x69,
    R0297J_EQU_3,          0x20,
    R0297J_EQU_4,          0x16,
    R0297J_EQU_5,          0x18,
    R0297J_EQU_6,          0x00,
    R0297J_INITDEM_0,      0x51,
    R0297J_INITDEM_1,      0xb8,
    R0297J_INITDEM_2,      0x00,
    R0297J_INITDEM_3,      0x00,
    R0297J_INITDEM_4,      0x00,
    R0297J_INITDEM_5,      0x88,
    R0297J_DELAGC_0,       0xff,
    R0297J_DELAGC_1,       0x3f,
    R0297J_DELAGC_2,       0xff,
    R0297J_DELAGC_3,       0x00,
    R0297J_DELAGC_4,       0x29,
    R0297J_DELAGC_5,       0x4c,
    R0297J_DELAGC_6,       0x80,
    R0297J_DELAGC_7,       0x96,
    R0297J_WBAGC_0,        0x36,
    R0297J_WBAGC_1,        0x30,
    R0297J_WBAGC_2,        0x1b,
    R0297J_WBAGC_3,        0x18,
    R0297J_WBAGC_4,        0x0c,
    R0297J_WBAGC_5,        0x73,
    R0297J_WBAGC_6,        0x80,
    R0297J_WBAGC_7,        0x12,
    R0297J_STLOOP_1,       0x06,
    R0297J_STLOOP_2,       0x00,
    R0297J_STLOOP_3,       0x06,
    R0297J_STLOOP_4,       0x30,
    R0297J_STLOOP_5,       0x4e,
    R0297J_STLOOP_6,       0x74,
    R0297J_STLOOP_7,       0x77,
    R0297J_STLOOP_8,       0x17,
    R0297J_STLOOP_9,       0x1e,
    R0297J_STLOOP_10,      0x04,
    R0297J_CRL_1,          0x4a,
    R0297J_CRL_2,          0x0b,
    R0297J_CRL_3,          0x00,
    R0297J_CRL_4,          0x17,
    R0297J_CRL_5,          0x00,
    R0297J_CRL_6,          0x00,
    R0297J_CRL_7,          0x00,
    R0297J_CRL_8,          0x00,
    R0297J_CRL_9,          0x80,
    R0297J_CRL_10,         0xd5,
    R0297J_CRL_11,         0xf3,
    R0297J_CRL_12,         0x0f,
    R0297J_CRL_13,         0x00,
    R0297J_CRL_14,         0x00,
    R0297J_PMFAGC_0,       0xff,
    R0297J_PMFAGC_1,       0x04,
    R0297J_PMFAGC_2,       0x10,
    R0297J_PMFAGC_3,       0x00,
    R0297J_PMFAGC_4,       0x00,
    R0297J_PMFAGC_5,       0x00,
    R0297J_INTER_0,        0xff,
    R0297J_INTER_1,        0xff,
    R0297J_SIG_FAD_0,      0x17,
    R0297J_SIG_FAD_2,      0x1e,
    R0297J_SIG_FAD_3,      0x37,
    R0297J_NEW_CRL_0,      0x04,
    R0297J_NEW_CRL_1,      0x0a,
    R0297J_NEW_CRL_2,      0x04,
    R0297J_NEW_CRL_3,      0x07,
    R0297J_NEW_CRL_4,      0x02,
    R0297J_NEW_CRL_5,      0x06,
    R0297J_NEW_CRL_6,      0x09,
    R0297J_FREQ_0,         0xc1,
    R0297J_FREQ_1,         0x7b,
    R0297J_FREQ_2,         0x05,
    R0297J_FREQ_3,         0x06,
    R0297J_FREQ_4,         0x08,
    R0297J_FREQ_5,         0xea,
    R0297J_FREQ_6,         0x81,
    R0297J_FREQ_7,         0x98,
    R0297J_FREQ_11,        0x22,
    R0297J_FREQ_12,        0xb4,
    R0297J_FREQ_13,        0x00,
    R0297J_FREQ_14,        0x00,
    R0297J_FREQ_15,        0xb0,
    R0297J_FREQ_16,        0xc8,
    R0297J_FREQ_17,        0x00,
    R0297J_FREQ_18,        0x00,
    R0297J_FREQ_19,        0x00,
    R0297J_FREQ_23,        0x00,
    R0297J_FREQ_24,        0x00,
    R0297J_DEINT_SYNC_0,   0x02,
    R0297J_BERT_0,         0x85,
    R0297J_BERT_1,         0x00,
    R0297J_BERT_2,         0x00,
    R0297J_DEINT_0,        0x91,
    R0297J_DEINT_1,        0x0b,
    R0297J_RS_DESC_6,      0x00,
    R0297J_RS_DESC_7,      0x00,
    R0297J_RS_DESC_8,      0x01,
    R0297J_CTRL_0,         0xc0,
    R0297J_CTRL_1,         0x00,
    R0297J_CTRL_2,         0x20,
    R0297J_CTRL_3,         0x80,
    R0297J_CTRL_4,         0x81,
    R0297J_CTRL_5,         0x00,
    R0297J_CTRL_6,         0x00,
    R0297J_CTRL_7,         0x00,
    R0297J_CTRL_9,         0x00,
    R0297J_CTRL_10,        0x1f,
    R0297J_CTRL_11,        0x00,
    R0297J_CTRL_12,        0x00,
    0x00
    };

const U8 FECB_QAM64_DCT7040[] =
    {
    R0297J_EQU_0,          0x48,
    R0297J_EQU_1,          0xa7,
    R0297J_EQU_3,          0x00,
    R0297J_EQU_4,          0x06,
    R0297J_EQU_5,          0x7a,
    R0297J_EQU_6,          0x00,
    R0297J_INITDEM_0,      0x51,
    R0297J_INITDEM_1,      0xb8,
    R0297J_INITDEM_2,      0x00,
    R0297J_INITDEM_3,      0x00,
    R0297J_INITDEM_4,      0x40,
    R0297J_INITDEM_5,      0x88,
    R0297J_DELAGC_0,       0xff,
    R0297J_DELAGC_1,       0x3f,
    R0297J_DELAGC_2,       0xff,
    R0297J_DELAGC_3,       0x00,
    R0297J_DELAGC_4,       0x29,
    R0297J_DELAGC_5,       0x4c,
    R0297J_DELAGC_6,       0x80,
    R0297J_DELAGC_7,       0x46,
    R0297J_WBAGC_0,        0x71,
    R0297J_WBAGC_1,        0x35,
    R0297J_WBAGC_2,        0x1b,
    R0297J_WBAGC_3,        0x18,
    R0297J_WBAGC_4,        0x0b,
    R0297J_WBAGC_5,        0xaf,
    R0297J_WBAGC_6,        0x88,
    R0297J_WBAGC_7,        0x13,
    R0297J_STLOOP_1,       0x08,
    R0297J_STLOOP_2,       0x00,
    R0297J_STLOOP_3,       0x08,
    R0297J_STLOOP_4,       0x30,
    R0297J_STLOOP_5,       0x24,
    R0297J_STLOOP_6,       0x6c,
    R0297J_STLOOP_7,       0x71,
    R0297J_STLOOP_8,       0x1b,
    R0297J_STLOOP_9,       0x5e,
    R0297J_STLOOP_10,      0x84,
    R0297J_CRL_1,          0x48,
    R0297J_CRL_2,          0x8b,
    R0297J_CRL_3,          0x00,
    R0297J_CRL_4,          0x2b,
    R0297J_CRL_5,          0x00,
    R0297J_CRL_6,          0x00,
    R0297J_CRL_7,          0x00,
    R0297J_CRL_8,          0x26,
    R0297J_CRL_9,          0x60,
    R0297J_CRL_10,         0xe8,
    R0297J_CRL_11,         0x29,
    R0297J_CRL_12,         0x06,
    R0297J_CRL_13,         0x00,
    R0297J_CRL_14,         0x00,
    R0297J_PMFAGC_0,       0xff,
    R0297J_PMFAGC_1,       0x0f,
    R0297J_PMFAGC_2,       0x10,
    R0297J_PMFAGC_3,       0x00,
    R0297J_PMFAGC_4,       0x00,
    R0297J_PMFAGC_5,       0x00,
    R0297J_INTER_0,        0xff,
    R0297J_INTER_1,        0xff,
    R0297J_SIG_FAD_0,      0x17,
    R0297J_SIG_FAD_2,      0x1e,
    R0297J_SIG_FAD_3,      0x37,
    R0297J_NEW_CRL_0,      0x04,
    R0297J_NEW_CRL_1,      0x08,
    R0297J_NEW_CRL_2,      0x02,
    R0297J_NEW_CRL_3,      0x05,
    R0297J_NEW_CRL_4,      0x02,
    R0297J_NEW_CRL_5,      0x05,
    R0297J_NEW_CRL_6,      0x09,
    R0297J_FREQ_0,         0x82,
    R0297J_FREQ_1,         0x6b,
    R0297J_FREQ_2,         0x05,
    R0297J_FREQ_3,         0x06,
    R0297J_FREQ_4,         0x0a,
    R0297J_FREQ_5,         0xf0,
    R0297J_FREQ_6,         0x00,
    R0297J_FREQ_7,         0x50,
    R0297J_FREQ_11,        0x22,
    R0297J_FREQ_12,        0xb4,
    R0297J_FREQ_13,        0x00,
    R0297J_FREQ_14,        0x00,
    R0297J_FREQ_15,        0xb0,
    R0297J_FREQ_16,        0xc8,
    R0297J_FREQ_17,        0x00,
    R0297J_FREQ_18,        0x00,
    R0297J_FREQ_19,        0x00,
    R0297J_FREQ_23,        0x00,
    R0297J_FREQ_24,        0x00,
    R0297J_DEINT_SYNC_0,   0x02,
    R0297J_BERT_0,         0x85,
    R0297J_BERT_1,         0x00,
    R0297J_BERT_2,         0x00,
    R0297J_DEINT_0,        0x91,
    R0297J_DEINT_1,        0x0b,
    R0297J_RS_DESC_6,      0x00,
    R0297J_RS_DESC_7,      0x00,
    R0297J_RS_DESC_8,      0x01,
    R0297J_CTRL_0,         0x00,
    R0297J_CTRL_1,         0x11,
    R0297J_CTRL_2,         0x20,
    R0297J_CTRL_3,         0xa2,
    R0297J_CTRL_4,         0x81,
    R0297J_CTRL_5,         0x00,
    R0297J_CTRL_6,         0x00,
    R0297J_CTRL_7,         0x00,
    R0297J_CTRL_9,         0x00,
    R0297J_CTRL_10,        0x1b, /* 58MHz */
    R0297J_CTRL_11,        0x00,
    R0297J_CTRL_12,        0x00,
    R0297J_MPEG_CTRL,      0xdb,
    R0297J_MPEG_SYNC_ACQ,  0x0f,
    R0297J_MPEG_SYNC_LOSS, 0x3f,
    R0297J_VIT_SYNC_ACQ,   0x0d,
    R0297J_VIT_SYNC_LOSS,  0xdc,
    R0297J_VIT_SYNC_GO,    0x08,
    R0297J_VIT_SYNC_STOP,  0x08,
    R0297J_FS_SYNC,        0xa8,
    R0297J_IN_DEPTH,       0x81,
    R0297J_RS_CONTROL,     0xfc,
    R0297J_TSMF_SEL,       0x00,
    R0297J_TSMF_CTRL,      0x00,
    R0297J_AUTOMATIC,      0x43,
    0x00
    };

const U8 FECB_QAM256_DCT7040[] =
    {
    R0297J_EQU_0,          0x39,
    R0297J_EQU_1,          0x69,
    R0297J_EQU_3,          0x20,
    R0297J_EQU_4,          0x06,
    R0297J_EQU_5,          0x18,
    R0297J_EQU_6,          0x00,
    R0297J_INITDEM_0,      0x51,
    R0297J_INITDEM_1,      0xb8,
    R0297J_INITDEM_2,      0x00,
    R0297J_INITDEM_3,      0x00,
    R0297J_INITDEM_4,      0x00,
    R0297J_INITDEM_5,      0x88,
    R0297J_DELAGC_0,       0xff,
    R0297J_DELAGC_1,       0x3f,
    R0297J_DELAGC_2,       0xff,
    R0297J_DELAGC_3,       0x00,
    R0297J_DELAGC_4,       0x29,
    R0297J_DELAGC_5,       0x4c,
    R0297J_DELAGC_6,       0x80,
    R0297J_DELAGC_7,       0x76,
    R0297J_WBAGC_0,        0x42,
    R0297J_WBAGC_1,        0x34,
    R0297J_WBAGC_2,        0x1b,
    R0297J_WBAGC_3,        0x18,
    R0297J_WBAGC_4,        0x6f,
    R0297J_WBAGC_5,        0x0a,
    R0297J_WBAGC_6,        0x88,
    R0297J_WBAGC_7,        0x13,
    R0297J_STLOOP_1,       0x08,
    R0297J_STLOOP_2,       0x00,
    R0297J_STLOOP_3,       0x08,
    R0297J_STLOOP_4,       0x30,
    R0297J_STLOOP_5,       0x40,
    R0297J_STLOOP_6,       0x2b,
    R0297J_STLOOP_7,       0x72,
    R0297J_STLOOP_8,       0x1b,
    R0297J_STLOOP_9,       0x5e,
    R0297J_STLOOP_10,      0x44,
    R0297J_CRL_1,          0x4a,
    R0297J_CRL_2,          0x8b,
    R0297J_CRL_3,          0x00,
    R0297J_CRL_4,          0xb9,
    R0297J_CRL_5,          0x00,
    R0297J_CRL_6,          0x00,
    R0297J_CRL_7,          0x00,
    R0297J_CRL_8,          0x26,
    R0297J_CRL_9,          0x40,
    R0297J_CRL_10,         0x46,
    R0297J_CRL_11,         0xfe,
    R0297J_CRL_12,         0x0f,
    R0297J_CRL_13,         0x00,
    R0297J_CRL_14,         0x00,
    R0297J_PMFAGC_0,       0xff,
    R0297J_PMFAGC_1,       0x04,
    R0297J_PMFAGC_2,       0x10,
    R0297J_PMFAGC_3,       0x00,
    R0297J_PMFAGC_4,       0x00,
    R0297J_PMFAGC_5,       0x00,
    R0297J_INTER_0,        0xff,
    R0297J_INTER_1,        0xff,
    R0297J_SIG_FAD_0,      0x17,
    R0297J_SIG_FAD_2,      0x1e,
    R0297J_SIG_FAD_3,      0x37,
    R0297J_NEW_CRL_0,      0x04,
    R0297J_NEW_CRL_1,      0x0a,
    R0297J_NEW_CRL_2,      0x04,
    R0297J_NEW_CRL_3,      0x07,
    R0297J_NEW_CRL_4,      0x01,
    R0297J_NEW_CRL_5,      0x06,
    R0297J_NEW_CRL_6,      0x09,
    R0297J_FREQ_0,         0xc1,
    R0297J_FREQ_1,         0x7b,
    R0297J_FREQ_2,         0x05,
    R0297J_FREQ_3,         0x06,
    R0297J_FREQ_4,         0x08,
    R0297J_FREQ_5,         0xea,
    R0297J_FREQ_6,         0x81,
    R0297J_FREQ_7,         0x98,
    R0297J_FREQ_11,        0x22,
    R0297J_FREQ_12,        0xb4,
    R0297J_FREQ_13,        0x00,
    R0297J_FREQ_14,        0x00,
    R0297J_FREQ_15,        0xb0,
    R0297J_FREQ_16,        0xc8,
    R0297J_FREQ_17,        0x00,
    R0297J_FREQ_18,        0x00,
    R0297J_FREQ_19,        0x00,
    R0297J_FREQ_23,        0x00,
    R0297J_FREQ_24,        0x00,
    R0297J_DEINT_SYNC_0,   0x02,
    R0297J_BERT_0,         0x86,
    R0297J_BERT_1,         0xff,
    R0297J_BERT_2,         0xff,
    R0297J_DEINT_0,        0x91,
    R0297J_DEINT_1,        0x0b,
    R0297J_RS_DESC_6,      0x00,
    R0297J_RS_DESC_7,      0x00,
    R0297J_RS_DESC_8,      0x00,
    R0297J_CTRL_0,         0x00,
    R0297J_CTRL_1,         0x11,
    R0297J_CTRL_2,         0x20,
    R0297J_CTRL_3,         0x80,
    R0297J_CTRL_4,         0x81,
    R0297J_CTRL_5,         0x00,
    R0297J_CTRL_6,         0x00,
    R0297J_CTRL_7,         0x00,
    R0297J_CTRL_9,         0x00,
    R0297J_CTRL_10,        0x1b, /* 58MHz */
    R0297J_CTRL_11,        0x00,
    R0297J_CTRL_12,        0x00,
    R0297J_MPEG_CTRL,      0x1b,
    R0297J_MPEG_SYNC_ACQ,  0x0f,
    R0297J_MPEG_SYNC_LOSS, 0x3f,
    R0297J_VIT_SYNC_ACQ,   0x0d,
    R0297J_VIT_SYNC_LOSS,  0xdc,
    R0297J_VIT_SYNC_GO,    0x08,
    R0297J_VIT_SYNC_STOP,  0x08,
    R0297J_FS_SYNC,        0xa8,
    R0297J_IN_DEPTH,       0x81,
    R0297J_RS_CONTROL,     0xfb,
    R0297J_TSMF_SEL,       0x00,
    R0297J_TSMF_CTRL,      0x00,
    R0297J_AUTOMATIC,      0x43,
    0x00
    };

/* Initialization of DCT7050 */
/* ========================= */

const U8 FECA_QAM16_DCT7050[] =
    {
    0x00
    };

const U8 FECA_QAM32_DCT7050[] =
    {
    0x00
    };

const U8 FECA_QAM64_DCT7050[] =
    {
    R0297J_EQU_0,          0x48,
    R0297J_EQU_1,          0xa7,
    R0297J_EQU_3,          0x00,
    R0297J_EQU_4,          0x16,
    R0297J_EQU_5,          0x7a,
    R0297J_EQU_6,          0x00,
    R0297J_INITDEM_0,      0xed,
    R0297J_INITDEM_1,      0xde,
    R0297J_INITDEM_2,      0x00,
    R0297J_INITDEM_3,      0x00,
    R0297J_INITDEM_4,      0x40,
    R0297J_INITDEM_5,      0x84,
    R0297J_DELAGC_0,       0xff,
    R0297J_DELAGC_1,       0x00,
    R0297J_DELAGC_2,       0xff,
    R0297J_DELAGC_3,       0x00,
    R0297J_DELAGC_4,       0x29,
    R0297J_DELAGC_5,       0x4c,
    R0297J_DELAGC_6,       0x80,
    R0297J_DELAGC_7,       0x46,
    R0297J_WBAGC_0,        0x37,
    R0297J_WBAGC_1,        0x34,
    R0297J_WBAGC_2,        0x1a,
    R0297J_WBAGC_3,        0x18,
    R0297J_WBAGC_4,        0x99,
    R0297J_WBAGC_5,        0xb0,
    R0297J_WBAGC_6,        0x88,
    R0297J_WBAGC_7,        0x13,
    R0297J_STLOOP_1,       0x08,
    R0297J_STLOOP_2,       0x00,
    R0297J_STLOOP_3,       0x08,
    R0297J_STLOOP_4,       0x30,
    R0297J_STLOOP_5,       0xc0,
    R0297J_STLOOP_6,       0xe9,
    R0297J_STLOOP_7,       0x04,
    R0297J_STLOOP_8,       0x1b,
    R0297J_STLOOP_9,       0x5e,
    R0297J_STLOOP_10,      0x04,
    R0297J_CRL_1,          0xc8,
    R0297J_CRL_2,          0x8b,
    R0297J_CRL_3,          0x00,
    R0297J_CRL_4,          0x00,
    R0297J_CRL_5,          0x44,
    R0297J_CRL_6,          0xf6,
    R0297J_CRL_7,          0x74,
    R0297J_CRL_8,          0x26,
    R0297J_CRL_9,          0x00,
    R0297J_CRL_10,         0x00,
    R0297J_CRL_11,         0x00,
    R0297J_CRL_12,         0x00,
    R0297J_CRL_13,         0x00,
    R0297J_CRL_14,         0x00,
    R0297J_PMFAGC_0,       0xff,
    R0297J_PMFAGC_1,       0x0f,
    R0297J_PMFAGC_2,       0x10,
    R0297J_PMFAGC_3,       0x00,
    R0297J_PMFAGC_4,       0x00,
    R0297J_PMFAGC_5,       0x00,
    R0297J_INTER_0,        0xff,
    R0297J_INTER_1,        0xff,
    R0297J_SIG_FAD_0,      0x16,
    R0297J_SIG_FAD_2,      0x1e,
    R0297J_SIG_FAD_3,      0x37,
    R0297J_NEW_CRL_0,      0x04,
    R0297J_NEW_CRL_1,      0x08,
    R0297J_NEW_CRL_2,      0x02,
    R0297J_NEW_CRL_3,      0x05,
    R0297J_NEW_CRL_4,      0x02,
    R0297J_NEW_CRL_5,      0x05,
    R0297J_NEW_CRL_6,      0x09,
    R0297J_FREQ_0,         0x82,
    R0297J_FREQ_1,         0x6b,
    R0297J_FREQ_2,         0x05,
    R0297J_FREQ_3,         0x06,
    R0297J_FREQ_4,         0x0a,
    R0297J_FREQ_5,         0xf0,
    R0297J_FREQ_6,         0x00,
    R0297J_FREQ_7,         0x50,
    R0297J_FREQ_11,        0x22,
    R0297J_FREQ_12,        0xb4,
    R0297J_FREQ_13,        0x00,
    R0297J_FREQ_14,        0x00,
    R0297J_FREQ_15,        0xb2,
    R0297J_FREQ_16,        0xc8,
    R0297J_FREQ_17,        0x00,
    R0297J_FREQ_18,        0x00,
    R0297J_FREQ_19,        0x00,
    R0297J_FREQ_23,        0x00,
    R0297J_FREQ_24,        0x00,
    R0297J_DEINT_SYNC_0,   0x02,
    R0297J_BERT_0,         0x85,
    R0297J_BERT_1,         0x9e,
    R0297J_BERT_2,         0x1a,
    R0297J_DEINT_0,        0x91,
    R0297J_DEINT_1,        0x0b,
    R0297J_RS_DESC_6,      0x00,
    R0297J_RS_DESC_7,      0x00,
    R0297J_RS_DESC_8,      0x01,
    R0297J_CTRL_0,         0xc0,
    R0297J_CTRL_1,         0x00,
    R0297J_CTRL_2,         0x20,
    R0297J_CTRL_3,         0x80,
    R0297J_CTRL_4,         0xa0,
    R0297J_CTRL_5,         0x00,
    R0297J_CTRL_6,         0x00,
    R0297J_CTRL_7,         0x00,
    R0297J_CTRL_9,         0x00,
    R0297J_CTRL_10,        0x1b, /* 58MHz */
    R0297J_CTRL_11,        0x00,
    R0297J_CTRL_12,        0x00,
    0x00
    };

const U8 FECA_QAM128_DCT7050[] =
    {
    0x00
    };

const U8 FECA_QAM256_DCT7050[] =
    {
    R0297J_EQU_0,          0x39,
    R0297J_EQU_1,          0x69,
    R0297J_EQU_3,          0x20,
    R0297J_EQU_4,          0x16,
    R0297J_EQU_5,          0x18,
    R0297J_EQU_6,          0x00,
    R0297J_INITDEM_0,      0x51,
    R0297J_INITDEM_1,      0xb8,
    R0297J_INITDEM_2,      0x00,
    R0297J_INITDEM_3,      0x00,
    R0297J_INITDEM_4,      0x00,
    R0297J_INITDEM_5,      0x84,
    R0297J_DELAGC_0,       0xff,
    R0297J_DELAGC_1,       0x00,
    R0297J_DELAGC_2,       0xff,
    R0297J_DELAGC_3,       0x00,
    R0297J_DELAGC_4,       0x29,
    R0297J_DELAGC_5,       0x4c,
    R0297J_DELAGC_6,       0x80,
    R0297J_DELAGC_7,       0xb6,
    R0297J_WBAGC_0,        0xff,
    R0297J_WBAGC_1,        0x33,
    R0297J_WBAGC_2,        0x1a,
    R0297J_WBAGC_3,        0x18,
    R0297J_WBAGC_4,        0x0c,
    R0297J_WBAGC_5,        0x73,
    R0297J_WBAGC_6,        0x80,
    R0297J_WBAGC_7,        0x12,
    R0297J_STLOOP_1,       0x08,
    R0297J_STLOOP_2,       0x00,
    R0297J_STLOOP_3,       0x08,
    R0297J_STLOOP_4,       0x30,
    R0297J_STLOOP_5,       0xda,
    R0297J_STLOOP_6,       0x7f,
    R0297J_STLOOP_7,       0x97,
    R0297J_STLOOP_8,       0x23,
    R0297J_STLOOP_9,       0x1e,
    R0297J_STLOOP_10,      0x04,
    R0297J_CRL_1,          0xca,
    R0297J_CRL_2,          0x8b,
    R0297J_CRL_3,          0x00,
    R0297J_CRL_4,          0x9a,
    R0297J_CRL_5,          0x00,
    R0297J_CRL_6,          0x00,
    R0297J_CRL_7,          0x00,
    R0297J_CRL_8,          0x26,
    R0297J_CRL_9,          0x00,
    R0297J_CRL_10,         0x00,
    R0297J_CRL_11,         0x00,
    R0297J_CRL_12,         0x00,
    R0297J_CRL_13,         0x00,
    R0297J_CRL_14,         0x00,
    R0297J_PMFAGC_0,       0xff,
    R0297J_PMFAGC_1,       0x04,
    R0297J_PMFAGC_2,       0x10,
    R0297J_PMFAGC_3,       0x00,
    R0297J_PMFAGC_4,       0x00,
    R0297J_PMFAGC_5,       0x00,
    R0297J_INTER_0,        0xff,
    R0297J_INTER_1,        0xff,
    R0297J_SIG_FAD_0,      0x16,
    R0297J_SIG_FAD_2,      0x1e,
    R0297J_SIG_FAD_3,      0x37,
    R0297J_NEW_CRL_0,      0x04,
    R0297J_NEW_CRL_1,      0x0a,
    R0297J_NEW_CRL_2,      0x04,
    R0297J_NEW_CRL_3,      0x07,
    R0297J_NEW_CRL_4,      0x02,
    R0297J_NEW_CRL_5,      0x06,
    R0297J_NEW_CRL_6,      0x09,
    R0297J_FREQ_0,         0xc1,
    R0297J_FREQ_1,         0x7b,
    R0297J_FREQ_2,         0x05,
    R0297J_FREQ_3,         0x06,
    R0297J_FREQ_4,         0x08,
    R0297J_FREQ_5,         0xea,
    R0297J_FREQ_6,         0x81,
    R0297J_FREQ_7,         0x98,
    R0297J_FREQ_11,        0x22,
    R0297J_FREQ_12,        0xb4,
    R0297J_FREQ_13,        0x00,
    R0297J_FREQ_14,        0x00,
    R0297J_FREQ_15,        0xb2,
    R0297J_FREQ_16,        0xc8,
    R0297J_FREQ_17,        0x00,
    R0297J_FREQ_18,        0x00,
    R0297J_FREQ_19,        0x00,
    R0297J_FREQ_23,        0x00,
    R0297J_FREQ_24,        0x00,
    R0297J_DEINT_SYNC_0,   0x01,
    R0297J_BERT_0,         0x85,
    R0297J_BERT_1,         0xff,
    R0297J_BERT_2,         0xff,
    R0297J_DEINT_0,        0x91,
    R0297J_DEINT_1,        0x0b,
    R0297J_RS_DESC_6,      0x00,
    R0297J_RS_DESC_7,      0x00,
    R0297J_RS_DESC_8,      0x01,
    R0297J_CTRL_0,         0xc0,
    R0297J_CTRL_1,         0x00,
    R0297J_CTRL_2,         0x20,
    R0297J_CTRL_3,         0x80,
    R0297J_CTRL_4,         0x81,
    R0297J_CTRL_5,         0x00,
    R0297J_CTRL_6,         0x00,
    R0297J_CTRL_7,         0x00,
    R0297J_CTRL_9,         0x00,
    R0297J_CTRL_10,        0x1f,
    R0297J_CTRL_11,        0x00,
    R0297J_CTRL_12,        0x00,
    0x00
    };

const U8 FECB_QAM64_DCT7050[] =
    {
    R0297J_EQU_0,          0x48,
    R0297J_EQU_1,          0xa7,
    R0297J_EQU_3,          0x00,
    R0297J_EQU_4,          0x06,
    R0297J_EQU_5,          0x7a,
    R0297J_EQU_6,          0x00,
    R0297J_INITDEM_0,      0x00,
    R0297J_INITDEM_1,      0xe0,
    R0297J_INITDEM_2,      0x00,
    R0297J_INITDEM_3,      0x00,
    R0297J_INITDEM_4,      0x40,
    R0297J_INITDEM_5,      0x84,
    R0297J_DELAGC_0,       0xff,
    R0297J_DELAGC_1,       0x00,
    R0297J_DELAGC_2,       0xff,
    R0297J_DELAGC_3,       0x00,
    R0297J_DELAGC_4,       0x29,
    R0297J_DELAGC_5,       0x4c,
    R0297J_DELAGC_6,       0x80,
    R0297J_DELAGC_7,       0xb6,
    R0297J_WBAGC_0,        0xf1,
    R0297J_WBAGC_1,        0x34,
    R0297J_WBAGC_2,        0x1a,
    R0297J_WBAGC_3,        0x18,
    R0297J_WBAGC_4,        0x99,
    R0297J_WBAGC_5,        0xb0,
    R0297J_WBAGC_6,        0x88,
    R0297J_WBAGC_7,        0x13,
    R0297J_STLOOP_1,       0x08,
    R0297J_STLOOP_2,       0x00,
    R0297J_STLOOP_3,       0x08,
    R0297J_STLOOP_4,       0x30,
    R0297J_STLOOP_5,       0x88,
    R0297J_STLOOP_6,       0xc4,
    R0297J_STLOOP_7,       0x29,
    R0297J_STLOOP_8,       0x17,
    R0297J_STLOOP_9,       0x5e,
    R0297J_STLOOP_10,      0x04,
    R0297J_CRL_1,          0x48,
    R0297J_CRL_2,          0x8b,
    R0297J_CRL_3,          0x00,
    R0297J_CRL_4,          0x5b,
    R0297J_CRL_5,          0x44,
    R0297J_CRL_6,          0xf6,
    R0297J_CRL_7,          0x74,
    R0297J_CRL_8,          0x26,
    R0297J_CRL_9,          0x80,
    R0297J_CRL_10,         0xb8,
    R0297J_CRL_11,         0x02,
    R0297J_CRL_12,         0x00,
    R0297J_CRL_13,         0x00,
    R0297J_CRL_14,         0x00,
    R0297J_PMFAGC_0,       0xff,
    R0297J_PMFAGC_1,       0x0f,
    R0297J_PMFAGC_2,       0x10,
    R0297J_PMFAGC_3,       0x00,
    R0297J_PMFAGC_4,       0x00,
    R0297J_PMFAGC_5,       0x00,
    R0297J_INTER_0,        0xff,
    R0297J_INTER_1,        0xff,
    R0297J_SIG_FAD_0,      0x17,
    R0297J_SIG_FAD_2,      0x1e,
    R0297J_SIG_FAD_3,      0x37,
    R0297J_NEW_CRL_0,      0x04,
    R0297J_NEW_CRL_1,      0x08,
    R0297J_NEW_CRL_2,      0x02,
    R0297J_NEW_CRL_3,      0x05,
    R0297J_NEW_CRL_4,      0x02,
    R0297J_NEW_CRL_5,      0x05,
    R0297J_NEW_CRL_6,      0x09,
    R0297J_FREQ_0,         0x82,
    R0297J_FREQ_1,         0x6b,
    R0297J_FREQ_2,         0x05,
    R0297J_FREQ_3,         0x06,
    R0297J_FREQ_4,         0x0a,
    R0297J_FREQ_5,         0xf0,
    R0297J_FREQ_6,         0x00,
    R0297J_FREQ_7,         0x50,
    R0297J_FREQ_11,        0x22,
    R0297J_FREQ_12,        0xb4,
    R0297J_FREQ_13,        0x00,
    R0297J_FREQ_14,        0x00,
    R0297J_FREQ_15,        0xb0,
    R0297J_FREQ_16,        0xc8,
    R0297J_FREQ_17,        0x00,
    R0297J_FREQ_18,        0x00,
    R0297J_FREQ_19,        0x00,
    R0297J_FREQ_23,        0x00,
    R0297J_FREQ_24,        0x00,
    R0297J_DEINT_SYNC_0,   0x02,
    R0297J_BERT_0,         0x85,
    R0297J_BERT_1,         0x9e,
    R0297J_BERT_2,         0x1a,
    R0297J_DEINT_0,        0x91,
    R0297J_DEINT_1,        0x0b,
    R0297J_RS_DESC_6,      0x00,
    R0297J_RS_DESC_7,      0x00,
    R0297J_RS_DESC_8,      0x01,
    R0297J_CTRL_0,         0x00,
    R0297J_CTRL_1,         0x11,
    R0297J_CTRL_2,         0x20,
    R0297J_CTRL_3,         0x80,
    R0297J_CTRL_4,         0xa0,
    R0297J_CTRL_5,         0x00,
    R0297J_CTRL_6,         0x00,
    R0297J_CTRL_7,         0x00,
    R0297J_CTRL_9,         0x00,
    R0297J_CTRL_10,        0x1b, /* 58MHz */
    R0297J_CTRL_11,        0x00,
    R0297J_CTRL_12,        0x00,
    R0297J_MPEG_CTRL,      0xdb,
    R0297J_MPEG_SYNC_ACQ,  0x0f,
    R0297J_MPEG_SYNC_LOSS, 0x3f,
    R0297J_VIT_SYNC_ACQ,   0x0d,
    R0297J_VIT_SYNC_LOSS,  0xdc,
    R0297J_VIT_SYNC_GO,    0x08,
    R0297J_VIT_SYNC_STOP,  0x08,
    R0297J_FS_SYNC,        0xa8,
    R0297J_IN_DEPTH,       0x81,
    R0297J_RS_CONTROL,     0xfc,
    R0297J_TSMF_SEL,       0x00,
    R0297J_TSMF_CTRL,      0x00,
    R0297J_AUTOMATIC,      0x43,
    0x00
    };

const U8 FECB_QAM256_DCT7050[] =
    {
    R0297J_EQU_0,          0x39,
    R0297J_EQU_1,          0x69,
    R0297J_EQU_3,          0x20,
    R0297J_EQU_4,          0x06,
    R0297J_EQU_5,          0x18,
    R0297J_EQU_6,          0x00,
    R0297J_INITDEM_0,      0x00,
    R0297J_INITDEM_1,      0xe0,
    R0297J_INITDEM_2,      0x00,
    R0297J_INITDEM_3,      0x00,
    R0297J_INITDEM_4,      0x00,
    R0297J_INITDEM_5,      0x84,
    R0297J_DELAGC_0,       0xff,
    R0297J_DELAGC_1,       0x00,
    R0297J_DELAGC_2,       0xff,
    R0297J_DELAGC_3,       0x00,
    R0297J_DELAGC_4,       0x29,
    R0297J_DELAGC_5,       0x4c,
    R0297J_DELAGC_6,       0x80,
    R0297J_DELAGC_7,       0x16,
    R0297J_WBAGC_0,        0xf2,
    R0297J_WBAGC_1,        0x34,
    R0297J_WBAGC_2,        0x1a,
    R0297J_WBAGC_3,        0x18,
    R0297J_WBAGC_4,        0x89,
    R0297J_WBAGC_5,        0xdf,
    R0297J_WBAGC_6,        0x88,
    R0297J_WBAGC_7,        0x13,
    R0297J_STLOOP_1,       0x08,
    R0297J_STLOOP_2,       0x00,
    R0297J_STLOOP_3,       0x08,
    R0297J_STLOOP_4,       0x30,
    R0297J_STLOOP_5,       0x48,
    R0297J_STLOOP_6,       0x13,
    R0297J_STLOOP_7,       0xe2,
    R0297J_STLOOP_8,       0x1a,
    R0297J_STLOOP_9,       0x5e,
    R0297J_STLOOP_10,      0x04,
    R0297J_CRL_1,          0x4a,
    R0297J_CRL_2,          0x8b,
    R0297J_CRL_3,          0x00,
    R0297J_CRL_4,          0xab,
    R0297J_CRL_5,          0x00,
    R0297J_CRL_6,          0x00,
    R0297J_CRL_7,          0x00,
    R0297J_CRL_8,          0x26,
    R0297J_CRL_9,          0x00,
    R0297J_CRL_10,         0xd5,
    R0297J_CRL_11,         0x02,
    R0297J_CRL_12,         0x00,
    R0297J_CRL_13,         0x00,
    R0297J_CRL_14,         0x00,
    R0297J_PMFAGC_0,       0xff,
    R0297J_PMFAGC_1,       0x04,
    R0297J_PMFAGC_2,       0x10,
    R0297J_PMFAGC_3,       0x00,
    R0297J_PMFAGC_4,       0x00,
    R0297J_PMFAGC_5,       0x00,
    R0297J_INTER_0,        0xff,
    R0297J_INTER_1,        0xff,
    R0297J_SIG_FAD_0,      0x17,
    R0297J_SIG_FAD_2,      0x1e,
    R0297J_SIG_FAD_3,      0x37,
    R0297J_NEW_CRL_0,      0x04,
    R0297J_NEW_CRL_1,      0x0a,
    R0297J_NEW_CRL_2,      0x04,
    R0297J_NEW_CRL_3,      0x07,
    R0297J_NEW_CRL_4,      0x02,
    R0297J_NEW_CRL_5,      0x06,
    R0297J_NEW_CRL_6,      0x09,
    R0297J_FREQ_0,         0xc1,
    R0297J_FREQ_1,         0x7b,
    R0297J_FREQ_2,         0x05,
    R0297J_FREQ_3,         0x06,
    R0297J_FREQ_4,         0x08,
    R0297J_FREQ_5,         0xea,
    R0297J_FREQ_6,         0x81,
    R0297J_FREQ_7,         0x98,
    R0297J_FREQ_11,        0x22,
    R0297J_FREQ_12,        0xb4,
    R0297J_FREQ_13,        0x00,
    R0297J_FREQ_14,        0x00,
    R0297J_FREQ_15,        0xb0,
    R0297J_FREQ_16,        0xc8,
    R0297J_FREQ_17,        0x00,
    R0297J_FREQ_18,        0x00,
    R0297J_FREQ_19,        0x00,
    R0297J_FREQ_23,        0x00,
    R0297J_FREQ_24,        0x00,
    R0297J_DEINT_SYNC_0,   0x01,
    R0297J_BERT_0,         0x85,
    R0297J_BERT_1,         0xff,
    R0297J_BERT_2,         0xff,
    R0297J_DEINT_0,        0x91,
    R0297J_DEINT_1,        0x0b,
    R0297J_RS_DESC_6,      0x00,
    R0297J_RS_DESC_7,      0x00,
    R0297J_RS_DESC_8,      0x01,
    R0297J_CTRL_0,         0x00,
    R0297J_CTRL_1,         0x11,
    R0297J_CTRL_2,         0x20,
    R0297J_CTRL_3,         0x80,
    R0297J_CTRL_4,         0x81,
    R0297J_CTRL_5,         0x00,
    R0297J_CTRL_6,         0x00,
    R0297J_CTRL_7,         0x00,
    R0297J_CTRL_9,         0x00,
/*  R0297J_CTRL_10,        0x1f,*/
    R0297J_CTRL_10,        0x1b, /* 58MHz */
    R0297J_CTRL_11,        0x00,
    R0297J_CTRL_12,        0x00,
    R0297J_MPEG_CTRL,      0x1b,
    R0297J_MPEG_SYNC_ACQ,  0x0f,
    R0297J_MPEG_SYNC_LOSS, 0x3f,
    R0297J_VIT_SYNC_ACQ,   0x0d,
    R0297J_VIT_SYNC_LOSS,  0xdc,
    R0297J_VIT_SYNC_GO,    0x08,
    R0297J_VIT_SYNC_STOP,  0x08,
    R0297J_FS_SYNC,        0xa8,
    R0297J_IN_DEPTH,       0x81,
    R0297J_RS_CONTROL,     0xfb,
    R0297J_TSMF_SEL,       0x00,
    R0297J_TSMF_CTRL,      0x00,
    R0297J_AUTOMATIC,      0x43,
    0x00
    };

/* Initialization of MACOETA50DR */
/* ============================= */

const U8 FECA_QAM16_MACOETA50DR[] =
    {
    0x00
    };

const U8 FECA_QAM32_MACOETA50DR[] =
    {
    0x00
    };

const U8 FECA_QAM64_MACOETA50DR[] =
    {
    R0297J_EQU_0,          0x48,
    R0297J_EQU_1,          0xa7,
    R0297J_EQU_3,          0x00,
    R0297J_EQU_4,          0x06,
    R0297J_EQU_5,          0x7a,
    R0297J_EQU_6,          0x00,
    R0297J_INITDEM_0,      0x51,
    R0297J_INITDEM_1,      0xb8,
    R0297J_INITDEM_2,      0x00,
    R0297J_INITDEM_3,      0x00,
    R0297J_INITDEM_4,      0x40,
    R0297J_INITDEM_5,      0x84,
    R0297J_DELAGC_0,       0xff,
    R0297J_DELAGC_1,       0x4c,
    R0297J_DELAGC_2,       0xff,
    R0297J_DELAGC_3,       0x4c,
    R0297J_DELAGC_4,       0x29,
    R0297J_DELAGC_5,       0x66,
    R0297J_DELAGC_6,       0x80,
    R0297J_DELAGC_7,       0x06,
    R0297J_WBAGC_0,        0xff,
    R0297J_WBAGC_1,        0x33,
    R0297J_WBAGC_2,        0x1a,
    R0297J_WBAGC_3,        0x18,
    R0297J_WBAGC_4,        0xd7,
    R0297J_WBAGC_5,        0x10,
    R0297J_WBAGC_6,        0x80,
    R0297J_WBAGC_7,        0x12,
    R0297J_STLOOP_1,       0x08,
    R0297J_STLOOP_2,       0x00,
    R0297J_STLOOP_3,       0x08,
    R0297J_STLOOP_4,       0x30,
    R0297J_STLOOP_5,       0x4c,
    R0297J_STLOOP_6,       0x7f,
    R0297J_STLOOP_7,       0xab,
    R0297J_STLOOP_8,       0x23,
    R0297J_STLOOP_9,       0x5e,
    R0297J_STLOOP_10,      0x04,
    R0297J_CRL_1,          0x48,
    R0297J_CRL_2,          0x8b,
    R0297J_CRL_3,          0x02,
    R0297J_CRL_4,          0x34,
    R0297J_CRL_5,          0x44,
    R0297J_CRL_6,          0xf6,
    R0297J_CRL_7,          0x74,
    R0297J_CRL_8,          0x26,
    R0297J_CRL_9,          0x00,
    R0297J_CRL_10,         0xc0,
    R0297J_CRL_11,         0x05,
    R0297J_CRL_12,         0x00,
    R0297J_CRL_13,         0x00,
    R0297J_CRL_14,         0x00,
    R0297J_PMFAGC_0,       0xff,
    R0297J_PMFAGC_1,       0x0f,
    R0297J_PMFAGC_2,       0x12,
    R0297J_PMFAGC_3,       0x00,
    R0297J_PMFAGC_4,       0x00,
    R0297J_PMFAGC_5,       0x00,
    R0297J_INTER_0,        0xff,
    R0297J_INTER_1,        0xff,
    R0297J_SIG_FAD_0,      0x17,
    R0297J_SIG_FAD_2,      0x1e,
    R0297J_SIG_FAD_3,      0x37,
    R0297J_NEW_CRL_0,      0x04,
    R0297J_NEW_CRL_1,      0x08,
    R0297J_NEW_CRL_2,      0x02,
    R0297J_NEW_CRL_3,      0x05,
    R0297J_NEW_CRL_4,      0x02,
    R0297J_NEW_CRL_5,      0x05,
    R0297J_NEW_CRL_6,      0x08,
    R0297J_FREQ_0,         0x82,
    R0297J_FREQ_1,         0x6b,
    R0297J_FREQ_2,         0x05,
    R0297J_FREQ_3,         0x06,
    R0297J_FREQ_4,         0x0a,
    R0297J_FREQ_5,         0xf0,
    R0297J_FREQ_6,         0x00,
    R0297J_FREQ_7,         0x50,
    R0297J_FREQ_11,        0x22,
    R0297J_FREQ_12,        0xb4,
    R0297J_FREQ_13,        0x00,
    R0297J_FREQ_14,        0x00,
    R0297J_FREQ_15,        0xb0,
    R0297J_FREQ_16,        0xc8,
    R0297J_FREQ_17,        0x00,
    R0297J_FREQ_18,        0x00,
    R0297J_FREQ_19,        0x00,
    R0297J_FREQ_23,        0x00,
    R0297J_FREQ_24,        0x00,
    R0297J_DEINT_SYNC_0,   0x02,
    R0297J_BERT_0,         0x85,
    R0297J_BERT_1,         0x14,
    R0297J_BERT_2,         0x00,
    R0297J_DEINT_0,        0x91,
    R0297J_DEINT_1,        0x0b,
    R0297J_RS_DESC_6,      0x00,
    R0297J_RS_DESC_7,      0x00,
    R0297J_RS_DESC_8,      0x01,
    R0297J_CTRL_0,         0xc0,
    R0297J_CTRL_1,         0x00,
    R0297J_CTRL_2,         0x20,
    R0297J_CTRL_3,         0x80,
    R0297J_CTRL_4,         0x80,
    R0297J_CTRL_5,         0x00,
    R0297J_CTRL_6,         0x00,
    R0297J_CTRL_7,         0x00,
    R0297J_CTRL_9,         0x00,
    R0297J_CTRL_10,        0x1b, /* 58MHz */
    R0297J_CTRL_11,        0x00,
    R0297J_CTRL_12,        0x00,
    0x00
    };

const U8 FECA_QAM128_MACOETA50DR[] =
    {
    0x00
    };

const U8 FECA_QAM256_MACOETA50DR[] =
    {
    R0297J_EQU_0,          0x39,
    R0297J_EQU_1,          0x69,
    R0297J_EQU_3,          0x00,
    R0297J_EQU_4,          0x16,
    R0297J_EQU_5,          0x18,
    R0297J_EQU_6,          0x70,
    R0297J_INITDEM_0,      0x51,
    R0297J_INITDEM_1,      0xb8,
    R0297J_INITDEM_2,      0x00,
    R0297J_INITDEM_3,      0x00,
    R0297J_INITDEM_4,      0x00,
    R0297J_INITDEM_5,      0x84,
    R0297J_DELAGC_0,       0xff,
    R0297J_DELAGC_1,       0x4c,
    R0297J_DELAGC_2,       0xff,
    R0297J_DELAGC_3,       0x4c,
    R0297J_DELAGC_4,       0x29,
    R0297J_DELAGC_5,       0x66,
    R0297J_DELAGC_6,       0x80,
    R0297J_DELAGC_7,       0xf6,
    R0297J_WBAGC_0,        0xff,
    R0297J_WBAGC_1,        0x33,
    R0297J_WBAGC_2,        0x1a,
    R0297J_WBAGC_3,        0x18,
    R0297J_WBAGC_4,        0x85,
    R0297J_WBAGC_5,        0x11,
    R0297J_WBAGC_6,        0x80,
    R0297J_WBAGC_7,        0x12,
    R0297J_STLOOP_1,       0x08,
    R0297J_STLOOP_2,       0x00,
    R0297J_STLOOP_3,       0x08,
    R0297J_STLOOP_4,       0x30,
    R0297J_STLOOP_5,       0x11,
    R0297J_STLOOP_6,       0x6a,
    R0297J_STLOOP_7,       0x00,
    R0297J_STLOOP_8,       0x00,
    R0297J_STLOOP_9,       0x1e,
    R0297J_STLOOP_10,      0x04,
    R0297J_CRL_1,          0x4a,
    R0297J_CRL_2,          0x8b,
    R0297J_CRL_3,          0x00,
    R0297J_CRL_4,          0x9a,
    R0297J_CRL_5,          0x00,
    R0297J_CRL_6,          0x00,
    R0297J_CRL_7,          0x00,
    R0297J_CRL_8,          0x26,
    R0297J_CRL_9,          0x00,
    R0297J_CRL_10,         0x00,
    R0297J_CRL_11,         0x00,
    R0297J_CRL_12,         0x00,
    R0297J_CRL_13,         0x00,
    R0297J_CRL_14,         0x00,
    R0297J_PMFAGC_0,       0xff,
    R0297J_PMFAGC_1,       0x04,
    R0297J_PMFAGC_2,       0x10,
    R0297J_PMFAGC_3,       0x00,
    R0297J_PMFAGC_4,       0x00,
    R0297J_PMFAGC_5,       0x00,
    R0297J_INTER_0,        0xff,
    R0297J_INTER_1,        0xff,
    R0297J_SIG_FAD_0,      0x16,
    R0297J_SIG_FAD_2,      0x1e,
    R0297J_SIG_FAD_3,      0x37,
    R0297J_NEW_CRL_0,      0x04,
    R0297J_NEW_CRL_1,      0x0a,
    R0297J_NEW_CRL_2,      0x04,
    R0297J_NEW_CRL_3,      0x07,
    R0297J_NEW_CRL_4,      0x02,
    R0297J_NEW_CRL_5,      0x06,
    R0297J_NEW_CRL_6,      0x09,
    R0297J_FREQ_0,         0xc1,
    R0297J_FREQ_1,         0x7b,
    R0297J_FREQ_2,         0x05,
    R0297J_FREQ_3,         0x06,
    R0297J_FREQ_4,         0x08,
    R0297J_FREQ_5,         0xea,
    R0297J_FREQ_6,         0x81,
    R0297J_FREQ_7,         0x98,
    R0297J_FREQ_11,        0x22,
    R0297J_FREQ_12,        0xb4,
    R0297J_FREQ_13,        0x00,
    R0297J_FREQ_14,        0x00,
    R0297J_FREQ_15,        0xb0,
    R0297J_FREQ_16,        0xc8,
    R0297J_FREQ_17,        0x00,
    R0297J_FREQ_18,        0x00,
    R0297J_FREQ_19,        0x00,
    R0297J_FREQ_23,        0x00,
    R0297J_FREQ_24,        0x00,
    R0297J_DEINT_SYNC_0,   0x01,
    R0297J_BERT_0,         0x85,
    R0297J_BERT_1,         0xff,
    R0297J_BERT_2,         0xff,
    R0297J_DEINT_0,        0x91,
    R0297J_DEINT_1,        0x0b,
    R0297J_RS_DESC_6,      0x00,
    R0297J_RS_DESC_7,      0x00,
    R0297J_RS_DESC_8,      0x01,
    R0297J_CTRL_0,         0xc0,
    R0297J_CTRL_1,         0x00,
    R0297J_CTRL_2,         0x20,
    R0297J_CTRL_3,         0xa2,
    R0297J_CTRL_4,         0x81,
    R0297J_CTRL_5,         0x00,
    R0297J_CTRL_6,         0x00,
    R0297J_CTRL_7,         0x00,
    R0297J_CTRL_9,         0x00,
    R0297J_CTRL_10,        0x1f,
    R0297J_CTRL_11,        0x00,
    R0297J_CTRL_12,        0x00,
    0x00
    };

const U8 FECB_QAM64_MACOETA50DR[] =
    {
    0x00
    };

const U8 FECB_QAM256_MACOETA50DR[] =
    {
    0x00
    };

/* variables --------------------------------------------------------------- */

static  int    Driv0297JCN[6][40];

/* functions --------------------------------------------------------------- */


/*----------------------------------------------------
FUNCTION      Drv0297J_WaitTuner
ACTION        Wait for tuner locked
PARAMS IN     TimeOut -> Maximum waiting time (in ms)
PARAMS OUT    NONE
RETURN        NONE (Handle == THIS_INSTANCE.Tuner.DrvHandle)
------------------------------------------------------*/
void Drv0297J_WaitTuner(STTUNER_tuner_instance_t *pTunerInstance, int TimeOut)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297J
   const char *identity = "STTUNER drv0297J.c Drv0297J_WaitTuner()";
#endif
    int Time = 0;
    BOOL TunerLocked = FALSE;
    ST_ErrorCode_t Error;

    while(!TunerLocked && (Time < TimeOut))
    {
        STV0297J_DelayInMilliSec(1);
        Error = (pTunerInstance->Driver->tuner_IsTunerLocked)(pTunerInstance->DrvHandle, &TunerLocked);
        Time++;
    }
    Time--;
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297J
    STTBX_Print(("%s\n", identity));
#endif
}


/***********************************************************
**FUNCTION	::	Drv0297J_GetLLARevision
**ACTION	::	Returns the 297J LLA driver revision
**RETURN	::	Revision297J
***********************************************************/
ST_Revision_t Drv0297J_GetLLARevision(void)
{
	return (Revision297J);
}


/*----------------------------------------------------
--FUNCTION      Drv0297J_CarrierWidth
--ACTION        Compute the width of the carrier
--PARAMS IN     SymbolRate -> Symbol rate of the carrier (Kbauds or Mbauds)
--              RollOff    -> Rolloff * 100
--PARAMS OUT    NONE
--RETURN        Width of the carrier (KHz or MHz)
------------------------------------------------------*/

long Drv0297J_CarrierWidth(long SymbolRate, long RollOff)
{
    return (SymbolRate  + (SymbolRate * RollOff)/100);
}

/*----------------------------------------------------
--FUNCTION      Drv0297J_CheckAgc
--ACTION        Check for Agc
--PARAMS IN     Params        => Pointer to SEARCHPARAMS structure
--PARAMS OUT    Params->State => Result of the check
--RETURN        E297J_NOCARRIER carrier not founded, E297J_CARRIEROK otherwise
------------------------------------------------------*/
D0297J_SignalType_t Drv0297J_CheckAgc(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, D0297J_SearchParams_t *Params)
{
/*
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297J
   const char *identity = "STTUNER drv0297J.c Drv0297J_CheckAgc()";
#endif
*/

    if ( STTUNER_IOREG_GetField(DeviceMap, IOHandle, F0297J_WAGC_ACQ) )
    {
        Params->State = E297J_AGCOK;
      
    }
    else
    {
        Params->State = E297J_NOAGC;
       
    }
/*
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297J
    STTBX_Print(("%s\n", identity));
#endif
*/

    return(Params->State);
}

/*----------------------------------------------------
 FUNCTION      Drv0297J_CheckData
 ACTION        Check for data founded
 PARAMS IN     Params        =>    Pointer to SEARCHPARAMS structure
 PARAMS OUT    Params->State    => Result of the check
 RETURN        E297J_NODATA data not founded, E297J_DATAOK otherwise
------------------------------------------------------*/

D0297J_SignalType_t Drv0297J_CheckData(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, D0297J_SearchParams_t *Params)
{
/*
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297J
   const char *identity = "STTUNER drv0297J.c Drv0297J_CheckData()";
#endif
*/

#ifdef STTUNER_DRV_CAB_J83B
    if ( STTUNER_IOREG_GetField(DeviceMap, IOHandle, F0297J_MPEGB) )
#else
    if ( STTUNER_IOREG_GetField(DeviceMap, IOHandle, F0297J_MPEGA_LOCK) )
#endif
    {
        Params->State = E297J_DATAOK;
 
    }
    else
    {
        Params->State = E297J_NODATA;
       
    }
/*
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297J
    STTBX_Print(("%s\n", identity));
#endif
*/

    return(Params->State);
}


/*----------------------------------------------------
 FUNCTION      Drv0297J_InitParams
 ACTION
 PARAMS IN     NONE
 PARAMS OUT    NONE
 RETURN        NONE
------------------------------------------------------*/
void Drv0297J_InitParams(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297J
   const char *identity = "STTUNER drv0297J.c Drv0297J_InitParams()";
#endif
    int idB ;
    int iQAM ;

    /*
    Driv0297JCN[iQAM][idB] init

    This table is used for the C/N estimation (performed in the Driv0297JCNEstimator routine).
    Driv0297JCNB[iQAM][idB] gives the C/N estimated value for a given iQAM QAM size and idB C/N ratio.
    */
    for (idB = 0 ; idB < 40; idB++)
      for (iQAM = 0 ; iQAM < 5 ; iQAM++)
        Driv0297JCN[iQAM][idB] = 100000;
	/* QAM = 16 */
	iQAM = 0;
	for (idB = 0; idB < 14; idB++)
		Driv0297JCN[iQAM][idB] = 11210 + (14-idB)*1000;
	Driv0297JCN[iQAM][39] =  1452;
	Driv0297JCN[iQAM][38] =  1474;
	Driv0297JCN[iQAM][37] =  1509;
	Driv0297JCN[iQAM][36] =  1540;
	Driv0297JCN[iQAM][35] =  1601;
	Driv0297JCN[iQAM][34] =  1674;
	Driv0297JCN[iQAM][33] =  1798;
	Driv0297JCN[iQAM][32] =  1920;
	Driv0297JCN[iQAM][31] =  2048;
	Driv0297JCN[iQAM][30] =  2235;
	Driv0297JCN[iQAM][29] =  2432;
	Driv0297JCN[iQAM][28] =  2626;
	Driv0297JCN[iQAM][27] =  2880;
	Driv0297JCN[iQAM][26] =  3225;
	Driv0297JCN[iQAM][25] =  3574;
	Driv0297JCN[iQAM][24] =  3908;
	Driv0297JCN[iQAM][23] =  4318;
	Driv0297JCN[iQAM][22] =  4763;
	Driv0297JCN[iQAM][21] =  5375;
	Driv0297JCN[iQAM][20] =  6024;
	Driv0297JCN[iQAM][19] =  6713;
	Driv0297JCN[iQAM][18] =  7548;
	Driv0297JCN[iQAM][17] =  8359;
	Driv0297JCN[iQAM][16] =  9342;
	Driv0297JCN[iQAM][15] = 10240;
	Driv0297JCN[iQAM][14] = 11210;
	/* QAM = 32 */
	iQAM = 1;
	for (idB = 0; idB < 18; idB++)
		Driv0297JCN[iQAM][idB] = 10439 + (18-idB)*1000;
	Driv0297JCN[iQAM][39] =  1777;
	Driv0297JCN[iQAM][38] =  1848;
	Driv0297JCN[iQAM][37] =  1914;
	Driv0297JCN[iQAM][36] =  1988;
	Driv0297JCN[iQAM][35] =  2106;
	Driv0297JCN[iQAM][34] =  2238;
	Driv0297JCN[iQAM][33] =  2374;
	Driv0297JCN[iQAM][32] =  2557;
	Driv0297JCN[iQAM][31] =  2754;
	Driv0297JCN[iQAM][30] =  3012;
	Driv0297JCN[iQAM][29] =  3322;
	Driv0297JCN[iQAM][28] =  3638;
	Driv0297JCN[iQAM][27] =  4030;
	Driv0297JCN[iQAM][26] =  4425;
	Driv0297JCN[iQAM][25] =  4920;
	Driv0297JCN[iQAM][24] =  5441;
	Driv0297JCN[iQAM][23] =  6202;
	Driv0297JCN[iQAM][22] =  6843;
	Driv0297JCN[iQAM][21] =  7585;
	Driv0297JCN[iQAM][20] =  8422;
	Driv0297JCN[iQAM][19] =  9297;
	Driv0297JCN[iQAM][18] = 10439;
	/* QAM = 64 */
	iQAM = 2;
	for (idB = 0; idB < 20; idB++)
		Driv0297JCN[iQAM][idB] = 11778 + (20-idB)*1000;
	Driv0297JCN[iQAM][39] =  2176;
	Driv0297JCN[iQAM][38] =  2309;
	Driv0297JCN[iQAM][37] =  2405;
	Driv0297JCN[iQAM][36] =  2559;
	Driv0297JCN[iQAM][35] =  2741;
	Driv0297JCN[iQAM][34] =  2942;
	Driv0297JCN[iQAM][33] =  3188;
	Driv0297JCN[iQAM][32] =  3461;
	Driv0297JCN[iQAM][31] =  3773;
	Driv0297JCN[iQAM][30] =  4184;
	Driv0297JCN[iQAM][29] =  4578;
	Driv0297JCN[iQAM][28] =  5053;
	Driv0297JCN[iQAM][27] =  5577;
	Driv0297JCN[iQAM][26] =  6331;
	Driv0297JCN[iQAM][25] =  6979;
	Driv0297JCN[iQAM][24] =  7745;
	Driv0297JCN[iQAM][23] =  8586;
	Driv0297JCN[iQAM][22] =  9552;
	Driv0297JCN[iQAM][21] = 10674;
	Driv0297JCN[iQAM][20] = 11778;
	/* QAM = 128 */
	iQAM = 3;
	for (idB = 0; idB < 23; idB++)
		Driv0297JCN[iQAM][idB] = 11104 + (23-idB)*1000;
	Driv0297JCN[iQAM][39] =  2928;
	Driv0297JCN[iQAM][38] =  3069;
	Driv0297JCN[iQAM][37] =  3261;
	Driv0297JCN[iQAM][36] =  3462;
	Driv0297JCN[iQAM][35] =  3734;
	Driv0297JCN[iQAM][34] =  4030;
	Driv0297JCN[iQAM][33] =  4380;
	Driv0297JCN[iQAM][32] =  4782;
	Driv0297JCN[iQAM][31] =  5249;
	Driv0297JCN[iQAM][30] =  5817;
	Driv0297JCN[iQAM][29] =  6427;
	Driv0297JCN[iQAM][28] =  7059;
	Driv0297JCN[iQAM][27] =  7948;
	Driv0297JCN[iQAM][26] =  8773;
	Driv0297JCN[iQAM][25] =  9711;
	Driv0297JCN[iQAM][24] = 10726;
	Driv0297JCN[iQAM][23] = 11104;
	/* QAM = 256 */
	iQAM = 4;
	for (idB = 0; idB < 27;      idB++)
		Driv0297JCN[iQAM][idB] = 10874 + (27-idB)*1000;
	Driv0297JCN[iQAM][39] =  3897;
	Driv0297JCN[iQAM][38] =  4166;
	Driv0297JCN[iQAM][37] =  4421;
	Driv0297JCN[iQAM][36] =  4748;
	Driv0297JCN[iQAM][35] =  5111;
	Driv0297JCN[iQAM][34] =  5567;
	Driv0297JCN[iQAM][33] =  6076;
	Driv0297JCN[iQAM][32] =  6677;
	Driv0297JCN[iQAM][31] =  7290;
	Driv0297JCN[iQAM][30] =  8135;
	Driv0297JCN[iQAM][29] =  8950;
	Driv0297JCN[iQAM][28] =  9866;
	Driv0297JCN[iQAM][27] = 10874;

#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297J
    STTBX_Print(("%s\n", identity));
#endif
}

/*----------------------------------------------------
 FUNCTION      Drv0297J_InitSearch
 ACTION        Set Params fields that are used by the search algorithm
 PARAMS IN
 PARAMS OUT
 RETURN        NONE
------------------------------------------------------*/
void Drv0297J_InitSearch(STTUNER_tuner_instance_t *TunerInstance, D0297J_StateBlock_t *StateBlock,
                        STTUNER_Modulation_t Modulation, int Frequency, int SymbolRate, STTUNER_Spectrum_t Spectrum,
                        BOOL ScanExact, STTUNER_J83_t J83)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297J
   const char *identity = "STTUNER drv0297J.c Drv0297J_InitSearch()";
#endif
    U32 BandWidth;
    ST_ErrorCode_t Error;
    TUNER_Status_t TunerStatus;

    /* Obtain current tuner status */
    Error = (TunerInstance->Driver->tuner_GetStatus)(TunerInstance->DrvHandle, &TunerStatus);

    /* Select closest bandwidth for tuner */ /* cast to U32 type to match function argument & eliminate compiler warning --SFS */
    Error = (TunerInstance->Driver->tuner_SetBandWidth)( TunerInstance->DrvHandle,
                                                        (U32)( Drv0297J_CarrierWidth(SymbolRate, StateBlock->Params.RollOff) /1000 + 3000),
                                                        &BandWidth);

    /*
    --- Set Parameters
    */
    StateBlock->Params.Frequency    = Frequency;
    StateBlock->Params.SymbolRate   = SymbolRate;
    StateBlock->Params.TunerBW      = (long)BandWidth;
    StateBlock->Params.TunerStep    = (long)TunerStatus.TunerStep;
    StateBlock->Params.TunerIF      = (long)TunerStatus.IntermediateFrequency;
    StateBlock->Result.SignalType   = E297J_NOAGC;
    StateBlock->Result.Frequency    = 0;
    StateBlock->Result.SymbolRate   = 0;
    StateBlock->SpectrumInversion   = Spectrum;
    StateBlock->Params.Direction    = E297J_SCANUP;
    StateBlock->Params.State        = E297J_NOAGC;
#ifdef STTUNER_DRV_CAB_J83A
    StateBlock->J83                 = STTUNER_J83_A;
#endif
#ifdef STTUNER_DRV_CAB_J83B
    StateBlock->J83                 = STTUNER_J83_B;
#endif
#ifdef STTUNER_DRV_CAB_J83C
    StateBlock->J83                 = STTUNER_J83_C;
#endif

    if (ScanExact)
    {
        StateBlock->ScanMode = DEF0297J_CHANNEL;   /* Subranges are scanned in a zig-zag mode */
    }
    else
    {
        StateBlock->ScanMode = DEF0297J_SCAN;      /* No Subranges */
    }

    /*
    --- Set QAMSize and SweepRate.
    --- SweepRate = 1000* (SweepRate (in MHz/s) / Symbol Rate (in MBaud/s))
    */
    switch(Modulation)
    {
        case STTUNER_MOD_16QAM :
        case STTUNER_MOD_32QAM :
        case STTUNER_MOD_64QAM :
            StateBlock->QAMSize     = Modulation;           /* Set by user */
            StateBlock->SweepRate   = STV0297J_FAST_SWEEP_SPEED;
            break;

        case STTUNER_MOD_128QAM :
        case STTUNER_MOD_256QAM :
            StateBlock->QAMSize     = Modulation;           /* Set by user */
            StateBlock->SweepRate   = STV0297J_SLOW_SWEEP_SPEED;
            break;

        case STTUNER_MOD_QAM :
        default:
            StateBlock->QAMSize     = STTUNER_MOD_64QAM;    /* Most Common Modulation for Scan */
            StateBlock->SweepRate   = STV0297J_FAST_SWEEP_SPEED;
            break;
    }

    /*
    --- For low SR, we MUST divide Sweep by 2
    */
    if (SymbolRate <= STV0297J_SYMBOLRATE_LEVEL)
    {
        StateBlock->SweepRate /= 2;
    }

    /*
    --- CO
    */
    StateBlock->CarrierOffset = DEF0297J_CARRIEROFFSET_RATIO * 100;              /* in % */

    /*
    --- Set direction
    */
    if ( StateBlock->Params.Direction == E297J_SCANUP )
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297J
        STTBX_Print(("%s ==> SCANUP\n", identity));
#endif
        StateBlock->CarrierOffset *= -1;
        StateBlock->SweepRate     *= 1;
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297J
        STTBX_Print(("%s ==> SCANDOWN\n", identity));
#endif
        StateBlock->CarrierOffset *= 1;
        StateBlock->SweepRate     *= -1;
    }

#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297J
    STTBX_Print(("%s\n", identity));
#endif
}


/*----------------------------------------------------
 FUNCTION      Driv0297JDemodSetting
 ACTION
 PARAMS IN     NONE
 PARAMS OUT    NONE
 RETURN        NONE
------------------------------------------------------*/
void Driv0297JDemodSetting(STTUNER_IOREG_DeviceMap_t *DeviceMap_p, IOARCH_Handle_t IOHandle,
                          D0297J_StateBlock_t *StateBlock_p, STTUNER_tuner_instance_t *TunerInstance_p, long Offset)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297J
   const char *identity = "STTUNER drv0297J.c Driv0297JDemodSetting()";
#endif
    long long_tmp ;
    long ExtClk ;
    int  int_tmp ;
    U8   *p_tab=NULL;
    U8   addr, value;

#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297J
    STTBX_Print(("%s Offset %d\n", identity, Offset));
#endif

    /* initial demodulator setting : init freq = IF + Offset - Fclock */

    ExtClk = DeviceMap_p->RegExtClk;                    /* unit Khz */
    long_tmp  = StateBlock_p->Params.TunerIF + Offset;  /* in KHz */
    long_tmp -= ExtClk;                                 /* in KHz */
    long_tmp *= 512;
	long_tmp *= 128;
    long_tmp /= ExtClk;
    if(long_tmp > 65535) long_tmp = 65535 ;
    int_tmp = (int)long_tmp ;
    

    STTUNER_IOREG_SetField(DeviceMap_p, IOHandle,F0297J_DEM_FQCY_HI, (int)(MAC0297J_B1(int_tmp)));
    STTUNER_IOREG_SetField(DeviceMap_p, IOHandle,F0297J_DEM_FQCY_LO, (int)(MAC0297J_B0(int_tmp)));
    

#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297J
    STTBX_Print(("%s DEM_FQCY %d (Clk is %d KHz)\n", identity, int_tmp, ExtClk));
#endif

    /*
    --- Set Registers
    */

#ifdef STTUNER_DRV_CAB_J83A
    switch(StateBlock_p->QAMSize)
    {
        case STTUNER_MOD_16QAM :
            switch(TunerInstance_p->Driver->ID)
            {
#if defined( STTUNER_DRV_CAB_TUN_TDBE1X016A) || defined(STTUNER_DRV_CAB_TUN_TDBE1X601) || defined (STTUNER_DRV_CAB_TUN_TDEE4X012A)                
                case STTUNER_TUNER_TDBE1X016A:
                case STTUNER_TUNER_TDBE1X601:
                case STTUNER_TUNER_TDEE4X012A:  p_tab = (U8*)&FECA_QAM16_TDEE4[0];       break;
#endif                
#if defined( STTUNER_DRV_CAB_TUN_MT2030) || defined(STTUNER_DRV_CAB_TUN_MT2040) || defined (STTUNER_DRV_CAB_TUN_MT2050)      
                case STTUNER_TUNER_MT2030:
                case STTUNER_TUNER_MT2040:
                case STTUNER_TUNER_MT2050:      p_tab = (U8*)&FECA_QAM16_MT2050[0];      break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_DCT7040
                case STTUNER_TUNER_DCT7040:     p_tab = (U8*)&FECA_QAM16_DCT7040[0];     break;
#endif      
#ifdef STTUNER_DRV_CAB_TUN_DCT7050      
                case STTUNER_TUNER_DCT7050:     p_tab = (U8*)&FECA_QAM16_DCT7050[0];     break;
#endif      
#ifdef STTUNER_DRV_CAB_TUN_MACOETA50DR      
                case STTUNER_TUNER_MACOETA50DR: p_tab = (U8*)&FECA_QAM16_MACOETA50DR[0]; break;
#endif      
                default:                                                                 break;
            }
            break;
        case STTUNER_MOD_32QAM :
            switch(TunerInstance_p->Driver->ID)
            {
#if defined( STTUNER_DRV_CAB_TUN_TDBE1X016A) || defined(STTUNER_DRV_CAB_TUN_TDBE1X601) || defined (STTUNER_DRV_CAB_TUN_TDEE4X012A)                
               case STTUNER_TUNER_TDBE1X016A:
                case STTUNER_TUNER_TDBE1X601:
                case STTUNER_TUNER_TDEE4X012A:  p_tab = (U8*)&FECA_QAM32_TDEE4[0];       break;
#endif               
#if defined( STTUNER_DRV_CAB_TUN_MT2030) || defined(STTUNER_DRV_CAB_TUN_MT2040) || defined (STTUNER_DRV_CAB_TUN_MT2050)      
                case STTUNER_TUNER_MT2030:
                case STTUNER_TUNER_MT2040:
                case STTUNER_TUNER_MT2050:      p_tab = (U8*)&FECA_QAM32_MT2050[0];      break;
#endif        
#ifdef STTUNER_DRV_CAB_TUN_DCT7040               
                case STTUNER_TUNER_DCT7040:     p_tab = (U8*)&FECA_QAM32_DCT7040[0];     break;
#endif               
#ifdef STTUNER_DRV_CAB_TUN_DCT7050            
                case STTUNER_TUNER_DCT7050:     p_tab = (U8*)&FECA_QAM32_DCT7050[0];     break;
#endif               
#ifdef STTUNER_DRV_CAB_TUN_MACOETA50DR              
                case STTUNER_TUNER_MACOETA50DR: p_tab = (U8*)&FECA_QAM32_MACOETA50DR[0]; break;
#endif               
                default:                                                                 break;
            }
            break;
        case STTUNER_MOD_64QAM :
            switch(TunerInstance_p->Driver->ID)
            {
#if defined( STTUNER_DRV_CAB_TUN_TDBE1X016A) || defined(STTUNER_DRV_CAB_TUN_TDBE1X601) || defined (STTUNER_DRV_CAB_TUN_TDEE4X012A)                
                case STTUNER_TUNER_TDBE1X016A:
                case STTUNER_TUNER_TDBE1X601:
                case STTUNER_TUNER_TDEE4X012A:  p_tab = (U8*)&FECA_QAM64_TDEE4[0];       break;
#endif                 
#if defined( STTUNER_DRV_CAB_TUN_MT2030) || defined(STTUNER_DRV_CAB_TUN_MT2040) || defined (STTUNER_DRV_CAB_TUN_MT2050)               
                case STTUNER_TUNER_MT2030:
                case STTUNER_TUNER_MT2040:
                case STTUNER_TUNER_MT2050:      p_tab = (U8*)&FECA_QAM64_MT2050[0];      break;
#endif                
#ifdef STTUNER_DRV_CAB_TUN_DCT7040                 
                case STTUNER_TUNER_DCT7040:     p_tab = (U8*)&FECA_QAM64_DCT7040[0];     break;
#endif                
#ifdef STTUNER_DRV_CAB_TUN_DCT7050                
                case STTUNER_TUNER_DCT7050:     p_tab = (U8*)&FECA_QAM64_DCT7050[0];     break;
#endif                 
#ifdef STTUNER_DRV_CAB_TUN_MACOETA50DR                
                case STTUNER_TUNER_MACOETA50DR: p_tab = (U8*)&FECA_QAM64_MACOETA50DR[0]; break;
 #endif                
                default:                                                                 break;
            }
            break;
        case STTUNER_MOD_128QAM :
            switch(TunerInstance_p->Driver->ID)
            {
#if defined( STTUNER_DRV_CAB_TUN_TDBE1X016A) || defined(STTUNER_DRV_CAB_TUN_TDBE1X601) || defined (STTUNER_DRV_CAB_TUN_TDEE4X012A)                
                case STTUNER_TUNER_TDBE1X016A:
                case STTUNER_TUNER_TDBE1X601:
                case STTUNER_TUNER_TDEE4X012A:  p_tab = (U8*)&FECA_QAM128_TDEE4[0];       break;
#endif                 
#if defined( STTUNER_DRV_CAB_TUN_MT2030) || defined(STTUNER_DRV_CAB_TUN_MT2040) || defined (STTUNER_DRV_CAB_TUN_MT2050)              
                case STTUNER_TUNER_MT2030:
                case STTUNER_TUNER_MT2040:
                case STTUNER_TUNER_MT2050:      p_tab = (U8*)&FECA_QAM128_MT2050[0];      break;
#endif                 
#ifdef STTUNER_DRV_CAB_TUN_DCT7040                
                case STTUNER_TUNER_DCT7040:     p_tab = (U8*)&FECA_QAM128_DCT7040[0];     break;
#endif                 
#ifdef STTUNER_DRV_CAB_TUN_DCT7050                
                case STTUNER_TUNER_DCT7050:     p_tab = (U8*)&FECA_QAM128_DCT7050[0];     break;
#endif                 
#ifdef STTUNER_DRV_CAB_TUN_MACOETA50DR                 
                case STTUNER_TUNER_MACOETA50DR: p_tab = (U8*)&FECA_QAM128_MACOETA50DR[0]; break;
#endif                 
                default:                                                                  break;
            }
            break;
        case STTUNER_MOD_256QAM :
            switch(TunerInstance_p->Driver->ID)
            {
#if defined( STTUNER_DRV_CAB_TUN_TDBE1X016A) || defined(STTUNER_DRV_CAB_TUN_TDBE1X601) || defined (STTUNER_DRV_CAB_TUN_TDEE4X012A)                
               case STTUNER_TUNER_TDBE1X016A:
                case STTUNER_TUNER_TDBE1X601:
                case STTUNER_TUNER_TDEE4X012A:  p_tab = (U8*)&FECA_QAM256_TDEE4[0];       break;
#endif                
#if defined( STTUNER_DRV_CAB_TUN_MT2030) || defined(STTUNER_DRV_CAB_TUN_MT2040) || defined (STTUNER_DRV_CAB_TUN_MT2050)              
                case STTUNER_TUNER_MT2030:
                case STTUNER_TUNER_MT2040:
                case STTUNER_TUNER_MT2050:      p_tab = (U8*)&FECA_QAM256_MT2050[0];      break;
#endif                
#ifdef STTUNER_DRV_CAB_TUN_DCT7040                 
                case STTUNER_TUNER_DCT7040:     p_tab = (U8*)&FECA_QAM256_DCT7040[0];     break;
#endif                
#ifdef STTUNER_DRV_CAB_TUN_DCT7050                
                case STTUNER_TUNER_DCT7050:     p_tab = (U8*)&FECA_QAM256_DCT7050[0];     break;
#endif                
#ifdef STTUNER_DRV_CAB_TUN_MACOETA50DR                 
                case STTUNER_TUNER_MACOETA50DR: p_tab = (U8*)&FECA_QAM256_MACOETA50DR[0]; break;
#endif                
                default:                                                                  break;
            }
            break;
        default:
            break;
    }
#endif

#ifdef STTUNER_DRV_CAB_J83B
    switch(StateBlock_p->QAMSize)
    {
        case STTUNER_MOD_64QAM :
            switch(TunerInstance_p->Driver->ID)
            {
#if defined( STTUNER_DRV_CAB_TUN_TDBE1X016A) || defined(STTUNER_DRV_CAB_TUN_TDBE1X601) || defined (STTUNER_DRV_CAB_TUN_TDEE4X012A)                
                case STTUNER_TUNER_TDBE1X016A:
                case STTUNER_TUNER_TDBE1X601:
                case STTUNER_TUNER_TDEE4X012A:  p_tab = (U8*)&FECB_QAM64_TDEE4[0];       break;
#endif                
#if defined( STTUNER_DRV_CAB_TUN_MT2030) || defined(STTUNER_DRV_CAB_TUN_MT2040) || defined (STTUNER_DRV_CAB_TUN_MT2050)                 
                case STTUNER_TUNER_MT2030:
                case STTUNER_TUNER_MT2040:
                case STTUNER_TUNER_MT2050:      p_tab = (U8*)&FECB_QAM64_MT2050[0];      break;
#endif                
#ifdef STTUNER_DRV_CAB_TUN_DCT7040                 
                case STTUNER_TUNER_DCT7040:     p_tab = (U8*)&FECB_QAM64_DCT7040[0];     break;
#endif                
#ifdef STTUNER_DRV_CAB_TUN_DCT7050                 
                case STTUNER_TUNER_DCT7050:     p_tab = (U8*)&FECB_QAM64_DCT7050[0];     break;
#endif               
#ifdef STTUNER_DRV_CAB_TUN_MACOETA50DR                 
                case STTUNER_TUNER_MACOETA50DR: p_tab = (U8*)&FECB_QAM64_MACOETA50DR[0]; break;
#endif                
                default:                                                                 break;
            }
            break;
        case STTUNER_MOD_256QAM :
            switch(TunerInstance_p->Driver->ID)
            {
#if defined( STTUNER_DRV_CAB_TUN_TDBE1X016A) || defined(STTUNER_DRV_CAB_TUN_TDBE1X601) || defined (STTUNER_DRV_CAB_TUN_TDEE4X012A)                
               case STTUNER_TUNER_TDBE1X016A:
                case STTUNER_TUNER_TDBE1X601:
                case STTUNER_TUNER_TDEE4X012A:  p_tab = (U8*)&FECB_QAM256_TDEE4[0];       break;
#endif                
#if defined( STTUNER_DRV_CAB_TUN_MT2030) || defined(STTUNER_DRV_CAB_TUN_MT2040) || defined (STTUNER_DRV_CAB_TUN_MT2050)                
                case STTUNER_TUNER_MT2030:
                case STTUNER_TUNER_MT2040:
                case STTUNER_TUNER_MT2050:      p_tab = (U8*)&FECB_QAM256_MT2050[0];      break;
#endif                
#ifdef STTUNER_DRV_CAB_TUN_DCT7040                 
                case STTUNER_TUNER_DCT7040:     p_tab = (U8*)&FECB_QAM256_DCT7040[0];     break;
#endif                
 #ifdef STTUNER_DRV_CAB_TUN_DCT7050               
                case STTUNER_TUNER_DCT7050:     p_tab = (U8*)&FECB_QAM256_DCT7050[0];     break;
#endif                
#ifdef STTUNER_DRV_CAB_TUN_MACOETA50DR                  
                case STTUNER_TUNER_MACOETA50DR: p_tab = (U8*)&FECB_QAM256_MACOETA50DR[0]; break;
#endif                
                default:                                                                  break;
            }
            break;
        default:
            break;
    }
#endif

#ifdef STTUNER_DRV_CAB_J83C
    switch(StateBlock_p->QAMSize)
    {
        case STTUNER_MOD_16QAM :
            switch(TunerInstance_p->Driver->ID)
            {
                case STTUNER_TUNER_TDBE1X016A:
                case STTUNER_TUNER_TDBE1X601:
                case STTUNER_TUNER_TDEE4X012A:
                case STTUNER_TUNER_MT2030:
                case STTUNER_TUNER_MT2040:
                case STTUNER_TUNER_MT2050:
                case STTUNER_TUNER_DCT7040:
                case STTUNER_TUNER_DCT7050:
                case STTUNER_TUNER_MACOETA50DR:                    break;
                default:                                           break;
            }
            break;
        case STTUNER_MOD_32QAM :
            switch(TunerInstance_p->Driver->ID)
            {
                case STTUNER_TUNER_TDBE1X016A:
                case STTUNER_TUNER_TDBE1X601:
                case STTUNER_TUNER_TDEE4X012A:
                case STTUNER_TUNER_MT2030:
                case STTUNER_TUNER_MT2040:
                case STTUNER_TUNER_MT2050:
                case STTUNER_TUNER_DCT7040:
                case STTUNER_TUNER_DCT7050:
                case STTUNER_TUNER_MACOETA50DR:                    break;
                default:                                           break;
            }
            break;
        case STTUNER_MOD_64QAM :
            switch(TunerInstance_p->Driver->ID)
            {
                case STTUNER_TUNER_TDBE1X016A:
                case STTUNER_TUNER_TDBE1X601:
                case STTUNER_TUNER_TDEE4X012A:
                case STTUNER_TUNER_MT2030:
                case STTUNER_TUNER_MT2040:
                case STTUNER_TUNER_MT2050:
                case STTUNER_TUNER_DCT7040:
                case STTUNER_TUNER_DCT7050:
                case STTUNER_TUNER_MACOETA50DR:                    break;
                default:                                           break;
            }
            break;
        case STTUNER_MOD_128QAM :
            switch(TunerInstance_p->Driver->ID)
            {
                case STTUNER_TUNER_TDBE1X016A:
                case STTUNER_TUNER_TDBE1X601:
                case STTUNER_TUNER_TDEE4X012A:
                case STTUNER_TUNER_MT2030:
                case STTUNER_TUNER_MT2040:
                case STTUNER_TUNER_MT2050:
                case STTUNER_TUNER_DCT7040:
                case STTUNER_TUNER_DCT7050:
                case STTUNER_TUNER_MACOETA50DR:                    break;
                default:                                           break;
            }
            break;
        case STTUNER_MOD_256QAM :
            switch(TunerInstance_p->Driver->ID)
            {
                case STTUNER_TUNER_TDBE1X016A:
                case STTUNER_TUNER_TDBE1X601:
                case STTUNER_TUNER_TDEE4X012A:
                case STTUNER_TUNER_MT2030:
                case STTUNER_TUNER_MT2040:
                case STTUNER_TUNER_MT2050:
                case STTUNER_TUNER_DCT7040:
                case STTUNER_TUNER_DCT7050:
                case STTUNER_TUNER_MACOETA50DR:                    break;
                default:                                           break;
            }
            break;
        default:
            break;
    }
#endif

    /* Write to registers */
    do
    {
        addr  = *p_tab++;
        value = *p_tab++;
        STTUNER_IOREG_SetRegister( DeviceMap_p, IOHandle,  addr, value);
    }
    while (*p_tab!=0);

}

/*----------------------------------------------------
 FUNCTION      Driv0297JCNEstimator
 ACTION
 PARAMS IN     NONE
 PARAMS OUT    NONE
 RETURN        NONE
------------------------------------------------------*/
void Driv0297JCNEstimator(STTUNER_IOREG_DeviceMap_t *DeviceMap_p, IOARCH_Handle_t IOHandle,
                         D0297J_StateBlock_t *StateBlock_p, STTUNER_tuner_instance_t *TunerInstance_p,
                         int *Mean_p, int *CN_dB100_p, int *CN_Ratio_p)
{
/*
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297J
   const char *identity = "STTUNER drv0297J.c Driv0297JCNEstimator()";
#endif
*/

    int CNEstimation=0;
    int CNEstimatorOffset=0;
    int int_tmp , i;
    int idB;
    int iQAM=0;
    int CurrentMean;
    int ComputedMean;
    int OldMean;
    int Result;
    int Driv0297JCN_Max, Driv0297JCN_Min;
   

    switch (Reg0297J_GetQAMSize(DeviceMap_p, IOHandle))
    {
        case STTUNER_MOD_16QAM :
            iQAM = 0;
            break;
        case STTUNER_MOD_32QAM :
            iQAM = 1;
            break;
        case STTUNER_MOD_64QAM :
            iQAM = 2;
            break;
        case STTUNER_MOD_128QAM :
            iQAM = 3;
            break;
        case STTUNER_MOD_256QAM :
            iQAM = 4;
            break;
        default :
            break; 	     
    }
    /*
    the STV0297J noise estimation must be filtered. The used filter is :
      Estimation(n) = 63/64*Estimation(n-1) + 1/64*STV0297JNoiseEstimation
    */
    for (i = 0 ; i < 100 ; i ++)
    {
        STV0297J_DelayInMilliSec(1);                                     /* wait 1 ms */
        
      
        
         int_tmp = MAC0297J_MAKEWORD(STTUNER_IOREG_GetField(DeviceMap_p, IOHandle,F0297J_NOISE_EST_HI),
                                    STTUNER_IOREG_GetField(DeviceMap_p, IOHandle,F0297J_NOISE_EST_LO));
        if (i == 0) CNEstimation = int_tmp;
        else        CNEstimation = (63*CNEstimation)/64 + int_tmp/64;
    }

    CNEstimatorOffset = 0;
    ComputedMean  = CNEstimation - CNEstimatorOffset  ; /* offset correction */
    CurrentMean   = Driv0297JCN[iQAM][0] ;
    idB = 1 ;
    while (TRUE)
    {
        OldMean = CurrentMean ;
        CurrentMean = Driv0297JCN[iQAM][idB] ;
        if ((CurrentMean <= ComputedMean)||(idB >= 39))
        {
            break;
        }
        else
        {
            idB += 1;
        }
    }
    /*
    An interpolation is performed in order to increase the result accuracy
    ( < 0.3 dB accuracy is typically reached)
    */
    Result = 100*idB ;
    if(CurrentMean < OldMean)
        Result -= (100*(CurrentMean - ComputedMean))/(CurrentMean - OldMean) ;

    *Mean_p = CNEstimation ;
    *CN_dB100_p   = Result ;

    Driv0297JCN_Max = Driv0297JCN[iQAM][0];
    Driv0297JCN_Min = Driv0297JCN[iQAM][39];
    *CN_Ratio_p   = 100 * CNEstimation;
    *CN_Ratio_p   /= (Driv0297JCN_Max-Driv0297JCN_Min);
    if ( *CN_Ratio_p > 100 )
    {
        *CN_Ratio_p = 0;
    }else if ( *CN_Ratio_p < 0 )
    {
        *CN_Ratio_p = 100;
    }
    else
    {
        *CN_Ratio_p   = 100 - *CN_Ratio_p;
    }

/*
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297J
    STTBX_Print(("%s Mean %d CNdBx100 %d CNRatio %d\n", identity, *Mean_p, *CN_dB100_p, *CN_Ratio_p));
#endif
*/

}

/*----------------------------------------------------
 FUNCTION      Drv0297JApplicationSaturationComputation
 ACTION
 PARAMS IN     NONE
 PARAMS OUT    NONE
 RETURN        NONE
------------------------------------------------------*/
void Drv0297JApplicationSaturationComputation(int * _pSaturation)
{
    int int_tmp ;
    /*
    In order to get a smooth saturation value, this is filtered
    with a low-pass filter : sat(n) = 0.25*sat(n-1) + 0.75*acc(n)
    pSaturation[0] = accumulator value
    pSaturation[1] = low-pass filter memory
    */
    if (_pSaturation[0] != 0)
    {
        int_tmp  = 100*_pSaturation[0];
        int_tmp /= 65536;
        if(_pSaturation[1] > int_tmp)
        {
            int_tmp  = 10*int_tmp/100;
            int_tmp += 90*_pSaturation[1]/100;
        }
        else
        {
            int_tmp  = 75*int_tmp/100;
            int_tmp += 25*_pSaturation[1]/100;
        }

        if(int_tmp > 100) int_tmp = 100;

        _pSaturation[1] = int_tmp ;
    }
    return ;
}

/*----------------------------------------------------
 FUNCTION      Driv0297JBertCount
 ACTION
 PARAMS IN     NONE
 PARAMS OUT    NONE
 RETURN        NONE
------------------------------------------------------*/
void Driv0297JBertCount(STTUNER_IOREG_DeviceMap_t *DeviceMap_p, IOARCH_Handle_t IOHandle,
                       D0297J_StateBlock_t *StateBlock_p, STTUNER_tuner_instance_t *TunerInstance_p,
                       int *_pBER_Cnt, int *_pBER)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297J
    const char *identity = "STTUNER drv0297J.c Driv0297JBertCount()";
#endif
    int     err_hi, err_lo;
    int     ErrMode;
    int     int_tmp, ii;
    U32     BER;
    int     NByte;

    /*
    --- Set parameters
    */
    _pBER[0] = 0;


    /*
    --- Compute ...
    ---
    ---     BER =   10E6 * (Counter)/(2^(2*NBYTE + 12))
    ---         = 244,14 * (Counter)/(2^(2*NBYTE))                   (Unit 10E-6)
    */
    ErrMode = STTUNER_IOREG_GetField(DeviceMap_p, IOHandle, F0297J_ERR_MODE);
    if(ErrMode == 0)
    {
        /* rate mode */
        if(STTUNER_IOREG_GetField(DeviceMap_p, IOHandle, F0297J_BERT_ON) == 0)
        {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297J
            STTBX_Print(("===> Count is finished\n"));
#endif
            /* the previous count is finished */
            /* Get BER from demod */
            err_hi = STTUNER_IOREG_GetField(DeviceMap_p, IOHandle, F0297J_ERRCOUNT_HI);
            err_lo = STTUNER_IOREG_GetField(DeviceMap_p, IOHandle, F0297J_ERRCOUNT_LO);
            err_hi = STTUNER_IOREG_GetField(DeviceMap_p, IOHandle, F0297J_ERRCOUNT_HI);
            err_lo = STTUNER_IOREG_GetField(DeviceMap_p, IOHandle, F0297J_ERRCOUNT_LO);
            *_pBER_Cnt = err_lo + (err_hi <<8);
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297J
            STTBX_Print(("===> _pBER_Cnt = %d\n", *_pBER_Cnt));
#endif
            BER = *_pBER_Cnt;

            NByte = STTUNER_IOREG_GetField(DeviceMap_p, IOHandle, F0297J_NBYTE);
            BER *= 24414;

            for (ii = 0; ii < (2*NByte); ii++) BER >>= 1;

            if(STTUNER_IOREG_GetField(DeviceMap_p, IOHandle, F0297J_ERR_SOURCE) == 0)
            {
                /* bit count */
                BER /= 800;
            }
            else
            {
                /* byte count */
                BER /= 100;
            }
            _pBER[2] = (int)BER;
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297J
            STTBX_Print(("===> BER = %d 10E-6\n", BER));
#endif

            _pBER[0] = *_pBER_Cnt;
            Drv0297JApplicationSaturationComputation(_pBER);
            /*
            --- Update NBYTE ?
            */
            if (_pBER[1] > 50)  /* Saturation */
            {
                int_tmp = STTUNER_IOREG_GetField(DeviceMap_p, IOHandle, F0297J_NBYTE) ;
                int_tmp -= 1 ;
                STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297J_NBYTE, int_tmp);
            } else if (_pBER[1] < 15)
            {
                int_tmp = STTUNER_IOREG_GetField(DeviceMap_p, IOHandle, F0297J_NBYTE) ;
                int_tmp += 1;
                STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297J_NBYTE, int_tmp);
            }
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297J
            STTBX_Print(("===> Start a new count with NBYTE = %d \n", STTUNER_IOREG_GetField(DeviceMap_p, IOHandle, F0297J_NBYTE)));
#endif
            STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297J_BERT_ON,1) ; /* restart a new count */
        }
        else
        {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297J
            STTBX_Print(("===> Keep latest BER value\n"));
#endif
        }
        _pBER[0] = *_pBER_Cnt;
    }

    /*
    --- Get Blk parameters
    */
    StateBlock_p->BlkCounter = Reg0297J_GetBlkCounter(DeviceMap_p, IOHandle);
    StateBlock_p->CorrBlk    = Reg0297J_GetCorrBlk(DeviceMap_p, IOHandle);
    StateBlock_p->UncorrBlk  = Reg0297J_GetUncorrBlk(DeviceMap_p, IOHandle);

#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297J
    STTBX_Print(("%s BER_Cnt %9d BER Cnt %9d Sat %3d Rate %9d (10E-6)\n", identity,
                *_pBER_Cnt,
                _pBER[0],
                _pBER[1],
                _pBER[2]
                ));
    STTBX_Print(("%s BlkCounter %9d CorrBlk %9d UncorrBlk %9d\n", identity,
                StateBlock_p->BlkCounter,
                StateBlock_p->CorrBlk,
                StateBlock_p->UncorrBlk
                ));
#endif

}

/*
This routine initializes the desired FEC and resets the blocks of the unused one
FECType is the value of FEC_AC_OR_B. Two different configuration files will be
used with these two types
FECType is the value of FEC_AC_OR_B
*/
void Driv0297JFecInit (STTUNER_IOREG_DeviceMap_t *DeviceMap_p, IOARCH_Handle_t IOHandle,
                       D0297J_StateBlock_t *StateBlock_p)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297J
    const char *identity = "STTUNER drv0297J.c Driv0297JFecInit()";
#endif
    int FecType;
    
    FecType = STTUNER_IOREG_GetField(DeviceMap_p, IOHandle, F0297J_FEC_AC_OR_B);
    if (FecType == 0)       /* FEC B chosen */
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297J
        STTBX_Print(("%s FEC B chosen\n", identity));
#endif
      
        
        
        STTUNER_IOREG_SetField(DeviceMap_p, IOHandle,F0297J_RESET_DI,1);              /* FEC A/C deinterleaver reset */
        STTUNER_IOREG_SetField(DeviceMap_p, IOHandle,F0297J_RESET_RS,1);              /* FEC A/C RS reset */
        STTUNER_IOREG_SetField(DeviceMap_p, IOHandle,F0297J_PAGE_MODE,0);             /* FEC B registers page mode selected */
      
        STTUNER_IOREG_SetField(DeviceMap_p, IOHandle,F0297J_FEC_RESET,0);             /* FEC B reset */
       
        STTUNER_IOREG_SetField(DeviceMap_p, IOHandle,F0297J_FEC_RESET,1);             /* FEC B reset cleared */
       
        STTUNER_IOREG_SetField(DeviceMap_p,IOHandle, F0297J_VIT_RESET,0);             /* FEC B viterbi reset */
        STTUNER_IOREG_SetField(DeviceMap_p, IOHandle,F0297J_RS_EN,0);                 /* FEC B RS correction disabled */
       
        STTUNER_IOREG_SetField(DeviceMap_p, IOHandle,F0297J_VIT_RESET,1);             /* FEC B viterbi reset cleared */
        STTUNER_IOREG_SetField(DeviceMap_p, IOHandle,F0297J_RS_EN,1);                 /* FEC B RS correction enabled */
       

        
    }
    else           /* FEC A/C chosen */
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297J
        STTBX_Print(("%s FEC A/C chosen\n", identity));
#endif
        STTUNER_IOREG_SetRegister( DeviceMap_p, IOHandle, R0297J_CTRL_0, 0x80); /* FEC B registers page mode selected */
        STTUNER_IOREG_SetRegister( DeviceMap_p, IOHandle, R0297J_IN_DEPTH, 0);  /* FEC B reset */
        STTUNER_IOREG_SetRegister( DeviceMap_p, IOHandle, R0297J_MPEG_CTRL, 0); /* FEC B viterbi and RS correction reset */
     
     
        
        
        STTUNER_IOREG_SetField(DeviceMap_p, IOHandle,F0297J_PAGE_MODE,1);             /* TSMF registers page mode selected */
        STTUNER_IOREG_SetField(DeviceMap_p, IOHandle,F0297J_RESET_DI,1);              /* FEC A/C deinterleaver&descrambler reset */
        STTUNER_IOREG_SetField(DeviceMap_p, IOHandle,F0297J_RESET_RS,1);              /* FEC A/C Reed-Salomon reset */
       
        STTUNER_IOREG_SetField(DeviceMap_p, IOHandle,F0297J_RESET_DI,0);              /* FEC A/C deinterleaver&descrambler reset released */
        STTUNER_IOREG_SetField(DeviceMap_p, IOHandle,F0297J_RESET_RS,0);              /* FEC A/C Reed-Salomon reset released */
       

        STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297J_DI_UNLOCK,1);      /* FEC A/C deinterleaver unlock forcing */
        STTUNER_IOREG_SetField(DeviceMap_p, IOHandle,F0297J_TRKMODE,2);               /* FEC A/C number of bytes required to unlock : 3 */
        STTUNER_IOREG_SetField(DeviceMap_p, IOHandle,F0297J_DI_UNLOCK,0);             /* FEC A/C deinterleaver unlock forcing released */
       
        STTUNER_IOREG_SetField(DeviceMap_p, IOHandle,F0297J_DI_SY_EV,0);              /* FEC A/C deinterleaver status clear */
        STTUNER_IOREG_SetField(DeviceMap_p, IOHandle,F0297J_SYNC_EV,0);               /* FEC A/C descrambler status clear */
      
        
        
        
    }
    return;
}

BOOL Driv0297JFECLockCheck (STTUNER_IOREG_DeviceMap_t *DeviceMap_p, IOARCH_Handle_t IOHandle,
                            D0297J_StateBlock_t *StateBlock_p,
                            BOOL EndOfSearch, long TimeOut, long DataSearchTime)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297J
    const char *identity = "STTUNER drv0297J.c Driv0297JFECLockCheck()";
#endif
    int     QAMSize,int_tmp,FECTrackingLock = 0, MPEGLockCount;
    int     Log2QAMSize=0;
    long    FECTimeOut;
    long    noCodeWordsInAframe = 60, FrameSyncTimeOut, ViterbiTimeOut, MPEGTimeOut;
    long    SymbolRate;
    BOOL    DataFound = false;
    

    int_tmp = STTUNER_IOREG_GetField(DeviceMap_p, IOHandle, F0297J_FEC_AC_OR_B);
    QAMSize     = StateBlock_p->QAMSize    ;
    SymbolRate  = StateBlock_p->Params.SymbolRate ; /* in Baud */
    switch (QAMSize)
    {
    case STTUNER_MOD_16QAM :
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297J
        STTBX_Print(("%s QAM16\n", identity));
#endif
        Log2QAMSize = 4   ;
        break;
    case STTUNER_MOD_32QAM :
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297J
        STTBX_Print(("%s QAM32\n", identity));
#endif
        Log2QAMSize = 5   ;
        break;
    case STTUNER_MOD_64QAM :
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297J
        STTBX_Print(("%s QAM64\n", identity));
#endif
        Log2QAMSize = 6   ;
        noCodeWordsInAframe = 60;
        break;
    case STTUNER_MOD_128QAM :
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297J
        STTBX_Print(("%s QAM128\n", identity));
#endif
        Log2QAMSize = 7   ;
        break;
    case STTUNER_MOD_256QAM :
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297J
        STTBX_Print(("%s QAM256\n", identity));
#endif
        Log2QAMSize = 8   ;
        noCodeWordsInAframe = 88;
        break;
    }

    if (int_tmp == 1)   /* FEC A/C */
    {
        /* FECTimeOut (in mSec) : it is the time necessary for the FEC A/C to lock in the worst case. */
        FECTimeOut = ((17+16)*(204*8))/(Log2QAMSize*(SymbolRate/1000)) ; /* in mSec */
        FECTimeOut += 10 ;
        TimeOut = DataSearchTime + FECTimeOut;
        while (!EndOfSearch)
        {
            STV0297J_DelayInMilliSec(STV0297J_DEMOD_WAITING_TIME);
            DataSearchTime +=STV0297J_DEMOD_WAITING_TIME ;
            FECTrackingLock = STTUNER_IOREG_GetField(DeviceMap_p, IOHandle, F0297J_MPEGA_LOCK) ;
            if((FECTrackingLock == 1)||(DataSearchTime > TimeOut))
                EndOfSearch = TRUE ;
        }
        if(FECTrackingLock == 1)
            DataFound = TRUE;
        else
		{
            STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297J_SPEC_INV, !STTUNER_IOREG_GetField(DeviceMap_p, IOHandle, F0297J_SPEC_INV));
            STV0297J_DelayInMilliSec(10);
            DataSearchTime += 10;
            if(STTUNER_IOREG_GetField(DeviceMap_p, IOHandle, F0297J_MPEGA_LOCK) == 1)
                DataFound = TRUE;
		}

        if(!DataFound)
        {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297J
            STTBX_Print(("%sFECTimeOut\n", identity));
          
        
        
        int_tmp  = STTUNER_IOREG_GetField(DeviceMap_p, IOHandle,F0297J_MPEGA_LOCK) * 128;
        int_tmp += STTUNER_IOREG_GetField(DeviceMap_p, IOHandle,F0297J_UNCORRA)    * 64;
        int_tmp += STTUNER_IOREG_GetField(DeviceMap_p, IOHandle,F0297J_DI_LOCKA)   * 32;
        int_tmp += STTUNER_IOREG_GetField(DeviceMap_p, IOHandle,F0297J_CORNER_LOCK)* 16;
        int_tmp += STTUNER_IOREG_GetField(DeviceMap_p, IOHandle,F0297J_EQU_LMS2)   * 8;
        int_tmp += STTUNER_IOREG_GetField(DeviceMap_p, IOHandle,F0297J_EQU_LMS1)   * 4;
        int_tmp += STTUNER_IOREG_GetField(DeviceMap_p, IOHandle,F0297J_WBAGC_IT)   * 2;
        int_tmp += STTUNER_IOREG_GetField(DeviceMap_p, IOHandle,F0297J_MPEGB);
        
            STTBX_Print(("INTER_2 = 0x%x\n", int_tmp));


#endif
            return (DataFound);
        }
    }
    else
    {
        ViterbiTimeOut = 20;
        MPEGTimeOut = 200;
        /* frameSyncTimeOut (in mSec) : it is the time necessary for an MPEG frame lock in the worst case. */
        FrameSyncTimeOut = (noCodeWordsInAframe*128*7)/(Log2QAMSize*(SymbolRate/1000)); /* in ms */
        /* length of a code word is 128 words; each word is of 7 bits */
        /* Waiting for MPEG sync Lock or a time-out; A few  successful */
        /* tracking indicators are required in succession to decide MPEG Sync */
        EndOfSearch = FALSE;
        MPEGTimeOut = MPEGTimeOut + ViterbiTimeOut + FrameSyncTimeOut;
        TimeOut = DataSearchTime + MPEGTimeOut;
        MPEGLockCount = 0;
        while (!EndOfSearch)
        {
            STV0297J_DelayInMilliSec(STV0297J_DEMOD_WAITING_TIME);
            DataSearchTime +=STV0297J_DEMOD_WAITING_TIME ;
            if(STTUNER_IOREG_GetField(DeviceMap_p, IOHandle, F0297J_MPEG_SYNC) == 1)   MPEGLockCount++;
            else	MPEGLockCount  = 0;
            if((MPEGLockCount == 1)||(DataSearchTime > TimeOut))
                EndOfSearch = TRUE;
        }
        if(MPEGLockCount == 1)
            DataFound = TRUE;
		else
		{
            STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297J_SPEC_INV,!STTUNER_IOREG_GetField(DeviceMap_p, IOHandle, F0297J_SPEC_INV));
            STV0297J_DelayInMilliSec(20);
            DataSearchTime += 20;
            if(STTUNER_IOREG_GetField(DeviceMap_p, IOHandle, F0297J_MPEG_SYNC) == 1)
                DataFound = TRUE;
		}
        if(STTUNER_IOREG_GetRegister(DeviceMap_p, IOHandle, R0297J_SYNC_STAT) != 0x1e)
        {
            DataFound = FALSE;
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297J
            STTBX_Print(("%s MPEGTimeOut\n", identity));
#endif
            if (STTUNER_IOREG_GetField(DeviceMap_p, IOHandle, F0297J_COMB_STATE) != 2)
            {
                DataFound = FALSE;
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297J
                STTBX_Print(("%s FrameSyncTimeOut\n", identity));
#endif
                if(    (STTUNER_IOREG_GetField(DeviceMap_p, IOHandle, F0297J_VIT_I_SYNC) == 0)
                    || (STTUNER_IOREG_GetField(DeviceMap_p, IOHandle, F0297J_VIT_Q_SYNC) == 0)
                    )
                {
                    DataFound = FALSE;
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297J
                    STTBX_Print(("%s ViterbiTimeOut\n", identity));
#endif
                }
            }
            return (DataFound);
        }
        DataFound = TRUE;
    }
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297J
    STTBX_Print(("%s DataSearchTime %d\n", identity, DataSearchTime));
#endif

    return DataFound;
}

/*----------------------------------------------------
 FUNCTION      Driv0297JDataSearch
 ACTION
                This routine performs a carrier lock trial with a given offset and sweep rate.
                The returned value is a flag (TRUE, FALSE) which indicates if the trial is
                successful or not. In case of lock, _pSignal->Frequency and _pSignal->SymbolRate
                are modifyed accordingly with the found carrier parameters.
 PARAMS IN     NONE
 PARAMS OUT    NONE
 RETURN        NONE
------------------------------------------------------*/
BOOL Driv0297JDataSearch(STTUNER_IOREG_DeviceMap_t *DeviceMap_p, IOARCH_Handle_t IOHandle,
                        D0297J_StateBlock_t *StateBlock_p, STTUNER_tuner_instance_t *TunerInstance_p)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297J
    const char *identity = "STTUNER drv0297J.c Driv0297JDataSearch()";
    clock_t                 time_spend_register;
    clock_t                 time_start_lock, time_end_lock;
#endif
    long                    InitDemodOffset;
    STTUNER_Modulation_t    Modulation;

    long TimeOut           ;
    long LMS2TimeOut       ;
    int  NbSymbols         ;
    int  Log2QAMSize       ;
    long AcqLoopsTime      ;
    int  SweepRate         ;
    int  CarrierOffset     ;
    int  LMS2TrackingLock  ;
    long SymbolRate        ;
    long Frequency         ;
    STTUNER_Spectrum_t SpectrumInversion ;
    long DataSearchTime    ;
    int  AGC2SD, int_tmp, WBAGCLock, AGCLoop, ACQThreshold;
    int Freq19, Freq23, Freq24;
    
    BOOL EndOfSearch       ;
    BOOL DataFound         ;

    /*
    --- Set Parameters
    */
    DataFound         = FALSE;
    EndOfSearch       = FALSE;
    DataSearchTime    = 0;
    Frequency         = StateBlock_p->Params.Frequency;
    Modulation        = StateBlock_p->QAMSize;
    SymbolRate        = StateBlock_p->Params.SymbolRate;    /* in Baud */
    SweepRate         = StateBlock_p->SweepRate;            /* in 1/ms */
    CarrierOffset     = StateBlock_p->CarrierOffset;        /* in % */
    SpectrumInversion = StateBlock_p->SpectrumInversion;
    InitDemodOffset   = StateBlock_p->InitDemodOffset;
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297J
    STTBX_Print(("%s %u kHz %u Baud Sweep%d\n", identity,
                Frequency,
                SymbolRate,
                SweepRate
                ));
    STTBX_Print(("%s CarrierOffset %d%% InitDemodOffset %d\n", identity,
                CarrierOffset/100,
                InitDemodOffset
                ));
#endif

#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297J
    time_start_lock = STOS_time_now();
#endif

    /*
    ===========================
    TimeOuts computation
    ===========================
    AcqLoopsTime (in mSec ) : it is the time required for the
    lock of the carrier and timing loops
    */
    switch (Modulation)
    {
        case STTUNER_MOD_16QAM :
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297J
            STTBX_Print(("%s QAM16\n", identity));
#endif
            Log2QAMSize = 4   ;
            break;
        case STTUNER_MOD_32QAM :
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297J
    STTBX_Print(("%s QAM32\n", identity));
#endif
            Log2QAMSize = 5   ;
            break;
        case STTUNER_MOD_64QAM :
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297J
            STTBX_Print(("%s QAM64\n", identity));
#endif
            Log2QAMSize = 6   ;
            break;
        case STTUNER_MOD_128QAM :
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297J
            STTBX_Print(("%s QAM128\n", identity));
#endif
            Log2QAMSize = 7   ;
            break;
        case STTUNER_MOD_256QAM :
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297J
            STTBX_Print(("%s QAM256\n", identity));
#endif
            Log2QAMSize = 8   ;
            break;
        default :
            break;	    
    }
    NbSymbols = 100000 ;/* 100000 symbols needed at least to lock the STL */
    AcqLoopsTime = NbSymbols/(SymbolRate/1000) ;/* in ms */
    /*
      SweepRate = 1000*(Sweep Rate/Symbol Rate)
      CarrierOffset = 10000 * (Offset/Symbol Rate)

      then :

       LMS2TimeOut = 100 * (CarrierOffset/SweepRate)

       In order to scan [-Offset, +Offset], we must double LMS2TimeOut.
       In order to be safe, 25% margin is added
    */
    if(SweepRate != 0)
    {
        LMS2TimeOut  = 250L*CarrierOffset;
        LMS2TimeOut /= SweepRate  ; /* in ms */
        if (LMS2TimeOut < 0) LMS2TimeOut = - LMS2TimeOut ;
        /*  Even In Low Carrier Offset Cases, LMS2TimeOut Must Not Be Lower Than A Minimum*/
		/* 	Value Which Is Linked To The QAM Size.*/
        switch (Modulation)
        {
            case STTUNER_MOD_16QAM :
                if( LMS2TimeOut < 50)
                    LMS2TimeOut = 100 ;
                break;
            case STTUNER_MOD_32QAM :
                if( LMS2TimeOut < 50)
                    LMS2TimeOut = 150 ;
                break;
            case STTUNER_MOD_64QAM :
                if( LMS2TimeOut < 50)
                    LMS2TimeOut = 100 ;
                break;
            case STTUNER_MOD_128QAM :
                if( LMS2TimeOut < 100)
                    LMS2TimeOut = 200 ;
                break;
            case STTUNER_MOD_256QAM :
                if( LMS2TimeOut < 100)
                    LMS2TimeOut = 150 ;
                break;
            default :
            	break;    
        }
    }
    else                        /* TimeOut When Estimator Is Used*/
    {
        LMS2TimeOut = 200;  /* Actual TimeOuts Will Be Used In Following Releases*/
    }
    /*
      The equalizer timeout must be greater than
      the carrier and timing lock time
    */
   	LMS2TimeOut += AcqLoopsTime ;

    /*
     $$$$ load register DEBUG
    */
#ifdef STV0297J_RELOAD_DEMOD_REGISTER   /* [ */
    /*  SOFT_RESET */
    STTUNER_IOREG_SetRegister( DeviceMap_p, IOHandle,  R0297J_CTRL_0, 1);
    STTUNER_IOREG_SetRegister( DeviceMap_p, IOHandle,  R0297J_CTRL_0, 0);
    /* Reload Registers */
    STTUNER_IOREG_Reset(&Instance->DeviceMap, Instance->IOHandle, defval, addressarray);
#endif  /* ] STV0297J_RELOAD_DEMOD_REGISTER */

    /* setting of the initial demodulator */
    Driv0297JDemodSetting(DeviceMap_p, IOHandle, StateBlock_p, TunerInstance_p, InitDemodOffset);

    /******************************/
	/*	Delayed AGC Initialization	*/
	/******************************/
    /* Wide Band AGC Loop Freeze, Wide Band AGC Clear And*/
    /* Wide Band AGC Status Forced To Unlock Position*/
    STTUNER_IOREG_SetRegister( DeviceMap_p, IOHandle,  R0297J_WBAGC_3, 0x40);

    /* Wide Band AGC AGC2SD Initialisation To 25% Range*/
    AGC2SD = 256;
    Reg0297J_SetAGC(DeviceMap_p, IOHandle, AGC2SD);
    /* AGC RollOff Set To 384 And Acquisition Threshold To 512*/
    /* In Order To Speed-up The WBAGC Lock. Old values are saved*/
    AGCLoop = Reg0297J_GetWBAGCloop(DeviceMap_p, IOHandle);
    ACQThreshold = STTUNER_IOREG_GetField(DeviceMap_p, IOHandle, F0297J_ACQ_THRESH);
    Reg0297J_SetWBAGCloop(DeviceMap_p, IOHandle, 384);
    STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297J_ACQ_THRESH,9) ;

    /****************************/
    /* STL Initialization       */
    /****************************/

    /* Symbol Timing Loop Reset And Release*/
	/* Both Integral And Direct Paths Are Cleared*/
    STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297J_RESET_STL, 1) ;
    STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297J_RESET_STL, 0) ;
    /* Integral Path Enabled Only After The WBAGC Lock*/
  
     STTUNER_IOREG_SetField(DeviceMap_p, IOHandle,F0297J_ERR_EN,0);
    /* Direct Path Enabled Immediatly */
     STTUNER_IOREG_SetField(DeviceMap_p, IOHandle,F0297J_PHASE_EN,1); 

    /****************************/
    /* CRL Initialization       */
    /****************************/

    /* Carrier Recovery Loop Reset And Release*/
	/* Frequency And Phase Offset Cleared As Well As All Internal Signals*/
    STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297J_RESET_CRL, 1) ;
    STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297J_RESET_CRL, 0) ;

    /* Direct Path Enabled Only After The PMFAGC Lock */
	/* Sweep OFF */
    STTUNER_IOREG_SetRegister(DeviceMap_p, IOHandle, R0297J_CRL_3,0x02);
   
    /* Corner Detector OFF */
    STTUNER_IOREG_SetField(DeviceMap_p,  IOHandle,F0297J_EN_CORNER_DET,0);
	/* PN Loop Bypass */
    STTUNER_IOREG_SetField(DeviceMap_p,  IOHandle,F0297J_SECOND_LOOP_BYPASS,1);
	/* Fading Detector OFF */
    STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297J_ENFADDET,0);
	/* Frequency Estimator Reset */
    STTUNER_IOREG_SetField(DeviceMap_p,  IOHandle,F0297J_RESET_FREQEST,1);
	/* I2C write */
    

	/******************************/
	/*	PMF AGC Initialization		*/
	/******************************/
	/* PMF AGC Status Forced To Unlock Position. Accumulator Is Also Reset */
    STTUNER_IOREG_SetRegister(DeviceMap_p, IOHandle, R0297J_PMFAGC_2,0x08);

    /****************************/
    /* Equalizer Initialization */
    /****************************/

    /* Equalizer Values Capture*/
    Freq19 = STTUNER_IOREG_GetRegister(DeviceMap_p, IOHandle, R0297J_FREQ_19);
    Freq23 = STTUNER_IOREG_GetRegister(DeviceMap_p, IOHandle, R0297J_FREQ_23);
    Freq24 = STTUNER_IOREG_GetRegister(DeviceMap_p, IOHandle, R0297J_FREQ_24);

    /* Equalizer Reset And Release*/
    STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297J_RESET_EQL,1);
    STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297J_RESET_EQL,0);

    /* Equalizer Values Reload*/
    STTUNER_IOREG_SetRegister(DeviceMap_p, IOHandle, R0297J_FREQ_19, Freq19);
    STTUNER_IOREG_SetRegister(DeviceMap_p, IOHandle, R0297J_FREQ_23, Freq23);
    STTUNER_IOREG_SetRegister(DeviceMap_p, IOHandle, R0297J_FREQ_24, Freq24);

    /*****************************/
    /* Signal Parameters Setting */
    /*****************************/

    Reg0297J_SetQAMSize(DeviceMap_p, IOHandle, Modulation);

    /* STL Freeze (Avoiding STV0297J Symbol Rate Update)*/
    STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297J_PHASE_CLR, 1);
    Reg0297J_SetSymbolRate(DeviceMap_p, IOHandle, SymbolRate);
    Reg0297J_SetSpectrumInversion(DeviceMap_p, IOHandle, SpectrumInversion);

    /****************************/
    /* FEC Initialization       */
    /****************************/
    Driv0297JFecInit(DeviceMap_p, IOHandle, StateBlock_p);

    /****************************/
    /* Acquisition Sequence     */
    /****************************/

#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297J
    time_spend_register = STOS_time_minus(STOS_time_now(), time_start_lock);
    time_start_lock = STOS_time_now();
#endif

    /****************************/
    /* AGC Lock Part            */
    /****************************/
    /* WBAGC Loop Enable*/
    STTUNER_IOREG_SetRegister(DeviceMap_p, IOHandle, R0297J_WBAGC_3,0x10);
    /* Wide Band AGC Lock Check*/
    EndOfSearch = FALSE;
    TimeOut = DataSearchTime + 30  ; /* 30 ms time-out */
    while(!EndOfSearch)
    {
        STV0297J_DelayInMilliSec(STV0297J_DEMOD_WAITING_TIME) ;
        DataSearchTime += STV0297J_DEMOD_WAITING_TIME ;
        WBAGCLock = STTUNER_IOREG_GetField(DeviceMap_p, IOHandle, F0297J_WAGC_ACQ) ;
        if((WBAGCLock == 1)||(DataSearchTime > TimeOut))
        {
            EndOfSearch = TRUE;
        }
    }
    if(WBAGCLock == 0)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297J
        time_end_lock = STOS_time_now();
        STTBX_Print(("%s DEMOD CONF REGISTER >>> (%u ticks)(%u ms)\n", identity,
                    time_spend_register,
                    ((time_spend_register)*1000)/ST_GetClocksPerSecond()
                    ));
        STTBX_Print(("%s WBAGC LOCK TIME OUT !!! (%u ticks)(%u ms)\n", identity,
                    STOS_time_minus(time_end_lock, time_start_lock),
                    ((STOS_time_minus(time_end_lock, time_start_lock))*1000)/ST_GetClocksPerSecond()
                    ));
#endif
        StateBlock_p->Result.SignalType = E297J_NOAGC; 
                    /* AGC 1st Step Ko */
        return (DataFound);
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297J
        STTBX_Print(("%s WBAGC LOCKED\n", identity));
#endif
        StateBlock_p->Result.SignalType = E297J_AGCOK;
                    /* AGC 1st Step Ok */
    }

    int_tmp  = (STTUNER_IOREG_GetField(DeviceMap_p, IOHandle, F0297J_AGC2SD_HI)<<8) + STTUNER_IOREG_GetField(DeviceMap_p, IOHandle, F0297J_AGC2SD_LO);

    /* AGC Roll Set To 5000 And Acquisition Threshold To 8192*/
	/* In Order To Switch To Tracking Mode*/
    Reg0297J_SetWBAGCloop(DeviceMap_p, IOHandle, AGCLoop);
    STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297J_ACQ_THRESH, ACQThreshold);

    /****************************/
    /* Post WBAGC Blocks Lock   */
    /****************************/
	/* STL Release*/
    STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297J_PHASE_CLR, 0x00);

    /* Different Settings According To Whether The Estimator Is Used Or Not*/
    if (  (STTUNER_IOREG_GetField(DeviceMap_p, IOHandle, F0297J_FREQESTCORR_BLIND_EN)
        || STTUNER_IOREG_GetField(DeviceMap_p, IOHandle, F0297J_FREQESTCORR_LMS1_EN)
        || STTUNER_IOREG_GetField(DeviceMap_p, IOHandle, F0297J_FREQESTCORR_LMS2_EN))
        != 1
        )
	{
        Reg0297J_SetFrequencyOffset(DeviceMap_p, IOHandle, CarrierOffset);
        Reg0297J_SetSweepRate(DeviceMap_p, IOHandle, SweepRate);
        STTUNER_IOREG_SetRegister(DeviceMap_p, IOHandle, R0297J_CRL_3,0x03);  /* Enable Sweep, Estimator Reset Above */
        STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297J_EN_CORNER_DET,0x01); /* corner detector on */
    }
    else      /* estimator used */
	{
        /* corner detector is not used koz there is a tool replacing it in the frequency estimator */
        Reg0297J_SetFrequencyOffset(DeviceMap_p, IOHandle, 0);/* offset zeroed */
        Reg0297J_SetSweepRate(DeviceMap_p, IOHandle, 0);/* sweep rate zeroed, sweep is disabled above */
        STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297J_RESET_FREQEST,0)          ;/* release of estimator reset */
	}
    /* PMF AGC Unlock Forcing Is Disabled, The CRL Can Act Now*/
    STTUNER_IOREG_SetRegister(DeviceMap_p, IOHandle, R0297J_PMFAGC_2,0x02);

    /* Waiting For A LMS2 Lock Indication Or A Timeout; Several Successive*/
	/* Lock Indications Are Requested To Decide Equalizer Convergence*/
    EndOfSearch    = FALSE;
    TimeOut = DataSearchTime + LMS2TimeOut ;
    LMS2TrackingLock = 0;
    while (!EndOfSearch)
    {
        STV0297J_DelayInMilliSec(STV0297J_DEMOD_WAITING_TIME) ;
        DataSearchTime += STV0297J_DEMOD_WAITING_TIME ;
        int_tmp = STTUNER_IOREG_GetField(DeviceMap_p, IOHandle, F0297J_EQU_LMS2) ;
        if(int_tmp == 1)
			LMS2TrackingLock += 1;
		else
			LMS2TrackingLock = 0;
		if((LMS2TrackingLock >= 5)||(DataSearchTime > TimeOut))
        {
            EndOfSearch = TRUE ;
        }
    }
    if(int_tmp == 0)
    {
        /* the equalizer has not converged */
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297J
        time_end_lock = STOS_time_now();
        STTBX_Print(("%s DEMOD CONF REGISTER >>> (%u ticks)(%u ms)\n", identity,
                    time_spend_register,
                    ((time_spend_register)*1000)/ST_GetClocksPerSecond()
                    ));
        STTBX_Print(("%s equalizer has not converged !!! (%u ticks)(%u ms)\n", identity,
                    STOS_time_minus(time_end_lock, time_start_lock),
                    ((STOS_time_minus(time_end_lock, time_start_lock))*1000)/ST_GetClocksPerSecond()
                    ));
#endif
        StateBlock_p->Result.SignalType = E297J_NOEQUALIZER;
      
        return (DataFound);
    }
    if (  (STTUNER_IOREG_GetField(DeviceMap_p, IOHandle, F0297J_FREQESTCORR_BLIND_EN)
        || STTUNER_IOREG_GetField(DeviceMap_p, IOHandle, F0297J_FREQESTCORR_LMS1_EN)
        || STTUNER_IOREG_GetField(DeviceMap_p, IOHandle, F0297J_FREQESTCORR_LMS2_EN))
        != 1)
	{

        STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297J_SWEEP_EN,0) ;      /* LMS2 was found : sweep is disabled */
        STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297J_EN_CORNER_DET,0x00) ;/* corner detector off */
        /* The following lines may be added by customer if needed */

        /*STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297J_RESET_FREQEST,0); */  /* release of estimator reset */
        /*STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297J_CORRECTIONFLAG_F,3); */ /* force the correction */
        /*STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297J_FREQESTCORR_BLIND_EN,1); */
        /*STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297J_FREQESTCORR_LMS1_EN,1); */
        /* Estimator is now activated */
	}

    STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297J_ENFADDET, 1);  /* Fading Detector Enabled*/
    if (SymbolRate <= 2000000)  /* Phase-Noise Loop Activated For Low Symbol Rate Signals*/
        STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297J_SECOND_LOOP_BYPASS, 0);

    /****************************/
    /* FEC Lock Check           */
    /****************************/

    EndOfSearch    = FALSE ;
	DataFound = Driv0297JFECLockCheck(DeviceMap_p, IOHandle, StateBlock_p, EndOfSearch, (long)TimeOut, (long)DataSearchTime);
    if(!DataFound)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297J
        time_end_lock = STOS_time_now();
        STTBX_Print(("%s DEMOD CONF REGISTER >>> (%u ticks)(%u ms)\n", identity,
                    time_spend_register,
                    ((time_spend_register)*1000)/ST_GetClocksPerSecond()
                    ));
        STTBX_Print(("%s NO DATA !!! (%d ticks)(%u ms)\n", identity,
                    STOS_time_minus(time_end_lock, time_start_lock),
                    ((STOS_time_minus(time_end_lock, time_start_lock))*1000)/ST_GetClocksPerSecond()
                    ));
#endif
        StateBlock_p->Result.SignalType = E297J_NODATA;   
                       /* Analog ??? */
        return (DataFound);
    }
    else
    {
        StateBlock_p->Result.SignalType = E297J_DATAOK; 
                           /* Transport is Present */
    }

    /****************************/
    /* Signal Parameters Update */
    /****************************/

    /* symbol rate update */
    /* carrier frequency update */
    StateBlock_p->Params.Frequency  = StateBlock_p->Result.Frequency;
    StateBlock_p->Params.Frequency  -= InitDemodOffset;
    StateBlock_p->Result.SymbolRate = SymbolRate * 1000;

#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297J
    time_end_lock = STOS_time_now();
    STTBX_Print(("%s DEMOD CONF REGISTER >>> (%u ticks)(%u ms)\n", identity,
                time_spend_register,
                ((time_spend_register)*1000)/ST_GetClocksPerSecond()
                ));
    STTBX_Print(("%s DATA FOUND (%u ticks)(%u ms)\n", identity,
                STOS_time_minus(time_end_lock, time_start_lock),
                ((STOS_time_minus(time_end_lock, time_start_lock))*1000)/ST_GetClocksPerSecond()
                ));
#endif

    return (TRUE);
}

/*----------------------------------------------------
 FUNCTION      Driv0297JCarrierSearch
 ACTION
                This routine is the ENTRY POINT of the STV0297J driver.
                ======================================================

                This routine performs the carrier search following the user indications.
                This routine is the entry point of the STV0297J driver.

                The returned value is a flag (TRUE, FALSE) which indicates if the trial is
                successful or not. In case of lock, _pSignal->Frequency and _pSignal->SymbolRate
                are modifyed accordingly with the found carrier parameters.
 PARAMS IN     NONE
 PARAMS OUT    NONE
 RETURN        NONE
------------------------------------------------------*/
BOOL Driv0297JCarrierSearch(STTUNER_IOREG_DeviceMap_t *DeviceMap_p, IOARCH_Handle_t IOHandle,
                           D0297J_StateBlock_t *StateBlock_p, STTUNER_tuner_instance_t *TunerInstance_p)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297J
    const char *identity = "STTUNER drv0297J.c Driv0297JCarrierSearch()";
    clock_t                 time_start_lock, time_end_lock, time_spend_tuner;
#endif

    ST_ErrorCode_t          Error = ST_NO_ERROR;

    BOOL    DataFound = FALSE;
    int     CarrierOffset;
    int     RangeOffset;
    int     SweepRate;
    int     int_tmp;
    int     InitDemodOffset;
    BOOL    InitDemod;
   
    STTUNER_Spectrum_t    SpectrumInversion;

#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297J
    int     ii=0;
#endif

    /*
    --- Get info from context
    */
    StateBlock_p->Result.SignalType = E297J_NOCARRIER;            /* disable the signal monitoring */
    SpectrumInversion = StateBlock_p->SpectrumInversion;
    DataFound = FALSE;
    InitDemod = FALSE;
    InitDemodOffset = 0;

    /*
    --- Set Quality parameters
    */
    StateBlock_p->Ber[0]        = 0;
    StateBlock_p->Ber[1]        = 50;                       /* Default Ratio for BER */
    StateBlock_p->Ber[2]        = 0;
    StateBlock_p->CNdB          = 0;
    StateBlock_p->SignalQuality = 0;
    StateBlock_p->BlkCounter    = 0;
    StateBlock_p->CorrBlk       = 0;
    StateBlock_p->UncorrBlk     = 0;

    /*
    --- Clear Blk counter
    */
    Reg0297J_StopBlkCounter(DeviceMap_p, IOHandle);

    /*
    --- Test Input Param
    */
    if((( (StateBlock_p->SweepRate) > 0)&&((StateBlock_p->CarrierOffset) > 0))  ||
        (( (StateBlock_p->SweepRate) < 0)&&((StateBlock_p->CarrierOffset) < 0)))
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297J
        STTBX_Print(("%s BAD INPUT PARAMETERS!!!!\n", identity
                    ));
#endif
        return(DataFound) ;
    }
    if((StateBlock_p->SweepRate) < 0)
    {
        SweepRate = -(StateBlock_p->SweepRate) ;
        CarrierOffset = -(StateBlock_p->CarrierOffset) ;
    }
    else
    {
        SweepRate = (StateBlock_p->SweepRate) ;
        CarrierOffset = (StateBlock_p->CarrierOffset) ;
    }

#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297J
    STTBX_Print(("%s %u kHz %u Baud Sweep %d\n", identity,
                StateBlock_p->Params.Frequency,
                StateBlock_p->Params.SymbolRate,
                SweepRate
                ));
    STTBX_Print(("%s CarrierOffset %d%% InitDemodOffset %d\n", identity,
                CarrierOffset/100,
                InitDemodOffset
                ));
#endif

    /*
     tuner setting
    */
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297J
    time_start_lock = STOS_time_now();
#endif
    Error = (TunerInstance_p->Driver->tuner_SetFrequency)(TunerInstance_p->DrvHandle, (U32)(StateBlock_p->Params.Frequency), (U32 *)(&(StateBlock_p->Result.Frequency)));
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297J
    time_spend_tuner = STOS_time_minus(STOS_time_now(), time_start_lock);
#endif
    if( Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297J
        STTBX_Print(("%s FAIL TO SET FREQUENCY (TUNER E297J_NOCARRIER)\n", identity));
#endif
        StateBlock_p->Result.SignalType = E297J_NOCARRIER;            /* Tuner is locked */
        return(DataFound) ;
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297J
        STTBX_Print(("%s TUNER IS LOCKED at %d KHz (Request %d KHz) (TUNER E297J_CARRIEROK)\n", identity,
            StateBlock_p->Result.Frequency,
            StateBlock_p->Params.Frequency
            ));
#endif
        StateBlock_p->Result.SignalType = E297J_CARRIEROK;            /* Tuner is locked */
     
    }

    /*
    ---
    */
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297J
    time_start_lock = STOS_time_now();
#endif

    RangeOffset = CarrierOffset ;

    /*
    init demod enabled ?
    */
    int_tmp = STTUNER_IOREG_GetField(DeviceMap_p, IOHandle, F0297J_IN_DEMOD_EN);
    if ((int_tmp == 1)&&(RangeOffset <= -400))
    {
        InitDemod = TRUE ;
        RangeOffset = -400 ;
    }
    else
    {
        InitDemod = FALSE ;
    }

    if (StateBlock_p->ScanMode == DEF0297J_SCAN)
    {
        InitDemod = FALSE ;
    }

    /*
      [- RangeOffset, + RangeOffset] range
    */
    InitDemodOffset = 0;
    StateBlock_p->InitDemodOffset = InitDemodOffset;
    StateBlock_p->CarrierOffset = (110*RangeOffset)/100;
    StateBlock_p->SweepRate = SweepRate;
    StateBlock_p->SpectrumInversion = SpectrumInversion;
    DataFound = Driv0297JDataSearch(DeviceMap_p, IOHandle, StateBlock_p, TunerInstance_p);

    if(InitDemod)
    {
        /* initial demodulation is required */
        InitDemodOffset = 0 ;
        while ((!DataFound)&&((InitDemodOffset + RangeOffset) >= CarrierOffset))
        {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297J
            STTBX_Print(("%s TUNER loop # %d InitDemodOffset %d RangeOffset %d CarrierOffset %d %%\n", identity, ii++,
                InitDemodOffset,
                RangeOffset,
                CarrierOffset/100
                ));
#endif
            /* the subranges are scanned in a zig-zag mode around the initial subrange */
            SweepRate = -SweepRate ;
            RangeOffset = -RangeOffset ;
            if(RangeOffset > 0)
            {
                InitDemodOffset = -InitDemodOffset + RangeOffset ;
            }
            else
            {
                InitDemodOffset = -InitDemodOffset ;
            }
            /*
              [InitDemod - RangeOffset, InitDemod + RangeOffset] range
            */
            StateBlock_p->InitDemodOffset = (InitDemodOffset*((StateBlock_p->Params.SymbolRate)/1000))/10000;
            StateBlock_p->CarrierOffset = (110*RangeOffset)/100;
            StateBlock_p->SweepRate = SweepRate;
            StateBlock_p->SpectrumInversion = SpectrumInversion;
            DataFound = Driv0297JDataSearch(DeviceMap_p, IOHandle, StateBlock_p, TunerInstance_p);
        }
    }

    /*
    --- Perfmeter
    */
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297J
    time_end_lock = STOS_time_now();
    STTBX_Print(("%s TIME >> TUNER (%u ticks)(%u ms)\n", identity,
                time_spend_tuner,
                ((time_spend_tuner)*1000)/ST_GetClocksPerSecond()
                ));
    STTBX_Print(("%s TIME >> DEMOD (%u ticks)(%u ms)\n", identity,
                STOS_time_minus(time_end_lock, time_start_lock),
                ((STOS_time_minus(time_end_lock, time_start_lock))*1000)/ST_GetClocksPerSecond()
                ));
#endif

    /*
    --- Update Status
    */
    if(DataFound)
    {
        StateBlock_p->Result.SignalType = E297J_LOCKOK;
        
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297J
        STTBX_Print(("%s ==> E297J_LOCKOK\n", identity));
#endif
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297J
        STTBX_Print(("%s ==> E297J_xxxxxx (%d)!!!!!\n", identity, StateBlock_p->Result.SignalType));
#endif
    }

    /*
    --- Reset Err Counters
    */
   
  
     STTUNER_IOREG_SetField(DeviceMap_p,IOHandle, F0297J_ERRCOUNT_HI, 0);
     STTUNER_IOREG_SetField(DeviceMap_p,IOHandle, F0297J_ERRCOUNT_LO, 0);

    /*
    --- Start Blk counter
    */
    Reg0297J_StartBlkCounter(DeviceMap_p, IOHandle);

    /*
    ---
    */

    return (DataFound) ;

}


/* End of drv0297J.c */
