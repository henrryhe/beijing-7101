/******************************************************************************
COPYRIGHT (C) STMicroelectronics 2005.  All rights reserved.

   File Name: pti_list.h
 Description: manage lists of objects

******************************************************************************/


#ifndef __PTI_LIST_H
 #define __PTI_LIST_H


/* Includes ------------------------------------------------------------ */


#include "stddefs.h"
#include "pti_loc.h"


/* Exported Types ------------------------------------------------------ */


typedef volatile struct List_s 
{
    U32            MaxHandles;
    U32            UsedHandles;
    STPTI_Handle_t Handle;
} List_t;


/* Exported Function Prototypes ---------------------------------------- */


ST_ErrorCode_t stpti_CreateListObject(FullHandle_t SessionHandle, FullHandle_t * NewListHandle);
ST_ErrorCode_t stpti_AddObjectToList(FullHandle_t ObjectListHandle, FullHandle_t ObjectHandle);

ST_ErrorCode_t stpti_ResizeObjectList(FullHandle_t *ListHandle_p);
ST_ErrorCode_t stpti_RemoveObjectFromList(FullHandle_t ObjectList, FullHandle_t ObjectHandle);

ST_ErrorCode_t stpti_DeallocateNextObjectInList(FullHandle_t ListHandle);
ST_ErrorCode_t stpti_DeallocateSessionList(FullHandle_t ListHandle, BOOL Force);
ST_ErrorCode_t stpti_DeallocateDeviceList(FullHandle_t ListHandle, BOOL Force);

ST_ErrorCode_t stpti_GetFirstHandleFromList(FullHandle_t ListHandle, FullHandle_t *FoundHandle);
ST_ErrorCode_t stpti_GetInstanceOfObjectInList(FullHandle_t List, STPTI_Handle_t Handle, U32 *Instance);

ST_ErrorCode_t stpti_ReallocateObjectList (FullHandle_t ListHandle, U32 NumberEntries);
ST_ErrorCode_t stpti_ReallocateSessionList(FullHandle_t ListHandle, size_t Size);
ST_ErrorCode_t stpti_ReallocateDeviceList (FullHandle_t ListHandle, size_t Size);


#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
 void stpti_DebugPrint_List(FullHandle_t ListHandle);
#endif
 
#endif  /* __PTI_LIST_H */
