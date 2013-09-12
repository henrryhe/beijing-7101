/*****************************************************************************

File name   :

Description : VIN registers

COPYRIGHT (C) ST-Microelectronics 2000.

Date               Modification                 Name
----               ------------                 ----
03/10/00           Created                      Jerome Audu

*****************************************************************************/

#ifndef __HAL_VIN_REG_H
#define __HAL_VIN_REG_H

/* Includes --------------------------------------------------------------- */
#include "stddefs.h"
#include "stsys.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Exported Types --------------------------------------------------------- */

/* Exported Constants ----------------------------------------------------- */

/******************************************************************************/
/*- High Definition Video Input Register                                     -*/
/******************************************************************************/

/*- Control register -----------------------------------------------------------*/
#define HDIN_CTRL                     0xB0    /* HD Pixel Port - RGB Interface Control Register */
#define HDIN_CTRL_MASK                0x000001FF

/* toggle on enbled/disabled Interface */
#define HDIN_CTRL_EN_MASK                 1
#define HDIN_CTRL_EN_EMP                  8
/* toggle to RGB / YCbCr Conversion */
#define HDIN_CTRL_CNV_MASK                1
#define HDIN_CTRL_CNV_EMP                 7
/* toggle Top/bootom field detection */
#define HDIN_CTRL_TBM_MASK              1
#define HDIN_CTRL_TBM_EMP               6
/* UPPER_FILED signal will be active high */
#define HDIN_CTRL_UFP_MASK              1
#define HDIN_CTRL_UFP_EMP               5
/* "0" rising edge on RGB_clock in RGB mode */
#define HDIN_CTRL_CAE_MASK              1
#define HDIN_CTRL_CAE_EMP               4
/* "0" rising edge on RGB_VSI & RGB_HSI */
#define HDIN_CTRL_SYNC_MASK               3
#define HDIN_CTRL_SYNC_EMP                2
/* Set the operational mode of the interface */
#define HDIN_CTRL_MDE_MASK                3
#define HDIN_CTRL_MDE_EMP                 0

/*- Reconstructed registers ------------------------------------------------*/
#define HDIN_DFW                       0x60     /* horizontal size of the reconstructed picture, in units of 16 pixels */
#define HDIN_DFW_MASK                    0xFF
#define HDIN_DMOD                        0x80     /* parameter concerning the storage type of the reconstructed picture */
#define HDIN_DMOD_MASK                   0x7C
/* Horizontal & Vertical Decimation Mode + Scan Type */
#define HDIN_DMOD_DEC_MASK             0x1F
#define HDIN_DMOD_DEC_EMP              2

#define HDIN_RCHP_32                     0x0C     /* Reconstructed Chroma Frame Pointer */
#define HDIN_RCHP_MASK                   0x03FFFE00
#define HDIN_RFP_32                      0x08     /* Reconstructed Luma Frame Pointer */
#define HDIN_RFP_MASK                    0x03FFFE00
/*------------------------------------------------------------------------*/

/*- Interrupt & Status registers -----------------------------------------*/
#define HDIN_ITM                         0x98     /* Mask register    */
#define HDIN_ITM_MASK                    0x03
#define HDIN_ITS_8                       0x9C     /* Interrupt Status */
#define HDIN_ITS_MASK                    0x03
#define HDIN_STA_8                       0xA0     /* Status Register */
#define HDIN_STA_MASK                    0x03

/*- For External Sync -*/
#define HDIN_LL                       0xB4    /* HD Pixel Port - RGB Interface Lower Horizontal Limit */
#define HDIN_LL_MASK                  0x000007FF
#define HDIN_UL                       0xB8    /* HD Pixel Port - RGB Interface Upper Horizontal Limit */
#define HDIN_UL_MASK                  0x000007FF
#define HDIN_LNCNT                    0xBC    /* HD Pixel Port - RGB Interface Line Counter */
#define HDIN_LNCNT_MASK               0x000003FF
#define HDIN_HOFF                     0xC0    /* HD Pixel Port - Time between Horizontal Sync and the rising edge of HSyncOut */
#define HDIN_HOFF_MASK                0x000007FF
#define HDIN_VLOFF                    0xC4    /* HD Pixel Port - Vertical Line Offset for VSyncOut */
#define HDIN_VLOFF_MASK               0x000003FF
#define HDIN_VHOFF                    0xC8    /* HD Pixel Port - Horizontal Line Offset for VSyncOut */
#define HDIN_VHOFF_MASK               0x000007FF

/*- Active Video -*/
#define HDIN_VBLANK                   0xCC    /* HD Pixel Port - Vertical Offset for Active Video */
#define HDIN_VBLANK_MASK              0x000003FF
#define HDIN_HBLANK                   0xD0    /* HD Pixel Port - Time between Horizontal Sync and active video */
#define HDIN_HBLANK_MASK              0x000007FF
#define HDIN_HSIZE                    0xD4    /* HD Pixel Port - Number of samples per input line */
#define HDIN_HSIZE_MASK               0x000003FF
#define HDIN_TOP_VEND                     0xD8    /* HD Pixel Port - Last line of the top field/frame */
#define HDIN_TOP_VEND_MASK                0x000003FF
#define HDIN_BOTTOM_VEND                  0xDC    /* HD Pixel Port - Last line of the bottom field */
#define HDIN_BOTTOM_VEND_MASK             0x000003FF

/**********************************************************************************/
/*- Standard Definition Video Input Register                                     -*/
/**********************************************************************************/

/*------------------------------------------------------------------------------*/
/*- STi7015 part ---------------------------------------------------------------*/

/*- Control register -----------------------------------------------------------*/
#define SDINn_CTRL                    0xB0    /* SD Pixel Port Control Register */
#define SDINn_CTRL_MASK               0x0000007F

/* toggle on enabled/disabled Interface */
#define SDINn_CTRL_SDE_MASK                1
#define SDINn_CTRL_SDE_EMP                 0
/* toggle CCIR656 or External Sync Mode */
#define SDINn_CTRL_MDE_MASK                1
#define SDINn_CTRL_MDE_EMP                 1
/* toggle ancillary data in nibble or standard mode */
#define SDINn_CTRL_ANC_MASK                1
#define SDINn_CTRL_ANC_EMP                 5
/* "0" rising edge on VSI & HSI */
#define SDINn_CTRL_SYNC_MASK               3
#define SDINn_CTRL_SYNC_EMP                3
/* toggle ancillary data capture or packets mode */
#define SDINn_CTRL_DC_MASK                 1
#define SDINn_CTRL_DC_EMP                  2
/* toggle progressive or interlace input mode */
#define SDINn_CTRL_TVS_MASK               1
#define SDINn_CTRL_TVS_EMP                6

/*- Reconstructed registers ------------------------------------------------*/
#define SDINn_ANC_32                     0x50     /* Ancillary Data Pointer */
#define SDINn_ANC_MASK                   0x03FFFF00
#define SDINn_DMOD                       0x80     /* Storage Memory Mode */
#define SDINn_DMOD_MASK                  0x0030
/* Horizontal Decimation Mode */
#define SDINn_DMOD_HDEC_MASK             3
#define SDINn_DMOD_HDEC_EMP              4


#define SDINn_DFW                        0x60     /* Frame Width */
#define SDINn_DFW_MASK                   0xFF
#define SDINn_RCHP_32                    0x0C     /* Reconstructed Chroma Frame Pointer */
#define SDINn_RCHP_MASK                  0x03FFFE00
#define SDINn_RFP_32                     0x08     /* Reconstructed Luma Frame Pointer */
#define SDINn_RFP_MASK                   0x03FFFE00

/*- Interrupt & Status registers -----------------------------------------*/
#define SDINn_ITM                        0x98   /* Interrupt Mask */
#define SDINn_ITM_MASK                   0x03
#define SDINn_ITS_8                      0x9C     /* Interrupt Status */
#define SDINn_ITS_MASK                   0x03
#define SDINn_STA_8                      0xA0    /* Status Register */
#define SDINn_STA_MASK                   0x03

/*- For External Sync -*/
#define SDINn_LL                      0xB4    /* SD Pixel Port Lower Horizontal Limit */
#define SDINn_LL_MASK                 0x000001FF
#define SDINn_UL                      0xB8    /* SD Pixel Port Upper Horizontal Limit */
#define SDINn_UL_MASK                 0x000001FF
#define SDINn_LNCNT                   0xBC    /* SD Pixel Port Line Counter */
#define SDINn_LNCNT_MASK              0x000000FF
#define SDINn_HOFF                    0xC0    /* Horizontal Offset for HSyncOut */
#define SDINn_HOFF_MASK               0x000003FF
#define SDINn_VLOFF                   0xC4    /* Vertical Line Offset for VSyncOut */
#define SDINn_VLOFF_MASK              0x000000FF
#define SDINn_VHOFF                   0xC8    /* Horizontal Line Offset for VSyncOut */
#define SDINn_VHOFF_MASK              0x000003FF

/*- Active Video -*/
#define SDINn_VBLANK                  0xCC    /* Vertical blanking after the trailing edge of vertical sync */
#define SDINn_VBLANK_MASK             0x00000007
#define SDINn_HBLANK                  0xD0    /* Time between Horizontal Sync and active video */
#define SDINn_HBLANK_MASK             0x000001FF

/* Ancillary Data */
#define SDINn_LENGTH               0xD4    /* Ancillary Data Capture Length */
#define SDINn_LENGTH_MASK          0x0000007F

/*------------------------------------------------------------------------------*/
/*- STi7020 part ---------------------------------------------------------------*/

/*- Control register -----------------------------------------------------------*/
#define SDINn_CTL                     0xB0    /* SD Pixel Port Control Register */
#define SDINn_CTL_MASK                0x000007FF

/* toggle on enabled/disabled Interface */
#define SDINn_CTL_SDE_MASK                     1
#define SDINn_CTL_SDE_EMP                      0
/* toggle on enabled/disabled Ancillary data capture */
#define SDINn_CTL_ADE_MASK                     1
#define SDINn_CTL_ADE_EMP                      1
/* toggle positive/negative polarity for external sync */
/* PS: only available if external sync enable          */
#define SDINn_CTL_ESP_MASK                     1
#define SDINn_CTL_ESP_EMP                      2
/* toggle on enabled/disabled VSync bottom half line */
#define SDINn_CTL_VBH_MASK                     1
#define SDINn_CTL_VBH_EMP                      3
/* toggle the mode of  of operation of interface */
/* mode CCIR-656 or External sync                */
#define SDINn_CTL_MDE_MASK                     1
#define SDINn_CTL_MDE_EMP                      4
/* toggle on HRef polarity */
#define SDINn_CTL_HRP_MASK                     1
#define SDINn_CTL_HRP_EMP                      5
/* toggle on VRef polarity */
#define SDINn_CTL_VRP_MASK                     1
#define SDINn_CTL_VRP_EMP                      6
/* toggle on enabled/disabled data capture mode */
#define SDINn_CTL_DC_MASK                      1
#define SDINn_CTL_DC_EMP                       7
/* toggle on sync phase not OK (top field selection) */
#define SDINn_CTL_SPN_MASK                     1
#define SDINn_CTL_SPN_EMP                      8
/* toggle on enabled/disabled extend 1-254 mode */
#define SDINn_CTL_E254_MASK                    1
#define SDINn_CTL_E254_EMP                     9
/* toggle odd even/not VSync vertical reference input */
#define SDINn_CTL_OEV_MASK                     1
#define SDINn_CTL_OEV_EMP                      10

/*- Aquisition window -*/
#define SDINn_TFO                     0xB4    /* SD input top field offset */
#define SDINn_TFO_MASK                0x07FF1FFF
#define SDINn_TFS                     0xB8    /* SD input top field stop   */
#define SDINn_TFS_MASK                0x07FF1FFF

#define SDINn_BFO                     0xBC    /* SD input bottom field offset */
#define SDINn_BFO_MASK                0x07FF1FFF
#define SDINn_BFS                     0xC0    /* SD input bottom field stop   */
#define SDINn_BFS_MASK                0x07FF1FFF

#define SDINn_YDO_MASK                0x000007FF
#define SDINn_YDO_EMP                 16
#define SDINn_YDS_MASK                0x000007FF
#define SDINn_YDS_EMP                 16
#define SDINn_XDO_MASK                0x00001FFF
#define SDINn_XDO_EMP                 0
#define SDINn_XDS_MASK                0x00001FFF
#define SDINn_XDS_EMP                 0

/*- Control register -----------------------------------------------------------*/
#define SDINn_VSD                     0xC4    /* SD input vertical synchronization delay   */
#define SDINn_VSD_MASK                0x07FF07FF

#define SDINn_HSD                     0xC8    /* SD input horizontal synchronization delay */
#define SDINn_HSD_MASK                0x07FF07FF

#define SDINn_HLL                     0xCC    /* SD input half-line length                 */
#define SDINn_HLFLN                   0xD0    /* SD input half-lines per verical           */

#define SDINn_APS                     0xD4    /* SD input ancillary data page size         */
#define SDINn_APS_MASK                0x00000FFF


/**********************************************************************************/
/*- Digital Video Port Register (DVP)                                            -*/
/**********************************************************************************/

/*- Control register -----------------------------------------------------------*/
#define GAM_DVPn_CTRL                0x00                /* DVP Control Register */
#if defined(ST_7109)
#define GAM_DVPn_CTRL_MASK           0xE08FA7FF
#else
#define GAM_DVPn_CTRL_MASK           0x808FA3FF
#endif /* ST_7109 */
#define GAM_DVPn_CTRL_EN_EMP         0                   /* toggle on enbled/disabled Interface */
#define GAM_DVPn_CTRL_EN_MASK        0x01
#define GAM_DVPn_CTRL_FULL_MASK      0x0fffff

#define GAM_DVPn_CTRL_ANC_DATA_EMP   1                   /* Ena Ancillary Data */
#define GAM_DVPn_CTRL_ANC_DATA_MASK  0x01

#define GAM_DVPn_CTRL_EXT_SYNC_POL_EMP   2               /* Ext_sync_polarity */
#define GAM_DVPn_CTRL_EXT_SYNC_POL_MASK  0x01            /* 0: negative for both horizontal and vertical */
                                                         /* 1: positive for both horizontal and vertical */

#define GAM_DVPn_CTRL_VSOUT_EMP   3                      /* Vsync_bot_half_line_en */
#define GAM_DVPn_CTRL_VSOUT_MASK  0x01                   /* 0: vsout starts at the beginning of the last top field line */
                                                         /* 1: vsout starts at the middle of the last top field line */

#define GAM_DVPn_CTRL_MDE_EMP        4                   /* Synchro CCIR / External */
#define GAM_DVPn_CTRL_MDE_MASK       0x01

/* Href polarity */
/* 0: The negative edge of Href (defined by bit[4]) is taken as reference for the horizontal counter reset. */
/* 1: The positive edge of Href (defined by bit[4]) is taken as reference for the horizontal counter reset */
#define GAM_DVPn_CTRL_HREF_POL_EMP   5
#define GAM_DVPn_CTRL_HREF_POL_MASK  0x01

/* Vref polarity */
/* 0: The negative edge of Vref (if bit[4]=1) is taken as reference for the vertical counter reset. */
/* 1: The positive edge of Vref (if bit[4]=1) is taken as reference for the vertical counter reset */
/* Note : In EAV/SAV mode, (bit[4] = 0). the positive edge is always active but the normal */
/* rising edge of V is phased with the next falling edge of H to respect the top field configuration */
#define GAM_DVPn_CTRL_VREF_POL_EMP   6
#define GAM_DVPn_CTRL_VREF_POL_MASK  0x01

/* Phase */
/* first pixel signification of capture window when 8 bits video data capture */
/* phase[7] = 0 first pixel is complete (i.e CB0_Y0_CR0) */
/* phase[7] = 1 first pixel is not complete (i.e Y1) */
/* phase[8] = 0 number of pixel to capture is even */
/* phase[8] = 1 number of pixel to capture is odd */
#define GAM_DVPn_CTRL_PHASE_Y1_EMP      7
#define GAM_DVPn_CTRL_PHASE_Y1_MASK  0x01
#define GAM_DVPn_CTRL_PHASE_ODD_EMP     8
#define GAM_DVPn_CTRL_PHASE_ODD_MASK 0x01

/* Oddeven_not_vsync */
/* 0: In external sync mode, vertical reference is a Vsync signal */
/* 1: In external sync mode, vertical reference is an odd/even signal */
#define GAM_DVPn_CTRL_ODDEVEN_EXT_SYNC_EMP   9
#define GAM_DVPn_CTRL_ODDEVEN_EXT_SYNC_MASK  0x01

/* Ena H-Resize */
/* 0: The horizontal resize engine is disabled */
/* 1: The horizontal resize engine is enabled */
#define GAM_DVPn_CTRL_HRESIZE_EMP    10
#define GAM_DVPn_CTRL_HRESIZE_MASK   0x01

/* Ena V-Resize */
/* 0: The vertical resize engine is disabled */
/* 1: The vertical resize engine is enabled */
#define GAM_DVPn_CTRL_VRESIZE_EMP    11
#define GAM_DVPn_CTRL_VRESIZE_MASK   0x01

/* Valid_anc_page_size_ext */
/* 0: Anc_page_size extracted from ancillary data bit[5:0] from header sixth byte */
/* 1: Anc_page_size given by DVP_APS register */
#define GAM_DVPn_CTRL_ANC_SIZE_EXT_EMP    13
#define GAM_DVPn_CTRL_ANC_SIZE_EXT_MASK   0x01

/* Synchro_phase_notok */
/* 0: External vertical and horizontal synchronization signals are presumed  */
/* to be in phase to initialize a top field */
/* 1: External vertical and horizontal synchronization signals may be out */
/* of phase, in this case the vertical synchronization signal must be earlier */
/* or later than horizontal synchronization signal with a maximum */
/* of 1/4 of line length */
#define GAM_DVPn_CTRL_SYNC_PHASE_EMP    15
#define GAM_DVPn_CTRL_SYNC_PHASE_MASK   0x01


/* Extended-1_254  */
#define GAM_DVPn_CTRL_INPUT_CLIPPING_EMP      16
#define GAM_DVPn_CTRL_INPUT_CLIPPING_MASK   0x01

/*[17] MIX_CAPT_PHASE*/
/*0: Compositor data resynchronized with positive edge of pixel clock*/
/*1: Compositor data resynchronized with negative edge of pixel clock*/
#define GAM_DVPn_CTRL_MIX_CAPT_PHASE_EMP      17
#define GAM_DVPn_CTRL_MIX_CAPT_PHASE_MASK   0x01

/* [18] MIX_CAPT_EN */
/*0: No compositor capture*/
/*1: Compositor capture enabled*/
#define GAM_DVPn_CTRL_MIX_CAPT_EN_EMP      18
#define GAM_DVPn_CTRL_MIX_CAPT_EN_MASK   0x01

#define GAM_DVPn_CTRL_MIX_EMP        19                     /* Mix Capt Prerequest */
#define GAM_DVPn_CTRL_MIX_MASK       0x01

#define GAM_DVPn_CTRL_BNL_EMP        23                     /* big_not_little */
#define GAM_DVPn_CTRL_BNL_MASK       0x01


#define GAM_DVPn_CTRL_FORCE_INTER_EN_EMP        29          /* This bit should be set in case of SMPTE295M interlaced stream */
#define GAM_DVPn_CTRL_FORCE_INTER_EN_MASK       0x01


#define GAM_DVPn_CTRL_HD_EN_EMP        30                   /* capture SD/HD data on 8/16 bits */
#define GAM_DVPn_CTRL_HD_EN_MASK       0x01

/* DVP_RST */
/* 1: Synchro input disabled (either external or embedded) */
/* 0: Synchro inputs enabled  */
#define GAM_DVPn_CTRL_RST_EMP      31
#define GAM_DVPn_CTRL_RST_MASK   0x01

/* X-Y Top Field Offset */
#define GAM_DVPn_TFO                 0x04
#define GAM_DVPn_TFO_MASK            0x07FF1FFF
#define GAM_DVPn_TFO_XDO_EMP         0
#define GAM_DVPn_TFO_XDO_MASK        0x1FFF
#define GAM_DVPn_TFO_YDO_EMP         16
#define GAM_DVPn_TFO_YDO_MASK        0x7FF

/* X-Y Top Field Stop */
#define GAM_DVPn_TFS                 0x08
#define GAM_DVPn_TFS_MASK            0x07FF1FFF
#define GAM_DVPn_TFS_XDS_EMP         0
#define GAM_DVPn_TFS_XDS_MASK        0x1FFF
#define GAM_DVPn_TFS_YDS_EMP         16
#define GAM_DVPn_TFS_YDS_MASK        0x7FF

/* X-Y Bottom Field Offset */
#define GAM_DVPn_BFO                 0x0C
#define GAM_DVPn_BFO_MASK            0x07FF1FFF
#define GAM_DVPn_BFO_XDO_EMP         0
#define GAM_DVPn_BFO_XDO_MASK        0x1FFF
#define GAM_DVPn_BFO_YDO_EMP         16
#define GAM_DVPn_BFO_YDO_MASK        0x7FF

/* X-Y Bottom Field Stop */
#define GAM_DVPn_BFS                 0x10
#define GAM_DVPn_BFS_MASK            0x07FF1FFF
#define GAM_DVPn_BFS_XDS_EMP         0
#define GAM_DVPn_BFS_XDS_MASK        0x1FFF
#define GAM_DVPn_BFS_YDS_EMP         16
#define GAM_DVPn_BFS_YDS_MASK        0x7FF

#define GAM_DVPn_VTP                 0x14                /* Video Top field memory pointer */
#define GAM_DVPn_VTP_MASK            0x03FFFFFF

#define GAM_DVPn_VBP                 0x18                /* Video Bottom field memory pointer */
#define GAM_DVPn_VBP_MASK            0x03FFFFFF

#define GAM_DVPn_VMP                 0x1C                /* Video Memory Pitch */
#define GAM_DVPn_VMP_MASK            0x00001FFF

#define GAM_DVPn_CVS                 0x20                /* Captured Video Window size */
#define GAM_DVPn_CVS_MASK            0xC7FF07FF
#define GAM_DVPn_CVS_WIDTH_EMP       0                   /* Pixmap width */
#define GAM_DVPn_CVS_WIDTH_MASK      0x7FF
#define GAM_DVPn_CVS_HEIGHT_EMP      16                  /* top Video window height */
#define GAM_DVPn_CVS_HEIGHT_MASK     0x7FF
#define GAM_DVPn_CVS_TOPBOT_REL_EMP  30                  /* bot_size relation */
#define GAM_DVPn_CVS_TOPBOT_REL_MASK 0x3


#define GAM_DVPn_VSD                 0x24                /* Vertical Synchronisation delay */
#define GAM_DVPn_VSD_MASK            0x07FF07FF

#define GAM_DVPn_HSD                 0x28                /* Horizontal Synchronisation delay */
#define GAM_DVPn_HSD_MASK            0x1FFF1FFF

#define GAM_DVPn_HLL                 0x2C                /* Half line length */
#define GAM_DVPn_HLL_MASK            0x00000FFF

#define GAM_DVPn_HSRC                0x30                /* Horizontal sample rate converter */
#define GAM_DVPn_HSRC_MASK           0x01070FFF
#define GAM_DVPn_HSRC_INC_EMP        0
#define GAM_DVPn_HSRC_INC_MASK       0xFFF
#define GAM_DVPn_HSRC_IP_EMP         16
#define GAM_DVPn_HSRC_IP_MASK        0x7
#define GAM_DVPn_HSRC_FILTER_EMP     24
#define GAM_DVPn_HSRC_FILTER_MASK    0x1

#define GAM_DVPn_HFC                 0x34                /* Horizontal filter coefficients (register file) */

#define GAM_DVPn_VSRC                0x60                /* Vertical sample rate converter */
#define GAM_DVPn_VSRC_MASK           0x01770FFF
#define GAM_DVPn_VSRC_INC_EMP        0
#define GAM_DVPn_VSRC_INC_MASK       0xFFF
#define GAM_DVPn_VSRC_IPT_EMP        16
#define GAM_DVPn_VSRC_IPT_MASK       0x7
#define GAM_DVPn_VSRC_IPB_EMP        20
#define GAM_DVPn_VSRC_IPB_MASK       0x7
#define GAM_DVPn_VSRC_FILTER_EMP     24
#define GAM_DVPn_VSRC_FILTER_MASK    0x1


#define GAM_DVPn_VFC                 0x64                /* Vertical filter coefficients (register file) */

#define GAM_DVPn_ITM                 0x98                /* Interrupt Mask */
#define GAM_DVPn_ITM_MASK            0x00000018

#define GAM_DVPn_ITS                 0x9C                /* Interrupt Status - Read Only */
#define GAM_DVPn_ITS_MASK            0x00000018

#define GAM_DVPn_STA                 0xA0                /* Status Register */
#define GAM_DVPn_STA_MASK            0x00000018

#define GAM_DVPn_LNSTA               0xA4                /* Line number status */
#define GAM_DVPn_LNSTA_MASK          0x0000009F

#define GAM_DVPn_HLFLN               0xA8                /* Half lines per vertical field */
#define GAM_DVPn_HLFLN_MASK          0x00000FFF

#define GAM_DVPn_ABA                 0xAC                /* Ancillary data base address */
#define GAM_DVPn_ABA_MASK            0xFFFFFFF0

#define GAM_DVPn_AEA                 0xB0                /* Ancillary data end address */
#define GAM_DVPn_AEA_MASK            0xFFFFFFF0

#define GAM_DVPn_APS                 0xB4                /* Ancillary data page size */
#define GAM_DVPn_APS_MASK            0x00000FF0

#define GAM_DVPn_PKZ                 0xFC                /* Maximum packet size */
#define GAM_DVPn_PKZ_MASK            0x00000007

#ifdef __cplusplus
}
#endif

#endif /* #ifndef __HAL_INP_REG_H */
/* ------------------------------- End of file ---------------------------- */

