/******************************************************************************

    File Name   :

    Description :

******************************************************************************/

/* Includes ---------------------------------------------------------------- */
#if defined (ST_OSLINUX)
#include <linux/stddef.h>
#include <linux/interrupt.h>
#include <asm/semaphore.h>
#else
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#endif

#include "local.h"

/*#define PROFILE_FDMA_INTERRUPT 1*/			
#define FDMA_INTERRUPT_OPTIMIZE 	1 		

#if defined (PROFILE_FDMA_INTERRUPT)
U32 fdma_int_TimeStart = 0;
U32 fdma_int_TimeEnd = 0;
U32 fdma_int_timeTaken = 0;
extern semaphore_t * interrupt_profile_sem_p;
#endif

#if defined (STFDMA_DEBUG_SUPPORT)
extern STFDMA_Status_t *stfdma_Status[STFDMA_MAX];
#endif


/*Extern Variables ------------------------------------------------------- */

#if defined (ST_OSLINUX) && defined (CONFIG_STM_DMA)
extern unsigned int STFDMA_MaxChannels;
extern unsigned int STFDMA_Mask;
#endif

/* Private Types ----------------------------------------------------------- */

/* Private Constants ------------------------------------------------------- */

#define INTERRUPT_CHANNEL_MASK 0x3     /* Selects two bits at a time */

/* Private Variables ------------------------------------------------------- */

/* Private Macros ---------------------------------------------------------- */

/* Private Function Prototypes --------------------------------------------- */

/* Functions --------------------------------------------------------------- */

/****************************************************************************
Name         : Interrupt handler for FDMA1
Description  : For blocking transfers, interrupt data is written to a global
                location and a semaphore is signalled.  For non-blocking transfers,
                a message is added to the message queue.
Parameters   :
Return Value :
****************************************************************************/
STOS_INTERRUPT_DECLARE(stfdma_InterruptHandler, pParams)
{
    MessageElement_t    MessageElement;
    U32                 InterruptStatus; 
#if defined (FDMA_INTERRUPT_OPTIMIZE)
	U32					InterruptClear = 0;
#endif
    BOOL                InterruptOccurred = TRUE; 
    int                 Channel; 
    U32                 DeviceNo; 
#if !defined(ST_5517)
    int                 State;
#endif

#if defined (CONFIG_STM_DMA)
    U32                 SavedInterruptStatus;
#endif

    DeviceNo = *(U32 *)pParams;

    MessageElement.InterruptTime = time_now();
#if defined (PROFILE_FDMA_INTERRUPT)		
	fdma_int_TimeStart = MessageElement.InterruptTime;
#endif

#if defined (ST_OSLINUX) && defined (CONFIG_STM_DMA)
    InterruptStatus  = GET_REG(STATUS, DeviceNo);
    /* Check if interrupt for STAPI FDMA channels (from 0 to STFDMA_MaxChannels) */
    if (!(InterruptStatus & STFDMA_Mask))
    {
       return IRQ_NONE;
    }
    SavedInterruptStatus = InterruptStatus;
#else
    InterruptStatus  = GET_REG(STATUS, DeviceNo);
#endif

    /* Bitwise search to find interrupt */
#if defined (ST_OSLINUX) && defined (CONFIG_STM_DMA)
    for (Channel=0; (Channel<STFDMA_MaxChannels) && (InterruptStatus != 0); Channel++)
#else
    for (Channel=0; (Channel<NUM_CHANNELS) && (InterruptStatus != 0); Channel++)
#endif

    {
        /* 2Bits per channel, with bits 1 and 0 for channel 0, bits 3 and 2 for channel 1 etc */
#if defined (FDMA_INTERRUPT_OPTIMIZE)	
		 while(!(InterruptStatus & INTERRUPT_CHANNEL_MASK)) 	
		 {
		 	InterruptStatus >>= 2; 
			Channel++;
		 }	
#endif		 
        switch (InterruptStatus & INTERRUPT_CHANNEL_MASK)
        {
        case 0: /* No event to report */
            InterruptOccurred = FALSE;
            break;

        case 1: /* End of node reached */
            InterruptOccurred = TRUE;
            MessageElement.ChannelNumber = Channel;
#if defined(ST_5517)
            MessageElement.InterruptType = INTERRUPT_NODE_COMPLETE;
#else
            /* Determin the signaled event */
            State = GET_NODE_STATE(Channel, DeviceNo);
            switch (State & 0X03)
            {
                case 0X00 :  /* Idle */
                    MessageElement.InterruptType = INTERRUPT_LIST_COMPLETE;
                    break;

                case 0X02 :  /* Channel Running */
                    MessageElement.InterruptType = INTERRUPT_NODE_COMPLETE;
                    break;

                case 0X03 :  /* Channel Paused */
                    if (stfdma_GetNextNodePtr(Channel, DeviceNo) == (U32)NULL)
                    {
                        MessageElement.InterruptType = INTERRUPT_LIST_COMPLETE;
                    }
                    else
                    {
                        MessageElement.InterruptType = INTERRUPT_NODE_COMPLETE_PAUSED;
                    }
                    break;

                default : /* Unknown message */
                case 0X01 :
                    InterruptOccurred = FALSE;
                    break;
            }
#endif
            break;

        case 2:
#if defined(ST_5517)
            /* Transfer halted */
            /* Determine if halted due to list completion or node completion.
             * If the nodes next ptr is NULL, end of list reached otherwise transfer
             * is paused.
             */
            if (stfdma_GetNextNodePtr(Channel, DeviceNo) == (U32)NULL)
            {
                MessageElement.InterruptType = INTERRUPT_LIST_COMPLETE;
            }
            else
            {
                MessageElement.InterruptType = INTERRUPT_NODE_COMPLETE_PAUSED;
            }
            MessageElement.ChannelNumber = Channel;
            InterruptOccurred = TRUE;
            break;
#else
            /* Error occured..interrupt missed */
            /* Drop through */
#endif

        default:
        case 3: /* Error occured..interrupt missed */
            /* Determine nature of this interrupt, regardless of error.
             * If the current nodes next ptr is NULL, the transfer is finished or
             * reached the end of the node node but one and is continuing.
             */

#if defined (STFDMA_DEBUG_SUPPORT)
            if(stfdma_Status[DeviceNo] != NULL)
                stfdma_Status[DeviceNo]->Channel[Channel].InterruptsMissed++;
#endif

            InterruptOccurred = TRUE;
            MessageElement.ChannelNumber = Channel;

#if defined (ST_5517)
            if (stfdma_GetNextNodePtr(Channel, DeviceNo) == (U32)NULL)
            {
                /* Current bytecount of 0 indicates list complete.
                 * Non 0 indicates the interrupt relates to the last node -1 completing
                 * and now the tranfer of the last node is in progress.
                 */
                if (GET_BYTE_COUNT(Channel, DeviceNo) == 0)
                {
                    MessageElement.InterruptType = INTERRUPT_ERROR_LIST_COMPLETE;
                }
                else
                {
                    MessageElement.InterruptType = INTERRUPT_ERROR_NODE_COMPLETE;
                }
            }
            else  /* Next ptr not NULL, so end of list not yet reached. */
            {
                /* If the transfer is not paused, the current byte count determines
                 * the status. Otherwise, the current node is complete and the transfer
                 * is pauased.
                 */
                if (stfdma_IsNodePauseEnabled(Channel, DeviceNo))
                {
                    /* A count of 0 indicates no more transfer activity, thus the list
                     * transfer is complete.
                     * Otherwise, if the count value is non-0, transfer continues and so
                     * this interrupt relates to the completion of the previous node.
                     */
                    if (GET_BYTE_COUNT(Channel, DeviceNo) == 0)
                    {
                        MessageElement.InterruptType = INTERRUPT_ERROR_NODE_COMPLETE_PAUSED;
                    }
                    else
                    {
                        MessageElement.InterruptType = INTERRUPT_ERROR_NODE_COMPLETE;
                    }
                }
                else  /* Channel is not paused, node is complete and transfer continues */
                {
                    MessageElement.InterruptType = INTERRUPT_ERROR_NODE_COMPLETE;
                }
            }
#else
            /* Determin the error signalled */
            State = GET_NODE_STATE(Channel, DeviceNo);
            if (State & 0X1C)
            {
#if defined (STFDMA_DEBUG_SUPPORT)
                if(stfdma_Status[DeviceNo] != NULL)
                    stfdma_Status[DeviceNo]->Channel[Channel].InterruptsMissed--;
#endif

                InterruptOccurred = FALSE;
            }
            else /* Missed the interupt */
            {
                /* Determin the signaled event */
                State = GET_NODE_STATE(Channel, DeviceNo);
                switch (State & 0X03)
                {
                    case 0X00 :  /* Idle */
                        MessageElement.InterruptType = INTERRUPT_ERROR_LIST_COMPLETE;
                        break;

                    case 0X02 :  /* Channel Running */
                        MessageElement.InterruptType = INTERRUPT_ERROR_NODE_COMPLETE;
                        break;

                    case 0X03 :  /* Channel Paused */
                        if (stfdma_GetNextNodePtr(Channel, DeviceNo) == (U32)NULL)
                        {
                            MessageElement.InterruptType = INTERRUPT_ERROR_LIST_COMPLETE;
                        }
                        else
                        {
                            MessageElement.InterruptType = INTERRUPT_ERROR_NODE_COMPLETE_PAUSED;
                        }
                        break;

                    default : /* Unknown message */
                    case 0X01 :
#if defined (STFDMA_DEBUG_SUPPORT)
                        if(stfdma_Status[DeviceNo] != NULL)
                            stfdma_Status[DeviceNo]->Channel[Channel].InterruptsMissed--;
#endif
                        InterruptOccurred = FALSE;
                        break;
                }
            }
#endif
            break;
        }

        /* Process interrupt if necessary */
        if (InterruptOccurred)
        {
            /* Blocking transfers require STFDMA_StartTrasfer to be unblocked with
             * semaphore signal, but only if full transfer is complete. Other events
             * do not unblock a blocked transfers.
             * Non-blocking transfer are informed of the interrupt via the callback
             * manager task(s).
             */

#if defined (STFDMA_DEBUG_SUPPORT)
            if(stfdma_Status[DeviceNo] != NULL)
                stfdma_Status[DeviceNo]->Channel[Channel].InterruptsTotal++;
#endif

            if (IS_BLOCKING(Channel, DeviceNo))
            {
#if defined (STFDMA_DEBUG_SUPPORT)
                if(stfdma_Status[DeviceNo] != NULL)
                    stfdma_Status[DeviceNo]->Channel[Channel].InterruptsBlocking++;
#endif

                if (IS_IDLE(Channel, DeviceNo))
                {
                    /* Nothing to signal - channel stopped */
                }
                else
                {
                    SET_BLOCKING_REASON(Channel, MessageElement.InterruptType, DeviceNo);

                    if (MessageElement.InterruptType == INTERRUPT_NODE_COMPLETE_PAUSED)
                    {
                        /* Transfer is either paused or an abort request been actioned.
                         * Pausing a blocking transfer is illegal and must result in an abort.
                         */
                        SET_ABORTING(Channel, DeviceNo);
                    }

                    /* If we had an error then let the application know */

                    if ((MessageElement.InterruptType == INTERRUPT_ERROR_LIST_COMPLETE) ||
                        (MessageElement.InterruptType == INTERRUPT_ERROR_NODE_COMPLETE_PAUSED))
                    {
                        SET_ERROR(Channel, DeviceNo);
                    }

                    /* If the transfer has completed, paused/aborted (same thing when blocking),
                     * then wake up the blocking StartTransfer().
                     */
                    if ((MessageElement.InterruptType == INTERRUPT_LIST_COMPLETE) ||
                        (MessageElement.InterruptType == INTERRUPT_NODE_COMPLETE_PAUSED) ||
                        (MessageElement.InterruptType == INTERRUPT_ERROR_LIST_COMPLETE) ||
                        (MessageElement.InterruptType == INTERRUPT_ERROR_NODE_COMPLETE_PAUSED))
                    {
                        SET_IDLE(Channel, DeviceNo);
                        STOS_SemaphoreSignal(GET_BLOCKING_SEM_PTR(Channel, DeviceNo));
                    }
                }

#if defined (ST_OSLINUX)
                wake_up_interruptible(&(stfdma_ControlBlock_p[DeviceNo]->ChannelTable[Channel].blockingQueue));
#endif
            }
            else
            {
#if defined (STFDMA_DEBUG_SUPPORT)
                if(stfdma_Status[DeviceNo] != NULL)
                    stfdma_Status[DeviceNo]->Channel[Channel].InterruptsNonBlocking++;
#endif

                /* Notify a callback manager of the interrupt */
                stfdma_SendMessage(&MessageElement, DeviceNo);				  	
            }
        }

        /* Acknowledge the interupt even if it is not a recognised event */
        if (InterruptStatus & INTERRUPT_CHANNEL_MASK)
        {
#if defined(ST_5517)
            /* No acknowledge on the STi5517 */
#else
#ifndef FDMA_INTERRUPT_OPTIMIZE
            SET_REG(STATUS_CLR, ((InterruptStatus & INTERRUPT_CHANNEL_MASK)<<(Channel*2)), DeviceNo);
#else
			  InterruptClear	|= ((InterruptStatus & INTERRUPT_CHANNEL_MASK)<<(Channel*2));			
#endif
#endif
        }

        /* Deal with other channels interrupts.
         * There are two status bits per channel - channel 0 is LSBits.
         */
        InterruptStatus >>= 2;
    }
#if defined (FDMA_INTERRUPT_OPTIMIZE)
            SET_REG(STATUS_CLR, InterruptClear, DeviceNo);
#endif

#if defined (PROFILE_FDMA_INTERRUPT)
	fdma_int_TimeEnd = time_now();
	fdma_int_timeTaken =  fdma_int_TimeEnd - fdma_int_TimeStart; 
	STOS_SemaphoreSignal(interrupt_profile_sem_p);	
#endif

	
#if defined (ST_OS21) || defined (ST_OSWINCE)
    return OS21_SUCCESS;
#elif defined (ST_OSLINUX)

#if defined (CONFIG_STM_DMA)
    /* If some interrupts are for an other driver, return IRQ_NONE */
    if (SavedInterruptStatus & ~STFDMA_Mask)
    {
    	/* printk("IRQ_None : 0x%x\n", SavedInterruptStatus); */
     	return IRQ_NONE;
    }
    else
    {
        return IRQ_HANDLED;
    }
#else
        return IRQ_HANDLED;
#endif

#endif
}

/*eof*/
