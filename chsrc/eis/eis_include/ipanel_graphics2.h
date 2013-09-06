/******************************************************************************/
/*    Copyright (c) 2009 iPanel Technologies, Ltd.                            */
/*    All rights reserved. You are not allowed to copy or distribute          */
/*    the code without permission.                                            */
/*    This is the demo implenment of the porting APIs needed by iPanel        */
/*    MiddleWare.                                                             */
/*    Maybe you should modify it accorrding to Platform.                      */
/*                                                                            */
/******************************************************************************/

#ifndef __IPANEL_GRAPHICS2_H__
#define __IPANEL_GRAPHICS2_H__

#include "ipanel_typedef.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 矩形 */
typedef struct
{
	INT32_T		x;
	INT32_T		y;
	INT32_T		width;
	INT32_T		height;
}IPANEL_GRAPHICS_RECT;


/* colorkey模式 */
typedef enum
{
	/* 禁用color_key */
	IPANEL_COLORKEY_DISABLE = 0,
	/* RGB作为colorkey */
	IPANEL_COLORKEY_RGB,
	/* ALPHA作为colorkey */
	IPANEL_COLORKEY_ALPHA,
	/* ARGB整个的作为colorkey */
	IPANEL_COLORKEY_ARGB,
}IPANEL_GRAPHICS_COLORKEY_MODE;

/* alpha模式 */
typedef enum
{
	/* 禁用alpha */
	IPANEL_ALPHA_DISABLE = 0,
	/* 全局alpha */
	IPANEL_ALPHA_GLOBAL,
	/* SrcOver混合操作 */
	IPANEL_ALPHA_SRCOVER,
}IPANEL_GRAPHICS_ALPHA_MODE;

/* 如果有alpha_flag与colorkey_flag都存在, 先计算colorkey_flag, 再计算alpha_flag */
typedef struct {
	/* alpha_flag
	1. alpha_flag==IPANEL_ALPHA_DISABLE 表示禁用alpha
	2. alpha_flag==IPANEL_ALPHA_GLOBAL 表示启用alpha, 计算公式为: 
	alpha = alpha*0xff/100
	Dst.Red		= ( alpha * Src.Red   + (0xff - Src.Alpha) * Dst.Red ) / 0xff
	Dst.Green	= ( alpha * Src.Green + (0xff - Src.Alpha) * Dst.Green ) / 0xff
	Dst.Blue	= ( alpha * Src.Blue  + (0xff - Src.Alpha) * Dst.Blue ) / 0xff
	Dst.Alpha	= ( alpha * Src.Alpha  + (0xff - Src.Alpha) * Dst.Alpha ) / 0xff
	3. alpha_flag==IPANEL_ALPHA_SRCOVER 表示启用alpha, 计算公式为:		//by willa,09/10/15,Java方面lyre要求实现
	其中srcAlpha是opaque类型的，即0标识透明度，0xff标识不透明				
	Dst.Red		= ( Src.Alpha * Src.Red   + (0xff - Src.Alpha) * Dst.Red ) / 0xff
	Dst.Green	= ( Src.Alpha * Src.Green + (0xff - Src.Alpha) * Dst.Green ) / 0xff
	Dst.Blue	= ( Src.Alpha * Src.Blue  + (0xff - Src.Alpha) * Dst.Blue ) / 0xff
	Dst.Alpha	= ( Src.Alpha  + (0xff - Src.Alpha) * Dst.Alpha ) / 0xff
	*/
	IPANEL_GRAPHICS_ALPHA_MODE		alpha_flag;
	/* colorkey模式
	Src, Dst都按32位来说明,
	Src , Dst, 是按ARGB32来说明: 
		|   Alpha |  Red | Green | Blue |
	1. colorkey_flag 是IPANEL_COLORKEY_ARGB
		如果Src==colorkey, 则Dst不变;
		如果Src!=colorkey, 则Dst=Src;
	2. colorkey_flag 是IPANEL_COLORKEY_ALPHA, , 计算公式为:
		Dst.Red		= ( Src.Alpha * Src.Red   + (0xff - Src.Alpha) * Dst.Red ) / 0xff
		Dst.Green	= ( Src.Alpha * Src.Green + (0xff - Src.Alpha) * Dst.Green ) / 0xff
		Dst.Blue	= ( Src.Alpha * Src.Blue  + (0xff - Src.Alpha) * Dst.Blue ) / 0xff
		Dst.Alpha	=  Dst.Alpha
	*/
	IPANEL_GRAPHICS_COLORKEY_MODE	colorkey_flag;
	/* alpha: 0=完全透明,100=完全不透明 */
	UINT32_T	alpha;
	/* colorkey, 是按平台支持的格式存放 */
	UINT32_T	colorkey;
}IPANEL_GRAPHICS_BLT_PARAM;


/* 初始化图形库 */
INT32_T ipanel_porting_graphics_init();

/* 关闭图形库 */
INT32_T ipanel_porting_graphics_exit();

/* 创建逻辑图层: surface创建后宽高不能再被改变直至destroy
*	width, height是surface的大小. 对于缩放或切入切出等特效，surface 的宽高 是图像变化过程中的宽高的最大值，不允许图像超出surface的边界
*  phSurface是surface句柄
*/
INT32_T ipanel_porting_graphics_create_surface( INT32_T type, INT32_T width, INT32_T height, VOID** phSurface );

/* 销毁逻辑图层 */
INT32_T ipanel_porting_graphics_destroy_surface( VOID* hSurface );


/* 取得图层点阵缓冲区信息 */
INT32_T ipanel_porting_graphics_get_surface_info( VOID* hSurface, VOID** pbuffer, INT32_T* width, INT32_T* height, INT32_T* bufWidth, INT32_T* bufHeight, INT32_T* format );


/* 图层预处理 */
INT32_T ipanel_porting_graphics_prepare_surface( VOID* hSurface, IPANEL_GRAPHICS_RECT* rect, IPANEL_GRAPHICS_BLT_PARAM* param );

/* 图层之间拷贝(带stretch/alpha/colorkey功能)
*  hSurface 是目标图层，hSurface==NULL表示直接拷贝到OSD上
*  rect目标图层的区域
*  hSrcSurface 是源图层
*  srcRect是srcSurface上的区域
*/
INT32_T ipanel_porting_graphics_stretch_surface( VOID* hSurface, IPANEL_GRAPHICS_RECT* rect, VOID* hSrcSurface, IPANEL_GRAPHICS_RECT* srcRect, IPANEL_GRAPHICS_BLT_PARAM* param );

/* 图层之间拷贝(带alpha/colorkey功能)
*  hSurface 是目标图层，hSurface==NULL表示直接拷贝到OSD上
*  rect目标图层的区域
*  hSrcSurface 是源图层
*  srcx,srcy是srcSurface上的位置
*/
INT32_T ipanel_porting_graphics_blt_surface( VOID* hSurface, IPANEL_GRAPHICS_RECT* rect, VOID* hSrcSurface, INT32_T srcx, INT32_T srcy, IPANEL_GRAPHICS_BLT_PARAM* param );


/* 在图层上填充矩形: color为平台支持的格式，rect是区域 */
INT32_T ipanel_porting_graphics_fill_surface_rect( VOID* hSurface, IPANEL_GRAPHICS_RECT* rect, UINT32_T color );

/* 在图层上画线: x1,y1是起始坐标，x2, y2是终点坐标 color为平台支持的格式 thickness为线的宽度*/
INT32_T ipanel_porting_graphics_draw_line(void* hSurface, INT32_T x1, INT32_T y1, INT32_T x2,INT32_T y2, INT32_T color, INT32_T thickness);


/* 图片预处理 */
INT32_T ipanel_porting_graphics_prepare_buffer( VOID* src, INT32_T srcLineWidth, IPANEL_GRAPHICS_RECT* rect, IPANEL_GRAPHICS_BLT_PARAM* param );
/* 将buffer图像拷贝到图层(带alpha/colorkey功能)
*  hSurface 是目标图层，hSurface==NULL表示直接拷贝到OSD上
*  rect目标图层的区域
*  src 是源图片指针
*  srcx, srcy 是源图片位置
*  srcLineWidth 是源图行像素宽度
*/
INT32_T ipanel_porting_graphics_blt_buffer( VOID* hSurface, IPANEL_GRAPHICS_RECT* rect, VOID* src, INT32_T srcx, INT32_T srcy, INT32_T srcLineWidth, IPANEL_GRAPHICS_BLT_PARAM* param );

/* 将buffer图像拷贝到图层(带stretch/alpha/colorkey功能)
*  hSurface 是目标图层，hSurface==NULL表示直接拷贝到OSD上
*  rect目标图层的区域
*  src 是源图片指针
*  srcx, srcy 是源图片位置
*  srcLineWidth 是源图行像素宽度
*/
INT32_T ipanel_porting_graphics_stretch_buffer( VOID* hSurface, IPANEL_GRAPHICS_RECT* rect, VOID* src, IPANEL_GRAPHICS_RECT* srcRect, INT32_T srcLineWidth, IPANEL_GRAPHICS_BLT_PARAM* param );


/* 是把osd的指定区域送到屏幕上显示 
*/
INT32_T ipanel_porting_graphics_update_osd( INT32_T x, INT32_T y, INT32_T width, INT32_T height );


/*
IPANEL_GRAPHICS_AVAIL_WIN_NOTIFY    arg为指向IPANEL_GRAPHICS_WIN_RECT结构的指针;
IPANEL_GRAPHICS_SET_ALPHA_NOTIFY    arg为指向 int 的指针,alpha值;当使用调色板工作模式时，设置调色板中颜色值
	透明度。注意这里设置的alpha不在作为区域alpha的控制方式。alpha：0～100，0为全透，100为完全不透明。
*/
typedef enum
{
	IPANEL_GRAPHICS_AVAIL_WIN_NOTIFY =1,
	IPANEL_GRAPHICS_GET_2D_STATUS,
	IPANEL_GRAPHICS_SET_ALPHA_NOTIFY
} IPANEL_GRAPHICS_IOCTL_e;

/*通知外部模块，浏览器实际输出的窗口的大小和位置。*/
typedef struct TAG_IPANEL_GRAPHICS_WIN_RECT
{
	int x;
	int y;
	int w;
	int h;  //willa ，ipanel_graphics.h中已经定义，编译错误。
}IPANEL_GRAPHICS_WIN_RECT;

/*
功能说明：
对Graphics进行一个操作，或者用于设置和获取Graphics设备的参数和属性。
参数说明：
输入参数：
op － 操作命令
arg - 操作命令所带的参数，当传递枚举型或32位整数值时，arg可强制转换成对应数据类型。
op, arg取值见下表：
op    arg    说明
输出参数：无
返    回：
IPANEL_OK:成功;
IPANEL_ERR:失败。
*/
INT32_T ipanel_porting_graphics_ioctl_ex(IPANEL_GRAPHICS_IOCTL_e op, VOID *arg);
#ifdef __cplusplus
}
#endif

#endif//__IPANEL_GRAPHICS2_H__
