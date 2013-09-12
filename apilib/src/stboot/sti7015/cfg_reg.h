/*****************************************************************************

File name   : cfg_reg.h

Description : Configuration register addresses

COPYRIGHT (C) ST-Microelectronics 1999, 2000.

Date        Modification                                                Name
----        ------------                                                ----
15-Sep-99   Created                                                     FC
13-Jan-00   Added LMI regsisters                                        FC
21-Mar-00   Changed all LMI regsisters                                  FC
20-Jul-00   Added Clock Gen registers                                   FC

TODO:
=====
TODO: FC: Update LMI bits (completely obsolete)

*****************************************************************************/

#ifndef __CFG_REG_H
#define __CFG_REG_H

/* Includes --------------------------------------------------------------- */

#ifdef __cplusplus
extern "C" {
#endif

/* Exported Types --------------------------------------------------------- */

/* Exported Constants ----------------------------------------------------- */

/* Register base address */
#define CFG CFG_BASE_ADDRESS    /* CFG block base address */
#define LMI LMI_BASE_ADDRESS    /* LMI block base address */
#define CKG CKG_BASE_ADDRESS    /* Clock Gen base address */

/* Register offsets */
#define CFG_CCF                 0x04 /* Chip Configuration,
                                        enable interfaces */
#define CFG_CDCF                0x08 /* CD input configuration */
#define CFG_PORD                0x0c /* Priority Order in LMC */
#define CFG_CDG                 0x10 /* CD Mngt config, bitstream channel
                                        gathering */
#define CFG_VDCTL               0x14 /* Video Decoder Ctrl */
#define CFG_DECPR               0x1c /* Multi-decode configuration */
#define CFG_VSEL                0x20 /* VSync selection for decode
                                        Synchronisation */
#define CFG_CMPEPS              0x114 /* Epsilon value for
                                         compression/decompression */
#define CFG_IDCOD               0x1f8 /* JTAG Id */
#define CFG_USRID               0x1fc /* User Id */

#define CFG_AUDIO_IO            0xc60 /* Audio outputs configuration */

#if defined (ST_7020)
#define CFG_AUDIO_IO_INIT       0x0f  /* Default value */
#else
#define CFG_AUDIO_IO_INIT       0x07  /* Default value */
#endif

#define CFG_GAM_PGAMMA          0xF04 /* Priority of Gamma processes */

#define EMPI_INT_LINE           0x18 /* Interrupt Line Register */
#define EMPI_WPRI               0x80 /* Priority of write micro accesses */
#define EMPI_RPRI               0x84 /* Priority of read micro accesses */
#define EMPI_DPRI               0x88 /* Priority of DMA micro accesses */
#define EMPI_WTIM               0x8c /* DMA flush timer */
#define EMPI_PTIM               0x90 /* Timer for micro priority latency */
#define EMPI_RCFG               0x94 /* Configuration of read ahead */
#define EMPI_DADD0              0x98 /* DMA0 channel start address */
#define EMPI_DADD1              0x9c /* DMA1 channel start address */
#define EMPI_END                0xa0 /* Configuration of endianness
                                        (all 0 or all 1) */
#define EMPI_DCFG0              0xa4 /* DMA0 channel config */
#define EMPI_DCFG1              0xa8 /* DMA1 channel config */
#if defined(ST_7020)
#define EMPI_DTIM               0xac  /* DMA timer */
#define EMPI_5516               0xb0  /* 16-bit interface enable */
#endif
#define EMPI_ITM                0x118 /* Interrupt mask */
#define EMPI_ITS                0x11c /* Interrupt status */
#define EMPI_STA                0x120 /* Status register */

#define LMI_TRCR                0x00
#define LMI_TRC                 0x04
#define LMI_TRP                 0x08
#define LMI_TWRD                0x0C
#define LMI_TRAS                0x10
#define LMI_TRRD                0x14
#define LMI_TRCD                0x18
#define LMI_TDPLR               0x1C
#define LMI_TDPLW               0x20
#define LMI_TBSTW               0x24

#define LMI_LAF_DEPTH           0x28
#define LMI_EMRS                0x2C
#define LMI_MRS                 0x30

#define LMI_REQ_EMRS            0x34
#define LMI_REQ_MRS             0x38
#define LMI_REQ_CBR             0x3C
#define LMI_REQ_PALL            0x40

#define LMI_CKE                 0x48
#define LMI_ENA                 0x4C

#define LMI_CFG                 0x50 /* LMI configuration */
#define LMI_REF_CFG             0x54 /* LMI refresh configuration */
#define LMI_IO_CFG              0x5C /* LMI IO pads configuration */
#if defined(ST_7015)
  #define LMI_DLL_CTRL          0x60
  #define LMI_DLL_RST           0x6C
#elif defined(ST_7020)
  #define LMI_DLL               0x60
  #define LMI_DLL_CTRL          0x6C
#endif
#define LMI_DLL_STATUS          0x64
#define LMI_CTRL                0x68
#define LMI_DLL_LCKCTL          0x70


#define CKG_SYN0_PAR            0x40 /* Frequency synthesizer parameters */
#define CKG_SYN1_PAR            0x44 /* Frequency synthesizer parameters */
#define CKG_SYN2_PAR            0x48 /* Frequency synthesizer parameters */
#define CKG_SYN3_PAR            0x4C /* Frequency synthesizer parameters */
#define CKG_SYN4_PAR            0x50 /* Frequency synthesizer parameters */
#define CKG_SYN5_PAR            0x54 /* Frequency synthesizer parameters */
#define CKG_SYN6_PAR            0x58 /* Frequency synthesizer parameters */
#define CKG_SYN7_PAR            0x5C /* Frequency synthesizer parameters */

#define CKG_SYN3_REF            0x70 /* Synthesizer 3 ref clock selection */
#define CKG_SYN7_REF            0x74 /* Synthesizer 7 denc pixel clock
                                        selection   */

#define CKG_PCMO_CFG            0xC74 /* PCM output clock generation config */
#if defined(ST_7015)
  #define CKG_PCMO_MD           0xC6C /* PCM output freq synth MD parameter */
  #define CKG_PCMO_PE           0xC68 /* PCM output freq synth PE parameter */
  #define CKG_PCMO_PSEL         0xC70 /* PCM output freq synth parameter
                                         selection  */
  #define CKG_PCMO_SDIV         0xC64 /* PCM output freq synth SDIV
                                         parameter */
#elif defined(ST_7020)
  #define CKG_PCMO_PAR          0xC64 /* PCM output freq synth parameters */
#endif

#define CKG_PRE_DIV             0x6C /* PreDivider setup             */
#define CKG_SEL_AHS             0xE4 /* Audio bit handshake clock selection */
#define CKG_SEL_AUD             0xD0 /* Audio DSP clock selection     */
#if defined(ST_7015)
  #define CKG_SEL_DENC          0xE0 /* DENC and auxiliary pixel clock selection */
#elif defined(ST_7020)
  #define CKG_SEL_AUX           0xE0 /* DENC and auxiliary pixel clock selection */
#endif
#define CKG_SEL_DDR             0xC0 /* DDR interface clock selection      */
#if defined(ST_7015)
  #define CKG_SEL_DEC           0xD8 /* Video decoder decomp clock selection */
#elif defined(ST_7020)
  #define CKG_SEL_ADISP         0xD8 /* Auxiliary display clock selection  */
#endif
#define CKG_SEL_DISP            0xDC /* Display processors clock selection */
#define CKG_SEL_GFX             0xC8 /* Graphics engine clock selection    */
#define CKG_SEL_MEM             0xC4 /* Memory subsystem clock selection   */
#define CKG_SEL_PIX             0xCC /* Main pixel clock selection         */
#define CKG_SEL_RCLK2           0xF0 /* RCLK2 selection                    */
#define CKG_SEL_USR1            0xF4 /* User clock 1 selection             */
#define CKG_SEL_USR2            0xF8 /* User clock 2 selection             */
#define CKG_SEL_USR3            0xFC /* User clock 3 selection             */
#define CKG_SEL_VID             0xD4 /* Video MPEG decoder clock selection */

/*------------------------- CFG registers bits: -----------------------------*/

/* CFG_CCF - Chip Configuration, enable interfaces --------------------------*/
#define CFG_CCF_EVI             0x01 /* Enable Video Interface             */
#define CFG_CCF_ECDI            0x02 /* Enable Compressed Data Interface   */
#define CFG_CCF_SD2X            0x04 /* Enable Double SD input on SD
                                        Interface         */

/* CFG_CDCF - CD input configuration ---------------------------------------*/
#define CFG_CDCF_SPS            0x01/* Subpicture mode select          */
#define CFG_CDCF_CDR            0x02 /* xxx_CDI register CD input only
                                        selection   */

/* CFG_CDG - CD Mngt config, bitstream channel gathering --------------------*/
#define CFG_CDG_GV1                     0x01                    /* Gather compressed data for video 1 Parser  */
#define CFG_CDG_GV2                     0x02                    /* Gather compressed data for video 2 Parser  */
#define CFG_CDG_GV3                     0x04                    /* Gather compressed data for video 3 Parser  */
#define CFG_CDG_GV4                     0x08                    /* Gather compressed data for video 4 Parser  */
#define CFG_CDG_GV5                     0x10                    /* Gather compressed data for video 5 Parser  */
#define CFG_CDG_GA1                     0x20                    /* Gather compressed data for audio 1 Parser  */
#define CFG_CDG_GA2                     0x40                    /* Gather compressed data for audio 2 Parser  */

/* CFG_DECPR - Multi-decode configuration ------------------------------------------------*/
#define CFG_DECPR_SELA_MASK 0x000007            /* 1st priority decoder                                           */
#define CFG_DECPR_SELB_MASK 0x000038            /* 2nd priority decoder                                           */
#define CFG_DECPR_SELC_MASK 0x0001C0            /* 3rd priority decoder                                           */
#define CFG_DECPR_SELD_MASK 0x000E00            /* 4th priority decoder                                           */
#define CFG_DECPR_SELE_MASK 0x007000            /* 5th priority decoder                                           */
#define CFG_DECPR_PS            0x008000                /* Priority Setup                                                         */
#define CFG_DECPR_FP            0x010000                /* Force Priority                                                         */

/* CFG_VDCTL - Video Decoder Ctrl --------------------------------------------------------*/
#define CFG_VDCTL_EDC           0x01                    /* Enable decoding                                                        */
#define CFG_VDCTL_ERO           0x02                    /* Automatic pipeline reset on overflow error */
#define CFG_VDCTL_ERU           0x04                    /* Automatic pipeline reset on underflow      */
#define CFG_VDCTL_MVC           0x08                    /* Ensure motion vector stays inside picture  */
#define CFG_VDCTL_SPI           0x10                    /* Slice picture ID check                     */

/* CFG_REV - Chip revision register ------------------------------------------------------*/
#define CFG_REV_MASK            0xff            /* Mask for chip version register */

/* CFG_USRID - User ID -------------------------------------------------------------------*/
#define CFG_USRID_AA            0x810     /* STi7015 1.0 */
#define CFG_USRID_AB            0x811     /* STi7015 1.1 */


/* EMPI_INT_LINE - Interrupt Line Register -----------------------------------------------*/
#define INT_NBR_OF_LINES        19                              /* total number of int lines                  */
#define VID1_IRQ                        0x00000001              /* Video 1 interrupt bit                              */
#define VID2_IRQ                        0x00000002              /* Video 2 interrupt bit                              */
#define VID3_IRQ                        0x00000004              /* Video 3 interrupt bit                              */
#define VID4_IRQ                        0x00000008              /* Video 4 interrupt bit                              */
#define VID5_IRQ                        0x00000010              /* Video 5 interrupt bit                              */
#define AUD1_IRQ                        0x00000020              /* Audio 1 interrupt bit                              */
#define AUD2_IRQ                        0x00000040              /* Audio 2 interrupt bit                              */
#define VTG1_IRQ                        0x00000080              /* VTG 1 interrupt bit                                */
#define VTG2_IRQ                        0x00000100              /* VTG 2 interrupt bit                                */
#define SDIN1_IRQ                       0x00000200              /* SDIN 1 interrupt bit                               */
#define SDIN2_IRQ                       0x00000400              /* SDIN 2 interrupt bit                               */
#define HDIN_IRQ                        0x00000800              /* HDIN interrupt bit                                 */
#define DIP_IRQ                         0x00001000              /* Aux display interrupt bit                  */
#define DIS_IRQ                         0x00002000              /* Main display interrupt bit                 */
#define EMPI_IRQ                        0x00004000              /* EMPI interrupt bit                                 */
#define AUDDSP_IRQ                      0x00008000              /* Internal audio interrupt bit               */
#define BLITTER_IRQ                     0x00010000              /* Gamma blitter interrupt bit                */
#define EXT_AUD_IRQ                     0x00020000              /* External audio interrupt bit               */
#define AUD_PCM_IRQ                     0x00040000              /* Audio PCM mixing interrupt bit                 */

/*------------------------- LMI registers bits: ------------------------------------------*/
#if 0 /* FC: Not updated for 7015 and 7020 !!! */
/* LMI_CFG - LMI configuration -----------------------------------------------------------*/
#define LMI_CFG_DIFF_FREQ       0x00000                 /* STBUS and LMICOre at different frequencies */
#define LMI_CFG_SAME_FREQ       0x10000                 /* STBUS and LMICOre at same frequency            */

#define LMI_CFG_BS1_POS22       0x0000                  /* 2nd SDRAM bank select bit 22 in input address */
#define LMI_CFG_BS1_POS23       0x0100                  /* 2nd SDRAM bank select bit 23 in input address */
#define LMI_CFG_BS1_POS24       0x0200                  /* 2nd SDRAM bank select bit 24 in input address */
#define LMI_CFG_BS1_POS25       0x0300                  /* 2nd SDRAM bank select bit 25 in input address */
#define LMI_CFG_BS1_POS26       0x0400                  /* 2nd SDRAM bank select bit 26 in input address */
#define LMI_CFG_BS1_POS9        0x0700                  /* 2nd SDRAM bank select bit 9 in input address  */

#define LMI_CFG_256_COL         0x0000                  /* SDRAM 256 columns per bank                 */
#define LMI_CFG_512_COL         0x0040                  /* SDRAM 512 columns per bank                 */
#define LMI_CFG_1024_COL        0x0080                  /* SDRAM 1024 columns per bank                */

#define LMI_CFG_2048_ROW        0x0000                  /* SDRAM 2048 rows per bank                   */
#define LMI_CFG_4096_ROW        0x0010                  /* SDRAM 4096 rows per bank                   */
#define LMI_CFG_8192_ROW        0x0020                  /* SDRAM 9192 rows per bank                   */

#define LMI_CFG_16_BIT          0x0000                  /* SDRAM 16 bit data bus                      */
#define LMI_CFG_32_BIT          0x0040                  /* SDRAM 32 bit data bus                      */
#define LMI_CFG_64_BIT          0x0080                  /* SDRAM 64 bit data bus                      */

#define LMI_CFG_SDRAM           0x0000                  /* JEDEC-SDRAM                                */
#define LMI_CFG_SGRAM           0x0001                  /* PC-SDRAM/DDR-SGRAM                         */
#define LMI_CFG_DDR                     0x0002                  /* DDR-SDRAM                                  */

/* LMI_MRS_CFG - LMI MRS configuration ---------------------------------------------------*/
#define LMI_MRS_DLL_RST         0x100                   /* DLL reset                                  */
#define LMI_MRS_CAS_2_CYC       0x20                    /* CAS latency during read access: 2 cycles   */
#define LMI_MRS_CAS_3_CYC       0x30                    /* CAS latency during read access: 3 cycles   */
#define LMI_MRS_CAS_2_5_CYC     0x60                    /* CAS latency during read access: 2.5 cycles */
#define LMI_MRS_BURST_8         0x03                    /* Burst length: 8                            */

/* LMI_EMRS_CFG - LMI EMRS configuration -------------------------------------------------*/
#define LMI_DLL_EN                      0x01                    /* DDR-SDRAM DLL enable                       */

/* LMI_REF_CFG - LMI refresh configuration -----------------------------------------------*/
#define LMI_REF_EN                      0x10000                 /* Refresh enable                             */
#define LMI_REF_CYCLES_MASK     0x7ff                   /* Mask for refresh interval in clock cycles  */

/* LMI_LAF_CFG - LMI look-ahead FIFO configuration ---------------------------------------*/
#define LMI_LAF_DEP_MASK        0x0F                    /* Mask for LMI look-ahead FIFO depth         */
#define LMI_LAF_EN                      0x10                    /* Enable LMI look-ahead FIFO                 */

/* LMI_SDR_CMD - LMI SDRAM command -------------------------------------------------------*/
#define LMI_SDR_CMD_PD          0x20                    /* LMI SDRAM command PD                       */
#define LMI_SDR_CMD_NOP         0x10                    /* LMI SDRAM command NOP                      */
#define LMI_SDR_CMD_CBR         0x08                    /* LMI SDRAM command CBR                      */
#define LMI_SDR_CMD_PALL        0x04                    /* LMI SDRAM command PALL                     */
#define LMI_SDR_CMD_EMRS        0x02                    /* LMI SDRAM command EMRS                     */
#define LMI_SDR_CMD_MRS         0x01                    /* LMI SDRAM command MRS                      */

/* LMI_CTRL - LMI control ----------------------------------------------------------------*/
#define LMI_EN                          0x01                    /* LMI enable */

/* LMI_CMD_TIMING - LMI command timing ---------------------------------------------------*/
#define LMI_CMD_T_RC_MASK       0x0f                    /* Mask for LMI command tRC                   */
#define LMI_CMD_T_RAS_MASK      0xf0                    /* Mask for LMI command tRAS                  */
#define LMI_CMD_T_RCD_MASK      0x300                   /* Mask for LMI command tRCD                  */
#define LMI_CMD_T_RRD_MASK      0xc00                   /* Mask for LMI command tRRD                  */
#define LMI_CMD_T_RP_MASK       0x3000                  /* Mask for LMI command tRP                   */
#define LMI_CMD_T_DPLW_MASK     0xc000                  /* Mask for LMI command tDPLW                 */
#define LMI_CMD_T_DPLR_MASK     0x30000                 /* Mask for LMI command tDPLR                 */
#define LMI_CMD_T_WRD_MASK      0xc0000                 /* Mask for LMI command tWRD                  */
#define LMI_CMD_T_BSTW_MASK     0x300000                /* Mask for LMI command tBSTW                 */

#endif /* 0 */

/*------------------------- CKG registers bits: ------------------------------------------*/
/* CKG_SYNn_PAR - Frequency synthesizer parameters ---------------------------------------*/
#define CKG_SYN_PAR_PE_MASK             0x0000ffff      /* Fine selector mask                     */
#define CKG_SYN_PAR_MD_MASK             0x001f0000      /* Coarse selector mask                   */
#define CKG_SYN_PAR_SDIV_MASK           0x00e00000      /* Output divider mask                    */
#define CKG_SYN_PAR_NDIV_MASK           0x03000000      /* Input divider mask                     */
#define CKG_SYN_PAR_PARAMS_MASK         0x03ffffff      /* Or of the 4 preceding masks            */
#define CKG_SYN_PAR_SSL                 0x04000000      /* not Bypassed bit                       */
#define CKG_SYN_PAR_EN                  0x08000000      /* Enable bit                             */
#if defined(ST_7020)
  #define CKG_SYN_PAR_PDIV              0x10000000      /* PreDiv bit for CKG_PCMO_PAR            */
#endif

/* CKG_PCMO_CFG - PCM output clock generation config -------------------------------------*/
#if defined(ST_7015)
#define CKG_PCMO_CFG_SSL                0x01
#define CKG_PCMO_CFG_CKRS               0x02
#define CKG_PCMO_CFG_I2SS               0x04
#define CKG_PCMO_CFG_I2S                0x08
#define CKG_PCMO_CFG_SFO                0x10
#define CKG_PCMO_CFG_NDIV               0x60
#elif defined(ST_7020)
  #define CKG_PCMO_CFG_CKRS             0x01
  #define CKG_PCMO_CFG_I2SS             0x02
  #define CKG_PCMO_CFG_I2S              0x04
  #define CKG_PCMO_CFG_SFO              0x08
#endif
/* CKG_PRE_DIV - PreDivider setup --------------------------------------------------------*/
#define CKG_PRE_DIV_SO                  0x3                                     /* Selection of output                    */
#define CKG_PRE_DIV_SI                  0x4                                     /* Selection of input                     */
#define CKG_PRE_DIV_PXO                 0x8                                     /* Pixel clock output                             */


#ifdef __cplusplus
}
#endif

#endif /* __CFG_REG_H */

/* End of cfg_reg.h */
