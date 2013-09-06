/* Copyright 2008 SMSC, All rights reserved
FILE: icmp_echo_tester.c
*/

#include "smsc_environment.h"
#include "protocols/ipv4.h"
#include "protocols/icmpv4.h"
#include "applications/icmp_echo_tester.h"

#if !ICMPV4_ECHO_REQUEST_ENABLED
/*Echo request support is required for this tester application */
#error ICMPV4 Echo request is not enabled
#endif

static PPACKET_BUFFER gEchoPacket=NULL;
static u8_t gEchoTimeOut=0;
static IPV4_ADDRESS gEchoDestination=0;
static u16_t gOriginalStartOffset=0;
static u16_t gOriginalLength=0;

static void IcmpEchoTester_Reply(void * param, PPACKET_BUFFER packet)
{
	if(packet==NULL) {
		SMSC_NOTICE(1,("Icmpv4EchoTester_Reply: Echo Request Timed Out"));
	} else {
		/* Check packet has incrementing data data */
		u8_t data=0;
		PPACKET_BUFFER buffer=packet;
		while(buffer!=NULL) {
			u16_t index;
			u8_t * dataPointer=PacketBuffer_GetStartPoint(buffer);
			for(index=0;index<buffer->mThisLength;index++,data++) {
				if(dataPointer[index]!=data) {
					SMSC_NOTICE(1,("Icmpv4EchoTester_Reply: Data corruption detected"));
					goto RELEASE_PACKET;
				}
			}
			buffer=buffer->mNext;
		}
		SMSC_TRACE(1,("Icmpv4EchoTester_Reply: Receive Echo Reply Correctly"));
	RELEASE_PACKET:
		PacketBuffer_DecreaseReference(packet);
	}
	
	/* We will be using this same packet again and again
		so lets hold a reference for it */
	PacketBuffer_IncreaseReference(gEchoPacket);
	
	/* Restore start offset, and length */
	gEchoPacket->mStartOffset=gOriginalStartOffset;
	gEchoPacket->mThisLength=gOriginalLength;
	gEchoPacket->mTotalLength=gOriginalLength;
	
	/* Send Echo Request */
	Icmpv4_SendEchoRequest(
		gEchoDestination,gEchoPacket,
		IcmpEchoTester_Reply,NULL,
		gEchoTimeOut);
}

void IcmpEchoTester_Initialize(IPV4_ADDRESS destination, u32_t size, u8_t timeOut)
{
	SMSC_ASSERT(gEchoPacket==NULL);
	gEchoPacket=Icmpv4_AllocatePacket(&destination,size);
	if(gEchoPacket==NULL) {
		SMSC_NOTICE(1,("Icmpv4EchoTester_Initialize: Failed to allocate gEchoPacket"));
		return;
	}
	
	/* store original start offset, and length */
	gOriginalStartOffset=gEchoPacket->mStartOffset;
	gOriginalLength=gEchoPacket->mThisLength;
	SMSC_ASSERT(gOriginalLength==gEchoPacket->mTotalLength);
	
	/* We will be using this same packet again and again
		so lets hold a reference for it */
	PacketBuffer_IncreaseReference(gEchoPacket);
	
	{/* Initialize packet with incrementing data */
		u8_t data=0;
		PPACKET_BUFFER buffer=gEchoPacket;
		while(buffer!=NULL) {
			u16_t index;
			u8_t * dataPointer=PacketBuffer_GetStartPoint(buffer);
			for(index=0;index<buffer->mThisLength;index++,data++) {
				dataPointer[index]=data;
			}
			buffer=buffer->mNext;
		}
	}	
	
	/* store period and destination */
	gEchoTimeOut=timeOut;
	gEchoDestination=destination;
	
	/* Send Echo Request */
	Icmpv4_SendEchoRequest(
		gEchoDestination,gEchoPacket,
		IcmpEchoTester_Reply,NULL,
		gEchoTimeOut);
}
