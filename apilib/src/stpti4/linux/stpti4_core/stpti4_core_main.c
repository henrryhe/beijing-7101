/*****************************************************************************
 * COPYRIGHT (C) STMicroelectronics 2005.  All rights reserved.
 *
 *  Module      : stpti4_core
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

#include "stpti4_core_proc.h"

/*** MODULE INFO *************************************************************/

MODULE_AUTHOR("STIEGLITZP");
MODULE_DESCRIPTION("");
MODULE_SUPPORTED_DEVICE("");
MODULE_LICENSE("ST Microelectronics");


/*** PROTOTYPES **************************************************************/

static int  stpti4_core_init_module(void);
static void stpti4_core_cleanup_module(void);


/*** MODULE PARAMETERS *******************************************************/
                                                           

/*** GLOBAL VARIABLES *********************************************************/


/*** EXPORTED SYMBOLS ********************************************************/


/*** LOCAL TYPES *********************************************************/


/*** LOCAL CONSTANTS ********************************************************/


/*** LOCAL VARIABLES *********************************************************/


/*** METHODS ****************************************************************/

/*=============================================================================

   stpti4_core_init_module

   Initialise the module (initialise the access routines (fops) and set the
   major number. Aquire any resources needed and setup internal structures.

  ===========================================================================*/
static int __init stpti4_core_init_module(void)
{
    /* Add proc fs for stpti_core */
    stpti4_core_init_proc_fs();
    
    printk(KERN_ALERT "Load module stpti4_core [?]\t\tby %s (pid %i)\n", current->comm, current->pid);
    
    return (0);  /* If we get here then we have succeeded */
}


/*=============================================================================

   stpti4_core_cleanup_module
   
   Realease any resources allocaed to this module before the module is
   unloaded.
   
  ===========================================================================*/
static void __exit stpti4_core_cleanup_module(void)
{                             
    /* Remove proc fs entries. */
    stpti4_core_cleanup_proc_fs();
                                    
    printk(KERN_ALERT "Unload module stpti4_core by %s (pid %i)\n", current->comm, current->pid);
}


/*** MODULE LOADING ******************************************************/


/* Tell the module system these are our entry points. */

module_init(stpti4_core_init_module);
module_exit(stpti4_core_cleanup_module);
