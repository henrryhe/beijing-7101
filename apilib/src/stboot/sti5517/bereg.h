/*******************************************************************************

File name : bereg.h

Description : Back end init used registers

COPYRIGHT (C) STMicroelectronics 2001.

Date               Modification                 Name
----               ------------                 ----
27 Sept 99         Created                      HG
09 July 2001       Modified for 5514            AdlF
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
#define CFG_CCF_EOU           0x40 /* Enable Ovf/Udf errors */
#define CFG_CCF_PBO           0x20 /* Prevent Bitbuffer Overflow */
#define CFG_CCF_EDI           0x02 /* Enable SDRAM Interface ( pads ) */
#define CFG_CCF_EVI           0x01 /* Enable Video Interface (internal bus) */

/* CFG_GCF - General Configuration (8-bit) ---------------------------------- */
#define CFG_GCF_PXD           0x80 /* Add one pixel delay */
#define CFG_GCF_EXT_INPUT     0x00 /* Ext strobes selected */
#define CFG_GCF_SP_INPUT      0x10 /* SP data input to CDR */
#define CFG_GCF_AUD_INPUT     0x08 /* Audio data input to CDR */
#define CFG_GCF_VID_INPUT     0x18 /* Video data input to CDR */

/* CFG_DRC - DRAM Configuration (8-bit) ------------------------------------- */
#define CFG_DRC_M64           0x80 /* Memory format defined for 5514*/
#define CFG_DRC_M128          0x40 /* Memory format defined for 5514*/
#define CFG_DRC_MRS           0x20 /* Mode register set */
#define CFG_DRC_P_MASK        0xF3 /* Clk3 to MemClk phase mask */
#define CFG_DRC_PH0           0x00 /* Phase 0 */
#define CFG_DRC_PH1           0x04 /* Phase 1 */
#define CFG_DRC_PH2           0x08 /* Phase 2 */
#define CFG_DRC_PH3           0x0C /* Phase 3 */
#define CFG_DRC_ERQ           0x02 /* Enable requests to LMC */
#define CFG_DRC_SDR           0x01 /* synchronous SDRAM mode */

/* Base address for clock gen decoder  -------------------------------------- */

/* CKG - clock gen registers ------------------------------------------------ */


/* MPEG control registers */

/* VID - video header search, decoding, display  registers */
#define VID_OUT         0x90  /* Video output gain [3:0] */
#define VID_LCK         0x7B  /* Registers protection    */
#define VID_DIS         0xD6  /* CPU Priority difinition */

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
