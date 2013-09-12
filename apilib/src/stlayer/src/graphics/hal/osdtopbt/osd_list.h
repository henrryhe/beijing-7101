/*******************************************************************************

File name   : osd_list.h

Description : STLAYER API. GFX  OSD Part. Prototypes and types

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                               Name
----               ------------                               ----
2000-11-27         Created                                    Michel Bruant
*******************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __OSDLIST_H
#define __OSDLIST_H

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */

enum
{
    OSD_OK,
    OSD_IMPOSSIBLE_RESIZE_MOVE,
    OSD_MOVE_INVERSION_LIST
};

#define NO_NEW_VP           0x0fffffff

/* Functions ---------------------------------------------------------------- */

ST_ErrorCode_t stlayer_InsertViewport(STLAYER_ViewPortHandle_t Handle);
ST_ErrorCode_t stlayer_ExtractViewport(STLAYER_ViewPortHandle_t Handle);
U32 stlayer_IndexNewViewport(void);
ST_ErrorCode_t stlayer_TestMove(STLAYER_ViewPortHandle_t VPHandle,
                        STGXOBJ_Rectangle_t * OutputRectangle);
/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __OSDLIST_H */

/* End of osd_list.h */
