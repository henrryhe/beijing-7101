/*****************************************************************************

File name   : vos_reg.h

Description : VTG register addresses for STi7015/20.

COPYRIGHT (C) ST-Microelectronics 2002.

Date               Modification                                     Name
----               ------------                                     ----
08 Dec 1999        Created from output_reg.h                         FC
04 Jan 2000        Added xxx_BASE_ADDRESS defines                    FC
07 Apr 2000        Changed ITS/STA register address (hardware bug)   FC
18 Jan 2001        Changed ITS/STA register address (undo previous) HSdLM
20 Feb 2001        Add Clock Generator registers                    HSdLM
06 Mar 2001        Add DHDO and C656 registers                      HSdLM
28 Jun 2001        Rename vtg_reg.h in vos_reg.h, add some VOS_ in  HSdLM
 *                 register names to differentiate with VFE ones.
17 Apr 2002        Add comments for STi7020                         HSdLM
04 Jul 2002        Add SYNTH5 for 7020 ADISPCLK                     HSdLM
17 Sep 2002        Replace DHDO_MIDLOW by DHDO_MIDLOWL to match     HSdLM
 *                 datasheet. Replace SYNTH5 by SYNTH2 for
 *                 7020 cut 2.0 ADISPCLK
21 Jan 2003        Add PROG293M support                             BS
*****************************************************************************/

#ifndef __VTG_VOS_REG_H
#define __VTG_VOS_REG_H


#include "stdevice.h"


/* Includes --------------------------------------------------------------- */

/* Base addresses */
#if defined (ST_7015)||defined (ST_7020)
#define VTG1_OFFSET     0x00006000  /* same for STi7020 */
#define VTG2_OFFSET     0x00008000  /* same for STi7020 */
#elif defined (ST_7710)||defined (ST_7100)||defined (ST_7109)
#define VTG1_OFFSET               0x00
#define VTG2_OFFSET               0x34
#endif

#ifndef VTG1_BASE_ADDRESS
#if defined (ST_7200)
#define VTG1_BASE_ADDRESS  ST7200_VTG1_BASE_ADDRESS
#elif defined(ST_7111)
#define VTG1_BASE_ADDRESS  ST7111_VTG1_BASE_ADDRESS
#elif defined(ST_7105)
#define VTG1_BASE_ADDRESS  ST7105_VTG1_BASE_ADDRESS
#else
#define VTG1_BASE_ADDRESS  (VTG_BASE_ADDRESS + VTG1_OFFSET)
#endif
#endif
#ifndef VTG2_BASE_ADDRESS
#if defined (ST_7200)
#define VTG2_BASE_ADDRESS  ST7200_VTG2_BASE_ADDRESS
#elif defined(ST_7111)
#define VTG2_BASE_ADDRESS  ST7111_VTG2_BASE_ADDRESS
#elif defined(ST_7105)
#define VTG2_BASE_ADDRESS  ST7105_VTG2_BASE_ADDRESS
#else
#define VTG2_BASE_ADDRESS  (VTG_BASE_ADDRESS + VTG2_OFFSET)
#endif
#endif
#if defined (ST_7200)
#define VTG3_BASE_ADDRESS  ST7200_VTG3_BASE_ADDRESS
#elif defined(ST_7111)
#define VTG3_BASE_ADDRESS  ST7111_VTG3_BASE_ADDRESS
#elif defined(ST_7105)
#define VTG3_BASE_ADDRESS  ST7105_VTG3_BASE_ADDRESS
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Exported Types --------------------------------------------------------- */

/* Exported Constants ----------------------------------------------------- */

/* Register offsets */
#if defined (ST_7710)|| defined (ST_7100)|| defined (ST_7109)|| defined (ST_7200)  /** for 7710 regsisters added with cut 3.0 */
#if defined (ST_7200)|| defined (ST_7111)|| defined (ST_7105)
#define AWG_RAM_OFFSET          0x300
#define AWG_RAM_SIZE            50

#if defined(ST_7200)
#define AWG_BASE_ADDRESS  ST7200_HD_TVOUT_HDF_BASE_ADDRESS
#elif defined(ST_7111)
#define AWG_BASE_ADDRESS  ST7111_HD_TVOUT_HDF_BASE_ADDRESS
#elif defined(ST_7105)
#define AWG_BASE_ADDRESS  ST7105_HD_TVOUT_HDF_BASE_ADDRESS
#endif
#define AWG_CTRL 0x0
#define AWG_HDO  0x1b8
#define AWG_VDO  0x1bc
#else
#define AWG_RAM_OFFSET          0x100
#define AWG_RAM_SIZE            46
#if defined (ST_7710)
#define AWG_BASE_ADDRESS  0x20103c00
#define AWG_CTRL 0x34
#define AWG_HDO  0x1b8
#define AWG_VDO  0x1bc
#else/*7100-7109*/
#define AWG_BASE_ADDRESS tlm_to_virtual(0xb9004800)
#define AWG_CTRL 0x0
#define AWG_HDO  0x130
#define AWG_VDO  0x134
#endif
#endif
#define DAC_HDO  0xc0
#define DAC_HDS  0xc0
#define DAC_VDO  0xc0
#endif

#if defined (ST_7710) || defined (ST_7100)|| defined (ST_7109)|| defined (ST_7200)|| defined (ST_7111)|| defined (ST_7105)
#if defined (ST_7710) || defined (ST_7100)|| defined (ST_7109)
#define VTG_HVDRIVE                         0x00        /* H/Vsync output enable */
#define VTG_CLKLN                           0x04        /* Line length (Pixclk) */
#define VTG_VOS_HDO                         0x08        /* HSync start relative to Href (Pixclk) */
#define VTG_VOS_HDS                         0x0C        /* Hsync end relative to Href (Pixclk) */
#define VTG_HLFLN                           0x10        /* Half-Lines per Field*/
#define VTG_VOS_VDO                         0x14        /* Vertical Drive Output start relative to Vref (half lines) */
#define VTG_VOS_VDS                         0x18        /* Vertical Drive Output end relative to Vref (half lines) */
#define VTG_VTGMOD                          0x1C        /* VTG Mode */
#define VTG_VTIMER                          0x20        /* Vsync Timer (Cycles) */
#define VTG_DRST                            0x24        /* VTG Reset */
#define VTG_RG1                             0x28        /* Start of range which generates a bottom field (in slave mode) */
#define VTG_RG2                             0x2C        /* End of range which generates a bottom field (in slave mode) */

#ifdef WA_GNBvd35956
#define GAM_DVP_ITM                         0x98
#define GAM_DVP_ITS                         0x9c
#endif /* WA_GNBvd35956 */

#define VTG_VOS_ITM                         0x28        /* Interrupt Mask */
#define VTG_VOS_ITS                         0x2C        /* Interrupt Status */
#define VTG_STA                             0x30        /* Status register */
#define VTG_MOD_FI                          0x4        /*Enable Force interleaved MODE */

#elif defined (ST_7200)||defined(ST_7111)||defined(ST_7105)

#define VTG_VID_TFO      0x04
#define VTG_VID_TFS      0x08
#define VTG_VID_BFO      0x0c
#define VTG_VID_BFS      0x10

#define VTG_VTGMOD       0x24        /* VTG Mode */
#define VTG_CLKLN        0x28        /* Line length (Pixclk) */

#define VTG_H_HD1        0x2c
#define VTG_TOP_V_VD1    0x30
#define VTG_BOT_V_VD1    0x34
#define VTG_TOP_V_HD1    0x38
#define VTG_BOT_V_HD1    0x3c

#define VTG_H_HD2        0x5c
#define VTG_TOP_V_VD2    0x60
#define VTG_BOT_V_VD2    0x64
#define VTG_TOP_V_HD2    0x68
#define VTG_BOT_V_HD2    0x6c

#define VTG_H_HD3        0x7c
#define VTG_TOP_V_VD3    0x80
#define VTG_BOT_V_VD3    0x84
#define VTG_TOP_V_HD3    0x88
#define VTG_BOT_V_HD3    0x8c

#define VTG_HLFLN        0x40        /* Half-Lines per Field*/
#define VTG_DRST         0x48        /* VTG Reset */
#define VTG_VOS_ITM      0x98        /* Interrupt Mask */
#define VTG_VOS_ITS      0x9C        /* Interrupt Status */
#define VTG_STA          0xA0        /* Status register */
#define VTG_LNSTAT       0xA4
#define VTG_LN_INT       0xA8


#define VTG_VTIMER       VTG_LN_INT

#define VTG_MOD_MASK     0x3
#define VTG_MOD_DISABLE  0x200
#define VTG_ODDEVEN_NOT_VSYNC  0x10
#endif

#define DHDO_ACFG       0x007C                /* Analog output configuration */
#define DHDO_ABROAD     0x0080                /* Sync pulse width */
#define DHDO_BBROAD     0x0084                /* Start of broad pulse */
#define DHDO_CBROAD     0x0088                /* End of broad pulse */
#define DHDO_BACTIVE    0x008C                /* Start of Active video */
#define DHDO_CACTIVE    0x0090                /* End of Active video */
#define DHDO_BROADL     0x0094                /* Broad Length */
#define DHDO_BLANKL     0x0098                /* Blanking Length */
#define DHDO_ACTIVEL    0x009C                /* Active Length */
#define DHDO_ZEROL      0x00A0                /* Zero Level */
#define DHDO_MIDL       0x00A4                /* Sync pulse Mid Level */
#define DHDO_HIGHL      0x00A8                /* Sync pulse High Level */
#define DHDO_MIDLOWL    0x00AC                /* Sync pulse Mid-Low Level */
#define DHDO_LOWL       0x00B0                /* Broad pulse Low Level */
#define DHDO_YMLT       0x00B8                /* Luma multiplication factor */
#define DHDO_CMLT       0x00BC                /* Chroma multiplication factor */
#define DHDO_COFF       0x00C0                /* Chroma offset */
#define DHDO_YOFF       0x00C4                /* Luma offset */
#define C656_ACTL       0x00C8                /* Active Length in lines */
#define C656_BACT       0x00CC                /* Begin of active line in pixels */
#define C656_BLANKL     0x00D0                /* Blank Length in lines */
#define C656_EACT       0x00D4                /* End of active line in pixels */
#define C656_PAL        0x00D8                /* PAL/notNTSC Mode */
#define DSPCFG_ANA      0x78

#else
#define VTG_HVDRIVE     0x20                /* H/Vsync output enable */
#define VTG_CLKLN       0x28                /* Line length (Pixclk) */
#define VTG_VOS_HDO     0x2C                /* HSync start relative to Href (Pixclk) */
#define VTG_VOS_HDS     0x30                /* Hsync end relative to Href (Pixclk) */
#define VTG_HLFLN       0x34                /* Half-Lines per Field*/
#define VTG_VOS_VDO     0x38                /* Vertical Drive Output start relative to Vref (half lines) */
#define VTG_VOS_VDS     0x3C                /* Vertical Drive Output end relative to Vref (half lines) */
#define VTG_VTGMOD      0x40                /* VTG Mode */
#define VTG_VTIMER      0x44                /* Vsync Timer (Cycles) */
#define VTG_DRST        0x48                /* VTG Reset */
#define VTG_RG1         0x4C                /* Start of range which generates a bottom field (in slave mode) */
#define VTG_RG2         0x50                /* End of range which generates a bottom field (in slave mode) */
#define VTG_VOS_ITM     0x98                /* Interrupt Mask */
#define VTG_VOS_ITS     0x9C                /* Interrupt Status */
#define VTG_STA         0xA0                /* Status register */


#define CKG_SYN2_PAR    0x48                /* Frequency synthesizer 2 parameters       */
#define CKG_SYN3_PAR    0x4C                /* Frequency synthesizer 3 parameters       */
#define CKG_SYN6_PAR    0x58                /* Frequency synthesizer 6 parameters       */
#define CKG_SYN7_PAR    0x5C                /* Frequency synthesizer 7 parameters       */
#define CKG_SYN3_REF    0x70                /* Synthesizer 3 ref clock selection        */
#define CKG_SYN7_REF    0x74                /* Synthesizer 7 denc pixel clock selection */

#define DHDO_ACFG       0x00                /* Analog output configuration */
#define DHDO_ABROAD     0x04                /* Sync pulse width */
#define DHDO_BBROAD     0x08                /* Start of broad pulse */
#define DHDO_CBROAD     0x0c                /* End of broad pulse */
#define DHDO_BACTIVE    0x10                /* Start of Active video */
#define DHDO_CACTIVE    0x14                /* End of Active video */
#define DHDO_BROADL     0x18                /* Broad Length */
#define DHDO_BLANKL     0x1c                /* Blanking Length */
#define DHDO_ACTIVEL    0x20                /* Active Length */
#define DHDO_ZEROL      0x24                /* Zero Level */
#define DHDO_MIDL       0x28                /* Sync pulse Mid Level */
#define DHDO_HIGHL      0x2c                /* Sync pulse High Level */
#define DHDO_MIDLOWL    0x30                /* Sync pulse Mid-Low Level */
#define DHDO_LOWL       0x34                /* Broad pulse Low Level */
#define DHDO_YMLT       0x3C                /* Luma multiplication factor */
#define DHDO_CMLT       0x40                /* Chroma multiplication factor */
#define DHDO_YOFF       0x44                /* Luma offset */
#define DHDO_COFF       0x48                /* Chroma offset */
#define C656_BACT       0x00                /* Begin of active line in pixels */
#define C656_EACT       0x04                /* End of active line in pixels */
#define C656_BLANKL     0x08                /* Blank Length in lines */
#define C656_ACTL       0x0c                /* Active Length in lines */
#define C656_PAL        0x10                /* PAL/notNTSC Mode */
#endif

#define VTG2_IT_OFFSET                      0x8  /* Offset used to reach the VTG2_IT */

/* Register Bits and Masks --------------------------------------------------- */
#define VTG_VTGMOD_SLAVEMODE_MASK           0x1f
#define VTG_VTGMOD_SLAVE_MODE_INTERNAL      0x0
#define VTG_VTGMOD_SLAVE_MODE_EXTERNAL      0x1
#define VTG_VTGMOD_SLAVE_MODE_EXT_VREF      0x2
#define VTG_VTGMOD_EXT_VSYNC_BNOTT          0x4
#define VTG_VTGMOD_EXT_VSYNC_HIGH           0x8
#define VTG_VTGMOD_EXT_HSYNC_HIGH           0x10
#if defined(ST_7710) || defined(ST_7100)|| defined (ST_7109)|| defined (ST_7200)||defined(ST_7111)||defined(ST_7105)
#define VTG_VTGMOD_FORCE_INTERLACED         0x4
#else
#define VTG_VTGMOD_FORCE_INTERLACED         0x20
#endif
#define VTG_VTGMOD_HVSYNC_CMD_MASK          0x3C0
#define VTG_VTGMOD_VSYNC_PIN                0x00        /* for VTG1 */
#define VTG_VTGMOD_VSYNC_VTG1               0x00        /* for VTG2 */
#define VTG_VTGMOD_VSYNC_SDIN1              0x40
#define VTG_VTGMOD_VSYNC_SDIN2              0x80
#define VTG_VTGMOD_VSYNC_HDIN               0xC0
#define VTG_VTGMOD_HSYNC_PIN                0x00        /* for VTG1 */
#define VTG_VTGMOD_HSYNC_VTG1               0x00        /* for VTG2 */
#define VTG_VTGMOD_HSYNC_SDIN1              0x100
#define VTG_VTGMOD_HSYNC_SDIN2              0x200
#define VTG_VTGMOD_HSYNC_HDIN               0x300


#define VTG_VTGMOD_MODE_MASTER     0x00000000
#define VTG_VTGMOD_MODE_SLAVE_EXT0 0x00000001
#define VTG_VTGMOD_MODE_SLAVE_EXT1 0x00000002
#define VTG_VTGMOD_DISABLE         0x00000200

#define VTG_HVDRIVE_MASK                    0x3
#define VTG_HVDRIVE_VSYNC                   0x1
#define VTG_HVDRIVE_HSYNC                   0x2

#define DSPCFG_ANA_AWG                      0x3
/* VTG interrupts */
#if defined (ST_7710) || defined(ST_7100)|| defined (ST_7109)

#ifdef WA_GNBvd35956
#define GAM_DVP_IT_MASK                     0x18
#define GAM_DVP_IT_VSB                      0x08
#define GAM_DVP_IT_VST                      0x10
#endif /* WA_GNBvd35956 */

#define VTG_VOS_IT_MASK                     0x0f
#define VTG_IT_OFD                          0x01
#define VTG_VOS_IT_VSB                      0x02
#define VTG_VOS_IT_VST                      0x04
#define VTG_IT_PDVS                         0x08

#elif defined(ST_7200)||defined(ST_7111)||defined(ST_7105)

#define VTG_VOS_IT_MASK                     0xd8
#define VTG_IT_OFD                          0x0  /*unused*/
#define VTG_VOS_IT_VSB                      0x08
#define VTG_VOS_IT_VST                      0x10
#define VTG_VOS_IT_LNB                      0x40
#define VTG_VOS_IT_LNT                      0x80

#define VTG_VOS_CLKLN_MASK          0x00001FFF
#define VTG_VOS_H_HDO_MASK          0x00001FFF
#define VTG_VOS_H_HDS_MASK          0x1FFF0000
#define VTG_VOS_TOP_V_VD0_MASK      0x000007FF
#define VTG_VOS_TOP_V_VDS_MASK      0x07FF0000
#define VTG_VOS_BOT_V_VD0_MASK      0x000007FF
#define VTG_VOS_BOT_V_VDS_MASK      0x07FF0000
#define VTG_VOS_TOP_V_HD0_MASK      0x00001FFF
#define VTG_VOS_TOP_V_HDS_MASK      0x1FFF0000
#define VTG_VOS_BOT_V_HD0_MASK      0x00001FFF
#define VTG_VOS_BOT_V_HDS_MASK      0x1FFF0000
#define VTG_VOS_HLFLN_MASK          0x00000FFF

#else                                   /* 7015-7020 */
#define VTG_VOS_IT_MASK                     0x3f
#define VTG_IT_OFD                          0x04
#define VTG_VOS_IT_VSB                      0x08
#define VTG_VOS_IT_VST                      0x10
#define VTG_IT_PDVS                         0x20
#endif
#define VTG_IT_CIFD                         0x01
#define VTG_IT_YIFD                         0x02

/* CKG_SYNn_PAR - Frequency synthesizer parameters -----------------------------------------*/
#define CKG_SYN_PAR_PE_MASK                 0x0000ffff  /* Fine selector mask               */
#define CKG_SYN_PAR_MD_MASK                 0x001f0000  /* Coarse selector mask             */
#define CKG_SYN_PAR_SDIV_MASK               0x00e00000  /* Output divider mask              */
#define CKG_SYN_PAR_NDIV_MASK               0x03000000  /* Input divider mask               */
#define CKG_SYN_PAR_PARAMS_MASK             0x03ffffff  /* Or of the 4 preceding masks      */
#define CKG_SYN_PAR_SSL                     0x04000000  /* not Bypassed bit                 */
#define CKG_SYN_PAR_EN                      0x08000000  /* Enable bit                       */

#define DHDO_PROG295M_MASK                  0x01        /* SMPTE295 Progressive/notInterlace */
#define DHDO_PROG293M_MASK                  0x04        /* SMPTE293 Progressive/notInterlace */
#define DHDO_ABROAD_MASK                    0xfff       /* Sync pulse width */
#define DHDO_BBROAD_MASK                    0xfff       /* Start of broad pulse */
#define DHDO_CBROAD_MASK                    0xfff       /* End of broad pulse */
#define DHDO_BACTIVE_MASK                   0xfff       /* Start of Active video */
#define DHDO_CACTIVE_MASK                   0xfff       /* End of Active video */
#define DHDO_BROADL_MASK                    0x7ff       /* Broad Length */
#define DHDO_BLANKL_MASK                    0x7ff       /* Blanking Length */
#define DHDO_ACTIVEL_MASK                   0x7ff       /* Active Length */
#define DHDO_ZEROL_MASK                     0x03ff03ff  /* Zero Level */
#define DHDO_MIDL_MASK                      0x03ff03ff  /* Sync pulse Mid Level */
#define DHDO_HIGHL_MASK                     0x03ff03ff  /* Sync pulse High Level */
#define DHDO_MIDLOWL_MASK                   0x03ff03ff  /* Sync pulse Mid-Low Level */
#define DHDO_LOWL_MASK                      0x03ff03ff  /* Broad pulse Low Level */


#ifdef __cplusplus
}
#endif

#endif /* __VTG_VOS_REG_H */
/* ------------------------------- End of file ---------------------------- */


