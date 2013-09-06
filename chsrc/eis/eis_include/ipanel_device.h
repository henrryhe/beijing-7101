/******************************************************************************/
/*    Copyright (c) 2005 iPanel Technologies, Ltd.                            */
/*    All rights reserved. You are not allowed to copy or distribute          */
/*    the code without permission.                                            */
/*    This is the demo implenment of the Porting APIs needed by iPanel        */
/*    MiddleWare.                                                             */
/*    Maybe you should modify it accorrding to Platform.                      */
/*                                                                            */
/*    $author huzh 2008/09/12                                                 */
/******************************************************************************/

#ifndef _IPANEL_MIDDLEWARE_PORTING_DEVICE_API_FUNCTOTYPE_H_
#define _IPANEL_MIDDLEWARE_PORTING_DEVICE_API_FUNCTOTYPE_H_

#include "ipanel_typedef.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IPANEL_DEVICE_PARAMS_MAX_LENGTH	64
#define IPANEL_DEVICE_BUFFER_MAX_LENGTH	64

typedef struct
{
	CHAR_T params[IPANEL_DEVICE_PARAMS_MAX_LENGTH];
	CHAR_T buffer[IPANEL_DEVICE_BUFFER_MAX_LENGTH];
}IPANEL_DEVICE_INFO;


/* EIS_MULTIFNIRKEY_MOTION P2扩展定义 */
typedef struct {
	unsigned char 	button;
	unsigned char 	dx;		/* delta_x; */
	unsigned char 	dy;		/* delta_y */
	unsigned char 	dwheel;	/* delta_wheel*/
	unsigned int  	seq;
	unsigned short 	ax;  	/* acceleration_x */
	unsigned short 	ay;		/* acceleration_y */
	unsigned short 	az;		/* acceleration_z */
	unsigned short 	angular_velocity_x;
	unsigned short 	angular_velocity_y;
	unsigned short 	angular_velocity_z;	
}MultiFnIrkeyMot;

/* EIS_MULTIFNIRKEY_POSITION P2扩展定义 */
typedef struct {
	unsigned char 	button;
	unsigned char 	dx;		/* delta_x; */
	unsigned char 	dy;		/* delta_y */
	unsigned char 	dwheel;	/* delta_wheel*/
	unsigned int  	seq;
	unsigned short 	x;
	unsigned short 	y;
	unsigned short 	z;
	unsigned short 	angular_position_a;
	unsigned short 	angular_position_b;
	unsigned short 	angular_position_c;
	unsigned short 	angular_position_d;
}MultiFnIrkeyPos;

/*获取网络，电子邮件等参数值，不同硬件商有不同的用途，一般都是和ipanel自定义的JS配合使用*/
/*	输入参数：params，自定义的名称; length，字符串值的长度.
	输出参数：buf，当前名称对应的值.
	返    回：返回字符串值的长度。*/
INT32_T	ipanel_porting_device_read(CONST CHAR_T *params, CHAR_T *buf, INT32_T length);

/*设置网络，电子邮件等参数值，不同硬件商有不同的用途，一般都是和ipanel自定义的JS配合使用*/
/*	输入参数：无
	输出参数：params，自定义的名称; buf，该名称参数对应的值; length，字符串值的长度.
	返    回：通常返回参数值的长度，-1 表示未知的参数或者错误。*/
INT32_T ipanel_porting_device_write(CONST CHAR_T *params, CHAR_T *buf, INT32_T length);

#ifdef __cplusplus
}
#endif

#endif // _IPANEL_MIDDLEWARE_PORTING_DEVICE_API_FUNCTOTYPE_H_
