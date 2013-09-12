/*
 * rpc_userver.c
 *
 * Copyright (C) STMicroelectronics Limited 2003. All rights reserved.
 *
 * RPC Micro Server - this Linux specific kernel component connects
 * the user space RPC stubs with the EMBX2 API implemented in kernel
 * space.
 */

/* this is pure Linux kernel code therefore this code is written not
 * in ANSI C but in GNU C. in particular readers are reminded that 
 * when performing pointer arithmetic on void pointers GNU C assumes 
 * that sizeof(void) == sizeof(char).
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/version.h>
#include <linux/vmalloc.h>
#include <asm/semaphore.h>
#include <asm/uaccess.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
/* Deal with CONFIG_MODVERSIONS */
#if CONFIG_MODVERSIONS==1
#define MODVERSIONS
#include <linux/modversions.h>
#endif
#endif

#ifdef RPC_VERBOSE
#define VERBOSE(x) x
#else
#define VERBOSE(x)
#endif

#include <embx.h>

#include "rpc_userver.h"

/* types and typed constants */
enum MESSAGE_TYPE {
    MESSAGE_TYPE_INVALID,
    MESSAGE_TYPE_REQUEST,
    MESSAGE_TYPE_REPLY
};

enum MESSAGE_HEADER {
    MESSAGE_HEADER_TYPE,
    MESSAGE_HEADER_ARENA,
    MESSAGE_HEADER_FUNCID,
    MESSAGE_HEADER_EVENT,
    MESSAGE_HEADER_END,

    MESSAGE_HEADER_SIZEOF = (4 * MESSAGE_HEADER_END)
};

typedef struct userver_event {
	int *buf;
	int bufSize;
	struct semaphore signal;
} userver_event_t;


MODULE_DESCRIPTION("STRPC Micro Server for Linux");
MODULE_AUTHOR("STMicroelectronics Limited");
MODULE_LICENSE("Copyright (C) 2003 STMicroelectronics Limited. All rights reserved.");

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
EXPORT_NO_SYMBOLS;
#endif

/* this is the same major device number as the original RPC server.
 * technically speaking this number is reserved for experimental
 * driver work ...
 */
#define USERVER_DEVICE_MAJOR 240

#define USERVER_NUM_PORTS 128
#define USERVER_NUM_MESSAGES 128
#define USERVER_NUM_HASH_ENTRIES 509 /* largest prime < 512 */

typedef struct userver_ctx {
	EMBX_TRANSPORT      transportHandle;
	EMBX_PORT           localPort;
	struct semaphore    remoteMutex;
	int	            remoteIndex;
	EMBX_PORT           remotePorts[USERVER_NUM_PORTS];
	struct semaphore    messageMutex;
	EMBX_RECEIVE_EVENT  messages[USERVER_NUM_MESSAGES];
	struct semaphore    hashTableMutex;
	struct userver_ctx_hash {
		void *key;
		void *value;
	} hashTable[USERVER_NUM_HASH_ENTRIES];
} userver_ctx_t;

/* this is a very simplistic hash table implementation. its performance
 * will massively degrade if it gets full as it has a very high
 * miss cost (in worst case the whole table must be searched). fortunately 
 * we know that in this case it will never be densely filled and should
 * never miss in normal use. specifically the maximum number of hashable 
 * elements is (USERVER_NUM_PORTS + USERVER_NUM_MESSAGES) versus a maximum 
 * storage of USERVER_NUM_HASH_ENTRIES.
 */
static inline unsigned int userver_gethashvalue(userver_ctx_t *ctx, void *key, void *find)
{
	unsigned int i, hash;

	i = hash = (unsigned int) key % USERVER_NUM_HASH_ENTRIES;
	do {
		if (find == ctx->hashTable[i].key) {
			break;
		}
		i = (i == (USERVER_NUM_HASH_ENTRIES-1) ? 0 : i+1);
	} while (i!=hash);

	return i;
}

static inline int userver_hash(userver_ctx_t *ctx, void *key, void *value)
{
	unsigned int hash;
	int res;

	down(&(ctx->hashTableMutex));

	hash = userver_gethashvalue(ctx, key, NULL);
	if (NULL == ctx->hashTable[hash].key) {
		ctx->hashTable[hash].key = key;
		ctx->hashTable[hash].value = value;
		res = 0;
	} else { 
		res = -ENOMEM;
	}

	up(&(ctx->hashTableMutex));
	return res;
}

static inline int userver_unhash(userver_ctx_t *ctx, void *key, void **value)
{
	unsigned int hash;
	int res;

	down(&(ctx->hashTableMutex));

	hash = userver_gethashvalue(ctx, key, key);
	if (key == ctx->hashTable[hash].key) {
		*value = ctx->hashTable[hash].value;
		ctx->hashTable[hash].key = 0;
		res = 0;
	} else {
		res = -EINVAL;
	}

	up(&(ctx->hashTableMutex));
	return res;
}

static inline int userver_init(userver_ctx_t *ctx, RPCIOCTL_Init_t *arg)
{ 
	RPCIOCTL_Init_t request;
	int len;
	char transportName[EMBX_MAX_TRANSPORT_NAME+1];
	char portName[EMBX_MAX_PORT_NAME+1];
	EMBX_ERROR err;

	if (0 != copy_from_user(&request, arg, sizeof(*arg))) {
		return -EFAULT;
	}

	if (request.transportName) {
		len = strncpy_from_user(&transportName, request.transportName, EMBX_MAX_TRANSPORT_NAME);
		transportName[EMBX_MAX_TRANSPORT_NAME] = '\0';
		if (len < 0) {
			return -EFAULT;
		}
	} else {
		transportName[0] = '\0';
	}

	len = strncpy_from_user(&portName, request.portName, EMBX_MAX_PORT_NAME);
	portName[EMBX_MAX_PORT_NAME] = '\0';
	if (len < 0) {
		return -EFAULT;
	}

	/* open a transport handle (use EMBX_GetFirstTransport if an anonymous transport
	 * was requested).
	 */
	if ('\0' == transportName[0]) {
		EMBX_TPINFO tpinfo;

		err = EMBX_GetFirstTransport(&tpinfo);
		if (EMBX_SUCCESS != err) {
			return -EINVAL;
		}

		err = EMBX_OpenTransport(tpinfo.name, &ctx->transportHandle);
	} else {
		err = EMBX_OpenTransport(transportName, &ctx->transportHandle);
	}
	if (EMBX_SUCCESS != err) {
		return -EINVAL;
	}

	err = EMBX_CreatePort(ctx->transportHandle, portName, &ctx->localPort);
	if (EMBX_SUCCESS != err) {
		EMBX_CloseTransport(ctx->transportHandle);
		return -EINVAL;
	}
	
	if (0 != put_user(ctx->transportHandle, &(arg->transportHandle)) ||
	    0 != put_user(ctx->localPort, &(arg->portHandle))) {
		EMBX_ClosePort(ctx->localPort);
		EMBX_CloseTransport(ctx->transportHandle);
		return -EFAULT;
	}

	return 0;
}

static inline int userver_connect(userver_ctx_t *ctx, RPCIOCTL_Connect_t *arg)
{
	RPCIOCTL_Connect_t  request;
	int                 len;
	char                portName[EMBX_MAX_PORT_NAME+1];
	EMBX_ERROR          err;
	int                 portHandle;

	if (0 != copy_from_user(&request, arg, sizeof(*arg))) {
		return -EFAULT;
	}

	len = strncpy_from_user(&portName, request.portName, EMBX_MAX_PORT_NAME);
	portName[EMBX_MAX_PORT_NAME] = '\0';
	if (len < 0) {
		return -EFAULT;
	}

	if (request.transportHandle != ctx->transportHandle) {
		return -EINVAL;
	}

	/* allocate a port handle */
	down(&(ctx->remoteMutex));
	portHandle = ctx->remoteIndex++;
	up(&(ctx->remoteMutex));
	if (portHandle > USERVER_NUM_PORTS) {
		ctx->remoteIndex = USERVER_NUM_PORTS+1; /* prevent an overflow attack */
		return -ENOMEM;
	}

	err = EMBX_ConnectBlock(ctx->transportHandle, portName, &(ctx->remotePorts[portHandle]));
	if (EMBX_SUCCESS != err) {
		return -EINVAL;
	}

	if (0 != put_user(portHandle, &(arg->portHandle))) {
		return -EFAULT;
	}

	return 0;
}
	
static inline int userver_wait(userver_ctx_t *ctx, RPCIOCTL_Wait_t *arg)
{
	RPCIOCTL_Wait_t     request;
	int                 i;
	int                 messageHandle;
	EMBX_RECEIVE_EVENT *message;
	EMBX_ERROR          err;
	unsigned int	    bufferSize;
	int		   *buffer;
	userver_event_t    *event;

	if (0 != copy_from_user(&request, arg, sizeof(*arg))) {
		return -EFAULT;
	}

	if (request.portHandle != ctx->localPort) {
		return -EINVAL;
	}

	/* allocate somewhere to keep the message */
	down(&(ctx->messageMutex));
	for (messageHandle=-1, i=0; i<USERVER_NUM_MESSAGES; i++) {
		if (0 == ctx->messages[i].size) {
			ctx->messages[i].size = 1;
			messageHandle = i;
			break;
		}
	}
	up(&(ctx->messageMutex));
	if (-1 == messageHandle) {
		return -ENOMEM;
	}

	message = &(ctx->messages[messageHandle]);

	while (1) {
		err = EMBX_ReceiveBlock(ctx->localPort, message);
		if (EMBX_SUCCESS != err || 
		    EMBX_REC_MESSAGE != message->type) {
			message->size = 0;
			return (EMBX_SYSTEM_INTERRUPT == err ? -EINTR : -EIO);
		}

		buffer = (int *) message->data;
		if (MESSAGE_TYPE_REQUEST == buffer[MESSAGE_HEADER_TYPE]) {
			break;
		}

		/* if we reach this point then we are working with an incoming
		 * reply which we can handle without returning to userland.
		 */
		event = (userver_event_t *) buffer[MESSAGE_HEADER_EVENT];
		if (0 != userver_unhash(ctx, event, (void **) &event->buf)) {
			message->size = 0;
			return -EBADMSG;
		}
		event->buf = buffer;
		event->bufSize = message->size;
		up(&(event->signal));
	}

	err = EMBX_GetBufferSize(message->data, &bufferSize);
	if (EMBX_SUCCESS != err || MESSAGE_HEADER_SIZEOF > bufferSize) {
		EMBX_Free(message->data);
		message->size = 0;
		return -EIO;
	}

	if (0 != put_user(messageHandle, &(arg->messageHandle)) ||
	    0 != put_user(bufferSize, &(arg->bufferSize))) {
		EMBX_Free(message->data);
		message->size = 0;
		return -EFAULT;
	}

	return 0;
}

/* fetch a request into memory allocated in userland */
static inline int userver_fetch(userver_ctx_t *ctx, RPCIOCTL_Fetch_t *arg)
{
	RPCIOCTL_Fetch_t    request;
	EMBX_RECEIVE_EVENT *message;

	if (0 != copy_from_user(&request, arg, sizeof(*arg))) {
		return -EFAULT;
	}

	/* ensure the messageHandle with within an appropriate range */
	if (request.messageHandle > USERVER_NUM_MESSAGES) {
		return -EINVAL;
	}

	message = &(ctx->messages[request.messageHandle]);

	if (request.portHandle != ctx->localPort ||
	    0 == message->size) {
		return -EINVAL;
	}

	if (0 != copy_to_user(request.buffer, message->data, message->size) ||
	    0 != put_user(message->size, &(arg->messageSize))) {
		return -EFAULT;
	}

	/* if this is a message request store the EMBX buffer in a hash table
	 * rather than freeing it, we will be able to pluck it out later.
	 */
	if (0 != userver_hash(ctx, request.buffer, message->data)) {
		EMBX_Free(message->data);
	}

	message->size = 0;
	return 0;
}


static inline int userver_copy_with_scatter(void *buffer, const RPCIOCTL_Send_t * request)
{
	/* both request and buffer are pointers into kernel space memory */

	RPCIOCTL_ScatterList_t scatter = { next: request->scatterList };
	int                    res = 0;
	unsigned int           offset = request->bufferSize;

	while (NULL != scatter.next) {
		/* fetch the next scatter request */
		res = copy_from_user(&scatter, scatter.next, sizeof(scatter));
		if (0 != res) { break; }

		/* validate the scatter request. on a machine with infinate precision
		 * arithmetic only the last test is needed but on real machines ...
		 */
		if (scatter.size == 0 ||
		    scatter.size > request->bufferSize ||
		    scatter.offset > offset ||
		    scatter.offset + scatter.size > offset) {
			EMBX_Free(buffer);
			return -EINVAL;
		}

		/* copy the data after the request */
		res = copy_from_user(buffer + scatter.offset + scatter.size,
				     request->buffer + scatter.offset + scatter.size,
				     offset - (scatter.offset + scatter.size));
		if (0 != res) { break; }

		/* now copy the request itself */
		res = copy_from_user(buffer + scatter.offset,
				     scatter.data,
				     scatter.size);
		if (0 != res) { break; }

		/* update the end-of-writable-area marker to the beginning of the 
		 * scattered section 
		 */
		offset = scatter.offset;
	}

	if (0 == res) {
		res = copy_from_user(buffer, request->buffer, offset);
	}

	/* if any copy_from_user() failed we will reach this point */
	if (0 != res) {
		EMBX_Free(buffer);
		return -EFAULT;
	}

	return 0;
}

static inline int userver_reply(userver_ctx_t *ctx, RPCIOCTL_Send_t *arg)
{
	RPCIOCTL_Send_t         request;
	EMBX_ERROR              err;
	EMBX_VOID              *buffer = NULL;

	if (0 != copy_from_user(&request, arg, sizeof(*arg))) {
		return -EFAULT;
	}

	if (request.transportHandle != ctx->transportHandle ||
	    request.portHandle >= ctx->remoteIndex) {
		return -EINVAL;
	}

	/* look up the EMBX buffer in the hash table (yes, it is expensive *if*
	 * we miss that hash table but we're not going to miss - although we do
	 * handle this gracefully just in case).
	 */
	if (0 != userver_unhash(ctx, request.buffer, &buffer)) {
		err = EMBX_Alloc(ctx->transportHandle, request.bufferSize, &buffer);
		if (EMBX_SUCCESS != err) {
			return -ENOMEM;
		}
	}

	if (NULL == request.scatterList) {
		if (0 != copy_from_user(buffer, request.buffer, request.messageSize)) {
			EMBX_Free(buffer);
			return -EFAULT;
		}
	} else {
		int err = userver_copy_with_scatter(buffer, &request);
		if (0 != err) {
			EMBX_Free(buffer);
			return err;
		}
	}

	err = EMBX_SendMessage(ctx->remotePorts[request.portHandle], buffer, request.messageSize);
	if (EMBX_SUCCESS != err) {
		EMBX_Free(buffer);
		return -EIO;
	}

	return 0;
}

static inline int userver_request(userver_ctx_t *ctx, RPCIOCTL_Send_t *arg)
{
	RPCIOCTL_Send_t         request;
	EMBX_ERROR              err;
	int		       *buffer = NULL;
	userver_event_t		event;

	if (0 != copy_from_user(&request, arg, sizeof(*arg))) {
		return -EFAULT;
	}

	if (request.transportHandle != ctx->transportHandle ||
	    request.portHandle >= ctx->remoteIndex) {
		return -EINVAL;
	}

	err = EMBX_Alloc(ctx->transportHandle, request.bufferSize, (void **) &buffer);
	if (EMBX_SUCCESS != err) {
		return -ENOMEM;
	}

	/* push the event into the hash table so we can validate it later */
	sema_init(&(event.signal), 0);
	if (0 != userver_hash(ctx, &event, buffer)) {
		EMBX_Free(buffer);
		return -ENOMEM;
	}

	if (NULL == request.scatterList) {
		if (0 != copy_from_user(buffer, request.buffer, request.messageSize)) {
			EMBX_Free(buffer);
			return -EFAULT;
		}
	} else {
		int err = userver_copy_with_scatter(buffer, &request);
		if (0 != err) {
			EMBX_Free(buffer);
			return err;
		}
	}

	/* ensure the event is appropriately recorded in the message buffer */
	buffer[MESSAGE_HEADER_EVENT] = (int) &event;

	/* send the message */
	err = EMBX_SendMessage(ctx->remotePorts[request.portHandle], buffer, request.messageSize);
	if (EMBX_SUCCESS != err) {
		EMBX_Free(buffer);
		return -EIO;
	}

	/* now wait for the reply to be picked up (side effect of in progress wait ioctls) */
	if (0 != down_interruptible(&(event.signal))) {
		return -EINTR;
	}

	/* TODO: need to un-scatter! */
	if (0 != copy_to_user(request.buffer, event.buf, event.bufSize)) {
		EMBX_Free(event.buf);
		return EFAULT;
	}

	EMBX_Free(event.buf);
	return 0;
}

static inline int userver_deinit(userver_ctx_t *ctx)
{
	EMBX_ERROR err;
	
	err = EMBX_InvalidatePort(ctx->localPort);
	if (EMBX_SUCCESS != err) {
		return -EINVAL;
	}

	return 0;
}

static int userver_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	userver_ctx_t *ctx = filp->private_data;
	int res;

	VERBOSE(printk("rpc_userver(%p): about to issue ioctl\n"));

	switch(cmd) {
	case RPCIOCTL_INIT:
		VERBOSE(printk("rpc_userver(%p): RPCIOCTL_INIT\n", filp->private_data));
		res = userver_init(ctx, (RPCIOCTL_Init_t *) arg);
		break;
	
	case RPCIOCTL_CONNECT:
		VERBOSE(printk("rpc_userver(%p): RPCIOCTL_CONNECT\n", filp->private_data));
		res = userver_connect(ctx, (RPCIOCTL_Connect_t *) arg);
		break;

	case RPCIOCTL_WAIT:
		VERBOSE(printk("rpc_userver(%p): RPCIOCTL_WAIT\n", filp->private_data));
		res = userver_wait(ctx, (RPCIOCTL_Wait_t *) arg);
		break;

	case RPCIOCTL_FETCH:
		VERBOSE(printk("rpc_userver(%p): RPCIOCTL_FETCH\n", filp->private_data));
		res = userver_fetch(ctx, (RPCIOCTL_Fetch_t *) arg);
		break;

	case RPCIOCTL_REPLY:
		VERBOSE(printk("rpc_userver(%p): RPCIOCTL_REPLY\n", filp->private_data));
		res = userver_reply(ctx, (RPCIOCTL_Send_t *) arg);
		break;

	case RPCIOCTL_REQUEST:
		VERBOSE(printk("rpc_userver(%p): RPCIOCTL_REQUEST\n", filp->private_data));
		res = userver_request(ctx, (RPCIOCTL_Send_t *) arg);
		break;
	
	case RPCIOCTL_DEINIT:
		VERBOSE(printk("rpc_userver(%p): RPCIOCTL_DEINIT\n", filp->private_data));
		res = userver_deinit(ctx);
		break;

	default:
		VERBOSE(printk("rpc_userver(%p): illegal ioctl operation\n", filp->private_data));
		/* ENOTTY - Inappropriate I/O control operation */
		res = -ENOTTY;
		break;
	}

	if (res < 0) {
		VERBOSE(printk("rpc_userver(%p): ioctl failed (err = %d)\n", ctx, -1 * res));
	}

	return res;
}

static int userver_open(struct inode *inode, struct file *filp)
{
	userver_ctx_t *ctx;

	ctx = vmalloc(sizeof(userver_ctx_t));
	if (NULL == ctx) {
		return -ENOMEM;
	}

	/* initialize the context structure */
	memset(ctx, 0, sizeof(*ctx));
	sema_init(&(ctx->remoteMutex), 1);
	sema_init(&(ctx->messageMutex), 1);
	sema_init(&(ctx->hashTableMutex), 1);
	filp->private_data = ctx;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,10)
	MOD_INC_USE_COUNT;
#endif

	VERBOSE(printk("rpc_userver(%p): device opened\n", ctx));
	return 0;
}

static int userver_close(struct inode *inode, struct file *filp)
{
	userver_ctx_t *ctx = filp->private_data;

	int i;

	VERBOSE(printk("rpc_userver(%p): device closed\n", ctx));

	/* TODO: make this thread safe */

	/* close all the remote ports */
	for (i=0; i<ctx->remoteIndex; i++) {
		(void) EMBX_ClosePort(ctx->remotePorts[i]);
	}

	/* close the local request port */
	(void) EMBX_ClosePort(ctx->localPort);
	ctx->localPort = 0;
	
	/* clean up any unclaimed buffers */
	for (i=0; i<USERVER_NUM_MESSAGES; i++) {
		if (0 != ctx->messages[i].size) {
			(void) EMBX_Free(ctx->messages[i].data);
			ctx->messages[i].size = 0;
		}
	}

	/* TODO: free messages cached in the hash table */

	(void) EMBX_CloseTransport(ctx->transportHandle);

	vfree(ctx);
	filp->private_data = NULL;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,10)
	MOD_DEC_USE_COUNT;
#endif
	return 0;
}

static struct file_operations userver_file_ops = {
	owner:		THIS_MODULE,
	ioctl:		userver_ioctl,
	open:		userver_open,
	release:	userver_close
};

int __init userver_module_init(void)
{
	int res;

	/* register our device */
	res = register_chrdev(USERVER_DEVICE_MAJOR, "userver", &userver_file_ops);
	if (0 != res) {
		return res;
	}

	VERBOSE(printk("rpc_userver: module initialized\n"));

	return 0;
}

void __exit userver_module_deinit(void)
{
	VERBOSE(printk("rpc_userver: module deinitialized\n"));

	/* TODO: unregister the device! */
}

module_init(userver_module_init);
module_exit(userver_module_deinit);
