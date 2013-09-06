/******************************************************************************/
/*    Copyright (c) 2005 iPanel Technologies, Ltd.                     */
/*    All rights reserved. You are not allowed to copy or distribute          */
/*    the code without permission.                                            */
/*    This is the demo implenment of the Porting APIs needed by iPanel        */
/*    MiddleWare.                                                             */
/*    Maybe you should modify it accorrding to Platform.                      */
/*                                                                            */
/*    $author huzh 2007/11/20                                                 */
/******************************************************************************/

#ifndef _IPANEL_MIDDLEWARE_PORTING_NETWORK_API_FUNCTOTYPE_H_
#define _IPANEL_MIDDLEWARE_PORTING_NETWORK_API_FUNCTOTYPE_H_

#include "ipanel_typedef.h"

#ifdef __cplusplus
extern "C" {
#endif

/* network relative */
typedef enum
{
    IPANEL_NETWORK_DEVICE_LAN          	= 1,  // LAN
    IPANEL_NETWORK_DEVICE_DIALUP		= 2,  // Dialup
    IPANEL_NETWORK_DEVICE_PPPOE			= 3,  // PPPoE
    IPANEL_NETWORK_DEVICE_CABLEMODEM  	= 4	  // CableModem
} IPANEL_NETWORK_DEVICE_e;

/* 'op' for ipanel_porting_network_ioctl() */
typedef enum
{
    IPANEL_NETWORK_CONNECT          	= 1,
    IPANEL_NETWORK_DISCONNECT       	= 2,
    IPANEL_NETWORK_SET_IFCONFIG     	= 3,
    IPANEL_NETWORK_GET_IFCONFIG     	= 4,
    IPANEL_NETWORK_GET_STATUS       	= 5,
    IPANEL_NETWORK_SET_STREAM_HOOK  	= 6,
    IPANEL_NETWORK_DEL_STREAM_HOOK  	= 7,
    IPANEL_NETWORK_SET_USER_NAME    	= 8,  
    IPANEL_NETWORK_SET_PWD          	= 9,
    IPANEL_NETWORK_SET_DIALSTRING   	= 10,
    IPANEL_NETWORK_SET_DNS_CONFIG   	= 11,
    IPANEL_NETWORK_GET_DNS_CONFIG   	= 12,
    IPANEL_NETWORK_SET_NIC_MODE  		= 13,
    IPANEL_NETWORK_GET_NIC_MODE  		= 14,
    IPANEL_NETWORK_SET_NIC_BUFFER_SIZE 	= 15,
    IPANEL_NETWORK_GET_NIC_BUFFER_SIZE 	= 16,
    IPANEL_NETWORK_RENEW_IP 			= 17,
    IPANEL_NETWORK_GET_MAC 				= 18,
    IPANEL_NETWORK_GET_NIC_SEND_PACKETS = 19,
    IPANEL_NETWORK_GET_NIC_REVD_PACKETS = 20,
    IPANEL_NETWORK_SEND_PING_REQ 		= 21,
    IPANEL_NETWORK_STOP_PING_REQ 		= 22,

	IPANEL_NETWORK_GET_CM_UPLINKFREQ       = 23,
	IPANEL_NETWORK_GET_CM_DOWNLINKFREQ     = 24,
	IPANEL_NETWORK_SET_CM_DOWNLINKFREQ	   = 25,
	IPANEL_NETWORK_GET_CM_DEVICE_INFO      = 26,
	IPANEL_NETWORK_GET_DOWNLINK_STATUS=27,
	IPANEL_NETWORK_GET_UPLINK_STATUS=28
	

} IPANEL_NETWORK_IOCTL_e;


typedef enum {//IPANEL_NETWORK_SET_DNS_FROM
    IPANEL_NETWORK_DNS_FROM_SERVER		= 1,
    IPANEL_NETWORK_DNS_FROM_USER		= 2
}IPANEL_NETWORK_DNS_MODE_e;

typedef enum { // IPANEL_NETWORK_CONNECT
    IPANEL_NETWORK_LAN_ASSIGN_IP 		= 1,
    IPANEL_NETWORK_LAN_DHCP				= 2      
} IPANEL_NETWORK_LAN_MODE_e;

typedef struct { // IPANEL_NETWORK_SET_IFCONFIG & IPANEL_NETWORK_GET_IFCONFIG
    UINT32_T							ipaddr;
    UINT32_T							netmask;
    UINT32_T							gateway;
} IPANEL_NETWORK_IF_PARAM;

typedef enum { // IPANEL_NETWORK_GET_STATUS
    IPANEL_NETWORK_IF_CONNECTED  		= 1,
    IPANEL_NETWORK_IF_CONNECTING		= 2,
    IPANEL_NETWORK_IF_DISCONNECTED 		= 3
}IPANEL_NETWORK_IF_STATUS_e;

typedef enum{
		IPANEL_NETWORK_CM_NOEXIST  		  =  0,
		IPANEL_NETWORK_CM_DISCONNECTED    =  1,
		IPANEL_NETWORK_CM_CONNECTED		  =  2
}IPANEL_NETWORK_CM_DEVICE_e;

typedef enum{
		IPANEL_NETWORK_CM_OFFLINE   =  0,
		IPANEL_NETWORK_CM_ONLINE    =  1,
		IPANEL_NETWORK_CM_SEARCHING =  2
}IPANEL_NETWORK_CM_STATUS_e;

typedef enum 
{
    IPANEL_NETWORK_NIC_CONFIG_UNKNOW 	= 0,
    IPANEL_NETWORK_AUTO_CONFIG 			= 1,
    IPANEL_NETWORK_10BASE_HALF_DUMPLEX 	= 2,
    IPANEL_NETWORK_10BASE_FULL_DUMPLEX 	= 3,
    IPANEL_NETWORK_100BASE_HALF_DUMPLEX = 4,
    IPANEL_NETWORK_100BASE_FULL_DUMPLEX = 5
}IPANEL_NETWORK_NIC_MODE_e;

typedef VOID (*IPANEL_NETWORK_STREAM_HOOK)(BYTE_T *buf, INT32_T len); // IPANEL_NETWORK_SET_STREAM_HOOK

INT32_T ipanel_porting_network_ioctl(IPANEL_NETWORK_DEVICE_e device, IPANEL_NETWORK_IOCTL_e op, VOID *arg);

#ifdef __cplusplus
}
#endif

#endif // _IPANEL_MIDDLEWARE_PORTING_NETWORK_API_FUNCTOTYPE_H_
