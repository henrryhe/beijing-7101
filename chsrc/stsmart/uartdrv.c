/*****************************************************************************
File Name   : uartdrv.c

Description : UART driver private routines for register access.

Copyright (C) 2006 STMicroelectronics

History     :

Reference   :

ST API Definition "UART Driver API" DVD-API-22
*****************************************************************************/

/* Includes --------------------------------------------------------------- */

#if defined(ST_OS20) || defined(ST_OS21)
#include <string.h>                     /* C libs */
#include <stdio.h>
#include <stdlib.h>
#endif

#include "stpio.h"
#include "stlite.h"
#include "sttbx.h"
#include "stsys.h"
#include "stddefs.h"
#include "stos.h"

#include "stuart.h"                     /* STAPI */

#include "ascregs.h"                    /* Private includes */
#include "timer.h"
#include "uartdat.h"
#include "uartdrv.h"

/* Private types/constants ------------------------------------------------ */

/* Private Variables ------------------------------------------------------ */

#ifdef STUART_NDS_BUSY_WAIT
/* DEBUG global */
U32 UART_BusyWaitCnt = 0;
#endif

#ifdef STUART_PIOCOMPARE_STATS
U32 UART_PioCompareSet = 0;
#endif

#define MAX_RETRIES  12
U32 Resend = 0;
U32 IntCounter = 0;

static char HWfifodata[16]; /*needed to temporarly store fifo data in case of NDS parity error*/
static U32 hwfifob=0;

/* Private Macros --------------------------------------------------------- */

#ifdef MIN                              /* Undefine for this module only */
#undef MIN
#endif

#define MIN(x,y)                    ((x<y)?x:y)

/* Private Functions Prototypes ------------------------------------------- */

static __inline BOOL UART_CheckTermString(UART_ControlBlock_t *Uart_p,
                                          U8 UserChar);
static __inline BOOL UART_CheckSWFlow(UART_ControlBlock_t *Uart_p,
                                      U8 UserChar);

/* Private Functions ------------------------------------------------------ */

/*****************************************************************************
UART_CheckTermString()
Description:
    The routine may then be called for checking a character off from the
    termination string.  If all character in the term string have been
    checked off, then the routine will return TRUE and reset the termination
    buffer pointers back to NULL.

Parameters:
    Uart_p,     pointer to UART control block for checking termination string.
    UserChar,   character to attempt to check off from the term string.

Return Value:
    FALSE,      not all characters have been checked off yet
    TRUE,       all characters have been checked off
See Also:
    UART_Read()
    UART_InterruptHandler()
*****************************************************************************/

static __inline BOOL UART_CheckTermString(UART_ControlBlock_t *Uart_p,
                                          U8 UserChar)
{
    BOOL rc = FALSE;
    ST_String_t WorkingTermString =
               Uart_p->DefaultParams.RxMode.TermString;

    /* Ensure that the termination string pointer is setup */
    if (Uart_p->TermString_p == NULL)
    {
        Uart_p->TermString_p = (U8 *)WorkingTermString;
    }

    if (Uart_p->TermString_p != NULL)
    {
        /* We have a termination string to check, check the
         * user character against the current term char.
         */
        if (UserChar == *Uart_p->TermString_p)
        {
            Uart_p->TermString_p++;

            /* Now check whether this is the end of the term string */
            if (*Uart_p->TermString_p == '\0')
            {
                /* All term string characters have been checked */
                rc = TRUE;
                Uart_p->TermString_p = NULL;
            }
        }
        else
        {
            /* Make sure we only match a contiguous string */
            Uart_p->TermString_p = (U8 *)WorkingTermString;
        }

    }

    return rc;
} /* UART_CheckTermString() */

/*****************************************************************************
UART_CheckSWFlow()
Description:
    Checks if the character it's just been passed is XOFF (stop transmitting)
    or XON (start transmitting).

Parameters:
    Uart_p,     pointer to UART control block for checking termination string.
    UserChar,   character to attempt to check off from the term string.

Return Value:
    TRUE        if a flow control character was encountered
    FALSE       otherwise

See Also:
    UART_Write()
    UART_InterruptHandler()
*****************************************************************************/

static __inline BOOL UART_CheckSWFlow(UART_ControlBlock_t *Uart_p,
                                      U8 UserChar)
{
    /* Are we actually checking for s/w flow control? */
    if (Uart_p->DefaultParams.TxMode.FlowControl == STUART_SW_FLOW_CONTROL)
    {
        /* Check if we got a flow-control character */
        if (UserChar == STUART_XOFF)
        {
            /* We did, and we aren't paused, so let's stop */
            if ((Uart_p->UartStatus & UART_XOFF_TX) == 0)
            {
                Uart_p->IntEnable &= ~Uart_p->IntEnableTxFlags;
                Uart_p->UartStatus |= UART_XOFF_TX;
            }

            return TRUE;

        }
        else if (UserChar == STUART_XON)
        {
            /* Got XON, and we are indeed paused, so unpause */
            if ((Uart_p->UartStatus & UART_XOFF_TX) != 0)
            {
                Uart_p->IntEnable |= Uart_p->IntEnableTxFlags;
                Uart_p->UartStatus &= ~UART_XOFF_TX;
            }
            return TRUE;

        }
        else
        {
            return FALSE;
        }

    }
    else
    {
        return FALSE;
    }

} /* UART_CheckSWFlow() */


/*****************************************************************************
UART_InterruptHandler()
Description:
    This is the interrupt handler for all UARTs on the main board.
    It performs the minimum processing required to clear all interrupt
    conditions on the interrupting UART.

    NOTE: The interrupt handler is installed/removed by STUART_Init() and
    STUART_Term() respectively per UART.

Parameters:
    Uart_p, pointer to the control block associated with the
                        interrupting device.
Return Value:
    Nothing.
See Also:
    STUART_Init()
    STUART_Term()
*****************************************************************************/
#if defined (ST_OSLINUX) && defined(LINUX_FULL_KERNEL_VERSION)
irqreturn_t UART_InterruptHandler(int irq, void* dev_id, struct pt_regs* regs)
#else
#ifdef ST_OS21
int UART_InterruptHandler(UART_ControlBlock_t *Uart_p)
#else
void UART_InterruptHandler(UART_ControlBlock_t *Uart_p)
#endif
#endif /* ST_OSLINUX && LINUX_FULL_KERNEL_VERSION */
{
#if !defined(ARCHITECTURE_ST40) && !defined(PROCESSOR_C1) && !defined(ST_OSLINUX)
#pragma ST_device(ASCRegs_p)
#endif

    UART_ControlBlock_t *Uart_p;
    volatile UART_ASCRegs_t *ASCRegs_p;
    U32 NumberBytes;
    BOOL ErrorSet = FALSE;
    U32 ASCStatus;
    BOOL XOFFNeeded = FALSE;
    U32 DataUnitsToBytes;
    U32 NumberToWrite = 0;
    U32 fifor=0;
	
    UART_Operation_t* WriteOp_p = NULL;
    UART_Operation_t* ReadOp_p  = NULL;

    /* Setup our register pointer */
#ifdef ST_OSLINUX
    Uart_p = (UART_ControlBlock_t* )dev_id;
    IntCounter = (IntCounter+1);
    ASCRegs_p = (volatile UART_ASCRegs_t *)Uart_p->MappingBaseAddress;
#else
    ASCRegs_p = (volatile UART_ASCRegs_t *)Uart_p->InitParams.BaseAddress;
#endif

    WriteOp_p = Uart_p->WriteOperation;
    ReadOp_p  = Uart_p->ReadOperation;

    /* Disable all interrupts whilst in the interrupt handler */
    UART_GlobalDisableInts(ASCRegs_p);

    /* Take a copy of the status register */
#ifdef ST_OSLINUX
    ASCStatus = UART_Status(ASCRegs_p) & (Uart_p->IntEnable | STATUS_NACKED);
#else
    ASCStatus = ASCRegs_p->ASCnStatus & (Uart_p->IntEnable | STATUS_NACKED);
#endif

    /* We must re-enable reception in smartcard mode */
    if (Uart_p->DefaultParams.SmartCardModeEnabled)
        UART_ReceiverEnable(ASCRegs_p);

    /* Check for nacked error (if in smartcard mode) */
    if (UART_IsNACKSupported(Uart_p->InitParams.UARTType) &&
        (ASCStatus & STATUS_NACKED) != 0)
    {
        /* Check for a write operation pending */
        if (Uart_p->DefaultParams.SmartCardModeEnabled &&
            WriteOp_p != NULL &&
            WriteOp_p->Status == STUART_ERROR_PENDING)
        {
            /* We must notify this nacked error to the caller */
            WriteOp_p->Status = STUART_ERROR_RETRIES;
            stuart_TimerSignal(WriteOp_p->Timer, FALSE);

            /* Reset the TXFIFO to clear the nack error */
            UART_ResetTxFIFO(ASCRegs_p);
        }
    }

    /* Check for a overrun error */
    if ((ASCStatus & STATUS_OVERRUNERROR) != 0)
    {
        /* Check for a read operation pending */
        if (ReadOp_p != NULL &&
            ReadOp_p->Status == STUART_ERROR_PENDING)
        {
            /* We must notify this overrun error to the caller */
            ReadOp_p->Status = STUART_ERROR_OVERRUN;
            stuart_TimerSignal(ReadOp_p->Timer, FALSE);
        }
        else
        {
            /* There is no pending read, so we should flag the error
             * ready for the next operation - this user
             * will be notified of the error allowing them to
             * take corrective action.
             */
            Uart_p->ReceiveError = STUART_ERROR_OVERRUN;
        }

        ErrorSet = TRUE;
        Resend = 0;

#if STUART_STATISTICS
        /* Increment overrun statistics */
        Uart_p->Statistics.NumberOverrunErrors++;
#endif
    }


    if ((ASCStatus & STATUS_PARITYERROR) != 0)
    {
    	/* In NDS Mode,if parity error occurs,the offending byte is resend
         * till MAX_RETRIES times - DDTS 28984
         */
        if (UART_DoesNdsFlowControlApply(Uart_p))     
        {
 		/*here we must empty the HW fifo except the last byte, that is dropped*/
		hwfifob=0;
	  	while ((ASCRegs_p->ASCnStatus & STATUS_PARITYERROR) != 0)
              	{
			HWfifodata[hwfifob++] = ASCRegs_p->ASCnRxBuffer;
                	ASCStatus = ASCRegs_p->ASCnStatus;
		}
		/*ditch the last byte*/
		hwfifob--;
              	Resend++;
		if (Resend>=MAX_RETRIES)
		{
			if (ReadOp_p != NULL)
	             	{
		      		ReadOp_p->Status = STUART_ERROR_PARITY;
			      	stuart_TimerSignal(ReadOp_p->Timer, FALSE);
 		            	ErrorSet = TRUE;   
		  	}
            		else
            		{
			     	/* There is no pending read, so we should flag the error
				* ready for the next operation - this user
				* will be notified of the error allowing them to
				* take corrective action.
				*/
				Uart_p->ReceiveError = STUART_ERROR_PARITY;
				ErrorSet = TRUE;
			}
	 		Resend = 0;
	 	}
	 }
	 else
	 {
	 	/* Check for a read operation pending */
	 	if (ReadOp_p != NULL &&
                   ReadOp_p->Status == STUART_ERROR_PENDING)
        	{
            		/* We must notify this parity error to the caller */
            		ReadOp_p->Status = STUART_ERROR_PARITY;
            		stuart_TimerSignal(ReadOp_p->Timer, FALSE);
            		ErrorSet = TRUE;
        	}
        	else
        	{
            		/* There is no pending read, so we should flag the error
             		* ready for the next operation - this user
             		* will be notified of the error allowing them to
             		* take corrective action.
             		*/
            		Uart_p->ReceiveError = STUART_ERROR_PARITY;
            		ErrorSet = TRUE;
        	}
	 }
        


#if STUART_STATISTICS
        /* Increment parity error statistics */
        Uart_p->Statistics.NumberParityErrors++;
#endif
    }

    /* Check for a frame error */
    if ((ASCStatus & STATUS_FRAMEERROR) != 0)
    {
        ErrorSet = TRUE;
        Resend = 0;

        /* Check for NACK in smartcard mode */
        if (!UART_IsNACKSupported(Uart_p->InitParams.UARTType) &&
            Uart_p->DefaultParams.SmartCardModeEnabled &&
            WriteOp_p != NULL &&
            WriteOp_p->Status == STUART_ERROR_PENDING)
        {
            U8 RxBuf;

            /* Check retries counter */
            if (Uart_p->Retries > 0)
            {
                /* Try resending the previous byte */
                WriteOp_p->UserBuffer--;
                WriteOp_p->NumberRemaining++;
                (*WriteOp_p->NumberBytes)--;
                Uart_p->Retries--;           /* Decrement retries counter */
                ASCStatus |= STATUS_TXEMPTY; /* Force re-transmit now */
            }
            else
            {
                /* All retries have been used - cancel the write operation */
                WriteOp_p->Status = STUART_ERROR_RETRIES;
                stuart_TimerSignal(WriteOp_p->Timer, FALSE);
            }

            /* Read RXFIFO to clear error */
#ifdef ST_OSLINUX
            RxBuf = UART_ReadRxBuf(ASCRegs_p);
#else
            RxBuf = ASCRegs_p->ASCnRxBuffer;
#endif
            ASCStatus &= ~(U32)(STATUS_RXBUFFULL|STATUS_RXHALFFULL);
            ErrorSet = FALSE;           /* Error should be cleared now */
        }   /* Check for a read operation pending */
        else if (ReadOp_p != NULL &&
                 ReadOp_p->Status == STUART_ERROR_PENDING)
        {
            /* We must notify this frame error to the caller */
            ReadOp_p->Status = STUART_ERROR_FRAMING;
            stuart_TimerSignal(ReadOp_p->Timer, FALSE);
        }
        else
        {
            /* There is no pending read, so we should flag the error
             * ready for the next operation - this user
             * will be notified of the error allowing them to
             * take corrective action.
             */
            Uart_p->ReceiveError = STUART_ERROR_FRAMING;
        }

#if STUART_STATISTICS
        /* Increment framing error statistics */
        Uart_p->Statistics.NumberFramingErrors++;
#endif
    }

    /* Clear any error conditions that may have arisen */
    if (ErrorSet)
    {
        if ( (UART_DoesNdsFlowControlApply(Uart_p)) && (Resend > 0))
        {
            /* Enable error interrupt */
            Uart_p->IntEnable |= (U32)UART_IE_ENABLE_ERRORS;

        }
        else
        {

            /* Disable error interrupts - until the error has been cleared
             * by the user.
             */
            Uart_p->IntEnable &= ~(U32)UART_IE_ENABLE_ERRORS;
        }
    }
    else
    {
        Resend = 0;
    }


    /* In 9-bit data mode we treat each data unit as two
     * consecutive 8-bit bytes, where the 9th data bit
     * is the least significant bit of the most significant
     * byte.
     */
    if (UART_Is9BitMode(Uart_p->DefaultParams.RxMode.DataBits))
    {
        DataUnitsToBytes = 2; /* 1du = 2bytes */
    }
    else
    {
        DataUnitsToBytes = 1; /* 1du = 1byte */
    }



    /* Check for a receive buffer half-full */
    if ((ASCStatus & (STATUS_RXBUFFULL
                   |  STATUS_RXHALFFULL
                   |  STATUS_TIMEOUTNOTEMPTY))
                   != 0)

    {

        U32 j;
        BOOL ReadPending = FALSE;
        U16 RxData;

        /* Reset smartcard retries count */
        Uart_p->Retries = Uart_p->DefaultParams.Retries;


        /* Check for a read operation pending, otherwise we must copy
         * the byte to our software FIFO (if there is one).
         */
        if (ReadOp_p != NULL &&
            ReadOp_p->Status == STUART_ERROR_PENDING)
        {

            /* A read operation is pending */
            ReadPending = TRUE;

            /* Only continue if not paused (otherwise let data sit in HW FIFO
               and potentially overflow) */
            if ((Uart_p->UartStatus & UART_PAUSE_RX) == 0)
            {
            	/* Re-trigger the timeout as we've just received some
                 * more data.
                 */
                stuart_TimerSignal(ReadOp_p->Timer, TRUE);

                /* Pass the new bytes to the read operation buffer */
		
#ifdef ST_OSLINUX
               	while (ReadPending && (((UART_Status(ASCRegs_p) & STATUS_RXBUFFULL) != 0 )
					|| (UART_DoesNdsFlowControlApply(Uart_p) && hwfifob!=fifor)))
#else
                
		while (ReadPending && (((ASCRegs_p->ASCnStatus & STATUS_RXBUFFULL) != 0)
			    		|| (UART_DoesNdsFlowControlApply(Uart_p) && hwfifob!=fifor)))
#endif
               	{
               	    /* Read the available data unit */
		    if (hwfifob!=fifor)
		    {
		    	RxData=HWfifodata[fifor++];
		    }
		    else 
		    {
			/* Read the available data unit */
#ifdef ST_OSLINUX
		     	RxData = UART_ReadRxBuf(ASCRegs_p);
#else
			RxData = ASCRegs_p->ASCnRxBuffer;
#endif	
 		    }

		    Uart_p->IntEnable |= UART_IE_ENABLE_ERRORS;
			

                    /* Check if we got a flow-control character */
                    if (UART_CheckSWFlow(Uart_p, (U8)RxData))
                    {
#if STUART_STATISTICS
                        /* Increment bytes received statistics */
                        Uart_p->Statistics.NumberBytesReceived++;
#endif
                        continue;
                    }

                    for (j = 0;  j < DataUnitsToBytes; j++, RxData >>= 8)
                    {
                        /* Copy the byte from RX buffer */
                        *ReadOp_p->UserBuffer++ = RxData;

                        /* Update character count */
                        ReadOp_p->NumberRemaining--;
                        (*ReadOp_p->NumberBytes)++;

#if STUART_STATISTICS
                        /* Increment bytes received statistics */
                        Uart_p->Statistics.NumberBytesReceived++;

#endif
                        /* Check for bytes remaining (or terminated string) */
                        if (ReadOp_p->NumberRemaining == 0 ||
                            UART_CheckTermString(Uart_p,
                            *(ReadOp_p->UserBuffer-1)))
                        {
                            /* Complete this read operation */
                            ReadOp_p->Status = ST_NO_ERROR;
                            stuart_TimerSignal(ReadOp_p->Timer, FALSE);
                            ReadPending = FALSE;
                        }
                    }
                }
            }
        }

        /* There may still be data to read into the SW FIFOs */
        if (Uart_p->InitParams.SwFIFOEnable && !ReadPending)
        {
            /* Copy the bytes in to the FIFO and update the
             * buffer free count.
             */
            
#ifdef ST_OSLINUX
            while (Uart_p->ReceiveBufferCharsFree >= DataUnitsToBytes && 
               		(((UART_Status(ASCRegs_p) & STATUS_RXBUFFULL) != 0) || 
                     	(UART_DoesNdsFlowControlApply(Uart_p) && hwfifob!=fifor)))
#else
            while (Uart_p->ReceiveBufferCharsFree >= DataUnitsToBytes && 
               	   	(((ASCRegs_p->ASCnStatus & STATUS_RXBUFFULL) != 0) ||
               	   	(UART_DoesNdsFlowControlApply(Uart_p) && hwfifob!=fifor)))
#endif
            {
            	if (hwfifob!=fifor) 
		    RxData=HWfifodata[fifor++];
		else
		{
#ifdef ST_OSLINUX
                    RxData = UART_ReadRxBuf(ASCRegs_p);
#else
                    RxData = ASCRegs_p->ASCnRxBuffer;
#endif
		}
                Uart_p->IntEnable |= UART_IE_ENABLE_ERRORS;

                /* Check if we got a flow-control character */
                if (UART_CheckSWFlow(Uart_p, (U8)RxData))
                {
#if STUART_STATISTICS
                    /* Increment bytes received statistics */
                    Uart_p->Statistics.NumberBytesReceived++;
#endif
                    continue;
                }

                for (j = 0; j < DataUnitsToBytes; j++, RxData >>= 8)
                {
                    /* Copy the byte from RX buffer */
                    *Uart_p->ReceivePut_p++ = RxData;
                    Uart_p->ReceiveBufferCharsFree--;

                    /* Check for put pointer buffer-wrap */
                    if (Uart_p->ReceivePut_p >=
                       (Uart_p->ReceiveBufferBase +
                        Uart_p->InitParams.FIFOLength))
                    {
                        Uart_p->ReceivePut_p = Uart_p->ReceiveBufferBase;
                    }
#if STUART_STATISTICS

                    Uart_p->Statistics.NumberBytesReceived++;
#endif
                }

                /* Are we (a) using s/w flow, and (b) at risk of
                 * overflowing?
                 */
                if ((Uart_p->DefaultParams.RxMode.FlowControl ==
                     STUART_SW_FLOW_CONTROL) &&
                    (Uart_p->ReceiveBufferCharsFree <
                     Uart_p->XOFFThreshold) &&
                    (Uart_p->UartStatus & UART_XOFF_RX) == 0)
                {
                    /* Yes, so let's send them XOFF, and hope they stop */
                    XOFFNeeded = TRUE;
                }
            }
#ifdef ST_OSLINUX
            if (Uart_p->ReceiveBufferCharsFree < DataUnitsToBytes && (UART_Status(ASCRegs_p) & STATUS_RXBUFFULL) != 0)
#else
            if (Uart_p->ReceiveBufferCharsFree < DataUnitsToBytes && (ASCRegs_p->ASCnStatus & STATUS_RXBUFFULL) != 0)
#endif
            {
                /* We have data to read - we must disable interrupts
                 * until space becomes available to read the data.
                 */
                Uart_p->IntEnable &= ~Uart_p->IntEnableRxFlags;
            }
        }
#ifdef ST_OSLINUX
        else if ((UART_Status(ASCRegs_p) & STATUS_RXBUFFULL) != 0 && !ReadPending)
#else
        else if ((ASCRegs_p->ASCnStatus & STATUS_RXBUFFULL) != 0 && !ReadPending)
#endif
        {
            /* We have data to read - we must disable interrupts
             * until space becomes available to read the data.
             */
            Uart_p->IntEnable &= ~Uart_p->IntEnableRxFlags;
        }

    }

    /* Check whether or not we need to send an XOFF */
    if (XOFFNeeded)
    {
        static U8 XOFFChar = STUART_XOFF;

        /* If we succeed, set the status flag, and clear
         * the need to send flag.
         */
#ifdef ST_OSLINUX
        if ((UART_Status(ASCRegs_p) & STATUS_TXFULL)==0)
#else
        if ((ASCRegs_p->ASCnStatus & STATUS_TXFULL)==0)
#endif
        {
            if (UART_WriteBlock(Uart_p, &XOFFChar, 1) == 1)
            {
                Uart_p->UartStatus |= UART_XOFF_RX;
            }
        }
    }
	
    if (UART_DoesNdsFlowControlApply(Uart_p))
    {
	hwfifob=fifor=0; //this touch a global variable but we have only one smartcard instance
    }

    /* Check for a transmit empty or transmit half empty */
    if ( ( (ASCStatus & STATUS_TXEMPTY) != 0 ||
        (ASCStatus & STATUS_TXHALFEMPTY) != 0 ) &&
        (Uart_p->WriteAwaitingPioCompare == FALSE) )
    {

        /* Check if there is a write operation that can feed more data
         * to the UART.
         */
        if (WriteOp_p != NULL &&
            WriteOp_p->Status == STUART_ERROR_PENDING &&
            WriteOp_p->NumberRemaining > 0 &&
            (Uart_p->UartStatus & UART_PAUSE_TX) == 0 &&
            (Uart_p->UartStatus & UART_XOFF_TX) == 0)
        {

            /* Update byte counts */
            WriteOp_p->UserBuffer +=
                WriteOp_p->LastTransferSize;
            WriteOp_p->NumberRemaining -=
                WriteOp_p->LastTransferSize;
            *WriteOp_p->NumberBytes +=
                WriteOp_p->LastTransferSize;


            if (WriteOp_p->NumberRemaining == 0)
            {

                /* All bytes have been written */
#ifdef ST_OSLINUX
                if ((UART_Status(ASCRegs_p) & STATUS_TXEMPTY) != 0)
#else
                if ((ASCRegs_p->ASCnStatus & STATUS_TXEMPTY) != 0)
#endif
                {
                    /* really complete and left transmit shift register (but has
                     * the time for final byte NACK passed in Smartcard mode?) 
                     */
                    WriteOp_p->Status = ST_NO_ERROR;
                    stuart_TimerSignal(WriteOp_p->Timer, FALSE);
                    Uart_p->IntEnable &= ~Uart_p->IntEnableTxFlags;
		    if (UART_DoesNdsFlowControlApply(Uart_p))
		    {
			/* reenable the receiver and the FIFO */
			UART_FifoEnable(ASCRegs_p);
			UART_ReceiverEnable(ASCRegs_p);
		    }
                }
                else
                {
                    /* wait for TXEMPTY; should have done this already when
                      final bytes were written. Reduce counts by at least one
                      byte because that's reality and it ensures we pass the
                      above 'if' next time */
                    WriteOp_p->LastTransferSize = 1;
                    WriteOp_p->UserBuffer -=
                        WriteOp_p->LastTransferSize;
                    WriteOp_p->NumberRemaining +=
                        WriteOp_p->LastTransferSize;
                    *WriteOp_p->NumberBytes -=
                        WriteOp_p->LastTransferSize;
                    Uart_p->IntEnable &= ~IE_TXHALFEMPTY;
                }

            }
            else
            {

                if (DataUnitsToBytes > 1)
                {
                    NumberToWrite = Uart_p->HwFIFOSize;
                }
                else
                {
                    NumberToWrite = WriteOp_p->NumberRemaining;
                }
                /* PIO state check :If the data line is found to be low, the transfer
                 * is halted until a PIO compare interrupt fires to
                 * indicate that the receiver has released the line.
                 */

                if (UART_DoesNdsFlowControlApply(Uart_p))
                {
                    ST_ErrorCode_t error=ST_NO_ERROR;
                    U8 Pin;

#ifdef STUART_NDS_BUSY_WAIT /* early debug version */
                    /* 5514 registers: 0x01 is TXD, 0x02 is RXD */
                    while(((*(U32*)(0x2010C000+0x10)) & 0x01) == 0)
                    {
                        UART_BusyWaitCnt++;
                    }
#else

                    /* Use TXD bit as it's setup bidirectional for
                     * smartcards in STUART_Open and RXD settings not
                     * used if TXD pin == RXD pin. TXDHandle covers just
                     * the one bit, so we don't need to look up which it
                     * is.
                     */
                    error = STPIO_Read(Uart_p->TXDHandle, &Pin);
                    if ((error == ST_NO_ERROR) && (Pin == 0))
                    {
                        STPIO_Compare_t cmp = { 0xff, 0 };
                        /* Can't write now - instead set up PIO compare
                         * to fire when the line goes high again. Turn
                         * off TXEMPTY interrupts until then.
                         */
                        Uart_p->IntEnable &= ~Uart_p->IntEnableTxFlags;

                        Uart_p->WriteAwaitingPioCompare = TRUE;

                        STPIO_SetCompare(Uart_p->TXDHandle, &cmp);

#ifdef STUART_PIOCOMPARE_STATS
                        UART_PioCompareSet++;
#endif
                        NumberToWrite = 0;
                    }
#endif
                }

                if (!Uart_p->WriteAwaitingPioCompare)
                {
                    if (DataUnitsToBytes > 1)
                    {
                        if ((ASCStatus & STATUS_TXHALFEMPTY) != 0 && NumberToWrite > 1)
                        {
                            NumberToWrite >>= 1;    /* Half empty transmit */
                        }


                        NumberBytes = UART_Write9BitBlock(
                                      Uart_p,
                                      WriteOp_p->UserBuffer,
                                      MIN((NumberToWrite << 1),  /* mult 2 */
                                      WriteOp_p->NumberRemaining)
                                                         );


                    }
                    else
                    {
                        NumberBytes = UART_WriteBlock(
                                      Uart_p,
                                      WriteOp_p->UserBuffer,
                                      WriteOp_p->NumberRemaining
                                                     );
                    }

                    /* Setup the number of bytes just sent.  We don't
                     * decrement it until the next interrupt.
                     */
                    WriteOp_p->LastTransferSize = NumberBytes;

                    if (NumberBytes == WriteOp_p->NumberRemaining)
                    {
                        /* We just wrote the last bytes; require full TXEMPTY
                         * before accepting write as complete.
                         */
                        Uart_p->IntEnable &= ~Uart_p->IntEnableTxFlags;
                        Uart_p->IntEnable |= UART_IE_ENABLE_TX_END;
                    }

                    /* Re-trigger the timeout since we've just sent out
                     * a new block of data.
                     */
                    stuart_TimerSignal(WriteOp_p->Timer, TRUE);
                }
            }

        }
        else
        {

            Uart_p->IntEnable &= ~Uart_p->IntEnableTxFlags;
	    if (UART_DoesNdsFlowControlApply(Uart_p))
	    {
		UART_FifoEnable(ASCRegs_p);
	    }
            UART_ReceiverEnable(ASCRegs_p); /* WHY ? */
            UART_SetProtocol(Uart_p, &Uart_p->DefaultParams.RxMode);
        }
    }

    /* Check whether or not we need to send a stuffing byte */
    if (Uart_p->StuffingEnabled)
    {
#ifdef ST_OSLINUX
        if  ((UART_Status(ASCRegs_p) & STATUS_TXEMPTY) != 0)
#else
        if  ((ASCRegs_p->ASCnStatus & STATUS_TXEMPTY) != 0)
#endif
        {
#ifdef ST_OSLINUX
            while ((UART_Status(ASCRegs_p) & STATUS_TXFULL)==0)
#else
            while ((ASCRegs_p->ASCnStatus & STATUS_TXFULL)==0)
#endif
            {
#ifdef ST_OSLINUX
                UART_WriteTxBuf(ASCRegs_p, Uart_p->StuffingValue);
#else
                ASCRegs_p->ASCnTxBuffer = Uart_p->StuffingValue;
#endif
            }
            Uart_p->IntEnable &= ~Uart_p->IntEnableTxFlags;
            Uart_p->IntEnable |= UART_IE_ENABLE_TX_END;
        }
    }
    /* Restore interrupt enable register */
    UART_EnableInts(ASCRegs_p,Uart_p->IntEnable);

#if defined (ST_OSLINUX) && defined(LINUX_FULL_KERNEL_VERSION)
    return IRQ_HANDLED;
#else
#ifdef ST_OS21
    return(STOS_SUCCESS);
#else
    return;
#endif
#endif

} /* UART_InterruptHandler() */

/*****************************************************************************
UART_BaudRate()
Description:
    This routine has the dual purpose of verifying the passed baud rate is
    supported by this device and if it is also sets the baud rate register
    for the requested UART.

Parameters:
    Uart_p,     pointer the the UART control block to set the baud rate for.
    BaudRate,   the baud rate.
    Set,        If set TRUE then the baud rate register will be updated.
Return Value:
    FALSE,      the baud rate was set successfully.
    TRUE,       the baud rate is invalid.
See Also:
    Nothing.
*****************************************************************************/

BOOL UART_BaudRate(UART_ControlBlock_t *Uart_p,
                   U32 BaudRate,
                   BOOL Set)
{

#if !defined(ARCHITECTURE_ST40) && !defined(PROCESSOR_C1) && !defined(ST_OSLINUX)
#pragma ST_device(ASCRegs_p)
#endif

    /* Ensure we have a valid baudrate */
    if (BaudRate < STUART_BAUDRATE_MIN || BaudRate > STUART_BAUDRATE_MAX)
    {
        return TRUE;                    /* Invalid baudrate */
    }

    /* If the Set flag is TRUE then set the baudrate according
     * to the reload value.  Note we don't reset the reload counter
     * if the request baudrate is the same as the current baud
     * rate.
     */
    if (Set && Uart_p->LastProtocol.BaudRate != BaudRate)
    {
        U8 ClockMode = 0;               /* Default clock mode */
        U16 ReloadValue[2];
        S32 BaudRateError[2];

#ifdef ST_OSLINUX
        volatile UART_ASCRegs_t *ASCRegs_p = (volatile UART_ASCRegs_t *)Uart_p->MappingBaseAddress;
#else
        volatile UART_ASCRegs_t *ASCRegs_p = (volatile UART_ASCRegs_t *)Uart_p->InitParams.BaseAddress;
#endif

        /* Compute reload value for 'decrement-trigger-zero' baud mode */
        ReloadValue[0] = (U16)((Uart_p->InitParams.ClockFrequency +
                               ((16 * BaudRate) / 2)) / (16 * BaudRate));
        BaudRateError[0] = (S32)(BaudRate -
                                (Uart_p->InitParams.ClockFrequency /
                                (16 * ReloadValue[0])));

        /* Compute reload value for 'multiply-carry-trigger' baud mode.
           The BR calculation has been revised for higher BR support- DDTS 25068 */

        ReloadValue[1] = (U16)(((BaudRate * (2 << 9)) +
                               (Uart_p->InitParams.ClockFrequency / (2 << 10))) /
                               (Uart_p->InitParams.ClockFrequency / (2 << 9)));
        BaudRateError[1] = (S32)(BaudRate - (ReloadValue[1] *
                               (Uart_p->InitParams.ClockFrequency / (2 << 9)) /
                               (2 << 9)));

        /* Compute optimum clock mode */
        if (UART_IsBaudModeSupported(Uart_p->InitParams.UARTType))
        {
            /* Select clock mode */
            if (abs(BaudRateError[0]) <= abs(BaudRateError[1]))
            {
                ClockMode = 0;          /* decrement-trigger-zero */
            }
            else
            {
                ClockMode = 1;          /* multiply-carry-trigger */
            }
        }

        /* Set the required clock mode */
        switch (ClockMode)
        {
            case 0:
                UART_BaudTickOnZero(ASCRegs_p);
                break;
            case 1:
                UART_BaudTickOnCarry(ASCRegs_p);
                break;
            default:
                break;
        }

        /* Invoke clock reload value */
#ifdef ST_OSLINUX
        UART_WriteBaudRate(ASCRegs_p, ReloadValue[ClockMode]);
#else
        ASCRegs_p->ASCnBaudRate = ReloadValue[ClockMode];
#endif
        Uart_p->LastProtocol.BaudRate = BaudRate;
        Uart_p->ActualBaudRate = BaudRate - BaudRateError[ClockMode];

    }

    /* No problems */
    return FALSE;
} /* UART_BaudRate() */

/*****************************************************************************
UART_GetDataFromSwFIFO()
Description:
    This routine safely copies bytes from the software FIFO of the
    given UART - as many bytes as available, up to the number of bytes
    requested. If we empty the SW FIFO without fulfilling the request,
    the operation is passed to the ISR

Parameters:
    Uart_p,         pointer the the UART control block to read data from.
    Buffer,         pointer to buffer where to put any data read from the
                    FIFO.
    NumberToRead,   number of bytes requested.
Return Value:
    The number of bytes copied.
See Also:
    STUART_Read()
*****************************************************************************/

U32 UART_GetDataFromSwFIFO(UART_ControlBlock_t *Uart_p,
                                    U8 *Buffer,
                                    U32 MinToRead,
                                    U32 MaxToRead,
                                    BOOL *Terminated,
                                    UART_Operation_t *ReadOperation)
{

#if !defined(ARCHITECTURE_ST40) && !defined(PROCESSOR_C1) && !defined(ST_OSLINUX)
#pragma ST_device(ASCRegs_p)
#endif
    volatile UART_ASCRegs_t *ASCRegs_p;

    U32 NumberRead = 0;
    U32 NumberAvailable = Uart_p->InitParams.FIFOLength
                             - Uart_p->ReceiveBufferCharsFree;

    /* Assume no terms string yet */
    *Terminated = FALSE;

#ifdef ST_OSLINUX
    ASCRegs_p = (volatile UART_ASCRegs_t *)Uart_p->MappingBaseAddress;
#else
    ASCRegs_p = (volatile UART_ASCRegs_t *)Uart_p->InitParams.BaseAddress;
#endif

    /* Copy bytes to the destination buffer */
    while (NumberRead < MaxToRead)
    {
        /* check whether or not input data is exhausted */
        if (NumberAvailable == 0)
        {
            if (NumberRead >= MinToRead)
            {
                break; /* got sufficient to not queue */
            }

            STOS_InterruptLock();

            NumberAvailable = Uart_p->InitParams.FIFOLength - NumberRead
                              - Uart_p->ReceiveBufferCharsFree;

            if (NumberAvailable == 0)
            {
                /* SW FIFO exhausted - queue ReadOperation */
                ReadOperation->UserBuffer += NumberRead;
                ReadOperation->NumberRemaining = MinToRead - NumberRead;
                *(ReadOperation->NumberBytes) = NumberRead;

                Uart_p->ReadOperation = ReadOperation;
                STOS_InterruptUnlock();
                break;
            }

            /* otherwise, more data was added whilst we were reading the first lot */
            STOS_InterruptUnlock();
        }
        else
        {
            /* Read next available character */
            *Buffer++ = *Uart_p->ReceiveGet_p++;

            /* Check for a read pointer buffer-wrap */
            if (Uart_p->ReceiveGet_p >=
                (Uart_p->ReceiveBufferBase + Uart_p->InitParams.FIFOLength))
            {
                Uart_p->ReceiveGet_p = Uart_p->ReceiveBufferBase;
            }

            /* Update count of characters */
            NumberRead++;
            NumberAvailable--;

            /* Check whether or not we have received the termination string */
            if (UART_CheckTermString(Uart_p, *(Buffer-1)))
            {
                *Terminated = TRUE;
                break;
            }
        }
    }

    /* write byte count UNLESS we've already passed read op to interrupt handler
     * (ReadOperation won't have been cleared if we did because timer isn't
     * installed yet in STUART_Read)
     */

    if (Uart_p->ReadOperation == NULL)
    {
        *(ReadOperation->NumberBytes) = NumberRead;
    }

    STOS_InterruptLock(); /* ReceiveBufferCharsFree increment must be atomic */

    /* ISR relies on ReceiveBufferCharsFree to determine if it can write */
    Uart_p->ReceiveBufferCharsFree += NumberRead;

    STOS_InterruptUnlock();

    /* Check if we need to send XON to receive more data */
    /* JF: shouldn't the test be >= XONThreshold? */
    if ((Uart_p->DefaultParams.RxMode.FlowControl == STUART_SW_FLOW_CONTROL) &&
         Uart_p->InitParams.FIFOLength - Uart_p->ReceiveBufferCharsFree
           <= Uart_p->XONThreshold &&
        (Uart_p->UartStatus & UART_XOFF_RX) != 0)
    {
        U8 XONChar = STUART_XON;

#ifdef ST_OSLINUX
        if ((UART_Status(ASCRegs_p) & STATUS_TXFULL)==0)
#else
        if ((ASCRegs_p->ASCnStatus & STATUS_TXFULL)==0)
#endif
        {
            /* We do, so let's send it (safe with interrupts enabled?) */
            if (UART_WriteBlock(Uart_p, &XONChar, 1) == 1)
            {
                Uart_p->UartStatus &= ~UART_XOFF_RX;
            }

        }
    }

    /* Common exit point */
    return NumberRead;
} /* UART_GetDataFromSwFIFO() */


/*****************************************************************************
UART_WriteBlock()
Description:
    This routine transmits a block of data through the UART's ASCnTxBuffer
    register until either the UART's FIFO is full or all required bytes have
    been written.
Parameters:
    Uart_p,         pointer the the UART control block to set the baud rate
                    for.
    Buffer,         pointer to buffer containing data to transmit.
    NumberToWrite,   number of bytes to write.
Return Value:
    The number of bytes written.
See Also:
    STUART_Write()
*****************************************************************************/

U32 UART_WriteBlock(UART_ControlBlock_t *Uart_p,
                             U8 *Buffer,
                             U32 NumberToWrite
                             )
{

#if !defined(ARCHITECTURE_ST40) && !defined(PROCESSOR_C1) && !defined(ST_OSLINUX)
#pragma ST_device(ASCRegs_p)
#endif

#ifdef ST_OSLINUX
    volatile UART_ASCRegs_t *ASCRegs_p = (UART_ASCRegs_t *)Uart_p->MappingBaseAddress;
#else
    volatile UART_ASCRegs_t *ASCRegs_p = (UART_ASCRegs_t *)Uart_p->InitParams.BaseAddress;
#endif

    U32 NumberWritten = 0;


    /* Write out a byte at a time to ASCnTxBuffer until all bytes
     * have gone or the transmit FIFO is full.
     */

    if (Uart_p->HwFIFOSize > 1)
    {
        do
        {
            if (NumberToWrite >= 8 )
            {

#ifdef ST_OSLINUX
                UART_WriteTxBuf(ASCRegs_p, Buffer[0]);
                UART_WriteTxBuf(ASCRegs_p, Buffer[1]);
                UART_WriteTxBuf(ASCRegs_p, Buffer[2]);
                UART_WriteTxBuf(ASCRegs_p, Buffer[3]);
                UART_WriteTxBuf(ASCRegs_p, Buffer[4]);
                UART_WriteTxBuf(ASCRegs_p, Buffer[5]);
                UART_WriteTxBuf(ASCRegs_p, Buffer[6]);
                UART_WriteTxBuf(ASCRegs_p, Buffer[7]);
#else
                ASCRegs_p->ASCnTxBuffer = Buffer[0];
                ASCRegs_p->ASCnTxBuffer = Buffer[1];
                ASCRegs_p->ASCnTxBuffer = Buffer[2];
                ASCRegs_p->ASCnTxBuffer = Buffer[3];
                ASCRegs_p->ASCnTxBuffer = Buffer[4];
                ASCRegs_p->ASCnTxBuffer = Buffer[5];
                ASCRegs_p->ASCnTxBuffer = Buffer[6];
                ASCRegs_p->ASCnTxBuffer = Buffer[7];
#endif
                Buffer += 8;
                NumberWritten+=8;
                NumberToWrite-=8;
            }
            else
            {
                Buffer -=(7 - NumberToWrite);
                switch(NumberToWrite)
                {
                    case 7:
#ifdef ST_OSLINUX
                        UART_WriteTxBuf(ASCRegs_p, Buffer[0]);
#else
                        ASCRegs_p->ASCnTxBuffer = Buffer[0];
#endif
                        /* No break */

                    case 6:
#ifdef ST_OSLINUX
                        UART_WriteTxBuf(ASCRegs_p, Buffer[1]);
#else
                        ASCRegs_p->ASCnTxBuffer = Buffer[1];
#endif
                        /* No break */

                    case 5:
#ifdef ST_OSLINUX
                        UART_WriteTxBuf(ASCRegs_p, Buffer[2]);
#else
                        ASCRegs_p->ASCnTxBuffer = Buffer[2];
#endif
                        /* No break */

                    case 4:
#ifdef ST_OSLINUX
                        UART_WriteTxBuf(ASCRegs_p, Buffer[3]);
#else
                        ASCRegs_p->ASCnTxBuffer = Buffer[3];
#endif
                        /* No break */

                    case 3:
#ifdef ST_OSLINUX
                        UART_WriteTxBuf(ASCRegs_p, Buffer[4]);
#else
                        ASCRegs_p->ASCnTxBuffer = Buffer[4];
#endif
                        /* No break */

                    case 2:
#ifdef ST_OSLINUX
                        UART_WriteTxBuf(ASCRegs_p, Buffer[5]);
#else
                        ASCRegs_p->ASCnTxBuffer = Buffer[5];
#endif
                        /* No break */

                    case 1:
#ifdef ST_OSLINUX
                        UART_WriteTxBuf(ASCRegs_p, Buffer[6]);
#else
                        ASCRegs_p->ASCnTxBuffer = Buffer[6];
#endif
                }

                NumberWritten += NumberToWrite;
                break;
            }

        }
#ifdef ST_OSLINUX
        while ((UART_Status(ASCRegs_p) & STATUS_TXHALFEMPTY) !=0);
#else
        while ((ASCRegs_p->ASCnStatus & STATUS_TXHALFEMPTY) !=0);
#endif

    }
    else
    {
	if (UART_DoesNdsFlowControlApply(Uart_p))
    	{
        	/*force to write only one byte to allows the NDS flowcontrol to work*/
		STOS_InterruptLock();
   		UART_FifoDisable(ASCRegs_p);
#ifdef ST_OSLINUX
            	UART_WriteTxBuf(ASCRegs_p, *Buffer++);
#else
            	ASCRegs_p->ASCnTxBuffer = *Buffer++;
#endif
            	NumberWritten++;
		/*Wait for end of transmission in order to re-enable 
		as quickly as possible the FIFO and the receiver */
#ifdef ST_OSLINUX
		while(!(UART_Status(ASCRegs_p) & STATUS_TXEMPTY));
#else
        	while (!(ASCRegs_p->ASCnStatus & STATUS_TXEMPTY));
#endif
		//Tx FIFO is empty here
		UART_FifoEnable(ASCRegs_p);
		UART_ReceiverEnable(ASCRegs_p);
		STOS_InterruptUnlock();
	}
	else
    	{
#ifdef ST_OSLINUX
        	while(NumberToWrite > 0 && (UART_Status(ASCRegs_p) & STATUS_TXFULL) == 0)
#else
        	while(NumberToWrite > 0 && (ASCRegs_p->ASCnStatus & STATUS_TXFULL) == 0)
#endif
        	{
#ifdef ST_OSLINUX
            		UART_WriteTxBuf(ASCRegs_p, *Buffer++);
#else
            		ASCRegs_p->ASCnTxBuffer = *Buffer++;
#endif
            		NumberWritten++;
            		NumberToWrite--;
        	}
    	}
    }

#if STUART_STATISTICS
    /* Increment bytes transmitted statistics */
    Uart_p->Statistics.NumberBytesTransmitted += NumberWritten;
#endif

    return NumberWritten;
} /* UART_WriteBlock() */

/*****************************************************************************
UART_Write9BitBlock()
Description:
    This routine transmits a block of data through the UART's ASCnTxBuffer
    register until either the UART's FIFO is full or all required bytes have
    been written - the data must be 9-bit no parity i.e., each 9-bit data
    unit is treated as two consecutive U8s.  The least significant bit of
    the most significant byte is the 9th data bit.
Parameters:
    Uart_p,         pointer the the UART control block to set the baud rate
                    for.
    Buffer,         pointer to buffer containing 9-bit data to transmit.
    NumberToWrite,  number of bytes to write.
Return Value:
    The number of bytes written.
See Also:
    STUART_Write()
*****************************************************************************/

U32 UART_Write9BitBlock(UART_ControlBlock_t *Uart_p,
                                 U8 *Buffer,
                                 U32 NumberToWrite)
{

#if !defined(ARCHITECTURE_ST40) && !defined(PROCESSOR_C1) && !defined(ST_OSLINUX)
#pragma ST_device(ASCRegs_p)
#endif

#ifdef ST_OSLINUX
    volatile UART_ASCRegs_t *ASCRegs_p = (UART_ASCRegs_t *)Uart_p->MappingBaseAddress;
#else
    volatile UART_ASCRegs_t *ASCRegs_p = (UART_ASCRegs_t *)Uart_p->InitParams.BaseAddress;
#endif

    U32 NumberWritten = 0;

    /* Write out one 9-bit unit at a time to ASCnTxBuffer until all bytes
     * have gone or the transmit FIFO is full.
     */
#ifdef ST_OSLINUX
    while (NumberToWrite > 0 && (UART_Status(ASCRegs_p) & STATUS_TXFULL) == 0)
#else
    while (NumberToWrite > 0 && (ASCRegs_p->ASCnStatus & STATUS_TXFULL) == 0)
#endif
    {
        U16 DataUnit9Bits;

        DataUnit9Bits = *(Buffer+1);  /* 9th data bit in MSB */
        DataUnit9Bits <<= 8;          /* Shift it to 9th bit position */
        DataUnit9Bits |= *Buffer;     /* Copy in LSB */
#ifdef ST_OSLINUX
        UART_WriteTxBuf(ASCRegs_p, DataUnit9Bits);
#else
        ASCRegs_p->ASCnTxBuffer = DataUnit9Bits;
#endif
        NumberWritten+=2;
        NumberToWrite-=2;
        Buffer+=2;
    }

#if STUART_STATISTICS
    /* Increment bytes transmitted statistics */
    Uart_p->Statistics.NumberBytesTransmitted += NumberWritten;
#endif

    return NumberWritten;
} /* UART_Write9BitBlock() */

/*****************************************************************************
UART_SetProtocol()
Description:
    Sets the protocol intended for use by the UART device.

Parameters:
    Uart_p,     the UART control block to set params for.
    Protocol_p, pointer to a structure containing the communications
                parameters to be used by the UART device.
Return Value:
    Nothing.
See Also:
    UART_CheckProtocol()
*****************************************************************************/

void UART_SetProtocol(UART_ControlBlock_t *Uart_p,
                      STUART_Protocol_t *Protocol_p)
{
#if !defined(ARCHITECTURE_ST40) && !defined(PROCESSOR_C1) && !defined(ST_OSLINUX)
#pragma ST_device(ASCRegs_p)
#endif

    volatile UART_ASCRegs_t *ASCRegs_p;

#ifdef ST_OSLINUX
    ASCRegs_p = (UART_ASCRegs_t *)Uart_p->MappingBaseAddress;
#else
    ASCRegs_p = (UART_ASCRegs_t *)Uart_p->InitParams.BaseAddress;
#endif

    /* Set Mode */
    if (Uart_p->LastProtocol.DataBits != Protocol_p->DataBits)
    {
        switch(Protocol_p->DataBits)
        {
            case STUART_7BITS_ODD_PARITY:
                UART_7BitsParity(ASCRegs_p);
                UART_ParityOdd(ASCRegs_p);
                break;

            case STUART_7BITS_EVEN_PARITY:
                UART_7BitsParity(ASCRegs_p);
                UART_ParityEven(ASCRegs_p);
                break;
            case STUART_8BITS_NO_PARITY:
                UART_8Bits(ASCRegs_p);
                break;
            case STUART_8BITS_ODD_PARITY:
                UART_8BitsParity(ASCRegs_p);
                UART_ParityOdd(ASCRegs_p);
                break;

            case STUART_8BITS_EVEN_PARITY:
                UART_8BitsParity(ASCRegs_p);
                UART_ParityEven(ASCRegs_p);
                break;

            case STUART_8BITS_PLUS_WAKEUP:
                UART_8BitsWakeup(ASCRegs_p);
                break;

            case STUART_9BITS_NO_PARITY:
                UART_9Bits(ASCRegs_p);
                break;
            default:
                break;
        }
    }

    /* Set stop bits */
    if (Uart_p->LastProtocol.StopBits != Protocol_p->StopBits)
    {
        switch(Protocol_p->StopBits)
        {
            case STUART_STOP_0_5:
                UART_0_5_StopBits(ASCRegs_p);
                break;

            case STUART_STOP_1_0:
                UART_1_0_StopBits(ASCRegs_p);
                break;

            case STUART_STOP_1_5:
                UART_1_5_StopBits(ASCRegs_p);
                break;

            case STUART_STOP_2_0:
                UART_2_0_StopBits(ASCRegs_p);
                break;

            default:
                break;
        }
    }

    /* Set flow control */
    if (Uart_p->LastProtocol.FlowControl != Protocol_p->FlowControl)
    {
        switch(Protocol_p->FlowControl)
        {
            case STUART_NO_FLOW_CONTROL:
                if (UART_IsRTSCTSSupported(Uart_p->InitParams.UARTType))
                    UART_CTSDisable(ASCRegs_p);
                break;

            case STUART_RTSCTS_FLOW_CONTROL:
                UART_CTSEnable(ASCRegs_p);
                break;

            case STUART_SW_FLOW_CONTROL: /* Not a hardware setting */
            default:
                break;
        }
    }

    /* Set baud rate */
    UART_BaudRate(Uart_p,
                  Protocol_p->BaudRate,
                  TRUE
                 );

    Uart_p->LastProtocol = *Protocol_p;
} /* UART_SetProtocol() */

/*****************************************************************************
UART_SetParams()
Description:
    This call should only be made after a call to STUART_SetParams() has
    been performed i.e., the new parameters are ready to be committed.
    It is responsible for setting up the UART registers appropriately for
    the new parameters.  Note that a call to UART_SetProtocol() should be
    made in order to set the protocol elements of the STUART_Params_t
    datastructure.

Parameters:
    Uart_p,     the UART control block to set params for.
Return Value:
    Nothing.
See Also:
    STUART_Read()
    STUART_Write()
*****************************************************************************/

void UART_SetParams(UART_ControlBlock_t *Uart_p)
{

#if !defined(ARCHITECTURE_ST40) && !defined(PROCESSOR_C1) && !defined(ST_OSLINUX)
#pragma ST_device(ASCRegs_p)
#endif

    volatile UART_ASCRegs_t *ASCRegs_p;
    U32 OldIntEnableRx, OldIntEnableTx;
    U32 i;
    STPIO_BitConfig_t BitConfigure[8];


#ifdef ST_OSLINUX
    ASCRegs_p = (volatile UART_ASCRegs_t *)Uart_p->MappingBaseAddress;
#else
    ASCRegs_p = (volatile UART_ASCRegs_t *)Uart_p->InitParams.BaseAddress;
#endif

    /* Handle SmartCard setup as appropriate */
    if (Uart_p->DefaultParams.SmartCardModeEnabled)
    {
        /* If smartcard mode is enabled, we must be careful to ensure
         * that the TXD pin is set in bidirectional mode i.e., the
         * ASC is effectively operating in half-duplex mode. Otherwise
         * we'll be driving the line when the card is trying to transmit
         */
        i = UART_PIOBitFromBitMask(Uart_p->InitParams.TXD.BitMask);
        BitConfigure[i] = STPIO_BIT_ALTERNATE_BIDIRECTIONAL;
        STPIO_SetConfig(Uart_p->TXDHandle, BitConfigure);

        /* Enable smart card mode */
        UART_SmartCardEnable(ASCRegs_p);

        /* Set guard time appropriately */
        ASCRegs_p->ASCnGuardTime = Uart_p->DefaultParams.GuardTime;

        if (Uart_p->InitParams.UARTType == STUART_ISO7816)
        {
            /* Set NACK behaviour */
            if (Uart_p->DefaultParams.NACKSignalDisabled == FALSE)
            {
                UART_NACKEnable(ASCRegs_p);
            }
            else
            {
                UART_NACKDisable(ASCRegs_p);
            }

            /* Set HWFifo behaviour */
            if (Uart_p->DefaultParams.HWFifoDisabled == FALSE)
            {
                /* Enable the hardware FIFO */
                UART_FifoEnable(ASCRegs_p);
                Uart_p->HwFIFOSize = 16;
            }
            else
            {
                /* Disable the hardware FIFO */
                UART_FifoDisable(ASCRegs_p);
                Uart_p->HwFIFOSize = 1;
            }
        }
    }
    else
    {
        /* Restore default PIO from STUART_Init, if different */
        if ((Uart_p->InitParams.TXD.PortName[0] != 0) && !Uart_p->IsHalfDuplex)
        {
            i = UART_PIOBitFromBitMask(Uart_p->InitParams.TXD.BitMask);
            BitConfigure[i] = STPIO_BIT_ALTERNATE_OUTPUT;
            STPIO_SetConfig(Uart_p->TXDHandle, BitConfigure);
        }

        /* Disable smartcard mode */
        UART_SmartCardDisable(ASCRegs_p);
    }

    OldIntEnableRx = Uart_p->IntEnableRxFlags;
    OldIntEnableTx = Uart_p->IntEnableTxFlags;

    /* Setup interrupt enable flags */
    if (Uart_p->HwFIFOSize > 1)
    {
        /* HwFIFOSize other than 1: we can take advantage of
         * back-to-back transfers using halfempty interrupts.
         */
        Uart_p->IntEnableRxFlags = UART_IE_ENABLE_RX_FIFO;
        Uart_p->IntEnableTxFlags = UART_IE_ENABLE_TX_FIFO;
    }
    else
    {
        /* HwFIFOSize is 1: we cannot do back-to-back transfers
         * because we must wait until the FIFO is empty.
         */
        /* Infact, for Tx even with FIFO disabled, and even on ASC,
          mark 2, we can still put data in at TXHALFEMPTY. The
          exception is NDS flow control, where we must know we are
          allowed to transmit before starting to do so */
        Uart_p->IntEnableRxFlags = UART_IE_ENABLE_RX_NO_FIFO;
        Uart_p->IntEnableTxFlags = UART_DoesNdsFlowControlApply(Uart_p)
            ? UART_IE_ENABLE_TX_END : UART_IE_ENABLE_TX_NO_FIFO;
    }

    /* be sure any interrupts being turned off are actually removed
      (we generally clear using ~Uart_p->IntEnableTxFlags) */
    Uart_p->IntEnable &= ~(OldIntEnableRx & ~Uart_p->IntEnableRxFlags);
    Uart_p->IntEnable &= ~(OldIntEnableTx & ~Uart_p->IntEnableTxFlags);

} /* UART_SetParams() */

/*****************************************************************************
Debug/test routines
*****************************************************************************/

#ifdef STUART_DEBUG

void UART_DumpRegs(UART_ControlBlock_t *Uart_p)
{

#if !defined(ARCHITECTURE_ST40) && !defined(PROCESSOR_C1) && !defined(ST_OSLINUX)
#pragma ST_device(ASCRegs_p)
#endif

#ifdef ST_OSLINUX
    volatile UART_ASCRegs_t *ASCRegs_p = (volatile UART_ASCRegs_t *)Uart_p->MappingBaseAddress;
    printk("(0x%04x) ASCnBaudRate  = 0x%04x\n", (U32)&ASCRegs_p->ASCnBaudRate, UART_ReadBaudRate(ASCRegs_p));
    printk("(0x%04x) ASCnTxBuffer  = 0x%04x\n", (U32)&ASCRegs_p->ASCnTxBuffer, UART_ReadTxBuf(ASCRegs_p));
    printk("(0x%04x) ASCnRxBuffer  = 0x%04x\n", (U32)&ASCRegs_p->ASCnRxBuffer, UART_ReadRxBuf(ASCRegs_p));
    printk("(0x%04x) ASCnControl   = 0x%04x\n", (U32)&ASCRegs_p->ASCnControl,  UART_ReadControl(ASCRegs_p));
    printk("(0x%04x) ASCnIntEnable = 0x%04x\n", (U32)&ASCRegs_p->ASCnIntEnable, UART_ReadIntEnable(ASCRegs_p));
    printk("(0x%04x) ASCnStatus    = 0x%04x\n", (U32)&ASCRegs_p->ASCnStatus, UART_Status(ASCRegs_p));
    printk("(0x%04x) ASCnGuardTime = 0x%04x\n", (U32)&ASCRegs_p->ASCnGuardTime, UART_ReadGuardTime(ASCRegs_p));
#else
    volatile UART_ASCRegs_t *ASCRegs_p = (volatile UART_ASCRegs_t *)Uart_p->InitParams.BaseAddress;

    STTBX_Print(("(0x%04x) ASCnBaudRate  = 0x%04x\n", (U32)&ASCRegs_p->ASCnBaudRate, ASCRegs_p->ASCnBaudRate));
    STTBX_Print(("(0x%04x) ASCnTxBuffer  = 0x%04x\n", (U32)&ASCRegs_p->ASCnTxBuffer, ASCRegs_p->ASCnTxBuffer));
    STTBX_Print(("(0x%04x) ASCnRxBuffer  = 0x%04x\n", (U32)&ASCRegs_p->ASCnRxBuffer, ASCRegs_p->ASCnRxBuffer));
    STTBX_Print(("(0x%04x) ASCnControl   = 0x%04x\n", (U32)&ASCRegs_p->ASCnControl, ASCRegs_p->ASCnControl));
    STTBX_Print(("(0x%04x) ASCnIntEnable = 0x%04x\n", (U32)&ASCRegs_p->ASCnIntEnable, ASCRegs_p->ASCnIntEnable));
    STTBX_Print(("(0x%04x) ASCnStatus    = 0x%04x\n", (U32)&ASCRegs_p->ASCnStatus, ASCRegs_p->ASCnStatus));
    STTBX_Print(("(0x%04x) ASCnGuardTime = 0x%04x\n", (U32)&ASCRegs_p->ASCnGuardTime, ASCRegs_p->ASCnGuardTime));
    STTBX_Print(("(0x%04x) ASCnTimeout   = 0x%04x\n", (U32)&ASCRegs_p->ASCnTimeout, ASCRegs_p->ASCnTimeout));
#endif
}

#endif /* STUART_DEBUG */
/* End of uartdrv.c */

