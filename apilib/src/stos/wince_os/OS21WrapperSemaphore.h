/*! Time-stamp: <@(#)OS21WrapperSemaphore.h   06/04/2005 - 10:22:23   William hennebois>
 *********************************************************************
 *  @file   : OS21WrapperSemaphore.h
 *
 *  Project : STAPI Windows CE
 *
 *  Package : 
 *
 *  Company :  TeamLog for ST
 *
 *  Author  : William hennebois            Date: 06/04/2005
 *
 *  Purpose : Implementation of methods to manage FIFO and Semaphores
 *
 *********************************************************************
 * Version History:
 *
 * V 0.10  06/04/2005 WH : First Revision
 *
 *********************************************************************
 */
#ifndef __OS21WrapperSemaphore__
#define __OS21WrapperSemaphore__

#define OS21_DEF_MIN_STACK_SIZE		(16 * 1024)

#define OBJECT_TYPE_LIST			 OBJECT_TYPE_LIST_WINCE
#define ST40_PHYS_MASK		(0x1FFFFFFF)
#define ST40_PHYS_ADDR(a)       (((unsigned int)(a) & ST40_PHYS_MASK))


////////////////////////////////////////////////////////
///////////////                 ////////////////////////
//////////////// Types		    ////////////////////////
///////////////                 ////////////////////////
////////////////////////////////////////////////////////


////////////////////////////////////////////////////////
///////////////                 ////////////////////////
//////////////// Struct		    ////////////////////////
///////////////                 ////////////////////////
////////////////////////////////////////////////////////

////////////////////////////////////////////////////////
///////////////                      ///////////////////
//////////////// Semaphore functions ///////////////////
///////////////                      ///////////////////
////////////////////////////////////////////////////////


//! semaphore_create_fifo() creates a counting semaphore, initialized to value.
//! The memory for the semaphore structure is allocated from the system heap.
//! Semaphores created with this function have the usual semaphore semantics, except
//! that when a task calls semaphore_wait() it is always appended to the end of the
//! queue of waiting tasks, irrespective of its priority.

semaphore_t * semaphore_create_fifo(int count);
semaphore_t * semaphore_create_fifo_p(partition_t * pp,int count);


//! semaphore_delete() deletes the semaphore, sem.
int semaphore_delete (semaphore_t* Semaphore);


int semaphore_init_fifo(semaphore_t *sem, int count);
int semaphore_init_fifo_timeout(semaphore_t *sem, int count);
int semaphore_init_priority(semaphore_t *sem, int count);

int semaphore_wait(semaphore_t *sem);
int semaphore_wait_timeout(semaphore_t *sem, osclock_t *timeout);
int semaphore_signal(semaphore_t *sem);

int semaphore_init_priority(semaphore_t *sem, int count);


semaphore_t * semaphore_create_priority_p(partition_t * pp, int count);


semaphore_t * semaphore_create_fifo_timeout(int count);
semaphore_t * semaphore_create_priority_timeout(int count);
semaphore_t * semaphore_create_priority(int count);
int			  semaphore_value(semaphore_t* sem);





#endif // __OS21WrapperSemaphore__