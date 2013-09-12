/*******************************************************************************

File name   : osd_list.h

Description : 

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                               Name
----               ------------                               ----
2000-12-20          Creation                                    Michel Bruant

*******************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __OSDLIST_H
#define __OSDLIST_H

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Constants ---------------------------------------------------------------- */

#define NO_NEW_VP 0x0fff

/* Functions ---------------------------------------------------------------- */

ST_ErrorCode_t stlayer_osd_InsertViewport(STLAYER_ViewPortHandle_t Handle);
ST_ErrorCode_t stlayer_osd_ExtractViewport(STLAYER_ViewPortHandle_t Handle);
U32 stlayer_osd_IndexNewViewport(void);

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __OSDLIST_H */

/* End of osd_list.h */
