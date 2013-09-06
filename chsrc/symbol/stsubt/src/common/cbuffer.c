/******************************************************************************\
 *
 * File Name    : cbuffer.c
 *
 * Description  : 
 *     Implementation of a synchronized circular buffer data structure.  
 *     This is a buffer into which insertion and removal of items are managed 
 *     through a  FIFO structure with two basic operations: Insert() and 
 *     Copy(). The queue is also provided of methods to Peek() at (read only) 
 *     the first item of the queue, to Remove() packets of data from buffer, 
 *     and to determine if the buffer isEmpty() or isFull(). 
 *     Data packet may wrap around the buffer at any point.
 *
 * Note : 
 *     For a optimal synchronization each task (either a producer or 
 *     a consumer) should provide its own private semaphore. This is a 
 *     semplified version of circular buffer created to be used by just 
 *     one producer and one consumer; semaphores are built-in into the data 
 *     structure. 
 *
 * Author : Marino Congiu - Dec 1998
 *
 * Copyright STMicroelectronics - November 1998
 *
\******************************************************************************/
#include <stdlib.h>
#include <string.h>

#include <stddefs.h>
#include <stlite.h>
#include <sttbx.h>
#include "subtdev.h"

#include "cbuffer.h"


/******************************************************************************\
 * Function: cBuffer_Create
 * Purpose : Create a circular buffer of suitable size
 *           Buffer size is aligned on a 32-bits word boundary.
 * Parameters:
 *      MemoryPartition: 
 *           Partition from which memory is allocated.
 *      BufferSize: 
 *           Required size of buffer in bytes.
 *      RealBufferSize:
 *           Returned size of allocated buffer.
 *           Buffer size will be aligned to a 32 bits word boundary, so real
 *           buffer size may be different (greater) than needed.
 * Return  : A pointer to the base of the buffer; NULL in case of error
\******************************************************************************/

cBuffer_t * cBuffer_Create (ST_Partition_t *MemoryPartition,
                            U32 BufferSize, U32 *RealBufferSize)
{
    cBuffer_t *cbuffer_p;

    /* Align buffer size on a word boundary */

    *RealBufferSize = ((BufferSize + 0x3) & ~0x3);
 
    /* Alloc memory for buffer and initialize structure  */

    if ((cbuffer_p = (cBuffer_t *)memory_allocate (MemoryPartition, 
                                                   sizeof(cBuffer_t))) == NULL)
    {
        *RealBufferSize = 0;
        return (NULL);
    }

    /* register partition name */

    cbuffer_p->MemoryPartition = MemoryPartition;

    /* Create semaphore */
    cbuffer_p->SpaceAvailable_p = 
        (semaphore_t *)memory_allocate(MemoryPartition, sizeof(semaphore_t));
    if (cbuffer_p->SpaceAvailable_p == NULL)
    {     
        /* free resources previosly allocated*/
        memory_deallocate (MemoryPartition, cbuffer_p);  
        *RealBufferSize = 0;
        return (NULL); 
    }
    semaphore_create_fifo(cbuffer_p->SpaceAvailable_p, 0);       

    cbuffer_p->DataAvailable_p = 
        (semaphore_t *) memory_allocate(MemoryPartition, sizeof(semaphore_t));
    if (cbuffer_p->DataAvailable_p == NULL)
    {
        /* free resources previosly allocated*/
        semaphore_delete (cbuffer_p->SpaceAvailable_p);
        memory_deallocate (MemoryPartition, cbuffer_p->SpaceAvailable_p);
        memory_deallocate (MemoryPartition, cbuffer_p);
        *RealBufferSize = 0;
        return (NULL);
    }
    semaphore_create_fifo(cbuffer_p->DataAvailable_p, 0);

    cbuffer_p->Mutex_p = (semaphore_t *)memory_allocate (MemoryPartition, 
                                                         sizeof(semaphore_t));
    if (cbuffer_p->Mutex_p == NULL)
    {
        /* free resources previosly allocated*/
        semaphore_delete (cbuffer_p->SpaceAvailable_p);        
        memory_deallocate (MemoryPartition, cbuffer_p->SpaceAvailable_p);
        semaphore_delete (cbuffer_p->DataAvailable_p);                
        memory_deallocate (MemoryPartition, cbuffer_p->DataAvailable_p);
        memory_deallocate (MemoryPartition, cbuffer_p);
        *RealBufferSize = 0;
        return (NULL);
    }
    semaphore_create_fifo(cbuffer_p->Mutex_p, 1);

    /* Alloc memory buffer */
    cbuffer_p->Buffer_p = memory_allocate(MemoryPartition, *RealBufferSize);   
    if (cbuffer_p->Buffer_p == NULL)
    {
        /* if memory is not available, free resources previosly allocated */
        semaphore_delete (cbuffer_p->SpaceAvailable_p);        
        memory_deallocate (MemoryPartition, cbuffer_p->SpaceAvailable_p);
        semaphore_delete (cbuffer_p->DataAvailable_p);                        
        memory_deallocate (MemoryPartition, cbuffer_p->DataAvailable_p);
        semaphore_delete (cbuffer_p->Mutex_p);
        memory_deallocate (MemoryPartition, cbuffer_p->Mutex_p);                     
        memory_deallocate (MemoryPartition, cbuffer_p);    
        *RealBufferSize = 0;
        return (NULL);
    }

    /* Data is inserted into the Tail position and removed from the Head. */

    cbuffer_p->Head = cbuffer_p->Buffer_p;
    cbuffer_p->Tail = cbuffer_p->Buffer_p;

    cbuffer_p->BufferSize = *RealBufferSize;
    cbuffer_p->NumBytesStored = 0;

    /* EndBuffer is heavely used by routines, so its value is provided */
    cbuffer_p->EndBuffer = (void *)((U32)cbuffer_p->Buffer_p + *RealBufferSize);

    cbuffer_p->DataInvalidated = FALSE;
    cbuffer_p->DataOverWritten = FALSE;

    return (cbuffer_p);                                           
}


/******************************************************************************\
 * Function: cBuffer_Delete
 * Purpose : Delete a circular buffer; 
 *           All allocated resources are freed.
\******************************************************************************/

void cBuffer_Delete (cBuffer_t *cbuffer_p)
{
    ST_Partition_t *MemoryPartition = cbuffer_p->MemoryPartition;

    memory_deallocate (MemoryPartition, cbuffer_p->Buffer_p);
    semaphore_delete (cbuffer_p->SpaceAvailable_p);    
    memory_deallocate (MemoryPartition, cbuffer_p->SpaceAvailable_p);
    semaphore_delete (cbuffer_p->DataAvailable_p);                    
    memory_deallocate (MemoryPartition, cbuffer_p->DataAvailable_p);
    semaphore_delete (cbuffer_p->Mutex_p);    
    memory_deallocate (MemoryPartition, cbuffer_p->Mutex_p);
    memory_deallocate (MemoryPartition, cbuffer_p);
}


/******************************************************************************\
 * Function: cBuffer_Insert
 * Purpose : Add nbytes of data to the tail of the buffer. 
 *           It is used by a task to depose data into the buffer. 
 *           If the buffer is full the task is suspended until data 
 *           is removed, making room for new data.
 * Return  : - ST_NO_ERROR on success, 
 *           - CBUFFER_ERROR_NOT_ENOUGHT_SPACE if cbuffer was not big enought,
 *           - CBUFFER_ERROR_INVALID_DATA if data buffer was invalid, 
 *                                        e.g. after a buffer flush
 *           - CBUFFER_GENERIC_ERROR on generic failure
 * Note    : A Producer task tries to acquire the "resource" buffer opening a 
 *           critical section. If space is not available, it suspends itself,
 *           but only outside of the critical section, in order to avoid 
 *           deadlock (this is done through a preventive signal on the 
 *           semaphore SpaceAvailable_p).
 *           Then it deposes data, and release the resource buffer. 
 * Note    : The first four bytes of each block of data in the buffer will
 *           contain the size in bytes of the block.
 *
\******************************************************************************/
extern BOOL myflag;


ST_ErrorCode_t cBuffer_Insert (cBuffer_t *cbuffer_p,
                                  void *Data_p,
                                  U32   nbytes)
{
    cBuffer_PacketHeader_t packet;
    BOOL DataStored  = FALSE;
    U32  end_free_bytes;
    U32  start_free_bytes;
    U32  RealDataSize;
    U32  total_bytes; 



    /* Allign data size on a 32-bit word boundary */
 
    RealDataSize = ((nbytes + 0x3) & ~0x3);

    /* The total number of bytes allocated for the block of data is 
     * the size of the block aligned to a word boundary + 
     * CBUFFER_PACKET_HEADER_SIZE bytes to store control packet data.
     */

    total_bytes = RealDataSize + CBUFFER_PACKET_HEADER_SIZE;

    /* Check if packet can be stored in buffer */

    if (total_bytes > cbuffer_p->BufferSize)
    {
        return (CBUFFER_ERROR_NOT_ENOUGHT_SPACE);
    }

    while ((DataStored == FALSE) && (cbuffer_p->DataInvalidated == FALSE))
    {
        /* --- Start critical section --- */

        semaphore_wait(cbuffer_p->Mutex_p);

        /* Is there enought space available? */

        if (((cbuffer_p->NumBytesStored + total_bytes)<=cbuffer_p->BufferSize))
        {
          /* --- There is enough free space: check if it is adjacent --- */

          if ((U32)cbuffer_p->Head <= (U32)cbuffer_p->Tail)
          {
              end_free_bytes = (U32)cbuffer_p->EndBuffer - (U32)cbuffer_p->Tail;
              start_free_bytes =(U32)cbuffer_p->Head - (U32)cbuffer_p->Buffer_p;

              if (end_free_bytes >= total_bytes)
              {
                  /* Add header of data block as first bytes of block */

                  packet.size = nbytes;
                  packet.dummy_flag = CBUFFER_FALSE;

                  memcpy(cbuffer_p->Tail, &packet, CBUFFER_PACKET_HEADER_SIZE);
                  
                  cbuffer_p->Tail = (void*)((U8*)cbuffer_p->Tail + CBUFFER_PACKET_HEADER_SIZE);
                  
                  /* Insert Packet */
                  memcpy(cbuffer_p->Tail, Data_p, nbytes);

                  
                  if (end_free_bytes == total_bytes)
                       cbuffer_p->Tail = cbuffer_p->Buffer_p;
                  else
                      cbuffer_p->Tail = (void*)((U8*)cbuffer_p->Tail + RealDataSize);
                  
                  DataStored = TRUE;
              }
              else if (start_free_bytes >= total_bytes)
              {
                  /* space at the end of the buffer is not sufficient.
                   * but there is enough space at the beginning.
                   */

                  /* fill free space at end of buffer with a dummy packet */
    
                  packet.size = end_free_bytes - CBUFFER_PACKET_HEADER_SIZE;
                  packet.dummy_flag = CBUFFER_TRUE;

                  memcpy(cbuffer_p->Tail, &packet, CBUFFER_PACKET_HEADER_SIZE);
                  /*cbuffer_p->Tail = cbuffer_p->Buffer_p; not necessary */

                  cbuffer_p->NumBytesStored += end_free_bytes ;

                  /* insert packet at the beginning of buffer */

                  packet.size = nbytes;
                  packet.dummy_flag = CBUFFER_FALSE;

                  memcpy(cbuffer_p->Buffer_p, &packet, 
                                            CBUFFER_PACKET_HEADER_SIZE);
                  cbuffer_p->Tail = (void*)((U8*)cbuffer_p->Buffer_p +
                                            CBUFFER_PACKET_HEADER_SIZE);
                  memcpy(cbuffer_p->Tail, Data_p, nbytes);
                  cbuffer_p->Tail = (void*)((U8*)cbuffer_p->Tail+RealDataSize);

                  DataStored = TRUE;
              }
          }
          else if (((U32)cbuffer_p->Head - (U32)cbuffer_p->Tail) >= total_bytes)
          {
              /* Add header of data block as first bytes of block */

              packet.size = nbytes;
              packet.dummy_flag = CBUFFER_FALSE;

              memcpy(cbuffer_p->Tail, &packet, CBUFFER_PACKET_HEADER_SIZE);
              cbuffer_p->Tail = (void*)((U8*)cbuffer_p->Tail +
                                        CBUFFER_PACKET_HEADER_SIZE);
              /* Insert Packet */
              memcpy(cbuffer_p->Tail, Data_p, nbytes);
              cbuffer_p->Tail = (void*)((U8*)cbuffer_p->Tail + RealDataSize);
              DataStored = TRUE;
          }

          /* Preventive signal */

          if (DataStored == TRUE)
          {
              cbuffer_p->NumBytesStored += total_bytes ;
              semaphore_signal(cbuffer_p->SpaceAvailable_p);
          }
        }

        /* --- End of critical section --- */

        semaphore_signal(cbuffer_p->Mutex_p);

        /* Suspend the task if not enought space was available */

        semaphore_wait(cbuffer_p->SpaceAvailable_p);

    }  /* end while */

    /* Possibly wake up a task which is waiting for data */ 

    semaphore_signal(cbuffer_p->DataAvailable_p);

    if (cbuffer_p->DataInvalidated == TRUE)
    {
         return (CBUFFER_ERROR_INVALID_DATA);
    }
    else return (ST_NO_ERROR);
}


#if (defined DVD_TRANSPORT_LINK) && (!defined PTI_EMULATION_MODE)
/******************************************************************************\
 * Function: cBuffer_InsertSegmentFromCircular
 * Purpose : Essentially the same as the cBuffer_Insert function, but
             takes data from circular FilterBuffer managing the wrap condition
             should it occur.
 * Return 
\******************************************************************************/

ST_ErrorCode_t cBuffer_InsertSegmentFromCircular(void *Handle,
                                                 U32   nbytes)
{
    cBuffer_PacketHeader_t packet;
    BOOL DataStored  = FALSE;
    U32  end_free_bytes;
    U32  start_free_bytes;
    U32  RealDataSize;
    U32  total_bytes;
    cBuffer_t *cbuffer_p;
    STSUBT_InternalHandle_t *DecoderHandle;

    DecoderHandle = (STSUBT_InternalHandle_t*) Handle;

    
    cbuffer_p = DecoderHandle->CodedDataBuffer;


    /* Allign data size on a 32-bit word boundary */

    RealDataSize = ((nbytes + 0x3) & ~0x3);
    
    /* The total number of bytes allocated for the block of data is 
     * the size of the block aligned to a word boundary + 
     * CBUFFER_PACKET_HEADER_SIZE bytes to store control packet data.
     */

    total_bytes = RealDataSize + CBUFFER_PACKET_HEADER_SIZE;

    /* Check if packet can be stored in buffer */

    if (total_bytes > cbuffer_p->BufferSize)
    {
        return (CBUFFER_ERROR_NOT_ENOUGHT_SPACE);
    }

    while ((DataStored == FALSE) && (cbuffer_p->DataInvalidated == FALSE))
    {
        /* --- Start critical section --- */

        semaphore_wait(cbuffer_p->Mutex_p);

        /* Is there enought space available? */

        if (((cbuffer_p->NumBytesStored + total_bytes)<=cbuffer_p->BufferSize))
        {
          /* --- There is enough free space: check if it is adjacent --- */

          if ((U32)cbuffer_p->Head <= (U32)cbuffer_p->Tail)
          {
              end_free_bytes = (U32)cbuffer_p->EndBuffer - (U32)cbuffer_p->Tail;
              start_free_bytes =(U32)cbuffer_p->Head - (U32)cbuffer_p->Buffer_p;

              if (end_free_bytes >= total_bytes)
              {
                  /* Add header of data block as first bytes of block */

                  packet.size = nbytes;
                  packet.dummy_flag = CBUFFER_FALSE;

                  memcpy(cbuffer_p->Tail, &packet, CBUFFER_PACKET_HEADER_SIZE);
                  
                  cbuffer_p->Tail = (void*)((U8*)cbuffer_p->Tail + CBUFFER_PACKET_HEADER_SIZE);

                  
                  /* Need to wrap FilterBuffer to exract?.. */
                  if (((DecoderHandle->FilterBuffer + DecoderHandle->FilterBufferSize) -
                       DecoderHandle->FilterBufferSegmentConsumer) >= nbytes)
                  {
                      /* no wrap, ok to take straight out */                 
                      memcpy(cbuffer_p->Tail, DecoderHandle->FilterBufferSegmentConsumer, nbytes);
                  }
                  else
                  {  /* wrap required, so take some from top...*/                      
                      memcpy(cbuffer_p->Tail,
                             DecoderHandle->FilterBufferSegmentConsumer,
                             (U32)((DecoderHandle->FilterBuffer + DecoderHandle->FilterBufferSize) -
                              DecoderHandle->FilterBufferSegmentConsumer));

                      /* and rest from bottom */
                      memcpy( (((U8*)cbuffer_p->Tail) +
                               (U32)((DecoderHandle->FilterBuffer +
                                      DecoderHandle->FilterBufferSize) -
                                     DecoderHandle->FilterBufferSegmentConsumer)),
                              DecoderHandle->FilterBuffer,
                              (nbytes - (U32)(((DecoderHandle->FilterBuffer +
                                                DecoderHandle->FilterBufferSize) -
                                               DecoderHandle->FilterBufferSegmentConsumer))));
                      
                  }

                  
                  if (end_free_bytes == total_bytes)
                       cbuffer_p->Tail = cbuffer_p->Buffer_p;
                  else
                      cbuffer_p->Tail = (void*)((U8*)cbuffer_p->Tail + RealDataSize);
                  
                  DataStored = TRUE;
              }
              else if (start_free_bytes >= total_bytes)
              {
                  /* space at the end of the buffer is not sufficient.
                   * but there is enough space at the beginning.
                   */

                  /* fill free space at end of buffer with a dummy packet */
    
                  packet.size = end_free_bytes - CBUFFER_PACKET_HEADER_SIZE;
                  packet.dummy_flag = CBUFFER_TRUE;

                  memcpy(cbuffer_p->Tail, &packet, CBUFFER_PACKET_HEADER_SIZE);
                  /*cbuffer_p->Tail = cbuffer_p->Buffer_p; not necessary */

                  cbuffer_p->NumBytesStored += end_free_bytes ;

                  /* insert packet at the beginning of buffer */

                  packet.size = nbytes;
                  packet.dummy_flag = CBUFFER_FALSE;

                  memcpy(cbuffer_p->Buffer_p, &packet, 
                                            CBUFFER_PACKET_HEADER_SIZE);
                  
                  cbuffer_p->Tail = (void*)((U8*)cbuffer_p->Buffer_p +
                                            CBUFFER_PACKET_HEADER_SIZE);


                  /* Need to wrap FilterBuffer to exract?.. */
                  if (((DecoderHandle->FilterBuffer + DecoderHandle->FilterBufferSize) -
                       DecoderHandle->FilterBufferSegmentConsumer) >= nbytes)
                  {
                      /* no wrap, ok to take straight out */                 
                      memcpy(cbuffer_p->Tail, DecoderHandle->FilterBufferSegmentConsumer, nbytes);
                  }
                  else
                  {  /* wrap required, so take some from top...*/                      
                      memcpy(cbuffer_p->Tail,
                             DecoderHandle->FilterBufferSegmentConsumer,
                             (U32)(((DecoderHandle->FilterBuffer + DecoderHandle->FilterBufferSize) -
                                    DecoderHandle->FilterBufferSegmentConsumer)));

                      /* and rest from bottom */
                      memcpy( (((U8*)cbuffer_p->Tail) +
                               ((DecoderHandle->FilterBuffer +
                                 DecoderHandle->FilterBufferSize) -
                                DecoderHandle->FilterBufferSegmentConsumer)),                              
                              DecoderHandle->FilterBuffer,
                              (nbytes - (U32)(((DecoderHandle->FilterBuffer +
                                                DecoderHandle->FilterBufferSize) -
                                               DecoderHandle->FilterBufferSegmentConsumer))));
                      
                  }
                                   
                  cbuffer_p->Tail = (void*)((U8*)cbuffer_p->Tail+RealDataSize);

                  DataStored = TRUE;
              }
          }
          else if (((U32)cbuffer_p->Head - (U32)cbuffer_p->Tail) >= total_bytes)
          {
              /* Add header of data block as first bytes of block */

              packet.size = nbytes;
              packet.dummy_flag = CBUFFER_FALSE;

              memcpy(cbuffer_p->Tail, &packet, CBUFFER_PACKET_HEADER_SIZE);
              
              cbuffer_p->Tail = (void*)((U8*)cbuffer_p->Tail +
                                        CBUFFER_PACKET_HEADER_SIZE);
                  
              /* Need to wrap FilterBuffer to exract?.. */
              
              if (((DecoderHandle->FilterBuffer + DecoderHandle->FilterBufferSize) -
                   DecoderHandle->FilterBufferSegmentConsumer) >= nbytes)
              {
                  /* no wrap, ok to take straight out */                 
                  memcpy(cbuffer_p->Tail, DecoderHandle->FilterBufferSegmentConsumer, nbytes);
              }
              else
              {  /* wrap required, so take some from top...*/
                                    
                  memcpy(cbuffer_p->Tail,
                         DecoderHandle->FilterBufferSegmentConsumer,
                         (U32)(((DecoderHandle->FilterBuffer +
                                 DecoderHandle->FilterBufferSize) -
                                DecoderHandle->FilterBufferSegmentConsumer)));
                  
                  /* and rest from bottom */
                  memcpy( ((U8*)cbuffer_p->Tail) +
                          (U32)(((DecoderHandle->FilterBuffer +
                                  DecoderHandle->FilterBufferSize) -
                                 DecoderHandle->FilterBufferSegmentConsumer)),
                          DecoderHandle->FilterBuffer,
                          (nbytes - (U32)(((DecoderHandle->FilterBuffer +
                                            DecoderHandle->FilterBufferSize) -
                                           DecoderHandle->FilterBufferSegmentConsumer))));
                  
              }
              
              cbuffer_p->Tail = (void*)((U8*)cbuffer_p->Tail + RealDataSize);
              DataStored = TRUE;
          }

          /* Preventive signal */

          if (DataStored == TRUE)
          {
              cbuffer_p->NumBytesStored += total_bytes ;
              semaphore_signal(cbuffer_p->SpaceAvailable_p);
          }
        }

        /* --- End of critical section --- */

        semaphore_signal(cbuffer_p->Mutex_p);

        /* Suspend the task if not enought space was available */

        semaphore_wait(cbuffer_p->SpaceAvailable_p);

    }  /* end while */

    /* Possibly wake up a task which is waiting for data */ 

    semaphore_signal(cbuffer_p->DataAvailable_p);

    if (cbuffer_p->DataInvalidated == TRUE)
    {
         return (CBUFFER_ERROR_INVALID_DATA);
    }
    else return (ST_NO_ERROR);
}

#endif  /* DVD_TRANSPORT_LINK */

/******************************************************************************\
 * Function: cBuffer_Copy
 * Purpose : Read and Remove a packet of data from the head of cbuffer_p;
 *           Data is copied into a provided buffer.
 *           If the buffer is empty the calling task is suspended until new data
 *           is inserted.
 * Parameters:
 *      data_size:
 *           Returned number of bytes available for the current data packet.
 *           If buffer_size is less that this value than the buffer was not 
 *           big enought and data were lost; 
 *           CBUFFER_ERROR_NOT_ENOUGHT_SPACE is returned in that case.
 *      buffer: 
 *           A pointer to the buffer where to copy data
 *      buffer_size:
 *           Size in bytes of buffer.
 * Return  : 
 *           - ST_NO_ERROR on success,
 *           - CBUFFER_ERROR_NOT_ENOUGHT_SPACE if buffer is not big enought,
 *           - CBUFFER_ERROR_INVALID_DATA if buffer data was invalid, 
 *                                        e.g. after a buffer flush
 *           - CBUFFER_GENERIC_ERROR on generic failure
\******************************************************************************/
 
ST_ErrorCode_t cBuffer_Copy (cBuffer_t *cbuffer_p, U32 *data_size, 
                             void * buffer, U32 buffer_size)
{
    cBuffer_PacketHeader_t packet;
    BOOL GotData = FALSE;
    U32  RealDataSize;
    U32  nBytesRead;
    ST_ErrorCode_t res = ST_NO_ERROR;
 
    while ((GotData == FALSE) && (cbuffer_p->DataInvalidated == FALSE))
    {
        /* --- Start of critical section --- */
 
        semaphore_wait(cbuffer_p->Mutex_p);
 
        if (cbuffer_p->NumBytesStored > 0)
        {
          /* Check if first available packet is a dummy Packet */ 

          packet = *(cBuffer_PacketHeader_t*)cbuffer_p->Head;
          if (packet.dummy_flag == CBUFFER_TRUE)
          {
            /*  Skip dummy packet  */

            RealDataSize = ((packet.size + 0x3) & ~0x3);
            cbuffer_p->Head = (void*)cbuffer_p->Buffer_p;
            cbuffer_p->NumBytesStored -= ( RealDataSize 
                                         + CBUFFER_PACKET_HEADER_SIZE);
          }

          if (cbuffer_p->NumBytesStored > 0)
          {
              /* First bytes contains control packet data */
              /* Compute size of data packet and fetch data */
    
              packet = *(cBuffer_PacketHeader_t*)cbuffer_p->Head;
    
              /* If buffer size is smaller than packet size, then */
              /* we can read just buffer_size of data */
    
              nBytesRead = packet.size;
              if (buffer_size < packet.size)
              {
                  nBytesRead = buffer_size;
                  res = CBUFFER_ERROR_NOT_ENOUGHT_SPACE;
              }
    
              /* Compute real packet size.
               * It is the next address aligned on a 32-bit word boundary 
               */
    
              RealDataSize = ((packet.size + 0x3) & ~0x3);
    
              cbuffer_p->Head = (void*)((U8*)cbuffer_p->Head +
                                        CBUFFER_PACKET_HEADER_SIZE);
              
              memcpy(buffer, cbuffer_p->Head, nBytesRead);
              cbuffer_p->Head = (void*)((U8*)cbuffer_p->Head + RealDataSize);
              if (cbuffer_p->Head >= cbuffer_p->EndBuffer)
                  cbuffer_p->Head = cbuffer_p->Buffer_p;
              
              cbuffer_p->NumBytesStored -= (RealDataSize +
                                            CBUFFER_PACKET_HEADER_SIZE);
              if (cbuffer_p->NumBytesStored == 0)
              {
                  cbuffer_p->Head = cbuffer_p->Buffer_p;
                  cbuffer_p->Tail = cbuffer_p->Buffer_p;
              }
    
              /* Preventive signal */
              semaphore_signal(cbuffer_p->DataAvailable_p);
    
              GotData = TRUE;
          }
        }
 
        /* --- End of critical section --- */
 
        semaphore_signal(cbuffer_p->Mutex_p);
 
        /* Suspend the task if data was not available */
        semaphore_wait(cbuffer_p->DataAvailable_p);
 
    } /* end while */
 
    /* Possibly wake up a task which is waiting to insert  */

    semaphore_signal(cbuffer_p->SpaceAvailable_p);
        
    if (cbuffer_p->DataInvalidated == TRUE)
    {
         *data_size = 0;
         return (CBUFFER_ERROR_INVALID_DATA);
    }
    *data_size = nBytesRead;
    return (res);
}


/******************************************************************************\
 * Function: cBuffer_Peek
 * Purpose : Extracts information about next available packet of data 
 *           in the circular buffer. 
 *           Calling task may be suspended waiting for data.
 * Parameters:
 *       data_size:
 *           Returned number of bytes of the data packet
 *       data_pointer:
 *           Returned pointer to the packet data
 * Return  : ST_NO_ERROR on success,
 *           CBUFFER_ERROR_INVALID_DATA if buffer data was invalid,
 *           e.g. after a buffer flush
 * Note    : The pointers to the head and the tail of the buffer are not updated
 *           cBuffer_Peek can be used togheter with cBuffer_Remove
 *           in order to update pointers. (Be worry in case more consumers tasks
 *           are accessing the buffer).
\******************************************************************************/
 
ST_ErrorCode_t cBuffer_Peek (cBuffer_t *cbuffer_p, 
                             U32   *data_size,
                             void **data_pointer)
{
    cBuffer_PacketHeader_t packet;
    BOOL PeekedData = FALSE;
 
    while ((PeekedData == FALSE) && (cbuffer_p->DataInvalidated == FALSE))
    {
        /* --- Start of critical section --- */
 
        semaphore_wait(cbuffer_p->Mutex_p);
 
        if (cbuffer_p->NumBytesStored > 0)
        {
            /* Check if first available packet is a dummy Packet */ 

            packet = *(cBuffer_PacketHeader_t*)cbuffer_p->Head;
            if (packet.dummy_flag == CBUFFER_TRUE)
            {
                /*  Skip dummy packet  */

                cbuffer_p->Head = (void*)cbuffer_p->Buffer_p;
                cbuffer_p->NumBytesStored -= ( ((packet.size + 0x3) & ~0x3)
                                         + CBUFFER_PACKET_HEADER_SIZE);
            }

            if (cbuffer_p->NumBytesStored > 0)
            {
                /* First bytes contains control packet data */
                /* Compute size of data packet and fetch data */
      
                packet = *(cBuffer_PacketHeader_t*)cbuffer_p->Head;
      
                *data_size = packet.size;
                *data_pointer = (void*)((U8*)cbuffer_p->Head +
                                        CBUFFER_PACKET_HEADER_SIZE);
      
                /* Preventive signal */
                semaphore_signal(cbuffer_p->DataAvailable_p);
      
                PeekedData = TRUE;
            }
        }
 
        /* --- End of critical section --- */
 
        semaphore_signal(cbuffer_p->Mutex_p);
 
        /* Suspend the task if data was not available */
        semaphore_wait(cbuffer_p->DataAvailable_p);
 
    } /* end while */
 
    if (cbuffer_p->DataInvalidated == TRUE)
    {
        *data_size = 0;
        *data_pointer = NULL; 
        return (CBUFFER_ERROR_INVALID_DATA);
    }
    return (ST_NO_ERROR);
}


/******************************************************************************\
 * Function: cBuffer_Remove
 * Purpose : Removes a packet of data from the head of the buffer.
 *           If no data is available then returns immediately.
 *           It is likely to be used after cBuffer_Peek in order to update
 *           the head and tail pointers.
 * Return  : ST_NO_ERROR on success,
 *           CBUFFER_ERROR_INVALID_DATA if buffer data was invalid,
 *           e.g. after a buffer flush
\******************************************************************************/

ST_ErrorCode_t cBuffer_Remove(cBuffer_t *cbuffer_p)
{
    cBuffer_PacketHeader_t packet;
    U32 RealDataSize;
    BOOL RemovedData = FALSE;

    /* --- Start of critical section --- */

    semaphore_wait(cbuffer_p->Mutex_p);

    if ((cbuffer_p->NumBytesStored > 0) && (cbuffer_p->DataInvalidated==FALSE))
    {
        /* Check if first available packet is a dummy Packet */ 

        packet = *(cBuffer_PacketHeader_t*)cbuffer_p->Head;
        if (packet.dummy_flag == CBUFFER_TRUE)
        {
            /*  Skip dummy packet  */

            RealDataSize = ((packet.size + 0x3) & ~0x3);
            cbuffer_p->Head = (void*)cbuffer_p->Buffer_p;
            cbuffer_p->NumBytesStored -= ( RealDataSize 
                                         + CBUFFER_PACKET_HEADER_SIZE);
        }

        if (cbuffer_p->NumBytesStored > 0)
        {
            /* First bytes contains control packet data */
            /* Compute size of data packet and fetch data */
   
            packet = *(cBuffer_PacketHeader_t*)cbuffer_p->Head;
  
            /* Compute real packet size.
             * It is the next address aligned on a 32-bit word boundary 
             */
  
            RealDataSize = ((packet.size + 0x3) & ~0x3);
  
            cbuffer_p->Head = (void*)((U8*)cbuffer_p->Head 
                              + CBUFFER_PACKET_HEADER_SIZE + RealDataSize);
            
            if (cbuffer_p->Head >= cbuffer_p->EndBuffer)
                cbuffer_p->Head = cbuffer_p->Buffer_p;
            
            cbuffer_p->NumBytesStored -= (RealDataSize +
                                          CBUFFER_PACKET_HEADER_SIZE);
            if (cbuffer_p->NumBytesStored == 0)
            {
                cbuffer_p->Head = cbuffer_p->Buffer_p;
                cbuffer_p->Tail = cbuffer_p->Buffer_p;
            }
        }
        RemovedData = TRUE;
    }

    /* --- End of critical section --- */

    semaphore_signal(cbuffer_p->Mutex_p);
 
    /* Possibly wake up a task which is waiting to insert  */

    if (RemovedData == TRUE)
        semaphore_signal(cbuffer_p->SpaceAvailable_p);

    if (cbuffer_p->DataInvalidated == TRUE)
         return (CBUFFER_ERROR_INVALID_DATA);
    else return (ST_NO_ERROR);
}


/******************************************************************************\
 * Function: cBuffer_isEmpty
 * Purpose : Determine if buffer is empty
 * Return  : True if the buffer is empty; False otherwise.
\******************************************************************************/
 
BOOL cBuffer_isEmpty(cBuffer_t *cbuffer_p)
{
    BOOL empty;
 
    /* --- Enter critical section --- */
 
    semaphore_wait(cbuffer_p->Mutex_p);
 
    empty = (cbuffer_p->NumBytesStored == 0);
 
    semaphore_signal(cbuffer_p->Mutex_p);
 
    /* --- End of critical section --- */
 
    return (empty);
}

 
/******************************************************************************\
 * Function: cBuffer_isFull
 * Purpose : Determine if buffer is full
 * Return  : True if the buffer is full; False otherwise.
\******************************************************************************/
 
BOOL cBuffer_isFull(cBuffer_t *cbuffer_p)
{
    BOOL full;
 
    /* --- Enter of critical section --- */
 
    semaphore_wait(cbuffer_p->Mutex_p);
 
    full = (cbuffer_p->NumBytesStored == cbuffer_p->BufferSize);
 
    semaphore_signal(cbuffer_p->Mutex_p);
 
    /* --- End of critical section --- */
 
    return (full);
}


/******************************************************************************\
 * Function: cBuffer_FlushOutProducer
 * Purpose : Invalidate buffer data and force a producer task out of the 
 *           semaphore queue (if any)
 * Note    : This function is called when producer task is being stopped,
 *           and it must be assured that it is not suspended on a semaphore. 
\******************************************************************************/
 
void cBuffer_FlushOutProducer (cBuffer_t *cbuffer_p)
{
    semaphore_wait(cbuffer_p->Mutex_p);
 
    cbuffer_p->DataInvalidated = TRUE;
 
    semaphore_signal(cbuffer_p->SpaceAvailable_p);
 
    semaphore_signal(cbuffer_p->Mutex_p);
}


/******************************************************************************\
 * Function: cBuffer_FlushOutConsumer
 * Purpose : Invalidate buffer data and force a consumer task out of the 
 *           semaphore queue (if any)
 * Note    : This function is called when consumer task is being stopped.,
 *           and it must be assured that it is not suspended on a semaphore. 
\******************************************************************************/
 
void cBuffer_FlushOutConsumer (cBuffer_t *cbuffer_p)
{
    semaphore_wait(cbuffer_p->Mutex_p);
 
    cbuffer_p->DataInvalidated = TRUE;
 
    semaphore_signal(cbuffer_p->DataAvailable_p);
 
    semaphore_signal(cbuffer_p->Mutex_p);
}


/******************************************************************************\
 * Function: cBuffer_Flush
 * Purpose : Invalidate buffer data and force both consumer and producer tasks
 *           out of the semaphore queues (if any).
 * Note    : This function is called when tasks are being stopped,
 *           and it must be assured that they are not suspended on semaphores.
\******************************************************************************/
 
void cBuffer_Flush (cBuffer_t *cbuffer_p)
{
    /* --- Enter critical section --- */

    semaphore_wait(cbuffer_p->Mutex_p);

    cbuffer_p->DataInvalidated = TRUE;
 
    /* Possibly wake up a consumer task */

    semaphore_signal(cbuffer_p->DataAvailable_p);

    /* Possibly wake up a producer task */

    semaphore_signal(cbuffer_p->SpaceAvailable_p);

    semaphore_signal(cbuffer_p->Mutex_p);

    /* --- End of critical section --- */
}


/******************************************************************************\
 * Function: cBuffer_Reset
 * Purpose : Reset the status of the buffer.
 * Note    : Buffer must have been flushed before reset.
\******************************************************************************/
 
void cBuffer_Reset (cBuffer_t *cbuffer_p)
{
    /* --- Enter critical section --- */

    semaphore_wait(cbuffer_p->Mutex_p);
    semaphore_create_fifo(cbuffer_p->SpaceAvailable_p, 0);       
    semaphore_create_fifo(cbuffer_p->DataAvailable_p, 0);

    cbuffer_p->Head = cbuffer_p->Buffer_p;
    cbuffer_p->Tail = cbuffer_p->Buffer_p;

    cbuffer_p->NumBytesStored = 0;

    cbuffer_p->DataInvalidated = FALSE;

    semaphore_signal(cbuffer_p->Mutex_p);

    /* --- End of critical section --- */
}


/******************************************************************************\
 * Function: cBuffer_InsertAPDU
 * Purpose : Insert an APDU into the tail of the buffer
 *           If the buffer is full the task is suspended until data 
 *           is removed, making room for new data.
 * Return  : - ST_NO_ERROR on success, 
 *           - CBUFFER_ERROR_NOT_ENOUGHT_SPACE if cbuffer was not big enought,
 *           - CBUFFER_ERROR_INVALID_DATA if data buffer was invalid, 
 *                                        e.g. after a buffer flush
 *           - CBUFFER_GENERIC_ERROR on generic failure
\******************************************************************************/

ST_ErrorCode_t cBuffer_InsertAPDU (cBuffer_t *cbuffer_p,
                             U32   apdu_tag,
                             U32   apdu_size,
                             void *apdu_data,
                             U8    packet_status,
                             U8    reset_flag /*,
                             U8    overriding_flag,
                             U8    blocking_flag*/)
{
    cBuffer_APDUHeader_t packet;
    BOOL DataStored  = FALSE;
    U32  end_free_bytes;
    U32  start_free_bytes;
    U32  RealDataSize;
    U32  total_bytes; 

    /* Allign data size on a 32-bit word boundary */
 
    RealDataSize = ((apdu_size + 0x3) & ~0x3);

    /* The total number of bytes allocated for the block of data is 
     * the size of the block aligned to a word boundary + 
     * CBUFFER_APDU_HEADER_SIZE bytes to store control packet data.
     */

    total_bytes = RealDataSize + CBUFFER_APDU_HEADER_SIZE;

    /* Check if packet can be stored in buffer */

    if (total_bytes > cbuffer_p->BufferSize)
    {
        return (CBUFFER_ERROR_NOT_ENOUGHT_SPACE);
    }

    while ((DataStored == FALSE) && (cbuffer_p->DataInvalidated == FALSE))
    {
        /* --- Start critical section --- */

        semaphore_wait(cbuffer_p->Mutex_p);

        /* Is there enought space available? */

        if (((cbuffer_p->NumBytesStored + total_bytes)<=cbuffer_p->BufferSize))
        {
          /* --- There is enough free space: check if it is adjacent --- */

          if ((U32)cbuffer_p->Head <= (U32)cbuffer_p->Tail)
          {
              end_free_bytes = (U32)cbuffer_p->EndBuffer - (U32)cbuffer_p->Tail;
              start_free_bytes =(U32)cbuffer_p->Head - (U32)cbuffer_p->Buffer_p;

              if (end_free_bytes >= total_bytes)
              {
                  /* Add header of data block as first bytes of block */

                  packet.apdu_tag = apdu_tag;
                  packet.apdu_size = apdu_size;
                  packet.dummy_flag = CBUFFER_FALSE;
                  packet.packet_status = packet_status;
                  packet.reset_flag = reset_flag;

                  memcpy(cbuffer_p->Tail, &packet, CBUFFER_APDU_HEADER_SIZE);
                  cbuffer_p->Tail = (void*)((U8*)cbuffer_p->Tail +
                                            CBUFFER_APDU_HEADER_SIZE);
                  /* Insert Packet */
                  memcpy(cbuffer_p->Tail, apdu_data, apdu_size);
                  if (end_free_bytes == total_bytes)
                       cbuffer_p->Tail = cbuffer_p->Buffer_p;
                  else cbuffer_p->Tail = (void*)((U8*)cbuffer_p->Tail 
                                                + RealDataSize);
                  DataStored = TRUE;
              }
              else if (start_free_bytes >= total_bytes)
              {
                  /* space at the end of the buffer is not sufficient.
                   * but there is enough space at the beginning.
                   */

                  /* fill free space at end of buffer with a dummy packet */
    
                  packet.apdu_tag = 0;
                  packet.apdu_size = end_free_bytes -CBUFFER_APDU_HEADER_SIZE;
                  packet.dummy_flag = CBUFFER_TRUE;
                  packet.packet_status = CBUFFER_FALSE;
                  packet.reset_flag = CBUFFER_FALSE;


                  memcpy(cbuffer_p->Tail, &packet, CBUFFER_APDU_HEADER_SIZE);
                  /*cbuffer_p->Tail = cbuffer_p->Buffer_p; not necessary */

                  cbuffer_p->NumBytesStored += end_free_bytes ;

                  /* insert packet at the beginning of buffer */

                  packet.apdu_tag = apdu_tag;
                  packet.apdu_size = apdu_size;
                  packet.dummy_flag = CBUFFER_FALSE;
                  packet.packet_status = packet_status;
                  packet.reset_flag = reset_flag;

                  memcpy(cbuffer_p->Buffer_p, &packet, 
                                            CBUFFER_APDU_HEADER_SIZE);
                  cbuffer_p->Tail = (void*)((U8*)cbuffer_p->Buffer_p +
                                            CBUFFER_APDU_HEADER_SIZE);
                  memcpy(cbuffer_p->Tail, apdu_data, apdu_size);
                  cbuffer_p->Tail = (void*)((U8*)cbuffer_p->Tail+RealDataSize);

                  DataStored = TRUE;
              }
          }
          else if (((U32)cbuffer_p->Head - (U32)cbuffer_p->Tail) >= total_bytes)
          {
              /* Add header of data block as first bytes of block */

              packet.apdu_tag = apdu_tag;
              packet.apdu_size = apdu_size;
              packet.dummy_flag = CBUFFER_FALSE;
              packet.packet_status = packet_status;
              packet.reset_flag = reset_flag;


              memcpy(cbuffer_p->Tail, &packet, CBUFFER_APDU_HEADER_SIZE);
              cbuffer_p->Tail = (void*)((U8*)cbuffer_p->Tail +
                                        CBUFFER_APDU_HEADER_SIZE);
              /* Insert Packet */
              memcpy(cbuffer_p->Tail, apdu_data, apdu_size);
              cbuffer_p->Tail = (void*)((U8*)cbuffer_p->Tail + RealDataSize);
              DataStored = TRUE;
          }

          /* Preventive signal */

          if (DataStored == TRUE)
          {
              cbuffer_p->NumBytesStored += total_bytes ;
              semaphore_signal(cbuffer_p->SpaceAvailable_p);
          }
        }

        /* --- End of critical section --- */

        semaphore_signal(cbuffer_p->Mutex_p);

        /* Suspend the task if not enought space was available */

        semaphore_wait(cbuffer_p->SpaceAvailable_p);

    }  /* end while */

    /* Possibly wake up a task which is waiting for data */ 

    semaphore_signal(cbuffer_p->DataAvailable_p);

    if (cbuffer_p->DataInvalidated == TRUE)
    {
         return (CBUFFER_ERROR_INVALID_DATA);
    }
    else return (ST_NO_ERROR);
}


/******************************************************************************\
 * Function: cBuffer_CopyAPDU
 * Purpose : Read and Remove a packet from the head of cbuffer_p;
 *           Data is copied into a provided buffer.
 *           If the buffer is empty the calling task is suspended until new data
 *           is inserted.
 * Parameters:
 *      data_size:
 *           Returned number of bytes available for the current data packet.
 *      buffer: 
 *           A pointer to the buffer where to copy data
 *      buffer_size:
 *           Size in bytes of buffer.
 * Return  : 
 *           - ST_NO_ERROR on success,
 *           - CBUFFER_ERROR_NOT_ENOUGHT_SPACE if buffer is not big enought,
 *           - CBUFFER_ERROR_INVALID_DATA if buffer data was invalid, 
 *                                        e.g. after a buffer flush
 *           - CBUFFER_GENERIC_ERROR on generic failure
\******************************************************************************/
 
ST_ErrorCode_t cBuffer_CopyAPDU (cBuffer_t *cbuffer_p, 
                                 U32  *data_tag,
                                 U32  *data_size,
                                 void *buffer, 
                                 U32   buffer_size)
{
    cBuffer_APDUHeader_t packet;
    BOOL GotData = FALSE;
    U32  RealDataSize;
    U32  nBytesRead;
    ST_ErrorCode_t res = ST_NO_ERROR;
 
    while ((GotData == FALSE) && (cbuffer_p->DataInvalidated == FALSE))
    {
        /* --- Start of critical section --- */
 
        semaphore_wait(cbuffer_p->Mutex_p);
 
        if (cbuffer_p->DataOverWritten == TRUE)
        {
            res = CBUFFER_ERROR_PACKET_OVERWRITTEN;
        }

        /* reset overwritten data flag */

        cbuffer_p->DataOverWritten = FALSE;

        if (cbuffer_p->NumBytesStored > 0)
        {
          /* Check if first available packet is a dummy Packet */ 

          packet = *(cBuffer_APDUHeader_t*)cbuffer_p->Head;
          if (packet.dummy_flag == CBUFFER_TRUE)
          {
            /*  Skip dummy packet  */

            RealDataSize = ((packet.apdu_size + 0x3) & ~0x3);
            cbuffer_p->Head = (void*)cbuffer_p->Buffer_p;
            cbuffer_p->NumBytesStored -= ( RealDataSize 
                                         + CBUFFER_APDU_HEADER_SIZE);
          }

          if (cbuffer_p->NumBytesStored > 0)
          {
              /* First bytes contains control packet data */
              /* Compute size of data packet and fetch data */
    
              packet = *(cBuffer_APDUHeader_t*)cbuffer_p->Head;
    
              *data_tag = packet.apdu_tag;
              *data_size = packet.apdu_size;

              /* If buffer size is smaller than packet size, then */
              /* we can read just buffer_size of data */
    
              nBytesRead = packet.apdu_size;
              if (buffer_size < packet.apdu_size)
              {
                  nBytesRead = buffer_size;
                  res = CBUFFER_ERROR_NOT_ENOUGHT_SPACE;
              }
    
              /* Compute real packet size.
               * It is the next address aligned on a 32-bit word boundary 
               */
    
              RealDataSize = ((packet.apdu_size + 0x3) & ~0x3);
    
              cbuffer_p->Head = (void*)((U8*)cbuffer_p->Head +
                                        CBUFFER_APDU_HEADER_SIZE);
              
              memcpy(buffer, cbuffer_p->Head, nBytesRead);
              cbuffer_p->Head = (void*)((U8*)cbuffer_p->Head + RealDataSize);
              if (cbuffer_p->Head >= cbuffer_p->EndBuffer)
                  cbuffer_p->Head = cbuffer_p->Buffer_p;
              
              cbuffer_p->NumBytesStored -= (RealDataSize +
                                            CBUFFER_APDU_HEADER_SIZE);
              if (cbuffer_p->NumBytesStored == 0)
              {
                  cbuffer_p->Head = cbuffer_p->Buffer_p;
                  cbuffer_p->Tail = cbuffer_p->Buffer_p;
              }
    
              /* Preventive signal */
              semaphore_signal(cbuffer_p->DataAvailable_p);
    
              /* reset overwritten data flag */
              cbuffer_p->DataOverWritten = FALSE;

              GotData = TRUE;
          }
        }
 
        /* --- End of critical section --- */
 
        semaphore_signal(cbuffer_p->Mutex_p);
 
        /* Suspend the task if data was not available */
        semaphore_wait(cbuffer_p->DataAvailable_p);
 
    } /* end while */
 
    /* Possibly wake up a task which is waiting to insert  */

    semaphore_signal(cbuffer_p->SpaceAvailable_p);
        
    if (cbuffer_p->DataInvalidated == TRUE)
    {
         *data_tag = 0;
         *data_size = 0;
         return (CBUFFER_ERROR_INVALID_DATA);
    }
    return (res);
}


/******************************************************************************\
 * Function: cBuffer_PeekAPDU
 * Purpose : Extracts information about next available APDU packet 
 *           in the circular buffer. 
 *           Calling task may be suspended waiting for data.
 * Parameters:
 *       data_size:
 *           Returned number of bytes of the data packet
 *       data_pointer:
 *           Returned pointer to the packet data
 *       packet_status:
 *           tell if packet data is single, more or last. 
 *       reset_flag:
 *           instruct the decoder to reset the working buffer
 * Return  : ST_NO_ERROR on success,
 *           CBUFFER_ERROR_INVALID_DATA if buffer data was invalid,
 *           e.g. after a buffer flush
 * Note    : The pointers to the head and the tail of the buffer are not updated
 *           cBuffer_PeekAPDU can be used togheter with cBuffer_Remove
 *           in order to update pointers. (Be worry in case more consumers tasks
 *           are accessing the buffer).
\******************************************************************************/
 
ST_ErrorCode_t cBuffer_PeekAPDU (cBuffer_t *cbuffer_p,
                             U32   *data_tag,
                             U32   *data_size,
                             void **data_pointer,
                             U8    *packet_status,
                             U8    *reset_flag)
{
    cBuffer_APDUHeader_t packet;
    BOOL PeekedData = FALSE;
 
    while ((PeekedData == FALSE) && (cbuffer_p->DataInvalidated == FALSE))
    {
        /* --- Start of critical section --- */
 
        semaphore_wait(cbuffer_p->Mutex_p);
 
        if (cbuffer_p->NumBytesStored > 0)
        {
            /* Check if first available packet is a dummy Packet */ 

            packet = *(cBuffer_APDUHeader_t*)cbuffer_p->Head;
            if (packet.dummy_flag == CBUFFER_TRUE)
            {
                /*  Skip dummy packet  */

                cbuffer_p->Head = (void*)cbuffer_p->Buffer_p;
                cbuffer_p->NumBytesStored -= (((packet.apdu_size + 0x3) & ~0x3)
                                              + CBUFFER_APDU_HEADER_SIZE);
            }

            if (cbuffer_p->NumBytesStored > 0)
            {
                /* First bytes contains control packet data */
                /* Compute size of data packet and fetch data */
      
                packet = *(cBuffer_APDUHeader_t*)cbuffer_p->Head;
      
                *data_tag = packet.apdu_tag;
                *data_size = packet.apdu_size;
                *packet_status = packet.packet_status;
                *reset_flag = packet.reset_flag;
                *data_pointer = (void*)((U8*)cbuffer_p->Head +
                                        CBUFFER_APDU_HEADER_SIZE);
      
                /* Preventive signal */
                semaphore_signal(cbuffer_p->DataAvailable_p);
      
                PeekedData = TRUE;
            }
        }
 
        /* --- End of critical section --- */
 
        semaphore_signal(cbuffer_p->Mutex_p);
 
        /* Suspend the task if data was not available */
        semaphore_wait(cbuffer_p->DataAvailable_p);
 
    } /* end while */
 
    if (cbuffer_p->DataInvalidated == TRUE)
    {
        *data_tag = 0;
        *data_size = 0;
        *data_pointer = NULL; 
        *packet_status = FALSE;
        *reset_flag = FALSE;

        return (CBUFFER_ERROR_INVALID_DATA);
    }
    return (ST_NO_ERROR);
}


/******************************************************************************\
 * Function: cBuffer_RemoveAPDU
 * Purpose : Removes an APDU packet from the head of the buffer.
 *           If no data is available then returns immediately.
 *           It is likely to be used after cBuffer_Peek in order to update
 *           the head and tail pointers.
 * Return  : ST_NO_ERROR on success,
 *           CBUFFER_ERROR_INVALID_DATA if buffer data was invalid,
 *           e.g. after a buffer flush
\******************************************************************************/

ST_ErrorCode_t cBuffer_RemoveAPDU (cBuffer_t *cbuffer_p)
{
    cBuffer_APDUHeader_t packet;
    U32 RealDataSize;
    BOOL RemovedData = FALSE;

    /* --- Start of critical section --- */

    semaphore_wait(cbuffer_p->Mutex_p);

    if ((cbuffer_p->NumBytesStored > 0) && (cbuffer_p->DataInvalidated==FALSE))
    {
        /* Check if first available packet is a dummy Packet */ 

        packet = *(cBuffer_APDUHeader_t*)cbuffer_p->Head;
        if (packet.dummy_flag == CBUFFER_TRUE)
        {
            /*  Skip dummy packet  */

            RealDataSize = ((packet.apdu_size + 0x3) & ~0x3);
            cbuffer_p->Head = (void*)cbuffer_p->Buffer_p;
            cbuffer_p->NumBytesStored -= ( RealDataSize 
                                         + CBUFFER_APDU_HEADER_SIZE);
        }

        if (cbuffer_p->NumBytesStored > 0)
        {
            /* First bytes contains control packet data */
            /* Compute size of data packet and fetch data */
   
            packet = *(cBuffer_APDUHeader_t*)cbuffer_p->Head;
  
            /* Compute real packet size.
             * It is the next address aligned on a 32-bit word boundary 
             */
  
            RealDataSize = ((packet.apdu_size + 0x3) & ~0x3);
  
            cbuffer_p->Head = (void*)((U8*)cbuffer_p->Head 
                              + CBUFFER_APDU_HEADER_SIZE + RealDataSize);
            
            if (cbuffer_p->Head >= cbuffer_p->EndBuffer)
                cbuffer_p->Head = cbuffer_p->Buffer_p;
            
            cbuffer_p->NumBytesStored -= (RealDataSize +
                                          CBUFFER_APDU_HEADER_SIZE);
            if (cbuffer_p->NumBytesStored == 0)
            {
                cbuffer_p->Head = cbuffer_p->Buffer_p;
                cbuffer_p->Tail = cbuffer_p->Buffer_p;
            }
        }
        RemovedData = TRUE;
    }

    /* --- End of critical section --- */

    semaphore_signal(cbuffer_p->Mutex_p);
 
    /* Possibly wake up a task which is waiting to insert  */

    if (RemovedData == TRUE)
        semaphore_signal(cbuffer_p->SpaceAvailable_p);

    if (cbuffer_p->DataInvalidated == TRUE)
         return (CBUFFER_ERROR_INVALID_DATA);
    else return (ST_NO_ERROR);
}


/******************************************************************************\
 * Function: cBuffer_GetReplyAPDU
 * Purpose : Get APDU packet from the buffer. If no packet is available,
 *           immediatelly returns.
 * Parameters:
 *       apdu_tag:
 *           Returned apdu tag 
 *       apdu_size:
 *           Returned number of bytes of the data packet
 *       apdu_data:
 *           Returned pointer to the packet data
 * Return  : ST_NO_ERROR on success,
 *           CBUFFER_ERROR_INVALID_DATA 
 *           CBUFFER_ERROR_NO_PACKET_AVAILABLE 
 *           CBUFFER_ERROR_PACKET_OVERWRITTEN 
\******************************************************************************/
 
ST_ErrorCode_t cBuffer_GetReplyAPDU (cBuffer_t *cbuffer_p,
                                     U32       *apdu_tag,
                                     U32       *apdu_size,
                                     U32       *apdu_data) 
{
    cBuffer_APDUHeader_t packet;
    U32 PacketDataSize;
    ST_ErrorCode_t res = ST_NO_ERROR;
 
    *apdu_tag  = 0;
    *apdu_size = 0;
    *apdu_data = 0;

    /* --- Start of critical section --- */
 
    semaphore_wait(cbuffer_p->Mutex_p);

    if (cbuffer_p->DataOverWritten == TRUE)
    {
        res = CBUFFER_ERROR_PACKET_OVERWRITTEN;
    }

    /* reset overwritten data flag */

    cbuffer_p->DataOverWritten = FALSE;
 
    if (cbuffer_p->NumBytesStored > 0)
    {
        /* Check if first available packet is a dummy Packet */ 

        packet = *(cBuffer_APDUHeader_t*)cbuffer_p->Head;
        if (packet.dummy_flag == CBUFFER_TRUE)
        {
            /*  Skip dummy packet  */
            cbuffer_p->Head = (void*)cbuffer_p->Buffer_p;
            cbuffer_p->NumBytesStored -= ( ((packet.apdu_size + 0x3) & ~0x3)
                                       + CBUFFER_APDU_HEADER_SIZE);
        }

        if (cbuffer_p->NumBytesStored > 0)
        {
            /* First bytes contains control packet data */
            /* Compute size of data packet and fetch data */
  
            packet = *(cBuffer_APDUHeader_t*)cbuffer_p->Head;
  
            *apdu_tag  = packet.apdu_tag;
            *apdu_size = packet.apdu_size;
            cbuffer_p->Head = (void*)((U8*)cbuffer_p->Head +
                                      CBUFFER_APDU_HEADER_SIZE);

            memcpy (apdu_data, cbuffer_p->Head, packet.apdu_size);

            PacketDataSize = ((packet.apdu_size + 0x3) & ~0x3);
            cbuffer_p->Head = (void*)((U8*)cbuffer_p->Head + PacketDataSize);
            if (cbuffer_p->Head >= cbuffer_p->EndBuffer)
                cbuffer_p->Head  = cbuffer_p->Buffer_p;
            cbuffer_p->NumBytesStored -= (PacketDataSize +
                                          CBUFFER_APDU_HEADER_SIZE);
            if (cbuffer_p->NumBytesStored == 0)
            {
                cbuffer_p->Head = cbuffer_p->Buffer_p;
                cbuffer_p->Tail = cbuffer_p->Buffer_p;
            }
        }
    }
    else
    {
        res = CBUFFER_ERROR_NO_PACKET_AVAILABLE;
    }

    if (cbuffer_p->DataInvalidated == TRUE)
    {
        *apdu_tag  = 0;
        *apdu_size = 0;
        *apdu_data = 0;
        res = CBUFFER_ERROR_INVALID_DATA;
    }

    /* --- End of critical section --- */
 
    semaphore_signal(cbuffer_p->Mutex_p);
 
    return (res);
}

/******************************************************************************\
 * Function: cBuffer_InsertAPDU_ow
 * Purpose : Insert an APDU into the tail of the buffer
 *           If the buffer is full, it removes packets making room for new data.
 * Return  : - ST_NO_ERROR on success,
 *           - CBUFFER_ERROR_NOT_ENOUGHT_SPACE if cbuffer was not big enought,
 *           - CBUFFER_ERROR_INVALID_DATA if data buffer was invalid,
 *                                        e.g. after a buffer flush
 *           - CBUFFER_GENERIC_ERROR on generic failure
\******************************************************************************/

ST_ErrorCode_t cBuffer_InsertAPDU_ow (
                             cBuffer_t *cbuffer_p,
                             U32   apdu_tag,
                             U32   apdu_size,
                             void *apdu_data)
{
    cBuffer_APDUHeader_t packet;
    BOOL DataStored  = FALSE;
    U32  end_free_bytes;
    U32  start_free_bytes;
    U32  RealDataSize;
    U32  DataSize;
    U32  total_bytes; 

    /* Allign data size on a 32-bit word boundary */
 
    RealDataSize = ((apdu_size + 0x3) & ~0x3);

    /* The total number of bytes allocated for the block of data is 
     * the size of the block aligned to a word boundary + 
     * CBUFFER_APDU_HEADER_SIZE bytes to store control packet data.
     */

    total_bytes = RealDataSize + CBUFFER_APDU_HEADER_SIZE;

    /* Check if packet can be stored in buffer */

    if (total_bytes > cbuffer_p->BufferSize)
    {
        return (CBUFFER_ERROR_NOT_ENOUGHT_SPACE);
    }

    /* --- Start critical section --- */

    semaphore_wait(cbuffer_p->Mutex_p);

    do
    {
        /* Is there enought space available? */

        if (((cbuffer_p->NumBytesStored + total_bytes)<=cbuffer_p->BufferSize))
        {
          /* --- There is enough free space: check if it is adjacent --- */

          if ((U32)cbuffer_p->Head <= (U32)cbuffer_p->Tail)
          {
              end_free_bytes = (U32)cbuffer_p->EndBuffer - (U32)cbuffer_p->Tail;
              start_free_bytes =(U32)cbuffer_p->Head - (U32)cbuffer_p->Buffer_p;

              if (end_free_bytes >= total_bytes)
              {
                  /* Add header of data block as first bytes of block */

                  packet.apdu_tag = apdu_tag;
                  packet.apdu_size = apdu_size;
                  packet.dummy_flag = CBUFFER_FALSE;

                  memcpy(cbuffer_p->Tail, &packet, CBUFFER_APDU_HEADER_SIZE);
                  cbuffer_p->Tail = (void*)((U8*)cbuffer_p->Tail +
                                            CBUFFER_APDU_HEADER_SIZE);
                  /* Insert Packet */
                  memcpy(cbuffer_p->Tail, apdu_data, apdu_size);
                  if (end_free_bytes == total_bytes)
                       cbuffer_p->Tail = cbuffer_p->Buffer_p;
                  else cbuffer_p->Tail = (void*)((U8*)cbuffer_p->Tail 
                                                + RealDataSize);
                  DataStored = TRUE;
              }
              else if (start_free_bytes >= total_bytes)
              {
                  /* space at the end of the buffer is not sufficient.
                   * but there is enough space at the beginning.
                   */

                  /* fill free space at end of buffer with a dummy packet */
    
                  packet.apdu_tag = 0;
                  packet.apdu_size = end_free_bytes -CBUFFER_APDU_HEADER_SIZE;
                  packet.dummy_flag = CBUFFER_TRUE;

                  memcpy(cbuffer_p->Tail, &packet, CBUFFER_APDU_HEADER_SIZE);
                  /*cbuffer_p->Tail = cbuffer_p->Buffer_p; not necessary */

                  cbuffer_p->NumBytesStored += end_free_bytes ;

                  /* insert packet at the beginning of buffer */

                  packet.apdu_tag = apdu_tag;
                  packet.apdu_size = apdu_size;
                  packet.dummy_flag = CBUFFER_FALSE;

                  memcpy(cbuffer_p->Buffer_p, &packet, 
                                            CBUFFER_APDU_HEADER_SIZE);
                  cbuffer_p->Tail = (void*)((U8*)cbuffer_p->Buffer_p +
                                            CBUFFER_APDU_HEADER_SIZE);
                  memcpy(cbuffer_p->Tail, apdu_data, apdu_size);
                  cbuffer_p->Tail = (void*)((U8*)cbuffer_p->Tail+RealDataSize);

                  DataStored = TRUE;
              }
          }
          else if (((U32)cbuffer_p->Head - (U32)cbuffer_p->Tail) >= total_bytes)
          {
              /* Add header of data block as first bytes of block */

              packet.apdu_tag = apdu_tag;
              packet.apdu_size = apdu_size;
              packet.dummy_flag = CBUFFER_FALSE;

              memcpy(cbuffer_p->Tail, &packet, CBUFFER_APDU_HEADER_SIZE);
              cbuffer_p->Tail = (void*)((U8*)cbuffer_p->Tail +
                                        CBUFFER_APDU_HEADER_SIZE);
              /* Insert Packet */
              memcpy(cbuffer_p->Tail, apdu_data, apdu_size);
              cbuffer_p->Tail = (void*)((U8*)cbuffer_p->Tail + RealDataSize);
              DataStored = TRUE;
          }
        }

        /* if packet has been not stored then remove the oldest in the buffer */

        if (DataStored == FALSE)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_INFO,
						"cBuffer_InsertAPDU_ow: removed oldest packet in buffer"));

            /* Check if first available packet is a dummy Packet */
    
            packet = *(cBuffer_APDUHeader_t*)cbuffer_p->Head;
            if (packet.dummy_flag == CBUFFER_TRUE)
            {
                DataSize = ((packet.apdu_size + 0x3) & ~0x3);
                cbuffer_p->Head = (void*)cbuffer_p->Buffer_p;
                cbuffer_p->NumBytesStored -= ( DataSize
                                             + CBUFFER_APDU_HEADER_SIZE);
            }
    
            if (cbuffer_p->NumBytesStored > 0)
            {
                packet = *(cBuffer_APDUHeader_t*)cbuffer_p->Head;
                DataSize = ((packet.apdu_size + 0x3) & ~0x3);
                cbuffer_p->Head = (void*)((U8*)cbuffer_p->Head
                                  + CBUFFER_APDU_HEADER_SIZE + DataSize);
                if (cbuffer_p->Head >= cbuffer_p->EndBuffer)
                    cbuffer_p->Head = cbuffer_p->Buffer_p;
                cbuffer_p->NumBytesStored -= (DataSize +
                                              CBUFFER_APDU_HEADER_SIZE);
                if (cbuffer_p->NumBytesStored == 0)
                {
                    cbuffer_p->Head = cbuffer_p->Buffer_p;
                    cbuffer_p->Tail = cbuffer_p->Buffer_p;
                }
                /* set overwritten data flag */
                cbuffer_p->DataOverWritten = TRUE;
            }
        }
        else
        {
            cbuffer_p->NumBytesStored += total_bytes ;
            break;
        }

    }   while ((DataStored == FALSE) && (cbuffer_p->DataInvalidated == FALSE));

    /* --- End of critical section --- */

    semaphore_signal(cbuffer_p->Mutex_p);

    /* Possibly wake up a task which is waiting for data */ 

    semaphore_signal(cbuffer_p->DataAvailable_p);

    if (cbuffer_p->DataInvalidated == TRUE)
    {
         return (CBUFFER_ERROR_INVALID_DATA);
    }
    else return (ST_NO_ERROR);
}
