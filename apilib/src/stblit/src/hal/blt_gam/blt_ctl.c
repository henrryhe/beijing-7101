/*******************************************************************************

File name   : blt_ctl.c

Description :  back-end control source file

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
16 Jun 2000        Created                                           TM
******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#include "blt_be.h"
#include "blt_init.h"
#include "stblit.h"
#include "blt_com.h"
#include "blt_ctl.h"
#include "blt_job.h"
#include "blt_pool.h"
#include "blt_mem.h"

#ifdef STBLIT_EMULATOR
#include "blt_emu.h"
#endif

/* Private Types ------------------------------------------------------------ */


/* Private Constants -------------------------------------------------------- */


/* Private Variables (static)------------------------------------------------ */


/* Global Variables --------------------------------------------------------- */


/* Private Macros ----------------------------------------------------------- */


/* Private Function prototypes ---------------------------------------------- */


/* Functions ---------------------------------------------------------------- */

/*******************************************************************************
Name        : stblit_PostSubmitMessage
Description : Post SUBMIT Message in the High/Low Priority queue
Parameters  : The structure associated with the device , the address of the first
              node of the list to insert and the last node of the list to submit.
Assumptions :  Valid params (non null pointer)
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t stblit_PostSubmitMessage(stblit_Device_t * const Device_p,
                                        void* FirstNode_p, void* LastNode_p,
                                        BOOL InsertAtFront,
                                        BOOL LockBeforeFirstNode)
 {
    ST_ErrorCode_t  Err               = ST_NO_ERROR;
    stblit_Msg_t*   Msg_p;

#ifdef STBLIT_EMULATOR
    U32             DeviceNodeAddress = (U32)stblit_CpuToDevice((void*)FirstNode_p, &(Device_p->VirtualMapping));
    U32             CpuNodeAddress    = (U32)FirstNode_p;
    stblit_Node_t*  Node_p;
    BOOL            EnaBlitCompletedIRQ;

    if (Device_p->DeviceType == STBLIT_DEVICE_TYPE_SOFTWARE)
    {
        /* Set NIP of last node to 0 */
        STSYS_WriteRegMem32LE((void*)(&(((stblit_Node_t*)(LastNode_p))->HWNode.BLT_NIP)),0);

        /* In the case of the software implementation, call the emulator synchronously */
        while (DeviceNodeAddress != 0)
        {

            Node_p = (stblit_Node_t*) CpuNodeAddress;

            /* Process Cpu Node, and get Next node device address + Blit completion IT info */
            DeviceNodeAddress = stblit_EmulatorProcessNode(Device_p, CpuNodeAddress,&EnaBlitCompletedIRQ);

            /* Get Next node Cpu address */
            CpuNodeAddress = (U32)stblit_DeviceToCpu((void*)DeviceNodeAddress, &(Device_p->VirtualMapping));


            /* Notify events */
            if (Node_p->ITOpcode & STBLIT_IT_OPCODE_BLIT_COMPLETION_MASK)
            {
                U32 UserTag = STSYS_ReadRegMem32LE((void*)(&(Node_p->HWNode.BLT_USR)));

                STEVT_NotifySubscriber(Device_p->EvtHandle,Device_p->EventID[STBLIT_BLIT_COMPLETED_ID],
                                      (void*)UserTag,Node_p->SubscriberID);
            }

            if ((Node_p->ITOpcode & STBLIT_IT_OPCODE_JOB_COMPLETION_MASK) && ((void*)Node_p->JobHandle != NULL))
            {
                STEVT_NotifySubscriber(Device_p->EvtHandle,Device_p->EventID[STBLIT_JOB_COMPLETED_ID],
                                       (void*)Node_p->JobHandle,((stblit_Job_t*)Node_p->JobHandle)->SubscriberID);
            }

            /* Free up the node for later reuse */
            stblit_ReleaseNode(Device_p, (stblit_NodeHandle_t)Node_p);
        }
    }
    else /* All other device types are working asynchronously */
    {
#endif
        if (InsertAtFront == TRUE)
        {
            Msg_p = (stblit_Msg_t*) STOS_MessageQueueClaimTimeout(Device_p->HighPriorityQueue, TIMEOUT_INFINITY);

            if (LockBeforeFirstNode == FALSE)
            {
                Msg_p->MessageID = SUBMIT;
            }
            else
            {
                Msg_p->MessageID = SUBMIT_LOCK;
            }
            Msg_p->Data1_p  = FirstNode_p;
            Msg_p->Data2_p  = LastNode_p;

            STOS_MessageQueueSend(Device_p->HighPriorityQueue, (void*)Msg_p);
        }
        else
        {
            Msg_p = (stblit_Msg_t*) STOS_MessageQueueClaim(Device_p->LowPriorityQueue);

            if (LockBeforeFirstNode == FALSE)
            {
                Msg_p->MessageID = SUBMIT;
            }
            else
            {
                Msg_p->MessageID = SUBMIT_LOCK;
            }
            Msg_p->Data1_p  = FirstNode_p;
            Msg_p->Data2_p  = LastNode_p;

            STOS_MessageQueueSend(Device_p->LowPriorityQueue, (void*)Msg_p);
        }

#ifdef STBLIT_EMULATOR
    }
#endif

    return(Err);
 }

/*
--------------------------------------------------------------------------------
Lock API function
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STBLIT_Lock(STBLIT_Handle_t  Handle, STBLIT_JobHandle_t JobHandle)
{
    ST_ErrorCode_t   Err      = ST_NO_ERROR;
    stblit_Msg_t*    Msg_p;
    stblit_Unit_t*   Unit_p   = (stblit_Unit_t*) Handle;
    stblit_Device_t* Device_p;
    stblit_Job_t*    Job_p    = (stblit_Job_t*) JobHandle;


    /* Check Handle validity */
    if (Handle == (STBLIT_Handle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    Device_p = Unit_p->Device_p ;

    if (Unit_p->UnitValidity != STBLIT_VALID_UNIT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    if (JobHandle == STBLIT_NO_JOB_HANDLE)  /* Single blit related*/
    {
        /* Start proctection for access control*/
        STOS_SemaphoreWait(Device_p->AccessControl);

        if (Device_p->LowPriorityLockSent == FALSE)
        {
            Msg_p = (stblit_Msg_t*) STOS_MessageQueueClaim(Device_p->LowPriorityQueue);
            Msg_p->MessageID = LOCK;
            STOS_MessageQueueSend(Device_p->LowPriorityQueue, (void*)Msg_p);

            Device_p->LowPriorityLockSent = TRUE;
        }

        /* Stop protection */
        STOS_SemaphoreSignal(Device_p->AccessControl);
    }
    else
    {
        /* Check Job Validity */
        if (Job_p->JobValidity != STBLIT_VALID_JOB)
        {
            return(STBLIT_ERROR_INVALID_JOB_HANDLE);
        }

        /* Check Handles mismatch */
        if (Job_p->Handle != Handle)
        {
            return(STBLIT_ERROR_JOB_HANDLE_MISMATCH);
        }

        /* Check if Job is not closed */
        if (Job_p->Closed == TRUE)
        {
            /* Job already submitted. No change permitted */
            return(STBLIT_ERROR_JOB_CLOSED);
        }

        /* Lock only if no pending lock */
        if (Job_p->PendingLock == FALSE)
        {
            Err = stblit_AddLockInJob(JobHandle);
        }
    }
    return(Err);
}

/*
--------------------------------------------------------------------------------
Unlock API function
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STBLIT_Unlock(STBLIT_Handle_t  Handle, STBLIT_JobHandle_t JobHandle)
{
    ST_ErrorCode_t   Err      = ST_NO_ERROR;
    stblit_Msg_t*    Msg_p;
    stblit_Unit_t*   Unit_p   = (stblit_Unit_t*) Handle;
    stblit_Device_t* Device_p;
    stblit_Job_t*    Job_p    = (stblit_Job_t*) JobHandle;


    /* Check Handle validity */
    if (Handle == (STBLIT_Handle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    Device_p = Unit_p->Device_p ;

    if (Unit_p->UnitValidity != STBLIT_VALID_UNIT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    if (JobHandle == STBLIT_NO_JOB_HANDLE)  /* Single blit related*/
    {
        /* Start proctection for access control*/
        STOS_SemaphoreWait(Device_p->AccessControl);

        if (Device_p->LowPriorityLockSent == TRUE)
        {

            Msg_p = (stblit_Msg_t*) STOS_MessageQueueClaim(Device_p->LowPriorityQueue);
            Msg_p->MessageID = UNLOCK;
            STOS_MessageQueueSend(Device_p->LowPriorityQueue, (void*)Msg_p);

            Device_p->LowPriorityLockSent = FALSE;
        }

        /* Stop protection */
        STOS_SemaphoreSignal(Device_p->AccessControl);
    }
    else
    {
        /* Check Job Validity */
        if (Job_p->JobValidity != STBLIT_VALID_JOB)
        {
            return(STBLIT_ERROR_INVALID_JOB_HANDLE);
        }

        /* Check Handles mismatch */
        if (Job_p->Handle != Handle)
        {
            return(STBLIT_ERROR_JOB_HANDLE_MISMATCH);
        }

        /* Check if Job is not closed */
        if (Job_p->Closed == TRUE)
        {
            /* Job already submitted. No change permitted */
            return(STBLIT_ERROR_JOB_CLOSED);
        }

        /* Lock only if no pending lock */
        if (Job_p->PendingLock == TRUE)
        {
            Err = stblit_AddUnlockInJob(JobHandle);
        }
    }

    return(Err);
}


/*
--------------------------------------------------------------------------------
Submit Job at Front API function
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STBLIT_SubmitJobFront(STBLIT_Handle_t  Handle, STBLIT_JobHandle_t JobHandle)
{
    ST_ErrorCode_t      Err      = ST_NO_ERROR;
    stblit_Unit_t*      Unit_p   = (stblit_Unit_t*) Handle;
    stblit_Device_t*    Device_p ;
    stblit_Job_t*       Job_p    = (stblit_Job_t*) JobHandle;
    stblit_Node_t*      FirstNode_p;
    stblit_Node_t*      LastNode_p;
    stblit_NodeHandle_t  NodeHandle, LeftHandle, RightHandle, CurrentNodeHandle;

    /* Check Handle validity */
    if (Handle == (STBLIT_Handle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    Device_p = Unit_p->Device_p ;

    if (Unit_p->UnitValidity != STBLIT_VALID_UNIT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    /* Check Job Validity */
    if (JobHandle == STBLIT_NO_JOB_HANDLE)
    {
        return(STBLIT_ERROR_INVALID_JOB_HANDLE);
    }
    if (Job_p->JobValidity != STBLIT_VALID_JOB)
    {
        return(STBLIT_ERROR_INVALID_JOB_HANDLE);
    }
    /* Check Handles mismatch */
    if (Job_p->Handle != Handle)
    {
        return(STBLIT_ERROR_JOB_HANDLE_MISMATCH);
    }
    /* Check if Job empty */
    if (Job_p->NbNodes == 0)
    {
        return(STBLIT_ERROR_JOB_EMPTY);
    }

    /* Check if lock pending */
    if (Job_p->PendingLock == TRUE)
    {
        return(STBLIT_ERROR_JOB_PENDING_LOCK);
    }

    /* The original Job is not submitted to the hw. It is in fact its nodes copy which is submitted */
    /* Start proctection for pool access control*/
    STOS_SemaphoreWait(Device_p->AccessControl);

    /* Make sure there is enough nodes in the pool for the copy */
    if (Job_p->NbNodes > Device_p->JBlitNodeDataPool.NbFreeElem)
    {
        /* Stop protection */
        STOS_SemaphoreSignal(Device_p->AccessControl);

        return(STBLIT_ERROR_MAX_JOB_NODE);
    }

    if(Job_p->NbNodes == 1)
    {
		U32 TempAddress;

		/* Added to rmove compilation warnning (dereferencing type-punned pointer will break strict-aliasing rules) */
		TempAddress = (U32)&NodeHandle;

		/* Get a Node */
        stblit_GetElement(&(Device_p->JBlitNodeDataPool), (void**) TempAddress);

        /* Copy The node */
        *((stblit_Node_t*)NodeHandle) =   *((stblit_Node_t*)Job_p->FirstNodeHandle);

        FirstNode_p = (stblit_Node_t*)NodeHandle;
        LastNode_p = (stblit_Node_t*) NodeHandle;
    }
    else /* At least 2 nodes */
    {
		U32 TempAddress;

		/* Added to rmove compilation warnning (dereferencing type-punned pointer will break strict-aliasing rules) */
		TempAddress = (U32)&RightHandle;

		/* Get a Node */
        stblit_GetElement(&(Device_p->JBlitNodeDataPool), (void**) TempAddress);
        /* Copy The node */
        *((stblit_Node_t*)RightHandle) =  *((stblit_Node_t*)Job_p->LastNodeHandle);

        LastNode_p = (stblit_Node_t*)RightHandle;
        CurrentNodeHandle = Job_p->LastNodeHandle;

        while(CurrentNodeHandle != Job_p->FirstNodeHandle) /* There is at least 1 node on the left side of the current node */
        {
			/* Added to rmove compilation warnning (dereferencing type-punned pointer will break strict-aliasing rules) */
			TempAddress = (U32)&LeftHandle;

			/* Get a Node */
            stblit_GetElement(&(Device_p->JBlitNodeDataPool), (void**) TempAddress);
            /* Copy The node */
            *((stblit_Node_t*)LeftHandle) =  *((stblit_Node_t*)(((stblit_Node_t*)CurrentNodeHandle)->PreviousNode));

            stblit_Connect2Nodes(Device_p,(stblit_Node_t*)LeftHandle,(stblit_Node_t*) RightHandle);

            CurrentNodeHandle = ((stblit_Node_t*)CurrentNodeHandle)->PreviousNode;
            RightHandle = LeftHandle;
        }
        FirstNode_p = (stblit_Node_t*)LeftHandle;
    }

    /* Stop proctection */
    STOS_SemaphoreSignal(Device_p->AccessControl);

    Job_p->Closed = TRUE;

    /* Post Message */
    #ifndef STBLIT_TEST_FRONTEND
    stblit_PostSubmitMessage(Unit_p->Device_p, (void*)FirstNode_p, (void*)LastNode_p, TRUE, Job_p->LockBeforeFirstNode);
    #endif

    return(Err);
}

/*
--------------------------------------------------------------------------------
Submit Job at Back API function
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STBLIT_SubmitJobBack(STBLIT_Handle_t  Handle, STBLIT_JobHandle_t JobHandle)
{
    ST_ErrorCode_t      Err      = ST_NO_ERROR;
    stblit_Unit_t*      Unit_p   = (stblit_Unit_t*) Handle;
    stblit_Device_t*    Device_p;
    stblit_Job_t*       Job_p    = (stblit_Job_t*) JobHandle;
    stblit_Node_t*      FirstNode_p;
    stblit_Node_t*      LastNode_p;
    stblit_NodeHandle_t  NodeHandle, LeftHandle, RightHandle, CurrentNodeHandle;

    /* Check Handle validity */
    if (Handle == (STBLIT_Handle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    Device_p = Unit_p->Device_p ;

    if (Unit_p->UnitValidity != STBLIT_VALID_UNIT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Check Job Validity */
    if (JobHandle == STBLIT_NO_JOB_HANDLE)
    {
        return(STBLIT_ERROR_INVALID_JOB_HANDLE);
    }
    if (Job_p->JobValidity != STBLIT_VALID_JOB)
    {
        return(STBLIT_ERROR_INVALID_JOB_HANDLE);
    }

    /* Check Handles mismatch */
    if (Job_p->Handle != Handle)
    {
        return(STBLIT_ERROR_JOB_HANDLE_MISMATCH);
    }

    /* Check if Job empty */
    if (Job_p->NbNodes == 0)
    {
        return(STBLIT_ERROR_JOB_EMPTY);
    }

    /* Check if lock pending */
    if (Job_p->PendingLock == TRUE)
    {
        return(STBLIT_ERROR_JOB_PENDING_LOCK);
    }


    /* The original Job is not submitted to the hw. It is in fact its nodes copy which is submitted */

    /* Start proctection for access control*/
    STOS_SemaphoreWait(Device_p->AccessControl);

    /* A job can not be sent into the low priority queue when the queue contains already a lock (and unlock not yet arrived)
     *  => Indeed it will end up with nested lock
     * This problem only concern the low priority queue */
    if ((Device_p->LowPriorityLockSent == TRUE) &&
        (Job_p->UseLockingMechanism == TRUE))
    {
        /* Stop protection */
        STOS_SemaphoreSignal(Device_p->AccessControl);

        return(STBLIT_ERROR_NESTED_LOCK);
    }

    /* Make sure there is enough nodes in the pool for the copy */
    if (Job_p->NbNodes > Device_p->JBlitNodeDataPool.NbFreeElem)
    {
        /* Stop proctection */
        STOS_SemaphoreSignal(Device_p->AccessControl);

        return(STBLIT_ERROR_MAX_JOB_NODE);
    }

    if(Job_p->NbNodes == 1)
    {
		U32 TempAddress;

		/* Added to rmove compilation warnning (dereferencing type-punned pointer will break strict-aliasing rules) */
		TempAddress = (U32)&NodeHandle;

		/* Get a Node */
        stblit_GetElement(&(Device_p->JBlitNodeDataPool), (void**) TempAddress);

        /* Copy The node */
        *((stblit_Node_t*)NodeHandle) =   *((stblit_Node_t*)Job_p->FirstNodeHandle);

        FirstNode_p = (stblit_Node_t*)NodeHandle;
        LastNode_p = (stblit_Node_t*) NodeHandle;
    }
    else /* At least 2 nodes */
    {
		U32 TempAddress;

		/* Added to rmove compilation warnning (dereferencing type-punned pointer will break strict-aliasing rules) */
		TempAddress = (U32)&RightHandle;

        /* Get a Node */
        stblit_GetElement(&(Device_p->JBlitNodeDataPool), (void**) TempAddress);
        /* Copy The node */
        *((stblit_Node_t*)RightHandle) =  *((stblit_Node_t*)Job_p->LastNodeHandle);

        LastNode_p = (stblit_Node_t*)RightHandle;
        CurrentNodeHandle = Job_p->LastNodeHandle;

        while(CurrentNodeHandle != Job_p->FirstNodeHandle) /* There is at least 1 node on the left side of the current node */
        {
			/* Added to rmove compilation warnning (dereferencing type-punned pointer will break strict-aliasing rules) */
			TempAddress = (U32)&LeftHandle;

			/* Get a Node */
            stblit_GetElement(&(Device_p->JBlitNodeDataPool), (void**) TempAddress);
            /* Copy The node */
            *((stblit_Node_t*)LeftHandle) =  *((stblit_Node_t*)(((stblit_Node_t*)CurrentNodeHandle)->PreviousNode));

            stblit_Connect2Nodes(Device_p,(stblit_Node_t*)LeftHandle,(stblit_Node_t*) RightHandle);

            CurrentNodeHandle = ((stblit_Node_t*)CurrentNodeHandle)->PreviousNode;
            RightHandle = LeftHandle;
        }
        FirstNode_p = (stblit_Node_t*)LeftHandle;
    }

    /* Stop proctection */
    STOS_SemaphoreSignal(Device_p->AccessControl);

    Job_p->Closed = TRUE;

    /* Post Message */
    #ifndef STBLIT_TEST_FRONTEND
    stblit_PostSubmitMessage(Unit_p->Device_p, (void*)FirstNode_p, (void*)LastNode_p, FALSE, Job_p->LockBeforeFirstNode);
    #endif

    return(Err);
}

/* End of blt_ctl.c */
