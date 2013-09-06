/************************************************************************

Source file name :      ckg_reg.h

Description :           CKG registers defines.

COPYRIGHT (C) STMicroelectronics 2004

References  :

************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __CKG_REG_H
#define __CKG_REG_H

/* Includes ----------------------------------------------------------------- */
#include "stdevice.h"
#include "stddefs.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */
/* --- CKGA registers ( hardware specific ) --------------------------------- */
#define CKGA_LOCK           0x000
#define MD_STATUS           0x004

    /* PLL1 control */
#define PLL1_CTRL           0x008
#define PLL1_CTRL2          0x00c
#define PLL1_LOCK_STATUS    0x010
#define PLL1_CLK1_CTRL      0x014
#define PLL1_CLK2_CTRL      0x018
#define PLL1_CLK3_CTRL      0x01c
#define PLL1_CLK4_CTRL      0x020

    /* PLL2 control */
#define PLL2_CTRL           0x024
#define PLL2_CTRL2          0x028
#define PLL2_LOCK_STATUS    0x02c

#define CKGA_REDUCED_CLOCK  0x030
#define CKGA_CLOCK_ENABLE   0x034
#define CKGA_OUT_CTRL       0x038
#define SYSCLKOUT_CTRL      0x03c


/* --- CKGB registers ( hardware specific ) --------------------------------- */
    /* Clock recovery registers */
#define CKGB_RECOV_REF_MAX      0x000
#define CKGB_RECOV_CMD          0x004
#define CKGB_RECOV_CPT_PCM      0x008
#define CKGB_RECOV_CPT_HD       0x00c

    /* Clockgen B registers */
#define CKGB_LOCK               0x010
#define CKGB_FS0_CTRL           0x014
#define CKGB_FS0_MD1            0x018
#define CKGB_FS0_PE1            0x01c
#define CKGB_FS0_EN_PRG1        0x020
#define CKGB_FS0_SDIV1          0x024
#define CKGB_FS0_MD2            0x028
#define CKGB_FS0_PE2            0x02c
#define CKGB_FS0_EN_PRG2        0x030
#define CKGB_FS0_SDIV2          0x034
#define CKGB_FS0_MD3            0x038
#define CKGB_FS0_PE3            0x03c
#define CKGB_FS0_EN_PRG3        0x040
#define CKGB_FS0_SDIV3          0x044
/*#define CKGB_FS0_MD4            0x048
#define CKGB_FS0_PE4            0x04c
#define CKGB_FS0_EN_PRG4        0x050
#define CKGB_FS0_SDIV4          0x054*/
#define CKGB_FS0_CLKOUT_CTRL    0x058
#define CKGB_FS1_CTRL           0x05c
#define CKGB_FS1_MD1            0x060
#define CKGB_FS1_PE1            0x064
#define CKGB_FS1_EN_PRG1        0x068
#define CKGB_FS1_SDIV1          0x06c
#define CKGB_FS1_MD2            0x070
#define CKGB_FS1_PE2            0x074
#define CKGB_FS1_EN_PRG2        0x078
#define CKGB_FS1_SDIV2          0x07c
#define CKGB_FS1_MD3            0x080
#define CKGB_FS1_PE3            0x084
#define CKGB_FS1_EN_PRG3        0x088
#define CKGB_FS1_SDIV3          0x08c
#define CKGB_FS1_MD4            0x090
#define CKGB_FS1_PE4            0x094
#define CKGB_FS1_EN_PRG4        0x098
#define CKGB_FS1_SDIV4          0x09c
#define CKGB_FS1_CLKOUT_CTRL    0x0a0
#define CKGB_DISPLAY_CFG        0x0a4
#define CKGB_FS_SELECT          0x0a8
#define CKGB_POWER_DOWN         0x0ac
#define CKGB_POWER_ENABLE       0x0b0
#define CKGB_OUT_CTRL           0x0b4
#define CKGB_CRISTAL_SEL        0x0b8

/* --- CKGC registers ( hardware specific ) --------------------------------- */
#define CKGC_FS0_CFG            0x000
#define CKGC_FS0_MD1            0x010
#define CKGC_FS0_PE1            0x014
#define CKGC_FS0_SDIV1          0x018
#define CKGC_FS0_EN_PRG1        0x01C
#define CKGC_FS0_MD2            0x020
#define CKGC_FS0_PE2            0x024
#define CKGC_FS0_SDIV2          0x028
#define CKGC_FS0_EN_PRG2        0x02C
#define CKGC_FS0_MD3            0x030
#define CKGC_FS0_PE3            0x034
#define CKGC_FS0_SDIV3          0x038
#define CKGC_FS0_EN_PRG3        0x03C

#define PERIPH_BASE                      0xB8000000
#define CKGB_BASE                       (PERIPH_BASE + 0x01000000)
#define CKGC_BASE                       (PERIPH_BASE + 0x01210000)
#define CKGA_BASE                       (PERIPH_BASE + 0x01213000)

/* Exported Types ----------------------------------------------------------- */
typedef struct
{
    U32 sh4_clk;                      /* en Hz */
    U32 sh4_ic_clk;
    U32 sh4_per_clk;
    U32 slim_clk;
    U32 mpeg2_clk;
    U32 lx_dh_clk;
    U32 lx_aud_clk;
    U32 lmi2x_sys_clk;
    U32 lmi2x_vid_clk;
    U32 ic_clk;
    U32 icdiv2_emi_clk;
}CkgA_Clock_t;

typedef enum
{
    HD_FS = 0,
    SD_FS
}Fs_source_t;

typedef struct
{
    U32 hdmi_dll_bch_clk;            /* en Hz */
    U32 hdmi_tmds_clk;
    U32 hdmi_pix_clk;
    U32 pix_hd_clk;
    U32 disp_hd_clk;
    U32 ccir656_clk;
    U32 gdp2_clk;
    U32 disp_sd_clk;
    U32 pix_sd_clk;
    U32 pipe_clk;
    U32 preproc_clk;
    U32 rtc_clk;
    Fs_source_t pix_sd_src;
    Fs_source_t gdp2_src;
}CkgB_Clock_t;

typedef struct
{
    U32 pcm0_clk;            /* en Hz */
    U32 pcm1_clk;
    U32 spdif_clk;
}CkgC_Clock_t;

typedef enum
{
    FS0_out0 = 0,
    FS0_out1,
    FS0_out2,
    FS0_out3,
    FS1_out0,
    FS1_out1,
    FS1_out2,
    FS1_out3,
    FS2_out0,
    FS2_out1,
    FS2_out2
}FS_sel_t;

/* Exported Variables ------------------------------------------------------- */

/* Exported Macros ---------------------------------------------------------- */

/* Exported Functions ------------------------------------------------------- */
BOOL  ST_Ckg_Init(void);
BOOL  ST_Ckg_SetFSClock(FS_sel_t FS_sel, int freq);
int   ST_CkgA_GetClock(CkgA_Clock_t *Clock);
int   ST_CkgB_GetClock(CkgB_Clock_t *Clock);
int   ST_CkgC_GetClock(CkgC_Clock_t *Clock);
BOOL  ST_Ckg_Dbg(void);
#ifdef TESTTOOL_SUPPORT
void CKG_RegisterCommands(void);
#endif

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* __CKG_REG_H */
