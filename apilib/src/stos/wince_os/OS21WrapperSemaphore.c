/*! Time-stamp: <@(#)OS21WrapperSemaphore.c   06/04/2005 - 10:20:41   William Hennebois>
 *********************************************************************
 *  @file   : OS21WrapperSemaphore.c
 *
 *  Project : STAPI Windows CE
 *
 *  Package : 
 *
 *  Company :  TeamLog for ST
 *
 *  Author  : William Hennebois            Date: 06/04/2005
 *
 *  Purpose : Implementation of methods to manage FIFO and Semaphores
 *
 *********************************************************************
 * Version History:
 *
 * V 0.10  06/04/2005 WH : First Revision
 * V 0.10  12/04/2005 WH : Re-implementation for direct use of Windows API

 *
 *********************************************************************
 */
#include "OS21WrapperSemaphore.h"


/*!-------------------------------------------------------------------------------------
//! semaphore_create_fifo() creates a counting semaphore, initialized to value.
//! The memory for the semaphore structure is allocated from the system heap.
//! Semaphores created with this function have the usual semaphore semantics, except
//! that when a task calls semaphore_wait() it is always appended to the end of the
//! queue of waiting tasks, irrespective of its priority.
 *
 * @param count : size of the stack
 *
 * @return semaphore_t *  : 
 */
semaphore_t * semaphore_create_fifo(int count)
{
	int iRet;
	semaphore_t *pSem;
	WCE_MSG(MDL_OSCALL,"-- semaphore_create_fifo(%d)",count);


	pSem = memory_allocate(NULL,sizeof(semaphore_t));
	memset(pSem,0,sizeof(semaphore_t));

	iRet = WCE_Semaphore_Create(pSem,count,FALSE);
	if(!iRet)
	{	
		return NULL;
	}
	return pSem;
//    return( semaphore_create_fifo_timeout(count) );
}



/*!-------------------------------------------------------------------------------------
//! semaphore_create_fifo() creates a counting semaphore, initialized to value.
//! The memory for the semaphore structure is allocated from the system heap.
//! Semaphores created with this function have the usual semaphore semantics, except
//! that when a task calls semaphore_wait() it is always appended to the end of the
//! queue of waiting tasks, irrespective of its priority.
 *
 * @param count : size of the stack
 *
 * @return semaphore_t *  : 
 */
semaphore_t * semaphore_create_fifo_p(partition_t * pp,int count)
{
	WCE_MSG(MDL_OSCALL,"-- semaphore_create_fifo_p(%d)",count);

    return( semaphore_create_fifo(count) );
//    return( semaphore_create_fifo_timeout(count) );
}



/*!-------------------------------------------------------------------------------------
 * Create a FIFO semapore, it is a emulation of Wait for Multi Object,
 * I prefere to use this emulation  for instance ( comes from linux version)
 *
 * @param *sem : 
 * @param count : 
 *
 * @return int  : 
 */
int semaphore_init_fifo(semaphore_t *sem, int count)
{
	int	ret;

	WCE_MSG(MDL_OSCALL,"-- semaphore_init_fifo(%d)",count);

	ret = WCE_Semaphore_Create(sem,count, FALSE);
    if (ret)
	{
		if (count == 0)
		{
			WCE_Semaphore_Lock(sem);
		}
		else
		{
	        if (count <= 1)
	        {
	            WCE_Semaphore_Unlock(sem);
                WCE_Yield();
            }
            else
            {
                ret = 0xFF;
            }
		}
	}
	else
	{
		WCE_MSG(MDL_MSG,"semaphore_init_fifo creation problem 0x%x (%d)!!!!!\n\n", sem, ret);
	}

    return (ret  ? OS21_SUCCESS : OS21_FAILURE);
}


/*!-------------------------------------------------------------------------------------
 * semaphore_init_fifo_timeout
 *
 * @param *sem : 
 * @param count : 
 *
 * @return int  : 
 */
int semaphore_init_fifo_timeout(semaphore_t *sem, int count)
{
	int	ret;

	WCE_MSG(MDL_OSCALL,"-- semaphore_init_fifo_timeout(%d)",count);

	ret = WCE_Semaphore_Create(sem,count, TRUE);
    if (ret)
	{
		if (count == 0)
		{
			WCE_Semaphore_Lock(sem);
		}
		else
		{
	        if (count <= 1)
	        {
	            WCE_Semaphore_Unlock(sem);
                WCE_Yield();
            }
            else
            {
                ret = 0xFF;
            }
		}
	}
	else
	{
		WCE_MSG(MDL_MSG,"semaphore_init_fifo_timeout creation problem 0x%x (%d)!!!!!\n\n", sem, ret);
	}

    return (ret  ? OS21_SUCCESS : OS21_FAILURE);
//	return semaphore_init_fifo(sem,count);
}



/*!-------------------------------------------------------------------------------------
 * semaphore_init_priority
 *
 * The next function should ensure that sem_wait will place the thread at top priority
 * but is not the case in Linux. Putting it in kernel with waiting task ??? 
 *
 * @param *sem : 
 * @param count : 
 *
 * @return int  : 
 */
int semaphore_init_priority(semaphore_t *sem, int count)
{
	WCE_MSG(MDL_OSCALL,"-- semaphore_init_priority(%d)",count);

	/* Not managed yet, emulate it !!! */
	return semaphore_init_fifo(sem, count) ;
}


/*!-------------------------------------------------------------------------------------
 * semaphore_create_fifo_timeout
 *
 * @param count : number of levels for the first in first out
 *
 * @return semaphore_t *  : a pointer on semaphore_t 
 */
semaphore_t * semaphore_create_fifo_timeout(int count)
{
	int iRet;
	semaphore_t *pSem;
	WCE_MSG(MDL_OSCALL,"-- semaphore_create_fifo_timeout(%d)",count);


	pSem = memory_allocate(NULL,sizeof(semaphore_t));
	memset(pSem,0,sizeof(semaphore_t));

	iRet = WCE_Semaphore_Create(pSem,count,TRUE);
	if(!iRet)
	{	
		return NULL;
	}
	return pSem;
}




/*!-------------------------------------------------------------------------------------
 *  semaphore_create_priority_timeout
 *
 * The next function should ensure that sem_wait will place the thread at top priority
 * but is not the case in Linux. Putting it in kernel with waiting task ??? 

 *
 * @param count : 
 *
 * @return semaphore_t *  : 
 */
semaphore_t * semaphore_create_priority_timeout(int count)
{
	WCE_MSG(MDL_OSCALL,"-- semaphore_create_priority_timeout(%d)",count);

	/* Not managed yet, emulate it !!! */
	return ( semaphore_create_fifo_timeout(count) );
}

/*!-------------------------------------------------------------------------------------
 *  semaphore_create_priority
 *
 * The next function should ensure that sem_wait will place the thread at top priority
 * but is not the case in Linux. Putting it in kernel with waiting task ??? 

 *
 * @param count : 
 *
 * @return semaphore_t *  : 
 */
semaphore_t * semaphore_create_priority(int count)
{
	WCE_MSG(MDL_OSCALL,"-- semaphore_create_priority_timeout(%d)",count);

	/* Not managed yet, emulate it !!! */
	return ( semaphore_create_fifo(count) );
}



/*
 * semaphore_delete
 */

/*!-------------------------------------------------------------------------------------
 * semaphore_delete() deletes the semaphore, sem.
 *
 * @param *sem : 
 *
 * @return void semaphore_delete  : 
 */
int semaphore_delete (semaphore_t *pSem)
{
	int iRet;
	WCE_ASSERT(pSem)
	WCE_MSG(MDL_OSCALL,"-- semaphore_delete()");

	iRet = WCE_Semaphore_Destroy(pSem);
	memory_deallocate(NULL,pSem);
    return (iRet  ? OS21_SUCCESS : OS21_FAILURE);
}


/*
 * semaphore_wait
 */
int semaphore_wait(semaphore_t *sem)
{
	WCE_MSG(MDL_OSCALL,"-- semaphore_wait()");
    return (WCE_Semaphore_Lock(sem)? OS21_SUCCESS : OS21_FAILURE);
}

/*!-------------------------------------------------------------------------------------
 * 
 * semaphore_wait_timeout() performs a wait operation on the specified
 * semaphore (sem). If the time specified by the timeout is reached before a signal
 * operation is performed on the semaphore, then semaphore_wait_timeout()
 * returns the value OS21_FAILURE indicating that a timeout occurred, and the
 * semaphore count is unchanged. If the semaphore is signalled before the timeout is
 * reached, then semaphore_wait_timeout() returns OS21_SUCCESS.
 * Note: Timeout is an absolute not a relative value, so if a relative timeout is required this
 * needs to be made explicit, as shown in the example below.
 * The timeout value may be specified in ticks, which is an implementation
 * dependent quantity. Two special time values may also be specified for timeout.
 * TIMEOUT_IMMEDIATE causes the semaphore to be polled, that is, the function
 * always returns immediately. This must be the value used if
 * semaphore_wait_timeout() is called from an interrupt service routine. If the
 * semaphore count is greater than zero, then it is successfully decremented, and the
 * function returns OS21_SUCCESS, otherwise the function returns a value of
 * OS21_FAILURE. A timeout of TIMEOUT_INFINITY behaves exactly as
 * semaphore_wait().
 *
 * @param *sem : 
 * @param timeout_p : 
 *
 * @return int  : 
 */
int semaphore_wait_timeout(semaphore_t *sem, osclock_t * timeout_p)
{
    int    ret = -1; /* Timeout by default */
    DWORD dwTicks;

    WCE_MSG(MDL_OSCALL,"-- semaphore_wait_timeout()");

    if (timeout_p == TIMEOUT_IMMEDIATE)
        dwTicks = 0;
    else if (timeout_p == TIMEOUT_INFINITY)
        dwTicks = INFINITE;
    else
    {
        osclock_t now = time_now();
        WCE_VERIFY(timeout_p != NULL);

        if (time_after(*timeout_p, now)) //timeout not yet reached
           dwTicks = (DWORD)_WCE_Tick2Millisecond(time_minus(*timeout_p, now));
        else
		   dwTicks = 0;
    }

    ret = WCE_Semaphore_TimedLock(sem, dwTicks);

    if (ret != T_SIGNALED && ret != T_ETIMEDOUT)
    {
        WCE_TRACE("semaphore_wait_timeout: pb:%d !!!!\n", ret);
    }
    return (ret == T_SIGNALED ? OS21_SUCCESS : OS21_FAILURE);
}


/*!-------------------------------------------------------------------------------------
 * Signal the semaphore 
 *
 * @param *sem : 
 *
 * @return int  : 
 */
int semaphore_signal(semaphore_t *sem)
{

    int mut_ret;
   	WCE_MSG(MDL_OSCALL,"-- semaphore_signal()");

    mut_ret = WCE_Semaphore_Unlock(sem) ? OS21_SUCCESS : OS21_FAILURE;
    return (mut_ret);
}

/*!-------------------------------------------------------------------------------------
 *  semaphore_create_priority_p
 *
 * The next function should ensure that sem_wait will place the thread at top priority
 * but is not the case in Linux. Putting it in kernel with waiting task ??? 

 *
 * @param count : 
 *
 * @return semaphore_t *  : 
 */
semaphore_t * semaphore_create_priority_p(partition_t * pp, int count)
{
	WCE_MSG(MDL_OSCALL,"-- semaphore_create_priority_p(%d)",count);

	/* Not managed yet, emulate it !!! */
	return ( semaphore_create_fifo_timeout(count) );
}








/*!-------------------------------------------------------------------------------------
 * semaphore_value() returns the current value of the given semaphores count.
 * The value returned may be out of date if the calling task is preempted by another
 * task or ISR which modifies the semaphore.
 *
 * @param sem : 
 *
 * @return  : 
 */
int	semaphore_value(semaphore_t* sem)
{
	
	int count = 0;
	WCE_MSG(MDL_OSCALL,"-- semaphore_value(%d)",sem);
	WCE_ASSERT(sem);
	task_lock();
	count =  WCE_Semaphore_Count(sem);
	task_unlock();
	return count;

}

