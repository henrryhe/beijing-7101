/*******************************************************************************
File Name   : clk_gen.h

Description : Header file of Clock Gen setup

COPYRIGHT (C) STMicroelectronics 2001.

Date               Modification                                     Name
----               ------------                                     ----
20 Feb 2001        Created                                          HSdLM
28 Jun 2001        Set prefix stvtg_ on exported symbols            HSdLM
*******************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __VTG_CLK_GEN_H
#define __VTG_CLK_GEN_H

/* Includes --------------------------------------------------------------- */
#include "vtg_drv.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ----------------------------------------------------- */
#define HTO_MAIN_CTL_PADS           0x000
#define HTO_MAIN_CLK_SEL            0x004
#define HTO_MAIN_SYNC_SEL           0x008
#define HTO_MAIN_SYNC_ACCESS        0x00C
#define HTO_MAIN_PORT_STATUS        0x010
#define HTO_MAIN_MISR_REFSG1        0x070
#define HTO_MAIN_MISR_REFSG2        0x074
#define HTO_MAIN_MISR_REFSG3        0x078
#define HTO_MAIN_MISRTSTCTRL        0x07C
#define HTO_MAIN_MISRTSTSTAT        0x080
#define HTO_MAIN_MISR_SG1           0x084
#define HTO_MAIN_MISR_SG2           0x088
#define HTO_MAIN_MISR_SG3           0x08c

/* From TVO GLUE AUX (0xfd10c500) */
#define HTO_AUX_CTL_PADS            0x000
#define HTO_AUX_CLK_SEL             0x004
#define HTO_AUX_SYNC_SEL            0x008
#define HTO_AUX_SYNC_ACCESS         0x00C
#define HTO_AUX_PORT_CTRL           0x010
#define HTO_AUX_PORT_STATUS         0x014
#define HTO_AUX_INT_ENABLE          0x018
#define HTO_AUX_INT_STATUS          0x01C
#define HTO_AUX_INT_CLR             0x020
#define HTO_AUX_TTXT_CTRL           0x030
#define HTO_AUX_TTXTFIFODATA        0x034
#define HTO_AUX_TTXTFIFO0STATUS     0x038
#define HTO_AUX_TTXTFIFO1STATUS     0x03C
#define HTO_AUX_MISR_REFSG1         0x070
#define HTO_AUX_MISR_REFSG2         0x074
#define HTO_AUX_MISR_REFSG3         0x078
#define HTO_AUX_MISRTSTCTRL         0x07C
#define HTO_AUX_MISRTSTSTAT         0x080
#define HTO_AUX_MISR_SG1            0x084
#define HTO_AUX_MISR_SG2            0x088
#define HTO_AUX_MISR_SG3            0x08c
/* --- SDTVOUT registers (0xfd10f000) ---------------------------------------- */

/* From TVO GLUE (0xfd10f400) */
#define STO_CTL_PADS                0x000
#define STO_CLK_SEL                 0x004
#define STO_SYNC_SEL                0x008
#define STO_SYNC_ACCESS             0x00C
#define STO_PORT_CTRL               0x010
#define STO_PORT_STATUS             0x014
#define STO_INT_ENABLE              0x018
#define STO_INT_STATUS              0x01C
#define STO_INT_CLR                 0x020
#define STO_TTXT_CTRL               0x030
#define STO_TTXTFIFODATA            0x034
#define STO_TTXTFIFO0STATUS         0x038
#define STO_TTXTFIFO1STATUS         0x03C
#define STO_MISR_REFSG1             0x070
#define STO_MISR_REFSG2             0x074
#define STO_MISR_REFSG3             0x078
#define STO_MISRTSTCTRL             0x07C
#define STO_MISRTSTSTAT             0x080
#define STO_MISR_SG1                0x084
#define STO_MISR_SG2                0x088
#define STO_MISR_SG3                0x08c

/* Exported Variables ----------------------------------------------------- */

/* Exported Types --------------------------------------------------------- */
typedef struct
{
    /* FS/PLL outputs */
    U32 Fs0_clk[5];  /* Fsi_clk[0] is the input clock */
    U32 Fs1_clk[5];
    U32 Fs2_clk[5];
    U32 pll0_clk;

    /* PLL0 clocks */
    U32 bdisp_b_266;
    U32 dmu1_b_266_slim_tsg;
    U32 ic_reg_b;
    U32 compo_b_200;
    U32 disp_b_200;
    U32 vdp_b_200;
    U32 emi_master;
    U32 ethernet;

    /* CKGB clocks */
    U32 hdmi_pll;
    U32 pixel_hd0;
    U32 pixel_hd1;
    U32 pipe;
    U32 dmu0_pipe;
    U32 dmu1_pipe;
    U32 disp_pipe;
    U32 vdp_pipe;
    U32 usb48;

    U32 pixel_sd0;
    U32 tsg_sd0;
    U32 periph_27;
    U32 pixel_sd1;
    U32 tsg_sd1;
    U32 clk_216;
    U32 frc2;
    U32 ic_177;

    U32 spare;
    U32 dss;
    U32 ic_166;
    U32 rtc;
    U32 lpc;

    U32 obelix_fvp;
    U32 obelix_main;
    U32 obelix_aux;
    U32 obelix_gdp;
    U32 obelix_vdp;
    U32 obelix_cap;
    U32 asterix_aux;
    U32 tmds;
    U32 ch34mod;
    U32 loc_hdtvo;
    U32 loc_sdtvo;
    U32 rem_sdtvo;

    U32 hdtvo_hdvid_dac;
    U32 hdtvo_pix_main;
    U32 hdtvo_tmds;
    U32 hdtvo_bch;
    U32 hdtvo_fdvo;
    U32 hdtvo_sdvid_dac;
    U32 hdtvo_pix_aux;
    U32 hdtvo_denc;

    U32 sdtvo_pix_main;
    U32 sdtvo_fvdo;
    U32 sdtvo_denc;
} CkgB_Clock_t;

typedef enum
{
    CLKB_DIV1=0x1,
    CLKB_DIV2=0x2,
    CLKB_DIV4=0x4,
    CLKB_DIV8=0x8,
    CLKB_DIV16=0x10,
    CLKB_DIV32=0x20,
    CLKB_DIV64=0x40,
    CLKB_DIV128=0x80,
    CLKB_DIV256=0x100,
    CLKB_DIV512=0x200,
    CLKB_DIV1024=0x400
} Ckg_Divider_t;


typedef enum
{
    CLKB_SYSBCLKIN=0,
    CLKB_FS0_CH1,
    CLKB_FS0_CH2,
    CLKB_FS0_CH3,
    CLKB_FS0_CH4,
    CLKB_FS1_CH1,
    CLKB_FS1_CH2,
    CLKB_FS1_CH3,
    CLKB_FS1_CH4,
    CLKB_FS2_CH1,
    CLKB_FS2_CH2,
    CLKB_FS2_CH3,
    CLKB_FS2_CH4,

    CLKB_HDMI_PLL,
    CLKB_PIX_HD0,
    CLKB_PIX_HD1,
    CLKB_PIPE_DMU01_DISP_VDP,
    CLKB_USB48,

    CLKB_PIX_SD0_TSG_SD0_P27,
    CLKB_PIX_SD1_TSG_SD1,
    CLKB_216,
    CLKB_FRC2,
    CLKB_IC_177,

    CLKB_DSS,
    CLKB_IC_166,
    CLKB_RTC_LPC,

    CLKB_PIX_OBELIX,
    CLKB_PIX_ASTERIX,

    CLKB_OBELIX_FVP_MAIN,
    CLKB_OBELIX_AUX,
    CLKB_OBELIX_GDP,
    CLKB_OBELIX_VDP,
    CLKB_OBELIX_CAP,
    CLKB_ASTERIX_AUX,
    CLKB_TMDS,
    CLKB_CH34MOD,
    CLKB_LOC_HDTVO,
    CLKB_LOC_SDTVO,
    CLKB_REM_SDTVO,

    CLKB_HDTVO_PIX_MAIN,
    CLKB_HDTVO_TMDS,
    CLKB_HDTVO_FDVO,
    CLKB_HDTVO_BCH,

    CLKB_HDTVO_PIX_AUX,
    CLKB_HDTVO_DENC,

    CLKB_SDTVO_PIX_MAIN,
    CLKB_SDTVO_FDVO,
    CLKB_SDTVO_DENC,

    NB_OF_CLOCKB,
    CLKB_EXTERN
} clockName_t;

typedef struct
{
    const char name[30];
    clockName_t id;
    clockName_t sourceid;
    clockName_t sources[2];
    U32 divider;
} CkgB_ClockInstance_t;

typedef enum
{
    CKGMODE_PRIMARY_MAINHD_AUX1SD = 1,
    CKGMODE_ALTERNATE1_MAINED_AUX1SD,
    CKGMODE_ALTERNATE2_MAINSD_AUX1SD
} Ckg_HDTvoutMode_t;

typedef enum
{
    CKGMODE_AUX2HD = 10,
    CKGMODE_AUX2ED,
    CKGMODE_AUX2SD
} Ckg_SDTvoutMode_t;

/* Exported Macros -------------------------------------------------------- */

/* Exported Functions ----------------------------------------------------- */
void stvtg_HALInitClockGen (stvtg_Device_t * const Device_p);
void stvtg_HALSetPixelClock(stvtg_Device_t * const Device_p);
void stvtg_HALEnablePWMclock(stvtg_Device_t * const Device_p);

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* ifndef __VTG_CLK_GEN_H */

/* End of clk_gen.h */
