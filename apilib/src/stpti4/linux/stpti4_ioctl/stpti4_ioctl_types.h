/*****************************************************************************
 * COPYRIGHT (C) STMicroelectronics 2005.  All rights reserved.
 *
 *  Module      : stpti4_ioctl
 *  Date        : 17-04-2005
 *  Author      : STIEGLITZP
 *  Description :
 *
 *****************************************************************************/


#ifndef STPTI4_IOCTL_TYPES_H
#define STPTI4_IOCTL_TYPES_H

/*** This is the node data structure ***/

/*
 *  The node data structure holds the private data for each device
 *  (minor number) managed by the module. The private data is used
 *  to control (manage) access to the device
 */

                                                    
#define DATA_BUFFER_SIZE 64 /* The size of the internal buffer for the node */

typedef struct {
    struct semaphore      lock;        /* Lock for the device        */
}
stpti4_ioctl_dev_t;
                                                     


/*** Manage the private data (per minor) ***/

extern struct semaphore stpti4_ioctl_lock; /* Global lock for the module */

stpti4_ioctl_dev_t *stpti4_ioctl_NewNode(unsigned int minor);
stpti4_ioctl_dev_t *stpti4_ioctl_GetNode(unsigned int minor);
void              stpti4_ioctl_CleanUp(void);

#endif
