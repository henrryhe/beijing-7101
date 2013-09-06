/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 * Improved by Marc Boucher <marc@mbsi.ca> and David Haas <dhaas@alum.rpi.edu>
 *
 */
/* 
Modifications of this software Copyright(C) 2008, Standard Microsystems Corporation
  All Rights Reserved.
  The modifications to this program code are proprietary to SMSC and may not be copied,
  distributed, or used without a license to do so.  Such license may have
  Limited or Restricted Rights. Please refer to the license for further
  clarification.
  2008-05-08  修改SOCKET接口支持connect,bind,send,receive,close等操作的超时控制
*/

#define CH_SOCKET_TIMEOUT_SUPPORT /**/  /*是否支持timeout功能*/

#include "smsc_environment.h"
#include "smsc_threading.h"
#include "sockets.h"
#include "task_manager.h"
#if TCP_ENABLED
#include "tcp.h"
#endif
#if UDP_ENABLED
#include "udp.h"
#endif

#if !SOCKETS_ENABLED
#error Sockets are not enabled
#endif

#if !SMSC_THREADING_ENABLED
#error Multithreading is not enabled
#endif


#ifdef CH_SOCKET_TIMEOUT_SUPPORT
#define SOCKET_CONNECT_TIME               5000                              /*5秒*/
#define SOCKET_DISCONNECT_TIME         2000                              /*5秒*/

#define SOCKET_BIND_TIME                      2000                              /*5秒*/
#define SOCKET_SEND_TIME                     5000                              /*5秒*/
#define SOCKET_RECEIVE_TIME                5000                             /*5秒*/
#define SOCKET_CLOSE_TIME                    5000                              /*5秒*/

#define LISTEN_RECEIVE_TIME                   -1                                    /*无限等待*/
#define ACCEPT_CLOSE_TIME                    -1                                 /*无限等待*/


#endif
#ifdef M_WUFEI_080922
#define	ENOMEM				-1/* ERR_MEM  -1      Out of memory error.     */
#define	ENOBUFS				-2/* ERR_BUF  -2      Buffer error.            */
#define	ECONNABORTED		-3/* ERR_ABRT -3      Connection aborted.      */
#define	ECONNRESET			-4/* ERR_RST  -4      Connection reset.        */
#define	ESHUTDOWN			-5/* ERR_CLSD -5      Connection closed.       */
#define	ENOTCONN			-6/* ERR_CONN -6      Not connected.           */
#define	EINVAL				-7/* ERR_VAL  -7      Illegal value.           */
#define	EIO					-8/* ERR_ARG  -8      Illegal argument.        */
#define	EHOSTUNREACH		-9/* ERR_RTE  -9      Routing problem.         */
#define	EADDRINUSE			-10/* ERR_USE  -10     Address in use.          */
#define	EWOULDBLOCK		-11/**/
#endif

static int err_to_errno_table[11] = {
	0,				/* ERR_OK    0      No error, everything OK. */
	ENOMEM,			/* ERR_MEM  -1      Out of memory error.     */
	ENOBUFS,		/* ERR_BUF  -2      Buffer error.            */
	ECONNABORTED,	/* ERR_ABRT -3      Connection aborted.      */
	ECONNRESET,		/* ERR_RST  -4      Connection reset.        */
	ESHUTDOWN,		/* ERR_CLSD -5      Connection closed.       */
	ENOTCONN,		/* ERR_CONN -6      Not connected.           */
	EINVAL,			/* ERR_VAL  -7      Illegal value.           */
	EIO,			/* ERR_ARG  -8      Illegal argument.        */
	EHOSTUNREACH,	/* ERR_RTE  -9      Routing problem.         */
	EADDRINUSE		/* ERR_USE  -10     Address in use.          */
};

#define ERR_TO_ERRNO_TABLE_SIZE \
	(sizeof(err_to_errno_table)/sizeof(err_to_errno_table[0]))

#define err_to_errno(err) \
	(-(err) >= 0 && -(err) < ERR_TO_ERRNO_TABLE_SIZE ? \
	err_to_errno_table[-(err)] : EIO)

#ifdef ERRNO
#define set_errno(err) errno = (err)
#else
#define set_errno(err)
#endif

#define sock_set_errno(socket, errorCode) do { \
	(socket)->mErrorCode = (errorCode); \
	set_errno((socket)->mErrorCode); \
} while (0)

#if SMSC_ERROR_ENABLED
#define SOCKET_SIGNATURE	(0x9E3ECFF5)
#endif
struct SOCKET 
{
	DECLARE_SIGNATURE
	struct SOCKET * mNext;
	u8_t mIsActive;
	
#define SOCKET_TYPE_UNDEFINED	(0)
	u8_t mType;
	union {
		void * mPointer;
	#if RAW_ENABLED
		#define SOCKET_TYPE_RAW	(1)
		/*struct raw_pcb *raw;*/
	#endif

	#if UDP_ENABLED
		#define SOCKET_TYPE_UDP	(2)
		struct UDP_CONTROL_BLOCK *mUdp;
	#endif

	#if TCP_ENABLED
		#define SOCKET_TYPE_TCP	(3)
		struct TCP_CONTROL_BLOCK *mTcp;
	#endif
	} mControlBlock;
	SMSC_SEMAPHORE mRequestLock;
#ifdef CH_SOCKET_TIMEOUT_SUPPORT

	int  connect_timeout_ms;
	int  disconnect_timeout_ms;
       int  bind_timeout_ms;
	int  send_timeout_ms;
	int  receive_timeout_ms;
	int  close_timeout_ms;

	int  listen_timeout_ms;
	int  accept_timeout_ms;

	SMSC_SEMAPHORE mResponseSignal;

#else
	SMSC_SEMAPHORE mResponseSignal;
#endif

#define REQUEST_UNDEFINED					(0)
#if UDP_ENABLED
#define REQUEST_CREATE_UDP_CONTROL_BLOCK	(1)
#endif
#if TCP_ENABLED
#define REQUEST_CREATE_TCP_CONTROL_BLOCK	(2)
#endif
#define REQUEST_BIND						(3)
#define REQUEST_CONNECT						(4)
#define REQUEST_DISCONNECT					(5)
#define REQUEST_SEND						(6)
#define REQUEST_RECEIVE						(7)
#define REQUEST_CLOSE						(8)
#define REQUEST_LISTEN						(9)
#define REQUEST_ACCEPT						(10)
	u8_t mRequestCode;

#define RESPONSE_UNDEFINED					(0)
#define RESPONSE_SUCCESSFUL					(1)
#define RESPONSE_UNSUCCESSFUL				(2)
#define RESPONSE_MEMORY_ERROR				(3)
#define RESPONSE_UNKNOWN_REQUEST			(4)
#define RESPONSE_ILLEGAL_OPERATION			(5)
#define RESPONSE_PENDING					(6)
#define RESPONSE_CONNECTION_RESET			(7)
	u8_t mResponseCode;
	
	union {
	#if UDP_ENABLED
		struct {
			u8_t mUseUdpLite;
		} mCreateUdpParams;
	#endif
		struct {
			IP_ADDRESS mIpAddress;
			u16_t mPort;
		} mBindParams;
		struct {
			IP_ADDRESS mIpAddress;
			u16_t mPort;
		} mConnectParams;
		struct {
			void * mData;
			int mSize;
			unsigned int mFlags;
		} mSendParams;
		struct {
			void * mData;
			int mSize;
			IP_ADDRESS mIpAddress;
			u16_t mPort;
		} mReceiveParams;
		struct {
			struct SOCKET * mNewSocket;
			IP_ADDRESS mFromIpAddress;
			u16_t mFromPort;
		} mAcceptParams;
	} mRequestParameters;
	
	int mErrorCode;
	int mSocketIndex;
	
	int mReceiveFinishedFlag;
	PACKET_QUEUE mReceiveQueue;
	
	#if TCP_ENABLED
	int mListenBackLog;
	struct TCP_CONTROL_BLOCK * mAcceptList[SOCKET_MAXIMUM_BACKLOG];
	u8_t mAcceptListSize;
	int mConnectionWasReset;
	#endif
	int mIsWritable;
	
	u16_t mFlags;
};

struct smsc_select_cb
{
	struct smsc_select_cb *next;
	fd_set *readset;
	fd_set *writeset;
	fd_set *exceptset;
	int sem_signalled;
	SMSC_SEMAPHORE sem;
};

static struct SOCKET sockets[MAXIMUM_NUMBER_OF_SOCKETS];
static SMSC_SEMAPHORE gSocketsLock;
static struct smsc_select_cb * gSelectList = NULL;

static struct SOCKET * gRequestQueueHead=NULL;
static struct SOCKET * gRequestQueueTail=NULL;
static SMSC_SEMAPHORE gRequestQueueLock;

static SMSC_SEMAPHORE gSelectLock;

static void SocketsInternal_RequestHandler(void * param);
static TASK gSocketTask;

static void Sockets_PushRequest(struct SOCKET * socket);
static struct SOCKET * Sockets_PopRequest(void);

#if UDP_ENABLED
static int SocketRequest_CreateUdpControlBlock(struct SOCKET * socket);
#endif
#if TCP_ENABLED
static int SocketRequest_CreateTcpControlBlock(struct SOCKET * socket);
#endif
static int SocketRequest_Bind(struct SOCKET * socket, PIP_ADDRESS ipAddress, u16_t port);
static err_t SocketRequest_Connect(struct SOCKET * socket, PIP_ADDRESS ipAddress, u16_t port);
static err_t SocketRequest_Disconnect(struct SOCKET * socket);
static err_t SocketRequest_GetPeer(struct SOCKET * socket,PIP_ADDRESS ipAddress, u16_t * port);
static int SocketRequest_Send(struct SOCKET * socket, void *data, int size, unsigned int flags);
static int SocketRequest_Receive(
	struct SOCKET * socket,
	void * data,
	int size,
	PIP_ADDRESS ipAddress,
	u16_t * port);
static int SocketRequest_Close(
	struct SOCKET * socket);
#if TCP_ENABLED
static err_t SocketRequest_Listen(struct SOCKET * socket);
static int SocketRequest_AcceptTcpControlBlock(
	struct SOCKET * listenerSocket,
	struct SOCKET * acceptedSocket,
	PIP_ADDRESS fromIpAddress,
	u16_t * fromPort);
#endif

void Sockets_Initialize()
{
	memset(sockets,0,sizeof(struct SOCKET)*MAXIMUM_NUMBER_OF_SOCKETS);
	gSocketsLock = smsc_semaphore_create(1);
	gRequestQueueHead=NULL;
	gRequestQueueTail=NULL;
	gRequestQueueLock = smsc_semaphore_create(1);
	gSelectLock = smsc_semaphore_create(1);
	gSelectList=NULL;
	Task_Initialize(&gSocketTask, TASK_PRIORITY_APPLICATION_NORMAL,
		SocketsInternal_RequestHandler,NULL);
	TaskManager_ScheduleByInterruptActivation(&gSocketTask);
}

static void Sockets_PushRequest(struct SOCKET * socket)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(socket!=NULL);
	CHECK_SIGNATURE(socket,SOCKET_SIGNATURE);
	SMSC_ASSERT(socket->mNext==NULL);
	
	smsc_semaphore_wait(gRequestQueueLock);
	
	if(gRequestQueueHead!=NULL) {
		SMSC_ASSERT(gRequestQueueTail!=NULL);
		SMSC_ASSERT(gRequestQueueTail->mNext==NULL);
		gRequestQueueTail->mNext=socket;
		gRequestQueueTail=socket;
	} else {
		SMSC_ASSERT(gRequestQueueTail==NULL);
		gRequestQueueTail=socket;
		gRequestQueueHead=socket;
	}
	
	smsc_semaphore_signal(gRequestQueueLock);
}

static struct SOCKET * Sockets_PopRequest(void)
{
	struct SOCKET * result=NULL;
	smsc_semaphore_wait(gRequestQueueLock);
	if(gRequestQueueHead!=NULL) {
		SMSC_ASSERT(gRequestQueueTail!=NULL);
		result=gRequestQueueHead;
		gRequestQueueHead=result->mNext;
		result->mNext=NULL;
		if(gRequestQueueHead==NULL) {
			gRequestQueueTail=NULL;
		}
	} else {
		SMSC_ASSERT(gRequestQueueTail==NULL);
	}
	smsc_semaphore_signal(gRequestQueueLock);
	return result;
}

static int alloc_socket(void)
{
	int i;

	/* Protect socket array */
	smsc_semaphore_wait(gSocketsLock);

	/* allocate a new socket identifier */
	for(i = 1/*0*/; i < MAXIMUM_NUMBER_OF_SOCKETS; ++i) {
		if (!sockets[i].mIsActive) {
			memset(&sockets[i],0,sizeof(struct SOCKET));
			ASSIGN_SIGNATURE(&(sockets[i]),SOCKET_SIGNATURE);
			sockets[i].mIsActive=1;
			sockets[i].mRequestLock = smsc_semaphore_create(1);
#ifdef CH_SOCKET_TIMEOUT_SUPPORT
			sockets[i].connect_timeout_ms = SOCKET_CONNECT_TIME;
			sockets[i].disconnect_timeout_ms = SOCKET_CONNECT_TIME;
			sockets[i].bind_timeout_ms = SOCKET_BIND_TIME;
			sockets[i].send_timeout_ms = SOCKET_SEND_TIME;
			sockets[i].receive_timeout_ms = SOCKET_RECEIVE_TIME;
			sockets[i].close_timeout_ms = SOCKET_CLOSE_TIME;   
			sockets[i].listen_timeout_ms = SOCKET_RECEIVE_TIME;
			sockets[i].accept_timeout_ms = SOCKET_CLOSE_TIME;   	
			sockets[i].mResponseSignal = smsc_semaphore_create(0);
#else			
			sockets[i].mResponseSignal = smsc_semaphore_create(0);
#endif
			sockets[i].mRequestCode=REQUEST_UNDEFINED;
			sockets[i].mResponseCode=RESPONSE_UNDEFINED;
			sockets[i].mErrorCode=0;
			sockets[i].mSocketIndex=i;
			sockets[i].mIsWritable=1;
			sockets[i].mReceiveFinishedFlag=0;
			PacketQueue_Initialize(&(sockets[i].mReceiveQueue));
			smsc_semaphore_signal(gSocketsLock);
			return i;
		}
	}
	smsc_semaphore_signal(gSocketsLock);
	return -1;
}

static void free_socket(int socketIndex)
{           
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(socketIndex>=0);
	SMSC_ASSERT(socketIndex<MAXIMUM_NUMBER_OF_SOCKETS);
	smsc_semaphore_wait(gSocketsLock);
	CHECK_SIGNATURE(&(sockets[socketIndex]),SOCKET_SIGNATURE);
	SMSC_ASSERT(sockets[socketIndex].mIsActive);
	sockets[socketIndex].mIsActive=0;
	ASSIGN_SIGNATURE(&(sockets[socketIndex]),SOCKET_SIGNATURE);
	smsc_semaphore_signal(gSocketsLock);
}

static struct SOCKET * get_socket(int socketIndex)
{
	struct SOCKET * socket=NULL;
	SMSC_ASSERT(socketIndex>=0);
	SMSC_ASSERT(socketIndex<MAXIMUM_NUMBER_OF_SOCKETS);
	socket=&(sockets[socketIndex]);
	SMSC_ASSERT(socket->mIsActive);
	return socket;
}

int smsc_socket(int domain, int type, int protocol)
{
	int socketIndex=-1;
	struct SOCKET * socket=NULL;
       
	socketIndex=alloc_socket();

	if (socketIndex == -1) {
		set_errno(ENOBUFS);
		goto ERROR;
	}

	socket=get_socket(socketIndex);

	switch(type) {
	case SOCK_RAW:
		SMSC_ERROR(("smsc_socket: Raw sockets are not supported"));
		set_errno(EINVAL);
		goto ERROR;
#if UDP_ENABLED
	case SOCK_DGRAM:
		if(SocketRequest_CreateUdpControlBlock(socket)!=0) {
			set_errno(ENOBUFS);
			goto ERROR;
		}
		break;
#endif
#if TCP_ENABLED
	case SOCK_STREAM:
		if(SocketRequest_CreateTcpControlBlock(socket)!=0) {
			set_errno(ENOBUFS);
			goto ERROR;
		}
		break;
#endif
	default:
		SMSC_ERROR(("smsc_socket: unsupported type = %d",type));
		set_errno(EINVAL);
		goto ERROR;
	}
	set_errno(0);
	return socketIndex;
ERROR:
	if(socketIndex>=0) {
		free_socket(socketIndex);
		socketIndex=-1;
	}
	return -1;
}

int smsc_accept(int socketIndex, struct sockaddr * addr, socklen_t * addrlen)
{
#if TCP_ENABLED
	int newSocketIndex=-1;
	struct SOCKET * newSocket=NULL;
    struct SOCKET * socket=NULL;
    IP_ADDRESS ipAddress;
    u16_t port;

    socket=get_socket(socketIndex);

	newSocketIndex=alloc_socket();

	if (newSocketIndex == -1) {
		set_errno(ENOBUFS);
		goto ERROR;
	}

	newSocket=get_socket(newSocketIndex);
    
    /* only TCP sockets can use the accept call */
	SMSC_ASSERT(socket->mType==SOCKET_TYPE_TCP);
	
	if(SocketRequest_AcceptTcpControlBlock(socket,newSocket,&ipAddress,&port)!=0) {
		SMSC_ERROR(("smsc_accept: failed to accept tcp control block"));
		set_errno(ENOBUFS);
		goto ERROR;
	}
	if((addr!=NULL)&&(addrlen!=NULL)&&((*addrlen)>=sizeof(struct sockaddr_in)))
	{
		SMSC_ASSERT(IP_ADDRESS_IS_IPV4(&ipAddress));
		((struct sockaddr_in *)addr)->sin_len=sizeof(struct sockaddr_in);
		((struct sockaddr_in *)addr)->sin_family=AF_INET;
		IPV4_ADDRESS_COPY_FROM_IP_ADDRESS(&(((struct sockaddr_in *)addr)->sin_addr.s_addr),&ipAddress);
		((struct sockaddr_in *)addr)->sin_port=ntohs(port);
	}
	set_errno(0);
	return newSocketIndex;
ERROR:
	if(newSocketIndex>=0) {
		free_socket(newSocketIndex);
		newSocketIndex=-1;
	}
#endif /* TCP_ENABLED */
	return -1;
}

int smsc_bind(int socketIndex, struct sockaddr *name, socklen_t namelen)
{
	struct SOCKET * socket;
	IP_ADDRESS local_addr;
	u16_t local_port;
	err_t error;

	socket = get_socket(socketIndex);
	if (!socket) {
		set_errno(EBADF);
		return -1;
	}

	SMSC_ASSERT(name->sa_family==AF_INET);
	IP_ADDRESS_COPY_FROM_IPV4_ADDRESS(&local_addr,&(((struct sockaddr_in *)name)->sin_addr.s_addr));
	local_port = ((struct sockaddr_in *)name)->sin_port;

	#if SMSC_TRACE_ENABLED
	{
		char addressString[IP_ADDRESS_STRING_SIZE];
		SMSC_TRACE(SOCKETS_DEBUG, ("smsc_bind(%d, addr=%s port=%u)",
			socketIndex,IP_ADDRESS_TO_STRING(addressString,&local_addr),
			ntohs(local_port)));
	}
	#endif

	error = SocketRequest_Bind(socket, &local_addr, ntohs(local_port));

	if (error != ERR_OK) {
		SMSC_WARNING(SOCKETS_DEBUG, ("smsc_bind(%d) failed, error=%d", socketIndex, error));
		sock_set_errno(socket, err_to_errno(error));
		return -1;
	}

	SMSC_TRACE(SOCKETS_DEBUG, ("smsc_bind(%d) succeeded", socketIndex));
	sock_set_errno(socket, 0);
	return 0;
}

int smsc_close(int s)
{
	struct SOCKET * socket;

	SMSC_TRACE(SOCKETS_DEBUG, ("smsc_close(%d)", s));

	/* We cannot allow multiple closes of the same socket. */
	smsc_semaphore_wait(gSocketsLock);

	socket = get_socket(s);
	if (!socket) {
		smsc_semaphore_signal(gSocketsLock);
		set_errno(EBADF);
		return -1;
	}

	SocketRequest_Close(socket);

	ASSIGN_SIGNATURE(socket,0);
	socket->mIsActive=0;
#if 1 	/*Flex++ 2009/8/20 8:54:12*/
	smsc_semaphore_free(socket->mRequestLock);
	smsc_semaphore_free(socket->mResponseSignal);
#endif 	/*Flex++ 2009/8/20 8:54:12*/
	smsc_semaphore_signal(gSocketsLock);
	sock_set_errno(socket, 0);
	return 0;
}

int smsc_connect(int s, struct sockaddr *name, socklen_t namelen)
{
	struct SOCKET *socket;
	err_t err;

	socket = get_socket(s);
	if (!socket) {
		set_errno(EBADF);
		return -1;
	}

	if (((struct sockaddr_in *)name)->sin_family == AF_UNSPEC) {
		SMSC_NOTICE(SOCKETS_DEBUG, ("smsc_connect(%d, AF_UNSPEC)", s));
		err = SocketRequest_Disconnect(socket);
	} else {
		IP_ADDRESS remote_addr;
		u16_t remote_port;

		SMSC_ASSERT(name->sa_family==AF_INET);/* only Ipv4 is currently supported */
		IP_ADDRESS_COPY_FROM_IPV4_ADDRESS(&remote_addr,&(((struct sockaddr_in *)name)->sin_addr.s_addr));
		remote_port = ((struct sockaddr_in *)name)->sin_port;

		#if SMSC_TRACE_ENABLED
		{
			char addressString[IP_ADDRESS_STRING_SIZE];
			SMSC_TRACE(SOCKETS_DEBUG, ("smsc_connect(%d, addr=%s port=%u)", s,
				IP_ADDRESS_TO_STRING(addressString,&remote_addr),ntohs(remote_port)));
		}
		#endif

		err= SocketRequest_Connect(socket, &remote_addr, ntohs(remote_port));
	}

	if (err != ERR_OK) {
		SMSC_NOTICE(SOCKETS_DEBUG, ("smsc_connect(%d) failed, err=%d", s, err));
		sock_set_errno(socket, err_to_errno(err));
		return -1;
	}

	SMSC_TRACE(SOCKETS_DEBUG, ("smsc_connect(%d) succeeded", s));
	sock_set_errno(socket, 0);
	return 0;
}
int smsc_disconnect(int s)
{
	struct SOCKET *socket;
	err_t err;

	socket = get_socket(s);
	if (!socket) {
		set_errno(EBADF);
		return -1;
	}

	
	err = SocketRequest_Disconnect(socket);

	if (err != ERR_OK) {
		SMSC_NOTICE(SOCKETS_DEBUG, ("smsc_connect(%d) failed, err=%d", s, err));
		sock_set_errno(socket, err_to_errno(err));
		return -1;
	}

	SMSC_TRACE(SOCKETS_DEBUG, ("smsc_connect(%d) succeeded", s));
	sock_set_errno(socket, 0);
	return 0;
}
int smsc_listen(int s, int backlog)
{
#if TCP_ENABLED
	struct SOCKET * socket;
	err_t err;

	SMSC_TRACE(SOCKETS_DEBUG, ("smsc_listen(%d, backlog=%d)", s, backlog));
	socket = get_socket(s);
	if (!socket) {
		set_errno(EBADF);
		return -1;
	}
	if(backlog>SOCKET_MAXIMUM_BACKLOG) {
		set_errno(EVAL);
		return -1;
	}
	socket->mListenBackLog=backlog;
	err=SocketRequest_Listen(socket);
	
	if(err!=ERR_OK) {
		SMSC_TRACE(SOCKETS_DEBUG, ("smsc_listen(%d) failed, err=%d", s, err));
		sock_set_errno(socket, err_to_errno(err));
		return -1;
	}

	sock_set_errno(socket, 0);
	return 0;
#else
	set_errno(EVAL);
	return -1;
#endif
}

int smsc_recvfrom(int s, void *mem, int len, unsigned int flags,
	struct sockaddr *from, socklen_t *fromlen)
{
	struct SOCKET *socket;
	int result=-1;

	SMSC_TRACE(SOCKETS_DEBUG, ("smsc_recvfrom(%d, %p, %d, 0x%x, ..)", s, mem, len, flags));
	socket = get_socket(s);
	if (!socket) {
		set_errno(EBADF);
		return -1;
	}

	/* If this is non-blocking call, then check first */
	if (((flags & MSG_DONTWAIT) || (socket->mFlags & O_NONBLOCK))
		&& ((!(PacketQueue_IsEmpty(&(socket->mReceiveQueue))))||(socket->mReceiveFinishedFlag)) )
	{
		SMSC_TRACE(SOCKETS_DEBUG, ("smsc_recvfrom(%d): returning EWOULDBLOCK", s));
		sock_set_errno(socket, EWOULDBLOCK);
		return -1;
	}
	
	{
		IP_ADDRESS ipAddress;
		u16_t port;
		result=SocketRequest_Receive(socket,mem,len,&ipAddress,&port);
		
		#if SMSC_TRACE_ENABLED
		{
			char addressString[IP_ADDRESS_STRING_SIZE];
			SMSC_TRACE(SOCKETS_DEBUG, ("smsc_recvfrom(%d): addr=%s port=%u, size=%d", s,
				IP_ADDRESS_TO_STRING(addressString,&ipAddress),port,result));
		}
		#endif
		if (from && fromlen) {
			struct sockaddr_in sin;

			SMSC_ASSERT(IP_ADDRESS_IS_IPV4(&ipAddress));

			memset(&sin, 0, sizeof(sin));
			sin.sin_len = sizeof(sin);
			sin.sin_family = AF_INET;
			sin.sin_port = htons(port);
			
			IPV4_ADDRESS_COPY_FROM_IP_ADDRESS(
				&(sin.sin_addr.s_addr),&ipAddress);

			if (*fromlen > sizeof(sin))
				*fromlen = sizeof(sin);

			memcpy(from, &sin, *fromlen);
		}
	}
	if(result>=0) {
		sock_set_errno(socket, 0);
	} else {
		sock_set_errno(socket, ECONNRESET);
	}
	return result;
}

int smsc_recv(int s, void *mem, int len, unsigned int flags)
{
	return smsc_recvfrom(s, mem, len, flags, NULL, NULL);
}

int smsc_send(int s, void *data, int size, unsigned int flags)
{
	struct SOCKET * socket;

	SMSC_TRACE(SOCKETS_DEBUG, ("smsc_send(%d, data=%p, size=%d, flags=0x%x)", s, data, size, flags));

	socket = get_socket(s);
	if (!socket) {
		set_errno(EBADF);
		return -1;
	}

	size=SocketRequest_Send(socket,data,size,flags);
	if(size<0) {
		SMSC_TRACE(SOCKETS_DEBUG, ("smsc_send(%d) err=%d", s, size));
		sock_set_errno(socket, err_to_errno(size));
		return -1;
	}

	SMSC_TRACE(SOCKETS_DEBUG, ("smsc_send(%d) ok size=%d", s, size));
	sock_set_errno(socket, 0);
	return size;
}

int smsc_sendto(int s, void *data, int size, unsigned int flags,
	struct sockaddr *to, socklen_t tolen)
{
	struct SOCKET * socket;
	IP_ADDRESS remote_addr, addr;
	u16_t remote_port, port;
	int ret,connected;

	socket = get_socket(s);
	if (!socket) {
		set_errno(EBADF);
		return -1;
	}

	/* get the peer if currently connected */
	connected = (SocketRequest_GetPeer(socket, &addr, &port) == ERR_OK);

	SMSC_ASSERT(to!=NULL);
	SMSC_ASSERT(to->sa_family==AF_INET);/* Only Ipv4 is suppored currently */
	
	IP_ADDRESS_COPY_FROM_IPV4_ADDRESS(&remote_addr,&((struct sockaddr_in *)to)->sin_addr.s_addr);
	remote_port = ((struct sockaddr_in *)to)->sin_port;

	#if SMSC_TRACE_ENABLED
	{
		char addressString[IP_ADDRESS_STRING_SIZE];
		SMSC_TRACE(SOCKETS_DEBUG, ("smsc_sendto(%d, data=%p, size=%d, flags=0x%x to=%s port=%u", s, data, size, flags,
			IP_ADDRESS_TO_STRING(addressString,&remote_addr),ntohs(remote_port)));
	}
	#endif

	SocketRequest_Connect(socket, &remote_addr, ntohs(remote_port));

	ret = smsc_send(s, data, size, flags);

	/* reset the remote address and port number
		of the connection */
	if (connected)
		SocketRequest_Connect(socket, &addr, port);
	else
		SocketRequest_Disconnect(socket);
	return ret;
}

static int smsc_selscan(int maxfdp1, fd_set *readset, fd_set *writeset, fd_set *exceptset)
{
	int i, nready = 0;
	fd_set lreadset, lwriteset, lexceptset;
	struct SOCKET *p_sock;

	FD_ZERO(&lreadset);
	FD_ZERO(&lwriteset);
	FD_ZERO(&lexceptset);
 
	/* Go through each socket in each list to count number of sockets which
	currently match */
	for(i = 0; i < maxfdp1; i++)
	{
		if (FD_ISSET(i, readset))
		{
			/* See if netconn of this socket is ready for read */
			p_sock = get_socket(i);
			if (p_sock && (!PacketQueue_IsEmpty(&(p_sock->mReceiveQueue))))
			{
				FD_SET(i, &lreadset);
				SMSC_TRACE(SOCKETS_DEBUG, ("smsc_selscan: fd=%d ready for reading", i));
				nready++;
			}
#if 1
			/*zxg 20081020增加如果SOCKET服务器连接已经关闭，也认为有数据可读*/
			else if(p_sock == NULL || p_sock->mControlBlock.mTcp == NULL || p_sock->mReceiveFinishedFlag/*p_sock->mControlBlock.mTcp->mState == TCP_STATE_CLOSED*/)
			{
				FD_SET(i, &lreadset);
				do_report(0,"smsc_selscan: fd[%d] closed\n", i);
				nready++;
			}
		/***************************************************************************************/
#endif
		}
		if (FD_ISSET(i, writeset))
		{
			/* See if netconn of this socket is ready for write */
			p_sock = get_socket(i);
			if (p_sock && (p_sock->mIsWritable))
			{
				FD_SET(i, &lwriteset);
				SMSC_TRACE(SOCKETS_DEBUG, ("smsc_selscan: fd=%d ready for writing", i));
				nready++;
			}
		}
	}
	*readset = lreadset;
	*writeset = lwriteset;
	FD_ZERO(exceptset);

	return nready;
}


int smsc_select(
	int maxfdp1,
	fd_set *readset,
	fd_set *writeset,
	fd_set *exceptset,
	struct timeval *timeout)
{
	s32_t i;
	int nready;
	fd_set lreadset, lwriteset, lexceptset;
	u32_t msectimeout;
	struct smsc_select_cb select_cb;
	struct smsc_select_cb *p_selcb;

	SMSC_TRACE(SOCKETS_DEBUG, ("smsc_select(%d, %p, %p, %p, tvsec=%ld tvusec=%ld)",
		maxfdp1, (void *)readset, (void *) writeset, (void *) exceptset,
		timeout ? timeout->tv_sec : -1L, timeout ? timeout->tv_usec : -1L));

	select_cb.next = 0;
	select_cb.readset = readset;
	select_cb.writeset = writeset;
	select_cb.exceptset = exceptset;
	select_cb.sem_signalled = 0;

	/* Protect ourselves searching through the list */
	smsc_semaphore_wait(gSelectLock);

	if (readset)
		lreadset = *readset;
	else
		FD_ZERO(&lreadset);
	if (writeset)
		lwriteset = *writeset;
	else
		FD_ZERO(&lwriteset);
	if (exceptset)
		lexceptset = *exceptset;
	else
		FD_ZERO(&lexceptset);

	/* Go through each socket in each list to count number of sockets which
	currently match */
	nready = smsc_selscan(maxfdp1, &lreadset, &lwriteset, &lexceptset);

	/* If we don't have any current events, then suspend if we are supposed to */
	if (!nready)
	{
		if (timeout && timeout->tv_sec == 0 && timeout->tv_usec == 0)
		{
			smsc_semaphore_signal(gSelectLock);
			if (readset)
				FD_ZERO(readset);
			if (writeset)
				FD_ZERO(writeset);
			if (exceptset)
				FD_ZERO(exceptset);

			SMSC_TRACE(SOCKETS_DEBUG, ("smsc_select: no timeout, returning 0"));
			set_errno(0);

			return 0;
		}

		/* add our semaphore to list */
		/* We don't actually need any dynamic memory. Our entry on the
		* list is only valid while we are in this function, so it's ok
		* to use local variables */

		select_cb.sem =smsc_semaphore_create(0)/* smsc_semaphore_create(0)*/;
		
		/* Note that we are still protected */
		/* Put this select_cb on top of list */
		select_cb.next = gSelectList;
		gSelectList = &select_cb;

		/* Now we can safely unprotect */
		smsc_semaphore_signal(gSelectLock);

		/* Now just wait to be woken */
		if (timeout == 0) {
			/* Wait forever */
			i=SMSC_TIMEOUT-1;/* set i to something other that SMSC_TIMEOUT */
			smsc_semaphore_wait(select_cb.sem);
		} else 
		{
#ifndef M_WUFEI_080922
			msectimeout =  ((timeout->tv_sec * 1000) + ((timeout->tv_usec + 500)/1000));
#else
			/*最小及时单位为ms，故把时间结构体中us放大成ms*/
			msectimeout =  ((timeout->tv_sec * 1000) + ((timeout->tv_usec * 1000)/1000));
#endif
			i = smsc_semaphore_wait_timeout(select_cb.sem, msectimeout);
		}

		/* Take us off the list */
		smsc_semaphore_wait(gSelectLock);
		if (gSelectList == &select_cb)
			gSelectList = select_cb.next;
		else for (p_selcb = gSelectList; p_selcb; p_selcb = p_selcb->next) {
			if (p_selcb->next == &select_cb)
			{
				p_selcb->next = select_cb.next;
				break;
			}
		}

		smsc_semaphore_signal(gSelectLock);

		smsc_semaphore_free(select_cb.sem);
		if (i == SMSC_TIMEOUT)             /* Timeout */
		{
			if (readset)
				FD_ZERO(readset);
			if (writeset)
				FD_ZERO(writeset);
			if (exceptset)
				FD_ZERO(exceptset);

			SMSC_TRACE(SOCKETS_DEBUG, ("smsc_select: timeout expired"));
			set_errno(0);

			return 0;
		}

		if (readset)
			lreadset = *readset;
		else
			FD_ZERO(&lreadset);
		if (writeset)
			lwriteset = *writeset;
		else
			FD_ZERO(&lwriteset);
		if (exceptset)
			lexceptset = *exceptset;
		else
			FD_ZERO(&lexceptset);

		/* See what's set */
		nready = smsc_selscan(maxfdp1, &lreadset, &lwriteset, &lexceptset);
	} else {
		smsc_semaphore_signal(gSelectLock);
	}

	if (readset)
		*readset = lreadset;
	if (writeset)
		*writeset = lwriteset;
	if (exceptset)
		*exceptset = lexceptset;

	SMSC_TRACE(SOCKETS_DEBUG, ("smsc_select: nready=%d", nready));
	set_errno(0);

	return nready;
}

int smsc_shutdown(int s, int how)
{
	SMSC_WARNING(SOCKETS_DEBUG, ("smsc_shutdown(%d, how=%d)", s, how));
	return smsc_close(s); /* XXX temporary hack until proper implementation */
}

int smsc_getpeername (int s, struct sockaddr *name, socklen_t *namelen)
{
	SMSC_ERROR(("smsc_getpeername: not implemented"));
	return 0;
}

int smsc_getsockname (int s, struct sockaddr *name, socklen_t *namelen)
{
	SMSC_ERROR(("smsc_getsockname: not implemented"));
	return 0;
}

int smsc_getsockopt (int s, int level, int optname, void *optval, socklen_t *optlen)
{
	SMSC_ERROR(("smsc_getsockopt: not implemented"));
	return 0;
}

int smsc_setsockopt (int s, int level, int optname, const void *optval, socklen_t optlen)
{
#ifndef CH_SOCKET_TIMEOUT_SUPPORT

	SMSC_ERROR(("smsc_setsockopt: not implemented"));
#else
{


	struct SOCKET * socket;


	socket = get_socket(s);
	if (!socket) {
		return -1;
	}
   if(SOL_SOCKET == level)
   	{
   	    switch(optname)
   	    	{
   	    	case SOCKET_CONNECT:
			  socket->connect_timeout_ms = (int)((int *)optval);
			 break;
              case SOCKET_DISCONNECT :
                      socket->disconnect_timeout_ms = (int)((int *)optval);
			  break;
		case SOCKET_BIND:
		        socket->bind_timeout_ms =(int) ((int *)optval);
			 break;
              case SOCKET_SEND:
                      socket->send_timeout_ms = (int)((int *)optval);			  	
			  break;
             case  SOCKET_RECEIVE:
                      socket->receive_timeout_ms =(int) ((int *)optval);			  	
		        break;
             case  SOCKET_CLOSE:
                      socket->close_timeout_ms =(int) ((int *)optval);			  				 	
			 break;
             case SOCKET_LISTEN:
			 socket->listen_timeout_ms = (int)((int *)optval);			  				 	 
		       break;
            case SOCKET_ACCEPT:
        		 socket->accept_timeout_ms = (int)((int *)optval);			  				 	 
			break;
   	    	}

   	}
   else
   	{
   	    return -1;
   	}
	

return 0;
	
}
#endif
}

int smsc_read(int s, void *mem, int len)
{
	return smsc_recvfrom(s, mem, len, 0, NULL, NULL);
}

int smsc_write(int s, void *dataptr, int size)
{
	return smsc_send(s, dataptr, size, 0);
}

int smsc_ioctl(int s, long cmd, void *argp)
{
#ifndef CH_SOCKET_TIMEOUT_SUPPORT
	SMSC_ERROR(("smsc_ioctl: not implemented"));
#else
	struct SOCKET * socket;
	int times=1;

	if(argp)
		times = *(int *)argp;


	socket = get_socket(s);
	if (!socket) {
		return -1;
	}

	if(O_NONBLOCK == cmd)
		{
	              socket->connect_timeout_ms = SOCKET_CONNECT_TIME*times;
  	              socket->disconnect_timeout_ms = SOCKET_CONNECT_TIME*times;
                     socket->bind_timeout_ms = SOCKET_BIND_TIME*times;
	              socket->send_timeout_ms = SOCKET_SEND_TIME*times;
              	socket->receive_timeout_ms = SOCKET_RECEIVE_TIME*times;
              	socket->close_timeout_ms = SOCKET_CLOSE_TIME*times;   
			socket->listen_timeout_ms = SOCKET_RECEIVE_TIME*times;
              	socket->accept_timeout_ms = SOCKET_CLOSE_TIME*times;   	
		}
	else/*阻赛工作模式*/
		{
              socket->connect_timeout_ms = -1;
  	              socket->disconnect_timeout_ms = -1;
                     socket->bind_timeout_ms = -1;
	              socket->send_timeout_ms = -1;
              	socket->receive_timeout_ms = -1;
              	socket->close_timeout_ms = -1;   
			socket->listen_timeout_ms = -1;
              	socket->accept_timeout_ms = -1;   
		}
	return 0;
#endif
}
 
#define in_range(c, lo, up)  ((u8_t)c >= lo && (u8_t)c <= up)
#define isprint(c)           in_range(c, 0x20, 0x7f)
#define isdigit(c)           in_range(c, '0', '9')
#define isxdigit(c)          (isdigit(c) || in_range(c, 'a', 'f') || in_range(c, 'A', 'F'))
#define islower(c)           in_range(c, 'a', 'z')
#define isspace(c)           (c == ' ' || c == '\f' || c == '\n' || c == '\r' || c == '\t' || c == '\v')

/*
 * Ascii internet address interpretation routine.
 * The value returned is in network order.
 */
u32_t inet_addr(const char *cp)
{
	struct in_addr val;

	if (inet_aton(cp, &val)) {
		return (val.s_addr);
	}
	return (u32_t)(0xFFFFFFFF);
}

/*
 * Check whether "cp" is a valid ascii representation
 * of an Internet address and convert to a binary address.
 * Returns 1 if the address is valid, 0 if not.
 * This replaces inet_addr, the return value from which
 * cannot distinguish between failure and a local broadcast address.
 */
int inet_aton(const char *cp, struct in_addr *addr)
{
	u32_t val;
	int base, n, c;
	u32_t parts[4];
	u32_t *pp = parts;

	c = *cp;
	for (;;) {
		/*
		* Collect number up to ``.''.
		* Values are specified as for C:
		* 0x=hex, 0=octal, 1-9=decimal.
		*/
		if (!isdigit(c))
			return (0);
		val = 0;
		base = 10;
		if (c == '0') {
			c = *++cp;
			if (c == 'x' || c == 'X') {
				base = 16;
				c = *++cp;
			} else
				base = 8;
		}
		for (;;) {
			if (isdigit(c)) {
				val = (val * base) + (int)(c - '0');
				c = *++cp;
			} else if (base == 16 && isxdigit(c)) {
				val = (val << 4) | (int)(c + 10 - (islower(c) ? 'a' : 'A'));
				c = *++cp;
			} else
				break;
		}
		if (c == '.') {
			/*
			* Internet format:
			*  a.b.c.d
			*  a.b.c   (with c treated as 16 bits)
			*  a.b (with b treated as 24 bits)
			*/
			if (pp >= parts + 3)
				return (0);
			*pp++ = val;
			c = *++cp;
		} else
			break;
	}
	/*
	* Check for trailing characters.
	*/
	if (c != '\0' && (!isprint(c) || !isspace(c)))
		return (0);
	/*
	* Concoct the address according to
	* the number of parts specified.
	*/
	n = pp - parts + 1;
	switch (n) {

	case 0:
		return (0);       /* initial nondigit */

	case 1:             /* a -- 32 bits */
		break;

	case 2:             /* a.b -- 8.24 bits */
		if (val > 0xffffff)
			return (0);
		val |= parts[0] << 24;
		break;

	case 3:             /* a.b.c -- 8.8.16 bits */
		if (val > 0xffff)
			return (0);
		val |= (parts[0] << 24) | (parts[1] << 16);
		break;

	case 4:             /* a.b.c.d -- 8.8.8.8 bits */
		if (val > 0xff)
			return (0);
		val |= (parts[0] << 24) | (parts[1] << 16) | (parts[2] << 8);
		break;
	}
	if (addr)
		addr->s_addr = htonl(val);
	return (1);
}

/* Convert numeric IP address into decimal dotted ASCII representation.
 * returns ptr to static buffer; not reentrant!
 */
char * inet_ntoa(struct in_addr addr)
{
	static char str[16];
	u32_t s_addr = addr.s_addr;
	char inv[3];
	char *rp;
	u8_t *ap;
	u8_t rem;
	u8_t n;
	u8_t i;

	rp = str;
	ap = (u8_t *)&s_addr;
	for(n = 0; n < 4; n++) {
		i = 0;
		do {
			rem = *ap % (u8_t)10;
			*ap /= (u8_t)10;
			inv[i++] = '0' + rem;
		} while(*ap);
		while(i--)
			*rp++ = inv[i];
		*rp++ = '.';
		ap++;
	}
	*--rp = 0;
	return str;
}
/*--------------------------------------------------------------------------*/
/* 函数名称 : inet_atoh															*/
/* 功能描述 : IP地址格式转换函数.点序->主机字节序								*/
/* 入口参数 : <addr>[in]需要转换的IP地址(形如:x.x.x.x)							*/
/* 出口参数 : 无																*/
/* 返回值:	成功，返回转换后的主机字节序的IP地址,，失败返回0					*/
/*--------------------------------------------------------------------------*/
u32_t inet_atoh(char *addr)
{
	u32_t retval = 0,i;
	u32_t buf[4] = {0};

	if(NULL == addr)
	{
		return 0;
	}
	sscanf(addr,"%d.%d.%d.%d",\
			&buf[3],\
			&buf[2],\
			&buf[1],\
			&buf[0]);
	for(i = 0;i < 4;i++)
		retval += buf[i]<<(8*i);
	return retval;
}

/*--------------------------------------------------------------------------*/
/* 函数名称 : inet_htoa															*/
/* 功能描述 : IP地址格式转换函数.主机字节序->点序								*/
/* 入口参数 : <addr_src>[in]需要转换的IP地址									*/
/* 出口参数 : <addr_dst>[out]转换后的点序IP地址									*/
/* 返回值:	无																*/
/*--------------------------------------------------------------------------*/
void inet_htoa(u32_t addr_src,s8_t *addr_dst )
{
	if(addr_dst == NULL)
	{
		return;
	}
	sprintf((char *)addr_dst,"%d.%d.%d.%d",	\
			(addr_src>>24)&0xffU,\
			(addr_src>>16)&0xffU,\
			(addr_src>>8)&0xffU,\
			addr_src&0xffU);
}
static int SocketRequest_Close(
	struct SOCKET * socket)
{
	int result=0;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(socket!=NULL,ERR_ARG);
	CHECK_SIGNATURE(socket,SOCKET_SIGNATURE);
	smsc_semaphore_wait(socket->mRequestLock);
	SMSC_ASSERT(socket->mRequestCode==REQUEST_UNDEFINED);
	socket->mRequestCode=REQUEST_CLOSE;

	socket->mResponseCode=RESPONSE_UNDEFINED;
	Sockets_PushRequest(socket);
	TaskManager_InterruptActivation(&gSocketTask);

#ifdef CH_SOCKET_TIMEOUT_SUPPORT
	smsc_semaphore_wait_timeout(socket->mResponseSignal,socket->close_timeout_ms);
#else	
	smsc_semaphore_wait(socket->mResponseSignal);
#endif

	SMSC_ASSERT(socket->mResponseCode!=RESPONSE_UNDEFINED);
	if(socket->mResponseCode==RESPONSE_SUCCESSFUL) {
		result=0;
	} else {
		SMSC_NOTICE(1,("Close socket failed[%d]",socket->mResponseCode));
		result=-1;
	}
	socket->mRequestCode=REQUEST_UNDEFINED;
	smsc_semaphore_signal(socket->mRequestLock);
	return result;
}	

static int SocketRequest_Receive(
	struct SOCKET * socket,
	void * data,
	int size,
	PIP_ADDRESS ipAddress,
	u16_t * port)
{
	int result=0;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(socket!=NULL,ERR_ARG);
	CHECK_SIGNATURE(socket,SOCKET_SIGNATURE);
	smsc_semaphore_wait(socket->mRequestLock);
	SMSC_ASSERT(socket->mRequestCode==REQUEST_UNDEFINED);

	socket->mRequestCode=REQUEST_RECEIVE;
	socket->mRequestParameters.mReceiveParams.mData=data;
	socket->mRequestParameters.mReceiveParams.mSize=size;

	socket->mResponseCode=RESPONSE_UNDEFINED;
	Sockets_PushRequest(socket);
	TaskManager_InterruptActivation(&gSocketTask);
#ifdef CH_SOCKET_TIMEOUT_SUPPORT
	smsc_semaphore_wait_timeout(socket->mResponseSignal,socket->receive_timeout_ms);
#else	
	smsc_semaphore_wait(socket->mResponseSignal);
#endif

	SMSC_ASSERT(socket->mResponseCode!=RESPONSE_UNDEFINED);
	if(socket->mResponseCode==RESPONSE_SUCCESSFUL) {
		result=socket->mRequestParameters.mReceiveParams.mSize;
		if(ipAddress!=NULL) {
			IP_ADDRESS_COPY(ipAddress,&(socket->mRequestParameters.mReceiveParams.mIpAddress));
		}
		if(port!=NULL) {
			(*port)=socket->mRequestParameters.mReceiveParams.mPort;
		}
	} else {
		if(socket->mResponseCode!=RESPONSE_CONNECTION_RESET) {
			/* the caller of this function expects that a result of -1 means connection reset */
			SMSC_WARNING(SOCKETS_DEBUG,("Unexpected response code = %d",socket->mResponseCode));
		}
		result=-1;
	}
	socket->mRequestCode=REQUEST_UNDEFINED;
	smsc_semaphore_signal(socket->mRequestLock);
	return result;
}

static int SocketRequest_Send(struct SOCKET * socket, void *data, int size, unsigned int flags)
{
	int result=0;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(socket!=NULL,ERR_ARG);
	CHECK_SIGNATURE(socket,SOCKET_SIGNATURE);
	smsc_semaphore_wait(socket->mRequestLock);
	SMSC_ASSERT(socket->mRequestCode==REQUEST_UNDEFINED);
	#if TCP_ENABLED
	if(socket->mConnectionWasReset) {
		result=ECONNRESET;
		goto UNLOCK;
	}
	#endif

	socket->mRequestCode=REQUEST_SEND;
	socket->mRequestParameters.mSendParams.mData=data;
	socket->mRequestParameters.mSendParams.mSize=size;
	socket->mRequestParameters.mSendParams.mFlags=flags;

	socket->mResponseCode=RESPONSE_UNDEFINED;
	Sockets_PushRequest(socket);
	TaskManager_InterruptActivation(&gSocketTask);
#ifdef CH_SOCKET_TIMEOUT_SUPPORT
	smsc_semaphore_wait_timeout(socket->mResponseSignal,socket->send_timeout_ms);
#else	
	smsc_semaphore_wait(socket->mResponseSignal);
#endif
	SMSC_ASSERT(socket->mResponseCode!=RESPONSE_UNDEFINED);
	if(socket->mResponseCode==RESPONSE_SUCCESSFUL) {
		result=socket->mRequestParameters.mSendParams.mSize;
	} else {
		if(socket->mResponseCode!=RESPONSE_CONNECTION_RESET) {
			SMSC_WARNING(SOCKETS_DEBUG,("SocketRequest_Send: unexpected response code = %d",socket->mResponseCode));
		}
		result=ECONNRESET;
	}
#if TCP_ENABLED
UNLOCK:
#endif
	socket->mRequestCode=REQUEST_UNDEFINED;
	smsc_semaphore_signal(socket->mRequestLock);
	return result;
}

static err_t SocketRequest_GetPeer(struct SOCKET * socket,PIP_ADDRESS ipAddress, u16_t * port)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(socket!=NULL,ERR_ARG);
	CHECK_SIGNATURE(socket,SOCKET_SIGNATURE);
	switch(socket->mType)
	{
#if RAW_ENABLED
	case SOCKET_TYPE_RAW:
		return ERR_CONN;
#endif
#if UDP_ENABLED
	case SOCKET_TYPE_UDP:
		if((socket->mControlBlock.mUdp==NULL) ||
			(((socket->mControlBlock.mUdp->mFlags)&UDP_FLAGS_CONNECTED)==0))
		{
			return ERR_CONN;
		}
		IP_ADDRESS_COPY(ipAddress,&(socket->mControlBlock.mUdp->mRemoteAddress));
		(*port)=socket->mControlBlock.mUdp->mRemotePort;
		break;
#endif
#if TCP_ENABLED
	case SOCKET_TYPE_TCP:
		if(socket->mControlBlock.mTcp==NULL) {
			return ERR_CONN;
		}
		IP_ADDRESS_COPY(ipAddress,&(socket->mControlBlock.mTcp->mRemoteAddress));
		(*port)=socket->mControlBlock.mTcp->mRemotePort;
		break;
#endif
	default:
		return ERR_CONN;	
	}
	return ERR_OK;
}

static int SocketRequest_Bind(struct SOCKET * socket, PIP_ADDRESS ipAddress, u16_t port)
{
	int result=0;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(socket!=NULL,ERR_ARG);
	CHECK_SIGNATURE(socket,SOCKET_SIGNATURE);
	smsc_semaphore_wait(socket->mRequestLock);
	SMSC_ASSERT(socket->mRequestCode==REQUEST_UNDEFINED);

	socket->mRequestCode=REQUEST_BIND;
	IP_ADDRESS_COPY(&(socket->mRequestParameters.mBindParams.mIpAddress),ipAddress);
	socket->mRequestParameters.mBindParams.mPort=port;

	socket->mResponseCode=RESPONSE_UNDEFINED;
	Sockets_PushRequest(socket);
	TaskManager_InterruptActivation(&gSocketTask);
#ifdef CH_SOCKET_TIMEOUT_SUPPORT
	smsc_semaphore_wait_timeout(socket->mResponseSignal,socket->bind_timeout_ms);
#else		
	smsc_semaphore_wait(socket->mResponseSignal);
#endif
	SMSC_ASSERT(socket->mResponseCode!=RESPONSE_UNDEFINED);
	if(socket->mResponseCode!=RESPONSE_SUCCESSFUL) {
		result=-1;
	}
	socket->mRequestCode=REQUEST_UNDEFINED;
	smsc_semaphore_signal(socket->mRequestLock);
	return result;
}

static err_t SocketRequest_Connect(struct SOCKET * socket, PIP_ADDRESS ipAddress, u16_t port)
{
	err_t result=ERR_OK;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(socket!=NULL,ERR_ARG);
	CHECK_SIGNATURE(socket,SOCKET_SIGNATURE);
	smsc_semaphore_wait(socket->mRequestLock);
	SMSC_ASSERT(socket->mRequestCode==REQUEST_UNDEFINED);

	socket->mRequestCode=REQUEST_CONNECT;
	IP_ADDRESS_COPY(&(socket->mRequestParameters.mConnectParams.mIpAddress),ipAddress);
	socket->mRequestParameters.mConnectParams.mPort=port;

	socket->mResponseCode=RESPONSE_UNDEFINED;
	Sockets_PushRequest(socket);
	TaskManager_InterruptActivation(&gSocketTask);
#ifdef CH_SOCKET_TIMEOUT_SUPPORT
	smsc_semaphore_wait_timeout(socket->mResponseSignal,socket->connect_timeout_ms);
#else			
	smsc_semaphore_wait(socket->mResponseSignal);
#endif
	SMSC_ASSERT(socket->mResponseCode!=RESPONSE_UNDEFINED);
	if(socket->mResponseCode!=RESPONSE_SUCCESSFUL) {
		result=ERR_VAL;
	}
	socket->mRequestCode=REQUEST_UNDEFINED;
	smsc_semaphore_signal(socket->mRequestLock);
	return result;
}

static err_t SocketRequest_Disconnect(struct SOCKET * socket)
{
	err_t result=ERR_OK;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(socket!=NULL,ERR_ARG);
	CHECK_SIGNATURE(socket,SOCKET_SIGNATURE);
	smsc_semaphore_wait(socket->mRequestLock);
	SMSC_ASSERT(socket->mRequestCode==REQUEST_UNDEFINED);

	socket->mRequestCode=REQUEST_DISCONNECT;

	socket->mResponseCode=RESPONSE_UNDEFINED;
	Sockets_PushRequest(socket);
	TaskManager_InterruptActivation(&gSocketTask);
#ifdef CH_SOCKET_TIMEOUT_SUPPORT
	smsc_semaphore_wait_timeout(socket->mResponseSignal,socket->disconnect_timeout_ms);
#else			
	smsc_semaphore_wait(socket->mResponseSignal);
#endif
	SMSC_ASSERT(socket->mResponseCode!=RESPONSE_UNDEFINED);
	if(socket->mResponseCode!=RESPONSE_SUCCESSFUL) {
		result=ERR_VAL;
	}
	socket->mRequestCode=REQUEST_UNDEFINED;
	smsc_semaphore_signal(socket->mRequestLock);
	return result;
}

#if UDP_ENABLED
static int SocketRequest_CreateUdpControlBlock(
	struct SOCKET * socket)
{
	int result=0;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(socket!=NULL,ERR_ARG);
	CHECK_SIGNATURE(socket,SOCKET_SIGNATURE);
	smsc_semaphore_wait(socket->mRequestLock);
	SMSC_ASSERT(socket->mRequestCode==REQUEST_UNDEFINED);

	socket->mRequestCode=REQUEST_CREATE_UDP_CONTROL_BLOCK;
	socket->mRequestParameters.mCreateUdpParams.mUseUdpLite=0;

	socket->mResponseCode=RESPONSE_UNDEFINED;
	Sockets_PushRequest(socket);
	TaskManager_InterruptActivation(&gSocketTask);
	#ifdef CH_SOCKET_TIMEOUT_SUPPORT
	smsc_semaphore_wait_timeout(socket->mResponseSignal,-1);
#else
	smsc_semaphore_wait(socket->mResponseSignal);
#endif
	SMSC_ASSERT(socket->mResponseCode!=RESPONSE_UNDEFINED);
	if(socket->mResponseCode!=RESPONSE_SUCCESSFUL) {
		result=-1;
	} else {
		SMSC_ASSERT(socket->mControlBlock.mUdp!=NULL);
	}
	socket->mRequestCode=REQUEST_UNDEFINED;
	smsc_semaphore_signal(socket->mRequestLock);
	return result;
}
#endif /* UDP_ENABLED */

#if TCP_ENABLED
static int SocketRequest_CreateTcpControlBlock(
	struct SOCKET * socket)
{
	int result=0;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(socket!=NULL,ERR_ARG);
	CHECK_SIGNATURE(socket,SOCKET_SIGNATURE);
	smsc_semaphore_wait(socket->mRequestLock);
	SMSC_ASSERT(socket->mRequestCode==REQUEST_UNDEFINED);
	
	socket->mRequestCode=REQUEST_CREATE_TCP_CONTROL_BLOCK;
	
	socket->mResponseCode=RESPONSE_UNDEFINED;
	Sockets_PushRequest(socket);
	TaskManager_InterruptActivation(&gSocketTask);
	#ifdef CH_SOCKET_TIMEOUT_SUPPORT
	smsc_semaphore_wait_timeout(socket->mResponseSignal,-1);
       #else	
	smsc_semaphore_wait(socket->mResponseSignal);
	#endif
	SMSC_ASSERT(socket->mResponseCode!=RESPONSE_UNDEFINED);
	if(socket->mResponseCode!=RESPONSE_SUCCESSFUL) {
		result=-1;
	} else {
		SMSC_ASSERT(socket->mControlBlock.mTcp!=NULL);
	}
	socket->mRequestCode=REQUEST_UNDEFINED;
	smsc_semaphore_signal(socket->mRequestLock);
	return result;
}
#endif /* TCP_ENABLED */

#if TCP_ENABLED
static err_t SocketRequest_Listen(struct SOCKET * socket)
{
	err_t result=ERR_OK;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(socket!=NULL,ERR_ARG);
	CHECK_SIGNATURE(socket,SOCKET_SIGNATURE);
	smsc_semaphore_wait(socket->mRequestLock);
	SMSC_ASSERT(socket->mRequestCode==REQUEST_UNDEFINED);
	
	socket->mRequestCode=REQUEST_LISTEN;
	
	socket->mResponseCode=RESPONSE_UNDEFINED;
	Sockets_PushRequest(socket);
	
	TaskManager_InterruptActivation(&gSocketTask);
#ifdef CH_SOCKET_TIMEOUT_SUPPORT
	smsc_semaphore_wait_timeout(socket->mResponseSignal,socket->disconnect_timeout_ms);
#else	
	smsc_semaphore_wait(socket->mResponseSignal);
#endif	
	SMSC_ASSERT(socket->mResponseCode!=RESPONSE_UNDEFINED);
	if(socket->mResponseCode!=RESPONSE_SUCCESSFUL) {
		result=ERR_MEM;
		/* NOTE this may not be the most appropriate error code 
			but the important thing is that it is not ERR_OK */
	}
	socket->mRequestCode=REQUEST_UNDEFINED;
	smsc_semaphore_signal(socket->mRequestLock);
	return result;	
}

static int SocketRequest_AcceptTcpControlBlock(
	struct SOCKET * listenerSocket,
	struct SOCKET * acceptedSocket,
	PIP_ADDRESS fromIpAddress,
	u16_t * fromPort)
{
	int result=0;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(listenerSocket!=NULL,ERR_ARG);
	CHECK_SIGNATURE(listenerSocket,SOCKET_SIGNATURE);
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(acceptedSocket!=NULL,ERR_ARG);
	CHECK_SIGNATURE(acceptedSocket,SOCKET_SIGNATURE);

	smsc_semaphore_wait(listenerSocket->mRequestLock);
	SMSC_ASSERT(listenerSocket->mRequestCode==REQUEST_UNDEFINED);

	listenerSocket->mRequestCode=REQUEST_ACCEPT;
	listenerSocket->mRequestParameters.mAcceptParams.mNewSocket=acceptedSocket;

	listenerSocket->mResponseCode=RESPONSE_UNDEFINED;
	Sockets_PushRequest(listenerSocket);
	TaskManager_InterruptActivation(&gSocketTask);
#ifdef CH_SOCKET_TIMEOUT_SUPPORT
	smsc_semaphore_wait_timeout(listenerSocket->mResponseSignal,listenerSocket->accept_timeout_ms);
#else		
	smsc_semaphore_wait(listenerSocket->mResponseSignal);
#endif
	SMSC_ASSERT(listenerSocket->mResponseCode!=RESPONSE_UNDEFINED);
	if(listenerSocket->mResponseCode!=RESPONSE_SUCCESSFUL) {
		result=ERR_MEM;
		/* NOTE this may not be the most appropriate error code 
			but the important thing is that it is not ERR_OK */
	} else {
		SMSC_ASSERT(acceptedSocket->mType==SOCKET_TYPE_TCP);
		SMSC_ASSERT(acceptedSocket->mControlBlock.mTcp!=NULL);
		if(fromIpAddress!=NULL) {
			IP_ADDRESS_COPY(fromIpAddress,&(listenerSocket->mRequestParameters.mAcceptParams.mFromIpAddress));
		}
		if(fromPort!=NULL) {
			(*fromPort)=listenerSocket->mRequestParameters.mAcceptParams.mFromPort;
		}
	}
	listenerSocket->mRequestCode=REQUEST_UNDEFINED;
	smsc_semaphore_signal(listenerSocket->mRequestLock);
	
	return result;
}
#endif /* TCP_ENABLED */

static void SocketsInternal_FreeControlBlock(struct SOCKET * socket)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(socket!=NULL);
	CHECK_SIGNATURE(socket,SOCKET_SIGNATURE);
	switch(socket->mType) {
	case SOCKET_TYPE_UNDEFINED:
		SMSC_ASSERT(socket->mControlBlock.mPointer==NULL);
		break;
#if RAW_ENABLED
	case SOCKET_TYPE_RAW:
		SMSC_ERROR(("SocketsInternal_FreeControlBlock: Raw sockets are not implemented"));
		break;
#endif
#if UDP_ENABLED
	case SOCKET_TYPE_UDP:
		SMSC_ASSERT(socket->mControlBlock.mUdp!=NULL);
		Udp_FreeControlBlock(socket->mControlBlock.mUdp);
		socket->mControlBlock.mUdp=NULL;		
		break;
#endif
#if TCP_ENABLED
	case SOCKET_TYPE_TCP:
		SMSC_ASSERT(socket->mControlBlock.mTcp!=NULL);
		Tcp_FreeControlBlock(socket->mControlBlock.mTcp);
		socket->mControlBlock.mTcp=NULL;
		break;
#endif
	default:
		SMSC_ERROR(("SocketsInternal_FreeControlBlock: unknown socket type = %"U16_F,((u16_t)(socket->mType)) ));
		break;
	}
	socket->mType=SOCKET_TYPE_UNDEFINED;
}
#if UDP_ENABLED
static void SocketsInternal_PrependSocketData(PPACKET_BUFFER packetBuffer,PIP_ADDRESS ipAddress,u16_t port)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(packetBuffer!=NULL);
	CHECK_SIGNATURE(packetBuffer,PACKET_BUFFER_SIGNATURE);
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(ipAddress!=NULL);
	/* Use header space to store port number */
	if(PacketBuffer_MoveStartPoint(packetBuffer,-MEMORY_ALIGNED_SIZE(sizeof(u16_t)))!=ERR_OK) {
		SMSC_ERROR(("SocketsInternal_PrependSocketData: Failed to reserve space for port number"));
	}
	(*((u16_t *)(PacketBuffer_GetStartPoint(packetBuffer))))=port;
	
	/* Use header space to store ip address */
	if(PacketBuffer_MoveStartPoint(packetBuffer,-MEMORY_ALIGNED_SIZE(sizeof(IP_ADDRESS)))!=ERR_OK) {
		SMSC_ERROR(("SocketsInternal_PrependSocketData: Failed to reserve space for ip address"));
	}
	IP_ADDRESS_COPY(((PIP_ADDRESS)PacketBuffer_GetStartPoint(packetBuffer)),ipAddress);
}

static void SocketsInternal_ExtractSocketData(PPACKET_BUFFER packetBuffer,PIP_ADDRESS ipAddress,u16_t *port)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(packetBuffer!=NULL);
	CHECK_SIGNATURE(packetBuffer,PACKET_BUFFER_SIGNATURE);
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(ipAddress!=NULL);
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(port!=NULL);
	
	IP_ADDRESS_COPY(ipAddress,((PIP_ADDRESS)PacketBuffer_GetStartPoint(packetBuffer)));
	if(PacketBuffer_MoveStartPoint(packetBuffer,MEMORY_ALIGNED_SIZE(sizeof(IP_ADDRESS)))!=ERR_OK) {
		SMSC_ERROR(("SocketsInternal_ExtractSocketData: Failed to extract ip address"));
	}
	
	(*port)=(*((u16_t *)(PacketBuffer_GetStartPoint(packetBuffer))));
	if(PacketBuffer_MoveStartPoint(packetBuffer,MEMORY_ALIGNED_SIZE(sizeof(u16_t)))!=ERR_OK) {
		SMSC_ERROR(("SocketsInternal_ExtractSocketData: Failed to extract port number"));
	}
}
#endif /* UDP_ENABLED */

static void SocketsInternal_DoReceiveCopy(struct SOCKET * socket)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(socket!=NULL);
	CHECK_SIGNATURE(socket,SOCKET_SIGNATURE);
	SMSC_ASSERT(socket->mRequestCode==REQUEST_RECEIVE);

#if UDP_ENABLED||TCP_ENABLED
	if(
#if UDP_ENABLED
		(socket->mType==SOCKET_TYPE_UDP)
#endif
#if UDP_ENABLED&&TCP_ENABLED
		||
#endif
#if TCP_ENABLED
		(socket->mType==SOCKET_TYPE_TCP)
#endif
	) {
		PPACKET_BUFFER packetBuffer;
		s32_t size=(u32_t)(socket->mRequestParameters.mReceiveParams.mSize);
		u8_t * data=((u8_t *)(socket->mRequestParameters.mReceiveParams.mData));
		#if UDP_ENABLED
		IP_ADDRESS originalSourceAddress;
		u16_t originalSourcePort;
		IP_ADDRESS nextAddress;
		u16_t nextPort;
		int isUdp=0;
		if(socket->mType==SOCKET_TYPE_UDP) {
			isUdp=1;
		}
		#endif
		SMSC_ASSERT(data!=NULL);

		packetBuffer=PacketQueue_Pop(&(socket->mReceiveQueue));
		#if UDP_ENABLED
		if((packetBuffer!=NULL)&&(isUdp)) {
			SocketsInternal_ExtractSocketData(packetBuffer,&originalSourceAddress,&originalSourcePort);
		}
		#endif

		/* Copy all packets whose total length will fit in the receive buffer */
		while( (packetBuffer!=NULL) &&
			((packetBuffer->mTotalLength)<=size) )
		{
			do {
				PPACKET_BUFFER nextBuffer=packetBuffer->mNext;
				SMSC_ASSERT(size>=packetBuffer->mThisLength);
				memcpy(data,PacketBuffer_GetStartPoint(packetBuffer),packetBuffer->mThisLength);
				data+=packetBuffer->mThisLength;
				size-=packetBuffer->mThisLength;
				packetBuffer->mTotalLength=packetBuffer->mThisLength;
				packetBuffer->mNext=NULL;
				PacketBuffer_DecreaseReference(packetBuffer);
				packetBuffer=nextBuffer;
			} while(packetBuffer!=NULL);
			
			packetBuffer=PacketQueue_Pop(&(socket->mReceiveQueue));
			#if UDP_ENABLED
			if((packetBuffer!=NULL)&&(isUdp)) {
				SocketsInternal_ExtractSocketData(packetBuffer,&nextAddress,&nextPort);
				if((nextPort!=originalSourcePort)||(!IP_ADDRESS_COMPARE(&nextAddress,&originalSourceAddress)))
				{
					SocketsInternal_PrependSocketData(packetBuffer,&nextAddress,nextPort);
					PacketQueue_UnPop(&(socket->mReceiveQueue),packetBuffer);
					packetBuffer=NULL;
				}
			}
			#endif
		}
		
		/* Copy all packet buffers whose full buffer length will fit in receive buffer */
		while( (packetBuffer!=NULL) &&
			((packetBuffer->mThisLength)<=size) )
		{
			PPACKET_BUFFER nextBuffer=packetBuffer->mNext;
			memcpy(data,PacketBuffer_GetStartPoint(packetBuffer),packetBuffer->mThisLength);
			data+=packetBuffer->mThisLength;
			size-=packetBuffer->mThisLength;
			packetBuffer->mTotalLength=packetBuffer->mThisLength;
			packetBuffer->mNext=NULL;
			PacketBuffer_DecreaseReference(packetBuffer);
			packetBuffer=nextBuffer;
		}
		
		/* Copy part of the last buffer into the remaining size of receive buffer */
		if(packetBuffer!=NULL) {
			if(size>0) {
				SMSC_ASSERT(packetBuffer->mThisLength>size);
				memcpy(data,PacketBuffer_GetStartPoint(packetBuffer),(s16_t)size);
				PacketBuffer_MoveStartPoint(packetBuffer,(s16_t)size);
				data+=size;
				size=0;
			}
			#if UDP_ENABLED
			if(isUdp) {
				SocketsInternal_PrependSocketData(packetBuffer,&originalSourceAddress,originalSourcePort);
			}
			#endif
			PacketQueue_UnPop(&(socket->mReceiveQueue),packetBuffer);
		}
		socket->mRequestParameters.mReceiveParams.mSize-=(int)size;
		#if UDP_ENABLED
		if(isUdp) {
			IP_ADDRESS_COPY(&(socket->mRequestParameters.mReceiveParams.mIpAddress),&originalSourceAddress);
			socket->mRequestParameters.mReceiveParams.mPort=originalSourcePort;
		} else 
		#endif
		{
			#if TCP_ENABLED
			SMSC_ASSERT(socket->mControlBlock.mTcp!=NULL);
			Tcp_AcknowledgeReceivedData(
				socket->mControlBlock.mTcp,
				socket->mRequestParameters.mReceiveParams.mSize);
			IP_ADDRESS_COPY(&(socket->mRequestParameters.mReceiveParams.mIpAddress),
				&(socket->mControlBlock.mTcp->mRemoteAddress));
			socket->mRequestParameters.mReceiveParams.mPort=
				socket->mControlBlock.mTcp->mRemotePort;
			#endif /* TCP_ENABLED */
		}
	} else 
#endif /* UDP_ENABLED||TCP_ENABLED */
	{
		SMSC_ERROR(("SocketsInternal_DoReceiveCopy: Unexpected socket type"));
	}
}

#if UDP_ENABLED
static void SocketsInternal_UdpReceiveFunction(void * param,
	struct UDP_CONTROL_BLOCK * udpControlBlock,
	PPACKET_BUFFER packetBuffer,
	PIP_ADDRESS sourceAddress,
	u16_t sourcePort)
{
	struct SOCKET * socket=(struct SOCKET *)param;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(socket!=NULL);
	CHECK_SIGNATURE(socket,SOCKET_SIGNATURE);
	
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(packetBuffer!=NULL);
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(sourceAddress!=NULL);
	
	SocketsInternal_PrependSocketData(packetBuffer,sourceAddress,sourcePort);
	
	PacketQueue_Push(&(socket->mReceiveQueue),packetBuffer);
	
	if((socket->mResponseCode==RESPONSE_PENDING)&&
		(socket->mRequestCode==REQUEST_RECEIVE))
	{
		SocketsInternal_DoReceiveCopy(socket);
		socket->mResponseCode=RESPONSE_SUCCESSFUL;

		smsc_semaphore_signal(socket->mResponseSignal);
	}
}
#endif /* UDP_ENABLED */

#if UDP_ENABLED
static void SocketsInternal_DoCreateUdpControlBlock(struct SOCKET * socket)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(socket!=NULL);
	CHECK_SIGNATURE(socket,SOCKET_SIGNATURE);
	if(socket->mType!=SOCKET_TYPE_UNDEFINED) {
		SocketsInternal_FreeControlBlock(socket);
		SMSC_ASSERT(socket->mType==SOCKET_TYPE_UNDEFINED);
	}
	SMSC_ASSERT(socket->mControlBlock.mUdp==NULL);
	socket->mControlBlock.mUdp=Udp_CreateControlBlock();
	if(socket->mControlBlock.mUdp!=NULL) {
		socket->mType=SOCKET_TYPE_UDP;
		if(socket->mRequestParameters.mCreateUdpParams.mUseUdpLite) {
			socket->mControlBlock.mUdp->mFlags|=UDP_FLAGS_UDPLITE;
		}
		Udp_SetReceiverCallBack(
			socket->mControlBlock.mUdp,
			SocketsInternal_UdpReceiveFunction,(void *)socket);
		socket->mResponseCode=RESPONSE_SUCCESSFUL;
	} else {
		socket->mResponseCode=RESPONSE_MEMORY_ERROR;
	}
}
#endif /* UDP_ENABLED */

static void SocketsInternal_SignalWaitingThreads(struct SOCKET * socket)
{
	struct smsc_select_cb * scb;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(socket!=NULL);
	CHECK_SIGNATURE(socket,SOCKET_SIGNATURE);
	while (1)
	{
		smsc_semaphore_wait(gSelectLock);
		for (scb = gSelectList; scb; scb = scb->next)
		{
			if (scb->sem_signalled == 0)
			{
				/* Test this select call for our socket */
				if (scb->readset && FD_ISSET(socket->mSocketIndex, scb->readset))
					if ((!PacketQueue_IsEmpty(&(socket->mReceiveQueue)))||
						(socket->mReceiveFinishedFlag)
						#if TCP_ENABLED
						||(socket->mAcceptListSize>0)
						||(socket->mConnectionWasReset)
						#endif
						)
						break;
				if (scb->writeset && FD_ISSET(socket->mSocketIndex, scb->writeset))
					if ((socket->mIsWritable)
						#if TCP_ENABLED
						||(socket->mConnectionWasReset)
						#endif
						)
						break;
			}
		}
		if (scb)
		{
			scb->sem_signalled = 1;
			smsc_semaphore_signal(gSelectLock);
			smsc_semaphore_signal(scb->sem);
		} else {
			smsc_semaphore_signal(gSelectLock);
			break;
		}
	}
}

#if TCP_ENABLED
/* Function to be called when more send buffer space is available. */
static err_t SocketsInternal_TcpSentCallBack(void *arg, struct TCP_CONTROL_BLOCK * tcpControlBlock, u16_t space)
{
	struct SOCKET * socket=(struct SOCKET *)arg;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(socket!=NULL,ERR_ARG);
	CHECK_SIGNATURE(socket,SOCKET_SIGNATURE);
	
	SMSC_ASSERT(socket->mType==SOCKET_TYPE_TCP);
	if(socket->mControlBlock.mTcp->mAvailableSendSpace>TCP_SENDABLE_THRESHOLD) {
		socket->mIsWritable=1;
		SocketsInternal_SignalWaitingThreads(socket);
	}
	return ERR_OK;/*wufei add this line 2009-2-5*/
}

/* Function to be called when (in-sequence) data has arrived. */
static err_t SocketsInternal_TcpReceiveCallBack(void *arg, struct TCP_CONTROL_BLOCK * tcpControlBlock, PPACKET_BUFFER packetBuffer, err_t err)
{
	struct SOCKET * socket=(struct SOCKET *)arg;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(socket!=NULL,ERR_ARG);
	CHECK_SIGNATURE(socket,SOCKET_SIGNATURE);
	SMSC_TRACE(SOCKETS_DEBUG,("SocketsInternal_TcpReceiveCallBack:"));
	if(packetBuffer!=NULL) {
		/* we should not receive more data after receiving end of data */
		SMSC_ASSERT(socket->mReceiveFinishedFlag==0);
#ifndef  MEMORY_POOL_CHANGE_090217
		PacketQueue_Push(&(socket->mReceiveQueue),packetBuffer);
#else
{
	PPACKET_BUFFER tempPacket = NULL;
	
	while(packetBuffer != NULL){
		tempPacket = packetBuffer;
		packetBuffer = packetBuffer->mNext;
		tempPacket->mNext = NULL;
		/*tempPacket->mTotalLength = tempPacket->mThisLength;*/
		PacketQueue_Push(&(socket->mReceiveQueue),tempPacket);
	}
}
#endif
	} else {
		socket->mReceiveFinishedFlag=1;
	}

	if((socket->mResponseCode==RESPONSE_PENDING)&&
		(socket->mRequestCode==REQUEST_RECEIVE))
	{
		SocketsInternal_DoReceiveCopy(socket);
		socket->mResponseCode=RESPONSE_SUCCESSFUL;
		
		smsc_semaphore_signal(socket->mResponseSignal);
	}
	
	if((!PacketQueue_IsEmpty(&(socket->mReceiveQueue)))||
		(socket->mReceiveFinishedFlag))
	{
		SocketsInternal_SignalWaitingThreads(socket);
	}
	return ERR_OK;/*wufei add this line 2009-2-5*/
}

/* Function to be called when a connection has been set up. */
static err_t SocketsInternal_TcpConnectedCallBack(void *arg, struct TCP_CONTROL_BLOCK * tcpControlBlock, err_t err)
{
	struct SOCKET * socket=(struct SOCKET *)arg;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(socket!=NULL,ERR_ARG);
	CHECK_SIGNATURE(socket,SOCKET_SIGNATURE);
	SMSC_ASSERT(socket->mType==SOCKET_TYPE_TCP);
	SMSC_ASSERT(socket->mRequestCode==REQUEST_CONNECT);
	SMSC_ASSERT(socket->mResponseCode==RESPONSE_PENDING);
	
	socket->mErrorCode=err;
	if(err==ERR_OK) {
		socket->mResponseCode=RESPONSE_SUCCESSFUL;
	} else {
		socket->mResponseCode=RESPONSE_UNSUCCESSFUL;
	}

	smsc_semaphore_signal(socket->mResponseSignal);
	return ERR_OK;
}

static void SocketsInternal_AcceptNewTcpControlBlock(
	struct SOCKET * socket);

/* Function to call when a listener has been connected. */
static err_t SocketsInternal_TcpAcceptCallBack(void *arg, struct TCP_CONTROL_BLOCK * newControlBlock, err_t err)
{
	struct SOCKET * socket=(struct SOCKET *)arg;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(socket!=NULL,ERR_ARG);
	CHECK_SIGNATURE(socket,SOCKET_SIGNATURE);
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(newControlBlock!=NULL,ERR_ARG);
	
	if(socket->mAcceptListSize<socket->mListenBackLog) {
		SMSC_ASSERT(socket->mAcceptListSize<SOCKET_MAXIMUM_BACKLOG);
		socket->mAcceptList[socket->mAcceptListSize]=newControlBlock;
		socket->mAcceptListSize++;
				
		if((socket->mResponseCode==RESPONSE_PENDING)&&
			(socket->mRequestCode==REQUEST_ACCEPT)) 
		{
			SocketsInternal_AcceptNewTcpControlBlock(socket);
		
			smsc_semaphore_signal(socket->mResponseSignal);
		}
		if(socket->mAcceptListSize>0) {
			SocketsInternal_SignalWaitingThreads(socket);
		}
	} else {
		SMSC_NOTICE(SOCKETS_DEBUG,("SocketsInternal_TcpAcceptCallBack: Unable to accept connection"));
		SMSC_NOTICE(SOCKETS_DEBUG,("    Too many connection on backlog"));
		return ERR_ABRT;
	}

	return ERR_OK;
}

/* Function to be called whenever a fatal error occurs. */
static void SocketsInternal_TcpErrorCallBack(void *arg, err_t err)
{
	struct SOCKET * socket=(struct SOCKET *)arg;

	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(socket!=NULL);
	CHECK_SIGNATURE(socket,SOCKET_SIGNATURE);
	SMSC_ASSERT(socket->mType==SOCKET_TYPE_TCP);

	if(err==ERR_RST) {
		SMSC_NOTICE(SOCKETS_DEBUG,("SocketsInternal_TcpErrorCallBack: RESET detected"));
		socket->mConnectionWasReset=1;
		if(socket->mResponseCode==RESPONSE_PENDING) {
			if(socket->mRequestCode==REQUEST_RECEIVE) {
				SocketsInternal_DoReceiveCopy(socket);
				socket->mResponseCode=RESPONSE_CONNECTION_RESET;
				smsc_semaphore_signal(socket->mResponseSignal);
			}
		}
		SocketsInternal_SignalWaitingThreads(socket);
	} else {
		SMSC_WARNING(SOCKETS_DEBUG,("SocketsInternal_TcpErrorCallBack: Unhandled error, err=%d",err));
	}
}

static void SocketsInternal_DoCreateTcpControlBlock(struct SOCKET * socket)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(socket!=NULL);
	CHECK_SIGNATURE(socket,SOCKET_SIGNATURE);
	if(socket->mType!=SOCKET_TYPE_UNDEFINED) {
		SocketsInternal_FreeControlBlock(socket);
		SMSC_ASSERT(socket->mType==SOCKET_TYPE_UNDEFINED);
	}
	SMSC_ASSERT(socket->mControlBlock.mTcp==NULL);
	socket->mControlBlock.mTcp=Tcp_NewControlBlock();
	if(socket->mControlBlock.mTcp!=NULL) {
		socket->mType=SOCKET_TYPE_TCP;
		Tcp_SetCallBackArgument(socket->mControlBlock.mTcp,socket);
		Tcp_SetErrorCallBack(socket->mControlBlock.mTcp,SocketsInternal_TcpErrorCallBack);
		Tcp_SetReceiveCallBack(socket->mControlBlock.mTcp,SocketsInternal_TcpReceiveCallBack);
		Tcp_SetSentCallBack(socket->mControlBlock.mTcp,SocketsInternal_TcpSentCallBack);
		socket->mResponseCode=RESPONSE_SUCCESSFUL;
	} else {
		socket->mResponseCode=RESPONSE_MEMORY_ERROR;
	}
}
#endif /* TCP_ENABLED */

static void SocketsInternal_DoBind(struct SOCKET * socket)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(socket!=NULL);
	CHECK_SIGNATURE(socket,SOCKET_SIGNATURE);
	if (socket->mControlBlock.mPointer == NULL) {
		/* Need to create a control block first
			in other words smsc_socket must complete successfully */
		SMSC_WARNING(SOCKETS_DEBUG,("SocketsInternal_DoBind: Illegal Operation"));
		socket->mResponseCode=RESPONSE_ILLEGAL_OPERATION;
		return;
	}
	switch (socket->mType) {
#if RAW_ENABLED
	case SOCKET_TYPE_RAW:
		/*msg->conn->err = raw_bind(msg->conn->pcb.raw,msg->msg.bc.ipaddr);*/
		SMSC_ERROR(("SocketsInternal_DoBind: Raw sockets not implemented"));
		break;
#endif
#if UDP_ENABLED
	case SOCKET_TYPE_UDP:
		if(Udp_Bind(
			socket->mControlBlock.mUdp,
			&(socket->mRequestParameters.mBindParams.mIpAddress), 
			socket->mRequestParameters.mBindParams.mPort)==ERR_OK)
		{
			socket->mResponseCode=RESPONSE_SUCCESSFUL;
		} else {
			socket->mResponseCode=RESPONSE_UNSUCCESSFUL;
		}
		break;
#endif
#if TCP_ENABLED
	case SOCKET_TYPE_TCP:
		if(Tcp_Bind(socket->mControlBlock.mTcp,
			&(socket->mRequestParameters.mBindParams.mIpAddress),
			socket->mRequestParameters.mBindParams.mPort)==ERR_OK) 
		{
			socket->mResponseCode=RESPONSE_SUCCESSFUL;
		} else {
			socket->mResponseCode=RESPONSE_UNSUCCESSFUL;
		}
		break;
#endif
	default:
		SMSC_WARNING(SOCKETS_DEBUG,("SocketsInternal_DoBind: Unknown Socket Type=%"U16_F,(u16_t)(socket->mType)));
		socket->mResponseCode=RESPONSE_ILLEGAL_OPERATION;
		break;
	}
}

static void SocketsInternal_DoConnect(struct SOCKET * socket)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(socket!=NULL);
	CHECK_SIGNATURE(socket,SOCKET_SIGNATURE);
	if (socket->mControlBlock.mPointer == NULL) {
		/* Need to create a control block first
			in other words smsc_socket must complete successfully */
		SMSC_WARNING(SOCKETS_DEBUG,("SocketsInternal_DoConnect: Illegal Operation"));
		socket->mResponseCode=RESPONSE_ILLEGAL_OPERATION;
		return;
	}
	switch (socket->mType) {
#if RAW_ENABLED
	case SOCKET_TYPE_RAW:
		/*raw_connect(msg->conn->pcb.raw, msg->msg.bc.ipaddr);*/
		SMSC_ERROR(("SocketsInternal_DoConnect: Raw sockets not implemented"));
		socket->mResponseCode=RESPONSE_ILLEGAL_OPERATION;
		break;
#endif
#if UDP_ENABLED
	case SOCKET_TYPE_UDP:
		if(Udp_Connect(socket->mControlBlock.mUdp,
			&(socket->mRequestParameters.mConnectParams.mIpAddress),
			socket->mRequestParameters.mConnectParams.mPort)==ERR_OK)
		{
			socket->mResponseCode=RESPONSE_SUCCESSFUL;
		} else {
			socket->mResponseCode=RESPONSE_UNSUCCESSFUL;
		}
		break;
#endif
#if TCP_ENABLED
	case SOCKET_TYPE_TCP:
		Tcp_Connect(socket->mControlBlock.mTcp,
			&(socket->mRequestParameters.mConnectParams.mIpAddress),
			socket->mRequestParameters.mConnectParams.mPort,
			SocketsInternal_TcpConnectedCallBack);
		socket->mReceiveFinishedFlag=0;
		socket->mResponseCode=RESPONSE_PENDING;
		break;
#endif
	default:
		SMSC_WARNING(SOCKETS_DEBUG,("SocketsInternal_DoConnect: Unknown Socket Type=%"U16_F,(u16_t)(socket->mType)));
		socket->mResponseCode=RESPONSE_ILLEGAL_OPERATION;
		break;
	}
}

static void SocketsInternal_DoDisconnect(struct SOCKET * socket)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(socket!=NULL);
	CHECK_SIGNATURE(socket,SOCKET_SIGNATURE);
	socket->mResponseCode=RESPONSE_SUCCESSFUL;
	if (socket->mControlBlock.mPointer == NULL) {
		/* Need to create a control block first
			in other words smsc_socket must complete successfully */
		SMSC_WARNING(SOCKETS_DEBUG,("SocketsInternal_DoDisconnect: Illegal Operation"));
		socket->mResponseCode=RESPONSE_ILLEGAL_OPERATION;
		return;
	}
	switch (socket->mType) {
#if RAW_ENABLED
	case SOCKET_TYPE_RAW:
		/*raw_connect(msg->conn->pcb.raw, msg->msg.bc.ipaddr);*/
		SMSC_NOTICE(SOCKETS_DEBUG,("SocketsInternal_DoDisconnect: Raw sockets need not disconnect"));
		break;
#endif
#if UDP_ENABLED
	case SOCKET_TYPE_UDP:
		Udp_Disconnect(socket->mControlBlock.mUdp);
		break;
#endif
#if TCP_ENABLED
	case SOCKET_TYPE_TCP:
		SMSC_NOTICE(SOCKETS_DEBUG,("SocketsInternal_DoDisconnect: TCP sockets need not disconnect, close instead"));
		break;
#endif
	default:
		SMSC_WARNING(SOCKETS_DEBUG,("SocketsInternal_DoDisconnect: Unknown Socket Type=%"U16_F,(u16_t)(socket->mType)));
		socket->mResponseCode=RESPONSE_ILLEGAL_OPERATION;
		break;
	}
}

static void SocketsInternal_DoSend(struct SOCKET * socket)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(socket!=NULL);
	CHECK_SIGNATURE(socket,SOCKET_SIGNATURE);
	if (socket->mControlBlock.mPointer == NULL) {
		/* Need to create a control block first
			in other words smsc_socket must complete successfully */
		SMSC_WARNING(SOCKETS_DEBUG,("SocketsInternal_DoSend: Illegal Operation"));
		socket->mResponseCode=RESPONSE_ILLEGAL_OPERATION;
		return;
	}
	switch(socket->mType) {
#if RAW_ENABLED
	case SOCKET_TYPE_RAW:
		SMSC_ERROR(("SocketsInternal_DoSend: Raw Sockets are not implemented"));
		break;
#endif
#if UDP_ENABLED
	case SOCKET_TYPE_UDP:
		if(socket->mRequestParameters.mSendParams.mSize>0) {
			PPACKET_BUFFER packetBuffer;
			u32_t size=(u32_t)(socket->mRequestParameters.mSendParams.mSize);
			SMSC_ASSERT(socket->mRequestParameters.mSendParams.mData!=NULL);
			packetBuffer=Udp_AllocatePacket(
				&(socket->mControlBlock.mUdp->mRemoteAddress),
				size);
			if(packetBuffer==NULL) {
				socket->mResponseCode=RESPONSE_MEMORY_ERROR;
				return;
			}
			if(packetBuffer->mTotalLength<size) {
				size=packetBuffer->mTotalLength;
			}
			PacketBuffer_CopyFromExternalBuffer(packetBuffer,0,
				socket->mRequestParameters.mSendParams.mData,
				size);
			Udp_Send(socket->mControlBlock.mUdp,packetBuffer);
			socket->mRequestParameters.mSendParams.mSize=(int)size;
		}
		socket->mResponseCode=RESPONSE_SUCCESSFUL;
		break;
#endif
#if TCP_ENABLED
	case SOCKET_TYPE_TCP:
		if(socket->mConnectionWasReset) {
			socket->mResponseCode=RESPONSE_CONNECTION_RESET;
		} else {
			if(socket->mRequestParameters.mSendParams.mSize>0) {
				PPACKET_BUFFER packetBuffer;
				u32_t size=(u32_t)(socket->mRequestParameters.mSendParams.mSize);
				SMSC_ASSERT(socket->mRequestParameters.mSendParams.mData!=NULL);
				packetBuffer=Tcp_AllocateDataBuffers(
					socket->mControlBlock.mTcp,
					size);
				if(packetBuffer==NULL) {
					socket->mResponseCode=RESPONSE_MEMORY_ERROR;
					return;
				}
				if(packetBuffer->mTotalLength<size) {
					size=packetBuffer->mTotalLength;
				}
				PacketBuffer_CopyFromExternalBuffer(packetBuffer,0,
					socket->mRequestParameters.mSendParams.mData,
					size);
				packetBuffer=Tcp_QueueData(socket->mControlBlock.mTcp,packetBuffer);
				if(packetBuffer!=NULL) {
					socket->mIsWritable=0;
					/* some data could not be queued */
					size-=packetBuffer->mTotalLength;
					PacketBuffer_DecreaseReference(packetBuffer);
					packetBuffer=NULL;
				}
				Tcp_SendQueuedData(socket->mControlBlock.mTcp);
				socket->mRequestParameters.mSendParams.mSize=(int)size;
			}
			socket->mResponseCode=RESPONSE_SUCCESSFUL;
		}
		break;
#endif
	default:
		SMSC_ERROR(("SocketsInternal_DoSend: Unknown Socket Type=%"U16_F,
			(u16_t)(socket->mType) ));
		break;
	}
}

static void SocketsInternal_DoReceive(struct SOCKET * socket)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(socket!=NULL);
	CHECK_SIGNATURE(socket,SOCKET_SIGNATURE);
	if (socket->mControlBlock.mPointer == NULL) {
		/* Need to create a control block first
			in other words smsc_socket must complete successfully */
		SMSC_WARNING(SOCKETS_DEBUG,("SocketsInternal_DoReceive: Illegal Operation"));
		socket->mResponseCode=RESPONSE_ILLEGAL_OPERATION;
		return;
	}
	switch(socket->mType) {
#if RAW_ENABLED
	case SOCKET_TYPE_RAW:
		SMSC_ERROR(("SocketsInternal_DoReceive: Raw Sockets are not implemented"));
		break;
#endif
#if UDP_ENABLED
	case SOCKET_TYPE_UDP:
		/* fall through */
#endif
#if TCP_ENABLED
	case SOCKET_TYPE_TCP:
#endif
#if UDP_ENABLED || TCP_ENABLED
		socket->mResponseCode=RESPONSE_SUCCESSFUL;
		if(socket->mRequestParameters.mReceiveParams.mSize>0) {
			if(PacketQueue_IsEmpty(&(socket->mReceiveQueue))&&(!(socket->mReceiveFinishedFlag)) )
			{
				socket->mResponseCode=RESPONSE_PENDING;
			} else {
				SocketsInternal_DoReceiveCopy(socket);
			}
		}
		break;
#endif
	default:
		SMSC_ERROR(("SocketsInternal_DoReceive: Unknown Socket Type=%"U16_F,
			(u16_t)(socket->mType) ));
		break;
	}
}

static void SocketsInternal_DoClose(struct SOCKET * socket)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(socket!=NULL);
	CHECK_SIGNATURE(socket,SOCKET_SIGNATURE);
	if (socket->mControlBlock.mPointer == NULL) {
		/* Need to create a control block first
			in other words smsc_socket must complete successfully */
		SMSC_WARNING(SOCKETS_DEBUG,("SocketsInternal_DoClose: Illegal Operation"));
		socket->mResponseCode=RESPONSE_ILLEGAL_OPERATION;
		return;
	}
	switch(socket->mType) {
#if RAW_ENABLED
	case SOCKET_TYPE_RAW:
		SMSC_ERROR(("SocketsInternal_DoClose: Raw Sockets are not implemented"));
		break;
#endif
#if UDP_ENABLED
	case SOCKET_TYPE_UDP:
		/* free any remaining packets */
		while(!PacketQueue_IsEmpty(&(socket->mReceiveQueue)))
		{
			PPACKET_BUFFER packetBuffer=PacketQueue_Pop(&(socket->mReceiveQueue));
			SMSC_ASSERT(packetBuffer!=NULL);
			PacketBuffer_DecreaseReference(packetBuffer);
		}
		Udp_SetReceiverCallBack(socket->mControlBlock.mUdp,NULL,NULL);
		Udp_UnBind(socket->mControlBlock.mUdp);
		Udp_FreeControlBlock(socket->mControlBlock.mUdp);
		socket->mControlBlock.mUdp=NULL;
		
		socket->mResponseCode=RESPONSE_SUCCESSFUL;
		break;
#endif
#if TCP_ENABLED
	case SOCKET_TYPE_TCP:
		/* free any remaining packets */
		while(!PacketQueue_IsEmpty(&(socket->mReceiveQueue)))
		{
			PPACKET_BUFFER packetBuffer=PacketQueue_Pop(&(socket->mReceiveQueue));
			SMSC_ASSERT(packetBuffer!=NULL);
			Tcp_AcknowledgeReceivedData(socket->mControlBlock.mTcp,packetBuffer->mTotalLength);
			PacketBuffer_DecreaseReference(packetBuffer);
		}
		if(Tcp_Close(socket->mControlBlock.mTcp)==ERR_OK) {
			socket->mResponseCode=RESPONSE_SUCCESSFUL;
		} else {
			socket->mResponseCode=RESPONSE_UNSUCCESSFUL;
		}
		socket->mControlBlock.mTcp=NULL;
		
		socket->mResponseCode=RESPONSE_SUCCESSFUL;
		break;
#endif
	default:
		SMSC_ERROR(("SocketsInternal_DoClose: Unknown Socket Type=%"U16_F,
			(u16_t)(socket->mType) ));
		break;
	}
}

#if TCP_ENABLED
static void SocketsInternal_DoListen(struct SOCKET * socket)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(socket!=NULL);
	CHECK_SIGNATURE(socket,SOCKET_SIGNATURE);
	
	if((socket->mType!=SOCKET_TYPE_TCP)||(socket->mControlBlock.mTcp==NULL)) {
		/* Need to create a tcp control block first
			in other words smsc_socket must complete successfully */
		SMSC_WARNING(SOCKETS_DEBUG,("SocketsInternal_DoListen: Illegal Operation"));
		socket->mResponseCode=RESPONSE_ILLEGAL_OPERATION;
		return;
	}
	
	Tcp_Listen(socket->mControlBlock.mTcp);
	Tcp_SetCallBackArgument(socket->mControlBlock.mTcp,socket);
	Tcp_SetAcceptCallBack(socket->mControlBlock.mTcp,SocketsInternal_TcpAcceptCallBack);
	socket->mResponseCode=RESPONSE_SUCCESSFUL;
}

static void SocketsInternal_AcceptNewTcpControlBlock(
	struct SOCKET * socket)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(socket!=NULL);
	CHECK_SIGNATURE(socket,SOCKET_SIGNATURE);
	SMSC_ASSERT(socket->mType==SOCKET_TYPE_TCP);
	SMSC_ASSERT(socket->mRequestCode==REQUEST_ACCEPT);
	SMSC_ASSERT(socket->mResponseCode==RESPONSE_PENDING);
	
	/* This is the socket with which we will fill in the new tcp control block */
	SMSC_ASSERT(socket->mRequestParameters.mAcceptParams.mNewSocket!=NULL);
	SMSC_ASSERT(socket->mRequestParameters.mAcceptParams.mNewSocket->mType==SOCKET_TYPE_UNDEFINED);
	SMSC_ASSERT(socket->mRequestParameters.mAcceptParams.mNewSocket->mControlBlock.mTcp==NULL);
	
	SMSC_ASSERT(socket->mAcceptListSize>0);
	SMSC_ASSERT(socket->mAcceptList[0]!=NULL);
	socket->mRequestParameters.mAcceptParams.mNewSocket->mType=SOCKET_TYPE_TCP;
	socket->mRequestParameters.mAcceptParams.mNewSocket->mControlBlock.mTcp=socket->mAcceptList[0];
	Tcp_SetCallBackArgument(socket->mRequestParameters.mAcceptParams.mNewSocket->mControlBlock.mTcp,
		socket->mRequestParameters.mAcceptParams.mNewSocket);
	
	Tcp_SetErrorCallBack(socket->mRequestParameters.mAcceptParams.mNewSocket->mControlBlock.mTcp,SocketsInternal_TcpErrorCallBack);
	Tcp_SetReceiveCallBack(socket->mRequestParameters.mAcceptParams.mNewSocket->mControlBlock.mTcp,SocketsInternal_TcpReceiveCallBack);
	Tcp_SetSentCallBack(socket->mRequestParameters.mAcceptParams.mNewSocket->mControlBlock.mTcp,SocketsInternal_TcpSentCallBack);
	
	IP_ADDRESS_COPY(&(socket->mRequestParameters.mAcceptParams.mFromIpAddress),
		&(socket->mAcceptList[0]->mRemoteAddress));
	socket->mRequestParameters.mAcceptParams.mFromPort=
		(socket->mAcceptList[0]->mRemotePort);
	{
		int index=0;
		while((index+1)<socket->mAcceptListSize) {
			socket->mAcceptList[index]=socket->mAcceptList[index+1];
			index++;
		}
		socket->mAcceptList[index]=NULL;
		socket->mAcceptListSize--;
	}
	socket->mResponseCode=RESPONSE_SUCCESSFUL;
}

static void SocketsInternal_DoAccept(struct SOCKET * socket)
{
	SMSC_ASSERT(socket!=NULL);
	CHECK_SIGNATURE(socket,SOCKET_SIGNATURE);
	if ((socket->mControlBlock.mTcp == NULL)||(socket->mType!=SOCKET_TYPE_TCP)) {
		/* Need to create a tcp control block first
			in other words smsc_socket must complete successfully */
		SMSC_WARNING(SOCKETS_DEBUG,("SocketsInternal_DoAccept: Illegal Operation"));
		socket->mResponseCode=RESPONSE_ILLEGAL_OPERATION;
		return;
	}
	
	socket->mResponseCode=RESPONSE_PENDING;
	if(socket->mAcceptListSize>0) {
		SocketsInternal_AcceptNewTcpControlBlock(socket);
	}
}
#endif /* TCP_ENABLED */

static void SocketsInternal_RequestHandler(void * param)
{
	struct SOCKET * socket=NULL;
	u8_t count=0;

	/* Must schedule call back before doing any work. 
		Because the act of scheduling may clear an
		activation signal. This way even if it does
		clear an activation signal we will still handle the work
		following the schedule.*/
	TaskManager_ScheduleByInterruptActivation(&gSocketTask);
	
AGAIN:
	if(param!=NULL) {
		socket=(struct SOCKET *)param;
		param=NULL;
	} else {
		socket=Sockets_PopRequest();
	}

	if(count>=SOCKET_REQUEST_BURST_COUNT) {
		/* no more requests are permitted at this time */
		goto DONE;
	}

	if(socket!=NULL)
	{
		CHECK_SIGNATURE(socket,SOCKET_SIGNATURE);
		SMSC_ASSERT(socket->mResponseCode==RESPONSE_UNDEFINED);
		switch(socket->mRequestCode) {
		#if UDP_ENABLED
		case REQUEST_CREATE_UDP_CONTROL_BLOCK:
			SocketsInternal_DoCreateUdpControlBlock(socket);
			break;
		#endif
		#if TCP_ENABLED
		case REQUEST_CREATE_TCP_CONTROL_BLOCK:
			SocketsInternal_DoCreateTcpControlBlock(socket);
			break;
		#endif
		case REQUEST_BIND:
			SocketsInternal_DoBind(socket);
			break;
		case REQUEST_CONNECT:
			SocketsInternal_DoConnect(socket);
			break;
		case REQUEST_DISCONNECT:
			SocketsInternal_DoDisconnect(socket);
			break;
		case REQUEST_SEND:
			SocketsInternal_DoSend(socket);
			break;
		case REQUEST_RECEIVE:
			SocketsInternal_DoReceive(socket);
			if(socket->mResponseCode==RESPONSE_PENDING) {
				SMSC_NOTICE(SOCKETS_DEBUG,("SocketsInternal_RequestHandler: Waiting for receive data"));
			}
			break;
		case REQUEST_CLOSE:
			SocketsInternal_DoClose(socket);
			break;
		#if TCP_ENABLED
		case REQUEST_LISTEN:
			SocketsInternal_DoListen(socket);
			break;
		case REQUEST_ACCEPT:
			SocketsInternal_DoAccept(socket);
			break;
		#endif
		default:
			SMSC_WARNING(SOCKETS_DEBUG,("SocketsInternal_RequestHandler: Unsupported Request Code"));
			socket->mResponseCode=RESPONSE_UNKNOWN_REQUEST;
			goto DONE;
			
		}
		if(socket->mResponseCode!=RESPONSE_PENDING) {
			/* Request has completed */
			smsc_semaphore_signal(socket->mResponseSignal);
		}
		socket=NULL;
		count++;
		goto AGAIN;
	}
			
DONE:
	if(socket!=NULL) {
		Task_SetParameter(&gSocketTask,socket);
		TaskManager_InterruptActivation(&gSocketTask);
	} else {
		Task_SetParameter(&gSocketTask,NULL);
	}
}
s32_t CH_GetTCPStatus(s32_t s)
{
	struct SOCKET *p_sock = NULL;
	s32_t i_Ret = 0;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(!(s < 0),-1);
	
	p_sock = get_socket(s);
	if(p_sock == NULL)
	{
		SMSC_ERROR(("Can not find the socket[%d]",s));
		i_Ret = 0;
	}
	else if(p_sock->mType != SOCKET_TYPE_TCP)
	{
		SMSC_ERROR(("socket[%d] is not TCP",s));
		i_Ret = 0;
	}
	else if (p_sock && (!PacketQueue_IsEmpty(&(p_sock->mReceiveQueue))))
	{
		i_Ret = 0;
	}
	else if(p_sock->mControlBlock.mTcp == NULL || p_sock->mReceiveFinishedFlag
		/*|| p_sock->mControlBlock.mTcp->mState == TCP_STATE_CLOSED*/)
	{
		SMSC_TRACE(TCPIP_DEBUG,("CHMID_TCPIP_GetTCPStatus: fd[%d] closed\n", s));
		i_Ret = 1;
	}
	else 
	{
		i_Ret = 0;
	}
	return i_Ret;
}

void ch_tcpip_printsocketinfo(int sock_index)
{
	struct SOCKET * pSOCK = NULL;

	pSOCK = get_socket(sock_index);
	if(pSOCK == NULL)
	{
		SMSC_ERROR(("Failed to find socket[%d]'s struct!!!\n",sock_index));
	}
	else
	{
		SMSC_TRACE(1,("socket[%d]->localport = [%d]\n",sock_index,pSOCK->mControlBlock.mTcp->mLocalPort));
	}
	return ;
}
/*EOF*/
