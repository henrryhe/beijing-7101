/*****************************************************************************
 *
 *  Module      : stdenc_ioctl
 *  Date        : 21-10-2005
 *  Author      : AYARITAR
 *  Description :
 *  Copyright   : STMicroelectronics (c)2005 
 *****************************************************************************/

/* Requires   MODULE   defined on the command line */
/* Requires __KERNEL__ defined on the command line */
/* Requires __SMP__    defined on the command line for SMP systems */

#define _NO_VERSION_
#include <linux/module.h>  /* Module support */
#include <linux/version.h> /* Kernel version */
#include <linux/fs.h>      /* File operations (fops) defines */
#include <linux/ioport.h>  /* Memory/device locking macros   */
#include <asm/uaccess.h>   /* User space access methods      */

#include <linux/errno.h>   /* Defines standard error codes */

#include "stdenc_ioctl_types.h"       /* Modules specific defines */
#include "stos.h"

/*** PROTOTYPES **************************************************************/


#include "stdenc_ioctl_fops.h"

static void notify_data(struct file *filp);
static void notify_space(struct file *filp);

/*** LOCAL TYPES *********************************************************/


/*** LOCAL CONSTANTS ********************************************************/

/*** LOCAL VARIABLES *********************************************************/

/*** METHODS ****************************************************************/

/*=============================================================================

   stdenc_ioctl_read
   
   Pass data upto user space. This call can deshedule if there is no data
   available.
   
   This function can deschedule and MUST be reentrant.
   
  ===========================================================================*/
ssize_t stdenc_ioctl_read   (struct file *filp, char *buffer, size_t size, loff_t *offset)
{
    int err = 0;
    int transfer_size;
    int remaining_size;
                                                                                                                                                         
    stdenc_ioctl_dev_t *dev_p = (stdenc_ioctl_dev_t*)filp->private_data;                  
    
    /*** Get the lock ***/
    
    if (down_interruptible(&(dev_p->lock))) {
    
        /* Call interrupted */
        err = -ERESTARTSYS;
        goto fault;
    }
    
    /*** Read permittion is check by the kernel before calling here ***/
    
    if (filp->f_flags & O_NONBLOCK) {
    
        STTBX_Print((KERN_ALERT "Access methods are non-blocking\n"));
        
        if (dev_p->read_idx == dev_p->write_idx) {
        
            /* No data avaialable */
            err = -EAGAIN;
            goto fault_locked;
        }
    }
    else {
    
        STTBX_Print((KERN_ALERT "Read Access methods are blocking\n"));
        
        /* Release the lock */
        
        up(&(dev_p->lock));
        
        /* Wait for data */
        while (dev_p->read_idx == dev_p->write_idx) {
        
            /*** Block waiting for data ***/
            if (wait_event_interruptible(dev_p->read_queue, (dev_p->read_idx != dev_p->write_idx)))
            {
                /* Call interrupted */
                err = -ERESTARTSYS;
                goto fault;
            }
        }
    
        /* Get the lock */
    
        if (down_interruptible(&(dev_p->lock))) {
        
            /* Call interrupted */
            err = -ERESTARTSYS;
            goto fault;
        }
    }
    
    /*** Read the data ***/
    
    /* Calculate the transfer size */
    /* This does not wrap at the end of the buffer */
    /* A second read is needed to empty the buffer */
    
    transfer_size = (dev_p->write_idx < dev_p->read_idx)?(DATA_BUFFER_SIZE - dev_p->read_idx):(dev_p->write_idx - dev_p->read_idx);
    if (transfer_size > size) transfer_size = size;
    
    /* Do the transfer */
    
    remaining_size = copy_to_user(buffer, dev_p->buffer+dev_p->read_idx, transfer_size);
    
    /* Update the read pointer */
    
    dev_p->read_idx += transfer_size - remaining_size;
    dev_p->read_idx %= DATA_BUFFER_SIZE;
    
    /* Notify space in the buffer */
    
    if (transfer_size != remaining_size) notify_space(filp);
    
    /* Error handling for the transfer */
    
    if (remaining_size) {
    
        /* Only a partial transfer */
        err = -EFAULT;
        goto fault_locked;
    } 
    
    /*** Release the lock ***/
    
    up(&(dev_p->lock));
    
    return (transfer_size);  /* If we get here then we have succeeded */
    
    /**************************************************************************/
    
    /*** Clean up on error ***/
    
fault_locked:
    
    /* Release the lock */
    
    up(&(dev_p->lock));
    
fault:
    
    STTBX_Print((KERN_ALERT "Read : Exit with error %d\n", -err));
    return (err);
}



/*=============================================================================

   stdenc_ioctl_write
   
   Pass data down from user space. This call can deshedule if there is no space
   to store the data yet.
   
   This function can deschedule and MUST be reentrant.
   
  ===========================================================================*/
ssize_t stdenc_ioctl_write  (struct file *filp, const char *buffer, size_t size, loff_t *offset)
{
    int err = 0;
    int transfer_size;
    int remaining_size;
                                                                                     
    stdenc_ioctl_dev_t *dev_p = (stdenc_ioctl_dev_t*)filp->private_data;                 
    
    /*** Get the lock ***/
    
    if (down_interruptible(&(dev_p->lock))) {
    
        /* Call interrupted */
        err = -ERESTARTSYS;
        goto fault;
    }
    
    /*** Write permittion is checked by the kernel before calling here ***/
    
    if (filp->f_flags & O_NONBLOCK) {
    
        STTBX_Print((KERN_ALERT "Access methods are non-blocking\n"));
        
        if ((dev_p->write_idx+1)%DATA_BUFFER_SIZE == dev_p->read_idx) {
        
            /* No data avaialable */
            err = -EAGAIN;
            goto fault_locked;
        }
    }
    else
    {
        STTBX_Print((KERN_ALERT "Write Access methods are blocking\n"));
        
        /* Release the lock */
        
        up(&(dev_p->lock));
        
        /* Wait for space to write */
        
        while ((dev_p->write_idx+1)%DATA_BUFFER_SIZE == dev_p->read_idx) {
        
            /*** Block waiting for data ***/
            if (wait_event_interruptible(dev_p->write_queue, ((dev_p->write_idx+1)%DATA_BUFFER_SIZE != dev_p->read_idx))) {
            
                /* Call interrupted */
                err = -ERESTARTSYS;
                goto fault;
            }
        }
    
        /* Get the lock */

        if (down_interruptible(&(dev_p->lock))) {
        
            /* Call interrupted */
            err = -ERESTARTSYS;
            goto fault;
        }
    }
    
    /*** Write the data ***/
    
    /* Calculate the transfer size */
    /* This does not wrap at the end of the buffer */
    /* A second write is needed to fill the buffer */
    
    transfer_size  = (dev_p->write_idx >= dev_p->read_idx)?(DATA_BUFFER_SIZE - dev_p->write_idx):(dev_p->read_idx - dev_p->write_idx -1);
    if ((dev_p->write_idx+transfer_size)% DATA_BUFFER_SIZE == dev_p->read_idx) transfer_size -= 1;
    if (transfer_size >= size)             transfer_size = size;
    
    /* Transfer the data */
    
    remaining_size = copy_from_user(dev_p->buffer+dev_p->write_idx, buffer, transfer_size);
    
    /* Update the write pointer */
    
    dev_p->write_idx += transfer_size - remaining_size;
    dev_p->write_idx %= DATA_BUFFER_SIZE;
    
    /* Notify the presence of data to the read side */
    
    if (transfer_size != remaining_size) notify_data(filp);
    
    /* Error handling for the data transfer */
    
    if (remaining_size) {
    
        /* Only a partial transfer */
        err = -EFAULT;
        goto fault_locked;
    } 
    
    /*** Release the lock ***/
    
    up(&(dev_p->lock));
   
    return (transfer_size);  /* If we get here then we have succeeded */
    
    /**************************************************************************/
    
    /*** Clean up on error ***/
    
fault_locked:
    
    /* Release the lock */
    
    up(&(dev_p->lock));
    
fault:
    
    STTBX_Print((KERN_ALERT "Write : Exit with error %d\n", -err));
    return (err);
}


/*=============================================================================

   stdenc_ioctl_flush
   
   Block until all outstanding data for this file handle hs been sent.
   
   This is only really meeningful for network devices. This is not meaningful
   in the template driver and stdenc_ioctl_lseek should not be added to the fops.
   
  ===========================================================================*/
int stdenc_ioctl_flush  (struct file *filp)
{
    return (0);
}


/*=============================================================================

   stdenc_ioctl_lseek
   
   Adjust the file position.
   
   This is not meaningful in the template driver. Signal an error to prevent
   seeking on this device.
   
  ===========================================================================*/
loff_t  stdenc_ioctl_lseek  (struct file *filp,  loff_t offset, int other)
{   
    return (-ENOTTY);
}

/*
 * notify_data() and notify_space() are examples of how to signal the
 * transfer of data to and from the physical device.
 * These methods are called from stdenc_ioctl_read() and stdenc_ioctl_write()
 * in the template code because this driver just passes date from write
 * calls to read calls via an internal buffer.
 */
 
static void notify_data(struct file *filp)
{                                                                                    
    stdenc_ioctl_dev_t *dev_p = (stdenc_ioctl_dev_t*)filp->private_data;                 
                                                                                     
    
    /* Wake up any blocking reads */
    wake_up_interruptible(&(dev_p->read_queue));
}


static void notify_space(struct file *filp)
{                                                                                    
    stdenc_ioctl_dev_t *dev_p = (stdenc_ioctl_dev_t*)filp->private_data;                 
                                                                                     
    
    /* Wake up any blocking writes */
    wake_up_interruptible(&(dev_p->write_queue));
}


