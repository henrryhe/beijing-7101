/*******************************************************************************

File name   : disp_gam.h

Description : header file of module allowing to display a graphic throught the Gamma

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                                     Name
----               ------------                                     ----
15 Mar 2002        Created                                           HS
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __DISP_GAM_H
#define __DISP_GAM_H


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

void Gamma_Display(void);
BOOL GammaDisplayCmd( STTST_Parse_t *pars_p, char *result_sym_p );
BOOL DispGam_RegisterCmd(void);

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __DISP_GAM_H */

/* End of disp_gam.h */
