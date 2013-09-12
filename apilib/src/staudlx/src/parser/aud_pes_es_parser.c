/******************************************************************************/
/*                                                                            */
/* File name   : AUD_PES_ES_PARSER.C                                          */
/*                                                                            */
/* Description : PES+ES parser file                                           */
/*                                                                            */
/* COPYRIGHT (C) ST-Microelectronics 2005                                     */
/* History     :                                                              */
/* Date               Modification                 Name                       */
/* ----               ------------                 ----                       */
/* 15/09/05           Created                      K.MAITI                    */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/******************************************************************************/

//#define  STTBX_PRINT
#ifndef ST_OSLINUX
#include "sttbx.h"
#endif
#include "stos.h"
#include "aud_pes_es_parser.h"

ST_DeviceName_t PES_ES_DeviceName[] = {"PESES0","PESES1", "PESES2"};

#define POLLING_DURATION_FIXED  0
#define USE_TRACE_BUFFER_FOR_DEBUG 0


#if USE_TRACE_BUFFER_FOR_DEBUG
static osclock_t previoustime;
extern char Trace_PrintBuffer[32]; /* global variable to be seen by modules */
extern void DUMP_DATA(char *info);
 /* ifdef USE_TRACE_BUFFER_FOR_DEBUG */
U32 STC_array_aud [5][10000];
U32 sampleCount_aud =0;
#endif

#ifndef MSPP_PARSER
#ifdef STAUD_INSTALL_HEAAC
extern ST_ErrorCode_t Parse_HEAAC(PESES_ParserControl_t *ControlBlock);
extern ST_ErrorCode_t HEAAC_Init(PESES_ParserControl_t *ControlBlock);
extern ST_ErrorCode_t HEAAC_Stop(PESES_ParserControl_t *ControlBlock);
extern ST_ErrorCode_t HEAAC_Start(PESES_ParserControl_t *ControlBlock);
extern ST_ErrorCode_t GetStreamInfoHEAAC(PESES_ParserControl_t *ControlBlock, STAUD_StreamInfo_t * StreamInfo);
#endif
#endif

#ifdef MSPP_PARSER
extern ST_ErrorCode_t Mspp_Parse_Frame(PESES_ParserControl_t *ControlBlock);
extern ST_ErrorCode_t Mspp_Parser_Start(PESES_ParserControl_t *ControlBlock);
extern ST_ErrorCode_t Mspp_Parser_Stop(PESES_ParserControl_t *ControlBlock);
extern ST_ErrorCode_t Mspp_Parser_Term(PESES_ParserControl_t *ControlBlock_p);
#else
extern ST_ErrorCode_t Com_Parse_Frame(PESES_ParserControl_t *ControlBlock);
extern ST_ErrorCode_t Com_Parser_Start(PESES_ParserControl_t *ControlBlock);
extern ST_ErrorCode_t Com_Parser_Stop(PESES_ParserControl_t *ControlBlock);
extern ST_ErrorCode_t Com_Parser_Term(PESES_ParserControl_t *ControlBlock_p);
#endif

 /* Queue of initialized Memory Pool */
PESES_ParserControl_t *PESESHead_p = NULL;

static STOS_Mutex_t *PESESInitMutex_p = NULL;

/* Private Functions Prototypes -------------------------------------------- */
static void PESES_ResetParams(PESES_ParserControl_t *ControlBlock_p,BOOL x);
ParserState_t GetPESES_ParseState(STPESES_Handle_t Handle);

/* Memory Pool Queue Management */
static U32  PESES_QueueSize(void);

static void PESES_QueueAdd(PESES_ParserControl_t *Item_p);

static void PESES_QueueRemove(PESES_ParserControl_t *Item_p);

static BOOL PESES_IsInit(const ST_DeviceName_t PoolName);

static PESES_ParserControl_t *
   PESES_GetControlBlockFromName(const ST_DeviceName_t);

static PESES_ParserControl_t *
   PESES_GetControlBlockFromHandle(STPESES_Handle_t Handle);

/*****************************************************************************
PESES_QueueSize()
Description:
    This routine finds the no of allocated Parser control blocks.

Parameters:
    void.
Return Value:
    U32, the no of allocated Parser control blocks.
See Also:

*****************************************************************************/
static U32  PESES_QueueSize(void)
{
    U32 QueueSize=0;
    PESES_ParserControl_t *qp = PESESHead_p;

    while(qp)
    {
        QueueSize++;
        qp = qp->Next_p;
    }

    return QueueSize;

} /*PESES_QueueSize()*/

/*****************************************************************************
PESES_QueueAdd()
Description:
    This routine appends an allocated Parser control block to the
    queue.

    NOTE: The semaphore lock must be held when calling this routine.

Parameters:
    Item_p, the control block to append to the queue.
Return Value:
    Nothing.
See Also:
    PESES_QueueRemove()
*****************************************************************************/
static void PESES_QueueAdd(PESES_ParserControl_t *Item_p)
{


    /* Check the base element is defined, otherwise, append to the end of the
    * linked list.
    */
    if (PESESHead_p == NULL)
    {
        /* As this is the first item, no need to append */
        PESESHead_p = Item_p;
        //PESESHead_p->Next_p = NULL;
    }
    else
    {
        PESES_ParserControl_t *qp;
        /* Iterate along the list until we reach the final item */
        qp = PESESHead_p;
        while (qp->Next_p)
        {
            qp = qp->Next_p;
        }

        /* Append the new item */
        qp->Next_p = Item_p;

    }
    Item_p->Next_p = NULL;
} /*PESES_QueueAdd*/

/*****************************************************************************
PESES_QueueRemove()
Description:
    This routine removes an allocated Parser control block from the
    Parser queue.

    NOTE: The semaphore lock must be held when calling this routine.
    Use PESES_IsInit() or PESES_GetControlBlockFromName() to verify whether
    or not a particular item is in the queue prior to calling this routine.

Parameters:
    Item_p, the control block to remove from the queue.
Return Value:
    Nothing.
See Also:
    PESES_QueueAdd()
*****************************************************************************/
static void PESES_QueueRemove(PESES_ParserControl_t *Item_p)
{
    PESES_ParserControl_t *this_p, *last_p;

    /* Check the base element is defined, otherwise quit as no items are
     * present on the queue.
     */
    if (PESESHead_p != NULL)
    {
        /* Reset pointers to loop start conditions */
        last_p = NULL;
        this_p = PESESHead_p;

        /* Iterate over each queue element until we find the required
         * queue item to remove.
         */
        while (this_p != NULL && this_p != Item_p)
        {
            last_p = this_p;
            this_p = this_p->Next_p;
        }

        /* Check we found the queue item */
        if (this_p == Item_p)
        {
            /* Unlink the item from the queue */
            if (last_p != NULL)
            {
                last_p->Next_p = this_p->Next_p;
            }
            else
            {
                /* Recalculate the new head of the queue i.e.,
                 * we have just removed the queue head.
                 */
                PESESHead_p = this_p->Next_p;
            }
        }
    }
}/*PESES_QueueRemove*/

/*****************************************************************************
PESES_IsInit()
Description:
    Runs through the queue of initialized objects and checks that
    the object with the specified device name has not already been added.

    NOTE: Must be called with the queue semaphore held.

Parameters:
    Name, text string indicating the device name to check.
Return Value:
    TRUE, the device has already been initialized.
    FALSE, the device is not in the queue and therefore is not initialized.
See Also:
    PESES_Init()
*****************************************************************************/
static BOOL PESES_IsInit(const ST_DeviceName_t Name)
{
    BOOL Initialized = FALSE;
    PESES_ParserControl_t *qp = PESESHead_p; /* Global queue head pointer */

    while (qp)
    {
        /* Check the device name for a match with the item in the queue */
        if (strcmp(qp->Name, Name) != 0)
        {
            /* Next Mem Pool in the queue */
            qp = qp->Next_p;
        }
        else
        {
            /* The object is already initialized */
            Initialized = TRUE;
            break;
        }
    }

    /* Boolean return value reflecting whether the device is initialized */
    return Initialized;
} /* PESES_IsInit() */

/*****************************************************************************
PESES_GetControlBlockFromName()
Description:
    Runs through the queue of initialized objects and checks for
    the Mem Pool with the specified name.

    NOTE: Must be called with the queue semaphore held.

Parameters:
    Name, text string indicating the device name to check.
Return Value:
    NULL, end of queue encountered - invalid device name.
    Otherwise, returns a pointer to the control block for the device.
See Also:
    PESES_IsInit()
*****************************************************************************/
static PESES_ParserControl_t *
   PESES_GetControlBlockFromName(const ST_DeviceName_t Name)
{
    PESES_ParserControl_t *qp = PESESHead_p; /* Global queue head pointer */

    while (qp != NULL)
    {
        /* Check the name for a match with the item in the queue */
        if (strcmp(qp->Name, Name) != 0)
        {
            /* Next object in the queue */
            qp = qp->Next_p;
        }
        else
        {
            /* The entry has been found */
            break;
        }
    }

    /* Return the control block (or NULL) to the caller */
    return qp;

}/*PESES_GetControlBlockFromName()*/

/*****************************************************************************
PESES_GetControlBlockFromHandle()
Description:
    Runs through the queue of initialized objects and checks for
    the object with the specified handle.

    NOTE: Must be called with the queue semaphore held.

Parameters:
    Handle, an open handle.
Return Value:
    NULL, end of queue encountered - invalid handle.
    Otherwise, returns a pointer to the control block for the device.
See Also:
    PESES_GetControlBlockFromName()
*****************************************************************************/
static PESES_ParserControl_t *
   PESES_GetControlBlockFromHandle(STPESES_Handle_t Handle)
{
    PESES_ParserControl_t *qp = PESESHead_p; /* Global queue head pointer */

    if(Handle)
    {
        /* Iterate over the Mem Pool queue */
        while (qp)
        {
            /* Check for a matching handle */
            if (Handle == qp->Handle)
            {
                break;  /* This is a valid handle */
            }

            /* Try the next object */
            qp = qp->Next_p;
        }
    }
    else
    {
        qp = NULL;
    }

    /* Return the object control block (or NULL) to the caller */
    return qp;
} /*PESES_GetControlBlockFromHandle()*/

/*****************************************************************************
PESES_ReleaseMemory()
Description:
    Releases the allocate resources in aud_pes_es_parsre
    NOTE: this will also release the held PESESInitMutex_p

Parameters:
    qp_p, parser control block.
Return Value:

See Also:

*****************************************************************************/
ST_ErrorCode_t PESES_ReleaseMemory(PESES_ParserControl_t *qp_p)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
#ifndef ST_51XX
    BOOL   AllocatedFromEMBX=FALSE;
#endif
    if(qp_p)
    {
        ST_Partition_t  *CPUPartition = qp_p->CPUPartition_p;

        if(qp_p->EvtHandle)
        {
            error = PESES_UnSubscribeEvents(qp_p);
        }

        /*Free allocated memories*/
        if(qp_p->PESESCurrentState.Lock)
        {
            STOS_MutexDelete(qp_p->PESESCurrentState.Lock);
            qp_p->PESESCurrentState.Lock = NULL;
        }

        if(qp_p->PESESNextState.Lock)
        {
            STOS_MutexDelete(qp_p->PESESNextState.Lock);
            qp_p->PESESNextState.Lock = NULL;
        }

        if(qp_p->Lock_p)
        {
            /*free Mutex*/
            STOS_MutexDelete(qp_p->Lock_p);
            qp_p->Lock_p = NULL;
        }

        if(qp_p->SemWait_p)
        {
            /*free semaphore*/
            STOS_SemaphoreDelete(NULL,qp_p->SemWait_p);
            qp_p->SemWait_p = NULL;
        }

        if(qp_p->SemComplete_p)
        {
            /*free semaphore*/
            STOS_SemaphoreDelete(NULL,qp_p->SemComplete_p);
            qp_p->SemComplete_p = NULL;
        }

        if(qp_p->PESbufferAddress_p)
        {
            if(qp_p->PESBufferHandle)
            {
                STAVMEM_FreeBlockParams_t FreeBlockParams;
                FreeBlockParams.PartitionHandle = qp_p->AudioBufferPartition;
                error |= STAVMEM_FreeBlock(&FreeBlockParams,&qp_p->PESBufferHandle);
                if(error == ST_NO_ERROR)
                {
                    qp_p->PESBufferHandle = 0;
                }
            }
#ifndef STAUD_REMOVE_EMBX
            else
            {
                AllocatedFromEMBX = TRUE;
                error |= EMBX_Free((EMBX_VOID *)qp_p->PESbufferAddress_p);
            }
#endif
            qp_p->PESbufferAddress_p = NULL;
            qp_p->BitBufferSize         = 0;
        }

#if (defined STAUD_INSTALL_AIFF) || (defined STAUD_INSTALL_WAVE)
		if(qp_p->DummyBufferBaseAddress_p)
		{
			if(qp_p->DummyBufferHandle)
			{
			    STAVMEM_FreeBlockParams_t FreeBlockParams;
				FreeBlockParams.PartitionHandle	= qp_p->AudioBufferPartition;
				error |= STAVMEM_FreeBlock(&FreeBlockParams, &(qp_p->DummyBufferHandle));
				qp_p->DummyBufferHandle = 0;
			}
#ifndef STAUD_REMOVE_EMBX
    	    else
    	    {
    		    error |= EMBX_Free((EMBX_VOID *)qp_p->DummyBufferBaseAddress_p);
    	    }
#endif
        }
#endif

#ifndef STAUD_REMOVE_EMBX
        if(AllocatedFromEMBX)
        {
            error |= EMBX_CloseTransport(qp_p->tp);
        }
#endif

#if defined (PES_TO_ES_BY_FDMA) || defined (STAUD_INSTALL_WAVE) || defined(STAUD_INSTALL_AIFF)
        if(qp_p->NodePESToES_p)
        {
            STAVMEM_FreeBlockParams_t FreeBlockParams;
            FreeBlockParams.PartitionHandle = qp_p->AVMEMPartition;
            error |= STAVMEM_FreeBlock(&FreeBlockParams,&qp_p->PESToESBufferHandle);

            if(error == ST_NO_ERROR)
            {
                qp_p->PESToESBufferHandle = 0;
                qp_p->NodePESToES_p         = NULL;
            }
        }

        if(qp_p->PESToESChannelID)
        {
            error |= STFDMA_UnlockChannel(qp_p->PESToESChannelID,qp_p->FDMABlock);
            qp_p->PESToESChannelID = 0;
        }
#endif

#ifdef MSPP_PARSER
        if(qp_p->Mspp_Cntrl_p)
        {
            error |= Mspp_Parser_Term(qp_p);
            qp_p->Mspp_Cntrl_p = NULL;
        }
#else
        if(qp_p->ComParser_p)
        {
            error |= Com_Parser_Term(qp_p);
            qp_p->ComParser_p = NULL;
        }
#endif


        /*Free allocated memories*/
        STOS_MemoryDeallocate(CPUPartition, qp_p);

    }
    /* Now release the mutex */
    STOS_MutexRelease(PESESInitMutex_p);
    return error;
}

/*****************************************************************************
PESES_Init()
Description:
    Initializes the PES-ES Parser.


Parameters:
    DeviceName, text string indicating the device name to be used.
    InitParams_p, parameters to control the initialization of the device.
     Handle, Handle to the device.
Return Value:
    ST_NO_ERROR,                    no errors.
    ST_ERROR_NO_MEMORY,             unable to allocate memory.
    ST_ERROR_BAD_PARAMETER,         one or more of the parameters was invalid.
    ST_ERROR_ALREADY_INITIALIZED,   another user has already initialized the
                                    device.
    ST_ERROR_FEATURE_NOT_SUPPORTED, the device type is not supported.
    ST_ERROR_UNKNOWN_DEVICE,        invalid device name .
    ST_ERROR_NO_FREE_HANDLES,       another user has already opened the Pool,
                                    or it is in use.
See Also:
    PESES_Term()
*****************************************************************************/
ST_ErrorCode_t PESES_Init(ST_DeviceName_t  Name, PESESInitParams_t *InitParams_p,
                                     STPESES_Handle_t *Handle)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    PESES_ParserControl_t *qp_p = NULL;

    STAVMEM_AllocBlockParams_t  AllocParams;

#if defined (PES_TO_ES_BY_FDMA) || defined (STAUD_INSTALL_WAVE) || defined (STAUD_INSTALL_AIFF)
    U32 Size;
    void * AllocAddress;
#endif

    ST_Partition_t  *CPUPartition = NULL;
    U32 i;

   STOS_TaskLock();


    if(PESESInitMutex_p == NULL)
    {
        PESESInitMutex_p = STOS_MutexCreateFifo();
    }
    STOS_TaskUnlock();

     STOS_MutexLock(PESESInitMutex_p);
    /* Check the parameters passed to init. */
    if ((strlen(Name) > ST_MAX_DEVICE_NAME) || (InitParams_p->CPUPartition_p == NULL) ||
        (strlen(InitParams_p->EvtHandlerName) > ST_MAX_DEVICE_NAME))
    {
        /*Bad parameter*/
        error = PESES_ReleaseMemory(qp_p);
        return ST_ERROR_BAD_PARAMETER;
    }

    if(PESES_QueueSize() >= PES_ES_MAX_NUMBER)
    {
        error = PESES_ReleaseMemory(qp_p);
        return ST_ERROR_NO_MEMORY;
    }

    if(PESES_IsInit(Name))
    {
        /*Already initialized*/
        error = PESES_ReleaseMemory(qp_p);
        return ST_ERROR_ALREADY_INITIALIZED;
    }

    /*Allocate the control block*/
    CPUPartition = InitParams_p->CPUPartition_p;
    qp_p = (PESES_ParserControl_t *)STOS_MemoryAllocate(CPUPartition,sizeof(PESES_ParserControl_t));

    if(qp_p == NULL)
    {
        /*No memory*/
        error = PESES_ReleaseMemory(qp_p);
        return ST_ERROR_NO_MEMORY;
    }

    /*Initialize the controll block*/
    memset(qp_p,0,sizeof(PESES_ParserControl_t));
    strncpy(qp_p->Name, Name, ST_MAX_DEVICE_NAME_TOCOPY);

    qp_p->CPUPartition_p                = CPUPartition;
    qp_p->AVMEMPartition                = InitParams_p->AVMEMPartition;
    qp_p->AudioBufferPartition          = InitParams_p->AudioBufferPartition;
    qp_p->PESbufferAddress_p            = NULL;

#if ((defined STAUD_INSTALL_AIFF) || (defined STAUD_INSTALL_WAVE))
    qp_p->DummyBufferSize               = MAXBUFFSIZE;
#endif

    qp_p->PESBlock.StartPointer = 0;
    qp_p->PESBlock.ValidSize = 0;

    //qp_p->StartupState                    =; to be initialized in start
    /* Set the PESES parser State */
    qp_p->PESESCurrentState.State       = TIMER_INIT;   /*Current state of Parser*/
    qp_p->PESESCurrentState.Lock        = STOS_MutexCreateFifo();
    qp_p->PESESNextState.State          = TIMER_INIT;   /*Next state of Parser*/
    qp_p->PESESNextState.Lock           = STOS_MutexCreateFifo();
    qp_p->PESESInputState               = PARSER_STATE_IDLE;
    qp_p->StreamID                      = 0;

    qp_p->EOFFlag                       = FALSE;
    qp_p->Speed                         = 100; /*default speed is 100 == 1x*/
    /* Setting the BroadCast Profile to a default value of DVB */
    qp_p->BroadcastProfile              = STAUD_BROADCAST_DVB;

#if defined (PES_TO_ES_BY_FDMA) || defined (STAUD_INSTALL_WAVE) || defined (STAUD_INSTALL_AIFF)
    qp_p->FDMABlock                     = STFDMA_1; // to be moved to init params if required
#endif

#ifndef STAUD_REMOVE_CLKRV_SUPPORT
    qp_p->CLKRV_Handle                  = InitParams_p->CLKRV_Handle;
    qp_p->CLKRV_HandleForSynchro        = InitParams_p->CLKRV_HandleForSynchro;
#endif
    qp_p->SamplingFrequency = InitParams_p->SamplingFrequency;

    /*Set Startup sync counts*/
    qp_p->LowDataLevel                  = 0;
    qp_p->LowLevelDataEvent             = FALSE;

    /*Input block manager module*/
    qp_p->IpBufHandle                   = InitParams_p->IpBufHandle;

    /*output block manager module*/
    qp_p->OpBufHandle                   = InitParams_p->OpBufHandle;
    qp_p->EnableMSPPSupport             =   InitParams_p->EnableMSPPSupport;
#if defined( ST_OSLINUX )
    qp_p->NumberOfTicksPerSecond        = STLINUX_GetClocksPerSecond();
#else
    qp_p->NumberOfTicksPermSecond       = (ST_GetClocksPerSecond()/1000);
#endif

    qp_p->Lock_p        = STOS_MutexCreateFifo();
#if defined(ST_OS20)
    qp_p->SemWait_p     = STOS_SemaphoreCreateFifoTimeOut(NULL,0);
#else
    qp_p->SemWait_p     = STOS_SemaphoreCreateFifo(NULL,0);
#endif
    qp_p->SemComplete_p = STOS_SemaphoreCreateFifo(NULL,0);

    for(i = 0;i < NUM_NODES_PARSER;i++)
    {
        qp_p->LinearQueue[i].Valid      = FALSE;
        qp_p->LinearQueue[i].OpBlk_p    = NULL;

    }
    qp_p->Blk_in            = 0;
    qp_p->Blk_in_Next       = 0;
    qp_p->Blk_out           = 0;

    qp_p->BitBufferSize     = InitParams_p->PESbufferSize;

    if(!(qp_p->IpBufHandle))
    {
        if(qp_p->AudioBufferPartition)
        {
            /*Allocate from Audio AVMEM*/
            STAVMEM_SharedMemoryVirtualMapping_t   VirtualMapping;
            void                                   *VirtualAddress;

            memset(&AllocParams,0,sizeof(STAVMEM_AllocBlockParams_t));

            STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping);

#if ((defined STAUD_INSTALL_AIFF) || (defined STAUD_INSTALL_WAVE))
            AllocParams.PartitionHandle          = InitParams_p->AudioBufferPartition;
            AllocParams.Alignment                = 64;
            AllocParams.AllocMode                = STAVMEM_ALLOC_MODE_TOP_BOTTOM;
            AllocParams.ForbiddenRangeArray_p    = NULL;
            AllocParams.ForbiddenBorderArray_p   = NULL;
            AllocParams.NumberOfForbiddenRanges  = 0;
            AllocParams.NumberOfForbiddenBorders = 0;
            AllocParams.Size                     = qp_p->DummyBufferSize;

            error = STAVMEM_AllocBlock(&AllocParams, &qp_p->DummyBufferHandle);
            if(error != ST_NO_ERROR)
            {
                /*No memory*/
                error = PESES_ReleaseMemory(qp_p);
                return ST_ERROR_NO_MEMORY;
            }


            error = STAVMEM_GetBlockAddress(qp_p->DummyBufferHandle, &VirtualAddress);
            if(error != ST_NO_ERROR)
            {
                /*No memory*/
                error = PESES_ReleaseMemory(qp_p);
                return ST_ERROR_NO_MEMORY;
            }

            qp_p->DummyBufferBaseAddress_p= (U8 *)STAVMEM_VirtualToCPU(((char*)VirtualAddress),&VirtualMapping);
#ifndef MSPP_PARSER
            memset((char *)qp_p->DummyBufferBaseAddress_p, 0, qp_p->DummyBufferSize);
#endif
#endif

            AllocParams.PartitionHandle          = qp_p->AudioBufferPartition;
            AllocParams.Alignment                = 32;
            AllocParams.AllocMode                = STAVMEM_ALLOC_MODE_TOP_BOTTOM;
            AllocParams.ForbiddenRangeArray_p    = NULL;
            AllocParams.ForbiddenBorderArray_p   = NULL;
            AllocParams.NumberOfForbiddenRanges  = 0;
            AllocParams.NumberOfForbiddenBorders = 0;
            AllocParams.Size                     = qp_p->BitBufferSize;

            error = STAVMEM_AllocBlock(&AllocParams, &qp_p->PESBufferHandle);
            if(error != ST_NO_ERROR)
            {
                /*No memory*/
                error = PESES_ReleaseMemory(qp_p);
                return ST_ERROR_NO_MEMORY;
            }

            /*Buffer Allocated*/
            error = STAVMEM_GetBlockAddress(qp_p->PESBufferHandle, &VirtualAddress);
            if(error != ST_NO_ERROR)
            {
                /*No memory*/
                error = PESES_ReleaseMemory(qp_p);
                return ST_ERROR_NO_MEMORY;
            }

            qp_p->PESbufferAddress_p            = (U32 *)STAVMEM_VirtualToCPU(((char*)VirtualAddress),&VirtualMapping);
#ifndef MSPP_PARSER
            memset((char *)qp_p->PESbufferAddress_p, 0, qp_p->BitBufferSize);
#endif
            /*Set PES buffer params*/
            qp_p->PTIBasePointer_p              = (U8 *)qp_p->PESbufferAddress_p;
            qp_p->PTITopPointer_p               = qp_p->PTIBasePointer_p + InitParams_p->PESbufferSize;
            qp_p->PreviousPTIWritePointer_p     = qp_p->PTIBasePointer_p;
            qp_p->CurrentPTIWritePointer_p      = qp_p->PreviousPTIWritePointer_p;
            qp_p->PTIReadPointer_p              = qp_p->PTIBasePointer_p;
            qp_p->PreviousPTIReadPointer_p      = qp_p->PTIBasePointer_p;
            qp_p->BitBufferThreshold            = qp_p->BitBufferSize/MAX_BIT_BUFFER;
            qp_p->PointerDifference             = 0;
            qp_p->PESFilled                     = 0;
            qp_p->sentForParsing                = 0;
            qp_p->BytesUsed                     = 0;
            qp_p->PTIBufferUnderflowCount       = 0;
        }
#ifndef STAUD_REMOVE_EMBX
        else
        {
            /*Allocate from EMBX*/

            error = EMBX_OpenTransport(EMBX_TRANSPORT_NAME, &qp_p->tp);
            if(error != EMBX_SUCCESS)
            {
                /*No memory*/
                error = PESES_ReleaseMemory(qp_p);
                return ST_ERROR_NO_MEMORY;
            }

#if ((defined STAUD_INSTALL_AIFF) || (defined STAUD_INSTALL_WAVE))
            error = EMBX_Alloc(qp_p->tp, qp_p->DummyBufferSize, (EMBX_VOID **)(&qp_p->DummyBufferBaseAddress_p));
            if(error != EMBX_SUCCESS)
            {
                /*No memory*/
                error = PESES_ReleaseMemory(qp_p);
                return ST_ERROR_NO_MEMORY;
            }
#endif
            qp_p->BitBufferSize = InitParams_p->PESbufferSize;

            error = EMBX_Alloc(qp_p->tp, (qp_p->BitBufferSize), (EMBX_VOID **)(&qp_p->PESbufferAddress_p));
            if(error != EMBX_SUCCESS)
            {
                /*No memory*/
                error = PESES_ReleaseMemory(qp_p);
                return ST_ERROR_NO_MEMORY;
            }

#ifndef MSPP_PARSER
            memset((char *)qp_p->PESbufferAddress_p, 0, qp_p->BitBufferSize);
#endif
            /*Set PES buffer params*/
            qp_p->PTIBasePointer_p              = (U8 *)qp_p->PESbufferAddress_p;
            qp_p->PTITopPointer_p               = qp_p->PTIBasePointer_p + InitParams_p->PESbufferSize;
            qp_p->PreviousPTIWritePointer_p     = qp_p->PTIBasePointer_p;
            qp_p->CurrentPTIWritePointer_p      = qp_p->PreviousPTIWritePointer_p;
            qp_p->PTIReadPointer_p              = qp_p->PTIBasePointer_p;
            qp_p->PreviousPTIReadPointer_p      = qp_p->PTIBasePointer_p;
            qp_p->BitBufferThreshold            = qp_p->BitBufferSize/MAX_BIT_BUFFER;
            qp_p->PointerDifference             = 0;
            qp_p->PESFilled                     = 0;
            qp_p->sentForParsing                = 0;
            qp_p->BytesUsed                     = 0;
            qp_p->PTIBufferUnderflowCount       = 0;
        }
#endif
    }
    else
    {
        /*we have a valid input BM. So get data from there*/
    }
#if defined (PES_TO_ES_BY_FDMA) || defined (STAUD_INSTALL_WAVE) || defined (STAUD_INSTALL_AIFF)

    Size=(sizeof(STFDMA_GenericNode_t));
    AllocParams.PartitionHandle          = qp_p->AVMEMPartition;
    AllocParams.Alignment                = 32;
    AllocParams.AllocMode                = STAVMEM_ALLOC_MODE_TOP_BOTTOM;
    AllocParams.ForbiddenRangeArray_p    = NULL;
    AllocParams.ForbiddenBorderArray_p   = NULL;
    AllocParams.NumberOfForbiddenRanges  = 0;
    AllocParams.NumberOfForbiddenBorders = 0;
    AllocParams.Size                     = Size;

    error = STAVMEM_AllocBlock(&AllocParams, &qp_p->PESToESBufferHandle);
    if(error != ST_NO_ERROR)
    {
        /*No memory*/
        error = PESES_ReleaseMemory(qp_p);
        return ST_ERROR_NO_MEMORY;
    }

    error = STAVMEM_GetBlockAddress(qp_p->PESToESBufferHandle, &AllocAddress);
    if(error != ST_NO_ERROR)
    {
        /*No memory*/
        error = PESES_ReleaseMemory(qp_p);
        return ST_ERROR_NO_MEMORY;
    }

    qp_p->NodePESToES_p=(STFDMA_GenericNode_t *)AllocAddress;

    qp_p->NodePESToES_p->Node.NumberBytes                       = 0; /*will change depending upon frame Size*/
    qp_p->NodePESToES_p->Node.SourceAddress_p                   = NULL;/* will change depending upon Offset  */
    qp_p->NodePESToES_p->Node.DestinationAddress_p              =NULL; /* will change depening upon ES Buffer*/
    qp_p->NodePESToES_p->Node.Length                            = 0;  /* will chnage depending upon frame size*/
    qp_p->NodePESToES_p->Node.SourceStride                      = 0;
    qp_p->NodePESToES_p->Node.DestinationStride                 = 0;
    qp_p->NodePESToES_p->Node.NodeControl.PaceSignal            = STFDMA_REQUEST_SIGNAL_NONE;
    qp_p->NodePESToES_p->Node.NodeControl.SourceDirection       = STFDMA_DIRECTION_INCREMENTING;
    qp_p->NodePESToES_p->Node.NodeControl.DestinationDirection  = STFDMA_DIRECTION_INCREMENTING;
    qp_p->NodePESToES_p->Node.NodeControl.NodeCompleteNotify    = TRUE;
    qp_p->NodePESToES_p->Node.NodeControl.NodeCompletePause     = TRUE;
    qp_p->NodePESToES_p->Node.NodeControl.Reserved              = 0;
    qp_p->NodePESToES_p->Node.Next_p                            =NULL;

    error =STFDMA_LockChannelInPool(STFDMA_DEFAULT_POOL,&qp_p->PESToESChannelID,qp_p->FDMABlock);
    if(error != ST_NO_ERROR)
    {
        /*Not able to get channel*/
        error = PESES_ReleaseMemory(qp_p);
        return ST_ERROR_NO_MEMORY;
    }

    qp_p->PESTOESTransferParams.ChannelId          = qp_p->PESToESChannelID;
    qp_p->PESTOESTransferParams.Pool               = STFDMA_DEFAULT_POOL;
    qp_p->PESTOESTransferParams.BlockingTimeout    = 0;
    qp_p->PESTOESTransferParams.CallbackFunction   = NULL; /* NO CallBack */
    qp_p->PESTOESTransferParams.ApplicationData_p  = NULL;
    qp_p->PESTOESTransferParams.FDMABlock          = qp_p->FDMABlock;
    qp_p->PESTOESTransferParams.NodeAddress_p      = qp_p->NodePESToES_p;

#endif

    strncpy(qp_p->EvtHandlerName, InitParams_p->EvtHandlerName,ST_MAX_DEVICE_NAME_TOCOPY);
    strncpy(qp_p->Name, InitParams_p->Name,ST_MAX_DEVICE_NAME_TOCOPY);
    strncpy(qp_p->EvtGeneratorName, InitParams_p->EvtGeneratorName,ST_MAX_DEVICE_NAME_TOCOPY);

    qp_p->ObjectID = InitParams_p->ObjectID;
    error = PESES_SubscribeEvents(InitParams_p, qp_p);
    if(error != ST_NO_ERROR)
    {
        /*Not able to get channel*/
        error = PESES_ReleaseMemory(qp_p);
        return ST_ERROR_NO_MEMORY;
    }

    error = PESES_RegisterEvents(InitParams_p->EvtHandlerName,InitParams_p->EvtGeneratorName,qp_p);
    if(error != ST_NO_ERROR)
    {
        /*Not able to get channel*/
        error = PESES_ReleaseMemory(qp_p);
        return ST_ERROR_NO_MEMORY;
    }
#ifdef MSPP_PARSER

    {
        MspParserInit_t MsppInitParams;
        memset(&MsppInitParams,0,sizeof(MspParserInit_t));
        MsppInitParams.Memory_Partition       = CPUPartition;
        MsppInitParams.OpBufHandle            = qp_p->OpBufHandle;
        MsppInitParams.CLKRV_HandleForSynchro = qp_p->CLKRV_HandleForSynchro;
        MsppInitParams.AVMEMPartition         = qp_p->AVMEMPartition;
        MsppInitParams.EventIDNewFrame        = qp_p->EventIDNewFrame;
        MsppInitParams.EventIDFrequencyChange = qp_p->EventIDFrequencyChange;
        MsppInitParams.EvtHandle              = qp_p->EvtHandle;
        MsppInitParams.ObjectID               = qp_p->ObjectID;
        MsppInitParams.PESESBytesConsumedcallback = (void *)PESES_BytesConsumed;
        MsppInitParams.PESESHandle                = (U32)qp_p;
        error = Mspp_Parser_Init(&MsppInitParams, (MspParser_Handle_t*)&qp_p->Mspp_Cntrl_p);
        if(error != ST_NO_ERROR)
        {
            /*Not able to get channel*/
            error |= PESES_ReleaseMemory(qp_p);
            return error;
        }
    }

#else
    {
        ComParserInit_t ComInitParams;
        memset(&ComInitParams,0,sizeof(ComParserInit_t));
        ComInitParams.Memory_Partition        = CPUPartition;
        ComInitParams.AVMEMPartition          = InitParams_p->AVMEMPartition;
        ComInitParams.AudioBufferPartition    = InitParams_p->AudioBufferPartition;
        ComInitParams.OpBufHandle             = qp_p->OpBufHandle;
#ifndef STAUD_REMOVE_CLKRV_SUPPORT
        ComInitParams.CLKRV_HandleForSynchro  = qp_p->CLKRV_HandleForSynchro;
#endif

        /*Initialize the event parameters*/
        ComInitParams.EvtHandle               = qp_p->EvtHandle;
        ComInitParams.ObjectID                = qp_p->ObjectID;
        ComInitParams.EventIDFrequencyChange  = qp_p->EventIDFrequencyChange;
        ComInitParams.EventIDNewFrame         = qp_p->EventIDNewFrame;
        ComInitParams.EventIDNewRouting       = qp_p->EventIDNewRouting;
#if defined (STAUD_INSTALL_WAVE) || defined(STAUD_INSTALL_AIFF)
        ComInitParams.DummyBufferBaseAddress  = qp_p->DummyBufferBaseAddress_p;
#endif
        /* transfer fdma node pointer*/
#if defined (PES_TO_ES_BY_FDMA) || defined (STAUD_INSTALL_WAVE) || defined(STAUD_INSTALL_AIFF)
        ComInitParams.NodePESToES_p           = qp_p->NodePESToES_p;
#endif

#if defined (SPDIF_BYTE_REVERSE) || defined (SPDIF_BYTE_REVERSE_ASM)
        ComInitParams.OpBufHandle_SPDIF = ;
#endif
        ComInitParams.MixerEnabled = InitParams_p->MixerEnabled;
        error = Com_Parser_Init(&ComInitParams, (ComParser_Handle_t*)&qp_p->ComParser_p);
        if(error != ST_NO_ERROR)
        {
            /*Not able to get channel*/
            error |= PESES_ReleaseMemory(qp_p);
            return error;
        }

    }
#endif
    /*Launch the PES ES parser task*/

#if defined (ST_OS20)

    STOS_TaskCreate(PESES_Parse_Entry,(void *)qp_p, qp_p->CPUPartition_p,PES_ES_PARSER_TASK_STACK_SIZE,
                (void **)&qp_p->PESESTaskstack,qp_p->CPUPartition_p,&qp_p->PESESTask,
                &qp_p->PESESTaskDesc,PES_ES_PARSER_TASK_PRIORITY,Name,0);

#else

    STOS_TaskCreate(PESES_Parse_Entry,(void *)qp_p, NULL, PES_ES_PARSER_TASK_STACK_SIZE,
                NULL, NULL, &qp_p->PESESTask, NULL, PES_ES_PARSER_TASK_PRIORITY, Name, 0);

#endif

    if(qp_p->PESESTask == NULL)
    {
        error |= PESES_ReleaseMemory(qp_p);
        return ST_ERROR_NO_MEMORY;
    }

    /* Everything is going well, so we can now
     * finally append this item to the Mem Pool queue.
     */
    qp_p->Handle = (STPESES_Handle_t)qp_p;
    *Handle = qp_p->Handle;
    PESES_QueueAdd(qp_p);

    /* Now release the mutex */
    STOS_MutexRelease(PESESInitMutex_p);

    return error;
} /*PESES_Init()*/

/*****************************************************************************
PESES_Term()
Description:
    Initializes the named Mem Pool.

Parameters:
     Handle, handle to the device
Return Value:
    ST_NO_ERROR,                    no errors.
    ST_ERROR_BAD_PARAMETER,         one or more of the parameters was invalid.
    ST_ERROR_UNKNOWN_DEVICE,        invalid device name .
See Also:
    PESES_Init()
*****************************************************************************/
ST_ErrorCode_t PESES_Term(STPESES_Handle_t Handle)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    PESES_ParserControl_t *ControlBlock_p;

    if (PESESInitMutex_p != NULL)
    {
        STOS_MutexLock(PESESInitMutex_p);
    }
    else
    {
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    /* Obtain the Mem Pool control block from the passed in handle */
    ControlBlock_p = PESES_GetControlBlockFromHandle(Handle);

    if(ControlBlock_p)
    {
        PESES_QueueRemove(ControlBlock_p);
        error = PESES_ReleaseMemory(ControlBlock_p);
    }
    else
    {
        error = ST_ERROR_INVALID_HANDLE;
        STOS_MutexRelease(PESESInitMutex_p);
    }

    return error;
} /*PESES_Term()*/

ST_ErrorCode_t PESES_BytesConsumed(U32 ControlBlockhandle, U32 BytesConsumed)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    PESES_ParserControl_t *ControlBlock_p = (PESES_ParserControl_t *)ControlBlockhandle;

    if(ControlBlock_p->IpBufHandle)
    {
        /*Free up the buffers*/
        U32 IPQueueIndex = ControlBlock_p->IPQueueFront & PES_ES_NUM_IP_MASK;

        PESESInBlocklist_t  *IPBlkFrmProducer_p = &(ControlBlock_p->IPBlkFrmProducer[IPQueueIndex]);
        BufferState_t           BufferSatus = IPBlkFrmProducer_p->BufferSatus;
        STTBX_Print(("PEES(%s)check releasing block IPQueueIndex=%d,IPQueueFront=%d\n",ControlBlock_p->Name,IPQueueIndex,ControlBlock_p->IPQueueFront));
        if((BufferSatus == BUFFER_FIILED)||(BufferSatus == BUFFER_SENT))
        {
            U32 BytesFreed = BytesConsumed + IPBlkFrmProducer_p->BytesFreed;
            STTBX_Print(("PEES(%s)BytesFreed=%d,IPQueueFront=%d\n",ControlBlock_p->Name,BytesFreed,ControlBlock_p->IPQueueFront));
            if(BytesFreed < IPBlkFrmProducer_p->InputBlock_p->Size)
            {
                /*partial buffer is freed*/
                /*update the BytesFreed*/
                IPBlkFrmProducer_p->BytesFreed = BytesFreed;
            }
            else
            {
                /*full buffer can be freed*/
                /*mark it as free!*/
                if (MemPool_FreeIpBlk(ControlBlock_p->IpBufHandle, (U32 *)(&IPBlkFrmProducer_p->InputBlock_p), (U32)ControlBlock_p) != ST_NO_ERROR)
                {
                    STTBX_Print(("PEES(%s)Err in releasing memory block\n",ControlBlock_p->Name));
                }
                else
                {
                    STTBX_Print(("PEES(%s)rel mem blk %u\n",ControlBlock_p->Name,ControlBlock_p->IPQueueFront));
                }

                IPBlkFrmProducer_p->BufferSatus = BUFFER_FREE;
                IPBlkFrmProducer_p->BytesFreed = 0;
                ControlBlock_p->IPQueueFront++;
            }
        }
    }
    else
    {
        //STTBX_Print(("PESES:Bytes con,%u,Read,%u\n",BytesConsumed,ControlBlock_p->PTIReadPointer_p));

        ControlBlock_p->PTIReadPointer_p += BytesConsumed;

        if (ControlBlock_p->PTIReadPointer_p >= ControlBlock_p->PTITopPointer_p)
        {
            ControlBlock_p->PTIReadPointer_p = ControlBlock_p->PTIBasePointer_p;
        }

        if(ControlBlock_p->PTIDMAReadWritePointer.ReadAddress_p)
        {
            ControlBlock_p->PTIDMAReadWritePointer.ReadAddress_p(ControlBlock_p->PTIDMAReadWritePointer.Handle,ControlBlock_p->PTIReadPointer_p);
        }
    }
    return error;
}

/*****************************************************************************
PESES_SetClkSynchro()
Description:
    Change the clock handle for audio synchronisation.

Parameters:
     Handle, handle to the device
     ClkSource, new clk handle to be used.
Return Value:
    ST_NO_ERROR, no errors.
See Also:
    None
*****************************************************************************/
#ifndef STAUD_REMOVE_CLKRV_SUPPORT
ST_ErrorCode_t PESES_SetClkSynchro(STPESES_Handle_t Handle,STCLKRV_Handle_t Clksource)
{
    PESES_ParserControl_t *ControlBlock_p;

    if (PESESInitMutex_p != NULL)
    {
        STOS_MutexLock(PESESInitMutex_p);
    }
    else
    {
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    /* Obtain the control block from the passed in handle */
    ControlBlock_p = PESES_GetControlBlockFromHandle(Handle);

    STOS_MutexRelease(PESESInitMutex_p);

    if(ControlBlock_p)
    {
        ControlBlock_p->CLKRV_HandleForSynchro = Clksource;

#ifdef MSPP_PARSER
        Mspp_Parser_SetClkSynchro((MspParser_Handle_t)ControlBlock_p->Mspp_Cntrl_p,Clksource);
#else
        /*Send to com parser*/
        Com_Parser_SetClkSynchro((ComParser_Handle_t)ControlBlock_p->ComParser_p,Clksource);
#endif
    }
    else
    {
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    return(ST_NO_ERROR);
}
#endif //STAUD_REMOVE_CLKRV_SUPPORT
/*****************************************************************************
PESES_AVSyncCmd()
Description:
    Sends an AVSync command.

Parameters:
     Handle, handle to the device
     command, command.
Return Value:
    ST_NO_ERROR,                    no errors.
    ST_ERROR_BAD_PARAMETER,         one or more of the parameters was invalid.
    ST_ERROR_FEATURE_NOT_SUPPORTED, the device type is not supported.
    ST_ERROR_UNKNOWN_DEVICE,        invalid device name .
    ST_ERROR_NO_FREE_HANDLES,       another user has already opened the Pool,
                                    or it is in use.
See Also:
    PESES_Init()
*****************************************************************************/

ST_ErrorCode_t PESES_AVSyncCmd(STPESES_Handle_t Handle,BOOL AVSyncEnable)
{
   ST_ErrorCode_t Error = ST_NO_ERROR;
    PESES_ParserControl_t *ControlBlock_p;

    if (PESESInitMutex_p != NULL)
    {
        STOS_MutexLock(PESESInitMutex_p);
    }
    else
    {
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    /* Obtain the control block from the passed in handle */
    ControlBlock_p = PESES_GetControlBlockFromHandle(Handle);

    STOS_MutexRelease(PESESInitMutex_p);

    if(ControlBlock_p)
    {
        STOS_MutexLock(ControlBlock_p->Lock_p);
        ControlBlock_p->AVSyncEnable = AVSyncEnable;
#ifdef MSPP_PARSER
#else
        Error = Com_Parser_AVSyncCmd((ComParser_Handle_t)ControlBlock_p->ComParser_p, AVSyncEnable);
#endif
        STOS_MutexRelease(ControlBlock_p->Lock_p);

    }
    else
    {
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    return Error;
}

ST_ErrorCode_t PESESSetAudioType(STPESES_Handle_t Handle,STAUD_StreamParams_t * StreamParams_p)
{
   ST_ErrorCode_t Error=ST_NO_ERROR;
    PESES_ParserControl_t *ControlBlock_p;

    if (PESESInitMutex_p != NULL)
    {
        STOS_MutexLock(PESESInitMutex_p);
    }
    else
    {
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    /* Obtain the Mem Pool control block from the passed in handle */
    ControlBlock_p = PESES_GetControlBlockFromHandle(Handle);

    STOS_MutexRelease(PESESInitMutex_p);

    if(ControlBlock_p)
    {
        ParserState_t CurrentState;
        STOS_MutexLock(ControlBlock_p->PESESCurrentState.Lock);
        CurrentState = ControlBlock_p->PESESCurrentState.State;
        STOS_MutexRelease(ControlBlock_p->PESESCurrentState.Lock);

        if((CurrentState != TIMER_INIT)&&(CurrentState != TIMER_STOP))
        {
            return STAUD_ERROR_INVALID_STATE;
        }

        STOS_MutexLock(ControlBlock_p->Lock_p);

        switch(StreamParams_p->StreamContent)
        {
#ifndef MSPP_PARSER
            case STAUD_STREAM_CONTENT_HE_AAC:
#ifdef  STAUD_INSTALL_HEAAC
                ControlBlock_p->Parse_Frame_fp = (Parse_FrameFunc_t)Parse_HEAAC;
                ControlBlock_p->Parse_StopFunc_fp = (Parse_StopFunc_t)HEAAC_Stop;
                ControlBlock_p->Parse_StartFunc_fp = (Parse_StartFunc_t)HEAAC_Start;
                ControlBlock_p->StreamInfo.StreamContent=StreamParams_p->StreamContent;
                HEAAC_Init(ControlBlock_p);
#endif //#ifdef STAUD_INSTALL_HEAAC

                break;
#endif

            default:
#ifdef MSPP_PARSER
                    Mspp_Parser_SetStreamType((MspParser_Handle_t)ControlBlock_p->Mspp_Cntrl_p,StreamParams_p->StreamType,StreamParams_p->StreamContent);
                    ControlBlock_p->Parse_Frame_fp = (Parse_FrameFunc_t)Mspp_Parse_Frame;
                    ControlBlock_p->Parse_StopFunc_fp = (Parse_StopFunc_t)Mspp_Parser_Stop;
                    ControlBlock_p->Parse_StartFunc_fp = (Parse_StartFunc_t)Mspp_Parser_Start;
#else

                ControlBlock_p->StreamInfo.StreamContent=StreamParams_p->StreamContent; // Moved Here For ALSA
#ifdef STAUDLX_ALSA_SUPPORT
            if (StreamParams_p->StreamContent == STAUD_STREAM_CONTENT_ALSA)
                    StreamParams_p->StreamContent = STAUD_STREAM_CONTENT_PCM;



#endif
                    Com_Parser_SetStreamType((ComParser_Handle_t)ControlBlock_p->ComParser_p,StreamParams_p->StreamType,StreamParams_p->StreamContent);
                    ControlBlock_p->Parse_Frame_fp = (Parse_FrameFunc_t)Com_Parse_Frame;
                    ControlBlock_p->Parse_StopFunc_fp = (Parse_StopFunc_t)Com_Parser_Stop;
                    ControlBlock_p->Parse_StartFunc_fp = (Parse_StartFunc_t)Com_Parser_Start;
#endif

                break;
        }

        ControlBlock_p->StreamType=StreamParams_p->StreamType;


        STOS_MutexRelease(ControlBlock_p->Lock_p);
    }
    else
    {
        return ST_ERROR_UNKNOWN_DEVICE;
    }



    return Error;
}


/* ------------------------------------------------------------------------------
 * function :       PES_ES_Parse_Entry
 * parameters:      Parameter pointer
 *
 * returned value:  none
 * writes the data : value in register located at address: address
   -----------------------------------------------------------------------------*/

void PESES_Parse_Entry(void *Params_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    /* Get the PES ES control Block for this task */
    PESES_ParserControl_t *ControlBlock_p = (PESES_ParserControl_t *)Params_p;

    U32 IPQueueIndex = 0;
    PESESInBlocklist_t  *IPBlkFrmProducer_p;
    U32 BitBufferLevel=0;
    signed long delay=0;

    STOS_Clock_t Timeout;
#if POLLING_DURATION_FIXED
    STOS_Clock_t Timeup;
#endif
    void * Timeout_p = TIMEOUT_INFINITY;

#if USE_TRACE_BUFFER_FOR_DEBUG
    static FILE *file_p1 = NULL;
#endif

    STOS_TaskEnter(Params_p);

#if USE_TRACE_BUFFER_FOR_DEBUG
            if (file_p1 == NULL)
            {
                file_p1 = fopen("d:\\stc_drift.csv","w+");
                    if (file_p1 == NULL)
                        STTBX_Print(("Error in opening file \n"));
            }
            else
            {
                /*fclose(file_p);*/
            }
#endif

    while(1)
    {
        BOOL LOOP=TRUE;
        //delay=10; //Just One instruction before locking the semaphore

        STOS_SemaphoreWaitTimeOut(ControlBlock_p->SemWait_p, (STOS_Clock_t *)Timeout_p);
#if POLLING_DURATION_FIXED
        Timeup = STOS_time_now();
#endif

        switch(ControlBlock_p->PESESCurrentState.State)
        {
            case TIMER_INIT:

                STOS_MutexLock(ControlBlock_p->PESESNextState.Lock);
                if (ControlBlock_p->PESESNextState.State != ControlBlock_p->PESESCurrentState.State)
                {
                    Timeout_p = TIMEOUT_IMMEDIATE;
                    ControlBlock_p->PESESCurrentState.State = ControlBlock_p->PESESNextState.State;
                    STTBX_Print(("Changing Timer state from TIMER_INIT\n"));
                    //STOS_SemaphoreSignal(ControlBlock_p->SemWait_p);
                    STOS_SemaphoreSignal(ControlBlock_p->SemComplete_p);
                }
                STOS_MutexRelease(ControlBlock_p->PESESNextState.Lock);

                break;
            case TIMER_TRANSFER_START:
                while(LOOP)
                {
                    switch(ControlBlock_p->PESESInputState)
                    {
                        case PARSER_STATE_IDLE:
                            if(ControlBlock_p->IpBufHandle)
                            {
                                ControlBlock_p->PESESInputState = PARSER_STATE_GET_INPUT_BUFFER;
                                /*fall through*/
                            }
                            else
                            {
                                ControlBlock_p->PESESInputState = PARSER_STATE_POLL_INPUT_BUFFER;
                                //STOS_SemaphoreSignal(ControlBlock_p->SemWait_p);
                                break;
                            }

                        case PARSER_STATE_GET_INPUT_BUFFER:
                            /*get input buffer*/
                            while(((ControlBlock_p->IPQueueTail - ControlBlock_p->IPQueueFront) < PES_ES_NUM_IP_BUFFERS) &&
                                (MemPool_GetIpBlk(ControlBlock_p->IpBufHandle, (U32 *)&ControlBlock_p->CurrentIPBlock_p,(U32)ControlBlock_p) == ST_NO_ERROR))
                            {
                                IPQueueIndex = ControlBlock_p->IPQueueTail & PES_ES_NUM_IP_MASK;
                                STTBX_Print(("PEES(%s)IPQueueTail Index=%u,IPQueueSent=%d\n",ControlBlock_p->Name,IPQueueIndex, ControlBlock_p->IPQueueTail));
                                IPBlkFrmProducer_p = &(ControlBlock_p->IPBlkFrmProducer[IPQueueIndex]);
                                /*We have space to store the I/P buffer and also have an I/P buffer*/
                                IPBlkFrmProducer_p->InputBlock_p = ControlBlock_p->CurrentIPBlock_p;
                                IPBlkFrmProducer_p->BytesSent = 0;
                                IPBlkFrmProducer_p->BytesFreed = 0;
                                IPBlkFrmProducer_p->BufferSatus = BUFFER_FIILED;
                                STTBX_Print(("PEES(%s)get mem blk %u\n",ControlBlock_p->Name,ControlBlock_p->IPQueueTail));
                                ControlBlock_p->IPQueueTail++;

                            }
                            /*Get the first input block*/
                            IPQueueIndex = ControlBlock_p->IPQueueSent & PES_ES_NUM_IP_MASK;
                            STTBX_Print(("PEES(%s)IPQueueSent Index=%u,IPQueueSent=%d\n",ControlBlock_p->Name,IPQueueIndex, ControlBlock_p->IPQueueSent));
                            IPBlkFrmProducer_p = &(ControlBlock_p->IPBlkFrmProducer[IPQueueIndex]);
                            /*check for validity*/
                            if(IPBlkFrmProducer_p->BufferSatus == BUFFER_FIILED)
                            {
                                /*Block is valid so send this for parsing*/
                                U32 i;
                                ControlBlock_p->PESBlock.StartPointer = IPBlkFrmProducer_p->InputBlock_p->MemoryStart + IPBlkFrmProducer_p->BytesSent;
                                ControlBlock_p->BytesUsed                 = 0;
                                ControlBlock_p->PESBlock.ValidSize    = IPBlkFrmProducer_p->InputBlock_p->Size - IPBlkFrmProducer_p->BytesSent;

                                STTBX_Print(("PEES(%s)for parsing %u,Size=%d,used=%d\n",ControlBlock_p->Name,ControlBlock_p->IPQueueSent, ControlBlock_p->PESBlock.ValidSize,ControlBlock_p->BytesUsed));

                                BitBufferLevel = ControlBlock_p->PESBlock.ValidSize - ControlBlock_p->BytesUsed;
                                for(i = (IPQueueIndex + 1); i < (IPQueueIndex + PES_ES_NUM_IP_BUFFERS); i++)
                                {
                                    IPBlkFrmProducer_p = &(ControlBlock_p->IPBlkFrmProducer[i & PES_ES_NUM_IP_MASK]);
                                    if(IPBlkFrmProducer_p->BufferSatus == BUFFER_FIILED)
                                    {
                                        BitBufferLevel += IPBlkFrmProducer_p->InputBlock_p->Size;
                                    }
                                    else
                                    {
                                        break;
                                    }
                                }

                                ControlBlock_p->PESESInputState = PARSER_STATE_PARSE_INPUT;
                            }
                            else
                            {
                                /*There is no available blocks*/
                                /*wait for trigger from input BM*/
                                ControlBlock_p->PESBlock.ValidSize = 0;
                                STTBX_Print(("PEES(%s)NoData to Parse\n",ControlBlock_p->Name));
                                if(ControlBlock_p->EnableMSPPSupport)
                                {
                                    ControlBlock_p->PESESInputState = PARSER_STATE_PARSE_INPUT;
                                }
                                else
                                {
                                    delay = -1;
                                    LOOP = FALSE;
                                    Error = STAUD_MEMORY_GET_ERROR;
                                }
                            }

                            break;
                            /*fall through for now*/
                        case PARSER_STATE_POLL_INPUT_BUFFER:
                            /*Get the ControlBlock_p->CurrentPTIWritePointer_p*/
                            if(ControlBlock_p->PTIDMAReadWritePointer.WriteAddress_p && ControlBlock_p->PTIDMAReadWritePointer.ReadAddress_p)
                            {
                                ControlBlock_p->PreviousPTIWritePointer_p = ControlBlock_p->CurrentPTIWritePointer_p;

                                if(ControlBlock_p->PTIDMAReadWritePointer.WriteAddress_p(ControlBlock_p->PTIDMAReadWritePointer.Handle,(void **)&ControlBlock_p->CurrentPTIWritePointer_p) != ST_NO_ERROR)
                                {
                                    ControlBlock_p->CurrentPTIWritePointer_p = ControlBlock_p->PreviousPTIWritePointer_p;
                                }

                                ControlBlock_p->PointerDifference = ControlBlock_p->CurrentPTIWritePointer_p - ControlBlock_p->PreviousPTIReadPointer_p;

                                if(ControlBlock_p->PointerDifference < 0)
                                {
                                    /*read below write so write has wrapped around*/
                                    /*pick only upto the end and the rest when we have completed this pick up*/
                                    BitBufferLevel = ControlBlock_p->PTITopPointer_p - ControlBlock_p->PreviousPTIReadPointer_p;
                                    /*calculate the actual amount of data in the PES/ES buffer*/
                                    ControlBlock_p->PointerDifference = ControlBlock_p->BitBufferSize + ControlBlock_p->PointerDifference;

                                }
                                else
                                {
                                    /*write below read so delta is write - read */
                                    /*pick the whole chunk*/
                                    BitBufferLevel = ControlBlock_p->PointerDifference;
                                }
                                /*read pointer points from where the driver will read*/
                                /*write pointer points from where the appli will write*/
                                /* check level in the PES buffer by checking read and write pointers only */
                                {
                                    S32 BitBufferReadWriteLevel;

                                    BitBufferReadWriteLevel=ControlBlock_p->CurrentPTIWritePointer_p-ControlBlock_p->PreviousPTIReadPointer_p;

                                    if(BitBufferReadWriteLevel<0)
                                    {
                                        BitBufferReadWriteLevel=ControlBlock_p->BitBufferSize+BitBufferReadWriteLevel;
                                    }

                                    if((U32)BitBufferReadWriteLevel<((ControlBlock_p->BitBufferSize*ControlBlock_p->LowDataLevel)/100))
                                    {
                                        // Notify an event for Low Data Level
                                        if (ControlBlock_p->LowLevelDataEvent == FALSE)
                                        {
                                            Error=STAudEVT_Notify(ControlBlock_p->EvtHandle, ControlBlock_p->EventIDLowDataLevel, NULL, 0, ControlBlock_p->ObjectID);
                                            ControlBlock_p->LowLevelDataEvent = TRUE;
                                        }
                                    }
                                    else
                                    {
                                        ControlBlock_p->LowLevelDataEvent = FALSE;
                                    }
                                }

                                ControlBlock_p->PESBlock.StartPointer = (U32)ControlBlock_p->PreviousPTIReadPointer_p;

#ifdef STAUDLX_ALSA_SUPPORT
                                if(!ControlBlock_p->EnableMSPPSupport)
                                {
                                    if(ControlBlock_p->StreamInfo.StreamContent == STAUD_STREAM_CONTENT_ALSA)
                                    {
                                        if(BitBufferLevel>2048)
                                            BitBufferLevel = 2048;
                                    }
                                }
#endif
                                // to ensure that not too much data is parsed at one go.
                                ControlBlock_p->PESBlock.ValidSize  = (BitBufferLevel>2048)?2048:BitBufferLevel;
                                ControlBlock_p->BytesUsed               = 0;

                                if(BitBufferLevel)
                                {
                                    ControlBlock_p->PESFilled++;
                                }
                            }   //if(ControlBlock_p->PTIDMAReadWritePointer.WriteAddress_p && ControlBlock_p->PTIDMAReadWritePointer.ReadAddress_p)

                            ControlBlock_p->PESESInputState = PARSER_STATE_CHECK_UNDERFLOW;
                         case PARSER_STATE_CHECK_UNDERFLOW:

                            if(BitBufferLevel < ControlBlock_p->BitBufferThreshold)
                            {
                                ControlBlock_p->PTIBufferUnderflowCount++;
                                if(ControlBlock_p->PTIBufferUnderflowCount > 1000)
                                {
                                    STTBX_Print(("PESES(%s):BitBufferLevel:%d\n",ControlBlock_p->Name,BitBufferLevel));
                                    Error=STAudEVT_Notify(ControlBlock_p->EvtHandle, ControlBlock_p->EventIDDataUnderFlow, NULL, 0, ControlBlock_p->ObjectID);
                                    ControlBlock_p->PTIBufferUnderflowCount = 0;
                                }

                            }
                            else
                            {
                                ControlBlock_p->PTIBufferUnderflowCount = 0;
                            }

                            ControlBlock_p->PESESInputState = PARSER_STATE_PARSE_INPUT;

                        case PARSER_STATE_PARSE_INPUT:

                            Error = ControlBlock_p->Parse_Frame_fp((STPESES_Handle_t)ControlBlock_p);
                            //STTBX_Print(("PESES:Error,%u, size %d\n",Error,ControlBlock_p->PESBlock.ValidSize));
                            if(ControlBlock_p->IpBufHandle)
                            {
                                ControlBlock_p->PESESInputState = PARSER_STATE_FREE_INPUT_BUFFER;
                            }
                            else
                            {
                                ControlBlock_p->PESESInputState = PARSER_STATE_UPDATE_READ_PONTER;
                                break;
                            }

                        case PARSER_STATE_FREE_INPUT_BUFFER:
                            /*free the input buffer. take care of MSPP*/
                            /*If sucessful go to get the next input buffer*/
                            if(ControlBlock_p->PESBlock.ValidSize)
                            {
                                IPQueueIndex = ControlBlock_p->IPQueueSent & PES_ES_NUM_IP_MASK;
                                IPBlkFrmProducer_p = &(ControlBlock_p->IPBlkFrmProducer[IPQueueIndex]);
                                if(IPBlkFrmProducer_p->BufferSatus == BUFFER_FIILED)
                                {
                                    if((ControlBlock_p->BytesUsed + IPBlkFrmProducer_p->BytesSent) < IPBlkFrmProducer_p->InputBlock_p->Size)
                                    {
                                        /*partial buffer is consumed*/
                                        /*update the BytesConsumed*/
                                        IPBlkFrmProducer_p->BytesSent += ControlBlock_p->BytesUsed;
                                        STTBX_Print(("PEES(%s)parse over blk=%u,used=%d\n",ControlBlock_p->Name,ControlBlock_p->IPQueueSent, ControlBlock_p->BytesUsed));
                                    }
                                    else
                                    {
                                        /*full buffer is consumed*/
                                        /*mark it sent!*/

                                        IPBlkFrmProducer_p->BufferSatus = BUFFER_SENT;
                                        IPBlkFrmProducer_p->BytesSent = 0;
                                        STTBX_Print(("PEES(%s)sent com blk %u\n",ControlBlock_p->Name,ControlBlock_p->IPQueueSent));
                                        ControlBlock_p->IPQueueSent++;
                                    }
                                }

                                /*check if we can free the buffer here only for non MSPP*/
                                if(!ControlBlock_p->EnableMSPPSupport)
                                {
                                    PESES_BytesConsumed((U32)ControlBlock_p, ControlBlock_p->BytesUsed);
                                }
                            }

                            if(Error == ST_NO_ERROR)
                            {
                                delay = 4;//In ms
                            }
                            else
                            {
                                /*No Output buffer*/
                                /*will be signaled by the BM */
                                delay = -1;
                            }
                            LOOP = FALSE;
                            ControlBlock_p->PESESInputState = PARSER_STATE_IDLE;
                            break;
                        case PARSER_STATE_UPDATE_READ_PONTER:
                            if(ControlBlock_p->PESBlock.ValidSize)
                            {

#ifdef STAUDLX_ALSA_SUPPORT
                                if(!ControlBlock_p->EnableMSPPSupport)
                                {
                                    // 10 K Data, Required to Start the PCM Player is ~9K
                                    // Manipulated reporting of pointer Position
                                    if(ControlBlock_p->sentForParsing <5 && ControlBlock_p->StreamInfo.StreamContent == STAUD_STREAM_CONTENT_ALSA)
                                    {
                                        AUD_TaskDelayMs(3);
                                    }
                                }
#endif

                                //STTBX_Print(("PESES:Bytes Used,%u,Read,%u\n",ControlBlock_p->BytesUsed,ControlBlock_p->PreviousPTIReadPointer_p));
                                if(ControlBlock_p->BytesUsed <= ControlBlock_p->PESBlock.ValidSize)
                                {
                                    U32 BytesConsumed = ControlBlock_p->BytesUsed;
                                    ControlBlock_p->PreviousPTIReadPointer_p += BytesConsumed;
                                    ControlBlock_p->PESBlock.StartPointer += BytesConsumed;
                                    ControlBlock_p->sentForParsing++;
                                    BitBufferLevel -= BytesConsumed;

                                    if (ControlBlock_p->PreviousPTIReadPointer_p >= ControlBlock_p->PTITopPointer_p)
                                    {
                                        ControlBlock_p->PreviousPTIReadPointer_p = ControlBlock_p->PTIBasePointer_p;
                                    }

                                    if(!ControlBlock_p->EnableMSPPSupport)
                                    {
                                        PESES_BytesConsumed((U32)ControlBlock_p, BytesConsumed);
                                    }
                                }

                            }
                            else
                            {
                                /*We may have gone into underflow
                                check if we have to force the last partial frame into the next unit*/
                                BOOL testEOF;
                                STOS_MutexLock(ControlBlock_p->Lock_p);
                                testEOF = ControlBlock_p->EOFFlag;
                                STOS_MutexRelease(ControlBlock_p->Lock_p);

                                /*Send EOF only if we have already sent some data and EOF flag has been set*/
                                if(testEOF && ControlBlock_p->PESFilled)
                                {
                                    STTBX_Print(("Received EOF\n"));
#ifdef MSPP_PARSER
                                    Error = Mspp_Parser_HandleEOF((MspParser_Handle_t)ControlBlock_p->Mspp_Cntrl_p);
#else
                                    /*We have received a EOF and so we push all the remaining data*/
                                    Error = Com_Parser_HandleEOF((ComParser_Handle_t)ControlBlock_p->ComParser_p);
#endif
                                    if(Error == ST_NO_ERROR)
                                    {
                                        STTBX_Print(("PESES:EOF Error:%d\n",Error));
                                        STOS_MutexLock(ControlBlock_p->Lock_p);
                                        ControlBlock_p->EOFFlag = FALSE;
                                        STOS_MutexRelease(ControlBlock_p->Lock_p);
                                    }
                                    else
                                    {
                                        /*try to send EOF Again*/
                                        STTBX_Print(("PESES:EOF sent\n"));
                                        Error = ST_NO_ERROR; /*for some delay*/
                                    }

                                }
                            }
                            if(Error == ST_NO_ERROR)
                            {
                                U32 Frameduration = 4; // in ms
#if !defined (ST_7200)&&!defined (ST_OS20)
#if !defined( MSPP_PARSER)  && !defined(STAUDLX_ALSA_SUPPORT)
                                if(ControlBlock_p->Speed == 100)
                                {
                                    Frameduration = Com_Parser_GetRemainTime((ComParser_Handle_t)ControlBlock_p->ComParser_p);
                                    STTBX_Print(("Frameduration:%d\n",Frameduration));
                                    Frameduration = (Frameduration < 4)?4:Frameduration;
                                }
#endif
#endif
                                if(ControlBlock_p->PESBlock.ValidSize)
                                {
                                    if((U32)ControlBlock_p->PointerDifference > ControlBlock_p->PESBlock.ValidSize)
                                    {
                                        delay = 0;
                                    }
                                    else
                                    {
                                        delay = Frameduration;
                                    }
                                }
                                else
                                {
                                    delay = 10;
                                }
                            }
                            else
                            {
                                /*No Output buffer*/
                                delay = -1;
                                /*need to signal for MSPP as we need to send input in the absence of output*/
                                if(ControlBlock_p->EnableMSPPSupport)
                                {
                                    delay = 4;//delay in ms(ControlBlock_p->NumberOfTicksPermSecond);
                                }
                            }
                            ControlBlock_p->PESESInputState = PARSER_STATE_IDLE;
                            LOOP=FALSE;
                        break;
                        default:
                        break;
                    }
                }
                //}//if(ControlBlock_p->PTIDMAReadWritePointer.WriteAddress_p && ControlBlock_p->PTIDMAReadWritePointer.ReadAddress_p)
                //STTBX_Print(("sentForParsing:%d\n",ControlBlock_p->sentForParsing));

                STOS_MutexLock(ControlBlock_p->PESESNextState.Lock);
                switch(ControlBlock_p->PESESNextState.State)
                {
                case TIMER_STOP:
                    /*reset Parser Params*/
                    PESES_ResetParams(ControlBlock_p,TRUE);
                    delay = -1;
                    if(ControlBlock_p->Parse_StopFunc_fp)
                    {
                        ControlBlock_p->Parse_StopFunc_fp((STPESES_Handle_t)ControlBlock_p);
                    }

                    ControlBlock_p->PESESCurrentState.State = ControlBlock_p->PESESNextState.State;
#ifdef STAUDLX_ALSA_SUPPORT
                    if (ControlBlock_p->StreamInfo.StreamContent==STAUD_STREAM_CONTENT_ALSA)
                        ;
                    else
                        STOS_SemaphoreSignal(ControlBlock_p->SemComplete_p);
#else
                    STOS_SemaphoreSignal(ControlBlock_p->SemComplete_p);
#endif

                break;
                case TIMER_TRANSFER_PAUSE:
                    delay = -1;
                    ControlBlock_p->PESESCurrentState.State = ControlBlock_p->PESESNextState.State;
                    STOS_SemaphoreSignal(ControlBlock_p->SemComplete_p);
                    break;
                default:
                    break;
                }
                STOS_MutexRelease(ControlBlock_p->PESESNextState.Lock);
                switch(delay)
                {
#if !(POLLING_DURATION_FIXED)
                    case -1:
                        STTBX_Print(("PEES(%s):Timeout sent\n",ControlBlock_p->Name));
                        Timeout_p = TIMEOUT_INFINITY;
                        break;
#endif
                    case 0:
                        Timeout_p = TIMEOUT_IMMEDIATE;
                        break;
                    default:
#if defined( ST_OSLINUX )
                        switch(ControlBlock_p->NumberOfTicksPerSecond)
                        {
                        case 100:
                            delay = (delay + 10)/10; /*taking the ceiling*/
                            /*delay in 10msec*/
                            break;
                        case 1000:
                            /*delay already in msec*/
                            break;
                        default:
                            delay =10;
                            break;
                        }
#else
                        STTBX_Print(("PEES(%s):delay sent,%d\n",ControlBlock_p->Name,delay));
                        delay = delay*ControlBlock_p->NumberOfTicksPermSecond;
#endif
#if POLLING_DURATION_FIXED
                        delay = 19*ControlBlock_p->NumberOfTicksPermSecond;
                        Timeout = STOS_time_plus(Timeup,(STOS_Clock_t)delay);
                        if(STOS_time_after(Timeout, STOS_time_now()))
                        {
                            STTBX_Print(("PEES(%s):delay sent,%d\n",ControlBlock_p->Name,20));
                            delay = 20*ControlBlock_p->NumberOfTicksPermSecond;
                            Timeout = STOS_time_plus(Timeup,(STOS_Clock_t)delay);
                            Timeout_p = &Timeout;
                        }
                        else
                        {
                            Timeout_p = TIMEOUT_IMMEDIATE;
                        }
#else
                        Timeout = STOS_time_plus(STOS_time_now(),(STOS_Clock_t)delay);
                        Timeout_p = &Timeout;
#endif
                        break;
                }

                break;
            case TIMER_TRANSFER_PAUSE:
                Timeout_p = TIMEOUT_INFINITY;
                STOS_MutexLock(ControlBlock_p->PESESNextState.Lock);
                switch(ControlBlock_p->PESESNextState.State)
                {
                case TIMER_TRANSFER_START:
                    ControlBlock_p->PESESCurrentState.State = ControlBlock_p->PESESNextState.State;
                    STOS_SemaphoreSignal(ControlBlock_p->SemWait_p);
                    STOS_SemaphoreSignal(ControlBlock_p->SemComplete_p);
                    break;
                case TIMER_STOP:
                    /*reset Parser Params*/
                    PESES_ResetParams(ControlBlock_p,TRUE);

                    if(ControlBlock_p->Parse_StopFunc_fp)
                    {
                        ControlBlock_p->Parse_StopFunc_fp((STPESES_Handle_t)ControlBlock_p);
                    }

                    ControlBlock_p->PESESCurrentState.State = ControlBlock_p->PESESNextState.State;
                    STOS_SemaphoreSignal(ControlBlock_p->SemWait_p);
#ifdef STAUDLX_ALSA_SUPPORT
                    if (ControlBlock_p->StreamInfo.StreamContent==STAUD_STREAM_CONTENT_ALSA)
                        ;
                    else
                        STOS_SemaphoreSignal(ControlBlock_p->SemComplete_p);
#else
                    STOS_SemaphoreSignal(ControlBlock_p->SemComplete_p);
#endif
                    break;
                default:
                    break;
                }
                STOS_MutexRelease(ControlBlock_p->PESESNextState.Lock);
                break;
            case TIMER_STOP:
                Timeout_p = TIMEOUT_INFINITY;
                STOS_MutexLock(ControlBlock_p->PESESNextState.Lock);

                if(ControlBlock_p->PESESNextState.State != TIMER_STOP)
                {
                    /*reset Parser Params again before going to start*/
                    PESES_ResetParams(ControlBlock_p,FALSE);

                    ControlBlock_p->PESESCurrentState.State = ControlBlock_p->PESESNextState.State;
                    STTBX_Print(("Changing State PES_ES\n"));
                    STOS_SemaphoreSignal(ControlBlock_p->SemWait_p);
                    STOS_SemaphoreSignal(ControlBlock_p->SemComplete_p);
                }
                STOS_MutexRelease(ControlBlock_p->PESESNextState.Lock);

                break;
            case TIMER_TERMINATE:
#if 0
#ifdef MSPP_PARSER
            Error = Mspp_Parser_Term(ControlBlock_p);
#else
            Error = Com_Parser_Term(ControlBlock_p);
#endif
#endif
                Error |= PESES_Term((STPESES_Handle_t)ControlBlock_p);
                if(Error == ST_NO_ERROR)
                STOS_TaskExit(Params_p);
#if defined( ST_OSLINUX )
                {
                    return ;
                }
#else
                    task_exit(1);
#endif
                break;
            default:
                break;
        }
    }
}

/* ------------------------------------------------------------------------------
 * function :       PESES_ResetParams
 * parameters:      PESES_ParserControl_t
 *
 * returned value:  Void
 *
   -----------------------------------------------------------------------------*/

static void PESES_ResetParams(PESES_ParserControl_t *ControlBlock_p, BOOL x)
{

    if(ControlBlock_p->IpBufHandle)
    {
        U32 IPQueueIndex;
        PESESInBlocklist_t  *IPBlkFrmProducer_p;
        while(ControlBlock_p->IPQueueFront != ControlBlock_p->IPQueueTail)
        {
            IPQueueIndex = ControlBlock_p->IPQueueFront & PES_ES_NUM_IP_MASK;
            IPBlkFrmProducer_p = &(ControlBlock_p->IPBlkFrmProducer[IPQueueIndex]);
            if(IPBlkFrmProducer_p->BufferSatus != BUFFER_FREE )
            {
                /*free it!*/
                if (MemPool_FreeIpBlk(ControlBlock_p->IpBufHandle, (U32 *)(&IPBlkFrmProducer_p->InputBlock_p), (U32)ControlBlock_p) != ST_NO_ERROR)
                {
                    STTBX_Print(("PEES(%s)Err in releasing memory block\n",ControlBlock_p->Name));
                }
                else
                {
                    IPBlkFrmProducer_p->BufferSatus = BUFFER_FREE;
                    IPBlkFrmProducer_p->BytesFreed = 0;
                    IPBlkFrmProducer_p->BytesSent = 0;
                    ControlBlock_p->IPQueueFront++;
                }
            }
        }

        ControlBlock_p->IPQueueFront = 0;
        ControlBlock_p->IPQueueTail  = 0;
    }
    else
    {
        /*Set the read-write pointer to the base of the PES/ES buffer*/
        ControlBlock_p->PreviousPTIWritePointer_p = ControlBlock_p->PTIBasePointer_p;
        ControlBlock_p->CurrentPTIWritePointer_p = ControlBlock_p->PreviousPTIWritePointer_p;
        ControlBlock_p->PTIReadPointer_p = ControlBlock_p->PTIBasePointer_p;
        ControlBlock_p->PreviousPTIReadPointer_p = ControlBlock_p->PTIBasePointer_p;

        /*Clear the PES/ES Buffer*/
        if(!ControlBlock_p->EnableMSPPSupport)
        {
            memset((void *)ControlBlock_p->PTIBasePointer_p,0,ControlBlock_p->BitBufferSize);
        }

        if(x)
        {
            if(ControlBlock_p->PTIDMAReadWritePointer.ReadAddress_p)
            {
                ControlBlock_p->PTIDMAReadWritePointer.ReadAddress_p(ControlBlock_p->PTIDMAReadWritePointer.Handle,ControlBlock_p->PTIReadPointer_p);
            }
        }
    }

    ControlBlock_p->PESESInputState = PARSER_STATE_IDLE;

    ControlBlock_p->PESBlock.StartPointer       = 0;
    ControlBlock_p->PESBlock.ValidSize          = 0;
    ControlBlock_p->sentForParsing              = 0;
    ControlBlock_p->PESFilled                       = 0;
    ControlBlock_p->BytesUsed                       = 0;
    ControlBlock_p->PTIBufferUnderflowCount = 0;
    ControlBlock_p->LowLevelDataEvent           = FALSE;

    /*if(ControlBlock_p->Parse_StopFunc_fp)
    {
        ControlBlock_p->Parse_StopFunc_fp((STPESES_Handle_t)ControlBlock_p);
    }*/
}

/* ------------------------------------------------------------------------------
 * function :       PES_ES_ParseState
 * parameters:      handle
 *
 * returned value:  ParserState_t
 *
   -----------------------------------------------------------------------------*/
ParserState_t GetPESES_ParseState(STPESES_Handle_t Handle)
{
   ParserState_t CurrentState;
    PESES_ParserControl_t *ControlBlock_p;

    if (PESESInitMutex_p != NULL)
    {
        STOS_MutexLock(PESESInitMutex_p);
    }
    else
    {
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    /* Obtain the Mem Pool control block from the passed in handle */
    ControlBlock_p = PESES_GetControlBlockFromHandle(Handle);

    STOS_MutexRelease(PESESInitMutex_p);

    if(ControlBlock_p)
    {
        STOS_MutexLock(ControlBlock_p->PESESCurrentState.Lock);
        CurrentState = ControlBlock_p->PESESCurrentState.State;
        STOS_MutexRelease(ControlBlock_p->PESESCurrentState.Lock);
    }
    else
    {
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    return CurrentState;
}

/* ------------------------------------------------------------------------------
 * function :       SendPES_ES_ParseCmd
 * parameters:      void *
 *
 * returned value:  ST_ErrorCode_t
 *
   -----------------------------------------------------------------------------*/
ST_ErrorCode_t SendPES_ES_ParseCmd(STPESES_Handle_t Handle, ParserState_t State)
{
    PESES_ParserControl_t *ControlBlock_p;
    task_t  *PESESTask = NULL;
    ParserState_t CurrentState;

    if (PESESInitMutex_p != NULL)
    {
        STOS_MutexLock(PESESInitMutex_p);
    }
    else
    {
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    /* Obtain the Mem Pool control block from the passed in handle */
    ControlBlock_p = PESES_GetControlBlockFromHandle(Handle);

    STOS_MutexRelease(PESESInitMutex_p);
    if(ControlBlock_p)
    {
        STOS_MutexLock(ControlBlock_p->PESESCurrentState.Lock);
        CurrentState = ControlBlock_p->PESESCurrentState.State;
        STOS_MutexRelease(ControlBlock_p->PESESCurrentState.Lock);
        switch(State)
        {
            case TIMER_TRANSFER_START:
                if((CurrentState == TIMER_INIT)||(CurrentState == TIMER_STOP))
                {

                    /*Start from here*/
                    if(ControlBlock_p->Parse_StartFunc_fp)
                    {
                        ST_ErrorCode_t Error = ST_NO_ERROR;

                        Error = ControlBlock_p->Parse_StartFunc_fp((STPESES_Handle_t)ControlBlock_p);

                        if(Error != ST_NO_ERROR)
                        {
                            return Error;
                        }
                    }

                    {
                        ConsumerParams_t BMParams;
                        BMParams.Handle = (U32)ControlBlock_p;
#if POLLING_DURATION_FIXED
                        BMParams.sem = NULL;
#else
                        BMParams.sem = ControlBlock_p->SemWait_p;
#endif

                        /*Register to I/P memory block*/
                        if(ControlBlock_p->IpBufHandle)
                        {
                            if(MemPool_RegConsumer(ControlBlock_p->IpBufHandle, BMParams) != ST_NO_ERROR )
                            {
                                return ST_ERROR_INVALID_HANDLE;
                            }
                        }

                        /*Register to O/P memory block*/
                        if(MemPool_RegProducer(ControlBlock_p->OpBufHandle, BMParams) != ST_NO_ERROR )
                        {
                            return ST_ERROR_INVALID_HANDLE;
                        }
                    }


                }
                else
                {
                    return STAUD_ERROR_INVALID_STATE;
                }
                break;
            case TIMER_TRANSFER_PAUSE:
                if(CurrentState == TIMER_TRANSFER_START)
                {

                }
                else
                {
                    return STAUD_ERROR_INVALID_STATE;
                }
                break;
            case TIMER_TRANSFER_RESUME:
                if(CurrentState == TIMER_TRANSFER_PAUSE)
                {
                    State = TIMER_TRANSFER_START;
                }
                else
                {
                    return STAUD_ERROR_INVALID_STATE;
                }
                break;
            case TIMER_STOP:
                if((CurrentState == TIMER_TRANSFER_PAUSE)||
                    (CurrentState == TIMER_TRANSFER_START))
                {

                }
                else
                {
                    return STAUD_ERROR_INVALID_STATE;
                }
                break;
            case TIMER_TERMINATE:
                if((CurrentState == TIMER_INIT)||(CurrentState == TIMER_STOP))
                {
                    PESESTask = ControlBlock_p->PESESTask;
                }
                else
                {
                    return STAUD_ERROR_INVALID_STATE;
                }
                break;
            case TIMER_INIT:
            default:
                break;
        }

        //STTBX_Print(("TimerNextState=%d,%d\n",ControlBlock_p,ControlBlock_p->PESESNextState.State));
        STOS_MutexLock(ControlBlock_p->PESESNextState.Lock);
        ControlBlock_p->PESESNextState.State = State;
        STOS_MutexRelease(ControlBlock_p->PESESNextState.Lock);

        STOS_SemaphoreSignal(ControlBlock_p->SemWait_p);

        switch(State)
        {
            case TIMER_TRANSFER_START:
            case TIMER_TRANSFER_PAUSE:
            case TIMER_TRANSFER_RESUME:
                STOS_SemaphoreWait(ControlBlock_p->SemComplete_p);
                break;
            case TIMER_STOP:

#ifdef STAUDLX_ALSA_SUPPORT
                if(ControlBlock_p->StreamInfo.StreamContent==STAUD_STREAM_CONTENT_ALSA)
                    ;

                else
                    STOS_SemaphoreWait(ControlBlock_p->SemComplete_p);
#else

                STOS_SemaphoreWait(ControlBlock_p->SemComplete_p);
#endif


                {
                    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
                    /*Unregister from I/P BM*/
                    if(ControlBlock_p->IpBufHandle)
                    {
                        ErrorCode = MemPool_UnRegConsumer(ControlBlock_p->IpBufHandle, (U32)ControlBlock_p);
                        if(ErrorCode != ST_NO_ERROR)
                        {
                            STTBX_Print(("Unable to Unregister I/P: maybe not registered"));
                        }
                    }

                    /*Unregister from O/P BM*/
                    ErrorCode = MemPool_UnRegProducer(ControlBlock_p->OpBufHandle, (U32)ControlBlock_p);
                    if(ErrorCode != ST_NO_ERROR)
                    {
                        STTBX_Print(("Unable to Unregister O/P: maybe not registered"));
                    }
                }

                STTBX_Print(("PESES:Command Completed\n"));
                break;
            case TIMER_TERMINATE:
            /* Delete the task */
            STOS_TaskWait(&PESESTask,TIMEOUT_INFINITY);
#if defined(ST_OS20)
            STOS_TaskDelete(PESESTask,ControlBlock_p->CPUPartition_p,(void *)ControlBlock_p->PESESTaskstack,ControlBlock_p->CPUPartition_p);
#else
            STOS_TaskDelete(PESESTask,NULL,NULL,NULL);
#endif
            STTBX_Print(("PESESTask Deleted\n"));
            break;
            case TIMER_INIT:
            default:
            break;
        }
        //STTBX_Print(("TimerNextState=%x\n",State));
    }
    else
    {
        return ST_ERROR_UNKNOWN_DEVICE;
    }
    return ST_NO_ERROR;
}

ST_ErrorCode_t PESES_SetBitBufferLevel(STPESES_Handle_t Handle, U32 Level)
{
    PESES_ParserControl_t *ControlBlock_p;
    if (PESESInitMutex_p != NULL)
    {
        STOS_MutexLock(PESESInitMutex_p);
    }
    else
    {
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    /* Obtain the Mem Pool control block from the passed in handle */
    ControlBlock_p = PESES_GetControlBlockFromHandle(Handle);

    STOS_MutexRelease(PESESInitMutex_p);
    if(ControlBlock_p)
    {
        ControlBlock_p->LowDataLevel=(U8)Level;
    }
    else
    {
        return ST_ERROR_UNKNOWN_DEVICE;
    }
    return ST_NO_ERROR;
}


/******************** AV Sync events ****************************/
/* Subscribe Parser control block for receiving events from AVSync */
ST_ErrorCode_t PESES_SubscribeEvents(PESESInitParams_t *InitParams_p,PESES_ParserControl_t *ControlBlock_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STEVT_OpenParams_t EVT_OpenParams;


    memset(&EVT_OpenParams    ,0,sizeof(STEVT_OpenParams_t));

    memset(&ControlBlock_p->EVT_SubcribeParams,0,sizeof(STEVT_DeviceSubscribeParams_t));
    ControlBlock_p->EVT_SubcribeParams.NotifyCallback = PESES_Handle_AvSync_Evt;
    ControlBlock_p->EVT_SubcribeParams.SubscriberData_p = (void *)ControlBlock_p;

    /* Open the ST Event */
    ErrorCode =STEVT_Open(InitParams_p->EvtHandlerName,&EVT_OpenParams,&ControlBlock_p->EvtHandle);

    /* Subscribe to the events */
    if(InitParams_p->EvtSkipId != 0)
    {
        ErrorCode|=STEVT_SubscribeDeviceEvent(ControlBlock_p->EvtHandle,InitParams_p->EvtSkipName,
                                                                                          InitParams_p->EvtSkipId,&ControlBlock_p->EVT_SubcribeParams);
        strncpy(ControlBlock_p->EvtSkipName, InitParams_p->EvtSkipName,ST_MAX_DEVICE_NAME_TOCOPY);
        ControlBlock_p->EvtSkipId = InitParams_p->EvtSkipId;
    }
    //ErrorCode|=STEVT_SubscribeDeviceEvent(ControlBlock_p->EvtHandle,"PLAYER0",STAUD_AVSYNC_SKIP_EVT,&ControlBlock_p->EVT_SubcribeParams);

    return ErrorCode;
}

/* Register events to be sent */
ST_ErrorCode_t PESES_RegisterEvents(ST_DeviceName_t EventHandlerName,ST_DeviceName_t AudioDevice, PESES_ParserControl_t *ControlBlock_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Register to the events */
    ErrorCode|=STEVT_RegisterDeviceEvent(ControlBlock_p->EvtHandle,AudioDevice,STAUD_NEW_FRAME_EVT,&ControlBlock_p->EventIDNewFrame);
    //ErrorCode|=STEVT_RegisterDeviceEvent(STAUD_EventHandle,AudioDevice,STAUD_EMPHASIS_EVT,&EventIDEmphasis);
    //ErrorCode|=STEVT_RegisterDeviceEvent(STAUD_EventHandle,AudioDevice,STAUD_STOPPED_EVT,&EventIDStopped);
    //ErrorCode|=STEVT_RegisterDeviceEvent(STAUD_EventHandle,AudioDevice,STAUD_PCM_UNDERFLOW_EVT,&EventIDPCMUnderFlow);
    //ErrorCode|=STEVT_RegisterDeviceEvent(STAUD_EventHandle,AudioDevice,STAUD_PCM_BUF_COMPLETE_EVT,&EventIDPCMBufferComplete);
    ErrorCode|=STEVT_RegisterDeviceEvent(ControlBlock_p->EvtHandle,AudioDevice,STAUD_NEW_FREQUENCY_EVT,&ControlBlock_p->EventIDFrequencyChange);
    //ErrorCode|=STEVT_RegisterDeviceEvent(STAUD_EventHandle,AudioDevice,STAUD_DATA_ERROR_EVT,&EventIDCRC);
    ErrorCode|=STEVT_RegisterDeviceEvent(ControlBlock_p->EvtHandle,AudioDevice,STAUD_CDFIFO_UNDERFLOW_EVT,&ControlBlock_p->EventIDDataUnderFlow);
    //ErrorCode|=STEVT_RegisterDeviceEvent(STAUD_EventHandle,AudioDevice,STAUD_SYNC_ERROR_EVT,&EventIDSYNCError);
    ErrorCode|=STEVT_RegisterDeviceEvent(ControlBlock_p->EvtHandle,AudioDevice,STAUD_LOW_DATA_LEVEL_EVT,&ControlBlock_p->EventIDLowDataLevel);
    ErrorCode|=STEVT_RegisterDeviceEvent(ControlBlock_p->EvtHandle,AudioDevice,STAUD_FIFO_OVERF_EVT,&ControlBlock_p->EventIDDataOverFlow);
    ErrorCode|=STEVT_RegisterDeviceEvent(ControlBlock_p->EvtHandle,AudioDevice,STAUD_PREPARED_EVT,&ControlBlock_p->EventIDPrepared);
    ErrorCode|=STEVT_RegisterDeviceEvent(ControlBlock_p->EvtHandle,AudioDevice,STAUD_NEW_ROUTING_EVT,&ControlBlock_p->EventIDNewRouting);

    return ErrorCode;
}

/* Unsubscribe the parser */
ST_ErrorCode_t PESES_UnSubscribeEvents(PESES_ParserControl_t *ControlBlock_p)
{
    //PESES_ParserControl_t *ControlBlock_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    if(ControlBlock_p->EvtSkipId != 0)
    {
        ErrorCode =STEVT_UnsubscribeDeviceEvent(ControlBlock_p->EvtHandle,ControlBlock_p->EvtSkipName,ControlBlock_p->EvtSkipId);
    }

    ErrorCode|=STEVT_Close(ControlBlock_p->EvtHandle);
    return ErrorCode;
}

/* AV Sync event receiver Callback for the parser */
void PESES_Handle_AvSync_Evt(STEVT_CallReason_t Reason,const ST_DeviceName_t RegistrantName,STEVT_EventConstant_t Event,const void *EventData,const void *SubscriberData_p)
{
    PESES_ParserControl_t *ControlBlock_p;

    if (PESESInitMutex_p != NULL)
    {
        STOS_MutexLock(PESESInitMutex_p);
    }
    else
    {
        return ;
    }
    /* Get the control block for which event is received */
    ControlBlock_p = PESES_GetControlBlockFromName(SubscriberData_p);
    /* Now release the semaphore */
    STOS_MutexRelease(PESESInitMutex_p);

    /* Set the parameters for skipping the frame */
    if (Reason == CALL_REASON_NOTIFY_CALL)
   {
      if(Event == STAUD_AVSYNC_SKIP_EVT)
      {
            STOS_MutexLock(ControlBlock_p->Lock_p);
            if(ControlBlock_p->SkipDurationInMs == 0)
            {
                ControlBlock_p->SkipDurationInMs = *(U32 *)EventData;
                STTBX_Print(("SKIP=%d\n",(U32)ControlBlock_p->SkipDurationInMs));
            }

            STOS_MutexRelease(ControlBlock_p->Lock_p);

        }
    }

}

ST_ErrorCode_t PESES_GetSamplingFreq(STPESES_Handle_t Handle,U32 *SamplingFrequency_p)
{
    ST_ErrorCode_t Error=ST_NO_ERROR;
    PESES_ParserControl_t *ControlBlock_p;

    if (PESESInitMutex_p != NULL)
    {
        STOS_MutexLock(PESESInitMutex_p);
    }
    else
    {
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    /* Obtain the Mem Pool control block from the passed in handle */
    ControlBlock_p = PESES_GetControlBlockFromHandle(Handle);

    STOS_MutexRelease(PESESInitMutex_p);

    if(ControlBlock_p)
    {
        *SamplingFrequency_p=ControlBlock_p->SamplingFrequency;

        if(ControlBlock_p->StreamInfo.StreamContent != STAUD_STREAM_CONTENT_HE_AAC)
        {
#ifdef MSPP_PARSER
            Mspp_Parser_GetFreq((MspParser_Handle_t)ControlBlock_p->Mspp_Cntrl_p,SamplingFrequency_p);
#else
            Com_Parser_GetFreq((ComParser_Handle_t)ControlBlock_p->ComParser_p,SamplingFrequency_p);
#endif
        }
    }
    else
    {
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    return Error;
}

ST_ErrorCode_t PESES_SetStreamID(STPESES_Handle_t Handle,U32 StreamID)
{
    ST_ErrorCode_t Error=ST_NO_ERROR;
    PESES_ParserControl_t *ControlBlock_p;

    if (PESESInitMutex_p != NULL)
    {
        STOS_MutexLock(PESESInitMutex_p);
    }
    else
    {
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    /* Obtain the Mem Pool control block from the passed in handle */
    ControlBlock_p = PESES_GetControlBlockFromHandle(Handle);

    STOS_MutexRelease(PESESInitMutex_p);

    if(ControlBlock_p)
    {
        ControlBlock_p->StreamID=StreamID;
#ifdef MSPP_PARSER
        Error = Mspp_Parser_SetStreamID((MspParser_Handle_t)ControlBlock_p->Mspp_Cntrl_p, StreamID);
#else
        Error = Com_Parser_SetStreamID((ComParser_Handle_t)ControlBlock_p->ComParser_p, StreamID);
#endif

    }
    else
    {
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    return Error;
}

ST_ErrorCode_t PESES_GetStreamInfo(STPESES_Handle_t Handle,STAUD_StreamInfo_t * StreamInfo_p)
{
    ST_ErrorCode_t Error=ST_NO_ERROR;
    PESES_ParserControl_t *ControlBlock_p;

    if (PESESInitMutex_p != NULL)
    {
        STOS_MutexLock(PESESInitMutex_p);
    }
    else
    {
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    /* Obtain the Mem Pool control block from the passed in handle */
    ControlBlock_p = PESES_GetControlBlockFromHandle(Handle);

    STOS_MutexRelease(PESESInitMutex_p);

    if(ControlBlock_p)
    {
        /* These layer are not defined in Staudlx.h */
        StreamInfo_p->StreamContent=ControlBlock_p->StreamInfo.StreamContent;

#ifndef MSPP_PARSER

#ifdef  STAUD_INSTALL_HEAAC
        if(ControlBlock_p->StreamInfo.StreamContent==STAUD_STREAM_CONTENT_HE_AAC)
        {
            GetStreamInfoHEAAC(ControlBlock_p,StreamInfo_p);

        }
#endif
#endif
        if(ControlBlock_p->StreamInfo.StreamContent != STAUD_STREAM_CONTENT_HE_AAC)
        {
#ifdef MSPP_PARSER
            Error = Mspp_Parser_GetInfo((MspParser_Handle_t)ControlBlock_p->Mspp_Cntrl_p,StreamInfo_p);
#else
            Error = Com_Parser_GetInfo((ComParser_Handle_t)ControlBlock_p->ComParser_p,StreamInfo_p);
#endif
        }
    }
    else
    {
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    return Error;
}

ST_ErrorCode_t PESES_GetInputBufferParams(STPESES_Handle_t Handle,STAUD_BufferParams_t* DataParams_p)
{

    ST_ErrorCode_t Error=ST_NO_ERROR;
    PESES_ParserControl_t *ControlBlock_p;

    if (PESESInitMutex_p != NULL)
    {
        STOS_MutexLock(PESESInitMutex_p);
    }
    else
    {
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    /* Obtain the control block from the passed in handle */
    ControlBlock_p = PESES_GetControlBlockFromHandle(Handle);

    STOS_MutexRelease(PESESInitMutex_p);

    if(ControlBlock_p)
    {
        DataParams_p->BufferBaseAddr_p = (void*)ControlBlock_p->PESbufferAddress_p;
        DataParams_p->BufferSize         = ControlBlock_p->BitBufferSize;
        DataParams_p->NumBuffers         = 1;
    }
    else
    {
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    return Error;
}

ST_ErrorCode_t PESES_SetDataInputInterface(STPESES_Handle_t Handle,
                                                         GetWriteAddress_t  GetWriteAddress_p,
                                                         InformReadAddress_t InformReadAddress_p, void * const Handle_p)
{

    ST_ErrorCode_t Error=ST_NO_ERROR;
    PESES_ParserControl_t *ControlBlock_p;

    if (PESESInitMutex_p != NULL)
    {
        STOS_MutexLock(PESESInitMutex_p);
    }
    else
    {
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    /* Obtain the control block from the passed in handle */
    ControlBlock_p = PESES_GetControlBlockFromHandle(Handle);

    STOS_MutexRelease(PESESInitMutex_p);

    if(ControlBlock_p)
    {
        //PTIDMAReadWritePointer.WriteAddress_p= GetWriteAddress_p;
        //PTIDMAReadWritePointer.ReadAddress_p= InformReadAddress_p;
        //PTIDMAReadWritePointer.Handle=Handle_p;

        ControlBlock_p->PTIDMAReadWritePointer.WriteAddress_p= GetWriteAddress_p;
        ControlBlock_p->PTIDMAReadWritePointer.ReadAddress_p= InformReadAddress_p;
        ControlBlock_p->PTIDMAReadWritePointer.Handle=Handle_p;
    }
    else
    {
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    return Error;
}

ST_ErrorCode_t PESES_GetInputCapability(STPESES_Handle_t Handle, STAUD_IPCapability_t * Capability_p)
{

    ST_ErrorCode_t Error = ST_NO_ERROR;
    PESES_ParserControl_t *ControlBlock_p;

    if (PESESInitMutex_p != NULL)
    {
        STOS_MutexLock(PESESInitMutex_p);
    }
    else
    {
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    /* Obtain the control block from the passed in handle */
    ControlBlock_p = PESES_GetControlBlockFromHandle(Handle);

    STOS_MutexRelease(PESESInitMutex_p);

    if(ControlBlock_p)
    {
        Capability_p->PESParserCapable = TRUE;
        Capability_p->I2SInputCapable = FALSE;
    }
    else
    {
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    return Error;
}

ST_ErrorCode_t PESES_SetPCMParams(STPESES_Handle_t Handle, U32 Frequency, U32 SampleSize, U32 Nch)
{

    ST_ErrorCode_t Error = ST_NO_ERROR;
    PESES_ParserControl_t *ControlBlock_p;

    if (PESESInitMutex_p != NULL)
    {
        STOS_MutexLock(PESESInitMutex_p);
    }
    else
    {
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    /* Obtain the control block from the passed in handle */
    ControlBlock_p = PESES_GetControlBlockFromHandle(Handle);

    STOS_MutexRelease(PESESInitMutex_p);

    if(ControlBlock_p)
    {
#ifdef MSPP_PARSER
        Error = Mspp_Parser_SetPCMParams((MspParser_Handle_t)ControlBlock_p->Mspp_Cntrl_p,Frequency,SampleSize,Nch);
#else
        Error = Com_Parser_SetPCMParams((ComParser_Handle_t)ControlBlock_p->ComParser_p,Frequency,SampleSize,Nch);
#endif
    }
    else
    {
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    return Error;
}



/*-------------New Function to set broadcast profile-----------------------*/


ST_ErrorCode_t PESES_SetBroadcastProfile(STPESES_Handle_t Handle, STAUD_BroadcastProfile_t BroadcastProfile)
{

    PESES_ParserControl_t *ControlBlock_p;
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if (PESESInitMutex_p != NULL)
    {
        STOS_MutexLock(PESESInitMutex_p);
    }
    else
    {
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    ControlBlock_p = PESES_GetControlBlockFromHandle(Handle);

    STOS_MutexRelease(PESESInitMutex_p);

    if(ControlBlock_p)
    {
#ifdef MSPP_PARSER
        Error = Mspp_Parser_SetBroadcastProfile((MspParser_Handle_t)ControlBlock_p->Mspp_Cntrl_p,BroadcastProfile);
#else
        Error = Com_Parser_SetBroadcastProfile((ComParser_Handle_t)ControlBlock_p->ComParser_p ,BroadcastProfile);
#endif
        ControlBlock_p->BroadCastProfile = BroadcastProfile;
    }
    else
    {
        Error = ST_ERROR_INVALID_HANDLE;
    }

    return Error;
}

/*-------------New Function to Get broadcast profile-----------------------*/


ST_ErrorCode_t PESES_GetBroadcastProfile(STPESES_Handle_t Handle, STAUD_BroadcastProfile_t* BroadcastProfile_p)
{

    PESES_ParserControl_t *ControlBlock_p;
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if (PESESInitMutex_p != NULL)
    {
        STOS_MutexLock(PESESInitMutex_p);
    }
    else
    {
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    ControlBlock_p = PESES_GetControlBlockFromHandle(Handle);

    STOS_MutexRelease(PESESInitMutex_p);

    if(ControlBlock_p)
    {
        *BroadcastProfile_p =  ControlBlock_p->BroadCastProfile;
    }
    else
    {
        Error = ST_ERROR_INVALID_HANDLE;
    }

    return Error;
}
/*-------------New Function to enable EOF Handling-----------------------*/

ST_ErrorCode_t PESES_EnableEOFHandling(STPESES_Handle_t Handle)
{

    PESES_ParserControl_t *ControlBlock_p;
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if (PESESInitMutex_p != NULL)
    {
        STOS_MutexLock(PESESInitMutex_p);
    }
    else
    {
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    ControlBlock_p = PESES_GetControlBlockFromHandle(Handle);

    STOS_MutexRelease(PESESInitMutex_p);

    if(ControlBlock_p)
    {
        STOS_MutexLock(ControlBlock_p->Lock_p);
        if(ControlBlock_p->EOFFlag == FALSE)
        {

            ControlBlock_p->EOFFlag = TRUE;
        }
        else
        {
            Error = ST_ERROR_BAD_PARAMETER;
        }
        STOS_MutexRelease(ControlBlock_p->Lock_p);
    }
    else
    {
        Error = ST_ERROR_INVALID_HANDLE;
    }
    return Error;
}

/*-------------New Function to Set Speed-----------------------*/

ST_ErrorCode_t PESES_SetSpeed(STPESES_Handle_t Handle, S32 Speed)
{

    PESES_ParserControl_t *ControlBlock_p;
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if (PESESInitMutex_p != NULL)
    {
        STOS_MutexLock(PESESInitMutex_p);
    }
    else
    {
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    ControlBlock_p = PESES_GetControlBlockFromHandle(Handle);

    STOS_MutexRelease(PESESInitMutex_p);

    if(ControlBlock_p)
    {
        STOS_MutexLock(ControlBlock_p->Lock_p);
        ControlBlock_p->Speed = Speed;
        STOS_MutexRelease(ControlBlock_p->Lock_p);
#ifdef MSPP_PARSER
        Error = Mspp_Parser_SetSpeed((MspParser_Handle_t)ControlBlock_p->Mspp_Cntrl_p, Speed);
#else
        Error = Com_Parser_SetSpeed((ComParser_Handle_t)ControlBlock_p->ComParser_p, Speed);
#endif
    }
    else
    {
        Error = ST_ERROR_INVALID_HANDLE;
    }
    return Error;
}

/*-------------New Function to Get Speed-----------------------*/

ST_ErrorCode_t PESES_GetSpeed(STPESES_Handle_t Handle, S32 *Speed_p)
{

    PESES_ParserControl_t *ControlBlock_p;
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if (PESESInitMutex_p != NULL)
    {
        STOS_MutexLock(PESESInitMutex_p);
    }
    else
    {
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    ControlBlock_p = PESES_GetControlBlockFromHandle(Handle);

    STOS_MutexRelease(PESESInitMutex_p);

    if(ControlBlock_p)
    {
        *Speed_p = ControlBlock_p->Speed;
    }
    else
    {
        Error = ST_ERROR_INVALID_HANDLE;
    }
    return Error;
}

/*-------------New Function to Set WMA Stream ID to Play -----------------------*/
ST_ErrorCode_t PESES_SetWMAStreamID(STPESES_Handle_t Handle, U8 WMAStreamNumber)
{
    PESES_ParserControl_t *ControlBlock_p;
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if (PESESInitMutex_p != NULL)
    {
        STOS_MutexLock(PESESInitMutex_p);
    }
    else
    {
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    ControlBlock_p = PESES_GetControlBlockFromHandle(Handle);

    STOS_MutexRelease(PESESInitMutex_p);

    if(ControlBlock_p)
    {
#ifdef MSPP_PARSER
        Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
#else
        Error = Com_Parser_SetWMAStreamID((ComParser_Handle_t)ControlBlock_p->ComParser_p, WMAStreamNumber);
#endif
    }
    else
    {
        Error = ST_ERROR_INVALID_HANDLE;
    }
    return Error;
}

/*-------------New Function to Get Free Bit Buffer Size-----------------------*/

ST_ErrorCode_t PESES_GetBitBufferFreeSize(STPESES_Handle_t Handle, U32 * Size_p)
{
    PESES_ParserControl_t *ControlBlock_p;
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if(Size_p == NULL)
    {
        /*Bad Pointer*/
        return ST_ERROR_BAD_PARAMETER;
    }

    if (PESESInitMutex_p != NULL)
    {
        STOS_MutexLock(PESESInitMutex_p);
    }
    else
    {
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    ControlBlock_p = PESES_GetControlBlockFromHandle(Handle);

    STOS_MutexRelease(PESESInitMutex_p);

    if(ControlBlock_p)
    {

        if(ControlBlock_p->IpBufHandle)
        {
            /*There is no Bit buffer. Data is obtained from input block*/
            return ST_ERROR_BAD_PARAMETER;
        }

        STOS_MutexLock(ControlBlock_p->Lock_p);
        if(ControlBlock_p->PTIDMAReadWritePointer.WriteAddress_p && ControlBlock_p->PTIDMAReadWritePointer.ReadAddress_p)
        {
            U32 CurrentPTIWritePointer = 0;
            U32 PTIReadPointer =0;
            int PointerDifference = 0;
            if(ControlBlock_p->PTIDMAReadWritePointer.WriteAddress_p(ControlBlock_p->PTIDMAReadWritePointer.Handle,(void **)&CurrentPTIWritePointer) != ST_NO_ERROR)
            {
                Error = ST_ERROR_BAD_PARAMETER;
            }
            else
            {
                PTIReadPointer = (U32)ControlBlock_p->PTIReadPointer_p;
                PointerDifference = CurrentPTIWritePointer - PTIReadPointer;

                if(PointerDifference < 0)
                {
                    /*read below write so write has wrapped around*/
                    /*pick only upto the end and the rest when we have completed this pick up*/
                    PointerDifference = ControlBlock_p->BitBufferSize - (PTIReadPointer - CurrentPTIWritePointer);
                }

                *Size_p = ControlBlock_p->BitBufferSize - PointerDifference;
            }
        }
        else
        {
            *Size_p = 0;
            Error = ST_ERROR_BAD_PARAMETER;
        }
        STOS_MutexRelease(ControlBlock_p->Lock_p);
    }
    else
    {
        Error = ST_ERROR_INVALID_HANDLE;
    }
    return Error;

}

/*-------------New Function to Get Synchro unit-----------------------*/
ST_ErrorCode_t PESES_GetSynchroUnit(STPESES_Handle_t Handle,STAUD_SynchroUnit_t *SynchroUnit_p)
{
    PESES_ParserControl_t *ControlBlock_p;
    ST_ErrorCode_t Error = ST_NO_ERROR;
    ParserState_t CurrentState;

    if (PESESInitMutex_p != NULL)
    {
        STOS_MutexLock(PESESInitMutex_p);
    }
    else
    {
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    ControlBlock_p = PESES_GetControlBlockFromHandle(Handle);

    STOS_MutexRelease(PESESInitMutex_p);

    if(ControlBlock_p)
    {
        //already in task lock
        //STOS_MutexLock(ControlBlock_p->PESESCurrentState.Lock);
        CurrentState = ControlBlock_p->PESESCurrentState.State;
        //STOS_MutexRelease(ControlBlock_p->PESESCurrentState.Lock);

        if(CurrentState == TIMER_TRANSFER_START)
        {
#ifdef MSPP_PARSER
            Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
#else
            Error = Com_Parser_GetSynchroUnit((ComParser_Handle_t)ControlBlock_p->ComParser_p, SynchroUnit_p);
#endif
        }
        else
        {
            Error = STAUD_ERROR_DECODER_STOPPED;
        }
    }
    else
    {
        Error = ST_ERROR_INVALID_HANDLE;
    }
    return Error;

}

/*-------------New Function to skip-----------------------*/
ST_ErrorCode_t PESES_SkipSynchro(STPESES_Handle_t Handle, U32 Delay)
{
    PESES_ParserControl_t *ControlBlock_p;
    ST_ErrorCode_t Error = ST_NO_ERROR;
    ParserState_t CurrentState;

    if (PESESInitMutex_p != NULL)
    {
        STOS_MutexLock(PESESInitMutex_p);
    }
    else
    {
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    ControlBlock_p = PESES_GetControlBlockFromHandle(Handle);

    STOS_MutexRelease(PESESInitMutex_p);

    if(ControlBlock_p)
    {
        //already in task lock
        //STOS_MutexLock(ControlBlock_p->PESESCurrentState.Lock);
        CurrentState = ControlBlock_p->PESESCurrentState.State;
        //STOS_MutexRelease(ControlBlock_p->PESESCurrentState.Lock);

        if(CurrentState == TIMER_TRANSFER_START)
        {
#ifdef MSPP_PARSER
            Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
#else
            Error = Com_Parser_SkipSynchro((ComParser_Handle_t)ControlBlock_p->ComParser_p, Delay);
#endif
        }
        else
        {
            Error = STAUD_ERROR_DECODER_STOPPED;
        }
    }
    else
    {
        Error = ST_ERROR_INVALID_HANDLE;
    }
    return Error;
}

/*-------------New Function to pause-----------------------*/
ST_ErrorCode_t PESES_PauseSynchro(STPESES_Handle_t Handle, U32 Delay)
{
    PESES_ParserControl_t *ControlBlock_p;
    ST_ErrorCode_t Error = ST_NO_ERROR;
    ParserState_t CurrentState;

    if (PESESInitMutex_p != NULL)
    {
        STOS_MutexLock(PESESInitMutex_p);
    }
    else
    {
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    ControlBlock_p = PESES_GetControlBlockFromHandle(Handle);

    STOS_MutexRelease(PESESInitMutex_p);

    if(ControlBlock_p)
    {
        //already in task lock
        //STOS_MutexLock(ControlBlock_p->PESESCurrentState.Lock);
        CurrentState = ControlBlock_p->PESESCurrentState.State;
        //STOS_MutexRelease(ControlBlock_p->PESESCurrentState.Lock);

        if(CurrentState == TIMER_TRANSFER_START)
        {
#ifdef MSPP_PARSER
            Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
#else
            Error = Com_Parser_PauseSynchro((ComParser_Handle_t)ControlBlock_p->ComParser_p, Delay);
#endif
        }
        else
        {
            Error = STAUD_ERROR_DECODER_STOPPED;
        }
    }
    else
    {
        Error = ST_ERROR_INVALID_HANDLE;
    }
    return Error;
}

/*-------------End of Function-----------------------*/


