/*****************************************************************************
 *
 *  Module      : fdmatest
 *  Date        : 05-04-2005
 *  Author      : WALKERM
 *  Description :
 *
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

/* UTS_RELEASE             - Kernel version  (text) */
/* LINUX_VERSION_CODE      - Kernel version  (hex)  */

#include <linux/errno.h>   /* Defines standard error codes */
#include <linux/sched.h>   /* Defines pointer (current) to current task */

#include "fdmatest_types.h"       /* Modules specific defines */

#include "stfdma.h"

/*** MODULE INFO *************************************************************/

MODULE_AUTHOR("WALKERM");
MODULE_DESCRIPTION("");
MODULE_SUPPORTED_DEVICE("");
MODULE_LICENSE("ST Microelectronics");

/*** PROTOTYPES **************************************************************/

static int  fdmatest_init_module(void);
static void fdmatest_cleanup_module(void);

#include "fdmatest_fops.h"

/*** MODULE PARAMETERS *******************************************************/
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,12)
unsigned int     irq_number   = 4;
MODULE_PARM      (irq_number,   "i");
MODULE_PARM_DESC (irq_number,   "Interrupt request line");
#endif

/*** GLOBAL VARIABLES *********************************************************/

/*** EXPORTED SYMBOLS ********************************************************/

#if defined (EXPORT_SYMTAB)
/* EXPORT_SYMBOL(func);           - With    version info */
/* EXPORT_SYMBOL_NOVERS(func);    - Without version info */
#endif

/*** LOCAL TYPES *********************************************************/

/*** LOCAL CONSTANTS ********************************************************/

#define MAX_DEVICES    4

/*** LOCAL VARIABLES *********************************************************/

static unsigned int major = 240;

struct semaphore fdmatest_lock; /* Global lock for the module */

static struct cdev         *fdmatest_cdev  = NULL;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,12)
static struct class_simple *fdmatest_class = NULL;
#endif

static struct file_operations fdmatest_fops = {

    open    : fdmatest_open,
    read    : fdmatest_read,
    write   : fdmatest_write,
    ioctl   : fdmatest_ioctl,
 /* flush   : fdmatest_flush,    */
 /* lseek   : fdmatest_lseek,    */
 /* readdir : fdmatest_readdir,  */
    poll    : fdmatest_poll,
 /* mmap    : fdmatest_mmap,     */
 /* fsync   : fdmatest_fsync,    */
    release : fdmatest_release,     /* close */
};

/*** METHODS ****************************************************************/

/*=============================================================================

   fdmatest_init_module

   Initialise the module (initialise the access routines (fops) and set the
   major number. Aquire any resources needed and setup internal structures.

  ===========================================================================*/
static int __init fdmatest_init_module(void)
{
    /*** Initialise the module ***/

    int err     = 0;  /* No error */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,12)
    int entries = 0;
#endif
    dev_t base_dev_no;

    SET_MODULE_OWNER(&fdmatest_fops);

    /*** Set up the modules global lock ***/

    sema_init(&(fdmatest_lock), 1);

    /*
     * Register the major number. If major = 0 then a major number is auto
     * allocated. The allocated number is returned.
     * The major number can be seen in user space in '/proc/devices'
     */

    if (major == 0) {

        if (alloc_chrdev_region(&base_dev_no, 0, MAX_DEVICES, "fdmatest")) {

            err = -EBUSY;
            printk(KERN_ALERT "No major numbers for fdmatest by %s (pid %i)\n", current->comm, current->pid);
            goto fail;
        }

        major = MAJOR(base_dev_no);
    }
    else
    {
	base_dev_no = MKDEV(major, 0);

        if (register_chrdev_region(base_dev_no, MAX_DEVICES, "fdmatest")) {

            err = -EBUSY;
            printk(KERN_ALERT "No major numbers for fdmatest by %s (pid %i)\n", current->comm, current->pid);
            goto fail;
        }
    }

    if (NULL == (fdmatest_cdev = cdev_alloc())) {

         err = -EBUSY;
         printk(KERN_ALERT "No major numbers for fdmatest by %s (pid %i)\n", current->comm, current->pid);
         goto fail_reg;
    }

    fdmatest_cdev->owner = THIS_MODULE;
    fdmatest_cdev->ops   = &fdmatest_fops;
    kobject_set_name(&(fdmatest_cdev->kobj), "fdmatest%d", 0);


    /*** Register any register regions ***/


    /*** Do the work ***/

    /* Appears in /var/log/syslog */
    printk(KERN_ALERT "Load module fdmatest [%d] by %s (pid %i)\n", major, current->comm, current->pid);


    /*** Register the device nodes for this module ***/

    /* Add the char device structure for this module */

    if (cdev_add(fdmatest_cdev, base_dev_no, MAX_DEVICES)) {

        err = -EBUSY;
        printk(KERN_ALERT "Failed to add a charactor device for fdmatest by %s (pid %i)\n", current->comm, current->pid);
        goto fail_remap;
    }

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,12)
    /* Create a class for this module */
    fdmatest_class = class_simple_create(THIS_MODULE, "fdmatest");

    if (IS_ERR(fdmatest_class)) {

            printk(KERN_ERR "Error creating fdmatest class.\n");
            err = -EBUSY;
            goto fail_final;
    }

    /* Add entries into /sys for each minor number we use */

    for (; (entries < MAX_DEVICES); entries++)
    {
    	struct class_device *class_err;

        class_err = class_simple_device_add(fdmatest_class, MKDEV(major, entries), NULL, "fdmatest%d", entries);

    	if (IS_ERR(class_err)) {

            printk(KERN_ERR "Error creating fdmatest device %d.\n", entries);
            err = PTR_ERR(class_err);
            goto fail_dev;
        }
    }
#endif

    return (0);  /* If we get here then we have succeeded */

    /**************************************************************************/

    /*** Clean up on error ***/
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,12)
fail_dev:

    /* Remove any /sys entries we have added so far */
    for (entries--; (entries >= 0); entries--)
    {
        class_simple_device_remove(MKDEV(major, entries));
    }

    /* Remove the device class entry */
    class_simple_destroy(fdmatest_class);

fail_final:
#endif

    /* Remove the char device structure (has been added) */
    cdev_del(fdmatest_cdev);
    goto fail_reg;

fail_remap:

    /* Remove the char device structure (has not been added) */
    kobject_put(&(fdmatest_cdev->kobj));

fail_reg:

    /* Unregister the module */
    unregister_chrdev_region(MKDEV(major, 0), MAX_DEVICES);

fail : return (err);
}


/*=============================================================================

   fdmatest_cleanup_module

   Realease any resources allocaed to this module before the module is
   unloaded.

  ===========================================================================*/
static void __exit fdmatest_cleanup_module(void)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,12)
    int i;

    /* Release the /sys entries for this module */

    /* Remove any devices */
    for (i = 0; (i < MAX_DEVICES); i++)
    {
        class_simple_device_remove(MKDEV(major, i));
    }

    /* Remove the device class entry */
    class_simple_destroy(fdmatest_class);
#endif

    /* Remove the char device structure (has been added) */
    cdev_del(fdmatest_cdev);

    /* Unregister the module */
    unregister_chrdev_region(MKDEV(major, 0), MAX_DEVICES);

    /* Free any module resources */
    fdmatest_CleanUp();

    printk(KERN_ALERT "Unload module fdmatest by %s (pid %i)\n", current->comm, current->pid);
}


/*** MODULE LOADING ******************************************************/


/* Tell the module system these are our entry points. */

module_init(fdmatest_init_module);
module_exit(fdmatest_cleanup_module);
