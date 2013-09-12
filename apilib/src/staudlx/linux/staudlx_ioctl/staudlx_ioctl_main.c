/*****************************************************************************
 *
 *  Module      : staudlx_ioctl
 *  Date        : 11-07-2005
 *  Author      : TAYLORD
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

#include "staudlx_ioctl_types.h"       /* Modules specific defines */
#include "stos.h"

/*** MODULE INFO *************************************************************/

MODULE_AUTHOR("TAYLORD");
MODULE_DESCRIPTION("");
MODULE_SUPPORTED_DEVICE("");
MODULE_LICENSE("ST Microelectronics");


/*** PROTOTYPES **************************************************************/

static int  staudlx_ioctl_init_module(void);
static void staudlx_ioctl_cleanup_module(void);

#include "staudlx_ioctl_fops.h"


#include "staudlx_ioctl_proc.h"



/*** MODULE PARAMETERS *******************************************************/
                                                           
#if 0 /* LDDE2.2 */
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

#ifdef STAUDLX_IOCTL_MAJOR
static unsigned int staudlx_ioctl_major = STAUDLX_IOCTL_MAJOR;
#else
static unsigned int staudlx_ioctl_major = 0;
#endif

#ifndef STAUD_IOCTL_LINUX_DEVICE_NAME
#define STAUDLX_IOCTL_LINUX_DEVICE_NAME "staudlx_ioctl"
#endif

struct semaphore staudlx_ioctl_lock; /* Global lock for the module */

static struct cdev         *staudlx_ioctl_cdev  = NULL;
static struct class_simple *staudlx_ioctl_class = NULL;
                                                                                                       

static struct file_operations staudlx_ioctl_fops = {

    open    : staudlx_ioctl_open,
    read    : staudlx_ioctl_read,
    write   : staudlx_ioctl_write,
    ioctl   : staudlx_ioctl_ioctl,
 /* flush   : staudlx_ioctl_flush,    */
 /* lseek   : staudlx_ioctl_lseek,    */
 /* readdir : staudlx_ioctl_readdir,  */
    poll    : staudlx_ioctl_poll,     
    mmap    : staudlx_ioctl_mmap,
 /* fsync   : staudlx_ioctl_fsync,    */  
    release : staudlx_ioctl_release,     /* close */
};



/*** METHODS ****************************************************************/

/*=============================================================================

   staudlx_ioctl_init_module

   Initialise the module (initialise the access routines (fops) and set the
   major number. Aquire any resources needed and setup internal structures.

  ===========================================================================*/
static int __init staudlx_ioctl_init_module(void)
{
    /*** Initialise the module ***/

    /* Register the device with the kernel */
    if (STLINUX_DeviceRegister(&staudlx_ioctl_fops, 1, STAUDLX_IOCTL_LINUX_DEVICE_NAME,
                             &staudlx_ioctl_major, &staudlx_ioctl_cdev,&staudlx_ioctl_class) != 0)
    {
        printk(KERN_ERR "%s(): LINUX_DeviceRegister() error !!!\n", __FUNCTION__);
        return(-1);
    }

    sema_init(&(staudlx_ioctl_lock), 1);
	
    /* Create the proc filesystem. */
    if (staudlx_ioctl_init_proc_fs()) {
    
	printk(KERN_ERR "Failed to create proc filesystem.\n");
	STLINUX_DeviceUnregister(1, STAUDLX_IOCTL_LINUX_DEVICE_NAME, staudlx_ioctl_major, staudlx_ioctl_cdev, staudlx_ioctl_class);
	return (-1);
    }

    return (0);  /* If we get here then we have succeeded */
    
    /**************************************************************************/
}


/*=============================================================================

   staudlx_ioctl_cleanup_module
   
   Realease any resources allocaed to this module before the module is
   unloaded.
   
  ===========================================================================*/
static void __exit staudlx_ioctl_cleanup_module(void)
{                             
//    int i;
                                     
    /* Remove proc filesystem */
    staudlx_ioctl_cleanup_proc_fs();
                                     
    /* UnRegister the Driver */
    STLINUX_DeviceUnregister(1, STAUDLX_IOCTL_LINUX_DEVICE_NAME, 
    						  staudlx_ioctl_major, staudlx_ioctl_cdev, 
    						  staudlx_ioctl_class);

    /* Free any module resources */           
    staudlx_ioctl_CleanUp();

    printk(KERN_ALERT "Unload module staudlx_ioctl by %s (pid %i)\n", current->comm, current->pid);
}


/*** MODULE LOADING ******************************************************/


/* Tell the module system these are our entry points. */

module_init(staudlx_ioctl_init_module);
module_exit(staudlx_ioctl_cleanup_module);
