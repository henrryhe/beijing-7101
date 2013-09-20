/*
  * ===================================================================================
  * CopyRight By CHANGHONG NET L.T.D.
  * 文件: 	eis_api_ca.c
  * 描述: 	实现CA 相关的接口
  * 作者:	 刘战芬，蒋庆洲
  * 时间:	2008-10-23
  * ===================================================================================
  */

#include "eis_api_define.h"
#include "eis_api_globe.h"
#include "eis_api_debug.h"
#include "eis_api_msg.h"
#include "..\key\keymap.h"
#include "eis_include\ipanel_porting_event.h"

#include "eis_include\ipanel_demux.h"
#include "eis_api_demux.h"
#include "..\main\initterm.h"
#include "channelbase.h"
#include "eis_include\ipanel_vdec.h"

#include "chmid_tcpip_api.h"
#include "chmid_cm_api.h"
#ifdef EIS_TEST
#include "..\dbase\vdbase.h"
#endif
//#define XML_TEST
/*add for the ipanel keymap*/

#define ch_KEY_A           140/*8c*/
#define ch_KEY_B           141
#define ch_KEY_C           142
#define ch_KEY_D           143
#define ch_KEY_E            144
#define ch_KEY_F           145
#define ch_KEY_DIGIT0     0		/* converted K0_KEY to K9_KEY */
#define ch_KEY_DIGIT1     1
#define ch_KEY_DIGIT2     2
#define ch_KEY_DIGIT3     3
#define ch_KEY_DIGIT4     4
#define ch_KEY_DIGIT5     5
#define ch_KEY_DIGIT6     6
#define ch_KEY_DIGIT7     7
#define ch_KEY_DIGIT8     8
#define ch_KEY_DIGIT9     9
#define ch_RED_KEY							109/*6d*/
#define ch_GREEN_KEY						110
#define ch_YELLOW_KEY						111
#define ch_BLUE_KEY							112
#define ch_VOLUME_UP_KEY				16
#define ch_VOLUME_DOWN_KEY 				17
#define ch_P_UP_KEY 				33
#define ch_P_DOWN_KEY 				32
#define ch_RETURN_KEY			   131
#define ch_UP_ARROW_KEY				88
#define ch_DOWN_ARROW_KEY				89
#define ch_LEFT_ARROW_KEY				90	
#define ch_RIGHT_ARROW_KEY			    	91	
#define ch_OK_KEY							92	
#define ch_EXIT_KEY						10 
#define ch_HELP_KEY							15
#define ch_MUTE_KEY						13
#define ch_EPG_KEY							204
#define ch_MENU_KEY						84
#define ch_HOMEPAGE_KEY					84	
#define ch_POWER_KEY						12
#define ch_LIST_KEY 			       245
#define ch_homepage_key                     0x82
#define ch_dianbo_key                   0x8d
#define ch_jiaohu_key                    0x8e
#define ch_xinhao_key                    0x9f
#define ch_jinhao_key                    0xa0
#define ch_luzhi_key                       0x37
#define ch_tingzhi_key                    0x31
#define ch_radio_key                       0x4e
#define ch_stock_key                      0x93
#define ch_nvod_key                       0x92
#define	UP_ARROW_FRONTKEY				0xf01
#define	DOWN_ARROW_FRONTKEY				0xf02
#define	LEFT_ARROW_FRONTKEY				0xf03	
#define	RIGHT_ARROW_FRONTKEY			0xf04
#define	MENU_FRONTKEY						0xf05
#define	OK_FRONTKEY						0xf06

#ifdef __EIS_API_DEBUG_ENABLE__
#define __EIS_API_MAIN_DEBUG__
#endif
#define DHCP_OUTTIME 16000 /* dhcp 超时时间单位为毫秒*/
#define CM_FREQ_SIZE 30 /* 获得CM频率字符串最大长度*/

static boolean send_NetFailedStatus=false; /*本次DHCP 是否已经发送网络连接失败消息*/
static UINT32_T ip_addr = 0;
static UINT32_T ip_DHCP_satrt_time = 0;
static U8 MAC_CM[10]={0,0,0,0,0,0,0,0,0,0};
static U8 DownFreq_CMSet[CM_FREQ_SIZE]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
static U8 DownFreq_CMGet[CM_FREQ_SIZE]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
static U8 UpFreq_CMGet[CM_FREQ_SIZE]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

static boolean get_MAC_CM=false;
static boolean set_DownFreq_CM=false;
static semaphore_t *sem_eis_CM= NULL;	/* 用于time显示启动 */
boolean EIS_DHCP_set=false;

 void eis_init_ipaddr( void )
{
	ip_addr=0;
	send_NetFailedStatus=false; 
}

void eis_set_CMDownFreq( int freq )
{
	 semaphore_wait(sem_eis_CM) ;
	memset(DownFreq_CMSet,0,CM_FREQ_SIZE);
	sprintf(DownFreq_CMSet,"%d00",freq);	
	set_DownFreq_CM=true;
	eis_report("\n >>>>>>>>>>>>eis_set_CMDownFreq freq=%d ",freq);
	semaphore_signal(sem_eis_CM) ;
}
void eis_get_CMDownFreq( char *freq)
{
	 semaphore_wait(sem_eis_CM) ;
	if(freq!=NULL)
	{
		memcpy(freq,DownFreq_CMGet,20);
		eis_report("\n >>>>>>>>>>>>eis_get_CMMAC DownFreq=%s ",DownFreq_CMGet);
	}
	semaphore_signal(sem_eis_CM) ;
	
}
void eis_get_CMUpFreq( char *freq)
{
	 semaphore_wait(sem_eis_CM) ;
	if(freq!=NULL)
	{
		memcpy(freq,UpFreq_CMGet,20);
		eis_report("\n >>>>>>>>>>>>eis_get_CMMAC UpFreq=%s ",UpFreq_CMGet);
	}
	semaphore_signal(sem_eis_CM) ;
}

void eis_get_CMMAC( char *mac)
{
	semaphore_wait(sem_eis_CM) ;
	if(mac!=NULL)
	{
		memcpy(mac,MAC_CM,6);
		eis_report("\n >>>>>>>>>>>>eis_get_CMMAC mac=%s ",MAC_CM);
	}
	semaphore_signal(sem_eis_CM) ;
}
/*
void stmerge_STVID(void *prt)
{
	while(1)
	{
		CH_PrintStmerge();
		
		CH_STVID_GetStatistics();

		task_delay(ST_GetClocksPerSecond()*5);
	}
	
}
*/
/*信号检查函数*/
void eis_check_signal_msg(void *ptr)
{
	static boolean cable_signal=true;/*单向信号监控状态true:lock  false:unlock*/  
	static CHMID_CM_ONLINE_STATUS_e CM_state = CHMID_CM_OFFLINE;/*CM状态*/
	static boolean video_state=true;/* 视频信号监控状态true:ok  false:error*/
	static int IP_state_outtime=0;
	UINT16_T netStatus = 0;
	unsigned short  net1=20;
	U8 ipaddr[4],netmask[4],gateway[4],mac[6];
	static boolean bcheck_cm=true;/*状态位 :  为true CM 底层监控打开*/
	char frep_temp[CM_FREQ_SIZE];
	static int freq=0,last_freq=0;
	char *end_p;
	UINT32_T ip_temp = 0;
	UINT32_T gateway_temp = 0;
	static boolean first_SendCMOnLIneMsg=false;
	{/*	如果CM已经在线但是上下行频点尚未获取过，则需要获取*/

		memset(frep_temp,0,CM_FREQ_SIZE);
		CHMID_CM_GetDownFreq(frep_temp);
		
		semaphore_wait(sem_eis_CM) ;
		if(memcmp(DownFreq_CMGet,frep_temp,CM_FREQ_SIZE)!=0)
		{
			memcpy(DownFreq_CMGet,frep_temp,CM_FREQ_SIZE);
		}
		semaphore_signal(sem_eis_CM) ;
		
		memset(frep_temp,0,CM_FREQ_SIZE);
		CHMID_CM_GetUPFreq(frep_temp);
		
		semaphore_wait(sem_eis_CM) ;
		if(memcmp(UpFreq_CMGet,frep_temp,CM_FREQ_SIZE)!=0)
		{
			memcpy(UpFreq_CMGet,frep_temp,CM_FREQ_SIZE);
		}
		semaphore_signal(sem_eis_CM) ;
	}
	
     //  CH_GetPortStatus();
	while(1)
	{
	/*单向信号监控*/
	 if(EIS_get_tuner_state())
	 {		
		if (TRUE == CH_TUNER_IsLocked())
		{
			if(false == cable_signal)
			{	/*原来未锁定，现在锁定发连接消息*/
				eis_report("\n send IPANEL_CABLE_NETWORK_CONNECT to IPANLE,time=%d",get_time_ms());
				eis_api_msg_send ( IPANEL_EVENT_TYPE_DVB, IPANEL_CABLE_NETWORK_CONNECT, 0 );
				cable_signal=true;
						
						/*如果是H264视频正在播放需要单独恢复*/
						if((EIS_GetVIdeoType()==H264_VIDEO_STREAM)&&(true==EIS_GetLastVideoControlStaus()))
						{
							EIS_ResumeVideoPlay();
						}
			}
		}
		else
		{
			if(true == cable_signal)
			{	/*原来锁定，现在未锁定发断开消息*/
				eis_report("\n send IPANEL_CABLE_NETWORK_DISCONNECT to IPANLE,time=%d",get_time_ms());
				eis_api_msg_send ( IPANEL_EVENT_TYPE_DVB, IPANEL_CABLE_NETWORK_DISCONNECT, 0 );
				cable_signal=false;
			}
		}
	 }
	
       #if 1
	   if(IP_state_outtime>1)
	   {
	   	IP_state_outtime=0;
		if((true==EIS_GetLastVideoControlStaus())) /* 视频正在播放*/
		{ /* 视频信号监控*/
			if(CH_Check_VIDData())
			{/*视频BUF空*/
				if(true == video_state 
					|| EIS_GetIFrameStatus() == false /*判断I帧是否真正显示出来，如未成功，应再次显示 sqzow20100723*/
				)/*原来 视频正常，现在为空发视频异常消息*/
				{
					eis_report("\n send IPANEL_DEVICE_DECODER_HUNGER to IPANLE");
					eis_api_msg_send ( IPANEL_EVENT_TYPE_DVB, IPANEL_DEVICE_DECODER_HUNGER, 0 );
					video_state=false;
				}
			}
			else
			{
				if((EIS_GetVIdeoType()==MPEG1_VIDEO_STREAM)||(EIS_GetVIdeoType()==MPEG2_VIDEO_STREAM)||(EIS_GetVIdeoType()==H264_VIDEO_STREAM))
				{
					if(CH_VID_CheckVidMode())
					{
						CH_VID_ResumeMode();
					}
			}
			}
		}
		
		if((false == video_state)&&(CH_Check_VIDData()==false))	
		{	/*原来 视频异常，现在正常发视频恢复消息*/
			eis_report("\n send IPANEL_DEVICE_DECODER_NORMAL to IPANLE");
			eis_api_msg_send ( IPANEL_EVENT_TYPE_DVB, IPANEL_DEVICE_DECODER_NORMAL, 0 );
			video_state=true;
				
				/*如果是H264视频正在播放需要单独恢复*/
				if((EIS_GetVIdeoType()==H264_VIDEO_STREAM)&&(true==EIS_GetLastVideoControlStaus()))
				{
					EIS_ResumeVideoPlay();
				}
		}
	   }
	   else
	{
		IP_state_outtime++;
	}
	#endif	
	
	/*IP连接状态监控*/
	#if 1
	{
		if(CH_GetEisSocketNum()>0)/*双向使用时关闭查询功能*/
		{
			if(bcheck_cm)
			{
				CHMID_CM_Close();
				bcheck_cm=false;
			}
		}
		else/*双向 未使用时查询*/
		{
			CHMID_CM_ONLINE_STATUS_e cmStatus = CHMID_CM_CheckCMStatus();/*上线状态*/
			//eis_report("\n CM 状态监控cmStatus=%d, CM_state=%d",cmStatus,CM_state);
			if(bcheck_cm==false)
			{
				CHMID_CM_Open();
				bcheck_cm=true;
			}
			
			/*   将所有获得CN MAC和设置CM下行频点CM操作函数移植到一个进程以免引起阻塞*/
			{
				int Status;
				if(get_MAC_CM==false) /*开机后尚未成功获得CM 的MAC地址就去获取*/
				{
					char mac_temp[8];
					memset(mac_temp,0,8);
					Status=CHMID_CM_GetCMMac(mac_temp);
					if(Status==0)
					{
						semaphore_wait(sem_eis_CM) ;
						memcpy(MAC_CM,mac_temp,8);
						semaphore_signal(sem_eis_CM) ;
						eis_report("\n  Get CM Mac mac=%s ",mac_temp);
						get_MAC_CM=true;
					}
				}
				
				if(set_DownFreq_CM) /*需要设置CM下行频点*/
				{
					memset(frep_temp,0,CM_FREQ_SIZE);
					semaphore_wait(sem_eis_CM) ;
					memcpy(frep_temp,DownFreq_CMSet,CM_FREQ_SIZE);
					semaphore_signal(sem_eis_CM) ;
					CHMID_CM_SetDownFreq(frep_temp);
					task_delay(ST_GetClocksPerSecond()*2);
					eis_report("\n  Set CM Down Freq  freq=%s ",frep_temp);
					cmStatus = CHMID_CM_INITED;	
					set_DownFreq_CM=false;
				}
			}
			
			// eis_report("\n **************cmStatus=%d",cmStatus);
			if((cmStatus==CHMID_CM_DOWN_FREQ_LOCKED)) /*当状态为下行锁定时需要读取当前上行频点*/
			{
				memset(frep_temp,0,CM_FREQ_SIZE);
				CHMID_CM_GetUPFreq(frep_temp);
				semaphore_wait(sem_eis_CM) ;
				memcpy(UpFreq_CMGet,frep_temp,CM_FREQ_SIZE);
				freq=strtod(UpFreq_CMGet,&end_p);
				semaphore_signal(sem_eis_CM) ;
				{
					//eis_report("\n send IPANEL_CABLE_NETWORK_CM_CONNECTING to IPANLE CM freq=%d",freq);
					eis_api_msg_send ( IPANEL_EVENT_TYPE_NETWORK, IPANEL_CABLE_NETWORK_CM_CONNECTING, freq );
				}
			}
			 
			if((cmStatus != CM_state)||(cmStatus==CHMID_CM_INITED)||(cmStatus==CHMID_CM_CHECK_FAIL))
			{
				if(CM_state==CHMID_CM_ONLINE)
				{	/* CM 从CHMID_CM_ONLINE变为其他状态表示CM下线*/
					eis_report("\n send IPANEL_CABLE_NETWORK_CM_DISCONNECTED to IPANLE CM");
					eis_api_msg_send ( IPANEL_EVENT_TYPE_NETWORK, IPANEL_CABLE_NETWORK_CM_DISCONNECTED, 0 );
					eis_api_msg_send ( IPANEL_EVENT_TYPE_NETWORK, IPANEL_IP_NETWORK_DISCONNECT, 0 );
					/*启动DHCP*/
					memset(ipaddr,0,4);
					memset(netmask,0,4);
					memset(gateway,0,4);
					EIS_LoadMacAddr(&mac);
					eis_set_ip(ipaddr,netmask,gateway,mac,0);
					ip_addr=0;
					send_NetFailedStatus=false;
					ip_DHCP_satrt_time=get_time_ms();
					/*初始化上下行频点*/
					semaphore_wait(sem_eis_CM) ;
					memset(UpFreq_CMGet,0,CM_FREQ_SIZE);
					memset(DownFreq_CMGet,0,CM_FREQ_SIZE);
					semaphore_signal(sem_eis_CM) ;
					CHMID_CM_SetCheckCycleTime(3);
				}
				CM_state=cmStatus;
				
				switch(CM_state)
				{
					case CHMID_CM_CHECK_FAIL: /*检测失败*/
					case CHMID_CM_INITED: /*初始状态*/
						{	
							memset(frep_temp,0,CM_FREQ_SIZE);
							CHMID_CM_GetDownFreq(frep_temp);
							semaphore_wait(sem_eis_CM) ;
							memcpy(DownFreq_CMGet,frep_temp,CM_FREQ_SIZE);
							freq=strtod(DownFreq_CMGet,&end_p);
							semaphore_signal(sem_eis_CM) ;
							if(last_freq!=freq)
							{
								last_freq=freq;
								//eis_report("\n send IPANEL_CABLE_NETWORK_CM_CONNECTING to IPANLE CM freq=%d",freq);
								eis_api_msg_send ( IPANEL_EVENT_TYPE_NETWORK, IPANEL_CABLE_NETWORK_CM_CONNECTING, freq );
							}
						}
						break;
					case CHMID_CM_DOWN_FREQ_LOCKED: /*下行已锁定*/
						{	
							eis_report("\n send EIS_CABLE_NETWORK_CM_DOWNLINK_STATUS to IPANLE CM");
							memset(frep_temp,0,CM_FREQ_SIZE);
							CHMID_CM_GetDownFreq(frep_temp);
							semaphore_wait(sem_eis_CM) ;
							memcpy(DownFreq_CMGet,frep_temp,CM_FREQ_SIZE);
							semaphore_signal(sem_eis_CM) ;
							
							eis_api_msg_send ( IPANEL_EVENT_TYPE_NETWORK, EIS_CABLE_NETWORK_CM_DOWNLINK_STATUS, 1 );
						}
						break;
					case CHMID_CM_UP_FREQ_LOCKED:/*上行已锁定*/
						{	
							eis_report("\n send EIS_CABLE_NETWORK_CM_UPLINK_STATUS to IPANLE CM");
							memset(frep_temp,0,CM_FREQ_SIZE);
							CHMID_CM_GetUPFreq(frep_temp);
							semaphore_wait(sem_eis_CM) ;
							memcpy(UpFreq_CMGet,frep_temp,CM_FREQ_SIZE);
							semaphore_signal(sem_eis_CM) ;
							eis_api_msg_send ( IPANEL_EVENT_TYPE_NETWORK, EIS_CABLE_NETWORK_CM_UPLINK_STATUS, 1 );
						}
						break;
					case CHMID_CM_OFFLINE:/*	CM不在线*/
						#if 0
						{	
							eis_report("\n send IPANEL_CABLE_NETWORK_CM_DISCONNECTED to IPANLE CM");
							eis_api_msg_send ( IPANEL_EVENT_TYPE_NETWORK, IPANEL_CABLE_NETWORK_CM_DISCONNECTED, 0 );
							eis_api_msg_send ( IPANEL_EVENT_TYPE_NETWORK, IPANEL_IP_NETWORK_DISCONNECT, 0 );
							/*启动DHCP*/
							memset(ipaddr,0,4);
							memset(netmask,0,4);
							memset(gateway,0,4);
							EIS_LoadMacAddr(&mac);
							eis_set_ip(ipaddr,netmask,gateway,mac,0);
							ip_addr=0;
							ip_DHCP_satrt_time=get_time_ms();
						}
						#endif
						break;
					case CHMID_CM_ONLINE:/*	CM在线*/
						{	
							UINT32_T ip_temp1 = 0;
							eis_report("\n send IPANEL_CABLE_NETWORK_CM_CONNECTED to IPANLE CM time=%d",get_time_ms());
							CHMID_CM_SetCheckCycleTime(5);

							CHMID_TCPIP_GetGateWay(&ip_temp1);
							eis_api_msg_send ( IPANEL_EVENT_TYPE_NETWORK, IPANEL_CABLE_NETWORK_CM_CONNECTED, 0 );
							// eis_api_msg_send ( IPANEL_EVENT_TYPE_NETWORK, IPANEL_IP_NETWORK_DISCONNECT, 0 );
							/*启动DHCP*/
							if((first_SendCMOnLIneMsg)||(ip_temp1==0))
							{
							memset(ipaddr,0,4);
							memset(netmask,0,4);
							memset(gateway,0,4);
							EIS_LoadMacAddr(&mac);
							eis_set_ip(ipaddr,netmask,gateway,mac,0);
							ip_addr=0;
							send_NetFailedStatus=false;
							ip_DHCP_satrt_time=get_time_ms();
								first_SendCMOnLIneMsg=true;
								eis_report("\n start DHCP...");
							}
							else
							{
								first_SendCMOnLIneMsg=true;
							}
						}
						break;

				}
			}
		}
		
		if(EIS_DHCP_set)
		{
			EIS_DHCP_set=false;
			eis_DHCP_Start();
		}
		
		if((ip_addr==0))
		{
			net1 = CHMID_TCPIP_GetNetStatus();
			if(1/*net1&CHMID_TCPIP_USABLE*/)
			{
				CHMID_TCPIP_GetIP(&ip_addr);
				if(ip_addr!=0)
				{
					ip_temp=CHMID_TCPIP_NTOHL(ip_addr);
					CHMID_TCPIP_GetGateWay(&gateway_temp);
					if(((ip_temp>>8)==0xc0a864)&&(CM_state==CHMID_CM_ONLINE)&&(gateway_temp==0))
					{/*  如果在CM上线时获得的是CM分配的IP地址就认为是无效的重新获取*/
						memset(ipaddr,0,4);
						memset(netmask,0,4);
						memset(gateway,0,4);
						EIS_LoadMacAddr(&mac);
						eis_set_ip(ipaddr,netmask,gateway,mac,0);
						ip_addr=0;
					}
					else
					{
					eis_report("\n send IPANEL_IP_NETWORK_READY to IPANLE");
					 eis_api_msg_send ( IPANEL_EVENT_TYPE_NETWORK, IPANEL_IP_NETWORK_READY, 0 );
				}
				}
				else
				{
					if((get_time_ms()-ip_DHCP_satrt_time)>DHCP_OUTTIME)
					{
						if(send_NetFailedStatus==false)
					{
						eis_report("\n send IPANEL_IP_NETWORK_FAILED to IPANLE");
						 eis_api_msg_send ( IPANEL_EVENT_TYPE_NETWORK, IPANEL_IP_NETWORK_FAILED, 0 );
							 send_NetFailedStatus=true;
						}
					}
				}
			}
			else
			{
				if((get_time_ms()-ip_DHCP_satrt_time)>DHCP_OUTTIME)
				{
					if(send_NetFailedStatus==false)
				{
				eis_report("\n send IPANEL_IP_NETWORK_FAILED to IPANLE");
				eis_api_msg_send ( IPANEL_EVENT_TYPE_NETWORK, IPANEL_IP_NETWORK_FAILED, 0 );
						 send_NetFailedStatus=true;
					}
			}
		}
		}

		if((CHMID_CM_ONLINE!=CM_state)&&(first_SendCMOnLIneMsg==false))
			task_delay(ST_GetClocksPerSecond()/5);
		else
			task_delay(ST_GetClocksPerSecond()*1);
	}
	#endif
	}
}

void EIS_Msg_process_init( void )
{
	sem_eis_CM = semaphore_create_fifo(1);
	CHMID_CM_SetCheckCycleTime(1);	 
	if ( ( Task_Create( eis_check_signal_msg, NULL,
					  1024*40, 5,
					  "eis_check_signal_msg", 0 ) ) == NULL )
	{
		eis_report ( "\n eis_check_signal_msg creat failed\n");
	}
/*
	if ( ( Task_Create(stmerge_STVID, NULL,
					  1024*2, 5,
					  "stmerge_STVID", 0 ) ) == NULL )
	{
		eis_report ( "\n stmerge_STVID creat failed\n");
	}
*/	
}


/*
  * Func: eis_key_map
  * Desc: 键值映射函数
  * In:	DVBKey
  * Out:	EisKey

  只转换ipannel需要处理的键值，其余的均返回-1，由系统usif来处理，sqzow080319
  */
int eis_front_key_map( int iDVBKey )
{
	switch( iDVBKey )
	{

		case UP_ARROW_FRONTKEY:           return IPANEL_IRKEY_UP;			
		case DOWN_ARROW_FRONTKEY: 	return IPANEL_IRKEY_DOWN;
		case LEFT_ARROW_FRONTKEY: 	return IPANEL_IRKEY_LEFT;
		case RIGHT_ARROW_FRONTKEY: 	return IPANEL_IRKEY_RIGHT;
		case MENU_FRONTKEY: 	return IPANEL_IRKEY_MENU;
		case OK_FRONTKEY:		return IPANEL_IRKEY_SELECT;

		default: return -1;
	}
}



void CH_SetIp(void)
{
	int i_result = -1;

	U8 ipaddr[4];
	U8 netmask[4];
	U8 gateway[4];
	U8 mac[6];
	
	CHDRV_NET_Config_t gNetConfigInfo = {{0x00, 0x14, 0x49, 0x10, 0x6f, 0x27},{172,17,83,79},{255,255,255,0},{172,17,83,254},{10,3,4,24},0};
	
	//i_result = CHMID_TCPIP_SetIP(gNetConfigInfo.ipaddress, gNetConfigInfo.netmask, gNetConfigInfo.gateway);

	if(0 == i_result)
	{
		eis_report("\n set ip ok \n");
	}
	else
	{
		eis_report("\n set ip error \n");
	}

	EIS_LoadMacAddr(&mac);
	
	memcpy(ipaddr, &gNetConfigInfo.ipaddress,4);
	memcpy(netmask, &gNetConfigInfo.netmask,4);
	memcpy(gateway, &gNetConfigInfo.gateway,4);
	
	eis_set_ip( ipaddr , netmask, gateway, mac, 1);

	task_delay(5000);
	
}


int eis_key_map( int iDVBKey )
{
	switch( iDVBKey )
	{
	#if 1
		case ch_KEY_DIGIT0: 			return  IPANEL_IRKEY_NUM0;
		case ch_KEY_DIGIT1: 			return  IPANEL_IRKEY_NUM1;
		case ch_KEY_DIGIT2: 			return  IPANEL_IRKEY_NUM2;
		case ch_KEY_DIGIT3: 			return  IPANEL_IRKEY_NUM3;
		case ch_KEY_DIGIT4: 			return  IPANEL_IRKEY_NUM4;
		case ch_KEY_DIGIT5: 			return  IPANEL_IRKEY_NUM5;
		case ch_KEY_DIGIT6: 			return  IPANEL_IRKEY_NUM6;
		case ch_KEY_DIGIT7: 			return  IPANEL_IRKEY_NUM7;
		case ch_KEY_DIGIT8: 			return  IPANEL_IRKEY_NUM8;
		case ch_KEY_DIGIT9: 			return  IPANEL_IRKEY_NUM9;
		case ch_KEY_A:                         return  IPANEL_IRKEY_FUNCTION_A;
		case ch_homepage_key:            return  IPANEL_IRKEY_HOMEPAGE;
		case ch_KEY_B:                         return  IPANEL_IRKEY_FUNCTION_B;
		case ch_KEY_C:                         return  IPANEL_IRKEY_IME;	
		case ch_KEY_D:                         return  IPANEL_IRKEY_FUNCTION_D;	
	#endif	
		case ch_UP_ARROW_KEY:           return IPANEL_IRKEY_UP;			
		case ch_DOWN_ARROW_KEY: 	return IPANEL_IRKEY_DOWN;
		case ch_LEFT_ARROW_KEY: 	return IPANEL_IRKEY_LEFT;
		case ch_RIGHT_ARROW_KEY: 	return IPANEL_IRKEY_RIGHT;

		case ch_OK_KEY: 		return IPANEL_IRKEY_SELECT;
		
		case ch_RED_KEY: 			return IPANEL_IRKEY_RED;
		case ch_GREEN_KEY: 			return IPANEL_IRKEY_GREEN;
		case ch_YELLOW_KEY: 		return IPANEL_IRKEY_YELLOW; 
		case ch_BLUE_KEY: 			return IPANEL_IRKEY_BLUE;

		case ch_RETURN_KEY: 		return IPANEL_IRKEY_BACK;
		#if 0
		case TV_RADIO_KEY:		return IPANEL_IRKEY_IME;/*用马赛克键作为中英文切换键*/
		#endif
		case ch_MUTE_KEY:			return IPANEL_IRKEY_VOLUME_MUTE;
		case ch_VOLUME_UP_KEY:		return IPANEL_IRKEY_VOLUME_UP;
		case ch_VOLUME_DOWN_KEY: 	return IPANEL_IRKEY_VOLUME_DOWN;

		case ch_EXIT_KEY:
			return IPANEL_IRKEY_EXIT;
		case ch_MENU_KEY:
			return IPANEL_IRKEY_MENU;
		case ch_EPG_KEY:
			return IPANEL_IRKEY_EPG;
		case ch_HELP_KEY:
			return IPANEL_IRKEY_INFO;
		case ch_POWER_KEY:
			return IPANEL_IRKEY_STANDBY;	
		case ch_LIST_KEY:
			 return  IPANEL_IRKEY_PLAYLIST;
		case ch_P_UP_KEY :
			return IPANEL_IRKEY_HISTORY_FORWARD; 
		case ch_P_DOWN_KEY 	:
			return IPANEL_IRKEY_HISTORY_BACKWARD; 
		case ch_xinhao_key:
			return IPANEL_IRKEY_ASTERISK;
		case ch_jinhao_key:
			return IPANEL_IRKEY_NUMBERSIGN;	
		case ch_luzhi_key:
			return IPANEL_IRKEY_RECORD;
              case ch_tingzhi_key:
			return IPANEL_IRKEY_STOP;
		case ch_KEY_E :
			return IPANEL_IRKEY_FUNCTION_E;
              case ch_KEY_F :
			 return IPANEL_IRKEY_FUNCTION_F;
		case ch_radio_key:
			return IPANEL_IRKEY_DATA_BROADCAST_BK;
		case ch_stock_key:
			return IPANEL_IRKEY_STOCK;
		case ch_nvod_key:
			return IPANEL_IRKEY_NVOD;
		
 
#if 0
		case HTML_KEY:
			return IPANEL_IRKEY_PLAY;
		case NVOD_KEY:
			return IPANEL_IRKEY_STOP;
		case STOCK_KEY:
			return IPANEL_IRKEY_PAUSE;
		case EMAIL_KEY:
			return IPANEL_IRKEY_FASTFORWARD;
		case F9_KEY:
			return IPANEL_IRKEY_REWIND;
#endif
			
#if 0
		case EIS_POWER_KEY:				/* 电源键 */
			return EIS_IRKEY_POWER;

		case EIS_MUTE_KEY:
			return EIS_IRKEY_VOLUME_MUTE;

		case EIS_HELP_KEY:
			return EIS_IRKEY_VK_F1/*20060331 add*/;

		case EIS_VOLUME_UP_KEY:	
			return EIS_IRKEY_VOLUME_UP;

		case EIS_VOLUME_DOWN_KEY: 
			return EIS_IRKEY_VOLUME_DOWN;
#if 1
		case EIS_PAGE_UP_KEY:  	return EIS_IRKEY_VK_F7;
		case EIS_PAGE_DOWN_KEY:return EIS_IRKEY_VK_F8;
#else
		case EIS_PAGE_UP_KEY:  	return EIS_IRKEY_BACK;
#endif
		case EIS_UP_KEY: 		return EIS_IRKEY_UP;
		case EIS_DOWN_KEY: 		return EIS_IRKEY_DOWN;
		case EIS_LEFT_KEY: 		return EIS_IRKEY_LEFT;
		case EIS_RIGHT_KEY: 		return EIS_IRKEY_RIGHT;

		case EIS_SELECT_KEY: 	return EIS_IRKEY_SELECT;

		case EIS_HOME_PAGE_KEY:	return EIS_IRKEY_HOMEPAGE;
		
		case EIS_RETURN_KEY: 	return EIS_IRKEY_BACK;/**/

#if 0 /*m200708*/
		/*case EIS_MENU_KEY: return EIS_IRKEY_MENU;*/
#else
		case MOSAIC_PAGE_KEY: return EIS_IRKEY_MENU;
#endif
		case EIS_RED_KEY: return EIS_IRKEY_BUTTON_RED;
		
		case EIS_GREEN_KEY: return EIS_IRKEY_BUTTON_GREEN;
		case EIS_YELLOW_KEY: return EIS_IRKEY_BUTTON_YELLOW; 
		case EIS_BLUE_KEY: return EIS_IRKEY_BUTTON_BLUE;

		case EIS_A_KEY: return EIS_IRKEY_IME;

		#ifdef USE_RC6_REMOTE
		case EIS_PROG_LIST_KEY:
			return EIS_IRKEY_PROGRAM_LIST;
		#endif
#endif
		default: 
		{
#ifdef eis_debug_main	
			STTBX_Print(( "没有匹配键值!\n" ));
#endif
			return -1;
		}
	 	
	}
	return -1;
}

typedef enum {
 IPANEL_OPEN_OC_HOMEPAGE =1,
 } IPANEL_IOCTL_e;
#include "sectionbase.h"

void lzf_test_demux(void)
{
					int MaxSlotCount=MAX_NO_SLOT;
					int i,j;
					for(i=0;i<MaxSlotCount;i++)
					{
						if(1/*SectionSlot[i].PidValue==0x10*/)
						{
								//eis_report("\n 找到NIT的PID");
								if(SectionSlot[i].SlotHandle!=0xffffffff)
								{
									eis_report("\n\n SlotStatus=%d, PidValue=%d, SlotHandle=0x%x, BufferHandle=0x%x, SignalHandle=0x%x, FilterCount=%d",
										SectionSlot[i].SlotStatus,
										SectionSlot[i].PidValue,
										SectionSlot[i].SlotHandle,
										SectionSlot[i].BufferHandle,
										SectionSlot[i].SignalHandle,
										SectionSlot[i].FilterCount);
									for(j=0;j<MAX_NO_FILTER_TOTAL;j++)
									{
										if(SectionFilter[j].SlotHandle==SectionSlot[i].SlotHandle)
										{
											//eis_report("\n 找到对应FILTER");
											eis_report("\n FilterStatus=%d,SlotHandle=0x%x,FilterHandle=0x%x,appfilter=%d",
												SectionFilter[j].FilterStatus,
												SectionFilter[j].SlotHandle,
												SectionFilter[j].FilterHandle,
												SectionFilter[j].appfilter);
										}				
									}
								}
							}
						
					}
					task_delay(ST_GetClocksPerSecond()/2);
				}
/*
  * the entrance of ipanel module
  */

int eis_entry (  )
{
	int 			result = 0;
	int 			key;
	unsigned long p1;
	unsigned int 	sockevent;
	unsigned int 	event[3];
	U8			uLoop;
	int			uTstTunerTimeOut = 0;
	boolean		bExit = false;
	U32			iEisKey;
	int			iMsgStatus;
	U32			u32_MsgType, u32_MsgParam1, u32_MsgParam2;
	char uri[100];
	int i1=0;
	//U32			fre = 658000;
	extern void eis_exit_by_changhong();	/* Jqz add on 040707 */
	int temp_volume=0;
	BOOLEAN ExigencyBroad=FALSE;
#ifdef SUMA_SECURITY
	int KeyValueTemp;
#endif
	eis_report("\n eis_entry entering..go..here\n");
	CH6_AVControl(VIDEO_BLACK, false, CONTROL_VIDEOANDAUDIO);
	CH_AudioDelayControl(1);
	if ( 1 == eis_api_enter_init () )
	{
		return 1;
	}
	CH_UpdateApplMode ( APP_MODE_HTML );
	log_out_state 		= false;

        ipanel_porting_graphics_init();
	
	//CH_OnlyVideoControl(0x1c01, true, 0x1c01, 27);
	
	pEisHandle	=(VOID *) ipanel_create ( 80*1024*1024 );
#ifdef __EIS_API_MAIN_DEBUG__	
	eis_report ( "\n++>eis dvb_main(start) ok\n" );
#endif
	if(pEisHandle==0)
	{
		eis_api_out_term ();
		return 1;
	}
	while (1)
	{
		if( bExit == true )
			break;
		ipanel_porting_check_reboot ();/* 检查是否需要重启 */
		iMsgStatus = eis_api_msg_get ( &u32_MsgType, &u32_MsgParam1, &u32_MsgParam2 );
		if ( IPANEL_OK == iMsgStatus )
		{	
			if(IPANEL_DVB_TUNE_SUCCESS == u32_MsgParam1)
				do_report(0,"\n**** u32_MsgType = %d, u32_MsgParam1 = %d, u32_MsgParam2 = %d\n",u32_MsgType, u32_MsgParam1, u32_MsgParam2);

			ipanel_proc ( pEisHandle, u32_MsgType, u32_MsgParam1 ,u32_MsgParam2 );


		}
		/* 读键值 */
#if 0/*不用PIN码暂时屏蔽*/
#ifdef SUMA_SECURITY
			key = -1;/* 复位按键值*/
			if(CH_GetUserIDStatus() == false)/*是否正在输入密码*/
#endif
#endif
			{
				int SocketNum=CH_GetEisSocketNum();
				key = GetMilliKeyFromKeyQueue(10+SocketNum);
			}
#ifdef SUMA_SECURITY
			if(key == VIRKEY_CLOSESKB_KEY)/*终止软键盘进程任务*/
			{
				CH_SKB_TerminateTask();
				key = -1;
			}
			if(key == VIRKEY_CLOSESUMA_KEY)
			{
				SumaSTB_DeleteThread();/*检查数码视讯待删除任务 sqzow20100706*/
				key = -1;
			}
			
			if(key == VIRKEY_CHANGETS_KEY)
			{
				do_report(0,"IPANEL_CHANGE_TS_BYOTHERAPP \n");
				ipanel_ioctl(pEisHandle,/*IPANEL_CHANGE_TS_BYOTHERAPP*/6,NULL);
				do_report(0,"IPANEL_CHANGE_TS_BYOTHERAPP OVER \n");
			}
#endif		
		/* 当没有键值时 */
		if ( key == -1 )
		{
			/* timer事件和网络事件要等到有数据再给中间件发出*/
			ipanel_proc ( pEisHandle, IPANEL_EVENT_TYPE_TIMER, 0, 0 );
			CH_AudioDelayControl(-1);
		}
		else
		{
#ifdef SUMA_SECURITY
			 if(CH_GetSkbStatus() == true)
			 {
			
			 	KeyValueTemp = ch_RC6keyTOSecurityKEY(key);
				//do_report(0,"SumaSecure_SKB_pushMsg %d \n",KeyValueTemp);
				if(KeyValueTemp != -1)
				{
					
					SumaSecure_SKB_pushMsg(KeyValueTemp);				
				}
				
			 }
			 else
			 	
#endif		
{
			eis_report( "\n time=%d, eis key=%x",get_time_ms(),key);
			if(key == KEY_F)
			{
				eis_report("ipanel_DisableDebugControl\n");
                ipanel_DisableDebugControl();/*开茁壮打印*/
			}
		
			else if(key == KEY_E)/*切换卡模式*/
			{
				eis_report("ipanel_EnableDebugControl\n");
				ipanel_EnableDebugControl();/*开茁壮打印*/

		      }
						
			iEisKey = eis_key_map( key );
			if( iEisKey != -1 )
			{
				ipanel_proc ( pEisHandle, IPANEL_EVENT_TYPE_KEYDOWN, iEisKey, 0 );
				ipanel_proc ( pEisHandle, IPANEL_EVENT_TYPE_KEYUP, iEisKey, 0 );
				ipanel_proc ( pEisHandle, IPANEL_EVENT_TYPE_TIMER, 0, 0 );
			}
			else
			{
				iEisKey = eis_front_key_map( key );
				if( iEisKey != -1 )
				{
					ipanel_proc ( pEisHandle, IPANEL_EVENT_TYPE_KEYDOWN, iEisKey, 1 );
					ipanel_proc ( pEisHandle, IPANEL_EVENT_TYPE_KEYUP, iEisKey, 1 );
					ipanel_proc ( pEisHandle, IPANEL_EVENT_TYPE_TIMER, 0, 0 );
				}
			}
		}
			}
	#ifdef  NAGRA_PRODUCE_VERSION		
		/* 每循环50次检查一下信号是否正常*/
	#if 0/* 更改为独立任务查询*/
		if(uTstTunerTimeOut++>100)
		{
			eis_check_signal_msg();
			uTstTunerTimeOut=0;
			
		}
	#endif
	#endif	

	}

	ipanel_destroy ( pEisHandle );
	pEisHandle = NULL;

	ipanel_porting_graphics_exit();
	
	eis_api_out_term ();
	CH_UpdateApplMode ( APP_MODE_PLAY );
	return result;
}

double atof(const char *nptr)
{
    double result = 0, integer, decimal;
    char *p = (char *)nptr;
    int radix_point = 0;
    int i, j;  
    int flag = 0;
	int len;
	unsigned int base = 10;

	while(*p == ' '){ 
		p++;
	}
	
	if(*p == '-'){ 
		flag = 1;
		p++;
	}
	
    for (i=0; i<(int)strlen(nptr); i++)
    {
        if ((*p <= '9') && (*p >= '0'))
        {
            if (radix_point == 0)
            {
                integer = *p - '0';
                result *= 10;
                result += integer;
            }
            else
            {
                decimal = *p - '0';
		
                 decimal /= base;
			base*=10;
                result += decimal;
            }
        }
        else if (*p == '.')
        {
            if (radix_point == 0)
            {
                radix_point = 1;
            }
            else
            {
                break;
            }
        }
        else
        {
            break;
        }
               
        p++;    
    }
	
	if(flag == 1) result = 0 - result;
/*	do_report(0,"\n out=%f",result);*/
    return result;
}

/*ST SOCKET测试程序*/

static int TCPIPi_Read_Str_Token(U32 sock_desc,char *token,U32 size)
{
 char buffer;
 U32  rc,i=0;

 /* Loop to read all the bytes of the token */
 /* --------------------------------------- */
 while(i<size)
  {
   rc=0;
   while(rc==0)
    {
     /* Read the next character from the socket */

	rc =  CHMID_TCPIP_Recv(sock_desc, &buffer, 1, 0);
     if (rc==-1)
      {
       eis_report("TCPIPi_Read_Str_Token():**ERROR** !!! Failed to read from socket, !!!\n");
       return(-1);
      }
    }
   /* Copy the buffer to the token */
   if ((buffer=='\0')||(buffer =='\r')||(buffer=='\n'))
    {
     *token++='\0';
     return(0);
    }
   /* Store byte and continue, need to wait for a return carrier byte or 0 byte */
   else
    {
     *token++=buffer;
     i++;
    }
  }

 /* Return an error */
 /* -------------- */
 return(-1);
}

/* ========================================================================
   Name:        TCPIPi_Read_Exp_Token
   Description: Read a token from the socket and compare it
   ======================================================================== */

static int TCPIPi_Read_Exp_Token(U32 sock_desc,char *expected)
{
 char token[80];

 /* Attempt to read the token */
 /* ------------------------- */
 if (TCPIPi_Read_Str_Token(sock_desc,token,80)==-1)
  {
   eis_report("TCPIPi_Read_Exp_Token():**ERROR** !!! Failed to read token !!!\n");
   return(-1);
  }

 /* Compare the token read to what we expecte */
 /* ----------------------------------------- */
 if (strcmp (token, expected) != 0)
  {
   eis_report("TCPIPi_Read_Exp_Token():**ERROR** !!! Read token '%s' but expected '%s' !!!\n",token,expected);
   return(-1);
  }

 /* Return no errors */
 /* ---------------- */
 return(0);
}

/* ========================================================================
   Name:        TCPIPi_Send_Str_Token
   Description: Write the specified string token to the specifed socket
   ======================================================================== */

static int TCPIPi_Send_Str_Token(U32 sock_desc,char *token)
{
 return CHMID_TCPIP_Send(sock_desc, (char *)token, strlen(token)+1, 0);
}

/* ========================================================================
   Name:        TCPIPi_Send_Int_Token
   Description: Write the specified integer value to the specifed socket
   ======================================================================== */

static int TCPIPi_Send_Int_Token(U32 sock_desc,U32 value)
{
 char token[80];
 sprintf(token,"%u",value);
 return(TCPIPi_Send_Str_Token(sock_desc,token));
}


/* ========================================================================
   Name:        TCPIPi_Read_Data
   Description: Read the specified amount of data specified in packet size
   ======================================================================== */

static int TCPIPi_Read_Data(U32 sock_desc,U32 data_size,U32 packet_size)
{
 U8  *buffer;
 U32  start_time,todo,size,rc;

 /* Output the status */
 /* ----------------- */
 eis_report("\tReading %d bytes of data in %lu-byte packets...\r",data_size,packet_size);

 /* Allocate a buffer to read the data into */
 /* --------------------------------------- */
 if ((buffer=malloc(packet_size))==NULL)
  {
   eis_report("TCPIPi_Read_Data():**ERROR** !!! Failed to allocate packet buffer!!!\n");
   return(-1);
  }

 /* Send the 'start send' token */
 /* --------------------------- */
 if (TCPIPi_Send_Str_Token(sock_desc,"start send")==-1)
  {
   eis_report("TCPIPi_Read_Data():**ERROR** !!! Failed to send 'start send' token !!!\n");
   free(buffer);
   return(-1);
  }

 /* Get the current time */
 /* -------------------- */
 start_time=Time_Now();

 /* Loop reading the data */
 /* --------------------- */
 for (todo=data_size;todo>0;)
  {
   /* Calculate how much data to process this time round */
   size=(todo>packet_size)?packet_size:todo;
   /* Actually read the data */
   if ((rc=CHMID_TCPIP_Recv(sock_desc,buffer,size,0))==-1)
    {
     eis_report("TCPIPi_Read_Data():**ERROR** !!! Failed to read datas!!!\n");
     free(buffer);
     return(-1);
    }
   /* Subtract the amount of data read */
   todo-=rc;
  }


 /* Free off the buffer */
 /* ------------------- */
 free(buffer);

 /* Return no errors */
 /* ---------------- */
 return(0);
}

/* ========================================================================
   Name:        TCPIPi_Send_Data
   Description: Send the specified amount of data specified in packet size
   ======================================================================== */

static int TCPIPi_Send_Data(U32 sock_desc,U32 data_size,U32 packet_size)
{
 U8  *buffer;
 U32  start_time,todo,size,rc;

 /* Output the status */
 /* ----------------- */
 eis_report("\tSending %d bytes of data in %lu-byte packets...\r",data_size,packet_size);

 /* Allocate a buffer to send the data into */
 /* --------------------------------------- */
 if ((buffer=malloc(packet_size))==NULL)
  {
   eis_report("TCPIPi_Send_Data():**ERROR** !!! Failed to allocate packet buffer !!!\n");
   return(-1);
  }

 /* Read the 'start send' token */
 /* --------------------------- */
 if (TCPIPi_Read_Exp_Token(sock_desc,"start send")==-1)
  {
   eis_report("TCPIPi_Send_Data():**ERROR** !!! Failed to read 'start send' token !!!\n");
   free(buffer);
   return(-1);
  }

 /* Get the current time */
 /* -------------------- */
 start_time=Time_Now();

 /* Loop sending the data */
 /* --------------------- */
 for (todo=data_size;todo>0;)
  {
   /* Calculate how much data to process this time round */
   size=(todo>packet_size)?packet_size:todo;
   /* Actually send the data */
   if ((rc=CHMID_TCPIP_Send(sock_desc,buffer,size,0))==-1)
    {
     free(buffer);
     return(-1);
    }
   /* Subtract the amount of data sent */
   todo-=rc;
  }


 /* Free off the buffer */
 /* ------------------- */
 free(buffer);

 /* Return no errors */
 /* ---------------- */
 return(0);
}
#if 0
static U32              data_size = 4*1024*1024;
static U32              packet_size = 1500;
static U32              iterations =     1;
typedef  CHMID_TCPIP_SockAddr_t sockaddr_in_t;

void ST_SocketTest(void)
{
 S32    ID;
 U32    sock_id,i,timeout;
 int      i_ret;
   sockaddr_in_t serv_addr;

 /* Create the socket descriptor */
 /* ---------------------------- */
 if ((sock_id=CHMID_TCPIP_Socket(AF_INET,SOCK_STREAM,0))==-1)
  {
   eis_report("--> Failed to create socket!\n");
   return(TRUE);
  }
serv_addr.us_Family= AF_INET;		
serv_addr.us_Port= CHMID_TCPIP_HTONS(1234);  
serv_addr.ui_IP= CHMID_TCPIP_NTOHL(0x0A00424b);
//serv_addr.ui_IP= CHMID_TCPIP_NTOHL(0x59C7C68A);
while(true)
{
	i_ret = CHMID_TCPIP_Connect(sock_id,&serv_addr, sizeof(serv_addr));
	if(i_ret >=0)
	{
	      eis_report("Connect ok\n");
	      break;
	}
	else
	{
	      eis_report("Connect failed,try again\n");
		
	    continue;
	}
}	

 /* Handshake with the TCP bandwidth server */
 /* ======================================= */
 eis_report("Handshaking with TCP bandwidth server...\n");

 /* Send 'ready' token to the socket */
 /* -------------------------------- */
 if (TCPIPi_Send_Str_Token(sock_id,"ready")==-1)
  {
   eis_report("--> Failed to send 'ready' token !\n");

   CHMID_TCPIP_Close(sock_id);
   return(TRUE);
  }

 /* Read 'ready' token back from the socket */
 /* --------------------------------------- */
 if (TCPIPi_Read_Exp_Token(sock_id,"ready")==-1)
  {
   eis_report("--> Failed to read 'ready' token!\n");
   CHMID_TCPIP_Close(sock_id);
   return(TRUE);
  }

 /* Send 'parameters' token to the socket */
 /* ------------------------------------- */
 if (TCPIPi_Send_Str_Token(sock_id,"parameters")==-1)
  {
   eis_report("--> Failed to send 'parameters' token !\n");
   CHMID_TCPIP_Close(sock_id);
   return(TRUE);
  }

 /* Read 'data size' token back from the socket */
 /* ------------------------------------------- */
 if (TCPIPi_Read_Exp_Token(sock_id,"data size")==-1)
  {
   eis_report("--> Failed to read 'data size' token\n");
   CHMID_TCPIP_Close(sock_id);
   return(TRUE);
  }

 /* Send the data size to the socket */
 /* -------------------------------- */
 if (TCPIPi_Send_Int_Token(sock_id,data_size)==-1)
  {
   eis_report("--> Failed to send data size !\n");
   CHMID_TCPIP_Close(sock_id);
   return(TRUE);
  }

 /* Read 'packet size' token back from the socket */
 /* --------------------------------------------- */
 if (TCPIPi_Read_Exp_Token(sock_id,"packet size")==-1)
  {
   eis_report("--> Failed to read 'packet size' token !\n");
   CHMID_TCPIP_Close(sock_id);
   return(TRUE);
  }

 /* Send the packet size to the socket */
 /* ---------------------------------- */
 if (TCPIPi_Send_Int_Token(sock_id,packet_size)==-1)
  {
   eis_report("--> Failed to send packet size!\n");
   CHMID_TCPIP_Close(sock_id);
   return(TRUE);
  }

 /* Read 'iterations' token back from the socket */
 /* -------------------------------------------- */
 if (TCPIPi_Read_Exp_Token(sock_id,"iterations")==-1)
  {
   eis_report("--> Failed to read 'iterations' token !\n");
   CHMID_TCPIP_Close(sock_id);
   return(TRUE);
  }

 /* Send the iterations to the socket */
 /* --------------------------------- */
 if (TCPIPi_Send_Int_Token(sock_id,iterations)==-1)
  {
   eis_report("--> Failed to send iterations !\n");
   return(TRUE);
  }

 /* Read 'ok' token back from the socket */
 /* ------------------------------------ */
 if (TCPIPi_Read_Exp_Token(sock_id,"ok")==-1)
  {
   eis_report("--> Failed to read 'ok' token!\n");
   return(TRUE);
  }
 eis_report("Connected to server...\n\n");


 /* Loop for each iteration */
 /* ======================= */
 for (i=0;i<iterations;++i)
  {
   /* Trace debug */
   /* ----------- */
   eis_report("Iteration %d of %lu\n",i+1,iterations);

   /* Actually read the data from the server */
   /* -------------------------------------- */
   if (TCPIPi_Read_Data(sock_id,data_size,packet_size)==-1)
    {
     eis_report("--> Failed to read data from server !\n");
   CHMID_TCPIP_Close(sock_id);
     return(TRUE);
    }

   /* Actually send the data to the server */
   /* ------------------------------------ */
   if (TCPIPi_Send_Data(sock_id,data_size,packet_size)==-1)
    {
     eis_report("--> Failed to send data to server\n");
   CHMID_TCPIP_Close(sock_id);
     return(TRUE);
    }
  }

 /* Close the socket */
 /* ================ */
   CHMID_TCPIP_Close(sock_id);

 /* Show the statistics */
 /* =================== */
 eis_report("end \n");
/* Return no errors */
 /* ================ */
 return(FALSE);
}
#endif






/*--eof----------------------------------------------------------------------------------------------------*/

