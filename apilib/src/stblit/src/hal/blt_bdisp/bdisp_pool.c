/*******************************************************************************

File name   : bdisp_pool.c

Description : Functions of the data pool Manager

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
01 Jun 2000        Created                                          HE
*******************************************************************************/



/* Includes ----------------------------------------------------------------- */
#if !defined(ST_OSLINUX) || defined(STBLIT_LINUX_FULL_USER_VERSION)
#include <stdlib.h>
#include <string.h>
#endif

#include "stddefs.h"
#include "stos.h"
#include "bdisp_init.h"
#include "bdisp_com.h"
#include "bdisp_pool.h"
#ifdef STBLIT_ENABLE_DATA_POOL_FREE_ELEMENT_CHECK
#include "stcommon.h"
#endif /* STBLIT_ENABLE_DATA_POOL_FREE_ELEMENT_CHECK */
#ifdef STBLIT_ENABLE_MISSING_INTERRUPTIONS_WA
#include "stevt.h"
#include "bdisp_mem.h"
#ifdef STBLIT_ENABLE_BASIC_TRACE
#include "bdisp_be.h"
#endif
#endif /* STBLIT_ENABLE_MISSING_INTERRUPTIONS_WA */
/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */

/* Private Variables -------------------------------------------------------- */

/* Private Macros ----------------------------------------------------------- */


/* Private Function prototypes ---------------------------------------------- */


/* Functions ---------------------------------------------------------------- */
/*******************************************************************************
Name        : stblit_InitDataPool
Description : initializes a generic datapool
Parameters  :
Assumptions :
Limitations : No error checking is performed
Returns     :
*******************************************************************************/
void stblit_InitDataPool(
  stblit_DataPoolDesc_t*    DataPool_p,
  U32                       NbElem,
  U32                       ElemSize,
  void*                     ElemArray_p,
  void**                    HandleArray_p)
{
  U32 i;

  DataPool_p->NbFreeElem      = NbElem;
  DataPool_p->NbElem          = NbElem;
  DataPool_p->ElemSize        = ElemSize;
  DataPool_p->ElemArray_p     = ElemArray_p;
  DataPool_p->HandleArray_p   = HandleArray_p;

/* semaphores initialization */
  DataPool_p->AccessControl = STOS_SemaphoreCreateFifo( NULL, 1 );

#ifdef STBLIT_ENABLE_DATA_POOL_FREE_ELEMENT_CHECK
  DataPool_p->FreeElementSemaphore = STOS_SemaphoreCreateFifo( NULL, NbElem );
#endif /* STBLIT_ENABLE_DATA_POOL_FREE_ELEMENT_CHECK */


  /* Initialize Handle array content*/
  for( i=0; i < NbElem; i++)
    DataPool_p->HandleArray_p[i] = (void*) ((U32)ElemArray_p + ElemSize*(NbElem - 1 -i));
}

#if defined(ST_OSWINCE)
void stblit_DebugDataPool(stblit_DataPoolDesc_t* DataPool_p)
{
  printf("stblit_DebugDataPool %p: %d free out of %d items of size %d bytes\n",
            DataPool_p, DataPool_p->NbFreeElem, DataPool_p->NbElem, DataPool_p->ElemSize);
}
#endif

/*******************************************************************************
Name        : stblit_GetElement
Description : gets an element from the data pool returning its handle
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t stblit_GetElement(
  stblit_DataPoolDesc_t* DataPool_p,
  void**                 Handle_p)
{
#ifdef STBLIT_ENABLE_DATA_POOL_FREE_ELEMENT_CHECK
    STOS_Clock_t TimeOut = STOS_time_plus( STOS_time_now(), ST_GetClocksPerSecond() * STBLIT_FREE_ELEMENT_TIME_OUT );

    /* Wait for free element if the pool is full */
    STOS_SemaphoreWaitTimeOut(DataPool_p->FreeElementSemaphore, &TimeOut);
#endif /* STBLIT_ENABLE_DATA_POOL_FREE_ELEMENT_CHECK */

    /* Protect data pool access */
    STOS_SemaphoreWait(DataPool_p->AccessControl);

    if(DataPool_p->NbFreeElem > 0)
    {
        *Handle_p = (void*) DataPool_p->HandleArray_p[DataPool_p->NbFreeElem - 1];
        DataPool_p->HandleArray_p[DataPool_p->NbFreeElem - 1] = NULL;
        DataPool_p->NbFreeElem--;

        /* Release data pool control */
        STOS_SemaphoreSignal(DataPool_p->AccessControl);

        return(ST_NO_ERROR);
    }
    else
    {
        /* Release data pool control */
        STOS_SemaphoreSignal(DataPool_p->AccessControl);

        return(ST_ERROR_NO_FREE_HANDLES);
    }
}

/*******************************************************************************
Name        : stblit_ReleaseElement
Description :
Parameters  :
Assumptions :
Limitations : upper level must ensure an handle is released only once
              upper level must also ensure handle validity
Returns     :
*******************************************************************************/
ST_ErrorCode_t stblit_ReleaseElement(
  stblit_DataPoolDesc_t*  DataPool_p,
  void*                   Handle)
{
    STOS_SemaphoreWait(DataPool_p->AccessControl);

    DataPool_p->NbFreeElem++;
    DataPool_p->HandleArray_p[DataPool_p->NbFreeElem - 1] = Handle;

  /* Release data pool control */
    STOS_SemaphoreSignal(DataPool_p->AccessControl);

#ifdef STBLIT_ENABLE_DATA_POOL_FREE_ELEMENT_CHECK
  /* New element can now be got */
    STOS_SemaphoreSignal(DataPool_p->FreeElementSemaphore);
#endif /* STBLIT_ENABLE_DATA_POOL_FREE_ELEMENT_CHECK */

    return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : stblit_ReleaseNode
Description :
Parameters  :
Assumptions : Access control protection done at upper level
              Valid NodeHandle
Limitations :
Returns     :
*******************************************************************************/
void stblit_ReleaseNode(stblit_Device_t* Device_p, stblit_NodeHandle_t NodeHandle)
{
    U32 i;

    if (((stblit_Node_t*)NodeHandle)->JobHandle == STBLIT_NO_JOB_HANDLE)  /* Single blit node */
    {
        /* Reset Node to  0*/
/*        memset((void*)NodeHandle, 0, sizeof(stblit_Node_t));*/
        for (i=0;i < sizeof(stblit_Node_t) /4 ;i++ )
        {
/*            *((U32*)(NodeHandle + 4*i)) = 0;*/
            STSYS_WriteRegMem32LE((void*)((U32)NodeHandle + 4 * i),0);
        }

        stblit_ReleaseElement(&(Device_p->SBlitNodeDataPool), (void*) NodeHandle);
    }
    else  /* Node from job */
    {
        /* Reset Node to  0*/
/*        memset((void*)NodeHandle, 0, sizeof(stblit_Node_t));*/
        for (i=0;i < sizeof(stblit_Node_t) /4 ;i++ )
        {
/*            *((U32*)(NodeHandle + 4*i)) = 0;*/
            STSYS_WriteRegMem32LE((void*)((U32)NodeHandle + 4 * i),0);
        }

        stblit_ReleaseElement(&(Device_p->JBlitNodeDataPool), (void*) NodeHandle);
    }
}


/*******************************************************************************
Name        : stblit_ReleaseListNode
Description : Being given the last node of the list, it releases and reset to 0 all nodes.
Parameters  :
Assumptions : The PreviousNode field of the first node in the list is STBLIT_NO_JOB_HANDLE
              Access control protection done at upper level. It is due to the fact that this function must
              not deschedule at all (used in backend module!).
Limitations :
Returns     : Nothing
*******************************************************************************/
void stblit_ReleaseListNode(stblit_Device_t* Device_p, stblit_NodeHandle_t NodeHandle)
{
    stblit_NodeHandle_t  PreviousHandle;
    stblit_Node_t*       NextNode;
    U32                  NIP_Value, NullValue = 0;
#ifdef STBLIT_ENABLE_MISSING_INTERRUPTIONS_WA
    stblit_NodeHandle_t  FirstNodeHandle;
#endif


    /* Get the physic value for 0 */
#ifdef STBLIT_LINUX_FULL_USER_VERSION
    if ( PhysicJobBlitNodeBuffer_p != 0 )
    {
        NullValue = (U32) Device_p->JobBlitNodeBuffer_p - (U32) PhysicJobBlitNodeBuffer_p;
    }
    else
    {
        NullValue = (U32) Device_p->SingleBlitNodeBuffer_p - (U32) PhysicSingleBlitNodeBuffer_p;
    }

#else
    NullValue = (U32)stblit_DeviceToCpu( NullValue, &(Device_p->VirtualMapping) );
#endif

    /* Get the Phy adress */
#ifdef STBLIT_LINUX_FULL_USER_VERSION
    if ( PhysicJobBlitNodeBuffer_p != 0 )
    {
        NIP_Value = (U32)(((stblit_Node_t*)NodeHandle)->HWNode.BLT_NIP) + (U32) Device_p->JobBlitNodeBuffer_p - (U32) PhysicJobBlitNodeBuffer_p;
    }
    else
    {
        NIP_Value = (U32)(((stblit_Node_t*)NodeHandle)->HWNode.BLT_NIP) + (U32) Device_p->SingleBlitNodeBuffer_p - (U32) PhysicSingleBlitNodeBuffer_p;
    }
#else
    NIP_Value = (U32)stblit_DeviceToCpu((((stblit_Node_t*)NodeHandle)->HWNode.BLT_NIP), &(Device_p->VirtualMapping) );
#endif

    /* In the case where we have inserted nodes in the end of the executed list we must cut the link between the node
     * on which we start the nodes release and the next node */
    if (  NIP_Value != NullValue )
    {
        NextNode = (stblit_Node_t*)NIP_Value;
        NextNode->PreviousNode = STBLIT_NO_NODE_HANDLE;
    }

#ifdef STBLIT_ENABLE_MISSING_INTERRUPTIONS_WA
    FirstNodeHandle = NodeHandle;
#endif


    while (NodeHandle != STBLIT_NO_NODE_HANDLE)
    {
#ifdef STBLIT_ENABLE_MISSING_INTERRUPTIONS_WA
        U32                  ITOpcode, BLT_INS_Notif;
        void*                UserTag_p;
        STEVT_SubscriberID_t SubscriberID;
        STBLIT_JobHandle_t   JobHandle;

        /* Get Notification Bit */
        BLT_INS_Notif = (((((stblit_Node_t*)NodeHandle)->HWNode.BLT_INS) >> STBLIT_INS_ENABLE_IRQ_SHIFT) & STBLIT_INS_ENABLE_IRQ_MASK);

        /* If Notification bit is enabled, this means that this node was not taged by the blitter Hw,
         * so in this WA we should notify the user for the complete event */
        if ((BLT_INS_Notif == 1) && (NodeHandle != FirstNodeHandle))
        {
            /* Get notification information */
            UserTag_p = (void*)((stblit_Node_t*)NodeHandle)->UserTag_p;
            SubscriberID=((stblit_Node_t*)NodeHandle)->SubscriberID;
            ITOpcode =((stblit_Node_t*)NodeHandle)->ITOpcode;
            JobHandle = ((stblit_Node_t*)NodeHandle)->JobHandle;

            /* Notify user */
            if ((ITOpcode & STBLIT_IT_OPCODE_BLIT_COMPLETION_MASK) != 0)
            {
                /* Notify Blit completion as required. */
#ifdef ST_OSLINUX
                STEVT_Notify(  Device_p->EvtHandle,
                            Device_p->EventID[STBLIT_BLIT_COMPLETED_ID],
                            STEVT_EVENT_DATA_TYPE_CAST &UserTag_p );
#else
#if 0 /*yxl 2007-06-19 temp modify for test*/
                STEVT_NotifySubscriber(Device_p->EvtHandle,Device_p->EventID[STBLIT_BLIT_COMPLETED_ID], STEVT_EVENT_DATA_TYPE_CAST &UserTag_p, SubscriberID);
#else
                STEVT_NotifySubscriber(Device_p->EvtHandle,Device_p->EventID[STBLIT_BLIT_COMPLETED_ID],
					STEVT_EVENT_DATA_TYPE_CAST UserTag_p, SubscriberID);

#endif/*end yxl 2007-06-19 temp modify for test*/
#endif

#ifdef STBLIT_ENABLE_BASIC_TRACE
                STBLIT_TRACE("\r\n[WA%d] U=%s H=%x", ++Device_p->NbBltInt, UserTag_p, (U32)NodeHandle);
#endif /* STBLIT_ENABLE_BASIC_TRACE */
            }
            if ((ITOpcode & STBLIT_IT_OPCODE_JOB_COMPLETION_MASK) != 0)
            {
                /* Notify JOB completion as required. */
#ifdef ST_OSLINUX
                STEVT_Notify(  Device_p->EvtHandle,
                           Device_p->EventID[STBLIT_JOB_COMPLETED_ID],
                          STEVT_EVENT_DATA_TYPE_CAST &UserTag_p );
#else
                STEVT_NotifySubscriber( Device_p->EvtHandle,Device_p->EventID[STBLIT_JOB_COMPLETED_ID],  STEVT_EVENT_DATA_TYPE_CAST &UserTag_p,
                                        ((stblit_Job_t*)JobHandle)->SubscriberID);
#endif

#ifdef STBLIT_ENABLE_BASIC_TRACE
                STBLIT_TRACE("\r\n[WA%d] U=%s H=%x", ++Device_p->NbBltInt, UserTag_p, (U32)NodeHandle);
#endif /* STBLIT_ENABLE_BASIC_TRACE */
            }
        }
#endif /* STBLIT_ENABLE_MISSING_INTERRUPTIONS_WA */

        PreviousHandle = ((stblit_Node_t*)NodeHandle)->PreviousNode;
        stblit_ReleaseNode(Device_p,NodeHandle);
        NodeHandle = PreviousHandle;
    }
}



/* End of blt_pool.c */


