/*****************************************************************************
 *
 *  Module      : staudlx_core_type.h
 *  Date        : 02-07-2005
 *  Author      : WALKERM
 *  Description :
 *  Copyright   : STMicroelectronics (c)2005 
 *****************************************************************************/


#ifndef STAUDLX_CORE_TYPES_H
#define STAUDLX_CORE_TYPES_H

/*** This is the node data structure ***/

/*
 *  The node data structure holds the private data for each device
 *  (minor number) managed by the module. The private data is used
 *  to control (manage) access to the device
 */

                                                    
#define DATA_BUFFER_SIZE 64 /* The size of the internal buffer for the node */

typedef struct {

    struct semaphore      lock;        /* Lock for the device        */
    wait_queue_head_t     read_queue;  /* Wait for data to read      */
    wait_queue_head_t     write_queue; /* Wait for space to write to */        
    
    char                  buffer[DATA_BUFFER_SIZE];    /* Internal  data buffer          */
    int                   read_idx;                    /* The read  index for the buffer */
    int                   write_idx;                   /* The write index for the buffer */
                                                       
}
staudlx_core_dev_t;
                                                     


/*** Manage the private data (per minor) ***/

extern struct semaphore staudlx_core_lock; /* Global lock for the module */

staudlx_core_dev_t *staudlx_core_NewNode(unsigned int minor);
staudlx_core_dev_t *staudlx_core_GetNode(unsigned int minor);
void              staudlx_core_CleanUp(void);

#endif
