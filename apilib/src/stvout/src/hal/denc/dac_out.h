/*******************************************************************************

File name   : dac_out.h

Description : VOUT driver header for DAC inputs

COPYRIGHT (C) STMicroelectronics 2001.

Date               Modification                                     Name
----               ------------                                     ----
14 Sep 2001        Created                                          HSdLM
*******************************************************************************/

#ifndef __DAC_OUT_H
#define __DAC_OUT_H

/* Includes ----------------------------------------------------------------- */

#ifdef __cplusplus
extern "C" {
#endif

/* Private Types ---------------------------------------------------------- */

/* Private Constants ------------------------------------------------------ */

/* Private variables (static) --------------------------------------------- */

/* Global Variables --------------------------------------------------------- */

/* Private Macros --------------------------------------------------------- */

/* Exported Functions ----------------------------------------------------- */

void stvout_HalSetDACSource(const stvout_Device_t * const Device_p);

/* ------------------------------- End of file ---------------------------- */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __DAC_OUT_H */
