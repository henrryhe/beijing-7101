/*****************************************************************************

File name   : Extvi_reg.h

Description : External converters dir digital input register addresses

COPYRIGHT (C) ST-Microelectronics 1999, 2000.

Date               Modification                 Name
----               ------------                 ----
04-December-00     Created                      BD

TODO:
=====

*****************************************************************************/

#ifndef __EXTVI_REG_H
#define __EXTVI_REG_H

/* Includes --------------------------------------------------------------- */

#ifdef __cplusplus
extern "C" {
#endif

/* Exported Types --------------------------------------------------------- */
/* EXTVIN - SAA7114 - standard 8 bit calls     --------_----- */
#define SAA7114_CHIP_VERSION     0x00  /* Chip Version  [4:0]   */

/* Video decoder :0x1...0x2F */
#define SAA7114_FRONT_END_PART    0x01  /* Front-end part */
#define SAA7114_DECODER_PART      0x06  /* Decoder part */
#define SAA7114_SYNC_CONTROL      0x08  /* Sync Control */
#define SAA7114_CHROMINANCE_CONTROL_1 0x0E  /* Decoder part */
#define SAA7114_VID_STATUS        0x1F  /* Video decoder status byte */

/* X-port, I-port and scaler */
#define SAA7114_TASK_IND_GLOB_SET 0x80  /* Task independant global settings */
#define SAA7114_X_PORT_IO_EN      0x83  /* X-port i/O enable and output clock phase control */
#define SAA7114_POW_SAV_CONT      0x88  /* Power save control */
#define SAA7114_TASK_A_DEFINITION 0x90  /* Task A definition */
#define SAA7114_TASK_B_DEFINITION 0xC0  /* Task B definition */



/* EXTVIN - SAA7111 - standard 8 bit calls     ------------- */
/* Register Name                       Address       Description          type bits */
#define SAA7111_CHIP_VERSION             0x00  /* Chip Name and version    RW   8  */
#define SAA7111_ANALOG_INPUT_CONTR_1     0x02  /* Analog Input Control 1   RW   8  */
#define SAA7111_ANALOG_INPUT_CONTR_2     0x03  /* Analog Input Control 2   RW   7  */
#define SAA7111_ANALOG_INPUT_CONTR_3     0x04  /* Channel 1 gain control   RW   8  */
#define SAA7111_ANALOG_INPUT_CONTR_4     0x05  /* Channel 1 gain control   RW   8  */
#define SAA7111_HORIZONTAL_SYNC_START    0x06  /* Horizontal Sync begin    RW   8  */
#define SAA7111_HORIZONTAL_SYNC_STOP     0x07  /* Horizontal Sync Stop     RW   8  */
#define SAA7111_SYNC_CONTROL             0x08  /* Sync. Control            RW   7  */
#define SAA7111_LUMINANCE_CONTROL        0x09  /* Luminace control         RW   8  */
#define SAA7111_LUMINANCE_BRIGHTNESS     0x0A  /* Luma brigthness control  RW   8  */
#define SAA7111_LUMINANCE_CONTRAST       0x0B  /* Luma Contrast control    RW   8  */
#define SAA7111_CHROMA_SATURATION        0x0C  /* Chroma Saturation ctrl   RW   8  */
#define SAA7111_CHROMA_HUE_CONTROL       0x0D  /* Chroma Hue control       RW   8  */
#define SAA7111_CHROMA_CONTROL           0x0E  /* Chroma control           RW   8  */
#define SAA7111_FORMAT_DELAY_CONTROL     0x10  /* Output format control    RW   8  */
#define SAA7111_OUTPUT_CONTROL_1         0x11  /* Output Control 1         RW   8  */
#define SAA7111_OUTPUT_CONTROL_2         0x12  /* Output Control 2         RW   8  */
#define SAA7111_OUTPUT_CONTROL_3         0x13  /* Output Control 3         RW   8  */
#define SAA7111_VGATE1_START             0x15  /* Vertical gate start      RW   8  */
#define SAA7111_VGATE1_STOP              0x16  /* Vertical gate stop       RW   8  */
#define SAA7111_VGATE1_MSB               0x17  /* Vertical gate MSB        RW   2  */
#define SAA7111_TEXT_SLICER_STATUS       0x1A  /* Line 21 text Slicer Stat R    4  */
#define SAA7111_TEXT_SLICER_BYTE_1       0x1B  /* Line 21 Data Byte 1      R    8  */
#define SAA7111_TEXT_SLICER_BYTE_2       0x1C  /* Line 21 Data Byte 2      R    8  */
#define SAA7111_STATUS_BYTE              0x1F  /* Chip Status              R    8  */

/* SAA7111 Register bits   */

/* VIP_CHIP_VERSION */
#define SAA7111_CHIP_VER_NUMBER   0xF0  /* Chip Verion number  */
#define SAA7111_CHIP_NAME         0x0F  /* Chip Name Number    */

/* SAA7111_ANALOG_INPUT_CONTR_1  */
#define SAA7111_AN_FUNC           0xC0  /* Analog Function Select */
#define SAA7111_ANF_BYP           0x00  /* Bypass */
#define SAA7111_ANF_AMP           0x80  /* Amplifier active */
#define SAA7111_ANF_FILT          0xC0  /* Amplifier+Filter Active */
#define SAA7111_GAIN_HYST         0x38  /* Update Hysteresis for gain  */
#define SAA7111_INPUT_SELECT      0x07  /* Input Mode */
#define SAA7111_COMP_11           0x00  /* CVBS Input AI11 */
#define SAA7111_COMP_12           0x01  /* CVBS Input AI12 */
#define SAA7111_COMP_21           0x02  /* CVBS Input AI21 */
#define SAA7111_COMP_22           0x03  /* CVBS Input AI22 */
#define SAA7111_YC_1F             0x04  /* YC Input AI11/AI12 Fixed Gain  */
#define SAA7111_YC_2F             0x05  /* YC Input AI21/AI22 Fixed Gain  */
#define SAA7111_YC_1A             0x06  /* YC Input AI11/AI12 Adapted Gain  */
#define SAA7111_YC_2A             0x07  /* YC Input AI21/AI22 Adapted Gain  */

/* SAA7111_ANALOG_INPUT_CONTR_2 */
#define SAA7111_HLNRS             0x40  /* HL not ref. select */
#define SAA7111_VBSL              0x20  /* Vertical blanking length */
#define SAA7111_WPOFF             0x10  /* White Peak Off  */
#define SAA7111_HOLDG             0x08  /* AGC Freeze  */
#define SAA7111_GAFIX             0x04  /* Fixed gain control */
#define SAA7111_GAI28             0x02  /* Channel 2 Gain control sign bit */
#define SAA7111_GAI18             0x01  /* Channel 1 Gain control sign bit */

/* SAA7111_SYNC_CONTROL */
#define SAA7111_AUFD              0x80  /* Automatic Standard detection (625/525) */
#define SAA7111_FSEL              0x40  /* Standard selection (1=525 0=625) */
#define SAA7111_EXFIL             0x20  /* Loop filter word width */
#define SAA7111_VTRC              0x08  /* VTR Mode select */
#define SAA7111_HPLL              0x04  /* Open PLL select */
#define SAA7111_VNOI              0x03  /* Vertical noise reduction */
#define SAA7111_VNOI_NORM         0x00  /* VNOI Normal mode */
#define SAA7111_VNOI_SEARCH       0x01  /* VNOI Searching mode */
#define SAA7111_VNOI_FRUN         0x02  /* VNOI Free running mode */
#define SAA7111_VNOI_BYP          0x03  /* VNOI Bypassed */

/* SAA7111_LUMINANCE_CONTROL */
#define SAA7111_BYPS              0x80  /* Chroma trap bypass  */
#define SAA7111_PREF              0x40  /* Prefilter enable */
#define SAA7111_BPSS              0x30  /* Aperture band pass */
#define SAA7111_BPSS_41           0x00  /* 4.1 MHz center frequency */
#define SAA7111_BPSS_38           0x10  /* 3.8 MHz center frequency */
#define SAA7111_BPSS_26           0x20  /* 2.6 MHz center frequency */
#define SAA7111_BPSS_29           0x30  /* 2.9 MHz center frequency */
#define SAA7111_VBLB              0x08  /* Luminance Bypass during Vert. Blank  */
#define SAA7111_UPTCV             0x04  /* AGC Update 0=Horiz., 1=Vert.  */
#define SAA7111_APER              0x03  /* Aperture factor */
#define SAA7111_APER_0            0x00  /* Aperture factor 0  */
#define SAA7111_APER_25           0x01  /* Aperture factor 0.25 */
#define SAA7111_APER_50           0x02  /* Aperture factor 0.50  */
#define SAA7111_APER_100          0x03  /* Aperture factor 1  */

/* SAA7111_CHROMA_CONTROL */
#define SAA7111_CDTO              0x80  /* Clear DTO */
#define SAA7111_CSTD              0x70  /* Color standard Selection */
#define SAA7111_PAL_NTSC          0x00  /* Automatic switching PAL/NTSC */
#define SAA7111_PAL4_NTSC4        0x10  /* Automatic switching PAL4.43/NTSC4.43 */
#define SAA7111_PALN_NTSC4        0x20  /* Automatic switching PAL N/NTSC4.43 60Hz */
#define SAA7111_PALM_NTSCN        0x30  /* Automatic switching PAL M/NTSC N */
#define SAA7111_PAL4_SECAM        0x50  /* Automatic switching PAL4.43 and SECAM */
#define SAA7111_DCCF              0x08  /* Disable Chroma Comb Filter */
#define SAA7111_FCTC              0x04  /* Fast Color time constant */
#define SAA7111_CHBW              0x03  /* Chroma Banwidth */
#define SAA7111_CHBW_SMALL        0x00  /* Small bandwidth */
#define SAA7111_CHBW_NOMIN        0x01  /* Nominal bandwidth */
#define SAA7111_CHBW_MED          0x02  /* Medium bandwidth  */
#define SAA7111_CHBW_WIDE         0x03  /* Wide bandwidth */

/* SAA7111_FORMAT_DELAY_CONTROL */
#define SAA7111_OFORM             0xC0  /* Output Format */
#define SAA7111_OF_RGB            0x00  /* RGB Output */
#define SAA7111_OF_YUV422         0x40  /* YUV 422 16 bits */
#define SAA7111_OF_YUV411         0x80  /* YUV 411 12 bits */
#define SAA7111_OF_CCIR656        0xC0  /* YUV CCIR656 8 bits */
#define SAA7111_HDEL              0x30  /* HS fine position */
#define SAA7111_VREF_LONG         0x08  /* VREF output timing */
#define SAA7111_YDELAY            0x07  /* Luma delay compensation */

/* OUTPUT_CONTROL_1 */
#define SAA7111_GPWS              0x80  /* General purpose switch */
#define SAA7111_CM99              0x40  /* Compatiblity with SAA7199 */
#define SAA7111_FECO              0x20  /* FEI sampling control */
#define SAA7111_COMPO             0x10  /* 0=VREF Vert Ref, 1=VREF inv. comp. blank */
#define SAA7111_OEYC              0x08  /* VPO bus output enable */
#define SAA7111_OEHV              0x04  /* Sync signals output enable */
#define SAA7111_SAA7111B              0x02  /* Decoder Bypass */
#define SAA7111_COLO              0x01  /* Color Forced On */

/* OUTPUT_CONTROL_2 */
#define SAA7111_RTSE1             0x80  /* Select output to pin RTS1 */
#define SAA7111_RTSE0             0x40  /* Select output to pin RTS0 */
#define SAA7111_TCLO              0x20  /* 3 state control VPO[0..7] */
#define SAA7111_CBR               0x10  /* Chroma interpolation filter function */
#define SAA7111_RGB888            0x08  /* Select RGB888 output */
#define SAA7111_DIT               0x04  /* Enable dithering */
#define SAA7111_AOSL              0x03  /* Analog Test Select (AOUT pin selection) */
#define SAA7111_AOSL_TP1          0x00  /* internal test point 1 */
#define SAA7111_AOSL_AD1          0x01  /* AD converter 1 input  */
#define SAA7111_AOSL_AD2          0x02  /* AD converter 2 input  */
#define SAA7111_AOSL_TP2          0x03  /* internal test point 2 */

/* SAA7111_TEXT_SLICER_STATUS */
#define SAA7111_F2VAL             0x08  /* Line 21 Field 2 carries Data */
#define SAA7111_F2RDY             0x04  /* Line 21 Field 2 Data Ready */
#define SAA7111_F1VAL             0x02  /* Line 21 Field 1 carries Data */
#define SAA7111_F1RDY             0x01  /* Line 21 Field 1 Data Ready */

/* SAA7111_STATUS_BYTE */
#define SAA7111_STTC              0x80  /* Horiz phase loop status */
#define SAA7111_HLCK              0x40  /* 0=Horiz. freq locked */
#define SAA7111_FIDT              0x20  /* detected field freq. 0=50Hz 1=60Hz */
#define SAA7111_GLIMT             0x10  /* Gain for luminance overflow  */
#define SAA7111_GLIMB             0x08  /* Gain for luminance undedflow  */
#define SAA7111_WIPA              0x04  /* White peak loop active */
#define SAA7111_SLTCA             0x02  /* slow time constant in WIPA mode */
#define SAA7111_CODE              0x01  /* Color signal detected for selected standard */


/* Exported Constants ----------------------------------------------------- */
/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __EXTVI_REG_H */

/* End of extvi_pr.h */

/* ------------------------------- End of file ---------------------------- */
