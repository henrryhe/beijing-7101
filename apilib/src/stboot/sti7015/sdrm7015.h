/*******************************************************************************
File Name   : sdrm7015.h

Description : Header file of SDRAM setup

COPYRIGHT (C) STMicroelectronics 1999.

Date               Modification                             Name
----               ------------                             ----
15 Sept 99          Created                                  HG
*******************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __BOOTSDRAM7015_H
#define __BOOTSDRAM7015_H

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Includes --------------------------------------------------------------- */

/* Exported Constants ----------------------------------------------------- */

/* Exported Variables ----------------------------------------------------- */

/* Exported Types --------------------------------------------------------- */

/* Exported Macros -------------------------------------------------------- */

/* Exported Functions ----------------------------------------------------- */

BOOL stboot_InitSDRAM7015(U32 Frequency);

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* ifndef __BOOTSDRAM7015_H */

/* End of sdrm7015.h */
