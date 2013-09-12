/*******************************************************************************

File name   : hard_mng.c

Description : functions Hardware Manager

COPYRIGHT (C) STMicroelectronics 2004

Date               Modification                               Name
----               ------------                               ----
2000-05-30         Created                                    Michel Bruant

Hardware Manager functions
--------------------------
stlayer_VSyncCallback
STLAYER_GraphicsTask
stlayer_HardwareManagerInit
stlayer_HardwareManagerTerm
*******************************************************************************/



/* Includes ----------------------------------------------------------------- */
#ifndef ST_OSLINUX
    #include <string.h>
    #include <stdlib.h>
    #include "stlite.h"
#endif
#include "stsys.h"
#include "sttbx.h"
#include "stlayer.h"
#include "hard_mng.h"
#include "stvtg.h"
#include "stevt.h"
#include "back_end.h"



/* Private Variables ------------------------------------------------------------ */


/* Private Constants -------------------------------------------------------- */



/* Private Variables (static)------------------------------------------------ */


/* Private Function prototypes ---------------------------------------------- */

static ST_ErrorCode_t stlayer_TaskTerm(stlayer_XLayerContext_t* LayerContext_p);
static void STLAYER_GraphicsTask(void* Context_p);
static void stlayer_VSyncCallback(STEVT_CallReason_t Reason,
                                const ST_DeviceName_t RegistrantName,
                                STEVT_EventConstant_t Event,
                                void *EventData,
                                void *SubscriberData_p);
static void treatMsg(stlayer_HWManagerMsg_t * Msg_p, STLAYER_Layer_t LayerType);





/* Functions ---------------------------------------------------------------- */

static void treatMsg(stlayer_HWManagerMsg_t * Msg_p, STLAYER_Layer_t LayerType)
{


    #if defined (ST_7109)
        U32 CutId;
    #endif

#if defined (DVD_SECURED_CHIP)&& !defined(STLAYER_NO_STMES)
    unsigned int uMemoryStatus;
#endif


   #if defined (ST_7109)
    CutId = ST_GetCutRevision();
   #endif



    if(LayerType == STLAYER_GAMMA_CURSOR)
    {
#ifdef WA_GNBvd06319
        /* Do not drive the CTL register */
#else
        STSYS_WriteRegDev32LE(&(Msg_p->NodeToAffect_p->CurNode.CTL),
                        Msg_p->NodeValue.CurNode.CTL);
#endif
        STSYS_WriteRegDev32LE(&(Msg_p->NodeToAffect_p->CurNode.VPO),
                        Msg_p->NodeValue.CurNode.VPO);
        STSYS_WriteRegDev32LE(&(Msg_p->NodeToAffect_p->CurNode.PML),
                        Msg_p->NodeValue.CurNode.PML);
        STSYS_WriteRegDev32LE(&(Msg_p->NodeToAffect_p->CurNode.PMP),
                        Msg_p->NodeValue.CurNode.PMP);
        STSYS_WriteRegDev32LE(&(Msg_p->NodeToAffect_p->CurNode.SIZE),
                        Msg_p->NodeValue.CurNode.SIZE);
        STSYS_WriteRegDev32LE(&(Msg_p->NodeToAffect_p->CurNode.CML),
                        Msg_p->NodeValue.CurNode.CML);

        STSYS_WriteRegDev32LE(&(Msg_p->NodeToAffect_p->CurNode.AWS),
                        Msg_p->NodeValue.CurNode.AWS);
        STSYS_WriteRegDev32LE(&(Msg_p->NodeToAffect_p->CurNode.AWE),
                        Msg_p->NodeValue.CurNode.AWE);
    }
    else /* LayerType == STLAYER_GAMMA_BKL or GDP */
    {

        if(Msg_p->NodeValue.BklNode.NVN == NON_VALID_REG)
        {
            STSYS_WriteRegMem32LE(&(Msg_p->NodeToAffect_p->BklNode.CTL),
                            Msg_p->NodeValue.BklNode.CTL);
            STSYS_WriteRegMem32LE(&(Msg_p->NodeToAffect_p->BklNode.AGC),
                            Msg_p->NodeValue.BklNode.AGC);
            STSYS_WriteRegMem32LE(&(Msg_p->NodeToAffect_p->BklNode.HSRC),
                            Msg_p->NodeValue.BklNode.HSRC);
            STSYS_WriteRegMem32LE(&(Msg_p->NodeToAffect_p->BklNode.VPO),
                            Msg_p->NodeValue.BklNode.VPO);
            STSYS_WriteRegMem32LE(&(Msg_p->NodeToAffect_p->BklNode.VPS),
                            Msg_p->NodeValue.BklNode.VPS);\
            STSYS_WriteRegMem32LE(&(Msg_p->NodeToAffect_p->BklNode.PML),
                            Msg_p->NodeValue.BklNode.PML);
            STSYS_WriteRegMem32LE(&(Msg_p->NodeToAffect_p->BklNode.PMP),
                            Msg_p->NodeValue.BklNode.PMP);
            STSYS_WriteRegMem32LE(&(Msg_p->NodeToAffect_p->BklNode.SIZE),
                            Msg_p->NodeValue.BklNode.SIZE);
            STSYS_WriteRegMem32LE(&(Msg_p->NodeToAffect_p->BklNode.VSRC),
                            Msg_p->NodeValue.BklNode.VSRC);
            STSYS_WriteRegMem32LE(&(Msg_p->NodeToAffect_p->BklNode.KEY1),
                            Msg_p->NodeValue.BklNode.KEY1);
            STSYS_WriteRegMem32LE(&(Msg_p->NodeToAffect_p->BklNode.KEY2),
                            Msg_p->NodeValue.BklNode.KEY2);
            STSYS_WriteRegMem32LE(&(Msg_p->NodeToAffect_p->BklNode.HFP),
                            Msg_p->NodeValue.BklNode.HFP);
            STSYS_WriteRegMem32LE(&(Msg_p->NodeToAffect_p->BklNode.PPT),
                            Msg_p->NodeValue.BklNode.PPT);


#if defined (ST_7109) || defined (ST_7200)

       #if defined (ST_7109)

	   if (CutId >= 0xC0)
	     {
	   #endif

            STSYS_WriteRegMem32LE(&(Msg_p->NodeToAffect_p->BklNode.VFP),
                            Msg_p->NodeValue.BklNode.VFP);
            STSYS_WriteRegMem32LE(&(Msg_p->NodeToAffect_p->BklNode.CML),
                            Msg_p->NodeValue.BklNode.CML);
	    #if defined (ST_7109)
	      }
	    #endif

#endif

        }
        else
        {
#if defined (DVD_SECURED_CHIP)&& !defined(STLAYER_NO_STMES)
uMemoryStatus = STMES_IsMemorySecure((void *)Msg_p->NodeValue.BklNode.NVN, (U32)(MAX_GDP_NODE_SIZE), 0);
     if(uMemoryStatus == SECURE_REGION)
                {
            STSYS_WriteRegMem32LE(&(Msg_p->NodeToAffect_p->BklNode.NVN),
                            Msg_p->NodeValue.BklNode.NVN|STLAYER_SECURE_ON);
                }
                else
                 {
            STSYS_WriteRegMem32LE(&(Msg_p->NodeToAffect_p->BklNode.NVN),
                            Msg_p->NodeValue.BklNode.NVN);
                 }

#else
            STSYS_WriteRegMem32LE(&(Msg_p->NodeToAffect_p->BklNode.NVN),
                            Msg_p->NodeValue.BklNode.NVN);
#endif


            if(Msg_p->NodeValue.BklNode.CTL != NON_VALID_REG)
            {
                STSYS_WriteRegMem32LE(&(Msg_p->NodeToAffect_p->BklNode.CTL),
                            Msg_p->NodeValue.BklNode.CTL);
            }
        }
    }
}

/*******************************************************************************
Name        : stlayer_HardwareManagerInit
Description : Initializes the connection to the event handler,
              the message queues,
              the hardware manager task for a particular layer
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t stlayer_HardwareManagerInit(
                                stlayer_XLayerContext_t*  LayerContext_p)
{
    STEVT_OpenParams_t      OpenParams;
    ST_ErrorCode_t          Error;
    char                    TaskName[STLAYER_MAX_HWTASK_NAME]
                                                = "STLAYER-GFX/CUR - \0";

    LayerContext_p->TaskTerminate = FALSE;

    /* semaphore initialization */
    LayerContext_p->VSyncSemaphore_p =  STOS_SemaphoreCreateFifo(NULL,0);

    if (LayerContext_p->VSyncSemaphore_p==NULL)
    {
        return(ST_ERROR_NO_MEMORY);
    }

    layergfx_Trace( ("STLAYER - GFX : Create task GDP-BKL-CUR") );
    /* top field message queue */
    LayerContext_p->MsgQueueTop_p = STOS_MessageQueueCreate(sizeof(stlayer_HWManagerMsg_t), STLAYER_MSG_QUEUE_SIZE);
    if(LayerContext_p->MsgQueueTop_p == NULL)
    {
        layergfx_Defense(ST_ERROR_NO_MEMORY);
        return(ST_ERROR_NO_MEMORY);
    }


    /* bottom field message queue */

    LayerContext_p->MsgQueueBot_p = STOS_MessageQueueCreate(sizeof(stlayer_HWManagerMsg_t), STLAYER_MSG_QUEUE_SIZE);
    if(LayerContext_p->MsgQueueBot_p == NULL)
    {
        layergfx_Defense(ST_ERROR_NO_MEMORY);
        return(ST_ERROR_NO_MEMORY);
    }

    /* prepare the task name */
    strcat(TaskName, LayerContext_p->DeviceName); /* completion of taskname */

    /* Create the task */
    Error = STOS_TaskCreate ((void (*) (void*))STLAYER_GraphicsTask,
                        (void*)LayerContext_p,
                        LayerContext_p->CPUPartition_p,
                        STLAYER_TASK_STACK_SIZE,
                        &(LayerContext_p->TaskStack),
                        LayerContext_p->CPUPartition_p,
                        &(LayerContext_p->Task_p),
                        &(LayerContext_p->TaskDesc),
                        STLAYER_GAMMAGFX_TASK_PRIORITY,
                        TaskName,
                        0 /*task_flags_no_min_stack_size*/);

    if(Error != ST_NO_ERROR)

    {
        layergfx_Defense(ST_ERROR_NO_MEMORY);
        return(ST_ERROR_NO_MEMORY);
    }



    /* opening of the named event handler */

   Error = STEVT_Open(LayerContext_p->EvtHandlerName,
                     &OpenParams,
                     &(LayerContext_p->EvtHandle));

    if(Error != ST_NO_ERROR)
    {
        layergfx_Defense(Error);
        return(Error);
    }
    Error = STEVT_RegisterDeviceEvent(LayerContext_p->EvtHandle,
                                LayerContext_p->DeviceName,
                                STLAYER_UPDATE_PARAMS_EVT,
                                &LayerContext_p->UpdateParamEvtIdent);
    if(Error != ST_NO_ERROR)
    {
        layergfx_Defense(Error);
        return(Error);
    }

    /* subscription to the vtg event will be done when */
    /* calling STLAYER_UpdateFromMixer -> stlayer_VTGSynchro */

    return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : stlayer_HardwareManagerTerm
Description : terminates the hardware manager for a named layer
Parameters  : LayerContext_p: pointer to the layer descriptor structure
Assumptions : the layewr is already initialized
Limitations :
Returns     : error if something went wrong
*******************************************************************************/
ST_ErrorCode_t stlayer_HardwareManagerTerm(
                                    stlayer_XLayerContext_t*  LayerContext_p)
{
    task_t*             Task_p = LayerContext_p->Task_p;
    ST_ErrorCode_t      Err = ST_NO_ERROR;

    /* Termination and deletion of the HW manager task: set the flag */
    LayerContext_p->TaskTerminate = TRUE;

    /* schedule the task and wait for its term */
    STOS_SemaphoreSignal(LayerContext_p->VSyncSemaphore_p);

    STOS_TaskWait( &Task_p, TIMEOUT_INFINITY );

    Err = STOS_TaskDelete ( Task_p,
            LayerContext_p->CPUPartition_p,
            LayerContext_p->TaskStack,
            LayerContext_p->CPUPartition_p );
    if(Err != ST_NO_ERROR)
    {
        STTBX_Print(("Error in call to STOS_TaskDelete()\n\r"));
        return(Err);
    }

    STOS_SemaphoreDelete(NULL,LayerContext_p->VSyncSemaphore_p);

    STEVT_UnregisterDeviceEvent(LayerContext_p->EvtHandle,
                                LayerContext_p->DeviceName,
                                STLAYER_UPDATE_PARAMS_EVT);
    STEVT_Close(LayerContext_p->EvtHandle);

    return(ST_NO_ERROR);
}


/*******************************************************************************
Name        : stlayer_VSyncCallback
Description : layer BKL0 callback function to be executed by STEVT on
              notification of a VSync event
Parameters  : Reason, Event, EventData
Assumptions : STEVT initialized and open
Limitations :
Returns     : Nothing
*******************************************************************************/
static void stlayer_VSyncCallback(STEVT_CallReason_t Reason,
                                const ST_DeviceName_t RegistrantName,
                                STEVT_EventConstant_t Event,
                                void *EventData,
                                void *SubscriberData_p)
{
    STVTG_VSYNC_t             EventType;
    stlayer_XLayerContext_t* LayerContext_p;

    EventType = *((const STVTG_VSYNC_t*)EventData);
    LayerContext_p = (stlayer_XLayerContext_t*)SubscriberData_p;

    UNUSED_PARAMETER(Reason);
    UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(Event);

    /* Set/Reset VTG_TopNotBottom */
    switch (EventType)
    {
        case STVTG_TOP:
           LayerContext_p->VTG_TopNotBottom = TRUE;
        break;
        case STVTG_BOTTOM:
            LayerContext_p->VTG_TopNotBottom = FALSE;
        break;
        default :
        break;
    }

    /*printf("V%c (0x%x)\n", LayerContext_p->VTG_TopNotBottom ? 'T' : 'B', &LayerContext_p->VSyncSemaphore);*/

    STOS_SemaphoreSignal(LayerContext_p->VSyncSemaphore_p);

}

#if defined(ST_OSWINCE) && defined(WINCE_MSTV_ENHANCED)

#define DeviceAdd(Add) ((U32)(Add) \
           + (U32)LayerContext_p->VirtualMapping.PhysicalAddressSeenFromDevice_p \
           - (U32)LayerContext_p->VirtualMapping.PhysicalAddressSeenFromCPU_p)

/*******************************************************************************
Name        : FastHardwareManagerTask
Description : use two flip-flop lists of nodes to do fast update of viewports (especially fast SetViewportSource)
*******************************************************************************/
void FastHardwareManagerTask(stlayer_XLayerContext_t* LayerContext_p)
{
    BOOL flipFlop; /* TRUE: current nodes are in first half, FALSE: current nodes are in second half of the array of nodes */

    /* Initialization */
    flipFlop = TRUE; /* the two pre-filled "service" nodes are in the first half of the array */

    while (!LayerContext_p->TaskTerminate)
    {
        stlayer_Node_t* currentNodeArray; /* the currently displayed nodes */
        stlayer_Node_t* nextNodeArray;    /* the nodes after they've been updated */
        int iNode;
        stlayer_HWManagerMsg_t* Msg_p;    /* an update message from the front-end */
        int receivedTopMessages = 0;
        int receivedBotMessages = 0;
        int waitResult;

        /* 0. Compute position of current and next nodes in the array of nodes */
        if (flipFlop)
        {
            currentNodeArray = LayerContext_p->NodeArray;
            nextNodeArray = LayerContext_p->NodeArray + 2 * (LayerContext_p->MaxViewPorts + 1);
        }
        else
        {
            nextNodeArray = LayerContext_p->NodeArray;
            currentNodeArray = LayerContext_p->NodeArray + 2 * (LayerContext_p->MaxViewPorts + 1);
        }

        /* 1. Copy the current nodes into the next ones */
        for (iNode = 0; iNode < 2 * (LayerContext_p->MaxViewPorts + 1); iNode++)
        {
            stlayer_Node_t* currentNode = currentNodeArray + iNode;
            stlayer_Node_t* nextNode    = nextNodeArray    + iNode;

            *nextNode = *currentNode;
            if (nextNode->BklNode.NVN != 0)
                nextNode->BklNode.NVN += (U32)nextNodeArray - (U32)currentNodeArray;
        }

        /* 2. Apply all outstanding changes to the next nodes */

        /* 2a. Top messages: wait for first one, then immediately receive all */
        while (!LayerContext_p->TaskTerminate)
        {
            clock_t timeout = time_plus(time_now(), time_ticks_per_sec() / 10); /* .1 second */
            Msg_p = (stlayer_HWManagerMsg_t*) STOS_MessageQueueReceiveTimeout(LayerContext_p->MsgQueueTop_p, receivedTopMessages == 0 ? &timeout : TIMEOUT_IMMEDIATE);
            if (Msg_p == NULL) /* timeout */
            {
                if (receivedTopMessages == 0)
                    continue; /* waiting indefinitely for first message */
                else
                    break; /* no more message */
            }
            ++ receivedTopMessages; /* got one */

            /* tweak message so that it manages the next nodes */
            if (flipFlop) /* the next nodes are in the second half of the array */
            {
                Msg_p->NodeToAffect_p += nextNodeArray - currentNodeArray;
                if (Msg_p->NodeValue.BklNode.NVN != NON_VALID_REG)
                    Msg_p->NodeValue.BklNode.NVN += (U32)nextNodeArray - (U32)currentNodeArray;
            }

            /* process message */
            treatMsg(Msg_p, LayerContext_p->LayerType);

            /* release message */
            STOS_MessageQueueRelease(LayerContext_p->MsgQueueTop_p, (void*) Msg_p);
        } /* end of processing of top messages */

        /* 2b. Bottom messages : receive the same number as the top ones */
        while (!LayerContext_p->TaskTerminate && receivedBotMessages < receivedTopMessages)
        {
            clock_t timeout = time_plus(time_now(), time_ticks_per_sec() / 10); /* .1 second */
            Msg_p = (stlayer_HWManagerMsg_t*) STOS_MessageQueueReceiveTimeout(LayerContext_p->MsgQueueBot_p, &timeout);
            if (Msg_p == NULL) /* timeout */
                    continue; /* waiting indefinitely for correct amount of messages */
            ++ receivedBotMessages; /* got one */

            /* tweak message */
            if (flipFlop) /* the next nodes are in the second half of the array */
            {
                Msg_p->NodeToAffect_p += nextNodeArray - currentNodeArray;
                if (Msg_p->NodeValue.BklNode.NVN != NON_VALID_REG)
                    Msg_p->NodeValue.BklNode.NVN += (U32)nextNodeArray - (U32)currentNodeArray;
            }

            /* process message */
            treatMsg(Msg_p, LayerContext_p->LayerType);

            /* release message */
            STOS_MessageQueueRelease(LayerContext_p->MsgQueueBot_p, (void*) Msg_p);
        } /* end of processing of bottom messages */

        /* 3. Switch hardware to new array of nodes: let the NVN of all current nodes point to the next nodes */
        for (iNode = 0; iNode < 2 * (LayerContext_p->MaxViewPorts + 1); iNode++)
        {
            stlayer_Node_t* currentNode = currentNodeArray + iNode;
            stlayer_Node_t* nextNode    = nextNodeArray + iNode;
            if (flipFlop)
                currentNode->BklNode.NVN += (U32)nextNode - (U32)currentNode;
            else
                currentNode->BklNode.NVN -= (U32)currentNode - (U32)nextNode; // avoid negative arithmetic on U32 numbers
        }

        /* 4. Wait for next top vsync */

        /* 4.a Reset semaphore (it is a counting semaphore so it is increased by 1 each time it is signalled) */
        while (STOS_SemaphoreWaitTimeOut(LayerContext_p->VSyncSemaphore_p, TIMEOUT_IMMEDIATE) == OS21_SUCCESS)
            ;

        /* 4.b Wait for next signal */
        while (1)
        {
            if (LayerContext_p->TaskTerminate)
                break;
            STOS_SemaphoreWait(LayerContext_p->VSyncSemaphore_p);
            if (LayerContext_p->VTG_TopNotBottom)
                break;
        }

        /* 5. Switch to new array of nodes */
        flipFlop = !flipFlop;
    } /* end of while (!terminate) */
}
#endif /* ST_OSWINCE && WINCE_MSTV_ENHANCED */

/*******************************************************************************
Name        :STLAYER_GraphicsTask
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void STLAYER_GraphicsTask(void* Context_p)
{
    stlayer_XLayerContext_t*    LayerContext_p;
    stlayer_HWManagerMsg_t*     Msg_p;


    STOS_TaskEnter(NULL);

    /* initialize the context pointer */
    LayerContext_p = (stlayer_XLayerContext_t*) Context_p;
#if defined(ST_OSWINCE) && defined(WINCE_MSTV_ENHANCED)
    /* use fast update version for GDP layers */
    if (LayerContext_p->LayerType == STLAYER_GAMMA_GDP)
        FastHardwareManagerTask(LayerContext_p);
    else
    {
#endif

    /* task loop */
    while (LayerContext_p->TaskTerminate == FALSE)
    {
        /* wait for schedule sig */
        STOS_SemaphoreWait(LayerContext_p->VSyncSemaphore_p);
        if(LayerContext_p->LayerParams.ScanType == STGXOBJ_PROGRESSIVE_SCAN)
        {
            /* Simulate alternate Top-Bot */
            if (LayerContext_p->VTG_TopNotBottomSimulation)
            {
                LayerContext_p->VTG_TopNotBottomSimulation = FALSE;
            }
            else
            {
                LayerContext_p->VTG_TopNotBottomSimulation = TRUE;
            }
            LayerContext_p->VTG_TopNotBottom =
                            LayerContext_p->VTG_TopNotBottomSimulation;
        }

        if(LayerContext_p->VTG_TopNotBottom)
        {

            Msg_p = (stlayer_HWManagerMsg_t*)
                STOS_MessageQueueReceiveTimeout((LayerContext_p->MsgQueueBot_p),
                                                    TIMEOUT_IMMEDIATE);


            while(Msg_p != NULL)
            {
                if(LayerContext_p->LayerType != STLAYER_GAMMA_CURSOR)
                {
                    treatMsg(Msg_p,LayerContext_p->LayerType);
                }
                STOS_MessageQueueRelease((LayerContext_p->MsgQueueBot_p),(void*) Msg_p);
                Msg_p = (stlayer_HWManagerMsg_t*)
                    STOS_MessageQueueReceiveTimeout((LayerContext_p->MsgQueueBot_p),
                                            TIMEOUT_IMMEDIATE);
            }
        }
        else /* Bot vsync received:  update top stuff */
        {
            Msg_p = (stlayer_HWManagerMsg_t*)
                STOS_MessageQueueReceiveTimeout(LayerContext_p->MsgQueueTop_p,
                                        TIMEOUT_IMMEDIATE);
            while(Msg_p != NULL)
            {
                treatMsg(Msg_p,LayerContext_p->LayerType);

                STOS_MessageQueueRelease(LayerContext_p->MsgQueueTop_p,(void*) Msg_p);

                Msg_p = (stlayer_HWManagerMsg_t*)
                    STOS_MessageQueueReceiveTimeout((LayerContext_p->MsgQueueTop_p),
                                            TIMEOUT_IMMEDIATE);
            }
        }
    }/* task loop */
#if defined(ST_OSWINCE) && defined(WINCE_MSTV_ENHANCED)
    }
#endif

    /*  Task Term */
    stlayer_TaskTerm(LayerContext_p);

    STOS_TaskExit(NULL);

} /* task return */

/*******************************************************************************
Name        : stlayer_TaskTerm
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t stlayer_TaskTerm(stlayer_XLayerContext_t * LayerContext_p)
{
    stlayer_HWManagerMsg_t*   Msg_p;
    ST_ErrorCode_t            Error;
    ST_DeviceName_t Empty = "\0\0";

    /* unsubscribe to the handled VTG_VSYNC event */
    if(strcmp(LayerContext_p->VTGName,Empty) != 0 )
    {
        /* A synchro was already in effect */
        /* -> No more synchro */
        Error = STEVT_UnsubscribeDeviceEvent(LayerContext_p->EvtHandle,
                                     LayerContext_p->VTGName,
                                     STVTG_VSYNC_EVT);
        if(Error != ST_NO_ERROR)
        {
            layergfx_Defense(Error);
            return(Error);
        }
    }

    /* pull all messages in queues */
    Msg_p = (stlayer_HWManagerMsg_t*)
               STOS_MessageQueueReceiveTimeout((LayerContext_p->MsgQueueTop_p),
                                        TIMEOUT_IMMEDIATE);

    while(Msg_p != NULL)
    {
        treatMsg(Msg_p,LayerContext_p->LayerType);
        STOS_MessageQueueRelease((LayerContext_p->MsgQueueTop_p),(void*) Msg_p);
        Msg_p = (stlayer_HWManagerMsg_t*)
                STOS_MessageQueueReceiveTimeout((LayerContext_p->MsgQueueTop_p),
                                        TIMEOUT_IMMEDIATE);
    }

    Msg_p = (stlayer_HWManagerMsg_t*)
                    STOS_MessageQueueReceiveTimeout((LayerContext_p->MsgQueueBot_p),
                                            TIMEOUT_IMMEDIATE);
    while(Msg_p != NULL)
    {
        if(LayerContext_p->LayerType != STLAYER_GAMMA_CURSOR)
        {
            treatMsg(Msg_p,LayerContext_p->LayerType);
        }
        STOS_MessageQueueRelease((LayerContext_p->MsgQueueBot_p),(void*) Msg_p);
        Msg_p = (stlayer_HWManagerMsg_t*)
                    STOS_MessageQueueReceiveTimeout((LayerContext_p->MsgQueueBot_p),
                                            TIMEOUT_IMMEDIATE);
    }

    /* HW Manager Message queues deletion */

    STOS_MessageQueueDelete(LayerContext_p->MsgQueueTop_p);
    STOS_MessageQueueDelete(LayerContext_p->MsgQueueBot_p);
#if defined(ST_OS20) || defined(ST_OS21)
    STOS_MemoryDeallocate(LayerContext_p->CPUPartition_p,
                    LayerContext_p->MsgQueueBufferTop_p);
    STOS_MemoryDeallocate(LayerContext_p->CPUPartition_p,
                     LayerContext_p->MsgQueueBufferBot_p);
#endif

    return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : stlayer_VTGSynchro
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t stlayer_VTGSynchro(stlayer_XLayerContext_t * LayerContext_p,
                        ST_DeviceName_t NewVTG)
{
    STEVT_DeviceSubscribeParams_t SubscribeParams;
    ST_DeviceName_t Empty = "\0\0";
    STVTG_VSYNC_t   Top         = STVTG_TOP;
    STVTG_VSYNC_t   Bot         = STVTG_BOTTOM;
    ST_ErrorCode_t  Error;

    if(strcmp(LayerContext_p->VTGName,Empty) != 0 )
    {
        /* A synchro was already in effect */
        /* -> No more synchro */
        STEVT_UnsubscribeDeviceEvent(LayerContext_p->EvtHandle,
                                     LayerContext_p->VTGName,
                                     STVTG_VSYNC_EVT);
    }
    if(NewVTG != NULL)
    {
        /* subscription to the event handler */
        SubscribeParams.NotifyCallback     = (STEVT_DeviceCallbackProc_t)stlayer_VSyncCallback;
        SubscribeParams.SubscriberData_p   = (void*)LayerContext_p;
        Error = STEVT_SubscribeDeviceEvent(LayerContext_p->EvtHandle,
                                   NewVTG,
                                   STVTG_VSYNC_EVT,
                                   &SubscribeParams);
        if (Error == ST_NO_ERROR)
        {
            layergfx_Trace("STLAYER - GFX : Synchro Vsync is OK");
            strncpy(LayerContext_p->VTGName, NewVTG, sizeof(ST_DeviceName_t)-1);
        }
        else
        {
            layergfx_Trace("STLAYER - GFX : Synchro KO: layer not synchro");
            layergfx_Defense(Error);
            return (Error /* error to be defined */);
        }
    }
    else
    {
        /* No new VTG */
        strncpy(LayerContext_p->VTGName, Empty, sizeof(ST_DeviceName_t)-1);
        /* Schedule the task via its callback to pull all pending messages */
        stlayer_VSyncCallback(0,LayerContext_p->VTGName,0,&Top,LayerContext_p);
        stlayer_VSyncCallback(0,LayerContext_p->VTGName,0,&Bot,LayerContext_p);
    }

    return(ST_NO_ERROR);
}

/* End of hard_mng.c */
