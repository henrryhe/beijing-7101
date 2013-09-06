/*******************************************************************************
* 文件名称 : CHMID_TCPIP_IO.c											
* 功能描述 :	对tcpip协议栈提供收发数据支持，直接与驱动接口函数进行对接，在更换驱动
* 时，需要针对不同的驱动作相应的修改。注意，在STMAC110芯片的实现上，创建了很多个
* (10个)接收任务进行收数据，因此，在驱动中也要有相应个数的收数据接口函数
* (CHDRV_NET_RecvX).
* 作者:flex
* 修改历史:
* 日期					作者						描述
* 2009/3/30				flex							create
*
********************************************************************************/

#include "network_stack.h"
#include "smsc_threading.h"
#include "packet_manager.h"
#include "CHMID_TCPIP_IO.h"

#define RX_TASK_PRIORITY	15
#define RX_TASK_STACK_SIZE 	8192
#define RX_BUFFER_LEN	1520
#define TX_BUFFER_LEN RX_BUFFER_LEN

#ifndef USE_MULTI_RX_THREAD
static void CH_TCPIP_RxTask(void *arg);
#else
static void CH_TCPIP_RxTask1(void *arg);
static void CH_TCPIP_RxTask2(void *arg);
static void CH_TCPIP_RxTask3(void *arg);
static void CH_TCPIP_RxTask4(void *arg);
static void CH_TCPIP_RxTask5(void *arg);
static void CH_TCPIP_RxTask6(void *arg);
static void CH_TCPIP_RxTask7(void *arg);
static void CH_TCPIP_RxTask8(void *arg);
static void CH_TCPIP_RxTask9(void *arg);
#endif

static  u8_t Tx_Buff[TX_BUFFER_LEN];

extern CHDRV_NET_Handle_t EtherDviceHandle[NET_INTERFACE_NUM];
static u32_t Rx_Flag = 0;

boolean CH_InitEthernetIO( struct NETWORK_INTERFACE *netif,int interfaceIndex)
{
	s32_t  ErrorCode=0;
	NETCARD_PRIVATE_DATA * privateData = (NETCARD_PRIVATE_DATA *)(netif->mHardwareData); 

	/*Initialize Tx queque*/
	PacketQueue_Initialize(&(privateData->mTxPacketQueue));
	Task_Initialize(&(privateData->mTxTask),TASK_PRIORITY_TX,CH_TCPIP_TxTask,netif);
	/*TaskManager_ScheduleByInterruptActivation(&(privateData->mTxTask));*/

	/*Initialize Rx queque*/
	PacketQueue_Initialize(&(privateData->mRxPacketQueue));
#ifndef USE_MULTI_RX_THREAD
	smsc_thread_create(CH_TCPIP_RxTask, netif, RX_TASK_STACK_SIZE, RX_TASK_PRIORITY, (s8_t *)"TCPIP_RX_TASK0", 0);
#else
	smsc_thread_create(CH_TCPIP_RxTask1, netif, RX_TASK_STACK_SIZE, RX_TASK_PRIORITY, "TCPIP_RX_TASK1", 0);
	smsc_thread_create(CH_TCPIP_RxTask2, netif, RX_TASK_STACK_SIZE, RX_TASK_PRIORITY, "TCPIP_RX_TASK2", 0);
	smsc_thread_create(CH_TCPIP_RxTask3, netif, RX_TASK_STACK_SIZE, RX_TASK_PRIORITY, "TCPIP_RX_TASK3", 0);
	smsc_thread_create(CH_TCPIP_RxTask4, netif, RX_TASK_STACK_SIZE, RX_TASK_PRIORITY, "TCPIP_RX_TASK4", 0);
	smsc_thread_create(CH_TCPIP_RxTask5, netif, RX_TASK_STACK_SIZE, RX_TASK_PRIORITY, "TCPIP_RX_TASK5", 0);
	smsc_thread_create(CH_TCPIP_RxTask6, netif, RX_TASK_STACK_SIZE, RX_TASK_PRIORITY, "TCPIP_RX_TASK6", 0);
	smsc_thread_create(CH_TCPIP_RxTask7, netif, RX_TASK_STACK_SIZE, RX_TASK_PRIORITY, "TCPIP_RX_TASK7", 0);
	smsc_thread_create(CH_TCPIP_RxTask8, netif, RX_TASK_STACK_SIZE, RX_TASK_PRIORITY, "TCPIP_RX_TASK8", 0);
	smsc_thread_create(CH_TCPIP_RxTask9, netif, RX_TASK_STACK_SIZE, RX_TASK_PRIORITY, "TCPIP_RX_TASK9", 0);/**/
#endif
	//smsc_thread_create(CH_Handle_RxData_Task, netif, 8192*50, 14, "TCPIP_HandleRX_TASK", 0);
	Task_Initialize(&(privateData->mRxTask),TASK_PRIORITY_RX,CH_Handle_RxData_Task,netif);
	TaskManager_ScheduleByInterruptActivation(&(privateData->mRxTask));/**/
	
	SMSC_ASSERT(MINIMUM_MTU<=1500);
	netif->mMTU=1500;
	sprintf(netif->mName,"SMSC_%d",interfaceIndex);
	SMSC_ASSERT(strlen(netif->mName)<NETWORK_INTERFACE_NAME_SIZE);
	ASSIGN_SIGNATURE(	privateData,SMSC911X_PRIVATE_DATA_SIGNATURE);
	
	if(ErrorCode == 0)
	{ 
		return false;
	}
	else
	{
		return true;
	}
}

 void CH_TCPIP_TxTask(void * arg)
{
	PPACKET_BUFFER   pkt = NULL;
	CHDRV_NET_RESULT_e     SendResult;
	PPACKET_QUEUE queue;
	struct NETWORK_INTERFACE * netp = (struct NETWORK_INTERFACE *)arg;
	NETCARD_PRIVATE_DATA * privateData;
	u16_t SendLen = 0;
	privateData=netp->mHardwareData;
	queue=&(privateData->mTxPacketQueue);

	/* Indicate output is active */
	/* Loop outputting packets */
	while(1)
	{
		pkt=PacketQueue_Pop(queue);
		if(pkt == NULL)
		{
			return;
		}
		memset(Tx_Buff,0,TX_BUFFER_LEN);
		memcpy(Tx_Buff,PacketBuffer_GetStartPoint(pkt),pkt->mThisLength);
		SendResult = 0;
		//SMSC_PLATFORM_MESSAGE(("NEED Tx send packet[%p] !len = [%d]\n",pkt,pkt->mThisLength));
		//ch_tcpip_dbgprintdata("TX",PacketBuffer_GetStartPoint(pkt),pkt->mThisLength);
		//printf("\nTX<<%02x %02x %02x %02x %02x %02x>>\n",Tx_Buff[0],Tx_Buff[1],Tx_Buff[2],Tx_Buff[3],Tx_Buff[4],Tx_Buff[5]);
		SendLen = pkt->mThisLength;
		SendResult = CHDRV_NET_Send(EtherDviceHandle[0],Tx_Buff,&SendLen);
		if(SendResult == CHDRV_NET_OK)
		{
			/* Free packet */
			//SMSC_PLATFORM_MESSAGE(("Tx send data success!len = [%d]\n",pkt->mThisLength));
		}
		else
		{
			/* Error sending packet, update statistics, and free packet */
		}
		PacketBuffer_DecreaseReference(pkt);
		pkt = NULL;
	}
}
#ifndef USE_MULTI_RX_THREAD
static  u8_t Rx_Buff0[RX_BUFFER_LEN];

static void CH_TCPIP_RxTask(void *arg)
{
	u16_t bufflen =0 ;
	struct NETWORK_INTERFACE * netp = (struct NETWORK_INTERFACE *)arg;
	CHDRV_NET_RESULT_e Ret = 0;
	PPACKET_BUFFER packet = NULL;
	NETCARD_PRIVATE_DATA * privateData= NULL;
	
	privateData= netp->mHardwareData;
	/* Infinite loop */
	while(1)
	{
		bufflen = CHDRV_NET_GetPacketLength(EtherDviceHandle[0]);
		if(bufflen > 0){
			packet = PacketBuffer_Create(bufflen);
			if(packet == NULL){
				continue;
			}
			Ret = CHDRV_NET_Recv(EtherDviceHandle[0],PacketBuffer_GetStartPoint(packet), &bufflen);
			if(Ret==CHDRV_NET_OK && bufflen>0){
				packet->mThisLength = bufflen;
				packet->mTotalLength = bufflen;
				PacketQueue_Push(&(privateData->mRxPacketQueue), packet);
				TaskManager_InterruptActivation(&(privateData->mRxTask));
				Rx_Flag++;
			}
		}else if(bufflen == 0){
			bufflen = RX_BUFFER_LEN;
			Ret = CHDRV_NET_Recv(EtherDviceHandle[0],&Rx_Buff0[0], &bufflen);
			if(Ret==CHDRV_NET_OK && bufflen>0)
			{			
				packet = PacketBuffer_Create(bufflen);
				if(packet == NULL)
				{
					memset(Rx_Buff0,0,RX_BUFFER_LEN);
					continue;
				}
				packet->mThisLength = bufflen;
				packet->mTotalLength = bufflen;
				memcpy(PacketBuffer_GetStartPoint(packet),&Rx_Buff0[0],bufflen);/**/
				PacketQueue_Push(&(privateData->mRxPacketQueue), packet);
				TaskManager_InterruptActivation(&(privateData->mRxTask));
				Rx_Flag++;
			}
		}
	}
}
#else
static  u8_t Rx_Buff1[RX_BUFFER_LEN];
static  u8_t Rx_Buff2[RX_BUFFER_LEN];
static  u8_t Rx_Buff3[RX_BUFFER_LEN];
static  u8_t Rx_Buff4[RX_BUFFER_LEN];
static  u8_t Rx_Buff5[RX_BUFFER_LEN];
static  u8_t Rx_Buff6[RX_BUFFER_LEN];
static  u8_t Rx_Buff7[RX_BUFFER_LEN];
static  u8_t Rx_Buff8[RX_BUFFER_LEN];
static  u8_t Rx_Buff9[RX_BUFFER_LEN];
static void CH_TCPIP_RxTask1(void *arg)
{
	u16_t bufflen =0 ;
	struct NETWORK_INTERFACE * netp = (struct NETWORK_INTERFACE *)arg;
	CHDRV_NET_RESULT_e Ret = 0;
	PPACKET_BUFFER packet = NULL;
	NETCARD_PRIVATE_DATA * privateData= NULL;

	privateData= netp->mHardwareData;
	while(1)
	{
		bufflen = RX_BUFFER_LEN;
		Ret = CHDRV_NET_Recv1(EtherDviceHandle[0],&Rx_Buff1[0], &bufflen);
		if(Ret==CHDRV_NET_OK && bufflen>0)
		{			
			//SMSC_PLATFORM_MESSAGE(("Rx receive data success!len = [%d]\n",g_async_read[0].length));
			packet = PacketBuffer_Create(bufflen);
			if(packet == NULL)
			{
				memset(Rx_Buff1,0,RX_BUFFER_LEN);
				continue;
			}
			packet->mThisLength = bufflen;
			packet->mTotalLength = bufflen;
			memcpy(PacketBuffer_GetStartPoint(packet),&Rx_Buff1[0],bufflen);/**/
			PacketQueue_Push(&(privateData->mRxPacketQueue), packet);
			TaskManager_InterruptActivation(&(privateData->mRxTask));
			Rx_Flag++;
		}
	}
}

static void CH_TCPIP_RxTask2(void *arg)
{
	u16_t bufflen =0 ;
	struct NETWORK_INTERFACE * netp = (struct NETWORK_INTERFACE *)arg;
	CHDRV_NET_RESULT_e Ret = 0;
	PPACKET_BUFFER packet = NULL;
	NETCARD_PRIVATE_DATA * privateData= NULL;

	privateData= netp->mHardwareData;
	while(1)
	{
		bufflen = RX_BUFFER_LEN;
		Ret = CHDRV_NET_Recv2(EtherDviceHandle[0],&Rx_Buff2[0], &bufflen);
		if(Ret==CHDRV_NET_OK && bufflen>0)
		{			
			//SMSC_PLATFORM_MESSAGE(("Rx receive data success!len = [%d]\n",g_async_read[0].length));
			packet = PacketBuffer_Create(bufflen);
			if(packet == NULL)
			{
				memset(Rx_Buff2,0,RX_BUFFER_LEN);
				continue;
			}
			packet->mThisLength = bufflen;
			packet->mTotalLength = bufflen;
			memcpy(PacketBuffer_GetStartPoint(packet),&Rx_Buff2[0],bufflen);/**/
			PacketQueue_Push(&(privateData->mRxPacketQueue), packet);
			TaskManager_InterruptActivation(&(privateData->mRxTask));
			Rx_Flag++;
		}
	}
}
 
static void CH_TCPIP_RxTask3(void *arg)
{
	u16_t bufflen =0 ;
	struct NETWORK_INTERFACE * netp = (struct NETWORK_INTERFACE *)arg;
	CHDRV_NET_RESULT_e Ret = 0;
	PPACKET_BUFFER packet = NULL;
	NETCARD_PRIVATE_DATA * privateData= NULL;

	privateData= netp->mHardwareData;
	while(1)
	{
		bufflen = RX_BUFFER_LEN;
		Ret = CHDRV_NET_Recv3(EtherDviceHandle[0],&Rx_Buff3[0], &bufflen);
		if(Ret==CHDRV_NET_OK && bufflen>0)
		{			
			//SMSC_PLATFORM_MESSAGE(("Rx receive data success!len = [%d]\n",g_async_read[0].length));
			packet = PacketBuffer_Create(bufflen);
			if(packet == NULL)
			{
				memset(Rx_Buff3,0,RX_BUFFER_LEN);
				continue;
			}
			packet->mThisLength = bufflen;
			packet->mTotalLength = bufflen;
			memcpy(PacketBuffer_GetStartPoint(packet),&Rx_Buff3[0],bufflen);/**/
			PacketQueue_Push(&(privateData->mRxPacketQueue), packet);
			TaskManager_InterruptActivation(&(privateData->mRxTask));
			Rx_Flag++;
		}
	}
}

static void CH_TCPIP_RxTask4(void *arg)
{
	u16_t bufflen =0 ;
	struct NETWORK_INTERFACE * netp = (struct NETWORK_INTERFACE *)arg;
	CHDRV_NET_RESULT_e Ret = 0;
	PPACKET_BUFFER packet = NULL;
	NETCARD_PRIVATE_DATA * privateData= NULL;

	privateData= netp->mHardwareData;
	while(1)
	{
		bufflen = RX_BUFFER_LEN;
		Ret = CHDRV_NET_Recv4(EtherDviceHandle[0],&Rx_Buff4[0], &bufflen);
		if(Ret==CHDRV_NET_OK && bufflen>0)
		{			
			//SMSC_PLATFORM_MESSAGE(("Rx receive data success!len = [%d]\n",g_async_read[0].length));
			packet = PacketBuffer_Create(bufflen);
			if(packet == NULL)
			{
				memset(Rx_Buff4,0,RX_BUFFER_LEN);
				continue;
			}
			packet->mThisLength = bufflen;
			packet->mTotalLength = bufflen;
			memcpy(PacketBuffer_GetStartPoint(packet),&Rx_Buff4[0],bufflen);/**/
			PacketQueue_Push(&(privateData->mRxPacketQueue), packet);
			TaskManager_InterruptActivation(&(privateData->mRxTask));
			Rx_Flag++;
		}
	}
}
 
static void CH_TCPIP_RxTask5(void *arg)
{
	u16_t bufflen =0 ;
	struct NETWORK_INTERFACE * netp = (struct NETWORK_INTERFACE *)arg;
	CHDRV_NET_RESULT_e Ret = 0;
	PPACKET_BUFFER packet = NULL;
	NETCARD_PRIVATE_DATA * privateData= NULL;

	privateData= netp->mHardwareData;
	while(1)
	{
		bufflen = RX_BUFFER_LEN;
		Ret = CHDRV_NET_Recv5(EtherDviceHandle[0],&Rx_Buff5[0], &bufflen);
		if(Ret==CHDRV_NET_OK && bufflen>0)
		{			
			//SMSC_PLATFORM_MESSAGE(("Rx receive data success!len = [%d]\n",g_async_read[0].length));
			packet = PacketBuffer_Create(bufflen);
			if(packet == NULL)
			{
				memset(Rx_Buff5,0,RX_BUFFER_LEN);
				continue;
			}
			packet->mThisLength = bufflen;
			packet->mTotalLength = bufflen;
			memcpy(PacketBuffer_GetStartPoint(packet),&Rx_Buff5[0],bufflen);/**/
			PacketQueue_Push(&(privateData->mRxPacketQueue), packet);
			TaskManager_InterruptActivation(&(privateData->mRxTask));
			Rx_Flag++;
		}
	}
}

static void CH_TCPIP_RxTask6(void *arg)
{
	u16_t bufflen =0 ;
	struct NETWORK_INTERFACE * netp = (struct NETWORK_INTERFACE *)arg;
	CHDRV_NET_RESULT_e Ret = 0;
	PPACKET_BUFFER packet = NULL;
	NETCARD_PRIVATE_DATA * privateData= NULL;

	privateData= netp->mHardwareData;
	while(1)
	{
		bufflen = RX_BUFFER_LEN;
		Ret = CHDRV_NET_Recv6(EtherDviceHandle[0],&Rx_Buff6[0], &bufflen);
		if(Ret==CHDRV_NET_OK && bufflen>0)
		{			
			//SMSC_PLATFORM_MESSAGE(("Rx receive data success!len = [%d]\n",g_async_read[0].length));
			packet = PacketBuffer_Create(bufflen);
			if(packet == NULL)
			{
				memset(Rx_Buff6,0,RX_BUFFER_LEN);
				continue;
			}
			packet->mThisLength = bufflen;
			packet->mTotalLength = bufflen;
			memcpy(PacketBuffer_GetStartPoint(packet),&Rx_Buff6[0],bufflen);/**/
			PacketQueue_Push(&(privateData->mRxPacketQueue), packet);
			TaskManager_InterruptActivation(&(privateData->mRxTask));
			Rx_Flag++;
		}
	}
}

static void CH_TCPIP_RxTask7(void *arg)
{
	u16_t bufflen =0 ;
	struct NETWORK_INTERFACE * netp = (struct NETWORK_INTERFACE *)arg;
	CHDRV_NET_RESULT_e Ret = 0;
	PPACKET_BUFFER packet = NULL;
	NETCARD_PRIVATE_DATA * privateData= NULL;

	privateData= netp->mHardwareData;
	while(1)
	{
		bufflen = RX_BUFFER_LEN;
		Ret = CHDRV_NET_Recv7(EtherDviceHandle[0],&Rx_Buff7[0], &bufflen);
		if(Ret==CHDRV_NET_OK && bufflen>0)
		{			
			//SMSC_PLATFORM_MESSAGE(("Rx receive data success!len = [%d]\n",g_async_read[0].length));
			packet = PacketBuffer_Create(bufflen);
			if(packet == NULL)
			{
				memset(Rx_Buff7,0,RX_BUFFER_LEN);
				continue;
			}
			packet->mThisLength = bufflen;
			packet->mTotalLength = bufflen;
			memcpy(PacketBuffer_GetStartPoint(packet),&Rx_Buff7[0],bufflen);/**/
			PacketQueue_Push(&(privateData->mRxPacketQueue), packet);
			TaskManager_InterruptActivation(&(privateData->mRxTask));
			Rx_Flag++;
		}
	}
}

static void CH_TCPIP_RxTask8(void *arg)
{
	u16_t bufflen =0 ;
	struct NETWORK_INTERFACE * netp = (struct NETWORK_INTERFACE *)arg;
	CHDRV_NET_RESULT_e Ret = 0;
	PPACKET_BUFFER packet = NULL;
	NETCARD_PRIVATE_DATA * privateData= NULL;

	privateData= netp->mHardwareData;
	while(1)
	{
		bufflen = RX_BUFFER_LEN;
		Ret = CHDRV_NET_Recv8(EtherDviceHandle[0],&Rx_Buff8[0], &bufflen);
		if(Ret==CHDRV_NET_OK && bufflen>0)
		{			
			//SMSC_PLATFORM_MESSAGE(("Rx receive data success!len = [%d]\n",g_async_read[0].length));
			packet = PacketBuffer_Create(bufflen);
			if(packet == NULL)
			{
				memset(Rx_Buff8,0,RX_BUFFER_LEN);
				continue;
			}
			packet->mThisLength = bufflen;
			packet->mTotalLength = bufflen;
			memcpy(PacketBuffer_GetStartPoint(packet),&Rx_Buff8[0],bufflen);/**/
			PacketQueue_Push(&(privateData->mRxPacketQueue), packet);
			TaskManager_InterruptActivation(&(privateData->mRxTask));
			Rx_Flag++;
		}
	}
}

static void CH_TCPIP_RxTask9(void *arg)
{
	u16_t bufflen =0 ;
	struct NETWORK_INTERFACE * netp = (struct NETWORK_INTERFACE *)arg;
	CHDRV_NET_RESULT_e Ret = 0;
	PPACKET_BUFFER packet = NULL;
	NETCARD_PRIVATE_DATA * privateData= NULL;

	privateData= netp->mHardwareData;
	while(1)
	{
		bufflen = RX_BUFFER_LEN;
		Ret = CHDRV_NET_Recv9(EtherDviceHandle[0],&Rx_Buff9[0], &bufflen);
		if(Ret==CHDRV_NET_OK && bufflen>0)
		{			
			//SMSC_PLATFORM_MESSAGE(("Rx receive data success!len = [%d]\n",g_async_read[0].length));
			packet = PacketBuffer_Create(bufflen);
			if(packet == NULL)
			{
				memset(Rx_Buff9,0,RX_BUFFER_LEN);
				continue;
			}
			packet->mThisLength = bufflen;
			packet->mTotalLength = bufflen;
			memcpy(PacketBuffer_GetStartPoint(packet),&Rx_Buff9[0],bufflen);/**/
			PacketQueue_Push(&(privateData->mRxPacketQueue), packet);
			TaskManager_InterruptActivation(&(privateData->mRxTask));
			Rx_Flag++;
		}
	}
}/* TCPIPi_InputTask() */
#endif

void CH_Handle_RxData_Task(void *arg)
{
	struct NETWORK_INTERFACE * netp = (struct NETWORK_INTERFACE *)arg;
	PPACKET_BUFFER packet = NULL;
	NETCARD_PRIVATE_DATA * privateData= netp->mHardwareData;
	PPACKET_QUEUE queue = &(privateData->mRxPacketQueue);
	u8_t Rx_Count = 0;

	s32_t time = 0;
	s32_t time1 = 0;
	time = CHDRV_NET_TimeNow()/CHDRV_NET_TicksOfSecond();
	
	TaskManager_ScheduleByInterruptActivation(&(privateData->mRxTask));
	while(!PacketQueue_IsEmpty(queue)/*&&Rx_Count<SMSC911X_RX_BURST_COUNT*/){
		packet = PacketQueue_Pop(queue);
		if(packet == NULL)
		{
			break;
		}
		Rx_Flag--;
		//ch_tcpip_dbgprintdata("Handle Packet",PacketBuffer_GetStartPoint(packet),packet->mThisLength);
		//SMSC_PLATFORM_MESSAGE(("\nRx handle packet[%p],packet->mThisLength[%d]...\n",packet,packet->mThisLength));
		SMSC_ASSERT(packet->mNext==NULL);
		Ethernet_Input(netp,packet);
		//SMSC_PLATFORM_MESSAGE(("END,Rx_Flag[%d]\n",Rx_Flag));
		Rx_Count++;
		time1 = CHDRV_NET_TimeNow()/CHDRV_NET_TicksOfSecond()-time;
	};
	if(!PacketQueue_IsEmpty(queue)) 
	{
		 TaskManager_InterruptActivation(&(privateData->mRxTask));
	}

}

static NETCARD_PRIVATE_DATA tmp_ETHERCARD_PRIVATE_DATA;
NETCARD_PRIVATE_DATA * EthernetCardInfo_Initialize(
struct NETWORK_INTERFACE * networkInterface)
{
	memset(&tmp_ETHERCARD_PRIVATE_DATA,0,sizeof(NETCARD_PRIVATE_DATA));
	networkInterface->mHardwareData=&tmp_ETHERCARD_PRIVATE_DATA;
	return &tmp_ETHERCARD_PRIVATE_DATA	;
}

void EthernetOutput(
	struct NETWORK_INTERFACE * networkInterface,
	PPACKET_BUFFER packet)
{
	NETCARD_PRIVATE_DATA * privateData;
	SMSC_ASSERT(networkInterface!=NULL);
	CHECK_SIGNATURE(networkInterface,NETWORK_INTERFACE_SIGNATURE);
	privateData=networkInterface->mHardwareData;
	SMSC_ASSERT(privateData!=NULL);
	CHECK_SIGNATURE(privateData,SMSC911X_PRIVATE_DATA_SIGNATURE);
	SMSC_ASSERT(packet!=NULL);
	SMSC_ASSERT(packet->mNext==NULL);/* We only accept one frame at a time */
	
	PacketQueue_Push(&(privateData->mTxPacketQueue),packet);

	if(Task_IsIdle(&(privateData->mTxTask))) {
		TaskManager_ScheduleAsSoonAsPossible(&(privateData->mTxTask));
	}/**/
	
}
/*EOF*/
