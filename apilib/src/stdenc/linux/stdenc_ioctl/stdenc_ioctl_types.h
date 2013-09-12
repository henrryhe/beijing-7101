/*****************************************************************************
 *
 *  Module      : stdenc_ioctl
 *  Date        : 21-10-2005
 *  Author      : AYARITAR
 *  Description :
 *  Copyright   : STMicroelectronics (c)2005
 *****************************************************************************/


#ifndef STDENC_IOCTL_TYPES_H
#define STDENC_IOCTL_TYPES_H

/*** This is the node data structure ***/

/*
 *  The node data structure holds the private data for each device
 *  (minor number) managed by the module. The private data is used
 *  to control (manage) access to the device
 */
#define STDENC_IOCTL_LINUX_DEVICE_NAME        "stdenc_ioctl"


#define DATA_BUFFER_SIZE 64 /* The size of the internal buffer for the node */

typedef struct {

    struct semaphore      lock;        /* Lock for the device        */
    wait_queue_head_t     read_queue;  /* Wait for data to read      */
    wait_queue_head_t     write_queue; /* Wait for space to write to */

    char                  buffer[DATA_BUFFER_SIZE];    /* Internal  data buffer          */
    int                   read_idx;                    /* The read  index for the buffer */
    int                   write_idx;                   /* The write index for the buffer */

}
stdenc_ioctl_dev_t;



/*** Manage the private data (per minor) ***/

extern struct semaphore stdenc_ioctl_lock; /* Global lock for the module */

stdenc_ioctl_dev_t *stdenc_ioctl_NewNode(unsigned int minor);
stdenc_ioctl_dev_t *stdenc_ioctl_GetNode(unsigned int minor);
void              stdenc_ioctl_CleanUp(void);

#endif
