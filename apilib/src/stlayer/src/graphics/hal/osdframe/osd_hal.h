/*****************************************************************************

File name   : osd_hal.h

Description : 

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                 Name
----               ------------                                 ----
2001-12-20          Creation                                    Michel Bruant


*****************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __OSDHAL_H
#define __OSDHAL_H

/* Includes --------------------------------------------------------------- */

#include "stddefs.h"
#include "stavmem.h"
#include "stevt.h"


#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ----------------------------------------------------- */

/* OSD Configuration register stuff */
#define OSD_CFG             0x91 /* Register address */
#define OSD_CFG_RGB16       0x80 /* Enable the rbg16 mode + double header */
#define OSD_CFG_REC         0x40 /* Mix the OSD plan in the 4:2:2 output */
#define OSD_CFG_POL         0x20 /* invert Enot0 signal */                    
#define OSD_CFG_LLG         0x10 /* double granularity for a 64Mbits space. */
#define OSD_CFG_DBH         0x08 /* double header.*/
#define OSD_CFG_ENA         0x04 /* Enable OSD plane display mask*/
#define OSD_CFG_NOR         0x02 /* Normal display mode mask*/
#define OSD_CFG_FIL         0x01 /* OSD filter mode anti flutter/flicker bit*/

/* Active Signal Configuration register stuff */
#define OSD_ACT             0x3E  /* OSD Active Signal Regiter address */
#define OSD_ACT_OAM         0x40  /* Active signal mode mask*/
#define OSD_ACT_OAD         0x3F  /* Active signal delay mask [5:0] */

/* Boundary Weight Regiter */
#define OSD_BDW             0x92  /* OSD Boundary Weight Regiter address */

/* Pointer Regiters */ 
#define OSD_OTP             0x2A  /* OSD Top Field Pointer Regiter address */
#define OSD_OBP             0x2B  /* OSD Bottom Field Pointer Regiter address */

/* Video DCF Register stuff (seen by the OSD driver) */
#define VID_DCF             0x75   /* Regiter address */
#define VID_DCF_OSDPOL      0x04   /* Pol bit mask */

/* Video DCF Register stuff (seen by the OSD driver) */
#define VID_OUT             0x90   /* Register address */
#define VID_OUT_NOS         0x08    /* Not OSD bit */


/* Exported Types --------------------------------------------------------- */

typedef struct
{
    U32 word1;
    U32 word2;
    U32 word3;
    U32 word4;
}osd_RegionHeader_t;

/* Exported funcions ------------------------------------------------------ */

void            stlayer_HardUpdateViewportList(BOOL VSyncIsTop);
void            stlayer_BuildHardImage(void);
ST_ErrorCode_t  stlayer_HardEnableDisplay (void);
ST_ErrorCode_t  stlayer_HardDisableDisplay (void);
void            stlayer_HardInit(void);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef __OSDHAL_H */

/* End of osd_hal.h */
