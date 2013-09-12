/*****************************************************************************
 *
 *  Module      : stvidtest_core
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

#include "vid_util.h"

/*** MODULE INFO *************************************************************/

MODULE_AUTHOR("Cyril Dailly");
MODULE_DESCRIPTION("");
MODULE_SUPPORTED_DEVICE("");
MODULE_LICENSE("ST Microelectronics");


/*** PROTOTYPES **************************************************************/
struct proc_dir_entry    *STAPI_dir ;

static int  stvidtest_core_init_module(void);
static void stvidtest_core_cleanup_module(void);

/*** MODULE PARAMETERS *******************************************************/

/*** GLOBAL VARIABLES *********************************************************/

/*** EXPORTED SYMBOLS ********************************************************/

/*** LOCAL TYPES *********************************************************/

/*** LOCAL CONSTANTS ********************************************************/

/*** LOCAL VARIABLES *********************************************************/

/*** METHODS ****************************************************************/

/*=============================================================================

   stvidtest_core_init_module

   Initialise the module (initialise the access routines (fops) and set the
   major number. Aquire any resources needed and setup internal structures.

  ===========================================================================*/
static int __init stvidtest_core_init_module(void)
{
    /*** Initialise the module ***/

    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "%s(): Initializing STVIDTEST CORE Module", __FUNCTION__));

    /* Creation if not existing of /proc/STAPI directory */
    STAPI_dir = proc_mkdir(STAPI_STAT_DIRECTORY, NULL);
    if ( STAPI_dir == NULL )
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s(): proc_mkdir() error !!!", __FUNCTION__));
        return( -1 );
    }

    VID_Inj_RegisterCmd();

    return (0);

    /* KERN_ERR: 3
       KERN_WARNING: 4
       KERN_NOTICE: 5
       KERN_INFO: 6
       KERN_DEBUG: 7 */
}


/*=============================================================================

   stvidtest_core_cleanup_module

   Realease any resources allocated to this module before the module is
   unloaded.

  ===========================================================================*/
static void __exit stvidtest_core_cleanup_module(void)
{
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "stvidtest_core: Exiting STVIDTEST kernel Module"));
}


/*** MODULE LOADING ******************************************************/


/* Tell the module system these are our entry points. */

module_init(stvidtest_core_init_module);
module_exit(stvidtest_core_cleanup_module);
