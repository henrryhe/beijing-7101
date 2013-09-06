/*******************头文件说明注释**********************************/
/*公司版权说明：版权（2007）归长虹网络科技有限公司所有。           */
/*文件名：chdrv_tcpip.h                                              */
/*版本号：V1.0                                                     */
/*作者：  曾祥根                                                   */
/*创建日期：2007-08-10                                             */
/*头文件内容概要描述（功能/变量等）：                              */
/*                                                                 */
/*修改历史：修改时间    作者      修改部分       原因              */
/*                                                                 */
/*******************************************************************/
/*****************条件编译部分**************************************/
#ifndef CHDRV_TCPIP
#define CHDRV_TCPIP
/*******************************************************************/
/**************** #include 部分*************************************/
#include "packet_manager.h"
#include "CHDRV_NET.h"
#include "task_manager.h"

/*******************************************************************/
/**************** #typedef 申明部分 ********************************/
#define       CH_BOOL               boolean           
#define		CH_RX_QUEUE

/*******************************************************************/
/***************  外部变量申明部分 *********************************/

#if (SMSC_ERROR_ENABLED)
#define SMSC911X_PRIVATE_DATA_SIGNATURE (0xB43784B8)
#endif

typedef struct NETCARD_PRIVATE_DATA_ {
	DECLARE_SIGNATURE
	mem_ptr_t mIoAddress;		/* Base address */
	int mInterfaceIndex;
	u32_t mIdRev;
	u32_t mGeneration;
#if SMSC_ERROR_ENABLED
	s32_t software_irq_signal; /* only used for testing ISR signaling */
#endif
	TASK mRxTask;
	TASK mTxTask;
	PACKET_QUEUE mTxPacketQueue;
#ifdef CH_RX_QUEUE	
	PACKET_QUEUE mRxPacketQueue;/*20081111 add*/
#endif
#if USE_PHY_WORK_AROUND
#define MIN_PACKET_SIZE (64)
	u8_t mLoopBackTxPacket[MIN_PACKET_SIZE];
	u8_t mLoopBackRxPacket[MIN_PACKET_SIZE];
	u32_t mResetCount;
#endif
} NETCARD_PRIVATE_DATA;

/*******************************************************************/
/***************  外部函数申明部分 *********************************/
void CH_Handle_RxData_Task(void *arg);
 void CH_TCPIP_TxTask(void * arg);
NETCARD_PRIVATE_DATA * EthernetCardInfo_Initialize(struct NETWORK_INTERFACE * networkInterface);
void EthernetOutput(	struct NETWORK_INTERFACE * networkInterface,PPACKET_BUFFER packet);
boolean CH_InitEthernetIO( struct NETWORK_INTERFACE *netif,int interfaceIndex);

/*******************************************************************/
#endif

