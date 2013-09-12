/*****************************************************************************
File Name   : fdmasim.c

Description : Hardware simulator for STFDMA testing
              Provides basic FDMA interface...no transfers ta ke place
              for testing on silicon without FDMA hardware block

Copyright (C) 2002 STMicroelectronics

History     :

Reference   :
*****************************************************************************/
#include <string.h>                     /* C libs */
#include <stdio.h>

#include "sttbx.h"
#include "stcommon.h"

#include "stlite.h"                     /* Standard includes */
#include "stddefs.h"


#include "stfdma.h"                     /* fdma api header file */
#include "fdmatst.h"                    /* test header files */
#include "fdmasim.h"

/* Includes --------------------------------------------------------------- */

/* Private  Constants ------------------------------------------------------ */

/* Only set the following for visually checking the req line values set */
/*#define FDMASIM_CHECK_REQ_LINE_SETUP 1*/

/* Hardware access constants...offsets from BaseAddress */
#define ID                  0x00       /* Hardware id */
#define VERSION             0x04       /* Hardware version number */ 
#define ENABLE              0x08       /* Block enable control */
#define SW_ID               0x4000     /* ASM software ID */
#define STATUS              0x4004     /* Interrupt status register */
#define NODE_PTR_BASE       0x4014     /* Channel 0 Node ptr address.
                                        * Other channels at (NODE_PTR_BASE + (Channel * 4)) */

#define REQ_CONT_BASE       0x402c     /* Channel 0 request signal control.
                                        * Other channels at (REQ_CONT_BASE + (ReqLine * 4)) */

#define COUNT_BASE          0x4184     /* Channel 0 bytes transfered count 
                                        * Other channels at (COUNT_BASE + (Channel * 64)) */

#define DMEM_OFFSET         0x4000     /* contains the control word interface */
#define IMEM_OFFSET         0x6000

#define REQ_CONT_OFFSET         0x04    /* Request control offset = 4 bytes */

#define INTERRUPTCHANNELMASK 0x3        /* selects two bits at a time */



/* Number of channels to simulate */
#define NUM_CHANNELS    6

#define NUM_TASKS   (NUM_CHANNELS)

#define TASK_STACK_SIZE     1024

#define NODE_END                0x01
#define TRANSFER_HALTED         0x02


/* Private Variables ------------------------------------------------------- */

static U32 SimBaseAddress = 0;
static U32 SimInterruptNumber = 0;
static partition_t *SimDriverPartition_p = NULL;

/* Type for task data */
typedef struct 
{
    semaphore_t         WakeUp;    /* Only relevant for channel tasks */
    BOOL                Suicide;
    U8                  *TaskStack;
    task_t              TaskID;
    tdesc_t             TaskDesc;
    BOOL                Paused;
}TaskDetails_t;

static TaskDetails_t TaskDetails[NUM_TASKS]; /* Task details */

static semaphore_t InterruptStatusUpdate;

/* Private Function Prototypes --------------------------------------------- */
static void ChannelTask(U32 Chan);

/* Functions --------------------------------------------------------------- */

/****************************************************************************
Name         : FDMASIM_Init
Description  : Initialises the fdma simulator.
Parameters   : 
Return Value : 
****************************************************************************/
ST_ErrorCode_t FDMASIM_Init(U32 *BaseAddress_p, U32 InterruptNumber,
                            ST_Partition_t *DriverPartition_p)
{
    U32 i= 0;
    U32 j= 0;

    VERBOSE_PRINT("SIM: FDMASIM_Init.\n");

    VERBOSE_PRINT("SIM: Initialise task stack memory and control vars.\n");
    for (i=0; i!=NUM_TASKS; i++)
    {
        TaskDetails[i].TaskStack = (U8*)memory_allocate_clear(DriverPartition_p,1,TASK_STACK_SIZE);
        if (TaskDetails[i].TaskStack == NULL)
        {
            VERBOSE_PRINT("*** FDMASIM NO_MEMORY.\n");
            
            /* deallocate memory that was successfull */
            for (j=(i-1); j!=0; j--)
            {
                memory_deallocate(DriverPartition_p,TaskDetails[i].TaskStack);
            }
            return ST_ERROR_NO_MEMORY;
        }
        semaphore_init_fifo(&TaskDetails[i].WakeUp,0);
        TaskDetails[i].Suicide = FALSE;
        TaskDetails[i].Paused = FALSE;
    }
    /* Creating global semaps */
    semaphore_init_fifo(&InterruptStatusUpdate,1);    
    
    VERBOSE_PRINT("SIM: Creating simulator tasks.\n");        

    for (i=0; i<NUM_TASKS; i++)
    {
        /* Create channel tasks, passing in channel id (i) */
        task_init((void(*)(void *))ChannelTask,
                  (void *)i,
                  TaskDetails[i].TaskStack,
                  TASK_STACK_SIZE,
                  &TaskDetails[i].TaskID,
                  &TaskDetails[i].TaskDesc,
                  MAX_USER_PRIORITY,
                  "FDMASIM_ChannelTask",
                  0);
    }

    SimBaseAddress = (U32)BaseAddress_p;
    SimInterruptNumber = InterruptNumber;
    SimDriverPartition_p = DriverPartition_p;

    /* Ensure int status zeroed! */
    (*(volatile U32*)(SimBaseAddress + STATUS)) = 0;
    
    VERBOSE_PRINT("SIM: FDMASIM_Init complete.\n");
    return ST_NO_ERROR;

}

/****************************************************************************
Name         : FDMASIM_Term
Description  : Terminates the fdma simulator resources
Parameters   : 
Return Value : 
****************************************************************************/
void FDMASIM_Term(void)
{

    U32 i =0;
    task_t *Task_p;
    
    VERBOSE_PRINT("SIM: FDMASIM_Term.\n");

    /* Kill tasks */
    VERBOSE_PRINT("SIM: Killing channels tasks.\n");
    for(i=0; i<NUM_CHANNELS;i++)
    {
        TaskDetails[i].Suicide = TRUE;
        semaphore_signal(&TaskDetails[i].WakeUp);
        Task_p = &TaskDetails[i].TaskID;
        task_wait(&Task_p, 1, TIMEOUT_INFINITY);
        task_delete(Task_p);
    }
    
    /* Kill all tasks stack and task semaps */
    for(i=0;i<NUM_TASKS;i++)
    {
        memory_deallocate(SimDriverPartition_p,TaskDetails[i].TaskStack);
        semaphore_delete(&TaskDetails[i].WakeUp);
    }

    /* Kill global semaphores */
    semaphore_delete(&InterruptStatusUpdate);    
    

    VERBOSE_PRINT("SIM: FDMASIM_Term complete.\n");
    
}

/****************************************************************************
Name         : FDMASIM_SetReg
Description  : Performs register write access from the driver
Parameters   : 
Return Value : 
****************************************************************************/
void FDMASIM_SetReg(U32 Offset, U32 Value)
{
    U32 i=0;
    STFDMA_Node_t       *Node_p;
    BOOL        ValueUpdateDone = FALSE;


#ifdef FDMASIM_CHECK_REQ_LINE_SETUP
    /* Is register write for request line configuration?
     * NOTE: Conditional since its alot of data to print and can be very obtrusive
     *       to debug output and needs to done infrequently.
     */
    if ((Offset >= (REQ_CONT_BASE + (0 * REQ_CONT_OFFSET))) &&
         (Offset <= (REQ_CONT_BASE + (31 * REQ_CONT_OFFSET))))
    {
        VERBOSE_PRINT_DATA("SIM: Set REQ_CONT 0x%x to ",Offset);
        VERBOSE_PRINT_DATA("0x%x\n",Value);        
    }
#endif
    
    /* Check if NodePtr written....*/
    for (i=0; i!=NUM_CHANNELS;i++)
    {
        if ((Offset == NODE_PTR_BASE + (i * 4)))
        {
            VERBOSE_PRINT_DATA("SIM: Updated channel %d.\n",i);
            
            /* If transfer is to be aborted...*/
            if (Value == 0)
            {
                Node_p = (STFDMA_Node_t *)*((volatile U32*)((U32)SimBaseAddress + Offset));
                
                /* Set next_p to null */
                Node_p->Next_p = NULL;                
                
                /* Do not write 0 to actual node, leave as node address (hardware behaviour) */
                ValueUpdateDone = TRUE;
                
                /* Need to generate halted interrupt when transfer paused (hardware behaviour) */
                if (TaskDetails[i].Paused == TRUE)
                {     
                    semaphore_signal(&TaskDetails[i].WakeUp);
                }                
            }
            else
            {
                /* Load NumberBytes to Channel NumberBytes location */
                Node_p = (STFDMA_Node_t *)(Value);                
                (*(U32*)(SimBaseAddress + COUNT_BASE + (i * 64))) = (U32)Node_p->NumberBytes;
                
                /* Wake up channel task */
                semaphore_signal(&TaskDetails[i].WakeUp);
            }
            break;
        }
    }

    /* Write given value to specified address */
    if (ValueUpdateDone == FALSE)
    {
        *((volatile U32*)((U32)SimBaseAddress + Offset)) = (U32)Value;
    }

}

/****************************************************************************
Name         : FDMASIM_GetReg
Description  : Performs register read access from the driver
Parameters   : 
Return Value : 
****************************************************************************/
U32 FDMASIM_GetReg(U32 Offset)
{
    U32 RetVal = 0;

    /* CAN BE CALLED UNDER INTERRUPT CONTEXT */

    /* If reading interrupt status, clear after read (hardware behaviour)*/
    if (Offset == STATUS)
    {
        RetVal = ( *(volatile U32*)((U32)SimBaseAddress + Offset) );
         ( *(volatile U32*)((U32)SimBaseAddress + Offset) )  = 0;
    }
    else
    {
        RetVal = ( *(volatile U32*)((U32)SimBaseAddress + Offset) );
    }
    
    return RetVal;
}

/****************************************************************************
Name         : ChannelTask
Description  : Mimics an FDMA channel, but does not perform any data transfer.
               Reads nodes, decrements transfer count, raises interrupts.
Parameters   : 
Return Value : 
****************************************************************************/
static void ChannelTask(U32 Chan)
{
    U32 Channel;
    clock_t EndTime;
    U32 Interrupt = 0;
    STFDMA_Node_t       *Node_p;
    BOOL GenerateInterrupt = FALSE;
    BOOL ChannelSleep = TRUE;
    

    VERBOSE_PRINT_DATA("SIM: ChannelTask %d running.\n",Chan);    
    Channel = Chan;
    while (1)
    {
        if (ChannelSleep == TRUE)
        {
            semaphore_wait(&TaskDetails[Channel].WakeUp);
            VERBOSE_PRINT_DATA("SIM: Channel %d active.\n", Channel);

            if (TaskDetails[Channel].Suicide == TRUE)
            {
                VERBOSE_PRINT_DATA("SIM: Channel %d time to die.\n",Channel);
                break;
            }

            if (TaskDetails[Channel].Paused == TRUE)
            {
                VERBOSE_PRINT_DATA("SIM: Channel %d resume.\n",Channel);            
                TaskDetails[Channel].Paused = FALSE;
            }
        }

        Node_p = (STFDMA_Node_t *)*((volatile U32*)((U32)SimBaseAddress + NODE_PTR_BASE + (Channel * 4)));

        /* Only perform dummy transfer if transfer is not being aborted */
        if (Node_p != NULL)
        {       
            /* dummy transfer..wait until 1secs passes while deccing count */
            EndTime = time_plus(time_now(),(ST_GetClocksPerSecond() * 1));
            while (time_after(time_now(),EndTime) == 0)
            {
                /* dec count as long as not 0 already */
                if ((*(U32*)(SimBaseAddress + COUNT_BASE + (Channel * 64))) != 0)
                {
                    (*(U32*)(SimBaseAddress + COUNT_BASE + (Channel * 64)))--;
                }
                task_delay(ST_GetClocksPerSecond() / 1000);
            }
            /* transfer coomplete so clear out count */
            (*(U32*)(SimBaseAddress + COUNT_BASE + (Channel * 64))) = 0;
        }

        /* Check action to take.... */
        GenerateInterrupt = FALSE;
        
        /* Transfer complete or transfer paused? */
        if (Node_p->Next_p == NULL)
        {
            /* End of list, always generate interrupt and wait for app to resume channel */
            VERBOSE_PRINT_DATA("SIM: Channel %d transfer halted.\n", Channel);            
            Interrupt = TRANSFER_HALTED;
            GenerateInterrupt = TRUE;
            ChannelSleep = TRUE;
        }
        else if (Node_p->NodeControl.NodeCompletePause == TRUE)
        {
            /* End of node and paused. App must resume transfer and will load node to resume */
            VERBOSE_PRINT_DATA("SIM: Channel %d paused.\n",Channel);
            Interrupt = TRANSFER_HALTED;
            TaskDetails[Channel].Paused = TRUE;
            ChannelSleep = TRUE;
        }
        else 
        {
            /* End of node in linked list, need to load next and continue (hardware operation) */
            Interrupt = NODE_END;            
            VERBOSE_PRINT_DATA("SIM: Channel %d end of node.\n", Channel);
            ChannelSleep = FALSE;
            (*((volatile U32*)((U32)SimBaseAddress + NODE_PTR_BASE + (Channel * 4)))) = (U32)Node_p->Next_p;
            (*(U32*)(SimBaseAddress + COUNT_BASE + (Channel * 64))) = (U32)Node_p->Next_p->NumberBytes;
        }

        /* Need to ensure interrupt generated if end of node interrupt requested */
        if (Node_p->NodeControl.NodeCompleteNotify == TRUE)
        {
            GenerateInterrupt = TRUE;
        }

        
        if (GenerateInterrupt == TRUE)
        {
            /* Write interrupt into status word and raise interrupt */
            semaphore_wait(&InterruptStatusUpdate);
            VERBOSE_PRINT_DATA("SIM: Channel %d interrupt raised.\n", Channel);
            /* 2bits per channel, bits 1 and 0 for channel 0, bits 3 and 2 for channel 1 etc. */
            (*(volatile U32*)(SimBaseAddress + STATUS)) |= (Interrupt << (Channel * 2));
            VERBOSE_PRINT_DATA("SIM: InterruptStatus: %x.\n",(*(volatile U32*)(SimBaseAddress + STATUS)));
            interrupt_raise_number(SimInterruptNumber);
            semaphore_signal(&InterruptStatusUpdate);
        }

    }
    
    VERBOSE_PRINT_DATA("SIM: ChannelTask %d terminating.\n",Channel);    
}

/* eof */
