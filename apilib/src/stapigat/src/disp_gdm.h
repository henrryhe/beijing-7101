/*******************************************************************************

File name   : disp_gdm.h

Description : header file of module allowing to display a graphic throught GDMA

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                                     Name
----               ------------                                     ----
17 Dec 2003        Created (based on Disp_gam.c)                     MH
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __disp_gdm_H
#define __disp_gdm_H


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

void GDMA_Display(void);
BOOL GDMADisplayCmd( STTST_Parse_t *pars_p, char *result_sym_p );
BOOL DispGDMA_RegisterCmd(void);

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __disp_gdm_H */

/* End of disp_gdm.h */
