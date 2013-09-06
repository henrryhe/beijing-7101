/*******************文件说明注释************************************/
/*公司版权说明：版权（2009）归长虹网络科技有限公司所有。           */
/*文件名：MGCA_Ipanel.c                                            */
/*版本号：V1.0                                                     */
/*作者：  曾祥根                                                   */
/*创建日期：2009-06-23                                             */
/*模块功能:                                                        */
/*主要函数及功能: 实现IPANEL3.0中间件和MGCA之前的衔接控制          */
/*                                                                 */
/*修改历史：修改时间    作者      修改部分       原因              */
/*                                                                 */
/*******************************************************************/

/**************** #include 部分*************************************/
#include "stddefs.h"  /* standard definitions           */
#include "stevt.h"

#include "stddefs.h"
#include "stcommon.h"
#include "appltype.h"

#include "ipanel_typedef.h"
#include "ca_dmx_api.h"
#include "ca_emm_api.h"
#include "ca_srv_api.h"
#include "stb_ca_app.h"
#include "ca_pmt_api.h"
#include "MGCA_Ipanel.h"
#include "..\chcabase\ChUtility.h"

static int               gi_pmt_filter_index = 1;                                     /*当前PMT FILTER索引*/
static unsigned char     gc_pmt_version = 0xff;                                 /*当前PMT版本号*/
static unsigned char     gc_pmt_recv_flag = 0;                                    /*当前PMT是否已经接收到标示*/
static BOOL              gb_CAScrambleEnabled = 0;                              /*是否已经启动CA解扰动功能*/
static CH_Service_Info_t gch_CurServiceInfo;                                     /*当前播放节目服务信息*/
static semaphore_t                    *gp_MGCA_Ipanel = NULL;
static STB_CA_CONFIG_S          g_STB_CA_ConfigInfo;                              /*CA启动配置信息*/    
static STBCAAppCallback           g_STBCA_AppCallbackFuc = NULL;                            
STB_CA_STATUS_S                  g_STB_CA_Status;
STB_CA_OPERATOR_S              g_CA_OperatorList[16];                        /*CA 信息列表*/
int                                           gi_OPINumber = 0;
STB_CA_ID_S                          g_CA_ID_S;                                          /*CA版本信息*/                        
STB_CA_CARD_CONTENT_S      g_CA_CardContent;                              /*CA智能卡内容*/
STB_CA_PID_LIST_S                g_CA_PIDList;
U16                                         g_PIDList[2];

/*函数名:void CH_MGCA_IPanel_Init(void)                                      */
/*开发人和开发时间:                                                          */
/*函数功能描述:   初始化  MGCA_IPanel模块相关数据结构                        */
/*函数原理说明:                                                              */
/*使用的全局变量、表和结构：                                                 */
/*调用的主要函数列表：                                                       */
/*返回值说明：0，成功，-1，失败                                              */
void CH_MGCA_IPanel_Init(void)
{
       gi_pmt_filter_index  = 1;
       gc_pmt_version       =  0xff;
       gc_pmt_recv_flag     = -1;
	gb_CAScrambleEnabled = 0;
       gch_CurServiceInfo.original_network_id = -1;
	gch_CurServiceInfo.transport_stream_id = -1;
	gch_CurServiceInfo.service_id          = -1;
	gch_CurServiceInfo.video_pid           = 8191;
	gch_CurServiceInfo.audio_pid           = 8191;
	gch_CurServiceInfo.pcr_pid             = 8191;
	g_STB_CA_Status.bCAStarted = 0;
       g_STB_CA_Status.bIsPaired  = 1;
       g_STB_CA_Status.enScStatus = STB_CA_CardNotFound;
	  
	memset(&g_CA_ID_S,0,sizeof(STB_CA_ID_S));
	memset(&g_CA_CardContent,0,sizeof(STB_CA_CARD_CONTENT_S));
	g_CA_PIDList.puwPID = g_PIDList;
	g_STBCA_AppCallbackFuc = NULL;
	gp_MGCA_Ipanel = semaphore_create_fifo(1);

}

/*函数名:void CH_MGCA_IPanel_Init(void)                                      */
/*开发人和开发时间:                                                          */
/*函数功能描述:   初始化  MGCA_IPanel模块相关数据结构                        */
/*函数原理说明:                                                              */
/*使用的全局变量、表和结构：                                                 */
/*调用的主要函数列表：                                                       */
/*返回值说明：0，成功，-1，失败                                              */
void CH_MGCA_Ipanel_GetService(CH_Service_Info_t *rp_ServiceInfo)
{
    if(rp_ServiceInfo == NULL)
	{
		return;
	}
	semaphore_wait(gp_MGCA_Ipanel);
	rp_ServiceInfo->original_network_id = gch_CurServiceInfo.original_network_id;
	rp_ServiceInfo->transport_stream_id = gch_CurServiceInfo.transport_stream_id;
	rp_ServiceInfo->service_id          = gch_CurServiceInfo.service_id         ;
	rp_ServiceInfo->video_pid           = gch_CurServiceInfo.video_pid          ;
	rp_ServiceInfo->audio_pid           = gch_CurServiceInfo.audio_pid          ;
	rp_ServiceInfo->pcr_pid             = gch_CurServiceInfo.pcr_pid            ;
	semaphore_signal(gp_MGCA_Ipanel);
}

static CH_CA2IPANEL_EVT_t ipanel_event_e = CH_CA2IPANEL_Have_Right;
#ifndef    NAGRA_PRODUCE_VERSION 	
static CH_CA2IPANEL_EVT_t last_ipanel_event_e = CH_CA2IPANEL_Have_Right;
#endif

/*函数名:INT32_T ipanel_cam_pmt_ctrl(IPANEL_CAM_PMT_CTRL_e act, BYTE_T *data, INT32_T len)          */
/*开发人和开发时间:                                                          */
/*函数功能描述:                     */
/*函数原理说明:                                                              */
/*使用的全局变量、表和结构：                                                 */
/*调用的主要函数列表：                                                       */
/*返回值说明：0，成功，-1，失败                                              */

INT32_T ipanel_cam_pmt_ctrl(IPANEL_CAM_PMT_CTRL_e act, BYTE_T *data, INT32_T len)
{
       unsigned short   us_servicetype;
       if(data == NULL)
       	{
                 return -1;
       	}
	if (/*act == IPANEL_CAM_SET_PMT &&*/ (data[0] == 0x02) && (len > 8)) /*PMT数据有效*/
	{
			
			gc_pmt_version = data[5] & 0x3E;
			
		
			   /*首先判断服务类型*/
		      ipanel_cam_get_curr_srv_type(&us_servicetype);
			if(us_servicetype == 0xF6)/*NVOD*/
			{
		            if(CHCA_AppPmtParseDes(data))
		      	      {
                                  CH_MGCA_Ipanel_UpdateCardStatus(CH_CA2IPANEL_Have_Right,NULL,0);
		      	     } 
			else
				{
                                   CH_MGCA_Ipanel_UpdateCardStatus(CH_CA2IPANEL_NO_Right,NULL,0);
				}
			}
			else		  
			{
				if (IPANEL_CAM_UPDATE_PMT == act)
				{
					ChSendMessage2PMT(gch_CurServiceInfo.service_id, gch_CurServiceInfo.transport_stream_id, START_PMT_NCTP);
				}
			            CHCA_InsertPMTData(gi_pmt_filter_index,data);/*通知数据*/
			}

			
	}
	else
	{
		return -1;
	}
	return 0;
}
/*函数名:static INT32_T pmt_recv_notify(VOID* tag, BYTE_T* data, INT32_T len)*/
/*开发人和开发时间:                                                          */
/*函数功能描述:   PMT数据通知函数                                            */
/*函数原理说明:   the function must be non blocking                          */
/*使用的全局变量、表和结构：                                                 */
/*调用的主要函数列表：                                                       */
/*返回值说明：0，成功，-1，失败                                              */
static INT32_T pmt_recv_notify(VOID* tag, BYTE_T* data, INT32_T len)
{
       unsigned short   us_servicetype;
	if (tag && data && (tag == (void*)1000) && (data[0] == 0x02) && (len > 8)) /*PMT数据有效*/
	{
		if (gc_pmt_recv_flag == 0)
		{	
			gc_pmt_version = data[5] & 0x3E;
			
			gc_pmt_recv_flag = 1;
	              /*首先判断服务类型*/
		      ipanel_cam_get_curr_srv_type(&us_servicetype);
			if(us_servicetype == 0xF6)/*NVOD*/
			{
		            if(CHCA_AppPmtParseDes(data))
		      	      {
                                  CH_MGCA_Ipanel_UpdateCardStatus(CH_CA2IPANEL_Have_Right,NULL,0);
		      	     } 
			else
				{
                                   CH_MGCA_Ipanel_UpdateCardStatus(CH_CA2IPANEL_NO_Right,NULL,0);
				}
			}
			else		  
			{				
			      CHCA_InsertPMTData(gi_pmt_filter_index,data);/*通知数据*/
			}

		}
		else if (gc_pmt_version != (data[5]&0x3E))       /*PMT版本变化*/
		{
			gc_pmt_version = data[5] & 0x3E;
		}
	}
	else
	{
		return -1;
	}
	return 0;
}
/*函数名:static INT32_T pmt_recv_notify(VOID* tag, BYTE_T* data, INT32_T len)*/
/*开发人和开发时间:                                                          */
/*函数功能描述:  申请PMT过滤器                                               */
/*函数原理说明:                                                              */
/*使用的全局变量、表和结构：                                                 */
/*调用的主要函数列表：                                                       */
/*返回值说明：                                                               */
static void add_pmt_filter(unsigned short srv_id, unsigned short pid)
{
	INT32_T nRet = -1;
	IPANEL_FILTER_INFO info[1] = {0};
	info->pid = pid;
	info->coef[0] = 0x02;
	info->coef[3] = (srv_id >> 8) & 0xff;
	info->coef[4] = srv_id & 0xff;
	info->mask[0] = 0xff;
	info->mask[3] = 0xff;
	info->mask[4] = 0xff;
	info->depth = 5;
	info->func = pmt_recv_notify;
	info->tag = (void*)1000;
	if (gi_pmt_filter_index == -1) /*没有申请PMT*/
	{
		gi_pmt_filter_index = ipanel_add_filter(info);
	}
	else if (gi_pmt_filter_index >= 0) /*之前申请过PMT*/
	{
		nRet = ipanel_modify_filter(gi_pmt_filter_index, info);
	}
}
/*函数名:static void remove_pmt_filter(void)                                 */
/*开发人和开发时间:                                                          */
/*函数功能描述:  释放PMT过滤器                                               */
/*函数原理说明:                                                              */
/*使用的全局变量、表和结构：                                                 */
/*调用的主要函数列表：                                                       */
/*返回值说明：                                                               */
static void remove_pmt_filter(void)
{
	INT32_T nRet = -1;
	if (gi_pmt_filter_index >= 0) {
		nRet = ipanel_remove_filter(gi_pmt_filter_index);
		if (nRet >= 0)/*释放成功*/
		{
			gi_pmt_filter_index = -1;
			gc_pmt_recv_flag    = 0;
		}
	}
}

#if 1
/*函数名:INT32_T ipanel_cam_srv_ctrl(IPANEL_CAM_ECM_CTRL_e act, IPANEL_CAM_SRV_INFO *psrv)*/
/*开发人和开发时间:                                              */
/*函数功能描述:根据函数操作类型，service信息向CA添加/删除/升级   */
/*函数原理说明:                                                  */
/*使用的全局变量、表和结构：                                     */
/*调用的主要函数列表：                                           */
/*返回值说明：0，成功，-1，失败                                  */
INT32_T ipanel_cam_srv_ctrl(IPANEL_CAM_ECM_CTRL_e act, IPANEL_CAM_SRV_INFO *psrv)
{
	if (act == IPANEL_CAM_ADD_SRV) /*添加服务*/
	{
		if (psrv) 
		{
			if(gb_CAScrambleEnabled == 1)/*第一次启动节目加扰功能*/
			{
				gb_CAScrambleEnabled = 0;
				if(ChSendMessage2PMT(psrv->service_id,psrv->transport_stream_id,START_MGCA)==1)
				{
                    do_report(0,"\n[ipanel_cam_srv_ctrl==>] fail to send message the START_MGCA\n");
				}
			}
			else
			{
				if(gch_CurServiceInfo.transport_stream_id != psrv->transport_stream_id)
				{
					do_report(0,"NEW TSID = gi_LastPMT_TSID\n");
					if(ChSendMessage2PMT(psrv->service_id,psrv->transport_stream_id,START_PMT_CTP)==1)
					{
                        do_report(0,"\n[ipanel_cam_srv_ctrl==>] fail to send message the START_PMT_CTP\n");
					}
					
				}
				else
				{
					if(ChSendMessage2PMT(psrv->service_id,psrv->transport_stream_id,START_PMT_NCTP)==1)
					{
						do_report(0,"\n[ipanel_cam_srv_ctrl==>] fail to send message the START_PMT_NCTP\n");
					}
				}
			}
			#if 0
			/*确保释放上次的PMT FILTER*/
			remove_pmt_filter();
			
			/*开始新的PMT过滤*/

			add_pmt_filter(psrv->service_id, psrv->pmt_pid);
                   #endif

            semaphore_wait(gp_MGCA_Ipanel);
            gch_CurServiceInfo.original_network_id = psrv->original_network_id;
	        gch_CurServiceInfo.transport_stream_id = psrv->transport_stream_id;
            gch_CurServiceInfo.service_id          = psrv->service_id;
	        gch_CurServiceInfo.video_pid           = psrv->video_pid;
	        gch_CurServiceInfo.audio_pid           = psrv->audio_pid;
	        gch_CurServiceInfo.pcr_pid             = psrv->pcr_pid;
            semaphore_signal(gp_MGCA_Ipanel);
		
		}
	}
	else if (act == IPANEL_CAM_DEL_SRV) /*删除服务*/
	{
		//remove_pmt_filter();
	}
	else if(act == IPANEL_CAM_UPDATE_SRV)/*更新服务*/
	{
		
	}
	return 0;
}
#endif
#if 0
/*函数名:INT32_T ipanel_cam_emm_ctrl(IPANEL_CAM_EMM_CTRL_e act, BYTE_T *data, INT32_T len)*/
/*开发人和开发时间:                                                                       */
/*函数功能描述:根据函数操作类型，CAT表数据，启动或者停止EMM数据接收                       */
/*函数原理说明:                                                                           */
/*使用的全局变量、表和结构：                                                              */
/*调用的主要函数列表：                                                                    */
/*返回值说明：0，成功，-1，失败                                                           */
INT32_T ipanel_cam_emm_ctrl(IPANEL_CAM_EMM_CTRL_e act, BYTE_T *data, INT32_T len)
{
     if(act == IPANEL_CAM_SET_EMM)        /*开始EMM数据接收*/
	 {

	 }
     else if(act == IPANEL_CAM_STOP_EMM)  /*停止EMM数据接收*/
	 {

	 }
	 else if(act == IPANEL_CAM_UPDATE_EMM)/*更新EMM数据接收*/
	 {

	 }
	 return 0;
}
#endif
/*函数名:INT32 STBInitCA(STB_CA_CONFIG_S  stConfig, STBCAAppCallback pfnNotify)           */
/*开发人和开发时间:                                                                       */
/*函数功能描述: 启动CAK                                                                   */
/*函数原理说明:                                                                           */
/*使用的全局变量、表和结构：                                                              */
/*调用的主要函数列表：                                                                    */
/*返回值说明：0，成功，-1，失败                                                           */
INT32 STBInitCA(STB_CA_CONFIG_S  stConfig, STBCAAppCallback pfnNotify)
{
    memcpy(&g_STB_CA_ConfigInfo,&stConfig,sizeof(STB_CA_CONFIG_S));/*暂时不用使用*/
	/*开始初始化*/
	semaphore_wait(gp_MGCA_Ipanel);
    g_STBCA_AppCallbackFuc = pfnNotify;
	semaphore_signal(gp_MGCA_Ipanel);
	#ifdef  NAFR_KERNEL
	/*NAGRA FRANCE CA MODULE*/
	{
		extern  boolean  CH_CAModuleSetup(void);				
		if(CH_CAModuleSetup())
		{
			do_report(0,"Failed to initialise CH_CAModuleSetup\n");
			return -1; 
		}
	}	   
   #endif
    semaphore_wait(gp_MGCA_Ipanel);
	g_STB_CA_Status.bCAStarted = true;
	semaphore_signal(gp_MGCA_Ipanel);

	return 0;
}

/*函数名:INT32 STBGetCAStatus(STB_CA_STATUS_S* pstStatus)                                 */
/*开发人和开发时间:                                                                       */
/*函数功能描述: CA状态查询                                                                */
/*函数原理说明:                                                                           */
/*使用的全局变量、表和结构：                                                              */
/*调用的主要函数列表：                                                                    */
/*返回值说明：0，成功，-1，失败                                                           */
INT32 STBGetCAStatus(STB_CA_STATUS_S* pstStatus)
{
    if(pstStatus == NULL)
	{
       return -1;
	}
	else
	{
	   	semaphore_wait(gp_MGCA_Ipanel);
        memcpy(pstStatus,&g_STB_CA_Status,sizeof(STB_CA_STATUS_S));
	    semaphore_signal(gp_MGCA_Ipanel);
       return 0;
	}
}
/*函数名:INT32 STBSetSysPassword(CHAR* szOldPwd, CHAR* szNewPwd)                          */
/*开发人和开发时间:                                                                       */
/*函数功能描述: 设置系统密码                                                              */
/*函数原理说明:                                                                           */
/*使用的全局变量、表和结构：                                                              */
/*调用的主要函数列表：                                                                    */
/*返回值说明：0，成功，-1，失败                                                           */
INT32 STBSetSysPassword(CHAR* szOldPwd, CHAR* szNewPwd)
{
  return 0;
}
/*函数名:INT32 STBCheckSysPassword(CHAR* szPwd)                                           */
/*开发人和开发时间:                                                                       */
/*函数功能描述: 检查系统密码系统密码                                                      */
/*函数原理说明:                                                                           */
/*使用的全局变量、表和结构：                                                              */
/*调用的主要函数列表：                                                                    */
/*返回值说明：0，成功，-1，失败                                                           */
INT32 STBCheckSysPassword(CHAR* szPwd)
{
  return 0;
}
/*函数名:INT32 STBGetOperatorInfo(STB_CA_OP_TYPE_E enType, UINT16 uwOpi)                  */
/*开发人和开发时间:                                                                       */
/*函数功能描述: 获取运营商信息                                                            */
/*函数原理说明:                                                                           */
/*使用的全局变量、表和结构：                                                              */
/*调用的主要函数列表：                                                                    */
/*返回值说明：0，成功，-1，失败                                                           */
INT32 STBGetOperatorInfo(STB_CA_OP_TYPE_E enType, UINT16 uwOpi)
{
	U32      OPINumber;                    /*当前OPI个数*/
	if(enType == STB_CA_OP_GET_ONE || enType == STB_CA_OP_GET_FIRST)
            CHCA_IPanelGetOperatorInfo(g_CA_OperatorList,&gi_OPINumber);
	   
       if(gi_OPINumber <= 0)/*没有获取到信息*/
	{
               CH_MGCA_IpanelNotify(STB_CA_EvOperator,STB_CA_CardNotFound,sizeof(STB_CA_OPERATOR_S),NULL);
	}
	else
	{
		if(enType == STB_CA_OP_GET_ONE)
		{
		#if 0
		for(OPINumber = 0;OPINumber<gi_OPINumber;OPINumber++)
			{
                          if(g_CA_OperatorList[OPINumber].uwOPI == uwOpi)
                          	{
                                break;
                          	}
			}
		#endif
		OPINumber = uwOpi;
		
		      if(OPINumber < gi_OPINumber)
			{
			  CH_MGCA_IpanelNotify(STB_CA_EvOperator,STB_CA_CardOk,sizeof(STB_CA_OPERATOR_S),&g_CA_OperatorList[OPINumber]);
			}
		else
			{
                        CH_MGCA_IpanelNotify(STB_CA_EvOperator,STB_CA_CardNotFound,sizeof(STB_CA_OPERATOR_S),NULL);
  
			}

		}
		else if(enType == STB_CA_OP_GET_FIRST)
		{
		        OPINumber = 0;
			 CH_MGCA_IpanelNotify(STB_CA_EvOperator,STB_CA_CardOk,sizeof(STB_CA_OPERATOR_S),&g_CA_OperatorList[OPINumber]);
		}
		else if(enType == STB_CA_OP_GET_NEXT)
		{
		#if 0
			for(OPINumber = 0;OPINumber<gi_OPINumber;OPINumber++)
			{
                          if(g_CA_OperatorList[OPINumber].uwOPI == uwOpi)
                          	{
                                break;
                          	}
			}
		#endif	
			OPINumber = uwOpi;
			 if(OPINumber < (gi_OPINumber-1))
			{
			  CH_MGCA_IpanelNotify(STB_CA_EvOperator,STB_CA_CardOk,sizeof(STB_CA_OPERATOR_S),&g_CA_OperatorList[OPINumber]);
			}
		else if(OPINumber == (gi_OPINumber-1))
			{
                        CH_MGCA_IpanelNotify(STB_CA_EvOperator,STB_CA_Idling,sizeof(STB_CA_OPERATOR_S),&g_CA_OperatorList[OPINumber]);

			}
		else
			{
                        CH_MGCA_IpanelNotify(STB_CA_EvOperator,STB_CA_CardNotFound,sizeof(STB_CA_OPERATOR_S),NULL);
  
			}
			
		}
	}
	return 0;
}
/*函数名:INT32 STBStartRatingCheck(CHAR* szPwd)                                           */
/*开发人和开发时间:                                                                       */
/*函数功能描述: 启动成人级检测                                                            */
/*函数原理说明:                                                                           */
/*使用的全局变量、表和结构：                                                              */
/*调用的主要函数列表：                                                                    */
/*返回值说明：0，成功，-1，失败                                                           */
INT32 STBStartRatingCheck(CHAR* szPwd)
{
   return 0;
}
/*函数名INT32 STBStopRatingCheck(CHAR* szPwd)                                             */
/*开发人和开发时间:                                                                       */
/*函数功能描述: 停止成人级检测                                                            */
/*函数原理说明:                                                                           */
/*使用的全局变量、表和结构：                                                              */
/*调用的主要函数列表：                                                                    */
/*返回值说明：0，成功，-1，失败                                                           */
INT32 STBStopRatingCheck(CHAR* szPwd)
{
   return 0;
}
/*函数名INT32 STBGetRating(CHAR* szPwd)                                                   */
/*开发人和开发时间:                                                                       */
/*函数功能描述: 获取成人级                                                                */
/*函数原理说明:                                                                           */
/*使用的全局变量、表和结构：                                                              */
/*调用的主要函数列表：                                                                    */
/*返回值说明：0，成功，-1，失败                                                           */
INT32 STBGetRating(CHAR* szPwd)
{
   return 0;
}
/*函数名INT32 STBSetRating(UINT8 ucRating, CHAR* szPwd)                                   */
/*开发人和开发时间:                                                                       */
/*函数功能描述: 获取成人级                                                                */
/*函数原理说明:                                                                           */
/*使用的全局变量、表和结构：                                                              */
/*调用的主要函数列表：                                                                    */
/*返回值说明：0，成功，-1，失败                                                           */
INT32 STBSetRating(UINT8 ucRating, CHAR* szPwd)
{
   return 0;
}

/*函数名INT32 STBGetCAId(void)                                                            */
/*开发人和开发时间:                                                                       */
/*函数功能描述: 获取CA版本信息                                                            */
/*函数原理说明:                                                                           */
/*使用的全局变量、表和结构：                                                              */
/*调用的主要函数列表：                                                                    */
/*返回值说明：0，成功，-1，失败                                                           */
INT32 STBGetCAId(void)
{

   if(CHCA_GetKernelRelease((char *)g_CA_ID_S.astVer[0].Release) == 0)
   	{
   	       memcpy(&g_CA_ID_S.astVer[0].Date,"25/06/09",sizeof("25/06/09"));
		memcpy(&g_CA_ID_S.astVer[0].Name,"NAGRA MG",sizeof("NAGRA MG"));  
              CH_MGCA_IpanelNotify(STB_CA_EvGetId,STB_CA_Ok,sizeof(STB_CA_ID_S),&g_CA_ID_S);
		return 0;

   	}
   else
   	{
   	      CH_MGCA_IpanelNotify(STB_CA_EvGetId,STB_CA_NotAvailable,sizeof(STB_CA_ID_S),NULL);
             return -1;
   	}
}

/*函数名INT32 STBGetCardContent(void)                                                     */
/*开发人和开发时间:                                                                       */
/*函数功能描述: 获取智能卡内容                                                            */
/*函数原理说明:                                                                           */
/*使用的全局变量、表和结构：                                                              */
/*调用的主要函数列表：                                                                    */
/*返回值说明：0，成功，-1，失败                                                           */
INT32 STBGetCardContent(void)
{
   STB_CA_CARD_APP_S  temp_ca_app;
        g_CA_CardContent.pstAppData = &temp_ca_app;
   if(CHCA_GetCardContent(g_CA_CardContent.ucSerialNumber ) == false)
   	{
   	  
              CH_MGCA_IpanelNotify(STB_CA_EvContent,STB_CA_Ok,sizeof(STB_CA_CARD_CONTENT_S),&g_CA_CardContent);
		return 0;

   	}
   else
   	{
   	      CH_MGCA_IpanelNotify(STB_CA_EvGetId,STB_CA_CardFailure,sizeof(STB_CA_CARD_CONTENT_S),NULL);
             return -1;
   	}
  
   return 0;
}
/*函数名INT32 STBGetFuncData(UINT16 uwOpi, UINT8 ucIndex)                                 */
/*开发人和开发时间:                                                                       */
/*函数功能描述: 添加功能数据                                                              */
/*函数原理说明:                                                                           */
/*使用的全局变量、表和结构：                                                              */
/*调用的主要函数列表：                                                                    */
/*返回值说明：0，成功，-1，失败                                                           */
INT32 STBGetFuncData(UINT16 uwOpi, UINT8 ucIndex)
{
   return 0;
}

/*函数名INT32 STBCardUpdate(void);                                                        */
/*开发人和开发时间:                                                                       */
/*函数功能描述:更新智能卡信息                                                             */
/*函数原理说明:                                                                           */
/*使用的全局变量、表和结构：                                                              */
/*调用的主要函数列表：                                                                    */
/*返回值说明：0，成功，-1，失败                                                           */
INT32 STBCardUpdate(void)
{
   return 0;
}

/*函数名void CH_MGCA_Ipanel_UpdateCardStatus(CH_CA2IPANEL_EVT_t r_evt,U8 *Data)            */
/*开发人和开发时间:                                                                       */
/*函数功能描述: 更新卡的状态u                                                          */
/*函数原理说明:                                                                           */
/*使用的全局变量、表和结构：                                                              */
/*调用的主要函数列表：                                                                    */
/*返回值说明：                                                 */

void CH_MGCA_Ipanel_UpdateCardStatus(CH_CA2IPANEL_EVT_t r_evt,U8 *Data,int ri_len)
	{

	 if(CH_GetSmartControlStatus()==false)/*需要卡操作模式*/
	 {
		semaphore_wait(gp_MGCA_Ipanel);
		ipanel_event_e = CH_CA2IPANEL_OK_CARD;
		g_STB_CA_Status.enScStatus = STB_CA_CardOk;
		CH_MGCA_IpanelNotify(STB_CA_EvReady,0,0,NULL);
		semaphore_signal(gp_MGCA_Ipanel);

		return;

	 }
	 
	do_report(0,"CH_MGCA_Ipanel_UpdateCardStatus r_evt =%d\n",r_evt);
	semaphore_wait(gp_MGCA_Ipanel);
	ipanel_event_e = r_evt;

	switch(r_evt)
	{
                case   CH_CA2IPANEL_EXTRACT_CARD:
			    g_STB_CA_Status.enScStatus = STB_CA_CardNotFound;
#ifdef SUMA_SECURITY
				CH_SKB_EvExtractAction();
#endif
			   CH_MGCA_IpanelNotify(STB_CA_EvExtract,0,0,NULL);	
			   break;
                case   CH_CA2IPANEL_BAD_CARD:
			   g_STB_CA_Status.enScStatus = STB_CA_CardFailure;
		          CH_MGCA_IpanelNotify(STB_CA_EvBadCard,0,0,NULL);
			  break;
                case   CH_CA2IPANEL_UNKNOW_CARD:
			   g_STB_CA_Status.enScStatus = STB_CA_CardFailure;
		          CH_MGCA_IpanelNotify(STB_CA_EvBadCard,0,0,NULL);
			  break;
                case   CH_CA2IPANEL_OK_CARD:
			   g_STB_CA_Status.enScStatus = STB_CA_CardOk;
			   CH_MGCA_IpanelNotify(STB_CA_EvReady,0,0,NULL);	
			  break;
                case   CH_CA2IPANEL_Have_Right:
			  g_CA_PIDList.puwPID[0] = gch_CurServiceInfo.video_pid;
		         g_CA_PIDList.puwPID[1] = gch_CurServiceInfo.audio_pid;
			  g_CA_PIDList.ucNumber = 2;
			  
			 CH_MGCA_IpanelNotify(STB_CA_EvAcknowledged,STB_CA_CMAck,sizeof(STB_CA_PID_LIST_S),&g_CA_PIDList);		
			  break;
                case   CH_CA2IPANEL_NO_Right:
			 g_CA_PIDList.puwPID[0] = gch_CurServiceInfo.video_pid;
		         g_CA_PIDList.puwPID[1] = gch_CurServiceInfo.audio_pid;
			  g_CA_PIDList.ucNumber = 2;
			  
			 CH_MGCA_IpanelNotify(STB_CA_EvRejected,STB_CA_CMNANoRights,sizeof(STB_CA_PID_LIST_S),&g_CA_PIDList);
			 break;
		case CH_CA2IPANEL_EXPIRED:
		         g_CA_PIDList.puwPID[0] = gch_CurServiceInfo.video_pid;
		         g_CA_PIDList.puwPID[1] = gch_CurServiceInfo.audio_pid;
			  g_CA_PIDList.ucNumber = 2;
			  CH_MGCA_IpanelNotify(STB_CA_EvRejected,STB_CA_CMNAExpired,sizeof(STB_CA_PID_LIST_S),&g_CA_PIDList);			
			 break;
                 case  CH_CA2IPANEL_BLACKOUT:
		         g_CA_PIDList.puwPID[0] = gch_CurServiceInfo.video_pid;
		         g_CA_PIDList.puwPID[1] = gch_CurServiceInfo.audio_pid;
			  g_CA_PIDList.ucNumber = 2;
			  CH_MGCA_IpanelNotify(STB_CA_EvRejected,STB_CA_CMNABlackout,sizeof(STB_CA_PID_LIST_S),&g_CA_PIDList);			
	 	
			  break;
                case   CH_CA2IPANEL_RESET_PIN:
			  CH_MGCA_IpanelNotify(STB_CA_EvResetPwd,0,ri_len,Data);			

			  break;
                case   CH_CA2IPANEL_NEW_MAIL:
			   CH_MGCA_IpanelNotify(STB_CA_EvNewMail,0,ri_len,Data);	
			 break;
                case   CH_CA2IPANEL_PAIR_OK:
			   CH_MGCA_IpanelNotify(STB_CA_EvPairing,STB_CA_Pair_Yes,0,NULL);	
			  break;
                case CH_CA2IPANEL_PAIR_FAIL:
			   CH_MGCA_IpanelNotify(STB_CA_EvPairing,STB_CA_Pair_No,0,NULL);	
			 break;

	}
      
	  
             semaphore_signal(gp_MGCA_Ipanel);
}


/*函数名VOID CH_MGCA_IpanelNotify(UINT32 ulEventId, UINT32 ulResult, UINT32 ulDataLen, void* pData)        */
/*开发人和开发时间:                                                                       */
/*函数功能描述:通知IPANEL CA相关数据和状态                                */
/*函数原理说明:                                                                           */
/*使用的全局变量、表和结构：                                                              */
/*调用的主要函数列表：                                                                    */
/*返回值说明：                                                 */
void CH_MGCA_IpanelNotify(UINT32 ulEventId, UINT32 ulResult, UINT32 ulDataLen, void* pData)
{
   if(g_STBCA_AppCallbackFuc != NULL)
   	{
           	      g_STBCA_AppCallbackFuc(ulEventId,ulResult,ulDataLen,pData);

   	}
}
/*函数名int STBCheckPaired(void)      */
/*开发人和开发时间:                                                                       */
/*函数功能描述:                    */
/*函数原理说明:                                                                           */
/*使用的全局变量、表和结构：                                                              */
/*调用的主要函数列表：                                                                    */
/*返回值说明：                                                 */
int STBCheckPaired(void)
{
    return CH_IPNAEL_MGCA();
}
