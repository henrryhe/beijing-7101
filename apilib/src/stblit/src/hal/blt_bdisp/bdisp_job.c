/*******************************************************************************

File name   : blt_job.c

Description : job source file

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
23 Jun 2004        Created                                          HE
30 May 2006        Modified for WinCE platform					    WinCE Team-ST Noida
27 sep 2006        - added implementation of all new APIs			WinCE Team-ST Noida
				   - All changes are under ST_OSWINCE option.
******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#include "stos.h"
#include "stblit.h"
#include "bdisp_init.h"
#include "bdisp_com.h"
#include "bdisp_job.h"
#include "bdisp_mem.h"
#include "bdisp_pool.h"


/* Private Types ------------------------------------------------------------ */


/* Private Constants -------------------------------------------------------- */

/* Private Variables (static)------------------------------------------------ */


/* Global Variables --------------------------------------------------------- */


/* Private Macros ----------------------------------------------------------- */


/* Private Function prototypes ---------------------------------------------- */
static ST_ErrorCode_t AddJobToList(STBLIT_JobHandle_t JobHandle);
static ST_ErrorCode_t RemoveJobFromList(STBLIT_JobHandle_t JobHandle);
static void ReleaseListJBlit(stblit_Device_t* Device_p, STBLIT_JobBlitHandle_t JBlitHandle);


/* Functions ---------------------------------------------------------------- */

/*******************************************************************************
Name        : stblit_ReleaseListJBlit
Description : Being given the first JBlit of the list, it releases all JBlit.
Parameters  :
Assumptions : The NextJBlit field of the Last job Blit in the list is STBLIT_NO_JOB_BLIT_HANDLE
Limitations :
Returns     : Nothing
*******************************************************************************/
static void ReleaseListJBlit(stblit_Device_t* Device_p, STBLIT_JobBlitHandle_t JBlitHandle)
{
    STBLIT_JobBlitHandle_t  NextHandle;

    while (JBlitHandle != STBLIT_NO_JOB_BLIT_HANDLE)
    {
        NextHandle = ((stblit_JobBlit_t*)JBlitHandle)->NextJBlit;

        /* Release current JBlit descriptor */
        ((stblit_JobBlit_t*)JBlitHandle)->JBlitValidity = (U32)~STBLIT_VALID_JOB_BLIT;
        stblit_ReleaseElement(&(Device_p->JBlitDataPool), (void*) JBlitHandle);
        JBlitHandle = NextHandle;
    }
}


/*******************************************************************************
Name        : stblit_DeleteAllJobFromList
Description : Delete all jobs from list of created jobs contained in stblit_Unit_t
              Start from First to Last Job!
Parameters  :
Assumptions : JobHandle != STBLIT_NO_JOB_HANDLE , and non null Unit_p is valid
              The update of Unit_p created Job list must be done at upper level
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t stblit_DeleteAllJobFromList(stblit_Unit_t* Unit_p,STBLIT_JobHandle_t JobHandle)
{
    ST_ErrorCode_t              Err           = ST_NO_ERROR;
    stblit_Device_t*            Device_p      = Unit_p->Device_p;
    STBLIT_JobHandle_t          NextHandle;

    while (JobHandle != STBLIT_NO_JOB_HANDLE)
    {
        NextHandle = ((stblit_Job_t*)JobHandle)->NextJob;

        /* Release Job Nodes */
        if (((stblit_Job_t*)JobHandle)->NbNodes != 0)
        {
            /* Release the full list of nodes within the job (From Last to First) */
            /* Make sure the First Node in the list has got its PreviousNode field set to STBLIT_NO_NODE_HANDLE */
            stblit_ReleaseListNode(Device_p, ((stblit_Job_t*)JobHandle)->LastNodeHandle);
        }

        /* Release Job Blit */
        if (((stblit_Job_t*)JobHandle)->NbJBlit != 0)
        {
            /* Release the full list of Job Blit within the job (From First to Last) */
            /* Make sure the Last Job Blit in the list has got its NextJBlit field set to STBLIT_NO_JOB_BLIT_HANDLE */
            ReleaseListJBlit(Device_p, ((stblit_Job_t*)JobHandle)->FirstJBlitHandle);
        }

        /* Update job descriptor */
        ((stblit_Job_t*)JobHandle)->JobValidity = (U32)~STBLIT_VALID_JOB;

        /* Release Job */
        stblit_ReleaseElement(&(Device_p->JobDataPool), (void*) JobHandle);

        JobHandle = NextHandle;
    }

    return(Err);
}


/*******************************************************************************
Name        : AddJobToList
Description : Add Job in list of created jobs contained in stblit_Unit_t
Parameters  :
Assumptions : JobHandle valid and non NULL
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t AddJobToList(STBLIT_JobHandle_t JobHandle)
{
    ST_ErrorCode_t      Err         = ST_NO_ERROR;
    stblit_Unit_t*      Unit_p;
    stblit_Job_t*       Job_p  = (stblit_Job_t*) JobHandle;

    Unit_p = (stblit_Unit_t*)(Job_p->Handle);

    /* Update JOB descriptor */
    Job_p->NextJob        = STBLIT_NO_JOB_HANDLE;
    Job_p->PreviousJob    = Unit_p->LastCreatedJob;

    /* Update stblit_unit_t */
    if (Unit_p->NbCreatedJobs == 0)
    {
        Unit_p->FirstCreatedJob = JobHandle;
    }
    else /* LastCreatedJob != STBLIT_NO_JOB_HANDLE */
    {
        ((stblit_Job_t*)(Unit_p->LastCreatedJob))->NextJob = JobHandle;
    }
    (Unit_p->NbCreatedJobs)++;
    Unit_p->LastCreatedJob = JobHandle;

    return(Err);
}

/*******************************************************************************
Name        : RemoveJobFromList
Description : Remove JobHandle in list of created job contained in stblit_Unit_t
Parameters  :
Assumptions : Job is in list and only once!
              JobHandle non null and valid
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t RemoveJobFromList(STBLIT_JobHandle_t JobHandle)
{
    ST_ErrorCode_t          Err                 = ST_NO_ERROR;
    stblit_Unit_t*          Unit_p;
    stblit_Job_t*           Job_p          = (stblit_Job_t*) JobHandle;
    STBLIT_JobHandle_t      PreviousJob;
    STBLIT_JobHandle_t      NextJob;

    Unit_p = (stblit_Unit_t*)(Job_p->Handle);

    /* Update previous and next job descriptor */
    PreviousJob = Job_p->PreviousJob ;
    NextJob     = Job_p->NextJob ;
    if (PreviousJob != STBLIT_NO_JOB_HANDLE)
    {
        ((stblit_Job_t*)PreviousJob)->NextJob = NextJob;
    }
    else
    {
        /* Update stblit_unit_t */
        Unit_p->FirstCreatedJob  = NextJob;
    }

    if (NextJob != STBLIT_NO_JOB_HANDLE)
    {
        ((stblit_Job_t*)NextJob)->PreviousJob = PreviousJob;
    }
    else
    {
        /* Update stblit_Unit_t */
        Unit_p->LastCreatedJob  = PreviousJob;
    }

    (Unit_p->NbCreatedJobs)--;

    return(Err);
}


 /*
--------------------------------------------------------------------------------
Create Job API function
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STBLIT_CreateJob( STBLIT_Handle_t         Handle,
                                 STBLIT_JobParams_t*     Params_p,
                                 STBLIT_JobHandle_t*     JobHandle_p)
{
    ST_ErrorCode_t      Err         = ST_NO_ERROR;
    stblit_Unit_t*      Unit_p      = (stblit_Unit_t*) Handle;
    stblit_Device_t*    Device_p;
    stblit_Job_t*       Job_p;


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

    /* Check parameters */
    if ((Params_p == NULL) ||
        (JobHandle_p == NULL))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Check Job Params */

    /* Get Job descriptor handle*/
    Err = stblit_GetElement(&(Device_p->JobDataPool), (void**) JobHandle_p);
    if (Err != ST_NO_ERROR)
    {
        return(STBLIT_ERROR_MAX_JOB);
    }

    Job_p = (stblit_Job_t*) *JobHandle_p;

    Job_p->Handle           = Handle;
    Job_p->FirstNodeHandle  = STBLIT_NO_NODE_HANDLE;
    Job_p->LastNodeHandle   = STBLIT_NO_NODE_HANDLE;
#ifndef STBLIT_LINUX_FULL_USER_VERSION
    Job_p->SubscriberID     = Params_p->EventSubscriberID;
#endif
    Job_p->NotifyCompletion = Params_p->NotifyJobCompletion;
    Job_p->NbNodes          = 0;
    Job_p->JobValidity      = STBLIT_VALID_JOB;
    Job_p->Closed           = FALSE;
    Job_p->PendingLock      = FALSE;
    Job_p->UseLockingMechanism = FALSE;
    Job_p->LockBeforeFirstNode  = FALSE;
    Job_p->FirstJBlitHandle = STBLIT_NO_JOB_BLIT_HANDLE;
    Job_p->LastJBlitHandle  = STBLIT_NO_JOB_BLIT_HANDLE;
    Job_p->NbJBlit          = 0;

    /* Update Created job list */
    AddJobToList(*JobHandle_p);

#ifdef STBLIT_LINUX_FULL_USER_VERSION
    if ( PhysicJobBlitNodeBuffer_p == 0 )
    {
        PhysicJobBlitNodeBuffer_p = (U32)stblit_CpuToDevice((void*)Device_p->JobBlitNodeBuffer_p) ;
    }
#endif

    return(Err);
}

/*
--------------------------------------------------------------------------------
Delete Job API function
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STBLIT_DeleteJob( STBLIT_Handle_t        Handle,
                                 STBLIT_JobHandle_t     JobHandle)
{
    ST_ErrorCode_t      Err         = ST_NO_ERROR;
    stblit_Unit_t*      Unit_p      = (stblit_Unit_t*) Handle;
    stblit_Device_t*    Device_p;
    stblit_Job_t*       Job_p       = (stblit_Job_t*) JobHandle;


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

    /* Release Job Nodes */
    if (Job_p->NbNodes != 0)
    {
        /* Release the full list of nodes within the job (From Last to First) */
        /* Make sure the First Node in the list has got its PreviousNode field set to STBLIT_NO_NODE_HANDLE */
        stblit_ReleaseListNode(Device_p, Job_p->LastNodeHandle);
    }

    /* Release Job Blit */
    if (Job_p->NbJBlit != 0)
    {
        /* Release the full list of Job Blit within the job (From First to Last) */
        /* Make sure the Last Job Blit in the list has got its NextJBlit field set to STBLIT_NO_JOB_BLIT_HANDLE */
        ReleaseListJBlit(Device_p, Job_p->FirstJBlitHandle);
    }

    /* Remove job from the list in stblit_Unit_t gathering created jobs */
    RemoveJobFromList(JobHandle);

    /* Release Job descriptor */
    Job_p->JobValidity = (U32)~STBLIT_VALID_JOB;

    stblit_ReleaseElement(&(Device_p->JobDataPool), (void*) JobHandle);

    return(Err);
}

/*******************************************************************************
Name        : stblit_AddNodeListInJob
Description : Add a list of Nodes corresponding to an operation in a Job
Parameters  :
Assumptions : All parameters Checks have to be done at upper level
              JobHandle different from STBLIT_NO_JOB_HANDLE
Limitations :
Returns     :
*******************************************************************************/
 ST_ErrorCode_t  stblit_AddNodeListInJob (stblit_Device_t*      Device_p,
                                          STBLIT_JobHandle_t    JobHandle,
                                          stblit_NodeHandle_t   FirstNodeHandle,
                                          stblit_NodeHandle_t   LastNodeHandle,
                                          U32                   NbNodesInList)
 {
    ST_ErrorCode_t  Err             = ST_NO_ERROR;
    stblit_Job_t*   Job_p           = (stblit_Job_t*) JobHandle;
    stblit_Node_t*  LastNode_p      = (stblit_Node_t*)LastNodeHandle;
    stblit_Node_t*  InsertionNode_p;
    U32             CompletionMask;
    U32             Tmp;
    U32             INS;

	/* To remove comipation warnning */
	UNUSED_PARAMETER(Device_p);


    if (Job_p->NbNodes == 0)
    {
        Job_p->FirstNodeHandle = FirstNodeHandle;
        Job_p->LastNodeHandle  = LastNodeHandle;

    }
    else
    {
        /* Fetch insertion node */
        InsertionNode_p                 = (stblit_Node_t*) Job_p->LastNodeHandle;

        /* Update NIP of Insertion Node */
#ifdef STBLIT_LINUX_FULL_USER_VERSION
        Tmp = (U32) stblit_CpuToDevice((void*)FirstNodeHandle);
#else
        Tmp = (U32) stblit_CpuToDevice((void*)FirstNodeHandle, &(Device_p->VirtualMapping));
#endif
        STSYS_WriteRegMem32LE((void*)(&(InsertionNode_p->HWNode.BLT_NIP)),Tmp);

        ((stblit_Node_t*)FirstNodeHandle)->PreviousNode = (stblit_NodeHandle_t)InsertionNode_p;
        Job_p->LastNodeHandle                           = LastNodeHandle;

        if (Job_p->NotifyCompletion == TRUE)
        {
            /* Remove ITOpcode at insertion point for job completion */
            InsertionNode_p->ITOpcode &= (U32)~STBLIT_IT_OPCODE_JOB_COMPLETION_MASK;

            /* Remove interrupt if interruption not needed for some other possible reasons of this stage :
             * + S/W Workaround for h/w cuts limitation
             * + Blit completion
             * + Lock
             * + unlock
             */
            CompletionMask =
#if defined (WA_FILL_OPERATION) || defined (WA_SOFT_RESET_EACH_NODE)  || defined (WA_GNBvd15279)
                             STBLIT_IT_OPCODE_SW_WORKAROUND_MASK |
#endif
                             STBLIT_IT_OPCODE_BLIT_COMPLETION_MASK |
                             STBLIT_IT_OPCODE_LOCK_MASK |
                             STBLIT_IT_OPCODE_UNLOCK_MASK;
            if ((InsertionNode_p->ITOpcode & CompletionMask) == 0)
            {
                /* Remove Interrupt */
                INS = (U32)STSYS_ReadRegMem32LE((void*)(&(InsertionNode_p->HWNode.BLT_INS)));
                INS &= ((U32)(~((U32)(STBLIT_INS_ENABLE_IRQ_COMPLETED) << STBLIT_INS_ENABLE_IRQ_SHIFT)));
                STSYS_WriteRegMem32LE((void*)(&(InsertionNode_p->HWNode.BLT_INS)),INS);
            }
        }
    }

    /* Update last node NIP to NULL */
    STSYS_WriteRegMem32LE((void*)(&(LastNode_p->HWNode.BLT_NIP)),(U32)NULL);


    /* INSTALL interruption + ITOpcode in the new last node of the job
     * DeInstall ITOpcode of the previous last node  and perhaps its interruption (if not asked for something else)
     * (to do so, check the ITOpcode */

    if (Job_p->NotifyCompletion == TRUE)
    {
        /* set ITOpcode + Interrupt */
        LastNode_p->ITOpcode       |=  STBLIT_IT_OPCODE_JOB_COMPLETION_MASK;
        INS = (U32)STSYS_ReadRegMem32LE((void*)(&(LastNode_p->HWNode.BLT_INS)));
        INS |= ((U32)(STBLIT_INS_ENABLE_IRQ_COMPLETED & STBLIT_INS_ENABLE_IRQ_MASK) << STBLIT_INS_ENABLE_IRQ_SHIFT);
        STSYS_WriteRegMem32LE((void*)(&(LastNode_p->HWNode.BLT_INS)),INS);
    }

    Job_p->NbNodes += NbNodesInList;

    return(Err);
 }

/********************************************************************************
Name        : stblit_AddLockInJob
Description : Update description of the last node in the job so that
              subsequent nodes will be locked
              JobHandle different from STBLIT_NO_JOB_HANDLE
Parameters  :
Assumptions : All parameters Checks have to be done at upper level
Limitations :
Returns     :
*******************************************************************************/
 ST_ErrorCode_t  stblit_AddLockInJob (STBLIT_JobHandle_t     JobHandle)
 {
    ST_ErrorCode_t  Err             = ST_NO_ERROR;
    stblit_Job_t*   Job_p           = (stblit_Job_t*) JobHandle;
    stblit_Node_t*  LastNode_p;
    U32             INS;

    if(Job_p->NbNodes != 0) /* There is already at least one node within the job */
    {
        LastNode_p  = (stblit_Node_t*) Job_p->LastNodeHandle;

        /* If there is already an unlock request for this node , then remove it because unlock-lock <=> no effect */
        if ((LastNode_p->ITOpcode & STBLIT_IT_OPCODE_UNLOCK_MASK) != 0)
        {
            /* Reset UNLOCK ITOpcode */
            LastNode_p->ITOpcode  &=  (U32)~STBLIT_IT_OPCODE_UNLOCK_MASK;

            /* Remove Interruption related to unlock if interruption not needed for some other possible reasons of this stage:
             * + S/W Workaround for h/w cuts limitation
             * + Blit completion
             * + Job completion
             * Note that Unlock opcode has been just reset and LOCK Opcode is not possible (upper function detect several locks)
             */
            if ((LastNode_p->ITOpcode &
                (
#if defined (WA_FILL_OPERATION) || defined (WA_SOFT_RESET_EACH_NODE) || defined (WA_GNBvd15279)
                STBLIT_IT_OPCODE_SW_WORKAROUND_MASK |
#endif
                STBLIT_IT_OPCODE_BLIT_COMPLETION_MASK |
                STBLIT_IT_OPCODE_JOB_COMPLETION_MASK)) == 0)
            {
                INS = (U32)STSYS_ReadRegMem32LE((void*)(&(LastNode_p->HWNode.BLT_INS)));
                INS &= ((U32)(~((U32)(STBLIT_INS_ENABLE_IRQ_COMPLETED) << STBLIT_INS_ENABLE_IRQ_SHIFT)));
                STSYS_WriteRegMem32LE((void*)(&(LastNode_p->HWNode.BLT_INS)),INS);
            }
        }
        else /* Insert IT */
        {
            LastNode_p->ITOpcode       |=  STBLIT_IT_OPCODE_LOCK_MASK;
            INS = (U32)STSYS_ReadRegMem32LE((void*)(&(LastNode_p->HWNode.BLT_INS)));
            INS |= ((U32)(STBLIT_INS_ENABLE_IRQ_COMPLETED & STBLIT_INS_ENABLE_IRQ_MASK) << STBLIT_INS_ENABLE_IRQ_SHIFT);
            STSYS_WriteRegMem32LE((void*)(&(LastNode_p->HWNode.BLT_INS)),INS);
        }
    }
    else  /* No node within the job*/
    {
        Job_p->LockBeforeFirstNode = TRUE;
    }

    Job_p->PendingLock = TRUE;
    Job_p->UseLockingMechanism = TRUE;

    return(Err);
 }

 /*******************************************************************************
Name        : stblit_AddUnlockInJob
Description : Update description of the previous node of the last one
              in the job so that subsequent nodes will be unlocked.
             JobHandle different from STBLIT_NO_JOB_HANDLE
Parameters  :
Assumptions : All parameters Checks have to be done at upper level
              There is a lock pending
              JobHandle different from STBLIT_NO_JOB_HANDLE
Limitations :
Returns     :
*******************************************************************************/
 ST_ErrorCode_t  stblit_AddUnlockInJob (STBLIT_JobHandle_t     JobHandle)
 {
    ST_ErrorCode_t  Err             = ST_NO_ERROR;
    stblit_Job_t*   Job_p           = (stblit_Job_t*) JobHandle;
    stblit_Node_t*  LastNode_p;
    U32             INS;

    if(Job_p->NbNodes != 0)
    {
        LastNode_p = (stblit_Node_t*) Job_p->LastNodeHandle;

        /* If there is already a lock request for this node , then remove it because lock-unlock <=> no effect */
        if ((LastNode_p->ITOpcode & STBLIT_IT_OPCODE_LOCK_MASK) != 0)
        {
            /* Reset LOCK ITOpcode */
            LastNode_p->ITOpcode  &=  (U32)~STBLIT_IT_OPCODE_LOCK_MASK;

            /* Remove Interruption related to lock if interruption not needed for some other possible reasons of this stage:
             * + S/W Workaround for h/w cuts limitation
             * + Blit completion
             * + Job completion
             * Note that lock opcode has been just reset and UNLOCK Opcode is not possible (upper function detect several unlocks)
             */
            if ((LastNode_p->ITOpcode &
                (
#if defined (WA_FILL_OPERATION) || defined (WA_SOFT_RESET_EACH_NODE) || defined (WA_GNBvd15279)
                STBLIT_IT_OPCODE_SW_WORKAROUND_MASK |
#endif
                STBLIT_IT_OPCODE_BLIT_COMPLETION_MASK |
                STBLIT_IT_OPCODE_JOB_COMPLETION_MASK)) == 0)
            {
                INS = (U32)STSYS_ReadRegMem32LE((void*)(&(LastNode_p->HWNode.BLT_INS)));
                INS &= ((U32)(~((U32)(STBLIT_INS_ENABLE_IRQ_COMPLETED) << STBLIT_INS_ENABLE_IRQ_SHIFT)));
                STSYS_WriteRegMem32LE((void*)(&(LastNode_p->HWNode.BLT_INS)),INS);
            }
        }
        else /* Insert IT */
        {
            LastNode_p->ITOpcode |=  STBLIT_IT_OPCODE_UNLOCK_MASK;
            INS = (U32)STSYS_ReadRegMem32LE((void*)(&(LastNode_p->HWNode.BLT_INS)));
            INS |= ((U32)(STBLIT_INS_ENABLE_IRQ_COMPLETED & STBLIT_INS_ENABLE_IRQ_MASK) << STBLIT_INS_ENABLE_IRQ_SHIFT);
            STSYS_WriteRegMem32LE((void*)(&(LastNode_p->HWNode.BLT_INS)),INS);
        }
    }
    else /* No node within the job + PendingLock =>  LockBeforeFirstNode */
    {
        Job_p->LockBeforeFirstNode = FALSE;
    }

    Job_p->PendingLock = FALSE;

    return(Err);
 }

/*******************************************************************************
Name        : stblit_AddJBlitInJob
Description : Set a Job Blit descriptor for the last operation inserted in the job.
              Update the JBlit list from the job.
Parameters  :
Assumptions : All parameters Checks have to be done at upper level
              JobHandle valid and different from STBLIT_NO_JOB_HANDLE
              Config_p points to an allocated OperationConfiguration structure.
Limitations :
Returns     :
*******************************************************************************/
 ST_ErrorCode_t  stblit_AddJBlitInJob        (stblit_Device_t*      Device_p,
                                              STBLIT_JobHandle_t    JobHandle,
                                              stblit_NodeHandle_t   FirstNodeHandle,
                                              stblit_NodeHandle_t   LastNodeHandle,
                                              U32                   NbNodesInList,
                                              stblit_OperationConfiguration_t*   Config_p)
 {
    ST_ErrorCode_t          Err             = ST_NO_ERROR;
    stblit_Job_t*           Job_p           = (stblit_Job_t*) JobHandle;
    STBLIT_JobBlitHandle_t  JBlitHandle;
    stblit_JobBlit_t*       JBlit_p;
	U32                     TempAddress;

	/* Added to rmove compilation warnning (dereferencing type-punned pointer will break strict-aliasing rules) */
	TempAddress = (U32)&JBlitHandle;

    /* Get Job Blit descriptor handle*/
    Err = stblit_GetElement(&(Device_p->JBlitDataPool), (void**) TempAddress);
    if (Err != ST_NO_ERROR)
    {
       return(STBLIT_ERROR_MAX_JOB_BLIT);
    }
    JBlit_p = (stblit_JobBlit_t*)JBlitHandle;


    /* Update Job descriptor */
    if (Job_p->NbJBlit == 0)
    {
        Job_p->FirstJBlitHandle = JBlitHandle;
    }
    else
    {
        /* Update Previous Job Blit descriptor */
        ((stblit_JobBlit_t*)(Job_p->LastJBlitHandle))->NextJBlit = JBlitHandle;
    }
    Job_p->LastJBlitHandle = JBlitHandle;
    Job_p->NbJBlit = Job_p->NbJBlit + 1;

    /* Update new Job Blit descriptor */
    JBlit_p->JBlitValidity      = STBLIT_VALID_JOB_BLIT;
    JBlit_p->JobHandle          = JobHandle;
    JBlit_p->FirstNodeHandle    = FirstNodeHandle;
    JBlit_p->LastNodeHandle     = LastNodeHandle;
    JBlit_p->NbNodes            = NbNodesInList;
    JBlit_p->NextJBlit          = STBLIT_NO_JOB_BLIT_HANDLE;

    /* Update JBlit operation configuration */
    JBlit_p->Cfg = *Config_p;

    return(Err);
 }


/*
--------------------------------------------------------------------------------
Get JobBlit Handle API function
--------------------------------------------------------------------------------
*/
 ST_ErrorCode_t STBLIT_GetJobBlitHandle( STBLIT_Handle_t          Handle,
                                         STBLIT_JobHandle_t       JobHandle,
                                         STBLIT_JobBlitHandle_t*  JobBlitHandle_p)
 {
    ST_ErrorCode_t      Err         = ST_NO_ERROR;
    stblit_Unit_t*      Unit_p      = (stblit_Unit_t*) Handle;
    stblit_Device_t*    Device_p;
    stblit_Job_t*       Job_p       = (stblit_Job_t*) JobHandle;

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

    /* Check parameters */
    if (JobBlitHandle_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
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

    /* Fetch last Blit in job  */
    *JobBlitHandle_p = Job_p->LastJBlitHandle;


    return(Err);
 }


#ifdef ST_OSWINCE

/*
--------------------------------------------------------------------------------
stblit_GetPreviousJBlit utility function
returns NULL if the job blit is the first in the job or is not in the job
--------------------------------------------------------------------------------
*/
static stblit_JobBlit_t* stblit_GetPreviousJBlit(stblit_Job_t* Job_p, STBLIT_JobBlitHandle_t JobBlitHandle)
{
	stblit_JobBlit_t*   PreviousJBlit_p;

	if (Job_p->FirstJBlitHandle == JobBlitHandle)
		return NULL;
	else
	{
		PreviousJBlit_p = (stblit_JobBlit_t*)(Job_p->FirstJBlitHandle);
		while (PreviousJBlit_p->NextJBlit != STBLIT_NO_JOB_BLIT_HANDLE && PreviousJBlit_p->NextJBlit != JobBlitHandle)
			PreviousJBlit_p = (stblit_JobBlit_t*)(PreviousJBlit_p->NextJBlit);
		if (PreviousJBlit_p->NextJBlit == STBLIT_NO_JOB_BLIT_HANDLE) // should not happen
				return NULL;
		return PreviousJBlit_p;
	}
}


/*
--------------------------------------------------------------------------------
stblit_SetInterruptMask utility function
--------------------------------------------------------------------------------
*/
static void stblit_SetInterruptMask(stblit_Node_t* Node_p, BOOL setNotReset)
{
	U32 INS;

	INS = (U32)STSYS_ReadRegMem32LE((void*)(&(Node_p->HWNode.BLT_INS)));

	if (setNotReset)
	{
        Node_p->ITOpcode |= STBLIT_IT_OPCODE_JOB_COMPLETION_MASK;
		INS |= ((U32)(STBLIT_INS_ENABLE_IRQ_COMPLETED & STBLIT_INS_ENABLE_IRQ_MASK) << STBLIT_INS_ENABLE_IRQ_SHIFT);
	}
	else
	{
        Node_p->ITOpcode &= (U32)~STBLIT_IT_OPCODE_JOB_COMPLETION_MASK;
        INS &= ((U32)(~((U32)(STBLIT_INS_ENABLE_IRQ_COMPLETED) << STBLIT_INS_ENABLE_IRQ_SHIFT)));
	}

	STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_INS)),INS);
}

/*
--------------------------------------------------------------------------------
Remove JobBlit from job API function
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STBLIT_RemoveJobBlit(STBLIT_Handle_t Handle,
                                    STBLIT_JobBlitHandle_t JobBlitHandle)
{
    stblit_Unit_t*      Unit_p      = (stblit_Unit_t*) Handle; // STBLIT instance
    stblit_Device_t*    Device_p;                              // STBLIT driver
    stblit_Job_t*       Job_p;		// JobBlitHandle belongs to this job
    stblit_JobBlit_t*   JBlit_p = (stblit_JobBlit_t*) JobBlitHandle;
    stblit_JobBlit_t*   NextJBlit_p;     // the job blit after JobBlitHandle in the parent job, or NULL
	stblit_JobBlit_t*   PreviousJBlit_p; // the job blit before JobBlitHandle in the parent job, or NULL
    stblit_Node_t*		PreviousNode_p;  // the last node of the previous job blit, or NULL
	stblit_Node_t*		NextNode_p;      // the first node of the next job blit, or NULL

    /* Check Handle validity */
    if (Handle == NULL)
        return ST_ERROR_INVALID_HANDLE;

    Device_p = Unit_p->Device_p ;
    if (Unit_p->UnitValidity != STBLIT_VALID_UNIT)
        return ST_ERROR_INVALID_HANDLE;

    /* Check Job Blit handle validity */
    if (JobBlitHandle == NULL)
        return ST_ERROR_BAD_PARAMETER;

    if (JBlit_p->JBlitValidity != STBLIT_VALID_JOB_BLIT)
        return STBLIT_ERROR_INVALID_JOB_BLIT_HANDLE;

	/* Retrieve Job */
	if (JBlit_p->JobHandle == NULL)
		return STBLIT_ERROR_JOB_BLIT_DETACHED; // cant remove an already removed job blit
	Job_p = (stblit_Job_t*)(JBlit_p->JobHandle);

    /* Check Job Blit Handles mismatch */
    if (Job_p->Handle != Handle)
        return STBLIT_ERROR_JOB_HANDLE_MISMATCH;

    /* Check Job Validity */
    if (Job_p->JobValidity != STBLIT_VALID_JOB)
        return STBLIT_ERROR_INVALID_JOB_HANDLE;
	if (Job_p->NbJBlit == 0)
		return STBLIT_ERROR_JOB_EMPTY;

	// lock access
#ifdef STBLIT_WITH_LOCKS
        STOS_SemaphoreWait(Device_p->AccessControl);
#endif

	// Find next job blit and node
	NextJBlit_p = (stblit_JobBlit_t*)(JBlit_p->NextJBlit); // NULL if the removed job blit is the last
	if (NextJBlit_p == NULL)
		NextNode_p = NULL;
	else
		NextNode_p = (stblit_Node_t*)(NextJBlit_p->FirstNodeHandle);

	// Find previous job blit and node
	PreviousJBlit_p = stblit_GetPreviousJBlit(Job_p, JobBlitHandle);
	if (PreviousJBlit_p == NULL && Job_p->FirstJBlitHandle != JobBlitHandle) // should not happen
		return STBLIT_ERROR_JOB_BLIT_HANDLE_MISMATCH;
	if (PreviousJBlit_p == NULL)
		PreviousNode_p = NULL;
	else
		PreviousNode_p = (stblit_Node_t*)(PreviousJBlit_p->LastNodeHandle);

	// Update job
	if (Job_p->FirstJBlitHandle == JobBlitHandle)
	{
		Job_p->FirstJBlitHandle = (STBLIT_JobBlitHandle_t)NextJBlit_p;
		Job_p->FirstNodeHandle = (stblit_NodeHandle_t)NextNode_p;
	}
	if (Job_p->LastJBlitHandle == JobBlitHandle)
	{
		Job_p->LastJBlitHandle = (STBLIT_JobBlitHandle_t)PreviousJBlit_p;
		Job_p->LastNodeHandle = (stblit_NodeHandle_t)PreviousNode_p;
	}
	-- Job_p->NbJBlit;
	Job_p->NbNodes -= JBlit_p->NbNodes;

	// Update job blits
	if (PreviousJBlit_p != NULL)
		PreviousJBlit_p->NextJBlit = (STBLIT_JobBlitHandle_t)NextJBlit_p;

	// Update nodes
	if (PreviousJBlit_p != NULL)
	{
		// update "next node" of previous node
		U32 Tmp = (U32)stblit_CpuToDevice((void*)NextNode_p,&(Device_p->VirtualMapping));
	    STSYS_WriteRegMem32LE(&(PreviousNode_p->HWNode.BLT_NIP), Tmp);

		// Set interrupt mask if PreviousNode_p is now the last node of the job whose completion flag is set
		if (NextNode_p == NULL && Job_p->NotifyCompletion)
			stblit_SetInterruptMask(PreviousNode_p, TRUE);
	}
	STSYS_WriteRegMem32LE(&(((stblit_Node_t*)(JBlit_p->LastNodeHandle))->HWNode.BLT_NIP), 0);
	if (NextJBlit_p != NULL)
	{
		// update "previous node" of next node
		NextNode_p->PreviousNode = (stblit_NodeHandle_t)PreviousNode_p;
	}
	((stblit_Node_t*)(JBlit_p->FirstNodeHandle))->PreviousNode = NULL;

	// Update newly detached Job blit
	JBlit_p->JobHandle = STBLIT_NO_JOB_HANDLE;
	JBlit_p->NextJBlit = STBLIT_NO_JOB_BLIT_HANDLE;
	stblit_SetInterruptMask((stblit_Node_t*)(JBlit_p->LastNodeHandle), FALSE);

	// unlock access
#ifdef STBLIT_WITH_LOCKS
        STOS_SemaphoreSignal(Device_p->AccessControl);
#endif

	return ST_NO_ERROR;
}

/*
--------------------------------------------------------------------------------
Remove all the JobBlits from a job API function
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STBLIT_RemoveAllJobBlits(STBLIT_Handle_t Handle,
                                        STBLIT_JobHandle_t JobHandle)
{
    stblit_Unit_t*      Unit_p      = (stblit_Unit_t*) Handle; // STBLIT instance
    stblit_Device_t*    Device_p;                              // STBLIT driver
    stblit_Job_t*       Job_p = (stblit_Job_t*) JobHandle;
	stblit_Node_t*      Node_p;
	stblit_JobBlit_t*   JobBlit_p;

    /* Check Handle validity */
    if (Handle == NULL)
        return ST_ERROR_INVALID_HANDLE;

    Device_p = Unit_p->Device_p ;
    if (Unit_p->UnitValidity != STBLIT_VALID_UNIT)
        return ST_ERROR_INVALID_HANDLE;

    /* Check Job Blit handle validity */
    if (JobHandle == NULL)
        return ST_ERROR_BAD_PARAMETER;

    /* Check Job Handle mismatch */
    if (Job_p->Handle != Handle)
        return STBLIT_ERROR_JOB_HANDLE_MISMATCH;

    /* Check Job Validity */
    if (Job_p->JobValidity != STBLIT_VALID_JOB)
        return STBLIT_ERROR_INVALID_JOB_HANDLE;

	// lock access
#ifdef STBLIT_WITH_LOCKS
        STOS_SemaphoreWait(Device_p->AccessControl);
#endif

	// Update nodes

	Node_p = (stblit_Node_t*)(Job_p->LastNodeHandle);
	stblit_SetInterruptMask(Node_p, FALSE);

	// Update Job blits
	for (JobBlit_p = (stblit_JobBlit_t*)(Job_p->FirstJBlitHandle);
	     JobBlit_p != NULL;
		 JobBlit_p = (stblit_JobBlit_t*)(JobBlit_p->NextJBlit))
	 {
        Node_p = (stblit_Node_t*)(JobBlit_p->FirstNodeHandle);
        Node_p->PreviousNode = NULL;
        Node_p = (stblit_Node_t*)(JobBlit_p->LastNodeHandle);
        STSYS_WriteRegMem32LE(&(Node_p->HWNode.BLT_NIP), 0);
        JobBlit_p->JobHandle = NULL;
	 }

	// Update job
	Job_p->FirstJBlitHandle = NULL;
	Job_p->FirstNodeHandle = NULL;
	Job_p->LastJBlitHandle = NULL;
	Job_p->LastNodeHandle = NULL;
	Job_p->NbJBlit = 0;
	Job_p->NbNodes = 0;

	// unlock access
#ifdef STBLIT_WITH_LOCKS
        STOS_SemaphoreSignal(Device_p->AccessControl);
#endif

	return ST_NO_ERROR;
}

// Device_p->AccessControl must have been taken before calling this function
static void stblit_InsertJobBlit(stblit_Device_t*  Device_p,
								 stblit_Job_t*     Job_p,
								 stblit_JobBlit_t* InsertedJBlit_p,
								 stblit_JobBlit_t* PreviousJBlit_p)
{
    stblit_JobBlit_t*   NextJBlit_p;     // after the inserted job blit, or NULL
    stblit_Node_t*		PreviousNode_p;  // the last node of the previous job blit, or NULL
	stblit_Node_t*		NextNode_p;      // the first node of the next job blit
	stblit_Node_t*		FirstNode_p;     // First inserted node
	stblit_Node_t*		LastNode_p;      // Last inserted node
	stblit_Node_t*		CurrentNode_p;   // walking node
	STBLIT_JobBlitHandle_t insertThisJobBlit = (STBLIT_JobBlitHandle_t)InsertedJBlit_p;

	// Get previous node
	if (PreviousJBlit_p == NULL)
		PreviousNode_p = NULL;
	else
		PreviousNode_p = (stblit_Node_t*)(PreviousJBlit_p->LastNodeHandle);

	// Get next node
	if (PreviousJBlit_p == NULL) // insert at beginning of job
		NextJBlit_p = (stblit_JobBlit_t*)(Job_p->FirstJBlitHandle);
	else
		NextJBlit_p = (stblit_JobBlit_t*)(PreviousJBlit_p->NextJBlit);

	if (NextJBlit_p == NULL)
		NextNode_p = NULL;
	else
		NextNode_p = (stblit_Node_t*)(NextJBlit_p->FirstNodeHandle);

	// Update job
	if (PreviousJBlit_p == NULL)
	{
		Job_p->FirstJBlitHandle = insertThisJobBlit;
		Job_p->FirstNodeHandle = InsertedJBlit_p->FirstNodeHandle;
	}
	if (NextJBlit_p == NULL)
	{
		Job_p->LastJBlitHandle = insertThisJobBlit;
		Job_p->LastNodeHandle = InsertedJBlit_p->LastNodeHandle;
	}

	++ Job_p->NbJBlit;
	Job_p->NbNodes += InsertedJBlit_p->NbNodes;

	// Update Job blits
	if (PreviousJBlit_p != NULL)
		PreviousJBlit_p->NextJBlit = insertThisJobBlit;
	InsertedJBlit_p->NextJBlit = (STBLIT_JobBlitHandle_t)NextJBlit_p;
	InsertedJBlit_p->JobHandle = (STBLIT_JobHandle_t)Job_p;

	// Update nodes
	FirstNode_p = (stblit_Node_t*)(InsertedJBlit_p->FirstNodeHandle);
	LastNode_p =  (stblit_Node_t*)(InsertedJBlit_p->LastNodeHandle);
	if (PreviousNode_p != NULL)
	{
		U32 Tmp = (U32)stblit_CpuToDevice((void*)FirstNode_p,&(Device_p->VirtualMapping));
	    STSYS_WriteRegMem32LE(&(PreviousNode_p->HWNode.BLT_NIP), Tmp);
	}
	FirstNode_p->PreviousNode = (stblit_NodeHandle_t)PreviousNode_p;
	{
		U32 Tmp = (U32)stblit_CpuToDevice((void*)NextNode_p,&(Device_p->VirtualMapping));
	    STSYS_WriteRegMem32LE(&(LastNode_p->HWNode.BLT_NIP), Tmp);
	}
	if (NextNode_p != NULL)
		NextNode_p->PreviousNode = (stblit_NodeHandle_t)LastNode_p;

	if (NextNode_p == NULL && Job_p->NotifyCompletion)
	{
		// The inserted job blit is the new last one of the job
		stblit_SetInterruptMask(LastNode_p, TRUE);
		if (PreviousNode_p != NULL)
			stblit_SetInterruptMask(PreviousNode_p, FALSE);
	}

	for (CurrentNode_p = LastNode_p;
	     CurrentNode_p != NULL && CurrentNode_p->JobHandle != (STBLIT_JobHandle_t)Job_p;
		 CurrentNode_p = (stblit_Node_t*)(CurrentNode_p->PreviousNode))
		CurrentNode_p->JobHandle = (STBLIT_JobHandle_t)Job_p;

	// unlock access
#ifdef STBLIT_WITH_LOCKS
        STOS_SemaphoreSignal(Device_p->AccessControl);
#endif

	return ST_NO_ERROR;
}


/*
--------------------------------------------------------------------------------
Insert JobBlit into job API function
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STBLIT_InsertJobBlitBefore(STBLIT_Handle_t Handle,
									      STBLIT_JobHandle_t JobHandle,
                                          STBLIT_JobBlitHandle_t insertBeforeThisJobBlit,
                                          STBLIT_JobBlitHandle_t insertThisJobBlit)
{
    stblit_Unit_t*      Unit_p      = (stblit_Unit_t*) Handle; // STBLIT instance
    stblit_Device_t*    Device_p;                              // STBLIT driver
    stblit_Job_t*       Job_p = (stblit_Job_t*)JobHandle;
    stblit_JobBlit_t*   InsertedJBlit_p = (stblit_JobBlit_t*) insertThisJobBlit;
	stblit_JobBlit_t*   PreviousJBlit_p; // the job blit before NextJBlit_p in the parent job, or NULL
    stblit_JobBlit_t*   NextJBlit_p = (stblit_JobBlit_t*) insertBeforeThisJobBlit;

    /* Check Handle validity */
    if (Handle == NULL)
        return ST_ERROR_INVALID_HANDLE;

    Device_p = Unit_p->Device_p ;
    if (Unit_p->UnitValidity != STBLIT_VALID_UNIT)
        return ST_ERROR_INVALID_HANDLE;

    /* Check Job Blit handle validity */
    if (insertBeforeThisJobBlit == NULL)
        return ST_ERROR_BAD_PARAMETER;

    if (NextJBlit_p->JBlitValidity != STBLIT_VALID_JOB_BLIT)
        return STBLIT_ERROR_INVALID_JOB_BLIT_HANDLE;

	if (NextJBlit_p->JobHandle == STBLIT_NO_JOB_HANDLE)
        return STBLIT_ERROR_JOB_BLIT_DETACHED;

    /* Check Job Validity */
    if (Job_p->Handle != Handle)
        return STBLIT_ERROR_JOB_HANDLE_MISMATCH;

    if (Job_p->JobValidity != STBLIT_VALID_JOB)
        return STBLIT_ERROR_INVALID_JOB_HANDLE;

	if (PreviousJBlit_p != NULL && PreviousJBlit_p->JobHandle != JobHandle)
		return STBLIT_ERROR_JOB_HANDLE_MISMATCH;

	/* Check Job Blit Handles mismatch */
    if (Job_p->Handle != Handle)
        return STBLIT_ERROR_JOB_HANDLE_MISMATCH;

    /* Check Job Validity */
    if (Job_p->JobValidity != STBLIT_VALID_JOB)
        return STBLIT_ERROR_INVALID_JOB_HANDLE;

	/* Check inserted Job Blit */
	if (insertThisJobBlit == NULL)
        return ST_ERROR_BAD_PARAMETER;

    if (InsertedJBlit_p->JBlitValidity != STBLIT_VALID_JOB_BLIT)
        return STBLIT_ERROR_INVALID_JOB_BLIT_HANDLE;

	if (InsertedJBlit_p->JobHandle != STBLIT_NO_JOB_HANDLE)
		return STBLIT_ERROR_JOB_BLIT_DETACHED; // cant insert a job blit belonging to another job

	// lock access
#ifdef STBLIT_WITH_LOCKS
        STOS_SemaphoreWait(Device_p->AccessControl);
#endif

	// Find previous job blit
	PreviousJBlit_p = stblit_GetPreviousJBlit(Job_p, insertBeforeThisJobBlit);

	// Do insertion
	stblit_InsertJobBlit(Device_p, Job_p, InsertedJBlit_p, PreviousJBlit_p);

	return ST_NO_ERROR;
}

/*
--------------------------------------------------------------------------------
InsertJobBlitAfter into job API function
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STBLIT_InsertJobBlitAfter(
	STBLIT_Handle_t Handle,
	STBLIT_JobHandle_t JobHandle,
	STBLIT_JobBlitHandle_t insertAfterThisJobBlit,
	STBLIT_JobBlitHandle_t insertThisJobBlit)
{
    stblit_Unit_t*      Unit_p      = (stblit_Unit_t*) Handle; // STBLIT instance
    stblit_Device_t*    Device_p;                              // STBLIT driver
    stblit_Job_t*       Job_p = (stblit_Job_t*)JobHandle;
    stblit_JobBlit_t*   InsertedJBlit_p = (stblit_JobBlit_t*) insertThisJobBlit;
	stblit_JobBlit_t*   PreviousJBlit_p = (stblit_JobBlit_t*) insertAfterThisJobBlit;


    /* Check Handle validity */
    if (Handle == NULL)
        return ST_ERROR_INVALID_HANDLE;

    Device_p = Unit_p->Device_p ;
    if (Unit_p->UnitValidity != STBLIT_VALID_UNIT)
        return ST_ERROR_INVALID_HANDLE;

    /* Check Job Blit handle validity */
    if (insertThisJobBlit == NULL)
        return ST_ERROR_BAD_PARAMETER;

    if (InsertedJBlit_p->JBlitValidity != STBLIT_VALID_JOB_BLIT)
        return STBLIT_ERROR_INVALID_JOB_BLIT_HANDLE;

	if (InsertedJBlit_p->JobHandle != STBLIT_NO_JOB_HANDLE)
		return STBLIT_ERROR_JOB_BLIT_DETACHED;

	if (PreviousJBlit_p != NULL && PreviousJBlit_p->JBlitValidity != STBLIT_VALID_JOB_BLIT)
        return STBLIT_ERROR_INVALID_JOB_BLIT_HANDLE;

	/* Check Job Blit Handles mismatch */
    if (Job_p->Handle != Handle)
        return STBLIT_ERROR_JOB_HANDLE_MISMATCH;

    /* Check Job Validity */
    if (Job_p->JobValidity != STBLIT_VALID_JOB)
        return STBLIT_ERROR_INVALID_JOB_HANDLE;

	if (PreviousJBlit_p != NULL && PreviousJBlit_p->JobHandle != JobHandle)
		return STBLIT_ERROR_JOB_HANDLE_MISMATCH;

	// lock access
#ifdef STBLIT_WITH_LOCKS
        STOS_SemaphoreWait(Device_p->AccessControl);
#endif

	// Do insertion
	stblit_InsertJobBlit(Device_p, Job_p, InsertedJBlit_p, PreviousJBlit_p);

	return ST_NO_ERROR;
}

/*
--------------------------------------------------------------------------------
STBLIT_InsertJobBlitAtEnd API function
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STBLIT_InsertJobBlitAtEnd(
	STBLIT_Handle_t Handle,
	STBLIT_JobHandle_t JobHandle,
    STBLIT_JobBlitHandle_t insertThisJobBlit)
{
    stblit_Unit_t*      Unit_p      = (stblit_Unit_t*) Handle; // STBLIT instance
    stblit_Device_t*    Device_p;                              // STBLIT driver
    stblit_Job_t*       Job_p = (stblit_Job_t*)JobHandle;
    stblit_JobBlit_t*   InsertedJBlit_p = (stblit_JobBlit_t*) insertThisJobBlit;


    /* Check Handle validity */
    if (Handle == NULL)
        return ST_ERROR_INVALID_HANDLE;

    Device_p = Unit_p->Device_p ;
    if (Unit_p->UnitValidity != STBLIT_VALID_UNIT)
        return ST_ERROR_INVALID_HANDLE;

    /* Check Job Blit handle validity */
    if (insertThisJobBlit == NULL)
        return ST_ERROR_BAD_PARAMETER;

    if (InsertedJBlit_p->JBlitValidity != STBLIT_VALID_JOB_BLIT)
        return STBLIT_ERROR_INVALID_JOB_BLIT_HANDLE;

	if (InsertedJBlit_p->JobHandle != STBLIT_NO_JOB_HANDLE)
		return STBLIT_ERROR_JOB_BLIT_DETACHED;

	/* Check Job Blit Handles mismatch */
    if (Job_p->Handle != Handle)
        return STBLIT_ERROR_JOB_HANDLE_MISMATCH;

    /* Check Job Validity */
    if (Job_p->JobValidity != STBLIT_VALID_JOB)
        return STBLIT_ERROR_INVALID_JOB_HANDLE;

	// lock access
#ifdef STBLIT_WITH_LOCKS
        STOS_SemaphoreWait(Device_p->AccessControl);
#endif

	// Do insertion
	stblit_InsertJobBlit(Device_p, Job_p, InsertedJBlit_p, (stblit_JobBlit_t*)(Job_p->LastJBlitHandle));

	return ST_NO_ERROR;
}

/*
--------------------------------------------------------------------------------
InsertJobBlitCopy API function
--------------------------------------------------------------------------------
*/

ST_ErrorCode_t STBLIT_InsertJobBlitCopy(
	STBLIT_Handle_t Handle,
	STBLIT_JobHandle_t JobHandle,
    STBLIT_JobBlitHandle_t sourceJobBlit)
{
    stblit_Unit_t*      Unit_p      = (stblit_Unit_t*) Handle; // STBLIT instance
    stblit_Device_t*    Device_p;                              // STBLIT driver
    stblit_Job_t*       Job_p = (stblit_Job_t*)JobHandle;
    stblit_JobBlit_t*   SourceJBlit_p = (stblit_JobBlit_t*) sourceJobBlit;
	STBLIT_JobBlitHandle_t CopiedJBlit;
	stblit_JobBlit_t*      CopiedJBlit_p;
	stblit_Node_t*	SourceNode_p;
	stblit_Node_t*	CopiedNode_p;
	ST_ErrorCode_t ErrorCode;

    /* Check Handle validity */
    if (Handle == NULL)
        return ST_ERROR_INVALID_HANDLE;

    Device_p = Unit_p->Device_p ;
    if (Unit_p->UnitValidity != STBLIT_VALID_UNIT)
        return ST_ERROR_INVALID_HANDLE;

    /* Check Job Blit handle validity */
    if (sourceJobBlit == NULL)
        return ST_ERROR_BAD_PARAMETER;

    if (SourceJBlit_p->JBlitValidity != STBLIT_VALID_JOB_BLIT)
        return STBLIT_ERROR_INVALID_JOB_BLIT_HANDLE;

	/* Check Job Blit Handles mismatch */
    if (Job_p->Handle != Handle)
        return STBLIT_ERROR_JOB_HANDLE_MISMATCH;

    /* Check Job Validity */
    if (Job_p->JobValidity != STBLIT_VALID_JOB)
        return STBLIT_ERROR_INVALID_JOB_HANDLE;

	// Limitation: only momo-node job blit
	if (SourceJBlit_p->NbNodes != 1)
		return ST_ERROR_FEATURE_NOT_SUPPORTED;
	SourceNode_p = (stblit_Node_t*)(SourceJBlit_p->FirstNodeHandle);

	// lock access
#ifdef STBLIT_WITH_LOCKS
        STOS_SemaphoreWait(Device_p->AccessControl);
#endif

    // Prepare a copy
	ErrorCode = ST_NO_ERROR;
	if (stblit_GetElement(&(Device_p->JBlitDataPool), (void**) &CopiedJBlit) != ST_NO_ERROR)
		ErrorCode = STBLIT_ERROR_MAX_JOB_BLIT;
	else if (stblit_GetElement(&(Device_p->JBlitNodeDataPool), (void**) &CopiedNode_p) != ST_NO_ERROR)
		ErrorCode = STBLIT_ERROR_MAX_JOB_NODE;
	if (ErrorCode != ST_NO_ERROR)
	{
#ifdef STBLIT_WITH_LOCKS
        STOS_SemaphoreSignal(Device_p->AccessControl);
#endif
        return ErrorCode;
	}
    CopiedJBlit_p = (stblit_JobBlit_t*)CopiedJBlit;

	CopiedJBlit_p->JBlitValidity = SourceJBlit_p->JBlitValidity;
	CopiedJBlit_p->JobHandle = STBLIT_NO_JOB_HANDLE; // detached
	CopiedJBlit_p->NbNodes = SourceJBlit_p->NbNodes; // = 1
    CopiedJBlit_p->NextJBlit = NULL;
	CopiedJBlit_p->Cfg = SourceJBlit_p->Cfg;
	CopiedJBlit_p->FirstNodeHandle = (stblit_NodeHandle_t)CopiedNode_p;
	CopiedJBlit_p->LastNodeHandle  = (stblit_NodeHandle_t)CopiedNode_p;

	memcpy(CopiedNode_p, SourceNode_p, sizeof(*CopiedNode_p));
	CopiedNode_p->PreviousNode = STBLIT_NO_NODE_HANDLE;
	CopiedNode_p->HWNode.BLT_NIP = 0;
	stblit_SetInterruptMask(CopiedNode_p, FALSE);

	// Do insertion
	stblit_InsertJobBlit(Device_p, Job_p, CopiedJBlit_p, (stblit_JobBlit_t*)(Job_p->LastJBlitHandle));

	return ST_NO_ERROR;
}


/*
--------------------------------------------------------------------------------
Delete JobBlit API function
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STBLIT_DeleteJobBlit(STBLIT_Handle_t Handle,
                                    STBLIT_JobBlitHandle_t JobBlitHandle)
{
    stblit_Unit_t*      Unit_p      = (stblit_Unit_t*) Handle; // STBLIT instance
    stblit_Device_t*    Device_p;                              // STBLIT driver
    stblit_JobBlit_t*   JBlit_p = (stblit_JobBlit_t*) JobBlitHandle;

    /* Check Handle validity */
    if (Handle == NULL)
        return ST_ERROR_INVALID_HANDLE;

    Device_p = Unit_p->Device_p ;
    if (Unit_p->UnitValidity != STBLIT_VALID_UNIT)
        return ST_ERROR_INVALID_HANDLE;

    /* Check Job Blit handle validity */
    if (JobBlitHandle == NULL)
        return ST_ERROR_BAD_PARAMETER;
    if (JBlit_p->JBlitValidity != STBLIT_VALID_JOB_BLIT)
        return STBLIT_ERROR_INVALID_JOB_BLIT_HANDLE;
	if (JBlit_p->JobHandle != STBLIT_NO_JOB_HANDLE)
		return STBLIT_ERROR_JOB_BLIT_DETACHED; // cant delete a job blit belonging to another job

	/* delete nodes */
	#ifdef STBLIT_WITH_LOCKS
        STOS_SemaphoreWait(Device_p->AccessControl);
	#endif

        stblit_ReleaseListNode(Device_p, JBlit_p->LastNodeHandle);

    #ifdef STBLIT_WITH_LOCKS
        STOS_SemaphoreSignal(Device_p->AccessControl);
	#endif

	/* delete job blit */
    JBlit_p->JBlitValidity = (U32)~STBLIT_VALID_JOB_BLIT;

    #ifdef STBLIT_WITH_LOCKS
    STOS_SemaphoreWait(Device_p->AccessControl);
    #endif
    stblit_ReleaseElement(&(Device_p->JBlitDataPool), (void*) JBlit_p);
    #ifdef STBLIT_WITH_LOCKS
        STOS_SemaphoreSignal(Device_p->AccessControl);
    #endif

	return ST_NO_ERROR;
}

#endif

 /*
--------------------------------------------------------------------------------
Set JobBlit color API function
--------------------------------------------------------------------------------
*/
 ST_ErrorCode_t STBLIT_SetJobBlitColorFill( STBLIT_Handle_t         Handle,
                                            STBLIT_JobBlitHandle_t  JobBlitHandle,
                                            STBLIT_DataType_t       DataType,
                                            STGXOBJ_Color_t*        Color_p)
 {
    ST_ErrorCode_t      Err         = ST_NO_ERROR;
    stblit_Unit_t*      Unit_p      = (stblit_Unit_t*) Handle;
    stblit_JobBlit_t*   JBlit_p     = (stblit_JobBlit_t*) JobBlitHandle;
    stblit_Device_t*    Device_p;


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

    /* Check Job Blit handle validity */
    if (JobBlitHandle ==(STBLIT_JobBlitHandle_t)NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (JBlit_p->JBlitValidity != STBLIT_VALID_JOB_BLIT)
    {
        return(STBLIT_ERROR_INVALID_JOB_BLIT_HANDLE);
    }

    /* Check Job Blit Handles mismatch */
    if (((stblit_Job_t*)(JBlit_p->JobHandle))->Handle != Handle)
    {
        return(STBLIT_ERROR_JOB_BLIT_HANDLE_MISMATCH);
    }

    /* Check Parameter */
    if (Color_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Not supported modes */
    if ((JBlit_p->Cfg.SrcMB == TRUE) ||
        (JBlit_p->Cfg.ConcatMode == TRUE) ||
        (JBlit_p->Cfg.XYLMode == STBLIT_JOB_BLIT_XYL_TYPE_DEVICE))
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    switch (DataType)
    {
        case STBLIT_DATA_TYPE_FOREGROUND :
            /* Check Color versus current node configuration */
            if (JBlit_p->Cfg.Foreground.Type != STBLIT_INFO_TYPE_COLOR)
            {
                return(ST_ERROR_FEATURE_NOT_SUPPORTED);
            }
            if (Color_p->Type != JBlit_p->Cfg.Foreground.Info.Color.ColorType)
            {
                return(ST_ERROR_FEATURE_NOT_SUPPORTED);
            }

            /* Update JBlit structure : New Color  => Nothing to update*/

            /* Update nodes */
            Err = stblit_UpdateForegroundColor(JBlit_p,Color_p);
            break;

        case STBLIT_DATA_TYPE_BACKGROUND :
        case STBLIT_DATA_TYPE_DESTINATION :
        case STBLIT_DATA_TYPE_MASK :
        default :
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
            break;
    }

    return(Err);
 }



/*
--------------------------------------------------------------------------------
Set JobBlit bitmap API function
--------------------------------------------------------------------------------
*/
 ST_ErrorCode_t STBLIT_SetJobBlitBitmap(STBLIT_Handle_t         Handle,
                                        STBLIT_JobBlitHandle_t  JobBlitHandle,
                                        STBLIT_DataType_t   DataType,
                                        STGXOBJ_Bitmap_t*       Bitmap_p)
 {
    ST_ErrorCode_t      Err         = ST_NO_ERROR;
    stblit_Unit_t*      Unit_p      = (stblit_Unit_t*) Handle;
    stblit_JobBlit_t*   JBlit_p     = (stblit_JobBlit_t*) JobBlitHandle;
    stblit_Device_t*    Device_p;

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

    /* Check Job Blit handle validity */
    if (JobBlitHandle ==(STBLIT_JobBlitHandle_t)NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (JBlit_p->JBlitValidity != STBLIT_VALID_JOB_BLIT)
    {
        return(STBLIT_ERROR_INVALID_JOB_BLIT_HANDLE);
    }

    /* Check Job Blit Handles mismatch */
    if (((stblit_Job_t*)(JBlit_p->JobHandle))->Handle != Handle)
    {
        return(STBLIT_ERROR_JOB_BLIT_HANDLE_MISMATCH);
    }

    /* Check Parameter */
    if (Bitmap_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Not supported modes */
    if ((JBlit_p->Cfg.ConcatMode == TRUE) ||
        (JBlit_p->Cfg.XYLMode == STBLIT_JOB_BLIT_XYL_TYPE_DEVICE))
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    switch (DataType)
    {
        case STBLIT_DATA_TYPE_FOREGROUND :
            /* Check bitmap versus current node configuration */
            if (JBlit_p->Cfg.Foreground.Type != STBLIT_INFO_TYPE_BITMAP)
            {
                return(ST_ERROR_FEATURE_NOT_SUPPORTED);
            }
            if (Bitmap_p->BitmapType != JBlit_p->Cfg.Foreground.Info.Bitmap.BitmapType)
            {
                return(ST_ERROR_FEATURE_NOT_SUPPORTED);
            }
            if (Bitmap_p->ColorType != JBlit_p->Cfg.Foreground.Info.Bitmap.ColorType)
            {
                return(ST_ERROR_FEATURE_NOT_SUPPORTED);
            }
            if (JBlit_p->Cfg.Foreground.Info.Bitmap.IsSubByteFormat == TRUE)
            {
                if (Bitmap_p->SubByteFormat != JBlit_p->Cfg.Foreground.Info.Bitmap.SubByteFormat)
                {
                    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
                }
            }
            if (Bitmap_p->Width < JBlit_p->Cfg.Foreground.Info.Bitmap.BitmapWidth)
            {
                /* Check versus rectangle PositionX and Width (Line : if first pix rectangle is out of bitmap, same for last one)*/
                if ((JBlit_p->Cfg.Foreground.Info.Bitmap.Rectangle.PositionX + JBlit_p->Cfg.Foreground.Info.Bitmap.Rectangle.Width - 1)
                        > (Bitmap_p->Width - 1))
                {
                    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
                }
            }
            if (Bitmap_p->Height < JBlit_p->Cfg.Foreground.Info.Bitmap.BitmapHeight)
            {
                /* Check versus rectangle PositionY and Height (Column : if first pix rectangle is out of bitmap, same for last one)*/
                if ((JBlit_p->Cfg.Foreground.Info.Bitmap.Rectangle.PositionY + JBlit_p->Cfg.Foreground.Info.Bitmap.Rectangle.Height - 1)
                        > (Bitmap_p->Height - 1))
                {
                    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
                }
            }

            /* Update JBlit structure : New Foreground bitmap */
            JBlit_p->Cfg.Foreground.Info.Bitmap.BitmapWidth  = Bitmap_p->Width;
            JBlit_p->Cfg.Foreground.Info.Bitmap.BitmapHeight = Bitmap_p->Height;

            /* Update nodes */
            Err = stblit_UpdateForegroundBitmap(Device_p, JBlit_p,Bitmap_p);
            break;

        case STBLIT_DATA_TYPE_BACKGROUND :
            /* Check bitmap versus current node configuration */
            if (JBlit_p->Cfg.Background.Type != STBLIT_INFO_TYPE_BITMAP)
            {
                return(ST_ERROR_FEATURE_NOT_SUPPORTED);
            }
            if (Bitmap_p->BitmapType != JBlit_p->Cfg.Background.Info.Bitmap.BitmapType)
            {
                return(ST_ERROR_FEATURE_NOT_SUPPORTED);
            }
            if (Bitmap_p->ColorType != JBlit_p->Cfg.Background.Info.Bitmap.ColorType)
            {
                return(ST_ERROR_FEATURE_NOT_SUPPORTED);
            }
            if (JBlit_p->Cfg.Background.Info.Bitmap.IsSubByteFormat == TRUE)
            {
                if (Bitmap_p->SubByteFormat != JBlit_p->Cfg.Background.Info.Bitmap.SubByteFormat)
                {
                    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
                }
            }
            if (Bitmap_p->Width < JBlit_p->Cfg.Background.Info.Bitmap.BitmapWidth)
            {
                /* Check versus rectangle PositionX and Width (Line : if first pix rectangle is out of bitmap, same for last one)*/
                if ((JBlit_p->Cfg.Background.Info.Bitmap.Rectangle.PositionX + JBlit_p->Cfg.Background.Info.Bitmap.Rectangle.Width - 1)
                     > (Bitmap_p->Width - 1))
                {
                    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
                }
            }
            if (Bitmap_p->Height < JBlit_p->Cfg.Background.Info.Bitmap.BitmapHeight)
            {
                /* Check versus rectangle PositionY and Height (Column : if first pix rectangle is out of bitmap, same for last one)*/
                if ((JBlit_p->Cfg.Background.Info.Bitmap.Rectangle.PositionY + JBlit_p->Cfg.Background.Info.Bitmap.Rectangle.Height - 1)
                        > (Bitmap_p->Height - 1))
                {
                    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
                }
            }

            /* Update JBlit structure : New Background bitmap */
            JBlit_p->Cfg.Background.Info.Bitmap.BitmapWidth  = Bitmap_p->Width;
            JBlit_p->Cfg.Background.Info.Bitmap.BitmapHeight = Bitmap_p->Height;

            /* Update nodes */
            Err = stblit_UpdateBackgroundBitmap(Device_p, JBlit_p,Bitmap_p);
            break;

        case STBLIT_DATA_TYPE_DESTINATION :
            /* Check bitmap versus current node configuration */
            if (Bitmap_p->BitmapType != JBlit_p->Cfg.Destination.BitmapType)
            {
                return(ST_ERROR_FEATURE_NOT_SUPPORTED);
            }
            if (Bitmap_p->ColorType != JBlit_p->Cfg.Destination.ColorType)
            {
                return(ST_ERROR_FEATURE_NOT_SUPPORTED);
            }
            if (JBlit_p->Cfg.Destination.IsSubByteFormat == TRUE)
            {
                if (Bitmap_p->SubByteFormat != JBlit_p->Cfg.Destination.SubByteFormat)
                {
                    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
                }
            }
            if (Bitmap_p->Width < JBlit_p->Cfg.Destination.BitmapWidth)
            {
                /* Check versus rectangle PositionX and Width (Line : if first pix rectangle is out of bitmap, same for last one)*/
                if ((JBlit_p->Cfg.Destination.Rectangle.PositionX + JBlit_p->Cfg.Destination.Rectangle.Width - 1)> (Bitmap_p->Width - 1))
                {
                    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
                }
            }
            if (Bitmap_p->Height < JBlit_p->Cfg.Destination.BitmapHeight)
            {
                /* Check versus rectangle PositionY and Height (Column : if first pix rectangle is out of bitmap, same for last one)*/
                if ((JBlit_p->Cfg.Destination.Rectangle.PositionY + JBlit_p->Cfg.Destination.Rectangle.Height - 1)> (Bitmap_p->Height - 1))
                {
                    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
                }
            }

            /* Update JBlit structure : New Destination bitmap */
            JBlit_p->Cfg.Destination.BitmapWidth  = Bitmap_p->Width;
            JBlit_p->Cfg.Destination.BitmapHeight = Bitmap_p->Height;

            /* Update nodes */
            Err = stblit_UpdateDestinationBitmap(Device_p, JBlit_p,Bitmap_p);

            break;

        case STBLIT_DATA_TYPE_MASK :
            /* Check Mode mask versus current node configuration */
            if (JBlit_p->Cfg.MaskEnabled != TRUE)
            {
                return(ST_ERROR_FEATURE_NOT_SUPPORTED);
            }
            if (Bitmap_p->ColorType != JBlit_p->Cfg.MaskInfo.ColorType)
            {
                return(ST_ERROR_FEATURE_NOT_SUPPORTED);
            }
            if (JBlit_p->Cfg.MaskInfo.IsSubByteFormat == TRUE)
            {
                if (Bitmap_p->SubByteFormat != JBlit_p->Cfg.MaskInfo.SubByteFormat)
                {
                    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
                }
            }
            if (Bitmap_p->Width < JBlit_p->Cfg.MaskInfo.BitmapWidth)
            {
                /* Check versus rectangle PositionX and Width (Line : if first pix rectangle is out of bitmap, same for last one)*/
                if ((JBlit_p->Cfg.MaskInfo.Rectangle.PositionX + JBlit_p->Cfg.MaskInfo.Rectangle.Width - 1)> (Bitmap_p->Width - 1))
                {
                    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
                }
            }
            if (Bitmap_p->Height < JBlit_p->Cfg.MaskInfo.BitmapHeight)
            {
                /* Check versus rectangle PositionY and Height (Column : if first pix rectangle is out of bitmap, same for last one)*/
                if ((JBlit_p->Cfg.MaskInfo.Rectangle.PositionY + JBlit_p->Cfg.MaskInfo.Rectangle.Height - 1)> (Bitmap_p->Height - 1))
                {
                    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
                }
            }

            /* Update JBlit structure : New Mask bitmap */
            JBlit_p->Cfg.MaskInfo.BitmapWidth  = Bitmap_p->Width;
            JBlit_p->Cfg.MaskInfo.BitmapHeight = Bitmap_p->Height;

            /* Update nodes. Assume that mode is mask at this stage */
#if !defined (ST_5100) && !defined (ST_5105) && !defined (ST_5301) && !defined (ST_7109) && !defined (ST_5188) && !defined (ST_5525)\
 && !defined (ST_5107) && !defined(ST_7200)
            Err = stblit_UpdateMaskBitmap(Device_p, JBlit_p,Bitmap_p);
#endif

            break;

        default :
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
            break;
    }

    return(Err);
 }
#ifdef ST_OSWINCE

 /*
--------------------------------------------------------------------------------
CheckRectVsSize: checks that a rectangle is inside a bitmap (returns TRUE is problem)
--------------------------------------------------------------------------------
*/
static BOOL CheckRectVsSize(STGXOBJ_Rectangle_t* rect_p, int Width, int Height)
{
	return (   rect_p->PositionX < 0
		    || rect_p->PositionY < 0
		    || rect_p->PositionX + rect_p->Width > Width
		    || rect_p->PositionY + rect_p->Height > Height);
}

/*
--------------------------------------------------------------------------------
AdjustRectangle: move rectangle is too much at right or at bottom
--------------------------------------------------------------------------------
*/
static void AdjustRectangle(STGXOBJ_Rectangle_t* rect_p, int Width, int Height)
{
    if (rect_p->PositionX + rect_p->Width > Width)
        rect_p->PositionX -= rect_p->PositionX + rect_p->Width - Width;
    if (rect_p->PositionY + rect_p->Height > Height)
        rect_p->PositionY -= rect_p->PositionY + rect_p->Height - Height;
}

#endif




 /*
--------------------------------------------------------------------------------
Set JobBlit rectangle API function
--------------------------------------------------------------------------------
*/
#ifdef ST_OSWINCE
ST_ErrorCode_t STBLIT_SetJobBlitRectangle(STBLIT_Handle_t         Handle,
                                          STBLIT_JobBlitHandle_t  JobBlitHandle,
                                          STBLIT_DataType_t       DataType,
                                          STGXOBJ_Rectangle_t*    Rectangle_p)
{
    ST_ErrorCode_t      Err         = ST_NO_ERROR;
    stblit_Unit_t*      Unit_p      = (stblit_Unit_t*) Handle;
    stblit_JobBlit_t*   JBlit_p     = (stblit_JobBlit_t*) JobBlitHandle;
	stblit_Node_t*      Node_p      = NULL;
    stblit_Device_t*    Device_p;
    STGXOBJ_Rectangle_t fgndRectangle, bgndRectangle, destRectangle;
    STGXOBJ_Rectangle_t adjustedFgndRectangle;
    STGXOBJ_Rectangle_t adjustedFgndRectangleChroma; // when foreground bitmap is MB
	BOOL isNodeResize;
	U32 INS;

    /* Check Handle validity */
    if (Handle == (STBLIT_Handle_t)NULL)
        return ST_ERROR_INVALID_HANDLE;

    Device_p = Unit_p->Device_p ;

    if (Unit_p->UnitValidity != STBLIT_VALID_UNIT)
        return ST_ERROR_INVALID_HANDLE;

    /* Check Job Blit handle validity */
    if (JobBlitHandle ==(STBLIT_JobBlitHandle_t)NULL)
        return ST_ERROR_BAD_PARAMETER;

    if (JBlit_p->JBlitValidity != STBLIT_VALID_JOB_BLIT)
        return STBLIT_ERROR_INVALID_JOB_BLIT_HANDLE;

    /* Check Job Blit Handles mismatch */
    if (((stblit_Job_t*)(JBlit_p->JobHandle))->Handle != Handle)
        return STBLIT_ERROR_JOB_BLIT_HANDLE_MISMATCH;

    /* Check Parameter */
    if (Rectangle_p == NULL)
        return ST_ERROR_BAD_PARAMETER;

    /* Not supported modes */
    if ((JBlit_p->Cfg.ConcatMode == TRUE) ||
        (JBlit_p->Cfg.XYLMode == STBLIT_JOB_BLIT_XYL_TYPE_DEVICE))
        return ST_ERROR_FEATURE_NOT_SUPPORTED;

	// Tiling is not supported (TBD)
	if (JBlit_p->NbNodes != 1)
		return ST_ERROR_FEATURE_NOT_SUPPORTED;

	// Scanning configuration different from "down/right" is not supported
	if (JBlit_p->Cfg.ScanConfig != 0)
		return ST_ERROR_FEATURE_NOT_SUPPORTED;

	// Sanity check
    if ((Rectangle_p->Width == 0)    ||
        (Rectangle_p->Height == 0)   ||
        (Rectangle_p->PositionX < 0) ||
        (Rectangle_p->PositionY < 0))
		return ST_ERROR_FEATURE_NOT_SUPPORTED;

	if (DataType == STBLIT_DATA_TYPE_FOREGROUND && JBlit_p->Cfg.Foreground.Type != STBLIT_INFO_TYPE_BITMAP)
		return ST_ERROR_FEATURE_NOT_SUPPORTED;
	if (DataType == STBLIT_DATA_TYPE_BACKGROUND && JBlit_p->Cfg.Background.Type != STBLIT_INFO_TYPE_BITMAP)
		return ST_ERROR_FEATURE_NOT_SUPPORTED;

    // Is resizing enabled ?
	Node_p = (stblit_Node_t*)(JBlit_p->FirstNodeHandle);
    INS = (U32)STSYS_ReadRegMem32LE((void*)(&(Node_p->HWNode.BLT_INS)));
	isNodeResize = INS >> (STBLIT_INS_ENABLE_RESCALE_SHIFT) & STBLIT_INS_ENABLE_RESCALE_MASK;

	// Check that rectangles of background and destination are identical
	if (   JBlit_p->Cfg.Background.Type == STBLIT_INFO_TYPE_BITMAP
		&& (   JBlit_p->Cfg.Destination.Rectangle.PositionX != JBlit_p->Cfg.Background.Info.Bitmap.Rectangle.PositionX
            || JBlit_p->Cfg.Destination.Rectangle.PositionY != JBlit_p->Cfg.Background.Info.Bitmap.Rectangle.PositionY))
		return ST_ERROR_FEATURE_NOT_SUPPORTED; // manages only background == dest, TBD

	// Compute the rectangles we would like to set
    switch (DataType)
    {
        case STBLIT_DATA_TYPE_DESTINATION :
        case STBLIT_DATA_TYPE_BACKGROUND :
			bgndRectangle = destRectangle = *Rectangle_p;
			if (JBlit_p->Cfg.Foreground.Type == STBLIT_INFO_TYPE_BITMAP)
			{
				if (isNodeResize)
				{
					fgndRectangle = JBlit_p->Cfg.Foreground.Info.Bitmap.Rectangle; // keep same rectangle
				}
				else
				{
					fgndRectangle.PositionX = JBlit_p->Cfg.Foreground.Info.Bitmap.Rectangle.PositionX;
					fgndRectangle.PositionY = JBlit_p->Cfg.Foreground.Info.Bitmap.Rectangle.PositionY;
					fgndRectangle.Width = Rectangle_p->Width;
					fgndRectangle.Height = Rectangle_p->Height;
                    AdjustRectangle(&fgndRectangle, JBlit_p->Cfg.Foreground.Info.Bitmap.BitmapWidth, JBlit_p->Cfg.Foreground.Info.Bitmap.BitmapHeight);
				}
			}
			else
				fgndRectangle = *Rectangle_p;
			break;
        case STBLIT_DATA_TYPE_FOREGROUND :
			fgndRectangle = *Rectangle_p;
			if (isNodeResize)
			{
				bgndRectangle = destRectangle = JBlit_p->Cfg.Destination.Rectangle;
			}
			else
			{
				destRectangle.PositionX = JBlit_p->Cfg.Destination.Rectangle.PositionX;
				destRectangle.PositionY = JBlit_p->Cfg.Destination.Rectangle.PositionY;
				destRectangle.Width = Rectangle_p->Width;
				destRectangle.Height = Rectangle_p->Height;
                AdjustRectangle(&destRectangle, JBlit_p->Cfg.Destination.BitmapWidth, JBlit_p->Cfg.Destination.BitmapHeight);
				bgndRectangle = destRectangle;
			}
			break;
		case STBLIT_DATA_TYPE_MASK :
		default:
			return ST_ERROR_FEATURE_NOT_SUPPORTED;
	}

	// Check rectangles vs. dimensions of bitmaps
	if (CheckRectVsSize(&destRectangle, JBlit_p->Cfg.Destination.BitmapWidth, JBlit_p->Cfg.Destination.BitmapHeight))
		return ST_ERROR_FEATURE_NOT_SUPPORTED;
	if (   JBlit_p->Cfg.Foreground.Type == STBLIT_INFO_TYPE_BITMAP
		&& CheckRectVsSize(&fgndRectangle, JBlit_p->Cfg.Foreground.Info.Bitmap.BitmapWidth, JBlit_p->Cfg.Foreground.Info.Bitmap.BitmapHeight))
		return ST_ERROR_FEATURE_NOT_SUPPORTED;
	if (   JBlit_p->Cfg.Background.Type == STBLIT_INFO_TYPE_BITMAP
		&& CheckRectVsSize(&bgndRectangle, JBlit_p->Cfg.Background.Info.Bitmap.BitmapWidth, JBlit_p->Cfg.Background.Info.Bitmap.BitmapHeight))
		return ST_ERROR_FEATURE_NOT_SUPPORTED;

	// === Update job blit nodes ===
    // Set resizing filters (as a side-effect, updates the foreground rectangle)
	adjustedFgndRectangle = fgndRectangle;
	if (isNodeResize)
	{
		if (fgndRectangle.Width > STBLIT_MAX_SRC_WIDTH - HSRC_NB_LINE_REPEATED_LEFT_BORDER - HSRC_NB_REAL_LINE_LEFT_BORDER )
			return ST_ERROR_FEATURE_NOT_SUPPORTED; // must be tiled
        stblit_UpdateFilters(Device_p, JBlit_p, &destRectangle, JBlit_p->Cfg.SrcMB, &adjustedFgndRectangle, &adjustedFgndRectangleChroma);
    }
	// Update blit rectangles
    Err = stblit_UpdateForegroundRectangle(JBlit_p, JBlit_p->Cfg.SrcMB, &adjustedFgndRectangle, &adjustedFgndRectangleChroma);
	if (Err != ST_NO_ERROR)
		return Err;
    Err = stblit_UpdateBackgroundRectangle(JBlit_p, &bgndRectangle);
	if (Err != ST_NO_ERROR)
		return Err;
    Err = stblit_UpdateDestinationRectangle(JBlit_p, &destRectangle);
	if (Err != ST_NO_ERROR)
		return Err;

	// Update job blit
    JBlit_p->Cfg.Foreground.Info.Bitmap.Rectangle = fgndRectangle;
    JBlit_p->Cfg.Background.Info.Bitmap.Rectangle = bgndRectangle;
    JBlit_p->Cfg.Destination.Rectangle = destRectangle;

	return ST_NO_ERROR;
}

#else /*! ST_OSWINCE*/

 ST_ErrorCode_t STBLIT_SetJobBlitRectangle(STBLIT_Handle_t         Handle,
                                           STBLIT_JobBlitHandle_t  JobBlitHandle,
                                           STBLIT_DataType_t       DataType,
                                           STGXOBJ_Rectangle_t*    Rectangle_p)
{
    ST_ErrorCode_t      Err         = ST_NO_ERROR;
    stblit_Unit_t*      Unit_p      = (stblit_Unit_t*) Handle;
    stblit_JobBlit_t*   JBlit_p     = (stblit_JobBlit_t*) JobBlitHandle;
    stblit_Device_t*    Device_p;

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

    /* Check Job Blit handle validity */
    if (JobBlitHandle ==(STBLIT_JobBlitHandle_t)NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (JBlit_p->JBlitValidity != STBLIT_VALID_JOB_BLIT)
    {
        return(STBLIT_ERROR_INVALID_JOB_BLIT_HANDLE);
    }

    /* Check Job Blit Handles mismatch */
    if (((stblit_Job_t*)(JBlit_p->JobHandle))->Handle != Handle)
    {
        return(STBLIT_ERROR_JOB_BLIT_HANDLE_MISMATCH);
    }

    /* Check Parameter */
    if (Rectangle_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Not supported modes */
    if ((JBlit_p->Cfg.ConcatMode == TRUE) ||
        (JBlit_p->Cfg.XYLMode == STBLIT_JOB_BLIT_XYL_TYPE_DEVICE))
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    switch (DataType)
    {
        case STBLIT_DATA_TYPE_FOREGROUND :
            /* Check rectangle versus current node configuration */
            if ((Rectangle_p->Width == 0)    ||
                (Rectangle_p->Height == 0)   ||
                (Rectangle_p->PositionX < 0) ||
                (Rectangle_p->PositionY < 0))
            {
                return(ST_ERROR_FEATURE_NOT_SUPPORTED);
            }
            if (((Rectangle_p->PositionX + Rectangle_p->Width -1) > (JBlit_p->Cfg.Foreground.Info.Bitmap.BitmapWidth - 1)) ||
                ((Rectangle_p->PositionY + Rectangle_p->Height -1) > (JBlit_p->Cfg.Foreground.Info.Bitmap.BitmapHeight - 1)))
            {
                return(ST_ERROR_FEATURE_NOT_SUPPORTED);
            }
#if !defined (ST_5100) && !defined (ST_5105) && !defined (ST_5301) && !defined (ST_7109) && !defined (ST_5188) && !defined (ST_5525)\
 && !defined (ST_5107) && !defined(ST_7200)
            if ((Rectangle_p->Width != JBlit_p->Cfg.Foreground.Info.Bitmap.Rectangle.Width) ||
                (Rectangle_p->Height != JBlit_p->Cfg.Foreground.Info.Bitmap.Rectangle.Height))
            {
                return(ST_ERROR_FEATURE_NOT_SUPPORTED);
                /* Be careful, rectangle Foreground/Mask (in multi pass) must heve the same size !*/
/*               stblit_UpdateResize(Device_p,JBlit_p, Rectangle_p)*/
            }
#endif
            /* Update JBlit structure : New Rectangle */
            JBlit_p->Cfg.Foreground.Info.Bitmap.Rectangle.PositionX    = Rectangle_p->PositionX;
            JBlit_p->Cfg.Foreground.Info.Bitmap.Rectangle.PositionY    = Rectangle_p->PositionY;
            JBlit_p->Cfg.Foreground.Info.Bitmap.Rectangle.Width        = Rectangle_p->Width;
            JBlit_p->Cfg.Foreground.Info.Bitmap.Rectangle.Height       = Rectangle_p->Height;

            /* Update nodes */
            Err = stblit_UpdateForegroundRectangle(JBlit_p, FALSE, Rectangle_p, NULL);

            break;

        case STBLIT_DATA_TYPE_BACKGROUND :
            /* Check rectangle versus current node configuration */
            if ((Rectangle_p->Width == 0)    ||
                (Rectangle_p->Height == 0)   ||
                (Rectangle_p->PositionX < 0) ||
                (Rectangle_p->PositionY < 0))
            {
                return(ST_ERROR_FEATURE_NOT_SUPPORTED);
            }
            if (((Rectangle_p->PositionX + Rectangle_p->Width -1) > (JBlit_p->Cfg.Background.Info.Bitmap.BitmapWidth - 1)) ||
                ((Rectangle_p->PositionY + Rectangle_p->Height -1) > (JBlit_p->Cfg.Background.Info.Bitmap.BitmapHeight - 1)))
            {
                return(ST_ERROR_FEATURE_NOT_SUPPORTED);
            }
            /* Destination rectangle must have same size as Background one. Make sure consistent with Destination bitmap */
/*            if (((JBlit_p->Cfg.Destination.Rectangle.PositionX + Rectangle_p->Width -1) >*/
/*                (JBlit_p->Cfg.Destination.BitmapWidth - 1)) ||*/
/*                ((JBlit_p->Cfg.Destination.Rectangle.PositionY + Rectangle_p->Height -1) >*/
/*                (JBlit_p->Cfg.Destination.BitmapHeight - 1)))*/
/*            {*/
/*                return(ST_ERROR_FEATURE_NOT_SUPPORTED);*/
/*            }*/
#if !defined (ST_5100) && !defined (ST_5105) && !defined (ST_5301) && !defined (ST_7109) && !defined (ST_5188) && !defined (ST_5525)\
 && !defined (ST_5107) && !defined(ST_7200)
            if ((Rectangle_p->Width  != JBlit_p->Cfg.Background.Info.Bitmap.Rectangle.Width) ||
               (Rectangle_p->Height != JBlit_p->Cfg.Background.Info.Bitmap.Rectangle.Height))
            {
                return(ST_ERROR_FEATURE_NOT_SUPPORTED);
                /* Be careful, rectangle Background/Dest must heve the same size !*/
/*                stblit_UpdateResize(Device_p,JBlit_p, Rectangle_p)*/
            }
#endif
            /* Update JBlit structure : New Rectangle */
            JBlit_p->Cfg.Background.Info.Bitmap.Rectangle.PositionX    = Rectangle_p->PositionX;
            JBlit_p->Cfg.Background.Info.Bitmap.Rectangle.PositionY    = Rectangle_p->PositionY;
            JBlit_p->Cfg.Background.Info.Bitmap.Rectangle.Width        = Rectangle_p->Width;
            JBlit_p->Cfg.Background.Info.Bitmap.Rectangle.Height       = Rectangle_p->Height;

            /* Update nodes */
            Err = stblit_UpdateBackgroundRectangle(JBlit_p,Rectangle_p);

            break;

        case STBLIT_DATA_TYPE_DESTINATION :
            /* Check rectangle versus current node configuration */
           if ((Rectangle_p->Width == 0)    ||
               (Rectangle_p->Height == 0)   ||
               (Rectangle_p->PositionX < 0) ||
               (Rectangle_p->PositionY < 0))
           {
               return(ST_ERROR_FEATURE_NOT_SUPPORTED);
           }
           if (((Rectangle_p->PositionX + Rectangle_p->Width -1) > (JBlit_p->Cfg.Destination.BitmapWidth - 1)) ||
               ((Rectangle_p->PositionY + Rectangle_p->Height -1) > (JBlit_p->Cfg.Destination.BitmapHeight - 1)))
           {
               return(ST_ERROR_FEATURE_NOT_SUPPORTED);
           }
#if !defined (ST_5100) && !defined (ST_5105) && !defined (ST_5301) && !defined (ST_7109) && !defined (ST_5188) && !defined (ST_5525)\
 && !defined (ST_5107) && !defined(ST_7200)
           if ((Rectangle_p->Width != JBlit_p->Cfg.Destination.Rectangle.Width) ||
               (Rectangle_p->Height != JBlit_p->Cfg.Destination.Rectangle.Height))
           {
               return(ST_ERROR_FEATURE_NOT_SUPPORTED);
               /* Be careful, rectangle BAckground/Dest must heve the same size !*/
            /*        stblit_UpdateResize(Device_p,JBlit_p, Rectangle_p)*/
           }
#endif
            /* Background rectangle must have same size as Destination one. Make sure consistent with Background bitmap */
/*            if (((JBlit_p->Cfg.Background.Info.Bitmap.Rectangle.PositionX + Rectangle_p->Width -1) >*/
/*                (JBlit_p->Cfg.Background.Info.Bitmap.BitmapWidth - 1)) ||*/
/*                ((JBlit_p->Cfg.Background.Info.Bitmap.Rectangle.PositionY + Rectangle_p->Height -1) >*/
/*                (JBlit_p->Cfg.Background.Info.Bitmap.BitmapHeight - 1)))*/
/*            {*/
/*                return(ST_ERROR_FEATURE_NOT_SUPPORTED);*/
/*            }*/


            /* Update JBlit structure : New Rectangle */
            JBlit_p->Cfg.Destination.Rectangle.PositionX    = Rectangle_p->PositionX;
            JBlit_p->Cfg.Destination.Rectangle.PositionY    = Rectangle_p->PositionY;
            JBlit_p->Cfg.Destination.Rectangle.Width        = Rectangle_p->Width;
            JBlit_p->Cfg.Destination.Rectangle.Height       = Rectangle_p->Height;

            /* Update nodes */
            Err = stblit_UpdateDestinationRectangle(JBlit_p,Rectangle_p);
            break;

        case STBLIT_DATA_TYPE_MASK :
        default :
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
            break;
    }

    return(Err);
}
#endif /* ST_OSWINCE*/

#ifdef ST_OSWINCE
/*
--------------------------------------------------------------------------------
Set JobBlit bitmap & rectangle API function
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STBLIT_SetJobBlitBitmapAndRectangle(STBLIT_Handle_t        Handle,
                                                   STBLIT_JobBlitHandle_t JobBlitHandle,
                                                   STBLIT_DataType_t      DataType,
                                                   STGXOBJ_Bitmap_t*      Bitmap_p,
                                                   STGXOBJ_Rectangle_t*   Rectangle_p
)
{
    ST_ErrorCode_t      Err         = ST_NO_ERROR;
    stblit_Unit_t*      Unit_p      = (stblit_Unit_t*) Handle;
    stblit_JobBlit_t*   JBlit_p     = (stblit_JobBlit_t*) JobBlitHandle;
    stblit_Device_t*    Device_p;
    stblit_Node_t*      Node_p      = NULL;
    STGXOBJ_Rectangle_t fgndRectangle, bgndRectangle, destRectangle;
    STGXOBJ_Rectangle_t adjustedFgndRectangle;
    STGXOBJ_Rectangle_t adjustedFgndRectangleChroma; // for MB bitmaps
	BOOL isNodeResize;
	U32 INS;

    /* Check Handle validity */
    if (Handle == (STBLIT_Handle_t)NULL)
        return ST_ERROR_INVALID_HANDLE;

    Device_p = Unit_p->Device_p ;

    if (Unit_p->UnitValidity != STBLIT_VALID_UNIT)
        return ST_ERROR_INVALID_HANDLE;

    /* Check Job Blit handle validity */
    if (JobBlitHandle ==(STBLIT_JobBlitHandle_t)NULL)
        return ST_ERROR_BAD_PARAMETER;

    if (JBlit_p->JBlitValidity != STBLIT_VALID_JOB_BLIT)
        return STBLIT_ERROR_INVALID_JOB_BLIT_HANDLE;

    /* Check Job Blit Handles mismatch */
    if (((stblit_Job_t*)(JBlit_p->JobHandle))->Handle != Handle)
        return STBLIT_ERROR_JOB_BLIT_HANDLE_MISMATCH;

    /* Check Parameter */
    if (Bitmap_p == NULL)
        return ST_ERROR_BAD_PARAMETER;

    /* Not supported modes */
    if ((JBlit_p->Cfg.ConcatMode == TRUE) ||
        (JBlit_p->Cfg.XYLMode == STBLIT_JOB_BLIT_XYL_TYPE_DEVICE))
        return ST_ERROR_FEATURE_NOT_SUPPORTED;

    /* Check Parameter */
    if (Rectangle_p == NULL)
        return ST_ERROR_BAD_PARAMETER;

	// Tiling is not supported (TBD)
	if (JBlit_p->NbNodes != 1)
		return ST_ERROR_FEATURE_NOT_SUPPORTED;

	// Scanning configuration different from "down/right" is not supported
	if (JBlit_p->Cfg.ScanConfig != 0)
		return ST_ERROR_FEATURE_NOT_SUPPORTED;

	// Sanity check
    if ((Rectangle_p->Width == 0)    ||
        (Rectangle_p->Height == 0)   ||
        (Rectangle_p->PositionX < 0) ||
        (Rectangle_p->PositionY < 0))
		return ST_ERROR_FEATURE_NOT_SUPPORTED;

    if (CheckRectVsSize(Rectangle_p, Bitmap_p->Width, Bitmap_p->Height))
        return ST_ERROR_FEATURE_NOT_SUPPORTED;

	// Check that rectangles of background and destination are identical
	if (   JBlit_p->Cfg.Background.Type == STBLIT_INFO_TYPE_BITMAP
		&& (   JBlit_p->Cfg.Destination.Rectangle.PositionX != JBlit_p->Cfg.Background.Info.Bitmap.Rectangle.PositionX
            || JBlit_p->Cfg.Destination.Rectangle.PositionY != JBlit_p->Cfg.Background.Info.Bitmap.Rectangle.PositionY))
		return ST_ERROR_FEATURE_NOT_SUPPORTED; // manages only background == dest, TBD

    switch (DataType)
    {
        case STBLIT_DATA_TYPE_FOREGROUND :
            /* Check bitmap versus current node configuration */
            if (JBlit_p->Cfg.Foreground.Type != STBLIT_INFO_TYPE_BITMAP)
                return ST_ERROR_FEATURE_NOT_SUPPORTED;
            if (Bitmap_p->BitmapType != JBlit_p->Cfg.Foreground.Info.Bitmap.BitmapType)
                return ST_ERROR_FEATURE_NOT_SUPPORTED;
            if (Bitmap_p->ColorType != JBlit_p->Cfg.Foreground.Info.Bitmap.ColorType)
                return ST_ERROR_FEATURE_NOT_SUPPORTED;
            if (JBlit_p->Cfg.Foreground.Info.Bitmap.IsSubByteFormat)
            {
                if (Bitmap_p->SubByteFormat != JBlit_p->Cfg.Foreground.Info.Bitmap.SubByteFormat)
                    return ST_ERROR_FEATURE_NOT_SUPPORTED;
            }

            /* Update JBlit structure : New Foreground bitmap */
            JBlit_p->Cfg.Foreground.Info.Bitmap.BitmapWidth  = Bitmap_p->Width;
            JBlit_p->Cfg.Foreground.Info.Bitmap.BitmapHeight = Bitmap_p->Height;

            /* Update nodes */
            Err = stblit_UpdateForegroundBitmap(Device_p, JBlit_p, Bitmap_p);
            break;

        case STBLIT_DATA_TYPE_BACKGROUND :
            /* Check bitmap versus current node configuration */
            if (JBlit_p->Cfg.Background.Type != STBLIT_INFO_TYPE_BITMAP)
                return ST_ERROR_FEATURE_NOT_SUPPORTED;
            if (Bitmap_p->BitmapType != JBlit_p->Cfg.Background.Info.Bitmap.BitmapType)
                return ST_ERROR_FEATURE_NOT_SUPPORTED;
            if (Bitmap_p->ColorType != JBlit_p->Cfg.Background.Info.Bitmap.ColorType)
                return ST_ERROR_FEATURE_NOT_SUPPORTED;
            if (JBlit_p->Cfg.Background.Info.Bitmap.IsSubByteFormat)
            {
                if (Bitmap_p->SubByteFormat != JBlit_p->Cfg.Background.Info.Bitmap.SubByteFormat)
                    return ST_ERROR_FEATURE_NOT_SUPPORTED;
            }

            /* Update JBlit structure : New Background bitmap */
            JBlit_p->Cfg.Background.Info.Bitmap.BitmapWidth  = Bitmap_p->Width;
            JBlit_p->Cfg.Background.Info.Bitmap.BitmapHeight = Bitmap_p->Height;

            /* Update nodes */
            Err = stblit_UpdateBackgroundBitmap(Device_p, JBlit_p,Bitmap_p);
            break;

        case STBLIT_DATA_TYPE_DESTINATION :
            /* Check bitmap versus current node configuration */
            if (Bitmap_p->BitmapType != JBlit_p->Cfg.Destination.BitmapType)
                return ST_ERROR_FEATURE_NOT_SUPPORTED;
            if (Bitmap_p->ColorType != JBlit_p->Cfg.Destination.ColorType)
                return ST_ERROR_FEATURE_NOT_SUPPORTED;
            if (JBlit_p->Cfg.Destination.IsSubByteFormat)
            {
                if (Bitmap_p->SubByteFormat != JBlit_p->Cfg.Destination.SubByteFormat)
                    return ST_ERROR_FEATURE_NOT_SUPPORTED;
            }

            /* Update JBlit structure : New Destination bitmap */
            JBlit_p->Cfg.Destination.BitmapWidth  = Bitmap_p->Width;
            JBlit_p->Cfg.Destination.BitmapHeight = Bitmap_p->Height;

            /* Update nodes */
            Err = stblit_UpdateDestinationBitmap(Device_p, JBlit_p,Bitmap_p);
            break;

        case STBLIT_DATA_TYPE_MASK :
            return ST_ERROR_FEATURE_NOT_SUPPORTED; // cant set rectangle of mask
            break;

        default :
            return ST_ERROR_FEATURE_NOT_SUPPORTED;
            break;
    }

    // Is resizing enabled ?
	Node_p = (stblit_Node_t*)(JBlit_p->FirstNodeHandle);
    INS = (U32)STSYS_ReadRegMem32LE((void*)(&(Node_p->HWNode.BLT_INS)));
	isNodeResize = INS >> (STBLIT_INS_ENABLE_RESCALE_SHIFT) & STBLIT_INS_ENABLE_RESCALE_MASK;

	// Compute the rectangles we would like to set
    switch (DataType)
    {
        case STBLIT_DATA_TYPE_DESTINATION :
        case STBLIT_DATA_TYPE_BACKGROUND :
			bgndRectangle = destRectangle = *Rectangle_p;
			if (JBlit_p->Cfg.Foreground.Type == STBLIT_INFO_TYPE_BITMAP)
			{
				if (isNodeResize)
				{
					fgndRectangle = JBlit_p->Cfg.Foreground.Info.Bitmap.Rectangle; // keep same rectangle
				}
				else
				{
					fgndRectangle.PositionX = JBlit_p->Cfg.Foreground.Info.Bitmap.Rectangle.PositionX;
					fgndRectangle.PositionY = JBlit_p->Cfg.Foreground.Info.Bitmap.Rectangle.PositionY;
					fgndRectangle.Width = Rectangle_p->Width;
					fgndRectangle.Height = Rectangle_p->Height;
                    AdjustRectangle(&fgndRectangle, JBlit_p->Cfg.Foreground.Info.Bitmap.BitmapWidth, JBlit_p->Cfg.Foreground.Info.Bitmap.BitmapHeight);
				}
			}
			else
				fgndRectangle = *Rectangle_p;
			break;
        case STBLIT_DATA_TYPE_FOREGROUND :
			fgndRectangle = *Rectangle_p;
			if (isNodeResize)
			{
				bgndRectangle = destRectangle = JBlit_p->Cfg.Destination.Rectangle;
			}
			else
			{
				destRectangle.PositionX = JBlit_p->Cfg.Destination.Rectangle.PositionX;
				destRectangle.PositionY = JBlit_p->Cfg.Destination.Rectangle.PositionY;
				destRectangle.Width = Rectangle_p->Width;
				destRectangle.Height = Rectangle_p->Height;
                AdjustRectangle(&destRectangle, JBlit_p->Cfg.Destination.BitmapWidth, JBlit_p->Cfg.Destination.BitmapHeight);
				bgndRectangle = destRectangle;
			}
			break;
		case STBLIT_DATA_TYPE_MASK :
		default:
			return ST_ERROR_FEATURE_NOT_SUPPORTED;
	}

	// Check rectangles vs. dimensions of bitmaps
	if (CheckRectVsSize(&destRectangle, JBlit_p->Cfg.Destination.BitmapWidth, JBlit_p->Cfg.Destination.BitmapHeight))
		return ST_ERROR_FEATURE_NOT_SUPPORTED;
	if (   JBlit_p->Cfg.Foreground.Type == STBLIT_INFO_TYPE_BITMAP
		&& CheckRectVsSize(&fgndRectangle, JBlit_p->Cfg.Foreground.Info.Bitmap.BitmapWidth, JBlit_p->Cfg.Foreground.Info.Bitmap.BitmapHeight))
		return ST_ERROR_FEATURE_NOT_SUPPORTED;
	if (   JBlit_p->Cfg.Background.Type == STBLIT_INFO_TYPE_BITMAP
		&& CheckRectVsSize(&bgndRectangle, JBlit_p->Cfg.Background.Info.Bitmap.BitmapWidth, JBlit_p->Cfg.Background.Info.Bitmap.BitmapHeight))
		return ST_ERROR_FEATURE_NOT_SUPPORTED;

	// === Update job blit nodes ===
	// Set resizing filters (as a side-effect, updates the foreground rectangle)
	adjustedFgndRectangle = fgndRectangle;
	if (isNodeResize)
	{
		if (fgndRectangle.Width > STBLIT_MAX_SRC_WIDTH - HSRC_NB_LINE_REPEATED_LEFT_BORDER - HSRC_NB_REAL_LINE_LEFT_BORDER )
			return ST_ERROR_FEATURE_NOT_SUPPORTED; // must be tiled
        stblit_UpdateFilters(Device_p, JBlit_p, &destRectangle,
                             JBlit_p->Cfg.SrcMB,
                             &adjustedFgndRectangle, &adjustedFgndRectangleChroma);
	}
	// Update blit rectangles
    Err = stblit_UpdateForegroundRectangle(JBlit_p, JBlit_p->Cfg.SrcMB,
                                           &adjustedFgndRectangle, &adjustedFgndRectangleChroma);

	if (Err != ST_NO_ERROR)
		return Err;
    Err = stblit_UpdateBackgroundRectangle(JBlit_p, &bgndRectangle);
	if (Err != ST_NO_ERROR)
		return Err;
    Err = stblit_UpdateDestinationRectangle(JBlit_p, &destRectangle);
	if (Err != ST_NO_ERROR)
		return Err;

	// Update job blit
    JBlit_p->Cfg.Foreground.Info.Bitmap.Rectangle = fgndRectangle;
    JBlit_p->Cfg.Background.Info.Bitmap.Rectangle = bgndRectangle;
    JBlit_p->Cfg.Destination.Rectangle = destRectangle;
    return ST_NO_ERROR;
}

#endif

 /*
--------------------------------------------------------------------------------
Set JobBlit Clipping rectangle API function
--------------------------------------------------------------------------------
*/

ST_ErrorCode_t STBLIT_SetJobBlitClipRectangle(STBLIT_Handle_t          Handle,
                                              STBLIT_JobBlitHandle_t   JobBlitHandle,
                                              BOOL                     WriteInsideClipRectangle,
                                              STGXOBJ_Rectangle_t*     ClipRectangle_p)
{
    ST_ErrorCode_t      Err         = ST_NO_ERROR;
    stblit_Unit_t*      Unit_p      = (stblit_Unit_t*) Handle;
    stblit_JobBlit_t*   JBlit_p     = (stblit_JobBlit_t*) JobBlitHandle;
    stblit_Device_t*    Device_p;

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

    /* Check Job Blit handle validity */
    if (JobBlitHandle ==(STBLIT_JobBlitHandle_t)NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (JBlit_p->JBlitValidity != STBLIT_VALID_JOB_BLIT)
    {
        return(STBLIT_ERROR_INVALID_JOB_BLIT_HANDLE);
    }

    /* Check Job Blit Handles mismatch */
    if (((stblit_Job_t*)(JBlit_p->JobHandle))->Handle != Handle)
    {
        return(STBLIT_ERROR_JOB_BLIT_HANDLE_MISMATCH);
    }

    /* Check Parameter */
    if (ClipRectangle_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Not supported modes */
    if ((JBlit_p->Cfg.ConcatMode == TRUE) ||
        (JBlit_p->Cfg.XYLMode == STBLIT_JOB_BLIT_XYL_TYPE_DEVICE))
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }


    /* Update  */
    Err = stblit_UpdateClipRectangle(JBlit_p,WriteInsideClipRectangle,ClipRectangle_p);

    return(Err);
}


/*
--------------------------------------------------------------------------------
Set JobBlit Mask word API function
--------------------------------------------------------------------------------
*/

ST_ErrorCode_t STBLIT_SetJobBlitMaskWord(STBLIT_Handle_t          Handle,
                                         STBLIT_JobBlitHandle_t   JobBlitHandle,
                                         U32                      MaskWord)
{

#if !defined(ST_7109) && !defined(ST_7200)
    return ST_ERROR_FEATURE_NOT_SUPPORTED;
#else /* !ST_7109 */

    ST_ErrorCode_t      Err         = ST_NO_ERROR;
    stblit_Unit_t*      Unit_p      = (stblit_Unit_t*) Handle;
    stblit_JobBlit_t*   JBlit_p     = (stblit_JobBlit_t*) JobBlitHandle;
    stblit_Device_t*    Device_p;

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

    /* Check Job Blit handle validity */
    if (JobBlitHandle ==(STBLIT_JobBlitHandle_t)NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (JBlit_p->JBlitValidity != STBLIT_VALID_JOB_BLIT)
    {
        return(STBLIT_ERROR_INVALID_JOB_BLIT_HANDLE);
    }

    /* Check Job Blit Handles mismatch */
    if (((stblit_Job_t*)(JBlit_p->JobHandle))->Handle != Handle)
    {
        return(STBLIT_ERROR_JOB_BLIT_HANDLE_MISMATCH);
    }

    /* Not supported modes */
    if ((JBlit_p->Cfg.ConcatMode == TRUE) ||
        (JBlit_p->Cfg.XYLMode == STBLIT_JOB_BLIT_XYL_TYPE_DEVICE))
        /*(JBlit_p->Cfg.Background.Type == STBLIT_INFO_TYPE_NONE))*/
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    /* Update  */
    Err = stblit_UpdateMaskWord(JBlit_p,MaskWord);

    return(Err);
#endif
}

/*
--------------------------------------------------------------------------------
Set JobBlit Color Key API function
--------------------------------------------------------------------------------
*/

ST_ErrorCode_t STBLIT_SetJobBlitColorKey(STBLIT_Handle_t          Handle,
                                         STBLIT_JobBlitHandle_t   JobBlitHandle,
                                         STBLIT_ColorKeyCopyMode_t ColorKeyCopyMode,
                                         STGXOBJ_ColorKey_t*       ColorKey_p)
{

#if !defined(ST_7109) && !defined(ST_7200)
    return ST_ERROR_FEATURE_NOT_SUPPORTED;
#else /* !ST_7109 */

    ST_ErrorCode_t      Err         = ST_NO_ERROR;
    stblit_Unit_t*      Unit_p      = (stblit_Unit_t*) Handle;
    stblit_JobBlit_t*   JBlit_p     = (stblit_JobBlit_t*) JobBlitHandle;
    stblit_Device_t*    Device_p;


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

    /* Check Job Blit handle validity */
    if (JobBlitHandle ==(STBLIT_JobBlitHandle_t)NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (JBlit_p->JBlitValidity != STBLIT_VALID_JOB_BLIT)
    {
        return(STBLIT_ERROR_INVALID_JOB_BLIT_HANDLE);
    }

    /* Check Job Blit Handles mismatch */
    if (((stblit_Job_t*)(JBlit_p->JobHandle))->Handle != Handle)
    {
        return(STBLIT_ERROR_JOB_BLIT_HANDLE_MISMATCH);
    }

    /* Check Parameter */
    if ((ColorKey_p == NULL) || (ColorKeyCopyMode == STBLIT_COLOR_KEY_MODE_NONE))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Not supported modes */
    if ((JBlit_p->Cfg.ConcatMode == TRUE) ||
        (JBlit_p->Cfg.XYLMode == STBLIT_JOB_BLIT_XYL_TYPE_DEVICE) ||
        ((JBlit_p->Cfg.Background.Type == STBLIT_INFO_TYPE_NONE) &&
         (ColorKeyCopyMode == STBLIT_COLOR_KEY_MODE_DST)))
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }
    /* Update  */
    Err = stblit_UpdateColorKey(JBlit_p,ColorKeyCopyMode,ColorKey_p);
    return(Err);
#endif
}


#ifdef ST_OSWINCE
/*
--------------------------------------------------------------------------------
Set JobBlit Global Alpha API function
--------------------------------------------------------------------------------
*/

ST_ErrorCode_t STBLIT_SetJobBlitGlobalAlpha(STBLIT_Handle_t          Handle,
                                            STBLIT_JobBlitHandle_t   JobBlitHandle,
											U8                       GlobalAlpha)
{
    ST_ErrorCode_t      Err         = ST_NO_ERROR;
    stblit_Unit_t*      Unit_p      = (stblit_Unit_t*) Handle;
    stblit_JobBlit_t*   JBlit_p     = (stblit_JobBlit_t*) JobBlitHandle;
    stblit_Device_t*    Device_p;

#ifndef ST_7109
    return ST_ERROR_FEATURE_NOT_SUPPORTED;
#else /* !ST_7109 */

    /* Check Handle validity */
    if (Handle == (STBLIT_Handle_t)NULL)
        return ST_ERROR_INVALID_HANDLE;

    Device_p = Unit_p->Device_p ;

    if (Unit_p->UnitValidity != STBLIT_VALID_UNIT)
        return ST_ERROR_INVALID_HANDLE;

    /* Check Job Blit handle validity */
    if (JobBlitHandle == (STBLIT_JobBlitHandle_t)NULL)
        return ST_ERROR_BAD_PARAMETER;

    if (JBlit_p->JBlitValidity != STBLIT_VALID_JOB_BLIT)
        return STBLIT_ERROR_INVALID_JOB_BLIT_HANDLE;

    /* Check Job Blit Handles mismatch */
    if (((stblit_Job_t*)(JBlit_p->JobHandle))->Handle != Handle)
        return STBLIT_ERROR_JOB_BLIT_HANDLE_MISMATCH;

    /* Not supported modes */
    if (!JBlit_p->Cfg.BlendOp)
        return ST_ERROR_FEATURE_NOT_SUPPORTED;

    /* Update  */
    Err = stblit_UpdateGlobalAlpha(JBlit_p, GlobalAlpha);
    return Err;

#endif /*7109*/
}

#endif
/* End of blt_job.c */
