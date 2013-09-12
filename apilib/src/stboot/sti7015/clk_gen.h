/*******************************************************************************
File Name   : clk_gen.h

Description : Header file of Clock Gen setup

COPYRIGHT (C) STMicroelectronics 2001.

*******************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __CLK_GEN_H
#define __CLK_GEN_H

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Includes --------------------------------------------------------------- */

/* Exported Constants ----------------------------------------------------- */

/* Exported Variables ----------------------------------------------------- */

/* Exported Types --------------------------------------------------------- */
typedef enum
{
    DISABLE = -2,
    INPUT = -1,
    SYNTH0 = 0, /* Synthesizer clocks */
    SYNTH1 = 1,
    SYNTH2 = 2,
    SYNTH3 = 3,
    SYNTH4 = 4,
    SYNTH5 = 5,
    SYNTH6 = 6,
    SYNTH7 = 7,
    SYNTH8 = 8,
    PREDIV,
    REF_CLK1,   /* input clocks */
    REF_CLK2,
    USR_CLK0,
    USR_CLK1,
    USR_CLK2,
    USR_CLK3,
    SDIN_CLK,
    HDIN_CLK,
    PIX_CLKIN,
    PREDIV_CLK,
    DDR_CLK,    /* output clocks */
    LMC_CLK,
    GFX_CLK,
    PIPE_CLK,
#if defined(ST_7015)
    DEC_CLK,
#elif defined(ST_7020)
    ADISP_CLK,
#else
#error ERROR: Invalid DVD_BACKEND defined
#endif
    DISP_CLK,
#if defined(ST_7015)
    DENC_CLK,
#elif defined(ST_7020)
    AUX_CLK,
#else
#error ERROR: Invalid DVD_BACKEND defined
#endif

    PIX_CLK,
    PCM_CLK,    /* Audio clocks */
    ASTRB1,
    ASTRB2,
    AUD_CLK,
    AUD_BIT_CLK,
    EXT_USR_CLK1, /* Ext Synth clocks (ICS9161) */
    EXT_USR_CLK2,
    EXT_USR_CLK3,
    EXT_PIX_CLK,
    EXT_REF_CLK2,
    EXT_PCM_CLK,
    EXT_CD_CLK,
    CLOCK_T_NUMBER_OF_ELEMENTS  /* Always in last position ! */
} Clock_t;

typedef enum
{
    PREDIV_BY_1 = 0,
    PREDIV_BY_2 = 1,
    PREDIV_BY_3 = 2,
    PREDIV_BY_11_2 = 3
} PreDiv_t;

/* Exported Macros -------------------------------------------------------- */

/* Exported Functions ----------------------------------------------------- */
extern BOOL stboot_InitClockGen(Clock_t RefClk, U32 RefClkFreq, U32 DDRClkFreq);
extern void stboot_DisplayClockGenStatus(void);


/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* ifndef __CLK_GEN_H */

/* End of clk_gen.h */
