/******************************************************************************
COPYRIGHT (C) STMicroelectronics 2005.  All rights reserved.

   File Name: pti_linux.h
      begin : Tue Apr 5 2005
 Description: STPTI Linux specific interface definitions.

 Kernel mode API. 
 
Notes:

******************************************************************************/

#ifndef __PTI_LINUX_H
 #define __PTI_LINUX_H
 
#include "pti_loc.h"

/* ------------------------------------------------------------------------- */

/* Exported kernel functions.*/
ST_ErrorCode_t STPTI_OSLINUX_SendSessionEvent(STPTI_Handle_t Session, EventData_t *evt_event_p);
BOOL STPTI_OSLINUX_IsEvent( STPTI_Handle_t Session );
EventData_t *STPTI_OSLINUX_GetNextEvent(STPTI_Handle_t Session);
ST_ErrorCode_t STPTI_OSLINUX_SubscribeEvent(STPTI_Handle_t Session, STPTI_Event_t Event_ID);
ST_ErrorCode_t STPTI_OSLINUX_UnsubscribeEvent(STPTI_Handle_t Session, STPTI_Event_t Event_ID);
wait_queue_head_t *STPTI_OSLINUX_GetWaitQueueHead(STPTI_Handle_t Session);
ST_ErrorCode_t STPTI_OSLINUX_WaitForEvent(STPTI_Handle_t Session, STPTI_Event_t *Event_ID, STPTI_EventData_t *EventData );
ST_ErrorCode_t STPTI_OSLINUX_QueueSessionEvent(STPTI_Handle_t Session, STPTI_Event_t Event_ID, STPTI_EventData_t *EventData);

/* Local to pticore */
ST_ErrorCode_t stpti_LinuxEventQueueInit(FullHandle_t DeviceHandle);
ST_ErrorCode_t stpti_LinuxEventQueueTerm(FullHandle_t DeviceHandle);
STPTI_OSLINUX_evt_struct_t *stpti_LinuxAllocateEvtStruct(unsigned int Qsize);
ST_ErrorCode_t stpti_LinuxDeallocateEvtStruct(STPTI_OSLINUX_evt_struct_t *evt_p);
ST_ErrorCode_t stpti_RemoveSessionFromEvtArray(FullHandle_t FullSessionHandle);
ST_ErrorCode_t stpti_LinuxUnsubscribeEvent(FullHandle_t FullSessionHandle, STPTI_Event_t Event_ID);
ST_ErrorCode_t stpti_LinuxSendDeviceEvent(FullHandle_t FullHandle, EventData_t *evt_event_p);

/* User space mapping helpers */
ST_ErrorCode_t stpti_GetUserSpaceMap(STPTI_Buffer_t BufferHandle, U32 *AddrDiff);
ST_ErrorCode_t stpti_SetUserSpaceMap(STPTI_Buffer_t BufferHandle, U32 AddrDiff);
void stpti_BufferCacheInvalidate(STPTI_Buffer_t BufferHandle);

#endif /* __PTI_LINUX_H */

