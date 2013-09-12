/*****************************************************************************
*
*   File name   : video_reg.h
*
*   Description : Video register addresses
*
*   COPYRIGHT (C) ST-Microelectronics 1999-2001.
*
*   Date            Modification                                                    Name
*   ----            ------------                                                    ----
*   14-Sep-1999     Created                                                         FC
*   14-Jan-2000     Changed all pXXX to AXXX                                        FC
*   22-Nov-2001     Add descriptions for registers of SDMEGO2 IP                    JFC
*   23-Nov-2001     Include declaration for video specific config registers.        JFC
*   28-Nov-2001     Remove compression related items                                JFC
*   20-Dec-2001     Add real support for SDMPEGO2 quantization tables addressing    JFC
*
*   TODO:
*   =====
*****************************************************************************/

#ifndef __VIDEO_REG_H
#define __VIDEO_REG_H

/* Includes --------------------------------------------------------------- */

/* #include "stdevice.h"*/
#include "dumpreg.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Exported Types --------------------------------------------------------- */

/* Exported Constants ----------------------------------------------------- */

/* Register offsets */
/* Configuration registers */
#if defined(VIDEO_SDMPEGO2)

/*+    #error "SDMEGO2 selected" */
    /* Register base address */
    #define VID1            VID1_REG_BASE_ADDRESS
    #define VID2            VID2_REG_BASE_ADDRESS
    #define VID1_QTABLES    VID1_QTABLES_BASE_ADDRESS
    #define VID2_QTABLES    VID2_QTABLES_BASE_ADDRESS

    /* ---------------- Moved registers
    */
    #define VID_CDI              0x000          /* CD Input from host interface */
/*+ JFC (20-Nov-2001) : Should be changed (cf. with L. Hervieu) */
    #define VID_QMWI             0x000          /* Quantization matrix data, intra table */
    #define VID_QMWNI            0x080          /* Quantization matrix data, non intra table */


    /* ---------------- Configuration registers
    */
    #define VID_CFG_DECPR        0x000          /* Decoding priority setup */
    #define VID_CFG_PORD         0x004          /* Priority Order of decoding processes */
    #define VID_CFG_VDCTL        0x008          /* Video Decoder Control */
    #define VID_CFG_VSEL         0x00c          /* Vsync Selection */
    #define VID_CFG_VIDIC        0x010          /* Video Decoder Interconnect Configuration */
    #define VID_CFG_THP          0x014          /* Threshold Period */

    /* ---------------- Stream independent debug registers
    */
    #define VID_DBG_REQS         0x040          /* Request Counter */
    #define VID_DBG_REQC         0x044          /* Request State */
    #define VID_DBG_INS1         0x060          /* Debug Reg 1 (Current instruction in video decoder 1st part) */
    #define VID_DBG_INS2         0x064          /* Debug Reg 2 (Current instruction in video decoder 2nd part) */
    #define VID_DBG_VMBN         0x068          /* VLD Macro Block Number */
    #define VID_DBG_VLDS         0x06C          /* VLD State */
    #define VID_DBG_MBE0         0x070          /* Macro block Error statistic [0 to 3] */
    #define VID_DBG_MBE1         0x074          /* Macro block Error statistic [4 to 7] */
    #define VID_DBG_MBE2         0x078          /* Macro block Error statistic [8 to 11] */
    #define VID_DBG_MBE3         0x07C          /* Macro block Error statistic [12 to 15] */


    /* ---------------- Stream dependent registers
    */
    #define VID_TIS             0x000           /* Task Instruction */
    #define VID_PPR             0x004           /* Picture F Parameters and other parameters */
    #define VID_PRS             0x008           /* Pipe Reset */
    #define VID_SRS             0x00c           /* Soft Reset */
    #define VID_THI             0x020           /* Threshold Inactive */
    #define VID_PTI             0x024           /* Panic Time */
    #define VID_TRA             0x028           /* Panic Transition */
    #define VID_PMA             0x02c           /* Panic Maximum */
    #define VID_ITM             0x0f0           /* Interrupt mask */
    #define VID_ITS             0x0f4           /* Interrupt status */
    #define VID_STA             0x0f8           /* Status register */
    #define VID_DFH             0x100           /* Decoded Frame Height (in MB) */
    #define VID_DFS             0x104           /* Decoded Frame Size (in MB) */
    #define VID_DFW             0x108           /* Decoded Frame Width (in MB) */
    #define VID_BBG             0x10C           /* Bit Buffer start address */
    #define VID_BBL             0x110           /* Bit Buffer level */
    #define VID_BBS             0x114           /* Bit Buffer end address */
    #define VID_BBT             0x118           /* Bit Buffer Threshold */
    #define VID_BBTL            0x11c           /* Bit Buffer Threshold Low */
    #define VID_CDB             0x120           /* Compressed Data Buffer (configuration) */
    #define VID_CDPTR           0x124           /* Compressed Data Pointer */
    #define VID_CDLP            0x128           /* Compressed Data Load Pointer */
    #define VID_CDPTR_CUR       0x12c           /* Current Value of Compressed Data Pointer */
    #define VID_CDWL            0x130           /* Top address limit of CD write in Trick mode */
    #define VID_SCDB            0x134           /* Start Code Detector (configuration) */
    #define VID_SCDRL           0x138           /* Start Code Detector Read Limit */
    #define VID_SCDPTR_CUR      0x13c           /* Current Value of Start Code Detector Pointer */
    #define VID_VLDB            0x140           /* VLD Buffer (configuration) */
    #define VID_VLD_PTR_CUR     0x144           /* Current Value of VLD Read Pointer */
    #define VID_VLDRL           0x148           /* VLD Read Limit */
    #define VID_SCDUP           0x14c           /* Update Start Code Detector Pointer */
    #define VID_SCDUPTR         0x150           /* Start Code Detector Update Pointer */
    #define VID_VLUP            0x154           /* Update VLD Pointer  */
    #define VID_VLDUPTR         0x158           /* VLD Update Pointer */
    #define VID_VLDPTR          0x15C           /* VLD start address for multi-decode & trick mode */
    #define VID_MBNM            0x180           /* Macroblock Number for the Main Reconstruction */
    #define VID_MBNS            0x184           /* Macroblock Number for the Secondary Reconstruction */
    #define VID_BCHP            0x188           /* Backward Chroma Frame Buffer */
    #define VID_BFP             0x18C           /* Backward Frame Buffer (256 bytes unit) */
    #define VID_FCHP            0x190           /* Forward Chroma Frame Buffer */
    #define VID_FFP             0x194           /* Forward Frame Buffer (256 bytes unit) */
    #define VID_RCHP            0x198           /* Reconstruction Chroma Frame Buffer */
    #define VID_RFP             0x19C           /* Reconstruction Frame Buffer (256 bytes unit) */
    #define VID_SRCHP           0x1A0           /* Secondary Reconstruction Chroma Frame Buffer */
    #define VID_SRFP            0x1A4           /* Secondary Reconstruction Frame Buffer (256 bytes unit) */
    #define VID_DMOD            0x1a8           /* Decoder Memory Mode: compress, H/n, V/n, ovw */
    #define VID_SDMOD           0x1ac           /* Secondary Reconstruction Memory Mode: compress, H/n, V/n, ovw */
    #define VID_HDF             0x200           /* Header Data FIFO */
    #define VID_HLNC            0x204           /* Launch start code detector */
    #define VID_SCD             0x208           /* Start Codes Disabled */
    #define VID_SCD_TM          0x20c           /* Bit Buffer Input Counter for Trick Mode */
    #define VID_SCPTR           0x210           /* Start Code Pointer */
    #define VID_PSCPTR          0x214           /* PSC Byte address in memory */
    #define VID_SCDPTR          0x218           /* SCD address pointer for launch in Trick mode */
    #define VID_PES_CFG         0x280           /* PES parser Configuration */
    #define VID_PES_SI          0x284           /* PES Stream Id */
    #define VID_PES_FSI         0x288           /* Stream ID Filter */
    #define VID_PES_TFI         0x28c           /* Trick Filtering ID (Fast Forward ID) */
    #define VID_PES_ASS         0x290           /* DSM and TS association flags */
    #define VID_PES_DSM         0x294           /* DSM Value */
    #define VID_PES_DSMC        0x298           /* PES DSM  Counter */
    #define VID_PES_TS0         0x2a0           /* Time-Stamp Value LSW */
    #define VID_PES_TS1         0x2a4           /* Time-Stamp Value MSB */
    #define VID_PES_TSC         0x2a8           /* PES Time Stamp Counter */
    #define VID_PES_CTA         0x2ac           /* Counter of Time Stamp Association */
    #define VID_SCDC            0x2b0           /* SCD count register */
    #define VID_CDC             0x2b4           /* CD count register */




    /* Mask for buffers addresses and pointers ---------------------------------------- */
    #define VID_FRAME_BUF_MASK   0xfffffe00     /* frame buffers : Address on 32 bits, aligned on 512 */
    #define VID_BIT_BUF_MASK     0xffffff00     /* bit buffers   : Address on 32 bits, aligned on 256 */
    #define VID_CD_WR_LIMIT_MASK 0xffffff80     /* bit buffers : Address on 32 bits, aligned on 128 */

    /* VID_CFG_VSEL - Vertical synchro selection (and validation) --------------------- */
    #define VID_CFG_VSEL_SRCE_MASK  0x01
    #define VID_CFG_VSEL_VSYNC_1    0x00        /* VTG1 is used by SDMEGO2 IP           */
    #define VID_CFG_VSEL_VSYNC_2    0x01        /* VTG2 is used by SDMEGO2 IP           */
    #define VID_CFG_VSEL_DISABLE    0x02        /* Disable decoder VSync                */

    /* VID_CFG_VDCTL - Video Decoder Ctrl ----------------------------------------------------*/
    #define VID_CFG_VDCTL_ERO       0x02        /* Automatic pipeline reset on overflow error */
    #define VID_CFG_VDCTL_ERU       0x04        /* Automatic pipeline reset on underflow      */
    #define VID_CFG_VDCTL_MVC       0x08        /* Ensure motion vector stays inside picture  */

    /* CFG_DECPR - Multi-decode configuration ----------------------------------------- */
    #define VID_CFG_DECPR_PS        0x02        /* Priority Setup                       */
    #define VID_CFG_DECPR_FP        0x00        /* Force Priority                       */

    /* VID_CDB - Compressed Data Buffer ----------------------------------------------- */
    #define VID_CDB_CM              0x01        /* Circular Mode                        */
    #define VID_CDB_WLE             0x02        /* Write Limit Enable                   */
    #define VID_CDB_PBO             0x04        /* Prevent Bit buffer Overflow          */

    /* VID_CDLP - Compressed Data Load Pointer ---------------------------------------- */
    #define VID_CDLP_LP             0x01        /* Load Pointer                         */
    #define VID_CDLP_RP             0x02        /* Reset Parser                         */

    /* VID_TIS - Task Instruction (8-bit) --------------------------------------------- */
    #define VID_TIS_EXE             0x01        /* Execute the next Task                */
    #define VID_TIS_FIS             0x02        /* Force Instruction                    */
    #define VID_TIS_SKIP            0x0C        /* skip modempeg2 mode                  */
    #define VID_TIS_SKIP_MASK       0xF3        /* skip bit flags position              */
    #define NO_SKIP                 0x00        /* Dont skip any picture                */
    #define SKIP_ONE_DECODE_NEXT    0x04        /* skip one pict and decode next pict   */
    #define SKIP_TWO_DECODE_NEXT    0x08        /* skip two pict and decode next pict   */
    #define SKIP_ONE_STOP_DECODE    0x0c        /* skip one pict and stop decoder       */
    #define VID_TIS_MP2             0x10        /* MPEG-2 mode                          */
    #define VID_TIS_DPR             0x20        /* decoding period                      */
    #define VID_TIS_LFR             0x40        /* load from register                   */
    #define VID_TIS_LDP             0x80        /* load pointer                         */
    #define VID_TIS_SCN             0x100       /* start code detector disable          */
    #define VID_TIS_SPD             0x200       /* Select DID not PID                   */
    #define VID_TIS_EOD             0x400       /* End of decoding                      */

    /* VID_PPR - Picture F Parameters and other parameters -----------------------------*/
    #define VID_PPR_FFH             0x0000000F  /* (4) forward_horz_f_code              */
    #define VID_PPR_BFH             0x000000F0  /* (4) backward_horz_f_code             */
    #define VID_PPR_BFH_SHIFT       4           /* Shift for BFH                        */
    #define VID_PPR_FFV             0x00000F00  /* (4) forward_vert_f_code              */
    #define VID_PPR_FFV_SHIFT       8           /* Shift for FFV                        */
    #define VID_PPR_BFV             0x0000F000  /* (4) backward_vert_f_code             */
    #define VID_PPR_BFV_SHIFT       12          /* Shift for BFV                        */
    #define VID_PPR_PST             0x00030000  /* (2) pict_struct                      */
    #define VID_PPR_TOP_FIELD       0x00010000  /* PST = 1 Top Field                    */
    #define VID_PPR_BOT_FIELD       0x00020000  /* PST = 2 Bottom field                 */
    #define VID_PPR_FRAME           0x00030000  /* PST = 3 Frame picture                */
    #define VID_PPR_DCP             0x000C0000  /* (2) intra_dc_precision               */
    #define VID_PPR_PCT             0x00300000  /* (2) pict_coding_type                 */
    #define VID_PPR_AZZ             0x00400000  /* alternate_scan                       */
    #define VID_PPR_IVF             0x00800000  /* intra_vlc_format                     */
    #define VID_PPR_QST             0x01000000  /* q_scale_type                         */
    #define VID_PPR_CMV             0x02000000  /* concealment_motion_vectors           */
    #define VID_PPR_FRM             0x04000000  /* frame_pred_frame_dct                 */
    #define VID_PPR_TFF             0x08000000  /* Top Field First                      */
    #define VID_PPR_FFN             0x30000000  /* Force Field Number                   */
    #define VID_PPR_FFN_SHIFT       28          /* Shift for FFN                        */

/*+ JFC (20-Nov-2001) : Need to be moved to some display driver */
    /* VID_PFLD - Presentation Field Selection -----------------------------------------*/
    #define VID_PFLD_TCEN           0x01        /* Toggle on Chroma Enable              */
    #define VID_PFLD_TYEN           0x02        /* Toggle on Luma Enable                */
    #define VID_PFLD_BTC            0x04        /* Bottom not Top for chroma            */
    #define VID_PFLD_BTY            0x08        /* Bottom not Top for luma              */

    /* VID_PMOD - Presentation Memory Mode ---------------------------------------------*/
    #define VID_PMOD_V2_VDEC        0x04        /* V/2 decimation                       */
    #define VID_PMOD_V4_VDEC        0x08        /* V/4 decimation                       */
    #define VID_PMOD_HDEC           0x30        /* H decimation mode          */
    #define VID_PMOD_NO_HDEC        0x00        /* no horizontal decimation   */
    #define VID_PMOD_V2_HDEC        0x10        /* H/2 decimation                       */
    #define VID_PMOD_V4_HDEC        0x20        /* H/4 decimation                       */
    #define VID_PMOD_PDL            0x40        /* Progressive display luma             */
    #define VID_PMOD_PDC            0x80        /* Progressive display chroma           */
    #define VID_PMOD_SFB            0x100       /* Secondary frame buffer               */


    /* VID_APMOD - PIP Presentation Memory Mode ----------------------------------------*/
    #define VID_APMOD_DEC           0x3C        /* H and V decimation modes   */
    #define VID_APMOD_VDEC          0x0C        /* V decimation               */
    #define VID_APMOD_NO_VDEC       0x00        /* no vertical decimation     */
    #define VID_APMOD_V2_VDEC       0x04        /* V/2 decimation                       */
    #define VID_APMOD_V4_VDEC       0x08        /* V/4 decimation                       */
    #define VID_APMOD_HDEC          0x30        /* H decimation               */
    #define VID_APMOD_NO_HDEC       0x00        /* no horizontal decimation             */
    #define VID_APMOD_V2_HDEC       0x10        /* H/2 decimation                       */
    #define VID_APMOD_V4_HDEC       0x20        /* H/4 decimation                       */
    #define VID_APMOD_PDL           0x40        /* Progressive display luma             */
    #define VID_APMOD_PDC           0x80        /* Progressive display chroma           */
    #define VID_APMOD_SFB           0x100       /* Secondary frame buffer               */
/*+ End of JFC (20-Nov-2001) : */

    /* VID_DMOD - Decoder Memory Mode: -------------------------------------------------*/
    #define VID_DMOD_EN             0x001       /* Enable reconstruction in main buffer */

    /* VID_SDMOD - Secondary Reconstruction Memory Mode: H/n, V/n -----------------------*/
    #define VID_SDMOD_DEC           0x078        /* H and V decimation modes mask        */
    #define VID_SDMOD_VDEC          0x060        /* V decimation mode                    */
    #define VID_SDMOD_NO_VDEC       0x000        /* no decimation                        */
    #define VID_SDMOD_V2_VDEC       0x020        /* V/2 decimation                       */
    #define VID_SDMOD_V4_VDEC       0x040        /* V/4 decimation                       */
    #define VID_SDMOD_HDEC          0x018        /* H decimation mode mask               */
    #define VID_SDMOD_NO_HDEC       0x000        /* no decimation                        */
    #define VID_SDMOD_V2_HDEC       0x008        /* H/2 decimation                       */
    #define VID_SDMOD_V4_HDEC       0x010        /* H/4 decimation                       */
    #define VID_SDMOD_EN            0x002       /* Enable reconstruction in main buffer */
    #define VID_SDMOD_PS            0x001       /* Progressive sequence                 */


    /* VID_ITX - Interrupt bits --------------------------------------------------------*/
    #define VID_ITX_SCH             0x00000001  /* Start code header hit                        */
    #define VID_ITX_HFF             0x00000002  /* Header fifo nearly full                      */
    #define VID_ITX_HFE             0x00000004  /* Header fifo empty                            */
    #define VID_ITX_IER             0x00000008  /* Inconsistency error in PES parser            */
    #define VID_ITX_BFF             0x00000010  /* Bitstream fifo full                          */

    #define VID_ITX_PSD             0x00000040  /* Pipeline start decoding                      */
    #define VID_ITX_PID             0x00000080  /* Pipeline idle                                */
    #define VID_ITX_DID             0x00000100  /* Decoder idle                                 */
    #define VID_ITX_OTE             0x00000200  /* Decoding overtime error                      */
    #define VID_ITX_DOE             0x00000400  /* Decoding overflow error                      */
    #define VID_ITX_DUE             0x00000800  /* Decoding underflow error                     */
    #define VID_ITX_MSE             0x00001000  /* Semantic or syntax error detected            */
    #define VID_ITX_DSE             0x00002000  /* Decoding semantic or syntax error            */
    #define VID_ITX_PAN             0x00004000  /* Threshold decoding speed has been reached    */
    #define VID_ITX_BBF             0x00008000  /* Bit Buffer Nearly full                       */
    #define VID_ITX_BBNE            0x00010000  /* Bit Buffer Nearly Empty */
    #define VID_ITX_BBE             0x00020000  /* Bit Buffer Empty                             */
    #define VID_ITX_SBE             0x00040000  /* SCD bit Buffer Empty                         */
    #define VID_ITX_CDWL            0x00080000  /* Compressed Data Write Limit                  */
    #define VID_ITX_SCDRL           0x00100000  /* Start Code Detector Read Limit               */
    #define VID_ITX_VLDRL           0x00200000  /* VLD Read Limit                               */
    #define VID_ITX_R_OPC           0x00400000  /* r_opc                                        */
    #define VID_ITX_V_TOP           0x00800000  /* Top Field detected                           */
    #define VID_ITX_V_BOT           0x01000000  /* Bottom Field detected                        */
    #define VID_ITX_MASK            0x01FFFFFF  /* Mask for all interrupt bits                  */

    /* VID_PES_CFG - PES parser Configuration ------------------------------------------*/
    #define VID_PES_SS              0x01        /* System stream                        */
    #define VID_PES_SDT             0x02        /* Store DTS not PTS                    */
    #define VID_PES_OFS_MASK        0x0C        /* Output format PES output             */
    #define VID_PES_OFS_PES         0x00        /* Output format PES output             */
    #define VID_PES_OFS_ES_SUBID    0x04        /* Output format ES substream ID layer removed */
    #define VID_PES_OFS_ES          0x0C        /* Output format ES "normal"            */
    #define VID_PES_MODE_MASK       0x30        /* Mode mask                            */
    #define VID_PES_MODE_AUTO       0x00        /* Mode 00: Auto                        */
    #define VID_PES_MODE_MP1        0x10        /* Mode 01: MPEG-1 system stream        */
    #define VID_PES_MODE_MP2        0x20        /* Mode 10: MPEG-2 PES stream           */
    #define VID_PES_PTF             0x40        /* PES trick filtering                  */

    /* VID_PES_ASS - DSM and TS association flags --------------------------------------*/
    #define VID_PES_TSA             0x01        /* Time Stamp association flags         */
    #define VID_PES_DSA             0x02        /* DSM association flags                */

    /* VID_HLNC - Launch start code detector -------------------------------------------*/
    #define VID_HLNC_LNC_MASK       0x03        /* Mask for LNC bits                    */
    #define VID_HLNC_START_NORM     0x01        /* Start code is launched normally      */
    #define VID_HLNC_START_REG      0x02        /* Start code is launched from SCDPTR   */
    #define VID_HLNC_SCNT           0x04        /* Select Counter                       */
    #define VID_HLNC_RTA            0x08        /* Restart Time stamp Association       */

    /* VID_HDF - Header Data FIFO ------------------------------------------------------*/
    #define VID_HDF_DATA            0x0000FFFF  /* Header data from stream              */
    #define VID_HDF_SCM             0x00010000  /* Start code on MSB flag               */

    /* VID_SCD - Start Code Disabled ---------------------------------------------------*/
    #define VID_SCD_PSC                   0x001 /* Picture Start Codes (00)                                   */
    #define VID_SCD_SSC                   0x002 /* all Slice Start Codes (01 to AF)                           */
    #define VID_SCD_SSCF                  0x004 /* all SSC except the first ones after a PSC                  */
    #define VID_SCD_UDSC                  0x008 /* User Data Start Codes (B2)                                 */
    #define VID_SCD_SHC                   0x010 /* Sequence Header Codes (B3)                                 */
    #define VID_SCD_SEC                   0x020 /* Sequence Error Codes (B4)                                  */
    #define VID_SCD_ESC                   0x040 /* Extension Start Codes (B5)                                 */
    #define VID_SCD_SENC                  0x080 /* Sequence End Codes (B7)                                    */
    #define VID_SCD_GSC                   0x100 /* Group Start Codes (B8)                                     */
    #define VID_SCD_SYSC                  0x200 /* System start Codes (B9 through FF)                         */
    #define VID_SCD_RSC                   0x400 /* Reserved Start Codes (B0, B1, B6)                          */
    #define VID_SCD_IGNORE_ALL            0x7ff /* Ignore all start codes (for specific validation only)      */
    #define VID_SCD_IGNORE_ALL_BUT_SEQ    0x7ef /* Ignore all start code except Sequence header (B3)          */
    #define VID_SCD_ENABLE_ALL_BUT_SLICE  VID_SCD_SSC /* Enable all start code except all slices (01 to AF)   */

    /* VID_SCDB - Start Code Detector Buffer ------------------------------------------- */
    #define VID_SCDB_CM              0x01       /* Circular Mode                         */
    #define VID_SCDB_RLE             0x02       /* Read Limit Enable                     */
    #define VID_SCDB_PRNW            0x04       /* Prevent Prevent Read Not Write        */

    /* VID_SCDUP - Update Start Code Pointer ------------------------------------------------- */
    #define VID_SCDUP_UP             0x01       /* Update the start code detector read pointer */

    /* VID_VLDB - VLD Buffer ----------------------------------------------------------- */
    #define VID_VLDB_CM              0x01       /* Circular Mode                         */
    #define VID_VLDB_RLE             0x02       /* Read Limit Enable                     */
    #define VID_VLDB_PRNW            0x04       /* Prevent Prevent Read Not Write        */


    /* VID_DBG_INS1 - Decoder instruction part 1 ---------------------------------------*/
    #define VID_DBG_INS1_MP2        0x80000000  /* MP2 from VIDn_TIS                    */
    #define VID_DBG_INS1_PPR        0x04FFFFFF  /* VIDn_PPR[29:00]                      */

    /* VID_DBG_INS2 - Decoder instruction part 2 ---------------------------------------*/
    #define VID_DBG_INS2_PPE_STATE2     0x00000003  /* State of the PPE2 finite state machine   */
    #define VID_DBG_INS2_PPE_STATE1     0x00000038  /* State of the PPE1 finite state machine   */
    #define VID_DBG_INS2_HDSH           0x00003fc0  /* Pipeline and MCU hanshakes               */
    #define VID_DBG_INS2_MCU_ADDER2_REQ 0x00000040  /* Request MCU -> PEN                       */
    #define VID_DBG_INS2_MCU_ADDER1_REQ 0x00000080  /* Request MCU -> PEN                       */
    #define VID_DBG_INS2_PEN_ADDER2_ACK 0x00000100  /* Acknowledge PEN -> MCU                   */
    #define VID_DBG_INS2_PEN_ADDER1_ACK 0x00000200  /* Acknowledge PEN -> MCU                   */
    #define VID_DBG_INS2_PEE_ADDER2_ACK 0x00000400  /* Acknowledge PPE2 -> PEN                  */
    #define VID_DBG_INS2_PEE_ADDER1_ACK 0x00000800  /* Acknowledge PPE1 -> PEN                  */
    #define VID_DBG_INS2_PEN_ADDER2_REQ 0x00001000  /* Acknowledge PEN -> PPE2                  */
    #define VID_DBG_INS2_PEN_ADDER1_REQ 0x00002000  /* Acknowledge PEN -> PPE1                  */
    #define VID_DBG_INS2_IDLE           0x00004000  /* Idle                                     */
    #define VID_DBG_INS2_SSEL           0x00030000  /* Stream Select (1 or 2 when not Idle)     */

    #define VID_DBG_REQS_PES1           0x0001      /* Request of PES parser for video 1                */
    #define VID_DBG_REQS_PES1_M         0x0002      /* Request of PES parser for video 1 after mask     */
    #define VID_DBG_REQS_PES2           0x0004      /* Request of PES parser for video 2                */
    #define VID_DBG_REQS_PES2_M         0x0008      /* Request of PES parser for video 2 after mask     */
    #define VID_DBG_REQS_SCD1           0x0010      /* Request of SCD for video 1                       */
    #define VID_DBG_REQS_SCD1_M         0x0020      /* Request of SCD for video 1 after mask            */
    #define VID_DBG_REQS_SCD2           0x0040      /* Request of SCD for video 2                       */
    #define VID_DBG_REQS_SCD2_M         0x0080      /* Request of SCD for video 2 after mask            */
    #define VID_DBG_REQS_VLD            0x0100      /* Request of VLD                                   */
    #define VID_DBG_REQS_VLD_M          0x0200      /* Request of VLD after mask                        */
    #define VID_DBG_REQS_MRY            0x0400      /* Request for Y     in main reconstruction         */
    #define VID_DBG_REQS_MRC            0x0800      /* Request for Cr/Cb in main reconstruction         */
    #define VID_DBG_REQS_SRY            0x1000      /* Request for Y     in secondary reconstruction    */
    #define VID_DBG_REQS_SRC            0x2000      /* Request for Cr/Cb in secondary reconstruction    */
    #define VID_DBG_REQS_PRDY           0x4000      /* Request for Y     data prediction                */
    #define VID_DBG_REQS_PRDC           0x8000      /* Request for Cr/Cb data prediction                */







#elif defined(VIDEO_7015) || defined(VIDEO_7020)

    /* ---------------- Configuration registers (moved from cfg_reg.h to be homogeneous with SDMPEGO2 IP)
    */
    #define VID_CFG_PORD        0x0c            /* Priority Order in LMC */
    #define VID_CFG_VDCTL       0x14            /* Video Decoder Ctrl */
    #define VID_CFG_DECPR       0x1c            /* Multi-decode configuration */
    #define VID_CFG_VSEL        0x20            /* VSync selection for decode Synchronisation */
    #define VID_CFG_CMPEPS      0x114           /* Epsilon value for compression/decompression */

    /* Register base address */
#if defined(VIDEO_7015)
    #define VID1            (STI7015_BASE_ADDRESS+ST7015_VID1_OFFSET)
    #define VID2            (STI7015_BASE_ADDRESS+ST7015_VID2_OFFSET)
    #define VID3            (STI7015_BASE_ADDRESS+ST7015_VID3_OFFSET)
    #define VID4            (STI7015_BASE_ADDRESS+ST7015_VID4_OFFSET)
    #define VID5            (STI7015_BASE_ADDRESS+ST7015_VID5_OFFSET)
    #define VIDEO_CFG_BASE_ADDRESS (STI7015_BASE_ADDRESS+ST7015_CFG_OFFSET) /* as for CFG */
#else
    #define VID1            (STI7020_BASE_ADDRESS+ST7020_VID1_OFFSET)
    #define VID2            (STI7020_BASE_ADDRESS+ST7020_VID2_OFFSET)
    #define VID3            (STI7020_BASE_ADDRESS+ST7020_VID3_OFFSET)
    #define VID4            (STI7020_BASE_ADDRESS+ST7020_VID4_OFFSET)
    #define VID5            (STI7020_BASE_ADDRESS+ST7020_VID5_OFFSET)
    #define VIDEO_CFG_BASE_ADDRESS (STI7020_BASE_ADDRESS+ST7020_CFG_OFFSET) /* as for CFG */
#endif

    #define VID_TIS             0x00            /* Task Instruction */
    #define VID_PPR             0x04            /* Picture F Parameters and other parameters */
    #define VID_RFP             0x08            /* Reconstruction Frame Buffer (256 bytes unit) */
    #define VID_RCHP            0x0c            /* Reconstruction Chroma Frame Buffer */
    #define VID_FFP             0x10            /* Forward Frame Buffer (256 bytes unit) */
    #define VID_FCHP            0x14            /* Forward Chroma Frame Buffer */
    #define VID_BFP             0x18            /* Backward Frame Buffer (256 bytes unit) */
    #define VID_BCHP            0x1c            /* Backward Chroma Frame Buffer */
    #define VID_SRFP            0x20            /* Secondary Reconstruction Frame Buffer (256 bytes unit) */
    #define VID_SRCHP           0x24            /* Secondary Reconstruction Chroma Frame Buffer */
    #define VID_PFP             0x28            /* Presentation Frame Buffer (256 bytes unit) */
    #define VID_PCHP            0x2c            /* Presentation Chroma Frame Buffer */
    #define VID_APFP            0x30            /* PIP Presentation Frame Buffer (256 bytes unit) */
    #define VID_APCHP           0x34            /* PIP Presentation Chroma Frame Buffer */
    #define VID_PFLD            0x38            /* Presentation Field Selection */
    #define VID_APFLD           0x3c            /* PIP Presentation Field Selection */
    #define VID_BBG             0x40            /* Bit Buffer start address */
    #define VID_BBS             0x44            /* Bit Buffer end address */
    #define VID_BBT             0x48            /* Bit Buffer threshold */
    #define VID_BBL             0x4c            /* Bit Buffer level */
    #define VID_PSCPTR          0x50            /* PSC Byte address in memory */
    #define VID_VLDPTR          0x54            /* VLD start address for multi-decode & trick mode */

    #define VID_DFW             0x60            /* Decoded Frame Width (in MB) */
    #define VID_DFH             0x64            /* Decoded Frame Heigt (in MB) */
    #define VID_DFS             0x68            /* Decoded Frame Size (in MB) */
    #define VID_PFW             0x6c            /* Presentation Frame Width (in MB) */
    #define VID_PFH             0x70            /* Presentation Frame Size (in MB) */

    #define VID_STP             0x78            /* Video bitstream management setup */
    #define VID_CDI             0x7c            /* CD Input from host interface */
    #define VID_DMOD            0x80            /* Decoder Memory Mode: compress, H/n, V/n */
    #define VID_PMOD            0x84            /* Presentation Memory Mode */
    #define VID_SDMOD           0x88            /* Secondary Reconstruction Memory Mode: H/n, V/n, ovw */
    #define VID_APMOD           0x8c            /* PIP Presentation Memory Mode */
    #define VID_SRS             0x90            /* Soft Reset */
    #define VID_PRS             0x94            /* Pipe Reset */

    #define VID_ITM             0x98            /* Interrupt mask */
    #define VID_ITS             0x9c            /* Interrupt status */
    #define VID_STA             0xa0            /* Status register */

    #define VID_CDWL            0xa8            /* Top address limit of CD write in Trick mode */
    #define VID_SCDPTR          0xac            /* SCD addr pointer for launch in Trick mode */

    #define VID_CDR             0xb4            /* CD from register */

    #define VID_CDC             0xb8            /* CD count register */
    #define VID_SCDC            0xbc            /* SCD count register */

    #define VID_PES_CFG         0xc0            /* PES parser Configuration */
    #define VID_PES_SI          0xc4            /* PES Stream Id */
    #define VID_PES_FSI         0xc8            /* Stream ID Filter */
    #define VID_PES_TFI         0xd4            /* Trick Filtering ID (Fast Forward ID) */
    #define VID_PES_DSM         0xd8            /* DSM Value */
    #define VID_PES_TS0         0xdc            /* Time-Stamp Value LSW */
    #define VID_PES_TS1         0xe0            /* Time-Stamp Value MSB */
    #define VID_PES_ASS         0xe4            /* DSM and TS association flags */

    #define VID_HDF             0xec            /* Header Data FIFO */
    #define VID_HLNC            0xf0            /* Launch start code detector */

    #define VID_QMWI            0xe00           /* Quantization matrix data, intra table */
    #define VID_QMWNI           0xe40           /* Quantization matrix data, non intra table */

    #define VID_VPP_CFG         0x120           /* HD-PIP parser configuration */
    #define VID_VPP_DFHT        0x124           /* HD-PIP parser frame height threshold */
    #define VID_VPP_DFWT        0x128           /* HD-PIP parser frame width threshold  */

    #define VID_CDWPTR          0x130           /* Compressed data write pointer */
    #define VID_VLDRPTR         0x134           /* VLD Read pointer */
    #define VID_SCDRPTR         0x138           /* SCD read pointer */

    #define VID_DBG_INS1        0x180           /* Current instruction in video decoder 1st part */
    #define VID_DBG_INS2        0x184           /* Current instruction in video decoder 2nnd part */

    /* Mask for buffers addresses and pointers ---------------------------------------- */
    #define VID_FRAME_BUF_MASK   0xfffffe00     /* frame buffers : Address on 32 bits, aligned on 512 */
    #define VID_BIT_BUF_MASK     0xffffff00     /* bit buffers : Address on 32 bits, aligned on 256 */
    #define VID_CD_WR_LIMIT_MASK 0xffffff80     /* bit buffers : Address on 32 bits, aligned on 128 */

    /* CFG_DECPR - Multi-decode configuration ------------------------------------------------*/
    #define VID_CFG_DECPR_SELA_MASK 0x000007    /* 1st priority decoder                       */
    #define VID_CFG_DECPR_SELB_MASK 0x000038    /* 2nd priority decoder                       */
    #define VID_CFG_DECPR_SELC_MASK 0x0001C0    /* 3rd priority decoder                       */
    #define VID_CFG_DECPR_SELD_MASK 0x000E00    /* 4th priority decoder                       */
    #define VID_CFG_DECPR_SELE_MASK 0x007000    /* 5th priority decoder                       */
    #define VID_CFG_DECPR_PS        0x008000    /* Priority Setup                             */
    #define VID_CFG_DECPR_FP        0x010000    /* Force Priority                             */

    /* VID_CFG_VDCTL - Video Decoder Ctrl --------------------------------------------------------*/
    #define VID_CFG_VDCTL_EDC       0x01        /* Enable decoding                            */
    #define VID_CFG_VDCTL_ERO       0x02        /* Automatic pipeline reset on overflow error */
    #define VID_CFG_VDCTL_ERU       0x04        /* Automatic pipeline reset on underflow      */
    #define VID_CFG_VDCTL_MVC       0x08        /* Ensure motion vector stays inside picture  */
    #define VID_CFG_VDCTL_SPI       0x10        /* Slice picture ID check                     */



    /* VID_TIS - Task Instruction (8-bit) --------------------------------------------- */
    #define VID_TIS_EXE          0x01           /* Execute the next Task                */
    #define VID_TIS_FIS          0x02           /* Force Instruction                    */
    #define VID_TIS_SKIP         0x0C           /* skip modempeg2 mode                  */
    #define VID_TIS_SKIP_MASK    0xF3           /* skip bit flags position              */
    #define NO_SKIP              0x00           /* Dont skip any picture                */
    #define SKIP_ONE_DECODE_NEXT 0x04           /* skip one pict and decode next pict   */
    #define SKIP_TWO_DECODE_NEXT 0x08           /* skip two pict and decode next pict   */
    #define SKIP_ONE_STOP_DECODE 0x0C           /* skip one pict and stop decoder       */
    #define VID_TIS_MP2          0x10           /* MPEG-2 mode                          */
    #define VID_TIS_DPR          0x20           /* decoding period                      */
    #define VID_TIS_LFR          0x80           /* load from register                   */
    #define VID_TIS_LDP          0x100          /* load pointer                         */
    #define VID_TIS_SCN          0x200          /* start code detector disable          */

    /* VID_PPR - Picture F Parameters and other parameters -----------------------------*/
    #define VID_PPR_FFH          0x0000000F     /* (4) forward_horz_f_code              */
    #define VID_PPR_BFH          0x000000F0     /* (4) backward_horz_f_code             */
    #define VID_PPR_BFH_SHIFT    4              /* Shift for BFH                        */
    #define VID_PPR_FFV          0x00000F00     /* (4) forward_vert_f_code              */
    #define VID_PPR_FFV_SHIFT    8              /* Shift for FFV                        */
    #define VID_PPR_BFV          0x0000F000     /* (4) backward_vert_f_code             */
    #define VID_PPR_BFV_SHIFT    12             /* Shift for BFV                        */
    #define VID_PPR_PST          0x00030000     /* (2) pict_struct                      */
    #define VID_PPR_TOP_FIELD    0x00010000     /* PST = 1 Top Field                    */
    #define VID_PPR_BOT_FIELD    0x00020000     /* PST = 2 Bottom field                 */
    #define VID_PPR_FRAME        0x00030000     /* PST = 3 Frame picture                */
    #define VID_PPR_DCP          0x000C0000     /* (2) intra_dc_precision               */
    #define VID_PPR_PCT          0x00300000     /* (2) pict_coding_type                 */
    #define VID_PPR_AZZ          0x00400000     /* alternate_scan                       */
    #define VID_PPR_IVF          0x00800000     /* intra_vlc_format                     */
    #define VID_PPR_QST          0x01000000     /* q_scale_type                         */
    #define VID_PPR_CMV          0x02000000     /* concealment_motion_vectors           */
    #define VID_PPR_FRM          0x04000000     /* frame_pred_frame_dct                 */
    #define VID_PPR_TFF          0x08000000     /* Top Field First                      */

    /* VID_PFLD - Presentation Field Selection -----------------------------------------*/
    #define VID_PFLD_TCEN        0x01           /* Toggle on Chroma Enable              */
    #define VID_PFLD_TYEN        0x02           /* Toggle on Luma Enable                */
    #define VID_PFLD_BTC         0x04           /* Bottom not Top for chroma            */
    #define VID_PFLD_BTY         0x08           /* Bottom not Top for luma              */

    /* VID_APFLD - PIP Presentation Field Selection ------------------------------------*/
    #define VID_APFLD_TCEN       0x01           /* Toggle on Chroma Enable              */
    #define VID_APFLD_TYEN       0x02           /* Toggle on Luma Enable                */
    #define VID_APFLD_BTC        0x04           /* Bottom not Top for chroma            */
    #define VID_APFLD_BTY        0x08           /* Bottom not Top for luma              */

    /* VID_STP - Video bitstream management setup --------------------------------------*/
    #define VID_STP_PBO          0x01           /* Prevent bit buffer overflow          */
    #define VID_STP_SSC          0x02           /* Stop on first slice                  */
    #define VID_STP_TM           0x04           /* Trick mode                           */

    /* VID_DMOD - Decoder Memory Mode: compress, H/n, V/n, ovw -------------------------*/
    #define VID_DMOD_COMP        0x03           /* Memory compression mode mask         */
    #define VID_DMOD_NO_COMP     0x00           /* no compression                       */
    #define VID_DMOD_OVW         0x40           /* Overwrite mode                       */
    #define VID_DMOD_EN          0x100          /* Enable reconstruction in main buffer */
    #define VID_DMOD_PS          0x200          /* Progressive sequence                 */

    /* VID_PMOD - Presentation Memory Mode ---------------------------------------------*/
    #define VID_PMOD_COMP        0x03           /* Memory compression mode mask         */
    #define VID_PMOD_NO_COMP     0x00           /* no compression                       */
    #define VID_PMOD_DEC         0x3C           /* H and V decimation modes             */
    #define VID_PMOD_VDEC        0x0C           /* V decimation mode                    */
    #define VID_PMOD_NO_VDEC     0x00           /* no vertical decimation               */
    #define VID_PMOD_V2_VDEC     0x04           /* V/2 decimation                       */
    #define VID_PMOD_V4_VDEC     0x08           /* V/4 decimation                       */
    #define VID_PMOD_HDEC        0x30           /* H decimation mode                    */
    #define VID_PMOD_NO_HDEC     0x00           /* no horizontal decimation             */
    #define VID_PMOD_V2_HDEC     0x10           /* H/2 decimation                       */
    #define VID_PMOD_V4_HDEC     0x20           /* H/4 decimation                       */
    #define VID_PMOD_PDL         0x40           /* Progressive display luma             */
    #define VID_PMOD_PDC         0x80           /* Progressive display chroma           */
    #define VID_PMOD_SFB         0x100          /* Secondary frame buffer               */

    /* VID_SDMOD - Secondary Reconstruction Memory Mode: H/n, V/n, ovw --------*/
    #define VID_SDMOD_DEC        0x3C           /* H and V decimation modes             */
    #define VID_SDMOD_VDEC       0x0C           /* V decimation mode                    */
    #define VID_SDMOD_NO_VDEC    0x00           /* no decimation                        */
    #define VID_SDMOD_V2_VDEC    0x04           /* V/2 decimation                       */
    #define VID_SDMOD_V4_VDEC    0x08           /* V/4 decimation                       */
    #define VID_SDMOD_HDEC       0x30           /* H decimation mode                    */
    #define VID_SDMOD_NO_HDEC    0x00           /* no decimation                        */
    #define VID_SDMOD_V2_HDEC    0x10           /* H/2 decimation                       */
    #define VID_SDMOD_V4_HDEC    0x20           /* H/4 decimation                       */
    #define VID_SDMOD_OVW        0x40           /* Overwrite mode                       */
    #define VID_SDMOD_EN         0x100          /* Enable reconstruction in main buffer */
    #define VID_SDMOD_PS         0x200          /* Progressive sequence                 */


    /* VID_APMOD - PIP Presentation Memory Mode ----------------------------------------*/
    #define VID_APMOD_DEC        0x3C           /* H and V decimation modes             */
    #define VID_APMOD_VDEC       0x0C           /* V decimation                         */
    #define VID_APMOD_NO_VDEC    0x00           /* no vertical decimation               */
    #define VID_APMOD_V2_VDEC    0x04           /* V/2 decimation                       */
    #define VID_APMOD_V4_VDEC    0x08           /* V/4 decimation                       */
    #define VID_APMOD_HDEC       0x30           /* H decimation                         */
    #define VID_APMOD_NO_HDEC    0x00           /* no horizontal decimation             */
    #define VID_APMOD_V2_HDEC    0x10           /* H/2 decimation                       */
    #define VID_APMOD_V4_HDEC    0x20           /* H/4 decimation                       */
    #define VID_APMOD_PDL        0x40           /* Progressive display luma             */
    #define VID_APMOD_PDC        0x80           /* Progressive display chroma           */
    #define VID_APMOD_SFB        0x100          /* Secondary frame buffer               */

    /* VID_ITX - Interrupt bits --------------------------------------------------------*/
    #define VID_ITX_SCH          0x0001         /* Start code header hit                */
    #define VID_ITX_HFF          0x0002         /* Header fifo nearly full              */
    #define VID_ITX_HFE          0x0004         /* Header fifo empty                    */
    #define VID_ITX_PSD          0x0008         /* Pipeline start decoding              */
    #define VID_ITX_PID          0x0010         /* Pipeline idle                        */
    #define VID_ITX_DID          0x0020         /* Decoder idle                         */
    #define VID_ITX_DUE          0x0040         /* Decoding underflow error             */
    #define VID_ITX_OTE          0x0080         /* Decoding overtime error              */
    #define VID_ITX_DSE          0x0100         /* Decoding semantic or syntax error    */
    #define VID_ITX_DOE          0x0200         /* Decoding overflow error              */
    #define VID_ITX_BBF          0x0400         /* Bit buffer nearly full               */
    #define VID_ITX_BBE          0x0800         /* Bit buffer empty                     */
    #define VID_ITX_IER          0x1000         /* Inconsistency error in PES parser    */
    #define VID_ITX_BFF          0x2000         /* Bitstream fifo full                  */
    #define VID_ITX_SBE          0x4000         /* SCD bit buffer empty                 */
    #define VID_ITX_MASK         0x7FFF         /* Mask for all interrupt bits          */

    /* VID_PES_CFG - PES parser Configuration ------------------------------------------*/
    #define VID_PES_SS           0x01           /* System stream                        */
    #define VID_PES_SDT          0x02           /* Store DTS not PTS                    */
    #define VID_PES_OFS_MASK     0x0C           /* Output format PES output     */
    #define VID_PES_OFS_PES      0x00           /* Output format PES output     */
    #define VID_PES_OFS_ES_SUBID 0x04           /* Output format ES substream ID layer removed */
    #define VID_PES_OFS_ES       0x0C           /* Output format ES "normal" */
    #define VID_PES_MODE_MASK    0x30           /* Mode mask                            */
    #define VID_PES_MODE_AUTO    0x00           /* Mode 00: Auto                        */
    #define VID_PES_MODE_MP1     0x10           /* Mode 01: MPEG-1 system stream        */
    #define VID_PES_MODE_MP2     0x20           /* Mode 10: MPEG-2 PES stream           */
    #define VID_PES_PTF          0x40           /* PES trick filtering                  */
    #define VID_PES_SP           0x80           /* Sub-Picture                          */

    /* VID_PES_ASS - DSM and TS association flags --------------------------------------*/
    #define VID_PES_TSA          0x01           /* Time Stamp association flags         */
    #define VID_PES_DSA          0x02           /* DSM association flags                */

    /* VID_HLNC - Launch start code detector -------------------------------------------*/
    #define VID_HLNC_START_NORM  0x01           /* Start code is launched normally      */
    #define VID_HLNC_START_REG   0x02           /* Start code is launched from SCDPTR   */

    /* VID_HDF - Header Data FIFO ------------------------------------------------------*/
    #define VID_HDF_DATA         0x0000FFFF     /* Header data from stream              */
    #define VID_HDF_SCM          0x00010000     /* Start code on MSB flag               */

    /* VID_DBG_INS1 - Decoder instruction part 1 ---------------------------------------*/
    #define VID_DBG_INS1_MP2     0x10000000     /* MP2 from VIDn_TIS                    */
    #define VID_DBG_INS1_PPR1    0x0FC00000     /* VIDn_PPR[27:22]                      */
    #define VID_DBG_INS1_PPR2    0x00200000     /* VIDn_PPR[17]                         */
    #define VID_DBG_INS1_PPR3    0x00100000     /* VIDn_PPR[16] xor VIDn_PPR[18]        */
    #define VID_DBG_INS1_PPR4    0x000F0000     /* VIDn_PPR[21:18]                      */
    #define VID_DBG_INS1_PPR5    0x0000FF00     /* VIDn_PPR[15:8] if MP2, else VIDn_PPR[7:0] */
    #define VID_DBG_INS1_PPR6    0x000000FF     /* VIDn_PPR[7:0]                        */

    /* VID_DBG_INS2 - Decoder instruction part 2 ---------------------------------------*/
    #define VID_DBG_INS2_HDSH    0x0003FFE0     /* Pipeline and MCU hanshakes           */
    #define VID_DBG_INS2_EDINT   0x00000010     /* Enable Decode int (!Idle)            */
    #define VID_DBG_INS2_IDLE    0x00000008     /* Idle                                 */
    #define VID_DBG_INS2_SSEL    0x00000007     /* Stream Select (1 to 5, 6 when Idle)  */

#elif defined(VIDEO_5528)

    #define VIDEO_CFG_BASE_ADDRESS      ST5528_VIDEO_BASE_ADDRESS

    #define VIDEO_REG_OFFSET            0x300
    #define VID1            (ST5528_VID1_BASE_ADDRESS + VIDEO_REG_OFFSET)
    #define VID2            (ST5528_VID2_BASE_ADDRESS + VIDEO_REG_OFFSET)

    /* ---------------- Configuration registers
    */
    #define VID_CFG_DECPR        0x000          /* Decoding priority setup */
    #define VID_CFG_VDCTL        0x008          /* Video Decoder Control */
    #define VID_CFG_VSEL         0x00c          /* Vsync Selection */


    /* VID 1 */

    #define VID_QMWI1            0x100          /* Quantization matrix data, intra table */
    #define VID_QMWNI1           0x180          /* Quantization matrix data, non intra table */

    /*  VID 2 */

    #define VID_QMWI2            0x140          /* Quantization matrix data, intra table */
    #define VID_QMWNI2           0x1C0          /* Quantization matrix data, non intra table */


    /* ---------------- Stream dependent registers
    */
    #define VID_TIS             0x000           /* Task Instruction */
    #define VID_PPR             0x004           /* Picture F Parameters and other parameters */
    #define VID_PRS             0x008           /* Pipe Reset */
    #define VID_SRS             0x00c           /* Soft Reset */
    #define VID_ITM             0x0f0           /* Interrupt mask */
    #define VID_ITS             0x0f4           /* Interrupt status */
    #define VID_STA             0x0f8           /* Status register */
    #define VID_DFH             0x100           /* Decoded Frame Height (in MB) */
    #define VID_DFS             0x104           /* Decoded Frame Size (in MB) */
    #define VID_DFW             0x108           /* Decoded Frame Width (in MB) */
    #define VID_BBG             0x10C           /* Bit Buffer start address */
    #define VID_BBL             0x110           /* Bit Buffer level */
    #define VID_BBS             0x114           /* Bit Buffer end address */
    #define VID_BBT             0x118           /* Bit Buffer Threshold */
    #define VID_BBTL            0x11c           /* Bit Buffer Threshold Low */
    #define VID_CDB             0x120           /* Compressed Data Buffer (configuration) */
    #define VID_CDPTR           0x124           /* Compressed Data Pointer */
    #define VID_CDLP            0x128           /* Compressed Data Load Pointer */
    #define VID_CDPTR_CUR       0x12c           /* Current Value of Compressed Data Pointer */
    #define VID_CDWL            0x130           /* Top address limit of CD write in Trick mode */
    #define VID_SCDB            0x134           /* Start Code Detector (configuration) */
    #define VID_SCDRL           0x138           /* Start Code Detector Read Limit */
    #define VID_SCDPTR_CUR      0x13c           /* Current Value of Start Code Detector Pointer */
    #define VID_VLDB            0x140           /* VLD Buffer (configuration) */
    #define VID_VLD_PTR_CUR     0x144           /* Current Value of VLD Read Pointer */
    #define VID_VLDRL           0x148           /* VLD Read Limit */
    #define VID_SCDUP           0x14c           /* Update Start Code Detector Pointer */
    #define VID_SCDUPTR         0x150           /* Start Code Detector Update Pointer */
    #define VID_VLDUP           0x154           /* Update VLD Pointer  */
    #define VID_VLDUPTR         0x158           /* VLD Update Pointer */
    #define VID_VLDPTR          0x15C           /* VLD start address for multi-decode & trick mode */
    #define VID_MBNM            0x180           /* Macroblock Number for the Main Reconstruction */
    #define VID_MBNS            0x184           /* Macroblock Number for the Secondary Reconstruction */
    #define VID_BCHP            0x188           /* Backward Chroma Frame Buffer */
    #define VID_BFP             0x18C           /* Backward Frame Buffer (256 bytes unit) */
    #define VID_FCHP            0x190           /* Forward Chroma Frame Buffer */
    #define VID_FFP             0x194           /* Forward Frame Buffer (256 bytes unit) */
    #define VID_RCHP            0x198           /* Reconstruction Chroma Frame Buffer */
    #define VID_RFP             0x19C           /* Reconstruction Frame Buffer (256 bytes unit) */
    #define VID_SRCHP           0x1A0           /* Secondary Reconstruction Chroma Frame Buffer */
    #define VID_SRFP            0x1A4           /* Secondary Reconstruction Frame Buffer (256 bytes unit) */
    #define VID_DMOD            0x1a8           /* Decoder Memory Mode: compress, H/n, V/n, ovw */
    #define VID_SDMOD           0x1ac           /* Secondary Reconstruction Memory Mode: compress, H/n, V/n, ovw */
    #define VID_HDF             0x200           /* Header Data FIFO */
    #define VID_HLNC            0x204           /* Launch start code detector */
    #define VID_SCD             0x208           /* Start Codes Disabled */
    #define VID_SCPTR           0x210           /* Start Code Pointer */
    #define VID_PSCPTR          0x214           /* PSC Byte address in memory */
    #define VID_SCDPTR          0x218           /* SCD address pointer for launch in Trick mode */
    #define VID_PES_CFG         0x280           /* PES parser Configuration */
    #define VID_PES_SI          0x284           /* PES Stream Id */
    #define VID_PES_FSI         0x288           /* Stream ID Filter */
    #define VID_PES_TFI         0x28c           /* Trick Filtering ID (Fast Forward ID) */
    #define VID_PES_ASS         0x290           /* DSM and TS association flags */
    #define VID_PES_DSM         0x294           /* DSM Value */
    #define VID_PES_TSL         0x2a0           /* Time-Stamp Value LSW */
    #define VID_PES_TSH         0x2a4           /* Time-Stamp Value MSB */
    #define VID_SCDC            0x2b0           /* SCD count register */
    #define VID_CDC             0x2b4           /* CD count register */

    /* ************************************************************************************** */

#elif defined (VIDEO_7710) || defined (VIDEO_5100) || defined(VIDEO_7100) || defined(VIDEO_7109)

	/* Register base address */
#if defined (VIDEO_7710)
	#define VID1            ST7710_VID1_BASE_ADDRESS
#elif defined (VIDEO_7100)
    #define VID1            ST7100_VID1_BASE_ADDRESS
#elif defined (VIDEO_7109)
    #define VID1            ST7109_VID1_BASE_ADDRESS
#else
	#define VID1            ST5100_VID1_BASE_ADDRESS
#endif
/* ??? CD Base Address */
/*    #define VID1_CDBASE     VID1_CD_BASE_ADDRESS*/
/*    #define VID2_CDBASE     VID2_CD_BASE_ADDRESS*/
/*    #define VID1_QTABLES    VID1_QTABLES_BASE_ADDRESS */
    #define VID1_QTABLES   	VID1
/*    #define VID2_QTABLES    VID2_QTABLES_BASE_ADDRESS*/

	#define VIDEO_CFG_BASE_ADDRESS	VID1

    /* ---------------- Moved registers
    */
/*    #define VID_CDI              0x000          *//* CD Input from host interface */
    #define VID_QMWI             0x100          /* Quantization matrix data, intra table */
    #define VID_QMWNI            0x180          /* Quantization matrix data, non intra table */


    /* ---------------- Configuration registers
    */
/*    #define VID_CFG_DECPR        0x000          *//* Decoding priority setup */
/*    #define VID_CFG_PORD         0x004          *//* Priority Order of decoding processes */
/*    #define VID_CFG_VDCTL        0x008          *//* Video Decoder Control */
/*    #define VID_CFG_VSEL         0x00c          *//* Vsync Selection */
    #define VID_CFG_VIDIC	   	0x010          /* Video Decoder Interconnect Configuration */
    #define VID_EXE				0x008          /* Threshold Period */

    /* ---------------- Stream independent debug registers
    */
    #define VID_DBG_REQS         0x040          /* Request Counter */
    #define VID_DBG_REQC         0x044          /* Request State */
/*    #define VID_DBG_INS1         0x060          *//* Debug Reg 1 (Current instruction in video decoder 1st part) */
    #define VID_DBG_DBG2         0x064          /* Debug Reg 2 (Current instruction in video decoder 2nd part) */
    #define VID_DBG_VLDS         0x068          /* VLD Macro Block Number */
    #define VID_DBG_VMBN         0x06C          /* VLD State */
    #define VID_DBG_MBE0         0x070          /* Macro block Error statistic [0 to 3] */
    #define VID_DBG_MBE1         0x074          /* Macro block Error statistic [4 to 7] */
    #define VID_DBG_MBE2         0x078          /* Macro block Error statistic [8 to 11] */
    #define VID_DBG_MBE3         0x07C          /* Macro block Error statistic [12 to 15] */
#if defined (VIDEO_7710) || defined(VIDEO_7100) || defined(VIDEO_7109)
    #define VID_FRN		         0x014          /* Force Row Number */
#endif

    /* ---------------- Stream dependent registers
    */
    #define VID_TIS             0x300           /* Task Instruction */
    #define VID_PPR             0x304           /* Picture F Parameters and other parameters */
/*    #define VID_PRS             0x008           *//* Pipe Reset */
    #define VID_SRS             0x30c           /* Soft Reset */
/*    #define VID_THI             0x020           *//* Threshold Inactive */
/*    #define VID_PTI             0x024           *//* Panic Time */
/*    #define VID_TRA             0x028           *//* Panic Transition */
/*    #define VID_PMA             0x02c           *//* Panic Maximum */
    #define VID_ITM             0x3f0           /* Interrupt mask */
    #define VID_ITS             0x3f4           /* Interrupt status */
    #define VID_STA             0x3f8           /* Status register */
    #define VID_DFH             0x400           /* Decoded Frame Height (in MB) */
    #define VID_DFS             0x404           /* Decoded Frame Size (in MB) */
    #define VID_DFW             0x408           /* Decoded Frame Width (in MB) */

/* caution */
/* BBS used to be bit buffer stop */
/* BBG used to be bit buffer start */
    #define VID_BBS             0x40C           /* Bit Buffer start address */
	#define VID_BBG				VID_BBS
/*    #define VID_BBL             0x110           *//* Bit Buffer level */
    #define VID_BBE             0x414           /* Bit Buffer end address */
/*    #define VID_BBT             0x118           *//* Bit Buffer Threshold */
/*    #define VID_BBTL            0x11c           *//* Bit Buffer Threshold Low */
/*    #define VID_CDB             0x120           *//* Compressed Data Buffer (configuration) */
/*    #define VID_CDPTR           0x124           *//* Compressed Data Pointer */
/*    #define VID_CDLP            0x128           *//* Compressed Data Load Pointer */
/*    #define VID_CDPTR_CUR       0x12c           *//* Current Value of Compressed Data Pointer */
/*    #define VID_CDWL            0x130           *//* Top address limit of CD write in Trick mode */
/*    #define VID_SCDB            0x134           *//* Start Code Detector (configuration) */
/*    #define VID_SCDRL           0x138           *//* Start Code Detector Read Limit */
/*    #define VID_SCDPTR_CUR      0x13c           *//* Current Value of Start Code Detector Pointer */
/*    #define VID_VLDB            0x140           *//* VLD Buffer (configuration) */
/*    #define VID_VLDPTR_CUR      0x144           *//* Current Value of VLD Read Pointer */
     #define VID_VLDRL           0x448           /* VLD Read Limit */
/*    #define VID_SCDUP           0x14c           *//* Update Start Code Detector Pointer */
/*    #define VID_SCDUPTR         0x150           *//* Start Code Detector Update Pointer */
/*    #define VID_VLDUP           0x154           *//* Update VLD Pointer  */
/*    #define VID_VLDUPTR         0x158           *//* VLD Update Pointer */
    #define VID_VLDPTR          0x45C           /* VLD start address for multi-decode & trick mode */
    #define VID_MBNM            0x480           /* Macroblock Number for the Main Reconstruction */
#if defined (VIDEO_7710) || defined(VIDEO_7100) || defined(VIDEO_7109)
    #define VID_MBNS            0x184           /* Macroblock Number for the Secondary Reconstruction */
	#define VID_RCM				0x4ac			/* Reconstruction mode */
#endif
    #define VID_BCHP            0x488           /* Backward Chroma Frame Buffer */
    #define VID_BFP             0x48C           /* Backward Frame Buffer (256 bytes unit) */
    #define VID_FCHP            0x490           /* Forward Chroma Frame Buffer */
    #define VID_FFP             0x494           /* Forward Frame Buffer (256 bytes unit) */
    #define VID_RCHP            0x498           /* Reconstruction Chroma Frame Buffer */
    #define VID_RFP             0x49C           /* Reconstruction Frame Buffer (256 bytes unit) */
#if defined (VIDEO_7710) || defined(VIDEO_7100) || defined(VIDEO_7109)
    #define VID_SRCHP           0x4A0           /* Secondary Reconstruction Chroma Frame Buffer */
    #define VID_SRFP            0x4A4           /* Secondary Reconstruction Frame Buffer (256 bytes unit) */
#endif
/*    #define VID_DMOD            0x1a8           *//* Decoder Memory Mode: compress, H/n, V/n, ovw */
/*    #define VID_SDMOD           0x1ac           *//* Secondary Reconstruction Memory Mode: compress, H/n, V/n, ovw */
/*    #define VID_HDF             0x200           *//* Header Data FIFO */
/*    #define VID_HLNC            0x204           *//* Launch start code detector */
/*    #define VID_SCD             0x208           *//* Start Codes Disabled */
/*    #define VID_SCD_TM          0x20c           *//* Bit Buffer Input Counter for Trick Mode */
/*    #define VID_SCPTR           0x210           *//* Start Code Pointer */
/*    #define VID_PSCPTR          0x214           *//* PSC Byte address in memory */
/*    #define VID_SCDPTR          0x218           *//* SCD address pointer for launch in Trick mode */
/*    #define VID_PES_CFG         0x280           *//* PES parser Configuration */
/*    #define VID_PES_SI          0x284           *//* PES Stream Id */
/*    #define VID_PES_FSI         0x288           *//* Stream ID Filter */
/*    #define VID_PES_TFI         0x28c           *//* Trick Filtering ID (Fast Forward ID) */
/*    #define VID_PES_ASS         0x290           *//* DSM and TS association flags */
/*    #define VID_PES_DSM         0x294           *//* DSM Value */
/*    #define VID_PES_DSMC        0x298           *//* PES DSM  Counter */
/*    #define VID_PES_TS0         0x2a0           *//* Time-Stamp Value LSW */
/*    #define VID_PES_TS1         0x2a4           *//* Time-Stamp Value MSB */
/*    #define VID_PES_TSC         0x2a8           *//* PES Time Stamp Counter */
/*    #define VID_PES_CTA         0x2ac           *//* Counter of Time Stamp Association */
/*    #define VID_SCDC            0x2b0           *//* SCD count register */
/*    #define VID_CDC             0x2b4           *//* CD count register */

    /* Mask for buffers addresses and pointers ---------------------------------------- */
    #define VID_FRAME_BUF_MASK   0xfffffe00     /* frame buffers : Address on 32 bits, aligned on 512 */
    #define VID_BIT_BUF_MASK     0xffffff00     /* bit buffers   : Address on 32 bits, aligned on 256 */
    #define VID_CD_WR_LIMIT_MASK 0xffffff80     /* bit buffers : Address on 32 bits, aligned on 128 */

    /* VID_CFG_VSEL - Vertical synchro selection (and validation) --------------------- */
/* Kept but no effect */
    #define VID_CFG_VSEL_SRCE_MASK  0x03
    #define VID_CFG_VSEL_VSYNC_1    0x00        /* VTG1 is used by SDMEGO2 IP           */
    #define VID_CFG_VSEL_VSYNC_2    0x01        /* VTG2 is used by SDMEGO2 IP           */
    #define VID_CFG_VSEL_DISABLE    0x02        /* Disable decoder VSync                */

    /* VID_CFG_VDCTL - Video Decoder Ctrl ----------------------------------------------------*/
/*    #define VID_CFG_VDCTL_ERO       0x02        *//* Automatic pipeline reset on overflow error */
/*    #define VID_CFG_VDCTL_ERU       0x04        *//* Automatic pipeline reset on underflow      */
/*    #define VID_CFG_VDCTL_MVC       0x08        *//* Ensure motion vector stays inside picture  */
/*    #define VID_CFG_VDCTL_RMM       0x10        *//* Reconstruct missing macroblock             */

    /* CFG_DECPR - Multi-decode configuration ----------------------------------------- */
/*    #define VID_CFG_DECPR_PS        0x02        *//* Priority Setup                       */
/*    #define VID_CFG_DECPR_FP        0x00        *//* Force Priority                       */

    /* VID_CDB - Compressed Data Buffer ----------------------------------------------- */
/*    #define VID_CDB_CM              0x01        *//* Circular Mode                        */
/*    #define VID_CDB_WLE             0x02        *//* Write Limit Enable                   */
/*    #define VID_CDB_PBO             0x04        *//* Prevent Bit buffer Overflow          */

    /* VID_CDLP - Compressed Data Load Pointer ---------------------------------------- */
/*    #define VID_CDLP_LP             0x01        *//* Load Pointer                         */
/*    #define VID_CDLP_RP             0x02        *//* Reset Parser                         */

    /* VID_TIS - Task Instruction (8-bit) --------------------------------------------- */
	/* New defines for VID_TIS*/

	#define VID_TIS_RMM				0x8		/* Reconstruct missing macroblock */
	#define VID_TIS_MVC				0x4		/* Motion Vector Check */
	#define VID_TIS_SBD				0x2		/* Simplified B Decoding */
	#define VID_TIS_OVW				0x1		/* Enable Overwrite */

 	/* kept defines for LCMPEGS1 */
/*    #define VID_TIS_EXE             0x01        *//* Execute the next Task                */
/*    #define VID_TIS_FIS             0x02        *//* Force Instruction                    */
/*    #define VID_TIS_SKIP            0x0C        *//* skip modempeg2 mode                  */
/*    #define VID_TIS_SKIP_MASK       0xF3        *//* skip bit flags position              */
/*    #define NO_SKIP                 0x00        *//* Dont skip any picture                */
/*    #define SKIP_ONE_DECODE_NEXT    0x04        *//* skip one pict and decode next pict   */
/*    #define SKIP_TWO_DECODE_NEXT    0x08        *//* skip two pict and decode next pict   */
/*    #define SKIP_ONE_STOP_DECODE    0x0c        *//* skip one pict and stop decoder       */
/*  #define VID_TIS_MP2             0x10          *//* MPEG-2 mode                          */
/*    #define VID_TIS_DPR             0x20        *//* decoding period                      */
/*    #define VID_TIS_LFR             0x40        *//* load from register                   */
/*    #define VID_TIS_LDP             0x80        *//* load pointer                         */
/*    #define VID_TIS_SCN             0x100       *//* start code detector disable          */
/*    #define VID_TIS_SPD             0x200       *//* Select DID not PID                   */
/*    #define VID_TIS_EOD             0x400       *//* End of decoding                      */
/*    #define VID_TIS_STD             0x800       *//* Still picture                        */

    /* VID_PPR - Picture F Parameters and other parameters -----------------------------*/
    #define VID_PPR_FFH             0x0000000F  /* (4) forward_horz_f_code              */
    #define VID_PPR_BFH             0x000000F0  /* (4) backward_horz_f_code             */
    #define VID_PPR_BFH_SHIFT       4           /* Shift for BFH                        */
    #define VID_PPR_FFV             0x00000F00  /* (4) forward_vert_f_code              */
    #define VID_PPR_FFV_SHIFT       8           /* Shift for FFV                        */
    #define VID_PPR_BFV             0x0000F000  /* (4) backward_vert_f_code             */
    #define VID_PPR_BFV_SHIFT       12          /* Shift for BFV                        */
    #define VID_PPR_PST             0x00030000  /* (2) pict_struct                      */
    #define VID_PPR_TOP_FIELD       0x00010000  /* PST = 1 Top Field                    */
    #define VID_PPR_BOT_FIELD       0x00020000  /* PST = 2 Bottom field                 */
    #define VID_PPR_FRAME           0x00030000  /* PST = 3 Frame picture                */
    #define VID_PPR_DCP             0x000C0000  /* (2) intra_dc_precision               */
    #define VID_PPR_PCT             0x00300000  /* (2) pict_coding_type                 */
    #define VID_PPR_AZZ             0x00400000  /* alternate_scan                       */
    #define VID_PPR_IVF             0x00800000  /* intra_vlc_format                     */
    #define VID_PPR_QST             0x01000000  /* q_scale_type                         */
    #define VID_PPR_CMV             0x02000000  /* concealment_motion_vectors           */
    #define VID_PPR_FRM             0x04000000  /* frame_pred_frame_dct                 */
    #define VID_PPR_TFF             0x08000000  /* Top Field First                      */
    #define VID_PPR_FFN             0x30000000  /* Force Field Number                   */
    #define VID_PPR_FFN_FIRST       0x10000000  /* Force first Field                    */
    #define VID_PPR_FFN_SECOND      0x30000000  /* Force second Field                   */
    #define VID_PPR_FFN_SHIFT       28          /* Shift for FFN                        */
    #define VID_PPR_MP2				0x40000000	/* MPEG2 Sequence*/

/*+ JFC (20-Nov-2001) : Need to be moved to some display driver */
    /* VID_PFLD - Presentation Field Selection -----------------------------------------*/
/*    #define VID_PFLD_TCEN           0x01        *//* Toggle on Chroma Enable              */
/*    #define VID_PFLD_TYEN           0x02        *//* Toggle on Luma Enable                */
/*    #define VID_PFLD_BTC            0x04        *//* Bottom not Top for chroma            */
/*    #define VID_PFLD_BTY            0x08        *//* Bottom not Top for luma              */

    /* VID_PMOD - Presentation Memory Mode ---------------------------------------------*/
/*    #define VID_PMOD_V2_VDEC        0x04        *//* V/2 decimation                       */
/*    #define VID_PMOD_V4_VDEC        0x08        *//* V/4 decimation                       */
/*    #define VID_PMOD_HDEC           0x30        *//* H decimation mode          */
/*    #define VID_PMOD_NO_HDEC        0x00        *//* no horizontal decimation   */
/*    #define VID_PMOD_V2_HDEC        0x10        *//* H/2 decimation                       */
/*    #define VID_PMOD_V4_HDEC        0x20        *//* H/4 decimation                       */
/*    #define VID_PMOD_PDL            0x40        *//* Progressive display luma             */
/*   #define VID_PMOD_PDC            0x80        *//* Progressive display chroma           */
/*    #define VID_PMOD_SFB            0x100       *//* Secondary frame buffer               */


    /* VID_APMOD - PIP Presentation Memory Mode ----------------------------------------*/
/*    #define VID_APMOD_DEC           0x3C        *//* H and V decimation modes   */
/*    #define VID_APMOD_VDEC          0x0C        *//* V decimation               */
/*    #define VID_APMOD_NO_VDEC       0x00        *//* no vertical decimation     */
/*   #define VID_APMOD_V2_VDEC       0x04        *//* V/2 decimation                       */
/*    #define VID_APMOD_V4_VDEC       0x08        *//* V/4 decimation                       */
/*    #define VID_APMOD_HDEC          0x30        *//* H decimation               */
/*    #define VID_APMOD_NO_HDEC       0x00        *//* no horizontal decimation             */
/*    #define VID_APMOD_V2_HDEC       0x10        *//* H/2 decimation                       */
/*    #define VID_APMOD_V4_HDEC       0x20        *//* H/4 decimation                       */
/*    #define VID_APMOD_PDL           0x40        *//* Progressive display luma             */
/*    #define VID_APMOD_PDC           0x80        *//* Progressive display chroma           */
/*    #define VID_APMOD_SFB           0x100       *//* Secondary frame buffer               */
/*+ End of JFC (20-Nov-2001) : */

    /* VID_DMOD - Decoder Memory Mode: -------------------------------------------------*/
/* kept for LCMPEGS1 */
    #define VID_DMOD_EN             0x001       /* Enable reconstruction in main buffer */
/*#ifdef VIDEO_HDMPEGO2*/
/*#if defined(VIDEO_LCMPEGH1)*/
/*    #define VID_DMOD_OVW            0x002       *//* Enable Overwrite mechanism           */
/*#endif*/

    /* VID_SDMOD - Secondary Reconstruction Memory Mode: H/n, V/n -----------------------*/
/*    #define VID_SDMOD_DEC           0x078        *//* H and V decimation modes mask        */
/*    #define VID_SDMOD_VDEC          0x060        *//* V decimation mode                    */
/*    #define VID_SDMOD_NO_VDEC       0x000        *//* no decimation                        */
/*    #define VID_SDMOD_V2_VDEC       0x020        *//* V/2 decimation                       */
/*    #define VID_SDMOD_V4_VDEC       0x040        *//* V/4 decimation                       */
/*    #define VID_SDMOD_HDEC          0x018        *//* H decimation mode mask               */
/*    #define VID_SDMOD_NO_HDEC       0x000        *//* no decimation                        */
/*    #define VID_SDMOD_V2_HDEC       0x008        *//* H/2 decimation                       */
/*    #define VID_SDMOD_V4_HDEC       0x010        *//* H/4 decimation                       */
/*    #define VID_SDMOD_EN            0x002       *//* Enable reconstruction in main buffer */
/*    #define VID_SDMOD_PS            0x001       *//* Progressive sequence                 */
/*    #define VID_SDMOD_DEC         	 0x078        */ /* H and V decimation modes mask        */

/*Reconstruction Modes*/
	#define VID_RCM_VDEC		  	 0x060
	#define	VID_RCM_NO_VDEC			 0x000
	#define VID_RCM_V2_VDEC			 0x020
	#define VID_RCM_V4_VDEC			 0x040
    #define VID_RCM_HDEC         	 0x018       /* H decimation mode mask               */
    #define VID_RCM_NO_HDEC          0x000       /* no decimation                        */
    #define VID_RCM_V2_HDEC       	 0x008       /* H/2 decimation                       */
    #define VID_RCM_V4_HDEC       	 0x010       /* H/4 decimation                       */
    #define VID_RCM_ENS            	 0x002       /* Enable reconstruction in secondary buffer */
    #define VID_RCM_PS            	 0x001       /* Progressive sequence                 */
    #define VID_RCM_ENM            	 0x004       /* Enable reconstruction in main buffer */

    /* VID_ITX - Interrupt bits --------------------------------------------------------*/

    #define VID_ITX_SCH             0x20000000  /* Start code header hit                        */

    #define VID_ITX_DID             0x00000001  /* Decoder idle                                 */
    #define VID_ITX_DOE             0x00000002  /* Decoding overflow error                      */
    #define VID_ITX_DUE             0x00000004  /* Decoding underflow error                     */
    #define VID_ITX_MSE             0x00000008  /* Semantic or syntax error detected            */
    #define VID_ITX_DSE             0x00000010  /* Decoding semantic or syntax error            */
    #define VID_ITX_VLDRL           0x00000020  /* VLD Read Limit                               */
    #define VID_ITX_R_OPC           0x00000040  /* r_opc                                        */
    #define VID_ITX_VTG_VSB         0x04000000  /* VTG Bottom Field detected            */
    #define VID_ITX_VTG_VST         0x08000000  /* VTG Top Field detected               */
    #define VID_ITX_VTG_PDVS        0x10000000  /* VTG Delayed Field detected           */

/*** DEBUG : Simulated IT ***/
/*    #define VID_ITX_HFF             0x00000080  *//* Header fifo nearly full                      */
/*    #define VID_ITX_HFE             0x00000100  *//* Header fifo empty                            */
/*    #define VID_ITX_IER             0x00000200  *//* Inconsistency error in PES parser            */
/*    #define VID_ITX_BFF             0x00000400  *//* Bitstream fifo full                          */
    #define VID_ITX_PSD             0x00000800  /* Pipeline start decoding                      */
/*    #define VID_ITX_PID             0x00001000  *//* Pipeline idle                                */
/*    #define VID_ITX_OTE             0x00002000  *//* Decoding overtime error                      */
/*    #define VID_ITX_PAN             0x00004000  *//* Threshold decoding speed has been reached    */
/*    #define VID_ITX_BBF             0x00008000  *//* Bit Buffer Nearly full                       */
/*    #define VID_ITX_BBNE            0x00010000  *//* Bit Buffer Nearly Empty */
/*    #define VID_ITX_BBE             0x00020000  *//* Bit Buffer Empty                             */
/*    #define VID_ITX_SBE             0x00040000  *//* SCD bit Buffer Empty                         */
/*    #define VID_ITX_CDWL            0x00080000  *//* Compressed Data Write Limit                  */
/*    #define VID_ITX_SCDRL           0x00100000  *//* Start Code Detector Read Limit               */
/*    #define VID_ITX_SRR             0x02000000  *//* srr                                          */
/* Fake VTG status bits */
    #define VID_ITX_MASK            0x027FFFFF  /* Mask for all interrupt bits                  */

    /* VID_PES_CFG - PES parser Configuration ------------------------------------------*/
/*    #define VID_PES_SS              0x01        *//* System stream                        */
/*    #define VID_PES_SDT             0x02        *//* Store DTS not PTS                    */
/*    #define VID_PES_OFS_MASK        0x0C        *//* Output format PES output             */
/*    #define VID_PES_OFS_PES         0x00        *//* Output format PES output             */
/*    #define VID_PES_OFS_ES_SUBID    0x04        *//* Output format ES substream ID layer removed */
/*    #define VID_PES_OFS_ES          0x0C        *//* Output format ES "normal"            */
/*    #define VID_PES_MODE_MASK       0x30        *//* Mode mask                            */
/*    #define VID_PES_MODE_AUTO       0x00        *//* Mode 00: Auto                        */
/*    #define VID_PES_MODE_MP1        0x10        *//* Mode 01: MPEG-1 system stream        */
/*    #define VID_PES_MODE_MP2        0x20        *//* Mode 10: MPEG-2 PES stream           */
/*    #define VID_PES_PTF             0x40        *//* PES trick filtering                  */

    /* VID_PES_ASS - DSM and TS association flags --------------------------------------*/
/*    #define VID_PES_TSA             0x01        *//* Time Stamp association flags         */
/*    #define VID_PES_DSA             0x02        *//* DSM association flags                */

    /* VID_HLNC - Launch start code detector -------------------------------------------*/
/*    #define VID_HLNC_LNC_MASK       0x03        *//* Mask for LNC bits                    */
/*    #define VID_HLNC_START_NORM     0x01        *//* Start code is launched normally      */
/*    #define VID_HLNC_START_REG      0x02        *//* Start code is launched from SCDPTR   */
/*    #define VID_HLNC_SCNT           0x04        *//* Select Counter                       */
/*    #define VID_HLNC_RTA            0x08        *//* Restart Time stamp Association       */

    /* VID_HDF - Header Data FIFO ------------------------------------------------------*/
/*    #define VID_HDF_DATA            0x0000FFFF  *//* Header data from stream              */
/*    #define VID_HDF_SCM             0x00010000  *//* Start code on MSB flag               */

    /* VID_SCD - Start Code Disabled ---------------------------------------------------*/
    #define VID_SCD_PSC                   0x001 /* Picture Start Codes (00)                                   */
    #define VID_SCD_SSC                   0x002 /* all Slice Start Codes (01 to AF)                           */
    #define VID_SCD_SSCF                  0x004 /* all SSC except the first ones after a PSC                  */
    #define VID_SCD_UDSC                  0x008 /* User Data Start Codes (B2)                                 */
    #define VID_SCD_SHC                   0x010 /* Sequence Header Codes (B3)                                 */
    #define VID_SCD_SEC                   0x020 /* Sequence Error Codes (B4)                                  */
    #define VID_SCD_ESC                   0x040 /* Extension Start Codes (B5)                                 */
    #define VID_SCD_SENC                  0x080 /* Sequence End Codes (B7)                                    */
    #define VID_SCD_GSC                   0x100 /* Group Start Codes (B8)                                     */
    #define VID_SCD_SYSC                  0x200 /* System start Codes (B9 through FF)                         */
    #define VID_SCD_RSC                   0x400 /* Reserved Start Codes (B0, B1, B6)                          */
    #define VID_SCD_IGNORE_ALL            0x7ff /* Ignore all start codes (for specific validation only)      */
    #define VID_SCD_IGNORE_ALL_BUT_SEQ    0x7ef /* Ignore all start code except Sequence header (B3)          */
    #define VID_SCD_ENABLE_ALL_BUT_SLICE  VID_SCD_SSC /* Enable all start code except all slices (01 to AF)   */

    /* VID_SCDB - Start Code Detector Buffer ------------------------------------------- */
/*    #define VID_SCDB_CM              0x01       *//* Circular Mode                         */
/*    #define VID_SCDB_RLE             0x02       *//* Read Limit Enable                     */
/*    #define VID_SCDB_PRNW            0x04       *//* Prevent Prevent Read Not Write        */

    /* VID_SCDUP - Update Start Code Pointer ------------------------------------------------- */
/*    #define VID_SCDUP_UP             0x01       *//* Update the start code detector read pointer */

    /* VID_VLDB - VLD Buffer ----------------------------------------------------------- */
/*    #define VID_VLDB_CM              0x01       *//* Circular Mode                         */
/*    #define VID_VLDB_RLE             0x02       *//* Read Limit Enable                     */
/*    #define VID_VLDB_PRNW            0x04       *//* Prevent Prevent Read Not Write        */

    /* VID_VLDUP - Update Start Code Pointer ------------------------------------------------- */
    #define VID_VLDUP_UP             0x01       /* Update the start code detector read pointer */

    /* VID_DBG_INS1 - Decoder instruction part 1 ---------------------------------------*/
    #define VID_DBG_INS1_MP2        0x40000000  /* MP2 from VIDn_TIS                    */
    #define VID_DBG_INS1_PPR        0x02FFFFFF  /* VIDn_PPR[29:00]                      */

    /* VID_DBG_INS2 - Decoder instruction part 2 ---------------------------------------*/
    #define VID_DBG_INS2_PPE_STATE2     0x00000007  /* State of the PPE2 finite state machine   */
    #define VID_DBG_INS2_PPE_STATE1     0x00000038  /* State of the PPE1 finite state machine   */
    #define VID_DBG_INS2_HDSH           0x00003fc0  /* Pipeline and MCU hanshakes               */
    #define VID_DBG_INS2_MCU_ADDER2_REQ 0x00000040  /* Request MCU -> PEN                       */
    #define VID_DBG_INS2_MCU_ADDER1_REQ 0x00000080  /* Request MCU -> PEN                       */
    #define VID_DBG_INS2_PEN_ADDER2_ACK 0x00000100  /* Acknowledge PEN -> MCU                   */
    #define VID_DBG_INS2_PEN_ADDER1_ACK 0x00000200  /* Acknowledge PEN -> MCU                   */
    #define VID_DBG_INS2_PEE_ADDER2_ACK 0x00000400  /* Acknowledge PPE2 -> PEN                  */
    #define VID_DBG_INS2_PEE_ADDER1_ACK 0x00000800  /* Acknowledge PPE1 -> PEN                  */
    #define VID_DBG_INS2_PEN_ADDER2_REQ 0x00001000  /* Acknowledge PEN -> PPE2                  */
    #define VID_DBG_INS2_PEN_ADDER1_REQ 0x00002000  /* Acknowledge PEN -> PPE1                  */
    #define VID_DBG_INS2_IDLE           0x00004000  /* Idle                                     */
    #define VID_DBG_INS2_SSEL           0x00030000  /* Stream Select (1 or 2 when not Idle)     */

    #define VID_DBG_REQS_PES1           0x0001      /* Request of PES parser for video 1                */
    #define VID_DBG_REQS_PES1_M         0x0002      /* Request of PES parser for video 1 after mask     */
    #define VID_DBG_REQS_PES2           0x0004      /* Request of PES parser for video 2                */
    #define VID_DBG_REQS_PES2_M         0x0008      /* Request of PES parser for video 2 after mask     */
    #define VID_DBG_REQS_SCD1           0x0010      /* Request of SCD for video 1                       */
    #define VID_DBG_REQS_SCD1_M         0x0020      /* Request of SCD for video 1 after mask            */
    #define VID_DBG_REQS_SCD2           0x0040      /* Request of SCD for video 2                       */
    #define VID_DBG_REQS_SCD2_M         0x0080      /* Request of SCD for video 2 after mask            */
    #define VID_DBG_REQS_VLD            0x0100      /* Request of VLD                                   */
    #define VID_DBG_REQS_VLD_M          0x0200      /* Request of VLD after mask                        */

    #define VID_DBG_REQS_MR             0x00400     /* Request for main reconstruction                  */
    #define VID_DBG_REQS_MR_M           0x00800     /* Request for main reconstruction after mask       */
    #define VID_DBG_REQS_SR             0x01000     /* Request for secondary reconstruction             */
    #define VID_DBG_REQS_SR_M           0x02000     /* Request for secondary reconstruction after mask  */
    #define VID_DBG_REQS_PRD            0x04000     /* Request for prediction                           */
    #define VID_DBG_REQS_PRD_M          0x08000     /* Request for prediction after mask                */
    #define VID_DBG_REQS_INTRA          0x10000     /* Request prediction for intra MB                  */
#else
    #error "No video decoder defined" ;
#endif /* defined(VIDEO_SDMPEGO2) */


#ifdef __cplusplus
}
#endif

#endif /* __VIDEO_REG_H */
/* ------------------------------- End of file  video_reg.h ---------------------------- */


