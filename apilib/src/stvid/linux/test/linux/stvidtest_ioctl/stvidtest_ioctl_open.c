/*****************************************************************************
 *
 *  Module      : stvidtest_ioctl
 *  Date        : 09-01-2006
 *  Author      : C. DAILLY
 *  Description :
 *  Copyright   : STMicroelectronics (c)2006
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

#include "sttbx.h"

#include "stvidtest_ioctl.h"
#include "stvidtest_ioctl_types.h"
      /* Modules specific defines */


/*** PROTOTYPES **************************************************************/

#include "stvidtest_ioctl_fops.h"

/*** PROTOTYPES **************************************************************/


/*=============================================================================

   stvidtest_ioctl_open

   This function can deschedule and MUST be reentrant.

  ===========================================================================*/
int stvidtest_ioctl_open(struct inode *node, struct file *filp)
{

    int err   = 0;                      /* No error */
    int minor = iminor(node);           /* The minor number */


    /*** Modify the private data if needed - could be set by devfs/sysfs ***/

    if (NULL == filp->private_data) {

        /* stvidtest_ioctl_NewNode() may deshedule - call must be reentrant */
        filp->private_data = stvidtest_ioctl_NewNode(minor);
        if (NULL == filp->private_data) {

            /* Memory allocation error */
            err = -ENOMEM;
            goto fail;
        }
    }

    /*** Check the access mode ***/

    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "ACCESS %08X", filp->f_flags));

    if ((filp->f_flags & O_ACCMODE) == O_RDONLY) {

        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Open Read only"));
    }
    else if ((filp->f_flags & O_ACCMODE) == O_WRONLY) {

        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Open Write only"));
    }
    else if ((filp->f_flags & O_ACCMODE) == O_RDWR) {

        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Open Read/Write"));
    }

    if (filp->f_flags & O_CREAT) {

        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Create if it does not exist"));
    }

    if (filp->f_flags & O_EXCL) {

        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Provide exclusive access"));
    }

    if (filp->f_flags & O_NOCTTY) {

        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "???? O_NOCTTY"));
    }

    if (filp->f_flags & O_TRUNC) {

        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Truncate the file to zero size first"));
    }

    if (filp->f_flags & O_APPEND) {

        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Append to the file (don't overwrite)"));
    }

    if (filp->f_flags & O_NONBLOCK) {

        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Access methods are non-blocking"));
    }

    if (filp->f_flags & O_SYNC) {

        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "???? O_SYNC"));
    }

    return (0);  /* If we get here then we have succeeded */

    /**************************************************************************/

    /*** Clean up on error ***/

fail :

    return (err);
}

/*=============================================================================

   stvidtest_ioctl_release   (close)

   This function can deschedule and MUST be reentrant.

  ===========================================================================*/
int stvidtest_ioctl_release(struct inode *node, struct file *filp)  /* close */
{

    int err = 0;  /* No error */

    return (0);  /* If we get here then we have succeeded */

    /**************************************************************************/

    /*** Clean up on error ***/

    return (err);
}


static stvidtest_ioctl_dev_t *table[16];


/*=============================================================================

   stvidtest_ioctl_NewNode

   Create a new data structure for this 'device' (minor number)

   This function can deschedule and MUST be reentrant.

  ===========================================================================*/
stvidtest_ioctl_dev_t *stvidtest_ioctl_NewNode(unsigned int id)
{
    /* Check valid id */

    if (id > (sizeof(table)/sizeof(*table))) {

        /* Invalid id */
        return (NULL);
    }

    down(&(stvidtest_ioctl_lock)); /* We can deshedule here !!! */

    if (NULL == table[id]) {

        /* Initialise the data structure */

        /* We can deshedule here !!! */
        table[id] = (stvidtest_ioctl_dev_t*)kmalloc(sizeof(stvidtest_ioctl_dev_t), GFP_KERNEL);

        if (NULL != table[id])
        {
            sema_init          (&(table[id]->lock), 1);
            init_waitqueue_head(&(table[id]->read_queue));
            init_waitqueue_head(&(table[id]->write_queue));
            table[id]->read_idx  = 0;
            table[id]->write_idx = 0;
        }
    }
    up(&(stvidtest_ioctl_lock));  /* Release the global lock */

    return (table[id]);
}


/*=============================================================================

   stvidtest_ioctl_GetNode

   Get an existing data structure for this 'device' (minor number)

  ===========================================================================*/
stvidtest_ioctl_dev_t *stvidtest_ioctl_GetNode(unsigned int id)
{
    /* Check valid id */

    if (id > (sizeof(table)/sizeof(*table))) {

        /* Invalid id */
        return (NULL);
    }

    return (table[id]);
}

/*=============================================================================

   stvidtest_ioctl_CleanUp

   Free up all the allocated 'device' structures.

   This function can deschedule and MUST be reentrant.

  ===========================================================================*/
void stvidtest_ioctl_CleanUp()
{
    int i;

    down(&(stvidtest_ioctl_lock)); /* We can deshedule here !!! */

    for (i = 0; (i < (sizeof(table)/sizeof(*table))); i++) {

        if (NULL != table[i]) {

            kfree(table[i]);
        }

        table[i] = NULL;
    }

    up(&(stvidtest_ioctl_lock));
}
