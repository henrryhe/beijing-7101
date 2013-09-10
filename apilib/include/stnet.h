/************************************************************************
COPYRIGHT (C) STMicroelectronics 2007

Source file name : stnet.h

NET Interface

************************************************************************/

#ifndef __STNET_H
#define __STNET_H

/* Includes ---------------------------------------------------------------- */
#include "stos.h"

#ifdef __cplusplus
extern "C" {
#endif

#define STNET_VERSION       "1.0.0"
#define STNET_REVISION      "STNET-REL_" STNET_VERSION "               "
#define STNET_DRIVER_ID     500 /*to be changed */
#define STNET_DRIVER_BASE   (STNET_DRIVER_ID << 16)


/* The stack bypass device has an injecter task to submit packets via STINJ.
 * This is the default priority of that task.
 * TODO: Submit stcommon patch for this
 */
#ifndef DVD_USE_GLOBAL_PRIORITIES
#define STNET_RECEIVER_SB_INJECTER_TASK_PRIORITY	MAX_USER_PRIORITY
#endif


typedef enum
{
    STNET_DEVICE_RECEIVER = 1,/*For receving data via sockets*/
    STNET_DEVICE_TRANSMITTER = 2,/*For transmitting data*/
    STNET_DEVICE_RECEIVER_SB_INJECTER =4,/*Receive Data -> Stack Bypass -> Inject*/
    STNET_DEVICE_UNKNOWN=0
}STNET_Device_t;

/*Not required for Linux.Only required for OS21*/
typedef enum STNET_EthernetDevice_s
{
    STNET_DEVICE_STMAC110,
    STNET_DEVICE_LAN91,
    STNET_DEVICE_LAN9X1X,
    STNET_DEVICE_USB_CBL_MDM,
    STNET_DEVICE_USB_ETHERNET,
    STNET_DEVICE_USB_H2HETH,
    STNET_DEVICE_USB_H2H
} STNET_EthernetDevice_t;


typedef enum
{
    STNET_PROTOCOL_RTP_TS,
    STNET_PROTOCOL_TS,
    STNET_PROTOCOL_RTP_ES /*Will be used later*/
}STNET_TransmissionProtocol_t;



/*STNET return error codes-- Other error types will also be defined. Think about it*/
enum
{
    STNET_ERROR_INVALID_DEVICE_TYPE = STNET_DRIVER_BASE,
    STNET_ERROR_INVALID_PROTOCOL,
    STNET_ERROR_INVALID_ADDRESS,
    STNET_ERROR_NOT_RECEIVER_OBJECT,
    STNET_ERROR_NOT_TRANSMITTER_OBJECT,
    STNET_ERROR_ALREADY_STARTED,
    STNET_ERROR_ALREADY_STOPPED,
    STNET_ERROR_SOCKET_OPEN_FAILED,
    STNET_ERROR_SOCKET_BIND_FAILED,
    STNET_ERROR_SOCKET_CONNECT_FAILED,
    STNET_ERROR_SOCKET_SET_OPTION_FAILED,
    STNET_ERROR_SOCKET_READ_FAILED,
    STNET_ERROR_SOCKET_WRITE_FAILED,
    STNET_ERROR_SOCKET_SHUTDOWN_FAILED,
    STNET_ERROR_SOCKET_CLOSE_FAILED,
    STNET_ERROR_REGISTER_HOOK_FAILED,

};

typedef U16 STNET_Handle_t;

typedef enum STNET_Event_e
{
    STNET_EVT_TRANSMIT_COMPLETED = STNET_DRIVER_BASE
} STNET_Event_t;


#define STNET_MAX_IPADDRESS	20	/*modify for ipv6*/

typedef struct
{
    U16  TransmitterPort;
    char TransmitterIPAddress[STNET_MAX_IPADDRESS];
    U16  ReceiverPort;
    char ReceiverIPAddress[STNET_MAX_IPADDRESS];
}STNET_Connection_t;

typedef struct
{
    STNET_Device_t DeviceType;

    ST_DeviceName_t     EVTDeviceName;

    /*Not really required for LINUX*/
    ST_Partition_t     *DriverPartition;
    ST_Partition_t     *NCachePartition;

    /*Not required for LINUX.Reqd only for OS21 to initialise TCP stack of OSPLUS*/
    STNET_EthernetDevice_t EthernetDevice;
    char                IPAddress[STNET_MAX_IPADDRESS];
    char                IPGateway[STNET_MAX_IPADDRESS];
    char                IPSubnetMask[STNET_MAX_IPADDRESS];
    char                MACAddress[STNET_MAX_IPADDRESS];

}STNET_InitParams_t;

typedef struct STNET_OpenParams_s
{
    STNET_TransmissionProtocol_t    TransmissionProtocol;
    STNET_Connection_t Connection;
    /*Only valid for DeviceType == STNET_DEVICE_RECEIVER_SB_INJECTER*/
    ST_DeviceName_t INJ_HandlerName;

} STNET_OpenParams_t;


typedef struct STNET_ReceiverStartParams_s
{
    U8                 *BufferAddress;
    U32                 BufferSize;
    BOOL                AllowOverflow;
} STNET_ReceiverStartParams_t;

typedef struct STNET_TransmitterStartParams_s
{
    U8                 *BufferAddress;
    U32                 BufferSize;
    BOOL                IsBlocking;
} STNET_TransmitterStartParams_t;

typedef struct STNET_StartParams_s
{
    union
    {
        STNET_ReceiverStartParams_t ReceiverParams;
        STNET_TransmitterStartParams_t TransmitterParams;
    } u;
}STNET_StartParams_t;


#define STNET_DEFAULT_WINDOWSIZE	70
#define STNET_DEFAULT_WINDOWTIMEOUT	500

typedef struct STNET_ReceiverConfigParams_s
{
    U32 WindowSize; /*Reordering Depth*/
    U32 WindowTimeout; /*In ms*/
}STNET_ReceiverConfigParams_t;

typedef struct
{
    BOOL ForceTerminate;
}STNET_TermParams_t;

typedef struct {
	STNET_Device_t	Device;
} STNET_Capability_t;


ST_ErrorCode_t STNET_Init(const ST_DeviceName_t DeviceName,const STNET_InitParams_t *InitParams);

ST_ErrorCode_t STNET_Open(const ST_DeviceName_t DeviceName,const STNET_OpenParams_t *OpenParams, STNET_Handle_t *Handle);

ST_ErrorCode_t STNET_Start(STNET_Handle_t Handle, const STNET_StartParams_t *StartParams);

ST_ErrorCode_t STNET_Stop(STNET_Handle_t Handle);

ST_ErrorCode_t STNET_Close(STNET_Handle_t Handle);

ST_ErrorCode_t STNET_ReceiverGetWritePointer(STNET_Handle_t Handle, void **Address);

ST_ErrorCode_t STNET_ReceiverSetReadPointer(STNET_Handle_t Handle, void *Address);

ST_ErrorCode_t STNET_SetReceiverConfig(STNET_Handle_t Handle,STNET_ReceiverConfigParams_t *ConfigParams);

ST_ErrorCode_t STNET_Term(const ST_DeviceName_t DeviceName,const STNET_TermParams_t *TermParams);

ST_ErrorCode_t STNET_GetCapability(const ST_DeviceName_t DeviceName,STNET_Capability_t *Capability_p);

ST_Revision_t  STNET_GetRevision(void);

#ifdef __cplusplus
}
#endif

#endif /* _STPIO_H */
