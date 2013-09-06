/************************************************************************
File Name   : os_20to21_map.h

Description : Contains os20 call wrappers to substitute os21 calls.

Copyright (C) 2004 STMicroelectronics

Revision History:

  06Aug2004   File Created.

************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __OS_20TO21_MAP_H
#define __OS_20TO21_MAP_H

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* OS20-OS21 functions wrapper */

#ifdef ST_OS21
	#define semaphore_create_fifo_timeout(count) semaphore_create_fifo(count)
	#define semaphore_create_priority_timeout(count) semaphore_create_priority(count)
	#define interrupt_install(a,b,c,d) interrupt_install(interrupt_handle(a),(interrupt_handler_t )c,d)
	#define interrupt_install_shared(a,b,c,d) interrupt_install_shared(interrupt_handle(a),(interrupt_handler_t )c,d)
	#define interrupt_uninstall(a,b) interrupt_uninstall(interrupt_handle(a))
	#define interrupt_enable_number(number) interrupt_enable(interrupt_handle(number))
	#define interrupt_disable_number(number) interrupt_disable(interrupt_handle(number))
	#define interrupt_clear_number(number) interrupt_clear(interrupt_handle(number))
	#define interrupt_raise_number(number) interrupt_raise(interrupt_handle(number))
	#define task_flags_high_priority_process  0 /* does not exist for ST40 */
	#define clock_t  osclock_t
#endif

#ifdef ST_OS20
    #define task_flags_no_min_stack_size  0
    #define osclock_t  clock_t
#endif

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __OS_20TO21_MAP_H */

