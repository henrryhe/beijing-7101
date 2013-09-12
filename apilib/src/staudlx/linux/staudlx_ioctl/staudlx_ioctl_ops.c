/*****************************************************************************
 *
 *  Module      : staudlx_ioctl
 *  Date        : 11-07-2005
 *  Author      : TAYLORD
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
#include <linux/poll.h>    /* Poll and select support        */
#include <linux/errno.h>   /* Defines standard error codes */
#include <asm/pgtable.h>

#include "staudlx_ioctl_types.h"       /* Modules specific defines */

/*** PROTOTYPES **************************************************************/


#include "staudlx_ioctl_fops.h"


int staudlx_ioctl_readdir(struct file *filp,  void *other, filldir_t othermore)
{
    return (0);
}


unsigned int staudlx_ioctl_poll   (struct file *filp,  struct poll_table_struct *pollp)
{
    int err = 0;    
    unsigned int mask = 0;

                                                                         
    staudlx_ioctl_dev_t *dev_p = (staudlx_ioctl_dev_t*)filp->private_data;                  

    /*** Get the lock ***/
    
    if (down_interruptible(&(dev_p->lock))) {
    
        /* Call interrupted */
    mask = POLLERR | POLLIN | POLLOUT | POLLRDNORM | POLLWRNORM;
    /* Need to think about this. */
    /* Where should the error be put since read and write should return error */
    goto fault;
    }

    poll_wait(filp, &dev_p->read_queue, pollp);
    poll_wait(filp, &dev_p->write_queue, pollp);

    if( dev_p->write_idx != dev_p->read_idx ){
    
        mask |= POLLIN | POLLRDNORM; /* Readable */
    }
    
    if( (dev_p->write_idx+1)%DATA_BUFFER_SIZE != dev_p->read_idx ){
    
        mask |= POLLOUT | POLLWRNORM; /* Writeable */
    }
    
    /* Release the lock */
    up(&(dev_p->lock));

    return mask;
    
fault:
    
    return (err);
}


int staudlx_ioctl_mmap (struct file *filp, struct vm_area_struct *vma)
{
    if (remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff,
                        vma->vm_end - vma->vm_start,
                        pgprot_noncached(vma->vm_page_prot)))
        return -EAGAIN;

    return 0;
}


int staudlx_ioctl_fsync  (struct inode *node, struct dentry *entry, int other)
{
    return (-EBUSY);
}




