/*****************************************************************************
 *
 *  Module      : stvidtest_ioctl
 *  Date        : 09-01-2006
 *  Author      : C. DAILLY
 *  Description :
 *  Copyright   : STMicroelectronics (c)2006
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

#include "sttbx.h"

#include "stvidtest_ioctl_types.h"  /* Modules specific defines */


/*** MODULE INFO *************************************************************/

MODULE_AUTHOR("Cyril Dailly");
MODULE_DESCRIPTION("");
MODULE_SUPPORTED_DEVICE("");
MODULE_LICENSE("ST Microelectronics");


/*** PROTOTYPES **************************************************************/

static int  stvidtest_ioctl_init_module(void);
static void stvidtest_ioctl_cleanup_module(void);


#include "stvidtest_ioctl_fops.h"


/*** MODULE PARAMETERS *******************************************************/

/*** GLOBAL VARIABLES *********************************************************/
unsigned int             stvidtest_ioctl_major;
struct cdev            * stvidtest_ioctl_cdev  = NULL;
struct class_simple    * stvidtest_ioctl_class = NULL;
struct proc_dir_entry  * STAPI_dir ;

/*** EXPORTED SYMBOLS ********************************************************/

/*** LOCAL TYPES *********************************************************/

/*** LOCAL CONSTANTS ********************************************************/


/* =================================================================
   Local variables declaration
   ================================================================= */


struct semaphore stvidtest_ioctl_lock; /* Global lock for the module */

static struct file_operations stvidtest_ioctl_fops = {
    owner:   THIS_MODULE,
    open    : stvidtest_ioctl_open,
    ioctl   : stvidtest_ioctl_ioctl,
    release : stvidtest_ioctl_release,     /* close */
};



/*** METHODS ****************************************************************/

/*=============================================================================

   stvidtest_ioctl_init_module

   Initialise the module (initialise the access routines (fops) and set the
   major number. Aquire any resources needed and setup internal structures.

  ===========================================================================*/
static int __init stvidtest_ioctl_init_module(void)
{
    /*** Initialise the module ***/

    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "%s(): Initializing STVIDTEST IOCTL Module", __FUNCTION__));
#if defined(STVIDTEST_IOCTL_MAJOR)
    stvidtest_ioctl_major = STVIDTEST_IOCTL_MAJOR;
#else
    stvidtest_ioctl_major = 0;
#endif

    /* Register the device with the kernel */
    if (STLINUX_DeviceRegister(&stvidtest_ioctl_fops, 1, STVIDTEST_IOCTL_LINUX_DEVICE_NAME,
                             &stvidtest_ioctl_major, &stvidtest_ioctl_cdev, &stvidtest_ioctl_class) != 0)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s(): LINUX_DeviceRegister() error !!!", __FUNCTION__));
        return(-1);
    }

    /*** Set up the modules global lock ***/

    sema_init(&(stvidtest_ioctl_lock), 1);

    /* Creation if not existing of /proc/STAPI directory */
    STAPI_dir = proc_mkdir(STAPI_STAT_DIRECTORY, NULL);
    if ( STAPI_dir == NULL )
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s(): proc_mkdir() error !!!", __FUNCTION__));
        STLINUX_DeviceUnregister(1, STVIDTEST_IOCTL_LINUX_DEVICE_NAME, stvidtest_ioctl_major, stvidtest_ioctl_cdev, stvidtest_ioctl_class);
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

   stvidtest_ioctl_cleanup_module

   Realease any resources allocaed to this module before the module is
   unloaded.

  ===========================================================================*/
static void __exit stvidtest_ioctl_cleanup_module(void)
{
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "stvidtest_ioctl: Exiting STVIDTEST kernel Module"));

    /* Unregister the device with the kernel */
    STLINUX_DeviceUnregister(1, STVIDTEST_IOCTL_LINUX_DEVICE_NAME, stvidtest_ioctl_major, stvidtest_ioctl_cdev, stvidtest_ioctl_class);

}


/*** MODULE LOADING ******************************************************/


/* Tell the module system these are our entry points. */

module_init(stvidtest_ioctl_init_module);
module_exit(stvidtest_ioctl_cleanup_module);
