/**
 * @brief    视频云系统终端移植库头文件
 * @version  1.3.1
 * @date     2012.07.01
 */

#ifndef CYBER_CLOUD_API_H
#define CYBER_CLOUD_API_H

#ifdef __cplusplus
extern "C"
{
#endif

#define IN                                  ///<输入
#define OUT                                 ///<输出
#define INOUT                               ///<输入输出

/// 约束指标
#define CLOUD_VERSION_LEN           24      ///<版本信息长度
#define CLOUD_VENDOR_LEN            32      ///<最大厂商信息长度
#define CLOUD_KEYBOARD_VALUE_SIZE   6       ///<键盘按键值的大小
#define CLOUD_JOYSTICK_VALUE_SIZE   4       ///<游戏杆按键值的大小
#define MSG_MAX_LEN                 1024    ///<简单提示信息最大长度

///FLASH类型
#define CLOUD_FLASH_BLOCK_A        1        ///<BLOCK A，存储空间大小为64KBytes
#define CLOUD_FLASH_BLOCK_B        2        ///<BLOCK B，存储空间大小为256KBytes

/// 返回代码
#define CLOUD_OK                    0x0000  ///<成功
#define CLOUD_FAILURE               0x0001  ///<失败
#define CLOUD_TIMEOUT               0x8001  ///<超时

#define C_TRUE  1
#define C_FALSE 0

/// 基本数据类型
typedef signed char    C_S8;
typedef unsigned char  C_U8;
typedef unsigned short C_U16;
typedef unsigned long  C_U32;
typedef unsigned long  C_RESULT;            ///<具体含义见返回代码
typedef int            C_BOOL;

/// 信号量句柄
typedef void* C_SemaphoreHandle;

/// 线程句柄
typedef void * C_ThreadHandle;

/// 遥控器按键定义
typedef enum
{
    CloudKey_0              = 0x27, ///<数字键0
    CloudKey_1              = 0x1E, ///<数字键1
    CloudKey_2              = 0x1F, ///<数字键2
    CloudKey_3              = 0x20, ///<数字键3
    CloudKey_4              = 0x21, ///<数字键4
    CloudKey_5              = 0x22, ///<数字键5
    CloudKey_6              = 0x23, ///<数字键6
    CloudKey_7              = 0x24, ///<数字键7
    CloudKey_8              = 0x25, ///<数字键8
    CloudKey_9              = 0x26, ///<数字键9
    CloudKey_OK             = 0x28, ///<确认键.
    CloudKey_Back           = 0x29, ///<返回键.
    CloudKey_Up             = 0x52, ///<上键.
    CloudKey_Down           = 0x51, ///<下键.
    CloudKey_Left           = 0x50, ///<左键.
    CloudKey_Right          = 0x4F, ///<右键.
    CloudKey_PageUp         = 0x4B, ///<向上翻页键.
    CloudKey_PageDown       = 0x4E, ///<向下翻页键.
    CloudKey_Red            = 0xA1, ///<红色功能键.
    CloudKey_Green          = 0xA2, ///<绿色功能键.
    CloudKey_Yellow         = 0xA3, ///<黄色功能键.
    CloudKey_Blue           = 0xA4, ///<蓝色功能键.
    CloudKey_ChannelUp      = 0xA5, ///<频道加键.
    CloudKey_ChannelDown    = 0xA6, ///<频道减键.
    CloudKey_Forward        = 0xA7, ///<快进键.
    CloudKey_Backward       = 0xA8, ///<快退键.
    CloudKey_Play           = 0xA9, ///<播放键.
    CloudKey_Pause          = 0xAA, ///<暂停键.
    CloudKey_PlayPause      = 0xAB, ///<播放/暂停键.
    CloudKey_Stop           = 0xAC, ///<停止键.
    ///山东网络项目开始增加的按键
    CloudKey_Track			= 0xAD, ///<声道键
    CloudKey_Music			= 0xAE, ///<音乐键
    CloudKey_Av				= 0xAF, ///<电视键
    CloudKey_Setup			= 0xB0, ///<设置键
    CloudKey_LiveTv			= 0xB1, ///<直播键
    CloudKey_Vod			= 0xB2,	///<点播键
    CloudKey_Info			= 0xB3, ///<信息键
    CloudKey_Desktop		= 0xB4, ///<桌面键
	CloudKey_BreakBack		= 0xB5,	///<回切键
	CloudKey_Location		= 0xB6,	///<定位键
	CloudKey_Help			= 0xB7, ///<帮助键
	CloudKey_Store			= 0xB8, ///<收藏键
	CloudKey_Star			= 0xB9, ///<*键
	CloudKey_Pound			= 0xBA, ///<#键
	CloudKey_F1				= 0xBB, ///<F1键
	CloudKey_F2				= 0xBC, ///<F2键
	CloudKey_F3				= 0xBD, ///<F3键
	CloudKey_F4				= 0xBE, ///<F4键
    ///当采用移植库内部绘制界面的机制时，需要将以下五个按键传给移植库处理
    CloudKey_Menu           = 0xF0, ///<菜单键.
    CloudKey_Exit           = 0xF1, ///<退出键.
    CloudKey_VolUp          = 0xF2, ///<音量加键.
    CloudKey_VolDown        = 0xF3, ///<音量减键.
    CloudKey_Mute           = 0xF4  ///<静音键.
}C_RCKey;

/// 网络IP地址
typedef C_U32 C_NetworkIP;               ///<字节顺序为网络字节顺序，即大端模式

/// 网络端口号
typedef C_U16 C_NetworkPort;             ///<字节顺序为网络字节顺序，即大端模式

/// 套接字地址
typedef struct
{
    C_NetworkIP   uIP;                   ///<网络IP地址
    C_NetworkPort uPort;                 ///<网络端口号
}C_SocketAddr;

/// 套接字类型
typedef enum
{
    SocketType_Stream = 1,               ///<数据流套接字
    SocketType_Datagram                  ///<数据报套接字
}C_SocketType;

///套接字设置选项的层次
typedef enum
{
    SocketOptLevel_SOL_SOCKET = 0,  ///<套接字接口层
    SocketOptLevel_IPPROTO_TCP,     ///<TCP层
    SocketOptLevel_FILEIO,          ///<文件IO层
}C_SocketOptLevel;

///套接字设置选项
typedef enum
{
    SocketOptName_SO_BROADCAST,     ///<参数类型:C_BOOL;允许套接字传送广播信息
    SocketOptName_SO_RCVBUF,        ///<参数类型:C_U32;设置接收缓冲区大小
    SocketOptName_SO_SNDBUF,        ///<参数类型:C_U32;设置发送缓冲区大小
    SocketOptName_SO_REUSEADDR,     ///<参数类型:C_BOOL;允许套接口和一个已在使用中的地址捆绑,即socket可重用
    SocketOptName_TCP_NODELAY,      ///<参数类型:C_BOOL;禁止Nagle算法
    SocketOptName_FILEIO_NBIO,      ///<参数类型:C_BOOL;禁止阻塞模式
}C_SocketOptName;

/// 套接字句柄
typedef void* C_SocketHandle;

/// 调制方式
typedef enum
{
    ModType_QAM16 = 1,     ///<16 QAM
    ModType_QAM32,         ///<32 QAM
    ModType_QAM64,         ///<64 QAM
    ModType_QAM128,        ///<128 QAM
    ModType_QAM256         ///<256 QAM
}C_ModulationType;

/// 视频类型
typedef enum
{
    VideoType_MPEG2 = 1,
    VideoType_H264
}C_VideoType;

/// 音频类型
typedef enum
{
    AudioType_MPEG2 = 1,
    AudioType_MP3,
    AudioType_MPEG2_AAC
}C_AudioType;

/// 数据流类型
typedef enum
{
    StreamType_TS = 1,
    StreamType_RTP
}C_StreamType;

/// 有线传输频点参数
typedef struct
{
    C_U32                    uFrequency;        ///<频率，单位：MHz
    C_U32                    uSymbolRate;       ///<符号率，单位：KSymbol/s
    C_ModulationType         uModulationType;   ///<调制方式
} C_FreqParam;

/// PID传输参数
typedef struct
{
    C_VideoType         uVideoType;     ///<视频类型
    C_U16               uVideoPid;      ///<视频PID
    C_AudioType         uAudioType;     ///<音频类型
    C_U16               uAudioPid;      ///<音频PID
    C_U16               uPcrPid;        ///<PCR PID
} C_PIDParam;

/// 消息代码
typedef enum
{
    MsgCode_ShowInfo,    ///<显示信息
    MsgCode_HideInfo,    ///<隐藏信息
    MsgCode_ShowCursor,  ///<显示光标，如果终端支持光标层，则需要传递光标的绝对坐标消息
    MsgCode_HideCursor   ///<隐藏光标，根据终端实际情况，传递相对坐标或者绝对坐标消息
} C_MessageCode;

/// 按键类型
typedef enum
{
    KeyType_Unknown    = 0,     ///<未知设备
    KeyType_HID        = 1,     ///<HID类设备
    KeyType_Keyboard   = 2,     ///<键盘
    KeyType_MouseRel   = 3,     ///<相对坐标鼠标
    KeyType_MouseAbs   = 4,     ///<绝对坐标鼠标（含单点触摸屏）
    KeyType_Joystick   = 5,     ///<游戏杆
    KeyType_MultiTouch = 6,     ///<多点触控输入设备
    KeyType_Flingpc    = 7,     ///<鼎亿体感手柄   
    KeyType_RC         = 8      ///<遥控器
}C_KeyType;

typedef struct
{
    C_U8        uButton;    ///<按键指示:值为1时，表示按键被按下;值为0时，表示按键弹起;
    C_RCKey     uKeyValue;  ///<按键值
} C_RC;

/**
 * @brief  键盘按键消息
 * @param    uModifierKey 特殊按键指示，指示位为1时，表示该键为按下状态
 *             bit0 ---- left  ctrl
 *             bit1 ---- left  shift
 *             bit2 ---- left  alt
 *             bit3 ---- left  windows
 *             bit4 ---- right ctrl
 *             bit5 ---- right shift
 *             bit6 ---- right alt
 *             bit7 ---- right windows
 * @param    uLeds        键盘LED灯指示，指示位为1时，表示灯被点亮
 *             bit0 ---- caps   lock
 *             bit1 ---- num    lock
 *             bit2 ---- scroll lock
 *             bit3~bit7 ---- 0
 * @param    uKeyValue    键盘值
 */
typedef struct
{
    C_U32   handle_;
    C_U8    uModifierKey;                         ///< 特殊按键指示，此参数在按键输入消息时有效
    C_U8    uLeds;                                ///< 键盘LED灯指示，此参数在外设指示消息时有效
    C_U8    uKeyValue[CLOUD_KEYBOARD_VALUE_SIZE]; ///< 键盘值，此参数在按键输入消息时有效
} C_Keyboard;

/**
 * @brief    相对坐标鼠标消息
 * @param    uButton 鼠标按键指示，指示位值为1时，表示按键被按下
 *             bit0 ---- button1 鼠标 左键,按下时uButton = 0x01
 *             bit1 ---- button2 鼠标 右键,按下时uButton = 0x02
 *             bit2 ---- button3 鼠标滚轴键,按下时uButton = 0x04
 *             bit3~bit7 ---- 0
 * @param    uXPosition 鼠标移动的X轴距离，向右移动距离值为正值，向左移动距离值为负值
 * @param    uYPosition 鼠标移动的Y轴距离，向下移动距离值为正值，向上移动距离值为负值
 * @param    uWheel     鼠标滚轮移动的距离，向上滚动一次，距离值为+1，向下滚动一次，距离值为-1
 */
typedef struct
{
    C_U32               handle_;
    C_U8                uButton;      ///<鼠标按键指示
    C_S8                uXPosition;   ///<鼠标移动的X轴距离
    C_S8                uYPosition;   ///<鼠标移动的Y轴距离
    C_S8                uWheel;       ///<鼠标滚轮移动的距离
} C_MouseRel;

/**
 * @brief    绝对坐标鼠标消息
  * @param    uButton 按键指示，指示位值为1时，表示按键被按下
 *             bit0 ---- button1
 *             bit1 ---- button2
 *             bit2~bit7 ---- 0
 * @param    uXPosition 相对于原点的X轴距离，绝对坐标
 * @param    uYPosition 相对于原点的X轴距离，绝对坐标
 * @param    uWheel     滚轮移动的距离，向上滚动一次，距离值为+1，向下滚动一次，距离值为-1
 * @note     终端视频云服务的显示区对应的绝对坐标范围为：0-4096，
             如果实际设备的绝对坐标范围不同，需要进行转换。
*/
typedef struct
{
    C_U32       handle_;
    C_U8        uButton;    ///<按键指示
    C_U16       uXPosition; ///<移动的X轴距离
    C_U16       uYPosition; ///<移动的Y轴距离
    C_S8        uWheel;     ///<滚轮移动的距离
} C_MouseAbs;

/**
 * @brief    定义标准游戏杆输入数据格式
 * @param    uJoystickId 游戏杆ID（固定值：1-第一个游戏杆，2-第二个游戏杆，3-第三个游戏杆，4-第四个游戏杆）
 * @param    uX  X 轴位置，默认值为0x7F，向左移动，值会连续减少至0x00，向右移动，值会连续增加至0xFF
 * @param    uY  Y 轴位置，默认值为0x7F，向上移动，值会连续减少至0x00，向下移动，值会连续增加至0xFF
 * @param    uZ  Z 轴位置，默认值为0x7F，向前移动，值会连续减少至0x00，向后移动，值会连续增加至0xFF
 * @param    uRx Rx轴位置，默认值为0x7F，向左旋转，值会连续减少至0x00，向右旋转，值会连续增加至0xFF
 * @param    uRy Ry轴位置，默认值为0x7F，向上旋转，值会连续减少至0x00，向右旋转，值会连续增加至0xFF
 * @param    uRz Rz轴位置，默认值为0x7F，向前旋转，值会连续减少至0x00，向后旋转，值会连续增加至0xFF
 * @param    uHatSwitch 瞄准镜头盔
 *             0 ---- 0/360 degree
 *             1 ---- 45 degree
 *             2 ---- 90 degree
 *             3 ---- 135 degree
 *             4 ---- 185 degree
 *             5 ---- 230 degree
 *             6 ---- 275 degree
 *             7 ---- 315 degree
 *             8 ---- 无效
 * @param    uButton[0] 按键指示，指示位为1时，表示该键被按下
 *             bit0 ---- button1
 *             bit1 ---- button2
 *             bit2 ---- button3
 *             bit3 ---- button4
 *             bit4 ---- button5
 *             bit5 ---- button6
 *             bit6 ---- button7
 *             bit7 ---- button8
 * @param    uButton[1] 按键指示，指示位为1时，表示该键被按下
 *             bit0 ---- button9
 *             bit1 ---- button10
 *             bit2 ---- button11
 *             bit3 ---- button12
 *             bit4 ---- button13
 *             bit5 ---- button14
 *             bit6 ---- button15
 *             bit7 ---- button16
 * @param    uButton[2] 按键指示，指示位为1时，表示该键被按下
 *             bit0 ---- button17
 *             bit1 ---- button18
 *             bit2 ---- button19
 *             bit3 ---- button20
 *             bit4 ---- button21
 *             bit5 ---- button22
 *             bit6 ---- button23
 *             bit7 ---- button24
 * @param    uButton[3] 按键指示，指示位为1时，表示该键被按下
 *             bit0 ---- button25
 *             bit1 ---- button26
 *             bit2 ---- button27
 *             bit3 ---- button28
 *             bit4 ---- button29
 *             bit5 ---- button30
 *             bit6 ---- button31
 *             bit7 ---- button32
 */
 typedef struct
{
    C_U32   handle_;
    C_U8    uJoystickId;                          ///<游戏杆ID（固定值：1-第一个游戏杆，2-第二个游戏杆，3-第三个游戏杆，4-第四个游戏杆）
    C_U8    uX;                                   ///<X轴位置
    C_U8    uY;                                   ///<Y轴位置
    C_U8    uZ;                                   ///<Z轴位置
    C_U8    uRx;                                  ///<Rx轴坐标
    C_U8    uRy;                                  ///<Ry轴坐标
    C_U8    uRz;                                  ///<Rz轴位置
    C_U8    uHatSwitch;                           ///<瞄准镜头盔
    C_U8    uButton[CLOUD_JOYSTICK_VALUE_SIZE];   ///<按键指示
}C_JoyStick;

/**
 * @brief 定义一个触控点的数据格式
 */
typedef struct
{
    C_U32 touch_id_;            ///<触控点标识符
    C_U16 x_;                   ///<X轴坐标
    C_U16 y_;                   ///<Y轴坐标
    C_U32 flag_;                ///<一组位标志,参见宏: MTI_xxxx
    C_U16 area_width_;          ///<触控区域的宽度，[0-4096]
    C_U16 area_height_;         ///<触控区域的高度，[0-4096]
}C_Touch;

/**
 * @brief 定义标准多点触控数据格式
 */
typedef struct
{
    C_U32         handle_;
    C_U16         touch_count_;   ///<触点个数
    C_Touch       * touch_buffer_;///<每个触点信息缓冲区
}C_MultiTouch;


/// 鼎亿体感手柄消息
typedef struct
{
    C_U32   handle_;
    C_U16   uPacketCount;   ///<FingPC操作数据包个数
    C_U16   uPacketSize;    ///<FingPC操作数据包大小
    C_U8    *pdata;         ///<操作数据，数据长度=uPacketCount*uPacketSize
} C_FlingPC;

/// HID设备按键消息
typedef struct
{
    C_U8        *pdata;      ///<数据
    C_U16       uDataSize;   ///<数据大小
} C_HID;

/**
 * @brief 定义外设描述信息封装数据结构
 */
typedef union C_DeviceInfo
{
    C_HID    hid_;      ///<针对HID类设备
                        ///<对于外设插入，数据格式定义如下：
                        ///<            count(4),{dev_id(4),handle(4),rpt_desc_len(2),rpt_desc(rpt_desc_len)}*
                        ///<对于外设拔出，数据格式定义如下：
                        ///<            count(4),{dev_id(4),handle(4)}*
    C_U32    handle_;   ///<针对其它类型的设备
}C_DeviceInfo;

/// 解码器状态码
typedef enum
{
    DecoderStatusCode_OK = 0x01,    ///<解码正常
    DecoderStatusCode_DecodeError,  ///<解码错误
    DecoderStatusCode_DataError,    ///<数据错误
    DecoderStatusCode_DataOverflow, ///<数据上溢
    DecoderStatusCode_DataUnderflow ///<数据下溢
}C_DecoderStatusCode;

/// Ts信号质量参数
typedef struct
{
    C_U32       uLevel;     ///<信号电平，单位：dBμV。
    C_U32       uCN;        ///<载噪比，单位：dB。
    C_U32       uErrRate;   ///<误码率 = uErrRate * 10-9
}C_TsSignal;

/// 颜色模式
typedef enum
{
    ColorMode_RGB565    = 0x01,
    ColorMode_ARGB1555  = 0x02,
    ColorMode_ARGB4444  = 0x03,
    ColorMode_RGB888    = 0x04,
    ColorMode_ARGB8888  = 0x05
}C_ColorMode;

/// 云服务状态
typedef enum
{
    Status_Portal = 1,   ///<门户状态
    Status_Application,  ///<应用状态
    Status_Unknown       ///<状态未知
}C_Status;

/***************** 以下为移植库实现，提供给终端使用的函数接口  *************************/
/**
 * @brief  移植库初始化
 * @return   返回初始化结果
 * @retval   CLOUD_OK           初始化成功
             CLOUD_FAILURE      初始化失败
 * @note     如果返回失败，移植库不能正常工作。
 *           该函数为移植库初始化函数，登录云服务之前调用。
 */
C_RESULT Cloud_Init(void);

/**
 * @brief  移植库反初始化
 * @return   返回结果
 * @retval   CLOUD_OK           反初始化成功
             CLOUD_FAILURE      反初始化失败
 */
C_RESULT Cloud_Final(void);

/**
 * @brief  查询移植库版本
 * @note   获取移植库的版本号及厂商信息
 * @param    szVersion  版本信息字符串，返回的字符串格式参考："CloudLib-1.2.2.29"。
 * @param    szVendor   厂商信息字符串，返回的字符串格式参考："CyberCloud"。

 * @retval   CLOUD_OK               成功
 * @retval   CLOUD_FAILURE          失败
 */
C_RESULT Cloud_GetVersion(
    OUT char szVersion[CLOUD_VERSION_LEN + 1],
    OUT char szVendor[CLOUD_VENDOR_LEN + 1]
);

/**
 * @brief  启动云服务
 * @param    szCloudInfo 云服务参数，格式：CloudURL?parameter1=value1&parameter2=value2…
             例如:"CyberCloud://192.168.16.11:10531?UserID=1234567890123456&Password=123456&
               STBID=1234567890123456&MAC=00-FF-B4-A9-90-7E"
 * @return   返回启动结果
 * @retval   CLOUD_OK               成功
 * @retval   CLOUD_FAILURE          失败
 */
C_RESULT Cloud_Start( IN char const* szCloudInfo);

/**
 * @brief 停止视频云服务
 * @note  停止云服务
 * @return 返回停止云服务结果
 * @retval   CLOUD_OK               成功
 * @retval   CLOUD_FAILURE          失败
 */
C_RESULT Cloud_Stop( void );

/**
 * @brief 响应按键消息。将按键消息映射给视频云服务器进行处理。
 * @param  uType      按键类型；
 * @param  uLen	      按键消息的字节数；
 * @param  puKeyData  按键消息；根据按键类型，选择合适的数据结构：
                    C_RC，C_Keyboard， C_Mouse，C_Touch，C_JoyStick，C_FlingPC或者
                    C_HID进行按键消息构造，然后将构造好的消息转换成C_U8的数组传入本函数。
 * @note   C_RCKey定义的所有按键均需调用此函数传给移植库进行处理；
           待机键：调用停止视频云服务函数，然后进入待机状态；
 */
void Cloud_OnKey(
    IN C_KeyType    uType,
    IN C_U8         uLen,
    IN C_U8*        puKeyData
);

/**
 * @brief USBHID设备插入。通知移植库一个新的USBHID设备插入终端。
 * @param    key_type   外设类型
 * @param    dev_info   外设描述信息
 * @note   如果终端平台移植了USB数据采集库，那么此函数必须实现。
 */
void Cloud_DeviceConnect(
    IN C_KeyType    key_type,
    IN C_DeviceInfo *dev_info
);

/**
 * @brief USBHID 设备拔出。通知移植库一个USBHID设备拔出终端。
 * @param    key_type   外设类型
 * @param    dev_info   外设描述信息
 * @note   如果终端平台移植了USB数据采集库，那么此函数必须实现。
 *         uhandle 与Cloud_DeviceConnect()的设备句柄相对应
 */
void Cloud_DeviceDisconnect( 
    IN C_KeyType    key_type,
    IN C_DeviceInfo *dev_info
);

/**
 * @brief  云服务退出回调函数定义
 * @param  uExitFlag   退出标示: 1，表示按退出键退出；2，表示按菜单键退出；
 * @param  szBackURL   返回地址，机顶盒根据该参数返回相应的页面(当终端不支持浏览器时，
                        该参数为NULL)
 * @note
           当用户要退出云服务时，移植库会调用注册的回调函数通知机顶盒，机顶盒在该函数中
           调用Cloud_Stop()，并且根据返回地址，退到相应的页面。
           例如:
           void exit_callback_func( IN C_U8 uFlag, IN char const *szBackURL )
           {
                Cloud_Stop();
                if( uExitFlag == 1 )
                {
                    //机顶盒进行退出键退出的逻辑处理
                }
                if( uExitFlag == 2 )
                {
                    //机顶盒进行菜单键退出的逻辑处理
                }
           }
 */
typedef void (*CStb_CloudExitNotify)( IN C_U8 uFlag, IN char const *szBackURL );

/**
 * @brief  注册云服务退出回调函数
 * @param  notifyFunc  回调函数指针
 * @note
           该函数必须在Cloud_Init之后，Cloud_Start之前调用；
           当用户要退出云服务时，移植库会调用注册的回调函数通知机顶盒
 */
void Cloud_RegisterExitCallback( IN CStb_CloudExitNotify notifyFunc );

/**
 * @brief 工程调试接口,可以通过外部命令的方式进行移植库的调试。
 * @param szCmd 调试命令字符串。
 * @return 返回设置结果
 * @retval   CLOUD_OK               成功
 * @retval   CLOUD_FAILURE          失败
 * @return 返回设置结果
 * @retval   CLOUD_OK                  成功
 * @retval   CLOUD_FAILURE          失败
 */
C_RESULT Cloud_DebugSet ( IN char*  szCmd );

/**
 * @brief 返回门户
 * @note  终端调用此函数，云服务返回到门户。
 * @return 返回门户结果
 * @retval   CLOUD_OK               成功
 * @retval   CLOUD_FAILURE          失败
 */
C_RESULT Cloud_BackHome(void);

/**
 * @brief  读取云服务状态
 * @return 返回云服务状态
 */
C_Status Cloud_GetStatus(void);


/***************** 以上为移植库实现，提供给终端使用的函数接口  *************************/

/***************** 以下为终端实现，提供给移植库使用的函数接口  *************************/

 ///基本接口
/**
 * @brief 打印输出信息
 * @param   szFmt   输出信息格式描述文本，具体格式参见标准C：printf()函数
 * @param   …       输出信息中的参数列表
 */
void CStb_Print( IN char *szFmt, IN ... );

/**
 * @brief 外设指示消息
 * @param   uType       外设类型；
 * @param   uLen       消息的字节数；
 * @param   puKeyData   指示消息（网络字节序）；
                        根据外设类型，将数组中的数据转换成相应的数据结构：
                        C_Keyboard，C_FlingPC或者C_HID。
 */                                                 
void CStb_OnKeyOutput(
    IN C_U32        handle,
    IN C_KeyType    uType,
    IN C_U8         uLen,
    IN C_U8*        puKeyData
);

/**
 * @brief 消息通知
 * @param uMsgCode  消息代码
 * @param uMessage  显示信息内容
 * @param uErrCode  错误码，十进制
 * @note 
 				 当采用移植库外部绘制界面的机制时，需要实现此函数；
 				 移植库提供给终端设备应用的消息通知。终端应用在实现此函数时，
      根据消息码、显示信息内容以及错误号完成相应的显示或者操作。
         当uMsgCode为MsgCode_ShowInfo时，uMessage参数才会生效：此时，
      如果uErrCode为0，则表示该消息为提示信息，终端只需要给出提示即可；如果uErrCode为非0值，
      则表示该消息为错误提示，需要终端显示出带确认按钮的提示框，并且用户只能按确认键退出云服务；        
 				当uMsgCode为其他值时，uMessage和uErrCode两个参数忽略；
 */
void CStb_Notify(
    IN C_MessageCode    uMsgCode,
    IN C_U8            	uMessage[MSG_MAX_LEN],
    IN C_U16            uErrCode
);

/**
 * @brief  获取系统运行时间，单位为ms。
 * @return 返回终端系统运行时间，单位ms
 */
C_U32 CStb_GetUpTime();

/**
 * @brief 分配内存.请求分配指定大小的内存块，分配到的内存空间必须是连续的整块内存。
 * @param uLength 需要分配的内存大小，以字节为单位
 * @return  返回结果
 * @retval  成功    返回分配内存地址
            失败    返回0

 */
void * CStb_Malloc( IN C_U32 uLength );

/**
 * @brief  释放内存.释放pBuf指定的内存空间，并且该内存空间必须是由CStb_Malloc分配的。
 * @param pBuf 需要释放的内存空间的地址
 */
void CStb_Free(IN void * pBuf);


#define Priority_Lowest    0 ///<最低优先级.
#define Priority_Low       1 ///<低优先级.
#define Priority_Normal    2 ///<正常优先级.
#define Priority_High      3 ///<高优先级.
#define Priority_Highest   4 ///<最高优先级.
/**
 * @brief 创建线程.向终端设备申请创建并启动一个线程的运行。
 * @param  pThreadHandle     指向线程句柄的指针
 * @param  szName            线程名称
 * @param  uPriority         线程的优先级，
                const U8  Priority_Lowest  =  0;    ///<最低优先级.
                const U8  Priority_Low     =  1;    ///<低优先级.
                const U8  Priority_Normal  =  2;    ///<正常优先级.
                const U8  Priority_High    =  3;    ///<高优先级.
                const U8  Priority_Highest =  4;    ///<最高优先级.
 * @param  pThreadFunc	    线程函数地址
 * @param  pParam           线程函数参数
 * @param  uStackSize       线程栈的大小
 * @return  返回结果
 * @retval  CLOUD_OK	      成功
            CLOUD_FAILURE	  失败
 */
C_RESULT CStb_CreateThread (
    OUT C_ThreadHandle  *pThreadHandle,
    IN char const*      szName,
    IN C_U8             uPriority,
    IN void             (*pThreadFunc)(void*),
    IN void *           pParam,
    IN C_U16            uStackSize
);

/**
 * @brief 删除线程
 * @param hThreadHandle 线程句柄
 */
void CStb_DeleteThread ( IN  C_ThreadHandle   hThreadHandle );

/**
 * @brief 休眠.申请当前线程休眠一段时间。
 * @param uMicroSeconds   线程休眠时间，单位为微秒
 */
void CStb_Sleep( IN C_U32 uMicroSeconds );

/**
 * @brief 创建信号量
        申请创建信号量，并给该信号量赋初始计数值。
 * @param  pHandle  指向输出信号量句柄的指针
 * @param  uCount   信号量的初始计数值
 * @return 返回结果
 * @retval CLOUD_OK         成功
           CLOUD_FAILURE    失败
 * @note 移植库使用计数信号量控制对共享资源的访问：
        当信号量计数等于0时，线程不能使用资源，进入等待状态；
        当信号量计数大于0时，线程可以使用资源，并同时将信号量减1；
 */
C_RESULT CStb_CreateSemaphore (
    OUT C_SemaphoreHandle *pHandle,
    IN  C_U32              uCount
);

/**
 * @brief 给信号量加信号
 * @param  hHandle  信号量句柄
 */
void CStb_SemaphoreSignal( IN C_SemaphoreHandle   hHandle );

/**
 * @brief 等待信号量,当信号量计数大于0时等待成功，信号量计数减1。
 * @param   hHandle	 信号量句柄
 * @param   uTimeout 等待信号量的超时时间，单位为毫秒
                    如果uTimeout = 0xFFFFFFFF，则表示永不超时，直到获得信号量为止。
 * @return 返回结果
 * @retval CLOUD_OK         成功
           CLOUD_TIMEOUT    超时
           CLOUD_FAILURE    失败
 */
C_RESULT CStb_SemaphoreWait(
    IN C_SemaphoreHandle   hHandle,
    IN C_U32               uTimeout
);

/**
 * @brief 删除信号量
 */
void CStb_DeleteSemaphore( IN C_SemaphoreHandle   hHandle );

///网络接口
/**
 * @brief  创建套接字
 * @param  pSocketHandle  套接字句柄
 * @param  uType          套接字类型
 * @param  bIsBlocking    是否创建阻塞套接字 
 * @return 返回结果
 * @retval CLOUD_OK        成功
           CLOUD_FAILURE   失败
 */
C_RESULT CStb_SocketOpen(
    OUT C_SocketHandle     *pSocketHandle,
    IN  C_SocketType       uType,
    IN  C_BOOL             bIsBlocking
);

/**
 * @brief  对目标套接字建立连接
 * @param  pSocketHandle  套接字句柄
 * @param  pDstSocketAddr 套接字目的地址
 * @return 返回结果
 * @retval CLOUD_OK        成功
           CLOUD_FAILURE   失败
 * @note 
        对于非阻塞套接字，本函数立即返回CLOUD_OK，连接状态可以通过CStb_SocketSelect查询。
        对于阻塞套接字该函数要等到连接成功或失败后才返回。
 */
C_RESULT CStb_SocketConnect( 
    IN  C_SocketHandle      hSocketHandle, 
    IN  C_SocketAddr const *pDstSocketAddr
);

/**
 * @brief  设置套接字的选项
 * @param  pSocketHandle  套接字句柄
 * @param  uLevel         选项定义的层次；
 * @param  uOptname       设置的选项
 * @param  pOptval        指向存放选项值的缓冲区指针
 * @param  uOptlen        缓冲区长度
 * @return 返回结果
 * @retval CLOUD_OK        成功
           CLOUD_FAILURE   失败
 */
C_RESULT CStb_SocketSetOpt(
    IN  C_SocketHandle    hSocketHandle,
    IN  C_SocketOptLevel  uLevel, 
    IN  C_SocketOptName   uOptname,
    IN  C_U8  const *     pOptval, 
    IN  C_U32             uOptlen
);

/**
 * @brief  由域名返回IP地址
 * @param  pDomainName  域名字符串
 * @return 网络IP地址
 */
C_NetworkIP CStb_SocketGetHostByName( IN  char const *pDomainName );

/**
 * @brief  检测一个或多个套接字状态
 * @param  pReadSockets         输入:要做读检测套接字序列;返回:读状态变化的套接字序列
 * @param  uReadSocketCount     要进行读检测的套接字个数
 * @param  pWriteSockets        输入:要做写检测套接字序列;返回:写状态变化的套接字序列
 * @param  uWriteSocketCount    要进行写检测的套接字个数 
 * @param  pExceptSockets       输入:要做错误检测套接字序列;返回:出错的套接字序列
 * @param  uExceptSocketCount   要进行错误检测的套接字个数  
 * @param  uTimeout             超时时间，单位毫秒
 * @return 返回结果
 * @retval CLOUD_OK        成功
           CLOUD_TIMEOUT   超时
 * @note  参数pReadSockets,pWriteSockets和pExceptSockets会返回状态变化的套接字序列,
      状态没变化的套接字将被设置为NULL。
 */
C_RESULT CStb_SocketSelect(      
    INOUT C_SocketHandle *pReadSockets,
    IN    C_U32          uReadSocketCount,
    INOUT C_SocketHandle *pWriteSockets,
    IN    C_U32          uWriteSocketCount,
    INOUT C_SocketHandle *pExceptSockets, 
    IN    C_U32          uExceptSocketCount,    
    IN    C_U32          uTimeout 
);

/**
 * @brief 向连接套接字发送数据
 * @param  hSocketHandle  套接字句柄
 * @param  pBuf	          数据Buffer
 * @param  uBytesToSend	  要发送的字节数
 * @param  puBytesSent	  实际发送的字节数
 * @return 返回结果
 * @retval CLOUD_OK       成功
           CLOUD_FAILURE  失败
 */
C_RESULT  CStb_SocketSend(
    IN  C_SocketHandle   hSocketHandle,
    IN  C_U8 const       *pBuf,
    IN  C_U32            uBytesToSend,
    OUT C_U32            *puBytesSent
);

/**
 * @brief 从连接套接字接收数据
 * @param  hSocketHandle    套接字句柄
 * @param  pBuf             数据Buffer
 * @param  uBytesToReceive  要接收的字节数
 * @param  puBytesReceived  实际接收到的字节数
 * @return 返回结果
 * @retval CLOUD_OK        成功
           CLOUD_TIMEOUT   超时
           CLOUD_FAILURE   失败
 */
C_RESULT  CStb_SocketRecv(
    IN  C_SocketHandle  hSocketHandle,
    OUT C_U8            *pBuf,
    IN  C_U32           uBytesToReceive,
    OUT C_U32           *puBytesReceived
);

/**
 * @brief 无连接套接字发送数据
 * @param  hSocketHandle  套接字句柄
 * @param  pSocketAddr	  目的地地址
 * @param  pBuf           数据Buffer
 * @param  uBytesToSend	  要发送的字节数
 * @param  puBytesSent	  实际发送的字节数
 * @return 返回结果
 * @retval CLOUD_OK           成功
           CLOUD_FAILURE      失败
 * @note 本函数要求套接字是无连接的。
 */
C_RESULT  CStb_SocketSendTo(
    IN  C_SocketHandle      hSocketHandle,
    IN  C_SocketAddr const  *pSocketAddr,
    IN  C_U8 const          *pBuf,
    IN  C_U32               uBytesToSend,
    OUT C_U32               *puBytesSent
);

/**
 * @brief 无连接套接字接收数据,接收数据报，并保存源地址。
 * @param  hSocketHandle    套接字句柄
 * @param  pSocketAddr	    源地址
 * @param  pBuf             数据Buffer
 * @param  uBytesToReceive  要接收的字节数
 * @param  puBytesReceived  实际接收到的字节数
 * @param  uTimeout         接收数据的超时时间,单位毫秒
 * @return 返回结果
 * @retval CLOUD_OK        成功
           CLOUD_TIMEOUT   超时
           CLOUD_FAILURE   失败
 * @note 本函数要求套接字是无连接的。
 */
C_RESULT  CStb_SocketReceiveFrom(
    IN  C_SocketHandle  hSocketHandle,
    OUT C_SocketAddr    *pSocketAddr,
    OUT C_U8                *pBuf,
    IN  C_U32                   uBytesToReceive,
    OUT C_U32               *pBytesReceived
);

/**
 * @brief 销毁套接字
 * @param  hSocketHandle  套接字句柄
 * @return 返回结果
 * @retval CLOUD_OK         成功
           CLOUD_FAILURE    失败
 */
C_RESULT  CStb_SocketClose( IN  C_SocketHandle  hSocketHandle );

///数据存储接口
/**
 * @brief 读取存储空间,读取Flash存储空间中的数据。
 * @param  uBlockID         存储空间ID
                            CLOUD_FLASH_BLOCK_A:第一块64KBytes存储空间Block；
                            CLOUD_FLASH_BLOCK_B:第二块256KBytes存储空间Block；
                            其它:无效；
 * @param  pBuf             数据Buffer
 * @param  uBytesToRead     要读取的字节数
 * @param  puBytesRead      实际读取的字节数 
 * @return   返回读取结果
 * @retval   CLOUD_OK          成功
             CLOUD_FAILURE     失败
 * @note 每次读取的起始位置相同，均为该指定块的起始位置
 */
C_RESULT  CStb_FlashRead( 
    IN  C_U8            uBlockID,
    OUT C_U8            *pBuf,
    IN  C_U32           uBytesToRead,
    OUT C_U32           *puBytesRead
);

/**
 * @brief 写入存储空间,向Flash存储空间中写入数据。
 * @param  uBlockID         存储空间ID
                            CLOUD_FLASH_BLOCK_A:第一块64KBytes存储空间Block；
                            CLOUD_FLASH_BLOCK_B:第二块256KBytes存储空间Block；
                            其它:无效；
 * @param  pBuf             数据Buffer
 * @param  uBytesToWrite    要写入的字节数
 * @return   返回读取结果
 * @retval   CLOUD_OK          成功
             CLOUD_FAILURE     失败
 * @note 每次写入的起始位置相同，均为该指定块的起始位置
 */
C_RESULT  CStb_FlashWrite ( 
    IN  C_U8            uBlockID,
    IN  C_U8 const      *pBuf,
    IN  C_U32           uBytesToWrite
);

///DEMUX数据收取
/**
 * @brief 从过滤器中收取指定PID的一个section数据。
 * @param    pFreq      频点参数指针
 * @param    uPID       数据的PID
 * @param    uTableId   Table id
 * @param    pBuf       存储section数据的缓冲区指针
 * @param    uBufMaxLen 存储section数据的缓冲区的最大尺寸
 * @param    pBytesReceived 接收到的section数据的实际长度
 * @param    uTimeout       接收数据的超时时间,单位毫秒
 * @return   返回结果
 * @retval   CLOUD_OK          成功
             CLOUD_TIMEOUT	   超时
             CLOUD_FAILURE     失败
 * @note 此函数只有Cable终端需要实现。
 */
C_RESULT  CStb_DemuxReceiveData (
    IN    C_FreqParam  const    *pFreq,
    IN    C_U16                 uPID,
    IN    C_U8                  uTableId,
    OUT   C_U8                  *pBuf,
    IN    C_U32                 uBufMaxLen,
    OUT   C_U32                 *pBytesReceived,
    IN    C_U32                 uTimeout
);

///播放接口
/**
 * @brief 通过PID播放音视频
 * @param pFreq         频点参数指针
 * @param pPidParams    PID传输参数指针
 * @return   返回结果
 * @retval   CLOUD_OK          成功
             CLOUD_FAILURE     失败
 * @note 此函数要求快速返回，应用可将消息记录下来后另行处理，
         不可在此函数中对消息做阻塞处理；此函数只有Cable终端需要实现。
 */
C_RESULT CStb_AVPlayByPid( IN C_FreqParam  const *pFreq, IN C_PIDParam  const *pPidParams );

/**
 * @brief 通过节目号播放音视频
 * @param pFreq     频点参数指针
 * @param uProgNo   节目号
 * @return   返回结果
 * @retval   CLOUD_OK          成功
             CLOUD_FAILURE     失败
 * @note 此函数要求快速返回，应用可将消息记录下来后另行处理，
         不可在此函数中对消息做阻塞处理；此函数只有Cable终端需要实现。
 */
C_RESULT CStb_AVPlayByProgNo( IN C_FreqParam  const *pFreq, IN C_U32 uProgNo );

///AV解扰接口(Cable终端)
/**
 * @brief 设置音视频的CW
 * @param uType     该组解扰字对应的类型
                    0:视频；
                    1:音频；
                    其它:无效
 * @param pEven     偶周期解扰字
 * @param pOdd      奇周期解扰字
 * @param uKeyLen   解扰字的长度
 * @return   返回结果
 * @retval   CLOUD_OK          成功
             CLOUD_FAILURE     失败
 * @note 该函数在播放函数之后调用(CStb_AVPlayByPid或者CStb_AVPlayByProgNo)，根据uType找到对应的
         音频PID或者视频PID，并设置相应的解扰字。
 */
C_RESULT CStb_AVSetCW(
    IN C_U8         uType, 
    IN C_U8 const   *pEven, 
    IN C_U8 const   *pOdd,
    IN C_U8         uKeyLen
);


///播放需要考虑多进程的情况，因此，播放模块需要单独封装成一个库，需要通过集成方，将协议库的参数
///传递给播放进行中，并通过播放库进行解密的操作；

/**
 * @brief 注入TS流数据回调函数
 * @param pInjectData   数据指针
 * @param uDataLen      数据长度
 * @return   返回结果
 * @retval   CLOUD_OK          成功
             CLOUD_FAILURE     失败
 * @note 此函数注入TS流数据，只有需要通过TS流播放音视频的终端需要实现。
         此函数要求快速返回，内部可用队列缓存数据，不可在此函数中对消息做阻塞处理；
 */
typedef C_RESULT (*CStb_AVInjectTSData)( IN C_U8 *pInjectData, IN C_U32 uDataLen );

/**
 * @brief 从TS流播放音视频
 * @param pPidParams        PID传输参数指针
 *        InjectTSDataFun   注入TS数据回调函数
 * @return   返回结果
 * @retval   CLOUD_OK          成功
             CLOUD_FAILURE     失败
 * @note 此函数完成播放初始化，并开始等待播放TS流数据，只有需要通过TS流播放音视频的终端需要实现。
         此函数要求快速返回，不可在此函数中对消息做阻塞处理；
 */
C_RESULT CStb_AVPlayTSOverIP(IN C_PIDParam const *pPidParams, OUT CStb_AVInjectTSData *InjectTSDataFun);

/**
 * @brief 设置音量，范围0-100之间，0表示最小音量，100表示最大音量
 * @param   uVol    音量值，如果值不在0-100之间，则返回失败
 * @return   返回结果
 * @retval   CLOUD_OK          成功
             CLOUD_FAILURE     失败
 * @note
        云服务的音量范围是0-100，集成厂家需要根据自己平台的特点进行相应的转换。
 */
C_RESULT CStb_SetVolume( IN C_U8 uVol );

/**
 * @brief 获取当前音量,范围0-100之间，0表示最小音量，100表示最大音量
 * @return   返回当前音量
 * @note
        该函数在启动云服务时会被调用，用于保持云服务的初始音量和进入前一致。
        请将进入云服务前的音量转换为0-100范围内的值返给云服务；
 */
C_U8 CStb_GetVolume(void);

/**
 * @brief 设置静音状态
 * @param   bMuteStatus     C_TRUE  静音
                            C_FALSE 非静音
 * @return   返回结果
 * @retval   CLOUD_OK          成功
             CLOUD_FAILURE     失败
 * @note
        云服务通过该函数设置静音状态，集成厂家可以记录该状态，在退出云服务时，
        将静音状态传给机顶盒其它应用。
 */
C_RESULT CStb_SetMuteState( IN C_BOOL bMuteStatus );

/**
 * @brief 获取当前静音状态
 * @return   返回当前静音状态
 * @retval   C_TRUE     静音
             C_FALSE    非静音
 * @note
        该函数在启动云服务时会被调用，用于获得机顶盒进入云服务前的静音状态。
 */
C_BOOL CStb_GetMuteState(void);

/**
 * @brief 设置视频窗口位置
 * @param    uX         起始点横坐标
 * @param    uY         起始点纵坐标
 * @param    uWidth     视频窗口的宽度
 * @param    uHeight    视频窗口的高度

 * @return   返回结果
 * @retval   CLOUD_OK          成功
             CLOUD_FAILURE     失败
 * @note
        本函数认为视频区域和显示区域坐标重合，坐标为相对于图形显示区域的坐标；
        如果两个坐标不重合，需要集成厂家进行相应的转换。
 */
C_RESULT CStb_SetVideoWindow
(
    IN C_U32    uX,
    IN C_U32    uY,
    IN C_U32    uWidth,
    IN C_U32    uHeight
);

/**
 * @brief 停止播放
 */
void CStb_AVStop(void);

/**
 * @brief 获得信号质量
 * @param   pSignal  信号质量信息
 * @return  返回结果
 * @retval  CLOUD_OK          成功
            CLOUD_FAILURE     失败
 * @note 当Cable终端播放音视频时，移植库会定期调用该函数获取当前TS的信号质量。
 */
C_RESULT CStb_GetTsSignalInfo( OUT C_TsSignal *pSignal );

/**
 * @brief 获取丢包率
 * @param   pRate    当前丢包率 = pRate/10000
 * @return   返回结果
 * @retval   CLOUD_OK          成功
             CLOUD_FAILURE     失败
 * @note 当IP终端播放音视频时，移植库会定期调用该函数获取当前丢包率。
 */
C_RESULT CStb_GetPacketLossRate ( OUT C_U32 *pRate );

/**
 * @brief 获得解码器状态
 * @param uDecoderType  解码器类型：
                        const   U8  Video_Decoder  = 1; ///<视频解码器
                        const   U8  Audio_ Decoder = 2; ///<音频解码器
 * @return 返回解码器状态
 * @note 该函数的调用频率为1秒一次。

 */
C_DecoderStatusCode CStb_GetDecoderStatus (  IN C_U8 uDecoderType  );

///图像显示接口
/**
 * @brief  获取终端显示区域的信息。
 * @param pBuff 视频云服务显示缓冲区的地址，此缓冲区的大小是根据宽、高以及颜色模式共同确定的，
 		用于视频云服务的界面显示，视频云服务在此缓冲区中更新完数据后，会调用"CStb_UpdateRegion"函数通知更新显存。
    如果该缓冲区地址为显存地址，不需要实现"CStb_UpdateRegion"函数，但是用户可能会看到界面绘制过程。
    在云服务使用过程中，此缓冲区不能被释放。
    当退出云服务时，此缓冲区可以被释放。
 * @param pWidth 显示区域的宽度
 * @param pHeight 显示区域的高度
 * @param pColorMode 颜色模式
 * @note 建议不要直接返回显存地址。

 */
void CStb_GraphicsGetInfo(
    OUT C_U8             **pBuff,
    OUT C_U32           *pWidth,
    OUT C_U32        *pHeight,
    OUT C_ColorMode  *pColorMode
);

/**
 * @brief  更新屏幕指定区域
 * @param uX    更新屏幕的矩形区域的x 轴位置
 * @param uY    更新屏幕的矩形区域的y 轴位置
 * @param uWidth    更新屏幕的矩形区域的宽度
 * @param uHeight   更新屏幕的矩形区域的高度
 * @return   返回结果
 * @retval   CLOUD_OK          成功
             CLOUD_FAILURE     失败

 */
C_RESULT CStb_UpdateRegion(
    IN C_U32    uX,
    IN C_U32    uY,
    IN C_U32    uWidth,
    IN C_U32    uHeight
);

/**
 * @brief 清除屏幕,即：将显示区域置为透明。
 */
void CStb_CleanScreen( void );

/***************** 以上为终端实现，提供给移植库使用的函数接口  *************************/
#ifdef __cplusplus
}
#endif

#endif //CYBER_CLOUD_API_H


