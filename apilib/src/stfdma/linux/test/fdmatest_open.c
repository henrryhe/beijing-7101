/*****************************************************************************
 *
 *  Module      : fdmatest
 *  Date        : 05-04-2005
 *  Author      : WALKERM
 *  Description :
 *
 *****************************************************************************/

/* Requires   MODULE   defined on the command line */
/* Requires __KERNEL__ defined on the command line */
/* Requires __SMP__    defined on the command line for SMP systems */

#define _NO_VERSION_
#include <linux/module.h>  /* Module support */
#include <linux/version.h> /* Kernel version */
#include <linux/fs.h>      /* File operations (fops) defines */
#include <linux/ioport.h>  /* Memory/device locking macros   */

#include <linux/errno.h>   /* Defines standard error codes */
#include <linux/slab.h>    /* Memory allocation */
#include <linux/slab.h>    /* Memory allocation */

#include "fdmatest_types.h"       /* Modules specific defines */


/*** PROTOTYPES **************************************************************/

#include "fdmatest_fops.h"
#include "fdmatest_evts.h"

/* Output appears in /var/log/syslog */
int  printk(char const *format, ...);

/*** PROTOTYPES **************************************************************/



/*=============================================================================

   fdmatest_open
   
   This function can deschedule and MUST be reentrant.
   
  ===========================================================================*/
int fdmatest_open(struct inode *node, struct file *filp)
{
    
    int err   = 0;                      /* No error */
 /* int major = imajor(node);         *//* The major number */
    int minor = iminor(node);           /* The major number */
    
    
    /*** Modify the file ops table if needed ***/
    
    /*filp->f_op = &fops;                        */
    /*return filp->f_op->open(node, filp);       */  /* Delegate */
    
    
    /*** Modify the private data if needed - could be set by devfs/sysfs ***/
    
    if (NULL == filp->private_data) {
                                                      

        /* fdmatest_NewNode() may deshedule - call must be reentrant */
        filp->private_data = fdmatest_NewNode(minor);

        if (NULL == filp->private_data) {
        
            /* Memory allocation error */
            err = -ENOMEM;
            goto fail;
        }                                             
    }
    
    
    /*** Check the access mode ***/
    
    printk(KERN_ALERT "ACCESS %08X\n", filp->f_flags);
    
    if ((filp->f_flags & O_ACCMODE) == O_RDONLY) {
    
        printk(KERN_ALERT "Open Read only\n");
    }
    else if ((filp->f_flags & O_ACCMODE) == O_WRONLY) {
    
        printk(KERN_ALERT "Open Write only\n");
    }
    else if ((filp->f_flags & O_ACCMODE) == O_RDWR) {
    
        printk(KERN_ALERT "Open Read/Write\n");
    }
    
    if (filp->f_flags & O_CREAT) {
    
        printk(KERN_ALERT "Create if it does not exist\n");
    }
    
    if (filp->f_flags & O_EXCL) {
    
        printk(KERN_ALERT "Provide exclusive access\n");
    }
    
    if (filp->f_flags & O_NOCTTY) {
    
        printk(KERN_ALERT "???? O_NOCTTY\n");
    }
    
    if (filp->f_flags & O_TRUNC) {
    
        printk(KERN_ALERT "Truncate the file to zero size first\n");
    }
    
    if (filp->f_flags & O_APPEND) {
    
        printk(KERN_ALERT "Append to the file (don't overwrite)\n");
    }
    
    if (filp->f_flags & O_NONBLOCK) {
    
        printk(KERN_ALERT "Access methods are non-blocking\n");
    }
    
    if (filp->f_flags & O_SYNC) {
    
        printk(KERN_ALERT "???? O_SYNC\n");
    }
    
    
    
    return (0);  /* If we get here then we have succeeded */
    
    /**************************************************************************/
    
    /*** Clean up on error ***/
    
fail :
    
    return (err);
}


/*=============================================================================

   fdmatest_release   (close)
   
   This function can deschedule and MUST be reentrant.
   
  ===========================================================================*/
int fdmatest_release(struct inode *node, struct file *filp)  /* close */
{
    
    int err = 0;  /* No error */
                                     
                                     
    
    return (0);  /* If we get here then we have succeeded */
    
    /**************************************************************************/
    
    /*** Clean up on error ***/
    
    return (err);
}


static fdmatest_dev_t *table[16];


/*=============================================================================

   fdmatest_NewNode
   
   Create a new data structure for this 'device' (minor number)
   
   This function can deschedule and MUST be reentrant.
   
  ===========================================================================*/
fdmatest_dev_t *fdmatest_NewNode(unsigned int id)
{
    /* Check valid id */
    
    if (id > (sizeof(table)/sizeof(*table))) {
    
        /* Invalid id */
        return (NULL);
    }

    down(&(fdmatest_lock)); /* We can deshedule here !!! */
    
    if (NULL == table[id]) {
    
        /* Initialise the data structure */

        /* We can deshedule here !!! */
        table[id] = (fdmatest_dev_t*)kmalloc(sizeof(fdmatest_dev_t), GFP_KERNEL);

        if (NULL != table[id])
        {
            sema_init          (&(table[id]->lock), 1);
            init_waitqueue_head(&(table[id]->read_queue));
            init_waitqueue_head(&(table[id]->write_queue));  
            table[id]->read_idx  = 0;                         
            table[id]->write_idx = 0;                        
        }
    }

    up(&(fdmatest_lock));  /* Release the global lock */
    
    return (table[id]);
}


/*=============================================================================

   fdmatest_GetNode
   
   Get an existing data structure for this 'device' (minor number)
   
  ===========================================================================*/
fdmatest_dev_t *fdmatest_GetNode(unsigned int id)
{
    /* Check valid id */
    
    if (id > (sizeof(table)/sizeof(*table))) {
    
        /* Invalid id */
        return (NULL);
    }
    
    return (table[id]);
}

/*=============================================================================

   fdmatest_CleanUp
   
   Free up all the allocated 'device' structures.
   
   This function can deschedule and MUST be reentrant.
   
  ===========================================================================*/
void fdmatest_CleanUp()
{
    int i;

    down(&(fdmatest_lock)); /* We can deshedule here !!! */
    
    for (i = 0; (i < (sizeof(table)/sizeof(*table))); i++) {
    
        if (NULL != table[i]) {
        
            kfree(table[i]);
        }
        
        table[i] = NULL;
    }

    up(&(fdmatest_lock));
}
