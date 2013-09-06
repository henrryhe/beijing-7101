/******************************************************************************/
/*    Copyright (c) 2005 iPanel Technologies, Ltd.                     */
/*    All rights reserved. You are not allowed to copy or distribute          */
/*    the code without permission.                                            */
/*    This is the demo implenment of the Porting APIs needed by iPanel        */
/*    MiddleWare.                                                             */
/*    Maybe you should modify it accorrding to Platform.                      */
/*                                                                            */
/*    $author huzh 2007/11/19                                                 */
/******************************************************************************/

#ifndef _IPANEL_MIDDLEWARE_PORTING_AUDIO_OUTPUT_API_FUNCTOTYPE_H_
#define _IPANEL_MIDDLEWARE_PORTING_AUDIO_OUTPUT_API_FUNCTOTYPE_H_

#include "ipanel_typedef.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    IPANEL_AOUT_DEVICE_ANALOG_STERO = 0x01, // 模拟立体声输出
    IPANEL_AOUT_DEVICE_ANALOG_MUTI  = 0x02, // 模拟多声道输出
    IPANEL_AOUT_DEVICE_SPDIF        = 0x04, // S/PDIF输出
    IPANEL_AOUT_DEVICE_HDMI         = 0x08, // HDMI输出
    IPANEL_AOUT_DEVICE_I2S          = 0x10, // I2S输出
    IPANEL_AOUT_DEVICE_ALL          = 0xff  // 所有端口
} IPANEL_AOUT_DEVICE_e;

typedef enum
{
    IPANEL_AOUT_SET_OUTPUT   = 1,
    IPANEL_AOUT_SET_VOLUME   = 2,
    IPANEL_AOUT_SET_BALANCE  = 3,
	IPANEL_AOUT_GET_VOLUME	 = 4
} IPANEL_AOUT_IOCTL_e;

/* audio output */
/*打开音频输出管理单元*/
UINT32_T ipanel_porting_audio_output_open(VOID);

/*关闭指定的音频输出单元*/
INT32_T ipanel_porting_audio_output_close(UINT32_T handle);

/*对声音输出设置进行一个操作，或者用于设置和获取声音输出设备的参数和属性*/
INT32_T ipanel_porting_audio_output_ioctl(UINT32_T handle, IPANEL_AOUT_IOCTL_e op, VOID *arg);

#ifdef __cplusplus
}
#endif

#endif // _IPANEL_MIDDLEWARE_PORTING_AUDIO_OUTPUT_API_FUNCTOTYPE_H_
