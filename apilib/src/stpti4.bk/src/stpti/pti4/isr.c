/******************************************************************************
COPYRIGHT (C) STMicroelectronics 2005.  All rights reserved.

   File Name: isr.c
 Description: pti4 cell interrupt handler.

******************************************************************************/

#if !defined(ST_OSLINUX)
#include <string.h>
#include <stdio.h>
#endif /* #if !defined(ST_OSLINUX) */

#include "stddefs.h"
#include "stdevice.h"
#if !defined(ST_OSLINUX)
#include "sttbx.h"
#endif /* #if !defined(ST_OSLINUX) */
#include "stpti.h"

#include "pti_evt.h"
#include "pti_loc.h"
#include "pti_hndl.h"
#include "pti_hal.h"

#include "memget.h"

#include "cam.h"
#include "tchal.h"

#include "pti4.h"
#include "isr.h"

#if defined(ST_OSLINUX)
static BOOL exitflag = FALSE;
#endif
/* ------------------------------------------------------------------------- */

/* defined in pti_ba.c */
extern volatile U32 stpti_InterruptTotalCount;

#ifdef STPTI_DEBUG_SUPPORT
extern STPTI_DebugStatistics_t DebugStatistics;
/* report every error */
#define MESSAGE_QUEUE_SUCCESS_THRESHOLD 0
#else
/* require 3 consecutive successful message claims berfore reporting a failure */
#define MESSAGE_QUEUE_SUCCESS_THRESHOLD 3
#endif


/* Support Interrupts from TSMERGER.  They are ORed with the PTI interrupt on
   5100 and 5301 as a quick hardware fix. */
#if ( defined(ST_5100) || defined(ST_5301) )
#define STMERGE_PTI_INTERRUPT_SUPPORT
#endif

/*
 * Prevent message claim failures from causing an avalanche of error events 
 * which kill the box. Message claim fail events will only be generated ONCE
 * before being disabled. Only after MESSAGE_QUEUE_SUCCESS_THRESHOLD successful
 * claims have happened in a row is the ISR free to report an error
 */
/* initialise system so that initial error will be reported */
static U32 ISR_MessageQueueClaimSuccessCount = MESSAGE_QUEUE_SUCCESS_THRESHOLD;

/*
 * Only report the first invalid interrupt that we can report - don't disable
 * reporting unless we have managed to report the error
 */
static BOOL ReportInvalidInterrupts = TRUE;

#if defined(ST_OSLINUX)
DECLARE_WAIT_QUEUE_HEAD(isr_wq);

InterruptCtl_t InterruptCtl;

/******************************************************************************
Function Name : stptiHAL_InitInterruptCtl
  Description : Initialise the Interrupt control data.
   Parameters : NONE.
******************************************************************************/
void stptiHAL_InitInterruptCtl()
{
    InterruptCtl.WriteIndex = 0;
    InterruptCtl.ReadIndex = 0;
}

/******************************************************************************
Function Name : AddIntStatus
  Description : In lined by the ISR. Adds interrupt on to list that is obtained by
                stptiHAL_WaitForInterrupt.
   Parameters :     U32 EventStatus;     0 - Status has interrupt status
                         STPTI_EVENT_INTERRUPT_FAIL_EVT - Buffer overflowed 
                         STPTI_EVENT_DMA_COMPLETE_EVT   - Status = DMA destination.
******************************************************************************/
inline static int AddIntStatus( __u32 DeviceHandle, __u32 EventStatus, __u32 Status )
{
    InterruptData_t *InterruptData_p = InterruptCtl.InterruptData + InterruptCtl.WriteIndex;
    U32 temp = InterruptCtl.WriteIndex + 1;
    
    InterruptData_p->DeviceHandle = DeviceHandle;
    InterruptData_p->EventStatus = EventStatus; /* Status = InterruptStatus */
    InterruptData_p->Status = Status;


    temp %= INTERRUPT_DATA_RING_SIZE;

    /* Check for over flow */
    if( temp == InterruptCtl.ReadIndex ){
        /* Over write last status */
        InterruptData_p->EventStatus = STPTI_EVENT_INTERRUPT_FAIL_EVT;
        ISR_MessageQueueClaimSuccessCount = 0;
        return -1;
    }
    else{
        InterruptCtl.WriteIndex = temp;/* Only move write on if succeeds. */
        ++ISR_MessageQueueClaimSuccessCount;
    }

    wake_up_interruptible( &isr_wq );
    return 0; /* Good */
}

/******************************************************************************
Function Name : stptiHAL_WaitForInterrupt
  Description : Called from interrupt task toi replace message_cleam_timeout
   Parameters : Passed a pointer to a BackendInterruptData_t structure that
                is returned to maintain compatability.
******************************************************************************/
BackendInterruptData_t *stptiHAL_WaitForInterrupt( BackendInterruptData_t *BackendInterruptData_p )
{
    InterruptData_t *InterruptData_p;
    
    wait_event_interruptible( isr_wq, (exitflag != FALSE) || (InterruptCtl.ReadIndex != InterruptCtl.WriteIndex) );

    InterruptData_p = InterruptCtl.InterruptData + InterruptCtl.ReadIndex;
    InterruptCtl.ReadIndex  = (InterruptCtl.ReadIndex + 1) % INTERRUPT_DATA_RING_SIZE ;
    
    if( exitflag == TRUE ){
        BackendInterruptData_p->FullHandle.word = TASK_QUIT_PATTERN;
        exitflag = FALSE;
    }
    else{
        switch( InterruptData_p->EventStatus ){
        case STPTI_EVENT_INTERRUPT_FAIL_EVT:
            {                
                /* We previously succeeded enough times when we claimed a message => tell someone of this failure */
                STPTI_EventData_t EventData;
                FullHandle_t            FullDeviceHandle;
                FullDeviceHandle.word = InterruptData_p->DeviceHandle;
                
                EventData.u.ErrEventData.DMANumber      = 0XFACEBEEF;
                EventData.u.ErrEventData.BytesRemaining = 0;
                
                (void)stpti_QueueEvent(FullDeviceHandle, STPTI_EVENT_INTERRUPT_FAIL_EVT, &EventData);
            }
            break;
        case STPTI_EVENT_DMA_COMPLETE_EVT:
            {
                STPTI_EventData_t       EventData;
                FullHandle_t            FullDeviceHandle;
                FullDeviceHandle.word = InterruptData_p->DeviceHandle;
                
                EventData.u.DMAEventData.Destination = InterruptData_p->Status;
                stpti_DirectQueueEvent(FullDeviceHandle, STPTI_EVENT_DMA_COMPLETE_EVT, &EventData);
            }
            break;
        case 0:
            BackendInterruptData_p->FullHandle.word = InterruptData_p->DeviceHandle;
            BackendInterruptData_p->InterruptStatus = InterruptData_p->Status;
            break;
        default:
            break;
        }
    }

    return( BackendInterruptData_p );
}

void stptiHAL_StopWaiting()
{
    exitflag = TRUE;
    wake_up_interruptible( &isr_wq );
}

#endif

/******************************************************************************
Function Name : stptiHAL_InterruptHandler
  Description : stpti_TCInterruptHandler
   Parameters :
******************************************************************************/
STOS_INTERRUPT_DECLARE(stptiHAL_InterruptHandler, DeviceHandle)
{
    FullHandle_t FullDeviceHandle;
    U32 InterruptStatus;
    Device_t *Device_p;
    TCDevice_t *TC_Device_p;
    
    /* This is to mask the current interrupt if it is a TSMerger interrupt
       otherwise we use the original global variable */
    BOOL FoundTSMERGERInterrupt = FALSE; 
    
    FullDeviceHandle.word = (STPTI_Handle_t) DeviceHandle;
    Device_p = stptiMemGet_Device(FullDeviceHandle);
    TC_Device_p = (TCDevice_t *)  Device_p->TCDeviceAddress_p;    
    
    InterruptStatus = STSYS_ReadRegDev32LE((void*)&TC_Device_p->PTIIntStatus0);

    
#if defined(STMERGE_PTI_INTERRUPT_SUPPORT) 
    /* --------------- Check for STMERGER Interrupts -------------- */

    /*  On 5100 and 5301 the TSMERGER interrupt is ORed into the PTI interrupt (a last minute
        h/w fix).  This means that we must service this interrupt here.  We don`t use the 
        TSMERGER driver API to keep the drivers independent, which unfortunately makes this  
        code a little cryptic.                                                                            
        
        The TSMERGER interrupt fires when there is a tsmerger ram overflow.  The recovery
        from this is to find out which channel caused it, clear the overflow bit for that
        channel, turn off the channel and turn it on again.                                   */

    #define TSMERGER_CONFIG_OFFSET           0     /* CONFIG register is first register 0 byte offset */
    #define TSMERGER_STREAM_ON_BIT           0x80  /* bit 7 is STREAM ON */

    #define TSMERGER_STATUS_OFFSET           4     /* STATUS register at 0x10 byte offset, expressed as 4 words */
    #define TSMERGER_RAM_OVERFLOW_BIT        0x04  /* bit 2 is RAM OVERFLOW */
    #define TSMERGER_SW_RESET                0x3F8  /* SW reset offset in bytes */

    #define NUMBER_OF_CHANNELS               5     /* Total number of Channels 0,1,2,3,4 */
    #define TSMERGER_STREAM_REGISTERS_OFFSET 8     /* Size of the Block of TSMERGER Registers for a Channel */
    
    {
        volatile U32 *TSMERGER_ChannelBase_p = (U32 *)TSMERGE_BASE_ADDRESS;
        volatile U32 *TSMERGER_ResetBase_p = (U32 *)(TSMERGE_BASE_ADDRESS + TSMERGER_SW_RESET);
        int channel;
        
        U32 Val = 0;
        U32 StatusReg = 0;

        for (channel=0; channel<NUMBER_OF_CHANNELS; channel++)
        {
            /* Read TSM_STREAM_STA_n register */
            StatusReg = *( TSMERGER_ChannelBase_p + TSMERGER_STATUS_OFFSET );

            if ( ( StatusReg & TSMERGER_RAM_OVERFLOW_BIT ) !=0 )
            {
                /* Clear the overflow bit since it does not clear on reading - Probably now redundant JCC @20apr06 */
                *( TSMERGER_ChannelBase_p + TSMERGER_STATUS_OFFSET ) = StatusReg & (~TSMERGER_RAM_OVERFLOW_BIT);
                
                /* Read TSM_STREAM_CFG_BASE register */
                Val = *( TSMERGER_ChannelBase_p + TSMERGER_CONFIG_OFFSET );

                /* stop the stream on bit of this channel */
                *( TSMERGER_ChannelBase_p + TSMERGER_CONFIG_OFFSET ) = Val & (~TSMERGER_STREAM_ON_BIT);

                /* Read TSM_STREAM_STA_n register - Additional JCC @20apr06 */
                Val = *( TSMERGER_ChannelBase_p + TSMERGER_STATUS_OFFSET );

                /* Clear the overflow bit since it does not clear on reading - Additional JCC @20apr06 */
                *( TSMERGER_ChannelBase_p + TSMERGER_STATUS_OFFSET ) = Val & (~TSMERGER_RAM_OVERFLOW_BIT);

                /* Read TSM_STREAM_CFG_BASE register - do we really need to read it again? JCC */
                Val = *( TSMERGER_ChannelBase_p + TSMERGER_CONFIG_OFFSET );

                /* set the stream on bit of this channel */
                *(TSMERGER_ChannelBase_p+TSMERGER_CONFIG_OFFSET) = Val | TSMERGER_STREAM_ON_BIT;

                {
                  U32 Loop=0;
                  StatusReg = *( TSMERGER_ChannelBase_p + TSMERGER_STATUS_OFFSET );
                  if ( ( StatusReg & TSMERGER_RAM_OVERFLOW_BIT ) !=0 )
                  {
                    /* Overflow bit is still set to 1 */                  
                    do
                    {
                      /* Reset of Channel is not enough, reset full TSMerger */
                          
                      *(TSMERGER_ResetBase_p) = 0;
                      *(TSMERGER_ResetBase_p) = 6;
                      for(Val=0;Val<Loop*10;Val++)__ASM_NOP; /* Incease delay at each loop in case of reset takes long time */
                      StatusReg = *( TSMERGER_ChannelBase_p + TSMERGER_STATUS_OFFSET );
                      Loop++;
                    }while((Loop<100)&&(( StatusReg & TSMERGER_RAM_OVERFLOW_BIT ) !=0));
                  }   
                }

                /* do not report an invalid interrupt if the interrupt was from the TSMerger */
                FoundTSMERGERInterrupt = TRUE;      
            }
            TSMERGER_ChannelBase_p += TSMERGER_STREAM_REGISTERS_OFFSET;
        }
    }
#endif    
    
    
    if ( ( !FoundTSMERGERInterrupt ) || ( InterruptStatus != 0 ) )
    {
        /* If this is a legal interrupt, OR this is the first illegal interrupt */
        if (((InterruptStatus & INTERRUPT_STATUS_VALID) == InterruptStatus) ||  ReportInvalidInterrupts)
        {
#if defined(ST_OSLINUX)
            AddIntStatus( FullDeviceHandle.word, 0, InterruptStatus  );

#else
            /* Send appropriate message to Interrupt Task, or if appropriate, cause error event if no message is available */
            BackendInterruptData_t *BackendInterruptData_p;
    
            BackendInterruptData_p = message_claim_timeout(STPTI_Driver.InterruptQueue_p, TIMEOUT_IMMEDIATE);
    
            if ( BackendInterruptData_p != NULL )
            {
                BackendInterruptData_p->FullHandle.word = FullDeviceHandle.word;
                BackendInterruptData_p->InterruptStatus = InterruptStatus;
    
                message_send( STPTI_Driver.InterruptQueue_p, BackendInterruptData_p );
                
                /* If we just reported our first invalid interrupt - don't do it again */
                if (ReportInvalidInterrupts && ((InterruptStatus & INTERRUPT_STATUS_VALID) != InterruptStatus))
                {
                    ReportInvalidInterrupts = FALSE;
                }            
                /* move toward being able to report message claim failures - because we have just succeeded this time */
                ++ISR_MessageQueueClaimSuccessCount;
            }
            else
            {
                /* We can't forward the data */
                if (ISR_MessageQueueClaimSuccessCount >= MESSAGE_QUEUE_SUCCESS_THRESHOLD)
                {
                    /* We previously succeeded enough times when we claimed a message => tell someone of this failure */
                    STPTI_EventData_t EventData;
        
                    EventData.u.ErrEventData.DMANumber      = 0XFACEBEEF;
                    EventData.u.ErrEventData.BytesRemaining = 0;
    
                    (void)stpti_QueueEvent(FullDeviceHandle, STPTI_EVENT_INTERRUPT_FAIL_EVT, &EventData);
                }
                /*
                 * don't tell anyone again until we've had
                 * MESSAGE_QUEUE_SUCCESS_THRESHOLD consecutive successes
                 */
                ISR_MessageQueueClaimSuccessCount = 0;
            }
#endif /* #endif defined(ST_OSLINUX) */
        }
    }
    
    /* ---------------- Check for DMA Interrupts ---------------- */
    
    {
        /* Get the signalled interupt */
        U32  DmaEmptyFlags = STSYS_ReadRegDev32LE((void*)&TC_Device_p->DMAempty_STAT) &
                             STSYS_ReadRegDev32LE((void*)&TC_Device_p->DMAempty_EN);
        U32  DirectDma;
    
        /* Disable the DMA empty flag(s) to prevent further interrupts */
        STSYS_ClearRegMask32LE((void*)&TC_Device_p->DMAempty_EN, DmaEmptyFlags);

        if ( DmaEmptyFlags )
        {
            STPTI_EventData_t       EventData;
            TCPrivateData_t        *PrivateData_p;
            TCUserDma_t            *TCUserDma_p;

            PrivateData_p =      (TCPrivateData_t *) &Device_p->TCPrivateData;
            TCUserDma_p = PrivateData_p->TCUserDma_p;

            /* Find the DMA */
            for (DirectDma = 0; DirectDma < 3; ++DirectDma)
            {
                if ((DmaEmptyFlags & (1 << DirectDma)) &&
                    (TCUserDma_p->DirectDmaUsed[DirectDma + 1] == TRUE))
                {
                    TCUserDma_p->DirectDmaUsed[DirectDma + 1] = FALSE;
                    EventData.u.DMAEventData.Destination = TCUserDma_p->DirectDmaCDAddr[DirectDma + 1];
#if defined(ST_OSLINUX)
                    AddIntStatus( FullDeviceHandle.word, STPTI_EVENT_DMA_COMPLETE_EVT,  EventData.u.DMAEventData.Destination );
#else
                    stpti_DirectQueueEvent(FullDeviceHandle, STPTI_EVENT_DMA_COMPLETE_EVT, &EventData);
#endif /* #endif defined(ST_OSLINUX) */
        
#ifdef STPTI_DEBUG_SUPPORT
                    DebugStatistics.DMACompleteEventCount++;
#endif                                
                }
            }
        }
    }    
    
    /* --------------------------------------------------------- */
    
    stpti_InterruptTotalCount++;

    /* Prevent box from being swamped by excessive error interrupts */
    /* Only allow errors to be signalled once before disabling them */
    /* Client must re-enable interrupts if interested */
    if (InterruptStatus & INTERRUPT_STATUS_INTERRUPT_FAILED)
    {
        STSYS_ClearRegMask32LE( (void*)&TC_Device_p->PTIIntEnable0, INTERRUPT_STATUS_INTERRUPT_FAILED );
    }
    
    if (InterruptStatus & INTERRUPT_STATUS_PACKET_ERROR_INTERRUPT)
    {
        STSYS_ClearRegMask32LE( (void*)&TC_Device_p->PTIIntEnable0, INTERRUPT_STATUS_PACKET_ERROR_INTERRUPT );
    }


    /* --------------------------------------------------------- */

    /* Acknowledge Interrupt */
    STSYS_WriteRegDev32LE((void*)&TC_Device_p->PTIIntAck0, InterruptStatus );

    STOS_INTERRUPT_EXIT( STOS_SUCCESS );
}

/* EOF --------------------------------------------------------- */
