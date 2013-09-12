/*****************************************************************************
File Name   : stblast.c

Description : STBLAST driver.

History     :

    08/07/03  Added TaskName Identificable(DDTS 20059)

Copyright (C) 2002 STMicroelectronics

Reference   : ST API Definition "ST Blaster Driver API" DVD-API-138
*****************************************************************************/

/* Includes --------------------------------------------------------------- */
#if !defined(ST_OSLINUX)
#include <string.h>                     /* C libs */
#include <stdio.h>
#endif

#include "stos.h"
#include "stddefs.h"

#if !defined(ST_OSLINUX)
#include "stlite.h"                     /* Standard includes */
#endif

#include "stpio.h"                      /* Dependencies */
#include "stevt.h"

#include "stblast.h"                    /* STAPI header file */
#include "timer.h"                      /* Private includes */
#include "blastint.h"
#include "blasthw.h"
#include "stcommon.h"

/* Private types/constants ------------------------------------------------ */
#ifdef ST_OSLINUX
#define STBLAST_MAX_SYMBOL_BUFFER_SIZE 200
#define STBLAST_MAX_CMD_BUFFER_SIZE 200
#endif

/* #define BLAST_API_DEBUG 1 */
#define BLAST_PREEMPT_KERNEL 1

/* All timer tasks created by the driver have a configurable task
 * priority.  This may be overrided in an external system header file.
 */
#ifndef STBLAST_TIMER_TASK_PRIORITY

#define STBLAST_TIMER_TASK_PRIORITY      MAX_USER_PRIORITY
#endif

#ifndef STBLAST_TASK_PRIORITY
#define STBLAST_TASK_PRIORITY            MIN_USER_PRIORITY
#endif

#ifndef STBLAST_RECEIVE_STACK_SIZE
#ifdef ST_OS21
#define STBLAST_RECEIVE_STACK_SIZE       20*1024
#else
#define STBLAST_RECEIVE_STACK_SIZE       4*1024
#endif
#endif
#define BlastDeviceSuffix "Receive"
#define MAX_TIMER_DEVICE_NAME \
(ST_MAX_DEVICE_NAME + sizeof(BlastDeviceSuffix)/sizeof(BlastDeviceSuffix[0]) - 1)
char BlastDeviceName[MAX_TIMER_DEVICE_NAME];
/* Private Variables ------------------------------------------------------ */


void DoDecode(BLAST_ControlBlock_t *Blaster_p);
/*****************************************************************************
BLASTQueueHead_p points to the head of the queue of IR blaster devices that
have been initialized i.e., STBLAST_Init() has been called to append them
to the queue.
At system startup, BLASTQueueHead_p will be NULL.  When the first blaster is
initialized, BLASTQueueHead_p will point to the BLAST control block for that
IR blaster once STBLAST_Init() has been called.
*****************************************************************************/

/* Queue of initialized blaster devices */
static BLAST_ControlBlock_t *BLASTQueueHead_p = NULL;

/* To ensure parts of stblast_init are atomic */
static semaphore_t *InitSemaphore_p = NULL;

/* How many copies have been initialised */
static U32 BLASTInstanceCount = 0;

/* Private Macros --------------------------------------------------------- */

/* Private Function Prototypes -------------------------------------------- */
    
static BOOL BLAST_IsInit(const ST_DeviceName_t DeviceName);
static void BLAST_QueueRemove(BLAST_ControlBlock_t *QueueItem_p);
static void BLAST_QueueAdd(BLAST_ControlBlock_t *QueueItem_p);

static U8 BLAST_PIOBitFromBitMask(U8 BitMask);
static ST_ErrorCode_t BLAST_EvtRegister(const ST_DeviceName_t EVTDeviceName,
                                 BLAST_ControlBlock_t *Blaster_p);
static ST_ErrorCode_t BLAST_EvtUnregister(BLAST_ControlBlock_t *Blaster_p);

static void BLAST_WriteCallback(BLAST_ControlBlock_t *Blaster_p);
static void BLAST_ReadCallback(BLAST_ControlBlock_t *Blaster_p);

/* STBLAST API routines (alphabetical order) ------------------------------- */

/*****************************************************************************
STBLAST_Abort()
Description:
    Abort any pending IO associated with the handle.
Parameters:
    Handle, the device handle.
Return Value:
    ST_NO_ERROR,                no errors
    ST_ERROR_INVALID_HANDLE,    the device handle is invalid
See Also:
    STBLAST_RcSend()
    STBLAST_Read()
    STBLAST_Write()
*****************************************************************************/
ST_ErrorCode_t STBLAST_Abort(STBLAST_Handle_t Handle)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    BLAST_ControlBlock_t *Blaster_p;

    Blaster_p = BLAST_GetControlBlockFromHandle(Handle);

    if(Blaster_p != NULL)              /* Valid control block? */
    {
        STOS_InterruptLock();            /* Acquire lock */

        if(Blaster_p->Operation != NULL) /* Operation pending? */
        {
            /* Set the operation status -- lock out interrupt handler
            * whilst we do this, as the ISR reads this field and
            */
            Blaster_p->Operation->Status = STBLAST_ERROR_ABORT;
            STOS_InterruptUnlock();         /* Release lock */

            /* Signal the timer to abort */
            stblast_TimerSignal(&Blaster_p->Timer, FALSE);
        }
        else                            /* No operation? */
        {
            STOS_InterruptUnlock();
        }
    }
    else
    {
        /* Handle is invalid */
        error = ST_ERROR_INVALID_HANDLE;
    }

    /* Common exit */
    return error;
}

/*****************************************************************************
STBLAST_Close()
Description:
    Closes an IR Blaster device associated with a handle
Parameters:
    Handle,         an open device handle
Return Value:
    ST_NO_ERROR                     no error
    ST_ERROR_INVALID_HANDLE         handle invalid or not open
    STBLAST_ERROR_EVENT_UNREGISTER  error occurred while unregistering events
See Also:
    STBLAST_Open()
*****************************************************************************/
ST_ErrorCode_t STBLAST_Close(STBLAST_Handle_t Handle)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    BLAST_ControlBlock_t *Blaster_p;
    BLAST_BlastRegs_t *BlastRegs_p;

    /* Obtain the control block from the passed in handle */
    Blaster_p = BLAST_GetControlBlockFromHandle(Handle);

    if(Blaster_p != NULL)
    {
        /* Make sure nothing's queued in the meantime */
        STOS_SemaphoreWait(Blaster_p->LockSemaphore_p);

        /* Attempt to abort all activity */
        error = STBLAST_Abort(Handle);
#if defined(ST_OSLINUX)
        if (error != ST_NO_ERROR)
        {
            printk(KERN_ALERT "BLAST API: STAPI_Close() Abort Failed \n");
        }
#endif
        BlastRegs_p = &Blaster_p->Regs;

        /* Disable interrupts and rx/tx */
        if((Blaster_p->InitParams.DeviceType == STBLAST_DEVICE_IR_RECEIVER) ||
            (Blaster_p->InitParams.DeviceType == STBLAST_DEVICE_UHF_RECEIVER)
           )
        {
            BLAST_RxDisable(BlastRegs_p);
            BLAST_RxDisableInt(BlastRegs_p);
        }
        else
        {
            BLAST_TxDisable(BlastRegs_p);
            BLAST_TxDisableInt(BlastRegs_p);
        }

        /* Invalidate user's handle from hereon */
        Blaster_p->Handle = 0;

        /* Release the semaphore */
        STOS_SemaphoreSignal(Blaster_p->LockSemaphore_p);
    }
    else
    {
        /* The handle is invalid */
        error = ST_ERROR_INVALID_HANDLE;
    }

    return error;
}

/*****************************************************************************
STBLAST_GetCapability()
Description:
    Get information regarding the driver's capabilities.
Parameters:
    DeviceName      the device name for the IR Blaster device
    Capablity       pointer to the capability return structure
Return Value:
    ST_NO_ERROR                 no error
    ST_ERROR_UNKNOWN_DEVICE     device not initialized
    ST_ERROR_BAD_PARAMETER      Bad Parameter
See Also:
    STBLAST_Capability_t
*****************************************************************************/
ST_ErrorCode_t STBLAST_GetCapability(const ST_DeviceName_t DeviceName,
                                     STBLAST_Capability_t *Capability
                                    )
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    BLAST_ControlBlock_t *Blaster_p;

    if(Capability == NULL)
    {
        error = ST_ERROR_BAD_PARAMETER;
    }

    if(error == ST_NO_ERROR)
    {
        Blaster_p = BLAST_GetControlBlockFromName(DeviceName);

        if(Blaster_p != NULL)
        {
            *Capability = Blaster_p->Capability;
        }
        else
        {
            error = ST_ERROR_UNKNOWN_DEVICE;
        }
    }

    return error;
}

/*****************************************************************************
STBLAST_GetProtocol()
Description:
    Returns the configuation of the protocol in use
Parameters:
    Handle          an open handle, as returned by STBLAST_Open
    Protocol      pointer to a structure defining the RC protocol in use
Return Value:
    ST_NO_ERROR                         no error
    ST_ERROR_BAD_PARAMETER              one or more parameters was invalid
    ST_ERROR_INVALID_HANDLE             device handle is invalid
    STBLAST_ERROR_INVALID_DEVICE_TYPE   Handle does not correspond to a
                                        transmitter device
See Also:
    STBLAST_Protocol_t
    STBLAST_SetProtocol()
*****************************************************************************/
ST_ErrorCode_t STBLAST_GetProtocol( STBLAST_Handle_t Handle,
                                    STBLAST_Protocol_t *Protocol,
                                    STBLAST_ProtocolParams_t *ProtocolParams
                                    )
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    BLAST_ControlBlock_t *Blaster_p;


    /* Don't want to assign to a NULL pointer, that's bad... */
    if((Protocol == NULL) || (ProtocolParams == NULL) )
    {
        error = ST_ERROR_BAD_PARAMETER;
    }

    if(error == ST_NO_ERROR)
    {
        /* That seems okay; has this thing been initialised/opened? */
        Blaster_p = BLAST_GetControlBlockFromHandle(Handle);

        if(Blaster_p != NULL)
        {
            /* semaphore-protected, to avoid conflict with setrcprotocol */
            STOS_SemaphoreWait(Blaster_p->LockSemaphore_p);
            *Protocol = Blaster_p->Protocol;
            *ProtocolParams = Blaster_p->ProtocolParams;
            STOS_SemaphoreSignal(Blaster_p->LockSemaphore_p);
        }
        else
        {
            error = ST_ERROR_INVALID_HANDLE;
        }
    }

    return error;
}

/*****************************************************************************
STBLAST_GetRevision()
Description:
    Gets the current revision of the driver
Parameters:
Return Value:
    Returns the revision of the device driver
See Also:
    ST_Revision_t
*****************************************************************************/
ST_Revision_t STBLAST_GetRevision(void)
{
   	 return((ST_Revision_t)STBLAST_REVISION);
}

/*****************************************************************************
STBLAST_Init()
Description:
    Initialises the named IR Blaster device

    NOTES: The very first call to this routine will create a semaphore
    for locking access to the blaster queue.  The semaphore will be deleted
    during the final call to STBLAST_Term().

Parameters:
    DeviceName      Name to be associated with the IR Blaster device upon
                    initialisation
    InitParams      Pointer to the initialisation parameter structure

Return Value:
    ST_NO_ERROR                     No error
    ST_ERROR_ALREADY_INITIALIZED    A previous init call has not been
                                    terminated
    ST_ERROR_BAD_PARAMETER          One or more parameters was invalid
    ST_ERROR_FEATURE_NOT_SUPPORTED  The device type is not supported
    ST_ERROR_INTERRUPT_INSTALL      Unable to install ISR
    ST_ERROR_NO_MEMORY              Unable to allocate required memory from
                                    partition given in InitParams
    STBLAST_ERROR_EVENT_REGISTER    An error occurred while attempting to
                                    register the driver events.
    STBLAST_ERROR_PIO               PIO Error

See Also:
    STBLAST_InitParams_t
    STBLAST_Close()
*****************************************************************************/
ST_ErrorCode_t STBLAST_Init(const ST_DeviceName_t DeviceName,
                            const STBLAST_InitParams_t *InitParams
                           )
{
    BLAST_BlastRegs_t *BlastRegs_p;
    ST_ErrorCode_t error = ST_NO_ERROR;
    BLAST_ControlBlock_t *Blaster_p = NULL;
    partition_t *DriverPartition = NULL;
    U32 MemoryInit = 0;
    U32 PIOInit = 0;
    U32 TimerInit = 0;
    U32 SemaphoreInit = 0;
    U8 PinNumber;
    /* Make sure the semaphore has been created */
#if defined(ST_OSLINUX)
    unsigned int flags = 0;
#if BLAST_PREEMPT_KERNEL
    preempt_disable();
#else
    STOS_TaskLock(current);
#endif /* BLAST_PREEMPT_KERNEL */
#else
    STOS_TaskLock();
#endif /* ST_OSLINUX */

    if(InitSemaphore_p == NULL)
    {
        InitSemaphore_p = STOS_SemaphoreCreateFifo(NULL,1);
    }

#if defined(ST_OSLINUX)
#if BLAST_PREEMPT_KERNEL
    preempt_enable();
#else
    STOS_TaskUnlock(current);
#endif /* BLAST_PREEMPT_KERNEL */
#else
    STOS_TaskUnlock();
#endif /* ST_OSLINUX */


    /* Check the name */
    if((DeviceName == NULL) ||
        (strlen(DeviceName) > ST_MAX_DEVICE_NAME))
    {
        error = ST_ERROR_BAD_PARAMETER;
    }

    /* Check for some obviously bad parameters, before doing anything else */
    if((InitParams != NULL) && (error == ST_NO_ERROR))
    {
#if defined(ST_OSLINUX)
        if((InitParams->DeviceType < STBLAST_DEVICE_IR_TRANSMITTER) ||
            (InitParams->DeviceType >= STBLAST_DEVICE_UNKNOWN) ||
            (InitParams->BaseAddress == 0) ||
            (InitParams->SymbolBufferSize == 0) ||
            (InitParams->SymbolBufferSize > STBLAST_MAX_SYMBOL_BUFFER_SIZE) ||
            (InitParams->CmdBufferSize == 0) ||
            (InitParams->CmdBufferSize > STBLAST_MAX_CMD_BUFFER_SIZE) ||
            (InitParams->InterruptNumber == 0) ||
            (strlen(InitParams->EVTDeviceName) == 0) ||
            (InitParams->ClockSpeed == 0)
            ) /* unnessary to check partition and interrupt level in STLinux */
#else
        if((InitParams->DeviceType < STBLAST_DEVICE_IR_TRANSMITTER) ||
            (InitParams->DeviceType >= STBLAST_DEVICE_UNKNOWN) ||
            (InitParams->DriverPartition == NULL) ||
            (InitParams->BaseAddress == 0) ||
            (InitParams->SymbolBufferSize == 0) ||
            (strlen(InitParams->EVTDeviceName) == 0) ||
            (InitParams->ClockSpeed == 0)
           )
#endif /* ST_OSLINUX */
        {
            error = ST_ERROR_BAD_PARAMETER;
        }

        /* Check PIO pins */
        if(error == ST_NO_ERROR)
        {
            if(InitParams->DeviceType == STBLAST_DEVICE_IR_TRANSMITTER)
            {
                if((strlen(InitParams->TxPin.PortName) == 0)
                   || (InitParams->TxPin.BitMask == 0)
#if !defined(STBLAST_NOT_USE_IRB_JACK)
                    || (strlen(InitParams->TxInvPin.PortName) == 0)
                    || (InitParams->TxInvPin.BitMask == 0)
#endif
                    )
                {
                    error = ST_ERROR_BAD_PARAMETER;
                }
            }
            else if((InitParams->DeviceType == STBLAST_DEVICE_IR_RECEIVER) ||
                    (InitParams->DeviceType == STBLAST_DEVICE_UHF_RECEIVER))
            {
                if((strlen(InitParams->RxPin.PortName) == 0) ||
                    (InitParams->RxPin.BitMask == 0))
                {
                    error = ST_ERROR_BAD_PARAMETER;
                }
            }
	    }
    }
    else
    {
        error = ST_ERROR_BAD_PARAMETER;
    }

    /* Allocate memory, install interrupts etc */
    if (error == ST_NO_ERROR)
    {
        /* Enter criticial region */
        STOS_SemaphoreWait(InitSemaphore_p);

        if(!BLAST_IsInit(DeviceName))
        {
#if defined(ST_OSLINUX) 
#if defined(BLAST_API_DEBUG)
            printk("BLAST API: Initialize BLAST Device %s ... \n", \
                   DeviceName);
#endif
#endif
            /* Allocate memory for the control block */
            DriverPartition = (partition_t *)InitParams->DriverPartition;

            Blaster_p = (BLAST_ControlBlock_t *)STOS_MemoryAllocate(
                        DriverPartition,sizeof(BLAST_ControlBlock_t));

#ifndef ST_OSLINUX
            /* clear the memory */
            memset(Blaster_p, 0, sizeof(BLAST_ControlBlock_t));
#endif

            if(Blaster_p != NULL)      /* Allocated ok? */
            {
                MemoryInit++;
                /* Allocate symbol buffer */
                Blaster_p->SymbolBufBase_p =
                        (STBLAST_Symbol_t *)STOS_MemoryAllocate(
                                DriverPartition,
                                sizeof(STBLAST_Symbol_t) * InitParams->SymbolBufferSize);

                /* Set size of symbol buffer */
                if(Blaster_p->SymbolBufBase_p != NULL) /* Alloc ok? */
                {
                    MemoryInit++;
                    Blaster_p->SymbolBufSize = InitParams->SymbolBufferSize;
#if defined(ST_OSLINUX)
#if defined(BLAST_API_DEBUG)
                    printk("BLAST API: symbol buffer allocated Done \n");
#endif
#endif
                }
#if defined(ST_OSLINUX)
                else
                {
                    printk("BLAST API: symbol buffer allocated failed \n");
                }
#endif

                /* Allocate command buffer */
#if defined(ST_OSLINUX)
                if (MemoryInit == 2)
                {
                    Blaster_p->CmdBufBase_p =
                                       (U32*)STOS_MemoryAllocate(DriverPartition,
                                            sizeof(U32) * InitParams->CmdBufferSize);

                    if (Blaster_p->CmdBufBase_p != NULL)
                    {
                        Blaster_p->CmdBufSize = InitParams->CmdBufferSize;
                        MemoryInit++;
#if defined(BLAST_API_DEBUG)
                        printk("BLAST API: command buffer allocated Done \n");
#endif
                    }
                    else
                    {
                        printk("BLAST API: command buffer allocated failed \n");;
                	}
                }
#endif

            }

#if defined(ST_OSLINUX)
            if(MemoryInit == 3)
#else
            if(MemoryInit == 2)
#endif
            {
                STPIO_OpenParams_t PIOOpenParams;

                ST_ErrorCode_t PIOError;

                /* Copy parameters into control block */
                Blaster_p->InitParams = *InitParams;
                strncpy(Blaster_p->DeviceName, DeviceName, ST_MAX_DEVICE_NAME);
                Blaster_p->DeviceType = InitParams->DeviceType;
                PIOOpenParams.IntHandler = NULL;
                
                /* Open PIO pins */

                if((InitParams->DeviceType == STBLAST_DEVICE_IR_RECEIVER) ||
                    (InitParams->DeviceType == STBLAST_DEVICE_UHF_RECEIVER))                
                {
#if defined(ST_OSLINUX)
#if defined(BLAST_API_DEBUG)
                    printk(KERN_ALERT "BLAST API: configure RX PIO PORT[%s] BIT[%d] \n",
                              InitParams->RxPin.PortName, InitParams->RxPin.BitMask);
#endif
#endif
                    PinNumber = BLAST_PIOBitFromBitMask(InitParams->RxPin.BitMask);
                    PIOOpenParams.BitConfigure[PinNumber] = STPIO_BIT_INPUT;
                    PIOOpenParams.ReservedBits = InitParams->RxPin.BitMask;
                    PIOError = STPIO_Open(InitParams->RxPin.PortName, &PIOOpenParams,
                                          &Blaster_p->RxPIOHandle);
                    if(PIOError == ST_NO_ERROR)
                    {
#if defined(ST_OSLINUX)                    	
#if defined(BLAST_API_DEBUG)
                        printk(KERN_ALERT "BLAST API: Request RX pio Done\n");
#endif                 
#endif   	
                        PIOInit++;
                    }
                }
				else
                {
#if defined(ST_OSLINUX)
#if defined(BLAST_API_DEBUG)
                    printk(KERN_ALERT "BLAST API: configure TX PIO PORT[%d] BIT[%d] \n",
                              InitParams->TxPioPortNum, InitParams->TxPioPinNum);
#endif
#endif
                    PinNumber = BLAST_PIOBitFromBitMask(InitParams->TxPin.BitMask);
                    PIOOpenParams.BitConfigure[PinNumber] = STPIO_BIT_ALTERNATE_OUTPUT;
                    PIOOpenParams.ReservedBits = InitParams->TxPin.BitMask;
                    
                    PIOError = STPIO_Open(InitParams->TxPin.PortName, &PIOOpenParams,
                            &Blaster_p->TxPIOHandlePPM);
                    if(PIOError == ST_NO_ERROR)
                    {
                        PIOInit++;
#if defined(ST_OSLINUX)                     
#if defined(BLAST_API_DEBUG)
                        printk(KERN_ALERT "BLAST API: Request TX pio Done\n");
#endif       
#endif 
                 
#if !defined(STBLAST_NOT_USE_IRB_JACK)
                        PinNumber = BLAST_PIOBitFromBitMask(InitParams->TxInvPin.BitMask);
                        PIOOpenParams.BitConfigure[PinNumber] = STPIO_BIT_ALTERNATE_BIDIRECTIONAL;
                        PIOOpenParams.ReservedBits = InitParams->TxInvPin.BitMask;
                        PIOError = STPIO_Open(InitParams->TxInvPin.PortName, &PIOOpenParams,
                                    &Blaster_p->TxPIOHandleJack);

                        if(PIOError == ST_NO_ERROR)
                        {
                            PIOInit++;
                        }
#endif
                    }
                }
                
                if(PIOError != ST_NO_ERROR)
                {
                    error = STBLAST_ERROR_PIO;
                }

                /* Opened PIO successfully? */
                if(error == ST_NO_ERROR)
                {
                    /* Set some flags */
                    Blaster_p->Handle = 0;
                    Blaster_p->ProtocolIsSet = FALSE;

                    if ( error == ST_NO_ERROR)
                    {
                       
			           if((Blaster_p->InitParams.DeviceType == STBLAST_DEVICE_IR_RECEIVER) ||
			              (Blaster_p->InitParams.DeviceType == STBLAST_DEVICE_UHF_RECEIVER))
			           {
                            sprintf(Blaster_p->DecodeTaskName, "STBLAST_ReceiveTask_%s", DeviceName);
                            strcpy(BlastDeviceName,DeviceName);
                            strcat(BlastDeviceName,BlastDeviceSuffix);

                    	    Blaster_p->ReceiveSem_p = STOS_SemaphoreCreateFifo(NULL,0);
				            /* Mesaage Queue creation before task initialization */
                            if(error == ST_NO_ERROR)
                            {
                               Blaster_p->InterruptQueue_p = STOS_MessageQueueCreate(sizeof(BlastInterruptData_t),50);
                               if (Blaster_p->InterruptQueue_p == NULL)
                               {
                                    error = ST_ERROR_NO_MEMORY;
                               }
                            }                    	    
                        	                                                     
                            error = STOS_TaskCreate((void(*)(void *))DoDecode,
                                                    (void*)Blaster_p,
			        				                Blaster_p->InitParams.DriverPartition,
				                                    STBLAST_RECEIVE_STACK_SIZE,
				                                    &Blaster_p->Task_p,
				                                    Blaster_p->InitParams.DriverPartition,
				                                    &Blaster_p->ReceiveTask,
				                                    &Blaster_p->TaskDescriptor,
				                                    STBLAST_TASK_PRIORITY,
				                                    Blaster_p->DecodeTaskName,
				                                    (task_flags_t) task_flags_no_min_stack_size );
                                                    

                            if (error != ST_NO_ERROR)
                            {
                                /* Unable to create the recieve task */
                                error = ST_ERROR_NO_MEMORY;
                            }
                            else
                            {
                                Blaster_p->ReceiveTaskTerm = FALSE;
                            }
                       }
                       else
                       {
                                Blaster_p->ReceiveSem_p = NULL;
                                Blaster_p->ReceiveTask  = NULL;
                       }
                    }
                                   
                    if (error == ST_NO_ERROR )
                    {
                        /* Used for ensuring certain things are atomic */

                        Blaster_p->LockSemaphore_p = STOS_SemaphoreCreateFifo(NULL,1);

                        SemaphoreInit++;

                        /* Set the register map up */
#if defined(ST_OSLINUX)
#if defined(BLAST_API_DEBUG)
                        printk("BLAST API: Map Register ... \n");
#endif
#endif
                        BLAST_MapRegisters(Blaster_p);
                        BlastRegs_p = &Blaster_p->Regs;

                        /* Ascertain capabilities */
                        Blaster_p->Capability.Device = Blaster_p->DeviceType;

                        /*  Type casted to remove lint warning */
                        Blaster_p->Capability.SupportedCodingTypes = (U32)(
                                                                    (U32)(STBLAST_CODING_PULSE) |
                                                                    (U32)(STBLAST_CODING_SPACE) |
                                                                    (U32)(STBLAST_CODING_SHIFT));
                        Blaster_p->Capability.SupportedProtocols =
                                                                (U32)((U32)(STBLAST_PROTOCOL_USER_DEFINED) |
                                                                (U32)(STBLAST_PROTOCOL_RC5) |
                                                                (U32)(STBLAST_LOW_LATENCY_PROTOCOL) |
                                                                (U32)(STBLAST_PROTOCOL_RC6A)|
                                                                (U32)(STBLAST_PROTOCOL_RC6_MODE0)|
                                                                (U32)(STBLAST_PROTOCOL_RCMM)|
                                                                (U32)(STBLAST_PROTOCOL_RCRF8)|
                                                                (U32)(STBLAST_PROTOCOL_MULTIPLE)|
                                                                (U32)(STBLAST_PROTOCOL_RUWIDO) |
                                                                (U32)(STBLAST_PROTOCOL_RSTEP) |
                                                                (U32)(STBLAST_PROTOCOL_RMAP) |
                                                                (U32)(STBLAST_PROTOCOL_RMAP_DOUBLEBIT) |
                                                                (U32)(STBLAST_LOW_LATENCY_PRO_PROTOCOL));

#if defined(ST_5518)
                        Blaster_p->Capability.ProgrammableDutyCycle = FALSE;
#else
                        Blaster_p->DutyCycle = 50;
                        Blaster_p->Capability.ProgrammableDutyCycle = TRUE;
#endif

                        if((InitParams->DeviceType != STBLAST_DEVICE_IR_RECEIVER)&&
                            (Blaster_p->DeviceType != STBLAST_DEVICE_UHF_RECEIVER))
                        {
                            /* Disable transmitter */
                            BLAST_TxDisableInt(BlastRegs_p);
                            BLAST_TxDisable(BlastRegs_p);
                            BLAST_TxSetIE(BlastRegs_p, TXIE_TWOEMPTY);

                        }
                        else
                        {
                            /* Disable receiver */
                            BLAST_RxDisableInt(BlastRegs_p);
                            BLAST_RxDisable(BlastRegs_p);
                            BLAST_RxSetIE(BlastRegs_p, RXIE_ONEFULL);

                        }

                        /* Attempt to install the interrupt handler */
#if defined(ST_OSLINUX)
#ifndef  BLAST_PREEMPT_KERNEL    /* being removed because request_irq can sleep in preempt kernel */
                        spin_lock_irqsave(&current->sighand->siglock, flags);
#endif
#else
                        STOS_InterruptLock();       /* Must be atomic */
#endif

                        if(BLASTInstanceCount == 0)
                        {
#if defined(ST_OSLINUX)
#if defined(BLAST_API_DEBUG)
                            printk("BLAST API: Request Blast IRQ ...  \n");
                            printk("BLAST API: Interrupt Number %d \n", InitParams->InterruptNumber);
#endif

                            if(request_irq((int)(InitParams->InterruptNumber),
                                           BLAST_InterruptHandler,
                                           0,
                                           "STBLAST",
                                           (void *)&BLASTQueueHead_p) == 0) /* success */
#else
                            if(interrupt_install(InitParams->InterruptNumber,
                                    InitParams->InterruptLevel,
                                    (void(*)(void*))BLAST_InterruptHandler,
                                    &BLASTQueueHead_p ) == 0 /* success */)
#endif
                            {

                                /* Enable interrupts at the specified level */
#if defined(ST_OSLINUX)
#if defined(BLAST_API_DEBUG)
                                printk("BLAST API: Request Blast IRQ Done  \n");
#endif
                                /* enable_irq will cause irq badness */
			                    /* enable_irq(InitParams->InterruptNumber); */

                                /* Everything is going well, so we can now finally
                                 * append this item to the UART queue.
                                 */

                                BLAST_QueueAdd(Blaster_p);
                                BLASTInstanceCount++;
#else
#ifndef ST_OS21
#ifdef STAPI_INTERRUPT_BY_NUMBER
                                if (interrupt_enable_number(InitParams->InterruptNumber)== 0 )
#else
                                if (interrupt_enable(InitParams->InterruptLevel) == 0)
#endif
#else /* ST_OS21 */
                                if (interrupt_enable_number(InitParams->InterruptNumber) == 0)

#endif

                                {
                                    STOS_InterruptUnlock(); /* Re-enable interrupts */

                                    /* Everything is going well, so we can now finally
                                    * append this item to the UART queue.
                                    */
                                    BLAST_QueueAdd(Blaster_p);
                                    BLASTInstanceCount++;
                                }
                                else
                                {
                                    STOS_InterruptUnlock(); /* Re-enable interrupts */

                                    /* Error enabling interrupts */
                                    error = ST_ERROR_BAD_PARAMETER;
                                }
#endif /* ST_OSLINUX */
                            }
                            else
                            {
#if defined(ST_OSLINUX)
#ifdef KERNEL_API_DEBUG
                                printk("KERNEL API: Request Blast IRQ Failed \n");
#endif
#ifndef BLAST_PREEMPT_KERNEL /* not correct under preempt kernel because request_irq() sleep */
                                spin_unlock_irqrestore(&current->sighand->siglock, flags);
#endif /* BLAST_PREEMPT_KERNEL */
#else
                                STOS_InterruptUnlock();     /* Re-enable interrupts */
#endif /* ST_OSLINUX */

                                /* Error installing interrupts */
                                error = ST_ERROR_INTERRUPT_INSTALL;
                            }
                        }
                        else
                        {
#if defined(ST_OSLINUX)
#ifndef BLAST_PREEMPT_KERNEL
                            spin_unlock_irqrestore (&current->sighand->siglock, flags);
#else
                            local_irq_restore(flags);
#endif /* BLAST_PREEMPT_KERNEL */
#else
                            STOS_InterruptUnlock();     /* Re-enable interrupts */
#endif


                            /* Everything is going well, so we can now finally
                            * append this item to the STBLAST queue.
                            */
                            BLAST_QueueAdd(Blaster_p);
                            BLASTInstanceCount++;
                        }
                        
                    	if(stblast_TimerCreate(DeviceName,&Blaster_p->Timer,
                        (U32)STBLAST_TIMER_TASK_PRIORITY))
                    	{
#if defined(ST_OSLINUX)
                        	printk("BLAST API: Create Timer task Failed \n");
#endif
                        	error = ST_ERROR_NO_MEMORY;
                    	}
                   	    else
                    	{
#if defined(ST_OSLINUX)
#if defined(BLAST_API_DEBUG)
                        	printk("BLAST API: Create Timer task Done \n");
#endif
#endif
                        	TimerInit++;
                    	}
                                                
                        /* Register events */
                        if(error == ST_NO_ERROR) /* david added to check install irq */
                        {
                            error = BLAST_EvtRegister(Blaster_p->InitParams.EVTDeviceName,
                                                      Blaster_p);
#if defined(ST_OSLINUX)
                            if (error != ST_NO_ERROR)
                            {
                                printk("BLAST API: BLAST_Evt Regist Failed \n");
                            }
#endif
                        }
                    }
                }

                /* Release semaphore */
                STOS_SemaphoreSignal(InitSemaphore_p);
            }
            else
            {
                /* Memory allocate failed */
                error = ST_ERROR_NO_MEMORY;
                STOS_SemaphoreSignal(InitSemaphore_p);
            }
        }
        else
        {
            /* The blaster is already in use */
            error = ST_ERROR_ALREADY_INITIALIZED;
            STOS_SemaphoreSignal(InitSemaphore_p);
        }
    }

    if (error != ST_NO_ERROR)           /* All ok? */
    {
#if defined(ST_OSLINUX)
#if defined(BLAST_API_DEBUG)
        printk("BLAST API: STAPI_Init() Failed and Deallocate Resources ...\n");
#endif
#endif
#if !defined(ST_OSLINUX)
        /* Deallocate memory */
          /* Deallocate memory */
        if ( MemoryInit > 0 )
        {
            /* Free symbol buffer */
            STOS_MemoryDeallocate(DriverPartition,Blaster_p->SymbolBufBase_p);
            MemoryInit--;

            if ( MemoryInit > 0 )
            {
                /* Free up the control structure */
                STOS_MemoryDeallocate(DriverPartition, Blaster_p);
                MemoryInit--;

            }
        }

#endif

        /* Delete allocated timers */
        if (TimerInit > 0)
        {
            stblast_TimerDelete(DeviceName,&Blaster_p->Timer);
        }

        /* Delete lock semaphore */
        if (SemaphoreInit > 0)
        {
            STOS_SemaphoreDelete(NULL,Blaster_p->LockSemaphore_p);
        }

        /* Close allocated PIO handles */
        if (PIOInit > 0)
        {
 			if((InitParams->DeviceType == STBLAST_DEVICE_IR_RECEIVER) ||
               (InitParams->DeviceType == STBLAST_DEVICE_UHF_RECEIVER))  
            {
                STPIO_Close(Blaster_p->RxPIOHandle);
            }
            else
            {
                STPIO_Close(Blaster_p->TxPIOHandlePPM);
	            PIOInit--;
                if (PIOInit > 0)
                {
#if !defined(STBLAST_NOT_USE_IRB_JACK)
                    STPIO_Close(Blaster_p->TxPIOHandleJack);
#endif
                }
            }
        }
#if defined(ST_OSLINUX)
        /* fix the bug which not existed under OS20 but Linux kernel space */
        /* Deallocate memory */
        if (MemoryInit > 0)
        {
            /* Free cmd buffer */
            if (MemoryInit > 2)
            {
                STOS_MemoryDeallocate(DriverPartition,
                                  Blaster_p->CmdBufBase_p);
                MemoryInit--;
                printk("BLAST API: STAPI_Init() deallocate command buffer \n");
            }

            /* Free symbol buffer */
            if(MemoryInit > 1)
            {
                printk("BLAST API: STAPI_Init() Blaster_p 0x%x \n", (U32)Blaster_p);
                STOS_MemoryDeallocate(DriverPartition,
                                  Blaster_p->SymbolBufBase_p);
                printk("BLAST API: STAPI_Init() deallocate symbol buffer \n");
            }

            /* Free up the control structure */
            printk("BLAST API: STAPI_Init() Blaster_p 0x%x \n", (U32)Blaster_p);
            STOS_MemoryDeallocate(DriverPartition,
                              Blaster_p);
            MemoryInit--;
            printk("BLAST API: STAPI_Init() deallocate blaster_p \n");
        }
#endif /* fix the bug which not existed under OS20 */

    }
    return error;
}

/*****************************************************************************
STBLAST_Open()
Description:
    Open an IR Blaster device and associate it with a handle
Parameters:
    DeviceName      Name of the IR Blaster device to make a connection to
    OpenParams      Pointer to the open parameters structure
    Handle          The returned handle of the opened device
Return Value:
    ST_NO_ERROR                     no error
    ST_ERROR_BAD_PARAMETER          one or more parameters was invalid
    ST_ERROR_NO_FREE_HANDLES        the device has already been opened
    ST_ERROR_UNKNOWN_DEVICE         invalid device name
See Also:
    STBLAST_OpenParams_t
*****************************************************************************/
ST_ErrorCode_t STBLAST_Open(const ST_DeviceName_t DeviceName,
                            const STBLAST_OpenParams_t *OpenParams,
                            STBLAST_Handle_t *Handle
                           )
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    BLAST_ControlBlock_t *Blaster_p;
    BLAST_BlastRegs_t *BlasterRegs_p;

    /* Check the name */
    if(DeviceName == NULL)
    {
        error = ST_ERROR_UNKNOWN_DEVICE;
    }
    else
    {
        /* Obtain the BLAST control block associated with the device name */
        Blaster_p = BLAST_GetControlBlockFromName(DeviceName);

        if(Blaster_p != NULL)
        {
            BlasterRegs_p = &Blaster_p->Regs;

            /* Make sure we don't get conflicts */
            STOS_SemaphoreWait(Blaster_p->LockSemaphore_p);

            /* Ensure nobody already has this device open */
            if(Blaster_p->Handle == 0)
            {
                if(error == ST_NO_ERROR)
                {
                    /* Set the handle to a unique non-zero value - using
                    * this particular choice makes it a handy shortcut
                    * for accessing the BLAST control block from the
                    * handle alone.
                    */
                    Blaster_p->Handle = (STBLAST_Handle_t)Blaster_p;
                    *Handle = Blaster_p->Handle;

                    /* Release the semaphore */
                    STOS_SemaphoreSignal(Blaster_p->LockSemaphore_p);

                    /* Copy the open parameters into the BLAST control block */
                    Blaster_p->OpenParams = *OpenParams;

                    if((Blaster_p->InitParams.DeviceType == STBLAST_DEVICE_IR_RECEIVER) ||
                        (Blaster_p->InitParams.DeviceType == STBLAST_DEVICE_UHF_RECEIVER)
                    )
                    {
                        /* Set the noise-suppression parameter */
                        BLAST_RxSetNoiseSuppress(BlasterRegs_p,
                                                OpenParams->RxParams.GlitchWidth);
		                BLAST_RxEnable(BlasterRegs_p);
		                BLAST_RxEnableInt(BlasterRegs_p);
		                
#ifdef NEW_INTERRUPT_MECHANISM
		                BLAST_RxSetIE(BlasterRegs_p, RXIE_ONEFULL);
#endif
		                if(Blaster_p->InitParams.InputActiveLow)
		                {
		                    BLAST_RxPolarityInvert(BlasterRegs_p);
		                }
                    }
                    /* Clear the current operation pointer */
                    Blaster_p->Operation = NULL;
                }
                else
                {
                    STOS_SemaphoreSignal(Blaster_p->LockSemaphore_p);
                }
            }
            else
            {
                /* The handle is already open */
                error = ST_ERROR_NO_FREE_HANDLES;

                /* Release the semaphore */
                STOS_SemaphoreSignal(Blaster_p->LockSemaphore_p);
            }
        }
        else
        {
            /* The named device does not exist */
            error = ST_ERROR_UNKNOWN_DEVICE;
        }
    }

    /* Common exit point */
    return error;
}


/*****************************************************************************
STBLAST_Receive()
Description:
    Receive an RC code using a transmitter device
Parameters:
    Handle              an open handle, as returned by STBLAST_Open
    Buffer              pointer to the buffer to be filled
    NumberToRead        maximum number of symbols to read into buffer
    NumberRead          pointer to the number of symbols successfully read
    ReceiveTimeout      length of timeout, specified in 1ms intervals. A
                        value of 0 specifies an infinite timeout.
Return Value:
    ST_NO_ERROR                     no error
    ST_ERROR_BAD_PARAMETER          one or more parameters was invalid
    ST_ERROR_DEVICE_BUSY            an operation is already in progress on
                                    the device associated with this handle
    ST_ERROR_INVALID_HANDLE         device handle is invalid
    STBLAST_ERROR_INVALID_DEVICE_TYPE   the Handle parameter does not
                                        correspond to a transmitter device
    STBLAST_ERROR_UNDEFINED_PROTOCOL    A protocol has not been specified

See Also:
    STBLAST_Symbol_t
    STBLAST_Abort()
    STBLAST_Open()
*****************************************************************************/
ST_ErrorCode_t STBLAST_Receive(STBLAST_Handle_t Handle,
                                 U32 *Buffer,
                                 U32 NumberToRead,
                                 U32 *NumberRead,
                                 U32 ReceiveTimeout
                                )
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    BLAST_ControlBlock_t *Blaster_p = NULL;
    BLAST_Operation_t *Op = NULL;
    BLAST_BlastRegs_t *BlastRegs_p = NULL;

    Blaster_p = BLAST_GetControlBlockFromHandle(Handle);

    /* Parameter checking */
    if((Buffer == NULL) ||
        (NumberToRead == 0) ||
        (NumberRead == NULL))
    {
        error = ST_ERROR_BAD_PARAMETER;
    }

    /* Ensure that STBLAST_SetProtocol has been called */
    if((!Blaster_p -> ProtocolIsSet) && (error == ST_NO_ERROR) )
    {
        error = STBLAST_ERROR_UNDEFINED_PROTOCOL;
    }

    /* Main body */
    if((Blaster_p != NULL) && (error == ST_NO_ERROR))
    {
          /* Make sure it's the right device type */
        if((Blaster_p->InitParams.DeviceType == STBLAST_DEVICE_IR_RECEIVER) ||
            (Blaster_p->InitParams.DeviceType == STBLAST_DEVICE_UHF_RECEIVER)
           )
        {
            /* Make sure that checks are atomic */
            STOS_SemaphoreWait(Blaster_p->LockSemaphore_p);

#if defined(ST_OSLINUX)
            if(NumberToRead > Blaster_p->CmdBufSize)
            {
                error = ST_ERROR_BAD_PARAMETER;
            }
#endif
            /* Nothing using the blaster at present */
           if((Blaster_p->Operation == NULL) && (error == ST_NO_ERROR))
            {
                /* Take an operation from the "pool" */
                Op = Blaster_p->OperationPool;

                /* Setup the operation */
                Op->Status = (U32)BLAST_OP_PENDING;
                Op->Timeout = ReceiveTimeout * (ST_GetClocksPerSecond()/1000);
                Op->RawMode = FALSE;
#if defined(ST_OSLINUX)
                Op->UserBuf_p = Blaster_p->CmdBufBase_p;
#else
                Op->UserBuf_p = Buffer;
#endif
                Op->UserRemaining = NumberToRead;
                Op->UserBufSize = NumberToRead;

#if defined(ST_OSLINUX)
                Op->UserCount_p = &(Blaster_p->CmdsReceived);
#else
                Op->UserCount_p = NumberRead;
#endif

#if defined(ST_OSLINUX)
                /* Set raw mode flag */
                Blaster_p->LastRawMode = FALSE;
#endif

                /* Reset user count */
#if defined(ST_OSLINUX)
                Blaster_p->SymbolsRead = 0;
                Blaster_p->CmdsReceived = 0;
                Blaster_p->CmdUserBufBase_p = Buffer;
#else
                *NumberRead = 0;
#endif

                /* Reset symbol buffer */
                Blaster_p->SymbolBuf_p = Blaster_p->SymbolBufBase_p;
                Blaster_p->SymbolBufCount = 0;
                Blaster_p->SymbolBufFree = Blaster_p->SymbolBufSize;

                /* Assign it */
                Blaster_p->Operation = Op;

               /* Enable receiver */
                BlastRegs_p = &Blaster_p->Regs;
                BLAST_RxSetMaxSymTime(BlastRegs_p,
                                      Blaster_p->MaxSymTime);


                /* Start the timer task */
                stblast_TimerWait(&Blaster_p->Timer,
                                  (void(*)(void *))BLAST_ReadCallback,
                                  Blaster_p,
                                  &Blaster_p->Operation->Timeout,
                                  FALSE
                                 );

                /* Release the semaphore */
                STOS_SemaphoreSignal(Blaster_p->LockSemaphore_p);
            }
            else
            {
                STOS_SemaphoreSignal(Blaster_p->LockSemaphore_p);
                error = ST_ERROR_DEVICE_BUSY;
            }
        }
        else
        {
            /* Trying to do a read on a transmitter... */
            error = STBLAST_ERROR_INVALID_DEVICE_TYPE;
        }
    }
    else
    {
        if (error == ST_NO_ERROR)
        {
            error = ST_ERROR_INVALID_HANDLE;
        }
    }

    /* Common exit */
    Blaster_p->DecodeKey = TRUE; 
    return error;
}
/*****************************************************************************
STBLAST_ReceiveExt()
Description:
    Receive an RC code using a transmitter device
Parameters:
    Handle              an open handle, as returned by STBLAST_Open
    Buffer              pointer to the buffer to be filled
    NumberToRead        maximum number of symbols to read into buffer
    NumberRead          pointer to the number of symbols successfully read
    ReceiveTimeout      length of timeout, specified in 1ms intervals. A
                        value of 0 specifies an infinite timeout.
Return Value:
    ST_NO_ERROR                     no error
    ST_ERROR_BAD_PARAMETER          one or more parameters was invalid
    ST_ERROR_DEVICE_BUSY            an operation is already in progress on
                                    the device associated with this handle
    ST_ERROR_INVALID_HANDLE         device handle is invalid
    STBLAST_ERROR_INVALID_DEVICE_TYPE   the Handle parameter does not
                                        correspond to a transmitter device
    STBLAST_ERROR_UNDEFINED_PROTOCOL    A protocol has not been specified

See Also:
    STBLAST_Symbol_t
    STBLAST_Abort()
    STBLAST_Open()
*****************************************************************************/
ST_ErrorCode_t STBLAST_ReceiveExt(STBLAST_Handle_t Handle,
                                 U32 *Buffer,
                                 U32 NumberToRead,
                                 U32 *NumberRead,
                                 U32 ReceiveTimeout,
                                 STBLAST_Protocol_t *ProtocolSet
                                )
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    BLAST_ControlBlock_t *Blaster_p = NULL;
    BLAST_Operation_t *Op = NULL;
    BLAST_BlastRegs_t *BlastRegs_p = NULL;

    Blaster_p = BLAST_GetControlBlockFromHandle(Handle);

    /* Parameter checking */
    if((Buffer == NULL) ||
        (NumberToRead == 0) ||
        (NumberRead == NULL))
    {
        error = ST_ERROR_BAD_PARAMETER;
    }
   /* Ensure that STBLAST_SetProtocol has been called */
    if((!Blaster_p -> ProtocolIsSet) && (error == ST_NO_ERROR) )
    {
        error = STBLAST_ERROR_UNDEFINED_PROTOCOL;
    }

    /* Main body */
    if((Blaster_p != NULL) && (error == ST_NO_ERROR))
    {
        /* Make sure it's the right device type */
        if((Blaster_p->InitParams.DeviceType == STBLAST_DEVICE_IR_RECEIVER) ||
            (Blaster_p->InitParams.DeviceType == STBLAST_DEVICE_UHF_RECEIVER)
           )
        {
            /* Make sure that checks are atomic */
            STOS_SemaphoreWait(Blaster_p->LockSemaphore_p);

#if defined(ST_OSLINUX)
            if(NumberToRead > Blaster_p->CmdBufSize)
            {
                error = ST_ERROR_BAD_PARAMETER;
            }
#endif
            /* Nothing using the blaster at present */
            if((Blaster_p->Operation == NULL) && (error == ST_NO_ERROR))
            {
                /* Take an operation from the "pool" */
                Op = Blaster_p->OperationPool;

                /* Setup the operation */
                Op->Status = (U32)BLAST_OP_PENDING;
                Op->Timeout = ReceiveTimeout * (ST_GetClocksPerSecond()/1000);
                Op->RawMode = FALSE;
#if defined(ST_OSLINUX)
                Op->UserBuf_p = Blaster_p->CmdBufBase_p;
#else
                Op->UserBuf_p = Buffer;
#endif
                Op->UserRemaining = NumberToRead;
                Op->UserBufSize = NumberToRead;

#if defined(ST_OSLINUX)
                Op->UserCount_p = &(Blaster_p->CmdsReceived);
#else
                Op->UserCount_p = NumberRead;
#endif

#if defined(ST_OSLINUX)
                /* Set raw mode flag */
                Blaster_p->LastRawMode = FALSE;
#endif

                /* Reset user count */
#if defined(ST_OSLINUX)
                Blaster_p->SymbolsRead = 0;
                Blaster_p->CmdsReceived = 0;
                Blaster_p->CmdUserBufBase_p = Buffer;
#else
                *NumberRead = 0;
#endif

                /* Reset symbol buffer */
                Blaster_p->SymbolBuf_p = Blaster_p->SymbolBufBase_p;
                Blaster_p->SymbolBufCount = 0;
                Blaster_p->SymbolBufFree = Blaster_p->SymbolBufSize;

                /* Assign it */
                Blaster_p->Operation = Op;

               /* Enable receiver */
                BlastRegs_p = &Blaster_p->Regs;
                BLAST_RxSetMaxSymTime(BlastRegs_p,
                                      Blaster_p->MaxSymTime);

                /* Start the timer task */
                stblast_TimerWait(&Blaster_p->Timer,
                                  (void(*)(void *))BLAST_ReadCallback,
                                  Blaster_p,
                                  &Blaster_p->Operation->Timeout,
                                  FALSE
                                 );

                /* Release the semaphore */
                STOS_SemaphoreSignal(Blaster_p->LockSemaphore_p);
            }
            else
            {
                STOS_SemaphoreSignal(Blaster_p->LockSemaphore_p);
                error = ST_ERROR_DEVICE_BUSY;
            }
        }
        else
        {
            /* Trying to do a read on a transmitter... */
            error = STBLAST_ERROR_INVALID_DEVICE_TYPE;
        }
    }
    else
    {
        if (error == ST_NO_ERROR)
        {
            error = ST_ERROR_INVALID_HANDLE;
        }
    }

    *ProtocolSet = Blaster_p->ProtocolSet;

    /* Common exit */
    return error;
}

/*****************************************************************************
STBLAST_Send()
Description:
    Send an RC code using a transmitter device
Parameters:
    Handle              an open handle, as returned by STBLAST_Open
    Buffer              pointer to the buffer to be written
    NumberToWrite       maximum number of symbols to write from buffer
    NumberWritten       pointer to the number of symbols successfully written
    SendTimeout         length of timeout, specified in 1ms intervals. A
                        value of 0 specifies an infinite timeout.
Return Value:
    ST_NO_ERROR                     no error
    ST_ERROR_BAD_PARAMETER          one or more parameters was invalid
    ST_ERROR_DEVICE_BUSY            an operation is already in progress on
                                    the device associated with this handle
    ST_ERROR_INVALID_HANDLE         device handle is invalid
    STBLAST_ERROR_INVALID_DEVICE_TYPE   the Handle parameter does not
                                        correspond to a transmitter device
    STBLAST_ERROR_UNDEFINED_PROTOCOL  A Protocol has not been defined
See Also:
    STBLAST_Symbol_t
    STBLAST_Abort()
    STBLAST_Open()
*****************************************************************************/

ST_ErrorCode_t STBLAST_Send(STBLAST_Handle_t Handle,
                              U32 *Buffer,
                              U32 NumberToSend,
                              U32 *NumberSent,
                              U32 SendTimeout
                             )
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    BLAST_ControlBlock_t *Blaster_p;
    BLAST_Operation_t *Op;
    BLAST_BlastRegs_t *BlastRegs_p;
    U32 SymbolsEncoded;

    Blaster_p = BLAST_GetControlBlockFromHandle(Handle);

    if((Buffer == NULL) ||
        (NumberToSend == 0) ||
        (NumberSent == NULL))
    {
        error = ST_ERROR_BAD_PARAMETER;
    }

    /* Ensure that STBLAST_SetProtocol has been called */
    if((!Blaster_p->ProtocolIsSet) && (error == ST_NO_ERROR))
    {
        error = STBLAST_ERROR_UNDEFINED_PROTOCOL;
    }
     /* Main body */
    if((Blaster_p != NULL) && (error == ST_NO_ERROR))
    {
        if(Blaster_p->InitParams.DeviceType == STBLAST_DEVICE_IR_TRANSMITTER)
        {
            /* Make sure that checks are atomic */
            STOS_SemaphoreWait(Blaster_p->LockSemaphore_p);

#if defined(ST_OSLINUX)
            if(NumberToSend > Blaster_p->CmdBufSize)
            {
                error = ST_ERROR_BAD_PARAMETER;
            }
#endif

            if((Blaster_p->Operation == NULL)&& (error == ST_NO_ERROR))
            {
                /* Take an operation from the "pool" */
                Op = Blaster_p->OperationPool;

                /* Setup the operation */
                Op->Status = (U32)BLAST_OP_PENDING;
                Op->Timeout = SendTimeout * (ST_GetClocksPerSecond()/1000);
                Op->RawMode = FALSE;
#if defined(ST_OSLINUX)
                Op->UserBuf_p = Buffer;
#else
                Op->UserBuf_p = (U32 *)Buffer;
#endif
                Op->UserRemaining = NumberToSend;
                Op->UserBufSize = NumberToSend;
#if defined(ST_OSLINUX)
                Op->UserCount_p = &(Blaster_p->CmdsSent);
#else
                Op->UserCount_p = NumberSent;
#endif
                Op->UserPending = 1; /* Write one symbol */

#if defined(ST_OSLINUX)
                /* Set raw mode flag */
                Blaster_p->LastRawMode = FALSE;
#endif

                /* Reset user count */
#if defined(ST_OSLINUX)
                Blaster_p->SymbolsWritten = 0;
                Blaster_p->CmdsSent = 0;
                Blaster_p->CmdUserBufBase_p = Buffer;
#else
                *NumberSent = 0;
#endif

                /* Reset symbol buffer */
                Blaster_p->SymbolBuf_p = Blaster_p->SymbolBufBase_p;
                Blaster_p->SymbolBufCount = 0;
                Blaster_p->SymbolBufFree = Blaster_p->SymbolBufSize;

                /******************************************************
                 * Encode the first command and then transmit the
                 * first symbol - the rest will be transmitted under
                 * interrupt control.
                 */

                /* Set up subcarrier */
                BlastRegs_p = &Blaster_p->Regs;

                switch (Blaster_p -> Protocol)
                {
                    case STBLAST_PROTOCOL_RC6A:
                    case STBLAST_PROTOCOL_RC6_MODE0:
                    case STBLAST_PROTOCOL_RC5:
                    case STBLAST_PROTOCOL_RCMM:
                        Blaster_p->SubCarrierFreq =
                            BLAST_InitSubCarrier(   BlastRegs_p,
                                                    Blaster_p->InitParams.ClockSpeed,
                                                    36000
                                                );
                        break;
                    case STBLAST_PROTOCOL_RUWIDO:
                       Blaster_p->SubCarrierFreq =
                           BLAST_InitSubCarrier(   BlastRegs_p,
                                                    Blaster_p->InitParams.ClockSpeed,
                                                    38000
                                                );
                       break;

                    case STBLAST_PROTOCOL_RMAP:
                       Blaster_p->SubCarrierFreq =
                           BLAST_InitSubCarrier(   BlastRegs_p,
                                                    Blaster_p->InitParams.ClockSpeed,
                                                    38000
                                                );
                       break;
					
				    case STBLAST_PROTOCOL_RMAP_DOUBLEBIT:
                       Blaster_p->SubCarrierFreq =
                           BLAST_InitSubCarrier(   BlastRegs_p,
                                                    Blaster_p->InitParams.ClockSpeed,
                                                    56000
                                                );
                       break;
                    case STBLAST_PROTOCOL_RSTEP:
                       Blaster_p->SubCarrierFreq =
                           BLAST_InitSubCarrier(   BlastRegs_p,
                                                    Blaster_p->InitParams.ClockSpeed,
                                                    56000
                                                );
                       break;

                    case STBLAST_PROTOCOL_RCRF8:
                       Blaster_p->SubCarrierFreq =
                           BLAST_InitSubCarrier(   BlastRegs_p,
                                                    Blaster_p->InitParams.ClockSpeed,
                                                    38000
                                                );
                       break;

                    case STBLAST_LOW_LATENCY_PROTOCOL:
                        Blaster_p->SubCarrierFreq =
                            BLAST_InitSubCarrier(   BlastRegs_p,
                                                    Blaster_p->InitParams.ClockSpeed,
                                                    38000
                                                );
                        break;
                        
                    case STBLAST_LOW_LATENCY_PRO_PROTOCOL:
                        Blaster_p->SubCarrierFreq =
                            BLAST_InitSubCarrier(   BlastRegs_p,
                                                    Blaster_p->InitParams.ClockSpeed,
                                                    56000
                                                );
                        break;                        
                    case STBLAST_PROTOCOL_USER_DEFINED:
                         Blaster_p->SubCarrierFreq =
                            BLAST_InitSubCarrier(   BlastRegs_p,
                                                    Blaster_p->InitParams.ClockSpeed,
                                                    Blaster_p->ProtocolParams.UserDefined.SubCarrierFreq
                                                 );
                        break;                                                 
					case STBLAST_PROTOCOL_MULTIPLE:
                         error = ST_ERROR_FEATURE_NOT_SUPPORTED;
                        break;
                }

                if(error != ST_NO_ERROR)
                {
                    /* Release the semaphore */
                    STOS_SemaphoreSignal(Blaster_p->LockSemaphore_p);
                    return error;
                }

                /* Commit current duty cycle, if supported */
                if(Blaster_p->Capability.ProgrammableDutyCycle)
                    BLAST_SetDutyCycle(Blaster_p, Blaster_p->DutyCycle);

                /* Set transmit interrupt mode */
                BLAST_TxSetIE(BlastRegs_p, TXIE_TWOEMPTY);

                /* Encode the first element in the user buffer */
                BLAST_DoEncode(Op->UserBuf_p,
                               1,
                               Blaster_p->SymbolBufBase_p,
                               Blaster_p->SymbolBufFree,
                               &SymbolsEncoded,
                               Blaster_p -> Protocol,
                               &Blaster_p->ProtocolParams);

                /* Update symbol buffer */
                Blaster_p->SymbolBuf_p = Blaster_p->SymbolBufBase_p;
                Blaster_p->SymbolBufFree -= SymbolsEncoded;
                Blaster_p->SymbolBufCount += SymbolsEncoded;

                /* If this isn't the last command, add on the
                 * inter-command delay period.
                 */
                if( Op->UserRemaining > 1)
                {
                    U32 SymbolPeriod;

                    /* Add on the protocol delay to the last symbol
                     * that is transmitted as part of the command.
                     */

                    SymbolPeriod = Blaster_p->SymbolBuf_p[
                            Blaster_p->SymbolBufCount - 1
                            ].SymbolPeriod + Blaster_p->Delay;
                    SymbolPeriod = min(65535, SymbolPeriod);

                    Blaster_p->SymbolBuf_p[
                            Blaster_p->SymbolBufCount - 1
                            ].SymbolPeriod = SymbolPeriod;
                }

                /* Write out the first symbol to the transmit FIFO */
                BLAST_TxDisable(BlastRegs_p);
                BLAST_TxSetSymbolTime(BlastRegs_p,
                                      Blaster_p->SymbolBuf_p->SymbolPeriod);
                BLAST_TxSetOnTime(BlastRegs_p,
                                  Blaster_p->SymbolBuf_p->MarkPeriod);

                /* Clear the underrun condition */
                BLAST_ClrUnderrun(BlastRegs_p);
                BLAST_TxEnable(BlastRegs_p);

                /* Set the 'current' operation */
                Blaster_p->Operation = Op;

                /* Enable transmitter */
                BLAST_TxEnableInt(BlastRegs_p);

                /* Start the timer task */
                stblast_TimerWait((Timer_t *)&Blaster_p->Timer,
                                  (void(*)(void *))BLAST_WriteCallback,
                                  Blaster_p,
                                  &Blaster_p->Operation->Timeout,
                                  FALSE
                                 );

                /* Release the semaphore */
                STOS_SemaphoreSignal(Blaster_p->LockSemaphore_p);
            }
            else
            {
                STOS_SemaphoreSignal(Blaster_p->LockSemaphore_p);
                error = ST_ERROR_DEVICE_BUSY;
            }
        }
        else
        {
            /* Trying to transmit on a receiver */
            error = STBLAST_ERROR_INVALID_DEVICE_TYPE;
        }
    }
    else
    {
        if (error == ST_NO_ERROR)
        {
            error = ST_ERROR_INVALID_HANDLE;
        }
    }

    /* Common exit */
    return error;
}

/*****************************************************************************
STBLAST_ReadRaw()
Description:
    Read data from a receiver device
Parameters:
    Handle              an open handle, as returned by STBLAST_Open
    Buffer              pointer to the result buffer
    SymbolsToRead       maximum number of symbols to read into Buffer
    SymbolsRead         pointer to the actual number of Symbols read
    SymbolTimeout       Defines the maximum period in microseconds to
                        allow for a complete symbol to be received once
                        the receiver has detected the leading edge of the
                        symbol.
    ReadTimeout         length of timeout, specified in 1ms intervals. A
                        value of 0 specifies an infinite timeout.
Return Value:
    ST_NO_ERROR                     no error
    ST_ERROR_BAD_PARAMETER          one or more parameters was invalid
    ST_ERROR_DEVICE_BUSY            an operation is already in progress on
                                    the device associated with this handle
    ST_ERROR_INVALID_HANDLE         device handle is invalid
    STBLAST_ERROR_INVALID_DEVICE_TYPE   the Handle parameter does not
                                        correspond to a transmitter device
See Also:
    STBLAST_Symbol_t
    STBLAST_Abort()
    STBLAST_Open()
*****************************************************************************/

ST_ErrorCode_t STBLAST_ReadRaw(STBLAST_Handle_t Handle,
	                           STBLAST_Symbol_t *Buffer,
	                           U32 SymbolsToRead,
	                           U32 *SymbolsRead,
	                           U32 SymbolTimeout,
	                           U32 ReadTimeout
	                          )
	{
    ST_ErrorCode_t error = ST_NO_ERROR;
    BLAST_ControlBlock_t *Blaster_p = NULL;
    BLAST_Operation_t *Op;

    Blaster_p = BLAST_GetControlBlockFromHandle(Handle);

    if((Buffer == NULL) ||
        (SymbolsToRead == 0) ||
        (SymbolsRead == NULL) ||
        (SymbolTimeout == 0))
    {
        error = ST_ERROR_BAD_PARAMETER;
    }

    if((Blaster_p != NULL) && (error == ST_NO_ERROR))
    {
        if ((Blaster_p->InitParams.DeviceType == STBLAST_DEVICE_IR_RECEIVER) ||
            (Blaster_p->InitParams.DeviceType == STBLAST_DEVICE_UHF_RECEIVER))
        {
            BLAST_BlastRegs_t *BlastRegs_p = &Blaster_p->Regs;

            /* Make sure we don't potentially queue two operations */
            STOS_SemaphoreWait(Blaster_p->LockSemaphore_p);

#if defined(ST_OSLINUX)
            if((Blaster_p->Operation == NULL) && (error == ST_NO_ERROR))
#else
            if(Blaster_p->Operation == NULL)
#endif
            {
                /* Take an operation from the "pool" */
                Op = Blaster_p->OperationPool;

                /* Setup the operation */
                Op->Status = (U32)BLAST_OP_PENDING;
                Op->Timeout = ReadTimeout *
                              (ST_GetClocksPerSecond()/1000);
                Op->RawMode = TRUE;
                Op->UserBuf_p = NULL; /* Not used for raw mode */
                Op->UserRemaining = SymbolsToRead;
                Op->UserBufSize = SymbolsToRead;
#if defined(ST_OSLINUX)
                Op->UserCount_p = &(Blaster_p->SymbolsRead);
#else
                Op->UserCount_p = SymbolsRead;
#endif

                /* Reset user count */
#if defined(ST_OSLINUX)
                Blaster_p->SymbolsRead = 0;
#else
                *SymbolsRead = 0;
#endif

#if defined(ST_OSLINUX)
                /* Set raw mode flag */
                Blaster_p->LastRawMode = TRUE;
#endif

                /* Reset symbol buffer */
#if defined(ST_OSLINUX)
                Blaster_p->SymbolUserBufBase_p = Buffer;
                Blaster_p->SymbolBuf_p = Blaster_p->SymbolBufBase_p;
#else
                Blaster_p->SymbolBuf_p = Buffer;
#endif
                Blaster_p->SymbolBufCount = 0;
                Blaster_p->SymbolBufFree = SymbolsToRead;

                /* Assign it */
                Blaster_p->Operation = Op;

                /* Set maximum symbol time */
                BLAST_RxSetMaxSymTime(BlastRegs_p,
                                      SymbolTimeout);

                /* Release lock semaphore */
                STOS_SemaphoreSignal(Blaster_p->LockSemaphore_p);

                /* Start the timer task */
                stblast_TimerWait(&Blaster_p->Timer,
                                  (void(*)(void *))BLAST_ReadCallback,
                                  Blaster_p,
                                  &Blaster_p->Operation->Timeout,
                                  FALSE
                                 );
            }
            else
            {
                /* Already an operation in progress */
                error = ST_ERROR_DEVICE_BUSY;

                /* Release the semaphore */
                STOS_SemaphoreSignal(Blaster_p->LockSemaphore_p);
            }
        }
        else
        {
            error = STBLAST_ERROR_INVALID_DEVICE_TYPE;
        }
    }
    else
    {
        if (error == ST_NO_ERROR)
        {
            error = ST_ERROR_INVALID_HANDLE;
        }
    }

    return error;
}


/*****************************************************************************
STBLAST_SetProtocol()
Description:
    Sets the protocol in use
Parameters:
    Handle          an open handle, as returned by STBLAST_Open
    Protocol    Enum defining the protocol to use.
    ProtocolParams  Union for configuring supported protocols

Return Value:
    ST_NO_ERROR                     no error
    ST_ERROR_BAD_PARAMETER          one or more parameters was invalid
    ST_ERROR_DEVICE_BUSY            an operation is already in progress on
                                    the device associated with this handle
    ST_ERROR_INVALID_HANDLE         device handle is invalid
    STBLAST_ERROR_INVALID_DEVICE_TYPE   the Handle parameter does not
                                        correspond to a transmitter device

See Also:
    STBLAST_Protocol_t
    STBLAST_GetProtocol()
    STBLAST_Send()
*****************************************************************************/

ST_ErrorCode_t STBLAST_SetProtocol(STBLAST_Handle_t Handle,
                                     const STBLAST_Protocol_t Protocol,
                                     STBLAST_ProtocolParams_t *ProtocolParams
                                    )
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    BLAST_ControlBlock_t *Blaster_p;
    U8 Loop;

    /* Check for valid protocol */
    if((Protocol < STBLAST_PROTOCOL_USER_DEFINED) ||
         (Protocol > STBLAST_LOW_LATENCY_PRO_PROTOCOL)
       )
    {
        return ST_ERROR_BAD_PARAMETER;
    }

    if((Protocol != STBLAST_PROTOCOL_RC5) && (ProtocolParams == NULL))
    {
        return ST_ERROR_BAD_PARAMETER;  /* ProtocolParams is not used for RC5 */
    }

    switch (Protocol)
    {
        case STBLAST_PROTOCOL_USER_DEFINED:
            if( /* check Coding */
                 (ProtocolParams->UserDefined.Coding < STBLAST_CODING_PULSE) ||
                 (ProtocolParams->UserDefined.Coding > STBLAST_CODING_SHIFT) ||
                 /* check start-stop symbols */
                 (ProtocolParams->UserDefined.NumberStartSymbols > STBLAST_MAX_START_SYMBOLS) ||
                 (ProtocolParams->UserDefined.NumberStopSymbols > STBLAST_MAX_STOP_SYMBOLS)   ||
                 /* check NumberPayloadBits */
                 (ProtocolParams->UserDefined.NumberPayloadBits > (ProtocolParams->UserDefined.BufferElementsPerPayload*32))
               )
            {
                return ST_ERROR_BAD_PARAMETER;
            }

            if(ProtocolParams->UserDefined.Coding == STBLAST_CODING_SHIFT)
            {
                 /* check Validity of data symbols */
                if(((ProtocolParams->UserDefined.HighDataSymbol.MarkPeriod!=0)&&(ProtocolParams->UserDefined.LowDataSymbol.MarkPeriod !=0)) ||
                      ((ProtocolParams->UserDefined.HighDataSymbol.MarkPeriod==0)&&(ProtocolParams->UserDefined.LowDataSymbol.MarkPeriod ==0))
                    )
                {
                    return ST_ERROR_BAD_PARAMETER;
                }
            }
            break;

         case STBLAST_PROTOCOL_RC6A:

            if(  /* check NumberPayloadBits */
                 (ProtocolParams->RC6A.NumberPayloadBits > (ProtocolParams->RC6A.BufferElementsPerPayload*32))
               )
            {
                return ST_ERROR_BAD_PARAMETER;
            }

            break;

        case STBLAST_PROTOCOL_RC6_MODE0:
            if(  /* check NumberPayloadBits */
                 (ProtocolParams->RC6M0.NumberPayloadBits > (ProtocolParams->RC6M0.BufferElementsPerPayload*32))
               )
            {
                return ST_ERROR_BAD_PARAMETER;
            }

            break;

        case STBLAST_PROTOCOL_RUWIDO:
        case STBLAST_PROTOCOL_RMAP:
        case STBLAST_PROTOCOL_RMAP_DOUBLEBIT:
        case STBLAST_PROTOCOL_RSTEP:
        case STBLAST_PROTOCOL_RC5:
        case STBLAST_PROTOCOL_RCRF8:
        case STBLAST_LOW_LATENCY_PROTOCOL:
        case STBLAST_LOW_LATENCY_PRO_PROTOCOL:
        case STBLAST_PROTOCOL_RCMM:
        case STBLAST_PROTOCOL_MULTIPLE:
        default:
            break;


    }

    /* Find the control block associated with the handle */
    Blaster_p = BLAST_GetControlBlockFromHandle(Handle);

    if(Blaster_p != NULL)
    {
        /* Check operation pointer - make sure it's exclusive */
        STOS_SemaphoreWait(Blaster_p->LockSemaphore_p);

        if(Blaster_p->Operation == NULL)
        {
            /* No operation in progress, start setting the protocol */

            /* Copy it into the control block - OpenParams is maybe not the
             * ideal place, but it already has a protocol structure, so...
             */

            Blaster_p->Protocol = Protocol;

            if(Protocol != STBLAST_PROTOCOL_RC5) /* ProtocolParams is not used for RC5 */
            {
                Blaster_p->ProtocolParams = *ProtocolParams;
            }

            switch(Protocol)
            {
                case STBLAST_PROTOCOL_USER_DEFINED:

                    /* Copy the start/stop sequences */
                    memcpy(Blaster_p->StartSymbols, ProtocolParams->UserDefined.StartSymbols,
                            STBLAST_MAX_START_SYMBOLS * sizeof(STBLAST_Symbol_t));
                    memcpy(Blaster_p->StopSymbols, ProtocolParams->UserDefined.StopSymbols,
                            STBLAST_MAX_STOP_SYMBOLS * sizeof(STBLAST_Symbol_t));

                    /* Determine the maximum symbol time, based on the
                    * current protocol.
                    */
                    if((Blaster_p->InitParams.DeviceType == STBLAST_DEVICE_IR_RECEIVER)||
                        (Blaster_p->InitParams.DeviceType == STBLAST_DEVICE_UHF_RECEIVER))
                    {

                        if(ProtocolParams->UserDefined.Coding == STBLAST_CODING_SHIFT)
                        {
                            Blaster_p->MaxSymTime = 5000;
                        }
                        else
                        {

                            /* When dealing with the maximum symbol time for
                             * a mark/space encoded protocol, we choose the
                             * maximum symbol period of the start, stop,
                             * highdata and lowdata bits.
                             *
                             * We then double this period to allow a good
                             * margin of error.
                            */
                            Blaster_p->MaxSymTime =
                                               max(ProtocolParams->UserDefined.HighDataSymbol.SymbolPeriod,
                                               ProtocolParams->UserDefined.LowDataSymbol.SymbolPeriod);

                            /* If there's a start sequence check the periods */
                            for(Loop = 0;
                                 Loop < ProtocolParams->UserDefined.NumberStartSymbols;
                                Loop++)
                            {
                                Blaster_p->MaxSymTime =
                                       max(Blaster_p->MaxSymTime,
                                       ProtocolParams->UserDefined.StartSymbols[Loop].SymbolPeriod);
                            }

                            /* If there's a stop sequence check the periods */
                            for(Loop = 0;
                                 Loop < ProtocolParams->UserDefined.NumberStopSymbols;
                                 Loop++)
                            {
                                Blaster_p->MaxSymTime =
                                         max(Blaster_p->MaxSymTime,
                                        ProtocolParams->UserDefined.StopSymbols[Loop].SymbolPeriod);
                            }

                            /* take it 1.5 times  the period in case of error */
                            Blaster_p->MaxSymTime = Blaster_p->MaxSymTime + (Blaster_p->MaxSymTime/2);
                       }

                    }
                    Blaster_p ->Delay = MICROSECONDS_TO_SYMBOLS(
                                                ProtocolParams->UserDefined.SubCarrierFreq,
                                                ProtocolParams->UserDefined.Delay);

                    Blaster_p->TimeoutValue = Blaster_p->MaxSymTime ;

                    /* set subcarrier freq */
                    Blaster_p ->SubCarrierFreq = ProtocolParams->UserDefined.SubCarrierFreq;

                    Blaster_p -> DutyCycle = ProtocolParams->UserDefined.DutyCycle;
                    break;


                case STBLAST_PROTOCOL_RC5:

                    /* Determine the maximum symbol time, based on the
                    * current protocol.
                    */
                    if((Blaster_p->InitParams.DeviceType == STBLAST_DEVICE_IR_RECEIVER)||
                        (Blaster_p->InitParams.DeviceType == STBLAST_DEVICE_UHF_RECEIVER))
                    {
                        /* 64 x 27.778us = 1 RC5 coded bit.
                         *
                         * We may receive at most 2 bits in one
                         * symbol period (see encoded/decode routines).
                         *
                         * We double this period to allow a good
                         * margin of error.
                         */
                        Blaster_p->MaxSymTime = (64 * 28 * 2)*2;
                    }

                    Blaster_p ->SubCarrierFreq = 36000;

                    /* There are 64 subcarrier clock periods in 1 bit-period */
                    Blaster_p->Delay = 50 * 64;
                    Blaster_p->TimeoutValue =10000;  /* 10 ms should be enough */
                    Blaster_p -> DutyCycle = 25;
                    break;

                case STBLAST_LOW_LATENCY_PROTOCOL:
                    Blaster_p ->SubCarrierFreq = 38000;
                    Blaster_p->TimeoutValue = 10000;  /* 10 ms should be enough */
                    Blaster_p -> DutyCycle = 33;
                    break;
                    
				case STBLAST_LOW_LATENCY_PRO_PROTOCOL:
				    Blaster_p ->SubCarrierFreq = 56000;
                    Blaster_p->TimeoutValue = 10000;  /* 10 ms should be enough */
                    Blaster_p -> DutyCycle = 33;
                    break;
                    
                case STBLAST_PROTOCOL_RCMM:
                    Blaster_p ->SubCarrierFreq = 36000;
                    Blaster_p->TimeoutValue = 10000;  /* 10 ms should be enough */
                    Blaster_p -> DutyCycle = 33;
                    break;

                case STBLAST_PROTOCOL_RUWIDO:
                       Blaster_p ->SubCarrierFreq = 38000;
                       Blaster_p->TimeoutValue = 10000;  /* 10 ms should be enough */
                       Blaster_p -> DutyCycle = 50;
                       break;
                case STBLAST_PROTOCOL_RMAP:
                       Blaster_p ->SubCarrierFreq = 38000;
                       Blaster_p->TimeoutValue = 10000;  /* 10 ms should be enough */
                       Blaster_p -> DutyCycle = 50;
                       break;

                case STBLAST_PROTOCOL_RMAP_DOUBLEBIT:
                       Blaster_p ->SubCarrierFreq = 56000;
                       Blaster_p->TimeoutValue = 10000;  /* 10 ms should be enough */
                       Blaster_p -> DutyCycle = 33;     
                       break;
                case STBLAST_PROTOCOL_RSTEP:
                       Blaster_p ->SubCarrierFreq = 56000;
                       Blaster_p->TimeoutValue = 10000;  /* 10 ms should be enough */
                       Blaster_p -> DutyCycle = 50;
                       break;

                case STBLAST_PROTOCOL_RCRF8:
                       Blaster_p ->SubCarrierFreq = 38000;
                       Blaster_p->TimeoutValue = 10000;  /* 10 ms should be enough */
                       Blaster_p -> DutyCycle = 50;
                       break;

                case STBLAST_PROTOCOL_RC6A:
                       Blaster_p ->SubCarrierFreq = 36000;

                    /* For Command Mode, each frame is 150ms */
                    /* Header, startbit, mode and trailer require 20 bit periods */
                    /* Remaining number of bits include custom code (8) and NumberPayloadBits*/
                    /* Total is 20 + 8 + NumberPayloadBits */
                    {
                        U32 BitCount = 20 + 8 + ProtocolParams->RC6A.NumberPayloadBits;
                        U32 FreeBitPeriodsInFrame;

                        /* free time in frame */
                        FreeBitPeriodsInFrame = (150000/(U32)888.888 - BitCount);

                        /* There are 32 subcarrier clock periods in each (2t) bit period */
                        Blaster_p->Delay = FreeBitPeriodsInFrame * 32 + 1000; /* add a margin of 1000 */
                    }

                    Blaster_p->TimeoutValue = 10000;  /* 10 ms should be enough */
                    Blaster_p -> DutyCycle = 25;
                    break;

                case STBLAST_PROTOCOL_RC6_MODE0:
                     Blaster_p ->SubCarrierFreq = 36000;

                    /* For Command Mode, each frame is 150ms */
                    /* Header, startbit, mode and trailer require 20 bit periods */
                    /* Remaining number of bits include custom code (8) and NumberPayloadBits*/
                    /* Total is 20 + 8 + NumberPayloadBits */
                    {
                        U32 BitCount = 20 + 8 + ProtocolParams->RC6M0.NumberPayloadBits;
                        U32 FreeBitPeriodsInFrame;

                        /* free time in frame */
                        FreeBitPeriodsInFrame = (150000/(U32)888.888 - BitCount);

                        /* There are 32 subcarrier clock periods in each (2t) bit period */
                        Blaster_p->Delay = FreeBitPeriodsInFrame * 32 + 1000; /* add a margin of 1000 */
                    }

                    Blaster_p->TimeoutValue = 10000;  /* 10 ms should be enough */
                    Blaster_p -> DutyCycle = 25;
                    break;


                case STBLAST_PROTOCOL_MULTIPLE:
                    Blaster_p->TimeoutValue =10000;  /* 10 ms should be enough */
                    Blaster_p ->DutyCycle = 25;
                    break;

                default:
                    break;
            }
        }
        else
        {
            /* Don't change protocol while receiving/transmitting */
            error = ST_ERROR_DEVICE_BUSY;
        }

        /* Finally, we can now release the semaphore (all values
         * have been updated)
         */
        STOS_SemaphoreSignal(Blaster_p->LockSemaphore_p);
    }
    else
    {
        error = ST_ERROR_INVALID_HANDLE;
    }

    if(error == ST_NO_ERROR)
    {
        Blaster_p->ProtocolIsSet = TRUE;
    }

    /* Common exit point */
    return error;
}


/*****************************************************************************
STBLAST_SetSubCarrierFreq()
Description:
    Sets the protocol in use
Parameters:
    Handle          an open handle, as returned by STBLAST_Open

Return Value:

*****************************************************************************/

ST_ErrorCode_t STBLAST_SetSubCarrierFreq(STBLAST_Handle_t Handle, U32 SubCarrierFreq )
{
    BLAST_ControlBlock_t *Blaster_p;

    /* Obtain the control block associated with handle */
    Blaster_p = BLAST_GetControlBlockFromHandle(Handle);

    /* Only continue if we have a control block for this device */
    if (Blaster_p != NULL)
    {
        Blaster_p ->SubCarrierFreq = SubCarrierFreq;
        return (ST_NO_ERROR);

    }
    else
    {
        /* The control block was not retrieved from the queue, therefore the
         * device name must be invalid.
         */
        return(ST_ERROR_UNKNOWN_DEVICE);

    }


}

/*****************************************************************************
STBLAST_Term()
Description:
    Terminate the named IR Blaster device driver
Parameters:
    DeviceName      name of the IR Blaster device to be terminated
    TermParams      the termination parameters
Return Value:
    ST_NO_ERROR                     no error
    ST_ERROR_BAD_PARAMETER          one or more parameters was invalid
    ST_ERROR_INTERRUPT_UNINSTALL    unable to uninstall ISR
    ST_ERROR_OPEN_HANDLE            could not terminate driver; not all
                                    handles have been closed
    ST_ERROR_UNKNOWN_DEVICE         invalid device name
    STBLAST_ERROR_EVENT_UNREGISTER  error occurred while unregistering
                                    events
See Also:
    STBLAST_TermParams_t
    STBLAST_Init()
*****************************************************************************/
ST_ErrorCode_t STBLAST_Term(const ST_DeviceName_t DeviceName,
                            const STBLAST_TermParams_t *TermParams
                           )
{
    BLAST_BlastRegs_t *BlastRegs_p;
    ST_ErrorCode_t error = ST_NO_ERROR;
    BLAST_ControlBlock_t *Blaster_p;
    partition_t *DriverPartition;
#ifndef ST_OSLINUX
#ifndef ST_OS21
    interrupt_status_t IntStatus;
#endif
#endif

#ifdef ST_OS21
    if(DeviceName == NULL)
    {
        return ST_ERROR_UNKNOWN_DEVICE;
    }
#endif

    /* InitSemaphore can be null legitimately - call init, term, term.
     * On the second term, the semaphore is no longer valid.
     */
    if(InitSemaphore_p != NULL)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }

    /* Obtain the control block associated with the device name */
    Blaster_p = BLAST_GetControlBlockFromName(DeviceName);

    /* Only continue if we have a control block for this device */
    if(Blaster_p != NULL)
    {
        Blaster_p->ReceiveTaskTerm = TRUE;

        /* This device may be terminated if either:
         * a) the caller wishes to force termination
         * b) the device is closed
         */
        if(Blaster_p->Handle == 0 || TermParams->ForceTerminate)
        {
#if defined(ST_OSLINUX)
	    unsigned int flags;
#endif
            /* Abort any outstanding operations on this device */
            if(Blaster_p->Handle != 0)
            {
                STBLAST_Close(Blaster_p->Handle);
            }

            /* Unregister events */
            error = BLAST_EvtUnregister(Blaster_p);
            if(error != ST_NO_ERROR)
            {
                /* Note the error, but continue terminating */
                error = STBLAST_ERROR_EVENT_UNREGISTER;
            }

            /* Disable sending etc. on the blaster */
            BlastRegs_p = &Blaster_p->Regs;

            /* Close any open PIO ports */
          	if((Blaster_p->InitParams.DeviceType  == STBLAST_DEVICE_IR_RECEIVER) ||
               (Blaster_p->InitParams.DeviceType  == STBLAST_DEVICE_UHF_RECEIVER))              
            {
                BLAST_RxDisable(BlastRegs_p);
                STPIO_Close(Blaster_p->RxPIOHandle);
            }
            else
            {
                BLAST_TxDisable(BlastRegs_p);

                STPIO_Close(Blaster_p->TxPIOHandlePPM);
#if !defined(STBLAST_NOT_USE_IRB_JACK)                
                STPIO_Close(Blaster_p->TxPIOHandleJack);
#endif                
            }

#if 0 /* Disabled because BLAST_EvtUnregister() above already close - David Liu */
            STEVT_Close(Blaster_p->EvtHandle);
#endif

#if defined(ST_OSLINUX)
#ifndef BLAST_PREEMPT_KERNEL  /* not applicable to preempt kernel */
            spin_lock_irqsave (&current->sighand->siglock, flags);
#else
            local_irq_save(flags);
#endif /* BLAST_PREEMPT_KERNEL */
#else
            STOS_InterruptLock();       /* Operation must be atomic */
#endif
           
            BLASTInstanceCount--;

            /* If there's an instance, the semaphore must be valid */
            STOS_SemaphoreSignal(InitSemaphore_p);

            if(BLASTInstanceCount == 0)
            {
#if defined(ST_OSLINUX)
                /* Now Unmap memory under Linux */
                if (check_mem_region((unsigned long)Blaster_p->MappingBaseAddress,
                                     Blaster_p->MappingWidth) < 0)
                {
                    release_mem_region((unsigned long)Blaster_p->MappingBaseAddress,
                                       Blaster_p->MappingWidth);
                }
                iounmap(Blaster_p->MappingBaseAddress);
#endif
                /* Now uninstall the interrupt handler */
#if defined(ST_OSLINUX)
	            free_irq(Blaster_p->InitParams.InterruptNumber, (void *)&BLASTQueueHead_p);
#else
                if(interrupt_uninstall(Blaster_p->InitParams.InterruptNumber,
                                        Blaster_p->InitParams.InterruptLevel) != 0)
                {
                    /* Unable to remove the handler */
                    error = ST_ERROR_INTERRUPT_UNINSTALL;
                }
#endif
            }
#if defined(ST_OSLINUX)
            /* disable_irq(Blaster_p->InitParams.InterruptNumber); */
#else
#ifndef ST_OS21
            /* Obtain number of active handlers still installed */
            if(interrupt_status(Blaster_p->InitParams.InterruptLevel,
                                 &IntStatus) == 0)
            {
                /* Disable interrupt level if no handler are installed */
                if(IntStatus.interrupt_numbers == 0)
                {
                    /* Disable interrupts at this level */
#ifdef STAPI_INTERRUPT_BY_NUMBER
                    if (interrupt_disable_number(Blaster_p->InitParams.InterruptNumber)!= 0)

#else
                    if (interrupt_disable(Blaster_p->InitParams.InterruptLevel) != 0)
#endif
                    {
                        error = ST_ERROR_INTERRUPT_UNINSTALL;
                    }
                }
            }
            else
            {
                /* Unable to obtain interrupt level status */
                error = ST_ERROR_INTERRUPT_UNINSTALL;
            }



#else
            if (interrupt_disable_number(Blaster_p->InitParams.InterruptNumber) != 0)
            {
                /* Unable to disable interrupts */
                error = ST_ERROR_INTERRUPT_UNINSTALL;
            }

#endif
#endif /* !ST_OSLINUX */

#if defined(ST_OSLINUX)
#ifndef BLAST_PREEMPT_KERNEL
            spin_unlock_irqrestore (&current->sighand->siglock, flags);
#else
            local_irq_restore(flags);
#endif /* BLAST_PREEMPT_KERNEL */
#else
            STOS_InterruptUnlock();     /* Re-enable interrupts */
#endif

            /* Store driver partition pointer, before deallocating
             * memory.
             */
            DriverPartition =
                (partition_t *)Blaster_p->InitParams.DriverPartition;

            /* Free symbol buffer */
            STOS_MemoryDeallocate(DriverPartition,
                              Blaster_p->SymbolBufBase_p);

#if defined(ST_OSLINUX)
            STOS_MemoryDeallocate(DriverPartition,
                              Blaster_p->CmdBufBase_p);
#endif

            /* delete lock semaphore */
            STOS_SemaphoreDelete(NULL,Blaster_p->LockSemaphore_p);
            
        	/* Delete allocated timers */
            stblast_TimerDelete(DeviceName,&Blaster_p->Timer);
            
            /* Remove device from queue */
            BLAST_QueueRemove(Blaster_p);
            
           if((Blaster_p->InitParams.DeviceType == STBLAST_DEVICE_IR_RECEIVER) ||
              (Blaster_p->InitParams.DeviceType == STBLAST_DEVICE_UHF_RECEIVER))
           {
                /* signal before deleting*/
                /* Instead of semaphore signal use the send a message  */
                
                BlastInterruptData_t *TempMessage;
                TempMessage = STOS_MessageQueueClaim(Blaster_p->InterruptQueue_p);
                STOS_MessageQueueSend(Blaster_p->InterruptQueue_p, TempMessage);

               if ( STOS_TaskWait(&Blaster_p->ReceiveTask,TIMEOUT_INFINITY) == 0 )
               {
                    STOS_TaskDelete(Blaster_p->ReceiveTask,
				                   (partition_t *)Blaster_p->InitParams.DriverPartition,
				                    Blaster_p->Task_p,
				                   (partition_t *)Blaster_p->InitParams.DriverPartition);
               }
                /* delete lock semaphore*/
               STOS_SemaphoreDelete(NULL,Blaster_p->ReceiveSem_p);
           }
         
            /* Free up the control structure for the BLAST */
             STOS_MemoryDeallocate(DriverPartition,
                              Blaster_p);
        }
        else
        {
            /* There is still an outstanding open on this device, but the
             * caller does not wish to force termination.
             */
            error = ST_ERROR_OPEN_HANDLE;
            STOS_SemaphoreSignal(InitSemaphore_p);
        }
    }
    else
    {
        /* The control block was not retrieved from the queue, therefore the
         * device name must be invalid.
         */
        error = ST_ERROR_UNKNOWN_DEVICE;

        if(InitSemaphore_p != NULL)
        {
            STOS_SemaphoreSignal(InitSemaphore_p);
        }
    }

    /* Common exit point */
    return error;
} /* STBLAST_Term */


/*****************************************************************************
STBLAST_WriteRaw()
Description:
    Write symbols to a transmitter device
Parameters:
    Handle              an open handle, as returned by STBLAST_Open
    SubCarrierFreq      subcarrier frequency in Hz
    DutyCycle           Duty Cycle , If Programable Duty cycle
    Buffer              pointer to the symbol buffer to be written
    SymbolsToWrite      maximum number of symbols to write from buffer
    SymbolsWritten      pointer to the number of symbols successfully written
    WriteTimeout        length of timeout, specified in 1ms intervals. A
                        value of 0 specifies an infinite timeout.
Return Value:
    ST_NO_ERROR                     no error
    ST_ERROR_BAD_PARAMETER          one or more parameters was invalid
    ST_ERROR_DEVICE_BUSY            an operation is already in progress on
                                    the device associated with this handle
    ST_ERROR_INVALID_HANDLE         device handle is invalid
    STBLAST_ERROR_INVALID_DEVICE_TYPE   the Handle parameter does not
                                        correspond to a transmitter device
See Also:
    STBLAST_Symbol_t
    STBLAST_Abort()
    STBLAST_Open()
*****************************************************************************/

ST_ErrorCode_t STBLAST_WriteRaw(STBLAST_Handle_t Handle,
                             U32 SubCarrierFreq,
                             U8  DutyCycle,
                             const STBLAST_Symbol_t *Buffer,
                             U32 SymbolsToWrite,
                             U32 *SymbolsWritten,
                             U32 WriteTimeout
                            )
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    BLAST_ControlBlock_t *Blaster_p;
    BLAST_Operation_t *Op;

    Blaster_p = BLAST_GetControlBlockFromHandle(Handle);

    if((SubCarrierFreq == 0) ||
        (Buffer == NULL) ||
        (SymbolsToWrite == 0) ||
        (SymbolsWritten == NULL))
    {
        error = ST_ERROR_BAD_PARAMETER;
    }

    /* Some basic parameter/environment checking */
    if((Blaster_p != NULL) && (error == ST_NO_ERROR))
    {
        /* Check device type */
        if(Blaster_p->InitParams.DeviceType == STBLAST_DEVICE_IR_TRANSMITTER)
        {
            BLAST_BlastRegs_t *BlastRegs_p = &Blaster_p->Regs;

            STOS_SemaphoreWait(Blaster_p->LockSemaphore_p);

            if(Blaster_p->Operation == NULL)
            {
                /* Take an operation from the "pool" */
                Op = Blaster_p->OperationPool;

                /* Setup the operation */
                Op->Status = (U32)BLAST_OP_PENDING;
                Op->Timeout = WriteTimeout * (ST_GetClocksPerSecond()/1000);
                Op->RawMode = TRUE;
                Op->UserBuf_p = NULL; /* Not used in raw mode */
                Op->UserRemaining = SymbolsToWrite;
                Op->UserBufSize = SymbolsToWrite;
#if defined(ST_OSLINUX)
                Op->UserCount_p = &(Blaster_p->SymbolsWritten);
#else
                Op->UserCount_p = SymbolsWritten;
#endif
                Op->UserPending = 1; /* Write one symbol */

                /* Reset user count */
#if defined(ST_OSLINUX)
                Blaster_p->SymbolsWritten = 0;
#else
                *SymbolsWritten = 0;
#endif

#if defined(ST_OSLINUX)
                /* Set raw mode flag */
                Blaster_p->LastRawMode = TRUE;
#endif
                /* Reset symbol buffer */
                Blaster_p->SymbolBuf_p = (STBLAST_Symbol_t *)STOS_MemoryAllocate(
                        			      Blaster_p->InitParams.DriverPartition,SymbolsToWrite);       
                memcpy( Blaster_p->SymbolBuf_p,Buffer,SymbolsToWrite);
                
                Blaster_p->SymbolBufCount = SymbolsToWrite;
                Blaster_p->SymbolBufFree = 0; /* Not used */

                /* Set up subcarrier and store away (since this is
                 * a raw mode access)
                 */
                Blaster_p->SubCarrierFreq =
                    BLAST_InitSubCarrier(BlastRegs_p,
                                         Blaster_p->InitParams.ClockSpeed,
                                         SubCarrierFreq
                                        );

                /* Commit current duty cycle, if supported */
                if(Blaster_p->Capability.ProgrammableDutyCycle)
                {
                    BLAST_SetDutyCycle(Blaster_p, DutyCycle);
                }

                /******************************************************
                 * Transmit the first symbol - the rest will be
                 * transmitted under interrupt control.
                 */

                /* Disable transmitter */
                BLAST_TxDisable(BlastRegs_p);

                /* Set transmit interrupt mode */
                BLAST_TxSetIE(BlastRegs_p, TXIE_TWOEMPTY);
#if defined(ST_OSLINUX)
#if defined(BLAST_API_DEBUG)
                printk("stblast.c:Blaster_p->SymbolBuf_p->MarkPeriod=%d, Blaster_p->SymbolBuf_p->SymbolPeriod=%d",
                       Blaster_p->SymbolBuf_p->MarkPeriod, Blaster_p->SymbolBuf_p->SymbolPeriod);
#endif
#endif
                /* Write out the symbol */
                BLAST_TxSetSymbolTime(
                    BlastRegs_p,
                    MICROSECONDS_TO_SYMBOLS(
                    SubCarrierFreq,
                    Blaster_p->SymbolBuf_p->SymbolPeriod)
                );

                BLAST_TxSetOnTime(
                    BlastRegs_p,
                    MICROSECONDS_TO_SYMBOLS(
                    SubCarrierFreq,
                    Blaster_p->SymbolBuf_p->MarkPeriod)
                );

                /* Clear underrun condition */
                BLAST_ClrUnderrun(BlastRegs_p);
                BLAST_TxEnable(BlastRegs_p);

                /* Set the 'current' operation */
                Blaster_p->Operation = Op;

                /* Enable interrupts */
                BLAST_TxEnableInt(BlastRegs_p);

                /* Start timer */
                stblast_TimerWait((Timer_t *)&Blaster_p->Timer,
                                  (void(*)(void *))BLAST_WriteCallback,
                                  Blaster_p,
                                  &Blaster_p->Operation->Timeout,
                                  FALSE
                                 );

                /* Release the semaphore */
                STOS_SemaphoreSignal(Blaster_p->LockSemaphore_p);
            }
            else
            {
                /* Already an operation in progress */
                error = ST_ERROR_DEVICE_BUSY;

                /* Release the semaphore */
                STOS_SemaphoreSignal(Blaster_p->LockSemaphore_p);
            }
        }
        else
        {
            /* Can't write using a receiver */
            error = STBLAST_ERROR_INVALID_DEVICE_TYPE;
        }
    }
    else
    {
        if(error == ST_NO_ERROR)
            error = ST_ERROR_INVALID_HANDLE;
    }

    return error;
}

/* Added STBLAST API on STLinux */
#if defined(ST_OSLINUX)
/*****************************************************************************
STBLAST_GetSymbolRead()
Description:
    Get nymbers of symbols STBLAST has read already
Parameters:
    Handle              an open handle, as returned by STBLAST_Open
    SymbolsRead         the numbers of symbols has read

Return Value:
    ST_NO_ERROR                     no error
    ST_ERROR_BAD_PARAMETER          one or more parameters was invalid
    ST_ERROR_DEVICE_BUSY            an operation is already in progress on
                                    the device associated with this handle
    ST_ERROR_INVALID_HANDLE         device handle is invalid
    STBLAST_ERROR_INVALID_DEVICE_TYPE   the Handle parameter does not
                                        correspond to a receiver device
See Also:
*****************************************************************************/
ST_ErrorCode_t STBLAST_GetSymbolRead(STBLAST_Handle_t Handle, U32 *SymbolsRead)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    BLAST_ControlBlock_t *Blaster_p;

    Blaster_p = BLAST_GetControlBlockFromHandle(Handle);

    if (Blaster_p != NULL)
    {
        if(((Blaster_p->InitParams.DeviceType == STBLAST_DEVICE_IR_RECEIVER) 
            || (Blaster_p->InitParams.DeviceType == STBLAST_DEVICE_UHF_RECEIVER) )
            && (Blaster_p->LastRawMode == TRUE))
        {
            STOS_SemaphoreWait(Blaster_p->LockSemaphore_p);

            if (Blaster_p->Operation == NULL)
            {
                *SymbolsRead = Blaster_p->SymbolsRead;
            }
            else
            {
                error = ST_ERROR_DEVICE_BUSY;
            }

            STOS_SemaphoreSignal(Blaster_p->LockSemaphore_p);
        }
        else
        {
            error = STBLAST_ERROR_INVALID_DEVICE_TYPE;
        }
    }
    else
    {
        error = ST_ERROR_INVALID_HANDLE;
    }

    return error;
}

/*****************************************************************************
STBLAST_CopySymbolstoBuffer
Description:
    Copy symbols from kernel buffer to user buffer
Parameters:
    Handle              an open handle, as returned by STBLAST_Open
    BUffer              the pointer of buffer in user space
    NumSymboltoCopy     Number of symbols to be copied
    NumSymbolCopied     Number of symbols copied

Return Value:
    ST_NO_ERROR                     no error
    ST_ERROR_DEVICE_BUSY            an operation is already in progress on
                                    the device associated with this handle
    ST_ERROR_INVALID_HANDLE         device handle is invalid
    STBLAST_ERROR_INVALID_DEVICE_TYPE   the Handle parameter does not
                                        correspond to a receiver device
See Also:
*****************************************************************************/
ST_ErrorCode_t STBLAST_CopySymbolstoBuffer(STBLAST_Handle_t Handle, STBLAST_Symbol_t *Buffer,
                                        U32 NumSymboltoCopy, U32 *NumSymbolCopied)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    BLAST_ControlBlock_t *Blaster_p;

    Blaster_p = BLAST_GetControlBlockFromHandle(Handle);

    if (Blaster_p != NULL)
    {
        if(((Blaster_p->InitParams.DeviceType == STBLAST_DEVICE_IR_RECEIVER) 
            || (Blaster_p->InitParams.DeviceType == STBLAST_DEVICE_UHF_RECEIVER) )
            && (Blaster_p->LastRawMode == TRUE))
        {
            STOS_SemaphoreWait(Blaster_p->LockSemaphore_p);

		   if (Blaster_p->Operation == NULL)
           {
                memcpy(Buffer,Blaster_p->SymbolBufBase_p,
                       Blaster_p->SymbolsRead * sizeof(STBLAST_Symbol_t));

                *NumSymbolCopied = Blaster_p->SymbolsRead;
           }
           else
           {
                error = ST_ERROR_DEVICE_BUSY;
           }
           
            STOS_SemaphoreSignal(Blaster_p->LockSemaphore_p);
        }
        else
        {
            error = STBLAST_ERROR_INVALID_DEVICE_TYPE;
        }
    }
    else
    {
        error = ST_ERROR_INVALID_HANDLE;
    }

    return error;
}

/*****************************************************************************
STBLAST_GetCmdReceived
Description:
    Get commands received by STBLAST
Parameters:
    Handle              an open handle, as returned by STBLAST_Open
    NumCmdReceived      the number of command has been received

Return Value:
    ST_NO_ERROR                     no error
    ST_ERROR_DEVICE_BUSY            an operation is already in progress on
                                    the device associated with this handle
    ST_ERROR_INVALID_HANDLE         device handle is invalid
    STBLAST_ERROR_INVALID_DEVICE_TYPE   the Handle parameter does not
                                        correspond to a receiver device
See Also:
*****************************************************************************/
ST_ErrorCode_t STBLAST_GetCmdReceived(STBLAST_Handle_t Handle, U32 *NumCmdReceived)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    BLAST_ControlBlock_t *Blaster_p;

    Blaster_p = BLAST_GetControlBlockFromHandle(Handle);

    if (Blaster_p != NULL)
    {
        if(((Blaster_p->InitParams.DeviceType == STBLAST_DEVICE_IR_RECEIVER) 
            || (Blaster_p->InitParams.DeviceType == STBLAST_DEVICE_UHF_RECEIVER) )
            && (Blaster_p->LastRawMode == FALSE))
        {

            STOS_SemaphoreWait(Blaster_p->LockSemaphore_p);

            if (Blaster_p->Operation == NULL)
            {
                *NumCmdReceived = Blaster_p->CmdsReceived;
            }
            else
            {
                error = ST_ERROR_DEVICE_BUSY;
            }

            STOS_SemaphoreSignal(Blaster_p->LockSemaphore_p);
        }
        else
        {
            error = STBLAST_ERROR_INVALID_DEVICE_TYPE;
        }
    }
    else
    {
        error = ST_ERROR_INVALID_HANDLE;
    }

    return error;
}

/*****************************************************************************
STBLAST_CopyCmdstoBuffer
Description:
    Copy commands from kernel buffer to user buffer
Parameters:
    Handle              an open handle, as returned by STBLAST_Open
    BUffer              the pointer of buffer in user space
    NumCmdtoCopy        Number of commands to be copied
    NumCmdCopied        Number of commands copied

Return Value:
    ST_NO_ERROR                     no error
    ST_ERROR_DEVICE_BUSY            an operation is already in progress on
                                    the device associated with this handle
    ST_ERROR_INVALID_HANDLE         device handle is invalid
    STBLAST_ERROR_INVALID_DEVICE_TYPE   the Handle parameter does not
                                        correspond to a receiver device
See Also:
*****************************************************************************/
ST_ErrorCode_t STBLAST_CopyCmdstoBuffer(STBLAST_Handle_t Handle, U32 *Cmd, 
										U32 NumCmdtoCopy,U32 *NumCmdCopied)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    BLAST_ControlBlock_t *Blaster_p;

    Blaster_p = BLAST_GetControlBlockFromHandle(Handle);

    if (Blaster_p != NULL)
    {
  		if(((Blaster_p->DeviceType == STBLAST_DEVICE_IR_RECEIVER) 
            || (Blaster_p->DeviceType == STBLAST_DEVICE_UHF_RECEIVER))
            && (Blaster_p->LastRawMode == FALSE))
        {
            STOS_SemaphoreWait(Blaster_p->LockSemaphore_p);
       
            if (Blaster_p->Operation == NULL)
            {
			 	memcpy(Cmd, Blaster_p->CmdBufBase_p,Blaster_p->CmdsReceived * sizeof(U32));
			 	*NumCmdCopied = Blaster_p->CmdsReceived;
 			}
 			else
            {
                error = ST_ERROR_DEVICE_BUSY;
            }
            
            STOS_SemaphoreSignal(Blaster_p->LockSemaphore_p);
        }
        else
        {
            error = STBLAST_ERROR_INVALID_DEVICE_TYPE;
        }
    }
    else
    {
        error = ST_ERROR_INVALID_HANDLE;
    }

    return error;
}

/*****************************************************************************
STBLAST_GetSymbolWritten()
Description:
    Get number of symbols STBLAST has written
Parameters:
    Handle                 an open handle, as returned by STBLAST_Open
    SymbolsWritten         the numbers of symbols has sent

Return Value:
    ST_NO_ERROR                     no error
    ST_ERROR_BAD_PARAMETER          one or more parameters was invalid
    ST_ERROR_DEVICE_BUSY            an operation is already in progress on
                                    the device associated with this handle
    ST_ERROR_INVALID_HANDLE         device handle is invalid
    STBLAST_ERROR_INVALID_DEVICE_TYPE   the Handle parameter does not
                                        correspond to a receiver device
See Also:
*****************************************************************************/
ST_ErrorCode_t STBLAST_GetSymbolWritten(STBLAST_Handle_t Handle, U32 *SymbolsWritten)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    BLAST_ControlBlock_t *Blaster_p;

    Blaster_p = BLAST_GetControlBlockFromHandle(Handle);

    if (Blaster_p != NULL)
    {
        if ((Blaster_p->InitParams.DeviceType == STBLAST_DEVICE_IR_TRANSMITTER)
            && (Blaster_p->LastRawMode == TRUE))
        {
            STOS_SemaphoreWait(Blaster_p->LockSemaphore_p);

            if (Blaster_p->Operation == NULL)
            {
                *SymbolsWritten = Blaster_p->SymbolsWritten;
            }
            else
            {
                error = ST_ERROR_DEVICE_BUSY;
            }

            STOS_SemaphoreSignal(Blaster_p->LockSemaphore_p);
        }
        else
        {
            error = STBLAST_ERROR_INVALID_DEVICE_TYPE;
        }
    }
    else
    {
        error = ST_ERROR_INVALID_HANDLE;
    }

    return error;
}

/*****************************************************************************
STBLAST_GetCmdSent
Description:
    Get commands Sent by STBLAST
Parameters:
    Handle              an open handle, as returned by STBLAST_Open
    NumCmdSent          the number of command has been sent

Return Value:
    ST_NO_ERROR                     no error
    ST_ERROR_DEVICE_BUSY            an operation is already in progress on
                                    the device associated with this handle
    ST_ERROR_INVALID_HANDLE         device handle is invalid
    STBLAST_ERROR_INVALID_DEVICE_TYPE   the Handle parameter does not
                                        correspond to a receiver device
See Also:
*****************************************************************************/
ST_ErrorCode_t STBLAST_GetCmdSent(STBLAST_Handle_t Handle, U32 *NumCmdSent)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    BLAST_ControlBlock_t *Blaster_p;

    Blaster_p = BLAST_GetControlBlockFromHandle(Handle);

    if (Blaster_p != NULL)
    {
        if ((Blaster_p->InitParams.DeviceType == STBLAST_DEVICE_IR_TRANSMITTER)
            && (Blaster_p->LastRawMode == FALSE))
        {

            STOS_SemaphoreWait(Blaster_p->LockSemaphore_p);

            if (Blaster_p->Operation == NULL)
            {
                *NumCmdSent = Blaster_p->CmdsSent;
            }
            else
            {
                error = ST_ERROR_DEVICE_BUSY;
            }

            STOS_SemaphoreSignal(Blaster_p->LockSemaphore_p);
        }
        else
        {
            error = STBLAST_ERROR_INVALID_DEVICE_TYPE;
        }
    }
    else
    {
        error = ST_ERROR_INVALID_HANDLE;
    }

    return error;
}


#endif /* ST_OSLINUX */

/*****************************************************************************
 * Worker routines (no API-level functions)                                  *
 *****************************************************************************/

/*****************************************************************************
BLAST_EvtRegister()
Description:
    Open an event handler, and register all relevant events with it.
    Must be called from only one thread at a time! Assumes Blaster_p is valid.

Parameters:
    EVTDeviceName       the name of the handler to use
    Blaster_p           pointer to a blaster control block
Return Value:
    ST_NO_ERROR         no problems
    STBLAST_ERROR_EVENT_REGISTER   problem registering events
See Also:
    STBLAST_Init()
*****************************************************************************/

static ST_ErrorCode_t BLAST_EvtRegister(const ST_DeviceName_t EVTDeviceName,
                                        BLAST_ControlBlock_t *Blaster_p)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    STEVT_OpenParams_t OpenParams;
    BOOL EvtOpen = FALSE;

   /* Open the event handler */
    /* is it necessary to call STEVT API again?? D.Liu */
    error = STEVT_Open(EVTDeviceName, &OpenParams, &Blaster_p->EvtHandle);

    if(error == ST_NO_ERROR)
    {
        if((Blaster_p->DeviceType == STBLAST_DEVICE_IR_RECEIVER) ||
            (Blaster_p->DeviceType == STBLAST_DEVICE_UHF_RECEIVER) )
        {

            error = STEVT_RegisterDeviceEvent(Blaster_p->EvtHandle,
                                    Blaster_p->DeviceName,
                                    STBLAST_READ_DONE_EVT,
                                    &Blaster_p->Events[
                                        EVENT_TO_INDEX(STBLAST_READ_DONE_EVT)]);
        }
        else
        {
            error = STEVT_RegisterDeviceEvent(Blaster_p->EvtHandle,
                                    Blaster_p->DeviceName,
                                    STBLAST_WRITE_DONE_EVT,
                                    &Blaster_p->Events[
                                        EVENT_TO_INDEX(STBLAST_WRITE_DONE_EVT)]);
        }
    }

    if(error != ST_NO_ERROR)
    {
        /* Unregister events */
        STEVT_UnregisterDeviceEvent(Blaster_p->Handle,
                                    Blaster_p->DeviceName,
                                    STBLAST_READ_DONE_EVT);
        STEVT_UnregisterDeviceEvent(Blaster_p->Handle,
                                    Blaster_p->DeviceName,
                                    STBLAST_WRITE_DONE_EVT);

        /* Close the event handler */
        if(EvtOpen)
        {
            STEVT_Close(Blaster_p->EvtHandle);
        }

        /* Error while opening handler */
        error = STBLAST_ERROR_EVENT_REGISTER;
    }

    return error;
}
/*****************************************************************************
BLAST_EvtUnregister()
Description:
    Unregisters (and closes) the event handler associated with the blaster_p
    block. Must only be called from one thread at a time! Assumes Blaster_p
    is valid.

Parameters:
    Blaster_p           pointer to a blaster control block
Return Value:
    ST_NO_ERROR         no problems
    STBLAST_ERROR_EVT   some error occurred while dealing with EVT (open,
                            register etc.)

See Also:
    STBLAST_Close()
*****************************************************************************/
static ST_ErrorCode_t BLAST_EvtUnregister(BLAST_ControlBlock_t *Blaster_p)
{
    ST_ErrorCode_t error = ST_NO_ERROR, stored = ST_NO_ERROR;

    if((Blaster_p->DeviceType == STBLAST_DEVICE_IR_RECEIVER)||
        (Blaster_p->DeviceType == STBLAST_DEVICE_UHF_RECEIVER))
    {
        error = STEVT_UnregisterDeviceEvent(Blaster_p->EvtHandle,
                                     Blaster_p->DeviceName,
                                     STBLAST_READ_DONE_EVT);
    }
    else
    {
        error = STEVT_UnregisterDeviceEvent(Blaster_p->EvtHandle,
                                     Blaster_p->DeviceName,
                                     STBLAST_WRITE_DONE_EVT);
    }

    if( error != ST_NO_ERROR )
    {
        stored = STBLAST_ERROR_EVENT_UNREGISTER;
    }

    error = STEVT_Close(Blaster_p->EvtHandle);

    if ( error != ST_NO_ERROR )
    {
        stored = STBLAST_ERROR_EVENT_UNREGISTER;
    }
    /* Common exit point */
    return stored;
}

/*****************************************************************************
BLAST_WriteCallback()
Description:

Parameters:
    Blaster_p           pointer to blaster control block
Return Value:
    nona
See Also:
    STBLAST_Write()
*****************************************************************************/
static void BLAST_WriteCallback(BLAST_ControlBlock_t *Blaster_p)
{
    /* Assume that Blaster_p != NULL */
    BLAST_Operation_t *Op_p = Blaster_p->Operation;
    STBLAST_EvtWriteDoneParams_t DoneParams;

    /* Remove ourselves from servicing */
#if defined(ST_OSLINUX)
    unsigned int flags;
    spin_lock_irqsave (&current->sighand->siglock, flags);
#else
    STOS_InterruptLock();
#endif

    Blaster_p->Operation = NULL;

#if defined(ST_OSLINUX)
    spin_unlock_irqrestore (&current->sighand->siglock, flags);
#else
    STOS_InterruptUnlock();
#endif

    /* Did we time out? */
    if(Blaster_p->Timer.TimeoutError == TIMER_TIMEDOUT)
    {
        /* We did, so fill in the error code and params. */
        Op_p->Status = ST_ERROR_TIMEOUT;
    }

    /* Send out the event notification */
    DoneParams.Handle = Blaster_p->Handle;
    DoneParams.Result = Op_p->Status;

    STEVT_Notify(Blaster_p->EvtHandle,
                 Blaster_p->Events[EVENT_TO_INDEX(STBLAST_WRITE_DONE_EVT)],
                 &DoneParams);
}

/*****************************************************************************
BLAST_ReadCallback()
Description:

Parameters:
    Blaster_p           pointer to blaster control block
Return Value:
    none
See Also:
    STBLAST_Read()
*****************************************************************************/

static void BLAST_ReadCallback(BLAST_ControlBlock_t *Blaster_p)
{
    BLAST_Operation_t *Op_p = Blaster_p->Operation;
    STBLAST_EvtReadDoneParams_t DoneParams;

    /* Remove ourselves from servicing */
#if defined(ST_OSLINUX)
    unsigned int flags;
    spin_lock_irqsave (&current->sighand->siglock, flags);
#else
    STOS_InterruptLock();
#endif

    Blaster_p->Operation = NULL;

#if defined(ST_OSLINUX)
    spin_unlock_irqrestore (&current->sighand->siglock, flags);
#else
    STOS_InterruptUnlock();
#endif

    /* Did we time out? */
    if(Blaster_p->Timer.TimeoutError == TIMER_TIMEDOUT)
    {
        /* We did, so fill in the error code and params. */
        Op_p->Status = ST_ERROR_TIMEOUT;
    }

    /* Set up the event parameters */
    DoneParams.Handle = Blaster_p->Handle;

    /* So the caller can check the result */
    DoneParams.Result = Op_p->Status;

    /* Send out the event notification */
    STEVT_Notify(Blaster_p->EvtHandle,
                 Blaster_p->Events[EVENT_TO_INDEX(STBLAST_READ_DONE_EVT)],
                 &DoneParams);
}

/*****************************************************************************
BLAST_IsInit()
Description:
    Runs through the queue of initialized devices and checks that
    the blaster with the specified device name has not already been added.

    NOTE: Must be called with the queue semaphore held.

Parameters:
    DeviceName, text string indicating the device name to check.
Return Value:
    TRUE, the device has already been initialized.
    FALSE, the device is not in the queue and therefore is not initialized.
See Also:
    STBLAST_Init()
*****************************************************************************/
static BOOL BLAST_IsInit(const ST_DeviceName_t DeviceName)
{
    BOOL Initialized = FALSE;
    BLAST_ControlBlock_t *qp = BLASTQueueHead_p; /* Global queue head pointer */

    while(qp != NULL)
    {
        /* Check the device name for a match with the item in the queue */
        if(strcmp(qp->DeviceName, DeviceName) != 0)
        {
            /* Next BLAST in the queue */
            qp = qp->Next_p;
        }
        else
        {
            /* The BLAST is already initialized */
            Initialized = TRUE;
            break;
        }
    }

    /* Boolean return value reflecting whether the device is initialized */
    return Initialized;
} /* BLAST_IsInit() */

/*****************************************************************************
BLAST_QueueAdd()
Description:
    This routine appends an allocated BLAST control block to the
    BLAST queue.

    NOTE: The interrupt lock must be held when calling this routine.

Parameters:
    QueueItem_p, the control block to append to the queue.
Return Value:
    Nothing.
See Also:
    BLAST_QueueRemove()
*****************************************************************************/

static void BLAST_QueueAdd(BLAST_ControlBlock_t *QueueItem_p)
{
    BLAST_ControlBlock_t *qp;

    /* Check the base element is defined, otherwise, append to the end of the
     * linked list.
     */
    if(BLASTQueueHead_p == NULL)
    {
        /* As this is the first item, no need to append */
        BLASTQueueHead_p = QueueItem_p;
        QueueItem_p->Next_p = NULL;
    }
    else
    {
        /* Iterate along the list until we reach the final item */
        qp = BLASTQueueHead_p;
        while(qp != NULL && qp->Next_p)
        {
            qp = qp->Next_p;
        }

        /* Append the new item */
        qp->Next_p = QueueItem_p;
        QueueItem_p->Next_p = NULL;
    }

} /* BLAST_QueueAdd() */

/*****************************************************************************
BLAST_QueueRemove()
Description:
    This routine removes an allocated BLAST control block from the
    BLAST queue.

    NOTE: The interrupt lock must be held when calling this routine.
    Use BLAST_IsInit() or BLAST_GetControlBlockFromName() to verify whether
    or not a particular item is in the queue prior to calling this routine.

Parameters:
    QueueItem_p, the control block to remove from the queue.
Return Value:
    Nothing.
See Also:
    BLAST_QueueAdd()
*****************************************************************************/

static void BLAST_QueueRemove(BLAST_ControlBlock_t *QueueItem_p)
{
    BLAST_ControlBlock_t *this_p, *last_p;

    /* Check the base element is defined, otherwise quit as no items are
     * present on the queue.
     */
    if(BLASTQueueHead_p != NULL)
    {
        /* Reset pointers to loop start conditions */
        last_p = NULL;
        this_p = BLASTQueueHead_p;

        /* Iterate over each queue element until we find the required
         * queue item to remove.
         */
        while(this_p != NULL && this_p != QueueItem_p)
        {
            last_p = this_p;
            this_p = this_p->Next_p;
        }

        /* Check we found the queue item */
        if(this_p == QueueItem_p)
        {
            /* Unlink the item from the queue */
            if(last_p != NULL)
            {
                last_p->Next_p = this_p->Next_p;
            }
            else
            {
                /* Recalculate the new head of the queue i.e.,
                 * we have just removed the queue head.
                 */
                BLASTQueueHead_p = this_p->Next_p;
            }
        }
    }
} /* BLAST_QueueRemove() */


/*****************************************************************************
BLAST_PIOBitFromBitMask()
Description:
    This routine is used for converting a PIO bitmask to a bit value in
    the range 0-7. E.g., 0x01 => 0, 0x02 => 1, 0x04 => 2 ... 0x80 => 7

Parameters:
    BitMask, a bitmask - should only have a single bit set.
Return Value:
    The bit value of the bit set in the bitmask.
See Also:
    STBLAST_Init()
*****************************************************************************/
static U8 BLAST_PIOBitFromBitMask(U8 BitMask)
{
    U8 Count = 0;                       /* Initially assume bit zero */

    /* Shift bit along until the bitmask is set to one */
    while(BitMask != 1 && Count < 7)
    {
        BitMask >>= 1;
        Count++;
    }

    return Count;                       /* Should contain the bit [0-7] */
}

/*****************************************************************************
BLAST_GetControlBlockFromName()
Description:
    Runs through the queue of initialized BLAST devices and checks for
    the BLAST with the specified device name.

    NOTE: Must be called with the queue semaphore held.

Parameters:
    DeviceName, text string indicating the device name to check.
Return Value:
    NULL, end of queue encountered - invalid device name.
    Otherwise, returns a pointer to the control block for the device.
See Also:
    BLAST_IsInit()
*****************************************************************************/
BLAST_ControlBlock_t *
   BLAST_GetControlBlockFromName(const ST_DeviceName_t DeviceName)
{
    BLAST_ControlBlock_t *qp = BLASTQueueHead_p; /* Global queue head pointer */

    while(qp != NULL)
    {
        /* Check the device name for a match with the item in the queue */
        if(strcmp(qp->DeviceName, DeviceName) != 0)
        {
            /* Next BLAST in the queue */
            qp = qp->Next_p;
        }
        else
        {
            /* The BLAST entry has been found */
            break;
        }
    }

    /* Return the BLAST control block (or NULL) to the caller */
    return qp;
} /* BLAST_GetControlBlockFromName() */

/*****************************************************************************
BLAST_GetControlBlockFromHandle()
Description:
    Runs through the queue of initialized BLAST devices and checks for
    the BLAST with the specified handle.

    NOTE: Must be called with the queue semaphore held.

Parameters:
    Handle, an open handle.
Return Value:
    NULL, end of queue encountered - invalid handle.
    Otherwise, returns a pointer to the control block for the device.
See Also:
    BLAST_GetControlBlockFromName()
*****************************************************************************/
BLAST_ControlBlock_t *BLAST_GetControlBlockFromHandle(STBLAST_Handle_t Handle)
{
    BLAST_ControlBlock_t *qp = NULL;

    /* Check handle */
    if((void *)Handle != NULL)
    {
        /* Dereference and check for "magic" */
        qp = (BLAST_ControlBlock_t *)Handle;

        /* If magic failed, reset pointer to null */
        if (qp->Handle != Handle)
        {
            qp = NULL;
        }
    }

    /* Return the BLAST control block (or NULL) to the caller */
    return qp;
} /* BLAST_GetControlBlockFromHandle() */

/*****************************************************************************
DoDecode()
Description:


Parameters:

Return Value:

See Also:
*****************************************************************************/
void DoDecode(BLAST_ControlBlock_t *Blaster_p)
{
    U16 SymbolPeriod = 0,MarkPeriod = 0;
    U16 SymbolPeriod1 = 0,MarkPeriod1 = 0;
    STBLAST_Protocol_t   NewProtocol = 0;
    BLAST_Operation_t *Op;
    STBLAST_Symbol_t SymbolBuf_p[40]; /* maximum symbols available  - just a guess */
    U32 SymbolsAvailable;
    U32  UserBuf_p[10];
    U32 DecodeCount = 0;
    U32 SymbolsConsumed = 0;
    U32  TimeDiff;
    BlastInterruptData_t  *TmpBlastInterruptData_p = NULL;
    
    /*Bug GNBvd58162*/
    STOS_TaskEnter(Blaster_p);
    while(1)
    {
        TmpBlastInterruptData_p = STOS_MessageQueueReceiveTimeout(Blaster_p->InterruptQueue_p,
        														  TIMEOUT_INFINITY);

        if (Blaster_p->ReceiveTaskTerm == TRUE)
        {
            break;
        }

        if(TmpBlastInterruptData_p == NULL)
        {
            break; /* some error has occured */
        }		
        	
        memcpy(SymbolBuf_p,TmpBlastInterruptData_p->MessageSymbolBuffer,
              (TmpBlastInterruptData_p->MessageSymbolCount)*sizeof(STBLAST_Symbol_t));
              
        SymbolsAvailable = TmpBlastInterruptData_p->MessageSymbolCount;
        TimeDiff         = TmpBlastInterruptData_p->TimeDiff;
        						
#if defined(ST_OSLINUX)	 
#if defined(BLAST_API_DEBUG)         
        printk("Symbols available %d\n",SymbolsAvailable);
#endif        
#endif        
        /* Release the message back to empty queue */
        STOS_MessageQueueRelease(Blaster_p->InterruptQueue_p, TmpBlastInterruptData_p);

        /* Try to consume as many symbols as are available -- symbols
         * are not consumed unless a complete command can be
         * decoded.
         */

            if ( Blaster_p->Protocol == STBLAST_PROTOCOL_MULTIPLE )
            {
        	    /* Deduce protocol from Mark and Symbol period */
                SymbolPeriod  = SymbolBuf_p[0].SymbolPeriod;
                MarkPeriod    = SymbolBuf_p[0].MarkPeriod;
                SymbolPeriod1 = SymbolBuf_p[1].SymbolPeriod;
                MarkPeriod1   = SymbolBuf_p[1].MarkPeriod;

                switch (Blaster_p->SubCarrierFreq)
                {

                    case 36000:
                    case 38000:

                    if ((( MarkPeriod >= RC5_BIT_PERIOD) && (MarkPeriod <= RC5_BIT_PERIOD_MAX )) &&  /* 50% */ \
                        (( SymbolPeriod >= 2*RC5_BIT_PERIOD) && (SymbolPeriod <= 2*RC5_BIT_PERIOD_MAX)))
                    {
                        NewProtocol = STBLAST_PROTOCOL_RC5;
                    }
                    else if ((MarkPeriod >= RC6AM0_MIN_6T && MarkPeriod <= RC6AM0_MAX_6T ) && \
                             (SymbolPeriod1 >= RC6AM0_MIN_3T) && (SymbolPeriod1 <= RC6AM0_MAX_3T) )
                    {
                        NewProtocol = STBLAST_PROTOCOL_RC6_MODE0;
                    }
                    else if ((MarkPeriod >= RC6A_MIN_6T && MarkPeriod <= RC6A_MAX_6T ) && \
                             (SymbolPeriod1 >= RC6A_MIN_2T) && (SymbolPeriod1 <= RC6A_MAX_2T) )
                    {
                        NewProtocol = STBLAST_PROTOCOL_RC6A;
                    }
                    else if (((MarkPeriod >= RCMM_MIN_HEADER_ON )&& (MarkPeriod <= RCMM_MAX_HEADER_ON) ) && \
                             ((SymbolPeriod >= RCMM_MIN_HEADER_TIME) && (SymbolPeriod <= RCMM_MAX_HEADER_TIME) ))
                    {
                        NewProtocol = STBLAST_PROTOCOL_RCMM;
                    }
                    else if (((MarkPeriod >= RUWIDO_HALF_LEVEL_DURATION_MIN) && (MarkPeriod <= RUWIDO_HALF_LEVEL_DURATION_MAX) ) && \
                             ((SymbolPeriod >= RUWIDO_SYMBOL_DURATION_3T_MIN) && (SymbolPeriod <= RUWIDO_SYMBOL_DURATION_3T_MAX)))
                    {
                        NewProtocol = STBLAST_PROTOCOL_RUWIDO;
                    }
                    else if (((MarkPeriod >= RMAP_HALF_LEVEL_DURATION) && (MarkPeriod <= RMAP_HALF_LEVEL_DURATION_MAX) ) && \
                              ((SymbolPeriod >= 2*RMAP_HALF_LEVEL_DURATION) && (SymbolPeriod <= 2*RMAP_HALF_LEVEL_DURATION_MAX)))
                    {
                        NewProtocol = STBLAST_PROTOCOL_RMAP;
                    }
                    else if ((SymbolPeriod >= LOW_MIN_12T) && (SymbolPeriod <= LOW_MAX_12T))
                    {
                        NewProtocol = STBLAST_LOW_LATENCY_PROTOCOL;
                    }

                    break;

                    case 56000:
                    
		            if (((MarkPeriod >= 216) && (MarkPeriod <= 250) ) && \
		                   ((SymbolPeriod >= 2*216) && (SymbolPeriod <= 2*375)))
					{             
						NewProtocol = STBLAST_PROTOCOL_RMAP_DOUBLEBIT;      
		            }
         
                    else if ((SymbolPeriod >= LOW_MIN_12T) && (SymbolPeriod <= LOW_MAX_12T))
                    {
                        NewProtocol = STBLAST_LOW_LATENCY_PRO_PROTOCOL;
                    }

                }

            }
            else
            {
        	    NewProtocol = Blaster_p->Protocol;

            }

            switch(NewProtocol)
            {

                case STBLAST_PROTOCOL_USER_DEFINED:

                if((Blaster_p->ProtocolParams.UserDefined.Coding==STBLAST_CODING_SPACE)||
                   (Blaster_p->ProtocolParams.UserDefined.Coding==STBLAST_CODING_PULSE))
                {
		                BLAST_SpaceDecode(UserBuf_p,
			                              1,
			                              SymbolBuf_p,
			                              SymbolsAvailable,
			                              &DecodeCount,
			                              &SymbolsConsumed,
			                              &Blaster_p->ProtocolParams,
			                              TimeDiff);
                }
                else
                {
                    BLAST_ShiftDecode(UserBuf_p,
                    			  1,
                                  SymbolBuf_p,
                                  SymbolsAvailable,
                                  &DecodeCount,
                                  &SymbolsConsumed,
                                  &Blaster_p->ProtocolParams);

                }
                break;

                case STBLAST_PROTOCOL_RC5:

                BLAST_RC5Decode(UserBuf_p,
                                SymbolBuf_p,
                                SymbolsAvailable,
                                &DecodeCount,
                                &SymbolsConsumed);
                                
                break;


                case STBLAST_PROTOCOL_RCRF8:

                BLAST_RCRF8Decode(UserBuf_p,
                				  SymbolBuf_p,
                                  SymbolsAvailable,
                                  &DecodeCount,
                                  &SymbolsConsumed,
                                  &Blaster_p->ProtocolParams);
                break;

                case STBLAST_LOW_LATENCY_PROTOCOL:

                BLAST_LowLatencyDecode(UserBuf_p,
                                SymbolBuf_p,
                                SymbolsAvailable,
                                &DecodeCount,
                                &SymbolsConsumed,
                                &Blaster_p->ProtocolParams);
                break;

                case STBLAST_PROTOCOL_RC6A:

                BLAST_RC6ADecode(UserBuf_p,
                                SymbolBuf_p,
                                SymbolsAvailable,
                                &DecodeCount,
                                &SymbolsConsumed,
                                &Blaster_p->ProtocolParams);
                break;

                case STBLAST_PROTOCOL_RUWIDO:

                BLAST_RuwidoDecode(UserBuf_p,
                                  SymbolBuf_p,
                                  SymbolsAvailable,
                                  &DecodeCount,
                                  &SymbolsConsumed);
                break;

                case  STBLAST_PROTOCOL_RC6_MODE0:

                BLAST_RC6ADecode_Mode0(UserBuf_p,
                                       SymbolBuf_p,
                                       SymbolsAvailable,
                                       &DecodeCount,
                                       &SymbolsConsumed,
                                       &Blaster_p->ProtocolParams);
                break;

                case STBLAST_PROTOCOL_RCMM:

                BLAST_RcmmDecode(UserBuf_p,
                                 SymbolBuf_p,
                                 SymbolsAvailable,
                                 &DecodeCount,
                                 &SymbolsConsumed,
                                 &Blaster_p->ProtocolParams);
                break;

                case STBLAST_PROTOCOL_RMAP:
                case STBLAST_PROTOCOL_RSTEP:

                BLAST_RMapDecode(UserBuf_p,
                                 SymbolBuf_p,
                                 SymbolsAvailable,
                                 &DecodeCount,
                                 &Blaster_p->ProtocolParams);
                break;
                
                case STBLAST_PROTOCOL_RMAP_DOUBLEBIT:
                
                BLAST_RMapDoubleBitDecode(UserBuf_p,
		                                  SymbolBuf_p,
		                                  SymbolsAvailable,
		                                  &DecodeCount,
		                                  &SymbolsConsumed,
		                                  &Blaster_p->ProtocolParams);
                 
				break;
				
				case STBLAST_LOW_LATENCY_PRO_PROTOCOL:
                BLAST_LowLatencyProDecode(UserBuf_p,
						                  SymbolBuf_p,
						                  SymbolsAvailable,
						                  &DecodeCount,
						                  &SymbolsConsumed,
						                  &Blaster_p->ProtocolParams);
				
				default:
                break;

            }
            Blaster_p->ProtocolSet = (STBLAST_Protocol_t)NewProtocol; 
        if(Blaster_p->Operation != NULL)
        {
             /* Shortens things */
             Op = Blaster_p->Operation;
             
   		     memset(Op->UserBuf_p,0,sizeof(Op->UserBuf_p));
	         memcpy(Op->UserBuf_p,UserBuf_p,DecodeCount * sizeof(Op->UserBuf_p));

             Op->UserBuf_p += DecodeCount;
             
             if ( DecodeCount > 1 )
             {
                 	Op->UserRemaining = 0;
             }
             else
             {
                 	Op->UserRemaining -= DecodeCount;
             }
             
             *Op->UserCount_p += DecodeCount;
             
             if (DecodeCount == 0)
             {
   	              	/*Set the status as pending.This will allow the next interrupt to be processed*/
                  	Op->Status = (U32)BLAST_OP_PENDING;
             }
             else if( (Op->UserRemaining == 0) || (Op->Status != (U32)BLAST_OP_PENDING))
             {
	                /* No error occurred */
	                if (Op->Status == (U32)BLAST_OP_PENDING)
	                {
	                    	Op->Status = ST_NO_ERROR;
	                }
                    /* Wake-up the timer to complete the operation */
                    stblast_TimerSignal(&Blaster_p->Timer, FALSE);
             }
             else if(DecodeCount > 0)    /* Data received? */
             {
				    if(Op->UserRemaining >= 1)
				   	{
		                     /* More data received - restart the timeout */
		                    stblast_TimerSignal(&Blaster_p->Timer, TRUE);
				   	}
             }
        } /* end of "if (Blaster->Operation !=NULL)" */

    } /* end of while loop */
	STOS_TaskExit(Blaster_p);         
}
/* End of stblast.c */
