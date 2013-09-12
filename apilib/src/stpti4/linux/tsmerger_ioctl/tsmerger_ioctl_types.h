/*****************************************************************************
 *
 *  Module      : tsmerger_ioctl
 *  Date        : 24-07-2005
 *  Author      : STIEGLITZP
 *  Description :
 *  Copyright   : STMicroelectronics (c)2005 
 *****************************************************************************/


#ifndef TSMERGER_IOCTL_TYPES_H
#define TSMERGER_IOCTL_TYPES_H

#undef PDEBUG             /* undef it, just in case */
#ifdef STPTI_DEBUG
#  ifdef __KERNEL__
     /* This one if debugging is on, and kernel space */
#    define PDEBUG(fmt, args...) printk( KERN_ALERT "stpti: " fmt, ## args)
#  else
     /* This one for user space */
#    define PDEBUG(fmt, args...) fprintf(stderr, fmt, ## args)
#  endif
#else
#  define PDEBUG(fmt, args...) /* not debugging: nothing */
#endif

#undef PDEBUGG
#define PDEBUGG(fmt, args...) /* nothing: it's a placeholder */

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
tsmerger_ioctl_dev_t;
                                                     


/*** Manage the private data (per minor) ***/

extern struct semaphore tsmerger_ioctl_lock; /* Global lock for the module */

tsmerger_ioctl_dev_t *tsmerger_ioctl_NewNode(unsigned int minor);
tsmerger_ioctl_dev_t *tsmerger_ioctl_GetNode(unsigned int minor);
void              tsmerger_ioctl_CleanUp(void);

#endif
