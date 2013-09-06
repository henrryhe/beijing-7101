/* Copyright 2008 SMSC, All rights reserved
File: network_stack.c
*/

#include "smsc_environment.h"
#include "network_stack.h"

#if TCP_ENABLED
#include "tcp.h"
#endif

#if UDP_ENABLED
#include "udp.h"
#endif

#if IPV4_ENABLED
#include "ipv4.h"
#endif

#if IPV6_ENABLED
#include "ipv6.h"
#endif

#if ETHERNET_ENABLED
#include "ethernet.h"
#endif

#if DHCP_ENABLED
#include "dhcp.h"
#endif

#if SOCKETS_ENABLED
#include "sockets.h"
#endif

struct NETWORK_INTERFACE * gInterfaceList=NULL;
struct NETWORK_INTERFACE * gDefaultInterface=NULL;

#if LOOP_BACK_ENABLED
#include "loop_back.h"
struct NETWORK_INTERFACE gLoopBackInterface;
#endif

void NetworkInterface_Initialize(struct NETWORK_INTERFACE * networkInterface)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(networkInterface!=NULL);
	memset(networkInterface,0,sizeof(*networkInterface));
	ASSIGN_SIGNATURE(networkInterface,NETWORK_INTERFACE_SIGNATURE);
	networkInterface->mNext=NULL;
	
	networkInterface->mFlags=
		NETWORK_INTERFACE_FLAG_BROADCAST;
}
void NetworkInterface_SetUp(struct NETWORK_INTERFACE * networkInterface)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(networkInterface!=NULL);
	CHECK_SIGNATURE(networkInterface,NETWORK_INTERFACE_SIGNATURE);
	networkInterface->mFlags|=NETWORK_INTERFACE_FLAG_UP;
}

void NetworkInterface_SetDown(struct NETWORK_INTERFACE * networkInterface)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(networkInterface!=NULL);
	CHECK_SIGNATURE(networkInterface,NETWORK_INTERFACE_SIGNATURE);
	networkInterface->mFlags&=(~(NETWORK_INTERFACE_FLAG_UP));	
}
int NetworkInterface_IsUp(struct NETWORK_INTERFACE * networkInterface)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(networkInterface!=NULL,-1);
	CHECK_SIGNATURE(networkInterface,NETWORK_INTERFACE_SIGNATURE);
	return ((networkInterface->mFlags&NETWORK_INTERFACE_FLAG_UP)!=0);
}


void NetworkStack_Initialize(void)
{
	TaskManager_Initialize();
	PacketManager_Initialize();
#if TCP_ENABLED
	Tcp_Initialize();
#endif
#if UDP_ENABLED
	Udp_Initialize();
#endif
#if IPV4_ENABLED
	Ipv4_Initialize();
#endif
#if IPV6_ENABLED
	Ipv6_Initialize();
#endif
#if ETHERNET_ENABLED
	Ethernet_Initialize();
#endif
#if LOOP_BACK_ENABLED
	NetworkInterface_Initialize(&gLoopBackInterface);
	if(LoopBack_InitializeInterface(&gLoopBackInterface)!=ERR_OK) {
		SMSC_ERROR(("Failed to initialize loop back interface"));
	}
	Ipv4_SetAddresses(&gLoopBackInterface,
		IPV4_ADDRESS_MAKE(127,0,0,1),
		IPV4_ADDRESS_MAKE(255,255,255,255),
		IPV4_ADDRESS_MAKE(0,0,0,0));
	NetworkInterface_SetUp(&gLoopBackInterface);
	NetworkStack_AddInterface(&gLoopBackInterface,1);
#endif

#if SOCKETS_ENABLED
	Sockets_Initialize();
#endif
}

void NetworkStack_AddInterface(struct NETWORK_INTERFACE * interface,u8_t defaultFlag)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(interface!=NULL);
	SMSC_ASSERT(interface->mNext==NULL);
	interface->mNext=gInterfaceList;
	gInterfaceList=interface;
	if((gDefaultInterface==NULL)||(defaultFlag))
	{
		/* make this interface the default interface. */
		gDefaultInterface=interface;
	}
}

