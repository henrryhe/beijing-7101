/*******************************************************************************

File Name   : cbuffer.h

Description : Definition of a Circular buffer data structure

Author : Marino Congiu - Dec 1998

Modified : Alessandro Innocenti - Nov 1999 for DVB CI extension.

Copyright STMicroelectronics - November 1998

*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __CBUFFER_H
#define __CBUFFER_H

#include <semaphor.h>
#include <stddefs.h>

#define CBUFFER_FALSE 	0
#define CBUFFER_TRUE 	1

#define CBUFFER_SINGLE 	0
#define CBUFFER_MORE 	1
#define CBUFFER_LAST 	2

#define CBUFFER_NO_ERROR			0
#define CBUFFER_GENERIC_ERROR			1
#define CBUFFER_ERROR_INVALID_DATA		2
#define CBUFFER_ERROR_NOT_ENOUGHT_SPACE		3
#define CBUFFER_ERROR_NO_PACKET_AVAILABLE	4
#define CBUFFER_ERROR_PACKET_OVERWRITTEN	5

/* Defines a Circular Buffer data structure */

typedef struct cBuffer_s {
    ST_Partition_t *MemoryPartition;
    U32             BufferSize;
    U32             NumBytesStored;
    void           *Buffer_p ;
    void           *Head ;
    void           *Tail ;
    void           *EndBuffer ;
    semaphore_t    *SpaceAvailable_p ;
    semaphore_t    *DataAvailable_p ;
    semaphore_t    *Mutex_p ;
    BOOL            DataInvalidated;
    BOOL            DataOverWritten;
} cBuffer_t ;

/* Defines a simple packet header */

typedef struct {
  U32 size;
  U32 dummy_flag;
} cBuffer_PacketHeader_t;

/* Defines a simple packet header */

typedef struct {
  U32 apdu_tag;
  U32 apdu_size;
  U8  dummy_flag;
  U8  packet_status;
  U8 reset_flag;
  U8 reserved;
} cBuffer_APDUHeader_t;

#define CBUFFER_PACKET_HEADER_SIZE  sizeof(cBuffer_PacketHeader_t)
#define CBUFFER_APDU_HEADER_SIZE    sizeof(cBuffer_APDUHeader_t)


/* 
===================================
Circular Buffer: Exported Functions 
===================================
*/

/* Create a circular buffer */
cBuffer_t * cBuffer_Create (ST_Partition_t *MemoryPartition,
                            U32 BufferSize, U32 *RealBufferSize);

/* Delete a circular buffer */
void cBuffer_Delete (cBuffer_t *cbuffer_p);

/* Insert nbytes of data into the tail of the buffer */
ST_ErrorCode_t cBuffer_Insert (
                             cBuffer_t *cbuffer_p,
                             void *data_p,
                             U32 nbytes);


/* Copy (and remove) data from the head of the circular buffer into a buffer */
ST_ErrorCode_t cBuffer_Copy (cBuffer_t *cbuffer_p, U32 *data_size, 
                             void * buffer, U32 buffer_size);

/* Get length and location of next packet of data in the buffer */
ST_ErrorCode_t cBuffer_Peek (cBuffer_t *cbuffer_p,
                             U32   *data_size,
                             void **data_pointer);

/* Remove a packet of data from the head of the buffer */
ST_ErrorCode_t cBuffer_Remove(cBuffer_t *cbuffer_p);

/* Insert an APDU into the tail of the buffer */
ST_ErrorCode_t cBuffer_InsertAPDU (
                             cBuffer_t *cbuffer_p,
                             U32   apdu_tag,
                             U32   apdu_length,
                             void *apdu_data,
                             U8    packet_status,
                             U8    reset_flag);

/* Copy (and remove) APDU from the head of the circular buffer into a buffer */
ST_ErrorCode_t cBuffer_CopyAPDU (
                             cBuffer_t *cbuffer_p, 
                             U32       *apdu_tag,
                             U32       *apdu_length,
                             void      *buffer, U32 buffer_size);

/* Get length and location of next APDU in the buffer */
ST_ErrorCode_t cBuffer_PeekAPDU (cBuffer_t *cbuffer_p,
                             U32   *apdu_tag,
                             U32   *data_size,
                             void **apdu_data,
                             U8    *packet_status,
                             U8    *reset_flag);

/* Get an APDU from the buffer, immediatelly returns if data is not available */
ST_ErrorCode_t cBuffer_GetReplyAPDU (cBuffer_t *cbuffer_p,
                                     U32       *apdu_tag,
                                     U32       *apdu_length,
                                     U32       *apdu_data);

/* Remove an APDU from the head of the buffer */
ST_ErrorCode_t cBuffer_RemoveAPDU(cBuffer_t *cbuffer_p);

/* Insert an APDU into the tail of the buffer (overwriting mode) */
ST_ErrorCode_t cBuffer_InsertAPDU_ow (
                             cBuffer_t *cbuffer_p,
                             U32   apdu_tag,
                             U32   apdu_length,
                             void *apdu_data);

/* Flush the buffer */
void cBuffer_Flush (cBuffer_t *cbuffer_p);
void cBuffer_FlushOutProducer (cBuffer_t *cbuffer_p);
void cBuffer_FlushOutConsumer (cBuffer_t *cbuffer_p);

/* Reset the buffer */
void cBuffer_Reset (cBuffer_t *cbuffer_p);

/* Determine if buffer is empty or full */
BOOL cBuffer_isEmpty (cBuffer_t *cbuffer_p);
BOOL cBuffer_isFull  (cBuffer_t *cbuffer_p);


#if defined (DVD_TRANSPORT_LINK)
ST_ErrorCode_t cBuffer_InsertSegmentFromCircular(void *Handle, U32   nbytes);
#endif

#endif  /* #ifndef __CBUFFER_H */
