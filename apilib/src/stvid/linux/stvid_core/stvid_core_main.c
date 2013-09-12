/*****************************************************************************
 *
 *  Module      : stvid_core
 *  Date        : 09-01-2006
 *  Author      : C. DAILLY
 *  Description :
 *  Copyright   : STMicroelectronics (c)2006
 *****************************************************************************/

/* Requires   MODULE         defined on the command line */
/* Requires __KERNEL__       defined on the command line */
/* Requires __SMP__          defined on the command line for SMP systems   */
/* Requires EXPORT_SYMTAB    defined on the command line to export symbols */

#include <linux/init.h>    /* Initiliasation support */
#include <linux/module.h>  /* Module support */
#include <linux/kernel.h>  /* Kernel support */
#include <linux/proc_fs.h>

#include "stos.h"
#include "sttbx.h"
#include "stvid_core_proc.h"

/*** MODULE INFO *************************************************************/

MODULE_AUTHOR("Cyril Dailly");
MODULE_DESCRIPTION("");
MODULE_SUPPORTED_DEVICE("");
MODULE_LICENSE("ST Microelectronics");


/*** PROTOTYPES **************************************************************/
struct proc_dir_entry    *STAPI_dir ;

static int  stvid_core_init_module(void);
static void stvid_core_cleanup_module(void);



/*** MODULE PARAMETERS *******************************************************/

/*** GLOBAL VARIABLES *********************************************************/

/*** EXPORTED SYMBOLS ********************************************************/

/*** LOCAL TYPES *********************************************************/

/*** LOCAL CONSTANTS ********************************************************/

/*** LOCAL VARIABLES *********************************************************/

/*** METHODS ****************************************************************/

/*=============================================================================

   stvid_core_init_module

   Initialise the module (initialise the access routines (fops) and set the
   major number. Aquire any resources needed and setup internal structures.

  ===========================================================================*/
static int __init stvid_core_init_module(void)
{
	int Err;

	STTBX_Report((STTBX_REPORT_LEVEL_INFO, "%s(): Initializing STVID CORE Module", __FUNCTION__));

	/* Add proc fs for stvid_core */
	Err = stvid_core_init_proc_fs(STAPI_STAT_DIRECTORY);
	if ( Err != 0 )
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s(): proc_mkdir() error !!!", __FUNCTION__));
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

   stvid_core_cleanup_module

   Realease any resources allocaed to this module before the module is
   unloaded.

  ===========================================================================*/
static void __exit stvid_core_cleanup_module(void)
{
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "stvid_core: Exiting STVID kernel Module"));

    /* Remove proc fs entries. */
    stvid_core_cleanup_proc_fs();
}


/*** MODULE LOADING ******************************************************/


/* Tell the module system these are our entry points. */

module_init(stvid_core_init_module);
module_exit(stvid_core_cleanup_module);
