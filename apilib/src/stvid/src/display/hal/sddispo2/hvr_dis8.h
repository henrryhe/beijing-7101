/*******************************************************************************

File name   : hvr_dis8.h

Description : Video display module HAL registers for sddispo2 header file

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                                     Name
----               ------------                                     ----
29 Oct 2002        Created                                          HG
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __DISPLAY_REGISTERS_SDDISPO2_H
#define __DISPLAY_REGISTERS_SDDISPO2_H


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"


/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */

/* Video plug Register offsets */
#define DISP_LUMA_BA            0x084	/* Display Luma   Source Pixmap Memory Location */
#define DISP_CHR_BA             0x088	/* Display Chroma Source Pixmap Memory Location */
#define DISP_PMP                0x08c	/* Display Pixmap Memory Pitch */

/* Exported Types ----------------------------------------------------------- */


/* Exported Variables ------------------------------------------------------- */


/* Exported Macros ---------------------------------------------------------- */


/* Exported Functions ------------------------------------------------------- */



/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __DISPLAY_REGISTERS_SDDISPO2_H */

/* End of hvr_dis8.h */
