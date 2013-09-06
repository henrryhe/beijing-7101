/*******************************************************************************
* 文件名称 : tcpip.c											
* 功能描述 :	提供网络模块初始化,出厂设置,更新IP等一些接口函数的描述与实现.这些
* 接口函数可供上层应用进行调用			
*
* 作者:flex
* 修改历史:
* 日期					作者						描述
* 2009/3/30				 flex							create
*
********************************************************************************/
#include "tcpip.h"

/*----------------------------------------------------------------------*/
static boolean bProcessSetting = false;
/*网络是否可用标志,
小于等于0为不可用，大于0为可用,-1为CM下线*/
static u16_t gNetStatus = 0;
static CHDRV_NET_Config_t gNetConfigInfo;
 SMSC_SEMAPHORE psemSendToNETSetting =NULL;

#define GET_IP_WORD(x,i) (x)>>(8*(3-i))&0xffU


extern int CH_InitSMSCTCPIP(u32_t ip, u32_t netmask, u32_t gateway, u8_t mac [ 6 ]);
extern int CHMID_TCPIP_GetLINKStatus(void);
extern int CHMID_TCPIP_SetIP(u8_t rui_IPAddr [ 4 ], u8_t rui_NetMask [ 4 ], u8_t rui_Gateway [ 4 ]);
extern int CHMID_TCPIP_SetOption(CHMID_TCPIP_OPTION_e setType, s8_t * sName);
extern int CHMID_TCPIP_StartDHCPClient(void);
extern int CHMID_TCPIP_StopDHCPClient(void);
extern int CHMID_TCPIP_SetIP(u8_t rui_IPAddr [ 4 ], u8_t rui_NetMask [ 4 ], u8_t rui_Gateway [ 4 ]);
extern int CHMID_TCPIP_SetDNSServerNum(s32_t rui_DnsServerNum);
extern int CHMID_TCPIP_SetDNSServer(s32_t ri_DNSSequenceNum, u32_t rui_DNSServerAddr);


/*--------------------------------------------------------------------------*/
/* 函数名称 : CHMID_TCPIP_FactorySet											*/
/* 功能描述 :网络部分出厂设置		*/
/* 入口参数 : 无						*/
/* 出口参数 : 无																*/
/* 返回值:	无					*/
/*--------------------------------------------------------------------------*/
void CHMID_TCPIP_FactorySet(void)
{
	u32_t addr[6], i;
	CHDRV_NET_RESULT_e error =CHDRV_NET_FAIL;


	gNetConfigInfo.ipmodel=/*MANUAL*/AUTO_DHCP; /*0==dhcp 1==manual*/
	gNetConfigInfo.dhcpport=RJ45_PORT_MODEL;
	/* Initialize the ethernet interface*/	
	memset(addr,0xFF,6);
	error= CHDRV_NET_LoadNetConfig(&gNetConfigInfo);
	if (error==CHDRV_NET_OK)
	{
		SMSC_NOTICE(TCPIP_DEBUG,("Read MAC Successed!MAC=[%02x:%02x:%02x:%02x:%02x:%02x]",
		gNetConfigInfo.macaddress[0],
		gNetConfigInfo.macaddress[1],
		gNetConfigInfo.macaddress[2],
		gNetConfigInfo.macaddress[3],
		gNetConfigInfo.macaddress[4],
		gNetConfigInfo.macaddress[5]));
	}
	else
	{
		SMSC_WARNING(TCPIP_DEBUG,("Read MAC Failed!Use Default MAC=[%s]!",IPDRIVER_MAC_ADDRESS));
		sscanf(IPDRIVER_MAC_ADDRESS,"%02x:%02x:%02x:%02x:%02x:%02x", \
				&addr[0],\
				&addr[1],\
				&addr[2],\
				&addr[3],\
				&addr[4],\
				&addr[5]);
		memcpy(gNetConfigInfo.macaddress,addr,MAC_ADDRESS_LENGTH);
	}
#if 1
	sscanf(IPDRIVER_IP_ADDRESS, "%d.%d.%d.%d",
		&addr[0],
		&addr[1],
		&addr[2],
		&addr[3]);
	for (i = 0; i < 4; i++)
		gNetConfigInfo.ipaddress[i] = addr[i];
	
	sscanf(IPDRIVER_NETMASK, "%d.%d.%d.%d",
		&addr[0],
		&addr[1],
		&addr[2],
		&addr[3]);
	for (i = 0; i < 4; i++)
		gNetConfigInfo.netmask[i] = addr[i];

	sscanf(IPDRIVER_GATEWAY, "%d.%d.%d.%d",
		&addr[0],
		&addr[1],
		&addr[2],
		&addr[3]);
	for (i = 0; i < 4; i++)
		gNetConfigInfo.gateway[i] = addr[i];

	sscanf(IPDRIVER_DNS, "%d.%d.%d.%d",
		&addr[0],
		&addr[1],
		&addr[2],
		&addr[3]);
	for (i = 0; i < 4; i++)
		gNetConfigInfo.dns[i] = addr[i];
	

	/*pstBoxInfo->proxy.proxytype=0;NO PROXY*/
#else
	memset(gNetConfigInfo.ipaddress,0,IP_STRING_LENGTH);
	memset(gNetConfigInfo.netmask,0,IP_STRING_LENGTH);
	memset(gNetConfigInfo.gateway,0,IP_STRING_LENGTH);
	memset(gNetConfigInfo.dns,0,IP_STRING_LENGTH);
#endif
	CHDRV_NET_SaveNetConfig(&gNetConfigInfo);
	/*重新获取IP*/
	bProcessSetting = true;
	gNetStatus = 0;
	smsc_semaphore_signal(psemSendToNETSetting);
}
/*--------------------------------------------------------------------------*/
/* 函数名称 : CHMID_TCPIP_Init												*/
/* 功能描述 :网络初始化，包括硬件复位，驱动初始化，协议栈初始化，开始获取IP。	*/
/* 入口参数 : 无						*/
/* 出口参数 : 无																*/
/* 返回值:	1，成功，否则失败					*/
/*--------------------------------------------------------------------------*/
CHMID_TCPIP_RESULT_e CHMID_TCPIP_Init(void)
{
	s32_t rt = 0;
	u32_t addr[6], i;
	u32_t ipaddr = 0,netmask = 0,gw = 0;
	CHDRV_NET_RESULT_e error =CHDRV_NET_FAIL;
	CHMID_TCPIP_Version_t version;
	
	memset(&gNetConfigInfo,0,sizeof(CHDRV_NET_Config_t));
	memset(gNetConfigInfo.macaddress,0xFF,MAC_ADDRESS_LENGTH);
	memset(addr,0xFF,6);
	error= CHDRV_NET_LoadNetConfig(&gNetConfigInfo);
	if (error==CHDRV_NET_OK)
	{
		SMSC_NOTICE(TCPIP_DEBUG,("Read MAC Successed!MAC=[%02x:%02x:%02x:%02x:%02x:%02x]\n",
		gNetConfigInfo.macaddress[0],
		gNetConfigInfo.macaddress[1],
		gNetConfigInfo.macaddress[2],
		gNetConfigInfo.macaddress[3],
		gNetConfigInfo.macaddress[4],
		gNetConfigInfo.macaddress[5]));

		addr[0]=gNetConfigInfo.macaddress[0];
		addr[1]=gNetConfigInfo.macaddress[1];
		addr[2]=gNetConfigInfo.macaddress[2];
		addr[3]=gNetConfigInfo.macaddress[3];
		addr[4]=gNetConfigInfo.macaddress[4];
		addr[5]=gNetConfigInfo.macaddress[5];
	}
	else
	{
		SMSC_WARNING(TCPIP_DEBUG,("\nRead MAC Failed!Use Default MAC=[%s]!\n",IPDRIVER_MAC_ADDRESS));
		sscanf(IPDRIVER_MAC_ADDRESS,"%x:%x:%x:%x:%x:%x", \
				&addr[0],\
				&addr[1],\
				&addr[2],\
				&addr[3],\
				&addr[4],\
				&addr[5]);
	}

	for (i = 0; i < 6; i++)
		gNetConfigInfo.macaddress[i] = (u8_t)addr[i];

	if(gNetConfigInfo.ipmodel == MANUAL)
	{
		for(i = 0; i < 4;i++)
			ipaddr += (gNetConfigInfo.ipaddress[i])<<(8*(3-i));
		for(i = 0; i < 4;i++)
			netmask += (gNetConfigInfo.netmask[i])<<(8*(3-i));
		for(i = 0; i < 4;i++)
			gw += (gNetConfigInfo.gateway[i])<<(8*(3-i));
		rt = CH_InitSMSCTCPIP(ipaddr,netmask,gw,gNetConfigInfo.macaddress);
		if(rt != 0)
		{
			SMSC_ERROR(("ERROR:init netif failed!"));
			return rt;
		}
	}
	else
	{

		rt = CH_InitSMSCTCPIP(0x0U,0x0U,0x0U,gNetConfigInfo.macaddress);
		if(rt != 0)
		{
			SMSC_ERROR(("ERROR:init netif failed!"));
			return rt;
		}
	}
	/*Create the net update task*/
	CH_InitNetSettingSemaphoreAndTask();
	CHMID_TCPIP_GetVersion(&version);
	CHDRV_NET_Print("*******************************************\r\n");
	CHDRV_NET_Print("            CHMID_TCPIP INIT OK!\r\n");
	CHDRV_NET_Print("            (v%d.%d.%d.%d)\r\n",version.i_InterfaceMainVer,version.i_InterfaceSubVer,version.i_ModuleMainVer,version.i_ModuleSubVer);
	CHDRV_NET_Print("            Built time: %s,%s\r\n",__DATE__,__TIME__);
	CHDRV_NET_Print("*******************************************\r\n");
	return 0;
	
}



/*--------------------------------------------------------------------------*/
/* 函数名称 : CHMID_TCPIP_RenewIP												*/
/* 功能描述 : 向网络模块发起更新IP请求	*/
/* 入口参数 : NetConfig,网络模块更新IP所遵循的规则,配置信息						*/
/* 出口参数 : 无																*/
/* 返回值:	无					*/
/*--------------------------------------------------------------------------*/
CHMID_TCPIP_RESULT_e CHMID_TCPIP_NetConfigRenew(CHDRV_NET_Config_t NetConfig)
{
	s32_t timeLoop = 100;
	memcpy(&gNetConfigInfo,&NetConfig,sizeof(CHDRV_NET_Config_t));
	do{
		if(CHMID_TCPIP_GetNetStatus()&CHMID_TCPIP_RENEWING_IP){
			CHDRV_NET_ThreadDelay(100);
		}else{
			break;
		}
	}while(timeLoop--);	
	bProcessSetting = true;
	gNetStatus = 0;
	smsc_semaphore_signal(psemSendToNETSetting);
	timeLoop = 100;
	do{
		if(CHMID_TCPIP_GetNetStatus()&CHMID_TCPIP_RENEWING_IP){
			CHDRV_NET_ThreadDelay(100);
		}else{
			break;
		}
	}while(timeLoop--);

	return CHMID_TCPIP_OK;
}
/*--------------------------------------------------------------------------*/
/* 函数名称 : CHMID_TCPIP_GetNetConfig								*/
/* 功能描述 : 获取当前网络配置信息			*/
/* 入口参数 :  无								*/
/* 出口参数 :	NetConfig,网络配置信息							*/
/* 返回值说明：成功返回CHDRV_NET_OK*/
/* 注意:				*/
/*--------------------------------------------------------------------------*/
CHMID_TCPIP_RESULT_e CHMID_TCPIP_GetNetConfig(CHDRV_NET_Config_t * pNetConfig)
{
	if(pNetConfig == NULL)
	{
		return CHMID_TCPIP_INVALID_PARAM;
	}
	memcpy(pNetConfig,&gNetConfigInfo,sizeof(CHDRV_NET_Config_t));
	return CHMID_TCPIP_OK;
}
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
u16_t CHMID_TCPIP_GetNetStatus(void)
{
	u16_t us_Ret = CHMID_TCPIP_UNUSABLE;

	if(bProcessSetting)
		us_Ret |= CHMID_TCPIP_RENEWING_IP;
	if(gNetStatus)
		us_Ret |= CHMID_TCPIP_USABLE;
	if(CHMID_LINK_UP == CHMID_TCPIP_GetLINKStatus())
		us_Ret |= CHMID_TCPIP_LINKUP;
	return us_Ret;
}
/*--------------------------------------------------------------------------*/
/* 函数名称 : CH_tcp_test														*/
/* 功能描述 : 测试目标IP地址是否可到达。可用来测试机顶盒所在网络是否通畅		*/
/* 入口参数 : <ipaddr>[in]需要做ping测试的IP地址(形如:x.x.x.x)							*/
/* 出口参数 : 无																*/
/* 返回值:	0，网络通畅；否则返回1												*/
/*--------------------------------------------------------------------------*/
CHMID_TCPIP_RESULT_e CHMID_TCPIP_FactoryTest
(u8_t ruc_SrcIp[4],
u8_t ruc_SrcNetmask[4],
u8_t ruc_SrcGw[4],
u8_t ruc_SrcDns[4],
u8_t ruc_DstIp[4])
{
	s32_t iRepeat = 100;
	u32_t ttl = 64,datalen = 60 ,time_out = 3000,timeused = 0;
	u32_t iIPaddr = 0;
	CHMID_TCPIP_RESULT_e ires = CHMID_TCPIP_FAIL;
	CHDRV_NET_Config_t NetConfig,NetConfigtmp;
	CHMID_TCPIP_Pingcfg_t pingTemp ;

	memcpy(&NetConfigtmp,&gNetConfigInfo,sizeof(CHDRV_NET_Config_t));
	memcpy(&(pingTemp.ui_IPAddr),&ruc_DstIp[0],4);
	pingTemp.i_Datalen = datalen;
	pingTemp.i_Timeoutms = time_out;
	pingTemp.i_TTL = ttl;
	/*Manual set IP*/
	memcpy(&NetConfig,&gNetConfigInfo,sizeof(CHDRV_NET_Config_t));
	memcpy(&(NetConfig.ipaddress),&ruc_SrcIp[0],4);
	memcpy(&(NetConfig.netmask),&ruc_SrcNetmask[0],4);
	memcpy(&(NetConfig.gateway),&ruc_SrcGw[0],4);
	NetConfig.ipmodel = MANUAL;
	CHMID_TCPIP_NetConfigRenew(NetConfig);

	while(iRepeat--)
	{
		if(CHMID_TCPIP_GetNetStatus()&CHMID_TCPIP_RENEWING_IP)
		{
			CHDRV_NET_ThreadDelay(100);
			continue;
		}
		CHMID_TCPIP_GetIP(&iIPaddr);
		if((NetConfig.ipaddress[0]== (iIPaddr)&0xff)&&
		(NetConfig.ipaddress[0] == (iIPaddr>>8)&0xff)&&
		(NetConfig.ipaddress[0] == (iIPaddr>>16)&0xff)&&
		(NetConfig.ipaddress[0] == (iIPaddr>>24)&0xff))
		{
			break;
		}
		else
		{
			CHMID_TCPIP_NetConfigRenew(NetConfig);
		}
	}
	
	if(iRepeat == 0)
	{
		SMSC_ERROR(("Manul set ip failed"));
		ires =  CHMID_TCPIP_FAIL;
		goto iExit;
	}
	/*test RJ45*/
	for(iRepeat= 0;iRepeat < 4; iRepeat++)
	{
		ires = CHMID_TCPIP_Ping(pingTemp,(s32_t *)&timeused);
		if(ires == CHMID_TCPIP_OK)
			break;
	}
iExit:
	memcpy(&gNetConfigInfo,&NetConfigtmp,sizeof(CHDRV_NET_Config_t));	
	CHMID_TCPIP_NetConfigRenew(NetConfigtmp);
	return ires;
}
/*--------------------------------------------------------------------------*/
/* 函数名称 : CH_UpdateNetSetting												*/
/* 功能描述 :重新获取IP	*/
/* 入口参数 : 无						*/
/* 出口参数 :	无															*/
/* 返回值:	无				*/
/*--------------------------------------------------------------------------*/
static void CH_UpdateNetSetting(void)
{
	s32_t loop = 0;
	s32_t err_no = 0;
	CHMID_TCPIP_RESULT_e rt = CHMID_TCPIP_OK;
	s32_t i,j;
	static boolean bDHCP_inited = 0,bDHCP_started = 0;
	u8_t cIPaddr[4];
	u32_t iIPaddr = 0,netmask = 0,gw = 0,dns_num = 0,dns = 0;
	if(gNetConfigInfo.ipmodel == AUTO_DHCP)/*DHCP*/
	{
		memset(&gNetConfigInfo.ipaddress,0,4);
		memset(&gNetConfigInfo.netmask,0,4);
		memset(&gNetConfigInfo.gateway,0,4);
		memset(&gNetConfigInfo.dns,0,4);
		CHMID_TCPIP_SetIP(gNetConfigInfo.ipaddress,
			gNetConfigInfo.netmask,
			gNetConfigInfo.gateway);
		
		if(!bDHCP_inited)
		{
			Dhcp_Initialize();
			SMSC_TRACE(TCPIP_DEBUG,("OK: init dhcpc ok!"));
			bDHCP_inited = true;
		}
		if(bDHCP_started)
		{
			CHMID_TCPIP_StopDHCPClient();		
			bDHCP_started = false;
		}
		if(!bDHCP_started)
		{
#if 1/*添加option60*/
			CHMID_TCPIP_SetOption(DHCP_SET_OPTION_60_CLIENT, gNetConfigInfo.Opt60ClientName);
			CHMID_TCPIP_SetOption(DHCP_SET_OPTION_60_SERVER, gNetConfigInfo.Opt60ServerName);
#endif
			rt = CHMID_TCPIP_StartDHCPClient();
			if(rt != CHMID_TCPIP_OK)
			{
				SMSC_WARNING(TCPIP_DEBUG,("ERROR:start dhcpc failed!"));
				goto iEXIT;
			}
			bDHCP_started = true;
		}
		do/*查询是否已经获取IP*/
		{
			err_no = 0;
			/*get ip success*/
			rt = CHMID_TCPIP_GetIP(&iIPaddr);/*Update IP*/
			if((rt == CHMID_TCPIP_OK)&&(iIPaddr!=0))
			{
				err_no += 0;
			}
			else
			{
				memset(&(gNetConfigInfo.ipaddress[0]),0,4);
				SMSC_NOTICE(TCPIP_DEBUG,("NO IP ,need renew !"));
				err_no += 1;
			}
			rt = CHMID_TCPIP_GetNetMask(&netmask);/*Update netmask*/
			if((rt == CHMID_TCPIP_OK)&&(netmask!=0))
			{
				err_no += 0;
			}
			else
			{
				memset(&(gNetConfigInfo.netmask[0]),0,4);
				//SMSC_NOTICE(TCPIP_DEBUG,("NO NET MASK ,need renew !"));
				err_no += 1;
			}
			rt = CHMID_TCPIP_GetGateWay(&gw);/*Update gateway*/
			if((rt == CHMID_TCPIP_OK)&&(gw!=0))
			{
				err_no += 0;
			}
			else
			{
				memset(&(gNetConfigInfo.gateway[0]),0,4);
				//SMSC_NOTICE(TCPIP_DEBUG,("NO GATEWAY ,need renew !"));
				/*根据需要屏蔽下行,有些局域网中没有网关*/
				/*err_no += 1;*/
			}
			/*get DNS server number*/
			rt = CHMID_TCPIP_GetDNSServerNum((s32_t *)&dns_num);
			if((rt == CHMID_TCPIP_OK)&&(dns_num!=0))
			{
				err_no += 0;
			}
			else
			{
				memset(&(gNetConfigInfo.dns[0]),0,4);
				/*由于江苏项目中必须访问域名，
				因此在获取IP的时候DNS必须获取成功，
				否则重新获取IP */
				//SMSC_NOTICE(TCPIP_DEBUG,("NO DNS ,need renew !"));
				/*err_no += 1;*/
			}
			
			if(err_no == 0)
			{
#if 0
				/*如果收到192.168.100.n，而且是在cm端口获取的，则丢弃后重收，
				避免用户因收到100.2的ip后无法登陆  sqzow080703*/
				if(gNetConfigInfo.dhcpport == CM_PORT_MODEL
					&& iIPaddr & 0xffffff00L == 0xc0a86400L)
				{
					goto iEXIT;
				}
				else
#endif
				{
					break;
				}
			}
			
			loop++;
			CHDRV_NET_ThreadDelay(2000);
		}while(loop < 3 );
	}
	else/*MANUAL set ip*/
	{
		err_no = 0;
		for(i = 0;i < 4;i++)
		{
			iIPaddr += ((u32_t)gNetConfigInfo.ipaddress[i])<<(8*i);/*主机序*/
			netmask += ((u32_t)gNetConfigInfo.netmask[i])<<(8*i);
			gw += ((u32_t)gNetConfigInfo.gateway[i])<<(8*i);
		}
		if(bDHCP_started)
		{
			CHMID_TCPIP_StopDHCPClient();
		}

		CHMID_TCPIP_SetIP(gNetConfigInfo.ipaddress,
							gNetConfigInfo.netmask,
							gNetConfigInfo.gateway);
		if(gNetConfigInfo.dns[0] != 0)
		{
			dns = 0x0U;
			dns_num = 1;
			CHMID_TCPIP_SetDNSServerNum(1);/*Support ONLY one DNS server now*/
			for(i = 0;i < 4;i++)
				dns += ((u32_t)gNetConfigInfo.dns[i])<<(8*i);
			CHMID_TCPIP_SetDNSServer(0, dns);
		}
	}
	/*check*/
	if(err_no != 0 && AUTO_DHCP == gNetConfigInfo.ipmodel)
	{	
		SMSC_TRACE(TCPIP_DEBUG,("DHCP IS GETING IP..."));/**/
		gNetStatus = 0;
	}
	else if(err_no != 0 && MANUAL == gNetConfigInfo.ipmodel)
	{
		SMSC_WARNING(TCPIP_DEBUG,("ERROR : MANUAL get ip faild!"));
		gNetStatus = 0;
	}
	else if(0 == err_no)
	{
		/*show ip*/
		for(i = 0;i < 4;i++)
		{
			cIPaddr[i] = GET_IP_WORD(iIPaddr,i);
			gNetConfigInfo.ipaddress[3-i] = cIPaddr[i];
		}
		SMSC_NOTICE(TCPIP_DEBUG,("NEW IP:%d.%d.%d.%d",
					cIPaddr[3],\
					cIPaddr[2],\
					cIPaddr[1],\
					cIPaddr[0]));
		/*show net mask*/
		for(i = 0;i < 4;i++)
		{
			cIPaddr[i] = GET_IP_WORD(netmask,i);
			gNetConfigInfo.netmask[3-i] = cIPaddr[i];
		}
		SMSC_NOTICE(TCPIP_DEBUG,("NEW NETMASK:%d.%d.%d.%d",\
					cIPaddr[3],\
					cIPaddr[2],\
					cIPaddr[1],\
					cIPaddr[0]));
		/*show gateway*/
		for(i = 0;i < 4;i++)
		{
			cIPaddr[i] = GET_IP_WORD(gw,i);
			gNetConfigInfo.gateway[3-i] = cIPaddr[i];
		}
		SMSC_NOTICE(TCPIP_DEBUG,("NEW GATEWAY:%d.%d.%d.%d",
					cIPaddr[3],\
					cIPaddr[2],\
					cIPaddr[1],\
					cIPaddr[0]));
		/*show dnses..*/
		if(dns_num > 0)
		{
			if(gNetConfigInfo.ipmodel == AUTO_DHCP)
			{
				for(j = 0;j < dns_num;j++)
				{
					rt = CHMID_TCPIP_GetDNSServer(j,&dns);
					if((rt == CHMID_TCPIP_OK)&&(dns!=0))
					{
						for(i = 0;i < 4;i++)
						{
							cIPaddr[i] = GET_IP_WORD(dns,i);
							gNetConfigInfo.dns[3-i] = cIPaddr[i];
						}
						SMSC_NOTICE(TCPIP_DEBUG,("NEW DNS[%d]:%d.%d.%d.%d",
									j+1,
									cIPaddr[3],\
									cIPaddr[2],\
									cIPaddr[1],\
									cIPaddr[0]));
					}
					else
					{
						SMSC_NOTICE(TCPIP_DEBUG,("WARING:get DNS server[%d] failed!",j+1));
					}
				}
			}
			else
			{
				SMSC_NOTICE(TCPIP_DEBUG,("NEW DNS[1]:%d.%d.%d.%d",	\
							gNetConfigInfo.dns[0] ,\
							gNetConfigInfo.dns[1] ,\
							gNetConfigInfo.dns[2] ,\
							gNetConfigInfo.dns[3] ));
			}
		}
		else
		{
			SMSC_NOTICE(TCPIP_DEBUG,("WARING:NO dns!"));
		}
		
		/*save*/
		CHDRV_NET_SaveNetConfig(&gNetConfigInfo);
		if(AUTO_DHCP == gNetConfigInfo.ipmodel)
		{
			SMSC_NOTICE(TCPIP_DEBUG,("SUCCESS: DHCP set  ip success!"));
		}
		else
		{
			SMSC_NOTICE(TCPIP_DEBUG,("SUCCESS: MANUAL set  ip success!"));
		}
		gNetStatus = 1;
	}
iEXIT:
	bProcessSetting = false;
}
/*--------------------------------------------------------------------------*/
/* 函数名称 : CH_NETSettingTask												*/
/* 功能描述 : 网络设置任务,更新网络IP.可以在此任务中添加监控iploader升级检测	*/
/* 入口参数 :保留					*/
/* 出口参数 : 无																*/
/* 返回值:	无				*/
/*--------------------------------------------------------------------------*/
static void CH_NETSettingTask( void *pvParam)
{
	
	bProcessSetting = true;
	CHDRV_NET_ThreadDelay(1000);
	gNetConfigInfo.ipmodel =AUTO_DHCP;	
	while(1)
	{	
		if(MANUAL == gNetConfigInfo.ipmodel && !(gNetStatus) )
		{
			 CH_UpdateNetSetting();
		}
		else if(AUTO_DHCP == gNetConfigInfo.ipmodel && !(gNetStatus) )
		{
			 CH_UpdateNetSetting();
		}
#if 0
		/*如果未获取到ip，则重新获取*/
		if(!(gNetStatus))
		{
			CHDRV_NET_ThreadDelay(1000);
			continue;
		}
#endif
		/*等待更新IP信号*/
		if(smsc_semaphore_wait_timeout( psemSendToNETSetting, 3000) == 0)
		{
			continue;
		}
	}
}
/*--------------------------------------------------------------------------*/
/* 函数名称 : CH_InitNetSettingSemaphoreAndTask									*/
/* 功能描述 : 初始化网络IP更新任务					*/
/* 入口参数 : 无						*/
/* 出口参数 : 无																*/
/* 返回值:	无				*/
/*--------------------------------------------------------------------------*/
static void CH_InitNetSettingSemaphoreAndTask(void)
{
	
	psemSendToNETSetting = smsc_semaphore_create(0);

	if ( smsc_thread_create( CH_NETSettingTask,
					NULL,
					4096/**10*/,
					6,
					(s8_t *)"CH_NETSettingTask",
					0 ) == NULL )
  	{
  		SMSC_ERROR(("Failed to start netsetting task" ));
  	}
}
/*EOF*/
