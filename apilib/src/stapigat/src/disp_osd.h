/*******************************************************************************

File name   : disp_osd.h

Description : header file of module allowing to display a graphic throught OSD

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                                     Name
----               ------------                                     ----
21 Feb 2002        Created                                           HS
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __DISP_OSD_H
#define __DISP_OSD_H


/* Includes ----------------------------------------------------------------- */

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */

/* Exported Types ----------------------------------------------------------- */

/* Exported Variables ------------------------------------------------------- */

/* Exported Macros ---------------------------------------------------------- */

/* Exported Functions ------------------------------------------------------- */

void OSD_Display(void);
BOOL OSDDisplayCmd( STTST_Parse_t *pars_p, char *result_sym_p );
BOOL DispOSD_RegisterCmd(void);

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __DISP_OSD_H */

/* End of disp_osd.h */
