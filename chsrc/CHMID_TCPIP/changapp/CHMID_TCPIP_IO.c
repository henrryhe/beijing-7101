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
#include <osplus/ethernet/_ethernet.h>

#define RX_TASK_PRIORITY	15
#define RX_TASK_STACK_SIZE 	8192*2
#define RX_BUFFER_LEN	1520
#define TX_BUFFER_LEN RX_BUFFER_LEN



#define   MAX_TCPIP_NUM                1024
message_queue_t                               *gp_RxMessage = NULL;
message_queue_t                               *gp_TxMessage  = NULL;

void CHMID_TCPIP_ReceiveCallback(ethernet_async_t *async);
void CHMID_TCPIP_SendCallback(ethernet_async_t *async);
void CHMID_TCPIP_InitTxRX(void);
 void CHMID_TCPIP_InputTask(void *arg);
 void CHMID_TCPIP_SendTask(void *arg);
 extern struct NETWORK_INTERFACE smscNetworkInterface;
extern CHDRV_NET_SEMAPHORE  gp_io_semaphore ;
extern device_handle_t*  gpch_Ethernet_Hndl_p;	

extern CHDRV_NET_Handle_t EtherDviceHandle[NET_INTERFACE_NUM];
static u32_t Rx_Flag = 0;

static SMSC_SEMAPHORE gp_rx_queque_seamaphore = 0;

boolean CH_InitEthernetIO( struct NETWORK_INTERFACE *netif,int interfaceIndex)
{
	s32_t  ErrorCode=0;
	NETCARD_PRIVATE_DATA * privateData = (NETCARD_PRIVATE_DATA *)(netif->mHardwareData); 

       CHMID_TCPIP_InitTxRX();

	gp_rx_queque_seamaphore = smsc_semaphore_create(1);
	/*Initialize Tx queque*/
	PacketQueue_Initialize(&(privateData->mTxPacketQueue));
	Task_Initialize(&(privateData->mTxTask),TASK_PRIORITY_TX,CHMID_TCPIP_SendTask,netif);
	//TaskManager_ScheduleByInterruptActivation(&(privateData->mTxTask));

	/*Initialize Rx queque*/
	PacketQueue_Initialize(&(privateData->mRxPacketQueue));	
	smsc_thread_create(CHMID_TCPIP_InputTask, netif, RX_TASK_STACK_SIZE, RX_TASK_PRIORITY, "CHMID_TCPIP_InputTask", 0);

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

void RxPacketQueue_Push(PPACKET_QUEUE packetQueue,PPACKET_BUFFER packet)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(packetQueue!=NULL);
	CHECK_SIGNATURE(packetQueue,PACKET_QUEUE_SIGNATURE);
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(packet!=NULL);
	smsc_semaphore_wait(gp_rx_queque_seamaphore);	
	//SMSC_ASSERT(packet->mNext==NULL);/* ensure this is not a multifragment packet */
	/* TODO this function needs to be updated to support multifragment packets */
	/* TODO also update PacketQueue_Pop at the same time */
	/* TODO also update PacketQueue_UnPop at the same time */
	//SMSC_PLATFORM_MESSAGE(("Rx  packetQueue->mTail[%p],packet[%p]\n",packetQueue->mTail,packet));
	if(packetQueue->mHead!=NULL)
	{
		//SMSC_ASSERT(packetQueue->mTail!=NULL);
		//SMSC_ASSERT(packetQueue->mTail->mNext==NULL);
              if(packetQueue->mTail!=NULL)
		   packetQueue->mTail->mNext=packet;
	}
	else 
	{
		//SMSC_ASSERT(packetQueue->mTail==NULL);
		packetQueue->mHead=packet;
	}
	packetQueue->mTail=packet;
	smsc_semaphore_signal(gp_rx_queque_seamaphore);
}

PPACKET_BUFFER RxPacketQueue_Pop(PPACKET_QUEUE packetQueue)
{
	PPACKET_BUFFER result=NULL;

	SMSC_FUNCTION_PARAMS_CHECK_RETURN(packetQueue!=NULL,NULL);
	CHECK_SIGNATURE(packetQueue,PACKET_QUEUE_SIGNATURE);
	smsc_semaphore_wait(gp_rx_queque_seamaphore);	
	if(packetQueue->mHead!=NULL) {
		SMSC_ASSERT(packetQueue->mTail!=NULL);
		result=packetQueue->mHead;
		packetQueue->mHead=result->mNext;
		if(packetQueue->mHead==NULL) {
			packetQueue->mTail=NULL;
		}
		result->mNext=NULL;
	} else {
		SMSC_ASSERT(packetQueue->mTail==NULL);
	}
	smsc_semaphore_signal(gp_rx_queque_seamaphore);
	return result;
}



 
 

void CH_Handle_RxData_Task(void *arg)
{
	struct NETWORK_INTERFACE * netp = (struct NETWORK_INTERFACE *)arg;
	PPACKET_BUFFER packet = NULL;
	NETCARD_PRIVATE_DATA * privateData= netp->mHardwareData;
	PPACKET_QUEUE queue = &(privateData->mRxPacketQueue);


	while(!PacketQueue_IsEmpty(queue)){

		packet = RxPacketQueue_Pop(queue);
		if(packet == NULL)
		{
			break;
		}

		SMSC_ASSERT(packet->mNext==NULL);
		//smsc_semaphore_wait(gp_rx_queque_seamaphore);
		Ethernet_Input(netp,packet);
		//smsc_semaphore_signal(gp_rx_queque_seamaphore);
	};
	if(!PacketQueue_IsEmpty(queue)) 
	{
               TaskManager_ScheduleAsSoonAsPossible(&(privateData->mRxTask));
	}
else
       {
		TaskManager_ScheduleByInterruptActivation(&(privateData->mRxTask));

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
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(networkInterface!=NULL);
	CHECK_SIGNATURE(networkInterface,NETWORK_INTERFACE_SIGNATURE);
	privateData=networkInterface->mHardwareData;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(privateData!=NULL);
	CHECK_SIGNATURE(privateData,SMSC911X_PRIVATE_DATA_SIGNATURE);
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(packet!=NULL);
	SMSC_ASSERT(packet->mNext==NULL);/* We only accept one frame at a time */
	
	PacketQueue_Push(&(privateData->mTxPacketQueue),packet);
	#if 1
	if(Task_IsIdle(&(privateData->mTxTask))) 
	{
		TaskManager_ScheduleAsSoonAsPossible(&(privateData->mTxTask));
	}
	#else
             TaskManager_InterruptActivation(&(privateData->mTxTask));	
	 #endif		

	
}


void CHMID_TCPIP_ReceiveCallback(ethernet_async_t *async)
{
     struct NETWORK_INTERFACE * netp;
    NETCARD_PRIVATE_DATA * privateData= NULL;
     PPACKET_BUFFER                pkt ;
	if(async == NULL)
	{
                return ;
	}
	netp = (struct NETWORK_INTERFACE *)async->statep;
	 pkt =  (PPACKET_BUFFER)async->bufp;
     privateData= netp->mHardwareData;
    /* Pass buffer to Nexgen if buffer non 0 */
    /* ===================================== */
   if ((async->length!=0)&&(pkt!=NULL))
   	{
   		 pkt->mThisLength  = async->length;
		 pkt->mTotalLength = async->length;
		 #if 1
               RxPacketQueue_Push(&(privateData->mRxPacketQueue), pkt);
		 TaskManager_InterruptActivation(&(privateData->mRxTask));
		 #else
		         task_lock();
                       Ethernet_Input(netp,pkt);
			 task_unlock();
			 
		 #endif
   	}
	 /* Else an invalid packet has been received, drop it */
	 /* ================================================= */
	 else
	  {
	        PacketBuffer_DecreaseReference(pkt);
	  }

 /* Release the message */
 /* =================== */
 CHDRV_NET_MessageRelease(gp_RxMessage,async);
}
void CHMID_TCPIP_SendCallback(ethernet_async_t *async)
{
     struct NETWORK_INTERFACE * netp ;
     NETCARD_PRIVATE_DATA * privateData= NULL;
     PPACKET_BUFFER                pkt ;
	if(async == NULL)
	{
                return ;
	}
	netp = (struct NETWORK_INTERFACE *)async->statep;
	 pkt =  (PPACKET_BUFFER)async->bufp;
	 privateData= netp->mHardwareData;
	 /* ----------------- */
	
         /* Release async for re-use */
         /* ------------------------ */
          CHDRV_NET_MessageRelease(gp_TxMessage,async);
	   PacketBuffer_DecreaseReference(pkt);
}
/*--------------------------------------------------------------------------*/

void CHMID_TCPIP_InitTxRX(void)
{
      int i;
	/*创建接收消息对列*/
	gp_RxMessage = CHDRV_NET_CreateMessageQueue(sizeof(ethernet_async_t),MAX_TCPIP_NUM);
	if( gp_RxMessage == NULL)
	{
		return ;	
	}
	   /* Fill up all the messages */
	for (i=0;i<MAX_TCPIP_NUM;i++)
	{
	 ethernet_async_t *async=CHDRV_NET_ClaimMessageTimeout(gp_RxMessage,CHDRV_NET_TIMEOUT_INFINITY);
	 async->callback = CHMID_TCPIP_ReceiveCallback;
	 async->statep   = &smscNetworkInterface;
	 CHDRV_NET_MessageRelease(gp_RxMessage,async);
	}
		/*创建发送消息对列*/
	gp_TxMessage = CHDRV_NET_CreateMessageQueue(sizeof(ethernet_async_t),MAX_TCPIP_NUM);
	if( gp_TxMessage == NULL)
	{
		return ;	
	}
	   /* Fill up all the messages */
	for (i=0;i<MAX_TCPIP_NUM;i++)
	{
	 ethernet_async_t *async=CHDRV_NET_ClaimMessageTimeout(gp_TxMessage,CHDRV_NET_TIMEOUT_INFINITY);
	 async->callback = CHMID_TCPIP_SendCallback;
	 async->statep   = &smscNetworkInterface;
	 CHDRV_NET_MessageRelease(gp_TxMessage,async);
	}

}


 void CHMID_TCPIP_InputTask(void *arg)
{
	CHDRV_NET_RESULT_e ret = CHDRV_NET_FAIL;
	struct NETWORK_INTERFACE * netp = (struct NETWORK_INTERFACE *)arg;
	NETCARD_PRIVATE_DATA * privateData= NULL;
       int                                    i_ret;
	u16_t bufflen = 0;


	privateData= netp->mHardwareData;

       while(1)
	  {
	    PPACKET_BUFFER                pkt = NULL;


	   /* Allocate a buffer for the packet */
	   /* -------------------------------- */
	     pkt = PacketBuffer_Create(1520);

	   /* If the packet is null, go out */
	   /* ----------------------------- */
	   if (pkt==NULL)
	    {
	     CHDRV_NET_ThreadDelay(1);
	      //do_report(0,"gp_RxMessage no packet= %d\n",get_time_ms());
	     continue;
	    }
	   
	   /* -------------------------------------- */
	   //do_report(0,"start wait gp_RxMessage = %d\n",get_time_ms());
	     /* Fill the message */
	     ethernet_async_t *async=CHDRV_NET_ClaimMessageTimeout(gp_RxMessage,CHDRV_NET_TIMEOUT_INFINITY);
	     if (async==NULL)
	      {
	        PacketBuffer_DecreaseReference(pkt);
               //do_report(0," wait gp_RxMessage timeout= %d\n",get_time_ms());
	        continue;
	      }
	     async->buffer = PacketBuffer_GetStartPoint(pkt);
	     async->length = 1520;
	     async->bufp   = pkt;
		// do_report(0,"end wait gp_RxMessage = %d\n",get_time_ms());
		// CHDRV_NET_SemaphoreWait(gp_io_semaphore,-1);
	       i_ret = device_ioctl(gpch_Ethernet_Hndl_p,OSPLUS_IOCTL_ETH_ASYNC_READ,async);
		//CHDRV_NET_SemaphoreSignal(gp_io_semaphore);
		
		if(i_ret != 0)
	      {
	      do_report(0,"OSPLUS_IOCTL_ETH_ASYNC_READ error\n");
	       PacketBuffer_DecreaseReference(pkt);
	       CHDRV_NET_MessageRelease(gp_RxMessage,async);
	      }  
	  }
}

 void CHMID_TCPIP_SendTask(void *arg)
{
       PPACKET_BUFFER   pkt = NULL;
	CHDRV_NET_RESULT_e     SendResult;
	PPACKET_QUEUE queue;
	struct NETWORK_INTERFACE * netp = (struct NETWORK_INTERFACE *)arg;
	NETCARD_PRIVATE_DATA * privateData;
	int                                    i_ret;

	privateData=netp->mHardwareData;
	queue=&(privateData->mTxPacketQueue);


     /* Loop outputting packets */
      while(1)
	  {
	   	pkt = PacketQueue_Pop(queue);
		if(pkt  ==  NULL)
		{
		     //  do_report(0,"gp_TxMessage no packet = %d\n",get_time_ms());
			break;
		}

	   /* --------------------------------------- */
	     /* Fill the message */
	     ethernet_async_t *async=CHDRV_NET_ClaimMessageTimeout(gp_TxMessage,CHDRV_NET_TIMEOUT_IMMEDIATE);
	     if (async==NULL)
	      {
	         //do_report(0,"gp_TxMessage timeout = %d\n",get_time_ms());
  	         break;
	      }
	     async->buffer =  PacketBuffer_GetStartPoint(pkt);;
	     async->length =  pkt->mThisLength;
	     async->bufp   =  pkt;
		//  do_report(0,"Start write time = %d\n",get_time_ms());
	    //  CHDRV_NET_SemaphoreWait(gp_io_semaphore,-1);
	     i_ret = device_ioctl(gpch_Ethernet_Hndl_p,OSPLUS_IOCTL_ETH_ASYNC_WRITE,async);
	  //   CHDRV_NET_SemaphoreSignal(gp_io_semaphore);
		// 	  do_report(0,"end write time = %d\n",get_time_ms());
            if(i_ret != 0)
            	{
                  do_report(0,"OSPLUS_IOCTL_ETH_ASYNC_WRITE error\n");
            	}
	    
	  }
}


/*EOF*/
