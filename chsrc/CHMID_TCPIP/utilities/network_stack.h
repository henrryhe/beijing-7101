/*****************************************************************************
* Copyright 2008 SMSC, All rights reserved
*
* FILE: network_stack.h
* 
* PURPOSE:
*    This file provides headers for NetworkStack_ functions
* and NetworkInterface_ functions. The main purpose is to
* simplify the initialization process. Instead of the user 
* having to initialize each protocol layer individually,
* the NetworkStack_Initialize will initialize which ever
* protocols have been enabled in custom_options.h
*/                   

#ifndef NETWORK_STACK_H
#define NETWORK_STACK_H

#include "smsc_environment.h"

#if IPV4_ENABLED
#include "ipv4.h"
#endif

#if IPV6_ENABLED
#include "ipv6.h"
#endif

#if DHCP_ENABLED
#include "dhcp.h"
#endif

#if ETHERNET_ENABLED
#include "ethernet.h"
#endif

#if SMSC_ERROR_ENABLED
#define NETWORK_INTERFACE_SIGNATURE	(0xB3ECD2E6)
#endif

struct NETWORK_INTERFACE {       
	DECLARE_SIGNATURE
	struct NETWORK_INTERFACE * mNext;
#if IPV4_ENABLED
	IPV4_DATA mIpv4Data;
#endif
#if IPV6_ENABLED
	IPV6_DATA mIpv6Data;
#endif
	u16_t mMTU;
	
#define NETWORK_INTERFACE_FLAG_UP				(0x01)
#define NETWORK_INTERFACE_FLAG_BROADCAST		(0x02)
/*#define NETWORK_INTERFACE_FLAG_POINT_TO_POINT	(0x04)*/
#define NETWORK_INTERFACE_FLAG_DHCP				(0x08)
/**/#define NETWORK_INTERFACE_FLAG_LINK_UP		(0x10)
	u16_t mFlags;
#if DHCP_ENABLED
	PDHCP_DATA mDhcpData;
#endif

#define NETWORK_INTERFACE_NAME_SIZE	(8)
	char mName[NETWORK_INTERFACE_NAME_SIZE];
	
#define NETWORK_INTERFACE_LINK_TYPE_UNDEFINED	(0)
#define NETWORK_INTERFACE_LINK_TYPE_LOOP_BACK	(1)
#define NETWORK_INTERFACE_LINK_TYPE_ETHERNET	(2)
	u8_t mLinkType;
	union {
#if ETHERNET_ENABLED
		ETHERNET_DATA mEthernetData;
#endif
		/* Add other link types here */
	} mLinkData;
	void * mHardwareData;
};
 
extern struct NETWORK_INTERFACE * gInterfaceList;
extern struct NETWORK_INTERFACE * gDefaultInterface;

#if LOOP_BACK_ENABLED
extern struct NETWORK_INTERFACE gLoopBackInterface;
#endif

/*****************************************************************************
* FUNCTION: NetworkInterface_Initialize
* PURPOSE:
*   This function initializes a NETWORK_INTERFACE structure. It should be
*     called before the device specific initializer is called.
*****************************************************************************/
void NetworkInterface_Initialize(struct NETWORK_INTERFACE * networkInterface);

#if INLINE_SIMPLE_FUNCTIONS
#define NetworkInterface_IsUp(networkInterface)	\
	(((networkInterface)->mFlags&NETWORK_INTERFACE_FLAG_UP)!=0)
#else
int NetworkInterface_IsUp(struct NETWORK_INTERFACE * networkInterface);
#endif


/*****************************************************************************
* FUNCTION: NetworkStack_Initialize
* PURPOSE: Initializes network stack protocols and utilities, according
*    to what was enabled in custom_options.h
*****************************************************************************/
void NetworkStack_Initialize(void);

/*****************************************************************************
* FUNCTION: NetworkStack_AddInterface
* PURPOSE: 
*   Adds an initialized NETWORK_INTERFACE structure to the 
*     global NETWORK_INTERFACE list. The interface can then be used for
*     sending and receiving traffic. A device specific initializer should
*     have been called on this interface before calling this function.
* PARAMETERS:
*   networkInterface: a pointer to the NETWORK_INTERFACE structure to add
*        to the stack.
*   defaultFlag: a flag that indicates if this interface should be the default
*        interface. zero means, not default. non-zero means it is the default
*        interface. 
*****************************************************************************/
void NetworkStack_AddInterface(struct NETWORK_INTERFACE * networkInterface,u8_t defaultFlag);

void NetworkInterface_SetUp(struct NETWORK_INTERFACE * networkInterface);
void NetworkInterface_SetDown(struct NETWORK_INTERFACE * networkInterface);

#endif
