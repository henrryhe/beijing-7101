/*****************************************************************************
 *
 *  Module      : stlayer_ioctl
 *  Date        : 13-11-2005
 *  Author      : AYARITAR
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
#include <linux/proc_fs.h>

#include "stlayer_ioctl_types.h"       /* Modules specific defines */
#include "stos.h"

/*** MODULE INFO *************************************************************/

MODULE_LICENSE("ST Microelectronics");

MODULE_AUTHOR("Anis.Tajouri");
MODULE_DESCRIPTION("STBLIT IOCTL STAPI");
MODULE_SUPPORTED_DEVICE("7100,7109,7200");


/*** PROTOTYPES **************************************************************/

static int  stlayer_ioctl_init_module(void);
static void stlayer_ioctl_cleanup_module(void);

#include "stlayer_ioctl_fops.h"




/*** MODULE PARAMETERS *******************************************************/

/*** GLOBAL VARIABLES *********************************************************/

/*** EXPORTED SYMBOLS ********************************************************/

/*** LOCAL TYPES *********************************************************/

/*** LOCAL CONSTANTS ********************************************************/

/*** LOCAL VARIABLES *********************************************************/

static unsigned int             stlayer_ioctl_major;

struct semaphore stlayer_ioctl_lock; /* Global lock for the module */

static struct cdev         *stlayer_ioctl_cdev  = NULL;
static struct class_simple *stlayer_ioctl_class = NULL;
struct proc_dir_entry *STAPI_dir ;


static struct file_operations stlayer_ioctl_fops = {

    open    : stlayer_ioctl_open,
    ioctl   : stlayer_ioctl_ioctl,
    mmap    : STLINUX_MMapMethod,     /*  mmap     */
    release : stlayer_ioctl_release,     /* close */
};



/*** METHODS ****************************************************************/

/*=============================================================================

   stlayer_ioctl_init_module

   Initialise the module (initialise the access routines (fops) and set the
   major number. Aquire any resources needed and setup internal structures.

  ===========================================================================*/
static int __init stlayer_ioctl_init_module(void)
{
 STTBX_Print(("stlayer_ioctl_init_module: Initializing STLAYER kernel Module\n"));
#if defined(STLAYER_IOCTL_MAJOR)
    stlayer_ioctl_major = STLAYER_IOCTL_MAJOR;
#else
    stlayer_ioctl_major = 0;
#endif


    /* Register the device with the kernel */
    if (STLINUX_DeviceRegister(&stlayer_ioctl_fops, 1, STLAYER_IOCTL_LINUX_DEVICE_NAME,
                             &stlayer_ioctl_major, &stlayer_ioctl_cdev, &stlayer_ioctl_class) != 0)
    {
        STTBX_Print((KERN_ERR "%s(): LINUX_DeviceRegister() error !!!\n", __FUNCTION__));
        return(-1);
    }

    /*** Set up the modules global lock ***/

    sema_init(&(stlayer_ioctl_lock), 1);

    /* Creation if not existing of /proc/STAPI directory */
    STAPI_dir = proc_mkdir(STAPI_STAT_DIRECTORY, NULL);
    if ( STAPI_dir == NULL )
    {
        STTBX_Print((KERN_ERR "stlayer_ioctl_init_module: proc_mkdir() error !!!\n"));
        STLINUX_DeviceUnregister(1, STLAYER_IOCTL_LINUX_DEVICE_NAME, stlayer_ioctl_major, stlayer_ioctl_cdev, stlayer_ioctl_class);
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

   stlayer_ioctl_cleanup_module

   Realease any resources allocaed to this module before the module is
   unloaded.

  ===========================================================================*/
static void __exit stlayer_ioctl_cleanup_module(void)
{
    STTBX_Print((KERN_INFO "stlayer_ioctl_cleanup_module: Exiting STLAYER_IOCTL kernel Module\n"));
    STLINUX_DeviceUnregister(1, STLAYER_IOCTL_LINUX_DEVICE_NAME, stlayer_ioctl_major, stlayer_ioctl_cdev, stlayer_ioctl_class);
}


/*** MODULE LOADING ******************************************************/


/* Tell the module system these are our entry points. */

module_init(stlayer_ioctl_init_module);
module_exit(stlayer_ioctl_cleanup_module);
