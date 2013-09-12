/*****************************************************************************
 *
 *  Module      : stvid_ioctl
 *  Date        : 09-01-2006
 *  Author      : C. DAILLY
 *  Description :
 *  Copyright   : STMicroelectronics (c)2006
 *****************************************************************************/

/* Requires   MODULE   defined on the command line */
/* Requires __KERNEL__ defined on the command line */
/* Requires __SMP__    defined on the command line for SMP systems */

#define _NO_VERSION_
#include <linux/module.h>  /* Module support */
#include <linux/version.h> /* Kernel version */
#include <linux/fs.h>      /* File operations (fops) defines */
#include <linux/ioport.h>  /* Memory/device locking macros   */
#include <linux/poll.h>    /* Poll and select support        */
#include <linux/errno.h>   /* Defines standard error codes */

#include "stvid_ioctl.h"
#include "stvid_ioctl_types.h"
      /* Modules specific defines */

/*** PROTOTYPES **************************************************************/

#include "stvid_ioctl_fops.h"

/* No operation yet */

