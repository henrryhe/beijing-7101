
/******************************************************************************\
 *
 * File Name    : fcbuffer.c
 *
 * Description  : Implementation of a circular buffer with item of fixed length
 *
 * Copyright STMicroelectronics - Dicember 1998
 *
 * Author       : Alessandro Innocenti
 *
\******************************************************************************/

/* Includes ----------------------------------------------------------------- */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <ostime.h>
#include <semaphor.h>

#include <stddefs.h>

#include <common.h>
#include "fcbuffer.h"


/* Functions ---------------------------------------------------------------- */


/******************************************************************************\
 * Function: fcBuffer_Create
 * Purpose : 
 *     A buffer is a place to store data. It may also be used as a way to
 *     exchange data between tasks
 * Parameters:
 *   - MemoryPartition : partition from which memory is allocated.
 *   - MaxNumItem : maximum number of item that can be hold in the 
 *     circular buffer. 
 *   - ItemSize : size of the items into the circular buffer. All items have
 *     the same size
 * Note:
\******************************************************************************/

fcBuffer_t * fcBuffer_Create (ST_Partition_t *MemoryPartition,
                              U32 MaxNumItem, U32 ItemSize)
{
    fcBuffer_t *fcbuffer_p;

    /* Alloc memory for circular buffer and initialize structure  */

    if ((fcbuffer_p = (fcBuffer_t*)memory_allocate(MemoryPartition, 
                                                   sizeof(fcBuffer_t))) == NULL)
    {
        return(NULL);  
    }

    fcbuffer_p->MemoryPartition = MemoryPartition;
    fcbuffer_p->MaxNumItem = MaxNumItem;
    fcbuffer_p->ItemSize = ItemSize;

    /* Create semaphores */

    fcbuffer_p->SpaceAvailable_p = memory_allocate (MemoryPartition, 
                                                    sizeof(semaphore_t));
    if (fcbuffer_p->SpaceAvailable_p == NULL)
    {     
        /* free memory previosly allocated*/
        memory_deallocate (MemoryPartition, fcbuffer_p);  
        return (NULL); 
    }

    semaphore_init_fifo(fcbuffer_p->SpaceAvailable_p, MaxNumItem);

    fcbuffer_p->DataAvailable_p = memory_allocate (MemoryPartition, 
                                                   sizeof(semaphore_t));
    if (fcbuffer_p->DataAvailable_p == NULL)
    {
        /* free memory previosly allocated*/
        semaphore_delete (fcbuffer_p->SpaceAvailable_p);
        memory_deallocate (MemoryPartition, fcbuffer_p->SpaceAvailable_p);
        memory_deallocate (MemoryPartition, fcbuffer_p);
        return(NULL);
    }
    semaphore_init_fifo_timeout(fcbuffer_p->DataAvailable_p, 0);

    fcbuffer_p->Mutex_p = memory_allocate (MemoryPartition, 
                                           sizeof(semaphore_t));
    if (fcbuffer_p->Mutex_p == NULL)
    {
        /* free memory previosly allocated*/
        semaphore_delete (fcbuffer_p->SpaceAvailable_p);        
        memory_deallocate (MemoryPartition, fcbuffer_p->SpaceAvailable_p);
        semaphore_delete (fcbuffer_p->DataAvailable_p);
        memory_deallocate (MemoryPartition, fcbuffer_p->DataAvailable_p);
        memory_deallocate (MemoryPartition, fcbuffer_p);
        return(NULL); 
    }
    semaphore_init_fifo(fcbuffer_p->Mutex_p, 1);

    /* Alloc memory buffer for data */
    fcbuffer_p->Buffer_p = memory_allocate (MemoryPartition, 
                                            ItemSize * MaxNumItem);   
    if (fcbuffer_p->Buffer_p == NULL)
    {
        /* if memory is not available, free memory previosly allocated */
        semaphore_delete (fcbuffer_p->SpaceAvailable_p);        
        memory_deallocate (MemoryPartition, fcbuffer_p->SpaceAvailable_p);
        semaphore_delete (fcbuffer_p->DataAvailable_p);        
        memory_deallocate (MemoryPartition, fcbuffer_p->DataAvailable_p);
        semaphore_delete (fcbuffer_p->Mutex_p);                
        memory_deallocate (MemoryPartition, fcbuffer_p->Mutex_p);                     
        memory_deallocate (MemoryPartition, fcbuffer_p);    
        return(NULL);   
    }

   /* Items are inserted into the Tail position and removed from the Head. */

    fcbuffer_p->Head = 0;
    fcbuffer_p->Tail = 0;

    fcbuffer_p->NumItemStored = 0;
    fcbuffer_p->Datainvalidate = FALSE;

    return (fcbuffer_p);                                           
}


/******************************************************************************\
 * Function: fcBuffer_Delete
 * Purpose : 
\******************************************************************************/

void fcBuffer_Delete (fcBuffer_t *fcbuffer_p)
{
    ST_Partition_t *MemoryPartition = fcbuffer_p->MemoryPartition;
    memory_deallocate (MemoryPartition, fcbuffer_p->Buffer_p);
    semaphore_delete (fcbuffer_p->SpaceAvailable_p);    
    memory_deallocate (MemoryPartition, fcbuffer_p->SpaceAvailable_p);
    semaphore_delete (fcbuffer_p->DataAvailable_p);    
    memory_deallocate (MemoryPartition, fcbuffer_p->DataAvailable_p);
    semaphore_delete (fcbuffer_p->Mutex_p);                    
    memory_deallocate (MemoryPartition, fcbuffer_p->Mutex_p);
    memory_deallocate (MemoryPartition, fcbuffer_p);
}


/******************************************************************************\
 * Function: fcBuffer_Insert
 * Purpose : It is used by a task to insert data in the circular buffer. 
 *           If the buffer is full the task is suspended until a item 
 *           is removed, making room for a new one.
 * Error   : Return 1 if buffer is not consistent.
\******************************************************************************/

ST_ErrorCode_t fcBuffer_Insert (fcBuffer_t *fcbuffer_p, void *Item_p)
{
    U32 ItemSize = fcbuffer_p->ItemSize;
    ST_ErrorCode_t res = ST_NO_ERROR;


    /* If Buffer is full then wait */

    semaphore_wait(fcbuffer_p->SpaceAvailable_p);       

	STTBX_Report ((STTBX_REPORT_LEVEL_USER1,"## Insert FC Buffer NumItemStored[%d]\n",fcbuffer_p->NumItemStored));	

    /*** Start of critical section controlled by Mutex semaphor ***/ 

    semaphore_wait(fcbuffer_p->Mutex_p);

    /* Control if buffer is still consistent */

    if (fcbuffer_p->Datainvalidate == FALSE)
    {
        memcpy((void*)((U32)fcbuffer_p->Buffer_p + fcbuffer_p->Tail*ItemSize), 
               Item_p, ItemSize);
        fcbuffer_p->Tail = (fcbuffer_p->Tail + 1) % fcbuffer_p->MaxNumItem;
        fcbuffer_p->NumItemStored = (fcbuffer_p->NumItemStored + 1) ;
    }
    else
    {
       res = FCBUFFER_ERROR_INVALID_DATA;
    }

    semaphore_signal(fcbuffer_p->Mutex_p);

    /*** End of critical section controlled by Mutex semaphor ***/ 

    semaphore_signal(fcbuffer_p->DataAvailable_p);

    return (res);
}


/******************************************************************************\
 * Function: fcBuffer_Remove
 * Purpose : It is used by a task to fetch data from the circular buffer. 
 *           If the buffer is empty then it is suspended untill new data 
 *           is inserted.
 * Parameters:
 *      buffer:
 *           A pointer to the buffer where to copy data
 *           It is assumed that a buffer of sufficient size exists
 * Return  : ST_NO_ERROR on success,
 *           FCBUFFER_ERROR_INVALID_DATA if buffer data was invalid,
 *                                       e.g. after a buffer flush
\******************************************************************************/

ST_ErrorCode_t fcBuffer_Remove (fcBuffer_t *fcbuffer_p, void * buffer)
{
    U32 ItemSize = fcbuffer_p->ItemSize;
    ST_ErrorCode_t res = ST_NO_ERROR;

    /* If there is no data wait */
    semaphore_wait(fcbuffer_p->DataAvailable_p);    

	STTBX_Report ((STTBX_REPORT_LEVEL_USER1,"## Remove FC Buffer NumItemStored[%d]\n",fcbuffer_p->NumItemStored));	

    /*** Start of critical section controlled by Mutex semaphor ***/ 

    semaphore_wait(fcbuffer_p->Mutex_p);

    /* Control if buffer is still consistent */

    if (fcbuffer_p->Datainvalidate == FALSE)
    {
        memcpy(buffer, 
               (void*)((U32)fcbuffer_p->Buffer_p + fcbuffer_p->Head * ItemSize),
               ItemSize);
    
        fcbuffer_p->Head = (fcbuffer_p->Head + 1) % fcbuffer_p->MaxNumItem;
        fcbuffer_p->NumItemStored = (fcbuffer_p->NumItemStored - 1) ;
    }
    else 
    {
        res = FCBUFFER_ERROR_INVALID_DATA;
    }

    semaphore_signal(fcbuffer_p->Mutex_p);

    /*** End of critical section controlled by Mutex semaphor ***/ 

    semaphore_signal(fcbuffer_p->SpaceAvailable_p);

    return (res);
}


/******************************************************************************\
 * Function: fcBuffer_Remove_Timeout
 * Purpose : It is used by a task to fetch data from the circular buffer.
 *           If the buffer is empty then it is suspended untill new data
 *           is inserted.
 *           It waits for a new data packet for at most timeout clock ticks
 * Parameters:
 *      buffer:
 *           A pointer to the buffer where to copy data
 *           It is assumed that a buffer of sufficient size exists
 *      timeout:
 *           Maximum time to wait for a data packet, expressed in ticks
 *           or TIMEOUT_NONE or TIMEOUT_INFINITY
 * Return  : ST_NO_ERROR on success,
 *           FCBUFFER_ERROR_INVALID_DATA if buffer data was invalid,
 *                                       e.g. after a buffer flush
 *           FCBUFFER_WAIT_TIMEDOUT wait timed out
\******************************************************************************/
 
ST_ErrorCode_t fcBuffer_Remove_Timeout (fcBuffer_t *fcbuffer_p, 
                                        void * buffer,
                                        clock_t *timeout)
{
    U32 ItemSize = fcbuffer_p->ItemSize;
    ST_ErrorCode_t res = ST_NO_ERROR;
 
	STTBX_Report ((STTBX_REPORT_LEVEL_USER1,"## Remove_Timeout FC Buffer NumItemStored[%d]\n",fcbuffer_p->NumItemStored));	

    /* waits for data for at most timeout ticks */

    if (semaphore_wait_timeout(fcbuffer_p->DataAvailable_p, timeout) == -1)
    {
        return (FCBUFFER_WAIT_TIMEDOUT);
    }

    /*** Start of critical section controlled by Mutex semaphor ***/
 
    semaphore_wait(fcbuffer_p->Mutex_p);
 
    /* Control if buffer is still consistent */
 
    if (fcbuffer_p->Datainvalidate == FALSE)
    {
        memcpy(buffer,
               (void*)((U32)fcbuffer_p->Buffer_p + fcbuffer_p->Head * ItemSize),
               ItemSize);
 
        fcbuffer_p->Head = (fcbuffer_p->Head + 1) % fcbuffer_p->MaxNumItem;
        fcbuffer_p->NumItemStored = (fcbuffer_p->NumItemStored - 1) ;
    }
    else
    {
        res = FCBUFFER_ERROR_INVALID_DATA;
    }
 
    semaphore_signal(fcbuffer_p->Mutex_p);
 
    /*** End of critical section controlled by Mutex semaphor ***/
 
    semaphore_signal(fcbuffer_p->SpaceAvailable_p);
 
    return (res);
}


/******************************************************************************\
 * Function: fcBuffer_IsEmpty
 * Purpose : It is used by a task to determinate if a buffer is empty. 
 *           It return 1 if the buffer is empty. 
 *           It return 0 if the buffer is not empty. 
\******************************************************************************/

BOOL fcBuffer_IsEmpty (fcBuffer_t *fcbuffer_p)
{
    BOOL empty;

    /*** Start of critical section controlled by Mutex semaphor ***/ 

    semaphore_wait(fcbuffer_p->Mutex_p);

    empty = (fcbuffer_p->NumItemStored == 0);

    /*** End of critical section controlled by Mutex semaphor ***/ 

    semaphore_signal(fcbuffer_p->Mutex_p);

    return (empty);
}


/******************************************************************************\
 * Function: fcBuffer_IsFull
 * Purpose : It is used by a task to determinate if a buffer is full. 
 *           It return 1 if the buffer is full. 
 *           It return 0 if the buffer is not full. 
\******************************************************************************/

BOOL fcBuffer_IsFull (fcBuffer_t *fcbuffer_p)
{
    BOOL full;


    /*** Start of critical section controlled by Mutex semaphor ***/ 

    semaphore_wait(fcbuffer_p->Mutex_p);

    full = (fcbuffer_p->NumItemStored == fcbuffer_p->MaxNumItem);

    /*** End of critical section controlled by Mutex semaphor ***/ 

    semaphore_signal(fcbuffer_p->Mutex_p);

    return (full);
}


/******************************************************************************\
 * Function: fcBuffer_FlushoutConsumer
 * Purpose : It is used to force the exit of tasks waiting to remove an Item
 *           from the buffer.
\******************************************************************************/

void fcBuffer_FlushoutConsumer (fcBuffer_t *fcbuffer_p)
{
    /*** Start of critical section controlled by Mutex semaphor ***/ 

    semaphore_wait(fcbuffer_p->Mutex_p);

    fcbuffer_p->Datainvalidate = TRUE;

    semaphore_signal(fcbuffer_p->DataAvailable_p);

    /*** End of critical section controlled by Mutex semaphor ***/ 

    semaphore_signal(fcbuffer_p->Mutex_p);
}

/******************************************************************************\
 * Function: fcBuffer_FlushoutProducer
 * Purpose : It is used to force the exit of tasks waiting to insert an Item
 *           from the buffer.
\******************************************************************************/

void fcBuffer_FlushoutProducer (fcBuffer_t *fcbuffer_p)
{
    /*** Start of critical section controlled by Mutex semaphor ***/ 

    semaphore_wait(fcbuffer_p->Mutex_p);

    fcbuffer_p->Datainvalidate = TRUE;

    semaphore_signal(fcbuffer_p->SpaceAvailable_p);

    /*** End of critical section controlled by Mutex semaphor ***/ 

    semaphore_signal(fcbuffer_p->Mutex_p);
}



/******************************************************************************\
 * Function: fcBuffer_Reset
 * Purpose : It is used to reset all the structure of the buffer.
\******************************************************************************/

void fcBuffer_Reset (fcBuffer_t *fcbuffer_p)
{
    /*** Start of critical section controlled by Mutex semaphor ***/ 

    semaphore_wait(fcbuffer_p->Mutex_p);
    semaphore_init_fifo_timeout(fcbuffer_p->DataAvailable_p,0);
    semaphore_init_fifo(fcbuffer_p->SpaceAvailable_p,fcbuffer_p->MaxNumItem);

    semaphore_wait(fcbuffer_p->SpaceAvailable_p);

    fcbuffer_p->Head = 0;
    fcbuffer_p->Tail = 0;
    fcbuffer_p->NumItemStored = 0;
    fcbuffer_p->Datainvalidate = FALSE;


    /*** End of critical section controlled by Mutex semaphor ***/ 

    semaphore_signal(fcbuffer_p->Mutex_p);
}

/* End of fcbuffer.c */

