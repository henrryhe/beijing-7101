/*******************************************************************************

File name   : digital.h

Description : VOUT driver module : digital outputs.

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
25 Jul 2000        Created                                          JG
29 Oct 2001        Disable digital output when not used because of  HSdLM
 *                 noise set on 7015 HD DACS.
06 Dec 2001        New function to switch mode CCIR601/656 for      HSdLM
 *                 Digital Output (added for STi5514)
21 Aou 2003        Split files/directories for better understanding HSdLM
*******************************************************************************/

#ifndef __STVOUT_DIGITAL_H
#define __STVOUT_DIGITAL_H

#ifdef __cplusplus
extern "C" {
#endif

ST_ErrorCode_t stvout_HalEnableDigital( stvout_Device_t * Device_p);
ST_ErrorCode_t stvout_HalDisableDigital( stvout_Device_t * Device_p);
ST_ErrorCode_t stvout_HalSetDigitalRgbYcbcr( void* BaseAddress, U8 RgbYcbcr);
ST_ErrorCode_t stvout_HalSetDigitalEmbeddedSyncHd( void* BaseAddress, BOOL EmbeddedSync);
ST_ErrorCode_t stvout_HalSetDigitalEmbeddedSyncSd( void* BaseAddress, BOOL EmbeddedSync);
ST_ErrorCode_t stvout_HalSetDigitalMainAuxiliary( void* BaseAddress, U8 RgbYcbcr);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef __STVOUT_DIGITAL_H */

/* ------------------------------- End of file ---------------------------- */
