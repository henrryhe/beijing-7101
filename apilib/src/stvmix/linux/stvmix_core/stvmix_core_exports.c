/*****************************************************************************
 *
 *  Module      : stvmix_core_exports.c
 *  Date        : 30-10-2005
 *  Author      : AYARITAR
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

#include "stvmix.h"

/*** PROTOTYPES **************************************************************/

/*** LOCAL TYPES *********************************************************/


/*** LOCAL CONSTANTS ********************************************************/

/*** LOCAL VARIABLES *********************************************************/

/*** METHODS ****************************************************************/

#define EXPORT_SYMTAB

/* Exported STVMIX functions */
EXPORT_SYMBOL (STVMIX_Init);
EXPORT_SYMBOL (STVMIX_Open);
EXPORT_SYMBOL (STVMIX_GetCapability);
EXPORT_SYMBOL (STVMIX_Close);
EXPORT_SYMBOL (STVMIX_Term);
EXPORT_SYMBOL (STVMIX_Enable);
EXPORT_SYMBOL (STVMIX_Disable);
EXPORT_SYMBOL (STVMIX_DisconnectLayers);
EXPORT_SYMBOL (STVMIX_GetBackgroundColor);
EXPORT_SYMBOL (STVMIX_GetConnectedLayers);
EXPORT_SYMBOL (STVMIX_GetScreenOffset);
EXPORT_SYMBOL (STVMIX_GetScreenParams);
EXPORT_SYMBOL (STVMIX_GetTimeBase);
EXPORT_SYMBOL (STVMIX_SetBackgroundColor);
EXPORT_SYMBOL (STVMIX_SetScreenOffset);
EXPORT_SYMBOL (STVMIX_SetScreenParams);
EXPORT_SYMBOL (STVMIX_SetTimeBase);
EXPORT_SYMBOL (STVMIX_ConnectLayers);



