/*****************************************************************************
File Name   : blastint.c

Description : STBLAST driver private routines.

Copyright (C) 2002 STMicroelectronics

History     :

Reference   :

*****************************************************************************/

/* Includes --------------------------------------------------------------- */
#if !defined (ST_OSLINUX)
#include <string.h>                     /* C libs */
#include <stdio.h>
#include <stdlib.h>
#endif

#include "stos.h"

#if !defined (ST_OSLINUX)
#include "stlite.h"                     /* Standard includes */
#endif

#include "stddefs.h"
#include "stcommon.h"
#include "stpio.h"                      /* Dependencies */
#include "stblast.h"                    /* STAPI header file */
#include "timer.h"                      /* Private includes */
#include "blastint.h"
#include "blasthw.h"

#include "space.h"                      /* Coding routines */
#include "rc5.h"
#include "rc6a.h"
#include "shift.h"
#include "ruwido.h"
#include "rc6aM0.h"
#include "rcrf8.h"
#include "lowlatency.h"
#include "rcmm.h"
#include "rmap.h"
#include "rmapdoublebit.h"
#include "lowlatencyPro.h"

#define TIMEOUTVALUE 0xFFFF

BOOL SymbolsOkay(const STBLAST_Symbol_t *Incoming,
                 const STBLAST_Symbol_t *Source,
                 const U32 Count);
                 
/*************************************************************************** */
/* Private variables */
/*************************************************************************** */

/*************************************************************************** */
/* Function bodies */
/*************************************************************************** */
/* Function implementations */
#if defined (ST_OSLINUX)
irqreturn_t BLAST_InterruptHandler(int irq, void* dev_id, struct pt_regs* regs)
#else
#ifdef ST_OS21
int  BLAST_InterruptHandler(BLAST_ControlBlock_t **BLASTQueueHead_p)
#else
void BLAST_InterruptHandler(BLAST_ControlBlock_t **BLASTQueueHead_p)
#endif
#endif /* ST_OSLINUX */
{
#if defined(ST_OSLINUX)
    BLAST_ControlBlock_t **BLASTQueueHead_p;
#endif
    BLAST_ControlBlock_t *Blaster_p;
    BLAST_BlastRegs_t *BlastRegs_p;
    BLAST_Operation_t *Op;
    STBLAST_Symbol_t *Symbol_p;
    U32 Status, DataCount=0, i;
    static U32  InterruptCount;
    BlastInterruptData_t *BlastInterruptData_p;

    /* Run down the linked list of 'initialized' devices --
     * check which one needs servicing.
     */
#if defined(ST_OSLINUX)
    BLASTQueueHead_p = (BLAST_ControlBlock_t **)dev_id;
    if (BLASTQueueHead_p == NULL)
    {
        return (IRQ_HANDLED);
    }
    if (* BLASTQueueHead_p == NULL)
    {
        return (IRQ_HANDLED);
    }
#endif
    Blaster_p = *BLASTQueueHead_p;

    while (Blaster_p != NULL)
    {
        /* Map the registers to the address */
        BlastRegs_p = Blaster_p->Regs_p;

        if ((Blaster_p->DeviceType != STBLAST_DEVICE_IR_RECEIVER) &&
                (Blaster_p->DeviceType != STBLAST_DEVICE_UHF_RECEIVER)
            )
        {
            Status = BLAST_TxGetStatus(BlastRegs_p);
        }
        else
        {
            Status = BLAST_RxGetStatus(BlastRegs_p);
        }
        /* Shortens things */
        Op = Blaster_p->Operation;


        /***********************************************/
        /* Check receive */

#ifdef NEW_INTERRUPT_MECHANISM
        /* Just check in the status any byte in rx_status_ir
           Any data is present in FIFO */
        if (((Blaster_p->InitParams.DeviceType == STBLAST_DEVICE_IR_RECEIVER)||
               (Blaster_p->InitParams.DeviceType == STBLAST_DEVICE_UHF_RECEIVER))
             && ((Status & RXIS_ONEFULL) != 0))
#else
        if (((Blaster_p->InitParams.DeviceType == STBLAST_DEVICE_IR_RECEIVER)||
               (Blaster_p->InitParams.DeviceType == STBLAST_DEVICE_UHF_RECEIVER))
             && ((Status & RXIS_ENABLE) != 0))
#endif
        {
            /* Operation pending? */
            if (Op != NULL && Op->Status == (ST_ErrorCode_t)BLAST_OP_PENDING)
            {
                BOOL Timeout = FALSE;

                /* Check for an overrun error */
                if (BLAST_IsOverrun(Status))
                {
                    U16 Mark, Symbol;

                    /* Clear the overrun condition */
                    Mark = BLAST_RxGetOnTime(BlastRegs_p);
                    Symbol = BLAST_RxGetSymbolTime(BlastRegs_p);

                    /* An overrun has occurred */
                    Op->Status = STBLAST_ERROR_OVERRUN;

                    /* clear rx_status_ir by reading all bytes
                       and clear One full interrupt in
                       rx_int_status_ir register  */
#ifdef NEW_INTERRUPT_MECHANISM

                    for (i=0;i<(FIFO_SIZE - 1);i++)
                    {
                        Mark = BLAST_RxGetOnTime(BlastRegs_p);
                        Symbol = BLAST_RxGetSymbolTime(BlastRegs_p);
                    }

                    BLAST_ClrONEFULL(BlastRegs_p);

#else
                    /* Clear it from the blaster device */
                    BLAST_ClrOverrun(BlastRegs_p);
#endif

                }
                else
                {
                    /* Reset data count */
                    DataCount = 0;
                    InterruptCount++;

                    /* Point to first available symbol in buffer */
                    Symbol_p = Blaster_p->SymbolBuf_p;

#ifdef NEW_INTERRUPT_MECHANISM
                    /* Read them into the symbol buffer, if there's space */
                    /* As interrupt mechanism has been changed,
                       Just check any data is there in FIFO */
                    while(((Status & RXIS_ONEFULL) != 0) &&
                            Blaster_p->SymbolBufFree > 0 &&
                            !Timeout)
#else
                    /* Read them into the symbol buffer, if there's space */
                    while(((Status & RXIS_THREEFULL) != 0) &&
                            Blaster_p->SymbolBufFree > 0 &&
                            !Timeout)
#endif
                    {
                        U32 MarkPeriod, SymbolPeriod;

                        /* Read the symbol data */
                        MarkPeriod = BLAST_RxGetOnTime(BlastRegs_p);
                        SymbolPeriod = BLAST_RxGetSymbolTime(BlastRegs_p);

                        if ((SymbolPeriod < Blaster_p->TimeoutValue && !Op->RawMode)||
                              (SymbolPeriod < TIMEOUTVALUE && Op->RawMode)
                           )
                        {
                            /* Apply adjustment factor (in KHz) to reflect the
                             * actual clock supplied to the module
                             * (ideally 10MHz).
                             */
                            MarkPeriod = (MarkPeriod * 10000) /
                                          Blaster_p->ActualClockInKHz;
                            SymbolPeriod = (SymbolPeriod * 10000) /
                                          Blaster_p->ActualClockInKHz;


                        }
                        else
                        {
                            Timeout = TRUE;
#ifdef NEW_INTERRUPT_MECHANISM
                            BLAST_ClrONEFULL(BlastRegs_p);
#endif
                        }

                        /* Store symbol in buffer */
                        Symbol_p->MarkPeriod = MarkPeriod;
                        Symbol_p->SymbolPeriod = SymbolPeriod;

                        /* Next symbol */
                        Symbol_p++;

                        /* Update symbol buffer counts */
                        Blaster_p->SymbolBufCount++;
                        Blaster_p->SymbolBufFree--;
                        Blaster_p->SymbolBuf_p++;
                        DataCount++;


                        /* Read status again - in case of more data */
                        Status = BLAST_RxGetStatus(BlastRegs_p);

                        if (Blaster_p->SymbolBufFree == 0) /* to avoid overflow */
                        {
                            Timeout = TRUE;

#ifdef NEW_INTERRUPT_MECHANISM
                            BLAST_ClrONEFULL(BlastRegs_p);
#endif
                        }
                    } /* end of symbol read while loop */

                    if (Timeout  && !Op->RawMode ) /* New batch to decode*/ 
                    {
                         /* Evaluating the present command time in the interrupt */
                         Blaster_p->PresentCommandTime = STOS_time_now();
                         Blaster_p->TimeDiff = STOS_time_minus(Blaster_p->PresentCommandTime, Blaster_p->LastCommandTime);
#if defined(ST_OSLINUX)                        
						 Blaster_p->TimeDiff = (Blaster_p->TimeDiff*1000)/ST_GetClocksPerSecond();
#else
#if (__CORE__ == 1)
                         Blaster_p->TimeDiff /= (ST_GetClocksPerSecond()/1000);
                    
#else
                         Blaster_p->TimeDiff = (Blaster_p->TimeDiff*1000)/ST_GetClocksPerSecond();
#endif
#endif	/*ST_OSLINUX*/

                         Blaster_p->LastCommandTime = Blaster_p->PresentCommandTime;

                         BlastInterruptData_p = STOS_MessageQueueClaimTimeout(Blaster_p->InterruptQueue_p, TIMEOUT_IMMEDIATE);

                         if ( BlastInterruptData_p != NULL )
                         {
                             strcpy(BlastInterruptData_p->DeviceName, Blaster_p->DeviceName);
                             BlastInterruptData_p->TimeDiff   = Blaster_p->TimeDiff;
                             BlastInterruptData_p->MessageSymbolCount = Blaster_p->SymbolBufCount;
                             
                             
                             memcpy(BlastInterruptData_p->MessageSymbolBuffer,
                                    Blaster_p->SymbolBufBase_p,
                                    ((Blaster_p->SymbolBufCount)*(sizeof(STBLAST_Symbol_t))));

                             STOS_MessageQueueSend( Blaster_p->InterruptQueue_p, BlastInterruptData_p );

                            /* Reset symbol buffer to beginning */
                             Blaster_p->SymbolBufCount = 0;
                             Blaster_p->SymbolBufFree = Blaster_p->SymbolBufSize;
                             Blaster_p->SymbolBuf_p   = Blaster_p->SymbolBufBase_p;
                             DataCount = 0; /* reset */

                         }
                         else
                         {
                            /* hit the notification that the queue is full */
                         }
                    }
                    else if(Op->RawMode) /* Raw mode? */
                    {
                        /* Update user counts */
                        Op->UserRemaining -= DataCount;
                        *Op->UserCount_p += DataCount;

                        /* Did symbol timeout occur? */
                        if(Timeout)
                        {
                            Op->Status = ST_ERROR_TIMEOUT;
                        }
                    }

                } /* End of else */

                /* If the incoming signal is in error do nothing else signal the timer */
                if( (Op->UserRemaining == 0 || Op->Status != (U32)BLAST_OP_PENDING))
                {
                    /* No error occurred */
                    if (Op->Status == (U32)BLAST_OP_PENDING)
                    {
                        Op->Status = ST_NO_ERROR;
#ifdef NEW_INTERRUPT_MECHANISM
                        BLAST_ClrONEFULL(BlastRegs_p);
#endif
                    }
                    /* Wake-up the timer to complete the operation */
                    stblast_TimerSignal(&Blaster_p->Timer, FALSE);
                }
                else if(DataCount > 0)    /* Data received? */
                {

                    /* Reenable the BLAST as more data has to be received*/
                    /* More data received - restart the timeout */
                    stblast_TimerSignal(&Blaster_p->Timer, TRUE);

                    /*  Clr the status register as receive has been completed
                        for this interrupt */
#ifdef NEW_INTERRUPT_MECHANISM
                    BLAST_ClrONEFULL(BlastRegs_p);
#endif

                }
            }
            else
            {
                U16 Mark, Symbol;

                /* Just consume the symbols, as there's no operation
                 * pending.
                 */
                Mark = BLAST_RxGetOnTime(BlastRegs_p);
                Symbol = BLAST_RxGetSymbolTime(BlastRegs_p);

                /* Clear rx_status_ir by reading all bytes
                  and clear one byte data from
                  rx_int_status_ir register */
#ifdef NEW_INTERRUPT_MECHANISM
                for (i=0;i<(FIFO_SIZE - 1);i++)
                {
                    Mark = BLAST_RxGetOnTime(BlastRegs_p);
                    Symbol = BLAST_RxGetSymbolTime(BlastRegs_p);
                }
                BLAST_ClrONEFULL(BlastRegs_p);
#else
                /* Clear it from the blaster device */
                BLAST_ClrOverrun(BlastRegs_p);
#endif
            }
        }
        /***********************************************/
        /* Check transmit */
#ifdef NEW_INTERRUPT_MECHANISM

        /* As interrupt mechanism has been changed
          just check whether space is there to send
          atleast 1 bytes */
        else if((Blaster_p->InitParams.DeviceType ==
                  STBLAST_DEVICE_IR_TRANSMITTER) &&
                 ((Status & TXIE_ONEEMPTY) != 0))

#else
        /* check whether space is there to send
          atleast 2 bytes */
        else if((Blaster_p->InitParams.DeviceType ==
                  STBLAST_DEVICE_IR_TRANSMITTER) &&
                 ((Status & TXIS_ENABLE) != 0))
#endif

        {
        	BLAST_TXClrInts(BlastRegs_p);
            /* Operation pending? */
            if (Op != NULL && Op->Status == (ST_ErrorCode_t)BLAST_OP_PENDING)
            {
                /* Update symbol buffer count with data just sent */
                Blaster_p->SymbolBufCount -= Op->UserPending;
                Blaster_p->SymbolBuf_p += Op->UserPending;

                if (Op->RawMode)     /* Raw mode? */
                {
                    /* Update user symbols remaining */
                    Op->UserRemaining -= Op->UserPending;
                    *Op->UserCount_p += Op->UserPending;
                }

                /* Are there more symbols to send? */
                if (Blaster_p->SymbolBufCount == 0)
                {
                    if (!Op->RawMode) /* Protocol mode? */
                    {
                        /* A command has been sent */
                        Op->UserRemaining--;
                        Op->UserBuf_p++;
                        (*Op->UserCount_p)++;
                    }

                    /* Is there more data to encode? */
                    if (Op->UserRemaining > 0)
                    {
                        if (!Op->RawMode) /* Encode? */
                        {
                            U32 SymbolsEncoded;

                            /* Encode next command */
                            BLAST_DoEncode(Op->UserBuf_p,
                                           1,
                                           Blaster_p->SymbolBufBase_p,
                                           Blaster_p->SymbolBufSize,
                                           &SymbolsEncoded,
                                           Blaster_p->Protocol,
                                           &Blaster_p->ProtocolParams);

                            /* Update symbol buffer */
                            Blaster_p->SymbolBuf_p = Blaster_p->SymbolBufBase_p;
                            Blaster_p->SymbolBufCount = SymbolsEncoded;

                            /* If this isn't the last command, add on the
                             * inter-command delay period.
                             */
                            if (Op->UserRemaining > 1)
                            {
                                U32 SymbolPeriod;

                                /* Add on the protocol delay to the last symbol
                                 * that is transmitted as part of the command.
                                 * Be careful we don't overflow the 16-bit
                                 * register.
                                 */
                                SymbolPeriod = Blaster_p->SymbolBuf_p[
                                        Blaster_p->SymbolBufCount - 1
                                        ].SymbolPeriod +
                                        Blaster_p->Delay;
                                SymbolPeriod = min(65535, SymbolPeriod);

                                Blaster_p->SymbolBuf_p[
                                        Blaster_p->SymbolBufCount - 1
                                        ].SymbolPeriod = SymbolPeriod;
                            }
                        }
                    }
                    else
                    {
                        /* Operation is complete */
                        BLAST_TxDisableInt(BlastRegs_p);

                        /* Set error code and wake-up timer */
                        Op->Status = ST_NO_ERROR;
                        stblast_TimerSignal(&Blaster_p->Timer, FALSE);
                        continue;
                    }
                }

                /* Send next batch of data (if any) */
                if (Blaster_p->SymbolBufCount > 0)
                {
                    /* Work out how many symbols we can put in the FIFO */
#ifdef NEW_INTERRUPT_MECHANISM
                    /* To utilize the fifo of 7 words*/
                    /* if  want to utilize the FIFO,
                      put (FIFO_SIZE-1) in place of 1 */
                    /* But Then it does not syncronize with reciver
                       As receiver waits for any symbol in FIFO*/
                   /* DataCount  = (FIFO_SIZE-1) - ((Status & DATA_IN_TX_FIFO_MASK) >> FIFO_SIZE); */
                    DataCount  = 1 - ((Status & DATA_IN_TX_FIFO_MASK) >> FIFO_SIZE);

#else
                    DataCount = (FIFO_SIZE-1) - ((Status & TXIS_THREEEMPTY) >> FIFO_SIZE);
#endif
                    DataCount = min(DataCount, Blaster_p->SymbolBufCount);

                    /* Next symbol to send */
                    Symbol_p = Blaster_p->SymbolBuf_p;

                    /* Send the data */
                    for (i = 0; i < DataCount; i++)
                    {
                        if (!Op->RawMode)
                        {
                            /* If not in raw mode, the symbols have already
                             * been converted from microsec. to the required
                             * units
                             */
                            BLAST_TxSetSymbolTime(BlastRegs_p,
                                Symbol_p->SymbolPeriod);
                            BLAST_TxSetOnTime(BlastRegs_p,
                                              Symbol_p->MarkPeriod);
                        }
                        else
                        {
                            /* Whereas in raw mode, they're still in usecs so
                             * convert them before passing them to the hardware.
                             */
                            BLAST_TxSetSymbolTime(
                                BlastRegs_p,
                                MICROSECONDS_TO_SYMBOLS(
                                Blaster_p->SubCarrierFreq,
                                Symbol_p->SymbolPeriod)
                                );

                            BLAST_TxSetOnTime(
                                BlastRegs_p,
                                MICROSECONDS_TO_SYMBOLS(
                                Blaster_p->SubCarrierFreq,
                                Symbol_p->MarkPeriod)
                                );
                        }

                        Symbol_p++;
                    }

                    /* Store the size of the next batch we're sending */
                    Op->UserPending = DataCount;

                    if (DataCount > 0)
                    {
                        /* Re-start the timer as we've sent more data */
                        stblast_TimerSignal(&Blaster_p->Timer, TRUE);
                    }
                }
            }
            else
            {
                /* Operation is null, we're done, turn the interrupts off */
                /* For 5100/7710/5105/5107/5301 no need to clear the status register
                   disableing interrupt automatically clears the status register
                 */
                BLAST_TxDisableInt(BlastRegs_p);
            }
        }

        /* Try next device in chain */
        Blaster_p = Blaster_p->Next_p;
    }

#if defined (ST_OSLINUX)
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,0))
       return (IRQ_HANDLED);
#else
       return;
#endif
#else
#ifdef ST_OS21
    return(OS21_SUCCESS);
#else
    return;
#endif
#endif
}

/*****************************************************************************
BLAST_MapRegisters()
Description:
    Set the receive/transmit register structs in the register struct in the
    control block to the appropriate values (depending on transmit/receive,
    and which number of receiver it is).

Parameters:
    Blaster_p               pointer to a blaster control block

See Also:
    STBLAST_Init
*****************************************************************************/

void BLAST_MapRegisters(BLAST_ControlBlock_t *Blaster_p)
{
    U32 DeviceNumber = 0;
    U32 Rate = 0;

#if defined(ST_OSLINUX)
    /* Remap base address under Linux */
    Blaster_p->MappingWidth = STBLAST_MAPPING_WIDTH;
    if (check_mem_region((unsigned long)Blaster_p->InitParams.BaseAddress,
                         Blaster_p->MappingWidth) < 0 )
    {
        printk("blast: checking memory busy \n");
        printk("blast: assumed memory has been mapped\n");
    }

    request_mem_region((unsigned long)Blaster_p->InitParams.BaseAddress,
                       Blaster_p->MappingWidth, "BLAST");

    Blaster_p->MappingBaseAddress = (unsigned long *)ioremap(
                                               (unsigned long)Blaster_p->InitParams.BaseAddress,
                                               Blaster_p->MappingWidth);

    if (Blaster_p->MappingBaseAddress == NULL)
    {
        release_mem_region((unsigned long)Blaster_p->InitParams.BaseAddress,
                           Blaster_p->MappingWidth);
        printk("blast: ioremap failed -> STBLAST_ERROR_IOREMAP \n");
        return;
    }

    printk("blast:  virtual io kernel address of phys %lX = %lX\n",
           ((unsigned long)Blaster_p->InitParams.BaseAddress ),
           (unsigned long)Blaster_p->MappingBaseAddress);

#endif

    /* Setup shortcut to registers */
    Blaster_p->Regs_p = &Blaster_p->Regs;

/* For execting on espresso it needs to be use the UHF section register */
#if defined(espresso)

    /* Used for RxRegs offset calculation */
    if(Blaster_p->DeviceType == STBLAST_DEVICE_IR_RECEIVER)
    {
        DeviceNumber = 2;
    }
    else if(Blaster_p->DeviceType == STBLAST_DEVICE_UHF_RECEIVER)
    {
        DeviceNumber = 1;
    }
    else
    {
        DeviceNumber = 0; /* not needed */
    }

#else

    /* Used for RxRegs offset calculation */
    if(Blaster_p->DeviceType == STBLAST_DEVICE_IR_RECEIVER)
    {
        DeviceNumber = 1;
    }
    else if(Blaster_p->DeviceType == STBLAST_DEVICE_UHF_RECEIVER)
    {
        DeviceNumber = 2;
    }
    else
    {
        DeviceNumber = 0; /* not needed */
    }
#endif


    /* Compute register base addresses based on device type */
    if((Blaster_p->InitParams.DeviceType == STBLAST_DEVICE_IR_RECEIVER) ||
        (Blaster_p->InitParams.DeviceType == STBLAST_DEVICE_UHF_RECEIVER)
       )
    {
        /* Receiver base address */
#if defined(ST_OSLINUX)
        Blaster_p->Regs_p->RxRegs_p =
            (BLAST_RxRegs_t *)
                ((U32)Blaster_p->MappingBaseAddress +
                    (RX_REGS_OFFSET * DeviceNumber));
#else
        Blaster_p->Regs_p->RxRegs_p =
            (BLAST_RxRegs_t *)
                ((U32)Blaster_p->InitParams.BaseAddress +
                    (RX_REGS_OFFSET * DeviceNumber));
#endif

#if defined(espresso)
        if(Blaster_p->InitParams.DeviceType == STBLAST_DEVICE_IR_RECEIVER)
        {
            /* Polarity Invert for RX */
#if defined(ST_OSLINUX)
            Blaster_p->Regs_p->RxPolarityInvert =
                (BLAST_DU32 *)
                ((U32))Blaster_p->MappingBaseAddress +
                    IRB_POLINV_REG_UHF);
#else
            Blaster_p->Regs_p->RxPolarityInvert =
                (BLAST_DU32 *)
                ((U32)Blaster_p->InitParams.BaseAddress +
                    IRB_POLINV_REG_UHF);
#endif
        }
        else
        {
            /* Polarity Invert for RX */
#if defined(ST_OSLINUX)
            Blaster_p->Regs_p->RxPolarityInvert =
                (BLAST_DU32 *)
                ((U32)Blaster_p->MappingBaseAddress +
                    RX_POLARITY_INVERT_OFFSET);
#else
            Blaster_p->Regs_p->RxPolarityInvert =
                (BLAST_DU32 *)
                ((U32)Blaster_p->InitParams.BaseAddress +
                    RX_POLARITY_INVERT_OFFSET);
#endif
        }
#else /* not espresso */
        if(Blaster_p->InitParams.DeviceType == STBLAST_DEVICE_IR_RECEIVER)
        {
            /* Polarity Invert for RX */
#if defined(ST_OSLINUX)
            Blaster_p->Regs_p->RxPolarityInvert =
                (BLAST_DU32 *)
                ((U32)Blaster_p->MappingBaseAddress +
                    RX_POLARITY_INVERT_OFFSET);
#else
            Blaster_p->Regs_p->RxPolarityInvert =
                (BLAST_DU32 *)
                ((U32)Blaster_p->InitParams.BaseAddress +
                    RX_POLARITY_INVERT_OFFSET);
#endif
        }
        else
        {
#if defined(ST_OSLINUX)
            /* Polarity Invert for RX */
            Blaster_p->Regs_p->RxPolarityInvert =
                (BLAST_DU32 *)
                ((U32)Blaster_p->MappingBaseAddress +
                 IRB_POLINV_REG_UHF);
#else
            Blaster_p->Regs_p->RxPolarityInvert =
                (BLAST_DU32 *)
                ((U32)Blaster_p->InitParams.BaseAddress +
                    IRB_POLINV_REG_UHF);
#endif
        }
#endif

    }
    else
    {
         /* Transmitter base address */
#if defined(ST_OSLINUX)
        Blaster_p->Regs_p->TxRegs_p =
            (BLAST_TxRegs_t *)((U32)Blaster_p->MappingBaseAddress +
                               TX_REGS_OFFSET);
#else
        Blaster_p->Regs_p->TxRegs_p =
            (BLAST_TxRegs_t *)((U32)Blaster_p->InitParams.BaseAddress +
                               TX_REGS_OFFSET);
#endif
    }

    /* Common receiver sampling rate */
#if defined(ST_OSLINUX)
    Blaster_p->Regs_p->RxSamplingRateCommon =
        (BLAST_DU32 *)
            ((U32)Blaster_p->MappingBaseAddress +
                RX_SAMPLING_RATE_OFFSET);
#else
    Blaster_p->Regs_p->RxSamplingRateCommon =
        (BLAST_DU32 *)
            ((U32)Blaster_p->InitParams.BaseAddress +
                RX_SAMPLING_RATE_OFFSET);
#endif

    /* We need a 10MHz clock from the CPU clock - this is used to generate an
     * internal 1MHz clock for microsecond resolution timing.  Note
     * that we apply a scaling factor to all symbols received to allow
     * for non-divisible input frequencies.
     */
    Rate = (Blaster_p->InitParams.ClockSpeed + 5000000) / 10000000;
    BLAST_SetSamplingRate(Blaster_p->Regs_p, Rate);

    /* Record actual clock (in KHz) for scaling the symbol periods returned
     * by the hardware device.
     */
    Blaster_p->ActualClockInKHz = Blaster_p->InitParams.ClockSpeed / Rate;
    Blaster_p->ActualClockInKHz /= 1000;
}

/*****************************************************************************
BLAST_SetDutyCycle()
Description:
    Compute carrier width register value based on current
    duty cycle parameter, defined in control block.
Parameters:
    Blaster_p   pointer to a blaster control block
See Also:
    STBLAST_Init
*****************************************************************************/

void BLAST_SetDutyCycle(BLAST_ControlBlock_t *Blaster_p, U8 DutyCycle)
{
    U32 Reg;
    U8 LocalDutyCycle;

    if((DutyCycle<25) || (DutyCycle>75))
    {
        LocalDutyCycle = 50;
    }
    else
    {
        LocalDutyCycle = DutyCycle;
    }

    /* The subcarrier width is defined in prescaled clock cycles i.e.,
     * in the same units as the subcarrier frequency.
     * Note that the subcarrier width can not exceed the
     * total subcarrier period.  So, in order to apply the duty
     * cycle we first read the subcarrier period register and scale it
     * according to the required duty cycle.
     */
    Reg = BLAST_TxGetSubCarrier(Blaster_p->Regs_p);
    Reg = ((Reg * LocalDutyCycle) + 50) / 100;

    /* Note that a zero subcarrier width would effectively disable
     * the transmitter.  We therefore treat this as a special case
     * and set the register to one.
     */
    if(Reg == 0)
    {
        Reg = 1;
    }

    /* Commit subcarrier width */
    BLAST_TxSetSubcarrierWidth(Blaster_p->Regs_p, Reg);
}

/*****************************************************************************
BLAST_InitSubCarrier()
Description:
    Programs the IrBlaster subcarrier generator for a given input clock
    and target subcarrier frequency.  Assumes that the given regs structure
    is a valid transmitter device.
Parameters:
    BlastRegs_p, pointer to register map structure.
    Clock,       module clock frequency.
    SubCarrierFreq, required subcarrier frequency.
Return Value:
    Returns actual subcarrier frequency computed.
See Also:
*****************************************************************************/

U32 BLAST_InitSubCarrier(BLAST_BlastRegs_t *BlastRegs_p,
                         U32 Clock,
                         U32 SubCarrierFreq
                        )
{
    U32 SguDiv, SguFreq, PreScaler;

    /* Set up subcarrier based on a signal generator frequency
     * of approx 10MHz.
     */
    PreScaler = ((Clock + 5000000) / 10000000);
    BLAST_TxSetPrescaler(BlastRegs_p, PreScaler);

    /* Compute actual SGU frequency */
    SguFreq = Clock / PreScaler;

    /* Find most accurate divider for required subcarrier.  Note
     * that we round up to the nearest integer.
     *
     * Also note that the method for computing the divider
     * is different depending on the device type:
     *
     * BASIC TRANSMITTER:
     *
     *   DIV = CLOCK/PRESCALER*SUBCARRIER*2
     *
     * TRANSMITTER WITH DUTY CYCLE:
     *
     *   DIV = CLOCK/PRESCALER*SUBCARRIER
     */

#if defined(ST_5518)
    SguDiv = (SguFreq + SubCarrierFreq) / (SubCarrierFreq * 2);
#else
    SguDiv = (SguFreq + (SubCarrierFreq>>1)) / SubCarrierFreq;
#endif

    /* Set the subcarrier */
    BLAST_TxSetSubCarrier(BlastRegs_p, SguDiv);

    /* Return actual subcarrier */
    return (SguFreq / SguDiv);
} /* BLAST_InitSubCarrier() */

/*****************************************************************************
BLAST_DoEncode()
Description:
Parameters:
    Blaster_p               pointer to a blaster control block
    Op                      pointer to an operation
Return Value:
See Also:
    BLAST_SpaceEncode
    BLAST_DoDecode
*****************************************************************************/

ST_ErrorCode_t BLAST_DoEncode(const U32                  *UserBuf_p,
                              const U32                  UserBufSize,
                              STBLAST_Symbol_t           *SymbolBuf_p,
                              const U32                  SymbolBufSize,
                              U32                        *SymbolsEncoded_p,
                              const STBLAST_Protocol_t   Protocol,
                              const STBLAST_ProtocolParams_t *ProtocolParams_p)
{
    ST_ErrorCode_t error = ST_NO_ERROR;

    *SymbolsEncoded_p = 0;

    /* Encode based on current protocol settings */
    switch(Protocol)
    {
        case STBLAST_PROTOCOL_USER_DEFINED:
            if((ProtocolParams_p->UserDefined.Coding == STBLAST_CODING_SPACE)||
                (ProtocolParams_p->UserDefined.Coding==STBLAST_CODING_PULSE))
            {
                BLAST_SpaceEncode(UserBuf_p,
                              UserBufSize,
                              SymbolBuf_p,
                              SymbolBufSize,
                              SymbolsEncoded_p,
                              ProtocolParams_p);
            }
            else
            {
                 BLAST_ShiftEncode(UserBuf_p,
                              UserBufSize,
                              SymbolBuf_p,
                              SymbolBufSize,
                              SymbolsEncoded_p,
                              ProtocolParams_p);
            }
            break;

        case STBLAST_PROTOCOL_RC5:
            BLAST_RC5Encode(UserBuf_p,
            				UserBufSize,
                            SymbolBuf_p,
                            SymbolsEncoded_p);
                                                        
        case STBLAST_PROTOCOL_RCRF8:
            BLAST_RCRF8Encode(UserBuf_p,
                              UserBufSize,
                              SymbolBuf_p,
                              SymbolsEncoded_p,
                              ProtocolParams_p);
            break;

        case STBLAST_LOW_LATENCY_PROTOCOL:
            BLAST_LowLatencyEncode(UserBuf_p,
                                   SymbolBuf_p,
                                   SymbolsEncoded_p,
                                   ProtocolParams_p);
            break;

        case STBLAST_PROTOCOL_RC6A:
            BLAST_RC6AEncode(UserBuf_p,
                             SymbolBuf_p,
                             SymbolsEncoded_p,
                             ProtocolParams_p);
            break;

        case STBLAST_PROTOCOL_RC6_MODE0:
            BLAST_RC6AEncode_Mode0(UserBuf_p,
                                   SymbolBuf_p,
                                   SymbolsEncoded_p,
                                   ProtocolParams_p);
            break;

        case STBLAST_PROTOCOL_RCMM:
            BLAST_RcmmEncode(UserBuf_p,
                             SymbolBuf_p,
                             SymbolsEncoded_p,
                             ProtocolParams_p);

            break;
       case STBLAST_LOW_LATENCY_PRO_PROTOCOL:
            BLAST_LowLatencyProEncode(UserBuf_p,
		                              SymbolBuf_p,
		                              SymbolsEncoded_p,
		                              ProtocolParams_p);  
		   break;		                                        
        default:
            break;
   }

    /* Common exit */
    return error;
}

/*****************************************************************************
BLAST_DoDecode()
Description:
Parameters:
    Blaster_p               pointer to a blaster control block
    Op                      pointer to an operation
Return Value:
See Also:
    BLAST_DoEncode
    BLAST_SpaceDecode
*****************************************************************************/

ST_ErrorCode_t BLAST_DoDecode(BLAST_ControlBlock_t *Blaster_p)
{
    ST_ErrorCode_t error = ST_NO_ERROR;

    STOS_SemaphoreSignal(Blaster_p->ReceiveSem_p);

    /* Common exit */
    return error;
}


/*****************************************************************************
SymbolsOkay
Description:
    Verify that two buffers of symbols are identical (within certain drift
    limits)

Parameters:
    Incoming    the symbols to compare
    Source      the 'reference' symbols to compare them against
    Count       how many symbols to compare

Return Value:
    TRUE        if symbols are basically the same
    FALSE       otherwise

See Also:
    BLAST_SpaceDecode
*****************************************************************************/
BOOL SymbolsOkay(const STBLAST_Symbol_t *Incoming,
                 const STBLAST_Symbol_t *Source,
                 const U32 Count)
{
    BOOL Okay = TRUE;
    U32 AllowedMarkDrift, AllowedSymbolDrift;
    U32 MarkDrift, SymbolDrift;
    U8 j;

    for(j = 0; j < Count && (Okay == TRUE); j++)
    {
        AllowedMarkDrift = Source[j].MarkPeriod / 5;
        AllowedSymbolDrift = Source[j].SymbolPeriod / 5;

        MarkDrift = abs(Source[j].MarkPeriod - Incoming[j].MarkPeriod);
        SymbolDrift = abs(Source[j].SymbolPeriod - Incoming[j].SymbolPeriod);

        if((MarkDrift > AllowedMarkDrift) || (SymbolDrift > AllowedSymbolDrift))
        {
            Okay = FALSE;
        }
    }

    return Okay;
}
