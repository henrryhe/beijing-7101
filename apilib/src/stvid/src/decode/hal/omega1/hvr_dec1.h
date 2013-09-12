/*******************************************************************************

File name   : hvr_dec1.h

Description : Video decoder module HAL registers for omega 1 header file

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
06 Jan 2000        Created                                          HG
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __DECODE_REGISTERS_OMEGA1_H
#define __DECODE_REGISTERS_OMEGA1_H


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"


/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */

/* VID - video header search, decoding, display  registers    ------------- */
#define VID_CTL         0x02  /* control of the video decoder  [7:0]            */
#define VID_TIS         0x03  /* Task instruction for video pipeline [5:0]      */
#define VID_PFH         0x04  /* Picture F parameters vertical [7:0]            */
#define VID_PFV         0x05  /* Picture F parameters horizontal [7:0]          */
#define VID_PPR1        0x06  /* Picture parameters 1 [6:0]                     */
#define VID_PPR2        0x07  /* Picture parameters 2 [6:0]                     */
#define VID_DFP1        0x0C  /* Display Frame pointer [13:8] luma              */
#define VID_DFP0        0x0D  /* Display Frame pointer [7:0] luma               */
#define VID_RFP1        0x0E  /* Reconstructed Frame pointer [13:8] luma        */
#define VID_RFP0        0x0F  /* Reconstructed Frame pointer [7:0] luma         */
#define VID_FFP1        0x10  /* Forward Frame pointer [13:8] luma              */
#define VID_FFP0        0x11  /* Forward Frame pointer [7:0] luma               */
#define VID_BFP1        0x12  /* Backward Frame pointer [13:8] luma             */
#define VID_BFP0        0x13  /* Backward Frame pointer [7:0] luma              */
#define VID_VBG1        0x14  /* Video Bit buffer Start [13:8]                  */
#define VID_VBG0        0x15  /* Video Bit Buffer Start [7:0]                   */
#define VID_VBL1        0x16  /* Video Bit buffer Level [13:8]                  */
#define VID_VBL0        0x17  /* Video Bit Buffer Level [7:0]                   */
#define VID_VBS1        0x18  /* Video Bit buffer Stop [13:8]                   */
#define VID_VBS0        0x19  /* Video Bit Buffer Stop [7:0]                    */
#define VID_VBT1        0x1A  /* Video Bit buffer Threshold [13:8]              */
#define VID_VBT0        0x1B  /* Video Bit Buffer Threshold [7:0]               */
#define VID_ABG1        0x1C  /* Video Bit buffer Start [13:8]                  */
#define VID_ABG0        0x1D  /* Audio Bit Buffer Start [7:0]                   */
#define VID_ABL1        0x1E  /* Audio Bit buffer Level [13:8]                  */
#define VID_ABL0        0x1F  /* Audio Bit Buffer Level [7:0]                   */
#define VID_ABS1        0x20  /* Audio Bit buffer Stop [13:8]                   */
#define VID_ABS0        0x21  /* Audio Bit Buffer Stop [7:0]                    */
#define VID_ABT1        0x22  /* Audio Bit buffer Threshold [13:8]              */
#define VID_ABT0        0x23  /* Audio Bit Buffer Threshold [7:0]               */
#define VID_DFS         0x24  /* Video decoded frame  size, cyclic [14:0]       */
#define VID_DFW         0x25  /* Video decoded frame width, [7:0]               */
#define VID_XFW         0x28  /* Video displayed frame width, [7:0]             */
#define VID_OTP         0x2A  /* Video OSD top pointer, cyclic [13:0]           */
#define VID_OBP         0x2B  /* Video OSD bottom pointer, cyclic [13:0]        */
#define VID_PAN1        0x2C  /* Video Pan display [10:8]                       */
#define VID_PAN0        0x2D  /* Video Pan display [7:0]                        */
#define VID_PTH1        0x2E  /* Video Panic Threshold [13:8]                   */
#define VID_PTH0        0x2F  /* Video Panic Threshold [7:0]                    */
#define VID_STL         0x30  /* SCD Trick Mode Launch                          */
#define VID_CWL         0x31  /* CD write launch                                */
#define VID_SRA         0x35  /* Audio Soft reset [0]                           */
#define VID_SRV         0x39  /* Video Soft reset [0]                           */
#define VID_STA2        0x3B  /* Video status of interrupts [23:16]             */
#define VID_ITM2        0x3C  /* Video interrupt mask [23:16]                   */
#define VID_ITS2        0x3D  /* Video interrupt status [23:16]                 */
#define VID_LDP         0x3F  /* Video Load internal pointer [0] : 5510/12      */
#define VID_TP_LDP      0x3F  /* Video Load internal pointer [2:0] : 5508/18    */
#define VID_YDS1        0x46  /* Video Display Stop [8]                         */
#define VID_YDS0        0x47  /* Video Display Stop [7:0]                       */
#define VID_SPR         0x4E  /* Sub picture Read pointer , 3 cycles [18:0]     */
#define VID_SPW         0x4F  /* Sub picture write pointer, 3 cycles [18:0]     */
#define VID_SPB1        0x50  /* Sub picture buffer start [10:8]                */
#define VID_SPB0        0x51  /* Sub picture buffer start [7:0]                 */
#define VID_SPE1        0x52  /* Sub picture buffer stop [10:8]                 */
#define VID_SPE0        0x53  /* Sub picture buffer stop [7:0]                  */
#define VID_VFL         0x54  /* Video vertical filter for display [4:0] luma   */
#define VID_VFC         0x55  /* Video vertical filter for display [4:0] chroma */
#define VID_TRF1        0x56  /* Video temporal reference [11:8]                */
#define VID_TRF0        0x57  /* Video temporal reference [7:0]                 */
#define VID_DFC1        0x58  /* Display Frame pointer [13:8] chroma            */
#define VID_DFC0        0x59  /* Display Frame pointer [7:0]  chroma            */
#define VID_RFC1        0x5A  /* Reconstructed Frame pointer [13:8] chroma      */
#define VID_RFC0        0x5B  /* Reconstructed Frame pointer [7:0]  chroma      */
#define VID_FFC1        0x5C  /* Forward Frame pointer [13:8] chroma            */
#define VID_FFC0        0x5D  /* Forward Frame pointer [7:0]  chroma            */
#define VID_BFC1        0x5E  /* Backward Frame pointer [13:8] chroma           */
#define VID_BFC0        0x5F  /* Backward Frame pointer [7:0] chroma            */
#define VID_ITM1        0x60  /* Video interrupt mask [15:8]                    */
#define VID_ITM0        0x61  /* Video interrupt mask [7:0]                     */
#define VID_ITS1        0x62  /* Video interrupt status [15:8]                  */
#define VID_ITS0        0x63  /* Video interrupt status [7:0]                   */
#define VID_STA1        0x64  /* Video status of interrupts [15:8]              */
#define VID_STA0        0x65  /* Video status of interrupts [7:0]               */
#define VID_HDF         0x66  /* Video Header search , 2 cycles [15:0]          */
#define VID_CD_COUNT    0x67  /* Video CD counter of FIFO, 3 cycles [24:0]      */
#define VID_SCD_COUNT   0x68  /* Video SCD counter of FIFO, 3 cycles [24:0]     */
#define VID_HDS         0x69  /* Video header Search control [3:0]              */
#define VID_LSO         0x6A  /* Video SRC luma sub pixel offset [7:0]          */
#define VID_LSR0        0x6B  /* Video LUMA and CHROMA SRC control [7:0]        */
#define VID_CSO         0x6C  /* Video SRC chroma sub pixel offset [7:0]        */
#define VID_LSR1        0x6D  /* Video LUMA and CHROMA SRC control [8]          */
#define VID_YDO1        0x6E  /* Video Y axis Start [8]                         */
#define VID_YDO0        0x6F  /* Video Y axis Start [7:0]                       */
#define VID_XDO1        0x70  /* Video X axis Start [9:8]                       */
#define VID_XDO0        0x71  /* Video X axis Start [7:0]                       */
#define VID_XDS1        0x72  /* Video X axis Stop  [9:8]                       */
#define VID_XDS0        0x73  /* Video X axis Stop  [7:0]                       */
#define VID_DCF1        0x74  /* Video Display Configuration [13:8]             */
#define VID_DCF0        0x75  /* Video Display Configuration [7:0]              */
#define VID_QMW         0x76  /* Video Quantization matrix [7:0]                */
#define VID_TST         0x77  /* Video test register                            */
#define VID_REV         0x78  /* Video revision register                        */
#define VID_LCK         0x7B  /* Def for STi5512                                */
#define VID_SCN         0x87  /* Video Scan vector [5:0] in rows of macroblocs  */
#define VID_OUT         0x90  /* Video output gain [3:0]                        */
#define VID_MWV         0x9B  /* Video Mixing weight factor [7:0]               */
#define VID_MWS         0x9C  /* Still picture mixing weight [7:0]              */
#define VID_MWSV        0x9D  /* Still/Video picture mixing weight [7:0]        */
#define VID_DIS         0xD4  /* Sti5512 Priority management, zoom-out x2, FBS  */
#define VID_VFCMODE4    0xEA  /* Configure chrominance of the block-row : 5508  */
#define VID_VFCMODE3    0xEB  /* Configure chrominance of the block-row : 5508  */
#define VID_VFCMODE2    0xEC  /* Configure chrominance of the block-row : 5508  */
#define VID_VFCMODE1    0xED  /* Configure chrominance of the block-row : 5508  */
#define VID_VFCMODE0    0xEE  /* Configure chrominance of the block-row : 5508  */
#define VID_VFLMODE4    0xEF  /* Configure luminance of the block-row : 5508    */
#define VID_VFLMODE3    0xF0  /* Configure luminance of the block-row : 5508    */
#define VID_VFLMODE2    0xF1  /* Configure luminance of the block-row : 5508    */
#define VID_VFLMODE1    0xF2  /* Configure luminance of the block-row : 5508    */
#define VID_VFLMODE0    0xF3  /* Configure luminance of the block-row : 5508    */


/* VID - Video registers specific 16 bit calls ---------------------------- */
#define VID_DFP16       0x0C  /* Display Frame pointer [13:0] luma              */
#define VID_RFP16       0x0E  /* Reconstructed Frame pointer [13:0] luma        */
#define VID_FFP16       0x10  /* Forward Frame pointer [13:0] luma              */
#define VID_BFP16       0x12  /* Backward Frame pointer [13:0] luma             */
#define VID_VBG16       0x14  /* Video Bit buffer Start [13:0]                  */
#define VID_VBL16       0x16  /* Video Bit buffer Level [13:0]                  */
#define VID_VBS16       0x18  /* Video Bit buffer Stop [13:0]                   */
#define VID_VBT16       0x1A  /* Video Bit buffer Threshold [13:0]              */
#define VID_ABG16       0x1C  /* Video Bit buffer Start [13:0]                  */
#define VID_ABL16       0x1E  /* Audio Bit buffer Level [13:0]                  */
#define VID_ABS16       0x20  /* Audio Bit buffer Stop [13:0]                   */
#define VID_ABT16       0x22  /* Audio Bit buffer Threshold [13:0]              */
#define VID_PAN16       0x2C  /* Video Pan display [10:0]                       */
#define VID_PTH16       0x2E  /* Video Panic Threshold [13:0]                   */
#define VID_YDS16       0x46  /* Video Display Stop [8:0]                       */
#define VID_SPB16       0x50  /* Sub picture buffer start [10:0]                */
#define VID_SPE16       0x52  /* Sub picture buffer stop [10:0]                 */
#define VID_TRF16       0x56  /* Video temporal reference [12:0]                */
#define VID_DFC16       0x58  /* Display Frame pointer [13:0] chroma            */
#define VID_RFC16       0x5A  /* Reconstructed Frame pointer [13:0] chroma      */
#define VID_FFC16       0x5C  /* Forward Frame pointer [13:0] chroma            */
#define VID_BFC16       0x5E  /* Backward Frame pointer [13:0] chroma           */
#define VID_ITM16       0x60  /* Video interrupt mask [15:0]                    */
#define VID_ITS16       0x62  /* Video interrupt status [15:0]                  */
#define VID_STA16       0x64  /* Video status of interrupts [15:0]              */
#define VID_YDO16       0x6E  /* Video Y axis Start [8:0]                       */
#define VID_XDO16       0x70  /* Video X axis Start [9:0]                       */
#define VID_XDS16       0x72  /* Video X axis Stop  [9:0]                       */
#define VID_DCF16       0x74  /* Video Display Configuration [13:0]             */
#define VID_TP_SCD_RD24 0x80  /* SCD pointer for the VLD load address [16:0]    */
                              /* 5516, 5517 */
#define VID_TP_SCD_RD16 0x81  /* SCD pointer for the VLD load address [15:0]    */
                              /* 5508, 5518, 5514 */
#define VID_TP_VLD24    0xD3  /* VLD pointer load address [16:0]                */
                              /* 5516, 5517 */
#define VID_TP_VLD16    0xD4  /* VLD pointer load address [15:0]                */
                              /* 5508, 5518, 5514 */


/* VID - Video registers specific 24 bit calls ---------------------------- */
#define VID_TP_CD_RD24       0x83 /* CD pointer current load address [19:0]     */
#define VID_TP_SCD24         0xC0 /* SCD pointer load address [16:0]            */
#define VID_TP_VLD_RD24      0xC3 /* VCD pointer current load address [19:0]    */
#define VID_TP_CD24          0xCA /* CD pointer load address [19:0]             */
#define VID_TP_SCD_CURRENT24 0xCD /* SCD pointer current load address [19:0]    */
#define VID_TP_CDLIMIT24     0xE0 /* CD write limit address [16:0]              */

/* VID - Video registers specific circular 16 bits calls ------------------ */
#define VID_DFS2C       0x24  /* Video display frame pointer, 2 cycles [14:0]   */
#define VID_OTP2C       0x2A  /* Video top OSD pointer , 2 cycles               */
#define VID_OBP2C       0x2B  /* Video bottom OSD pointer, 2 cycles             */
#define VID_HDF2C       0x66  /* Video Header FIFO, 2 cycles                    */


/* VID - Video registers specific circular 24 bits calls ------------------ */
#define VID_SPR3C       0x4E  /* Sub picture read pointer [18:0]                */
#define VID_SPW3C       0x4F  /* Sub picture Write pointer [18:0]               */
#define VID_CD3C        0x67  /* Video CD count input FIFO [24:0]               */
#define VID_SCD3C       0x68  /* Video SCD count output to Header search [24:0] */



#define VID_MAX_PAN     0x5FF
#define VID_MAX_SCN     0x3F
#define VID_MAX_SCN_YDO 0x1F  /* STi5512                                        */

/* VID_CTL - Control (8-bit) ---------------------------------------------- */
#define VID_CTL_EDC     0x01 /* enable video decoding                         */
#define VID_CTL_SRS     0x02 /* video software reset ?????????                */
#define VID_CTL_PRS     0x04 /* pipeline reset                                */
#define VID_CTL_ERS     0x40 /* enable pipeline reset on severe error         */
#define VID_CTL_ERU     0x80 /* enable pipeline reset on picture decode error */

/* VID_TIS - Task Instruction (8-bit) ------------------------------------- */
#define VID_TIS_SKIP          0x30 /* skip modempeg2 mode                   */
#define VID_TIS_OVW           0x08 /* Overwrite Mode                        */
#define VID_TIS_FIS           0x04 /* Force Instruction                     */
#define VID_TIS_RPT           0x02 /* Reapeat Mode                          */
#define VID_TIS_EXE           0x01 /* Execute the next Task                 */
#define VID_TIS_SKIP_MASK     0xCF /* skip bit flags position               */
#define NO_SKIP               0x00 /* Dont skip any picture                 */
#define SKIP_ONE_DECODE_NEXT  0x10 /* skip one pict and decode next pict    */
#define SKIP_TWO_DECODE_NEXT  0x20 /* skip two pict and decode next pict    */
#define SKIP_ONE_STOP_DECODE  0x30 /* skip one pict and stop decoder        */

/* VID_PFH - Picture F-parameters Horizontal (8-bit) ---------------------- */
#define VID_PFH_BFH           0xF0 /* (4) backward_horz_f_code              */
#define VID_PFH_FFH           0x0F /* (4) forward_horz_f_code               */
#define VID_PFH_BFH_SHIFT     0x04 /* backward_horz_f_code position         */
#define VID_PFH_FULL_PEL_VECTOR 0x8

/* VID_PFV - Picture F-parameters Horizontal (8-bit) ---------------------- */
#define VID_PFV_BFV           0xF0 /* (4) backward_vert_f_code              */
#define VID_PFV_FFV           0x0F /* (4) forward_vert_f_code               */
#define VID_PFV_BFV_SHIFT     0x04 /* backward_vert_f_code position         */

/* VID_PPR1 - Picture Parameters 1 (8-bit) -------------------------------- */
#define VID_PPR1_OTF          0x40 /* Enable On The FLY                     */
#define VID_PPR1_PCT          0x30 /* (2) pict_coding_type                  */
#define VID_PPR1_DCP          0x0C /* (2) intra_dc_precision                */
#define VID_PPR1_PST          0x03 /* (2) pict_struct                       */
#define VID_PPR1_TOP_FIELD    0x01 /* PST = 1 Top Field                     */
#define VID_PPR1_BOT_FIELD    0x02 /* PST = 2 Bottom field                  */
#define VID_PPR1_FRAME        0x03 /* PST = 3 Frame picture                 */
#define VID_PPR1_PCT_SHIFT    0x04 /* PCT position                          */

/* VID_PPR2 - Picture Parameters 2 (8-bit) -------------------------------- */
#define VID_PPR2_MP2          0x40 /* MPEG2 mode                            */
#define VID_PPR2_TFF          0x20 /* Top Field First                       */
#define VID_PPR2_FRM          0x10 /* frame_pred_frame_dct                  */
#define VID_PPR2_CMV          0x08 /* concealment_motion_vectors            */
#define VID_PPR2_QST          0x04 /* q_scale_type                          */
#define VID_PPR2_IVF          0x02 /* intra_vlc_format                      */
#define VID_PPR2_AZZ          0x01 /* alternate_scan                        */

/* VID_PTH - Panic control register ( 16 bits ) --------------------------- */
#define VID_PTH_FPAN          0x2000 /* Force panic mode ( debug )          */
#define VID_PTH_PEN           0x1000 /* Enable panic mode                   */
#define VID_PTH_NFW           0x0800 /* Near forward, concealment mode      */
#define VID_PTH_MASK          0x03FF /* Panic threshold mask                */

/* VID_SRA - Soft reset audio ( 1 bit ) ----------------------------------- */
#define VID_SRA_SET           0x01  /* set soft reset audio                 */
#define VID_SRA_RESET         0x00  /* reset soft reset audio               */

/* VID_SRV - Soft reset video ( 1 bit ) ----------------------------------- */
#define VID_SRV_SET           0x01  /* set soft reset video                 */
#define VID_SRV_RESET         0x00  /* reset soft reset video               */

/* VID_DFS - enable the cif mode in dfs ----------------------------------- */
#define VID_DFS_CIF           0x4000

/* VID_STL - SCD Trick Mode Launch (2-bits) ------------------------------- */
#define VID_STL_STL_SET       0x01 /* Launch Start Code Detector            */
#define VID_STL_INR_SET       0x02 /* inhibit re-decode mode                */
#define VID_STL_DSS_SET       0x04 /* disable sequence search after soft reset */

/* VID_CWL - CD Write Launch (1-bits) ------------------------------------- */
#define VID_CWL_SET           0x01 /* Launch Compress Data write FIFO       */

/* VID_ITX - Interrupt registers [23:16] (8 bits )------------------------- */
#define VID_ITX_HIGH_NDP      0x80 /* new discarded packet                  */
#define VID_ITX_HIGH_ERR      0x40 /* Inconsistancy err in PES parser       */
#define VID_ITX_HIGH_ABF      0x08 /* Audio BitBuff FULL                    */
#define VID_ITX_HIGH_SFF      0x04 /* Sub picture CD FIFO full              */
#define VID_ITX_HIGH_AFF      0x02 /* Audio CD FIFO full                    */
#define VID_ITX_HIGH_ABE      0x01 /* Audio BitBuff EMPTY                   */

/* VID_ITX - Interrupt registers [15:0] (16 bits )------------------------- */
#define VID_ITX_NDP   0x800000 /* new discarded packet                  */
#define VID_ITX_ERR   0x400000 /* Inconsistancy err in PES parser       */
#define VID_ITX_TR_OK 0x100000 /* VLD is READY                          */
#define VID_ITX_ABF   0x080000 /* Audio BitBuff FULL                    */
#define VID_ITX_SFF   0x040000 /* Sub picture CD FIFO full              */
#define VID_ITX_AFF   0x020000 /* Audio CD FIFO full                    */
#define VID_ITX_ABE   0x010000 /* Audio BitBuff EMPTY                   */
#define VID_ITX_PDE   0x8000       /* Pict decoding or Underflow error      */
#define VID_ITX_SER   0x4000       /* Severe or Overflow error              */
#define VID_ITX_BMI   0x2000       /* Block Move Idle                       */
#define VID_ITX_HFF   0x1000       /* Header FIFO full                      */
#define VID_ITX_PNC   0x0800       /* Panic occurence                       */
#define VID_ITX_ERC   0x0400       /* Error concealment                     */
#define VID_ITX_DEI   0x0200       /* Decoder idle                          */
#define VID_ITX_PII   0x0100       /* Pipeline idle                         */
#define VID_ITX_PSD   0x0080       /* Dsync - Decode start                  */
#define VID_ITX_VST   0x0040       /* Top(Odd) Vsync                        */
#define VID_ITX_VSB   0x0020       /* Bottom(Even) Vsync                    */
#define VID_ITX_BBE   0x0010       /* Video Bitbuff empty                   */
#define VID_ITX_BBF   0x0008       /* Video Bitbuff full                    */
#define VID_ITX_HFE   0x0004       /* Header FIFO empty                     */
#define VID_ITX_BFF   0x0002       /* CD Fifo full                          */
#define VID_ITX_SCH   0x0001       /* Start Code Hit                        */

/* VID_LDP - Load Pointer (1-bit) ----------------------------------------- */
#define VID_LDP_SET             0x01 /* Load start code detector pointer    */
#define VID_LDP_RESET           0x00 /* reset ( normal decoding )           */

/* VID_TP_LDP - Load Pointer (3-bits) ------------------------------------- */
#define VID_TP_LDP_LDP_SET      0x01 /* Load start code detector pointer    */
#define VID_TP_LDP_TM_SET       0x02 /* Trick Mode enable                   */
#define VID_TP_LDP_VSB_SET      0x04 /* Stop the VLD read                   */
#define VID_TP_LDP_SBS          0x08 /* stop the SCD read */
#define VID_TP_LDP_RESET        0x00 /* reset ( normal decoding )           */

/* VID_VFL - Vertical luma filter ( 5 bits )------------------------------- */
#define VID_VFL_MASK            0x1F /* vertical luma filter mask           */

/* VID_VFC - Vertical chroma filter ( 5 bits ) ---------------------------- */
#define VID_VFC_MASK            0x1F /* vertical chroma filter mask         */

/* VID_TRF - Temporal reference (13-bit) ---------------------------------- */
#define VID_TRF_MASK        0x03FF /* mask for temporal reference value     */
#define VID_TRF_DTR         0x0400 /* Disable temporal reference            */
#define VID_TRF_DC2         0x0800 /* Redecode same B-frame twice           */
#define VID_TRF_TML         0x1000 /* Trick mode launch                     */

/* VID_HDS - Header Search (4-bit) ---------------------------------------- */
#define VID_HDS_START       0x01 /* start header search                     */
#define VID_HDS_QMI         0x02 /* select intra Q table for loading        */
#define VID_HDS_QMN         0x00 /* select non-intra Q table for loading    */
#define VID_HDS_SOS         0x04 /* stop on first slice                     */
#define VID_HDS_SCM         0x08 /* start code position                     */

/* VID_DCF - Display Configuration (16-bit) ------------------------------- */
#define VID_DCF_BLL         0x2000 /* Blank last line                       */
#define VID_DCF_BFL         0x1000 /* Blank first line                      */
#define VID_DCF_FNF         0x0800 /* Frame not field                       */
#define VID_DCF_FLY         0x0400 /* On the fly                            */
#define VID_DCF_ORF         0x0200 /* One row per frame                     */
#define VID_DCF_FRZ         0x0100 /* Freeze bit                            */
#define VID_DCF_EVD         0x0020 /* Enable Video Display                  */
#define VID_DCF_DSR         0x0008 /* Disable Sample Rate Convertor         */
#define VID_DCF_STP_DEC     0x0001 /* Prevent picture degradation in ZOut x4*/

/* VID_DIS - Display Configuration (8 bit) -------------------------------- */
#define VID_DIS_ZO_X4       0x04   /* Line drop : zoom out x2 -> x4         */
#define VID_DIS_CPU         0x60   /* CPU Priority                          */
#define VID_DIS_DIS         0x18   /* Display Priority                      */

/* VID_MWV - Video mix ( 8 bits) ------------------------------------------ */
#define VID_MWV_BCK         0xFF   /* background is 100%, Video is 0%       */
#define VID_MWV_VID         0x00   /* Video is 100%, background is 0%       */

/* VID_MWS - Still mix ( 8 bits) ------------------------------------------ */
#define VID_MWS_BCK         0xFF   /* background is 100%, still is 0%       */
#define VID_MWS_TDL         0x00   /* Still is 100%, background is 0%       */

/* VID_MWSV - Still / Video mix ( 8 bits) --------------------------------- */
#define VID_MWSV_TDL         0xFF  /* Still is 100%, Video is 0%            */
#define VID_MWSV_VID         0x00  /* Video is 100%, Still is 0%            */


/* --------------------------------------------------------
Definitions for "external" registers also used by the video drivers
WARNING: this registers are not necessary fully dedicated
to the video and may be re-defined and used by other drivers.
They have to be used with care and may be protected against
concurent access.
----------------------------------------------------------- */

/* Base address for config registers  --------------------------------------- */

#define CFG_BASE_ADDRESS      0x00000000

/* CFG - configuration registers : SDRAM, clocks, interface   --------------- */
#define CFG_CDR               0x44   /* Manual compressed data input ( debug and still load */

/* CAUTION: Register CFG_GCF is also used in STBOOT and STAUD. Therefore, access to it should be protected. */
#define CFG_GCF               0x3A   /* Chip general configuration [7:0] */

/* CAUTION: Register CFG_CCF is also used in STBOOT. Therefore, access to it should be protected. */
#define CFG_CCF               0x01   /* Clock configuration [7:0] */


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


/* CFG_CCF - Chip Configuration (8-bit) ------------------------------------- */
#define CFG_CCF_EAI           0x80 /* Enable Audio Interface */
#define CFG_CCF_EOU           0x40 /* Enable Ovf/Udf errors */
#define CFG_CCF_PBO           0x20 /* Prevent Bitbuffer Overflow */
#define CFG_CCF_EC3           0x10 /* Enable Clock 3 */
#define CFG_CCF_EC2           0x08 /* Enable Clock 2 */
#define CFG_CCF_ECK           0x04 /* Enable Clock1 that gives ck2 and ck3 */
#define CFG_CCF_EDI           0x02 /* Enable SDRAM Interface ( pads ) */
#define CFG_CCF_EVI           0x01 /* Enable Video Interface (internal bus) */


/* Base address for pes parser decoder  ------------------------------------- */
#define PES_BASE_ADDRESS      0x00000000

/* PES - pes parser registers ----------------------------------------------- */
#define PES_CF2               0x41 /* Pes parser conf 2, dedicated for video decoding[7:0] */
#define PES_TM1               0x42 /* DSM trick mode bits [7:0] */
#define PES_TM2               0x43 /* Pes parser status [1:0] */
#define PES_TS0               0x49 /* Pes Time stamp [ 7:0] */
#define PES_TS1               0x4A /* Pes Time stamp [15:8] */
#define PES_TS2               0x4B /* Pes Time stamp [23:16] */
#define PES_TS3               0x4C /* Pes Time stamp [31:24] */
#define PES_TS4               0x4D /* Pes Time stamp [33:32] */

/* PES - Pes registers specific 32 bit calls -------------------------------- */
#define PES_TS32              0x49   /* Pes Time stamp [31:0] : 32 LSB only */

/* PES_CF2 - Video decoding control (8-bit) --------------------------------- */
/* #define PES_CF2_MODE            0xC0*/ /* (2) mode of Video Stream Parser */
#define PES_CF2_SS            0x20 /* System or elementary stream */
#define PES_CF2_IVI           0x10 /* Ignore Video Stream ID ????????? */
#define PES_CF2_VID_ID_MASK   0xF0 /* Video Stream Id mask */
#define PES_CF2_MOD_00        0x00
#define PES_CF2_MOD_01        0x40
#define PES_CF2_MOD_10        0x80
#define PES_CF2_MOD_11        0xC0
#define PES_CF2_MOD_MASK      0x3F /* MOD mask */

/* PES_TM2 - PES parser status ( 2 bits ) ----------------------------------- */
#define PES_TM2_MASK          0x03  /* Mask to read significant data */
#define PES_TM2_M2            0x02  /* MPEG2 not MPEG1 stream */
#define PES_TM2_DSA           0x01  /* Pes parser DSM association flag */

/* PES_TS - PES time stamps MSbits ( 1 ) bit -------------------------------- */
#define PES_TS_TSA            0x02  /* Time stamp association */
/* -------- */



/* Exported Types ----------------------------------------------------------- */


/* Exported Variables ------------------------------------------------------- */


/* Exported Macros ---------------------------------------------------------- */


/* Exported Functions ------------------------------------------------------- */



/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __DECODE_REGISTERS_OMEGA1_H */

/* End of hvr_dec1.h */
