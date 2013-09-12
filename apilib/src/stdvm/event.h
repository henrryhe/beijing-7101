/*****************************************************************************

File Name  : event.h

Description: Event notification functions of STDVM header

COPYRIGHT (C) 2005 STMicroelectronics

*****************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __EVENT_H
#define __EVENT_H

/* Includes --------------------------------------------------------------- */
#include "dvmint.h"

/* Exported Constants ----------------------------------------------------- */

/* Exported Variables ----------------------------------------------------- */

/* Exported Macros -------------------------------------------------------- */

/* Exported Types --------------------------------------------------------- */

/* Exported Functions ----------------------------------------------------- */

ST_ErrorCode_t STDVMi_EventInit(ST_DeviceName_t     DVMDeviceName,
                                ST_DeviceName_t     PRMDeviceName,
                                ST_DeviceName_t     MP3PBDeviceName,
                                STDVM_InitParams_t *InitParams_p);
ST_ErrorCode_t STDVMi_EventTerm(ST_DeviceName_t     DVMDeviceName,
                                ST_DeviceName_t     PRMDeviceName,
                                ST_DeviceName_t     MP3PBDeviceName,
                                BOOL                ForceTerminate);

ST_ErrorCode_t STDVMi_NotifyEvent(STDVM_Event_t Event, STDVMi_Handle_t *STDVMi_Handle_p);
ST_ErrorCode_t STDVMi_NotifyErrorEvent(STDVM_Event_t Event, STDVM_Errors_t Error, STDVMi_Handle_t *STDVMi_Handle_p);

U32 STDVMi_GetReadWriteThresholdInMs(void);
U32 STDVMi_GetReadWriteThresholdOKInMs(void);
U32 STDVMi_GetMinDiskSpace(void);

#endif /*__EVENT_H*/

/* EOF --------------------------------------------------------------------- */

