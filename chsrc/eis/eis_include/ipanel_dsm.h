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

#ifndef _IPANEL_MIDDLEWARE_PORTING_DSM_API_FUNCTOTYPE_H_
#define _IPANEL_MIDDLEWARE_PORTING_DSM_API_FUNCTOTYPE_H_

#include "ipanel_typedef.h"

#ifdef __cplusplus
extern "C" {
#endif

/* descrambler */
/* encryption modes */
typedef enum
{
	IPANEL_DSM_TYPE_PES,
	IPANEL_DSM_TYPE_TS,
	IPANEL_DSM_TYPE_UNKNOWN
} IPANEL_ENCRY_MODE_e;

/*******************************************************************************
分配一个解扰器通道，并按照指定设定工作模式。
input:	encryptmode：解扰模式，如TS或PES
output:	None
return:	> 0 成功， 返回分配的解扰器的句柄; 0 失败。
*******************************************************************************/
UINT32_T ipanel_porting_descrambler_allocate(IPANEL_ENCRY_MODE_e encryptmode);

/*******************************************************************************
释放一个通过ipanel_porting_descrambler_allocate分配的解扰器通道。
input:	handle:解扰器的句柄
output:	None
return:	IPANEL_OK 成功, IPANEL_ERR 失败
*******************************************************************************/
INT32_T ipanel_porting_descrambler_free(UINT32_T handle);

/*******************************************************************************
设置解扰器通道PID
input:	handle:解扰器的句柄
				pid:需要解扰的流的PID(AudioPid,VideoPid)
output:	None
return:	IPANEL_OK 成功; IPANEL_ERR 失败
*******************************************************************************/
INT32_T ipanel_porting_descrambler_set_pid(UINT32_T handle, UINT16_T pid);

/*******************************************************************************
设置奇控制字。
input:	handle:解扰器的句柄
				key:控制字的值
				keylen:控制字长度
output:	None
return:	IPANEL_OK 成功; IPANEL_ERR 失败
*******************************************************************************/
INT32_T ipanel_porting_descrambler_set_oddkey(UINT32_T handle, BYTE_T *key, INT32_T keylen);

/*******************************************************************************
设置偶控制字。
input:	handle:解扰器的句柄
				key:控制字的值
				keylen:控制字长度
output:	None
return:	IPANEL_OK 成功; IPANEL_ERR 失败
*******************************************************************************/
INT32_T ipanel_porting_descrambler_set_evenkey(UINT32_T handle, BYTE_T *key, INT32_T keylen);

#ifdef __cplusplus
}
#endif

#endif // _IPANEL_MIDDLEWARE_PORTING_DSM_API_FUNCTOTYPE_H_
