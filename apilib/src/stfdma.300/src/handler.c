/******************************************************************************

    File Name   :

    Description :

******************************************************************************/

/* Includes ---------------------------------------------------------------- */
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

#if defined (ST_498)
U32 WaitLoop = 0;
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

#if defined (STFDMA_MPX_SUPPORT)

#if defined (STFDMA_MPX_USE_EXT_IRQ3)  
extern U32		* ILCMappedBaseAddr;

#else
/****************************************************************************
Name         : PIO Interrupt handler for FDMA_MPX when using PIO->PIO interupt
Description  : 
Parameters   :
Return Value :
****************************************************************************/
void stfdma_PIOInterruptHandler(STPIO_Handle_t Handle, STPIO_BitMask_t ActiveBits)
{
	U32 param = STFDMA_MPX;
#if defined (ST_OSLINUX)
	stfdma_InterruptHandler(0, (void *)&param, NULL);
#else
	stfdma_InterruptHandler((void *)&param);
#endif	
}
#endif /*STFDMA_MPX_USE_EXT_IRQ3*/

#endif /*STFDMA_MPX_SUPPORT*/


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

#if defined (ST_498)
#if defined (STFDMA_MPX_USE_EXT_IRQ3)   
	U32 Value = 0;
#endif	
	U32 *  SharedInterruptStatus = (U32 *)STFDMA_MPX_ECM_SHARED_INT_STATUS;
#endif
#if defined (STFDMA_MPX_SUPPORT)
#if defined (STFDMA_MPX_USE_EXT_IRQ3)   
	U32 Value = 0;
#endif	
	U32 *  SharedInterruptStatus = NULL; 
	SharedInterruptStatus = (U32 *)STFDMA_MPX_ESTB_SHARED_INT_STATUS;
#endif

    MessageElement.InterruptTime = time_now();
#if defined (PROFILE_FDMA_INTERRUPT)		
	fdma_int_TimeStart = MessageElement.InterruptTime;
#endif

#if defined (STFDMA_MPX_SUPPORT)
	/*Clear the external interrupt that was routed from 498 to 7109*/
    if(DeviceNo == STFDMA_MPX)
    {    	
        stfdma_ResetPIOBit(MPX_ECM_PIOx_BITy);
#if defined (STFDMA_MPX_USE_EXT_IRQ3)        
        Value = STSYS_ReadRegDev32LE(ILC3_CLR_EN2(ILCMappedBaseAddr));      
        Value |= 0x8;
        STSYS_WriteRegDev32LE(ILC3_CLR_EN2(ILCMappedBaseAddr), Value);
#endif        
    }
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
#if defined(STFDMA_MPX_SUPPORT)
    if(DeviceNo == STFDMA_MPX)
    {
	 	InterruptStatus = STSYS_ReadRegDev32LE(SharedInterruptStatus);
	}	 	
#endif
#endif

    /* Bitwise search to find interrupt */
#if defined (ST_OSLINUX) && defined (CONFIG_STM_DMA)
    for (Channel=0; (Channel<STFDMA_MaxChannels) && (InterruptStatus != 0); Channel++)
#elif defined (ST_498)
 /*For STFDMA running on 498 the NUM_CHANNELS == 14 , herefore last two channels (14-15) will be ignored*/
	/*NUM_CHANNELS == 14 for ST_498 so that users on ST_498 see only 14 ch but interrupt handles 16 ch where last 2 ch is for MCHI*/
    for (Channel=0; (Channel<(NUM_CHANNELS + 2)) && (InterruptStatus != 0); Channel++) 
#else
    for (Channel=0; (Channel<NUM_CHANNELS) && (InterruptStatus != 0); Channel++)
#endif

    {
 #if defined (STFDMA_MPX_SUPPORT)
    	/*For 7109 STFDMA controlling 498 H/W skip(ignore) all channels (0-13) exept last 2 (14/15) used for MCHI */
		if(DeviceNo == STFDMA_MPX && Channel == 0)
		{
			Channel = MCHI_CHANNEL_NUMBER_START;
			InterruptStatus >>= 2*	MCHI_CHANNEL_NUMBER_START;
		}
#endif
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
#if defined (STFDMA_MPX_SUPPORT) 
				  if(DeviceNo == STFDMA_MPX)
				  {
	                switch (((State>>2) & 0x03))
	                {
	                    case 0X02 :  /* MCHI timeout error */
	                        MessageElement.InterruptType = INTERRUPT_ERROR_MCHI_TIMEOUT;
	                        InterruptOccurred = TRUE;
	                        break;

	                    default : /* Unknown message */
	                        InterruptOccurred = FALSE;
	                        break;
	                } 
                }
                else
                {
#endif /*STFDMA_MPX_SUPPORT*/                            
#if defined (STFDMA_DEBUG_SUPPORT)
                if(stfdma_Status[DeviceNo] != NULL)
                    stfdma_Status[DeviceNo]->Channel[Channel].InterruptsMissed--;
#endif

                InterruptOccurred = FALSE;
#if defined (STFDMA_NAND_SUPPORT)
                if(State & 0x4)
                {
                    InterruptOccurred = TRUE;
                    MessageElement.InterruptType = INTERRUPT_ERROR_NAND_READ;
                }
#endif
#if defined (STFDMA_MPX_SUPPORT)                 
                }                
#endif /*STFDMA_MPX_SUPPORT*/                
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

#if defined (ST_498)
		if(!(Channel == MCHI_CHANNEL_1 || Channel == MCHI_CHANNEL_2))
#endif		
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
                        (MessageElement.InterruptType == INTERRUPT_ERROR_NODE_COMPLETE_PAUSED)
#if defined (STFDMA_MPX_SUPPORT)
                        ||(MessageElement.InterruptType == INTERRUPT_ERROR_MCHI_TIMEOUT)
#endif                        
#if defined (STFDMA_NAND_SUPPORT)                        
                        ||(MessageElement.InterruptType == INTERRUPT_ERROR_NAND_READ)
#endif                        
                        )
                    {
                        SET_ERROR(Channel, DeviceNo);
                    }

                    /* If the transfer has completed, paused/aborted (same thing when blocking),
                     * then wake up the blocking StartTransfer().
                     */
                    if ((MessageElement.InterruptType == INTERRUPT_LIST_COMPLETE) ||
                        (MessageElement.InterruptType == INTERRUPT_NODE_COMPLETE_PAUSED) ||
                        (MessageElement.InterruptType == INTERRUPT_ERROR_LIST_COMPLETE) ||
                        (MessageElement.InterruptType == INTERRUPT_ERROR_NODE_COMPLETE_PAUSED)
#if defined (STFDMA_MPX_SUPPORT)
							||(MessageElement.InterruptType == INTERRUPT_ERROR_MCHI_TIMEOUT)
#endif
                        )
                    {
                        SET_IDLE(Channel, DeviceNo);
                        STOS_SemaphoreSignal(GET_BLOCKING_SEM_PTR(Channel, DeviceNo));
                    }
                }
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
#if defined (STFDMA_MPX_SUPPORT)
	 if(DeviceNo != STFDMA_MPX)
	 {
	 	SET_REG(STATUS_CLR, InterruptClear, DeviceNo);   	 	
	 }
#else
	SET_REG(STATUS_CLR, InterruptClear, DeviceNo);    
#endif
#endif
#if defined (ST_498)
			 if((InterruptClear & MCHI_CHANNEL_MASK)) /*any interrupt on ch 14/15*/
			 {			 		
				    *SharedInterruptStatus |= (InterruptClear & MCHI_CHANNEL_MASK);	/*never clear any ch14/15 interrupt from 498*/	
#if defined (STFDMA_MPX_USE_EXT_IRQ3)				   
        			Value = STSYS_ReadRegDev32LE(MPX_7109_ILC3_SET_EN2);
        			Value |= 0x8;				   
			 		STSYS_WriteRegDev32LE(MPX_7109_ILC3_SET_EN2, Value);	
#endif			 		
			 		stfdma_SetPIOBit(MPX_ECM_PIOx_BITy);

					do
					{	
			 			/*
			 			!!!Inevitable wait to prevent race condition for MCHI channels!!!
			 			In normal cases the interrupt handler exits when the interrupt processing is complete.
			 			In the eCM->eSTB interrupt the interrupt processing is not complete till the eSTB interrupt handler
			 			processes its interrupt once it is routed from eCM to eSTB, hence added a looped wait at eCM side to 
			 			wait till eSTB signals its interrupt processing as done by clearing the shared interrupt mem location
			 			*/
			 			/*
			 			On some platforms a simple "while(*SharedInterruptStatus != 0)" loop does not work since it blocks 
			 			the LMI subsystem not allowing the eSTB interrupt handler to clear the shared mem location. To circumvent 
			 			this added few lines of code using a global variable that is "not optimized" out by the compiler which acts as 
			 			a NOP on eCM side to unblock the memory subsystem allowing eSTB interrupt handler to write to the shared 
			 			mem location
			 			*/					
						if(WaitLoop == InterruptClear)
							WaitLoop++;
						else
							WaitLoop--;
					}while(*SharedInterruptStatus != 0);					

			  }	
#endif

#if defined(STFDMA_MPX_SUPPORT)
	if(DeviceNo == STFDMA_MPX)
	{	 	
		STSYS_WriteRegDev32LE(SharedInterruptStatus, 0);
	}	 	
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
