/******************************************************************************/
/*    Copyright (c) 2005 iPanel Technologies, Ltd.                            */
/*    All rights reserved. You are not allowed to copy or distribute          */
/*    the code without permission.                                            */
/*    This is the demo implenment of the Porting APIs needed by iPanel        */
/*    MiddleWare.                                                             */
/*    Maybe you should modify it accorrding to Platform.                      */
/*                                                                            */
/*    $author Zouxianyun 2005/04/28                                           */
/******************************************************************************/

#ifndef _IPANEL_MIDDLEWARE_API_H_
#define _IPANEL_MIDDLEWARE_API_H_

#include "ipanel_typedef.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * create iPanel application. you must input a memory size (>=5M).
 * and it will return browser handle, null is fail
 */
VOID *ipanel_create(UINT32_T mem_size);

/* process events */
INT32_T ipanel_proc(VOID *handle, UINT32_T msg, UINT32_T p1, UINT32_T p2);

/* destroy iPanel middleware */
INT32_T ipanel_destroy(VOID *handle);

INT32_T ipanel_open_uri(VOID *handle, CHAR_T *url);

INT32_T ipanel_open_overlay(VOID *handle, CHAR_T *url, UINT32_T x,UINT32_T y,UINT32_T w,UINT32_T h);

UINT32_T ipanel_get_utc_time();//返回从1970年开始到当前的秒数
INT32_T ipanel_set_dns_server(INT32_T no, CHAR_T *ipaddr);
INT32_T ipanel_get_dns_server(INT32_T no, CHAR_T *ipaddr, INT32_T len);

/* register protocol */
typedef INT32_T (*IPANEL_PROTOCOL_EXT_CBK)(CONST CHAR_T *protocolurl);
INT32_T ipanel_register_protocol(CONST CHAR_T *protocol, IPANEL_PROTOCOL_EXT_CBK func);

typedef enum {
	IPANEL_OPEN_OC_HOMEPAGE = 1,	/* 指向serviceid的整型指针,外部APP向ipanel请求打开serviceid的oc首页 */
	IPANEL_AUDIO_CHANNEL	= 2,	/* arg传字符串指针，字符分别为："STEREO"--立体声, "LEFT_MONO"--左声道, "RIGHT_MONO"--右声道, "MIX_MONO"--左右声道混合，"STEREO_REVERSE"--立体声、左右声道反转 */
	IPANEL_AUDIO_MUTE	 	= 3,	/* arg为字符串指针，字符串分别为："UNMUTE"--非静音，"MUTE"--静音 */
	IPANEL_AUDIO_VOLUME		= 4,	/* arg传int类型指针 */
	IPANEL_PUSHVOD_START				= 5,	/* arg为指向存放pushvod频点的频率整型指针 */
	IPANEL_CHANGE_TS_BYOTHERAPP			= 6 	/* arg无意义*/ 
} IPANEL_IOCTL_e;
/*
 * 对iPanel MiddleWare当前实例进行一个操作，或者用于设置和获取iPanel MiddleWare当前实例的参数和属性。  
 */
INT32_T ipanel_ioctl(VOID *handle, IPANEL_IOCTL_e op, VOID *arg);

/* 该接口给dvb3.0裁剪版使用，用以从底层获取频点，如果不是dvb3.0裁剪版底层不用实现，库里自行实现了 */
int ipanel_porting_get_frequency(int on_id, int ts_id, int service_id);

#ifdef __cplusplus
}
#endif

#endif//_IPANEL_MIDDLEWARE_API_H_
