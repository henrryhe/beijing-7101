/*****************************************************************************

File Name  : dvmtask.h

Description: DVM internal task function

COPYRIGHT (C) 2005 STMicroelectronics

*****************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __DVMTASK_H
#define __DVMTASK_H

/* Includes --------------------------------------------------------------- */
#include "dvmint.h"

/* Exported Constants ----------------------------------------------------- */

/* Exported Variables ----------------------------------------------------- */

/* Exported Macros -------------------------------------------------------- */

/* Exported Types --------------------------------------------------------- */

/* Exported Functions ----------------------------------------------------- */

ST_ErrorCode_t STDVMi_ServiceTaskInit(void);
ST_ErrorCode_t STDVMi_ServiceTaskTerm(void);

STDVMi_PidChangeState_t STDVMi_GetChangePidState(STDVMi_Handle_t *Handle_p);

void STDVMi_SetChangePidState(STDVMi_Handle_t *Handle_p, STDVMi_PidChangeState_t PidChangeState);

ST_ErrorCode_t STDVMi_NotifyPidChange(STDVMi_Handle_t *Handle_p);


#endif /*__DVMTASK_H*/

/* EOF --------------------------------------------------------------------- */



