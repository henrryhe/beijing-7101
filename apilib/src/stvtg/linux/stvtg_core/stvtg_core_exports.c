/*****************************************************************************
 *
 *  Module      : stvtg_core_exports.c
 *  Date        : 15-10-2005
 *  Author      : ATROUSWA
 *  Description : Implementation for exporting STAPI functions
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

#include "stvtg.h"
#include "vtg_hal2.h"

/*** PROTOTYPES **************************************************************/

/*** LOCAL TYPES *********************************************************/


/*** LOCAL CONSTANTS ********************************************************/

/*** LOCAL VARIABLES *********************************************************/

/*** METHODS ****************************************************************/

#define EXPORT_SYMTAB

/* Exported STVTG functions */
EXPORT_SYMBOL (STVTG_Init);
EXPORT_SYMBOL (STVTG_Term);
EXPORT_SYMBOL (STVTG_Open);
EXPORT_SYMBOL (STVTG_Close);
EXPORT_SYMBOL (STVTG_SetMode);
EXPORT_SYMBOL (STVTG_SetOptionalConfiguration);
EXPORT_SYMBOL (STVTG_SetSlaveModeParams);
EXPORT_SYMBOL (STVTG_GetMode);
EXPORT_SYMBOL (STVTG_GetModeSyncParams);
EXPORT_SYMBOL (STVTG_GetVtgId);
EXPORT_SYMBOL (STVTG_GetCapability);
EXPORT_SYMBOL (STVTG_GetOptionalConfiguration);
EXPORT_SYMBOL (STVTG_GetSlaveModeParams);
#if defined (ST_7710) || defined (ST_7100)|| defined (ST_7109)
EXPORT_SYMBOL (stvtg_WA_GNBvd47682_VTimer_Raise);
#endif
EXPORT_SYMBOL (STVTG_CalibrateSyncLevel);
EXPORT_SYMBOL (STVTG_GetLevelSettings);


