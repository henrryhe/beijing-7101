/*******************************************************************************

File name   : disp_vid.h

Description : header file of module allowing to display a VIDEO

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                                     Name
----               ------------                                     ----
04 July 2002       Created                                           BS
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __DISP_VIDEO_H
#define __DISP_VIDEO_H


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

void VIDEO_Display(void** DataAdd_p_p, U32* BitmapWidth_p, U32* BitmapHeight_p, U8 LayerIdentity, char Disp_Video_FileName[80]);
BOOL VIDEODisplayCmd( STTST_Parse_t* pars_p, char* result_sym_p );
BOOL DispVideo_RegisterCmd(void);

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __DISP_VIDEO_H */

/* End of disp_vid.h */
