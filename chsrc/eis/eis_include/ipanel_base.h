/******************************************************************************/
/*    Copyright (c) 2005 iPanel Technologies, Ltd.                     */
/*    All rights reserved. You are not allowed to copy or distribute          */
/*    the code without permission.                                            */
/*    This is the demo implenment of the Porting APIs needed by iPanel        */
/*    MiddleWare.                                                             */
/*    Maybe you should modify it accorrding to Platform.                      */
/*                                                                            */
/*    $author Zouxianyun 2005/04/28                                           */
/******************************************************************************/

#ifndef _IPANEL_MIDDLEWARE_PORTING_BASE_API_FUNCTOTYPE_H_
#define _IPANEL_MIDDLEWARE_PORTING_BASE_API_FUNCTOTYPE_H_

#include "ipanel_typedef.h"

#ifdef __cplusplus
extern "C" {
#endif

/* alloc memory function */
VOID *ipanel_porting_malloc(INT32_T memsize);

/* free memory function */
VOID ipanel_porting_free(VOID *ptr);

/* log output function */
INT32_T ipanel_porting_dprintf(CONST CHAR_T *fmt, ...);

/* iPanel middleware runtime base time, and unit is millisecond (1/1000 second) */
UINT32_T ipanel_porting_time_ms(VOID);

typedef enum
{
    IPANEL_SYS_INFO_NULL 				= 0,	/* 无效取值 */
    IPANEL_LOADER_NAME 	 				= 1,  	/* 当前系统加载程序的名字 */
    IPANEL_LOADER_VERSION				= 2, 	/* 当前系统加载程序的版本 */
    IPANEL_LOADER_PROVIDER				= 3,  	/* 当前系统加载程序的提供者 */
    IPANEL_LOADER_SIZE					= 4,  	/* 当前系统加载程序的大小 */
    IPANEL_DRIVER_NAME					= 5,  	/* 当前驱动程序的名字 */
    IPANEL_DRIVER_VERSION				= 6,  	/* 当前驱动程序的版本 */
    IPANEL_DRIVER_PROVIDER				= 7, 	/* 当前驱动程序的提供者 */
    IPANEL_DRIVER_SIZE					= 8,   	/* 当前驱动程序的大小 */
    IPANEL_HARDWARE_SERIAL				= 9, 	/* 硬件序列号 */
    IPANEL_HARDWARE_PROVIDER			= 10, 	/* 硬件提供者 */
    IPANEL_PRODUCT_DESC					= 11,  	/* 产品描述 */
    IPANEL_PRODUCT_SERIAL				= 12,  	/* 产品序列号 */
    IPANEL_PRODUCT_MAC_ADDR				= 13, 	/* 网卡MAC地址 */
    IPANEL_PRODUCT_FLASH				= 14,  	/* 产品闪存大小 */
    IPANEL_PRODUCT_RAM					= 15, 	/* 产品内存大小 */
    IPANEL_SMART_CARD_ID				= 16, 	/* 智能卡ID */
    IPANEL_PRODUCT_CA_NAME				= 17,  	/* 当前系统的CA的名字 */
    IPANEL_PRODUCT_CA_VERSION			= 18,  	/* 当前系统的CA的版本 */
    IPANEL_PRODUCT_CA_PROVIDER			= 19,  	/* 当前系统的CA的提供商 */
    IPANEL_PRODUCT_CA_SIZE				= 20,  	/* 当前系统的CA的大小 */
    IPANEL_PORTING_VERSION				= 21, 	/* 海南项目需要porting版本号 */
    IPANEL_SOFTWARE_VERSION				= 22,  	/* 海南项目需要整个软件的版本号 */
    IPANEL_JFT_CARD_INFO				= 23,  	/* 嘉富通刷卡信息 */
    IPANEL_JFT_CARD_STATUS				= 24, 	/* 嘉富通刷卡状态 */
    IPANEL_HARDWARE_PRODUCTIONBATCH		= 25, 	/* 硬件生产批次*/  //为杭州项目添加的特别宏，由天柏实现
    IPANEL_SIHUA_REGION_ID 				= 26, 	/* 没有搜索到region id的情况下使用的默认值*/
    IPANEL_OC_SERVICE_ID				= 27,  	/* 开始OC打开时对应的service_id,DVB2.0启动流程之所必须*/
    IPANEL_GD_TABLE_ID					= 28,	/* 广电私有表table_id */
    IPANEL_GD_PID						= 29,	/* 广电私有表pid */
    IPANEL_BOOT_TYPE					= 30,	/* 系统启动类型:"warm":热启动,"cold":冷启动 ,string 类型,len<=8*/
    IPANEL_BOOT_STRING					= 31, 	/* 系统启动时,对应的URL,string 类型,len<=256*/
	IPANEL_PRODUCT_STARTUP_MODE			= 32,  	/* 获取iPanel工作模式 --新流程实现后删除, porting guide中未定义该枚举值*/
	IPANEL_PRODUCT_INI_FILENAME			= 33,  	/* 获取文件下载配置文件的文件名 --新流程实现后删除 porting guide中未定义该枚举值*/
    IPANEL_GET_TIME_ZONE				= 34,	/* 获取设备所在地的时区,以秒为单位的时间*/
    IPANEL_GET_UTC_TIME					= 35,	/* 获取当前时刻以秒计的格林威治时间,起始时间为1970年1月1日0点0分0秒*/
	IPANEL_GET_START_FREQUENCY			= 36,	/* 获取开机频点， DVB2.0启动流程所需要的*/
	IPANEL_HARDWARE_VERSION 			= 37,	/* 获取硬件版本号*/
	IPANEL_LOADER_SYSIDENTIFICATION		= 38,	/* 获取loader system identification*/
	IPANEL_LOADER_KEYVERSION   			= 39,	/* 获取loader key version*/
	IPANEL_LOADER_SIGNATUREVERSION  	= 40,	/* 获取loader signature version*/
	IPANEL_LOADER_CSSN     				= 41,	/* 获取loader cssn*/
	IPANEL_LOADER_DOWNLOADID   			= 42,	/* 获取loader download Id*/
	IPANEL_MANUFACTURER_ID    			= 43,	/* 获取manufacturer Id*/
	IPANEL_IRD_VARIANT     				= 44,	/* 获取IRD variant*/
	IPANEL_STB_PROVIDER_ID				= 45,	/* 获取STB提供商ID*/
	IPANEL_STB_MODEL_ID					= 46,	/* 获取STB型号ID*/
	IPANEL_PRODUCT_CA_SYSTEM_ID			= 47,	/* 获取本机CA提供商ID*/
	IPANEL_STB_GROUP_ID					= 48,	/* 获取本机的group Id*/
	IPANEL_BOOT_VERSION					= 49,	/* BOOT版本号*/
	IPANEL_ROOT_FFS_VERSION				= 50,	/* ROOT_FFS版本号*/
	IPANEL_OS_VERSION					= 51,	/* 操作系统版本号*/
	IPANEL_SOFTWARE_RAM_SIZE			= 52,	/* 系统运行时,软件以及全局变量等占用静态内存大小*/
	IPANEL_DECODER_RAM_SIZE				= 53,	/* 系统运行时,解码器占用动态内存大小*/
	IPANEL_STANDBY_FLAG					= 54,	/* 系统启动时，获取是否需要进入待机的待机标志位，1表示启动后进待机状态，0表示启动后不进待机状态。*/
	IPANEL_SEARCH_FLAG					= 55,	/* 系统启动时，获取是否需要搜台的标志位，1表示搜索，0表示不搜索。*/
    IPANEL_CM_STATUS                    = 56,   /*获取内置CM连接状态。0:表示不存在内置CM，1:表示有内置CM，但物理上未连接，2:表示有内置CM，且物理上已连接。*/
	IPANEL_UPGRADE_FLAG					= 106,	/*系统启动时，获取前一次关机前升级是否成功的标志位:1表示升级成功；0表示未升级*/
	IPANEL_SYS_INFO_UNKNOWN
} IPANEL_STB_INFO_e;
INT32_T ipanel_porting_system_get_info(IPANEL_STB_INFO_e name, CHAR_T *buf, INT32_T len);
INT32_T ipanel_porting_system_set_info(IPANEL_STB_INFO_e name, CHAR_T *buf, INT32_T len);

typedef enum 
{
    IPANEL_RESOUCE_FONT = 1,
    IPANEL_RESOUCE_UI	= 2
} IPANEL_RESOURCE_TYPE_e;
INT32_T ipanel_porting_get_outside_dat_info(CHAR_T **address, INT32_T *size, 
            IPANEL_RESOURCE_TYPE_e type);

INT32_T ipanel_porting_system_reboot(INT32_T s);
INT32_T ipanel_porting_system_standby(INT32_T s);

#ifdef __cplusplus
}
#endif

#endif//_IPANEL_MIDDLEWARE_PORTING_BASE_API_FUNCTOTYPE_H_
