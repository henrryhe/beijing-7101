/*******************************************************************************
File Name   : hd_reg.h

Description : 7015/20 out registers

COPYRIGHT (C) STMicroelectronics 2003

*******************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __HD_REG_H
#define __HD_REG_H

/* Includes ----------------------------------------------------------------- */
#include "stsys.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Exported Types ----------------------------------------------------------- */

/* Exported Constants ------------------------------------------------------- */

/* 7015/20: Configuration registers */
#define CFG_CCF     0x04   /* Chip configuration */

#define CFG_CCF_EVI 0x01   /* Enable digital video interface*/


/* 7015: Auxiliary analog video output registers */
#define DENC_CFG    0x3C0  /* Configuration register  CLKRUN and Denc On      */
#define DENC_TTX    0x3C4  /* Start address of teletext file                  */

/* 7710: register Display Output Configuration  */
#if defined (ST_7710)|| defined (ST_7100) || defined (ST_7109)
#define DSPCFG_CLK  0x70  /* Display clocks and global configuration */
#define DSPCFG_DIG  0x74  /* Display digital out configuration */
#define DSPCFG_ANA  0x78  /* Main analog display out configuration */
#define DSPCFG_CLK_UPSEL        0x1 /* select Upsampler Mode  */
#define DSPCFG_CLK_UPSEL_MASK   0x8 /* select Upsampler Mode MASK  */
#define DSPCFG_CLK_NUP_SD       0x1 /* Enable SD Upsampler Mode  */
#define DSPCFG_CLK_NUP_SD_MASK  0x4 /* Enable SD Upsampler Mode MASK */
#define DSPCFG_CLK_UPS_DIV_HD   0x1 /*: UPS post-filter division =256*/
#define DSPCFG_CLK_UPS_DIV_HDRQ 0x2 /*: UPS post-filter division =1024*/
#define DSPCFG_CLK_UPS_DIV_SD   0x0 /*: UPS post-filter division =512*/
#define DSPCFG_CLK_UPS_DIV_MASK 0xffffffcf /*: UPS post-filter division MASK*/
#define DSPCFG_CLK_RST          0x0 /* Reset DSPCFG_CLK register */
#if defined (ST_7710)
#define DSPCFG_CLK_HD4x_MASK    0x80 /* Upsampler 4x even on HD mode*/
#else
#define DSPCFG_CLK_HD4x_MASK    0x400 /* Upsampler 4x even on HD mode*/
#endif
#elif defined (ST_7200)|| defined (ST_7111)|| defined (ST_7105)
#define ANA_SRC_CONFIG  0x14
#else
/* 7015/20: register Display Output Configuration  */
#define DSPCFG_CLK  0x00  /* Display clocks and global configuration */
#define DSPCFG_DIG  0x04  /* Display digital out configuration */
#define DSPCFG_ANA  0x08  /* Main analog display out configuration */
#endif

#define DSPCFG_CAPT 0x0C  /* capture configuration */

#if defined(ST_7710) || defined (ST_7100) || defined (ST_7109)
#define DHDO_ACFG   0x07C /* Analog output confgiuration */
#define DHDO_COLOR  0x0B4 /* Main display output color space selection */
#elif !(defined (ST_7200)|| defined (ST_7111)|| defined (ST_7105))
#define DHDO_ACFG   0x100 /* Analog output confgiuration */
#define DHDO_COLOR  0x138 /* Main display output color space selection */
#endif
#if defined (ST_7710)
#define DSPCFG_TST  0xe0  /* Test configuration */
#elif !(defined (ST_7200)|| defined(ST_7111)|| defined (ST_7105))
#define DSPCFG_TST  0x2000  /* Test configuration */
#endif
#define DSPCFG_SVM  0x2004  /* SVM static value */
#define DSPCFG_REF  0x2008  /* REF static value*/
#if defined (ST_7710) || defined(ST_7100) || defined (ST_7109)
#define DSPCFG_DAC  0xDC  /* Dac configuration */
#elif !(defined (ST_7200)|| defined (ST_7111)|| defined (ST_7105))
#define DSPCFG_DAC  0x200C  /* Dac configuration */
#endif
/* 7015/20: register Display Output Configuration  */
#define DENC_CFG_CLKRUN        0x02 /* Clkrun insertion                      */
#define DENC_CFG_ON            0x01 /* DENC On                               */

#define DSPCFG_CLK_NUP         0x10  /* No upsample */
#define DSPCFG_CLK_INVCK       0x08  /* Invert PIXCLK */
#define DSPCFG_CLK_DESL        0x04  /* DENC selection */
#define DSPCFG_CLK_DSDS        0x02  /* Digital standard definition output selection */
#define DSPCFG_CLK_PNR         0x01  /* PIP not record, only 7015 */
#define DSPCFG_DIG_SYSD        0x08  /* Synchronisation on SD path */
#if defined (ST_7710) || defined(ST_7100) || defined (ST_7109)
#define DSPCFG_DIG_RGB         0x04  /* RGB/YCbCr out --not defined on 7710*/
#define DSPCFG_DIG_HDS         0x01  /* High definition selection */
#define DSPCFG_DIG_EN          0x02  /* DVO enable*/
#elif !(defined (ST_7200)|| defined (ST_7111)|| defined (ST_7105))
#define DSPCFG_DIG_HDS         0x04  /* High definition selection */
#define DSPCFG_DIG_RGB         0x01  /* RGB/YCbCr out */
#endif
#define DSPCFG_DIG_SYHD        0x02  /* Synchronisation on HD path */
#define DSPCFG_ANA_RSC         0x04  /* Rescale */
#define DSPCFG_ANA_SYHD        0x02  /* Synchronisation on HD path */
#define DSPCFG_ANA_RGB         0x01  /* RGB/YCbCr out */
#define DSPCFG_CAPT_SEL        0x01  /* capture configuration */
/*7020 only*/
#define DSPCFG_TST_RSVM        0x80  /* Reference on SVM */
#define DSPCFG_TST_TDAC4       0x40  /* Test Dac4 */
#define DSPCFG_TST_TDAC3       0x20  /* Test Dac3 */
#define DSPCFG_TST_TDAC2       0x10  /* Test Dac2 */
#define DSPCFG_TST_TDAC1       0x08  /* Test Dac1 */
#define DSPCFG_TST_TDAC0       0x04  /* Test Dac0 */
#define DSPCFG_TST_SD2D        0x02  /* SD in to DENC */
#define DSPCFG_TST_TZO         0x01  /* TZ Observation */

#define DSPCFG_REF_MASK        0x03FF  /* Mask for REF static value*/

#if defined(ST_7710)  || defined(ST_7100)
#define DSPCFG_DAC_BLRGB       0x00000040  /* blank on RGB configuration on STi7710 only...*/
#elif !(defined (ST_7200)|| defined(ST_7111)|| defined (ST_7105))
#define DSPCFG_DAC_BLRGB       0x00010000  /* blank on RGB configuration */
#endif
#define DSPCFG_DAC_BLREF       0x00008000  /* blank on REF configuration */
#define DSPCFG_DAC_BLSVM       0x00004000  /* blank on SVM configuration */
#if defined (ST_7710)
#define DSPCFG_DAC_PD          0x00000080  /* power down */
#elif !(defined (ST_7200)||defined (ST_7111)|| defined (ST_7105))
#define DSPCFG_DAC_PD          0x00002000  /* power down */
#endif

#if defined (ST_7100)||defined (ST_7109)
#define DSPCFG_DAC_PO_SD       0x00000080  /* power ON SD */
#define DSPCFG_DAC_PO_HD       0x00000100  /* power on HD*/
#define DSPCFG_DAC_PO          0x00000180  /* power on */
#endif

#define DSPCFG_DAC_BL_RCR      0x00001000  /* blank on R/Cr dac output */
#define DSPCFG_DAC_BL_GY       0x00000800  /* blank on G/Y  dac output */
#define DSPCFG_DAC_BL_BCB      0x00000400  /* blank on B/Cb dac output */
#define DSPCFG_DAC_BL_REFRGB   0x00000200  /* blank on RGB ref dac output */
#define DSPCFG_DAC_BL_SVM      0x00000100  /* blank on Svm dac output */
#define DSPCFG_DAC_SLP_RGBYCBCR 0x00000080 /* sleep mode on B/Cb dac output */
#define DSPCFG_DAC_SLP_RGBREF  0x00000040  /* sleep mode on RgbRef dac output */
#define DSPCFG_DAC_SLP_SVM     0x00000020  /* sleep mode on Svm dac output */
#define DSPCFG_DAC_BLK_RCR     0x00000010  /* black on R/Cr dac output */
#define DSPCFG_DAC_BLK_GY      0x00000008  /* black on G/Y  dac output */
#define DSPCFG_DAC_BLK_BCB     0x00000004  /* black on B/Cb dac output */
#define DSPCFG_DAC_BLK_REFRGB  0x00000002  /* black on RGB ref dac output */
#define DSPCFG_DAC_BLK_SVM     0x00000001  /* black on Svm dac output */

#if defined (ST_7710) || defined(ST_7100) || defined (ST_7109)
#define DHDO_ACFG_SIC          0x00000001 /* Synchronization in chroma. */
#elif !(defined (ST_7200)|| defined(ST_7111)|| defined (ST_7105))
#define DHDO_ACFG_SIC          0x00000002 /* Synchronization in chroma. */
#endif


/* Upsampler tap coefficients registers */
#if defined (ST_7710)
#ifdef STI7710_CUT2x
#define  COEFF_SET1_1  0x00E4
#define  COEFF_SET1_2  0x00E8
#define  COEFF_SET1_3  0x00EC
#define  COEFF_SET1_4  0x00F0
#define  COEFF_SET2_1  0x00F4
#define  COEFF_SET2_2  0x00F8
#define  COEFF_SET2_3  0x00FC
#define  COEFF_SET2_4  0x0100
#define  COEFF_SET3_1  0x0104
#define  COEFF_SET3_2  0x0108
#define  COEFF_SET3_3  0x010C
#define  COEFF_SET3_4  0x0110
#define  COEFF_SET4_1  0x0114
#define  COEFF_SET4_2  0x0118
#define  COEFF_SET4_3  0x011C
#define  COEFF_SET4_4  0x0120
#else /*STI7710_CUT2x*/
#define  COEFF_SET1_1  0x01c0
#define  COEFF_SET1_2  0x01c4
#define  COEFF_SET1_3  0x01c8
#define  COEFF_SET1_4  0x01cc
#define  COEFF_SET2_1  0x01d0
#define  COEFF_SET2_2  0x01d4
#define  COEFF_SET2_3  0x01d8
#define  COEFF_SET2_4  0x01dc
#define  COEFF_SET3_1  0x01e0
#define  COEFF_SET3_2  0x01e4
#define  COEFF_SET3_3  0x01e8
#define  COEFF_SET3_4  0x01ec
#define  COEFF_SET4_1  0x01f0
#define  COEFF_SET4_2  0x01f4
#define  COEFF_SET4_3  0x01f8
#define  COEFF_SET4_4  0x01fc
#endif  /*STI7710_CUT2x*/
#else /*ST_7710*/
#define  COEFF_SET1_1  0x00E4
#define  COEFF_SET1_2  0x00E8
#define  COEFF_SET1_3  0x00EC
#define  COEFF_SET1_4  0x00F0
#define  COEFF_SET2_1  0x00F4
#define  COEFF_SET2_2  0x00F8
#define  COEFF_SET2_3  0x00FC
#define  COEFF_SET2_4  0x0100
#define  COEFF_SET3_1  0x0104
#define  COEFF_SET3_2  0x0108
#define  COEFF_SET3_3  0x010C
#define  COEFF_SET3_4  0x0110
#define  COEFF_SET4_1  0x0114
#define  COEFF_SET4_2  0x0118
#define  COEFF_SET4_3  0x011C
#define  COEFF_SET4_4  0x0120
#endif  /*ST_7710*/

/*  SD Coefficient */
#define  SD_COEFF_SET1_1  0x3F9013FF
#define  SD_COEFF_SET1_2  0x011FC40B
#define  SD_COEFF_SET1_3  0x3F1045ED
#define  SD_COEFF_SET1_4  0x004FE40B
#define  SD_COEFF_SET2_1  0x3ED027FC
#define  SD_COEFF_SET2_2  0x0A1EF824
#define  SD_COEFF_SET2_3  0x01DEDDC0
#define  SD_COEFF_SET2_4  0x3FF013F4
#define  SD_COEFF_SET3_1  0x3E902BFC
#define  SD_COEFF_SET3_2  0x143E802E
#define  SD_COEFF_SET3_3  0x02EE8143
#define  SD_COEFF_SET3_4  0x3FC02BE9
#define  SD_COEFF_SET4_1  0x3F4013FF
#define  SD_COEFF_SET4_2  0x1C0EDC1D
#define  SD_COEFF_SET4_3  0x024EF8A1
#define  SD_COEFF_SET4_4  0x3FC027ED

/*  ED Coefficient */
#define  ED_COEFF_SET1_1  0x3EE02C06
#define  ED_COEFF_SET1_2  0x04AF7018
#define  ED_COEFF_SET1_3  0x01CF2DFB
#define  ED_COEFF_SET1_4  0x3E2053EB
#define  ED_COEFF_SET2_1  0x3D208FFF
#define  ED_COEFF_SET2_2  0x0F4E803E
#define  ED_COEFF_SET2_3  0x03EE558F
#define  ED_COEFF_SET2_4  0x3E108FD4
#define  ED_COEFF_SET3_1  0x3D408FE1
#define  ED_COEFF_SET3_2  0x18FE543E
#define  ED_COEFF_SET3_3  0x03EE80F4
#define  ED_COEFF_SET3_4  0x3FF08FD2
#define  ED_COEFF_SET4_1  0x3EB053E2
#define  ED_COEFF_SET4_2  0x1FBF2C1C
#define  ED_COEFF_SET4_3  0x018F704A
#define  ED_COEFF_SET4_4  0x00602FEE

/*  HD Coefficient */
#if 1 /*yxl 2007-10-05 temp modify for test,below is org*/
/*f10061*/

#define  HD_COEFF_SET1_1  0x3EC027FC
#define  HD_COEFF_SET1_2  0x056F0823
#define  HD_COEFF_SET1_3  0x01DEDD0C
#define  HD_COEFF_SET1_4  0x3FC027EF
#define  HD_COEFF_SET2_1  0x3EF027FC
#define  HD_COEFF_SET2_2  0x10CEDC1D
#define  HD_COEFF_SET2_3  0x023F0856
#define  HD_COEFF_SET2_4  0x3FC027EC
#define  HD_COEFF_SET3_1  0x00000000
#define  HD_COEFF_SET3_2  0x00000000
#define  HD_COEFF_SET3_3  0x00000000
#define  HD_COEFF_SET3_4  0x00000000
#define  HD_COEFF_SET4_1  0x00000000
#define  HD_COEFF_SET4_2  0x00000000
#define  HD_COEFF_SET4_3  0x00000000
#define  HD_COEFF_SET4_4  0x00000000

#else
/*f10062
#define  HD_COEFF_SET1_1  0x3F1023FD
#define  HD_COEFF_SET1_2  0x052F5419
#define  HD_COEFF_SET1_3  0x01CF20F3
#define  HD_COEFF_SET1_4  0x3FC027EE
#define  HD_COEFF_SET2_1  0x3EE027FC
#define  HD_COEFF_SET2_2  0x0F3F201C
#define  HD_COEFF_SET2_3  0x019F5452
#define  HD_COEFF_SET2_4  0x3FD023F1
#define  HD_COEFF_SET3_1  0x00000000
#define  HD_COEFF_SET3_2  0x00000000
#define  HD_COEFF_SET3_3  0x00000000
#define  HD_COEFF_SET3_4  0x00000000
#define  HD_COEFF_SET4_1  0x00000000
#define  HD_COEFF_SET4_2  0x00000000
#define  HD_COEFF_SET4_3  0x00000000
#define  HD_COEFF_SET4_4  0x00000000
*/

/*f10063
#define  HD_COEFF_SET1_1  0x3F201FFD
#define  HD_COEFF_SET1_2  0x04FF5C19
#define  HD_COEFF_SET1_3  0x01DF28EF
#define  HD_COEFF_SET1_4  0x3FC027F0
#define  HD_COEFF_SET2_1  0x3F0027FC
#define  HD_COEFF_SET2_2  0x0EFF281D
#define  HD_COEFF_SET2_3  0x019F5C4F
#define  HD_COEFF_SET2_4  0x3FD01FF2
#define  HD_COEFF_SET3_1  0x00000000
#define  HD_COEFF_SET3_2  0x00000000
#define  HD_COEFF_SET3_3  0x00000000
#define  HD_COEFF_SET3_4  0x00000000
#define  HD_COEFF_SET4_1  0x00000000
#define  HD_COEFF_SET4_2  0x00000000
#define  HD_COEFF_SET4_3  0x00000000
#define  HD_COEFF_SET4_4  0x00000000
*/


/*f10064
#define  HD_COEFF_SET1_1  0x3F8013FE
#define  HD_COEFF_SET1_2  0x049F8810
#define  HD_COEFF_SET1_3  0x019F38ED
#define  HD_COEFF_SET1_4  0x3FD01FF3
#define  HD_COEFF_SET2_1  0x3F301FFD
#define  HD_COEFF_SET2_2  0x0EDF3819
#define  HD_COEFF_SET2_3  0x010F8849
#define  HD_COEFF_SET2_4  0x3FE013F8
#define  HD_COEFF_SET3_1  0x00000000
#define  HD_COEFF_SET3_2  0x00000000
#define  HD_COEFF_SET3_3  0x00000000
#define  HD_COEFF_SET3_4  0x00000000
#define  HD_COEFF_SET4_1  0x00000000
#define  HD_COEFF_SET4_2  0x00000000
#define  HD_COEFF_SET4_3  0x00000000
#define  HD_COEFF_SET4_4  0x00000000
*/


#endif



/*  HDRQ (HD Reduced Quality Coefficient */
#define  HDRQ_COEFF_SET1_1  0x03405805
#define  HDRQ_COEFF_SET1_2  0x0A32185D
#define  HDRQ_COEFF_SET1_3  0x07225CAA
#define  HDRQ_COEFF_SET1_4  0x00C08C48
#define  HDRQ_COEFF_SET2_1  0x04808C0C
#define  HDRQ_COEFF_SET2_2  0x0AA25C72
#define  HDRQ_COEFF_SET2_3  0x05D218A3
#define  HDRQ_COEFF_SET2_4  0x00505834
#define  HDRQ_COEFF_SET3_1  0x00000000
#define  HDRQ_COEFF_SET3_2  0x00000000
#define  HDRQ_COEFF_SET3_3  0x00000000
#define  HDRQ_COEFF_SET3_4  0x00000000
#define  HDRQ_COEFF_SET4_1  0x00000000
#define  HDRQ_COEFF_SET4_2  0x00000000
#define  HDRQ_COEFF_SET4_3  0x00000000
#define  HDRQ_COEFF_SET4_4  0x00000000

#if defined (ST_7200)|| defined (ST_7111)|| defined (ST_7105)
#define ANA_CFG                 0x00
#define HDFORM_ANA_YC_SHIFT     0x0004
#define ANA_SCALE_CTRL_Y        0x08
#define ANA_SCALE_CTRL_C        0x0C
#define ANA_ANCILLARY_CTRL      0x10
#define ANA_SRC_CONFIG          0x14
#define AWG_INST_ANC            0x200
#define AWG_INST_SYNC           0x300

#define ANA_SEL_MAIN_YCBCR_INPUT   0x00
#define ANA_SEL_MAIN_RGB_INPUT     0x01
#define ANA_SEL_AUX_INPUT          0x02

#define ANA_RECORDER_CTRL_R_SEL1   (1<<2)
#define ANA_RECORDER_CTRL_R_SEL2   (2<<2)
#define ANA_RECORDER_CTRL_R_SEL3   (3<<2)

#define ANA_RECORDER_CTRL_G_SEL1   (1<<4)
#define ANA_RECORDER_CTRL_G_SEL2   (2<<4)
#define ANA_RECORDER_CTRL_G_SEL3   (3<<4)

#define ANA_RECORDER_CTRL_B_SEL1   (1<<6)
#define ANA_RECORDER_CTRL_B_SEL2   (2<<6)
#define ANA_RECORDER_CTRL_B_SEL3   (3<<6)

#define ANA_CLIP_EN                (1<<8)
#define ANA_CLIP_RB                (1<<9)
#define ANA_PREFILTER_EN           (1<<10)
#define ANA_SYNC_ON_PBPR           (1<<11)
#define SEL_INPUT_MAIN_YCBCR       (1<<12)

#define PBPR_SYNC_OFFSET_ED_576P   (0x1F5<<16)
#define PBPR_SYNC_OFFSET_ED_480P   (0x1FF<<16)
#define PBPR_SYNC_OFFSET_HD_1080I  (0x1F3<<16)
#define PBPR_SYNC_OFFSET_HD_720P   (0x1F7<<16)

#define AWG_ASYNC_EN               (1<<27)
#define AWG_ASYNC_HSYNC_NO_EFFECT  (1<<28)
#define AWG_ASYNC_VSYNC_FIELD      (1<<29)
#define AWG_ASYNC_FILTER_MODE_1    (1<<30)
#define AWG_ASYNC_FILTER_MODE_2    (2<<30)
#define AWG_ASYNC_FILTER_MODE_3    (3<<30)

#define Y_SCALE_COEFF_ED            0x275
#define Y_OFFESET_ED                (0xBF<<16)
#define Y_SCALE_COEFF_HD            0x275
#define Y_OFFESET_HD                (0xC3<<16)

#define C_SCALE_COEFF_ED_576P       0x267
#define C_SCALE_COEFF_ED_480P       0x283
#define C_OFFESET_ED                (0xBF<<16)

#define C_SCALE_COEFF_HD_1080I      0x267
#define C_SCALE_COEFF_HD_720P       0x26C
#define C_OFFESET_HD                (0xC3<<16)

#define SRC_UPSAMPLE_CONF_BY_2      0x0
#define SRC_UPSAMPLE_CONF_BY_4      0x1
#define SRC_UPSAMPLE_CONF_BY_8      0x2
#define SRC_NO_UPSAMPLE             0x4

#define SRC_GAIN_FACTOR_BY_256      0x00
#define SRC_GAIN_FACTOR_BY_512      (1<<2)
#define SRC_GAIN_FACTOR_BY_1024     (2<<2)
#define SRC_GAIN_FACTOR_BY_2048     (3<<2)

#define COEFF_PHASE1_T7_SD          (0x01050001)
#define COEFF_PHASE1_T7_HD          (0x01130000)
#define COEFF_PHASE1_T7_HDRQ        (0x008B0000)

#define COEFF_PHASE1_TAP123         0x18
#define COEFF_PHASE1_TAP456         0x1C
#define COEFF_PHASE2_TAP123         0x20
#define COEFF_PHASE2_TAP456         0x24
#define COEFF_PHASE3_TAP123         0x28
#define COEFF_PHASE3_TAP456         0x2C
#define COEFF_PHASE4_TAP123         0x30
#define COEFF_PHASE4_TAP456         0x34

#define HDFORM_COEFF_PHASE1_TAP123  0x00FE83FB
#define HDFORM_COEFF_PHASE1_TAP456  0x1F900401
#define HDFORM_COEFF_PHASE2_TAP123  0x00000000
#define HDFORM_COEFF_PHASE2_TAP456  0x00000000
#define HDFORM_COEFF_PHASE3_TAP123  0x00F408F9
#define HDFORM_COEFF_PHASE3_TAP456  0x055F7C25
#define HDFORM_COEFF_PHASE4_TAP123  0x00000000
#define HDFORM_COEFF_PHASE4_TAP456  0x00000000

#define EDFORM_COEFF_PHASE1_TAP123  0x00FF8083
#define EDFORM_COEFF_PHASE1_TAP456  0x1FEFF803
#define EDFORM_COEFF_PHASE2_TAP123  0x00F80600
#define EDFORM_COEFF_PHASE2_TAP456  0x026FB218
#define EDFORM_COEFF_PHASE3_TAP123  0x00F508F5
#define EDFORM_COEFF_PHASE3_TAP456  0x055F8622
#define EDFORM_COEFF_PHASE4_TAP123  0x00F80773
#define EDFORM_COEFF_PHASE4_TAP456  0x07879819

#define HDRQFORM_COEFF_PHASE1_TAP123  0x00FD7BFD
#define HDRQFORM_COEFF_PHASE1_TAP456  0x03A88613
#define HDRQFORM_COEFF_PHASE2_TAP123  0x00000000
#define HDRQFORM_COEFF_PHASE2_TAP456  0x00000000
#define HDRQFORM_COEFF_PHASE3_TAP123  0x0001FBFA
#define HDRQFORM_COEFF_PHASE3_TAP456  0x0428BC29
#define HDRQFORM_COEFF_PHASE4_TAP123  0x00000000
#define HDRQFORM_COEFF_PHASE4_TAP456  0x00000000


#define ANA_ANC_MACRIVISION_EN      0x20
#define AWG_INST_ANC_VALUE_1080I    0x1147
#define AWG_INST_ANC_VALUE_720P     0x1d7c
#define AWG_INST_SYNC_VALUE         0x1a2c

#endif

/* Exported Variables ------------------------------------------------------- */

/* Exported Macros ---------------------------------------------------------- */
#define stvout_rd32(a)           STSYS_ReadRegDev32LE((void*)(a))
#define stvout_wr32(a,v)         STSYS_WriteRegDev32LE((void*)(a),(v))

/* Exported Functions ------------------------------------------------------- */

#ifdef __cplusplus
}
#endif

#endif  /* #ifndef __HD_REG_H */

/* End of gam_reg.h */


