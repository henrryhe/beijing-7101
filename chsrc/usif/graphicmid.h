/*
  Name:graphicmid.h
  Description:header file

Modifiction:

Date:Yixiaoli 2007-09-03 by yixiaoli  
1.Add ARGB8888 格式支持，由MACRO YXL_ARGB8888控制，并修改如下函数支持ARGB8888
（主要是将与颜色相关的参数由int 等改为unsigned int)
(1)CH6_DisRectTxtInfoNobck(...);
(2)CH6_MulLine(...);
(3)DispLaySelect(...);
(4)CH6_DrawLinkLine(STGFX_Handle_t GHandle,STLAYER_ViewPortHandle_t VPHandle,int StartPosX,
		int StartPosY,int EndPosX,int EndPosY,U8 Ratio,
		int LineThickness,unsigned int LineColor)(...);
(5)CH6_DrawLinkLine(...);
(6)DrawTriangle(...);
(7)CH6_MulLine_ChangeLine(...);
(8)CH6_MulLine_ChangeLine1(...);
(9)CH6_Displayscoll(...);
(10)DrawTrigon1(...);
(11)DrawTrigon2(...);
*/


#include "stlayer.h"
#include "stgfx.h"
#include "stgxobj.h"
#include "graphicbase.h"

#ifndef GRAPH_MID
#define GRAPH_MID

#ifndef USE_GFX
#define USE_GFX             /*针对底层一些基本图形不能显示而作的修改*/
#endif

/*系统位置设置定义*/

#define SYSTEM_TITLE_FRAM_X       75           /*系统设置标题框X位置*/
#define SYSTEM_TITLE_FRAM_Y       46           /*系统设置标题框Y位置*/

#define SYSTEM_TIME_FRAM_X        502          /*系统设置时间框X位置*/
#define SYSTEM_TIME_FRAM_Y        42           /*系统设置时间框Y位置*/
#define SYSTEM_TIME_FRAM_WIDTH        160      /*系统设置时间框宽度*/
#define SYSTEM_TIME_FRAM_HEIGHT        50      /*系统设置时间框高度*/

#define SYSTEM_OUTER_FRAM_X       48          /*系统设置外边框X位置*/
#define SYSTEM_OUTER_FRAM_Y       92          /*系统设置外边框Y位置*/
#define SYSTEM_OUTER_FRAM_WIDTH   604         /*系统设置外边框宽度*/
#define SYSTEM_OUTER_FRAM_HEIGHT  393         /*系统设置外边框高度*/

#define SYSTEM_INNER_FRAM_X       238        /*系统设置内边框X位置*/
#define SYSTEM_INNER_FRAM_Y       108        /*系统设置内边框Y位置*/
#define SYSTEM_INNER_FRAM_WIDTH   403        /*系统设置内边框宽度*/
#define SYSTEM_INNER_FRAM_HEIGHT  363        /*系统设置内边框高度*/
#define SYSTEM_INFO_FRAM_X       246        /*系统设置对话框X位置*/
#define SYSTEM_INFO_FRAM_Y       170        /*系统设置对话框Y位置*/


#define SYSTEM_FIRSTITEM_X   57             /*系统设置条目X位置*/
#define SYSTEM_FIRSTITEM_Y   146              /*系统设置条目Y位置*/
#define SYSTEM_ITEM_WIDTH   175             /*系统设置条目宽度*/
#define SYSTEM_ITEM_HEIGHT  35                /*系统设置条目高度*/

#define SYSTEM_ITEM_GAP_BETWEEN   15        /*系统设置条目间间距*/
#define SYSTEM_ITEM_GAP_ARROW     15        /*系统设置条目与上下箭头间距*/
#define SYSTEM_ARROW_GAP_OUTER    20        /*系统设置上下箭头与外边框间距*/


#define SET_BANNER_X_START 300
#define SET_BANNER_Y_START 140
#define SET_BANNER_WIDTH   340
#define SET_BANNER_HEIGHT  250

/*20051115  WZ add  the  file  for AV  setting */

#define AVSET_ITEM_LEFT_X      238+10+10
#define AVSET_ITEM_LEFT_WIDTH       170     /* AV设置条目的宽*/
#define AVSET_ITEM_RIGHT_X    238+10+10+170+10
#define AVSET_ITEM_RIGHT_WIDTH     180
#define  AVSET_ITEM_Y               108+30+/*45 20060923 change*/30
#define AVSET_ITEM_HEIGH        30
#define AVSET_ITEM_H_GAP       20    /*AV设置条目的水平间距*/
#define AVSET_ITEM_V_GAP       /*15 20060923 change*/10     /*AV设置条目的垂直间距*/


#define  AVSET_FRAM_X_START  238+10
#define  AVSET_FRAM_Y_START  108+30
#define  AVSET_FRAM_WIDTH      380
#define  AVSET_FRAM_HEIGH       300



#ifdef OSD_COLOR_TYPE_RGB16
#define TEXT_COLOR			0x910e/*F247*/
#define RECT_FILL_COLOR		0xF3DE/*FEFF*/

#else
#define TEXT_COLOR			0xff204070/*F247*/
#define RECT_FILL_COLOR		0xffe0f0f0/*FEFF*/
#endif

/***********/
#define CHARACTER_WIDTH 12
/*****all the pic name define***/



#ifndef OSD_COLOR_TYPE_RGB16  
#define RADIOBACK "BACK.MPG"
#define BLACK_PIC "BLACK.MPG"   

#define BUTTON_RETURN "b_return.ga"  /*yxl 2004-12-20 add this line*/
#define BACKGROUND_PIC "back.gam"       
#define FAV_PIC "Favo.gam"              
#define SKIP_PIC "skip.gam"
#define  LOCK_PIC "lock.gam"
#define  UNREAD_MAIL_PIC "mailnew.bmp"
#define  READ_MAIL_PIC "mailold.bmp"
#define  CLOCK_PIC    "clock.gam"
#define  MAIL_UNREAD_PIC    "mail.gam"
#define  MAIL_READ_PIC  "rmail.gam"
#define DLG1_PIC   "clock.gam"  
#define DLG2_PIC   "red.bmp"    
#define ADD1_PIC     "add1.gam"
#define ADD_PIC     "add.gam"
#define SUB1_PIC     "sub1.gam"
#define SUB_PIC     "sub.gam"
#define RADIO_PIC   "Radio.gam"
#define BAR_PIC     "bar.gam"
#define RED_BUTTON   "Rbutt.gam"
#define GREEN_BUTTON "Gbutt.gam"
#define YELLOW_BUTTON "Ybutt.gam"
#define BLUE_BUTTON  "Bbutt.gam"
#define  LOGMPG     "LOGO.MPG"
#define  RADIOMPG   "RADIO.MPG"  
#define MUTE_PIC    "mute.gam"
#define UNMUTE_PIC  "Unmute.gam"
#define BLUEFILL_PIC "Bfill.gam"
#define YELLOWFILL_PIC "Yfill.gam"
#define DIALOG1_PIC     "Dlg1.gam"
#define DIALOG2_PIC     "Dlg1.gam"/*"DlgTran.gam"*/
#define CH_TITLE_PIC       "CHTitle.gam"
#define EN_TITLE_PIC       "ENTitle.gam"
#define NVOD_CHI_TITLE_PIC "CHnvod.gam"
#define NVOD_ENG_TITLE_PIC "ENnvod.gam"
#define EN_EPGTITLE		"ENepg.gam"
#define CH_EPGTITLE		"CHepg.gam"
#define PROG_PIC	"prog.gam"
#define SPROG_PIC	"Sprog.gam"
/*ljg 040903 add*/
#define LEFTRIGHT_PIC "b_lr.gam"
#define UPARRF_PIC	"Uarrowf.gam"
#define UPARR_PIC	"Uarrow.gam"
#define DOWNARRF_PIC	"Darrowf.gam"
#define DOWNARR_PIC	"Darrow.gam"
#define BUTTON_A		"butt_A.gam"
#define BUTTON_B		"butt_B.gam"
#define BUTTON_C		"butt_C.gam"
#define BUTTON_PAGE	"b_page.gam"
#define BUTTON_OK		"b_ok.gam"
#define BUTTON_EXIT		"b_exit.gam"
#define LEFT_PROG_B 	"Left_B.gam"
#define LEFT_PROG_Y 	"Left_Y.gam"
#define RIGHT_PROG_B 	"Right_B.gam"
#define RIGHT_PROG_Y 	"Right_Y.gam"
#define RIGHT_PROG_W 	"Right_W.gam"
#define BUTTON_DOWN	"b_down.gam"
#define BUTTON_UP		"b_up.gam"
#define BUTTON_LEFT		"b_left.gam"
#define BUTTON_RIGHT	"b_right.gam"
#define BUTTON_MENU	"b_menu.gam"
#define BUTTON_INFO "b_info.gam"
#define CH_GAMETITLE	"CHgame.gam"
#define EN_GAMETITLE	"ENgame.gam"
#define YELLOW_PROG_2   "proy_2.gam"
#define YELLOW_PROG_5   "proy_5.gam"
#define YELLOW_PROG_10   "proy_10.gam"
#define YELLOW_PROG_20   "proy_20.gam"
#define YELLOW_PROG_50   "proy_50.gam"
#define BLUE_PROG_2   	"prob_2.gam"
#define BLUE_PROG_5   	"prob_5.gam"
#define BLUE_PROG_10  	 "prob_10.gam"
#define BLUE_PROG_20   	"prob_20.gam"
#define BLUE_PROG_50   	"prob_50.gam"
/*END ljg 040903 add*/

#define BLACK_MPG		"BLACK.MPG"

#define SEARCH_MPG		"Search.mpg"
#define MULTI_AUDIO                "MultAud.gam"
#define MULTI_CH                   "MultC.gam"
#define MULTI_EN                   "MultE.gam"
#define AC3_SIGN                    "ac3.gam"   /* 20061203 wz add*/


#define SYSTEM_FONT_NORMA_COLORL         0xff204070
#define SYSTEM_FONT_FOCUS_COLOR      /*  0XDB5C*/0xfff8f8f8
#define SYSTEM_TIME_TEXT_COLOR           	0xff205080
#define SYSTEM_SELECT_ITEM_FILL_COLOR  0xfff08020
#define SYSTEM_SELECT_ITEM_BORDER_COLOR  0xffc0d0e0
#define SYSTEM_NORNAL_ITEM_FILL_COLOR  0xffc0d0e0
#define SYSTEM_NORMAL_ITEM_BORDER_COLOR 0xff5080b0
#define SYSTEM_INNER_FRAM_BORDER_COLOR	 0xff5080b0
#define SYSTEM_OUTER_FRAM_BORDER_COLOR   0xffd0d0e0
#define SYSTEM_WAITFONT_BACKCOLOR      0xff7090b0
#define SYSTEM_TIME_BACK_COLOR            0xff80a0d0/*系统设置时间字体背景颜色*/
#define SYSTEM_INNER_FRAM_COLOR          0xffc0d0e0  /*系统设置内框颜色*/
#define SYSTEM_OUTER_FRAM_COLOR         0xff80a0c0/*   系统设置外框颜色*/
#define SYSTEM_lIST_FILL_COLOR          0xffe0f0f0
#define SYSTEM_lIST_BORDER_COLOR         0xff5080b0
#define SYSTEM_BUTTON_COLOR		0xffe0f0f0
#define SYSTEM_BUTTONBORDER_COLOR	0xff70a0c0
/*yxl 2004-12-18 modify this section*/
#if 0
#define SEARCH_INFO_COLOR          0x8CDE    /*系统设置内框颜色*/
#else
#define SEARCH_INFO_COLOR          0xffd0e0f0/*0x3fff */   /*系统设置内框颜色*/
#endif
/*end yxl 2004-12-18 modify this section*/

#define SYSTEM_HELP_FILL_COLOR           0xff6080a0/*  系统设置帮助信息填充色*/
#define SYSTEM_HELP_BORDER_CLOR       0xffc0d0e0  /*系统设置帮助信息边框色*/      

#define SCROLL_FILL_COLOR                0xff6080a0   /*滑竿背景色*/
#define SCROLL_SLIDER_COLOR              0xffd0e0f0    /*滑竿条色*/


#define whitecolor                       0xfff8f8f8
#define RedColor                         0xffa02030
#define BlueColor                        0xff305080
#define GreenColor                       0xff00c000
#define orange_color				0xfff08020
#define PROG_YELLOW_COLOR			     0xfff07020
#define PROG_BLUE_COLOR			         0xff7090c0

#if 0 /*yxl 2004-12-22 modify this line*/
#define SEARCH_TEXT_COLOR				0xf106
#else
#define SEARCH_TEXT_COLOR				0xff6090d0
#endif/*end yxl 2004-12-22 modify this line*/


#else
#define RADIOBACK "BACK.MPG"
#define BLACK_PIC "BLACK.MPG"   

#define BUTTON_RETURN "b_return.ga"  /*yxl 2004-12-20 add this line*/
#define BACKGROUND_PIC "back.gam"       
#define FAV_PIC "Favo.gam"              
#define SKIP_PIC "skip.gam"
#define  LOCK_PIC "lock.gam"
#define  UNREAD_MAIL_PIC "mailnew.bmp"
#define  READ_MAIL_PIC "mailold.bmp"
#define  CLOCK_PIC    "clock.gam"
#define  MAIL_UNREAD_PIC    "mail.gam"
#define  MAIL_READ_PIC  "rmail.gam"
#define DLG1_PIC   "clock.gam"  
#define DLG2_PIC   "red.bmp"    
#define ADD1_PIC     "add1.gam"
#define ADD_PIC     "add.gam"
#define SUB1_PIC     "sub1.gam"
#define SUB_PIC     "sub.gam"
#define RADIO_PIC   "Radio.gam"
#define BAR_PIC     "bar.gam"
#define RED_BUTTON   "Rbutt.gam"
#define GREEN_BUTTON "Gbutt.gam"
#define YELLOW_BUTTON "Ybutt.gam"
#define BLUE_BUTTON  "Bbutt.gam"
#define  LOGMPG     "LOGO.MPG"
#define  RADIOMPG   "RADIO.MPG"  
#define MUTE_PIC    "mute.gam"
#define UNMUTE_PIC  "Unmute.gam"
#define BLUEFILL_PIC "Bfill.gam"
#define YELLOWFILL_PIC "Yfill.gam"
#define DIALOG1_PIC     "Dlg1.gam"
#define DIALOG2_PIC     "DlgTran.gam"/*"Dlg2.gam"*/

#define CH_TITLE_PIC       "CHTitle.gam"
#define EN_TITLE_PIC       "ENTitle.gam"
#define NVOD_CHI_TITLE_PIC "CHnvod.gam"
#define NVOD_ENG_TITLE_PIC "ENnvod.gam"
#define EN_EPGTITLE		"ENepg.gam"
#define CH_EPGTITLE		"CHepg.gam"
#define PROG_PIC	"prog.gam"
#define SPROG_PIC	"Sprog.gam"
/*ljg 040903 add*/
#define LEFTRIGHT_PIC "b_lr.gam"
#define UPARRF_PIC	"Uarrowf.gam"
#define UPARR_PIC	"Uarrow.gam"
#define DOWNARRF_PIC	"Darrowf.gam"
#define DOWNARR_PIC	"Darrow.gam"
#define BUTTON_A		"butt_A.gam"
#define BUTTON_B		"butt_B.gam"
#define BUTTON_C		"butt_C.gam"
#define BUTTON_PAGE	"b_page.gam"
#define BUTTON_OK		"b_ok.gam"
#define BUTTON_EXIT		"b_exit.gam"
#define LEFT_PROG_B 	"Left_B.gam"
#define LEFT_PROG_Y 	"Left_Y.gam"
#define RIGHT_PROG_B 	"Right_B.gam"
#define RIGHT_PROG_Y 	"Right_Y.gam"
#define RIGHT_PROG_W 	"Right_W.gam"
#define BUTTON_DOWN	"b_down.gam"
#define BUTTON_UP		"b_up.gam"
#define BUTTON_LEFT		"b_left.gam"
#define BUTTON_RIGHT	"b_right.gam"
#define BUTTON_MENU	"b_menu.gam"
#define BUTTON_INFO "b_info.gam"
#define CH_GAMETITLE	"CHgame.gam"
#define EN_GAMETITLE	"ENgame.gam"
#define YELLOW_PROG_2   "proy_2.gam"
#define YELLOW_PROG_5   "proy_5.gam"
#define YELLOW_PROG_10   "proy_10.gam"
#define YELLOW_PROG_20   "proy_20.gam"
#define YELLOW_PROG_50   "proy_50.gam"
#define BLUE_PROG_2   	"prob_2.gam"
#define BLUE_PROG_5   	"prob_5.gam"
#define BLUE_PROG_10  	 "prob_10.gam"
#define BLUE_PROG_20   	"prob_20.gam"
#define BLUE_PROG_50   	"prob_50.gam"
/*END ljg 040903 add*/

#define BLACK_MPG		"BLACK.MPG"

#define SEARCH_MPG		"Search.mpg"
#define MULTI_AUDIO                "MultAud.gam"
#define MULTI_CH                   "MultC.gam"
#define MULTI_EN                   "MultE.gam"
#define AC3_SIGN                    "ac3.gam"   /* 20061203 wz add*/


#define SYSTEM_FONT_NORMA_COLORL         0x910e
#define SYSTEM_FONT_FOCUS_COLOR      /*  0XDB5C*/0xFFFF
#define SYSTEM_TIME_TEXT_COLOR           	0x9150
#define SYSTEM_SELECT_ITEM_FILL_COLOR  0XFA04
#define SYSTEM_SELECT_ITEM_BORDER_COLOR  0XE35C
#define SYSTEM_NORNAL_ITEM_FILL_COLOR  0XE35C
#define SYSTEM_NORMAL_ITEM_BORDER_COLOR 0XAA16
#define SYSTEM_INNER_FRAM_BORDER_COLOR	 0XAA16
#define SYSTEM_OUTER_FRAM_BORDER_COLOR   0XEB5C
#define SYSTEM_WAITFONT_BACKCOLOR      0XBA56
#define SYSTEM_TIME_BACK_COLOR            0XC29A/*系统设置时间字体背景颜色*/
#define SYSTEM_INNER_FRAM_COLOR          0XE35C  /*系统设置内框颜色*/
#define SYSTEM_OUTER_FRAM_COLOR         0XC298/*   系统设置外框颜色*/
#define SYSTEM_lIST_FILL_COLOR          0XF3DE
#define SYSTEM_lIST_BORDER_COLOR         0XAA16
#define SYSTEM_BUTTON_COLOR		0XF3DE
#define SYSTEM_BUTTONBORDER_COLOR	0XBA98
/*yxl 2004-12-18 modify this section*/
#if 0
#define SEARCH_INFO_COLOR          0x8CDE    /*系统设置内框颜色*/
#else
#define SEARCH_INFO_COLOR          0xeb9E/*0x3fff */   /*系统设置内框颜色*/
#endif
/*end yxl 2004-12-18 modify this section*/

#define SYSTEM_HELP_FILL_COLOR           0xB214/*  系统设置帮助信息填充色*/
#define SYSTEM_HELP_BORDER_CLOR       0XE35C  /*系统设置帮助信息边框色*/      

#define SCROLL_FILL_COLOR                0xB214   /*滑竿背景色*/
#define SCROLL_SLIDER_COLOR              0xEB9E    /*滑竿条色*/


#define whitecolor                       0xFFFF
#define RedColor                         0xD086
#define BlueColor                        0x9950
#define GreenColor                       0x8300
#define orange_color				0xfa04
#define PROG_YELLOW_COLOR			     0xF9C4
#define PROG_BLUE_COLOR			         0xBA58

#if 0 /*yxl 2004-12-22 modify this line*/
#define SEARCH_TEXT_COLOR				0xf106
#else
#define SEARCH_TEXT_COLOR				0XB25A
#endif/*end yxl 2004-12-22 modify this line*/
#endif


/*FlashFileStr  *GlobalFlashFileInfo;*/
#define M_CHECK(X,XLOW,XHIGH) {if((X)<(XLOW)) (X)=(XHIGH);else if ((X)>(XHIGH)) (X)=(XLOW);}/*ljg 20040712 add*/
struct Focus{U16 XPs;U16 YPs;U16 Wid; U16 Het;};


#ifdef USE_GFX

typedef enum{
	REC_STYLE_N		= 10,
  	REC_STYLE_L		= 30,
  	REC_STYLE_R		= 20,
  	REC_STYLE_U		= 100,
  	REC_STYLE_B		= 120,
  	REC_STYLE_A		= 40,
  	REC_STYLE_LU		= 50,
  	REC_STYLE_LD		= 60,
	REC_STYLE_RU		= 70,
	REC_STYLE_RD		= 80
}RECSTYLE;


extern	void CH_DrawPixel(STGFX_Handle_t GFXHandle,STLAYER_ViewPortHandle_t CHVPHandle,STGFX_GC_t* pGC,
				  			int PosX,int PosY,unsigned int Color);

extern	boolean CH6_QFilledCircle( STGFX_Handle_t Handle, STLAYER_ViewPortHandle_t VPHandle,
							U16 uCenterX, U16 uCenterY, U8 uRadius, U32 uColor );


extern	boolean CH_DrawLine( STGFX_Handle_t GHandle, STLAYER_ViewPortHandle_t VPHandle,STGFX_GC_t* pGC,
						   U16 uX1, U16 uY1, U16 uX2, U16 uY2, unsigned int uColor	) ; 


extern  void CH_Stk_DrawPoint(STGFX_Handle_t GHandle, STLAYER_ViewPortHandle_t VPHandle,int PosX,int PosY,unsigned int Color);

#endif

extern int GetKeyFromKeyQueue(S8 WaitingSeconds);


/*函数功能：
该函数用于画一个以(x,y)为起点，以width, height为宽高的矩形。
接口说明：
int  Lcolor   边线色,U16 LWidth    边线宽
Int  color      填充色为-1不填充，
STGFX_Handle_t GHandle:the handle of the opened STGFX
STLAYER_ViewPortHandle_t VPHandle:the handle of the viewport which will be operated

返回值：
true   成功；
false   失 败
*/
extern boolean  CH6m_DrawRectangle(STGFX_Handle_t GHandle,STLAYER_ViewPortHandle_t VPHandle,STGFX_GC_t* pGC, U32 x, U32 y, U32 width, U32 height,unsigned int color,unsigned int Lcolor,U16 LWidth);

/*
 函数功能：
    该函数用于画一个以(XStart, YStart)为起点，以width, height为宽高，以Radiu为圆角半径的圆角矩形。
接口说明：
int   Lcolor     边线色,U16 LWidth 边线宽
 int  pColor    填充色
STGFX_Handle_t GHandle:the handle of the opened STGFX
STLAYER_ViewPortHandle_t VPHandle:the handle of the viewport which will be operated

返回值：
true   成功；
false   失败。
*/
extern boolean  CH6m_DrawRoundRectangle(STGFX_Handle_t GHandle,STLAYER_ViewPortHandle_t VPHandle,STGFX_GC_t* pGC, U32 XStart,U32 YStart,U32 Width,U32 Height,U32 Radiu,unsigned int pColor,unsigned int LColor,U16 LWidth);


/*线*/
/* 函数功能：
    显示一条以（X1，Y1）为起点，以（X2，Y2）为终点的线段。
    接口说明：
    int Selectcolor 显示线的颜色。
STGFX_Handle_t GHandle:the handle of the opened STGFX
STLAYER_ViewPortHandle_t VPHandle:the handle of the viewport which will be operated
    返回值：
true   成功；
false   失败。
*/
#ifdef USE_GFX
extern boolean  CH6m_DrawLine(STGFX_Handle_t GHandle,STLAYER_ViewPortHandle_t VPHandle,STGFX_GC_t* pGC,U16 X1,U16 Y1,U16 X2,U16 Y2,U32 Selectcolor);
#else
extern boolean  CH6m_DrawLine(STGFX_Handle_t GHandle,STLAYER_ViewPortHandle_t VPHandle,
					   STGFX_GC_t* pGC,S32 X1,S32 Y1,S32 X2,S32 Y2,unsigned int Selectcolor);
#endif

/*椭圆，圆的显示*/
/* 函数功能：
    显示一个以（XStart，YStart）为圆心的圆或椭圆。
    接口说明：
    U32 HRadiu,U32 VRadiu    输入须显示的垂直半径和水平半径。
Int  pColor  填充色
Int Lcolor  ,U16 Lwidth  输入需显示的勾边的颜色和宽。
STGFX_Handle_t GHandle:the handle of the opened STGFX
STLAYER_ViewPortHandle_t VPHandle:the handle of the viewport which will be operated
  返回值：
 true   成功；
false   失败。
*/

extern boolean CH6m_DrawEllipse(STGFX_Handle_t GHandle,STLAYER_ViewPortHandle_t VPHandle,STGFX_GC_t* pGC,U32 XStart,U32 YStart,U32 HRadiu,U32 VRadiu,unsigned int  pColor/*填充色*/,unsigned int  LColor/*边线颜色*/,U16 LWidth);
/*                       位图放大缩小                                       */
//extern void bmscaler(int dest_width,int dest_height,int source_width, int source_height,  U8 *DesBitmap, U8 *bitmap);
/*函数功能：
显示一个自定义大小的图片。
接口说明：
char * WhichBall    需显示的图片名
int x,   int y     为图片显示的起始位置
int width,   int height    图片显示的宽高
返回值：
无。
*/

extern void CH6m_DisplayBall(char * WhichBall,int x,int y,int width,int height);


/*有背景的字符串的显示*/
/*函数功能：
该函数用于显示有背景的文本
接口说明：
int  x_top,   int  y_top      字符显示的起始位置
int Width,  int Heigth,         背景的宽高
char  *TxtStr                要显示的字符串
int  BackColor   显示的背景颜色
int  txtColor      显示的字体颜色
STGFX_Handle_t GHandle:the handle of the opened STGFX
STLAYER_ViewPortHandle_t VPHandle:the handle of the viewport which will be operated
  返回值：
无。
*/
extern void CH6_DisRectTxtInfo(STGFX_Handle_t GHandle,STLAYER_ViewPortHandle_t VPHandle,int x_top,int y_top,int Width,int Heigth,char *TxtStr,unsigned int  BackColor,unsigned int  txtColor,int LineWidth,U8 Align);


/*无背景的字符串的显示*/
/*函数功能：
居中显示一行无背景的字符串，主要是区别于有背景的显示。
接口说明：
int  x_top,   int  y_top     显示的字符的起始位置
int Width, int Heigth          显示的区域
char  *TxtStr                 显示的内容
int  txtColor    显示的字的颜色
STGFX_Handle_t GHandle:the handle of the opened STGFX
STLAYER_ViewPortHandle_t VPHandle:the handle of the viewport which will be operated
  返回值：
无。
*/
extern void CH6_DisRectTxtInfoNobck(STGFX_Handle_t GHandle,STLAYER_ViewPortHandle_t VPHandle,int x_top,int y_top,int Width,int Heigth,char *TxtStr,unsigned int  txtColor,U8 Align);


/*显示当前右上角菜单标题*/
/*函数功能：
显示当前右上角标题。
接口说明：
char  CH_Title    中文标题
char  *EN_Title    英文标题
STGFX_Handle_t GHandle:the handle of the opened STGFX
STLAYER_ViewPortHandle_t VPHandle:the handle of the viewport which will be operated
返回值：
无。
*/
extern void DisplayUpTitle(STGFX_Handle_t GHandle,STLAYER_ViewPortHandle_t VPHandle,char *CH_Title,char *EN_Title);

/****得到位图数据中的某一块区域******/
U8* GetBmpData(U32 start_x,U32 start_y,U32 width,U32 height,U32 source_width,U32 source_height,U8 *SourceData);


/*显示背景某一部分*/
/*函数功能：
显示背景图片的一部分。
接口说明：
U32 x1,  U32 y1     背景图片的起始位置
U32 width,  U32 height     显示部分的大小
U32 x2, U32  y2     显示图片的起始位置
STGFX_Handle_t GHandle:the handle of the opened STGFX
STLAYER_ViewPortHandle_t VPHandle:the handle of the viewport which will be operated
返回值：
无。
*/
extern void display_home_right_bottom( STGFX_Handle_t GHandle,STLAYER_ViewPortHandle_t VPHandle,char *pMessage);


/*显示当前帮助信息*/
/*函数功能：
显示当前提供的帮助信息。
接口说明：
char    *HelpInfo    要显示的帮助信息内容。
STGFX_Handle_t GHandle:the handle of the opened STGFX
STLAYER_ViewPortHandle_t VPHandle:the handle of the viewport which will be operated
返回值：
无。
*/
extern void DisplayRightHelp(STGFX_Handle_t GHandle,STLAYER_ViewPortHandle_t VPHandle,char *HelpInfo);




/*函数功能：
显示一个对话框。
接口说明：
  char *DialogContent   需显示的字符串
  char Buttons          显示类型标志
  int  WaitSeconds      显示的时间
  STGFX_Handle_t GHandle:the handle of the opened STGFX
  STLAYER_ViewPortHandle_t VPHandle:the handle of the viewport which will be operated
  yxl 2004-12-26 add  paramter U8 DefReturnValue:
  U8 DefReturnValue:0,standfor return OK button when no key pressed in 2 buttons mode,
					1,standfor return exit button when no key pressed in 2 buttons mode,
					other value,standfor return OK button when no key pressed in 2 buttons mode
*/
#if 0 /*yxl 2004-12-26 modify below section*/ 
extern boolean	CH6_PopupDialog (STGFX_Handle_t GHandle,STLAYER_ViewPortHandle_t VPHandle,char *DialogContent,char Buttons,int WaitSeconds);
#else
extern boolean	CH6_PopupDialog (STGFX_Handle_t GHandle,STLAYER_ViewPortHandle_t VPHandle,char *DialogContent,char Buttons,int WaitSeconds,U8 DefReturnValue);

#endif/*end yxl 2004-12-26 modify below section*/ 


/*below section is added by ljg*/

 /*函数描述:获取一行文字的 长度，单位象素
 *参数:
 		text:指向文字的指针
 *返回值:
 		-1:错误
 		Width:文字的长度
*/
extern int CH6_GetTextWidth(char* text);

/*函数说明:计算数字的位数
*/
extern int CH6_ReturnNumofData(int InputData);

/*函数描述:显示连接两点的折线，折线由三段组成，
				第一段水平直线，第二段竖直直线，第三段水平直线
 *参数:
 		StartPosX	:折线起点位置的横坐标
 		StartPosY		:折线起点位置的纵坐标
 		EndPosX		:折线终点位置的横坐标
 		EndPosY		:折线终点位置的纵坐标
 		Ratio		:第一段和第三段的比例
 		Thickness	:线的粗细
 		LineColor		:线的颜色
		STGFX_Handle_t GHandle:the handle of the opened STGFX
        STLAYER_ViewPortHandle_t VPHandle:the handle of the viewport which will be operated
 *返回值:无
*/
extern void CH6_DrawLinkLine(STGFX_Handle_t GHandle,STLAYER_ViewPortHandle_t VPHandle,int StartPosX,int StartPosY,int EndPosX,int EndPosY,U8 Ratio,int LineThickness,unsigned int  LineColor);


/*函数描述:画一填充颜色的圆角矩形，在其中显示一行文字
 *参数:
 		iStartX		:圆角矩形起点横坐标
 		iStartY		:圆角矩形起点纵坐标
 		iWidth		:圆角矩形宽度
 		iHeight		:圆角矩形高度
 		Color		:填充颜色
 		pItemMessage:要显示的文字
		STGFX_Handle_t GHandle:the handle of the opened STGFX
		STLAYER_ViewPortHandle_t VPHandle:the handle of the viewport which will be operated
 *返回值:无
 */
/*extern boolean CH6_DrawRoundRecText(STGFX_Handle_t Ghandle,STLAYER_ViewPortHandle_t VPHandle,U32 StartX ,U32 StartY , U32 Width , U32 Height ,int  BackColor,int TextColor, int Align,char* pItemMessage);*/

/*函数描述:画一背景框，并在其上画水平滑动杆和滑动块，
				三者都是填充了颜色的矩形			     
 *参数:
 		StartX	:背景框起点横坐标
 		StartY	:背景框起点纵坐标
 		Height	:背景框高度
 		sWidth	:滑动块宽度
 		sHeight	:滑动块高度
 		lHeight	:滑动杆粗细
 		ItemMax	:最大滑动次数
 		Step		:步进值
 		Index	:当前滑动块所处位置
 		Color	:背景框颜色
 		sColor	:滑动杆颜色和滑动块内框颜色
		STGFX_Handle_t GHandle:the handle of the opened STGFX
		STLAYER_ViewPortHandle_t VPHandle:the handle of the viewport which will be operated
 *返回值:
 		     tempWidth:背景框宽度
*/
/*extern int CH6_DrawHorizontalSlider(STGFX_Handle_t GHandle,STLAYER_ViewPortHandle_t VPHandle,U32 StartX,U32 StartY,U32 Height,U32 sWidth, U32 sHeight,U32 sLineWidth,U32 lHeight,U32 ItemMax,U32 Step,U32 Index,int Color,int sColor);*/


extern void CH6_MulLine(STGFX_Handle_t GHandle,STLAYER_ViewPortHandle_t VPHandle,int x_top,int y_top,int Width,int Heigth,char *TxtStr,unsigned int  txtColor,U8 H_Align,U8 V_Align);
/**/
extern void CH6_DrawLog(void);
extern void CH6_DrawRadio(void);

/*ljg 040831 add*/
extern void CH6_DrawAdjustBar(STGFX_Handle_t GHandle,STLAYER_ViewPortHandle_t VPHandle,int StartX,int StartY,int PreValue,int CurValue,BOOL FirstDraw,U8 FillColorType);
extern boolean DrawTriangle(STGFX_Handle_t GHandle,STLAYER_ViewPortHandle_t VPHandle,STGFX_GC_t* pGC,STGFX_Point_t Point1,STGFX_Point_t Point2,STGFX_Point_t Point3,unsigned int Color);
extern void CH6_DisplaySomeBP(STGFX_Handle_t GHandle,STLAYER_ViewPortHandle_t VPHandle,char*SPicName,U32 x1,U32 y1,U32 SWidth,U32 SHeight,U32 width,U32 height,U32 x2,U32 y2);
extern S8 PasswordVerify(char* pPassword);

extern void CH6_DisplayKeyButton(int StartX,int StartY,char* KeyButtonName);

extern U8	CH6_FindPageCutSize( U8* uBuff, U16 uPageLength, U16* pCutPoint, U8 uMaxPage );

extern void CH_UpdateAllOSDView(void);
extern void CH6_CloseMepg(void);
#endif

