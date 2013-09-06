/*******************************************************************************
* 文件名称:CHDRV_NET_Platform.c
* 功能描述: 为网络驱动和TCPIP协议栈提供平台支持，包括OS,系统调用等相关接口函数
* 在对协议栈做跨平台移植时,需要对此文件进行修改.
*
* 作者:flex
* 修改历史:
* 日期					作者						描述
* -----				-----------					------------
* 2009/3/30				flex 							create
*
********************************************************************************/

/*-----------------------------------------------------------------------------*/
/*-----------------------------------HEADERS---------------------------------------*/
#include "CHDRV_NET.h"
#include "initterm.h"
#include "stlite.h"
#include "stddefs.h"
#include "../../dbase/vdbase.h"
#include "stdarg.h"
#include "sttbx.h"
#include "CHDRV_NET_MAC110.h"
/*-------------------------------OS INTERFACES---------------------------------------*/
/*************************************************************************************
*
*
*
*
*
*
**************************************************************************************/
CHDRV_NET_THREAD CHDRV_NET_ThreadCreate(void (*rpf_function) (void *rpv_Arg),void *rpv_Arg, 
							u32_t rui_StackSize,s32_t rsi_Priority,s8_t *rsc_Name,u8_t ruc_Flags)
{
	CHDRV_NET_THREAD new_task;
	CHDRV_NET_ASSERT(rpf_function != NULL);
	CHDRV_NET_ASSERT(rsc_Name != NULL);
	
	CHDRV_NET_TRACE(CHDRV_NET_PLATFORM_DEBUG,("create thread"));
	new_task = (CHDRV_NET_THREAD)Task_Create(rpf_function, rpv_Arg, rui_StackSize, rsi_Priority, rsc_Name, ruc_Flags);
	if (new_task == NULL)
	{
		CHDRV_NET_ERROR(CHDRV_NET_PLATFORM_DEBUG,("CREATE THREAD FAIL"));
		return NULL;
	}
	CHDRV_NET_TRACE(CHDRV_NET_PLATFORM_DEBUG,("create thread ok"));
	return new_task;
}
CHDRV_NET_RESULT_e CHDRV_NET_ThreadResume(CHDRV_NET_THREAD	rh_Task)
{
	Task_t *task = (Task_t *)(rh_Task);
	CHDRV_NET_ASSERT(task!=NULL);
	Task_resume(task);
	return CHDRV_NET_OK;
}
CHDRV_NET_RESULT_e  CHDRV_NET_ThreadSuspend(CHDRV_NET_THREAD rh_Task)
{
	return CHDRV_NET_OK;
}

CHDRV_NET_RESULT_e  CHDRV_NET_ThreadSetPriority(CHDRV_NET_THREAD	rh_Task,s32_t ri_Priority)
{
	CHDRV_NET_ASSERT(rh_Task!=NULL);

	Task_Priority_Set((Task_t *)rh_Task,ri_Priority);
	return CHDRV_NET_OK;
}


CHDRV_NET_RESULT_e  CHDRV_NET_ThreadLock (void)
{
	Task_Lock();
	return CHDRV_NET_OK;
}
CHDRV_NET_RESULT_e CHDRV_NET_ThreadUnLock (void)
{
	Task_Unlock();
	return CHDRV_NET_OK;
}
/*****************************************************************************
* FUNCTION: CHDRV_NET_SemaphoreCreate
* DESCRIPTION:
*    creates a semaphore with a specified count.
*****************************************************************************/
CHDRV_NET_RESULT_e CHDRV_NET_SemaphoreCreate(s32_t ri_Count,CHDRV_NET_SEMAPHORE *rph_Semaphore)
{
	CHDRV_NET_SEMAPHORE psem = 0;
	Semaphore_Create_Fifo_TimeOut(psem,ri_Count);
	if(psem!=0)
	{
		*rph_Semaphore = psem;
		return 	CHDRV_NET_OK;
	}
	else
	{
		*rph_Semaphore = 0;
		CHDRV_NET_ERROR(CHDRV_NET_PLATFORM_DEBUG, ("semaphore create fail"));
		return CHDRV_NET_FAIL;
	}
}

/*****************************************************************************
* FUNCTION: CHDRV_NET_SemaphoreWait
* DESCRIPTION:
*    Behaves like smsc_semaphore_wait but will also wake up after a
*    specified number of milliSeconds.
* RETURN VALUE:
*    returns CHDRV_NET_OK if it get the  semaphore,
*    or returns CHDRV_NET_OK if it could not get the semaphore
*****************************************************************************/
CHDRV_NET_RESULT_e CHDRV_NET_SemaphoreWait(CHDRV_NET_SEMAPHORE rh_Semaphore,s32_t ri_TimeOut)
{ 
	clock_t  TimeOut;
	CHDRV_NET_ASSERT(rh_Semaphore!= 0UL);

	if(ri_TimeOut <=0 )
	{
		semaphore_wait_timeout(rh_Semaphore,TIMEOUT_INFINITY) ;
	}
	else
	{
		//do_report(0,"win [%d]s\n",time_now()/ST_GetClocksPerSecond());
		TimeOut=Time_Plus(Time_Now(),(ST_GetClocksPerSecond()/1000) *ri_TimeOut);
		if(semaphore_wait_timeout(rh_Semaphore,&TimeOut) != 0)
		{
			
		//do_report(0,"wout1 [%d]s\n",time_now()/ST_GetClocksPerSecond());
			return CHDRV_NET_TIMEOUT;
		}
	}
		//do_report(0,"wout2 [%d]s\n",time_now()/ST_GetClocksPerSecond());
	return CHDRV_NET_OK;
}

CHDRV_NET_RESULT_e  CHDRV_NET_SemaphoreSignal(CHDRV_NET_SEMAPHORE	 rh_Semaphore)
{
	CHDRV_NET_ASSERT(rh_Semaphore != 0UL);
	Semaphore_Signal(rh_Semaphore);
	return CHDRV_NET_OK;
}

CHDRV_NET_RESULT_e CHDRV_NET_SemaphoreFree(CHDRV_NET_SEMAPHORE rh_Semaphore)
{
	CHDRV_NET_ASSERT(rh_Semaphore!=0UL);
	Semaphore_Delete(rh_Semaphore);
	return CHDRV_NET_OK;
}

CHDRV_NET_MESSAGEQUEUE CHDRV_NET_CreateMessageQueue(u32_t ui_MsgLen,u32_t ui_QueueLen)
{
	CHDRV_NET_ASSERT(ui_MsgLen>0);
	CHDRV_NET_ASSERT(ui_QueueLen>0);
	return message_create_queue_timeout(ui_MsgLen,ui_QueueLen);
}
void * CHDRV_NET_ClaimMessageTimeout(CHDRV_NET_MESSAGEQUEUE MsgQueue,u32_t ui_TimeMs)
{
	clock_t timeout = ui_TimeMs;
	CHDRV_NET_ASSERT(MsgQueue);
	if(ui_TimeMs == CHDRV_NET_TIMEOUT_INFINITY){
		return message_claim_timeout(MsgQueue,TIMEOUT_INFINITY);
	}else if(ui_TimeMs == CHDRV_NET_TIMEOUT_IMMEDIATE){
		return message_claim_timeout(MsgQueue,TIMEOUT_IMMEDIATE);
	}else{
		return message_claim_timeout(MsgQueue,&timeout);
	}
}

CHDRV_NET_RESULT_e CHDRV_NET_MessageRelease(CHDRV_NET_MESSAGEQUEUE MsgQueue,void * up_Msg)
{
	CHDRV_NET_ASSERT(MsgQueue);
	CHDRV_NET_ASSERT(up_Msg!=NULL);
	message_release(MsgQueue,up_Msg);
	return CHDRV_NET_OK;
}
CHDRV_NET_RESULT_e CHDRV_NET_MessageSend(CHDRV_NET_MESSAGEQUEUE MsgQueue,void * up_Msg)
{
	CHDRV_NET_ASSERT(MsgQueue);
	CHDRV_NET_ASSERT(up_Msg!=NULL);
	message_send(MsgQueue,up_Msg); 
	return CHDRV_NET_OK;
}
void * CHDRV_NET_MessageReceive(CHDRV_NET_MESSAGEQUEUE MsgQueue,u32_t ui_TimeMs)
{
	clock_t timeout = ui_TimeMs;
	CHDRV_NET_ASSERT(MsgQueue);
	if(ui_TimeMs == CHDRV_NET_TIMEOUT_INFINITY){
		return message_receive_timeout( MsgQueue,TIMEOUT_INFINITY );
	}else if(ui_TimeMs == CHDRV_NET_TIMEOUT_INFINITY){
		return message_receive_timeout( MsgQueue,TIMEOUT_IMMEDIATE );
	}else{
		return message_receive_timeout( MsgQueue,&timeout );
	}
}
CHDRV_NET_RESULT_e CHDRV_NET_InterruptInstall(CHDRV_NET_InterruptHandler * int_func,void * arg)
{
	return CHDRV_NET_OK;
}
CHDRV_NET_RESULT_e CHDRV_NET_InterruptUninstall(void)
{
	return CHDRV_NET_OK;
}

extern ST_Partition_t          *appl_partition;
void *CHDRV_NET_MemAlloc(u32_t rui_Size)
{
	return memory_allocate(appl_partition,rui_Size);
}
void CHDRV_NET_MemFree(void *ptr)
{
	CHDRV_NET_ASSERT(ptr != NULL);
	memory_deallocate(appl_partition,ptr);
}
/*-------------------------------PLATFORM INTERFACES---------------------------------------*/
extern  BOX_INFO_STRUCT	*pstBoxInfo;
static CHDRV_NET_SEMAPHORE report_lock;
void CHDRV_NET_Print(const char *rsc_ContentStr, ...)
{
#if 0
  	s8_t msg[512];
  	va_list args;
	static u8_t sem_init = 0;
	if(sem_init == 0)
	{
		CHDRV_NET_SemaphoreCreate(1,&report_lock);
		sem_init = 1;
	}
	CHDRV_NET_SemaphoreWait(report_lock,-1);
  	va_start(args, rsc_ContentStr);
  	vsprintf(msg, rsc_ContentStr, args);
  	va_end(args);
  	STTBX_Print(("%s",msg));
	CHDRV_NET_SemaphoreSignal(report_lock);
#endif
}

CHDRV_NET_RESULT_e CHDRV_NET_LoadNetConfig(CHDRV_NET_Config_t * NetConfig)
{
	CHDRV_NET_ASSERT(NetConfig != NULL);
	memcpy(&(NetConfig->ipaddress),&(pstBoxInfo->ipcfg.ipaddress),IP_STRING_LENGTH);
	memcpy(&(NetConfig->gateway),&(pstBoxInfo->ipcfg.gateway),IP_STRING_LENGTH);
	memcpy(&(NetConfig->netmask),&(pstBoxInfo->ipcfg.netmask),IP_STRING_LENGTH);
	memcpy(&(NetConfig->dns),&(pstBoxInfo->ipcfg.dns),IP_STRING_LENGTH);
	/*option set*/
	strcpy(pstBoxInfo->ipcfg.Opt60ClientName,"openCable2.0:GHSTB");
	//pstBoxInfo->ipcfg.Opt60ClientName[0] = 0;
	strcpy(pstBoxInfo->ipcfg.Opt60ServerName,"");
	memcpy(&(NetConfig->Opt60ClientName),&(pstBoxInfo->ipcfg.Opt60ClientName),DHCP_OPTION_60_LEN);		
	memcpy(&(NetConfig->Opt60ServerName),&(pstBoxInfo->ipcfg.Opt60ServerName),DHCP_OPTION_60_LEN);
	NetConfig->ipmodel = pstBoxInfo->ipcfg.ipmodel;
	NetConfig->dhcpport= pstBoxInfo->ipcfg.dhcpport;
	/*从flash中读取机顶盒MAC地址,IP设置信息等*/
	/*...*/
	#if 1
	{
		EIS_LoadMacAddr(pstBoxInfo->ipcfg.macaddress);
	}	
	#else
	pstBoxInfo->ipcfg.macaddress[0] = 0x00;
	pstBoxInfo->ipcfg.macaddress[1] = 0x11;
	pstBoxInfo->ipcfg.macaddress[2] = 0x22;
	pstBoxInfo->ipcfg.macaddress[3] = 0x33;
	pstBoxInfo->ipcfg.macaddress[4] = 0xff;
	pstBoxInfo->ipcfg.macaddress[5] = 0x16;
	#endif

	/**/
	memcpy(NetConfig->macaddress,pstBoxInfo->ipcfg.macaddress,MAC_ADDRESS_LENGTH);
	return CHDRV_NET_OK;
}
CHDRV_NET_RESULT_e CHDRV_NET_SaveNetConfig(CHDRV_NET_Config_t * NetConfig)
{
	CHDRV_NET_ASSERT(NetConfig != NULL);
	/*保存网络配置信息到机顶盒*/
	memcpy(&pstBoxInfo->ipcfg,NetConfig,sizeof(CHDRV_NET_Config_t));
	CH_NVMUpdate ( idNvmBoxInfoId );
	return CHDRV_NET_OK;
}
u16_t CHDRV_NET_Chksum(void *rvp_Data, s32_t rsi_Len)
{
	
	u8_t *ps = ( u8_t *)rvp_Data;
	u32_t sum = 0;
	s32_t i = 0;
	s32_t len = rsi_Len;
	
	while (len > 1) {
		/*sum += SMSC_MAKE_SHORT_WORD(ps,i);*/
		sum += ((u16_t)(((u8_t *)ps)[i++]));
		sum += (((u16_t)(((u8_t *)ps)[i++]))<<8);
		len -= 2;
	}
	/* Consume left-over byte, if any */
	if (len > 0)
	{
		u16_t t = 0;
		((u8_t *)&t)[0] = ((u8_t *)ps)[i];
		sum += t;
	}
	/*  Fold 32-bit sum to 16 bits */
	while (sum >> 16)
	{
		sum = (sum & 0xffff) + (sum >> 16);
	}
	/*SMSC_TRACE(IPV4_DEBUG, ("smsc_chksum_linux(): 0x%4x", (u16_t)sum));*/
	return (u16_t)sum;

}
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
CHDRV_NET_RESULT_e CHDRV_NET_GetVersion(CHDRV_NET_Version_t *rpstru_Ver)
{
	CHDRV_NET_ASSERT(rpstru_Ver != NULL);
	rpstru_Ver->i_InterfaceMainVer = 2;
	rpstru_Ver->i_InterfaceSubVer = 1;
	rpstru_Ver->i_ModuleMainVer = 1;
	rpstru_Ver->i_ModuleSubVer = 0;
	return CHDRV_NET_OK;

}
/*-------------------------------DRIVER INTERFACES---------------------------------------*/
s8_t NetIfList[NET_INTERFACE_NUM][NET_INTERFACE_NAME_LEN] =
{
	ETHER_DEVICE_NAME
};

/******************************************************************************
*
*			BE DEFINED IN "CHDRV_NET_xxx.C" OR OTHER FILES OF DRIVER
*
*******************************************************************************/
/*EOF*/
