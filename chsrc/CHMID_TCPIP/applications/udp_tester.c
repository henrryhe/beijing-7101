/* Copyright 2008 SMSC, All rights reserved
FILE: udp_tester.c
*/

#include "smsc_environment.h"

#if !UDP_ENABLED
#error UDP is not enabled
#endif

#include "applications/udp_tester.h" 
#include "utilities/packet_manager.h"
#include "protocols/udp.h"

#if SMSC_ERROR_ENABLED
#define UDP_RX_TESTER_SIGNATURE 	(0xFB348732)
#endif
typedef struct UDP_RX_TESTER_ {
	DECLARE_SIGNATURE
	struct UDP_CONTROL_BLOCK * mUdpControlBlock;
} UDP_RX_TESTER, * PUDP_RX_TESTER;

void UdpTester_InitializeReceiver(
	PUDP_RX_TESTER udpTester,
	PIP_ADDRESS localAddress,u16_t localPort);

#if SMSC_ERROR_ENABLED
#define UDP_TX_TESTER_SIGNATURE 	(0xF83783CD)
#endif
typedef struct UDP_TX_TESTER_ {
	DECLARE_SIGNATURE
	struct UDP_CONTROL_BLOCK * mUdpControlBlock;
	TASK mTransmitTask;
	u32_t mPeriod;
	IP_ADDRESS mRemoteAddress;
	u16_t mRemotePort;
	u32_t mPacketSize;
} UDP_TX_TESTER, * PUDP_TX_TESTER;



 UDP_RX_TESTER udpReceiver;
 UDP_TX_TESTER udpTransmitter;

#define USE_INCREMENTING_DATA 	(0)
#define UDP_TESTER_TX_BURST_COUNT	(4)


void UdpTester_ReceiveFunction(
	void * param, struct UDP_CONTROL_BLOCK * udpControlBlock,
	PPACKET_BUFFER packet,
	PIP_ADDRESS sourceAddress,
	u16_t sourcePort)
{
	PPACKET_BUFFER buffer;
	PPACKET_BUFFER txPacket;
	u8_t *dataPointer;
	static u32_t udpMsgCount;
	struct stat {
		u32_t msgReceived;
		double secRecvTime;
	} stat;
	static smsc_clock_t start_time;
	
	SMSC_ASSERT(packet!=NULL);
	CHECK_SIGNATURE(packet,PACKET_BUFFER_SIGNATURE);
	buffer=packet; 
	udpMsgCount++;
	if((udpMsgCount%1000)==0) {
		SMSC_TRACE(1,("UdpTester_ReceiveFunction: udpMsgCount=%"U32_F, udpMsgCount));
	}
#if !USE_INCREMENTING_DATA		
	if (buffer!=NULL) {
		u8_t * data=PacketBuffer_GetStartPoint(buffer);				
		if(strncmp((char *)data,(char *)"SSSSSS",6)==0) {
			udpMsgCount=0;
			SMSC_TRACE(1,("***Received START MARKER***"));
			start_time=smsc_time_now();
			goto DROP_PACKET;			
		}
		if(strncmp((char *)data,(char *)"EEEEEE",6)==0) {
			u32_t recvTime=smsc_tick_to_sec(smsc_time_minus(smsc_time_now(),start_time));
			SMSC_TRACE(1,("***Received END MARKER*** udpMsgCount=%"U32_F,udpMsgCount - 1));
			txPacket=Udp_AllocatePacket(sourceAddress,sizeof(stat));
			dataPointer=PacketBuffer_GetStartPoint(txPacket);
			stat.msgReceived=udpMsgCount - 1;
			stat.secRecvTime=(double)recvTime;
			SMSC_TRACE(1,("msgReceived=%"U32_F"  msecRecvTime=%"U32_F, stat.msgReceived, recvTime)); 
			memcpy(dataPointer, (char *) &stat, sizeof(stat));
			Udp_SendTo(udpControlBlock,txPacket,sourceAddress,sourcePort);
			goto DROP_PACKET;			
		}
	} 
#else	/* Verify the data */		
	while(buffer!=NULL) {
		u8_t firstByte=1;
		u8_t testData;
		u8_t startDataCount=0;
		u8_t endDataCount=0;
		u32_t index=0;
		u8_t * data=PacketBuffer_GetStartPoint(buffer);
		u32_t length=buffer->mThisLength;
		while(index<length) {
			if(firstByte) {
				testData=data[index];
				firstByte=0;
			} else if(testData!=data[index]) {
				if(data[index]=='S') {
					if(++startDataCount==4) {
						SMSC_TRACE(1,("***Received START MARKER*** buf=%c", data[index]));	
						start_time=smsc_time_now();
					}
					udpMsgCount=0;
					testData++;
					index++;
					continue;
				}
				if(data[index]=='E') {
					if (++endDataCount==4) {
						u32_t recvTime=smsc_tick_to_sec(smsc_time_minus(smsc_time_now(),start_time));
						SMSC_TRACE(1,("***Received END MARKER*** buf=%c udpMsgCount=%"U32_F, data[index],udpMsgCount - 1));
						txPacket=Udp_AllocatePacket(sourceAddress,sizeof(stat));
						dataPointer=PacketBuffer_GetStartPoint(txPacket);
						stat.msgReceived=udpMsgCount - 1;
						stat.secRecvTime=(double)recvTime;
						SMSC_TRACE(1,("msgReceived=%"U32_F"  msecRecvTime=%"U32_F, stat.msgReceived, recvTime)); 
						memcpy(dataPointer, (char *) &stat, sizeof(stat));
						Udp_SendTo(udpControlBlock,txPacket,sourceAddress,sourcePort);
						goto DROP_PACKET;
					}
					testData++;
					index++;
					continue;
				}
				SMSC_NOTICE(1,("UdpTester_ReceiveFunction: Detected non incrementing data, msgcount=%"U32_F" buf=%c", udpMsgCount,data[index]));
				goto DROP_PACKET;
			}
			testData++;
			index++;
		}
		buffer=buffer->mNext;
	}   
	SMSC_TRACE(1,("UdpTester_ReceiveFunction: Verified incrementing data, msgcount=%"U32_F, udpMsgCount));
#endif
DROP_PACKET:
	PacketBuffer_DecreaseReference(packet);
}
void UdpTester_InitializeReceiver(
	PUDP_RX_TESTER udpTester,
	PIP_ADDRESS localAddress,u16_t localPort)
{    
	struct UDP_CONTROL_BLOCK * udpControlBlock;
	SMSC_ASSERT(udpTester!=NULL);
	memset(udpTester,0,sizeof(UDP_RX_TESTER));
	ASSIGN_SIGNATURE(udpTester,UDP_RX_TESTER_SIGNATURE);
	SMSC_ASSERT(localAddress!=NULL);
	        
	udpControlBlock=Udp_CreateControlBlock();
	if(udpControlBlock==NULL) {
		SMSC_NOTICE(1,("UdpTester_InitializeReceiver: Failed to create UDP_CONTROL_BLOCK"));
		return;
	}
	if(Udp_Bind(udpControlBlock,localAddress,localPort)!=ERR_OK) {
		SMSC_NOTICE(1,("UdpTester_InitializeReceiver: Failed to Bind UDP ControlBlock"));
		Udp_FreeControlBlock(udpControlBlock);
		return;
	}                                         
	Udp_SetReceiverCallBack(udpControlBlock,UdpTester_ReceiveFunction,udpTester);
	udpTester->mUdpControlBlock=udpControlBlock;
}

void UdpTester_TransmitTask(void * param)
{
	PUDP_TX_TESTER udpTester=(PUDP_TX_TESTER)param;
	struct UDP_CONTROL_BLOCK * udpControlBlock;
	PPACKET_BUFFER packet; 
	int count=0;
	SMSC_ASSERT(udpTester!=NULL);
	CHECK_SIGNATURE(udpTester,UDP_TX_TESTER_SIGNATURE);

	udpControlBlock=udpTester->mUdpControlBlock;
	SMSC_ASSERT(udpControlBlock!=NULL);

	while(count<UDP_TESTER_TX_BURST_COUNT) {
		packet=Udp_AllocatePacket(&(udpTester->mRemoteAddress),udpTester->mPacketSize);
		if(packet==NULL) {
			SMSC_NOTICE(1,("UdpTester_TransmitTask: Failed to allocate packet buffer"));
			goto RESCHEDULE;
		} else {
			/*SMSC_TRACE(1,("UdpTester_TransmitTask: Transmitting UDP Packet"));*/
		}

		#if USE_INCREMENTING_DATA
		{/* Initialize packet with incrementing data */
			u8_t data=0;
			PPACKET_BUFFER buffer=packet;
			while(buffer!=NULL) {
				u16_t index;
				u8_t * dataPointer=PacketBuffer_GetStartPoint(buffer);
				for(index=0;index<buffer->mThisLength;index++,data++) {
					dataPointer[index]=data;
				}
				buffer=buffer->mNext;
			}
		}
		#endif

		Udp_SendTo(
			udpControlBlock,packet,
			&(udpTester->mRemoteAddress),udpTester->mRemotePort);
		count++;
	}
RESCHEDULE:
	TaskManager_ScheduleAsSoonAsPossible(&(udpTester->mTransmitTask));
}

void UdpTester_InitializeTransmitter(
	PUDP_TX_TESTER udpTester,
	PIP_ADDRESS remoteAddress,u16_t remotePort,
	u32_t size)
{
	struct UDP_CONTROL_BLOCK * udpControlBlock;
	SMSC_ASSERT(udpTester!=NULL);
	memset(udpTester,0,sizeof(UDP_TX_TESTER));
	ASSIGN_SIGNATURE(udpTester,UDP_TX_TESTER_SIGNATURE);
	SMSC_ASSERT(remoteAddress!=NULL);
	        
	udpControlBlock=Udp_CreateControlBlock();
	if(udpControlBlock==NULL) {
		SMSC_NOTICE(1,("UdpTester_InitializeTransmitter: Failed to create UDP_CONTROL_BLOCK"));
		return;
	}
	
	IP_ADDRESS_COPY(&(udpTester->mRemoteAddress),remoteAddress);
	udpTester->mRemotePort=remotePort;
	udpTester->mPacketSize=size;
	udpTester->mUdpControlBlock=udpControlBlock;
	Task_Initialize(&(udpTester->mTransmitTask),
		TASK_PRIORITY_APPLICATION_LOW,/* Priority is low to ensure that the RX task is not choked */
		UdpTester_TransmitTask,udpTester);
	TaskManager_ScheduleAsSoonAsPossible(&(udpTester->mTransmitTask));
}


void  chaUdptest()
{

       	{
		IP_ADDRESS ipAddress;
		IP_ADDRESS_SET_IPV4(&ipAddress,127,0,0,1);
		UdpTester_InitializeReceiver(
			&udpReceiver,
			&ipAddress,	/* local address */
			6000);		/* local port */
       		}
       		{
		IP_ADDRESS ipAddress;
		IP_ADDRESS_SET_IPV4(&ipAddress,127,0,0,1);
		UdpTester_InitializeTransmitter(
			&udpTransmitter,
			&ipAddress,	/* remote address */
			6000, 		/* remote port */
			1472);		/* payload size */
       			}
	
}
/*eof*/
