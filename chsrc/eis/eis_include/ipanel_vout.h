/******************************************************************************/
/*    Copyright (c) 2005 iPanel Technologies, Ltd.                            */
/*    All rights reserved. You are not allowed to copy or distribute          */
/*    the code without permission.                                            */
/*    This is the demo implenment of the Porting APIs needed by iPanel        */
/*    MiddleWare.                                                             */
/*    Maybe you should modify it accorrding to Platform.                      */
/*                                                                            */
/*    $author huzh 2007/11/19                                                 */
/******************************************************************************/

#ifndef _IPANEL_MIDDLEWARE_PORTING_VIDEO_OUTPUT_API_FUNCTOTYPE_H_
#define _IPANEL_MIDDLEWARE_PORTING_VIDEO_OUTPUT_API_FUNCTOTYPE_H_

#include "ipanel_typedef.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    IPANEL_VIDEO_OUTPUT_CVBS		= 1<<0,
    IPANEL_VIDEO_OUTPUT_SVIDEO		= 1<<1,
    IPANEL_VIDEO_OUTPUT_RGB			= 1<<2,
    IPANEL_VIDEO_OUTPUT_YPBPR		= 1<<3,
    IPANEL_VIDEO_OUTPUT_HDMI		= 1<<4,
    IPANEL_VIDEO_OUTPUT_DVI			= 1<<5,
    IPANEL_VIDEO_OUTPUT_SCART		= 1<<6,
	IPANEL_VIDEO_OUTPUT_YCBCR		= 1<<7,
	IPANEL_VIDEO_OUTPUT_CVBS_Y_C	= 1<<8,
	IPANEL_VIDEO_OUTPUT_CVBS_RGB	= 1<<9
}IPANEL_VDIS_VIDEO_OUTPUT_e;

typedef enum
{
    IPANEL_DIS_TVENC_NTSC,
    IPANEL_DIS_TVENC_PAL,
    IPANEL_DIS_TVENC_SECAM,
    IPANEL_DIS_TVENC_AUTO
} IPANEL_DIS_TVENC_MODE_e;

typedef enum
{
    IPANEL_DIS_AR_FULL_SCREEN,
    IPANEL_DIS_AR_PILLARBOX,
    IPANEL_DIS_AR_LETTERBOX,
    IPANEL_DIS_AR_PAN_SCAN,
    IPANEL_DIS_AR_AUTO      /*表示标准播放模式。即输出视频信号的宽高比与节目源信号的宽高比保持一致。*/

} IPANEL_DIS_AR_MODE_e;

typedef enum
{
    IPANEL_DIS_HD_RES_480I,
    IPANEL_DIS_HD_RES_480P,
    IPANEL_DIS_HD_RES_576I,
    IPANEL_DIS_HD_RES_576P,
	IPANEL_DIS_HD_RES_720P,			/*50Hz*/
	IPANEL_DIS_HD_RES_1080I,		/*25Hz*/
	IPANEL_DIS_HD_RES_1080P,		/*50Hz*/
	IPANEL_DIS_HD_RES_720P_60,		/*60Hz*/
	IPANEL_DIS_HD_RES_1080I_30,		/*30Hz*/
	IPANEL_DIS_HD_RES_1080P_60		/*60Hz*/
} IPANEL_DIS_HD_RES_e;

typedef struct
{
    UINT32_T x;
    UINT32_T y;
    UINT32_T w;
    UINT32_T h;
} IPANEL_RECT;

typedef enum
{
    IPANEL_DIS_SELECT_DEV           = 1,
    IPANEL_DIS_ENABLE_DEV           = 2,
    IPANEL_DIS_SET_MODE             = 3,
    IPANEL_DIS_SET_VISABLE          = 4,
    IPANEL_DIS_SET_ASPECT_RATIO     = 5,
    IPANEL_DIS_SET_WIN_LOCATION     = 6,
    IPANEL_DIS_SET_WIN_TRANSPARENT  = 7,
    IPANEL_DIS_SET_CONTRAST         = 8,
    IPANEL_DIS_SET_HUE              = 9,
    IPANEL_DIS_SET_BRIGHTNESS       = 10,
    IPANEL_DIS_SET_SATURATION       = 11,
    IPANEL_DIS_SET_SHARPNESS        = 12,
    IPANEL_DIS_SET_HD_RES           = 13,
    IPANEL_DIS_SET_IFRAME_LOCATION  = 14
}IPANEL_DIS_IOCTL_e;

/* video output */
/*打开一个显示单元*/
UINT32_T ipanel_porting_display_open(VOID);

/*关闭指定的显示管理单元*/
INT32_T ipanel_porting_display_close(UINT32_T display);

/*在指定的显示管理单元上的创建一个窗口*/
UINT32_T ipanel_porting_display_open_window(UINT32_T display, INT32_T type);

/*关闭指定的窗口*/
INT32_T ipanel_porting_display_close_window(UINT32_T display, UINT32_T window);

/*对显示设备进行一个操作，或者用于设置和获取显示设备的参数和属性*/
INT32_T ipanel_porting_display_ioctl(UINT32_T display, IPANEL_DIS_IOCTL_e op, VOID *arg);

#ifdef __cplusplus
}
#endif

#endif // _IPANEL_MIDDLEWARE_PORTING_VIDEO_OUTPUT_API_FUNCTOTYPE_H_
