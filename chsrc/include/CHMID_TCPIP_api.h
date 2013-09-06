/*******************************************************************************
* 文件名称 : CHMID_TCPIP_api.h											
* 功能描述 : HMID_TCPIP模块提供给上层应用调用的功能接口函数定义，及其一些数据结构
* ，宏，常量等定义。共应用层编程调用
*
* 作者:flex
* 修改历史:
* 日期					作者						描述
* 2009/3/30				 flex							create
*
********************************************************************************/
#ifndef __CHMID_TCPIP_H__
#define __CHMID_TCPIP_H__
#include "CHDRV_NET.h"

#define AF_INET				2	/*IPv4协议族*/
#define AF_INET6				28	/*IPv6协议族*/

#define SOCK_STREAM		1	/*流数据传输socket类型*/
#define SOCK_DGRAM			2	/*数据报传输socket类型*/
#define SOCK_RAW			3	/*原始socket类型*/

/*---------------------------NET status define-----------------------*/
#define CHMID_TCPIP_UNUSABLE		0x0000/*net can't be used*/
#define CHMID_TCPIP_LINKUP			0x0001/*phy link up*/
#define CHMID_TCPIP_RENEWING_IP	0x0002/*updating ip*/
#define CHMID_TCPIP_USABLE		0x0004/*net can be used*/

#define CHMID_TCPIP_O_NONBLOCK    04000U

/*-------------------------------------------------------------------
|CHMID_TCPIP_HTONS:字符串格式端口转换成网络字节序格式				|
|eg:0x50(80) --> 0x5000												|
--------------------------------------------------------------------*/
#define CHMID_TCPIP_HTONS(num16bit)						\
	((u16_t)(((((u16_t)(num16bit))&0x00FF)<<8)|		\
	((((u16_t)(num16bit))&0xFF00)>>8)))
/*-------------------------------------------------------------------
|CHMID_TCPIP_NTOHS:字符串格式端口转换成网络字节序格式				|
|eg:0x5000 --> 0x50(80)												|
--------------------------------------------------------------------*/
#define CHMID_TCPIP_NTOHS(num16bit)	CHMID_TCPIP_HTONS(num16bit)
/*-------------------------------------------------------------------
|CHMID_TCPIP_HTONL:主机字节序格式端口转换成网络字节序格式			|
|eg:0xc0a80001 --> 0x100a8c0											|
--------------------------------------------------------------------*/
#define CHMID_TCPIP_HTONL(num32bit)		\
	(((((u32_t)(num32bit))&0x000000FF)<<24)|				\
	((((u32_t)(num32bit))&0x0000FF00)<<8)|	\
	((((u32_t)(num32bit))&0x00FF0000)>>8)|	\
	((((u32_t)(num32bit))&0xFF000000)>>24))
/*-------------------------------------------------------------------
|CHMID_TCPIP_NTOHL:网络字节序格式IP转换成主机字节序格式				|
|eg:0x100a8c0 --> 0xc0a80001											|
--------------------------------------------------------------------*/
#define CHMID_TCPIP_NTOHL(num32bit)	CHMID_TCPIP_HTONL(num32bit)

#define CHMID_TCPIP_FD_SETSIZE    16
#define CHMID_TCPIP_FD_SET(n, p)  ((p)->fd_bits[(n)/8] |=  (1 << ((n) & 7)))
#define CHMID_TCPIP_FD_CLR(n, p)  ((p)->fd_bits[(n)/8] &= ~(1 << ((n) & 7)))
#define CHMID_TCPIP_FD_ISSET(n,p) ((p)->fd_bits[(n)/8] &   (1 << ((n) & 7)))
#define CHMID_TCPIP_FD_ZERO(p)    memset((void*)(p),0,sizeof(*(p)))

  typedef struct 
{
	u8_t fd_bits [(CHMID_TCPIP_FD_SETSIZE+7)/8];
} CHMID_TCPIP_FDSet_t;

typedef struct
{
	u8_t ui_IPAddr[IP_STRING_LENGTH] ;
	s32_t i_Datalen;	
	s32_t  i_Timeoutms;
	s32_t  i_TTL;
} CHMID_TCPIP_Pingcfg_t;

typedef struct
{
	u8_t uc_Len;				/*地址长度*/
	u8_t us_Family;			/*AF_INET*/
	u16_t us_Port;			/*端口*/		
	u32_t ui_IP;				/*IP地址*/	
	u8_t uc_zero[8];			/*保留*/	
}CHMID_TCPIP_SockAddr_t;


typedef enum
{
	CHMID_LINK_UP,                /*   物理网络处于连通状态*/
	CHMID_LINK_DOWN          /*  物理网络处于中断状态*/
}CHMID_TCPIP_LINK_e;

typedef enum
{
	CHMID_TCPIP_OK = 0,				/*	操作成功*/
	CHMID_TCPIP_FAIL = -1,				/*	操作失败*/
	CHMID_TCPIP_NOT_INIT = -2,		/*	模块没有初始化*/
	CHMID_TCPIP_ALREADY_INIT = -3,	/*    模块已经初始化*/
	CHMID_TCPIP_TIMEOUT = -4,			/*	超时*/
	CHMID_TCPIP_INVALID_PARAM = -5	/*	参数错误*/
}CHMID_TCPIP_RESULT_e;

typedef enum
{
	DHCP_SET_OPTION_60_SERVER,	/*服务器*/
	DHCP_SET_OPTION_60_CLIENT	/*客户端*/
}CHMID_TCPIP_OPTION_e;
/*Version*/
typedef struct
{
	s32_t i_InterfaceMainVer;/*为接口文档的主版本号*/
	s32_t i_InterfaceSubVer;	/*为接口文档的次版本号*/
	s32_t i_ModuleMainVer;	/*为模块实现的主版本*/
	s32_t i_ModuleSubVer;	/*为模块实现的次版本*/
}CHMID_TCPIP_Version_t;
/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
/* 函数名称 : CHMID_TCPIP_Init								*/
/* 功能描述 : 初始化网络协议栈及其相关硬件复位和驱动						*/
/* 入口参数 : 无											*/
/* 出口参数 : 无											*/
/* 返回值说明：成功返回CHDRV_NET_OK*/
/* 注意:				*/
/*--------------------------------------------------------------------------*/
CHMID_TCPIP_RESULT_e  CHMID_TCPIP_Init(void);
/*--------------------------------------------------------------------------*/
/* 函数名称 : CHMID_TCPIP_GetVersion								*/
/* 功能描述 : 得到CHMID_TCPIP版本号						*/
/* 入口参数 : 无											*/
/* 出口参数 : 无											*/
/* 返回值说明：成功返回CHDRV_NET_OK*/
/* 注意:				*/
/*--------------------------------------------------------------------------*/
CHMID_TCPIP_RESULT_e CHMID_TCPIP_GetVersion(CHMID_TCPIP_Version_t *c_Ver);
/*--------------------------------------------------------------------------*/
/* 函数名称 : CHMID_TCPIP_Term								*/
/* 功能描述 : 停止运行网络部分						*/
/* 入口参数 : 无											*/
/* 出口参数 : c_Ver,版本结构体											*/
/* 返回值说明：成功返回CHDRV_NET_OK*/
/* 注意:	一般不调用，在调用之后需要重新调用CHMID_TCPIP_Init启动			*/
/*--------------------------------------------------------------------------*/
CHMID_TCPIP_RESULT_e  CHMID_TCPIP_Term(void);
/*--------------------------------------------------------------------------*/
/* 函数名称 : CHMID_TCPIP_GetIP								*/
/* 功能描述 : 读取当前系统IP地址						*/
/* 入口参数 : 无											*/
/* 出口参数 : rpui_IPAddr,ip地址											*/
/* 返回值说明：成功返回CHDRV_NET_OK*/
/* 注意: 网络字节序				*/
/*--------------------------------------------------------------------------*/
CHMID_TCPIP_RESULT_e CHMID_TCPIP_GetIP(u32_t *rpui_IPAddr);
/*--------------------------------------------------------------------------*/
/* 函数名称 : CHMID_TCPIP_GetGateWay								*/
/* 功能描述 : 读取当前系统网关地址						*/
/* 入口参数 : 无											*/
/* 出口参数 : rpui_Gateway,网关地址											*/
/* 返回值说明：成功返回CHDRV_NET_OK*/
/* 注意: 网络字节序				*/
/*--------------------------------------------------------------------------*/
CHMID_TCPIP_RESULT_e CHMID_TCPIP_GetGateWay(u32_t *rpui_Gateway);
/*--------------------------------------------------------------------------*/
/* 函数名称 : CHMID_TCPIP_GetNetMask								*/
/* 功能描述 : 读取当前系统子网掩码						*/
/* 入口参数 : 无											*/
/* 出口参数 : rpui_NetMask,子网掩码											*/
/* 返回值说明：成功返回CHDRV_NET_OK*/
/* 注意: 网络字节序				*/
/*--------------------------------------------------------------------------*/
CHMID_TCPIP_RESULT_e CHMID_TCPIP_GetNetMask(u32_t *rpui_NetMask);
/*--------------------------------------------------------------------------*/
/* 函数名称 : CHMID_TCPIP_GetNetMask								*/
/* 功能描述 : 读取当前系统DNS组数						*/
/* 入口参数 : 无											*/
/* 出口参数 : rpi_DNSServerNum,DNS组数											*/
/* 返回值说明：成功返回CHDRV_NET_OK*/
/* 注意:最大为2		*/
/*--------------------------------------------------------------------------*/
CHMID_TCPIP_RESULT_e CHMID_TCPIP_GetDNSServerNum(s32_t 	*rpi_DNSServerNum);
/*--------------------------------------------------------------------------*/
/* 函数名称 : CHMID_TCPIP_GetNetMask								*/
/* 功能描述 : 根据DNS序号读取当前系统DNS地址						*/
/* 入口参数 : ri_DNSSequenceNum,DNS序号,从0开始,最大为1	*/
/* 出口参数 : rpui_DnsServerIP,DNS地址							*/
/* 返回值说明：成功返回CHDRV_NET_OK*/
/* 注意: 网络字节序				*/
/*--------------------------------------------------------------------------*/
CHMID_TCPIP_RESULT_e CHMID_TCPIP_GetDNSServer (s32_t 	ri_DNSSequenceNum,u32_t * rpui_DnsServerIP);
/*--------------------------------------------------------------------------*/
/* 函数名称 : CHMID_TCPIP_FactoryTest								*/
/* 功能描述 : 工厂测试软硬件系统是否正常						*/
/* 入口参数 : 										*/
/* ruc_SrcIp,本地ip地址				*/
/* ruc_SrcNetmask,本地子网掩码		*/
/* ruc_SrcGw,本地网关地址				*/
/* ruc_SrcDns,本地DNS地址				*/
/* ruc_DstIp,目标ip地址				*/
/* 出口参数 : 无												*/
/* 返回值说明：成功返回CHDRV_NET_OK*/
/* 注意:				*/
/*--------------------------------------------------------------------------*/
CHMID_TCPIP_RESULT_e CHMID_TCPIP_FactoryTest
(u8_t ruc_SrcIp [ 4 ], u8_t ruc_SrcNetmask [ 4 ], u8_t ruc_SrcGw [ 4 ], u8_t ruc_SrcDns [ 4 ], u8_t ruc_DstIp [ 4 ]);
/*--------------------------------------------------------------------------*/
/* 函数名称 : CHMID_TCPIP_GetMACAddr								*/
/* 功能描述 : 获取当前系统所使用的MAC地址 						*/
/* 入口参数 : 无											*/
/* 出口参数 : rpuc_MACAddr,MAC地址							*/
/* 返回值说明：成功返回CHDRV_NET_OK*/
/* 注意:				*/
/*--------------------------------------------------------------------------*/
CHMID_TCPIP_RESULT_e CHMID_TCPIP_GetMACAddr(u8_t *rpuc_MACAddr);
/*--------------------------------------------------------------------------*/
/* 函数名称 : CHMID_TCPIP_Ping								*/
/* 功能描述 : 						*/
/* 入口参数 : 			*/
/* rch_Pingcfg,ping测试配置信息			*/
/* ri_Timems,ping测试超时时间,单位ms*/
/* 出口参数 : 无											*/
/* 返回值说明：成功返回CHDRV_NET_OK*/
/* 注意:				*/
/*--------------------------------------------------------------------------*/
CHMID_TCPIP_RESULT_e CHMID_TCPIP_Ping(CHMID_TCPIP_Pingcfg_t rch_Pingcfg,s32_t *ri_Timems);
/*--------------------------------------------------------------------------*/
/* 函数名称 : CHMID_TCPIP_FactorySet								*/
/* 功能描述 : 出厂设置						*/
/* 入口参数 : 无											*/
/* 出口参数 : 无											*/
/* 返回值说明：无*/
/* 注意:				*/
/*--------------------------------------------------------------------------*/
void CHMID_TCPIP_FactorySet(void);
/*--------------------------------------------------------------------------*/
/* 函数名称 : CHMID_TCPIP_NetConfigRenew								*/
/* 功能描述 : 刷新网络配置信息,重新获取IP						*/
/* 入口参数 : NetConfig,网络配置信息							*/
/* 出口参数 : 无											*/
/* 返回值说明：成功返回CHDRV_NET_OK*/
/* 注意:				*/
/*--------------------------------------------------------------------------*/
CHMID_TCPIP_RESULT_e CHMID_TCPIP_NetConfigRenew(CHDRV_NET_Config_t  NetConfig);
/*--------------------------------------------------------------------------*/
/* 函数名称 : CHMID_TCPIP_GetNetConfig								*/
/* 功能描述 : 获取当前网络配置信息			*/
/* 入口参数 :  无								*/
/* 出口参数 :	NetConfig,网络配置信息							*/
/* 返回值说明：成功返回CHDRV_NET_OK*/
/* 注意:				*/
/*--------------------------------------------------------------------------*/
CHMID_TCPIP_RESULT_e CHMID_TCPIP_GetNetConfig(CHDRV_NET_Config_t * pNetConfig);

/*--------------------------------------------------------------------------*/
/* 函数名称 : CHMID_TCPIP_GetHostName								*/
/* 功能描述 : 获取本地主机网络名					*/
/* 入口参数 : ri_NameLen,名字缓冲长度											*/
/* 出口参数 : rpc_HostName名字缓冲											*/
/* 返回值说明：成功返回CHDRV_NET_OK*/
/* 注意:				*/
/*--------------------------------------------------------------------------*/
CHMID_TCPIP_RESULT_e CHMID_TCPIP_GetHostName(s8_t *rpc_HostName,s32_t ri_NameLen);
/*--------------------------------------------------------------------------*/
/* 函数名称 : CHMID_TCPIP_GetHostByName								*/
/* 功能描述 : 根据主机网络名获取主机的IP地址				*/
/* 入口参数 : rpc_HostDomainName,主机网络名							*/
/* 出口参数 : rpui_HostIP,主机IP地址											*/
/* 返回值说明：成功返回CHDRV_NET_OK*/
/* 注意:				*/
/*--------------------------------------------------------------------------*/
CHMID_TCPIP_RESULT_e CHMID_TCPIP_GetHostByName(s8_t *rpc_HostDomainName,u32_t *rpui_HostIP);
/*--------------------------------------------------------------------------*/
/* 函数名称 : CHMID_TCPIP_GetHostByAddr								*/
/* 功能描述 : 根据主机地址获取其网络名					*/
/* 入口参数 : rui_HostIP,主机ip地址;ri_NameLen,名字缓冲长度*/
/* 出口参数 : rpc_HostDomainName,主机网络名		*/
/* 返回值说明：成功返回CHDRV_NET_OK*/
/* 注意:				*/
/*--------------------------------------------------------------------------*/
CHMID_TCPIP_RESULT_e CHMID_TCPIP_GetHostByAddr(u32_t rui_HostIP,s8_t *rpc_HostDomainName,s32_t ri_NameLen);

/*--------------------------------------------------------------------------*/
/* 函数名称 : CHMID_TCPIP_GetNetStatus								*/
/* 功能描述 : 						*/
/* 入口参数 : 无											*/
/* 出口参数 : 无											*/
/* 返回值说明：*/
/*返回一个unsigned short值,根据,每一位标记一个状态,由下面这些宏定义		*/
/*
 #define CHMID_TCPIP_UNUSABLE	0x0000
 #define CHMID_TCPIP_LINKUP		0x0001
 #define CHMID_TCPIP_RENEWING_IP	0x0002
 #define CHMID_TCPIP_USABLE		0x0004
*/
/* 注意:				*/
/*--------------------------------------------------------------------------*/
u16_t CHMID_TCPIP_GetNetStatus(void);
/*--------------------------------------------------------------------------*/
/* 函数名称 : CHMID_TCPIP_Socket								*/
/* 功能描述 : 创建一个socket						*/
/* 入口参数 : 									*/
/* ri_Domain,协议族(ipv4 or ipv6),目前仅支持IPV4		*/
/* ri_Type,协议类型,UDP/TCP		*/
/* ri_Protocol,保留,一般置0	*/
/* 出口参数 : 无											*/
/* 返回值说明：返回一个非负的套节字句柄值	*/
/* 注意:				*/
/*--------------------------------------------------------------------------*/
s32_t CHMID_TCPIP_Socket (s32_t ri_Domain,s32_t ri_Type,s32_t ri_Protocol);
/*--------------------------------------------------------------------------*/
/* 函数名称 : CHMID_TCPIP_Socket								*/
/* 功能描述 : 邦定一个套节字句柄				*/
/* 入口参数 : 									*/
/* ri_SockFd,套节字句柄					*/
/* rpstru_MyAddr,本地地址结构				*/
/* ri_AddrLen,本地地址结构长度				*/
/* 出口参数 : 无											*/
/* 返回值说明：成功返回0,否则返回负值	*/
/* 注意:				*/
/*--------------------------------------------------------------------------*/
s32_t CHMID_TCPIP_Bind(s32_t ri_SockFd,CHMID_TCPIP_SockAddr_t *rpstru_MyAddr,s32_t ri_AddrLen);
/*--------------------------------------------------------------------------*/
/* 函数名称 : CHMID_TCPIP_Listen								*/
/* 功能描述 : 监听已邦定套节字的端口						*/
/* 入口参数 : 									*/
/* ri_SockFd,套节字句柄					*/
/* ri_BackLog,可处理的最大客户端连接数				*/
/* 出口参数 : 无											*/
/* 返回值说明：成功返回0,否则返回负值	*/
/* 注意:				*/
/*--------------------------------------------------------------------------*/
s32_t CHMID_TCPIP_Listen(s32_t ri_SockFd,s32_t ri_BackLog);
/*--------------------------------------------------------------------------*/
/* 函数名称 : CHMID_TCPIP_Accept								*/
/* 功能描述 :  接收处理监听到的一个客户端连接						*/
/* 入口参数 : 									*/
/* ri_SockFd,已监听的套节字句柄					*/
/* 出口参数 : 										*/
/* rpstru_ClientAddr,客户端地址信息结构				*/
/* rpi_AddrLen,客户端地址信息结构长度				*/
/* 返回值说明：成功返回非负的客户端套节字句柄	*/
/* 注意:				*/
/*--------------------------------------------------------------------------*/
s32_t CHMID_TCPIP_Accept(s32_t ri_SockFd,CHMID_TCPIP_SockAddr_t *rpstru_ClientAddr,u32_t *rpi_AddrLen);
/*--------------------------------------------------------------------------*/
/* 函数名称 : CHMID_TCPIP_Connect								*/
/* 功能描述 : 连接服务器						*/
/* 入口参数 : 									*/
/* ri_SockFd,套节字句柄					*/
/* rpstru_ServerAddr,服务器地址结构				*/
/* ri_AddrLen,服务器地址结构长度				*/
/* 出口参数 : 无											*/
/* 返回值说明：成功返回0,否则返回负值	*/
/* 注意:				*/
/*--------------------------------------------------------------------------*/
s32_t CHMID_TCPIP_Connect(s32_t ri_SockFd,CHMID_TCPIP_SockAddr_t *rpstru_ServerAddr,s32_t ri_AddrLen);
/*--------------------------------------------------------------------------*/
/* 函数名称 : CHMID_TCPIP_Send								*/
/* 功能描述 : 发送一段数据						*/
/* 入口参数 : 									*/
/* ri_SockFd,套节字句柄					*/
/* rpuc_DataBuf,数据缓存指针				*/
/* ri_DataLen,数据长度						*/
/* rui_Flags,保留,一般置为0					*/
/* 出口参数 : 无											*/
/* 返回值说明：成功返回已发送的数据长度	*/
/* 注意:									*/
/*--------------------------------------------------------------------------*/
s32_t CHMID_TCPIP_Send(s32_t ri_SockFd,void *rpuc_DataBuf,s32_t ri_DataLen,u32_t rui_Flags);
/*--------------------------------------------------------------------------*/
/* 函数名称 : CHMID_TCPIP_Recv								*/
/* 功能描述 : 收取一段一段数据						*/
/* 入口参数 : 									*/
/* ri_SockFd,套节字句柄					*/
/* rpuc_DataBuf,数据缓存指针				*/
/* ri_DataLen,数据缓存长度						*/
/* rui_Flags,保留,一般置为0					*/
/* 出口参数 : 无											*/
/* 返回值说明：成功返回已接收的数据长度	*/
/* 注意:									*/
/*--------------------------------------------------------------------------*/
s32_t CHMID_TCPIP_Recv(s32_t ri_SockFd,void *rpuc_DataBuf,s32_t ri_BufLen,u32_t rui_Flags);
/*--------------------------------------------------------------------------*/
/* 函数名称 : CHMID_TCPIP_SendTo								*/
/* 功能描述 : 发送一段数据						*/
/* 入口参数 : 									*/
/* ri_SockFd,套节字句柄					*/
/* rpuc_DataBuf,数据缓存指针				*/
/* ri_DataLen,数据长度						*/
/* rui_Flags,保留,一般置为0					*/
/* rpstru_DestAddr,目标主机地址结构				*/
/* ri_AddrLen,目标主机地址结构长度					*/
/* 出口参数 : 无											*/
/* 返回值说明：成功返回已发送的数据长度	*/
/* 注意:									*/
/*--------------------------------------------------------------------------*/
s32_t CHMID_TCPIP_SendTo(s32_t ri_SockFd,
	void *rpuc_DataBuf,s32_t ri_DataLen,u32_t rui_Flags,CHMID_TCPIP_SockAddr_t *rpstru_DestAddr,s32_t ri_AddrLen);
/*--------------------------------------------------------------------------*/
/* 函数名称 : CHMID_TCPIP_SendTo								*/
/* 功能描述 : 发送一段数据						*/
/* 入口参数 : 									*/
/* ri_SockFd,套节字句柄					*/
/* rpuc_DataBuf,数据缓存指针				*/
/* ri_BufLen,数据长度						*/
/* rui_Flags,保留,一般置为0					*/
/* rpstru_SrcAddr,数据源主机地址结构				*/
/* rpi_AddrLen,数据源主机地址结构长度					*/
/* 出口参数 : 无											*/
/* 返回值说明：成功返回已接收的数据长度	*/
/* 注意:									*/
/*--------------------------------------------------------------------------*/
s32_t CHMID_TCPIP_RecvFrom(s32_t ri_SockFd,
	void *rpuc_DataBuf,s32_t ri_BufLen,u32_t rui_Flags,CHMID_TCPIP_SockAddr_t *rpstru_SrcAddr,u32_t *rpi_AddrLen);
/*--------------------------------------------------------------------------*/
/* 函数名称 : CHMID_TCPIP_ShutDown								*/
/* 功能描述 : 关闭一个与客户端的连接						*/
/* 入口参数 : 									*/
/* ri_SockFd,套节字句柄					*/
/* ri_HowTo,保留,一般置为0			*/
/* 出口参数 : 无											*/
/* 返回值说明：成功返回已接收的数据长度	*/
/* 注意:	用于服务器程序								*/
/*--------------------------------------------------------------------------*/
s32_t CHMID_TCPIP_ShutDown(s32_t ri_SockFd, s32_t ri_HowTo);
/*--------------------------------------------------------------------------*/
/* 函数名称 : CHMID_TCPIP_Close								*/
/* 功能描述 : 关闭一个套节字						*/
/* 入口参数 : 									*/
/* ri_SockFd,套节字句柄					*/
/* 出口参数 : 无											*/
/* 返回值说明：成功返回0,否则返回负值	*/
/* 注意:								*/
/*--------------------------------------------------------------------------*/
s32_t CHMID_TCPIP_Close(s32_t ri_SockFd);
/*--------------------------------------------------------------------------*/
/* 函数名称 : CHMID_TCPIP_GetPeerName								*/
/* 功能描述 : 获取制定已绑定套节字的端口名						*/
/* 入口参数 : 									*/
/* ri_SockFd,套节字句柄					*/
/* 出口参数 : 											*/
/* rpstru_PeerAddr,名字结构			*/
/* rpi_AddrLen,名字结构长度		*/
/* 返回值说明：成功返回0,否则返回负值	*/
/* 注意:	暂未实现			*/
/*--------------------------------------------------------------------------*/
s32_t CHMID_TCPIP_GetPeerName(s32_t  ri_SockFd,CHMID_TCPIP_SockAddr_t *rpstru_PeerAddr,u32_t *rpi_AddrLen);
/*--------------------------------------------------------------------------*/
/* 函数名称 : CHMID_TCPIP_GetSocketOpt								*/
/* 功能描述 : 获取一个套接口选项						*/
/* 入口参数 : 无											*/
/* 出口参数 : 无											*/
/* 返回值说明：成功返回0,否则返回负值	*/
/* 注意:	暂未实现			*/
/*--------------------------------------------------------------------------*/
s32_t CHMID_TCPIP_GetSocketOpt(s32_t ri_SockFd,s32_t i_Level,s32_t ri_OptName,void *rpv_OptVal,u32_t *rpi_OptLen);
/*--------------------------------------------------------------------------*/
/* 函数名称 : CHMID_TCPIP_SetSocketOpt								*/
/* 功能描述 : 设置一个套接口选项						*/
/* 入口参数 : 无											*/
/* 出口参数 : 无											*/
/* 返回值说明：成功返回0,否则返回负值	*/
/* 注意:	暂未实现			*/
/*--------------------------------------------------------------------------*/
s32_t CHMID_TCPIP_SetSocketOpt(s32_t ri_SockFd,s32_t ri_Level,s32_t ri_OptName,const void *rpv_OptVal,u32_t ri_OptLen);
/*--------------------------------------------------------------------------*/
/* 函数名称 : CHMID_TCPIP_Fctrl								*/
/* 功能描述 : 对一个套节字设置操作属性						*/
/* 入口参数 : 									*/
/* ri_SockFd,套节字句柄					*/
/* ri_Command,操作类型			*/
/* ri_Arguement,实际设置的值		*/
/* 出口参数 : 无											*/
/* 返回值说明：成功返回0,否则返回负值	*/
/* 注意:	功能同CHMID_TCPIP_Ioctl(),目前仅支持设置阻塞模式模式			*/
/*--------------------------------------------------------------------------*/
s32_t CHMID_TCPIP_Fctrl(s32_t ri_SockFd,s32_t ri_Command,void * ri_Arguement);
/*--------------------------------------------------------------------------*/
/* 函数名称 : CHMID_TCPIP_Select								*/
/* 功能描述 : 查询套节字状态						*/
/* 入口参数 : 									*/
/* ri_Maxfdnum,集合范围,一般为最大套节字加1					*/
/* ri_Timeout,超时时间,单位ms		*/
/* 出口参数 : 											*/
/* rpstru_Readset,可读集合					*/
/* rpstru_Writeset,可写集合				*/
/* rpstru_Exceptset,错误集合				*/
/* 返回值说明：*/
/* 大于0为可操作套节字个数		*/
/* 0为超时						*/
/* 负数为有错误发生						*/
/* 注意:				*/
/*--------------------------------------------------------------------------*/
s32_t CHMID_TCPIP_Select(s32_t ri_Maxfdnum,
CHMID_TCPIP_FDSet_t *rpstru_Readset,
CHMID_TCPIP_FDSet_t *rpstru_Writeset,
CHMID_TCPIP_FDSet_t  *rpstru_Exceptset,
s32_t  ri_Timeout);
/*--------------------------------------------------------------------------*/
/* 函数名称 : CHMID_TCPIP_GetSocketName								*/
/* 功能描述 : 获取套节字的端口名						*/
/* 入口参数 : 									*/
/* ri_SockFd,套节字句柄					*/
/* 出口参数 : 											*/
/* rp_Name,名字结构			*/
/* ruip_Namelen,名字结构长度		*/
/* 返回值说明：成功返回0,否则返回负值	*/
/* 注意:	暂未实现			*/
/*--------------------------------------------------------------------------*/
s32_t CHMID_TCPIP_GetSocketName(s32_t ri_SockFd, CHMID_TCPIP_SockAddr_t * rp_Name, u32_t * ruip_Namelen);
/*--------------------------------------------------------------------------*/
/* 函数名称 : CHMID_TCPIP_Read								*/
/* 功能描述 : 收取一段数据						*/
/* 入口参数 : 									*/
/* ri_SockFd,套节字句柄					*/
/* rvp_Mem,数据缓存指针				*/
/* ri_MemLen,数据缓存长度						*/
/* 出口参数 : 无											*/
/* 返回值说明：成功返回已接收的数据长度	*/
/* 注意:	同CHMID_TCPIP_Recv()								*/
/*--------------------------------------------------------------------------*/
s32_t CHMID_TCPIP_Read(s32_t ri_SockFd, void *rvp_Mem, s32_t ri_MemLen);
/*--------------------------------------------------------------------------*/
/* 函数名称 : CHMID_TCPIP_Write								*/
/* 功能描述 : 发送一段数据						*/
/* 入口参数 : 									*/
/* ri_SockFd,套节字句柄					*/
/* rvp_DataPtr,数据缓存指针				*/
/* ri_Size,数据缓存长度						*/
/* 出口参数 : 无											*/
/* 返回值说明：成功返回已接收的数据长度	*/
/* 注意:	同CHMID_TCPIP_Recv()								*/
/*--------------------------------------------------------------------------*/
s32_t CHMID_TCPIP_Write(s32_t ri_SockFd, void *rvp_DataPtr, s32_t ri_Size);
/*--------------------------------------------------------------------------*/
/* 函数名称 : CHMID_TCPIP_Ioctl								*/
/* 功能描述 : 对一个套节字设置操作属性						*/
/* 入口参数 : 									*/
/* ri_SockFd,套节字句柄					*/
/* ri_Cmd,操作类型			*/
/* rvp_Argp,实际设置的值		*/
/* 出口参数 : 											*/
/* 返回值说明：成功返回0,否则返回负值	*/
/* 注意:	功能同CHMID_TCPIP_Fctrl(),目前仅支持设置阻塞模式模式			*/
/*--------------------------------------------------------------------------*/
s32_t CHMID_TCPIP_Ioctl(s32_t ri_SockFd, s32_t ri_Cmd, void *rvp_Argp);
/*--------------------------------------------------------------------------*/
/* 函数名称 : CHMID_TCPIP_InetAddr								*/
/* 功能描述 : 字符串格式IP转换成网络字节序格式				*/
/* eg:192.168.0.1 --> 0x100a8c0							*/
/* 入口参数 : rc_Addr,ip 地址字符串											*/
/* 出口参数 : 无											*/
/* 返回值说明：失败返回0xFFFFFFFF*/
/* 注意:								*/
/*--------------------------------------------------------------------------*/
u32_t CHMID_TCPIP_InetAddr(const s8_t * rc_Addr);
/*--------------------------------------------------------------------------*/
/* 函数名称 : CHMID_TCPIP_InetAddr								*/
/* 功能描述 : 字符串格式IP转换成网络字节序格式				*/
/* eg:192.168.0.1 --> 0x100a8c0							*/
/* 入口参数 : rc_Addr,ip 地址字符串											*/
/* 出口参数 : rui_AddrOut,转换后的IP地址										*/
/* 返回值说明：成功返回1，失败返回0*/
/* 注意:								*/
/*--------------------------------------------------------------------------*/
s32_t CHMID_TCPIP_InetAton(const s8_t * rc_Addr,u32_t * rui_AddrOut);
/*--------------------------------------------------------------------------*/
/* 函数名称 : CHMID_TCPIP_GetIP								*/
/* 功能描述 : 网络字节序格式IP转换成字符串格式	.eg:0x100a8c0--> 192.168.0.1	*/
/* 入口参数 : rui_AddrIn,网络字节序格式IP地址				*/
/* 出口参数 : 无											*/
/* 返回值说明：成功返回转换后的IP地址字符串,否则返回NULL*/
/* 注意:				*/
/*--------------------------------------------------------------------------*/
s8_t * CHMID_TCPIP_InetNtoa(u32_t rui_AddrIn);

#endif
/*EOF*/
