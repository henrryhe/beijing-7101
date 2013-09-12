/*****************************************************************************
 *
 *  Module      : stvtg_ioctl
 *  Date        : 15-10-2005
 *  Author      : ATROUSWA
 *  Description :
 *  Copyright   : STMicroelectronics (c)2005
 *****************************************************************************/

/* Requires   MODULE   defined on the command line */
/* Requires __KERNEL__ defined on the command line */
/* Requires __SMP__    defined on the command line for SMP systems */

#define _NO_VERSION_
#include <linux/module.h>  /* Module support */
#include <linux/version.h> /* Kernel version */
#include <linux/fs.h>      /* File operations (fops) defines */
#include <linux/ioport.h>  /* Memory/device locking macros   */
#include <asm/uaccess.h>   /* User space access methods      */

#include <linux/errno.h>   /* Defines standard error codes */

#include "stvtg_ioctl.h"
#include "stvtg_ioctl_types.h"
      /* Modules specific defines */
#include "stvtg.h"


/*** PROTOTYPES **************************************************************/


#include "stvtg_ioctl_fops.h"

/*** LOCAL TYPES *********************************************************/



/*** LOCAL VARIABLES *********************************************************/
/*** METHODS ****************************************************************/

/*=============================================================================

   stvtg_ioctl_read

   Pass data upto user space. This call can deshedule if there is no data
   available.

   This function can deschedule and MUST be reentrant.

  ===========================================================================*/
ssize_t stvtg_ioctl_read   (struct file *file, char *buf, size_t count, loff_t *offset)
{
 return(0);
}



