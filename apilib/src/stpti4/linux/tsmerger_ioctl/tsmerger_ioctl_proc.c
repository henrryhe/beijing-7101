/*****************************************************************************
 *
 *  Module      : tsmerger_ioctl_proc.c
 *  Date        : 24-07-2005
 *  Author      : STIEGLITZP
 *  Description : procfs implementation
 *  Copyright   : STMicroelectronics (c)2005
 *****************************************************************************/

/* Requires   MODULE   defined on the command line */
/* Requires __KERNEL__ defined on the command line */
/* Requires __SMP__    defined on the command line for SMP systems */

#define _NO_VERSION_
#include <linux/module.h>  /* Module support */
#include <linux/version.h> /* Kernel version */
#include <linux/proc_fs.h>

#include <linux/errno.h>   /* Defines standard error codes */

#include "tsmerger_ioctl_types.h"       /* Modules specific defines */

#include "tsmerger_ioctl_proc.h"        /* Prototytpes for this compile unit */

#include "stos.h"

#include "tsmerger.h"
#include "stpti.h"

/*** PROTOTYPES **************************************************************/

static int tsmerger_ioctl_read_proc(char *page, char **start, off_t off,int count, int *eof, void *data);

/*** LOCAL TYPES *********************************************************/


/*** LOCAL CONSTANTS ********************************************************/

/*** LOCAL VARIABLES *********************************************************/

struct proc_dir_entry *tsmerger_ioctl_proc_root_dir = NULL; /* All proc files for this module will appear in here. */
struct proc_dir_entry *tsmerger_ioctl_proc_file = NULL;/* This is a read only for info */


/*** METHODS ****************************************************************/

static int tsmerger_ioctl_read_proc(char *buf, char **start, off_t off,int count, int *eof, void *data)
{
    int len = 0;

    #if defined( STPTI_FRONTEND_TSMERGER )

        len += sprintf( buf+len,"Not available.\n");
    
    #endif // #if defined( STPTI_FRONTEND_TSMERGER ) ... #endif

    *eof = 1;
    return len;
}

/*=============================================================================

   tsmerger_ioctl_init_proc_fs

   Initialise the proc file system
   Called from module initialisation routine.

   This function is called from  tsmerger_ioctl_init_module() and is not reentrant.

  ===========================================================================*/
int tsmerger_ioctl_init_proc_fs   ()
{
    #if defined( STPTI_FRONTEND_TSMERGER )

        tsmerger_ioctl_proc_root_dir = proc_mkdir("tsmerger_ioctl",NULL);
        if (tsmerger_ioctl_proc_root_dir == NULL) {

            goto fault_no_dir;
        }

        tsmerger_ioctl_proc_file = create_proc_read_entry("registers",
                                           0444,
                                           tsmerger_ioctl_proc_root_dir,
                                           tsmerger_ioctl_read_proc,
                                           NULL);

        if (tsmerger_ioctl_proc_file == NULL) {

            goto fault;
        }

    #endif // #if defined( STPTI_FRONTEND_TSMERGER ) ... #endif

    return 0;  /* If we get here then we have succeeded */

    /**************************************************************************/

    /*** Clean up on error ***/

 fault:
    #if defined( STPTI_FRONTEND_TSMERGER )

        remove_proc_entry("tsmerger_ioctl",NULL);

    #endif // #if defined( STPTI_FRONTEND_TSMERGER ) ... #endif

 fault_no_dir:
    return (-1);
}

/*=============================================================================

   tsmerger_ioctl_cleanup_proc_fs

   Remove the proc file system and and clean up.
   Called from module cleanup routine.

   This function is called from  tsmerger_ioctl_cleanup_module() and is not reentrant.

  ===========================================================================*/
int tsmerger_ioctl_cleanup_proc_fs   ()
{
    int err = 0;

    #if defined( STPTI_FRONTEND_TSMERGER )

        remove_proc_entry("registers",tsmerger_ioctl_proc_root_dir);
        remove_proc_entry("tsmerger_ioctl",NULL);

    #endif // #if defined( STPTI_FRONTEND_TSMERGER ) ... #endif

    return (err);
}



