/*****************************************************************************
 *
 *  Module      : stvidtest_core_exports.c
 *  Date        : 09-01-2006
 *  Author      : C. DAILLY
 *  Description : Implementation for exporting STAPI functions
 *  Copyright   : STMicroelectronics (c)2006
 *****************************************************************************/


/* Requires   MODULE         defined on the command line */
/* Requires __KERNEL__       defined on the command line */
/* Requires __SMP__          defined on the command line for SMP systems   */
/* Requires EXPORT_SYMTAB    defined on the command line to export symbols */

#include <linux/init.h>    /* Initiliasation support */
#include <linux/module.h>  /* Module support */
#include <linux/kernel.h>  /* Kernel support */
#include <linux/version.h> /* Kernel version */

#include "stvidtest.h"
#include "vid_util.h"

/*** PROTOTYPES **************************************************************/

/*** LOCAL TYPES *********************************************************/


/*** LOCAL CONSTANTS ********************************************************/

/*** LOCAL VARIABLES *********************************************************/

/*** METHODS ****************************************************************/

#define EXPORT_SYMTAB
EXPORT_SYMBOL (VID_KillTask);
EXPORT_SYMBOL (VIDDecodeFromMemory);
EXPORT_SYMBOL (VID_MemInject);

/* end of stvidtest_core_exports.c */
