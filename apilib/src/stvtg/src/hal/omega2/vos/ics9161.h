/************************************************************************
File Name   :   ics9161.h

Description :   ICS9161 external clock driver

COPYRIGHT (C) STMicroelectronics 2002

Date                Modification                    Name
----                ------------                    ----
06-Oct-00           Created                         FC
30 Apr 2002         Port to STVTG                   HSdLM
************************************************************************/

#ifndef __ICS9161_H
#define __ICS9161_H

/* Includes -----------------------------------------------------------*/
#include "clk_gen.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Exported Types -----------------------------------------------------*/

/* Exported Constants -------------------------------------------------*/

/* Exported Variables (Globals, not static) ---------------------------*/

/* Exported Macros ----------------------------------------------------*/

/* Exported Functions (not static) ------------------------------------*/
void stvtg_ICS9161SetPixClkFrequency(U32 Frequency);

/* ------------------------------- End of file ---------------------- */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __ICS9161_H */

