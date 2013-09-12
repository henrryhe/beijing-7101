/*****************************************************************************
 *
 *  Module      : stvtg_ioctl
 *  Date        : 15-10-2005
 *  Author      : ATROUSWA
 *  Description :
 *  Copyright   : STMicroelectronics (c)2005
 *****************************************************************************/

/* Requires   MODULE         defined on the command line */
/* Requires __KERNEL__       defined on the command line */
/* Requires __SMP__          defined on the command line for SMP systems   */
/* Requires EXPORT_SYMTAB    defined on the command line to export symbols */

#include <linux/init.h>    /* Initiliasation support */
#include <linux/module.h>  /* Module support */
#include <linux/kernel.h>  /* Kernel support */
#include <linux/version.h> /* Kernel version */
#include <linux/fs.h>      /* File operations (fops) defines */
#include <linux/cdev.h>    /* Charactor device support */
#include <linux/netdevice.h> /* ??? SET_MODULE_OWNER ??? */
#include <linux/ioport.h>  /* Memory/device locking macros   */

#include <linux/proc_fs.h>   /*** ++ ***/


/* UTS_RELEASE             - Kernel version  (text) */
/* LINUX_VERSION_CODE      - Kernel version  (hex)  */

#include <linux/errno.h>   /* Defines standard error codes */
#include <linux/sched.h>   /* Defines pointer (current) to current task */


#include "stvtg_ioctl_types.h"  /* Modules specific defines */
#include "stos.h"

/*** MODULE INFO *************************************************************/

MODULE_AUTHOR("Walid ATROUS");
MODULE_DESCRIPTION("");
MODULE_SUPPORTED_DEVICE("");
MODULE_LICENSE("ST Microelectronics");


/*** PROTOTYPES **************************************************************/

static int  stvtg_ioctl_init_module(void);
static void stvtg_ioctl_cleanup_module(void);


#include "stvtg_ioctl_fops.h"


/*** MODULE PARAMETERS *******************************************************/

/*** GLOBAL VARIABLES *********************************************************/
 unsigned int             stvtg_ioctl_major;
 struct cdev             *stvtg_ioctl_cdev  = NULL;
 struct class_simple     *stvtg_ioctl_class = NULL;
 struct proc_dir_entry *STAPI_dir ;

/*** EXPORTED SYMBOLS ********************************************************/

#if defined (EXPORT_SYMTAB)

/* EXPORT_SYMBOL(func);           - With    version info */
/* EXPORT_SYMBOL_NOVERS(func);    - Without version info */

#endif


/*** LOCAL TYPES *********************************************************/


/*** LOCAL CONSTANTS ********************************************************/

#define MAX_DEVICES    4



/* =================================================================
   Local variables declaration
   ================================================================= */


struct semaphore stvtg_ioctl_lock; /* Global lock for the module */

static struct file_operations stvtg_ioctl_fops = {
    owner:   THIS_MODULE,
    open    : stvtg_ioctl_open,
    read    : stvtg_ioctl_read,
    ioctl   : stvtg_ioctl_ioctl,
    release : stvtg_ioctl_release,     /* close */
};



/*** METHODS ****************************************************************/

/*=============================================================================

   stvtg_ioctl_init_module

   Initialise the module (initialise the access routines (fops) and set the
   major number. Aquire any resources needed and setup internal structures.

  ===========================================================================*/
static int __init stvtg_ioctl_init_module(void)
{

    /*** Initialise the module ***/

    printk(KERN_INFO "stvtg_ioctl_init_module: Initializing STVTG IOCTL Module\n");
    #if defined(STVTG_IOCTL_MAJOR)
        stvtg_ioctl_major = STVTG_IOCTL_MAJOR;
    #else
        stvtg_ioctl_major = 0;
    #endif

    /* Register the device with the kernel */
    if (STLINUX_DeviceRegister(&stvtg_ioctl_fops, 1, STVTG_IOCTL_LINUX_DEVICE_NAME,
                             &stvtg_ioctl_major, &stvtg_ioctl_cdev, &stvtg_ioctl_class) != 0)
    {
        printk(KERN_ERR "%s(): LINUX_DeviceRegister() error !!!\n", __FUNCTION__);
        return(-1);
    }

    /*** Set up the modules global lock ***/

    sema_init(&(stvtg_ioctl_lock), 1);

    /* Creation if not existing of /proc/STAPI directory */
    STAPI_dir = proc_mkdir(STAPI_STAT_DIRECTORY, NULL);
    if ( STAPI_dir == NULL )
    {
        printk(KERN_ERR "stvtg_ioctl_init_module: proc_mkdir() error !!!\n");
        STLINUX_DeviceUnregister(1, STVTG_IOCTL_LINUX_DEVICE_NAME, stvtg_ioctl_major, stvtg_ioctl_cdev, stvtg_ioctl_class);
        return( -1 );
    }

    return (0);

    /* KERN_ERR: 3
       KERN_WARNING: 4
       KERN_NOTICE: 5
       KERN_INFO: 6
       KERN_DEBUG: 7 */

}


/*=============================================================================

   stvtg_ioctl_cleanup_module

   Realease any resources allocaed to this module before the module is
   unloaded.

  ===========================================================================*/
static void __exit stvtg_ioctl_cleanup_module(void)
{
    printk(KERN_INFO "stvtg_ioctl: Exiting STVTG kernel Module\n");

    /* Unregister the device with the kernel */
    STLINUX_DeviceUnregister(1, STVTG_IOCTL_LINUX_DEVICE_NAME, stvtg_ioctl_major, stvtg_ioctl_cdev, stvtg_ioctl_class);

}


/*** MODULE LOADING ******************************************************/


/* Tell the module system these are our entry points. */

module_init(stvtg_ioctl_init_module);
module_exit(stvtg_ioctl_cleanup_module);
