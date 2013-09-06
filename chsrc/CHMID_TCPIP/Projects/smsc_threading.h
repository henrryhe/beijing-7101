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
* FILE: smsc_threading.h
* PURPOSE:
*    This file is used to declare a threading environment wrapper to shield the
*    SMSC Network Stack from platform and OS specifics.
*****************************************************************************/

#ifndef SMSC_THREADING_H
#define SMSC_THREADING_H

#include "smsc_environment.h"

#if !SMSC_THREADING_ENABLED
#error SMSC_THREADING_ENABLED=0 threading must be enabled to using threading functions
#endif

/* Semaphore utilities */
/*****************************************************************************
* CONSTANT: SMSC_SEMAPHORE_NULL
* DESCRIPTION:
*    represents a value that indicates an invalid PSMSC_SEMAPHORE
*****************************************************************************/
#define SMSC_SEMAPHORE_NULL  NULL

/*****************************************************************************
* TYPES:
*   SMSC_SEMAPHORE is a platform specific semaphore type
*   PSMSC_SEMAPHORE is a pointer to the SMSC_SEMAPHORE type
*****************************************************************************/
#define SMSC_SEMAPHORE CHDRV_NET_SEMAPHORE 
#define SMSC_THREAD CHDRV_NET_THREAD 

#define SMSC_THREAD_FLAG_SUSPEND	(0x01)

#define smsc_semaphore_initialize(pSemaphore, count)CHDRV_NET_SemaphoreIinitialize(pSemaphore, count)
/*****************************************************************************
* FUNCTION: smsc_semaphore_free
* DESCRIPTION:
*    Frees a semaphore that was created with smsc_semaphore_create
*****************************************************************************/
#define  smsc_semaphore_free(pSemaphore)   CHDRV_NET_SemaphoreFree(pSemaphore)

/*****************************************************************************
* FUNCTION: smsc_semaphore_signal
* DESCRIPTION:
*    Increases the semaphore count in a multithread safe manner. And allow
*    other threads that are waiting the the semaphore to wake up.
*****************************************************************************/
#define smsc_semaphore_signal(pSemaphore) CHDRV_NET_SemaphoreSignal(pSemaphore)

/*****************************************************************************
* FUNCTION: smsc_semaphore_wait
* DESCRIPTION:
*    The following is done in a multithread safe manner.
*    If the semaphore count is greater than zero, then it decreases the count
*        and continues.
*    If the semaphore count is zero then it waits until another thread calls
*        smsc_semaphore_signal to increase the count.
*****************************************************************************/
#define smsc_semaphore_wait(pSemaphore) CHDRV_NET_SemaphoreWait(pSemaphore,-1)
#define smsc_semaphore_wait_timeout( sem, milliSeconds) CHDRV_NET_SemaphoreWait(sem,milliSeconds)

/*****************************************************************************
* FUNCTION: smsc_semaphore_create
* DESCRIPTION:
*    creates a semaphore with a specified count.
*****************************************************************************/
SMSC_SEMAPHORE smsc_semaphore_create(int count);

/* Thread utilities. */
#define SMSC_THREAD_PRIORITY_LOWEST		(0)
#define SMSC_THREAD_PRIORITY_LOW		(4)
#define SMSC_THREAD_PRIORITY_NORMAL		(8)
#define SMSC_THREAD_PRIORITY_HIGH		(12)
#define SMSC_THREAD_PRIORITY_HIGHEST	(15)
#define SMSC_THREAD_NULL 0



/*****************************************************************************
* FUNCTION: smsc_thread_create
* DESCRIPTION:
*    creates a thread
*****************************************************************************/
#define smsc_thread_create(func,arg,stackSize,priority,name,flag)	CHDRV_NET_ThreadCreate(func,arg,stackSize,priority,name,flag)
/*****************************************************************************
* FUNCTION: smsc_thread_resume
* DESCRIPTION:
*    resumes a thread that is in the SUSPEND state.
*****************************************************************************/
#define smsc_thread_resume(threadHandle) CHDRV_NET_ThreadResume(threadHandle)

/*****************************************************************************
* FUNCTION: smsc_thread_suspend
* DESCRIPTION:
*    puts a thread in the suspend state.
*****************************************************************************/
#define smsc_thread_suspend(threadHandle) CHDRV_NET_ThreadSuspend(threadHandle)

/*****************************************************************************
* FUNCTION: smsc_thread_set_priority
* DESCRIPTION:
*    sets the priority of a thread
*****************************************************************************/
#define smsc_thread_set_priority( threadHandle, priority) CHDRV_NET_ThreadSetPriority( threadHandle, priority)

/*****************************************************************************
* FUNCTION: smsc_thread_get_current
* DESCRIPTION:
*    get the current thread.
*****************************************************************************/
#define smsc_thread_get_current()	0 /*((PSMSC_THREAD)(pthread_self()))*/


#endif /* SMSC_THREADING_H */
