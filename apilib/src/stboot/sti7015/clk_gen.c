/*****************************************************************************
File Name   : clk_gen.c

Description : Clock Gen configuration.

COPYRIGHT (C) STMicroelectronics 2002.

*****************************************************************************/

/* Includes --------------------------------------------------------------- */

#include <stdio.h>
#include <math.h>
#include <stlite.h>             /* STLite includes */

#include "stdevice.h"           /* Device includes */
#include "stddefs.h"

#include "stsys.h"              /* STAPI includes */
#include "stcommon.h"

#include "cfg_reg.h"            /* Localy used registers */
#include "bereg.h"              /* Localy used registers */

#include "clk_gen.h"

/* Private types/constants ------------------------------------------------ */
/* Check the existance of the following frequencies in the FreqSynthElements array */
#if defined(ST_7015)
   #define DEFAULT_LMC_CLK              75000000
   #define DEFAULT_PIX_CLK             148500000        /* for HD */
   #define DEFAULT_AUD_CLK              63000000
   #define DEFAULT_DEC_CLK              81000000        /* Decompression */ /* SYNTH5 Stoped */
   #define DEFAULT_DISP_CLK             81000000
   #define DEFAULT_DENC_CLK             27000000
   #define DEFAULT_PCM_CLK              12288000        /* 256 x 48kHz */
#elif defined(ST_7020)
  #define DEFAULT_LMC_CLK              100000000        /* Covers blitter _and_ MPEG memory on cut 2.0 */
  #define DEFAULT_PIX_CLK              148500000        /* for HD; obtainable from internal SYNTH on cut 2.0*/
  #define DEFAULT_AUD_CLK              140000000        /* MMDSP clock is internally divided by 2 in 7020 */
  #define DEFAULT_PIPE_CLK              74000000
  #define DEFAULT_ADISP_CLK             72000000
  #define DEFAULT_DISP_CLK              87000000
  #define DEFAULT_AUX_CLK               27000000
  #define DEFAULT_PCM_CLK               12288000        /* 256 x 48kHz */
#else
   #error ERROR: invalid DVD_BACKEND defined
#endif

typedef struct {        /* Freq synth configuration */
    U32 Fin;
    U32 Fout;
    U32 ndiv;
    U32 sdiv;
    U32 md;
    U32 pe;
} FreqSynthElements_t;

/* Private variables ------------------------------------------------------ */
#if defined(ST_7015)
static FreqSynthElements_t FreqSynthElements[] = {  /* Freq synth configuration depending on frequency */
    {27000000,  9818181, 0x1, 0x4, 0x16, 0x7fff},   /* 480P23976 */
    {27000000, 19636362, 0x1, 0x3, 0x16, 0x7fff},   /* double of previous output frequency */
    {27000000,  9828000, 0x1, 0x4, 0x15, 0x02d0},   /* 480P24000 */
    {27000000, 19656000, 0x1, 0x3, 0x15, 0x02d0},   /* double of previous output frequency */
    {27000000, 10800000, 0x1, 0x4, 0x13, 0x0000},   /* 480P23976 */
    {27000000, 21600000, 0x1, 0x3, 0x13, 0x0000},   /* double of previous output frequency */
    {27000000, 10810800, 0x1, 0x4, 0x13, 0x028e},   /* 480P24000 */
    {27000000, 21621600, 0x1, 0x3, 0x13, 0x028e},   /* double of previous output frequency */
    {27000000, 12272727, 0x1, 0x4, 0x11, 0x3333},   /* 480I59940 */
    {27000000, 24545454, 0x1, 0x3, 0x11, 0x3333},   /* 480P59940 (double of previous)*/
    {27000000, 49090908, 0x1, 0x2, 0x11, 0x3333},   /* double of previous output frequency */
    {27000000, 12285000, 0x1, 0x4, 0x11, 0x3573},   /* 480I60000;480P30000 */
    {27000000, 24570000, 0x1, 0x3, 0x11, 0x3573},   /* 480P60000 (double of previous) */
    {27000000, 49140000, 0x1, 0x2, 0x11, 0x3573},   /* double of previous output frequency */
    {27000000, 12288000, 0x1, 0x4, 0x11, 0x3600},
    {27000000, 24576000, 0x1, 0x3, 0x11, 0x3600},   /* double of previous output frequency */
    {27000000, 13500000, 0x1, 0x4, 0x0f, 0x0000},   /* 480I59940;480P29970;576I50000*/
    {27000000, 27000000, 0x1, 0x3, 0x0f, 0x0000},   /* 480P59940 (double of previous) */
    {27000000, 54000000, 0x1, 0x2, 0x0f, 0x0000},   /* double of previous output frequency */
    {27000000, 13513500, 0x1, 0x3, 0x1f, 0x0417},   /* 480I60000;480P30000 */
    {27000000, 27027000, 0x1, 0x2, 0x1f, 0x0417},   /* 480P60000;480P29970 (double of previous) */
    {27000000, 54054000, 0x1, 0x1, 0x1f, 0x0417},   /* double of previous output frequency */
    {27000000, 14750000, 0x1, 0x3, 0x1d, 0x5b1e},   /* 576I50000 */
    {27000000, 29500000, 0x1, 0x2, 0x1d, 0x5b1e},   /* double of previous output frequency */
    {27000000, 29670329, 0x1, 0x2, 0x1d, 0x70a3},   /* 720P23976 */
    {27000000, 59340658, 0x1, 0x1, 0x1d, 0x70a3},   /* double of previous output frequency */
    {27000000, 29700000, 0x1, 0x2, 0x1d, 0x745d},   /* 720P24000 */
    {27000000, 59400000, 0x1, 0x1, 0x1d, 0x745d},   /* double of previous output frequency */
    {27000000, 37087912, 0x1, 0x2, 0x17, 0x5a1c},   /* 720P29970 */
    {27000000, 74175824, 0x1, 0x1, 0x17, 0x5a1c},   /* 1080I59940;1080P29970;1080P23976;1035I59940;720P59940 (double of previous) */
    {27000000,148351648, 0x1, 0x0, 0x17, 0x5a1c},   /* 1080P59940 (double of previous)*/
    {27000000, 37125000, 0x1, 0x2, 0x17, 0x5d17},   /* 720P30000 */
    {27000000, 74250000, 0x1, 0x1, 0x17, 0x5d17},   /* 1080I60000;1080I50000;1080P30000;1080P25000;1080P24000;1035I60000;720P60000;720P50000;1152I50000 (double of previous) */
    {27000000,148500000, 0x1, 0x0, 0x17, 0x5d17},   /* 1080P60000;1080P50000 (double of previous) */
    {27000000, 63000000, 0x1, 0x1, 0x1b, 0x4924},
    {27000000,126000000, 0x1, 0x0, 0x1b, 0x4924},   /* double of previous output frequency */
    {27000000, 81000000, 0x1, 0x1, 0x15, 0x5555},
    {27000000,162000000, 0x1, 0x0, 0x15, 0x5555},   /* double of previous output frequency */


    {27000000, 40000000, 0x1, 0x2, 0x15, 0x3333},
    {27000000, 45000000, 0x1, 0x2, 0x13, 0x6666},
    {27000000, 50000000, 0x1, 0x2, 0x11, 0x5c28},
    {27000000, 55000000, 0x1, 0x1, 0x1f, 0x4a79},
    {27000000, 60000000, 0x1, 0x1, 0x1c, 0x1999},
    {27000000, 65000000, 0x1, 0x1, 0x1a, 0x352b},
    {27000000, 70000000, 0x1, 0x1, 0x18, 0x283a},
    {27000000, 75000000, 0x1, 0x1, 0x17, 0x7ae1},
    {27000000, 80000000, 0x1, 0x1, 0x15, 0x3333},
    {27000000, 85000000, 0x1, 0x1, 0x14, 0x55d5},
    {27000000, 90000000, 0x1, 0x1, 0x13, 0x6666},
    {27000000, 95000000, 0x1, 0x1, 0x12, 0x67bf},
    {27000000,100000000, 0x1, 0x1, 0x11, 0x5c28},
    {27000000,105000000, 0x1, 0x1, 0x10, 0x457c},
    {27000000,110000000, 0x1, 0x0, 0x1f, 0x4a79},
    {27000000,115000000, 0x1, 0x0, 0x1e, 0x7952},
    {27000000,120000000, 0x1, 0x0, 0x1c, 0x1999},
    {27000000,125000000, 0x1, 0x0, 0x1b, 0x2d0e},
    {27000000,130000000, 0x1, 0x0, 0x1a, 0x352b}
};
#elif defined(ST_7020) /* Params for all synths except SYNTH8 (PCMCLK) have been changed on cut 2.0 */
static FreqSynthElements_t FreqSynthElements[] = {
    /* Freq synth configuration depending on frequency */
    /* fin     fout      ndiv,sdiv,   md, pe    */
    {27000000, 9818181, 0x1, 0x4, 0x5, 0x4000},
    {27000000, 9828000, 0x1, 0x4, 0x5, 0x40b4},
    {27000000, 10800000, 0x1, 0x4, 0x4, 0x0},
    {27000000, 10810800, 0x1, 0x4, 0x4, 0xa4},
    {27000000, 12272727, 0x1, 0x4, 0x4, 0x4ccd},
    {27000000, 12285000, 0x1, 0x4, 0x4, 0x4d5d},
    {27000000, 12288000, 0x1, 0x4, 0x4, 0x4d80},
    {27000000, 13500000, 0x1, 0x3, 0x7, 0x0},
    {27000000, 13513500, 0x1, 0x3, 0x7, 0x106},
    {27000000, 14750000, 0x1, 0x3, 0x7, 0x56c8},
    {27000000, 19636362, 0x1, 0x3, 0x5, 0x4000},
    {27000000, 19656000, 0x1, 0x3, 0x5, 0x40b4},
    {27000000, 21600000, 0x1, 0x3, 0x4, 0x0},
    {27000000, 21621600, 0x1, 0x3, 0x4, 0xa4},
    {27000000, 24545454, 0x1, 0x3, 0x4, 0x4ccd},
    {27000000, 24570000, 0x1, 0x3, 0x4, 0x4d5d},
    {27000000, 24576000, 0x1, 0x3, 0x4, 0x4d80},
    {27000000, 27000000, 0x1, 0x2, 0x7, 0x0},
    {27000000, 27027000, 0x1, 0x2, 0x7, 0x106},
    {27000000, 29500000, 0x1, 0x2, 0x7, 0x56c8},
    {27000000, 29670329, 0x1, 0x2, 0x7, 0x5c29},
    {27000000, 29700000, 0x1, 0x2, 0x7, 0x5d17},
    {27000000, 37087912, 0x1, 0x2, 0x5, 0x1687},
    {27000000, 37125000, 0x1, 0x2, 0x5, 0x1746},
    {27000000, 40000000, 0x1, 0x2, 0x5, 0x4ccd},
    {27000000, 45000000, 0x1, 0x2, 0x4, 0x199a},
    {27000000, 49090908, 0x1, 0x2, 0x4, 0x4ccd},
    {27000000, 49140000, 0x1, 0x2, 0x4, 0x4d5d},
    {27000000, 50000000, 0x1, 0x2, 0x4, 0x570a},
    {27000000, 54000000, 0x1, 0x1, 0x7, 0x0},
    {27000000, 54054000, 0x1, 0x1, 0x7, 0x106},
    {27000000, 55000000, 0x1, 0x1, 0x7, 0x129e},
    {27000000, 59340658, 0x1, 0x1, 0x7, 0x5c29},
    {27000000, 59400000, 0x1, 0x1, 0x7, 0x5d17},
    {27000000, 60000000, 0x1, 0x1, 0x7, 0x6666},
    {27000000, 63000000, 0x1, 0x1, 0x6, 0x1249},
    {27000000, 65000000, 0x1, 0x1, 0x6, 0x2d4b},
    {27000000, 70000000, 0x1, 0x1, 0x6, 0x6a0f},
    {27000000, 72000000, 0x1, 0x1, 0x5, 0x0},
    {27000000, 74000000, 0x1, 0x1, 0x5, 0x14c2},
    {27000000, 74175824, 0x1, 0x1, 0x5, 0x1687},
    {27000000, 74250000, 0x1, 0x1, 0x5, 0x1746},
    {27000000, 75000000, 0x1, 0x1, 0x5, 0x1eb8},
    {27000000, 80000000, 0x1, 0x1, 0x5, 0x4ccd},
    {27000000, 81000000, 0x1, 0x1, 0x5, 0x5555},
    {27000000, 85000000, 0x1, 0x1, 0x5, 0x7575},
    {27000000, 86400000, 0x1, 0x1, 0x4, 0x0},
    {27000000, 87000000, 0x1, 0x1, 0x4, 0x46a},
    {27000000, 90000000, 0x1, 0x1, 0x4, 0x199a},
    {27000000, 95000000, 0x1, 0x1, 0x4, 0x39f0},
    {27000000, 100000000, 0x1, 0x1, 0x4, 0x570a},
    {27000000, 105000000, 0x1, 0x1, 0x4, 0x715f},
    {27000000, 108000000, 0x1, 0x0, 0x7, 0x0},
    {27000000, 110000000, 0x1, 0x0, 0x7, 0x129e},
    {27000000, 115000000, 0x1, 0x0, 0x7, 0x3e55},
    {27000000, 120000000, 0x1, 0x0, 0x7, 0x6666},
    {27000000, 125000000, 0x1, 0x0, 0x6, 0xb44},
    {27000000, 126000000, 0x1, 0x0, 0x6, 0x1249},
    {27000000, 130000000, 0x1, 0x0, 0x6, 0x2d4b},
    {27000000, 140000000, 0x1, 0x0, 0x6, 0x6a0f},
    {27000000, 148351648, 0x1, 0x0, 0x5, 0x1687},
    {27000000, 148500000, 0x1, 0x0, 0x5, 0x1746},
    {27000000, 162000000, 0x1, 0x0, 0x5, 0x5555}
};
static FreqSynthElements_t AudioFreqSynthElements[] = {
    /* Freq synth configuration depending on frequency */
    /* fin     fout      ndiv,sdiv,   md, pe    */
    {27000000,   9818181, 0x1, 0x4, 0x1d, 0x3fff},
    {27000000,   9828000, 0x1, 0x4, 0x1d, 0x40b4},
    {27000000,  10800000, 0x1, 0x4, 0x1c, 0x0000},
    {27000000,  10810800, 0x1, 0x4, 0x1c, 0x00a3},
    {27000000,  12272727, 0x1, 0x4, 0x1c, 0x4ccc},
    {27000000,  12285000, 0x1, 0x4, 0x1c, 0x4d5c},
    {27000000,  12288000, 0x1, 0x4, 0x1c, 0x4d80},
    {27000000,  13500000, 0x1, 0x4, 0x1b, 0x0000},
    {27000000,  13513500, 0x1, 0x3, 0x1f, 0x0105},
    {27000000,  14750000, 0x1, 0x3, 0x1f, 0x56c7},
    {27000000,  19636362, 0x1, 0x3, 0x1d, 0x3fff},
    {27000000,  19656000, 0x1, 0x3, 0x1d, 0x40b4},
    {27000000,  21600000, 0x1, 0x3, 0x1c, 0x0000},
    {27000000,  21621600, 0x1, 0x3, 0x1c, 0x00a3},
    {27000000,  24545454, 0x1, 0x3, 0x1c, 0x4ccc},
    {27000000,  24570000, 0x1, 0x3, 0x1c, 0x4d5c},
    {27000000,  24576000, 0x1, 0x3, 0x1c, 0x4d80},
    {27000000,  27000000, 0x1, 0x3, 0x1b, 0x0000},
    {27000000,  27027000, 0x1, 0x2, 0x1f, 0x0105},
    {27000000,  29500000, 0x1, 0x2, 0x1f, 0x56c7},
    {27000000,  29670329, 0x1, 0x2, 0x1f, 0x5c28},
    {27000000,  29700000, 0x1, 0x2, 0x1f, 0x5d17},
    {27000000,  37087912, 0x1, 0x2, 0x1d, 0x1687},
    {27000000,  37125000, 0x1, 0x2, 0x1d, 0x1745},
    {27000000,  40000000, 0x1, 0x2, 0x1d, 0x4ccc},
    {27000000,  45000000, 0x1, 0x2, 0x1c, 0x1999},
    {27000000,  49090908, 0x1, 0x2, 0x1c, 0x4ccc},
    {27000000,  49140000, 0x1, 0x2, 0x1c, 0x4d5c},
    {27000000,  50000000, 0x1, 0x2, 0x1c, 0x570a},
    {27000000,  54000000, 0x1, 0x2, 0x1b, 0x0000},
    {27000000,  54054000, 0x1, 0x1, 0x1f, 0x0105},
    {27000000,  55000000, 0x1, 0x1, 0x1f, 0x129e},
    {27000000,  59340658, 0x1, 0x1, 0x1f, 0x5c28},
    {27000000,  59400000, 0x1, 0x1, 0x1f, 0x5d17},
    {27000000,  60000000, 0x1, 0x1, 0x1f, 0x6666},
    {27000000,  63000000, 0x1, 0x1, 0x1e, 0x1249},
    {27000000,  65000000, 0x1, 0x1, 0x1e, 0x2d4a},
    {27000000,  70000000, 0x1, 0x1, 0x1e, 0x6a0e},
    {27000000,  72000000, 0x1, 0x1, 0x1d, 0x0000},
    {27000000,  74175824, 0x1, 0x1, 0x1d, 0x1687},
    {27000000,  74250000, 0x1, 0x1, 0x1d, 0x1745},
    {27000000,  75000000, 0x1, 0x1, 0x1d, 0x1eb8},
    {27000000,  80000000, 0x1, 0x1, 0x1d, 0x4ccc},
    {27000000,  81000000, 0x1, 0x1, 0x1d, 0x5555},
    {27000000,  85000000, 0x1, 0x1, 0x1d, 0x7575},
    {27000000,  86400000, 0x1, 0x1, 0x1c, 0x0000},
    {27000000,  90000000, 0x1, 0x1, 0x1c, 0x1999},
    {27000000,  95000000, 0x1, 0x1, 0x1c, 0x39ef},
    {27000000, 100000000, 0x1, 0x1, 0x1c, 0x570a},
    {27000000, 105000000, 0x1, 0x1, 0x1c, 0x715f},
    {27000000, 108000000, 0x1, 0x1, 0x1b, 0x0000},
    {27000000, 110000000, 0x1, 0x0, 0x1f, 0x129e},
    {27000000, 115000000, 0x1, 0x0, 0x1f, 0x3e54},
    {27000000, 120000000, 0x1, 0x0, 0x1f, 0x6666},
    {27000000, 125000000, 0x1, 0x0, 0x1e, 0x0b43},
    {27000000, 126000000, 0x1, 0x0, 0x1e, 0x1249},
    {27000000, 130000000, 0x1, 0x0, 0x1e, 0x2d4a},
    {27000000, 140000000, 0x1, 0x0, 0x1e, 0x6a0e},
    {27000000, 148351648, 0x1, 0x0, 0x1d, 0x1687},
    {27000000, 148500000, 0x1, 0x0, 0x1d, 0x1745},
    {27000000, 162000000, 0x1, 0x0, 0x1d, 0x5555}
};
#else
#error ERROR:invalid DVD_BACKEND defined
#endif

static U32 ClockFreq[CLOCK_T_NUMBER_OF_ELEMENTS];       /* Frequency of each clock */

#ifdef STTBX_PRINT
static char *ClockName[CLOCK_T_NUMBER_OF_ELEMENTS] = {
    "SYNTH0",
    "SYNTH1",
    "SYNTH2",
    "SYNTH3",
    "SYNTH4",
    "SYNTH5",
    "SYNTH6",
    "SYNTH7",
    "SYNTH8",
    "PREDIV",
    "REF_CLK1",
    "REF_CLK2",
    "USR_CLK0",
    "USR_CLK1",
    "USR_CLK2",
    "USR_CLK3",
    "SDIN_CLK",
    "HDIN_CLK",
    "PIX_CLKIN",
    "PREDIV_CLK",
    "DDR_CLK",
    "LMC_CLK",
    "GFX_CLK",
    "PIPE_CLK",
#if defined(ST_7015)
    "DEC_CLK",
#elif defined(ST_7020)
    "ADISP_CLK",
#else
#error ERROR:invalid DVD_BACKEND defined
#endif
    "DISP_CLK",
#if defined(ST_7015)
    "DENC_CLK",
#elif defined(ST_7020)
    "AUX_CLK",
#else
#error ERROR:invalid DVD_BACKEND defined
#endif
    "PIX_CLK",
    "PCM_CLK",
    "ASTRB1",
    "ASTRB2",
    "AUD_CLK",
    "AUD_BIT_CLK",
    "EXT_USR_CLK1", /* Could be used for both chip */
    "EXT_USR_CLK2",
    "EXT_USR_CLK3",
    "EXT_PIX_CLK",
    "EXT_REF_CLK2",
    "EXT_PCM_CLK",
    "EXT_CD_CLK"};
#endif /* STTBX_PRINT */

/* Private function prototypes -------------------------------------------- */
static int GetSynthParam(U32 Fin, U32 Fout, BOOL AudioSynth, U32 *ndiv, U32 *sdiv, U32 *md, U32 *pe);
static int ConfigInputClock(Clock_t Input, U32 Freq);
static S32 ConfigSynth(Clock_t Synth, BOOL Enable, BOOL SelectSynth, Clock_t RefClock, U32 OutputClockFreq);
static int ConfigOutputClock(Clock_t Output, Clock_t Source);

/* Global variables ------------------------------------------------------- */

/* Function definitions --------------------------------------------------- */

/******************************************************************************
Name        : stboot_InitClockGen
Description : Init and Configure Clock Gen, to have a correct DDR clock
Parameters  : RefClk:           Reference clock for Synthesizer 0 (Typ: REF_CLK1)
              RefClkFreq:       Reference clock frequency (Typ: 27e6)
              DDRClkFreq:       Required DDR clock frequency (Typ: 100e6)
Assumptions :
Limitations :
Returns     : TRUE if Init has failed. FALSE if init was successful.
*****************************************************************************/

BOOL stboot_InitClockGen(Clock_t RefClk, U32 RefClkFreq, U32 DDRClkFreq)
{
    BOOL RetError = FALSE;

    /* Configure all reference clocks */
    if (ConfigInputClock(RefClk, RefClkFreq) == -1) { RetError = TRUE; }

#if defined(ST_7015)
    /* Configure all synthesizers */
    if (ConfigSynth(SYNTH0, TRUE,  TRUE, RefClk, DDRClkFreq) < 0)            { RetError = TRUE; }
    if (ConfigSynth(SYNTH1, TRUE,  TRUE, RefClk, DEFAULT_LMC_CLK) < 0)       { RetError = TRUE; }
    if (ConfigSynth(SYNTH2, FALSE, TRUE, RefClk, (DEFAULT_LMC_CLK / 2)) < 0) { RetError = TRUE; }
    if (ConfigSynth(SYNTH3, TRUE,  TRUE, RefClk, DEFAULT_PIX_CLK) < 0)       { RetError = TRUE; }
    if (ConfigSynth(SYNTH4, TRUE,  TRUE, RefClk, DEFAULT_AUD_CLK) < 0)       { RetError = TRUE; }
    if (ConfigSynth(SYNTH5, FALSE, TRUE, RefClk, DEFAULT_DEC_CLK) < 0)       { RetError = TRUE; }
    if (ConfigSynth(SYNTH6, TRUE,  TRUE, RefClk, DEFAULT_DISP_CLK) < 0)      { RetError = TRUE; }
    if (ConfigSynth(SYNTH7, TRUE,  TRUE, RefClk, DEFAULT_DENC_CLK) < 0)      { RetError = TRUE; }
    if (ConfigSynth(SYNTH8, TRUE,  TRUE, RefClk, DEFAULT_PCM_CLK) < 0)       { RetError = TRUE; }

    /* Configure all outputs */
    if (ConfigOutputClock(DDR_CLK,     SYNTH0) == -1)   { RetError = TRUE; }
    if (ConfigOutputClock(LMC_CLK,     SYNTH1) == -1)   { RetError = TRUE; }
    if (ConfigOutputClock(GFX_CLK,     SYNTH2) == -1)   { RetError = TRUE; }
    if (ConfigOutputClock(DEC_CLK,     SYNTH5) == -1)   { RetError = TRUE; }
    if (ConfigOutputClock(DISP_CLK,    SYNTH6) == -1)   { RetError = TRUE; }
    if (ConfigOutputClock(DENC_CLK,    SYNTH7) == -1)   { RetError = TRUE; }
    if (ConfigOutputClock(PIX_CLK,     SYNTH3) == -1)   { RetError = TRUE; }
    if (ConfigOutputClock(AUD_CLK,     SYNTH4) == -1)   { RetError = TRUE; }
    if (ConfigOutputClock(PIPE_CLK,    AUD_CLK) == -1)  { RetError = TRUE; }
    if (ConfigOutputClock(AUD_BIT_CLK, REF_CLK1) == -1) { RetError = TRUE; }   /* FC: ???? */

    /* Configure user clocks as outputs for test purpose */
    if (ConfigOutputClock(USR_CLK0,    SYNTH1) == -1) { RetError = TRUE; }
    if (ConfigOutputClock(USR_CLK1,    SYNTH4) == -1) { RetError = TRUE; }  /* Synth 2 is disable */
    if (ConfigOutputClock(USR_CLK2,    SYNTH6) == -1) { RetError = TRUE; }  /* Synth 5 is not used */
    if (ConfigOutputClock(USR_CLK3,    SYNTH7) == -1) { RetError = TRUE; }  /* Pre-div is not used */

    /* FC: Enable PIXCLK output */
    STSYS_WriteRegDev32LE(CKG + CKG_PRE_DIV, CKG_PRE_DIV_PXO);

    /* Invert PIXCLK output from STi7015 to get a correct clock */
    STSYS_WriteRegDev32LE(DSPCFG + DSPCFG_CLK, VOS_CLK_INVCLK);
#elif defined(ST_7020)

    if (ConfigSynth(SYNTH0, TRUE,  TRUE,  RefClk, DDRClkFreq) < 0) { RetError = TRUE; }
    if (ConfigSynth(SYNTH1, TRUE,  TRUE,  RefClk, DEFAULT_LMC_CLK) < 0) { RetError = TRUE; }
    if (ConfigSynth(SYNTH2, TRUE,  TRUE,  RefClk, DEFAULT_ADISP_CLK) < 0) { RetError = TRUE; }
    if (ConfigSynth(SYNTH3, TRUE,  TRUE,  RefClk, DEFAULT_PIX_CLK) < 0) { RetError = TRUE; } /* Can use internal PixClk on cut 2.0 */
    if (ConfigSynth(SYNTH4, TRUE,  TRUE,  RefClk, DEFAULT_AUD_CLK) < 0) { RetError = TRUE; }
    if (ConfigSynth(SYNTH5, TRUE,  TRUE,  RefClk, DEFAULT_PIPE_CLK) < 0) { RetError = TRUE; } /* can be distinct from ADISP_CLK on cut 2.0 since GFX_CLK = LMC_CLK frees SYNTH2 */
    if (ConfigSynth(SYNTH6, TRUE,  TRUE,  RefClk, DEFAULT_DISP_CLK) < 0) { RetError = TRUE; }
    if (ConfigSynth(SYNTH7, TRUE,  TRUE,  RefClk, DEFAULT_AUX_CLK) < 0) { RetError = TRUE; }
    if (ConfigSynth(SYNTH8, TRUE,  TRUE,  RefClk, DEFAULT_PCM_CLK) < 0) { RetError = TRUE; }

    /* Configure all outputs */
    if (ConfigOutputClock(DDR_CLK,     SYNTH0)   == -1)   { RetError = TRUE; }
    if (ConfigOutputClock(LMC_CLK,     SYNTH1)   == -1)   { RetError = TRUE; }
    if (ConfigOutputClock(GFX_CLK,     LMC_CLK)  == -1)   { RetError = TRUE; } /* GFX_CLK = LMC_CLK on cut 2.0 */
    if (ConfigOutputClock(PIX_CLK,     SYNTH3)   == -1)   { RetError = TRUE; }
    if (ConfigOutputClock(AUD_CLK,     SYNTH4)   == -1)   { RetError = TRUE; }
    if (ConfigOutputClock(PIPE_CLK,    SYNTH5)   == -1)   { RetError = TRUE; }
    if (ConfigOutputClock(ADISP_CLK,   SYNTH2)   == -1)   { RetError = TRUE; }
    if (ConfigOutputClock(DISP_CLK,    SYNTH6)   == -1)   { RetError = TRUE; }
    if (ConfigOutputClock(AUX_CLK,     SYNTH7)   == -1)   { RetError = TRUE; }
    if (ConfigOutputClock(AUD_BIT_CLK, REF_CLK1) == -1)   { RetError = TRUE; }

    /* Configure user clocks as outputs for test purpose */
    /* suppress ConfigOutputClock(USR_CLK0,SYNTH1) : default config must be used */
    if (ConfigOutputClock(USR_CLK1,    SYNTH4) == -1)   { RetError = TRUE; }    /* Synth 2 is disable */

    /* Set PIXCLK as an input */
    STSYS_WriteRegDev32LE(CKG + CKG_PRE_DIV, 0x0);

    /* Invert PIXCLK output from STi7015 to get a correct clock */
    STSYS_WriteRegDev32LE(DSPCFG + DSPCFG_CLK, VOS_CLK_INVCLK);

#else
  #error ERROR:invalid DVD_BACKEND defined
#endif

    return (RetError);
}


/*----------------------------------------------------------------------------
 Display complete status
 This function uses printf() functions, and thus can be called during boot,
 before toolboxes are initialized.
 Param:         None
 Return:        FALSE if no error occured
----------------------------------------------------------------------------*/
void stboot_DisplayClockGenStatus(void)
{
#ifdef STTBX_PRINT
    int i;

    /* Print all *theorical* frequencies */
    printf("\n**** Programmed Clock Frequencies: ****\n");
    for (i = 0; i < CLOCK_T_NUMBER_OF_ELEMENTS; i++)
    {
        if (i%4 == 0)
        {
            printf("\n");
        }
        printf("%12s: %6d kHz\t", ClockName[i], ClockFreq[i]/1000);
    }
    printf("\n");
#endif
}


/*****************************************************************************
Name        : ConfigOutputClock
Description : Configures an output clock
Parameters  : Output:   Output clock
              Source:   Source clock
Assumptions :
Limitations :
Returns     : Output frequency if OK, -1 on error
*****************************************************************************/
static int ConfigOutputClock(Clock_t Output, Clock_t Source)
{

    switch (Output)
    {
    case DDR_CLK:
        switch (Source)
        {
        case SYNTH0:
            STSYS_WriteRegDev32LE(CKG + CKG_SEL_DDR, 1);
            break;
        case DISABLE:
            STSYS_WriteRegDev32LE(CKG + CKG_SEL_DDR, 0);
            break;
        default:
#ifdef STTBX_PRINT
            fprintf(stderr, "ConfigOutputClock: Invalid source clock %d for output %d\n",
                    Source, Output);
#endif
            return -1;
        }
        break;
    case LMC_CLK:
        switch (Source)
        {
        case SYNTH1:
            STSYS_WriteRegDev32LE(CKG + CKG_SEL_MEM, 1);
            break;
        case DDR_CLK:
            STSYS_WriteRegDev32LE(CKG + CKG_SEL_MEM, 0);
            break;
        default:
#ifdef STTBX_PRINT
            fprintf(stderr, "ConfigOutputClock: Invalid source clock %d for output %d\n", Source, Output);
#endif
            return -1;
        }
        break;
    case GFX_CLK:
        switch (Source)
        {
        case SYNTH2:
            STSYS_WriteRegDev32LE(CKG + CKG_SEL_GFX, 1);
            break;
        case LMC_CLK:
            STSYS_WriteRegDev32LE(CKG + CKG_SEL_GFX, 0);
            break;
        default:
#ifdef STTBX_PRINT
            fprintf(stderr, "ConfigOutputClock: Invalid source clock %d for output %d\n", Source, Output);
#endif
            return -1;
        }
        break;
    case PIX_CLK:
        switch (Source)
        {
        case SYNTH3:
            STSYS_WriteRegDev32LE(CKG + CKG_SEL_PIX, 1);
            break;
        case DISABLE:
            STSYS_WriteRegDev32LE(CKG + CKG_SEL_PIX, 0);
            break;
        default:
#ifdef STTBX_PRINT
            fprintf(stderr, "ConfigOutputClock: Invalid source clock %d for output %d\n", Source, Output);
#endif
            return -1;
        }
        break;
    case AUD_CLK:
        switch (Source)
        {
        case SYNTH4:
            STSYS_WriteRegDev32LE(CKG + CKG_SEL_AUD, 1);
            break;
        case DISABLE:
            STSYS_WriteRegDev32LE(CKG + CKG_SEL_AUD, 0);
            break;
        default:
#ifdef STTBX_PRINT
            fprintf(stderr, "ConfigOutputClock: Invalid source clock %d for output %d\n", Source, Output);
#endif
            return -1;
        }
        break;
    case PIPE_CLK:
        switch (Source)
        {
        case SYNTH5:
            STSYS_WriteRegDev32LE(CKG + CKG_SEL_VID, 1);
            break;
        case AUD_CLK:
            STSYS_WriteRegDev32LE(CKG + CKG_SEL_VID, 0);
            break;
        default:
#ifdef STTBX_PRINT
            fprintf(stderr, "ConfigOutputClock: Invalid source clock %d for output %d\n", Source, Output);
#endif
            return -1;
        }
        break;

#if defined(ST_7015)
        case DEC_CLK:
            switch (Source)
            {
            case SYNTH5:
                STSYS_WriteRegDev32LE(CKG + CKG_SEL_DEC, 1);
                break;
            case DISABLE:
                STSYS_WriteRegDev32LE(CKG + CKG_SEL_DEC, 0);
                break;
            default:
                fprintf(stderr, "ConfigOutputClock: Invalid source clock %d for output %d\n",
                        Source, Output);
                return -1;
            }
            break;
#elif defined(ST_7020)
        case ADISP_CLK:
            switch (Source)
            {
            case SYNTH2:
                STSYS_WriteRegDev32LE(CKG + CKG_SEL_ADISP, 1);
                break;
            case SYNTH5:
                STSYS_WriteRegDev32LE(CKG + CKG_SEL_ADISP, 2);
                break;
            case SYNTH6:
                STSYS_WriteRegDev32LE(CKG + CKG_SEL_ADISP, 3);
                break;
            case DISABLE:
                STSYS_WriteRegDev32LE(CKG + CKG_SEL_ADISP, 0);
                break;
            default:
                fprintf(stderr, "ConfigOutputClock: Invalid source clock %d for output %d\n",
                        Source, Output);
                return -1;
            }
            break;
#else
#error ERROR:invalid DVD_BACKEND defined
#endif
    case DISP_CLK:
        switch (Source)
        {
        case SYNTH6:
            STSYS_WriteRegDev32LE(CKG + CKG_SEL_DISP, 1);
            break;
        case DISABLE:
            STSYS_WriteRegDev32LE(CKG + CKG_SEL_DISP, 0);
            break;
        default:
#ifdef STTBX_PRINT
            fprintf(stderr, "ConfigOutputClock: Invalid source clock %d for output %d\n",
                    Source, Output);
#endif
            return -1;
        }
        break;
#if defined(ST_7015)
        case DENC_CLK:
            switch (Source)
            {
            case SYNTH7:
                STSYS_WriteRegDev32LE(CKG + CKG_SEL_DENC, 1);
                break;
            case DISABLE:
                STSYS_WriteRegDev32LE(CKG + CKG_SEL_DENC, 0);
                break;
            default:
                fprintf(stderr, "ConfigOutputClock: Invalid source clock %d for output %d\n",
                        Source, Output);
                return -1;
            }
            break;
#elif defined(ST_7020)
        case AUX_CLK:
            switch (Source)
            {
            case SYNTH7:
                STSYS_WriteRegDev32LE(CKG + CKG_SEL_AUX, 1);
                break;
            case DISABLE:
                STSYS_WriteRegDev32LE(CKG + CKG_SEL_AUX, 0);
                break;
            default:
                fprintf(stderr, "ConfigOutputClock: Invalid source clock %d for output %d\n",
                        Source, Output);
                return -1;
            }
            break;
#else
#error ERROR:invalid DVD_BACKEND defined
#endif
    case AUD_BIT_CLK:
        switch (Source)
        {
        case ASTRB2:
            STSYS_WriteRegDev32LE(CKG + CKG_SEL_AHS, 3);
            break;
        case PCM_CLK:
            STSYS_WriteRegDev32LE(CKG + CKG_SEL_AHS, 2);
            break;
        case REF_CLK1:
            STSYS_WriteRegDev32LE(CKG + CKG_SEL_AHS, 1);
            break;
        case DISABLE:
            STSYS_WriteRegDev32LE(CKG + CKG_SEL_AHS, 0);
            break;
        default:
#ifdef STTBX_PRINT
            fprintf(stderr, "ConfigOutputClock: Invalid source clock %d for output %d\n",
                    Source, Output);
#endif
            return -1;
        }
        break;
    case USR_CLK0:
        switch (Source)
        {
        case SYNTH3:
            STSYS_WriteRegDev32LE(CKG + CKG_SEL_RCLK2, 3);
            break;
        case SYNTH1:
            STSYS_WriteRegDev32LE(CKG + CKG_SEL_RCLK2, 2);
            break;
        default:
#ifdef STTBX_PRINT
            fprintf(stderr, "ConfigOutputClock: Invalid source clock %d for output %d\n",
                    Source, Output);
#endif
            return -1;
        }
        break;
    case USR_CLK1:
        switch (Source)
        {
        case SYNTH4:
            STSYS_WriteRegDev32LE(CKG + CKG_SEL_USR1, 3);
            break;
        case SYNTH2:
            STSYS_WriteRegDev32LE(CKG + CKG_SEL_USR1, 2);
            break;
        default:
#ifdef STTBX_PRINT
            fprintf(stderr, "ConfigOutputClock: Invalid source clock %d for output %d\n",
                    Source, Output);
#endif
            return -1;
        }
        break;
    case USR_CLK2:
        switch (Source)
        {
        case SYNTH6:
            STSYS_WriteRegDev32LE(CKG + CKG_SEL_USR2, 3);
            break;
        case SYNTH5:
            STSYS_WriteRegDev32LE(CKG + CKG_SEL_USR2, 2);
            break;
        default:
#ifdef STTBX_PRINT
            fprintf(stderr, "ConfigOutputClock: Invalid source clock %d for output %d\n",
                    Source, Output);
#endif
            return -1;
        }
        break;
    case USR_CLK3:
        switch (Source)
        {
        case PREDIV:
            STSYS_WriteRegDev32LE(CKG + CKG_SEL_USR3, 3);
            break;
        case SYNTH7:
            STSYS_WriteRegDev32LE(CKG + CKG_SEL_USR3, 2);
            break;
        default:
#ifdef STTBX_PRINT
            fprintf(stderr, "ConfigOutputClock: Invalid source clock %d for output %d\n",
                    Source, Output);
#endif
            return -1;
        }
        break;

    default:
#ifdef STTBX_PRINT
        fprintf(stderr, "ConfigOutputClock: Invalid output clock %d\n", Output);
#endif
        return -1;
    }

    /* Store output frequency */
    if (Source < 0)
    {
        ClockFreq[Output] = 0;
    }
    else
    {
        ClockFreq[Output] = ClockFreq[Source];
    }

    return ClockFreq[Output];
}

/*****************************************************************************
Name        : ConfigInputClock
Description : Configures an input clock
Parameters  : Input:    Input clock
              Freq:     Input frequency
Assumptions :
Limitations :
Returns     : 0 if OK, -1 on error
*****************************************************************************/
static int ConfigInputClock(Clock_t Input, U32 Freq)
{

    if (Input >= CLOCK_T_NUMBER_OF_ELEMENTS)
    {
#ifdef STTBX_PRINT
        fprintf(stderr, "ConfigInputClock: Invalid input clock %d\n", Input);
#endif
        return -1;
    }

    switch (Input)
    {
    case REF_CLK2:      /* = USR_CLK0 */
    case USR_CLK0:
        STSYS_WriteRegDev32LE(CKG + CKG_SEL_RCLK2, 0);
        break;
    case USR_CLK1:
        STSYS_WriteRegDev32LE(CKG + CKG_SEL_USR1, 0);
        break;
    case USR_CLK2:
        STSYS_WriteRegDev32LE(CKG + CKG_SEL_USR2, 0);
        break;
    case USR_CLK3:
        STSYS_WriteRegDev32LE(CKG + CKG_SEL_USR3, 0);
        break;
    }

    /* Store input frequency */
    ClockFreq[Input] = Freq;

    return 0;
}

/*****************************************************************************
Name        : ConfigSynth
Description : Configures a synthesizer
Parameters  : Synth: Addressed synthesizer
              Enable: Enable or disable synthesizer
              SelectSynth: Select synthesizer or external PLL clock
              RefClock: Input reference clock
              OutputClockFreq: Required output clock frequency
Assumptions :
Limitations :
Returns     : Real output frequency, negative value on error
*****************************************************************************/
static S32 ConfigSynth(Clock_t Synth, BOOL Enable, BOOL SelectSynth, Clock_t RefClock, U32 OutputClockFreq)
{
    int foutr=0;
#ifndef HARDWARE_EMULATOR
    U32 par, cfg, en, ssl, sfo, ndiv, sdiv, md, pe, i2s, i2ss, ckrs;

    if (Synth < SYNTH0 || Synth > SYNTH8)
    {
#ifdef STTBX_PRINT
        fprintf(stderr,"ConfigSynth: Invalid Synthesizer %d\n", (int)Synth);
#endif
        return -1;
    }

    if (Synth == SYNTH8)
    {
#if defined(ST_7015)
        en = ((STSYS_ReadRegDev32LE(MMDSP + AUD_PLLPCM) & 0x2) >> 1);
        cfg = STSYS_ReadRegDev32LE(CKG + CKG_PCMO_CFG);
        ssl  =  cfg & CKG_PCMO_CFG_SSL;
        ckrs = (cfg & CKG_PCMO_CFG_CKRS) >> 1;
        i2ss = (cfg & CKG_PCMO_CFG_I2SS) >> 2;
        i2s  = (cfg & CKG_PCMO_CFG_I2S)  >> 3;
        sfo  = 1;
        ndiv = (cfg & CKG_PCMO_CFG_NDIV) >> 5;
        pe = STSYS_ReadRegDev32LE(CKG + CKG_PCMO_PE);
        md = STSYS_ReadRegDev32LE(CKG + CKG_PCMO_MD);
        sdiv = STSYS_ReadRegDev32LE(CKG + CKG_PCMO_SDIV);
#elif defined(ST_7020)
        cfg  = STSYS_ReadRegDev32LE(CKG + CKG_PCMO_CFG);
        ckrs =  cfg & CKG_PCMO_CFG_CKRS;
        i2ss = (cfg & CKG_PCMO_CFG_I2SS) >> 1;
        i2s  = (cfg & CKG_PCMO_CFG_I2S)  >> 2;
        sfo  = 1;

        par  =  STSYS_ReadRegDev32LE(CKG + CKG_PCMO_PAR);
        pe   =  par & CKG_SYN_PAR_PE_MASK;
        md   = (par & CKG_SYN_PAR_MD_MASK)   >> 16;
        sdiv = (par & CKG_SYN_PAR_SDIV_MASK) >> 21;
        ndiv = (par & CKG_SYN_PAR_NDIV_MASK) >> 24;
        ssl  = (par & CKG_SYN_PAR_SSL)       >> 26;
        en   = (par & CKG_SYN_PAR_EN)        >> 27;
#else
#error ERROR:invalid DVD_BACKEND defined
#endif
    }
    else
    {
        /* Read current parameters */
        par = STSYS_ReadRegDev32LE(CKG + CKG_SYN0_PAR + Synth*4);
        pe   =  par & CKG_SYN_PAR_PE_MASK;
        md   = (par & CKG_SYN_PAR_MD_MASK)   >> 16;
        sdiv = (par & CKG_SYN_PAR_SDIV_MASK) >> 21;
        ndiv = (par & CKG_SYN_PAR_NDIV_MASK) >> 24;
        en   = (par & CKG_SYN_PAR_EN)        >> 27;
        ssl  = (par & CKG_SYN_PAR_SSL)       >> 26;
    }

    if (Enable)
    {
        /* Select synthesizer or reference clock */
        ssl = SelectSynth ? 1 : 0;

        switch (Synth)
        {
#if defined(ST_7015)
            case SYNTH0:
            case SYNTH1:
            case SYNTH2:
                if (SelectSynth)
                {
                    if (RefClock != REF_CLK1)
                    {
#ifdef STTBX_PRINT
                        fprintf(stderr,"ConfigSynth: Invalid RefClock for synthesizer %d\n", (int)Synth);
#endif
                        return -1;
                    }
                }
                else
                {
                    RefClock = USR_CLK2;
                }
                break;
#elif defined(ST_7020)
            case SYNTH0:
            case SYNTH2:
                if (SelectSynth)
                {
                    switch (RefClock)
                    {
                        case REF_CLK1:          /* Synthesizer selected */
                            STSYS_WriteRegDev32LE(CKG + CKG_SYN3_REF, 0);
                            break;
                        case REF_CLK2:          /* Synthesizer selected */
                            STSYS_WriteRegDev32LE(CKG + CKG_SYN3_REF, 1);
                            break;
                        case PREDIV_CLK:        /* Synthesizer selected */
                            STSYS_WriteRegDev32LE(CKG + CKG_SYN3_REF, 2);
                            break;
                        case SDIN_CLK:          /* Synthesizer selected */
                            STSYS_WriteRegDev32LE(CKG + CKG_SYN3_REF, 3);
                            break;
                        default:
                            printf("ConfigSynth: Invalid RefClock for synthesizer %d\n", (int)Synth);
                            return -1;
                    }
                }
                else
                {
                    RefClock = USR_CLK2;
                }
                break;
            case SYNTH1:
                if (SelectSynth)
                {
                    switch (RefClock)
                    {
                        case REF_CLK1:          /* Synthesizer selected */
                            STSYS_WriteRegDev32LE(CKG + CKG_SYN3_REF, 0);
                            break;
                        case REF_CLK2:          /* Synthesizer selected */
                            STSYS_WriteRegDev32LE(CKG + CKG_SYN3_REF, 1);
                            break;
                        case PREDIV_CLK:        /* Synthesizer selected */
                            STSYS_WriteRegDev32LE(CKG + CKG_SYN3_REF, 2);
                            break;
                        case SDIN_CLK:          /* Synthesizer selected */
                            STSYS_WriteRegDev32LE(CKG + CKG_SYN3_REF, 3);
                            break;
                        default:
                            printf("ConfigSynth: Invalid RefClock for synthesizer %d\n", (int)Synth);
                            return -1;
                    }
                }
                else
                {
                    RefClock = ASTRB2;
                }
                break;
#else
#error ERROR:invalid DVD_BACKEND defined
#endif
        case SYNTH3:
            if (SelectSynth)
            {
                switch (RefClock)
                {
                case REF_CLK1:                  /* Synthesizer selected */
                    STSYS_WriteRegDev32LE(CKG + CKG_SYN3_REF, 0);
                    break;
                case REF_CLK2:                  /* Synthesizer selected */
                    STSYS_WriteRegDev32LE(CKG + CKG_SYN3_REF, 1);
                    break;
                case PREDIV_CLK:                /* Synthesizer selected */
                    STSYS_WriteRegDev32LE(CKG + CKG_SYN3_REF, 2);
                    break;
                case SDIN_CLK:                  /* Synthesizer selected */
                    STSYS_WriteRegDev32LE(CKG + CKG_SYN3_REF, 3);
                    break;
                default:
#ifdef STTBX_PRINT
                    fprintf(stderr,"ConfigSynth: Invalid RefClock for synthesizer %d\n", (int)Synth);
#endif
                    return -1;
                }
            }
            break;
#if defined(ST_7015)
            case SYNTH4:
                if (SelectSynth)
                {
                    if (RefClock != REF_CLK1)
                    {
#ifdef STTBX_PRINT
                        fprintf(stderr,"ConfigSynth: Invalid RefClock for synthesizer %d\n", (int)Synth);
#endif
                        return -1;
                    }
                }
                else
                {
                    RefClock = USR_CLK1;
                }
                break;
            case SYNTH5:
                if (SelectSynth)
                {
                    if (RefClock != REF_CLK1)
                    {
#ifdef STTBX_PRINT
                        fprintf(stderr,"ConfigSynth: Invalid RefClock for synthesizer %d\n", (int)Synth);
#endif
                        return -1;
                    }
                }
                else
                {
                    RefClock = REF_CLK2;
                }
                break;
            case SYNTH6:
                if (SelectSynth)
                {
                    if (RefClock != REF_CLK1)
                    {
#ifdef STTBX_PRINT
                        fprintf(stderr,"ConfigSynth: Invalid RefClock for synthesizer %d\n", (int)Synth);
#endif
                        return -1;
                    }
                }
                else
                {
                    RefClock = USR_CLK3;
                }
                break;
#elif defined(ST_7020)
            case SYNTH4:
                if (SelectSynth)
                {
                    switch (RefClock)
                    {
                        case REF_CLK1:          /* Synthesizer selected */
                            STSYS_WriteRegDev32LE(CKG + CKG_SYN7_REF, 0);
                            break;
                        case REF_CLK2:          /* Synthesizer selected */
                            STSYS_WriteRegDev32LE(CKG + CKG_SYN7_REF, 1);
                            break;
                        case PREDIV_CLK:        /* Synthesizer selected */
                            STSYS_WriteRegDev32LE(CKG + CKG_SYN7_REF, 2);
                            break;
                        case SDIN_CLK:          /* Synthesizer selected */
                            STSYS_WriteRegDev32LE(CKG + CKG_SYN7_REF, 3);
                            break;
                        default:
                            printf("ConfigSynth: Invalid RefClock for synthesizer %d\n", (int)Synth);
                            return -1;
                    }
                }
                else
                {
                    RefClock = USR_CLK1;
                }
                break;
            case SYNTH5:
                if (SelectSynth)
                {
                    switch (RefClock)
                    {
                        case REF_CLK1:          /* Synthesizer selected */
                            STSYS_WriteRegDev32LE(CKG + CKG_SYN7_REF, 0);
                            break;
                        case REF_CLK2:          /* Synthesizer selected */
                            STSYS_WriteRegDev32LE(CKG + CKG_SYN7_REF, 1);
                            break;
                        case PREDIV_CLK:        /* Synthesizer selected */
                            STSYS_WriteRegDev32LE(CKG + CKG_SYN7_REF, 2);
                            break;
                        case SDIN_CLK:          /* Synthesizer selected */
                            STSYS_WriteRegDev32LE(CKG + CKG_SYN7_REF, 3);
                            break;
                        default:
                            printf("ConfigSynth: Invalid RefClock for synthesizer %d\n", (int)Synth);
                            return -1;
                    }
                }
                else
                {
                    RefClock = REF_CLK2;
                }
                break;
            case SYNTH6:
                if (SelectSynth)
                {
                    switch (RefClock)
                    {
                        case REF_CLK1:          /* Synthesizer selected */
                            STSYS_WriteRegDev32LE(CKG + CKG_SYN7_REF, 0);
                            break;
                        case REF_CLK2:          /* Synthesizer selected */
                            STSYS_WriteRegDev32LE(CKG + CKG_SYN7_REF, 1);
                            break;
                        case PREDIV_CLK:        /* Synthesizer selected */
                            STSYS_WriteRegDev32LE(CKG + CKG_SYN7_REF, 2);
                            break;
                        case SDIN_CLK:          /* Synthesizer selected */
                            STSYS_WriteRegDev32LE(CKG + CKG_SYN7_REF, 3);
                            break;
                        default:
                            printf("ConfigSynth: Invalid RefClock for synthesizer %d\n", (int)Synth);
                            return -1;
                    }
                }
                else
                {
                    RefClock = USR_CLK3;
                }
                break;
#else
#error ERROR:invalid DVD_BACKEND defined
#endif
        case SYNTH7:
            if (SelectSynth)
            {
                switch (RefClock)
                {
                    case REF_CLK1:              /* Synthesizer selected */
                        STSYS_WriteRegDev32LE(CKG + CKG_SYN7_REF, 0);
                        break;
                    case REF_CLK2:              /* Synthesizer selected */
                        STSYS_WriteRegDev32LE(CKG + CKG_SYN7_REF, 1);
                        break;
                    case PREDIV_CLK:            /* Synthesizer selected */
                        STSYS_WriteRegDev32LE(CKG + CKG_SYN7_REF, 2);
                        break;
                    case SDIN_CLK:              /* Synthesizer selected */
                        STSYS_WriteRegDev32LE(CKG + CKG_SYN7_REF, 3);
                        break;
                    default:
#ifdef STTBX_PRINT
                        fprintf(stderr,"ConfigSynth: Invalid RefClock for synthesizer %d\n", (int)Synth);
#endif
                        return -1;
                }
            }
            break;
        case SYNTH8:
            if (SelectSynth)
            {
                switch (RefClock)
                {
                case REF_CLK1:                          /* Synthesizer selected */
                    i2s = i2ss = ckrs = 0;
                    break;
                case REF_CLK2:                          /* Synthesizer selected */
                    i2s = i2ss = 0;
                    ckrs = CKG_PCMO_CFG_CKRS;
                    break;
                case ASTRB1:                            /* Synthesizer selected */
                    i2ss = ckrs = 0;
                    i2s = CKG_PCMO_CFG_I2S;
                    break;
                case ASTRB2:                            /* Synthesizer selected */
                    i2s = CKG_PCMO_CFG_I2S;
                    i2ss = CKG_PCMO_CFG_I2SS;
                    ckrs = 0;
                    break;
                default:
#ifdef STTBX_PRINT
                    fprintf(stderr,"ConfigSynth: Invalid RefClock for synthesizer %d\n", (int)Synth);
#endif
                    return -1;
                }
            }
            break;
        }

        /* Enable synthesizer */
        en = 1;

        /* Get synthesizer parameters */
        if (SelectSynth)
        {
            if ((foutr = GetSynthParam(ClockFreq[(int)RefClock], OutputClockFreq, (Synth == SYNTH8), &ndiv, &sdiv, &md, &pe)) < 0)
            {
#ifdef STTBX_PRINT
                fprintf(stderr,"ConfigSynth: Couldn't calculate synthesizer parameters\n");
                fprintf(stderr,"ConfigSynth: Input %d Output %d\n",ClockFreq[(int)RefClock],
                        OutputClockFreq);
#endif
                return -1;
            }
            ClockFreq[Synth] = (U32)foutr;
        }
        else
        {
            ClockFreq[Synth] = ClockFreq[RefClock];
        }
    }
    else
    {
        /* Disable synthesizer */
        en = 0;

#if defined(ST_7015)
        ndiv = 0;
#elif defined(ST_7020)
        ndiv = 1;   /* This is common to 4 synthesizers ! */
#else
#error ERROR:invalid DVD_BACKEND defined
#endif

        /* Store real output frequency in frequency array */
        ClockFreq[Synth] = 0;
    }

    if (Synth == SYNTH8)
    {
#if defined(ST_7015)
        STSYS_WriteRegDev32LE(CKG + CKG_PCMO_PE, pe);
        STSYS_WriteRegDev32LE(CKG + CKG_PCMO_MD, md);
        STSYS_WriteRegDev32LE(CKG + CKG_PCMO_SDIV, sdiv);
        cfg = (ndiv << 5)
            | (sfo  << 4)
            | (i2s  << 3)
            | (i2ss << 2)
            | (ckrs << 1)
            |  ssl;
        STSYS_WriteRegDev32LE(CKG + CKG_PCMO_CFG, cfg);
        STSYS_WriteRegDev32LE(MMDSP + AUD_PLLPCM, (((STSYS_ReadRegDev32LE(MMDSP + AUD_PLLPCM)) & ~0x2) | (en<<1)));
#elif defined(ST_7020)
        par = (0x1 << 28)   /* FC: by default we set PDIV = 1 */
            | (en  << 27)
            | (ssl << 26)
            | (ndiv << 24)
            | (sdiv << 21)
            | (md << 16)
            |  pe;
        STSYS_WriteRegDev32LE(CKG + CKG_PCMO_PAR, par);
        cfg = (sfo  << 3)
            | (i2s  << 2)
            | (i2ss << 1)
            |  ckrs;
        STSYS_WriteRegDev32LE(CKG + CKG_PCMO_CFG, cfg);
#else
#error ERROR:invalid DVD_BACKEND defined
#endif
    }
    else
    {
        /* Add synthesizer parameters to register */
        par = (en  << 27)
            | (ssl << 26)
            | (ndiv << 24)
            | (sdiv << 21)
            | (md << 16)
            |  pe;

        /* Refresh parameters */
        STSYS_WriteRegDev32LE(CKG + CKG_SYN0_PAR + Synth*4, par);
    }

#endif /* not HARDWARE_EMULATOR */

    /* Return real output frequency */
    return(foutr);
}


/*****************************************************************************
Name        : GetSynthParam
Description : Get synthesizer parameters from specific table according to
              input/output frequencies
Parameters  : fin: input reference frequency
              fout: required output frequency
              ndiv:
              sdiv:
              md:
              pe: synthesizer parameter bits
Assumptions :
Limitations :
Returns     : Real output frequency, negative value on error
*****************************************************************************/
static int GetSynthParam(U32 Fin, U32 Fout, BOOL AudioSynth, U32 *ndiv, U32 *sdiv, U32 *md, U32 *pe)
{
    int def=0;
    double fin, fout, foutr;
    int k, ipe1, sp, np, md2;

    fin = (double)Fin;
    fout = (double)Fout;

    if (!(((fin < (1+0.005)*13.5e6) && (fin >= (1-0.005)*13.5e6))
          ||((fin < (1+0.005)*27e6) && (fin >= (1-0.005)*27e6))
          ||((fin <= (1+0.005)*54e6) && (fin > (1-0.005)*54e6)) ))
    {
#ifdef STTBX_PRINT
        fprintf(stderr, "Input frequency not allowed for this synthesizer !!\n");
        fprintf(stderr, "Please enter an input frequency equal to 13.5MHz, 27MHz or 54MHz  +/- 0.5/100  ...\n");
#endif
        def=1;
    }

    if ( ( (fout <= 843.75e3) || (fout >= 216e6) ) && (fout != 0) )
    {
#ifdef STTBX_PRINT
        fprintf(stderr, "Output frequency not allowed for this synthetizer !!\n");
        fprintf(stderr, "Please enter frequency in interval ]843.75 kHz , 216 MHz[\n");
#endif
        def=1;
    }

    if (def==0)
    {
        foutr = -1;
#ifdef ST_7015
        for(k=0;k<sizeof(FreqSynthElements)/sizeof(FreqSynthElements_t);k++)
        {
            if(FreqSynthElements[k].Fin == Fin && FreqSynthElements[k].Fout == Fout)
            {
                np = FreqSynthElements[k].ndiv;
                sp = FreqSynthElements[k].sdiv;
                md2 = FreqSynthElements[k].md;
                ipe1 = FreqSynthElements[k].pe;
                foutr = (double)FreqSynthElements[k].Fout;
                break;
            }
        }
#else /* STi7020 */
        if (AudioSynth==FALSE)
        {
            for(k=0;k<sizeof(FreqSynthElements)/sizeof(FreqSynthElements_t);k++)
            {
                if(FreqSynthElements[k].Fin == Fin && FreqSynthElements[k].Fout == Fout)
                {
                    np = FreqSynthElements[k].ndiv;
                    sp = FreqSynthElements[k].sdiv;
                    md2 = FreqSynthElements[k].md;
                    ipe1 = FreqSynthElements[k].pe;
                    foutr = (double)FreqSynthElements[k].Fout;
                    break;
                }
            }
        }
        else
        {
            for(k=0;k<sizeof(AudioFreqSynthElements)/sizeof(FreqSynthElements_t);k++)
            {
                if(AudioFreqSynthElements[k].Fin == Fin && AudioFreqSynthElements[k].Fout == Fout)
                {
                    np = AudioFreqSynthElements[k].ndiv;
                    sp = AudioFreqSynthElements[k].sdiv;
                    md2 = AudioFreqSynthElements[k].md;
                    ipe1 = AudioFreqSynthElements[k].pe;
                    foutr = (double)AudioFreqSynthElements[k].Fout;
                    break;
                }
            }
        }
#endif

        *ndiv = (np   & 0x3);
        *sdiv = (sp   & 0x7);
        *md   = (md2  & 0x1f);
        *pe   = (ipe1 & 0xffff);
    }
    return ((int)foutr);
}
/* End of clk_gen.c */
