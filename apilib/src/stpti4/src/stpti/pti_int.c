/******************************************************************************
COPYRIGHT (C) STMicroelectronics 2005,2006.  All rights reserved.

   File Name: pti_int.c
 Description: task to process messages pushed into a queue by the
              stpti interrupt handler (TC generated interrupt).

******************************************************************************/

/* Includes ---------------------------------------------------------------- */

#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
#define STTBX_PRINT
#define DEBUG 3
#endif

#if !defined(ST_OSLINUX)
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#endif /* #endif !defined(ST_OSLINUX) */

#include "stcommon.h"
#include "sttbx.h"
#include "stpti.h"

#include "pti_hndl.h"
#include "pti_loc.h"
#include "pti_mem.h"
#include "validate.h"

#include "memget.h"
#include "pti_evt.h"
#include "pti_loc.h"
#include "pti_hndl.h"
#include "pti_hal.h"
#include "memget.h"
#include "cam.h"
#include "pti4.h"
#include "isr.h"

#ifdef STPTI_DEBUG_SUPPORT
extern STPTI_DebugStatistics_t       DebugStatistics;
extern STPTI_DebugInterruptStatus_t *DebugInterruptStatus;

extern int  IntInfoIndex;
extern int  IntInfoCapacity;
extern BOOL IntInfoStart;

extern volatile U32 stpti_InterruptTotalCount;  /* defined in pti_ba.c */
#endif

/* Const ------------------------------------------------------------------- */

#if defined(ST_OSLINUX)
#define ISR_TASK_NAME "stpti4_IntTask"
static DECLARE_COMPLETION(pticore_inttask_exited);
#else
#define ISR_TASK_NAME "stpti_InterruptTask"
/* this must be anything other than NULL or a valid working (ram) pointer address */
#define TASK_QUIT_PATTERN   0xfffffefe
#endif /* #endif..else defined(ST_OSLINUX) */

/* Public Variables -------------------------------------------------------- */

extern Driver_t STPTI_Driver;

volatile U32 stpti_InterruptTaskCount;

void *IntTaskStack_p; /* pointer to interrupt task stack GNbvd21068 */


/* Private Function Prototypes --------------------------------------------- */

#if defined( ST_OSLINUX )
    static int stpti_InterruptTask( void *arg);
#else
__INLINE static void stpti_InterruptTask( void );
#endif /* #endif..else defined( ST_OSLINUX ) */

#ifdef STPTI_DEBUG_SUPPORT
__INLINE static void stpti_addInterruptDebugEntry( U32               DMANumber,
        TCDevice_t        *TC_Device_p,
        U32               InterruptStatus,
        TCPrivateData_t   *PrivateData_p,
        const TCStatus_t  *Status_p,
        FullHandle_t      EventBufferHandle );
#endif /* #ifdef STPTI_DEBUG_SUPPORT */

__INLINE static void stpti_processPacketSignal( U32 DMANumber,
        const STPTI_TCParameters_t *TC_Params_p,
        FullHandle_t *SlotHandle_p,
        STPTI_EventData_t *EventData_p,
        const TCPrivateData_t *PrivateData_p,
        const FullHandle_t FullDeviceHandle,
        U32 BytesInBuffer,
        const FullHandle_t EventSlotHandle,
        const TCStatus_t *Status_p,
        BOOL *PacketSignalled_p );


#if !defined ( STPTI_BSL_SUPPORT )
__INLINE static void stpti_getSlotHandleForPCRBasedEvent( STPTI_EventData_t *EventData_p,
        const TCStatus_t *Status_p,
        const STPTI_TimeStamp_t *ArrivalTime_p,
        U16 ArrivalTimeExtension,
        const Device_t *Device_p,
        TCPrivateData_t *PrivateData_p,
        FullHandle_t FullDeviceHandle,
        STPTI_TimeStamp_t *PCRTime_p );
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */

__INLINE static void stpti_getSlotHandleForSlotBasedEvent( STPTI_EventData_t *EventData_p,
                                                           FullHandle_t EventSlotHandle,
                                                           FullHandle_t FullDeviceHandle,
                                                           TCPrivateData_t *PrivateData_p,
                                                           const TCStatus_t  *Status_p,
                                                           BOOL *PacketDumped_p );

__INLINE static void stpti_interruptFailed( FullHandle_t FullDeviceHandle );

#ifndef STPTI_NO_INDEX_SUPPORT
__INLINE static void stpti_defineIndex( STPTI_EventData_t *EventData_p,
                                        FullHandle_t EventSlotHandle,
                                        FullHandle_t EventBufferHandle,
                                        const TCStatus_t *Status_p,
                                        Device_t *Device_p,
                                        const STPTI_TCParameters_t   *TC_Params_p,
                                        STPTI_TimeStamp_t *PCRTime_p,
                                        U16 PCRTimeExtension,
                                        STPTI_TimeStamp_t *ArrivalTime,
                                        U16 ArrivalTimeExtension,
                                        FullHandle_t FullDeviceHandle,
                                        BOOL PacketDumped,
                                        BOOL PacketSignalled );


__INLINE static void stpti_makeDVBIndexDefinition( STPTI_IndexDefinition_t *IndexDef_p,
                                                   const TCStatus_t *Status_p );

__INLINE static void stpti_setScramblingStatusFlags( STPTI_IndexDefinition_t *IndexDef_p,
                                                     const TCStatus_t *Status_p );
#endif

#if !defined ( STPTI_BSL_SUPPORT )


__INLINE static void stpti_processIndex( Device_t   *Device_p,
                                         TCSessionInfo_t  *TCSessionInfo_p,
                                         STPTI_TimeStamp_t *ArrivalTime_p,
                                         U16 *ArrivalTimeExtension_p,
                                         STPTI_TimeStamp_t *PCRTime_p,
                                         U16 *PCRTimeExtension_p,
                                         const TCStatus_t *Status_p );
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */

__INLINE static void stpti_processStatusBlock( U32 *BytesInBuffer,
                                               U32 *ReadPtr_p,
                                               U32 WritePtr,
                                               U32 TopPtr,
                                               U32 BasePtr,
                                               U32 BasePtr_physical,
                                               TCPrivateData_t *PrivateData_p,
                                               const STPTI_TCParameters_t *TC_Params_p,
                                               STPTI_EventData_t *EventData_p,
                                               TCInterruptDMAConfig_t *TCInterruptDMAConfig_p,
                                               TCSessionInfo_t *TCSessionInfo_p,
                                               U32 *DMANumber_p,
                                               TCStatus_t *Status_p,
                                               FullHandle_t *EventBufferHandle_p );

#if !defined ( STPTI_BSL_SUPPORT )
__INLINE static void stpti_recoverPCR( const TCStatus_t  *Status_p,
                                       STPTI_TimeStamp_t *PCRTime_p,
                                       U16               *PCRTimeExtension_p );

__INLINE static void stpti_scrambleChange( FullHandle_t FullDeviceHandle,
                                           const TCStatus_t *Status_p,
                                           STPTI_EventData_t *EventData_p,
                                           TCPrivateData_t *PrivateData_p );
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */

__INLINE static void stpti_sendInvalidInterruptMessage( FullHandle_t FullDeviceHandle );

#ifdef STPTI_CAROUSEL_SUPPORT
__INLINE static void stpti_signalCarouselEntryArrival( Device_t                  *Device_p,
                                                       const TCStatus_t          *Status_p,
                                                       const STPTI_TimeStamp_t   *ArrivalTime_p );
#endif

#if !defined ( STPTI_BSL_SUPPORT )
__INLINE static void stpti_signalTCCodeFault( STPTI_EventData_t *EventData_p,
                                              const FullHandle_t FullDeviceHandle,
                                              const TCStatus_t *Status_p,
                                              BOOL *PacketDumped_p );

__INLINE static void stpti_statusBufferCorruption( FullHandle_t FullDeviceHandle );
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */

/* Functions --------------------------------------------------------------- */


#if !defined ( STPTI_BSL_SUPPORT )
/******************************************************************************
Function Name : stpti_InterruptQueueTerm
  Description : Performs a controlled termination of the message queue used by
                the ISR to issue messages to the Interrupt task.
   Parameters : None
******************************************************************************/
ST_ErrorCode_t stpti_InterruptQueueTerm( void )
{
    ST_ErrorCode_t error = ST_NO_ERROR;
#if defined( ST_OSLINUX )
    /* Release memory lock while waiting for task to shut down */
    STOS_SemaphoreSignal(stpti_MemoryLock);

    stptiHAL_StopWaiting(); /* This causes the interrupt task to stop. */

    wait_for_completion(&pticore_inttask_exited);           /* Wait for task to exit */

    STOS_SemaphoreWait(stpti_MemoryLock);   /* Reset Lock */

    STOS_MessageQueueDelete( STPTI_Driver.InterruptQueue_p );  /* delete the queue */
    STPTI_Driver.InterruptQueue_p = NULL;                   /* Make sure it can't be used */
#else
    BackendInterruptData_t *BackendInterruptData_p;
    CLOCK time;

    STTBX_Print(("\nstpti_InterruptQueueTerm()\n"));

    do  /* grab a message */
    {
        time = STOS_time_plus(STOS_time_now(), 10);

        BackendInterruptData_p = STOS_MessageQueueClaimTimeout(STPTI_Driver.InterruptQueue_p, &time);
    }
    while (BackendInterruptData_p == NULL);

    /* Send the terminate pattern to the isr task */
    BackendInterruptData_p->FullHandle.word = TASK_QUIT_PATTERN;
    STOS_MessageQueueSend(STPTI_Driver.InterruptQueue_p, BackendInterruptData_p );

    /* Release memory lock while waiting for task to shut down */
    STOS_SemaphoreSignal(stpti_MemoryLock);

    time = STOS_time_plus(STOS_time_now(), TICKS_PER_SECOND * 10);

    /* Wait for task to exit */

    if ( STOS_TaskWait((task_t **)&STPTI_Driver.InterruptTaskId_p, &time) != ST_NO_ERROR )
    {
        error = ST_ERROR_TIMEOUT;
    }
    else
    {
        error = STOS_TaskDelete ( STPTI_Driver.InterruptTaskId_p,
                                  STPTI_Driver.MemCtl.Partition_p,
                                  IntTaskStack_p,
                                  STPTI_Driver.MemCtl.Partition_p );

        if( error == ST_NO_ERROR )
        {
            STOS_MessageQueueDelete( STPTI_Driver.InterruptQueue_p );  /* delete the queue */
            STPTI_Driver.InterruptQueue_p = NULL;                   /* Make sure it can't be used */
        }
    }

    STOS_SemaphoreWait(stpti_MemoryLock);   /* Reset Lock */

#endif /* #endif..else defined( ST_OSLINUX ) */

    return error;
}
#endif /* #if !defined ( STPTI_BSL_SUPPORT )    */


/******************************************************************************
Function Name : stpti_InterruptQueueInit
  Description : Creates both the interrupt task, and the message queue used by
                the ISR to communicate with the interrupt task.
   Parameters : ST_Partition_t *Partition_p - pointer to the partition used to
                allocate memory required by the interrupt task and its message
                queue.
******************************************************************************/
ST_ErrorCode_t stpti_InterruptQueueInit( ST_Partition_t *Partition_p )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    static tdesc_t TaskDesc;    /* task descriptor GNbvd21068 */

    /* Initialise the interrupt status queue*/
#if defined (ST_OSLINUX)
    stptiHAL_InitInterruptCtl();
#endif

    STPTI_Driver.InterruptQueue_p = STOS_MessageQueueCreate( sizeof( BackendInterruptData_t ), INTERRUPT_QUEUE_SIZE );

    if (STPTI_Driver.InterruptQueue_p == NULL)
    {
        Error = ST_ERROR_NO_MEMORY;
    }
    else    /* Create a process for handling interrupt notification */
    {
        Error = STOS_TaskCreate( (void (*)(void *))stpti_InterruptTask,
                                 NULL,
                                 Partition_p,
                                 STPTI_Driver.InterruptTaskStackSize,
                                 &IntTaskStack_p,
                                 Partition_p,
                                 (task_t **)&STPTI_Driver.InterruptTaskId_p,
                                 &TaskDesc,
                                 STPTI_Driver.InterruptTaskPriority,
                                 ISR_TASK_NAME,
                                 0 );

    }

    return Error;
}


/******************************************************************************
Function Name : sendInvalidInterruptMessage
  Description : When notice of an invalid interrupt is received from the ISR,
                notifies STEVT via its message queue.
   Parameters : FullHandle_t FullDeviceHandle - handle for the current virtual PTI.
******************************************************************************/
__INLINE static void stpti_sendInvalidInterruptMessage( FullHandle_t FullDeviceHandle )
{
    /* code in isr should mean that this error is only reported once
      => ONE ERROR MAY BE SIGNIFICANT */

    STPTI_EventData_t EventData;

    EventData.u.ErrEventData.DMANumber      = 0XFEEDABBA;
    EventData.u.ErrEventData.BytesRemaining = 0;
    (void)stpti_DirectQueueEvent(FullDeviceHandle, STPTI_EVENT_INTERRUPT_FAIL_EVT, &EventData);

    STOS_SemaphoreWait( stpti_MemoryLock );
    stptiHAL_ErrorEventModifyState( FullDeviceHandle, STPTI_EVENT_INTERRUPT_FAIL_EVT, FALSE );
    STOS_SemaphoreSignal( stpti_MemoryLock );

#ifdef STPTI_DEBUG_SUPPORT

    ++DebugStatistics.InterruptFailEventCount;
    ++DebugStatistics.InterruptStatusCorruptionCount;
#endif
}


#if !defined ( STPTI_BSL_SUPPORT )
/******************************************************************************
Function Name : scrambleChange
  Description : Handles changes to the scrambling state of a stream.
   Parameters :
******************************************************************************/
__INLINE static void stpti_scrambleChange( FullHandle_t FullDeviceHandle,
                                           const TCStatus_t *Status_p,
                                           STPTI_EventData_t *EventData_p,
                                           TCPrivateData_t *PrivateData_p )
{
    if (Status_p->Scrambled)
    {
#ifdef STPTI_DEBUG_SUPPORT
        ++DebugStatistics.ClearChangeToScrambledEventCount;
#endif

        (void)stpti_DirectQueueEvent(FullDeviceHandle, STPTI_EVENT_CLEAR_TO_SCRAMBLED_EVT, EventData_p);
    }
    else
    {
#ifdef STPTI_DEBUG_SUPPORT
        ++DebugStatistics.ScrambledChangeToClearEventCount;
#endif

        (void)stpti_DirectQueueEvent(FullDeviceHandle, STPTI_EVENT_SCRAMBLED_TO_CLEAR_EVT, EventData_p);
    }

    /* Protect access to tc structure PrivateData */
    STOS_SemaphoreWait( stpti_MemoryLock );

    /* Update 'Seen' status */
    PrivateData_p->SeenStatus_p[Status_p->SlotNumber] |= TC_SEEN_PACKET;

    if ( Status_p->Scrambled )
    {
        if (Status_p->PESScrambled)
        {
            PrivateData_p->SeenStatus_p[Status_p->SlotNumber] |= TC_SEEN_PES_SCRAMBLED_PACKET;
        }
        else
        {
            PrivateData_p->SeenStatus_p[Status_p->SlotNumber] = TC_SEEN_TRANSPORT_SCRAMBLED_PACKET;
        }
    }

    STOS_SemaphoreSignal( stpti_MemoryLock );

}
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */


/******************************************************************************
Function Name : interruptFailed
  Description : Notifies STEVT of a failed interrupt.
   Parameters : FullHandle_t FullDeviceHandle - handle for the current virtual PTI.
******************************************************************************/
__INLINE static void stpti_interruptFailed( FullHandle_t FullDeviceHandle )
{
    STPTI_EventData_t       EventData;

    EventData.u.ErrEventData.DMANumber      = 0XDEADBEAF;
    EventData.u.ErrEventData.BytesRemaining = 0;


    /*
     * This clean up should no longer be necessary as isr disables interrupt failed
     * interrupts after their first occurrance.  Status Block queue should recover
     * bu itself.
     *
     *  stpti_clearMessageQueue( InterruptQueue_p );
     *  / * Reset the buffer * /
     *  TCInterruptDMAConfig_p->DMARead_p = TCInterruptDMAConfig_p->DMAWrite_p;
     */

#ifdef STPTI_DEBUG_SUPPORT

    ++DebugStatistics.InterruptFailEventCount;
#endif

#if defined( ST_OSLINUX )
    printk(KERN_ALERT"pticore: stpti_interruptFailed() -- STPTI_EVENT_INTERRUPT_FAIL_EVT\n");
#endif
    (void)stpti_DirectQueueEvent(FullDeviceHandle, STPTI_EVENT_INTERRUPT_FAIL_EVT, &EventData);


    STOS_SemaphoreWait( stpti_MemoryLock );
    stptiHAL_ErrorEventModifyState( FullDeviceHandle, STPTI_EVENT_INTERRUPT_FAIL_EVT, FALSE );
    STOS_SemaphoreSignal( stpti_MemoryLock );
}



#if !defined ( STPTI_BSL_SUPPORT )
/******************************************************************************
Function Name : statusBufferCorruption
  Description : Notifies STEVT of corruption detected in the status buffer.
   Parameters : FullHandle_t FullDeviceHandle - handle for the current virtual PTI.
******************************************************************************/
__INLINE static void stpti_statusBufferCorruption( FullHandle_t FullDeviceHandle )
{
    STPTI_EventData_t       EventData;

    EventData.u.ErrEventData.DMANumber      = 0XDEADABBA;
    EventData.u.ErrEventData.BytesRemaining = 0;

#if defined( ST_OSLINUX )
    printk(KERN_ALERT"pticore: stpti_statusBufferCorruption() -- STPTI_EVENT_TC_CODE_FAULT_EVT\n");
#endif
    (void)stpti_DirectQueueEvent(FullDeviceHandle, STPTI_EVENT_TC_CODE_FAULT_EVT, &EventData);

    STOS_SemaphoreWait( stpti_MemoryLock );
    stptiHAL_ErrorEventModifyState( FullDeviceHandle, STPTI_EVENT_TC_CODE_FAULT_EVT, FALSE );
    STOS_SemaphoreSignal( stpti_MemoryLock );
}
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */




/******************************************************************************
Function Name : makeDVBIndexDefinition
  Description : Prepares an index event from information in the status block.
   Parameters : STPTI_IndexDefinition_t *IndexDef_p - pointer to the index definition to be prepared.
                const TCStatus_t *Status_p - pointer to the current status block.
******************************************************************************/
#ifndef STPTI_NO_INDEX_SUPPORT
__INLINE static void stpti_makeDVBIndexDefinition( STPTI_IndexDefinition_t *IndexDef_p,
                                                   const TCStatus_t *Status_p )
{
    IndexDef_p->s.PayloadUnitStartIndicator    = (Status_p->Flags & STATUS_FLAGS_PUSI_FLAG)                 ? 1 : 0;
    IndexDef_p->s.DiscontinuityIndicator       = (Status_p->Flags & STATUS_FLAGS_DISCONTINUITY_INDICATOR)   ? 1 : 0;
    IndexDef_p->s.RandomAccessIndicator        = (Status_p->Flags & STATUS_FLAGS_RANDOM_ACCESS_INDICATOR)   ? 1 : 0;
    IndexDef_p->s.PriorityIndicator            = (Status_p->Flags & STATUS_FLAGS_PRIORITY_INDICATOR)        ? 1 : 0;
    IndexDef_p->s.PCRFlag                      = (Status_p->Flags & STATUS_FLAGS_PCR_FLAG)                  ? 1 : 0;
    IndexDef_p->s.OPCRFlag                     = (Status_p->Flags & STATUS_FLAGS_OPCR_FLAG)                 ? 1 : 0;
    IndexDef_p->s.SplicingPointFlag            = (Status_p->Flags & STATUS_FLAGS_SPLICING_POINT_FLAG)       ? 1 : 0;
    IndexDef_p->s.TransportPrivateDataFlag     = (Status_p->Flags & STATUS_FLAGS_PRIVATE_DATA_FLAG)         ? 1 : 0;
    IndexDef_p->s.AdaptationFieldExtensionFlag = (Status_p->Flags & STATUS_FLAGS_ADAPTATION_EXTENSION_FLAG) ? 1 : 0;
    IndexDef_p->s.MPEGStartCode                = (Status_p->Flags & STATUS_FLAGS_START_CODE_FLAG)           ? 1 : 0;
}
#endif


/******************************************************************************
Function Name : setScramblingStatusFlags
  Description : Sets the scrambling status flags for an index event.
   Parameters : STPTI_IndexDefinition_t *IndexDef_p - pointer to the index definition to be prepared.
                const TCStatus_t *Status_p - pointer to the current status block.
******************************************************************************/
#ifndef STPTI_NO_INDEX_SUPPORT
__INLINE static void stpti_setScramblingStatusFlags( STPTI_IndexDefinition_t *IndexDef_p,
                                                     const TCStatus_t *Status_p )
{
    IndexDef_p->s.FirstRecordedPacket =
        (Status_p->Flags & STATUS_FLAGS_FIRST_RECORDED_PACKET) ? 1 : 0;
    IndexDef_p->s.Reserved = 0;

    if ( Status_p->Flags & STATUS_FLAGS_SCRAMBLE_CHANGE )
    {
        if ( Status_p->Scrambled )
        {
            if ( Status_p->Odd_Even == 1 )
            {
                IndexDef_p->s.ScramblingChangeToOdd = 1;
            }
            else
            {
                IndexDef_p->s.ScramblingChangeToEven = 1;
            }
        }
        else
        {
            IndexDef_p->s.ScramblingChangeToClear = 1;
        }
    }
}
#endif


#if !defined ( STPTI_BSL_SUPPORT )
/******************************************************************************
Function Name : recoverPCR
  Description :
   Parameters : const TCStatus_t *Status_p - pointer to the current status block.
******************************************************************************/
__INLINE static void stpti_recoverPCR( const TCStatus_t  *Status_p,
                                       STPTI_TimeStamp_t *PCRTime_p,
                                       U16               *PCRTimeExtension_p )
{
    U8 RawData[5];

    /* Casting required here, because of an st200 compiler issue that is to be resovled */
    RawData[0] = (U8) (( ((U16)Status_p->Pcr0) & 0xff00) >> 8);
    RawData[1] = (U8) Status_p->Pcr0 & 0xff;
    RawData[2] = (U8) (( ((U16)Status_p->Pcr1) & 0xff00) >> 8);
    RawData[3] = (U8) Status_p->Pcr1 & 0xff;
    RawData[4] = (U8) (( ((U16)Status_p->Pcr2) & 0xff00) >> 8);

    stptiHAL_PcrToTimestamp( PCRTime_p, RawData );

    *PCRTimeExtension_p = Status_p->Pcr2 & 0x1ff;
}
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */


/******************************************************************************
Function Name : signalCarouselEntryArrival
  Description :
   Parameters : const TCStatus_t *Status_p - pointer to the current status block.
******************************************************************************/
#ifdef STPTI_CAROUSEL_SUPPORT
__INLINE static void stpti_signalCarouselEntryArrival( Device_t                  *Device_p,
                                                       const TCStatus_t          *Status_p,
                                                       const STPTI_TimeStamp_t   *ArrivalTime_p )
{
    if (Device_p->CarouselSem_p != NULL)
    {
        Device_p->CarouselSemTime = *ArrivalTime_p;


    }
}
#endif





/******************************************************************************
Function Name : getSlotHandleForBufferBasedEvent
  Description :
   Parameters : const TCStatus_t *Status_p - pointer to the current status block.
******************************************************************************/
__INLINE static void stpti_processPacketSignal( U32 DMANumber,
                                                const STPTI_TCParameters_t *TC_Params_p,
                                                FullHandle_t *SlotHandle_p,
                                                STPTI_EventData_t *EventData_p,
                                                const TCPrivateData_t *PrivateData_p,
                                                const FullHandle_t FullDeviceHandle,
                                                U32 BytesInBuffer,
                                                const FullHandle_t EventSlotHandle,
                                                const TCStatus_t *Status_p,
                                                BOOL *PacketSignalled_p )
{
    FullHandle_t            BufferHandle;
    BufferInterrupt_t       BufferInterrupt;

#ifdef STPTI_DEBUG_SUPPORT

    ++DebugStatistics.PacketSignalCount;
#endif

    STOS_SemaphoreWait( stpti_MemoryLock );

    /* Is it a valid buffer number? */
    if (DMANumber >= TC_Params_p->TC_NumberDMAs)
    {
        EventData_p->u.ErrEventData.DMANumber      = DMANumber;
        EventData_p->u.ErrEventData.BytesRemaining = BytesInBuffer;
#ifdef STPTI_DEBUG_SUPPORT

        ++DebugStatistics.InterruptFailEventCount;
        ++DebugStatistics.InterruptBufferErrorCount;
#endif

        STOS_SemaphoreSignal(stpti_MemoryLock);         /* UNLOCK MEMORY READY FOR RAISING EVENT */
        (void)stpti_DirectQueueEvent(FullDeviceHandle, STPTI_EVENT_INTERRUPT_FAIL_EVT, EventData_p);
        STOS_SemaphoreWait( stpti_MemoryLock );         /* LOCK MEMORY */

        stptiHAL_ErrorEventModifyState( FullDeviceHandle, STPTI_EVENT_INTERRUPT_FAIL_EVT, FALSE );

    }
    else
    {
        TCDMAConfig_t *TC_DMAConfig_p = &((TCDMAConfig_t *) TC_Params_p->TC_DMAConfigStart)[DMANumber];
        U32 SignalModeFlags = STSYS_ReadRegDev16LE((void*)&(TC_DMAConfig_p->SignalModeFlags));
        U32 BufferPacketCount;

        /* Ignore packet signals if we are disabled */
        if ( !(SignalModeFlags & TC_DMA_CONFIG_SIGNAL_MODE_FLAGS_SIGNAL_DISABLE) )
        {
            BufferInterrupt.SlotHandle        = EventSlotHandle.word;
            BufferInterrupt.BufferPacketCount = Status_p->BufferPacketNumber;
            BufferInterrupt.BufferHandle      = PrivateData_p->BufferHandles_p[DMANumber];
            BufferInterrupt.DMANumber         = DMANumber;

            BufferHandle.word  = BufferInterrupt.BufferHandle;
            DMANumber          = BufferInterrupt.DMANumber;
            SlotHandle_p->word = BufferInterrupt.SlotHandle;
            BufferPacketCount  = BufferInterrupt.BufferPacketCount;

#if !defined ( STPTI_BSL_SUPPORT )
            if (ST_NO_ERROR != stptiValidate_IntBufferHandle(BufferHandle) )
            {
                STTBX_Print(("Interrupt->Bad buffer handle 0X%08X %d\n", BufferHandle.word, DMANumber));
            }
            else    /*  == ST_NO_ERROR */
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */
            {
                STOS_MessageQueue_t *SignalQueue_p;

                /* disable further buffer Interrupts */
                SignalModeFlags |= TC_DMA_CONFIG_SIGNAL_MODE_FLAGS_SIGNAL_DISABLE;
                STSYS_SetTCMask16LE((void*)&(TC_DMAConfig_p->SignalModeFlags), SignalModeFlags);

                *PacketSignalled_p = TRUE;

                /* update stpti data */
                (stptiMemGet_Buffer(BufferHandle))->BufferPacketCount = BufferPacketCount;

                SignalQueue_p = (stptiMemGet_Buffer(BufferHandle))->Queue_p;

                if (SignalQueue_p != NULL)
                {
                    STPTI_Buffer_t *Signalbuffer_p;
                    Signalbuffer_p = (STPTI_Buffer_t *)STOS_MessageQueueClaimTimeout( SignalQueue_p, TIMEOUT_IMMEDIATE );

                    STOS_SemaphoreSignal(stpti_MemoryLock);
                    /* AT THIS POINT MEMORY IS UNLOCKED READY FOR MESSAGE SENDING / RAISING EVENTS */

                    if( Signalbuffer_p != NULL )
                    {
                        Signalbuffer_p[0] = BufferHandle.word;
                        Signalbuffer_p[1] = 0XEEEEEEEE;
                        STOS_MessageQueueSend( SignalQueue_p, Signalbuffer_p );
                        return;     /* We're done, exit now to avoid having to do a wait on a semaphore and a signal (not time efficient) */
                    }
                    else
                    {
                        /* If no messages are available to claim, notify STEVT... */
#if defined(ST_OSLINUX_INT_DEBUG)
                        printk(KERN_ALERT"pticore: STPTI_EVENT_BUFFER_OVERFLOW_EVT\n");
#endif

                        EventData_p->u.ErrEventData.DMANumber      = 0xBABEBABE;
        
                        /* This is not the correct event to call, but in the absence of anything better, it will suffice. */
                        (void)stpti_DirectQueueEvent( FullDeviceHandle, STPTI_EVENT_INTERRUPT_FAIL_EVT, EventData_p );

                        STOS_SemaphoreWait( stpti_MemoryLock );         /* LOCK MEMORY */
                        stptiHAL_ErrorEventModifyState( FullDeviceHandle, STPTI_EVENT_INTERRUPT_FAIL_EVT, FALSE );
                    }
                }
            }
        }
    }

    STOS_SemaphoreSignal(stpti_MemoryLock);     /* UNLOCK MEMORY READY FOR EXIT */

}


#if !defined ( STPTI_BSL_SUPPORT )
/******************************************************************************
Function Name : signalTCCodeFault
  Description :
   Parameters : const TCStatus_t *Status_p - pointer to the current status block.
******************************************************************************/
__INLINE static void stpti_signalTCCodeFault( STPTI_EventData_t *EventData_p,
                                              const FullHandle_t FullDeviceHandle,
                                              const TCStatus_t *Status_p,
                                              BOOL *PacketDumped_p )
{
    EventData_p->SlotHandle = STPTI_NullHandle ();

    if (Status_p->Flags & STATUS_FLAGS_TC_CODE_FAULT)
    {
#ifdef STPTI_DEBUG_SUPPORT
        ++DebugStatistics.TCCodeFaultEventCount;
#endif

        (void)stpti_DirectQueueEvent(FullDeviceHandle, STPTI_EVENT_TC_CODE_FAULT_EVT, EventData_p);

        STOS_SemaphoreWait( stpti_MemoryLock );
        stptiHAL_ErrorEventModifyState( FullDeviceHandle, STPTI_EVENT_TC_CODE_FAULT_EVT, FALSE );
        STOS_SemaphoreSignal( stpti_MemoryLock );
   }
    *PacketDumped_p = TRUE;
}

/******************************************************************************
Function Name : getSlotHandleForPCRBasedEvent
  Description :
   Parameters : const TCStatus_t *Status_p - pointer to the current status block.
******************************************************************************/
__INLINE static void stpti_getSlotHandleForPCRBasedEvent( STPTI_EventData_t *EventData_p,
                                                          const TCStatus_t *Status_p,
                                                          const STPTI_TimeStamp_t *ArrivalTime_p,
                                                          U16 ArrivalTimeExtension,
                                                          const Device_t *Device_p,
                                                          TCPrivateData_t *PrivateData_p,
                                                          FullHandle_t FullDeviceHandle,
                                                          STPTI_TimeStamp_t *PCRTime_p )
{
    FullHandle_t EventPCRSlotHandle;


    /* Protect access to tc structure PrivateData */
    STOS_SemaphoreWait( stpti_MemoryLock );

    EventPCRSlotHandle.word = PrivateData_p->PCRSlotHandles_p[Status_p->SlotNumber];
    EventData_p->SlotHandle    = PrivateData_p->SlotHandles_p[Status_p->SlotNumber];

    STOS_SemaphoreSignal( stpti_MemoryLock );

    EventData_p->u.PCREventData.PCRArrivalTime          = *ArrivalTime_p;
    EventData_p->u.PCREventData.PCRArrivalTimeExtension = ArrivalTimeExtension;

    {
        EventData_p->u.PCREventData.PCRBase = *PCRTime_p;
        EventData_p->u.PCREventData.PCRExtension = Status_p->Pcr2 & 0x1ff;
        EventData_p->u.PCREventData.DiscontinuityFlag = (Status_p->Flags & STATUS_FLAGS_DISCONTINUITY_INDICATOR) != 0;
    }
#ifdef STPTI_DEBUG_SUPPORT
    ++DebugStatistics.PCRReceivedEventCount;
#endif

    /* This is a direct call to the PCR handler (Clock Recovery) avoid a task_deschedule. */
    /* This is done for performance reasons. */
    {
        Slot_t          *SlotStructure;
        FullHandle_t     FullSlotHandle;

        FullSlotHandle.word = EventData_p->SlotHandle; /* get the slot handle that the event occurred on */
        SlotStructure = stptiMemGet_Slot( FullSlotHandle );                       /* extract the SlotStructure for this slot */
        EventData_p->SlotHandle = SlotStructure->PCRReturnHandle;
#if defined(ST_OSLINUX)
        STEVT_NotifyWithSize(stptiMemGet_Device(FullDeviceHandle)->EventHandle,
                             stptiMemGet_Device(FullDeviceHandle)->EventID[STPTI_EVENT_PCR_RECEIVED_EVT - STPTI_EVENT_BASE],
                             (void *)EventData_p,
                             sizeof(STPTI_EventData_t));
#else
        STEVT_Notify(stptiMemGet_Device(FullDeviceHandle)->EventHandle,
                     stptiMemGet_Device(FullDeviceHandle)->EventID[STPTI_EVENT_PCR_RECEIVED_EVT - STPTI_EVENT_BASE],
                     (void *)EventData_p);
#endif
    }
}
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */


/******************************************************************************
Function Name : getSlotHandleForSlotBasedEvent
  Description :
   Parameters : const TCStatus_t *Status_p - pointer to the current status block.
******************************************************************************/
__INLINE static void stpti_getSlotHandleForSlotBasedEvent( STPTI_EventData_t *EventData_p,
                                                           FullHandle_t EventSlotHandle,
                                                           FullHandle_t FullDeviceHandle,
                                                           TCPrivateData_t *PrivateData_p,
                                                           const TCStatus_t  *Status_p,
                                                           BOOL *PacketDumped_p )
{
    EventData_p->SlotHandle = EventSlotHandle.word;

    /*
     * Note: For each error event flagged, disable the event after notifying STEVT, so
     * that it cannot occur again unless re-enabled by a call to STPTI_EnableErrorEvent().
     */

    if (Status_p->Flags & STATUS_FLAGS_SECTION_CRC_ERROR)
    {
#ifdef STPTI_DEBUG_SUPPORT
        ++DebugStatistics.SectionDiscardedOnCRCCheckEventCount;
#endif

        (void)stpti_DirectQueueEvent(FullDeviceHandle, STPTI_EVENT_SECTIONS_DISCARDED_ON_CRC_CHECK_EVT, EventData_p);

        STOS_SemaphoreWait( stpti_MemoryLock );
        stptiHAL_ErrorEventModifyState( FullDeviceHandle, STPTI_EVENT_SECTIONS_DISCARDED_ON_CRC_CHECK_EVT, FALSE );
        STOS_SemaphoreSignal( stpti_MemoryLock );
    }

#if !defined ( STPTI_BSL_SUPPORT )
    if (Status_p->Flags & STATUS_FLAGS_SCRAMBLE_CHANGE)
    {
        stpti_scrambleChange(FullDeviceHandle, Status_p, EventData_p, PrivateData_p);
    }

    if ( Status_p->Flags & STATUS_FLAGS_INVALID_DESCRAMBLE_KEY )
    {
        ST_ErrorCode_t Error = ST_NO_ERROR;

#ifdef STPTI_DEBUG_SUPPORT

        ++DebugStatistics.InvalidDescramblerKeyEventCount;
#endif

        Error = stpti_DirectQueueEvent(FullDeviceHandle, STPTI_EVENT_INVALID_DESCRAMBLE_KEY_EVT, EventData_p);

        STOS_SemaphoreWait( stpti_MemoryLock );
        Error |= stptiHAL_ErrorEventModifyState( FullDeviceHandle, STPTI_EVENT_INVALID_DESCRAMBLE_KEY_EVT, FALSE );
        STOS_SemaphoreSignal( stpti_MemoryLock );

        if( Error != ST_NO_ERROR )
        {
            STTBX_Print(( "Either unable to send or disable event: %s\n", "STPTI_EVENT_INVALID_DESCRAMBLE_KEY_EVT" ));
        }
    }
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */

    if ( Status_p->Flags & STATUS_FLAGS_INVALID_PARAMETER )
    {
        ST_ErrorCode_t Error = ST_NO_ERROR;

#ifdef STPTI_DEBUG_SUPPORT

        ++DebugStatistics.InvalidParameterEventCount;
#endif

        Error = stpti_DirectQueueEvent(FullDeviceHandle, STPTI_EVENT_INVALID_PARAMETER_EVT, EventData_p);

        STOS_SemaphoreWait( stpti_MemoryLock );
        Error |= stptiHAL_ErrorEventModifyState( FullDeviceHandle, STPTI_EVENT_INVALID_PARAMETER_EVT, FALSE );
        STOS_SemaphoreSignal( stpti_MemoryLock );

        if( Error != ST_NO_ERROR )
        {
            STTBX_Print(( "Either unable to send or disable event: %s\n", "STPTI_EVENT_INVALID_PARAMETER_EVT" ));
        }
    }

    if ( Status_p->Flags & STATUS_FLAGS_INVALID_LINK )
    {
        ST_ErrorCode_t Error = ST_NO_ERROR;

#ifdef STPTI_DEBUG_SUPPORT

        ++DebugStatistics.InvalidLinkEventCount;
#endif

        Error = stpti_DirectQueueEvent(FullDeviceHandle, STPTI_EVENT_INVALID_LINK_EVT, EventData_p);

        STOS_SemaphoreWait( stpti_MemoryLock );
        Error |= stptiHAL_ErrorEventModifyState( FullDeviceHandle, STPTI_EVENT_INVALID_LINK_EVT, FALSE );
        STOS_SemaphoreSignal( stpti_MemoryLock );

        if( Error != ST_NO_ERROR )
        {
            STTBX_Print(( "Either unable to send or disable event: %s\n", "STPTI_EVENT_INVALID_LINK_EVT" ));
        }
    }

    if ( Status_p->Flags & STATUS_FLAGS_INVALID_CC )
    {
        ST_ErrorCode_t Error = ST_NO_ERROR;

#ifdef STPTI_DEBUG_SUPPORT

        ++DebugStatistics.CCErrorEventCount;
#endif

        Error = stpti_DirectQueueEvent(FullDeviceHandle, STPTI_EVENT_CC_ERROR_EVT, EventData_p);

        STOS_SemaphoreWait( stpti_MemoryLock );
        Error |= stptiHAL_ErrorEventModifyState( FullDeviceHandle, STPTI_EVENT_CC_ERROR_EVT, FALSE );
        STOS_SemaphoreSignal( stpti_MemoryLock );

        if( Error != ST_NO_ERROR )
        {
            STTBX_Print(( "Either unable to send or disable event: %s\n", "STPTI_EVENT_CC_ERROR_EVT" ));
        }
    }

    /* MAS: 10/08/2004: Changes done for GNBvd28262 */
    if ( Status_p->Flags & STATUS_FLAGS_PES_ERROR )
    {
        ST_ErrorCode_t Error = ST_NO_ERROR;

#ifdef STPTI_DEBUG_SUPPORT

        ++DebugStatistics.PESErrorEventCount;
#endif

        Error = stpti_DirectQueueEvent(FullDeviceHandle, STPTI_EVENT_PES_ERROR_EVT, EventData_p);

        STOS_SemaphoreWait( stpti_MemoryLock );
        Error |= stptiHAL_ErrorEventModifyState( FullDeviceHandle, STPTI_EVENT_PES_ERROR_EVT, FALSE );
        STOS_SemaphoreSignal( stpti_MemoryLock );

        if( Error != ST_NO_ERROR )
        {
            STTBX_Print(( "Either unable to send or disable event: %s\n", "STPTI_EVENT_PES_ERROR_EVT" ));
        }
    }

    if ( Status_p->Flags & (STATUS_FLAGS_INVALID_DESCRAMBLE_KEY | STATUS_FLAGS_INVALID_PARAMETER |
                            STATUS_FLAGS_INVALID_CC | STATUS_FLAGS_INVALID_LINK | STATUS_FLAGS_PES_ERROR) )
    {
        *PacketDumped_p = TRUE;
    }
}

/******************************************************************************
Function Name : defineIndex
  Description :
   Parameters : const TCStatus_t *Status_p - pointer to the current status block.
******************************************************************************/
#ifndef STPTI_NO_INDEX_SUPPORT
__INLINE static void stpti_defineIndex( STPTI_EventData_t *EventData_p,
                                        FullHandle_t EventSlotHandle,
                                        FullHandle_t EventBufferHandle,
                                        const TCStatus_t *Status_p,
                                        Device_t *Device_p,
                                        const STPTI_TCParameters_t   *TC_Params_p,
                                        STPTI_TimeStamp_t *PCRTime_p,
                                        U16 PCRTimeExtension,
                                        STPTI_TimeStamp_t *ArrivalTime,
                                        U16 ArrivalTimeExtension,
                                        FullHandle_t FullDeviceHandle,
                                        BOOL PacketDumped,
                                        BOOL PacketSignalled )
{
    EventData_p->SlotHandle = EventSlotHandle.word; /* Indexing is Slot Based so return a SlotHandle */

    if ( Status_p->Flags & INDEX_FLAGS )
    {
        STPTI_IndexDefinition_t IndexDefinition;

        IndexDefinition.word = 0;
        {
            stpti_makeDVBIndexDefinition(&IndexDefinition, Status_p);
        }
        stpti_setScramblingStatusFlags( &IndexDefinition, Status_p);

        /* Adjust the packet count to index from zero not one */
        EventData_p->u.IndexEventData.PacketCount = Status_p->BufferPacketNumber -1;
        EventData_p->u.IndexEventData.RecordBufferPacketCount = Status_p->RecordBufferPacketNumber -1;
        EventData_p->u.IndexEventData.BufferPacketAddress = Status_p->BufferPacketAddress;         /* Supply the Packet's Address in the buffer */

        STOS_SemaphoreWait( stpti_MemoryLock );         /* lock memory to prevent buffer being dealloc'd underneath us */
        
#if defined(STPTI_ARCHITECTURE_PTI4_SECURE_LITE)
        /* Is this a start code event? */
        if( (IndexDefinition.s.MPEGStartCode==1) && (stpti_BufferHandleCheck(EventBufferHandle) == ST_NO_ERROR) )
        {
            U32 i;
            U8  StartCode, PacketSize;
            U8  *IPB_Frame_p;
            U16 IPB_Frame;
            U32 IPB_Frame_paddr;
            int OffsetFromStartOfTP, SizeOfTPHdr;
            U32 DeltaPkts;

            U16 DMAIdent = stptiMemGet_Buffer(EventBufferHandle)->TC_DMAIdent;
            TCDMAConfig_t *DMAConfig_p = &((TCDMAConfig_t *)  TC_Params_p->TC_DMAConfigStart)[DMAIdent];
            U32 Top_paddr   = (DMAConfig_p->DMATop_p & ~0xf) + 16;          /* top of active buffer (physical address) */
            U32 Base_paddr  = DMAConfig_p->DMABase_p;                       /* base of active buffer (physical address) */
            
            PacketSize = DVB_TS_PACKET_LENGTH;
            
            /* Work ou the TP header size (incl. AF) */
            SizeOfTPHdr = PacketSize-Status_p->PayloadLength;
            
            /* Update Event Data with Start Codes found */
            EventData_p->u.IndexEventData.NumberStartCodesDetected = Status_p->NumberStartCodes;

            for(i=0;i<Status_p->NumberStartCodes;i++)
            {
                StartCode = Status_p->StartCodes[i].Code;
                EventData_p->u.IndexEventData.MPEGStartCode[i].MPEGStartCodeValue = StartCode;
                
                /* Is it an MPEG2 Picture Start Code */
                if(StartCode==0x00)
                {
                    /* Status_p->StartCodes[i].Offset point to the 6th byte of the start code, and are the # of byte from the END of the TP packet */
                    /* e.g. 0 means last byte of tp, 184 means first byte after the tp header */
                    OffsetFromStartOfTP = (int)(PacketSize-1) - (int)(Status_p->StartCodes[i].Offset + 5);

                    /* Look up the byte referenced in case it is a frame start code */
                    IPB_Frame_paddr  = Status_p->BufferPacketAddress;                                   /* physical address of start of packet */
                    IPB_Frame_paddr += (int)(PacketSize-1) - (int)(Status_p->StartCodes[i].Offset);     /* add offset into packet */

                    /* need to deal with buffer wrapping */
                    if ( IPB_Frame_paddr >= Top_paddr ) IPB_Frame_paddr = (IPB_Frame_paddr-Top_paddr)+Base_paddr;

                    /* Convert to virtual non-cached address to find I,P,B Picture type field */
                    IPB_Frame_p = ((U8 *)stptiMemGet_Buffer(EventBufferHandle)->MappedStart_p) + (IPB_Frame_paddr - Base_paddr);
                    stpti_InvalidateRegion((void*) IPB_Frame_p, 4);      /* invalidate a word */

                    IPB_Frame = ((*IPB_Frame_p)>>3) & 0x07;         /* These 3 bits indicate what frame it is (whether it is an I, P or B frame) */
                    EventData_p->u.IndexEventData.MPEGStartCode[i].AuxillaryData = (U16) IPB_Frame;
                }
                else   /* some other start code than 0x00 */
                {
                    /* Status_p->StartCodes[i].Offset points to the 4th byte of the start code, and is the # of bytes from the END of the TP packet */
                    /* e.g. 0 means last byte of tp (offset 187 from the start), 184 means first byte after the tp header (offset 4 from the start) */
                    OffsetFromStartOfTP = (int)(PacketSize-1) - (int)(Status_p->StartCodes[i].Offset + 3);
                    EventData_p->u.IndexEventData.MPEGStartCode[i].AuxillaryData = 0;
                }
                
                if(OffsetFromStartOfTP >= SizeOfTPHdr)
                {
                    /* 1st byte of start code is in the same tp */
                    EventData_p->u.IndexEventData.MPEGStartCode[i].MPEGStartCodeOffsetInToTSPacket = (U8)OffsetFromStartOfTP;
                }
                else
                {
                    /* split start code - 1st byte of start code is in last tp */
                    
                    /* LIMITATION: This makes the assumption that the previous tp payload is large enough to hold a the 
                       remainder of the start code (upto 3 bytes or 5bytes for a MPEG2 picture code).  We also make the 
                       assumption that the previous packet for this pid is less than 256 packets away in the buffer.  This
                       last requirement arises from space issues in the status block, also in the TC start code filter. */
                    
                    EventData_p->u.IndexEventData.MPEGStartCode[i].MPEGStartCodeOffsetInToTSPacket = (U8) ((int)PacketSize+(OffsetFromStartOfTP-SizeOfTPHdr));
                    
                    /* only look at LSByte of BufferPacketNumber as tc only supplies that */
                    DeltaPkts=((U32)Status_p->BufferPacketNumber - (U32)Status_p->StartCodePreviousBPN) & 0x000000FF;
                    EventData_p->u.IndexEventData.PacketCount = Status_p->BufferPacketNumber - DeltaPkts;
                    
                    /* only look at LSByte of RecordBufferPacketNumber as tc only supplies that */
                    DeltaPkts=((U32)Status_p->RecordBufferPacketNumber - (U32)Status_p->StartCodePreviousRecordBPN) & 0x000000FF;
                    EventData_p->u.IndexEventData.RecordBufferPacketCount = Status_p->RecordBufferPacketNumber - DeltaPkts;
                    
                }
            }
        }
        
#else       /*  a non secure lite platform  */
        
        /* Is this a start code event? */
        if( (IndexDefinition.s.MPEGStartCode==1) && (stpti_BufferHandleCheck(EventBufferHandle) == ST_NO_ERROR) )
        {
            /* If Start Code Detection is not available (SCD only on PTI4SL) then report no start codes. */
            /* This is bombproofing as this line won't be run as SCD won't have been turned on! */
            EventData_p->u.IndexEventData.NumberStartCodesDetected = 0;
        }
        
#endif      /* STPTI_ARCHITECTURE_PTI4_SECURE_LITE */

        EventData_p->u.IndexEventData.IndexBitMap.word = IndexDefinition.word;

        if (stpti_BufferHandleCheck(EventBufferHandle) == ST_NO_ERROR)
        {
            if (stptiMemGet_Buffer(EventBufferHandle)->TC_DMAIdent < TC_Params_p->TC_NumberDMAs)    /* Check for valid DMA */
            {
                EventData_p->u.IndexEventData.OffsetIntoBuffer = Status_p->BufferPacketAddress;
                EventData_p->u.IndexEventData.BufferHandle = EventBufferHandle.word;

                /* Convert to virtual non-cached address */
                /* TC gives us address of byte rather than it's offset into buffer
                 * => must subtract base address of buffer from it to derive true offset
                 */
                {
                    U16 DMAIdent = stptiMemGet_Buffer(EventBufferHandle)->TC_DMAIdent;
                    TCDMAConfig_t *DMAConfig_p = &((TCDMAConfig_t *)  TC_Params_p->TC_DMAConfigStart)[DMAIdent];
                    U32 Base_paddr  = DMAConfig_p->DMABase_p;                       /* base of active buffer (physical address) */
                    EventData_p->u.IndexEventData.OffsetIntoBuffer = Status_p->BufferPacketAddress - Base_paddr;
                }
            }
        }

        EventData_p->u.IndexEventData.PacketArrivalTime = *ArrivalTime;
        EventData_p->u.IndexEventData.ArrivalTimeExtension = ArrivalTimeExtension;

        if (Status_p->Flags & STATUS_FLAGS_PCR_FLAG)
        {
            EventData_p->u.IndexEventData.PCRTime = *PCRTime_p;
            EventData_p->u.IndexEventData.PCRTimeExtension = PCRTimeExtension;
        }

        STOS_SemaphoreSignal(stpti_MemoryLock);         /* unlock memory ready for raising event (although it is a direct call) */

#ifdef STPTI_DEBUG_SUPPORT
        ++DebugStatistics.IndexMatchEventCount;
#endif

        (void)stpti_DirectQueueIndex(FullDeviceHandle, STPTI_EVENT_INDEX_MATCH_EVT, PacketDumped, PacketSignalled, EventData_p);
    }
}
#endif


#if !defined ( STPTI_BSL_SUPPORT )
/******************************************************************************
Function Name : stpti_processIndex
  Description :
   Parameters : const TCStatus_t *Status_p - pointer to the current status block.
******************************************************************************/
__INLINE static void stpti_processIndex( Device_t   *Device_p,
                                         TCSessionInfo_t   *TCSessionInfo_p,
                                         STPTI_TimeStamp_t *ArrivalTime_p,
                                         U16 *ArrivalTimeExtension_p,
                                         STPTI_TimeStamp_t *PCRTime_p,
                                         U16 *PCRTimeExtension_p,
                                         const TCStatus_t *Status_p )
{

    /* When the timestamp arrives from the TC its in the following formats

                    |                 Arrival Time 2                  |                  Arrive Time 1                  |                 Arrival Time 0                  |
      Source        |  x  x  x  x  x  x  x  x|  x  x  x  x  x  x  x  x|  x  x  x  x  x  x  x  x|  x  x  x  x  x  x  x  x|  x  x  x  x  x  x  x  x|  x  x  x  x  x  x  x  x|

      TSGDMA           6  5  4  3  2  1  0 s8| s7 s6 s5 s4 s3 s2 s1 s0| 22 21 20 19 18 17 16 15| 14 13 12 11 10  9  8  7|                   32 31| 30 29 28 27 26 25 24 23|
      PTI CLK       |  0  -  -  -  -  -  - s8| s7 s6 s5 s4 s3 s2 s1 s0| 16 15 14 13 12 11 10 09|  8  7  6  5  4  3  2  1| 32 31 30 29 28 27 26 25| 24 23 22 21 20 19 18 17|
      TSMERGER      |  6  5  4  3  2  1  0 s8| s7 s6 s5 s4 s3 s2 s1 s0| 22 21 20 19 18 17 16 15| 14 13 12 11 10  9  8  7|  -  -  -  -  -  - 32 31| 30 29 28 27 26 25 24 23|

      bits prefixed with s are the 0-299 300 27MHz ticks Other Ticks hence are incremented at 90KHz  */


    /* At least one Index event has arrived... so calculate packet arrival time and PCR if appropriate */

    stptiHAL_STCToTimestamp(Device_p, Status_p->ArrivalTime0, Status_p->ArrivalTime1,
                            Status_p->ArrivalTime2, ArrivalTime_p, ArrivalTimeExtension_p);

    if ( Status_p->Flags & STATUS_FLAGS_PCR_FLAG )
    {
        stpti_recoverPCR(Status_p, PCRTime_p, PCRTimeExtension_p);
    }
}


#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */



/******************************************************************************
Function Name : processStatusBlock
  Description :
   Parameters :
******************************************************************************/
__INLINE static void stpti_processStatusBlock( U32 *BytesInBuffer,
                                               U32 *ReadPtr_p,
                                               U32 WritePtr,
                                               U32 TopPtr,
                                               U32 BasePtr,
                                               U32 BasePtr_physical,
                                               TCPrivateData_t *PrivateData_p,
                                               const STPTI_TCParameters_t *TC_Params_p,
                                               STPTI_EventData_t *EventData_p,
                                               TCInterruptDMAConfig_t *TCInterruptDMAConfig_p,
                                               TCSessionInfo_t *TCSessionInfo_p,
                                               U32 *DMANumber_p,
                                               TCStatus_t *Status_p,
                                               FullHandle_t *EventBufferHandle_p )
{
    FullHandle_t    SlotHandle;
    Device_t       *Device_p = NULL;
    FullHandle_t    FullDeviceHandle;
    U32             ReadBufferIndex = *ReadPtr_p - BasePtr;
    FullHandle_t    EventSlotHandle, EventBufferHandle, EventRecordBufferHandle;
    U32             SessionNumber;

    EventBufferHandle.word = STPTI_NullHandle();
    EventRecordBufferHandle.word = STPTI_NullHandle();

    if (WritePtr > *ReadPtr_p)
    {
#if !defined(ARCHITECTURE_ST20) && !defined( ARCHITECTURE_ST200 )
        stpti_InvalidateRegion((void*) *ReadPtr_p, sizeof (TCStatus_t));
#endif
        memcpy( (void*)Status_p, (void*) *ReadPtr_p, sizeof (TCStatus_t) );
    }
    else
    {
        if ((TopPtr - *ReadPtr_p) >= sizeof (TCStatus_t))
        {
#if !defined(ARCHITECTURE_ST20) && !defined( ARCHITECTURE_ST200 )
            stpti_InvalidateRegion((void*) *ReadPtr_p, sizeof (TCStatus_t));
#endif
            memcpy( (void*)Status_p, (void*) *ReadPtr_p, sizeof (TCStatus_t) );
        }
        else
        {
#if !defined(ARCHITECTURE_ST20) && !defined( ARCHITECTURE_ST200 )
            stpti_InvalidateRegion((void*) *ReadPtr_p, (TopPtr - *ReadPtr_p));
            stpti_InvalidateRegion((void*) BasePtr, sizeof( TCStatus_t ) - (TopPtr - *ReadPtr_p));
#endif
            memcpy( (void*)Status_p, (void*) *ReadPtr_p, (TopPtr - *ReadPtr_p) );
            memcpy( (void*)&((U8 *) Status_p)[(TopPtr - *ReadPtr_p)],
                                 (void*)BasePtr,
                                 sizeof( TCStatus_t ) - (TopPtr - *ReadPtr_p));
        }
    }

    /* Copying done.... advance ReadPtr_p */
    ReadBufferIndex = (ReadBufferIndex + sizeof (TCStatus_t)) % (TopPtr - BasePtr);
    *ReadPtr_p = (U32) &(((U8 *) BasePtr)[ReadBufferIndex]);

    /* Update the tc's copy of the readpointer as we have finished with that part of the status buffer */
    STSYS_WriteRegDev32LE((void*)&TCInterruptDMAConfig_p->DMARead_p, BasePtr_physical + (*ReadPtr_p - BasePtr));

    if ( Status_p->Flags == 0 )
    {
#if defined( ST_OSLINUX )
        printk(KERN_ALERT"pti_core: INVALID STATUS BLOCK (present but no cause).\n");
#else
        STTBX_Print(("STATUS BLOCK but no cause?\n"));
#endif /* #enfif defined( ST_OSLINUX ) */
        return;
    }

    STTBX_Print(("STATUS BLOCK 0x%08x\n", Status_p->Flags));

    /* Safe to access PrivateData, since we are only reading a single U32 */

    /* Is slot invalid (could be true for a purely carousel related event) */
    if ( Status_p->SlotNumber == 0xFF )
    {
        EventSlotHandle.word = STPTI_NullHandle();
    }
    else
    {
        EventSlotHandle.word = PrivateData_p->SlotHandles_p[Status_p->SlotNumber];
        STPTI_GetBuffersFromSlot((STPTI_Slot_t)EventSlotHandle.word, (STPTI_Buffer_t *)&EventBufferHandle.word,
                                  (STPTI_Buffer_t *)&EventRecordBufferHandle.word);
    }

    /* Is dma invalid (could be true for a purely slot related event) */
    if ( (Status_p->DMACtrl == 0) || (Status_p->DMACtrl == 0xf000) )
    {
        *DMANumber_p = 0xFFFFFFFF;
        EventBufferHandle.word = STPTI_NullHandle();
    }
    else
    {
        /* Get the DMA Index of the conventional buffer - a record DMA Index may also be present  for watch and record */
        *DMANumber_p = (Status_p->DMACtrl - ((U8 *) TC_Params_p->TC_DMAConfigStart - (U8 *) TC_Params_p->TC_DataStart + 0x8000)) / sizeof( TCDMAConfig_t );
        EventBufferHandle.word = PrivateData_p->BufferHandles_p[*DMANumber_p];
    }
    
    if((Status_p->Flags & INDEX_FLAGS) && (EventRecordBufferHandle.word != STPTI_NullHandle()))
    {
        EventBufferHandle_p->word = EventRecordBufferHandle.word;
    }
    else
    {
        EventBufferHandle_p->word = EventBufferHandle.word;
    }

#if !defined ( STPTI_BSL_SUPPORT )
    /* Get the Device pointer used later to work out which virtual device the event is related to */
    if (stpti_SlotHandleCheck(EventSlotHandle) == ST_NO_ERROR)
    {
        Device_p         = stptiMemGet_Device(EventSlotHandle);
    }
    else if ((stpti_BufferHandleCheck(EventBufferHandle) == ST_NO_ERROR) ||
             ((Status_p->Flags & INDEX_FLAGS) && (stpti_BufferHandleCheck(EventRecordBufferHandle) == ST_NO_ERROR)))
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */
    {
        Device_p         = stptiMemGet_Device(*EventBufferHandle_p);
    }
#if !defined ( STPTI_BSL_SUPPORT )
    else
    {
        STTBX_Print(("Both Slot 0x%08x and Buffer 0x%08x handles are invalid.\n", EventSlotHandle, EventBufferHandle));
        /* This sometimes happens when the PTI has been term'ed whilst the stream is going thru. We can safely ignore these. */
        return;
    }
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */

    FullDeviceHandle     = Device_p->Handle;

    /* Now find the session no. from the Device_p rather then from the StatusBlock*/
    SessionNumber        = Device_p->Session;

    TCSessionInfo_p      = &TCSessionInfo_p[SessionNumber];

#if !defined ( STPTI_BSL_SUPPORT )


#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */



    if ( (Status_p->Flags & STATUS_FLAGS_BUFFER_OVERFLOW) || (Status_p->Flags & STATUS_FLAGS_RECORD_BUFFER_OVERFLOW) )
    {
#ifdef STPTI_DEBUG_SUPPORT
        ++DebugStatistics.BufferOverflowEventCount;
#endif
        EventData_p->SlotHandle = PrivateData_p->SlotHandles_p[Status_p->SlotNumber];
        (void)stpti_DirectQueueEvent(FullDeviceHandle, STPTI_EVENT_BUFFER_OVERFLOW_EVT, EventData_p);

        STOS_SemaphoreWait( stpti_MemoryLock );
        stptiHAL_ErrorEventModifyState( FullDeviceHandle, STPTI_EVENT_BUFFER_OVERFLOW_EVT, FALSE );
        STOS_SemaphoreSignal( stpti_MemoryLock );
    }

/*
    else
           Removed to allow all events to be seen even when overflow occurs on output buffer GNBvd56312
*/
    {
#if !defined ( STPTI_BSL_SUPPORT )
        STPTI_TimeStamp_t PCRTime;
        U16               PCRTimeExtension=0;
        STPTI_TimeStamp_t ArrivalTime;
        U16               ArrivalTimeExtension=0;
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */
        BOOL              PacketDumped    = FALSE;
        BOOL              PacketSignalled = FALSE;

#if !defined ( STPTI_BSL_SUPPORT )
#if defined( STPTI_CAROUSEL_SUPPORT )
        if ( ( Status_p->Flags & INDEX_FLAGS ) || ( Status_p->Flags & STATUS_FLAGS_SUBSTITUTE_COMPLETE ) )
#else
        if ( Status_p->Flags & INDEX_FLAGS )
#endif
        {
            stpti_processIndex( Device_p,
                                TCSessionInfo_p,
                                &ArrivalTime,
                                &ArrivalTimeExtension,
                                &PCRTime,
                                &PCRTimeExtension,
                                Status_p );
        }
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */

#ifdef STPTI_CAROUSEL_SUPPORT
        /* Substitute complete interrupt */
        if ( Status_p->Flags & STATUS_FLAGS_SUBSTITUTE_COMPLETE )
        {
            stpti_signalCarouselEntryArrival(Device_p, Status_p, &ArrivalTime);
        }
#endif

#ifndef STPTI_NO_INDEX_SUPPORT
        if ( Status_p->Flags & INDEX_FLAGS )
        {
            /* This directly raises any event (bypass event task) as event callback may could need */
            /* information removed when we notify the packet signal.  i.e we won't continue here   */
            /* until event callback function has finished (as it is done by this task). */

            stpti_defineIndex( EventData_p,
                               EventSlotHandle,
                               *EventBufferHandle_p,
                               Status_p,
                               Device_p,
                               TC_Params_p,
                               &PCRTime,
                               PCRTimeExtension,
                               &ArrivalTime,
                               ArrivalTimeExtension,
                               FullDeviceHandle,
                               PacketDumped,
                               PacketSignalled );
        }
#endif

        /* Buffer Based events... return a SlotHandle */
        if ( Status_p->Flags & STATUS_FLAGS_PACKET_SIGNAL )
        {
            stpti_processPacketSignal( *DMANumber_p,
                                       TC_Params_p,
                                       &SlotHandle,
                                       EventData_p,
                                       PrivateData_p,
                                       FullDeviceHandle,
                                       *BytesInBuffer,
                                       EventSlotHandle,
                                       Status_p,
                                       &PacketSignalled );
        }

        if ( ( Device_p->TCCodes == STPTI_SUPPORTED_TCCODES_SUPPORTS_DVB )
           )
        {
            /* Record Buffer Based events... return a SlotHandle */
            if ( Status_p->Flags & STATUS_FLAGS_PACKET_SIGNAL_RECORD_BUFFER )
            {
                
                if( stpti_BufferHandleCheck(EventRecordBufferHandle) == ST_NO_ERROR )
                {
                    stpti_processPacketSignal( (stptiMemGet_Buffer(EventRecordBufferHandle))->TC_DMAIdent,    /* Use DMA Index for record buffer */
                                               TC_Params_p,
                                               &SlotHandle,
                                               EventData_p,
                                               PrivateData_p,
                                               FullDeviceHandle,
                                               *BytesInBuffer,
                                               EventSlotHandle,
                                               Status_p,
                                               &PacketSignalled );
                }
            }
        }
#if !defined ( STPTI_BSL_SUPPORT )
#ifndef STPTI_LOADER_SUPPORT
        /* Device Based events... return a NullHandle */
        if ( Status_p->Flags & STATUS_FLAGS_TC_CODE_FAULT )
        {
            stpti_signalTCCodeFault( EventData_p,
                                     FullDeviceHandle,
                                     Status_p,
                                     &PacketDumped);
        }
#endif

        /* PCR Based events... return a PCR associated SlotHandle */
        if ( Status_p->Flags & STATUS_FLAGS_PCR_FLAG )
        {
            stpti_getSlotHandleForPCRBasedEvent( EventData_p,
                                                 Status_p,
                                                 &ArrivalTime,
                                                 ArrivalTimeExtension,
                                                 Device_p,
                                                 PrivateData_p,
                                                 FullDeviceHandle,
                                                 &PCRTime );
        }
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */

        /* Slot Based events... return a SlotHandle */
        if ( Status_p->Flags & (STATUS_FLAGS_SECTION_CRC_ERROR |
#if !defined ( STPTI_BSL_SUPPORT )
                                STATUS_FLAGS_SCRAMBLE_CHANGE |
                                STATUS_FLAGS_INVALID_DESCRAMBLE_KEY |
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */
                                STATUS_FLAGS_INVALID_PARAMETER |
                                STATUS_FLAGS_INVALID_CC |
                                STATUS_FLAGS_INVALID_LINK |
                                STATUS_FLAGS_PES_ERROR) )
        {
            stpti_getSlotHandleForSlotBasedEvent( EventData_p,
                                                  EventSlotHandle,
                                                  FullDeviceHandle,
                                                  PrivateData_p,
                                                  Status_p,
                                                  &PacketDumped );
        }

    }

}

/******************************************************************************
Function Name : addInterruptDebugEntry
  Description :
   Parameters : const TCStatus_t *Status_p - pointer to the current status block.
******************************************************************************/
#ifdef STPTI_DEBUG_SUPPORT

__INLINE static void stpti_addInterruptDebugEntry( U32 DMANumber,
                                                   TCDevice_t *TC_Device_p,
                                                   U32 InterruptStatus,
                                                   TCPrivateData_t *PrivateData_p,
                                                   const TCStatus_t *Status_p,
                                                   FullHandle_t EventBufferHandle )
{
    /* Safe to access PrivateData, since we are only reading a single value */

    DebugInterruptStatus[IntInfoIndex].Status          = InterruptStatus;
    DebugInterruptStatus[IntInfoIndex].SlotHandle      = PrivateData_p->SlotHandles_p[Status_p->SlotNumber];
    DebugInterruptStatus[IntInfoIndex].BufferHandle    = EventBufferHandle.word;
    DebugInterruptStatus[IntInfoIndex].PTIBaseAddress  = (U32) TC_Device_p;
    DebugInterruptStatus[IntInfoIndex].InterruptCount  = stpti_InterruptTotalCount;
    DebugInterruptStatus[IntInfoIndex].DMANumber       = DMANumber;

    IntInfoIndex = (IntInfoIndex+1 ) % IntInfoCapacity; /* increment index to debug array */
}
#endif

/******************************************************************************
Function Name : stpti_InterruptTask
  Description :
   Parameters :
******************************************************************************/
#if defined( ST_OSLINUX )
static int stpti_InterruptTask( void *arg )
{
    BackendInterruptData_t BackendInterruptData;
#else
__INLINE static void stpti_InterruptTask( void )
{
#endif /* #endif..else defined( ST_OSLINUX ) */

    BackendInterruptData_t *BackendInterruptData_p;

#if defined( ST_OSLINUX_INT_DEBUG )
    printk(KERN_ALERT"pticore: Interrupt task started\n");
#endif

    while ( TRUE )
    {
        stpti_InterruptTaskCount++;
        /* TODO
         * Interrupt lock around receive ?
         * May be safe anyway...
         */
#if defined( ST_OSLINUX )
        BackendInterruptData_p = stptiHAL_WaitForInterrupt( &BackendInterruptData );
        if (BackendInterruptData_p->InterruptStatus == 0)
        {
            /* This situation can arise if we overflowed our "message queue" */
            /* The event will have been raised in stptiHAL_WaitForInterrupt() so we don't need */
            /* to do anything here. */
            continue;
        }
#else
        BackendInterruptData_p = STOS_MessageQueueReceiveTimeout(STPTI_Driver.InterruptQueue_p, TIMEOUT_INFINITY);
#endif /* #endif defined( ST_OSLINUX ) */

        if ( (BackendInterruptData_p->FullHandle.word) == TASK_QUIT_PATTERN )
        {
#if !defined( ST_OSLINUX )
            STOS_MessageQueueRelease(STPTI_Driver.InterruptQueue_p, BackendInterruptData_p);
#endif /* #endif !defined( ST_OSLINUX ) */
            break;
        }

        {
            Device_t               *Device_p;
            TCDevice_t             *TC_Device_p;
            TCPrivateData_t        *PrivateData_p;
            STPTI_TCParameters_t   *TC_Params_p;
            TCSessionInfo_t        *TCSessionInfo_p;
            TCInterruptDMAConfig_t *TCInterruptDMAConfig_p;
            U32                     InterruptStatus;
            FullHandle_t            FullDeviceHandle;
            FullHandle_t            EventBufferHandle;
            STPTI_EventData_t       EventData;
            TCStatus_t              Status = { 0 };
            U32 DMANumber = 0;

#ifdef STPTI_DEBUG_SUPPORT
            /*
             * initialise all of the current structure to 0xffffffff in case an element
             * is not set in the isr handler
             */
            if ( IntInfoStart == TRUE )
            {
                memset (&(DebugInterruptStatus[IntInfoIndex]),
                        0xff,
                        sizeof (STPTI_DebugInterruptStatus_t) );
                DebugInterruptStatus[IntInfoIndex].Time = (U32) time_now();
            }
#endif

            /* --------------------------------------------------------- */

            FullDeviceHandle.word = BackendInterruptData_p->FullHandle.word;
            InterruptStatus  = BackendInterruptData_p->InterruptStatus;

#if !defined(ST_OSLINUX)
            STOS_MessageQueueRelease( STPTI_Driver.InterruptQueue_p, BackendInterruptData_p );
#endif /* #endif !defined(ST_OSLINUX) */
            STOS_SemaphoreWait( stpti_MemoryLock );

            Device_p = stptiMemGet_Device(FullDeviceHandle);

            TC_Device_p = (TCDevice_t *)  Device_p->TCDeviceAddress_p;
            PrivateData_p = (TCPrivateData_t *) &Device_p->TCPrivateData;
            TC_Params_p = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
            TCSessionInfo_p = (TCSessionInfo_t *)  TC_Params_p->TC_SessionDataStart;

            STOS_SemaphoreSignal( stpti_MemoryLock );

            TCInterruptDMAConfig_p = (TCInterruptDMAConfig_t *)  TC_Params_p->TC_InterruptDMAConfigStart;

            /* --------------------------------------------------------- */

            /* Undefined interrupt type - so it shouldn't happen */
            if ( !(InterruptStatus & INTERRUPT_STATUS_VALID) )
            {
                stpti_sendInvalidInterruptMessage( FullDeviceHandle );
            }
            else
            {
                if ( InterruptStatus & INTERRUPT_STATUS_INTERRUPT_FAILED )
                {
                    stpti_interruptFailed( FullDeviceHandle );
                }
                
                if ( InterruptStatus & INTERRUPT_STATUS_STATUS_BLOCK_ARRIVED )
                {
                    /*
                     * Status block processing - this can be an else as if the status
                     * buffer has overflowed we dont want the status block
                     */

                    U32 ReadPtr_physical  =  STSYS_ReadRegDev32LE( (void*)&TCInterruptDMAConfig_p->DMARead_p );
                    U32 WritePtr_physical =  STSYS_ReadRegDev32LE( (void*)&TCInterruptDMAConfig_p->DMAWrite_p );
                    U32 TopPtr_physical   =  (STSYS_ReadRegDev32LE( (void*)&TCInterruptDMAConfig_p->DMATop_p ) & ~0xf) + 16;
                    U32 BasePtr_physical  =  STSYS_ReadRegDev32LE( (void*)&TCInterruptDMAConfig_p->DMABase_p );

                    U32 ReadPtr, WritePtr, TopPtr, BasePtr, BytesInBuffer;

                    /* Convert to virtual non-cached address */
                    ReadPtr = (U32) PrivateData_p->InterruptBufferStart_p + (ReadPtr_physical - BasePtr_physical);
                    WritePtr = (U32) PrivateData_p->InterruptBufferStart_p + (WritePtr_physical - BasePtr_physical);
                    TopPtr = (U32) PrivateData_p->InterruptBufferStart_p + (TopPtr_physical - BasePtr_physical);
                    BasePtr = (U32) PrivateData_p->InterruptBufferStart_p;

                    BytesInBuffer = (WritePtr >= ReadPtr) ? WritePtr - ReadPtr
                                        : (TopPtr - ReadPtr) + (WritePtr - BasePtr);


#if defined (ST_OSLINUX_INT_DEBUG)
printk(KERN_ALERT"pticore: InterruptStatus & INTERRUPT_STATUS_STATUS_BLOCK_ARRIVED\n");
printk(KERN_ALERT"ReadPtr %x\n",ReadPtr);
printk(KERN_ALERT"WritePtr %x\n",WritePtr);
printk(KERN_ALERT"TopPtr %x\n",TopPtr);
printk(KERN_ALERT"BasePtr %x\n",BasePtr);
printk(KERN_ALERT"BytesInBuffer %x\n",BytesInBuffer);
#endif /* ST_OSLINUX_INT_DEBUG */

                    /*
                     *  The while() loop ensures that all complete status blocks
                     * are consumed but could mean in the case of this tasks
                     * priority being too low or a very high rate of arrival
                     * for the status blocks that we eat up lots of the CPU
                     */
                    while ( BytesInBuffer >= sizeof( TCStatus_t ) ) /* Read the status block from the buffer */
                    {
                        stpti_processStatusBlock( &BytesInBuffer,
                                                  &ReadPtr,
                                                  WritePtr,
                                                  TopPtr,
                                                  BasePtr,
                                                  BasePtr_physical,
                                                  PrivateData_p,
                                                  TC_Params_p,
                                                  &EventData,
                                                  TCInterruptDMAConfig_p,
                                                  TCSessionInfo_p,
                                                  &DMANumber,
                                                  &Status,
                                                  &EventBufferHandle );

                        /* Update the bytes in buffer */
                        WritePtr_physical = STSYS_ReadRegDev32LE( (void*)&TCInterruptDMAConfig_p->DMAWrite_p );
                        /* Internally addresses are virtual */
                        WritePtr = (U32)PrivateData_p->InterruptBufferStart_p + (WritePtr_physical-BasePtr_physical);
                        BytesInBuffer = (WritePtr >= ReadPtr) ? WritePtr - ReadPtr
                                       : (TopPtr - ReadPtr) + (WritePtr - BasePtr);

                    }
#if !defined ( STPTI_BSL_SUPPORT )
                    /*
                     * Just check that we haven't got any strange number of bytes left in the buffer.
                     * Remember we've just read all of the status block sized data - buffer should
                     * be empty.  If it isn't empty then we probably have an incorrect version of
                     * TC code loaded
                     */
                    if (BytesInBuffer != 0)
                    {
                        /* Clear the buffer */
                        STSYS_WriteRegDev32LE((void*)&TCInterruptDMAConfig_p->DMARead_p,
                                                STSYS_ReadRegDev32LE((void*)&TCInterruptDMAConfig_p->DMAWrite_p));
                        /* Warn the system */
                        stpti_statusBufferCorruption( FullDeviceHandle );
                    }
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */
                }   /* if (InterruptStatus & INTERRUPT_STATUS_STATUS_BLOCK_ARRIVED) this cannot be an 'else if' as
                                      several bits can be set at once in the InterruptStatus */
                /* --------------------------------------------------------- */

                if (InterruptStatus & INTERRUPT_STATUS_PACKET_ERROR_INTERRUPT )
                {
#ifdef STPTI_DEBUG_SUPPORT
                    /*
                     * Remember ISR task now disables this interrupt after its first occurrance
                     * - a count of 1 may still mean a serious problem exists
                     */
                    ++DebugStatistics.PacketErrorEventCount;
#endif

                    (void)stpti_DirectQueueEvent(FullDeviceHandle, STPTI_EVENT_PACKET_ERROR_EVT, &EventData);
                }

            }

            /* --------------------------------------------------------- */

#ifdef STPTI_DEBUG_SUPPORT
            if ( IntInfoStart == TRUE )
            {
                stpti_addInterruptDebugEntry(DMANumber,
                                       TC_Device_p,
                                       InterruptStatus,
                                       PrivateData_p,
                                       &Status,
                                       EventBufferHandle);
            }
#endif

        }

    }


#if defined(ST_OSLINUX)
#if defined(ST_OSLINUX_INT_DEBUG)
    printk(KERN_ALERT"pticore: Interrupt task terminated\n");
#endif
    complete_and_exit(&pticore_inttask_exited ,0);

    return 0;
#endif /* #endif defined(ST_OSLINUX) */
}


/* ------------------------------------------------------------------------- */


/* --- EOF --- */
