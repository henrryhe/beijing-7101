/***************************************************************************
 *
 * Copyright (C) 2008  SMSC
 *
 * All rights reserved.
 *
 ***************************************************************************
 * File: loop_back.c
 */

#include "smsc_environment.h"

#if !LOOP_BACK_ENABLED
#error LOOP_BACK is not enabled
#endif

#include "loop_back.h"
#include "network_stack.h"

/* these are used as a tag to make sure the loop back
 interface passes the packets back up to the correct
 protocol */
#define LOOP_BACK_TYPE_IPV4		(1)
#define LOOP_BACK_TYPE_IPV6		(2)

static void LoopBack_RxTask(void * arg);

#if IPV4_ENABLED
#include "ipv4.h"
static void LoopBack_Ipv4Output(
	struct NETWORK_INTERFACE * networkInterface,
	PPACKET_BUFFER packet,
	IPV4_ADDRESS destinationAddress);
#endif
#if IPV6_ENABLED           
#include "ipv6.h"
static void LoopBack_Ipv6Output(
	struct NETWORK_INTERFACE * networkInterface,
	PPACKET_BUFFER packet);
#endif

static LOOP_BACK_PRIVATE_DATA gLoopBackPrivateData;

err_t LoopBack_InitializeInterface(struct NETWORK_INTERFACE * networkInterface)
{
	err_t result=ERR_OK;
	PLOOP_BACK_PRIVATE_DATA privateData;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(networkInterface!=NULL,ERR_ARG);
	CHECK_SIGNATURE(networkInterface,NETWORK_INTERFACE_SIGNATURE);
	
	SMSC_TRACE(LOOP_BACK_DEBUG,("Initializing Loop Back interface, for use with the SMSC IP Stack"));
	SMSC_TRACE(LOOP_BACK_DEBUG,("Driver Compiled: %s, %s",__DATE__,__TIME__));
	
	networkInterface->mLinkType=NETWORK_INTERFACE_LINK_TYPE_LOOP_BACK;
	
	privateData=&gLoopBackPrivateData;
	
	memset(privateData,0,sizeof(*privateData));
	ASSIGN_SIGNATURE(privateData,LOOP_BACK_PRIVATE_DATA_SIGNATURE);
	networkInterface->mHardwareData=privateData;

	/* Initialize Rx Task */
	Task_Initialize(&(privateData->mRxTask),
		TASK_PRIORITY_RX,LoopBack_RxTask,networkInterface);
	
	PacketQueue_Initialize(&(privateData->mPacketQueue));
    
    SMSC_ASSERT(MINIMUM_MTU<=1500);
    networkInterface->mMTU=1500;
    sprintf(networkInterface->mName,"LOOP");
    SMSC_ASSERT(strlen(networkInterface->mName)<NETWORK_INTERFACE_NAME_SIZE);

#if IPV4_ENABLED
    Ipv4_InitializeInterface(networkInterface,LoopBack_Ipv4Output);
#endif
#if IPV6_ENABLED
	Ipv6_InitializeInterface(networkInterface,LoopBack_Ipv6Output);
#endif
 
	return result;
}

/* RX task to receive packets */
static void LoopBack_RxTask(void *arg)
{
	struct NETWORK_INTERFACE * networkInterface = (struct NETWORK_INTERFACE *)arg;
	LOOP_BACK_PRIVATE_DATA * privateData;
	u16_t receiveCount=0;
	u32_t queueEmpty=0;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(networkInterface!=NULL);
	CHECK_SIGNATURE(networkInterface,NETWORK_INTERFACE_SIGNATURE);
	privateData=networkInterface->mHardwareData;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(privateData!=NULL);
	CHECK_SIGNATURE(privateData,LOOP_BACK_PRIVATE_DATA_SIGNATURE);
	queueEmpty=PacketQueue_IsEmpty(&(privateData->mPacketQueue));
	while((receiveCount<LOOP_BACK_RX_BURST_COUNT)&&(!queueEmpty)) {
		/* Read a Packet */
		PPACKET_BUFFER packet=PacketQueue_Pop(&(privateData->mPacketQueue));
		if(packet!=NULL) {
			u8_t packetType=PacketBuffer_GetStartPoint(packet)[0];
			if(PacketBuffer_MoveStartPoint(packet,1)!=ERR_OK) {
				/* this should never happen because all packet push in the
				queue have been prepended with a one byte packet type code */
				SMSC_ERROR(("LoopBack_RxTask: unable to move start point"));
			}
			switch(packetType)
			{
#if IPV4_ENABLED
			case LOOP_BACK_TYPE_IPV4:
				Ipv4_Input(networkInterface,packet);
				break;
#endif
#if IPV6_ENABLED
			case LOOP_BACK_TYPE_IPV6:
				SMSC_ERROR(("LoopBack_RxTask: IPV6 is not implemented"));
				break;
#endif
			default:
				SMSC_NOTICE(LOOP_BACK_DEBUG,("LoopBack_RxTask: Unknown packet type"));
				PacketBuffer_DecreaseReference(packet);
			}
		} else {
			queueEmpty=1;
		}
		receiveCount++;
	}
	if(!queueEmpty) {
		/* Packets are still available */
		/* Reschedule this task to run as soon as possible */
		if(Task_IsIdle(&(privateData->mRxTask))) {
			/* we make sure task state is idle because it may have already been 
			scheduled during packet processing. For example if we ping our self,
			the initial request will generate an immediate response which would have
			already scheduled this task */
			TaskManager_ScheduleAsSoonAsPossible(&(privateData->mRxTask));
		}
	}
}

static void LoopBack_Output(
	struct NETWORK_INTERFACE * networkInterface,
	PPACKET_BUFFER packet,
	u8_t packetType)
{
	LOOP_BACK_PRIVATE_DATA * privateData;
	PPACKET_BUFFER packetCopy;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(networkInterface!=NULL);
	CHECK_SIGNATURE(networkInterface,NETWORK_INTERFACE_SIGNATURE);
	privateData=networkInterface->mHardwareData;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(privateData!=NULL);
	CHECK_SIGNATURE(privateData,LOOP_BACK_PRIVATE_DATA_SIGNATURE);
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(packet!=NULL);
	if(PacketBuffer_MoveStartPoint(packet,-1)!=ERR_OK) {
		SMSC_ERROR(("LoopBack_Ipv4Output: Unabled to move start point"));
		goto DROP_PACKET;
	}
	PacketBuffer_GetStartPoint(packet)[0]=packetType;
	
	/* NOTE: We need to copy this packet because the upper layers
		may assume ownership returns to them after transmit.
		If we simply push this packet on the receive queue
		then the receive queue holds ownership and the assumption
		is broken */
	packetCopy=PacketBuffer_Copy(packet);
	
	if(packetCopy!=NULL) {
		PacketQueue_Push(&(privateData->mPacketQueue),packetCopy);
		if(Task_IsIdle(&(privateData->mRxTask))) {
			TaskManager_ScheduleAsSoonAsPossible(&(privateData->mRxTask));
		}
	} else {
		SMSC_NOTICE(LOOP_BACK_DEBUG, ("LoopBack_Output: Unable to copy, dropping packet"));
	}
DROP_PACKET:
	PacketBuffer_DecreaseReference(packet);
	return;
}

#if IPV4_ENABLED
static void LoopBack_Ipv4Output(
	struct NETWORK_INTERFACE * networkInterface,
	PPACKET_BUFFER packet,
	IPV4_ADDRESS destinationAddress)
{
	LoopBack_Output(networkInterface,packet,LOOP_BACK_TYPE_IPV4);
}
#endif /* IPV4_ENABLED */
#if IPV6_ENABLED
static void LoopBack_Ipv6Output(
	struct NETWORK_INTERFACE * networkInterface,
	PPACKET_BUFFER packetBuffer)
{
	LoopBack_Output(networkInterface,packet,LOOP_BACK_TYPE_IPV6);
}
#endif /* IPV6_ENABLED */
