/*
 * harness/harness.c
 *
 * Copyright (C) STMicroelectronics Limited 2003. All rights reserved.
 *
 * 
 */

#if defined __OS20__
#include <cache.h>
#include <interrup.h>
#include <task.h>
#include <ostime.h>
#endif

#if defined __OS21__
#include <os21.h>
#if defined __SH4__
#include <os21/st40.h>
#endif
#endif

#include <stdarg.h>

#include "harness.h"

#include "embx_osinterface.h"

int lastError;

static EMBX_TRANSPORT transport = 0;

EMBX_TRANSPORT harness_openTransport(void)
{
	EMBX(OpenTransport(harness_getTransportName(), &transport));

	return transport;
}

EMBX_TRANSPORT harness_getTransport(void)
{
	return transport;
}

void harness_closeTransport(void)
{
	EMBX(CloseTransport(transport));
	transport = 0;
}

void harness_passed()
{
	EMBX_PORT          port;
	EMBX_ERROR         err;
	EMBX_RECEIVE_EVENT event;

	/* ensure the EMBX is alive (some MME implementions have already shut it down */
	EMBX_I(Init());

	if (0 == transport) {
		EMBX(OpenTransport(harness_getTransportName(), &transport));
	}
	EMBX(CreatePort(transport, "shutdownPort", &port));
	printf("******** "CPU"/"OS" waiting for shutdown acknowledge ********\n");
	err = EMBX_I(ReceiveBlock(port, &event));
	if (EMBX_SUCCESS == err) {
		assert(EMBX_REC_MESSAGE == event.type);
		assert(0xdead == *((EMBX_UINT *) event.data));
		EMBX(Free(event.data));
	} else {
		assert(EMBX_PORT_INVALIDATED == err ||
		       EMBX_INVALID_PORT     == err);
	}

#ifndef CONF_NO_SHUTDOWN
	/* the only way Deinit()'s code path will be heavily tested
	 * it to include it in every test. Note that this will not
	 * prove that we can re-initialize.
	 */
	EMBX(ClosePort(port));
	EMBX(CloseTransport(transport));
	EMBX(Deinit());

	harness_deinit();
#endif

	/* provide a few moments for our partners runtime loader to exit */
	harness_sleep(5);

	printf("******** Passed ********\n");

	exit(0);
}


	
void harness_waitForShutdown()
{
	EMBX_PORT       port;
	void           *mem;
	EMBX_UINT      *deadMarker;

	/* ensure the EMBX is alive (some MME implementions have already shut it down */
	EMBX_I(Init());

	if (0 == transport) {
		EMBX(OpenTransport(harness_getTransportName(), &transport));
	}

	/* at this point we will block until our partner calls
         * harness_passed() and creates the shutdown port
	 */
	printf("******** "CPU"/"OS" waiting for shutdown request ********\n");
	EMBX(ConnectBlock(transport, "shutdownPort", &port));

	/* this will unblock our partner */
	EMBX(Alloc(transport, sizeof(*deadMarker), (EMBX_VOID **) &mem));
	deadMarker = (EMBX_UINT *) mem;
	*deadMarker = 0xdead;

	EMBX(SendMessage(port, deadMarker, sizeof(*deadMarker)));
#if 1

	/* TODO: Linux timing bug: DOn't terminate yet! */
	EMBX(ClosePort(port));
	EMBX(CloseTransport(transport));
#define SLEEP 20        
        harness_sleep(SLEEP);
	EMBX(Deinit());

	harness_deinit();
#else
#ifndef CONF_NO_SHUTDOWN
	/* the only way Deinit()'s code path will be heavily tested
	 * it to include it in every test. Note that this will not
	 * prove that we can re-initialize.
	 */
	EMBX(ClosePort(port));
	EMBX(CloseTransport(transport));
	EMBX(Deinit());

	harness_deinit();
#endif
#endif

	printf("******** "CPU"/"OS" shutdown OK ********\n");
	exit(0);
}

void harness_connect(EMBX_TRANSPORT transport, EMBX_CHAR *name, EMBX_PORT *pInPort, EMBX_PORT *pOutPort)
{
#if !defined(__KERNEL__)
	int   maxNameLen;
	EMBX_CHAR *inName;
	EMBX_CHAR *outName;

	assert(name);
	assert(pInPort);
	assert(pOutPort);

	maxNameLen = strlen(name) + 16;
	inName = malloc(maxNameLen);
	outName = malloc(maxNameLen);
	 
	assert(inName);
	assert(outName);

	strcpy(inName, name);
	strcpy(outName, name);
	strcat(inName, LOCAL);
	strcat(outName, REMOTE);
	
	EMBX(CreatePort(transport, inName, pInPort));
	EMBX(ConnectBlock(transport, outName, pOutPort));

	printf("******** "CPU"/"OS" connected to %s ********\n", name);
#endif
}

void harness_waitForSignal(EMBX_PORT port)
{
	EMBX_RECEIVE_EVENT event;

	EMBX(ReceiveBlock(port, &event));
	assert(0 == event.size);

	EMBX(Free(event.data));
}

void harness_sendSignal(EMBX_TRANSPORT transport, EMBX_PORT port)
{
	EMBX_VOID *buffer;
	EMBX(Alloc(transport, 1, &buffer));
	EMBX(SendMessage(port, buffer, 0));
}

void harness_rendezvous(EMBX_TRANSPORT transport, EMBX_PORT inPort, EMBX_PORT outPort)
{
	harness_sendSignal(transport, outPort);
	harness_waitForSignal(inPort);
}

#if 0
static task_t *children[16];
#else
#define MAX_THREADS 256
static EMBX_THREAD children[MAX_THREADS];
#endif
static int     numChildren = 0;

void harness_createThread(void (*entry)(void*), void *param)
{
	int n;

	assert(numChildren < (MAX_THREADS-1));
	EMBX_OS_INTERRUPT_LOCK();
	n = numChildren++;
	EMBX_OS_INTERRUPT_UNLOCK();
	children[n] = EMBX_OS_ThreadCreate( entry, param, 
			EMBX_DEFAULT_THREAD_PRIORITY, EMBX_DEFAULT_THREAD_NAME);
	assert(EMBX_INVALID_THREAD != children[n]);

}

void harness_waitForChildren(void)
{
	EMBX_ERROR res;
	while (0 != numChildren) {
		res = EMBX_OS_ThreadDelete(children[numChildren-1]);
		assert(EMBX_SUCCESS == res);
		
		numChildren--;
	}
}

void harness_interruptLock(void)
{
#if defined __OS20__ || defined __OS21__
	interrupt_lock();
#endif
}

void harness_interruptUnlock(void)
{
#if defined __OS20__ || defined __OS21__
	interrupt_unlock();
#endif
}

void harness_fillPattern(void *pBuf, unsigned int size, unsigned int pattern)
{
	unsigned int *pFill = (unsigned int *) pBuf;
	
	while (size >= 4) {
		*pFill++ = pattern;
		size -= 4;
		pattern = (0xfffffff0 & (pattern << 4 )) | 
		          (0x0000000f & (pattern >> 28));
	}
}

int harness_testPattern(void *pBuf, unsigned int size, unsigned int pattern)
{
	unsigned int *pFill = (unsigned int *) pBuf;
	
	VERBOSE(printf(MACHINE"harness_testPattern(0x%08x, %d, 0x%x)\n", (unsigned) pBuf, size, pattern));

	while (size >= 4) {
		if (*pFill++ != pattern) {
			pFill--;
			printf(MACHINE"testPattern failed at 0x%08x - expected 0x%08x; got 0x%08x\n", pFill, pattern, *pFill);
			return 0;
		}
		size -= 4;
		pattern = (0xfffffff0 & (pattern << 4 )) | 
		          (0x0000000f & (pattern >> 28));
	}

	return 1;
}

#if !defined __LINUX__ && !defined __KERNEL__

int harness_printf(char *fmt, ...)
{
	int res;
	va_list ap;
	va_start(ap, fmt);

	res = vprintf(fmt, ap);
	fflush(stdout);

	va_end(ap);
	return res;
}

#endif
