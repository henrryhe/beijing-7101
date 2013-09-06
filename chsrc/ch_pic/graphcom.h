#ifndef _GRAPHCOM__H_
#define _GRAPHCOM__H_

#include "status.h"
#include "graphconfig.h"
#include "common.h"
/*#include "ch_CQInfoService.h" */
extern ST_Partition_t          *appl_partition ;


#define gz_STATIC  /*静态定义*/

/*内存管理函数  ，  需要根据平台设置*/




#define	CH_MIN(x,y)	((x)<(y)?(x):(y))
#define	CH_MAX(x,y)	((x)>(y)?(x):(y))
#define CH_ABS(x) (((x)<0)?(-(x)):(x))
#define CH_SQU(x)	((x) * (x))
#define CH_ZERO(x)	memset(&(x), 0, sizeof(x))

#ifndef EOF
#define EOF -1
#endif

#ifndef NULL
#undef  NULL
#define NULL (void*)0
#endif

/*无符号整数,根据不同的平台应该有所改变*/
#ifndef DEFINED_U8
#define DEFINED_U8
#define U8 unsigned char
#endif

#ifndef DEFINED_U16
#define DEFINED_U16
#define U16 unsigned short
#endif

#ifndef DEFINED_U32
#define DEFINED_U32
#define U32 unsigned  long
#endif

#ifndef DEFINED_U64
#define DEFINED_U64
#define U64 CH_UNSIGNED_64
#endif

#ifndef UINT
#define UINT unsigned int/* 尽量少用，最好不用,而用U32类型替代，避免产生编译警告 */
#endif

/*符号整数，根据不同的平台应该有所改变*/
#ifndef DEFINED_S8
#define DEFINED_S8
#define S8 signed char
#endif

#ifndef DEFINED_S16
#define DEFINED_S16
#define S16 signed short
#endif

#ifndef DEFINED_S32
#define DEFINED_S32
#define S32 signed  long
#endif

#ifndef DEFINED_S64
#define DEFINED_S64
#define S64 CH_SIGNED_64
#endif

#if 0
#ifndef INT
#define INT int /* 尽量少用，最好不用,而用S32类型替代，避免产生编译警告 */
#endif
#endif/*5107平台已经定义*/

#ifndef CH_FLOAT 
#define CH_FLOAT float
#endif

#ifndef CH_BOOL
#define CH_BOOL int
#endif

#define CH_TRUE 1
#define CH_FALSE 0

#ifndef PU64
#define PU64 unsigned long int
#endif

/*取最大、最小值*/
#if 0
#define	CH_MIN(x,y)	((x)<(y)?(x):(y))
#define	CH_MAX(x,y)	((x)>(y)?(x):(y))
#define CH_ABS(x) (((x)<0)?(-x):(x))
#endif
#define ch_SCREEN_WIDTH (720 - 1)/*显示屏幕大小*/
#define ch_SCREEN_HIGH (576 - 1)

#define ch_SCREEN_ADJUST_X 	(0)/*屏幕坐标偏移*/
#define ch_SCREEN_ADJUST_Y	(0)

#define ch_MaxWindowsNum 20/*最大允许打开的窗口个数*/

#define DashedLength 4 /*虚线段及虚线留白的长度*/

#define CH_IfColorGet(a)	{if(CH_NotCaseSensitiveStrCompare(ColorName,(S8*)(#a)) == 0)		\
						return (PCH_RGB_COLOUR)&ch_gcolor_##a;}	

#ifdef USE_ARGB1555

 #define CHZ_DrawColorPoint(r, g, b, buf)	\
 {		\
 	*((unsigned short*)(buf)) = 0x8000  		\
 						+ ((((unsigned short)(r) >> 3) & 0x1f) << 10)		\
 						+ ((((unsigned short)(g) >> 3) & 0x1f) << 5)		\
 						+ ((((unsigned short)(b) >> 3) & 0x1f));		\
 }
 #elif USE_ARGB4444
  #define CHZ_DrawColorPoint(r, g, b, buf)	\
 {		\
 	*((unsigned short*)(buf)) = 0xf000  		\
 						+ ((((unsigned short)(r) >> 4) & 0x0f) << 8)		\
 						+ ((((unsigned short)(g) >> 4) & 0x0f) << 4)		\
 						+ ((((unsigned short)(b) >> 4) & 0x0f));		\
 }
#elif defined(USE_ARGB8888)
 #define CHZ_DrawColorPoint(r, g, b, buf)	\
 {		\
 	*((unsigned char*)(buf + 3)) = 0xff;		\
 	*((unsigned char*)(buf + 2)) = r;		\
 	*((unsigned char*)(buf + 1)) = g;		\
 	*((unsigned char*)(buf + 0)) = b;		\
 }
#else
 #define CHZ_DrawColorPoint(r, g, b, buf)	\
 {		\
 	*((unsigned short*)(buf)) = ((g & 0xf0) + (b >> 4)) + (((unsigned short)(0xf0 + (r >> 4))) << 8);		\
 }
#endif
 
/* 输入参数：CH_RGB_COLOUR 结构
 * 返回对应平台的RBG颜色值*/
#define  CH_GET_RGB_COLOR(RgbColor) RgbColor->Rgb##32##Entry


/*通用图形接口函数*/


/*基本的绘图数据结构*/
typedef struct
{
	S16 	x;
	S16 	y;
}CH_POINT,*PCH_POINT;

typedef struct
{
	U16 	Left;
	U16 	Right;
	U16 Top;
	U16 Bottom;
}CH_AREA_LIMIT,*PCH_AREA_LIMIT;

typedef struct
{
	U8			uRed;
	U8			uGreen;
	U8			uBlue;
	U8			bAlphaState;
}CH_RGB15_ENTRY,*PCH_RGB15_ENTRY;

typedef	struct
{
	U8			uRed;
	U8			uGreen;
	U8			uBlue;
}CH_RGB16_ENTRY,*PCH_RGB16_ENTRY;

typedef	struct
{
	U8			uRed;
	U8			uGreen;
	U8			uBlue;
	U8			uAlpha;
}CH_RGB32_ENTRY,*PCH_RGB32_ENTRY;

typedef	struct
{
	U8			uRed;
	U8			uGreen;
	U8			uBlue;
	U8		       uAlpha;
}CH_RGB12_ENTRY;

typedef	union
{
	U8	iColourIndex;
	CH_RGB15_ENTRY	Rgb15Entry;
	CH_RGB16_ENTRY	Rgb16Entry;
	CH_RGB32_ENTRY	Rgb32Entry;
	CH_RGB12_ENTRY	Rgb12Entry;
}CH_RGB_COLOUR,*PCH_RGB_COLOUR;

typedef struct 
{
	int iStartX;
	int iStartY;/*这个窗口或者对象的左上角点的坐标*/
	int iWidth;
	int iHeigh;/*窗口或者对象的高度和宽度*/
}CH_RECT,*PCH_RECT;

typedef struct
{
	U16 iStartX;/*起始点坐标*/
	U16 iStartY;
	U16 iLeftLimit;/*行左*/
	U16 iRightLimit;/*行右*/
	U16 iEndX;/*结束点坐标*/
	U16 iEndY;
}CH_LimitArea, *PCH_LimitArea;/*区域定义，因为不一定是规则多边形，所以需要单独定义*/

typedef struct _CH_WINDOW_BUF
{
	U32 dwHandle;/*屏幕缓冲区地址*/
	U16 width;
	U16 height;
	struct _CH_WINDOW_BUF* pNext;
}CH_WINDOW_BUF, *PCH_WINDOW_BUF;

typedef enum
{
	CH_HYPERLINK_T = 1,/*超联接*/
	CH_CHECKBOX_T,/*复选框*/
	CH_RADIO_T,/*单选按纽*/
	CH_SELECT_T,/*下拉菜单*/
	CH_BUTTON_T,/*按纽*/
	CH_BASICDLG_T,/*消息对话框*/
	CH_EDIT_T,/*输入框*/
	/*以上定义的顺序不能改，且不可以加入其他东西*/
	
	CH_WINDOWS_MENU_T,/*窗口系统的菜单*/
	CH_USER_DEFINED_EVENT_T/*用户自定义的特殊元素*/
}CH_WinDlg_TYPE;

typedef struct  _CH_ACTIVE_NODE
{
	S8 Enable;/*0为未激活，非0为激活*/
	PCH_RECT Place;/*位置大小等信息*/
	PCH_WINDOW_BUF ActFrame;/*表示目前显示到动画的第几桢了*/
	U32 DelayTime;/*延迟时间*/
	U32 TimeCount ;/*计时器*/
	PCH_WINDOW_BUF PicAddr;/*接点内容*/
	void* buf;
	struct _CH_ACTIVE_NODE *Next;
} CH_ACTIVE_NODE, *PCH_ACTIVE_NODE;


typedef struct 
{
	CH_WINDOW_BUF scrBuffer;
	CH_RECT Screen;/*当前窗口大小等信息*/
	U16 SingleScrWidth;/*单屏宽*/
	U16 SingleScrHeight;/*单屏高*/
	S16 ColorByte;/*表示一个象素所用的字节数。注意，不是位数!*/
	PCH_RGB_COLOUR BackgroundColor;
	PCH_RGB_COLOUR CurrentColor;
	PCH_ACTIVE_NODE ActNode;/*动态节点，需要适时刷新的部分，如动画，公告栏等*/
	struct tagCONTROL* DlgControl;/*控件，如按钮等等*/
	struct _ControlCom* ControlHead;/*控件排序树的头*/
}CH_WINDOW,*PCH_WINDOW;



typedef enum
{
	LR_TB = 1,/*从左到右，从上到下*/
	RL_TB,/*从右到左，从上到下*/
	TB_RL/*从上到下，从右到左*/
}CH_TEXT_DIRECTIOIN;/*文字方向*/

typedef enum
{
	XING_KAI = 1,/*行楷*/
	SONG_TI,/*宋体*/
	FANG_SONG,/*仿宋*/
	LI_SHU/*隶书*/
}CH_FONT_STYLE;/*字体*/

typedef enum
{
	NONE = 1,/*无*/
	UNDERLINE ,/*下划线*/
	LINE_THROUGH,/*贯穿线*/
	OVERLINE/*上划线*/
}CH_FONT_DICORATION;/*装饰*/

typedef struct
{
	S16 	iWidth;
	S16 	iHeight;
}CH_SIZE;/*大小*/


typedef struct 
{
	CH_FONT_STYLE Style;/*字体*/
	CH_SIZE FontSize;/*大小*/
	PCH_RGB_COLOUR Color;/*颜色*/
	CH_TEXT_DIRECTIOIN Drct;/*文字方向*/
	CH_FONT_DICORATION UnderLine;/*下划线*/
	S16 Oblique;/*字体倾斜，0为正常，非0为倾斜*/
	S16 WordSpacing;/*老美的意思是英文单词之间的距离，我理解为中文字体间距*/
	S16 LetterSpacing;/*英文字母之间的间距*/
	S16 LineHeight;/*行高，在这里以上都用像素来度量*/
}CH_FONT, *PCH_FONT;/*字的有关信息*/

 typedef enum
{
	CPT_JPG = 1,
	CPT_JPEG ,
	CPT_JPE,
	CPT_JFIF,
	CPT_GIF,
	CPT_BMP,
	CPT_DIB,
	CPT_PNG,
	CPT_PCX
}CH_PictureType;/*图象类型*/

#define	id_backgroundRepeat(a)	id_backgroundRepeat_##a
typedef enum
{
	id_backgroundRepeat(repeat) = 1,/*背景图像在纵向和横向上平铺*/
	id_backgroundRepeat(no_repeat),/*背景图像不平铺*/
	id_backgroundRepeat(repeat_x),/*背景图像在横向上平铺*/
	id_backgroundRepeat(repeat_y),/*背景图像在纵向平铺*/
	id_backgroundRepeat(room)/*缩放到指定范围大小*/
}id_backgroundRepeat;
 

extern CH_STATUS CH_PutBitmapToScr3(PCH_WINDOW ch_this, PCH_RECT psrcRect, PCH_RECT pdestRect, int trans, S8* bitmap);
extern PCH_WINDOW CH_CreateWindow(S16 iWindowWidth, S16 iWindowHigh);
CH_PictureType CH_GetPicType(S8* FileName);
void CH_FreeMem(void * pAddr) ;
void *CH_AllocMem(U32 size);/*分配size个字节的内存*/


#define CH_MemSet memset
#define CH_MemCopy memcpy
	
#define J_OSP_FreeMemory CH_FreeMem


#endif


