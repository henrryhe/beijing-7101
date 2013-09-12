/*****************************************************************************
File Name   : scapi.c

Description : STSMART driver API.

    This implementation of the driver only supports T=0 ISO-7816 compliant
    smartcards.

Copyright (C) 2006 STMicroelectronics

History     :

    29/09/99    Implemented switch debouncing.
    04/10/99    Fixed problems with ETU / WWT calculation.
    10/11/99    Bug fixes to processing of procedure bytes in accordance
                with ISO7816/3.
                Bug fix to computation of receive timeouts - pre-multiplies
                by 10 to reduce rounding errors.
    02/03/00    Initiate atr function was not flushing the UART prior
                to performing the reset.  Precautionary flush call added.
    03/03/00    Fixed problem with process procedure bytes -- as per
                DDTS GNBvd03352.
                Added support for 'too many retries' error code.
    21/03/00    Added support for raw mode API extensions.
                Added event handler support.
    10/04/00    Initialization code now allows ClkGenExtClk to be
                unspecified -- required for STi5508/18.
    30/05/00    All stuart communications have been moved to the scio.c
                module.  This simplifies all smartcard communications
                routines and avoids much code duplication in the driver.
    04/04/01    SetProtocol() no longer checks SpecificTypeChangable.
                This is because a PTS may be submitted even if the protocol
                type is not changable e.g., change of work etu for
                negotiable mode cards.
    [...]
    17/03/03    Adding STSMART_SetClockFrequency()
    30/06/04    In STSMART_SetClockFrequency (), the frequency can be
                changed when the driver is not reset and in reset
                the default frequency will be taken only when it is
                not configured with STSMART_SetClockFrequency()

Reference   :

ST API Definition "SMARTCARD Driver API" DVD-API-11 v3.13
*****************************************************************************/

/* Includes --------------------------------------------------------------- */
#if defined(ST_OS20) || defined(ST_OS21)
#include <string.h>                     /* C libs */
#include <stdio.h>
#endif

#if defined( ST_OSLINUX )
#include <linux/stpio.h>
#include "linux/kthread.h"
#else
#include "stlite.h"
#endif

#include "sttbx.h"
#include "stsys.h"

#include "stos.h"                       /* Includes compat.h for LINUX */
#include "stcommon.h"
#include "stddefs.h"                    /* Explicitly include this */
#include "stevt.h"

#include "stsmart.h"                    /* STAPI includes */

#include "scregs.h"                     /* Local includes */
#include "scio.h"
#include "scdat.h"
#include "scdrv.h"
#include "sct0.h"
#include "sct1.h"


/* Private types/constants ------------------------------------------------ */

/* Private variables ------------------------------------------------------ */


/*****************************************************************************
The SmartQueueHead_p points to the head of the queue of smartcard devices
that have been initialized i.e., STSMART_Init() has been called to append
them to the queue.
At system startup, SmartQueueHead_p will be NULL.  When the first smartcard
controller is initialized, SmartQueueHead_p will point to the control block
for that smartcard, once STSMART_Init() has been called.
*****************************************************************************/

/* Queue of initialized Smartcards */
SMART_ControlBlock_t                        *SmartQueueHead_p = NULL;
static semaphore_t                          *SmartQueueSemaphore_p;
/* So that we only init the queue semaphore once */
static BOOL                                 FirstInit = TRUE;

/* Private macros --------------------------------------------------------- */

/* Private function prototypes -------------------------------------------- */

static SMART_ControlBlock_t *SMART_GetControlBlockFromHandle(
                                            STSMART_Handle_t Handle);
static SMART_ControlBlock_t *SMART_GetControlBlockFromName(
                                            const ST_DeviceName_t DeviceName);
static BOOL SMART_IsInit(const ST_DeviceName_t DeviceName);
static void SMART_QueueRemove(SMART_ControlBlock_t *QueueItem_p);
static void SMART_QueueAdd(SMART_ControlBlock_t *QueueItem_p);
static ST_ErrorCode_t SMART_CheckCardReady(STSMART_Handle_t Handle,
                                           SMART_ControlBlock_t *Smart_p);
static ST_ErrorCode_t SMART_RegisterEvents(SMART_ControlBlock_t *Smart_p);
static ST_ErrorCode_t SMART_UnregisterEvents(SMART_ControlBlock_t *Smart_p);

/* STSMART API routines (alphabetical order) ------------------------------ */

/*****************************************************************************
STSMART_Abort()
Description:
    Aborts any pending IO operations in the driver and blocks until the
    operation has been aborted.
Parameters:
    Handle,             client handle for the smartcard device.
Return Value:
    ST_NO_ERROR,                the operation completed without error.
    ST_ERROR_INVALID_HANDLE,    the handle is invalid.
    ST_ERROR_DEVICE_BUSY,       the device is busy (card is locked).
    STSMART_ERROR_NOT_INSERTED,       no card is inserted.
    STSMART_ERROR_NOT_RESET,          the card has not been reset.
    STSMART_ERROR_RESET_FAILED,       the last reset operation failed.
See Also:
    Nothing.
*****************************************************************************/

ST_ErrorCode_t STSMART_Abort(STSMART_Handle_t Handle)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    SMART_ControlBlock_t *Smart_p;
    BOOL Locked = FALSE;

    /* Obtain the smartcard controlblock from the passed in handle */
    Smart_p = SMART_GetControlBlockFromHandle(Handle);

    if (Smart_p != NULL)
    {
        /* Check the card ready for use */
        error = SMART_CheckCardReady(Handle, Smart_p);
        if (error == ST_NO_ERROR)
        {
            /* Abort any pending IO operations with the card.  This
             * is easily done by simply aborting all IO with the
             * UART.  The completion routines of the smartcard driver
             * will take care of the rest.
             */
            STOS_SemaphoreWait(Smart_p->ControlLock_p);
            Smart_p->IoAbort = TRUE;
            Locked = TRUE;

            /* If an operation is in progress, busy wait on IoAbort :
             * the driver IO task will set this to false when the IO
             * has successfully aborted.
             */
            while ((Smart_p->IoAbort) &&
                   (Smart_p->IoOperation.Context != SMART_CONTEXT_NONE))
            {
                /* Now that we're sure this is a pre-existing operation, and
                 * not one that's been started since we've been called,
                 * relesae the semaphore.
                 */
                Locked = FALSE;
                STOS_SemaphoreSignal(Smart_p->ControlLock_p);

                /* Will force a deschedule */
#ifdef ST_OSLINUX
                STOS_TaskDelay(0);
#else
                STOS_TaskSchedule();
#endif
                /* We need to keep aborting in case the transfer has
                 * not yet started...
                 */
                SMART_IO_Abort(Smart_p->IoHandle);
            }

            /* If we haven't released yet (ie no operation was in progress),
             * do it now.
             */
            if (Locked == TRUE)
            {
                STOS_SemaphoreSignal(Smart_p->ControlLock_p);
            }
        }
    }
    else
    {
        /* The handle is invalid */
        error = ST_ERROR_INVALID_HANDLE;
    }

    return error;
} /* STSMART_Abort() */

/*****************************************************************************
STSMART_Close()
Description:
    Closes an open connection on a smartcard device.
Parameters:
    Handle,     client handle for the smartcard device.
Return Value:
    ST_NO_ERROR,                the operation completed without error.
    ST_ERROR_INVALID_HANDLE,    the handle is invalid.
See Also:
    STSMART_Open()
*****************************************************************************/

ST_ErrorCode_t STSMART_Close(STSMART_Handle_t Handle)
{
    ST_ErrorCode_t error = ST_NO_ERROR;

    SMART_ControlBlock_t *Smart_p;

    /* Obtain the smartcard controlblock from the passed in handle */
    Smart_p = SMART_GetControlBlockFromHandle(Handle);

    if (Smart_p != NULL)
    {
        /* Obtain the control lock for removing this handle */
        STOS_SemaphoreWait(Smart_p->ControlLock_p);

        /* Check this caller hasn't been stupid enough to close whilst
         * holding the smartcard lock!
         */
        if (Smart_p->CardLock.Owner != 0 && Handle == Smart_p->CardLock.Owner)
        {
            /* We must free the lock before closing their handle */
            Smart_p->CardLock.Owner = 0;

            /* Signal the semaphore to unblock a waiting client */
            STOS_SemaphoreSignal(Smart_p->CardLock.Semaphore_p);
        }

        /* Abort any pending IO operations */
        Smart_p->IoAbort = TRUE;
        SMART_IO_Abort(Smart_p->IoHandle);

        /* Free the handle from the open handle list */
        ((SMART_OpenBlock_t *)Handle)->ControlBlock = NULL;
        ((SMART_OpenBlock_t *)Handle)->InUse = FALSE;
        Smart_p->FreeHandles++;

        STOS_SemaphoreSignal(Smart_p->ControlLock_p);
    }
    else
    {
        /* The handle is invalid */
        error = ST_ERROR_INVALID_HANDLE;
    }

    return error;
} /* STSMART_Close() */

/*****************************************************************************
STSMART_DataAvailable()
Description:
    If an operation is in progress, return the number of bytes so far read.
    If no operation is in progress, returns zero.
Parameters:
    Handle,             client handle for the smartcard device.
    Size_p              pointer to where the bytecount should be stored
Return Value:
    ST_NO_ERROR,                the operation completed without error.
    ST_ERROR_INVALID_HANDLE,    the handle is invalid.
See Also:
    STSMART_RawRead()
    STSMART_Transfer()
*****************************************************************************/
ST_ErrorCode_t STSMART_DataAvailable(STSMART_Handle_t Handle,
                                     U32              *Size_p)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    SMART_ControlBlock_t *Smart_p;

    /* Obtain the smartcard controlblock from the passed in handle */
    Smart_p = SMART_GetControlBlockFromHandle(Handle);

    if (Smart_p != NULL)
    {
        /* Check the operation is in use */
        if (Smart_p->IoOperation.Context != SMART_CONTEXT_NONE)
        {
            *Size_p = *Smart_p->IoOperation.ReadBytes_p;
        }
        else
        {
            *Size_p = 0;
        }
    }
    else
    {
        error = ST_ERROR_INVALID_HANDLE;
    }

    return error;
}

/*****************************************************************************
STSMART_Deactivate()
Description:
    Disconnects the smartcard's contacts and Vcc power is removed.

    NOTE: The card must be reset in order to re-establish Vcc.

Parameters:
    Handle,     client handle for the smartcard device.
Return Value:
    ST_NO_ERROR,                the operation completed without error.
    ST_ERROR_INVALID_HANDLE,    the handle is invalid.
    ST_ERROR_DEVICE_BUSY,       the device is busy (card is locked).
    STSMART_ERROR_NOT_INSERTED,       no card is inserted.
See Also:
    STSMART_Reset()
*****************************************************************************/

ST_ErrorCode_t STSMART_Deactivate(STSMART_Handle_t Handle)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    SMART_ControlBlock_t *Smart_p;

    /* Obtain the smartcard controlblock from the passed in handle */
    Smart_p = SMART_GetControlBlockFromHandle(Handle);

    if (Smart_p != NULL)
    {
        /* Check the card is ready for deactivation */
        error = SMART_CheckCardReady(Handle, Smart_p);

        if (error != STSMART_ERROR_NOT_INSERTED && error != ST_ERROR_DEVICE_BUSY)
        {
            SMART_Message_t *Msg_p;

            /* Make sure error is ST_NO_ERROR, since at this point we
             * will complete the operation (although technically the
             * message queue could fail).
             */
            error = ST_NO_ERROR;

            /* Deactivate the card's contacts */
            SMART_Deactivate(Smart_p);

            /* Now generate an event to "broadcast" to clients */
            Msg_p = (SMART_Message_t *)STOS_MessageQueueClaimTimeout(Smart_p->MessageQueue_p,
                                                             TIMEOUT_IMMEDIATE);

            /* Check we claimed a message buffer */
            if (Msg_p != NULL)
            {
                /* Setup the smartcard event to be generated */
                Msg_p->MessageType = SMART_EVENT;
                Msg_p->MessageParams_u.EventParams_s.Smart_p = Smart_p;
                Msg_p->MessageParams_u.EventParams_s.Broadcast = TRUE;
                Msg_p->MessageParams_u.EventParams_s.Handle = Handle;
                Msg_p->MessageParams_u.EventParams_s.Event = STSMART_EV_CARD_DEACTIVATED;
                Msg_p->MessageParams_u.EventParams_s.Error = ST_NO_ERROR;

                /* Now send the message to the queue */
                STOS_MessageQueueSend(Smart_p->MessageQueue_p, Msg_p);
            }
        }
    }
    else
    {
        /* The handle is invalid */
        error = ST_ERROR_INVALID_HANDLE;
    }

    return error;
} /* STSMART_Deactivate() */

/*****************************************************************************
STSMART_GetCapability()
Description:
    After a stsmart device has been initialized it is possible to interrogate
    the capabilities of that device with a call to this function.
Parameters:
    DeviceName,     the name of device registered at init time.
    Capability_p,   pointer to capability structure.
Return Value:
    ST_NO_ERROR,                the operation completed without error.
    ST_ERROR_UNKNOWN_DEVICE,    the device is not recognised.
See Also:
    STSMART_Init()
*****************************************************************************/

ST_ErrorCode_t STSMART_GetCapability(const ST_DeviceName_t DeviceName,
                                     STSMART_Capability_t *Capability_p)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    SMART_ControlBlock_t *Smart_p;

    /* Obtain the UART control block associated with the device name */
    Smart_p = SMART_GetControlBlockFromName(DeviceName);

    /* Only continue if we have a control block for this device */
    if (Smart_p != NULL)
    {
        /* Store device capability */
        *Capability_p = Smart_p->Capability;
    }
    else
    {
        /* Unknown device name */
        error = ST_ERROR_UNKNOWN_DEVICE;
    }

    return error;

} /* STSMART_GetCapability() */

/*****************************************************************************
STSMART_GetPtsBytes()
Description:
    Returns the bytes sent by the card from the last PTS negotiation
    (STSMART_SetProtocol). Contents undefined if no negotiation has taken
    place.
Parameters:
    Handle              The device handle
    PtsBytes[4]         The PTS bytes returned (PCK is stripped)
Return Value:
    ST_NO_ERROR,                the operation completed without error.
    ST_ERROR_INVALID_HANDLE,    the device is not recognised.
See Also:
    STSMART_SetProtocol()
*****************************************************************************/
ST_ErrorCode_t STSMART_GetPtsBytes(STSMART_Handle_t Handle,
                                   U8 PtsBytes[4])
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    SMART_ControlBlock_t *Smart_p = NULL;

    /* Obtain the smartcard controlblock from the passed in handle */
    Smart_p = SMART_GetControlBlockFromHandle(Handle);

    /* Only continue if we have a control block for this device */
    if (Smart_p != NULL)
    {
        /* Copy the bytes */
        STOS_memcpy(PtsBytes, Smart_p->PtsBytes, 4);
    }
    else
    {
        /* Bad handle */
        error = ST_ERROR_INVALID_HANDLE;
    }

    return error;

} /* STSMART_GetPtsBytes() */

/*****************************************************************************
STSMART_Flush()
Description:
    Flushes the internal FIFO of the receiver device.
Parameters:
    Handle,             client handle for the smartcard device.
Return Value:
    ST_NO_ERROR,                the operation completed without error.
    ST_ERROR_INVALID_HANDLE,    the handle is invalid.
    ST_ERROR_DEVICE_BUSY,       the device is busy (card is locked).
See Also:
    Nothing.
*****************************************************************************/

ST_ErrorCode_t STSMART_Flush(STSMART_Handle_t Handle)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    SMART_ControlBlock_t *Smart_p;

    /* Obtain the smartcard controlblock from the passed in handle */
    Smart_p = SMART_GetControlBlockFromHandle(Handle);

    if (Smart_p != NULL)
    {
        /* Check the card ready for use */
        error = SMART_CheckCardReady(Handle, Smart_p);
        if (error != ST_ERROR_DEVICE_BUSY)
        {
            /* Ensure no operations are in progress before we flush */
            STOS_SemaphoreWait(Smart_p->ControlLock_p);

            /* Check this operation is not already in use */
            if (Smart_p->IoOperation.Context == SMART_CONTEXT_NONE)
            {
                /* Flush the internal FIFOs of the driver */
                SMART_IO_Flush(Smart_p->IoHandle);
            }
            else
            {
                /* Device is busy -- can't flush whilst IO is in progress */
                error = ST_ERROR_DEVICE_BUSY;
            }

            /* Release access semaphore */
            STOS_SemaphoreSignal(Smart_p->ControlLock_p);
        }
    }
    else
    {
        /* The handle is invalid */
        error = ST_ERROR_INVALID_HANDLE;
    }

    return error;
} /* STSMART_Flush() */

/*****************************************************************************
STSMART_GetHistory()
Description:
    The history bytes of the last ATR are retrieved and returned to the
    caller.  The card must have been reset.
Parameters:
    Handle,             client handle for the smartcard device.
    History,            array of 32 bytes for storing the history bytes.
    HistoryLength_p,    update with number of history bytes in array.
Return Value:
    ST_NO_ERROR,                the operation completed without error.
    ST_ERROR_INVALID_HANDLE,    the handle is invalid.
    STSMART_ERROR_NOT_INSERTED,       no card is inserted.
    STSMART_ERROR_NOT_RESET,          the card has not been reset.
    STSMART_ERROR_RESET_FAILED,       the last reset operation failed.
See Also:
    Nothing.
*****************************************************************************/

ST_ErrorCode_t STSMART_GetHistory(STSMART_Handle_t Handle,
                                  U8 History[15],
                                  U32 *HistoryLength_p)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    SMART_ControlBlock_t *Smart_p;

    /* Obtain the smartcard controlblock from the passed in handle */
    Smart_p = SMART_GetControlBlockFromHandle(Handle);

    if (Smart_p != NULL)
    {
        /* Check the card ready for use */
        error = SMART_CheckCardReady(Handle, Smart_p);
        if (error == ST_NO_ERROR)
        {
            /* Copy the historical bytes to the caller */
            STOS_memcpy(History, Smart_p->HistoricalBytes,
                   Smart_p->ATRHistoricalSize);
            *HistoryLength_p = Smart_p->ATRHistoricalSize;
        }
    }
    else
    {
        /* The handle is invalid */
        error = ST_ERROR_INVALID_HANDLE;
    }

    return error;
} /* STSMART_GetHistory() */

/*****************************************************************************
STSMART_GetParams()
Description:
    Obtains the current operational parameters of a smartcard.
    This function may only be called after a card has been reset.
Parameters:
    Handle,             client handle for the smartcard device.
    Params_p,           pointer to smartcard params structure.
Return Value:
    ST_NO_ERROR,                the operation completed without error.
    ST_ERROR_INVALID_HANDLE,    the handle is invalid.
    STSMART_ERROR_NOT_INSERTED,       no card is inserted.
    STSMART_ERROR_NOT_RESET,          the card has not been reset.
    STSMART_ERROR_RESET_FAILED,       the last reset operation failed.
See Also:
    Nothing.
*****************************************************************************/

ST_ErrorCode_t STSMART_GetParams(STSMART_Handle_t Handle,
                                 STSMART_Params_t *Params_p)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    SMART_ControlBlock_t *Smart_p;

    /* Obtain the smartcard controlblock from the passed in handle */
    Smart_p = SMART_GetControlBlockFromHandle(Handle);

    if (Smart_p != NULL)
    {
        /* Check the card ready for use */
        error = SMART_CheckCardReady(Handle, Smart_p);
        if (error == ST_NO_ERROR)
        {
            /* Copy device parameters */
            *Params_p = Smart_p->SmartParams;
        }
    }
    else
    {
        /* The handle is invalid */
        error = ST_ERROR_INVALID_HANDLE;
    }

    return error;
} /* STSMART_GetHistory() */

/*****************************************************************************
STSMART_GetRevision()
Description:
    Obtains the revision number of the driver.
Parameters:
    None.
Return Value:
    The revision of the device driver.
See Also:
    Nothing.
*****************************************************************************/

ST_Revision_t STSMART_GetRevision(void)
{
    /* Simply return the global revision string */
    return STSMART_REVISION;
} /* STSMART_GetRevision() */

/***********************************************************************
STSMART_GetRxTimeout()
Description:
    Obtains RxTimeout
Parameters:
    Handle,             client handle for the smartcard device.
    Timeout,            Timeout returned
Return Value:
    ST_NO_ERROR,                the operation completed without error.
    ST_ERROR_INVALID_HANDLE,    the handle is invalid.
See Also:
    STSMART_SetRxTimeout.
***********************************************************************/

ST_ErrorCode_t STSMART_GetRxTimeout(STSMART_Handle_t Handle, U32 *Timeout_p)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    SMART_ControlBlock_t *Smart_p = NULL;

    /* Obtain the smartcard controlblock from the passed in handle */
    Smart_p = SMART_GetControlBlockFromHandle(Handle);

    if (Smart_p != NULL)
    {
        *Timeout_p = Smart_p->RxTimeout;
    }
    else
    {
        /* The handle is invalid */
        error = ST_ERROR_INVALID_HANDLE;
    }
    return error;

}/* STSMART_GetRxTimeout */

/*****************************************************************************
STSMART_GetStatus()
Description:
    Returns status parameters of the last operation performed on a
    smartcard device.  This contains the errorcode and procedure bytes of
    the last smartcard response.
Parameters:
    Handle,             client handle for the smartcard device.
    Status_p,           updated with the status params.
Return Value:
    ST_NO_ERROR,                the operation completed without error.
    ST_ERROR_INVALID_HANDLE,    the handle is invalid.
    STSMART_ERROR_NOT_INSERTED,       no card is inserted.
    STSMART_ERROR_NOT_RESET,          the card has not been reset.
    STSMART_ERROR_RESET_FAILED,       the last reset operation failed.
    STSMART_ERROR_NO_ANSWER,          the card failed to respond to the last
                                command sent to it.
See Also:
    Nothing.
*****************************************************************************/

ST_ErrorCode_t STSMART_GetStatus(STSMART_Handle_t Handle,
                                 STSMART_Status_t *Status_p)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    SMART_ControlBlock_t *Smart_p;

    /* Obtain the smartcard controlblock from the passed in handle */
    Smart_p = SMART_GetControlBlockFromHandle(Handle);

    if (Smart_p != NULL)
    {
        /* Check the card ready for use */
        error = SMART_CheckCardReady(Handle, Smart_p);
        if (error == ST_NO_ERROR)
        {
            /* Copy status to caller */
            *Status_p = *Smart_p->Status_p;
        }
    }
    else
    {
        /* The handle is invalid */
        error = ST_ERROR_INVALID_HANDLE;
    }

    return error;
} /* STSMART_GetStatus() */

/*****************************************************************************
STSMART_Init()
Description:
    Performs initialization of a smartcard device; memory allocation, PIO
    setup, UART setup and default parameters in the smartcard control block.

    NOTE: On the first invocation of this routine, the global
          SmartQueueSemaphore is initialized.
Parameters:
    DeviceName,     name of the smartcard device.
    InitParams_p,   pointer to caller-supplied initialization params.
Return Value:
    ST_NO_ERROR,                     the operation completed without error.
    ST_ERROR_FEATURE_NOT_SUPPORTED,  the device type is not supported.
    ST_ERROR_ALREADY_INITALIZED,     the device has already been initialized.
    ST_ERROR_BAD_PARAMETER,          one or more params was invalid.
    ST_ERROR_NO_MEMORY,              a memory allocation failed.
    ST_ERROR_UNKNOWN_DEVICE,         the PIO/UART device is not recognised.
    ST_ERROR_DEVICE_BUSY,            unable to claim one or more PIO pins.
See Also:
    STSMART_Term()
*****************************************************************************/

ST_ErrorCode_t STSMART_Init( ST_DeviceName_t DeviceName,
                            const STSMART_InitParams_t *InitParams_p)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    BOOL SemaphoreInit = FALSE;
    BOOL SmartIoInit = FALSE;
    partition_t *DriverPartition;

    /* If this is the first init call, initialise the semaphore. Check and
     * init have to be atomic, to avoid races, hence interrupt_lock().
    */

    STOS_TaskLock();

    if (TRUE == FirstInit)
    {
        FirstInit = FALSE;
        SmartQueueSemaphore_p = STOS_SemaphoreCreateFifo(NULL, 1);
    }

    STOS_TaskUnlock();

    /* Ensure mutually exclusive access to the UART queue */
    STOS_SemaphoreWait(SmartQueueSemaphore_p);

    /* Check for bad device type */
    if ((InitParams_p->DeviceType < STSMART_ISO) ||
        (InitParams_p->DeviceType > STSMART_ISO_NDS))
    {
        error = ST_ERROR_BAD_PARAMETER;
    }

    /* Check for other bad parameters */
    if (error == ST_NO_ERROR &&
        strlen(DeviceName) <= ST_MAX_DEVICE_NAME &&
        InitParams_p->MaxHandles <= SMART_MAX_HANDLES &&
        InitParams_p->MaxHandles > 0 &&
        InitParams_p->ClockFrequency > 0 &&
        InitParams_p->ClockSource >= STSMART_CPU_CLOCK &&
        InitParams_p->ClockSource <= STSMART_EXTERNAL_CLOCK &&
        InitParams_p->BaseAddress != NULL)
    {
        /* Run through the queue to ensure this device is free */
        if (!SMART_IsInit(DeviceName))
        {
            SMART_ControlBlock_t *Smart_p;

            /* Allocate memory for the new queue item */
            DriverPartition = (partition_t *)InitParams_p->DriverPartition;

            Smart_p = (SMART_ControlBlock_t *)
                      STOS_MemoryAllocate(DriverPartition,
                                      sizeof(SMART_ControlBlock_t));

            /* Ensure memory allocation was successful */
            if (Smart_p != NULL)
            {

                U8 i;
                STPIO_OpenParams_t ClkGenExtClk, Clk, Reset, CmdVcc, CmdVpp,Detect;

                /* Make sure structure is initialised to zero, for anything
                 * we don't explicitly set below.
                 */
                memset(Smart_p, 0, sizeof(SMART_ControlBlock_t));

                /* Copy initialization parameters */
                Smart_p->InitParams = *InitParams_p;

#ifdef ST_OSLINUX
                /* Remap base address under Linux */
                Smart_p->SmartMappingWidth = STSMART_MAPPING_WIDTH;
                Smart_p->MappingBaseAddress = (unsigned long *)STLINUX_MapRegion( (void *)Smart_p->InitParams.BaseAddress,
                                                                          Smart_p->SmartMappingWidth,"SMARTMEM" );
                if (Smart_p->MappingBaseAddress == NULL)
                {
                    return ST_ERROR_NO_MEMORY;
                }
#endif


                /* Zero out the control block handles */
                Smart_p->ClkGenExtClkHandle = 0;
                Smart_p->ClkHandle = 0;
                Smart_p->ResetHandle = 0;
                Smart_p->CmdVccHandle = 0;
                Smart_p->CmdVppHandle = 0;
                Smart_p->DetectHandle = 0;
                /* Set status back pointer */
                Smart_p->Status_p = &Smart_p->SmartParams.CardStatus;

                /* Set device capabilities */
                Smart_p->Capability.DeviceType = InitParams_p->DeviceType;
                Smart_p->Capability.SupportedISOProtocols =
                    (1 << STSMART_T0_PROTOCOL) |
                    (1 << STSMART_T1_PROTOCOL);

                /* Copy the device name for this device */
                strncpy(Smart_p->DeviceName,
                        DeviceName,
                        ST_MAX_DEVICE_NAME);

                /* We're not locked at present */
                Smart_p->CardLock.Owner = 0;

                /* Default to normal clock behaviour */
                Smart_p->ResetFrequency = 0;

                /* Is smartcard internal ? */
                if (InitParams_p->IsInternalSmartcard == TRUE)
                {
                    Smart_p->IsInternalSmartcard = TRUE;
                }
                else
                {
                    Smart_p->IsInternalSmartcard = FALSE;
                }

                /* Configure each of the open parameters */
                i = SMART_PIOBitFromBitMask(InitParams_p->ClkGenExtClk.BitMask); /* Ext clk (IN) */
                ClkGenExtClk.BitConfigure[i] = STPIO_BIT_INPUT;
                ClkGenExtClk.ReservedBits = InitParams_p->ClkGenExtClk.BitMask;
                ClkGenExtClk.IntHandler = NULL;
                i = SMART_PIOBitFromBitMask(InitParams_p->Clk.BitMask); /* Clk (OUT) */


                /* Changes done for NDS type of Cards */
                if (InitParams_p->DeviceType != STSMART_ISO_NDS)
                {
                    Clk.BitConfigure[i] = STPIO_BIT_OUTPUT;
                }
                else
                {
                    Clk.BitConfigure[i] = STPIO_BIT_ALTERNATE_OUTPUT;
                }

                Clk.ReservedBits = InitParams_p->Clk.BitMask;
                Clk.IntHandler = NULL;
                i = SMART_PIOBitFromBitMask(InitParams_p->Reset.BitMask); /* Reset (OUT) */
                Reset.BitConfigure[i] = STPIO_BIT_OUTPUT;
                Reset.ReservedBits = InitParams_p->Reset.BitMask;
                Reset.IntHandler = NULL;
                i = SMART_PIOBitFromBitMask(InitParams_p->CmdVcc.BitMask); /* Vcc (OUT) */

                /* Changes done for NDS type of Cards */
                if (InitParams_p->DeviceType != STSMART_ISO_NDS)
                {
                    CmdVcc.BitConfigure[i] = STPIO_BIT_OUTPUT;
                }
                else
                {
                    CmdVcc.BitConfigure[i] = STPIO_BIT_OUTPUT_HIGH;
                }

                CmdVcc.ReservedBits = InitParams_p->CmdVcc.BitMask;
                CmdVcc.IntHandler = NULL;
                i = SMART_PIOBitFromBitMask(InitParams_p->CmdVpp.BitMask); /* Vpp (OUT) */
                CmdVpp.BitConfigure[i] = STPIO_BIT_OUTPUT;
                CmdVpp.ReservedBits = InitParams_p->CmdVpp.BitMask;
                CmdVpp.IntHandler = NULL;

                if ( Smart_p->IsInternalSmartcard == FALSE)
                {
                    i = SMART_PIOBitFromBitMask(InitParams_p->Detect.BitMask); /* Detect (IN) */
                    Detect.BitConfigure[i] = STPIO_BIT_INPUT;
                    Detect.ReservedBits = InitParams_p->Detect.BitMask;
                    Detect.IntHandler = SMART_DetectHandler;
                }

                /* Open up the PIO device for each control line
                 * we are interested in (above).
                 */
                if (InitParams_p->ClkGenExtClk.BitMask != 0) /* Optional */
                    error = STPIO_Open(
                                InitParams_p->ClkGenExtClk.PortName,
                                &ClkGenExtClk, &Smart_p->ClkGenExtClkHandle);
                if (error == ST_NO_ERROR)
                {
                    error = STPIO_Open(InitParams_p->Clk.PortName,
                                       &Clk, &Smart_p->ClkHandle);
                    if (error == ST_NO_ERROR)
                    {
                        error = STPIO_Open(InitParams_p->Reset.PortName,
                                           &Reset, &Smart_p->ResetHandle);
                        if (error == ST_NO_ERROR)
                        {
                            error = STPIO_Open(InitParams_p->CmdVcc.PortName,
                                               &CmdVcc, &Smart_p->CmdVccHandle);
                            if (error == ST_NO_ERROR)
                            {
                                /* Vpp is optional */
                                if (InitParams_p->CmdVpp.BitMask != 0)
                                {
                                    error = STPIO_Open(
                                        InitParams_p->CmdVpp.PortName,
                                        &CmdVpp, &Smart_p->CmdVppHandle);
                                }

                                if (error == ST_NO_ERROR && Smart_p->IsInternalSmartcard == FALSE )
                                {

                                    error = STPIO_Open(
                                            InitParams_p->Detect.PortName,
                                            &Detect, &Smart_p->DetectHandle);
                                }
                            }
                        }
                    }
                }

                /* Proceed with IO/UART initialization */
                if (error == ST_NO_ERROR)
                {
                    error = SMART_IO_Init(InitParams_p->UARTDeviceName,
                                          InitParams_p->DriverPartition,
                                          InitParams_p->DeviceType,
                                          &Smart_p->IoHandle);
                    if (error == ST_NO_ERROR)
                    {
                        SmartIoInit = TRUE;
                    }
#if defined(STSMART_LINUX_DEBUG)
                    printk("In scapi.c=>STSMART_Init(),after SMART_IO_Init error=0x%x\n",error);
#endif

                }

                /* Proceed with event registration */
                if (error == ST_NO_ERROR &&
                    SMART_UsingEventHandler(Smart_p))
                {

                    error = SMART_RegisterEvents(Smart_p);
#if defined(STSMART_LINUX_DEBUG)
                    printk("scapi.c=>STSMART_Init:after SMART_RegisterEvents() error=0x%x\n",error);
#endif

                }

                if (error == ST_NO_ERROR)
                {
#if !defined(PROCESSOR_C1) && !defined(ST_OSLINUX) && !defined(ST_OS21)
#pragma ST_device(ClockRegs_p)
#endif

                    volatile SMART_ClkRegs_t *ClockRegs_p;
                    STPIO_Compare_t Compare;
                    U8 b;

                    /* Initialize the control block */
                    Smart_p->ControlLock_p = STOS_SemaphoreCreateFifo(NULL,1);
                    Smart_p->CardLock.Semaphore_p = STOS_SemaphoreCreateFifoTimeOut(NULL,1);
                    Smart_p->IoSemaphore_p = STOS_SemaphoreCreateFifo(NULL,0);
                    Smart_p->RxSemaphore_p = STOS_SemaphoreCreateFifoTimeOut(NULL,0);

                    SemaphoreInit = TRUE;
                    if (Smart_p->IsInternalSmartcard == FALSE)
                    {

                        /* Check card detect state */
                        error = STPIO_Read(Smart_p->DetectHandle, &b);
                        Smart_p->CardInserted = SMART_IsCardInserted(Smart_p, b);

#if defined(STSMART_DETECT_INVERTED)
                        /* Set PIO output compare register */
                        Compare.ComparePattern = Smart_p->CardInserted ? 0 : Smart_p->InitParams.Detect.BitMask;
                        Compare.CompareEnable  = Smart_p->InitParams.Detect.BitMask;
#else
                        /* Set PIO output compare register */
                        Compare.ComparePattern = Smart_p->CardInserted ?
                                             Smart_p->InitParams.Detect.BitMask : 0;
                        Compare.CompareEnable  = Smart_p->InitParams.Detect.BitMask;
#endif
                        STPIO_SetCompare(Smart_p->DetectHandle, &Compare);

                    }
                    else
                    {
                        Smart_p->CardInserted = TRUE;
                    }

                    /* external card */

                    Smart_p->FreeHandles = InitParams_p->MaxHandles;
                    Smart_p->CardReset   = STSMART_ERROR_NOT_RESET;
                    Smart_p->BitConvention = ATR_DIRECT_CONVENTION;

                    /* Set the source of the clock input */
#ifdef ST_OSLINUX
                    ClockRegs_p = (volatile SMART_ClkRegs_t *)Smart_p->MappingBaseAddress;
#else
                    ClockRegs_p = (volatile SMART_ClkRegs_t *)InitParams_p->BaseAddress;
#endif

                    SMART_SetClockSource(ClockRegs_p, \
                                         (Smart_p->InitParams.ClockSource == \
                                         STSMART_CPU_CLOCK ? CLOCK_SOURCE_GLOBAL :\
                                         CLOCK_SOURCE_EXTERNAL));


                    /* Create event message queue */
#ifndef ST_OS21
                    /* Store message queue pointer */
                    Smart_p->MessageQueue_p = &Smart_p->MessageQueue;
                    message_init_queue_timeout(Smart_p->MessageQueue_p,
                                               (void *)Smart_p->MessageQueueMemory,
                                               sizeof(SMART_Message_t),
                                               SMART_MQUEUE_MAX_MESSAGES);
#else
                    Smart_p->MessageQueue_p =STOS_MessageQueueCreate(sizeof(SMART_Message_t),
                                                                           SMART_MQUEUE_MAX_MESSAGES);
#endif

                    /* Create event manager task */
                    error = STOS_TaskCreate((void(*)(void *))SMART_EventManager,
                                        Smart_p->MessageQueue_p,
                                        InitParams_p->DriverPartition,
                                        SMART_TASK_STACK_SIZE,
                                        &Smart_p->EventManagerTaskStack_p,
                                        InitParams_p->DriverPartition,
                                        &Smart_p->EventManagerTask_p,
                                        &Smart_p->EventManagerTaskDescriptor,
                                        STSMART_EVENTMGR_TASK_PRIORITY,
                                        "SMART_EventManager",
                                        (task_flags_t)0);
                    if(error != ST_NO_ERROR)
                    {
                        /* Free up allocated resources */
                        STOS_MessageQueueDelete(Smart_p->MessageQueue_p);
                        error = ST_ERROR_NO_MEMORY;
                    }
                }
                if (error == ST_NO_ERROR)
                {
                    /* Finally append this device to the control block queue */
                    SMART_QueueAdd(Smart_p);
                    SMART_Deactivate(Smart_p);

                    /* Set magic number */
                    Smart_p->Magic = STSMART_DRIVER_ID;
                }
                else
                {
                    /* Attempt to tidy up PIO if we failed to open anything */
                    if((U32*)(Smart_p->ClkGenExtClkHandle) != NULL)
                    {
                        STPIO_Close(Smart_p->ClkGenExtClkHandle);
                    }
                    STPIO_Close(Smart_p->ClkHandle);
                    STPIO_Close(Smart_p->ResetHandle);
                    STPIO_Close(Smart_p->CmdVccHandle);
                    if((U32*)Smart_p->CmdVppHandle != NULL)
                    {
                        STPIO_Close(Smart_p->CmdVppHandle);
                    }
                    if (Smart_p->IsInternalSmartcard == FALSE)
                    {
                        STPIO_Close(Smart_p->DetectHandle);
                    }

                    /* Attempt to tidy up IO/UART also */
                    if (SmartIoInit)
                    {
                        SMART_IO_Term(Smart_p->IoHandle);
                    }

                    /* Free up semaphores */
                    if (SemaphoreInit)
                    {
                        STOS_SemaphoreDelete(NULL,Smart_p->ControlLock_p);
                        STOS_SemaphoreDelete(NULL,Smart_p->CardLock.Semaphore_p);
                        STOS_SemaphoreDelete(NULL,Smart_p->IoSemaphore_p);
                        STOS_SemaphoreDelete(NULL,Smart_p->RxSemaphore_p);
                    }

#ifdef STSMART_USE_STEVT
                    /* Unregister events */
                    if (SMART_UsingEventHandler(Smart_p))
                    {
                        SMART_UnregisterEvents(Smart_p);
                    }
#endif
                    /* Free up memory associated with control block */
                    STOS_MemoryDeallocate(DriverPartition, Smart_p);
                }
            }
            else
            {
                /* Memory allocation failure */
                error = ST_ERROR_NO_MEMORY;
            }
        }
        else
        {
            /* The device is already in use */
            error = ST_ERROR_ALREADY_INITIALIZED;
        }
    }
    else
    {
        /* One or more parameters is invalid */
        error = ST_ERROR_BAD_PARAMETER;
    }

#if defined(STSMART_LINUX_DEBUG)
    printk("In scapi.c=>STSMART_Init(),error = 0x%x\n",error);
#endif
    /* Common exit point - release semaphore and return error code */
    if (SmartQueueHead_p != NULL)
    {
        STOS_SemaphoreSignal(SmartQueueSemaphore_p);
    }

    return error;
} /* STSMART_Init() */

/*****************************************************************************
STSMART_Lock()
Description:
    Locks the smartcard device for exclusive access.
Parameters:
    Handle,             client handle for the smartcard device.
    WaitForLock,        if set true, the caller will block until they have
                        exclusive access to the smartcard device.
Return Value:
    ST_NO_ERROR,                the operation completed without error.
    ST_ERROR_INVALID_HANDLE,    the handle is invalid.
    ST_ERROR_DEVICE_BUSY,       the card lock is already held and the caller
                                does not wish to block.
See Also:
    STSMART_Unlock()
*****************************************************************************/

ST_ErrorCode_t STSMART_Lock(STSMART_Handle_t Handle, BOOL WaitForLock)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    SMART_ControlBlock_t *Smart_p;
    int rc;

    /* Obtain the smartcard controlblock from the passed in handle */
    Smart_p = SMART_GetControlBlockFromHandle(Handle);

    if (Smart_p != NULL)
    {
        /* The timeout value can be either "immediate" -- the caller does not wish
         * to block, or "infinity" -- the caller will wait until the semaphore
         * is released.
         */
        if (WaitForLock)
        {
            rc = STOS_SemaphoreWaitTimeOut(Smart_p->CardLock.Semaphore_p,
                                                    TIMEOUT_INFINITY);
        }
        else
        {
            rc = STOS_SemaphoreWaitTimeOut(Smart_p->CardLock.Semaphore_p,
                                                    TIMEOUT_IMMEDIATE);
        }

        /* Check the return code:  0 => the semaphore has been acqiured.
         * Otherwise the timeout has elapsed (immediate).
         */
        if (rc == 0)
        {
            /* The semaphore has been successfully acquired.  We should now
             * fill in the details of the owner of the smartcard lock --
             * to ensure nobody else is able to utilise the smartcard
             * whilst the lock is held.
             */
            STOS_SemaphoreWait(Smart_p->ControlLock_p);
            Smart_p->CardLock.Owner = Handle;
            STOS_SemaphoreSignal(Smart_p->ControlLock_p);
        }
        else
        {
            /* The caller is not prepared to wait -- the device is
             * still locked.
             */
            error = ST_ERROR_DEVICE_BUSY;
        }
    }
    else
    {
        /* The handle is invalid */
        error = ST_ERROR_INVALID_HANDLE;
    }

    return error;
} /* STSMART_Lock() */

/*****************************************************************************
STSMART_Open()
Description:
    Opens a new connection with a smartcard device.
Parameters:
    DeviceName,     the name of the device registered during initialization.
    OpenParams_p,   pointer to the open params.
    Handle_p,       updated with the client handle for communicating with
                    a smartcard.
Return Value:
    ST_NO_ERROR,                the operation completed without error.
    ST_ERROR_BAD_PARAMETER,     one or more parameters is invalid.
    ST_ERROR_UNKNOWN_DEVICE,    the device has not been initialized.
    ST_ERROR_NO_FREE_HANDLES,   the maximum handles limit has been reached
                                for this device.
See Also:
    STSMART_Close()
*****************************************************************************/

ST_ErrorCode_t STSMART_Open( ST_DeviceName_t DeviceName,
                            const STSMART_OpenParams_t *OpenParams_p,
                            STSMART_Handle_t *Handle_p)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    SMART_ControlBlock_t *Smart_p;

    Smart_p = SMART_GetControlBlockFromName(DeviceName);

    if (Smart_p != NULL)
    {
        /* Obtain the control lock while we allocate a new handle */
        STOS_SemaphoreWait(Smart_p->ControlLock_p);
        if (Smart_p->FreeHandles > 0)
        {
            U32 i, hdl;

            /* Find first free handle */
            for (i = 0; i < Smart_p->InitParams.MaxHandles; i++)
            {
                if (!Smart_p->OpenHandles[i].InUse)
                {
                    break;
                }
            }

            /* Set handle value : this value is calculating in such a way
             * allowing easy conversion from the handle to the array
             * index, using a simple modulo computation.
             */
            hdl = (U32)&Smart_p->OpenHandles[i];
            *Handle_p = hdl;

            /* Copy handle parameters */
            Smart_p->OpenHandles[i].OpenParams = *OpenParams_p;
            Smart_p->OpenHandles[i].InUse = TRUE;
            Smart_p->OpenHandles[i].Handle = hdl;
            Smart_p->FreeHandles--;

            /* Set control block back pointer */
            Smart_p->OpenHandles[i].ControlBlock = Smart_p;

            SMART_ClearStatus(Smart_p);
        }
        else
        {
            /* No free handles */
            error = ST_ERROR_NO_FREE_HANDLES;
        }
        STOS_SemaphoreSignal(Smart_p->ControlLock_p);
    }
    else
    {
        /* The device is unknown */
        error = ST_ERROR_UNKNOWN_DEVICE;
    }

    return error;
} /* STSMART_Open() */

/*****************************************************************************
STSMART_RawRead()
Description:
Parameters:
    Handle,
Return Value:
    ST_NO_ERROR,                the operation completed without error.
    ST_ERROR_INVALID_HANDLE,    the handle is invalid.
    ST_ERROR_DEVICE_BUSY,       the device is busy (card is locked).
    STSMART_ERROR_NOT_INSERTED,       no card is inserted.
    STSMART_ERROR_NOT_RESET,          the card has not been reset.
    STSMART_ERROR_RESET_FAILED,       the last reset operation failed.
    STSTMART_NO_ANSWER,         no response was recieved in time.
    STSMART_ERROR_OVERRUN,      a buffer overrun occurred prior to the
                                read operation -- the data may not be
                                complete.
    STSMART_ERROR_FRAME,        a frame error ocurred during the read.
    STSMART_ERROR_PARITY,       a parity error ocurred during the read.
See Also:
    Nothing.
*****************************************************************************/

ST_ErrorCode_t STSMART_RawRead(STSMART_Handle_t Handle,
                               U8 *Buffer,
                               U32 NumberToRead,
                               U32 *NumberRead_p,
                               U32 Timeout)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    SMART_ControlBlock_t *Smart_p;

    /* Obtain the smartcard controlblock from the passed in handle */
    Smart_p = SMART_GetControlBlockFromHandle(Handle);

    if (Smart_p != NULL)                /* Valid handle? */
    {
        /* Check the card ready for use */
        error = SMART_CheckCardReady(Handle, Smart_p);

        if (error == ST_NO_ERROR)
        {
            /* Allocate/setup an operation for the reset */
            STOS_SemaphoreWait(Smart_p->ControlLock_p);

            /* Check this operation is not already in use */
            if (Smart_p->IoOperation.Context == SMART_CONTEXT_NONE)
            {
                SMART_Message_t *Msg_p;

                /* Setup the read operation for updating caller's params */
                Smart_p->IoOperation.Context = SMART_CONTEXT_READ;
                Smart_p->IoOperation.OpenBlock_p = (SMART_OpenBlock_t *)Handle;
                Smart_p->IoOperation.NumberToRead = NumberToRead;
                Smart_p->IoOperation.ReadBuffer_p = Buffer;
                Smart_p->IoOperation.ReadBytes_p = NumberRead_p;
                Smart_p->IoOperation.Timeout = Timeout;
                Smart_p->IoOperation.Status_p = NULL;
                Smart_p->IoOperation.NotifySemaphore_p =
                    Smart_p->IoSemaphore_p;
                Smart_p->IoOperation.Error_p = &error;
                Smart_p->IoAbort = FALSE;
                STOS_SemaphoreSignal(Smart_p->ControlLock_p);

                /* Zero running count of bytes before starting */
                *NumberRead_p = 0;

                /* Claim a message for starting this operation */
                Msg_p = (SMART_Message_t *)STOS_MessageQueueClaimTimeout(
                    Smart_p->MessageQueue_p,
                    TIMEOUT_IMMEDIATE
                    );

                /* Fill in message parameters */
                Msg_p->MessageType = SMART_IO_OPERATION;
                Msg_p->MessageParams_u.IoParams_s.Smart_p = Smart_p;

                /* Send the message to the smartcard task's queue */
                STOS_MessageQueueSend(Smart_p->MessageQueue_p, Msg_p);

                /* Block if the caller wishes to block */
                if (((SMART_OpenBlock_t *)Handle)->OpenParams.BlockingIO)
                {
                    /* Block until IO is complete */
                    STOS_SemaphoreWait(Smart_p->IoSemaphore_p);
                }
            }
            else
            {
                /* We can't allow multiple reads/writes */
                STOS_SemaphoreSignal(Smart_p->ControlLock_p);
                error = ST_ERROR_DEVICE_BUSY;
            }
        }
    }
    else
    {
        /* The handle is invalid */
        error = ST_ERROR_INVALID_HANDLE;
    }

    return error;
} /* STSMART_RawRead() */

/*****************************************************************************
STSMART_RawWrite()
Description:
Parameters:
    Handle,
Return Value:
    ST_NO_ERROR,                the operation completed without error.
    ST_ERROR_INVALID_HANDLE,    the handle is invalid.
    ST_ERROR_DEVICE_BUSY,       the device is busy (card is locked).
    STSMART_ERROR_NOT_INSERTED,       no card is inserted.
    STSMART_ERROR_NOT_RESET,          the card has not been reset.
    STSMART_ERROR_RESET_FAILED,       the last reset operation failed.
See Also:
    Nothing.
*****************************************************************************/

ST_ErrorCode_t STSMART_RawWrite(STSMART_Handle_t Handle,
                                const U8 *Buffer,
                                U32 NumberToWrite,
                                U32 *NumberWritten_p,
                                U32 Timeout)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    SMART_ControlBlock_t *Smart_p;

    /* Obtain the smartcard controlblock from the passed in handle */
    Smart_p = SMART_GetControlBlockFromHandle(Handle);

    if (Smart_p != NULL)                /* Valid handle? */
    {
        /* Check the card ready for use */
        error = SMART_CheckCardReady(Handle, Smart_p);

        if (error == ST_NO_ERROR)
        {
            /* Allocate/setup an operation for the reset */
            STOS_SemaphoreWait(Smart_p->ControlLock_p);

            /* Check this operation is not already in use */
            if (Smart_p->IoOperation.Context == SMART_CONTEXT_NONE)
            {
                SMART_Message_t *Msg_p;

                /* Setup the operation */
                Smart_p->IoOperation.Context = SMART_CONTEXT_WRITE;
                Smart_p->IoOperation.OpenBlock_p = (SMART_OpenBlock_t *)Handle;
                Smart_p->IoOperation.WriteBuffer_p = (U8 *)Buffer;
                Smart_p->IoOperation.WrittenBytes_p = NumberWritten_p;
                Smart_p->IoOperation.NumberToWrite = NumberToWrite;
                Smart_p->IoOperation.Timeout = Timeout;
                Smart_p->IoOperation.Status_p = NULL;
                Smart_p->IoOperation.NotifySemaphore_p =
                    Smart_p->IoSemaphore_p;
                Smart_p->IoOperation.Error_p = &error;
                Smart_p->IoAbort = FALSE;
                STOS_SemaphoreSignal(Smart_p->ControlLock_p);

                /* Zero running count of bytes before starting */
                *NumberWritten_p = 0;

                /* Claim a message for starting this operation */
                Msg_p = (SMART_Message_t *)STOS_MessageQueueClaimTimeout(
                    Smart_p->MessageQueue_p,
                    TIMEOUT_IMMEDIATE
                    );

                /* Fill in message parameters */
                Msg_p->MessageType = SMART_IO_OPERATION;
                Msg_p->MessageParams_u.IoParams_s.Smart_p = Smart_p;

                /* Send the message to the smartcard task's queue */
                STOS_MessageQueueSend(Smart_p->MessageQueue_p, Msg_p);

                /* Block if the caller wishes to block */
                if (((SMART_OpenBlock_t *)Handle)->OpenParams.BlockingIO)
                {
                    /* Block until IO is complete */
                    STOS_SemaphoreWait(Smart_p->IoSemaphore_p);
                }
            }
            else
            {
                /* We can't allow multiple reads/writes */
                STOS_SemaphoreSignal(Smart_p->ControlLock_p);
                error = ST_ERROR_DEVICE_BUSY;
            }
        }
    }
    else
    {
        /* The handle is invalid */
        error = ST_ERROR_INVALID_HANDLE;
    }

    return error;
} /* STSMART_RawWrite() */

/*****************************************************************************
STSMART_Reset()
Description:
    Commences an ATR (answer to reset) sequence with the smartcard device.
    The history bytes of the ATR sequence are preserved in the driver,
    allowing future interrogation.
    This call is synchronous, any connected clients will be notified via
    their callback (if defined) that the card has been reset.
Parameters:
    Handle,         client handle for the smartcard device.
    Answer,         array of 33 bytes updated with the bytes of the ATR.
    AnswerLength_p, updated with the number of bytes in the ATR buffer.
Return Value:
    ST_NO_ERROR,                the operation completed without error.
    ST_ERROR_INVALID_HANDLE,    the handle is invalid.
    ST_ERROR_DEVICE_BUSY,       the device is busy (card is locked).
    STSMART_ERROR_NOT_INSERTED, no card is inserted.
    STSMART_ERROR_OVERRUN,      a buffer overrun occurred during the
                                ATR.
    STSMART_ERROR_FRAME,        a frame error ocurred during the ATR.
    STSMART_ERROR_PARITY,       a parity error ocurred during the ATR.
    STSMART_ERROR_NO_ANSWER,    the smartcard failed to respond to the
                                ATR request.
    STSMART_ERROR_WRONG_TS_CHAR,the initial character (TS) received
                                is invalid.
    STSMART_ERROR_UNKNOWN,      the procedure bytes are not recognised
                                by the driver.
See Also:
    Nothing.
*****************************************************************************/

ST_ErrorCode_t STSMART_Reset(STSMART_Handle_t Handle,
                             U8 Answer[33],
#if defined( STSMART_WARM_RESET )
                             U32 *AnswerLength_p,
                             STSMART_Reset_t ResetType )
#else
                             U32 *AnswerLength_p)
#endif
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    SMART_ControlBlock_t *Smart_p;

    /* Obtain the smartcard controlblock from the passed in handle */
    Smart_p = SMART_GetControlBlockFromHandle(Handle);

    if (Smart_p != NULL)
    {
        /* Check the card ready for use */
        error = SMART_CheckCardReady(Handle, Smart_p);

        if (error != STSMART_ERROR_NOT_INSERTED && error != ST_ERROR_DEVICE_BUSY)
        {
            /* Assume no errors */
            error = ST_NO_ERROR;

            /* Setup an operation for the reset */
            STOS_SemaphoreWait(Smart_p->ControlLock_p);

            /* Check this operation is not already in use */
            if (Smart_p->IoOperation.Context == SMART_CONTEXT_NONE)
            {
                SMART_Message_t *Msg_p;

                /* Ensure all ATR bytes zeroed before we start */
                memset(Smart_p->ATRBytes, 0, SMART_MAX_ATR);

                /* Set to IO as busy */

#if defined( STSMART_WARM_RESET )
                if( ResetType == SC_COLD_RESET )
                {
                    Smart_p->IoOperation.Context = SMART_CONTEXT_RESET;
                }
                else if (ResetType == SC_WARM_RESET)
                {
                   if (Smart_p->CardReset == ST_NO_ERROR)
                   {
                        Smart_p->IoOperation.Context = SMART_CONTEXT_WARM_RESET;
                   }
                   else
                   {
                        /* COLD RESET not done already */
                        STOS_SemaphoreSignal(Smart_p->ControlLock_p);
                        return STSMART_ERROR_NOT_RESET;
                   }

                }
#else
                Smart_p->IoOperation.Context = SMART_CONTEXT_RESET;
#endif

                STOS_SemaphoreSignal(Smart_p->ControlLock_p);

                /* Fill out remaining parameters to IO operation */
                Smart_p->IoOperation.OpenBlock_p = (SMART_OpenBlock_t *)Handle;
                Smart_p->IoOperation.ReadBuffer_p = Answer;
                Smart_p->IoOperation.ReadBytes_p = AnswerLength_p;
                Smart_p->IoOperation.NotifySemaphore_p = Smart_p->IoSemaphore_p;
                Smart_p->IoOperation.Error_p = &error;
                Smart_p->IoOperation.Status_p = NULL;

                /* Select default values for smartcard parameters */
                Smart_p->N    = SMART_N_DEFAULT;
                Smart_p->FInt = SMART_F_DEFAULT;
                Smart_p->DInt = SMART_D_DEFAULT;
                Smart_p->WInt = SMART_W_DEFAULT;
                Smart_p->IoAbort = FALSE;

                /* Claim a message for starting this operation */
                Msg_p = (SMART_Message_t *)STOS_MessageQueueClaimTimeout(
                    Smart_p->MessageQueue_p,
                    TIMEOUT_IMMEDIATE
                    );

                /* Fill in message parameters */
                Msg_p->MessageType = SMART_IO_OPERATION;
                Msg_p->MessageParams_u.IoParams_s.Smart_p = Smart_p;

                /* Send the message to the smartcard task's queue */
                STOS_MessageQueueSend(Smart_p->MessageQueue_p, Msg_p);

                /* Block if the caller wishes to block */
                if (((SMART_OpenBlock_t *)Handle)->OpenParams.BlockingIO)
                {
                    /* Block until IO is complete */
                    STOS_SemaphoreWait(Smart_p->IoSemaphore_p);

                }
            }
            else
            {
                /* We can't allow multiple reads */
                STOS_SemaphoreSignal(Smart_p->ControlLock_p);
                error = ST_ERROR_DEVICE_BUSY;
            }
        }
    }
    else
    {
        /* The handle is invalid */
        error = ST_ERROR_INVALID_HANDLE;

    }
#if defined(STSMART_LINUX_DEBUG)
    printk("scapi.c=>STSMART_Reset(), reach the end, error=0x%x, *AnswerLength_p=%d\n",error, *AnswerLength_p);
#endif

    return error;
} /* STSMART_Reset() */

/*****************************************************************************
STSMART_SetBaudRate()
Description:
    Sets the bitrate used to communicate with a smartcard.
Parameters:
    Handle,             client handle for the smartcard device.
    BaudRate,           value of bitrate required.
Return Value:
    ST_NO_ERROR,                the operation completed without error.
    ST_ERROR_BAD_PARAMETER,     unable to select required bitrate.
    ST_ERROR_INVALID_HANDLE,    the handle is invalid.
    STSMART_ERROR_NOT_INSERTED,       no card is inserted.
    STSMART_ERROR_NOT_RESET,          the card has not been reset.
    STSMART_ERROR_RESET_FAILED,       the last reset operation failed.

See Also:
    Nothing.
*****************************************************************************/

ST_ErrorCode_t STSMART_SetBaudRate(STSMART_Handle_t Handle,
                                   U32 BaudRate)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    SMART_ControlBlock_t *Smart_p;

    /* Obtain the smartcard controlblock from the passed in handle */
    Smart_p = SMART_GetControlBlockFromHandle(Handle);

    if (Smart_p != NULL)
    {
        /* Check the card ready for use */
        error = SMART_CheckCardReady(Handle, Smart_p);

        if (error == ST_NO_ERROR)
        {
            /* Check if the card is NDS type */
            /* if yes, we need not to check max baud rate */
            if (Smart_p->InitParams.DeviceType != STSMART_ISO_NDS)
            {
                if (BaudRate <= Smart_p->SmartParams.MaxBaudRate)
                {
                    error = SMART_IO_SetBitRate(Smart_p->IoHandle, BaudRate);
                    if (error == ST_NO_ERROR)
                    {
                        Smart_p->SmartParams.BaudRate = BaudRate;
                    }
                }
                else
                {
                    error = ST_ERROR_BAD_PARAMETER;
                }
            }
            else
            {
                error = SMART_IO_SetBitRate(Smart_p->IoHandle, BaudRate);
                if (error == ST_NO_ERROR)
                {
                    Smart_p->SmartParams.BaudRate = BaudRate;
                }
            }
        }
    }
    else
    {
        /* The handle is invalid */
        error = ST_ERROR_INVALID_HANDLE;
    }

    return error;
} /* STSMART_SetBaudRate() */

/*****************************************************************************
STSMART_SetBlockWaitIndex()
Description:
    Sets index for calculating inter-block waiting time. Should only be
    used for overriding pre-calculated values from ATR.
Parameters:
    Handle,             client handle for the smartcard device.
    Index               BWI (see ISO 7816)
Return Value:
    ST_NO_ERROR,                the operation completed without error.
    ST_ERROR_BAD_PARAMETER,     invalid value specified for guard delay.
    ST_ERROR_INVALID_HANDLE,    the handle is invalid.
    STSMART_ERROR_NOT_INSERTED,       no card is inserted.
    STSMART_ERROR_NOT_RESET,          the card has not been reset.
    STSMART_ERROR_RESET_FAILED,       the last reset operation failed.
See Also:
    Nothing.
*****************************************************************************/
ST_ErrorCode_t STSMART_SetBlockWaitIndex(STSMART_Handle_t Handle,
                                         U8 Index)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    SMART_ControlBlock_t *Smart_p;

    /* Obtain the smartcard controlblock from the passed in handle */
    Smart_p = SMART_GetControlBlockFromHandle(Handle);

    if (Smart_p != NULL)
    {
        /* Check the card has been reset */
        if (Smart_p->CardReset != STSMART_ERROR_NOT_RESET)
        {
            Smart_p->BWInt = Index;

            /* Invoke new parameters */
            error = SMART_SetParams(Smart_p);
        }
        else
        {
            /* Need values from ATR to calculate this. */
            error = STSMART_ERROR_NOT_RESET;
        }
    }
    else
    {
        /* The handle is invalid */
        error = ST_ERROR_INVALID_HANDLE;
    }

    return error;
}

/*****************************************************************************
STSMART_SetCharWaitIndex()
Description:
    Sets index for calculating inter-character waiting time. Should only be
    used for overriding pre-calculated values from ATR.
Parameters:
    Handle,             client handle for the smartcard device.
    Index               CWI (see ISO 7816)
Return Value:
    ST_NO_ERROR,                the operation completed without error.
    ST_ERROR_BAD_PARAMETER,     invalid value specified for guard delay.
    ST_ERROR_INVALID_HANDLE,    the handle is invalid.
    STSMART_ERROR_NOT_INSERTED,       no card is inserted.
    STSMART_ERROR_NOT_RESET,          the card has not been reset.
    STSMART_ERROR_RESET_FAILED,       the last reset operation failed.
See Also:
    Nothing.
*****************************************************************************/
ST_ErrorCode_t STSMART_SetCharWaitIndex(STSMART_Handle_t Handle,
                                        U8 Index)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    SMART_ControlBlock_t *Smart_p;

    /* Obtain the smartcard controlblock from the passed in handle */
    Smart_p = SMART_GetControlBlockFromHandle(Handle);

    if (Smart_p != NULL)
    {
        /* Don't need card to be ready to change this.  */
        Smart_p->CWInt = Index;

        /* Invoke new parameters */
        error = SMART_SetParams(Smart_p);
    }
    else
    {
        /* The handle is invalid */
        error = ST_ERROR_INVALID_HANDLE;
    }

    return error;
}

/*****************************************************************************
STSMART_SetClockFrequency()
Description:
    Attempts to set a specific clock frequency. Should only be used for
    overriding ATR interpretation (not recommended).
Parameters:
    Handle,             Client handle for the smartcard device.
    Frequency           Frequency to try
    ActualFrequency     The actual frequency achieved.
Return Value:
    ST_NO_ERROR,                the operation completed without error.
    ST_ERROR_BAD_PARAMETER,     invalid value specified for guard delay.
    ST_ERROR_INVALID_HANDLE,    the handle is invalid.
    STSMART_ERROR_NOT_INSERTED,       no card is inserted.
    STSMART_ERROR_NOT_RESET,          the card has not been reset.
    STSMART_ERROR_RESET_FAILED,       the last reset operation failed.
See Also:
    STSMART_CodeType_t
*****************************************************************************/

ST_ErrorCode_t STSMART_SetClockFrequency(STSMART_Handle_t Handle,
                                         U32 Frequency,
                                         U32 *ActualFrequency)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    SMART_ControlBlock_t *Smart_p = NULL;

    /* Check parameter */
    if (NULL == ActualFrequency)
    {
        return ST_ERROR_BAD_PARAMETER;
    }

    /* Obtain the smartcard controlblock from the passed in handle */
    Smart_p = SMART_GetControlBlockFromHandle(Handle);

    if (Smart_p != NULL)
    {
        /* If we haven't reset yet, check the user has given us
         * frequencies in-line with the ISO spec.
         */
        if (Smart_p->CardReset == STSMART_ERROR_NOT_RESET)
        {
            if ((Frequency > SMART_MAX_RESET_FREQUENCY) ||
                (Frequency < SMART_MIN_RESET_FREQUENCY))
            {
                return ST_ERROR_BAD_PARAMETER;
            }

            /* Change hardware */
            SMART_SetClockFrequency(Smart_p, Frequency, ActualFrequency);

            /* Copy into internal struct. Smart_p->Fs isn't used before
             * reset, so we don't have to update that, but we do anyway,
             * just to try and keep everything in sync with the hardware.
             */
            Smart_p->ResetFrequency = *ActualFrequency;
            Smart_p->Fs = *ActualFrequency;
        }
        else
        {
            /* We have reset, so check they're obeying the max frequency
             * reported by the card.
             */
            /* Don't check the max clock when card is of NDS type */
            if (Smart_p->InitParams.DeviceType != STSMART_ISO_NDS)
            {
                if (Frequency > Smart_p->SmartParams.MaxClockFrequency)
                {
                    return ST_ERROR_BAD_PARAMETER;
                }
            }

            /* Change hardware */
            SMART_SetClockFrequency(Smart_p,
                                    Frequency,
                                    ActualFrequency);

            /* Update internal structures */
            Smart_p->Fs = *ActualFrequency;
            error = SMART_SetParams(Smart_p);
        }
    }
    else
    {
        /* The handle is invalid */
        error = ST_ERROR_INVALID_HANDLE;
    }

    return error;
}

/***********************************************************************
STSMART_StoreClockFrequency()
Description:
    When smartcard clock is managed by Pios,SetClockFrequency function
    does not work,then this function is called to update the control_block.

Parameters:
    Handle,             Client handle for the smartcard device.
    Frequency           Frequency to try
Return Value:
    ST_NO_ERROR,                the operation completed without error.
    ST_ERROR_INVALID_HANDLE,    the handle is invalid.
See Also:
    Nothing
***********************************************************************/

ST_ErrorCode_t STSMART_StoreClockFrequency(STSMART_Handle_t Handle,
                                           U32 Frequency)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    SMART_ControlBlock_t *Smart_p = NULL;

    /* Obtain the smartcard controlblock from the passed in handle */
    Smart_p = SMART_GetControlBlockFromHandle(Handle);

    if (Smart_p != NULL)
    {
        Smart_p->SmartParams.ClockFrequency = Frequency;
    }
    else
    {
        /* The handle is invalid */
        error = ST_ERROR_INVALID_HANDLE;
    }

    return error;

}/* STSMART_StoreClockFrequency */

/*****************************************************************************
STSMART_SetErrorCodeType()
Description:
    Sets error code type used for checking blocks. Should only be used
    for overriding pre-calculated values from ATR.
Parameters:
    Handle,             client handle for the smartcard device.
    Code                Which type to use
Return Value:
    ST_NO_ERROR,                the operation completed without error.
    ST_ERROR_BAD_PARAMETER,     invalid value specified for guard delay.
    ST_ERROR_INVALID_HANDLE,    the handle is invalid.
    STSMART_ERROR_NOT_INSERTED,       no card is inserted.
    STSMART_ERROR_NOT_RESET,          the card has not been reset.
    STSMART_ERROR_RESET_FAILED,       the last reset operation failed.
See Also:
    STSMART_CodeType_t
*****************************************************************************/
ST_ErrorCode_t STSMART_SetErrorCodeType(STSMART_Handle_t Handle,
                                        STSMART_CodeType_t Code)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    SMART_ControlBlock_t *Smart_p;

    /* Check parameter */
    if ((Code < STSMART_CODETYPE_LRC) || (Code > STSMART_CODETYPE_CRC))
    {
        return ST_ERROR_BAD_PARAMETER;
    }

    /* Obtain the smartcard controlblock from the passed in handle */
    Smart_p = SMART_GetControlBlockFromHandle(Handle);

    if (Smart_p != NULL)
    {
        /* Don't need card to be ready to change this */
        if (Code == STSMART_CODETYPE_CRC)
        {
            Smart_p->RC = 1;
        }
        else
        {
            Smart_p->RC = 0;
        }
    }
    else
    {
        /* The handle is invalid */
        error = ST_ERROR_INVALID_HANDLE;
    }

    return error;
}

/*****************************************************************************
STSMART_SetGuardDelay()
Description:
    Sets the total guard delay used when transmitting characters to a
    smartcard.
Parameters:
    Handle,             client handle for the smartcard device.
    GuardDelay,         Time from leading edge of one byte to leading edge
                        of next byte; other values derived from this
Return Value:
    ST_NO_ERROR,                the operation completed without error.
    ST_ERROR_BAD_PARAMETER,     invalid value specified for guard delay.
    ST_ERROR_INVALID_HANDLE,    the handle is invalid.
    STSMART_ERROR_NOT_INSERTED,       no card is inserted.
    STSMART_ERROR_NOT_RESET,          the card has not been reset.
    STSMART_ERROR_RESET_FAILED,       the last reset operation failed.
See Also:
    Nothing.
*****************************************************************************/
ST_ErrorCode_t STSMART_SetGuardDelay(STSMART_Handle_t Handle,
                                     U16 GuardDelay)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    SMART_ControlBlock_t *Smart_p;

    /* Obtain the smartcard controlblock from the passed in handle */
    Smart_p = SMART_GetControlBlockFromHandle(Handle);

    if (Smart_p != NULL)
    {
        /* Check the card ready for use */
        error = SMART_CheckCardReady(Handle, Smart_p);

        if (error == ST_NO_ERROR)
        {
            /* Check guardtime parameter */
            if ( GuardDelay <= SMART_GUARD_DELAY_MAX )
            {
                /* The GuardDelay is the time between sending the parity bit of
                 * a character to the start bit of the next byte/character.
                 */
                if ((Smart_p->WorkingType == STSMART_T1_PROTOCOL) && (GuardDelay == 0))
                {
                    /* Special case - protocol T=1 allows a minimum
                     * "total time" of 11etu, therefore we have to program
                     * UART accordingly.
                     */
                    Smart_p->N = 1;
                }
                else
                {
                    if ( GuardDelay < SMART_GUARD_DELAY_MIN )
                    {
                        Smart_p->N = SMART_GUARD_DELAY_MIN;
                    }
                    else
                    {
                        Smart_p->N = GuardDelay;
                    }
                }

                /* Invoke new parameters */
                error = SMART_SetParams(Smart_p);
            }
            else
            {
                /* Invalid guard delay specified */
                error = ST_ERROR_BAD_PARAMETER;
            }
        }
    }
    else
    {
        /* The handle is invalid */
        error = ST_ERROR_INVALID_HANDLE;
    }

    return error;
} /* STSMART_SetGuardDelay() */

/*****************************************************************************
STSMART_SetInfoFieldSize()
Description:
    Sets IFSD for T=1. This will notify the card via S-block.
Parameters:
    Handle,             client handle for the smartcard device.
    IFSD                Size of device buffers (nominal)
Return Value:
    ST_NO_ERROR,                the operation completed without error.
    ST_ERROR_INVALID_HANDLE,    the handle is invalid.
    STSMART_ERROR_NOT_INSERTED,       no card is inserted.
    STSMART_ERROR_NOT_RESET,          the card has not been reset.
    STSMART_ERROR_RESET_FAILED,       the last reset operation failed.
See Also:
    Nothing.
*****************************************************************************/
ST_ErrorCode_t STSMART_SetInfoFieldSize(STSMART_Handle_t Handle,
                                        U8 IFSD)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    SMART_ControlBlock_t *Smart_p;

    /* Obtain the smartcard controlblock from the passed in handle */
    Smart_p = SMART_GetControlBlockFromHandle(Handle);

    if (Smart_p != NULL)
    {
        /* Check the card ready for use */
        error = SMART_CheckCardReady(Handle, Smart_p);
        if (error == ST_NO_ERROR)
        {
            SMART_OpenBlock_t *Open_p = (SMART_OpenBlock_t *)Handle;

            error = SMART_T1_SendSBlock(Smart_p,
                                        S_IFS_REQUEST, IFSD,
                                        Open_p->OpenParams.SAD,
                                        Open_p->OpenParams.DAD);
        }
    }
    else
    {
        /* The handle is invalid */
        error = ST_ERROR_INVALID_HANDLE;
    }

    return error;
}

/*****************************************************************************
STSMART_SetTxRetries()
Description:
    Sets the character transmit repitition count.
    For T=0 this is mandatory, but may be overridden using this function.
Parameters:
    Handle,             client handle for the smartcard device.
    TxRetries,          new character repitition count.  Setting to 0 disables
                        character reptition.
Return Value:
    ST_NO_ERROR,                the operation completed without error.
    ST_ERROR_INVALID_HANDLE,    the handle is invalid.
    STSMART_ERROR_NOT_INSERTED,       no card is inserted.
    STSMART_ERROR_NOT_RESET,          the card has not been reset.
    STSMART_ERROR_RESET_FAILED,       the last reset operation failed.
See Also:
    Nothing.
*****************************************************************************/

ST_ErrorCode_t STSMART_SetTxRetries(STSMART_Handle_t Handle,
                                    U8 TxRetries)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    SMART_ControlBlock_t *Smart_p;

    /* Obtain the smartcard controlblock from the passed in handle */
    Smart_p = SMART_GetControlBlockFromHandle(Handle);

    if (Smart_p != NULL)
    {
        /* Check the card ready for use */
        error = SMART_CheckCardReady(Handle, Smart_p);
        if (error == ST_NO_ERROR)
        {
            /* Update new retries count */
            Smart_p->Retries = TxRetries;

            /* Invoke new parameters */
            error = SMART_SetParams(Smart_p);
        }
    }
    else
    {
        /* The handle is invalid */
        error = ST_ERROR_INVALID_HANDLE;
    }

    return error;
} /* STSMART_SetTxRetries() */

/*****************************************************************************
STSMART_SetVpp()
Description:
    Sets the state of the card programming voltage input Vpp.
Parameters:
    Handle,             client handle for the smartcard device.
    VppActive,          TRUE =>     enables Vpp
                        FALSE =>    disables Vpp
Return Value:
    ST_NO_ERROR,                the operation completed without error.
    ST_ERROR_INVALID_HANDLE,    the handle is invalid.
    STSMART_ERROR_NOT_INSERTED,       no card is inserted.
    STSMART_ERROR_NOT_RESET,          the card has not been reset.
    STSMART_ERROR_RESET_FAILED,       the last reset operation failed.
See Also:
    Nothing.
*****************************************************************************/

ST_ErrorCode_t STSMART_SetVpp(STSMART_Handle_t Handle,
                              BOOL VppActive)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    SMART_ControlBlock_t *Smart_p;

    /* Obtain the smartcard controlblock from the passed in handle */
    Smart_p = SMART_GetControlBlockFromHandle(Handle);

    if (Smart_p != NULL)
    {
        /* Check the card ready for use */
        error = SMART_CheckCardReady(Handle, Smart_p);
        if (error == ST_NO_ERROR)
        {
            /* Set Vpp according to setting */
            if (VppActive)
            {
                SMART_EnableVpp(Smart_p);
            }
            else
            {
                SMART_DisableVpp(Smart_p);
            }

            /* Update card parameters */
            error = SMART_SetParams(Smart_p);
        }
    }
    else
    {
        /* The handle is invalid */
        error = ST_ERROR_INVALID_HANDLE;
    }

    return error;
} /* STSMART_SetVpp() */

/*****************************************************************************
STSMART_SetWorkWaitingTime()
Description:
    Sets the work waiting time interval i.e., the maximum delay time
    when awaiting a character from a smartcard.
Parameters:
    Handle,             client handle for the smartcard device.
    WorkWaitingTime,    New work waiting time value.
Return Value:
    ST_NO_ERROR,                the operation completed without error.
    ST_ERROR_INVALID_HANDLE,    the handle is invalid.
    STSMART_ERROR_NOT_INSERTED,       no card is inserted.
    STSMART_ERROR_NOT_RESET,          the card has not been reset.
    STSMART_ERROR_RESET_FAILED,       the last reset operation failed.
See Also:
    Nothing.
*****************************************************************************/

ST_ErrorCode_t STSMART_SetWorkWaitingTime(STSMART_Handle_t Handle,
                                          U32 WorkWaitingTime)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    SMART_ControlBlock_t *Smart_p;

    /* Obtain the smartcard controlblock from the passed in handle */
    Smart_p = SMART_GetControlBlockFromHandle(Handle);

    if (Smart_p != NULL)
    {
        /* Check the card ready for use */
        error = SMART_CheckCardReady(Handle, Smart_p);
        if (error == ST_NO_ERROR)
        {
            /* We ensure that the requested work waiting time is at least
             * 1ms (the minimum allowed by the driver).  If it isn't
             * we round up the WWT to 1ms.
             */
            if (((WorkWaitingTime * 1000) / Smart_p->SmartParams.BaudRate) > 0)
            {
                Smart_p->WWT = WorkWaitingTime;
            }
            else
            {
                Smart_p->WWT = ((Smart_p->SmartParams.BaudRate/1000)+1);
            }

            /* Invoke new parameters */
            error = SMART_SetParams(Smart_p);
        }
    }
    else
    {
        /* The handle is invalid */
        error = ST_ERROR_INVALID_HANDLE;
    }

    return error;
} /* STSMART_SetWorkWaitingTime() */

/*****************************************************************************
STSMART_SetProtocol()
Description:
    Commences a PTS (protocol selection) on the smartcard device.
    This call is synchronous and will not invoke a callback.
Parameters:
    Handle,         client handle for the smartcard device.
    PtsBytes_p,     array of bytes containing the PTS bytes i.e.,
                    PTS0, ..., PST3.
Return Value:
    ST_NO_ERROR,                the operation completed without error.
    ST_ERROR_INVALID_HANDLE,    the handle is invalid.
    ST_ERROR_DEVICE_BUSY,       the device is busy (card is locked).
    STSMART_ERROR_NOT_INSERTED,       no card is inserted.
    STSMART_ERROR_NOT_RESET,          the card has not been reset.
    STSMART_ERROR_RESET_FAILED,       the last reset operation failed.
    STSTMART_NO_ANSWER,         no response was recieved in time.
    STSMART_ERROR_OVERRUN,      a buffer overrun occurred prior to the
                                read operation -- the data may not be
                                complete.
    STSMART_ERROR_FRAME,        a frame error ocurred during the read.
    STSMART_ERROR_PARITY,       a parity error ocurred during the read.
    STSMART_ERROR_INVALID_PROTOCOL,   the smartcard does not support the requested
                                protocol.
    STSMART_ERROR_PTS_NOT_SUPPORTED,  the smartcard does not support any protocol
                                selection.
    STSMART_ERROR_UNKNOWN,      the procedure bytes are not recognised
                                by the driver.
See Also:
    Nothing.
*****************************************************************************/

ST_ErrorCode_t STSMART_SetProtocol(STSMART_Handle_t Handle,
                                   U8 *PtsBytes_p)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    SMART_ControlBlock_t *Smart_p;

    /* Obtain the smartcard controlblock from the passed in handle */
    Smart_p = SMART_GetControlBlockFromHandle(Handle);

    if (Smart_p != NULL)
    {
        /* Check the card ready for use */
        error = SMART_CheckCardReady(Handle, Smart_p);

        if (error == ST_NO_ERROR)
        {
            SMART_Message_t *Msg_p;

            /* Clear out status parameters */
            SMART_ClearStatus(Smart_p);

            /* Allocate/setup an operation for the reset */
            STOS_SemaphoreWait(Smart_p->ControlLock_p);

            /* Check this operation is not already in use */
            if (Smart_p->IoOperation.Context == SMART_CONTEXT_NONE &&
                Smart_p->IoOperation.Context == SMART_CONTEXT_NONE)
            {
                Smart_p->IoOperation.Context = SMART_CONTEXT_SETPROTOCOL;
                Smart_p->IoOperation.OpenBlock_p = (SMART_OpenBlock_t *)Handle;
                Smart_p->IoOperation.WriteBuffer_p = PtsBytes_p;
                Smart_p->IoOperation.NotifySemaphore_p =
                    Smart_p->IoSemaphore_p;
                Smart_p->IoOperation.Error_p = &error;
                Smart_p->IoAbort = FALSE;
                STOS_SemaphoreSignal(Smart_p->ControlLock_p);

                /* Claim a message for starting this operation */
                Msg_p = (SMART_Message_t *)STOS_MessageQueueClaimTimeout(
                    Smart_p->MessageQueue_p,
                    TIMEOUT_IMMEDIATE
                    );

                /* Fill in message parameters */
                Msg_p->MessageType = SMART_IO_OPERATION;
                Msg_p->MessageParams_u.IoParams_s.Smart_p = Smart_p;

                /* Send the message to the smartcard task's queue */
                STOS_MessageQueueSend(Smart_p->MessageQueue_p, Msg_p);

                /* Block if the caller wishes to block */
                if (((SMART_OpenBlock_t *)Handle)->OpenParams.BlockingIO)
                {
                    /* Block until IO is complete */
                    STOS_SemaphoreWait(Smart_p->IoSemaphore_p);
                }
            }
            else
            {
                /* We can't allow multiple reads */
                STOS_SemaphoreSignal(Smart_p->ControlLock_p);
                error = ST_ERROR_DEVICE_BUSY;
            }
        }
    }
    else
    {
        /* The handle is invalid */
        error = ST_ERROR_INVALID_HANDLE;
    }

    return error;
} /* STSMART_SetProtocol() */

/***********************************************************************
STSMART_SetRxTimeout()
Description:
    Update RxTimeout
Parameters:
    Handle,             client handle for the smartcard device.
    Timeout,            Timeout to be updated
Return Value:
    ST_NO_ERROR,                the operation completed without error.
    ST_ERROR_INVALID_HANDLE,    the handle is invalid.
See Also:
    STSMART_GetRxTimeout.
***********************************************************************/

ST_ErrorCode_t STSMART_SetRxTimeout(STSMART_Handle_t Handle, U32 Timeout)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    SMART_ControlBlock_t *Smart_p = NULL;

    /* Obtain the smartcard controlblock from the passed in handle */
    Smart_p = SMART_GetControlBlockFromHandle(Handle);

    if (Smart_p != NULL)
    {
        Smart_p->RxTimeout = Timeout;
    }
    else
    {
        /* The handle is invalid */
        error = ST_ERROR_INVALID_HANDLE;
    }

    return error;

}/* STSMART_SetRxTimeout */

/*****************************************************************************
STSMART_Term()
Description:
    Terminates a smartcard device.  Any resources claimed during init will
    be freed.
    Note that if a 'ForceTerminate' is specified whilst a clients is waiting
    on a pending read/write transfer, they will be notified with a
    'STSMART_ERROR_ABORTED' error, and their handle will be closed.
Parameters:
    DeviceName,     the name of device registered at init time.
    TermParams_p,   parameters that guide the termination process.
Return Value:
    ST_NO_ERROR,                the operation completed without error.
    ST_ERROR_UNKNOWN_DEVICE,    the device is not recognised.
    ST_ERROR_OPEN_HANDLE,       one or more handles are still open.
See Also:
    STSMART_Init()
*****************************************************************************/

ST_ErrorCode_t STSMART_Term( ST_DeviceName_t DeviceName,
                            const STSMART_TermParams_t *TermParams_p)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    SMART_ControlBlock_t *Smart_p;
    partition_t *DriverPartition;

    if (SmartQueueHead_p != NULL)
    {
        /* Ensure nobody accesses the queue while we're in */
        STOS_SemaphoreWait(SmartQueueSemaphore_p);

        /* Obtain the UART control block associated with the device name */
        Smart_p = SMART_GetControlBlockFromName(DeviceName);

        /* Only continue if we have a control block for this device */
        if (Smart_p != NULL)
        {
            /* This device may be terminated if either:
             * a) the caller wishes to force termination
             * b) no clients are connected
             */
            if (Smart_p->FreeHandles == Smart_p->InitParams.MaxHandles ||
                TermParams_p->ForceTerminate)
            {
                U32 i;
                SMART_Message_t *Msg_p;

                /* Iterate over each handle and close it -- this will also
                 * abort any pending IO with the UART
                 */
                for (i = 0; i < SMART_MAX_HANDLES; i++)
                {
                    if (Smart_p->OpenHandles[i].InUse)
                    {
                        STSMART_Close(Smart_p->OpenHandles[i].Handle);
                    }
                }

                /* Deactivate the smartcard */
                SMART_Deactivate(Smart_p);

                /* Close associated IO/UART device */
                SMART_IO_Term(Smart_p->IoHandle);

                /* Close any open PIO ports */
                if((U32*)Smart_p->ClkGenExtClkHandle != NULL)
                {
                    STPIO_Close(Smart_p->ClkGenExtClkHandle);
                }
                STPIO_Close(Smart_p->ClkHandle);
                STPIO_Close(Smart_p->ResetHandle);
                STPIO_Close(Smart_p->CmdVccHandle);
                if((U32*)Smart_p->CmdVppHandle != NULL)
                {
                    STPIO_Close(Smart_p->CmdVppHandle);
                }
                if (Smart_p->IsInternalSmartcard == FALSE)
                {
                    STPIO_Close(Smart_p->DetectHandle);
                }

                /* Deallocate queue resources */
                SMART_QueueRemove(Smart_p);

                /* Free up other resources */
                STOS_SemaphoreDelete(NULL,Smart_p->ControlLock_p);
                STOS_SemaphoreDelete(NULL,Smart_p->CardLock.Semaphore_p);
                STOS_SemaphoreDelete(NULL,Smart_p->IoSemaphore_p);
                STOS_SemaphoreDelete(NULL,Smart_p->RxSemaphore_p);

                /* Unregister events */
                if (SMART_UsingEventHandler(Smart_p))
                {
                    error = SMART_UnregisterEvents(Smart_p);
                }

                /* Signal event manager task to die */
                Msg_p = (SMART_Message_t *)STOS_MessageQueueClaimTimeout(
                    Smart_p->MessageQueue_p,
                    TIMEOUT_INFINITY
                    );

                /* Check we claimed a message buffer */
                if (Msg_p != NULL)
                {
                    /* Setup the smartcard event to be generated */
                    Msg_p->MessageType = SMART_TASK_CONTROL;
                    Msg_p->MessageParams_u.TaskControlParams_s.ExitTask = TRUE;

#ifdef ST_OSLINUX
                    Msg_p->MessageParams_u.EventParams_s.Smart_p = Smart_p;
#endif
                    /* Now send the message to the queue */
                    STOS_MessageQueueSend(Smart_p->MessageQueue_p, Msg_p);

                    error = STOS_TaskWait( &Smart_p->EventManagerTask_p, TIMEOUT_INFINITY);
                    if(error == ST_NO_ERROR)
                    {
                        STOS_TaskDelete(Smart_p->EventManagerTask_p,
                                        (partition_t *)Smart_p->InitParams.DriverPartition,
                                        Smart_p->EventManagerTaskStack_p,
                                        (partition_t *)Smart_p->InitParams.DriverPartition);
                    }
                    /* Deallocate message queue resources */
                    STOS_MessageQueueDelete(Smart_p->MessageQueue_p);
                }

                /* Clear magic number */
                Smart_p->Magic = 0;

                /* Free up the control structure */
#ifdef ST_OSLINUX
                STLINUX_UnmapRegion((void *)Smart_p->MappingBaseAddress, Smart_p->SmartMappingWidth);
#endif
                DriverPartition =
                    (partition_t *)Smart_p->InitParams.DriverPartition;

                STOS_MemoryDeallocate(DriverPartition,
                                  Smart_p);
            }
            else
            {
                /* There is still an outstanding open on this device, but the
                 * caller does not wish to force termination.
                 */
                error = ST_ERROR_OPEN_HANDLE;
            }
        }
        else
        {
            /* The control block was not retrieved from the queue, therefore the
             * device name must be invalid.
             */
            error = ST_ERROR_UNKNOWN_DEVICE;
        }

        /* Common exit point; release the semaphore */
        STOS_SemaphoreSignal(SmartQueueSemaphore_p);
    }
    else
    {
        /* The queue head pointer is NULL, so there can not be any devices
         * to terminate.
         */
        error = ST_ERROR_UNKNOWN_DEVICE;
    }

    return error;
} /* STSMART_Term() */

/*****************************************************************************
STSMART_Unlock()
Description:
    Unlocks the smartcard lock disabling exclusive access.
    This will allow a blocked caller to obtain the smartcard lock.
Parameters:
    Handle,             client handle for the smartcard device.
Return Value:
    ST_NO_ERROR,                the operation completed without error.
    ST_ERROR_INVALID_HANDLE,    the handle is invalid.
    ST_ERROR_BAD_PARAMETER,     the caller is not the owner of the lock.
See Also:
    STSMART_Lock()
*****************************************************************************/

ST_ErrorCode_t STSMART_Unlock(STSMART_Handle_t Handle)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    SMART_ControlBlock_t *Smart_p;

    /* Obtain the smartcard controlblock from the passed in handle */
    Smart_p = SMART_GetControlBlockFromHandle(Handle);

    if (Smart_p != NULL)
    {
        /* Obtain the control lock before checking lock */
        STOS_SemaphoreWait(Smart_p->ControlLock_p);

        /* Ensure that the caller is the owner of the lock */
        if (Handle == Smart_p->CardLock.Owner)
        {
            /* The caller no longer requires exclusive access, so
             * we clear the owner field of the card lock.
             */
            Smart_p->CardLock.Owner = 0;

            /* Signal the semaphore to unblock a waiting client */
            STOS_SemaphoreSignal(Smart_p->CardLock.Semaphore_p);

            /* Release control lock now we've done */
            STOS_SemaphoreSignal(Smart_p->ControlLock_p);
        }
        else
        {
            /* The caller is not the owner of the lock, so we must
             * not allow them to clear the lock!
             */
            STOS_SemaphoreSignal(Smart_p->ControlLock_p); /* Release control lock */
            error = ST_ERROR_BAD_PARAMETER;
        }
    }
    else
    {
        /* The handle is invalid */
        error = ST_ERROR_INVALID_HANDLE;
    }

    return error;
} /* STSMART_Unlock() */

/*****************************************************************************
STSMART_Transfer()
Description:
    Combines the functionality of both a write and read.
    If the driver is opened in asynchronous mode, then only one notification
    will be received by the client
Parameters:
Handle,
    Command_p,              pointer to buffer containing write command.
    NumberToWrite,          contains number of bytes in the command buffer.
    NumberWritten_p,        updated with the number of bytes written.
    Response_p,             updated with the response from the card.
    NumberToRead,           number of bytes expected (0 => all available).
    NumberRead_p,           updated with the number of bytes read (excluding
                            the procedure bytes).
    Status_p,               updated with the status params i.e., errorcode
                            and procedure bytes.
Return Value:
    ST_NO_ERROR,                the operation completed without error.
    ST_ERROR_INVALID_HANDLE,    the handle is invalid.
    ST_ERROR_DEVICE_BUSY,       the device is busy (card is locked).
    STSMART_ERROR_NOT_INSERTED,       no card is inserted.
    STSMART_ERROR_NOT_RESET,          the card has not been reset.
    STSMART_ERROR_RESET_FAILED,       the last reset operation failed.
    STSTMART_NO_ANSWER,         no response was recieved in time.
    STSMART_ERROR_OVERRUN,      a buffer overrun occurred prior to the
                                read operation -- the data may not be
                                complete.
    STSMART_ERROR_FRAME,        a frame error ocurred during the read.
    STSMART_ERROR_PARITY,       a parity error ocurred during the read.
    STSMART_ERROR_INVALID_CODE,       the INS byte was not recognised by the
                                smartcard (T=0 only).
    STSMART_ERROR_INVALID_CLASS,      the CLA byte was not recognised by the
                                smartcard (T=0 only).
    STSMART_ERROR_INCORRECT_LENGTH,   the command sent to the smartcard did
                                not contain the correct number of bytes.
    STSMART_ERROR_INCORRECT_REFERENCE,    the command sent to the smartcard
                                    contained an invalid reference.
    STSMART_ERROR_INVALID_STATUS_BYTE,    one or more of the status bytes
                                    recieved from the card do not
                                    conform with the protocol (T=0).
    STSMART_ERROR_UNKNOWN,      the procedure bytes are not recognised
                                by the driver.
See Also:
    Nothing.
*****************************************************************************/

ST_ErrorCode_t STSMART_Transfer(STSMART_Handle_t Handle,
                                U8 *Command_p,
                                U32 NumberToWrite,
                                U32 *NumberWritten_p,
                                U8 *Response_p,
                                U32 NumberToRead,
                                U32 *NumberRead_p,
                                STSMART_Status_t *Status_p)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    SMART_ControlBlock_t *Smart_p;

    /* Obtain the smartcard controlblock from the passed in handle */
    Smart_p = SMART_GetControlBlockFromHandle(Handle);

    if (Smart_p != NULL)
    {
        /* Check the card ready for use */
        error = SMART_CheckCardReady(Handle, Smart_p);

        if (error == ST_NO_ERROR)       /* Proceed if card is powered and reset */
        {
            /* Check current protocol is supported */
            if (((1 << Smart_p->SpecificType) &
                 Smart_p->Capability.SupportedISOProtocols) != 0)
            {
                SMART_Message_t *Msg_p;

                /* Check user parameters before starting the transfer -
                 * the check is protocol specific.
                 */
                error = SMART_CheckTransfer(Smart_p,
                                            Command_p);

                /* Do not proceed unless the parameter check passed */
                if (error != ST_NO_ERROR)
                {
                    return error;
                }

                /* Allocate/setup an operation for the reset */
                STOS_SemaphoreWait(Smart_p->ControlLock_p);

                /* Check the operation is not already in use */
                if (Smart_p->IoOperation.Context == SMART_CONTEXT_NONE)
                {
                    /* Setup the operation for updating caller's params */
                    Smart_p->IoOperation.Context = SMART_CONTEXT_TRANSFER;
                    Smart_p->IoOperation.OpenBlock_p = (SMART_OpenBlock_t *)Handle;
                    Smart_p->IoOperation.ReadBytes_p = NumberRead_p;
                    Smart_p->IoOperation.ReadBuffer_p = Response_p;
                    Smart_p->IoOperation.NumberToRead = NumberToRead;
                    Smart_p->IoOperation.WrittenBytes_p = NumberWritten_p;
                    Smart_p->IoOperation.WriteBuffer_p = Command_p;
                    Smart_p->IoOperation.NumberToWrite = NumberToWrite;
                    Smart_p->IoOperation.Status_p = Status_p;
                    Smart_p->IoOperation.NotifySemaphore_p =
                        Smart_p->IoSemaphore_p;
                    Smart_p->IoOperation.Error_p = &error;
                    Smart_p->IoAbort = FALSE;

                    /* Setup the read operation for updating the caller's params */
                    STOS_SemaphoreSignal(Smart_p->ControlLock_p);

                    /* Zero running count of bytes before starting */
                    *NumberRead_p = 0;
                    *NumberWritten_p = 0;

                    /* Claim a message for starting this operation */
                    Msg_p = (SMART_Message_t *)STOS_MessageQueueClaimTimeout(
                        Smart_p->MessageQueue_p,
                        TIMEOUT_IMMEDIATE
                        );

                    /* Fill in message parameters */
                    Msg_p->MessageType = SMART_IO_OPERATION;
                    Msg_p->MessageParams_u.IoParams_s.Smart_p = Smart_p;

                    /* Send the message to the smartcard task's queue */
                    STOS_MessageQueueSend(Smart_p->MessageQueue_p, Msg_p);

                    /* Block if the caller wishes to block */
                    if (((SMART_OpenBlock_t *)Handle)->OpenParams.BlockingIO)
                    {
                        /* Block until IO is complete */
                        STOS_SemaphoreWait(Smart_p->IoSemaphore_p);
                    }
                }
                else
                {
                    /* We can't allow multiple reads/writes */
                    STOS_SemaphoreSignal(Smart_p->ControlLock_p);
                    error = ST_ERROR_DEVICE_BUSY;
                }
            }
            else
            {
                /* Protocol not supported */
                error = STSMART_ERROR_INVALID_PROTOCOL;
            }
        }
    }
    else
    {
        /* The handle is invalid */
        error = ST_ERROR_INVALID_HANDLE;
    }

    return error;
} /* STSMART_Transfer() */

/* Private functions ------------------------------------------------------ */

/*****************************************************************************
SMART_QueueAdd()
Description:
    This routine appends an allocated smartcard control block to the
    smartcard queue.

    NOTE: The smartcard queue semaphore must be held when calling this routine.

Parameters:
    QueueItem_p, the control block to append to the queue.
Return Value:
    Nothing.
See Also:
    SMART_QueueRemove()
*****************************************************************************/

static void SMART_QueueAdd(SMART_ControlBlock_t *QueueItem_p)
{
    SMART_ControlBlock_t *qp;

    /* Check the base element is defined, otherwise, append to the end of the
     * linked list.
     */
    if (SmartQueueHead_p == NULL)
    {
        /* As this is the first item, no need to append */
        SmartQueueHead_p = QueueItem_p;
        QueueItem_p->Next_p = NULL;
    }
    else
    {
        /* Iterate along the list until we reach the final item */
        qp = SmartQueueHead_p;
        while (qp != NULL && qp->Next_p)
        {
            qp = qp->Next_p;
        }

        /* Append the new item */
        qp->Next_p = QueueItem_p;
        QueueItem_p->Next_p = NULL;
    }
} /* SMART_QueueAdd() */

/*****************************************************************************
SMART_QueueRemove()
Description:
    This routine removes an allocated smartcard control block from the
    smartcard queue.

    NOTE: The smartcard queue semaphore must be held when calling this routine.
    Use SMART_IsInit() or SMART_GetControlBlockFromName() to verify whether
    or not a particular item is in the queue prior to calling this routine.

Parameters:
    QueueItem_p, the control block to remove from the queue.
Return Value:
    Nothing.
See Also:
    SMART_QueueAdd()
*****************************************************************************/

static void SMART_QueueRemove(SMART_ControlBlock_t *QueueItem_p)
{
    SMART_ControlBlock_t *this_p, *last_p;

    /* Check the base element is defined, otherwise quit as no items are
     * present on the queue.
     */
    if (SmartQueueHead_p != NULL)
    {
        /* Reset pointers to loop start conditions */
        last_p = NULL;
        this_p = SmartQueueHead_p;

        /* Iterate over each queue element until we find the required
         * queue item to remove.
         */
        while (this_p != NULL && this_p != QueueItem_p)
        {
            last_p = this_p;
            this_p = this_p->Next_p;
        }

        /* Check we found the queue item */
        if (this_p == QueueItem_p)
        {
            /* Unlink the item from the queue */
            if (last_p != NULL)
            {
                last_p->Next_p = this_p->Next_p;
            }
            else
            {
                /* Recalculate the new head of the queue i.e.,
                 * we have just removed the queue head.
                 */
                SmartQueueHead_p = this_p->Next_p;
            }
        }
    }
} /* SMART_QueueRemove() */

/*****************************************************************************
SMART_IsInit()
Description:
    Runs through the queue of initialized smartcard devices and checks that
    the smartcard with the specified device name has not already been added.

    NOTE: Must be called with the queue semaphore held.

Parameters:
    DeviceName, text string indicating the device name to check.
Return Value:
    TRUE, the device has already been initialized.
    FALSE, the device is not in the queue and therefore is not initialized.
See Also:
    STSMART_Init()
*****************************************************************************/

static BOOL SMART_IsInit(const ST_DeviceName_t DeviceName)
{
    return SMART_GetControlBlockFromName(DeviceName) != NULL ? TRUE : FALSE;
} /* SMART_IsInit() */

/*****************************************************************************
SMART_GetControlBlockFromName()
Description:
    Runs through the queue of initialized smartcard devices and checks for
    the smartcard with the specified device name.

    NOTE: Must be called with the queue semaphore held.

Parameters:
    DeviceName, text string indicating the device name to check.
Return Value:
    NULL, end of queue encountered - invalid device name.
    Otherwise, returns a pointer to the control block for the device.
See Also:
    SMART_IsInit()
*****************************************************************************/

static SMART_ControlBlock_t *
   SMART_GetControlBlockFromName(const ST_DeviceName_t DeviceName)
{
    SMART_ControlBlock_t *qp = SmartQueueHead_p; /* Global queue head pointer */

    while (qp != NULL)
    {
        /* Check the device name for a match with the item in the queue */
        if (strcmp(qp->DeviceName, DeviceName) != 0)
        {
            /* Next smartcard in the queue */
            qp = qp->Next_p;
        }
        else
        {
            /* The smartcard entry has been found */
            break;
        }
    }

    /* Return the smartcard control block (or NULL) to the caller */
    return qp;
} /* SMART_GetControlBlockFromName() */

/*****************************************************************************
SMART_GetControlBlockFromHandle()
Description:
    Runs through the queue of initialized smartcard devices and checks for
    the smartcard with the specified handle.
Parameters:
    Handle, an open handle.
Return Value:
    NULL, end of queue encountered - invalid handle.
    Otherwise, returns a pointer to the control block for the device.
See Also:
    SMART_GetControlBlockFromName()
*****************************************************************************/

static SMART_ControlBlock_t *
   SMART_GetControlBlockFromHandle(STSMART_Handle_t Handle)
{
    SMART_ControlBlock_t *Smart_p;
    S32 Addr;
    SMART_OpenBlock_t *Open_p;

    /* Check handle is in valid address space i.e., none peripheral space */
    Addr = (S32)Handle;
    if (Addr == 0)
    {
        return NULL;
    }

    /* Obtain control block */
    Open_p = (SMART_OpenBlock_t *)Handle;
    Smart_p = (SMART_ControlBlock_t *)Open_p->ControlBlock;

    /* Check magic number */
    if (Smart_p->Magic != STSMART_DRIVER_ID)
    {
        return NULL;
    }

    /* Return the smartcard control block (or NULL) to the caller */
    return Smart_p;
} /* SMART_GetControlBlockFromHandle() */

/*****************************************************************************
SMART_CheckCardReady()
Description:
    Performs a number of routine checks on the smartcard to ensure that it
    is ready to process an operation e.g., the card is inserted and reset.
    This routine also checks that the client handle is the owner of the
    card lock, if the card lock is taken.
Parameters:
    Handle,             client handle for the smartcard device.
    Smart_p,            pointer to the smartcard control block.
Return Value:
    ST_NO_ERROR,                smartcard is ready for next operation.
    STSMART_ERROR_NOT_INSERTED,       there is no smartcard inserted.
    STSMART_ERROR_NOT_RESET,          the card has not been reset.
    STSMART_ERROR_RESET_FAILED,       the last reset operation failed.
See Also:
    Nothing
*****************************************************************************/

static ST_ErrorCode_t SMART_CheckCardReady(STSMART_Handle_t Handle,
                                           SMART_ControlBlock_t *Smart_p)
{
    ST_ErrorCode_t error = ST_NO_ERROR;

    /* Ensure the control lock is not in use */
    STOS_SemaphoreWait(Smart_p->ControlLock_p);
    if (Smart_p->CardLock.Owner != 0 && Handle != Smart_p->CardLock.Owner)
    {
        /* The caller is not the owner of the lock */
        error = ST_ERROR_DEVICE_BUSY;
    }
    else if (!Smart_p->CardInserted)    /* Check the card is inserted */
    {
        /* No card inserted */
        error = STSMART_ERROR_NOT_INSERTED;
    }
    else                                /* Check card is reset */
    {
        /* Propagate the card reset status */
        error = Smart_p->CardReset;
    }
    STOS_SemaphoreSignal(Smart_p->ControlLock_p);

    return error;
} /* SMART_CheckCardReady() */

/*****************************************************************************
SMART_RegisterEvents()
Description:
    Register events with a nominated event handler driver.
Parameters:
    Smart_p,                    pointer to the smartcard control block.
Return Value:
    ST_NO_ERROR,                registered events ok.
    STSMART_ERROR_EVT_REGISTER, problem registering events.
See Also:
    Nothing
*****************************************************************************/

static ST_ErrorCode_t SMART_RegisterEvents(SMART_ControlBlock_t *Smart_p)
{
    ST_ErrorCode_t Error;
    STEVT_OpenParams_t EVTOpenParams;

    Error = STEVT_Open(Smart_p->InitParams.EVTDeviceName,
                       &EVTOpenParams,
                       &Smart_p->EVTHandle);

    if (Error == ST_NO_ERROR)           /* Open succeeded? */
    {
        /* Register all events */
        if (STEVT_RegisterDeviceEvent(Smart_p->EVTHandle,
                                      Smart_p->DeviceName,
                                      STSMART_EV_CARD_INSERTED,
                                      &Smart_p->EvtId[EventToId(STSMART_EV_CARD_INSERTED)]
                                     ) != ST_NO_ERROR ||
            STEVT_RegisterDeviceEvent(Smart_p->EVTHandle,
                                      Smart_p->DeviceName,
                                      STSMART_EV_CARD_REMOVED,
                                      &Smart_p->EvtId[EventToId(STSMART_EV_CARD_REMOVED)]
                                     ) != ST_NO_ERROR ||
            STEVT_RegisterDeviceEvent(Smart_p->EVTHandle,
                                      Smart_p->DeviceName,
                                      STSMART_EV_CARD_RESET,
                                      &Smart_p->EvtId[EventToId(STSMART_EV_CARD_RESET)]
                                     ) != ST_NO_ERROR ||
            STEVT_RegisterDeviceEvent(Smart_p->EVTHandle,
                                      Smart_p->DeviceName,
                                      STSMART_EV_CARD_DEACTIVATED,
                                      &Smart_p->EvtId[EventToId(STSMART_EV_CARD_DEACTIVATED)]
                                     ) != ST_NO_ERROR ||
            STEVT_RegisterDeviceEvent(Smart_p->EVTHandle,
                                      Smart_p->DeviceName,
                                      STSMART_EV_READ_COMPLETED,
                                      &Smart_p->EvtId[EventToId(STSMART_EV_READ_COMPLETED)]
                                     ) != ST_NO_ERROR ||
            STEVT_RegisterDeviceEvent(Smart_p->EVTHandle,
                                      Smart_p->DeviceName,
                                      STSMART_EV_WRITE_COMPLETED,
                                      &Smart_p->EvtId[EventToId(STSMART_EV_WRITE_COMPLETED)]
                                     ) != ST_NO_ERROR ||
            STEVT_RegisterDeviceEvent(Smart_p->EVTHandle,
                                      Smart_p->DeviceName,
                                      STSMART_EV_TRANSFER_COMPLETED,
                                      &Smart_p->EvtId[EventToId(STSMART_EV_TRANSFER_COMPLETED)]
                                     ) != ST_NO_ERROR
            )

        {
            /* Event registration failed */
            STEVT_Close(Smart_p->EVTHandle); /* Close handle to EVT */
            Error = STSMART_ERROR_EVT_REGISTER;
        }
    }
    else
    {
        Error = STSMART_ERROR_EVT_REGISTER;
    }

    return Error;
} /* SMART_RegisterEvents() */

/*****************************************************************************
SMART_UnregisterEvents()
Description:
    Unregister events with a nominated event handler driver.
Parameters:
    Smart_p,                    pointer to the smartcard control block.
Return Value:
    ST_NO_ERROR,                registered events ok.
    STSMART_ERROR_EVT_REGISTER, problem registering events.
See Also:
    Nothing
*****************************************************************************/

static ST_ErrorCode_t SMART_UnregisterEvents(SMART_ControlBlock_t *Smart_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    /* Unregister all events */
    if ( STEVT_UnregisterDeviceEvent(Smart_p->EVTHandle,
                                     Smart_p->DeviceName,
                                     STSMART_EV_CARD_INSERTED) != ST_NO_ERROR ||
         STEVT_UnregisterDeviceEvent(Smart_p->EVTHandle,
                                     Smart_p->DeviceName,
                                     STSMART_EV_CARD_REMOVED)  != ST_NO_ERROR ||
         STEVT_UnregisterDeviceEvent(Smart_p->EVTHandle,
                                     Smart_p->DeviceName,
                                     STSMART_EV_CARD_RESET)  != ST_NO_ERROR ||
         STEVT_UnregisterDeviceEvent(Smart_p->EVTHandle,
                                     Smart_p->DeviceName,
                                     STSMART_EV_CARD_DEACTIVATED)  != ST_NO_ERROR ||
         STEVT_UnregisterDeviceEvent(Smart_p->EVTHandle,
                                     Smart_p->DeviceName,
                                     STSMART_EV_READ_COMPLETED)  != ST_NO_ERROR ||
         STEVT_UnregisterDeviceEvent(Smart_p->EVTHandle,
                                     Smart_p->DeviceName,
                                     STSMART_EV_WRITE_COMPLETED)  != ST_NO_ERROR ||
         STEVT_UnregisterDeviceEvent(Smart_p->EVTHandle,
                                     Smart_p->DeviceName,
                                     STSMART_EV_TRANSFER_COMPLETED)  != ST_NO_ERROR ||
         STEVT_Close(Smart_p->EVTHandle) != ST_NO_ERROR )
    {
        /* Problem unregisterting / closing */
        Error = STSMART_ERROR_EVT_UNREGISTER;
    }

    return Error;
} /* SMART_UnregisterEvents() */

/* End of scapi.c */
