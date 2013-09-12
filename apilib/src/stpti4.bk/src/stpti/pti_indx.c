/******************************************************************************
COPYRIGHT (C) STMicroelectronics 2005.  All rights reserved.

   File Name: pti_indx.c
 Description: indexing for STPTI

******************************************************************************/

#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
#define STTBX_PRINT
#endif


#ifdef STPTI_NO_INDEX_SUPPORT
 #error Incorrect build option!
#endif


/* Includes ---------------------------------------------------------------- */

#if !defined(ST_OSLINUX)
#include <assert.h>
#include <string.h>

#include "stcommon.h"
#endif /* #endif !defined(ST_OSLINUX) */
#include "stpti.h"
#if !defined(ST_OSLINUX)
#include "sttbx.h"
#endif /* #endif !defined(ST_OSLINUX) */

#include "pti_hndl.h"
#include "pti_list.h"
#include "pti_loc.h"
#include "pti_mem.h"
#include "validate.h"
#include "pti_hal.h"

#include "pti_indx.h"

#include "memget.h"

/* Private Function Prototypes --------------------------------------------- */

/* Public Variables -------------------------------------------------------- */

void *IndTaskStack;/* pointer to Index task stack GNBvd21068*/


/* Functions --------------------------------------------------------------- */


/******************************************************************************
Function Name : STPTI_IndexAllocate
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_IndexAllocate(STPTI_Handle_t SessionHandle, STPTI_Index_t *IndexHandle_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullSessionHandle;
    FullHandle_t FullIndexHandle;
    Index_t TmpIndex;
    U32 IndexIdent;
    

    STOS_SemaphoreWait(stpti_MemoryLock);

    FullSessionHandle.word = SessionHandle;

    Error = stptiValidate_IndexAllocate( FullSessionHandle, IndexHandle_p );
    if (Error != ST_NO_ERROR)
    {
        STOS_SemaphoreSignal(stpti_MemoryLock);
        return( Error );
    }
    
    Error = stptiHAL_IndexAllocate(FullSessionHandle, &IndexIdent);

    if (Error == ST_NO_ERROR)
    {
        Error = stpti_CreateObjectHandle(FullSessionHandle, OBJECT_TYPE_INDEX, sizeof(Index_t), &FullIndexHandle);

        if (Error == ST_NO_ERROR)
        {
            TmpIndex.OwnerSession        = SessionHandle;           /* Load Parameters */
            TmpIndex.Handle              = FullIndexHandle.word;
            TmpIndex.SlotListHandle.word = STPTI_NullHandle();
            TmpIndex.AssociatedPid       = STPTI_InvalidPid();
            TmpIndex.IndexMask.word      = 0;
            TmpIndex.TC_IndexIdent       = IndexIdent;

            memcpy((U8 *) stptiMemGet_Index(FullIndexHandle), (U8 *) &TmpIndex, sizeof(Index_t));

            *IndexHandle_p = FullIndexHandle.word;

            Error = stptiHAL_IndexInitialise(FullIndexHandle);
        }
    }

    STOS_SemaphoreSignal(stpti_MemoryLock);
    return Error;
}


/******************************************************************************
Function Name : stpti_IndexClearSlots
  Description :
   Parameters :
******************************************************************************/
static ST_ErrorCode_t stpti_IndexClearSlots(FullHandle_t IndexHandle)
{
    ST_ErrorCode_t Error = STPTI_ERROR_INTERNAL_NOT_EXPLICITLY_SET;
    FullHandle_t SlotListHandle;
    FullHandle_t SlotHandle;

    SlotListHandle = (stptiMemGet_Index(IndexHandle))->SlotListHandle;

    while ( SlotListHandle.word != STPTI_NullHandle() )
    {
        U16 Indx;
        U16 MaxIndx = stptiMemGet_List(SlotListHandle)->MaxHandles;

        for (Indx = 0; Indx < MaxIndx; ++Indx)
        {
            SlotHandle.word = (&stptiMemGet_List(SlotListHandle)->Handle)[Indx];
            if (SlotHandle.word != STPTI_NullHandle())
            {
                Error = stpti_SlotDisassociateIndex(SlotHandle, IndexHandle, TRUE);
                break;
            }
        }

        if (Indx == MaxIndx)
            Error = STPTI_ERROR_INTERNAL_ALL_MESSED_UP;

        if (Error != ST_NO_ERROR)
            break;

        SlotListHandle = (stptiMemGet_Index(IndexHandle))->SlotListHandle;
    }

    return Error;
}


/******************************************************************************
Function Name : stpti_DeallocateIndex
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t stpti_DeallocateIndex(FullHandle_t IndexHandle, BOOL Force)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t ListHandle;


    assert(SEMAPHORE_P_COUNT(stpti_MemoryLock) == 0); /* Must be entered with Memory Locked */

    if ((stptiMemGet_Index(IndexHandle))->SlotListHandle.word != STPTI_NullHandle())
    {
        if (Force) Error = stpti_IndexClearSlots(IndexHandle);      /* Clear Slot Associations */
              else Error = STPTI_ERROR_INTERNAL_OBJECT_ASSOCIATED;
    }

    if (Error == ST_NO_ERROR)
    {
        Error = stptiHAL_IndexDeallocate(IndexHandle);      /* Index specific deallocations done here */

        /* Remove from session lists */

        ListHandle = stptiMemGet_Session(IndexHandle)->AllocatedList[IndexHandle.member.ObjectType];
        Error = stpti_RemoveObjectFromList(ListHandle, IndexHandle);
        if (Error == ST_NO_ERROR)
        {
            stpti_ResizeObjectList(&stptiMemGet_Session(IndexHandle)->AllocatedList[IndexHandle.member.ObjectType]);

            /* Deallocate Index control block */
            stpti_MemDealloc(&(stptiMemGet_Session(IndexHandle)->MemCtl), IndexHandle.member.Object);
        }
    }

    assert(SEMAPHORE_P_COUNT(stpti_MemoryLock) == 0); /* and be exited with Memory Locked */

    return Error;
}


/******************************************************************************
Function Name : STPTI_IndexDeallocate
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_IndexDeallocate(STPTI_Index_t IndexHandle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullIndexHandle;


    STOS_SemaphoreWait(stpti_MemoryLock);

    FullIndexHandle.word = IndexHandle;

    Error = stptiValidate_IndexDeallocate( FullIndexHandle );

    if (Error == ST_NO_ERROR)
        Error = stpti_DeallocateIndex(FullIndexHandle, TRUE);
 
    STOS_SemaphoreSignal(stpti_MemoryLock);
    return Error;
}


/******************************************************************************
Function Name: STPTI_IndexStart
 Description :
  Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_IndexStart(ST_DeviceName_t DeviceName)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t DeviceHandle;

    STOS_SemaphoreWait(stpti_MemoryLock);

    Error = stpti_GetDeviceHandleFromDeviceName(DeviceName, &DeviceHandle, TRUE);
    if (Error == ST_NO_ERROR)
    {
        stptiMemGet_Device(DeviceHandle)->IndexEnable = TRUE;
      
        if ( (Error = stptiValidate_IndexStartStop( DeviceHandle )) == ST_NO_ERROR)
        {
            Error = stptiHAL_IndexStart( DeviceHandle );
        }
    }
    STOS_SemaphoreSignal(stpti_MemoryLock);
    return Error;
}


/******************************************************************************
Function Name: STPTI_IndexStop
 Description :
  Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_IndexStop(ST_DeviceName_t DeviceName)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t DeviceHandle;


    STOS_SemaphoreWait(stpti_MemoryLock);

    Error = stpti_GetDeviceHandleFromDeviceName(DeviceName, &DeviceHandle, TRUE);

    if ( Error == ST_NO_ERROR )
    {
        
        Error = stptiValidate_IndexStartStop( DeviceHandle );

        if (Error == ST_NO_ERROR)
        {
            Error = stptiHAL_IndexStop( DeviceHandle );
        }

        if (Error == ST_NO_ERROR)
        {
            stptiMemGet_Device(DeviceHandle)->IndexEnable = FALSE;
        }
    }


    STOS_SemaphoreSignal(stpti_MemoryLock);
    return Error;
}



/******************************************************************************
Function Name: STPTI_IndexSet
 Description :
  Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_IndexSet(STPTI_Index_t IndexHandle, STPTI_IndexDefinition_t IndexMask, U32 MPEGStartCode, U32 MPEGStartCodeMode)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullIndexHandle;

    STOS_SemaphoreWait(stpti_MemoryLock);

    FullIndexHandle.word = IndexHandle;

    Error = stptiValidate_IndexSet( FullIndexHandle, IndexMask, MPEGStartCode, MPEGStartCodeMode);
 
    if (Error == ST_NO_ERROR)
    {
        (stptiMemGet_Index(FullIndexHandle))->IndexMask         = IndexMask;        
        (stptiMemGet_Index(FullIndexHandle))->MPEGStartCode     = (U8) (MPEGStartCode & 0x000000FF);
        (stptiMemGet_Index(FullIndexHandle))->MPEGStartCodeMode = MPEGStartCodeMode;
        Error = stptiHAL_IndexSet( FullIndexHandle);
    }

    STOS_SemaphoreSignal(stpti_MemoryLock);
    return Error;
}


/******************************************************************************
Function Name: STPTI_IndexReset
 Description :
  Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_IndexReset(STPTI_Index_t IndexHandle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullIndexHandle;

    STOS_SemaphoreWait(stpti_MemoryLock);

    FullIndexHandle.word = IndexHandle;


 
    if (Error == ST_NO_ERROR)
    {
        (stptiMemGet_Index(FullIndexHandle))->IndexMask.word = 0;        

        Error = stptiHAL_IndexReset( FullIndexHandle );
    }        

    STOS_SemaphoreSignal(stpti_MemoryLock);
    return Error;
}


/******************************************************************************
Function Name : stpti_IndexAssociateSlot
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t stpti_IndexAssociateSlot(FullHandle_t IndexHandle, FullHandle_t SlotHandle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t SlotListHandle, IndexListHandle;

    
    Error = stptiValidate_IndexAssociateSlot( IndexHandle, SlotHandle );
    if (Error != ST_NO_ERROR)
      return( Error );
    
    Error = stptiHAL_IndexAssociate(IndexHandle, SlotHandle); 
    if (Error == ST_NO_ERROR)
      {
      IndexListHandle = (stptiMemGet_Slot(SlotHandle))->IndexListHandle;               
        /* We KNOW this is the one-and-only association, so don't bother to check that!!! */
        
        Error = stpti_CreateListObject(SlotHandle, &IndexListHandle);
        if (Error == ST_NO_ERROR)
          {
          (stptiMemGet_Slot(SlotHandle))->IndexListHandle = IndexListHandle;                    
            SlotListHandle = (stptiMemGet_Index(IndexHandle))->SlotListHandle;
            
            if (SlotListHandle.word == STPTI_NullHandle())
              {
              Error = stpti_CreateListObject(IndexHandle, &SlotListHandle);
                if (Error == ST_NO_ERROR)
                  {
                    (stptiMemGet_Index(IndexHandle))->SlotListHandle = SlotListHandle;
                    Error = stpti_AssociateObjects(IndexListHandle, SlotHandle, SlotListHandle, IndexHandle);
                    /* if association goes fine, then also set the IndexMask */
                    if(Error == ST_NO_ERROR)
                      {
                        Error = stptiHAL_IndexSet(IndexHandle);
                      }
                  }
            }
        }
    }
    
    return Error;
}


/******************************************************************************
Function Name : stpti_IndexAssociatePid
  Description :
   Parameters :
******************************************************************************/
static ST_ErrorCode_t stpti_IndexAssociatePid(FullHandle_t IndexHandle, STPTI_Pid_t Pid)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t SlotHandle;
    STPTI_Slot_t Slot;

    Error = stptiValidate_IndexAssociatePid( IndexHandle );
    if (Error != ST_NO_ERROR)
        return( Error );

    /* Scan for a Slot using this pid and associate the Index with it */
    /* Don't check return of stpti_SlotQueryPid as is always returns ST_NO_ERROR */
    stpti_SlotQueryPid(IndexHandle, Pid, &Slot);
    if ( Slot != STPTI_NullHandle())
    {
        SlotHandle.word = Slot;
        Error = stpti_IndexAssociateSlot(IndexHandle, SlotHandle);
    }
    /* If Pid is not already set this structure will be read by set pid then 
       the association and set will be performed */
    (stptiMemGet_Index(IndexHandle))->AssociatedPid = Pid;

    return Error;
}


/******************************************************************************
Function Name : STPTI_IndexAssociate
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_IndexAssociate(STPTI_Index_t IndexHandle, STPTI_SlotOrPid_t SlotOrPid)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullIndexHandle;
    FullHandle_t FullSlotHandle;
    STPTI_Pid_t Pid;


    FullIndexHandle.word = IndexHandle;
    STOS_SemaphoreWait(stpti_MemoryLock);

    Error = stptiValidate_IndexAssociate( FullIndexHandle, SlotOrPid );
    if (Error != ST_NO_ERROR)
    {
        STOS_SemaphoreSignal(stpti_MemoryLock);
        return( Error );
    }

    if ( stptiMemGet_Device(FullIndexHandle)->IndexAssociationType == STPTI_INDEX_ASSOCIATION_ASSOCIATE_WITH_SLOTS )
    {
        FullSlotHandle.word = SlotOrPid.Slot;
        Error = stpti_IndexAssociateSlot(FullIndexHandle, FullSlotHandle);
    }
    else
    {
        Pid = SlotOrPid.Pid;
        Error = stpti_IndexAssociatePid(FullIndexHandle, Pid);
    }

    STOS_SemaphoreSignal(stpti_MemoryLock);
    return Error;
}


/******************************************************************************
Function Name : stpti_SlotDisassociateIndex
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t stpti_SlotDisassociateIndex(FullHandle_t SlotHandle, FullHandle_t IndexHandle, BOOL PreChecked)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    FullHandle_t IndexListHandle = (stptiMemGet_Slot(SlotHandle))->IndexListHandle;
    FullHandle_t SlotListHandle = (stptiMemGet_Index(IndexHandle))->SlotListHandle;

    Error = stptiValidate_SlotDisassociateIndex( SlotHandle, IndexHandle, PreChecked );
    if (Error != ST_NO_ERROR)
        return( Error );

    Error = stptiHAL_IndexDisassociate(IndexHandle, SlotHandle);    /* Object specific disassociations here */

    Error = stpti_RemoveObjectFromList(IndexListHandle, IndexHandle);   /* Disassociate and resize */
    if (Error == ST_NO_ERROR)
    {
        Error = stpti_RemoveObjectFromList(SlotListHandle, SlotHandle);
        if (Error == ST_NO_ERROR)
        {
            stpti_ResizeObjectList( &(stptiMemGet_Index(IndexHandle))->SlotListHandle );
            stpti_ResizeObjectList(  &(stptiMemGet_Slot(SlotHandle))->IndexListHandle );
        }
    }

    return Error;
}


/******************************************************************************
Function Name : stpti_PidDisassociateIndex
  Description :
   Parameters :
******************************************************************************/
static ST_ErrorCode_t stpti_PidDisassociateIndex(FullHandle_t IndexHandle)
{
    ST_ErrorCode_t Error = STPTI_ERROR_INTERNAL_NOT_EXPLICITLY_SET;
    FullHandle_t SlotHandle;
    STPTI_Slot_t Slot;
    STPTI_Pid_t Pid = (stptiMemGet_Index(IndexHandle))->AssociatedPid;


    /* Scan for a Slot using this pid and disassociate the Index from it */
    Error = stpti_SlotQueryPid(IndexHandle, Pid, &Slot);

    if (Slot != STPTI_NullHandle())
    {
        SlotHandle.word = Slot;
        Error = stpti_SlotDisassociateIndex(SlotHandle, IndexHandle, TRUE);
    }

    if (Error == ST_NO_ERROR)
    {
        (stptiMemGet_Index(IndexHandle))->AssociatedPid = STPTI_InvalidPid();
    }

    return Error;
}


/******************************************************************************
Function Name : STPTI_IndexDisassociate
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_IndexDisassociate(STPTI_Index_t IndexHandle, STPTI_SlotOrPid_t SlotOrPid)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullIndexHandle;
    FullHandle_t FullSlotHandle;


    STOS_SemaphoreWait(stpti_MemoryLock);

    FullIndexHandle.word = IndexHandle;

    Error = stptiValidate_IndexDisassociate( FullIndexHandle );

    if (Error == ST_NO_ERROR)
    {

        if (stptiMemGet_Device(FullIndexHandle)->IndexAssociationType == STPTI_INDEX_ASSOCIATION_ASSOCIATE_WITH_SLOTS)
        {
            FullSlotHandle.word = SlotOrPid.Slot;
            Error = stpti_SlotDisassociateIndex(FullSlotHandle, FullIndexHandle, FALSE);
        }
        else
        {
            Error = stpti_PidDisassociateIndex(FullIndexHandle);
        }
    }

    STOS_SemaphoreSignal(stpti_MemoryLock);
    return Error;
}






/******************************************************************************
Function Name : stpti_IndexQueueInit
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t stpti_IndexQueueInit(ST_Partition_t *Partition_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    STPTI_Driver.IndexQueue_p = STOS_MessageQueueCreate( sizeof(EventData_t), INDEX_QUEUE_SIZE);

    if (STPTI_Driver.IndexQueue_p == NULL)
    {
        Error = ST_ERROR_NO_MEMORY;
    }

    return Error;
}


/******************************************************************************
Function Name : stpti_IndexQueueTerm
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t stpti_IndexQueueTerm(void)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

#if !defined(ST_OSLINUX)
    EventData_t *event_p;
    FullHandle_t FullNullHandle;
    CLOCK time;

    FullNullHandle.word = STPTI_NullHandle();

    /* Send 'terminate' message to task */

    do
    {
        time = time_plus(time_now(), 10);
        event_p = message_claim_timeout(STPTI_Driver.IndexQueue_p, &time);
    }
    while (event_p == NULL);

    event_p->Source = FullNullHandle;
    message_send(STPTI_Driver.IndexQueue_p, (void *) event_p);

    /* Wait for task to exit */

    STOS_SemaphoreSignal(stpti_MemoryLock); /* Release memory lock while waiting for task to shut down */

    time = time_plus(time_now(), TICKS_PER_SECOND * 10);
    if (STOS_TaskWait((task_t **) & STPTI_Driver.IndexTaskId_p, &time) != ST_NO_ERROR)
    {
        Error = ST_ERROR_TIMEOUT;
    }
    else
    {
        /* delete task  */
        if (STOS_TaskDelete(STPTI_Driver.IndexTaskId_p, STPTI_Driver.MemCtl.Partition_p, IndTaskStack, STPTI_Driver.MemCtl.Partition_p ))
        {
            Error = ST_ERROR_TIMEOUT;
        }
        else
        {
            /* delete the queue */
            message_delete_queue(STPTI_Driver.IndexQueue_p);
            STPTI_Driver.IndexQueue_p = NULL;    /* Make sure it can't be used */
        }
    }

    STOS_SemaphoreWait(stpti_MemoryLock);            /* Reset Lock */
    
#endif /* #endif !defined(ST_OSLINUX)  */
      
    return Error;
}

/******************************************************************************
Function Name : stpti_ProcessEvent()
  Description : Function that processes event data. Called from ISRTASK.
   Parameters : Event data structure.
******************************************************************************/
static void stpti_ProcessEvent(EventData_t *event_p)
{
    FullHandle_t DeviceHandle;
    FullHandle_t SlotHandle;
    STPTI_IndexDefinition_t IndexDefinition;
    BOOL IndexEnable;
    message_queue_t *IndexQueue_p = STPTI_Driver.IndexQueue_p;

    STEVT_Handle_t EventHandle;
    STEVT_EventID_t EventID;

    DeviceHandle.word = event_p->Source.word;
    SlotHandle.word = event_p->EventData.SlotHandle;
    IndexDefinition = event_p->EventData.u.IndexEventData.IndexBitMap;

    if (DeviceHandle.word == STPTI_NullHandle())    /*Wrong device??*/
    {
        message_release(IndexQueue_p, (void *) event_p);
        return;
    }

    STOS_SemaphoreWait(stpti_MemoryLock);
    IndexEnable = stptiMemGet_Device(DeviceHandle)->IndexEnable;
    STOS_SemaphoreSignal(stpti_MemoryLock);

    if (IndexEnable)

    {

        /* Print Event Data */
#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
        STTBX_Print(("\n"));
        STTBX_Print(("Index Event\n-----------\n"));
        STTBX_Print(("                                event_p->Source:0x%08x\n", event_p->Source.word));
        STTBX_Print(("                                 event_p->Event:0x%x\n", event_p->Event));
        STTBX_Print(("                  event_p->EventData.SlotHandle:0x%08x\n", SlotHandle.word));
        STTBX_Print(
                    ("event_p->EventData.u.IndexEventData.PacketCount:%d\n",
                     event_p->EventData.u.IndexEventData.PacketCount));
        STTBX_Print(("event_p->EventData.u.IndexEventData.IndexBitMap:0x%08x\n", IndexDefinition.word));

        if (IndexDefinition.s.PayloadUnitStartIndicator == 1)
            STTBX_Print(("PayloadUnitStartIndicator\n"));

        if (IndexDefinition.s.ScramblingChangeToClear == 1)
            STTBX_Print(("ScramblingChangeToClear\n"));

        if (IndexDefinition.s.ScramblingChangeToEven == 1)
            STTBX_Print(("ScramblingChangeToEven\n"));

        if (IndexDefinition.s.ScramblingChangeToOdd == 1)
            STTBX_Print(("ScramblingChangeToOdd\n"));

        if (IndexDefinition.s.DiscontinuityIndicator == 1)
            STTBX_Print(("DiscontinuityIndicator\n"));

        if (IndexDefinition.s.RandomAccessIndicator == 1)
            STTBX_Print(("RandomAccessIndicator\n"));

        if (IndexDefinition.s.PriorityIndicator == 1)
            STTBX_Print(("PriorityIndicator\n"));

        if (IndexDefinition.s.PCRFlag == 1)
            STTBX_Print(("PCRFlag\n"));

        if (IndexDefinition.s.OPCRFlag == 1)
            STTBX_Print(("OPCRFlag\n"));

        if (IndexDefinition.s.SplicingPointFlag == 1)
            STTBX_Print(("SplicingPointFlag\n"));

        if (IndexDefinition.s.TransportPrivateDataFlag == 1)
            STTBX_Print(("TransportPrivateDataFlag\n"));

        if (IndexDefinition.s.AdaptationFieldExtensionFlag == 1)
            STTBX_Print(("AdaptationFieldExtensionFlag\n"));

        if (IndexDefinition.s.FirstRecordedPacket == 1)
            STTBX_Print(("FirstRecordedPacket\n"));

        if (IndexDefinition.s.MPEGStartCode == 1)
            STTBX_Print(("MPEGStartCode\n"));

        STTBX_Print(("\n"));
#endif

        STOS_SemaphoreWait(stpti_MemoryLock);

        {
            /* Remove those index events that are not required to be reported */

            FullHandle_t IndexListHandle;

            IndexListHandle = (stptiMemGet_Slot(SlotHandle))->IndexListHandle;                
            if (IndexListHandle.word != STPTI_NullHandle())
            {
                U32 Index;
                U32 MaxHandles = stptiMemGet_List(IndexListHandle)->MaxHandles;

                
                for (Index = 0; Index < MaxHandles; ++Index)
                {
                    FullHandle_t IndexHandle;

                    IndexHandle.word = (&stptiMemGet_List(IndexListHandle)->Handle)[Index];
                    if (IndexHandle.word != STPTI_NullHandle())
                    {     
                        IndexDefinition.word &= (stptiMemGet_Index(IndexHandle))->IndexMask.word;                          
                        break;               /* there can only be one Index per slot */
                    }
                }
            }
        }
        
        EventHandle = stptiMemGet_Device(DeviceHandle)->EventHandle;
        EventID = stptiMemGet_Device(DeviceHandle)->EventID[event_p->Event - STPTI_EVENT_BASE];

        STOS_SemaphoreSignal(stpti_MemoryLock);
        if (IndexDefinition.word != 0)
        {   
            event_p->EventData.u.IndexEventData.IndexBitMap.word = IndexDefinition.word;            

#if defined(ST_OSLINUX)
            STEVT_NotifyWithSize(EventHandle, EventID, (void *) &event_p->EventData, sizeof(event_p->EventData));
#else
            STEVT_Notify(EventHandle, EventID, (void *) &event_p->EventData);
#endif /* #endif defined(ST_OSLINUX) */
        }
    }
}


/******************************************************************************
Function Name : stpti_DirectQueueIndex
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t stpti_DirectQueueIndex(FullHandle_t Handle, STEVT_EventConstant_t Event, BOOL Dumped, BOOL Signalled,
                                      STPTI_EventData_t * EventData)
{
    EventData_t local_event;

    local_event.Source = Handle;
    local_event.Event = Event;
    local_event.PacketSignalled = Signalled;
    local_event.PacketDumped = Dumped;
    memcpy( (void *)&local_event.EventData, EventData, sizeof(STPTI_EventData_t) );

    stpti_ProcessEvent(&local_event);

    return( ST_NO_ERROR );
}


/* ------------------------------------------------------------------------- */


/* --- EOF --- */
