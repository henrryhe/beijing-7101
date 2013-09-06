/*******************************************************************************
* 文件名称 : CHMID_TCPIP_api.c											
* 功能描述 : 实现HMID_TCPIP模块提供给上层应用调用的功能接口函数。
*
* 作者:flex
* 修改历史:
* 日期					作者						描述
* 2009/3/30				flex							create
*
********************************************************************************/

/*---------------------------------type--------------------------------*/
#ifndef __CHDRV_NET_H__
#define __CHDRV_NET_H__
#include <stdio.h>
#include "stdlib.h"

#include "stapp_os.h"

/*****************************************************************************
* The following types are used in the SMSC Network stack
* u8_t is an unsigned 8 bit integer
* s8_t is a signed 8 bit integer
* u16_t is an unsigned 16 bit integer
* s16_t is a signed 16 bit integer
* u32_t is an unsigned 32 bit integer
* s32_t is a signed 32 bit integer
* mem_ptr_t is an unsigned integer who bit width is the same as a pointer (void *)
* NOTE:
*   mem_size_t is not currently used and may be deprecated in the future.
*****************************************************************************/
typedef unsigned   char    u8_t;
typedef signed     char    s8_t;
typedef unsigned   short   u16_t;
typedef signed     short   s16_t;
typedef unsigned   int    u32_t;
typedef signed     int    s32_t;

typedef u32_t mem_ptr_t;
typedef u16_t mem_size_t;

#ifndef boolean 
#define  boolean unsigned   char 
#endif

#ifndef true 
#define true 1
#endif
#ifndef false 
#define false 0
#endif

#ifdef NULL
#undef NULL
#endif
#define NULL 0UL

#ifdef CHDRV_NET_TIMEOUT_INFINITY
#undef CHDRV_NET_TIMEOUT_INFINITY
#endif
#define CHDRV_NET_TIMEOUT_INFINITY 0xFFFFFFFFUL

#ifdef CHDRV_NET_TIMEOUT_IMMEDIATE
#undef CHDRV_NET_TIMEOUT_IMMEDIATE
#endif
#define CHDRV_NET_TIMEOUT_IMMEDIATE 0x0UL

/*test typedef*/
/*--------------------------------start-----------------------------*/
typedef semaphore_t * CH_RTOS_SemaphoreHandler;
typedef task_t * CH_RTOS_TaskHadler;
typedef message_queue_t * CH_RTOS_MessageQueue_t;
/*--------------------------------end-----------------------------*/

typedef  CH_RTOS_SemaphoreHandler CHDRV_NET_SEMAPHORE;
typedef CH_RTOS_TaskHadler CHDRV_NET_THREAD;
typedef CH_RTOS_MessageQueue_t CHDRV_NET_MESSAGEQUEUE;
typedef void (*CHDRV_NET_InterruptHandler)(void *rvp_Arg);
typedef u32_t   CHDRV_NET_Handle_t;
typedef  void  (*CHDRV_NET_CallbackFun_t)(void *rvp_Arg);

typedef enum
{
	AUTO_DHCP,
	MANUAL
}IP_GET_MODEL;

typedef enum
{
	CM_PORT_MODEL,/*cable modem*/
	RJ45_PORT_MODEL/*RJ45 */
}DHCP_PORT_MODEL;

#define IP_STRING_LENGTH 4
#define MAC_ADDRESS_LENGTH 6
#define DHCP_OPTION_60_LEN 	30
#define HOST_NAME_LEN		20
typedef struct tagNetConfig
{
	u8_t macaddress[MAC_ADDRESS_LENGTH];			/*MAC Address*/
	u8_t ipaddress[IP_STRING_LENGTH];					/*IP Address*/
	u8_t netmask[IP_STRING_LENGTH];					/*Netmask*/
	u8_t gateway[IP_STRING_LENGTH];					/*Gateway Address*/
	u8_t dns[IP_STRING_LENGTH];						/*DNS Address*/
	IP_GET_MODEL ipmodel;							/*Get IP mode*/
	DHCP_PORT_MODEL dhcpport;						/*Get IP port,RJ43 OR CM*/
	s8_t Opt60ServerName[DHCP_OPTION_60_LEN];		/*Option60 server name*/
	s8_t Opt60ClientName[DHCP_OPTION_60_LEN];		/*Option60 client name*/
	s8_t hostname[HOST_NAME_LEN];					/*Local host name*/
	u8_t vod[IP_STRING_LENGTH];						/*VOD server address*/
	u8_t http[IP_STRING_LENGTH];						/*Reserved*/
}CHDRV_NET_Config_t;

/*Return result*/
typedef enum
{
	CHDRV_NET_OK = 0,					/*	操作成功*/
	CHDRV_NET_FAIL = -1,				/*	操作失败*/
	CHDRV_NET_NOT_INIT = -2,			/*	模块没有初始化*/
	CHDRV_NET_ALREADY_INIT = -3,		/*   模块已经初始化*/
	CHDRV_NET_TIMEOUT = -4,			/*	超时*/
	CHDRV_NET_INVALID_PARAM = -5	/*	参数错误*/
}CHDRV_NET_RESULT_e;

/*PHY mode*/
typedef enum 
{
	CHDRV_NET_PHY_MODE_10MHD,	/*10M半双工	*/
	CHDRV_NET_PHY_MODE_100MHD,/*100M半双工*/	
	CHDRV_NET_PHY_MODE_10MFD,	/*10M全双工	*/
	CHDRV_NET_PHY_MODE_100MFD,	/*100M全双工*/
	CHDRV_NET_PHY_MODE_AUTO	/*自适应	*/
}CHDRV_NET_PHY_MODE_e;

/*Version*/
typedef struct
{
	s32_t i_InterfaceMainVer;/*为接口文档的主版本号*/
	s32_t i_InterfaceSubVer;	/*为接口文档的次版本号*/
	s32_t i_ModuleMainVer;	/*为模块实现的主版本*/
	s32_t i_ModuleSubVer;	/*为模块实现的次版本*/
}CHDRV_NET_Version_t;

typedef osclock_t CHDRV_NET_CLOCK_t;

/*-------------------------------DEFINE START-------------------------------------*/
/*netif define*/
#define NET_INTERFACE_NUM	1
#define NET_INTERFACE_NAME_LEN	20

/*print contrl*/
#define CHDRV_NET_TRACE_ENABLE		1
#define CHDRV_NET_NOTICE_ENABLE		1
#define CHDRV_NET_WARNING_ENABLE		1
#define CHDRV_NET_ASSERT_ENABLE		1
#define CHDRV_NET_ERROR_ENABLE		1
/*debug contrl*/
#define CHDRV_NET_PLATFORM_DEBUG	1
#define CHDRV_NET_DRIVER_DEBUG		1

#if (CHDRV_NET_TRACE_ENABLE)
#define CHDRV_NET_TRACE(condition,message)	do{	\
	if(condition){	\
		CHDRV_NET_Print("CHDRV_NET_TRACE:");	\
		CHDRV_NET_Print message ;		\
		CHDRV_NET_Print("\n");	\
	}	\
}while(0)
#else
#define CHDRV_NET_TRACE(condition,message) {}
#endif

#if (CHDRV_NET_NOTICE_ENABLE)
#define CHDRV_NET_NOTICE(condition,message)	do{	\
	if(condition){	\
		CHDRV_NET_Print("CHDRV_NET_NOTICE:");	\
		CHDRV_NET_Print message;		\
		CHDRV_NET_Print("\n");	\
	}	\
}while(0)
#else
#define CHDRV_NET_NOTICE(condition,message)	{}
#endif

#if (CHDRV_NET_WARNING_ENABLE)
#define CHDRV_NET_WARNING(condition,message)	do{	\
	if(condition){	\
		CHDRV_NET_Print("CHDRV_NET_WARNING:");	\
		CHDRV_NET_Print message;		\
		CHDRV_NET_Print("\n");	\
	}	\
}while(0)
#else
#define CHDRV_NET_WARNING(condition,message)	{}
#endif

#if (CHDRV_NET_ERROR_ENABLE)
#define CHDRV_NET_ERROR(condition,message)	do{	\
	if(condition){	\
		CHDRV_NET_Print("CHDRV_NET_ERROR:");	\
		CHDRV_NET_Print message;		\
		CHDRV_NET_Print("\n file=%s,line=%d\n",__FILE__,__LINE__);\
	}	\
}while(0)
#else
#define CHDRV_NET_ERROR(condition,message)	{}
#endif

#if (CHDRV_NET_ASSERT_ENABLE)
#define CHDRV_NET_ASSERT(condition)	do{	\
	if(!(condition)){							\
		CHDRV_NET_Print("CHDRV_NET:");	\
		CHDRV_NET_Print("ASSERT FAILURE at file=%s,line=%d\n",__FILE__,__LINE__);	\
	}	\
}while(0)
#else
#define CHDRV_NET_ASSERT(condition)	{}
#endif
/*-------------------------------DEFINE END-------------------------------------*/
/*-------------------------------OS----------------------------------*/

/*--------------------------------------------------------------------------*/
/* 函数名称 : CHDRV_NET_ThreadCreate								*/
/* 功能描述 : 创建一个任务							*/
/* 入口参数 : 											*/
/* *rpf_function,线程函数指针			*/
/* *rpv_Arg,线程函数指针参数				*/
/*  rui_StackSize,线程堆栈值						*/
/*  rsi_Priority	,线程优先级				*/
/*  rsc_Name,线程名			*/
/*  ruc_Flage,	标志量				*/
/* 出口参数 : 无											*/
/* 返回值: 成功返回任务指针,失败返回NULL					*/
/* 注意:								*/
/*--------------------------------------------------------------------------*/
CHDRV_NET_THREAD CHDRV_NET_ThreadCreate(void (*rpf_function) (void *rpv_Arg),void *rpv_Arg, u32_t rui_StackSize,s32_t rsi_Priority,s8_t *rsc_Name,u8_t ruc_Flags);
/*--------------------------------------------------------------------------*/
/* 函数名称 : CHDRV_NET_ThreadResume								*/
/* 功能描述 : 恢复一个任务为运行状态							*/
/* 入口参数 : 											*/
/* *rh_Task,任务句柄			*/
/* 出口参数 : 无											*/
/*返回值说明：成功返回CHDRV_NET_OK*/
/* 注意:	在CHDRV_NET_ThreadSuspend之后调用							*/
/*--------------------------------------------------------------------------*/
CHDRV_NET_RESULT_e CHDRV_NET_ThreadResume(CHDRV_NET_THREAD	rh_Task);
/*--------------------------------------------------------------------------*/
/* 函数名称 : CHDRV_NET_ThreadSuspend								*/
/* 功能描述 : 挂起一个任务							*/
/* 入口参数 : 											*/
/* *rh_Task,任务句柄			*/
/* 出口参数 : 无											*/
/*返回值说明：成功返回CHDRV_NET_OK*/
/* 注意:								*/
/*--------------------------------------------------------------------------*/
CHDRV_NET_RESULT_e  CHDRV_NET_ThreadSuspend(CHDRV_NET_THREAD rh_Task);
/*--------------------------------------------------------------------------*/
/* 函数名称 : CHDRV_NET_ThreadSetPriority								*/
/* 功能描述 : 设置一个任务的优先级							*/
/* 入口参数 : 											*/
/* *rh_Task,任务句柄			*/
/* ri_Priority,优先级				*/
/* 出口参数 : 无											*/
/*返回值说明：成功返回CHDRV_NET_OK*/
/* 注意:								*/
/*--------------------------------------------------------------------------*/
CHDRV_NET_RESULT_e  CHDRV_NET_ThreadSetPriority(CHDRV_NET_THREAD	rh_Task,s32_t ri_Priority);
/*--------------------------------------------------------------------------*/
/* 函数名称 : CHDRV_NET_ThreadDelay								*/
/* 功能描述 : 延时函数							*/
/* 入口参数 : 											*/
/* ri_DelayTimeMS,时长(ms)			*/
/* 出口参数 : 无											*/
/* 返回值: 无				*/
/* 注意:								*/
/*--------------------------------------------------------------------------*/
extern unsigned int ST_GetClocksPerSecond( void );
#define CHDRV_NET_ThreadDelay(ri_DelayTimeMS) task_delay(ST_GetClocksPerSecond()/1000*ri_DelayTimeMS)
/*--------------------------------------------------------------------------*/
/* 函数名称 : CHDRV_NET_ThreadLock								*/
/* 功能描述 : 任务加锁						*/
/* 入口参数 : 无											*/
/* 出口参数 : 无											*/
/*返回值说明：成功返回CHDRV_NET_OK*/
/* 注意:	必须与CHDRV_NET_ThreadUnLock成套使用				*/
/*--------------------------------------------------------------------------*/
CHDRV_NET_RESULT_e  CHDRV_NET_ThreadLock (void);
/*--------------------------------------------------------------------------*/
/* 函数名称 : CHDRV_NET_ThreadUnLock								*/
/* 功能描述 : 任务解锁						*/
/* 入口参数 : 无											*/
/* 出口参数 : 无											*/
/*返回值说明：成功返回CHDRV_NET_OK*/
/* 注意:	必须与CHDRV_NET_ThreadLock成套使用				*/
/*--------------------------------------------------------------------------*/
CHDRV_NET_RESULT_e CHDRV_NET_ThreadUnLock (void);
/*--------------------------------------------------------------------------*/
/* 函数名称 : CHDRV_NET_SemaphoreCreate								*/
/* 功能描述 : 创建一个信号量					*/
/* 入口参数 : 											*/
/* ri_Count,信号量初始值			*/
/* 出口参数 : */
/* rph_Semaphore,信号量指针*/
/*返回值说明：成功返回CHDRV_NET_OK*/
/* 注意:								*/
/*--------------------------------------------------------------------------*/
CHDRV_NET_RESULT_e CHDRV_NET_SemaphoreCreate(s32_t 	ri_Count,CHDRV_NET_SEMAPHORE *rph_Semaphore);
/*--------------------------------------------------------------------------*/
/* 函数名称 : CHDRV_NET_SemaphoreWait								*/
/* 功能描述 : 信号量等待							*/
/* 入口参数 : 											*/
/* rh_Semaphore,信号量句柄			*/
/* ri_TimeOut,超时时间(ms),-1为永久等待			*/
/* 出口参数 : 无											*/
/*返回值说明：成功返回CHDRV_NET_OK*/
/* 注意:								*/
/*--------------------------------------------------------------------------*/
CHDRV_NET_RESULT_e CHDRV_NET_SemaphoreWait(CHDRV_NET_SEMAPHORE rh_Semaphore,s32_t ri_TimeOut);
/*--------------------------------------------------------------------------*/
/* 函数名称 : CHDRV_NET_SemaphoreSignal								*/
/* 功能描述 : 信号量释放								*/
/* 入口参数 : 											*/
/* rh_Semaphore,信号量句柄			*/
/* 出口参数 : 无											*/
/*返回值说明：成功返回CHDRV_NET_OK*/
/* 注意:								*/
/*--------------------------------------------------------------------------*/
CHDRV_NET_RESULT_e  CHDRV_NET_SemaphoreSignal(CHDRV_NET_SEMAPHORE	 rh_Semaphore);
/*--------------------------------------------------------------------------*/
/* 函数名称 : CHDRV_NET_SemaphoreFree								*/
/* 功能描述 : 信号量销毁							*/
/* 入口参数 : 											*/
/* rh_Semaphore,信号量句柄			*/
/* 出口参数 : 无											*/
/*返回值说明：成功返回CHDRV_NET_OK			*/
/* 注意:								*/
/*--------------------------------------------------------------------------*/
CHDRV_NET_RESULT_e CHDRV_NET_SemaphoreFree(CHDRV_NET_SEMAPHORE rh_Semaphore);
/*--------------------------------------------------------------------------*/
/* 函数名称 : CHDRV_NET_CreateMessageQueue								*/
/* 功能描述 : 创建消息队列						*/
/* 入口参数 : 											*/
/* ui_MsgLen,消息元素长度			*/
/* ui_QueueLen,队列长度			*/
/* 出口参数 : 无											*/
/*返回值说明：成功返回消息队列句柄,否则返回NULL			*/
/* 注意:								*/
/*--------------------------------------------------------------------------*/
CHDRV_NET_MESSAGEQUEUE CHDRV_NET_CreateMessageQueue(u32_t ui_MsgLen,u32_t ui_QueueLen);
/*--------------------------------------------------------------------------*/
/* 函数名称 : CHDRV_NET_ClaimMessageTimeout								*/
/* 功能描述 : 向消息队列中申请一个消息元素						*/
/* 入口参数 : 											*/
/* MsgQueue,消息队列句柄		*/
/* ui_TimeMs,申请超时Ms		*/
/* 出口参数 : 无											*/
/*返回值说明：成功返回一个队列中一个消息的指针,否则返回NULL			*/
/* 注意:								*/
/*--------------------------------------------------------------------------*/
void * CHDRV_NET_ClaimMessageTimeout(CHDRV_NET_MESSAGEQUEUE MsgQueue,u32_t ui_TimeMs);
/*--------------------------------------------------------------------------*/
/* 函数名称 : CHDRV_NET_ClaimMessageTimeout								*/
/* 功能描述 : 销毁消息队列中一个消息元素						*/
/* 入口参数 : 											*/
/* MsgQueue,消息队列句柄		*/
/* up_Msg,消息元素句柄		*/
/* 出口参数 : 无											*/
/*返回值说明：见CHDRV_NET_RESULT_e定义				*/
/* 注意:								*/
/*--------------------------------------------------------------------------*/
CHDRV_NET_RESULT_e CHDRV_NET_MessageRelease(CHDRV_NET_MESSAGEQUEUE MsgQueue,void * up_Msg);
/*--------------------------------------------------------------------------*/
/* 函数名称 : CHDRV_NET_ClaimMessageTimeout								*/
/* 功能描述 : 发送消息队列中一个消息元素						*/
/* 入口参数 : 											*/
/* MsgQueue,消息队列句柄		*/
/* up_Msg,消息元素句柄		*/
/* 出口参数 : 无											*/
/*返回值说明：见CHDRV_NET_RESULT_e定义				*/
/* 注意:								*/
/*--------------------------------------------------------------------------*/
CHDRV_NET_RESULT_e CHDRV_NET_MessageSend(CHDRV_NET_MESSAGEQUEUE MsgQueue,void * up_Msg);
/*--------------------------------------------------------------------------*/
/* 函数名称 : CHDRV_NET_ClaimMessageTimeout								*/
/* 功能描述 : 接收消息						*/
/* 入口参数 : 											*/
/* MsgQueue,消息队列句柄		*/
/* ui_TimeMs,接收超时ms	*/
/* 出口参数 : 无											*/
/*返回值说明：成功返回一个队列中一个消息的指针,否则返回NULL			*/
/* 注意:								*/
/*--------------------------------------------------------------------------*/
void * CHDRV_NET_MessageReceive(CHDRV_NET_MESSAGEQUEUE MsgQueue,u32_t ui_TimeMs);

/*--------------------------------------------------------------------------*/
/* 函数名称 : CHDRV_NET_MemAlloc								*/
/* 功能描述 : 申请内存							*/
/* 入口参数 : 											*/
/* rui_Size,内存长度		*/
/* 出口参数 : 无											*/
/*返回值说明：成功返回内存指针，否则返回NULL			*/
/* 注意:								*/
/*--------------------------------------------------------------------------*/
void *CHDRV_NET_MemAlloc(u32_t rui_Size);
/*--------------------------------------------------------------------------*/
/* 函数名称 : CHDRV_NET_MemFree								*/
/* 功能描述 : 释放内存						*/
/* 入口参数 : 											*/
/* rp_MemHandle,内存指针			*/
/* 出口参数 : 无											*/
/*返回值说明：无			*/
/* 注意:								*/
/*--------------------------------------------------------------------------*/
void CHDRV_NET_MemFree(void *rp_MemHandle);

/*-------------------------------PLATFORM---------------------------------------*/

/*--------------------------------------------------------------------------*/
/* 函数名称 : CHDRV_NET_Print								*/
/* 功能描述 : 打印函数						*/
/* 入口参数 : 											*/
/* rsc_ContentStr,需打印的字符串			*/
/* 出口参数 : 无											*/
/*返回值说明：无		*/
/* 注意:								*/
/*--------------------------------------------------------------------------*/
void CHDRV_NET_Print(const char *rsc_ContentStr,...);
/*--------------------------------------------------------------------------*/
/* 函数名称 : CHDRV_NET_Chksum								*/
/* 功能描述 : checksum计算函数						*/
/* 入口参数 : 											*/
/* rvp_Data,数据指针			*/
/* rvp_Data,数据长度(字节数)			*/
/* 出口参数 : 无											*/
/*返回值说明：计算结果		*/
/* 注意:								*/
/*--------------------------------------------------------------------------*/
u16_t CHDRV_NET_Chksum (void *rvp_Data, s32_t rsi_Len);
/*--------------------------------------------------------------------------*/
/* 函数名称 : CHDRV_NET_LoadNetConfig								*/
/* 功能描述 : 从STB FLASH中读取网络配置信息						*/
/* 入口参数 : 无										*/
/* 出口参数 : */
/* Config,配置信息结构体*/
/*返回值说明：成功返回CHDRV_NET_OK			*/
/* 注意:								*/
/*--------------------------------------------------------------------------*/
CHDRV_NET_RESULT_e CHDRV_NET_LoadNetConfig(CHDRV_NET_Config_t * rph_NetConfig);
/*--------------------------------------------------------------------------*/
/* 函数名称 : CHDRV_NET_LoadNetConfig								*/
/* 功能描述 : 保存网络配置信息到机顶盒						*/
/* 入口参数 : 无										*/
/* 出口参数 : */
/* Config,配置信息结构体*/
/*返回值说明：成功返回CHDRV_NET_OK			*/
/* 注意:								*/
/*--------------------------------------------------------------------------*/
CHDRV_NET_RESULT_e CHDRV_NET_SaveNetConfig(CHDRV_NET_Config_t * rph_NetConfig);
/*--------------------------------------------------------------------------*/
/* 函数名称 : CHDRV_NET_TimeNow								*/
/* 功能描述 : 得到当前系统时间						*/
/* 入口参数 : 无										*/
/* 出口参数 : 无									*/
/*返回值说明：系统当前时钟(tick)			*/
/* 注意:								*/
/*--------------------------------------------------------------------------*/
#define CHDRV_NET_TimeNow() 			time_now()
/*--------------------------------------------------------------------------*/
/* 函数名称 : CHDRV_NET_TimePlus								*/
/* 功能描述 : 系统时钟和计算						*/
/* 入口参数 : 	*/
/* t1,时钟1*/
/* t2,时钟2*/
/* 出口参数 : 无									*/
/*返回值说明：计算和(t1+t2)		*/
/* 注意:								*/
/*--------------------------------------------------------------------------*/
#define CHDRV_NET_TimePlus(t1,t2) 		time_plus(t1,t2)
/*--------------------------------------------------------------------------*/
/* 函数名称 : CHDRV_NET_TimeAfter								*/
/* 功能描述 : 系统时钟比较						*/
/* 入口参数 : 无	*/
/* t1,时钟1*/
/* t2,时钟2*/
/* 出口参数 : 无									*/
/*返回值说明：(t1>t2)?1:0			*/
/* 注意:								*/
/*--------------------------------------------------------------------------*/
#define CHDRV_NET_TimeAfter(t1,t2) 		time_after(t1,t2)
/*--------------------------------------------------------------------------*/
/* 函数名称 : CHDRV_NET_TimeMinus								*/
/* 功能描述 : 系统时钟差计算				*/
/* 入口参数 : 无	*/
/* t1,时钟1*/
/* t2,时钟2*/
/* 出口参数 : 无									*/
/*返回值说明：计算差(t1-t2)		*/
/* 注意:								*/
/*--------------------------------------------------------------------------*/
#define CHDRV_NET_TimeMinus(t1,t2) 		time_minus(t1,t2)
/*--------------------------------------------------------------------------*/
/* 函数名称 : CHDRV_NET_TicksOfSecond								*/
/* 功能描述 : 计算1秒钟系统时钟的tick数						*/
/* 入口参数 : 无	*/
/* 出口参数 : 无									*/
/*返回值说明：系统1秒钟的时钟(tick)数			*/
/* 注意:								*/
/*--------------------------------------------------------------------------*/
#define CHDRV_NET_TicksOfSecond() 		ST_GetClocksPerSecond()
/*--------------------------------------------------------------------------*/
/* 函数名称 : CHDRV_NET_InterruptInstall								*/
/* 功能描述 : 向操作系统注册网卡中断函数					*/
/* 入口参数 : 无	*/
/* 出口参数 : int_func,中断函数指针,arg,中断函数参数									*/
/*返回值说明：见CHDRV_NET_RESULT_e定义			*/
/* 注意:								*/
/*--------------------------------------------------------------------------*/
CHDRV_NET_RESULT_e CHDRV_NET_InterruptInstall(CHDRV_NET_InterruptHandler * int_func,void * arg);
/*--------------------------------------------------------------------------*/
/* 函数名称 : CHDRV_NET_InterruptUninstall								*/
/* 功能描述 : 注销网卡中断函数					*/
/* 入口参数 : 无	*/
/* 出口参数 : 无									*/
/*返回值说明：见CHDRV_NET_RESULT_e定义			*/
/* 注意:								*/
/*--------------------------------------------------------------------------*/
CHDRV_NET_RESULT_e CHDRV_NET_InterruptUninstall(void);

/*-------------------------------DRIVER---------------------------------------*/
/*--------------------------------------------------------------------------*/
/* 函数名称 : CHDRV_NET_Init								*/
/* 功能描述 : 网络设备初始化								*/
/* 入口参数 : 											*/
/* rpfun_CallBack,回调函数指针，暂时不用，供以后扩展使用	*/
/* rph_NetDevice,设备句柄									*/
/* ruc_MacAddr ,MAC地址									*/
/* 出口参数 : 无											*/
/* 返回值: 成功返回CHDRV_NET_OK,否则失败					*/
/* 注意:	此函数一般不重复调用							*/
/*--------------------------------------------------------------------------*/
CHDRV_NET_RESULT_e CHDRV_NET_Init(void);/**/
/*--------------------------------------------------------------------------*/
/*函数名:CHDRV_TCPIP_Handle_t *CHDRV_TCPIP_OpenEthenet ( )    */
/*开发人和开发时间:flex 2009-04-14                        */
/*函数功能描述:打开一个以太网控制器                               */
/*输入参数：	*/
/*ri_NetifIndex:网络控制器序号 */
/*输出参数:                                                     */
/*rph_NetDevice:网络控制器句柄*/
/*返回值说明：CHDRV_NET_OK表示操作成功,其他表示操作失败*/   
/*调用注意事项:                                                   */
/*其他说明:                                                       */  
/*--------------------------------------------------------------------------*/
CHDRV_NET_RESULT_e CHDRV_NET_Open(s32_t ri_NetifIndex, CHDRV_NET_Handle_t * rph_NetDevice);
/*--------------------------------------------------------------------------*/
/*函数名:CHDRV_NET_Close */
/*开发人和开发时间:flex 2009-04-14                        */
/*函数功能描述:关闭一个以太网控制器                               */
/*输入参数：*/
/*rph_NetDevice:网络设备句柄		*/
/*输出参数: 无                                                    */
/*使用的全局变量、表和结构：                                      */
/*调用的主要函数列表：                                            */
/*返回值说明：CHDRV_NET_OK表示操作成功                   */   
/*调用注意事项:                                                   */
/*其他说明:                                                       */  
/*--------------------------------------------------------------------------*/
CHDRV_NET_RESULT_e CHDRV_NET_Close( CHDRV_NET_Handle_t rph_NetDevice);
/*--------------------------------------------------------------------------*/
/* 函数名称 : CHDRV_NET_Recv									*/
/* 功能描述 : 获取当前将要被接收的数据包长度*/
/* 入口参数 :						 */
/*  rh_NetDevice,设备句柄				*/
/* rpuc_Buffer,数据缓冲指针			*/
/* rpi_Length,数据缓冲长度				*/
/* 出口参数 :										 */
/*  rpi_Length,收到的数据长度					*/
/* 返回值:	成功返回CHDRV_NET_OK*/
/* 注意:			*/
/*--------------------------------------------------------------------------*/
CHDRV_NET_RESULT_e  CHDRV_NET_Recv(CHDRV_NET_Handle_t rh_NetDevice,u8_t *rpuc_Buffer,u16_t *rpi_Length);
/*--------------------------------------------------------------------------*/
/* 函数名称 : CHDRV_NET_Send											*/
/* 功能描述 : 发送一个数据包*/
/* 入口参数 : 									*/
/* rh_NetDevice,设备句柄					*/
/* rpuc_Data,数据指针				*/
/* rpi_Length,数据长度				*/
/* 出口参数 : 无																*/
/* 返回值:	成功返回CHDRV_NET_OK				*/
/* 注意:					*/
/*--------------------------------------------------------------------------*/
CHDRV_NET_RESULT_e  CHDRV_NET_Send(CHDRV_NET_Handle_t rh_NetDevice,u8_t *rpuc_Data,u16_t *rpi_Length);
/*--------------------------------------------------------------------------*/
/*函数名:CHDRV_NET_GetVersion( )                     */
/*开发人和开发时间:zengxianggen 2007-08-10                        */
/*函数功能描述:得到CHDRV_NET版本号*/
/*输入参数：  无*/
/*输出参数:     rpstru_Ver:版本信息*/                                              
/*返回值说明：成功返回CHDRV_NET_OK*/
/*调用注意事项:                                                   */
/*其他说明:                                                       */  
/*--------------------------------------------------------------------------*/
CHDRV_NET_RESULT_e CHDRV_NET_GetVersion(CHDRV_NET_Version_t *rpstru_Ver);
/*--------------------------------------------------------------------------*/
/*函数名:CHDRV_NET_Reset*/
/*开发人和开发时间:zengxianggen     2006-06-01                    */
/*函数功能描述:  复位ethenet 芯片                             */
/*使用的全局变量、表和结构：                                       */
/*输入:	*/
/* rh_NetDevice,设备句柄*/
/*返回值说明：成功返回CHDRV_NET_OK*/
/*--------------------------------------------------------------------------*/
CHDRV_NET_RESULT_e CHDRV_NET_Reset(CHDRV_NET_Handle_t rh_NetDevice);
/*--------------------------------------------------------------------------*/
/* 函数名称 : CHDRV_NET_GetPacketLength									*/
/* 功能描述 : 获取当前将要被接收的数据包长度*/
/* 入口参数 : 无						*/
/* 出口参数 : 无																*/
/* 返回值:	大于0正常				*/
/* 注意:			*/
/*--------------------------------------------------------------------------*/
u16_t CHDRV_NET_GetPacketLength (CHDRV_NET_Handle_t rph_NetDevice);
/*--------------------------------------------------------------------------*/
/*函数名:void CHDRV_NET_SetMacAddr(void)*/
/*开发人和开发时间:flex     2009-04-13                   */
/*函数功能描述: 设置网络设备MAC地址                            */
/*使用的全局变量、表和结构：                                       */
/*输入:	*/
/* rh_NetDevice,设备句柄*/
/* rpuc_MacAddr,MAC地址*/
/*返回值说明：成功返回CHDRV_NET_OK*/
/*--------------------------------------------------------------------------*/
CHDRV_NET_RESULT_e CHDRV_NET_SetMacAddr(CHDRV_NET_Handle_t rh_NetDevice,u8_t *rpuc_MacAddr);
/*--------------------------------------------------------------------------*/
/*函数名:CHDRV_NET_GetMacAddr( )                     */
/*开发人和开发时间:zengxianggen 2007-08-10                        */
/*函数功能描述:得到芯片中已设置的MAC地址*/
/*输入参数：  rh_NetDevice,设备句柄*/
/*输出参数:     rpuc_MacAddr,MAC地址*/                                              
/*返回值说明：成功返回CHDRV_NET_OK*/
/*调用注意事项:                                                   */
/*其他说明:                                                       */  
/*--------------------------------------------------------------------------*/
CHDRV_NET_RESULT_e CHDRV_NET_GetMacAddr(CHDRV_NET_Handle_t rh_NetDevice,u8_t *rpuc_MacAddr);
/*--------------------------------------------------------------------------*/
/*函数名:void CHDRV_NET_SetPHYMode(void)*/
/*开发人和开发时间:flex     2009-04-13                   */
/*函数功能描述:  设置物理连接模式                          */
/*输入:		*/
/* rh_NetDevice,设备句柄*/
/* renm_PHYMode,连接模式*/
/*返回值说明：成功返回CHDRV_NET_OK*/
/*--------------------------------------------------------------------------*/
CHDRV_NET_RESULT_e CHDRV_NET_SetPHYMode(CHDRV_NET_Handle_t rh_NetDevice,CHDRV_NET_PHY_MODE_e	renm_PHYMode);
/*--------------------------------------------------------------------------*/
/*函数名:void CHDRV_NET_SetMultiCast(void)*/
/*开发人和开发时间:flex     2009-04-13                   */
/*函数功能描述:  设置为多播模式                          */
/*使用的全局变量、表和结构：                                       */
/*输入:	*/
/* rh_NetDevice,设备句柄*/
/*返回值说明：成功返回CHDRV_NET_OK*/
/*--------------------------------------------------------------------------*/
CHDRV_NET_RESULT_e CHDRV_NET_SetMultiCast(CHDRV_NET_Handle_t	rh_NetDevice);
/*--------------------------------------------------------------------------*/
/*函数名:void CHDRV_NET_GetLinkStatus*/
/*开发人和开发时间:flex     2009-04-13                   */
/*函数功能描述: 得到link状态                         */
/*输入:	*/
/* rh_NetDevice,设备句柄*/
/* 输出:*/
/* rpuc_LinkStatus,link状态*/
/*返回值说明：成功返回CHDRV_NET_OK*/
/*--------------------------------------------------------------------------*/
CHDRV_NET_RESULT_e CHDRV_NET_GetLinkStatus(CHDRV_NET_Handle_t	rh_NetDevice,u8_t*rpuc_LinkStatus);
/*--------------------------------------------------------------------------*/
/*函数名:void CHDRV_NET_Term(void)*/
/*开发人和开发时间:flex     2009-04-13                   */
/*函数功能描述:  关闭NET模块                             */
/*使用的全局变量、表和结构：                                       */
/*输入:无								*/
/*返回值说明：成功返回CHDRV_NET_OK*/
/*--------------------------------------------------------------------------*/
CHDRV_NET_RESULT_e CHDRV_NET_Term (void);
/*--------------------------------------------------------------------------*/
/*函数名:void CHDRV_NET_DebugInfo(void)*/
/*开发人和开发时间:flex     2009-04-13                   */
/*函数功能描述:  打印驱动中的定状态信息，供调试用      */
/*使用的全局变量、表和结构：                                       */
/*输入:无								*/
/*返回值说明：无*/
/*--------------------------------------------------------------------------*/
void CHDRV_NET_DebugInfo(void);

#endif/*__CHDRV_NET_H__*/


