/*******************************************************************************
File Name   : fpga.h

Description : Header file of FPGA & EPLD setup

COPYRIGHT (C) STMicroelectronics 2001.

*******************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __FPGA_H
#define __FPGA_H

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

BOOL stboot_FpgaInit(void);
BOOL stboot_EpldInit(void);

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* ifndef __FPGA_H */

/* End of fpga.h */
