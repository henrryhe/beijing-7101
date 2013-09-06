/******************************************************************************/
/*    Copyright (c) 2005 iPanel Technologies, Ltd.                     		  */
/*    All rights reserved. You are not allowed to copy or distribute          */
/*    the code without permission.                                            */
/*    This is the demo implenment of the Porting APIs needed by iPanel        */
/*    MiddleWare.                                                             */
/*    Maybe you should modify it accorrding to Platform.                      */
/*                                                                            */
/*    $author huzh 2007/11/22                                                 */
/******************************************************************************/

#ifndef _IPANEL_MIDDLEWARE_PORTING_RF_API_FUNCTOTYPE_H_
#define _IPANEL_MIDDLEWARE_PORTING_RF_API_FUNCTOTYPE_H_

#include "ipanel_typedef.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
	IPANEL_RF_GET_CHRANGE = 0,
	IPANEL_RF_SET_CHANNEL = 1,
	IPANEL_RF_GET_CHANNEL = 2
} IPANEL_RF_IOCTL_e;

typedef struct
{
	INT32_T startch;
	INT32_T endch;
} IPANEL_RF_RANGE;

/* 获取或操作电视信号调制器设备信息 */
/* 返回值:IPANEL_OK: 函数执行成功; IPANEL_ERR: 函数执行失败 */
INT32_T ipanel_porting_rf_ioctl(IPANEL_RF_IOCTL_e cmd, void* arg);

#ifdef __cplusplus
}
#endif

#endif // _IPANEL_MIDDLEWARE_PORTING_RF_API_FUNCTOTYPE_H_
