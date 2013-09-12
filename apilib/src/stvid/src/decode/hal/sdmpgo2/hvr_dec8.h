/*******************************************************************************

File name   : hvr_dec8.h

Description : Video decoder module HAL registers for SDMpegO2 IP header file

COPYRIGHT (C) STMicroelectronics 2005

Date               Modification                                     Name
----               ------------                                     ----
24 fev 2003        Created                                          PLe
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __DECODE_REGISTERS_SDMPGO2_H
#define __DECODE_REGISTERS_SDMPGO2_H


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"


/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */

/* ------------------------------------------------------------------------------------ */
#define VID1_OFFSET                         0x000
#define VID2_OFFSET                         0x300

/* ------------------------------------------------------------------------------------ */
#define VIDn_CDI                            0x000             /* Compress Data Input Register */

/* ------------------------------------------------------------------------------------ */
#define CFG_DECPR                           0x000             /* Decoding Priority Setup */
/* This register contains indications for the multi-decoder in term of bitstream decoding order. */
#define CFG_DECPR_FORCE_PRIORITY            0x00000001
#define CFG_DECPR_PRIO_TO_VID1              0x00000000
#define CFG_DECPR_PRIO_TO_VID2              0x00000002

/* ------------------------------------------------------------------------------------ */
#define CFG_PORD                            0x004             /* Priority Order of Decoding Processes */
/* This register allows the modification of memory access priorities on T2_CD and T2_VD plugs. For */
/* T2_VD, a fixed priority is implemented for the decoding processes (DP) on T2_VD plug:  */
/* VLD, Main reconstruction, Second reconstruction, Prediction. */
/* VLD has the highest priority and prediction the lowest priority. A Priority may be set between the */
/* decoding processes (DP), and the start code detectors. */
#define CFG_PORD_CD_MASK                    0x00030000
#define CFG_PORD_CD_PRIO_TO_VID1            0x00000000
#define CFG_PORD_CD_PRIO_TO_VID2            0x00010000
#define CFG_PORD_CD_ROUND_ROBIN             0x00030000

#define CFG_PORD_VD_MASK                    0x00000007
#define CFG_PORD_VD_SCD1_SCD2_DP            0x00000000
#define CFG_PORD_VD_SCD1_DP_SCD2            0x00000001
#define CFG_PORD_VD_SCD2_SCD1_DP            0x00000002
#define CFG_PORD_VD_SCD2_DP_SCD1            0x00000003
#define CFG_PORD_VD_DP_SCD1_SCD2            0x00000004
#define CFG_PORD_VD_DP_SCD2_SCD1            0x00000006

/* ------------------------------------------------------------------------------------ */
#define CFG_THP                             0x014             /* Threshold period */
/* The 8 16 bit value hold by this register defines a reference period T in number of processing clock */
/* cycles (ic_vd). It is combined with the value of the VIDn_THI register to specify the threshold speed of */
/* the decoder. It may be used for performance analysis of the system or debug purpose. */

/* ------------------------------------------------------------------------------------ */
#define CFG_VDCTL                           0x008             /* Video Decoder Control */
#define CFG_VDCTL_RMM_EN                    0x00000010
#define CFG_VDCTL_MVC_EN                    0x00000008
#define CFG_VDCTL_ERU_EN                    0x00000004
#define CFG_VDCTL_ERO_EN                    0x00000002

/* ------------------------------------------------------------------------------------ */
#define CFG_VIDIC                           0x010             /* Video Decoder Interconnect configuration */
/* Maximum packet size for processes on T2 plugs. For some processes, the message size may be smaller */
/* that the packet size. In this case, the packet size is equal to the message size. */
#define CFG_VIDIC_PFO_EN                    0x00010000

#define CFG_VIDIC_LP_MASK                   0x00000007
#define CFG_VIDIC_LP_DEFAULT                0x00000000
#define CFG_VIDIC_LP_8                      0x00000002
#define CFG_VIDIC_LP_4                      0x00000003
#define CFG_VIDIC_LP_2                      0x00000004

#define CFG_VIDIC_PDRL_MASK                 0x00000018
#define CFG_VIDIC_PDRL_DEFAULT              0x00000000
#define CFG_VIDIC_PDRL_5                    0x00000008
#define CFG_VIDIC_PDRL_2                    0x00000010

#define CFG_VIDIC_PDRC_MASK                 0x0000006C
#define CFG_VIDIC_PDRC_DEFAULT              0x00000000
#define CFG_VIDIC_PDRC_3                    0x00000020
#define CFG_VIDIC_PDRC_2                    0x00000040

/* ------------------------------------------------------------------------------------ */
#define CFG_VSEL                            0x00C             /* Vsync Selection */
/* Video decoder VSync selection for Decode synchronization. */
#define CFG_VSEL_VSYNC_FROM_VID1            0x00000000
#define CFG_VSEL_VSYNC_FROM_VID2            0x00000001
#define CFG_VSEL_VSYNC_DISCARDED            0x00000003


/* ------------------------------------------------------------------------------------ */
#define VID1_QMWI                           0x100             /* Quantization Matrix Data, intra table */
#define VID2_QMWI                           0x140
/* The quantization coefficients for intra table must be written to these addresses in increasing order in the */
/* order in which they appear in the bitstream, i.e. in zig-zag order. Thus, the first coefficient which appears */
/* in the bitstream is located at the lower register address and the last coefficient in the bitstream is located */
/* at the higher register address. The order must be strictly respected. Each 32-bit word contains 4 8-bits */
/* coefficients. */

/* ------------------------------------------------------------------------------------ */
#define VID1_QMWNI                          0x180             /* Quantization Matrix Data, non intra table */
#define VID2_QMWNI                          0x1C0
/* The quantization coefficients for non intra table must be written to these addresses in increasing order */
/* in the order in which they appear in the bitstream, i.e. in zig-zag order. Thus, the first coefficient which */
/* appears in the bitstream is located at the lower register address and the last coefficient in the bitstream */
/* is located at the higher register address. The order must be strictly respected. Each 32-bit word contains */
/* 4 8-bits coefficients. */
/* Note Quantization table for picture n+1 may be loaded only when decoding has started for picture n. */


/* ------------------------------------------------------------------------------------ */
#define VIDn_BBG                            0x40C             /* Start of Bit Buffer */
/* This register holds the starting address of the bit buffer in unit of 256 bytes. */

/* ------------------------------------------------------------------------------------ */
#define VIDn_BBL                            0x410             /* Bit Buffer Level */
/* Stores the current level of occupation of the bit buffer, defined in units of 256 bytes. It can be read at any */
/* time. After a reset (Hw or Sw), the bit buffer level is the difference between the CD write pointer and the */
/* SCD read pointer. When the first picture decoding is launched, the bit buffer level becomes the differ-ence */
/* between the CD write pointer and the VLD read pointer. */
/* The register contains a relevant value only when all processes (CD,SCD,VLD) are working in the same */
/* bit buffer. In normal mode, when VIDn_BBL is greater than or equal to VIDn_BBT, the status bit */
/* VIDn_STA.BBF is set. When VIDn_BBL is zero, status bit VIDn_STA.BBE is set. When VIDn_BBL is */
/* equal or below VIDn_BBTL, status bit VIDn_STA.BBNE is set. */

/* ------------------------------------------------------------------------------------ */
#define VIDn_BBS                            0x414             /* Bit Buffer Stop Address */
/* This register holds the ending address of the bit buffer in unit of 256 bytes. The last address of the bit */
/* buffer is ((BBS + 1)* 256 -1) in byte unit. */

/* ------------------------------------------------------------------------------------ */
#define VIDn_BBT                            0x418             /* Bit Buffer Threshold */
/* Stores the threshold level of occupancy of the bit buffer, in units of 256 bytes. When this threshold is */
/* reached, i.e. VIDn_BBL is greater than or equal to VIDn_BBT, the status bit VIDn_STA.BBF is set. The */
/* system must increase the theoretical threshold by at least 512 bytes since the VLD read pointer may */
/* be 512 bytes ahead compared to the position of the VLD at the end of a picture decoding (VLD fifo size */
/* is 512 bytes). */

/* ------------------------------------------------------------------------------------ */
#define VIDn_BBTL                           0x41C             /* Bit Buffer Threshold Low */
/* Stores the low threshold level of occupancy of the bit buffer, in units of 256 bytes. When this threshold */
/* is reached, i.e. VIDn_BBL is lower or equal to VIDn_BBTL, the status bit VIDn_STA.BBNE is set. */

/* ------------------------------------------------------------------------------------ */
#define VIDn_BCHP                           0x488             /* Backward Chroma Frame Buffer */
/* This register holds the start address of the backward frame chroma prediction buffer, #defined in units of */
/* 512 bytes. */

/* ------------------------------------------------------------------------------------ */
#define VIDn_BFP                            0x48C             /* Backward Luma Frame Pointer */
/* This register holds the start address of the luma backward prediction frame picture buffer, defined in */
/* units of 512 bytes. */

/* ------------------------------------------------------------------------------------ */
#define VIDn_CDB                            0x420             /* compressed Data Buffer */
/* This register contains a set of bits which indicates how the bit buffer must be accessed for bitstream */
/* buffering i.e how compressed data are written in the bit buffer. */
#define VIDn_CDB_MASK                       0xFFFFFFF8
#define VIDn_CDB_PBO_EN                     0x00000004
#define VIDn_CDB_WLE_EN                     0x00000002
#define VIDn_CDB_CM_EN                      0x00000001

/* ------------------------------------------------------------------------------------ */
#define VIDn_CDC                            0x5B4             /* Bit Buffer Input Counter */
/* This register holds the number of bytes input to the bit buffer plus 4, including the number of bytes written */
/* in the CD FIFO. It has a relevant value whatever the configuration of the pes parser (even if VIDn_PES_CFG.SS = 0). */

/* ------------------------------------------------------------------------------------ */
#define VIDn_CDLP                           0x428             /* compress Data load pointer */
#define VIDn_CDLP_RESET_PARSER              0x00000002
#define VIDn_CDLP_LOAD_POINTER              0x00000001

/* ------------------------------------------------------------------------------------ */
#define VIDn_CDPTR                          0x424             /* Compressed data pointer */
/* This register stores the value of the CD write pointer defined in units of 128 bytes. The compressed data */
/* write pointer is initialised with this value when VIDn_CDLP.LP is set to 1. */

/* ------------------------------------------------------------------------------------ */
#define VIDn_CDPTR_CUR                      0x42C             /* Value of compressed data pointer */
/* This register stores the current value of the compressed data write pointer computed by the IP. It is de-fined */
/* in unit of 128 bytes. */

/* ------------------------------------------------------------------------------------ */
#define VIDn_CDWL                           0x430             /* Compress Data Write Limit */
/* The compressed data write limit is ONLY taken into account when VIDn_CDB.CDWL = 1. If this condi-tion */
/* is true, when the compressed data write pointer reaches CDWL, writing in the bit buffer is stopped */
/* and VIDn_STA.CDWL is set. It is defined in unit of 128 bytes. */
/* When a new compressed data write pointer is loaded, and VIDn_CDB.WLE = 1 (write limit is on), */
/* no data are written into the bit buffer until a different CD write limit is written. Writing in this reg-ister */
/* must be considered in this case as an exe. */

/* ------------------------------------------------------------------------------------ */
#define VID_DBG1                            0x060             /* Debug register */
/* This register gives the decoding parameters for the picture being decoded. It is updated on PSD event */
/* and is stable until the next same event. It gives information on the picture being decoded. */

/* ------------------------------------------------------------------------------------ */
#define VID_DBG2                            0x064             /* Debug register */

/* ------------------------------------------------------------------------------------ */
#define VIDn_DFH                            0x400             /* Decoded Frame Height */
/* Decoded frame height (in rows of macroblocks). This register is used for error concealment and mac-roblocks */
/* error statistics. This is derived from the vertical size value transmitted in the sequence header. */
/* It is divided internally by 2 in case of field picture decoding. */

/* ------------------------------------------------------------------------------------ */
#define VIDn_DFS                            0x404             /* Decoded Frame Size */
/* This register is set up with a value equal to the number of macroblocks in the decoded picture. This is */
/* derived from the horizontal size and vertical size values transmitted in the sequence header. It is divided */
/* internally by 2 in case of field picture decoding. */

/* ------------------------------------------------------------------------------------ */
#define VIDn_DFW                            0x408             /* Decode Frame Width */
/* This register is set up with a value equal to the width in macroblocks of the decoded picture.This is de-rived */
/* from the horizontal size value transmitted in the sequence header. */

/* ------------------------------------------------------------------------------------ */
#define VIDn_DMOD                           0x4A8             /* Main Decoding Memory Mode */
/* This bit field indicates if main reconstruction must be performed. */
#define VIDn_DMOD_PRI_REC_EN                0x0001
#define VIDn_DMOD_PRI_REC_DIS               0x0000
/* This bit field indicates if the OVW has to be enabled. */
#define VIDn_DMOD_OVW_EN                    0x0002
#define VIDn_DMOD_OVW_DIS                   0x0000

/* ------------------------------------------------------------------------------------ */
#define VIDn_FCHP                           0x490             /* Forward Chroma Frame Buffer */
/* This register holds the start address of the chroma forward prediction frame picture buffer, defined in */
/* units of 512 bytes. */

/* ------------------------------------------------------------------------------------ */
#define VIDn_FFP                            0x494             /* Forward Luma Frame Pointer */
/* This register holds the start address of the luma forward prediction frame picture buffer, defined in units */
/* of 512 bytes. */

/* ------------------------------------------------------------------------------------ */
#define VIDn_HDF                            0x500             /* Header Data FIFO Read */
/* When the start code detector has found a start code, the header data FIFO must be read in order to */
/* identify the start code and to read the header data. Before reading the header FIFO, status bit */
/* VIDn_STA.HFE or VIDn_STA.HFF must be read to ensure that the header FIFO is not empty. If the reg */
/* register is read while the header data is empty, the data in VIDn_HDF corresponds to the previous one. */
/* When this register is read, the data is updated with the next 16 bit word present in the bitstream. This */
/* register must be read only when the start code detector is in idle state, between a start code hit and a */
/* start code launch. */
#define VIDn_HDF_SCM_SC_on_MSB              0x00010000
#define VIDn_HDF_SCM_SC_on_MSB_SHIFT        16

#define VIDn_HDF_HEADER_DATA_MASK           0x0000FFFF

/* ------------------------------------------------------------------------------------ */
#define VIDn_HLNC                           0x504             /* Header Launch */

#define VIDn_HLNC_RTA                       0x08
#define VIDn_HLNC_SCNT_TS_ASS_FROZEN        0x04

#define VIDn_HLNC_LNC_MASK                  0x03
#define VIDn_HLNC_LNC_FROM_ADDRESS_LAUNCHED 0x02
#define VIDn_HLNC_LNC_NORMALLY_LAUNCHED     0x01

/* ------------------------------------------------------------------------------------ */
#define VIDn_ITM                            0x3F0             /* Interrupt Mask */
/* Any bit set in this register will enable the corresponding interrupt in VIDn_IRQ line. An interrupt is gen-erated */
/* whenever a bit in the VIDn_STA register changes from 0 to 1 and the corresponding mask bit is set. */

/* ------------------------------------------------------------------------------------ */
#define VIDn_ITS                            0x3F4             /* Interrupt Status */

/* ------------------------------------------------------------------------------------ */
#define VID_MBEx                            0x070             /* Macroblock error statistic (x=0,1,2,3) */
/* Decoded pictures is divided in 16 areas. For each area, MBE gives the number of macroblock errors */
/* (semantic and syntax) detected by the VLD. These data are stable at the end of picture decoding task */
/* until the start of the next picture decoding task (whatever the video channel 1 or 2). */

/* ------------------------------------------------------------------------------------ */
#define VIDn_MBNM                           0x480             /* Macroblock number for the main reconstruction */
/* This register holds the position of the macroblock to be reconstructed in the frame buffer for the main */
/* reconstruction. When no picture is being decoded, it gives the position of the macroblock following the */
/* last decoded macroblock for the previous decoded picture. */

/* ------------------------------------------------------------------------------------ */
#define VIDn_MBNS                           0x484             /* Macroblock number for the secondary reconstruction */
/* This register holds the position of the macroblock to be reconstructed in the frame buffer for the second */
/* reconstruction. When no picture is being decoded, it gives the position of the macroblock following the */
/* last decoded macroblock for the previous decoded picture. The value is stable between the end of a */
/* decoding task and the next PSD event. */
/* The first macroblock (macroblock 0) starts at row 0 and column 0. The column is incremented by one */
/* every 2 horizontal decimated macroblocks if horizontal decimation is 2 and every 4 decimated */
/* macroblocks if horizontal decimation is 4. The row is incremented by one at the end of each row of */
/* decimated MBs. */

/* ------------------------------------------------------------------------------------ */
#define VIDn_PES_ASS                        0x590             /* PES association */
/* This register indicates when time stamps or dsm flag are available for the picture detected by the start */
/* code detector. It contains a relevant value when VIDn_STA.SCH is set to 1 and the start code detected */
/* by the start code detector is a picture start code. The value is stable until the next same condition occurs */
/* (next picture start code). */
#define VIDn_PES_ASS_DSA                    0x00000002
#define VIDn_PES_ASS_TSA                    0x00000001

/* ------------------------------------------------------------------------------------ */
#define VIDn_PES_CFG                        0x580             /* PES Parser Configuration */
/* This register contains a set of bits to configure the pes parser. */
#define VIDn_PES_CFG_PTF_EN                 0x00000040

#define VIDn_PES_CFG_MODE_MASK              0x00000030
#define VIDn_PES_CFG_MODE_AUTOMATIC         0x00000000
#define VIDn_PES_CFG_MODE_MPEG1             0x00000010
#define VIDn_PES_CFG_MODE_MPEG2_PES         0x00000030

#define VIDn_PES_CFG_OFS_MASK               0x0000000C
#define VIDn_PES_CFG_OFS_PES                0x00000000
#define VIDn_PES_CFG_OFS_ES                 0x0000000C

#define VIDn_PES_CFG_DTS_STORE_EN           0x00000002
#define VIDn_PES_CFG_PARSER_EN              0x00000001

/* ------------------------------------------------------------------------------------ */
#define VIDn_PES_CTA                        0x5AC             /* Counter of TS association (debug) */

/* ------------------------------------------------------------------------------------ */
#define VIDn_PES_DSM                        0x594             /* DSM trick mode */
/* This register contains the DSM flags associated to the picture hit by the start code detector when */
/* VIDn_PES_ASS.DSA =1. The value is stable until the next picture start code hit. */

/* ------------------------------------------------------------------------------------ */
#define VIDn_PES_DSMC                       0x598             /* PES DSM counter (debug) */
/* This register contains the number of DSM data present in the TS FIFOs. */

/* ------------------------------------------------------------------------------------ */
#define VIDn_PES_FSI                        0x588             /* Filter PES Stream ID */
/* This register holds the value of the filter for the stream_id, allowing the transmission of the PES having */
/* several IDs, or disabling the ID filter when all bits are reset. The PES parser filters data and transmit */
/* them when the following equation is true (see VIDn_PES_SI): */
/* (Stream ID & VIDn_PES_FSI) = (VIDn_PES_SI & VIDn_PES_FSI). */

/* ------------------------------------------------------------------------------------ */
#define VIDn_PES_SI                         0x584             /* PES Stream Identification */
/* This register holds the value of the stream_id used to detect the PES packet which must be processed */
/* i.e from which the video elementary stream must be extracted (or pes packet transmitted). */

/* ------------------------------------------------------------------------------------ */
#define VIDn_PES_TFI                        0x58C             /* Trick filtering PES stream ID */
/* This register holds the stream_id used to recognize the last PES packet containing the last picture to be */
/* written in the bit buffer. It is used when VIDn_PES_CFG.PTF is set to 1 */

/* ------------------------------------------------------------------------------------ */
#define VIDn_PES_TS_TS_32                   0x5A4             /* PES Time Stamp TS[32] */
#define VIDn_PES_TS_TS_31_0                 0x5A0             /* PES Time Stamp TS[31..0] */
/* These registers store the time stamps selected using the control bit in VIDn_PES_CFG.SDT. They give */
/* the time stamp value of the picture hit by the start code detector when VIDn_PES_ASS.TSA = 1. There */
/* are stable until the next picture start code hit. */

/* ------------------------------------------------------------------------------------ */
#define VIDn_PES_TSC                        0x5A8             /* PES Time Stamp counter (debug) */
/* This register holds the number of time stamps recorded in the TS FIFOs. */

/* ------------------------------------------------------------------------------------ */
#define VIDn_PMA                            0x32C             /* Panic maximum */
/* The 24 bits value hold by this register represents the maximum number of processing clock cycles the */
/* pipe is inactive during the decoding of one.It may be used for performance analysis in the system. It is */
/* updated at the end of the decoding. */

/* ------------------------------------------------------------------------------------ */
#define VIDn_PPR                             0x304             /* Picture Parameters */
/* This register contains parameters of the picture to be decoded. These parameters are extracted from */
/* the bitstream. In MPEG-1 mode (i.e. when VIDn_TIS.MP2 is reset), only PCT, BFH and FFH have to be */
/* set. Other bits must be reset. */
#define VIDn_PPR_FFN_MASK                    0x30000000
#define VIDn_PPR_FFN_FIRST_FIELD             0x10000000
#define VIDn_PPR_FFN_SCND_FIELD              0x30000000
#define VIDn_PPR_FFN_INTERNAL                0x00000000

#define VIDn_PPR_TFF                         0x08000000
#define VIDn_PPR_TFF_SHIFT                   27
#define VIDn_PPR_FRM                         0x04000000
#define VIDn_PPR_FRM_SHIFT                   26
#define VIDn_PPR_CMV                         0x02000000
#define VIDn_PPR_CMV_SHIFT                   25
#define VIDn_PPR_QST                         0x01000000
#define VIDn_PPR_QST_SHIFT                   24
#define VIDn_PPR_IVF                         0x00800000
#define VIDn_PPR_IVF_SHIFT                   23
#define VIDn_PPR_AZZ                         0x00400000
#define VIDn_PPR_AZZ_SHIFT                   22

#define VIDn_PPR_PCT_MASK                    0x00300000
#define VIDn_PPR_PCT_SHIFT                   20
#define VIDn_PPR_DCP_MASK                    0x000C0000
#define VIDn_PPR_DCP_SHIFT                   18

#define VIDn_PPR_PST_MASK                    0x00030000
#define VIDn_PPR_PST_SHIFT                   16

#define VIDn_PPR_BFV_MASK                    0x0000F000
#define VIDn_PPR_BFV_SHIFT                   12
#define VIDn_PPR_FFV_MASK                    0x00000F00
#define VIDn_PPR_FFV_SHIFT                   8
#define VIDn_PPR_BFH_MASK                    0x000000F0
#define VIDn_PPR_BFH_SHIFT                   4
#define VIDn_PPR_FFH_MASK                    0x0000000F

/* ------------------------------------------------------------------------------------ */
#define VIDn_PRS                            0x308             /* Decoding Pipeline Reset */
/* Writing to the least significant byte of this register starts the pipeline reset */
/* sequence for the corresponding video channel. The reset is just activated once on writing. */
#define VIDn_PRS_PIPELINE_RESET             0x000000FF

/* ------------------------------------------------------------------------------------ */
#define VIDn_PSCPTR                         0x514             /* Picture Start Code pointer */

/* ------------------------------------------------------------------------------------ */
#define VIDn_PTI                            0x324             /* Panic Time */
/* The 24 bit value hold by this register sums the number of processing clock cycles the decoder is inactive */
/* once the threshold value defined by the VIDn_THI is reached. The addition is performed during the */
/* whole decoding process of a picture. It may be used for performance analysis in the system. It is updated */
/* at the end of the decoding. */


/* ------------------------------------------------------------------------------------ */
#define VIDn_RCHP                           0x498             /* Main Reconstructed Chroma Frame Pointer */
/* This register holds the start address of the reconstructed chroma frame picture buffer, defined in units of */
/* 512 bytes. */

/* ------------------------------------------------------------------------------------ */
#define VID_REQC                            0x044             /* Request COUNTER */
/* This register indicates if PES, SCD, VLD and PRD are in an idle state, from STBus transaction point of */
/* vue. If a bit is set, all the transactions asked by the adhoc block are completed in memory. */

/* ------------------------------------------------------------------------------------ */
#define VID_REQS                            0x40              /* Request State */
/* This register gives the request state of each process before and after internal mask is applied. Internal */
/* masks may be due to functional behaviour (bit buffer empty for example */

/* ------------------------------------------------------------------------------------ */
#define VIDn_RFP                            0x49C             /* Main Reconstructed Luma Frame Pointer */
/* This register holds the start address of the reconstructed (decoded) luma picture buffer, defined in units */
/* of 512 bytes. */

/* ------------------------------------------------------------------------------------ */
#define VIDn_SCD                            0x508             /*  Start Codes Disabled. */
#define VIDn_SCD_MASK                       0x7FF
/* This register indicates which start codes must not be detected by the start code detector. */
#define VIDn_SCD_RSC_DISCARD_EN             0x00000400
#define VIDn_SCD_SYSC_DISCARD_EN            0x00000200
#define VIDn_SCD_GSC_DISCARD_EN             0x00000100
#define VIDn_SCD_SENC_DISCARD_EN            0x00000080
#define VIDn_SCD_ESC_DISCARD_EN             0x00000040
#define VIDn_SCD_SEC_DISCARD_EN             0x00000020
#define VIDn_SCD_SHC_DISCARD_EN             0x00000010
#define VIDn_SCD_UDSC_DISCARD_EN            0x00000008
#define VIDn_SCD_SSCF_DISCARD_EN            0x00000004
#define VIDn_SCD_SSC_DISCARD_EN             0x00000002
#define VIDn_SCD_PSC_DISCARD_EN             0x00000001

/* ------------------------------------------------------------------------------------ */
#define VIDn_SCDB                           0x434             /* Start Code Detector Buffer */
/* This register contains a set of bit to configure the buffer read by the start code detector.The reset value */
/* corresponds to a normal mode and guaranty that no transaction occurs on T2_VD after the hardware reset */
#define VIDn_SCDB_MASK                      0xFFFFFFF8
#define VIDn_SCDB_PRNW_EN                   0x00000004
#define VIDn_SCDB_RLE_EN                    0x00000002
#define VIDn_SCDB_CM_EN                     0x00000001


/* ------------------------------------------------------------------------------------ */
#define VIDn_SCDC                           0x5B0             /* Bit Buffer Output Counter */
/* This register holds the number of 16-bit words processed by the start code detector (processed by its in-ternal */
/* state machine and so doesnt include the last one exited from its FIFO) at the output of the bit buff-er */
/* when VIDn_HLNC.SCNT = 0. The value of this counter is used internally for TS association purpose. */
/* SCD_COUNT does not include the data present in the header FIFO. */
#define VIDn_SCDC_MASK                      0x01FFFFFE        /* Register mask */
#define VIDn_SCDC_BITs                      24                /* Value is on 24 bits */
#define VIDn_SCDC_SHIFT                     1                 /* Shift the value because the first byte is reserved */

/* ------------------------------------------------------------------------------------ */
#define VIDn_SCDC_TM                        0x50C             /* Bit Buffer Input Counter for trick mode (debug) */
/* This register holds the number of 16-bit words processed by the start code detector at the output from */
/* the bit buffer when no time stamp association is running (trick mode). */

/* ------------------------------------------------------------------------------------ */
#define VIDn_SCDPTR                         0x518             /* Start code detector Pointer */
/* This register holds the memory address pointer, in byte, starting point of Start Code search */

/* ------------------------------------------------------------------------------------ */
#define VIDn_SCDPTR_CUR                     0x43C             /* Start Code Detector Pointer (debug) */
/* This register holds the current value of the start code detector pointer, in 128 byte unit. */

/* ------------------------------------------------------------------------------------ */
#define VIDn_SCDRL                          0x438             /* Start Code Detector read limit. */
/* This register stores the start code detector read limit. It is taken into account when VIDn_SCDB.SCDRL */
/* is high. When the SCD read pointer reaches VIDn_SCDRL, reading in the buffer is stopped and the bit */
/* SCDRL is set in the status register. This pointer is defined in 128 bytes unit. */
/* When an SCD update pointer is loaded (VIDn_SCDUP.UP = 1) and VIDn_SCDB.RLE = 1 (read limit */
/* is on, compulsory to have correct behaviour for the IP), no data are read in the bit buffer until a */
/* different read limit is written in the register. It may be considered as an exe. */
/* Please refer to Trick Modes chapter to apply correct procedure. */

/* ------------------------------------------------------------------------------------ */
#define VIDn_SCDUP                          0x44C             /* Update Start Code Pointer */
/* Writing 1 in this register updated the start code detector read pointer with VIDn_SCDUPTR. It has no */
/* effect on start code detection process. We consider in the case that there is a discontinuity in the */
/* memory space read by the start code detector but no continuity in the input stream. No instruction is */
/* executed when 0 is written in this register. When reading this register, value 0 is always returned. */
#define VIDn_SCDUP_UP                       0x00000001

/* ------------------------------------------------------------------------------------ */
#define VIDn_SCDUPTR                        0x450             /* Start Code detector Update Pointer. */
/* This pointer, defined in 128 bytes unit is loaded in the start code detector read pointer when */
/* VIDn_SCDUP.UP is set to 1. This pointer may be used when the location of the bit buffer must be */
/* changed while there is no discontinuity in the input stream. */
/* This register must be updated only when the start code detector is not reading data in the bit buffer, */
/* when the SCD read limit is reached. */

/* ------------------------------------------------------------------------------------ */
#define VIDn_SCPTR                          0x510             /* Start Code Pointer */
/* This register stores the address of the first byte of the start code detected by the start code detector. It */
/* is given in byte unit. It is updated when VIDn_STA.SCH = 1 and is stable until the next same condition. */
/* If a picture start code is detected, the data is the identical to VIDn_PSCPTR. */

/* ------------------------------------------------------------------------------------ */
#define VIDn_SDMOD                          0x4AC             /* Secondary Decoding Memory Mode */
/* This register indicates how the second reconstruction must be done. */
#define VIDn_SDMOD_MASK                     0x0000007B

#define VIDn_SDMOD_PS_EN                    0x00000001
#define VIDn_SDMOD_2nd_REC_EN               0x00000002

#define VIDn_SDMOD_HDEC_NONE                0x00000000
#define VIDn_SDMOD_HDEC_H2                  0x00000008
#define VIDn_SDMOD_HDEC_H4                  0x00000010

#define VIDn_SDMOD_VDEC_NONE                0x00000000
#define VIDn_SDMOD_VDEC_V2                  0x00000020
#define VIDn_SDMOD_VDEC_V4                  0x00000040



/* ------------------------------------------------------------------------------------ */
#define VIDn_SRCHP                          0x4A0             /* Secondary reconstructed Chroma Frame Buffer */
/* This register holds the start address of the secondary reconstructed chroma frame buffer, defined in */
/* units of 512 bytes. */

/* ------------------------------------------------------------------------------------ */
#define VIDn_SRFP                           0x4A4             /* Secondary reconstructed Luma Frame Buffer */

/* ------------------------------------------------------------------------------------ */
#define VIDn_SRS                            0x30C             /* Decoding Soft Reset */
/* Writing to the least significant byte of this register starts the software reset */
/* sequence for the corresponding video channel. The reset is just activated once on writing. */
/* For a clean behaviour of the IP, the application must ensure that no compressed data are sent to the IP */
/* when the reset is activated. If compressed data are sent when the reset is performed, It could be that */
/* some remaining bytes from the previous stream are taken into account when the reset becomes inactive. */
#define VIDn_SRS_SOFT_RESET                 0x000000FF


/* ------------------------------------------------------------------------------------ */
#define VIDn_STA                            0x3F8             /* Status Register */
/* This register contains a set of bits which represent the status of the decoder at any instant. Any change */
/* from 0 to 1 of any of these bits sets the corresponding bit of the VIDn_ITS register, and can thus potentially */
/* cause an interrupt on go2_IRQn line. Some bits are pulses and are unlikely ever to be read as a 0. */
#define VIDn_STA_MASK                       0x027FFFDF
#define VIDn_ITX_SRR                        0x02000000
#define VIDn_ITX_R_OPC                      0x00400000
#define VIDn_ITX_VLDRL                      0x00200000
#define VIDn_ITX_SCDRL                      0x00100000
#define VIDn_ITX_CDWL                       0x00080000
#define VIDn_ITX_SBE                        0x00040000
#define VIDn_ITX_BBE                        0x00020000
#define VIDn_ITX_BBNE                       0x00010000
#define VIDn_ITX_BBF                        0x00008000
#define VIDn_ITX_PAN                        0x00004000
#define VIDn_ITX_DSE                        0x00002000
#define VIDn_ITX_MSE                        0x00001000
#define VIDn_ITX_DUE                        0x00000800
#define VIDn_ITX_DOE                        0x00000400
#define VIDn_ITX_OTE                        0x00000200
#define VIDn_ITX_DID                        0x00000100
#define VIDn_ITX_PID                        0x00000080
#define VIDn_ITX_PSD                        0x00000040
#define VIDn_ITX_BFF                        0x00000010
#define VIDn_ITX_IER                        0x00000008
#define VIDn_ITX_HFE                        0x00000004
#define VIDn_ITX_HFF                        0x00000002
#define VIDn_ITX_SCH                        0x00000001

/* ------------------------------------------------------------------------------------ */
#define VIDn_THI                            0x320             /* Threshold inactive */
/* This 16 bit register gives, for the reference period T hold in CFG_THP, the number of processing clock */
/* cycles R, when working at the threshold speed, the decoder is inactive. Thus, the threshold speed is */
/* defined by: THS = (T - R)/T. */

/* ------------------------------------------------------------------------------------ */
#define VIDn_TIS                            0x300             /* Task Instruction */
/* This register contains the decoding task instruction. */
#define VIDn_TIS_STD                        0x00000800
#define VIDn_TIS_EOD                        0x00000400
#define VIDn_TIS_SPD                        0x00000200
#define VIDn_TIS_SCN_EN                     0x00000100
#define VIDn_TIS_LDP_EN                     0x00000080
#define VIDn_TIS_LFR_EN                     0x00000040
#define VIDn_TIS_DPR_EN                     0x00000020
#define VIDn_TIS_MP2_MPEG2_EN               0x00000010

#define VIDn_TIS_SKP_MASK                   0x0000000C
#define VIDn_TIS_SKP_NO_SKIP                0x00000000
#define VIDn_TIS_SKP_SK_1_PIC_DEC_NXT       0x00000004
#define VIDn_TIS_SKP_SK_2_PIC_DEC_NXT       0x00000008
#define VIDn_TIS_SKP_SK_1_PIC_STOP          0x0000000C

#define VIDn_TIS_FIS_EN                     0x00000002
#define VIDn_TIS_EXE                        0x00000001

/* ------------------------------------------------------------------------------------ */
#define VIDn_TRA                            0x328             /* Panic trans */
/* This 16 bit register gives the number of times, during the decoding of one picture, the decoder reaches */
/* the panic threshold defined by VIDn_THI. (i.e the number of times the interrupt panic is raised). It is */
/* updated at the end of the decoding. */

/* ------------------------------------------------------------------------------------ */
#define VIDn_VLDB                           0x440             /* VLD buffer */
/* This register holds a set of bit for VLD buffer configuration. The reset value corresponds to a normal */
/* mode and guaranty that no transaction occurs on T2_VD after the hardware reset. */
#define VIDn_VLDB_PRNW_EN                   0x00000004
#define VIDn_VLDB_RLE_EN                    0x00000002
#define VIDn_VLDB_CM_EN                     0x00000001


/* ------------------------------------------------------------------------------------ */
#define VIDn_VLDPTR                         0x45C             /* VLD start Pointer */
/* Memory address pointer, in byte, starting point of decoding (VLD read pointer) when VIDn_TIS.LDP and */
/* VIDn_TIS.LFR are both set to 1. */

/* ------------------------------------------------------------------------------------ */
#define VIDn_VLDPTR_CUR                     0x444             /* VLD read Pointer (debug) */
/* VLD read pointer given in 256 bytes unit. */

/* ------------------------------------------------------------------------------------ */
#define VIDn_VLDRL                          0x448             /* VLD read limit. */
/* This register stores the VLD read limit. It is taken into account only when VIDn_VLDB.VLDRL = 1. When */
/* the VLD read pointer reaches VIDn_VLDRL, reading in the buffer is stopped and the bit VLDRL is set to */
/* 1 in the status register. This pointer is defined in 256 bytes unit. */
/* When a VLD update pointer is loaded (VIDn_VLDUP.UP = 1) and the read limit is on */
/* (VIDn_VLDB.RLE = 1, compulsory to have correct behaviour for the IP), no data are read in the */
/* bit buffer until a different VLD read limit is written. Writing a limit may be considered as an exe */
/* to update a pointer. */

/* ------------------------------------------------------------------------------------ */
#define VIDn_VLDUP                          0x454             /* Update VLD Pointer */
/* Writing 1 in UP load the vld read pointer with the value defined in VIDn_VLDUPTR. This bit has no effect */
/* on VLD processing. We consider in the case that there is a discontinuity in the memory space read by */
/* the VLD but no discontinuity in the input stream. Writing 0 to this bit has no effect. When reading this */
/* register, value 0 is always returned. */

/* ------------------------------------------------------------------------------------ */
#define VIDn_VLDUPTR                        0x458             /* VLD Pointer to be updated */
/* This pointer, defined in 256 bytes unit defines the start of the VLD read pointer. It is loaded when */
/* VIDn_VLDUP.UP = 1. This pointer may be used when the location of the bit buffer must be changed */
/* while there is no discontinuity in the input stream. */
/* When a VLD update pointer is loaded (VIDn_VLDUP.UP = 1) and VIDn_VLDB.RLE = 1 (read limit */
/* is on, compulsory to have correct behaviour for the IP), no data are read in the bit buffer until a */
/* different read limit is written in the register. It may be considered as an exe. */

/* ------------------------------------------------------------------------------------ */
#define VID_VMBN                            0x6C              /* VLD Macroblock number (debug) */
/* This register holds the number of the macroblock being decoded by the VLD. It is stable at the end of the */
/* decoding task until start of next picture decoding task. */

/* ------------------------------------------------------------------------------------ */
#define VID_VLDS                            0x68              /* VLD state (debug). */
/* This register holds the latest start code found by the VLD. It is stable at the end of the decoding task until */
/* start of next picture decoding task. It allows to know on which start code the VLD stops parsing the video */
/* elementary stream. */

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __DECODE_REGISTERS_SDMPGO2_H */

/* End of hvr_dec8.h */
