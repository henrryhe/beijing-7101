/*****************************************************************************
 * COPYRIGHT (C) STMicroelectronics 2005.  All rights reserved.
 *
 *  Module      : stpti4_ioctl
 *  Date        : 17-04-2005
 *  Author      : STIEGLITZP
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

#include "linuxcommon.h"
#include "stpti.h"
#include "pti_linux.h"

/* UTS_RELEASE             - Kernel version  (text) */
/* LINUX_VERSION_CODE      - Kernel version  (hex)  */

#include <linux/errno.h>   /* Defines standard error codes */
#include <linux/sched.h>   /* Defines pointer (current) to current task */

#include "stpti4_ioctl_types.h"       /* Modules specific defines */

#include "stpti4_ioctl_cfg.h"  /* PTI specific configurations. */

/*** MODULE INFO *************************************************************/

MODULE_AUTHOR("STIEGLITZP");
MODULE_DESCRIPTION("");
MODULE_SUPPORTED_DEVICE("");
MODULE_LICENSE("ST Microelectronics");


/*** PROTOTYPES **************************************************************/
static int  stpti4_ioctl_init_module(void);
static void stpti4_ioctl_cleanup_module(void);

#include "stpti4_ioctl_fops.h"


#include "stpti4_ioctl_utils.h"

/*** GLOBAL VARIABLES *********************************************************/

#if defined( STPTI_TCLOADER )

/* If STPTI_TCLOADER is defined then the user is selecting which loader to use */
#define STPTI_TCLOADER_STRING     "STPTI_TCLoaderUSER"

#else

/* If STPTI_TCLOADER is not defined then we select a default loader depending on the environment */
#if defined(STPTI_ARCHITECTURE_PTI4)
#if defined( STPTI_DVB_SUPPORT )
#define STPTI_TCLOADER            STPTI_DVBTCLoader
#define STPTI_TCLOADER_STRING     "STPTI_DVBTCLoader"
#endif

#else
#if defined(STPTI_ARCHITECTURE_PTI4_LITE)
#if defined( STPTI_DVB_SUPPORT )
#define STPTI_TCLOADER            STPTI_DVBTCLoaderL
#define STPTI_TCLOADER_STRING     "STPTI_DVBTCLoaderL"
#endif

#else
#if defined( STPTI_ARCHITECTURE_PTI4_SECURE_LITE ) && ( STPTI_ARCHITECTURE_PTI4_SECURE_LITE == 1 )
#if defined( STPTI_DVB_SUPPORT )
#define STPTI_TCLOADER            STPTI_DVBTCLoaderSL
#define STPTI_TCLOADER_STRING     "STPTI_DVBTCLoaderSL"
#endif

#else
#if defined( STPTI_ARCHITECTURE_PTI4_SECURE_LITE ) && ( STPTI_ARCHITECTURE_PTI4_SECURE_LITE == 2 ) && defined ( STPTI_FRONTEND_HYBRID )
#if defined( STPTI_DVB_SUPPORT )
#define STPTI_TCLOADER            STPTI_DVBTCLoaderSL2
#define STPTI_TCLOADER_STRING     "STPTI_DVBTCLoaderSL2"
#endif

#else
#if defined( STPTI_ARCHITECTURE_PTI4_SECURE_LITE ) && ( STPTI_ARCHITECTURE_PTI4_SECURE_LITE == 2 ) && defined ( STPTI_FRONTEND_TSMERGER )
#if defined( STPTI_DVB_SUPPORT )
#define STPTI_TCLOADER            STPTI_DVBTCLoaderSL3
#define STPTI_TCLOADER_STRING     "STPTI_DVBTCLoaderSL3"
#endif

#else
#error No target device selected by environment (variable DVD_FRONTEND), or selected device not supported
#endif
#endif
#endif
#endif
#endif

#endif  /* #if !defined( STPTI_TCLOADER ) */


#if defined(ST_5528)
#define LINUX_PTIA_INTERRUPT 0xA4
#define LINUX_PTIB_INTERRUPT 0xA5

PTI_Device_Info PTI_Devices[STPTI_CORE_COUNT] = { { "PTI0",
                                                    ST5528_PTIA_BASE_ADDRESS,
                                                    sizeof(TCDevice_t),
                                                    LINUX_PTIA_INTERRUPT,
                                                    STPTI_TCLOADER, STPTI_TCLOADER_STRING,
                                                    STPTI_DEVICE_PTI_4},
                                                  { "PTI1",
                                                    ST5528_PTIB_BASE_ADDRESS,
                                                    0,
                                                    sizeof(TCDevice_t),
                                                    LINUX_PTIB_INTERRUPT,
                                                    STPTI_TCLOADER, STPTI_TCLOADER_STRING,
                                                    STPTI_DEVICE_PTI_4},
                                                    };
#else
#if defined(ST_7100)
PTI_Device_Info PTI_Devices[STPTI_CORE_COUNT] = { { "PTI0",
                                                    ST7100_PTI_BASE_ADDRESS,
                                                    sizeof(TCDevice_t),
                                                    ST7100_PTI_INTERRUPT,
                                                    STPTI_TCLOADER, STPTI_TCLOADER_STRING,
                                                    STPTI_DEVICE_PTI_4},
                                                    };
#else
#if defined(ST_7109)
PTI_Device_Info PTI_Devices[STPTI_CORE_COUNT] = { { "PTI0",
                                                    ST7109_PTIA_BASE_ADDRESS,
                                                    sizeof(TCDevice_t),
                                                    ST7109_PTIA_INTERRUPT,
                                                    STPTI_TCLOADER, STPTI_TCLOADER_STRING,
                                                    STPTI_DEVICE_PTI_4},
                                                  { "PTI1",
                                                    ST7109_PTIB_BASE_ADDRESS,
                                                    sizeof(TCDevice_t),
                                                    ST7109_PTIB_INTERRUPT,
                                                    STPTI_TCLOADER, STPTI_TCLOADER_STRING,
                                                    STPTI_DEVICE_PTI_4},
                                                    };
#else
#if defined(ST_7111)
PTI_Device_Info PTI_Devices[STPTI_CORE_COUNT] = { { "PTI0",
                                                    ST7111_PTIA_BASE_ADDRESS,
                                                    sizeof(TCDevice_t),
                                                    ST7111_PTIA_INTERRUPT,
                                                    STPTI_TCLOADER, STPTI_TCLOADER_STRING,
                                                    STPTI_DEVICE_PTI_4},
                                                    };
#else
#if defined(ST_7200)
PTI_Device_Info PTI_Devices[STPTI_CORE_COUNT] = { { "PTI0",
                                                    ST7200_PTIA_BASE_ADDRESS,
                                                    sizeof(TCDevice_t),
                                                    ST7200_PTIA_INTERRUPT,
                                                    STPTI_TCLOADER, STPTI_TCLOADER_STRING,
                                                    STPTI_DEVICE_PTI_4},
                                                  { "PTI1",
                                                    ST7200_PTIB_BASE_ADDRESS,
                                                    sizeof(TCDevice_t),
                                                    ST7200_PTIB_INTERRUPT,
                                                    STPTI_TCLOADER, STPTI_TCLOADER_STRING,
                                                    STPTI_DEVICE_PTI_4},
                                                    };
#else
#error No target device selected by environment (variable DVD_FRONTEND), or selected device not supported
#endif
#endif
#endif
#endif
#endif

/*** EXPORTED SYMBOLS ********************************************************/

#if defined (EXPORT_SYMTAB)

/* EXPORT_SYMBOL(func);           - With    version info */
/* EXPORT_SYMBOL_NOVERS(func);    - Without version info */

#endif


/*** LOCAL TYPES *********************************************************/


/*** LOCAL CONSTANTS ********************************************************/

#define MAX_DEVICES    4


/*** LOCAL VARIABLES *********************************************************/

/* in later version of STCOMMON the manifest constant STPTI4_IOCTL_MAJOR are defined and we should use that */
#if defined( STPTI4_IOCTL_MAJOR )
static unsigned int major = STPTI4_IOCTL_MAJOR;
#else
static unsigned int major = 0;
#endif

struct semaphore stpti4_ioctl_lock; /* Global lock for the module */

static struct cdev         *stpti4_ioctl_cdev  = NULL;

static struct file_operations stpti4_ioctl_fops = {
    open    : stpti4_ioctl_open,
    ioctl   : stpti4_ioctl_ioctl,
    mmap    : stpti4_ioctl_mmap,
    release : stpti4_ioctl_release,     /* close */
};



/*** METHODS ****************************************************************/

    
/*=============================================================================

   STPTI_GetDefaultLoaderFunctionPointer

   Returns a default Loader Function Pointer for calling STPTI_Init() directly
   in kernelspace.

  ===========================================================================*/

STPTI_LoaderFunctionPointer_t STPTI_GetDefaultLoaderFunctionPointer(void)
{
    return(STPTI_TCLOADER);
}
EXPORT_SYMBOL(STPTI_GetDefaultLoaderFunctionPointer);
 
   
/*=============================================================================

   stpti4_ioctl_init_module

   Initialise the module (initialise the access routines (fops) and set the
   major number. Aquire any resources needed and setup internal structures.

  ===========================================================================*/
static int __init stpti4_ioctl_init_module(void)
{
    /*** Initialise the module ***/

    int err     = 0;  /* No error */

    dev_t base_dev_no;

    SET_MODULE_OWNER(&stpti4_ioctl_fops);

    /*** Set up the modules global lock ***/

    sema_init(&(stpti4_ioctl_lock), 1);

    /*
     * Register the major number. If major = 0 then a major number is auto
     * allocated. The allocated number is returned.
     * The major number can be seen in user space in '/proc/devices'
     */

    if (major == 0) {

        if (alloc_chrdev_region(&base_dev_no, 0, MAX_DEVICES, "stpti4_ioctl")) {

            err = -EBUSY;
            printk(KERN_ALERT "No major numbers for stpti4_ioctl by %s (pid %i)\n", current->comm, current->pid);
            goto fail;
        }

        major = MAJOR(base_dev_no);
    }
    else
    {
	base_dev_no = MKDEV(major, 0);

        if (register_chrdev_region(base_dev_no, MAX_DEVICES, "stpti4_ioctl")) {

            err = -EBUSY;
            printk(KERN_ALERT "No major numbers for stpti4_ioctl by %s (pid %i)\n", current->comm, current->pid);
            goto fail;
        }
    }

    if (NULL == (stpti4_ioctl_cdev = cdev_alloc())) {

         err = -EBUSY;
         printk(KERN_ALERT "No major numbers for stpti4_ioctl by %s (pid %i)\n", current->comm, current->pid);
         goto fail_reg;
    }

    stpti4_ioctl_cdev->owner = THIS_MODULE;
    stpti4_ioctl_cdev->ops   = &stpti4_ioctl_fops;
    kobject_set_name(&(stpti4_ioctl_cdev->kobj), "stpti4_ioctl%d", 0);


    /*** Do the work ***/

    /* Appears in /var/log/syslog */
    printk(KERN_ALERT "Load module stpti4_ioctl [%d]\t\tby %s (pid %i)\n", major,current->comm, current->pid);

    /*** Register the device nodes for this module ***/

    /* Add the char device structure for this module */

    if (cdev_add(stpti4_ioctl_cdev, base_dev_no, MAX_DEVICES)) {

        err = -EBUSY;
        printk(KERN_ALERT "Failed to add a charactor device for stpti4_ioctl by %s (pid %i)\n", current->comm, current->pid);
        goto fail_remap;
    }

    return (0);  /* If we get here then we have succeeded */

    /**************************************************************************/

    /*** Clean up on error ***/

fail_remap:

    /* Remove the char device structure (has not been added) */
    kobject_put(&(stpti4_ioctl_cdev->kobj));

fail_reg:

    /* Unregister the module */
    unregister_chrdev_region(MKDEV(major, 0), MAX_DEVICES);

fail : return (err);
}


/*=============================================================================

   stpti4_ioctl_cleanup_module

   Realease any resources allocaed to this module before the module is
   unloaded.

  ===========================================================================*/
static void __exit stpti4_ioctl_cleanup_module(void)
{
    /* If a process exits without calling STPTI_Term()
       for all initialised PTIs then this call make sure that
       all is tidied up. */
    STPTI_TermAll();

    /* Remove the char device structure (has been added) */
    cdev_del(stpti4_ioctl_cdev);

    /* Unregister the module */
    unregister_chrdev_region(MKDEV(major, 0), MAX_DEVICES);

    /* Free any module resources */
    stpti4_ioctl_CleanUp();

    printk(KERN_ALERT "Unload module stpti4_ioctl by %s (pid %i)\n", current->comm, current->pid);
}


/*** MODULE LOADING ******************************************************/


/* Tell the module system these are our entry points. */

module_init(stpti4_ioctl_init_module);
module_exit(stpti4_ioctl_cleanup_module);
