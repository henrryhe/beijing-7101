/******************************************************************************

File name   : net_transmitter.c

Description : STNET normal transmitter

COPYRIGHT (C) 2007 STMicroelectronics

******************************************************************************/

/* Includes ---------------------------------------------------------------- */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stddefs.h"
#include "stnet.h"
#include "net.h"
#include "net_rtp.h"
#include "net_udp.h"
#include "net_recvtrans.h"

#ifndef ST_OSLINUX
#include "sttbx.h"
#endif

/*Add necessary Headers*/

/* Private Types ----------------------------------------------------------- */


/* Global Variables -------------------------------------------------------- */

/* Public functions -------------------------------------------------------- */




/******************************************************************************
Name         : STNETi_Transmitter_Open()

Description  :

Parameters   :

Return Value : Error code
******************************************************************************/
ST_ErrorCode_t STNETi_Transmitter_Open(STNETi_Device_t* Device_p,U8 *Index,const STNET_OpenParams_t* OpenParams_p)
{

    STNETi_Handle_t * Handle_p;
    ST_ErrorCode_t ErrorCode= ST_NO_ERROR;
    S32 Err;


    Handle_p = memory_allocate(Device_p->DriverPartition, sizeof(STNETi_Handle_t));
    if(Handle_p == NULL)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "memory_allocate(%d) failed\n", sizeof(STNETi_Handle_t)));
        return ST_ERROR_NO_MEMORY;
    }


    Handle_p->AccessMutex = STOS_SemaphoreCreateFifo(Device_p->DriverPartition,1);
    STOS_SemaphoreWait(Handle_p->AccessMutex);

    Handle_p->Device_p   = Device_p;
    Handle_p->Next_p     = NULL;

    if(!(strcmp(OpenParams_p->Connection.TransmitterIPAddress,'\0')))
    {
#ifndef ST_OSLINUX
        Handle_p->Type.Transmitter.TransmitterSocketAddress.sin_addr = NG_INADDR_ANY;
#else
        Handle_p->Type.Transmitter.TransmitterSocketAddress.sin_addr.s_addr = htonl(INADDR_ANY);
#endif
    }
    else
    {
#ifndef ST_OSLINUX
        if((Err = ngInetATON(OpenParams_p->Connection.TransmitterIPAddress, &Handle_p->Type.Transmitter.TransmitterSocketAddress.sin_addr)) != NG_EOK)
#else
        Handle_p->Type.Transmitter.TransmitterSocketAddress.sin_family      = AF_INET;
        if(inet_aton(OpenParams_p->Connection.TransmitterIPAddress, &Handle_p->Type.Transmitter.TransmitterSocketAddress.sin_addr.s_addr) != 1)
#endif
        {
            STOS_SemaphoreSignal(Handle_p->AccessMutex);
            STOS_SemaphoreDelete(Device_p->DriverPartition,Handle_p->AccessMutex);
            memory_deallocate(Device_p->DriverPartition, Handle_p);
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "IP address %s not valid Err[%d]\n", OpenParams_p->Connection.TransmitterIPAddress, Err));
            return STNET_ERROR_INVALID_ADDRESS;
        }
    }

    /* Find Port number */
    if(OpenParams_p->Connection.TransmitterPort !=0)   /* Port number is present */
    {

#ifndef ST_OSLINUX
    Handle_p->Type.Transmitter.TransmitterSocketAddress.sin_port = ngHTONS(OpenParams_p->Connection.TransmitterPort);
#else
    Handle_p->Type.Transmitter.TransmitterSocketAddress.sin_port = htons(OpenParams_p->Connection.TransmitterPort);
#endif
    }



    if(!(strcmp(OpenParams_p->Connection.ReceiverIPAddress,'\0')))
    {
#ifndef ST_OSLINUX
        Handle_p->Type.Transmitter.ReceiverSocketAddress.sin_addr = NG_INADDR_ANY;
#else
        Handle_p->Type.Transmitter.ReceiverSocketAddress.sin_addr.s_addr = htonl(INADDR_ANY);
#endif
    }
    else
    {
#ifndef ST_OSLINUX
        if((Err = ngInetATON(OpenParams_p->Connection.ReceiverIPAddress, &Handle_p->Type.Transmitter.ReceiverSocketAddress.sin_addr)) != NG_EOK)
#else
        Handle_p->Type.Transmitter.ReceiverSocketAddress.sin_family      = AF_INET;
        if(inet_aton(OpenParams_p->Connection.ReceiverIPAddress, &Handle_p->Type.Transmitter.ReceiverSocketAddress.sin_addr.s_addr) != 1)
#endif
        {
            STOS_SemaphoreSignal(Handle_p->AccessMutex);
            STOS_SemaphoreDelete(Device_p->DriverPartition,Handle_p->AccessMutex);
            memory_deallocate(Device_p->DriverPartition, Handle_p);
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "IP address %s not valid Err[%d]\n", OpenParams_p->Connection.TransmitterIPAddress, Err));
            return STNET_ERROR_INVALID_ADDRESS;
        }
    }

    /* Find Port number */
    if(OpenParams_p->Connection.ReceiverPort !=0)   /* Port number is present */
    {

#ifndef ST_OSLINUX
    Handle_p->Type.Transmitter.ReceiverSocketAddress.sin_port = ngHTONS(OpenParams_p->Connection.ReceiverPort);
#else
    Handle_p->Type.Transmitter.ReceiverSocketAddress.sin_port = htons(OpenParams_p->Connection.ReceiverPort);
#endif
    }
    else
    {
        Handle_p->Type.Transmitter.ReceiverSocketAddress.sin_port = 0;
    }


    /* open a socket */
#ifndef ST_OSLINUX
    if((Err = ngSockCreate(&Handle_p->Type.Transmitter.SocketDescriptor, NG_AF_INET, NG_SOCK_DGRAM, 0, NG_O_RDONLY | NG_O_WRONLY)) != NG_EOK)
#else
    if((Handle_p->Type.Transmitter.SocketDescriptor = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
#endif
    {
        STOS_SemaphoreSignal(Handle_p->AccessMutex);
        STOS_SemaphoreDelete(Device_p->DriverPartition,Handle_p->AccessMutex);
        memory_deallocate(Device_p->DriverPartition, Handle_p);
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "socket open failed Err[%d]\n", Err));
        return STNET_ERROR_SOCKET_OPEN_FAILED;
    }

#ifndef ST_OSLINUX
        if((Err = ngSockBind(Handle_p->Type.Transmitter.SocketDescriptor, &Handle_p->Type.Transmitter.TransmitterSocketAddress)) != NG_EOK)
#else
        if((Err = bind(Handle_p->Type.Transmitter.SocketDescriptor, (struct sockaddr *)&Handle_p->Type.Transmitter.TransmitterSocketAddress,sizeof(Handle_p->Type.Transmitter.ReceiverSocketAddress)))!=0)
#endif
        {
#ifndef ST_OSLINUX
            ngSockClose(Handle_p->Type.Transmitter.SocketDescriptor);
#else
            close(Handle_p->Type.Transmitter.SocketDescriptor);
#endif
            STOS_SemaphoreSignal(Handle_p->AccessMutex);
            STOS_SemaphoreDelete(Device_p->DriverPartition,Handle_p->AccessMutex);
            memory_deallocate(Device_p->DriverPartition, Handle_p);
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "socket bind failed Err[%d]\n", Err));
            return STNET_ERROR_SOCKET_BIND_FAILED;
        }



    *Index = STNETi_AddToList(Device_p, (STNETi_Handle_t *) Handle_p);


    STOS_SemaphoreSignal(Handle_p->AccessMutex);

    return ErrorCode;

}

/******************************************************************************
Name         : STNETi_Receiver_Close()

Description  :

Parameters   :

Return Value : Error code
******************************************************************************/
ST_ErrorCode_t STNETi_Transmitter_Close(STNETi_Device_t* Device_p,U8 Index)
{

    STNETi_Handle_t * Handle_p;
    ST_ErrorCode_t ErrorCode= ST_NO_ERROR;
    S32 Err;


    Handle_p = STNETi_RemovefromList(Device_p , Index);
    if(Handle_p == NULL)
    {
        return ST_ERROR_INVALID_HANDLE;
    }

    STOS_SemaphoreWait(Handle_p->AccessMutex);

#ifndef ST_OSLINUX
    if((Err = ngSockShutdown(Handle_p->Type.Transmitter.SocketDescriptor)) != NG_EOK)

#else
    if((Err = shutdown(Handle_p->Type.Transmitter.SocketDescriptor, SHUT_RDWR)) != 0)
#endif
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "socket shutdown failed Err[%d]\n", Err));
    }

#ifndef ST_OSLINUX
    if((Err = ngSockClose(Handle_p->Type.Transmitter.SocketDescriptor)) != NG_EOK)
#else
    if((Err = close(Handle_p->Type.Transmitter.SocketDescriptor)) != 0)
#endif
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "socket close failed Err[%d]\n", Err));
        return STNET_ERROR_SOCKET_CLOSE_FAILED;
    }


    STOS_SemaphoreSignal(Handle_p->AccessMutex);
    STOS_SemaphoreDelete(Device_p->DriverPartition,Handle_p->AccessMutex);

    memory_deallocate(Device_p->DriverPartition, Handle_p);

    return ErrorCode;

}

/******************************************************************************
Name         : STNETi_Receiver_CloseAll()

Description  :

Parameters   :

Return Value : Error code
******************************************************************************/
ST_ErrorCode_t STNETi_Transmitter_CloseAll(STNETi_Device_t* Device_p)
{
    STNETi_Handle_t * Handle_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    Handle_p = (STNETi_Handle_t *)Device_p->HandleList_p;

    while(Handle_p != NULL)
    {
        ErrorCode |= STNETi_Transmitter_Close(Device_p,Handle_p->Instance);
        Handle_p = Handle_p->Next_p;
    }

    return ErrorCode;

}
/******************************************************************************
Name         : STNETi_TransmitterStart()

Description  : Start receiver task to receive data from opened socket

Parameters   : STNET_Handle_t               Handle              handle returned by open
               STNET_ReceiverStartParams_t *ReceiverStartParams start parameters

Return Value : Error code
******************************************************************************/
ST_ErrorCode_t STNETi_Transmitter_Start(STNETi_Device_t *Device_p, U8 Instance , const STNET_TransmitterStartParams_t * TransmitterStartParams_p)
{
    S32                 Err;
    STNETi_Handle_t    *Handle_p ;
    U8                 *TSBuffer;
    U32                 TSNbPackets,
                        TSNbPacketsToWrite;
    STNETi_RTP_Header_t RTPHeader;
    U8                 *IPPacketBuffer;
    U32                 IPPacketSize,
                        IPPayloadSize;
    U32                 i;
#ifndef ST_OSLINUX
    NGbuf              *NGBuffer;
#endif



    Handle_p = STNETi_GetHandlefromList(Device_p,Instance);
    if(Handle_p == NULL)
    {
        return ST_ERROR_INVALID_HANDLE;
    }

     if(((TransmitterStartParams_p->BufferSize / STNETi_TS_PACKET_SIZE) * STNETi_TS_PACKET_SIZE) != TransmitterStartParams_p->BufferSize)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "BufferSize[%d] not a multiple of PacektSize[%d]\n",
                TransmitterStartParams_p->BufferSize, STNETi_TS_PACKET_SIZE));
        return ST_ERROR_BAD_PARAMETER;
    }

    if(TransmitterStartParams_p->IsBlocking == FALSE)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Non blocking mode not supported\n"));
        return ST_ERROR_FEATURE_NOT_SUPPORTED;
    }



    STOS_SemaphoreWait(Handle_p->AccessMutex);


    if(Handle_p->TransmissionProtocol == STNET_PROTOCOL_TS)
    {
        IPPacketSize = STNETi_TS_PAYLOAD_SIZE_IN_UDP;
    }
    else if(Handle_p->TransmissionProtocol == STNET_PROTOCOL_RTP_TS)
    {
        /* populate RTP header */
        RTPHeader.Version        = 2;
        RTPHeader.Padding        = 0;
        RTPHeader.Extension      = 0;
        RTPHeader.CSRCCount      = 0;
        RTPHeader.Marker         = 0;
        RTPHeader.PayloadType    = STNETi_RTP_PAYLOAD_TYPE_TS;
        RTPHeader.SequenceNumber = 0;
        RTPHeader.TimeStamp      = 0;                   /* TODO: TBI as per the spec */
        RTPHeader.SSRC           = STNET_DRIVER_BASE;   /* TODO: TBI as per the spec */

        IPPacketSize = STNETi_RTP_HEADER_MIN_SIZE + STNETi_TS_PAYLOAD_SIZE_IN_UDP;
    }

#ifdef ST_OSLINUX
    IPPacketBuffer = memory_allocate(Device_p->NCachePartition, IPPacketSize);
    if (IPPacketBuffer == NULL)
    {
        STOS_SemaphoreSignal(Handle_p->AccessMutex);
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "memory_allocate(%d) failed\n", IPPacketSize));
        return ST_ERROR_NO_MEMORY;
    }
#endif

    TSBuffer = TransmitterStartParams_p->BufferAddress;
    TSNbPackets = TransmitterStartParams_p->BufferSize / STNETi_TS_PACKET_SIZE;

    /* write data to network */
    while(TSNbPackets > 0)
    {
#ifndef ST_OSLINUX
        Err = ngSockBufAlloc(Handle_p->Type.Transmitter.SocketDescriptor, &NGBuffer);
        if(Err != NG_EOK)
        {
            STOS_SemaphoreSignal(Handle_p->AccessMutex);
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "socket buf alloc failed Err[%d]\n", Err));
            return ST_ERROR_NO_MEMORY;
        }
        IPPacketBuffer = NGBuffer->buf_datap;
#endif

        TSNbPacketsToWrite = (TSNbPackets > STNETi_TS_NB_PACKETS_IN_UDP) ? STNETi_TS_NB_PACKETS_IN_UDP : TSNbPackets;

        /* handle protocol here */
        if(Handle_p->TransmissionProtocol == STNET_PROTOCOL_TS)
        {
            /* copy data */
            memcpy(IPPacketBuffer, TSBuffer, TSNbPacketsToWrite * STNETi_TS_PACKET_SIZE);

            IPPayloadSize = TSNbPacketsToWrite * STNETi_TS_PACKET_SIZE;
        }
        else if(Handle_p->TransmissionProtocol == STNET_PROTOCOL_RTP_TS)
        {
            RTPHeader.SequenceNumber = Handle_p->Type.Transmitter.SequenceNumber++;
            RTPHeader.TimeStamp = time_now();   /* passing an incrementaal clock */
#if 0
            IPPacketBuffer[0]  = (RTPHeader.Version <<6) | (RTPHeader.Padding <<5) | (RTPHeader.Extension <<4) | RTPHeader.CSRCCount;
            IPPacketBuffer[1]  = (RTPHeader.Marker << 7) | RTPHeader.PayloadType;
            IPPacketBuffer[2]  = RTPHeader.SequenceNumber >> 8;
            IPPacketBuffer[3]  = RTPHeader.SequenceNumber;
            IPPacketBuffer[4]  = RTPHeader.TimeStamp >> 24;
            IPPacketBuffer[5]  = RTPHeader.TimeStamp >> 16;
            IPPacketBuffer[6]  = RTPHeader.TimeStamp >> 8;
            IPPacketBuffer[7]  = RTPHeader.TimeStamp;
            IPPacketBuffer[8]  = RTPHeader.SSRC >> 24;
            IPPacketBuffer[9]  = RTPHeader.SSRC >> 16;
            IPPacketBuffer[10] = RTPHeader.SSRC >> 8;
            IPPacketBuffer[11] = RTPHeader.SSRC;
#else
            STNETi_ENDIAN_MEMCPY_WORDS(IPPacketBuffer, &RTPHeader, STNETi_RTP_HEADER_MIN_SIZE);
#endif

            /* copy data */
            memcpy(IPPacketBuffer + STNETi_RTP_HEADER_MIN_SIZE, TSBuffer, TSNbPacketsToWrite * STNETi_TS_PACKET_SIZE);

            IPPayloadSize = STNETi_RTP_HEADER_MIN_SIZE + (TSNbPacketsToWrite * STNETi_TS_PACKET_SIZE);
        }

#ifndef ST_OSLINUX
        NGBuffer->buf_datalen = IPPayloadSize;

        if(ngSockBufSend(Handle_p->Type.Transmitter.SocketDescriptor, NGBuffer, 0, NULL) != IPPayloadSize)
        {
            ngSockBufFree(Handle_p->Type.Transmitter.SocketDescriptor, NGBuffer);
            STOS_SemaphoreSignal(Handle_p->AccessMutex);
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "socket write[%d] failed\n", IPPayloadSize));
            return STNET_ERROR_SOCKET_WRITE_FAILED;
        }
#else
        if(sendto(Handle_p->Type.Transmitter.SocketDescriptor, IPPacketBuffer, IPPayloadSize,0,
                (struct sockaddr *)&Handle_p->Type.Transmitter.ReceiverSocketAddress,sizeof(Handle_p->Type.Transmitter.ReceiverSocketAddress)) != IPPayloadSize)
        {
            memory_deallocate(Device_p->NCachePartition, IPPacketBuffer);
            STOS_SemaphoreSignal(Handle_p->AccessMutex);
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "socket write[%d] failed\n", IPPayloadSize));
            return STNET_ERROR_SOCKET_WRITE_FAILED;
        }
#endif

        TSBuffer    += TSNbPacketsToWrite * STNETi_TS_PACKET_SIZE;
        TSNbPackets -= TSNbPacketsToWrite;
    }

#ifdef ST_OSLINUX
    memory_deallocate(Device_p->NCachePartition, IPPacketBuffer);
#endif

    STOS_SemaphoreSignal(Handle_p->AccessMutex);

    return ST_NO_ERROR;
}



/******************************************************************************
Name         : STNETi_Transmitter_Stop()

Description  : Start receiver task to receive data from opened socket

Parameters   : STNET_Handle_t               Handle              handle returned by open
               STNET_ReceiverStartParams_t *ReceiverStartParams start parameters

Return Value : Error code
******************************************************************************/
ST_ErrorCode_t STNETi_Transmitter_Stop(STNETi_Device_t *Device_p, U8 Instance)
{

    /*Nothing at the moment*/
    STNETi_Handle_t    *Handle_p ;

    Handle_p = STNETi_GetHandlefromList(Device_p,Instance);
    if(Handle_p == NULL)
    {
        return ST_ERROR_INVALID_HANDLE;
    }


    return ST_NO_ERROR;

}



/* EOF --------------------------------------------------------------------- */

