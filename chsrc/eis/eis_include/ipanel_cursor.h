/******************************************************************************/
/*    Copyright (c) 2005 iPanel Technologies, Ltd.                     */
/*    All rights reserved. You are not allowed to copy or distribute          */
/*    the code without permission.                                            */
/*    This is the demo implenment of the Porting APIs needed by iPanel        */
/*    MiddleWare.                                                             */
/*    Maybe you should modify it accorrding to Platform.                      */
/*                                                                            */
/*    $author Zouxianyun 2005/04/28                                           */
/******************************************************************************/

#ifndef _IPANEL_MIDDLEWARE_PORTING_CURSOR_API_FUNCTOTYPE_H_
#define _IPANEL_MIDDLEWARE_PORTING_CURSOR_API_FUNCTOTYPE_H_

#include "ipanel_typedef.h"

#ifdef __cplusplus
extern "C" {
#endif

INT32_T ipanel_porting_cursor_get_position(INT32_T *x, INT32_T *y);
INT32_T ipanel_porting_cursor_set_position(INT32_T x, INT32_T y);
INT32_T ipanel_porting_cursor_get_shape(VOID);
INT32_T ipanel_porting_cursor_set_shape(INT32_T shape);
INT32_T ipanel_porting_cursor_show(INT32_T showflag);

#ifdef __cplusplus
}
#endif

#endif//_IPANEL_MIDDLEWARE_PORTING_CURSOR_API_FUNCTOTYPE_H_
