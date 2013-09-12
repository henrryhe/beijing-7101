/*******************************************************************************

File name   : blt_be.c

Description : back-end source file

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
13 Jun 2000        Created                                          TM
29 May 2006        Modified for WinCE platform						WinCE Team-ST Noida
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */


/* Includes ----------------------------------------------------------------- */
#if !defined(ST_OSLINUX) || defined(STBLIT_LINUX_FULL_USER_VERSION)
#include <stdlib.h>
#endif

#include "stddefs.h"
#include "blt_be.h"
#include "blt_init.h"

#ifdef ST_OSLINUX
#ifdef STBLIT_LINUX_FULL_USER_VERSION
#include "stblit_ioctl.h"
#endif
#else
#include "stsys.h"
#endif

/*#define BLIT_DEBUG 1*/

#include "stevt.h"
#include "blt_mem.h"
#include "blt_pool.h"
#include "blt_com.h"

#if defined(ST_OSLINUX) && !defined(STBLIT_LINUX_FULL_USER_VERSION)
#define KERNEL_THREAD_BLIT_MASTER_NAME              "kblit_master"
#define KERNEL_THREAD_BLIT_SLAVE_NAME               "kblit_slave"
#define KERNEL_THREAD_BLIT_INTERRUPT_NAME           "kblit_interrupt"
#endif




/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */
#if defined(WA_GNBvd06529) || defined(WA_GNBvd06530) || defined(WA_FILL_OPERATION) || defined(WA_SOFT_RESET_EACH_NODE) || defined (WA_GNBvd15279)
#define RESET_ENGINE
#endif

#ifdef ST_OSLINUX
#ifdef MODULE

#define PrintTrace(x)       STTBX_Print(x)

#else   /* MODULE */

#define PrintTrace(x)		printf x

#endif  /* MODULE */
#endif

/* Private Variables (static)------------------------------------------------ */

/* Global Variables --------------------------------------------------------- */

/* Private Macros ----------------------------------------------------------- */

/* Private Function prototypes ---------------------------------------------- */
__inline static void StartEngine(stblit_Device_t* Device_p);

#if !defined(STBLIT_LINUX_FULL_USER_VERSION)
#ifdef RESET_ENGINE
__inline static void ResetEngine(stblit_Device_t* Device_p);
#endif
#endif /* STBLIT_LINUX_FULL_USER_VERSION */

static void SetLock(stblit_Device_t* Device_p);
static void SetUnlock(stblit_Device_t* Device_p);
static void SubmitNode(stblit_Device_t* Device_p, stblit_Msg_t*  Msg_p, BOOL InsertAtFront, BOOL LockBeforeFirstNode);

#ifdef STBLIT_LINUX_FULL_USER_VERSION
extern int fileno(FILE*);
#endif /* STBLIT_LINUX_FULL_USER_VERSION */

/* Functions ---------------------------------------------------------------- */

/*******************************************************************************
Name        : StartEngine
Description : Start HW blitter to process the node
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
__inline static void StartEngine(stblit_Device_t* Device_p)
{
#ifdef STBLIT_EMULATOR
    if (Device_p->DeviceType == STBLIT_DEVICE_TYPE_SOFTWARE_EMULATION)
    {
        STOS_SemaphoreSignal(Device_p->EmulatorStartSemaphore);
    }
    else  /* GAMMA devices (7015, 7020, GX1, 5528, 7710, 7100) */
    {
        STSYS_WriteRegDev32LE((void*)((U32)Device_p->BaseAddress_p + BLT_CTL_OFFSET), BLT_CTL_START);
        STSYS_WriteRegDev32LE((void*)((U32)Device_p->BaseAddress_p + BLT_CTL_OFFSET), 0);
    }
#else   /* GAMMA devices (7015, 7020, GX1, 5528, 7710, 7100) */
#ifdef ST_OSLINUX
/*     PrintTrace(("STBLIT: StartEngine...... \n" ));*/
#endif
    STSYS_WriteRegDev32LE((void*)((U32)Device_p->BaseAddress_p + BLT_CTL_OFFSET), BLT_CTL_START);
    STSYS_WriteRegDev32LE((void*)((U32)Device_p->BaseAddress_p + BLT_CTL_OFFSET), 0);

#endif

}


#if !defined(STBLIT_LINUX_FULL_USER_VERSION)
#ifdef RESET_ENGINE
/*******************************************************************************
Name        : ResetEngine
Description : Reset HW blitter to process the node
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
__inline static void ResetEngine(stblit_Device_t* Device_p)
{
    if ((Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7015) ||
        (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7020) ||
        (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_5528) ||
        (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7710) ||
        (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7100) ||
        (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_ST40GX1))
    {
        STSYS_WriteRegDev32LE((void*)((U32)Device_p->BaseAddress_p + BLT_CTL_OFFSET), BLT_CTL_RESET);
        STSYS_WriteRegDev32LE((void*)((U32)Device_p->BaseAddress_p + BLT_CTL_OFFSET), 0);
    }
}
#endif
#endif                                  /* STBLIT_LINUX_FULL_USER_VERSION */

/*******************************************************************************
Name        : stblit_SlaveProcess
Description : Back-end Low priority queue process
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
void stblit_SlaveProcess (void* data_p)
{
    stblit_Device_t* Device_p;
    stblit_Msg_t* Msg_p;

    STOS_TaskEnter(NULL);

    Device_p = (stblit_Device_t*) data_p;

    while (Device_p->TaskTerminate == FALSE)
    {
        while (TRUE)
        {
            Msg_p = (stblit_Msg_t*) STOS_MessageQueueReceive(Device_p->LowPriorityQueue);

            if (Msg_p->MessageID == TERMINATE)
            {
                /* Tells the slave is terminated */
                STOS_SemaphoreSignal(Device_p->SlaveTerminatedSemaphore);

                STOS_MessageQueueRelease(Device_p->LowPriorityQueue, Msg_p);
                break;
            }
            else if (Msg_p->MessageID == SUBMIT)
            {
                    STOS_SemaphoreWait(Device_p->BackEndQueueControl);

					/* Take access control */
                    STOS_SemaphoreWait(Device_p->AccessControl);

					/* Process Submit */
					SubmitNode(Device_p, Msg_p,FALSE, FALSE);  /* Note that Access control is signaled within the SubmitNode function */
                    STOS_SemaphoreSignal(Device_p->BackEndQueueControl);
            }
            else if (Msg_p->MessageID == SUBMIT_LOCK)
            {
                    STOS_SemaphoreWait(Device_p->BackEndQueueControl);

		            /* Take access control */
                    STOS_SemaphoreWait(Device_p->AccessControl);

		            /* Process Submit */
			        SubmitNode(Device_p, Msg_p,FALSE, TRUE);  /* Note that Access control is signaled within the SubmitNode function */
                    STOS_SemaphoreSignal(Device_p->BackEndQueueControl);
            }
            else if (Msg_p->MessageID == LOCK)
            {
                    STOS_SemaphoreWait(Device_p->BackEndQueueControl);

		            /* Process Lock */
			        SetLock(Device_p);

                    STOS_SemaphoreSignal(Device_p->BackEndQueueControl);
            }
            else if (Msg_p->MessageID == UNLOCK)
            {
                    STOS_SemaphoreWait(Device_p->BackEndQueueControl);

		            /* Process Unlock */
			        SetUnlock(Device_p);

                    STOS_SemaphoreSignal(Device_p->BackEndQueueControl);
            }

            STOS_MessageQueueRelease(Device_p->LowPriorityQueue, Msg_p);
        }
    }

}



/*******************************************************************************
Name        : stblit_waitallblitscompleted()
Description : Wait until the blitter enter in the idle state (used Sync chip
                    for DirectFB )
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
void stblit_waitallblitscompleted( stblit_Device_t* Device_p )
{
    /* Waiting start only if there is pending node */
    if ( Device_p->SBlitNodeDataPool.NbFreeElem < (Device_p->SBlitNodeDataPool.NbElem - 1) )
    {
        Device_p->StartWaitBlitComplete = TRUE ;
        /* Code for sync chip : Sleep until waiting node is 0 */
        STOS_SemaphoreWait(Device_p->AllBlitsCompleted_p);
    }
}


/*******************************************************************************
Name        : stblit_MasterProcess
Description : Back-end High priority queue process
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
void stblit_MasterProcess (void* data_p)
{
    stblit_Device_t*    Device_p;
    stblit_Msg_t*       Msg_p;

    STOS_TaskEnter(NULL);

    Device_p = (stblit_Device_t*) data_p;

    while (Device_p->TaskTerminate == FALSE)
    {
        while (TRUE)
        {
            Msg_p = (stblit_Msg_t*) STOS_MessageQueueReceiveTimeout(Device_p->HighPriorityQueue, TIMEOUT_IMMEDIATE);

            if (Msg_p == NULL)      /* Empty queue */
            {
                STOS_SemaphoreSignal(Device_p->BackEndQueueControl);  /* Give Control to slave because queue empty*/
                /* Wait for queue to receive a message and take ownership back*/
                Msg_p = (stblit_Msg_t*) STOS_MessageQueueReceiveTimeout(Device_p->HighPriorityQueue, TIMEOUT_INFINITY);
                STOS_SemaphoreWait(Device_p->BackEndQueueControl);   /* Take control */
            }

            /* There is a message to process and Master has got the ownership at that point */
            if (Msg_p->MessageID == TERMINATE)
            {
                #if defined(ST_OSLINUX) && !defined(STBLIT_LINUX_FULL_USER_VERSION)
                    STOS_SemaphoreSignal(Device_p->MasterTerminatedSemaphore);
                #endif
                STOS_MessageQueueRelease(Device_p->HighPriorityQueue, Msg_p);

                break;
            }
            else if (Msg_p->MessageID == SUBMIT)
            {
                /* Take access control */
                STOS_SemaphoreWait(Device_p->AccessControl);

                if (Device_p->HWUnLocked == FALSE)
                {
                    /* Release access control */
                    STOS_SemaphoreSignal(Device_p->AccessControl);
                    STOS_SemaphoreSignal(Device_p->BackEndQueueControl);  /* Give Control to slave because HW locked */
                    STOS_SemaphoreWait(Device_p->HWUnlockedSemaphore);    /* wait for HW to be unlocked */
                    STOS_SemaphoreWait(Device_p->BackEndQueueControl);    /* Take control */

                    /* Take access control */
                    STOS_SemaphoreWait(Device_p->AccessControl);
                }

                SubmitNode(Device_p, Msg_p,TRUE, FALSE);    /* Note that Access control is signaled within the SubmitNode function */
                STOS_MessageQueueRelease(Device_p->HighPriorityQueue, Msg_p);
            }
            else if (Msg_p->MessageID == SUBMIT_LOCK)
            {
                /* Take access control */
                STOS_SemaphoreWait(Device_p->AccessControl);

                if (Device_p->HWUnLocked == FALSE)
                {
                    /* Release access control */
                    STOS_SemaphoreSignal(Device_p->AccessControl);

                    STOS_SemaphoreSignal(Device_p->BackEndQueueControl);  /* Give Control to slave because HW locked */
                    STOS_SemaphoreWait(Device_p->HWUnlockedSemaphore);    /* wait for HW to be unlocked */
                    STOS_SemaphoreWait(Device_p->BackEndQueueControl);    /* Take control */

                    /* Take access control */
                    STOS_SemaphoreWait(Device_p->AccessControl);
                }
                SubmitNode(Device_p, Msg_p,TRUE, TRUE);    /* Note that Access control is signaled within the SubmitNode function */

                STOS_MessageQueueRelease(Device_p->HighPriorityQueue, Msg_p);
            }
        }
    }
}

/*******************************************************************************
Name        : SubmitNode
Description : Add Node list at the beginning of the Blitter list
Parameters  :
Assumptions : Access Control semaphore has been already waited before entering the function!!!
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SubmitNode(stblit_Device_t* Device_p, stblit_Msg_t*  Msg_p, BOOL InsertAtFront, BOOL LockBeforeFirstNode)
{
    U32 INS;
#if defined(ST_OSLINUX) && defined(BLIT_DEBUG)
    stblit_Node_t*          Node_p;
#endif

    if (Device_p->BlitterIdle == TRUE)   /* The blitter is ready (status3) and there isn't any pending interruption.
                                          * Node insertion has to be done manually here */
    {

#if defined(ST_OSLINUX) && defined(BLIT_DEBUG)
#ifdef STBLIT_LINUX_FULL_USER_VERSION
        if ( (U32) Msg_p->Data1_p > (U32) Device_p->JobBlitNodeBuffer_p )
        {
            PrintTrace(("address node job=0x%x***New node job address=0x%x\n", (U32)Msg_p->Data1_p,
                    (U32)stblit_CpuToDevice((void*)Device_p->JobBlitNodeBuffer_p)+
                    (U32) Msg_p->Data1_p - (U32) Device_p->JobBlitNodeBuffer_p ));
        }
        else
        {
            PrintTrace(("address node=0x%x, new address=0x%x\n", (U32)Msg_p->Data1_p, (U32)stblit_CpuToDevice((void*)Msg_p->Data1_p )));
        }
#else
        PrintTrace(("address node=0x%x, new address=0x%x\n", (U32)Msg_p->Data1_p, (U32)stblit_CpuToDevice((void*)Msg_p->Data1_p, &(Device_p->VirtualMapping))  ));
#endif
#endif

#ifdef STBLIT_LINUX_FULL_USER_VERSION
        if ( (U32) Msg_p->Data1_p > (U32) Device_p->JobBlitNodeBuffer_p )
        {
            STSYS_WriteRegDev32LE((void*)((U32)Device_p->BaseAddress_p + BLT_NIP_OFFSET),
                                        (U32)PhysicJobBlitNodeBuffer_p +
                                        (U32) Msg_p->Data1_p - (U32) Device_p->JobBlitNodeBuffer_p  );
        }
        else
        {
            /* First node to insert*/
            STSYS_WriteRegDev32LE((void*)((U32)Device_p->BaseAddress_p + BLT_NIP_OFFSET),
                                        (U32)stblit_CpuToDevice((void*)Msg_p->Data1_p));
        }
#else
            /* First node to insert*/
            STSYS_WriteRegDev32LE((void*)((U32)Device_p->BaseAddress_p + BLT_NIP_OFFSET),
                                        (U32)stblit_CpuToDevice((void*)Msg_p->Data1_p, &(Device_p->VirtualMapping)));
#endif

        /* Update First Node desciptor */
        ((stblit_Node_t*) (Msg_p->Data1_p))->PreviousNode = (stblit_NodeHandle_t)STBLIT_NO_NODE_HANDLE;

        /* Last node to insert */
        STSYS_WriteRegMem32LE((void*)(&(((stblit_Node_t*)(Msg_p->Data2_p))->HWNode.BLT_NIP)),0);

        /* Update LastNodeInBlitList */
        Device_p->LastNodeInBlitList_p =  (stblit_Node_t*) Msg_p->Data2_p;

        /* Set Interrupt for last node in HW list (used for the submission process ).(No ITOpcode for submission process IT)*/
        INS = (U32)STSYS_ReadRegMem32LE((void*)(&(Device_p->LastNodeInBlitList_p->HWNode.BLT_INS)));
        INS |= ((U32)(STBLIT_INS_ENABLE_IRQ_COMPLETED & STBLIT_INS_ENABLE_IRQ_MASK) << STBLIT_INS_ENABLE_IRQ_SHIFT);

        STSYS_WriteRegMem32LE((void*)(&(Device_p->LastNodeInBlitList_p->HWNode.BLT_INS)),INS);

        /* Lock management : Case of job node insertion which contains a lock before its first node.
         * We directly change the lock status */
        if (LockBeforeFirstNode == TRUE)
        {
            /* Update Device */
            Device_p->HWUnLocked = FALSE;
        }
        Device_p->BlitterIdle = FALSE;

#if defined(ST_OSLINUX) && defined(BLIT_DEBUG)
    Node_p = (stblit_Node_t*)Msg_p->Data1_p ;
    PrintTrace(( "---------- Node = 0x%x---\n", Node_p ));

    PrintTrace(( "---------- BLT_NIP = 0x%x-----------------\n", Node_p->HWNode.BLT_NIP ));
    PrintTrace(( "---------- BLT_USR = 0x%x-----------------\n", Node_p->HWNode.BLT_USR ));
    PrintTrace(( "---------- BLT_INS = 0x%x-----------------\n", Node_p->HWNode.BLT_INS));
    PrintTrace(( "---------- BLT_S1BA = 0x%x-----------------\n", Node_p->HWNode.BLT_S1BA ));

    PrintTrace(( "---------- BLT_S1TY = 0x%x-----------------\n", Node_p->HWNode.BLT_S1TY ));
    PrintTrace(( "---------- BLT_S1XY = 0x%x-----------------\n", Node_p->HWNode.BLT_S1XY ));
    PrintTrace(( "---------- BLT_S1SZ = 0x%x-----------------\n", Node_p->HWNode.BLT_S1SZ ));
    PrintTrace(( "---------- BLT_S1CF = 0x%x-----------------\n", Node_p->HWNode.BLT_S1CF ));

    PrintTrace(( "---------- BLT_S2BA = 0x%x-----------------\n", Node_p->HWNode.BLT_S2BA ));
    PrintTrace(( "---------- BLT_S2TY = 0x%x-----------------\n", Node_p->HWNode.BLT_S2TY ));
    PrintTrace(( "---------- BLT_S2XY = 0x%x-----------------\n", Node_p->HWNode.BLT_S2XY ));
    PrintTrace(( "---------- BLT_S2SZ = 0x%x-----------------\n", Node_p->HWNode.BLT_S2SZ ));

    PrintTrace(( "---------- BLT_S2CF = 0x%x-----------------\n", Node_p->HWNode.BLT_S2CF ));
    PrintTrace(( "---------- BLT_TBA = 0x%x-----------------\n", Node_p->HWNode.BLT_TBA ));
    PrintTrace(( "---------- BLT_TTY = 0x%x-----------------\n", Node_p->HWNode.BLT_TTY ));
    PrintTrace(( "---------- BLT_TXY = 0x%x-----------------\n", Node_p->HWNode.BLT_TXY ));

    PrintTrace(( "---------- BLT_CWO = 0x%x-----------------\n", Node_p->HWNode.BLT_CWO ));
    PrintTrace(( "---------- BLT_CWS = 0x%x-----------------\n", Node_p->HWNode.BLT_CWS ));
    PrintTrace(( "---------- BLT_CCO = 0x%x-----------------\n", Node_p->HWNode.BLT_CCO ));
    PrintTrace(( "---------- BLT_CML = 0x%x-----------------\n", Node_p->HWNode.BLT_CML ));

    PrintTrace(( "---------- BLT_RZC = 0x%x-----------------\n", Node_p->HWNode.BLT_RZC ));
    PrintTrace(( "---------- BLT_HFP = 0x%x-----------------\n", Node_p->HWNode.BLT_HFP ));
    PrintTrace(( "---------- BLT_VFP = 0x%x-----------------\n", Node_p->HWNode.BLT_VFP ));
    PrintTrace(( "---------- BLT_RSF = 0x%x-----------------\n", Node_p->HWNode.BLT_RSF ));

    PrintTrace(( "---------- BLT_FF0 = 0x%x-----------------\n", Node_p->HWNode.BLT_FF0 ));
    PrintTrace(( "---------- BLT_FF1 = 0x%x-----------------\n", Node_p->HWNode.BLT_FF1 ));
    PrintTrace(( "---------- BLT_FF2 = 0x%x-----------------\n", Node_p->HWNode.BLT_FF2 ));
    PrintTrace(( "---------- BLT_FF3 = 0x%x-----------------\n", Node_p->HWNode.BLT_FF3 ));

    PrintTrace(( "---------- BLT_ACK = 0x%x-----------------\n", Node_p->HWNode.BLT_ACK ));
    PrintTrace(( "---------- BLT_KEY1 = 0x%x-----------------\n", Node_p->HWNode.BLT_KEY1 ));
    PrintTrace(( "---------- BLT_KEY2 = 0x%x-----------------\n", Node_p->HWNode.BLT_KEY2 ));
    PrintTrace(( "---------- BLT_PMK = 0x%x-----------------\n", Node_p->HWNode.BLT_PMK ));

    PrintTrace(( "---------- BLT_RST = 0x%x-----------------\n", Node_p->HWNode.BLT_RST ));
    PrintTrace(( "---------- BLT_XYL = 0x%x-----------------\n", Node_p->HWNode.BLT_XYL ));
    PrintTrace(( "---------- BLT_XYP = 0x%x-----------------\n", Node_p->HWNode.BLT_XYP ));
#endif

        /* Start Engine */
        StartEngine(Device_p);

        /* Release accesss control */
        STOS_SemaphoreSignal(Device_p->AccessControl);
    }
    else  /* The blitter is busy or ready (status3) but there still is at least one pending interruption */
    {
        /* Update Device_p structure */
        Device_p->FirstNode_p           = (stblit_Node_t*) Msg_p->Data1_p;
        Device_p->LastNode_p            = (stblit_Node_t*) Msg_p->Data2_p;
        Device_p->InsertAtFront         = InsertAtFront;
        Device_p->InsertionRequest      = TRUE;
        Device_p->LockBeforeFirstNode   = LockBeforeFirstNode;

#ifdef STBLIT_EMULATOR
        if (Device_p->DeviceType == STBLIT_DEVICE_TYPE_SOFTWARE_EMULATION)
        {
                STOS_SemaphoreSignal(Device_p->EmulatorSuspendSemaphore);
        }
        else  /* GAMMA devices (7015, 7020, GX1) */
        {
            /* suspend the HW (no effect if already stopped) */
            STSYS_WriteRegDev32LE((void*)((U32)Device_p->BaseAddress_p + BLT_CTL_OFFSET), BLT_CTL_SUSPEND);
        }
#else   /* GAMMA devices (7015, 7020, GX1) */
        STSYS_WriteRegDev32LE((void*)((U32)Device_p->BaseAddress_p + BLT_CTL_OFFSET), BLT_CTL_SUSPEND);
#endif
        /* Release accesss control */
        STOS_SemaphoreSignal(Device_p->AccessControl);

        /* wait for next interrupt to be raised and executed to make sure the node insertion has been done */
        STOS_SemaphoreWait(Device_p->InsertionDoneSemaphore);
    }
}

/*******************************************************************************
Name        : SetLock
Description : Add Lock
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetLock(stblit_Device_t* Device_p)
{

    /* Take access control */
    STOS_SemaphoreWait(Device_p->AccessControl);

    if (Device_p->BlitterIdle == TRUE)   /* The blitter is ready (status3) and there isn't any pending interruption.
                                          * Lock set manually here */
    {
        /* Update Device */
        Device_p->HWUnLocked = FALSE;

        /* Release accesss control */
        STOS_SemaphoreSignal(Device_p->AccessControl);
    }
    else  /* The blitter is busy or ready (status3) but there still is at least one pending interruption */
    {
        /* Update Device_p structure */
        Device_p->LockRequest = TRUE;



#ifdef STBLIT_EMULATOR
        if (Device_p->DeviceType == STBLIT_DEVICE_TYPE_SOFTWARE_EMULATION)
        {
            STOS_SemaphoreSignal(Device_p->EmulatorSuspendSemaphore);
        }
        else    /* GAMMA devices (7015, 7020, GX1) */
        {
            /* suspend the HW (no effect if already stopped)*/
            STSYS_WriteRegDev32LE((void*)((U32)Device_p->BaseAddress_p + BLT_CTL_OFFSET), BLT_CTL_SUSPEND);
        }
#else    /* GAMMA devices (7015, 7020, GX1) */
        STSYS_WriteRegDev32LE((void*)((U32)Device_p->BaseAddress_p + BLT_CTL_OFFSET), BLT_CTL_SUSPEND);
#endif

        /* Release accesss control */
        STOS_SemaphoreSignal(Device_p->AccessControl);
		/* wait for next interrupt to be raised and executed to make sure the Lock installation has been done */
        STOS_SemaphoreWait(Device_p->LockInstalledSemaphore);
    }
}

/*******************************************************************************
Name        : SetUnlock
Description : Add Unlock
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetUnlock(stblit_Device_t* Device_p)
{
    /* Take access control */
    STOS_SemaphoreWait(Device_p->AccessControl);

    if (Device_p->BlitterIdle == TRUE)   /* The blitter is ready (status3) and there isn't any pending interruption.
                                          * Unlock set manually here */
    {
        /* Update Device */
        Device_p->HWUnLocked = TRUE;

        STOS_SemaphoreSignal(Device_p->HWUnlockedSemaphore);

        /* Release accesss control */
        STOS_SemaphoreSignal(Device_p->AccessControl);
    }
    else  /* The blitter is busy or ready (status3) but there still is at least one pending interruption */
    {
        /* Update Device_p structure */
        Device_p->UnlockRequest = TRUE;


#ifdef STBLIT_EMULATOR
        if (Device_p->DeviceType == STBLIT_DEVICE_TYPE_SOFTWARE_EMULATION)
        {
            STOS_SemaphoreSignal(Device_p->EmulatorSuspendSemaphore);
        }
        else  /* GAMMA devices (7015, 7020, GX1) */
        {
            /* suspend the HW (no effect if already stopped)*/
            STSYS_WriteRegDev32LE((void*)((U32)Device_p->BaseAddress_p + BLT_CTL_OFFSET), BLT_CTL_SUSPEND);
        }
#else  /* GAMMA devices (7015, 7020, GX1) */
        STSYS_WriteRegDev32LE((void*)((U32)Device_p->BaseAddress_p + BLT_CTL_OFFSET), BLT_CTL_SUSPEND);
#endif
		/* Release accesss control */
        STOS_SemaphoreSignal(Device_p->AccessControl);
		/* wait for next interrupt to be raised and executed to make sure the Unlock installation has been done */
        STOS_SemaphoreWait(Device_p->UnlockInstalledSemaphore);
    }
}



/*******************************************************************************
Name        : stblit_BlitterInterruptHandler
Description : Interrupt routine callback (part of the main ISR)
Parameters  :
Assumptions : Its thread is the main ISR (Interruot Service routine).
              Device_p is passed through params.
              Must be as short as possible!
Limitations :
Returns     : Nothing
*******************************************************************************/
#if !defined(STBLIT_LINUX_FULL_USER_VERSION)
#if defined(ST_OSLINUX)
irqreturn_t stblit_BlitterInterruptHandler(int irq, void * Param_p, struct pt_regs * regs)
#endif  /* ST_OSLINUX */
#if defined(ST_OS21) || defined(ST_OSWINCE)
int stblit_BlitterInterruptHandler (void* Param_p)
#endif  /* ST_OS21 */
#ifdef ST_OS20
void stblit_BlitterInterruptHandler (void* Param_p)
#endif /* ST_OS20 */
{
    stblit_Device_t*    Device_p;
    U32                 Status3;


    Device_p = (stblit_Device_t*)Param_p;

    /* Read Status register 3 to get the activity of the blitter */
    Status3 = (U32)STSYS_ReadRegDev32LE((void*)((U32)Device_p->BaseAddress_p + BLT_STA3_OFFSET)) & BLT_STA3_MASK;

    /* Read Status register 1 to get the node address */
    Device_p->Status1 = (U32)stblit_DeviceToCpu((void*)((U32)STSYS_ReadRegDev32LE((void*)((U32)Device_p->BaseAddress_p + BLT_STA1_OFFSET))
                                                                              & BLT_STA1_MASK), &(Device_p->VirtualMapping));
    /* Reset Pending Interrupt */
    STSYS_WriteRegDev32LE((void*)((U32)Device_p->BaseAddress_p + BLT_STA3_OFFSET), 0xE); /* TBD */

#if defined(BLIT_DEBUG)
      PrintTrace(("BlitterInterruptHandler:Status3=0x%x,   Status1=0x%x\n", Device_p->Status3, Device_p->Status1 ));
#endif


#ifdef RESET_ENGINE
    /* Reset Engine */
    ResetEngine(Device_p);
#endif


#ifdef ST_OSLINUX
   if ( !(Status3 & 1) )
   {
     Device_p->Status3 = Status3;
   }
   else
   {
     return IRQ_HANDLED;
   }
#else
     Device_p->Status3 = Status3;
#endif

    /* Start stblit_BlitterInterrupt Process */
    STOS_SemaphoreSignal(Device_p->InterruptProcessSemaphore);

#if defined(ST_OS21) || defined(ST_OSWINCE)
    return(OS21_SUCCESS);
#elif defined(ST_OSLINUX)
    return IRQ_HANDLED;
#endif

}

#endif /*STBLIT_LINUX_FULL_USER_VERSION*/

#ifdef STBLIT_LINUX_FULL_USER_VERSION

void stblit_InterruptHandlerTask(const void* SubscriberData_p)
{
    STBLITMod_Status_t    STBLITMod_Status;
	U32 TempAddress = (U32)SubscriberData_p;
    stblit_Device_t     * Device_p = (stblit_Device_t*)TempAddress;

		/* To remove comipation warnning */
	UNUSED_PARAMETER(SubscriberData_p);

	while (Device_p->TaskTerminate == FALSE)
    {

		STOS_SemaphoreWait( Device_p->BlitInterruptHandlerSemaphore );

        ioctl(fileno(Device_p->BLIT_DevFile), STBLIT_IOCTL_GET_STATUSES, &STBLITMod_Status);

        /* Read Status register 3 to get the activity of the blitter */
        Device_p->Status3 = STBLITMod_Status.Status3 & BLT_STA3_MASK;

        /* Read Status register 1 to get the node address */
        if ( PhysicJobBlitNodeBuffer_p )
        {
            Device_p->Status1 = (U32) STBLITMod_Status.Status1 - (U32) PhysicJobBlitNodeBuffer_p + (U32) Device_p->JobBlitNodeBuffer_p ;
        }
        else
        {
            if ( (U32) STBLITMod_Status.Status1 > (U32) PhysicSingleBlitNodeBuffer_p )
            {
                Device_p->Status1 = (U32) STBLITMod_Status.Status1 - (U32) PhysicSingleBlitNodeBuffer_p + (U32) Device_p->SingleBlitNodeBuffer_p ;
            }
            else
            {
                Device_p->Status1 = (U32)stblit_DeviceToCpu((void*)(STBLITMod_Status.Status1 & BLT_STA1_MASK));
            }
        }

#if defined(BLIT_DEBUG)
        PrintTrace(("stblit_InterruptHandlerTask():Status3=0x%x,   Status1=0x%x\n", Device_p->Status3, Device_p->Status1 ));
#endif
        STOS_SemaphoreSignal(Device_p->InterruptProcessSemaphore);
	}
}
#endif


/*******************************************************************************
Name        : stblit_BlitterInterruptEventHandler
Description : Interrupt routine callback (part of the main ISR) raised by INTMR module
Parameters  :
Assumptions : Its thread is the main ISR (Interruot Service routine).
              Device_p is passed through SubscriberData.
              Must be as short as possible!
Limitations :
Returns     : Nothing
*******************************************************************************/
#ifndef STBLIT_LINUX_FULL_USER_VERSION

#if defined(ST_OSLINUX)
void stblit_BlitterInterruptEventHandler (STEVT_CallReason_t     Reason,
                                     const ST_DeviceName_t  RegistrantName,
                                     STEVT_EventConstant_t  Event,
                                     const void*            EventData_p,
                                     const void*            SubscriberData_p)
#endif
#if defined(ST_OS21) || defined(ST_OSWINCE)
int stblit_BlitterInterruptEventHandler (STEVT_CallReason_t     Reason,
                                     const ST_DeviceName_t  RegistrantName,
                                     STEVT_EventConstant_t  Event,
                                     const void*            EventData_p,
                                     const void*            SubscriberData_p)
#endif
#ifdef ST_OS20
void stblit_BlitterInterruptEventHandler (STEVT_CallReason_t     Reason,
                                     const ST_DeviceName_t  RegistrantName,
                                     STEVT_EventConstant_t  Event,
                                     const void*            EventData_p,
                                     const void*            SubscriberData_p)
#endif
{
#if defined(ST_OSLINUX)
    stblit_BlitterInterruptHandler(0, (void *)EventData_p, NULL);
#else
	U32 Address = (U32)SubscriberData_p;

	stblit_BlitterInterruptHandler((void *)Address);
#endif

	/* To remove comipation warnning */
	UNUSED_PARAMETER(Reason);
    UNUSED_PARAMETER(RegistrantName);
	UNUSED_PARAMETER(Event);
	UNUSED_PARAMETER(EventData_p);


#if defined(ST_OS21) || defined(ST_OSWINCE)
    return(OS21_SUCCESS);
#endif /* ST_OS21 */
}
#endif                                  /* STBLIT_LINUX_FULL_USER_VERSION */

/*******************************************************************************
Name        : stblit_InterruptProcess
Description : Process started by ISR :
                + Insert Node, Lock install or Unlock install
                + Remove already processed Nodes
                + Notify Event(s)
                + Restart Blitter engine if needed
Parameters  :
Assumptions :  Lock Install , Unlock Install and Node insertion  can not be done
               simultaneously
Limitations :
Returns     : Nothing
*******************************************************************************/
void stblit_InterruptProcess (void* data_p)
{

    stblit_Device_t*    Device_p;
    stblit_Node_t*      Node_p;     /* Current Node after suspension point */
    stblit_Node_t*      NextNode_p; /* Next Node after suspension point */
    U32                 ITOpcode;   /* Action of interrution */
    U32                 SubscriberID;
    STBLIT_JobHandle_t  JobHandle;
    U32                 UserTag;
    BOOL                ImmediateHWLock;
    BOOL                ImmediateHWUnlock;
    BOOL                SignalHWUnlockedSemaphore;
    BOOL                SignalInsertionDoneSemaphore;
    BOOL                SignalUnlockInstalledSemaphore;
    BOOL                SignalLockInstalledSemaphore;
    BOOL                RestartEngine;
    BOOL                NotifyBlitCompletion;
    BOOL                NotifyJobCompletion;
    BOOL                StopOnLastNode;
    U32                 Tmp;
    U32                 INS;
#if defined(WA_GNBvd06529) || defined (WA_GNBvd06530) || defined (WA_FILL_OPERATION) || defined (WA_SOFT_RESET_EACH_NODE) || defined (WA_GNBvd15279)
    BOOL                NIPisUpdated;
#endif

	/* To remove comipation warnning */
	UNUSED_PARAMETER(Device_p);
#ifdef STBLIT_LINUX_FULL_USER_VERSION
    UNUSED_PARAMETER(SubscriberID);
#endif


    STOS_TaskEnter(NULL);

    /* Get Device_t info */
    Device_p = (stblit_Device_t*) data_p;

    while(TRUE)
    {
        STOS_SemaphoreWait(Device_p->InterruptProcessSemaphore);

        if (Device_p->TaskTerminate == TRUE)
        {
            break;
        }

        /* Init local variables */
        ImmediateHWLock                 = FALSE;
        ImmediateHWUnlock               = FALSE;
        SignalHWUnlockedSemaphore       = FALSE;
        SignalInsertionDoneSemaphore    = FALSE;
        SignalUnlockInstalledSemaphore  = FALSE;
        SignalLockInstalledSemaphore    = FALSE;
        RestartEngine                   = FALSE;
        NotifyBlitCompletion            = FALSE;
        NotifyJobCompletion             = FALSE;
#if defined(WA_GNBvd06529) || defined (WA_GNBvd06530) || defined (WA_FILL_OPERATION) || defined (WA_SOFT_RESET_EACH_NODE) || defined (WA_GNBvd15279)
        NIPisUpdated                    = FALSE;
#endif

#ifdef ST_OSLINUX
#if defined(BLIT_DEBUG)
        PrintTrace(("Interrupt process\n"));
#endif
#endif
        /* ################################ START OF PROTECTED AREA #####################################" */

        /* Take access control */
        STOS_SemaphoreWait(Device_p->AccessControl);

        /* Extract Node and its Opcode */
        Node_p = (stblit_Node_t*) Device_p->Status1;

        /* Extract the address of the next node after the suspension point */
/*        NextNode_p = (stblit_Node_t*)stblit_DeviceToCpu((void*)Node_p->HWNode.BLT_NIP, &(Device_p->VirtualMapping));*/
        Tmp = STSYS_ReadRegMem32LE((void*)(&(Node_p->HWNode.BLT_NIP)));
#ifdef STBLIT_LINUX_FULL_USER_VERSION
        if ( PhysicJobBlitNodeBuffer_p )
        {
            NextNode_p = (stblit_Node_t*)((U32) Tmp - (U32) PhysicJobBlitNodeBuffer_p + (U32) Device_p->JobBlitNodeBuffer_p);

        }
        else
        {
            if ( (U32) Tmp > (U32) PhysicSingleBlitNodeBuffer_p )
            {
                NextNode_p = (stblit_Node_t*)((U32) Tmp - (U32) PhysicSingleBlitNodeBuffer_p + (U32) Device_p->SingleBlitNodeBuffer_p);
            }
            else
            {
                NextNode_p = (stblit_Node_t*)stblit_DeviceToCpu((void*)Tmp);
            }
        }
#else
        NextNode_p = (stblit_Node_t*)stblit_DeviceToCpu((void*)Tmp, &(Device_p->VirtualMapping));
#endif
        if (Tmp == 0 ) /* HW stopped at last node */
        {
            StopOnLastNode = TRUE;
        }
        else
        {
            StopOnLastNode = FALSE;
        }
        /* Record some Node information to make it available after node deletion */
#ifndef STBLIT_LINUX_FULL_USER_VERSION
        SubscriberID = Node_p->SubscriberID;
#endif
        JobHandle    = Node_p->JobHandle;
        UserTag      = STSYS_ReadRegMem32LE((void*)(&(Node_p->HWNode.BLT_USR)));
        ITOpcode     = Node_p->ITOpcode;
        /* Update Default Previous Node of NextNode Descriptor  (The link between Node and NextNode has to be broken
        * because Node is going to be released. Later, when releasing NextNode , it could be a pb if the link is not broken
        * (release twice the Node)
        * This is a default value  and in case of insertion of Node in Front it's going to change*/
        if (StopOnLastNode == FALSE)
        {
            NextNode_p->PreviousNode = (stblit_NodeHandle_t) STBLIT_NO_NODE_HANDLE;
        }
        /* NOTE : Node insertion, Lock and Unlock installations are never performed simultaneously within the same interrupt.
         * They are exclusive! In fact, when the master or slave task is asking for one of this operation, it waits for the operation
         * to be done before releasing the BackEndQueueControl semaphore token for further operations.*/

        /* Node Insertion management */
        if (Device_p->InsertionRequest == TRUE)
        {
            /* Insert Nodes if any */
            if (Device_p->InsertAtFront == TRUE)
            {
                /* First node to insert*/
#ifdef STBLIT_LINUX_FULL_USER_VERSION
        if ( PhysicJobBlitNodeBuffer_p )
        {
            STSYS_WriteRegDev32LE((void*)((U32)Device_p->BaseAddress_p + BLT_NIP_OFFSET),
                                        (U32)PhysicJobBlitNodeBuffer_p +
                                        (U32) Device_p->FirstNode_p - (U32) Device_p->JobBlitNodeBuffer_p  );
        }
        else
        {
            /* First node to insert*/
            STSYS_WriteRegDev32LE((void*)((U32)Device_p->BaseAddress_p + BLT_NIP_OFFSET),
                                        (U32)stblit_CpuToDevice((void*)Device_p->FirstNode_p));
        }
#else
            /* First node to insert*/
            STSYS_WriteRegDev32LE((void*)((U32)Device_p->BaseAddress_p + BLT_NIP_OFFSET),
                                        (U32)stblit_CpuToDevice((void*)Device_p->FirstNode_p, &(Device_p->VirtualMapping)));
#endif
#if defined(WA_GNBvd06529) || defined (WA_GNBvd06530) || defined (WA_FILL_OPERATION) || defined (WA_SOFT_RESET_EACH_NODE) || defined (WA_GNBvd15279)
                NIPisUpdated = TRUE;
#endif

                /* Update First Node desciptor */
                Device_p->FirstNode_p->PreviousNode = (stblit_NodeHandle_t)STBLIT_NO_NODE_HANDLE;

                /* Last node to insert */
                STSYS_WriteRegMem32LE((void*)(&(Device_p->LastNode_p->HWNode.BLT_NIP)),Tmp);


                /* Update the NextNode descriptor */
                if (StopOnLastNode == FALSE)
                {
                    NextNode_p->PreviousNode = (stblit_NodeHandle_t) Device_p->LastNode_p;
                }

                /* In case there is a LOCK ITOpcode in node at insertion point (node_p),
                 * the LOCK opcode must be moved in the last node of the inserted node list */
                if ((ITOpcode & STBLIT_IT_OPCODE_LOCK_MASK) != 0)
                {
                    /* Remove ITOpcode from current node */
                    ITOpcode &= (U32)~STBLIT_IT_OPCODE_LOCK_MASK;

                    /* Move ITOpcode in the last node of inserted nodes list */
                    Device_p->LastNode_p->ITOpcode  |=  STBLIT_IT_OPCODE_LOCK_MASK;
                    INS = (U32)STSYS_ReadRegMem32LE((void*)(&(Device_p->LastNode_p->HWNode.BLT_INS)));
                    INS |= ((U32)(STBLIT_INS_ENABLE_IRQ_COMPLETED & STBLIT_INS_ENABLE_IRQ_MASK) << STBLIT_INS_ENABLE_IRQ_SHIFT);
                    STSYS_WriteRegMem32LE((void*)(&(Device_p->LastNode_p->HWNode.BLT_INS)),INS);
                }

                /* Update LastNodeInBlitList */
                if (Device_p->LastNodeInBlitList_p == Node_p)
                {
                    Device_p->LastNodeInBlitList_p =  Device_p->LastNode_p;

                    /* Set Interrupt for last node in HW list (used for the submission process ).(No ITOpcode for submission process IT)
                     * First implementation, IT always setted even already done above ! */
                    INS = (U32)STSYS_ReadRegMem32LE((void*)(&(Device_p->LastNodeInBlitList_p->HWNode.BLT_INS)));
                    INS |= ((U32)(STBLIT_INS_ENABLE_IRQ_COMPLETED & STBLIT_INS_ENABLE_IRQ_MASK) << STBLIT_INS_ENABLE_IRQ_SHIFT);
                    STSYS_WriteRegMem32LE((void*)(&(Device_p->LastNodeInBlitList_p->HWNode.BLT_INS)),INS);
                }

                /* Lock management : Case of job node insertion which contains a lock before its first node.
                 * We directly change the lock status when front insertion*/
                if (Device_p->LockBeforeFirstNode == TRUE)
                {
                    /* Operation to lock is done later : Wait ITOpcode analysis first */
                    ImmediateHWLock = TRUE;
                }
            }
            else
            {
                /* First node to insert */
                if (Device_p->LastNodeInBlitList_p != Node_p)   /* HW stopped before last node */
                {

                    /* Remove Interruption related to submission process if interruption not needed for some other possible reasons of
                     * this stage :
                     * + Blit completion
                     * + Job completion
                     * + Lock
                     * + unlcok
                     * + S/w workaround for h/w cut limitation */
                    if ((Device_p->LastNodeInBlitList_p->ITOpcode &
                            (
#if defined (WA_FILL_OPERATION) || defined (WA_SOFT_RESET_EACH_NODE) || defined (WA_GNBvd15279)
                             STBLIT_IT_OPCODE_SW_WORKAROUND_MASK |
#endif
                             STBLIT_IT_OPCODE_BLIT_COMPLETION_MASK |
                             STBLIT_IT_OPCODE_JOB_COMPLETION_MASK |
                             STBLIT_IT_OPCODE_LOCK_MASK |
                             STBLIT_IT_OPCODE_UNLOCK_MASK)) == 0)
                    {
                        INS = (U32)STSYS_ReadRegMem32LE((void*)(&(Device_p->LastNodeInBlitList_p->HWNode.BLT_INS)));
                        INS &= ((U32)(~((U32)(STBLIT_INS_ENABLE_IRQ_COMPLETED) << STBLIT_INS_ENABLE_IRQ_SHIFT)));
                        STSYS_WriteRegMem32LE((void*)(&(Device_p->LastNodeInBlitList_p->HWNode.BLT_INS)),INS);
                    }
#ifdef STBLIT_LINUX_FULL_USER_VERSION
                    Tmp =  (U32)stblit_CpuToDevice((void*)Device_p->FirstNode_p );
#else
                    Tmp =  (U32)stblit_CpuToDevice((void*)Device_p->FirstNode_p, &(Device_p->VirtualMapping));
#endif
                    STSYS_WriteRegMem32LE((void*)(&(Device_p->LastNodeInBlitList_p->HWNode.BLT_NIP)),((U32)(Tmp & STBLIT_S2BA_POINTER_MASK)
                                                                                                     << STBLIT_S2BA_POINTER_SHIFT ));


                    /* Update first Node inserted descriptor */
                    Device_p->FirstNode_p->PreviousNode = (stblit_NodeHandle_t) Device_p->LastNodeInBlitList_p;

                    /* Lock management : Case of job node insertion which contains a lock before its first node.
                     * We add lock IT opcode for last node */
                    if (Device_p->LockBeforeFirstNode == TRUE)
                    {
                        /* When lock and Unlock are on the same node => no effect. IT lock and unlock opcodes could be removed.
                         * At first implementation , IT Opcodes are not removed. Anyway there is not any problem because
                         * the ISR mechanism detects that there is nothing to do */

                        /* Operation to lock is done later : Wait ITOpcode analysis first */
                        Device_p->LastNodeInBlitList_p->ITOpcode  |=  STBLIT_IT_OPCODE_LOCK_MASK;
                        INS = (U32)STSYS_ReadRegMem32LE((void*)(&(Device_p->LastNodeInBlitList_p->HWNode.BLT_INS)));
                        INS |= ((U32)(STBLIT_INS_ENABLE_IRQ_COMPLETED & STBLIT_INS_ENABLE_IRQ_MASK) << STBLIT_INS_ENABLE_IRQ_SHIFT);
                        STSYS_WriteRegMem32LE((void*)(&(Device_p->LastNodeInBlitList_p->HWNode.BLT_INS)),INS);
                    }
                }
                else  /* HW stopped at the end of list */
                {
#ifdef STBLIT_LINUX_FULL_USER_VERSION
                        if ( PhysicJobBlitNodeBuffer_p )
                        {
                            STSYS_WriteRegDev32LE((void*)((U32)Device_p->BaseAddress_p + BLT_NIP_OFFSET),
                                                        (U32)PhysicJobBlitNodeBuffer_p +
                                                        (U32) Device_p->FirstNode_p - (U32) Device_p->JobBlitNodeBuffer_p  );
                        }
                        else
                        {
                            STSYS_WriteRegDev32LE((void*)((U32)Device_p->BaseAddress_p + BLT_NIP_OFFSET),
                                                        (U32)stblit_CpuToDevice((void*)Device_p->FirstNode_p));
                        }
#else
                        STSYS_WriteRegDev32LE((void*)((U32)Device_p->BaseAddress_p + BLT_NIP_OFFSET),
                                                    (U32)stblit_CpuToDevice((void*)Device_p->FirstNode_p, &(Device_p->VirtualMapping)));
#endif

#if defined(WA_GNBvd06529) || defined (WA_GNBvd06530) || defined (WA_FILL_OPERATION) || defined (WA_SOFT_RESET_EACH_NODE) || defined (WA_GNBvd15279)
                    NIPisUpdated = TRUE;
#endif
                    /* Update First Node desciptor */
                    Device_p->FirstNode_p->PreviousNode = (stblit_NodeHandle_t)STBLIT_NO_NODE_HANDLE;

                    /* Lock management : Case of job node insertion which contains a lock before its first node.
                     * We directly change the lock status */
                     if (Device_p->LockBeforeFirstNode == TRUE)
                    {
                        /* Operation to lock is done later : Wait ITOpcode analysis first */
                        ImmediateHWLock = TRUE;
                    }
                }

                /* Last node to insert */
                STSYS_WriteRegMem32LE((void*)(&(Device_p->LastNode_p->HWNode.BLT_NIP)),0);

                /* Update LastNodeInBlitList */
                Device_p->LastNodeInBlitList_p = Device_p->LastNode_p;

                /* Set Interrupt for last node in HW list (used for the submission process ) */
                INS = (U32)STSYS_ReadRegMem32LE((void*)(&(Device_p->LastNodeInBlitList_p->HWNode.BLT_INS)));
                INS |= ((U32)(STBLIT_INS_ENABLE_IRQ_COMPLETED & STBLIT_INS_ENABLE_IRQ_MASK) << STBLIT_INS_ENABLE_IRQ_SHIFT);
                STSYS_WriteRegMem32LE((void*)(&(Device_p->LastNodeInBlitList_p->HWNode.BLT_INS)),INS);
            }
        }
        /* Lock Management */
        else if (Device_p->LockRequest == TRUE)
        {
            /* When lock and Unlock are on the same node => no effect. IT lock and unlock opcodes could be removed.
             * At first implementation , IT Opcodes are not removed. Anyway there is not any problem because
             * the ISR mechanism detects that there is nothing to do */


            if (Device_p->LastNodeInBlitList_p != Node_p)   /* HW stopped before last node */
            {
                Device_p->LastNodeInBlitList_p->ITOpcode  |=  STBLIT_IT_OPCODE_LOCK_MASK;
                INS = (U32)STSYS_ReadRegMem32LE((void*)(&(Device_p->LastNodeInBlitList_p->HWNode.BLT_INS)));
                INS |= ((U32)(STBLIT_INS_ENABLE_IRQ_COMPLETED & STBLIT_INS_ENABLE_IRQ_MASK) << STBLIT_INS_ENABLE_IRQ_SHIFT);
                STSYS_WriteRegMem32LE((void*)(&(Device_p->LastNodeInBlitList_p->HWNode.BLT_INS)),INS);
            }
            else  /* HW stopped at the end of list */
            {
                /* Operation to lock is done later : Wait ITOpcode analysis first */
                ImmediateHWLock = TRUE;
            }
        }
        /* Unlock Management */
        else if (Device_p->UnlockRequest == TRUE)
        {
            /* When lock and Unlock are on the same node => no effect. IT lock and unlock opcodes could be removed.
             * At first implementation , IT Opcodes are not removed. Anyway there is not any problem because
             * the ISR mechanism detects that there is nothing to do */

            if (Device_p->LastNodeInBlitList_p != Node_p)   /* HW stopped before last node */
            {
                Device_p->LastNodeInBlitList_p->ITOpcode  |=  STBLIT_IT_OPCODE_UNLOCK_MASK;
                INS = (U32)STSYS_ReadRegMem32LE((void*)(&(Device_p->LastNodeInBlitList_p->HWNode.BLT_INS)));
                INS |= ((U32)(STBLIT_INS_ENABLE_IRQ_COMPLETED & STBLIT_INS_ENABLE_IRQ_MASK) << STBLIT_INS_ENABLE_IRQ_SHIFT);
                STSYS_WriteRegMem32LE((void*)(&(Device_p->LastNodeInBlitList_p->HWNode.BLT_INS)),INS);
            }
            else  /* HW stopped at the end of list */
            {
                /* Operation to unlock is done later : Wait ITOpcode analysis first */
                ImmediateHWUnlock = TRUE;
            }
        }

        /* Whatever the interruption the driver releases all nodes (Single blit + Job) which have been already processed.
         * Note that Node_p data not available anymore after this point !!!!*/
        stblit_ReleaseListNode(Device_p, (stblit_NodeHandle_t)Node_p);

        if ((Device_p->Status3 & BLT_STA3_COMPLETED) != 0)  /* Blit completed */
        {
            if ((ITOpcode & STBLIT_IT_OPCODE_BLIT_COMPLETION_MASK) != 0)
            {
                /* Tell to notify event */
                NotifyBlitCompletion = TRUE;
            }
            if ((ITOpcode & STBLIT_IT_OPCODE_JOB_COMPLETION_MASK) != 0)
            {
                /* Tell to notify event */
                NotifyJobCompletion = TRUE;
            }
            if ((ITOpcode & STBLIT_IT_OPCODE_LOCK_MASK) != 0)
            {
                ImmediateHWLock = TRUE;
            }
            if ((ITOpcode & STBLIT_IT_OPCODE_UNLOCK_MASK) != 0)
            {
                ImmediateHWUnlock = TRUE;
            }
        }

        /* Lock/Unlock immediate action
         * No effect if ImmediateHWLock and ImmediateHWUnlock simultaneously TRUE (whatever the order) :
               1) IT Lock + unlock insert.
               2) IT Unlock + lock insert. */
        if ((ImmediateHWLock == TRUE) && (ImmediateHWUnlock == FALSE))
        {
            /* Update lock/unlcck status */
            Device_p->HWUnLocked = FALSE;
        }
        else if ((ImmediateHWLock == FALSE) && (ImmediateHWUnlock == TRUE))
        {
            /* Update lock/unlock status */
            Device_p->HWUnLocked = TRUE;

            /* Tell to signal Semaphore */
            SignalHWUnlockedSemaphore = TRUE;
        }

        /* Lock/Unlock related  update*/
        if (Device_p->UnlockRequest == TRUE)
        {
            /* Reset request */
            Device_p->UnlockRequest = FALSE;

            /* Tell to signal Semaphore */
            SignalUnlockInstalledSemaphore = TRUE;
        }
        else if (Device_p->LockRequest == TRUE)
        {
            /* Reset request */
            Device_p->LockRequest = FALSE;

            /* Tell to signal Semaphore */
            SignalLockInstalledSemaphore = TRUE;
        }

        /* Setup control to restart or not the device  */
        if ((Device_p->InsertionRequest == FALSE) && (StopOnLastNode == TRUE))
        {
            Device_p->BlitterIdle = TRUE;

            /* Code for sync chip : Unlock if waiting node is 0 */
            if ( (Device_p->SBlitNodeDataPool.NbFreeElem == Device_p->SBlitNodeDataPool.NbElem) && (Device_p->StartWaitBlitComplete == TRUE) )
            {
                STOS_SemaphoreSignal( Device_p->AllBlitsCompleted_p );
                Device_p->StartWaitBlitComplete = FALSE ;
            }
        }
        else
        {

            /* Reset request */
            if (Device_p->InsertionRequest == TRUE)
            {
                /* Reset request */
                Device_p->InsertionRequest = FALSE;

                /* Tells to signal semaphore */
                SignalInsertionDoneSemaphore = TRUE;
            }

            Device_p->BlitterIdle = FALSE;

#if defined(WA_GNBvd06529) || defined (WA_GNBvd06530) || defined (WA_FILL_OPERATION) || defined (WA_SOFT_RESET_EACH_NODE) || defined (WA_GNBvd15279)
            if (NIPisUpdated == FALSE)
            {
                /* Load NIP */

#ifdef STBLIT_LINUX_FULL_USER_VERSION
                if ( PhysicJobBlitNodeBuffer_p )
                {
                    STSYS_WriteRegDev32LE((void*)((U32)Device_p->BaseAddress_p + BLT_NIP_OFFSET),
                                                (U32)PhysicJobBlitNodeBuffer_p +
                                                (U32) NextNode_p - (U32) Device_p->JobBlitNodeBuffer_p  );
                }
                else
                {
                    if ( (U32) NextNode_p >  (U32) Device_p->SingleBlitNodeBuffer_p )
                    {
                        STSYS_WriteRegDev32LE((void*)((U32)Device_p->BaseAddress_p + BLT_NIP_OFFSET),
                                                    (U32)PhysicSingleBlitNodeBuffer_p +
                                                    (U32) NextNode_p - (U32) Device_p->SingleBlitNodeBuffer_p  );

                    }
                    else
                    {
                        STSYS_WriteRegDev32LE((void*)((U32)Device_p->BaseAddress_p + BLT_NIP_OFFSET),
                                                    (U32)stblit_CpuToDevice((void*)NextNode_p));
                    }
                }
#else
                STSYS_WriteRegDev32LE((void*)((U32)Device_p->BaseAddress_p + BLT_NIP_OFFSET),
                                            (U32)stblit_CpuToDevice((void*)NextNode_p, &(Device_p->VirtualMapping)));
#endif
            }
#endif

            /* Tells to Restart the engine */
            RestartEngine = TRUE;
        }


        /* Release accesss control */
        STOS_SemaphoreSignal(Device_p->AccessControl);


        /* ################################ END OF PROTECTED AREA  ##################################### */
        /* AFTER THIS POINT Parent Device structure may have been updated by other processes)
         * WRITE /READ ACCESS TO PARENT DEVICE STRUCTURE MUST BE USED WITH CARE !!!! */

        /* Notify events */
        if (NotifyBlitCompletion == TRUE)
        {

#ifdef ST_OSLINUX
#if defined(BLIT_DEBUG)
            PrintTrace(("stblit_InterruptProcess(): signal Blit Completion UserTag=0x%x\n", (U32) &UserTag ));
#endif
#endif

#ifdef STBLIT_LINUX_FULL_USER_VERSION
            STOS_SemaphoreSignal( BlitCompletionSemaphore );
#else
#ifdef ST_OSLINUX
            STEVT_Notify(  Device_p->EvtHandle,
                           Device_p->EventID[STBLIT_BLIT_COMPLETED_ID],
                          STEVT_EVENT_DATA_TYPE_CAST &UserTag );
#else
            STEVT_NotifySubscriber(Device_p->EvtHandle,Device_p->EventID[STBLIT_BLIT_COMPLETED_ID],
                                  STEVT_EVENT_DATA_TYPE_CAST &UserTag, SubscriberID );
#endif
#endif
        }
        if ((NotifyJobCompletion == TRUE) && ((void*)JobHandle != NULL))
        {
#ifdef STBLIT_LINUX_FULL_USER_VERSION
        STOS_SemaphoreSignal( JobCompletionSemaphore );
#else
#ifdef ST_OSLINUX
            PrintTrace(("STEVT_Notify(): JobHandle=0x%x\n", JobHandle ));
            STEVT_Notify(  Device_p->EvtHandle,
                           Device_p->EventID[STBLIT_JOB_COMPLETED_ID],
                          STEVT_EVENT_DATA_TYPE_CAST &UserTag  );
#else
            STEVT_NotifySubscriber(Device_p->EvtHandle,Device_p->EventID[STBLIT_JOB_COMPLETED_ID],
                                   STEVT_EVENT_DATA_TYPE_CAST &JobHandle, ((stblit_Job_t*)JobHandle)->SubscriberID );
#endif
#endif

        }

        /* Signal semaphores */
        if (SignalHWUnlockedSemaphore == TRUE)
        {
            STOS_SemaphoreSignal(Device_p->HWUnlockedSemaphore);
        }
        /* Node insertion , lock and unlock are exclusif */
        if (SignalInsertionDoneSemaphore == TRUE)
        {
            STOS_SemaphoreSignal(Device_p->InsertionDoneSemaphore);
        }
        else if (SignalUnlockInstalledSemaphore == TRUE)
        {
            STOS_SemaphoreSignal(Device_p->UnlockInstalledSemaphore);
        }
        else if (SignalLockInstalledSemaphore == TRUE)
        {
            STOS_SemaphoreSignal(Device_p->LockInstalledSemaphore);
        }

        /* Restart engine */
        if (RestartEngine == TRUE)
        {

#ifdef ST_OSLINUX
#ifdef BLIT_DEBUG
            PrintTrace(("ReCall StartEngine\n" ));
#endif
#endif
            StartEngine(Device_p);
        }
    }

}

/* End of blt_be.c */
