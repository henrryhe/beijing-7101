/*****************************************************************************
 *
 *  Module      : staudlx_ioctl_proc.c
 *  Date        : 11-07-2005
 *  Author      : TAYLORD
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

#include "staudlx_ioctl_types.h"       /* Modules specific defines */

#include "staudlx_ioctl_proc.h"        /* Prototytpes for this compile unit */

/*** PROTOTYPES **************************************************************/

static int staudlx_ioctl_read_proc(char *page, char **start, off_t off,int count, int *eof, void *data);


#define MAX_LENGTH_TO_COPY 4095
/*** LOCAL TYPES *********************************************************/


/*** LOCAL CONSTANTS ********************************************************/

/*** LOCAL VARIABLES *********************************************************/

struct proc_dir_entry *staudlx_ioctl_proc_root_dir = NULL; /* All proc files for this module will appear in here. */
struct proc_dir_entry *staudlx_ioctl_proc_file = NULL;/* This is a read only for info */

/*** METHODS ****************************************************************/

static int staudlx_ioctl_read_proc(char *page, char **start, off_t off,int count, int *eof, void *data)
{

    strncpy(page, "Here we could output some useful info such as register dumps\n",MAX_LENGTH_TO_COPY);
    *eof = 1;

    /*
     * WARNING: If the length is larger than 4096 then there is more work
     * to do. See O'Reilly
     */

    return (strlen (page));
}

/*=============================================================================

   staudlx_ioctl_init_proc_fs

   Initialise the proc file system
   Called from module initialisation routine.

   This function is called from  staudlx_ioctl_init_module() and is not reentrant.

  ===========================================================================*/
int staudlx_ioctl_init_proc_fs   ()
{
    staudlx_ioctl_proc_root_dir = proc_mkdir("staudlx_ioctl",NULL);

    if (staudlx_ioctl_proc_root_dir == NULL) {

        goto fault_no_dir;
    }

    staudlx_ioctl_proc_file = create_proc_read_entry("info",
                                       0444,
                                       staudlx_ioctl_proc_root_dir,
                                       staudlx_ioctl_read_proc,
                                       NULL);

    if (staudlx_ioctl_proc_file == NULL) {

        goto fault;
    }

    return 0;  /* If we get here then we have succeeded */

    /**************************************************************************/

    /*** Clean up on error ***/

 fault:
    remove_proc_entry("staudlx_ioctl",NULL);

 fault_no_dir:
    return (-1);
}

/*=============================================================================

   staudlx_ioctl_cleanup_proc_fs

   Remove the proc file system and and clean up.
   Called from module cleanup routine.

   This function is called from  staudlx_ioctl_cleanup_module() and is not reentrant.

  ===========================================================================*/
int staudlx_ioctl_cleanup_proc_fs   ()
{
    int err = 0;

    remove_proc_entry("info",staudlx_ioctl_proc_root_dir);
    remove_proc_entry("staudlx_ioctl",NULL);

    return (err);
}



