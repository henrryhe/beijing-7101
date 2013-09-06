/******************************************************************************/
/*                                                                            */
/* File name   : STAPP_OS.H                                                   */
/*                                                                            */
/* Description : Operating system layer adaptation                            */
/*                                                                            */
/* COPYRIGHT (C) ST-Microelectronics 2005                                     */
/*                                                                            */
/* Date               Modification                 Name                       */
/* ----               ------------                 ----                       */
/* 02/03/05           Created                      M.CHAPPELLIER              */
/*                                                                            */
/******************************************************************************/

#ifndef _STAPP_OS_H_
#define _STAPP_OS_H_

/* C++ support */
/* ----------- */
#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================
   OS20
   ======================================================================== */

#if defined(ST_OS20)
#include <partitio.h>
#include <semaphor.h>
#include <interrup.h>
#include <task.h>
#include <kernel.h>
#include <ostime.h>
#include <message.h>
#include <string.h>
#include <mutex.h>

/* Misc */
/* ---- */
#define Kernel_Version()   kernel_version()

/* Timers */
/* ------ */
#if defined(PROCESSOR_C1)
#define TimerHighGet(_value_) _value_=time_now();
#define TimerLowGet(_value_)  _value_=time_now();
#define TimerHighSet(_value_)
#define TimerLowSet(_value_) 
#else
#define TimerHighGet(_value_) __optasm { ldc 0; ldclock; st _value_; }
#define TimerLowGet(_value_)  __optasm { ldc 1; ldclock; st _value_; }
#define TimerHighSet(_value_) __optasm { ld _value_; ldc 0; stclock; }
#define TimerLowSet(_value_)  __optasm { ld _value_; ldc 1; stclock; }
#endif
#define Clock_t                    clock_t
#define Time_Now()                 time_now()
#define Time_Plus(_time1_,_time2_) time_plus(_time1_,_time2_)

/* Task creation */
/* ------------- */
#define TASK_DEF  void
#define TASK_EXIT
#define Task_t                                                  task_t
#define Task_Delay(x)                                           task_delay(x)
#define Task_Create(_func_,_param_,_stack_,_pri_,_name_,_flag_) task_create(_func_,_param_,_stack_,_pri_,_name_,_flag_)
#define Task_Lock()                                             task_lock()
#define Task_Unlock()                                           task_unlock()
#define Task_Delete(_task_id_)                                  task_delete(_task_id_)
#define Task_Priority_Set(_task_id_,_pri_)                      task_priority_set(_task_id_,_pri_)
#define Task_Wait(_task_id_)                                    task_wait(&_task_id_,1,TIMEOUT_INFINITY);
#define Task_Name(_task_id_)                                    task_name(_task_id_)
#define Task_Id()                                               task_id()
#define Task_Context                                            task_context
#define Task_Context_Interrupt                                  task_context_interrupt


/* Semaphores */
/* ---------- */
#define Semaphore_t                                  semaphore_t
#define Semaphore_Init_Fifo(_sem_,_value_)           semaphore_init_fifo                 (&_sem_,_value_)
#define Semaphore_Init_Fifo_TimeOut(_sem_,_value_)   semaphore_init_fifo_timeout         (&_sem_,_value_)
#define Semaphore_Create_Fifo(_sem_,_value_)         _sem_=semaphore_create_fifo         (_value_)
#define Semaphore_Create_Fifo_TimeOut(_sem_,_value_) _sem_=semaphore_create_fifo_timeout (_value_)
#define Semaphore_Delete(_sem_)                      semaphore_delete                    (&_sem_)
#define Semaphore_Wait(_sem_)                        semaphore_wait                      (&_sem_)
#define Semaphore_Wait_Timeout(_sem_,_time_)         semaphore_wait_timeout              (&_sem_,_time_)
#define Semaphore_Signal(_sem_)                      semaphore_signal                    (&_sem_)

/* Mutex */
/* ----- */
#define Mutex_t                                      mutex_t
#define Mutex_Init_Fifo(_mutex_)                     mutex_init_fifo                     (&_mutex_)
#define Mutex_Create_Fifo(_mutex_,_value_)           _mutex_=mutex_create_fifo           ()
#define Mutex_Delete(_mutex_)                        mutex_delete                        (&_mutex_)
#define Mutex_Lock(_mutex_)                          mutex_lock                          (&_mutex_)
#define Mutex_Unlock(_mutex_)                        mutex_release                       (&_mutex_)

/* Memory partitions */
/* ----------------- */
#define Partition_t                                         partition_t
#define Partition_Init_Simple(_partition_,_memory_, _size_) partition_init_simple (&_partition_,_memory_,_size_)
#define Partition_Init_Heap(_partition_,_memory_,_size_)    partition_init_heap   (&_partition_,_memory_,_size_)
#define Memory_Allocate(_partition_,_size_)                 memory_allocate(_partition_,_size_)
#define Memory_Deallocate(_partition_,_ptr_)                memory_deallocate(_partition_,_ptr_)
#define Memory_Reallocate(_partition_,_ptr_,_size_)         memory_reallocate(_partition_,_ptr_,_size_)
#define Memory_Allocate_Clear(_partition_,_nmemb_,_size_)   memory_allocate_clear(_partition_,_nmemb_,_size_)

/* Interrupts */
/* ---------- */
#define IT_DEF  void
#define IT_EXIT
#define Interrupt_Install(_number_,_level_,_handler_, _param_) interrupt_install   (_number_,_level_,_handler_,_param_)
#define Interrupt_Uninstall(_number_,_level_)                  interrupt_uninstall (_number_,_level_)
#define Interrupt_Enable(_number_,_level_)                     interrupt_enable_number(_number_)
#define Interrupt_Disable(_number_,_level_)                    interrupt_disable_number(_number_)
#define Interrupt_Lock()                                       interrupt_lock()
#define Interrupt_Unlock()                                     interrupt_unlock()
#endif

/* ========================================================================
   OS21 
   ======================================================================== */

#if defined(ST_OS21)
#include "os21/partition.h"
#include "os21/semaphore.h"
#include "os21/mutex.h"
#include "os21/kernel.h"
#include "os21/task.h"
#include "os21/ostime.h"
#include "os21/message.h"
#include "os21/event.h"
#include "os21/interrupt.h"
#include "os21/mutex.h"

#define OS_SUCCESS  OS21_SUCCESS /*yxl 2007-01-11 add*/
/* Misc */
/* ---- */
#define Kernel_Version()   kernel_version()

/* Timers */
/* ------ */
#define TimerHighGet(_value_)      _value_=time_now()
#define TimerLowGet(_value_)       _value_=time_now()
#define TimerHighSet(_value_)
#define TimerLowSet(_value_)
#define Clock_t                    osclock_t
#define Time_Now()                 time_now()
#define Time_Plus(_time1_,_time2_) time_plus(_time1_,_time2_)

/* Task creation */
/* ------------- */
#define TASK_DEF  void
#define TASK_EXIT
#define Task_t                                                  task_t
#define Task_Delay(x)                                           task_delay(x)
#if 1
#define Task_Create(_func_,_param_,_stack_,_pri_,_name_,_flag_) task_create(_func_,_param_,((_stack_)>16384?(_stack_):(_stack_)+16384),/*20070710 change for test*/(/*MAX_USER_PRIORITY-15+*/_pri_-3),_name_,_flag_)
#else
#define Task_Create(_func_,_param_,_stack_,_pri_,_name_,_flag_) task_create(_func_,_param_,_stack_,_pri_,_name_,(_flag_)|task_flags_no_min_stack_size)
#endif
#define Task_Lock()                                             task_lock()
#define Task_Unlock()                                           task_unlock()
#define Task_Delete(_task_id_)                                  task_delete(_task_id_)
#define Task_Priority_Set(_task_id_,_pri_)                      task_priority_set(_task_id_,_pri_)
#define Task_Wait(_task_id_)                                    task_wait(&_task_id_,1,TIMEOUT_INFINITY);
#define Task_Name(_task_id_)                                    task_name(_task_id_)
#define Task_Id()                                               task_id()
#define Task_Context                                            task_context
#define Task_Context_Interrupt                                  task_context_interrupt
#define Task_resume(_task_id_)                                  task_resume(_task_id_) /*yxl 2007-01-11 add*/

/* Semaphores */
/* ---------- */
#define Semaphore_t                                   semaphore_t *
#define Semaphore_Init_Fifo(_sem_,_value_)            _sem_=semaphore_create_fifo         (_value_)
#define Semaphore_Init_Fifo_TimeOut(_sem_,_value_)    _sem_=semaphore_create_fifo         (_value_)
#define Semaphore_Create_Fifo(_sem_,_value_)          _sem_=semaphore_create_fifo         (_value_)
#define Semaphore_Create_Fifo_TimeOut(_sem_,_value_)  _sem_=semaphore_create_fifo_timeout (_value_)
#define Semaphore_Delete(_sem_)                       semaphore_delete                    (_sem_)
#define Semaphore_Wait_Timeout(_sem_,_time_)          semaphore_wait_timeout              (_sem_,_time_)
#define Semaphore_Signal(_sem_)                       semaphore_signal                    (_sem_)
#define Semaphore_Wait(_sem_)                         semaphore_wait                      (_sem_)

/* Mutex */
/* ----- */
#define Mutex_t                                      mutex_t *
#define Mutex_Init_Fifo(_mutex_)                     _mutex_=mutex_create_fifo           ()
#define Mutex_Create_Fifo(_mutex_,_value_)           _mutex_=mutex_create_fifo           ()
#define Mutex_Delete(_mutex_)                        mutex_delete                        (_mutex_)
#define Mutex_Lock(_mutex_)                          mutex_lock                          (_mutex_)
#define Mutex_Unlock(_mutex_)                        mutex_release                       (_mutex_)

/* Memory partitions */
/* ----------------- */
#define Partition_t                                        partition_t *
#define Partition_Init_Simple(_partition_,_memory_,_size_) _partition_=partition_create_simple (_memory_,_size_)
#define Partition_Init_Heap(_partition_,_memory_,_size_)   _partition_=partition_create_heap   (_memory_,_size_)
#define Memory_Allocate(_partition_,_size_)                memory_allocate(_partition_,_size_)
#define Memory_Deallocate(_partition_,_ptr_)               memory_deallocate(_partition_,_ptr_)
#define Memory_Reallocate(_partition_,_ptr_,_size_)        memory_reallocate(_partition_,_ptr_,_size_)
#define Memory_Allocate_Clear(_partition_,_nmemb_,_size_)  memory_allocate_clear(_partition_,_nmemb_,_size_)

/* Interrupts */
/* ---------- */
#define IT_DEF  int
#define IT_EXIT return(OS21_SUCCESS);
#define Interrupt_Install(_number_,_level_,_handler_,_param_) interrupt_install(interrupt_handle(_number_),(int(*)(void*))_handler_, _param_)
#define Interrupt_Uninstall(_number_,_level_)                 interrupt_uninstall(interrupt_handle(_number_))
#define Interrupt_Enable(_number_,_level_)                    interrupt_enable(interrupt_handle(_number_))
#define Interrupt_Disable(_number_,_level_)                   interrupt_disable(interrupt_handle(_number_))
#define Interrupt_Lock()                                      interrupt_lock()
#define Interrupt_Unlock()                                    interrupt_unlock()
#endif

/* C++ support */
/* ----------- */
#ifdef __cplusplus
}
#endif
#endif

