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

#include <linux/errno.h>   /* Defines standard error codes */
#include <linux/slab.h>    /* Memory allocation */
#include <linux/slab.h>    /* Memory allocation */

#include "stdenc_ioctl_types.h"       /* Modules specific defines */
#include "stos.h"

/*** PROTOTYPES **************************************************************/

#include "stdenc_ioctl_fops.h"

/*** PROTOTYPES **************************************************************/



/*=============================================================================

   stdenc_ioctl_open

   This function can deschedule and MUST be reentrant.

  ===========================================================================*/
int stdenc_ioctl_open(struct inode *node, struct file *filp)
{

    int err   = 0;                      /* No error */
 /* int major = imajor(node);         *//* The major number */
    int minor = iminor(node);           /* The major number */


    /*** Modify the file ops table if needed ***/

    /*filp->f_op = &fops;                        */
    /*return filp->f_op->open(node, filp);       */  /* Delegate */


    /*** Modify the private data if needed - could be set by devfs/sysfs ***/

    if (NULL == filp->private_data) {


        /* stdenc_ioctl_NewNode() may deshedule - call must be reentrant */
        filp->private_data = stdenc_ioctl_NewNode(minor);

        if (NULL == filp->private_data) {

            /* Memory allocation error */
            err = -ENOMEM;
            goto fail;
        }
    }


    /*** Check the access mode ***/

    STTBX_Print((KERN_ALERT "ACCESS %08X\n", filp->f_flags));

    if ((filp->f_flags & O_ACCMODE) == O_RDONLY) {

        STTBX_Print((KERN_ALERT "Open Read only\n"));
    }
    else if ((filp->f_flags & O_ACCMODE) == O_WRONLY) {

        STTBX_Print((KERN_ALERT "Open Write only\n"));
    }
    else if ((filp->f_flags & O_ACCMODE) == O_RDWR) {

        STTBX_Print((KERN_ALERT "Open Read/Write\n"));
    }

    if (filp->f_flags & O_CREAT) {

        STTBX_Print((KERN_ALERT "Create if it does not exist\n"));
    }

    if (filp->f_flags & O_EXCL) {

        STTBX_Print((KERN_ALERT "Provide exclusive access\n"));
    }

    if (filp->f_flags & O_NOCTTY) {

        STTBX_Print((KERN_ALERT "???? O_NOCTTY\n"));
    }

    if (filp->f_flags & O_TRUNC) {

        STTBX_Print((KERN_ALERT "Truncate the file to zero size first\n"));
    }

    if (filp->f_flags & O_APPEND) {

        STTBX_Print((KERN_ALERT "Append to the file (don't overwrite)\n"));
    }

    if (filp->f_flags & O_NONBLOCK) {

        STTBX_Print((KERN_ALERT "Access methods are non-blocking\n"));
    }

    if (filp->f_flags & O_SYNC) {

        STTBX_Print((KERN_ALERT "???? O_SYNC\n"));
    }



    return (0);  /* If we get here then we have succeeded */

    /**************************************************************************/

    /*** Clean up on error ***/

fail :

    return (err);
}


/*=============================================================================

   stdenc_ioctl_release   (close)

   This function can deschedule and MUST be reentrant.

  ===========================================================================*/
int stdenc_ioctl_release(struct inode *node, struct file *filp)  /* close */
{

    int err = 0;  /* No error */



    return (0);  /* If we get here then we have succeeded */

    /**************************************************************************/

    /*** Clean up on error ***/

    return (err);
}


static stdenc_ioctl_dev_t *table[16];


/*=============================================================================

   stdenc_ioctl_NewNode

   Create a new data structure for this 'device' (minor number)

   This function can deschedule and MUST be reentrant.

  ===========================================================================*/
stdenc_ioctl_dev_t *stdenc_ioctl_NewNode(unsigned int id)
{
    /* Check valid id */

    if (id > (sizeof(table)/sizeof(*table))) {

        /* Invalid id */
        return (NULL);
    }

    down(&(stdenc_ioctl_lock)); /* We can deshedule here !!! */

    if (NULL == table[id]) {

        /* Initialise the data structure */

        /* We can deshedule here !!! */
        table[id] = (stdenc_ioctl_dev_t*)kmalloc(sizeof(stdenc_ioctl_dev_t), GFP_KERNEL);

        if (NULL != table[id])
        {
            sema_init          (&(table[id]->lock), 1);
            init_waitqueue_head(&(table[id]->read_queue));
            init_waitqueue_head(&(table[id]->write_queue));
            table[id]->read_idx  = 0;
            table[id]->write_idx = 0;
        }
    }

    up(&(stdenc_ioctl_lock));  /* Release the global lock */

    return (table[id]);
}


/*=============================================================================

   stdenc_ioctl_GetNode

   Get an existing data structure for this 'device' (minor number)

  ===========================================================================*/
stdenc_ioctl_dev_t *stdenc_ioctl_GetNode(unsigned int id)
{
    /* Check valid id */

    if (id > (sizeof(table)/sizeof(*table))) {

        /* Invalid id */
        return (NULL);
    }

    return (table[id]);
}

/*=============================================================================

   stdenc_ioctl_CleanUp

   Free up all the allocated 'device' structures.

   This function can deschedule and MUST be reentrant.

  ===========================================================================*/
void stdenc_ioctl_CleanUp()
{
    int i;

    down(&(stdenc_ioctl_lock)); /* We can deshedule here !!! */

    for (i = 0; (i < (sizeof(table)/sizeof(*table))); i++) {

        if (NULL != table[i]) {

            kfree(table[i]);
        }

        table[i] = NULL;
    }

    up(&(stdenc_ioctl_lock));
}
