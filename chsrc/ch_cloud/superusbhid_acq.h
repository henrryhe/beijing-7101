/**
 * @brief    SuperUSBHID v1.0.1 版本输入数据采集模块头文件
 * @author   胡勇新
 * @version  1.0.0
 * @date     2010.10.11
 */

#ifndef VIDEO_CLOUD_SUPERUSBHID_ACQ
#define VIDEO_CLOUD_SUPERUSBHID_ACQ

#ifdef WIN32

#ifdef SUPERUSBHID_SDK_EXPORTS
#define SUPERUSBHID_API __declspec(dllexport)
#else
#define SUPERUSBHID_API __declspec(dllimport)
#if defined USING_DXINPUT
#ifdef _DEBUG
#pragma comment(lib, "superUSBHIDD.lib")
#else
#pragma comment(lib, "superUSBHID.lib")
#endif
#else
#error "Please define macro USING_DXINPUT to determinate using which dll"
#endif
#endif

#ifdef __cplusplus
#define DEFINE_SUPERUSBHID_API extern "C" SUPERUSBHID_API
#else
#define DEFINE_SUPERUSBHID_API SUPERUSBHID_API
#endif

#else
#define DEFINE_SUPERUSBHID_API
#endif

#include "cloud_api.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief  当新设备插入时的回调函数
 * @param    device_info_buffer 外设信息缓冲区（格式：count(4),{dev_id(4),handle(4),rpt_desc_len(2),rpt_desc(rpt_desc_len)}）
 * @param    device_info_len    外设信息缓冲区长度
 */
typedef void (*ConnectCallbackFunc)( 
                                IN C_U8  * device_info_buffer,
                                IN C_U16   device_info_len );

/**
 * @brief  当设备拔出时的回调函数
 * @param    device_info_buffer 外设信息缓冲区（格式：count(4),{dev_id(4),handle(4)}）
 * @param    device_info_len    外设信息缓冲区长度
 */
typedef void (*DisconnectCallbackFunc)( 
                                IN C_U8  * device_info_buffer,
                                IN C_U16   device_info_len );

/**
 * @brief  当收到设备发出的输入报告时的回调函数
 * @param    input_buffer 输入报告缓冲区（格式：count(4),{handle(4),rpt_len(2),rpt_data(rpt_len)}）
 * @param    input_len    输入报告缓冲区长度
 */
typedef void (*InputReportCallbackFunc)( 
                                IN C_U8  * input_buffer,
                                IN C_U16   report_len );

/**
 * @brief  当收到FlingPC设备发出的输入数据包时的回调函数
 * @param    packet_size    FlingPC数据包大小
 * @param    packet_count   输入数据包个数
 * @param    packet_buffer  输入数据缓冲区
 */
typedef void (*FlingpcInputCallbackFunc)( 
                                IN C_U16   packet_size,
                                IN C_U16   packet_count,
                                IN C_U8  * packet_buffer );

/**
 * @brief  获取当前USB采集库版本号
 * @return 返回当前USB采集库版本号
 */
char* USBHIDACQ_Version();

/**
 * @brief  设置当新设备插入时的回调函数
 * @param    callback 回调函数，当系统发现新设备插入时调用
 */
DEFINE_SUPERUSBHID_API
void USBHIDACQ_SetConnectCallback( IN ConnectCallbackFunc callback );

/**
 * @brief  设置当设备拔出时的回调函数
 * @param    callback 回调函数，当系统发现设备拔出时调用
 */
DEFINE_SUPERUSBHID_API
void USBHIDACQ_SetDisconnectCallback( IN DisconnectCallbackFunc callback );

/**
 * @brief  设置当收到设备发出的输入报告时的回调函数
 * @param    callback 回调函数，当系统发现收到设备发出的输入报告时调用
 */
DEFINE_SUPERUSBHID_API
void USBHIDACQ_SetInputReportCallback( IN InputReportCallbackFunc callback );

/**
 * @brief  设置当收到FlingPC设备发出的输入数据包时的回调函数
 * @param    callback 回调函数，当系统发现收到FlingPC设备发出的输入数据包时调用
 */
DEFINE_SUPERUSBHID_API
void USBHIDACQ_SetFlingpcInputCallback( IN FlingpcInputCallbackFunc callback );

/**
 * @brief  映射库内部调用，通知OS底层一个USBHID的输出报告来了（由集成商实现）
 * @param    handle        设备句柄
 * @param    output_buffer 输出报告缓冲区
 * @param    output_len    输出报告长度
 */
DEFINE_SUPERUSBHID_API
void USBHIDACQ_OutputReport( IN C_U32  handle,
                             IN C_U8     *output_buffer,
                             IN C_U16     output_len );

/**
 * @brief  映射库内部调用，通知OS底层一个或一组FlingPC的输出操作来了（由集成商实现）
 * @param    packet_size    FlingPC数据包大小
 * @param    packet_count   输出数据包个数
 * @param    packet_buffer  输出数据缓冲区
 */
DEFINE_SUPERUSBHID_API
void USBHIDACQ_FlingpcOutput( IN C_U16  packet_size,
                              IN C_U16  packet_count,
                              IN C_U8  *packet_buffer );

/**
 * @brief  开始输入数据采集
 * @return 返回操作是否成功
 */
DEFINE_SUPERUSBHID_API
C_BOOL USBHIDACQ_Start( void );

/**
 * @brief  停止输入数据采集
 */
DEFINE_SUPERUSBHID_API
void USBHIDACQ_Stop( void );

/**
 * @brief  设置依附窗口
 */
DEFINE_SUPERUSBHID_API
void USBHIDACQ_SetAttachHWND(void* hwnd );

/**
 * @brief  设置停止采集按键回调函数
 */
DEFINE_SUPERUSBHID_API
void USBHIDACQ_SetStopMappingCallBackFunc(void (*callback)());

#ifdef __cplusplus
}
#endif

#endif // end of VIDEO_CLOUD_SUPERUSBHID_ACQ
