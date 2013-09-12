/*******************************************************************************

File name   : vid_mb2r.h

Description : Video Macroblock to Raster and Raster buffer Manager
              use header file.

COPYRIGHT (C) STMicroelectronics 2005

Date               Modification                                     Name
----               ------------                                     ----
24 dec 2004        Created                                          PLe
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __VIDDEC_MB2R_H
#define __VIDDEC_MB2R_H


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"

#include "vid_dec.h"



/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */


/* Exported Types ----------------------------------------------------------- */


/* Exported Variables ------------------------------------------------------- */


/* Exported Macros ---------------------------------------------------------- */


/* Exported Functions ------------------------------------------------------- */

ST_ErrorCode_t viddec_GetRasterPictures(VIDDEC_Data_t * VIDDEC_Data_p);
int viddec_MacroBlockToRasterInterruptHandler(void * Param);
void viddec_UnuseRasterPictures(VIDDEC_Data_t * VIDDEC_Data_p);
ST_ErrorCode_t viddec_ComputeFielsPicturesPTS(VIDDEC_Data_t * VIDDEC_Data_p, STVID_NewDecodedPictureEvtParams_t * pExportedPictureInfos, STVID_PTS_t * pPTS);
ST_ErrorCode_t viddec_SetRasterReconstructionConfiguration(VIDDEC_Data_t * VIDDEC_Data_p, VIDDEC_DecodePictureParams_t * const DecodeParams_p, const STVID_ScanType_t ScanType);

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __VIDDEC_INTERRUPT_H */

/* End of vid_intr.h */
