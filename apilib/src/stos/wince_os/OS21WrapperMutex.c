/*! Time-stamp: <@(#)OS21WrapperMutex.c   8/23/2005 - 5:07:09 PM   William Hennebois>
 *********************************************************************
 *  @file   : OS21WrapperMutex.c
 *
 *  Project : 
 *
 *  Package : 
 *
 *  Company : TeamLog 
 *
 *  Author  : William Hennebois            Date: 8/23/2005
 *
 *  Purpose :  Implementation of methods for mutex
 *
 *********************************************************************
 * Version History:
 *
 * V 0.10  8/23/2005  WH : First Revision
 *
 *********************************************************************
 */

#include "OS21WrapperMutex.h"


/*!-------------------------------------------------------------------------------------
//mutex_create_fifo() creates a mutex. The memory for the mutex structure is
//allocated from the system heap. Mutexes created with this function have the usual
//mutex semantics, except that when a task calls mutex_lock() it is always
// appended to the end of the queue of waiting tasks, irrespective of its priority. *
 * @param count : size of the stack
 *
 * @return mutex_t *  : 
 */
mutex_t * mutex_create_fifo (void)
{

	int iRet;
	mutex_t *pMut;
	WCE_MSG(MDL_OSCALL,"-- mutex_create_fifo()");


	pMut = memory_allocate(NULL,sizeof(mutex_t));
	memset(pMut,0,sizeof(*pMut));
	iRet = WCE_Mutex_Create(pMut);
	if(!iRet)
	{	
		return NULL;
	}
	return pMut;
}

mutex_t * mutex_create_priority(void)
{
	WCE_MSG(MDL_OSCALL,"-- mutex_create_priority()");

	return ( mutex_create_fifo () );
}







/*!-------------------------------------------------------------------------------------
 * mutex_delete() deletes the mutex, mut.
 *
 * @param *mut 
 *
 * @return void mutex_delete  : 
 */
int mutex_delete (mutex_t *pMut)
{
	int iRet;
	WCE_ASSERT(pMut)
	WCE_MSG(MDL_OSCALL,"-- mutex_delete()");

	iRet = WCE_Mutex_Destroy(pMut);
	memory_deallocate(NULL,pMut);
    return (iRet  ? OS21_SUCCESS : OS21_FAILURE);
}




/*!-------------------------------------------------------------------------------------
 * mutex_lock() acquires the given mutex. The exact behavior of this function
 * depends on the mutex type. If the mutex is currently not owned, or is already owned
 * by the task, then the task acquires the mutex, and carries on running. If the mutex
 * is owned by another task, then the calling task is added to the queue of tasks
 * waiting for the mutex, and deschedules.
 * Once the task acquires the mutex it is made immortal, until it releases the mutex.
 *
 * @param mutex : 
 *
 * @return void   : 
 */
void  mutex_lock (mutex_t* mutex)
{
	WCE_ASSERT(mutex)
	WCE_MSG(MDL_OSCALL,"-- mutex_lock()");
	WCE_Mutex_Lock(mutex);

}

/*!-------------------------------------------------------------------------------------
 * mutex_release() releases the specified mutex. The exact behavior of this
 * function depends on the mutex type. The operation checks the queue of tasks
 * waiting for the mutex, if the list is not empty, then the first task on the list is
 * restarted and granted ownership of the mutex, possibly preempting the current
 * task. Otherwise the mutex is released, and the task continues running.
 * If the releasing task had its priority temporarily boosted by the priority inversion
 * logic, then once the mutex is released the tasks priority is returned to its correct
 * value.
 *
 * @param mutex : 
 *
 * @return int   : 
 */
int       mutex_release (mutex_t* mutex)
{
	int iRet=0;
	WCE_ASSERT(mutex)
	WCE_MSG(MDL_OSCALL,"-- mutex_release()");
	iRet = WCE_Mutex_Release(mutex);
    return (iRet  ? OS21_SUCCESS : OS21_FAILURE);
	
}




