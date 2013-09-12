/******************************************************************************
COPYRIGHT (C) STMicroelectronics 2005.  All rights reserved.

   File Name: pti_evt.h
 Description: events for stpti

******************************************************************************/


#ifndef __PTI_EVT_H
 #define __PTI_EVT_H


/* Includes ------------------------------------------------------------ */


#include "pti_loc.h"


/* Exported Function Prototypes ---------------------------------------- */

/* change parameters to pass partition for task_init GNBvd21068*/
ST_ErrorCode_t stpti_EventQueueInit(ST_Partition_t *Partition_p);
ST_ErrorCode_t stpti_EventQueueTerm(void);

ST_ErrorCode_t stpti_EventOpen(FullHandle_t DeviceHandle);
ST_ErrorCode_t stpti_EventTerm(FullHandle_t DeviceHandle);

ST_ErrorCode_t stpti_EventRegister(FullHandle_t DeviceHandle);

ST_ErrorCode_t stpti_DirectQueueEvent(FullHandle_t Handle, STEVT_EventConstant_t Event, STPTI_EventData_t *EventData);
ST_ErrorCode_t stpti_QueueEvent      (FullHandle_t Handle, STEVT_EventConstant_t Event, STPTI_EventData_t *EventData);


#endif  /* __PTI_EVT_H */
