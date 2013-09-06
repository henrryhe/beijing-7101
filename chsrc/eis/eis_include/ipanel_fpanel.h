/******************************************************************************/
/*    Copyright (c) 2005 iPanel Technologies, Ltd.                     */
/*    All rights reserved. You are not allowed to copy or distribute          */
/*    the code without permission.                                            */
/*    This is the demo implenment of the Porting APIs needed by iPanel        */
/*    MiddleWare.                                                             */
/*    Maybe you should modify it accorrding to Platform.                      */
/*                                                                            */
/*    $author huzh 2007/11/22                                                 */
/******************************************************************************/

#ifndef _IPANEL_MIDDLEWARE_PORTING_FRONT_PANEL_API_FUNCTOTYPE_H_
#define _IPANEL_MIDDLEWARE_PORTING_FRONT_PANEL_API_FUNCTOTYPE_H_

#include "ipanel_typedef.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 'cmd' for ipanel_porting_front_panel_ioctl() */
typedef enum {
    IPANEL_FRONT_PANEL_SHOW_TEXT     = 1,
    IPANEL_FRONT_PANEL_SHOW_TIME     = 2,
    IPANEL_FRONT_PANEL_SET_INDICATOR = 3,
    IPANEL_FRONT_PANEL_UNKNOWN
} IPANEL_FRONT_PANEL_IOCTL_e;
INT32_T ipanel_porting_front_panel_ioctl(IPANEL_FRONT_PANEL_IOCTL_e cmd, VOID *arg);

typedef struct
{
	UINT32_T value;
	UINT32_T mask;
} IPANEL_FRONT_PANEL_INDICATOR;

#ifdef __cplusplus
}
#endif

#endif // _IPANEL_MIDDLEWARE_PORTING_FRONT_PANEL_API_FUNCTOTYPE_H_
