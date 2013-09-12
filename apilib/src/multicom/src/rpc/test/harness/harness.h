/*
 * harness.h
 *
 * Copyright (C) STMicroelectronics Limited 2002. All rights reserved.
 *
 * 
 */

#ifndef rpc_harness_h
#define rpc_harness_h

/* a macro to enable verbose logging */
#if defined DEBUG
#define VERBOSE(x) x
/* the Linux kernel headers must not see DEBUG defined */
#undef DEBUG
#else
#define VERBOSE(x) ((void) 0)
#endif

/*
 * universal set of headers 
 */

#if defined __LINUX__ && defined __KERNEL__

#include <linux/errno.h>
#include <linux/kernel.h>
#define  __NO_VERSION__
#include <linux/module.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/version.h>
#include <linux/smp_lock.h>
#include <linux/reboot.h>

/* deal with CONFIG_MODVERSIONS */
#if CONFIG_MODVERSIONS == 1
#define MODVERSIONS
#include <linux/modversions.h>
#endif

extern int rpcErrno;
#define errno rpcErrno

/* mangle some of the normal C functions that do not exist in kernel space */
#define assert(_e) \
	((void) ((_e) ?  0 : printk ("ERROR: assertion failure in %s() at %s:%d: \"%s\"\n", \
			             __FUNCTION__, __FILE__, __LINE__, # _e)))
#define exit(code) printk("WARNING: cannot exit (%d) from kernel module\n", code)
#define free(block) kfree(block)
#define fprintf(stream, fmt, args...) printk(fmt, ##args)
#define fflush(stream)
#define printf(fmt, args...) printk(fmt, ##args)
#define malloc(size) kmalloc(size, GFP_KERNEL)

#else

#include <assert.h>
#include <errno.h>
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
#define CPU_FOR_MASTER CPU_ST20
#define CPU_FOR_SLAVE  CPU_ST20
#elif defined __SH4__
#define CPU "SH4"
#define CPU_FOR_MASTER CPU_ST40
#define CPU_FOR_SLAVE  CPU_ST40
#elif defined __ST200__
#define CPU "ST200"
#define CPU_FOR_MASTER CPU_ST200
#define CPU_FOR_SLAVE  CPU_ST200
#else
#define CPU "Unknown!"
#endif

#if defined __OS20__
#define OS "OS20"
#define OS_FOR_MASTER OS_OS20
#define OS_FOR_SLAVE  OS_OS20
#elif defined __OS21__
#define OS "OS21"
#define OS_FOR_MASTER OS_OS21
#define OS_FOR_SLAVE  OS_OS21
#elif defined __LINUX__
#ifdef __KERNEL__
#define OS "Linux kernel"
#define OS_FOR_MASTER OS_LINUX_KERNEL
#define OS_FOR_SLAVE  OS_LINUX_KERNEL
#else
#define OS "Linux"
#define OS_FOR_MASTER OS_LINUX_USER
#define OS_FOR_SLAVE  OS_LINUX_USER
#endif
#else
#define OS "Unknown!"
#endif

#define MACHINE CPU"/"OS


/*
 * provide the concept of local and remote for use in creating and
 * connected to ports
 */

#if defined CONF_MASTER
#define LOCAL  "Master"
#define REMOTE "Slave"
#undef  CPU_FOR_SLAVE
#undef  OS_FOR_SLAVE
#else
#define LOCAL  "Slave"
#define REMOTE "Master"
#undef  CPU_FOR_MASTER
#undef  OS_FOR_MASTER
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
#define EMBX_E(err, call) ( VERBOSE(printf(MACHINE": "#err" = EMBX_"#call"\n")), \
                            assert(err == (lastError = EMBX_##call)) )
#define EMBX_I(call)      ( VERBOSE(printf(MACHINE": ?? = EMBX_"#call"\n")), \
			    (lastError = EMBX_##call) )
#define EMBX(call)        EMBX_E(EMBX_SUCCESS, call)


/*
 * prototypes
 */

void harness_boot(void);
void harness_block(void);
unsigned int harness_getTicksPerSecond(void);
unsigned int harness_getTime(void);
void harness_sleep(int seconds);
char *harness_getTransportName(void);
void harness_passed(void);
void harness_waitForShutdown(void);
void harness_sendShutdown(void);
void harness_sync(void);

import {
	{ harness_sendShutdown, mstr, slve }
};

#endif /* rpc_harness_h */
