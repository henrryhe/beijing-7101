/*****************************************************************************

File name   : vfe_reg.h

Description : VTG register addresses for Video Front End.

COPYRIGHT (C) ST-Microelectronics 2001.

Date               Modification                                     Name
----               ------------                                     ----
02 Jul 2001        Created                                          HSdLM
*****************************************************************************/

#ifndef __VTG_VFE_REG_H
#define __VTG_VFE_REG_H

/* Includes --------------------------------------------------------------- */

/* Base addresses */

#ifdef __cplusplus
extern "C" {
#endif

/* Exported Types --------------------------------------------------------- */

/* Exported Constants ----------------------------------------------------- */

/* Register offsets */

#define VTG_VFE_MOD                 0x24        /* VTG Mode */
#define VTG_VFE_CLKLN               0x28        /* Line length (Pixclk) */
#define VTG_VFE_H_HD                0x2C        /* HSYNC signal rising and falling edge (Pixclk) */
#define VTG_VFE_TOP_V_VD            0x30        /* VSYNC signal rising and falling edge, Top field (lines) */
#define VTG_VFE_BOT_V_VD            0x34        /* VSYNC signal rising and falling edge, Bottom field (lines)*/
#define VTG_VFE_TOP_V_HD            0x38        /* VSYNC signal rising and falling edge, Top field (Pixclk) */
#define VTG_VFE_BOT_V_HD            0x3C        /* VSYNC signal rising and falling edge, Bottom field (Pixclk)*/
#define VTG_VFE_HLFLN               0x40        /* Half-Lines per Field*/
#define VTG_VFE_SRST                0x48        /* VTG Soft Reset */
#define VTG_VFE_ITM                 0x98        /* Interrupt Mask */
#define VTG_VFE_ITS                 0x9C        /* Interrupt Status */
#define VTG_VFE_STA                 0xA0        /* Status register */

#if defined(ST_5105)|| defined(ST_5188)|| defined(ST_5525)|| defined(ST_5107)|| defined(ST_5162)
#define VTG_VFE_H_HD_2                0x5C        /* HSYNC2 signal rising and falling edge (Pixclk) */
#define VTG_VFE_TOP_V_VD_2            0x60        /* VSYNC2 signal rising and falling edge, Top field (lines) */
#define VTG_VFE_BOT_V_VD_2            0x64        /* VSYNC2 signal rising and falling edge, Bottom field (lines)*/
#define VTG_VFE_TOP_V_HD_2            0x68        /* VSYNC2 signal rising and falling edge, Top field (Pixclk) */
#define VTG_VFE_BOT_V_HD_2            0x6C        /* VSYNC2 signal rising and falling edge, Bottom field (Pixclk)*/
#define VTG_VFE_H_HD_2_HS_PAL         0x15e        /* HSYNC2 signal rising and falling edge (Pixclk) */
#define VTG_VFE_H_HD_2_HO_PAL         0x100        /* HSYNC2 signal rising and falling edge (Pixclk) */
#define VTG_VFE_TOP_V_VD_2_VS_PAL     0x18        /* VSYNC2 signal rising and falling edge, Top field (lines) */
#define VTG_VFE_TOP_V_VD_2_VO_PAL     0x06        /* VSYNC2 signal rising and falling edge, Top field (lines) */
#define VTG_VFE_BOT_V_VD_2_VS_PAL     0x18        /* VSYNC2 signal rising and falling edge, Top field (lines) */
#define VTG_VFE_BOT_V_VD_2_VO_PAL     0x06        /* VSYNC2 signal rising and falling edge, Top field (lines) */
#endif

#if defined(ST_5162)
#define VTG_VFE_H_HD_3                0x7c
#define VTG_VFE_TOP_V_VD_3            0x80
#define VTG_VFE_BOT_V_VD_3            0x84
#define VTG_VFE_TOP_V_HD_3            0x88
#define VTG_VFE_BOT_V_HD_3            0x8c
#define VTG_DRST                      0x48
#define VTG_LN_STAT                   0xa4
#define VTG_LN_INTERRUPT              0xa8
#define VTG_LNVST                     0x00000080
#define VTG_LNVSB                     0x00000040
#endif

#define VTG_VFE_MOD_MASK            0x0000000B
#define VTG_VFE_CLKLN_MASK          0x00001FFF
#define VTG_VFE_H_HDO_MASK          0x00001FFF
#define VTG_VFE_H_HDS_MASK          0x1FFF0000
#define VTG_VFE_TOP_V_VD0_MASK      0x000007FF
#define VTG_VFE_TOP_V_VDS_MASK      0x07FF0000
#define VTG_VFE_BOT_V_VD0_MASK      0x000007FF
#define VTG_VFE_BOT_V_VDS_MASK      0x07FF0000
#define VTG_VFE_TOP_V_HD0_MASK      0x000007FF
#define VTG_VFE_TOP_V_HDS_MASK      0x07FF0000
#define VTG_VFE_BOT_V_HD0_MASK      0x000007FF
#define VTG_VFE_BOT_V_HDS_MASK      0x07FF0000
#define VTG_VFE_HLFLN_MASK          0x00000FFF

#define VTG_VFE_MOD_MODE_MASTER     0x00000000
#define VTG_VFE_MOD_MODE_SLAVE_EXT0 0x00000001
#define VTG_VFE_MOD_MODE_SLAVE_EXT1 0x00000002
#define VTG_VFE_MOD_BLIT_CTRL       0x00000008 /* GX1 only */
#define VTG_VFE_MOD_DISABLE         0x00000200 /* but GX1 */

/* VTG interrupts */
#define VTG_VFE_IT_MASK             0x00000018
#define VTG_VFE_IT_VSB              0x00000008
#define VTG_VFE_IT_VST              0x00000010

#ifdef __cplusplus
}
#endif

#endif /* __VTG_VFE_REG_H */
/* ------------------------------- End of file ---------------------------- */


