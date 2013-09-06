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

#ifndef _IPANEL_MIDDLEWARE_PORTING_SYSTEM_APPLICATION_API_FUNCTOTYPE_H_
#define _IPANEL_MIDDLEWARE_PORTING_SYSTEM_APPLICATION_API_FUNCTOTYPE_H_

#include "ipanel_typedef.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
参数为一个字符串，只有一个，这个参数是从页面传过来的，iPanel MiddleWare相当于一个通道，
原封不动的将从页面获得的字符串调用这个接口传给底层。由底层触发一些动作。是否销毁
iPanel MiddleWare由实现此函数的相关人员决定。
*/
//INT32_T ipanel_start_other_app(CONST CHAR_T *name);

#ifdef __cplusplus
}
#endif

#endif // _IPANEL_MIDDLEWARE_PORTING_SYSTEM_APPLICATION_API_FUNCTOTYPE_H_
