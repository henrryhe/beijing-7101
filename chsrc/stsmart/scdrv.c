/*****************************************************************************
File Name   : scdrv.c

Description : Internal driver routines.

Copyright (C) 2006 STMicroelectronics

History     :

    22/03/00    Code migrated to new module from original stsmart.c.

    30/05/00    New function SMART_SetSCClk() written for better
                computation of the SCCLKGEN divider value.  This
                function takes into account the rounding errors
                introduced in the computed this value, allowing
                the real baudrate to be set on the IO interface.

                All stuart communications have been moved to the scio.c
                module.  This simplifies all smartcard communications
                routines and avoids much code duplication in the driver.

                SMART_ProcessATR() function has been re-written to
                only read bytes that are guaranteed to be present in
                the ATR, rather than assuming the maximumum and waiting
                for the timeout.  This avoids the need for a timeout
                to occur every time an ATR is performed.  NB:  the
                default timeout is 1s for an ATR timeout!

                SMART_ProcessATR() now correctly handles the TCK byte
                (check byte) that is conditionally sent by the smartcard.
                The criteria for TCK is that it should be present on
                a smartcard that supports protocols other than t=0.

    11/07/00    Fixed bug for special case where N (extra guard time)
                is reported as 0xFF by the card during the ATR.
                The ASC h/w can not support 11 etus and therefore we
                must assume 12 etus otherwise undefined behaviour
                will occur with the ASC.

    31/08/00    Added code to disable NAK detection for none t=0
                protocols.

    [..]

    30/05/03    Changed STSMART_DEBUG usage to match other drivers
                (checks for if defined, not value). Added much more
                debug output during reset process.

Reference   :

*****************************************************************************/

/* Includes --------------------------------------------------------------- */

#if defined(ST_OS20) || defined(ST_OS21)
#include <string.h>                     /* C libs */
#include <stdio.h>
#endif

#ifdef ST_OSLINUX
#include <linux/stpio.h>
#include "linux/kthread.h"
#else
#include "stlite.h"
#endif
#include "sttbx.h"
#include "stsys.h"

#include "stos.h"
#include "stcommon.h"

#include "stsmart.h"                    /* STAPI includes */

#include "scregs.h"                     /* Local includes */
#include "scio.h"
#include "scdat.h"
#include "scdrv.h"
#include "sct0.h"
#include "sct1.h"

/* Private types/constants ------------------------------------------------ */

/* Switch debouncing number of samples */
#ifndef SMART_DEBOUNCE_TIMEOUT
#define SMART_DEBOUNCE_TIMEOUT  (ST_GetClocksPerSecond()/100)
#endif

/* Override previous definitions of macros (if any) */
#ifdef MIN                              /* MIN */
#undef MIN
#endif

#ifdef MAX                              /* MAX */
#undef MAX
#endif

/* Useful macros */
#define MIN(x,y) ((x) < (y)) ? (x) : (y)
#define MAX(x,y) ((x) > (y)) ? (x) : (y)

/* Private variables ------------------------------------------------------ */

extern SMART_ControlBlock_t *SmartQueueHead_p;

/* Private macros --------------------------------------------------------- */

/* Private function prototypes -------------------------------------------- */

static SMART_ControlBlock_t *
   SMART_GetControlBlockFromPio(STPIO_Handle_t Handle);

#if defined( STSMART_WARM_RESET ) /* DDTS GNBvd53647 */

static ST_ErrorCode_t SMART_AnswerToReset(SMART_ControlBlock_t* Smart_p,
                                          U8* ReadBuffer_p,
                                          U32* NumberRead_p,
                                          BOOL ResetType  );

static ST_ErrorCode_t SMART_InitiateAnswerToWarmReset( SMART_ControlBlock_t* Smart_p,
                                                       BOOL ActiveHighReset );

#else /* ! STSMART_WARM_RESET */

static ST_ErrorCode_t SMART_AnswerToReset(SMART_ControlBlock_t *Smart_p,
                                          U8 *ReadBuffer_p,
                                          U32 *NumberRead_p );
#endif /* STSMART_WARM_RESET */

static ST_ErrorCode_t SMART_InitiateAnswerToReset( SMART_ControlBlock_t *Smart_p,
                                                   BOOL ActiveHighReset );

static ST_ErrorCode_t SMART_ProtocolSelect( SMART_ControlBlock_t *Smart_p,
                                            U8 *PTSBytes_p);

/* Exported Functions ----------------------------------------------------- */

/*****************************************************************************
SMART_PIOBitFromBitMask()
Description:
    This routine is used for converting a PIO bitmask to a bit value in
    the range 0-7. E.g., 0x01 => 0, 0x02 => 1, 0x04 => 2 ... 0x80 => 7
Parameters:
    BitMask, a bitmask - should only have a single bit set.
Return Value:
    The bit value of the bit set in the bitmask.
See Also:
    STSMART_Init()
*****************************************************************************/

U8 SMART_PIOBitFromBitMask(U8 BitMask)
{
    U8 Count = 0;                       /* Initially assume bit zero */

    /* Shift bit along until the bitmask is set to one */
    while (BitMask != 1 && Count < 7)
    {
        BitMask >>= 1;
        Count++;
    }

    return Count;                       /* Should contain the bit [0-7] */
} /* SMART_PIOBitFromBitMask() */

/*****************************************************************************
SMART_SetParams()
Description:
    This routine should be called after an ATR has been processed in order
    to set the clock and UART communications parameters as defined by
    the interface bytes sent by the ATR.

    This routine may also be used to update the API parameters from the
    private stored parameters in the driver controlblock datastructure.

Parameters:
    Smart_p,                pointer to a smartcard control block.
Return Value:
    ST_NO_ERROR,            the operation completed without error
See Also:
    Nothing.
*****************************************************************************/

ST_ErrorCode_t SMART_SetParams(SMART_ControlBlock_t *Smart_p)
{
    /* Update API params structure */
    Smart_p->SmartParams.ProtocolTypeChangeable = Smart_p->SpecificTypeChangable;
    Smart_p->SmartParams.WorkETU = Smart_p->WorkETU;
    Smart_p->SmartParams.VppActive = Smart_p->VppState;

    /* Do the inverse of what's done in STSMART_SetGuardDelay... */
    if ((Smart_p->WorkingType == STSMART_T1_PROTOCOL) && (Smart_p->N == 1))
    {
        Smart_p->SmartParams.GuardDelay = 0;
    }
    else
    {
        Smart_p->SmartParams.GuardDelay = Smart_p->N;
    }

    /* Set bit convention */
    Smart_p->SmartParams.DirectBitConvention =
        (Smart_p->BitConvention == ATR_INVERSE_CONVENTION) ? FALSE : TRUE;

    /* Set transmit retries */
    Smart_p->SmartParams.TxRetries = Smart_p->Retries;

    /* Calculate maximum baudrate */
    Smart_p->SmartParams.MaxBaudRate = Smart_p->Fmax / Smart_p->WorkETU;

    /* Set work waiting time */
    Smart_p->SmartParams.WorkWaitingTime = Smart_p->WWT;

    /* Clock frequency depends on FI reset parameter. If FInt == 0,
     * SMART_SetSCClock must have been called first, to set Smart_p->Fi.
     */
    if (Smart_p->FInt == 0)
    {
        Smart_p->SmartParams.ClockFrequency = Smart_p->Fi;
    }
    else
    {
        Smart_p->SmartParams.ClockFrequency = Smart_p->Fs;
    }

    /* Set maximum clock frequency */
    Smart_p->SmartParams.MaxClockFrequency = Smart_p->Fmax;

    /* Set the working type in the status structure */
    if ( (Smart_p->Capability.SupportedISOProtocols &
          (1 << Smart_p->WorkingType) ) != 0 )

    {
         /* The working mode is supported by the driver */
         Smart_p->Status_p->Protocol = (STSMART_Protocol_t)Smart_p->WorkingType;
    }
    else if ( (Smart_p->Capability.SupportedISOProtocols &
               (1 << Smart_p->SpecificType) ) != 0 )
    {
        /* The working mode is supported by the driver */
        Smart_p->Status_p->Protocol = (STSMART_Protocol_t)Smart_p->WorkingType;
    }
    else
    {
        /* The working mode is not supported by the driver */
        Smart_p->Status_p->Protocol = STSMART_UNSUPPORTED_PROTOCOL;
    }

    /* Enable NAKs if T=0 is in use; this can also affect stop bits */
    if ( Smart_p->WorkingType == 0 )
    {
        SMART_IO_SetNAK(Smart_p->IoHandle, TRUE);
    }
    else
    {
        SMART_IO_SetNAK(Smart_p->IoHandle, FALSE);
    }

    /* Compute new timeouts:
     *
     * - See ISO7816-3 for more information
     * - We multiply by 1000 to compute the value in
     *   millisecond intervals (used by the IO interface).
     */
    Smart_p->RxTimeout = Smart_p->WWT * 1000;
    Smart_p->RxTimeout /= Smart_p->SmartParams.BaudRate;

    /* We use the work waiting time as the basis for transmit timeouts,
     * as it applies equally.  We also add on the guard delay.
     */
    Smart_p->TxTimeout = (Smart_p->WWT + Smart_p->N) * 1000;
    Smart_p->TxTimeout /= Smart_p->SmartParams.BaudRate;

    /* Ensure that the timeout values are >= 1ms */
    Smart_p->RxTimeout = MAX(1, Smart_p->RxTimeout);
    Smart_p->TxTimeout = MAX(1, Smart_p->TxTimeout);

    /* Update new UART parameters */
    SMART_IO_SetRetries(Smart_p->IoHandle,
                        Smart_p->Retries); /* Retries */
    SMART_IO_SetGuardTime(Smart_p->IoHandle,
                          Smart_p->N);  /* Guard time */
    SMART_IO_SetBitRate(Smart_p->IoHandle,
                        Smart_p->SmartParams.BaudRate); /* Bitrate */

    /* Calculate CWT in etu */
    Smart_p->SmartParams.CharacterWaitingTime = ((1 << Smart_p->CWInt) + 11);

    /* Calculate BGT in etu */
    Smart_p->SmartParams.BlockGuardTime = 22;

    /* Calculate BWT in etu */
    if (Smart_p->FInt == 0)
    {
        /* Internal clock */
        Smart_p->SmartParams.BlockWaitingTime =
                        (((1 << Smart_p->BWInt) * Smart_p->SmartParams.BaudRate) / 10) + 11;
    }
    else
    {
        Smart_p->SmartParams.BlockWaitingTime = (((1 << Smart_p->BWInt) * 960 * 372 *
                                    Smart_p->SmartParams.BaudRate) / Smart_p->Fs) + 11;
    }

    Smart_p->SmartParams.BlockRetries = 3;

    return ST_NO_ERROR;

} /* SMART_SetParams() */

/*****************************************************************************
SMART_EventManager()
Description:
    The event manager is a single task that runs during the lifetime of
    the smartcard driver.  It is responsible for broadcasting low priority
    messages to connected clients.

    The following events are regarded as low priority:

        STSMART_EV_CARD_INSERTED
        STSMART_EV_CARD_REMOVED
        STSMART_EV_CARD_RESET
        STSMART_EV_CARD_DEACTIVATED

    Each of the above events must be notified to all connected clients, and
    are processed on a first-come-first-served basis.

    The following events are regarded as high priority:

        STSMART_EV_TRANSFER_COMPLETED

    Unfortunately, OS20 provides no mechanism for prioritizing messages
    on a message queue, so a separate handler is used for high priority
    messages.  See the UART handler routines for more information.

Parameters:
    MessageQueue_p,     pointer to the message queue where all messages are
                        received.
Return Value:
    None.
See Also:
    Nothing.
*****************************************************************************/

void SMART_EventManager(message_queue_t *MessageQueue_p)
{
    SMART_Message_t *Message_p;
    BOOL ExitTask = FALSE;

    STOS_TaskEnter(NULL);

    while (!ExitTask)
    {
        /* Wait for a message to arrive on the queue */
        Message_p = (SMART_Message_t *)STOS_MessageQueueReceive(MessageQueue_p);

        /* Check for non-NULL message -- shouldn't happen, but we check anyway */
        if (Message_p != NULL)
        {
            if (Message_p->MessageType == SMART_TASK_CONTROL)
            {
                /* Check whether or not a request has been made to exit
                 * the event manager.
                 */
                if (Message_p->MessageParams_u.TaskControlParams_s.ExitTask)
                {
                    ExitTask = TRUE;
                }
            }
            else if (Message_p->MessageType == SMART_EVENT)
            {
                SMART_ControlBlock_t *Smart_p;
                STSMART_EventType_t Event;

                Smart_p = (SMART_ControlBlock_t *)
                          Message_p->MessageParams_u.EventParams_s.Smart_p;
                Event = Message_p->MessageParams_u.EventParams_s.Event;

                if (Event == STSMART_EV_CARD_INSERTED ||
                    Event == STSMART_EV_CARD_REMOVED)
                {
                    U8 b;
                    /* Switch debouncing period */
                    STOS_TaskDelay(SMART_DEBOUNCE_TIMEOUT);

                    if (Smart_p->IsInternalSmartcard == FALSE)
                    {
                        /* Read PIO status for checking for insert/remove */
                        STPIO_Read(Smart_p->DetectHandle, &b);

                        /* Update control block with card insert status */
                        Smart_p->CardInserted = SMART_IsCardInserted(Smart_p, b);
                    }
                    else
                    {
                        Smart_p->CardInserted = TRUE;
                    }


                    /* Ascertain the correct event for notification */
                    Event = Smart_p->CardInserted ? STSMART_EV_CARD_INSERTED :
                            STSMART_EV_CARD_REMOVED;

                    /* Deactivate contacts if the card has just been removed */
                    if (Event == STSMART_EV_CARD_REMOVED)
                    {
                        /* Abort any pending operation */
                        SMART_IO_Abort(Smart_p->IoHandle);

                        /* Deactivate card contacts */
                        SMART_Deactivate(Smart_p);
                    }

                    if (Smart_p->IsInternalSmartcard == FALSE)
                    {
                         /* Re-enable the compare function for subsequent
                                  * card events.
                                  */
                        STPIO_Compare_t Compare;
#if defined(STSMART_DETECT_INVERTED)
                        Compare.ComparePattern =
                        Smart_p->CardInserted ? 0 : Smart_p->InitParams.Detect.BitMask;
                        Compare.CompareEnable = Smart_p->InitParams.Detect.BitMask;
#else
                        Compare.ComparePattern =
                        Smart_p->CardInserted ?
                             Smart_p->InitParams.Detect.BitMask : 0;
                        Compare.CompareEnable = Smart_p->InitParams.Detect.BitMask;
#endif
                        STPIO_SetCompare(Smart_p->DetectHandle,
                                     &Compare);
                    }

                    /* Update message type - after debouncing */
                    Message_p->MessageParams_u.EventParams_s.Event = Event;
                }

                /* Establish the originator of the event, and work
                 * out who wants notification of the event.
                 */
                if (Message_p->MessageParams_u.EventParams_s.Broadcast) /* Broadcast to all clients */
                {
                    U32 i;
                    void (* NotifyFunction)(STSMART_Handle_t Handle,
                                            STSMART_EventType_t Event,
                                            ST_ErrorCode_t ErrorCode);

                    if (!SMART_UsingEventHandler(Smart_p))
                    {
                        /* The event is required for all connected clients */
                        for (i = 0; i < Smart_p->InitParams.MaxHandles; i++)
                        {
                            /* Notify this client of the event, if notify routine is non-NULL --
                             * the handle of the open client is always passed back to enable the
                             * caller to determine who the event is for.
                             */
                            NotifyFunction = Smart_p->OpenHandles[i].OpenParams.NotifyFunction;
                            if (NotifyFunction != NULL &&
                                Smart_p->OpenHandles[i].InUse)
                                NotifyFunction(Smart_p->OpenHandles[i].Handle,
                                               Message_p->MessageParams_u.EventParams_s.Event,
                                               Message_p->MessageParams_u.EventParams_s.Error);
                        }
                    }
                    else
                    {
                        /* Event handler is to be used */
                        STSMART_EvtParams_t EventInfo;
                        STEVT_EventID_t EventId =
                            EventToId(Message_p->MessageParams_u.EventParams_s.Event);

                        /* Fill in event params */
                        EventInfo.Handle = 0; /* Not used */
                        EventInfo.Error =
                            Message_p->MessageParams_u.EventParams_s.Error;

                        /* Use the event handler to notify */
                        STEVT_Notify(Smart_p->EVTHandle,
                                     Smart_p->EvtId[EventId],
                                     &EventInfo);
                    }
                }
                else                    /* Single client */
                {
                    SMART_ControlBlock_t *Smart_p;
                    STSMART_Handle_t Handle;
                    void (* NotifyFunction)(STSMART_Handle_t Handle,
                                            STSMART_EventType_t Event,
                                            ST_ErrorCode_t ErrorCode);

                    /* The event is required for a single client */
                    Smart_p =
                        (SMART_ControlBlock_t *)Message_p->MessageParams_u.EventParams_s.Smart_p;

                    /* Copy value of handle for code readability */
                    Handle = Message_p->MessageParams_u.EventParams_s.Handle;

                    /* Get client notify function from open block */
                    NotifyFunction =
                        ((SMART_OpenBlock_t *)Handle)->OpenParams.NotifyFunction;

                    /* Call the notify routine with appropriate params, if
                     * event handler is not in use.
                     */
                    if (NotifyFunction != NULL &&
                        !SMART_UsingEventHandler(Smart_p))
                    {
                        NotifyFunction(
                            Handle,
                            Message_p->MessageParams_u.EventParams_s.Event,
                            Message_p->MessageParams_u.EventParams_s.Error
                        );
                    }
                    else
                    {
                        STSMART_EvtParams_t EventInfo;
                        STEVT_EventID_t EventId =
                            EventToId(Message_p->MessageParams_u.EventParams_s.Event);

                        /* Fill in event params */
                        EventInfo.Handle = Handle;
                        EventInfo.Error =
                            Message_p->MessageParams_u.EventParams_s.Error;

                        /* Use the event handler to notify */
                        STEVT_Notify(Smart_p->EVTHandle,
                                     Smart_p->EvtId[EventId],
                                     &EventInfo);
                    }
                }
            }
#if defined(STSMART_DEBUG)
            else if (Message_p->MessageType == SMART_DEBUG)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                              "%s\n",
                              Message_p->MessageParams_u.DebugParams_s.DebugMessage));
            }
#endif
            else if (Message_p->MessageType == SMART_IO_OPERATION)
            {
                SMART_ControlBlock_t *Smart_p =
                    (SMART_ControlBlock_t *)Message_p->MessageParams_u.IoParams_s.Smart_p;
                SMART_Operation_t *Op_p = &Smart_p->IoOperation;
                ST_ErrorCode_t Error = ST_NO_ERROR;

                /* Process operation based on the IO context */
                if (Error == ST_NO_ERROR)
                {
                    switch (Op_p->Context)
                    {
                        case SMART_CONTEXT_READ:
                            Error = SMART_IO_Rx(Smart_p->IoHandle,
                                                Op_p->ReadBuffer_p,
                                                Op_p->NumberToRead,
                                                Op_p->ReadBytes_p,
                                                (Op_p->Timeout == ((U32)-1)) ?
                                                Smart_p->RxTimeout : Op_p->Timeout,
                                                NULL,
                                                NULL
                                               );
                            break;
                        case SMART_CONTEXT_WRITE:
                            Error = SMART_IO_Tx(Smart_p->IoHandle,
                                                Op_p->WriteBuffer_p,
                                                Op_p->NumberToWrite,
                                                Op_p->WrittenBytes_p,
                                                (Op_p->Timeout == ((U32)-1)) ?
                                                Smart_p->TxTimeout : Op_p->Timeout,
                                                NULL,
                                                NULL
                                               );
                            break;
                        case SMART_CONTEXT_TRANSFER: /* protocol transfer */
                            switch (Smart_p->WorkingType)
                            {
                                case STSMART_T0_PROTOCOL:
                                    Error = SMART_T0_Transfer(
                                        Smart_p,
                                        Op_p->WriteBuffer_p,
                                        Op_p->NumberToWrite,
                                        Op_p->ReadBuffer_p,
                                        Op_p->NumberToRead,
                                        Op_p->WrittenBytes_p,
                                        Op_p->ReadBytes_p
                                        );
                                    break;
                                case STSMART_T1_PROTOCOL:
                                    Error = SMART_T1_Transfer(
                                        Smart_p,
                                        Op_p->WriteBuffer_p,
                                        Op_p->NumberToWrite,
                                        Op_p->WrittenBytes_p,
                                        Op_p->ReadBuffer_p,
                                        Op_p->NumberToRead,
                                        Op_p->ReadBytes_p,
                                        Op_p->OpenBlock_p->OpenParams.SAD,
                                        Op_p->OpenBlock_p->OpenParams.DAD
                                        );
                                    break;
                                default:
                                    break;
                            }
                            break;
                        case SMART_CONTEXT_RESET: /* reset operation */
                            Error = SMART_AnswerToReset(
                                               Smart_p,
                                               Op_p->ReadBuffer_p,
#if !defined( STSMART_WARM_RESET )
                                               Op_p->ReadBytes_p );
#else
                                               Op_p->ReadBytes_p,
                                               SC_COLD_RESET );
                                   break;

                        case SMART_CONTEXT_WARM_RESET: /* warm reset operation */
                            Error = SMART_AnswerToReset(
                                       Smart_p,
                                       Op_p->ReadBuffer_p,
                                       Op_p->ReadBytes_p,
                                       SC_WARM_RESET );
#endif
                                   break;

                        case SMART_CONTEXT_SETPROTOCOL: /* set protocol operation */
                            Error = SMART_ProtocolSelect(
                                Smart_p,
                                Op_p->WriteBuffer_p
                                );
                            break;
                        default:
                            break;
                    }
                }

                /* Signal to abortee that we have now aborted */
                STOS_SemaphoreWait(Smart_p->ControlLock_p);
#if defined(STSMART_LINUX_DEBUG)
                printk("scdrv.c=>SMART_EventManager(),Op_p->Context=%d,Smart_p->IoAbort=0x%x \n",Op_p->Context,Smart_p->IoAbort);
#endif
                if (Smart_p->IoAbort)
                {
                    Error = STSMART_ERROR_ABORTED;
                }
                Smart_p->IoAbort = FALSE;
                STOS_SemaphoreSignal(Smart_p->ControlLock_p);

                /* Complete the operation - process event, etc */
                SMART_CompleteOperation(Smart_p,
                                        Op_p,
                                        Error);
            }

            /* We can now release this message back to the pool */
            STOS_MessageQueueRelease(MessageQueue_p, Message_p);
        }
    } /* Forever */

    STOS_TaskExit(NULL);

} /* STSMART_EventManager() */

/* Handler routines ------------------------------------------------------- */

/*****************************************************************************
SMART_Deactivate()
Description:
    Deactivates a smartcards contacts.
Parameters:
    Smart_p,    pointer to a smartcard control block.
Return Value:
    None.
See Also:
    ISO 7816, 5.4 "Deactivation"
*****************************************************************************/

void SMART_Deactivate(SMART_ControlBlock_t *Smart_p)
{
#if !defined(PROCESSOR_C1) && !defined(ST_OSLINUX)
#pragma ST_device(ClockRegs_p)
#endif

    volatile SMART_ClkRegs_t *ClockRegs_p;

    /* Set reset to L */
    SMART_ClearReset(Smart_p);

    /* Disable smartcard clock source */
#ifdef ST_OSLINUX
    ClockRegs_p = (volatile SMART_ClkRegs_t *)Smart_p->MappingBaseAddress;
#else
    ClockRegs_p = (volatile SMART_ClkRegs_t *)Smart_p->InitParams.BaseAddress;
#endif
    SMART_ClockDisable(ClockRegs_p);

    if (Smart_p->InitParams.DeviceType != STSMART_ISO_NDS)
    {
        /* Reprogram clock PIO pin */
        U8 i;
        STPIO_BitConfig_t BitConfigure[8];
        i = SMART_PIOBitFromBitMask(Smart_p->InitParams.Clk.BitMask);
        BitConfigure[i] = STPIO_BIT_OUTPUT;
        STPIO_SetConfig(Smart_p->ClkHandle, BitConfigure);
    }

    /* Disable Vcc */
    SMART_DisableVcc(Smart_p);

    /* Clear the "reset" flag */
    Smart_p->CardReset = STSMART_ERROR_NOT_RESET;
} /* SMART_Deactivate() */

/*****************************************************************************
SMART_CompleteOperation()
Description:
    This routine should be called to perform final tidying/completion of
    a smartcard operation.  This will result in either the API call been
    awoken up (via posting a semaphore) or a callback being generated.

    For "broadcast" events, the event manager will take care of the
    callback after a message is placed on the event manager's message
    queue.

    For "single-client" operations, the callback is performed immediately.

Parameters:
    Operation_p,    pointer to the operation to complete.
Return Value:
    None.
See Also:
    SMART_IoHandler()
*****************************************************************************/

void SMART_CompleteOperation(SMART_ControlBlock_t *Smart_p,
                             SMART_Operation_t *Operation_p,
                             ST_ErrorCode_t ErrorCode)
{
    SMART_Operation_t Operation;

    /* Copy operation parameters */
    Operation = *Operation_p;

    /* Free up the operation */
    STOS_SemaphoreWait(Smart_p->ControlLock_p);
    Operation_p->Context = SMART_CONTEXT_NONE;
    STOS_SemaphoreSignal(Smart_p->ControlLock_p);

    /* Set status */
    Smart_p->Status_p->ErrorCode = ErrorCode;

    /* Set caller's status */
    if (Smart_p->IoOperation.Status_p != NULL)
    {
        *Smart_p->IoOperation.Status_p = *Smart_p->Status_p;
    }

    /* An operation has completed on the UART -- process the notification
     * of this event.
     */
    if (!Operation.OpenBlock_p->OpenParams.BlockingIO) /* Blocking call? */
    {
        if (Operation.Context == SMART_CONTEXT_RESET) /* broadcast */
        {
            /* This is a broadcast event to all connected clients */
            SMART_Message_t *Msg_p;

            /* Claim a message from the free message pool -- but we can't
             * afford to wait for it too long.
             */
            Msg_p = (SMART_Message_t *)STOS_MessageQueueClaimTimeout(Smart_p->MessageQueue_p,
                                                             TIMEOUT_IMMEDIATE);

            /* Check we claimed a message buffer */
            if (Msg_p != NULL)
            {
                /* Setup the smartcard event to be generated */
                Msg_p->MessageType = SMART_EVENT;
                Msg_p->MessageParams_u.EventParams_s.Smart_p = Smart_p;
                Msg_p->MessageParams_u.EventParams_s.Broadcast = TRUE;
                Msg_p->MessageParams_u.EventParams_s.Handle =
                    Operation.OpenBlock_p->Handle;
                Msg_p->MessageParams_u.EventParams_s.Event = STSMART_EV_CARD_RESET;
                Msg_p->MessageParams_u.EventParams_s.Error = ErrorCode;

                /* Now send the message to the queue */
                STOS_MessageQueueSend(Smart_p->MessageQueue_p, Msg_p);
            }
        }
    }
    if (!Operation.OpenBlock_p->OpenParams.BlockingIO) /* Blocking call? */
    {
        STSMART_EventType_t EventType;

        /* The caller isn't blocking, so notify them of the outcome of
         * this operation.
         */
        switch ( Operation.Context )
        {
            case SMART_CONTEXT_SETPROTOCOL:
            case SMART_CONTEXT_TRANSFER:
                EventType = STSMART_EV_TRANSFER_COMPLETED;
                break;
            case SMART_CONTEXT_READ:
                EventType = STSMART_EV_READ_COMPLETED;
                break;
            case SMART_CONTEXT_WRITE:
                EventType = STSMART_EV_WRITE_COMPLETED;
                break;
            default:
                EventType = STSMART_EV_NO_OPERATION;
                break;
        }

        if ( EventType != STSMART_EV_NO_OPERATION )
        {
            /* Use callback function */
            if (Operation.OpenBlock_p->OpenParams.NotifyFunction != NULL &&
                !SMART_UsingEventHandler(Smart_p))
            {
                /* Directly notify the calling client */
                Operation.OpenBlock_p->OpenParams.NotifyFunction(
                    Operation.OpenBlock_p->Handle,
                    EventType,
                    ErrorCode
                    );
            }
            else                        /* Use event handler */
            {
                STSMART_EvtParams_t EventInfo;
                STEVT_EventID_t EventId =
                    EventToId(EventType);

                /* Fill in event params */
                EventInfo.Handle = Operation.OpenBlock_p->Handle;
                EventInfo.Error = ErrorCode;

                /* Use the event handler to notify */
                STEVT_Notify(Smart_p->EVTHandle,
                             Smart_p->EvtId[EventId],
                             &EventInfo);
            }
        }
    }
    else
    {
        /* The caller is blocked, so wake them up.  The API call
         * should return the error code to them.
         */
        *Operation.Error_p = ErrorCode;
        STOS_SemaphoreSignal(Operation.NotifySemaphore_p);
    }

} /* SMART_CompleteOperation() */

/*****************************************************************************
SMART_DetectHandler()
Description:
    This callback is invoked whenever a bit alters on the PIO port for
    the card detect pin.

    All connected clients will be notified of the change i.e., a new
    message is sent to the event manager's message queue.

Parameters:
    Handle,             handle for PIO device.
    BitMask,            bitmask containing source of interrupt i.e., DETECT
                        pin has changed state.
Return Value:
    None.
See Also:
    Nothing.
*****************************************************************************/

void SMART_DetectHandler(STPIO_Handle_t Handle, STPIO_BitMask_t BitMask)
{
    SMART_ControlBlock_t *Smart_p;

    /* Get the control block whose smartcard has been inserted/removed */
    Smart_p = SMART_GetControlBlockFromPio(Handle);

    /* If the control block is valid, then updated the "card inserted" member
     * of the control block to indicate that the card has
     * been inserted/removed.
     */
    if (Smart_p != NULL)
    {
        SMART_Message_t *Msg_p;

        if (Smart_p->IsInternalSmartcard == FALSE)
        {
            /* Abort any pending IO */
            SMART_IO_Abort(Smart_p->IoHandle);

            /* Disable compare function - debouncing takes place in event mgr */
            {
                STPIO_Compare_t Compare;
                Compare.CompareEnable = 0;
                STPIO_SetCompare(Smart_p->DetectHandle,
                                  &Compare);
            }
            /* Claim a message from the free message pool -- but we can't
            * afford to wait for it as we are running in the context of
            * PIO.
            */
            Msg_p = (SMART_Message_t *)STOS_MessageQueueClaimTimeout(Smart_p->MessageQueue_p,
                                                         TIMEOUT_IMMEDIATE);

            /* Check we claimed a message buffer */
            if (Msg_p != NULL)
            {
                /* Setup the smartcard event to be generated */
                Msg_p->MessageType = SMART_EVENT;
                Msg_p->MessageParams_u.EventParams_s.Smart_p = Smart_p;
                Msg_p->MessageParams_u.EventParams_s.Broadcast = TRUE;
                Msg_p->MessageParams_u.EventParams_s.Handle = 0;
                Msg_p->MessageParams_u.EventParams_s.Event =
                STSMART_EV_CARD_INSERTED;  /* Assume inserted */
                Msg_p->MessageParams_u.EventParams_s.Error = ST_NO_ERROR;

                /* Now send the message to the queue */
                STOS_MessageQueueSend(Smart_p->MessageQueue_p, Msg_p);
            }
        }
    }
#if defined(STSMART_DEBUG)
    else
    {
        SMART_Message_t *Msg_p;

        /* Claim a message from the free message pool -- but we can't
         * afford to wait for it as we are running in the context of
         * PIO.
         */
        Msg_p = (SMART_Message_t *)STOS_MessageQueueClaimTimeout(Smart_p->MessageQueue_p,
                                                         TIMEOUT_IMMEDIATE);

        /* Check we claimed a message buffer */
        if (Msg_p != NULL)
        {
            /* Setup the smartcard event to be generated */
            Msg_p->MessageType = SMART_DEBUG;

            sprintf(Msg_p->MessageParams_u.DebugParams_s.DebugMessage,
                    "SMART_DetectHandler: Bad PIO handle = %u",
                    (U32)Handle);

            /* Now send the message to the queue */
            STOS_MessageQueueSend(Smart_p->MessageQueue_p, Msg_p);
        }
    }
#endif
} /* SMART_DetectHandler() */

/*****************************************************************************
SMART_ClearStatus()
Description:
    This routine clears the status parameters to defaults.
Parameters:
    Smart_p,    pointer to smartcard control block.
Return Value:
    None.
See Also:
    Nothing.
*****************************************************************************/

void SMART_ClearStatus(SMART_ControlBlock_t *Smart_p)
{
    /* Clear out status parameters */
    Smart_p->Status_p->ErrorCode = ST_NO_ERROR;
    Smart_p->Status_p->Protocol = STSMART_UNSUPPORTED_PROTOCOL;
} /* SMART_ClearStatus() */

/*****************************************************************************
SMART_SetETU()
Description:
    This routine computes the ETU value based on the ATR parameters,
    FInt and DInt.
Parameters:
    Smart_p,    pointer to smartcard control block.
Return Value:
    None.
See Also:
    Nothing.
*****************************************************************************/

void SMART_SetETU(SMART_ControlBlock_t *Smart_p)
{
    U16 InitialETU, WorkETU;
    U32 MaxClock;
    U32 WWT;

    /* Calculate work waiting time in work etus */
    WWT = 960 * Smart_p->WInt;

    /* Next translate our rate conversion factor Interger into F.
     * Used conversion table in IS0 7816-3 for values, coded this way to
     * make it easier to read.
     */
    switch(Smart_p->FInt)
    {
        case 0: /* Internal clock, treat differently */
            InitialETU = Smart_p->Fi / 9600;
            MaxClock = 0;
            break;
        case 1:
            InitialETU = 372;
            MaxClock = 5*MHZ;
            break;
        case 2:
            InitialETU = 558;
            MaxClock = 6*MHZ;
            break;
        case 3:
            InitialETU = 744;
            MaxClock = 8*MHZ;
            break;
        case 4:
            InitialETU = 1116;
            MaxClock = 12*MHZ;
            break;
        case 5:
            InitialETU = 1488;
            MaxClock = 16*MHZ;
            break;
        case 6:
            InitialETU = 1860;
            MaxClock = 20*MHZ;
            break;
        case 9:
            InitialETU = 512;
            MaxClock = 5*MHZ;
            break;
        case 10:
            InitialETU = 768;
            MaxClock = 7500000;
            break;
        case 11:
            InitialETU = 1024;
            MaxClock = 10*MHZ;
            break;
        case 12:
            InitialETU = 1536;
            MaxClock = 15*MHZ;
            break;
        case 13:
            InitialETU = 2048;
            MaxClock = 20*MHZ;
            break;
        default:
            InitialETU = 372;
            MaxClock = 0;
            break;
    }

    /* Now adjust using the Bit rate adjustment factor D.
     * This is also taken from ISO7816-3 and coded this way
     * to make it easier to read.
     */
    switch(Smart_p->DInt)
    {
        case 1:
            WorkETU = InitialETU;
            break;
        case 2:
            WorkETU = InitialETU/2;
            WWT = WWT * 2;
            break;
        case 3:
            WorkETU = InitialETU/4;
            WWT = WWT * 4;
            break;
        case 4:
            WorkETU = InitialETU/8;
            WWT = WWT * 8;
            break;
        case 5:
            WorkETU = InitialETU/16;
            WWT = WWT * 16;
            break;
        case 6:
            WorkETU = InitialETU/32;
            WWT = WWT * 32;
            break;
        case 8:
            WorkETU = InitialETU/12;
            WWT = WWT * 12;
            break;
        case 9:
            WorkETU = InitialETU/20;
            WWT = WWT * 20;
            break;
        case 10:
            WorkETU = InitialETU*2;
            WWT = WWT / 2;
            break;
        case 11:
            WorkETU = InitialETU*4;
            WWT = WWT / 4;
            break;
        case 12:
            WorkETU = InitialETU*8;
            WWT = WWT / 8;
            break;
        case 13:
            WorkETU = InitialETU*16;
            WWT = WWT / 16;
            break;
        case 14:
            WorkETU = InitialETU*32;
            WWT = WWT / 32;
            break;
        case 15:
            WorkETU = InitialETU*64;
            WWT = WWT / 64;
            break;
        default:
            WorkETU = InitialETU;
            break;
    }

    /* Now set the ETU */
    Smart_p->WorkETU = WorkETU;

    /* Set maximum clock frequency */
    Smart_p->Fmax = MaxClock;

    /* Set subsequent clock frequency */
    Smart_p->Fs = Smart_p->Fi;

    /* Set new WWT */
    Smart_p->WWT = WWT;

} /* SMART_SetETU() */

/*****************************************************************************
SMART_SetSCClock()
Description:
    Sets the smartcard clock to a frequency to match the required
    bit-rate/etu.
Parameters:
    Smart_p         pointer to smartcard control block
    Etu             required ETU.
    BitRate         required bit-rate.
    ActualClk       stores new clock frequency to smartcard
    ActualBitRate   stores new bit-rate to smartcard.
Return Value:
    None.
See Also:
    Nothing.
*****************************************************************************/

void SMART_SetSCClock(SMART_ControlBlock_t *Smart_p,
                      U32 Etu,
                      U32 BitRate,
                      U32 *ActualClk_p)
{
#if !defined(PROCESSOR_C1) && !defined(ST_OSLINUX)
#pragma ST_device(ClockRegs_p)
#endif

    volatile SMART_ClkRegs_t *ClockRegs_p;
    U32 ClkDiv, ClkF;

    /* Setup register pointer */
#ifdef ST_OSLINUX
    ClockRegs_p = (volatile SMART_ClkRegs_t *)Smart_p->MappingBaseAddress;
#else
    ClockRegs_p = (volatile SMART_ClkRegs_t *)Smart_p->InitParams.BaseAddress;
#endif

    /* Calculate new clock divider (round-up to nearest integer) */
    ClkDiv = (Smart_p->InitParams.ClockFrequency + (BitRate * Etu)) /
             (BitRate * Etu * 2);

    /* Check for clock divider boundary conditions and round accordingly */
    if (ClkDiv == 0)                    /* Zero not allowed */
    {
        ClkDiv = 1;                     /* Min clock div */
    }
    else if (ClkDiv >= (2<<5))          /* ClkDiv is only 5 bits */
    {
        ClkDiv = (2<<5)-1;              /* Max clock div */
    }

    /* Calculate new smartcard clock frequency */
    ClkF = Smart_p->InitParams.ClockFrequency / (ClkDiv * 2);

    /* Set clock divider register */
    ClockRegs_p->ScClkVal = ClkDiv;

    /* Compute new bitrate */
    *ActualClk_p = ClkF;

#if defined(STSMART_LINUX_DEBUG)
    printk("scdrv.c=>SMART_SetSCCClock(), Smart_p->InitParams.ClockFrequency=%d,BitRate=%d,Etu=%d \n",Smart_p->InitParams.ClockFrequency,BitRate,Etu);
    printk("scdrv.c=>SMART_SetSCCClock(), *ActualClk_p=ClkF=%d,ClockRegs_p->ScClkVal=ClkDiv=%d \n", ClkF,ClkDiv);
#endif
    /* Ensure clock is enabled */
    SMART_ClockEnable(ClockRegs_p);

} /* SMART_SetSCClock() */

/*****************************************************************************
SMART_SetClockFrequency()
Description:
    Sets the smartcard clock to a specified frequency only. No
    consideration of bit-rate or ETU is made; it is assumed the caller
    has dealt with all that. Clock values are in MHz.
Parameters:
    Smart_p         pointer to smartcard control block
    RequestedClk    Clock frequency requested
    ActualClk       stores new clock frequency to smartcard
Return Value:
    None.
See Also:
    Nothing.
*****************************************************************************/
void SMART_SetClockFrequency(SMART_ControlBlock_t *Smart_p,
                             U32 RequestedClk,
                             U32 *ActualClk_p)
{
#if !defined(PROCESSOR_C1) && !defined(ST_OSLINUX)
#pragma ST_device(ClockRegs_p)
#endif
    volatile SMART_ClkRegs_t *ClockRegs_p;
    /* Clock divider, temporary frequency, the last error value */
    U32 ClkDiv, ClkF, LastError;
    /* Temporary, so we get negative values */
    S32 Error;

    /* Setup register pointer */
#ifdef ST_OSLINUX
    ClockRegs_p = (volatile SMART_ClkRegs_t *)Smart_p->MappingBaseAddress;
#else
    ClockRegs_p = (volatile SMART_ClkRegs_t *)Smart_p->InitParams.BaseAddress;
#endif

    /* Find the divider which gives the frequency closest to that
     * requested. This is basically done by starting off with the
     * fastest, then going down until the error starts increasing
     * instead of decreasing. Turned out to be easiest that way rather
     * than finding the 'middle' one and checking each side.
     */
    ClkDiv = 1;
    LastError = (U32)-1;
    /* Divider register is 5-bits wide, so max 0b11111 => 0x1f */
    while (ClkDiv <= 0x1f)
    {
        ClkF = Smart_p->InitParams.ClockFrequency / (ClkDiv * 2);
        /* get abs error */
        Error = ClkF - RequestedClk;
        if (Error < 0)
        {
            Error = -Error;
        }

        if (Error < LastError)
        {
            LastError = Error;
        }
        else
        {
            /* Undo the last increment */
            ClkDiv--;
            break;
        }
        ClkDiv++;
    }

    /* Check for clock divider boundary conditions and round accordingly */
    if (ClkDiv == 0)                    /* Zero not allowed */
        ClkDiv = 1;                     /* Min clock div */
    else if (ClkDiv >= (2<<5))          /* ClkDiv is only 5 bits */
        ClkDiv = (2<<5)-1;              /* Max clock div */

    /* Calculate new smartcard clock frequency */
    *ActualClk_p = Smart_p->InitParams.ClockFrequency / (ClkDiv * 2);

    /* Store the new frequency in the parameters */
    Smart_p->SmartParams.ClockFrequency = *ActualClk_p;

    /* Set clock divider register */
    ClockRegs_p->ScClkVal = ClkDiv;

    /* Ensure clock is enabled */
    SMART_ClockEnable(ClockRegs_p);

} /* SMART_SetClockFrequency() */

ST_ErrorCode_t SMART_CheckTransfer(SMART_ControlBlock_t *Smart_p,
                                   U8 *Buffer_p)
{
    ST_ErrorCode_t error = ST_NO_ERROR;

    switch (Smart_p->WorkingType)
    {
        case 0:
            error = SMART_T0_CheckTransfer(Buffer_p);
            break;
        case 1:
        default:
            break;
    }

    return error;
}

/* Private Functions ------------------------------------------------------ */

static ST_ErrorCode_t SMART_InitiateAnswerToReset( SMART_ControlBlock_t *Smart_p,\
                                                    BOOL ActiveHighReset
                                                  )
{
    U32 i;
    STPIO_BitConfig_t BitConfigure[8];
    U32 ActualBitRate;
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;

#if defined( STSMART_WARM_RESET )
    SMART_DisableVcc(Smart_p);   /* Disable Vcc */
    STOS_TaskDelay(ST_GetClocksPerSecond() * 10 / SMART_INITIAL_BAUD);

#endif

#ifndef ST_OSLINUX    /* This is added for STB7200 on os21*/
    SMART_SetReset(Smart_p); 	/* Raise RST */
    STOS_TaskDelay(ST_GetClocksPerSecond()  / SMART_INITIAL_BAUD);
#endif	
    SMART_EnableVcc(Smart_p);   /* Enable Vcc */

    /* allow I/O line to return high and ensure any byte currently being read
      completes. 10 ticks is the observed minimum on 5514/mb314b */
    STOS_TaskDelay(ST_GetClocksPerSecond() * 10 / SMART_INITIAL_BAUD);

    /* Flush receive buffers before commencing ATR */
    ErrorCode=SMART_IO_Flush(Smart_p->IoHandle);

#if defined(STSMART_LINUX_DEBUG)
    printk("scdrv.c=>SMART_InitiateAnswerToReset(),after call SMART_IO_Flush,ErrorCode=0x%x\n",ErrorCode);
#endif

    /* Default timeouts */
    Smart_p->RxTimeout = SMART_RX_TIMEOUT;
    Smart_p->TxTimeout = SMART_TX_TIMEOUT;

    /* T=1 defaults */
    Smart_p->T1Details.TxMaxInfoSize = 32;
    Smart_p->T1Details.RxMaxInfoSize = 32;
    Smart_p->T1Details.OurSequence = 0;
    Smart_p->T1Details.TheirSequence = 1;
    Smart_p->CWInt = 13;
    Smart_p->BWInt = 4;

    /* Setup the smartcard clock divider */
    if (0 == Smart_p->ResetFrequency)
    {
        SMART_SetSCClock(Smart_p,
                         SMART_INITIAL_ETU,
                         SMART_INITIAL_BAUD,
                         &Smart_p->Fi);
    }
    else
    {
        /* Should already be exact (calculated when user made
         * STSMART_SetClockFrequency call), but just in case.
         */
        U32 ActFreq;

        SMART_SetClockFrequency(Smart_p,
                                Smart_p->ResetFrequency, &ActFreq);
        if (ActFreq != Smart_p->ResetFrequency)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                         "Smart_p->ResetFrequency != ActFreq\n"));
        }

        /* This is used in various places further down */
        Smart_p->Fi = Smart_p->ResetFrequency;
    }

#if defined (STSMART_WARM_RESET)
    Smart_p->WarmResetFreq = Smart_p->Fi;
#endif

#if defined(STSMART_LINUX_DEBUG)
    printk("scdrv.c=>SMART_InitiateAnswerToReset(),Smart_p->Fi=%d\n",Smart_p->Fi);
#endif

    /* Set the IO bit-rate for the UART */
    ActualBitRate = (Smart_p->Fi / SMART_INITIAL_ETU);
    ErrorCode=SMART_IO_SetBitRate(Smart_p->IoHandle, ActualBitRate);
#if defined(STSMART_LINUX_DEBUG)
    printk("scdrv.c=>SMART_InitiateAnswerToReset() After call SMART_IO_SetBitRate()<=ErrorCode=0x%x\n",ErrorCode);
#endif

    if(Smart_p->InitParams.DeviceType != STSMART_ISO_NDS)
    {
        /* Enable clock to the smartcard */
        i = SMART_PIOBitFromBitMask(Smart_p->InitParams.Clk.BitMask);
        BitConfigure[i] = STPIO_BIT_ALTERNATE_OUTPUT;
        ErrorCode=STPIO_SetConfig(Smart_p->ClkHandle, BitConfigure);
    }

    /* Disable Vpp - not required for reset */
#ifndef ST_OSLINUX
    SMART_DisableVpp(Smart_p);
#endif
#if 0/*zxg 20070726 change*/
    /* Perform the ATR sequence */
    SMART_ClearReset(Smart_p);  /* Lower RST */

    if (ActiveHighReset)
    {
        /* Wait for 100milliseconds; this should satisfy the requirements
         * for 7816-3, which is minimum of 400 clock cycles between
         * reset low and reset high. (Section 5.3.2)
         */
        STOS_TaskDelay(ST_GetClocksPerSecond() / 10);
        SMART_SetReset(Smart_p); /* Raise RST */
    }
#else
if (ActiveHighReset)
    	{
    	        SMART_SetReset(Smart_p); /* Raise RST */

    	}
    else
	{
    		SMART_ClearReset(Smart_p);  /* Lower RST */
    	}
	
       STOS_TaskDelay(ST_GetClocksPerSecond() / 8);/*cheng: ? what is requirement-2007/01/04*/
	   
    if (ActiveHighReset)
    	{
    	        SMART_ClearReset(Smart_p); /* Raise RST */

    	}
    else
	{
    		SMART_SetReset(Smart_p);  /* Lower RST */
    	}



#endif
    return ErrorCode/*ST_NO_ERROR*/;

} /* SMART_InitiateAnswerToReset() */



#if defined (STSMART_WARM_RESET)

static ST_ErrorCode_t SMART_InitiateAnswerToWarmReset(  SMART_ControlBlock_t *Smart_p,
                                                        BOOL ActiveHighReset
                                                      )
{
    U32 ActualBitRate;
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;

    /* No need to enable VCC */

    /* Flush receive buffers before commencing ATR */
    ErrorCode=SMART_IO_Flush(Smart_p->IoHandle);

    /* Default timeouts */
    Smart_p->RxTimeout = SMART_RX_TIMEOUT;
    Smart_p->TxTimeout = SMART_TX_TIMEOUT;

    /* T=1 defaults */
    Smart_p->T1Details.TxMaxInfoSize = 32;
    Smart_p->T1Details.RxMaxInfoSize = 32;
    Smart_p->T1Details.OurSequence = 0;
    Smart_p->T1Details.TheirSequence = 1;
    Smart_p->CWInt = 13;
    Smart_p->BWInt = 4;

    /* Set the IO bit-rate for the UART */

    ActualBitRate = (Smart_p->WarmResetFreq / SMART_INITIAL_ETU);
    ErrorCode=SMART_IO_SetBitRate(Smart_p->IoHandle, ActualBitRate);

    /* Disable Vpp - not required for reset */
#ifndef ST_OSLINUX
    if(Smart_p->CmdVppHandle)
    {
        SMART_DisableVpp(Smart_p);
    }
#endif
    /* Perform the ATR sequence as per NDS spec*/

    /* Perform the ATR sequence */

    if (ActiveHighReset)    /* Lower then raise RST*/
    {
        SMART_ClearReset(Smart_p);  /* Lower RST */
        /* wait at least 1 milliseconds  */
        STOS_TaskDelay(ST_GetClocksPerSecond() / 100 );
        SMART_SetReset(Smart_p); /* Raise RST */
    }
    else
    {   /* Raise then lower RST */

    	SMART_SetReset(Smart_p);
    	/* wait at least 1 milliseconds  */
        STOS_TaskDelay( ST_GetClocksPerSecond() / 100 );
        SMART_ClearReset(Smart_p);
    }

    return ErrorCode /*ST_NO_ERROR*/;

} /* SMART_InitiateAnswerToWarmReset() */

#endif /* STMART_WARM_RESET */

static ST_ErrorCode_t SMART_AnswerToReset(SMART_ControlBlock_t *Smart_p,
                                          U8 *ReadBuffer_p,
#if defined( STSMART_WARM_RESET )
                                          U32 *NumberRead_p,
                                          BOOL ResetType )
#else
                                          U32 *NumberRead_p )
#endif
{
    ST_ErrorCode_t error = ST_NO_ERROR;

    /* NOTE: the processing of the ATR is very cryptic. I will try to explain
     * things as I go along. The naming convention of TA1, TB1, etc is from the
     * smartcard standard ISO/IEC 7816-3, so don't blame me.
     */
    U32 i = 0, k;
    U8 BitmaskYx = 0;
    U8 InterfaceX = 1;
    U8 ModeT = 0, FirstT = 0;
    U8 HistoricalK = 0;
    U8 TAx, TBx, TCx, TDx;
    BOOL ActiveHigh;
    BOOL MinGuardTime = FALSE;
    BOOL TA1Present = FALSE;

    /* Reset caller's parameters */
    *NumberRead_p = 0;

    /* Changes done for NDS type of cards */
    if (Smart_p->InitParams.DeviceType == STSMART_ISO_NDS)
    {
        ActiveHigh = TRUE;
    }
    else
    {
        ActiveHigh = FALSE;
    }

    /* Reset supported protocol types */
    Smart_p->SmartParams.SupportedProtocolTypes = 0;
    Smart_p->N = SMART_N_DEFAULT;
    Smart_p->SpecificMode = FALSE; /* GNBVD58792*/

    if (Smart_p->InitParams.DeviceType == STSMART_ISO_NDS)
    {
        Smart_p->BitConvention = ATR_INVERSE_CONVENTION;
        error = SMART_IO_SetBitCoding(Smart_p->IoHandle, INVERSE);
    }
    else
    {
        /* Reset convention to direct for next ATR. */
        Smart_p->BitConvention = ATR_DIRECT_CONVENTION;
        error = SMART_IO_SetBitCoding(Smart_p->IoHandle, DIRECT);
    }

    if (error != ST_NO_ERROR)
    {
#if defined(STSMART_DEBUG)
        STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                     "%s: received error 0x%08x setting bit coding\n",
                     Smart_p->DeviceName, (U32)error));
#endif
        return error;
    }

    /* Initiate the ATR */
    for (;;)
    {
#if defined(STSMART_DEBUG)
        STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                     "%s: trying active %s reset, with %s convention",
                     Smart_p->DeviceName,
                     (ActiveHigh == TRUE)?"high":"low",
                     (Smart_p->BitConvention == ATR_DIRECT_CONVENTION)?"direct":"inverse"));
#endif

#if defined( STSMART_WARM_RESET )
        if( ResetType == SC_COLD_RESET )
        {
            SMART_InitiateAnswerToReset( Smart_p, ActiveHigh );
        }
        else
        {
	     if (Smart_p->InitParams.DeviceType != STSMART_ISO_NDS)
            		ActiveHigh = TRUE;
            SMART_InitiateAnswerToWarmReset( Smart_p, ActiveHigh );
        }
#else
        SMART_InitiateAnswerToReset( Smart_p, ActiveHigh );
#endif

        /* Read first bytes of the ATR */
        error = SMART_IO_Rx(Smart_p->IoHandle,
                            Smart_p->ATRBytes,
                            SMART_MIN_ATR,
                            &k,
                            Smart_p->RxTimeout,
                            NULL,
                            NULL
                           );
#if defined(STSMART_DEBUG)
        if (error != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                         "%s: error 0x%08x reading first ATR bytes - got %i",
                         Smart_p->DeviceName,
                         (U32)error, k));
        }
#endif

        /* Check for error cases */
        if (error == STSMART_ERROR_PARITY)
        {
            /* De-activate the card */
            SMART_Deactivate(Smart_p);

            if (Smart_p->InitParams.DeviceType == STSMART_ISO_NDS)
            {
                /* Convert bit-convention */
                if (Smart_p->BitConvention == ATR_INVERSE_CONVENTION)
                {
                    /* task_delay in InitiateAnswerToReset is important to ensure
                    any partially complete second byte of this first ATR is not
                    retained and noted as a second parity error. Alternatively,
                    a STOS_TaskDelay() here of 12 or more was observed to also cure
                    the problem on 5516B/mb361a, unified memory, caches off */
                    Smart_p->BitConvention = ATR_DIRECT_CONVENTION;
                    SMART_IO_SetBitCoding(Smart_p->IoHandle, DIRECT);
                }
                else
                {
                    /* Answer-to-reset failed -- tried both conventions */
#if defined(STSMART_DEBUG)
                    STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                                 "%s: tried both conventions, failed",
                                 Smart_p->DeviceName));
#endif
                    return STSMART_ERROR_NO_ANSWER;
                }
            }
            /* If not NDS Card */
            else
            {
                /* Convert bit-convention */
                if (Smart_p->BitConvention == ATR_DIRECT_CONVENTION)
                {
                    /* task_delay in InitiateAnswerToReset is important to ensure
                    any partially complete second byte of this first ATR is not
                    retained and noted as a second parity error. Alternatively,
                    a STOS_TaskDelay() here of 12 or more was observed to also cure
                    the problem on 5516B/mb361a, unified memory, caches off */
                    Smart_p->BitConvention = ATR_INVERSE_CONVENTION;
                    SMART_IO_SetBitCoding(Smart_p->IoHandle, INVERSE);
                }
                else
                {
                    /* Answer-to-reset failed -- tried both conventions */
#if defined(STSMART_DEBUG)
                    STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                                 "%s: tried both conventions, failed",
                                 Smart_p->DeviceName));
#endif
                    return STSMART_ERROR_NO_ANSWER;
                }
            }
        }
        else if (error == ST_ERROR_TIMEOUT)
        {
            /* De-activate the card */
            SMART_Deactivate(Smart_p);

            /* changed here for NDS types of cards */
            if (Smart_p->InitParams.DeviceType == STSMART_ISO_NDS)
            {
                /* No I/O received - should we try active low RST? */
                if (ActiveHigh)
                {
                    /* Try active low reset */
                    ActiveHigh = FALSE;
                }
                else
                {
                    /* Fail -- neither active low nor active high */
#if defined(STSMART_DEBUG)
                    STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                                "%s: both active high and low failed",
                                Smart_p->DeviceName));
#endif
                    return STSMART_ERROR_NO_ANSWER;
                }
            }
            else
            {

               /* No I/O received - should we try active high RST? */
                if (!ActiveHigh)
                {
                    /* Try active high reset */
                    ActiveHigh = TRUE;
                }
                else
                {
                    /* Fail -- neither active low nor active high */
#if defined(STSMART_DEBUG)
                    STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                                "%s: both active high and low failed",
                                Smart_p->DeviceName));
#endif
                    return STSMART_ERROR_NO_ANSWER;
                }
            }
        }
        else if (error != ST_NO_ERROR)
        {
#if defined(STSMART_LINUX_DEBUG)
            printk("scdrv.c=>SMART_AnswerToReset(),after call SMART_IO_Rx(),error!=ST_N0_ERROR,!=ST_ERROR_TIMEOUT\n");
#endif
            STOS_TaskDelay(5 * ST_GetClocksPerSecond());

            /* De-activate the card */
            SMART_Deactivate(Smart_p);

            /* Non-recoverable error case */
            return STSMART_ERROR_NO_ANSWER;
        }
        else
        {
            break;
        }
    }

    /* TS provides a bit synchronization sequence and a data convention,
     * basically to the software this means two values are valid.
     */
    Smart_p->TS = Smart_p->ATRBytes[i++];
    if ((Smart_p->TS == ATR_INVERSE_CONVENTION) ||
        (Smart_p->TS == ATR_DIRECT_CONVENTION))
    {
        /* The next character is the Format Character T0 */
        /* T0 contains the first Yx bitmask for the next group of data
         * and K the historical extra bits at the end of the message */
        BitmaskYx = Smart_p->ATRBytes[i++];
        HistoricalK = BitmaskYx & 0x0F;

        /* Now we will walk through the series of TAx, TBx, TCx, TDx 's
         * until all have been processed, initial mode = 0.
         * Initial interfaceX = 1.
         */
        while (BitmaskYx != 0)
        {
            if (BitmaskYx & 0x10)
            {
                /* TAx present */
                error = SMART_IO_Rx(Smart_p->IoHandle,
                                    &Smart_p->ATRBytes[i],
                                    1,
                                    &k,
                                    Smart_p->RxTimeout,
                                    NULL,
                                    NULL);
                if (error != ST_NO_ERROR)
                {
                    /* Failed to read TAx */
#if defined(STSMART_DEBUG)
                    STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                                 "%s: error 0x%08x reading TA%i",
                                 Smart_p->DeviceName,
                                 InterfaceX, (U32)error));
#endif
                    error = STSMART_ERROR_RESET_FAILED;
                    break;
                }

                TAx = Smart_p->ATRBytes[i++];
                if (InterfaceX == 1)
                {
                    /* TA1 codes FI over the most significant half byte */
                    Smart_p->FInt=(TAx & 0xF0)>>4;
                    /* TA1 codes DI over the least significant half */
                    Smart_p->DInt=(TAx & 0x0F);
                    TA1Present = TRUE;
                }
                if (InterfaceX == 2)
                {
                    /* TA2 indicates the specific mode of operation */
                    /* If TA2 is not present, negotiable mode is denoted */
                    Smart_p->SpecificMode = TRUE;
                    Smart_p->SpecificType = TAx & 0x0F;
                    Smart_p->SpecificTypeChangable =
                        (TAx & 0x80) ? FALSE : TRUE;
                }
                if ((InterfaceX > 2) && (ModeT == 1))
                {
                    /* TAi (i>2) gives the IFSC for the card */
                    Smart_p->IFSC = TAx;
                    Smart_p->T1Details.TxMaxInfoSize = Smart_p->IFSC;
                }
            }

            if (BitmaskYx & 0x20)
            {
                /* TBx present */
                error = SMART_IO_Rx(Smart_p->IoHandle,
                                    &Smart_p->ATRBytes[i],
                                    1,
                                    &k,
                                    Smart_p->RxTimeout,
                                    NULL,
                                    NULL);
                if (error != ST_NO_ERROR)
                {
                    /* Failed to read TBx */
#if defined(STSMART_DEBUG)
                    STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                                 "Error 0x%08x reading TB%i",
                                 InterfaceX, (U32)error));
#endif
                    error = STSMART_ERROR_RESET_FAILED;
                    break;
                }

                TBx = Smart_p->ATRBytes[i++];
                if (InterfaceX == 1)
                {
                    /* TB1 bit b8 equals 0 */
                    /* TB1 codes II over bits b7 & b6 */
                    Smart_p->IInt = (TBx & 0x60) >> 5;

                    /* TB1 codes PI1 over bits b5 to b1 */
                    Smart_p->PInt1 = (TBx & 0x1F);

                    /* WORK: I and P define the active state at VPP.
                     * This is not being done, as very few cards require
                     * a programming voltage, but if it needs to be here
                     * is where to do it */
                }
                if (InterfaceX == 2)
                {
                    /* TB2 code PI2 over the eight bits */
                    Smart_p->PInt2 = TBx;
                }
                if ((InterfaceX > 2) && (ModeT == 1))
                {
                    /* TBi (i>2) code the CWTint (character waiting time)
                     * and BWTint (block..)
                     */
                    Smart_p->CWInt = TBx & 0x0F;
                    Smart_p->BWInt = (TBx & 0xF0) >> 4;
                }
            }

            if (BitmaskYx & 0x40)
            {
                /* TCx present */
                error = SMART_IO_Rx(Smart_p->IoHandle,
                                    &Smart_p->ATRBytes[i],
                                    1,
                                    &k,
                                    Smart_p->RxTimeout,
                                    NULL,
                                    NULL);
                if (error != ST_NO_ERROR)
                {
                    /* Failed to read TCx */
#if defined(STSMART_DEBUG)
                    STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                                 "Error 0x%08x reading TC%i",
                                 InterfaceX, (U32)error));
#endif
                    error = STSMART_ERROR_RESET_FAILED;
                    break;
                }

                TCx = Smart_p->ATRBytes[i++];
                if (InterfaceX == 1)
                {
                    /* TC1 codes N over the eight bits */

                    /* Set the extra guard time */
                    if (TCx == 0xFF)
                    {
                        MinGuardTime = TRUE;

                        /* We need to reduce the time between two
                         * consecutive start edges to the protocol min.
                         * Unfortunately, we don't yet know what protocol
                         * we're going to be dealing with, and non-ASC4
                         * hardware can't do less than 12-total anyway.
                         * Therefore we default to a guard-time of 12,
                         * and adjust later if required and possible.
                         */
                        Smart_p->N = SMART_N_DEFAULT;
                    }
                    else
                    {
                        /* We add on the default extra guardtime to
                         * ensure that we are on the minimum guardtime
                         * setting recommended by ISO7816/3 i.e., 12 ETU.
                         */
                        Smart_p->N = TCx + SMART_N_DEFAULT;

                        /* Ensure the new value will not overflow the
                         * ASC's 8-bit register.
                         */
                        if ( Smart_p->N < SMART_N_DEFAULT )
                        {
                            Smart_p->N = SMART_N_DEFAULT;
                        }
                    }
                }
                if (InterfaceX == 2)
                {
                    /* TC2 code WI over the eight bits */
                    Smart_p->WInt = TCx;
                }
                if ((InterfaceX > 2) && (ModeT == 1))
                {
                    /* TCi (i>2) specifies the error detection code */
                    /* b1 = 1 use CRC, = 0 use LRC */
                    Smart_p->RC = TCx & 0x01;
                }
            }

            if (BitmaskYx & 0x80)
            {
                /* TDx present */
                error = SMART_IO_Rx(Smart_p->IoHandle,
                                    &Smart_p->ATRBytes[i],
                                    1,
                                    &k,
                                    Smart_p->RxTimeout,
                                    NULL,
                                    NULL);
                if (error != ST_NO_ERROR)
                {
                    /* Failed to read TDx */
#if defined(STSMART_DEBUG)
                    STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                                 "Error 0x%08x reading TD%i",
                                 InterfaceX, (U32)error));
#endif
                    error = STSMART_ERROR_RESET_FAILED;
                    break;
                }

                TDx = Smart_p->ATRBytes[i++];

                /* TDx always codes Yx+1 bitmask and T mode */
                BitmaskYx = (TDx & 0xF0);
                ModeT = (TDx & 0x0F);
            }
            else
            {
                BitmaskYx = 0;


                /* If there is no TDx byte present, then we can only assume
                 * T=0 if it is the first protocol offered.  Otherwise,
                 * we assume a continuation from the previously offered
                 * protocol.
                 */
                if (InterfaceX == 1)
                    ModeT = 0;
            }

            /* Update card's supported protocols */
            Smart_p->SmartParams.SupportedProtocolTypes |= (1 << ModeT);

            /* Set first offered protocol */
            if (InterfaceX == 1)
            {
                FirstT = ModeT;
            }

            /* Process next interface character */
            InterfaceX++;
        }

        /* OK, now that that has been completed, read the historical
         * characters.
         */
        if (error == ST_NO_ERROR)
        {
            /* Read historical  bytes (if any) */
            if (HistoricalK > 0)
            {
                error = SMART_IO_Rx(Smart_p->IoHandle,
                                    &Smart_p->ATRBytes[i],
                                    HistoricalK,
                                    &k,
                                    Smart_p->RxTimeout,
                                    NULL,
                                    NULL);
                if (error == ST_NO_ERROR)
                {
                    /* Copy bytes to internal historical structure */
                    memcpy(Smart_p->HistoricalBytes,
                           &Smart_p->ATRBytes[i],
                           HistoricalK);
                }
                else
                {
                    /* Failed to read all historical bytes */
#if defined(STSMART_DEBUG)
                    STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                                 "Error 0x%08x reading historical bytes",
                                 (U32)error));
#endif
                    error = STSMART_ERROR_RESET_FAILED;
                }
            }

            /* Set size of historical bytes */
            Smart_p->ATRHistoricalSize = HistoricalK;
            i += HistoricalK;

            /* TCK character should also be present only if list of
             * supported protocol types is not t=0 by itself.
             */
            if (Smart_p->SmartParams.SupportedProtocolTypes != 1)
            {
                error = SMART_IO_Rx(Smart_p->IoHandle,
                                    &Smart_p->ATRBytes[i],
                                    1, /* TCK is only one byte */
                                    &k,
                                    Smart_p->RxTimeout,
                                    NULL,
                                    NULL);
                if (error != ST_NO_ERROR)
                {
                    /* TCK byte not present */
#if defined(STSMART_DEBUG)
                    STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                                 "Error 0x%08x reading TCK byte",
                                 (U32)error));
#endif
                    error = STSMART_ERROR_RESET_FAILED;
                }

                i++;                /* Added TCK byte to size */
            }

            if (error == ST_NO_ERROR)
            {
                /* Set ATR message size */
                Smart_p->ATRMsgSize = i;

                /* Set caller's parameters */
                *NumberRead_p = i;
                memcpy(ReadBuffer_p, Smart_p->ATRBytes, i);
            }
        }
    }
    else
    {
        /* Invalid TS character */
        error = STSMART_ERROR_WRONG_TS_CHAR;
#if defined(STSMART_DEBUG)
        STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                     " Received invalid TS char - 0x%02x\n", Smart_p->TS));
#endif
    }

    /* Set the "reset" status to indicate success or failure */
    Smart_p->CardReset = (error != ST_NO_ERROR) ? STSMART_ERROR_RESET_FAILED :
                         ST_NO_ERROR;

    /* Configure new interface parameters */
    if (error == ST_NO_ERROR)
    {
        /* Set working mode */
        if (Smart_p->SpecificMode == TRUE)
        {
            /* Choose specific mode */
            Smart_p->WorkingType = Smart_p->SpecificType;
        }
        else
        {
            /* Choose first offered protocol */
            Smart_p->WorkingType = FirstT;

            /* Use default FInt and Dint if card hasn't specified them */
            if (TA1Present == FALSE || Smart_p->SpecificMode == FALSE)
            {
                Smart_p->FInt = SMART_F_DEFAULT;
                Smart_p->DInt = SMART_D_DEFAULT;
            }
        }

        /* Set default retries iff T0 */
        if (Smart_p->WorkingType == 0)
        {
            Smart_p->Retries = T0_RETRIES_DEFAULT;
        }
        else
        {
            Smart_p->Retries = 0; /* Zero otherwise */
        }

#if defined(ASC4_PRESENT)
        /* If we're going to be using T=1, and the card requested minimum
         * delay time, *and* the hardware can support it, then go to
         * 11etu total-guard-time (1bit extra GT).
         */
        if ((Smart_p->WorkingType == STSMART_T1_PROTOCOL) &&
            (MinGuardTime == TRUE))
        {
            Smart_p->N = SMART_N_DEFAULT - 1;
        }
#endif

        /* Calculate work etu */
        SMART_SetETU(Smart_p);

        /* Calculate baudrate from WorkETU and Fs */
        Smart_p->SmartParams.BaudRate = Smart_p->Fs / Smart_p->WorkETU;

        /* Invoke parameters */
        SMART_SetParams(Smart_p);
    }
    return error;
} /* SMART_AnswerToReset() */

static ST_ErrorCode_t SMART_ProtocolSelect(SMART_ControlBlock_t *Smart_p,
                                           U8 *PtsBytes_p
                                          )
{
    ST_ErrorCode_t Error;
    U8 PtsTxBuf[SMART_MAX_PTS], P, PTSx, PTS0, PCK = 0, FInt = 0, DInt = 0, ModeT = 0;
    U32 i, Index = 0, sz, NumberWritten;
    BOOL PTSReplyOk = FALSE;

    /* Build the PTS command */
    Index = 0;
    PtsTxBuf[Index++] = PTS_INITIAL_CHAR; /* PTSS */
    PtsTxBuf[Index++] = PtsBytes_p[0];  /* PTS0 */
    if ((PtsBytes_p[0] & 0x10) != 0)
    {
        PtsTxBuf[Index++] = PtsBytes_p[1]; /* PTS1 (optional) */
    }
    if ((PtsBytes_p[0] & 0x20) != 0)
    {
        PtsTxBuf[Index++] = PtsBytes_p[2]; /* PTS2 (optional) */
    }
    if ((PtsBytes_p[0] & 0x40) != 0)
    {
        PtsTxBuf[Index++] = PtsBytes_p[3]; /* PTS3 (optional) */
    }

    /* Compute the PCK check byte -- should be XOR of all other bytes */
    for (i = 0; i < Index; i++)
    {
        PCK ^= PtsTxBuf[i];
    }

    /* Append check byte to PTS */
    PtsTxBuf[Index] = PCK;     /* PCK */

    /* Flush FIFOs first */
    SMART_IO_Flush(Smart_p->IoHandle);

    Error = SMART_IO_Tx(Smart_p->IoHandle,
                        PtsTxBuf,
                        Index+1,                /* PTSS sent first */
                        &NumberWritten,
                        Smart_p->TxTimeout,
                        NULL,
                        NULL);

    if (Error == ST_NO_ERROR)
    {
        Error = SMART_IO_Rx(Smart_p->IoHandle,
                            &P,
                            1,
                            &sz,
                            Smart_p->RxTimeout,
                            NULL,
                            NULL
                           );
    }

    /* Ensure PTSS is confirmed correctly */
    if (Error == ST_NO_ERROR && P == PTS_INITIAL_CHAR)
    {
        /* Now process subsequent PTS response bytes */
        Index = 1;                      /* Index of PTS buffer */
        PTSx = 0;                       /* PTS char for x = 0, 1, 2 or 3 */
        PTS0 = PtsTxBuf[Index]; /* PTS0 used for PTSx presence */

        /* Process each PTS byte (NB: PTSx = 4 ==> PCK check byte) */
        while (PTSx <= 4 && Error == ST_NO_ERROR)
        {
            /* Proceed only if next PTS byte is to be transmitted */
            if ( ((0x08 << PTSx) & PTS0) != 0 || /* PTSx present */
                 PTSx == 0 ||       /* PTS0 */
                 PTSx == 4 )        /* PCK */
            {
                /* Read PTSx response from card */
                Error = SMART_IO_Rx(Smart_p->IoHandle,
                                    &P,
                                    1,
                                    &sz,
                                    Smart_p->RxTimeout,
                                    NULL,
                                    NULL
                                   );

                /* Check that the byte was echoed back successfully */
                if ( Error == ST_NO_ERROR && P == PtsTxBuf[Index] )
                {
                    PTSReplyOk = TRUE; /* PTS reply is good */
                }
                else
                {
                    PTSReplyOk = FALSE; /* PTS reply might be invalid */
                }

                /* If okay, store PTS response (not PCK) */
                if ((PTSReplyOk == TRUE) && (PTSx < 4))
                {
                    Smart_p->PtsBytes[PTSx] = P;
                }

                /* Process according to which PTS byte was sent */
                switch (PTSx)
                {
                    case 0:         /* PTS0 */
                        /* PTS0 should be echoed back:  Note that bits
                         * 0-3 encode a new protocol type, so we may
                         * need to change the driver protocol too.
                         */
                        if (PTSReplyOk)
                        {
                            /* Invoke new protocol type */
                            ModeT = (P & 0x0F);
                        }
                        else
                        {
                            /* PTS0 ack failed */
                            Error = STSMART_ERROR_IN_PTS_CONFIRM;
                        }
                        break;
                    case 1:         /* PTS1 */
                        /* PTS1 encodes new FI and D parameters.  The card
                         * will echo these back.  If it doesn't this is not
                         * an error, but the defaults are retained.
                         */
                        if (PTSReplyOk)
                        {
                            /* Invoke new FI and D */
                            FInt = ( P & 0xF0 ) >> 4;
                            DInt = ( P & 0x0F );
                        }
                        else
                        {
                            /* Default will be used */
                            Error = ST_NO_ERROR;
                        }
                        break;
                    case 2:         /* PTS2 */
                        /* PTS2 encodes '11 etu support' and a request for
                         * '12 etu extra guard time'.
                         *  The card will echo this byte back.  If it doesn't
                         * then this is an error.
                         */
                        /* Note: Cannot find a source for this information
                         * in ISO7816-3; according to that, PPS2 & 3 are
                         * reserved for future use. Where did Liam get this?
                         * -- PW
                         */
                        if (PTSReplyOk)
                        {
                            /* Invoke '11 etu support' or '12 etu guard time' */
                            if ( ( P & 0x01 ) != 0 ) /* 11 etu? */
                            {
                                Smart_p->N = SMART_N_DEFAULT - 1;
                            }
                            else if ( ( P & 0x02 ) != 0 ) /* N = 12? */
                            {
                                Smart_p->N = SMART_N_DEFAULT + 12;
                            }
                        }
                        else
                        {
                            /* PTS2 ack failed */
                            Error = STSMART_ERROR_IN_PTS_CONFIRM;
                        }
                        break;
                    case 3:         /* PTS3 */
                        /* PTS3 is not defined by the standard and therefore
                         * there is no action required by the driver in
                         * response to PTS3.
                         */
                        Error = ST_NO_ERROR;
                        break;
                    case 4:         /* PCK byte */
                        if (!PTSReplyOk)
                        {
                            /* PCK checksum failed */
                            Error = STSMART_ERROR_IN_PTS_CONFIRM;
                        }
                        break;
                    default:
                        /* Invalid PTS index - ignore the byte and abort! */
                        Error = STSMART_ERROR_PTS_NOT_SUPPORTED;
                        break;
                }

                /* Increment PTS buffer index */
                Index++;
            }

            /* Process next PTS byte */
            PTSx++;
        }
    }
    else
    {
        /* Incorrect PTS confirm byte received */
        Error = STSMART_ERROR_IN_PTS_CONFIRM;
    }

    /* Only invoke new parameters if the PTS succeeded */
    if (Error == ST_NO_ERROR)
    {
        /* Set new FI and DI */
        Smart_p->DInt = DInt;
        Smart_p->FInt = FInt;

        /* Set new working mode */
        Smart_p->WorkingType = ModeT;

        /* Calculate new ETU from FI and DI */
        SMART_SetETU(Smart_p);

        /* Calculate baudrate from WorkETU and Fs */
        Smart_p->SmartParams.BaudRate = Smart_p->Fs / Smart_p->WorkETU;

        /* Invoke new parameters */
        SMART_SetParams(Smart_p);
    }

    return Error;
}

/*****************************************************************************
SMART_GetControlBlockFromPio()
Description:
    Runs through the queue of initialized smartcard devices and checks for
    the smartcard with the specified PIO handle.
Parameters:
    Handle, an open handle.
Return Value:
    NULL, end of queue encountered - invalid handle.
    Otherwise, returns a pointer to the control block for the device.
See Also:
    Nothing.
*****************************************************************************/

static SMART_ControlBlock_t *
   SMART_GetControlBlockFromPio(STPIO_Handle_t Handle)
{
    SMART_ControlBlock_t *qp = SmartQueueHead_p; /* Global queue head pointer */
    U32 Index = 0;

    /* Iterate over the smartcard queue */
    while (qp != NULL)
    {
        if (Handle == qp->ResetHandle)
        {
            break;
        }

    if (qp->IsInternalSmartcard == FALSE)
    {
        /* Check for a valid matching handle */
        if (Handle == qp->DetectHandle)
        {
            break;
        }
    }

        /* Try the next smartcard */
        qp = qp->Next_p;
        Index++;
    }

    /* Return the smartcard control block (or NULL) to the caller */
    return qp;
} /* SMART_GetControlBlockFromPio() */

/* End of scdrv.c */

