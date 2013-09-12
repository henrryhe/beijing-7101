/*----------------------------------------------------------------------------
File Name   : drv0297.c (was drv0299.c)

Description : STV0297 front-end driver routines.

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

#include "reg0297.h"    /* register mappings for the stv0297 */
#include "d0297.h"      /* top level header for this driver */

#include "drv0297.h"    /* header for this file */


/* defines ----------------------------------------------------------------- */

#define STV0297_DEMOD_WAITING_TIME          5   /* In ms */
#define STV0297_DEMOD_TRACKING_LOCK         5
#define STV0297_CARRIER_SEARCH_A
#define STV0297_DATA_SEARCH_A
#define STV0297_RELOAD_DEMOD_REGISTER

#define STV0297_FAST_SWEEP_SPEED            750
#define STV0297_SLOW_SWEEP_SPEED            400

#define STV0297_SYMBOLRATE_LEVEL            3000000 /* When SR < STV0297_SYMBOLRATE_LEVEL, Sweep = Sweep/2 */

/* private types ----------------------------------------------------------- */


/* constants --------------------------------------------------------------- */

/* Current LLA revision */
static ST_Revision_t Revision297  = " STV0297-LLA_REL_32.02(GUI) ";

extern U16 Address[];
extern U8 DefVal[];




/* Common initialization for all tuners */
/* ==================================== */

const U8 QAM16[] =
    {
    R0297_EQU_0,           0x08,
    R0297_EQU_1,           0x58,
    R0297_CTRL_8,          0x08,
    R0297_DEINT_SYNC_0,    0x00,
    R0297_BERT_0,          0x84,
    0x00
    };

const U8 QAM32[] =
    {
    R0297_EQU_0,           0x18,
    R0297_EQU_1,           0x58,
    R0297_CTRL_8,          0x00,
    R0297_DEINT_SYNC_0,    0x02,
    R0297_BERT_0,          0x85,
    0x00
    };

const U8 QAM64[] =
    {
    R0297_EQU_0,           0x48,
    R0297_EQU_1,           0x58,
    R0297_CTRL_8,          0x00,
    R0297_DEINT_SYNC_0,    0x02,
    R0297_BERT_0,          0x85,
    0x00
    };

const U8 QAM128[] =
    {
    R0297_EQU_0,           0x29,
    R0297_EQU_1,           0x89,
    R0297_CTRL_8,          0x00,
    R0297_DEINT_SYNC_0,    0x00,
    R0297_BERT_0,          0x84,
    0x00
    };

const U8 QAM256[] =
    {
    R0297_EQU_0,           0x39,
    R0297_EQU_1,           0x69,
    R0297_CTRL_8,          0x08,
    R0297_DEINT_SYNC_0,    0x00,
    R0297_BERT_0,          0x85,
    0x00
    };

/* Initialization of TDBE1 */
/* ======================= */

const U8 QAM16_TDBE1[] =
    {
    R0297_DELAGC_0,        0xFF,
    R0297_DELAGC_1,        0x33,
    R0297_DELAGC_2,        0xF4,
    R0297_DELAGC_3,        0x44,
    R0297_DELAGC_4,        0x29,
    R0297_DELAGC_5,        0x33,
    R0297_DELAGC_6,        0x80,
    R0297_DELAGC_7,        0x6F,
    R0297_DELAGC_8,        0xDC,
    R0297_WBAGC_1,         0xE5,
    R0297_WBAGC_2,         0x2F,
    R0297_WBAGC_3,         0x00,
    R0297_WBAGC_4,         0xC4,
    R0297_WBAGC_5,         0x00,
    R0297_WBAGC_6,         0x00,
    R0297_WBAGC_9,         0x09,
    R0297_WBAGC_10,        0x66,
    R0297_WBAGC_11,        0xE6,
    R0297_STLOOP_2,        0x30,
    R0297_STLOOP_3,        0x06,
    R0297_STLOOP_9,        0x08,
    R0297_STLOOP_10,       0x5E,
    R0297_STLOOP_11,       0x04,
    R0297_CRL_1,           0x49,
    R0297_CRL_2,           0x0B,
    R0297_CRL_9,           0x0F,
    R0297_CRL_10,          0x03,
    R0297_PMFAGC_0,        0xFF,
    R0297_PMFAGC_1,        0x04,
    R0297_PMFAGC_2,        0x00,
    R0297_PMFAGC_3,        0x00,
    R0297_PMFAGC_4,        0x0C,
    0x00
    };

const U8 QAM32_TDBE1[] =
    {
    R0297_DELAGC_0,         0xFF,
    R0297_DELAGC_1,         0x33,
    R0297_DELAGC_2,         0xEB,
    R0297_DELAGC_3,         0x44,
    R0297_DELAGC_4,         0x29,
    R0297_DELAGC_5,         0x33,
    R0297_DELAGC_6,         0x80,
    R0297_DELAGC_7,         0x6C,
    R0297_DELAGC_8,         0x33,
    R0297_WBAGC_1,          0x4A,
    R0297_WBAGC_2,          0x2D,
    R0297_WBAGC_3,          0x00,
    R0297_WBAGC_4,          0xC4,
    R0297_WBAGC_5,          0x00,
    R0297_WBAGC_6,          0x00,
    R0297_WBAGC_9,          0x09,
    R0297_WBAGC_10,         0x0A,
    R0297_WBAGC_11,         0xF7,
    R0297_STLOOP_2,         0x30,
    R0297_STLOOP_3,         0x08,
    R0297_STLOOP_9,         0x08,
    R0297_STLOOP_10,        0x1E,
    R0297_STLOOP_11,        0x04,
    R0297_CRL_1,            0x49,
    R0297_CRL_2,            0x05,
    R0297_CRL_9,            0x00,
    R0297_CRL_10,           0x03,
    R0297_PMFAGC_0,         0xFF,
    R0297_PMFAGC_1,         0x84,
    R0297_PMFAGC_2,         0x00,
    R0297_PMFAGC_3,         0x00,
    R0297_PMFAGC_4,         0x0C,
    0x00
    };

const U8 QAM64_TDBE1[] =
    {
    R0297_DELAGC_0,        0xFF,
    R0297_DELAGC_1,        0x33,
    R0297_DELAGC_2,        0xE5,
    R0297_DELAGC_3,        0x44,
    R0297_DELAGC_4,        0x29,
    R0297_DELAGC_5,        0x33,
    R0297_DELAGC_6,        0x80,
    R0297_DELAGC_7,        0x6C,
    R0297_DELAGC_8,        0x6E,
    R0297_WBAGC_1,         0x9F,
    R0297_WBAGC_2,         0x20,
    R0297_WBAGC_3,         0x18,
    R0297_WBAGC_4,         0x80,
    R0297_WBAGC_5,         0x00,
    R0297_WBAGC_6,         0x00,
    R0297_WBAGC_9,         0x12,
    R0297_WBAGC_10,        0x51,
    R0297_WBAGC_11,        0xF8,
    R0297_STLOOP_2,        0x30,
    R0297_STLOOP_3,        0x06,
    R0297_STLOOP_9,        0x08,
    R0297_STLOOP_10,       0x1E,
    R0297_STLOOP_11,       0x04,
    R0297_CRL_1,           0x49,
    R0297_CRL_2,           0x05,
    R0297_CRL_9,           0x00,
    R0297_CRL_10,          0x03,
    R0297_PMFAGC_0,        0xFF,
    R0297_PMFAGC_1,        0x04,
    R0297_PMFAGC_2,        0x00,
    R0297_PMFAGC_3,        0x00,
    R0297_PMFAGC_4,        0x00,
    0x00
    };

const U8 QAM128_TDBE1[] =
    {
    R0297_DELAGC_0,        0xFF,
    R0297_DELAGC_1,        0x33,
    R0297_DELAGC_2,        0xE5,
    R0297_DELAGC_3,        0x44,
    R0297_DELAGC_4,        0x29,
    R0297_DELAGC_5,        0x33,
    R0297_DELAGC_6,        0x80,
    R0297_DELAGC_7,        0x60,
    R0297_DELAGC_8,        0x00,
    R0297_WBAGC_1,         0xED,
    R0297_WBAGC_2,         0x2F,
    R0297_WBAGC_3,         0x00,
    R0297_WBAGC_4,         0xC4,
    R0297_WBAGC_5,         0x00,
    R0297_WBAGC_6,         0x00,
    R0297_WBAGC_9,         0x09,
    R0297_WBAGC_10,        0x14,
    R0297_WBAGC_11,        0xFE,
    R0297_STLOOP_2,        0x30,
    R0297_STLOOP_3,        0x06,
    R0297_STLOOP_9,        0x08,
    R0297_STLOOP_10,       0x5E,
    R0297_STLOOP_11,       0x04,
    R0297_CRL_1,           0x49,
    R0297_CRL_2,           0x0B,
    R0297_CRL_9,           0x0F,
    R0297_CRL_10,          0x03,
    R0297_PMFAGC_0,        0xFF,
    R0297_PMFAGC_1,        0x04,
    R0297_PMFAGC_2,        0x00,
    R0297_PMFAGC_3,        0x00,
    R0297_PMFAGC_4,        0x0C,
    0x00
    };

const U8 QAM256_TDBE1[] =
    {
    R0297_DELAGC_0,        0xFF,
    R0297_DELAGC_1,        0x33,
    R0297_DELAGC_2,        0xE5,
    R0297_DELAGC_3,        0x44,
    R0297_DELAGC_4,        0x29,
    R0297_DELAGC_5,        0x33,
    R0297_DELAGC_6,        0x80,
    R0297_DELAGC_7,        0x6C,
    R0297_DELAGC_8,        0x42,
    R0297_WBAGC_1,         0xFF,
    R0297_WBAGC_2,         0x2F,
    R0297_WBAGC_3,         0x00,
    R0297_WBAGC_4,         0xC4,
    R0297_WBAGC_5,         0x00,
    R0297_WBAGC_6,         0x00,
    R0297_WBAGC_9,         0x09,
    R0297_WBAGC_10,        0x76,
    R0297_WBAGC_11,        0xFE,
    R0297_STLOOP_2,        0x30,
    R0297_STLOOP_3,        0x06,
    R0297_STLOOP_9,        0x08,
    R0297_STLOOP_10,       0x5E,
    R0297_STLOOP_11,       0x04,
    R0297_CRL_1,           0x49,
    R0297_CRL_2,           0x0B,
    R0297_CRL_9,           0x0F,
    R0297_CRL_10,          0x03,
    R0297_PMFAGC_0,        0xFF,
    R0297_PMFAGC_1,        0x04,
    R0297_PMFAGC_2,        0x00,
    R0297_PMFAGC_3,        0x00,
    R0297_PMFAGC_4,        0x00,
    0x00
    };

/* Initialization of MT2040 */
/* ======================== */

const U8 QAM16_MT2040[] =
    {
    R0297_DELAGC_0,        0xF9,
    R0297_DELAGC_1,        0x6B,
    R0297_DELAGC_2,        0xA6,
    R0297_DELAGC_3,        0x17,
    R0297_DELAGC_4,        0x29,
    R0297_DELAGC_5,        0x6C,
    R0297_DELAGC_6,        0x80,
    R0297_DELAGC_7,        0x64,
    R0297_DELAGC_8,        0xB8,
    R0297_WBAGC_1,         0xE4,
    R0297_WBAGC_2,         0x3B,
    R0297_WBAGC_3,         0x00,
    R0297_WBAGC_4,         0x10,
    R0297_WBAGC_5,         0x00,
    R0297_WBAGC_6,         0x00,
    R0297_WBAGC_9,         0x27,
    R0297_WBAGC_10,        0x66,
    R0297_WBAGC_11,        0xE6,
    R0297_STLOOP_2,        0x30,
    R0297_STLOOP_3,        0x06,
    R0297_STLOOP_9,        0x08,
    R0297_STLOOP_10,       0x5E,
    R0297_STLOOP_11,       0x04,
    R0297_CRL_1,           0x49,
    R0297_CRL_2,           0x0B,
    R0297_CRL_9,           0x05,
    R0297_CRL_10,          0x03,
    R0297_PMFAGC_0,        0xFF,
    R0297_PMFAGC_1,        0x04,
    R0297_PMFAGC_2,        0x00,
    R0297_PMFAGC_3,        0x00,
    R0297_PMFAGC_4,        0x0C,
    0x00
    };

const U8 QAM32_MT2040[] =
    {
    R0297_DELAGC_0,        0xF9,
    R0297_DELAGC_1,        0x6B,
    R0297_DELAGC_2,        0xA6,
    R0297_DELAGC_3,        0x17,
    R0297_DELAGC_4,        0x29,
    R0297_DELAGC_5,        0x6C,
    R0297_DELAGC_6,        0x80,
    R0297_DELAGC_7,        0x6D,
    R0297_DELAGC_8,        0x6D,
    R0297_WBAGC_1,         0xF6,
    R0297_WBAGC_2,         0x3B,
    R0297_WBAGC_3,         0x00,
    R0297_WBAGC_4,         0x10,
    R0297_WBAGC_5,         0x00,
    R0297_WBAGC_6,         0x00,
    R0297_WBAGC_9,         0x27,
    R0297_WBAGC_10,        0xFF,
    R0297_WBAGC_11,        0xFF,
    R0297_STLOOP_2,        0x30,
    R0297_STLOOP_3,        0x08,
    R0297_STLOOP_9,        0x08,
    R0297_STLOOP_10,       0x5E,
    R0297_STLOOP_11,       0x04,
    R0297_CRL_1,           0x49,
    R0297_CRL_2,           0x05,
    R0297_CRL_9,           0x0F,
    R0297_CRL_10,          0x03,
    R0297_PMFAGC_0,        0xFF,
    R0297_PMFAGC_1,        0x04,
    R0297_PMFAGC_2,        0x00,
    R0297_PMFAGC_3,        0x00,
    R0297_PMFAGC_4,        0x0C,
    0x00
    };

const U8 QAM64_MT2040[] =
    {
    R0297_DELAGC_0,        0xF9,
    R0297_DELAGC_1,        0x6B,
    R0297_DELAGC_2,        0xA6,
    R0297_DELAGC_3,        0x17,
    R0297_DELAGC_4,        0x29,
    R0297_DELAGC_5,        0x6B,
    R0297_DELAGC_6,        0x80,
    R0297_DELAGC_7,        0x66,
    R0297_DELAGC_8,        0x9B,
    R0297_WBAGC_1,         0xEA,
    R0297_WBAGC_2,         0x33,
    R0297_WBAGC_3,         0x00,
    R0297_WBAGC_4,         0xFF,
    R0297_WBAGC_5,         0x00,
    R0297_WBAGC_6,         0x00,
    R0297_WBAGC_9,         0x04,
    R0297_WBAGC_10,        0x51,
    R0297_WBAGC_11,        0xF8,
    R0297_STLOOP_2,        0x30,
    R0297_STLOOP_3,        0x06,
    R0297_STLOOP_9,        0x08,
    R0297_STLOOP_10,       0x5E,
    R0297_STLOOP_11,       0x04,
    R0297_CRL_1,           0x49,
    R0297_CRL_2,           0x0B,
    R0297_CRL_9,           0x0B,
    R0297_CRL_10,          0x03,
    R0297_PMFAGC_0,        0xFF,
    R0297_PMFAGC_1,        0x04,
    R0297_PMFAGC_2,        0x00,
    R0297_PMFAGC_3,        0x00,
    R0297_PMFAGC_4,        0x0C,
    0x00
    };

const U8 QAM128_MT2040[] =
    {
    R0297_DELAGC_0,        0xF9,
    R0297_DELAGC_1,        0x6B,
    R0297_DELAGC_2,        0xA6,
    R0297_DELAGC_3,        0x17,
    R0297_DELAGC_4,        0x29,
    R0297_DELAGC_5,        0x6B,
    R0297_DELAGC_6,        0x80,
    R0297_DELAGC_7,        0x60,
    R0297_DELAGC_8,        0x9E,
    R0297_WBAGC_1,         0xFC,
    R0297_WBAGC_2,         0x33,
    R0297_WBAGC_3,         0x00,
    R0297_WBAGC_4,         0xFF,
    R0297_WBAGC_5,         0x00,
    R0297_WBAGC_6,         0x00,
    R0297_WBAGC_9,         0x04,
    R0297_WBAGC_10,        0x14,
    R0297_WBAGC_11,        0xFE,
    R0297_STLOOP_2,        0x30,
    R0297_STLOOP_3,        0x06,
    R0297_STLOOP_9,        0x08,
    R0297_STLOOP_10,       0x5E,
    R0297_STLOOP_11,       0x04,
    R0297_CRL_1,           0x49,
    R0297_CRL_2,           0x0B,
    R0297_CRL_9,           0x00,
    R0297_CRL_10,          0x03,
    R0297_PMFAGC_0,        0xFF,
    R0297_PMFAGC_1,        0x04,
    R0297_PMFAGC_2,        0x00,
    R0297_PMFAGC_3,        0x00,
    R0297_PMFAGC_4,        0x0C,
    0x00
    };

const U8 QAM256_MT2040[] =
    {
    R0297_DELAGC_0,        0xF9,
    R0297_DELAGC_1,        0x6B,
    R0297_DELAGC_2,        0xA6,
    R0297_DELAGC_3,        0x17,
    R0297_DELAGC_4,        0x29,
    R0297_DELAGC_5,        0x6C,
    R0297_DELAGC_6,        0x80,
    R0297_DELAGC_7,        0x70,
    R0297_DELAGC_8,        0x00,
    R0297_WBAGC_1,         0x58,
    R0297_WBAGC_2,         0x31,
    R0297_WBAGC_3,         0x18,
    R0297_WBAGC_4,         0xFF,
    R0297_WBAGC_5,         0xB0,
    R0297_WBAGC_6,         0x40,
    R0297_WBAGC_9,         0x04,
    R0297_WBAGC_10,        0x76,
    R0297_WBAGC_11,        0xFE,
    R0297_STLOOP_2,        0x4C,
    R0297_STLOOP_3,        0x06,
    R0297_STLOOP_9,        0x08,
    R0297_STLOOP_10,       0x1E,
    R0297_STLOOP_11,       0x04,
    R0297_CRL_1,           0x49,
    R0297_CRL_2,           0x0A,
    R0297_CRL_9,           0x00,
    R0297_CRL_10,          0x03,
    R0297_PMFAGC_0,        0xFF,
    R0297_PMFAGC_1,        0x04,
    R0297_PMFAGC_2,        0x00,
    R0297_PMFAGC_3,        0x00,
    R0297_PMFAGC_4,        0x00,
    0x00
    };

/* Initialization of MT2050 */
/* ======================== */

const U8 QAM16_MT2050[] =
    {
    R0297_DELAGC_0,        0xff,
    R0297_DELAGC_1,        0x00,
    R0297_DELAGC_2,        0xff,
    R0297_DELAGC_3,        0x33,
    R0297_DELAGC_4,        0x32,
    R0297_DELAGC_5,        0x7f,
    R0297_DELAGC_6,        0x80,
    R0297_DELAGC_7,        0x60,
    R0297_DELAGC_8,        0x00,
    R0297_WBAGC_0,         0x1a,
    R0297_WBAGC_1,         0x51,
    R0297_WBAGC_2,         0x22,
    R0297_WBAGC_3,         0x08,
    R0297_WBAGC_4,         0x80,
    R0297_WBAGC_5,         0x00,
    R0297_WBAGC_6,         0x00,
    R0297_WBAGC_9,         0x12,
    R0297_WBAGC_10,        0xf5,
    R0297_WBAGC_11,        0xe8,
    R0297_STLOOP_2,        0x30,
    R0297_STLOOP_3,        0x06,
    R0297_STLOOP_9,        0x06,
    R0297_STLOOP_10,       0x1e,
    R0297_STLOOP_11,       0x04,
    R0297_CRL_0,           0x27,
    R0297_CRL_1,           0x38,
    R0297_CRL_2,           0x0a,
    R0297_CRL_6,           0xd6,
    R0297_CRL_7,           0x4e,
    R0297_CRL_8,           0xc6,
    R0297_CRL_9,           0x02,
    R0297_CRL_10,          0x02,
    R0297_CRL_11,          0x01,
    R0297_PMFAGC_0,        0xff,
    R0297_PMFAGC_1,        0x04,
    R0297_CTRL_5,          0x80,
    R0297_CTRL_6,          0x88,
    R0297_CTRL_8,          0x00,
    0x00
    };

const U8 QAM32_MT2050[] =
    {
    R0297_DELAGC_0,        0xff,
    R0297_DELAGC_1,        0x00,
    R0297_DELAGC_2,        0xff,
    R0297_DELAGC_3,        0x33,
    R0297_DELAGC_4,        0x32,
    R0297_DELAGC_5,        0x7f,
    R0297_DELAGC_6,        0x80,
    R0297_DELAGC_7,        0x60,
    R0297_DELAGC_8,        0x00,
    R0297_WBAGC_0,         0x19,
    R0297_WBAGC_1,         0x12,
    R0297_WBAGC_2,         0x21,
    R0297_WBAGC_3,         0x18,
    R0297_WBAGC_4,         0x80,
    R0297_WBAGC_5,         0x00,
    R0297_WBAGC_6,         0x00,
    R0297_WBAGC_9,         0x12,
    R0297_WBAGC_10,        0xc2,
    R0297_WBAGC_11,        0xf5,
    R0297_STLOOP_2,        0x30,
    R0297_STLOOP_3,        0x08,
    R0297_STLOOP_9,        0x08,
    R0297_STLOOP_10,       0x1e,
    R0297_STLOOP_11,       0x04,
    R0297_CRL_0,           0x11,
    R0297_CRL_1,           0x39,
    R0297_CRL_2,           0x06,
    R0297_CRL_6,           0x74,
    R0297_CRL_7,           0x23,
    R0297_CRL_8,           0x06,
    R0297_CRL_9,           0x00,
    R0297_CRL_10,          0x02,
    R0297_CRL_11,          0x01,
    R0297_PMFAGC_0,        0xff,
    R0297_PMFAGC_1,        0x04,
    R0297_CTRL_5,          0x80,
    R0297_CTRL_6,          0x88,
    R0297_CTRL_8,          0x00,
    0x00
    };

const U8 QAM64_MT2050[] =
    {
    R0297_DELAGC_0,        0xff,
    R0297_DELAGC_1,        0x33,
    R0297_DELAGC_2,        0xcc,
    R0297_DELAGC_3,        0x33,
    R0297_DELAGC_4,        0x31,
    R0297_DELAGC_5,        0x7f,
    R0297_DELAGC_6,        0x80,
    R0297_DELAGC_7,        0x60,
    R0297_DELAGC_8,        0x00,
    R0297_WBAGC_0,         0x1a,
    R0297_WBAGC_1,         0x74,
    R0297_WBAGC_2,         0x21,
    R0297_WBAGC_3,         0x18,
    R0297_WBAGC_4,         0x80,
    R0297_WBAGC_5,         0x00,
    R0297_WBAGC_6,         0x00,
    R0297_WBAGC_9,         0x12,
    R0297_WBAGC_10,        0x51,
    R0297_WBAGC_11,        0xf8,
    R0297_STLOOP_2,        0x30,
    R0297_STLOOP_3,        0x06,
    R0297_STLOOP_9,        0x08,
    R0297_STLOOP_10,       0x1e,
    R0297_STLOOP_11,       0x04,
    R0297_CRL_0,           0x1c,
    R0297_CRL_1,           0x49,
    R0297_CRL_2,           0x0b,
    R0297_CRL_6,           0xdc,
    R0297_CRL_7,           0x0e,
    R0297_CRL_8,           0x06,
    R0297_CRL_9,           0x00,
    R0297_CRL_10,          0x02,
    R0297_CRL_11,          0x01,
    R0297_PMFAGC_0,        0xff,
    R0297_PMFAGC_1,        0x04,
    R0297_CTRL_5,          0x80,
    R0297_CTRL_6,          0x80,
    R0297_CTRL_8,          0x00,
    0x00
    };

const U8 QAM128_MT2050[] =
    {
    R0297_DELAGC_0,        0xff,
    R0297_DELAGC_1,        0x00,
    R0297_DELAGC_2,        0xff,
    R0297_DELAGC_3,        0x33,
    R0297_DELAGC_4,        0x32,
    R0297_DELAGC_5,        0x7f,
    R0297_DELAGC_6,        0x80,
    R0297_DELAGC_7,        0x6e,
    R0297_DELAGC_8,        0x08,
    R0297_WBAGC_0,         0x1a,
    R0297_WBAGC_1,         0x15,
    R0297_WBAGC_2,         0x21,
    R0297_WBAGC_3,         0x18,
    R0297_WBAGC_4,         0x80,
    R0297_WBAGC_5,         0x00,
    R0297_WBAGC_6,         0x00,
    R0297_WBAGC_9,         0x12,
    R0297_WBAGC_10,        0x70,
    R0297_WBAGC_11,        0xfd,
    R0297_STLOOP_2,        0x30,
    R0297_STLOOP_3,        0x08,
    R0297_STLOOP_9,        0x08,
    R0297_STLOOP_10,       0x1e,
    R0297_STLOOP_11,       0x04,
    R0297_CRL_0,           0x13,
    R0297_CRL_1,           0x3a,
    R0297_CRL_2,           0x06,
    R0297_CRL_6,           0xea,
    R0297_CRL_7,           0xf0,
    R0297_CRL_8,           0xe0,
    R0297_CRL_9,           0x09,
    R0297_CRL_10,          0x02,
    R0297_CRL_11,          0x01,
    R0297_PMFAGC_0,        0xff,
    R0297_PMFAGC_1,        0x04,
    R0297_CTRL_5,          0x80,
    R0297_CTRL_6,          0x88,
    R0297_CTRL_8,          0x00,
    0x00
    };

const U8 QAM256_MT2050[] =
    {
    R0297_DELAGC_0,        0xff,
    R0297_DELAGC_1,        0x33,
    R0297_DELAGC_2,        0xcc,
    R0297_DELAGC_3,        0x33,
    R0297_DELAGC_4,        0x31,
    R0297_DELAGC_5,        0x7f,
    R0297_DELAGC_6,        0x80,
    R0297_DELAGC_7,        0x70,
    R0297_DELAGC_8,        0x00,
    R0297_WBAGC_0,         0x1a,
    R0297_WBAGC_1,         0x96,
    R0297_WBAGC_2,         0x21,
    R0297_WBAGC_3,         0x18,
    R0297_WBAGC_4,         0x80,
    R0297_WBAGC_5,         0xb0,
    R0297_WBAGC_6,         0x40,
    R0297_WBAGC_9,         0x12,
    R0297_WBAGC_10,        0xff,
    R0297_WBAGC_11,        0xff,
    R0297_STLOOP_2,        0x4c,
    R0297_STLOOP_3,        0x06,
    R0297_STLOOP_9,        0x06,
    R0297_STLOOP_10,       0x1e,
    R0297_STLOOP_11,       0x04,
    R0297_CRL_0,           0x33,
    R0297_CRL_1,           0x49,
    R0297_CRL_2,           0x0a,
    R0297_CRL_6,           0x84,
    R0297_CRL_7,           0xd4,
    R0297_CRL_8,           0xdf,
    R0297_CRL_9,           0x0f,
    R0297_CRL_10,          0x02,
    R0297_CRL_11,          0xb5,
    R0297_PMFAGC_0,        0xff,
    R0297_PMFAGC_1,        0x04,
    R0297_CTRL_5,          0x80,
    R0297_CTRL_6,          0x84,
    R0297_CTRL_8,          0x00,
    0x00
    };

/* Initialization of MT2060 */
/* ======================== */

const U8 QAM16_MT2060[] =
    {
    R0297_DELAGC_0,        0xff,
    R0297_DELAGC_1,        0x33,
    R0297_DELAGC_2,        0xcc,
    R0297_DELAGC_3,        0x33,
    R0297_DELAGC_4,        0x31,
    R0297_DELAGC_5,        0x7f,
    R0297_DELAGC_6,        0x80,
    R0297_DELAGC_7,        0x60,
    R0297_DELAGC_8,        0x00,
    R0297_WBAGC_0,         0x1a,
    R0297_WBAGC_1,         0xff,
    R0297_WBAGC_2,         0x23,
    R0297_WBAGC_3,         0x18,
    R0297_WBAGC_4,         0x80,
    R0297_WBAGC_5,         0x00,
    R0297_WBAGC_6,         0x00,
    R0297_WBAGC_9,         0x12,
    R0297_WBAGC_10,        0xf5,
    R0297_WBAGC_11,        0xe8,
    R0297_STLOOP_2,        0x30,
    R0297_STLOOP_3,        0x07,
    R0297_STLOOP_9,        0x07,
    R0297_STLOOP_10,       0x1e,
    R0297_STLOOP_11,       0x04,
    R0297_CRL_0,           0x27,
    R0297_CRL_1,           0x39,
    R0297_CRL_2,           0x0b,
    R0297_CRL_6,           0x5a,
    R0297_CRL_7,           0xcb,
    R0297_CRL_8,           0xc5,
    R0297_CRL_9,           0x0c,
    R0297_CRL_10,          0x02,
    R0297_CRL_11,          0x00,
    R0297_PMFAGC_0,        0xff,
    R0297_PMFAGC_1,        0x04,
    R0297_CTRL_5,          0x80,
    R0297_CTRL_6,          0xc0,
    R0297_CTRL_8,          0x08,
    0x00
    };

const U8 QAM32_MT2060[] =
    {
    R0297_DELAGC_0,        0xff,
    R0297_DELAGC_1,        0x33,
    R0297_DELAGC_2,        0xcc,
    R0297_DELAGC_3,        0x33,
    R0297_DELAGC_4,        0x31,
    R0297_DELAGC_5,        0x7f,
    R0297_DELAGC_6,        0x80,
    R0297_DELAGC_7,        0x65,
    R0297_DELAGC_8,        0x15,
    R0297_WBAGC_0,         0x1a,
    R0297_WBAGC_1,         0xff,
    R0297_WBAGC_2,         0x23,
    R0297_WBAGC_3,         0x18,
    R0297_WBAGC_4,         0x80,
    R0297_WBAGC_5,         0x00,
    R0297_WBAGC_6,         0x00,
    R0297_WBAGC_9,         0x12,
    R0297_WBAGC_10,        0xc2,
    R0297_WBAGC_11,        0xf5,
    R0297_STLOOP_2,        0x30,
    R0297_STLOOP_3,        0x06,
    R0297_STLOOP_9,        0x08,
    R0297_STLOOP_10,       0x1e,
    R0297_STLOOP_11,       0x04,
    R0297_CRL_0,           0x27,
    R0297_CRL_1,           0x39,
    R0297_CRL_2,           0x0b,
    R0297_CRL_6,           0x5a,
    R0297_CRL_7,           0xd3,
    R0297_CRL_8,           0x49,
    R0297_CRL_9,           0x0f,
    R0297_CRL_10,          0x02,
    R0297_CRL_11,          0x00,
    R0297_PMFAGC_0,        0xff,
    R0297_PMFAGC_1,        0x04,
    R0297_CTRL_5,          0x80,
    R0297_CTRL_6,          0xc0,
    R0297_CTRL_8,          0x08,
    0x00
    };

const U8 QAM64_MT2060[] =
    {
    R0297_DELAGC_0,        0xff,
    R0297_DELAGC_1,        0x33,
    R0297_DELAGC_2,        0xcc,
    R0297_DELAGC_3,        0x33,
    R0297_DELAGC_4,        0x31,
    R0297_DELAGC_5,        0x7f,
    R0297_DELAGC_6,        0x80,
    R0297_DELAGC_7,        0x6a,
    R0297_DELAGC_8,        0x0f,
    R0297_WBAGC_0,         0x1a,
    R0297_WBAGC_1,         0x78,
    R0297_WBAGC_2,         0x21,
    R0297_WBAGC_3,         0x18,
    R0297_WBAGC_4,         0x80,
    R0297_WBAGC_5,         0x00,
    R0297_WBAGC_6,         0x00,
    R0297_WBAGC_9,         0x12,
    R0297_WBAGC_10,        0x5e,
    R0297_WBAGC_11,        0xfa,
    R0297_STLOOP_2,        0x30,
    R0297_STLOOP_3,        0x06,
    R0297_STLOOP_9,        0x08,
    R0297_STLOOP_10,       0x1e,
    R0297_STLOOP_11,       0x04,
    R0297_CRL_0,           0x27,
    R0297_CRL_1,           0x48,
    R0297_CRL_2,           0x0b,
    R0297_CRL_6,           0xda,
    R0297_CRL_7,           0xf5,
    R0297_CRL_8,           0x23,
    R0297_CRL_9,           0x00,
    R0297_CRL_10,          0x02,
    R0297_CRL_11,          0x00,
    R0297_PMFAGC_0,        0xff,
    R0297_PMFAGC_1,        0x04,
    R0297_CTRL_5,          0x80,
    R0297_CTRL_6,          0xc0,
    R0297_CTRL_8,          0x00,
    0x00
    };

const U8 QAM128_MT2060[] =
    {
    R0297_DELAGC_0,        0xff,
    R0297_DELAGC_1,        0x00,
    R0297_DELAGC_2,        0xcc,
    R0297_DELAGC_3,        0x00,
    R0297_DELAGC_4,        0x32,
    R0297_DELAGC_5,        0x7f,
    R0297_DELAGC_6,        0x80,
    R0297_DELAGC_7,        0x65,
    R0297_DELAGC_8,        0x00,
    R0297_WBAGC_0,         0x1a,
    R0297_WBAGC_1,         0xff,
    R0297_WBAGC_2,         0x23,
    R0297_WBAGC_3,         0x18,
    R0297_WBAGC_4,         0x80,
    R0297_WBAGC_5,         0x00,
    R0297_WBAGC_6,         0x00,
    R0297_WBAGC_9,         0x12,
    R0297_WBAGC_10,        0x70,
    R0297_WBAGC_11,        0xfd,
    R0297_STLOOP_2,        0x30,
    R0297_STLOOP_3,        0x08,
    R0297_STLOOP_9,        0x08,
    R0297_STLOOP_10,       0x1e,
    R0297_STLOOP_11,       0x04,
    R0297_CRL_0,           0x27,
    R0297_CRL_1,           0x3a,
    R0297_CRL_2,           0x0b,
    R0297_CRL_6,           0xf5,
    R0297_CRL_7,           0xa3,
    R0297_CRL_8,           0x0d,
    R0297_CRL_9,           0x02,
    R0297_CRL_10,          0x03,
    R0297_CRL_11,          0x00,
    R0297_PMFAGC_0,        0xff,
    R0297_PMFAGC_1,        0x04,
    R0297_CTRL_5,          0x80,
    R0297_CTRL_6,          0xc0,
    R0297_CTRL_8,          0x08,
    0x00
    };

const U8 QAM256_MT2060[] =
    {
    R0297_DELAGC_0,        0xff,
    R0297_DELAGC_1,        0x00,
    R0297_DELAGC_2,        0xcc,
    R0297_DELAGC_3,        0x00,
    R0297_DELAGC_4,        0x32,
    R0297_DELAGC_5,        0x7f,
    R0297_DELAGC_6,        0x80,
    R0297_DELAGC_7,        0x60,
    R0297_DELAGC_8,        0x00,
    R0297_WBAGC_0,         0x1a,
    R0297_WBAGC_1,         0xa4,
    R0297_WBAGC_2,         0x21,
    R0297_WBAGC_3,         0x18,
    R0297_WBAGC_4,         0x80,
    R0297_WBAGC_5,         0x00,
    R0297_WBAGC_6,         0x00,
    R0297_WBAGC_9,         0x12,
    R0297_WBAGC_10,        0xf9,
    R0297_WBAGC_11,        0xfe,
    R0297_STLOOP_2,        0x30,
    R0297_STLOOP_3,        0x06,
    R0297_STLOOP_9,        0x06,
    R0297_STLOOP_10,       0x1e,
    R0297_STLOOP_11,       0x04,
    R0297_CRL_0,           0x27,
    R0297_CRL_1,           0x39,
    R0297_CRL_2,           0x0b,
    R0297_CRL_6,           0x85,
    R0297_CRL_7,           0xf8,
    R0297_CRL_8,           0x23,
    R0297_CRL_9,           0x00,
    R0297_CRL_10,          0x02,
    R0297_CRL_11,          0x00,
    R0297_PMFAGC_0,        0xff,
    R0297_PMFAGC_1,        0x04,
    R0297_CTRL_5,          0x80,
    R0297_CTRL_6,          0xc0,
    R0297_CTRL_8,          0x00,
    0x00
    };

/* Initialization of DCT7040 */
/* ========================= */

#ifdef STTUNER_DRV_CAB_CBR0
const U8 QAM64_DCT7040[] =
    {
    R0297_DELAGC_0,        0xff,
    R0297_DELAGC_1,        0x00,
    R0297_DELAGC_2,        0xff,
    R0297_DELAGC_3,        0x0a,
    R0297_DELAGC_4,        0x3a,
    R0297_DELAGC_5,        0x7f,
    R0297_DELAGC_6,        0x80,
    R0297_DELAGC_7,        0x63,
    R0297_DELAGC_8,        0x00,
    R0297_WBAGC_0,         0x1a,
    R0297_WBAGC_1,         0x94,
    R0297_WBAGC_2,         0x22,
    R0297_WBAGC_3,         0x18,
    R0297_WBAGC_4,         0x80,
    R0297_WBAGC_5,         0x00,
    R0297_WBAGC_6,         0x00,
    R0297_WBAGC_9,         0x12,
    R0297_WBAGC_10,        0x5e,
    R0297_WBAGC_11,        0xfa,
    R0297_STLOOP_3,        0x06,
    R0297_STLOOP_5,        0x55,
    R0297_STLOOP_6,        0x45,
    R0297_STLOOP_7,        0x8b,
    R0297_STLOOP_8,        0x3d,
    R0297_STLOOP_9,        0x06,
    R0297_STLOOP_10,       0x1e,
    R0297_CRL_0,           0x26,
    R0297_CRL_1,           0x39,
    R0297_CRL_6,           0x0c,
    R0297_CRL_7,           0x19,
    R0297_CRL_8,           0xd5,
    R0297_CRL_11,          0x99,
    R0297_CTRL_0,          0x20,
    R0297_CTRL_2,          0x2f,
    R0297_CTRL_4,          0x04,
    R0297_CTRL_5,          0x80,
    R0297_CTRL_6,          0x88,
    R0297_CTRL_8,          0x00,
    0x00
    };

const U8 QAM256_DCT7040[] =
    {
    R0297_DELAGC_0,        0xff,
    R0297_DELAGC_1,        0x00,
    R0297_DELAGC_2,        0xff,
    R0297_DELAGC_3,        0x0c,
    R0297_DELAGC_4,        0x3a,
    R0297_DELAGC_5,        0x7f,
    R0297_DELAGC_6,        0x80,
    R0297_DELAGC_7,        0x60,
    R0297_DELAGC_8,        0x00,
    R0297_WBAGC_0,         0x1a,
    R0297_WBAGC_1,         0x29,
    R0297_WBAGC_2,         0x21,
    R0297_WBAGC_3,         0x18,
    R0297_WBAGC_4,         0x80,
    R0297_WBAGC_5,         0x00,
    R0297_WBAGC_6,         0x00,
    R0297_WBAGC_9,         0x12,
    R0297_WBAGC_10,        0x3b,
    R0297_WBAGC_11,        0xff,
    R0297_STLOOP_3,        0x06,
    R0297_STLOOP_5,        0xc0,
    R0297_STLOOP_6,        0x16,
    R0297_STLOOP_7,        0x8b,
    R0297_STLOOP_8,        0x3d,
    R0297_STLOOP_9,        0x06,
    R0297_STLOOP_10,       0x1e,
    R0297_CRL_0,           0x26,
    R0297_CRL_1,           0x3a,
    R0297_CRL_6,           0x74,
    R0297_CRL_7,           0xf8,
    R0297_CRL_8,           0x34,
    R0297_CRL_11,          0xe5,
    R0297_CTRL_0,          0x20,
    R0297_CTRL_2,          0x2f,
    R0297_CTRL_4,          0x2a,
    R0297_CTRL_5,          0x80,
    R0297_CTRL_6,          0xc0,
    R0297_CTRL_8,          0x00,
    0x00
    };

#else
/* EVAL0297-T-V20 */

const U8 QAM16_DCT7040[] =
    {
    R0297_DELAGC_0,        0xff,
    R0297_DELAGC_1,        0x00,
    R0297_DELAGC_2,        0xff,
    R0297_DELAGC_3,        0x33,
    R0297_DELAGC_4,        0x32,
    R0297_DELAGC_5,        0x7f,
    R0297_DELAGC_6,        0x80,
    R0297_DELAGC_7,        0x6a,
    R0297_DELAGC_8,        0x41,
    R0297_WBAGC_0,         0x1a,
    R0297_WBAGC_1,         0x3e,
    R0297_WBAGC_2,         0x21,
    R0297_WBAGC_3,         0x18,
    R0297_WBAGC_4,         0x80,
    R0297_WBAGC_5,         0x00,
    R0297_WBAGC_6,         0x00,
    R0297_WBAGC_9,         0x12,
    R0297_WBAGC_10,        0xf5,
    R0297_WBAGC_11,        0xe8,
    R0297_STLOOP_2,        0x30,
    R0297_STLOOP_3,        0x06,
    R0297_STLOOP_9,        0x06,
    R0297_STLOOP_10,       0x1e,
    R0297_STLOOP_11,       0x04,
    R0297_CRL_0,           0x26,
    R0297_CRL_1,           0x38,
    R0297_CRL_2,           0x0a,
    R0297_CRL_6,           0x30,
    R0297_CRL_7,           0xf7,
    R0297_CRL_8,           0x06,
    R0297_CRL_9,           0x00,
    R0297_CRL_10,          0x02,
    R0297_CRL_11,          0x61,
    R0297_PMFAGC_0,        0xff,
    R0297_PMFAGC_1,        0x04,
    R0297_CTRL_5,          0x80,
    R0297_CTRL_6,          0x88,
    R0297_CTRL_8,          0x00,
    0x00
    };

const U8 QAM32_DCT7040[] =
    {
    R0297_DELAGC_0,        0xff,
    R0297_DELAGC_1,        0x00,
    R0297_DELAGC_2,        0xff,
    R0297_DELAGC_3,        0x33,
    R0297_DELAGC_4,        0x32,
    R0297_DELAGC_5,        0x7f,
    R0297_DELAGC_6,        0x80,
    R0297_DELAGC_7,        0x6f,
    R0297_DELAGC_8,        0x15,
    R0297_WBAGC_0,         0x19,
    R0297_WBAGC_1,         0x3d,
    R0297_WBAGC_2,         0x21,
    R0297_WBAGC_3,         0x18,
    R0297_WBAGC_4,         0x80,
    R0297_WBAGC_5,         0x00,
    R0297_WBAGC_6,         0x00,
    R0297_WBAGC_9,         0x12,
    R0297_WBAGC_10,        0xc2,
    R0297_WBAGC_11,        0xf5,
    R0297_STLOOP_2,        0x30,
    R0297_STLOOP_3,        0x08,
    R0297_STLOOP_9,        0x08,
    R0297_STLOOP_10,       0x1e,
    R0297_STLOOP_11,       0x04,
    R0297_CRL_0,           0x10,
    R0297_CRL_1,           0x39,
    R0297_CRL_2,           0x06,
    R0297_CRL_6,           0x44,
    R0297_CRL_7,           0xf8,
    R0297_CRL_8,           0x06,
    R0297_CRL_9,           0x00,
    R0297_CRL_10,          0x02,
    R0297_CRL_11,          0x61,
    R0297_PMFAGC_0,        0xff,
    R0297_PMFAGC_1,        0x04,
    R0297_CTRL_5,          0x80,
    R0297_CTRL_6,          0x88,
    R0297_CTRL_8,          0x00,
    0x00
    };

const U8 QAM64_DCT7040[] =
    {
    R0297_DELAGC_0,        0xff,
    R0297_DELAGC_1,        0x00,
    R0297_DELAGC_2,        0xff,
    R0297_DELAGC_3,        0x33,
    R0297_DELAGC_4,        0x32,
    R0297_DELAGC_5,        0x7f,
    R0297_DELAGC_6,        0x80,
    R0297_DELAGC_7,        0x6b,
    R0297_DELAGC_8,        0x10,
    R0297_WBAGC_0,         0x1a,
    R0297_WBAGC_1,         0x3f,
    R0297_WBAGC_2,         0x21,
    R0297_WBAGC_3,         0x18,
    R0297_WBAGC_4,         0x80,
    R0297_WBAGC_5,         0x00,
    R0297_WBAGC_6,         0x00,
    R0297_WBAGC_9,         0x12,
    R0297_WBAGC_10,        0x5e,
    R0297_WBAGC_11,        0xfa,
    R0297_STLOOP_2,        0x30,
    R0297_STLOOP_3,        0x07,
    R0297_STLOOP_9,        0x08,
    R0297_STLOOP_10,       0x1e,
    R0297_STLOOP_11,       0x04,
    R0297_CRL_0,           0x26,
    R0297_CRL_1,           0x39,
    R0297_CRL_2,           0x05,
    R0297_CRL_6,           0x3e,
    R0297_CRL_7,           0xfe,
    R0297_CRL_8,           0x06,
    R0297_CRL_9,           0x00,
    R0297_CRL_10,          0x02,
    R0297_CRL_11,          0x61,
    R0297_PMFAGC_0,        0xff,
    R0297_PMFAGC_1,        0x04,
    R0297_CTRL_5,          0x80,
    R0297_CTRL_6,          0x88,
    R0297_CTRL_8,          0x00,
    0x00
    };

const U8 QAM128_DCT7040[] =
    {
    R0297_DELAGC_0,        0xff,
    R0297_DELAGC_1,        0x00,
    R0297_DELAGC_2,        0xff,
    R0297_DELAGC_3,        0x33,
    R0297_DELAGC_4,        0x32,
    R0297_DELAGC_5,        0x7f,
    R0297_DELAGC_6,        0x80,
    R0297_DELAGC_7,        0x69,
    R0297_DELAGC_8,        0x07,
    R0297_WBAGC_0,         0x1a,
    R0297_WBAGC_1,         0x3f,
    R0297_WBAGC_2,         0x21,
    R0297_WBAGC_3,         0x18,
    R0297_WBAGC_4,         0x80,
    R0297_WBAGC_5,         0x00,
    R0297_WBAGC_6,         0x00,
    R0297_WBAGC_9,         0x12,
    R0297_WBAGC_10,        0x70,
    R0297_WBAGC_11,        0xfd,
    R0297_STLOOP_2,        0x30,
    R0297_STLOOP_3,        0x08,
    R0297_STLOOP_9,        0x08,
    R0297_STLOOP_10,       0x1e,
    R0297_STLOOP_11,       0x04,
    R0297_CRL_0,           0x13,
    R0297_CRL_1,           0x3a,
    R0297_CRL_2,           0x06,
    R0297_CRL_6,           0x09,
    R0297_CRL_7,           0x00,
    R0297_CRL_8,           0x07,
    R0297_CRL_9,           0x00,
    R0297_CRL_10,          0x02,
    R0297_CRL_11,          0x61,
    R0297_PMFAGC_0,        0xff,
    R0297_PMFAGC_1,        0x04,
    R0297_CTRL_5,          0x80,
    R0297_CTRL_6,          0x88,
    R0297_CTRL_8,          0x00,
    0x00
    };

const U8 QAM256_DCT7040[] =
    {
    R0297_DELAGC_0,        0xff,
    R0297_DELAGC_1,        0x00,
    R0297_DELAGC_2,        0xfd,
    R0297_DELAGC_3,        0x67,
    R0297_DELAGC_4,        0x2a,
    R0297_DELAGC_5,        0x66,
    R0297_DELAGC_6,        0x80,
    R0297_DELAGC_7,        0x6c,
    R0297_DELAGC_8,        0x03,
    R0297_WBAGC_0,         0x1a,
    R0297_WBAGC_1,         0x9f,
    R0297_WBAGC_2,         0x20,
    R0297_WBAGC_3,         0x18,
    R0297_WBAGC_4,         0x80,
    R0297_WBAGC_5,         0x00,
    R0297_WBAGC_6,         0x00,
    R0297_WBAGC_9,         0x12,
    R0297_WBAGC_10,        0xb8,
    R0297_WBAGC_11,        0xfe,
    R0297_STLOOP_2,        0x30,
    R0297_STLOOP_3,        0x06,
    R0297_STLOOP_9,        0x06,
    R0297_STLOOP_10,       0x1e,
    R0297_STLOOP_11,       0x04,
    R0297_CRL_0,           0x10,
    R0297_CRL_1,           0x4a,
    R0297_CRL_2,           0x05,
    R0297_CRL_6,           0x74,
    R0297_CRL_7,           0x22,
    R0297_CRL_8,           0x07,
    R0297_CRL_9,           0x00,
    R0297_CRL_10,          0x02,
    R0297_CRL_11,          0x61,
    R0297_PMFAGC_0,        0xff,
    R0297_PMFAGC_1,        0x04,
    R0297_CTRL_5,          0x80,
    R0297_CTRL_6,          0x88,
    R0297_CTRL_8,          0x00,
    0x00
    };
#endif

/* Initialization of DCF8710 */
/* ========================= */

const U8 QAM16_DCF8710[] =
    {
    R0297_DELAGC_0,        0xFF,
    R0297_DELAGC_1,        0x6c,
    R0297_DELAGC_2,        0xff,
    R0297_DELAGC_3,        0x0,
    R0297_DELAGC_4,        0x12,
    R0297_DELAGC_5,        0x6c,
    R0297_DELAGC_6,        0x80,
    R0297_DELAGC_7,        0x30,
    R0297_DELAGC_8,        0x0,
    R0297_WBAGC_0,         0x1C,
    R0297_WBAGC_1,         0xdb,
    R0297_WBAGC_2,         0x30,
    R0297_WBAGC_3,         0x18,
    R0297_WBAGC_4,         0x58,
    R0297_WBAGC_5,         0x00,
    R0297_WBAGC_6,         0x00,
    R0297_WBAGC_9,         0x02,
    R0297_WBAGC_10,        0x33,
    R0297_WBAGC_11,        0xFF,
    R0297_STLOOP_3,        0x08,
    R0297_STLOOP_5,        0x44,
    R0297_STLOOP_6,        0xdf,
    R0297_STLOOP_7,        0x18,
    R0297_STLOOP_8,        0x3d,
    R0297_STLOOP_9,        0x08,
    R0297_STLOOP_10,       0x1E,
    R0297_CRL_0,           0x13,
    R0297_CRL_1,           0x39,
    R0297_CRL_6,           0x7b,
    R0297_CRL_7,           0x1e,
    R0297_CRL_8,           0x01,
    R0297_CRL_11,          0xd0,
    R0297_CTRL_0,          0x20,
    R0297_CTRL_2,          0x0f,
    R0297_CTRL_4,          0x2a,
    R0297_CTRL_5,          0x00,
    R0297_CTRL_6,          0xc0,
    R0297_CTRL_8,          0x10,
    0x00
    };

const U8 QAM32_DCF8710[] =
    {
    R0297_DELAGC_0,        0xFF,
    R0297_DELAGC_1,        0x6c,
    R0297_DELAGC_2,        0xff,
    R0297_DELAGC_3,        0x0,
    R0297_DELAGC_4,        0x12,
    R0297_DELAGC_5,        0x6c,
    R0297_DELAGC_6,        0x80,
    R0297_DELAGC_7,        0x30,
    R0297_DELAGC_8,        0x0,
    R0297_WBAGC_0,         0x1C,
    R0297_WBAGC_1,         0xdb,
    R0297_WBAGC_2,         0x30,
    R0297_WBAGC_3,         0x18,
    R0297_WBAGC_4,         0x58,
    R0297_WBAGC_5,         0x00,
    R0297_WBAGC_6,         0x00,
    R0297_WBAGC_9,         0x02,
    R0297_WBAGC_10,        0x33,
    R0297_WBAGC_11,        0xFF,
    R0297_STLOOP_3,        0x08,
    R0297_STLOOP_5,        0x44,
    R0297_STLOOP_6,        0xdf,
    R0297_STLOOP_7,        0x18,
    R0297_STLOOP_8,        0x3d,
    R0297_STLOOP_9,        0x08,
    R0297_STLOOP_10,       0x1E,
    R0297_CRL_0,           0x13,
    R0297_CRL_1,           0x39,
    R0297_CRL_6,           0x7b,
    R0297_CRL_7,           0x1e,
    R0297_CRL_8,           0x01,
    R0297_CRL_11,          0xd0,
    R0297_CTRL_0,          0x20,
    R0297_CTRL_2,          0x0f,
    R0297_CTRL_4,          0x2a,
    R0297_CTRL_5,          0x00,
    R0297_CTRL_6,          0xc0,
    R0297_CTRL_8,          0x10,
    0x00
    };

const U8 QAM64_DCF8710[] =
    {
    R0297_DELAGC_0,        0xFF,
    R0297_DELAGC_1,        0x6c,
    R0297_DELAGC_2,        0xff,
    R0297_DELAGC_3,        0x0,
    R0297_DELAGC_4,        0x12,
    R0297_DELAGC_5,        0x6c,
    R0297_DELAGC_6,        0x80,
    R0297_DELAGC_7,        0x30,
    R0297_DELAGC_8,        0x0,
    R0297_WBAGC_0,         0x1C,
    R0297_WBAGC_1,         0xdb,
    R0297_WBAGC_2,         0x30,
    R0297_WBAGC_3,         0x18,
    R0297_WBAGC_4,         0x58,
    R0297_WBAGC_5,         0x00,
    R0297_WBAGC_6,         0x00,
    R0297_WBAGC_9,         0x02,
    R0297_WBAGC_10,        0x33,
    R0297_WBAGC_11,        0xFF,
    R0297_STLOOP_3,        0x08,
    R0297_STLOOP_5,        0x44,
    R0297_STLOOP_6,        0xdf,
    R0297_STLOOP_7,        0x18,
    R0297_STLOOP_8,        0x3d,
    R0297_STLOOP_9,        0x08,
    R0297_STLOOP_10,       0x1E,
    R0297_CRL_0,           0x13,
    R0297_CRL_1,           0x39,
    R0297_CRL_6,           0x7b,
    R0297_CRL_7,           0x1e,
    R0297_CRL_8,           0x01,
    R0297_CRL_11,          0xd0,
    R0297_CTRL_0,          0x20,
    R0297_CTRL_2,          0x0f,
    R0297_CTRL_4,          0x2a,
    R0297_CTRL_5,          0x00,
    R0297_CTRL_6,          0xc0,
    R0297_CTRL_8,          0x10,
    0x00
    };

const U8 QAM128_DCF8710[] =
    {
    R0297_DELAGC_0,        0xff,
    R0297_DELAGC_1,        0x6c,
    R0297_DELAGC_2,        0xff,
    R0297_DELAGC_3,        0x0,
    R0297_DELAGC_4,        0x12,
    R0297_DELAGC_5,        0x6c,
    R0297_DELAGC_6,        0x80,
    R0297_DELAGC_7,        0x00,
    R0297_DELAGC_8,        0x0,
    R0297_WBAGC_0,         0x1b,
    R0297_WBAGC_1,         0xda,
    R0297_WBAGC_2,         0x30,
    R0297_WBAGC_3,         0x18,
    R0297_WBAGC_4,         0x58,
    R0297_WBAGC_5,         0x00,
    R0297_WBAGC_6,         0x00,
    R0297_WBAGC_9,         0x02,
    R0297_WBAGC_10,        0xff,
    R0297_WBAGC_11,        0xff,
    R0297_STLOOP_3,        0x08,
    R0297_STLOOP_5,        0x04,
    R0297_STLOOP_6,        0xdf,
    R0297_STLOOP_7,        0x18,
    R0297_STLOOP_8,        0x3d,
    R0297_STLOOP_9,        0x08,
    R0297_STLOOP_10,       0x1E,
    R0297_CRL_0,           0x13,
    R0297_CRL_1,           0x39,
    R0297_CRL_6,           0xb0,
    R0297_CRL_7,           0x0f,
    R0297_CRL_8,           0x01,
    R0297_CRL_11,          0x1e,
    R0297_CTRL_0,          0x20,
    R0297_CTRL_2,          0x1f,
    R0297_CTRL_4,          0x2a,
    R0297_CTRL_5,          0x00,
    R0297_CTRL_6,          0xc0,
    R0297_CTRL_8,          0x10,
    0x00
    };

const U8 QAM256_DCF8710[] =
    {
    R0297_DELAGC_0,        0xff,
    R0297_DELAGC_1,        0x6c,
    R0297_DELAGC_2,        0xff,
    R0297_DELAGC_3,        0x0,
    R0297_DELAGC_4,        0x12,
    R0297_DELAGC_5,        0x6c,
    R0297_DELAGC_6,        0x80,
    R0297_DELAGC_7,        0x00,
    R0297_DELAGC_8,        0x0,
    R0297_WBAGC_0,         0x1b,
    R0297_WBAGC_1,         0xda,
    R0297_WBAGC_2,         0x30,
    R0297_WBAGC_3,         0x18,
    R0297_WBAGC_4,         0x58,
    R0297_WBAGC_5,         0x00,
    R0297_WBAGC_6,         0x00,
    R0297_WBAGC_9,         0x02,
    R0297_WBAGC_10,        0xff,
    R0297_WBAGC_11,        0xff,
    R0297_STLOOP_3,        0x08,
    R0297_STLOOP_5,        0x04,
    R0297_STLOOP_6,        0xdf,
    R0297_STLOOP_7,        0x18,
    R0297_STLOOP_8,        0x3d,
    R0297_STLOOP_9,        0x08,
    R0297_STLOOP_10,       0x1E,
    R0297_CRL_0,           0x13,
    R0297_CRL_1,           0x39,
    R0297_CRL_6,           0xb0,
    R0297_CRL_7,           0x0f,
    R0297_CRL_8,           0x01,
    R0297_CRL_11,          0x1e,
    R0297_CTRL_0,          0x20,
    R0297_CTRL_2,          0x1f,
    R0297_CTRL_4,          0x2a,
    R0297_CTRL_5,          0x00,
    R0297_CTRL_6,          0xc0,
    R0297_CTRL_8,          0x10,
    0x00
    };

/* Initialization of DCF8720 */
/* ========================= */

const U8 QAM16_DCF8720[] =
    {
    R0297_DELAGC_0,        0xFF,
    R0297_DELAGC_1,        0x5c,
    R0297_DELAGC_2,        0xff,
    R0297_DELAGC_3,        0x0,
    R0297_DELAGC_4,        0x32,
    R0297_DELAGC_5,        0x5c,
    R0297_DELAGC_6,        0x80,
    R0297_DELAGC_7,        0x30,
    R0297_DELAGC_8,        0x0,
    R0297_WBAGC_0,         0x1b,
    R0297_WBAGC_1,         0x9d,
    R0297_WBAGC_2,         0x30,
    R0297_WBAGC_3,         0x18,
    R0297_WBAGC_4,         0x58,
    R0297_WBAGC_5,         0x00,
    R0297_WBAGC_6,         0x00,
    R0297_WBAGC_9,         0x02,
    R0297_WBAGC_10,        0x33,
    R0297_WBAGC_11,        0xFF,
    R0297_STLOOP_3,        0x08,
    R0297_STLOOP_5,        0x44,
    R0297_STLOOP_6,        0xdf,
    R0297_STLOOP_7,        0x18,
    R0297_STLOOP_8,        0x3d,
    R0297_STLOOP_9,        0x08,
    R0297_STLOOP_10,       0x1E,
    R0297_CRL_0,           0x13,
    R0297_CRL_1,           0x39,
    R0297_CRL_6,           0x7b,
    R0297_CRL_7,           0x1e,
    R0297_CRL_8,           0x01,
    R0297_CRL_11,          0xd0,
    R0297_CTRL_0,          0x20,
    R0297_CTRL_2,          0x0f,
    R0297_CTRL_4,          0x2a,
    R0297_CTRL_5,          0x00,
    R0297_CTRL_6,          0xc0,
    R0297_CTRL_8,          0x00,
    0x00
    };

const U8 QAM32_DCF8720[] =
    {
    R0297_DELAGC_0,        0xFF,
    R0297_DELAGC_1,        0x5c,
    R0297_DELAGC_2,        0xff,
    R0297_DELAGC_3,        0x0,
    R0297_DELAGC_4,        0x32,
    R0297_DELAGC_5,        0x5c,
    R0297_DELAGC_6,        0x80,
    R0297_DELAGC_7,        0x30,
    R0297_DELAGC_8,        0x0,
    R0297_WBAGC_0,         0x1b,
    R0297_WBAGC_1,         0x9d,
    R0297_WBAGC_2,         0x30,
    R0297_WBAGC_3,         0x18,
    R0297_WBAGC_4,         0x58,
    R0297_WBAGC_5,         0x00,
    R0297_WBAGC_6,         0x00,
    R0297_WBAGC_9,         0x02,
    R0297_WBAGC_10,        0x33,
    R0297_WBAGC_11,        0xFF,
    R0297_STLOOP_3,        0x08,
    R0297_STLOOP_5,        0x44,
    R0297_STLOOP_6,        0xdf,
    R0297_STLOOP_7,        0x18,
    R0297_STLOOP_8,        0x3d,
    R0297_STLOOP_9,        0x08,
    R0297_STLOOP_10,       0x1E,
    R0297_CRL_0,           0x13,
    R0297_CRL_1,           0x39,
    R0297_CRL_6,           0x7b,
    R0297_CRL_7,           0x1e,
    R0297_CRL_8,           0x01,
    R0297_CRL_11,          0xd0,
    R0297_CTRL_0,          0x20,
    R0297_CTRL_2,          0x0f,
    R0297_CTRL_4,          0x2a,
    R0297_CTRL_5,          0x00,
    R0297_CTRL_6,          0xc0,
    R0297_CTRL_8,          0x00,
    0x00
    };

const U8 QAM64_DCF8720[] =
    {
    R0297_DELAGC_0,        0xFF,
    R0297_DELAGC_1,        0x5c,
    R0297_DELAGC_2,        0xff,
    R0297_DELAGC_3,        0x0,
    R0297_DELAGC_4,        0x32,
    R0297_DELAGC_5,        0x5c,
    R0297_DELAGC_6,        0x80,
    R0297_DELAGC_7,        0x30,
    R0297_DELAGC_8,        0x0,
    R0297_WBAGC_0,         0x1b,
    R0297_WBAGC_1,         0x9d,
    R0297_WBAGC_2,         0x30,
    R0297_WBAGC_3,         0x18,
    R0297_WBAGC_4,         0x58,
    R0297_WBAGC_5,         0x00,
    R0297_WBAGC_6,         0x00,
    R0297_WBAGC_9,         0x02,
    R0297_WBAGC_10,        0x33,
    R0297_WBAGC_11,        0xFF,
    R0297_STLOOP_3,        0x08,
    R0297_STLOOP_5,        0x44,
    R0297_STLOOP_6,        0xdf,
    R0297_STLOOP_7,        0x18,
    R0297_STLOOP_8,        0x3d,
    R0297_STLOOP_9,        0x08,
    R0297_STLOOP_10,       0x1E,
    R0297_CRL_0,           0x13,
    R0297_CRL_1,           0x39,
    R0297_CRL_6,           0x7b,
    R0297_CRL_7,           0x1e,
    R0297_CRL_8,           0x01,
    R0297_CRL_11,          0xd0,
    R0297_CTRL_0,          0x20,
    R0297_CTRL_2,          0x0f,
    R0297_CTRL_4,          0x2a,
    R0297_CTRL_5,          0x00,
    R0297_CTRL_6,          0xc0,
    R0297_CTRL_8,          0x00,
    0x00
    };

const U8 QAM128_DCF8720[] =
    {
    R0297_DELAGC_0,        0xff,
    R0297_DELAGC_1,        0x5c,
    R0297_DELAGC_2,        0xff,
    R0297_DELAGC_3,        0x0,
    R0297_DELAGC_4,        0x32,
    R0297_DELAGC_5,        0x5c,
    R0297_DELAGC_6,        0x80,
    R0297_DELAGC_7,        0x00,
    R0297_DELAGC_8,        0x0,
    R0297_WBAGC_0,         0x1b,
    R0297_WBAGC_1,         0xba,
    R0297_WBAGC_2,         0x30,
    R0297_WBAGC_3,         0x18,
    R0297_WBAGC_4,         0x58,
    R0297_WBAGC_5,         0x00,
    R0297_WBAGC_6,         0x00,
    R0297_WBAGC_9,         0x02,
    R0297_WBAGC_10,        0xff,
    R0297_WBAGC_11,        0xff,
    R0297_STLOOP_3,        0x08,
    R0297_STLOOP_5,        0x04,
    R0297_STLOOP_6,        0xdf,
    R0297_STLOOP_7,        0x18,
    R0297_STLOOP_8,        0x3d,
    R0297_STLOOP_9,        0x08,
    R0297_STLOOP_10,       0x1E,
    R0297_CRL_0,           0x13,
    R0297_CRL_1,           0x39,
    R0297_CRL_6,           0xb0,
    R0297_CRL_7,           0x0f,
    R0297_CRL_8,           0x01,
    R0297_CRL_11,          0x1e,
    R0297_CTRL_0,          0x20,
    R0297_CTRL_2,          0x1f,
    R0297_CTRL_4,          0x2a,
    R0297_CTRL_5,          0x00,
    R0297_CTRL_6,          0xc0,
    R0297_CTRL_8,          0x00,
    0x00
    };

const U8 QAM256_DCF8720[] =
    {
    R0297_DELAGC_0,        0xff,
    R0297_DELAGC_1,        0x5c,
    R0297_DELAGC_2,        0xff,
    R0297_DELAGC_3,        0x0,
    R0297_DELAGC_4,        0x32,
    R0297_DELAGC_5,        0x5c,
    R0297_DELAGC_6,        0x80,
    R0297_DELAGC_7,        0x00,
    R0297_DELAGC_8,        0x0,
    R0297_WBAGC_0,         0x1b,
    R0297_WBAGC_1,         0xba,
    R0297_WBAGC_2,         0x30,
    R0297_WBAGC_3,         0x18,
    R0297_WBAGC_4,         0x58,
    R0297_WBAGC_5,         0x00,
    R0297_WBAGC_6,         0x00,
    R0297_WBAGC_9,         0x02,
    R0297_WBAGC_10,        0xff,
    R0297_WBAGC_11,        0xff,
    R0297_STLOOP_3,        0x08,
    R0297_STLOOP_5,        0x04,
    R0297_STLOOP_6,        0xdf,
    R0297_STLOOP_7,        0x18,
    R0297_STLOOP_8,        0x3d,
    R0297_STLOOP_9,        0x08,
    R0297_STLOOP_10,       0x1E,
    R0297_CRL_0,           0x13,
    R0297_CRL_1,           0x39,
    R0297_CRL_6,           0xb0,
    R0297_CRL_7,           0x0f,
    R0297_CRL_8,           0x01,
    R0297_CRL_11,          0x1e,
    R0297_CTRL_0,          0x20,
    R0297_CTRL_2,          0x1f,
    R0297_CTRL_4,          0x2a,
    R0297_CTRL_5,          0x00,
    R0297_CTRL_6,          0xc0,
    R0297_CTRL_8,          0x00,
    0x00
    };

/* Initialization of default tuner */
/* =============================== */

const U8 QAM16_DEFTUN[] =
    {
    R0297_DELAGC_2,        0xF4,
    R0297_DELAGC_7,        0x6F,
    R0297_DELAGC_8,        0xDC,
    R0297_WBAGC_1,         0xE5,
    R0297_WBAGC_2,         0x2F,
    R0297_WBAGC_3,         0x00,
    R0297_WBAGC_4,         0xC4,
    R0297_WBAGC_9,         0x09,
    R0297_WBAGC_10,        0x66,
    R0297_WBAGC_11,        0xE6,
    R0297_STLOOP_3,        0x06,
    R0297_STLOOP_9,        0x08,
    R0297_STLOOP_10,       0x5E,
    R0297_CRL_1,           0x49,
    R0297_CRL_2,           0x0B,
    R0297_CRL_9,           0x0F,
    R0297_CRL_10,          0x03,
    R0297_PMFAGC_4,        0x0C,
    0x00
    };

const U8 QAM32_DEFTUN[] =
    {
    R0297_DELAGC_2,        0xEB,
    R0297_DELAGC_7,        0x6C,
    R0297_DELAGC_8,        0x33,
    R0297_WBAGC_1,         0x4A,
    R0297_WBAGC_2,         0x2D,
    R0297_WBAGC_3,         0x00,
    R0297_WBAGC_4,         0xC4,
    R0297_WBAGC_9,         0x09,
    R0297_WBAGC_10,        0x0A,
    R0297_WBAGC_11,        0xF7,
    R0297_STLOOP_3,        0x08,
    R0297_STLOOP_9,        0x08,
    R0297_STLOOP_10,       0x1E,
    R0297_CRL_1,           0x49,
    R0297_CRL_2,           0x05,
    R0297_CRL_9,           0x00,
    R0297_CRL_10,          0x02,
    R0297_PMFAGC_4,        0x0C,
    0x00
    };

const U8 QAM64_DEFTUN[] =
    {
    R0297_DELAGC_2,        0xE5,
    R0297_DELAGC_7,        0x6C,
    R0297_DELAGC_8,        0x6E,
    R0297_WBAGC_1,         0x9F,
    R0297_WBAGC_2,         0x20,
    R0297_WBAGC_3,         0x18,
    R0297_WBAGC_4,         0x80,
    R0297_WBAGC_9,         0x12,
    R0297_WBAGC_10,        0x51,
    R0297_WBAGC_11,        0xF8,
    R0297_STLOOP_3,        0x06,
    R0297_STLOOP_9,        0x08,
    R0297_STLOOP_10,       0x1E,
    R0297_CRL_1,           0x49,
    R0297_CRL_2,           0x05,
    R0297_CRL_9,           0x00,
    R0297_CRL_10,          0x03,
    R0297_PMFAGC_4,        0x00,
    0x00
    };

const U8 QAM128_DEFTUN[] =
    {
    R0297_DELAGC_2,        0xE5,
    R0297_DELAGC_7,        0x60,
    R0297_DELAGC_8,        0x00,
    R0297_WBAGC_1,         0xED,
    R0297_WBAGC_2,         0x2F,
    R0297_WBAGC_3,         0x00,
    R0297_WBAGC_4,         0xC4,
    R0297_WBAGC_9,         0x09,
    R0297_WBAGC_10,        0x14,
    R0297_WBAGC_11,        0xFE,
    R0297_STLOOP_3,        0x06,
    R0297_STLOOP_9,        0x08,
    R0297_STLOOP_10,       0x5E,
    R0297_CRL_1,           0x49,
    R0297_CRL_2,           0x0B,
    R0297_CRL_9,           0x0F,
    R0297_CRL_10,          0x03,
    R0297_PMFAGC_4,        0x0C,
    0x00
    };

const U8 QAM256_DEFTUN[] =
    {
    R0297_DELAGC_2,        0xE5,
    R0297_DELAGC_7,        0x6C,
    R0297_DELAGC_8,        0x42,
    R0297_WBAGC_1,         0xFF,
    R0297_WBAGC_2,         0x2F,
    R0297_WBAGC_3,         0x00,
    R0297_WBAGC_4,         0xC4,
    R0297_WBAGC_9,         0x09,
    R0297_WBAGC_10,        0x76,
    R0297_WBAGC_11,        0xFE,
    R0297_STLOOP_3,        0x06,
    R0297_STLOOP_9,        0x08,
    R0297_STLOOP_10,       0x5E,
    R0297_CRL_1,           0x49,
    R0297_CRL_2,           0x0B,
    R0297_CRL_9,           0x0F,
    R0297_CRL_10,          0x03,
    R0297_PMFAGC_4,        0x0C,
    0x00
    };

/* variables --------------------------------------------------------------- */

static  int    Driv0297CN[5][40];

/* functions --------------------------------------------------------------- */


/***********************************************************
**FUNCTION	::	Drv0297_GetLLARevision
**ACTION	::	Returns the 297 LLA driver revision
**RETURN	::	Revision297
***********************************************************/
ST_Revision_t Drv0297_GetLLARevision(void)
{
	return (Revision297);
}


/*----------------------------------------------------
FUNCTION      Drv0297_WaitTuner
ACTION        Wait for tuner locked
PARAMS IN     TimeOut -> Maximum waiting time (in ms)
PARAMS OUT    NONE
RETURN        NONE (Handle == THIS_INSTANCE.Tuner.DrvHandle)
------------------------------------------------------*/
void Drv0297_WaitTuner(STTUNER_tuner_instance_t *pTunerInstance, int TimeOut)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297
   const char *identity = "STTUNER drv0297.c Drv0297_WaitTuner()";
#endif
    int Time = 0;
    BOOL TunerLocked = FALSE;
    ST_ErrorCode_t Error;

    while(!TunerLocked && (Time < TimeOut))
    {
        STV0297_DelayInMilliSec(1);
        Error = (pTunerInstance->Driver->tuner_IsTunerLocked)(pTunerInstance->DrvHandle, &TunerLocked);
        Time++;
    }
    Time--;
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297
    STTBX_Print(("%s\n", identity));
#endif
}

/*----------------------------------------------------
--FUNCTION      Drv0297_CarrierWidth
--ACTION        Compute the width of the carrier
--PARAMS IN     SymbolRate -> Symbol rate of the carrier (Kbauds or Mbauds)
--              RollOff    -> Rolloff * 100
--PARAMS OUT    NONE
--RETURN        Width of the carrier (KHz or MHz)
------------------------------------------------------*/

long Drv0297_CarrierWidth(long SymbolRate, long RollOff)
{
    return (SymbolRate  + (SymbolRate * RollOff)/100);
}

/*----------------------------------------------------
--FUNCTION      Drv0297_CheckAgc
--ACTION        Check for Agc
--PARAMS IN     Params        => Pointer to SEARCHPARAMS structure
--PARAMS OUT    Params->State => Result of the check
--RETURN        E297_NOCARRIER carrier not founded, E297_CARRIEROK otherwise
------------------------------------------------------*/
D0297_SignalType_t Drv0297_CheckAgc(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, D0297_SearchParams_t *Params)
{
/*
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297
   const char *identity = "STTUNER drv0297.c Drv0297_CheckAgc()";
#endif
*/

    if ( STTUNER_IOREG_GetField(DeviceMap, IOHandle, F0297_WAGC_ACQ) )
    {
        Params->State = E297_AGCOK;
        
    }
    else
    {
        Params->State = E297_NOAGC;
    }
/*
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297
    STTBX_Print(("%s\n", identity));
#endif
*/
    return(Params->State);
}

/*----------------------------------------------------
 FUNCTION      Drv0297_CheckData
 ACTION        Check for data founded
 PARAMS IN     Params        =>    Pointer to SEARCHPARAMS structure
 PARAMS OUT    Params->State    => Result of the check
 RETURN        E297_NODATA data not founded, E297_DATAOK otherwise
------------------------------------------------------*/

D0297_SignalType_t Drv0297_CheckData(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, D0297_SearchParams_t *Params)
{
/*
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297
   const char *identity = "STTUNER drv0297.c Drv0297_CheckData()";
#endif
*/
    if ( STTUNER_IOREG_GetField(DeviceMap, IOHandle, F0297_SYNCSTATE) )
    {
        Params->State = E297_DATAOK;
    }
    else
    {
        Params->State = E297_NODATA;
    }

/*
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297
    STTBX_Print(("%s\n", identity));
#endif
*/

    return(Params->State);
}


/*----------------------------------------------------
 FUNCTION      Drv0297_InitParams
 ACTION
 PARAMS IN     NONE
 PARAMS OUT    NONE
 RETURN        NONE
------------------------------------------------------*/
void Drv0297_InitParams(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297
   const char *identity = "STTUNER drv0297.c Drv0297_InitParams()";
#endif
    int idB ;
    int iQAM ;

    /*
    Driv0297CN[iQAM][idB] init

    This table is used for the C/N estimation (performed in the
    Driv0297CNEstimator routine).
    Driv0297CNB[iQAM][idB] gives the C/N estimated value for
    a given iQAM QAM size and idB C/N ratio.
    */
    for (idB = 0 ; idB < 40; idB++)
        for (iQAM = 0 ; iQAM < 5 ; iQAM++)
            Driv0297CN[iQAM][idB] = 100000;
    /* QAM = 16 */
    iQAM = 0 ;
    for (idB = 0 ; idB < 15 ; idB++)
        Driv0297CN[iQAM][idB] = 10500 + (15-idB)*1000;
    Driv0297CN[iQAM][15] = 10500 ;
    Driv0297CN[iQAM][16] =  9000 ;
    Driv0297CN[iQAM][17] =  8120 ;
    Driv0297CN[iQAM][18] =  7300 ;
    Driv0297CN[iQAM][19] =  6530 ;
    Driv0297CN[iQAM][20] =  5870 ;
    Driv0297CN[iQAM][21] =  5310 ;
    Driv0297CN[iQAM][22] =  4790 ;
    Driv0297CN[iQAM][23] =  4320 ;
    Driv0297CN[iQAM][24] =  3920 ;
    Driv0297CN[iQAM][25] =  3590 ;
    Driv0297CN[iQAM][26] =  3270 ;
    Driv0297CN[iQAM][27] =  3000 ;
    Driv0297CN[iQAM][28] =  2760 ;
    Driv0297CN[iQAM][29] =  2560 ;
    Driv0297CN[iQAM][30] =  2420 ;
    Driv0297CN[iQAM][31] =  2260 ;
    Driv0297CN[iQAM][32] =  2150 ;
    Driv0297CN[iQAM][33] =  2060 ;
    Driv0297CN[iQAM][34] =  1980 ;
    Driv0297CN[iQAM][35] =  1910 ;
    Driv0297CN[iQAM][36] =  1850 ;
    Driv0297CN[iQAM][37] =  1810 ;
    Driv0297CN[iQAM][38] =  1750 ;
    Driv0297CN[iQAM][39] =  1740 ;
    /* QAM = 32 */
    iQAM = 1 ;
    for (idB = 0 ; idB < 18 ; idB++)
        Driv0297CN[iQAM][idB] = 10500 + (18-idB)*1000;
    Driv0297CN[iQAM][18] = 10500 ;
    Driv0297CN[iQAM][19] =  9120 ;
    Driv0297CN[iQAM][20] =  8100 ;
    Driv0297CN[iQAM][21] =  7300 ;
    Driv0297CN[iQAM][22] =  6560 ;
    Driv0297CN[iQAM][23] =  5930 ;
    Driv0297CN[iQAM][24] =  5380 ;
    Driv0297CN[iQAM][25] =  4920 ;
    Driv0297CN[iQAM][26] =  4520 ;
    Driv0297CN[iQAM][27] =  4130 ;
    Driv0297CN[iQAM][28] =  3800 ;
    Driv0297CN[iQAM][29] =  3520 ;
    Driv0297CN[iQAM][30] =  3290 ;
    Driv0297CN[iQAM][31] =  3120 ;
    Driv0297CN[iQAM][32] =  2980 ;
    Driv0297CN[iQAM][33] =  2850 ;
    Driv0297CN[iQAM][34] =  2730 ;
    Driv0297CN[iQAM][35] =  2650 ;
    Driv0297CN[iQAM][36] =  2560 ;
    Driv0297CN[iQAM][37] =  2510 ;
    Driv0297CN[iQAM][38] =  2480 ;
    Driv0297CN[iQAM][39] =  2440 ;
    /* QAM = 64 */
    iQAM = 2 ;
    for (idB = 0 ; idB < 21 ; idB++)
        Driv0297CN[iQAM][idB] = 10500 + (21-idB)*1000;
    Driv0297CN[iQAM][21] = 10500 ;
    Driv0297CN[iQAM][22] =  9300 ;
    Driv0297CN[iQAM][23] =  8400 ;
    Driv0297CN[iQAM][24] =  7600 ;
    Driv0297CN[iQAM][25] =  6850 ;
    Driv0297CN[iQAM][26] =  6250 ;
    Driv0297CN[iQAM][27] =  5750 ;
    Driv0297CN[iQAM][28] =  5250 ;
    Driv0297CN[iQAM][29] =  4850 ;
    Driv0297CN[iQAM][30] =  4450 ;
    Driv0297CN[iQAM][31] =  4200 ;
    Driv0297CN[iQAM][32] =  3900 ;
    Driv0297CN[iQAM][33] =  3700 ;
    Driv0297CN[iQAM][34] =  3550 ;
    Driv0297CN[iQAM][35] =  3400 ;
    Driv0297CN[iQAM][36] =  3300 ;
    Driv0297CN[iQAM][37] =  3200 ;
    Driv0297CN[iQAM][38] =  3130 ;
    Driv0297CN[iQAM][39] =  3060 ;
    /* QAM = 128 */
    iQAM = 3 ;
    for (idB = 0 ; idB < 24 ; idB++)
        Driv0297CN[iQAM][idB] = 10500 + (24-idB)*1000;
    Driv0297CN[iQAM][24] = 10500 ;
    Driv0297CN[iQAM][25] =  9660 ;
    Driv0297CN[iQAM][26] =  8780 ;
    Driv0297CN[iQAM][27] =  7970 ;
    Driv0297CN[iQAM][28] =  7310 ;
    Driv0297CN[iQAM][29] =  6750 ;
    Driv0297CN[iQAM][30] =  6220 ;
    Driv0297CN[iQAM][31] =  5810 ;
    Driv0297CN[iQAM][32] =  5430 ;
    Driv0297CN[iQAM][33] =  5090 ;
    Driv0297CN[iQAM][34] =  4880 ;
    Driv0297CN[iQAM][35] =  4700 ;
    Driv0297CN[iQAM][36] =  4500 ;
    Driv0297CN[iQAM][37] =  4340 ;
    Driv0297CN[iQAM][38] =  4270 ;
    Driv0297CN[iQAM][39] =  4150 ;
    /* QAM = 256 */
    iQAM = 4 ;
    for (idB = 0 ; idB < 28 ; idB++)
        Driv0297CN[iQAM][idB] = 10500 + (28-idB)*1000;
    Driv0297CN[iQAM][28] = 10500 ;
    Driv0297CN[iQAM][29] =  9600 ;
    Driv0297CN[iQAM][30] =  9000 ;
    Driv0297CN[iQAM][31] =  8400 ;
    Driv0297CN[iQAM][32] =  7800 ;
    Driv0297CN[iQAM][33] =  7400 ;
    Driv0297CN[iQAM][34] =  7100 ;
    Driv0297CN[iQAM][35] =  6700 ;
    Driv0297CN[iQAM][36] =  6550 ;
    Driv0297CN[iQAM][37] =  6370 ;
    Driv0297CN[iQAM][38] =  6200 ;
    Driv0297CN[iQAM][39] =  6150 ;

#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297
    STTBX_Print(("%s\n", identity));
#endif
}

/*----------------------------------------------------
 FUNCTION      Drv0297_InitSearch
 ACTION        Set Params fields that are used by the search algorithm
 PARAMS IN
 PARAMS OUT
 RETURN        NONE
------------------------------------------------------*/
void Drv0297_InitSearch(STTUNER_tuner_instance_t *TunerInstance, D0297_StateBlock_t *StateBlock,
                        STTUNER_Modulation_t Modulation, int Frequency, int SymbolRate, STTUNER_Spectrum_t Spectrum,
                        BOOL ScanExact)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297
   const char *identity = "STTUNER drv0297.c Drv0297_InitSearch()";
#endif
    U32 BandWidth;
    ST_ErrorCode_t Error;
    TUNER_Status_t TunerStatus;

    /* Obtain current tuner status */
    Error = (TunerInstance->Driver->tuner_GetStatus)(TunerInstance->DrvHandle, &TunerStatus);

    /* Select closest bandwidth for tuner */ /* cast to U32 type to match function argument & eliminate compiler warning --SFS */
    Error = (TunerInstance->Driver->tuner_SetBandWidth)( TunerInstance->DrvHandle,
                                                        (U32)( Drv0297_CarrierWidth(SymbolRate, StateBlock->Params.RollOff) /1000 + 3000),
                                                        &BandWidth);

    /*
    --- Set Parameters
    */
    StateBlock->Params.Frequency    = Frequency;
    StateBlock->Params.SymbolRate   = SymbolRate;
    StateBlock->Params.TunerBW      = (long)BandWidth;
    StateBlock->Params.TunerStep    = (long)TunerStatus.TunerStep;
    StateBlock->Params.TunerIF      = (long)TunerStatus.IntermediateFrequency;
    StateBlock->Result.SignalType   = E297_NOAGC;
    StateBlock->Result.Frequency    = 0;
    StateBlock->Result.SymbolRate   = 0;
    StateBlock->SpectrumInversion   = Spectrum;
    StateBlock->Params.Direction    = E297_SCANUP;
    StateBlock->Params.State        = E297_NOAGC;
    StateBlock->J83                 = STTUNER_J83_A | STTUNER_J83_C;
    if (ScanExact)
    {
        StateBlock->ScanMode = DEF0297_CHANNEL;   /* Subranges are scanned in a zig-zag mode */
    }
    else
    {
        StateBlock->ScanMode = DEF0297_SCAN;      /* No Subranges */
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
            StateBlock->SweepRate   = STV0297_FAST_SWEEP_SPEED;
            break;

        case STTUNER_MOD_128QAM :
        case STTUNER_MOD_256QAM :
            StateBlock->QAMSize     = Modulation;           /* Set by user */
            StateBlock->SweepRate   = STV0297_SLOW_SWEEP_SPEED;
            break;

        case STTUNER_MOD_QAM :
        default:
            StateBlock->QAMSize     = STTUNER_MOD_64QAM;    /* Most Common Modulation for Scan */
            StateBlock->SweepRate   = STV0297_FAST_SWEEP_SPEED;
            break;
    }

    /*
    --- For low SR, we MUST divide Sweep by 2
    */
    if (SymbolRate <= STV0297_SYMBOLRATE_LEVEL)
    {
        StateBlock->SweepRate /= 2;
    }

    /*
    --- CO
    */
    StateBlock->CarrierOffset = DEF0297_CARRIEROFFSET_RATIO * 100;              /* in % */

    /*
    --- Set direction
    */
    if ( StateBlock->Params.Direction == E297_SCANUP )
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297
        STTBX_Print(("%s ==> SCANUP\n", identity));
#endif
        StateBlock->CarrierOffset *= -1;
        StateBlock->SweepRate     *= 1;
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297
        STTBX_Print(("%s ==> SCANDOWN\n", identity));
#endif
        StateBlock->CarrierOffset *= 1;
        StateBlock->SweepRate     *= -1;
    }

#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297
    STTBX_Print(("%s\n", identity));
#endif
}


/*----------------------------------------------------
 FUNCTION      Driv0297DemodSetting
 ACTION
 PARAMS IN     NONE
 PARAMS OUT    NONE
 RETURN        NONE
------------------------------------------------------*/
void Driv0297DemodSetting(STTUNER_IOREG_DeviceMap_t *DeviceMap_p, IOARCH_Handle_t IOHandle,
                          D0297_StateBlock_t *StateBlock_p, STTUNER_tuner_instance_t *TunerInstance_p, long Offset)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297
   const char *identity = "STTUNER drv0297.c Driv0297DemodSetting()";
#endif
    long long_tmp ;
    long ExtClk ;
    int  int_tmp ;
    U8   *p_tabi,*p_tabt=NULL;
    U8   addr, value;

#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297
    STTBX_Print(("%s Offset %d\n", identity, Offset));
#endif
    /*
    initial demodulator setting : init freq = IF + Offset - Fclock
    */
    ExtClk = DeviceMap_p->RegExtClk;                    /* unit Khz */
    long_tmp  = StateBlock_p->Params.TunerIF + Offset;  /* in KHz */
    long_tmp -= ExtClk;                                 /* in KHz */
    long_tmp *= 65536 ;
    long_tmp /= ExtClk;
    if(long_tmp > 65535) long_tmp = 65535 ;
    int_tmp = (int)long_tmp ;

    STTUNER_IOREG_SetRegister(DeviceMap_p, IOHandle, R0297_INITDEM_0,(int)(MAC0297_B0(int_tmp)));
    STTUNER_IOREG_SetRegister(DeviceMap_p, IOHandle, R0297_INITDEM_1,(int)(MAC0297_B1(int_tmp)));
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297
    STTBX_Print(("%s DEM_FQCY %d (Clk is %d KHz)\n", identity, int_tmp, ExtClk));
#endif

    /*
    --- Set Registers
    */
    switch(StateBlock_p->QAMSize)
    {
        case STTUNER_MOD_16QAM :
            p_tabi = (U8*)&QAM16[0];
            switch(TunerInstance_p->Driver->ID)
            {
#if defined( STTUNER_DRV_CAB_TUN_TDBE1) || defined(STTUNER_DRV_CAB_TUN_TDBE2) || defined (STTUNER_DRV_CAB_TUN_TDDE1)
                case STTUNER_TUNER_TDBE1:
                case STTUNER_TUNER_TDBE2:
                case STTUNER_TUNER_TDDE1:   p_tabt = (U8*)&QAM16_TDBE1[0];     break;
#endif                
#if defined (STTUNER_DRV_CAB_TUN_MT2030) ||defined (STTUNER_DRV_CAB_TUN_MT2040)                
                case STTUNER_TUNER_MT2030:
                case STTUNER_TUNER_MT2040:  p_tabt = (U8*)&QAM16_MT2040[0];    break;
#endif                
#ifdef STTUNER_DRV_CAB_TUN_MT2050                
                case STTUNER_TUNER_MT2050:  p_tabt = (U8*)&QAM16_MT2050[0];    break;
#endif 
#ifdef STTUNER_DRV_CAB_TUN_MT2060                
                case STTUNER_TUNER_MT2060:  p_tabt = (U8*)&QAM16_MT2060[0];    break;
#endif                 
#ifdef STTUNER_DRV_CAB_TUN_DCT7040               
                case STTUNER_TUNER_DCT7040: p_tabt = (U8*)&QAM16_DCT7040[0];   break;
#endif                 
#ifdef STTUNER_DRV_CAB_TUN_DCF8710                
                case STTUNER_TUNER_DCF8710: p_tabt = (U8*)&QAM16_DCF8710[0];   break;
#endif                 
#ifdef STTUNER_DRV_CAB_TUN_DCF8720                
                case STTUNER_TUNER_DCF8720: p_tabt = (U8*)&QAM16_DCF8720[0];   break;
#endif                 
                default:                    p_tabt = (U8*)&QAM16_DEFTUN[0];    break;
            }
            break;

        case STTUNER_MOD_32QAM :
            p_tabi = (U8*)&QAM32[0];
            switch(TunerInstance_p->Driver->ID)
            {
#if defined( STTUNER_DRV_CAB_TUN_TDBE1) || defined(STTUNER_DRV_CAB_TUN_TDBE2) || defined (STTUNER_DRV_CAB_TUN_TDDE1)                
                case STTUNER_TUNER_TDBE1:
                case STTUNER_TUNER_TDBE2:
                case STTUNER_TUNER_TDDE1:   p_tabt = (U8*)&QAM32_TDBE1[0];     break;
#endif
#if defined (STTUNER_DRV_CAB_TUN_MT2030) ||defined (STTUNER_DRV_CAB_TUN_MT2040)                
                case STTUNER_TUNER_MT2030:
                case STTUNER_TUNER_MT2040:  p_tabt = (U8*)&QAM32_MT2040[0];    break;
#endif 
#ifdef STTUNER_DRV_CAB_TUN_MT2050
                case STTUNER_TUNER_MT2050:  p_tabt = (U8*)&QAM32_MT2050[0];    break;
#endif      
#ifdef STTUNER_DRV_CAB_TUN_MT2060      
                case STTUNER_TUNER_MT2060:  p_tabt = (U8*)&QAM32_MT2060[0];    break;
#endif     
#ifdef STTUNER_DRV_CAB_TUN_DCT7040     
                case STTUNER_TUNER_DCT7040: p_tabt = (U8*)&QAM32_DCT7040[0];   break;
#endif      
#ifdef STTUNER_DRV_CAB_TUN_DCF8710     
                case STTUNER_TUNER_DCF8710: p_tabt = (U8*)&QAM32_DCF8710[0];   break;
#endif      
#ifdef STTUNER_DRV_CAB_TUN_DCF8720      
                case STTUNER_TUNER_DCF8720: p_tabt = (U8*)&QAM32_DCF8720[0];   break;
#endif      
                default:                    p_tabt = (U8*)&QAM32_DEFTUN[0];    break;
            }
            break;

        case STTUNER_MOD_64QAM :
            p_tabi = (U8*)&QAM64[0];
            switch(TunerInstance_p->Driver->ID)
            {
#if defined( STTUNER_DRV_CAB_TUN_TDBE1) || defined(STTUNER_DRV_CAB_TUN_TDBE2) || defined (STTUNER_DRV_CAB_TUN_TDDE1)           
                case STTUNER_TUNER_TDBE1:
                case STTUNER_TUNER_TDBE2:         
                case STTUNER_TUNER_TDDE1:   p_tabt = (U8*)&QAM64_TDBE1[0];     break;
#endif
#if defined (STTUNER_DRV_CAB_TUN_MT2030) ||defined (STTUNER_DRV_CAB_TUN_MT2040)              
                case STTUNER_TUNER_MT2030:
                case STTUNER_TUNER_MT2040:  p_tabt = (U8*)&QAM64_MT2040[0];    break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_MT2050
                case STTUNER_TUNER_MT2050:  p_tabt = (U8*)&QAM64_MT2050[0];    break;
#endif      
#ifdef STTUNER_DRV_CAB_TUN_MT2060      
                case STTUNER_TUNER_MT2060:  p_tabt = (U8*)&QAM64_MT2060[0];    break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_DCT7040
                case STTUNER_TUNER_DCT7040: p_tabt = (U8*)&QAM64_DCT7040[0];   break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_DCF8710
                case STTUNER_TUNER_DCF8710: p_tabt = (U8*)&QAM64_DCF8710[0];   break;
#endif      
#ifdef STTUNER_DRV_CAB_TUN_DCF8720      
                case STTUNER_TUNER_DCF8720: p_tabt = (U8*)&QAM64_DCF8720[0];   break;
#endif      
                default:                    p_tabt = (U8*)&QAM64_DEFTUN[0];    break;
            }
            break;

        case STTUNER_MOD_128QAM :
            p_tabi = (U8*)&QAM128[0];
            switch(TunerInstance_p->Driver->ID)
            {
 #if defined( STTUNER_DRV_CAB_TUN_TDBE1) || defined(STTUNER_DRV_CAB_TUN_TDBE2) || defined (STTUNER_DRV_CAB_TUN_TDDE1)               
                case STTUNER_TUNER_TDBE1:
                case STTUNER_TUNER_TDBE2:
                case STTUNER_TUNER_TDDE1:   p_tabt = (U8*)&QAM128_TDBE1[0];    break;
  #endif              
  #if defined (STTUNER_DRV_CAB_TUN_MT2030) ||defined (STTUNER_DRV_CAB_TUN_MT2040)              
                case STTUNER_TUNER_MT2030:            
                case STTUNER_TUNER_MT2040:  p_tabt = (U8*)&QAM128_MT2040[0];   break;
  #endif              
  #ifdef STTUNER_DRV_CAB_TUN_MT2050              
                case STTUNER_TUNER_MT2050:  p_tabt = (U8*)&QAM128_MT2050[0];   break;
  #endif              
  #ifdef STTUNER_DRV_CAB_TUN_MT2060              
                case STTUNER_TUNER_MT2060:  p_tabt = (U8*)&QAM128_MT2060[0];   break;
  #endif              
  #ifdef STTUNER_DRV_CAB_TUN_DCT7040              
                case STTUNER_TUNER_DCT7040: p_tabt = (U8*)&QAM128_DCT7040[0];   break;
  #endif              
  #ifdef STTUNER_DRV_CAB_TUN_DCF8710              
                case STTUNER_TUNER_DCF8710: p_tabt = (U8*)&QAM128_DCF8710[0];  break;
 #endif               
 #ifdef STTUNER_DRV_CAB_TUN_DCF8720               
                case STTUNER_TUNER_DCF8720: p_tabt = (U8*)&QAM128_DCF8720[0];  break;
 #endif               
                default:                    p_tabt = (U8*)&QAM128_DEFTUN[0];   break;
            }
            break;

        case STTUNER_MOD_256QAM :
            p_tabi = (U8*)&QAM256[0];
            switch(TunerInstance_p->Driver->ID)
            {
 #if defined( STTUNER_DRV_CAB_TUN_TDBE1) || defined(STTUNER_DRV_CAB_TUN_TDBE2) || defined (STTUNER_DRV_CAB_TUN_TDDE1)               
                case STTUNER_TUNER_TDBE1:
                case STTUNER_TUNER_TDBE2:
                case STTUNER_TUNER_TDDE1:   p_tabt = (U8*)&QAM256_TDBE1[0];    break;
 #endif              
 #if defined (STTUNER_DRV_CAB_TUN_MT2030) ||defined (STTUNER_DRV_CAB_TUN_MT2040)               
                case STTUNER_TUNER_MT2030:
                case STTUNER_TUNER_MT2040:  p_tabt = (U8*)&QAM256_MT2040[0];   break;
 #endif               
 #ifdef STTUNER_DRV_CAB_TUN_MT2050               
                case STTUNER_TUNER_MT2050:  p_tabt = (U8*)&QAM256_MT2050[0];   break;
 #endif               
 #ifdef STTUNER_DRV_CAB_TUN_MT2060               
                case STTUNER_TUNER_MT2060:  p_tabt = (U8*)&QAM256_MT2060[0];   break;
 #endif               
 #ifdef STTUNER_DRV_CAB_TUN_DCT7040               
                case STTUNER_TUNER_DCT7040: p_tabt = (U8*)&QAM256_DCT7040[0];  break;
 #endif               
 #ifdef STTUNER_DRV_CAB_TUN_DCF8710               
                case STTUNER_TUNER_DCF8710: p_tabt = (U8*)&QAM256_DCF8710[0];  break;
 #endif              
 #ifdef STTUNER_DRV_CAB_TUN_DCF8720              
                case STTUNER_TUNER_DCF8720: p_tabt = (U8*)&QAM256_DCF8720[0];  break;
 #endif              
                default:                    p_tabt = (U8*)&QAM256_DEFTUN[0];   break;
            }
            break;

        default:
            p_tabi = NULL;
            break;
    }

    if (p_tabi == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297
        STTBX_Print(("ERROR: modulation not defined\n"));
#endif
    }
    else
    {
        /* Write to registers */
        do
        {
            addr  = *p_tabi++;
            value = *p_tabi++;
            STTUNER_IOREG_SetRegister( DeviceMap_p, IOHandle,  addr, value);
        }
        while (*p_tabi!=0);
    }

    if (p_tabt == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297
        STTBX_Print(("ERROR: modulation not yet defined for this tuner\n"));
#endif
    }
    else
    {
        /* Write to registers */
        do
        {
            addr  = *p_tabt++;
            value = *p_tabt++;
            STTUNER_IOREG_SetRegister( DeviceMap_p, IOHandle,  addr, value);
        }
        while (*p_tabt!=0);
    }
}

/*----------------------------------------------------
 FUNCTION      Driv0297CNEstimator
 ACTION
 PARAMS IN     NONE
 PARAMS OUT    NONE
 RETURN        NONE
------------------------------------------------------*/
void Driv0297CNEstimator(STTUNER_IOREG_DeviceMap_t *DeviceMap_p, IOARCH_Handle_t IOHandle,
                         D0297_StateBlock_t *StateBlock_p, STTUNER_tuner_instance_t *TunerInstance_p,
                         int *Mean_p, int *CN_dB100_p, int *CN_Ratio_p)
{
/*
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297
   const char *identity = "STTUNER drv0297.c Driv0297CNEstimator()";
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
    int Driv0297CN_Max, Driv0297CN_Min;
    U8 temp[1];

    /**/
    switch (Reg0297_GetQAMSize(DeviceMap_p, IOHandle))
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
    the STV0297 noise estimation must be filtered. The used filter is :
    Estimation(n) = 63/64*Estimation(n-1) + 1/64*STV0297NoiseEstimation
    */
    for (i = 0 ; i < 100 ; i ++)
    {
        STV0297_DelayInMilliSec(1);                                     /* wait 1 ms */
        STTUNER_IOREG_GetContigousRegisters(DeviceMap_p, IOHandle, R0297_EQU_7, 2, temp);
        int_tmp = MAC0297_MAKEWORD(temp[1],temp[0]);
        if (i == 0) CNEstimation = int_tmp;
        else        CNEstimation = (63*CNEstimation)/64 + int_tmp/64;
    }
    /**/
    CNEstimatorOffset = 0;
    ComputedMean  = CNEstimation - CNEstimatorOffset  ; /* offset correction */
    CurrentMean   = Driv0297CN[iQAM][0] ;
    idB = 1 ;
    while (TRUE)
    {
        OldMean = CurrentMean ;
        CurrentMean = Driv0297CN[iQAM][idB] ;
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
    /**/
    *Mean_p = CNEstimation ;
    *CN_dB100_p   = Result ;

    Driv0297CN_Max = Driv0297CN[iQAM][0];
    Driv0297CN_Min = Driv0297CN[iQAM][39];
    *CN_Ratio_p   = 100 * CNEstimation;
    *CN_Ratio_p   /= (Driv0297CN_Max-Driv0297CN_Min);
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
    /**/
/*
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297
    STTBX_Print(("%s Mean %d CNdBx100 %d CNRatio %d\n", identity, *Mean_p, *CN_dB100_p, *CN_Ratio_p));
#endif
*/

}

/*----------------------------------------------------
 FUNCTION      Drv0297ApplicationSaturationComputation
 ACTION
 PARAMS IN     NONE
 PARAMS OUT    NONE
 RETURN        NONE
------------------------------------------------------*/
void Drv0297ApplicationSaturationComputation(int * _pSaturation)
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
 FUNCTION      Driv0297BertCount
 ACTION
 PARAMS IN     NONE
 PARAMS OUT    NONE
 RETURN        NONE
------------------------------------------------------*/
void Driv0297BertCount(STTUNER_IOREG_DeviceMap_t *DeviceMap_p, IOARCH_Handle_t IOHandle,
                       D0297_StateBlock_t *StateBlock_p, STTUNER_tuner_instance_t *TunerInstance_p,
                       int *_pBER_Cnt, int *_pBER)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297
    const char *identity = "STTUNER drv0297.c Driv0297BertCount()";
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
    ErrMode = STTUNER_IOREG_GetField(DeviceMap_p, IOHandle, F0297_ERR_MODE);
    if(ErrMode == 0)
    {
        /* rate mode */
        if(STTUNER_IOREG_GetField(DeviceMap_p, IOHandle, F0297_BERT_ON) == 0)
        {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297
            STTBX_Print(("===> Count is finished\n"));
#endif
            /* the previous count is finished */
            /* Get BER from demod */
            err_hi = STTUNER_IOREG_GetField(DeviceMap_p, IOHandle, F0297_ERRCOUNT_HI);
            err_lo = STTUNER_IOREG_GetField(DeviceMap_p, IOHandle, F0297_ERRCOUNT_LO);
            err_hi = STTUNER_IOREG_GetField(DeviceMap_p, IOHandle, F0297_ERRCOUNT_HI);
            err_lo = STTUNER_IOREG_GetField(DeviceMap_p, IOHandle, F0297_ERRCOUNT_LO);
            *_pBER_Cnt = err_lo + (err_hi <<8);
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297
            STTBX_Print(("===> _pBER_Cnt = %d\n", *_pBER_Cnt));
#endif
            BER = *_pBER_Cnt;

            NByte = STTUNER_IOREG_GetField(DeviceMap_p, IOHandle, F0297_NBYTE);
            BER *= 24414;

            for (ii = 0; ii < (2*NByte); ii++) BER >>= 1;

            if(STTUNER_IOREG_GetField(DeviceMap_p, IOHandle, F0297_ERR_SOURCE) == 0)
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
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297
            STTBX_Print(("===> BER = %d 10E-6\n", BER));
#endif

            _pBER[0] = *_pBER_Cnt;
            Drv0297ApplicationSaturationComputation(_pBER);
            /*
            --- Update NBYTE ?
            */
            if (_pBER[1] > 50)  /* Saturation */
            {
                int_tmp = STTUNER_IOREG_GetField(DeviceMap_p, IOHandle, F0297_NBYTE) ;
                int_tmp -= 1 ;
                STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297_NBYTE, int_tmp);
            } else if (_pBER[1] < 15)
            {
                int_tmp = STTUNER_IOREG_GetField(DeviceMap_p, IOHandle, F0297_NBYTE) ;
                int_tmp += 1;
                STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297_NBYTE, int_tmp);
            }
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297
            STTBX_Print(("===> Start a new count with NBYTE = %d \n", STTUNER_IOREG_GetField(DeviceMap_p, IOHandle, F0297_NBYTE)));
#endif
            STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297_BERT_ON,1) ; /* restart a new count */
        }
        else
        {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297
            STTBX_Print(("===> Keep latest BER value\n"));
#endif
        }
        _pBER[0] = *_pBER_Cnt;
    }

    /*
    --- Get Blk parameters
    */
    StateBlock_p->BlkCounter = Reg0297_GetBlkCounter(DeviceMap_p, IOHandle);
    StateBlock_p->CorrBlk    = Reg0297_GetCorrBlk(DeviceMap_p, IOHandle);
    StateBlock_p->UncorrBlk  = Reg0297_GetUncorrBlk(DeviceMap_p, IOHandle);

#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297
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

/*----------------------------------------------------
 FUNCTION      Driv0297DataSearch
 ACTION
                This routine performs a carrier lock trial with a given offset and sweep rate.
                The returned value is a flag (YES, NO) which indicates if the trial is
                successful or not. In case of lock, _pSignal->Frequency and _pSignal->SymbolRate
                are modifyed accordingly with the found carrier parameters.
 PARAMS IN     NONE
 PARAMS OUT    NONE
 RETURN        NONE
------------------------------------------------------*/
BOOL Driv0297DataSearch(STTUNER_IOREG_DeviceMap_t *DeviceMap_p, IOARCH_Handle_t IOHandle,
                        D0297_StateBlock_t *StateBlock_p, STTUNER_tuner_instance_t *TunerInstance_p)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297
    const char *identity = "STTUNER drv0297.c Driv0297DataSearch()";
    clock_t                 time_spend_register;
    clock_t                 time_start_lock, time_end_lock;
#endif
    long                    InitDemodOffset;
    STTUNER_Modulation_t    Modulation;

    long TimeOut           ;
    long LMS2TimeOut       ;
    long FECTimeOut        ;
    int  NbSymbols         ;
    int  Log2QAMSize=0     ;
    long AcqLoopsTime      ;
    int  SweepRate         ;
    int  CarrierOffset     ;
    long SymbolRate        ;
    long Frequency         ;
    STTUNER_Spectrum_t SpectrumInversion ;
    long DataSearchTime    ;
    int  WBAGCLock         ;
    int  int_tmp           ;
    int  initial_u         ;
    int  blind_u           ;
    int  u_threshold       ;
    int  AGC1Max, AGC1Min  ;
    int  AGC2Max, AGC2Min  ;
    int  AGC2SD            ;
    int  WBAGC_Iref        ;
    int  AGC2Threshold     ;
    BOOL EndOfSearch       ;
    BOOL DataFound         ;
    U8   DamplingFactor    ;
    int  LMS2TrackingLock  ;
    int  FECTrackingLock   ;
    int  AGCNoSignalThreshold;
#ifdef STV0297_DATA_SEARCH_A
#else
    int  WBAGC_Iref_Overflow;
    int  int_tmp1           ;
    int  CornerOccurence    ;
    int  TheoreticalCornerOccurence ;
#endif

    /*
    --- Set Parameters
    */
    DataFound         = FALSE;
    EndOfSearch       = FALSE;
    DataSearchTime    = 0;
    Frequency         = StateBlock_p->Params.Frequency;
    Modulation        = StateBlock_p->QAMSize;
    SymbolRate        = StateBlock_p->Params.SymbolRate;    /* in Baud/s */
    SweepRate         = StateBlock_p->SweepRate;            /* in 1/ms */
    CarrierOffset     = StateBlock_p->CarrierOffset;        /* in % */
    SpectrumInversion = StateBlock_p->SpectrumInversion;
    InitDemodOffset   = StateBlock_p->InitDemodOffset;

    /* AGCNoSignalThreshold = 10.23 * NoSignalThreshold (from GUI configuration file) */
    switch(TunerInstance_p->Driver->ID)
    {
        case STTUNER_TUNER_MT2050:
        case STTUNER_TUNER_MT2060:
        case STTUNER_TUNER_DCT7040: AGCNoSignalThreshold = 690;  break;

        case STTUNER_TUNER_TDBE1:
        case STTUNER_TUNER_TDBE2:
        case STTUNER_TUNER_TDDE1:
        case STTUNER_TUNER_MT2030:
        case STTUNER_TUNER_MT2040:
        case STTUNER_TUNER_DCF8710:
	case STTUNER_TUNER_DCF8720:
        default:                    AGCNoSignalThreshold = 600;  break;
    }

#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297
    STTBX_Print(("%s Fce %u KHz SR %u Baud/s Sweep %d\n", identity,
                Frequency,
                SymbolRate,
                SweepRate
                ));
    switch(SpectrumInversion)
    {
        case STTUNER_INVERSION_NONE:
            STTBX_Print(("%s CarrierOffset %d %% InitDemodOffset %d STTUNER_INVERSION_NONE\n", identity,
                        CarrierOffset/100,
                        InitDemodOffset
                        ));
            break;
        case STTUNER_INVERSION:
            STTBX_Print(("%s CarrierOffset %d %% InitDemodOffset %d STTUNER_INVERSION\n", identity,
                        CarrierOffset/100,
                        InitDemodOffset
                        ));
            break;
        case STTUNER_INVERSION_AUTO:
        default:
            STTBX_Print(("%s CarrierOffset %d %% InitDemodOffset %d STTUNER_INVERSION_AUTO\n", identity,
                        CarrierOffset/100,
                        InitDemodOffset
                        ));
            break;
    }
#endif

#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297
    time_start_lock = STOS_time_now();
#endif

    /*
    ===========================

    Time Out computation

    ===========================
    */
    /*
    AcqLoopsTime (in mSec ) : it is the time required for the
    lock of the carrier and timing loops
    */
    switch (Modulation)
    {
        case STTUNER_MOD_16QAM :
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297
            STTBX_Print(("%s QAM16\n", identity));
#endif
            Log2QAMSize = 4   ;
            break;
        case STTUNER_MOD_32QAM :
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297
    STTBX_Print(("%s QAM32\n", identity));
#endif
            Log2QAMSize = 5   ;
            break;
        case STTUNER_MOD_64QAM :
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297
            STTBX_Print(("%s QAM64\n", identity));
#endif
            Log2QAMSize = 6   ;
            break;
        case STTUNER_MOD_128QAM :
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297
            STTBX_Print(("%s QAM128\n", identity));
#endif
            Log2QAMSize = 7   ;
            break;
        case STTUNER_MOD_256QAM :
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297
            STTBX_Print(("%s QAM256\n", identity));
#endif
            Log2QAMSize = 8   ;
            break;
        default :
            break; 	
    }
    NbSymbols = 100000 ;                                    /* 100000 symbols */
    AcqLoopsTime = (NbSymbols/(SymbolRate/1000));
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
    }
    else
    {
        LMS2TimeOut = 0 ;
    }
    /*
    The equalizer timeout must be greater than
    the carrier and timing lock time
    */
    LMS2TimeOut += AcqLoopsTime ;
    /*
    even in case of low carrier offset,
    LMS2TimeOut will not be lower than a min
    which is linked with the QAM size.
    */
    switch (Modulation)
    {
        case STTUNER_MOD_16QAM :
        case STTUNER_MOD_32QAM :
        case STTUNER_MOD_64QAM :
            if( LMS2TimeOut < 100)
              LMS2TimeOut = 100;
            break;
        case STTUNER_MOD_128QAM :
        case STTUNER_MOD_256QAM :
            if( LMS2TimeOut < 200)
              LMS2TimeOut = 200;
            break;
        default :
            break; 	
    }
    /*
    FECTimeOut (in mSec) : it is the time necessary for the FEC to lock
                           in the worst case.
    */
    FECTimeOut = ((17+16)*(204*8))/(Log2QAMSize*(SymbolRate/1000)) ; /* in mSec */
    FECTimeOut += 10 ;

    /*
    $$$$ load register DEBUG
    */
#ifdef STV0297_RELOAD_DEMOD_REGISTER   /* [ */
    /*  SOFT_RESET */
    STTUNER_IOREG_SetRegister( DeviceMap_p, IOHandle,  R0297_CTRL_0, 1);
    STTUNER_IOREG_SetRegister( DeviceMap_p, IOHandle,  R0297_CTRL_0, 0);
    /* RESET_DI */
    STTUNER_IOREG_SetRegister( DeviceMap_p, IOHandle,  R0297_CTRL_1, 1);
    STTUNER_IOREG_SetRegister( DeviceMap_p, IOHandle,  R0297_CTRL_1, 0);
    /* Reload Registers */
    STTUNER_IOREG_Reset(DeviceMap_p, IOHandle, DefVal, Address);
#endif  /* ] STV0297_RELOAD_DEMOD_REGISTER */

    /*
    setting of the initial demodulator
    */
    Driv0297DemodSetting(DeviceMap_p, IOHandle, StateBlock_p, TunerInstance_p, InitDemodOffset);

    /* The initial demodulator can be used only if (system clock) / (IF freq) = 4/5). */
    /* If it is the case the following line can be uncommented */
    /* STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297_IN_DEMOD_EN, 1); */
    /*
    source selection : internal A/D
    */
    STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297_SOURCESEL,0) ;

    /*
    =====================

    WB AGC setting

    =====================
    */
    /*
    AGC2 & AGC1 value save
    */
    AGC2Max = STTUNER_IOREG_GetField(DeviceMap_p, IOHandle, F0297_AGC2MAX) ;
    AGC2Min = STTUNER_IOREG_GetField(DeviceMap_p, IOHandle, F0297_AGC2MIN) ;
    AGC1Max = STTUNER_IOREG_GetField(DeviceMap_p, IOHandle, F0297_AGC1MAX) ;
    AGC1Min = STTUNER_IOREG_GetField(DeviceMap_p, IOHandle, F0297_AGC1MIN) ;
    AGC2Threshold = STTUNER_IOREG_GetField(DeviceMap_p, IOHandle, F0297_AGC2_THRES) ;
    /*
    AGC ref is memorized
    */
    WBAGC_Iref = STTUNER_IOREG_GetField(DeviceMap_p, IOHandle, F0297_I_REF) ;
    /*
    Wide Band AGC freeze
    */
    STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297_WAGC_EN,0x00) ;
    /*
    Wide Band AGC clear
    */
    STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297_WAGC_CLR,0x01) ;
    /*
    Wide Band AGC unlock forcing :
    As WB AGC lock status is unknown, it is preferible to force the unlock,
    This state will not change until the WB AGC will be released. This is
    necessary to avoid improper PMF AGC starting.
    */
    STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297_WAGC_ACQ,0x00) ;
    /*
    Wide Band AGC agc2sd initialisation : 25%-range
    */
    STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297_AGC2SD_LO,0x00) ;
    STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297_AGC2SD_HI,0x01) ;
    AGC2SD = 256 ;
    /*
    Wide Band AGC2 nofreeze
    */
    STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297_FRZ2_CTRL,0x00) ;
    /*
    Wide Band AGC1 nofreeze
    */
    STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297_FRZ1_CTRL,0x00) ;

    /*
    =====================

    PMF AGC setting

    =====================
    */
    /*
    PMF AGC unlock forcing is enabled
    */
    STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297_PMFA_F_UNLOCK,0x01) ;
    /*
    PMF AGC accumulator reset
    */
    STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297_PMFA_ACC0,0x00) ;
    STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297_PMFA_ACC1,0x00) ;
    STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297_PMFA_ACC2,0x00) ;

    /*
    ===================================

    Symbol Timing Loop (STL) setting

    ===================================
    */

    /* phase clear */
    STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297_PHASE_CLR,0x01) ;

    /* STL integral path clear */
    STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297_ERR_CLR,0x01) ;

    /* STL integral path clear release */

    STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297_ERR_CLR,0x00) ;

    /* integral path enabled only after the WBAGC lock */
    STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297_ERR_EN,0x00) ;

    /* direct path immediatly enabled */
    STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297_PHASE_EN,0x00) ;

    /* --- For low SR, we MUST Change Dampling Factor 4..8 to 4..A */
    if (SymbolRate < STV0297_SYMBOLRATE_LEVEL)
    {
        /* Pass Dampling factor to 4..8/9/10 */
        switch (Modulation)
        {
            case STTUNER_MOD_16QAM :
            case STTUNER_MOD_32QAM :
            case STTUNER_MOD_64QAM :
            default:
                DamplingFactor = 0x49;
                break;
            case STTUNER_MOD_128QAM :
            case STTUNER_MOD_256QAM :
                DamplingFactor = 0x4A;
                break;
        }
    }
    else
    {
        /* Pass Dampling factor to 4..8/9/10 */
        switch (Modulation)
        {
            case STTUNER_MOD_16QAM :
            case STTUNER_MOD_32QAM :
            case STTUNER_MOD_64QAM :
            default:
                DamplingFactor = 0x48;
                break;
            case STTUNER_MOD_128QAM :
            case STTUNER_MOD_256QAM :
                DamplingFactor = 0x49;
                break;
        }
    }
    switch(TunerInstance_p->Driver->ID)
    {
        case STTUNER_TUNER_TDBE1:
        case STTUNER_TUNER_TDBE2:
        case STTUNER_TUNER_TDDE1:
        case STTUNER_TUNER_MT2030:
        case STTUNER_TUNER_MT2040:
        case STTUNER_TUNER_MT2050:
        case STTUNER_TUNER_MT2060:
        case STTUNER_TUNER_DCT7040:
        case STTUNER_TUNER_DCF8710:
	case STTUNER_TUNER_DCF8720:
        default:
            STTUNER_IOREG_SetRegister( DeviceMap_p, IOHandle,  R0297_CRL_1, DamplingFactor);
            break;
    }


    /*
    =======================================

    Carrier Recovery Loop (STL) setting

    =======================================
    */

    /* Frequency Offset Clear */

    STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297_APHASE_0,0x00) ;
    STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297_APHASE_1,0x00) ;
    STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297_APHASE_2,0x00) ;
    STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297_IPHASE_0,0x00) ;
    STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297_IPHASE_1,0x00) ;
    STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297_IPHASE_2,0x00) ;
    STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297_IPHASE_3,0x00) ;

    /* Sweep disable */

    STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297_SWEEP_EN,0) ;

    /*
    =====================

    Equalizer setting

    =====================
    */

    /* Equalizer values capture */

    u_threshold = STTUNER_IOREG_GetField(DeviceMap_p, IOHandle, F0297_U_THRESHOLD) ;
    initial_u   = STTUNER_IOREG_GetField(DeviceMap_p, IOHandle, F0297_INITIAL_U) ;
    blind_u     = STTUNER_IOREG_GetField(DeviceMap_p, IOHandle, F0297_BLIND_U) ;

    /* Equalizer clear */

    STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297_RESET_EQL,1) ;
    STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297_RESET_EQL,0) ;

    /* Equalizer values reload */

    STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297_U_THRESHOLD,u_threshold) ;
    STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297_INITIAL_U,initial_u) ;
    STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297_BLIND_U,blind_u) ;
    STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297_EQU_LMS2,0) ;  /* clear lms1, lms2 */

    /*
    ================

    FEC clear

    ================
    */

    /* deinterleaver&descrambler reset */

    STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297_RESET_DI,1) ;
    STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297_RESET_DI,0) ;

    /* deinterleaver unlock forcing */

    STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297_DI_UNLOCK,1) ;
    STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297_DI_UNLOCK,0) ;
    /*
    deinterleaver & descrambler status clear
    */
    STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297_DI_SY_EV,0) ;
    STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297_SYNC_EV,0)  ;
    /*
    number of bytes required to unlock : 3
    */
      STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297_TRKMODE,2) ;
    /*
    Reed-Salomon clear
    */
    STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297_RESET_RS,1) ;
    STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297_RESET_RS,0) ;

    /*
    ========================

    Parameters setting

    ========================
    */
    Reg0297_SetQAMSize(DeviceMap_p, IOHandle, Modulation);
    STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297_PHASE_CLR,0x00) ; /* STL unfreeze */
    Reg0297_SetSymbolRate(DeviceMap_p, IOHandle, SymbolRate);
    Reg0297_SetSweepRate(DeviceMap_p, IOHandle, SweepRate);
    STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297_PHASE_CLR,0x01) ; /* STL freeze (avoiding STV0297 symbol rate update) */
    Reg0297_SetFrequencyOffset(DeviceMap_p, IOHandle, CarrierOffset);
    STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297_SPEC_INV,(SpectrumInversion==STTUNER_INVERSION_NONE)?FALSE:TRUE);

    /*===================
     Acquisition sequence
    */

#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297
    time_spend_register = STOS_time_minus(STOS_time_now(), time_start_lock);
    time_start_lock = STOS_time_now();
#endif

    /* phase clear release */

    STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297_PHASE_CLR,0x00) ;

   /* AGC roll-off is set to 384 and acquisition threshold to 256
      in order to speed-up the lock acquisition */
   STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297_ROLL_HI,0x01) ; /* roll-off = 384 */
   STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297_ROLL_LO,0x80) ;
   STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297_ACQ_THRESH,0x08) ; /* threshold = 256 */

    /* Wide Band AGC enable */

    STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297_WAGC_EN,0x01) ;

    /* Wide Band AGC acquisition test */

    EndOfSearch = FALSE;
    TimeOut = DataSearchTime + 30  ; /* 30 ms time-out */
    while(!EndOfSearch)
    {
        STV0297_DelayInMilliSec(STV0297_DEMOD_WAITING_TIME) ;
        DataSearchTime += STV0297_DEMOD_WAITING_TIME ;
        WBAGCLock = STTUNER_IOREG_GetField(DeviceMap_p, IOHandle, F0297_WAGC_ACQ) ;
        if((WBAGCLock == 1)||(DataSearchTime > TimeOut))
        {
            EndOfSearch = TRUE;
        }
    }
    if(WBAGCLock == 0)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297
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
        StateBlock_p->Result.SignalType = E297_NOAGC;              /* AGC 1st Step Ko */
        STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297_AGC2SD_HI,0x12) ;
        return (DataFound);
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297
        STTBX_Print(("%s WBAGC LOCKED\n", identity));
#endif
        StateBlock_p->Result.SignalType = E297_AGCOK;              /* AGC 1st Step Ok */
    }

    int_tmp  = (STTUNER_IOREG_GetField(DeviceMap_p, IOHandle, F0297_AGC2SD_HI)<<8) + STTUNER_IOREG_GetField(DeviceMap_p, IOHandle, F0297_AGC2SD_LO);
    TimeOut = DataSearchTime + 30  ; /* 30 ms time-out */
    while((DataSearchTime < TimeOut )&&(int_tmp-AGC2SD > 100 ))
    {
        STV0297_DelayInMilliSec(STV0297_DEMOD_WAITING_TIME) ;
        DataSearchTime += STV0297_DEMOD_WAITING_TIME ;
        AGC2SD = int_tmp ;
        int_tmp  = (STTUNER_IOREG_GetField(DeviceMap_p, IOHandle, F0297_AGC2SD_HI)<<8) + STTUNER_IOREG_GetField(DeviceMap_p, IOHandle, F0297_AGC2SD_LO);
    }
    if(int_tmp > AGCNoSignalThreshold)
    {
        /* The AGC value is very low; maybe there is no signal.
           In order to check if there is or not signal, the reference is decreased
        */
        AGC2SD = 2000 ;
        STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297_I_REF,10) ;
        TimeOut = DataSearchTime + 30  ; /* 30 ms time-out */
        while((DataSearchTime < TimeOut )&&(int_tmp > AGCNoSignalThreshold))
        {
            STV0297_DelayInMilliSec(STV0297_DEMOD_WAITING_TIME) ;
            DataSearchTime += STV0297_DEMOD_WAITING_TIME ;
            AGC2SD = int_tmp ;
            int_tmp  = (STTUNER_IOREG_GetField(DeviceMap_p, IOHandle, F0297_AGC2SD_HI)<<8) + STTUNER_IOREG_GetField(DeviceMap_p, IOHandle, F0297_AGC2SD_LO);
        }
        /* the initial AGC ref is reloaded */
        STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297_I_REF,WBAGC_Iref) ;
        if(int_tmp > AGCNoSignalThreshold)
        {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297
            time_end_lock = STOS_time_now();
            STTBX_Print(("%s DEMOD CONF REGISTER >>> (%u ticks)(%u ms)\n", identity,
                        time_spend_register,
                        ((time_spend_register)*1000)/ST_GetClocksPerSecond()
                        ));
            STTBX_Print(("%s WBAGC LOCK TIME OUT !!! (SECOND STEP) (%u ticks)(%u ms)(level %d)\n", identity,
                        STOS_time_minus(time_end_lock, time_start_lock),
                        ((STOS_time_minus(time_end_lock, time_start_lock))*1000)/ST_GetClocksPerSecond(),
                        int_tmp
                        ));
#endif
            StateBlock_p->Result.SignalType = E297_AGCMAX;       /* AGC => No signal */
            STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297_ROLL_HI,0x12) ; /* roll-off = 384 */
            return (DataFound);
        }
        else
        {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297
            STTBX_Print(("%s WBAGC LOCKED (SECOND STEP)\n", identity));
#endif
            StateBlock_p->Result.SignalType = E297_AGCOK;               /* AGC Ok */
        }
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297
        STTBX_Print(("%s WBAGC LOCKED (FIRST LOOP)\n", identity ));
#endif
        StateBlock_p->Result.SignalType = E297_AGCOK;                   /* AGC Ok */
    }

    /* AGC is switched in  tracking mode :   */
    /* AGC roll-off set to 5000              */
    STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297_ROLL_HI,0x12) ; /* roll-off = 384 */

    /* corner detector on */
    STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297_EN_CORNER_DET,0x01) ;

    /* Sweep enable */
    STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297_SWEEP_EN,1) ;

    /* PMF AGC unlock forcing is disabled. */
    STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297_PMFA_F_UNLOCK,0x00) ;

    /* Waiting for a LMS2 tracking indication or a time-out; several continuing
       tracking indicators are requested to decide LMS2 convergence */
    EndOfSearch    = FALSE;
    TimeOut = DataSearchTime + LMS2TimeOut ;
    while (!EndOfSearch)
    {
        STV0297_DelayInMilliSec(STV0297_DEMOD_WAITING_TIME) ;
        DataSearchTime += STV0297_DEMOD_WAITING_TIME ;
        int_tmp = STTUNER_IOREG_GetField(DeviceMap_p, IOHandle, F0297_EQU_LMS2) ;
        if((int_tmp == 1)||(DataSearchTime > TimeOut))
        {
            EndOfSearch = TRUE ;
        }
    }
#ifdef STV0297_DATA_SEARCH_A
   /* begin STV0297_DATA_SEARCH_A */
    if(int_tmp == 0)
    {
        /* the equalizer has not converged */
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297
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
        StateBlock_p->Result.SignalType = E297_NOEQUALIZER;
        return (DataFound);
    }
    /*
    LMS2 was found : sweep is disabled
    */
    STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297_SWEEP_EN,0) ;
    /*
    corner detector off
    */
    STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297_EN_CORNER_DET,0x00) ;

    LMS2TrackingLock = 0 ;
    EndOfSearch    = FALSE ;
    TimeOut = DataSearchTime + 100 ;
    while (!EndOfSearch)
    {
        STV0297_DelayInMilliSec(STV0297_DEMOD_WAITING_TIME);
        DataSearchTime += STV0297_DEMOD_WAITING_TIME ;
        int_tmp = STTUNER_IOREG_GetField(DeviceMap_p, IOHandle, F0297_EQU_LMS2) ;
        if(int_tmp == 1)
        {
            LMS2TrackingLock += 1 ;
        }
        else
        {
            LMS2TrackingLock = 0 ;
        }
        if((LMS2TrackingLock >= STV0297_DEMOD_TRACKING_LOCK)||(DataSearchTime > TimeOut))
        {
            EndOfSearch = TRUE ;
        }
    }
    if (LMS2TrackingLock == STV0297_DEMOD_TRACKING_LOCK)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297
        STTBX_Print(("%s equalizer has converged\n", identity));
#endif
        StateBlock_p->Result.SignalType = E297_EQUALIZEROK;
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297
        STTBX_Print(("%s equalizer has not converged !!!\n", identity));
#endif
        StateBlock_p->Result.SignalType = E297_NOEQUALIZER;
        return (DataFound);
    }
    /*
    waiting for a FEC tracking indication or a time-out
    */
    EndOfSearch    = FALSE ;
    TimeOut = DataSearchTime + FECTimeOut ;

    while (!EndOfSearch)
    {
        STV0297_DelayInMilliSec(STV0297_DEMOD_WAITING_TIME) ;
        DataSearchTime +=STV0297_DEMOD_WAITING_TIME ;
        FECTrackingLock = STTUNER_IOREG_GetField(DeviceMap_p, IOHandle, F0297_SYNCSTATE) ;
        if((FECTrackingLock == 1)||(DataSearchTime > TimeOut))
        {
            EndOfSearch = TRUE ;
        }
    }

    if(FECTrackingLock == 1)
    {
        DataFound = TRUE;
    }

    if(!DataFound)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297
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
        StateBlock_p->Result.SignalType = E297_NODATA;                   /* Analog ??? */
        return (DataFound);
    }
    else
    {
        StateBlock_p->Result.SignalType = E297_DATAOK;                   /* Transport is Present */
    }
   /* end STV0297_DATA_SEARCH_A */
#else
    /* begin STV0297_DATA_SEARCH_B */
    TheoreticalCornerOccurence = 16384/(1 << Log2QAMSize) ; /* 16384/QAM = 4096*4/QAM */
    CornerOccurence = STTUNER_IOREG_GetField(DeviceMap_p, IOHandle, F0297_CORNER_RATE_HI)<<4
                    + STTUNER_IOREG_GetField(DeviceMap_p, IOHandle, F0297_CORNER_RATE_LO);
    int_tmp = STTUNER_IOREG_GetField(DeviceMap_p, IOHandle, F0297_EQU_LMS2) ;
    if(int_tmp == 0)
    {
        /* LMS2 = 0 : the equalizer is not locked.
           LMS1 is checked */
        int_tmp1 = STTUNER_IOREG_GetField(DeviceMap_p, IOHandle, F0297_EQU_LMS1) ;
        if((int_tmp1 == 1)&&(CornerOccurence >= (7*TheoreticalCornerOccurence/10)))
        {
            /* LMS1 is locked and the corner detection value is quite good.
               it is maybe a null frame : time is added and I_REF value is increased in order to add noise */
            switch (Modulation)
            {
                case STTUNER_MOD_16QAM:  WBAGC_Iref_Overflow = WBAGC_Iref + 34; break;
                case STTUNER_MOD_32QAM:  WBAGC_Iref_Overflow = WBAGC_Iref + 30; break;
                case STTUNER_MOD_64QAM:  WBAGC_Iref_Overflow = WBAGC_Iref + 26; break;
                case STTUNER_MOD_128QAM:
                case STTUNER_MOD_256QAM: WBAGC_Iref_Overflow = WBAGC_Iref + 16; break;
                default:                                                        break;
            }
            STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297_I_REF, WBAGC_Iref_Overflow) ;
            int_tmp1 = 0 ;
            int_tmp  = 0 ;

            while((int_tmp1 <= 300)&&(int_tmp == 0))
            {
                STV0297_DelayInMilliSec(30) ;
                int_tmp1 += 30 ;
                int_tmp = STTUNER_IOREG_GetField(DeviceMap_p, IOHandle, F0297_EQU_LMS2) ;
            }
            DataSearchTime += int_tmp1 ;
            /* IREF is smoothly decreased until the standard value */
            int_tmp = WBAGC_Iref_Overflow ;
            while (int_tmp > WBAGC_Iref)
            {
                int_tmp -= 2 ;
                STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297_I_REF, int_tmp) ;
                STV0297_DelayInMilliSec(2) ;
                DataSearchTime += 2 ;
            }
        }
        int_tmp = STTUNER_IOREG_GetField(DeviceMap_p, IOHandle, F0297_EQU_LMS2) ;
    }

    if(int_tmp == 0)
    {
        return (DataFound);
    }
    /* LMS2 was found : sweep is disabled */
    STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297_SWEEP_EN,0) ;

    /* corner detector off */
    STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297_EN_CORNER_DET,0x00) ;

    /*========================

      FEC acquisition

     ========================*/

    LMS2TrackingLock = 0 ;
    EndOfSearch      = FALSE ;
    TimeOut          = DataSearchTime + 100 ;
    while (!EndOfSearch)
    {
        STV0297_DelayInMilliSec(10) ;
        DataSearchTime += 10 ;
        int_tmp = STTUNER_IOREG_GetField(DeviceMap_p, IOHandle, F0297_EQU_LMS2) ;
        if(int_tmp == 1)
            LMS2TrackingLock += 1 ;
        else
            LMS2TrackingLock = 0 ;
        if((LMS2TrackingLock >= 5)||(DataSearchTime > TimeOut))
            EndOfSearch = TRUE ;
    }

    /* waiting for a FEC tracking indication or a time-out */
    EndOfSearch = FALSE ;
    FECTimeOut += 20 ;
    TimeOut = DataSearchTime + FECTimeOut ;

    while (!EndOfSearch)
    {
        STV0297_DelayInMilliSec(10) ;
        DataSearchTime +=10 ;
        FECTrackingLock = STTUNER_IOREG_GetField(DeviceMap_p, IOHandle, F0297_SYNCSTATE) ;
        if((FECTrackingLock == 1)||(DataSearchTime > TimeOut))
            EndOfSearch = TRUE ;
    }

    if(FECTrackingLock == 1)
        DataFound = TRUE;

    if(!DataFound)
    {
        return (DataFound);
    }

    /* end STV0297_DATA_SEARCH_B */
#endif
    /*
    direct path enabled only after AGC lock : this gives more chances to lock after a signal loss
    */
    STTUNER_IOREG_SetField(DeviceMap_p, IOHandle, F0297_PHASE_EN,0x00) ;
    /*
    symbol rate update
    carrier frequency update
    */
    StateBlock_p->Params.Frequency  = StateBlock_p->Result.Frequency;
    StateBlock_p->Params.Frequency  -= InitDemodOffset;
    StateBlock_p->Result.SymbolRate = SymbolRate * 1000;

#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297
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
 FUNCTION      Driv0297CarrierSearch
 ACTION
                This routine is the ENTRY POINT of the STV0297 driver.
                ======================================================

                This routine performs the carrier search following the user indications.
                This routine is the entry point of the STV0297 driver.

                The returned value is a flag (YES, NO) which indicates if the trial is
                successful or not. In case of lock, _pSignal->Frequency and _pSignal->SymbolRate
                are modifyed accordingly with the found carrier parameters.
 PARAMS IN     NONE
 PARAMS OUT    NONE
 RETURN        NONE
------------------------------------------------------*/
#ifdef STV0297_CARRIER_SEARCH_A
BOOL Driv0297CarrierSearch(STTUNER_IOREG_DeviceMap_t *DeviceMap_p, IOARCH_Handle_t IOHandle,
                           D0297_StateBlock_t *StateBlock_p, STTUNER_tuner_instance_t *TunerInstance_p)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297
    const char *identity = "STTUNER drv0297.c Driv0297CarrierSearch()";
    clock_t                 time_start_lock, time_end_lock, time_spend_tuner;
    int     ii=0;
#endif

    ST_ErrorCode_t          Error = ST_NO_ERROR;

    BOOL    DataFound;
    int     CarrierOffset;
    int     RangeOffset;
    int     SweepRate;
    int     int_tmp;
    int     InitDemodOffset;
    U8      temp[1];
    BOOL    InitDemod;
    STTUNER_Spectrum_t    SpectrumInversion;

    /*
    --- Get info from context
    */
    StateBlock_p->Result.SignalType = E297_NOCARRIER;            /* disable the signal monitoring */
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
    Reg0297_StopBlkCounter(DeviceMap_p, IOHandle);

    /*
    --- Test Input Param
    */
    if((( (StateBlock_p->SweepRate) > 0)&&((StateBlock_p->CarrierOffset) > 0))  ||
       (( (StateBlock_p->SweepRate) < 0)&&((StateBlock_p->CarrierOffset) < 0)))
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297
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

#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297
    STTBX_Print(("%s Fce %u KHz SR %u Baud/s Sweep %d\n", identity,
                StateBlock_p->Params.Frequency,
                StateBlock_p->Params.SymbolRate,
                SweepRate
                ));
    switch(SpectrumInversion)
    {
        case STTUNER_INVERSION_NONE:
            STTBX_Print(("%s CarrierOffset %d %% InitDemodOffset %d STTUNER_INVERSION_NONE\n", identity,
                        CarrierOffset/100,
                        InitDemodOffset
                        ));
            break;
        case STTUNER_INVERSION:
            STTBX_Print(("%s CarrierOffset %d %% InitDemodOffset %d STTUNER_INVERSION\n", identity,
                        CarrierOffset/100,
                        InitDemodOffset
                        ));
            break;
        case STTUNER_INVERSION_AUTO:
        default:
            STTBX_Print(("%s CarrierOffset %d %% InitDemodOffset %d STTUNER_INVERSION_AUTO\n", identity,
                        CarrierOffset/100,
                        InitDemodOffset
                        ));
            break;
    }
#endif

    /*
    tuner setting
    */
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297
    time_start_lock = STOS_time_now();
#endif
    Error = (TunerInstance_p->Driver->tuner_SetFrequency)(TunerInstance_p->DrvHandle, (U32)(StateBlock_p->Params.Frequency), (U32 *)(&(StateBlock_p->Result.Frequency)));
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297
    time_spend_tuner = STOS_time_minus(STOS_time_now(), time_start_lock);
#endif
    if( Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297
        STTBX_Print(("%s FAIL TO SET FREQUENCY (TUNER E297_NOCARRIER)\n", identity));
#endif
        StateBlock_p->Result.SignalType = E297_NOCARRIER;            /* Tuner is locked */
        return(DataFound) ;
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297
        STTBX_Print(("%s TUNER IS LOCKED at %d KHz (Request %d KHz) (TUNER E297_CARRIEROK)\n", identity,
            StateBlock_p->Result.Frequency,
            StateBlock_p->Params.Frequency
            ));
#endif
        StateBlock_p->Result.SignalType = E297_CARRIEROK;            /* Tuner is locked */
    }

    /*
    ---
    */
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297
    time_start_lock = STOS_time_now();
#endif

    RangeOffset = CarrierOffset ;

    /*
    init demod enabled ?
    */
    int_tmp = STTUNER_IOREG_GetField(DeviceMap_p, IOHandle, F0297_IN_DEMOD_EN);
    if ((int_tmp == 1)&&(RangeOffset <= -400))
    {
        InitDemod = TRUE ;
        RangeOffset = -400 ;
    }
    else
    {
        InitDemod = FALSE ;
    }

    if (StateBlock_p->ScanMode == DEF0297_SCAN)
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
    DataFound = Driv0297DataSearch(DeviceMap_p, IOHandle, StateBlock_p, TunerInstance_p);

    if(InitDemod)
    {
        /* initial demodulation is required */
        InitDemodOffset = 0 ;

        while ((!DataFound)&&((InitDemodOffset + RangeOffset) >= CarrierOffset))
        {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297
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
            DataFound = Driv0297DataSearch(DeviceMap_p, IOHandle, StateBlock_p, TunerInstance_p);
        }
    }

    /*
    --- Perfmeter
    */
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297
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
        StateBlock_p->Result.SignalType = E297_LOCKOK;
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297
        STTBX_Print(("%s ==> E297_LOCKOK\n", identity));
#endif
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297
        STTBX_Print(("%s ==> E297_xxxxxx (%d)!!!!!\n", identity, StateBlock_p->Result.SignalType));
#endif
    }

    /*
    --- Reset Err Counters*/
  
    temp[0]=0;    
    temp[1]=0;
    STTUNER_IOREG_SetContigousRegisters(DeviceMap_p, IOHandle, R0297_BERT_1, temp, 2);

    /*
    --- Start Blk counter
    */
    Reg0297_StartBlkCounter(DeviceMap_p, IOHandle);

    /*
    ---
    */

    return (DataFound) ;

}
/* end ifdef STV0297_CARRIER_SEARCH_A */
#else
/* begin ifdef STV0297_CARRIER_SEARCH_B */
BOOL Driv0297CarrierSearch(STTUNER_IOREG_DeviceMap_t *DeviceMap_p, IOARCH_Handle_t IOHandle,
                           D0297_StateBlock_t *StateBlock_p, STTUNER_tuner_instance_t *TunerInstance_p)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297
    const char *identity = "STTUNER drv0297.c Driv0297CarrierSearch()";
    clock_t                 time_start_lock, time_end_lock, time_spend_tuner;
    int     ii=0;
#endif

    ST_ErrorCode_t          Error = ST_NO_ERROR;

    BOOL    DataFound;
    int     CarrierOffset;
    int     RangeOffset;
    int     SweepRate;
    int     int_tmp;
    int     InitDemodOffset;
    U8      temp[1];

    BOOL    InitDemod;
    BOOL    ScanIsFinished;
    STTUNER_Spectrum_t    SpectrumInversion;

    /*
    --- Get info from context
    */
    StateBlock_p->Result.SignalType = E297_NOCARRIER;            /* disable the signal monitoring */
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
    Reg0297_StopBlkCounter(DeviceMap_p, IOHandle);

    /*
    --- Test Input Param
    */
    if((( (StateBlock_p->SweepRate) > 0)&&((StateBlock_p->CarrierOffset) > 0))  ||
       (( (StateBlock_p->SweepRate) < 0)&&((StateBlock_p->CarrierOffset) < 0)))
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297
        STTBX_Print(("%s BAD INPUT PARAMETERS!!!!\n", identity
                    ));
#endif
        return(DataFound) ;
    }
    SweepRate   = StateBlock_p->SweepRate ;
    RangeOffset = StateBlock_p->CarrierOffset ;
    if(StateBlock_p->CarrierOffset > 0 )
        CarrierOffset = StateBlock_p->CarrierOffset ;
    else
        CarrierOffset = -StateBlock_p->CarrierOffset ;

#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297
    STTBX_Print(("%s Fce %u KHz SR %u Baud/s Sweep %d\n", identity,
                StateBlock_p->Params.Frequency,
                StateBlock_p->Params.SymbolRate,
                SweepRate
                ));
    switch(SpectrumInversion)
    {
        case STTUNER_INVERSION_NONE:
            STTBX_Print(("%s CarrierOffset %d %% InitDemodOffset %d STTUNER_INVERSION_NONE\n", identity,
                        CarrierOffset/100,
                        InitDemodOffset
                        ));
            break;
        case STTUNER_INVERSION:
            STTBX_Print(("%s CarrierOffset %d %% InitDemodOffset %d STTUNER_INVERSION\n", identity,
                        CarrierOffset/100,
                        InitDemodOffset
                        ));
            break;
        case STTUNER_INVERSION_AUTO:
        default:
            STTBX_Print(("%s CarrierOffset %d %% InitDemodOffset %d STTUNER_INVERSION_AUTO\n", identity,
                        CarrierOffset/100,
                        InitDemodOffset
                        ));
            break;
    }
#endif

    /*
    tuner setting
    */
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297
    time_start_lock = STOS_time_now();
#endif
    Error = (TunerInstance_p->Driver->tuner_SetFrequency)(TunerInstance_p->DrvHandle, (U32)(StateBlock_p->Params.Frequency), (U32 *)(&(StateBlock_p->Result.Frequency)));
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297
    time_spend_tuner = STOS_time_minus(STOS_time_now(), time_start_lock);
#endif
    if( Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297
        STTBX_Print(("%s FAIL TO SET FREQUENCY (TUNER E297_NOCARRIER)\n", identity));
#endif
        StateBlock_p->Result.SignalType = E297_NOCARRIER;            /* Tuner is locked */
        return(DataFound) ;
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297
        STTBX_Print(("%s TUNER IS LOCKED at %d KHz (Request %d KHz) (TUNER E297_CARRIEROK)\n", identity,
            StateBlock_p->Result.Frequency,
            StateBlock_p->Params.Frequency
            ));
#endif
        StateBlock_p->Result.SignalType = E297_CARRIEROK;            /* Tuner is locked */
    }

    /*
    ---
    */
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297
    time_start_lock = STOS_time_now();
#endif

    /*
    init demod enabled ?
    */
    int_tmp = STTUNER_IOREG_GetField(DeviceMap_p, IOHandle, F0297_IN_DEMOD_EN);
    InitDemod = FALSE ;
    if (int_tmp == 1)
    {
        if((SweepRate > 0)&&(RangeOffset <= -400))
        {
            InitDemod = TRUE ;
            RangeOffset = -400 ;
        }
        else if((SweepRate < 0)&&(RangeOffset >=  400))
        {
            InitDemod = TRUE ;
            RangeOffset = 400 ;
        }
    }

    if (StateBlock_p->ScanMode == DEF0297_SCAN)
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
    DataFound = Driv0297DataSearch(DeviceMap_p, IOHandle, StateBlock_p, TunerInstance_p);

    if(InitDemod)
    {
        /* initial demodulation is required */
        InitDemodOffset = 0 ;
        ScanIsFinished  = FALSE ;

        while (!DataFound && !ScanIsFinished)
        {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297
            STTBX_Print(("%s TUNER loop # %d InitDemodOffset %d RangeOffset %d CarrierOffset %d %%\n", identity, ii++,
                InitDemodOffset,
                RangeOffset,
                CarrierOffset/100
                ));
#endif
            /* the subranges are scanned in a zig-zag mode around the initial subrange */
            SweepRate       = -SweepRate;
            InitDemodOffset = -InitDemodOffset;
            RangeOffset     = -RangeOffset;
            if((StateBlock_p->SweepRate)*RangeOffset > 0)
                InitDemodOffset += RangeOffset ;
            if((InitDemodOffset <= CarrierOffset)&&(InitDemodOffset >= -CarrierOffset))
            {
                /* [InitDemod - RangeOffset, InitDemod + RangeOffset] range */
                StateBlock_p->InitDemodOffset = (InitDemodOffset*((StateBlock_p->Params.SymbolRate)/1000))/10000;
                StateBlock_p->CarrierOffset = (110*RangeOffset)/100;
                StateBlock_p->SweepRate = SweepRate;
                StateBlock_p->SpectrumInversion = SpectrumInversion;
                DataFound = Driv0297DataSearch(DeviceMap_p, IOHandle, StateBlock_p, TunerInstance_p);
            }
            else
            {
                /* the frequency range is explored */
                ScanIsFinished = TRUE ;
            }
        }
    }

    /*
    --- Perfmeter
    */
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297
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
        StateBlock_p->Result.SignalType = E297_LOCKOK;
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297
        STTBX_Print(("%s ==> E297_LOCKOK\n", identity));
#endif
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0297
        STTBX_Print(("%s ==> E297_xxxxxx (%d)!!!!!\n", identity, StateBlock_p->Result.SignalType));
#endif
    }

    /*
    --- Reset Err Counters
    */

    temp[0]=0;
    temp[1]=0;
    STTUNER_IOREG_SetContigousRegisters(DeviceMap_p, IOHandle, R0297_BERT_1, temp, 2);

    /*
    --- Start Blk counter
    */
    Reg0297_StartBlkCounter(DeviceMap_p, IOHandle);

    /*
    ---
    */

    return (DataFound) ;

}
/* end ifdef STV0297_CARRIER_SEARCH_B */
#endif

/* End of drv0297.c */
