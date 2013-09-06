/******************************************************************************/
/*    Copyright (c) 2005 iPanel Technologies, Ltd.                            */
/*    All rights reserved. You are not allowed to copy or distribute          */
/*    the code without permission.                                            */
/*    This is the demo implenment of the porting APIs needed by iPanel        */
/*    MiddleWare.                                                             */
/*    Maybe you should modify it accorrding to Platform.                      */
/*                                                                            */
/*    $author Zouxianyun 2005/04/28                                           */
/******************************************************************************/

#ifndef _IPANEL_MIDDLEWARE_PORTING_GRAPHICS_API_FUNCTOTYPE_H_
#define _IPANEL_MIDDLEWARE_PORTING_GRAPHICS_API_FUNCTOTYPE_H_

#include "ipanel_typedef.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    int x;
    int y;
    int w;
    int h;
} IPANEL_GRAPHICS_WIN_RECT;

typedef enum
{
    IPANEL_GRAPHICS_AVAIL_WIN_NOTIFY	= 1,
	IPANEL_GRAPHICS_GET_2D_STATUS		= 2
} IPANEL_GRAPHICS_IOCTL_e;

enum {
	IPANEL_GRAPHICS_2D_STATUS_FINISH = 1
};

INT32_T ipanel_porting_graphics_ioctl(IPANEL_GRAPHICS_IOCTL_e op, VOID *arg);

/* get display information */
INT32_T ipanel_porting_graphics_get_info(INT32_T *width, INT32_T *height,
            VOID **pBuffer, INT32_T *bufWidth, INT32_T *bufHeight);

/* install palette, only 8bits need */
INT32_T ipanel_porting_graphics_install_palette(UINT32_T *pal, INT32_T npals);

/* draw image */
INT32_T ipanel_porting_graphics_draw_image(INT32_T x, INT32_T y, INT32_T w,
            INT32_T h, BYTE_T *bits, INT32_T w_src);

/* 设置整个graphics层的透明度。alpha：0～100，0为全透，100为完全不透明。
   返    回：IPANEL_OK:成功;IPANEL_ERR:失败。*/			   
INT32_T ipanel_porting_graphics_set_alpha(INT32_T alpha);

#ifdef __cplusplus
}
#endif

#endif//_IPANEL_MIDDLEWARE_PORTING_GRAPHICS_API_FUNCTOTYPE_H_
