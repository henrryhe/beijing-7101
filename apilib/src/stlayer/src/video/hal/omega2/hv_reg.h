/*****************************************************************************

File name   :

Description : VIDEO registers

COPYRIGH
 * T (C) ST-Microelectronics 2000.

Date               Modification                 Name
----               ------------                 ----
15/05/00           Created                      Philippe Legeard

*****************************************************************************/

#ifndef __HAL_LAYER_REG_H
#define __HAL_LAYER_REG_H

/* Includes --------------------------------------------------------------- */
#include "stddefs.h"
#include "stsys.h"

#ifdef __cplusplus
extern "C" {
#endif


/******************************************************************************/
/*- Auxillary display registers ----------------------------------------------*/
/******************************************************************************/

/*- Auxiliary display configuration register --------------------------------*/
#define DIP_XCFG                            0x00
#define DIP_XCFG_MASK                 0x0000000F    /* Mask to access available conf. bits. */
/* source ID (identification or selection).           */
#define DIP_XCFG_SRC_MASK                   0xF
#define DIP_XCFG_SRC_EMP                    0


/* Auxiliary display interface registers */

/* PIP luma rate control */
#define DIP_IF_YRCC                      0x1cc0
#define DIP_IF_YRCC_MASK                 0x000f
/* PIP chroma rate control */
#define DIP_IF_CRCC                      0x1cc4
#define DIP_IF_CRCC_MASK                 0x000f

/*- interrupt registers ------------------------------------------------*/
#define DIP_ITM                            0x98             /* Interrupt Mask     */
#define DIP_ITM_MASK                     0x001F
#define DIP_ITS                            0x9C             /* Interrupt Status   */
#define DIP_ITS_MASK                       0x1F
#define DIP_STA                          0x00A0             /* Status Register    */
#define DIP_STA_MASK                       0x1F

/******************************************************************************/
/*- Main display -------------------------------------------------------------*/
/******************************************************************************/

/*- interrupt registers ------------------------------------------------*/
#define DIS_ITM                             0x98            /* Interrupt Mask     */
#define DIS_ITM_MASK                       0x7FF
#define DIS_ITS                             0x9C            /* Interrupt Status   */
#define DIS_ITS_MASK                       0x7FF
#define DIS_STA                             0xA0            /* Status Register    */
#define DIS_STA_MASK                       0x7FF

/* Mask of each interrupt sources.                                      */
#define DIS_STA_EAW                   0x00000400
#define DIS_STA_YWFF                  0x00000200
#define DIS_STA_YRFE                  0x00000100
#define DIS_STA_CWFF                  0x00000080
#define DIS_STA_CRFE                  0x00000040
#define DIS_STA_YBFE                  0x00000020
#define DIS_STA_YAFE                  0x00000010
#define DIS_STA_CFE                   0x00000008
#define DIS_STA_PXFE                  0x00000004
#define DIS_STA_YIFD                  0x00000002
#define DIS_STA_CIFD                  0x00000001

/*------------------------------------------------------------------------*/

/*- dummy ----------------------------------------------------------------*/
#define DIS_DCMB                            0x4C              /* Dummy Chroma Macroblock Location                    */
#define DIS_DMB                             0x48              /* Dummy Luma Macroblock Location                      */

/*- LMU ------------------------------------------------------------------*/
#define DIS_LMP                             0x50              /* LMU Luma Buffer Start Pointer                       */
#define DIS_LMP_MASK                     0x3FFFF
#define DIS_LMP_EMP                            8

#define DIS_LCMP                            0x54              /* LMU Chroma Buffer Start Pointer                     */
#define DIS_LCMP_MASK                    0x3FFFF
#define DIS_LCMP_EMP                           8

#define DIS_LMU_AFD                       0x1C84              /* Accumulated field difference                        */
#define DIS_LMU_AFD_MASK                  0xFFFF
#define DIS_LMU_AFD_EMP                        0
#define DIS_LMU_AFD_DIFF_C_MASK             0xFF
#define DIS_LMU_AFD_DIFF_C_EMP                 0
#define DIS_LMU_AFD_DIFF_Y_MASK             0xFF
#define DIS_LMU_AFD_DIFF_Y_EMP                 8

#define DIS_LMU_CTRL                      0x1C80              /* Luma and Chroma LMU control                         */
#define DIS_LMU_CTRL_FML_C_EMP                 0              /* film compaison line number bits                     */
#define DIS_LMU_CTRL_FML_Y_EMP                16              /* film compaison line number bits                     */
#define DIS_LMU_CTRL_FML_MASK               0x7F
#define DIS_LMU_CTRL_FMC_C_EMP                 8              /* Film mode control bits                              */
#define DIS_LMU_CTRL_FMC_Y_EMP                24              /* Film mode control bits                              */
#define DIS_LMU_CTRL_FMC_MASK                  3
#define DIS_LMU_CTRL_CK_C_EMP                 10              /* Clock control bits                                  */
#define DIS_LMU_CTRL_CK_Y_EMP                 26              /* Clock control bits                                  */
#define DIS_LMU_CTRL_CK_MASK                   3
#define DIS_LMU_CTRL_EN_C_EMP                 12              /* luma ENABLE bit                                     */
#define DIS_LMU_CTRL_EN_Y_EMP                 28              /* luma ENABLE bit                                     */
#define DIS_LMU_CTRL_EN_MASK                   1
#define DIS_LMU_CTRL_DTI_C_EMP                13              /* temporal interpolation DISABLE bit                  */
#define DIS_LMU_CTRL_DTI_Y_EMP                29              /* temporal interpolation DISABLE bit                  */
#define DIS_LMU_CTRL_DTI_MASK                  1
#define DIS_LMU_CTRL_MC_C_EMP                 14              /* motion comparison mode bit                          */
#define DIS_LMU_CTRL_MC_Y_EMP                 30              /* motion comparison mode bit                          */
#define DIS_LMU_CTRL_MC_MASK                   1

/*- Window size ----------------------------------------------------------*/
#define DIS_MXFH                            0x40    /* Main Display Window Vertical Size    */
#define DIS_MXFH_MASK                       0x7F
#define DIS_MXFW                            0x44    /* Main Display Window Horizontal Size  */
#define DIS_MXFW_MASK                       0xFF

/* Multi-Display Horizontal Offset ---------------------------------------*/
#define DIS_XDOFF1                          0x04    /* Multi-Display hor. offset (picture 1)*/
#define DIS_XDOFF2                          0x14    /* Multi-Display hor. offset (picture 2)*/
#define DIS_XDOFF3                          0x24    /* Multi-Display hor. offset (picture 3)*/
#define DIS_XDOFF4                          0x34    /* Multi-Display hor. offset (picture 4)*/
#define DIS_XDOFF_STEP                      0x10    /* Step between each register.          */

/*- Multi-Display Vertical Offset ----------------------------------------*/
#define DIS_YDOFF1                          0x08    /* Multi-Display ver. offset (picture 1)*/
#define DIS_YDOFF2                          0x18    /* Multi-Display ver. offset (picture 2)*/
#define DIS_YDOFF3                          0x28    /* Multi-Display ver. offset (picture 3)*/
#define DIS_YDOFF4                          0x38    /* Multi-Display ver. offset (picture 4)*/
#define DIS_YDOFF_MASK                      0x7F
#define DIS_YDOFF_STEP                      0x10    /* Step between each register.          */
/*------------------------------------------------------------------------*/

/*- Multi display configuration registers --------------------------------*/
#define DIS_XCFG1                           0x00    /* Viewport 1 (picture 1)               */
#define DIS_XCFG2                           0x10    /* Viewport 2 (picture 2)               */
#define DIS_XCFG3                           0x20    /* Viewport 3 (picture 3)               */
#define DIS_XCFG4                           0x30    /* Viewport 4 (picture 4)               */
#define DIS_XCFG_MASK                 0x0000007F    /* Mask to access available conf. bits. */
#define DIS_XCFG_STEP                       0x10    /* Step between each register.          */
/* enable/disable the use of the picture for display. */
#define DIS_XCFG_ENA_DISA_LAYER_MASK        1
#define DIS_XCFG_ENA_DISA_LAYER_EMP         6
/* display priority (for several overlayed pictures). */
#define DIS_XCFG_PRIORITY_MASK              3
#define DIS_XCFG_PRIORITY_EMP               4
#define DIS_XCFG_PRIORITY_0                 0x00
#define DIS_XCFG_PRIORITY_1                 0x01
#define DIS_XCFG_PRIORITY_2                 0x02
#define DIS_XCFG_PRIORITY_3                 0x03
/* source ID (identification or selection).           */
#define DIS_XCFG_SRC_MASK                   0xF
#define DIS_XCFG_SRC_EMP                    0
/*------------------------------------------------------------------------*/

/*- Block to Line --------------------------------------------------------*/
#define DIS_BTL_BPL                        0x1d00
#define DIS_BTL_BPL_MASK                   0x7F
/*------------------------------------------------------------------------*/

/*- Vertical Block Correction --------------------------------------------*/
#define DIS_VBC_VINP                       0x1d80         /* Display vertical start position */
#define DIS_VBC_VINP_MASK                  0x07FF
/*------------------------------------------------------------------------*/

/*- Display Horizontal Format Control ------------------------------------*/
    /* note : DIX_xyz is DIS_xyz and DIP_xyz */
#define DIX_HFC_LCOEFFS_32                  0x1000        /* HFC Luma coeffs */
#define DIX_HFC_CCOEFFS_32                  0x1400        /* HFC Chroma coeffs */

#define DIX_HFC_YHINL                       0x1600        /* Pixel nbr of VFC luma output line */
#define DIX_HFC_YHINL_MASK                  0x07FF

#define DIX_HFC_CHINL                       0x1604        /* Pixel nbr of VFC chroma output line */
#define DIX_HFC_CHINL_MASK                  0x07FF

#define DIX_HFC_PDELTA1                     0x1608        /* PSTEP delta per output pixel Zone 1 */
#define DIX_HFC_PDELTA2                     0x160c        /* PSTEP delta per output pixel Zone 2 */
#define DIX_HFC_PDELTA4                     0x1610        /* PSTEP delta per output pixel Zone 4 */
#define DIX_HFC_PDELTA5                     0x1614        /* PSTEP delta per output pixel Zone 5 */
#define DIX_HFC_PDELTA_MASK                 0x01ff
#define DIX_HFC_PDELTA_MAX_VALUE            (DIX_HFC_PDELTA_MASK >> 1)

#define DIX_HFC_HZONE1                      0x1618        /* End of Horizontal zone 1 */
#define DIX_HFC_HZONE2                      0x161c        /* End of Horizontal zone 2 */
#define DIX_HFC_HZONE3                      0x1620        /* End of Horizontal zone 3 */
#define DIX_HFC_HZONE4                      0x1624        /* End of Horizontal zone 4 */
#define DIX_HFC_HZONE5                      0x1628        /* End of Horizontal zone 5 */
#define DIX_HFC_HZONE_MASK                  0x07ff

#define DIX_HFC_PSTEP                       0x162c        /* Distance between adjacent output pixels */
#define DIX_HFC_PSTEP_MASK                  0x7FFF
#define DIX_HFC_PSTEP_MAX_VALUE             (DIX_HFC_PSTEP_MASK >> 1)

#define DIX_HFC_PSTEP_INIT                  0x1630        /* Sub-pixel panning from HINP position */
#define DIX_HFC_PSTEP_INIT_MASK             0x3FFF

#define DIX_HFC_THRU                        0x1634        /* Horizontal bypass */
#define DIX_HFC_THRU_MASK                   0x1
/*------------------------------------------------------------------------*/

/*- Vertical Format Control ----------------------------------------------*/
    /* note : DIX_xyz is DIS_xyz and DIP_xyz */
#define DIX_VFC_COEFFS_32                   0x1800        /* HFC Chroma coeffs */
#define DIX_VFC_LDELTA1                     0x1a00        /*  */
#define DIX_VFC_LDELTA2                     0x1a04        /*  */
#define DIX_VFC_LDELTA4                     0x1a08        /*  */
#define DIX_VFC_LDELTA5                     0x1a0c        /*  */
#define DIX_VFC_LDELTA_MASK                 0x01ff

#define DIX_VFC_VZONE1                      0x1a10        /*  */
#define DIX_VFC_VZONE2                      0x1a14        /*  */
#define DIX_VFC_VZONE3                      0x1a18        /*  */
#define DIX_VFC_VZONE4                      0x1a1c        /*  */
#define DIX_VFC_VZONE5                      0x1a20        /*  */
#define DIX_VFC_VZONE_MASK                  0x07FF        /*  */

#define DIX_VFC_LSTEP                       0x1a24        /* Line to line step size. */
#define DIX_VFC_LSTEP_MASK                  0xFFFF

#define DIX_VFC_LSTEP_INIT                  0x1a28        /*  */
#define DIX_VFC_LSTEP_INIT_MASK             0x7FFF        /*  */

#define DIX_VFC_VINL                        0x1a2c        /* Number of input lines */
#define DIX_VFC_VINL_MASK                   0x07FF

#define DIX_VFC_IID                         0x1a38        /* Input image type for the VFC. */
#define DIX_VFC_IID_MASK                    0x3

#define DIX_VFC_THRU                        0x1a3c        /* VFC bypass control */
#define DIX_VFC_THRU_Y_EMP                  0
#define DIX_VFC_THRU_C_EMP                  1
#define DIX_VFC_THRU_MASK                   0x3

#define DIX_VFC_HINP                        0x1a30        /* horizontal start position of output line */
#define DIX_VFC_HINP_MASK                   0x07FF

#define DIX_VFC_HSINL                       0x1a34        /* Horizontal size of the stored image */
#define DIX_VFC_HSINL_MASK                  0x07FF
/*------------------------------------------------------------------------*/

/******************************************************************************/
/*- PSI ----------------------------------------------------------------------*/
/******************************************************************************/

/*- Edge Replacement -----------------------------------------------------*/
#define DIS_ER_CTRL                         0x1b10    /* Edge Replacement Gain Control.             */
#define DIS_ER_CTRL_MASK                    0x3f      /* Edge Replacement Gain Control Mask.        */
#define DIS_ER_CTRL_DISABLE                 0x00
#define DIS_ER_DELAY                        0x1b58    /* Edge Replacement Frequency Control.        */
#define DIS_ER_DELAY_MASK                   0x03      /* Edge Replacement Frequency Control Mask.   */
#define DIS_ER_DELAY_HF                     0x01      /* ER delay for High frequency filter.        */
#define DIS_ER_DELAY_MF                     0x02      /* ER delay for Medium frequency filter.      */
#define DIS_ER_DELAY_LF                     0x03      /* ER delay for Low frequency filter.         */


/*- Main Display Peaking -------------------------------------------------*/
#define DIS_PK_V_CORE                       0x1b00        /* Coring for vertical peaking.           */
#define DIS_PK_V_CORE_MASK                  0x0f          /* Coring for vertical peaking Mask.      */
#define DIS_PK_V_CORE_DISABLE               0x00
#define DIS_PK_V_PEAK                       0x1b04        /* Vertical peaking gain control.         */
#define DIS_PK_V_PEAK_MASK                  0x3f          /* Vertical peaking gain control Mask.    */
#define DIS_PK_V_PEAK_DISABLE               0x10
#define DIS_PK_FILT                         0x1b08        /* Horizontal peaking filter selection.   */
#define DIS_PK_FILT_MASK                    0x07          /* Horizontal peaking filter selection Mask*/
#define DIS_PK_FILT_MIN_VALUE               0x00
#define DIS_PK_H_PEAK                       0x1b0c        /* Horizontal peaking gain control.       */
#define DIS_PK_H_PEAK_MASK                  0x3f          /* Horizontal peaking gain control.       */
#define DIS_PK_H_PEAK_DISABLE               0x10
#define DIS_PK_H_CORE                       0x1b5c        /* Coring for horizontal peaking.         */
#define DIS_PK_H_CORE_MASK                  0x0f          /* Coring for horizontal peaking Mask.    */
#define DIS_PK_H_CORE_DISABLE               0x00
#define DIS_PK_SIEN                         0x1b60        /* Si Compensation.                       */
#define DIS_PK_SIEN_MASK                    0x01          /* Si Compensation Mask.                  */
#define DIS_PK_SIEN_ON                      0x01          /* Si Compensation ON status              */
#define DIS_PK_SIEN_OFF                     0x00          /* Si Compensation OFF status.            */

/*- Main Display Auto-Flesh -----------------------------------------------*/
#define DIS_AF_QWIDTH                       0x1b40        /* Quadrature flesh width control.        */
#define DIS_AF_QWIDTH_MASK                  0x03          /* Quadrature flesh width control Mask.   */
#define DIS_AF_QWIDTH_15_DEGREES            0x00
#define DIS_AF_QWIDTH_11_DEGREES            0x01
#define DIS_AF_QWIDTH_08_DEGREES            0x02
#define DIS_AF_OFF                          0x1b4c        /* Auto flesh on/off.                     */
#define DIS_AF_OFF_MASK                     0x7f          /* Auto flesh on/off Mask.                */
#define DIS_AF_OFF_DISABLE                  0x00
#define DIS_AF_AXIS                         0x1b50        /* Flesh axis control.                    */
#define DIS_AF_AXIS_MASK                    0x03          /* Flesh axis control Mask.               */
#define DIS_AF_AXIS_116_DEGREES             0x00
#define DIS_AF_AXIS_121_DEGREES             0x01
#define DIS_AF_AXIS_125_DEGREES             0x02
#define DIS_AF_AXIS_130_DEGREES             0x03

#define DIS_AF_TINT                         0x1b48        /* Tint control.                          */
#define DIS_AF_TINT_MASK                    0x3f          /* Tint control Mask.                     */
#define DIS_AF_TINT_DISABLE_VALUE           0x0           /* Value that disables Tint filter.       */

/*- Main Display Green-Boost -----------------------------------------------*/
#define DIS_GB_OFF                          0x1b54        /* Green boost on/off.                    */
#define DIS_GB_OFF_MASK                     0x3f          /* Green boost on/off Mask.               */
#define DIS_GB_OFF_DISABLE                  0x00
#define DIS_GB_MIN_VALUE                    -32
#define DIS_GB_MAX_VALUE                    31

/*- Main Display Dynamic Contrast Improvement ------------------------------*/
#define DIS_DCI_DCI_ON                      0x1b68        /* DCI on/off */
#define DIS_DCI_CSC_ON                      0x1b64        /* Color Saturation Compensation on/off */
#define DIS_DCI_FREEZE_ANLY                 0x1b6c        /* Freeze analysis on/off */

#define DIS_DCI_FILTER                      0x1b1c        /* Low-Pass filter selection */
#define DIS_DCI_COR_MOD                     0x1b14        /* Coring mode */
#define DIS_DCI_COR                         0x1b18        /* Coring level */
#define DIS_DCI_COR_MASK                    0x0000003f    /* Coring level Mask */
#define DIS_DCI_COR_DEFAULT                 5

#define DIS_DCI_SPIXEL                      0x1b98        /* Hor pel position of left edge of DCI analysis window  */
#define DIS_DCI_SPIXEL_MASK                 0x000007ff
#define DIS_DCI_EPIXEL                      0x1b88        /* Hor pel position of right edge of DCI analysis window */
#define DIS_DCI_EPIXEL_MASK                 0x000007ff
#define DIS_DCI_SLINE                       0x1b34        /* 1st line of DCI analysis */
#define DIS_DCI_SLINE_MASK                  0x000007ff
#define DIS_DCI_ELINE                       0x1b20        /* Last line of the DCI analysis window */
#define DIS_DCI_ELINE_MASK                  0x000007ff

#define DIS_DCI_LSTHR                       0x1b28        /* Light sample threshold             */
#define DIS_DCI_LSTHR_MASK                  0xff          /* Light sample threshold Mask        */
#define DIS_DCI_LSTHR_DEFAULT               106
#define DIS_DCI_DSTHR                       0x1b30        /* Dark sample threshold              */
#define DIS_DCI_DSTHR_MASK                  0xff          /* Dark sample threshold Mask         */
#define DIS_DCI_DSTHR_DEFAULT               122
#define DIS_DCI_SENSWS                      0x1b84        /* Sensitivity of ABA                 */
#define DIS_DCI_SENSWS_MASK                 0x7fff        /* Sensitivity of ABA Mask            */
#define DIS_DCI_LSWF                        0x1b2c        /* Light sample weighting factor      */
#define DIS_DCI_LSWF_MASK                   0x0f          /* Light sample weighting factor Mask */
#define DIS_DCI_LSWF_DEFAULT                2

#define DIS_DCI_TF_DPP                      0x1b3c        /* Transfer function dynamic pivot point */
#define DIS_DCI_TF_DPP_MASK                 0x3ff         /* Mask for transfer function dynamic pivot point */
#define DIS_DCI_TF_DPP_MIN                  7             /* Min (recommended) for transfer function dynamic pivot point */
#define DIS_DCI_TF_DPP_MAX                  409           /* Max (recommended) for transfer function dynamic pivot point */
#define DIS_DCI_GAIN_SEG1_FRC               0x1b94        /* Frac part of gain for 1st seg of dual seg xfer function */
#define DIS_DCI_GAIN_SEG1_FRC_MASK          0xfff         /* Mask for frac part of gain for 1st seg of xfer function */
#define DIS_DCI_GAIN_SEG2_FRC               0x1b24        /* Frac part of gain for 2nd seg of dual seg xfer function */
#define DIS_DCI_GAIN_SEG2_FRC_MASK          0xfff         /* Mask for frac part of gain for 2nd seg of xfer function */
#define DIS_DCI_GAIN_FRC_MIN                0             /* Min for frac part of gain for 1st seg of xfer function */
#define DIS_DCI_GAIN_FRC_MAX                2864          /* Max for frac part of gain for 1st seg of xfer function */


#define DIS_DCI_SENSBS                      0x1b80        /* Sensitivity of DSDA */
#define DIS_DCI_SENSBS_MASK                 0x3fff        /* Mask for sensitivity of DSDA */
#define DIS_DCI_DLTHR                       0x1b70        /* Dark level threshold for DSDA */
#define DIS_DCI_DLTHR_MASK                  0xff          /* Mask for dark level threshold for DSDA */
#define DIS_DCI_DLTHR_DEFAULT               52
#define DIS_DCI_DYTC                        0x1b74        /* Dark area size for DSDA */
#define DIS_DCI_DYTC_MASK                   0x3f          /* Mask for dark area size for DSDA */
#define DIS_DCI_DYTC_DEFAULT                17
#define DIS_DCI_ERR_COMP                    0x1b90        /* Error compensation */
#define DIS_DCI_ERR_COMP_MASK               0x3fff        /* Mask for DSD error compensation */

#define DIS_DCI_SNR_COR                     0x1b38        /* Signal to noise ratio estimation for coring */
#define DIS_DCI_SAT                         0x1b44        /* Saturation control */
#define DIS_SAT_DISABLE_VALUE               0x00          /* Saturation control disable value */
#define DIS_DCI_SAT_DISABLE_VALUE           0x20
#define DIS_DCI_SAT_MASK                    0x3f          /* Saturation control Mask.*/
#define DIS_DCI_PEAK_SIZE                   0x1b78        /* Peak area size */
#define DIS_DCI_PEAK_SIZE_MASK              0xf           /* Peak area size Mask */
#define DIS_DCI_PEAK_SIZE_DEFAULT           7
#define DIS_DCI_PEAK_THR0                   0x1b9c        /* Range 0 upper limit for peak detection */
#define DIS_DCI_PEAK_THR1                   0x1ba0        /* Range 1 upper limit for peak detection */
#define DIS_DCI_PEAK_THR2                   0x1ba4        /* Range 2 upper limit for peak detection */
#define DIS_DCI_PEAK_THR3                   0x1ba8        /* Range 3 upper limit for peak detection */
#define DIS_DCI_PEAK_THR4                   0x1bac        /* Range 4 upper limit for peak detection */
#define DIS_DCI_PEAK_THR5                   0x1bb0        /* Range 5 upper limit for peak detection */
#define DIS_DCI_PEAK_THR6                   0x1bb4        /* Range 6 upper limit for peak detection */
#define DIS_DCI_PEAK_THR7                   0x1bb8        /* Range 7 upper limit for peak detection */
#define DIS_DCI_PEAK_THR8                   0x1bbc        /* Range 8 upper limit for peak detection */
#define DIS_DCI_PEAK_THR9                   0x1bc0        /* Range 9 upper limit for peak detection */
#define DIS_DCI_PEAK_THR10                  0x1bc4        /* Range 10 upper limit for peak detection */
#define DIS_DCI_PEAK_THR11                  0x1bc8        /* Range 11 upper limit for peak detection */
#define DIS_DCI_PEAK_THR12                  0x1bcc        /* Range 12 upper limit for peak detection */
#define DIS_DCI_PEAK_THR13                  0x1bd0        /* Range 13 upper limit for peak detection */
#define DIS_DCI_PEAK_THR14                  0x1bd4        /* Range 14 upper limit for peak detection */
#define DIS_DCI_PEAK_THR15                  0x1bd8        /* Range 15 upper limit for peak detection */
#define DIS_DCI_CBCR_PEAK                   0x1be0        /* Output of chroma peak detector */
#define DIS_DCI_CBCR_PEAK_MASK              0x7ff         /* Mask for output of chroma peak detector */
#define DIS_DCI_DRKS_DIST                   0x1be4        /* Output of dark sample distribution */
#define DIS_DCI_DRKS_DIST_MASK              0xfffff       /* Mask for output of dark sample distribution */
#define DIS_DCI_AVRG_BR                     0x1be8        /* Output of the average brightness analysis */
#define DIS_DCI_AVRG_BR_MASK                0x3ffff       /* Mask for average picture brightness */
#define DIS_DCI_PEAK_N                      0x1bec        /* Upper limit control for peak detection algorithm */
#define DIS_DCI_PEAK_N_MASK                 0x0f          /* Mask for upper limit control for peak detection algorithm */
#define DIS_DCI_FR_PEAK0                    0x1bf0        /* Counter output for peak detection in range 0 */
#define DIS_DCI_FR_PEAK1                    0x1bf4        /* Counter output for peak detection in range 1 */
#define DIS_DCI_FR_PEAK2                    0x1bf8        /* Counter output for peak detection in range 2 */
#define DIS_DCI_FR_PEAK3                    0x1bfc        /* Counter output for peak detection in range 3 */
#define DIS_DCI_FR_PEAK4                    0x1c00        /* Counter output for peak detection in range 4 */
#define DIS_DCI_FR_PEAK5                    0x1c04        /* Counter output for peak detection in range 5 */
#define DIS_DCI_FR_PEAK6                    0x1c08        /* Counter output for peak detection in range 6 */
#define DIS_DCI_FR_PEAK7                    0x1c0c        /* Counter output for peak detection in range 7 */
#define DIS_DCI_FR_PEAK8                    0x1c10        /* Counter output for peak detection in range 8 */
#define DIS_DCI_FR_PEAK9                    0x1c14        /* Counter output for peak detection in range 9 */
#define DIS_DCI_FR_PEAK10                   0x1c18        /* Counter output for peak detection in range 10 */
#define DIS_DCI_FR_PEAK11                   0x1c1c        /* Counter output for peak detection in range 11 */
#define DIS_DCI_FR_PEAK12                   0x1c20        /* Counter output for peak detection in range 12 */
#define DIS_DCI_FR_PEAK13                   0x1c24        /* Counter output for peak detection in range 13 */
#define DIS_DCI_FR_PEAK14                   0x1c28        /* Counter output for peak detection in range 14 */
#define DIS_DCI_FR_PEAK15                   0x1c2c        /* Counter output for peak detection in range 15 */
#define DIS_DCI_FR_PEAK_MASK                0x7fff        /* Mask for counter output for peak detection */
#define DIS_DCI_BYPASS                      0x1c30        /* Enable/Disable bypass of PSI process   */
#define DIS_DCI_BYPASS_ENABLE_VALUE         0x01          /* Enable bypass of PSI process           */
#define DIS_DCI_BYPASS_DISABLE_VALUE        0x00          /* Disable bypass of PSI process          */
#define DIS_DCI_BYPASS_MASK                 0x01          /* Mask for bypass of PSI process         */

/*- Aux Display Tint & Saturation ----------------------------------------*/
#define DIP_TS_TINT                         0x1B00        /* tint Control Register.                     */
#define DIP_TS_TINT_MASK                    0x3f          /* tint Control Register Mask.                */
#define DIP_TS_TINT_DISABLE_VALUE           0x00          /* tint Control Register (disable value).     */
#define DIP_TS_SAT                          0x1B04        /* Saturation Control Register.               */
#define DIP_TS_SAT_MASK                     0x3f          /* Saturation Control Register Mask.          */
#define DIP_TS_SAT_DISABLE_VALUE            0x20          /* Saturation Control Register (disable value)*/
#define DIP_PK_SET_OUT                      0x1B08
#define DIP_TS_BYPASS                       0x1B0C
#define DIP_TS_BYPASS_MASK                    0x01

/******************************************************************************/
/*- Video Gamma compositor registers -----------------------------------------*/
/******************************************************************************/
/* VIn_CTL : control */
#define VIn_CTL                             0x00
#define VIn_CTL_MASK                        0x043f7000

/* VIn_ALP : global alpha value */
#define VIn_ALP                             0x04
#define VIn_ALP_MASK                        0x000000ff
#define VIn_ALP_FULL_TRANSPARANCY           0x00
#define VIn_ALP_DEFAULT                     0x80

/* VIn_KEY1 : lower limit of the color range */
#define VIn_KEY1                            0x28
#define VIn_KEY1_MASK                       0x00ffffff

/* VIn_KEY2 : upper limit of the color range */
#define VIn_KEY2                            0x2C
#define VIn_KEY2_MASK                       0x00ffffff

/* VIn_VPO : Viewport Offset */
#define VIn_VPO                             0x0C
#define VIn_VPO_MASK                        0x07ff0fff

/* VIn_VPS : Viewport stop */
#define VIn_VPS                             0x10
#define VIn_VPS_MASK                        0x07ff0fff

/* VIn_MPR matrix programming regiters */
#define VIn_MPR1                            0x30
#define VIn_MPR1_MASK                       0x0fff007f
#define VIn_MPR2                            0x34
#define VIn_MPR2_MASK                       0x0fff0fff
#define VIn_MPR3                            0x38
#define VIn_MPR3_MASK                       0x0fff0fff
#define VIn_MPR4                            0x3C
#define VIn_MPR4_MASK                       0x0fff0fff

#ifdef __cplusplus
}
#endif

#endif /* #ifndef __VI_HAL_DIS_REG_H */










