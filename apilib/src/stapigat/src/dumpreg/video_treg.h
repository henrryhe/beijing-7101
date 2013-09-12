/*****************************************************************************
*
* File name   : video_treg.h
*
* Description : Video register description for test
*
* COPYRIGHT (C) ST-Microelectronics 1999.
*
* Tabs (convert to spaces) are 4 characters wide.
*
* Date          Modification                                                            Name
* ----          ------------                                                            ----
* 14-Sep-1999   Created                                                                 FC
* 14-Jan-2000   Changed all pXXX to AXXX                                                FC
* 22-Nov-2001   Add descriptions for registers of SDMEGO2 IP                            JFC
* 23-Nov-2001   Move description of video specific config register (from cfg_treg.h)    JFC
*               (prefixing them with VID and using a VIDEO_CFG_BASE_ADDRESS)
* TODO:
* =====
*****************************************************************************/


#ifndef __VIDEO_TREG_H
#define __VIDEO_TREG_H

/* Includes --------------------------------------------------------------- */

#include "dumpreg.h"
#include "video_reg.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Exported Types --------------------------------------------------------- */

/* Exported Constants ----------------------------------------------------- */

/* Registers description */
#if defined(VIDEO_SDMPEGO2) || defined(VIDEO_5528)
    static const Register_t VideoReg[] =
    {
        /* --------------------- Config  ---------------------------------------- */
        {"VID_CFG_DECPR",                   /* Decoding priority setup */
        VIDEO_CFG_BASE_ADDRESS,
        VID_CFG_DECPR,
        0x03,
        0x03,
        0,
        REG_32B,
        0
        },
#ifndef VIDEO_5528
        {"VID_CFG_PORD",                   /* Priority Order of decoding processes */
        VIDEO_CFG_BASE_ADDRESS,
        VID_CFG_PORD,
        0x0003007,
        0x0003007,
        0,
        REG_32B,
        0
        },
#endif
        {"VID_CFG_VDCTL",                   /* Video Decoder Control */
        VIDEO_CFG_BASE_ADDRESS,
        VID_CFG_VDCTL,
        0x0e,
        0x0e,
        0,
        REG_32B,
        0
        },
        {"VID_CFG_VSEL",                   /* Vsync Selection */
        VIDEO_CFG_BASE_ADDRESS,
        VID_CFG_VSEL,
        0x03,
        0x03,
        0,
        REG_32B,
        0
        },
#ifndef VIDEO_5528
        {"VID_CFG_VIDIC",                   /* Video Decoder Interconnect Configuration */
        VIDEO_CFG_BASE_ADDRESS,
        VID_CFG_VIDIC,
        0x7f,
        0x7f,
        0,
        REG_32B,
        0
        },
        {"VID_CFG_THP",                   /* Threshold Period */
        VIDEO_SDMEGO2_BASE_ADDRESS,
        VID_CFG_THP,
        0xff,
        0xff,
        0,
        REG_32B,
        0
        },
#endif

    /* --------------------- DECODER 1 ---------------------------------------- */
        {"VID1_TIS",                   /* Task Instruction */
        VID1,
        VID_TIS,
        0x7ff,
        0x7ff,
        0,
        REG_32B|REG_SPECIAL|REG_SIDE_WRITE,
        0
        },
#ifndef VIDEO_5528
        {"VID1_THI",                   /* Threshold Inactive */
        VID1,
        VID_THI,
        0xff,
        0xff,
        0,
        REG_32B,
        0
        },
        {"VID1_PTI",                   /* Panic Time */
        VID1,
        VID_PTI,
        0x00ffffff,
        0x0,
        0,
        REG_32B,
        0
        },
        {"VID1_TRA",                   /* Panic Transition */
        VID1,
        VID_TRA,
        0x0001ffff,
        0x0,
        0,
        REG_32B,
        0
        },
        {"VID1_PMA",                   /* Panic Maximum */
        VID1,
        VID_PMA,
        0x00ffffff,
        0x0,
        0,
        REG_32B,
        0
        },
#endif /* ifndef 5528 */
        {"VID1_PPR",                   /* Picture F Parameters and other parameters */
        VID1,
        VID_PPR,
        0x3fffffff,
        0x3fffffff,
        0,
        REG_32B,
        0
        },
        {"VID1_RFP",                   /* Reconstruction Frame Buffer (256 bytes unit) */
        VID1,
        VID_RFP,
        0xfffffe00,
        0xfffffe00,
        0,
        REG_32B,
        0
        },
        {"VID1_RCHP",                  /* Reconstruction Chroma Frame Buffer */
        VID1,
        VID_RCHP,
        0xfffffe00,
        0xfffffe00,
        0,
        REG_32B,
        0
        },
        {"VID1_FFP",                   /* Forward Frame Buffer (256 bytes unit) */
        VID1,
        VID_FFP,
        0xfffffe00,
        0xfffffe00,
        0,
        REG_32B,
        0
        },
        {"VID1_FCHP",                  /* Forward Chroma Frame Buffer */
        VID1,
        VID_FCHP,
        0xfffffe00,
        0xfffffe00,
        0,
        REG_32B,
        0
        },
        {"VID1_BFP",                   /* Backward Frame Buffer (256 bytes unit) */
        VID1,
        VID_BFP,
        0xfffffe00,
        0xfffffe00,
        0,
        REG_32B,
        0
        },
        {"VID1_BCHP",                  /* Backward Chroma Frame Buffer */
        VID1,
        VID_BCHP,
        0xfffffe00,
        0xfffffe00,
        0,
        REG_32B,
        0
        },
        {"VID1_SRFP",                  /* PIP Reconstruction Frame Buffer (256 bytes unit) */
        VID1,
        VID_SRFP,
        0xfffffe00,
        0xfffffe00,
        0,
        REG_32B,
        0
        },
        {"VID1_SRCHP",                 /* PIP Reconstruction Chroma Frame Buffer */
        VID1,
        VID_SRCHP,
        0xfffffe00,
        0xfffffe00,
        0,
        REG_32B,
        0
        },
        {"VID1_BBG",                   /* Bit Buffer start address */
        VID1,
        VID_BBG,
        0xffffff00,
        0xffffff00,
        0,
        REG_32B,
        0
        },
        {"VID1_BBS",                   /* Bit Buffer end address */
        VID1,
        VID_BBS,
        0xffffff00,
        0xffffff00,
        0,
        REG_32B,
        0
        },
        {"VID1_BBT",                   /* Bit Buffer threshold */
        VID1,
        VID_BBT,
        0xffffff00,
        0xffffff00,
        0,
        REG_32B,
        0
        },
        {"VID1_BBL",                   /* Bit Buffer level */
        VID1,
        VID_BBL,
        0xffffff00,
        0x0,
        0,
        REG_32B,
        0
        },
        {"VID1_BBTL",                  /* Bit Buffer Threshold Low */
        VID1,
        VID_BBTL,
        0xff,
        0xff,
        0,
        REG_32B,
        0
        },
        {"VID1_CDB",                   /* Compressed Data Buffer (configuration) */
        VID1,
        VID_CDB,
        0x7,
        0x7,
        0,
        REG_32B,
        0
        },
        {"VID1_CDPTR",                 /* Compressed Data Pointer */
        VID1,
        VID_CDPTR,
        0x7f,
        0x7f,
        0,
        REG_32B,
        0
        },
        {"VID1_CDLP",                  /* Compressed Data Load Pointer */
        VID1,
        VID_CDLP,
        0x3,
        0x3,
        0,
        REG_32B | REG_SPECIAL,
        0
        },
        {"VID1_CDPTR_CUR",             /* Current Value of Compressed Data Pointer */
        VID1,
        VID_CDPTR_CUR,
        0xffffff80,
        0x0,
        0,
        REG_32B,
        0
        },
        {"VID1_SCDB",                  /* Start Code Detector (configuration) */
        VID1,
        VID_SCDB,
        0x07,
        0x07,
        0x05,
        REG_32B,
        0
        },
        {"VID1_SCDRL",                 /* Start Code Detector Read Limit */
        VID1,
        VID_SCDRL,
        0xffffff80,
        0xffffff80,
        0,
        REG_32B,
        0
        },
        {"VID1_SCDPTR_CUR",            /* Current Value of Start Code Detector Pointer */
        VID1,
        VID_SCDPTR_CUR,
        0xffffff80,
        0x00,
        0,
        REG_32B,
        0
        },
        {"VID1_VLDB",                  /* VLD Buffer (configuration) */
        VID1,
        VID_VLDB,
        0x07,
        0x07,
        0x05,
        REG_32B,
        0
        },
        {"VID1_VLD_PTR_CUR",           /* Current Value of VLD Read Pointer */
        VID1,
        VID_VLD_PTR_CUR,
        0xffffff00,
        0x00,
        0,                  /* more exactly value of VIDn_BBG */
        REG_32B,
        0
        },
        {"VID1_VLDRL",                 /* VLD Read Limit */
        VID1,
        VID_VLDRL,
        0xffffff00,
        0xffffff00,
        0,
        REG_32B,
        0
        },
        {"VID1_SCDUP",                 /* Update Start Code Detector Pointer */
        VID1,
        VID_SCDUP,
        0x01,
        0x01,
        0,
        REG_32B | REG_SPECIAL | REG_SIDE_WRITE,
        0
        },
        {"VID1_SCDUPTR",               /* Start Code Detector Update Pointer */
        VID1,
        VID_SCDUPTR,
        0xffffff80,
        0xffffff80,
        0,
        REG_32B,
        0
        },
        {"VID1_VLDUP",                 /* Update VLD Pointer */
        VID1,
        VID_VLDUP,
        0x01,
        0x01,
        0,
        REG_32B | REG_SPECIAL | REG_SIDE_WRITE,
        0
        },
        {"VID1_VLDUPTR",               /* VLD Update Pointer */
        VID1,
        VID_VLDUPTR,
        0xffffff00,
        0xffffff00,
        0,
        REG_32B,
        0
        },
        {"VID1_MBNM",                  /* Macroblock Number for the Main Reconstruction */
        VID1,
        VID_MBNM,
        0x1fff,
        0x00,
        0,
        REG_32B,
        0
        },
        {"VID1_MBNS",                  /* Macroblock Number for the Secondary Reconstruction */
        VID1,
        VID_MBNS,
        0x1fff,
        0x00,
        0,
        REG_32B,
        0
        },
        {"VID1_PSCPTR",                /* PSC Byte address in memory */
        VID1,
        VID_PSCPTR,
        0xffffffff,
        0x0,
        0,
        REG_32B,
        0
        },
        {"VID1_VLDPTR",                /* VLD start address for multi-decode & trick mode */
        VID1,
        VID_VLDPTR,
        0xffffffff,
        0xffffffff,
        0,
        REG_32B,
        0
        },

        {"VID1_DFW",                   /* Decoded Frame Width (in MB) */
        VID1,
        VID_DFW,
        0x7f,
        0x7f,
        0,
        REG_32B,
        0
        },
        {"VID1_DFH",                   /* Decoded Frame Heigt (in MB) */
        VID1,
        VID_DFH,
        0x7f,
        0x7f,
        0,
        REG_32B,
        0
        },
        {"VID1_DFS",                   /* Decoded Frame Size (in MB) */
        VID1,
        VID_DFS,
        0x1fff,
        0x1fff,
        0,
        REG_32B,
        0
        },
#ifndef VIDEO_5528
        {"VID1_CDI",                   /* CD Input from host interface */
        VID1,
        VID_CDI,
        0x0,
        0xffffffff,
        0,
    /*   REG_32B | REG_EXCL_FROM_TEST, */
        REG_32B,
        0
        },
#endif
        {"VID1_DMOD",                  /* Decoder Memory Mode: compress, H/n, V/n, ovw */
        VID1,
        VID_DMOD,
        0x1,
        0x1,
        0,
        REG_32B | REG_SPECIAL,
        0
        },
        {"VID1_SDMOD",                 /* Secondary reconstruction Memory Mode: compress, H/n, V/n, ovw */
        VID1,
        VID_SDMOD,
        0x7b,
        0x7b,
        0,
        REG_32B | REG_SPECIAL,
        0
        },
        {"VID1_SRS",                   /* Soft Reset */
        VID1,
        VID_SRS,
        0x0,
        0x1,
        0,
        REG_32B|REG_SPECIAL|REG_SIDE_WRITE,           /* Reset on Write */
        0
        },
        {"VID1_PRS",                   /* Pipe Reset */
        VID1,
        VID_PRS,
        0x0,
        0x1,
        0,
        REG_32B|REG_SPECIAL|REG_SIDE_WRITE,          /* Reset on Write */
        0
        },

        {"VID1_ITM",                   /* Interrupt mask */
        VID1,
        VID_ITM,
        0x01ffffdf,
        0x01ffffdf,
        0,
        REG_32B,
        0
        },
        {"VID1_ITS",                   /* Interrupt status */
        VID1,
        VID_ITS,
        0x01ffffdf,
        0x0,
        0,
        REG_32B | REG_SPECIAL | REG_SIDE_READ,
        0
        },
        {"VID1_STA",                   /* Status register */
        VID1,
        VID_STA,
        0x01ffffdf,
        0x0,
        0,
        REG_32B,
        0
        },

        {"VID1_CDWL",                  /* Top address limit of CD write in Trick mode */
        VID1,
        VID_CDWL,
        0xffffff80,
        0xffffff80,
        0,
        REG_32B,
        0
        },
        {"VID1_SCDPTR",                /* SCD addr pointer for launch in Trick mode */
        VID1,
        VID_SCDPTR,
        0xffffffff,
        0xffffffff,
        0,
        REG_32B,
        0
        },

        {"VID1_CDC",                   /* CD count register */
        VID1,
        VID_CDC,
        0x01ffffff,
        0x0,
        4,
        REG_32B,
        0
        },
        {"VID1_SCDC",                  /* SCD count register */
        VID1,
        VID_SCDC,
        0x01fffffe,
        0x0,
        0,
        REG_32B,
        0
        },

        {"VID1_PES_CFG",               /* PES parser Configuration */
        VID1,
        VID_PES_CFG,
        0x7f,
        0x7f,
        0,
        REG_32B,
        0
        },
        {"VID1_PES_SI",                /* PES Stream Id */
        VID1,
        VID_PES_SI,
        0xff,
        0xff,
        0,
        REG_32B,
        0
        },
        {"VID1_PES_FSI",               /* Stream ID Filter */
        VID1,
        VID_PES_FSI,
        0xff,
        0xff,
        0,
        REG_32B,
        0
        },
        {"VID1_PES_TFI",               /* Trick Filtering ID (Fast Forward ID) */
        VID1,
        VID_PES_TFI,
        0xff,
        0xff,
        0,
        REG_32B,
        0
        },
        {"VID1_PES_DSM",               /* DSM Value */
        VID1,
        VID_PES_DSM,
        0xff,
        0x0,
        0,                  /* In fact reset state is undefined */
        REG_32B,
        0
        },
#ifndef VIDEO_5528
        {"VID1_PES_DSMC",              /* PES DSM Counter */
        VID1,
        VID_PES_DSMC,
        0xff,
        0x00,
        0,
        REG_32B,
        0
        },
        {"VID1_PES_TS0",               /* Time-Stamp Value LSW */
        VID1,
        VID_PES_TS0,
        0xffffffff,
        0x0,
        0,                          /* In fact reset state is undefined */
        REG_32B,
        0
        },
        {"VID1_PES_TS1",               /* Time-Stamp Value MSW */
        VID1,
        VID_PES_TS1,
        0x1,
        0x0,
        0,                          /* In fact reset state is undefined */
        REG_32B,
        0
        },
        {"VID1_PES_TSC",               /* PES Time Stamp Counter */
        VID1,
        VID_PES_TSC,
        0x3f,
        0x00,
        0,
        REG_32B,
        0
        },
        {"VID1_PES_CTA",               /* Counter of Time Stamp Association */
        VID1,
        VID_PES_CTA,
        0x01ffffff,
        0x00,
        0,                              /* In fact Reset state is undefined */
        REG_32B,
        0
        },
        {"VID1_PES_ASS",               /* DSM and TS association flags */
        VID1,
        VID_PES_ASS,
        0x03,
        0x0,
        0,              /* In fact reset state is undefined */
        REG_32B,
        0
        },
#endif
        {"VID1_HDF",                   /* Header Data FIFO */
        VID1,
        VID_HDF,
        0x0001ffff,
        0x0,
        0xffff,
        REG_32B | REG_SPECIAL | REG_SIDE_READ,
        0
        },
        {"VID1_HLNC",                  /* Launch start code detector */
        VID1,
        VID_HLNC,
        0xf,
        0xf,
        0,
        REG_32B | REG_SPECIAL,
        0
        },
        {"VID1_SCD",                   /* Start Codes Disabled */
        VID1,
        VID_SCD,
        0x7ff,
        0x7ff,
        0xFFEF,
        REG_32B,
        0
        },
#ifndef VIDEO_5528
        {"VID1_SCD_TM",                /* Bit Buffer Input Counter for Trick Mode */
        VID1,
        VID_SCD_TM,
        0x01fffffe,
        0x0,
        0,
        REG_32B,
        0
        },
#endif
        {"VID1_SCPTR",                 /* Start Code Pointer */
        VID1,
        VID_SCPTR,
        0xffffffff,
        0x0,
        0,
        REG_32B,
        0
        },

#ifdef VIDEO_5528
        {"VID1_QMWI",                  /* Quantization matrix data, intra table */
        ST5528_VIDEO_BASE_ADDRESS,
        VID_QMWI1,
        0x0,
        0xffffffff,
        0,
        REG_32B|REG_ARRAY,
        16
        },
        {"VID1_QMWNI",                 /* Quantization matrix data, non intra table */
        ST5528_VIDEO_BASE_ADDRESS,
        VID_QMWNI1,
        0x0,
        0xffffffff,
        0,
        REG_32B|REG_ARRAY,
        16
    },
#else  /* SDMPEGO2 */

        {"VID1_QMWI",                  /* Quantization matrix data, intra table */
        VID1,
        VID_QMWI,
        0x0,
        0xffffffff,
        0,
        REG_32B|REG_ARRAY,
        16
        },
        {"VID1_QMWNI",                 /* Quantization matrix data, non intra table */
        VID1,
        VID_QMWNI,
        0x0,
        0xffffffff,
        0,
        REG_32B|REG_ARRAY,
        16
    },

#endif



    /* --------------------- DECODER 2 ---------------------------------------- */

        {"VID2_TIS",                   /* Task Instruction */
        VID2,
        VID_TIS,
        0x7ff,
        0x7ff,
        0,
        REG_32B|REG_SPECIAL|REG_SIDE_WRITE,
        0
        },
#ifndef VIDEO_5528
        {"VID2_THI",                   /* Threshold Inactive */
        VID2,
        VID_THI,
        0xff,
        0xff,
        0,
        REG_32B,
        0
        },
        {"VID2_PTI",                   /* Panic Time */
        VID2,
        VID_PTI,
        0x00ffffff,
        0x0,
        0,
        REG_32B,
        0
        },
        {"VID2_TRA",                   /* Panic Transition */
        VID2,
        VID_TRA,
        0x0001ffff,
        0x0,
        0,
        REG_32B,
        0
        },
        {"VID2_PMA",                   /* Panic Maximum */
        VID2,
        VID_PMA,
        0x00ffffff,
        0x0,
        0,
        REG_32B,
        0
        },
#endif
        {"VID2_PPR",                   /* Picture F Parameters and other parameters */
        VID2,
        VID_PPR,
        0x3fffffff,
        0x3fffffff,
        0,
        REG_32B,
        0
        },
        {"VID2_RFP",                   /* Reconstruction Frame Buffer (256 bytes unit) */
        VID2,
        VID_RFP,
        0xfffffe00,
        0xfffffe00,
        0,
        REG_32B,
        0
        },
        {"VID2_RCHP",                  /* Reconstruction Chroma Frame Buffer */
        VID2,
        VID_RCHP,
        0xfffffe00,
        0xfffffe00,
        0,
        REG_32B,
        0
        },
        {"VID2_FFP",                   /* Forward Frame Buffer (256 bytes unit) */
        VID2,
        VID_FFP,
        0xfffffe00,
        0xfffffe00,
        0,
        REG_32B,
        0
        },
        {"VID2_FCHP",                  /* Forward Chroma Frame Buffer */
        VID2,
        VID_FCHP,
        0xfffffe00,
        0xfffffe00,
        0,
        REG_32B,
        0
        },
        {"VID2_BFP",                   /* Backward Frame Buffer (256 bytes unit) */
        VID2,
        VID_BFP,
        0xfffffe00,
        0xfffffe00,
        0,
        REG_32B,
        0
        },
        {"VID2_BCHP",                  /* Backward Chroma Frame Buffer */
        VID2,
        VID_BCHP,
        0xfffffe00,
        0xfffffe00,
        0,
        REG_32B,
        0
        },
        {"VID2_SRFP",                  /* PIP Reconstruction Frame Buffer (256 bytes unit) */
        VID2,
        VID_SRFP,
        0xfffffe00,
        0xfffffe00,
        0,
        REG_32B,
        0
        },
        {"VID2_SRCHP",                 /* PIP Reconstruction Chroma Frame Buffer */
        VID2,
        VID_SRCHP,
        0xfffffe00,
        0xfffffe00,
        0,
        REG_32B,
        0
        },
        {"VID2_BBG",                   /* Bit Buffer start address */
        VID2,
        VID_BBG,
        0xffffff00,
        0xffffff00,
        0,
        REG_32B,
        0
        },
        {"VID2_BBS",                   /* Bit Buffer end address */
        VID2,
        VID_BBS,
        0xffffff00,
        0xffffff00,
        0,
        REG_32B,
        0
        },
        {"VID2_BBT",                   /* Bit Buffer threshold */
        VID2,
        VID_BBT,
        0xffffff00,
        0xffffff00,
        0,
        REG_32B,
        0
        },
        {"VID2_BBL",                   /* Bit Buffer level */
        VID2,
        VID_BBL,
        0xffffff00,
        0x0,
        0,
        REG_32B,
        0
        },
        {"VID2_BBTL",                  /* Bit Buffer Threshold Low */
        VID2,
        VID_BBTL,
        0xff,
        0xff,
        0,
        REG_32B,
        0
        },
        {"VID2_CDB",                   /* Compressed Data Buffer (configuration) */
        VID2,
        VID_CDB,
        0x7,
        0x7,
        0,
        REG_32B,
        0
        },
        {"VID2_CDPTR",                 /* Compressed Data Pointer */
        VID2,
        VID_CDPTR,
        0x7f,
        0x7f,
        0,
        REG_32B,
        0
        },
        {"VID2_CDLP",                  /* Compressed Data Load Pointer */
        VID2,
        VID_CDLP,
        0x3,
        0x3,
        0,
        REG_32B | REG_SPECIAL,
        0
        },
        {"VID2_CDPTR_CUR",             /* Current Value of Compressed Data Pointer */
        VID2,
        VID_CDPTR_CUR,
        0xffffff80,
        0x0,
        0,
        REG_32B,
        0
        },
        {"VID2_SCDB",                  /* Start Code Detector (configuration) */
        VID2,
        VID_SCDB,
        0x07,
        0x07,
        0x05,
        REG_32B,
        0
        },
        {"VID2_SCDRL",                 /* Start Code Detector Read Limit */
        VID2,
        VID_SCDRL,
        0xffffff80,
        0xffffff80,
        0,
        REG_32B,
        0
        },
        {"VID2_SCDPTR_CUR",            /* Current Value of Start Code Detector Pointer */
        VID2,
        VID_SCDPTR_CUR,
        0xffffff80,
        0x00,
        0,
        REG_32B,
        0
        },
        {"VID2_VLDB",                  /* VLD Buffer (configuration) */
        VID2,
        VID_VLDB,
        0x07,
        0x07,
        0x05,
        REG_32B,
        0
        },
        {"VID2_VLD_PTR_CUR",           /* Current Value of VLD Read Pointer */
        VID2,
        VID_VLD_PTR_CUR,
        0xffffff00,
        0x00,
        0,                  /* more exactly value of VIDn_BBG */
        REG_32B,
        0
        },
        {"VID2_VLDRL",                 /* VLD Read Limit */
        VID2,
        VID_VLDRL,
        0xffffff00,
        0xffffff00,
        0,
        REG_32B,
        0
        },
        {"VID2_SCDUP",                 /* Update Start Code Detector Pointer */
        VID2,
        VID_SCDUP,
        0x01,
        0x01,
        0,
        REG_32B | REG_SPECIAL | REG_SIDE_WRITE,
        0
        },
        {"VID2_SCDUPTR",               /* Start Code Detector Update Pointer */
        VID2,
        VID_SCDUPTR,
        0xffffff80,
        0xffffff80,
        0,
        REG_32B,
        0
        },
        {"VID2_VLDUP",                 /* Update VLD Pointer */
        VID2,
        VID_VLDUP,
        0x01,
        0x01,
        0,
        REG_32B | REG_SPECIAL | REG_SIDE_WRITE,
        0
        },
        {"VID2_VLDUPTR",               /* VLD Update Pointer */
        VID2,
        VID_VLDUPTR,
        0xffffff00,
        0xffffff00,
        0,
        REG_32B,
        0
        },
        {"VID2_MBNM",                  /* Macroblock Number for the Main Reconstruction */
        VID2,
        VID_MBNM,
        0x1fff,
        0x00,
        0,
        REG_32B,
        0
        },
        {"VID2_MBNS",                  /* Macroblock Number for the Secondary Reconstruction */
        VID2,
        VID_MBNS,
        0x1fff,
        0x00,
        0,
        REG_32B,
        0
        },
        {"VID2_PSCPTR",                /* PSC Byte address in memory */
        VID2,
        VID_PSCPTR,
        0xffffffff,
        0x0,
        0,
        REG_32B,
        0
        },
        {"VID2_VLDPTR",                /* VLD start address for multi-decode & trick mode */
        VID2,
        VID_VLDPTR,
        0xffffffff,
        0xffffffff,
        0,
        REG_32B,
        0
        },

        {"VID2_DFW",                   /* Decoded Frame Width (in MB) */
        VID2,
        VID_DFW,
        0x7f,
        0x7f,
        0,
        REG_32B,
        0
        },
        {"VID2_DFH",                   /* Decoded Frame Heigt (in MB) */
        VID2,
        VID_DFH,
        0x7f,
        0x7f,
        0,
        REG_32B,
        0
        },
        {"VID2_DFS",                   /* Decoded Frame Size (in MB) */
        VID2,
        VID_DFS,
        0x1fff,
        0x1fff,
        0,
        REG_32B,
        0
        },
#ifndef VIDEO_5528
        {"VID2_CDI",                   /* CD Input from host interface */
        VID2,
        VID_CDI,
        0x0,
        0xffffffff,
        0,
    /*   REG_32B | REG_EXCL_FROM_TEST, */
        REG_32B,
        0
        },
#endif
        {"VID2_DMOD",                  /* Decoder Memory Mode: compress, H/n, V/n, ovw */
        VID2,
        VID_DMOD,
        0x1,
        0x1,
        0,
        REG_32B | REG_SPECIAL,
        0
        },
        {"VID2_SDMOD",                 /* Secondary reconstruction Memory Mode: compress, H/n, V/n, ovw */
        VID2,
        VID_SDMOD,
        0x7b,
        0x7b,
        0,
        REG_32B | REG_SPECIAL,
        0
        },
        {"VID2_SRS",                   /* Soft Reset */
        VID2,
        VID_SRS,
        0x0,
        0x1,
        0,
        REG_32B|REG_SPECIAL|REG_SIDE_WRITE,           /* Reset on Write */
        0
        },
        {"VID2_PRS",                   /* Pipe Reset */
        VID2,
        VID_PRS,
        0x0,
        0x1,
        0,
        REG_32B|REG_SPECIAL|REG_SIDE_WRITE,          /* Reset on Write */
        0
        },

        {"VID2_ITM",                   /* Interrupt mask */
        VID2,
        VID_ITM,
        0x01ffffdf,
        0x01ffffdf,
        0,
        REG_32B,
        0
        },
        {"VID2_ITS",                   /* Interrupt status */
        VID2,
        VID_ITS,
        0x01ffffdf,
        0x0,
        0,
        REG_32B | REG_SPECIAL | REG_SIDE_READ,
        0
        },
        {"VID2_STA",                   /* Status register */
        VID2,
        VID_STA,
        0x01ffffdf,
        0x0,
        0,
        REG_32B,
        0
        },

        {"VID2_CDWL",                  /* Top address limit of CD write in Trick mode */
        VID2,
        VID_CDWL,
        0xffffff80,
        0xffffff80,
        0,
        REG_32B,
        0
        },
        {"VID2_SCDPTR",                /* SCD addr pointer for launch in Trick mode */
        VID2,
        VID_SCDPTR,
        0xffffffff,
        0xffffffff,
        0,
        REG_32B,
        0
        },

        {"VID2_CDC",                   /* CD count register */
        VID2,
        VID_CDC,
        0x01ffffff,
        0x0,
        4,
        REG_32B,
        0
        },
        {"VID2_SCDC",                  /* SCD count register */
        VID2,
        VID_SCDC,
        0x01fffffe,
        0x0,
        0,
        REG_32B,
        0
        },

        {"VID2_PES_CFG",               /* PES parser Configuration */
        VID2,
        VID_PES_CFG,
        0x7f,
        0x7f,
        0,
        REG_32B,
        0
        },
        {"VID2_PES_SI",                /* PES Stream Id */
        VID2,
        VID_PES_SI,
        0xff,
        0xff,
        0,
        REG_32B,
        0
        },
        {"VID2_PES_FSI",               /* Stream ID Filter */
        VID2,
        VID_PES_FSI,
        0xff,
        0xff,
        0,
        REG_32B,
        0
        },
        {"VID2_PES_TFI",               /* Trick Filtering ID (Fast Forward ID) */
        VID2,
        VID_PES_TFI,
        0xff,
        0xff,
        0,
        REG_32B,
        0
        },
        {"VID2_PES_DSM",               /* DSM Value */
        VID2,
        VID_PES_DSM,
        0xff,
        0x0,
        0,                  /* In fact reset state is undefined */
        REG_32B,
        0
        },
#ifndef VIDEO_5528
        {"VID2_PES_DSMC",              /* PES DSM Counter */
        VID2,
        VID_PES_DSMC,
        0xff,
        0x00,
        0,
        REG_32B,
        0
        },
        {"VID2_PES_TS0",               /* Time-Stamp Value LSW */
        VID2,
        VID_PES_TS0,
        0xffffffff,
        0x0,
        0,                          /* In fact reset state is undefined */
        REG_32B,
        0
        },
        {"VID2_PES_TS1",               /* Time-Stamp Value MSW */
        VID2,
        VID_PES_TS1,
        0x1,
        0x0,
        0,                          /* In fact reset state is undefined */
        REG_32B,
        0
        },
        {"VID2_PES_TSC",               /* PES Time Stamp Counter */
        VID2,
        VID_PES_TSC,
        0x3f,
        0x00,
        0,
        REG_32B,
        0
        },
        {"VID2_PES_CTA",               /* Counter of Time Stamp Association */
        VID2,
        VID_PES_CTA,
        0x01ffffff,
        0x00,
        0,                              /* In fact Reset state is undefined */
        REG_32B,
        0
        },
        {"VID2_PES_ASS",               /* DSM and TS association flags */
        VID2,
        VID_PES_ASS,
        0x03,
        0x0,
        0,              /* In fact reset state is undefined */
        REG_32B,
        0
        },
#endif
        {"VID2_HDF",                   /* Header Data FIFO */
        VID2,
        VID_HDF,
        0x0001ffff,
        0x0,
        0xffff,
        REG_32B | REG_SPECIAL | REG_SIDE_READ,
        0
        },
        {"VID2_HLNC",                  /* Launch start code detector */
        VID2,
        VID_HLNC,
        0xf,
        0xf,
        0,
        REG_32B | REG_SPECIAL,
        0
        },
        {"VID2_SCD",                   /* Start Codes Disabled */
        VID2,
        VID_SCD,
        0x7ff,
        0x7ff,
        0xFFEF,
        REG_32B,
        0
        },
#ifndef VIDEO_5528
        {"VID2_SCD_TM",                /* Bit Buffer Input Counter for Trick Mode */
        VID2,
        VID_SCD_TM,
        0x01fffffe,
        0x0,
        0,
        REG_32B,
        0
        },
#endif
        {"VID2_SCPTR",                 /* Start Code Pointer */
        VID2,
        VID_SCPTR,
        0xffffffff,
        0x0,
        0,
        REG_32B,
        0
        },

#ifdef VIDEO_5528
        {"VID2_QMWI",                  /* Quantization matrix data, intra table */
        ST5528_VIDEO_BASE_ADDRESS,
        VID_QMWI2,
        0x0,
        0xffffffff,
        0,
        REG_32B|REG_ARRAY,
        16
        },
        {"VID2_QMWNI",                 /* Quantization matrix data, non intra table */
        ST5528_VIDEO_BASE_ADDRESS,
        VID_QMWNI2,
        0x0,
        0xffffffff,
        0,
        REG_32B|REG_ARRAY,
        16
    },
#else  /* SDMPEGO2 */

        {"VID2_QMWI",                  /* Quantization matrix data, intra table */
        VID2,
        VID_QMWI,
        0x0,
        0xffffffff,
        0,
        REG_32B|REG_ARRAY,
        16
        },
        {"VID2_QMWNI",                 /* Quantization matrix data, non intra table */
        VID2,
        VID_QMWNI,
        0x0,
        0xffffffff,
        0,
        REG_32B|REG_ARRAY,
        16
    },

#endif

#ifndef VIDEO_5528

    /* --------------------- DEBUG  ---------------------------------------- */
        {"VID_DBG_REQS",               /* Request Counter */
        VIDEO_SDMEGO2_BASE_ADDRESS,
        VID_DBG_REQS,
        0xffff,
        0x00,
        0,
        REG_32B,
        0
        },
        {"VID_DBG_REQC",               /* Request State */
        VIDEO_SDMEGO2_BASE_ADDRESS,
        VID_DBG_REQC,
        0x00,               /*+ JFC (22-Nov-2001) : To be defined */
        0x00,
        0,
        REG_32B,
        0
        },
        {"VID_DBG_INS1",               /* Current instruction in decoder part 1 */
        VIDEO_SDMEGO2_BASE_ADDRESS,
        VID_DBG_INS1,
        0x1fffffff,
        0x00000000,
        0,
        REG_32B,
        0
        },
        {"VID_DBG_INS2",               /* Current instruction in decoder part 2 */
        VIDEO_SDMEGO2_BASE_ADDRESS,
        VID_DBG_INS2,
        0x0007ffff,
        0x00000000,
        0,
        REG_32B,
        0
        }
        {"VID_DBG_VMBN",               /* VLD Macro Block Number */
        VIDEO_SDMEGO2_BASE_ADDRESS,
        VID_DBG_VMBN,
        0x1fff,
        0x00,
        0,
        REG_32B,
        0
        },
        {"VID_DBG_VLDS",               /* VLD State */
        VIDEO_SDMEGO2_BASE_ADDRESS,
        VID_DBG_VLDS,
        0xff,
        0x00,
        0xff,
        REG_32B,
        0
        },
        {"VID_DBG_MBE0",               /* Macro block Error statistic [0 to 3] */
        VIDEO_SDMEGO2_BASE_ADDRESS,
        VID_DBG_MBE0,
        0xffffffff,
        0x00,
        0,
        REG_32B,
        0
        },
        {"VID_DBG_MBE1",               /* Macro block Error statistic [4 to 7] */
        VIDEO_SDMEGO2_BASE_ADDRESS,
        VID_DBG_MBE1,
        0xffffffff,
        0x00,
        0,
        REG_32B,
        0
        },
        {"VID_DBG_MBE2",               /* Macro block Error statistic [7 to 11] */
        VIDEO_SDMEGO2_BASE_ADDRESS,
        VID_DBG_MBE2,
        0xffffffff,
        0x00,
        0,
        REG_32B,
        0
        },
        {"VID_DBG_MBE3",               /* Macro block Error statistic [12 to 15] */
        VIDEO_SDMEGO2_BASE_ADDRESS,
        VID_DBG_MBE3,
        0xffffffff,
        0x00,
        0,
        REG_32B,
        0
        },
#endif
    };





#elif   defined(VIDEO_7015) || defined(VIDEO_7020)

    static const Register_t VideoReg[] =
    {
        /* ---------------- Configuration registers (moved from cfg_treg.h to be homogeneous with SDMPEGO2 IP)
        */
        {"VID_CFG_CMPEPS",              /* Epsilon value for compression/decompression */
        VIDEO_CFG_BASE_ADDRESS,
        VID_CFG_CMPEPS,
        0xffff,
        0xffff,
        0x0,
        REG_32B,
        0
        },
        {"VID_CFG_PORD",                /* Priority Order in LMC */
        VIDEO_CFG_BASE_ADDRESS,
        VID_CFG_PORD,
        0x1f,
        0x1f,
        0x0,
        REG_32B,
        0
        },
        {"VID_CFG_VDCTL",               /* Video Decoder Ctrl */
        VIDEO_CFG_BASE_ADDRESS,
        VID_CFG_VDCTL,
        0x1f,
        0x1f,
        0x0,
        REG_32B,
        0
        },
        {"VID_CFG_DECPR",               /* Multi-decode configuration */
        VIDEO_CFG_BASE_ADDRESS,
        VID_CFG_DECPR,
        0x1ffff,
        0x1ffff,
        0x0,
        REG_32B,
        0
        },
        {"VID_CFG_VSEL",                /* VSync selection for decode Synchronisation */
        VIDEO_CFG_BASE_ADDRESS,
        VID_CFG_VSEL,
        0x3,
        0x3,
        0x0,
        REG_32B,
        0
        },

        /* --------------------- DECODER 1 ---------------------------------------- */
        {"VID1_TIS",                   /* Task Instruction */
        VID1,
        VID_TIS,
        0x3bf,
        0x3bf,
        0,
        REG_32B|REG_SPECIAL,
        0
        },
        {"VID1_PPR",                   /* Picture F Parameters and other parameters */
        VID1,
        VID_PPR,
        0x0fffffff,
        0x0fffffff,
        0,
        REG_32B,
        0
        },
        {"VID1_RFP",                   /* Reconstruction Frame Buffer (256 bytes unit) */
        VID1,
        VID_RFP,
        0x3fffe00,
        0x3fffe00,
        0,
        REG_32B,
        0
        },
        {"VID1_RCHP",                  /* Reconstruction Chroma Frame Buffer */
        VID1,
        VID_RCHP,
        0x3fffe00,
        0x3fffe00,
        0,
        REG_32B,
        0
        },
        {"VID1_FFP",                   /* Forward Frame Buffer (256 bytes unit) */
        VID1,
        VID_FFP,
        0x3fffe00,
        0x3fffe00,
        0,
        REG_32B,
        0
        },
        {"VID1_FCHP",                  /* Forward Chroma Frame Buffer */
        VID1,
        VID_FCHP,
        0x3fffe00,
        0x3fffe00,
        0,
        REG_32B,
        0
        },
        {"VID1_BFP",                   /* Backward Frame Buffer (256 bytes unit) */
        VID1,
        VID_BFP,
        0x3fffe00,
        0x3fffe00,
        0,
        REG_32B,
        0
        },
        {"VID1_BCHP",                  /* Backward Chroma Frame Buffer */
        VID1,
        VID_BCHP,
        0x3fffe00,
        0x3fffe00,
        0,
        REG_32B,
        0
        },
        {"VID1_SRFP",                  /* PIP Reconstruction Frame Buffer (256 bytes unit) */
        VID1,
        VID_SRFP,
        0x3fffe00,
        0x3fffe00,
        0,
        REG_32B,
        0
        },
        {"VID1_SRCHP",                 /* PIP Reconstruction Chroma Frame Buffer */
        VID1,
        VID_SRCHP,
        0x3fffe00,
        0x3fffe00,
        0,
        REG_32B,
        0
        },
        {"VID1_PFP",                   /* Presentation Frame Buffer (256 bytes unit) */
        VID1,
        VID_PFP,
        0x3fffe00,
        0x3fffe00,
        0,
        REG_32B,
        0
        },
        {"VID1_PCHP",                  /* Presentation Chroma Frame Buffer */
        VID1,
        VID_PCHP,
        0x3fffe00,
        0x3fffe00,
        0,
        REG_32B,
        0
        },
        {"VID1_APFP",                  /* PIP Presentation Frame Buffer (256 bytes unit) */
        VID1,
        VID_APFP,
        0x3fffe00,
        0x3fffe00,
        0,
        REG_32B,
        0
        },
        {"VID1_APCHP",                 /* PIP Presentation Chroma Frame Buffer */
        VID1,
        VID_APCHP,
        0x3fffe00,
        0x3fffe00,
        0,
        REG_32B,
        0
        },
        {"VID1_PFLD",                  /* Presentation Field Selection */
        VID1,
        VID_PFLD,
        0xf,
        0xf,
        0,
        REG_32B,
        0
        },
        {"VID1_APFLD",                 /* PIP Presentation Field Selection */
        VID1,
        VID_APFLD,
        0xf,
        0xf,
        0,
        REG_32B,
        0
        },
        {"VID1_BBG",                   /* Bit Buffer start address */
        VID1,
        VID_BBG,
        0x3ffff00,
        0x3ffff00,
        0,
        REG_32B,
        0
        },
        {"VID1_BBS",                   /* Bit Buffer end address */
        VID1,
        VID_BBS,
        0x3ffff00,
        0x3ffff00,
        0,
        REG_32B,
        0
        },
        {"VID1_BBT",                   /* Bit Buffer threshold */
        VID1,
        VID_BBT,
        0x3ffff00,
        0x3ffff00,
        0,
        REG_32B,
        0
        },
        {"VID1_BBL",                   /* Bit Buffer level */
        VID1,
        VID_BBL,
        0x3ffff00,
        0x0,
        0,
        REG_32B,
        0
        },
        {"VID1_PSCPTR",                /* PSC Byte address in memory */
        VID1,
        VID_PSCPTR,
        0x3ffffff,
        0x0,
        0,
        REG_32B,
        0
        },
        {"VID1_VLDPTR",                /* VLD start address for multi-decode & trick mode */
        VID1,
        VID_VLDPTR,
        0x3ffffff,
        0x3ffffff,
        0,
        REG_32B,
        0
        },

        {"VID1_DFW",                   /* Decoded Frame Width (in MB) */
        VID1,
        VID_DFW,
        0xff,
        0xff,
        0,
        REG_32B,
        0
        },
        {"VID1_DFH",                   /* Decoded Frame Heigt (in MB) */
        VID1,
        VID_DFH,
        0x7f,
        0x7f,
        0,
        REG_32B,
        0
        },
        {"VID1_DFS",                   /* Decoded Frame Size (in MB) */
        VID1,
        VID_DFS,
        0x1fff,
        0x1fff,
        0,
        REG_32B,
        0
        },
        {"VID1_PFW",                   /* Presentation Frame Width (in MB) */
        VID1,
        VID_PFW,
        0xff,
        0xff,
        0,
        REG_32B,
        0
        },
        {"VID1_PFH",                   /* Presentation Frame Size (in MB) */
        VID1,
        VID_PFH,
        0x7f,
        0x7f,
        0,
        REG_32B,
        0
        },

        {"VID1_STP",                   /* Video bitstream management setup */
        VID1,
        VID_STP,
        0x03,
        0x03,
        0,
        REG_32B,
        0
        },
        {"VID1_CDI",                   /* CD Input from host interface */
        VID1,
        VID_CDI,
        0x0,
        0xffffffff,
        0,
    /*   REG_32B | REG_EXCL_FROM_TEST, */
    REG_32B,
        0
        },
        {"VID1_DMOD",                  /* Decoder Memory Mode: compress, H/n, V/n, ovw */
        VID1,
        VID_DMOD,
        0x7ff,
        0x7ff,
        0,
        REG_32B,
        0
        },
        {"VID1_PMOD",                  /* Presentation Memory Mode */
        VID1,
        VID_PMOD,
        0x7ff,
        0x7ff,
        0,
        REG_32B,
        0
        },
        {"VID1_SDMOD",                 /* Secondary reconstruction Memory Mode: compress, H/n, V/n, ovw */
        VID1,
        VID_SDMOD,
        0x37c,
        0x37c,
        0,
        REG_32B,
        0
        },
        {"VID1_APMOD",                 /* PIP Presentation Memory Mode */
        VID1,
        VID_APMOD,
        0x7fc,
        0x7fc,
        0,
        REG_32B,
        0
        },
        {"VID1_SRS",                   /* Soft Reset */
        VID1,
        VID_SRS,
        0x0,
        0x1,
        0,
        REG_32B|REG_SPECIAL,               /* Reset on Write */
        0
        },
        {"VID1_PRS",                   /* Pipe Reset */
        VID1,
        VID_PRS,
        0x0,
        0x1,
        0,
        REG_32B|REG_SPECIAL,               /* Reset on Write */
        0
        },

        {"VID1_ITM",                   /* Interrupt mask */
        VID1,
        VID_ITM,
        0x7fff,
        0x7fff,
        0,
        REG_32B,
        0
        },
        {"VID1_ITS",                   /* Interrupt status */
        VID1,
        VID_ITS,
        0x7fff,
        0x0,
        0,
        REG_32B | REG_SPECIAL | REG_SIDE_READ,
        0
        },
        {"VID1_STA",                   /* Status register */
        VID1,
        VID_STA,
        0x7fff,
        0x0,
        0,
        REG_32B,
        0
        },

        {"VID1_CDWL",                  /* Top address limit of CD write in Trick mode */
        VID1,
        VID_CDWL,
        0x3ffff80,
        0x3ffff80,
        0,
        REG_32B,
        0
        },
        {"VID1_SCDPTR",                /* SCD addr pointer for launch in Trick mode */
        VID1,
        VID_SCDPTR,
        0x3ffffff,
        0x3ffffff,
        0,
        REG_32B,
        0
        },

        {"VID1_CDC",                   /* CD count register */
        VID1,
        VID_CDC,
        0xffffff,
        0x0,
        0,
        REG_32B,
        0
        },
        {"VID1_SCDC",                  /* SCD count register */
        VID1,
        VID_SCDC,
        0xffffff,
        0x0,
        0,
        REG_32B,
        0
        },

        {"VID1_PES_CFG",               /* PES parser Configuration */
        VID1,
        VID_PES_CFG,
        0x7f,
        0x7f,
        0,
        REG_32B,
        0
        },
        {"VID1_PES_SI",                /* PES Stream Id */
        VID1,
        VID_PES_SI,
        0xff,
        0xff,
        0,
        REG_32B,
        0
        },
        {"VID1_PES_FSI",               /* Stream ID Filter */
        VID1,
        VID_PES_FSI,
        0xff,
        0xff,
        0,
        REG_32B,
        0
        },
        {"VID1_PES_TFI",               /* Trick Filtering ID (Fast Forward ID) */
        VID1,
        VID_PES_TFI,
        0xff,
        0xff,
        0,
        REG_32B,
        0
        },
        {"VID1_PES_DSM",               /* DSM Value */
        VID1,
        VID_PES_DSM,
        0xff,
        0x0,
        0,
        REG_32B,
        0
        },
        {"VID1_PES_TS0",               /* Time-Stamp Value LSW */
        VID1,
        VID_PES_TS0,
        0xffffffff,
        0x0,
        0,
        REG_32B,
        0
        },
        {"VID1_PES_TS1",               /* Time-Stamp Value MSW */
        VID1,
        VID_PES_TS1,
        0x1,
        0x0,
        0,
        REG_32B,
        0
        },
        {"VID1_PES_ASS",               /* DSM and TS association flags */
        VID1,
        VID_PES_ASS,
        0x03,
        0x0,
        0,
        REG_32B,
        0
        },

        {"VID1_HDF",                   /* Header Data FIFO */
        VID1,
        VID_HDF,
        0x0001ffff,
        0x0,
        0,
        REG_32B | REG_SPECIAL | REG_SIDE_READ,
        0
        },
        {"VID1_HLNC",                  /* Launch start code detector */
        VID1,
        VID_HLNC,
        0x0,
        0x3,
        0,
        REG_32B|REG_SPECIAL,
        0
        },

        {"VID1_QMWI",                  /* Quantization matrix data, intra table */
        VID1,
        VID_QMWI,
        0x0,
        0xffffffff,
        0,
        REG_32B|REG_ARRAY,
        16
        },
        {"VID1_QMWNI",                 /* Quantization matrix data, non intra table */
        VID1,
        VID_QMWNI,
        0x0,
        0xffffffff,
        0,
        REG_32B|REG_ARRAY,
        16
        },
#if defined(VIDEO_7020)
        {"VID1_CDR",                        /* CD from register */
        VID1,
        VID_CDR,
        0x1,
        0x1,
        0,
        REG_32B,
        0
        },
        {"VID1_CDWPTR",                     /* Compressed data write pointer */
        VID1,
        VID_CDWPTR,
        0x3ffff80,
        0x3ffff80,
        0,
        REG_32B,
        0
        },
        {"VID1_VLDRPTR",                     /* VLD Read pointer */
        VID1,
        VID_VLDRPTR,
        0x3ffff00,
        0x3ffff00,
        0,
        REG_32B,
        0
        },
        {"VID1_SCDRPTR",                     /* SCD read pointer */
        VID1,
        VID_SCDRPTR,
        0x3ffff80,
        0x3ffff80,
        0,
        REG_32B,
        0
        },
        {"VID1_VPP_CFG",                        /* HD-PIP parser configuration */
        VID1,
        VID_VPP_CFG,
        0x3f,
        0x3f,
        0,
        REG_16B,
        0
        },
        {"VID1_VPP_DFHT",                        /* HD-PIP parser frame height threshold */
        VID1,
        VID_VPP_DFHT,
        0x3fff,
        0x3fff,
        0,
        REG_16B,
        0
        },
        {"VID1_VPP_DFWT",                        /* HD-PIP parser frame width threshold */
        VID1,
        VID_VPP_DFWT,
        0x3fff,
        0x3fff,
        0,
        REG_16B,
        0
        },
#endif

    /* --------------------- DECODER 2 ---------------------------------------- */
        {"VID2_TIS",                        /* Task Instruction */
        VID2,
        VID_TIS,
        0x3bf,
        0x3bf,
        0,
        REG_32B|REG_SPECIAL,
        0
        },
        {"VID2_PPR",                        /* Picture F Parameters and other parameters */
        VID2,
        VID_PPR,
        0x0fffffff,
        0x0fffffff,
        0,
        REG_32B,
        0
        },
        {"VID2_RFP",                        /* Reconstruction Frame Buffer (256 bytes unit) */
        VID2,
        VID_RFP,
        0x3fffe00,
        0x3fffe00,
        0,
        REG_32B,
        0
        },
        {"VID2_RCHP",                       /* Reconstruction Chroma Frame Buffer */
        VID2,
        VID_RCHP,
        0x3fffe00,
        0x3fffe00,
        0,
        REG_32B,
        0
        },
        {"VID2_FFP",                        /* Forward Frame Buffer (256 bytes unit) */
        VID2,
        VID_FFP,
        0x3fffe00,
        0x3fffe00,
        0,
        REG_32B,
        0
        },
        {"VID2_FCHP",                       /* Forward Chroma Frame Buffer */
        VID2,
        VID_FCHP,
        0x3fffe00,
        0x3fffe00,
        0,
        REG_32B,
        0
        },
        {"VID2_BFP",                        /* Backward Frame Buffer (256 bytes unit) */
        VID2,
        VID_BFP,
        0x3fffe00,
        0x3fffe00,
        0,
        REG_32B,
        0
        },
        {"VID2_BCHP",                       /* Backward Chroma Frame Buffer */
        VID2,
        VID_BCHP,
        0x3fffe00,
        0x3fffe00,
        0,
        REG_32B,
        0
        },
        {"VID2_SRFP",                       /* PIP Reconstruction Frame Buffer (256 bytes unit) */
        VID2,
        VID_SRFP,
        0x3fffe00,
        0x3fffe00,
        0,
        REG_32B,
        0
        },
        {"VID2_SRCHP",                      /* PIP Reconstruction Chroma Frame Buffer */
        VID2,
        VID_SRCHP,
        0x3fffe00,
        0x3fffe00,
        0,
        REG_32B,
        0
        },
        {"VID2_PFP",                        /* Presentation Frame Buffer (256 bytes unit) */
        VID2,
        VID_PFP,
        0x3fffe00,
        0x3fffe00,
        0,
        REG_32B,
        0
        },
        {"VID2_PCHP",                       /* Presentation Chroma Frame Buffer */
        VID2,
        VID_PCHP,
        0x3fffe00,
        0x3fffe00,
        0,
        REG_32B,
        0
        },
        {"VID2_APFP",                       /* PIP Presentation Frame Buffer (256 bytes unit) */
        VID2,
        VID_APFP,
        0x3fffe00,
        0x3fffe00,
        0,
        REG_32B,
        0
        },
        {"VID2_APCHP",                      /* PIP Presentation Chroma Frame Buffer */
        VID2,
        VID_APCHP,
        0x3fffe00,
        0x3fffe00,
        0,
        REG_32B,
        0
        },
        {"VID2_PFLD",                       /* Presentation Field Selection */
        VID2,
        VID_PFLD,
        0xf,
        0xf,
        0,
        REG_32B,
        0
        },
        {"VID2_APFLD",                      /* PIP Presentation Field Selection */
        VID2,
        VID_APFLD,
        0xf,
        0xf,
        0,
        REG_32B,
        0
        },
        {"VID2_BBG",                        /* Bit Buffer start address */
        VID2,
        VID_BBG,
        0x3ffff00,
        0x3ffff00,
        0,
        REG_32B,
        0
        },
        {"VID2_BBS",                        /* Bit Buffer end address */
        VID2,
        VID_BBS,
        0x3ffff00,
        0x3ffff00,
        0,
        REG_32B,
        0
        },
        {"VID2_BBT",                        /* Bit Buffer threshold */
        VID2,
        VID_BBT,
        0x3ffff00,
        0x3ffff00,
        0,
        REG_32B,
        0
        },
        {"VID2_BBL",                        /* Bit Buffer level */
        VID2,
        VID_BBL,
        0x3ffff00,
        0x0,
        0,
        REG_32B,
        0
        },
        {"VID2_PSCPTR",                     /* PSC Byte address in memory */
        VID2,
        VID_PSCPTR,
        0x3ffffff,
        0x0,
        0,
        REG_32B,
        0
        },
        {"VID2_VLDPTR",                     /* VLD start address for multi-decode & trick mode */
        VID2,
        VID_VLDPTR,
        0x3ffffff,
        0x3ffffff,
        0,
        REG_32B,
        0
        },

        {"VID2_DFW",                        /* Decoded Frame Width (in MB) */
        VID2,
        VID_DFW,
        0xff,
        0xff,
        0,
        REG_32B,
        0
        },
        {"VID2_DFH",                        /* Decoded Frame Heigt (in MB) */
        VID2,
        VID_DFH,
        0x7f,
        0x7f,
        0,
        REG_32B,
        0
        },
        {"VID2_DFS",                        /* Decoded Frame Size (in MB) */
        VID2,
        VID_DFS,
        0x1fff,
        0x1fff,
        0,
        REG_32B,
        0
        },
        {"VID2_PFW",                        /* Presentation Frame Width (in MB) */
        VID2,
        VID_PFW,
        0xff,
        0xff,
        0,
        REG_32B,
        0
        },
        {"VID2_PFH",                        /* Presentation Frame Size (in MB) */
        VID2,
        VID_PFH,
        0x7f,
        0x7f,
        0,
        REG_32B,
        0
        },

        {"VID2_STP",                        /* Video bitstream management setup */
        VID2,
        VID_STP,
        0x03,
        0x03,
        0,
        REG_32B,
        0
        },
        {"VID2_CDI",                        /* CD Input from host interface */
        VID2,
        VID_CDI,
        0x0,
        0xffffffff,
        0,
    /*   REG_32B | REG_EXCL_FROM_TEST, */
    REG_32B,
        0
        },
        {"VID2_DMOD",                       /* Decoder Memory Mode: compress, H/n, V/n, ovw */
        VID2,
        VID_DMOD,
        0x7ff,
        0x7ff,
        0,
        REG_32B,
        0
        },
        {"VID2_PMOD",                       /* Presentation Memory Mode */
        VID2,
        VID_PMOD,
        0x7ff,
        0x7ff,
        0,
        REG_32B,
        0
        },
        {"VID2_SDMOD",                      /* Secondary reconstruction Memory Mode: compress, H/n, V/n, ovw */
        VID2,
        VID_SDMOD,
        0x37c,
        0x37c,
        0,
        REG_32B,
        0
        },
        {"VID2_APMOD",                      /* PIP Presentation Memory Mode */
        VID2,
        VID_APMOD,
        0x7fc,
        0x7fc,
        0,
        REG_32B,
        0
        },
        {"VID2_SRS",                        /* Soft Reset */
        VID2,
        VID_SRS,
        0x0,
        0x1,
        0,
        REG_32B|REG_SPECIAL,               /* Reset on Write */
        0
        },
        {"VID2_PRS",                        /* Pipe Reset */
        VID2,
        VID_PRS,
        0x0,
        0x1,
        0,
        REG_32B|REG_SPECIAL,               /* Reset on Write */
        0
        },

        {"VID2_ITM",                        /* Interrupt mask */
        VID2,
        VID_ITM,
        0x3fff,
        0x3fff,
        0,
        REG_32B,
        0
        },
        {"VID2_ITS",                        /* Interrupt status */
        VID2,
        VID_ITS,
        0x3fff,
        0x0,
        0,
        REG_32B | REG_SPECIAL | REG_SIDE_READ,
        0
        },
        {"VID2_STA",                        /* Status register */
        VID2,
        VID_STA,
        0x3fff,
        0x0,
        0,
        REG_32B,
        0
        },

        {"VID2_CDWL",                       /* Top address limit of CD write in Trick mode */
        VID1,
        VID_CDWL,
        0x3ffff80,
        0x3ffff80,
        0,
        REG_32B,
        0
        },
        {"VID2_SCDPTR",                     /* SCD addr pointer for launch in Trick mode */
        VID1,
        VID_SCDPTR,
        0x3ffffff,
        0x3ffffff,
        0,
        REG_32B,
        0
        },

        {"VID2_CDC",                        /* CD count register */
        VID2,
        VID_CDC,
        0xffffff,
        0x0,
        0,
        REG_32B,
        0
        },
        {"VID2_SCDC",                       /* SCD count register */
        VID2,
        VID_SCDC,
        0xffffff,
        0x0,
        0,
        REG_32B,
        0
        },

        {"VID2_PES_CFG",                    /* PES parser Configuration */
        VID2,
        VID_PES_CFG,
        0x7f,
        0x7f,
        0,
        REG_32B,
        0
        },
        {"VID2_PES_SI",                     /* PES Stream Id */
        VID2,
        VID_PES_SI,
        0xff,
        0xff,
        0,
        REG_32B,
        0
        },
        {"VID2_PES_FSI",                    /* Stream ID Filter */
        VID2,
        VID_PES_FSI,
        0xff,
        0xff,
        0,
        REG_32B,
        0
        },
        {"VID2_PES_TFI",                    /* Trick Filtering ID (Fast Forward ID) */
        VID2,
        VID_PES_TFI,
        0xff,
        0xff,
        0,
        REG_32B,
        0
        },
        {"VID2_PES_DSM",                    /* DSM Value */
        VID2,
        VID_PES_DSM,
        0xff,
        0x0,
        0,
        REG_32B,
        0
        },
        {"VID2_PES_TS0",                    /* Time-Stamp Value LSW */
        VID2,
        VID_PES_TS0,
        0xffffffff,
        0x0,
        0,
        REG_32B,
        0
        },
        {"VID2_PES_TS1",                    /* Time-Stamp Value MSW */
        VID2,
        VID_PES_TS1,
        0x1,
        0x0,
        0,
        REG_32B,
        0
        },
        {"VID2_PES_ASS",                    /* DSM and TS association flags */
        VID2,
        VID_PES_ASS,
        0x03,
        0x0,
        0,
        REG_32B,
        0
        },

        {"VID2_HDF",                        /* Header Data FIFO */
        VID2,
        VID_HDF,
        0x0001ffff,
        0x0,
        0,
        REG_32B | REG_SPECIAL | REG_SIDE_READ,
        0
        },
        {"VID2_HLNC",                       /* Launch start code detector */
        VID2,
        VID_HLNC,
        0x0,
        0x3,
        0,
        REG_32B|REG_SPECIAL,
        0
        },

        {"VID2_QMWI",                       /* Quantization matrix data, intra table */
        VID2,
        VID_QMWI,
        0x0,
        0xffffffff,
        0,
        REG_32B|REG_ARRAY,
        16
        },
        {"VID2_QMWNI",                      /* Quantization matrix data, non intra table */
        VID2,
        VID_QMWNI,
        0x0,
        0xffffffff,
        0,
        REG_32B|REG_ARRAY,
        16
        },
#if defined(VIDEO_7020)
        {"VID2_CDR",                        /* CD from register */
        VID2,
        VID_CDR,
        0x1,
        0x1,
        0,
        REG_32B,
        0
        },
        {"VID2_CDWPTR",                     /* Compressed data write pointer */
        VID2,
        VID_CDWPTR,
        0x3ffff80,
        0x3ffff80,
        0,
        REG_32B,
        0
        },
        {"VID2_VLDRPTR",                     /* VLD Read pointer */
        VID2,
        VID_VLDRPTR,
        0x3ffff00,
        0x3ffff00,
        0,
        REG_32B,
        0
        },
        {"VID2_SCDRPTR",                     /* SCD read pointer */
        VID2,
        VID_SCDRPTR,
        0x3ffff80,
        0x3ffff80,
        0,
        REG_32B,
        0
        },
        {"VID2_VPP_CFG",                        /* HD-PIP parser configuration */
        VID2,
        VID_VPP_CFG,
        0x3f,
        0x3f,
        0,
        REG_16B,
        0
        },
        {"VID2_VPP_DFHT",                        /* HD-PIP parser frame height threshold */
        VID1,
        VID_VPP_DFHT,
        0x3fff,
        0x3fff,
        0,
        REG_16B,
        0
        },
        {"VID2_VPP_DFWT",                        /* HD-PIP parser frame width threshold */
        VID1,
        VID_VPP_DFWT,
        0x3fff,
        0x3fff,
        0,
        REG_16B,
        0
        },
#endif

    /* --------------------- DECODER 3 ---------------------------------------- */
        {"VID3_TIS",                        /* Task Instruction */
        VID3,
        VID_TIS,
        0x3bf,
        0x3bf,
        0,
        REG_32B|REG_SPECIAL,
        0
        },
        {"VID3_PPR",                        /* Picture F Parameters and other parameters */
        VID3,
        VID_PPR,
        0x0fffffff,
        0x0fffffff,
        0,
        REG_32B,
        0
        },
        {"VID3_RFP",                        /* Reconstruction Frame Buffer (256 bytes unit) */
        VID3,
        VID_RFP,
        0x3fffe00,
        0x3fffe00,
        0,
        REG_32B,
        0
        },
        {"VID3_RCHP",                       /* Reconstruction Chroma Frame Buffer */
        VID3,
        VID_RCHP,
        0x3fffe00,
        0x3fffe00,
        0,
        REG_32B,
        0
        },
        {"VID3_FFP",                        /* Forward Frame Buffer (256 bytes unit) */
        VID3,
        VID_FFP,
        0x3fffe00,
        0x3fffe00,
        0,
        REG_32B,
        0
        },
        {"VID3_FCHP",                       /* Forward Chroma Frame Buffer */
        VID3,
        VID_FCHP,
        0x3fffe00,
        0x3fffe00,
        0,
        REG_32B,
        0
        },
        {"VID3_BFP",                        /* Backward Frame Buffer (256 bytes unit) */
        VID3,
        VID_BFP,
        0x3fffe00,
        0x3fffe00,
        0,
        REG_32B,
        0
        },
        {"VID3_BCHP",                       /* Backward Chroma Frame Buffer */
        VID3,
        VID_BCHP,
        0x3fffe00,
        0x3fffe00,
        0,
        REG_32B,
        0
        },
        {"VID3_SRFP",                       /* PIP Reconstruction Frame Buffer (256 bytes unit) */
        VID3,
        VID_SRFP,
        0x3fffe00,
        0x3fffe00,
        0,
        REG_32B,
        0
        },
        {"VID3_SRCHP",                      /* PIP Reconstruction Chroma Frame Buffer */
        VID3,
        VID_SRCHP,
        0x3fffe00,
        0x3fffe00,
        0,
        REG_32B,
        0
        },
        {"VID3_PFP",                        /* Presentation Frame Buffer (256 bytes unit) */
        VID3,
        VID_PFP,
        0x3fffe00,
        0x3fffe00,
        0,
        REG_32B,
        0
        },
        {"VID3_PCHP",                       /* Presentation Chroma Frame Buffer */
        VID3,
        VID_PCHP,
        0x3fffe00,
        0x3fffe00,
        0,
        REG_32B,
        0
        },
        {"VID3_APFP",                       /* PIP Presentation Frame Buffer (256 bytes unit) */
        VID3,
        VID_APFP,
        0x3fffe00,
        0x3fffe00,
        0,
        REG_32B,
        0
        },
        {"VID3_APCHP",                      /* PIP Presentation Chroma Frame Buffer */
        VID3,
        VID_APCHP,
        0x3fffe00,
        0x3fffe00,
        0,
        REG_32B,
        0
        },
        {"VID3_PFLD",                       /* Presentation Field Selection */
        VID3,
        VID_PFLD,
        0xf,
        0xf,
        0,
        REG_32B,
        0
        },
        {"VID3_APFLD",                      /* PIP Presentation Field Selection */
        VID3,
        VID_APFLD,
        0xf,
        0xf,
        0,
        REG_32B,
        0
        },
        {"VID3_BBG",                        /* Bit Buffer start address */
        VID3,
        VID_BBG,
        0x3ffff00,
        0x3ffff00,
        0,
        REG_32B,
        0
        },
        {"VID3_BBS",                        /* Bit Buffer end address */
        VID3,
        VID_BBS,
        0x3ffff00,
        0x3ffff00,
        0,
        REG_32B,
        0
        },
        {"VID3_BBT",                        /* Bit Buffer threshold */
        VID3,
        VID_BBT,
        0x3ffff00,
        0x3ffff00,
        0,
        REG_32B,
        0
        },
        {"VID3_BBL",                        /* Bit Buffer level */
        VID3,
        VID_BBL,
        0x3ffff00,
        0x0,
        0,
        REG_32B,
        0
        },
        {"VID3_PSCPTR",                     /* PSC Byte address in memory */
        VID3,
        VID_PSCPTR,
        0x3ffffff,
        0x0,
        0,
        REG_32B,
        0
        },
        {"VID3_VLDPTR",                     /* VLD start address for multi-decode & trick mode */
        VID3,
        VID_VLDPTR,
        0x3ffffff,
        0x3ffffff,
        0,
        REG_32B,
        0
        },

        {"VID3_DFW",                        /* Decoded Frame Width (in MB) */
        VID3,
        VID_DFW,
        0xff,
        0xff,
        0,
        REG_32B,
        0
        },
        {"VID3_DFH",                        /* Decoded Frame Heigt (in MB) */
        VID3,
        VID_DFH,
        0x7f,
        0x7f,
        0,
        REG_32B,
        0
        },
        {"VID3_DFS",                        /* Decoded Frame Size (in MB) */
        VID3,
        VID_DFS,
        0x1fff,
        0x1fff,
        0,
        REG_32B,
        0
        },
        {"VID3_PFW",                        /* Presentation Frame Width (in MB) */
        VID3,
        VID_PFW,
        0xff,
        0xff,
        0,
        REG_32B,
        0
        },
        {"VID3_PFH",                        /* Presentation Frame Size (in MB) */
        VID3,
        VID_PFH,
        0x7f,
        0x7f,
        0,
        REG_32B,
        0
        },

        {"VID3_STP",                        /* Video bitstream management setup */
        VID3,
        VID_STP,
        0x03,
        0x03,
        0,
        REG_32B,
        0
        },
        {"VID3_CDI",                        /* CD Input from host interface */
        VID3,
        VID_CDI,
        0x0,
        0xffffffff,
        0,
    /*   REG_32B | REG_EXCL_FROM_TEST, */
    REG_32B,
        0
        },
        {"VID3_DMOD",                       /* Decoder Memory Mode: compress, H/n, V/n, ovw */
        VID3,
        VID_DMOD,
        0x1ff,
        0x1ff,
        0,
        REG_32B,
        0
        },
        {"VID3_PMOD",                       /* Presentation Memory Mode */
        VID3,
        VID_PMOD,
        0x1ff,
        0x1ff,
        0,
        REG_32B,
        0
        },
        {"VID3_SDMOD",                      /* Secondary reconstruction Memory Mode: compress, H/n, V/n, ovw */
        VID3,
        VID_SDMOD,
        0x37c,
        0x37c,
        0,
        REG_32B,
        0
        },
        {"VID3_APMOD",                      /* PIP Presentation Memory Mode */
        VID3,
        VID_APMOD,
        0x1fc,
        0x1fc,
        0,
        REG_32B,
        0
        },
        {"VID3_SRS",                        /* Soft Reset */
        VID3,
        VID_SRS,
        0x0,
        0x1,
        0,
        REG_32B|REG_SPECIAL,               /* Reset on Write */
        0
        },
        {"VID3_PRS",                        /* Pipe Reset */
        VID3,
        VID_PRS,
        0x0,
        0x1,
        0,
        REG_32B|REG_SPECIAL,               /* Reset on Write */
        0
        },

        {"VID3_ITM",                        /* Interrupt mask */
        VID3,
        VID_ITM,
        0x3fff,
        0x3fff,
        0,
        REG_32B,
        0
        },
        {"VID3_ITS",                        /* Interrupt status */
        VID3,
        VID_ITS,
        0x3fff,
        0x0,
        0,
        REG_32B | REG_SPECIAL | REG_SIDE_READ,
        0
        },
        {"VID3_STA",                        /* Status register */
        VID3,
        VID_STA,
        0x3fff,
        0x0,
        0,
        REG_32B,
        0
        },

        {"VID3_CDWL",                       /* Top address limit of CD write in Trick mode */
        VID1,
        VID_CDWL,
        0x3ffff80,
        0x3ffff80,
        0,
        REG_32B,
        0
        },
        {"VID3_SCDPTR",                     /* SCD addr pointer for launch in Trick mode */
        VID1,
        VID_SCDPTR,
        0x3ffffff,
        0x3ffffff,
        0,
        REG_32B,
        0
        },

        {"VID3_CDC",                        /* CD count register */
        VID3,
        VID_CDC,
        0xffffff,
        0x0,
        0,
        REG_32B,
        0
        },
        {"VID3_SCDC",                       /* SCD count register */
        VID3,
        VID_SCDC,
        0xffffff,
        0x0,
        0,
        REG_32B,
        0
        },

        {"VID3_PES_CFG",                    /* PES parser Configuration */
        VID3,
        VID_PES_CFG,
        0x7f,
        0x7f,
        0,
        REG_32B,
        0
        },
        {"VID3_PES_SI",                     /* PES Stream Id */
        VID3,
        VID_PES_SI,
        0xff,
        0xff,
        0,
        REG_32B,
        0
        },
        {"VID3_PES_FSI",                    /* Stream ID Filter */
        VID3,
        VID_PES_FSI,
        0xff,
        0xff,
        0,
        REG_32B,
        0
        },
        {"VID3_PES_TFI",                    /* Trick Filtering ID (Fast Forward ID) */
        VID3,
        VID_PES_TFI,
        0xff,
        0xff,
        0,
        REG_32B,
        0
        },
        {"VID3_PES_DSM",                    /* DSM Value */
        VID3,
        VID_PES_DSM,
        0xff,
        0x0,
        0,
        REG_32B,
        0
        },
        {"VID3_PES_TS0",                    /* Time-Stamp Value LSW */
        VID3,
        VID_PES_TS0,
        0xffffffff,
        0x0,
        0,
        REG_32B,
        0
        },
        {"VID3_PES_TS1",                    /* Time-Stamp Value MSW */
        VID3,
        VID_PES_TS1,
        0x1,
        0x0,
        0,
        REG_32B,
        0
        },
        {"VID3_PES_ASS",                    /* DSM and TS association flags */
        VID3,
        VID_PES_ASS,
        0x03,
        0x0,
        0,
        REG_32B,
        0
        },

        {"VID3_HDF",                        /* Header Data FIFO */
        VID3,
        VID_HDF,
        0x0001ffff,
        0x0,
        0,
        REG_32B | REG_SPECIAL | REG_SIDE_READ,
        0
        },
        {"VID3_HLNC",                       /* Launch start code detector */
        VID3,
        VID_HLNC,
        0x0,
        0x3,
        0,
        REG_32B|REG_SPECIAL,
        0
        },

        {"VID3_QMWI",                       /* Quantization matrix data, intra table */
        VID3,
        VID_QMWI,
        0x0,
        0xffffffff,
        0,
        REG_32B|REG_ARRAY,
        16
        },
        {"VID3_QMWNI",                      /* Quantization matrix data, non intra table */
        VID3,
        VID_QMWNI,
        0x0,
        0xffffffff,
        0,
        REG_32B|REG_ARRAY,
        16
        },
#if defined(VIDEO_7020)
        {"VID3_CDR",                        /* CD from register */
        VID3,
        VID_CDR,
        0x1,
        0x1,
        0,
        REG_32B,
        0
        },
        {"VID3_CDWPTR",                     /* Compressed data write pointer */
        VID3,
        VID_CDWPTR,
        0x3ffff80,
        0x3ffff80,
        0,
        REG_32B,
        0
        },
        {"VID3_VLDRPTR",                     /* VLD Read pointer */
        VID3,
        VID_VLDRPTR,
        0x3ffff00,
        0x3ffff00,
        0,
        REG_32B,
        0
        },
        {"VID3_SCDRPTR",                     /* SCD read pointer */
        VID3,
        VID_SCDRPTR,
        0x3ffff80,
        0x3ffff80,
        0,
        REG_32B,
        0
        },
        {"VID3_VPP_CFG",                        /* HD-PIP parser configuration */
        VID3,
        VID_VPP_CFG,
        0x1f,
        0x1f,
        0,
        REG_16B,
        0
        },
        {"VID3_VPP_DFHT",                        /* HD-PIP parser frame height threshold */
        VID3,
        VID_VPP_DFHT,
        0x3fff,
        0x3fff,
        0,
        REG_16B,
        0
        },
        {"VID3_VPP_DFWT",                        /* HD-PIP parser frame width threshold */
        VID3,
        VID_VPP_DFWT,
        0x3fff,
        0x3fff,
        0,
        REG_16B,
        0
        },
#endif

    /* --------------------- DECODER 4 ---------------------------------------- */
        {"VID4_TIS",                        /* Task Instruction */
        VID4,
        VID_TIS,
        0x3bf,
        0x3bf,
        0,
        REG_32B|REG_SPECIAL,
        0
        },
        {"VID4_PPR",                        /* Picture F Parameters and other parameters */
        VID4,
        VID_PPR,
        0x0fffffff,
        0x0fffffff,
        0,
        REG_32B,
        0
        },
        {"VID4_RFP",                        /* Reconstruction Frame Buffer (256 bytes unit) */
        VID4,
        VID_RFP,
        0x3fffe00,
        0x3fffe00,
        0,
        REG_32B,
        0
        },
        {"VID4_RCHP",                       /* Reconstruction Chroma Frame Buffer */
        VID4,
        VID_RCHP,
        0x3fffe00,
        0x3fffe00,
        0,
        REG_32B,
        0
        },
        {"VID4_FFP",                        /* Forward Frame Buffer (256 bytes unit) */
        VID4,
        VID_FFP,
        0x3fffe00,
        0x3fffe00,
        0,
        REG_32B,
        0
        },
        {"VID4_FCHP",                       /* Forward Chroma Frame Buffer */
        VID4,
        VID_FCHP,
        0x3fffe00,
        0x3fffe00,
        0,
        REG_32B,
        0
        },
        {"VID4_BFP",                        /* Backward Frame Buffer (256 bytes unit) */
        VID4,
        VID_BFP,
        0x3fffe00,
        0x3fffe00,
        0,
        REG_32B,
        0
        },
        {"VID4_BCHP",                       /* Backward Chroma Frame Buffer */
        VID4,
        VID_BCHP,
        0x3fffe00,
        0x3fffe00,
        0,
        REG_32B,
        0
        },
        {"VID4_SRFP",                       /* PIP Reconstruction Frame Buffer (256 bytes unit) */
        VID4,
        VID_SRFP,
        0x3fffe00,
        0x3fffe00,
        0,
        REG_32B,
        0
        },
        {"VID4_SRCHP",                      /* PIP Reconstruction Chroma Frame Buffer */
        VID4,
        VID_SRCHP,
        0x3fffe00,
        0x3fffe00,
        0,
        REG_32B,
        0
        },
        {"VID4_PFP",                        /* Presentation Frame Buffer (256 bytes unit) */
        VID4,
        VID_PFP,
        0x3fffe00,
        0x3fffe00,
        0,
        REG_32B,
        0
        },
        {"VID4_PCHP",                       /* Presentation Chroma Frame Buffer */
        VID4,
        VID_PCHP,
        0x3fffe00,
        0x3fffe00,
        0,
        REG_32B,
        0
        },
        {"VID4_APFP",                       /* PIP Presentation Frame Buffer (256 bytes unit) */
        VID4,
        VID_APFP,
        0x3fffe00,
        0x3fffe00,
        0,
        REG_32B,
        0
        },
        {"VID4_APCHP",                      /* PIP Presentation Chroma Frame Buffer */
        VID4,
        VID_APCHP,
        0x3fffe00,
        0x3fffe00,
        0,
        REG_32B,
        0
        },
        {"VID4_PFLD",                       /* Presentation Field Selection */
        VID4,
        VID_PFLD,
        0xf,
        0xf,
        0,
        REG_32B,
        0
        },
        {"VID4_APFLD",                      /* PIP Presentation Field Selection */
        VID4,
        VID_APFLD,
        0xf,
        0xf,
        0,
        REG_32B,
        0
        },
        {"VID4_BBG",                        /* Bit Buffer start address */
        VID4,
        VID_BBG,
        0x3ffff00,
        0x3ffff00,
        0,
        REG_32B,
        0
        },
        {"VID4_BBS",                        /* Bit Buffer end address */
        VID4,
        VID_BBS,
        0x3ffff00,
        0x3ffff00,
        0,
        REG_32B,
        0
        },
        {"VID4_BBT",                        /* Bit Buffer threshold */
        VID4,
        VID_BBT,
        0x3ffff00,
        0x3ffff00,
        0,
        REG_32B,
        0
        },
        {"VID4_BBL",                        /* Bit Buffer level */
        VID4,
        VID_BBL,
        0x3ffff00,
        0x0,
        0,
        REG_32B,
        0
        },
        {"VID4_PSCPTR",                     /* PSC Byte address in memory */
        VID4,
        VID_PSCPTR,
        0x3ffffff,
        0x0,
        0,
        REG_32B,
        0
        },
        {"VID4_VLDPTR",                     /* VLD start address for multi-decode & trick mode */
        VID4,
        VID_VLDPTR,
        0x3ffffff,
        0x3ffffff,
        0,
        REG_32B,
        0
        },

        {"VID4_DFW",                        /* Decoded Frame Width (in MB) */
        VID4,
        VID_DFW,
        0xff,
        0xff,
        0,
        REG_32B,
        0
        },
        {"VID4_DFH",                        /* Decoded Frame Heigt (in MB) */
        VID4,
        VID_DFH,
        0x7f,
        0x7f,
        0,
        REG_32B,
        0
        },
        {"VID4_DFS",                        /* Decoded Frame Size (in MB) */
        VID4,
        VID_DFS,
        0x1fff,
        0x1fff,
        0,
        REG_32B,
        0
        },
        {"VID4_PFW",                        /* Presentation Frame Width (in MB) */
        VID4,
        VID_PFW,
        0xff,
        0xff,
        0,
        REG_32B,
        0
        },
        {"VID4_PFH",                        /* Presentation Frame Size (in MB) */
        VID4,
        VID_PFH,
        0x7f,
        0x7f,
        0,
        REG_32B,
        0
        },

        {"VID4_STP",                        /* Video bitstream management setup */
        VID4,
        VID_STP,
        0x03,
        0x03,
        0,
        REG_32B,
        0
        },
        {"VID4_CDI",                        /* CD Input from host interface */
        VID4,
        VID_CDI,
        0x0,
        0xffffffff,
        0,
    /*   REG_32B | REG_EXCL_FROM_TEST, */
    REG_32B,
        0
        },
        {"VID4_DMOD",                       /* Decoder Memory Mode: compress, H/n, V/n, ovw */
        VID4,
        VID_DMOD,
        0x1ff,
        0x1ff,
        0,
        REG_32B,
        0
        },
        {"VID4_PMOD",                       /* Presentation Memory Mode */
        VID4,
        VID_PMOD,
        0x1ff,
        0x1ff,
        0,
        REG_32B,
        0
        },
        {"VID4_SDMOD",                      /* Secondary reconstruction Memory Mode: compress, H/n, V/n, ovw */
        VID4,
        VID_SDMOD,
        0x37c,
        0x37c,
        0,
        REG_32B,
        0
        },
        {"VID4_APMOD",                      /* PIP Presentation Memory Mode */
        VID4,
        VID_APMOD,
        0x1fc,
        0x1fc,
        0,
        REG_32B,
        0
        },
        {"VID4_SRS",                        /* Soft Reset */
        VID4,
        VID_SRS,
        0x0,
        0x1,
        0,
        REG_32B|REG_SPECIAL,               /* Reset on Write */
        0
        },
        {"VID4_PRS",                        /* Pipe Reset */
        VID4,
        VID_PRS,
        0x0,
        0x1,
        0,
        REG_32B|REG_SPECIAL,               /* Reset on Write */
        0
        },

        {"VID4_ITM",                        /* Interrupt mask */
        VID4,
        VID_ITM,
        0x3fff,
        0x3fff,
        0,
        REG_32B,
        0
        },
        {"VID4_ITS",                        /* Interrupt status */
        VID4,
        VID_ITS,
        0x3fff,
        0x0,
        0,
        REG_32B | REG_SPECIAL | REG_SIDE_READ,
        0
        },
        {"VID4_STA",                        /* Status register */
        VID4,
        VID_STA,
        0x3fff,
        0x0,
        0,
        REG_32B,
        0
        },

        {"VID4_CDWL",                       /* Top address limit of CD write in Trick mode */
        VID1,
        VID_CDWL,
        0x3ffff80,
        0x3ffff80,
        0,
        REG_32B,
        0
        },
        {"VID4_SCDPTR",                     /* SCD addr pointer for launch in Trick mode */
        VID1,
        VID_SCDPTR,
        0x3ffffff,
        0x3ffffff,
        0,
        REG_32B,
        0
        },

        {"VID4_CDC",                        /* CD count register */
        VID4,
        VID_CDC,
        0xffffff,
        0x0,
        0,
        REG_32B,
        0
        },
        {"VID4_SCDC",                       /* SCD count register */
        VID4,
        VID_SCDC,
        0xffffff,
        0x0,
        0,
        REG_32B,
        0
        },

        {"VID4_PES_CFG",                    /* PES parser Configuration */
        VID4,
        VID_PES_CFG,
        0x7f,
        0x7f,
        0,
        REG_32B,
        0
        },
        {"VID4_PES_SI",                     /* PES Stream Id */
        VID4,
        VID_PES_SI,
        0xff,
        0xff,
        0,
        REG_32B,
        0
        },
        {"VID4_PES_FSI",                    /* Stream ID Filter */
        VID4,
        VID_PES_FSI,
        0xff,
        0xff,
        0,
        REG_32B,
        0
        },
        {"VID4_PES_TFI",                    /* Trick Filtering ID (Fast Forward ID) */
        VID4,
        VID_PES_TFI,
        0xff,
        0xff,
        0,
        REG_32B,
        0
        },
        {"VID4_PES_DSM",                    /* DSM Value */
        VID4,
        VID_PES_DSM,
        0xff,
        0x0,
        0,
        REG_32B,
        0
        },
        {"VID4_PES_TS0",                    /* Time-Stamp Value LSW */
        VID4,
        VID_PES_TS0,
        0xffffffff,
        0x0,
        0,
        REG_32B,
        0
        },
        {"VID4_PES_TS1",                    /* Time-Stamp Value MSW */
        VID4,
        VID_PES_TS1,
        0x1,
        0x0,
        0,
        REG_32B,
        0
        },
        {"VID4_PES_ASS",                    /* DSM and TS association flags */
        VID4,
        VID_PES_ASS,
        0x03,
        0x0,
        0,
        REG_32B,
        0
        },

        {"VID4_HDF",                        /* Header Data FIFO */
        VID4,
        VID_HDF,
        0x0001ffff,
        0x0,
        0,
        REG_32B | REG_SPECIAL | REG_SIDE_READ,
        0
        },
        {"VID4_HLNC",                       /* Launch start code detector */
        VID4,
        VID_HLNC,
        0x0,
        0x3,
        0,
        REG_32B|REG_SPECIAL,
        0
        },

        {"VID4_QMWI",                       /* Quantization matrix data, intra table */
        VID4,
        VID_QMWI,
        0x0,
        0xffffffff,
        0,
        REG_32B|REG_ARRAY,
        16
        },
        {"VID4_QMWNI",                      /* Quantization matrix data, non intra table */
        VID4,
        VID_QMWNI,
        0x0,
        0xffffffff,
        0,
        REG_32B|REG_ARRAY,
        16
        },
#if defined(VIDEO_7020)
        {"VID4_CDR",                        /* CD from register */
        VID4,
        VID_CDR,
        0x1,
        0x1,
        0,
        REG_32B,
        0
        },
        {"VID4_CDWPTR",                     /* Compressed data write pointer */
        VID4,
        VID_CDWPTR,
        0x3ffff80,
        0x3ffff80,
        0,
        REG_32B,
        0
        },
        {"VID4_VLDRPTR",                     /* VLD Read pointer */
        VID4,
        VID_VLDRPTR,
        0x3ffff00,
        0x3ffff00,
        0,
        REG_32B,
        0
        },
        {"VID4_SCDRPTR",                     /* SCD read pointer */
        VID4,
        VID_SCDRPTR,
        0x3ffff80,
        0x3ffff80,
        0,
        REG_32B,
        0
        },
        {"VID4_VPP_CFG",                        /* HD-PIP parser configuration */
        VID4,
        VID_VPP_CFG,
        0x1f,
        0x1f,
        0,
        REG_16B,
        0
        },
        {"VID4_VPP_DFHT",                        /* HD-PIP parser frame height threshold */
        VID4,
        VID_VPP_DFHT,
        0x3fff,
        0x3fff,
        0,
        REG_16B,
        0
        },
        {"VID4_VPP_DFWT",                        /* HD-PIP parser frame width threshold */
        VID4,
        VID_VPP_DFWT,
        0x3fff,
        0x3fff,
        0,
        REG_16B,
        0
        },
#endif

    /* --------------------- DECODER 5 ---------------------------------------- */
        {"VID5_TIS",                        /* Task Instruction */
        VID5,
        VID_TIS,
        0x3bf,
        0x3bf,
        0,
        REG_32B|REG_SPECIAL,
        0
        },
        {"VID5_PPR",                        /* Picture F Parameters and other parameters */
        VID5,
        VID_PPR,
        0x0fffffff,
        0x0fffffff,
        0,
        REG_32B,
        0
        },
        {"VID5_RFP",                        /* Reconstruction Frame Buffer (256 bytes unit) */
        VID5,
        VID_RFP,
        0x3fffe00,
        0x3fffe00,
        0,
        REG_32B,
        0
        },
        {"VID5_RCHP",                       /* Reconstruction Chroma Frame Buffer */
        VID5,
        VID_RCHP,
        0x3fffe00,
        0x3fffe00,
        0,
        REG_32B,
        0
        },
        {"VID5_FFP",                        /* Forward Frame Buffer (256 bytes unit) */
        VID5,
        VID_FFP,
        0x3fffe00,
        0x3fffe00,
        0,
        REG_32B,
        0
        },
        {"VID5_FCHP",                       /* Forward Chroma Frame Buffer */
        VID5,
        VID_FCHP,
        0x3fffe00,
        0x3fffe00,
        0,
        REG_32B,
        0
        },
        {"VID5_BFP",                        /* Backward Frame Buffer (256 bytes unit) */
        VID5,
        VID_BFP,
        0x3fffe00,
        0x3fffe00,
        0,
        REG_32B,
        0
        },
        {"VID5_BCHP",                       /* Backward Chroma Frame Buffer */
        VID5,
        VID_BCHP,
        0x3fffe00,
        0x3fffe00,
        0,
        REG_32B,
        0
        },
        {"VID5_SRFP",                       /* PIP Reconstruction Frame Buffer (256 bytes unit) */
        VID5,
        VID_SRFP,
        0x3fffe00,
        0x3fffe00,
        0,
        REG_32B,
        0
        },
        {"VID5_SRCHP",                      /* PIP Reconstruction Chroma Frame Buffer */
        VID5,
        VID_SRCHP,
        0x3fffe00,
        0x3fffe00,
        0,
        REG_32B,
        0
        },
        {"VID5_PFP",                        /* Presentation Frame Buffer (256 bytes unit) */
        VID5,
        VID_PFP,
        0x3fffe00,
        0x3fffe00,
        0,
        REG_32B,
        0
        },
        {"VID5_PCHP",                       /* Presentation Chroma Frame Buffer */
        VID5,
        VID_PCHP,
        0x3fffe00,
        0x3fffe00,
        0,
        REG_32B,
        0
        },
        {"VID5_APFP",                       /* PIP Presentation Frame Buffer (256 bytes unit) */
        VID5,
        VID_APFP,
        0x3fffe00,
        0x3fffe00,
        0,
        REG_32B,
        0
        },
        {"VID5_APCHP",                      /* PIP Presentation Chroma Frame Buffer */
        VID5,
        VID_APCHP,
        0x3fffe00,
        0x3fffe00,
        0,
        REG_32B,
        0
        },
        {"VID5_PFLD",                       /* Presentation Field Selection */
        VID5,
        VID_PFLD,
        0xf,
        0xf,
        0,
        REG_32B,
        0
        },
        {"VID5_APFLD",                      /* PIP Presentation Field Selection */
        VID5,
        VID_APFLD,
        0xf,
        0xf,
        0,
        REG_32B,
        0
        },
        {"VID5_BBG",                        /* Bit Buffer start address */
        VID5,
        VID_BBG,
        0x3ffff00,
        0x3ffff00,
        0,
        REG_32B,
        0
        },
        {"VID5_BBS",                        /* Bit Buffer end address */
        VID5,
        VID_BBS,
        0x3ffff00,
        0x3ffff00,
        0,
        REG_32B,
        0
        },
        {"VID5_BBT",                        /* Bit Buffer threshold */
        VID5,
        VID_BBT,
        0x3ffff00,
        0x3ffff00,
        0,
        REG_32B,
        0
        },
        {"VID5_BBL",                        /* Bit Buffer level */
        VID5,
        VID_BBL,
        0x3ffff00,
        0x0,
        0,
        REG_32B,
        0
        },
        {"VID5_PSCPTR",                     /* PSC Byte address in memory */
        VID5,
        VID_PSCPTR,
        0x3ffffff,
        0x0,
        0,
        REG_32B,
        0
        },
        {"VID5_VLDPTR",                     /* VLD start address for multi-decode & trick mode */
        VID5,
        VID_VLDPTR,
        0x3ffffff,
        0x3ffffff,
        0,
        REG_32B,
        0
        },

        {"VID5_DFW",                        /* Decoded Frame Width (in MB) */
        VID5,
        VID_DFW,
        0xff,
        0xff,
        0,
        REG_32B,
        0
        },
        {"VID5_DFH",                        /* Decoded Frame Heigt (in MB) */
        VID5,
        VID_DFH,
        0x7f,
        0x7f,
        0,
        REG_32B,
        0
        },
        {"VID5_DFS",                        /* Decoded Frame Size (in MB) */
        VID5,
        VID_DFS,
        0x1fff,
        0x1fff,
        0,
        REG_32B,
        0
        },
        {"VID5_PFW",                        /* Presentation Frame Width (in MB) */
        VID5,
        VID_PFW,
        0xff,
        0xff,
        0,
        REG_32B,
        0
        },
        {"VID5_PFH",                        /* Presentation Frame Size (in MB) */
        VID5,
        VID_PFH,
        0x7f,
        0x7f,
        0,
        REG_32B,
        0
        },

        {"VID5_STP",                        /* Video bitstream management setup */
        VID5,
        VID_STP,
        0x03,
        0x03,
        0,
        REG_32B,
        0
        },
        {"VID5_CDI",                        /* CD Input from host interface */
        VID5,
        VID_CDI,
        0x0,
        0xffffffff,
        0,
    /*   REG_32B | REG_EXCL_FROM_TEST, */
    REG_32B,
        0
        },
        {"VID5_DMOD",                       /* Decoder Memory Mode: compress, H/n, V/n, ovw */
        VID5,
        VID_DMOD,
        0x1ff,
        0x1ff,
        0,
        REG_32B,
        0
        },
        {"VID5_PMOD",                       /* Presentation Memory Mode */
        VID5,
        VID_PMOD,
        0x1ff,
        0x1ff,
        0,
        REG_32B,
        0
        },
        {"VID5_SDMOD",                      /* Secondary reconstruction Memory Mode: compress, H/n, V/n, ovw */
        VID5,
        VID_SDMOD,
        0x37c,
        0x37c,
        0,
        REG_32B,
        0
        },
        {"VID5_APMOD",                      /* PIP Presentation Memory Mode */
        VID5,
        VID_APMOD,
        0x1fc,
        0x1fc,
        0,
        REG_32B,
        0
        },
        {"VID5_SRS",                        /* Soft Reset */
        VID5,
        VID_SRS,
        0x0,
        0x1,
        0,
        REG_32B|REG_SPECIAL,               /* Reset on Write */
        0
        },
        {"VID5_PRS",                        /* Pipe Reset */
        VID5,
        VID_PRS,
        0x0,
        0x1,
        0,
        REG_32B|REG_SPECIAL,               /* Reset on Write */
        0
        },

        {"VID5_ITM",                        /* Interrupt mask */
        VID5,
        VID_ITM,
        0x3fff,
        0x3fff,
        0,
        REG_32B,
        0
        },
        {"VID5_ITS",                        /* Interrupt status */
        VID5,
        VID_ITS,
        0x3fff,
        0x0,
        0,
        REG_32B | REG_SPECIAL | REG_SIDE_READ,
        0
        },
        {"VID5_STA",                        /* Status register */
        VID5,
        VID_STA,
        0x3fff,
        0x0,
        0,
        REG_32B,
        0
        },

        {"VID5_CDWL",                       /* Top address limit of CD write in Trick mode */
        VID1,
        VID_CDWL,
        0x3ffff80,
        0x3ffff80,
        0,
        REG_32B,
        0
        },
        {"VID5_SCDPTR",                     /* SCD addr pointer for launch in Trick mode */
        VID1,
        VID_SCDPTR,
        0x3ffffff,
        0x3ffffff,
        0,
        REG_32B,
        0
        },

        {"VID5_CDC",                        /* CD count register */
        VID5,
        VID_CDC,
        0xffffff,
        0x0,
        0,
        REG_32B,
        0
        },
        {"VID5_SCDC",                       /* SCD count register */
        VID5,
        VID_SCDC,
        0xffffff,
        0x0,
        0,
        REG_32B,
        0
        },

        {"VID5_PES_CFG",                    /* PES parser Configuration */
        VID5,
        VID_PES_CFG,
        0xff,
        0xff,
        0,
        REG_32B,
        0
        },
        {"VID5_PES_SI",                     /* PES Stream Id */
        VID5,
        VID_PES_SI,
        0xff,
        0xff,
        0,
        REG_32B,
        0
        },
        {"VID5_PES_FSI",                    /* Stream ID Filter */
        VID5,
        VID_PES_FSI,
        0xff,
        0xff,
        0,
        REG_32B,
        0
        },
        {"VID5_PES_TFI",                    /* Trick Filtering ID (Fast Forward ID) */
        VID5,
        VID_PES_TFI,
        0xff,
        0xff,
        0,
        REG_32B,
        0
        },
        {"VID5_PES_DSM",                    /* DSM Value */
        VID5,
        VID_PES_DSM,
        0xff,
        0x0,
        0,
        REG_32B,
        0
        },
        {"VID5_PES_TS0",                    /* Time-Stamp Value LSW */
        VID5,
        VID_PES_TS0,
        0xffffffff,
        0x0,
        0,
        REG_32B,
        0
        },
        {"VID5_PES_TS1",                    /* Time-Stamp Value MSW */
        VID5,
        VID_PES_TS1,
        0x1,
        0x0,
        0,
        REG_32B,
        0
        },
        {"VID5_PES_ASS",                    /* DSM and TS association flags */
        VID5,
        VID_PES_ASS,
        0x03,
        0x0,
        0,
        REG_32B,
        0
        },
        {"VID5_HDF",                        /* Header Data FIFO */
        VID5,
        VID_HDF,
        0x0001ffff,
        0x0,
        0,
        REG_32B | REG_SPECIAL | REG_SIDE_READ,
        0
        },
        {"VID5_HLNC",                       /* Launch start code detector */
        VID5,
        VID_HLNC,
        0x0,
        0x3,
        0,
        REG_32B|REG_SPECIAL,
        0
        },

        {"VID5_QMWI",                       /* Quantization matrix data, intra table */
        VID5,
        VID_QMWI,
        0x0,
        0xffffffff,
        0,
        REG_32B|REG_ARRAY,
        16
        },
        {"VID5_QMWNI",                      /* Quantization matrix data, non intra table */
        VID5,
        VID_QMWNI,
        0x0,
        0xffffffff,
        0,
        REG_32B|REG_ARRAY,
        16
        },
#if defined(VIDEO_7020)
        {"VID5_CDR",                        /* CD from register */
        VID5,
        VID_CDR,
        0x1,
        0x1,
        0,
        REG_32B,
        0
        },
        {"VID5_CDWPTR",                     /* Compressed data write pointer */
        VID5,
        VID_CDWPTR,
        0x3ffff80,
        0x3ffff80,
        0,
        REG_32B,
        0
        },
        {"VID5_VLDRPTR",                     /* VLD Read pointer */
        VID5,
        VID_VLDRPTR,
        0x3ffff00,
        0x3ffff00,
        0,
        REG_32B,
        0
        },
        {"VID5_SCDRPTR",                     /* SCD read pointer */
        VID5,
        VID_SCDRPTR,
        0x3ffff80,
        0x3ffff80,
        0,
        REG_32B,
        0
        },
        {"VID5_VPP_CFG",                        /* HD-PIP parser configuration */
        VID5,
        VID_VPP_CFG,
        0x1f,
        0x1f,
        0,
        REG_16B,
        0
        },
        {"VID5_VPP_DFHT",                        /* HD-PIP parser frame height threshold */
        VID5,
        VID_VPP_DFHT,
        0x3fff,
        0x3fff,
        0,
        REG_16B,
        0
        },
        {"VID5_VPP_DFWT",                        /* HD-PIP parser frame width threshold */
        VID5,
        VID_VPP_DFWT,
        0x3fff,
        0x3fff,
        0,
        REG_16B,
        0
        },
#endif
#if defined(VIDEO_7015)
        {"VID_DBG_INS1",                    /* Current instruction in decoder part 1 */
        STI7015_BASE_ADDRESS,
        VID_DBG_INS1,
        0x1fffffff,
        0x00000000,
        0,
        REG_32B,
        0
        },
        {"VID_DBG_INS2",                    /* Current instruction in decoder part 2 */
        STI7015_BASE_ADDRESS,
        VID_DBG_INS2,
        0x0007ffff,
        0x00000000,
        0,
        REG_32B,
        0
        }
#else
        {"VID_DBG_INS1",                    /* Current instruction in decoder part 1 */
        STI7020_BASE_ADDRESS,
        VID_DBG_INS1,
        0x1fffffff,
        0x00000000,
        0,
        REG_32B,
        0
        },
        {"VID_DBG_INS2",                    /* Current instruction in decoder part 2 */
        STI7020_BASE_ADDRESS,
        VID_DBG_INS2,
        0x0007ffff,
        0x00000000,
        0,
        REG_32B,
        0
        }
#endif
	};
#elif defined(VIDEO_7710) || defined (VIDEO_5100) || defined (VIDEO_7100) || defined(VIDEO_7109)
    static const Register_t VideoReg[] =
    {
        {"VID_CFG_VIDIC",                   /* Video Decoder Interconnect Configuration */
        VIDEO_CFG_BASE_ADDRESS,
        VID_CFG_VIDIC,
        0x0000007f,
        0x0000007f,
        0,
        REG_32B,
        0
        },
        {"VID_EXE",
        VID1,
        VID_EXE,
        0x00000001,
        0x00000001,
        0,
        REG_32B,
        0
        },
        {"VID_REQS",
        VID1,
        VID_DBG_REQS,
        0x000000ff,
        0x000000fF,
        0,
        REG_32B,
        0
        },
        {"VID_REQC",
        VID1,
        VID_DBG_REQC,
        0x00000003,
        0x00000003,
        0,
        REG_32B,
        0
        },
        {"VID_DBG2",
        VID1,
        VID_DBG_DBG2,
        0x000000ff,
        0x000000ff,
        0,
        REG_32B,
        0
        },
        {"VID_VLDS",         		/* spec??*/
        VID1,
        VID_DBG_VLDS,
        0x000000ff,
        0x000000ff,
        0,
        REG_32B,
        0
        },
        {"VID_VMBN",
        VID1,
        VID_DBG_VMBN,
        0x00003fff,
        0x00003fff,
        0,
        REG_32B,
        0
        },
        {"VID_MBE0",
        VID1,
        VID_DBG_MBE0,
        0xffffffff,
        0xffffffff,
        0,
        REG_32B,
        0
        },
        {"VID_MBE1",
        VID1,
        VID_DBG_MBE1,
        0xffffffff,
        0xffffffff,
        0,
        REG_32B,
        0
        },
        {"VID_MBE2",
        VID1,
        VID_DBG_MBE2,
        0xffffffff,
        0xffffffff,
        0,
        REG_32B,
        0
        },
        {"VID_MBE3",
        VID1,
        VID_DBG_MBE3,
        0xffffffff,
        0xffffffff,
        0,
        REG_32B,
        0
        },
        {"VID5_QMWI",                       /* Quantization matrix data, intra table */
        VID1_QTABLES,
        VID_QMWI,
        0x0,
        0xffffffff,
        0,
        REG_32B|REG_ARRAY,
        16
        },
        {"VID5_QMWNI",                      /* Quantization matrix data, non intra table */
        VID1_QTABLES,
        VID_QMWNI,
        0x0,
        0xffffffff,
        0,
        REG_32B|REG_ARRAY,
        16
        },
        {"VID_TIS",
        VID1,
        VID_TIS,
        0x0000000f,
        0x0000000f,
        0,
        REG_32B,
        0
        },
        {"VID_PPR",
        VID1,
        VID_PPR,
        0x7fffffff,
        0x7fffffff,
        0,
        REG_32B,
        0
        },
         {"VID_SRS",
        VID1,
        VID_SRS,
        0x00000001,
        0x00000001,
        0,
        REG_32B,
        0
        },
         {"VID_ITM",
        VID1,
        VID_ITM,
        0x0000007f,
        0x0000007f,
        0,
        REG_32B,
        0
        },
         {"VID_ITS",
        VID1,
        VID_ITS,
        0x0000007f,
        0x0000007f,
        0,
        REG_32B,
        0
        },
        {"VID_STA",
        VID1,
        VID_STA,
        0x0000007f,
        0x0000007f,
        0,
        REG_32B,
        0
        },
        {"VID_DFH",
        VID1,
        VID_DFH,
        0x0000007f,
        0x0000007f,
        0,
        REG_32B,
        0
        },
        {"VID_DFS",
        VID1,
        VID_DFS,
        0x00001fff,
        0x00001fff,
        0,
        REG_32B,
        0
        },
        {"VID_DFW",
        VID1,
        VID_DFW,
        0x0000007f,
        0x0000007f,
        0,
        REG_32B,
        0
        },
        {"VID_BBS",
        VID1,
        VID_BBS,
        0xffffff00,
        0xffffff00,
        0,
        REG_32B,
        0
        },
        {"VID_BBE",
        VID1,
        VID_BBE,
        0xffffff00,
        0xffffff00,
        0,
        REG_32B,
        0
        },
         {"VID_VLDRL",
        VID1,
        VID_VLDRL,
        0xffffff00,
        0xffffff00,
        0,
        REG_32B,
        0
        },
         {"VID_VLDPTR",
        VID1,
        VID_VLDPTR,
        0xffffff80,
        0xffffff80,
        0,
        REG_32B,
        0
        },
         {"VID_MBNM",
        VID1,
        VID_MBNM,
        0x007f007f,
        0x007f007f,
        0,
        REG_32B,
        0
        },
         {"VID_BCHP",
        VID1,
        VID_BCHP,
        0xfffffe00,
        0xfffffe00,
        0,
        REG_32B,
        0
        },
         {"VID_BFP",
        VID1,
        VID_BFP,
        0xfffffe00,
        0xfffffe00,
        0,
        REG_32B,
        0
        },
         {"VID_FCHP",
        VID1,
        VID_FCHP,
        0xfffffe00,
        0xfffffe00,
        0,
        REG_32B,
        0
        },
         {"VID_FFP",
        VID1,
        VID_FFP,
        0xfffffe00,
        0xfffffe00,
        0,
        REG_32B,
        0
        },
         {"VID_RCHP",
        VID1,
        VID_FCHP,
        0xfffffe00,
        0xfffffe00,
        0,
        REG_32B,
        0
        },
         {"VID_RFP",
        VID1,
        VID_FFP,
        0xfffffe00,
        0xfffffe00,
        0,
        REG_32B,
        0
        },
#if defined(VIDEO_7710) || defined(VIDEO_7100) || defined(VIDEO_7109)
         {"VID_FRN",
        VID1,
        VID_FRN,
        0x000000ff,
        0x000000ff,
        0,
        REG_32B,
        0
        },
         {"VID_MBNS",
        VID1,
        VID_MBNS,
        0x007f007f,
        0x007f007f,
        0,
        REG_32B,
        0
        },
         {"VID_RCM",
        VID1,
        VID_RCM,
        0x000007ff,
        0x000007ff,
        0,
        REG_32B,
        0
        },
         {"VID_SRCHP",
        VID1,
        VID_SRCHP,
        0xffffff00,
        0xffffff00,
        0,
        REG_32B,
        0
        },
         {"VID_SRFP",
        VID1,
        VID_SRFP,
        0xffffff00,
        0xffffff00,
        0,
        REG_32B,
        0
        },
#endif /* specific VIDEO_7710, VIDEO_7100 and VIDEO_7109 registers.*/
    };
#else
    #error "No video decoder defined" ;
#endif /* defined(VIDEO_SDMPEGO2) */

/* Nbr of registers */
#define VIDEO_REGISTERS (sizeof(VideoReg)/sizeof(Register_t))



#ifdef __cplusplus
}
#endif

#endif /* __DENC_TREG_H */
/* ------------------------------- End of file video_treg.h ---------------------------- */


