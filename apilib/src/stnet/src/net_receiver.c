/******************************************************************************

File name   : net_receiver.c

Description : STNET normal receiver

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
#include "nginit.h"
#endif

/*Add necessary Headers*/

/* Private Types ----------------------------------------------------------- */


/* Global Variables -------------------------------------------------------- */


/*Function Prototype*/
static void STNETi_ReceiverTask(STNETi_Handle_t   *Handle_p);

/* Public functions -------------------------------------------------------- */



/******************************************************************************
Name         : STNETi_Receiver_Open()

Description  :

Parameters   :

Return Value : Error code
******************************************************************************/
ST_ErrorCode_t STNETi_Receiver_Open(STNETi_Device_t* Device_p,U8 *Index, const STNET_OpenParams_t *OpenParams_p)
{

    STNETi_Handle_t * Handle_p;
    ST_ErrorCode_t ErrorCode= ST_NO_ERROR;
#ifndef ST_OSLINUX
    NGtimeval           TimeOut;
#else
    struct timeval      TimeOut;
#endif
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


    if(strcmp(OpenParams_p->Connection.ReceiverIPAddress,"\0")!=0)
    {
#ifndef ST_OSLINUX
        Handle_p->Type.Receiver.TransmitterSocketAddress.sin_addr = NG_INADDR_ANY;
#else
        Handle_p->Type.Receiver.TransmitterSocketAddress.sin_addr.s_addr = htonl(INADDR_ANY);
#endif
    }
    else
    {
#ifndef ST_OSLINUX
        if((Err = ngInetATON(OpenParams_p->Connection.ReceiverIPAddress, &Handle_p->Type.Receiver.TransmitterSocketAddress.sin_addr)) != NG_EOK)
#else
        Handle_p->Type.Receiver.TransmitterSocketAddress.sin_family      = AF_INET;
        if(inet_aton(OpenParams_p->Connection.ReceiverIPAddress, &Handle_p->Type.Receiver.TransmitterSocketAddress.sin_addr.s_addr) != 1)
#endif
        {
            STOS_SemaphoreSignal(Handle_p->AccessMutex);
            STOS_SemaphoreDelete(Device_p->DriverPartition,Handle_p->AccessMutex);
            memory_deallocate(Device_p->DriverPartition, Handle_p);
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "IP address %s not valid Err[%d]\n", OpenParams_p->Connection.ReceiverIPAddress, Err));
            return STNET_ERROR_INVALID_ADDRESS;
        }
    }

    /* Find Port number */
    if(OpenParams_p->Connection.ReceiverPort !=0)   /* Port number is present */
    {

#ifndef ST_OSLINUX
    Handle_p->Type.Receiver.TransmitterSocketAddress.sin_port = ngHTONS(OpenParams_p->Connection.ReceiverPort);
#else
    Handle_p->Type.Receiver.TransmitterSocketAddress.sin_port = htons(OpenParams_p->Connection.ReceiverPort);
#endif
    }


    /*Get Port Nos of the host on which the socket to be bound-- Need to review*/

    if(strcmp(OpenParams_p->Connection.ReceiverIPAddress,"\0")!=0)
    {
#ifndef ST_OSLINUX
        Handle_p->Type.Receiver.ReceiverSocketAddress.sin_addr = NG_INADDR_ANY;
#else
        Handle_p->Type.Receiver.ReceiverSocketAddress.sin_addr.s_addr = htonl(INADDR_ANY);
#endif
    }
    else
    {
#ifndef ST_OSLINUX
        if((Err = ngInetATON(OpenParams_p->Connection.ReceiverIPAddress, &Handle_p->Type.Receiver.ReceiverSocketAddress.sin_addr)) != NG_EOK)
#else
        Handle_p->Type.Receiver.ReceiverSocketAddress.sin_family      = AF_INET;
        if(inet_aton(OpenParams_p->Connection.ReceiverIPAddress, &Handle_p->Type.Receiver.ReceiverSocketAddress.sin_addr.s_addr) != 1)
#endif
        {
            STOS_SemaphoreSignal(Handle_p->AccessMutex);
            STOS_SemaphoreDelete(Device_p->DriverPartition,Handle_p->AccessMutex);
            memory_deallocate(Device_p->DriverPartition, Handle_p);
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "IP address %s not valid Err[%d]\n", OpenParams_p->Connection.ReceiverIPAddress, Err));
            return STNET_ERROR_INVALID_ADDRESS;
        }
    }

    /* Find Port number */
    if(OpenParams_p->Connection.ReceiverPort !=0)   /* Port number is present */
    {

#ifndef ST_OSLINUX
    Handle_p->Type.Receiver.ReceiverSocketAddress.sin_port = ngHTONS(OpenParams_p->Connection.ReceiverPort);
#else
    Handle_p->Type.Receiver.ReceiverSocketAddress.sin_port = htons(OpenParams_p->Connection.ReceiverPort);
#endif
    }
    else
    {
        Handle_p->Type.Receiver.ReceiverSocketAddress.sin_port = 0;
    }
#ifndef ST_OSLINUX
    NG_SocketAddr.sin_addr = Handle_p->Type.Receiver.ReceiverSocketAddress.sin_addr;
    NG_SocketAddr.sin_port = Handle_p->Type.Receiver.ReceiverSocketAddress.sin_port;
#endif

    /* open a socket */
#ifndef ST_OSLINUX
#if !defined(OSPLUS_INPUT_STREAM_BYPASS)
    if((Err = ngSockCreate(&Handle_p->Type.Receiver.SocketDescriptor, NG_AF_INET, NG_SOCK_DGRAM, 0, NG_O_RDONLY | NG_O_WRONLY)) != NG_EOK)
    {
        STOS_SemaphoreSignal(Handle_p->AccessMutex);
        STOS_SemaphoreDelete(Device_p->DriverPartition,Handle_p->AccessMutex);
        memory_deallocate(Device_p->DriverPartition, Handle_p);
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "socket open failed Err[%d]\n", Err));
        return STNET_ERROR_SOCKET_OPEN_FAILED;
    }

#endif
#else
    if((Handle_p->Type.Receiver.SocketDescriptor = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        STOS_SemaphoreSignal(Handle_p->AccessMutex);
        STOS_SemaphoreDelete(Device_p->DriverPartition,Handle_p->AccessMutex);
        memory_deallocate(Device_p->DriverPartition, Handle_p);
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "socket open failed Err[%d]\n", Err));
        return STNET_ERROR_SOCKET_OPEN_FAILED;
    }
#endif


#ifndef ST_OSLINUX
        if(NG_IN_MULTICAST(Handle_p->Type.Receiver.TransmitterSocketAddress.sin_addr))
#else
        if(IN_MULTICAST(ntohl(Handle_p->Type.Receiver.TransmitterSocketAddress.sin_addr.s_addr)))
#endif
        {
#ifndef ST_OSLINUX
#if !defined(OSPLUS_INPUT_STREAM_BYPASS)
            NGip_mreq   MulticastReq;

            MulticastReq.imr_multiaddr = Handle_p->Type.Receiver.ReceiverSocketAddress.sin_addr;
            MulticastReq.imr_interface = Device_p->IPAddress;
            if((Err = ngSockSetOption(Handle_p->Type.Receiver.SocketDescriptor, NG_IOCTL_IP, NG_IP_ADD_MEMBERSHIP, &MulticastReq, sizeof(MulticastReq))) != NG_EOK)
            {
                ngSockClose(Handle_p->Type.Receiver.SocketDescriptor);
                STOS_SemaphoreSignal(Handle_p->AccessMutex);
                STOS_SemaphoreDelete(Device_p->DriverPartition,Handle_p->AccessMutex);
                memory_deallocate(Device_p->DriverPartition, Handle_p);
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "setsockopt IP_ADD_MEMBERSHIP failed Err[%d]\n", Err));
                return STNET_ERROR_SOCKET_SET_OPTION_FAILED;
            }
#endif
#else
            struct ip_mreq  MulticastReq;

            MulticastReq.imr_multiaddr.s_addr = Handle_p->Type.Receiver.TransmitterSocketAddress.sin_addr.s_addr;
            MulticastReq.imr_interface.s_addr = htonl(INADDR_ANY);  /* TODO: to add the HOST IP address */
            if((Err = setsockopt(Handle_p->Type.Receiver.SocketDescriptor, IPPROTO_IP, IP_ADD_MEMBERSHIP, &MulticastReq, sizeof(MulticastReq))) != 0)
            {
                close(Handle_p->Type.Receiver.SocketDescriptor);
                STOS_SemaphoreSignal(Handle_p->AccessMutex);
                STOS_SemaphoreDelete(Device_p->DriverPartition,Handle_p->AccessMutex);
                memory_deallocate(Device_p->DriverPartition, Handle_p);
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "setsockopt IP_ADD_MEMBERSHIP failed Err[%d]\n", Err));
                return STNET_ERROR_SOCKET_SET_OPTION_FAILED;
            }
#endif
        }

#ifndef ST_OSLINUX
#if !defined(OSPLUS_INPUT_STREAM_BYPASS)
        if((Err = ngSockBind(Handle_p->Type.Receiver.SocketDescriptor, &Handle_p->Type.Receiver.ReceiverSocketAddress)) != NG_EOK)
        {
            ngSockClose(Handle_p->Type.Receiver.SocketDescriptor);
            STOS_SemaphoreSignal(Handle_p->AccessMutex);
            STOS_SemaphoreDelete(Device_p->DriverPartition,Handle_p->AccessMutex);
            memory_deallocate(Device_p->DriverPartition, Handle_p);
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "socket bind failed Err[%d]\n", Err));
            return STNET_ERROR_SOCKET_BIND_FAILED;
        }
#endif
#else
        if((Err = bind(Handle_p->Type.Receiver.SocketDescriptor, (struct sockaddr *)&Handle_p->Type.Receiver.ReceiverSocketAddress,sizeof(Handle_p->Type.Receiver.ReceiverSocketAddress)))!=0)
        {
            close(Handle_p->Type.Receiver.SocketDescriptor);
            STOS_SemaphoreSignal(Handle_p->AccessMutex);
            STOS_SemaphoreDelete(Device_p->DriverPartition,Handle_p->AccessMutex);
            memory_deallocate(Device_p->DriverPartition, Handle_p);
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "socket bind failed Err[%d]\n", Err));
            return STNET_ERROR_SOCKET_BIND_FAILED;
        }
#endif
         /* Set receiver timeout so that read is not blocking */
        TimeOut.tv_sec  = 1;
        TimeOut.tv_usec = 0;
#ifndef ST_OSLINUX
#if !defined(OSPLUS_INPUT_STREAM_BYPASS)
        if((Err = ngSockSetOption(Handle_p->Type.Receiver.SocketDescriptor, NG_IOCTL_SOCKET, NG_SO_RCVTIMEO, &TimeOut, sizeof(TimeOut))) !=NG_EOK)
        {
            ngSockClose(Handle_p->Type.Receiver.SocketDescriptor);
            STOS_SemaphoreSignal(Handle_p->AccessMutex);
            STOS_SemaphoreDelete(Device_p->DriverPartition,Handle_p->AccessMutex);
            memory_deallocate(Device_p->DriverPartition, Handle_p);
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "setsockopt SO_RCVTIMEO failed Err[%d]\n", Err));
            return STNET_ERROR_SOCKET_SET_OPTION_FAILED;
        }
#endif
#else
        if((Err = setsockopt(Handle_p->Type.Receiver.SocketDescriptor, SOL_SOCKET, SO_RCVTIMEO, &TimeOut, sizeof(TimeOut))) != 0)
        {

            close(Handle_p->Type.Receiver.SocketDescriptor);

            STOS_SemaphoreSignal(Handle_p->AccessMutex);
            STOS_SemaphoreDelete(Device_p->DriverPartition,Handle_p->AccessMutex);
            memory_deallocate(Device_p->DriverPartition, Handle_p);
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "setsockopt SO_RCVTIMEO failed Err[%d]\n", Err));
            return STNET_ERROR_SOCKET_SET_OPTION_FAILED;
        }
#endif

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
ST_ErrorCode_t STNETi_Receiver_Close(STNETi_Device_t* Device_p,U8 Index)
{

    STNETi_Handle_t * Handle_p;
    ST_ErrorCode_t ErrorCode= ST_NO_ERROR;
    S32 Err;


   /* check if receiver is started */

    if((Handle_p->Type.Receiver.State != RECEIVER_RUNNING))
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "receiver already stopped\n"));
        return ST_ERROR_BAD_PARAMETER;  /* TODO: Add new error code */
    }

    STOS_SemaphoreWait(Handle_p->AccessMutex);

    Handle_p = STNETi_RemovefromList(Device_p , Index);
    if(Handle_p == NULL)
    {
        return ST_ERROR_INVALID_HANDLE;
    }

#ifndef ST_OSLINUX
#if !defined(OSPLUS_INPUT_STREAM_BYPASS)
    if((Err = ngSockShutdown(Handle_p->Type.Receiver.SocketDescriptor)) != NG_EOK)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "socket shutdown failed Err[%d]\n", Err));
    }
#endif
#else
    if((Err = shutdown(Handle_p->Type.Receiver.SocketDescriptor, SHUT_RDWR)) != 0)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "socket shutdown failed Err[%d]\n", Err));
    }
#endif


#ifndef ST_OSLINUX
#if !defined(OSPLUS_INPUT_STREAM_BYPASS)
    if((Err = ngSockClose(Handle_p->Type.Receiver.SocketDescriptor)) != NG_EOK)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "socket close failed Err[%d]\n", Err));
        return STNET_ERROR_SOCKET_CLOSE_FAILED;
    }
#endif
#else
    if((Err = close(Handle_p->Type.Receiver.SocketDescriptor)) != 0)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "socket close failed Err[%d]\n", Err));
        return STNET_ERROR_SOCKET_CLOSE_FAILED;
    }

#endif


    STOS_SemaphoreSignal(Handle_p->AccessMutex);
    STOS_SemaphoreDelete(Device_p->DriverPartition,Handle_p->AccessMutex);

    memory_deallocate(Device_p->DriverPartition, Handle_p);

    return ST_NO_ERROR;

}

/******************************************************************************
Name         : STNETi_Receiver_CloseAll()

Description  :

Parameters   :

Return Value : Error code
******************************************************************************/
ST_ErrorCode_t STNETi_Receiver_CloseAll(STNETi_Device_t* Device_p)
{
    STNETi_Handle_t * Handle_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    Handle_p = Device_p->HandleList_p;

    while(Handle_p != NULL)
    {
        ErrorCode |= STNETi_Receiver_Close(Device_p,Handle_p->Instance);
        Handle_p = Handle_p->Next_p;
    }

    return ErrorCode;

}
/******************************************************************************
Name         : STNETi_Receiver_Start()

Description  : Start receiver task to receive data from opened socket

Parameters   : STNET_Handle_t               Handle              handle returned by open
               STNET_ReceiverStartParams_t *ReceiverStartParams start parameters

Return Value : Error code
******************************************************************************/
ST_ErrorCode_t STNETi_Receiver_Start(STNETi_Device_t *Device_p, U8 Instance , const STNET_ReceiverStartParams_t * ReceiverStartParams_p)
{
    STNETi_Handle_t    *Handle_p ;
    U8                  TaskName[STNETi_MAX_ADDRESS_LENGTH+22] = "STNETi_ReceiverTask(";


    Handle_p = STNETi_GetHandlefromList(Device_p,Instance);
    if(Handle_p == NULL)
    {
        return ST_ERROR_INVALID_HANDLE;
    }

    STOS_SemaphoreWait(Handle_p->AccessMutex);

    /* Allocate buffer for data collection */
    if(ReceiverStartParams_p->BufferAddress == NULL)
    {
        Handle_p->Type.Receiver.BufferPtr = memory_allocate(Device_p->NCachePartition,STNETi_RECEIVER_BUFFER_SIZE+(STNETi_BUFFER_ALIGNMENT-1));
        if(Handle_p->Type.Receiver.BufferPtr == NULL)
        {
            STOS_SemaphoreSignal(Handle_p->AccessMutex);
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "memory_allocate(%d) failed\n", STNETi_RECEIVER_BUFFER_SIZE));
            return ST_ERROR_NO_MEMORY;
        }

        Handle_p->Type.Receiver.BufferBase = (U8 *)((((U32)Handle_p->Type.Receiver.BufferPtr)+STNETi_BUFFER_ALIGNMENT) & (~(STNETi_BUFFER_ALIGNMENT - 1)));
        Handle_p->Type.Receiver.BufferSize = STNETi_RECEIVER_BUFFER_SIZE;
    }
    else
    {
        if((((U32)ReceiverStartParams_p->BufferAddress) & (STNETi_BUFFER_ALIGNMENT -1)) ||
           (ReceiverStartParams_p->BufferSize & (STNETi_BUFFER_ALIGNMENT -1)))
        {
            STOS_SemaphoreSignal(Handle_p->AccessMutex);
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "buffer not aligned to %d bytes\n", STNETi_BUFFER_ALIGNMENT));
            return ST_ERROR_BAD_PARAMETER;
        }

        Handle_p->Type.Receiver.BufferPtr  = NULL;
        Handle_p->Type.Receiver.BufferBase = ReceiverStartParams_p->BufferAddress;
        Handle_p->Type.Receiver.BufferSize = ReceiverStartParams_p->BufferSize;
    }
    Handle_p->Type.Receiver.AllowOverflow  = ReceiverStartParams_p->AllowOverflow;
    Handle_p->Type.Receiver.BufferWritePtr = Handle_p->Type.Receiver.BufferBase;
    Handle_p->Type.Receiver.BufferReadPtr  = Handle_p->Type.Receiver.BufferBase;

    /* Create receiver task */
#ifndef ST_OSLINUX
    ngInetNTOA(Handle_p->Type.Receiver.ReceiverSocketAddress.sin_addr, &TaskName[strlen(TaskName)], STNETi_MAX_ADDRESS_LENGTH);
#else
    strcat(TaskName, (char *)inet_ntoa(Handle_p->Type.Receiver.TransmitterSocketAddress.sin_addr.s_addr));
#endif
    strcat(TaskName, ")");

    Handle_p->Type.Receiver.State = RECEIVER_RUNNING;

    if(STOS_TaskCreate((void (*)(void *))STNETi_ReceiverTask, Handle_p,
            Device_p->DriverPartition, STNETi_RECEIVER_TASK_SIZE, &Handle_p->Type.Receiver.Stack_p,
            Device_p->DriverPartition, &Handle_p->Type.Receiver.Task_p, &Handle_p->Type.Receiver.Tdesc,
            STNETi_RECEIVER_TASK_PRIORITY, TaskName, task_flags_no_min_stack_size) != ST_NO_ERROR)
    {
        memory_deallocate(Device_p->NCachePartition, Handle_p->Type.Receiver.BufferPtr);
        STOS_SemaphoreSignal(Handle_p->AccessMutex);
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "bind socket failed\n"));
        return ST_ERROR_NO_MEMORY;
    }

    STOS_SemaphoreSignal(Handle_p->AccessMutex);

    return ST_NO_ERROR;
}

/******************************************************************************
Name         : STNETi_Receiver_Stop()

Description  : Start receiver task to receive data from opened socket

Parameters   : STNET_Handle_t               Handle              handle returned by open
               STNET_ReceiverStartParams_t *ReceiverStartParams start parameters

Return Value : Error code
******************************************************************************/
ST_ErrorCode_t STNETi_Receiver_Stop(STNETi_Device_t *Device_p, U8 Instance)
{

    STNETi_Handle_t    *Handle_p ;

    Handle_p = STNETi_GetHandlefromList(Device_p,Instance);
    if(Handle_p == NULL)
    {
        return ST_ERROR_INVALID_HANDLE;
    }

    /* check if receiver is started */
    if(Handle_p->Type.Receiver.State == RECEIVER_STOPPED)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "receiver not active\n"));
        return STNET_ERROR_ALREADY_STOPPED;
    }

    STOS_SemaphoreWait(Handle_p->AccessMutex);

    /* request the task to exit */
    Handle_p->Type.Receiver.State = RECEIVER_STOPPED;

    /* delete task */
    STOS_TaskWait(&Handle_p->Type.Receiver.Task_p, TIMEOUT_INFINITY);
    STOS_TaskDelete(Handle_p->Type.Receiver.Task_p, Device_p->DriverPartition,
                    Handle_p->Type.Receiver.Stack_p,Device_p->DriverPartition);

    /* deallocate data buffer -- neeed to verify*/
    memory_deallocate(Device_p->NCachePartition, Handle_p->Type.Receiver.BufferPtr);

   STOS_SemaphoreSignal(Handle_p->AccessMutex);

    return ST_NO_ERROR;

}

/******************************************************************************
Name         : STNETi_ReceiverTask()

Description  : States: RUNNING, STOPPED
               read data from network socket
               dejitter
               copy to data buffer

Parameters   : STNETi_ReceiverHandle_t             *Handle_p            Receiver handle

Return Value : None
******************************************************************************/
static void STNETi_ReceiverTask(STNETi_Handle_t   *Handle_p)
{
    S32                     Err;
#ifndef ST_OSLINUX
    NGbuf                  *NGBuffer;
#else
    U32                     Size;
#endif
    U8                     *IPPacketBuffer;
    U32                     IPPacketSize;
    U8                     *TSPayloadData;
    U32                     TSPayloadSize;
    STNETi_RTP_Header_t     RTPHeader;
    U32                     RTPHeaderSize;
    U32                     RTPPaddingSize;
    U8                     *BufferWritePtr,
                           *BufferReadPtr;
    U32                     BufferFreeSize;
    U32                     SizeToCopy;
    U16                     PrevSeqNumber = 0xFFFF;
#ifndef ST_OSLINUX
#if defined(OSPLUS_INPUT_STREAM_BYPASS)
    clock_t                 Time2Wait;
    NGbuf                   *pkt;
    NGbuf                   **MsgReceived;
    NGifnet                 *netp;
#endif
#endif


    /* TODO: Handle dejitter */
#ifdef ST_OSLINUX
    IPPacketBuffer = memory_allocate(Handle_p->Device_p->DriverPartition, STNETi_RTP_PACKET_MAX_SIZE);
    if(IPPacketBuffer == NULL)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "memory_allocate(%d) failed\n", STNETi_RTP_PACKET_MAX_SIZE));
        return;
    }
#endif

    while(Handle_p->Type.Receiver.State == RECEIVER_RUNNING)
    {
        /* receive data from socket */
#ifndef ST_OSLINUX
#if !defined(OSPLUS_INPUT_STREAM_BYPASS)
        if((Err = ngSockBufRecv(Handle_p->Type.Receiver.SocketDescriptor, NULL, NULL, &NGBuffer)) != NG_EOK)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "socket receive failed Err[%d]\n", Err));
            continue;
        }

        if(NGBuffer == NULL)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Buffer not valid\n"));
            continue;
        }

        /* check for data */
        if(NGBuffer->buf_datap == NULL)
        {
            ngSockBufFree(Handle_p->Type.Receiver.SocketDescriptor, NGBuffer);
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Data not valid\n"));
            continue;
        }
        IPPacketBuffer = (U8*)NGBuffer->buf_datap;
        IPPacketSize   = (U32)NGBuffer->buf_datalen;
#else
        if((netp = ngIfGetPtr(ETHERNET_DEVICE_NAME)) == NULL)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Failed to get a pointer to network interface\n"));
        }
        /* wait on message queue to receive message otherwise  */
        Time2Wait = time_plus(time_now(), ST_GetClocksPerSecond()/20);

        MsgReceived = message_receive_timeout(STNETi_TaskMQ_p, &Time2Wait);

        if(MsgReceived!=NULL)
        {
            pkt = *MsgReceived;


            if((((char *)pkt->buf_datap)[0x24]==((char *)&Handle_p->Type.Receiver.TransmitterSocketAddress.sin_port)[0])&&(((char *)pkt->buf_datap)[0x25]==((char *)&Handle_p->Type.Receiver.TransmitterSocketAddress.sin_port)[1]))
            {
                /*STNETi_TraceDBG1(("\n !!!!!Valid Pkt found inside reciever task, size[%d]!!!!! \n",pkt->buf_datalen));*/
                IPPacketBuffer = pkt->buf_datap+42;
                IPPacketSize   = pkt->buf_datalen-42;
            }
            else
            {
                ngEtherInput(netp,pkt,1);
            }
            message_release(STNETi_TaskMQ_p, MsgReceived);
        }
#endif
#else
        Size         = sizeof(Handle_p->Type.Receiver.TransmitterSocketAddress);
        IPPacketSize = recvfrom(Handle_p->Type.Receiver.SocketDescriptor, (char *)IPPacketBuffer, STNETi_RTP_PACKET_MAX_SIZE,0,
                                (struct sockaddr *)&Handle_p->Type.Receiver.TransmitterSocketAddress,(socklen_t *)&Size);
#endif

        /* handle expected protocol */
        if(Handle_p->TransmissionProtocol == STNET_PROTOCOL_TS)
        {
            TSPayloadData = IPPacketBuffer;
            TSPayloadSize = IPPacketSize;
        }
        else if(Handle_p->TransmissionProtocol == STNET_PROTOCOL_RTP_TS)
        {
            if(IPPacketSize <= STNETi_RTP_HEADER_MIN_SIZE)
            {
#ifndef ST_OSLINUX
#if !defined(OSPLUS_INPUT_STREAM_BYPASS)
                ngSockBufFree(Handle_p->Type.Receiver.SocketDescriptor, NGBuffer);
#endif
#endif
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Empty packet\n"));
                continue;
            }

            /* parse header */
#if 0
            RTPHeader.Version        = ((IPPacketBuffer[0]) >> 6) & 0x03;
            RTPHeader.Padding        = ((IPPacketBuffer[0]) >> 5) & 0x01;
            RTPHeader.Extension      = ((IPPacketBuffer[0]) >> 4) & 0x01;
            RTPHeader.CSRCCount      = ((IPPacketBuffer[0]) >> 0) & 0x0F;
            RTPHeader.Marker         = ((IPPacketBuffer[1]) >> 7) & 0x01;
            RTPHeader.PayloadType    = ((IPPacketBuffer[1]) >> 0) & 0x7F;
            RTPHeader.SequenceNumber = (U16)(((U16)IPPacketBuffer[2] << 8) | IPPacketBuffer[3]);
            RTPHeader.TimeStamp      = (U32)(((U32)IPPacketBuffer[4] << 24) | ((U32)IPPacketBuffer[5] << 16) | ((U32)IPPacketBuffer[6] << 8) | ((U32)IPPacketBuffer[7] << 0));
            RTPHeader.SSRC           = (U32)(((U32)IPPacketBuffer[8] << 24) | ((U32)IPPacketBuffer[9] << 16) | ((U32)IPPacketBuffer[10] << 8) | ((U32)IPPacketBuffer[11] << 0));
#else
            STNETi_ENDIAN_MEMCPY_WORDS(&RTPHeader, IPPacketBuffer, sizeof(STNETi_RTP_Header_t));
#endif
            RTPHeaderSize = STNETi_RTP_HEADER_MIN_SIZE + (RTPHeader.CSRCCount * sizeof(U32));

            if(RTPHeader.SequenceNumber != (++PrevSeqNumber))
            {
                if(RTPHeader.SequenceNumber < PrevSeqNumber)
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Received past SequenceNumber Curr[%d] Prev[%d]\n",
                            RTPHeader.SequenceNumber, PrevSeqNumber - 1));
                }
                else
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "SequenceNumber mismatch Curr[%d] Prev[%d] DATA LOST\n",
                            RTPHeader.SequenceNumber, PrevSeqNumber - 1));
                }
                PrevSeqNumber = RTPHeader.SequenceNumber;
            }

            if(RTPHeader.PayloadType != STNETi_RTP_PAYLOAD_TYPE_TS)
            {
#ifndef ST_OSLINUX
#if !defined(OSPLUS_INPUT_STREAM_BYPASS)
                ngSockBufFree(Handle_p->Type.Receiver.SocketDescriptor, NGBuffer);
#endif
#endif
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "PayloadType[%d] is not type TS[%d] DROPPING DATA\n",
                        RTPHeader.PayloadType, STNETi_RTP_PAYLOAD_TYPE_TS));
                continue;
            }

            /* TODO: Handle Extension field */

            /* find padding bytes */
            RTPPaddingSize = (RTPHeader.Padding == 1) ? IPPacketBuffer[IPPacketSize - 1] : 0;

            TSPayloadData = IPPacketBuffer + RTPHeaderSize;
            TSPayloadSize = IPPacketSize - RTPHeaderSize - RTPPaddingSize;
#if 0
            STTBX_Print(("Version[%u] Padding[%u] Extension[%u] CSRCCount[%u] Marker[%u] PayloadType[%u] SequenceNumber[%u]" \
                         "TimeStamp[%u] SSRC[%u] TSPayloadSize[%u] TSPayloadData[%02X]\n" ,
                        RTPHeader.Version,
                        RTPHeader.Padding,
                        RTPHeader.Extension,
                        RTPHeader.CSRCCount,
                        RTPHeader.Marker,
                        RTPHeader.PayloadType,
                        RTPHeader.SequenceNumber,
                        RTPHeader.TimeStamp,
                        RTPHeader.SSRC,
                        TSPayloadSize,
                        TSPayloadData[0]));
#endif
        }

        if(((TSPayloadSize/STNETi_TS_PACKET_SIZE)*STNETi_TS_PACKET_SIZE) != TSPayloadSize)
        {
#ifndef ST_OSLINUX
#if !defined(OSPLUS_INPUT_STREAM_BYPASS)
            ngSockBufFree(Handle_p->Type.Receiver.SocketDescriptor, NGBuffer);
#endif
#endif
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TSPayloadSize[%d] not a multiple of TS Packet size[%d] DROPPING DATA\n",
                    TSPayloadSize, STNETi_TS_PACKET_SIZE));
            continue;
        }

#if 0
        /* SOP check */
        {
            U32      PckCount;

            for(PckCount = 0; PckCount < TSPayloadSize; PckCount += STNETi_TS_PACKET_SIZE)
            {
                if(TSPayloadData[PckCount] != 0x47)
                {
                    STTBX_Print(("SOP Error TSPayloadData[%02X]\n", TSPayloadData[PckCount]));
                }
            }
        }
#endif

        /* check buffer free size */
        BufferWritePtr = Handle_p->Type.Receiver.BufferWritePtr;
        BufferReadPtr  = Handle_p->Type.Receiver.BufferReadPtr;

        BufferFreeSize = (BufferReadPtr > BufferWritePtr) ?
                            (BufferReadPtr - BufferWritePtr) :
                            (Handle_p->Type.Receiver.BufferSize - (BufferWritePtr - BufferReadPtr));

        if(BufferFreeSize < TSPayloadSize)
        {
            if(Handle_p->Type.Receiver.AllowOverflow == FALSE)
            {
                /* TODO: check if we have to wait here b4 dropping  */
#ifndef ST_OSLINUX
#if !defined(OSPLUS_INPUT_STREAM_BYPASS)
                ngSockBufFree(Handle_p->Type.Receiver.SocketDescriptor, NGBuffer);
#endif
#endif
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Buffer free size[%d] not sufficient[%d] DROPPING DATA\n",
                        BufferFreeSize, TSPayloadSize));
                continue;
            }
            else
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Buffer free size[%d] not sufficient[%d] OVERFLOW\n",
                        BufferFreeSize, TSPayloadSize));
            }
        }

        /* copy payload */
        SizeToCopy = (Handle_p->Type.Receiver.BufferBase + Handle_p->Type.Receiver.BufferSize) - BufferWritePtr;
        if(TSPayloadSize <= SizeToCopy)
        {
            memcpy(BufferWritePtr, TSPayloadData, TSPayloadSize);
            BufferWritePtr += TSPayloadSize;
        }
        else
        {
            memcpy(BufferWritePtr, TSPayloadData, SizeToCopy);
            memcpy(Handle_p->Type.Receiver.BufferBase, TSPayloadData + SizeToCopy, TSPayloadSize - SizeToCopy);
            BufferWritePtr = Handle_p->Type.Receiver.BufferBase + (TSPayloadSize - SizeToCopy);
        }

        /* update write pointer */
        Handle_p->Type.Receiver.BufferWritePtr = BufferWritePtr;

#ifndef ST_OSLINUX
#if !defined(OSPLUS_INPUT_STREAM_BYPASS)
        ngSockBufFree(Handle_p->Type.Receiver.SocketDescriptor, NGBuffer);
#else
if((((char *)pkt->buf_datap)[0x24]==((char *)&Handle_p->Type.Receiver.ReceiverSocketAddress.sin_port)[0])&&(((char *)pkt->buf_datap)[0x25]==((char *)&Handle_p->Type.Receiver.ReceiverSocketAddress.sin_port)[1]))
{
    ngBufFree(pkt);
}
#endif
#endif
    }

#ifdef ST_OSLINUX
    memory_deallocate(Handle_p->Device_p->DriverPartition, IPPacketBuffer);
#endif
}


/* EOF --------------------------------------------------------------------- */

