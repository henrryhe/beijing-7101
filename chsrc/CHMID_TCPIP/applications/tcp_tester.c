/* Copyright 2008 SMSC, All rights reserved
FILE: tcp_tester.c
*/

#include "smsc_environment.h"

#if !TCP_ENABLED
#error TCP is not enabled
#endif

#include "applications/tcp_tester.h" 
#include "utilities/packet_manager.h"
#include "protocols/tcp.h"

void TcpTester_Forever(void * param);

static void TcpTester_ReceiverError(void *arg, err_t err);

#define USE_INCREMENTING_DATA	(0)

err_t TcpTester_ReceiveFunction(
	void * param, struct TCP_CONTROL_BLOCK * tcpControlBlock,
	PPACKET_BUFFER packet,err_t err)
{
	PTCP_RX_TESTER tcpTester=(PTCP_RX_TESTER)param;
	PPACKET_BUFFER buffer=NULL;
	u8_t * data=NULL;
	u8_t testValue;
	PPACKET_BUFFER txPacket;
	u8_t *dataPointer;
	static u32_t tcpMsgCount;
	struct stat {
		u32_t msgReceived;
		double secRecvTime;
	} stat;
	static smsc_clock_t start_time;
	
	SMSC_ASSERT(tcpTester!=NULL);
	CHECK_SIGNATURE(tcpTester,TCP_RX_TESTER_SIGNATURE);
	SMSC_ASSERT(tcpControlBlock!=NULL);
	if(packet==NULL) {
		/* This indicates the connection is closed
		   tcpControlBlock will be freed upon return */
		SMSC_TRACE(1,("TCP_RX_TESTER: Connection Closed"));
		Tcp_Close(tcpControlBlock);
		return ERR_OK;
	}
	SMSC_ASSERT(packet->mThisLength>0);
	
	data=PacketBuffer_GetStartPoint(packet);
	
	testValue=tcpTester->mNextIncrementValue;
	buffer=packet;
#if !USE_INCREMENTING_DATA
   	while(buffer!=NULL) {
		u8_t * data=PacketBuffer_GetStartPoint(buffer);
		tcpMsgCount++;
		if((tcpMsgCount%1000)==0) {
			SMSC_TRACE(1,("TcpTester_ReceiveFunction: received %"U32_F" segments",tcpMsgCount));
		}
		/*SMSC_TRACE(1,("TcpTester_ReceiveFunction: buffer received"));*/
		if(strncmp((char *)data,(char *)"SSSSSS",6)==0) {
			tcpMsgCount=0;
			SMSC_TRACE(1,("***Received START MARKER***"));
			start_time=smsc_time_now();
			goto DONE;		
		}
		if(strncmp((char *)data,(char *)"EEEEEE",6)==0) {
			u32_t recvTime=smsc_tick_to_sec(smsc_time_minus(smsc_time_now(),start_time));
			SMSC_TRACE(1,("***Received END MARKER*** tcpMsgCount=%"U32_F,tcpMsgCount - 1));
			txPacket=Tcp_AllocateDataBuffers(tcpControlBlock,sizeof(stat));
			dataPointer=PacketBuffer_GetStartPoint(txPacket);
			stat.msgReceived=tcpMsgCount - 1;
			stat.secRecvTime=(double)recvTime;
			SMSC_TRACE(1,("msgReceived=%"U32_F"  secRecvTime=%"U32_F, stat.msgReceived, recvTime)); 
			memcpy(dataPointer, (char *) &stat, sizeof(stat));
			txPacket=Tcp_QueueData(tcpControlBlock,txPacket);		
			Tcp_SendQueuedData(tcpControlBlock);
		}
		buffer=buffer->mNext;
	}
#else	/* Verify data packet */		
	while(buffer!=NULL) {
		u16_t index=0; 
		u8_t startDataCount=0;
		u8_t endDataCount=0;
		for(index=0;index<buffer->mThisLength;testValue++,index++) {
			if(data[index]!=testValue) {   
				if(data[index]=='S') {
					if(++startDataCount==5) {
						SMSC_TRACE(1,("***Received START MARKER*** buf=%c", data[index]));	
						start_time=smsc_time_now();
						goto DONE;
					}
					tcpMsgCount=0;
					continue;
				}
				if(data[index]=='E') {
					if (++endDataCount==4){
						u32_t recvTime=smsc_tick_to_sec(smsc_time_minus(smsc_time_now(),start_time));
						SMSC_TRACE(1,("***Received END MARKER*** buf=%c udpMsgCount=%"U32_F, data[index],tcpMsgCount - 1));
						txPacket=Tcp_AllocateDataBuffers(tcpControlBlock,sizeof(stat));
						dataPointer=PacketBuffer_GetStartPoint(txPacket);
						stat.msgReceived=tcpMsgCount - 1;
						stat.secRecvTime=(double)recvTime;
						SMSC_TRACE(1,("msgReceived=%"U32_F"  secRecvTime=%"U32_F, stat.msgReceived, recvTime)); 
						memcpy(dataPointer, (char *) &stat, sizeof(stat));
						txPacket=Tcp_QueueData(tcpControlBlock,txPacket);		
						Tcp_SendQueuedData(tcpControlBlock);
					}
					continue;
                 }
				SMSC_WARNING(1,("TCP_RX_TESTER: Invalid Data Found, Expecting Incrementing pattern, msgCount=%"U32_F, tcpMsgCount));
				goto DONE;
			}
		}
		buffer=buffer->mNext;
	}   
	tcpTester->mNextIncrementValue=testValue;
	SMSC_TRACE(1,("TCP_RX_TESTER: Verified Incrementing Data Pattern, msgCount=%"U32_F, tcpMsgCount));
#endif
DONE:
	SMSC_ASSERT(!((packet->mTotalLength)&0xFFFF0000));
	Tcp_AcknowledgeReceivedData(tcpControlBlock,((u16_t)(packet->mTotalLength)));
	PacketBuffer_DecreaseReference(packet);
	return ERR_OK;
}

void TcpTester_ReceiverError(void *arg, err_t err)
{
	PTCP_RX_TESTER tcpTester=(PTCP_RX_TESTER)arg;
	SMSC_ASSERT(tcpTester!=NULL);
	CHECK_SIGNATURE(tcpTester,TCP_RX_TESTER_SIGNATURE);
	
	SMSC_WARNING(1,("TCP_RX_TESTER: Connection Error, err=%"S16_F,(s16_t)err));
}

err_t TcpTester_AcceptFunction(void * arg,struct TCP_CONTROL_BLOCK * newControlBlock, err_t err)
{
	PTCP_RX_TESTER tcpTester=(PTCP_RX_TESTER)arg;
	SMSC_ASSERT(tcpTester!=NULL);
	CHECK_SIGNATURE(tcpTester,TCP_RX_TESTER_SIGNATURE);
	SMSC_ASSERT(newControlBlock!=NULL);
	Tcp_SetErrorCallBack(newControlBlock,TcpTester_ReceiverError);
	Tcp_SetReceiveCallBack(newControlBlock,TcpTester_ReceiveFunction);
	tcpTester->mNextIncrementValue = 0;
	SMSC_TRACE(1,("TCP_RX_TESTER: Accepted New Connection (%p)",(void *)newControlBlock));
	return ERR_OK;
}

void TcpTester_ListenerError(void *arg, err_t err)
{
	PTCP_RX_TESTER tcpTester=(PTCP_RX_TESTER)arg;
	SMSC_ASSERT(tcpTester!=NULL);
	CHECK_SIGNATURE(tcpTester,TCP_RX_TESTER_SIGNATURE);
	
	SMSC_WARNING(1,("TCP_RX_TESTER: Listener Error, err=%"S16_F,(s16_t)err));
}

void TcpTester_InitializeReceiver(
	PTCP_RX_TESTER tcpTester,
	PIP_ADDRESS localAddress,u16_t localPort)
{
	struct TCP_CONTROL_BLOCK * tcpControlBlock;
	SMSC_ASSERT(tcpTester!=NULL);
	memset(tcpTester,0,sizeof(TCP_RX_TESTER));
	ASSIGN_SIGNATURE(tcpTester,TCP_RX_TESTER_SIGNATURE);
	tcpControlBlock=Tcp_NewControlBlock();
	if(tcpControlBlock==NULL) {
		SMSC_WARNING(1,("TCP_RX_TESTER: unable to allocate tcpControlBlock for listener"));
		return;
	}
	tcpTester->mTcpControlBlock=tcpControlBlock;
	tcpTester->mNextIncrementValue=0;
	Tcp_SetCallBackArgument(tcpControlBlock,tcpTester);
	Tcp_SetAcceptCallBack(tcpControlBlock,TcpTester_AcceptFunction);
	Tcp_SetErrorCallBack(tcpControlBlock,TcpTester_ListenerError);
	if(Tcp_Bind(tcpControlBlock,localAddress,localPort)!=ERR_OK) {
		SMSC_WARNING(1,("TCP_RX_TESTER: unable to bind tcpControlBlock for listener"));
		Tcp_FreeControlBlock(tcpControlBlock);
		return;
	}
	Tcp_Listen(tcpControlBlock);
}

/*********************************************************************
		TRANSMITTER CODE BELOW / RECEIVER CODE ABOVE
*********************************************************************/

void TcpTester_Connect(PTCP_TX_TESTER tcpTester); 
void TcpTester_TransmitterError(void *arg, err_t err);

void TcpTester_SendData(struct TCP_CONTROL_BLOCK * tcpControlBlock)
{
	PPACKET_BUFFER packet;
	u16_t maximumSegmentSize;
	u16_t sendCount=0;
	SMSC_ASSERT(tcpControlBlock!=NULL);
	CHECK_SIGNATURE(tcpControlBlock,TCP_CONTROL_BLOCK_SIGNATURE);
	maximumSegmentSize=Tcp_GetMaximumSegmentSize(tcpControlBlock);
	SMSC_ASSERT(maximumSegmentSize>0);
	while((packet=Tcp_AllocateDataBuffers(tcpControlBlock,maximumSegmentSize))!=NULL)
	{
		packet=Tcp_QueueData(tcpControlBlock,packet);
		if(packet!=NULL) {
			/*SMSC_NOTICE(1,("TCP_TX_TESTER: Unable to queue packet"));*/
			PacketBuffer_DecreaseReference(packet);
			packet=NULL;
			break;
		/*} else {
			SMSC_TRACE(1,("TCP_TX_TESTER: queued 1 packet"));*/
		}
		sendCount++;
	}
	if(sendCount>0) {
		/*SMSC_TRACE(1,("TCP_TX_TESTER: sending %"U16_F" packets",sendCount));*/
		Tcp_SendQueuedData(tcpControlBlock);
	}
}

err_t TcpTester_Connected(
	void *arg,
	struct TCP_CONTROL_BLOCK * tcpControlBlock,
	err_t err)
{
	PTCP_TX_TESTER tcpTester=(PTCP_TX_TESTER)arg;
	SMSC_ASSERT(tcpTester!=NULL);
	CHECK_SIGNATURE(tcpTester,TCP_TX_TESTER_SIGNATURE);
	/* SMSC_TRACE(1,("TCP_TX_TESTER: Connection Established")); */
	
	SMSC_ASSERT(tcpTester->mTcpControlBlock==tcpControlBlock);

	TcpTester_SendData(tcpTester->mTcpControlBlock);
	
	if(!(tcpTester->mContinuousMode)) {
		SMSC_TRACE(1,("TCP_TX_TESTER: Closing Connection"));
		Tcp_Close(tcpTester->mTcpControlBlock);
		tcpTester->mTcpControlBlock=NULL;

		TcpTester_Connect(tcpTester);
	}

	return ERR_OK;
}

/* Function to be called when more send buffer space is available. */
err_t TcpTester_SentCallBack(void *arg, struct TCP_CONTROL_BLOCK * tcpControlBlock, u16_t space)
{
	PTCP_TX_TESTER tcpTester=(PTCP_TX_TESTER)arg;
	SMSC_ASSERT(tcpTester!=NULL);
	CHECK_SIGNATURE(tcpTester,TCP_TX_TESTER_SIGNATURE);
	SMSC_ASSERT(tcpTester->mTcpControlBlock==tcpControlBlock);
	SMSC_ASSERT(tcpTester->mContinuousMode);
	TcpTester_SendData(tcpControlBlock);
}

void TcpTester_Connect(PTCP_TX_TESTER tcpTester)
{
	SMSC_ASSERT(tcpTester!=NULL);
	CHECK_SIGNATURE(tcpTester,TCP_TX_TESTER_SIGNATURE);

	SMSC_ASSERT(tcpTester->mTcpControlBlock==NULL);

	tcpTester->mTcpControlBlock=Tcp_NewControlBlock();
	if(tcpTester->mTcpControlBlock==NULL) {
		SMSC_WARNING(1,("TCP_TX_TESTER: Failed to create tcpControlBlock"));
		return;
	}
	Tcp_SetCallBackArgument(tcpTester->mTcpControlBlock,tcpTester);
	Tcp_SetErrorCallBack(tcpTester->mTcpControlBlock,TcpTester_TransmitterError);
	if(tcpTester->mContinuousMode) {
		Tcp_SetSentCallBack(tcpTester->mTcpControlBlock,TcpTester_SentCallBack);
	}
	if(Tcp_Connect(tcpTester->mTcpControlBlock,&(tcpTester->mRemoteAddress),
		tcpTester->mRemotePort,TcpTester_Connected)!=ERR_OK) 
	{
		SMSC_WARNING(1,("TCP_TX_TESTER: tcp_connect failed"));
		Tcp_FreeControlBlock(tcpTester->mTcpControlBlock);
		tcpTester->mTcpControlBlock=NULL;
	}
}

void TcpTester_TransmitterError(void *arg, err_t err)
{
	PTCP_TX_TESTER tcpTester=(PTCP_TX_TESTER)arg;
	SMSC_ASSERT(tcpTester!=NULL);
	CHECK_SIGNATURE(tcpTester,TCP_TX_TESTER_SIGNATURE);

	SMSC_WARNING(1,("TCP_TX_TESTER: Connection Error, err=%"S16_F,(s16_t)err));
}

void TcpTester_InitializeTransmitter(
	PTCP_TX_TESTER tcpTester,
	PIP_ADDRESS remoteAddress,u16_t remotePort,
	u8_t continuousMode)
{
	SMSC_ASSERT(tcpTester!=NULL);
	memset(tcpTester,0,sizeof(TCP_TX_TESTER));
	ASSIGN_SIGNATURE(tcpTester,TCP_TX_TESTER_SIGNATURE);
	SMSC_ASSERT(remoteAddress!=NULL);
	tcpTester->mTcpControlBlock=NULL;
	IP_ADDRESS_COPY(&(tcpTester->mRemoteAddress),remoteAddress);
	tcpTester->mRemotePort=remotePort;
	tcpTester->mContinuousMode=continuousMode;
	
	TcpTester_Connect(tcpTester);
}
static TCP_TX_TESTER tcpReceiver;
static TCP_TX_TESTER tcpTransmitter;

void ch_testTcp()
{
	{
		IP_ADDRESS ipAddress;
		IP_ADDRESS_SET_IPV4(&ipAddress,127,0,0,1);
		TcpTester_InitializeReceiver(
			&tcpReceiver,
			&ipAddress,/* local address */
			6000);/* local port */
	}
	{
		IP_ADDRESS ipAddress;
		IP_ADDRESS_SET_IPV4(&ipAddress,127,0,0,1);
		TcpTester_InitializeTransmitter(
			&tcpTransmitter,
			&ipAddress,	/* remote address */
			6000, 		/* remote port */
			1);         /* continuous mode, 0==disable, 1==enable */
	}
}
/*eof*/
