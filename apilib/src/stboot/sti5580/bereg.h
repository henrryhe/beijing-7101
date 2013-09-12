/*******************************************************************************

File name : bereg.h

Description : Back end init used registers

COPYRIGHT (C) STMicroelectronics 1999.

Date               Modification                 Name
----               ------------                 ----
Jan 2000           Created for the 5508	
29/01/01           Updated for audio mmdsp
*******************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __BACKEND_REGISTERS_H
#define __BACKEND_REGISTERS_H

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------- */
/* Clock Generator registers */
/* ------------------------- */
/* CKG - clock gen registers ------------------------------------------------ */
#define CKG_RLOCK             0xCF /* Clock lock configuration */
#define CKG_CNT_AUD           0xD0 /* Audio clock controller register */
#define CKG_CNT_MCK           0xD4 /* Memory clock controller register */
#define CKG_DIV_MCK           0xD5 /* Memory clock divider register */
#define CKG_KARA1             0xDA /* Karaoke clock enable */
#define CKG_KARA2             0xDB /* Karaoke clock divider */
#define CKG_CNT_DENC          0xDC /* Denc clock controller register */
#define CKG_CNT_ST20          0xDD /* ST20/FEI/LINK clock controller register */
#define CKG_DIV_ST20          0xDE /* ST20/FEI/LINK divider register */
#define CKG_SFREQSYS_SDIV     0xDF /* M = Pre divider ratio setup of the PLL */
#define CKG_SFREQSYS_PE0      0xE0 /* N = Feed back divider ratio setup of the PLL */
#define CKG_SFREQSYS_PE1      0xE1 /* bits[7:5] : P = Post divider ratio setup of the PLL
                                      bits[4:2] : lock detector threshold */
#define CKG_SFREQSYS_MD       0xE2 /* bits[7:6] : Start up control
                                      bits[5:2] : Charge pump control */
#define CKG_CNT_IDDQPAD       0xE3 /* Clocks generator configuration of clock pins */
#define CKG_SFREQPCM_DIV      0xE4 
#define CKG_SFREQPCM_PE0      0xE5 
#define CKG_SFREQPCM_PE1      0xE6 
#define CKG_SFREQPCM_MD       0xE7 
#define CKG_SFREQPCM_CNT      0xE8 

/* Audio clock frequency synthesizer (offsets) */
#define CKG_SFREQAUD_SDIV     0xE9
#define CKG_SFREQAUD_PE0      0xEA
#define CKG_SFREQAUD_PE1      0xEB
#define CKG_SFREQAUD_MD       0xEC
#define CKG_SFREQAUD_CNT      0xED

/* CKG_CFG - Clock Generator Configuration ( 4 bits ) ----------------------- */
#define CKG_CFG_PIX_CLK       0x08 /* Pixel clock pin is an input */
#define CKG_CFG_HSYNC         0x10 /* The alternate function (HSYNC) of PWM0 pin (pin114) is enabled */
#define CKG_CFG_VSYNC         0x20 /* The alternate function (VSYNC) of PWM1 pin (pin116) is enabled*/
#define CKG_CFG_MMDSP_TST_CLK 0x40 /* Force Test clock for the MMMDSP */

/* CKG_AUD, CKG_KARA, CKG_DENC - control bits ------------------------------- */
#define CKG_XXX_ENA           0x80 /* Divider enabled for CKG_AUD and CKG_KARA */
                                   /* Clock input is enabled for CKG_DENC */
#define CKG_XXX_DV2           0x40 /* Divide by 2 the clock input */
#define CKG_XXX_NOTBYP        0x20 /* not bypass the clock generator*/

/* ----------------------------------- */
/* Configuration and control registers */
/* ----------------------------------- */
/* CFG - configuration registers : SDRAM, clocks, interface   --------------- */
#define CFG_MCF               0x00   /* Memory refresh interval [6:0] */
#define CFG_CCF               0x01   /* Clock configuration [7:0] */
#define CFG_DRC               0x38   /* SDRAM configuration [5:0] phases, requests to LMC */

/* CAUTION: Register CFG_GCF is also used in STVID and STAUD. Therefore, access to it should be protected. */
#define CFG_GCF               0x3A   /* Chip general configuration [7:0] */

/* CFG_MCF - Memory refresh interval  (7 bits) ------------------------------ */
#define CFG_MCF_RFI_MASK      0x7F /* memory refresh interval mask */

/* CFG_CCF - Chip Configuration (8-bit) ------------------------------------- */
#define CFG_CCF_EOU           0x40 /* Enable Ovf/Udf errors */
#define CFG_CCF_PBO           0x20 /* Prevent Bitbuffer Overflow */
#define CFG_CCF_EC3           0x10 /* Enable Clock 3 */
#define CFG_CCF_EC2           0x08 /* Enable Clock 2 */
#define CFG_CCF_ECK           0x04 /* Enable Clock1 that gives ck2 and ck3 */
#define CFG_CCF_EDI           0x02 /* Enable SDRAM Interface ( pads ) */
#define CFG_CCF_EVI           0x01 /* Enable Video Interface (internal bus) */

/* CFG_GCF - General Configuration (8-bit) ---------------------------------- */
#define CFG_GCF_NOTTRST_SPDIF 0x80 /* SPDIF_OUT pin not tristate */
#define CFG_GCF_A3RQ          0x40 /* External AC3 request polarity */
#define CFG_GCF_A3DI          0x20 /* External AC3 data strobe mode */
#define CFG_GCF_EXT_INPUT     0x00 /* Ext strobes selected */
#define CFG_GCF_SP_INPUT      0x10 /* Sub picture data input to CDR */
#define CFG_GCF_AUD_INPUT     0x08 /* Audio data input to CDR */
#define CFG_GCF_VID_INPUT     0x18 /* Video data input to CDR */
#define CFG_GCF_SCK           0x04 /* Audio Strobe Clock select */
#define CFG_GCF_A3M           0x02 /* Select AC3 decoder/Not Mpeg */
#define CFG_GCF_ACS           0x01 /* has to be reset */

/* CFG_DRC - DRAM Configuration (8-bit) ------------------------------------- */
#define CFG_DRC_M64           0x80 /* SDRAM based on 64Mbits */
#define CFG_DRC_MRS           0x20 /* Mode register set */
#define CFG_DRC_P_MASK        0xF3 /* Clk3 to MemClk phase mask */
#define CFG_DRC_PH0           0x00 /* Phase 0 */
#define CFG_DRC_PH1           0x04 /* Phase 1 */
#define CFG_DRC_PH2           0x08 /* Phase 2 */
#define CFG_DRC_PH3           0x0C /* Phase 3 */
#define CFG_DRC_ERQ           0x02 /* Enable requests to LMC */
#define CFG_DRC_SDR           0x01 /* synchronous SDRAM mode */

/* ------------------------------- */
/* Frequency Synthesizer registers */
/* ------------------------------- */
/* FSY - Frequency synthesizer registers ---------------- */
#define FSY_SELREG            0xE8 /* Select the way to control the FSY */

/* FSY_SELREG - FSY control ------------------------------------- */
#define NOT_MMDSP_CONTROL	  0x80 /* The FSY is controled by the ST20, not the MMDSP */

/* -------------------------------------------------------------- */
/* Different initialisation values for the MPEG CONTROL registers */
/* -------------------------------------------------------------- */
/* MPEG_CONTROL_0 - Control traffic to CD FIFO's (8-bit) */
#define MPEG_CTRL0_RESET_VALUE 0x00
#define MPEG_CTRL0_REG_1_EXTRA 0x07

/* MPEG_CONTROL_1 - Control register accesses and CD FIFO accesses speeds (8-bit) */
#define MPEG_CTRL1_RESET_VALUE 0x00
#define MPEG_CTRL1_REG_1_EXTRA 0x70

/* ------------------------------------------------------ */
/* Video registers to manage the different display planes */
/* ------------------------------------------------------ */
/* VID - video header search, decoding, display  registers */
#define VID_OUT         0x90  /* Video output gain [3:0] */
#define VID_LCK         0x7B  /* Registers protection    */

/* VID_OUT - Control 4:2:2 output display (8-bit) */
#define VID_OUT_NOS     0x08
#define VID_OUT_SPO     0x04
#define VID_OUT_LAY     0x03

/* VID_LCK - CFG_DRC, CFG_MCF, CFG_CCF protection */
#define VID_LCK_DIS 	0x00
#define VID_LCK_ENA 	0x01

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __BACKEND_REGISTERS_H */
/* End of bereg.h */
