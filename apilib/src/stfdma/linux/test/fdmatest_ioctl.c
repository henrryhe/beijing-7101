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
#include <linux/fs.h>            /* File operations (fops) defines */
#include <linux/ioport.h>        /* Memory/device locking macros   */

#include <linux/errno.h>         /* Defines standard error codes */

#include <asm/uaccess.h>         /* User space access routines */

#include <linux/sched.h>         /* User privilages (capabilities) */

#include "fdmatest_types.h"    /* Module specific types */

#include "fdmatest.h"          /* Defines ioctl numbers */
                                                                
#include "../../tests/fdmatst.h"

/*** PROTOTYPES **************************************************************/


#include "fdmatest_fops.h"                                                                                                     


/*** METHODS *****************************************************************/

/*=============================================================================

   fdmatest_ioctl
   
   Handle any device specific calls.
   
   The commands constants are defined in 'fdmatest.h'.
   
  ===========================================================================*/
int fdmatest_ioctl  (struct inode *node, struct file *filp, unsigned int cmd, unsigned long arg)
{
    int err = 0;
    
    printk(KERN_ALERT "ioctl command [0X%08X]\n", cmd);
    
    /*** Check the user privalage ***/
    
    /* if (!capable (CAP_SYS_ADMIN)) */ /* See 'sys/sched.h' */
    /* {                             */
    /*     return (-EPERM);          */
    /* }                             */
    
    /*** Check the command is for this module ***/
    
    if (_IOC_TYPE(cmd) != FDMATEST_TYPE) {
    
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
        case IO_STFDMA_StartTest:
            {
                printk(KERN_ALERT "fdma - starting test harness\n");
                fdmatst_main();
                printk(KERN_ALERT "fdma - test harness complete\n");
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


