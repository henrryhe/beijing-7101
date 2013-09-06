/*
  Name:graphicbase.c
  Description:bottom function about display using STGFX in 5516/7710/7101/7105 platform
  created date :2004-05-17 by yixiaoli for changhong  project
  modify date:
  1.2004-10-11 create .lib
  2.2004-10-15  by yixiaoli
  1)modify InitFlashFileInfo() function,对"GlobalflashFileinfo"对应的内存作了处理
  2）对FLASH 实际读出图片数和所传参数不统一的处理策略作了修改。
  3）函数返回类型由"void"改为"bool" 

  3.2004-10-15 modify function,CH_DrawRectangle(),add paramters "STGXOBJ_Bitmap_t* pDstBMP",
  并作内部修改，将其改为 static 使用, 并对涉及的CH6_ClearArea(),CH6_ClearFullArea()作修改。
  4.2004-10-18 add function BOOL CH6_DisplayPICByData();
	5.add two function CH6_SetPixelColor(),CH6_GetPixelColor();	
	6.2004-11-02 add  function extern void CH6_InitFont(unsigned int FontBaseAdr);

  7.2004-11-05 modify function  CH6_Drawbitmap(),solve the problem which can't display red picture when mixing with background

  8.2004-11-06 clear the bug of function CH6_GetPixelColor();

  9.2004-11-09,
  1)modify function CH6_CreateViewPort(),解决由I 帧进入调色板模式后再退出后I 帧显示异常的问题
  2)ADD macro YXL_GRAPHICBASE_DEBUG,All debug info is controled by this macro.
  10.2004-11-10
  1)modify CH6_SetColorValue(),与ZXG 约定，如所传ALPHA为9，则实际ALPHA=0xf,既不透，此为满足MOSAIC的需要。 
  2)Modify ch6_TextOut(),将汉字offset由0x18c90 改为 60912，既字库里不要空北的分区，save flash capabity
 
3)modify struct FlashFileHead,add member "unsigned char Magic[4]",used as check code when read pic from flash,
if it's not right,that is not "CHPC,all pic can't display but the program can run continue
10.2004-11-11 by yixiaoli
1)modify CH6_PutRectangle(),CH6_GetRectangle(),Add paramters STGFX_GC_t,用于多个VIEWPORT同时画图时不出现串色现象，
 （在股票中表现")
11.2004-11-12
1)rename function CH6_GetBitmapinfoFromViewporthandle as CH6_GetBitmapinfo();
2)删出不用的函数，变量等

11.2004-11-16 by yixiaoli
1)modify function CH6_DrawFont(),使字颜色的ALPHA 不起作用，总为0x0f
2)使gFontMem不起作用，既不分配gFontMem内存

12.2004-11-20 by yixiaoli
1.Eliminate the bug of function CH6_DrawFont() which made in 2004-11-16
(2004-11-16 改错地方了，同时也不对),
约定当ALPHA= 9，则ALPHA=15，其余则为本值；（在mosaic)

13.2004-11-30 by yixiaoli
1)modify function CH6_ClearFullArea() and CH6_ClearArea(),将清屏函数的初始值由ARGB(0X0FFF)改为ARGB(0X0000),
否则在进行viewport大小变换时会在屏幕上显示一条白线，
2)原来在 显示主菜单时出现闪动问题的最根本原因是Viewport的起点为（0，0），
如Viewport的起点不为（0，0），则清屏函数的初始值为ARGB(0X0000)也不会引起闪动

14.2004-12-05 纪（2004-12-04 to 2004-12-05 ) by yixiaoli
1)删出 function CH6_FreeGraphic(),this function isn't used
2)Move two struct CH6_Palette_t and CH6_Bitmap_t from graphicbase.h to graphicbase.c,because these two structs are inner
 structs,just used by inner fubction of graphicbase.c
3)Delete two members of struct CH6_ViewPortParams_t in graphicbase.h,that's
		STAVMEM_BlockHandle_t BlockHandleBitmap;
	STAVMEM_BlockHandle_t BlockHandlePalette;
Modify function CH6_CreateViewPort() which is correlative to this struct
4) 	Delete one members of struct CH_ViewPort_t in graphicbase.h,that's
	BOOL VPDisStatus;
	Modify correlative functions
	CH6_CreateViewPort() ;	CH6_ShowViewPort() ;CH6_HideViewPort() ;and so on
5)Add three members 
	STAVMEM_BlockHandle_t BlockHandleBitmap;
	STAVMEM_BlockHandle_t BlockHandlePalette;
	BOOL VPDisStatus;
	to struct CH6_ViewPort_InnerParams_t in graphicbase.c,
	Modify correlative functions
	CH6_CreateViewPort() ;	CH6_ShowViewPort() ;CH6_HideViewPort() ;
	CH6_InitInnerParam(),CH6_ReleaseInnerParam(),CH6_FindMatchInnerParam(),
	CH6_SetInnerParam()	and so on
6) Modify function CH6_GetViewportDisplayStatus();
7)add  function CH6_SettViewportDisplayStatus();
8)Modify function CH6_DeleteViewPort(CH_ViewPort_t VPHandle,CH6_ViewPortParams_t* Params) to
  BOOL CH6_DeleteViewPort(CH_ViewPort_t VPHandle);
  and made inner modification
9)Add member 	TYPE_BITMAP_AVMEM_HANDLE, 
	TYPE_PALLETTE_AVMEM_HANDLE, 
	TYPE_VIEWPORT_SHOW_STATUS
in enumeration InnerParamsType;

15.2005-05-30  by yixiaoli
Add macro FOR_CH_5100_USE,与此macro相关:
1)add member TYPE_BITMAP in enumerate and  
    STGXOBJ_Bitmap_t Bitmap in struct CH6_ViewPort_InnerParams_t;
2)modify function CH6_FindMatchInnerParam() and  CH6_SetInnerParam(),add case TYPE_BITMAP的相关内容
3）modify function CH6_CreateViewport(),使与VIEWPORT 相关的位图信息存在一个内部数据结构中，供以后使用
4）modify function CH6_GetBitmapInfo(),使与VIEWPORT 相关的位图信息从一个内部数据结构中得到，而不用ST的API
STLAYER_GetViewPortSource()

16.2005-06-07  by yixiaoli
1)Modify function CH6_CopyStillData();将与VIEWPORT 相关的位图信息从一个内部数据结构中得到，而不用ST的API
STLAYER_GetViewPortSource()
2)Cancel function  CH6_CopyStillData()的相关部分

17.2005-06-08  by yixiaoli
1)Modify function CH6_MixLayer();add three parameters 

18.2005-06-09  by yixiaoli
1)Add new function BOOL CH_AllocStillBitmap() 用于STILL layer的位图分配
2) Modify function CH_InitGraphic()的相关部分
3)

19.2005-07-14  by yixiaoli
1)Add macro FOR_CH_7710_USE

20.2005-09-20by yixiaoli
in australia 
1)Add function BOOL CH_SetViewPortAlpha(CH_ViewPort_t CHVPHandle,int AlphaValue);


21 .2005-09-23 by yixiaoli

22.2005-10-25- 2005-10-30  by yixiaoli
1)Rename struct CH_ViewPort_t to CH_ViewPort_t,and define CH_ViewPortHandle_t,add two new member 
	CH_ViewPortHandle_t ViewPortHandle;
	CH_ViewPortHandle_t ViewPortHandleAux;
	in struct CH_ViewPort_t
2)Add new member in struct CH6_ViewPort_InnerParams_t,these new member is connected to 
macro FOR_CH_7710_USE

3)Modify below function :CH6_InitInnerParam(),CH6_ReleaseInnerParam(),
CH6_FindMatchInnerParam(),CH6_SetInnerParam(),CH6_GetBitmapInfo(),CH6_ShowViewPort(),CH6_HideViewPort(),
CH6_GetViewPortDisplayStatus(),CH6_SetViewPortDisplayStatus(),CH6_CreateViewPort(),
CH_SetViewPortAlpha(),and so on.
4)Modify CH6_SetPixelColor() and CH6_GetPixelColor(), modify parameter CH_ViewPort_t to CH_ViewPortHandle_t,and modify inner
5)Add new function,CH_DisplayCHVP();

Date:2005-11-09  by yixiaoli
1)Copy file vidinj.c and vidinj.h from ZXG,该文件实现了7710 I帧显示功能，并将其加入到graphicbase.lib中
2）Modify function CH6_DisplayPIC(),使其能实现显示7710I帧显示功能   
3）将 function ST_ErrorCode_t CH7_FrameDisplay(U8 *MemBase,int MemSize) 独立出来


Date:2006-03-06  by yixiaoli
 1.Modify function CH6_TextOut() for new font display,this font has different font width.
 2.Modfiy ch_font.h.
 3.将CH6_GetTextWidth 由graphicmid.c 移到graphicbase.c ,并根据变宽字体作修改。

Date:2006-03-20  by yixiaoli
 1. Add new function CH6_GetCharWidth(char Char),用于变宽字体的计算

Date:2006-04-22  by yixiaoli
 1. Add new macro ENGLISH_FONT_MODE_EQUEALWIDTH,并修改相关函数 CH6_GetCharWidth()、CH6_TextOut()等

Date:2006-11-13  by yixiaoli
1. 优化相关函数

Date:2007-02-05 -- 2007-02-09 by yixiaoli
1. 对图形显示底层进行结构大修改，主要是为了支持7710、7100 HD、SD OSD 同时输出，但是该结构对只有
SD OSD 输出的QAMI5516也同样适宜。该修改在7100平台上已验证。
2、根据底层结构修改对相关函数进行修改。
3、删除以前不用的一些函数，包括修改记录过程。
4、优化相关函数
5、删除MACRO FOR_CH_5100_USE 和 FOR_CH_7710_USE，增加MACRO FOR_CH_QAMI5516_USE，
6、新增结构CH_GraphicInitParams和函数CH_InitGraphicBase()，将外部调用的字库、图片初始化函数集中到该函数中，
7、新增STATIC 变量CHGDPPartition，将图形底层使用的内存分区由固定的分去改变为调用参数决定。
8、修改函数BOOL InitFlashFileInfo(). 

Date:2007-03-05 by yixiaoli 
1. 修改函数CH6_TextOut(),将调用参数由char* pTextString改为unsigned char* pTextString，并在内部进行下列改动：
A)int offset改为unsigned int offset
B)U8* pFontData改为U8* pFontData=NULL
C)U8* pDisMemTemp改为U8* pDisMemTemp=NULL
D)char* pTempString=pTextString改为unsigned char* pTempString=pTextString,

（主要是因为操作系统由OS20 变为OS21 ,否则汉字显示不出来）

Date:2007-04-10---2007-04-12 by yixiaoli 
1.修改FALSH读写的相关部分，主要是由于ST M29，SPANSION 29 FALSH 引起的 

Date:yxl 2007-04-25 by yixiaoli 
1.将2007-04-10---2007-04-12 期间修改的FLASH读写的相关部分恢复为原状，因为修改
	ST M29，SPANSION 29 FALSH的读写控制函数后，这些FLASH可直接地址访问。

Date:yxl 2007-04-27 by yixiaoli

Date:yxl 2007-04-28 by yixiaoli  

Date:yxl 2007-07-03 by yixiaoli  
1.Modify function CHB_DisplayIFrame(),add new two parameters U32 Loops and U32 Priority,
主要用于由应用控制I帧显示的循环次数和优先级，保证I帧可靠显示

Date:yxl 2007-07-04 by yixiaoli  
1.Add new function CHB_DisplayIFrameFromMemory(U8* pData,U32 Size,CH_VIDCELL_t CHVIDCell,U32 Loops,U32 Priority),
主要用于从内存显示I帧

Date:2007-07-16 by yixiaoli  
1.Add MACRO GRAPHIC_FOR_LOADER_USE ,添加及修改相关的函数

Date:2007-08-14 by yixiaoli  
1.Modify function BOOL CH6_DrawBitmap()的相关部分，消出在颜色格式为ARGB1555，
ARGB565时有背景图片显示的一个BUG。

Date:2007-08-14，2007-09-03 by yixiaoli  
1.Add ARGB8888 格式支持，由MACRO YXL_ARGB8888控制，并修改如下函数支持ARGB8888
（主要是将与颜色相关的参数，变量由int 等改为unsigned int,或加入ARGB8888格式支持)
(1)CH6_TextOut(...);
(2)CH6_SetColorValue(...);
(3)CH6_GetPixelColor(...);
(4)CH6_SetPixelColor(...);
(5)CH6_DrawRectangle(...);
(6)CH6_DrawFont(...);
(7)CH6_LDR_TextOut(...);
(8)CH6_DrawBitmap();
(9)CH6_GetRectangleA();
(10)CH6_GetRectangle();
(11)CH6_PutRectangle();
(12)CH6_ClearArea();
(13)CH6_CreateViewPort();
(14)CH_SetViewPortAlpha();
(15)CH6_DisplayPIC();

2.Add new function char* CHDRV_OSDB_GetVision(void),用于查询图形底层的版本，
 目前版本为：CHDRV_OSDB_R3.1.1.

Date:2007-09-10 by Yixiaoli
1.Modify function CH6_SetColorValue(...),CH6_CreateViewPort()
解决ARGB8888格式在576P下显示异常问题.

Date:2007-09-11 by Yixiaoli
1.Modify function CH6_InitFont(...)
2.Add new function 
A:unsigned long int CHDRV_OSD_GetFontBaseAddr(void),
B:U8 CHDRV_OSD_GetFontLIBType(void),
C:U8* CHDRV_OSD_GetFontDisplayMem(STLAYER_ViewPortHandle_t VPHandle)

3.根据MACRO USE_UNICODE 是否定义来决定function CH6_TextOut() 的位置
如果定义USE_UNICODE ，则将function CH6_TextOut()移动到新建文件目录ch_text\下.
否则不变。

目的是:让字符编码由只支持GBK编码改为即支持GBK编码,又支持UNICODE编码,
		字库由只支持GBK字库改为即支持GBK字库,又支持UNICODE字库
备注:UNICODE编码和字库是由香港提出的,因此,在有关字符串显示上作了扩充
4.更新版本为CHDRV_OSDB_R3.1.2


Date:2007-11-22 by Yixiaoli
1.yxl 2007-11-22 add macro GRAPHIC_LOADER_ONLY_ENG,when define this macro,loader
  graphic lib only have english font ,并修改相应部分GetCodeOffset(),CH6_LDR_TextOut()
2.modify function CHDRV_OSDB_GetVision(),将LOADER的图形底层的版本和应用区分开来（用L标示LOADER），
并加入平台标志(7101)，因此更新图形版本为：
LOADER(only eng)：CHDRV_OSDB_7101RL3.1.1
LOADER(both eng and chi)：CHDRV_OSDB_7101RL3.1.2
应用：CHDRV_OSDB_7101R3.1.2


Date:2008-03-27 by Yixiaoli
1.Add new function CH_GetLastDisplayViewPort() 以及相应的局部变量，CH_ViewPortHandle_t LastUpdateVPHandle
该函数用于得到最近显示的VIEWPORT 句柄，主要用于MHEG5 显示控制的需要。

Date:2008-08-26 by Yixiaoli
1.定义MACRO YXL_ARGB8888，将ARGB8888格式公开,并修改函数CH6_SetColorValue(),CH6_DrawFont()将原来调试
8888格式时加的强制转换关掉。

3.更新版本为CHDRV_OSDB_7101R3.3.1。(USE_UNICODE=0,ENGLISH_FONT_MODE =1)
4.更新版本为CHDRV_OSDB_7101R3.3.2。(USE_UNICODE=1,ENGLISH_FONT_MODE =1)

Date:2008-09-26 by Yixiaoli
1.生成新版本为CHDRV_OSDB_7101R3.3.3。(USE_UNICODE=1,ENGLISH_FONT_MODE =1,NOmheg5)  


Date:2008-11-11 by Yixiaoli
1.增加新的结构CH_DrawRectangle_t，CH_DrawInfo_t，并对结构CH_ViewPort_t 作了修改。
1.Modify function CH6_CreateViewPort(),修改函数原型和内部，将画图视窗的个数和大小改由参数确定，并对函数内部作了优化。
3.生成新版本为CHDRV_OSDB_7101R4.1.2。(USE_UNICODE=1,ENGLISH_FONT_MODE =1,NOmheg5)  


Date:2009-04-07,2009-04-13 by Yixiaoli
1.生成新版本为CHDRV_OSDB_7101R4.2.1。(USE_UNICODE=1,ENGLISH_FONT_MODE =1)  
  生成新版本为CHDRV_OSDB_7101R4.2.2。(USE_UNICODE=1,ENGLISH_FONT_MODE =1,NOmheg5,to 天津)  

 
Date:2009-04-27 by Yixiaoli
1. 生成新版本为CHDRV_OSDB_7101R4.3.1。(USE_UNICODE=0,ENGLISH_FONT_MODE =1,have mheg5)  

2. Modidy  function CH6_DrawBitmap(),将 图片的ALPHA显示由外部函数CH_SLLK_GetAlpha()控制 ,
   主要是特殊市场的需要如斯里兰卡的需要,CH6_DisplayPIC()调用,
   生成新版本为CHDRV_OSDB_5202R4.3.2。(USE_UNICODE=1,ENGLISH_FONT_MODE =1,NOmheg5,to JQZ for 斯里兰卡的需要 )  

Date:2009-08-25 by Yixiaoli
1.to 北京双向
   生成新版本为CHDRV_OSDB_7101R4.5.1。(USE_UNICODE=0,ENGLISH_FONT_MODE =1,NOmheg5)  

 
*/

#include <string.h>
#include "stddefs.h"
#include "stdevice.h"
#include "..\main\errors.h"
#include "..\main\initterm.h"
#include "graphicbase.h"
#include "..\usif\graphicmid.h"
#include "channelbase.h"
#include "..\main\errors.h"

#if 0 /*yxl 2006-04-22 modify below section*/
#include "ch_font.h"

#else

#ifdef ENGLISH_FONT_MODE_EQUEALWIDTH
#include "ch_font_eqwidth.h"
#else
#include "ch_font.h"
#endif

#endif/*end yxl 2006-04-22 modify below section*/

#if 0
#define GRAPHIC_FOR_LOADER_USE /*yxl 2007-07-16 add this line*/
#endif

#ifdef GRAPHIC_FOR_LOADER_USE  /*yxl 2007-07-16 add below section*/

#if 0
#define GRAPHIC_LOADER_ONLY_ENG /*yxl 2007-11-22 add this macro,when define this macro,loader
                                 graphic lib only have english font */
#endif

#ifndef GRAPHIC_LOADER_ONLY_ENG /*yxl 2007-11-22 add this line*/
#if 0 
#include "ch_Loaderfont.h" /*简体中文*/
#else
#include "ch_Loaderfont_unicode.h"/*繁体中文*/
#endif

#endif /*end yxl 2007-07-16 add below section*/
#endif /*yxl 2007-11-22 add this line*/

#if 1
#define YXL_ARGB8888  /*yxl 2007-08-14 add*/
#endif

extern STBLIT_Handle_t BLITHndl; /*yxl 2007-06-18 add this line*/




#if 0
#define YXL_GRAPHICBASE_DEBUG   /*yxl 2004-11-09 add this macro*/
#endif



short GetFileIndex(char *filename);
FlashFileStr  *GlobalFlashFileInfo=NULL; /*yxl 2004-10-15 add =NULL*/
FlashFileHead *GlobalFlashFileHead=NULL;



/*static */ST_Partition_t* CHGDPPartition=NULL;/*yxl 2007-02-10 add this variable*/


typedef struct 
{
	STGXOBJ_Bitmap_t Bitmap;
	STAVMEM_BlockHandle_t BlockHandle1;
	STAVMEM_BlockHandle_t BlockHandle2;
	STAVMEM_PartitionHandle_t PartitionHandle;/*yxl 2006-11-13 add this member,
												Partition handle of the memory which the Bitmap will uses*/ 
	
}CH6_Bitmap_t;

typedef struct 
{
	STGXOBJ_Palette_t Palette;
	STAVMEM_BlockHandle_t BlockHandle;
	STAVMEM_PartitionHandle_t PartitionHandle;/*yxl 2006-11-13 add this member,
	Partition handle of the memory which the Palette will uses*/ 
}CH6_Palette_t;



typedef struct 
{
	STLAYER_ViewPortHandle_t LayerVPHandle;
	STGXOBJ_Bitmap_t LayerVPBitmap;
	STAVMEM_BlockHandle_t BlockHandle;
	BOOL VPDisStatus;
	
   	STAVMEM_PartitionHandle_t PartitionHandle;/* Partition handle of the memory which the viewport will uses,yxl 2006-11-13 add this member */
	
}CH6_SingleViewPort_InnerParams_t;



#define MAX_CHBITMAP_COUNT MAX_NUM_OF_DRAWVIEWPORT


typedef struct 
{
	BOOL OccupySign; /*TRUE,stand for used,FALSE standfor not used*/
	U8* pFontDisMem;
	U8* pFontDataMem;
	U8 CHBitmapCount; 
	CH_ViewPortHandle_t CHVPHandle;
 	CH6_Bitmap_t CHBitmapInner[MAX_CHBITMAP_COUNT];
	U8 LayerCount; 
	CH6_SingleViewPort_InnerParams_t LayerInnerParams[MAX_LAYERVIEWPORT_NUMBER];
	CH6_Palette_t CHPalette;

}CH6_ViewPort_InnerParams_t;



#define MAX_OSDVIEWPORT_NUMBER 3

#if 1 /*yxl 2004-10-29 add this variable for font display*/

static BOOL IsParamsInit=FALSE;
static CH6_ViewPort_InnerParams_t CH6_VPInner_Params[MAX_OSDVIEWPORT_NUMBER];
#endif /*yxl 2004-10-29 add this variable for font display*/


typedef struct
{
	U32 uOffset;
	U16	uSize;
	U16	uData;
}tuple_node;

#if 1 /* yxl 2004-10-29 add this section*/



typedef enum
{
	TYPE_FONT_DISPLAY_MEM,
	TYPE_FONT_DATA_MEM,
	TYPE_CHVPHANDLE,
	TYPE_LAYERHANDLE,
	TYPE_LAYERBITMAP,
	TYPE_CHBITMAP,
	TYPE_CHPALETTE
}InnerParamsType;



/*函数声明*/
static BOOL InitFlashFileInfo(unsigned int PicStartAdr,int MaxPicNameLen);
static BOOL CH6_DrawBitmap(STGFX_Handle_t Handle,STGXOBJ_Bitmap_t* pOperBitmap,int PosX,int PosY,int BMPWidth,int BMPHeight,U8* pBMPData,BOOL IsDisplayBack);
static BOOL CH6_SetViewPortDisplayStatus(CH_ViewPort_t* pCH6VPHandle/*STLAYER_ViewPortHandle_t VPHandle*/,BOOL* pDisplayStatus);
static BOOL CH6_GetRectangleA(STGFX_Handle_t Handle, STGXOBJ_Bitmap_t* pOperBitmap,int PosX, int PosY, int Width, int Height, 
				 U8* pBMPData);



/*yxl 2007-09-03 add below function*/
char* CHDRV_OSDB_GetVision(void)
{
#ifdef GRAPHIC_FOR_LOADER_USE /*yxl 2007-11-22 add below section*/
#ifdef GRAPHIC_LOADER_ONLY_ENG
	return "CHDRV_OSDB_7101RL3.1.1";
#else
/*	return "CHDRV_OSDB_7101RL3.1.2";yxl 2008-01-04 cancel this */
	return "CHDRV_OSDB_7101RL3.1.3";/*yxl 2008-01-04 add this */
	return "CHDRV_OSDB_7101RL4.1.1";/*yxl 2009-06-15 add this to蒋斌，去掉所有打印，DISABLE_STPRINT=1*/
#endif
	
#else/*end yxl 2007-11-22 add below section*/
/*	return "CHDRV_OSDB_R3.1.2"; yxl 2007-11-22 cancel*/
	/*	return "CHDRV_OSDB_7101R3.1.2";yxl 2007-11-22 add this line，yxl 2008-08-14 cancel below*/
/*	return "CHDRV_OSDB_7101R3.2.2";"CHDRV_OSDB_7101R3.2.1"，yxl 2008-08-14 add this line*/
/*	return "CHDRV_OSDB_7101R3.3.2";"CHDRV_OSDB_7101R3.3.1"，yxl 2008-08-25 add this line*/
/*	return "CHDRV_OSDB_7101R3.3.3";"CHDRV_OSDB_7101R3.3.1"，yxl 2008-09-26 add this line*/
/*	return "CHDRV_OSDB_7101R4.1.1";"CHDRV_OSDB_7101R3.3.3"，yxl 2008-11-11 add this line*/
/*	return "CHDRV_OSDB_7101R4.1.2";"CHDRV_OSDB_7101R4.1.1"，yxl 2008-12-04 add this line,no MHEG5*/
/*	return "CHDRV_OSDB_7101R4.2.1";/*"CHDRV_OSDB_7101R4.1.2" ;yxl 2008-12-04 add this line,no MHEG5*/
	/*return "CHDRV_OSDB_5202R4.3.2";yxl 2009-04-27 add this line,no MHEG5*/
/*	return "CHDRV_OSDB_7101R4.3.1";yxl 2009-04-27 add this line,erase a bug*/
	
		return "CHDRV_OSDB_7101R4.5.1";/*yxl 2009-08-25 add this line,*/

#endif /*yxl 2007-11-22 add this line*/
}
/*end yxl 2007-09-03 add below function*/




static int CH6_GetFreeParam(void)
{
	int i;
	for(i=0;i<MAX_OSDVIEWPORT_NUMBER;i++)
	{
		if(CH6_VPInner_Params[i].OccupySign==FALSE)
			break;
	}
	if(i>=MAX_OSDVIEWPORT_NUMBER) return -1;
	return i;
}


/*
int Index:when >=MAX_OSDVIEWPORT_NUMBER,initliaze all inner paramters,other init specified inner paramter
*/
static void CH6_InitInnerParam(int Index)
{
	int i=0;
	int j=0;
	int StartTemp;
	int EndTemp;

	if(Index>=MAX_OSDVIEWPORT_NUMBER)
	{
		StartTemp=0;
		EndTemp=MAX_OSDVIEWPORT_NUMBER;
	}
	else
	{
		StartTemp=Index;
		EndTemp=Index+1;		
	}

	for(i=StartTemp;i<EndTemp;i++)
	{
		
		CH6_VPInner_Params[i].OccupySign=FALSE;		
		CH6_VPInner_Params[i].pFontDataMem=NULL;
		CH6_VPInner_Params[i].pFontDisMem=NULL;
		CH6_VPInner_Params[i].CHVPHandle=ST_ERROR_INVALID_HANDLE;
		
		CH6_VPInner_Params[i].CHBitmapCount=0;
		for(j=0;j<MAX_CHBITMAP_COUNT;j++)
		{
			memset((void*)&CH6_VPInner_Params[i].CHBitmapInner[j],0,sizeof(CH6_Bitmap_t));
			CH6_VPInner_Params[i].CHBitmapInner[j].BlockHandle1=STAVMEM_INVALID_BLOCK_HANDLE;
			CH6_VPInner_Params[i].CHBitmapInner[j].BlockHandle2=STAVMEM_INVALID_BLOCK_HANDLE;
			CH6_VPInner_Params[i].CHBitmapInner[j].PartitionHandle=ST_ERROR_INVALID_HANDLE;
		}

		CH6_VPInner_Params[i].LayerCount=0;
		for(j=0;j<MAX_LAYERVIEWPORT_NUMBER;j++)
		{
			memset((void*)&CH6_VPInner_Params[i].LayerInnerParams[j],0,sizeof(CH6_SingleViewPort_InnerParams_t));
			CH6_VPInner_Params[i].LayerInnerParams[j].PartitionHandle=ST_ERROR_INVALID_HANDLE;
			CH6_VPInner_Params[i].LayerInnerParams[j].LayerVPHandle=ST_ERROR_INVALID_HANDLE;
			CH6_VPInner_Params[i].LayerInnerParams[j].BlockHandle=STAVMEM_INVALID_BLOCK_HANDLE;
			CH6_VPInner_Params[i].LayerInnerParams[j].VPDisStatus=FALSE;
		}

		memset((void*)&CH6_VPInner_Params[i].CHPalette,0,sizeof(CH6_Palette_t));
		CH6_VPInner_Params[i].CHPalette.PartitionHandle=ST_ERROR_INVALID_HANDLE;
		CH6_VPInner_Params[i].CHPalette.BlockHandle=STAVMEM_INVALID_BLOCK_HANDLE;

	}
	IsParamsInit=TRUE;
	return;
}


static BOOL CH6_ReleaseInnerParam(CH_ViewPortHandle_t Handle)
{
	STAVMEM_FreeBlockParams_t FreeBlockParams;
	STAVMEM_BlockHandle_t BlockHandleTemp;
	ST_ErrorCode_t ErrCode;
	int i;
	int j;

	for(i=0;i<MAX_OSDVIEWPORT_NUMBER;i++)
	{
		if(CH6_VPInner_Params[i].OccupySign==FALSE) continue;		
		if(CH6_VPInner_Params[i].CHVPHandle==Handle) break;
	}
	if(i>=MAX_OSDVIEWPORT_NUMBER) return TRUE;
	
	if(CH6_VPInner_Params[i].pFontDataMem!=NULL)
	{
		memory_deallocate(CHGDPPartition,CH6_VPInner_Params[i].pFontDataMem);	
	}
	
	if(CH6_VPInner_Params[i].pFontDisMem!=NULL)
	{
		memory_deallocate(CHGDPPartition,CH6_VPInner_Params[i].pFontDisMem);	
	}
	
	for(j=0;j<CH6_VPInner_Params[i].LayerCount;j++)
	{
		FreeBlockParams.PartitionHandle = CH6_VPInner_Params[i].LayerInnerParams[j].PartitionHandle;
		BlockHandleTemp=CH6_VPInner_Params[i].LayerInnerParams[j].BlockHandle;
		if(BlockHandleTemp!=STAVMEM_INVALID_BLOCK_HANDLE||\
			FreeBlockParams.PartitionHandle!=ST_ERROR_INVALID_HANDLE)
		{
			
			ErrCode=STAVMEM_FreeBlock (&FreeBlockParams, &BlockHandleTemp);
			if(ErrCode!=ST_NO_ERROR)
			{
				STTBX_Print(("STAVMEM_FreeBlock()%s",GetErrorText(ErrCode)));
			}
		}
	}
	
	for(j=0;j<CH6_VPInner_Params[i].CHBitmapCount;j++)
	{
		FreeBlockParams.PartitionHandle = CH6_VPInner_Params[i].CHBitmapInner[j].PartitionHandle;
		BlockHandleTemp=CH6_VPInner_Params[i].CHBitmapInner[j].BlockHandle1;
		if(BlockHandleTemp!=STAVMEM_INVALID_BLOCK_HANDLE||\
			FreeBlockParams.PartitionHandle!=ST_ERROR_INVALID_HANDLE)
		{
			
			ErrCode=STAVMEM_FreeBlock (&FreeBlockParams, &BlockHandleTemp);
			if(ErrCode!=ST_NO_ERROR)
			{
				STTBX_Print(("STAVMEM_FreeBlock()%s",GetErrorText(ErrCode)));
			}
		}
	}

	
	FreeBlockParams.PartitionHandle=CH6_VPInner_Params[i].CHPalette.PartitionHandle;
	BlockHandleTemp=CH6_VPInner_Params[i].CHPalette.BlockHandle;
	if(BlockHandleTemp!=STAVMEM_INVALID_BLOCK_HANDLE||\
		FreeBlockParams.PartitionHandle!=ST_ERROR_INVALID_HANDLE)
	{
		
		ErrCode=STAVMEM_FreeBlock (&FreeBlockParams, &BlockHandleTemp);
		if(ErrCode!=ST_NO_ERROR)
		{
			STTBX_Print(("STAVMEM_FreeBlock()%s",GetErrorText(ErrCode)));
		}
	}
	
	CH6_InitInnerParam(i);
	return FALSE;
}


/*static*/ void* CH6_FindMatchInnerParam(U32 Handle,InnerParamsType Type)
{
	int i,j;
	InnerParamsType ParamsType;
	void* pTemp=NULL;

	ParamsType=Type;

	for(i=0;i<MAX_OSDVIEWPORT_NUMBER;i++)
	{
		if(CH6_VPInner_Params[i].OccupySign==FALSE) continue;
		switch(ParamsType)
		{
			case TYPE_LAYERHANDLE:
			case TYPE_LAYERBITMAP:
				for(j=0;j<CH6_VPInner_Params[i].LayerCount;j++)
				{
					if(CH6_VPInner_Params[i].LayerInnerParams[j].LayerVPHandle==Handle)
					{
						if(ParamsType==TYPE_LAYERHANDLE)
						{
							pTemp=(void*)&CH6_VPInner_Params[i].LayerInnerParams[j].LayerVPHandle;
						}
						else if(ParamsType==TYPE_LAYERBITMAP)
						{
							pTemp=(void*)&CH6_VPInner_Params[i].LayerInnerParams[j].LayerVPBitmap;	
						}
						return pTemp;   
					}
				}
			break;
			case TYPE_CHBITMAP:
			case TYPE_FONT_DISPLAY_MEM:
			case TYPE_FONT_DATA_MEM:
				for(j=0;j<CH6_VPInner_Params[i].CHBitmapCount;j++)
				{
					if(CH6_VPInner_Params[i].CHBitmapInner[j].BlockHandle1==Handle)
					{
						if(ParamsType==TYPE_CHBITMAP)
						{
							pTemp=(void*)&CH6_VPInner_Params[i].CHBitmapInner[j].Bitmap;
						}
						else if(ParamsType==TYPE_FONT_DISPLAY_MEM)
						{
							pTemp=(void*)CH6_VPInner_Params[i].pFontDisMem;	
						}
						else if(ParamsType==TYPE_FONT_DATA_MEM)
						{
							pTemp=(void*)CH6_VPInner_Params[i].pFontDataMem;	
						}
						return pTemp;   
					}
				}
				
				break;

			case TYPE_CHVPHANDLE:
				
				break;
			default:
				break;
		}
	}

	if(i>=MAX_OSDVIEWPORT_NUMBER) 
	{
			
		return NULL;
	}
}



static BOOL CH6_SetInnerParam(int Index,InnerParamsType ParamsType,void* pvalue)
{

	int j=0;
	int i=Index;

	/*yxl 2007-05-14 add below section*/
	if(pvalue==NULL) 
	{
		return TRUE;
	}
	/*end yxl 2007-05-14 add below section*/

	if(Index>=MAX_OSDVIEWPORT_NUMBER) return TRUE;

	if(CH6_VPInner_Params[i].OccupySign==FALSE)
	{
		CH6_VPInner_Params[i].OccupySign=TRUE;
	}


	switch(ParamsType)
	{
		case TYPE_LAYERHANDLE:
		case TYPE_LAYERBITMAP:
			for(j=0;j<MAX_LAYERVIEWPORT_NUMBER;j++)
			{
				if(CH6_VPInner_Params[i].LayerInnerParams[j].LayerVPHandle==ST_ERROR_INVALID_HANDLE)
				{
					
					memcpy((void*)&CH6_VPInner_Params[i].LayerInnerParams[j],
						(const void*)pvalue,sizeof(CH6_SingleViewPort_InnerParams_t));
					CH6_VPInner_Params[i].LayerCount++;
					return FALSE;
				}
			}
			
			break;
			
		case TYPE_CHBITMAP:
			for(j=0;j<MAX_CHBITMAP_COUNT;j++)
			{
				if(CH6_VPInner_Params[i].CHBitmapInner[j].BlockHandle1==STAVMEM_INVALID_BLOCK_HANDLE)
				{
					
					memcpy((void*)&CH6_VPInner_Params[i].CHBitmapInner[j],
						(const void*)pvalue,sizeof(CH6_SingleViewPort_InnerParams_t));
					CH6_VPInner_Params[i].CHBitmapCount++;					
					return FALSE; 
				}
			}
			break;
		case TYPE_FONT_DISPLAY_MEM:
			CH6_VPInner_Params[i].pFontDisMem=(U8*)pvalue;		
			return FALSE;
		case TYPE_FONT_DATA_MEM:
			CH6_VPInner_Params[i].pFontDataMem=(U8*)pvalue;	
			return FALSE;
		case TYPE_CHPALETTE:

			memcpy((void*)&CH6_VPInner_Params[i].CHPalette,(const void*)pvalue,sizeof(CH6_Palette_t));
			return FALSE; 
		case TYPE_CHVPHANDLE:
			CH6_VPInner_Params[i].CHVPHandle=*((STLAYER_ViewPortHandle_t*)pvalue);	
			return FALSE; 


		default:
			break;

	}

	return TRUE;

}



#endif /*end yxl 2004-10-29 add this section*/







void CH6_Uncompress( U8* pCompress, U16* pObjData, U32 uByteLen )
{
	unsigned int			uLoop1, uLoop2, uSrcIndex;
	tuple_node		tuple;

	uLoop1		= 0;
	uLoop2		= 0;	
	uSrcIndex	= 0;


	do{

		tuple.uOffset	= pCompress[uSrcIndex] + (pCompress[uSrcIndex+1] << 8) + (pCompress[uSrcIndex+2] << 16) + (pCompress[uSrcIndex+3] << 24);
		uSrcIndex += 4;
		tuple.uSize		= pCompress[uSrcIndex] + (pCompress[uSrcIndex+1] << 8);
		uSrcIndex += 2;
		
		if( tuple.uSize == 0 )
		{
			tuple.uData			= pCompress[uSrcIndex] + (pCompress[uSrcIndex+1] << 8);
			uSrcIndex			+= 2;
			pObjData[uLoop1++]	= tuple.uData;
		}
		else
		{
			memcpy( &pObjData[uLoop1], &pObjData[tuple.uOffset], (tuple.uSize - 1)*2 );
			uLoop1 += (tuple.uSize - 1);
		}
	}while( uLoop1 < uByteLen/2 );

	return;
}




void RGB2YUV(U8 R, U8 G, U8 B,CH6_ColorYUVValue_t* pYUV)
{
	U8 Y, Cr, Cb;

	Y  = (16  + ( ((257*R)/1000) + ((504*G)/1000) + ((98 *B)/1000)));
    Cr = (128 + ( ((439*R)/1000) - ((368*G)/1000) - ((71 *B)/1000)));
    Cb = (128 + (-((148*R)/1000) - ((291*G)/1000) + ((439*B)/1000)));
	pYUV->Y=Y;
	pYUV->Cr=Cr;
	pYUV->Cb=Cb;
}

BOOL SetPaletteColor(STGXOBJ_Palette_t* pPalette,U8* pRGBValue,unsigned short IndexCount)
{
	int i;
	ST_ErrorCode_t ErrCode;
	STGXOBJ_Color_t Color;
	CH6_ColorYUVValue_t YUV;
 
	if(pPalette->Data_p==NULL) return TRUE;

	if(pRGBValue==NULL) return TRUE;
	if(pPalette->ColorType !=STGXOBJ_COLOR_TYPE_UNSIGNED_AYCBCR6888_444) return TRUE;
	Color.Type = STGXOBJ_COLOR_TYPE_UNSIGNED_AYCBCR6888_444;
	for(i=0;i<IndexCount;i++)
	{

		if(i!=0)	
		{
			RGB2YUV(*(pRGBValue+3*i), *(pRGBValue+3*i+1), *(pRGBValue+3*i+2),&YUV);	
			Color.Value.UnsignedAYCbCr6888_444.Alpha = 63;
		}
		else 
		{
			RGB2YUV(0xff, 0xff,0xff,&YUV);	
			Color.Value.UnsignedAYCbCr6888_444.Alpha = 0;
		}

		Color.Value.UnsignedAYCbCr6888_444.Y= YUV.Y;
		Color.Value.UnsignedAYCbCr6888_444.Cb= YUV.Cb;
		Color.Value.UnsignedAYCbCr6888_444.Cr= YUV.Cr;
		ErrCode = STGXOBJ_SetPaletteColor(pPalette,i,&Color);
		if(ErrCode != ST_NO_ERROR)
		{
			STTBX_Print(("STGXOBJ_SetPaletteColor()=%s",GetErrorText(ErrCode)));
			return TRUE;
		}
	}
	return FALSE;

}






#define GXOBJ_HARDUSE (STGXOBJ_GAMMA_BLITTER | \
                                   STGXOBJ_GAMMA_GDP_PIPELINE | \
                                   STGXOBJ_GAMMA_CURS_PIPELINE)

/*static BOOL CH_AllocOSDBitmap(STLAYER_Handle_t LayerHandle,STAVMEM_PartitionHandle_t AVHandle,
							  CH6_Bitmap_t* pCH6BMP,int BMPWidth,int BMPHeight)*/
/*yxl 2006-11-13 cancel paramter STAVMEM_PartitionHandle_t AVHandle*/
static BOOL CH_AllocOSDBitmap(STLAYER_Handle_t LayerHandle,CH6_Bitmap_t* pCH6BMP,int BMPWidth,int BMPHeight)
{

    ST_ErrorCode_t                       ErrCode;
	STGXOBJ_BitmapAllocParams_t BitmapAllocParams1;
    STGXOBJ_BitmapAllocParams_t BitmapAllocParams2;
	STAVMEM_BlockHandle_t   BlockHandle;	
    STAVMEM_AllocBlockParams_t          AllocBlockParams;
	
	STGXOBJ_Bitmap_t* Bitmap_p;
	
	memset((void *)&BitmapAllocParams1, 0,
		sizeof(STGXOBJ_BitmapAllocParams_t));
    memset((void *)&BitmapAllocParams2, 0,
		sizeof(STGXOBJ_BitmapAllocParams_t));
		
	Bitmap_p=&(pCH6BMP->Bitmap);

	
  	Bitmap_p->PreMultipliedColor=FALSE;
	Bitmap_p->ColorSpaceConversion=STGXOBJ_ITU_R_BT601;
	Bitmap_p->Width=BMPWidth;
    Bitmap_p->Height=BMPHeight;
	Bitmap_p->Pitch=0;
	Bitmap_p->Offset=0;
	Bitmap_p->Data1_p=NULL;
	Bitmap_p->Size1=0;
	Bitmap_p->Data2_p=NULL;
	Bitmap_p->Size2=0;
	Bitmap_p->SubByteFormat=STGXOBJ_SUBBYTE_FORMAT_RPIX_MSB;
	Bitmap_p->BigNotLittle=FALSE;


    memset((void *)&AllocBlockParams, 0, sizeof(STAVMEM_AllocBlockParams_t));
	

    AllocBlockParams.PartitionHandle = pCH6BMP->PartitionHandle;

    AllocBlockParams.AllocMode       = STAVMEM_ALLOC_MODE_BOTTOM_TOP;
    AllocBlockParams.NumberOfForbiddenBorders = 0;
    AllocBlockParams.ForbiddenBorderArray_p   = NULL;
	
    AllocBlockParams.NumberOfForbiddenRanges  = 0;
    AllocBlockParams.ForbiddenRangeArray_p    = NULL;
	
                                        
 
    /* Get Bitmap pitch and AVMEM alignement                         */
	
	ErrCode = STGXOBJ_GetBitmapAllocParams(Bitmap_p,
		GXOBJ_HARDUSE,
		&BitmapAllocParams1,
		&BitmapAllocParams2);
	if (ErrCode != ST_NO_ERROR)
	{
		STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
			"STGXOBJ_GetBitmapAllocParams()=%s",
			GetErrorText(ErrCode)));
	}
	
	Bitmap_p->Width=BMPWidth;
	Bitmap_p->Height=BMPHeight;
	Bitmap_p->Size1   = BitmapAllocParams1.AllocBlockParams.Size*1;
	Bitmap_p->Pitch   = BitmapAllocParams1.Pitch;
	Bitmap_p->Offset  = BitmapAllocParams1.Offset;
	Bitmap_p->Size2   = 0;
	Bitmap_p->Data2_p = NULL;
	AllocBlockParams.Size = Bitmap_p->Size1;
	AllocBlockParams.Alignment = BitmapAllocParams1.AllocBlockParams.Alignment;
	
	AllocBlockParams.Size += AllocBlockParams.Size % 4;
	/* Alignment is forced to 128 for 32 bits it is the worse    */
	/* constraint value. This is mandatory for GUTIL_Exchange.   */
	AllocBlockParams.Alignment = 4*8;
	
	/* Space for storing BlockHandle is done, allocate it        */
	ErrCode = STAVMEM_AllocBlock(&AllocBlockParams,
		(STAVMEM_BlockHandle_t *)&BlockHandle); 
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STAVMEM_AllocBlock()=%s",GetErrorText(ErrCode)));
		return TRUE;
	}
	
	pCH6BMP->BlockHandle1=BlockHandle;
	
	ErrCode=STAVMEM_GetBlockAddress(BlockHandle, (void **)&(Bitmap_p->Data1_p));
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STAVMEM_GetBlockAddress()=%s",GetErrorText(ErrCode)));
		return TRUE;
	}
	
	memset( Bitmap_p->Data1_p, 0x0, Bitmap_p->Size1);
	return FALSE;/*yxl 2007-02-02 add this line*/
}


/*yxl 2005-06-09 add below function*/
/*static BOOL CH_AllocStillBitmap(STLAYER_Handle_t LayerHandle,STAVMEM_PartitionHandle_t AVHandle,CH6_Bitmap_t* pCH6BMP,int BMPWidth,int BMPHeight)*/
/*yxl 2006-11-13 cancel paramter STAVMEM_PartitionHandle_t AVHandle,*/
static BOOL CH_AllocStillBitmap(STLAYER_Handle_t LayerHandle,CH6_Bitmap_t* pCH6BMP,int BMPWidth,int BMPHeight)
{

	ST_ErrorCode_t ErrCode;
	STAVMEM_AllocBlockParams_t    AllocParams;

	STAVMEM_BlockHandle_t   BlockHandle;



	STGXOBJ_Bitmap_t* pBMP;
	pBMP=&(pCH6BMP->Bitmap);
  



 
    pBMP->ColorSpaceConversion = STGXOBJ_ITU_R_BT601;
    pBMP->Width                = BMPWidth;
    pBMP->Height               = BMPHeight;
    pBMP->Pitch                = pBMP->Width * 2; /* 2bits per pixel */
    pBMP->Offset               = 0;
    if (pBMP->Pitch % 128 != 0)
    {
        pBMP->Pitch = pBMP->Pitch - (pBMP->Pitch % 128) + 128;
    }





	AllocParams.PartitionHandle =pCH6BMP->PartitionHandle;

    AllocParams.Size                     = pBMP->Pitch * pBMP->Height;
    AllocParams.Alignment                = 128;/*256*/;
    AllocParams.AllocMode                = STAVMEM_ALLOC_MODE_BOTTOM_TOP;
    AllocParams.NumberOfForbiddenRanges  = 0;
    AllocParams.ForbiddenRangeArray_p    = (void*)NULL;
    AllocParams.NumberOfForbiddenBorders = 0;
    AllocParams.ForbiddenBorderArray_p   = (void*)NULL;

 
	ErrCode=STAVMEM_AllocBlock((const STAVMEM_AllocBlockParams_t*)&AllocParams,
		(STAVMEM_BlockHandle_t *)&BlockHandle); 
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STAVMEM_AllocBlock()=%s",GetErrorText(ErrCode)));
		return TRUE;
	}
	pCH6BMP->BlockHandle1=BlockHandle;


	ErrCode=STAVMEM_GetBlockAddress(BlockHandle, (void **)&(pBMP->Data1_p));
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STAVMEM_GetBlockAddress()=%s",GetErrorText(ErrCode)));
		return TRUE;
	}

    if ( pBMP->BitmapType == STGXOBJ_BITMAP_TYPE_RASTER_PROGRESSIVE )
    {
        pBMP->Size1     = AllocParams.Size;
        pBMP->Data2_p   = NULL;
        pBMP->Size2     = 0;
    }
    else
    {
        pBMP->Size1     = AllocParams.Size/2;
        pBMP->Data2_p   = (U8*)pBMP->Data1_p + pBMP->Size1;
        pBMP->Size2     = pBMP->Size1;
    }


	return FALSE;

}
/*end yxl 2005-06-09 add below function*/



BOOL CH6_CopyStillData(STLAYER_ViewPortHandle_t VPHandle,char *PICFileName)
{

	ST_ErrorCode_t  ErrCode;
    STAVMEM_AllocBlockParams_t AllocParams;

    void * Unpitched_Data_p;
	short FileIndex;

    STAVMEM_BlockHandle_t BlockHandle;
    STAVMEM_FreeBlockParams_t FreeBlockParams;
	STGXOBJ_Bitmap_t BMPTemp;

	FileIndex=GetFileIndex(PICFileName);
	
	if(FileIndex==-1)
	{
		return true; 
	}

	if(GlobalFlashFileInfo[FileIndex].FileType!=YCrCb422) return true; 

#if 1 /*yxl 2005-06-07 reenable this section*/

	if(CH6_GetBitmapInfo(VPHandle,&BMPTemp))
	{
		return true;
	}
#else
	VPSource.Palette_p=&PaletteTemp;
    VPSource.SourceType=STLAYER_GRAPHIC_BITMAP;
	VPSource.Data.BitMap_p=&BMPTemp;
	ErrCode=STLAYER_GetViewPortSource(VPHandle,&VPSource);
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STLAYER_GetViewPortSource()=%s",GetErrorText(ErrCode)));
		return TRUE;
	}

#endif

	if(BMPTemp.ColorType!=STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422) return true;
    AllocParams.PartitionHandle          = AVMEMHandle;
	AllocParams.Size=BMPTemp.Size1+BMPTemp.Size2;

    AllocParams.Alignment                = 128;/*256*/

    AllocParams.AllocMode                = STAVMEM_ALLOC_MODE_BOTTOM_TOP;
    AllocParams.NumberOfForbiddenRanges  = 0;
    AllocParams.ForbiddenRangeArray_p    = (void*)NULL;
    AllocParams.NumberOfForbiddenBorders = 0;
    AllocParams.ForbiddenBorderArray_p   = NULL;

    STAVMEM_AllocBlock ((const STAVMEM_AllocBlockParams_t *) &AllocParams,
                               (STAVMEM_BlockHandle_t *) &(BlockHandle));
    ErrCode = STAVMEM_GetBlockAddress( BlockHandle, (void **)&(Unpitched_Data_p));
    if ( ErrCode != ST_NO_ERROR )
    {
        STTBX_Print(("STAVMEM_GetBlockAddress()=%s\n", GetErrorText(ErrCode)));
        return TRUE;
    }

#if 0 
   
	STTBX_DirectPrint(("Frame buffer loading , please wait ...\n"));

    strcpy (FileName, "../../Zdf.yuv");
    FileDescriptor = debugopen( (char *)FileName, "rb");
    if(FileDescriptor == -1)
    {
        STTBX_DirectPrint(("Unable to open file !!!\n"));

    }
     else
    {
   
        Size = (U32)debugread(FileDescriptor,(U8*)Unpitched_Data_p, AllocParams.Size);
        if ( Size != AllocParams.Size )
            STTBX_Print(("File not the same size (%d) as expected (%d)\n", Size, AllocParams.Size ));
        (void) debugclose(FileDescriptor);
    }
#else

#if 0
	BufTemp1=memory_allocate(CHGDPPartition,TEMPSIZE1);
	if(BufTemp1==NULL)
	{
		STTBX_Print(("\n YxlInfo(),can't allocate memory"));
	}
    STTBX_DirectPrint(("Frame buffer loading , please wait ...\n"));

    strcpy (FileName, "../../zipn.bin");
    FileDescriptor = debugopen( (char *)FileName, "rb");
    if(FileDescriptor == -1)
    {
        STTBX_DirectPrint(("Unable to open file !!!\n"));

    }
    else
    {
		
        Size = (U32)debugread(FileDescriptor,(U8*)BufTemp1, TEMPSIZE1);
        if ( Size != AllocParams.Size )
            STTBX_Print(("File not the same size (%d) as expected (%d)\n", Size, AllocParams.Size ));
        (void) debugclose(FileDescriptor);
    }
#endif
	
	
	if(GlobalFlashFileInfo[FileIndex].ZipSign!=0) /*when in zip*/
	{
		
#if 1
		
		CH6_Uncompress((U8*)GlobalFlashFileInfo[FileIndex].FlasdAddr, (U16*)Unpitched_Data_p,
			1536*576/*GlobalFlashFileInfo[FileIndex].iWidth*GlobalFlashFileInfo[FileIndex].iHeight*2*/);
#else
		CH6_Uncompress((U8*)BufTemp1, (U16*)Unpitched_Data_p,1536*576);
		
#endif
		
	}
	else 
	{

		memcpy((void*)Unpitched_Data_p,(const void*)GlobalFlashFileInfo[FileIndex].FlasdAddr,
		GlobalFlashFileInfo[FileIndex].FileLen);

	}

#endif

#if 1

#if 0
	STTBX_Print(("\n YxlInfo():A=%x %x %x B=%x %x %x C=%x %x %x \n",
	*(((U8*)(Unpitched_Data_p)+0)),*(((U8*)(Unpitched_Data_p)+1)),*(((U8*)(Unpitched_Data_p)+2)),
	*(((U8*)(BMPTemp.Data1_p)+0)),*(((U8*)(BMPTemp.Data1_p)+1)),*(((U8*)(BMPTemp.Data1_p)+2)),
	*(((U8*)(BMPTemp.Data2_p)+0)),*(((U8*)(BMPTemp.Data2_p)+1)),*(((U8*)(BMPTemp.Data2_p)+2))));

	STTBX_Print(("\n YxlInfo()A: file=%lx A=%lx B=%lx BW=%d BH=%d BPitch=%d\n",
		Unpitched_Data_p,BMPTemp.Data1_p,BMPTemp.Data2_p,BMPTemp.Width,BMPTemp.Height,
		BMPTemp.Pitch));


	

#endif

#if 1 /*yxl 2004-10-13 modify this section*/
	if( BMPTemp.Data2_p != NULL )
	{
		ErrCode=STAVMEM_CopyBlock2D ( (void *)((U8*)Unpitched_Data_p),
              BMPTemp.Width*2, BMPTemp.Height/2, BMPTemp.Pitch,
             (void *)BMPTemp.Data1_p, BMPTemp.Pitch );
	}
	else
	{

		ErrCode=STAVMEM_CopyBlock2D ( (void *)Unpitched_Data_p,
			BMPTemp.Width*2, BMPTemp.Height, BMPTemp.Pitch,
			(void *)BMPTemp.Data1_p, BMPTemp.Pitch );
		
		
		if ( ErrCode != ST_NO_ERROR )
		{
			STTBX_Print(("STAVMEM_CopyBlock2D(1)=%s\n", GetErrorText(ErrCode)));
			return TRUE;
		}
		
		ErrCode=STAVMEM_CopyBlock2D ( (void *)Unpitched_Data_p,
			BMPTemp.Width*2, BMPTemp.Height, BMPTemp.Pitch,
			(void *)BMPTemp.Data1_p, BMPTemp.Pitch );

	}

	if ( ErrCode != ST_NO_ERROR )
    {
        STTBX_Print(("STAVMEM_CopyBlock2D(1)=%s\n", GetErrorText(ErrCode)));
        return TRUE;
    }

	if ( BMPTemp.Data2_p != NULL ) 
	{

        ErrCode=STAVMEM_CopyBlock2D ( (void *)((U8*)Unpitched_Data_p+(AllocParams.Size/2)),
                                     BMPTemp.Width*2, BMPTemp.Height/2, BMPTemp.Pitch,
                                     (void *)BMPTemp.Data2_p, BMPTemp.Pitch );

		if( ErrCode != ST_NO_ERROR )
		{
			STTBX_Print(("STAVMEM_CopyBlock2D(2)=%s\n", GetErrorText(ErrCode)));
			return TRUE;
		}
	}

#else

	if( BMPTemp.Data2_p != NULL )
	{
		memcpy((void *)BMPTemp.Data1_p,(const void *)Unpitched_Data_p,AllocParams.Size/2);
	}

	else
	{
		memcpy((void *)BMPTemp.Data1_p,(const void *)Unpitched_Data_p,AllocParams.Size);
	}

	if ( BMPTemp.Data2_p != NULL ) 
	{
		memcpy((void *)BMPTemp.Data1_p,(const void *)((U8*)Unpitched_Data_p+(AllocParams.Size/2)),
			AllocParams.Size/2);
	}

#endif /*end yxl 2004-10-13 modify this section*/

#if 0

	time2=time_now();
	STTBX_Print(("\n YxlInfo(2):t1=%ld t2=%ld t=%ld",time1,time2,(time2-time1)));
#endif

#else

#if 0
    /* Copy from Temp to Display buffer */
    /* CopyBlock2D(Addr,Width,Height,Pitch, Addr,Pitch) */
     ErrCode=STAVMEM_CopyBlock2D ( (void *)Unpitched_Data_p,
                                 BMPTemp.Width*2, BMPTemp.Height/2, BMPTemp.Pitch,
                                 (void *)BMPTemp.Data1_p, BMPTemp.Pitch );
   if ( ErrCode != ST_NO_ERROR )
    {
        STTBX_Print(("STAVMEM_CopyBlock2D(1)=%s\n", GetErrorText(ErrCode)));
        return TRUE;
    }
			

    if ( BMPTemp.Data2_p != NULL )
         ErrCode=STAVMEM_CopyBlock2D ( (void *)((U8*)Unpitched_Data_p+(AllocParams.Size/2)),
                                     BMPTemp.Width*2, BMPTemp.Height/2, BMPTemp.Pitch,
                                     (void *)BMPTemp.Data2_p, BMPTemp.Pitch );

  if ( ErrCode != ST_NO_ERROR )
    {
        STTBX_Print(("STAVMEM_CopyBlock2D(2)=%s\n", GetErrorText(ErrCode)));
        return TRUE;
    }

#endif
  
#endif
  
#ifdef YXL_GRAPHICBASE_DEBUG
  STTBX_Print(("\n YxlInfo():A=%x %x %x B=%x %x %x C=%x %x %x \n",
	  *(((U8*)(Unpitched_Data_p)+0)),*(((U8*)(Unpitched_Data_p)+1)),*(((U8*)(Unpitched_Data_p)+2)),
	  *(((U8*)(BMPTemp.Data1_p)+0)),*(((U8*)(BMPTemp.Data1_p)+1)),*(((U8*)(BMPTemp.Data1_p)+2)),
	  *(((U8*)(BMPTemp.Data2_p)+0)),*(((U8*)(BMPTemp.Data2_p)+1)),*(((U8*)(BMPTemp.Data2_p)+2))));
#endif
  
  FreeBlockParams.PartitionHandle = AVMEMHandle;
  (void) STAVMEM_FreeBlock (&FreeBlockParams, &BlockHandle);
  
#ifdef YXL_GRAPHICBASE_DEBUG
  STTBX_Print(("\n YxlInfo(): file=%lx A=%lx B=%lx\n",
	  Unpitched_Data_p,BMPTemp.Data1_p,BMPTemp.Data2_p));
#endif
  
  
  return FALSE;
}


BOOL CH_AllocOSDPalette(STLAYER_Handle_t LayerHandle,CH6_Palette_t* pPalette)
{
    ST_ErrorCode_t ErrCode;
    STAVMEM_AllocBlockParams_t AllocParams;
    STAVMEM_BlockHandle_t BlockHandle;
    STGXOBJ_PaletteAllocParams_t PaletteParams;
    
    pPalette->Palette.ColorType  = STGXOBJ_COLOR_TYPE_UNSIGNED_AYCBCR6888_444;
    pPalette->Palette.ColorDepth = 8;
    
    STLAYER_GetPaletteAllocParams(LayerHandle, &(pPalette->Palette),&PaletteParams);

    AllocParams.PartitionHandle = pPalette->PartitionHandle;  

    AllocParams.Size = PaletteParams.AllocBlockParams.Size;
    AllocParams.Alignment = PaletteParams.AllocBlockParams.Alignment;
    AllocParams.AllocMode = STAVMEM_ALLOC_MODE_BOTTOM_TOP;
    AllocParams.NumberOfForbiddenRanges = 0;
    AllocParams.ForbiddenRangeArray_p = (STAVMEM_MemoryRange_t *)0;
    AllocParams.NumberOfForbiddenBorders = 0;
    AllocParams.ForbiddenBorderArray_p = (void **)0;
    STAVMEM_AllocBlock (&AllocParams, &(BlockHandle)); 
    ErrCode= STAVMEM_GetBlockAddress (BlockHandle, (void **)&(pPalette->Palette.Data_p));
    if ( ErrCode != ST_NO_ERROR )
    {
      STTBX_Print (("ErrCode allocating Palette : %s\n",GetErrorText(ErrCode)));
      return TRUE;
    }
	pPalette->BlockHandle=BlockHandle;


	return FALSE; 
}


/*BOOL CH_InitGraphic(STGXOBJ_Bitmap_t* pInitBitmap,STGXOBJ_Palette_t* pInitPalette)*/
/*yxl 2004-09-23 modify this function interface
BOOL CH_InitGraphic(CH6_Bitmap_t* pInitBitmap,STGXOBJ_Palette_t* pInitPalette)*/
/*yxl 2004-09-28 modify this function interface
static BOOL CH_InitGraphic(CH6_Bitmap_t* pInitBitmap,CH6_Palette_t* pInitPalette)*/
static BOOL CH_InitGraphic(STLAYER_Handle_t Handle,CH6_Bitmap_t* pInitBitmap,CH6_Palette_t* pInitPalette)
{

	int Width;
	int Height;
	BOOL res;

	if(pInitBitmap!=NULL)
	{
	Width=pInitBitmap->Bitmap.Width;
	Height=pInitBitmap->Bitmap.Height;

		
	if(pInitBitmap->Bitmap.ColorType==STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422)
	{
		res=CH_AllocStillBitmap(Handle,pInitBitmap,Width,Height);
	}
	else
	{
		res=CH_AllocOSDBitmap(Handle,pInitBitmap,Width,Height);
	}

	if(res==TRUE) return TRUE;
	}

	if(pInitPalette==NULL) return FALSE;


	res=CH_AllocOSDPalette(Handle,pInitPalette);

	if(res==TRUE) return TRUE;

	return FALSE;

}





static unsigned int FlashFontAddr=0;
static U8 stFlashFontType=0x10;/*yxl 2007-09-11 add */
static void CH6_InitFont(unsigned int FontBaseAdr)
{
#if 0	/*yxl 2007-09-11 modify below section*/ 
	FlashFontAddr=FontBaseAdr;
	return;
#else
	U8 ValTemp;
	ValTemp=*((U8*)FontBaseAdr);
	if(ValTemp==0x11||ValTemp==0x10)
	{
		FlashFontAddr=FontBaseAdr+1;
		stFlashFontType=0x11;
	}
	else
	{
		FlashFontAddr=FontBaseAdr;
		stFlashFontType=0x10;
	}
	return;
#endif/*end yxl 2007-09-11 modify below section*/ 

}


/*yxl 2007-09-11 add below function*/
unsigned long int CHDRV_OSD_GetFontBaseAddr(void)
{
	return FlashFontAddr;
}
/*end yxl 2007-09-11 add below function*/

/*yxl 2007-09-11 add below function*/
U8 CHDRV_OSD_GetFontLIBType(void)
{
	return stFlashFontType;
}
/*end yxl 2007-09-11 add below function*/

/*yxl 2007-09-11 add below function*/
U8* CHDRV_OSD_GetFontDisplayMem(STLAYER_ViewPortHandle_t VPHandle)
{
	U8* pDisMemTemp=NULL;
	pDisMemTemp=(U8*)CH6_FindMatchInnerParam(VPHandle,TYPE_FONT_DISPLAY_MEM);	
	
	return pDisMemTemp;
}
/*end yxl 2007-09-11 add below function*/


/*yxl 2007-02-09 add below function*/
BOOL CH_InitGraphicBase(CH_GraphicInitParams* pInitParams )
{
	
	BOOL res;

	if(pInitParams==NULL) return TRUE;

	if(pInitParams->pPartition==NULL) 
	{
		STTBX_Print(("\nYxlInfo: CH_InitGraphicBase() isn't successful,pInitParams->pPartition is NULL"));
		return TRUE;
	}
	else
	{
		CHGDPPartition=pInitParams->pPartition;
	}


	/*yxl 2007-07-16 add below section*/
#ifdef GRAPHIC_FOR_LOADER_USE
	if(pInitParams->PicStartAdr==NULL) 
{
		STTBX_Print(("\nYxlInfo:warning:picture address is NULL"));
return FALSE;
}
#endif
	/*end yxl 2007-07-16 add below section*/
	res=InitFlashFileInfo(pInitParams->PicStartAdr,pInitParams->PicMaxNameLen);
	if(res==TRUE) return TRUE;

	CH6_InitFont(pInitParams->FontStartAdr);

	return FALSE;

}
/*end yxl 2007-02-09 add below function*/


static BOOL InitFlashFileInfo(unsigned int PicStartAdr,int MaxPicNameLen)
{
	int i;
	unsigned short FileNum;

	unsigned int StartAddress;
	StartAddress=PicStartAdr;

	GlobalFlashFileHead=(FlashFileHead*)StartAddress;
   

	if(memcmp((void*)GlobalFlashFileHead->Magic,(const void*)"CHPC",4)!=0)
	{
		STTBX_Print(("\nYxlInfo:Pic in flash isn't valid"));
		return TRUE;
	}


	StartAddress+=sizeof(FlashFileHead);

	if(GlobalFlashFileHead->IsHavePallette!=0) 
		StartAddress=StartAddress+256*3;

	FileNum=GlobalFlashFileHead->FileNum;


	if(FileNum==0) 
	{
		STTBX_Print(("\nYxlInfo:no Pic in flash ,quit"));
		return TRUE;
	}


    GlobalFlashFileInfo=memory_allocate(CHGDPPartition,FileNum*sizeof(FlashFileStr));


	if(GlobalFlashFileInfo==NULL) return TRUE;/*yxl 2004-10-15 add this line*/

	memcpy((void *)GlobalFlashFileInfo,(void *)StartAddress,sizeof(FlashFileStr)*FileNum/*GlobalFlashFileHead->FileNum*/);

#ifdef YXL_GRAPHICBASE_DEBUG
	STTBX_Print(("\n YxlInfo():StartAddress=%lx ",StartAddress));
#endif

	for(i=0;i<FileNum;i++)
	{

		GlobalFlashFileInfo[i].FlasdAddr=GlobalFlashFileInfo[i].FlasdAddr+PicStartAdr;
		GlobalFlashFileInfo[i].FlashFIleName[MaxPicNameLen-1]='\0';

#ifdef YXL_GRAPHICBASE_DEBUG 
		STTBX_Print(("\n YxlInfo():i=%d offset=%lx name=%s W=%d h=%d Len=%d type=%d",
			i,GlobalFlashFileInfo[i].FlasdAddr,GlobalFlashFileInfo[i].FlashFIleName,
			GlobalFlashFileInfo[i].iWidth,GlobalFlashFileInfo[i].iHeight,
			GlobalFlashFileInfo[i].FileLen,GlobalFlashFileInfo[i].FileType));
#endif
	}

	return FALSE;
}




short GetFileIndex(char *filename)
{
	int i;

	for(i=0;i<GlobalFlashFileHead->FileNum;i++)
	{
    
		if(strcmp(filename,GlobalFlashFileInfo[i].FlashFIleName)==0)
		{
			return i;
		}
	}
	return -1;
}




BOOL CH6_GetBitmapInfo(U32 VPHandle,STGXOBJ_Bitmap_t* pBitmap)
{ 
	STGXOBJ_Bitmap_t*          pBitmapTemp;
	pBitmapTemp=(STGXOBJ_Bitmap_t*)CH6_FindMatchInnerParam(VPHandle,TYPE_CHBITMAP);

	if(pBitmapTemp==NULL) return TRUE;
	memcpy((void*)pBitmap,(const void*) pBitmapTemp,sizeof(STGXOBJ_Bitmap_t));
	return FALSE;
}

BOOL CH6_GetLayerBitmapInfo(U32 VPHandle,STGXOBJ_Bitmap_t* pBitmap)
{ 
	STGXOBJ_Bitmap_t*          pBitmapTemp;
	pBitmapTemp=(STGXOBJ_Bitmap_t*)CH6_FindMatchInnerParam(VPHandle,TYPE_LAYERBITMAP);
	if(pBitmapTemp==NULL) return TRUE;	
	memcpy((void*)pBitmap,(const void*) pBitmapTemp,sizeof(STGXOBJ_Bitmap_t));

	return FALSE;
}




#if 1/*yxl 2007-04-28 add below section,yxl 2007-07-03 add U32 Loops,U32 Priority*/
BOOL CHB_DisplayIFrame(int PosX,int PosY,
					char *PICFileName,CH_VIDCELL_t CHVIDCell,U32 Loops,U32 Priority)
{
	int SizeTemp=0;
	U8* pTemp=NULL; 
	short FileIndex;
	U32 LoopsTemp=Loops;
	U32 PriorityTemp=Priority;

	FileIndex=GetFileIndex(PICFileName);
	if(FileIndex==-1)
	{
		
		return true; 
	}
	if(GlobalFlashFileInfo[FileIndex].FileType==MPEGI)
	{
		pTemp=(U8 *)GlobalFlashFileInfo[FileIndex].FlasdAddr;
		SizeTemp=GlobalFlashFileInfo[FileIndex].FileLen;

		return CH7_FrameDisplay(pTemp,SizeTemp,CHGDPPartition,CHVIDCell,LoopsTemp,PriorityTemp);  

	}

}
#endif/*end yxl 2007-04-28 add below section*/


#if 1/*yxl 2007-07-04 add below section*/
BOOL CHB_DisplayIFrameFromMemory(U8* pData,U32 Size,CH_VIDCELL_t CHVIDCell,U32 Loops,U32 Priority)
{
	int SizeTemp=Size;
	U8* pDataTemp=pData; 
	U32 LoopsTemp=Loops;
	U32 PriorityTemp=Priority;

	if(pDataTemp==NULL||SizeTemp==0)
	{
		return TRUE;
	}

	return CH7_FrameDisplay(pDataTemp,SizeTemp,CHGDPPartition,CHVIDCell,
		LoopsTemp,PriorityTemp);  
	
}
#endif/*end yxl 2007-07-04 add below section*/


BOOL CH6_DisplayPIC(STGFX_Handle_t Handle,STLAYER_ViewPortHandle_t VPHandle,int PosX,int PosY,
					char *PICFileName,BOOL IsDisplayBackground)
{
	short FileIndex;
	int SizeTemp=0;
	void* pTemp=NULL; 
	FileIndex=GetFileIndex(PICFileName);
	if(FileIndex==-1)
	{
		
		return true; 
	}
	switch(GlobalFlashFileInfo[FileIndex].FileType)
	{
	case BMPM:
	case ARGB4444:
	case ARGB1555:
#ifdef YXL_ARGB8888 /*yxl 2007-09-03 add below section*/
	case ARGB8888:		
#endif
		{


			STGXOBJ_Bitmap_t OperatedBMP;
			if(CH6_GetBitmapInfo(VPHandle,&OperatedBMP))
			{
				return TRUE;
			}	
			if(GlobalFlashFileInfo[FileIndex].ZipSign==0) /*no zip*/
			{

				if(IsDisplayBackground)
					CH6_DrawBitmap(Handle, &OperatedBMP,PosX, PosY,GlobalFlashFileInfo[FileIndex].iWidth,GlobalFlashFileInfo[FileIndex].iHeight,
					(U8 *)GlobalFlashFileInfo[FileIndex].FlasdAddr,TRUE);
				else
					CH6_DrawBitmap(Handle, &OperatedBMP,PosX, PosY,GlobalFlashFileInfo[FileIndex].iWidth,GlobalFlashFileInfo[FileIndex].iHeight,
					(U8 *)GlobalFlashFileInfo[FileIndex].FlasdAddr,FALSE);

			}
			else/*in zip*/
			{
				SizeTemp=GlobalFlashFileInfo[FileIndex].iWidth*GlobalFlashFileInfo[FileIndex].iHeight;
				if(GlobalFlashFileInfo[FileIndex].FileType==ARGB4444||GlobalFlashFileInfo[FileIndex].FileType==ARGB1555) /*yxl 2007-04-27 add ARGB1555*/ 
				{
					SizeTemp+=SizeTemp;

				}
				/*yxl 2007-09-03 add below section*/
#ifdef YXL_ARGB8888
				else if(GlobalFlashFileInfo[FileIndex].FileType==ARGB8888) /*yxl 2007-04-27 add ARGB1555*/ 
				{
					SizeTemp=4*SizeTemp;
				}
#endif
				/*end yxl 2007-09-03 add below section*/
				pTemp=memory_allocate(CHGDPPartition,SizeTemp);
				if(pTemp==NULL) 
				{
					STTBX_Print(("\n YxlInfo():can't allocate memory in CH6_DisplayPIC"));
					return TRUE;
				}
				CH6_Uncompress((U8 *)GlobalFlashFileInfo[FileIndex].FlasdAddr,(U16*)pTemp,SizeTemp);
				
				if(IsDisplayBackground)
					CH6_DrawBitmap(Handle, &OperatedBMP,PosX, PosY,GlobalFlashFileInfo[FileIndex].iWidth,GlobalFlashFileInfo[FileIndex].iHeight,
					(U8 *)pTemp,TRUE);

				else
					CH6_DrawBitmap(Handle, &OperatedBMP,PosX, PosY,GlobalFlashFileInfo[FileIndex].iWidth,GlobalFlashFileInfo[FileIndex].iHeight,
					(U8 *)pTemp,FALSE);

				if(pTemp!=NULL) memory_deallocate(CHGDPPartition,pTemp);
				
			}

		}

		break;
#if 0
	case    BMPS:
		    CH_ShowIcon(Handle,
			PosX, PosY,GlobalFlashFileInfo[FileIndex].iWidth,GlobalFlashFileInfo[FileIndex].iHeight,
			index,
			(U8 *)GlobalFlashFileInfo[FileIndex].FlasdAddr);
		break;
	case    GIF:
		    DrawGifGraph(Handle,(U8 *)GlobalFlashFileInfo[FileIndex].FlasdAddr, PosX , PosY );
		break;
#endif
	case    MPEGI:
		
#ifdef FOR_CH_QAMI5516_USE /*yxl 2005-11-09 add this section*/
		CH6_ShowI((U8 *)GlobalFlashFileInfo[FileIndex].FlasdAddr,GlobalFlashFileInfo[FileIndex].FileLen);
#else/*end yxl 2005-11-09 add this section*/

	CH7_FrameDisplay((U8 *)GlobalFlashFileInfo[FileIndex].FlasdAddr,GlobalFlashFileInfo[FileIndex].FileLen);

#endif /*yxl 2005-11-09 add this line*/
			break;		

	}
	return false;
}



BOOL CH6_DisplayPICByData(STGFX_Handle_t Handle,STLAYER_ViewPortHandle_t VPHandle,int PosX,int PosY,
					int Width,int Height,U8* pData,BOOL IsDisplayBackground)
{

	STGXOBJ_Bitmap_t OperatedBMP;
	if(CH6_GetBitmapInfo(VPHandle,&OperatedBMP))
	{
		return TRUE;
	}	
	if(IsDisplayBackground)
		CH6_DrawBitmap(Handle, &OperatedBMP,PosX, PosY,Width,Height,
			pData,TRUE);
	else
		CH6_DrawBitmap(Handle, &OperatedBMP,PosX, PosY,Width,Height,
			pData,FALSE);

	return FALSE;

}


BOOL sw_copyrect(STGXOBJ_Bitmap_t*     SrcBitmap_p,
                             STGXOBJ_Rectangle_t*  SrcRectangle_p,
                             STGXOBJ_Bitmap_t*     DstBitmap_p,
                             int                   DstPositionX,
                             int                   DstPositionY,int BitsPerPixel )
{
    S32 DLeft, DRight, DTop, DBottom, SLeft, STop;
  
    U8  *SData_p, *DData_p;
    U32 BytesPerLine;
    U32 Tmp1, Tmp2;
    U32 NbLine;
	int Bpp;
	int y;
	ST_ErrorCode_t ErrCode;

    SLeft    = SrcRectangle_p->PositionX;
    DLeft    = DstPositionX;
    DRight   = DLeft + SrcRectangle_p->Width;
    STop     = SrcRectangle_p->PositionY;
    DTop     = DstPositionY;
    DBottom  = DTop + SrcRectangle_p->Height;
  
	Bpp=BitsPerPixel;
    if (DRight <= DLeft || DBottom <= DTop) 
	{
      
        return TRUE;
    }

    Tmp1 =  (U32)SrcBitmap_p->Data1_p;
    Tmp2 =  (U32)DstBitmap_p->Data1_p;

    /* Work out the pointer to the first byte and the byte width to copy */
    SData_p = ((U8 *)Tmp1) + SrcBitmap_p->Offset + STop * SrcBitmap_p->Pitch + (SLeft << (Bpp >> 4));
    DData_p = ((U8 *)Tmp2) + DstBitmap_p->Offset + DTop * DstBitmap_p->Pitch + (DLeft << (Bpp >> 4));
    BytesPerLine = (DRight - DLeft) << (Bpp >> 4);

    NbLine  = DBottom - DTop;

#if 1 /*yxl 2004-10-26 modify this section,yxl 2007-07-06 temp modify,yxl 2007-07-31*/  
    ErrCode=STAVMEM_CopyBlock2D((void*)SData_p,
                        BytesPerLine,
                        DBottom - DTop,
                        (U32)SrcBitmap_p->Pitch,
                        (void*)DData_p,(U32)DstBitmap_p->Pitch);
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("\nYxlInfo:STAVMEM_CopyBlock2D=%s",GetErrorText(ErrCode)));
	}

#else

    for (y = 0; y < NbLine; y++)
    {
        memcpy((void *) (DData_p + (DstBitmap_p->Pitch * y)), (void *) (SData_p + (SrcBitmap_p->Pitch * y)), SrcBitmap_p->Pitch);
    }
#endif/*end yxl 2004-10-26 modify this section*/  

	return FALSE;
}

#if 0 /*yxl 2009-04-23 modify below section for 图片的ALPHA显示由外部函数CH_SLLK_GetAlpha()控制 ,
主要是特殊市场的需要*/

static BOOL CH6_DrawBitmap(STGFX_Handle_t Handle,STGXOBJ_Bitmap_t* pOperBitmap,int PosX,int PosY,int BMPWidth,int BMPHeight,U8* pBMPData,BOOL IsDisplayBack)
{
	ST_ErrorCode_t ErrCode;
	STGXOBJ_Bitmap_t* pDstBMP;
	STGXOBJ_Bitmap_t SrcBMP;
	STGFX_GC_t* pGCTemp;
	void* pTemp=NULL;
	int i;
	int Bits;
	BOOL IsDisplayBackGround;
	STGXOBJ_Rectangle_t Rect;
	
	BOOL SignTemp=TRUE;
    
	U8 AlphaTemp;
	BOOL AlphaNeedUpdate=TRUE;
	U8 ValTemp;

	U8 DataMaskTemp=0xff;/*yxl 2007-08-14 add this variable*/

	pDstBMP=pOperBitmap;
	IsDisplayBackGround=IsDisplayBack;/*false;*/

	gGC.EnableFilling=TRUE;
	pGCTemp=&gGC;

    AlphaNeedUpdate=CH_SLLK_GetAlpha(&AlphaTemp); 

	memcpy((void*)&SrcBMP,(const void*)pDstBMP,sizeof(STGXOBJ_Bitmap_t));

	SrcBMP.Width=BMPWidth;
	SrcBMP.Height=BMPHeight;

	SrcBMP.Offset=0;

	switch(SrcBMP.ColorType)
	{
		case STGXOBJ_COLOR_TYPE_CLUT8:
			SrcBMP.Pitch=BMPWidth;
			SrcBMP.Size1=SrcBMP.Width*SrcBMP.Height;
			Bits=8;
			break;
		case STGXOBJ_COLOR_TYPE_ARGB1555:
		case STGXOBJ_COLOR_TYPE_ARGB4444:
		case STGXOBJ_COLOR_TYPE_RGB565:
			SrcBMP.Pitch=BMPWidth*2;
			SrcBMP.Size1=SrcBMP.Width*SrcBMP.Height*2;
			Bits=16;

			if(SrcBMP.ColorType==STGXOBJ_COLOR_TYPE_ARGB1555)
			{
				DataMaskTemp=0x7f;
				AlphaTemp=AlphaTemp<<7;
			}
			else if(SrcBMP.ColorType==STGXOBJ_COLOR_TYPE_ARGB4444) /*yxl 2009-04-27 modify from ARGB1555 to ARGB4444,this is a bug*/
			{
				DataMaskTemp=0x0f;
				AlphaTemp=AlphaTemp<<4;
			}
			else 
			{
				DataMaskTemp=0xff;
				AlphaNeedUpdate=FALSE;
			}

			break;
#ifdef YXL_ARGB8888  
		case STGXOBJ_COLOR_TYPE_ARGB8888:
			SrcBMP.Pitch=BMPWidth*4;
			SrcBMP.Size1=SrcBMP.Width*SrcBMP.Height*4;
			Bits=32;
			break;
#endif 
		default:
			return TRUE;
	}


    if(IsDisplayBackGround==false)	
	{
		if(AlphaNeedUpdate==FALSE)
		{
			SrcBMP.Data1_p=(void*)pBMPData;
			SignTemp=FALSE;
		}

	}
	
	if(SignTemp==TRUE)  /*   else*/
	{
		pTemp=memory_allocate(CHGDPPartition,SrcBMP.Size1);

		if(pTemp==NULL) 
		{
			return TRUE;
		}

		if(IsDisplayBackGround==TRUE)
		{
			CH6_GetRectangleA(Handle, pOperBitmap,PosX, PosY, SrcBMP.Width, SrcBMP.Height, pTemp);
		}

		switch(SrcBMP.ColorType)
		{
			case STGXOBJ_COLOR_TYPE_CLUT8:
				for(i=0;i<SrcBMP.Size1;i++)
				{
					if(*((U8*)pBMPData+i)!=0) 
						*((U8*)pTemp+i)=*((U8*)pBMPData+i);
				}
				
				break;
			case STGXOBJ_COLOR_TYPE_ARGB1555:
			case STGXOBJ_COLOR_TYPE_ARGB4444:
			case STGXOBJ_COLOR_TYPE_RGB565:
				
				for(i=0;i<SrcBMP.Size1/2;i++)
				{
					if(IsDisplayBackGround==TRUE)
					{
						if((((*((U8*)pBMPData+2*i+1))&DataMaskTemp)==0)&&((*((U8*)pBMPData+2*i))==0)) 
						{
							continue;
						}
					}
					
					*((U8*)pTemp+2*i)=*((U8*)pBMPData+2*i);


					if(AlphaNeedUpdate)
					{
						
						ValTemp=(*((U8*)pBMPData+2*i+1)&DataMaskTemp);
						*((U8*)pTemp+2*i+1)=ValTemp|AlphaTemp;

					}
					else
					{
						*((U8*)pTemp+2*i+1)=*((U8*)pBMPData+2*i+1);
					}

				}

				break;
#ifdef YXL_ARGB8888  
			case STGXOBJ_COLOR_TYPE_ARGB8888:
				{

					for(i=0;i<SrcBMP.Size1/4;i++)
					{

						if(IsDisplayBackGround==TRUE)
						{
							if(((*((U8*)pBMPData+4*i+2))==0)&&((*((U8*)pBMPData+4*i+1))==0)&&((*((U8*)pBMPData+4*i))==0)) 
							{
								continue;
							}
						}
						
						if(AlphaNeedUpdate==FALSE)
						{
							*((U8*)pTemp+4*i)=*((U8*)pBMPData+4*i);
						}
						else
						{
                            *((U8*)pTemp+4*i)=AlphaTemp;
						}

						*((U8*)pTemp+4*i+1)=*((U8*)pBMPData+4*i+1);
						*((U8*)pTemp+4*i+2)=*((U8*)pBMPData+4*i+2);
						*((U8*)pTemp+4*i+3)=*((U8*)pBMPData+4*i+3);
						
					}
				}
				break;
#endif 
				
			default:
				goto exit1;
		}

		SrcBMP.Data1_p=(void*)pTemp;
		
	}
	


	Rect.PositionX=0;
	Rect.PositionY=0;
	Rect.Width=SrcBMP.Width;
	Rect.Height=SrcBMP.Height;







	ErrCode=sw_copyrect(&SrcBMP,&Rect,pDstBMP,PosX,PosY,Bits);

	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STGFX_CopyArea(E)=%s",GetErrorText(ErrCode)));
		goto exit1;/*return TRUE;*/
	}

	if(pTemp!=NULL) memory_deallocate(CHGDPPartition,pTemp);


	return FALSE;	
exit1:
	if(pTemp!=NULL) memory_deallocate(CHGDPPartition,pTemp);
	return TRUE;
 
}

#else /*2009-04-24,below is org*/

static BOOL CH6_DrawBitmap(STGFX_Handle_t Handle,STGXOBJ_Bitmap_t* pOperBitmap,int PosX,int PosY,int BMPWidth,int BMPHeight,U8* pBMPData,BOOL IsDisplayBack)
{
	ST_ErrorCode_t ErrCode;
	STGXOBJ_Bitmap_t* pDstBMP;
	STGXOBJ_Bitmap_t SrcBMP;
	STGFX_GC_t* pGCTemp;
	void* pTemp=NULL;
	int i;
	int Bits;
	BOOL IsDisplayBackGround;
	STGXOBJ_Rectangle_t Rect;
	U8 DataMaskTemp=0xff;/*yxl 2007-08-14 add this variable*/

	pDstBMP=pOperBitmap;
	IsDisplayBackGround=IsDisplayBack;/*false;*/

	gGC.EnableFilling=TRUE;
	pGCTemp=&gGC;


	memcpy((void*)&SrcBMP,(const void*)pDstBMP,sizeof(STGXOBJ_Bitmap_t));

	SrcBMP.Width=BMPWidth;
	SrcBMP.Height=BMPHeight;

	SrcBMP.Offset=0;

	switch(SrcBMP.ColorType)
	{
		case STGXOBJ_COLOR_TYPE_CLUT8:
			SrcBMP.Pitch=BMPWidth;
			SrcBMP.Size1=SrcBMP.Width*SrcBMP.Height;
			Bits=8;
			break;
		case STGXOBJ_COLOR_TYPE_ARGB1555:
		case STGXOBJ_COLOR_TYPE_ARGB4444:
		case STGXOBJ_COLOR_TYPE_RGB565:
			SrcBMP.Pitch=BMPWidth*2;
			SrcBMP.Size1=SrcBMP.Width*SrcBMP.Height*2;
			Bits=16;

			if(SrcBMP.ColorType==STGXOBJ_COLOR_TYPE_ARGB1555)
			{
				DataMaskTemp=0x7f;
			}
			else if(SrcBMP.ColorType==STGXOBJ_COLOR_TYPE_ARGB4444) /*yxl 2009-04-27 modify from ARGB1555 to ARGB4444,this is a bug*/
			{
				DataMaskTemp=0x0f;
			
			}
			else 
			{
				DataMaskTemp=0xff;

			}
			break;
#ifdef YXL_ARGB8888  /*yxl 2007-08-14 add*/
		case STGXOBJ_COLOR_TYPE_ARGB8888:
			SrcBMP.Pitch=BMPWidth*4;
			SrcBMP.Size1=SrcBMP.Width*SrcBMP.Height*4;
			Bits=32;
			break;
#endif 
		default:
			return TRUE;
	}

    if(IsDisplayBackGround==false)	
	{
		SrcBMP.Data1_p=(void*)pBMPData;
	}
	else
	{
		pTemp=memory_allocate(CHGDPPartition,SrcBMP.Size1);

		if(pTemp==NULL) 
		{
			STTBX_Print(("\nYxlInfo 2005-11-24  ->11"));		
			return TRUE;
		}
		CH6_GetRectangleA(Handle, pOperBitmap,PosX, PosY, SrcBMP.Width, SrcBMP.Height, pTemp);
		switch(SrcBMP.ColorType)
		{
			case STGXOBJ_COLOR_TYPE_CLUT8:
				for(i=0;i<SrcBMP.Size1;i++)
				{
					if(*((U8*)pBMPData+i)!=0) 
						*((U8*)pTemp+i)=*((U8*)pBMPData+i);
				}
				
				break;
			case STGXOBJ_COLOR_TYPE_ARGB1555:
			case STGXOBJ_COLOR_TYPE_ARGB4444:
			case STGXOBJ_COLOR_TYPE_RGB565:
				
				for(i=0;i<SrcBMP.Size1/2;i++)
				{

					if((((*((U8*)pBMPData+2*i+1))&DataMaskTemp)==0)&&((*((U8*)pBMPData+2*i))==0)) continue;
					
					*((U8*)pTemp+2*i)=*((U8*)pBMPData+2*i);
					*((U8*)pTemp+2*i+1)=*((U8*)pBMPData+2*i+1);
					
				}
			
				break;
#ifdef YXL_ARGB8888  /*yxl 2007-08-14 add*/
			case STGXOBJ_COLOR_TYPE_ARGB8888:
				{
					for(i=0;i<SrcBMP.Size1/4;i++)
					{
						if(((*((U8*)pBMPData+4*i+2))==0)&&((*((U8*)pBMPData+4*i+1))==0)&&((*((U8*)pBMPData+4*i))==0)) continue;
						
						*((U8*)pTemp+4*i)=*((U8*)pBMPData+4*i);
						*((U8*)pTemp+4*i+1)=*((U8*)pBMPData+4*i+1);
						*((U8*)pTemp+4*i+2)=*((U8*)pBMPData+4*i+2);
						*((U8*)pTemp+4*i+3)=*((U8*)pBMPData+4*i+3);
						
					}
				}
				break;
#endif 
				
			default:
				goto exit1;
		}
		SrcBMP.Data1_p=(void*)pTemp;
		
	}
	
	Rect.PositionX=0;
	Rect.PositionY=0;
	Rect.Width=SrcBMP.Width;
	Rect.Height=SrcBMP.Height;

	ErrCode=sw_copyrect(&SrcBMP,&Rect,pDstBMP,PosX,PosY,Bits);

	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STGFX_CopyArea(F)=%s",GetErrorText(ErrCode)));
		goto exit1;
	}


	if(pTemp!=NULL) memory_deallocate(CHGDPPartition,pTemp);


	return FALSE;	
exit1:
	if(pTemp!=NULL) memory_deallocate(CHGDPPartition,pTemp);
	return TRUE;
 
}


#endif


#ifdef GRAPHIC_FOR_LOADER_USE/*yxl 2007-07-16 add below section*/
#ifndef GRAPHIC_LOADER_ONLY_ENG /*yxl 2007-11-22 add this line*/

/*
  * Func: GetCodeOffset
  * Desc: 根据字的内码得到字符起始地址
  * Out:  -1 		-> 没有找到该字
  	      其他	-> 字模起始地址
  */
static int GetCodeOffset( U16 iCode )
{
#if 0
	/* z.q 修改该偏移量计算到Font.h 汉字字库*/
	
	int    i,k,fondFlag;
	U16  hz_code,hz_code_;
	
	i = 0;
	k = 0;
	fondFlag=0;
	hz_code = iCode;
	
	while(hz_index_[k])
	{
		hz_code_ = hz_index_[k];
		hz_code_ <<= 8;
		hz_code_ &= 0xff00;
		hz_code_ |= hz_index_[k+1];
		if(hz_code_ == hz_code )
		{
			fondFlag = 1;
			break;
		}
		k += 2;
	}
	
	if( 1 == fondFlag )
	{
		return k/2;
	}
	else
	{
		return -1;
	}
#else
	/* z.q 修改该偏移量计算到Font.h 汉字字库*/
	
	int    i,k,fondFlag;
	U16  hz_code,hz_code_;
	U8 ValTemp1;
	U8 ValTemp2;
	
	i = 0;
	k = 0;
	fondFlag=0;
	hz_code = iCode;
	
	while(hz_index_[k])
	{
		ValTemp1=(U8)hz_index_[k];
		ValTemp2=(U8)hz_index_[k+1];
		hz_code_ =ValTemp1*256+ValTemp2;
		if(hz_code_ == hz_code )
		{
			fondFlag = 1;
			break;
		}
		k += 2;
	}
	
	if( 1 == fondFlag )
	{
		return k/2;
	}
	else
	{
		return -1;
	}

#endif

}
#endif /*end yxl 2007-07-16 add below section*/
#endif /*yxl 2007-11-22 add this line*/


#ifdef GRAPHIC_FOR_LOADER_USE/*yxl 2007-07-16 add below section*/
#if 0 /* yxl 2009-06-15 cancel below section*/
BOOL CH6_LDR_TextOut(STGFX_Handle_t Handle,STLAYER_ViewPortHandle_t VPHandle,int PosX,int PosY,unsigned int TextCorlorIndex,
				 unsigned char* pTextString) /*yxl 2007-03-05 add unsigned */
{
    STGXOBJ_Bitmap_t* pOperatedBMP;
	U8* pTemp=NULL;
    int curX=PosX;
	int curY=PosY;

	int itemp;
	unsigned int offset;/*yxl 2007-03-05 add unsigned */
	int FontWidth=FONT_SIZE;
	int FontHeight=FONT_SIZE;
	int length = strlen(pTextString);
	U8* pFontData=NULL;/*yxl 2007-03-05 add =NULL*/
	U8* pDisMemTemp=NULL;/*yxl 2007-03-05 add =NULL*/


	unsigned char* pTempString=pTextString;/*yxl 2007-03-05 add unsigned */


	pTemp=memory_allocate(CHGDPPartition,sizeof(STGXOBJ_Bitmap_t));
	if(pTemp==NULL)
	{
		STTBX_Print(("YxlInfo(%s,%dL):can't allocate memory",__FILE__,__LINE__));
		return TRUE;
	}
	pOperatedBMP=(STGXOBJ_Bitmap_t*)pTemp;
	if(CH6_GetBitmapInfo(VPHandle,pOperatedBMP))
	{
		return TRUE;
	}

	pDisMemTemp=(U8*)CH6_FindMatchInnerParam(VPHandle,TYPE_FONT_DISPLAY_MEM);


	for(itemp = 0; itemp < length; itemp++)
	{
		if(pTempString[itemp] & 0x80)
		{
#ifndef GRAPHIC_LOADER_ONLY_ENG /*yxl 2007-11-22 add this line*/
			offset = GetCodeOffset( pTempString[itemp] * 256 + pTempString[itemp+1] );

			if( offset >= 0 )
			{
				pFontData = (void*)(offset*(24 *24 / 8) +hz_lib);
				CH6_DrawFont(Handle,pOperatedBMP, curX, curY,FontWidth,FontHeight,TextCorlorIndex,pFontData,pDisMemTemp);
	
			}
			
			curX+=FONT_SIZE;
			itemp++;
#endif /*yxl 2007-11-22 add this line*/
		}
		else
		{
			if (pTempString[itemp] == LEFT_ARROW || pTempString[itemp] == RIGHT_ARROW 
					||  pTempString[itemp] == UP_ARROW || pTempString[itemp] == DOWN_ARROW )
			{

				/*  显示▼▲左右*/
				if ( pTempString[itemp] == LEFT_ARROW )
					pFontData = (U8*)( cjm_enfont[94] );
				else if ( pTempString[itemp] == RIGHT_ARROW )
					pFontData = (U8*)( cjm_enfont[95] );
				else if (pTempString[itemp] == UP_ARROW )
					pFontData = (U8*)( cjm_enfont[96] );
				else if (pTempString[itemp] == DOWN_ARROW )
					pFontData = (U8*)( cjm_enfont[97] );
				
				CH6_DrawFont(Handle,pOperatedBMP, curX, curY,FontWidth,FontHeight,TextCorlorIndex,pFontData,pDisMemTemp);
				
				curX+=FONT_SIZE;
			}
			else
			{
				if(pTempString[itemp] >= 0x21 && pTempString[itemp] < 0x7f)
				{
					pFontData = (void*)(cjm_enfont[pTempString[itemp]-0x21]);
					
					CH6_DrawFont(Handle, pOperatedBMP,curX, curY,FontWidth,FontHeight,TextCorlorIndex,pFontData,pDisMemTemp);
				}
#ifdef ENGLISH_FONT_MODE_EQUEALWIDTH /*yxl 2006-03-06 modify this section,yxl 2006-04-22 modify 0 to ifdef ENGLISH_FONT_MODE_EQUEALWIDTH */
				curX+=FONT_SIZE/2;
#else
				if(pTempString[itemp] >= 0x21 && pTempString[itemp] < 0x7f)
				{
					curX+= cModeWidth[pTempString[itemp]-0x21];
				}
				else
				{
					curX+=FONT_SIZE/2-2;
				}
#endif /*end yxl 2006-03-06 modify this section*/
				
			}
		}
	}
	if(pTemp!=NULL) memory_deallocate(CHGDPPartition,pTemp);
	return FALSE;
}
#endif  /* yxl 2009-06-15 cancel below section*/

#endif /*end yxl 2007-07-16 add below section*/


#if 1/* yxl 2007-09-05 modify this section,yxl 2007-04-25*/

#ifndef USE_UNICODE  /*yxl 2007-09-11 add this line*/
BOOL CH6_TextOut(STGFX_Handle_t Handle,STLAYER_ViewPortHandle_t VPHandle,int PosX,int PosY,unsigned int TextCorlorIndex,
				 /*unsigned*/ char* pTextString) /*yxl 2007-03-05 add unsigned */
{
    STGXOBJ_Bitmap_t* pOperatedBMP;
	U8* pTemp=NULL;
    int curX=PosX;
	int curY=PosY;

	int itemp;
	unsigned int offset;/*yxl 2007-03-05 add unsigned */
	int FontWidth=FONT_SIZE;
	int FontHeight=FONT_SIZE;
	int length = strlen((const char*)pTextString);
	U8* pFontData=NULL;/*yxl 2007-03-05 add =NULL*/
	U8* pDisMemTemp=NULL;/*yxl 2007-03-05 add =NULL*/

	/*unsigned*/ char* pTempString=pTextString;/*yxl 2007-03-05 add unsigned */


	pTemp=memory_allocate(CHGDPPartition,sizeof(STGXOBJ_Bitmap_t));
	if(pTemp==NULL)
	{
		STTBX_Print(("YxlInfo(%s,%dL):can't allocate memory",__FILE__,__LINE__));
		return TRUE;
	}
	pOperatedBMP=(STGXOBJ_Bitmap_t*)pTemp;
	if(CH6_GetBitmapInfo(VPHandle,pOperatedBMP))
	{
		return TRUE;
	}

	pDisMemTemp=(U8*)CH6_FindMatchInnerParam(VPHandle,TYPE_FONT_DISPLAY_MEM);


	for(itemp = 0; itemp < length; itemp++)
	{
		if(pTempString[itemp] & 0x80)
		{
			if(pTempString[itemp] >= 0xb0 && pTempString[itemp+1] >= 0xa1)	/*yxl 2004-11-10 modify 0x18c90 to 60912*/
				offset = 60912/*0x18c90*/+ FlashFontAddr + (((pTempString [ itemp ] - 0xb0)&0x7f) * 94 + ((pTempString[ itemp + 1]-0xa1) & 0x7f))*72;
			else if(pTempString[itemp] >= 0xa1 && pTempString[itemp+1] >= 0xa1)	
				offset = FlashFontAddr+(((pTempString [ itemp ] - 0xa1)&0x7f) * 94 + ((pTempString[ itemp + 1]-0xa1) & 0x7f))*72;
			else
				offset = FlashFontAddr;

			pFontData = (void*)offset;

	
			CH6_DrawFont(Handle,pOperatedBMP, curX, curY,FontWidth,FontHeight,TextCorlorIndex,pFontData,pDisMemTemp);


			curX+=FONT_SIZE;
			itemp++;
		}
		else
		{
			if ( pTempString[itemp] == LEFT_ARROW || pTempString[itemp] == RIGHT_ARROW )
			{

				if ( pTempString[itemp] == LEFT_ARROW )
					pFontData= (void*)(cjm_enfont[94]);
				else
					pFontData = (void*)(cjm_enfont[95]);

				CH6_DrawFont(Handle,pOperatedBMP, curX, curY,FontWidth,FontHeight,TextCorlorIndex,pFontData,pDisMemTemp);


				curX+=FONT_SIZE;
			}
			else
			{
				if(pTempString[itemp] >= 0x21 && pTempString[itemp] < 0x7f)
				{
					pFontData = (void*)(cjm_enfont[pTempString[itemp]-0x21]);

					CH6_DrawFont(Handle, pOperatedBMP,curX, curY,FontWidth,FontHeight,TextCorlorIndex,pFontData,pDisMemTemp);
				}
#ifdef ENGLISH_FONT_MODE_EQUEALWIDTH /*yxl 2006-03-06 modify this section,yxl 2006-04-22 modify 0 to ifdef ENGLISH_FONT_MODE_EQUEALWIDTH */
				curX+=FONT_SIZE/2;
#else
				if(pTempString[itemp] >= 0x21 && pTempString[itemp] < 0x7f)
				{
					curX+= cModeWidth[pTempString[itemp]-0x21];
				}
				else
				{
					curX+=FONT_SIZE/2-2;
				}
#endif /*end yxl 2006-03-06 modify this section*/
			}
		}
	}
	if(pTemp!=NULL) memory_deallocate(CHGDPPartition,pTemp);
	return FALSE;
}

#endif /* yxl 2007-09-11 add this line*/


#else

BOOL CH6_TextOut(STGFX_Handle_t Handle,STLAYER_ViewPortHandle_t VPHandle,int PosX,int PosY,int TextCorlorIndex,
				 unsigned char* pTextString) /*yxl 2007-03-05 add unsigned */
{
    STGXOBJ_Bitmap_t* pOperatedBMP;
	U8* pTemp=NULL;
    int curX=PosX;
	int curY=PosY;

	int itemp;
	unsigned int offset;/*yxl 2007-03-05 add unsigned */
	int FontWidth=FONT_SIZE;
	int FontHeight=FONT_SIZE;
	int length = strlen(pTextString);
	U8* pFontData=NULL;/*yxl 2007-03-05 add =NULL*/
	U8* pDisMemTemp=NULL;/*yxl 2007-03-05 add =NULL*/


	unsigned char* pTempString=pTextString;/*yxl 2007-03-05 add unsigned */


	pTemp=memory_allocate(CHGDPPartition,sizeof(STGXOBJ_Bitmap_t));
	if(pTemp==NULL)
	{
		STTBX_Print(("YxlInfo(%s,%dL):can't allocate memory",__FILE__,__LINE__));
		return TRUE;
	}
	pOperatedBMP=(STGXOBJ_Bitmap_t*)pTemp;
	if(CH6_GetBitmapInfo(VPHandle,pOperatedBMP))
	{
		return TRUE;
	}

	pDisMemTemp=(U8*)CH6_FindMatchInnerParam(VPHandle,TYPE_FONT_DISPLAY_MEM);


	for(itemp = 0; itemp < length; itemp++)
	{
		if(pTempString[itemp] & 0x80)
		{
			if(pTempString[itemp] >= 0xb0 && pTempString[itemp+1] >= 0xa1)	/*yxl 2004-11-10 modify 0x18c90 to 60912*/
				offset = 60912/*0x18c90*/+ FlashFontAddr + (((pTempString [ itemp ] - 0xb0)&0x7f) * 94 + ((pTempString[ itemp + 1]-0xa1) & 0x7f))*72;
			else if(pTempString[itemp] >= 0xa1 && pTempString[itemp+1] >= 0xa1)	
				offset = FlashFontAddr+(((pTempString [ itemp ] - 0xa1)&0x7f) * 94 + ((pTempString[ itemp + 1]-0xa1) & 0x7f))*72;
			else
				offset = FlashFontAddr;
			
			pFontData = (void*)offset;
			

			CH6_DrawFont(Handle,pOperatedBMP, curX, curY,FontWidth,FontHeight,TextCorlorIndex,pFontData,pDisMemTemp);
			
				
			curX+=FONT_SIZE;
			itemp++;
		}
		else
		{
			if ( pTempString[itemp] == LEFT_ARROW || pTempString[itemp] == RIGHT_ARROW )
			{

				if ( pTempString[itemp] == LEFT_ARROW )
					pFontData= (void*)(cjm_enfont[94]);
				else
					pFontData = (void*)(cjm_enfont[95]);

				CH6_DrawFont(Handle,pOperatedBMP, curX, curY,FontWidth,FontHeight,TextCorlorIndex,pFontData,pDisMemTemp);


				curX+=FONT_SIZE;
			}
			else
			{
				if(pTempString[itemp] >= 0x21 && pTempString[itemp] < 0x7f)
				{
					pFontData = (void*)(cjm_enfont[pTempString[itemp]-0x21]);

					CH6_DrawFont(Handle, pOperatedBMP,curX, curY,FontWidth,FontHeight,TextCorlorIndex,pFontData,pDisMemTemp);
				}
#ifdef ENGLISH_FONT_MODE_EQUEALWIDTH /*yxl 2006-03-06 modify this section,yxl 2006-04-22 modify 0 to ifdef ENGLISH_FONT_MODE_EQUEALWIDTH */
				curX+=FONT_SIZE/2;
#else
				if(pTempString[itemp] >= 0x21 && pTempString[itemp] < 0x7f)
				{
					curX+= cModeWidth[pTempString[itemp]-0x21];
				}
				else
				{
					curX+=FONT_SIZE/2-2;
				}
#endif /*end yxl 2006-03-06 modify this section*/
			}
		}
	}
	if(pTemp!=NULL) memory_deallocate(CHGDPPartition,pTemp);
	return FALSE;
}


#endif /*end yxl 2007-08-13 modify this section*/



/*static*/ BOOL CH6_DrawFont(STGFX_Handle_t Handle,STGXOBJ_Bitmap_t* pOperBitmap,int PosX,int PosY,int FontWidth,
						 int FontHeight,unsigned int Color,U8* pFontData,U8* pDisMem)
{

	STGXOBJ_Bitmap_t* pBMPTemp;
	STGXOBJ_Bitmap_t SrcBMP;
	STGFX_GC_t* pGCTemp;


	BOOL IsDisplayBackGround=false;

	STGXOBJ_Rectangle_t Rect;

	U32            x, y;
	S32            xShift = 0;
	S32            yShift = 0;
	int rowL;
	U8* pTemp;
	U8 ValueTemp;

	U8* pFontDTemp=pFontData;


	int FontWTemp=FontWidth;
	int FontHTemp=FontHeight;
	int SizeTemp=0;
	int Bits;
	U8* p1;
	U8 ColorTemp[4];

	SizeTemp=FontWidth*FontHeight;
	pBMPTemp=pOperBitmap;
	p1=(U8*)&Color;

	ColorTemp[0]=*p1; 
	ColorTemp[1]=*(p1+1);
	ColorTemp[2]=*(p1+2);
	ColorTemp[3]=*(p1+3);



	memcpy((void*)&SrcBMP,(const void*)pBMPTemp,sizeof(STGXOBJ_Bitmap_t));


	SrcBMP.Width=FontWTemp;
	SrcBMP.Height=FontHTemp;

	SrcBMP.Offset=0;

	switch(pBMPTemp->ColorType)
	{
		case STGXOBJ_COLOR_TYPE_CLUT8:
			SizeTemp=FontWidth*FontHeight;
			SrcBMP.Pitch=SrcBMP.Width;
			SrcBMP.Size1=SizeTemp;
			Bits=8;
	
			break;
		case STGXOBJ_COLOR_TYPE_ARGB1555:
		case STGXOBJ_COLOR_TYPE_ARGB4444:
		case STGXOBJ_COLOR_TYPE_RGB565:
			SizeTemp=2*FontWidth*FontHeight;
			SrcBMP.Pitch=SrcBMP.Width*2;
			SrcBMP.Size1=SizeTemp/*2*/;
			Bits=16;
			break;
#ifdef YXL_ARGB8888  /*yxl 2007-08-14 add*/
		case STGXOBJ_COLOR_TYPE_ARGB8888:
			SizeTemp=4*FontWidth*FontHeight;
			SrcBMP.Pitch=SrcBMP.Width*4;
			SrcBMP.Size1=SizeTemp;
			Bits=32;
			break;
#endif 
		default:
			STTBX_Print(("\nYxlInfo(A):ColorType not supported"));
			return TRUE;
	}

	pTemp=pDisMem;

	rowL = (FontWTemp)/8 + ((FontWTemp)%8 ? 1 : 0);

	CH6_GetRectangleA(Handle, pBMPTemp,PosX, PosY, FontWTemp, FontHTemp, pTemp);
  

    for(y=0; y<FontHTemp; y++)
    {  
		for(x=0; x<FontWTemp; x++)
        {
			ValueTemp=*((U8*)(pFontDTemp+(x+xShift)/8 + (y+yShift)*rowL));
			if((pFontDTemp[(x+xShift)/8 + (y+yShift)*rowL] >> (7-((x+xShift)%8))) & 0x1) /*2916,gfont*/
			{
				switch(pBMPTemp->ColorType)
				{
					case STGXOBJ_COLOR_TYPE_CLUT8:
						*(pTemp+x+y*FontWTemp)=ColorTemp[0];
						break;
					case STGXOBJ_COLOR_TYPE_ARGB1555:
					case STGXOBJ_COLOR_TYPE_ARGB4444:
					case STGXOBJ_COLOR_TYPE_RGB565:
						*(pTemp+2*x+y*FontWTemp*2)=ColorTemp[0];
						*(pTemp+2*x+y*FontWTemp*2+1)=ColorTemp[1];

						break;
					#ifdef YXL_ARGB8888  /*yxl 2007-08-14 add*/
					case STGXOBJ_COLOR_TYPE_ARGB8888:

						*(pTemp+4*x+y*FontWTemp*4)=ColorTemp[0];
						*(pTemp+4*x+y*FontWTemp*4+1)=ColorTemp[1];
						*(pTemp+4*x+y*FontWTemp*4+2)=ColorTemp[2];
						*(pTemp+4*x+y*FontWTemp*4+3)=ColorTemp[3];
						break;
					#endif 

					default:
						STTBX_Print(("\nYxlInfo():ColorType not supported"));
						return TRUE;
				}
			}
        }
    }

	IsDisplayBackGround=false;

    if(IsDisplayBackGround==false)	SrcBMP.Data1_p=(void*)pTemp;

	Rect.PositionX=0;
	Rect.PositionY=0;
	Rect.Width=SrcBMP.Width;
	Rect.Height=SrcBMP.Height;

	sw_copyrect(&SrcBMP,&Rect,pBMPTemp,PosX,PosY,Bits);

	return FALSE;	
}




static BOOL CH6_GetRectangleA(STGFX_Handle_t Handle, STGXOBJ_Bitmap_t* pOperBitmap,int PosX, int PosY, int Width, int Height, 
				 U8* pBMPData)
{
  	ST_ErrorCode_t ErrCode;
  	
  	STGXOBJ_Bitmap_t* pOSDBMPTemp;

	STGXOBJ_Bitmap_t BMPTemp;
	STGFX_GC_t* pGCTemp;
	STGXOBJ_Rectangle_t RectTemp;
	int Bits;

	pOSDBMPTemp=pOperBitmap;
	gGC.EnableFilling=TRUE;
	pGCTemp=&gGC;

	memcpy((void*)&BMPTemp,(const void*)pOSDBMPTemp,sizeof(STGXOBJ_Bitmap_t));


	BMPTemp.Width=Width;
	BMPTemp.Height=Height;
	BMPTemp.Offset=0;
	BMPTemp.Data1_p=pBMPData;


	switch(BMPTemp.ColorType)
	{
		case STGXOBJ_COLOR_TYPE_CLUT8:
			BMPTemp.Pitch=Width;
			BMPTemp.Size1=BMPTemp.Width*BMPTemp.Height;
			Bits=8;/*yxl 2005-09-23 add this line*/

			break;
		case STGXOBJ_COLOR_TYPE_ARGB1555:
		case STGXOBJ_COLOR_TYPE_ARGB4444:
		case STGXOBJ_COLOR_TYPE_RGB565:
			BMPTemp.Pitch=Width*2;
			BMPTemp.Size1=BMPTemp.Width*BMPTemp.Height*2;
			Bits=16;/*yxl 2005-09-23 add this line*/
			break;
		#ifdef YXL_ARGB8888  /*yxl 2007-08-14 add*/
		case STGXOBJ_COLOR_TYPE_ARGB8888:
			BMPTemp.Pitch=Width*4;
			BMPTemp.Size1=BMPTemp.Width*BMPTemp.Height*4;
			Bits=32;
			break;
		#endif 
		default:
			return TRUE;
	}


	RectTemp.PositionX=PosX;
	RectTemp.PositionY=PosY;

	RectTemp.Width=Width;
	RectTemp.Height=Height;

#if 0 /*yxl 2005-09-23 modfiy this section,yxl 2007-06-15 modify below section*/	 
	ErrCode=STGFX_CopyArea(Handle,pOSDBMPTemp,&BMPTemp,pGCTemp,&RectTemp,0,0);
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("CH6_GetRectangle()=%s",GetErrorText(ErrCode)));
		return TRUE;
	}
	ErrCode=STGFX_Sync(Handle);
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STGFX_Sync()=%s",GetErrorText(ErrCode)));
		return TRUE;
	}

#else

	ErrCode=sw_copyrect(pOSDBMPTemp,&RectTemp,&BMPTemp,0,0,Bits);
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("CH6_GetRectangleA()=%s",GetErrorText(ErrCode)));
		return TRUE;
	}


#endif /*end yxl 2005-09-23 modfiy this section*/

	return FALSE;
}




U8* CH6_GetRectangle(STGFX_Handle_t Handle, STLAYER_ViewPortHandle_t VPHandle,STGFX_GC_t* pGC,int PosX, int PosY, int Width, int Height)
{
  	ST_ErrorCode_t ErrCode;
  	
	STGXOBJ_Bitmap_t OSDBMPTemp;

	STGXOBJ_Bitmap_t BMPTemp;
	STGFX_GC_t* pGCTemp;
	STGXOBJ_Rectangle_t RectTemp;
	int Bits;/*yxl 2005-09-23 add this line*/

	U8* pTemp=NULL;

	pGCTemp=pGC;

	if(CH6_GetBitmapInfo(VPHandle,&OSDBMPTemp))
	{
		return NULL;		
	}	

	memcpy((void*)&BMPTemp,(const void*)&OSDBMPTemp,sizeof(STGXOBJ_Bitmap_t));

	BMPTemp.Width=Width;
	BMPTemp.Height=Height;
	BMPTemp.Offset=0;

	switch(BMPTemp.ColorType)
	{
		case STGXOBJ_COLOR_TYPE_CLUT8:
			BMPTemp.Pitch=Width;
			BMPTemp.Size1=BMPTemp.Width*BMPTemp.Height;
			Bits=8;/*yxl 2005-09-23 add this line*/
			break;
		case STGXOBJ_COLOR_TYPE_ARGB1555:
		case STGXOBJ_COLOR_TYPE_ARGB4444:
		case STGXOBJ_COLOR_TYPE_RGB565:
			BMPTemp.Pitch=Width*2;
			BMPTemp.Size1=BMPTemp.Width*BMPTemp.Height*2;	
			Bits=16;/*yxl 2005-09-23 add this line*/
			break;
		#ifdef YXL_ARGB8888  /*yxl 2007-08-14 add*/
		case STGXOBJ_COLOR_TYPE_ARGB8888:
			BMPTemp.Pitch=Width*4;
			BMPTemp.Size1=BMPTemp.Width*BMPTemp.Height*4;	
			Bits=32;
			break;
		#endif 

		default:
			STTBX_Print(("\n YxlInfo():not supported color type \n"));

		return NULL;		
	}

	pTemp=memory_allocate(CHGDPPartition,BMPTemp.Size1);
	if(pTemp==NULL) 
	{
		return NULL;		
	}

	BMPTemp.Data1_p=pTemp;

	RectTemp.PositionX=PosX;
	RectTemp.PositionY=PosY;

	RectTemp.Width=Width;
	RectTemp.Height=Height;


#if 1 /*yxl 2005-09-21 modify this section*/
	ErrCode=STGFX_CopyArea(Handle,&OSDBMPTemp,&BMPTemp,pGCTemp,&RectTemp,0,0);
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("CH6_GetRectangle()=%s",GetErrorText(ErrCode)));
		goto exit1;
	}
	ErrCode=STGFX_Sync(Handle);
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STGFX_Sync()=%s",GetErrorText(ErrCode)));
		goto exit1;
	}
#else

	ErrCode=sw_copyrect(&OSDBMPTemp,&RectTemp,&BMPTemp,0,0,Bits);
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STGFX_CopyArea()=%s",GetErrorText(ErrCode)));
		goto exit1;
	}

#endif /*end yxl 2005-09-21 modify this section*/



	return pTemp;
exit1:
	if(pTemp!=NULL) memory_deallocate(CHGDPPartition,pTemp);
	pTemp=NULL;

	return pTemp;
}


/*yxl 2004-11-10 add paramter STGFX_GC_t*/
BOOL CH6_PutRectangle(STGFX_Handle_t Handle, STLAYER_ViewPortHandle_t VPHandle,STGFX_GC_t* pGC,
					  int PosX, int PosY,int Width,int Height,U8* pBMPData)
{
  	ST_ErrorCode_t ErrCode;
  	STGXOBJ_Bitmap_t OSDBMPTemp;/*STGXOBJ_Bitmap_t* pOSDBMPTemp;*/
	STGXOBJ_Bitmap_t BMPTemp;
	STGFX_GC_t* pGCTemp;
	STGXOBJ_Rectangle_t RectTemp;
	int Bits;/*yxl 2005-09-23 add this line*/


	pGCTemp=pGC;

	pGCTemp->EnableFilling=TRUE;
	if(CH6_GetBitmapInfo(VPHandle,&OSDBMPTemp))
	{
		return TRUE;
	}

	memcpy((void*)&BMPTemp,(const void*)&OSDBMPTemp,sizeof(STGXOBJ_Bitmap_t));

	BMPTemp.Width=Width;
	BMPTemp.Height=Height;
	BMPTemp.Offset=0;

	BMPTemp.Data1_p=pBMPData;

	switch(BMPTemp.ColorType)
	{
		case STGXOBJ_COLOR_TYPE_CLUT8:
			BMPTemp.Pitch=Width;
			BMPTemp.Size1=BMPTemp.Width*BMPTemp.Height;
			Bits=8;/*yxl 2005-09-23 add this line*/
			break;
		case STGXOBJ_COLOR_TYPE_ARGB1555:
		case STGXOBJ_COLOR_TYPE_ARGB4444:
		case STGXOBJ_COLOR_TYPE_RGB565:
			BMPTemp.Pitch=Width*2;
			BMPTemp.Size1=BMPTemp.Width*BMPTemp.Height*2;
			Bits=16;/*yxl 2005-09-23 add this line*/
			break;
		#ifdef YXL_ARGB8888  /*yxl 2007-08-14 add*/
		case STGXOBJ_COLOR_TYPE_ARGB8888:
			BMPTemp.Pitch=Width*4;
			BMPTemp.Size1=BMPTemp.Width*BMPTemp.Height*4;
			Bits=32;
			break;
		#endif 

		default:
			return TRUE;
	}


	RectTemp.PositionX=0;
	RectTemp.PositionY=0;

	RectTemp.Width=BMPTemp.Width;
	RectTemp.Height=BMPTemp.Height;

	ErrCode=STGFX_CopyArea(Handle,&BMPTemp,&OSDBMPTemp,pGCTemp,&RectTemp,PosX,PosY);

	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("CH6_PutRectangle()=%s",GetErrorText(ErrCode)));
		return TRUE;
	}

	ErrCode=STGFX_Sync(Handle);
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STGFX_Sync()=%s",GetErrorText(ErrCode)));
		return TRUE;
	}



	return FALSE;
}




BOOL CH6_ShowViewPort(CH_ViewPort_t* pCH6VPHandle)
{
  	ST_ErrorCode_t Err;
	BOOL StatusTemp;
	int i;

	if(CH6_GetViewPortDisplayStatus(pCH6VPHandle,&StatusTemp)) return TRUE;

	if(StatusTemp==TRUE) return TRUE;

	if(pCH6VPHandle->OSDLayerCount==0) return TRUE;

	for(i=0;i<pCH6VPHandle->OSDLayerCount;i++)
	{
		
		Err=STLAYER_EnableViewPort(pCH6VPHandle->LayerViewPortHandle[i]);
		if(Err!=ST_NO_ERROR)
		{
			STTBX_Print(("CH6_ShowViewPort()=%s",GetErrorText(Err)));
			return TRUE;
		}
	}

	StatusTemp=TRUE;
	if(CH6_SetViewPortDisplayStatus(pCH6VPHandle,&StatusTemp)) return TRUE;

	return FALSE;
}




BOOL CH6_HideViewPort(CH_ViewPort_t* pCH6VPHandle)
{
 
  	ST_ErrorCode_t Err;
	BOOL StatusTemp;
	int i;

	if(CH6_GetViewPortDisplayStatus(pCH6VPHandle,&StatusTemp)) return TRUE;

	if(StatusTemp==FALSE) return TRUE;

	if(pCH6VPHandle->OSDLayerCount==0) return TRUE;

	for(i=0;i<pCH6VPHandle->OSDLayerCount;i++)
	{
		Err=STLAYER_DisableViewPort(pCH6VPHandle->LayerViewPortHandle[i]);
		if(Err!=ST_NO_ERROR)
		{
			STTBX_Print(("CH6_HideViewPort()=%s",GetErrorText(Err)));
			return TRUE;
		}
	}

	StatusTemp=false;
	if(CH6_SetViewPortDisplayStatus(pCH6VPHandle,&StatusTemp)) return TRUE;
	return FALSE;	

}


BOOL CH6_GetViewPortDisplayStatus(CH_ViewPort_t* pCH6VPHandle,BOOL* pDisplayStatus)
{
	STLAYER_ViewPortHandle_t VPHandleTemp;
	int i;
	BOOL StatusTemp;

	VPHandleTemp=(STLAYER_ViewPortHandle_t)pCH6VPHandle->ViewPortHandle;

	for(i=0;i<MAX_OSDVIEWPORT_NUMBER;i++)
	{
		if(CH6_VPInner_Params[i].CHVPHandle==VPHandleTemp)
		break;
	}
	if(i>=MAX_OSDVIEWPORT_NUMBER) return TRUE;

	if(CH6_VPInner_Params[i].LayerCount==0) return TRUE;

	StatusTemp=CH6_VPInner_Params[i].LayerInnerParams[0].VPDisStatus;
	*pDisplayStatus=StatusTemp;

	return FALSE;
	
}


static BOOL CH6_SetViewPortDisplayStatus(CH_ViewPort_t* pCH6VPHandle/*STLAYER_ViewPortHandle_t VPHandle*/,BOOL* pDisplayStatus)
{
	STLAYER_ViewPortHandle_t VPHandleTemp;
	int i,j;
	BOOL StatusTemp=*pDisplayStatus;
	
	VPHandleTemp=(STLAYER_ViewPortHandle_t)pCH6VPHandle->ViewPortHandle;

	for(i=0;i<MAX_OSDVIEWPORT_NUMBER;i++)
	{
		if(CH6_VPInner_Params[i].CHVPHandle==VPHandleTemp)
		break;
	}
	if(i>=MAX_OSDVIEWPORT_NUMBER) return TRUE;

	if(CH6_VPInner_Params[i].LayerCount==0) return TRUE;

	for(j=0;j<CH6_VPInner_Params[i].LayerCount;j++)
	{
		CH6_VPInner_Params[i].LayerInnerParams[j].VPDisStatus=StatusTemp;
	}

	return FALSE;
	
}



BOOL CH6_ClearArea(STGFX_Handle_t GHandle,STLAYER_ViewPortHandle_t VPHandle,int PosX,int PosY,int Width,int Height)
{

	ST_ErrorCode_t ErrCode;
	STGXOBJ_Bitmap_t OSDBMPTemp;
	STGXOBJ_Bitmap_t SrcBMP;
	STGFX_GC_t* pGCTemp;

	void* pTemp;

	STGXOBJ_Rectangle_t Rect;

	gGC.EnableFilling=TRUE;
	pGCTemp=&gGC;

	if(CH6_GetBitmapInfo(VPHandle,&OSDBMPTemp))
	{
		return TRUE;
	}

	memcpy((void*)&SrcBMP,(const void*)&OSDBMPTemp,sizeof(STGXOBJ_Bitmap_t));

	SrcBMP.Width=Width;
	SrcBMP.Height=Height;
	SrcBMP.Offset=0;

	switch(SrcBMP.ColorType)
	{
		case STGXOBJ_COLOR_TYPE_CLUT8:
	
			SrcBMP.Pitch=Width;
			SrcBMP.Size1=Width*Height;
			break;
		case STGXOBJ_COLOR_TYPE_ARGB1555:
		case STGXOBJ_COLOR_TYPE_ARGB4444:
		case STGXOBJ_COLOR_TYPE_RGB565:
			SrcBMP.Pitch=Width*2;
			SrcBMP.Size1=Width*Height*2;
			break;
		#ifdef YXL_ARGB8888  /*yxl 2007-08-14 add*/
		case STGXOBJ_COLOR_TYPE_ARGB8888:
			SrcBMP.Pitch=Width*4;
			SrcBMP.Size1=Width*Height*4;
			break;
		#endif 
		default:
			return TRUE;

	}

	if(SrcBMP.ColorType==STGXOBJ_COLOR_TYPE_CLUT8)
	{
		pTemp=memory_allocate(CHGDPPartition,SrcBMP.Size1);
		if(pTemp==NULL) return TRUE;
		SrcBMP.Data1_p=(void*)pTemp;
	
		memset(pTemp,0,SrcBMP.Size1);
		Rect.PositionX=0;
		Rect.PositionY=0;

		Rect.Width=Width;
		Rect.Height=Height;

		ErrCode=STGFX_CopyArea(GHandle,&SrcBMP,&OSDBMPTemp,pGCTemp,&Rect,PosX,PosY);
		if(ErrCode!=ST_NO_ERROR)
		{
			STTBX_Print(("STGFX_CopyArea(A)=%s",GetErrorText(ErrCode)));
			return TRUE;
		}

		ErrCode=STGFX_Sync(GHandle);
		if(ErrCode!=ST_NO_ERROR)
		{
			STTBX_Print(("STGFX_Sync()=%s",GetErrorText(ErrCode)));
			free(pTemp);
			return TRUE;
		}

		memory_deallocate(CHGDPPartition,pTemp);
		return FALSE;
	}
	else
	{

		CH6_DrawRectangle(GHandle,&OSDBMPTemp,PosX,PosY,SrcBMP.Width,SrcBMP.Height,1,0x0,0x0);

	}

}


BOOL CH6_ClearFullArea(STGFX_Handle_t GHandle,STLAYER_ViewPortHandle_t VPHandle)
{
	STGXOBJ_Bitmap_t OSDBMPTemp;

	if(CH6_GetBitmapInfo(VPHandle,&OSDBMPTemp))
	{
		return TRUE;
	}

	if(OSDBMPTemp.Data1_p!=NULL) 
{
        memset( OSDBMPTemp.Data1_p, 0x0, OSDBMPTemp.Size1);
}
	if(OSDBMPTemp.Data2_p!=NULL) 
{
			memset( OSDBMPTemp.Data2_p, 0x0/*0x0f*/, OSDBMPTemp.Size2);
}

	return FALSE;
}



BOOL CH6_SetColorValue(STGXOBJ_Color_t* pColor,unsigned int Value)
{
	STGXOBJ_Color_t* pColorTemp;
	U8* p1;
	U8 Temp[4];
	pColorTemp=pColor;

	p1=(U8*)&Value;

	Temp[0]=*p1;
	Temp[1]=*(p1+1);
	Temp[2]=*(p1+2);
	Temp[3]=*(p1+3);

	switch(pColorTemp->Type)
	{
		case STGXOBJ_COLOR_TYPE_CLUT8:

			pColorTemp->Value.CLUT8=(U8)Temp[0];
			break;
		case STGXOBJ_COLOR_TYPE_ARGB1555:

			pColorTemp->Value.ARGB1555.Alpha=Temp[1]>>7;
		
			pColorTemp->Value.ARGB1555.R=(Temp[1]&0x7c)>>2;

			pColorTemp->Value.ARGB1555.G=((Temp[1]&0x03)<<3)+((Temp[0]&0xe0)>>5);

			pColorTemp->Value.ARGB1555.B=(Temp[0]&0x1f);
			break;
		case STGXOBJ_COLOR_TYPE_ARGB4444:
			pColorTemp->Value.ARGB4444.Alpha=Temp[1]>>4;
			pColorTemp->Value.ARGB4444.R=Temp[1]&0x0f;
			pColorTemp->Value.ARGB4444.G=(Temp[0]&0xf0)>>4;
			pColorTemp->Value.ARGB4444.B=Temp[0]&0x0f;
			pColorTemp->Value.ARGB4444.Alpha=Temp[1]>>4;
			break;
		case STGXOBJ_COLOR_TYPE_RGB565:

			pColorTemp->Value.RGB565.R=(Temp[1]&0xf1)>>3;
			pColorTemp->Value.RGB565.G=((Temp[1]&0x07)<<3)+((Temp[0]&0xe0)>>5);
			pColorTemp->Value.RGB565.B=Temp[0]&0x1f;
			break;
		#ifdef YXL_ARGB8888  /*yxl 2007-08-14 add*/
		case STGXOBJ_COLOR_TYPE_ARGB8888:
			pColorTemp->Value.ARGB8888.Alpha=Temp[3];
			pColorTemp->Value.ARGB8888.R=Temp[2];
			pColorTemp->Value.ARGB8888.G=Temp[1];
			pColorTemp->Value.ARGB8888.B=Temp[0];
			break;
		#endif 
		/*case STGXOBJ_COLOR_TYPE_ARGB888:
			pColorTemp->Value.ARGB888.Alpha=Temp[3];
			pColorTemp->Value.ARGB888.R=Temp[2];
			pColorTemp->Value.ARGB888.G=Temp[1];
			pColorTemp->Value.ARGB888.B=Temp[0];
			break;
		case STGXOBJ_COLOR_TYPE_RGB888:
			pColorTemp->Value.RGB888.R=Temp[2];
			pColorTemp->Value.RGB888.G=Temp[1];
			pColorTemp->Value.RGB888.B=Temp[0];
			break;
		case STGXOBJ_COLOR_TYPE_ARGB8565:
			pColorTemp->Value.ARGB8565.Alpha=Temp[3];
			pColorTemp->Value.ARGB8565.R=(Temp[1]&0xf1)>>3;
			pColorTemp->Value.ARGB8565.G=(Temp[1]&0x03)<<2+(Temp[0]&0xe0)>>5;
			pColorTemp->Value.ARGB8565.B=Temp[0]&0x1f;
			break;
			*/
		default:
			STTBX_Print(("\n YxlInfo(A):unsupported color type"));
			return TRUE;

	}
	return FALSE;
}



/*2004-10-15 yxl add paramter STGXOBJ_Bitmap_t* pDstBMP*/
BOOL CH6_DrawRectangle(STGFX_Handle_t Handle,STGXOBJ_Bitmap_t* pDstBMP,int PosX,int PosY,int Width,int Height,int BorderW,
					   unsigned int BorderColor,unsigned int FillColor)
{
	ST_ErrorCode_t ErrCode;
	STGXOBJ_Rectangle_t Rect;

	STGFX_GC_t* pGCTemp;
	gGC.EnableFilling=TRUE;

	
	pGCTemp=&gGC;
	
   pGCTemp->FillColor.Type=pDstBMP->ColorType;
   pGCTemp->DrawColor.Type=pDstBMP->ColorType;


   pGCTemp->LineWidth=BorderW;	
   if(CH6_SetColorValue(&(pGCTemp->FillColor),FillColor)) return false;
   if(CH6_SetColorValue(&(pGCTemp->DrawColor),BorderColor)) return false;

	Rect.PositionX=PosX;
	Rect.PositionY=PosY;
	Rect.Width=Width;
	Rect.Height=Height;

	ErrCode=STGFX_DrawRectangle(Handle,pDstBMP,pGCTemp,Rect,0);
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STGFX_DrawRectangle(C)=%s Handle=%lx",GetErrorText(ErrCode),Handle));
		return TRUE;
	}
 
	
	return FALSE;
}




BOOL CH6_CreateViewPort(CH6_ViewPortParams_t* Params,CH_ViewPort_t* pCH6VPHandle,CH_DrawInfo_t RectInfo)
{
	ST_ErrorCode_t ErrCode;
	CH6_Palette_t PaletteTemp;
	STLAYER_ViewPortSource_t VPSource;
	STLAYER_ViewPortParams_t VPParams;
	STLAYER_ViewPortHandle_t VPHandleTemp;
	STLAYER_GlobalAlpha_t Alpha;
	int IndexTemp;
	int i;
	U8* pTemp;
	CH6_Bitmap_t BitmapTempInner;
	unsigned short CHVPNumTemp=RectInfo.DrawVPCount;

	if(pCH6VPHandle==NULL||Params==NULL)
	{
		STTBX_Print(("\nYxlInfo:Init Param is NULL,return"));
		return TRUE;
	}

	if(Params->ViewPortCount==0||Params->ViewPortCount>MAX_LAYERVIEWPORT_NUMBER)
	{
		STTBX_Print(("\nYxlInfo:Params is wrong,return "));
		return TRUE;
	}



	if(CHVPNumTemp==0||CHVPNumTemp>MAX_CHBITMAP_COUNT) 
	{
		STTBX_Print(("\nYxlInfo:Params is wrong,return "));
		return TRUE;
	}



	CH6_InitInnerParam(MAX_OSDVIEWPORT_NUMBER);

	IndexTemp=CH6_GetFreeParam();
	if(IndexTemp==-1) return TRUE;


	memset((void*)pCH6VPHandle,0,sizeof(CH_ViewPort_t));


	for(i=0;i<CHVPNumTemp;i++)
	{
		memset((void*)&BitmapTempInner,0,sizeof(CH6_Bitmap_t));
		
		BitmapTempInner.Bitmap.Width=RectInfo.DrawViewPort[i].Width;
		BitmapTempInner.Bitmap.Height=RectInfo.DrawViewPort[i].Height;

		BitmapTempInner.Bitmap.ColorType=Params->ViewPortParams[0].ColorType;
		BitmapTempInner.Bitmap.BitmapType=Params->ViewPortParams[0].BitmapType;
		BitmapTempInner.PartitionHandle=Params->ViewPortParams[0].ViewportPartitionHandle;
		BitmapTempInner.Bitmap.AspectRatio=Params->ViewPortParams[0].AspectRatio;
		BitmapTempInner.BlockHandle1=STAVMEM_INVALID_BLOCK_HANDLE;

		PaletteTemp.PartitionHandle=Params->ViewPortParams[0].ViewportPartitionHandle;

		switch(BitmapTempInner.Bitmap.ColorType)
		{
		case STGXOBJ_COLOR_TYPE_ARGB4444:
		case STGXOBJ_COLOR_TYPE_ARGB1555:
		case STGXOBJ_COLOR_TYPE_RGB565:
		case STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422: 
		#ifdef YXL_ARGB8888  /*yxl 2007-08-14 add*/
		case STGXOBJ_COLOR_TYPE_ARGB8888:
		#endif 

			if(CH_InitGraphic((STLAYER_Handle_t)NULL,&BitmapTempInner,(CH6_Palette_t*)NULL)) 
			{
				STTBX_Print(("CH_InitGraphic() is unsuccessful\n"));
				return TRUE;
			}
			VPSource.Palette_p=NULL;
			CH6_SetInnerParam(IndexTemp,TYPE_CHBITMAP,(void*)&BitmapTempInner);

			CH6_SetInnerParam(IndexTemp,TYPE_CHPALETTE,(void*)NULL);

			break;
		case STGXOBJ_COLOR_TYPE_CLUT8:
		case STGXOBJ_COLOR_TYPE_CLUT4:
		case STGXOBJ_COLOR_TYPE_CLUT2:
		case STGXOBJ_COLOR_TYPE_CLUT1:

			if(CH_InitGraphic((STLAYER_Handle_t)NULL,&BitmapTempInner,(CH6_Palette_t*)&PaletteTemp)) 
			{
				STTBX_Print(("CH_InitGraphic() is unsuccessful\n"));
				return TRUE;
			}
			VPSource.Palette_p=&(PaletteTemp.Palette);
			CH6_SetInnerParam(IndexTemp,TYPE_CHBITMAP,(void*)&BitmapTempInner);
			CH6_SetInnerParam(IndexTemp,TYPE_CHPALETTE,(void*)&PaletteTemp);

#ifdef FOR_CH_QAMI5516_USE 
			ValTemp=STSYS_ReadRegDev8((void*)(0x00000000+0xD6));/*yxl 2004-11-09 add this line*/ 
#endif
			break;
		default:
			STTBX_Print(("\nYxlInfo:Params ColorType is wrong,return "));
			return TRUE;
		}

		pCH6VPHandle->DrawViewPortHandle[i]=(CH_ViewPortHandle_t)BitmapTempInner.BlockHandle1;
		pCH6VPHandle->DrawVPCount++;

			if(i==0) 
			{
				pCH6VPHandle->ViewPortHandle=(CH_ViewPortHandle_t)BitmapTempInner.BlockHandle1;
			}
			else if(i==1)
			{
				pCH6VPHandle->ViewPortHandleAux=(CH_ViewPortHandle_t)BitmapTempInner.BlockHandle1;
			}
		


	}
	 
	for(i=0;i<Params->ViewPortCount;i++)
	{
			
		BitmapTempInner.Bitmap.Width=Params->ViewPortParams[i].Width;
		BitmapTempInner.Bitmap.Height=Params->ViewPortParams[i].Height;
		BitmapTempInner.Bitmap.ColorType=Params->ViewPortParams[i].ColorType;
		BitmapTempInner.Bitmap.BitmapType=Params->ViewPortParams[i].BitmapType;
		BitmapTempInner.PartitionHandle=Params->ViewPortParams[i].ViewportPartitionHandle;
		BitmapTempInner.Bitmap.AspectRatio=Params->ViewPortParams[i].AspectRatio;
		BitmapTempInner.BlockHandle1=STAVMEM_INVALID_BLOCK_HANDLE;

		PaletteTemp.PartitionHandle=Params->ViewPortParams[i].ViewportPartitionHandle;
		PaletteTemp.BlockHandle=STAVMEM_INVALID_BLOCK_HANDLE;

		switch(BitmapTempInner.Bitmap.ColorType)
		{
		case STGXOBJ_COLOR_TYPE_ARGB4444:
		case STGXOBJ_COLOR_TYPE_ARGB1555:
		case STGXOBJ_COLOR_TYPE_RGB565:
		#ifdef YXL_ARGB8888  /*yxl 2007-08-14 add*/
		case STGXOBJ_COLOR_TYPE_ARGB8888:
		#endif 
			if(CH_InitGraphic((STLAYER_Handle_t)NULL,&BitmapTempInner,NULL)) 
			{
				STTBX_Print(("CH_InitGraphic() is unsuccessful\n"));
				return TRUE;
			}
			VPSource.Palette_p=NULL;
#ifdef FOR_CH_QAMI5516_USE 
			ValTemp=STSYS_ReadRegDev8((void*)(0x00000000+0xD6));/*yxl 2004-11-09 add this line*/ 
			ValTemp=ValTemp|0x80;
#ifdef  YXL_GRAPHICBASE_DEBUG
			STTBX_Print(("\n yxlInfoA:ValTemp=%x",ValTemp));
#endif
#if 1 /*yxl 2005-06-03 temp cancel this line for test,this is ok*/
			STSYS_WriteRegDev8((void*)(0x00000000+0xD6),ValTemp); 	/*yxl 2004-10-20 add this line*/
#endif
			
#endif /*FOR_CH_QAMI5516_USE,yxl 2005-07-14 add this line*/       
			Alpha.A0=128;
			Alpha.A1=128;
			
			break;
		case STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422:
			if(CH_InitGraphic((STLAYER_Handle_t)NULL,&BitmapTempInner,NULL)) 
			{
				STTBX_Print(("CH_InitGraphic() is unsuccessful\n"));
				return TRUE;
			}
			
			VPSource.Palette_p=NULL;

			break;
			
		case STGXOBJ_COLOR_TYPE_CLUT8:
		case STGXOBJ_COLOR_TYPE_CLUT4:
		case STGXOBJ_COLOR_TYPE_CLUT2:
		case STGXOBJ_COLOR_TYPE_CLUT1:
			if(CH_InitGraphic((STLAYER_Handle_t)NULL,&BitmapTempInner,&PaletteTemp)) 
			{
				STTBX_Print(("CH_InitGraphic() is unsuccessful\n"));
				return TRUE;
			}
			
			VPSource.Palette_p=&(PaletteTemp.Palette);
#ifdef FOR_CH_QAMI5516_USE 
			ValTemp=STSYS_ReadRegDev8((void*)(0x00000000+0xD6));/*yxl 2004-11-09 add this line*/ 
			
			ValTemp=ValTemp&0x7f;
#ifdef  YXL_GRAPHICBASE_DEBUG
			STTBX_Print(("\n yxlInfoB:ValTemp=%x",ValTemp));
#endif
			
			
#endif /*FOR_CH_QAMI5516_USE */
			
#ifdef  YXL_GRAPHICBASE_DEBUG
#ifdef FOR_CH_QAMI5516_USE 
			STTBX_Print(("\n YxlInfo(C):VID_DCF(1)=%x VID_DCF(2)=%x VID_DIS=%x VID_MWS=%x VID_MWSV=%x  \n",
				STSYS_ReadRegDev8((void*)(0x00000000+0x74)), 
				STSYS_ReadRegDev8((void*)(0x00000000+0x75)), 
				STSYS_ReadRegDev8((void*)(0x00000000+0xD6)), 
				STSYS_ReadRegDev8((void*)(0x00000000+0x9C)), 
				STSYS_ReadRegDev8((void*)(0x00000000+0x9D)))); 
#endif
#endif
			break;
		default:
			STTBX_Print(("\nYxlInfo:Params ColorType is wrong,return "));
			return TRUE;
			
		}
		/*Set OSD viewport*/	
		VPSource.SourceType=STLAYER_GRAPHIC_BITMAP;
		VPSource.Data.BitMap_p=&(BitmapTempInner.Bitmap);
		
		VPParams.Source_p=&VPSource;
		VPParams.InputRectangle.PositionX=0;
		VPParams.InputRectangle.PositionY=0;
		VPParams.InputRectangle.Width=Params->ViewPortParams[i].Width;
		VPParams.InputRectangle.Height=Params->ViewPortParams[i].Height;
		
		VPParams.OutputRectangle.PositionX=Params->ViewPortParams[i].PosX;
		VPParams.OutputRectangle.PositionY=Params->ViewPortParams[i].PosY;
		VPParams.OutputRectangle.Width=Params->ViewPortParams[i].Width;
		VPParams.OutputRectangle.Height=Params->ViewPortParams[i].Height;

		ErrCode=STLAYER_OpenViewPort(Params->ViewPortParams[i].LayerHandle,&VPParams,
			&(pCH6VPHandle->LayerViewPortHandle[i]));
		if(ErrCode!=ST_NO_ERROR)
		{
			STTBX_Print(("STLAYER_OpenViewPort()=%s",GetErrorText(ErrCode)));
			return TRUE;
		}

		VPHandleTemp=pCH6VPHandle->LayerViewPortHandle[i];

		if(BitmapTempInner.Bitmap.ColorType==STGXOBJ_COLOR_TYPE_ARGB1555)
		{
			Alpha.A0=0;
			Alpha.A1=108;
		}
		else if(BitmapTempInner.Bitmap.ColorType==STGXOBJ_COLOR_TYPE_ARGB4444)
		{
			Alpha.A0=108;
			Alpha.A1=108;
		}
		#ifdef YXL_ARGB8888  /*yxl 2007-08-14 add*/
		else if(BitmapTempInner.Bitmap.ColorType==STGXOBJ_COLOR_TYPE_ARGB8888)
		{

			Alpha.A0=108;
			Alpha.A1=108;
		}	
		#endif 


		if(BitmapTempInner.Bitmap.ColorType!=STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422)
		{
			ErrCode=STLAYER_SetViewPortAlpha(VPHandleTemp,&Alpha);
			if(ErrCode!=ST_NO_ERROR)
			{
				STTBX_Print(("STLAYER_SetViewPortAlpha()=%s",GetErrorText(ErrCode)));
				return TRUE;
			}
		}

		#ifdef FOR_CH_QAMI5516_USE 
		if(BitmapTempInner.Bitmap.ColorType!=STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422)
		{
			
			ErrCode=STLAYER_EnableViewPortFilter(VPHandleTemp,0);
			if(ErrCode!=ST_NO_ERROR)
			{
				STTBX_Print(("STLAYER_EnableViewPortFilter()=%s",GetErrorText(ErrCode)));
			}
			
		}
		#endif 

		/*yxl 2007-07-02 add below section*/
#if 1
		{

			STLAYER_FlickerFilterMode_t eFilterMode= STLAYER_FLICKER_FILTER_MODE_SIMPLE;

			if(BitmapTempInner.Bitmap.ColorType!=STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422)
			{
				
				ErrCode = STLAYER_SetViewPortFlickerFilterMode(VPHandleTemp, eFilterMode);
				if(ErrCode!=ST_NO_ERROR)
				{
					STTBX_Print(("STLAYER_SetViewPortFlickerFilterMode()=%s",GetErrorText(ErrCode)));
				}	
				ErrCode=STLAYER_EnableViewPortFilter(VPHandleTemp,
					/*Params->ViewPortParams[i].LayerHandle*/0);
				if(ErrCode!=ST_NO_ERROR)
				{
					STTBX_Print(("STLAYER_EnableViewPortFilter()=%s",GetErrorText(ErrCode)));
				}
			}
		}
#endif
		/*end yxl 2007-07-02 add below section*/

		{
			CH6_SingleViewPort_InnerParams_t  SVPParamsTemp;
			SVPParamsTemp.LayerVPHandle=VPHandleTemp;
			SVPParamsTemp.BlockHandle=BitmapTempInner.BlockHandle1;
			SVPParamsTemp.PartitionHandle=BitmapTempInner.PartitionHandle;
			SVPParamsTemp.VPDisStatus=FALSE;

			memcpy((void*)&SVPParamsTemp.LayerVPBitmap,(const void*)&BitmapTempInner.Bitmap,
				sizeof(STGXOBJ_Bitmap_t));

			CH6_SetInnerParam(IndexTemp,TYPE_LAYERBITMAP,(void*)&SVPParamsTemp);

		}

		pCH6VPHandle->OSDLayerCount++;
	}	
	
	
	if(Params->ViewPortParams[0].ColorType==STGXOBJ_COLOR_TYPE_ARGB8888)
	{
		pTemp=memory_allocate(CHGDPPartition,72*72*4); /*最大为72*72点阵的字*/
	}
	else
	{
		pTemp=memory_allocate(CHGDPPartition,72*72*2); /*最大为72*72点阵的字*/
	}

	if(pTemp==NULL) return TRUE;

	CH6_SetInnerParam(IndexTemp,TYPE_FONT_DISPLAY_MEM,pTemp);
	
	CH6_SetInnerParam(IndexTemp,TYPE_CHVPHANDLE,(void*)&(pCH6VPHandle->ViewPortHandle));

#ifdef  YXL_GRAPHICBASE_DEBUG
	STTBX_Print(("\n YxlInfo:Index=%d Dis_Mem=%lx Font_mem=%lx Handle=%ld ",IndexTemp,
					
	CH6_VPInner_Params[IndexTemp].pFontDataMem,
	CH6_VPInner_Params[IndexTemp].pFontDisMem,
	CH6_VPInner_Params[IndexTemp].CHVPHandle));
#endif

	return FALSE;
}



BOOL CH6_SetViewPortAlpha(	STLAYER_ViewPortHandle_t VPHandle,STLAYER_GlobalAlpha_t* pAlpha)
{
	STLAYER_GlobalAlpha_t Alpha;
	ST_ErrorCode_t ErrCode;
	Alpha.A0=pAlpha->A0;
	Alpha.A1=pAlpha->A1;
	ErrCode=STLAYER_SetViewPortAlpha(VPHandle,&Alpha);
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STLAYER_SetViewPortAlpha()=%s",GetErrorText(ErrCode)));
		return TRUE;
	}

	return FALSE;
}



BOOL CH6_DeleteViewPort(CH_ViewPort_t VPHandle)
{
	int i;
	ST_ErrorCode_t ErrCode;

	STLAYER_ViewPortHandle_t HandleTemp;
	CH_ViewPortHandle_t CHHandleTemp;

	if(VPHandle.OSDLayerCount==0) return TRUE;

	
	for(i=0;i<VPHandle.OSDLayerCount;i++)
	{
		HandleTemp=VPHandle.LayerViewPortHandle[i];
		
		ErrCode=STLAYER_CloseViewPort(HandleTemp);	
		if(ErrCode!=ST_NO_ERROR)
		{
			STTBX_Print(("STLAYER_CloseViewPort()",GetErrorText(ErrCode)));
		}
	}

	CHHandleTemp=VPHandle.ViewPortHandle;
	CH6_ReleaseInnerParam(CHHandleTemp);
	
	return FALSE;
}



BOOL CH6_SetViewPortPaletteColor(STLAYER_ViewPortHandle_t VPHandle,U8* pRGBValue,unsigned short IndexCount)
{
	ST_ErrorCode_t ErrCode;
	STGXOBJ_Palette_t PaletteTemp;
	STGXOBJ_Bitmap_t BitmapTemp;
	STLAYER_ViewPortSource_t VPSource;

	VPSource.Palette_p=&PaletteTemp;
    VPSource.SourceType=STLAYER_GRAPHIC_BITMAP;
	VPSource.Data.BitMap_p=&BitmapTemp;

	ErrCode=STLAYER_GetViewPortSource(VPHandle,&VPSource);
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STLAYER_GetViewPortSource()=%s",GetErrorText(ErrCode)));
		return TRUE;
	}

	if(SetPaletteColor(&PaletteTemp,pRGBValue,IndexCount))
	{
		return TRUE;		
	}

	ErrCode=STLAYER_SetViewPortSource(VPHandle,&VPSource);
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STLAYER_SetViewPortSource()=%s",GetErrorText(ErrCode)));
		return TRUE;		
	}


	return FALSE;		

}


/*yxl 2005-10-30 modify parameter CH_ViewPort_t to CH_ViewPortHandle_t,and modify inner*/
BOOL CH6_GetPixelColor(STGFX_Handle_t GFXHandle, /*CH_ViewPort_t*/
					   CH_ViewPortHandle_t CHVPHandle,
				  int PosX,int PosY,unsigned int* pColorValue)
{
 	ST_ErrorCode_t ErrCode;
	STGXOBJ_Bitmap_t OSDBMPTemp;
	STGXOBJ_Color_t ColorTemp;
	unsigned int ValueTemp;

	if(PosX<0||PosY<0) 
	{
		return TRUE;
	}
	if(pColorValue==NULL)
	{
		return TRUE;
	}

	if(CH6_GetBitmapInfo(CHVPHandle/*.ViewPortHandle*/,&OSDBMPTemp))
	{
	
		return TRUE;		
	}

	if(PosX>OSDBMPTemp.Width||PosY>OSDBMPTemp.Height) return TRUE;

	ErrCode=STGFX_ReadPixel(GFXHandle,&OSDBMPTemp,PosX,PosY,&ColorTemp);
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STGFX_ReadPixel()=%s",GetErrorText(ErrCode)));
		return TRUE;		
	}

	switch(OSDBMPTemp.ColorType)
	{
		case STGXOBJ_COLOR_TYPE_CLUT8:
		case STGXOBJ_COLOR_TYPE_CLUT4:
		case STGXOBJ_COLOR_TYPE_CLUT2:
		case STGXOBJ_COLOR_TYPE_CLUT1:
			ValueTemp=ColorTemp.Value.CLUT8;
			break;

		case STGXOBJ_COLOR_TYPE_ARGB1555:
			ValueTemp=ColorTemp.Value.ARGB1555.Alpha*0x800+\
				ColorTemp.Value.ARGB1555.R*0x400+\
				ColorTemp.Value.ARGB1555.G*0x20+ColorTemp.Value.ARGB1555.B;

			break;
		case STGXOBJ_COLOR_TYPE_ARGB4444:
			ValueTemp=ColorTemp.Value.ARGB4444.Alpha*0x1000+\
				ColorTemp.Value.ARGB4444.R*0x100+\
				ColorTemp.Value.ARGB4444.G*0x10+ColorTemp.Value.ARGB4444.B;
		
			break;
		case STGXOBJ_COLOR_TYPE_RGB565:
			ValueTemp=ColorTemp.Value.RGB565.R*0x800+\
				ColorTemp.Value.RGB565.G*0x20+ColorTemp.Value.RGB565.B;
			break;
		#ifdef YXL_ARGB8888  
		case STGXOBJ_COLOR_TYPE_ARGB8888:
			ValueTemp=ColorTemp.Value.ARGB8888.Alpha*0x1000000+\
				ColorTemp.Value.ARGB8888.R*0x10000+\
				ColorTemp.Value.ARGB8888.G*0x100+ColorTemp.Value.ARGB8888.B;
		
			break;
		#endif 
		default:

			return TRUE;	
	}

	*pColorValue=ValueTemp;
	
	return FALSE;	
}

/*yxl 2005-10-30 modify parameter CH_ViewPort_t to CH_ViewPortHandle_t,and modify inner*/
BOOL CH6_SetPixelColor(STGFX_Handle_t GFXHandle,CH_ViewPortHandle_t CHVPHandle,STGFX_GC_t* pGC,
				  int PosX,int PosY,unsigned int Color)
{
 	ST_ErrorCode_t ErrCode;
  	STGXOBJ_Bitmap_t OSDBMPTemp;
	STGFX_GC_t* pGCTemp;


	if(PosX<0||PosY<0) return TRUE;
	if(CH6_GetBitmapInfo(CHVPHandle/*.ViewPortHandle*/,&OSDBMPTemp))
	{
		return TRUE;		
	}
	
	if(PosX>OSDBMPTemp.Width||PosY>OSDBMPTemp.Height) return TRUE;
	
	pGCTemp=pGC;
	pGCTemp->LineWidth=1;

	pGCTemp->FillColor.Type=OSDBMPTemp.ColorType;
	pGCTemp->DrawColor.Type=OSDBMPTemp.ColorType;


	if(CH6_SetColorValue(&(pGCTemp->FillColor),Color)) return TRUE;
	if(CH6_SetColorValue(&(pGCTemp->DrawColor),Color)) return TRUE;		

	ErrCode=STGFX_DrawDot(GFXHandle,&OSDBMPTemp,pGCTemp,PosX,PosY);
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STGFX_DrawDot()=%s",GetErrorText(ErrCode)));
		return TRUE;		
	}		

	ErrCode=STGFX_Sync(GFXHandle);
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STGFX_DrawDot()=%s",GetErrorText(ErrCode)));
		return TRUE;		
	}		 
	return FALSE;	
}



BOOL CH_SetViewPortAlpha(CH_ViewPort_t CHVPHandle,int AlphaValue)
{
	ST_ErrorCode_t ErrCode;
	STLAYER_GlobalAlpha_t Alpha;
	int i;
	STGXOBJ_Bitmap_t OSDBMPTemp;
	CH_ViewPortHandle_t VPHandleTemp;

	Alpha.A0=AlphaValue;

	Alpha.A1=AlphaValue;
	if(CH6_GetBitmapInfo(CHVPHandle.ViewPortHandle,&OSDBMPTemp))
	{
		STTBX_Print(("\nYxlInfo:can't get bitmap info"));
	
	}
	else
	{
		if(OSDBMPTemp.ColorType==STGXOBJ_COLOR_TYPE_ARGB1555)
		{
			Alpha.A0=0;
			Alpha.A1=AlphaValue;
		}
		else if(OSDBMPTemp.ColorType==STGXOBJ_COLOR_TYPE_ARGB4444)
		{
			Alpha.A0=AlphaValue;
			Alpha.A1=AlphaValue;
		}
		#ifdef YXL_ARGB8888  /*yxl 2007-08-14 add*/
		else if(OSDBMPTemp.ColorType==STGXOBJ_COLOR_TYPE_ARGB8888)
		{
			Alpha.A0=AlphaValue;
			Alpha.A1=AlphaValue;
		}
		#endif 

	}
	if(CHVPHandle.OSDLayerCount==0) return TRUE;
	
	for(i=0;i<CHVPHandle.OSDLayerCount;i++)
	{
		VPHandleTemp=CHVPHandle.LayerViewPortHandle[i];

		ErrCode=STLAYER_SetViewPortAlpha(VPHandleTemp,&Alpha);
		if(ErrCode!=ST_NO_ERROR)
		{
			STTBX_Print(("STLAYER_SetViewPortAlpha()=%s",GetErrorText(ErrCode)));
			return TRUE;
		}
	}
	return FALSE;

}






/*yxl 2008-03-27 add below sections*/
static CH_ViewPortHandle_t LastUpdateVPHandle=0;/*上次更新的Viewport handle*/

CH_ViewPortHandle_t CH_GetLastDisplayViewPort(void)
{
	return LastUpdateVPHandle;
}

/*end yxl 2008-03-27 add below sections*/

BOOL CH_DisplayCHVP(CH_ViewPort_t CHVPHandle,CH_ViewPortHandle_t VPHandle)
{
	int i;
	ST_ErrorCode_t ErrCode;
	STGXOBJ_Bitmap_t OSDBMPTemp;
	STGXOBJ_Bitmap_t OSDBMPTempDst;

	STGFX_GC_t GCTemp;
	STGXOBJ_Rectangle_t RectTemp;

	STGXOBJ_Rectangle_t RectTempDst;	
	
	STGXOBJ_Color_t DrawColor;
    STGXOBJ_Color_t FillColor;

    DrawColor.Type=FillColor.Type=STGXOBJ_COLOR_TYPE_ARGB4444;
	DrawColor.Value.ARGB4444.Alpha  = 15;
	DrawColor.Value.ARGB4444.R      = 15;
	DrawColor.Value.ARGB4444.G      = 15;
	DrawColor.Value.ARGB4444.B      = 15;
  
	FillColor.Value.ARGB4444.Alpha  = 15;
	FillColor.Value.ARGB4444.R      = 0;/*15;*/
	FillColor.Value.ARGB4444.G      = 15;
	FillColor.Value.ARGB4444.B      = 15;

	STGFX_SetGCDefaultValues(DrawColor, FillColor, &GCTemp);

	if(CH6_GetBitmapInfo(VPHandle,&OSDBMPTemp))
	{
		return TRUE;
	}	

	LastUpdateVPHandle=VPHandle;/*yxl 2008-03-27 add this line*/
	
	for(i=0;i<CHVPHandle.OSDLayerCount;i++)
	{
		
		if(CH6_GetLayerBitmapInfo(CHVPHandle.LayerViewPortHandle[i],&OSDBMPTempDst))
		{
			return TRUE;
		}		
		
		RectTemp.PositionX=0;
		RectTemp.PositionY=0;
		
		RectTemp.Width=OSDBMPTemp.Width;
		RectTemp.Height=OSDBMPTemp.Height;
		
		RectTempDst.PositionX=0;
		RectTempDst.PositionY=0;
		
		RectTempDst.Width=OSDBMPTempDst.Width;
		RectTempDst.Height=OSDBMPTempDst.Height;	
		ErrCode=STGFX_CopyAreaA(GFXHandle,&OSDBMPTemp,&OSDBMPTempDst,&GCTemp,&RectTemp,&RectTempDst);
		if(ErrCode!=ST_NO_ERROR)
		{
			STTBX_Print(("STGFX_CopyAreaA()=%s",GetErrorText(ErrCode)));
			return TRUE;
		}
		
		ErrCode=STGFX_Sync(GFXHandle);
		if(ErrCode!=ST_NO_ERROR)
		{
			STTBX_Print(("STGFX_Sync()=%s",GetErrorText(ErrCode)));
			return TRUE;
		}
		/*yxl 2007-06-25 reenable STGFX_Sync*/
	}
	return FALSE;
}




#if 1 /*yxl 2006-03-06 move this function from graphicmid.c,and modify*/

#ifndef ENGLISH_FONT_MODE_EQUEALWIDTH /*yxl 2006-04-22 add this line*/
/***************************************/

/*函数描述:获取一行文字的宽度，单位象素
*参数:
text:指向文字的指针
*返回值:
-1:错误
Width:文字的长度
*/
int CH6_GetTextWidth(char* text)
{
	int Width = 0 ;
	char *temp;
	int TotalCount = strlen(text) ;
	int CountTemp = 0;

	if( TotalCount > 255) return -1;

	temp = text;

	while( CountTemp<TotalCount )
	{

		if(*temp>=0x21&&*temp<0x7f)
		{
			Width+= cModeWidth[*temp-0x21];			
		} 
		else if(*temp==0x20)
		{
			Width+=FONT_SIZE/2-2;
		}
		else Width+=FONT_SIZE/2;
		temp++;
		CountTemp++;
	}

	return Width;	

}

#endif  /*yxl 2006-04-22 add this line*/

#endif /*end yxl 2006-03-06 move this function from graphicmid.c,and modify*/


/*yxl 2006-03-20 add this function*/
/***************************************/

/*函数描述:获取一个字符的宽度，单位象素
*参数:
text:
*返回值:
-1:错误
Width:字符的宽度
*/
int CH6_GetCharWidth(char Char)
{
	int Width = 0 ;
	char CharTemp;

	CharTemp=Char;


	if(CharTemp>=0x21&&CharTemp<0x7f)
	{
#ifndef ENGLISH_FONT_MODE_EQUEALWIDTH /*yxl 2006-04-22 add this line*/
		Width+= cModeWidth[CharTemp-0x21];			
#else /*yxl 2006-04-22 add below section*/
	Width+=FONT_SIZE/2-2;
#endif/*end yxl 2006-04-22 add below section*/
	} 
	else if(CharTemp==0x20)
	{
		Width+=FONT_SIZE/2-2;
	}
	else Width+=FONT_SIZE/2;

	return Width;	

}







/*End of file*/







