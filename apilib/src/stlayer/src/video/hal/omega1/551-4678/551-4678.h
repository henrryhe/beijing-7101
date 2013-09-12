/******************************************************************************

File name   : 551-4678.h

Description : VIDEO registers specific STi5514 STi5516 STi5517 STi5578 STi5508 STi5518

COPYRIGHT (C) ST-Microelectronics 2002.

Date               Modification                 Name
----               ------------                 ----
06/12/01           Created                     Hervi GOUJON
14 May 2003        Merged STi5508/18 file into this one as 99% same HG
******************************************************************************/

#ifndef __HAL_LAYER_REG_1416_H
#define __HAL_LAYER_REG_1416_H

/* Includes ---------------------------------------------------------------- */
#include "stddefs.h"
#include "stsys.h"

#ifdef __cplusplus
extern "C" {
#endif


/*- Multi display registers ----------------------------------------------*/
/* Enable display field */
#define VID_DCF_FRM_MASK    1
#define VID_DCF_FRM_EMP     8
/* E-not-O polarity */
#define VID_DCF_ENPOL_MASK  1
#define VID_DCF_ENPOL_EMP   9
/* select field polarity to freeze on 5508 */
#define VID_DCF_FPO_MASK    1
#define VID_DCF_FPO_EMP     13

/*- Vertical filter for STi5514/16/17 ----------------------------------------*/
#define VID_YMODE4    0xDA  /* Configure luminance of the block-row */
#define VID_YMODE3    0xDB  /* Configure luminance of the block-row */
#define VID_YMODE2    0xDC  /* Configure luminance of the block-row */
#define VID_YMODE1    0xDD  /* Configure luminance of the block-row */
#define VID_YMODE0    0xDE  /* Configure luminance of the block-row */
#define VID_CMODE4    0xFA  /* Configure chrominance of the block-row */
#define VID_CMODE3    0xFB  /* Configure chrominance of the block-row */
#define VID_CMODE2    0xFC  /* Configure chrominance of the block-row */
#define VID_CMODE1    0xFD  /* Configure chrominance of the block-row */
#define VID_CMODE0    0xFE  /* Configure chrominance of the block-row */

/* Masks depending on chip */
#define VID_YDO_MASK     0x3FF
#define VID_YDS_MASK     0x3FF

/*- Vertical filter for STi5508/18 -------------------------------------------*/
#define VID_VFCMODE4    0xEA  /* Configure chrominance of the block-row */
#define VID_VFCMODE3    0xEB  /* Configure chrominance of the block-row */
#define VID_VFCMODE2    0xEC  /* Configure chrominance of the block-row */
#define VID_VFCMODE1    0xED  /* Configure chrominance of the block-row */
#define VID_VFCMODE0    0xEE  /* Configure chrominance of the block-row */
#define VID_VFLMODE4    0xEF  /* Configure luminance of the block-row */
#define VID_VFLMODE3    0xF0  /* Configure luminance of the block-row */
#define VID_VFLMODE2    0xF1  /* Configure luminance of the block-row */
#define VID_VFLMODE1    0xF2  /* Configure luminance of the block-row */
#define VID_VFLMODE0    0xF3  /* Configure luminance of the block-row */

#define VID_YDO_MASK_0818   0x1FF
#define VID_YDS_MASK_0818   0x1FF

/* Patch register to allow a correct 656 mode             */
#define VID_656B        0x45

/*- Horizontal Offset for 656 ------------------------------------------------*/
#define VID_XDO_656_16  0xB5  /* Video X axis Start [9:0] */
#define VID_XDS_656_16  0xF5  /* Video X axis Stop  [9:0] */
#define VID_XDO_656_MASK   0x3FF
#define VID_XDS_656_MASK   0x3FF

/*-Vertical Offset for 656 ---------------------------------------------------*/
#define VID_YDO_656_16  0xb8  /* Video Y axis Start [9:0] */
#define VID_YDO0_656_8  0xb9  /* Video Y axis Start [7:0] */
#define VID_YDO1_656_8  0xb8  /* Video Y axis Start [9:8] */
#define VID_YDS_656_16  0xf8  /* Video Display Stop [9:0] */
#define VID_YDO0_656_MASK   0xFF
#define VID_YDO_656_MASK    0x3FF
#define VID_YDS_656_MASK    0x3FF


#ifdef __cplusplus
}
#endif

#endif /* __HAL_LAYER_REG_1416_H */

/* end of file 551-4678.h */
