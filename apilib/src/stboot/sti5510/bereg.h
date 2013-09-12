/*******************************************************************************

File name : bereg.h

Description : Back end init used registers

COPYRIGHT (C) STMicroelectronics 1999.

Date               Modification                 Name
----               ------------                 ----
27 Sept 99         Created                      HG
*******************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __BACKEND_REGISTERS_H
#define __BACKEND_REGISTERS_H


/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */

/* NOTE: CFG registers are relative to VIDEO_BASE_ADDRESS */
    
/* CFG - configuration registers : SDRAM, clocks, interface   --------------- */
#define CFG_MCF               0x00   /* Memory refresh interval [6:0] */
#define CFG_CCF               0x01   /* Clock configuration [7:0] */
#define CFG_DRC               0x38   /* SDRAM configuration [5:0] phases, requests to LMC */

/* CAUTION: Register CFG_GCF is also used in STVID and STAUD. Therefore, access to it should be protected. */
#define CFG_GCF               0x3A   /* Chip general configuration [7:0] */

/* CFG_MCF - Memory refresh interval  (7 bits) ------------------------------ */
#define CFG_MCF_RFI_MASK      0x7F /* memory refresh interval mask */

/* CFG_CCF - Chip Configuration (8-bit) ------------------------------------- */
#define CFG_CCF_EAI           0x80 /* Enable Audio Interface */
#define CFG_CCF_EOU           0x40 /* Enable Ovf/Udf errors */
#define CFG_CCF_PBO           0x20 /* Prevent Bitbuffer Overflow */
#define CFG_CCF_EC3           0x10 /* Enable Clock 3 */
#define CFG_CCF_EC2           0x08 /* Enable Clock 2 */
#define CFG_CCF_ECK           0x04 /* Enable Clock1 that gives ck2 and ck3 */
#define CFG_CCF_EDI           0x02 /* Enable SDRAM Interface ( pads ) */
#define CFG_CCF_EVI           0x01 /* Enable Video Interface (internal bus) */

/* CFG_GCF - General Configuration (8-bit) ---------------------------------- */
#define CFG_GCF_PXD           0x80 /* Add one pixel delay */
#define CFG_GCF_A3RQ          0x40 /* External AC3 request polarity */
#define CFG_GCF_A3DI          0x20 /* External AC3 data strobe mode */
#define CFG_GCF_EXT_INPUT     0x00 /* Ext strobes selected */
#define CFG_GCF_SP_INPUT      0x10 /* SP data input to CDR */
#define CFG_GCF_AUD_INPUT     0x08 /* Audio data input to CDR */
#define CFG_GCF_VID_INPUT     0x18 /* Video data input to CDR */
#define CFG_GCF_SCK           0x04 /* Audio Strobe Clock select */
#define CFG_GCF_A3M           0x02 /* Select AC3 decoder/Not Mpeg */
#define CFG_GCF_ACS           0x01 /* has to be reset !!! */

/* CFG_DRC - DRAM Configuration (8-bit) ------------------------------------- */
#define CFG_DRC_M64           0x80 /* Memory format defined for 5512*/
#define CFG_DRC_MRS           0x20 /* Mode register set */
#define CFG_DRC_P_MASK        0xF3 /* Clk3 to MemClk phase mask */
#define CFG_DRC_PH0           0x00 /* Phase 0 */
#define CFG_DRC_PH1           0x04 /* Phase 1 */
#define CFG_DRC_PH2           0x08 /* Phase 2 */
#define CFG_DRC_PH3           0x0C /* Phase 3 */
#define CFG_DRC_ERQ           0x02 /* Enable requests to LMC */
#define CFG_DRC_SDR           0x01 /* synchronous SDRAM mode */

/* Base address for clock gen decoder  -------------------------------------- */

#ifndef CKG_BASE_ADDRESS
#define CKG_BASE_ADDRESS      0x00000000
#endif

/* CKG - clock gen registers ------------------------------------------------ */
#define CKG_CFG               0x31 /* Clocks generator configuration of clock pins */
#define CKG_MCK3              0xD8 /* clock divider [30:24] for MEM clock */
#define CKG_MCK2              0xD9 /* clock divider [22:16] for MEM clock */
#define CKG_MCK1              0xDA /* clock divider [14:8]  for MEM clock */
#define CKG_MCK0              0xDB /* clock divider [6:0]   for MEM clock */
#define CKG_AUX3              0xDC /* clock divider [29:24] for AUX clock */
#define CKG_AUX2              0xDD /* clock divider [22:16] for AUX clock */
#define CKG_AUX1              0xDE /* clock divider [14:8]  for AUX clock */
#define CKG_AUX0              0xDF /* clock divider [6:0]   for AUX clock */

/* CAUTION: Register CKG_PLL is also used in STAUD. Therefore, access to it should be protected. */
#define CKG_PLL               0x30 /* Clock generator, control input and vco freq */

/* CAUTION: Register CKG_PCM is also used in STAUD. Therefore, access to it should be protected. */
#define CKG_PCM3              0xD0 /* clock divider [29:24] for PCM clock */
#define CKG_PCM2              0xD1 /* clock divider [22:16] for PCM clock */
#define CKG_PCM1              0xD2 /* clock divider [14:8]  for PCM clock */
#define CKG_PCM0              0xD3 /* clock divider [6:0]   for PCM clock */

/* CKG - Ckg registers specific 32 bit calls -------------------------------- */
#define CKG_MCK32             0xD8 /* clock divider [30:0 ] for MEM clock */
#define CKG_AUX32             0xDC /* clock divider [29:0 ] for AUX clock */

/* CAUTION: Register CKG_PCM is also used in STAUD. Therefore, access to it should be protected. */
#define CKG_PCM32             0xD0 /* clock divider [29:0 ] for PCM clock */

/* CKG_PLL - Clock Generator PLL Parameters (8 bits ) ----------------------- */
#define CKG_PLL_M_MASK        0xF0 /* PLL Multiplication Factor mask */
#define CKG_PLL_N             0x10 /* Divide-by-N ( 1 or 2 ) */
#define CKG_PLL_REF_MASK      0x9F /* Clock Generator Reference Mask */
#define CKG_PLL_REF_DOWN      0x00 /* Down mode */
#define CKG_PLL_REF_PCM       0x20 /* PCM clock used as reference */
#define CKG_PLL_REF_PXC       0x60 /* PIX clock used as reference */
#define CKG_PLL_PDM           0x80 /* Charge pump forced to down */

/* CKG_CFG - Clock Generator Configuration ( 7 bits ) ----------------------- */
#define CKG_CFG_MEM_EXT       0x40 /* MEMCLK is inputed externally */
#define CKG_CFG_MEM_INT       0x00 /* MEMCLK is generated internally */
#define CKG_CFG_PCM_EXT       0x00 /* PCMCLK is inputed externally */
#define CKG_CFG_PCM_INT       0x20 /* PCMCLK is generated internally */
#define CKG_CFG_AUX_INT       0x00 /* AUXCLK is generated internally */
#define CKG_CFG_AUX_VID       0x04 /* output video clock ???? - test */
#define CKG_CFG_AUX_AUD       0x08 /* output audio clock ???? - test */
#define CKG_CFG_AUX_PLL       0x0C /* output int ref clock on phase comp */

/* CKG_MCK, CKG_PCM, CKG_AUX - control bits inside frac div cell ------------ */
#define CKG_XXX_DIV           0x40000000 /* Use fractional divider ( mem clock )*/
#define CKG_XXX_ENA           0x20000000 /* Frac divider enabled */
#define CKG_XXX_DV2           0x10000000 /* Divide by 2 the o/p of frac divider */


/* MPEG control registers */
#define MPEG_CONTROL_0 0x00005000
#define MPEG_CONTROL_1 0x00005001


/* MPEG_CONTROL_0 - Control traffic to CD FIFO's (8-bit) */
#define MPEG_CTRL0_RESET_VALUE 0x00

/* MPEG_CONTROL_1 - Control register accesses and CD FIFO accesses speeds (8-bit) */
#define MPEG_CTRL1_RESET_VALUE 0x00
#define MPEG_CTRL1_REG_1_EXTRA 0x10
#define MPEG_CTRL1_REG_2_EXTRA 0x20
#define MPEG_CTRL1_REG_3_EXTRA 0x30



/* VID - video header search, decoding, display  registers */
#define VID_OUT         0x90  /* Video output gain [3:0] */
#define VID_LCK         0x7B  /* Registers protection    */
#define VID_DIS         0xD4  /* CPU Priority difinition */

/* VID_OUT - Control 4:2:2 output display (8-bit) */
#define VID_OUT_NOS 0x08
#define VID_OUT_SPO 0x04
#define VID_OUT_LAY 0x03

/* VID_LCK - CFG_DRC, CFG_MCF, CFG_CCF protection */
#define VID_LCK_DIS 0x00
#define VID_LCK_ENA 0x01

/* VID_DIS - CPU Priority 4 bits (8 bits reg)     */
#define VID_DIS_HLH 0x48

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __BACKEND_REGISTERS_H */

/* End of bereg.h */
