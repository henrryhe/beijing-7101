/*******************************************************************************

File name   : tbx_filt.h

Description : Filtering and redirection feature header file

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                                     Name
----               ------------                                     ----
13 Jan 2000        Created                                          HG
05 Feb 2002        Development.                                     HS
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __TBX_FILTER_REDIR_H
#define __TBX_FILTER_REDIR_H

#if !defined ST_OSLINUX
/* Under Linux, STTBX functions are defined in STOS vob */

/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#include "sttbx.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */

#define STTBX_NB_OF_INPUT_TYPES  1
#define STTBX_NB_OF_PRINT_TYPES  1
#define STTBX_NB_OF_REPORT_TYPES STTBX_NB_OF_REPORT_LEVEL


/* Exported Types ----------------------------------------------------------- */

/* Exported Variables ------------------------------------------------------- */

/* Exported Macros ---------------------------------------------------------- */

/* Exported Functions ------------------------------------------------------- */

void sttbx_ApplyInputFiltersOnInputDevice(const STTBX_ModuleID_t ModuleID, STTBX_Device_t *const Device_p);
void sttbx_ApplyPrintFiltersOnOutputDevice(const STTBX_ModuleID_t ModuleID, STTBX_Device_t * const Device_p);
void sttbx_ApplyReportFiltersOnOutputDevice(const STTBX_ReportLevel_t Level, const STTBX_ModuleID_t ModuleID, STTBX_Device_t * const Device_p);
void sttbx_ApplyRedirectionsOnDevice(STTBX_Device_t * const Device_p);

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif  /* #ifndef ST_OSLINUX */
#endif /* #ifndef __TBX_FILTER_REDIR_H */

/* End of tbx_filt.h */
