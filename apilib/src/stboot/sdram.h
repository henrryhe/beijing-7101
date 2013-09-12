/*******************************************************************************
File Name   : sdram.h

Description : Header file of SDRAM setup

COPYRIGHT (C) STMicroelectronics 1999.

Date               Modification                             Name
----               ------------                             ----
15 Sept 99          Created                                  HG
*******************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __BOOTSDRAM_H
#define __BOOTSDRAM_H

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

BOOL stboot_InitSDRAM(U32 Frequency, STBOOT_DramMemorySize_t MemorySize);



/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* ifndef __BOOTSDRAM_H */

/* End of sdram.h */
