/*****************************************************************************
 *
 *  Module      : stdenc_ioctl
 *  Date        : 21-10-2005
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
#include <linux/proc_fs.h>

/* UTS_RELEASE             - Kernel version  (text) */
/* LINUX_VERSION_CODE      - Kernel version  (hex)  */

#include <linux/errno.h>   /* Defines standard error codes */
#include <linux/sched.h>   /* Defines pointer (current) to current task */

#include "stdenc_ioctl_types.h"       /* Modules specific defines */
#include "stos.h"


/*** MODULE INFO *************************************************************/

MODULE_AUTHOR("AYARITAR");
MODULE_DESCRIPTION("");
MODULE_SUPPORTED_DEVICE("");
MODULE_LICENSE("ST Microelectronics");


/*** PROTOTYPES **************************************************************/

static int  stdenc_ioctl_init_module(void);
static void stdenc_ioctl_cleanup_module(void);

#include "stdenc_ioctl_fops.h"




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


 unsigned int             stdenc_ioctl_major;
 struct proc_dir_entry    *STAPI_dir ;


struct semaphore stdenc_ioctl_lock; /* Global lock for the module */

static struct cdev         *stdenc_ioctl_cdev  = NULL;
static struct class_simple *stdenc_ioctl_class = NULL;


static struct file_operations stdenc_ioctl_fops = {

    open    : stdenc_ioctl_open,
    read    : stdenc_ioctl_read,
    write   : stdenc_ioctl_write,
    ioctl   : stdenc_ioctl_ioctl,
 /* flush   : stdenc_ioctl_flush,    */
 /* lseek   : stdenc_ioctl_lseek,    */
 /* readdir : stdenc_ioctl_readdir,  */
 /* mmap    : STLINUX_MMapMethod,    */
 /* fsync   : stdenc_ioctl_fsync,    */
    release : stdenc_ioctl_release,     /* close */
};



/*** METHODS ****************************************************************/

/*=============================================================================

   stdenc_ioctl_init_module

   Initialise the module (initialise the access routines (fops) and set the
   major number. Aquire any resources needed and setup internal structures.

  ===========================================================================*/
static int __init stdenc_ioctl_init_module(void)
{
    /*** Initialise the module ***/

    STTBX_Print((KERN_INFO "stdenc_ioctl_init_module: Initializing STDENC IOCTL Module\n"));
    #if defined(STDENC_IOCTL_MAJOR)
        stdenc_ioctl_major = STDENC_IOCTL_MAJOR;
    #else
        stdenc_ioctl_major = 0;
    #endif

    /* Register the device with the kernel */
    if (STLINUX_DeviceRegister(&stdenc_ioctl_fops, 1, STDENC_IOCTL_LINUX_DEVICE_NAME,
                             &stdenc_ioctl_major, &stdenc_ioctl_cdev, &stdenc_ioctl_class) != 0)
    {
        STTBX_Print((KERN_ERR "%s(): LINUX_DeviceRegister() error !!!\n", __FUNCTION__));
        return(-1);
    }

    /*** Set up the modules global lock ***/

    sema_init(&(stdenc_ioctl_lock), 1);


    /* Creation if not existing of /proc/STAPI directory */
    STAPI_dir = proc_mkdir(STAPI_STAT_DIRECTORY, NULL);
    if ( STAPI_dir == NULL )
    {
        STTBX_Print((KERN_ERR "stdenc_ioctl_init_module: proc_mkdir() error !!!\n"));
        STLINUX_DeviceUnregister(1, STDENC_IOCTL_LINUX_DEVICE_NAME, stdenc_ioctl_major, stdenc_ioctl_cdev, stdenc_ioctl_class);
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

   stdenc_ioctl_cleanup_module

   Realease any resources allocaed to this module before the module is
   unloaded.

  ===========================================================================*/
static void __exit stdenc_ioctl_cleanup_module(void)
{
    STTBX_Print((KERN_INFO "stdenc_ioctl: Exiting STDENC kernel Module\n"));

    /* Unregister the device with the kernel */
    STLINUX_DeviceUnregister(1, STDENC_IOCTL_LINUX_DEVICE_NAME, stdenc_ioctl_major, stdenc_ioctl_cdev, stdenc_ioctl_class);

}


/*** MODULE LOADING ******************************************************/


/* Tell the module system these are our entry points. */

module_init(stdenc_ioctl_init_module);
module_exit(stdenc_ioctl_cleanup_module);
