/******************************************************************************
COPYRIGHT (C) STMicroelectronics 2005.  All rights reserved.

   File Name: pti_list.c
 Description: manage lists of objects (handles etc.)

******************************************************************************/


#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
 #define STTBX_PRINT
#endif


/* Includes ---------------------------------------------------------------- */

#if !defined(ST_OSLINUX)
#include <assert.h>
#include <stdlib.h>
#endif /* #endif !defined(ST_OSLINUX) */

#include "sttbx.h"

#include "pti_hndl.h"
#include "pti_list.h"
#include "pti_loc.h"
#include "pti_mem.h"

#include "memget.h"


/* Functions --------------------------------------------------------------- */


#if !defined ( STPTI_BSL_SUPPORT )
/******************************************************************************
Function Name : stpti_ResizeObjectList
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t stpti_ResizeObjectList(FullHandle_t *ListHandle_p)
{
    ST_ErrorCode_t Error = STPTI_ERROR_INTERNAL_NOT_EXPLICITLY_SET;
    U32 UsedHandles, MaxHandles;
    FullHandle_t ObjectList;
    List_t *List_p;


    ObjectList.word = ListHandle_p->word;

    List_p = stptiMemGet_List(ObjectList);

    UsedHandles = List_p->UsedHandles;
    MaxHandles  = List_p->MaxHandles;
    

    if (UsedHandles == 0)
    {
        /* The ListHandle MUST be Nulled before the list is deallocated as the deallocation
         *will* cause the objects to be shifted and the pointer _may_ no longer be valid */

        ListHandle_p->word = STPTI_NullHandle();
        stpti_MemDealloc(&(stptiMemGet_Session(ObjectList)->MemCtl), ObjectList.member.Object);  /* Blow List away ! */
    }
    else if (UsedHandles < MaxHandles / 2)
    {
        U32 NewMaxHandles;

        NewMaxHandles = MaxHandles / 2;
        Error = stpti_ReallocateObjectList(ObjectList, NewMaxHandles);  /* Resize List */
        if (Error != ST_NO_ERROR)
            return Error;

        List_p = stptiMemGet_List(ObjectList);  /* pointer may have moved! */
        List_p->MaxHandles = NewMaxHandles;
    }

    return( ST_NO_ERROR );
}
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */


/******************************************************************************
Function Name : stpti_CreateListObject
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t stpti_CreateListObject(FullHandle_t SessionHandle, FullHandle_t *NewListHandle)
{
    ST_ErrorCode_t Error = STPTI_ERROR_INTERNAL_NOT_EXPLICITLY_SET;
    FullHandle_t ListHandle;
    List_t *List_p;

    /* Create a List Object */

    Error = stpti_CreateObjectListHandle(SessionHandle, &ListHandle);
    if (Error != ST_NO_ERROR)
        return Error;

    /* Initialize List object */

    List_p = stptiMemGet_List(ListHandle);

    List_p->Handle      = STPTI_NullHandle();
    List_p->MaxHandles  = 1;
    List_p->UsedHandles = 0;

    NewListHandle->word = ListHandle.word;

    return( ST_NO_ERROR );
}


/******************************************************************************
Function Name : stpti_GetInstanceOfObjectInList
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t stpti_GetInstanceOfObjectInList(FullHandle_t ListHandle, STPTI_Handle_t Handle, U32 *Instance)
{
    static U32 ObjectNumber = 0;

    U32 MaxNumberOfObjects, i;

    List_t         *List_p = stptiMemGet_List(ListHandle);
    STPTI_Handle_t *Handle_p = (STPTI_Handle_t *) &List_p->Handle;


    MaxNumberOfObjects = List_p->MaxHandles;

    for (i = 0; i < MaxNumberOfObjects; ++i, ++ObjectNumber)
    {
        if (ObjectNumber >= MaxNumberOfObjects)
            ObjectNumber = 0;

        if (Handle_p[ObjectNumber] == Handle)
        {
            *Instance = ObjectNumber;
            return( ST_NO_ERROR );
        }
    }

    return( STPTI_ERROR_INTERNAL_OBJECT_NOT_FOUND );
}


/******************************************************************************
Function Name : stpti_AddObjectToList
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t stpti_AddObjectToList(FullHandle_t ListHandle, FullHandle_t ObjectHandle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    U32 UsedHandles, MaxHandles;
    U32 ObjectNumber;
    List_t *List_p;
#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
    List_t *CheckList_p;
#endif
    U32 NewMaxHandles, i;


    List_p = stptiMemGet_List(ListHandle);

    UsedHandles = List_p->UsedHandles;
    MaxHandles =  List_p->MaxHandles;

    if (UsedHandles >= MaxHandles)  /* No room in List .... Attempt to expand list */
    {
        NewMaxHandles = MAXIMUM(MaxHandles + MaxHandles / 2, 2);
        Error = stpti_ReallocateObjectList(ListHandle, NewMaxHandles);
        if (Error != ST_NO_ERROR)
            return Error;

        List_p = stptiMemGet_List(ListHandle);  /* may have moved! */
        List_p->MaxHandles = NewMaxHandles;

        for (i = MaxHandles; i < NewMaxHandles; ++i)
            (&List_p->Handle)[i] = STPTI_NullHandle();

    }

    /* Add object handle in next available space & increment object count */

    Error = stpti_GetInstanceOfObjectInList(ListHandle, STPTI_NullHandle(), &ObjectNumber);
    if (Error != ST_NO_ERROR)
        return Error;

#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
    CheckList_p = stptiMemGet_List(ListHandle);
    assert(List_p == CheckList_p);
#endif

    (&List_p->Handle)[ObjectNumber] = ObjectHandle.word;

    List_p->UsedHandles++;

    return ST_NO_ERROR;
}


#if !defined ( STPTI_BSL_SUPPORT )
/******************************************************************************
Function Name : stpti_RemoveObjectFromList
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t stpti_RemoveObjectFromList(FullHandle_t ObjectList, FullHandle_t ObjectHandle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    U32 ObjectNumber;
    List_t *List_p;


    Error = stpti_GetInstanceOfObjectInList(ObjectList, ObjectHandle.word, &ObjectNumber);
    if (Error != ST_NO_ERROR)
        return Error;

    List_p = stptiMemGet_List(ObjectList);

  (&List_p->Handle)[ObjectNumber] = STPTI_NullHandle();
    List_p->UsedHandles--;
    
    return ST_NO_ERROR;
}


/******************************************************************************
Function Name : stpti_DeallocateNextObjectInList
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t stpti_DeallocateNextObjectInList(FullHandle_t ListHandle)
{
    ST_ErrorCode_t Error = STPTI_ERROR_INTERNAL_NOT_EXPLICITLY_SET;
    FullHandle_t ObjectHandle;
    U32 MaxNumberOfObjects;
    U32 ObjectNumber;
    List_t *List_p;


    assert(SEMAPHORE_P_COUNT(stpti_MemoryLock) == 0); /* Must be entered with Memory Locked */
 
    List_p = stptiMemGet_List(ListHandle);

    MaxNumberOfObjects = List_p->MaxHandles;

    for (ObjectNumber = 0; ObjectNumber < MaxNumberOfObjects; ++ObjectNumber)
    {
        ObjectHandle.word = (&List_p->Handle)[ObjectNumber];

        if ( ObjectHandle.word != STPTI_NullHandle() )
            break;
    }

    if (ObjectNumber < MaxNumberOfObjects)
    {
        Error = stpti_DeallocateObject(ObjectHandle, TRUE);
        if (Error != ST_NO_ERROR)
            return Error;
    }
    return Error;
}


/******************************************************************************
Function Name : stpti_GetFirstHandleFromList
  Description : Returns the first handle it finds in a list.
   Parameters :
******************************************************************************/
ST_ErrorCode_t stpti_GetFirstHandleFromList(FullHandle_t ListHandle, FullHandle_t *FoundHandle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    U32 MaxNumberOfObjects;
    U32 ObjectNumber;
    List_t *List_p;

    assert(SEMAPHORE_P_COUNT(stpti_MemoryLock) == 0); /* Must be entered with Memory Locked */

    Error = stpti_ListHandleCheck(ListHandle);

    if (Error != ST_NO_ERROR)
        return( Error );
  
    List_p = stptiMemGet_List(ListHandle);

    MaxNumberOfObjects = List_p->MaxHandles;

    for (ObjectNumber = 0; ObjectNumber < MaxNumberOfObjects; ++ObjectNumber)
    {
        FoundHandle->word = (&List_p->Handle)[ObjectNumber];

        if ( FoundHandle->word != STPTI_NullHandle() )
            break;
    }

    return( Error );
}
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */


/* --- EOF --- */

