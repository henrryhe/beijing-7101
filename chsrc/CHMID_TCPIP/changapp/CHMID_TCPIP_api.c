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

#include "dhcp.h"
#include "sockets.h"
#include "network_stack.h"
#include "icmpv4.h"
#include "task_manager.h"
#include "smsc_environment.h"
#include "smsc_threading.h"
#include "CHMID_TCPIP_api.h"
#include "CHMID_TCPIP_io.h"

u8_t SMSC_MAC_ADDRESS[6];
u8_t smsc_print_out = 0;
/*static*/ struct NETWORK_INTERFACE smscNetworkInterface;
CHDRV_NET_Handle_t EtherDviceHandle[NET_INTERFACE_NUM] = {0U};
static DHCP_DATA smscDhcpData;


/*函数名:void CH_ResetEthChip(void)*/
/*开发人和开发时间:zengxianggen     2006-06-01                    */
/*函数功能描述:  复位ethenet 芯片                             */
/*函数原理说明:                                                    */
/*使用的全局变量、表和结构：                                       */
/*输入:无                                                          */
/*调用的主要函数列表：                                             */
/*返回值说明：无*/

/******************************************************************/
void CHMID_TCPIP_ResetEthChip(void)
{
       CHDRV_NET_Reset(EtherDviceHandle[0]);
}
#ifdef M_WUFEI_081120

/************************函数说明***************************/
/*函数名:void CH_SetOption60Name(int setType,char * sName)                   */
/*开发人和开发时间:                */
/*函数功能描述: 初始化SMSC以太网驱动和TCPIP协议*/
/*函数原理说明:                                                    */
/*使用的全局变量、表和结构：                                       */
/*调用的主要函数列表：                                             */
/*返回值说明：无                  */

s8_t g_Opt60ServerName[DHCP_OPTION_60_LEN];
s8_t g_Opt60ClientName[DHCP_OPTION_60_LEN];
CHMID_TCPIP_RESULT_e CHMID_TCPIP_SetOption
(CHMID_TCPIP_OPTION_e setType,s8_t * sName)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(sName != NULL,CHMID_TCPIP_INVALID_PARAM);
	
	if(setType == DHCP_SET_OPTION_60_SERVER)
	{
		memset(g_Opt60ServerName,0,DHCP_OPTION_60_LEN);
		strcpy((char *)g_Opt60ServerName,(char *)sName);
	}
	else if(setType == DHCP_SET_OPTION_60_CLIENT)
	{
		memset(g_Opt60ClientName,0,DHCP_OPTION_60_LEN);
		strcpy((char *)g_Opt60ClientName,(char *)sName);
	}
	return CHMID_TCPIP_OK;
}
#endif
CHMID_TCPIP_LINK_e CHMID_TCPIP_GetLINKStatus ( void )
{
	u8_t uc_Ret = 0;
	CHDRV_NET_GetLinkStatus(EtherDviceHandle[0], &uc_Ret);
	if(uc_Ret == 0){
		return CHMID_LINK_DOWN;
	}else{
		return CHMID_LINK_UP;
	}
}


/************************函数说明***************************/
/*函数名:void CH_InitSMSCTCPIP(void                                               */
/*开发人和开发时间:                */
/*函数功能描述: 初始化SMSC以太网驱动和TCPIP协议*/
/*函数原理说明:                                                    */
/*使用的全局变量、表和结构：                                       */
/*调用的主要函数列表：                                             */
/*返回值说明：无                  */

s32_t CH_InitSMSCTCPIP
(u32_t ip,u32_t netmask,u32_t gateway,u8_t mac[6])
{
	/*复位网络芯片*/
       CHDRV_NET_Reset(EtherDviceHandle[0]);
	/* Network Stack Initialization */
	NetworkStack_Initialize();		                
	/*网络接口初始化*/
	NetworkInterface_Initialize(&smscNetworkInterface);
	/*设置MAC地址*/
	SMSC_MAC_ADDRESS[0] = mac[0];
	SMSC_MAC_ADDRESS[1] = mac[1];
	SMSC_MAC_ADDRESS[2] = mac[2];
	SMSC_MAC_ADDRESS[3] = mac[3];
	SMSC_MAC_ADDRESS[4] = mac[4];
	SMSC_MAC_ADDRESS[5] = mac[5];
	/*初始化底层*/
	Ethernet_InitializeInterface(&smscNetworkInterface,
			EthernetOutput,
			SMSC_MAC_ADDRESS,
			ETHERNET_FLAG_ENABLE_IPV4|ETHERNET_FLAG_ENABLE_IPV6);
	if(EthernetCardInfo_Initialize(&smscNetworkInterface)==NULL){
		SMSC_ERROR(("Smsc911x_InitializeInterface: Failed to initialize platform"));
		return -1;
	}
	/*复位，初始化驱动*/
       if(CHDRV_NET_Init()!=CHDRV_NET_OK) {
		SMSC_ERROR(("Failed to initialize interface"));
		return -1;
	}
	   /*打开并启动设备*/
	if(CHDRV_NET_Open(0,&EtherDviceHandle[0])!= CHDRV_NET_OK && EtherDviceHandle[0]!= 0){
		SMSC_ERROR(("Failed to open interface"));
		return -1;
	}
	/*设置MAC地址*/
	if(CHDRV_NET_SetMacAddr(EtherDviceHandle[0],SMSC_MAC_ADDRESS)!=CHDRV_NET_OK){
		SMSC_ERROR(("Failed to open interface"));
		CHDRV_NET_Close(EtherDviceHandle[0]);
		return -1;
	}else{
		CHDRV_NET_GetMacAddr(EtherDviceHandle[0],SMSC_MAC_ADDRESS);
	}
	/*初始化协议栈IO*/
	if(CH_InitEthernetIO(&smscNetworkInterface,0)!=ERR_OK){
		SMSC_ERROR(("Failed to initialize IO"));
		return -1;
	}
	/*网络接口设置*/
	NetworkStack_AddInterface(&smscNetworkInterface,1);
	/*设置IP*/
	Ipv4_SetAddresses(&smscNetworkInterface,ip,netmask,gateway);
	/*创建tcpip调度任务*/
{
	CHDRV_NET_THREAD smsc_tsk = 0;
       s32_t priority = 15;
	s32_t stackSize = 1024*100;
	char * name = "TCPIP_Process";

	smsc_tsk = smsc_thread_create((void(*)(void *))TaskManager_Run, \
			NULL,		\
			stackSize,	\
			priority, 		\
			(s8_t *)name,		\
			0);
	if(smsc_tsk == NULL)
	{
		SMSC_ERROR(("Failed to create  TCPIP_Process"));
		return -1;
	}
	else
	{
		SMSC_TRACE(TCPIP_DEBUG,("CH_InitSMSCTCPIP success!"));
		return 0;	
	}
}
}
CHMID_TCPIP_RESULT_e CHMID_TCPIP_GetVersion(CHMID_TCPIP_Version_t *rpstru_Ver)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(rpstru_Ver != NULL,CHMID_TCPIP_INVALID_PARAM);
	rpstru_Ver->i_InterfaceMainVer = 0;
	rpstru_Ver->i_InterfaceSubVer = 8;
	rpstru_Ver->i_ModuleMainVer = 1;
	rpstru_Ver->i_ModuleSubVer = 1;
	return CHDRV_NET_OK;
}
CHMID_TCPIP_RESULT_e CHMID_TCPIP_GetMACAddr(u8_t *rpuc_MACAddr)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(rpuc_MACAddr!=NULL,CHMID_TCPIP_INVALID_PARAM);
	memcpy(rpuc_MACAddr,&SMSC_MAC_ADDRESS[0],6);
	return CHMID_TCPIP_OK;
}

static boolean b_DhcpStart = false;
/************************函数说明***************************/
/*函数名:void CH_DhcpStart(void)                                            */
/*开发人和开发时间:                */
/*函数功能描述: 启动DHCP服务*/
/*函数原理说明:                                                    */
/*使用的全局变量、表和结构：                                       */
/*调用的主要函数列表：                                             */
/*返回值说明：-1,启动dhcp的时候发生错误。-2，已经启动dhcp     */
CHMID_TCPIP_RESULT_e 
CHMID_TCPIP_StartDHCPClient(void)
{
	if(b_DhcpStart == false)
	{
		Ipv4_SetAddresses(&smscNetworkInterface,
			IPV4_ADDRESS_MAKE(0,0,0,0),
			IPV4_ADDRESS_MAKE(0,0,0,0),
			IPV4_ADDRESS_MAKE(0,0,0,0));

		if(Dhcp_Start(&smscNetworkInterface,&smscDhcpData) < 0)
		{
			return CHMID_TCPIP_FAIL;  /*启动DHCP服务*/
		}
		else 
		{
			b_DhcpStart = true;
			return CHMID_TCPIP_OK;
		}
	}
	else
	{
		return CHMID_TCPIP_ALREADY_INIT;
	}

}
/************************函数说明***************************/
/*函数名:void CH_DhcpStop(void)                                            */
/*开发人和开发时间:                */
/*函数功能描述: 停止DHCP服务*/
/*函数原理说明:                                                    */
/*使用的全局变量、表和结构：                                       */
/*调用的主要函数列表：                                             */
/*返回值说明：无                  */
CHMID_TCPIP_RESULT_e 
CHMID_TCPIP_StopDHCPClient(void)
{
	if(b_DhcpStart)
	{
		Dhcp_Stop(&smscNetworkInterface);  
		b_DhcpStart = false;
	}
	return CHMID_TCPIP_OK;
}
/************************函数说明***************************/
/*函数名:U32 CH_GetIpAddress(void)                                          */
/*开发人和开发时间:                */
/*函数功能描述: 获取IP地址*/
/*函数原理说明:                                                    */
/*使用的全局变量、表和结构：                                       */
/*调用的主要函数列表：                                             */
/*返回值说明：无                  */
CHMID_TCPIP_RESULT_e 
CHMID_TCPIP_GetIP
(u32_t *rpui_IPAddr)
{

	struct NETWORK_INTERFACE *local_address;
	u32_t sourceAddress = 0;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(rpui_IPAddr != NULL,CHMID_TCPIP_INVALID_PARAM);
	
	local_address = /*Ipv4_GetRoute(0)*/&smscNetworkInterface;
	if(b_DhcpStart)
	{
		sourceAddress=local_address->mDhcpData->mOfferedAddress;
	}
	else
	{
		sourceAddress=local_address->mIpv4Data.mAddress;
	}
	*rpui_IPAddr = sourceAddress;
	return CHMID_TCPIP_OK;
}
/************************函数说明***************************/
/*函数名:U32 CH_GetIpMask(void)                                          */
/*开发人和开发时间:                */
/*函数功能描述: 获取字网地址*/
/*函数原理说明:                                                    */
/*使用的全局变量、表和结构：                                       */
/*调用的主要函数列表：                                             */
/*返回值说明：无                  */

CHMID_TCPIP_RESULT_e 
CHMID_TCPIP_GetNetMask
(u32_t *rpui_NetMask)
{

	struct NETWORK_INTERFACE *local_address;
	int sourceAddress;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(rpui_NetMask != NULL,CHMID_TCPIP_INVALID_PARAM);
	local_address = /*Ipv4_GetRoute(0)*/&smscNetworkInterface;
	if(b_DhcpStart)
	{
		sourceAddress=local_address->mDhcpData->mOfferedNetMask;
	}
	else
	{
		sourceAddress=local_address->mIpv4Data.mNetMask;
	}
	*rpui_NetMask = sourceAddress;
	return CHMID_TCPIP_OK;
}
/************************函数说明***************************/
/*函数名:U32 CH_GetGateway(void)                                          */
/*开发人和开发时间:                */
/*函数功能描述: 获取网关地址*/
/*函数原理说明:                                                    */
/*使用的全局变量、表和结构：                                       */
/*调用的主要函数列表：                                             */
/*返回值说明：无                  */

CHMID_TCPIP_RESULT_e 
CHMID_TCPIP_GetGateWay
(u32_t *rpui_Gateway)
{
	struct NETWORK_INTERFACE *local_address;
	int sourceAddress;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(rpui_Gateway != NULL,CHMID_TCPIP_INVALID_PARAM);

	local_address =/* Ipv4_GetRoute(0)*/&smscNetworkInterface;
	if(b_DhcpStart)
	{
		sourceAddress=local_address->mDhcpData->mOfferedGateway;
	}
	else
	{
		sourceAddress=local_address->mIpv4Data.mGateway;
	}
	*rpui_Gateway = sourceAddress;
	return CHMID_TCPIP_OK;
}
/************************函数说明***************************/
/*函数名:U32 CH_GetDNSNum(void)                                          */
/*开发人和开发时间:                */
/*函数功能描述: 获取DNS的组数*/
/*函数原理说明:                                                    */
/*使用的全局变量、表和结构：                                       */
/*调用的主要函数列表：                                             */
/*返回值说明：无                  */

CHMID_TCPIP_RESULT_e
CHMID_TCPIP_GetDNSServerNum
(s32_t *rpi_DNSServerNum)
{
	struct NETWORK_INTERFACE *local_netif;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(rpi_DNSServerNum != NULL,CHMID_TCPIP_INVALID_PARAM);
	
	local_netif = /* Ipv4_GetRoute(0)*/&smscNetworkInterface;
	if(b_DhcpStart)
		*rpi_DNSServerNum = local_netif->mDhcpData->mDnsCount;
	else
		*rpi_DNSServerNum = local_netif->mIpv4Data.mDnsNm;
	return CHMID_TCPIP_OK;
}

/************************函数说明***************************/
/*函数名:U32 CH_GetDNSByNum(void)                                          */
/*开发人和开发时间:                */
/*函数功能描述: 获取DNS地址*/
/*函数原理说明:                                                    */
/*使用的全局变量、表和结构：                                       */
/*调用的主要函数列表：                                             */
/*返回值说明：无                  */

CHMID_TCPIP_RESULT_e 
CHMID_TCPIP_GetDNSServer
(s32_t ri_DNSSequenceNum,u32_t *rpui_DnsServerIP)
{
	struct NETWORK_INTERFACE *local_address;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(rpui_DnsServerIP != NULL,CHMID_TCPIP_INVALID_PARAM);
	/*最多有两组DNS*/
	if(ri_DNSSequenceNum < 0 || ri_DNSSequenceNum >= 2)
		return CHMID_TCPIP_INVALID_PARAM;
	local_address = &smscNetworkInterface;
	if(b_DhcpStart){
		memcpy(rpui_DnsServerIP,&(local_address->mDhcpData->mOfferedDns[ri_DNSSequenceNum]),sizeof(u32_t));
	}else{
		memcpy(rpui_DnsServerIP,&(local_address->mIpv4Data.mManuDns[ri_DNSSequenceNum]),sizeof(u32_t));
	}
	return CHMID_TCPIP_OK;
}
/************************函数说明***************************/
/*函数名: CH_ManuSetDNSNum(U32 num)                                          */
/*开发人和开发时间:                */
/*函数功能描述: 获取DNS的组数*/
/*函数原理说明:                                                    */
/*使用的全局变量、表和结构：                                       */
/*调用的主要函数列表：                                             */
/*返回值说明：无                  */

CHMID_TCPIP_RESULT_e 
CHMID_TCPIP_SetDNSServerNum
(s32_t rui_DnsServerNum)
{
	struct NETWORK_INTERFACE *local_address;

	local_address = /* Ipv4_GetRoute(0)*/&smscNetworkInterface;
	local_address->mIpv4Data.mDnsNm= rui_DnsServerNum;
	return CHMID_TCPIP_OK;
}

/************************函数说明***************************/
/*函数名:U32 CH_ManuSetDNSByNum(void)                                          */
/*开发人和开发时间:                */
/*函数功能描述: 获取DNS地址*/
/*函数原理说明:                                                    */
/*使用的全局变量、表和结构：                                       */
/*调用的主要函数列表：                                             */
/*返回值说明：无                  */

CHMID_TCPIP_RESULT_e 
CHMID_TCPIP_SetDNSServer
(s32_t ri_DNSSequenceNum,u32_t rui_DNSServerAddr)
{
	struct NETWORK_INTERFACE *local_address;
	int sourceAddress;
	
	if(ri_DNSSequenceNum < 0 || ri_DNSSequenceNum >= 2)/*最多有两组DNS*/
		return CHMID_TCPIP_INVALID_PARAM;
	local_address = /* Ipv4_GetRoute(0)*/&smscNetworkInterface;
	sourceAddress=local_address->mIpv4Data.mManuDns[ri_DNSSequenceNum] = rui_DNSServerAddr;
	return CHMID_TCPIP_OK;
}

/************************函数说明***************************/
/*函数名:void CH_DhcpStart(void)                                            */
/*开发人和开发时间:                */
/*函数功能描述: 启动DHCP服务*/
/*函数原理说明:                                                    */
/*使用的全局变量、表和结构：                                       */
/*调用的主要函数列表：                                             */
/*返回值说明：无                  */
CHMID_TCPIP_RESULT_e 
CHMID_TCPIP_SetIP
(u8_t rui_IPAddr[4],
u8_t rui_NetMask[4],
u8_t rui_Gateway[4])
{
	CHMID_TCPIP_StopDHCPClient();

	Ipv4_SetAddresses(&smscNetworkInterface,
	IPV4_ADDRESS_MAKE(rui_IPAddr[0],rui_IPAddr[1],rui_IPAddr[2],rui_IPAddr[3]),
	IPV4_ADDRESS_MAKE(rui_NetMask[0],rui_NetMask[1],rui_NetMask[2],rui_NetMask[3]),
	IPV4_ADDRESS_MAKE(rui_Gateway[0],rui_Gateway[1],rui_Gateway[2],rui_Gateway[3]));
	NetworkInterface_SetUp(&smscNetworkInterface);
	Manual_ArpInit();
	return CHMID_TCPIP_OK;
}
/************************函数说明***************************/
/*函数名:CHMID_TCPIP_RESULT_e CHMID_TCPIP_Ping()*/
/*开发人和开发时间:                */
/*函数功能描述: 启动DHCP服务*/
/*函数原理说明:                                                    */
/*使用的全局变量、表和结构：                                       */
/*调用的主要函数列表：                                             */
/*返回值说明：无                  */
CHMID_TCPIP_RESULT_e 
CHMID_TCPIP_Ping
(CHMID_TCPIP_Pingcfg_t rch_Pingcfg,s32_t *ri_Timems)
{
	s32_t ret = -1;
	CHMID_TCPIP_RESULT_e resultRet = CHMID_TCPIP_FAIL;
	s32_t i_retry = rch_Pingcfg.i_Timeoutms*2;
	smsc_clock_t timeStart = 0;
	smsc_clock_t timeEnd = 0;
	smsc_clock_t timeUsedTick = 0;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(ri_Timems != NULL,CHMID_TCPIP_INVALID_PARAM);
	
	IcmpEchoTest_Initialize(IPV4_ADDRESS_MAKE(
		rch_Pingcfg.ui_IPAddr[0],
		rch_Pingcfg.ui_IPAddr[1],
		rch_Pingcfg.ui_IPAddr[2],
		rch_Pingcfg.ui_IPAddr[3]),
		rch_Pingcfg.i_Datalen,
		rch_Pingcfg.i_Timeoutms/100);
	timeStart = CHDRV_NET_TimeNow();
	do
	{
		ret = Icmpv4EchoReply();
		if(ret == 1)
		{
			timeEnd = CHDRV_NET_TimeNow();
			resultRet= CHMID_TCPIP_OK;
			break ;
		}
		else if(ret == -1)/*time out*/
		{
			timeEnd = CHDRV_NET_TimeNow();
			resultRet = CHMID_TCPIP_TIMEOUT;
			break ;
		}
		else
			CHDRV_NET_ThreadDelay(1);
		i_retry--;

	}while(i_retry);
	timeUsedTick = CHDRV_NET_TimeMinus(timeEnd, timeStart);
	*ri_Timems = timeUsedTick/(CHDRV_NET_TicksOfSecond()/1000);
	Tcpip_Memory_test();
	return resultRet ;
}
s32_t CHMID_TCPIP_Socket 
(s32_t ri_Domain,s32_t ri_Type,s32_t ri_Protocol)
{
	return smsc_socket(ri_Domain,ri_Type,ri_Protocol);
}
s32_t CHMID_TCPIP_Bind
(s32_t ri_SockFd,CHMID_TCPIP_SockAddr_t *rpstru_MyAddr,s32_t ri_AddrLen)
{
	struct sockaddr_in s_addr;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(rpstru_MyAddr != NULL,CHMID_TCPIP_INVALID_PARAM);
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(ri_SockFd>=0,CHMID_TCPIP_INVALID_PARAM);
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(ri_AddrLen>=0,CHMID_TCPIP_INVALID_PARAM);
	s_addr.sin_addr.s_addr = rpstru_MyAddr->ui_IP;
	s_addr.sin_family = rpstru_MyAddr->us_Family;
	s_addr.sin_port = rpstru_MyAddr->us_Port;
	s_addr.sin_len = rpstru_MyAddr->uc_Len;
	return smsc_bind(ri_SockFd, (struct sockaddr * )&s_addr,ri_AddrLen);
}
s32_t CHMID_TCPIP_Listen
(s32_t ri_SockFd,s32_t ri_BackLog)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(ri_SockFd>=0,CHMID_TCPIP_INVALID_PARAM);
	return smsc_listen(ri_SockFd,ri_BackLog);
}
s32_t CHMID_TCPIP_Accept
(s32_t ri_SockFd,CHMID_TCPIP_SockAddr_t *rpstru_ClientAddr,u32_t *rpi_AddrLen)
{
	struct sockaddr_in s_addr;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(rpstru_ClientAddr != NULL,CHMID_TCPIP_INVALID_PARAM);
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(ri_SockFd>=0,CHMID_TCPIP_INVALID_PARAM);
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(rpi_AddrLen!=NULL,CHMID_TCPIP_INVALID_PARAM);
	s_addr.sin_addr.s_addr = rpstru_ClientAddr->ui_IP;
	s_addr.sin_family = rpstru_ClientAddr->us_Family;
	s_addr.sin_port = rpstru_ClientAddr->us_Port;
	s_addr.sin_len = rpstru_ClientAddr->uc_Len;
	return smsc_accept(ri_SockFd, (struct sockaddr *)&s_addr, rpi_AddrLen);
}
s32_t CHMID_TCPIP_Connect
(s32_t ri_SockFd,CHMID_TCPIP_SockAddr_t *rpstru_ServerAddr,s32_t ri_AddrLen)
{
	struct sockaddr_in s_addr;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(rpstru_ServerAddr != NULL,CHMID_TCPIP_INVALID_PARAM);
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(ri_SockFd>=0,CHMID_TCPIP_INVALID_PARAM);
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(ri_AddrLen>0,CHMID_TCPIP_INVALID_PARAM);
	s_addr.sin_addr.s_addr = rpstru_ServerAddr->ui_IP;
	s_addr.sin_family = rpstru_ServerAddr->us_Family;
	s_addr.sin_port = rpstru_ServerAddr->us_Port;
	s_addr.sin_len = rpstru_ServerAddr->uc_Len;
	return smsc_connect(ri_SockFd, (struct sockaddr *)&s_addr, ri_AddrLen);
}
s32_t CHMID_TCPIP_Send
(s32_t ri_SockFd,void *rpuc_DataBuf,s32_t ri_DataLen,u32_t rui_Flags)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(rpuc_DataBuf != NULL,CHMID_TCPIP_INVALID_PARAM);
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(ri_SockFd>=0,CHMID_TCPIP_INVALID_PARAM);
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(ri_DataLen>0,CHMID_TCPIP_INVALID_PARAM);
	return smsc_send(ri_SockFd,rpuc_DataBuf,ri_DataLen,rui_Flags);
}
s32_t CHMID_TCPIP_Recv
(s32_t ri_SockFd,void *rpuc_DataBuf,s32_t ri_BufLen,u32_t rui_Flags)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(rpuc_DataBuf != NULL,CHMID_TCPIP_INVALID_PARAM);
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(ri_SockFd>=0,CHMID_TCPIP_INVALID_PARAM);
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(ri_BufLen>0,CHMID_TCPIP_INVALID_PARAM);
	return smsc_recv(ri_SockFd, rpuc_DataBuf,ri_BufLen, rui_Flags);
}
s32_t CHMID_TCPIP_SendTo
(s32_t ri_SockFd,	void *rpuc_DataBuf,s32_t ri_DataLen,u32_t rui_Flags,
CHMID_TCPIP_SockAddr_t *rpstru_DestAddr,s32_t ri_AddrLen)
{
	struct sockaddr_in s_addr;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(rpuc_DataBuf != NULL,CHMID_TCPIP_INVALID_PARAM);
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(ri_SockFd>=0,CHMID_TCPIP_INVALID_PARAM);
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(ri_DataLen>0,CHMID_TCPIP_INVALID_PARAM);
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(ri_AddrLen>0,CHMID_TCPIP_INVALID_PARAM);
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(rpstru_DestAddr != NULL,CHMID_TCPIP_INVALID_PARAM);
	s_addr.sin_addr.s_addr = rpstru_DestAddr->ui_IP;
	s_addr.sin_family = rpstru_DestAddr->us_Family;
	s_addr.sin_port = rpstru_DestAddr->us_Port;
	s_addr.sin_len = rpstru_DestAddr->uc_Len;
	return smsc_sendto(ri_SockFd,rpuc_DataBuf,ri_DataLen,rui_Flags, (struct sockaddr *)&s_addr,ri_AddrLen);
}
s32_t CHMID_TCPIP_RecvFrom
(s32_t ri_SockFd,	void *rpuc_DataBuf,s32_t ri_BufLen,u32_t rui_Flags,
CHMID_TCPIP_SockAddr_t *rpstru_SrcAddr,u32_t *rpi_AddrLen)
{
	struct sockaddr_in s_addr;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(rpuc_DataBuf != NULL,CHMID_TCPIP_INVALID_PARAM);
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(ri_SockFd>=0,CHMID_TCPIP_INVALID_PARAM);
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(ri_BufLen>0,CHMID_TCPIP_INVALID_PARAM);
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(rpi_AddrLen!=NULL,CHMID_TCPIP_INVALID_PARAM);
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(rpstru_SrcAddr != NULL,CHMID_TCPIP_INVALID_PARAM);
	s_addr.sin_addr.s_addr = rpstru_SrcAddr->ui_IP;
	s_addr.sin_family = rpstru_SrcAddr->us_Family;
	s_addr.sin_port = rpstru_SrcAddr->us_Port;
	s_addr.sin_len = rpstru_SrcAddr->uc_Len;
	return smsc_recvfrom(ri_SockFd,rpuc_DataBuf,ri_BufLen,rui_Flags, (struct sockaddr *)&s_addr,rpi_AddrLen);
}
s32_t CHMID_TCPIP_ShutDown
(s32_t ri_SockFd, s32_t ri_HowTo)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(ri_SockFd>=0,CHMID_TCPIP_INVALID_PARAM);
	return smsc_shutdown(ri_SockFd, ri_HowTo);
}
s32_t CHMID_TCPIP_Close(s32_t ri_SockFd)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(ri_SockFd>=0,CHMID_TCPIP_INVALID_PARAM);
	return smsc_close(ri_SockFd);
}
s32_t CHMID_TCPIP_GetPeerName
(s32_t  ri_SockFd,CHMID_TCPIP_SockAddr_t *rpstru_PeerAddr,u32_t *rpi_AddrLen)
{
	struct sockaddr_in s_addr;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(ri_SockFd>=0,CHMID_TCPIP_INVALID_PARAM);
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(rpstru_PeerAddr!= NULL,CHMID_TCPIP_INVALID_PARAM);
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(rpi_AddrLen != NULL,CHMID_TCPIP_INVALID_PARAM);
	s_addr.sin_addr.s_addr = rpstru_PeerAddr->ui_IP;
	s_addr.sin_family = rpstru_PeerAddr->us_Family;
	s_addr.sin_port = rpstru_PeerAddr->us_Port;
	s_addr.sin_len = rpstru_PeerAddr->uc_Len;
	return smsc_getpeername(ri_SockFd, (struct sockaddr *)&s_addr, rpi_AddrLen);
}
s32_t CHMID_TCPIP_GetSocketOpt
(s32_t ri_SockFd,s32_t i_Level,s32_t ri_OptName,
void *rpv_OptVal,u32_t *rpi_OptLen)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(ri_SockFd>=0,CHMID_TCPIP_INVALID_PARAM);
	return smsc_getsockopt(ri_SockFd,i_Level,ri_OptName,rpv_OptVal,rpi_OptLen);
}
s32_t CHMID_TCPIP_SetSocketOpt
(s32_t ri_SockFd,s32_t ri_Level,s32_t ri_OptName,
const void *rpv_OptVal,u32_t  ri_OptLen)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(ri_SockFd>=0,CHMID_TCPIP_INVALID_PARAM);
	return smsc_setsockopt(ri_SockFd,ri_Level,ri_OptName, rpv_OptVal,ri_OptLen);
}
s32_t CHMID_TCPIP_Fctrl
(s32_t ri_SockFd,s32_t ri_Command,void * ri_Arguement)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(ri_SockFd>=0,CHMID_TCPIP_INVALID_PARAM);
	return smsc_ioctl(ri_SockFd,ri_Command, ri_Arguement);
}
s32_t CHMID_TCPIP_Select
(s32_t ri_Maxfdnum,
CHMID_TCPIP_FDSet_t *rpstru_Readset,
CHMID_TCPIP_FDSet_t *rpstru_Writeset,
CHMID_TCPIP_FDSet_t  *rpstru_Exceptset,
s32_t  ri_Timeout)
{
	struct timeval timeout;
	
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(ri_Maxfdnum>0,CHMID_TCPIP_INVALID_PARAM);
	if(ri_Timeout >0)
	{
		timeout.tv_sec = ri_Timeout/1000;
		timeout.tv_usec = ri_Timeout;
		return smsc_select(ri_Maxfdnum,( fd_set *) rpstru_Readset, (fd_set *)rpstru_Writeset,( fd_set *)rpstru_Exceptset,&timeout);
	}
	else
	{
		return smsc_select(ri_Maxfdnum,( fd_set *) rpstru_Readset, (fd_set *)rpstru_Writeset,( fd_set *)rpstru_Exceptset,NULL);
	}
}

s32_t CHMID_TCPIP_GetSocketName
	(s32_t ri_SockFd, CHMID_TCPIP_SockAddr_t * rp_Name, u32_t * ruip_Namelen)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(ri_SockFd>=0,CHMID_TCPIP_INVALID_PARAM);
	return smsc_getsockname(ri_SockFd, (struct sockaddr *) rp_Name, (socklen_t *)ruip_Namelen);
}
s32_t CHMID_TCPIP_Read(s32_t ri_SockFd, void *rvp_Mem, s32_t ri_MemLen)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(ri_SockFd>=0,CHMID_TCPIP_INVALID_PARAM);
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(rvp_Mem!=NULL,CHMID_TCPIP_INVALID_PARAM);
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(ri_MemLen>0,CHMID_TCPIP_INVALID_PARAM);
	return smsc_read(ri_SockFd,rvp_Mem,ri_MemLen);
}
s32_t CHMID_TCPIP_Write(s32_t ri_SockFd, void *rvp_DataPtr, s32_t ri_Size)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(ri_SockFd>=0,CHMID_TCPIP_INVALID_PARAM);
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(rvp_DataPtr!=NULL,CHMID_TCPIP_INVALID_PARAM);
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(ri_Size>0,CHMID_TCPIP_INVALID_PARAM);
	return smsc_write(ri_SockFd,rvp_DataPtr,ri_Size);
}
s32_t CHMID_TCPIP_Ioctl(s32_t ri_SockFd, s32_t ri_Cmd, void *rvp_Argp)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(ri_SockFd>=0,CHMID_TCPIP_INVALID_PARAM);
	return smsc_ioctl(ri_SockFd,ri_Cmd,rvp_Argp);
}
/*-------------------------------------------------------------------
|CHMID_TCPIP_INET_ADDR:字符串格式IP转换成网络字节序格式				|
|eg:192.168.0.1 --> 0x100a8c0											|
--------------------------------------------------------------------*/
u32_t CHMID_TCPIP_InetAddr(const s8_t * rc_Addr)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(rc_Addr !=NULL,CHMID_TCPIP_INVALID_PARAM);
	return inet_addr((char *)rc_Addr);
}
s32_t CHMID_TCPIP_InetAton(const s8_t * rc_Addr,u32_t * rui_AddrOut)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(rc_Addr !=NULL,CHMID_TCPIP_INVALID_PARAM);
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(rui_AddrOut !=NULL,CHMID_TCPIP_INVALID_PARAM);
	return inet_aton((char *)rc_Addr,(struct in_addr *)rui_AddrOut);
}
/*-------------------------------------------------------------------
|CHMID_TCPIP_INET_NTOA:字符串格式IP转换成网络字节序格式				|
|eg:0x100a8c0--> 192.168.0.1 											|
--------------------------------------------------------------------*/
s8_t * CHMID_TCPIP_InetNtoa(u32_t rui_AddrIn)
{
	struct in_addr addr;
	addr.s_addr = rui_AddrIn;
	return (s8_t *)inet_ntoa(addr);
}
s32_t CHMID_TCPIP_GetTCPStatus(s32_t ri_Socfd)
{
	return CH_GetTCPStatus(ri_Socfd);
}
/*------------------------------EOF----------------------------*/
