/*****************************************************************************
 *
 *  Module      : staudlx_core_main.c
 *  Date        : 02-07-2005
 *  Author      : WALKERM
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

/* UTS_RELEASE             - Kernel version  (text) */
/* LINUX_VERSION_CODE      - Kernel version  (hex)  */

#include <linux/errno.h>   /* Defines standard error codes */
#include <linux/sched.h>   /* Defines pointer (current) to current task */

#include "staudlx_core_types.h"       /* Modules specific defines */
#include "staudlx_core_proc.h"

#include "staudlx.h"

/*** MODULE INFO *************************************************************/

MODULE_AUTHOR("YALUNGR");
MODULE_DESCRIPTION("");
MODULE_SUPPORTED_DEVICE("");
MODULE_LICENSE("ST Microelectronics");


/*** PROTOTYPES **************************************************************/

static int  staudlx_core_init_module(void);
static void staudlx_core_cleanup_module(void);

#include "staudlx_core_fops.h"




/*** MODULE PARAMETERS *******************************************************/
                                                           


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

#ifndef STAUDLX_CORE_MAJOR
#define STAUDLX_CORE_MAJOR   0
#endif

static unsigned int major = STAUDLX_CORE_MAJOR;

struct semaphore staudlx_core_lock; /* Global lock for the module */

static struct cdev         *staudlx_core_cdev  = NULL;
#if 0 /* LDDE2.2 */
static struct class_simple *staudlx_core_class = NULL;                                                                                                       
#endif
static struct file_operations staudlx_core_fops = {

    open    : staudlx_core_open,
    read    : staudlx_core_read,
    write   : staudlx_core_write,
    ioctl   : staudlx_core_ioctl,
 /* flush   : staudlx_core_flush,    */
 /* lseek   : staudlx_core_lseek,    */
 /* readdir : staudlx_core_readdir,  */
    poll    : staudlx_core_poll,     
 /* mmap    : staudlx_core_mmap,     */
 /* fsync   : staudlx_core_fsync,    */  
    release : staudlx_core_release,     /* close */
};



/*** METHODS ****************************************************************/

/*=============================================================================

   staudlx_core_init_module

   Initialise the module (initialise the access routines (fops) and set the
   major number. Aquire any resources needed and setup internal structures.

  ===========================================================================*/
static int __init staudlx_core_init_module(void)
{
    /*** Initialise the module ***/

    int err     = 0;  /* No error */
//    int entries = 0;

    dev_t base_dev_no;

    SET_MODULE_OWNER(&staudlx_core_fops);

    /*** Set up the modules global lock ***/

    sema_init(&(staudlx_core_lock), 1);

    /*
     * Register the major number. If major = 0 then a major number is auto
     * allocated. The allocated number is returned.
     * The major number can be seen in user space in '/proc/devices'
     */

    if (major == 0) {

        if (alloc_chrdev_region(&base_dev_no, 0, MAX_DEVICES, "staudlx_core")) {
    
            err = -EBUSY;
            printk(KERN_ALERT "No major numbers for staudlx_core by %s (pid %i)\n", current->comm, current->pid);
            goto fail;
        }

        major = MAJOR(base_dev_no);
    }
    else
    {
    base_dev_no = MKDEV(major, 0);
        
        if (register_chrdev_region(base_dev_no, MAX_DEVICES, "staudlx_core")) {
        
            err = -EBUSY;
            printk(KERN_ALERT "No major numbers for staudlx_core by %s (pid %i)\n", current->comm, current->pid);
            goto fail;
        }
    }

    if (NULL == (staudlx_core_cdev = cdev_alloc())) {
    
         err = -EBUSY;
         printk(KERN_ALERT "No major numbers for staudlx_core by %s (pid %i)\n", current->comm, current->pid);
         goto fail_reg;
    }

    staudlx_core_cdev->owner = THIS_MODULE;
    staudlx_core_cdev->ops   = &staudlx_core_fops;
    kobject_set_name(&(staudlx_core_cdev->kobj), "staudlx_core%d", 0);


    /*** Register any register regions ***/                  


    /*** Do the work ***/

    /* Appears in /var/log/syslog */
    printk(KERN_ALERT "Load module staudlx_core [%d] by %s (pid %i)\n", major, current->comm, current->pid);


    /*** Register the device nodes for this module ***/

    /* Add the char device structure for this module */

    if (cdev_add(staudlx_core_cdev, base_dev_no, MAX_DEVICES)) {
    
        err = -EBUSY;
        printk(KERN_ALERT "Failed to add a charactor device for staudlx_core by %s (pid %i)\n", current->comm, current->pid);
        goto fail_remap;
    }

#if 0 /* LDDE2.2 */
    /* Create a class for this module */

    staudlx_core_class = class_simple_create(THIS_MODULE, "staudlx_core");
    
    if (IS_ERR(staudlx_core_class)) {
    
            printk(KERN_ERR "Error creating staudlx_core class.\n");
            err = -EBUSY;
            goto fail_final;
    }

    /* Add entries into /sys for each minor number we use */

    for (; (entries < MAX_DEVICES); entries++) {
    
    struct class_device *class_err;

        class_err = class_simple_device_add(staudlx_core_class, MKDEV(major, entries), NULL, "staudlx_core%d", entries);
        
        if (IS_ERR(class_err)) {
        
            printk(KERN_ERR "Error creating staudlx_core device %d.\n", entries);
            err = PTR_ERR(class_err);
            goto fail_dev;
        }
    }
#endif

    staudlx_core_init_proc_fs();

    return (0);  /* If we get here then we have succeeded */
    
    /**************************************************************************/

    /*** Clean up on error ***/
#if 0 /* LDDE2.2 */

fail_dev:
    /* Remove any /sys entries we have added so far */
    for (entries--; (entries >= 0); entries--) {
    
        class_simple_device_remove(MKDEV(major, entries));
    }

    /* Remove the device class entry */
    class_simple_destroy(staudlx_core_class);

fail_final:

    /* Remove the char device structure (has been added) */
    cdev_del(staudlx_core_cdev);  
    goto fail_reg;
#endif    

fail_remap:  

    /* Remove the char device structure (has not been added) */
    kobject_put(&(staudlx_core_cdev->kobj));

fail_reg:

    /* Unregister the module */
    unregister_chrdev_region(MKDEV(major, 0), MAX_DEVICES);

fail : return (err);
}


/*=============================================================================

   staudlx_core_cleanup_module
   
   Realease any resources allocaed to this module before the module is
   unloaded.
   
  ===========================================================================*/
static void __exit staudlx_core_cleanup_module(void)
{                             
#if 0 /* LDDE2.2 */
	int i;
                                     
    /* Release the /sys entries for this module */

    /* Remove any devices */
    for (i = 0; (i < MAX_DEVICES); i++) {
    
        class_simple_device_remove(MKDEV(major, i));
    }

    /* Remove the device class entry */
    class_simple_destroy(staudlx_core_class);
#endif

    /* Remove the char device structure (has been added) */
    cdev_del(staudlx_core_cdev);
                                              
    /* Unregister the module */
    unregister_chrdev_region(MKDEV(major, 0), MAX_DEVICES);

    /* Free any module resources */           
    staudlx_core_CleanUp();

    printk(KERN_ALERT "Unload module staudlx_core by %s (pid %i)\n", current->comm, current->pid);
}


/*** MODULE LOADING ******************************************************/


/* Tell the module system these are our entry points. */

module_init(staudlx_core_init_module);
module_exit(staudlx_core_cleanup_module);



