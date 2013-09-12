/*****************************************************************************
 *
 *  Module      : stvout_core_exports.c
 *  Date        : 23-10-2005
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

#include "stvout.h"

/*** PROTOTYPES **************************************************************/

/*** LOCAL TYPES *********************************************************/

/*** LOCAL CONSTANTS ********************************************************/

/*** LOCAL VARIABLES *********************************************************/

/*** METHODS ****************************************************************/

#define EXPORT_SYMTAB

/* Exported STVOUT functions */
EXPORT_SYMBOL (STVOUT_Init);
EXPORT_SYMBOL (STVOUT_Term);
EXPORT_SYMBOL (STVOUT_Open);
EXPORT_SYMBOL (STVOUT_GetCapability);
EXPORT_SYMBOL (STVOUT_Close);
EXPORT_SYMBOL (STVOUT_SetInputSource);
EXPORT_SYMBOL (STVOUT_GetDacSelect);
EXPORT_SYMBOL (STVOUT_Disable);
EXPORT_SYMBOL (STVOUT_Enable);
EXPORT_SYMBOL (STVOUT_SetOutputParams);
EXPORT_SYMBOL (STVOUT_GetOutputParams);
EXPORT_SYMBOL (STVOUT_SetOption);
EXPORT_SYMBOL (STVOUT_GetOption);
EXPORT_SYMBOL (STVOUT_GetState);
EXPORT_SYMBOL (STVOUT_GetTargetInformation);
EXPORT_SYMBOL (STVOUT_SendData);
EXPORT_SYMBOL (STVOUT_Start);
EXPORT_SYMBOL (STVOUT_SetHDMIOutputType);
EXPORT_SYMBOL (STVOUT_GetStatistics);
EXPORT_SYMBOL (STVOUT_AdjustChromaLumaDelay);
#if defined (STVOUT_HDCP_PROTECTION)
EXPORT_SYMBOL (STVOUT_SetHDCPParams);
EXPORT_SYMBOL (STVOUT_GetHDCPSinkParams);
EXPORT_SYMBOL (STVOUT_UpdateForbiddenKSVs);
EXPORT_SYMBOL (STVOUT_EnableDefaultOutput);
EXPORT_SYMBOL (STVOUT_DisableDefaultOutput);
#endif /* STVOUT_HDCP_PROTECTION */

#ifdef STVOUT_CEC_PIO_COMPARE
EXPORT_SYMBOL (STVOUT_SendCECMessage);
EXPORT_SYMBOL (STVOUT_CECPhysicalAddressAvailable);
EXPORT_SYMBOL (STVOUT_CECSetAdditionalAddress);
#endif
