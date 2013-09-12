/*****************************************************************************
 *
 *  Module      : stdenc_core_exports.c
 *  Date        : 21-10-2005
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

#include "stdenc.h"

/*** PROTOTYPES **************************************************************/

/*** LOCAL TYPES *********************************************************/


/*** LOCAL CONSTANTS ********************************************************/

/*** LOCAL VARIABLES *********************************************************/

/*** METHODS ****************************************************************/

#define EXPORT_SYMTAB

/* Exported STDENC functions */

EXPORT_SYMBOL (STDENC_Init);
EXPORT_SYMBOL (STDENC_Open);
EXPORT_SYMBOL (STDENC_GetCapability);
EXPORT_SYMBOL (STDENC_OrReg8);
EXPORT_SYMBOL (STDENC_Close);
EXPORT_SYMBOL (STDENC_CheckHandle);
EXPORT_SYMBOL (STDENC_MaskReg8);
EXPORT_SYMBOL (STDENC_RegAccessLock);
EXPORT_SYMBOL (STDENC_AndReg8);
EXPORT_SYMBOL (STDENC_ReadReg8);
EXPORT_SYMBOL (STDENC_WriteReg8);
EXPORT_SYMBOL (STDENC_SetEncodingMode);
EXPORT_SYMBOL (STDENC_Term);

EXPORT_SYMBOL (STDENC_GetModeOption);
EXPORT_SYMBOL (STDENC_RegAccessUnlock);
EXPORT_SYMBOL (STDENC_SetModeOption);
EXPORT_SYMBOL (STDENC_SetColorBarPattern);
EXPORT_SYMBOL (STDENC_GetEncodingMode);




