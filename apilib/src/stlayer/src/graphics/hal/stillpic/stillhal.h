/*******************************************************************************

File name   :

Description : definitions used in layer context.

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                               Name
----               ------------                               ----
2000-05-30         Created                                    Michel Bruant
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __STILLHAL_H
#define __STILLHAL_H

/* Includes ----------------------------------------------------------------- */

#include "stsys.h"
#include "still_cm.h"

/* Exported Macros and Defines -------------------------------------------- */

#define Address (U32)stlayer_still_context.BaseAddress
#define ststill_ReadReg8(a)  STSYS_ReadRegDev8((void*)(Address + (a)))
#define ststill_ReadReg16(a) STSYS_ReadRegDev16BE((void*)(Address + (a)))
#define ststill_WriteReg8(a,v) STSYS_WriteRegDev8((void*)(Address + (a)),v)
#define ststill_WriteReg16(a,v) STSYS_WriteRegDev16BE((void*)(Address + (a)),v)
#define ststill_WriteReg24(a,v) \
    {                                                                 \
        *(((STSYS_DU8 *) (Address + a))    ) = (U8) ((v) >> 16);      \
        *(((STSYS_DU8 *) (Address + a)) + 1) = (U8) ((v) >> 8 );      \
        *(((STSYS_DU8 *) (Address + a)) + 2) = (U8) ((v)      );      \
    }



/* Exported Constants ----------------------------------------------------- */

/* Registers Map */
/*---------------*/
#define TDL_TDW         0x86
#define BCK_Y           0x98
#define BCK_U           0x99
#define BCK_V           0x9A
#define VID_MWS         0x9C
#define TDL_LSR0        0xEB
#define TDL_LSR1        0xED
#define TDL_DCF         0xF4
#define TDL_SCN16       0xB0
#define TDL_YDS16       0xC6
#define TDL_SWT16       0xC8
#define TDL_YDO16       0xEE
#define TDL_XDO16       0xF0
#define TDL_XDS16       0xF2
# if defined HW_5510 || defined HW_5512
#define TDL_TEP24       0x83  /* even = bottom */
#define TDL_TOP24       0x80  /* odd  = top    */
#else
# if defined HW_5514 || defined HW_5516 || defined HW_5517
#define TDL_TEP24       0xE6  /* even = bottom */
#define TDL_TOP24       0xE3  /* odd  = top    */
#else
#error None chip defined. Use DVD_BACKEND
#endif
#endif

/* Registers fields mask and pos */
/* ----------------------------- */
#define TDL_LSR0_MASK       0xFF
#define TDL_LSR1_MASK       0x01
#define TDL_DCF_MASK        0x1F
#define TDL_YDS16_MASK      0x01FF
#define TDL_SWT16_MASK      0x01FF
#define TDL_YDO16_MASK      0x01FF
#define TDL_XDO16_MASK      0x03FF
#define TDL_XDS16_MASK      0x03FF
#if defined HW_5514 || defined HW_5516 || defined HW_5517
#define TDL_TODDP24_MASK    0x07FFFF
#define TDL_TEVENP24_MASK   0x07FFFF
#else
#define TDL_TODDP24_MASK    0x01FFFF
#define TDL_TEVENP24_MASK   0x01FFFF
#endif
#define TDL_SCN_PAN_POS     9

/* TDL_DCF - Display configuration register ( 5 bits ) --------------- */
#define TDL_DCF_TEVD   (1<<0)    /* Enable Third Video Display           */
#define TDL_DCF_DSR    (1<<1)    /* Disable Sample Rate converter (SRC)  */
#define TDL_DCF_UDS    (1<<2)    /* enable Up / Down Scaling             */
#define TDL_DCF_UND    (1<<3)    /* Up not Down vert. scaling            */
#define TDL_DCF_FMT    (1<<4)    /* Format = Format B                    */

/* functions ---------------------------------------------------------------- */

void stlayer_StillHardUpdate(void);
void stlayer_StillHardInit(void);
void stlayer_StillHardTerm(void);

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif


/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef  __STILLHAL_H */

