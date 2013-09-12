/******************************************************************************
COPYRIGHT (C) STMicroelectronics 2005.  All rights reserved.

   File Name: pti_car.c
 Description: stpti carousel

******************************************************************************/


#ifndef STPTI_CAROUSEL_SUPPORT
 #error Incorrect build option!
#endif

/* Includes ---------------------------------------------------------------- */
#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
 #define STTBX_PRINT
#endif

#if !defined(ST_OSLINUX)
#include <assert.h>
#include <string.h>

#include "stcommon.h"
#include "sttbx.h"
#endif /* #endif !defined(ST_OSLINUX) */
#include "stpti.h"
#include "osx.h"
#include "pti_hndl.h"
#include "pti_evt.h"
#include "pti_list.h"
#include "pti_loc.h"
#include "pti_mem.h"
#include "pti_hal.h"
#include "validate.h"
#include "pti4.h"
#include "pti_car.h"

#include "memget.h"



/* Private Types ----------------------------------------------------------- */


typedef struct
{
    STPTI_Carousel_t CarouselHandle;
    semaphore_t *ReadCarouselParams_p;
}
CarouselLaunchParams_t;

/* Private Variables ------------------------------------------------------- */
#ifdef STPTI_DEBUG_SUPPORT
extern STPTI_DebugStatistics_t      DebugStatistics;
#endif

static   U32 PacketsSent = 0;
volatile U32 stpti_TimedCarouselTaskCount = 0;

static semaphore_t *CarouselTCLockSem_p=NULL;
/* Private Macros ---------------------------------------------------------- */

/* Private Function Prototypes --------------------------------------------- */

static void           stpti_SimpleCarouselTask  (CarouselLaunchParams_t *CarouselLaunchParams_p);
static void           stpti_TimedCarouselTask   (CarouselLaunchParams_t *CarouselLaunchParams_p);
static ST_ErrorCode_t stpti_CarouselClearEntries(FullHandle_t CarouselHandle, BOOL Force);
static ST_ErrorCode_t stpti_EntryClearCarousels (FullHandle_t EntryHandle,    BOOL Force);
static ST_ErrorCode_t stpti_CarouselClearBuffers(FullHandle_t CarouselHandle);


/* Functions --------------------------------------------------------------- */


/******************************************************************************
Function Name : STPTI_CarouselAllocate
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_CarouselAllocate(STPTI_Handle_t Handle, STPTI_Carousel_t * Carousel)
{
    ST_ErrorCode_t Error = STPTI_ERROR_INTERNAL_NOT_EXPLICITLY_SET;
    FullHandle_t FullSessionHandle;
    FullHandle_t FullCarouselHandle;
    Carousel_t TmpCarousel;
    semaphore_t *ReadCarouselParams_p;
    static tdesc_t TaskDesc; /* task desc for Carousel task GNBvd21068*/

    STOS_SemaphoreWait(stpti_MemoryLock);
    FullSessionHandle.word = Handle;

    ReadCarouselParams_p=STOS_SemaphoreCreateFifo( NULL, 0);

    /* Set up Carousel's TC writing lock semaphore */
    if ( CarouselTCLockSem_p == NULL)
    {
        CarouselTCLockSem_p = STOS_SemaphoreCreateFifo( NULL, 1 );  
    }

    Error = stptiValidate_CarouselAllocate( FullSessionHandle, Carousel );
    if (Error == ST_NO_ERROR)
    {
        /* Only Allowed one Carousel Per Device */
        if ( stptiMemGet_Device(FullSessionHandle)->CarouselHandle != STPTI_NullHandle() )
        {
            /* Carousel already allocated to this device */
            Error = STPTI_ERROR_CAROUSEL_ALREADY_ALLOCATED;
        }
        else
        {
            Error = stpti_CreateObjectHandle( FullSessionHandle, OBJECT_TYPE_CAROUSEL, sizeof(Carousel_t), &FullCarouselHandle );
            if (Error == ST_NO_ERROR)   /* Load Parameters */
            {
                TmpCarousel.OwnerSession                 = Handle;
                TmpCarousel.Handle                       = FullCarouselHandle.word;
                TmpCarousel.LockingSession.word          = STPTI_NullHandle(); /* Carousel Initially unlocked */
                TmpCarousel.CarouselEntryListHandle.word = STPTI_NullHandle(); /* No Entries */
                TmpCarousel.BufferListHandle.word        = STPTI_NullHandle(); /* No Entries */

                TmpCarousel.StartEntry.word              = STPTI_NullHandle();
                TmpCarousel.EndEntry.word                = STPTI_NullHandle();
                TmpCarousel.NextEntry.word               = STPTI_NullHandle();
                TmpCarousel.CurrentEntry.word            = STPTI_NullHandle();
                TmpCarousel.UnlinkOnNextSem              = FALSE;
                
                TmpCarousel.PacketTimeTicks              = 0;
                TmpCarousel.CycleTimeTicks               = 0;

                memcpy((U8 *) stptiMemGet_Carousel(FullCarouselHandle), (U8 *) &TmpCarousel, sizeof(Carousel_t));

                *Carousel = FullCarouselHandle.word; /* Carousel Handle to be returned */

                /* Record Carousel Handle in Device structure */
                stptiMemGet_Device(FullSessionHandle)->CarouselHandle = FullCarouselHandle.word; 
                {
                    semaphore_t *Sem_p = STOS_SemaphoreCreateFifo( NULL, SEM_SYNCHRONIZATION );  /* Set up Carousel's signalling semaphore */
                    
                    stptiMemGet_Device(FullSessionHandle)->CarouselSem_p = Sem_p;
                    stptiMemGet_Carousel(FullCarouselHandle)->Sem_p = Sem_p;                    
                }

                stptiMemGet_Carousel(FullCarouselHandle)->AbortFlag = FALSE;

                {
                    S32 CarouselTaskPriority;
                    task_t *CarouselTaskID;
                    CarouselLaunchParams_t CarouselLaunchParams;

                    CarouselLaunchParams.CarouselHandle = FullCarouselHandle.word;
                    CarouselLaunchParams.ReadCarouselParams_p = ReadCarouselParams_p;

                    /* Start The Carousel Process */
                    CarouselTaskPriority = stptiMemGet_Device(FullSessionHandle)->CarouselProcessPriority;

                    Error = STOS_TaskCreate (  ((stptiMemGet_Device(FullSessionHandle)->EntryType == STPTI_CAROUSEL_ENTRY_TYPE_SIMPLE)
                                                   ? ((void (*)(void *))stpti_SimpleCarouselTask)
                                                   : ((void (*)(void *))stpti_TimedCarouselTask) ),
                                                (void *) &CarouselLaunchParams,
                                                stptiMemGet_Device(FullSessionHandle)->DriverPartition_p,
                                                CAROUSEL_TASK_STACK,
                                                (void **)&(stptiMemGet_Carousel(FullCarouselHandle))->CarTaskStack,
                                                stptiMemGet_Device(FullSessionHandle)->DriverPartition_p,
                                                (task_t **)&CarouselTaskID, 
                                                &TaskDesc,
                                                CarouselTaskPriority, 
                                                "CarouselTask",
                                                0 );
                    if( Error == ST_NO_ERROR )
                    {
                        (stptiMemGet_Carousel(FullCarouselHandle))->CarouselTaskID_p = CarouselTaskID;
                        /* Wait here for the carousel process to start and read all it's parameters */
                        STOS_SemaphoreWait(ReadCarouselParams_p); 
                    }

                }
            }
        }
    }

    STOS_SemaphoreDelete(NULL, ReadCarouselParams_p);  /* Now we can remove the semaphore */

    STOS_SemaphoreSignal(stpti_MemoryLock);
    return( Error );
}


/******************************************************************************
Function Name : STPTI_CarouselLinkToBuffer
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_CarouselLinkToBuffer(STPTI_Carousel_t CarouselHandle, STPTI_Buffer_t BufferHandle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullCarouselHandle;
    FullHandle_t FullBufferHandle;
    FullHandle_t BufferListHandle;
    FullHandle_t CarouselListHandle;
    FullHandle_t SlotListHandle;
    BOOL DMA_Exists;


    STOS_SemaphoreWait(stpti_MemoryLock);
    FullCarouselHandle.word = CarouselHandle;
    FullBufferHandle.word   = BufferHandle;

    Error = stptiValidate_CarouselLinkToBuffer( FullCarouselHandle, FullBufferHandle);
    if (Error == ST_NO_ERROR)
    {
        /* If buffer already has a slot linked to it, then re-use the DMA previously allocated */
        SlotListHandle = (stptiMemGet_Buffer(FullBufferHandle))->SlotListHandle;

        if (SlotListHandle.word != STPTI_NullHandle())
        {
            DMA_Exists = TRUE;
        }
        else
        {
            /* Enable buffer Interrupt and set stream type */
            (stptiMemGet_Buffer(FullBufferHandle))->Type = STPTI_SLOT_TYPE_RAW;
            DMA_Exists = FALSE;
        }

        if (Error == ST_NO_ERROR)
        {
            /* set-up a new DMA structure or reuse existing one */
            Error = stptiHAL_CarouselLinkToBuffer(FullCarouselHandle, FullBufferHandle, DMA_Exists);

            if (Error == ST_NO_ERROR)
            {
                BufferListHandle = (stptiMemGet_Carousel(FullCarouselHandle))->BufferListHandle;

                if (BufferListHandle.word == STPTI_NullHandle())
                {
                    Error = stpti_CreateListObject(FullCarouselHandle, &BufferListHandle);
                    if (Error == ST_NO_ERROR)
                    {
                        (stptiMemGet_Carousel(FullCarouselHandle))->BufferListHandle = BufferListHandle;
                    }
                }

                if (Error == ST_NO_ERROR)
                {
                    CarouselListHandle = (stptiMemGet_Buffer(FullBufferHandle))->CarouselListHandle;

                    if (CarouselListHandle.word == STPTI_NullHandle())
                    {
                        Error = stpti_CreateListObject(FullBufferHandle, &CarouselListHandle);
                        if (Error == ST_NO_ERROR)
                        {
                            (stptiMemGet_Buffer(FullBufferHandle))->CarouselListHandle = CarouselListHandle;
                        }
                    }
                }

                if (Error == ST_NO_ERROR)
                {
                    Error = stpti_AssociateObjects(BufferListHandle, FullCarouselHandle, CarouselListHandle, FullBufferHandle);
                }

            }
        }
    }

    STOS_SemaphoreSignal(stpti_MemoryLock);
    return( Error );

}

/******************************************************************************
Function Name : stpti_DeallocateCarousel
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t stpti_DeallocateCarousel(FullHandle_t CarouselHandle, BOOL Force)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t ListHandle;
    FullHandle_t CarouselEntryListHandle;
    task_t *CarouselTaskID;
    CLOCK time;
    semaphore_t *Sem_p;
    Carousel_t *Carousel_p;

    assert(stpti_MemoryLock->semaphore_count == 0); /* Must be entered with Memory Locked */

    /* Since the memory is locked, we can rely on the carousel pointer remaining unchanged */
    Carousel_p = stptiMemGet_Carousel(CarouselHandle);

    CarouselEntryListHandle.word = Carousel_p->CarouselEntryListHandle.word;

    if (CarouselEntryListHandle.word != STPTI_NullHandle())
    {
        Error = stpti_CarouselClearEntries(CarouselHandle, Force); /* Clear Carousel Entry Associations */
    }

    if (Error == ST_NO_ERROR)
    {
        Carousel_p->AbortFlag = TRUE;

        /* Kick the carousel to ensure it 'sees' the abort flag */
        Sem_p = Carousel_p->Sem_p;

        if (Sem_p != NULL)
        {
            STTBX_Print(("Signaling CarouselTask (from stpti_DeallocateCarousel)\n"));
            STOS_SemaphoreSignal(Sem_p);
        }

        /* Wait for Carousel Process to complete */
        CarouselTaskID = Carousel_p->CarouselTaskID_p;
        time = time_plus( time_now(), TICKS_PER_SECOND );

        STOS_SemaphoreSignal(stpti_MemoryLock); /* Wait for task to exit */

        if (STOS_TaskWait(&CarouselTaskID, &time) != ST_NO_ERROR )
        {
            Error = ST_ERROR_TIMEOUT;
        }
        else
        {
            Error = STOS_TaskDelete ( CarouselTaskID,
                                      stptiMemGet_Device(CarouselHandle)->DriverPartition_p,
                                      (stptiMemGet_Carousel(CarouselHandle))->CarTaskStack,
                                      stptiMemGet_Device(CarouselHandle)->DriverPartition_p );
        }

        STOS_SemaphoreWait(stpti_MemoryLock);

        /* Re-fetch the carousel pointer, since the location may have changed while the memory was unlocked */
        Carousel_p = stptiMemGet_Carousel(CarouselHandle);

        Sem_p = Carousel_p->Sem_p;
        Carousel_p->Sem_p = NULL;
        stptiMemGet_Device(CarouselHandle)->CarouselSem_p = NULL;
        
        if (Sem_p != NULL)
        {
            STOS_SemaphoreDelete(NULL, Sem_p);    /* destroy signalling semaphore */
        }

        /* Disassociate the buffers */

        if (Error == ST_NO_ERROR)
        {
            /* Disassociate the buffers */
      
            /* !!!! Comment : David Orchard                        !!! */
            /* !!!! This bit is new and fixes the original problem !!! */
            /* !!!! But has introduced a new one                   !!! */
            /* !!!! We may need to do some work when deallocating  !!! */
            /* !!!! buffers to disassociate the carousel           !!! */
      
            Error = stpti_CarouselClearBuffers(CarouselHandle);

            if (Error == ST_NO_ERROR)
            {
                /* Remove from session lists */
                ListHandle = stptiMemGet_Session(CarouselHandle)->AllocatedList[CarouselHandle.member.ObjectType];

                Error = stpti_RemoveObjectFromList(ListHandle, CarouselHandle);
                if (Error == ST_NO_ERROR)
                {
                    stpti_ResizeObjectList(&stptiMemGet_Session(CarouselHandle)->AllocatedList[CarouselHandle.member.ObjectType]);

                    stptiMemGet_Device(CarouselHandle)->CarouselHandle = STPTI_NullHandle();    /* Null Entry in device parameters */

                    stpti_MemDealloc(&(stptiMemGet_Session(CarouselHandle)->MemCtl), CarouselHandle.member.Object);  /* Deallocate Carousel control block */
                }
            }
        }
    }

    assert(stpti_MemoryLock->semaphore_count == 0); /* and be exited with Memory Locked */

    return( Error );
}


/******************************************************************************
Function Name : stpti_CarouselClearBuffers
  Description :
   Parameters :
******************************************************************************/
static ST_ErrorCode_t stpti_CarouselClearBuffers(FullHandle_t CarouselHandle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t BufferListHandle;
    FullHandle_t BufferHandle;
    U16 Indx;
    U16 MaxIndx;


    BufferListHandle = (stptiMemGet_Carousel(CarouselHandle))->BufferListHandle;
    while (BufferListHandle.word != STPTI_NullHandle())
    {
        MaxIndx = stptiMemGet_List(BufferListHandle)->MaxHandles;

        for (Indx = 0; Indx < MaxIndx; ++Indx)
        {

            BufferHandle.word = (&stptiMemGet_List(BufferListHandle)->Handle)[Indx];
            if (BufferHandle.word != STPTI_NullHandle())
            {
                Error = stpti_BufferDisassociateCarousel(BufferHandle, CarouselHandle, TRUE);
                break;
            }
        }

        if (Indx == MaxIndx)
        {
            Error = STPTI_ERROR_INTERNAL_ALL_MESSED_UP;
        }

        if (Error != ST_NO_ERROR)
        {
            break;
        }

        BufferListHandle = (stptiMemGet_Carousel(CarouselHandle))->BufferListHandle;
    }

    return( Error );
}


/******************************************************************************
Function Name : stpti_DeallocateCarouselEntry
  Description :
   Parameters :

            Clear Carousel Associations, if Force is TRUE then they are removed
           even if they are currently associated, if FALSE then only unlinked
           entries are removed. If this results in any entrys left associated
           then the function fails and returns STPTI_ERROR_CAROUSEL_ENTRY_INSERTED

******************************************************************************/
ST_ErrorCode_t stpti_DeallocateCarouselEntry(FullHandle_t EntryHandle, BOOL Force)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t CarouselListHandle;
    FullHandle_t ListHandle;


    CarouselListHandle = (stptiMemGet_Device(EntryHandle)->EntryType == STPTI_CAROUSEL_ENTRY_TYPE_SIMPLE) 
            ? stptiMemGet_CarouselSimpleEntry(EntryHandle)->CarouselListHandle 
            : stptiMemGet_CarouselTimedEntry(EntryHandle)->CarouselListHandle;
            
    if (CarouselListHandle.word != STPTI_NullHandle())
    {
        Error = stpti_EntryClearCarousels(EntryHandle, Force);
    }

    /* Remove from session lists */

    if (Error == ST_NO_ERROR)
    {
        ListHandle = stptiMemGet_Session(EntryHandle)->AllocatedList[EntryHandle.member.ObjectType];

        Error = stpti_RemoveObjectFromList(ListHandle, EntryHandle);
        if (Error == ST_NO_ERROR)
        {
            stpti_ResizeObjectList(&stptiMemGet_Session(EntryHandle)->AllocatedList[EntryHandle.member.ObjectType]);

            stpti_MemDealloc(&(stptiMemGet_Session(EntryHandle)->MemCtl), EntryHandle.member.Object);    /* Deallocate CarouselEntry memory */
        }
    }

    return( Error );
}


/******************************************************************************
Function Name :
  Description :
   Parameters :
******************************************************************************/
static ST_ErrorCode_t stpti_CarouselClearEntries(FullHandle_t CarouselHandle, BOOL Force)
{
    ST_ErrorCode_t Error = STPTI_ERROR_INTERNAL_NOT_EXPLICITLY_SET;
    FullHandle_t EntryListHandle;
    FullHandle_t EntryHandle;
    U16 Indx;
    U16 MaxIndx;


    EntryListHandle = (stptiMemGet_Carousel(CarouselHandle))->CarouselEntryListHandle;

    while (EntryListHandle.word != STPTI_NullHandle())
    {
        MaxIndx = stptiMemGet_List(EntryListHandle)->MaxHandles;

        for (Indx = 0; Indx < MaxIndx; ++Indx)
        {
            EntryHandle.word = (&stptiMemGet_List(EntryListHandle)->Handle)[Indx];

            if (EntryHandle.word != STPTI_NullHandle())
            {
                if (stptiMemGet_Device(CarouselHandle)->EntryType == STPTI_CAROUSEL_ENTRY_TYPE_SIMPLE)
                {
                    if (Force || !( stptiMemGet_CarouselSimpleEntry(EntryHandle)->Linked ))
                    {
                        Error = stpti_CarouselDisassociateSimpleEntry(CarouselHandle, EntryHandle, TRUE);
                    }
                    else
                    {
                        Error = STPTI_ERROR_CAROUSEL_ENTRY_INSERTED;
                    }
                }
                else
                {
                    if (Force || !(stptiMemGet_CarouselTimedEntry(EntryHandle))->Linked)
                    {
                        Error = stpti_CarouselDisassociateTimedEntry(CarouselHandle, EntryHandle, TRUE);
                    }
                    else
                    {
                        Error = STPTI_ERROR_CAROUSEL_ENTRY_INSERTED;
                    }
                }
                break;
            }

        }   /* for(Indx) */

        if (Indx == MaxIndx)
        {
            Error = STPTI_ERROR_INTERNAL_ALL_MESSED_UP;
        }

        if (Error != ST_NO_ERROR)
        {
            break;
        }

        EntryListHandle = (stptiMemGet_Carousel(CarouselHandle))->CarouselEntryListHandle;
    }

    return( Error );
}


/******************************************************************************
Function Name : stpti_EntryClearCarousels
  Description :
   Parameters :
******************************************************************************/
static ST_ErrorCode_t stpti_EntryClearCarousels(FullHandle_t EntryHandle, BOOL Force)
{
    ST_ErrorCode_t Error = STPTI_ERROR_INTERNAL_NOT_EXPLICITLY_SET;
    FullHandle_t CarouselListHandle;
    FullHandle_t CarouselHandle;
    U16 Indx;
    U16 MaxIndx;


    CarouselListHandle = (stptiMemGet_Device(EntryHandle)->EntryType == STPTI_CAROUSEL_ENTRY_TYPE_SIMPLE) 
            ? stptiMemGet_CarouselSimpleEntry(EntryHandle)->CarouselListHandle 
            : stptiMemGet_CarouselTimedEntry(EntryHandle)->CarouselListHandle;
            

    while (CarouselListHandle.word != STPTI_NullHandle())
    {
        MaxIndx = stptiMemGet_List(CarouselListHandle)->MaxHandles;

        for (Indx = 0; Indx < MaxIndx; ++Indx)
        {
            CarouselHandle.word = (&stptiMemGet_List(CarouselListHandle)->Handle)[Indx];

            if (CarouselHandle.word != STPTI_NullHandle())
            {
                if (stptiMemGet_Device(EntryHandle)->EntryType == STPTI_CAROUSEL_ENTRY_TYPE_SIMPLE)
                {
                    if (Force || !(stptiMemGet_CarouselSimpleEntry(EntryHandle)->Linked))
                    {
                        Error = stpti_CarouselDisassociateSimpleEntry(CarouselHandle, EntryHandle, TRUE);
                    }
                    else
                    {
                        Error = STPTI_ERROR_CAROUSEL_ENTRY_INSERTED;
                    }
                }
                else
                {
                    if (Force || !(stptiMemGet_CarouselTimedEntry(EntryHandle))->Linked)
                    {
                        Error = stpti_CarouselDisassociateTimedEntry(CarouselHandle, EntryHandle, TRUE);
                    }
                    else
                    {
                        Error = STPTI_ERROR_CAROUSEL_ENTRY_INSERTED;
                    }
                }
                break;
            }
        }

        if (Indx == MaxIndx)
        {
            Error = STPTI_ERROR_INTERNAL_ALL_MESSED_UP;
        }

        if (Error != ST_NO_ERROR)
        {
            break;
        }

        CarouselListHandle = (stptiMemGet_Device(EntryHandle)->EntryType == STPTI_CAROUSEL_ENTRY_TYPE_SIMPLE) 
                ? stptiMemGet_CarouselSimpleEntry(EntryHandle)->CarouselListHandle 
                : stptiMemGet_CarouselTimedEntry(EntryHandle)->CarouselListHandle;
    }

    return( Error );
}


/******************************************************************************
Function Name : STPTI_CarouselEntryAllocate
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_CarouselEntryAllocate(STPTI_Handle_t Handle, STPTI_CarouselEntry_t *CarouselEntryHandle)
{
    ST_ErrorCode_t Error = STPTI_ERROR_INTERNAL_NOT_EXPLICITLY_SET;
    FullHandle_t FullSessionHandle;
    FullHandle_t FullCarouselEntryHandle;


    STOS_SemaphoreWait(stpti_MemoryLock);
    FullSessionHandle.word = Handle;

    Error = stptiValidate_CarouselEntryAllocate( FullSessionHandle, CarouselEntryHandle);
    if (Error == ST_NO_ERROR)
    {
        if (stptiMemGet_Device(FullSessionHandle)->EntryType == STPTI_CAROUSEL_ENTRY_TYPE_SIMPLE)
        {
            Error = stpti_CreateObjectHandle(FullSessionHandle, OBJECT_TYPE_CAROUSEL_ENTRY, sizeof(CarouselSimpleEntry_t), &FullCarouselEntryHandle);
            if (Error == ST_NO_ERROR)
            {
                CarouselSimpleEntry_t TmpCarouselEntry;     /* Load Parameters */

                TmpCarouselEntry.OwnerSession            = Handle;
                TmpCarouselEntry.Handle                  = FullCarouselEntryHandle.word;
                TmpCarouselEntry.EntryType               = STPTI_CAROUSEL_ENTRY_TYPE_SIMPLE;
                TmpCarouselEntry.InjectionType           = STPTI_INJECTION_TYPE_ONE_SHOT_INJECT;

                TmpCarouselEntry.NextEntry.word          = STPTI_NullHandle();
                TmpCarouselEntry.PrevEntry.word          = STPTI_NullHandle();

                TmpCarouselEntry.NextPidEntry.word       = STPTI_NullHandle();
                TmpCarouselEntry.PrevPidEntry.word       = STPTI_NullHandle();
                TmpCarouselEntry.Linked                  = FALSE;

                TmpCarouselEntry.CarouselListHandle.word = STPTI_NullHandle();

                memcpy((U8 *) stptiMemGet_CarouselSimpleEntry(FullCarouselEntryHandle), (U8 *) &TmpCarouselEntry, sizeof(CarouselSimpleEntry_t));

                *CarouselEntryHandle = FullCarouselEntryHandle.word;
            }
        }
        else    /* STPTI_CAROUSEL_ENTRY_TYPE_TIMED */
        {
            Error = stpti_CreateObjectHandle(FullSessionHandle, OBJECT_TYPE_CAROUSEL_ENTRY, sizeof(CarouselTimedEntry_t), &FullCarouselEntryHandle);
            if (Error == ST_NO_ERROR)
            {
                CarouselTimedEntry_t TmpCarouselEntry;  /* Load Parameters */

                TmpCarouselEntry.OwnerSession            = Handle;
                TmpCarouselEntry.Handle                  = FullCarouselEntryHandle.word;
                TmpCarouselEntry.EntryType               = STPTI_CAROUSEL_ENTRY_TYPE_TIMED;
                TmpCarouselEntry.InjectionType           = STPTI_INJECTION_TYPE_ONE_SHOT_INJECT;

                TmpCarouselEntry.PacketData_p            = NULL;

                TmpCarouselEntry.NextEntry.word          = STPTI_NullHandle();
                TmpCarouselEntry.PrevEntry.word          = STPTI_NullHandle();

                TmpCarouselEntry.NextPidEntry.word       = STPTI_NullHandle();
                TmpCarouselEntry.PrevPidEntry.word       = STPTI_NullHandle();
                TmpCarouselEntry.Linked                  = FALSE;
                TmpCarouselEntry.CarouselListHandle.word = STPTI_NullHandle();

                memcpy((U8 *) stptiMemGet_CarouselTimedEntry(FullCarouselEntryHandle), (U8 *) &TmpCarouselEntry, sizeof(CarouselTimedEntry_t));

                *CarouselEntryHandle = FullCarouselEntryHandle.word;
            }
        }
    }

    STOS_SemaphoreSignal(stpti_MemoryLock);
    return( Error );
}


/******************************************************************************
Function Name : STPTI_CarouselInsertEntry
  Description :
   Parameters :
         Note : This function can only be used for 'simple' entries, 
                'timed' entries must use STPTI_CarouselInsertTimedEntry().
******************************************************************************/
ST_ErrorCode_t STPTI_CarouselInsertEntry(STPTI_Carousel_t CarouselHandle, STPTI_AlternateOutputTag_t AlternateOutputTag,
                                         STPTI_InjectionType_t InjectionType, STPTI_CarouselEntry_t Entry)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullCarouselHandle;
    FullHandle_t FullEntryHandle;
    FullHandle_t CarouselListHandle;
    FullHandle_t EntryListHandle;
    FullHandle_t LockingSession;


    STOS_SemaphoreWait(stpti_MemoryLock);
    FullCarouselHandle.word = CarouselHandle;
    FullEntryHandle.word    = Entry;

    Error = stptiValidate_CarouselInsertEntry( FullCarouselHandle, AlternateOutputTag, InjectionType, FullEntryHandle);
    if (Error == ST_NO_ERROR)
    {
        LockingSession = (stptiMemGet_Carousel(FullCarouselHandle))->LockingSession;
        if (LockingSession.word == STPTI_NullHandle() || LockingSession.word == CarouselHandle)
        {
            CarouselSimpleEntry_t *Entry_p = stptiMemGet_CarouselSimpleEntry(FullEntryHandle);

            /* Either Carousel isn't locked, or locked by this session so OK to modify */

            Entry_p->InjectionType = InjectionType;

            {
                STPTI_Pid_t Pid;    /* Set entry's Pid */

                Pid = Entry_p->PacketData[1] & 0x1f;
                Pid <<= 8;
                Pid |= Entry_p->PacketData[2];
                Entry_p->Pid = Pid;
            }

            /* Insert it into list */
            EntryListHandle = stptiMemGet_Carousel(FullCarouselHandle)->CarouselEntryListHandle;

            if (EntryListHandle.word == STPTI_NullHandle())
            {
                /* This will invalidate existing object pointers */
                Error = stpti_CreateListObject(FullCarouselHandle, &EntryListHandle);
                if (Error == ST_NO_ERROR)
                {
                    stptiMemGet_Carousel(FullCarouselHandle)->CarouselEntryListHandle = EntryListHandle;
                }
                
                /* Get the new object pointer */
                Entry_p = stptiMemGet_CarouselSimpleEntry(FullEntryHandle);
            }

            if (Error == ST_NO_ERROR)
            {
                CarouselListHandle = Entry_p->CarouselListHandle;

                if (CarouselListHandle.word == STPTI_NullHandle())
                {
                    /* This will invalidate existing object pointers */
                    Error = stpti_CreateListObject(FullEntryHandle, &CarouselListHandle);
                    if (Error == ST_NO_ERROR)
                    {
                        /* Get the new object pointer */
                        Entry_p = stptiMemGet_CarouselSimpleEntry(FullEntryHandle);
                        Entry_p->CarouselListHandle = CarouselListHandle;
                    }
                }
            }

            if (Error == ST_NO_ERROR)
            {
                Error = stpti_AssociateObjects(EntryListHandle, FullCarouselHandle, CarouselListHandle, FullEntryHandle);
                if (Error == ST_NO_ERROR)
                {
                    Carousel_t *Carousel_p = stptiMemGet_Carousel(FullCarouselHandle);

                    /* It's in so fix up the links, new entries always added at end of list */
                    if (Carousel_p->StartEntry.word == STPTI_NullHandle())
                    {
                        /* This must be the first entry */
                        Carousel_p->StartEntry.word = FullEntryHandle.word;
                        Carousel_p->NextEntry.word = FullEntryHandle.word;
                    }
                    else
                    {
                        FullHandle_t PrevEntry;

                        /* Adding to end of existing List */
                        PrevEntry.word = Carousel_p->EndEntry.word;
                        stptiMemGet_CarouselSimpleEntry(PrevEntry)->NextEntry.word = FullEntryHandle.word;
                        Entry_p->PrevEntry.word = PrevEntry.word;
                    }

                    Carousel_p->EndEntry.word = FullEntryHandle.word;

                    if (InjectionType == STPTI_INJECTION_TYPE_REPEATED_INJECT_WITH_CC_FIXUP)
                    {
                        /* we must add pid linking so that cc-fixup can be acheived */

                        /* search through the list to identify the previous and next entrys
                           that are of type REPEATED_INJECT_WITH_CC_FIXUP and have the same
                           pid, NOTE: we only ever add entries at the end of the list, so
                           starting at the beginning, the first one we come to will be the
                           'nextpid' element from that we can extract the 'previouspid'
                           element */

                        FullHandle_t TmpEntryHandle = Carousel_p->StartEntry;

                        while (TRUE)
                        {
                            if ( (stptiMemGet_CarouselSimpleEntry(TmpEntryHandle)->Pid == Entry_p->Pid ) &&
                                 (stptiMemGet_CarouselSimpleEntry(TmpEntryHandle)->InjectionType == 
                                                    STPTI_INJECTION_TYPE_REPEATED_INJECT_WITH_CC_FIXUP ) )
                            {
                                /* Found one !! */
                                if (TmpEntryHandle.word == FullEntryHandle.word)
                                {
                                    /* only me, so link to myself!! */
                                    Entry_p->NextPidEntry = FullEntryHandle;
                                    Entry_p->PrevPidEntry = FullEntryHandle;
                                    Entry_p->CCValue = 0;
                                }
                                else
                                {
                                    FullHandle_t NextPidEntry = TmpEntryHandle;
                                    FullHandle_t PrevPidEntry =
                                        stptiMemGet_CarouselSimpleEntry(TmpEntryHandle)->PrevPidEntry;

                                    /* Fixup Previous element */

                                    stptiMemGet_CarouselSimpleEntry(PrevPidEntry)->NextPidEntry = FullEntryHandle;

                                    /* Fixup Next element */

                                    stptiMemGet_CarouselSimpleEntry(NextPidEntry)->PrevPidEntry = FullEntryHandle;

                                    /* Fixup This elemnet */
                                    Entry_p->NextPidEntry = NextPidEntry;
                                    Entry_p->PrevPidEntry = PrevPidEntry;
                                    Entry_p->CCValue = 0;
                                }
                                break;
                            }
                            else
                            {
                                TmpEntryHandle = stptiMemGet_CarouselSimpleEntry(TmpEntryHandle)->NextEntry;
                                if (TmpEntryHandle.word == STPTI_NullHandle())
                                {
                                    Error = STPTI_ERROR_INTERNAL_ALL_MESSED_UP;
                                    break;
                                }
                            }
                        }
                    }

                    stptiMemGet_CarouselSimpleEntry(FullEntryHandle)->Linked = TRUE;
                }

                if ( (Error == ST_NO_ERROR) && (LockingSession.word == STPTI_NullHandle()) )
                {
                    semaphore_t *Sem_p;

                    /* Give the Carousel a kick to start it off... */
                    Sem_p = stptiMemGet_Carousel(FullCarouselHandle)->Sem_p;
                    if (Sem_p != NULL)
                    {
                        STTBX_Print(("Signaling CarouselTask (from STPTI_CarouselInsertEntry)\n"));
                        STOS_SemaphoreSignal(Sem_p);
                    }
                }

            }
        }
        else
        {
            Error = STPTI_ERROR_CAROUSEL_LOCKED_BY_DIFFERENT_SESSION;
        }
    }

    STOS_SemaphoreSignal(stpti_MemoryLock);
    return( Error );
}


/******************************************************************************
Function Name : STPTI_CarouselInsertTimedEntry
  Description :
   Parameters :
         Note : This function can only be used for 'timed' entries, 
                'simple' entries must use STPTI_CarouselEntryLoad().   
******************************************************************************/

ST_ErrorCode_t STPTI_CarouselInsertTimedEntry(STPTI_Carousel_t CarouselHandle, U8 *TSPacket_p, U8 CCValue,
                                              STPTI_InjectionType_t InjectionType, STPTI_TimeStamp_t MinOutputTime,
                                              STPTI_TimeStamp_t MaxOutputTime, BOOL EventOnOutput, BOOL EventOnTimeout,
                                              U32 PrivateData, STPTI_CarouselEntry_t Entry, U8 FromByte)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullCarouselHandle;
    FullHandle_t FullEntryHandle;
    FullHandle_t CarouselListHandle;
    FullHandle_t EntryListHandle;
    FullHandle_t LockingSession;


    STOS_SemaphoreWait(stpti_MemoryLock);
    FullCarouselHandle.word = CarouselHandle;
    FullEntryHandle.word    = Entry;

    Error = stptiValidate_CarouselInsertTimedEntry( FullCarouselHandle, TSPacket_p, CCValue, InjectionType, MinOutputTime, MaxOutputTime, EventOnOutput, EventOnTimeout, PrivateData, FullEntryHandle, FromByte);
    if (Error == ST_NO_ERROR)
    {
        LockingSession = (stptiMemGet_Carousel(FullCarouselHandle))->LockingSession;

        if (LockingSession.word == STPTI_NullHandle() || LockingSession.word == CarouselHandle)
        {
                CarouselTimedEntry_t *Entry_p = stptiMemGet_CarouselTimedEntry(FullEntryHandle);

                /* Either Carousel isn't locked, or locked by this session so OK to modify */

                Entry_p->PacketData_p   = TSPacket_p;       /* Set passed entry's parameters */   
                Entry_p->InjectionType  = InjectionType;
                Entry_p->MinOutputTime  = MinOutputTime;
                Entry_p->MaxOutputTime  = MaxOutputTime;
                Entry_p->EventOnOutput  = EventOnOutput;
                Entry_p->EventOnTimeout = EventOnTimeout;
                Entry_p->PrivateData    = PrivateData;
                /* From byte set to zero until correct handling is achieved ie have to output 188 bytes */
             /* Entry_p->FromByte       = FromByte; */
                Entry_p->FromByte       = 0;
                Entry_p->CCValue        = CCValue;
                STTBX_Print(("STPTI_CarouselInsertTimedEntry->Setting Entry_p->CCValue to 0x%x\n",CCValue));

                {
                    STPTI_Pid_t Pid;    /* Store entry's Pid */

                    Pid = ((U8 *) Entry_p->PacketData_p)[1] & 0x1f;
                    Pid <<= 8;
                    Pid |= ((U8 *) Entry_p->PacketData_p)[2];
                    Entry_p->Pid = Pid;
                }

                /* Create associations and Insert it into list */
                EntryListHandle = (stptiMemGet_Carousel(FullCarouselHandle))->CarouselEntryListHandle;

                if (EntryListHandle.word == STPTI_NullHandle())
                {
                    Error = stpti_CreateListObject(FullCarouselHandle, &EntryListHandle);
                    if (Error == ST_NO_ERROR)
                    {
                        (stptiMemGet_Carousel(FullCarouselHandle))->CarouselEntryListHandle = EntryListHandle;
                    }
                }

                if (Error == ST_NO_ERROR)
                {
                    CarouselListHandle = (stptiMemGet_CarouselTimedEntry(FullEntryHandle))->CarouselListHandle;

                    if (CarouselListHandle.word == STPTI_NullHandle())
                    {
                        Error = stpti_CreateListObject(FullEntryHandle, &CarouselListHandle);
                        if (Error == ST_NO_ERROR)
                        {
                            (stptiMemGet_CarouselTimedEntry(FullEntryHandle))->CarouselListHandle = CarouselListHandle;
                        }
                    }
                }
                /* End of creating associations */

                if (Error == ST_NO_ERROR)
                {

                    Error = stpti_AssociateObjects(EntryListHandle, FullCarouselHandle, CarouselListHandle, FullEntryHandle);
                    if (Error == ST_NO_ERROR)
                    {
                        Carousel_t *Carousel_p = stptiMemGet_Carousel(FullCarouselHandle);

                        /* It's in so fix up the links, new entries always added at end of list */
                        if (Carousel_p->StartEntry.word == STPTI_NullHandle())
                        {
                            /* This must be the first entry */
                            Carousel_p->StartEntry.word = FullEntryHandle.word;
                            Carousel_p->NextEntry.word = FullEntryHandle.word;
                        }
                        else
                        {
                            FullHandle_t PrevEntry;

                            /* Adding to end of existing List */
                            PrevEntry.word = Carousel_p->EndEntry.word;
                            (stptiMemGet_CarouselTimedEntry(PrevEntry))->NextEntry.word = FullEntryHandle.word;
                            Entry_p->PrevEntry.word = PrevEntry.word;
                        }

                        Carousel_p->EndEntry.word = FullEntryHandle.word;

                        if (InjectionType == STPTI_INJECTION_TYPE_REPEATED_INJECT_WITH_CC_FIXUP)
                        {

                            /* we must add pid linking so that cc-fixup can be acheived */

                            /* search through the list to identify the previous and next entrys
                               that are of type REPEATED_INJECT_WITH_CC_FIXUP and have the same
                               pid, NOTE: we only ever add entries at the end of the list, so
                               starting at the beginning, the first one we come to will be the
                               'nextpid' element from that we can extract the 'previouspid'
                               element */

                            FullHandle_t TmpEntryHandle = Carousel_p->StartEntry;

                            while (TRUE)
                            {
                                if (((stptiMemGet_CarouselTimedEntry(TmpEntryHandle))->Pid == Entry_p->Pid)
                                    && ((stptiMemGet_CarouselTimedEntry(TmpEntryHandle))->InjectionType ==
                                        STPTI_INJECTION_TYPE_REPEATED_INJECT_WITH_CC_FIXUP))
                                {
                                    /* Found one !! */
                                    if (TmpEntryHandle.word == FullEntryHandle.word)
                                    {
                                        /* only me, so link to myself!! */
                                        Entry_p->NextPidEntry = FullEntryHandle;
                                        Entry_p->PrevPidEntry = FullEntryHandle;
                                        Entry_p->LastCC = 0;
                                    }
                                    else
                                    {
                                        FullHandle_t NextPidEntry = TmpEntryHandle;
                                        FullHandle_t PrevPidEntry =
                                            (stptiMemGet_CarouselTimedEntry(TmpEntryHandle))->PrevPidEntry;

                                        /* Fixup Previous element */

                                        (stptiMemGet_CarouselTimedEntry(PrevPidEntry))->NextPidEntry =
                                            FullEntryHandle;

                                        /* Fixup Next element */

                                        (stptiMemGet_CarouselTimedEntry(NextPidEntry))->PrevPidEntry =
                                            FullEntryHandle;

                                        /* Fixup This elemnet */
                                        Entry_p->NextPidEntry = NextPidEntry;
                                        Entry_p->PrevPidEntry = PrevPidEntry;
                                        Entry_p->LastCC = 0;
                                    }
                                    break;
                                }
                                else
                                {
                                    TmpEntryHandle = (stptiMemGet_CarouselTimedEntry(TmpEntryHandle))->NextEntry;

                                    if ( TmpEntryHandle.word == STPTI_NullHandle() )
                                    {
                                        Error = STPTI_ERROR_INTERNAL_ALL_MESSED_UP;
                                        break;
                                    }
                                }
                            }
                        }

                        (stptiMemGet_CarouselTimedEntry(FullEntryHandle))->Linked = TRUE;
                    }

                    if ( (Error == ST_NO_ERROR) && (LockingSession.word == STPTI_NullHandle()) )
                    {
                        semaphore_t *Sem_p;

                        /* Give the Carousel a kick to start it off... */
                        Sem_p = (stptiMemGet_Carousel(FullCarouselHandle))->Sem_p;
                        if (Sem_p != NULL)
                        {
                            STOS_SemaphoreSignal(Sem_p);
                        }

                    }
                }                    
        }
        else
        {
            Error = STPTI_ERROR_CAROUSEL_LOCKED_BY_DIFFERENT_SESSION;
        }
    }

    STOS_SemaphoreSignal(stpti_MemoryLock);
    return( Error );
}


/******************************************************************************
Function Name : STPTI_CarouselLock
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_CarouselLock(STPTI_Carousel_t CarouselHandle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullCarouselHandle;
    FullHandle_t LockingSession;

    STOS_SemaphoreWait(stpti_MemoryLock);
    FullCarouselHandle.word = CarouselHandle;

    Error = stptiValidate_GeneralCarouselHandleCheck( FullCarouselHandle );
    if (Error == ST_NO_ERROR)
    {
        LockingSession = (stptiMemGet_Carousel(FullCarouselHandle))->LockingSession;

        if (LockingSession.word == STPTI_NullHandle())
        {
            /* Carousel Not Locked.... so LOCK it */
            (stptiMemGet_Carousel(FullCarouselHandle))->LockingSession.word = CarouselHandle;
            Error = ST_NO_ERROR;
        }
        else
        {
            Error = STPTI_ERROR_CAROUSEL_LOCKED_BY_DIFFERENT_SESSION;
        }
    }

    STOS_SemaphoreSignal(stpti_MemoryLock);
    return( Error );
}


/******************************************************************************
Function Name : STPTI_CarouselGetCurrentEntry
  Description :Gets information on the entry currently loaded in the carousel
               awaiting output.
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_CarouselGetCurrentEntry(STPTI_Carousel_t CarouselHandle, STPTI_CarouselEntry_t *Entry_p, U32 *PrivateData_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullCarouselHandle;


    STOS_SemaphoreWait(stpti_MemoryLock);
    FullCarouselHandle.word = CarouselHandle;

    Error = stptiValidate_GeneralCarouselHandleCheck( FullCarouselHandle );
    if (Error == ST_NO_ERROR)
    {
        Carousel_t *Carousel_p = stptiMemGet_Carousel(FullCarouselHandle);
        if (NULL != Entry_p)       *Entry_p       = Carousel_p->CurrentEntry.word;
        if (NULL != PrivateData_p) *PrivateData_p = Carousel_p->PrivateData;
    }

    STOS_SemaphoreSignal(stpti_MemoryLock);
    return( Error );
}


/******************************************************************************
Function Name : STPTI_CarouselRemoveEntry
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_CarouselRemoveEntry(STPTI_Carousel_t CarouselHandle, STPTI_CarouselEntry_t Entry)
{
    ST_ErrorCode_t Error;
    FullHandle_t FullCarouselHandle;
    FullHandle_t FullEntryHandle;
    FullHandle_t LockingSession;


    STOS_SemaphoreWait(stpti_MemoryLock);
    FullCarouselHandle.word = CarouselHandle;
    FullEntryHandle.word    = Entry;

    Error = stptiValidate_GeneralCarouselAndEntryHandleCheck( FullCarouselHandle, FullEntryHandle );
    if (Error == ST_NO_ERROR)
    {
        LockingSession = (stptiMemGet_Carousel(FullCarouselHandle))->LockingSession;

        if (LockingSession.word == STPTI_NullHandle() || LockingSession.word == CarouselHandle)
        {
            /* Either Carousel isn't locked, or locked by this session so OK to modify */

            Error = (stptiMemGet_Device(FullCarouselHandle)->EntryType == STPTI_CAROUSEL_ENTRY_TYPE_SIMPLE) 
                        ? stpti_CarouselDisassociateSimpleEntry(FullCarouselHandle, FullEntryHandle, FALSE) 
                        : stpti_CarouselDisassociateTimedEntry(FullCarouselHandle, FullEntryHandle, FALSE);
        }
        else
        {
            Error = STPTI_ERROR_CAROUSEL_LOCKED_BY_DIFFERENT_SESSION;
        }
    }

    STOS_SemaphoreSignal(stpti_MemoryLock);
    return( Error );
}


/******************************************************************************
Function Name : STPTI_CarouselSetAllowOutput
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_CarouselSetAllowOutput(STPTI_Carousel_t CarouselHandle, STPTI_AllowOutput_t OutputAllowed)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullCarouselHandle;
    

    STOS_SemaphoreWait(stpti_MemoryLock);
    FullCarouselHandle.word = CarouselHandle;

    Error =  stptiValidate_CarouselSetAllowOutput( FullCarouselHandle, OutputAllowed);
    if (Error == ST_NO_ERROR)
    {
        (stptiMemGet_Carousel(FullCarouselHandle))->OutputAllowed = OutputAllowed;

        Error = stptiHAL_CarouselSetAllowOutput(FullCarouselHandle);                
    }

    STOS_SemaphoreSignal(stpti_MemoryLock);
    return( Error );
}


/******************************************************************************
Function Name : STPTI_CarouselSetBurstMode
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_CarouselSetBurstMode(STPTI_Carousel_t CarouselHandle, U32 PacketTimeMs, U32 CycleTimeMs)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullCarouselHandle;


    STOS_SemaphoreWait(stpti_MemoryLock);
    FullCarouselHandle.word = CarouselHandle;

    Error = stptiValidate_GeneralCarouselHandleCheck( FullCarouselHandle );
    if (Error == ST_NO_ERROR)
    {
        (stptiMemGet_Carousel(FullCarouselHandle))->PacketTimeTicks = ( PacketTimeMs * TICKS_PER_SECOND ) / 1000;
        (stptiMemGet_Carousel(FullCarouselHandle))->CycleTimeTicks  = ( CycleTimeMs * TICKS_PER_SECOND ) / 1000;
    }

    STOS_SemaphoreSignal(stpti_MemoryLock);
    return( Error );
}


/******************************************************************************
Function Name : STPTI_CarouselUnlinkBuffer
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_CarouselUnlinkBuffer(STPTI_Carousel_t CarouselHandle, STPTI_Buffer_t BufferHandle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullCarouselHandle;
    FullHandle_t FullBufferHandle;


    STOS_SemaphoreWait(stpti_MemoryLock);
    FullCarouselHandle.word = CarouselHandle;
    FullBufferHandle.word   = BufferHandle;

    Error = stptiValidate_CarouselUnlinkBuffer( FullCarouselHandle, FullBufferHandle);
    if (Error == ST_NO_ERROR)
    {
        Error = stpti_BufferDisassociateCarousel(FullBufferHandle, FullCarouselHandle, FALSE);
    }

    STOS_SemaphoreSignal(stpti_MemoryLock);
    return( Error );
}


/******************************************************************************
Function Name : STPTI_CarouselUnLock
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_CarouselUnLock(STPTI_Carousel_t CarouselHandle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullCarouselHandle;
    FullHandle_t LockingSession;
    semaphore_t *Sem_p;


    STOS_SemaphoreWait(stpti_MemoryLock);
    FullCarouselHandle.word = CarouselHandle;

    Error = stptiValidate_GeneralCarouselHandleCheck( FullCarouselHandle );
    if (Error == ST_NO_ERROR)
    {
        LockingSession = (stptiMemGet_Carousel(FullCarouselHandle))->LockingSession;

        if (LockingSession.word == STPTI_NullHandle())
        {
            Error = STPTI_ERROR_CAROUSEL_NOT_LOCKED;
        }
        else if (LockingSession.word == CarouselHandle)
        {
            /* Locked by this session so OK to unlock it */
            (stptiMemGet_Carousel(FullCarouselHandle))->LockingSession.word = STPTI_NullHandle();

            Sem_p = (stptiMemGet_Carousel(FullCarouselHandle))->Sem_p;

            if (Sem_p != NULL)
            {
                STOS_SemaphoreSignal(Sem_p);
            }
        }
        else
        {
            Error = STPTI_ERROR_CAROUSEL_LOCKED_BY_DIFFERENT_SESSION;
        }
    }

    STOS_SemaphoreSignal(stpti_MemoryLock);
    return( Error );
}


/******************************************************************************
Function Name : STPTI_CarouselDeallocate
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_CarouselDeallocate(STPTI_Carousel_t CarouselHandle)
{
    ST_ErrorCode_t Error = STPTI_ERROR_INTERNAL_NOT_EXPLICITLY_SET;
    FullHandle_t FullCarouselHandle;

    STOS_SemaphoreWait(stpti_MemoryLock);
    FullCarouselHandle.word = CarouselHandle;

    Error = stptiValidate_GeneralCarouselHandleCheck( FullCarouselHandle );
    if (Error == ST_NO_ERROR)
    {
        Error = stpti_DeallocateCarousel(FullCarouselHandle, TRUE);
    }

    STOS_SemaphoreSignal(stpti_MemoryLock);
    return( Error );
}


/******************************************************************************
Function Name : STPTI_CarouselEntryDeallocate
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_CarouselEntryDeallocate(STPTI_CarouselEntry_t CarouselEntryHandle)
{
    ST_ErrorCode_t Error = STPTI_ERROR_INTERNAL_NOT_EXPLICITLY_SET;
    FullHandle_t FullCarouselEntryHandle;


    STOS_SemaphoreWait(stpti_MemoryLock);
    FullCarouselEntryHandle.word = CarouselEntryHandle;

    Error = stptiValidate_GeneralCarouselEntryHandleCheck( FullCarouselEntryHandle );
    if (Error == ST_NO_ERROR)
    {        
        Error = stpti_DeallocateCarouselEntry(FullCarouselEntryHandle, FALSE);
    }

    STOS_SemaphoreSignal(stpti_MemoryLock);
    return( Error );
}


/******************************************************************************
Function Name :
  Description :
   Parameters :
         Note : This function can only be used for 'simple' entries, 
                'timed' entries must use STPTI_CarouselInsertTimedEntry().
******************************************************************************/
ST_ErrorCode_t STPTI_CarouselEntryLoad(STPTI_CarouselEntry_t CarouselEntryHandle, STPTI_TSPacket_t Packet, U8 FromByte)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullEntryHandle;


    STOS_SemaphoreWait(stpti_MemoryLock);
    FullEntryHandle.word = CarouselEntryHandle;

    Error = stptiValidate_CarouselEntryLoad( FullEntryHandle, Packet, FromByte);
    if (Error == ST_NO_ERROR)
    {
        {            
            CarouselSimpleEntry_t *Entry_p = stptiMemGet_CarouselSimpleEntry(FullEntryHandle);
            U8 *Dest_p = &((U8 *) stptiMemGet_CarouselSimpleEntry(FullEntryHandle)->PacketData)[0];
            U8 *Src_p = &((U8 *) Packet)[0];
            size_t Size = sizeof(STPTI_TSPacket_t);

            Entry_p->FromByte = FromByte;
            memcpy(Dest_p, Src_p, Size);
        } 
    }

    STOS_SemaphoreSignal(stpti_MemoryLock);
    return( Error );
}


/******************************************************************************
Function Name : stpti_CarouselUnlinkSimpleEntry
  Description :
   Parameters :
******************************************************************************/
void stpti_CarouselUnlinkSimpleEntry(FullHandle_t CarouselHandle, FullHandle_t EntryHandle)
{
    CarouselSimpleEntry_t *Entry_p = stptiMemGet_CarouselSimpleEntry(EntryHandle);
    Carousel_t            *Carousel_p = stptiMemGet_Carousel(CarouselHandle);

    FullHandle_t PrevEntry = Entry_p->PrevEntry;
    FullHandle_t NextEntry = Entry_p->NextEntry;

    if (Entry_p->InjectionType == STPTI_INJECTION_TYPE_REPEATED_INJECT_WITH_CC_FIXUP)
    {
        /* Pid linking needs to be sorted..... */

        FullHandle_t NextPidEntry = Entry_p->NextPidEntry;
        FullHandle_t PrevPidEntry = Entry_p->PrevPidEntry;

        stptiMemGet_CarouselSimpleEntry(PrevPidEntry)->NextPidEntry = NextPidEntry;
        stptiMemGet_CarouselSimpleEntry(NextPidEntry)->PrevPidEntry = PrevPidEntry;
        stptiMemGet_CarouselSimpleEntry(PrevPidEntry)->CCValue =
        stptiMemGet_CarouselSimpleEntry(EntryHandle)->CCValue;
    }

    if ((PrevEntry.word == STPTI_NullHandle()) && (NextEntry.word == STPTI_NullHandle()))
    {
        /* Only entry in list */

        Carousel_p->StartEntry.word = STPTI_NullHandle();
        Carousel_p->EndEntry.word = STPTI_NullHandle();
    }
    else if (PrevEntry.word == STPTI_NullHandle())
    {
        /* This was the first entry of list */

        stptiMemGet_CarouselSimpleEntry(NextEntry)->PrevEntry.word = STPTI_NullHandle();
        Carousel_p->StartEntry = NextEntry;
    }
    else if (NextEntry.word == STPTI_NullHandle())
    {
        /* This was the last entry of list */

        stptiMemGet_CarouselSimpleEntry(PrevEntry)->NextEntry.word = STPTI_NullHandle();
        Carousel_p->EndEntry = PrevEntry;
    }
    else
    {
        /* Mid list */

        stptiMemGet_CarouselSimpleEntry(NextEntry)->PrevEntry.word = PrevEntry.word;
        stptiMemGet_CarouselSimpleEntry(PrevEntry)->NextEntry.word = NextEntry.word;
    }

    if (Carousel_p->NextEntry.word == EntryHandle.word)
    {
        /* this entry was about to be transmitted so fix up */

        if (NextEntry.word == STPTI_NullHandle())
        {
            Carousel_p->NextEntry.word = Carousel_p->StartEntry.word;
        }
        else
        {
            Carousel_p->NextEntry.word = NextEntry.word;
        }
    }
    
    /* Clear the pointers in the entry itself */

    stptiMemGet_CarouselSimpleEntry(EntryHandle)->PrevEntry.word = STPTI_NullHandle();
    stptiMemGet_CarouselSimpleEntry(EntryHandle)->NextEntry.word = STPTI_NullHandle();
    stptiMemGet_CarouselSimpleEntry(EntryHandle)->Linked         = FALSE;
}


/******************************************************************************
Function Name : stpti_CarouselUnlinkTimedEntry
  Description :
   Parameters :
******************************************************************************/
void stpti_CarouselUnlinkTimedEntry(FullHandle_t CarouselHandle, FullHandle_t EntryHandle)
{
    CarouselTimedEntry_t *Entry_p = stptiMemGet_CarouselTimedEntry(EntryHandle);
    Carousel_t           *Carousel_p = stptiMemGet_Carousel(CarouselHandle);

    FullHandle_t PrevEntry = Entry_p->PrevEntry;
    FullHandle_t NextEntry = Entry_p->NextEntry;

    if (Entry_p->InjectionType == STPTI_INJECTION_TYPE_REPEATED_INJECT_WITH_CC_FIXUP)
    {
        /* Pid linking needs to be sorted..... */

        FullHandle_t NextPidEntry = Entry_p->NextPidEntry;
        FullHandle_t PrevPidEntry = Entry_p->PrevPidEntry;

        stptiMemGet_CarouselTimedEntry(PrevPidEntry)->NextPidEntry = NextPidEntry;
        stptiMemGet_CarouselTimedEntry(NextPidEntry)->PrevPidEntry = PrevPidEntry;
        stptiMemGet_CarouselTimedEntry(PrevPidEntry)->LastCC =
        stptiMemGet_CarouselTimedEntry(EntryHandle)->LastCC;
    }

    if ((PrevEntry.word == STPTI_NullHandle()) && (NextEntry.word == STPTI_NullHandle()))
    {
        /* Only entry in list */

        Carousel_p->StartEntry.word = STPTI_NullHandle();
        Carousel_p->EndEntry.word   = STPTI_NullHandle();
    }
    else if (PrevEntry.word == STPTI_NullHandle())
    {
        /* This was the first entry of list */

        stptiMemGet_CarouselTimedEntry(NextEntry)->PrevEntry.word = STPTI_NullHandle();
        Carousel_p->StartEntry = NextEntry;
    }
    else if (NextEntry.word == STPTI_NullHandle())
    {
        /* This was the last entry of list */

        stptiMemGet_CarouselTimedEntry(PrevEntry)->NextEntry.word = STPTI_NullHandle();
        Carousel_p->EndEntry = PrevEntry;
    }
    else
    {
        /* Mid list */

        stptiMemGet_CarouselTimedEntry(NextEntry)->PrevEntry.word = PrevEntry.word;
        stptiMemGet_CarouselTimedEntry(PrevEntry)->NextEntry.word = NextEntry.word;
    }

    if (Carousel_p->NextEntry.word == EntryHandle.word)
    {
        /* this entry was about to be transmitted so fix up */

        if (NextEntry.word == STPTI_NullHandle())
        {
            Carousel_p->NextEntry.word = Carousel_p->StartEntry.word;
        }
        else
        {
            Carousel_p->NextEntry.word = NextEntry.word;
        }
    }
    
    /* Clear the pointers in the entry itself */
    
    stptiMemGet_CarouselTimedEntry(EntryHandle)->PrevEntry.word = STPTI_NullHandle();
    stptiMemGet_CarouselTimedEntry(EntryHandle)->NextEntry.word = STPTI_NullHandle();
    stptiMemGet_CarouselTimedEntry(EntryHandle)->Linked         = FALSE;
}


/******************************************************************************
Function Name : stpti_SimpleCarouselTask
  Description :
   Parameters :
******************************************************************************/
static void stpti_SimpleCarouselTask(CarouselLaunchParams_t *CarouselLaunchParams_p)
{
    FullHandle_t           CarouselHandle;
    STPTI_DevicePtr_t      DataDest_p;
    STPTI_DevicePtr_t      Control_p;
    STPTI_EventData_t      EventData;
    Carousel_t            *Carousel_p;
    CarouselSimpleEntry_t *Entry_p;
    FullHandle_t           NextEntryHandle;
    semaphore_t           *Sem_p;
    semaphore_t           *ReadCarouselParams_p;
    U32                   *src_p;
    U32                    PacketTimeTicks = 0;
    U32                    CycleTimeTicks  = 0;
    U32                    NextPacketDelay = 0;
    BOOL                   AbortFlag       = FALSE;
    int                    MissedSignals   = 0;


    CarouselHandle.word  = CarouselLaunchParams_p->CarouselHandle;
    ReadCarouselParams_p = CarouselLaunchParams_p->ReadCarouselParams_p;

    EventData.SlotHandle = STPTI_NullHandle();

    STOS_SemaphoreSignal(ReadCarouselParams_p);

    STOS_SemaphoreWait(stpti_MemoryLock);

    ( void )stptiHAL_CarouselGetSubstituteDataStart( CarouselHandle,  &Control_p);

    DataDest_p = Control_p + 2;

    Sem_p = (stptiMemGet_Carousel(CarouselHandle))->Sem_p;
    assert(Sem_p != NULL);

    PacketsSent = 0;          

    while (!AbortFlag)
    {
        /* Wait for TC to be ready for next Carousel Entry */

        STOS_SemaphoreSignal(stpti_MemoryLock);
        STOS_SemaphoreWait(Sem_p);
        STOS_SemaphoreWait(stpti_MemoryLock);
            
        if (stptiMemGet_Carousel(CarouselHandle)->AbortFlag)
        {
            stptiMemGet_Device(CarouselHandle)->CarouselSem_p = NULL;
            break;
        }

        {
            /* Wait for the specified time to inject packet */

            if (NextPacketDelay > 0)
            {
                STOS_SemaphoreSignal(stpti_MemoryLock);
                task_delay(NextPacketDelay);
                NextPacketDelay = 0;
                STOS_SemaphoreWait(stpti_MemoryLock);
            }

            Carousel_p = stptiMemGet_Carousel(CarouselHandle);

            
            /* Get the PacketTimeTicks and CycleTimeTicks from the Carousel structure*/    
            PacketTimeTicks = Carousel_p->PacketTimeTicks;
            CycleTimeTicks  = Carousel_p->CycleTimeTicks;

            /* this piece of code is moved down for GNBvd27052 */
            
            if (Carousel_p->LockingSession.word == STPTI_NullHandle())
            {
                while(MissedSignals>0)
                {
                    STOS_SemaphoreSignal(Sem_p);
                    MissedSignals--;
                }
                
                Carousel_p->CurrentEntry.word = STPTI_NullHandle();
                NextEntryHandle.word = Carousel_p->NextEntry.word;
                if (NextEntryHandle.word == STPTI_NullHandle())
                {
                    /* Flag that there is no current entry */
                    Carousel_p->CurrentEntry.word = NextEntryHandle.word;
                    Carousel_p->UnlinkOnNextSem   = FALSE;
                }
                else
                {
                    Entry_p = stptiMemGet_CarouselSimpleEntry(NextEntryHandle);
                    if (Entry_p->InjectionType == STPTI_INJECTION_TYPE_REPEATED_INJECT_WITH_CC_FIXUP)
                    {
                        /* fixup new entry's CC value */

                        FullHandle_t PrevPidEntry = Entry_p->PrevPidEntry;
                        U8  LastCC = stptiMemGet_CarouselSimpleEntry(PrevPidEntry)->CCValue;
                        /* U32 *src_p = (U32 *) Entry_p->PacketData; */
                        src_p = (U32 *) Entry_p->PacketData;
                        
                        
                        if (((U8 *) src_p)[3] & PAYLOAD_PRESENT)
                        {
                            ++LastCC;
                            LastCC &= 0x0f;
                        }
                        Entry_p->CCValue = LastCC;
                    }
                    else 
                    {
                        src_p = (U32 *) Entry_p->PacketData;  
                    }

                    /* Load data into TC - can't use memcpy 'cos it seems to be confused by device
                       types */

                    {
                        U32 i;
                        U32 tmp = *src_p;
                        tmp &= ~0x0f000000;
                        tmp |= ((Entry_p->CCValue & 0x0f) << 24);
                        *DataDest_p = tmp;
                        
                        /* Wait for any other carousels to stop writing to TC */
                        STOS_SemaphoreWait(CarouselTCLockSem_p);
                                      
                        for (i = 0; i < DVB_TS_PACKET_LENGTH / sizeof(U32); ++i)
                        {
                            DataDest_p[i] = src_p[i];                    
                        }
                        /* release the semaphore */
                        STOS_SemaphoreSignal(CarouselTCLockSem_p);                        
                    }

                    /* Record the entry in the Carousel Structure */

                    Carousel_p->CurrentEntry.word = NextEntryHandle.word;
                    Carousel_p->UnlinkOnNextSem = (Entry_p->InjectionType == STPTI_INJECTION_TYPE_ONE_SHOT_INJECT);

                    /* Allow TC to output on next substitutable packet */
                    {
                        U32 Flags;
                        
                        Flags = (Entry_p->FromByte << 8) | 0x01;                                                
                        *Control_p = Flags;
                    }
                    ++PacketsSent;

                    /* Update 'NextEntry' in Carousel structure */

                    if (NextEntryHandle.word == Carousel_p->EndEntry.word)
                    {
                        /* we have cycled round all entries */

                        EventData.u.CarouselEntryEventData.EntryHandle = Carousel_p->CurrentEntry.word;

                        Carousel_p->NextEntry.word = Carousel_p->StartEntry.word;
                        NextPacketDelay = MAXIMUM(PacketTimeTicks, CycleTimeTicks);
#ifdef STPTI_DEBUG_SUPPORT
                        ++DebugStatistics.DMACompleteEventCount;
#endif
                        (void) stpti_QueueEvent(CarouselHandle, STPTI_EVENT_CAROUSEL_CYCLE_COMPLETE_EVT, &EventData);
                    }
                    else
                    {
                        Carousel_p->NextEntry.word = Entry_p->NextEntry.word;
                        NextPacketDelay = PacketTimeTicks;
                    }
                }
            }
            else
            {
                    MissedSignals++;
            }
            /* GNBvd 27052, Moved down to adjust link list in case of ONE_SHOT_MODE,
               we should not wait for task_delay to unlink the entry. */
            if (Carousel_p->UnlinkOnNextSem)
            {
                /* GNBvd 27051, changed from stpti_CarouselUnlinkSimpleEntry() to 
                   stpti_CarouselDisassociateSimpleEntry() so that entry should be unlinked and its 
                   list object is also deleted.*/
                (void) stpti_CarouselDisassociateSimpleEntry(CarouselHandle, Carousel_p->CurrentEntry, FALSE);
                Carousel_p->UnlinkOnNextSem = FALSE;
            }
        }

        /* Update Carousel parameters */

        AbortFlag = Carousel_p->AbortFlag;
    }
    STOS_SemaphoreSignal(stpti_MemoryLock);
}


/******************************************************************************
Function Name : stpti_TimeAfter
  Description : 
   Parameters :
       Return : Returns TRUE if TimeA is AFTER TimeB (i.e. TimeA > TimeB )
******************************************************************************/
static BOOL stpti_TimeAfter(STPTI_TimeStamp_t * TimeA_p, STPTI_TimeStamp_t * TimeB_p)
{
    if (TimeA_p->Bit32 > TimeB_p->Bit32)
    {
        return TRUE;
    }
    else if (TimeA_p->Bit32 < TimeB_p->Bit32)
    {
        return FALSE;
    }
    else if (TimeA_p->LSW > TimeB_p->LSW)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


/******************************************************************************
Function Name : stpti_TimedCarouselTask
  Description :
   Parameters :
******************************************************************************/
static void stpti_TimedCarouselTask(CarouselLaunchParams_t * CarouselLaunchParams_p)
{
    STPTI_EventData_t     EventData;
    Carousel_t           *Carousel_p;
    CarouselTimedEntry_t *Entry_p;
    FullHandle_t          CarouselHandle;
    FullHandle_t          NextEntryHandle;
    semaphore_t          *Sem_p;
    semaphore_t          *ReadCarouselParams_p;
    U32                  *src_p;
    int                   FirstPacket         = TRUE;
    BOOL                  AbortFlag           = FALSE;
    TCPrivateData_t      *PrivateData_p       = NULL;
    STPTI_TCParameters_t *TC_Params_p         = NULL;
    TCCarousel_t         *CarouselDataStart_p = NULL;
    int                   MissedSignals       = 0;
    
    CarouselHandle.word = CarouselLaunchParams_p->CarouselHandle;
    ReadCarouselParams_p = CarouselLaunchParams_p->ReadCarouselParams_p;
    
    PrivateData_p  = stptiMemGet_PrivateData(CarouselHandle);
    TC_Params_p    = (STPTI_TCParameters_t *) & PrivateData_p->TC_Params;
    CarouselDataStart_p = (TCCarousel_t *) (TC_Params_p->TC_SubstituteDataStart);

    EventData.SlotHandle = STPTI_NullHandle();

    STOS_SemaphoreSignal(ReadCarouselParams_p);

    PacketsSent = 0;

    STOS_SemaphoreWait(stpti_MemoryLock);

    Sem_p = stptiMemGet_Carousel(CarouselHandle)->Sem_p;
    assert(Sem_p != NULL);

    while (!AbortFlag)
    {
        STOS_SemaphoreSignal(stpti_MemoryLock);
        STOS_SemaphoreWait(Sem_p);
        STOS_SemaphoreWait(stpti_MemoryLock);
        
        Carousel_p = stptiMemGet_Carousel(CarouselHandle);
            
        if (!Carousel_p->AbortFlag)
        {
            stpti_TimedCarouselTaskCount++;

            if (!FirstPacket)
            { 
                if (Carousel_p->EventOnOutput)
                {
                    EventData.u.CarouselEntryEventData.PrivateData = Carousel_p->PrivateData;
                    EventData.u.CarouselEntryEventData.EntryHandle = Carousel_p->CurrentEntry.word;
                    EventData.u.CarouselEntryEventData.BufferCount = stptiMemGet_Device(CarouselHandle)->CarouselBufferCount;

                    Carousel_p->EventOnOutput = FALSE;
#ifdef STPTI_DEBUG_SUPPORT
                    ++DebugStatistics.CarouselTimedEntryEventCount;
#endif                                                
                    (void) stpti_QueueEvent(CarouselHandle, STPTI_EVENT_CAROUSEL_TIMED_ENTRY_EVT, &EventData);
                }

                if (Carousel_p->EventOnTimeout
                    && stpti_TimeAfter((STPTI_TimeStamp_t *) & stptiMemGet_Device(CarouselHandle)->CarouselSemTime,
                                       (STPTI_TimeStamp_t *) & Carousel_p->MaxOutputTime ))
                {
                    EventData.u.CarouselEntryEventData.PrivateData = Carousel_p->PrivateData;
                    EventData.u.CarouselEntryEventData.EntryHandle = Carousel_p->CurrentEntry.word;
#ifdef STPTI_DEBUG_SUPPORT
                    ++DebugStatistics.CarouselEntryTimeoutEventCount;
#endif                            
                    (void) stpti_QueueEvent(CarouselHandle, STPTI_EVENT_CAROUSEL_ENTRY_TIMEOUT_EVT, &EventData);
                }
            }
            
            if (Carousel_p->LockingSession.word == STPTI_NullHandle())
            {
                while(MissedSignals>0)
                {
                    STOS_SemaphoreSignal(Sem_p);
                    MissedSignals--;
                }

                Carousel_p->CurrentEntry.word = STPTI_NullHandle();
                NextEntryHandle.word = Carousel_p->NextEntry.word; 

                if (NextEntryHandle.word == STPTI_NullHandle())
                {
                    /* Flag that there is no current entry */
                    Carousel_p->CurrentEntry.word = NextEntryHandle.word;
                    Carousel_p->UnlinkOnNextSem   = FALSE;
                    Carousel_p->PrivateData       = 0; 
                    
                    Carousel_p->CurrentEntry.word = NextEntryHandle.word;
                    Carousel_p->UnlinkOnNextSem   = FALSE;
                    Carousel_p->PrivateData       = 0;
                }
                else
                {
                    FullHandle_t DeviceHandle;
                    STPTI_TimeStamp_t  CurrentTimeStamp;

                    Entry_p = stptiMemGet_CarouselTimedEntry(NextEntryHandle);
                    DeviceHandle = stptiMemGet_Device(NextEntryHandle)->Handle;

                    (void)stptiHAL_GetCurrentPTITimer(DeviceHandle, &CurrentTimeStamp);

                    if((CurrentTimeStamp.LSW >= Entry_p->MaxOutputTime.LSW)?
                        (Entry_p->MaxOutputTime.Bit32 <= CurrentTimeStamp.Bit32):
                        (Entry_p->MaxOutputTime.Bit32 < CurrentTimeStamp.Bit32))
                    {
                        STPTI_EventData_t EventData;                        
                        
                        if (Entry_p->EventOnTimeout)
                        {
                            EventData.u.CarouselEntryEventData.PrivateData = Entry_p->PrivateData;
                            EventData.u.CarouselEntryEventData.EntryHandle = Entry_p->Handle;
                            
#ifdef STPTI_DEBUG_SUPPORT
                            ++DebugStatistics.CarouselEntryTimeoutEventCount;
#endif                            
                            (void) stpti_QueueEvent(NextEntryHandle, STPTI_EVENT_CAROUSEL_ENTRY_TIMEOUT_EVT, &EventData);  
                        }
                    }
                    
                    
                    
                    if (Entry_p->InjectionType == STPTI_INJECTION_TYPE_REPEATED_INJECT_WITH_CC_FIXUP)
                    {
                        /* fixup new entry's CC value */

                        FullHandle_t PrevPidEntry = Entry_p->PrevPidEntry;
                        U8  LastCC = stptiMemGet_CarouselTimedEntry(PrevPidEntry)->CCValue;
                        src_p = (U32 *) Entry_p->PacketData_p;

                        
                        if (((U8 *) src_p)[3] & PAYLOAD_PRESENT)
                        {
                            ++LastCC;
                            LastCC &= 0x0f;
                        }
                        Entry_p->CCValue = LastCC;
                    }
                    else
                    {
                        src_p = (U32 *) Entry_p->PacketData_p;
                    }

                    /* Load data into TC - can't use memcpy 'cos it seems to be confused by device
                       types */
                    {
                        U32 i;
                        U32 tmp = *src_p;
                        U16 PTISessionNumber=0;
                         Device_t *Device_p=NULL;
                                                                        
                        tmp &= ~0x0f000000;                                                
                        tmp |= ((Entry_p->CCValue & 0x0f) << 24);
                        *src_p = tmp;                                                
                        
                        /* do non critical stuff before we take the semaphore */                        
                        Device_p         = stptiMemGet_Device(CarouselHandle);
                        PTISessionNumber = Device_p->Session;
                        
                        /* Wait for any other carousels to stop writing to the TC dataram */ 
                        STOS_SemaphoreSignal(stpti_MemoryLock);                                              
                        STOS_SemaphoreWait(CarouselTCLockSem_p); 
                        STOS_SemaphoreWait(stpti_MemoryLock);                        
                        {
                            U16 CarouselSetup=0;                            
                            
                            /* Ensure there is not already a carousel entry in the TC awaiting output */
                            while( (STSYS_ReadRegDev16LE((void*)&CarouselDataStart_p->CarouselControl) & TC_CAROUSEL_ENTRY_READY) != 0)    
                            {
                                STOS_SemaphoreSignal(stpti_MemoryLock);
#if defined(ST_OSLINUX)
                                schedule();
#else
                                task_delay(100);
#endif /* #endif defined(ST_OSLINUX) */
                                STOS_SemaphoreWait(stpti_MemoryLock);
                                
                                Carousel_p = stptiMemGet_Carousel(CarouselHandle); 
                                if (Carousel_p->AbortFlag) 
                                {
                                    /* We are quiting so there is no point in waiting any longer */
                                    break;
                                }                        
                            }
                             
                            if (!Carousel_p->AbortFlag)
                            {
                                /* there is not an existing carousel entry in the tc awaiting carouseling out */
                                
                                /* Set CPUWriting flag */
                                CarouselSetup = STSYS_ReadRegDev16LE((void*)&CarouselDataStart_p->CarouselControl); 
                                CarouselSetup |= TC_CAROUSEL_CPU_WRITING;
                                STSYS_WriteTCReg16LE((void*)&CarouselDataStart_p->CarouselControl, CarouselSetup);
                                
                                /* This carousel has the semaphore but ensure the TC is not reading the data */
                                while(STSYS_ReadRegDev16LE((void*)&CarouselDataStart_p->TCReading) == TC_CAROUSEL_TC_READING) /* wait if TC is reading Carousel data */                                    
                                {
                                    STOS_SemaphoreSignal(stpti_MemoryLock);
#if defined(ST_OSLINUX)
                                    schedule();
#else
                                    task_delay(100);
#endif /* #endif..else defined(ST_OSLINUX) */                         
                                    STOS_SemaphoreWait(stpti_MemoryLock);
                                    
                                    Carousel_p = stptiMemGet_Carousel(CarouselHandle); 
                                    if (Carousel_p->AbortFlag) 
                                    {
                                        /* We are quiting so there is no point in waiting any longer */
                                        break;
                                    }                       
                                } 

                                /* Ok lets write the entry into into the data ram */                                                        
                                STTBX_Print(("Inserting Entry on Session 0x%x\n", PTISessionNumber)); 
                                for (i = 0; i < (DVB_TS_PACKET_LENGTH ) / sizeof(U32); ++i)                        
                                {                            
                                    CarouselDataStart_p->CarouselData[i] = src_p[i]; 
                                }
                                                                
                                /* Set-up min time in TC data space */
                                STSYS_WriteTCReg16LE((void*)&CarouselDataStart_p->MinTime_0, (Entry_p->MinOutputTime.LSW >> 16) );
                                STSYS_WriteTCReg16LE((void*)&CarouselDataStart_p->MinTime_1, (Entry_p->MinOutputTime.LSW) );
                                /* set CarouselSetup to say the entry is ready to output and clear TC_CAROUSEL_CPU_WRITING */                                
                                CarouselSetup = Entry_p->FromByte << 8;                    
                                CarouselSetup |= TC_CAROUSEL_ENTRY_READY  | TC_CAROUSEL_TIMED_ENTRY | (PTISessionNumber<<2); /* shift session number into correct posn */
                                STSYS_WriteTCReg16LE((void*)&CarouselDataStart_p->CarouselControl, CarouselSetup  );
                                ++PacketsSent;
                                FirstPacket = FALSE;                                
                            }
                        }
                        /* release the semaphore */
                        STOS_SemaphoreSignal(CarouselTCLockSem_p);
                    }

                    /* Record the entry in the Carousel Structure */

                    Carousel_p->CurrentEntry.word = NextEntryHandle.word;
                    Carousel_p->UnlinkOnNextSem = (Entry_p->InjectionType == STPTI_INJECTION_TYPE_ONE_SHOT_INJECT);
                    Carousel_p->PrivateData = Entry_p->PrivateData;
                    Carousel_p->MaxOutputTime = Entry_p->MaxOutputTime;
                    Carousel_p->EventOnOutput = Entry_p->EventOnOutput;
                    Carousel_p->EventOnTimeout = Entry_p->EventOnTimeout;

                    /* Update 'NextEntry' in Carousel structure */

                    if (NextEntryHandle.word == Carousel_p->EndEntry.word)
                    {
                        /* we have cycled round all entries */

                        EventData.u.CarouselEntryEventData.PrivateData = Carousel_p->PrivateData;
                        EventData.u.CarouselEntryEventData.EntryHandle = Carousel_p->CurrentEntry.word;

                        Carousel_p->NextEntry.word = Carousel_p->StartEntry.word;
#ifdef STPTI_DEBUG_SUPPORT
                        ++DebugStatistics.DMACompleteEventCount;
#endif                        
                        (void) stpti_QueueEvent(CarouselHandle, STPTI_EVENT_CAROUSEL_CYCLE_COMPLETE_EVT, &EventData);
                    }
                    else
                    {
                        Carousel_p->NextEntry.word = Entry_p->NextEntry.word;
                    }
                }

                /* GNBvd 27052, Moved down to adjust link list in case of ONE_SHOT_MODE,
                   we should not wait for task_delay to unlink the entry. */
                if (Carousel_p->UnlinkOnNextSem)
                {
                    /* GNBvd 27051, changed from stpti_CarouselUnlinkTimedEntry() to 
                       stpti_CarouselDisassociateTimedEntry() so that entry should be unlinked and its 
                       list object is also deleted.*/
                    (void) stpti_CarouselDisassociateTimedEntry(CarouselHandle, Carousel_p->CurrentEntry, FALSE);
                    Carousel_p->UnlinkOnNextSem = FALSE;
                }
            }
            else
            {
                    MissedSignals++;
            }

        }

        /* Update Carousel parameters */
        Carousel_p = stptiMemGet_Carousel(CarouselHandle);
        AbortFlag  = Carousel_p->AbortFlag;
    }
    STOS_SemaphoreSignal(stpti_MemoryLock);
}


/* ------------------------------------------------------------------------- */


U32 stpti_CarouselGetPacketsSent()
{
    return PacketsSent;
}


/* ------------------------------------------------------------------------- */


/* --- EOF --- */
