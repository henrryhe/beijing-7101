/*
 * harness.h
 *
 * Copyright (C) STMicroelectronics Limited 2002. All rights reserved.
 *
 * 
 */

#ifndef rpc_harness_h
#define rpc_harness_h

/* 
 * This must be at the top of this file to avoid problems with Linux kernel mode
 */

/* a macro to enable verbose logging */
#if defined DEBUG
#define VERBOSE(x) x
/* the Linux kernel headers must not see DEBUG defined */
#undef DEBUG
#else
#define VERBOSE(x) ((void) 0)
#endif

#ifdef SIM231
#define CONF_SLOW_SIMULATOR
#endif

/*
 * universal set of headers 
 */

#if defined __LINUX__ 

#define assert(_e) \
    ((void) ((_e) ?  0 : \
     ( harness_exit(1, "ERROR: assertion failure in %s() at %s:%d: \"%s\"\n",  \
      __FUNCTION__, __FILE__, __LINE__, # _e) \
     )))
void harness_exit(int code, char* str, ...);

#if defined __KERNEL__

#include <linux/errno.h>
#include <linux/kernel.h>
#define  __NO_VERSION__
#include <linux/module.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/version.h>

/* deal with CONFIG_MODVERSIONS */
#if CONFIG_MODVERSIONS == 1
#define MODVERSIONS
#include <linux/modversions.h>
#endif

/* mangle some of the normal C functions that do not exist in kernel space */

#define exit(code) harness_exit(code, NULL)
#define free(block) kfree(block)
#define fprintf(stream, fmt, args...) printk(fmt, ##args)
#define fflush(stream)
#define printf(fmt, args...) printk(fmt, ##args)
#define harness_printf(fmt, args...) printk(fmt, ##args)
#define malloc(size) kmalloc(size, GFP_KERNEL)

#define main(X) init_module(X)
#define harness_schedule() schedule()

#else /* __KERNEL__ */

#define USERMODE
#include <time.h>
#define harness_schedule()

#endif /* __KERNEL__ */

#else  /* __LINUX__ */

#ifdef __WINCE__
/* for some reason assert.h for Windows automatically sets NDEBUG if DEBUG is not set */
#define DEBUG
#endif

#include <assert.h>
#define harness_schedule()

#endif /* __LINUX__ */

#if !defined __KERNEL__

#if !defined __WINCE__
#include <errno.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#endif

#include <embx.h>

/*
 * convert macros in the cpp namespace into macros for use in logging
 */

#if defined __ST20__
#define CPU "ST20"
#elif defined __SH4__
#define CPU "SH4"
#elif defined __ST200__
#define CPU "ST200"
#elif defined __IA32__
#define CPU "x86"
#else
#define CPU "Unknown!"
#endif

#if defined __OS20__
#define OS "OS20"
#elif defined __OS21__
#define OS "OS21"
#elif defined __LINUX__
#define OS "Linux"
#elif defined __WINCE__
#define OS "WindowsCE"
#else
#define OS "Unknown!"
#endif

#ifdef CONF_MASTER
#define MACHINE CPU"/"OS" (mstr): "
#else
#define MACHINE CPU"/"OS" (slve): "
#endif

/*
 * provide the concept of local and remote for use in creating and
 * connected to ports
 */

#if defined CONF_MASTER
#define LOCAL  "Master"
#define REMOTE "Slave"
#else
#define LOCAL  "Slave"
#define REMOTE "Master"
#endif


/*
 * utility macros 
 */

/* a pair of macros to allow neat writing of unified code paths */
#if defined CONF_MASTER
#define MASTER(x) x
#define SLAVE(x) ((void) 0)
#else
#define MASTER(x) ((void) 0)
#define SLAVE(x) x
#endif 

/* choose the appropriate shutdown function */
#if defined CONF_MASTER
#define harness_shutdown() harness_passed()
#else
#define harness_shutdown() harness_waitForShutdown()
#endif

/* automatic error checking and logging when calling parts of the EMBX API */
extern int lastError;
#define EMBX_E(err, call) ( VERBOSE(printf(MACHINE#err" = EMBX_"#call"\n")), \
                            assert(err == (lastError = EMBX_##call)) )
#define EMBX_I(call)      ( VERBOSE(printf(MACHINE"?? = EMBX_"#call"\n")), \
			    (lastError = EMBX_##call) )
#define EMBX(call)        EMBX_E(EMBX_SUCCESS, call)

/* similar automatic error checking and logging for the MME API */
#define MME_E(err, call) ( VERBOSE(printf( MACHINE#err " = MME_"#call"\n")), \
                           assert(err == (lastError = MME_##call)) )
#define MME_I(call)      ( VERBOSE(printf( MACHINE"?? = MME_"#call"\n")), \
		           (lastError = MME_##call) )
#define MME(call)	 MME_E(MME_SUCCESS, call)

/*
 * prototypes
 */

void harness_boot(void);
void harness_block(void);
unsigned int harness_getTicksPerSecond(void);
unsigned int harness_getTime(void);
void harness_sleep(int seconds);
char *harness_getTransportName(void);
EMBX_TRANSPORT harness_openTransport(void);
EMBX_TRANSPORT harness_getTransport(void);
void harness_closeTransport(void);
void harness_passed(void);
void harness_waitForShutdown(void);
void harness_connect(EMBX_TRANSPORT transport, EMBX_CHAR *name, EMBX_PORT *pInPort, EMBX_PORT *pOutPort);
void harness_waitForSignal(EMBX_PORT port);
void harness_sendSignal(EMBX_TRANSPORT transport, EMBX_PORT port);
void harness_rendezvous(EMBX_TRANSPORT transport, EMBX_PORT inPort, EMBX_PORT outPort);
void harness_createThread(void (*entry)(void*), void *param);
void harness_waitForChildren(void);
void harness_interruptLock(void);
void harness_interruptUnlock(void);
void harness_fillPattern(void *pBuf, unsigned int size, unsigned int pattern);
int  harness_testPattern(void *pBuf, unsigned int size, unsigned int pattern);

#if defined(__LINUX__)
#if defined(__KERNEL__)
/* Kernel things */
#else
/* User mode things */
/* Remove harness calls in user app */
#define harness_printf printf

#undef  harness_shutdown
#define harness_shutdown() harness_exit(0, NULL)

#define harness_deinit

#endif /* __KERNEL__ */

#else

int  harness_printf(char *fmt, ...);
#define printf harness_printf
void harness_deinit(void);
 
#endif   /* __LINUX__ */
   
#endif /* rpc_harness_h */
