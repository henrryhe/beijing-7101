/*****************************************************************************
File Name   : clk_gen.c

Description : Pixel Clock configuration.

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                                     Name
----               ------------                                     ----
21 Jul 2000        Created                                           FC
16 Jan 2001        Change audio_reg.h and vos_reg.h inclusion by     FM
                   bereg.h inclusion
19 Feb 2001        Import from STBOOT. Fit for pixel clocks         HSdLM
 *                 configuration (main & aux)
28 Jun 2001        Update work-arounds of HW issues. Use table for  HSdLM
 *                 synthesizer parameter.
04 Sep 2001        When WA_GNBvd06791, program display clock in all HSdLM
 *                 cases.
16 Apr 2002        Update for STi7020.                              HSdLM
 *                 Re-program clocks only if their value has really
 *                 changed.
04 Jul 2002        Fix bug 7020: replace DISPCLK by ADISPCLK        HSdLM
17 Sep 2002        New synthesiser parameters for STi7020 cut 2.0   HSdLM
27 Sep 2002        Correct management of WA_GNBvd06791              HSdLM
21 Jan 2004        Add new CLK (VIDPIX_2XCLK) on 5528               AC
13 Sep 2004        Add support for ST40/OS21                        MH
*****************************************************************************/

/* Includes --------------------------------------------------------------- */
#if !defined(ST_OSLINUX)
#include <stdio.h>
#include <math.h>
#endif

#include "stsys.h"              /* STAPI includes */

#include "stddefs.h"

#include "stcommon.h"

#include "stvtg.h"
#include "vtg_drv.h"
#include "vos_reg.h"

#include "clk_gen.h"
#ifdef WA_PixClkExtern
#include "ics9161.h"
#endif

/* Private types/constants ------------------------------------------------ */

#if defined(ST_5100)|| defined(ST_5105)|| defined(ST_5301)|| defined(ST_5188)|| defined(ST_5525)|| defined(ST_5107)\
 || defined(ST_5162)
#define DEFAULT_PIX_CLK         27000000
/*#elif defined (ST_5525)|| defined(ST_5188)
#define DEFAULT_PIX_CLK         30000000*/
#else
#define DEFAULT_PIX_CLK         148500000
#endif
#define DEFAULT_AUX_CLK        27000000
#define REF_CLK1_CLK            27000000

#define PIXCLK19                19656000
#define PIXCLK21                21621600
#define PIXCLK24                24576000
#define PIXCLK27                27000000
#define DISP_CLOCK_FOR_PIXCLK19 54054000
#define DISP_CLOCK_FOR_PIXCLK21 63000000
#define DISP_CLOCK_FOR_PIXCLK24 70000000
#define DISP_CLOCK_FOR_PIXCLK27 81000000

#ifdef WA_GNBvd06791
#if defined BE_7015
#define DEFAULT_DISP_CLK        81000000
#elif defined BE_7020
#define DEFAULT_DISP_CLK        87000000
#endif
#define MIN_PIXCLK_FOR_DISPCLK  (DEFAULT_DISP_CLK / 3)
#endif /* WA_GNBvd06791 */

#define CKG_SYN1_CLK                        0x30     /*For general control on STi5528*/
#define CKG_SYN1_CLK2                       0x38     /*For USBCLK control on STi5528*/

#define CKG_SYN2_CLK                        0x50      /* For general control on STi5528*/
#define CKG_SYN2_CLK1                       0x54      /* For ST20CLK control on STi5528*/
#define CKG_SYN2_CLK2                       0x58      /* For HDDCLK control on STi5528*/
#define CKG_SYN2_CLK3                       0x5C      /* For SCCLK control on STi5528*/
#define CKG_SYN2_CLK4                       0x60      /* For VIDPIX_2XCLK control on STi5528*/
#define CKG_SYN2_CLK4_RST                   0xF5D5B1E /* Square pixel configuration Reset on STi5528*/


#define PIX_CLK_CTRL                        0x84    /*Pixel clock control on STi5528*/
#define PIX_CLK_CTRL1                       0x00    /*pixel clock (2X) is connected to SYSACLKIN on STi5528*/
#define PIX_CLK_CTRL2                       0x01    /*pixel clock (2X) is connected to SYSBCLKIN on STi5528*/
#define PIX_CLK_CTRL3                       0x02    /*pixel clock (2X) is connected to VIDINCLKIN on STi5528*/
#define PIX_CLK_CTRL4                       0x03    /*pixel clock (2X) is connected to QSYNTH2_CLK4 on STi5528*/
#define PIX_CLK_RST                         0x00    /*pixel clock control reset on STi5528*/

#define PWM_CLK_CTRL                        0x8c    /* PWM clock control */
#define PWM_CLK_STOP                        0x1     /* Stops PWMCLK */

#define CKG_LOCK_REG                        0x90    /* register write lock to prevent inopportune SW access */
#define CKG_LOCK                            0xc1a0  /* lock clock registers acces */
#define CKG_UNLOCK                          0xc0de  /* lock clock registers acces */

#define CKG_SYN2_PE_MASK                 0x0000ffff  /* Fine selector mask with STi5528              */
#define CKG_SYN2_MD_MASK                 0x001f0000  /* Coarse selector mask with STi5528            */
#define CKG_SYN2_SDIV_MASK               0x00e00000  /* Output divider mask  with STi5528            */
#define CKG_SYN_RST_MASK                 0x07000000  /* 3 bits from bit24->bit26 mask with STi5528   */

#define CKG_CLK_STOP                     0x78000000   /*Clock stoped on STi5528*/

#if defined(ST_5100) || defined(ST_5105)|| defined(ST_5301)|| defined(ST_5107) || defined(ST_5162)
    #define CKG_PIX_CLK_SETUP0             0x014
    #define CKG_PIX_CLK_SETUP1             0x018
    #define DCO_MODE_CFG                   0x170
    #define RGST_LK_CFG                    0x300
    #define RGST_LK_CFG_LK                 0x100
    #define RGST_LK_CFG_UNLK               0xff
    #define DCO_MODE_CFG_PRG               0x1
    #define PIX_CLK_SETUP0_RESETN          0x1
    #define PIX_CLK_SETUP0_SELOUT          0x1
    #define PIX_CLK_SETUP0_ENPRG           0x1
    #define PIX_CLK_SETUP0_RST            0x0EBF
    #define PIX_CLK_SETUP1_RST            0x0000
    #define PIX_CLK_SETUP0_MD_MASK         0x1F
    #define PIX_CLK_SETUP0_ENPRG_MASK      0x20
    #define PIX_CLK_SETUP0_SDIV_MASK       0x1C0
    #define PIX_CLK_SETUP0_SELOUT_MASK     0x200
    #define PIX_CLK_SETUP0_RESETN_MASK     0x400
#endif
#if  defined(ST_5188)
    #define CKG_PIX_CLK_SETUP0             0x310
    #define CKG_PIX_CLK_SETUP1             0x314
    #define DCO_MODE_CFG                   0x170
    #define DCO_MODE_CFG_PRG               0x00/*0x1*/
    /*#define PIX_CLK_SETUP0_RESETN          0x1*/
    #define PIX_CLK_SETUP0_SELOUT          0x1
    #define PIX_CLK_SETUP0_ENPRG           0x1
    #define PIX_CLK_SETUP0_RST             0x0EF1
    #define PIX_CLK_SETUP1_RST             0x1C72

    #define PIX_CLK_SETUP0_MD_MASK         0x1F
    #define PIX_CLK_SETUP0_ENPRG_MASK      0x20
    #define PIX_CLK_SETUP0_SDIV_MASK       0x1C0
    #define PIX_CLK_SETUP0_SELOUT_MASK     0x200
#endif
#if defined (ST_5525)
    #define CKG_PIX_CLK_SETUP0             0x350
    #define CKG_PIX_CLK_SETUP1             0x354
    #define DCO_MODE_CFG                   0x170
    #define DCO_MODE_CFG_PRG               0x1
    /*#define PIX_CLK_SETUP0_RESETN          0x1*/
    #define PIX_CLK_SETUP0_SELOUT          0x1
    #define PIX_CLK_SETUP0_ENPRG           0x1
    #define PIX_CLK_SETUP0_RST             0x0EF1
    #define PIX_CLK_SETUP1_RST             0x1C72

    #define PIX_CLK_SETUP0_MD_MASK         0x1F
    #define PIX_CLK_SETUP0_ENPRG_MASK      0x20
    #define PIX_CLK_SETUP0_SDIV_MASK       0x1C0
    #define PIX_CLK_SETUP0_SELOUT_MASK     0x200
#endif
#if defined (ST_7710)
#define VTG2_IT_OFFSET         0x8
#define UNLK_KEY                  0x5AA50000
#define CLK_OFF_PIXHD             0x00000008
#define CLK_OFF_PIXSD             0x00000200
#define CLK_ON_PIXHD              0xFFFFFFF7
#define CLK_ON_PIXSD              0xFFFFFDFF
#define FS_EN_PRG                 0x00000001
#define FS_DIS_PRG                0xFFFFFFFE
#define FS_CLOCKGEN_CFG_0         0x000C0
#define PLL2_CONFIG_2             0x98
#define FS_CLOCKGEN_CFG_2         0xC8
#define FS_12_CONFIG_0            0x6C
#define FS_12_CONFIG_1            0x70
#define FS_04_CONFIG_0            0x0004C
#define FS_11_CONFIG_0            0x00064
#define FS_04_CONFIG_1            0x00050
#define FS_11_CONFIG_1            0x00068
#define CFG_2_PLL2_BYPASS         0x2000
#define PIXEL_SD_CLKSEL_HD_DIV_4  (1<<5)
#define PIXCLK_CLKSEL_CLK_656     (1<<6)
#define DSPCFG_CLK                 0x70

#endif

#if defined (ST_7100)
#define VTG2_IT_OFFSET    0x8
#define PERIPH_BASE             tlm_to_virtual(0xB8000000)
#define CKGB_BASE               (PERIPH_BASE + 0x01000000)
#define CKGB_LOCK               0x010
#define CKGB_DISPLAY_CFG        0x0a4
#define CKGB_FS1_MD2            0x70
#define CKGB_FS1_PE2            0x74

/*-For Cut1.1 7100-*/
#define CKGB_REF_MAX            0x0
#define CKGB_CMD                0x4
#define CKGB_FS1_EN_PRG2        0x78

#define CKGB_CLK_SRC            0x0a8
#define CKGB_FS_SELECT          0x0a8
#define CKGB_REFCLK_SEL         0xb8

#define CKGB_FS0_CFG            0x014
#define CKGB_FS0_MD1            0x018
#define CKGB_FS0_PE1            0x01c
#define CKGB_FS0_EN_PRG1        0x020
#define CKGB_FS0_SDIV1          0x024
#define CKGB_FS1_MD1            0x060
#define CKGB_FS1_PE1            0x064
#define CKGB_FS1_EN_PRG1        0x068
#define CKGB_FS1_SDIV1          0x06c
#define CLK_PIX_SD_1            (1<<10)
#define CLK_PIX_SD_2            (2<<10)
#define CLK_PIX_SD_3            (3<<10)
#define CLK_DISP_ID_1           (1<<8)
#define CLK_DISP_ID_2           (2<<8)
#define CLK_DISP_ID_3           (3<<8)
#define CLK_565_1               (1<<6)
#define CLK_565_2               (2<<6)
#define CLK_565_3               (3<<6)
#define CLK_DISP_HD_1           (1<<4)
#define CLK_DISP_HD_2           (2<<4)
#define CLK_DISP_HD_3           (3<<4)
#define DSPCFG_CLK            0x70
#endif

#if defined (ST_7710)
#define  RESET_CTRL_1            0xd4
#define  PLL2_CFG_0              0x90
#define  PLL2_CFG_1              0x94
#define  PLL2_CFG_2              0x98
#endif

#if defined (ST_7100)|| defined (ST_7109)
#define SERIALIZER_HDMI_RST      0x800
#define PLL_S_HDMI_ENABLE        0x1000
#define PLL_S_HDMI_PDIV          0x4000
#define PLL_S_HDMI_NDIV          0x640000
#define PLL_S_HDMI_MDIV_HD       0x32000000
#define PLL_S_HDMI_MDIV          0x19000000
#define  SYS_CFG3                0x10C
#define  SYS_CFG34               0x188
#endif

#if defined (ST_7109)
#define VTG2_IT_OFFSET          0x8
#define PERIPH_BASE             tlm_to_virtual(0xB8000000)
#define CKGB_BASE               (PERIPH_BASE + 0x01000000)
#define CKGB_LOCK               0x010
#define CKGB_DISPLAY_CFG        0x0a4
#define CKGB_FS1_MD2            0x70
#define CKGB_FS1_PE2            0x74
#define CKGB_CLK_SRC            0x0a8
#define CKGB_FS_SELECT          0x0a8
#define CKGB_FS0_CFG            0x014
#define CKGB_FS0_MD1            0x018
#define CKGB_FS0_PE1            0x01c
#define CKGB_FS0_EN_PRG1        0x020
#define CKGB_FS0_SDIV1          0x024
#define CKGB_FS1_MD1            0x060
#define CKGB_FS1_PE1            0x064
#define CKGB_FS1_EN_PRG1        0x068
#define CKGB_FS1_SDIV1          0x06c
#define CLK_PIX_SD_1            (1<<10)
#define CLK_PIX_SD_2            (2<<10)
#define CLK_PIX_SD_3            (3<<10)
#define CLK_DISP_ID_1           (1<<8)
#define CLK_DISP_ID_2           (2<<8)
#define CLK_DISP_ID_3           (3<<8)
#define CLK_565_1               (1<<6)
#define CLK_565_2               (2<<6)
#define CLK_565_3               (3<<6)
#define CLK_DISP_HD_1           (1<<4)
#define CLK_DISP_HD_2           (2<<4)
#define CLK_DISP_HD_3           (3<<4)

#define CKGB_REFCLK_SEL         0xb8
#define CKGB_REF_MAX            0x0
#define CKGB_CMD                0x4
#define CKGB_FS1_EN_PRG2        0x78
#define DSPCFG_CLK            0x70
#endif

#if defined (ST_7200)|| defined (ST_7111)|| defined (ST_7105)
#define HTO_CTL_PADS            0x00
#define HTO_CLK_SEL             0x04
#define HTO_SYNC_SEL            0x08
/*#define HTO_AUX_CTL_PADS        0x00*/
#define CKGB_OUT_MUX_CFG        0x48
#define HDMI_GPOUT              0x20
#define HDMI_GPOUT_CONF         0x9C10
#define GEN_CFG_OUT             (0x15<<16)
#define CLK_SEL_PIX_MAIN_1      0x1
#define CLK_SEL_PIX_MAIN_2      0x2
#define CLK_SEL_PIX_MAIN_3      0x3
#define CLK_SEL_TMDS_MAIN_1     (1<<2)
#define CLK_SEL_TMDS_MAIN_2     (2<<2)
#define CLK_SEL_TMDS_MAIN_3     (3<<2)
#define CLK_SEL_BCH_MAIN_1      (1<<4)
#define CLK_SEL_BCH_MAIN_2      (2<<4)
#define CLK_SEL_BCH_MAIN_3      (3<<4)
#define CLK_SEL_FDVO_MAIN_1     (1<<6)
#define CLK_SEL_FDVO_MAIN_2     (2<<6)
#define CLK_SEL_FDVO_MAIN_3     (3<<6)
#define FLEXDVO_SYNC_1          0x1
#define FLEXDVO_SYNC_2          0x2
#define FLEXDVO_SYNC_3          0x3
#define HDFORMATTER_SYNC_1      (1<<2)
#define HDFORMATTER_SYNC_2      (2<<2)
#define HDFORMATTER_SYNC_3      (3<<2)
#define HDMI_SYNC_1             (1<<4)
#define HDMI_SYNC_2             (2<<4)
#define HDMI_SYNC_3             (3<<4)
#define DCS_AWG_ED_SYNC_1       (1<<6)
#define DCS_AWG_ED_SYNC_2       (2<<6)
#define DCS_AWG_ED_SYNC_3       (3<<6)
#define CLK_SEL_PIX_1            0x1
#define CLK_SEL_PIX_2            0x2
#define CLK_SEL_PIX_3            0x3
#define DENC_SYNC_1              0x1
#define DENC_SYNC_2              0x2
#define DENC_SYNC_3              0x3

#define S_HDMI_RST_N             0x1
#define PLL_S_HDMI_ENABLE        0x2
#define PLL_S_HDMI_PDIV          0x8
#define PLL_S_HDMI_NDIV          0xC80
#define PLL_S_HDMI_MDIV          0x64000
#define HDMI_PLL                 0x10c

#define DIG1_CFG                 0x100
#define DIG1_SEL_INPUT_RGB       0x01

#endif
#if defined(ST_7200)
#define HD_TVOUT_AUX_GLUE_BASE_ADDRESS  ST7200_HD_TVOUT_AUX_GLUE_BASE_ADDRESS
#define HD_TVOUT_MAIN_GLUE_BASE_ADDRESS ST7200_HD_TVOUT_MAIN_GLUE_BASE_ADDRESS
#define SD_TVOUT_TOPGLUE_BASE_ADDRESS   ST7200_SD_TVOUT_TOPGLUE_BASE_ADDRESS
#define AUDIO_IF_BASE_ADDRESS           ST7200_AUDIO_IF_BASE_ADDRESS
#elif defined(ST_7111)
#define HD_TVOUT_AUX_GLUE_BASE_ADDRESS  ST7111_HD_TVOUT_AUX_GLUE_BASE_ADDRESS
#define HD_TVOUT_MAIN_GLUE_BASE_ADDRESS ST7111_HD_TVOUT_MAIN_GLUE_BASE_ADDRESS
#define SD_TVOUT_TOPGLUE_BASE_ADDRESS   ST7111_SD_TVOUT_TOPGLUE_BASE_ADDRESS
#define AUDIO_IF_BASE_ADDRESS           ST7111_AUDIO_IF_BASE_ADDRESS
#elif defined(ST_7105)
#define HD_TVOUT_AUX_GLUE_BASE_ADDRESS  ST7105_HD_TVOUT_AUX_GLUE_BASE_ADDRESS
#define HD_TVOUT_MAIN_GLUE_BASE_ADDRESS ST7105_HD_TVOUT_MAIN_GLUE_BASE_ADDRESS
#define SD_TVOUT_TOPGLUE_BASE_ADDRESS   ST7105_SD_TVOUT_TOPGLUE_BASE_ADDRESS
#define AUDIO_IF_BASE_ADDRESS           ST7105_AUDIO_IF_BASE_ADDRESS
#endif
#if defined (BE_7015) || defined (BE_7020)
typedef enum
{
    SYNTH3,
#ifdef WA_GNBvd06791
    SYNTH6,
#endif
    SYNTH7
} Clock_t;

/*#if ST_7710*/
#else
typedef enum Clock_e
{
    QSYNTH1_CLK1,
    QSYNTH1_CLK2,
    QSYNTH2_CLK1,
    QSYNTH2_CLK2,
    QSYNTH2_CLK3,
    QSYNTH2_CLK4,
    SYNTH_CLK,                           /* for STi5100 */
    FSYNTH ,                              /* for STi7710 */
    FSY0_4 ,
    FSY1_1
 }Clock_t;
#endif

#if !(defined(ST_5100)|| defined(ST_5105)|| defined(ST_5301)|| defined(ST_5188)|| defined(ST_5525)|| defined(ST_5107) || defined(ST_5162))
typedef struct {       /* Freq synth configuration */
    U32 Fout;
    U32 sdiv;
    U32 md;
    U32 pe;
} FreqSynthElements_t;
#endif
/* Private variables ------------------------------------------------------ */
static BOOL FirstInitDone = FALSE;
/* Private variables ------------------------------------------------------ */
#if defined(BE_7015)
static FreqSynthElements_t FreqSynthElements[] = {   /* Freq synth configuration depending on frequency */
    {  9818181, 0x4, 0x16, 0x7fff},
    {  9828000, 0x4, 0x15, 0x02d0},
    { 10800000, 0x4, 0x13, 0x0000},
    { 10810800, 0x4, 0x13, 0x028e},
    { 12272727, 0x4, 0x11, 0x3333},
    { 12285000, 0x4, 0x11, 0x3573},
    { 12288000, 0x4, 0x11, 0x3600},
    { 13500000, 0x4, 0x0f, 0x0000},
    { 13513500, 0x3, 0x1f, 0x0417},
    { 14750000, 0x3, 0x1d, 0x5b1e},
    { 24545454, 0x3, 0x11, 0x3333},   /* also 2 x 12272727 */
    { 24570000, 0x3, 0x11, 0x3573},   /* also 2 x 12285000 */
    { 27000000, 0x3, 0x0f, 0x0000},   /* also 2 x 13500000 */
    { 27027000, 0x2, 0x1f, 0x0417},   /* also 2 x 13513500 */
    { 29670329, 0x2, 0x1d, 0x70a3},
    { 29700000, 0x2, 0x1d, 0x745d},
    { 37087912, 0x2, 0x17, 0x5a1c},
    { 37125000, 0x2, 0x17, 0x5d17},
    { 63000000, 0x1, 0x1b, 0x4924},   /* also closest <= 3 x 21600000 and 3 x 21621600 */
    { 74175824, 0x1, 0x17, 0x5a1c},   /* also 2 x 37087912 */
    { 74250000, 0x1, 0x17, 0x5d17},   /* also 2 x 37125000 */
    { 81000000, 0x1, 0x15, 0x5555},
    {148351648, 0x0, 0x17, 0x5a1c},   /* also 2 x 74175824 */
    {148500000, 0x0, 0x17, 0x5d17},   /* also 2 x 74250000 */

    { 19636362, 0x3, 0x16, 0x7fff},   /* 2 x 9818181 */
    { 19656000, 0x3, 0x15, 0x02d0},   /* 2 x 9828000 */
    { 21600000, 0x3, 0x13, 0x0000},   /* 2 x 10800000 */
    { 21621600, 0x3, 0x13, 0x028e},   /* 2 x 10810800*/
    { 24576000, 0x3, 0x11, 0x3600},   /* 2 x 12288000 */
    { 29500000, 0x2, 0x1d, 0x5b1e},   /* 2 x 14750000 */
    { 49090908, 0x2, 0x11, 0x3333},   /* 2 x 24545454 */
    { 49140000, 0x2, 0x11, 0x3573},   /* 2 x 24570000 */
    { 54000000, 0x2, 0x0f, 0x0000},   /* 2 x 27000000 */
    { 54054000, 0x1, 0x1f, 0x0417},   /* 2 x 27027000 */ /* closest <= 3 x 19636362 and 3 x 19656000*/
    { 59340658, 0x1, 0x1d, 0x70a3},   /* 2 x 29670329 */
    { 59400000, 0x1, 0x1d, 0x745d},   /* 2 x 29700000 */
    {126000000, 0x0, 0x1b, 0x4924},   /* 2 x 63000000 */
    {162000000, 0x0, 0x15, 0x5555},   /* 2 x 81000000 */

    { 70000000, 0x1, 0x18, 0x283a}    /* closest <= 3 x 24545454 and 3 x 24570000 and 3 x 24576000 */
};
#elif defined(BE_7020) /* parameters for STi7020 cut 2.0 */
static FreqSynthElements_t FreqSynthElements[] = {   /* Freq synth configuration depending on frequency */
    { 9818181,   0x4, 0x5, 0x4000},
    { 9828000,   0x4, 0x5, 0x40b4},
    { 10800000,  0x4, 0x4, 0x0000},
    { 10810800,  0x4, 0x4, 0x00a4},
    { 12272727,  0x4, 0x4, 0x4ccd},
    { 12285000,  0x4, 0x4, 0x4d5d},
    { 12288000,  0x4, 0x4, 0x4d80},
    { 13500000,  0x3, 0x7, 0x0000},
    { 13513500,  0x3, 0x7, 0x0106},
    { 14750000,  0x3, 0x7, 0x56c8},
    { 24545454,  0x3, 0x4, 0x4ccd},   /* also 2 x 12272727 */
    { 24570000,  0x3, 0x4, 0x4d5d},   /* also 2 x 12285000 */
    { 27000000,  0x2, 0x7, 0x0000},   /* also 2 x 13500000 */
    { 27027000,  0x2, 0x7, 0x0106},   /* also 2 x 13513500 */
    { 29670329,  0x2, 0x7, 0x5c29},
    { 29700000,  0x2, 0x7, 0x5d17},
    { 37087912,  0x2, 0x5, 0x1687},
    { 37125000,  0x2, 0x5, 0x1746},
    { 63000000,  0x1, 0x6, 0x1249},   /* also closest <= 3 x 21600000 and 3 x 21621600 */
    { 74175824,  0x1, 0x5, 0x1687},   /* also 2 x 37087912 */
    { 74250000,  0x1, 0x5, 0x1746},   /* also 2 x 37125000 */
    { 81000000,  0x1, 0x5, 0x5555},
    { 87000000,  0x1, 0x4, 0x046a},
    { 148351648, 0x0, 0x5, 0x1687},   /* also 2 x 74175824 */
    { 148500000, 0x0, 0x5, 0x1746},   /* also 2 x 74250000 */

    { 19636362,  0x3, 0x5, 0x4000},   /* 2 x 9818181 */
    { 19656000,  0x3, 0x5, 0x40b4},   /* 2 x 9828000 */
    { 21600000,  0x3, 0x4, 0x0000},   /* 2 x 10800000 */
    { 21621600,  0x3, 0x4, 0x00a4},   /* 2 x 10810800*/
    { 24576000,  0x3, 0x4, 0x4d80},   /* 2 x 12288000 */
    { 29500000,  0x2, 0x7, 0x56c8},   /* 2 x 14750000 */
    { 49090908,  0x2, 0x4, 0x4ccd},   /* 2 x 24545454 */
    { 49140000,  0x2, 0x4, 0x4d5d},   /* 2 x 24570000 */
    { 54000000,  0x1, 0x7, 0x0000},   /* 2 x 27000000 */
    { 54054000,  0x1, 0x7, 0x0106},   /* 2 x 27027000 */ /* closest <= 3 x 19636362 and 3 x 19656000 */
    { 59340658,  0x1, 0x7, 0x5c29},   /* 2 x 29670329 */
    { 59400000,  0x1, 0x7, 0x5d17},   /* 2 x 29700000 */
    { 126000000, 0x0, 0x6, 0x1249},   /* 2 x 63000000 */
    { 144000000, 0x0, 0x5, 0x0000},   /* 2 x 72000000 */
    { 162000000, 0x0, 0x5, 0x5555},   /* 2 x 81000000 */

    { 70000000,  0x1, 0x6, 0x6a0f},   /* closest <= 3 x 24545454 and 3 x 24570000 and 3 x 24576000 */
    { 72000000,  0x1, 0x5, 0x0000}
};

#elif defined (ST_5528)  /* For 5528 */
static FreqSynthElements_t FreqSynthElements[] = {   /* Freq synth configuration depending on frequency */
    {  9818181, 0x4, 0x16, 0x7fff},   /* must be verified */
    {  9828000, 0x4, 0x15, 0x02d0},   /* must be verified */
    { 10800000, 0x4, 0x13, 0x0000},   /* must be verified */
    { 10810800, 0x4, 0x13, 0x028e},   /* must be verified */
    { 12272727, 0x4, 0x11, 0x3333},   /* must be verified */
    { 12285000, 0x4, 0x11, 0x3573},   /* must be verified */
    { 12288000, 0x4, 0x11, 0x3600},   /* must be verified */
    { 13500000, 0x4, 0x0f, 0x0000},   /* must be verified */
    { 13513500, 0x3, 0x1f, 0x0417},   /* must be verified */
    { 14750000, 0x3, 0x1d, 0x5b1e},   /* must be verified */
    { 24545454, 0x3, 0x11, 0x3334},   /* also 2 x 12272727:OK */
    { 24570000, 0x3, 0x11, 0x3334},   /* also 2 x 12285000 :must be verified*/
    { 27000000, 0x3, 0x0f, 0x0000},   /* also 2 x 13500000 :must be verified*/
    { 27027000, 0x2, 0x1f, 0x0417},   /* also 2 x 13513500 :must be verified*/
    { 29670329, 0x2, 0x1d, 0x70a3},   /* must be verified */
    { 29700000, 0x2, 0x1d, 0x745d},   /* must be verified */
    { 37087912, 0x2, 0x17, 0x5a1c},   /* must be verified */
    { 37125000, 0x2, 0x17, 0x5d17},   /* must be verified */
    { 63000000, 0x1, 0x1b, 0x4924},   /* also closest <= 3 x 21600000 and 3 x 21621600:must be verified */
    { 74175824, 0x1, 0x17, 0x5a1c},   /* also 2 x 37087912 :must be verified*/
    { 74250000, 0x1, 0x17, 0x5d17},   /* also 2 x 37125000 :must be verified*/
    { 81000000, 0x1, 0x15, 0x5555},   /* must be verified */
    {148351648, 0x0, 0x17, 0x5a1c},   /* also 2 x 74175824 :must be verified*/
    {148500000, 0x0, 0x17, 0x5d17},   /* also 2 x 74250000 :must be verified*/

    { 19636362, 0x3, 0x16, 0x7fff},   /* 2 x 9818181  :must be verified*/
    { 19656000, 0x3, 0x15, 0x02d0},   /* 2 x 9828000  :must be verified*/
    { 21600000, 0x3, 0x13, 0x0000},   /* 2 x 10800000 :must be verified*/
    { 21621600, 0x3, 0x13, 0x028e},   /* 2 x 10810800 :must be verified*/
    { 24576000, 0x3, 0x11, 0x3600},   /* 2 x 12288000 :must be verified*/
    { 29500000, 0x2, 0x1d, 0x5b1f},   /* 2 x 14750000 :OK*/
    { 49090908, 0x2, 0x11, 0x3333},   /* 2 x 24545454 :must be verified*/
    { 49140000, 0x2, 0x11, 0x3573},   /* 2 x 24570000 :must be verified*/
    { 54000000, 0x2, 0x0f, 0x0000},   /* 2 x 27000000 :must be verified*/
    { 54054000, 0x1, 0x1f, 0x0417},   /* 2 x 27027000 */ /* closest <= 3 x 19636362 and 3 x 19656000:must be verified*/
    { 59340658, 0x1, 0x1d, 0x70a3},   /* 2 x 29670329 :must be verified*/
    { 59400000, 0x1, 0x1d, 0x745d},   /* 2 x 29700000 :must be verified*/
    {126000000, 0x0, 0x1b, 0x4924},   /* 2 x 63000000 :must be verified*/
    {162000000, 0x0, 0x15, 0x5555},   /* 2 x 81000000 :must be verified*/

    { 70000000, 0x1, 0x18, 0x283a}    /* closest <= 3 x 24545454 and 3 x 24570000 and 3 x 24576000:must be verified */
};
#endif

#if defined (ST_7200)|| defined (ST_7111)|| defined (ST_7105)

/* --- CKGB registers ( hardware specific ) ---------------------------------------- */

#define CKGB_FS0_SETUP              0x000
#define CKGB_FS1_SETUP              0x004
#define CKGB_FS2_SETUP              0x008

/* --- Audio CFG registers (0xfd601000) -------------------------------------- */
#define AUDCFG_FSYNA_CFG        0x00000000
#define AUDCFG_FSYNB_CFG        0x00000100

typedef enum
{
    FS0_out1=1,
    FS0_out2,
    FS0_out3,
    FS0_out4,
    FS1_out1,
    FS1_out2,
    FS1_out3,
    FS1_out4,
    FS2_out1,
    FS2_out2,
    FS2_out3,
    FS2_out4,
    FS3_out1,
    FS3_out2,
    FS3_out3,
    FS3_out4,
    FS4_out1,
    FS4_out2,
    FS4_out3,
    FS4_out4
} FS_sel_t;

typedef enum
{
    CKGMODE_NO_CONFIGURATION = 0,
    CKGMODE_HD,
    CKGMODE_ED,
    CKGMODE_SD
} Ckg_Mode_t;

static Ckg_Mode_t ModePrimary = CKGMODE_NO_CONFIGURATION;
static Ckg_Mode_t ModeLocal = CKGMODE_NO_CONFIGURATION;
static Ckg_Mode_t ModeRemote = CKGMODE_NO_CONFIGURATION;

#ifndef STVTG_USE_CLKRV
static U32 refclockB=30;
static U32 refclockC=30;
#endif

static CkgB_ClockInstance_t CkgB_Clocks[]=
{
/* clock name                 clock id                 default source id     possible source id                 ratio  */
 { "CLKB_SYSBCLKIN",          CLKB_SYSBCLKIN,          CLKB_EXTERN,          {CLKB_EXTERN,CLKB_EXTERN},       CLKB_DIV1},
 { "CLKB_FS0_CH1",            CLKB_FS0_CH1,            CLKB_EXTERN,          {CLKB_EXTERN,CLKB_EXTERN},       CLKB_DIV1},
 { "CLKB_FS0_CH2",            CLKB_FS0_CH2,            CLKB_EXTERN,          {CLKB_EXTERN,CLKB_EXTERN},       CLKB_DIV1},
 { "CLKB_FS0_CH3",            CLKB_FS0_CH3,            CLKB_EXTERN,          {CLKB_EXTERN,CLKB_EXTERN},       CLKB_DIV1},
 { "CLKB_FS0_CH4",            CLKB_FS0_CH4,            CLKB_EXTERN,          {CLKB_EXTERN,CLKB_EXTERN},       CLKB_DIV1},
 { "CLKB_FS1_CH1",            CLKB_FS1_CH1,            CLKB_EXTERN,          {CLKB_EXTERN,CLKB_EXTERN},       CLKB_DIV1},
 { "CLKB_FS1_CH2",            CLKB_FS1_CH2,            CLKB_EXTERN,          {CLKB_EXTERN,CLKB_EXTERN},       CLKB_DIV1},
 { "CLKB_FS1_CH3",            CLKB_FS1_CH3,            CLKB_EXTERN,          {CLKB_EXTERN,CLKB_EXTERN},       CLKB_DIV1},
 { "CLKB_FS1_CH4",            CLKB_FS1_CH4,            CLKB_EXTERN,          {CLKB_EXTERN,CLKB_EXTERN},       CLKB_DIV1},
 { "CLKB_FS2_CH1",            CLKB_FS2_CH1,            CLKB_EXTERN,          {CLKB_EXTERN,CLKB_EXTERN},       CLKB_DIV1},
 { "CLKB_FS2_CH2",            CLKB_FS2_CH2,            CLKB_EXTERN,          {CLKB_EXTERN,CLKB_EXTERN},       CLKB_DIV1},
 { "CLKB_FS2_CH3",            CLKB_FS2_CH3,            CLKB_EXTERN,          {CLKB_EXTERN,CLKB_EXTERN},       CLKB_DIV1},
 { "CLKB_FS2_CH4",            CLKB_FS2_CH4,            CLKB_EXTERN,          {CLKB_EXTERN,CLKB_EXTERN},       CLKB_DIV1},

 { "CLKB_HDMI_PLL",           CLKB_HDMI_PLL,           CLKB_FS0_CH1,         {CLKB_FS0_CH1,CLKB_FS0_CH1},     CLKB_DIV1},
 { "CLKB_PIX_HD0",            CLKB_PIX_HD0,            CLKB_FS0_CH1,         {CLKB_FS0_CH1,CLKB_FS0_CH1},     CLKB_DIV1},
 { "CLKB_PIX_HD1",            CLKB_PIX_HD1,            CLKB_FS0_CH2,         {CLKB_FS0_CH2,CLKB_FS0_CH2},     CLKB_DIV1},
 { "CLKB_PIPE_DMU01_DISP_VDP",CLKB_PIPE_DMU01_DISP_VDP,CLKB_FS0_CH3,         {CLKB_FS0_CH3,CLKB_FS0_CH3},     CLKB_DIV1},
 { "CLKB_USB48",              CLKB_USB48,              CLKB_FS0_CH4,         {CLKB_FS0_CH4,CLKB_FS0_CH4},     CLKB_DIV1},

 { "CLKB_PIX_SD0_TSG_SD0_P27",CLKB_PIX_SD0_TSG_SD0_P27,CLKB_FS1_CH1,         {CLKB_FS1_CH1,CLKB_FS1_CH1},     CLKB_DIV8},
 { "CLKB_PIX_SD1_TSG_SD1",    CLKB_PIX_SD1_TSG_SD1,    CLKB_FS1_CH2,         {CLKB_FS1_CH2,CLKB_FS1_CH2},     CLKB_DIV8},
 { "CLKB_216",                CLKB_216,                CLKB_FS1_CH1,         {CLKB_FS1_CH1,CLKB_FS1_CH2},     CLKB_DIV1},
 { "CLKB_FRC2",               CLKB_FRC2,               CLKB_FS1_CH3,         {CLKB_FS1_CH3,CLKB_FS1_CH3},     CLKB_DIV1},
 { "CLKB_IC_177",             CLKB_IC_177,             CLKB_FS1_CH4,         {CLKB_FS1_CH4,CLKB_FS1_CH4},     CLKB_DIV1},

 { "CLKB_DSS",                CLKB_DSS,                CLKB_FS2_CH2,         {CLKB_FS2_CH2,CLKB_FS2_CH2},     CLKB_DIV1},
 { "CLKB_IC_166",             CLKB_IC_166,             CLKB_FS2_CH3,         {CLKB_FS2_CH3,CLKB_FS2_CH3},     CLKB_DIV1},
 { "CLKB_RTC_LPC",            CLKB_RTC_LPC,            CLKB_FS2_CH4,         {CLKB_FS2_CH4,CLKB_FS2_CH4},     CLKB_DIV512},

 { "CLKB_PIX_OBELIX",         CLKB_PIX_OBELIX,         CLKB_PIX_HD0,         {CLKB_PIX_SD0_TSG_SD0_P27,CLKB_PIX_HD0},   CLKB_DIV1},
 { "CLKB_PIX_ASTERIX",        CLKB_PIX_ASTERIX,        CLKB_PIX_HD0,         {CLKB_PIX_SD1_TSG_SD1,CLKB_PIX_HD0},       CLKB_DIV1},

 { "CLKB_OBELIX_FVP_MAIN",    CLKB_OBELIX_FVP_MAIN,    CLKB_PIX_HD0,         {CLKB_PIX_HD0,CLKB_PIX_HD0},               (CLKB_DIV1 | CLKB_DIV2 | CLKB_DIV4 | CLKB_DIV8)},
 { "CLKB_OBELIX_AUX",         CLKB_OBELIX_AUX,         CLKB_PIX_OBELIX,      {CLKB_PIX_OBELIX,CLKB_PIX_OBELIX},         (CLKB_DIV1 | CLKB_DIV2 | CLKB_DIV4 | CLKB_DIV8)},
 { "CLKB_OBELIX_GDP",         CLKB_OBELIX_GDP,         CLKB_OBELIX_FVP_MAIN, {CLKB_OBELIX_FVP_MAIN,CLKB_OBELIX_AUX},    CLKB_DIV1},
 { "CLKB_OBELIX_VDP",         CLKB_OBELIX_VDP,         CLKB_OBELIX_FVP_MAIN, {CLKB_OBELIX_FVP_MAIN,CLKB_OBELIX_AUX},    CLKB_DIV1},
 { "CLKB_OBELIX_CAP",         CLKB_OBELIX_CAP,         CLKB_OBELIX_FVP_MAIN, {CLKB_OBELIX_FVP_MAIN,CLKB_OBELIX_AUX},    CLKB_DIV1},
 { "CLKB_ASTERIX_AUX",        CLKB_ASTERIX_AUX,        CLKB_PIX_ASTERIX,     {CLKB_PIX_ASTERIX,CLKB_PIX_ASTERIX},       (CLKB_DIV1 | CLKB_DIV2 | CLKB_DIV4 | CLKB_DIV8)},
 { "CLKB_TMDS",               CLKB_TMDS,               CLKB_PIX_HD0,         {CLKB_PIX_HD0,CLKB_PIX_HD0},               (CLKB_DIV1 | CLKB_DIV2 | CLKB_DIV4 | CLKB_DIV8)},
 { "CLKB_CH34MOD",            CLKB_CH34MOD,            CLKB_PIX_OBELIX,      {CLKB_PIX_OBELIX,CLKB_PIX_OBELIX},         (CLKB_DIV1 | CLKB_DIV2 | CLKB_DIV4 | CLKB_DIV8)},

 { "CLKB_LOC_HDTVO",          CLKB_LOC_HDTVO,          CLKB_PIX_HD0,         {CLKB_PIX_HD0,CLKB_PIX_HD0},               CLKB_DIV1},
 { "CLKB_LOC_SDTVO",          CLKB_LOC_SDTVO,          CLKB_PIX_HD0,         {CLKB_PIX_SD0_TSG_SD0_P27,CLKB_PIX_HD0},   CLKB_DIV1},
 { "CLKB_REM_SDTVO",          CLKB_REM_SDTVO,          CLKB_PIX_HD0,         {CLKB_PIX_SD1_TSG_SD1,CLKB_PIX_HD0},       CLKB_DIV1},

 { "CLKB_HDTVO_PIX_MAIN",     CLKB_HDTVO_PIX_MAIN,     CLKB_LOC_HDTVO,       {CLKB_LOC_HDTVO,CLKB_LOC_HDTVO},           (CLKB_DIV1 | CLKB_DIV2 | CLKB_DIV4 | CLKB_DIV8)},
 { "CLKB_HDTVO_TMDS",         CLKB_HDTVO_TMDS,         CLKB_LOC_HDTVO,       {CLKB_LOC_HDTVO,CLKB_LOC_HDTVO},           (CLKB_DIV1 | CLKB_DIV2 | CLKB_DIV4 | CLKB_DIV8)},
 { "CLKB_HDTVO_FDVO",         CLKB_HDTVO_FDVO,         CLKB_LOC_HDTVO,       {CLKB_LOC_HDTVO,CLKB_LOC_HDTVO},           (CLKB_DIV1 | CLKB_DIV2 | CLKB_DIV4 | CLKB_DIV8)},
 { "CLKB_HDTVO_BCH",          CLKB_HDTVO_BCH,          CLKB_LOC_HDTVO,       {CLKB_LOC_HDTVO,CLKB_LOC_HDTVO},           (CLKB_DIV1 | CLKB_DIV2 | CLKB_DIV4 | CLKB_DIV8)},

 { "CLKB_HDTVO_PIX_AUX",      CLKB_HDTVO_PIX_AUX,      CLKB_LOC_SDTVO,       {CLKB_LOC_SDTVO,CLKB_LOC_SDTVO},           (CLKB_DIV1 | CLKB_DIV2 | CLKB_DIV4 | CLKB_DIV8)},
 { "CLKB_HDTVO_DENC",         CLKB_HDTVO_DENC,         CLKB_LOC_SDTVO,       {CLKB_LOC_SDTVO,CLKB_LOC_SDTVO},           (CLKB_DIV1 | CLKB_DIV2 | CLKB_DIV4 | CLKB_DIV8)},

 { "CLKB_SDTVO_PIX_MAIN",     CLKB_SDTVO_PIX_MAIN,     CLKB_REM_SDTVO,       {CLKB_REM_SDTVO,CLKB_REM_SDTVO},           (CLKB_DIV1 | CLKB_DIV2 | CLKB_DIV4 | CLKB_DIV8)},
 { "CLKB_SDTVO_FDVO",         CLKB_SDTVO_FDVO,         CLKB_REM_SDTVO,       {CLKB_REM_SDTVO,CLKB_REM_SDTVO},           (CLKB_DIV1 | CLKB_DIV2 | CLKB_DIV4 | CLKB_DIV8)},
 { "CLKB_SDTVO_DENC",         CLKB_SDTVO_DENC,         CLKB_REM_SDTVO,       {CLKB_REM_SDTVO,CLKB_REM_SDTVO},           (CLKB_DIV1 | CLKB_DIV2 | CLKB_DIV4 | CLKB_DIV8)}

};

#endif /*7200*/


/* Private Macros --------------------------------------------------------- */

#ifdef ST_OSLINUX
#define stvtg_Read32(a)         STSYS_ReadRegDev32LE((void*)(a))
#else
#define stvtg_Read32(a)         STSYS_ReadRegDev32LE((a))
#endif

/* Private function prototypes -------------------------------------------- */
#if !(defined(ST_5100)|| defined(ST_7710)|| defined(ST_5105) || defined(ST_7100)|| defined(ST_5301)|| defined(ST_7109) \
   || defined(ST_5188)|| defined(ST_5525)|| defined(ST_5107) || defined(ST_7200)|| defined(ST_5162)|| defined(ST_7111) \
   || defined(ST_7105))
static void GetSynthParam(const U32 Fout, U32 * const ndiv, U32 * const sdiv, U32 * const md, U32 * const pe);
#endif
static void ConfigSynth(   const stvtg_Device_t * const Device_p
                         , const Clock_t Synth
                         , const U32 OutputClockFreq);
#if defined(ST_5100)|| defined(ST_5105)|| defined(ST_5301)|| defined(ST_5188)|| defined(ST_5525)|| defined(ST_5107)|| defined(ST_5162)
static BOOL ComputeFrequencySynthesizerParameters(const U32 Fin , const U32 Fout, U32 * const ndiv, U32 * const sdiv,U32 * const md, U32 * const pe);
#endif
#if defined ST_7710
void Stop_PLL_Rej(const U32 CKGBasAddress);
void Change_Freq_PixHD_CLK(const U32 CKGBasAddress,U32 sdiv, U32 md, U32 pe);
void Start_PLL_Rej(const U32 CKGBasAddress);
void Change_Freq_Pipe_CLK(const U32 CKGBasAddress,U32 sdiv, U32 md, U32 pe);
#endif

#if defined (ST_7200)|| defined (ST_7111)|| defined (ST_7105)
static U32 Ckg_FindClockInstanceIndex(clockName_t Name);
static U32 Ckg_GetClockConfig(void);
static void Ckg_UpdateClkCfg(U32 cfg);
static void Ckg_SetClockDivider( clockName_t clockid, Ckg_Divider_t divider );
static BOOL Ckg_SetClockSource (clockName_t destclockid, clockName_t srcclockid);
#ifndef STVTG_USE_CLKRV
static boolean Ckg_GetFSParameterFromFreq( int inputfreq, int outputfreq, int *md, int *pe, int *sdiv );
static BOOL Ckg_SetFSCHClock(FS_sel_t FS_sel, int freq);
#endif
static BOOL Ckg_SetMode( Ckg_Mode_t Mode_HDTVOUT_Primary, Ckg_Mode_t Mode_HDTVOUT_Local, Ckg_Mode_t Mode_SDTVOUT_Remote );
#endif

/* Global variables ------------------------------------------------------- */
#if defined (ST_7710)
#ifdef WA_GNBvd35956
static void Change_Freq_Disp_CLK(const U32 CKGBASEAddress, const U32 sdiv, const U32 md, const U32 pe);
#endif
#endif

/* Function definitions --------------------------------------------------- */

/*****************************************************************************
Name        : stvtg_HALEnablePWMclock
Description : Enable PWM clock on 5528 ST40 side
Parameters  : Device_p (IN) : pointer on VTG device to access
Assumptions :
Limitations :
Returns     : None
*****************************************************************************/
void stvtg_HALEnablePWMclock( stvtg_Device_t * const Device_p)
{
#if defined(ST_OS21) || defined(ST_OSWINCE)
    U32 CKGBaseAddress ;
    CKGBaseAddress = (U32)Device_p->CKGBaseAddress_p ;

    STSYS_WriteRegDev32LE(CKGBaseAddress + CKG_LOCK_REG,CKG_UNLOCK);
    STSYS_WriteRegDev32LE(CKGBaseAddress + PWM_CLK_CTRL, ~PWM_CLK_STOP);
    STSYS_WriteRegDev32LE(CKGBaseAddress + CKG_LOCK_REG, CKG_LOCK);
#else
    UNUSED_PARAMETER(Device_p);
#endif
}

#if defined (ST_7200)|| defined (ST_7111)|| defined (ST_7105)
/*****************************************************************************
Name        : Ckg_FindClockInstanceIndex
Description : Find the index of the clock in CkgB_ClockInstance_t table
Parameters  :
Assumptions :
Limitations :
Returns     : Clock Glue configuration
*****************************************************************************/
static U32 Ckg_FindClockInstanceIndex(clockName_t Name)
{
    U32 i;

    for (i=0;i<NB_OF_CLOCKB;i++)
    {
        if (CkgB_Clocks[i].id == Name) break;
    }

    return i;
}

/*****************************************************************************
Name        : GetClockConfig
Description : Retrieve the clock configuration of CKGB
Parameters  :
Assumptions :
Limitations :
Returns     : Clock Glue configuration
*****************************************************************************/
static U32 Ckg_GetClockConfig(void)
{
    U32 ckg_glue_cfg, data;

    ckg_glue_cfg=0;
    data= STSYS_ReadRegDev32LE(HD_TVOUT_AUX_GLUE_BASE_ADDRESS + HTO_AUX_CTL_PADS);
    data= (data>>15)&0x1FFFF;
    ckg_glue_cfg |= (data<<8);
    data= STSYS_ReadRegDev32LE(HDMI_BASE_ADDRESS + HDMI_GPOUT);
    data= (data>>8)&0xFF;
    ckg_glue_cfg |= (data);

    return (ckg_glue_cfg);
}

/*****************************************************************************
Name        : UpdateClkCfg
Description : Update the clock configuration of CKGB
 clk_cfg (7 :0 ) = hdmi_gpout (15 :8)
 clk_cfg (8) = hdtvout_top_aux_glue_pad(15)
 clk_cfg (24:9)= hdtvout_top_aux_glue_pad(31:16)
Parameters  :
Assumptions :
Limitations :
Returns     : None
*****************************************************************************/
static void Ckg_UpdateClkCfg(U32 cfg)
{
    U32 data=0;
	data= STSYS_ReadRegDev32LE(HD_TVOUT_AUX_GLUE_BASE_ADDRESS + HTO_AUX_CTL_PADS);
   	data = data & (0x00007FFF);
    data = data | ((((cfg>>8) & 0x1FFFF))<<15);
	STSYS_WriteRegDev32LE( HD_TVOUT_AUX_GLUE_BASE_ADDRESS + HTO_AUX_CTL_PADS , data );
	data= STSYS_ReadRegDev32LE(HDMI_BASE_ADDRESS + HDMI_GPOUT);
    data = data & 0xFFFF00FF ;
    data = data | ((cfg & 0xFF)<< 8);
    STSYS_WriteRegDev32LE( HDMI_BASE_ADDRESS + HDMI_GPOUT, data );
}/* end of UpdateClkCfg() */

/*****************************************************************************
Name        : Ckg_SetClockDivider
Description : Set Clock configuration divider
Parameters  :
Assumptions :
Limitations :
Returns     :
*****************************************************************************/
static void Ckg_SetClockDivider( clockName_t clockid, Ckg_Divider_t divider )
{
    U32 ckg_cfg, div, hdtvo_cfg, hdtvo_auxcfg, sdtvo_cfg;
    U32 clockindex;

    div=0x0;
    ckg_cfg= Ckg_GetClockConfig();
	hdtvo_cfg= STSYS_ReadRegDev32LE( HD_TVOUT_MAIN_GLUE_BASE_ADDRESS + HTO_MAIN_CLK_SEL );
    hdtvo_auxcfg= STSYS_ReadRegDev32LE( HD_TVOUT_AUX_GLUE_BASE_ADDRESS + HTO_AUX_CLK_SEL );
    sdtvo_cfg= STSYS_ReadRegDev32LE( SD_TVOUT_TOPGLUE_BASE_ADDRESS + STO_CLK_SEL );
	clockindex = Ckg_FindClockInstanceIndex(clockid);

    if (((CkgB_Clocks[clockindex].divider) & divider) == 0)
    {
        /*STTBX_Print(("Ckg_SetClockDivider: divider %d is not supported on clock %s\n", divider, CkgB_Clocks[clockindex].name));*/
        /*return (TRUE); */
    }

    if (divider==1)
        {div=0x0;}
    else if (divider==2)
        {div=0x1;}
    else if (divider==4)
        {div=0x2;}
    else if (divider==8)
        {div=0x3;}

    switch (clockid)
    {
        case CLKB_OBELIX_AUX:
            ckg_cfg = (ckg_cfg & 0xfffff9ff )| div<<9;
            Ckg_UpdateClkCfg(ckg_cfg) ;
            break;
        case CLKB_OBELIX_FVP_MAIN:
            ckg_cfg = (ckg_cfg & 0xfffffe7f) | div<<7;
            Ckg_UpdateClkCfg(ckg_cfg) ;
            break;
        case CLKB_ASTERIX_AUX:
            ckg_cfg = (ckg_cfg & 0xffffe7ff) | div<<11;
            Ckg_UpdateClkCfg(ckg_cfg) ;
            break;
        case CLKB_TMDS:
            ckg_cfg = (ckg_cfg & 0xffff9fff) | div<<13;
            Ckg_UpdateClkCfg(ckg_cfg) ;
            break;
        case CLKB_CH34MOD:
            ckg_cfg = (ckg_cfg & 0xfffe7fff) | div<<15;
            Ckg_UpdateClkCfg(ckg_cfg) ;
            break;
        case CLKB_HDTVO_PIX_MAIN:
            hdtvo_cfg = (hdtvo_cfg & ( ~(U32)(0x3))) | div;
            STSYS_WriteRegDev32LE( HD_TVOUT_MAIN_GLUE_BASE_ADDRESS + HTO_MAIN_CLK_SEL , hdtvo_cfg );
			break;
        case CLKB_HDTVO_TMDS:
            hdtvo_cfg = (hdtvo_cfg & ( ~(U32)(0x3<<2))) | div<<2;
			STSYS_WriteRegDev32LE( HD_TVOUT_MAIN_GLUE_BASE_ADDRESS + HTO_MAIN_CLK_SEL , hdtvo_cfg );
			break;
        case CLKB_HDTVO_BCH:
            hdtvo_cfg = (hdtvo_cfg & ( ~(U32)(0x3<<4))) | div<<4;
			STSYS_WriteRegDev32LE( HD_TVOUT_MAIN_GLUE_BASE_ADDRESS + HTO_MAIN_CLK_SEL , hdtvo_cfg );
			break;
        case CLKB_HDTVO_FDVO:
            hdtvo_cfg = (hdtvo_cfg & ( ~(U32)(0x3<<6))) | div<<6;
			STSYS_WriteRegDev32LE( HD_TVOUT_MAIN_GLUE_BASE_ADDRESS + HTO_MAIN_CLK_SEL , hdtvo_cfg );
			break;
        case CLKB_HDTVO_PIX_AUX:
            hdtvo_auxcfg = (hdtvo_auxcfg & ( ~(U32)(0x3))) | div;
            STSYS_WriteRegDev32LE( HD_TVOUT_AUX_GLUE_BASE_ADDRESS + HTO_AUX_CLK_SEL , hdtvo_auxcfg );
			break;
        case CLKB_HDTVO_DENC:
            hdtvo_auxcfg = (hdtvo_auxcfg & ( ~(U32)(0x3<<2))) | div<<2;
			STSYS_WriteRegDev32LE( HD_TVOUT_AUX_GLUE_BASE_ADDRESS + HTO_AUX_CLK_SEL , hdtvo_auxcfg );
            break;
        case CLKB_SDTVO_PIX_MAIN:
            sdtvo_cfg = (sdtvo_cfg & ( ~(U32)(0x3))) | div;
			STSYS_WriteRegDev32LE( SD_TVOUT_TOPGLUE_BASE_ADDRESS + STO_CLK_SEL , sdtvo_cfg );
			break;
        case CLKB_SDTVO_DENC:
            sdtvo_cfg = (sdtvo_cfg & ( ~(U32)(0x3<<2))) | div<<2;
			STSYS_WriteRegDev32LE( SD_TVOUT_TOPGLUE_BASE_ADDRESS + STO_CLK_SEL , sdtvo_cfg );
			break;
        case CLKB_SDTVO_FDVO:
            sdtvo_cfg = (sdtvo_cfg & ( ~(U32)(0x3<<4))) | div<<4;
			STSYS_WriteRegDev32LE( SD_TVOUT_TOPGLUE_BASE_ADDRESS + STO_CLK_SEL , sdtvo_cfg );
			break;
        default:
            break;
    }

}/* end of Ckg_SetClockDivider()*/

/* ========================================================================
   Name:        ST_Ckg_SetClockSource
   Description: Set the source clock of a clock

   ======================================================================== */
static BOOL Ckg_SetClockSource (clockName_t destclockid, clockName_t srcclockid)
{
    U32 ckg_cfg, data;
    U32 clockdestindex, clocksrcindex;

    if ((destclockid > NB_OF_CLOCKB) || (srcclockid > NB_OF_CLOCKB))
    {
/*        STTBX_Print(("ST_Ckg_SetClockSource: clockName_t unknown\n")) */
        return (TRUE);
    }

    clockdestindex = Ckg_FindClockInstanceIndex(destclockid);
    clocksrcindex = Ckg_FindClockInstanceIndex(srcclockid);

    if ((destclockid != CLKB_PIX_OBELIX) && (destclockid != CLKB_PIX_ASTERIX) && (destclockid != CLKB_OBELIX_GDP) &&
       (destclockid != CLKB_OBELIX_VDP) && (destclockid != CLKB_OBELIX_CAP) && (destclockid != CLKB_LOC_SDTVO) &&
       (destclockid != CLKB_REM_SDTVO) && (destclockid != CLKB_216))
    {
/*        STTBX_Print(("ST_Ckg_SetClockSource: %s has a fixed source\n", CkgB_Clocks[clockdestindex].name)) */
        return (FALSE);
    }

    if ((CkgB_Clocks[clockdestindex].sources[0] != srcclockid) && (CkgB_Clocks[clockdestindex].sources[1] != srcclockid))
    {
/*        STTBX_Print(("ST_Ckg_SetClockSource: %s can not have as source %s\n", CkgB_Clocks[clockdestindex].name, CkgB_Clocks[srcclockid].name)) */
        return (FALSE);
    }

    ckg_cfg = Ckg_GetClockConfig();

    /* change source mux */
    switch (destclockid)
    {
        case CLKB_PIX_OBELIX:
            if (srcclockid == CLKB_PIX_HD0)
            {
                ckg_cfg |= (0x1<<1);
            }
            else
            {
                ckg_cfg &= ~(U32)(0x1<<1);
            }
            break;
        case CLKB_PIX_ASTERIX:
            if (srcclockid == CLKB_PIX_HD0)
            {
                ckg_cfg |= 0x1;
            }
            else
            {
                ckg_cfg &= ~(U32)0x1;
            }
            break;
      case CLKB_OBELIX_GDP:
            if (srcclockid == CLKB_OBELIX_AUX)
            {
                ckg_cfg |= (0x1<<2);
            }
            else
            {
                ckg_cfg &= ~(U32)(0x1<<2);
            }
            break;
      case CLKB_OBELIX_VDP:
            if (srcclockid == CLKB_OBELIX_AUX)
            {
                ckg_cfg |= (0x1<<3);
            }
            else
            {
                ckg_cfg &= ~(U32)(0x1<<3);
            }
            break;
      case CLKB_OBELIX_CAP:
            if (srcclockid == CLKB_OBELIX_AUX)
            {
                ckg_cfg |= (0x1<<4);
            }
            else
            {
                ckg_cfg &= ~(U32)(0x1<<4);
            }
            break;
        case CLKB_LOC_SDTVO:
            if (srcclockid == CLKB_PIX_HD0)
            {
                ckg_cfg |= (0x1<<5);
            }
            else
            {
                ckg_cfg &= ~(U32)(0x1<<5);
            }
            break;
        case CLKB_REM_SDTVO:
            if (srcclockid == CLKB_PIX_HD0)
            {
                ckg_cfg |= (0x1<<6);
            }
            else
            {
                ckg_cfg &= ~(U32)(0x1<<6);
            }
            break;
        case CLKB_216:
            data = STSYS_ReadRegDev32LE( CKG_B_BASE_ADDRESS + CKGB_OUT_MUX_CFG );
            if (srcclockid == CLKB_FS1_CH1)
            {
                data &= ~(U32)(1<<13);
                STSYS_WriteRegDev32LE( CKG_B_BASE_ADDRESS + CKGB_OUT_MUX_CFG, data );
            }
            if (srcclockid == CLKB_FS1_CH2)
            {
                data |= (1<<13);
                STSYS_WriteRegDev32LE( CKG_B_BASE_ADDRESS + CKGB_OUT_MUX_CFG, data );
            }
            break;
        default :
            break;
    }

    Ckg_UpdateClkCfg(ckg_cfg) ;

    return (FALSE);
}

#ifndef STVTG_USE_CLKRV
/* ========================================================================
   Name:        Ckg_GetFSParameterFromFreq
   Description: Freq to Parameters

   ======================================================================== */
static boolean Ckg_GetFSParameterFromFreq( int inputfreq, int outputfreq, int *md, int *pe, int *sdiv )
{
    double fr, Tr, Td1, Tx, fmx, nd1, nd2, Tdif;
    int NTAP, msdiv, mfmx, ndiv;

    NTAP = 32;
    mfmx = 0;

    ndiv = 1.0;

    fr = inputfreq * 8000000.0;
    Tr = 1.0 / fr;
    Td1 = 1.0 / (NTAP * fr);
    msdiv = 0;

    /* Recherche de SDIV */
    while (! ((mfmx >= (inputfreq*8000000)) && (mfmx <= (inputfreq*16000000))) && (msdiv < 7))
    {
        msdiv = msdiv + 1;
        mfmx = pow(2,msdiv) * outputfreq;
    }

    *sdiv = msdiv - 1;
    fmx = mfmx / 1000000.0;
    if ((fmx < (8*inputfreq)) || (fmx > (16*inputfreq)))
    {
        return (TRUE);
    }

    Tx = 1 / (fmx * 1000000.0);

    Tdif = Tr - Tx;

    /* Recherche de MD */
    nd1 = floor((32.0 * (mfmx - fr) / mfmx));
    nd2 = nd1 + 1.0;

    *md = 32.0 - nd2;

    /* Recherche de PE */
    *pe = ceil((32.0 * (mfmx - fr) / mfmx - nd1) * 32768.0);

/*    STTBX_Print(( "Programmation for QSYNTH C090_4FS216_25 with fout= %d Hz and fin= 30 MHz\n", freq));
    STTBX_Print(( "      MD[4:0]               ->       md      = 0x%x\n", *md));
    STTBX_Print(( "      PE[15:0]              ->       pe      = 0x%x\n", *pe));
    STTBX_Print(( "      SDIV[2:0]             ->       sdiv    = 0x%x\n", *sdiv));*/

    return (FALSE);
}


/* ========================================================================
   Name:        Ckg_SetFSCHClock
   Description: Set FS clock

   ======================================================================== */
static BOOL Ckg_SetFSCHClock(FS_sel_t FS_sel, int freq)
{
    U32 data;
    boolean RetError = FALSE;
    int md, pe, sdiv;

    switch (FS_sel)
    {
        case FS0_out1 :
        case FS0_out2 :
        case FS0_out3 :
        case FS0_out4 :
        case FS1_out1 :
        case FS1_out2 :
        case FS1_out3 :
        case FS1_out4 :
        case FS2_out1 :
        case FS2_out2 :
        case FS2_out3 :
        case FS2_out4 :
            if ( (Ckg_GetFSParameterFromFreq( refclockB, freq, &md, &pe, &sdiv )) == TRUE )
            {
                return TRUE;
            }
            data = (pe&0xffff) | ((md&0x1f)<<16) | ((sdiv&0x7)<<22) | (0x1<<21) | (0x1<<25);
            STSYS_WriteRegDev32LE( CKG_B_BASE_ADDRESS + 0x8 + (4*FS_sel), data);
            STSYS_WriteRegDev32LE( CKG_B_BASE_ADDRESS + CKGB_FS0_SETUP, 0x4);
            STSYS_WriteRegDev32LE( CKG_B_BASE_ADDRESS + CKGB_FS1_SETUP, 0x4);
            STSYS_WriteRegDev32LE( CKG_B_BASE_ADDRESS + CKGB_FS2_SETUP, 0x4);
            break;
        case FS3_out1 :
        case FS3_out2 :
        case FS3_out3 :
        case FS3_out4 :
        case FS4_out1 :
        case FS4_out2 :
        case FS4_out3 :
        case FS4_out4 :
            if ( (Ckg_GetFSParameterFromFreq( refclockC, freq, &md, &pe, &sdiv )) == TRUE )
            {
                return TRUE;
            }
			STSYS_WriteRegDev32LE( AUDIO_IF_BASE_ADDRESS + 0x1c + (4*(FS_sel-FS3_out1)), 0);
            STSYS_WriteRegDev32LE( AUDIO_IF_BASE_ADDRESS + 0x10 + (4*(FS_sel-FS3_out1)), md);
            STSYS_WriteRegDev32LE( AUDIO_IF_BASE_ADDRESS + 0x14 + (4*(FS_sel-FS3_out1)), pe);
            STSYS_WriteRegDev32LE( AUDIO_IF_BASE_ADDRESS + 0x18 + (4*(FS_sel-FS3_out1)), sdiv);
            STSYS_WriteRegDev32LE( AUDIO_IF_BASE_ADDRESS + 0x1c + (4*(FS_sel-FS3_out1)), 1);
            STSYS_WriteRegDev32LE( AUDIO_IF_BASE_ADDRESS + AUDCFG_FSYNA_CFG, 0);
            STSYS_WriteRegDev32LE( AUDIO_IF_BASE_ADDRESS + AUDCFG_FSYNB_CFG, 0);
			break;
        default :
            RetError = TRUE;
            break;
    }
    return (RetError);
}
#endif

/*****************************************************************************
Name        : Ckg_SetMode
Description : Set Clock configuration Mode
Parameters  :
Assumptions :
Limitations :
Returns     :
*****************************************************************************/
static BOOL Ckg_SetMode( Ckg_Mode_t Mode_HDTVOUT_Primary, Ckg_Mode_t Mode_HDTVOUT_Local, Ckg_Mode_t Mode_SDTVOUT_Remote )
{

    switch (Mode_HDTVOUT_Primary)
    {
        case CKGMODE_HD :
#ifndef STVTG_USE_CLKRV
            Ckg_SetFSCHClock(FS0_out1, 148500000);
            Ckg_SetFSCHClock(FS0_out2, 148500000);
#endif
            Ckg_SetClockDivider(CLKB_OBELIX_FVP_MAIN, CLKB_DIV2);
            Ckg_SetClockDivider(CLKB_TMDS, CLKB_DIV2);
            Ckg_SetClockDivider(CLKB_HDTVO_PIX_MAIN, CLKB_DIV2);
            Ckg_SetClockDivider(CLKB_HDTVO_TMDS, CLKB_DIV2);
            Ckg_SetClockDivider(CLKB_HDTVO_FDVO, CLKB_DIV2);
            Ckg_SetClockDivider(CLKB_HDTVO_BCH, CLKB_DIV1);
            break;

        case CKGMODE_ED :
#ifndef STVTG_USE_CLKRV
            Ckg_SetFSCHClock(FS0_out1, 108000000);
            Ckg_SetFSCHClock(FS0_out2, 108000000);
#endif
            Ckg_SetClockDivider(CLKB_OBELIX_FVP_MAIN, CLKB_DIV4);
            Ckg_SetClockDivider(CLKB_TMDS, CLKB_DIV4);
            Ckg_SetClockDivider(CLKB_HDTVO_PIX_MAIN, CLKB_DIV4);
            Ckg_SetClockDivider(CLKB_HDTVO_TMDS, CLKB_DIV4);
            Ckg_SetClockDivider(CLKB_HDTVO_FDVO, CLKB_DIV2);
            Ckg_SetClockDivider(CLKB_HDTVO_BCH, CLKB_DIV2);
            break;

        case CKGMODE_SD :
#ifndef STVTG_USE_CLKRV
            Ckg_SetFSCHClock(FS0_out1, 108000000);
            Ckg_SetFSCHClock(FS0_out2, 108000000);
#endif
            Ckg_SetClockDivider(CLKB_OBELIX_FVP_MAIN, CLKB_DIV8);
            Ckg_SetClockDivider(CLKB_TMDS, CLKB_DIV4);
            Ckg_SetClockDivider(CLKB_HDTVO_PIX_MAIN, CLKB_DIV8);
            Ckg_SetClockDivider(CLKB_HDTVO_TMDS, CLKB_DIV4);
            Ckg_SetClockDivider(CLKB_HDTVO_FDVO, CLKB_DIV2);
            Ckg_SetClockDivider(CLKB_HDTVO_BCH, CLKB_DIV2);
            break;

        case CKGMODE_NO_CONFIGURATION :
        default:
            break;
    }

    switch (Mode_HDTVOUT_Local)
    {
        case CKGMODE_HD :
        case CKGMODE_ED :
/*            STTBX_Print(( "Ckg_SetMode : mode HD and ED for Local SD not supported\n")); */
            break;

        case CKGMODE_SD :
#ifndef STVTG_USE_CLKRV
            Ckg_SetFSCHClock(FS1_out1, 216000000);
            Ckg_SetClockSource(CLKB_216, CLKB_FS1_CH1);
#else
            STSYS_WriteRegDev32LE(0xfd70101c, 0x2311c72);
#endif
            Ckg_SetClockSource(CLKB_PIX_OBELIX, CLKB_PIX_SD0_TSG_SD0_P27);
            Ckg_SetClockDivider(CLKB_OBELIX_AUX, CLKB_DIV2);
            Ckg_SetClockSource(CLKB_OBELIX_GDP, CLKB_OBELIX_AUX);
            Ckg_SetClockSource(CLKB_OBELIX_VDP, CLKB_OBELIX_AUX);
            Ckg_SetClockSource(CLKB_OBELIX_CAP, CLKB_OBELIX_AUX);

            Ckg_SetClockDivider(CLKB_CH34MOD, CLKB_DIV1);

            Ckg_SetClockSource(CLKB_LOC_SDTVO, CLKB_PIX_SD0_TSG_SD0_P27);
            Ckg_SetClockDivider(CLKB_HDTVO_PIX_AUX, CLKB_DIV2);
            Ckg_SetClockDivider(CLKB_HDTVO_DENC, CLKB_DIV1);
            break;

        case CKGMODE_NO_CONFIGURATION :
        default:
            break;
    }

    switch (Mode_SDTVOUT_Remote)
    {
        case CKGMODE_HD :
        case CKGMODE_ED :
/*            STTBX_Print(( "Ckg_SetMode : mode HD and ED for AUX2 not implemented\n")); */
            break;

        case CKGMODE_SD :
#ifndef STVTG_USE_CLKRV
            Ckg_SetFSCHClock(FS1_out2, 216000000);
#endif
            Ckg_SetClockSource(CLKB_PIX_ASTERIX, CLKB_PIX_SD1_TSG_SD1);
            Ckg_SetClockDivider(CLKB_ASTERIX_AUX, CLKB_DIV2);
            Ckg_SetClockSource(CLKB_REM_SDTVO, CLKB_PIX_SD1_TSG_SD1);
            Ckg_SetClockDivider(CLKB_SDTVO_PIX_MAIN, CLKB_DIV2);
            Ckg_SetClockDivider(CLKB_SDTVO_DENC, CLKB_DIV1);
            Ckg_SetClockDivider(CLKB_SDTVO_FDVO, CLKB_DIV1);
            break;

        case CKGMODE_NO_CONFIGURATION :
        default:
            break;
    }

    /* Save the new configuration */
    ModePrimary = Mode_HDTVOUT_Primary;
    ModeLocal = Mode_HDTVOUT_Local;
    ModeRemote = Mode_SDTVOUT_Remote;

    return (FALSE);
}

#endif /*ST_7200||7111||ST_7105*/

/*****************************************************************************
Name        : ConfigSynth
Description : Configures a synthesizer
Parameters  : Device_p (IN) : pointer on VTG device to access
 *            Synth           : Addressed synthesizer
              OutputClockFreq : Required output clock frequency
Assumptions :
Limitations :
Returns     : None
*****************************************************************************/
static void ConfigSynth(   const stvtg_Device_t * const Device_p
                         , const Clock_t Synth
                         , const U32 OutputClockFreq)
{

     U32 CKGBaseAddress;
    #if !defined (ST_7710) && !defined(ST_7100) && !defined (ST_7109) && !defined (ST_7200) && !defined (ST_7111)&& !defined (ST_7105)
        U32 par, ndiv, sdiv, md, pe;

    #else
      U32 data;
      #if !(defined (ST_7200)|| defined (ST_7111)|| defined (ST_7105))
        U32 Dspcfg_Val;
      #endif
      #if defined (ST_7710)
        U32 disphd;
      #endif
      #if defined (ST_7100)|| defined(ST_7109)
        U32 CutId;
      #endif
    #endif
#if !(defined(ST_5100)|| defined(ST_7710)|| defined(ST_5105) || defined(ST_7100)|| defined(ST_5301)|| defined(ST_7109)\
   || defined(ST_5188)|| defined(ST_5525)|| defined(ST_5107) || defined(ST_7200)|| defined(ST_5162)|| defined(ST_7111)\
   || defined(ST_7105))
    U8 CkgSynPar=0;
#endif
#if defined (BE_7015) || defined (BE_7020)
    U32 en, ssl;
    U8 CkgSynRef=0;
#endif
#if defined (ST_7100)||defined (ST_7109)
   U32  SYSCFGBaseAddress;
#endif
#if defined (ST_7200)|| defined (ST_7111)|| defined (ST_7105)
   U32 SDGlueBaseAddress;
   U32 HDGlueMainBaseAddress;
   U32 HDGlueAuxBaseAddress;
#endif

#if defined (ST_7100)|| defined (ST_7109)|| defined(ST_7710)|| defined(ST_7200)|| defined (ST_7111)|| defined (ST_7105)
    UNUSED_PARAMETER(Synth);
    UNUSED_PARAMETER(OutputClockFreq);
#endif

    CKGBaseAddress = (U32)Device_p->CKGBaseAddress_p;

#if defined (BE_7015) || defined (BE_7020)
    switch (Synth)
    {
        case SYNTH3:
            CkgSynPar = CKG_SYN3_PAR;
            CkgSynRef = CKG_SYN3_REF;
            break;
#ifdef WA_GNBvd06791
        case SYNTH6:
            CkgSynPar = CKG_SYN6_PAR;
            break;
#endif
        case SYNTH7:
            CkgSynPar = CKG_SYN7_PAR;
            CkgSynRef = CKG_SYN7_REF;
            break;
        default:
            /* checked cannot be possible */
            break;
    }

    /* Read current parameters */
    par = stvtg_Read32(CKGBaseAddress + CkgSynPar);
    pe   =  par & CKG_SYN_PAR_PE_MASK;
    md   = (par & CKG_SYN_PAR_MD_MASK)   >> 16;
    sdiv = (par & CKG_SYN_PAR_SDIV_MASK) >> 21;
    ndiv = (par & CKG_SYN_PAR_NDIV_MASK) >> 24;
    en   = (par & CKG_SYN_PAR_EN)        >> 27;
    ssl  = (par & CKG_SYN_PAR_SSL)       >> 26;

    /* Select synthesizer */
    ssl = 1;
    /* Enable synthesizer */
    en = 1;

    /* Reference clock is clock reference 1 (RCLK1) */
    if ( (Synth == SYNTH3) || (Synth == SYNTH7))
    {
        STSYS_WriteRegDev32LE(CKGBaseAddress + CkgSynRef, 0);
    }

    /* Get synthesizer parameters */
    GetSynthParam(OutputClockFreq, &ndiv, &sdiv, &md, &pe);

    /* Add synthesizer parameters to register */
    par = (en  << 27)
        | (ssl << 26)
        | (ndiv << 24)
        | (sdiv << 21)
        | (md << 16)
        |  pe;

    /* Refresh parameters */
    STSYS_WriteRegDev32LE(CKGBaseAddress + CkgSynPar, par);
#elif defined(ST_5528) /* for STi5528 */
  /* unlock access to clock registers  */
  STSYS_WriteRegDev32LE(CKGBaseAddress + CKG_LOCK_REG, CKG_UNLOCK);

  switch (Synth)
    {
        case QSYNTH2_CLK1 : /* no break */
        case QSYNTH2_CLK2 : /* no break */
        case QSYNTH2_CLK3 :
            break;
        case QSYNTH2_CLK4 :
             CkgSynPar = CKG_SYN2_CLK4;
             /* pixel clock (2x) is connected to QSYNTH2_clk4*/
             STSYS_WriteRegDev8(CKGBaseAddress + PIX_CLK_CTRL, PIX_CLK_CTRL4);

        default :
            break;
    }
    /* Read current parameters For 5528*/
    par = stvtg_Read32(CKGBaseAddress + CKG_SYN2_CLK4);
    par &= ~CKG_CLK_STOP;

    pe   =  par & CKG_SYN2_PE_MASK;
    md   = (par & CKG_SYN2_MD_MASK)   >> 16;
    sdiv = (par & CKG_SYN2_SDIV_MASK) >> 21;
    ndiv = (par & CKG_SYN_RST_MASK)   >> 24;

    /* Get synthesizer parameters For 5528*/
    GetSynthParam(OutputClockFreq,&ndiv, &sdiv, &md, &pe);

    /* Add synthesizer parameters to register For 5528 */
    par = (ndiv << 24)
        | (sdiv << 21)
        | (md << 16)
        |  pe;

    /* Refresh parameters for qsynth2 on 5528*/
    STSYS_WriteRegDev32LE(CKGBaseAddress + CkgSynPar, par);
    /* lock access to clock registers  */
    STSYS_WriteRegDev32LE(CKGBaseAddress + CKG_LOCK_REG, CKG_LOCK);

#elif defined(ST_5100)|| defined(ST_5105)|| defined(ST_5301)|| defined(ST_5188)|| defined(ST_5525)|| defined(ST_5107)\
    ||defined(ST_5162)
if (Synth == SYNTH_CLK)                 /* for STi5100 /STi5105/STB5301 */
 {
    #if !(defined (ST_5525)||defined(ST_5188))
        /** Unlock clock generator register **/
      STSYS_WriteRegDev32LE(CKGBaseAddress+RGST_LK_CFG, RGST_LK_CFG_UNLK);
      STSYS_WriteRegDev32LE(CKGBaseAddress+RGST_LK_CFG, 0x0);
    #endif
    /** dco regsiter **/
    STSYS_WriteRegDev32LE(CKGBaseAddress+DCO_MODE_CFG, DCO_MODE_CFG_PRG);

    /* Get synthesizer parameters For 5100 - 5105 -5301*/
    ComputeFrequencySynthesizerParameters(DEFAULT_PIX_CLK, OutputClockFreq , &ndiv , &sdiv , &md , &pe);

    /* Add synthesizer parameters to register CLK_SETUP0 */
    par = (0xe00 |(sdiv)<<6 | md |(0x1<<5) |(0x1<<9) );
    STSYS_WriteRegDev32LE(CKGBaseAddress+CKG_PIX_CLK_SETUP0, par);

    /* Add synthesizer parameters to register CLK_SETUP1 */
    STSYS_WriteRegDev32LE(CKGBaseAddress+CKG_PIX_CLK_SETUP1, pe);
    #if !(defined (ST_5525)||defined(ST_5188))
        /* Lock clock generator register */
        STSYS_WriteRegDev32LE(CKGBaseAddress+RGST_LK_CFG, RGST_LK_CFG_UNLK);
    #endif
 }
#elif defined(ST_7710)
    switch (Device_p->TimingMode)
    {
        case STVTG_TIMING_MODE_480P59940_27000:
        case STVTG_TIMING_MODE_576P50000_27000:
            #ifdef STVTG_USE_CLKRV
               STCLKRV_SetApplicationMode( Device_p->ClkrvHandle, STCLKRV_APPLICATION_MODE_NORMAL );
               #ifdef WA_GNBvd50834
               /* For BCH  clock work around*/
               STCLKRV_SetNominalFreq(Device_p->ClkrvHandle,STCLKRV_CLOCK_HD_0, 54000000);
               #else /* WA_GNBvd50834*/
               STCLKRV_SetNominalFreq(Device_p->ClkrvHandle,STCLKRV_CLOCK_HD_0, 108000000);
               #endif /*WA_GNBvd50834*/
            #endif

            disphd = 0x0;
            data = STSYS_ReadRegDev32LE(FS_CLOCKGEN_CFG_2+CKGBaseAddress);
            data = data & 0xFF80;
            #ifdef WA_GNBvd50834
            /* PLL2 in bypass mode in the CFG2 register*/
            data = data | (disphd<<1) | 0x11;
            data = data | CFG_2_PLL2_BYPASS;
            #else
            data = data | (disphd<<1) | 0x1E;
            #endif

            data = data | 0x5AA50000;
            STSYS_WriteRegDev32LE(FS_CLOCKGEN_CFG_2+CKGBaseAddress,data);

            #ifdef WA_GNBvd50834
                /* For BCH clock WA */
                STSYS_WriteRegDev32LE(PLL2_CFG_0+CKGBaseAddress,0x5aa50701);
            #endif

            #ifndef STI7710_CUT2x
                if (Device_p->VtgId == STVTG_VTG_ID_1)
                {
                   Dspcfg_Val = STSYS_ReadRegDev32LE((U32)Device_p->BaseAddress_p+DSPCFG_CLK);
                   Dspcfg_Val = Dspcfg_Val | (1<<7) ;
                   STSYS_WriteRegDev32LE((U32)Device_p->BaseAddress_p+DSPCFG_CLK,Dspcfg_Val);
                }
            #endif
            break;
		case STVTG_TIMING_MODE_768P75000_78750:
			#ifdef STVTG_USE_CLKRV
			STCLKRV_SetApplicationMode( Device_p->ClkrvHandle, STCLKRV_APPLICATION_MODE_NORMAL );
			STCLKRV_SetNominalFreq(Device_p->ClkrvHandle,STCLKRV_CLOCK_HD_0, 157500000);
			#endif
			disphd = 0x0;
            data = STSYS_ReadRegDev32LE(FS_CLOCKGEN_CFG_2+CKGBaseAddress);
            data = data & 0xFF80;
            data = data | (disphd<<1) | 0x1D;
			data = data | 0x5AA50000;
            STSYS_WriteRegDev32LE(FS_CLOCKGEN_CFG_2+CKGBaseAddress,data);
			break;
        case STVTG_TIMING_MODE_768P60000_65000:
			 #ifdef STVTG_USE_CLKRV
			STCLKRV_SetApplicationMode( Device_p->ClkrvHandle, STCLKRV_APPLICATION_MODE_NORMAL );
			STCLKRV_SetNominalFreq(Device_p->ClkrvHandle,STCLKRV_CLOCK_HD_0, 130000000);
			#endif
			disphd = 0x0;
            data = STSYS_ReadRegDev32LE(FS_CLOCKGEN_CFG_2+CKGBaseAddress);
            data = data & 0xFF80;
            data = data | (disphd<<1) | 0x1D;
			data = data | 0x5AA50000;
            STSYS_WriteRegDev32LE(FS_CLOCKGEN_CFG_2+CKGBaseAddress,data);
			break;
		case STVTG_TIMING_MODE_768P85000_94500:
			#ifdef STVTG_USE_CLKRV
			STCLKRV_SetApplicationMode( Device_p->ClkrvHandle, STCLKRV_APPLICATION_MODE_NORMAL );
			STCLKRV_SetNominalFreq(Device_p->ClkrvHandle,STCLKRV_CLOCK_HD_0, 189000000);
			#endif
			disphd = 0x0;
            data = STSYS_ReadRegDev32LE(FS_CLOCKGEN_CFG_2+CKGBaseAddress);
            data = data & 0xFF80;
            data = data | (disphd<<1) | 0x1D;
			data = data | 0x5AA50000;
            STSYS_WriteRegDev32LE(FS_CLOCKGEN_CFG_2+CKGBaseAddress,data);
			break;
        case STVTG_TIMING_MODE_480P60000_27027:
            #ifdef STVTG_USE_CLKRV
                STCLKRV_SetApplicationMode( Device_p->ClkrvHandle, STCLKRV_APPLICATION_MODE_NORMAL );
                #ifdef WA_GNBvd50834
                STCLKRV_SetNominalFreq(Device_p->ClkrvHandle,STCLKRV_CLOCK_HD_0, 54054000);
                #else
                STCLKRV_SetNominalFreq(Device_p->ClkrvHandle,STCLKRV_CLOCK_HD_0, 108108000);
                #endif
            #endif
            disphd = 0x0;
            data = STSYS_ReadRegDev32LE(FS_CLOCKGEN_CFG_2+CKGBaseAddress);
            data = data & 0xFF80;

            #ifdef WA_GNBvd50834
            data = data | (disphd<<1) | 0x11;
            /* PLL2 in bypass mode in the CFG2 register*/
            data = data | CFG_2_PLL2_BYPASS;
            #else /* WA_GNBvd50834  */
            data = data | (disphd<<1) | 0x1E;
            #endif /* WA_GNBvd50834 */

            data = data | 0x5AA50000;
            STSYS_WriteRegDev32LE(FS_CLOCKGEN_CFG_2+CKGBaseAddress,data);

            #ifdef WA_GNBvd50834
                /* For BCH clock WA */
                STSYS_WriteRegDev32LE(PLL2_CFG_0+CKGBaseAddress,0x5aa50701);
            #endif /*WA_GNBvd50834 */

            #ifndef STI7710_CUT2x
                if (Device_p->VtgId == STVTG_VTG_ID_1)
                {
                   Dspcfg_Val = STSYS_ReadRegDev32LE((U32)Device_p->BaseAddress_p+DSPCFG_CLK);
                   Dspcfg_Val = Dspcfg_Val | (1<<7) ;
                   STSYS_WriteRegDev32LE((U32)Device_p->BaseAddress_p+DSPCFG_CLK,Dspcfg_Val);
                }
            #endif
            break;

        case STVTG_TIMING_MODE_480P60000_25200:
            #ifdef STVTG_USE_CLKRV
                /* 54054000 instead of 108108000 */
                #ifdef WA_GNBvd50834
                /* For BCH clock work around*/
                STCLKRV_SetNominalFreq(Device_p->ClkrvHandle,STCLKRV_CLOCK_HD_0, 50400000);
                #else /* WA_GNBvd50834 */
                STCLKRV_SetNominalFreq(Device_p->ClkrvHandle,STCLKRV_CLOCK_HD_0, 100800000);
                #endif
            #endif
            disphd = 0x0;
            data = STSYS_ReadRegDev32LE(FS_CLOCKGEN_CFG_2+CKGBaseAddress);
            data = data & 0xFF80;

            #ifdef WA_GNBvd50834
            data = data | (disphd<<1) | 0x11;
            /* PLL2 in bypass mode in the CFG2 register*/
            data = data | CFG_2_PLL2_BYPASS;
            #else
            data = data | (disphd<<1) | 0x1E;
            #endif
            data = data | 0x5AA50000;
            STSYS_WriteRegDev32LE(FS_CLOCKGEN_CFG_2+CKGBaseAddress,data);

            #ifdef WA_GNBvd50834
                /* For BCH clock WA */
                STSYS_WriteRegDev32LE(PLL2_CFG_0+CKGBaseAddress,0x5aa50701);
            #endif
            #ifndef STI7710_CUT2x
                if (Device_p->VtgId == STVTG_VTG_ID_1)
                {
                   Dspcfg_Val = STSYS_ReadRegDev32LE((U32)Device_p->BaseAddress_p+DSPCFG_CLK);
                   Dspcfg_Val = Dspcfg_Val | (1<<7) ;
                   STSYS_WriteRegDev32LE((U32)Device_p->BaseAddress_p+DSPCFG_CLK,Dspcfg_Val);
                }
            #endif
            break;
        case STVTG_TIMING_MODE_480P59940_25175:
            #ifdef STVTG_USE_CLKRV
                #ifdef WA_GNBvd50834
                /* 54054000 instead of 108108000 */
                STCLKRV_SetNominalFreq(Device_p->ClkrvHandle,STCLKRV_CLOCK_HD_0, 50350000);
                #else /* WA_GNBvd50834 */
                STCLKRV_SetNominalFreq(Device_p->ClkrvHandle,STCLKRV_CLOCK_HD_0, 100700000);
                #endif /* WA_GNBvd50834 */
            #endif
            disphd = 0x0;
            data = STSYS_ReadRegDev32LE(FS_CLOCKGEN_CFG_2+CKGBaseAddress);
            data = data & 0xFF80;

            #ifdef WA_GNBvd50834
            data = data | (disphd<<1) | 0x11;
            /* PLL2 in bypass mode in the CFG2 register*/
            data = data | CFG_2_PLL2_BYPASS;
            #else
            data = data | (disphd<<1) |0x1E;
            #endif
            data = data | 0x5AA50000;
            STSYS_WriteRegDev32LE(FS_CLOCKGEN_CFG_2+CKGBaseAddress,data);

            #ifdef WA_GNBvd50834
                /* For BCH clock WA */
                STSYS_WriteRegDev32LE(PLL2_CFG_0+CKGBaseAddress,0x5aa50701);
            #endif

            #ifndef STI7710_CUT2x
                if (Device_p->VtgId == STVTG_VTG_ID_1)
                {
                   Dspcfg_Val = STSYS_ReadRegDev32LE((U32)Device_p->BaseAddress_p+DSPCFG_CLK);
                   Dspcfg_Val = Dspcfg_Val | (1<<7) ;
                   STSYS_WriteRegDev32LE((U32)Device_p->BaseAddress_p+DSPCFG_CLK,Dspcfg_Val);
                }
            #endif
            break;
        case STVTG_TIMING_MODE_576I50000_13500 :
        case STVTG_TIMING_MODE_480I59940_13500 :
            if (Device_p->VtgId == STVTG_VTG_ID_1)
            {
                #ifdef STVTG_USE_CLKRV
                STCLKRV_SetApplicationMode( Device_p->ClkrvHandle, STCLKRV_APPLICATION_MODE_SD_ONLY );
                #ifdef WA_GNBvd50834
                    STCLKRV_SetNominalFreq(Device_p->ClkrvHandle,STCLKRV_CLOCK_HD_0, 54000000);
                #else /* WA_GNBvd50834 */
                    STCLKRV_SetNominalFreq(Device_p->ClkrvHandle,STCLKRV_CLOCK_HD_0, 108000000);
                #endif /* WA_GNBvd50834 */
                #endif

                 data = stvtg_Read32(FS_CLOCKGEN_CFG_2+CKGBaseAddress);
                 data = data & 0xFF80;
                 data = data | PIXCLK_CLKSEL_CLK_656;/*set CLK_656 for DVO*/
                 data = data | 0x12;
                 data = data | 0x5AA50000;

                #ifdef WA_GNBvd50834
                    /* PLL2 in bypass mode in the CFG2 register*/
                    data = data | CFG_2_PLL2_BYPASS;
                #else /* WA_GNBvd50834 */
                    data = data | 0x29;
                #endif /* WA_GNBvd50834 */

                STSYS_WriteRegDev32LE(FS_CLOCKGEN_CFG_2+CKGBaseAddress,data);
                #ifdef WA_GNBvd50834
                    /* For BCH clock WA */
                    STSYS_WriteRegDev32LE(PLL2_CFG_0+CKGBaseAddress,0x5aa50701);
                #else /* WA_GNBvd50834 */
                    STSYS_WriteRegDev32LE(PLL2_CFG_0+CKGBaseAddress,0x5aa50301);
                    STSYS_WriteRegDev32LE(PLL2_CFG_1+CKGBaseAddress,0x5aa50000);
                    STOS_TaskDelay( ST_GetClocksPerSecond()/100);
                    STSYS_WriteRegDev32LE(PLL2_CFG_1+CKGBaseAddress,0x5aa50002);
                #endif /* WA_GNBvd50834 */

            }
            #ifndef STI7710_CUT2x
                if (Device_p->VtgId == STVTG_VTG_ID_1)
                {
                    Dspcfg_Val = stvtg_Read32((U32)Device_p->BaseAddress_p+DSPCFG_CLK);
                    Dspcfg_Val = Dspcfg_Val & 0x7F ;
                    STSYS_WriteRegDev32LE((U32)Device_p->BaseAddress_p+DSPCFG_CLK,Dspcfg_Val);
                }
            #endif
            break;
        case STVTG_TIMING_MODE_480I60000_13514 :
            if (Device_p->VtgId == STVTG_VTG_ID_1)
            {
                #ifdef STVTG_USE_CLKRV
                    STCLKRV_SetApplicationMode( Device_p->ClkrvHandle, STCLKRV_APPLICATION_MODE_SD_ONLY );
                #ifdef WA_GNBvd50834
                   STCLKRV_SetNominalFreq(Device_p->ClkrvHandle,STCLKRV_CLOCK_HD_0, 54054000);
                #else /* WA_GNBvd50834 */
                   STCLKRV_SetNominalFreq(Device_p->ClkrvHandle,STCLKRV_CLOCK_HD_0, 108108000);
                #endif /* WA_GNBvd50834 */
                #endif

                data = stvtg_Read32(FS_CLOCKGEN_CFG_2+CKGBaseAddress);
                data = data & 0xFF80;
                data = data | PIXCLK_CLKSEL_CLK_656;/*set CLK_656 for DVO*/
                data = data | 0x12;
                data = data | 0x5AA50000;

                #ifdef WA_GNBvd50834
                /* PLL2 in bypass mode in the CFG2 register*/
                data = data | CFG_2_PLL2_BYPASS;
                #else /* WA_GNBvd50834 */
                data = data | 0x29;
                #endif /* WA_GNBvd50834 */

                STSYS_WriteRegDev32LE(FS_CLOCKGEN_CFG_2+CKGBaseAddress,data);

                #ifdef WA_GNBvd50834
                      /* For BCH clock WA */
                      STSYS_WriteRegDev32LE(PLL2_CFG_0+CKGBaseAddress,0x5aa50701);
                #else /* WA_GNBvd50834  */
                      STSYS_WriteRegDev32LE(PLL2_CFG_0+CKGBaseAddress,0x5aa50301);
                      STSYS_WriteRegDev32LE(PLL2_CFG_1+CKGBaseAddress,0x5aa50000);
                      STOS_TaskDelay( ST_GetClocksPerSecond()/100);
                      STSYS_WriteRegDev32LE(PLL2_CFG_1+CKGBaseAddress,0x5aa50002);
                #endif /* WA_GNBvd50834  */
            }
            #ifndef STI7710_CUT2x
                if (Device_p->VtgId == STVTG_VTG_ID_1)
                {
                    Dspcfg_Val = stvtg_Read32((U32)Device_p->BaseAddress_p+DSPCFG_CLK);
                    Dspcfg_Val = Dspcfg_Val & 0x7F ;
                    STSYS_WriteRegDev32LE((U32)Device_p->BaseAddress_p+DSPCFG_CLK,Dspcfg_Val);
                }
            #endif
            break;

        case STVTG_TIMING_MODE_576I50000_14750 :
            if (Device_p->VtgId == STVTG_VTG_ID_1)
            {
               STCLKRV_SetNominalFreq(Device_p->ClkrvHandle,STCLKRV_CLOCK_HD_0, 59000000);
               data = stvtg_Read32(FS_CLOCKGEN_CFG_2+CKGBaseAddress);
               data = data & 0xFFC0;
               data = data |0x22;
               data = data | 0x5AA50000;
               STSYS_WriteRegDev32LE(FS_CLOCKGEN_CFG_2+CKGBaseAddress,data);
               #ifndef STI7710_CUT2x
                  Dspcfg_Val = stvtg_Read32((U32)Device_p->BaseAddress_p+DSPCFG_CLK);
                  Dspcfg_Val = Dspcfg_Val & 0x7F ;
                  STSYS_WriteRegDev32LE((U32)Device_p->BaseAddress_p+DSPCFG_CLK,Dspcfg_Val);
               #endif
            }
            break;

        case STVTG_TIMING_MODE_480I60000_12285 :
        case STVTG_TIMING_MODE_480I59940_12273 :
            if (Device_p->VtgId == STVTG_VTG_ID_1)
            {
               STCLKRV_SetNominalFreq(Device_p->ClkrvHandle,STCLKRV_CLOCK_HD_0, 49140000);
               data = stvtg_Read32(FS_CLOCKGEN_CFG_2+CKGBaseAddress);
               data = data & 0xFFC0;
               data = data |0x22;
               data = data | 0x5AA50000;
               STSYS_WriteRegDev32LE(FS_CLOCKGEN_CFG_2+CKGBaseAddress,data);
               #ifndef STI7710_CUT2x
                  Dspcfg_Val = stvtg_Read32((U32)Device_p->BaseAddress_p+DSPCFG_CLK);
                  Dspcfg_Val = Dspcfg_Val & 0x7F ;
                  STSYS_WriteRegDev32LE((U32)Device_p->BaseAddress_p+DSPCFG_CLK,Dspcfg_Val);
               #endif
            }
            break;
        case STVTG_TIMING_MODE_720P59940_74176  :
        case STVTG_TIMING_MODE_1080I59940_74176 :
            #ifdef STVTG_USE_CLKRV
                STCLKRV_SetApplicationMode( Device_p->ClkrvHandle, STCLKRV_APPLICATION_MODE_NORMAL );
                STCLKRV_SetNominalFreq(Device_p->ClkrvHandle,STCLKRV_CLOCK_HD_0, 148351648);
            #endif
            disphd = 0x0;
            data = stvtg_Read32(FS_CLOCKGEN_CFG_2+CKGBaseAddress);
            data = data & 0xFF80;
            data = data | (disphd<<1) | 0x11;
            data = data | 0x5AA50000;
            data &= ~CFG_2_PLL2_BYPASS;
            STSYS_WriteRegDev32LE(FS_CLOCKGEN_CFG_2+CKGBaseAddress,data);

            #ifdef WA_GNBvd50834
                  /* For BCH clock WA */
                  STSYS_WriteRegDev32LE(PLL2_CFG_0+CKGBaseAddress,0x5aa50301);
            #endif

            #ifndef STI7710_CUT2x
                if (Device_p->VtgId == STVTG_VTG_ID_1)
                {
                   Dspcfg_Val = stvtg_Read32((U32)Device_p->BaseAddress_p+DSPCFG_CLK);
                   Dspcfg_Val = Dspcfg_Val & 0x7F ;
                   STSYS_WriteRegDev32LE((U32)Device_p->BaseAddress_p+DSPCFG_CLK,Dspcfg_Val);
                }
            #endif
            break;
        case STVTG_TIMING_MODE_SLAVE :   /*  VTG2 on slave mode : H_AND_V & V_ONLY*/
            disphd = 0x1;
            data = stvtg_Read32(FS_CLOCKGEN_CFG_2+CKGBaseAddress);
            data = data & 0xFF80;
            data = data | (disphd<<1) | 0x13;
            data = data | 0x5AA50000;
            STSYS_WriteRegDev32LE(FS_CLOCKGEN_CFG_2+CKGBaseAddress,data);
            break;

        case STVTG_TIMING_MODE_1080I60000_74250:
        case STVTG_TIMING_MODE_720P60000_74250:
        case STVTG_TIMING_MODE_720P50000_74250:
        case STVTG_TIMING_MODE_1080I50000_74250:
        case STVTG_TIMING_MODE_1080I50000_74250_1:
        case STVTG_TIMING_MODE_1080P30000_74250:
		case STVTG_TIMING_MODE_1080P25000_74250:
        case STVTG_TIMING_MODE_1080P24000_74250:
            #ifdef STVTG_USE_CLKRV
                 STCLKRV_SetApplicationMode( Device_p->ClkrvHandle, STCLKRV_APPLICATION_MODE_NORMAL );
                 STCLKRV_SetNominalFreq(Device_p->ClkrvHandle,STCLKRV_CLOCK_HD_0, 148500000);
            #endif
            disphd = 0x0;
            data = stvtg_Read32(FS_CLOCKGEN_CFG_2+CKGBaseAddress);
            data = data & 0xFF80;
            data = data | (disphd<<1) | 0x11;
            data = data | 0x5AA50000;
            data &= ~CFG_2_PLL2_BYPASS;
            STSYS_WriteRegDev32LE(FS_CLOCKGEN_CFG_2+CKGBaseAddress,data);

            #ifdef WA_GNBvd50834
                 /* For BCH clock WA */
                 STSYS_WriteRegDev32LE(PLL2_CFG_0+CKGBaseAddress,0x5aa50301);
            #endif

            #ifndef STI7710_CUT2x
                if (Device_p->VtgId == STVTG_VTG_ID_1)
                {
                   Dspcfg_Val = stvtg_Read32((U32)Device_p->BaseAddress_p+DSPCFG_CLK);
                   Dspcfg_Val = Dspcfg_Val & 0x7F ;
                   STSYS_WriteRegDev32LE((U32)Device_p->BaseAddress_p+DSPCFG_CLK,Dspcfg_Val);
                }
            #endif
            break;

        case STVTG_TIMING_MODE_1080P29970_74176:
		case STVTG_TIMING_MODE_1080P23976_74176:
            #ifdef STVTG_USE_CLKRV
                 STCLKRV_SetApplicationMode( Device_p->ClkrvHandle, STCLKRV_APPLICATION_MODE_NORMAL );
                 STCLKRV_SetNominalFreq(Device_p->ClkrvHandle,STCLKRV_CLOCK_HD_0, 148352000);
            #endif
            disphd = 0x0;
            data = stvtg_Read32(FS_CLOCKGEN_CFG_2+CKGBaseAddress);
            data = data & 0xFF80;
            data = data | (disphd<<1) | 0x11;
            data = data | 0x5AA50000;
            data &= ~CFG_2_PLL2_BYPASS;
            STSYS_WriteRegDev32LE(FS_CLOCKGEN_CFG_2+CKGBaseAddress,data);

            #ifdef WA_GNBvd50834
                 /* For BCH clock WA */
                 STSYS_WriteRegDev32LE(PLL2_CFG_0+CKGBaseAddress,0x5aa50301);
            #endif

            #ifndef STI7710_CUT2x
                if (Device_p->VtgId == STVTG_VTG_ID_1)
                {
                   Dspcfg_Val = stvtg_Read32((U32)Device_p->BaseAddress_p+DSPCFG_CLK);
                   Dspcfg_Val = Dspcfg_Val & 0x7F ;
                   STSYS_WriteRegDev32LE((U32)Device_p->BaseAddress_p+DSPCFG_CLK,Dspcfg_Val);
                }
            #endif
            break;

        case STVTG_TIMING_MODE_1152I50000_48000:
            #ifdef STVTG_USE_CLKRV
                STCLKRV_SetApplicationMode( Device_p->ClkrvHandle, STCLKRV_APPLICATION_MODE_NORMAL );
                STCLKRV_SetNominalFreq(Device_p->ClkrvHandle,STCLKRV_CLOCK_HD_0, 96000000);
            #endif
            disphd = 0x0;
            data = stvtg_Read32(FS_CLOCKGEN_CFG_2+CKGBaseAddress);
            data = data & 0xFF80;
            data = data | (disphd<<1) | 0x11;
            data = data | 0x5AA50000;
            data &= ~CFG_2_PLL2_BYPASS;
            STSYS_WriteRegDev32LE(FS_CLOCKGEN_CFG_2+CKGBaseAddress,data);
            #ifdef WA_GNBvd50834
                /* For BCH clock WA */
                STSYS_WriteRegDev32LE(PLL2_CFG_0+CKGBaseAddress,0x5aa50301);
            #endif
            #ifndef STI7710_CUT2x
                if (Device_p->VtgId == STVTG_VTG_ID_1)
                {
                    Dspcfg_Val = stvtg_Read32((U32)Device_p->BaseAddress_p+DSPCFG_CLK);
                    Dspcfg_Val = Dspcfg_Val & 0x7F ;
                    STSYS_WriteRegDev32LE((U32)Device_p->BaseAddress_p+DSPCFG_CLK,Dspcfg_Val);
                }
            #endif
            break;

        case STVTG_TIMING_MODE_1080I50000_72000:
            #ifdef STVTG_USE_CLKRV
                STCLKRV_SetApplicationMode( Device_p->ClkrvHandle, STCLKRV_APPLICATION_MODE_NORMAL );
                STCLKRV_SetNominalFreq(Device_p->ClkrvHandle,STCLKRV_CLOCK_HD_0, 144000000);
            #endif
            disphd = 0x0;
            data = stvtg_Read32(FS_CLOCKGEN_CFG_2+CKGBaseAddress);
            data = data & 0xFF80;
            data = data | (disphd<<1) | 0x11;
            data = data | 0x5AA50000;
            data &= ~CFG_2_PLL2_BYPASS;
            STSYS_WriteRegDev32LE(FS_CLOCKGEN_CFG_2+CKGBaseAddress,data);

            #ifdef WA_GNBvd50834
                  /* For BCH clock WA */
                  STSYS_WriteRegDev32LE(PLL2_CFG_0+CKGBaseAddress,0x5aa50301);
            #endif

            #ifndef STI7710_CUT2x
                if (Device_p->VtgId == STVTG_VTG_ID_1)
                {
                    Dspcfg_Val = stvtg_Read32((U32)Device_p->BaseAddress_p+DSPCFG_CLK);
                    Dspcfg_Val = Dspcfg_Val & 0x7F ;
                    STSYS_WriteRegDev32LE((U32)Device_p->BaseAddress_p+DSPCFG_CLK,Dspcfg_Val);
                }
            #endif
            break;
        default:
            break;
    }
    #ifdef WA_GNBvd50834 /* PLL & FS configuration needed for HDMI output */
        STSYS_WriteRegDev32LE(PLL2_CFG_1+CKGBaseAddress,0x5aa50000);
        STOS_TaskDelay( ST_GetClocksPerSecond()/1000);
        STSYS_WriteRegDev32LE(PLL2_CFG_1+CKGBaseAddress,0x5aa50002);
        STSYS_WriteRegDev32LE(PLL2_CFG_2+CKGBaseAddress,0x5aa50000);
        STOS_TaskDelay( ST_GetClocksPerSecond()/1000);
        STSYS_WriteRegDev32LE(PLL2_CFG_2+CKGBaseAddress,0x5aa50004);
        STSYS_WriteRegDev32LE(RESET_CTRL_1+CKGBaseAddress,0x5aa50001);
        STOS_TaskDelay( ST_GetClocksPerSecond()/1000);
        STSYS_WriteRegDev32LE(RESET_CTRL_1+CKGBaseAddress,0x5aa50000);
    #endif
    #ifdef WA_GNBvd35956
        /* Change also the SD pixel clock to have 148,5 MHz. */
        Change_Freq_Disp_CLK(CKGBaseAddress,0x0,0x17,0x5d17);
    #endif /* WA_GNBvd35956 */

#elif defined(ST_7100)|| defined (ST_7109)
    /*UnLock clock registers*/
    STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_LOCK,0xc0de);

    SYSCFGBaseAddress = (U32)Device_p->SYSCFGBaseAddress_p;


    CutId = ST_GetCutRevision();

    switch (Device_p->TimingMode)
    {
        case STVTG_TIMING_MODE_480P59940_27000:/*no break*/
        case STVTG_TIMING_MODE_576P50000_27000:
            #ifdef STVTG_USE_CLKRV
               #ifdef ST_7109
                  if ( CutId >= 0xB0 )
                     STCLKRV_SetNominalFreq(Device_p->ClkrvHandle,STCLKRV_CLOCK_HD_0, 108000000);
                  else
                     STCLKRV_SetNominalFreq(Device_p->ClkrvHandle,STCLKRV_CLOCK_HD_0, 54000000);
               #else
                  STCLKRV_SetNominalFreq(Device_p->ClkrvHandle,STCLKRV_CLOCK_HD_0, 54000000);
               #endif
            #endif
            #ifdef ST_7100
                if ( CutId < 0xB0 )
                {
                      STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_REF_MAX,0x0);
                      STSYS_WriteRegDev32LE(CKGBaseAddress+ CKGB_CMD ,0x1f);
                      STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_FS1_MD2 ,0x1f);
                      STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_FS1_PE2 ,0x0);
                      STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_FS1_EN_PRG2 ,0x1);
                      STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_DISPLAY_CFG,0x600);
                }
            #endif
            #ifdef ST_7109
                if (CutId >= 0xB0)
                {
                  STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_DISPLAY_CFG,0x415);
                  STSYS_WriteRegDev32LE(SYSCFGBaseAddress+SYS_CFG34,0x1);
                }
                else
                {
                  STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_DISPLAY_CFG,0x0);
                }
            #else
                STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_DISPLAY_CFG,0x0);
            #endif

            /*** Add Pix sd is routed from FS1 */
            {
                U32 Value=stvtg_Read32(CKGBaseAddress+CKGB_CLK_SRC);
                Value=Value|0x2;
                STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_CLK_SRC,Value);
            }
            if (Device_p->VtgId == STVTG_VTG_ID_1)
            {
                Dspcfg_Val = stvtg_Read32((U32)Device_p->BaseAddress_p+DSPCFG_CLK);
                Dspcfg_Val = Dspcfg_Val | (1<<10) ; /* 4* upsumpling*/
                STSYS_WriteRegDev32LE((U32)Device_p->BaseAddress_p+DSPCFG_CLK,Dspcfg_Val);

                STOS_InterruptLock();
                data = stvtg_Read32 (SYSCFGBaseAddress+SYS_CFG3);
                data &= 0x7ff;
                #ifdef ST_7109
                   if (CutId >= 0xB0)
                      data |= SERIALIZER_HDMI_RST|PLL_S_HDMI_ENABLE|PLL_S_HDMI_PDIV|PLL_S_HDMI_NDIV|PLL_S_HDMI_MDIV_HD;
                   else
                      data |= SERIALIZER_HDMI_RST|PLL_S_HDMI_ENABLE|PLL_S_HDMI_PDIV|PLL_S_HDMI_NDIV|PLL_S_HDMI_MDIV;
                #else
                   data |= SERIALIZER_HDMI_RST|PLL_S_HDMI_ENABLE|PLL_S_HDMI_PDIV|PLL_S_HDMI_NDIV|PLL_S_HDMI_MDIV;
                #endif
                /* PLL HDMI serializer configuration*/
                STSYS_WriteRegDev32LE(SYSCFGBaseAddress+SYS_CFG3,data);
                STOS_InterruptUnlock();
            }
            break;

        case STVTG_TIMING_MODE_480P60000_27027:
            #ifdef STVTG_USE_CLKRV
             #ifdef ST_7109
                  if ( CutId >= 0xB0 )
                     STCLKRV_SetNominalFreq(Device_p->ClkrvHandle,STCLKRV_CLOCK_HD_0, 108108000);
                  else
                     STCLKRV_SetNominalFreq(Device_p->ClkrvHandle,STCLKRV_CLOCK_HD_0, 54054000);
             #else
                  STCLKRV_SetNominalFreq(Device_p->ClkrvHandle,STCLKRV_CLOCK_HD_0, 54054000);
             #endif
            #endif

            #ifdef ST_7109
                if (CutId >= 0xB0)
                {
                  STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_DISPLAY_CFG,0x415);
                  STSYS_WriteRegDev32LE(SYSCFGBaseAddress+SYS_CFG34,0x1);
                }
                else
                {
                  STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_DISPLAY_CFG,0x0);
                }
            #else
                STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_DISPLAY_CFG,0x0);
            #endif

            #ifdef ST_7100
                  if ( CutId < 0xB0 )
                  {
                      STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_REF_MAX,0x0);
                      STSYS_WriteRegDev32LE(CKGBaseAddress+ CKGB_CMD ,0x1);
                      STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_FS1_MD2 ,0x1f);
                      STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_FS1_PE2 ,0x0);
                      STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_FS1_EN_PRG2 ,0x1);
                      STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_DISPLAY_CFG,0x600);

                  }
            #endif
            /*** Add Pix sd is routed from FS1 */
            {
              U32 Value=stvtg_Read32(CKGBaseAddress+CKGB_CLK_SRC);
              Value=Value|0x2;
              STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_CLK_SRC,Value);
            }
            if (Device_p->VtgId == STVTG_VTG_ID_1)
            {
                Dspcfg_Val = stvtg_Read32((U32)Device_p->BaseAddress_p+DSPCFG_CLK);
                Dspcfg_Val = Dspcfg_Val | (1<<10) ; /* 4* upsumpling*/
                STSYS_WriteRegDev32LE((U32)Device_p->BaseAddress_p+DSPCFG_CLK,Dspcfg_Val);

                /* PLL HDMI serializer configuration*/
                STOS_InterruptLock();
                data = stvtg_Read32(SYSCFGBaseAddress+SYS_CFG3);
                data &= 0x7ff;
                #ifdef ST_7109
                   if (CutId >= 0xB0)
                      data |= SERIALIZER_HDMI_RST|PLL_S_HDMI_ENABLE|PLL_S_HDMI_PDIV|PLL_S_HDMI_NDIV|PLL_S_HDMI_MDIV_HD;
                   else
                      data |= SERIALIZER_HDMI_RST|PLL_S_HDMI_ENABLE|PLL_S_HDMI_PDIV|PLL_S_HDMI_NDIV|PLL_S_HDMI_MDIV;
                #else
                   data |= SERIALIZER_HDMI_RST|PLL_S_HDMI_ENABLE|PLL_S_HDMI_PDIV|PLL_S_HDMI_NDIV|PLL_S_HDMI_MDIV;
                #endif
                STSYS_WriteRegDev32LE(SYSCFGBaseAddress+SYS_CFG3, data);
                STOS_InterruptUnlock();
            }
            break;

        case STVTG_TIMING_MODE_480P60000_25200:
            #ifdef STVTG_USE_CLKRV
                #ifdef ST_7109
                  if ( CutId >= 0xB0 )
                     STCLKRV_SetNominalFreq(Device_p->ClkrvHandle,STCLKRV_CLOCK_HD_0, 100800000);
                  else
                     STCLKRV_SetNominalFreq(Device_p->ClkrvHandle,STCLKRV_CLOCK_HD_0, 50400000);
                #else
                  STCLKRV_SetNominalFreq(Device_p->ClkrvHandle,STCLKRV_CLOCK_HD_0, 50400000);
                #endif
            #endif
            #ifdef ST_7109
                if (CutId >= 0xB0)
                {
                  STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_DISPLAY_CFG,0x415);
                  STSYS_WriteRegDev32LE(SYSCFGBaseAddress+SYS_CFG34,0x1);
                }
                else
                {
                  STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_DISPLAY_CFG,0x0);
                }
            #else
                STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_DISPLAY_CFG,0x0);
            #endif


            /* Add Pix sd is routed from FS1 */
            {
               U32 Value=stvtg_Read32(CKGBaseAddress+CKGB_CLK_SRC);
               Value=Value|0x2;
               STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_CLK_SRC,Value);
            }
            #ifdef ST_7100
                  if ( CutId < 0xB0 )
                  {
                      STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_REF_MAX,0x0);
                      STSYS_WriteRegDev32LE(CKGBaseAddress+ CKGB_CMD ,0x1);
                      STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_FS1_MD2 ,0x1f);
                      STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_FS1_PE2 ,0x0);
                      STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_FS1_EN_PRG2 ,0x1);
                      STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_DISPLAY_CFG,0x600);
                  }
            #endif
            if (Device_p->VtgId == STVTG_VTG_ID_1)
            {
                 Dspcfg_Val = stvtg_Read32((U32)Device_p->BaseAddress_p+DSPCFG_CLK);
                 Dspcfg_Val = Dspcfg_Val | (1<<10) ; /* 4* upsumpling*/
                 STSYS_WriteRegDev32LE((U32)Device_p->BaseAddress_p+DSPCFG_CLK,Dspcfg_Val);

                 STOS_InterruptLock();
                 data = stvtg_Read32 (SYSCFGBaseAddress+SYS_CFG3);
                 data &= 0x7ff;

                #ifdef ST_7109
                   if (CutId >= 0xB0)
                      data |= SERIALIZER_HDMI_RST|PLL_S_HDMI_ENABLE|PLL_S_HDMI_PDIV|PLL_S_HDMI_NDIV|PLL_S_HDMI_MDIV_HD;
                   else
                      data |= SERIALIZER_HDMI_RST|PLL_S_HDMI_ENABLE|PLL_S_HDMI_PDIV|PLL_S_HDMI_NDIV|PLL_S_HDMI_MDIV;
                #else
                   data |= SERIALIZER_HDMI_RST|PLL_S_HDMI_ENABLE|PLL_S_HDMI_PDIV|PLL_S_HDMI_NDIV|PLL_S_HDMI_MDIV;
                #endif

                 /* PLL HDMI serializer configuration*/
                 STSYS_WriteRegDev32LE(SYSCFGBaseAddress+SYS_CFG3,data);
                 STOS_InterruptUnlock();
            }
            break;
        case STVTG_TIMING_MODE_480P59940_25175:
            #ifdef STVTG_USE_CLKRV
               #ifdef ST_7109
                  if ( CutId >= 0xB0 )
                     STCLKRV_SetNominalFreq(Device_p->ClkrvHandle,STCLKRV_CLOCK_HD_0, 100700000);
                  else
                     STCLKRV_SetNominalFreq(Device_p->ClkrvHandle,STCLKRV_CLOCK_HD_0, 50350000);
               #else
                  STCLKRV_SetNominalFreq(Device_p->ClkrvHandle,STCLKRV_CLOCK_HD_0, 50350000);
               #endif
            #endif

            #ifdef ST_7109
                if (CutId >= 0xB0)
                {
                  STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_DISPLAY_CFG,0x415);
                  STSYS_WriteRegDev32LE(SYSCFGBaseAddress+SYS_CFG34,0x1);
                }
                else
                {
                  STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_DISPLAY_CFG,0x0);
                }
            #else
                STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_DISPLAY_CFG,0x0);
            #endif

            /* Add Pix sd is routed from FS1 */
            {
                U32 Value=stvtg_Read32(CKGBaseAddress+CKGB_CLK_SRC);
                Value=Value|0x2;
                STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_CLK_SRC,Value);
            }
            #ifdef ST_7100
                if ( CutId < 0xB0 )
                {
                    STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_REF_MAX,0x0);
                    STSYS_WriteRegDev32LE(CKGBaseAddress+ CKGB_CMD ,0x1);
                    STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_FS1_MD2 ,0x1f);
                    STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_FS1_PE2 ,0x0);
                    STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_FS1_EN_PRG2 ,0x1);
                    STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_DISPLAY_CFG,0x600);
                }
            #endif
            if (Device_p->VtgId == STVTG_VTG_ID_1)
            {
                  Dspcfg_Val = stvtg_Read32((U32)Device_p->BaseAddress_p+DSPCFG_CLK);
                  Dspcfg_Val = Dspcfg_Val | (1<<10) ; /* 4* upsumpling*/
                  STSYS_WriteRegDev32LE((U32)Device_p->BaseAddress_p+DSPCFG_CLK,Dspcfg_Val);

                  STOS_InterruptLock();
                  data = stvtg_Read32(SYSCFGBaseAddress+SYS_CFG3);
                  data &= 0x7ff;
                  #ifdef ST_7109
                   if (CutId >= 0xB0)
                      data |= SERIALIZER_HDMI_RST|PLL_S_HDMI_ENABLE|PLL_S_HDMI_PDIV|PLL_S_HDMI_NDIV|PLL_S_HDMI_MDIV_HD;
                   else
                      data |= SERIALIZER_HDMI_RST|PLL_S_HDMI_ENABLE|PLL_S_HDMI_PDIV|PLL_S_HDMI_NDIV|PLL_S_HDMI_MDIV;
                  #else
                   data |= SERIALIZER_HDMI_RST|PLL_S_HDMI_ENABLE|PLL_S_HDMI_PDIV|PLL_S_HDMI_NDIV|PLL_S_HDMI_MDIV;
                  #endif

                  /* PLL HDMI serializer configuration*/
                  STSYS_WriteRegDev32LE(SYSCFGBaseAddress+SYS_CFG3, data);
                  STOS_InterruptUnlock();
            }
            break;
        case STVTG_TIMING_MODE_480P60000_24570: /* ATSC P60 square*/
		    #ifdef STVTG_USE_CLKRV
               STCLKRV_SetNominalFreq(Device_p->ClkrvHandle,STCLKRV_CLOCK_HD_0, 49140000 );
            #endif
	   	  #ifdef ST_7109
		   	if (CutId >= 0xB0)
				{ 	STOS_InterruptLock();
				    STSYS_WriteRegDev32LE(SYSCFGBaseAddress+SYS_CFG34,0x0);
					STOS_InterruptUnlock();
				}
		    #endif
            STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_DISPLAY_CFG,0x0);
            /* Add Pix sd is routed from FS1 */
            {
                 U32 Value=stvtg_Read32(CKGBaseAddress+CKGB_CLK_SRC);
                 Value=Value|0x2;
                 STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_CLK_SRC,Value);
            }
            Dspcfg_Val = stvtg_Read32((U32)Device_p->BaseAddress_p+DSPCFG_CLK);
            Dspcfg_Val = Dspcfg_Val | (1<<10) ; /* 4* upsumpling*/
            STSYS_WriteRegDev32LE((U32)Device_p->BaseAddress_p+DSPCFG_CLK,Dspcfg_Val);
            break;
       case STVTG_TIMING_MODE_480P24000_10811: /* ATSC P24 */
		    #ifdef STVTG_USE_CLKRV
               STCLKRV_SetNominalFreq(Device_p->ClkrvHandle,STCLKRV_CLOCK_HD_0, 43244000 );
            #endif
	   	  #ifdef ST_7109
		   	if (CutId >= 0xB0)
				{ 	STOS_InterruptLock();
				    STSYS_WriteRegDev32LE(SYSCFGBaseAddress+SYS_CFG34,0x0);
					STOS_InterruptUnlock();
				}
		    #endif
            STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_DISPLAY_CFG,0x10);
            /* Add Pix sd is routed from FS1 */
            {
                 U32 Value=stvtg_Read32(CKGBaseAddress+CKGB_CLK_SRC);
                 Value=Value|0x2;
                 STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_CLK_SRC,Value);
            }

            Dspcfg_Val = stvtg_Read32((U32)Device_p->BaseAddress_p+DSPCFG_CLK);
            Dspcfg_Val = Dspcfg_Val | (1<<10) ; /* 4* upsumpling*/
            STSYS_WriteRegDev32LE((U32)Device_p->BaseAddress_p+DSPCFG_CLK,Dspcfg_Val);
            break;
       case STVTG_TIMING_MODE_480P24000_9828:  /* ATSC P24 square*/
		    #ifdef STVTG_USE_CLKRV
               STCLKRV_SetNominalFreq(Device_p->ClkrvHandle,STCLKRV_CLOCK_HD_0, 39312000 );
            #endif
	   	  #ifdef ST_7109
		   	if (CutId >= 0xB0)
				{ 	STOS_InterruptLock();
				    STSYS_WriteRegDev32LE(SYSCFGBaseAddress+SYS_CFG34,0x0);
					STOS_InterruptUnlock();
				}
		    #endif
            STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_DISPLAY_CFG,0x10);
            /* Add Pix sd is routed from FS1 */
            {
                 U32 Value=stvtg_Read32(CKGBaseAddress+CKGB_CLK_SRC);
                 Value=Value|0x2;
                 STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_CLK_SRC,Value);
            }
            Dspcfg_Val = stvtg_Read32((U32)Device_p->BaseAddress_p+DSPCFG_CLK);
            Dspcfg_Val = Dspcfg_Val | (1<<10) ; /* 4* upsumpling*/
            STSYS_WriteRegDev32LE((U32)Device_p->BaseAddress_p+DSPCFG_CLK,Dspcfg_Val);
             break;

        case STVTG_TIMING_MODE_768P75000_78750:
			#ifdef STVTG_USE_CLKRV
               STCLKRV_SetNominalFreq(Device_p->ClkrvHandle,STCLKRV_CLOCK_HD_0, 157500000);
            #endif
			 STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_DISPLAY_CFG,0x0);
	   	  #ifdef ST_7109
		   	if (CutId >= 0xB0)
				{ 	STOS_InterruptLock();
				    STSYS_WriteRegDev32LE(SYSCFGBaseAddress+SYS_CFG34,0x0);
					STOS_InterruptUnlock();
				}
		    #endif
		              /* Add Pix sd is routed from FS1 */
            {
             U32 Value=stvtg_Read32(CKGBaseAddress+CKGB_CLK_SRC);
             Value=Value|0x2;
             STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_CLK_SRC,Value);
            }
            Dspcfg_Val = stvtg_Read32((U32)Device_p->BaseAddress_p+DSPCFG_CLK);
            Dspcfg_Val = Dspcfg_Val & 0xBFF ;
            STSYS_WriteRegDev32LE((U32)Device_p->BaseAddress_p+DSPCFG_CLK,Dspcfg_Val);
            break;

        case STVTG_TIMING_MODE_768P60000_65000:
		    #ifdef STVTG_USE_CLKRV
               STCLKRV_SetNominalFreq(Device_p->ClkrvHandle,STCLKRV_CLOCK_HD_0, 130000000);
            #endif
	   	  #ifdef ST_7109
		   	if (CutId >= 0xB0)
				{ 	STOS_InterruptLock();
				    STSYS_WriteRegDev32LE(SYSCFGBaseAddress+SYS_CFG34,0x0);
					STOS_InterruptUnlock();
				}
		    #endif

            STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_DISPLAY_CFG,0x0);
            /* Add Pix sd is routed from FS1 */
            {
             U32 Value=stvtg_Read32(CKGBaseAddress+CKGB_CLK_SRC);
             Value=Value|0x2;
             STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_CLK_SRC,Value);
            }
            Dspcfg_Val = stvtg_Read32((U32)Device_p->BaseAddress_p+DSPCFG_CLK);
            Dspcfg_Val = Dspcfg_Val & 0xBFF ;
            STSYS_WriteRegDev32LE((U32)Device_p->BaseAddress_p+DSPCFG_CLK,Dspcfg_Val);
            break;

        case STVTG_TIMING_MODE_768P85000_94500:
		    #ifdef STVTG_USE_CLKRV
               STCLKRV_SetNominalFreq(Device_p->ClkrvHandle,STCLKRV_CLOCK_HD_0, 189000000);
            #endif
			   STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_DISPLAY_CFG,0x0);
		    #ifdef ST_7109
		   	if (CutId >= 0xB0)
				{ 	STOS_InterruptLock();
				    STSYS_WriteRegDev32LE(SYSCFGBaseAddress+SYS_CFG34,0x0);
					STOS_InterruptUnlock();
				}
		    #endif
			            /* Add Pix sd is routed from FS1 */
            {
             U32 Value=stvtg_Read32(CKGBaseAddress+CKGB_CLK_SRC);
             Value=Value|0x2;
             STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_CLK_SRC,Value);
            }
            Dspcfg_Val = stvtg_Read32((U32)Device_p->BaseAddress_p+DSPCFG_CLK);
            Dspcfg_Val = Dspcfg_Val & 0xBFF ;
            STSYS_WriteRegDev32LE((U32)Device_p->BaseAddress_p+DSPCFG_CLK,Dspcfg_Val);
            break;

        case STVTG_TIMING_MODE_576I50000_13500 :
        case STVTG_TIMING_MODE_480I59940_13500 :
		    /* Do it only one time for first vtg */
            if (Device_p->VtgId == STVTG_VTG_ID_1)
            {
              #ifdef STVTG_USE_CLKRV
                  /*STCLKRV_SetApplicationMode( Device_p->ClkrvHandle, STCLKRV_APPLICATION_MODE_SD_ONLY );*/
                  /* 54Mhz instead of 108Mhz */
                  STCLKRV_SetNominalFreq(Device_p->ClkrvHandle,STCLKRV_CLOCK_HD_0, 54000000);
              #endif
	   	  #ifdef ST_7109
		   	if (CutId >= 0xB0)
				{ 	STOS_InterruptLock();
				    STSYS_WriteRegDev32LE(SYSCFGBaseAddress+SYS_CFG34,0x0);
					STOS_InterruptUnlock();
				}
		    #endif


              #ifdef ST_7100
                 if ( CutId < 0xB0 )
                 {
                    STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_REF_MAX,0x0);
                    STSYS_WriteRegDev32LE(CKGBaseAddress+ CKGB_CMD ,0x1);
                    STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_FS1_MD2 ,0x1f);
                    STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_FS1_PE2 ,0x0);
                    STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_FS1_EN_PRG2 ,0x1);
                 }

              #endif
              data = stvtg_Read32((void*)(CKGB_DISPLAY_CFG+CKGBaseAddress));
              data = data & 0x2000;
              data = data |CLK_PIX_SD_3|CLK_565_3|CLK_DISP_HD_1 ;
              STSYS_WriteRegDev32LE(CKGB_DISPLAY_CFG+CKGBaseAddress,data);
              {
                  U32 Value=stvtg_Read32(CKGBaseAddress+CKGB_CLK_SRC);
                  /* STFAE - SD clock is routed from FS0 not FS1 and CLK_DVP_CPT sourced from VIDINCLKIN input pin*/
                  Value=Value&(~0xA);
                  STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_CLK_SRC,Value);
              }
              Dspcfg_Val = stvtg_Read32((U32)Device_p->BaseAddress_p+DSPCFG_CLK);
              Dspcfg_Val = Dspcfg_Val & 0xBFF ;
              STSYS_WriteRegDev32LE((U32)Device_p->BaseAddress_p+DSPCFG_CLK,Dspcfg_Val);

              /* This configuration is for HDMI and other outputs */
              data = stvtg_Read32((void*)(CKGB_DISPLAY_CFG+CKGBaseAddress));
              data |= 0x14;
              STSYS_WriteRegDev32LE(CKGB_DISPLAY_CFG+CKGBaseAddress,data);

              STOS_InterruptLock();
              data = stvtg_Read32 (SYSCFGBaseAddress+SYS_CFG3);
              data &= 0x7ff;
              data |= SERIALIZER_HDMI_RST|PLL_S_HDMI_ENABLE|PLL_S_HDMI_PDIV|PLL_S_HDMI_NDIV|PLL_S_HDMI_MDIV;
              /* PLL HDMI serializer configuration*/
              STSYS_WriteRegDev32LE(SYSCFGBaseAddress+SYS_CFG3, data);
              STOS_InterruptUnlock();
            }
            break;

        case  STVTG_TIMING_MODE_480I60000_13514 :   /* NTSC 60Hz */
        case  STVTG_TIMING_MODE_480P30000_13514 :   /* ATSC 30P */
            /* Do it only one time for first vtg */
            if (Device_p->VtgId == STVTG_VTG_ID_1)
            {
                #if defined STVTG_USE_CLKRV
                    /* 54Mhz instead of 108Mhz */
                    STCLKRV_SetNominalFreq(Device_p->ClkrvHandle,STCLKRV_CLOCK_HD_0, 54054000);
                #endif
	   	  #ifdef ST_7109
		   	if (CutId >= 0xB0)
				{ 	STOS_InterruptLock();
				    STSYS_WriteRegDev32LE(SYSCFGBaseAddress+SYS_CFG34,0x0);
					STOS_InterruptUnlock();
				}
		    #endif

                #ifdef ST_7100
                    if ( CutId < 0xB0 )
                    {
                       STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_REF_MAX,0x0);
                       STSYS_WriteRegDev32LE(CKGBaseAddress+ CKGB_CMD ,0x1);
                       STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_FS1_MD2 ,0x1f);
                       STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_FS1_PE2 ,0x0);
                       STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_FS1_EN_PRG2 ,0x1);
                    }

                #endif
                data = stvtg_Read32((void*)(CKGB_DISPLAY_CFG+CKGBaseAddress));
                data = data & 0x2000;
                data = data |CLK_PIX_SD_3|CLK_565_1|CLK_DISP_HD_1 ;
                STSYS_WriteRegDev32LE(CKGB_DISPLAY_CFG+CKGBaseAddress,data);
                {
                    U32 Value=stvtg_Read32(CKGBaseAddress+CKGB_CLK_SRC);
                    /* STFAE - SD clock is routed from FS0 not FS1 and CLK_DVP_CPT sourced from VIDINCLKIN input pin*/
                    Value=Value&(~0xA);
                    STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_CLK_SRC,Value);
                }
                Dspcfg_Val = stvtg_Read32((U32)Device_p->BaseAddress_p+DSPCFG_CLK);
                Dspcfg_Val = Dspcfg_Val & 0xBFF ;
                STSYS_WriteRegDev32LE((U32)Device_p->BaseAddress_p+DSPCFG_CLK,Dspcfg_Val);

                /* This configuration is for HDMI and other outputs */
                data = stvtg_Read32((void*)(CKGB_DISPLAY_CFG+CKGBaseAddress));
                data |= 0x14;
                STSYS_WriteRegDev32LE(CKGB_DISPLAY_CFG+CKGBaseAddress,data);
                STOS_InterruptLock();
                data = stvtg_Read32(SYSCFGBaseAddress+SYS_CFG3);
                data &= 0x7ff;
                data |= SERIALIZER_HDMI_RST|PLL_S_HDMI_ENABLE|PLL_S_HDMI_PDIV|PLL_S_HDMI_NDIV|PLL_S_HDMI_MDIV;
                /* PLL HDMI serializer configuration*/
                STSYS_WriteRegDev32LE(SYSCFGBaseAddress+SYS_CFG3, data);
                STOS_InterruptUnlock();
            }
            break;

        case STVTG_TIMING_MODE_576I50000_14750 :
            if (Device_p->VtgId == STVTG_VTG_ID_1)
            {
               #ifdef STVTG_USE_CLKRV
                  STCLKRV_SetNominalFreq(Device_p->ClkrvHandle,STCLKRV_CLOCK_HD_0, 59000000);
               #endif
	   	  #ifdef ST_7109
		   	if (CutId >= 0xB0)
				{ 	STOS_InterruptLock();
				    STSYS_WriteRegDev32LE(SYSCFGBaseAddress+SYS_CFG34,0x0);
					STOS_InterruptUnlock();
				}
		    #endif

               STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_DISPLAY_CFG,0x10);
               {
                    U32 Value=stvtg_Read32(CKGBaseAddress+CKGB_CLK_SRC);
                    /* STFAE - SD clock is routed from FS0 not FS1 */
                    Value=Value&(~0x2);
                    STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_CLK_SRC,Value);
               }
               Dspcfg_Val = stvtg_Read32((U32)Device_p->BaseAddress_p+DSPCFG_CLK);
               Dspcfg_Val = Dspcfg_Val & 0xBFF ;
               STSYS_WriteRegDev32LE((U32)Device_p->BaseAddress_p+DSPCFG_CLK,Dspcfg_Val);
            }
            break;

        case STVTG_TIMING_MODE_480I60000_12285 :
        case STVTG_TIMING_MODE_480I59940_12273 :
            if (Device_p->VtgId == STVTG_VTG_ID_1)
            {
              #ifdef STVTG_USE_CLKRV
                  STCLKRV_SetNominalFreq(Device_p->ClkrvHandle,STCLKRV_CLOCK_HD_0, 49140000);
              #endif
	   	  #ifdef ST_7109
		   	if (CutId >= 0xB0)
				{ 	STOS_InterruptLock();
				    STSYS_WriteRegDev32LE(SYSCFGBaseAddress+SYS_CFG34,0x0);
					STOS_InterruptUnlock();
				}
		    #endif

              STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_DISPLAY_CFG,0x10);
              {
                  U32 Value=stvtg_Read32(CKGBaseAddress+CKGB_CLK_SRC);
                  /* STFAE - SD clock is routed from FS0 not FS1 */
                  Value=Value&(~0x2);
                  STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_CLK_SRC,Value);
              }
              Dspcfg_Val = stvtg_Read32((U32)Device_p->BaseAddress_p+DSPCFG_CLK);
              Dspcfg_Val = Dspcfg_Val & 0xBFF ;
              STSYS_WriteRegDev32LE((U32)Device_p->BaseAddress_p+DSPCFG_CLK,Dspcfg_Val);
            }
            break;

        case STVTG_TIMING_MODE_720P30000_37125:  /* ATSC 720x1280 30P */

             #ifdef STVTG_USE_CLKRV
                STCLKRV_SetNominalFreq(Device_p->ClkrvHandle,STCLKRV_CLOCK_HD_0, 148500000);
            #endif
	   	  #ifdef ST_7109
		   	if (CutId >= 0xB0)
				{ 	STOS_InterruptLock();
				    STSYS_WriteRegDev32LE(SYSCFGBaseAddress+SYS_CFG34,0x0);
					STOS_InterruptUnlock();
				}
		    #endif

            STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_DISPLAY_CFG,0x10);
            /* Add Pix sd is routed from FS1 */
            {
              U32 Value=stvtg_Read32(CKGBaseAddress+CKGB_CLK_SRC);
              Value = Value | 0x2; /*Pix sd is routed from FS1*/
              STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_CLK_SRC,Value);
            }
            break;
        case STVTG_TIMING_MODE_720P24000_29700:  /* ATSC 720x1280 24P */
            #ifdef STVTG_USE_CLKRV
                STCLKRV_SetNominalFreq(Device_p->ClkrvHandle,STCLKRV_CLOCK_HD_0, 118800000);
            #endif
	   	  #ifdef ST_7109
		   	if (CutId >= 0xB0)
				{ 	STOS_InterruptLock();
				    STSYS_WriteRegDev32LE(SYSCFGBaseAddress+SYS_CFG34,0x0);
					STOS_InterruptUnlock();
				}
		    #endif

            STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_DISPLAY_CFG,0x10);
            /* Add Pix sd is routed from FS1 */
            {
              U32 Value=stvtg_Read32(CKGBaseAddress+CKGB_CLK_SRC);
              Value = Value | 0x2; /*Pix sd is routed from FS1*/
              STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_CLK_SRC,Value);
            }

            break;

        case STVTG_TIMING_MODE_720P59940_74176  :
        case STVTG_TIMING_MODE_1080I59940_74176 :
            #ifdef STVTG_USE_CLKRV
                STCLKRV_SetNominalFreq(Device_p->ClkrvHandle,STCLKRV_CLOCK_HD_0, 148351648);
            #endif
	   	  #ifdef ST_7109
		   	if (CutId >= 0xB0)
				{ 	STOS_InterruptLock();
				    STSYS_WriteRegDev32LE(SYSCFGBaseAddress+SYS_CFG34,0x0);
					STOS_InterruptUnlock();
				}
		    #endif


            #ifdef ST_7100
                if ( CutId < 0xB0 )
                {
                    STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_REF_MAX,0x0);
                    STSYS_WriteRegDev32LE(CKGBaseAddress+ CKGB_CMD ,0x1);
                    STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_FS1_MD2 ,0x1f);
                    STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_FS1_PE2 ,0x0);
                    STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_FS1_EN_PRG2 ,0x1);

                }
            #endif
            /* Pixel clock is rooted from FS1 and should be divided by 4 */
            /*STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_DISPLAY_CFG,0x400);*/
            /*no need to devide by 4*/
            STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_DISPLAY_CFG,0x0);
            /* Add Pix sd is routed from FS1 */
            {
              U32 Value=stvtg_Read32(CKGBaseAddress+CKGB_CLK_SRC);
              Value = Value | 0x2; /*Pix sd is routed from FS1*/
              STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_CLK_SRC,Value);
            }

            if (Device_p->VtgId == STVTG_VTG_ID_1)
            {
                Dspcfg_Val = stvtg_Read32((U32)Device_p->BaseAddress_p+DSPCFG_CLK);
                Dspcfg_Val = Dspcfg_Val & 0xBFF ;
                STSYS_WriteRegDev32LE((U32)Device_p->BaseAddress_p+DSPCFG_CLK,Dspcfg_Val);
                STOS_InterruptLock();
                data = stvtg_Read32(SYSCFGBaseAddress+SYS_CFG3);
                data &= 0x7ff;
                data |= SERIALIZER_HDMI_RST|PLL_S_HDMI_ENABLE|PLL_S_HDMI_PDIV|PLL_S_HDMI_NDIV|PLL_S_HDMI_MDIV_HD;
                /* PLL HDMI serializer configuration*/
                STSYS_WriteRegDev32LE(SYSCFGBaseAddress+SYS_CFG3, data);
                STOS_InterruptUnlock();
            }
            break;

        case STVTG_TIMING_MODE_SLAVE :   /*  VTG2 on slave mode : H_AND_V & V_ONLY*/
/*              data = STSYS_ReadRegDev32LE(CKGB_DISPLAY_CFG+CKGBaseAddress);
                data = data & 0x2000;
                data = data | CLK_DISP_ID_2 |CLK_PIX_SD_1;
                STSYS_WriteRegDev32LE(CKGB_DISPLAY_CFG+CKGBaseAddress,data);
*/
            break;

        case STVTG_TIMING_MODE_1080I60000_74250:
        case STVTG_TIMING_MODE_720P60000_74250:
        case STVTG_TIMING_MODE_720P50000_74250:
        case STVTG_TIMING_MODE_1080I50000_74250:
        case STVTG_TIMING_MODE_1080I50000_74250_1:
        case STVTG_TIMING_MODE_1080P30000_74250:
		case STVTG_TIMING_MODE_1080P25000_74250:
        case STVTG_TIMING_MODE_1080P24000_74250:
            #ifdef STVTG_USE_CLKRV
                STCLKRV_SetNominalFreq(Device_p->ClkrvHandle,STCLKRV_CLOCK_HD_0, 148500000);
            #endif
	   	  #ifdef ST_7109
		   	if (CutId >= 0xB0)
				{ 	STOS_InterruptLock();
				    STSYS_WriteRegDev32LE(SYSCFGBaseAddress+SYS_CFG34,0x0);
					STOS_InterruptUnlock();
				}
		    #endif

            #ifdef ST_7100
              if ( CutId < 0xB0 )
              {
                  STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_REF_MAX,0x0);
                  STSYS_WriteRegDev32LE(CKGBaseAddress+ CKGB_CMD ,0x1);
                  STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_FS1_MD2 ,0x1f);
                  STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_FS1_PE2 ,0x0);
                  STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_FS1_EN_PRG2 ,0x1);
              }
            #endif

            STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_DISPLAY_CFG,0x0);
            {
              U32 Value=stvtg_Read32(CKGBaseAddress+CKGB_CLK_SRC);
              Value = Value | 0x2; /*Pix sd is routed from FS1*/
              STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_CLK_SRC,Value);
            }

            if (Device_p->VtgId == STVTG_VTG_ID_1)
            {
                 Dspcfg_Val = stvtg_Read32((U32)Device_p->BaseAddress_p+DSPCFG_CLK);
                 Dspcfg_Val = Dspcfg_Val & 0xBFF ;
                 STSYS_WriteRegDev32LE((U32)Device_p->BaseAddress_p+DSPCFG_CLK,Dspcfg_Val);

                 STOS_InterruptLock();
                 data = stvtg_Read32(SYSCFGBaseAddress+SYS_CFG3);
                 data &= 0x7ff;
                 data |= SERIALIZER_HDMI_RST|PLL_S_HDMI_ENABLE|PLL_S_HDMI_PDIV|PLL_S_HDMI_NDIV|PLL_S_HDMI_MDIV_HD;
                 /* PLL HDMI serializer configuration*/
                 STSYS_WriteRegDev32LE(SYSCFGBaseAddress+SYS_CFG3, data);
                 STOS_InterruptUnlock();
            }
            break;

        case STVTG_TIMING_MODE_1080P29970_74176:
		case STVTG_TIMING_MODE_1080P23976_74176:
            #ifdef STVTG_USE_CLKRV
                STCLKRV_SetNominalFreq(Device_p->ClkrvHandle,STCLKRV_CLOCK_HD_0, 148352000);
            #endif
	   	  #ifdef ST_7109
		   	if (CutId >= 0xB0)
				{ 	STOS_InterruptLock();
				    STSYS_WriteRegDev32LE(SYSCFGBaseAddress+SYS_CFG34,0x0);
					STOS_InterruptUnlock();
				}
		    #endif

            #ifdef ST_7100
              if ( CutId < 0xB0 )
              {
                  STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_REF_MAX,0x0);
                  STSYS_WriteRegDev32LE(CKGBaseAddress+ CKGB_CMD ,0x1);
                  STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_FS1_MD2 ,0x1f);
                  STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_FS1_PE2 ,0x0);
                  STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_FS1_EN_PRG2 ,0x1);
              }
            #endif

            STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_DISPLAY_CFG,0x0);
            {
              U32 Value=stvtg_Read32(CKGBaseAddress+CKGB_CLK_SRC);
              Value = Value | 0x2; /*Pix sd is routed from FS1*/
              STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_CLK_SRC,Value);
            }

            if (Device_p->VtgId == STVTG_VTG_ID_1)
            {
                 Dspcfg_Val = stvtg_Read32((U32)Device_p->BaseAddress_p+DSPCFG_CLK);
                 Dspcfg_Val = Dspcfg_Val & 0xBFF ;
                 STSYS_WriteRegDev32LE((U32)Device_p->BaseAddress_p+DSPCFG_CLK,Dspcfg_Val);

                 STOS_InterruptLock();
                 data = stvtg_Read32(SYSCFGBaseAddress+SYS_CFG3);
                 data &= 0x7ff;
                 data |= SERIALIZER_HDMI_RST|PLL_S_HDMI_ENABLE|PLL_S_HDMI_PDIV|PLL_S_HDMI_NDIV|PLL_S_HDMI_MDIV_HD;
                 /* PLL HDMI serializer configuration*/
                 STSYS_WriteRegDev32LE(SYSCFGBaseAddress+SYS_CFG3, data);
                 STOS_InterruptUnlock();
            }
            break;

        case STVTG_TIMING_MODE_1152I50000_48000:
		   #ifdef STVTG_USE_CLKRV
                STCLKRV_SetNominalFreq(Device_p->ClkrvHandle,STCLKRV_CLOCK_HD_0, 96000000);
            #endif
	   	  #ifdef ST_7109
		   	if (CutId >= 0xB0)
				{ 	STOS_InterruptLock();
				    STSYS_WriteRegDev32LE(SYSCFGBaseAddress+SYS_CFG34,0x0);
					STOS_InterruptUnlock();
				}
		    #endif

            STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_CLK_SRC,0x2);
            STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_DISPLAY_CFG,0x0);

            if (Device_p->VtgId == STVTG_VTG_ID_1)
            {
                Dspcfg_Val = stvtg_Read32((U32)Device_p->BaseAddress_p+DSPCFG_CLK);
                Dspcfg_Val = Dspcfg_Val & 0xBFF ;
                STSYS_WriteRegDev32LE((U32)Device_p->BaseAddress_p+DSPCFG_CLK,Dspcfg_Val);
            }
            break;

        case STVTG_TIMING_MODE_1080I50000_72000:
            #ifdef STVTG_USE_CLKRV
                STCLKRV_SetNominalFreq(Device_p->ClkrvHandle,STCLKRV_CLOCK_HD_0, 144000000);
            #endif
	   	  #ifdef ST_7109
		   	if (CutId >= 0xB0)
				{ 	STOS_InterruptLock();
				    STSYS_WriteRegDev32LE(SYSCFGBaseAddress+SYS_CFG34,0x0);
					STOS_InterruptUnlock();
				}
		    #endif

            STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_CLK_SRC,0x2);
            STSYS_WriteRegDev32LE(CKGBaseAddress+CKGB_DISPLAY_CFG,0x0);

            if (Device_p->VtgId == STVTG_VTG_ID_1)
            {
                Dspcfg_Val = stvtg_Read32((U32)Device_p->BaseAddress_p+DSPCFG_CLK);
                Dspcfg_Val = Dspcfg_Val & 0xBFF ;
                STSYS_WriteRegDev32LE((U32)Device_p->BaseAddress_p+DSPCFG_CLK,Dspcfg_Val);
            }
            break;
        default:
            break;
    }
    /** Lock clock registers **/
    STSYS_WriteRegDev32LE(CKGB_LOCK+CKGBaseAddress,0xc1a0);

#elif defined(ST_7200)|| defined (ST_7111)|| defined (ST_7105)

       SDGlueBaseAddress = (U32)Device_p->SDGlueBaseAddress_p;
       HDGlueMainBaseAddress =(U32)Device_p->HDGlueMainBaseAddress_p;
       HDGlueAuxBaseAddress =(U32)Device_p->HDGlueAuxBaseAddress_p;

      switch (Device_p->TimingMode)
      {
          case STVTG_TIMING_MODE_480P59940_27000:/*no break*/
          case STVTG_TIMING_MODE_576P50000_27000:
                switch(Device_p->VtgId)
                {
                    case STVTG_VTG_ID_1:        /* For 7200 STVTG_VTG_ID_1 is HD primary */
                        #ifdef STVTG_USE_CLKRV
                              STCLKRV_SetNominalFreq(Device_p->ClkrvHandle,STCLKRV_CLOCK_HD_0, 108000000);
                        #endif
                         /*Synch pairs selection For ED output : HSYNC2 & VSYNC2 for DCS and analog output
                          *HSYNC2& VSYNC3 for HDMI &FLEXDVO*/
                         data = FLEXDVO_SYNC_3|HDFORMATTER_SYNC_2|HDMI_SYNC_3|DCS_AWG_ED_SYNC_2;
                         STSYS_WriteRegDev32LE(HTO_SYNC_SEL + HDGlueMainBaseAddress, data);

                         STSYS_WriteRegDev32LE(HTO_CTL_PADS + HDGlueMainBaseAddress, 0x50);

                         STSYS_WriteRegDev32LE(ST7200_CFG_BASE_ADDRESS+HDMI_PLL,  S_HDMI_RST_N|PLL_S_HDMI_ENABLE|PLL_S_HDMI_PDIV| \
                                                                     PLL_S_HDMI_NDIV|PLL_S_HDMI_MDIV);

                         Ckg_SetMode(CKGMODE_ED, CKGMODE_NO_CONFIGURATION, CKGMODE_NO_CONFIGURATION);
                         break;

                    case STVTG_VTG_ID_2:        /* For 7200 STVTG_VTG_ID_2 is HD local */
                    case STVTG_VTG_ID_3:        /* For 7200 STVTG_VTG_ID_3 is SD remote */
                        /* Not allowed */
                        break;

                    default:   /* Unknown VTG */
                        break;
                }
                break;

          case STVTG_TIMING_MODE_480P60000_27027:
                switch(Device_p->VtgId)
                {
                    case STVTG_VTG_ID_1:        /* For 7200 STVTG_VTG_ID_1 is HD primary */
                        #ifdef STVTG_USE_CLKRV
                              STCLKRV_SetNominalFreq(Device_p->ClkrvHandle,STCLKRV_CLOCK_HD_0, 108108000);
                        #endif
                         /*Synch pairs selection For ED output : HSYNC2 & VSYNC2 for DCS and analog output
                          *HSYNC2& VSYNC3 for HDMI & FLEXDVO */
                         data = FLEXDVO_SYNC_3|HDFORMATTER_SYNC_2|HDMI_SYNC_3|DCS_AWG_ED_SYNC_2;
                         STSYS_WriteRegDev32LE(HTO_SYNC_SEL + HDGlueMainBaseAddress, data);

                         STSYS_WriteRegDev32LE(HTO_CTL_PADS + HDGlueMainBaseAddress, 0x50);

                         STSYS_WriteRegDev32LE(ST7200_CFG_BASE_ADDRESS+HDMI_PLL,  S_HDMI_RST_N|PLL_S_HDMI_ENABLE|PLL_S_HDMI_PDIV| \
                                                                     PLL_S_HDMI_NDIV|PLL_S_HDMI_MDIV);

                         Ckg_SetMode(CKGMODE_ED, CKGMODE_NO_CONFIGURATION, CKGMODE_NO_CONFIGURATION);
                         break;

                    case STVTG_VTG_ID_2:        /* For 7200 STVTG_VTG_ID_2 is HD local */
                    case STVTG_VTG_ID_3:        /* For 7200 STVTG_VTG_ID_3 is SD remote */
                        /* Not allowed */
                        break;

                    default:   /* Unknown VTG */
                        break;
                }
                break;

          case STVTG_TIMING_MODE_480P60000_25200:
                switch(Device_p->VtgId)
                {
                    case STVTG_VTG_ID_1:        /* For 7200 STVTG_VTG_ID_1 is HD primary */
                        #ifdef STVTG_USE_CLKRV
                              STCLKRV_SetNominalFreq(Device_p->ClkrvHandle,STCLKRV_CLOCK_HD_0, 100800000);
                        #endif
                         /*Synch pairs selection For ED output : HSYNC2 & VSYNC2 for DCS and analog output
                          *HSYNC2& VSYNC3 for HDMI & FLEXDVO */
                         data = FLEXDVO_SYNC_3|HDFORMATTER_SYNC_2|HDMI_SYNC_3|DCS_AWG_ED_SYNC_2;
                         STSYS_WriteRegDev32LE(HTO_SYNC_SEL + HDGlueMainBaseAddress, data);


                         STSYS_WriteRegDev32LE(HTO_CTL_PADS + HDGlueMainBaseAddress, 0x50);

                         STSYS_WriteRegDev32LE(ST7200_CFG_BASE_ADDRESS+HDMI_PLL,  S_HDMI_RST_N|PLL_S_HDMI_ENABLE|PLL_S_HDMI_PDIV| \
                                                                     PLL_S_HDMI_NDIV|PLL_S_HDMI_MDIV);

                         data = STSYS_ReadRegDev32LE(ST7200_HD_TVOUT_HDF_BASE_ADDRESS+ DIG1_CFG);
                         data |= DIG1_SEL_INPUT_RGB;
                         STSYS_WriteRegDev32LE(ST7200_HD_TVOUT_HDF_BASE_ADDRESS+ DIG1_CFG, data);

                         Ckg_SetMode(CKGMODE_ED, CKGMODE_NO_CONFIGURATION, CKGMODE_NO_CONFIGURATION);
                         break;

                    case STVTG_VTG_ID_2:        /* For 7200 STVTG_VTG_ID_2 is HD local */
                    case STVTG_VTG_ID_3:        /* For 7200 STVTG_VTG_ID_3 is SD remote */
                        /* Not allowed */
                        break;

                    default:   /* Unknown VTG */
                        break;
                }
                break;

          case STVTG_TIMING_MODE_480P59940_25175:
                switch(Device_p->VtgId)
                {
                    case STVTG_VTG_ID_1:        /* For 7200 STVTG_VTG_ID_1 is HD primary */
                        #ifdef STVTG_USE_CLKRV
                              STCLKRV_SetNominalFreq(Device_p->ClkrvHandle,STCLKRV_CLOCK_HD_0, 100700000);
                        #endif
                         /*Synch pairs selection For ED output : HSYNC2 & VSYNC2 for DCS and analog output
                          *HSYNC3& VSYNC3 for HDMI & FLEXDVO*/
                         data = FLEXDVO_SYNC_3|HDFORMATTER_SYNC_2|HDMI_SYNC_3|DCS_AWG_ED_SYNC_2;
                         STSYS_WriteRegDev32LE(HTO_SYNC_SEL + HDGlueMainBaseAddress, data);


                         STSYS_WriteRegDev32LE(HTO_CTL_PADS + HDGlueMainBaseAddress, 0x50);

                         STSYS_WriteRegDev32LE(ST7200_CFG_BASE_ADDRESS+HDMI_PLL,  S_HDMI_RST_N|PLL_S_HDMI_ENABLE|PLL_S_HDMI_PDIV| \
                                                                     PLL_S_HDMI_NDIV|PLL_S_HDMI_MDIV);

                         data = STSYS_ReadRegDev32LE(ST7200_HD_TVOUT_HDF_BASE_ADDRESS+ DIG1_CFG);
                         data |= DIG1_SEL_INPUT_RGB;
                         STSYS_WriteRegDev32LE(ST7200_HD_TVOUT_HDF_BASE_ADDRESS+ DIG1_CFG, data);

                         Ckg_SetMode(CKGMODE_ED, CKGMODE_NO_CONFIGURATION, CKGMODE_NO_CONFIGURATION);
                         break;

                    case STVTG_VTG_ID_2:        /* For 7200 STVTG_VTG_ID_2 is HD local */
                    case STVTG_VTG_ID_3:        /* For 7200 STVTG_VTG_ID_3 is SD remote */
                        /* Not allowed */
                        break;

                    default:   /* Unknown VTG */
                        break;
                }
                break;

          case STVTG_TIMING_MODE_576I50000_13500 :
          case STVTG_TIMING_MODE_480I59940_13500 :
                switch(Device_p->VtgId)
                {
                    case STVTG_VTG_ID_1:        /* For 7200 STVTG_VTG_ID_1 is HD primary */
                        #ifdef STVTG_USE_CLKRV
                              STCLKRV_SetNominalFreq(Device_p->ClkrvHandle,STCLKRV_CLOCK_HD_0,108000000);
                        #endif
                         /*Synch pairs selection For SD output :HSYNC2&VSYNC2 for DCS HSYN3&VSYNC3 for HDMI/FLEXDVO*/
                         data = HDMI_SYNC_3|FLEXDVO_SYNC_3|DCS_AWG_ED_SYNC_2;
                         STSYS_WriteRegDev32LE(HTO_SYNC_SEL + HDGlueMainBaseAddress, data);

                         STSYS_WriteRegDev32LE(ST7200_CFG_BASE_ADDRESS+HDMI_PLL,  S_HDMI_RST_N|PLL_S_HDMI_ENABLE|PLL_S_HDMI_PDIV| \
                                                                     PLL_S_HDMI_NDIV|PLL_S_HDMI_MDIV);

                         data = CLK_SEL_PIX_MAIN_3|CLK_SEL_TMDS_MAIN_2|CLK_SEL_BCH_MAIN_1;
                         STSYS_WriteRegDev32LE(HTO_CLK_SEL + HDGlueMainBaseAddress, data);

                         Ckg_SetMode(CKGMODE_SD, CKGMODE_NO_CONFIGURATION, CKGMODE_NO_CONFIGURATION);
                         break;

                    case STVTG_VTG_ID_2:        /* For 7200 STVTG_VTG_ID_2 is HD local */
                        #ifdef STVTG_USE_CLKRV
                              STCLKRV_SetNominalFreq(Device_p->ClkrvHandle,STCLKRV_CLOCK_SD_0,27000000);
                        #endif
                         Ckg_SetMode(CKGMODE_NO_CONFIGURATION, CKGMODE_SD, CKGMODE_NO_CONFIGURATION);
                         break;

                    case STVTG_VTG_ID_3:        /* For 7200 STVTG_VTG_ID_3 is SD remote */
                        #ifdef STVTG_USE_CLKRV
                              STCLKRV_SetNominalFreq(Device_p->ClkrvHandle,STCLKRV_CLOCK_SD_1,27000000);
                        #endif
                         Ckg_SetMode(CKGMODE_NO_CONFIGURATION, CKGMODE_NO_CONFIGURATION, CKGMODE_SD);
                         break;
                        break;

                    default:   /* Unknown VTG */
                        break;
                }
                break;

          case  STVTG_TIMING_MODE_480I60000_13514 :
                switch(Device_p->VtgId)
                {
                    case STVTG_VTG_ID_1:        /* For 7200 STVTG_VTG_ID_1 is HD primary */
                        #ifdef STVTG_USE_CLKRV
                              STCLKRV_SetNominalFreq(Device_p->ClkrvHandle,STCLKRV_CLOCK_HD_0,108108000);
                        #endif
                        /*Synch pairs selection For SD output :HSYNC2&VSYNC2 for DCS HSYN3&VSYNC3 for HDMI/FLEXDVO*/
                         data = HDMI_SYNC_3|FLEXDVO_SYNC_3|DCS_AWG_ED_SYNC_2;
                         STSYS_WriteRegDev32LE(HTO_SYNC_SEL + HDGlueMainBaseAddress, data);

                         STSYS_WriteRegDev32LE(ST7200_CFG_BASE_ADDRESS+HDMI_PLL,  S_HDMI_RST_N|PLL_S_HDMI_ENABLE|PLL_S_HDMI_PDIV| \
                                                                     PLL_S_HDMI_NDIV|PLL_S_HDMI_MDIV);

                         Ckg_SetMode(CKGMODE_SD, CKGMODE_NO_CONFIGURATION, CKGMODE_NO_CONFIGURATION);
                         break;

                    case STVTG_VTG_ID_2:        /* For 7200 STVTG_VTG_ID_2 is HD local */
                        #ifdef STVTG_USE_CLKRV
                              STCLKRV_SetNominalFreq(Device_p->ClkrvHandle,STCLKRV_CLOCK_SD_0,27027000);
                        #endif
                         Ckg_SetMode(CKGMODE_NO_CONFIGURATION, CKGMODE_SD, CKGMODE_NO_CONFIGURATION);
                         break;

                    case STVTG_VTG_ID_3:        /* For 7200 STVTG_VTG_ID_3 is SD remote */
                        #ifdef STVTG_USE_CLKRV
                              STCLKRV_SetNominalFreq(Device_p->ClkrvHandle,STCLKRV_CLOCK_SD_1,27027000);
                        #endif
                         Ckg_SetMode(CKGMODE_NO_CONFIGURATION, CKGMODE_NO_CONFIGURATION, CKGMODE_SD);
                         break;
                        break;

                    default:   /* Unknown VTG */
                        break;
                }
                break;

          case STVTG_TIMING_MODE_576I50000_14750 :
            break;
          case STVTG_TIMING_MODE_480I60000_12285 :
          case STVTG_TIMING_MODE_480I59940_12273 :
            break;
          case STVTG_TIMING_MODE_720P59940_74176 :
			    switch(Device_p->VtgId)
                {
                    case STVTG_VTG_ID_1:
        				#ifdef STVTG_USE_CLKRV
                		STCLKRV_SetNominalFreq(Device_p->ClkrvHandle,STCLKRV_CLOCK_HD_0, 148351648);
            			#endif
						/*Synch pairs selection For HD output :  HSYNC2&VSYNC2 for analog/DCS and HSYN3&VSYNC3 for HDMI/FLEXDVO*/
                        	 data = FLEXDVO_SYNC_3|HDFORMATTER_SYNC_2|HDMI_SYNC_3|DCS_AWG_ED_SYNC_2;
                         	STSYS_WriteRegDev32LE(HTO_SYNC_SEL + HDGlueMainBaseAddress, data);

                         	STSYS_WriteRegDev32LE(HTO_CTL_PADS + HDGlueMainBaseAddress, 0x0);

                         	STSYS_WriteRegDev32LE(ST7200_CFG_BASE_ADDRESS+HDMI_PLL,  S_HDMI_RST_N|PLL_S_HDMI_ENABLE|PLL_S_HDMI_PDIV| \
                                                                     PLL_S_HDMI_NDIV|PLL_S_HDMI_MDIV);

                         	Ckg_SetMode(CKGMODE_HD, CKGMODE_NO_CONFIGURATION, CKGMODE_NO_CONFIGURATION);
                    break;
			      	case STVTG_VTG_ID_2:        /* For 7200 STVTG_VTG_ID_2 is HD local */
                    case STVTG_VTG_ID_3:        /* For 7200 STVTG_VTG_ID_3 is SD remote */
                        /* Not allowed */
                    break;

                    default:   /* Unknown VTG */
                        break;
                }
                break;
				case STVTG_TIMING_MODE_1080I59940_74176 :
			 	switch(Device_p->VtgId)
                {
                    case STVTG_VTG_ID_1:
						#ifdef STVTG_USE_CLKRV
                		STCLKRV_SetNominalFreq(Device_p->ClkrvHandle,STCLKRV_CLOCK_HD_0, 148351648);
            			#endif
				       		/*Synch pairs selection For HD output :  Href & Vref for analog, HSYNC2&VSYNC2 for DCS
                          	and HSYN3&VSYNC3 for HDMI/FLEXDVO*/
                         	data = FLEXDVO_SYNC_3|HDMI_SYNC_3|DCS_AWG_ED_SYNC_2;
                         	STSYS_WriteRegDev32LE(HTO_SYNC_SEL + HDGlueMainBaseAddress, data);

                         	STSYS_WriteRegDev32LE(HTO_CTL_PADS + HDGlueMainBaseAddress, 0x0);

                         	STSYS_WriteRegDev32LE(ST7200_CFG_BASE_ADDRESS+HDMI_PLL,  S_HDMI_RST_N|PLL_S_HDMI_ENABLE|PLL_S_HDMI_PDIV| \
                                                                     PLL_S_HDMI_NDIV|PLL_S_HDMI_MDIV);

                         	Ckg_SetMode(CKGMODE_HD, CKGMODE_NO_CONFIGURATION, CKGMODE_NO_CONFIGURATION);
                         	break;
					case STVTG_VTG_ID_2:        /* For 7200 STVTG_VTG_ID_2 is HD local */
                    case STVTG_VTG_ID_3:        /* For 7200 STVTG_VTG_ID_3 is SD remote */
                        /* Not allowed */
                        break;

                    default:   /* Unknown VTG */
                        break;
                }
                break;

          case STVTG_TIMING_MODE_SLAVE :   /*  VTG2 on slave mode : H_AND_V & V_ONLY*/
              break;

          case STVTG_TIMING_MODE_720P60000_74250:
          case STVTG_TIMING_MODE_720P50000_74250:
                switch(Device_p->VtgId)
                {
                    case STVTG_VTG_ID_1:        /* For 7200 STVTG_VTG_ID_1 is HD primary */
                        #ifdef STVTG_USE_CLKRV
                              STCLKRV_SetNominalFreq(Device_p->ClkrvHandle,STCLKRV_CLOCK_HD_0, 148500000);
                        #endif
                         /*Synch pairs selection For HD output :  HSYNC2&VSYNC2 for analog/DCS and HSYN3&VSYNC3 for HDMI/FLEXDVO*/
                         data = FLEXDVO_SYNC_3|HDFORMATTER_SYNC_2|HDMI_SYNC_3|DCS_AWG_ED_SYNC_2;
                         STSYS_WriteRegDev32LE(HTO_SYNC_SEL + HDGlueMainBaseAddress, data);

                         STSYS_WriteRegDev32LE(HTO_CTL_PADS + HDGlueMainBaseAddress, 0x0);

                         STSYS_WriteRegDev32LE(ST7200_CFG_BASE_ADDRESS+HDMI_PLL,  S_HDMI_RST_N|PLL_S_HDMI_ENABLE|PLL_S_HDMI_PDIV| \
                                                                     PLL_S_HDMI_NDIV|PLL_S_HDMI_MDIV);

                         data = STSYS_ReadRegDev32LE(ST7200_HD_TVOUT_HDF_BASE_ADDRESS+ DIG1_CFG);
                         data |= DIG1_SEL_INPUT_RGB;
                         STSYS_WriteRegDev32LE(ST7200_HD_TVOUT_HDF_BASE_ADDRESS+ DIG1_CFG, data);

                         Ckg_SetMode(CKGMODE_HD, CKGMODE_NO_CONFIGURATION, CKGMODE_NO_CONFIGURATION);
                         break;

                    case STVTG_VTG_ID_2:        /* For 7200 STVTG_VTG_ID_2 is HD local */
                    case STVTG_VTG_ID_3:        /* For 7200 STVTG_VTG_ID_3 is SD remote */
                        /* Not allowed */
                        break;

                    default:   /* Unknown VTG */
                        break;
                }
                break;

          case STVTG_TIMING_MODE_1080I60000_74250:
          case STVTG_TIMING_MODE_1080I50000_74250:
          case STVTG_TIMING_MODE_1080I50000_74250_1:
          case STVTG_TIMING_MODE_1080P30000_74250:
		  case STVTG_TIMING_MODE_1080P25000_74250:
          case STVTG_TIMING_MODE_1080P24000_74250:
                switch(Device_p->VtgId)
                {
                    case STVTG_VTG_ID_1:        /* For 7200 STVTG_VTG_ID_1 is HD primary */
                        #ifdef STVTG_USE_CLKRV
                              STCLKRV_SetNominalFreq(Device_p->ClkrvHandle,STCLKRV_CLOCK_HD_0, 148500000);
                        #endif

                         /*Synch pairs selection For HD output :  Href & Vref for analog, HSYNC2&VSYNC2 for DCS
                          and HSYN3&VSYNC3 for HDMI/FLEXDVO*/
                         data = FLEXDVO_SYNC_3|HDMI_SYNC_3|DCS_AWG_ED_SYNC_2;
                         STSYS_WriteRegDev32LE(HTO_SYNC_SEL + HDGlueMainBaseAddress, data);

                         STSYS_WriteRegDev32LE(HTO_CTL_PADS + HDGlueMainBaseAddress, 0x0);

                         STSYS_WriteRegDev32LE(ST7200_CFG_BASE_ADDRESS+HDMI_PLL,  S_HDMI_RST_N|PLL_S_HDMI_ENABLE|PLL_S_HDMI_PDIV| \
                                                                     PLL_S_HDMI_NDIV|PLL_S_HDMI_MDIV);

                         Ckg_SetMode(CKGMODE_HD, CKGMODE_NO_CONFIGURATION, CKGMODE_NO_CONFIGURATION);
                         break;

                    case STVTG_VTG_ID_2:        /* For 7200 STVTG_VTG_ID_2 is HD local */
                    case STVTG_VTG_ID_3:        /* For 7200 STVTG_VTG_ID_3 is SD remote */
                        /* Not allowed */
                        break;

                    default:   /* Unknown VTG */
                        break;
                }
                break;

          case STVTG_TIMING_MODE_1152I50000_48000:
            break;
          case STVTG_TIMING_MODE_1080I50000_72000:
            break;
          default:
              break;
      }

#endif

} /* End of ConfigSynth() function */

#if !(defined(ST_5100)|| defined(ST_7710)|| defined(ST_5105)|| defined(ST_7100)|| defined(ST_5301)|| defined(ST_7109)\
   || defined(ST_5188)|| defined(ST_5525)|| defined(ST_5107)|| defined(ST_7200)||defined(ST_5162)|| defined(ST_7111)|| defined(ST_7105))
/*****************************************************************************
Name        : GetSynthParam
Description : Get synthesizer parameters from table
Parameters  : Fout:  required output frequency
              ndiv / sdiv / md / pe: synthesizer parameter bits
Assumptions : required output frequency is known in FreqSynthElements table
Limitations :
Returns     : none
*****************************************************************************/
static void GetSynthParam(const U32 Fout, U32 * const ndiv, U32 * const sdiv, U32 * const md, U32 * const pe)
{
    U8 k;
    U32 ipe1 = 0;
    U32 sp   = 0;
    U32 md2  = 0;

    for(k=0; k<sizeof(FreqSynthElements)/sizeof(FreqSynthElements_t); k++)
    {
        if (FreqSynthElements[k].Fout == Fout)
        {
            sp = FreqSynthElements[k].sdiv;
            md2 = FreqSynthElements[k].md;
            ipe1 = FreqSynthElements[k].pe;
            break;
        }
    }
#if defined (BE_7015) || defined (BE_7020)
    *ndiv = (1    & 0x3);
#endif
    *sdiv = (sp   & 0x7);
    *md   = (md2  & 0x1f);
    *pe   = (ipe1 & 0xffff);
} /* End of GetSynthParam() function */
#endif
/* Functions (public)------------------------------------------------------- */


/******************************************************************************
Name        : stvtg_HALInitClockGen
Description : Init and Configure main & aux Pixel Clocks
Parameters  : Device_p (IN/OUT) : pointer on VTG device to access
Assumptions :
Limitations :
Returns     : None
*****************************************************************************/
void stvtg_HALInitClockGen(stvtg_Device_t * const Device_p)
{

    /* First time: initialize clock generator */
    if ( !FirstInitDone)
    {
        /* Configure synthesizers
         * Association :
         * SYNTH3 -> PIX_CLK
         * SYNTH7 -> AUX_CLK
         */
        switch (Device_p->DeviceType)
        {
#if defined (BE_7020) || defined (BE_7015)
             case STVTG_DEVICE_TYPE_VTG_CELL_7015 :
                ConfigSynth(Device_p, SYNTH3, DEFAULT_PIX_CLK);
                Device_p->CurrentSynth3Clock = DEFAULT_PIX_CLK;
#ifdef WA_GNBvd06791
                ConfigSynth(Device_p, SYNTH6, DEFAULT_DISP_CLK);
                Device_p->CurrentSynth6Clock = DEFAULT_DISP_CLK;
#endif
                break;
            case STVTG_DEVICE_TYPE_VTG_CELL_7020 :
#ifdef WA_PixClkExtern
                stvtg_ICS9161SetPixClkFrequency(DEFAULT_PIX_CLK/2);
                Device_p->CurrentSynth3Clock = DEFAULT_PIX_CLK/2;
#else
                ConfigSynth(Device_p, SYNTH3, DEFAULT_PIX_CLK);
                Device_p->CurrentSynth3Clock = DEFAULT_PIX_CLK;
#endif /* #ifdef WA_PixClkExtern */
                break;
#else
           case STVTG_DEVICE_TYPE_VFE2 :
#if defined(ST_5100)|| defined(ST_5105)|| defined(ST_5301)|| defined(ST_5188)|| defined(ST_5525)|| defined(ST_5107)\
 ||	defined(ST_5162)
                ConfigSynth(Device_p,SYNTH_CLK,DEFAULT_PIX_CLK);
#elif defined(ST_5528)
                ConfigSynth(Device_p,QSYNTH2_CLK4,DEFAULT_PIX_CLK);
#endif
                break;
#endif
            default :
                break;
        }
#if defined (BE_7015) || defined (BE_7020)
        ConfigSynth(Device_p, SYNTH7, DEFAULT_AUX_CLK);
        Device_p->CurrentSynth7Clock = DEFAULT_AUX_CLK;
#endif

#if defined (ST_7710) || defined (ST_7100)|| defined (ST_7109)|| defined (ST_7200)|| defined (ST_7111)|| defined (ST_7105)

        ConfigSynth(Device_p,FSYNTH, DEFAULT_AUX_CLK);
#endif

        FirstInitDone = TRUE;
    }
} /* end of stvtg_HALInitClockGen() */

/******************************************************************************
Name        : stvtg_HALSetPixelClock
Description : Set Pixel Clock
Parameters  : Device_p (IN/OUT) : pointer on VTG device to access
Assumptions : For WA_GNBvd06791: ADISP_CLK is set to 72MHz by STBOOT.
Limitations : For WA_GNBvd06791: Cannot output auxiliary path (VTG2, AUX_CLK)
 *            to main display (DISP_CLK).
Returns     : None
*****************************************************************************/
void stvtg_HALSetPixelClock(stvtg_Device_t * const Device_p)
{
    U32 PixClk;
#ifdef WA_GNBvd06791
    U32 DispClk;
#endif
    RegParams_t RegParams;
    ModeParamsLine_t *Line_p;

    /* Reach line in Table matching TimingMode */
    Line_p = stvtg_GetModeParamsLine(Device_p->TimingMode);

    /* Get corresponding register parameters in that line */
    RegParams = Line_p->RegParams;

    /* Warning : on STi7015/20 and on STi5528, VTG input clocks are half programmed pixel clocks */
    /* so multiplicate by 2 programmed pixel clocks to get right VTG clocks */
    PixClk = RegParams.PixelClock * 2;

    /* Configure synthesizers */
    if (Device_p->VtgId == STVTG_VTG_ID_1)
    {
        switch (Device_p->DeviceType)
        {
#if defined (BE_7015) || defined (BE_7020)
            case STVTG_DEVICE_TYPE_VTG_CELL_7015 :
                if (PixClk != Device_p->CurrentSynth3Clock)
                {
                    ConfigSynth(Device_p, SYNTH3, PixClk);
                    Device_p->CurrentSynth3Clock = PixClk;
                }
                break;
            case STVTG_DEVICE_TYPE_VTG_CELL_7020 :
#ifdef WA_PixClkExtern
                /* 7020 cut1 : SYNTH3 is extern */
                /* DSPCFG_CLK.NUP must be set for HD */
                if (Device_p->Upsample)
                {
                    if (PixClk != Device_p->CurrentSynth3Clock)
                    {
                        stvtg_ICS9161SetPixClkFrequency(PixClk);
                        Device_p->CurrentSynth3Clock = PixClk;
                    }
                }
                else
                {
                    if ((PixClk/2) != Device_p->CurrentSynth3Clock)
                    {
                        stvtg_ICS9161SetPixClkFrequency(PixClk/2);
                        Device_p->CurrentSynth3Clock = PixClk/2;
                    }
                }
#else /* of #ifdef WA_PixClkExtern */
                if (PixClk != Device_p->CurrentSynth3Clock)
                {
                    ConfigSynth(Device_p, SYNTH3, PixClk);
                    Device_p->CurrentSynth3Clock = PixClk;
                }
#endif /* #ifdef WA_PixClkExtern */
                break;
#else
           case STVTG_DEVICE_TYPE_VFE2:
             #if defined (ST_5100)||defined(ST_5105)||defined (ST_5301)||defined(ST_5188)||defined(ST_5525)||defined(ST_5107)\
				 || defined(ST_5162)
                        ConfigSynth(Device_p, SYNTH_CLK, PixClk);
                   #elif defined (ST_5528)
                 ConfigSynth(Device_p, QSYNTH2_CLK4, PixClk);
                   #endif
                break;
#endif
            default :
                break;
        } /* switch (Device_p->DeviceType) */
#ifdef WA_GNBvd06791
        if (PixClk < MIN_PIXCLK_FOR_DISPCLK)
        {
            if (PixClk >= PIXCLK27)
            {
                DispClk = DISP_CLOCK_FOR_PIXCLK27;
            }
            else if (PixClk >= PIXCLK24)
            {
                DispClk = DISP_CLOCK_FOR_PIXCLK24;
            }
            else if (PixClk >= PIXCLK21)
            {
                DispClk = DISP_CLOCK_FOR_PIXCLK21;
            }
            else
            {
                DispClk = DISP_CLOCK_FOR_PIXCLK19;
            }
            if (DispClk != Device_p->CurrentSynth6Clock)
            {
                ConfigSynth(Device_p, SYNTH6, DispClk);
                Device_p->CurrentSynth6Clock = DispClk;
            }
        }
        else
        {
            if (Device_p->CurrentSynth6Clock != DEFAULT_DISP_CLK)
            {
                ConfigSynth(Device_p, SYNTH6, DEFAULT_DISP_CLK);
                Device_p->CurrentSynth6Clock = DEFAULT_DISP_CLK;
            }
        }
#endif /* #ifdef WA_GNBvd06791 */
    } /* if (Device_p->VtgId == STVTG_VTG_ID_1) */
#if defined (BE_7015) || defined (BE_7020)
    if (Device_p->VtgId == STVTG_VTG_ID_2)
    {
        if (PixClk != Device_p->CurrentSynth7Clock)
        {
            ConfigSynth(Device_p, SYNTH7, PixClk);
            Device_p->CurrentSynth7Clock = PixClk;
        }
        /* WA_GNBvd06791: ADISP_CLK should be setup to 72MHz by STBOOT,
         * this fits PAL/NTSC (even square pixel) timings.
         * If auxiliary path (VTG2, AUX_CLK) is redirected to main
         * display (DISP_CLK) in SD, WA_GNBvd06791 is not respected.
         * This cross redirection is not known inside STVTG driver, only in STVMIX
         * and DISP_CLK must not be forced here unconditionally because it would
         * prevent from outputting HD on main and SD on aux simultaneously */
    }
#endif

#if defined (ST_7710) || defined (ST_7100) || defined (ST_7109)|| defined (ST_7200)|| defined (ST_7111) || defined (ST_7105)

        ConfigSynth(Device_p,FSYNTH, PixClk);

#endif

} /* End of stvtg_HALSetPixelClock() function */
#if defined (STVTG_VFE2)
/******************************************************************************
Name        : stvtg_HALSetPixelClockRst
Description : Pixel Clock configuration reset
Parameters  : Device_p (IN/OUT) : pointer on VTG device to access
Assumptions :
Limitations :
Returns     : None
*****************************************************************************/
void stvtg_HALSetPixelClockRst(stvtg_Device_t * const Device_p)
{
    U32 CKGBaseAddress;


    CKGBaseAddress=(U32)Device_p->CKGBaseAddress_p;

    if (Device_p->VtgId == STVTG_VTG_ID_1)
    {
        switch (Device_p->DeviceType)
        {
            case STVTG_DEVICE_TYPE_VFE2 :
                #if defined(ST_5528)
                    STSYS_WriteRegDev32LE((U32)CKGBaseAddress + CKG_LOCK_REG, CKG_UNLOCK);
                    STSYS_WriteRegDev32LE(((U32)CKGBaseAddress + CKG_SYN2_CLK4),CKG_SYN2_CLK4_RST);
                    STSYS_WriteRegDev8(((U32)CKGBaseAddress + PIX_CLK_CTRL), PIX_CLK_RST);
                    STSYS_WriteRegDev32LE((U32)CKGBaseAddress + CKG_LOCK_REG, CKG_LOCK);
              #elif defined(ST_5100)||defined(ST_5105)||defined(ST_5301)||defined(ST_5188)||defined(ST_5525)||defined(ST_5107)\
				 || defined(ST_5162)
                    STSYS_WriteRegDev32LE(((U32)CKGBaseAddress+DCO_MODE_CFG), DCO_MODE_CFG_PRG);
                    STSYS_WriteRegDev32LE(((U32)CKGBaseAddress+CKG_PIX_CLK_SETUP0),PIX_CLK_SETUP0_RST);
                    STSYS_WriteRegDev32LE(((U32)CKGBaseAddress+CKG_PIX_CLK_SETUP1), PIX_CLK_SETUP1_RST);
                #endif
            break;

            default :
                break;
        }
    }
}
#endif
#ifdef WA_PixClkExtern
/******************************************************************************
Name        : stvtg_SetPixelClockRatio
Description : Set Pixel Clock ratio depending on output: single for HD, twice for
 *            DENC internal or external.
 *            Function needed only for 7020 cut1.
Parameters  : Handle (IN) : VTG device to access
 *            Upsample : TRUE => pixclk * 2.
Assumptions :
Limitations :
Returns     : None
*****************************************************************************/
void stvtg_SetPixelClockRatio( const STVTG_Handle_t Handle, BOOL Upsample)
{
    ModeParamsLine_t *Line_p;
    stvtg_Unit_t *Unit_p;

    if (!IsValidHandle(Handle))
    {
        return;
    }
    Unit_p = (stvtg_Unit_t *)Handle;

    if (   (Unit_p->Device_p->VtgId == STVTG_VTG_ID_1)
        && (Unit_p->Device_p->DeviceType == STVTG_DEVICE_TYPE_VTG_CELL_7020)
        && (Unit_p->Device_p->Upsample != Upsample))
    {
        Unit_p->Device_p->Upsample = Upsample;
        Line_p = stvtg_GetModeParamsLine(Unit_p->Device_p->TimingMode);
        if (Upsample)
        {
            stvtg_ICS9161SetPixClkFrequency(Line_p->RegParams.PixelClock * 2);
        }
        else
        {
            stvtg_ICS9161SetPixClkFrequency(Line_p->RegParams.PixelClock);
        }
    }
} /* end of stvtg_SetPixelClockRatio()  */
#endif /* WA_PixClkExtern */

#if defined(ST_5100)|| defined(ST_5105)|| defined(ST_5301)|| defined(ST_5188)|| defined(ST_5525)|| defined(ST_5107)\
||  defined(ST_5162)
/*------------------------------------------------------------------------------
 Name    : ComputeFrequencySynthesizerParameters
 Purpose : Computes synthesizer parameters from input/output frequencies
 In      : InputFrequency   : input reference frequency (27 MHz)
           OutputFrequency  : required output frequency
 Out     : *ndiv, *sdiv, *md and *pe : synthesizer parameters
           returns false if no error occurs, otherwise true
 Note    : -
------------------------------------------------------------------------------*/
static BOOL ComputeFrequencySynthesizerParameters(const U32 InputFrequency,
                                                  const U32 OutputFrequency,
                                                     U32 * const ndiv1, U32 * const sdiv1,
                                                     U32 * const md1, U32 * const pe1)
{
  double      OutFrequency, InFrequency, foutr, fout2, fr,Tdl, Tr, Tdif,
              resol, diff;
  S32         k, m, n , s=512 ,ipe1, ipe2, nd1, nd2,Ntap, sp=8, np=3, md2;


  OutFrequency = (double)OutputFrequency;
  InFrequency  = (double)InputFrequency;

  /* k = number of bits for compute ipe digital bus */
  k = 16;
  if (!((InFrequency == 13500000) || (InFrequency == 27000000) || (InFrequency == 54000000)) )

  {
    /** Input frequency not allowed for this synthesizer **/
    return(TRUE);
  }
  if ( ( (OutFrequency <= 843750) || (OutFrequency >= 216000000) ) &&
      (OutFrequency != 0) )
  {
     /** Output frequency not allowed for this synthetizer **/
     return(TRUE);
  }

    /* Ntap = Number of taps at interpolators exit */
    Ntap = 32;

    /* m = division rank of internal divider */
    m = 16;

    /* Divider N rank */
    n = (S32)(InFrequency / 13500000);

    /* fr = vco ring frequency */
    fr = (16*InFrequency / n);
    Tr = (1 / fr);
    Tdl = (1 / (Ntap*fr));


    /* divider s computation */
    if (OutFrequency >= 108000000)
    {
      s = 2;
      sp = 0;
    }
    else if (OutFrequency >= 54000000)
    {
      s = 4;
      sp = 1;
    }
    else if (OutFrequency >= 27000000)
    {
      s = 8;
      sp = 2;
    }
    else if (OutFrequency >= 13500000)
    {
      s = 16;
      sp = 3;
    }
    else if (OutFrequency >= 6750000)
    {
      s = 32;
      sp = 4;
    }
    else if (OutFrequency >= 3375000)
    {
      s = 64;
      sp = 5;
    }
    else if (OutFrequency >= 1687500)
    {
      s = 128;
      sp = 6;
    }
    else if (OutFrequency >= 843750)
    {
      s = 256;
      sp = 7;
    }

    Tdif = (1/fr) - (1/(s*OutFrequency));
    nd1 = (S32) floor(Tdif/Tdl);
    nd2 = nd1 + 1;
    ipe1 = (S32)(pow(2, (double)(k-1.0))*(Tdif - (double)nd1*Tdl)/Tdl);
    ipe2 =(S32) (ipe1 - pow(2, (double)(k-1.0)));
    foutr = OutFrequency;
    OutFrequency  = pow(2, (double)(k-1.0)) /
              (s*((double)ipe1*(Tr-(double)nd2*Tdl)-ipe2*(Tr-(double)nd1*Tdl)));
    fout2 = pow(2, (double)(k-1.0))/(s*(((double)ipe1+1.0)*
                (Tr-(double)nd2*Tdl)-((double)ipe2+1.0)*(Tr-(double)nd1*Tdl)));
    diff  = OutFrequency - foutr;
    resol = (fout2/OutFrequency -1)*1000000;

    switch (n)
    {
      case 1 :
        np = 0;
        break;
      case 2 :
        np = 1;
        break;
      case 4 :
        np = 2;
        break;
    }
     md2 = -nd2;
    *ndiv1 = (np & 0x3);
    *sdiv1 = (sp & 0x7);
    *md1   = (md2 & 0x1f);
    *pe1   = (ipe1 & 0xffff);

  return(FALSE);
}
#endif
/*-----------------9/13/2004 10:55AM----------------
 * Program clock for 7710
 * --------------------------------------------------*/
#if defined ST_7710
void Stop_PLL_Rej(const U32 CKGBasAddress){
  /* stop clock by setting PLL_CONFIG_2.poff to 1 */
  STSYS_WriteRegDev32LE(CKGBasAddress+PLL2_CONFIG_2,0x5AA50002);
  /* wait 1 ms */
  STOS_TaskDelay(ST_GetClocksPerSecond()/1000);
}

void Change_Freq_PixHD_CLK(const U32 CKGBasAddress,U32 sdiv, U32 md, U32 pe){
  U32 data;
  /* Stop clock */
  data = stvtg_Read32(FS_CLOCKGEN_CFG_0+CKGBasAddress);
  data = data | 0x5AA50000 ;   /* key for registers access */
  data = data | 0x00000008 ;   /* clk_pixhd is switched off  */
  STSYS_WriteRegDev32LE(FS_CLOCKGEN_CFG_0+CKGBasAddress,data);
  /* Change parameters and set en_prog to '0' */
  data = stvtg_Read32(FS_04_CONFIG_1+CKGBasAddress);
  /* pe */
  data = (U32)pe & 0x0000FFFF ;
  data = data | 0x5AA50000 ;   /* key for registers access */
  STSYS_WriteRegDev32LE(FS_04_CONFIG_1+CKGBasAddress,data);
  data = stvtg_Read32(FS_04_CONFIG_0+CKGBasAddress);
  /*  md,sdiv,sel_out=1  ,reset_n=1  ,en_prog=0 */
  data = 0x00000006 ; /* sel_out=1  ,reset_n=1 , en_prog=0  */
  data = ( (U32)(((U32)md<<6)|((U32)sdiv<<3)) & 0x0000FFF8 ) | data ;
  data = data | 0x5AA50000 ;   /* key for registers access */
  STSYS_WriteRegDev32LE(FS_04_CONFIG_0+CKGBasAddress,data);
  /* en_prog = 1 */
  data = data | 0x00000001 ;
  STSYS_WriteRegDev32LE(FS_04_CONFIG_0+CKGBasAddress,data);
  /* en_prog = 0 */
  data = data & 0xFFFFFFFE ;
  STSYS_WriteRegDev32LE(FS_04_CONFIG_0+CKGBasAddress,data);
  /* Enable clock */
  data = stvtg_Read32(FS_CLOCKGEN_CFG_0+CKGBasAddress);
  data = data | 0x5AA50000 ;   /* key for registers access */
  data = data & 0xFFFFFFF7 ;   /* clk_pixhd is switched on   */
  STSYS_WriteRegDev32LE(FS_CLOCKGEN_CFG_0+CKGBasAddress,data);
}

/* PLL2 : rejection PLL for HDMI analog block  */
void Start_PLL_Rej(const U32 CKGBasAddress){
 /*  U32 data;*/
  /* start PLL1 by setting PLL_CONFIG_2.poff to 0  fbenable=0 rstn=1 */
  STSYS_WriteRegDev32LE(PLL2_CONFIG_2+CKGBasAddress,0x5AA50004);
}
#ifdef WA_GNBvd35956
void Change_Freq_Disp_CLK(const U32 CKGBASEAddress, const U32 sdiv, const U32 md, const U32 pe)
{

  U32 data;
  /* Stop clock */
  data = stvtg_Read32(FS_CLOCKGEN_CFG_0+CKGBasAddress);
  data = data | 0x5AA50000 ;   /* key for registers access */
  data = data | 0x00000200 ;   /* clk_dispclk is switched off  */
  STSYS_WriteRegDev32LE(FS_CLOCKGEN_CFG_0+CKGBasAddress,data);
  /* Change parameters and set en_prog to '0' */
  data = stvtg_Read32(FS_11_CONFIG_1+CKGBasAddress);
  /* pe */
  data = (U32)pe & 0x0000FFFF ;
  data = data | 0x5AA50000 ;   /* key for registers access */
  STSYS_WriteRegDev32LE(FS_11_CONFIG_1+CKGBasAddress,data);
  data = stvtg_Read32(FS_11_CONFIG_0+CKGBasAddress);
  /*  md,sdiv,sel_out=1  ,reset_n=1  ,en_prog=0 */
  data = 0x00000006 ; /* sel_out=1  ,reset_n=1 , en_prog=0  */
  data = ( (U32)(((U32)md<<6)|((U32)sdiv<<3)) & 0x0000FFF8 ) | data ;
  data = data | 0x5AA50000 ;   /* key for registers access */
  STSYS_WriteRegDev32LE(FS_11_CONFIG_0+CKGBasAddress,data);
  /* en_prog = 1 */
  data = data | 0x00000001 ;
  STSYS_WriteRegDev32LE(FS_11_CONFIG_0+CKGBasAddress,data);
  /* en_prog = 0 */
  data = data & 0xFFFFFFFE ;
  STSYS_WriteRegDev32LE(FS_11_CONFIG_0+CKGBasAddress,data);
  /* Enable clock */
  data = stvtg_Read32(FS_CLOCKGEN_CFG_0+CKGBasAddress);
  data = data | 0x5AA50000 ;   /* key for registers access */
  data = data & 0xFFFFFDFF ;   /* clk_pixsd is switched on   */
  STSYS_WriteRegDev32LE(FS_CLOCKGEN_CFG_0+CKGBasAddress,data);

}
#endif
void Change_Freq_Pipe_CLK(const U32 CKGBasAddress,U32 sdiv, U32 md, U32 pe){
  U32 data;
  /* Stop clock */
  data = stvtg_Read32(FS_CLOCKGEN_CFG_0+CKGBasAddress);
  data = data | 0x5AA50000 ;   /* key for registers access */
  data = data | 0x00000800 ;   /* clk_pipe    is switched off  */
  STSYS_WriteRegDev32LE(FS_CLOCKGEN_CFG_0+CKGBasAddress,data);
  /* Change parameters and set en_prog to '0' */
  data = stvtg_Read32(FS_12_CONFIG_1+CKGBasAddress);
  /* pe */
  data = (U32)pe & 0x0000FFFF ;
  data = data | 0x5AA50000 ;   /* key for registers access */
  STSYS_WriteRegDev32LE(FS_12_CONFIG_1+CKGBasAddress,data);
  data = stvtg_Read32(FS_12_CONFIG_0+CKGBasAddress);
  /*  md,sdiv,sel_out=1  ,reset_n=1  ,en_prog=0 */
  data =  0x00000006 ; /* sel_out=1  ,reset_n=1 , en_prog=0  */
  data = ( (U32)(((U32)md<<6)|((U32)sdiv<<3)) & 0x0000FFF8 ) | data ;
  data = data | 0x5AA50000 ;   /* key for registers access */
  STSYS_WriteRegDev32LE(FS_12_CONFIG_0+CKGBasAddress,data);
  /* en_prog = 1 */
  data = data | 0x00000001 ;
  STSYS_WriteRegDev32LE(FS_12_CONFIG_0+CKGBasAddress,data);
  /* en_prog = 0 */
  data = data & 0xFFFFFFFE ;
  STSYS_WriteRegDev32LE(FS_12_CONFIG_0+CKGBasAddress,data);
  /* Enable clock */
  data = stvtg_Read32(FS_CLOCKGEN_CFG_0+CKGBasAddress);
  data = data | 0x5AA50000 ;   /* key for registers access */
  data = data & 0xFFFFF7FF ;   /* clk_pipe  is switched on   */
  STSYS_WriteRegDev32LE(FS_CLOCKGEN_CFG_0+CKGBasAddress,data);
}

#endif

/*-----------------9/13/2004 10:55AM----------------
 * End programming clock for STi7710
 * --------------------------------------------------*/


/* End of clk_gen.c */




