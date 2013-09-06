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
 */
/*****************************************************************************
* Modifications of this software Copyright(C) 2008, Standard Microsystems Corporation
*   All Rights Reserved.
*   The modifications to this program code are proprietary to SMSC and may not be copied,
*   distributed, or used without a license to do so.  Such license may have
*   Limited or Restricted Rights. Please refer to the license for further
*   clarification.
******************************************************************************
* FILE: smsc_environment.h
* PURPOSE: 
*    This file declares the functions/macros that are used to create an
*    environment wrapper to shield the network stack from the specifices of
*    the platform, and OS. All c files in the SMSC Network stack, include
*    this file. Porting to specific platforms requires carefully
*    implementing these features as described below.
*****************************************************************************/

#ifndef SMSC_ENVIRONMENT_H
#define SMSC_ENVIRONMENT_H

#include "custom_options.h"
#include "CHDRV_NET.h"
/*****************************************************************************
* This file should include any header files that are necessary to make
* the platform specific environment complete.
*****************************************************************************/


/*****************************************************************************
* The following are format strings which are associated with specific types
* U16_F is a format string that specifies a 16 bit unsigned integer
* S16_F is a format string that specifies a 16 bit signed integer
* X16_F is a format string that specifies a 16 bit integer in hexidecimal form
* U32_F is a format string that specifies a 32 bit unsigned integer
* S32_F is a format string that specifies a 32 bit signed integer
* X32_F is a format string that specifies a 32 bit integer in hexidecimal form* 
*****************************************************************************/
#define U16_F "hu"
#define S16_F "hd"
#define X16_F "hX"
#define U32_F "lu"
#define S32_F "ld"
#define X32_F "lX"
#define HZ CHDRV_NET_TicksOfSecond()
/*****************************************************************************
* SMSC_MINIMUM(a,b) returns the minimum of a and b
* SMSC_MAXIMUM(a,b) returns the maximum of a and b
*****************************************************************************/
#define SMSC_MINIMUM(a,b)	(((a)<(b))?(a):(b))
#define SMSC_MAXIMUM(a,b)	(((a)>(b))?(a):(b))

/*****************************************************************************
* The following macros are used for endian specification
* BIG_ENDIAN This should be an integer not equal to LITTLE_ENDIAN
* LITTLE_ENDIAN this should be an integer not equal to BIG_ENDIAN
* ENDIAN_SETTING should be set to BIG_ENDIAN if the platform is big endian
*    should be set to LITTLE_ENDIAN if the platform is little endian.
*****************************************************************************/
#ifndef LINUX_LOOPBACK_DEMO
#define BIG_ENDIAN	(0x1234)
#define LITTLE_ENDIAN	(0x4321)
#endif /* !LINUX_LOOPBACK_DEMO */
#define ENDIAN_SETTING	LITTLE_ENDIAN

/*****************************************************************************
* The following macro handle byte order conversions. As long as ENDIAN_SETTING
* is defined correctly, then these macros should already be defined correctly.
* smsc_htons, converts a u16_t from host byte order to network byte order.
* smsc_ntohs, converts a u16_t from network byte order to host byte order.
* smsc_htonl, converts a u32_t from host byte order to network byte order.
* smsc_ntohl, converts a u32_t from network byte order to host byte order.
*****************************************************************************/
#if (ENDIAN_SETTING==LITTLE_ENDIAN)
/* byte swap is necessary */
#define smsc_htons(num16bit)						\
	((u16_t)(((((u16_t)(num16bit))&0x00FF)<<8)|		\
	((((u16_t)(num16bit))&0xFF00)>>8)))
#define smsc_ntohs(num16bit)	smsc_htons(num16bit)
#define smsc_htonl(num32bit)		\
	(((((u32_t)(num32bit))&0x000000FF)<<24)|				\
	((((u32_t)(num32bit))&0x0000FF00)<<8)|	\
	((((u32_t)(num32bit))&0x00FF0000)>>8)|	\
	((((u32_t)(num32bit))&0xFF000000)>>24))
#define smsc_ntohl(num32bit)	smsc_htonl(num32bit)
#else
/* byte swap is not necessary */
#define smsc_htons(num16bit) num16bit
#define smsc_ntohs(num16bit) num16bit
#define smsc_htonl(num32bit) num32bit
#define smsc_ntohl(num32bit) num32bit
#endif
#ifdef smsc_printf
#undef smsc_printf
#endif
#define smsc_printf CHDRV_NET_Print
/*****************************************************************************
* The following define some error codes used in the SMSC network stack.
* These are already platform independent and do not require any changes 
* for porting.
*****************************************************************************/
typedef s8_t err_t;
/* Definitions for error constants. */
#define ERR_OK    0      /* No error, everything OK. */
#define ERR_MEM  -1      /* Out of memory error.     */
#define ERR_BUF  -2      /* Buffer error.            */
#define ERR_ABRT -3      /* Connection aborted.      */
#define ERR_RST  -4      /* Connection reset.        */
#define ERR_CLSD -5      /* Connection closed.       */
#define ERR_CONN -6      /* Not connected.           */
#define ERR_VAL  -7      /* Illegal value.           */
#define ERR_ARG  -8      /* Illegal argument.        */
#define ERR_RTE  -9      /* Routing problem.         */
#define ERR_USE  -10     /* Address in use.          */
#define ERR_IF   -11     /* Low-level netif error    */
#define ERR_ISCONN -12   /* Already connected.       */

extern u8_t smsc_print_out;
/*****************************************************************************
* MACRO: SMSC_PLATFORM_MESSAGE
* DESCRIPTION:
*    prints a message to a diagnostic output console.
* PARAMETERS:
*    message, this takes the form of standard printf parameters
*        example is 
*            ("I am %d old",myAge)
* RETURN VALUE
*     None.
*****************************************************************************/
#define SMSC_PLATFORM_MESSAGE(message)	\
	do {		\
		if(smsc_print_out)		\
			smsc_printf message;		\
		} while(0)

/*****************************************************************************
* MACRO: SMSC_PLATFORM_HALT_MESSAGE
* DESCRIPTION:
*     This behaves the same as SMSC_PLATFORM_MESSAGE except that after
*     printing the message the system halts and does not continue program
*     execution. This macro should never return.
* PARAMETERS:
*    message, this takes the form of standard printf parameters
*        example is 
*            ("I am %d old",myAge)
* RETURN VALUE
*     This macro should never return.
*****************************************************************************/
#define SMSC_PLATFORM_HALT_MESSAGE(message)			\
	do {		\
		if(smsc_print_out)		\
			smsc_printf message;	/*abort();*/	\
		} while(0)
/*20081015 del fflush(NULL);abort();		\*/
/*****************************************************************************
* TYPE: smsc_clock_t
* DESCRIPTION:
*    This type is used for storing time values that can used in
*    smsc_time_now, smsc_time_plus, smsc_time_minus, smsc_time_after,
*    smsc_tick_to_sec, smsc_sec_to_tick
*    smsc_tick_to_msec, smsc_msec_to_tick
*    smsc_tick_to_usec, smsc_usec_to_tick
*****************************************************************************/
typedef CHDRV_NET_CLOCK_t smsc_clock_t;
/*****************************************************************************
* FUNCTION: smsc_time_now
* RETURN VALUE:
*     returns a smsc_clock_t that represents the present time.
*****************************************************************************/
#define smsc_time_now  CHDRV_NET_TimeNow
/*****************************************************************************
* FUNCTION: smsc_time_plus
* DESCRIPTION:
*    Adds two time values and returns the result. The caller
*    ensures that one of the times is a time offset or a relative time value.
*    It is not legal to add two absolute time values since the origin is not
*    defined by this specification. So this function can be used to add an 
*    absolute time to a relative time, or it can be used to add a relative 
*    time to a relative time.
* PARAMETERS:
*    smsc_clock_t t1, represents one of the time values to add.
*    smsc_clock_t t2, represents one of the time values to add.
* RETURN VALUE:
*    returns a smsc_clock_t that represents the sum of the two time values.
*****************************************************************************/
/*#define linux_time_plus(t1,t2)  ((t2) + (t1))*/
#define linux_time_plus(t1,t2)  CHDRV_NET_TimePlus((t1),(t2))
#define smsc_time_plus(t1,t2)	((smsc_clock_t)linux_time_plus((smsc_clock_t)(t1),(smsc_clock_t)(t2)))

/*****************************************************************************
* FUNCTION: smsc_time_minus
* DESCRIPTION:
*    Subtracts one time value from another time value. This is usually used
*    to figure a relative time value from two absolute time values.
* PARAMETERS:
*    smsc_clock_t t1, represents t1 of (t1-t2)
*    smsc_clock_t t2, represents t2 of (t1-t2)
* RETURN VALUE:
*    returns a smsc_clock_t that represents (t1-t2). Or in other words it
*       returns a relative time whose origin is t1.
*****************************************************************************/
/*#define linux_time_minus(t1,t2) ((t1) - (t2))*/
#define linux_time_minus(t1,t2) CHDRV_NET_TimeMinus(t1,t2)
#define smsc_time_minus(t1,t2)	((smsc_clock_t)linux_time_minus((smsc_clock_t)(t1),(smsc_clock_t)(t2)))

/*****************************************************************************
* FUNCTION: smsc_time_after
* DESCRIPTION:
*    Determine the relative time order between two time values. The two time
*    values can be assumed to be based around the same origin.
* PARAMETERS:
*    smsc_clock_t t1, represents a time value
*    smsc_clock_t t2, represents a time value
* RETURN VALUE:
*    returns 1 if t1 is after t2.
*    returns 0 if t1 is before or equal to t2
*****************************************************************************/
/*#define linux_time_after(t1,t2) ((long)(t2) - (long)(t1) < 0)*/
#define linux_time_after(t1,t2) CHDRV_NET_TimeAfter(t1,t2)
#define smsc_time_after(t1,t2)	((smsc_clock_t)linux_time_after((smsc_clock_t)(t1),(smsc_clock_t)(t2)))

/*****************************************************************************
* The following functions/macros are used to translate the units of 
* smsc_clock_t into or from the units of seconds, milliseconds, and microseconds
* 
* smsc_tick_to_sec, translates a smsc_clock_t into the number of seconds.
* smsc_sec_to_tick, translates a number of seconds into a smsc_clock_t
* smsc_tick_to_msec, translates a smsc_clock_t into the number of milliseconds.
* smsc_msec_to_tick, translates a number of milliseconds into a smsc_clock_t
* smsc_tick_to_usec, translates a smsc_clock_t into the number of microseconds.
* smsc_usec_to_tick, translates a number of microseconds into a smsc_clock_t
*****************************************************************************/
#define smsc_tick_to_sec(tick)  ((u32_t)((((u32_t)(tick))) / ((u32_t)HZ)))
#define smsc_sec_to_tick(sec)   ((smsc_clock_t)((sec) * HZ))
#define smsc_tick_to_msec(tick) ((u32_t)((((u32_t)(tick)) * ((u32_t)1000)) / ((u32_t)HZ)))
#define smsc_msec_to_tick(msec) ((smsc_clock_t)((msec) * HZ / 1000))
#define smsc_tick_to_usec(tick) ((u32_t)((tick) * 1000000 / HZ))
#define smsc_usec_to_tick(usec) ((smsc_clock_t)((usec) * HZ / 1000000))

/*****************************************************************************
* The following delay macros are used to wait a specified time period.
* It is not required that they release the CPU to other threads. They may 
* just spin until the specified time has expired. However they may be
* implemented to release the CPU to other threads if that seems useful.
*
* smsc_mdelay, waits a specified number of milliseconds
* smsc_udelay, waits a specified number of microseconds
*****************************************************************************/
#define usleep CHDRV_NET_ThreadDelay
#define time_ticks_per_sec() CHDRV_NET_TicksOfSecond()
/* OS20 time conversion macros */
#define tick_to_msec(tick)	((u32_t)((tick) * 1000 / time_ticks_per_sec())) 
#define msec_to_tick(msec)	((clock_t)((msec) * time_ticks_per_sec() / 1000))
#define tick_to_usec(tick)		((u32_t)((tick) * 1000000 / time_ticks_per_sec())) 
#define usec_to_tick(usec)	((clock_t)((usec) * time_ticks_per_sec() / 1000000))

/* delay macros */
#define smsc_mdelay(msec)	CHDRV_NET_ThreadDelay(msec)
#define smsc_udelay(usec)	CHDRV_NET_ThreadDelay(usec/1000)

#define SMSC_TIMEOUT	CHDRV_NET_TIMEOUT


/* These memory alignment macros are already platform independent */
#define MEMORY_ALIGNED_SIZE(size) \
((size+(MEMORY_ALIGNMENT - 1)) & (~(MEMORY_ALIGNMENT-1)))

#define MEMORY_ALIGN(addr) ((void *)(((mem_ptr_t)(addr) + (mem_ptr_t)(MEMORY_ALIGNMENT - 1)) & ~((mem_ptr_t)(MEMORY_ALIGNMENT-1))))

/* The following debug utilities are already platform independent */

/* SMSC_TRACE is used for informational purposes only,
	it may be useful during debugging but is usually too
	noisy to keep it enabled all the time.
	'condition' is usually filled in with an individual module 
	debug enable switch, but it could also be used to specify a runtime
	condition that activates the message. */
#if (SMSC_TRACE_ENABLED)
#define SMSC_TRACE(condition,message) 				\
	do {											\
		if(condition) {								\
			SMSC_PLATFORM_MESSAGE(("  TRACE: "));	\
			SMSC_PLATFORM_MESSAGE(message); 		\
			SMSC_PLATFORM_MESSAGE(("\n"));			\
		}											\
	} while(0)
#define SMSC_PRINT(condition,message)			\
	do {										\
		if(condition) {							\
			SMSC_PLATFORM_MESSAGE(message);		\
		}										\
	} while(0)
#else
#define SMSC_TRACE(condition,message)	{}
#define SMSC_PRINT(condition,message)		{}
#endif

/* SMSC_NOTICE is intended to be used only when an error is detected
   and fully handled and recoverable. (example is dropping packets) 
	'condition' is usually filled in with an individual module 
	debug enable switch, but it could also be used to specify a runtime
	condition that activates the message. */
#if (SMSC_NOTICE_ENABLED)
#define SMSC_NOTICE(condition,message) 				\
	do {											\
		if(condition) {								\
			SMSC_PLATFORM_MESSAGE((" NOTICE: "));	\
			SMSC_PLATFORM_MESSAGE(message);			\
			SMSC_PLATFORM_MESSAGE(("\n"));			\
		}											\
	} while(0)
#else
#define SMSC_NOTICE(condition,message)
#endif

/* SMSC_WARNING is intended to be used only when an error is detected
   and but may not be handled or recoverable. (example is how to handle unknown codes) 
	'condition' is usually filled in with an individual module 
	debug enable switch, but it could also be used to specify a runtime
	condition that activates the message. */
#if (SMSC_WARNING_ENABLED)
#define SMSC_WARNING(condition,message) 			\
	do {											\
		if(condition) {								\
			SMSC_PLATFORM_MESSAGE(("WARNING: "));	\
			SMSC_PLATFORM_MESSAGE(message);			\
			SMSC_PLATFORM_MESSAGE(("\n"));			\
		}											\
	} while(0)
#else
#define SMSC_WARNING(condition,message)
#endif

/* SMSC_ERROR is to be used only when a fatal unrecoverable error has 
   accured. It contains a halt command to prevent further corruption.
   SMSC_ASSERT is similar to SMSC_ERROR in that both can be used to notify
   of a fatal error. SMSC_ASSERT allow the user to provide a condition
   which if false will trigger the error. If the error is triggered an 
   halt command will prevent further execution.
   Neither SMSC_ERROR or SMSC_ASSERT can be enabled on a per module based.
   If they are enabled they are enabled for all modules.
   */
#if (SMSC_ERROR_ENABLED)
#define SMSC_ERROR_ONLY(x)	x
#define SMSC_ERROR(message) 								\
	do { 													\
		SMSC_PLATFORM_MESSAGE(("!ERROR!: "));				\
		SMSC_PLATFORM_MESSAGE(message);						\
		SMSC_PLATFORM_HALT_MESSAGE(							\
			("\n!ERROR!: file=%s, line=%d\n",				\
				__FILE__,__LINE__));		/*abort();*/				\
	} while(0)

#define SMSC_ASSERT(condition) 								\
	do { 													\
		if(!(condition)) {									\
			SMSC_PLATFORM_HALT_MESSAGE(						\
				("ASSERTION FAILURE: file=%s, line=%d\n", 	\
                	__FILE__, __LINE__));		/*abort();*/	\
		}													\
   	} while(0)
#define SMSC_ASSERT_JUDGE(condition,result) 								\
	do { 													\
		if(!(condition)) {									\
			SMSC_PLATFORM_HALT_MESSAGE(						\
				("ASSERTION FAILURE: file=%s, line=%d\n", 	\
                	__FILE__, __LINE__));		result++;	\
        	}													\
   	} while(0)
/*函数参数是否有效检测,只能用在函数有返回值且返回指定值*/
#define SMSC_FUNCTION_PARAMS_CHECK_RETURN(condition,ret)	do{	\
	s32_t result = 0;	\
	SMSC_ASSERT_JUDGE(condition,result);	\
	if(result)	return ret;	\
}while(0)
/*函数参数是否有效检测,只能用在函数无返回值*/
#define SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(condition)	do{	\
	s32_t result = 0;	\
	SMSC_ASSERT_JUDGE(condition,result);	\
	if(result)	return ;	\
}while(0)

/*
* The following macros help by verifying you have a valid structure pointer
*   This is especially useful when void pointers are used to pass structures
*   around. These macros can be used to make sure a structure has been 
*   initialized and make sure it is the correct structure.
*/
#define DECLARE_SIGNATURE	u32_t mSignature;
#define ASSIGN_SIGNATURE(structure_pointer,signature) 				\
	do {															\
		(structure_pointer)->mSignature=((u32_t)signature);			\
	}while(0)
#define CHECK_SIGNATURE(structure_pointer,signature) 				\
	do {															\
		if((structure_pointer)->mSignature!=((u32_t)signature)) {		\
			SMSC_PLATFORM_HALT_MESSAGE(								\
				("SIGNATURE_FAILURE: file=%s, line=%d\n",			\
					__FILE__, __LINE__));							\
		}															\
	} while(0)
#define CLEAR_SIGNATURE(structure_pointer)			\
	do {											\
		(structure_pointer)->mSignature=(u32_t)0);	\
	} while(0)
#else /*SMSC_ERROR_ENABLED */
#define SMSC_ERROR_ONLY(x) x
#define SMSC_ERROR(message)
#define SMSC_ASSERT(condition)  
#define DECLARE_SIGNATURE
#define ASSIGN_SIGNATURE(structure_pointer,signature)
#define CHECK_SIGNATURE(structure_pointer,signature)
#define CLEAR_SIGNATURE(structure_pointer)
#define SMSC_ASSERT_JUDGE(condition,result) 
#define SMSC_FUNCTION_PARAMS_CHECK_RETURN(condition,ret)
#define SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(condition)
#endif /*SMSC_ERROR_ENABLED */

/*****************************************************************************
* FUNCTION: SMSC_IPCHECKSUM
* DESCRIPTION:
*    This function handles data checksuming according to the IP specification.
* PARAMETERS:
*    void * dataptr, this is a pointer to the buffer that should be checksummed
*    int len, this is the length of the buffer that should be checksummed
* RETURN VALUE:
*    Returns the IP check sum as a u16_t
*****************************************************************************/
#define smsc_chksum_linux(data,len)	CHDRV_NET_Chksum(data,len)
#define SMSC_IPCHECKSUM smsc_chksum_linux

#endif /* ENVIRONMENT_H */
