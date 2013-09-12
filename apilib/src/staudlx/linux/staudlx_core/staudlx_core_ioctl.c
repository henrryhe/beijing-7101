/*****************************************************************************
 *
 *  Module      : staudlx_core_ioctl.c
 *  Date        : 02-07-2005
 *  Author      : WALKERM
 *  Description :
 *  Copyright   : STMicroelectronics (c)2005
 *****************************************************************************/

/* Requires   MODULE   defined on the command line */
/* Requires __KERNEL__ defined on the command line */
/* Requires __SMP__    defined on the command line for SMP systems */

#define _NO_VERSION_
#include <linux/module.h>  /* Module support */
#include <linux/version.h> /* Kernel version */
#include <linux/fs.h>            /* File operations (fops) defines */
#include <linux/ioport.h>        /* Memory/device locking macros   */

#include <linux/errno.h>         /* Defines standard error codes */

#include <asm/uaccess.h>         /* User space access routines */

#include <linux/sched.h>         /* User privilages (capabilities) */

#include "staudlx_core_types.h"    /* Module specific types */

#include "staudlx_core.h"          /* Defines ioctl numbers */



/*** PROTOTYPES **************************************************************/


#include "staudlx_core_fops.h"


/*** METHODS *****************************************************************/

/*=============================================================================

   staudlx_core_ioctl

   Handle any device specific calls.

   The commands constants are defined in 'staudlx_core.h'.

  ===========================================================================*/
int staudlx_core_ioctl  (struct inode *node, struct file *filp, unsigned int cmd, unsigned long arg)
{
    int err = 0;

    printk(KERN_ALERT "ioctl command [0X%08X]\n", cmd);

    /*** Check the user privalage ***/

    /* if (!capable (CAP_SYS_ADMIN)) */ /* See 'sys/sched.h' */
    /* {                             */
    /*     return (-EPERM);          */
    /* }                             */

    /*** Check the command is for this module ***/



    if (_IOC_TYPE(cmd) != STAUDLX_CORE_TYPE) {

        /* Not an ioctl for this module */
        err = -ENOTTY;
        goto fail;
    }




    /*** Check the access permittions ***/

    if ((_IOC_DIR(cmd) & _IOC_READ) && !(filp->f_mode & FMODE_READ)) {

        /* No Read permittions */
        err = -ENOTTY;
        goto fail;
    }

    if ((_IOC_DIR(cmd) & _IOC_WRITE) && !(filp->f_mode & FMODE_WRITE)) {

        /* No Write permittions */
        err = -ENOTTY;
        goto fail;
    }

    /*** Execute the command ***/

    switch (cmd) {

        case STAUDLX_CORE_CMD0 :
            break;

        case STAUDLX_CORE_CMD1 :
            {
                int data = 1;

                if ((err = put_user(data, (int*)arg))) {

                    /* Invalid user space address */
                    goto fail;
                }
            }

            break;

        case STAUDLX_CORE_CMD2 :
            break;

        case STAUDLX_CORE_CMD3 :
            {
                double data;

                if (copy_from_user(&data, (double*)arg, sizeof(data))) {

                    /* Data remaining to transfer */
                    /* Invalid user space address */
                    err = -EFAULT;
                    goto fail;
                }

                data = 10.0;

                if (copy_to_user((double*)arg, &data, sizeof(data))) {

                    /* Data remaining to transfer */
                    /* Invalid user space address */
                    err = -EFAULT;
                    goto fail;
                }
            }

            break;


        default :
            /*** Inappropriate ioctl for the device ***/
            err = -ENOTTY;
            goto fail;
    }


    return (0);

    /**************************************************************************/

    /*** Clean up on error ***/

fail:
    return (err);
}


