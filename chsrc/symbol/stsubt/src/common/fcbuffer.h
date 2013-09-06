
/*******************************************************************************
 *
 * File Name   : fcbuff.h
 *
 * Description : Definition of a circular buffer to store items 
 *               all of the same size
 *
 * (c) Copyright STMicroelectronics - November 1998
 *
 * Author      : Alessandro Innocenti
*******************************************************************************/

#ifndef __FCBUFFER_H
#define __FCBUFFER_H

#include <time.h>
#include <semaphor.h>

#include <stddefs.h>

#define FCBUFFER_ERROR_INVALID_DATA  	1
#define FCBUFFER_WAIT_TIMEDOUT  	2

/* Defines a Circular Buffer synchronization data structure */
/* to store items all with the same fixed size */

typedef struct fcBuffer_s {
    ST_Partition_t *MemoryPartition;
    void           *Buffer_p ;
    U32             MaxNumItem ;  
    U32             ItemSize;
    U32             Head ;
    U32             Tail ;
    U32             NumItemStored ;
    BOOL            Datainvalidate;
    semaphore_t    *SpaceAvailable_p ;
    semaphore_t    *DataAvailable_p ;
    semaphore_t    *Mutex_p ;
} fcBuffer_t ;

/* 
===============================
Fixed circular buffer Functions 
===============================
*/

fcBuffer_t * fcBuffer_Create (ST_Partition_t *MemoryPartition,
                              U32 MaxNumItem, U32 ItemSize);

void fcBuffer_Delete (fcBuffer_t *FixcBuffer_p);

ST_ErrorCode_t fcBuffer_Insert (fcBuffer_t *FixcBuffer_p, void *Item_p);

ST_ErrorCode_t fcBuffer_Remove (fcBuffer_t *fcbuffer_p, void * buffer);

ST_ErrorCode_t fcBuffer_Remove_Timeout (fcBuffer_t *fcbuffer_p, 
                                        void *buffer, 
                                        clock_t *timeout);

BOOL fcBuffer_IsEmpty (fcBuffer_t *FixcBuffer);

BOOL fcBuffer_IsFull (fcBuffer_t *FixcBuffer);

void fcBuffer_Flushout (fcBuffer_t *fcbuffer_p);

void fcBuffer_FlushoutConsumer (fcBuffer_t *fcbuffer_p);

void fcBuffer_FlushoutProducer (fcBuffer_t *fcbuffer_p);

void fcBuffer_Reset (fcBuffer_t *fcbuffer_p);

#endif  /* #ifndef __FCBUFFER_H */

