/*******************************************************************************

File name   : sd_dig.h

Description : VOUT driver module : SD digital outputs.

COPYRIGHT (C) STMicroelectronics 2003.

Date               Modification                                     Name
----               ------------                                     ----
21 Aug 2003        Extracted from old 'hdout/digital.h' file        HSdLM
*******************************************************************************/

#ifndef __STVOUT_SD_DIG_H
#define __STVOUT_SD_DIG_H

#ifdef __cplusplus
extern "C" {
#endif

ST_ErrorCode_t stvout_HalSetDigitalOutputCCIRMode( void* BaseAddress, BOOL EmbeddedSync);
ST_ErrorCode_t stvout_HalSetEmbeddedSystem( void* BaseAddress, STVOUT_EmbeddedSystem_t EmbeddedSystem);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef __STVOUT_SD_DIG_H */

/* ------------------------------- End of file ---------------------------- */
