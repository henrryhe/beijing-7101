/*****************************************************************************
 *
 *  Module      : tsmerger_ioctl
 *  Date        : 24-07-2005
 *  Author      : STIEGLITZP
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

#include "tsmerger_ioctl_types.h"       /* Modules specific defines */
#include "stpti.h"            /* Defines architecture information */


/*** MODULE INFO *************************************************************/

MODULE_AUTHOR("STIEGLITZP");
MODULE_DESCRIPTION("");
MODULE_SUPPORTED_DEVICE("");
MODULE_LICENSE("ST Microelectronics");


/*** PROTOTYPES **************************************************************/

static int  tsmerger_ioctl_init_module(void);
static void tsmerger_ioctl_cleanup_module(void);

#include "tsmerger_ioctl_fops.h"

#include "tsmerger_ioctl_proc.h"

#include "stpti.h"

#include "stdevice.h"

#if defined( STPTI_FRONTEND_TSMERGER )

#include "tsmerger.h"

#endif

/*** MODULE PARAMETERS *******************************************************/

/*** GLOBAL VARIABLES *********************************************************/
void* tsmerge_base_addr = 0;

/*** EXPORTED SYMBOLS ********************************************************/

#if defined (EXPORT_SYMTAB)

/* EXPORT_SYMBOL(func);           - With    version info */
/* EXPORT_SYMBOL_NOVERS(func);    - Without version info */

#endif


/*** LOCAL TYPES *********************************************************/


/*** LOCAL CONSTANTS ********************************************************/

#define MAX_DEVICES    4


/*** LOCAL VARIABLES *********************************************************/
#if defined(ST_7100) || defined(ST_7109) || defined(ST_7200)
volatile void *TSMergeDataPtr_p = NULL;
#endif
#define TSMERGE_REG_SIZE 0x1000

static unsigned int major = 0;
struct semaphore tsmerger_ioctl_lock; /* Global lock for the module */
static struct cdev         *tsmerger_ioctl_cdev  = NULL;

static struct file_operations tsmerger_ioctl_fops = {

    open    : tsmerger_ioctl_open,
    ioctl   : tsmerger_ioctl_ioctl,
    release : tsmerger_ioctl_release,     /* close */
};



/*** METHODS ****************************************************************/

void ReleaseDeviceMemory(unsigned long start,unsigned long length, char *name)
{
    PDEBUG("Releasing memory for %s from %x to %x\n",name,start,start+length);

    release_mem_region(start,length);
}

int RequestDeviceMemory(unsigned long start,unsigned long length,char *name)
{
    PDEBUG("Requesting memory for %s from %x to %x\n",name,start,start+length);

    if(check_mem_region(start,length)){
        printk("stpti: Memory @ %x to %x allready in use \n",(int)start,(int)(start+length));
        return -EBUSY;
    }

    if( request_mem_region(start,length,name) )
        return 0; /* Good */

    printk("stpti: Faiuled memory @ %x to %x allready in use \n",(int)start,(int)(start+length));

    return -EBUSY; /* Failed */
}

/*=============================================================================

   tsmerger_ioctl_init_module

   Initialise the module (initialise the access routines (fops) and set the
   major number. Aquire any resources needed and setup internal structures.

  ===========================================================================*/
static int __init tsmerger_ioctl_init_module(void)
{
    /*** Initialise the module ***/

    int err     = 0;  /* No error */

    dev_t base_dev_no;

    SET_MODULE_OWNER(&tsmerger_ioctl_fops);

    /*** Set up the modules global lock ***/

    sema_init(&(tsmerger_ioctl_lock), 1);

    /*
     * Register the major number. If major = 0 then a major number is auto
     * allocated. The allocated number is returned.
     * The major number can be seen in user space in '/proc/devices'
     */

    if (major == 0) {

        if (alloc_chrdev_region(&base_dev_no, 0, MAX_DEVICES, "tsmerger_ioctl")) {

            err = -EBUSY;
            printk(KERN_ALERT "No major numbers for tsmerger_ioctl by %s (pid %i)\n", current->comm, current->pid);
            goto fail;
        }

        major = MAJOR(base_dev_no);
    }
    else
    {
	base_dev_no = MKDEV(major, 0);

        if (register_chrdev_region(base_dev_no, MAX_DEVICES, "tsmerger_ioctl")) {

            err = -EBUSY;
            printk(KERN_ALERT "No major numbers for tsmerger_ioctl by %s (pid %i)\n", current->comm, current->pid);
            goto fail;
        }
    }

    if (NULL == (tsmerger_ioctl_cdev = cdev_alloc())) {

         err = -EBUSY;
         printk(KERN_ALERT "No major numbers for tsmerger_ioctl by %s (pid %i)\n", current->comm, current->pid);
         goto fail_reg;
    }

    tsmerger_ioctl_cdev->owner = THIS_MODULE;
    tsmerger_ioctl_cdev->ops   = &tsmerger_ioctl_fops;
    kobject_set_name(&(tsmerger_ioctl_cdev->kobj), "tsmerger_ioctl%d", 0);


    /*** Register any register regions ***/


    /*** Do the work ***/

    /* Appears in /var/log/syslog */
    printk(KERN_ALERT "Load module tsmerger_ioctl [%d] by %s (pid %i)\n", major, current->comm, current->pid);


    /*** Register the device nodes for this module ***/

    /* Add the char device structure for this module */

    if (cdev_add(tsmerger_ioctl_cdev, base_dev_no, MAX_DEVICES)) {

        err = -EBUSY;
        printk(KERN_ALERT "Failed to add a charactor device for tsmerger_ioctl by %s (pid %i)\n", current->comm, current->pid);
        goto fail_remap;
    }

    return (0);  /* If we get here then we have succeeded */

    /**************************************************************************/

    /*** Clean up on error ***/

fail_remap:

    /* Remove the char device structure (has not been added) */
    kobject_put(&(tsmerger_ioctl_cdev->kobj));

fail_reg:

    /* Unregister the module */
    unregister_chrdev_region(MKDEV(major, 0), MAX_DEVICES);

fail : return (err);
}


/*=============================================================================

   tsmerger_ioctl_cleanup_module

   Realease any resources allocaed to this module before the module is
   unloaded.

  ===========================================================================*/
static void __exit tsmerger_ioctl_cleanup_module(void)
{
    
#if defined( STPTI_FRONTEND_TSMERGER )
    TSMERGER_ClearAll();            /* This will unmap any registers mapped */
#endif    
    
    /* Remove the char device structure (has been added) */
    cdev_del(tsmerger_ioctl_cdev);

    /* Unregister the module */
    unregister_chrdev_region(MKDEV(major, 0), MAX_DEVICES);

    /* Free any module resources */
    tsmerger_ioctl_CleanUp();

    printk(KERN_ALERT "Unload module tsmerger_ioctl by %s (pid %i)\n", current->comm, current->pid);
}


/*** MODULE LOADING ******************************************************/


/* Tell the module system these are our entry points. */

module_init(tsmerger_ioctl_init_module);
module_exit(tsmerger_ioctl_cleanup_module);
