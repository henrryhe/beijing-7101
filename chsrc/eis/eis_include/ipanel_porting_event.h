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

#ifndef _IPANEL_MIDDLEWARE_PORTING_EVENT_H_
#define _IPANEL_MIDDLEWARE_PORTING_EVENT_H_

/**
 * messages same in Event[0] area.
 */
enum
{
    IPANEL_EVENT_TYPE_TIMER        = 0x000,
    IPANEL_EVENT_TYPE_SYSTEM,
    IPANEL_EVENT_TYPE_KEYDOWN,
    IPANEL_EVENT_TYPE_KEYUP,
    IPANEL_EVENT_TYPE_NETWORK,
    IPANEL_EVENT_TYPE_CHAR         = 0x005,
    IPANEL_EVENT_TYPE_MOUSE,
    IPANEL_EVENT_TYPE_IRKEY,
    IPANEL_EVENT_TYPE_MKEY,
    IPANEL_EVENT_TYPE_AUDIO,
    IPANEL_EVENT_TYPE_MULTIFNIRKEY,			/* 多功能遥控器 */

    IPANEL_EVENT_TYPE_DVB          = 0x100,
    IPANEL_EVENT_TYPE_IPTV,
    IPANEL_EVENT_TYPE_CAM
};

enum
{
	IPANEL_IRKEY_NULL      = 0x0100/* bigger than largest ASCII*/,
	IPANEL_IRKEY_POWER     = 0x0101,
	IPANEL_IRKEY_STANDBY,

	/* generic remote controller keys */
	IPANEL_IRKEY_NUM0      = 0x110,
	IPANEL_IRKEY_NUM1,
	IPANEL_IRKEY_NUM2,
	IPANEL_IRKEY_NUM3,
	IPANEL_IRKEY_NUM4,
	IPANEL_IRKEY_NUM5,     // 0x115
	IPANEL_IRKEY_NUM6,
	IPANEL_IRKEY_NUM7,
	IPANEL_IRKEY_NUM8,
	IPANEL_IRKEY_NUM9,
	IPANEL_IRKEY_UP,       // 0x11A
	IPANEL_IRKEY_DOWN,
	IPANEL_IRKEY_LEFT,
	IPANEL_IRKEY_RIGHT,
	IPANEL_IRKEY_SELECT,   // 0x11E

	/* generic editting/controling keys */
	IPANEL_IRKEY_INSERT    = 0x130,
	IPANEL_IRKEY_DELETE,
	IPANEL_IRKEY_HOME,
	IPANEL_IRKEY_END,
	IPANEL_IRKEY_ESC,
	IPANEL_IRKEY_CAPS,     // 0x135

	IPANEL_IRKEY_PREVLINK  = 0x150,
	IPANEL_IRKEY_NEXTLINK,
	IPANEL_IRKEY_REFRESH,
	IPANEL_IRKEY_EXIT,
	IPANEL_IRKEY_BACK,     /*backspace*/
	IPANEL_IRKEY_CANCEL,

	/* generic navigating keys */
	IPANEL_IRKEY_SCROLL_UP     = 0x170,
	IPANEL_IRKEY_SCROLL_DOWN,
	IPANEL_IRKEY_SCROLL_LEFT,
	IPANEL_IRKEY_SCROLL_RIGHT,
	IPANEL_IRKEY_PAGE_UP,
	IPANEL_IRKEY_PAGE_DOWN,
	IPANEL_IRKEY_HISTORY_FORWARD,
	IPANEL_IRKEY_HISTORY_BACKWARD,
	IPANEL_IRKEY_SHOW_URL,

	/* function remote controller keys */
	IPANEL_IRKEY_HOMEPAGE = 0x200,
	IPANEL_IRKEY_MENU,
	IPANEL_IRKEY_EPG,
	IPANEL_IRKEY_HELP,
	IPANEL_IRKEY_MOSAIC,
	IPANEL_IRKEY_VOD,          	
	IPANEL_IRKEY_NVOD,
	IPANEL_IRKEY_SETTING,
	IPANEL_IRKEY_STOCK,			// 0x208

	/* special remote controller keys */
	IPANEL_IRKEY_SOFT_KEYBOARD = 0x230,
	IPANEL_IRKEY_IME,
	IPANEL_IRKEY_DATA_BROADCAST_BK,
	IPANEL_IRKEY_VIDEO,            /*视频键*/
	IPANEL_IRKEY_AUDIO,            /*音频键*/
	IPANEL_IRKEY_LANGUAGE,     // 0x235
	IPANEL_IRKEY_SUBTITLE,
	IPANEL_IRKEY_INFO,
	IPANEL_IRKEY_RECOMMEND,        /*推荐键*/
	IPANEL_IRKEY_FORETELL,         /*预告键*/
	IPANEL_IRKEY_FAVORITE,         /*收藏键*/
	IPANEL_IRKEY_DATA_BROADCAST = 0x23B,
	IPANEL_IRKEY_PLAYLIST=0x23D,
	/* user controling remote controller keys */
	IPANEL_IRKEY_LAST_CHANNEL  = 0x250,
	IPANEL_IRKEY_CHANNEL_UP,
	IPANEL_IRKEY_CHANNEL_DOWN,
	IPANEL_IRKEY_VOLUME_UP,
	IPANEL_IRKEY_VOLUME_DOWN,
	IPANEL_IRKEY_VOLUME_MUTE,
	IPANEL_IRKEY_AUDIO_MODE,

	/* virtual function keys */
	IPANEL_IRKEY_VK_F1 = 0x300,
	IPANEL_IRKEY_VK_F2,
	IPANEL_IRKEY_VK_F3,
	IPANEL_IRKEY_VK_F4,
	IPANEL_IRKEY_VK_F5,
	IPANEL_IRKEY_VK_F6,        // 0x305
	IPANEL_IRKEY_VK_F7,
	IPANEL_IRKEY_VK_F8,
	IPANEL_IRKEY_VK_F9,
	IPANEL_IRKEY_VK_F10,
	IPANEL_IRKEY_VK_F11,       // 0x30A
	IPANEL_IRKEY_VK_F12,

	/* special function keys class A */
	IPANEL_IRKEY_FUNCTION_A = 0x320,
	IPANEL_IRKEY_FUNCTION_B,
	IPANEL_IRKEY_FUNCTION_C,
	IPANEL_IRKEY_FUNCTION_D,
	IPANEL_IRKEY_FUNCTION_E,
	IPANEL_IRKEY_FUNCTION_F,

	/* special function keys class B */
	IPANEL_IRKEY_RED = 0x340,
	IPANEL_IRKEY_GREEN,
	IPANEL_IRKEY_YELLOW,
	IPANEL_IRKEY_BLUE,

	IPANEL_IRKEY_ASTERISK = 0x350,
	IPANEL_IRKEY_NUMBERSIGN,

	/* VOD/DVD controling keys */
	IPANEL_IRKEY_PLAY          = 0x400,
	IPANEL_IRKEY_STOP,
	IPANEL_IRKEY_PAUSE,
	IPANEL_IRKEY_RECORD,
	IPANEL_IRKEY_FASTFORWARD,
	IPANEL_IRKEY_REWIND,
	IPANEL_IRKEY_STEPFORWARD,
	IPANEL_IRKEY_STEPBACKWARD,
	IPANEL_IRKEY_DVD_AB,
	IPANEL_IRKEY_DVD_MENU,
	IPANEL_IRKEY_DVD_TITILE,
	IPANEL_IRKEY_DVD_ANGLE,
	IPANEL_IRKEY_DVD_ZOOM,
	IPANEL_IRKEY_DVD_SLOW,
	IPANEL_IRKEY_TV_SYSTEM,
	IPANEL_IRKEY_DVD_EJECT
};

#define MAKE_MOUSE_POS(t, x, y)     (((((long)(x)) & 0xffff) << 16) | ((((long)(y)) & 0xffff)))
#define GET_MOUSE_XPOS(e)           (((e) >> 16) & 0xffff)
#define GET_MOUSE_YPOS(e)           ((e) & 0xffff)

enum
{
    IPANEL_MOUSE_NONE,
    IPANEL_MOUSE_MOUSEMOVE,
    IPANEL_MOUSE_LBUTTONDOWN,
    IPANEL_MOUSE_LBUTTONUP,
    IPANEL_MOUSE_MBUTTONDOWN,
    IPANEL_MOUSE_MBUTTONUP,
    IPANEL_MOUSE_RBUTTONDOWN,
    IPANEL_MOUSE_RBUTTONUP,
    IPANEL_MOUSE_LBUTTONDCLICK,
    IPANEL_MOUSE_RBUTTONDCLICK,
    IPANEL_MOUSE_UNDEFINED
};

enum
{
	IPANEL_DVB_CABLE_CONNECT = 5550,
	IPANEL_DVB_CABLE_DISCONNECT,
	IPANEL_DVB_TUNE_SUCCESS = 8001,
	IPANEL_DVB_TUNE_FAILED
}IPANEL_DVB_STATUS;

enum
{
	EIS_DEVICE_USB_INSERT					= 6001,
	EIS_DEVICE_USB_INSTALL					= 6002,
	EIS_DEVICE_USB_DELETE					= 6003,
	EIS_DEVICE_USB_UNKNOWN
};

/* network specific events */
enum {
    IPANEL_IP_NETWORK_CONNECT          = 5500,
    IPANEL_IP_NETWORK_DISCONNECT    = 5501,
    IPANEL_IP_NETWORK_READY 		    = 5502,
    IPANEL_IP_NETWORK_FAILED					= 5503,		/*logistic signal*/
    IPANEL_IP_NETWORK_DHCP_MODE				= 5504,		/*logistic signal*/
    
    IPANEL_IP_NETWORK_NTP_READY      = 5510,
    IPANEL_IP_NETWORK_NTP_TIMEOUT  = 5511,

    IPANEL_IP_NETWORK_PING_RESPONSE = 5520,      /*added by yuancp for PING  at 0316*/
    
    IPANEL_CABLE_NETWORK_CONNECT    = 5550,
    IPANEL_CABLE_NETWORK_DISCONNECT  = 5551,
 
    IPANEL_CABLE_NETWORK_CM_DISCONNECTED = 5560,   /*CM未上线 */
    IPANEL_CABLE_NETWORK_CM_CONNECTED = 5561,      /*CM已上线*/
    IPANEL_CABLE_NETWORK_CM_CONNECTING = 5562,    /*正在搜索， 此时P2值为freq*/
 IPANEL_DEVICE_DECODER_NORMAL  =6050,  /*decoder正常工作,没有异常状态.解码器从任何异常恢复后发送此消息.*/
	IPANEL_DEVICE_DECODER_HUNGER = 6051, /*	  decoder没有数据输入，处于饥饿状态*/
EIS_CABLE_NETWORK_CM_UPLINK_STATUS		= 5563,
    EIS_CABLE_NETWORK_CM_DOWNLINK_STATUS	= 5564,  
    IPANEL_NETWORK_UNDEFINED
};

enum
{
    IPANEL_CA_SMARTCARD_INSERT         = 5300,
    IPANEL_CA_SMARTCARD_EVULSION,

    IPANEL_CA_MESSAGE_OPEN             = 5350,
    IPANEL_CA_MESSAGE_CLOSE,

    IPANEL_CA_UNDEFINED
};

/* 杭州VOD使用到的消息 */
enum
{
    IPANEL_VOD_GETDATA_SUCCESS          = 5200, /* 下载数据成功,请求返回时移频道列表 */
    IPANEL_VOD_GETDATA_FAILED           = 5201, /* 下载数据失败,请求未返回时移频道列表 */
    IPANEL_VOD_PREPAREPLAY_SUCCESS      = 5202, /* 准备播放媒体成功 */
    IPANEL_VOD_CONNECT_FAILED           = 5203, /* 连接服务器失败,建立会话失败或者服务器返回超时 */
    IPANEL_VOD_DVB_SERACH_FAILED        = 5204, /* DVB在VOD指定的IPQAM中搜索数据失败 */
    IPANEL_VOD_PLAY_SUCCESS             = 5205, /* 播放媒体成功 */
    IPANEL_VOD_PLAY_FAILED              = 5206, /* 播放媒体失败 */
    IPANEL_VOD_PREPARE_PROGRAMS_SUCCESS = 5207, /* 更新时移频道的节目列表成功 */
    IPANEL_VOD_PREPARE_PROGRAMS_FAILED  = 5208, /* 更新时移频道的节目列表失败 */
    IPANEL_VOD_PROGRAM_BEGIN            = 5209, /* 时移频道或VOD影片播放到了点播开始的位置 */
    IPANEL_VOD_PROGRAM_END              = 5210, /* 时移频道或VOD影片播放到了点播结束的位置 */
    IPANEL_VOD_RELEASE_SUCCESS           		= 5211, 	//only DMX need
    IPANEL_VOD_RELEASE_FAILED            		= 5212, 	//only DMX need
    IPANEL_VOD_TSTVREQUEST_FAILED       = 5213,  /* 时移请求服务器失败 */
    IPANEL_VOD_EPGURL_REQUEST_FINILISHED 		= 5214, 	//hangzhou don't need
    IPANEL_VOD_CHANGE_SUCCESS            		= 5220, 	//以后要改成IPANEL_VOD_START_SUCCESS
    IPANEL_VOD_CHANGE_FAIL               		= 5221, 	//以后要改成IPANEL_VOD_START_FAILED
    IPANEL_VOD_START_BUFF                		= 5222,
    IPANEL_VOD_STOP_BUFF                 		= 5223,
    IPANEL_VOD_OUT_OF_RANGE              		= 5224,
    IPANEL_VOD_USER_EXCEPTION            		= 5225,
    IPANEL_VOD_UNDEFINED
};

/*	EIS_EVENT_TYPE_MULTIFNIRKEY,	 多功能遥控器 p1 */
enum {
    IPANEL_MULTIFNIRKEY_MOTION			  		= 3700,
    IPANEL_MULTIFNIRKEY_POSITION		  		= 3701
};

/*网络连接失败（如分配不到IP地址等）。 注意：底层发送的消息类型为EIS_EVENT_TYPE_NETWORK。
P2取值如下： */
enum {
    IPANEL_NETWORK_ERR_DHCP_IP = 10, /* 表示未搜索到DHCP服务或未获取到IP。 */
    IPANEL_NETWORK_ERR_DHCP_GATEWAY =11,/*未获取到网关地址?*/
    IPANEL_NETWORK_ERR_DHCP =12,/* DHCP租期到但续约未成功。*/
    IPANEL_NETWORK_ERR_IP_INTER= 20,/*IP设置错误（如IP冲突等）。 */
    IPANEL_NETWORK_ERR_MASK =21,/* 子网掩码错误（如网络号非全1或主机号非全0等）。*/
    IPANEL_NETWORK_ERR_GATEWAY =22,/* 网关错误（如与主机IP 不在同一网段等）。*/
    IPANEL_NETWORK_ERR_OTHER =10,/* 其它手动配置错误。*/
};

#endif//_IPANEL_MIDDLEWARE_PORTING_EVENT_H_

