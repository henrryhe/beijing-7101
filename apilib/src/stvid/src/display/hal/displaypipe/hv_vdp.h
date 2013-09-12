/*******************************************************************************

File name   : hv_vdp.h

Description : HAL video display pipe displaypipe family header file

COPYRIGHT (C) STMicroelectronics 2005

Date               Modification                                     Name
----               ------------                                     ----
17 Oct 2005        Created                                           DG
*******************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __HAL_VIDEO_DISPLAY_DISPLAYPIPE_H
#define __HAL_VIDEO_DISPLAY_DISPLAYPIPE_H


/* Includes ----------------------------------------------------------------- */
#include "stddefs.h"
#include "halv_dis.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */

/* VDP Register offsets */
#define VDP_DEI_PYF_BA		    		0x014	/* luma previous field base address */
#define VDP_DEI_CYF_BA			    	0x018	/* luma current field base address */
#define VDP_DEI_NYF_BA			    	0x01C	/* luma next field base address */
#define VDP_DEI_PCF_BA	    			0x020	/* chroma previous field base address */
#define VDP_DEI_CCF_BA		    		0x024	/* chroma current field base address */
#define VDP_DEI_NCF_BA			    	0x028	/* chroma next field base address */

/* Exported Types ----------------------------------------------------------- */

/* Exported Variables ------------------------------------------------------- */
extern const HALDISP_FunctionsTable_t HALDISP_DisplayPipeFunctions;

/* Exported Macros ---------------------------------------------------------- */

/* Exported Functions ------------------------------------------------------- */

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __HAL_VIDEO_DISPLAY_DISPLAYPIPE_H */

/* End of hv_vdp.h */
