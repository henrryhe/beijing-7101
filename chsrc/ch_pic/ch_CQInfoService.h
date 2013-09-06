/************************************************************************/
/* 版权所有：四川长虹网络科技有限公司(2006)                             */
/* 文件名：ch_CQInfoService.h                                           */
/* 版本号： V1. 0														*/
/* 作者：杨雄															*/
/* 创建日期：2006.7.13													*/
/* 头文件内容概述：机顶盒信息指令文档解析相关数据结构和接口API			*/
/* 修改历史:															*/
/* -- 时间           作者     修改部分及原因							*/                     
/* 2006-7-28      杨雄    	1).CH_ENUM_COMMAND_TYPE 中增加两个类型
 *						    2).增加CH_TIMEOUT_EXIST，CH_SCROLL_PARAMETERS，CH_SCROLL_TEXT三个数据结构
 *						    3).增加CH_GetTimeOutExistCommadnAttribute ，CH_GetScrollTextCommadnAttribute函数
 *						    4).增加CH_GetScrollParametersAttribute，CH_GetTimeAttribute函数
 *						    5).增加 CH_GetTimeOutExist，CH_SetTimeOutExist函数
 * 2006/07/20    王鳇峰     1).增加   CH_GetKeywordAttributeStr函数 
 *                          2).增加   CH_IsKeywordClose函数
06 /08/01 邹书强 1. 改结构体PCH_ACTION->ui_TextID 为 S8 ui_TextID[ID_NAME_MAX_LENGTH+1];
 				原为U16 ui_TextID;
 			2. 改结构体PCH_ACTION->Action_Z_Parameter.ui_TextID 为 S8 ui_TextID[ID_NAME_MAX_LENGTH+1];
 				原为U16 ui_TextID;
 			3. 增加结构体元素PCH_TEXT_COMMAND->Sct_Scollbar;
06 /09/19   杨雄  1) CH_ENUM_ACTION_TYPE 中 增加ACTION_ZGADD,ACTION_ZGDEC,ACTION_ZLADD,ACTION_ZLDEC,ACTION_TVGUIDE
				  2) CH_ACTION 结构中增加 Action_ZG_ZL_Parameter,Action_TVGuide_Parameter 
				  3) CH_ENUM_COMMAND_TYPE 中增加e_COMMAND_FORCEACTION
				  4) 增加结构CH_FORCE_ACTION
*****************************头文件注释结束*****************************/  

#ifndef _CH_CQINFOSERVICE_H_
#define _CH_CQINFOSERVICE_H_

#include "common.h"
#include "os_plat.h"

#define ID_NAME_MAX_LENGTH 50/*ID属性的最大长度*/
#define MAX_KEY_LENGTH 50 /*关键字最大长度*/
#define CARD_ID_LENGTH	18/*智能卡号的长度*/

#define z_DEFAULT_BKCOL_VUL 0xf1f2f3
/*指令类型*/
typedef enum _command_type_
{
	e_COMMAND_AV = 0, 
	e_COMMAND_REGION, 
	e_COMMAND_IMG,
	e_COMMAND_TEXT, 
	e_COMMAND_KEYACTION,
	e_COMMAND_TEXTLINK,
	e_COMMAND_TIMEOUTEXIST,
	e_COMMAND_SCROLLTEXT,
	e_COMMAND_FORCEACTION,	/*在一段时间后强制运行ACTION指令*/
	e_COMMAND_CARDID,
	e_COMMAND_STBID,
	e_COMMAND_STBIDTEXT,
	e_COMMAND_CARDIDTEXT,
	e_COMMAND_NUMINPUT, /* 数字输入指令*/
	e_COMMAND_COPYRIGHT,/*显示版本信息*/
	e_COMMAND_VCHANNEL,/*定时执行命令*/
#ifdef NET_ENABLE	
	e_COMMAND_LINKRESOURCEINFO,/*资源描述*/
#endif	
	e_COMMAND_EPG,/*调用其它模块*/
#ifdef NET_ENABLE	
	e_COMMAND_BLINK,/*调用第三方浏览器*/
#endif	
/*.....*/
	e_COMMAND_GROUPACTION,/*组操作指令 */
	__END_COMMAND
}CH_ENUM_COMMAND_TYPE;

/*转义字符集*/
typedef struct _esc_char_ 
{
	S8* pc_EscChars;/*转义符*/
	S8* pc_RealChars;/*代表的字符*/
}CH_ESC_CHARS,*PCH_ESC_CHARS;

/*显示区域定义*/
typedef struct _ShowRect_ 
{
	CH_ENUM_COMMAND_TYPE enum_CommandType;/*使用该显示区域的指令的指令类型*/
	S32 l_SX;/*内容区域起始X位置(左上角)*/
	S32 l_SY;/*内容区域起始Y位置(左上角)*/
	S32 l_EX;/*内容区域结束X位置(右下角)*/
	S32 l_EY;/*内容区域结束Y位置(右下角)*/
	S32 CX; /*中心点的坐标, sqzow add 0812*/
	S32 CY;
	U32 ul_BKColor;/*24位表示的RGB颜色值,高8位未用，设为0.
					该颜色表示视频切换或开始前所显示的背景颜色
					当ul_BKColor == 0xffffffff时表示没有该参数*/
	S32 l_Z;/*代表绘制时的Z处理顺序，数字越大越靠上*/
}CH_SHOWRECT,*PCH_SHOWRECT;

/* CH_ACTION 结构中 uc_AVType的取值，指示是音频还是视频*/
#define  AV_VIDEO  0			/*视频*/
#define  AV_AUDIO  1			/*音频*/

#define TYPE_AV	0
#define TYPE_DAV 	1

/* CH_ACTION 结构中 enum_Action 取值 */
typedef enum _action_type_
{
	ACTION_NONE = 0,		/*无动作*/
	ACTION_AV,				/*切换音视频*/
	ACTION_VACTION,			/*对当前视频操作，若当前为音频，该参数无效果*/
	ACTION_AACTION,			/*对当前音频操作，若当前为视频，该参数无效果*/
	ACTION_AVACTION,			/*对音视频进行统一操作  sqzow 0925*/
	ACTION_DAV,
	ACTION_RLINK,			/*跨频点切换当前页面*/
	ACTION_LINK,			/*切换当前页面*/
	ACTION_DLINK,	/*切换当前页面*/
	ACTION_CPAGEDOWN,		/*下翻页*/
	ACTION_CPAGEUP,			/*上翻页*/
	ACTION_Z,				/*更改指令的 Z 顺序 */
	ACTION_ZGADD,			/*更改某一组指令的 Z 顺序，将Z顺序大于第1 参数的
	                                                  * 全部指令的z 顺序增加第2个参数指定的值*/
	ACTION_ZGDEC,			/*更改某一组指令的 Z 顺序，将Z顺序大于第1 参数的
	                                                  * 全部指令的z 顺序减少加第2个参数指定的值*/
	ACTION_ZLADD,			/*更改某一组指令的 Z 顺序，将Z顺序小于第1 参数的
	                                                  * 全部指令的z 顺序增加第2个参数指定的值*/
	ACTION_ZLDEC,			/*更改某一组指令的 Z 顺序，将Z顺序小于第1 参数的
	                                                  * 全部指令的z 顺序减少加第2个参数指定的值*/
	ACTION_ZRADD,		/*将Z大于参数1小于参数2的对象加上参数3*/
	ACTION_ZRDEC,		/*将Z大于参数1小于参数2的对象减去参数3*/
	ACTION_TVGUIDE,			/*跳转到参数指定的频道*/
	ACTION_EXIT,			/*退出该信息应用*/
	ACTION_CHANNEL_UP,/*频道加减*/
	ACTION_CHANNEL_DOWN,
	ACTION_VODAV,				
	ACTION_GA,		/*组操作指令*/
	ACTION_VODLINK,
#ifdef NET_ENABLE	
	ACTION_HTTPGETLINK,
	ACTION_HTTPPOSTLINK,
	ACTION_BLINK,
#endif	
	ACTION_EPG
}CH_ENUM_ACTION_TYPE;

/*l_VActionParameter 和 l_AActionParameter 的取值*/
#define AV_CLOSE		0	/*关闭当前文档中AV指令播放的音视频。若AV为全屏或背景播放，则恢复到
							  非全屏状态。如果AV指令是视频，则同时全部指令文档按照Z参数大于AV
							  指令参数的重新绘制。若文档中无AV指令，该操作无效.*/
#define AV_OPEN			1	/*播放当前文档中的音视频。如果AV指令是视频，则同时全部指令文档按照
							  Z参数大于AV指令参数的重新绘制。若文档中无AV指令，该操作无效.*/
#define AV_FULL			2	/*该参数取值只对视频。全屏播放，同时全部指令文档按照Z参数大于AV指令
							  参数的重新绘制。若文档中无AV指令，或者视频没有播放，该操作无效.*/
#define AV_BACK			3	/*该参数取值只对视频。视频恢复到原大小，同时全部指令文档按照Z参数
							  大于AV指令参数的重新绘制。若文档中无AV指令,或者是音频指令,该操作无效.*/

/*操作型和混合型指令的Action动作和参数*/
typedef struct _action_ 
{
	CH_ENUM_COMMAND_TYPE enum_CommandType;/*使用该Action的指令的指令类型*/
	CH_ENUM_ACTION_TYPE enum_Action; /* 对应的处理方式(Action) */
	union
	{
		struct Action_AV_Parameter 
		{
			U32 uc_AVType;
			U16 ui_NetworkID;
			U16 ui_TSID;
			U16 ui_ServiceID;
			U16 ui_ComponetID;/*如果是视频，则对应Componet1参数*/
			U16 ui_ComponetID2;
		}stru_AVParameter; /* Action 为AV 时的参数*/

		struct Action_DAV_Parameter 
		{
			U32 uc_AVType;/*高16位表示vodtype, 低16位表示avtype.vodtype = 0: vod1,vod2; =3: vod3*/
			U32 ui_Freq;
			U16 ui_SymbolRate;
			U16 ui_PMTID;
			U32 ui_TimeOut;
			U16 ui_Componet1;/*这个几个值根据输入参数的个数来确定各自的意义
							所以此时还不能明确,只能用序号来代替   sqzow 061021*/
			U16 ui_Componet2;
			U16 ui_Componet3;
			U16 ui_Componet4;
		}stru_DAVParameter; /* Action 为DAV 时的参数*/
		
		S32 l_AVActionParameter;/*Action 为 VAction 或 AAction 时的参数*/
		
		struct Action_Rlink_Parameter 
		{
			U16 ui_NetworkID;
			U16 ui_TSID;
			U16 ui_ServiceID;
			U16 ui_ComponetID;
			U16 ui_Service_Page_ID;
		}stru_RLinkParameter; /* Action 为 RLink 时的参数*/

		struct Action_Dlink_Parameter 
		{
			U32 ui_Freq;
			U16 ui_SymbolRate;
			U16 ui_QAMType;
			U16 ui_ServiceID;/*节目号*/
			U16 ui_PID;/*数据的PID*/
			U16 ui_Service_Page_ID;
			U32 ui_TimeOut;
		}stru_DLinkParameter; /* Action 为 DLink 时的参数*/

		struct Action_Link_Parameter/* Action 为 Link  或 EPG时的参数 */
		{
			U32 ui_Service_Page_ID;
			S8* HttpURL;/*当ui_Service_Page_ID高16位不为0时生效，表示到http上搜索ibl文件*/
		}stru_LinkParameter;
		
		S8 pc_TextID[ID_NAME_MAX_LENGTH];
		/* Action 为 CPageDown 或 CPageUp 或GA时的参数，
			            对ID为TextID的Text指令区域进行翻页， 或者执行名称为本字符串的组操作*/
							
		struct Action_Z_Parameter
		{
			S8 pc_ElementID[ID_NAME_MAX_LENGTH];
			S32 l_ZOrderInt;
		}stru_ZParameter;/*Action 为 Z 时的参数。更改ID为ElementID指令的Z顺序为ZOrderInt*/
						
		struct Action_ZG_ZL_Parameter
		{
			S32 l_ZOrderInt;
			S32 l_FIXInt;
		}stru_ZGZLParameter;/*Action 为ZGADD,ZGDEC,ZLADD,ZLDEC, 时的参数。*/

		struct Action_ZR_Parameter
		{
			S32 l_FIXInt;
			S32 l_ZOrderMinInt,l_ZOrderMaxInt ;
		}stru_ZRParameter;/*Action 为ZRDEC,ZRADD, 时的参数。*/


		struct Action_TVGuide_Parameter 
		{
			U16 ui_NetworkID;
			U16 ui_TSID;
			U16 ui_ServiceID;
		}stru_TVGuideParameter; /* Action 为 TVGuide 时的参数*/

#ifdef NET_ENABLE
		struct Action_Blink_Parameter /* Action 为 BLINK 时的参数*/
		{
			S8* BlinkURL;
		}stru_BlinkParameter;

		struct Action_HttpPostLink_Parameter
		{
			S8* pc_URL;
			S8* pc_IP;
			U16 ui_Port;
			S8* pc_Location;
			S8* pc_FileName;
			S8* pc_Params;
		}stru_HttpPostLinkParameter;

		struct Action_HttpGetLink_Parameter
		{
			S8* pc_URL;
			S8* pc_IP;
			U16 ui_Port;
			S8* pc_Location;
			S8* pc_FileName;
		}stru_HttpGetLinkParameter;
#endif		
	}uni_Data;
}CH_ACTION,*PCH_ACTION;

/*设置 l_Init 的取值*/
#define  INIT_CLOSED					0	/*初始时，音视频是关闭的 */
#define  INIT_FULLSCREEN_PLAY			1	/*初始时，视频是全屏播放，若是音频则无效 */
#define  INIT_AREA_PLAY					2	/*初始时，在(sx,sy)(ex,ey)区域播放视频，若是音频则无效 */
#define  INIT_NOT_CLOSESERIVECOMPONET	3	/*初始时，不关闭当前Service	中的音视频 Componet */

/*AV 指令结构*/
typedef struct _AV_command_ 
{
	S8 pc_ID[ID_NAME_MAX_LENGTH+1];/*表示指令在文档范围内唯一的标记*/
	U16 ui_AVtype; /*表明是DAV还是AV*/ 
	CH_SHOWRECT stru_ShowArea;/*显示区域和背景*/
	CH_ACTION stru_Action; /*Action动作和参数*/
	S32 l_Init;/*表示音视频在指令文档初次显示时的状态*/
}CH_AV_COMMAND,*PCH_AV_COMMAND;

/*Region 指令*/
typedef struct _region_command_ 
{
	S8 pc_ID[ID_NAME_MAX_LENGTH+1];/*表示指令在文档范围内唯一的标记*/
	CH_SHOWRECT stru_ShowArea;/*显示区域和背景*/
	U32 ui_BkImg;/*显示的背景图片资源参数，协议中的 Servive_Obj_Id,0表示没有BkImg属性。 */
	U32 ul_Transparent;/* 表示该Region 的半透明值 */
}CH_REGION_COMMAND,*PCH_REGION_COMMAND;

/*Text 指令*/
typedef struct _text_command_ 
{
	S8 pc_ID[ID_NAME_MAX_LENGTH+1];/*表示指令在文档范围内唯一的标记*/
	CH_SHOWRECT stru_ShowArea;/*显示区域和背景*/
	U32 ul_Color;/* 24位表示的RGB颜色值,高8位未用，设为0.	该颜色表示文本颜色*/
	CH_BOOL b_Nowarp;/* 当 b_Nowarp == CH_TRUE 时，ex 位置不用换行,缺省是 CH_FALSE */
	CH_BOOL b_Scrollbar;/* 当 b_Scrollbar == CH_TRUE 时，文本超出 ey 位置时要显示滚动条,缺省是 CH_FALSE */
	U32 Sct_Scollbar;/*滚动条的相关内容，当b_Srollbar == ch_true 起作用*/
	S8* p_Content;/*需要显示的文本的实际内容,UTF-8表示*/
}CH_TEXT_COMMAND,*PCH_TEXT_COMMAND;

typedef int (*chz_key_action_func)(PCH_ACTION pkey_action);

/* KeyAction 指令*/
typedef struct _keyaction_command_ 
{
	S8 pc_ID[ID_NAME_MAX_LENGTH+1];/*表示指令在文档范围内唯一的标记*/
	U8 uc_Key;/* 对应的键值 */
	CH_ACTION stru_Action; /*Action动作和参数*/
	chz_key_action_func action_func;/*处理函数*/
}CH_KEY_ACTION_COMMAND,*PCH_KEY_ACTION_COMMAND;

/* Img 指令*/
typedef struct _img_command_ 
{
	S8 pc_ID[ID_NAME_MAX_LENGTH+1];/*表示指令在文档范围内唯一的标记*/
	CH_SHOWRECT stru_ShowArea;/*显示区域和背景*/
	U32 ui_SRC;/*显示的背景图片资源参数，协议中的 Servive_Obj_Id */
	U32 ui_FSRC;/*焦点的图片资源参数*/
	CH_BOOL b_IsDefaultfocus;/*如果为 CH_TRUE 表示该指令是文档的默认焦点*/
	CH_ACTION stru_Action; /*Action动作和参数*/
	chz_key_action_func action_func;/*处理函数*/
}CH_IMG_COMMAND,*PCH_IMG_COMMAND;

/* TextLink 指令 */
typedef struct _textlink_command_ 
{
	S8 pc_ID[ID_NAME_MAX_LENGTH+1];/*表示指令在文档范围内唯一的标记*/
	CH_SHOWRECT stru_ShowArea;/*显示区域和背景*/
	U32 ul_Color;/* 24位表示的RGB颜色值,高8位未用，设为0.
					该颜色表示文本颜色*/
	U32 ul_Fcolor;/*焦点前景颜色*/
	U32 ul_Fbkcolor;/*焦点背景颜色*/
	CH_BOOL b_Nowarp;/* 当 b_Nowarp == CH_TRUE 时，ex 位置不用换行,缺省是 CH_FALSE */
	CH_BOOL b_IsDefaultfocus;/*如果为 CH_TRUE 表示该指令是文档的默认焦点*/
 /*TXTLINK_SCROLL*/
	CH_BOOL b_FScroll;/*是否滚动显示焦点 */

	CH_ACTION stru_Action; /*Action动作和参数*/

	S8* p_Content;/*需要显示的文本的实际内容,UTF-8表示*/
	chz_key_action_func action_func;/*处理函数*/
}CH_TEXTLINK_COMMAND,*PCH_TEXTLINK_COMMAND;

/*TimeOutExist 指令*/
typedef struct _timeoutexist_command_
{
	S8 pc_ID[ID_NAME_MAX_LENGTH+1];/*表示指令在文档范围内唯一的标记*/
	U32 l_Time;/*表示超时退出的时间定义*/
}CH_TIMEOUT_EXIST,*PCH_TIMEOUT_EXIST;

/*滚动相关的参数*/
typedef struct _scroll_parameters_
{
	CH_ENUM_COMMAND_TYPE enum_CommandType;/*使用该scroll_parameters的指令的指令类型*/
	S32 l_ScrollType;/*滚动方式
						0:  横向滚动
						1:  纵向滚动*/	
	U32 l_ScrollTime;/*单次滚动刷新时间*/
	S32 l_ScrollStep;/*单次滚动距离*/
}CH_SCROLL_PARAMETERS,*PCH_SCROLL_PARAMETERS;

/*ScrollText 指令*/
typedef struct _scroll_text_
{
	S8 pc_ID[ID_NAME_MAX_LENGTH+1];/*表示指令在文档范围内唯一的标记*/
	CH_SHOWRECT stru_ShowArea;/*显示区域和背景*/
	U32 ul_Color;/* RGB颜色值,	该颜色表示文本颜色*/
	CH_BOOL b_Nowarp;/* 当 b_Nowarp == CH_TRUE 时，ex 位置不用换行,缺省是 CH_FALSE */
	CH_SCROLL_PARAMETERS stru_ScrollParamters;
	S8* p_Content;/*需要显示的文本的实际内容,UTF-8表示*/
}CH_SCROLL_TEXT,*PCH_SCROLL_TEXT;

/*ForceAction指令*/
typedef struct _force_action_
{
	S8 pc_ID[ID_NAME_MAX_LENGTH+1];/*表示指令在文档范围内唯一的标记*/
	U32 l_Time;/*表示超时退出的时间定义*/
	CH_ACTION stru_Action; /*Action动作和参数*/
	U32 i_CurrentTime;/*目前已经耗去的时间.*/
}CH_FORCE_ACTION,*PCH_FORCE_ACTION;

/*STBIDText指令*/
typedef struct _stbid_action
{
	S8 pc_ID[ID_NAME_MAX_LENGTH+1];/*表示指令在文档范围内唯一的标记*/
	U16 ui_Time, ui_CurrentTime;
	U16 ui_Type;
	S8 pc_CardID1[CARD_ID_LENGTH+1];
	S8 pc_CardID2[CARD_ID_LENGTH+1];
	S8 *pc_CardID;
	S8* p_Content;/*需要显示的文本的实际内容,UTF-8表示*/
	chz_key_action_func action_func;/*处理函数*/
	CH_ACTION stru_Action; /*Action动作和参数*/
}CH_STBID_ACTION,*PCH_STBID_ACTION; /* Action 为 STBID 时的参数*/


/*CardIDText指令*/
typedef struct _cardid_action
{
	S8 pc_ID[ID_NAME_MAX_LENGTH+1];/*表示指令在文档范围内唯一的标记*/
	U32 ui_Time, ui_CurrentTime;
	U16 ui_Type;
	S8 pc_CardID1[CARD_ID_LENGTH+1];
	S8 pc_CardID2[CARD_ID_LENGTH+1];
	S8 *pc_CardID;
	S8* p_Content;/*需要显示的文本的实际内容,UTF-8表示*/
	chz_key_action_func action_func;/*处理函数*/
	CH_ACTION stru_Action; /*Action动作和参数*/
}CH_CARDID_ACTION,*PCH_CARDID_ACTION; /* Action 为 CARDID 时的参数*/


/*STBIDText指令*/
typedef struct _stbidtext_
{
	S8 pc_ID[ID_NAME_MAX_LENGTH+1];/*表示指令在文档范围内唯一的标记*/
	CH_SHOWRECT stru_ShowArea;/*显示区域和背景*/
	U32 ul_Color;/* RGB颜色值,	该颜色表示文本颜色*/
	CH_BOOL b_Nowarp;/* 当 b_Nowarp == CH_TRUE 时，ex 位置不用换行,缺省是 CH_FALSE */
	S8* p_Content;/*需要显示的文本的实际内容,UTF-8表示*/
}CH_STBID_TEXT,*PCH_STBID_TEXT;

/*CardIDText指令*/
typedef struct _cardidtext_
{
	S8 pc_ID[ID_NAME_MAX_LENGTH+1];/*表示指令在文档范围内唯一的标记*/
	CH_SHOWRECT stru_ShowArea;/*显示区域和背景*/
	U32 ul_Color;/* RGB颜色值,	该颜色表示文本颜色*/
	CH_BOOL b_Nowarp;/* 当 b_Nowarp == CH_TRUE 时，ex 位置不用换行,缺省是 CH_FALSE */
	S8* p_Content;/*需要显示的文本的实际内容,UTF-8表示*/
}CH_CARDID_TEXT,*PCH_CARDID_TEXT;

/*COPYRIGHT指令*/
typedef struct _copyright_
{
	CH_SHOWRECT stru_ShowArea;/*显示区域和背景*/
	U32 ul_Color;/* RGB颜色值,	该颜色表示文本颜色*/
	CH_BOOL b_Nowarp;/* 当 b_Nowarp == CH_TRUE 时，ex 位置不用换行,缺省是 CH_FALSE */
}CH_COPYRIGHT,*PCH_COPYRIGHT;


/*NUMINPUT指令*/
typedef struct _numInput_
{
	S8 pc_ID[ID_NAME_MAX_LENGTH+1];/*表示指令在文档范围内唯一的标记*/
	S8 str_Answer[20];
	int ui_type;/*0机顶盒密码， 1智能卡密码(预留) 2密码形式， 3明文*/
	int ui_length;
	CH_SHOWRECT stru_ShowArea;/*显示区域和背景*/
	U32 ul_Color;/* RGB颜色值,	该颜色表示文本颜色*/
	CH_BOOL b_Nowarp;/* 当 b_Nowarp == CH_TRUE 时，ex 位置不用换行,缺省是 CH_FALSE */
	S8* p_Content;/*需要显示的文本的实际内容,UTF-8表示*/
	chz_key_action_func action_func;/*处理函数*/
	CH_ACTION stru_Action; /*Action动作和参数*/
	CH_BOOL b_IsDefaultfocus;/*如果为 CH_TRUE 表示该指令是文档的默认焦点*/
}CH_NUM_INPUT,*PCH_NUM_INPUT;

/*VCHANNEL指令*/
typedef struct _timeraction_
{
	U16 ui_Timer;
	chz_key_action_func action_func;/*处理函数*/
	CH_ACTION stru_Action; /*Action动作和参数*/
}CH_TimerAction,*PCH_TimerAction; /*定时触发指令*/

/*GroupAction指令*/
typedef struct _group_action_
{
	S8 pc_ID[ID_NAME_MAX_LENGTH+1];/*表示指令在文档范围内唯一的标记*/
	U32 l_Nums;/*ACTION数目*/
	S8* p_Content; /*传递闭包指令内容。 解析完成后该值为NULL*/
	CH_ACTION** ppActions; /*Action动作和参数*/
}CH_GROUP_ACTION, *PCH_GROUP_ACTION;/*组操作指令*/


#ifdef NET_ENABLE
/*记录当前页面上所有对象的ID名称*/
typedef struct _CommandPcIDRecord_
{
	S8* pc_ID;
	void* rp_Command;
	CH_ENUM_COMMAND_TYPE cmdType;
	unsigned long hash;
}CH_CmdPcIDRecord, *PCH_CmdPcIDRecord;

/*记录需要从http上搜索的数据url*/
typedef struct _HttpDataNeedResearch_
{
	char* url;
	char* dataType;
	unsigned long hash;
}CH_HttpDataNdSch_t, *PCH_HttpDataNdSch_t;
#endif

/*CMD参数定义*/
enum
{
/*添加新记录*/
	INFO_ADDRECORD_E,
/*重置记录*/
	INFO_RESET_E,
/*删除所有记录*/
	INFO_DELETE_E,
/*查询记录或搜索新数据*/
	INFO_SEARCH_E
};


/*以下为实现解析使用的函数*/
/**************************************************************************
** 功能: 从给定的字符串中获取指令的属性
** 参数: rp_Command,指令结构指针，根据需要应该转换成相应的指令数据结构
**					指令结构包括: CH_AV_COMMAND,CH_REGION_COMMAND,
**						CH_TEXT_COMMAND,CH_KEY_ACTION_COMMAND,CH_IMG_COMMAND,
**						CH_TEXTLINK_COMMAND,CH_TIMEOUT_EXIST,CH_SCROLL_TEXT
**       rp_Buffer: 属性字符串
**       rul_BufferLength: 属性字符串长度
** 返回: 成功返回 CH_TURE, 失败返回 CH_FALSE
**************************************************************************/
typedef CH_BOOL (*GET_KEY_ATTRIBUTE_FUNC)(void* rp_Command,S8 *rp_Buffer,U32 rul_BufferLength);

typedef struct _KeyMap_ 
{
	int enum_CommandTyep;	
	S8* pc_Key;/*关键字*/
	GET_KEY_ATTRIBUTE_FUNC fn_GetAttribute;/*对于该关键字，对应的属性处理函数*/
}CH_KEY_MAP,*PCH_KEY_MAP;

/* 根据属性字符串,获得CH_AV_COMMAND指令的属性
 * 参数: rp_Command,CH_AV_COMMAND 结构指针 
 * 其他参见 GET_KEY_ATTRIBUTE_FUNC 说明 */
CH_BOOL CH_GetAVCommadnAttribute(void* rp_Command,S8 *rp_Buffer,U32 rul_BufferLength);

/* 根据属性字符串,获得 CH_REGION_COMMAND 指令的属性
 * 参数: rp_Command,CH_REGION_COMMAND 结构指针 
 * 其他参见 GET_KEY_ATTRIBUTE_FUNC 说明 */
CH_BOOL CH_GetRegionCommadnAttribute(void* rp_Command,S8 *rp_Buffer,U32 rul_BufferLength);

/* 根据属性字符串,获得 CH_TEXT_COMMAND 指令的属性
 * 参数: rp_Command,CH_TEXT_COMMAND 结构指针 
 * 其他参见 GET_KEY_ATTRIBUTE_FUNC 说明 */
CH_BOOL CH_GetTextCommadnAttribute(void* rp_Command,S8 *rp_Buffer,U32 rul_BufferLength);

/* 根据属性字符串,获得 CH_KEY_ACTION_COMMAND 指令的属性
 * 参数: rp_Command,CH_KEY_ACTION_COMMAND 结构指针 
 * 其他参见 GET_KEY_ATTRIBUTE_FUNC 说明 */
CH_BOOL CH_GetKeyActionCommadnAttribute(void* rp_Command,S8 *rp_Buffer,U32 rul_BufferLength);

/* 根据属性字符串,获得 CH_IMG_COMMAND 指令的属性
 * 参数: rp_Command,CH_IMG_COMMAND 结构指针 
 * 其他参见 GET_KEY_ATTRIBUTE_FUNC 说明 */
CH_BOOL CH_GetImgCommadnAttribute(void* rp_Command,S8 *rp_Buffer,U32 rul_BufferLength);

/* 根据属性字符串,获得 CH_TEXTLINK_COMMAND 指令的属性
 * 参数: rp_Command,CH_TEXTLINK_COMMAND 结构指针 
 * 其他参见 GET_KEY_ATTRIBUTE_FUNC 说明 */
CH_BOOL CH_GetTextlinkCommadnAttribute(void* rp_Command,S8 *rp_Buffer,U32 rul_BufferLength);

/* 根据属性字符串,获得 CH_TIMEOUT_EXIST 指令的属性
 * 参数: rp_Command,CH_TIMEOUT_EXIST 结构指针 
 * 其他参见 GET_KEY_ATTRIBUTE_FUNC 说明 */
CH_BOOL CH_GetTimeOutExistCommadnAttribute(void* rp_Command,S8 *rp_Buffer,U32 rul_BufferLength);

/* 根据属性字符串,获得 CH_SCROLL_TEXT 指令的属性
 * 参数: rp_Command,CH_SCROLL_TEXT 结构指针 
 * 其他参见 GET_KEY_ATTRIBUTE_FUNC 说明 */
CH_BOOL CH_GetScrollTextCommadnAttribute(void* rp_Command,S8 *rp_Buffer,U32 rul_BufferLength);

/* 根据属性字符串,获得 CH_FORCE_ACTION 指令的属性
 * 参数: rp_Command,CH_FORCE_ACTION 结构指针 
 * 其他参见 GET_KEY_ATTRIBUTE_FUNC 说明 */
CH_BOOL CH_GetForceActionCommadnAttribute(void* rp_Command,S8 *rp_Buffer,U32 rul_BufferLength);
/**********************************************************************
** 从属性字符串中获得ID属性
** 参数: rpc_ID: 输出参数,存储解析的ID属性。已分配空间,缓冲大小是 ID_NAME_MAX_LENGTH+1,
**               如果解析出来的ID长度大于ID_NAME_MAX_LENGTH,则截去多余的部分
**       rp_Buffer: 属性字符串
**       rul_BufferLength: 属性字符串长度
** 返回: 成功返回 CH_TURE, 失败返回 CH_FALSE
**********************************************************************/
CH_BOOL CH_GetIDAttribute(S8* rpc_ID,S8 *rp_Buffer,U32 rul_BufferLength);

/**********************************************************************
** 从属性字符串中获得INIT属性.
** 参数: rp_Buffer: 属性字符串
**       rul_BufferLength: 属性字符串长度
** 返回值: 返回解析得到的INIT,如果属性字符串中没有INIT属性,则应该返回缺省值 2
**********************************************************************/
S32 CH_GetInitAttribute(S8 *rp_Buffer,U32 rul_BufferLength);

/**********************************************************************
** 从属性字符串中获得BkImg属性.
** 参数: rp_Buffer: 属性字符串
**       rul_BufferLength: 属性字符串长度
** 返回值: 返回解析得到的BkImg属性,如果属性字符串中没有INIT属性,则应该返回值0，
**         表示没有BkImg属性。
**********************************************************************/
U32 CH_GetBkImgAttribute(S8 *rp_Buffer,U32 rul_BufferLength);

/**********************************************************************
** 从属性字符串中获得Transparent属性.
** 参数: rp_Buffer: 属性字符串
**       rul_BufferLength: 属性字符串长度
** 返回值: 返回解析得到的Transparent属性,如果属性字符串中没有Transparent属性,
**         则应该返回缺省值 0
**********************************************************************/
U32 CH_GetTransparentAttribute(S8 *rp_Buffer,U32 rul_BufferLength);

/**********************************************************************
** 从属性字符串中获得Color属性.
** 参数: rp_Buffer: 属性字符串
**       rul_BufferLength: 属性字符串长度
** 返回值: 返回解析得到的Color属性
**********************************************************************/
U32 CH_GetColorAttribute(S8 *rp_Buffer,U32 rul_BufferLength);

/**********************************************************************
** 从属性字符串中获得Nowarp属性.
** 参数: rp_Buffer: 属性字符串
**       rul_BufferLength: 属性字符串长度
** 返回值: 返回解析得到的Nowarp属性,如果属性字符串中没有Nowarp属性,
**         则应该返回缺省值CH_FALSE
**********************************************************************/
CH_BOOL CH_GetNowarpAttribute(S8 *rp_Buffer,U32 rul_BufferLength);

/**********************************************************************
** 从属性字符串中获得Scrollbar属性.
** 参数: rp_Buffer: 属性字符串
**       rul_BufferLength: 属性字符串长度
** 返回值: 返回解析得到的Scrollbar属性,如果属性字符串中没有Scrollbar属性,
**         则应该返回缺省值CH_FALSE
**********************************************************************/
CH_BOOL CH_GetScrollbarAttribute(S8 *rp_Buffer,U32 rul_BufferLength);

/**********************************************************************
** 从属性字符串中获得Key(键值)属性.
** 参数: rp_Buffer: 属性字符串
**       rul_BufferLength: 属性字符串长度
** 返回值: 返回解析得到的Key属性
**********************************************************************/
U8 CH_GetKeyAttribute(S8 *rp_Buffer,U32 rul_BufferLength);

/**********************************************************************
** 从属性字符串中获得Img的src属性.
** 参数: rp_Buffer: 属性字符串
**       rul_BufferLength: 属性字符串长度
** 返回值: 返回解析得到的Img的src属性,如果属性字符串中没有src属性,则应该返回值0，
**         表示没有src属性。
**********************************************************************/
U32 CH_GetImgSrcAttribute(S8 *rp_Buffer,U32 rul_BufferLength);

/**********************************************************************
** 从属性字符串中获得IsDefaultfocus属性.
** 参数: rp_Buffer: 属性字符串
**       rul_BufferLength: 属性字符串长度
** 返回值: 返回解析得到的IsDefaultfocus属性,如果属性字符串中没有IsDefaultfocus属性,
**         则应该返回缺省值CH_FALSE
**********************************************************************/
CH_BOOL CH_GetIsDefaultfocusAttribute(S8 *rp_Buffer,U32 rul_BufferLength);

/**********************************************************************
** 从属性字符串中获得 CH_ACTION 相关属性
** 参数: rp_Action: 输出参数,存储解析的 CH_ACTION 的相关属性。已分配空间.
**                  CH_ACTION 的 enum_CommandTyp(CH_ENUM_COMMAND_TYPE)成员不在
**                  该函数中设置,而是在调用该函数时已经设置.
**                  对于enum_CommandTyp == e_COMMAND_AV的调用,函数应该设置
**                  CH_ACTION的数据成员enum_Action(CH_ENUM_ACTION_TYPE)为ACTION_AV
**       rp_Buffer: 属性字符串
**       rul_BufferLength: 属性字符串长度
** 返回: 成功返回 CH_TURE, 失败返回 CH_FALSE
**********************************************************************/
CH_BOOL CH_GetActionAttribute(PCH_ACTION rp_Action,	
						  S8 *rp_Buffer,
						  U32 rul_BufferLength);

/**********************************************************************
** 从属性字符串中获得 CH_SHOWRECT 相关属性
** 参数: rp_ShowRect: 输出参数,存储解析的 CH_SHOWRECT 相关属性。已分配空间.
**                  CH_SHOWRECT 的enum_CommandTyp(CH_ENUM_COMMAND_TYPE)成员不在
**                  该函数中设置.
**       rp_Buffer: 属性字符串
**       rul_BufferLength: 属性字符串长度
** 返回: 成功返回 CH_TURE, 失败返回 CH_FALSE
**********************************************************************/
CH_BOOL CH_GetShowRectAttribute(PCH_SHOWRECT rp_ShowRect,	
						  S8 *rp_Buffer,
						  U32 rul_BufferLength);

/**********************************************************************
** 从属性字符串中获得 CH_SCROLL_PARAMETERS 相关属性
** 参数: rp_ScrollParameters: 输出参数,存储解析的 CH_SCROLL_PARAMETERS 相关属性。已分配空间.
**                  CH_SCROLL_PARAMETERS 的enum_CommandTyp(CH_ENUM_COMMAND_TYPE)成员不在
**                  该函数中设置.
**       rp_Buffer: 属性字符串
**       rul_BufferLength: 属性字符串长度
** 返回: 成功返回 CH_TURE, 失败返回 CH_FALSE
**********************************************************************/
CH_BOOL CH_GetScrollParametersAttribute(PCH_SCROLL_PARAMETERS rp_ScrollParameters,	
						  S8 *rp_Buffer,
						  U32 rul_BufferLength);

/**********************************************************************
** 从属性字符串中获得time 属性.
** 参数: rp_Buffer: 属性字符串
**       rul_BufferLength: 属性字符串长度
** 返回值: 返回解析得到的time 属性,
**********************************************************************/
U32 CH_GetTimeAttribute(S8 *rp_Buffer,U32 rul_BufferLength);

/**********************************************************************
** 根据关键字,获得相应的属性处理函数
** 参数: rpc_Key: 关键字，函数处理时应该不区分大小写
** 返回: 成功,返回对应的属性处理函数，
**       失败,返回NULL
**********************************************************************/
GET_KEY_ATTRIBUTE_FUNC CH_GetAttributeHandleFunc(S8* rpc_Key);


/*以下为外部调用接口函数*/
/**********************************************************************
** 启动信息分析
** 参数: 
**      rui_PageId: 将要解析的文档的 PageID
**		rpc_DataBuffer: 待分析的缓冲
**		rul_DataLength: 缓冲的长度
** 返回:
**     成功返回非0,失败返回 0
***********************************************************************/
S32 CH_StartPageParse(U32 rui_PageId,S8* rpc_DataBuffer,U32 rul_DataLength);

/**********************************************************************
** 页面分析,主要是被 CH_StartPageParse 调用
** 参数: 
**      rui_PageId: 将要解析的文档的 PageID
**		rpc_DataBuffer: 待分析的缓冲
**		rul_DataLength: 缓冲的长度
** 返回:
**     成功返回非0,失败返回 0
***********************************************************************/
S32 CH_ParsePage(U32 rui_PageId,S8* rpc_DataBuffer,U32 rul_DataLength);

/**********************************************************************
** 处理关键字
** 参数: 
**		rpp_InOutCurrentBuf: 待分析的缓冲地址的指针,
**		rul_DataLength: 缓冲的长度
** 返回:
**     成功返回非0,rpp_InOutCurrentBuf缓冲的指针移到下一个要分析的字符位置
**     失败返回 0
***********************************************************************/
S32 CH_HandleKeyStart(S8** rpp_InOutCurrentBuf,U32 rul_CurrentBufLength);

/*初始化信息解析模块,创建或初始化全局变量等*/
S32 CH_InitInfoParser();

/*退出信息解析模块,释放相关资源*/
S32 CH_DeleteInfoParser();

/*得到文档的TimeOutExist指令,一个文档页只有一个TimeOutExist指令*/
PCH_TIMEOUT_EXIST CH_GetTimeOutExist();

/*得到文档的ForceAction指列表*/
PCH_GENERAL_ARRAY CH_GetForceActionArry();

/*设置文档的TimeOutExist指令,一个文档页只有一个TimeOutExist指令*/
void CH_SetTimeOutExist(PCH_TIMEOUT_EXIST rp_TimeOutExist);

/*得到文档的AV指令,一个文档页只有一个AV指令*/
PCH_AV_COMMAND CH_GetAVAction();

/*设置文档的AV指令,一个文档页只有一个AV指令*/
void CH_SetAVAction(PCH_AV_COMMAND rp_AvAction);

/*得到文档的KeyAction数组*/
PCH_GENERAL_ARRAY CH_GetKeyActionArray();

/*得到文档的XAction数组*/
PCH_GENERAL_ARRAY CH_GetXActionArray();

/*得到文档的YAction数组*/
PCH_GENERAL_ARRAY CH_GetYActionArray();

/*得到文档的ShowRect数组*/
PCH_GENERAL_ARRAY CH_GetShowRectArray();

/*得到文档的TEXT数组*/
PCH_GENERAL_ARRAY CH_GetTEXTArray();

PCH_GENERAL_ARRAY CH_GetTimerArray();

 PCH_GENERAL_ARRAY CH_GetGroupActionArray();


/*得到当前文档的PageID*/
U16 CH_GetCurrentDocumentID();

/*设置当前文档的PageID*/
void CH_SetCurrentDocumentID(U16 rui_PageID);

/*设置元素的z 顺序.返回原来的z 值*/
S32 CH_SetZCommand(PCH_SHOWRECT rp_ShowRect,S32 rl_Z);

/*******************************************************************
** 删除当前文档页面的数据: 删除gsp_KeyAction,gsp_ShowRect等数组中的指令数据.
** 清除gsp_KeyAction,gsp_XAction,gsp_YAction,gsp_ShowRect,gsp_TEXT等数组
** 为空数组(数组结构并没有清除,只是数组中没有元素，元素个数为0，数组还是有效)。
** 设置 gsp_AVAction = NULL,gsui_CurrentPageID = 0;
** 参数: 无
** 返回: 无
/*******************************************************************/
void CH_DestroyCurrentIDPage();


/*******************************************************************
* 功能： 这个函数用来在rp_Buffer属性串中才查找rp_SubString关键字，找到则将结
*        果返回到rp_Result中.
*  输入参数： rp_Buffuer;属性字符串
*            rp_SubStr;关键字
*            rul_BufferLen;属性字符串长度
*            rul_SubStrLen;关键字长度
* 输出参数：
*            rp_Result;返回关键字的属性值
* 返回:  成功：返回rp_Resultd的长度
*        失败：返回-1
/*******************************************************************/
U32 CH_GetKeywordAttributeStr(S8 *rp_Buffer,
                         S8 *rp_SubString,
						 S8 *rp_Result,
						 U32 rul_ResultLen,
						 U32 rul_BufferLength,
						 U32 rul_SubStrLen);


/**********************************************************************
** 判断是不是闭包指令
** 参数: 
**        sz_KeyWord: 待判断的指令;
** 返回:
**     成功返回true, 失败返回 false;
***********************************************************************/
CH_BOOL CH_IsKeywordClose(S8 * sz_KeyWord);

/*******************************************************************************/
/*函数功能：
*           比较.CH_ATION中的uc_Key
*参数列表:
*		    pElement1 ：要比较的第1个元素 
*		    pElement2 ：要比较的第2个元素 
*		    pOtherArg ：比较需要的其他任意参数
* 返回值:
* 	< 0     :     表示 pElement1 小于  pElement2
*	0       :     表示 pElement1 等于  pElement2
*	> 0     :     表示 pElement1 大于  pElement2
****************************************函数体定义*****************************/
S32 CH_KeyActionComp(void* pElement1,void* pElement2,void* pOtherArg);

/****************************函数CH_ShowRectComp的定义**************************
*函数功能：
*           比较CH_SHOWRECT中的l_Z;
*参数列表:
*		    pElement1 ：要比较的第1个元素 
*		    pElement2 ：要比较的第2个元素 
*		    pOtherArg ：比较需要的其他任意参数
* 返回值:
* 	< 0     :     表示 pElement1 小于  pElement2
*	0       :     表示 pElement1 等于  pElement2
*	> 0     :     表示 pElement1 大于  pElement2
****************************************函数体定义*****************************/
S32 CH_ShowRectComp(void* pElement1,void* pElement2,void* pOtherArg);
/*************************函数CH_StructActionComp的定义**************************
*函数功能：
*           比较CH_StructActionComp中的xx;
*参数列表:
*		    pElement1 ：要比较的第1个元素 
*		    pElement2 ：要比较的第2个元素 
*		    pOtherArg ：比较需要的其他任意参数
* 返回值:
* 	< 0     :     表示 pElement1 小于  pElement2
*	0       :     表示 pElement1 等于  pElement2
*	> 0     :     表示 pElement1 大于  pElement2
****************************************函数体定义*****************************/
S32 CH_StructActionComp(void* pElement1,void* pElement2,void* pOtherArg);

/*元素的内存释放函数*/
void CH_FreeArrayShowAreaElement(void* pElement, void *pOtherArg);


/**********************************************************************
** 从属性字符串中获得cardidtype 属性.
** 参数: rp_Buffer: 属性字符串
**       rul_BufferLength: 属性字符串长度
** 返回值: 返回解析得到的time 属性,
**********************************************************************/
U16 CH_GetTypeAttribute(S8 *rp_Buffer,U32 rul_BufferLength);

/* 根据属性字符串,获得 CH_CARDID_ACTION 指令的属性
 * 参数: rp_Command,CH_CARDID_ACTION 结构指针 
 * 其他参见 GET_KEY_ATTRIBUTE_FUNC 说明 */
CH_BOOL CH_GetCardIDCommadnAttribute(void* rp_Command,S8 *rp_Buffer,U32 rul_BufferLength);


/* 根据属性字符串,获得 CH_STBID_ACTION 指令的属性
 * 参数: rp_Command,CH_STBID_ACTION 结构指针 
 * 其他参见 GET_KEY_ATTRIBUTE_FUNC 说明 */
CH_BOOL CH_GetSTBIDCommadnAttribute(void* rp_Command,S8 *rp_Buffer,U32 rul_BufferLength);

/*获得CARDIDTEXT 或 STBIDTEXT的属性*/
CH_BOOL CH_GetCardIDTextCommadnAttribute(void* rp_Command,S8 *rp_Buffer,U32 rul_BufferLength);

/*获得CARDIDTEXT 或 STBIDTEXT的属性*/
CH_BOOL CH_GetSTBIDTextCommadnAttribute(void* rp_Command,S8 *rp_Buffer,U32 rul_BufferLength);

 PCH_CARDID_ACTION CH_GetCardID_Action();

 PCH_STBID_ACTION CH_GetSTBID_Action();
 
 CH_BOOL CH_GetNumInputAttribute(void* rp_NumInput,S8 *rp_Buffer,U32 rul_BufferLength);	
 
 CH_BOOL CH_GetCopyRightCommadnAttribute(void* rp_Command,S8 *rp_Buffer,U32 rul_BufferLength);

 CH_BOOL CH_GetVChannelCommadnAttribute(void* rp_Command,S8 *rp_Buffer,U32 rul_BufferLength);

     /* 根据属性字符串,获得 CH_GroupAction 指令的属性
  * 参数: rp_Command,CH_STBID_ACTION 结构指针 
 * 其他参见 GET_KEY_ATTRIBUTE_FUNC 说明 */
 CH_BOOL CH_GetGroupActionAttribute(void* rp_Command,S8 *rp_Buffer,U32 rul_BufferLength);
 CH_BOOL CH_GetLinkResourceInfoAttribute(void* rp_Command,S8 *rp_Buffer,U32 rul_BufferLength);


#endif






