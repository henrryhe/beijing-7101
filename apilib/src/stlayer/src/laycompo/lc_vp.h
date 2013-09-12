/*******************************************************************************

File name   : lc_vp.h

Description : Layer compo viewport header file

COPYRIGHT (C) STMicroelectronics 2003.

Date               Modification                                     Name
----               ------------                                     ----
Sept 2003        Created                                           TM
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __LC_VP_H
#define __LC_VP_H


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#include "stavmem.h"
#include "stgxobj.h"
#include "lc_init.h"
#include "stdisp.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */
#define LAYCOMPO_NO_VPDISPLAY_HANDLE ((STDISP_ViewPortHandle_t) NULL)
#define LAYCOMPO_VALID_VIEWPORT   0x092E92E0   /* 0xXYnZYnZX with the driver ID XYZ in hexa
                                            * STLAYER Id = 158 => 0x09E
                                            * n = 2 */


/* Exported Types ----------------------------------------------------------- */

typedef struct
{
    STLAYER_ViewPortSource_t        Source;
    STGXOBJ_Rectangle_t             InputRectangle;
    STGXOBJ_Rectangle_t             OutputRectangle;
    STLAYER_GlobalAlpha_t           GlobalAlpha;
    laycompo_Device_t*              Device_p;         /* Pointer to parent device */
    U32                             ViewPortValidity; /* Only the value LAYCOMPO_VALID_VIEWPORT means the it is valid */
    BOOL                            Enable;           /* If Viewport is enabled */
    STLAYER_ViewPortHandle_t        NextViewPort;     /* Handle of the next open viewport. LAYCOMPO_NO_VIEWPORT_HANDLE if none*/
    STLAYER_ViewPortHandle_t        PreviousViewPort; /* Handle of the previous open viewport. LAYCOMPO_NO_VIEWPORT_HANDLE if none*/
    STDISP_ViewPortHandle_t         VPDisplay;        /* Handle of the stdisp Viewport. LAYCOMPO_NO_VPDISPLAY_HANDLE if none */
    BOOL                            FlickerFilterOn;
    STLAYER_CompositionRecurrence_t       CompositionRecurrence;
    STLAYER_FlickerFilterMode_t           FlickerFilterMode;
} laycompo_ViewPort_t;


/* Exported Variables ------------------------------------------------------- */


/* Exported Macros ---------------------------------------------------------- */


/* Exported Functions ------------------------------------------------------- */
ST_ErrorCode_t laycompo_CloseAllViewPortFromList(STLAYER_ViewPortHandle_t VPHandle);


/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __LC_VP_H */

