/*****************************************************************************

File name   :  stfpga.h

Description :  prototypes for FPGA programmation

COPYRIGHT (C) ST-Microelectronics 1999-2000.

Date               Modification                 Name
----               ------------                 ----
 02/11/00          Created                      FR and SB

*****************************************************************************/
/* --- prevents recursive inclusion --------------------------------------- */
#ifndef __stfpga_H
#define __stfpga_H

/* --- allows C compiling with C++ compiler ------------------------------- */
#ifdef __cplusplus
extern "C" {
#endif

/* --- includes ----------------------------------------------------------- */
#include "stddefs.h"

/* --- defines ------------------------------------------------------------ */

/* --- variables ---------------------------------------------------------- */

/* --- enumerations ------------------------------------------------------- */

/* --- prototypes of functions -------------------------------------------- */
ST_ErrorCode_t  STFPGA_Init (void);
ST_ErrorCode_t  STFPGA_GetRevision ( U16* Name, U16* VersionId );

/* ------------------------------- End of file ---------------------------- */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __stfpga_H */
