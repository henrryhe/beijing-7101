/*****************************************************************************

File name   : hrv_dec2.h

Description : Video decoder module HAL registers for omega 2 header file

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                         Name
----               ------------                         ----
15/05/00           Created                              Philippe Legeard
13/03/01           PS field of DMOD & SDMOD access      GGn
*****************************************************************************/

#ifndef __DECODE_REGISTERS_OMEGA2_H
#define __DECODE_REGISTERS_OMEGA2_H

/* Includes --------------------------------------------------------------- */
#include "stddefs.h"
#include "stsys.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Exported Types --------------------------------------------------------- */

/* Exported Constants ----------------------------------------------------- */

/******************************************************************************/
/*- MPEG-2 video decoder registers -------------------------------------------*/
/******************************************************************************/

/*- Memory Mode registers ------------------------------------------------*/
#define VIDn_DMOD_16                    0x80           /* Main Decoding Memory mode */
#define VIDn_DMOD                       0x80           /* Main Decoding Memory mode */
#define VIDn_DMOD_MASK                  0x0740

/* indicates progressive or interlaced storage in frame buffers */
#define VIDn_MOD_PS_MASK               1
#define VIDn_MOD_PS_EMP                9
/* Enables reconstruction in frame buffers */
#define VIDn_MOD_EN_MASK                1
#define VIDn_MOD_EN_EMP                 8
/* OverWrite mode */
#define VIDn_MOD_OVW_MASK               1
#define VIDn_MOD_OVW_EMP                6
/* HD-PIP operation mode */
#define VIDn_MOD_HPD_MASK               1
#define VIDn_MOD_HPD_EMP                10
#define VIDn_MOD_HPS_MASK               1
#define VIDn_MOD_HPS_EMP                9

/* Compression mode : Only available forchip  STi7015 */
#define VIDn_DMOD_COMP_MASK            0x03
#define VIDn_DMOD_COMP_NONE            0x00
#define VIDn_DMOD_COMP_HQ              0x01
#define VIDn_DMOD_COMP_SQ              0x03

#define VIDn_SDMOD_16                   0x88           /* Secondary decoding Memory Mode */
#define VIDn_SDMOD                      0x88           /* Secondary decoding Memory Mode */
#define VIDn_SDMOD_MASK                 0x037C

#define VIDn_SDMOD_DEC_MASK             0x3c
#define VIDn_SDMOD_VDEC_NONE            0x00
#define VIDn_SDMOD_VDEC_V2              0x04
#define VIDn_SDMOD_VDEC_V4              0x08
#define VIDn_SDMOD_HDEC_NONE            0x00
#define VIDn_SDMOD_HDEC_H2              0x10
#define VIDn_SDMOD_HDEC_H4              0x20

/*------------------------------------------------------------------------*/

/*- Interrupt & Status registers -----------------------------------------*/
#define VIDn_STA_16                     0xA0                  /* Status Register  */
#define VIDn_STA_MASK                   0x7FFF
#define VIDn_ITM_16                     0x98                  /* Mask register    */
#define VIDn_ITM                        0x98                  /* Mask register    */
#define VIDn_ITM_MASK                   0x7FFF
#define VIDn_ITS_16                     0x9C                  /* Interrupt Status */
#define VIDn_ITS_MASK                   0x7FFF
/*------------------------------------------------------------------------*/

/*- Decoded Frame pointers registers -------------------------------------*/
#define VIDn_BCHP_32                    0x1C               /* Backward Chroma Frame Buffer                  */
#define VIDn_BFP_32                     0x18               /* Backward Luma Frame Buffer                    */
#define VIDn_FCHP_32                    0x14               /* Forward Chroma Frame Buffer                   */
#define VIDn_FFP_32                     0x10               /* Forward Luma Frame Buffer                     */
#define VIDn_RCHP_32                    0x0C               /* Main Reconstructed Chroma Frame Pointer       */
#define VIDn_RFP_32                     0x08               /* Main Reconstructed Luma Frame Pointer         */
#define VIDn_SRCHP_32                   0x24               /* Secondary Reconstructed chroma Frame Pointer  */
#define VIDn_SRFP_32                    0x20               /* Secondary Reconstructed Luma Frame Pointer    */
/* mask */
#define VIDn_FP_MASK                    0x03FFFE00
/*------------------------------------------------------------------------*/

/*- reset registers ------------------------------------------------------*/
#define VIDn_PRS_8                      0x94               /* Decoding Pipeline Reset  */
#define VIDn_PRS                        0x94               /* Decoding Pipeline Reset  */
#define VIDn_SRS_8                      0x90               /* Decoding soft Reset      */
#define VIDn_SRS                        0x90               /* Decoding soft Reset (32bits access).       */
/*------------------------------------------------------------------------*/

/*- Decoded Frame other registers ----------------------------------------*/
#define VIDn_DFH_8                      0x64               /* Decoded Frame Height                       */
#define VIDn_DFH                        0x64               /* Decoded Frame Height                       */
#define VIDn_DFH_MASK                   0x7F               /* mask */
#define VIDn_DFS_16                     0x68               /* Decoded Frame Size                         */
#define VIDn_DFS                        0x68               /* Decoded Frame Size                         */
#define VIDn_DFS_MASK                   0x1FFF             /* mask */
#define VIDn_DFW_8                      0x60               /* Decoded Frame width                        */
#define VIDn_DFW                        0x60               /* Decoded Frame width                        */
#define VIDn_DFW_MASK                   0xFF               /* mask */
/*------------------------------------------------------------------------*/

/*- Bit buffer registers -------------------------------------------------*/
#define VIDn_BBG_32                     0x40               /* Start of bit buffer */
#define VIDn_BBS_32                     0x44               /* Bit Buffer Stop */
#define VIDn_BBL_32                     0x4C               /* Bit Buffer Level */
#define VIDn_BBT_32                     0x48               /* Bit buffer Threshold */
#define VIDn_BB_MASK                    0x03FFFF00
/*------------------------------------------------------------------------*/

/*- Quantization registers -----------------------------------------------*/
#define VIDn_QMWI_32                    0xe00              /* Quantization Matrix data, intra table      */
#define VIDn_QMWNI_32                   0xe40              /* Quantization Matrix data, non intra table  */
/*------------------------------------------------------------------------*/

/*- Video Setup Register  ------------------------------------------------*/
#define VIDn_STP_8                      0x78
#define VIDn_STP                        0x78
#define VIDn_STP_MASK                   7
/* masks */
#define VIDn_STP_PBO_EMP                0
#define VIDn_STP_PBO_MASK               1
#define VIDn_STP_SSC_EMP                1
#define VIDn_STP_SSC_MASK               1
#define VIDn_STP_TM_EMP                 2
#define VIDn_STP_TM_MASK                1
/*------------------------------------------------------------------------*/

/*- Header Data fifo read ------------------------------------------------*/
#define VIDn_HDF_32                     0xEC
#define VIDn_HDF_SCM_EMP                16
#define VIDn_HDF_SCM_MASK               0x1
#define VIDn_HDF_HDF_EMP                0
#define VIDn_HDF_HDF_MASK               0xFFFF
#define VIDn_HDF_SCM                    0x00010000
#define VIDn_HDF_HDF                    0x0000FFFF

/*------------------------------------------------------------------------*/

/*- Header Launch Register ------------------------------------------------*/
#define VIDn_HLNC_8                               0xF0
#define VIDn_HLNC                                 0xF0
#define VIDn_HLNC_SC_SEARCH_NORMALLY_LAUNCHED     1
#define VIDn_HLNC_SC_SEARCH_FROM_ADDR_LAUNCHED    2
/*------------------------------------------------------------------------*/



/*- Picture Parameters ---------------------------------------------------*/
#define VIDn_PPR_32                     0x04               /* Picture Parameters                         */
#define VIDn_PPR_MASK                   0x0FFFFFFF
/* masks */
#define VIDn_PPR_ULO_EMP                28
#define VIDn_PPR_TFF_EMP                27
#define VIDn_PPR_FRM_EMP                26
#define VIDn_PPR_CMV_EMP                25
#define VIDn_PPR_QST_EMP                24
#define VIDn_PPR_IVF_EMP                23
#define VIDn_PPR_AZZ_EMP                22
/* PCT */
#define VIDn_PPR_PCT_MASK               3
#define VIDn_PPR_PCT_EMP                20
/* DCP */
#define VIDn_PPR_DCP_MASK               3
#define VIDn_PPR_DCP_EMP                18
/* PST */
#define VIDn_PPR_PST_MASK               3
#define VIDn_PPR_PST_EMP                16
/* BFV */
#define VIDn_PPR_BFV_MASK               0xF
#define VIDn_PPR_BFV_EMP                12
/* FFV */
#define VIDn_PPR_FFV_MASK               0xF
#define VIDn_PPR_FFV_EMP                8
/* BFH */
#define VIDn_PPR_BFH_MPG1_BFC_MASK      7
#define VIDn_PPR_BFH_MPG1_FPBV_EMP      7
#define VIDn_PPR_BFH_MPG2_BHFC_MASK     0xF
#define VIDn_PPR_BFH_EMP                4
/* FFH */
#define VIDn_PPR_FFH_MPG1_FFC_MASK      7
#define VIDn_PPR_FFH_MPG1_FPFV_EMP      3
#define VIDn_PPR_FFH_MPG2_FHFC_MASK     0xF
#define VIDn_PPR_FFH_EMP                0
/* FFH */
#define VIDn_PPR_FFH_MASK               7
/*#define VIDn_PPR_FFH_EMP                0*/
/*------------------------------------------------------------------------*/

/*- SCD Start Pointer ----------------------------------------------------*/
#define VIDn_SCDPTR_32                  0xAC
#define VIDn_SCDPTR_MASK                0x3FFFFFFF
/*------------------------------------------------------------------------*/

/*- Task Instruction -----------------------------------------------------*/
#define VIDn_TIS_16                     0x00
#define VIDn_TIS                        0x00
#define VIDn_TIS_MASK                   0x03BF

#define VIDn_TIS_EXE                0x001
#define VIDn_TIS_FIS                0x002
#define VIDn_TIS_SKP                0x00c
#define VIDn_TIS_MP2                0x010
#define VIDn_TIS_DPR                0x020
#define VIDn_TIS_LFR                0x080
#define VIDn_TIS_LDP                0x100
#define VIDn_TIS_SCN                0x200

#define NO_SKIP               0x00 /* Dont skip any picture                 */
#define SKIP_ONE_DECODE_NEXT  0x04 /* skip one pict and decode next pict    */
#define SKIP_TWO_DECODE_NEXT  0x08 /* skip two pict and decode next pict    */
#define SKIP_ONE_STOP_DECODE  0x0c /* skip one pict and stop decoder        */

/*------------------------------------------------------------------------*/

#define CFG_OFFSET           (0x00000000)

/* CFG - Video Decoder Configuration registers -------------------------------*/
#define CFG_VDCTL           0x14    /* Video Decoder Ctrl */
#define CFG_PORD            0x0c    /* Priority Order in LMC */
#define CFG_PORD_MAD_MASK   0x20
#define CFG_PORD_MMD_MASK   0x40
#define CFG_CMPEPS          0x114   /* Epsilon value for compression/decompression */
#define CFG_DECPR           0x1c    /* Multi-decode configuration */
#define CFG_VSEL            0x20    /* VSync selection for decode Synchronisation */

/* CFG_DECPR - Multi-decode configuration ------------------------------------*/
#define CFG_DECPR_SELA_MASK 0x000007    /* 1st priority decoder */
#define CFG_DECPR_SELB_MASK 0x000038    /* 2nd priority decoder */
#define CFG_DECPR_SELC_MASK 0x0001C0    /* 3rd priority decoder */
#define CFG_DECPR_SELD_MASK 0x000E00    /* 4th priority decoder */
#define CFG_DECPR_SELE_MASK 0x007000    /* 5th priority decoder */
#define CFG_DECPR_PS        0x008000    /* Priority Setup */
#define CFG_DECPR_FP        0x010000    /* Force Priority */

/* CFG_VDCTL - Video Decoder Ctrl --------------------------------------------*/
#define CFG_VDCTL_EDC   0x01    /* Enable decoding                            */
#define CFG_VDCTL_ERO   0x02    /* Automatic pipeline reset on overflow error */
#define CFG_VDCTL_ERU   0x04    /* Automatic pipeline reset on underflow      */
#define CFG_VDCTL_MVC   0x08    /* Ensure motion vector stays inside picture  */
#define CFG_VDCTL_SPI   0x10    /* Slice picture ID check                     */


/*- PES Association Stamps registers -------------------------------------*/
#define VIDn_PES_ASS_8                   0xE4              /* PES Association */
#define VIDn_PES_ASS_TSA_MASK            1
#define VIDn_PES_ASS_DSA_MASK            2
#define VIDn_PES_TS1_16                  0xE0              /* PES Time Stamp MSB */
#define VIDn_PES_TS1_TS_MASK             1
#define VIDn_PES_TS0_32                  0xDC              /* PES Time Stamp LSB */
/*------------------------------------------------------------------------*/

/*- PES Parser configuration ---------------------------------------------*/
#define VIDn_PES_CFG_8                   0xC0               /* PES Parser Configuration     */
#define VIDn_PES_CFG                     0xC0               /* PES Parser Configuration     */
#define VIDn_PES_CFG_MASK                0xFF
#define VIDn_PES_CFG_SP_MASK             1
#define VIDn_PES_CFG_SP_EMP              7
#define VIDn_PES_CFG_PTF_MASK            1
#define VIDn_PES_CFG_PTF_EMP             6
#define VIDn_PES_CFG_MODE_MASK           3
#define VIDn_PES_CFG_MODE_EMP            4
#define VIDn_PES_CFG_OFS_MASK            3
#define VIDn_PES_CFG_OFS_EMP             2
#define VIDn_PES_CFG_SDT_MASK            1
#define VIDn_PES_CFG_SDT_EMP             1
#define VIDn_PES_CFG_SS_MASK             1
#define VIDn_PES_CFG_SS_EMP              0

#define VIDn_PES_CFG_MODE_AUTO        0x00   /* Mode 00: Auto                 */
#define VIDn_PES_CFG_MODE_MP1         0x10   /* Mode 01: MPEG-1 system stream */
#define VIDn_PES_CFG_MODE_MP2         0x20   /* Mode 10: MPEG-2 PES stream    */

#define VIDn_PES_CFG_OFS_PES          0x00   /* Output format PES output      */
#define VIDn_PES_CFG_OFS_ES_SUBID     0x04   /* Output format ES substream ID layer removed */
#define VIDn_PES_CFG_OFS_ES           0x0C   /* Output format ES "normal"     */

#define VIDn_PES_CFG_SS_SYSTEM        0x01   /* System stream */

/*------------------------------------------------------------------------*/

/*- Set Stream ID registers ----------------------------------------------*/
#define VIDn_PES_FSI_8                   0xC8               /* Filter PES Stream ID         */
#define VIDn_PES_FSI                     0xC8               /* Filter PES Stream ID         */
#define VIDn_PES_SI_8                    0xC4               /* PES Stream identification    */
#define VIDn_PES_SI                      0xC4               /* PES Stream identification    */

/*------------------------------------------------------------------------*/

/*- HD PIP decoder registers ---------------------------------------------*/
#define VIDn_VPP_CFG                0x120   /* HD-PIP parser configuration          */
#define VIDn_VPP_CFG_EN_MASK            1   /* HD-PIP Enable mask.                  */
#define VIDn_VPP_CFG_EN_EMP             0
#define VIDn_VPP_CFG_FP_MASK            1   /* Force parsing mask.                  */
#define VIDn_VPP_CFG_FP_EMP             1
#define VIDn_VPP_CFG_PAR_MASK           7   /* Parser parameter mask.               */
#define VIDn_VPP_CFG_PAR_DEFAULT_VALUE  4
#define VIDn_VPP_CFG_PAR_EMP            2
#define VIDn_VPP_CFG_FI_MASK            1   /* Force interlaced HD-PIP parsing.     */
#define VIDn_VPP_CFG_FI_EMP             5

#define VIDn_VPP_DFHT               0x124   /* HD-PIP parser frame height threshold */
#define VIDn_VPP_DFHT_MASK         0x3FFF
#define VIDn_VPP_DFWT               0x128   /* HD-PIP parser frame width threshold  */
#define VIDn_VPP_DFWT_MASK         0x3FFF

/*------------------------------------------------------------------------*/

/*- VLD registers -----------------------------------------------------*/
#define VIDn_VLDPTR_32          0x54   /* VLD Start pointer            */
#define VIDn_VLDRPTR_32         0x138  /* VLD Read pointer             */

/*------------------------------------------------------------------------*/

/*- Other bit stream management registers ----------------------------------*/
#define VIDn_CDI_32             0x7C   /* Compressed Data Input Register    */
#define VIDn_CDWL_32            0xA8   /* Compressed Data Write Limit       */

#define VIDn_CDWL_MASK          0x7FFFF
#define VIDn_CDWL_EMP           7

#define VIDn_CDR_32             0xB4   /* Compressed Data From Register     */
#define VIDn_CDC_32             0xB8   /* Bit Buffer Input Counter          */
#define VIDn_SCDC_32            0xBC   /* Bit Buffer Output Counter         */
#define VIDn_CDWPTR_32         0x130   /* Compressed Data Write Pointer     */
#define VIDn_SCDRPTR_32        0x134   /* SCD Read Pointer                  */


#define VIDn_SCDC_32_SCDC      0x01FFFFFE   /* Bit Buffer Input Counter mask */


/*- Other registers ------------------------------------------------------*/
#define VIDn_PES_DSM_8               0xD8   /* DSM Trick Mode               */
#define VIDn_PES_TFI_8               0xD4   /* Trick Filtering PES Strem ID */
#define VIDn_PES_TFI                 0xD4   /* Trick Filtering PES Strem ID */
#define VIDn_PSCPTR_32               0x50   /* PSC pointer                  */


/*------------------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* #ifndef __DECODE_REGISTERS_OMEGA2_H */

/* End of hvr_dec2.h */
