/******************************************************************************/
/*                                                                            */
/* File name   : blockmanager.c                                               */
/*                                                                            */
/* Description : Initialization of Mem Pool driver                            */
/*                                                                            */
/* COPYRIGHT (C) ST-Microelectronics 2005                                     */
/* History     :                                                              */
/* Date               Modification                 Name                       */
/* ----               ------------                 ----                       */
/* 08/09/05           Created                      K.MAITI                    */
/*                                                                            */
/*                                                                            */
/*  NOTE:                                                                     */
/******************************************************************************/

/*****************************************************************************
Includes
*****************************************************************************/
//#define  STTBX_PRINT

#ifndef ST_OSLINUX
#include <string.h>
#include "sttbx.h"
#endif
#include "stos.h"
#include "blockmanager.h"
#include "staud.h"

STMEMBLK_Handle_t MEMBLK_Handle[MEMBLK_MAX_NUMBER];
#ifdef ST_51XX
ST_DeviceName_t MemPool_DeviceName[] = {"MEM0","MEM1","MEM2","MEM3","MEM4","MEM5","MEM6","MEM7"};
#else
ST_DeviceName_t MemPool_DeviceName[] = {"MEM0","MEM1","MEM2","MEM3","MEM4","MEM5","MEM6","MEM7",
                                        "MEM8","MEM9","MEM10","MEM11","MEM12","MEM13","MEM14","MEM15",
                                        "MEM16","MEM17","MEM18","MEM19","MEM20","MEM21","MEM22","MEM23",
                                        "MEM24","MEM25","MEM26","MEM27","MEM28","MEM29","MEM30","MEM31"};

#endif
#define NO_CONSUMER_MODE    0 //Important:modify only in consultation with audiodriver team
#define NO_PRODUCER_MODE    0 //Important:modify only in consultation with audiodriver team
/*****************************************************************************
The MemPoolHead_p points to the head of the queue of Memory Pools that have
been initialized i.e., MemoryPool_Init() has been called to append them
to the queue.
At system startup, MemPoolHead_p will be NULL.  When the first Memory Pool is
initialized, MemPoolHead_p will point to the Memory Pool control block for that
Pool once MemoryPool_Init() has been called.
*****************************************************************************/

 /* Queue of initialized Memory Pool */
MemoryPoolControl_t *MemPoolHead_p = NULL;

static STOS_Mutex_t *InitMutex_p = NULL;

/* Private Functions Prototypes -------------------------------------------- */

/* Memory Pool Queue Management */
static U32  MemPool_QueueSize(void);

static void MemPool_QueueAdd(MemoryPoolControl_t *Item_p);

static void MemPool_QueueRemove(MemoryPoolControl_t *Item_p);
/*
static MemoryPoolControl_t *
   MemPool_GetControlBlockFromName(const ST_DeviceName_t);
*/
static MemoryPoolControl_t *
   MemPool_GetControlBlockFromHandle(STMEMBLK_Handle_t Handle);

ST_ErrorCode_t MemPool_ReleaseMemory(MemoryPoolControl_t * Pool_p);

/*****************************************************************************
MemPool_QueueSize()
Description:
    This routine finds the no of allocated Memory Pool control blocks.

Parameters:
    void.
Return Value:
    U32, the no of allocated Memory Pool control blocks.
See Also:

*****************************************************************************/
static U32  MemPool_QueueSize(void)
{
    U32 QueueSize=0;
    MemoryPoolControl_t *qp = MemPoolHead_p;

    while(qp)
    {
        QueueSize++;
        qp = qp->Next_p;
    }

    return QueueSize;

}
/*****************************************************************************
MemPool_QueueAdd()
Description:
    This routine appends an allocated Memory Pool control block to the
    queue.

    NOTE: The semaphore lock must be held when calling this routine.

Parameters:
    Item_p, the control block to append to the queue.
Return Value:
    Nothing.
See Also:
    MemPool_QueueRemove()
*****************************************************************************/
static void MemPool_QueueAdd(MemoryPoolControl_t *Item_p)
{
    MemoryPoolControl_t *qp;

    /* Check the base element is defined, otherwise, append to the end of the
      linked list. */
    if (MemPoolHead_p == NULL)
    {
        /* As this is the first item, no need to append */
        MemPoolHead_p = Item_p;
        MemPoolHead_p->Next_p = NULL;
    }
    else
    {
        /* Iterate along the list until we reach the final item */
        qp = MemPoolHead_p;
        while (qp != NULL && qp->Next_p)
        {
            qp = qp->Next_p;
        }

        /* Append the new item */
        qp->Next_p = Item_p;
        Item_p->Next_p = NULL;
    }
} /*MemPool_QueueAdd*/

/*****************************************************************************
MemPool_QueueRemove()
Description:
    This routine removes an allocated Mem Pool control block from the
    Mem Pool queue.

    NOTE: The semaphore lock must be held when calling this routine.
    Use MemPool_IsInit() or MemPool_GetControlBlockFromName() to verify whether
    or not a particular item is in the queue prior to calling this routine.

Parameters:
    Item_p, the control block to remove from the queue.
Return Value:
    Nothing.
See Also:
    MemPool_QueueAdd()
*****************************************************************************/
static void MemPool_QueueRemove(MemoryPoolControl_t *Item_p)
{
    MemoryPoolControl_t *this_p, *last_p;

    /* Check the base element is defined, otherwise quit as no items are
     * present on the queue.
     */
    if (MemPoolHead_p != NULL)
    {
        /* Reset pointers to loop start conditions */
        last_p = NULL;
        this_p = MemPoolHead_p;

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
                MemPoolHead_p = this_p->Next_p;
            }
        }
    }
}/*MemPool_QueueRemove*/

/*****************************************************************************
MemPool_IsInit()
Description:
    Runs through the queue of initialized Mem Pools and checks that
    the Mem Pool with the specified device name has not already been added.

    NOTE: Must be called with the queue semaphore held.

Parameters:
    DeviceName, text string indicating the device name to check.
Return Value:
    TRUE, the device has already been initialized.
    FALSE, the device is not in the queue and therefore is not initialized.
See Also:
    MemPool_Init()
*****************************************************************************/
BOOL MemPool_IsInit(const ST_DeviceName_t PoolName)
{
    BOOL Initialized = FALSE;
    MemoryPoolControl_t *qp = MemPoolHead_p; /* Global queue head pointer */

    while (qp)
    {
        /* Check the device name for a match with the item in the queue */
        if (strcmp(qp->PoolName, PoolName) != 0)
        {
            /* Next Mem Pool in the queue */
            qp = qp->Next_p;
        }
        else
        {
            /* The Mem Pool is already initialized */
            Initialized = TRUE;
            break;
        }
    }

    /* Boolean return value reflecting whether the device is initialized */
    return Initialized;
} /* MemPool_IsInit() */

/*****************************************************************************
MemPool_GetControlBlockFromName()
Description:
    Runs through the queue of initialized Mem Pool and checks for
    the Mem Pool with the specified name.

    NOTE: Must be called with the queue semaphore held.

Parameters:
    DeviceName, text string indicating the device name to check.
Return Value:
    NULL, end of queue encountered - invalid device name.
    Otherwise, returns a pointer to the control block for the device.
See Also:
    MemPool_IsInit()
*****************************************************************************/
#if 0
static MemoryPoolControl_t *
   MemPool_GetControlBlockFromName(const ST_DeviceName_t PoolName)
{
    MemoryPoolControl_t *qp = MemPoolHead_p; /* Global queue head pointer */

    while (qp != NULL)
    {
        /* Check the Mem Pool name for a match with the item in the queue */
        if (strcmp(qp->PoolName, PoolName) != 0)
        {
            /* Next Mem Pool in the queue */
            qp = qp->Next_p;
        }
        else
        {
            /* The Mem Pool entry has been found */
            break;
        }
    }

    /* Return the Mem Pool control block (or NULL) to the caller */
    return qp;

}/*MemPool_GetControlBlockFromName()*/
#endif
/*****************************************************************************
MemPool_GetControlBlockFromHandle()
Description:
    Runs through the queue of initialized Mem Pool and checks for
    the Mem Pool with the specified handle.

    NOTE: Must be called with the queue semaphore held.

Parameters:
    Handle, an open handle.
Return Value:
    NULL, end of queue encountered - invalid handle.
    Otherwise, returns a pointer to the control block for the device.
See Also:
    MemPool_GetControlBlockFromName()
*****************************************************************************/
static MemoryPoolControl_t *
   MemPool_GetControlBlockFromHandle(STMEMBLK_Handle_t Handle)
{
    MemoryPoolControl_t *qp = MemPoolHead_p; /* Global queue head pointer */

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

            /* Try the next Mem Pool */
            qp = qp->Next_p;
        }
    }
    else
    {
        qp = NULL;
    }

    /* Return the Mem Pool control block (or NULL) to the caller */
    return qp;
} /*MemPool_GetControlBlockFromHandle()*/

/*****************************************************************************
MemPool_Init()
Description:
    Initializes the named Mem Pool.

    NOTES: The very first call to this routine will create a semaphore
    for locking access to the Mem Pool.  The semaphore will be deleted
    during the final call to MemPool_Term().

Parameters:
    DeviceName, text string indicating the device name to be used.
    InitParams_p, parameters to control the initialization of the device.
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
    MemPool_Term()
*****************************************************************************/
ST_ErrorCode_t MemPool_Init(ST_DeviceName_t  Name, MemPoolInitParams_t *InitParams_p,
                            STMEMBLK_Handle_t *Handle)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
#ifndef ST_51XX
    ST_ErrorCode_t Error = ST_NO_ERROR;
#endif
    MemoryPoolControl_t *Pool_p = NULL;
    ST_Partition_t  *CPUPartition = NULL;
    U32 i;

    STOS_TaskLock();
    if(InitMutex_p == NULL)
    {
        InitMutex_p = STOS_MutexCreateFifo();
    }
    STOS_TaskUnlock();

    STOS_MutexLock(InitMutex_p);
    /* Check the parameters passed to init. */
    if ((strlen(Name) > ST_MAX_DEVICE_NAME) ||(InitParams_p->NumBlocks == 0) ||
        (InitParams_p->BlockSize == 0) || (InitParams_p->NumBlocks > MAX_NUM_BLOCK))
    {
        /*Bad parameter*/
        error = MemPool_ReleaseMemory(Pool_p);
        return ST_ERROR_BAD_PARAMETER;
    }

    if(MemPool_QueueSize() >= MEMBLK_MAX_NUMBER)
    {
        error = MemPool_ReleaseMemory(Pool_p);
        return ST_ERROR_NO_MEMORY;
    }

    if(MemPool_IsInit(Name))
    {
        /*Already initialized*/
        error = MemPool_ReleaseMemory(Pool_p);
        return ST_ERROR_ALREADY_INITIALIZED;
    }

    /*Allocate the control block*/
    CPUPartition = InitParams_p->CPUPartition_p;
    Pool_p = (MemoryPoolControl_t *)STOS_MemoryAllocate(CPUPartition,sizeof(MemoryPoolControl_t));

    /* Ensure memory allocation was successful */
    if (Pool_p == NULL)
    {
        /*No memory*/
        error = MemPool_ReleaseMemory(Pool_p);
        return ST_ERROR_NO_MEMORY;
    }

    /*Initialize the controll block*/
    memset(Pool_p,0,sizeof(MemoryPoolControl_t));
    strncpy(Pool_p->PoolName, Name, ST_MAX_DEVICE_NAME_TOCOPY);

    Pool_p->NumBlocks = InitParams_p->NumBlocks;
    Pool_p->BlockSize = InitParams_p->BlockSize;
    Pool_p->TotalSize = (InitParams_p->BlockSize * InitParams_p->NumBlocks);
    Pool_p->CPUPartition_p = InitParams_p->CPUPartition_p;
    Pool_p->MaxLoggingEntries = MAX_LOGGING_ENTRIES;
    Pool_p->Start = FALSE;

    Pool_p->Lock_p = STOS_MutexCreateFifo();

    Pool_p->Next_p = NULL;

    Pool_p->NextFree = 0;
    Pool_p->NextFilled = 0;

    Pool_p->ConQSize  = 0;
    Pool_p->ProdQSize = Pool_p->NumBlocks;

#if MEMORY_SPLIT_SUPPORT
    Pool_p->SplitSize = 0;
#endif

    for(i = 0;i < MAX_LOGGING_ENTRIES; i++)
    {
        Pool_p->NextSent[i] = 0;
        Pool_p->NextRelease[i] = 0;
        Pool_p->Request[i] = FALSE;
    }

    Pool_p->Block_p = (Block_t *)STOS_MemoryAllocate(Pool_p->CPUPartition_p,(sizeof(Block_t) * InitParams_p->NumBlocks));

    if(Pool_p->Block_p == NULL)
    {
        error = MemPool_ReleaseMemory(Pool_p);
        return ST_ERROR_NO_MEMORY;
    }

    memset(Pool_p->Block_p,0,(sizeof(Block_t) * Pool_p->NumBlocks));

    for(i = 1; i< Pool_p->NumBlocks; i++)
    {
        Pool_p->Block_p[i-1].Next_p = &(Pool_p->Block_p[i]);
    }

    Pool_p->BlkList_p = (BlockList_t *)STOS_MemoryAllocate(Pool_p->CPUPartition_p,(sizeof(BlockList_t) * InitParams_p->NumBlocks));

    if(Pool_p->BlkList_p == NULL)
    {
        error = MemPool_ReleaseMemory(Pool_p);
        return ST_ERROR_NO_MEMORY;
    }

    memset(Pool_p->BlkList_p,0,(sizeof(BlockList_t) * Pool_p->NumBlocks));

#ifndef STAUD_REMOVE_EMBX
    if (InitParams_p->AllocateFromEMBX)
    {
        /*Allocate from EMBX*/
        Pool_p->AllocateFromEMBX = InitParams_p->AllocateFromEMBX;

        Error = EMBX_OpenTransport(EMBX_TRANSPORT_NAME, &Pool_p->tp);
        if(Error != EMBX_SUCCESS)
        {
            /*Unable to open transport*/
            error = MemPool_ReleaseMemory(Pool_p);
            return ST_ERROR_NO_MEMORY;/*set something else*/
        }

#ifdef ST_8010
        Error = EMBX_Alloc(Pool_p->tp, (Pool_p->TotalSize+64), (EMBX_VOID **)&(Pool_p->Buffer_p)) ;

        if(Error != EMBX_SUCCESS)
        {
            /*No memory*/
            error = MemPool_ReleaseMemory(Pool_p);
            return ST_ERROR_NO_MEMORY;
        }

        memset(Pool_p->Buffer_p,0,Pool_p->TotalSize+64);
        for(i = 0; i < Pool_p->NumBlocks; i++)
        {
            Pool_p->Block_p[i].Block.MemoryStart = (U32)(Pool_p->Buffer_p)+((U32)(Pool_p->Buffer_p)+64)%64 + (Pool_p->BlockSize*i);
            Pool_p->Block_p[i].Block.MaxSize = Pool_p->BlockSize;
        }
#else
        Error = EMBX_Alloc(Pool_p->tp, (Pool_p->TotalSize), (EMBX_VOID **)&(Pool_p->Buffer_p)) ;

        if(Error != EMBX_SUCCESS)
        {
            /*No memory*/
            error = MemPool_ReleaseMemory(Pool_p);
            return ST_ERROR_NO_MEMORY;
        }

        memset(Pool_p->Buffer_p,0,Pool_p->TotalSize);

        for(i = 0; i < Pool_p->NumBlocks; i++)
        {
            Pool_p->Block_p[i].Block.MemoryStart = (U32)(Pool_p->Buffer_p) + (Pool_p->BlockSize*i);
            Pool_p->Block_p[i].Block.MaxSize = Pool_p->BlockSize;
        }

#endif
    }
    else if (InitParams_p->AVMEMPartition)
#endif
#ifdef STAUD_REMOVE_EMBX
    if (InitParams_p->AVMEMPartition)
#endif
    {
        /*Allocate from AVMem*/
        STAVMEM_AllocBlockParams_t  AllocParams;
        STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;
        void                *VirtualAddress;

        memset(&AllocParams,0,sizeof(STAVMEM_AllocBlockParams_t));
        Pool_p->AVMEMPartition = InitParams_p->AVMEMPartition;

        STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping);

        AllocParams.PartitionHandle = Pool_p->AVMEMPartition;
        AllocParams.Alignment = 64;
        AllocParams.AllocMode = STAVMEM_ALLOC_MODE_TOP_BOTTOM;
        AllocParams.ForbiddenRangeArray_p = NULL;
        AllocParams.ForbiddenBorderArray_p = NULL;
        AllocParams.NumberOfForbiddenRanges = 0;
        AllocParams.NumberOfForbiddenBorders = 0;
        AllocParams.Size = Pool_p->TotalSize;

        //Pool_p->BufferHandle = 0;
        error = STAVMEM_AllocBlock(&AllocParams, &Pool_p->BufferHandle);
        if(error != ST_NO_ERROR)
        {
            /*No memory*/
            error = MemPool_ReleaseMemory(Pool_p);
            return ST_ERROR_NO_MEMORY;
        }

        /*Buffer Allocated*/
        error = STAVMEM_GetBlockAddress(Pool_p->BufferHandle, &VirtualAddress);
        if(error != ST_NO_ERROR)
        {
            /*No memory*/
            error = MemPool_ReleaseMemory(Pool_p);
            return ST_ERROR_NO_MEMORY;
        }

        Pool_p->Buffer_p = (U32 *)STAVMEM_VirtualToCPU(VirtualAddress,&VirtualMapping);
        for(i = 0; i < Pool_p->NumBlocks; i++)
        {
            Pool_p->Block_p[i].Block.MemoryStart = (U32)((U32)Pool_p->Buffer_p + (Pool_p->BlockSize * i));
            Pool_p->Block_p[i].Block.MaxSize = Pool_p->BlockSize;
        }
    }
    else
    {
        /*No partition to allocate memory from*/
        /*Block manager was to be initialized with no memory*/
        error = MemPool_ReleaseMemory(Pool_p);
        return ST_ERROR_BAD_PARAMETER;
    }

    /* Everything is going well, so we can now
     * finally append this item to the Mem Pool queue.
     */
    Pool_p->Handle = (STMEMBLK_Handle_t)Pool_p;
    *Handle = Pool_p->Handle;
    MemPool_QueueAdd(Pool_p);

    /* Now release the semaphore */
    STOS_MutexRelease(InitMutex_p);
    return error;
} /*MemPool_Init()*/


/*****************************************************************************
MemPool_Term()
Description:
    Terminates the Mem Pool device.
Parameters:
    Name, text string indicating Mem Pool to terminate.

Return Value:
    ST_NO_ERROR,                    no errors.
    ST_ERROR_UNKNOWN_DEVICE,        invalid device name.
    ST_ERROR_OPEN_HANDLE,           could not terminate the device.
    ST_ERROR_INVALID_HANDLE,        the handle is invalid.

See Also:
    MemPool_Init()
*****************************************************************************/
ST_ErrorCode_t MemPool_Term(STMEMBLK_Handle_t Handle)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    MemoryPoolControl_t *Pool_p;

    if (InitMutex_p != NULL)
    {
        STOS_MutexLock(InitMutex_p);
    }
    else
    {
        return ST_ERROR_BAD_PARAMETER;
    }

    /* Obtain the Mem Pool control block associated with the device name */
    Pool_p = MemPool_GetControlBlockFromHandle(Handle);

    if (Pool_p)
    {
        /* Remove device from queue */
        MemPool_QueueRemove(Pool_p);

        /*Free up all the memory now*/
        error = MemPool_ReleaseMemory(Pool_p);
    }
    else
    {
        error = ST_ERROR_INVALID_HANDLE;
        /* If there's an instance, the semaphore must be valid */
        STOS_MutexRelease(InitMutex_p);
    }

    return error;
}/*MemPool_Term*/

/*****************************************************************************
MemPool_RegProducer()
Description:
    Registers the producer to the Mem Pool.
Parameters:
    Handle, handle to the Mem Pool.
     Producer, Producer params.

Return Value:
    ST_NO_ERROR,                    no errors.
    ST_ERROR_UNKNOWN_DEVICE,        invalid device name.
    ST_ERROR_INVALID_HANDLE,        the handle is invalid.
    ST_ERROR_ALREADY_INITIALIZED        Already Registered
    ST_ERROR_NO_MEMORY                  No place to register
See Also:
    MemPool_RegConsumer()
*****************************************************************************/
ST_ErrorCode_t MemPool_RegProducer( STMEMBLK_Handle_t Handle, ProducerParams_t  Producer)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    MemoryPoolControl_t *Pool_p;

    if((Producer.Handle == (U32)NULL) /*|| (Producer.sem == NULL)*/)
    {
        return ST_ERROR_BAD_PARAMETER;
    }

    if (InitMutex_p != NULL)
    {
        STOS_MutexLock(InitMutex_p);
    }
    else
    {
        return ST_ERROR_BAD_PARAMETER;
    }

    /* Obtain the Mem Pool control block from the passed in handle */
    Pool_p = MemPool_GetControlBlockFromHandle(Handle);

    STOS_MutexRelease(InitMutex_p);

    /* Only continue if we have a control block for this device */
    if (Pool_p != NULL)
    {
        STOS_MutexLock(Pool_p->Lock_p);

        if((Pool_p->Logged[0].Handle == (U32)NULL) || (Pool_p->Logged[0].Handle == Producer.Handle))
        {
            /*Allow if no producer or teh same producer*/
            if(Pool_p->Logged[0].Handle == (U32)NULL)
            {
                Pool_p->Logged[0].Handle = Producer.Handle;
                Pool_p->Logged[0].sem    = Producer.sem;
                Pool_p->Request[0]       = FALSE;
                Pool_p->LoggedEntries ++; /*total logged entries*/
                Pool_p->NumProducer++; /*Should be 1(only one producer) after this instruction*/

            }
        }
        else
        {
            error = ST_ERROR_NO_MEMORY;
        }
        STOS_MutexRelease(Pool_p->Lock_p);

    }
    else
    {
        error = ST_ERROR_INVALID_HANDLE;
    }

    return error;
}/*MemPool_RegProducer()*/

/*****************************************************************************
MemPool_RegConsumer()
Description:
    Registers the producer to the Mem Pool.
Parameters:
    Handle, handle to the Mem Pool.
    Consumer, consumer params.

Return Value:
    ST_NO_ERROR,                    no errors.
    ST_ERROR_UNKNOWN_DEVICE,        invalid device name.
    ST_ERROR_INVALID_HANDLE,        the handle is invalid.
    ST_ERROR_ALREADY_INITIALIZED      Already Registered
    ST_ERROR_NO_MEMORY              No place to register
See Also:
    MemPool_RegProducer()
*****************************************************************************/
ST_ErrorCode_t MemPool_RegConsumer( STMEMBLK_Handle_t Handle, ConsumerParams_t  Consumer)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    MemoryPoolControl_t *Pool_p;
    U32 ConsumerCount=1;

    if((Consumer.Handle == (U32)NULL) /*|| (Consumer.sem == NULL)*/)
    {
        return ST_ERROR_BAD_PARAMETER;
    }

    if (InitMutex_p != NULL)
    {
        STOS_MutexLock(InitMutex_p);
    }
    else
    {
        return ST_ERROR_BAD_PARAMETER;
    }

    /* Obtain the Mem Pool control block from the passed in handle */
    Pool_p = MemPool_GetControlBlockFromHandle(Handle);

    STOS_MutexRelease(InitMutex_p);

    /* Only continue if we have a control block for this device */
    if (Pool_p != NULL)
    {
        STOS_MutexLock(Pool_p->Lock_p);

        while(Pool_p->Logged[ConsumerCount].Handle && (Pool_p->Logged[ConsumerCount].Handle != Consumer.Handle)
                                                                                            && (ConsumerCount < Pool_p->MaxLoggingEntries ))
        {
            ConsumerCount++;
        }

        if(Pool_p->Logged[ConsumerCount].Handle == (U32)NULL)
        {
            Pool_p->Logged[ConsumerCount].Handle = Consumer.Handle;
            Pool_p->Logged[ConsumerCount].sem    = Consumer.sem;
            Pool_p->Request[ConsumerCount] = FALSE;

            /*The next block to be filled by the producer*/

            Pool_p->NextSent[ConsumerCount] = Pool_p->NextFilled;
            Pool_p->NextRelease[ConsumerCount] = Pool_p->NextSent[ConsumerCount];

            Pool_p->LoggedEntries ++;
            Pool_p->NumConsumer ++;
        }
        else if(Pool_p->Logged[ConsumerCount].Handle != Consumer.Handle)
            {
                /*We are the last and so no memory*/
                error = ST_ERROR_NO_MEMORY;
            }

        STOS_MutexRelease(Pool_p->Lock_p);
    }
    else
    {
        error = ST_ERROR_INVALID_HANDLE;
    }

    return error;
}/*MemPool_RegConsumer()*/

/*****************************************************************************
MemPool_GetOpBlk()
Description:
    Registers the producer to the Mem Pool.
Parameters:
    Handle, handle to the Mem Pool.
     AddressToBlkPtr_p, free Block to be get.

Return Value:
    ST_NO_ERROR,                    no errors.
    ST_ERROR_UNKNOWN_DEVICE,        invalid device name.
    ST_ERROR_INVALID_HANDLE,        the handle is invalid.
    ST_ERROR_ALREADY_INITIALIZED      Already Registered
    ST_ERROR_NO_MEMORY              No place to register
See Also:
    MemPool_PutOpBlk()
     MemPool_GetIpBlk()
     MemPool_FreeIpBlk()
*****************************************************************************/
ST_ErrorCode_t MemPool_GetOpBlk(STMEMBLK_Handle_t Handle, U32  *AddressToBlkPtr_p)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    MemoryPoolControl_t *Pool_p = (MemoryPoolControl_t *)Handle;

    /* Only continue if we have a control block for this device */
    if (Pool_p != NULL)
    {
        STOS_MutexLock(Pool_p->Lock_p);
        if(Pool_p->Start)
        {

            Block_t * Block_p = NULL;

            *AddressToBlkPtr_p = (U32)NULL;

            if(Pool_p->BlkForPro_p)
            {
                BlockList_t * Blk_p = Pool_p->BlkForPro_p;

                while(!Blk_p->Blk_p->Free)
                {
                    Blk_p = Blk_p->Next_p;
                    if(Blk_p == NULL)
                    {
                        break;
                    }
                }

                if(Blk_p)
                {
                    Block_p = Blk_p->Blk_p;
                    /*Block is free to be sent*/
                    Block_p->Free = FALSE;
                    Block_p->Block.Flags = 0;
                    *AddressToBlkPtr_p = (U32)&(Block_p->Block);
                    Pool_p->Request[0] = FALSE;
                    Pool_p->NextFree++;
                }
                else
                {
                    Pool_p->Request[0] = TRUE; /*trigger when next available*/
                    error = ST_ERROR_NO_MEMORY;
                }
            }
            else
            {
                Pool_p->Request[0] = TRUE; /*trigger when next available*/
                error = ST_ERROR_NO_MEMORY;
            }
        }
        else
        {
            /*Not yet started*/
            *AddressToBlkPtr_p = (U32)NULL;
            Pool_p->Request[0] = TRUE; /*trigger when next available*/
            error = ST_ERROR_NO_MEMORY;
            STTBX_Print(("OP Not yet started\n"));
        }
        STOS_MutexRelease(Pool_p->Lock_p);
    }
    else
    {
        error = ST_ERROR_INVALID_HANDLE;
    }

    return error;
}/*MemPool_GetOpBlk()*/


/*****************************************************************************
MemPool_ReturnOpBlk()
Description:
    Returns an unfilled  block to the Mem Pool.Can be used later
Parameters:
    Handle, handle to the Mem Pool.
     AddressToBlkPtr_p, unfilled block to be returned.

Return Value:
    ST_NO_ERROR,                    no errors.
    ST_ERROR_UNKNOWN_DEVICE,        invalid device name.
    ST_ERROR_INVALID_HANDLE,        the handle is invalid.
    ST_ERROR_ALREADY_INITIALIZED      Already Registered
    ST_ERROR_NO_MEMORY              No place to register
See Also:
     MemPool_GetIpBlk()
     MemPool_GetOpBlk()
     MemPool_PutOpBlk()
     MemPool_FreeIpBlk()
*****************************************************************************/
ST_ErrorCode_t MemPool_ReturnOpBlk(STMEMBLK_Handle_t Handle, U32  *AddressToBlkPtr_p)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    MemoryPoolControl_t *Pool_p = (MemoryPoolControl_t *)Handle;

    /* Only continue if we have a control block for this device */
    if (Pool_p != NULL)
    {
        STOS_MutexLock(Pool_p->Lock_p);
        if(Pool_p->BlkForPro_p)
        {
            BlockList_t * Blk_p = Pool_p->BlkForPro_p;
            Block_t * Block_p = Blk_p->Blk_p;

            while(*AddressToBlkPtr_p != ((U32)&(Block_p->Block)))
            {
                Blk_p = Blk_p->Next_p;

                if(Blk_p == NULL)
                {
                    break;
                }
                Block_p = Blk_p->Blk_p;
            }

            if(Blk_p && !Block_p->Free)
            {
                Block_p->Free = TRUE;
                Block_p->Block.Flags = 0;
                Pool_p->NextFree--;
            }
            else
            {
                error = ST_ERROR_BAD_PARAMETER;
            }
        }
        else
        {
            error = ST_ERROR_BAD_PARAMETER;
        }

        STOS_MutexRelease(Pool_p->Lock_p);
    }
    else
    {
        error = ST_ERROR_INVALID_HANDLE;
    }

    return error;
}/*MemPool_ReturnOpBlk()*/

/*****************************************************************************
MemPool_PutOpBlk()
Description:
    Puts a filled block to the Mem Pool.
Parameters:
    Handle, handle to the Mem Pool.
     AddressToBlkPtr_p, filled block to be submitted.

Return Value:
    ST_NO_ERROR,                    no errors.
    ST_ERROR_UNKNOWN_DEVICE,        invalid device name.
    ST_ERROR_INVALID_HANDLE,        the handle is invalid.
    ST_ERROR_ALREADY_INITIALIZED      Already Registered
    ST_ERROR_NO_MEMORY              No place to register
See Also:
     MemPool_GetIpBlk()
     MemPool_GetOpBlk()
     MemPool_FreeIpBlk()
*****************************************************************************/
ST_ErrorCode_t MemPool_PutOpBlk(STMEMBLK_Handle_t Handle, U32  *AddressToBlkPtr_p)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    MemoryPoolControl_t *Pool_p = (MemoryPoolControl_t *)Handle;
    U32 ConsumerCount = 1;

    /* Only continue if we have a control block for this device */
    if (Pool_p != NULL)
    {
        Block_t * Block_p = NULL;
        STOS_MutexLock(Pool_p->Lock_p);

        if(Pool_p->BlkForPro_p)
        {
            /*Pick this block and put into the consumer block*/
            //U32 Offset = 0;
            BlockList_t * Blk_p = Pool_p->BlkForPro_p;
            BlockList_t * PrevBlk_p = NULL;
            Block_p = Blk_p->Blk_p;

            while(*AddressToBlkPtr_p != (U32)&(Block_p->Block))
            {
                PrevBlk_p = Blk_p;
                Blk_p = Blk_p->Next_p;

                if(Blk_p == NULL)
                {
                    break;
                }
                Block_p = Blk_p->Blk_p;
            }

            if(Blk_p && !Block_p->Free)
            {
                BlockList_t * BlkCon_p = NULL;;
                //Block_p = Blk_p->Blk_p;
                if(Pool_p->NumConsumer)
                {
                    /*find the position in the Consumer queue*/
                    if(Pool_p->BlkForCon_p)
                    {
                        /*already some members in the consumer block*/
                        /*Insert this new element in the last position*/
                        BlkCon_p = Pool_p->BlkForCon_p;
                        while(BlkCon_p->Next_p)
                        {
                            BlkCon_p = BlkCon_p->Next_p;
                        }

                        BlkCon_p->Next_p = Blk_p;
                    }
                    else
                    {
                        Pool_p->BlkForCon_p = Blk_p;
                    }
                    /*Now remove it from the Producer queue*/
                    if(PrevBlk_p)
                    {
                        PrevBlk_p->Next_p = Blk_p->Next_p;
                    }
                    else
                    {
                        Pool_p->BlkForPro_p = Blk_p->Next_p;
                    }

                    Blk_p->Next_p = NULL;
                    Pool_p->ProdQSize--;
                    Pool_p->ConQSize++;

                    /*fill up the parametes*/

                    Block_p->ConsumerCount = Pool_p->NumConsumer; /*No of consumers. It can be 0*/
                    Block_p->Filled = TRUE;
                    Pool_p->NextFilled++;

                    /*trigger all the consumers waiting*/
                    ConsumerCount = Pool_p->NumConsumer;
                    while(ConsumerCount)
                    {
                        Block_p->Fill[ConsumerCount] = TRUE; /*TRUE for all currently registered consumers */
#if MEMORY_SPLIT_SUPPORT
                        //Block_p->SplitCount[ConsumerCount] = Pool_p->NumSplit;
                        Block_p->SplitCount[ConsumerCount] = Pool_p->SplitSize ? (Block_p->Block.Size / Pool_p->SplitSize):1; /*split number*/
                        Block_p->SplitSent[ConsumerCount] = 0;
#endif
                        if(Pool_p->Request[ConsumerCount])
                        {
                            Pool_p->Request[ConsumerCount] = FALSE;
                            if(Pool_p->Logged[ConsumerCount].sem)
                            {
                                STOS_SemaphoreSignal(Pool_p->Logged[ConsumerCount].sem);
                            }
                        }
                        ConsumerCount--;
                    }
                }
                else
                {
                    /*Check if it is dirty . then free it too*/
                    if(Block_p->Block.Flags & BUFFER_DIRTY)
                    {
                        /*call the actual producer to release it*/
                        error = MemPool_FreeIpBlk(Block_p->Block.Data[CONSUMER_HANDLE], (U32*)&(Block_p->Block.AppData_p), Pool_p->Logged[0].Handle);
                    }
                    /*There are no consumers so don't put in consumer list*/
                    /*mark it free too*/
                    Block_p->Free = TRUE;
                    Block_p->ConsumerCount = 0;
                    if(Pool_p->Request[0])
                    {
                        Pool_p->Request[0] = FALSE;
                        STOS_SemaphoreSignal(Pool_p->Logged[0].sem);
                    }


                }

            }
            else
            {
                error =  ST_ERROR_BAD_PARAMETER;
            }
        }
        else
        {
            error =  ST_ERROR_BAD_PARAMETER;
        }

        STOS_MutexRelease(Pool_p->Lock_p);
    }
    else
    {
        error = ST_ERROR_INVALID_HANDLE;
    }

    return error;
} /*MemPool_PutOpBlk()*/

/*****************************************************************************
MemPool_GetIpBlk()
Description:
    Registers the producer to the Mem Pool.
Parameters:
    Handle, handle to the Mem Pool.
     AddressToBlkPtr_p, filled Block to get.
     ConHandle, Consumer handle

Return Value:
    ST_NO_ERROR,                    no errors.
    ST_ERROR_UNKNOWN_DEVICE,        invalid device name.
    ST_ERROR_INVALID_HANDLE,        the handle is invalid.
    ST_ERROR_ALREADY_INITIALIZED      Already Registered
    ST_ERROR_NO_MEMORY              No place to register
See Also:
    MemPool_PutOpBlk()
     MemPool_GetOpBlk()
     MemPool_FreeIpBlk()
*****************************************************************************/
ST_ErrorCode_t MemPool_GetIpBlk(STMEMBLK_Handle_t Handle, U32  *AddressToBlkPtr_p, U32 ConHandle)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    MemoryPoolControl_t *Pool_p = (MemoryPoolControl_t *)Handle;
    U32 ConsumerCount=1;

    /* Only continue if we have a control block for this device */
    if (Pool_p != NULL)
    {
        STOS_MutexLock(Pool_p->Lock_p);

        ConsumerCount = Pool_p->NumConsumer;
        while((Pool_p->Logged[ConsumerCount].Handle != ConHandle) && ConsumerCount)
        {
            ConsumerCount--;
        }


        if((Pool_p->Logged[ConsumerCount].Handle == ConHandle) && ConsumerCount)
        {
            /*Return if block has not started*/
            if(Pool_p->Start)
            {
                Block_t * Block_p = NULL;

                *AddressToBlkPtr_p = (U32)NULL;
                /*search for the block in the consumer queue*/

                if(Pool_p->BlkForCon_p)
                {
                    BlockList_t * Blk_p = Pool_p->BlkForCon_p;

                    Block_p = Blk_p->Blk_p;
                    while(!Block_p->Fill[ConsumerCount])
                    {
                        Blk_p = Blk_p->Next_p;

                        if(Blk_p == NULL)
                        {
                            break;
                        }
                        Block_p = Blk_p->Blk_p;
                    }

                    if(Blk_p)
                    {

                        /*check if this block is dirty*/
                        if(Block_p->Block.Flags & BUFFER_DIRTY)
                        {
                            /*forward the previous block*/
                            *AddressToBlkPtr_p = (U32) Block_p->Block.AppData_p;
                        }
                        else
                        {
                            /*A filled block is available*/
                            *AddressToBlkPtr_p = (U32)&(Block_p->Block);
                        }

                        Pool_p->Request[ConsumerCount] = FALSE;
                        Block_p->Fill[ConsumerCount] = FALSE;
                        Block_p->InUse[ConsumerCount] = TRUE;
                        Pool_p->NextSent[ConsumerCount]++;

                    }
                    else
                    {
                        Pool_p->Request[ConsumerCount] = TRUE; /*trigger when next available*/
                        error = ST_ERROR_NO_MEMORY;
                    }
                }
                else
                {
                    /*no block yet */

                    Pool_p->Request[ConsumerCount] = TRUE; /*trigger when next available*/
                    error = ST_ERROR_NO_MEMORY;
                }
            }
            else
            {
                /*not yet started*/
                *AddressToBlkPtr_p = (U32)NULL;
                Pool_p->Request[ConsumerCount] = TRUE; /*trigger when next available*/
                STTBX_Print(("IP Not yet started\n"));
                error = ST_ERROR_NO_MEMORY;
            }
        }
        else
        {
            error = ST_ERROR_INVALID_HANDLE;
        }

        STOS_MutexRelease(Pool_p->Lock_p);
    }
    else
    {
        error = ST_ERROR_INVALID_HANDLE;
    }

    return error;
}/*MemPool_GetIpBlk()*/

/*****************************************************************************
MemPool_GetSplitIpBlk()
Description:
    Registers the producer to the Mem Pool.
Parameters:
    Handle, handle to the Mem Pool.
     AddressToBlkPtr_p, filled Block to get.
     ConHandle, Consumer handle
     Offset, offset

Return Value:
    ST_NO_ERROR,                    no errors.
    ST_ERROR_UNKNOWN_DEVICE,        invalid device name.
    ST_ERROR_INVALID_HANDLE,        the handle is invalid.
    ST_ERROR_ALREADY_INITIALIZED      Already Registered
    ST_ERROR_NO_MEMORY              No place to register
See Also:
    MemPool_PutOpBlk()
     MemPool_GetOpBlk()
     MemPool_FreeIpBlk()
*****************************************************************************/
#if MEMORY_SPLIT_SUPPORT
ST_ErrorCode_t MemPool_GetSplitIpBlk(STMEMBLK_Handle_t Handle, U32  *AddressToBlkPtr_p, U32 ConHandle,MemoryParams_t * MemoryParams_p)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    MemoryPoolControl_t *Pool_p = (MemoryPoolControl_t *)Handle;
    U32 ConsumerCount=1;

    /* Only continue if we have a control block for this device */
    if (Pool_p != NULL)
    {
        STOS_MutexLock(Pool_p->Lock_p);

        ConsumerCount = Pool_p->NumConsumer;
        while((Pool_p->Logged[ConsumerCount].Handle != ConHandle) && ConsumerCount)
        {
            ConsumerCount--;
        }


        if((Pool_p->Logged[ConsumerCount].Handle == ConHandle) && ConsumerCount)
        {
            /*Return if block has not started*/
            if(Pool_p->Start)
            {
                Block_t * Block_p = NULL;

                *AddressToBlkPtr_p = (U32)NULL;
                /*search for the block in the consumer queue*/

                if(Pool_p->BlkForCon_p)
                {
                    BlockList_t * Blk_p = Pool_p->BlkForCon_p;

                    Block_p = Blk_p->Blk_p;
                    while(!Block_p->Fill[ConsumerCount])
                    {
                        Blk_p = Blk_p->Next_p;

                        if(Blk_p == NULL)
                        {
                            break;
                        }
                        Block_p = Blk_p->Blk_p;
                    }

                    if(Blk_p)
                    {

                        /*check if this block is dirty*/
                        if(Block_p->Block.Flags & BUFFER_DIRTY)
                        {
                            /*forward the previous block*/
                            *AddressToBlkPtr_p = (U32) Block_p->Block.AppData_p;
                        }
                        else
                        {
                            /*A filled block is available*/
                            *AddressToBlkPtr_p = (U32)&(Block_p->Block);
                        }

                        {
                            MemoryBlock_t *MemBlock_p = (MemoryBlock_t *)*AddressToBlkPtr_p;
                            //U32 SizePerSplit = MemBlock_p->Size/Pool_p->NumSplit;
                            U32 SizePerSplit = Pool_p->SplitSize?Pool_p->SplitSize:MemBlock_p->Size;

                            MemoryParams_p->MemoryStart = (MemBlock_p->MemoryStart + (Block_p->SplitSent[ConsumerCount]*SizePerSplit));
                            MemoryParams_p->Size = SizePerSplit;

                            /*send DUMMYBLOCK_VALID in all blocks*/
                            /*send EOF_VALID only in the last block*/
                            MemoryParams_p->Flags = Block_p->SplitSent[ConsumerCount]?
                             (MemBlock_p->Flags & DUMMYBLOCK_VALID):(MemBlock_p->Flags & (~EOF_VALID));

                            Block_p->SplitSent[ConsumerCount]++;

                            if(Block_p->SplitSent[ConsumerCount] == Block_p->SplitCount[ConsumerCount])
                            {
                                /*send EOF in the last block*/
                                MemoryParams_p->Flags |= (MemBlock_p->Flags & EOF_VALID);

                                Block_p->Fill[ConsumerCount] = FALSE;
                                Block_p->InUse[ConsumerCount] = TRUE;
                                Pool_p->NextSent[ConsumerCount]++;
                            }
                        }

                        Pool_p->Request[ConsumerCount] = FALSE;
                    }
                    else
                    {
                        Pool_p->Request[ConsumerCount] = TRUE; /*trigger when next available*/
                        error = ST_ERROR_NO_MEMORY;
                    }
                }
                else
                {
                    /*no block yet */

                    Pool_p->Request[ConsumerCount] = TRUE; /*trigger when next available*/
                    error = ST_ERROR_NO_MEMORY;
                }
            }
            else
            {
                /*not yet started*/
                *AddressToBlkPtr_p = (U32)NULL;
                Pool_p->Request[ConsumerCount] = TRUE; /*trigger when next available*/
                STTBX_Print(("IP Not yet started\n"));
                error = ST_ERROR_NO_MEMORY;
            }
        }
        else
        {
            error = ST_ERROR_INVALID_HANDLE;
        }

        STOS_MutexRelease(Pool_p->Lock_p);
    }
    else
    {
        error = ST_ERROR_INVALID_HANDLE;
    }

    return error;
}/*MemPool_GetIpBlk()*/
#endif
/*****************************************************************************
MemPool_ReturnIpBlk()
Description:
    Returns an unused block to the Mem Pool.Can be asked for later
Parameters:
    Handle, handle to the Mem Pool.
     AddressToBlkPtr_p, unused block to be returned.

Return Value:
    ST_NO_ERROR,                    no errors.
    ST_ERROR_UNKNOWN_DEVICE,        invalid device name.
    ST_ERROR_INVALID_HANDLE,        the handle is invalid.
    ST_ERROR_ALREADY_INITIALIZED      Already Registered
    ST_ERROR_NO_MEMORY              No place to register
See Also:
     MemPool_GetIpBlk()
     MemPool_GetOpBlk()
     MemPool_PutOpBlk()
     MemPool_FreeIpBlk()
*****************************************************************************/
ST_ErrorCode_t MemPool_ReturnIpBlk(STMEMBLK_Handle_t Handle, U32 * AddressToBlkPtr_p, U32 ConHandle)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    MemoryPoolControl_t *Pool_p = (MemoryPoolControl_t *)Handle;
    U32 ConsumerCount = 1;

    /* Only continue if we have a control block for this device */
    if (Pool_p != NULL)
    {
        STOS_MutexLock(Pool_p->Lock_p);
        ConsumerCount = Pool_p->NumConsumer;
        while((Pool_p->Logged[ConsumerCount].Handle != ConHandle) && ConsumerCount)
        {
            ConsumerCount--;
        }


        if((Pool_p->Logged[ConsumerCount].Handle == ConHandle) && ConsumerCount)
        {
            /*Return if block has not started*/
            if(Pool_p->Start)
            {
                if(Pool_p->BlkForCon_p)
                {
                    BlockList_t * Blk_p = Pool_p->BlkForCon_p;
                    Block_t * Block_p = Blk_p->Blk_p;

                    while(*AddressToBlkPtr_p != (U32)&(Block_p->Block))
                    {
                        Blk_p = Blk_p->Next_p;

                        if(Blk_p == NULL)
                        {
                            break;
                        }
                        Block_p = Blk_p->Blk_p;
                    }

                    if(Blk_p && !Block_p->Fill[ConsumerCount])
                    {
                        Block_p->Fill[ConsumerCount] = TRUE;
                        Pool_p->NextSent[ConsumerCount]--;
                    }
                    else
                    {
                        error =  ST_ERROR_BAD_PARAMETER;
                    }
                }
                else
                {
                    error =  ST_ERROR_BAD_PARAMETER;
                }
            }
            else
            {
                error =  ST_ERROR_BAD_PARAMETER;
            }
        }
        else
        {
            error =  ST_ERROR_BAD_PARAMETER;
        }

        STOS_MutexRelease(Pool_p->Lock_p);
    }
    else
    {
        error = ST_ERROR_INVALID_HANDLE;
    }

    return error;
}/*MemPool_ReturnIpBlk()*/

/*****************************************************************************
MemPool_FreeIpBlk()
Description:
    Registers the producer to the Mem Pool.
Parameters:
    Handle, handle to the Mem Pool.
     AddressToBlkPtr_p, consumed Block to be release.
     ConHandle, Consumer handle

Return Value:
    ST_NO_ERROR,                    no errors.
    ST_ERROR_UNKNOWN_DEVICE,        invalid device name.
    ST_ERROR_INVALID_HANDLE,        the handle is invalid.
    ST_ERROR_ALREADY_INITIALIZED      Already Registered
    ST_ERROR_NO_MEMORY              No place to register
See Also:
     MemPool_GetIpBlk()
    MemPool_PutOpBlk()
     MemPool_GetOpBlk()
*****************************************************************************/
ST_ErrorCode_t MemPool_FreeIpBlk(STMEMBLK_Handle_t Handle, U32  *AddressToBlkPtr_p, U32 ConHandle)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    MemoryPoolControl_t *Pool_p = (MemoryPoolControl_t *)Handle;
    U32 ConsumerCount=1;

    /* Only continue if we have a control block for this device */
    if (Pool_p != NULL)
    {
        STOS_MutexLock(Pool_p->Lock_p);

        ConsumerCount = Pool_p->NumConsumer;
        while((Pool_p->Logged[ConsumerCount].Handle != ConHandle) && ConsumerCount)
        {
            ConsumerCount--;
        }

        if((Pool_p->Logged[ConsumerCount].Handle == ConHandle) && ConsumerCount)
        {
            Block_t * Block_p = NULL;

            /*Find the block which needs to be freed*/
            if(Pool_p->BlkForCon_p)
            {
                BlockList_t * Blk_p = Pool_p->BlkForCon_p;
                BlockList_t * PrevBlk_p = NULL;
                Block_p = Blk_p->Blk_p;
                while(((U32)&Block_p->Block) != *AddressToBlkPtr_p)
                {
                    /*check if this block is dirty*/
                    if(Block_p->Block.Flags & BUFFER_DIRTY)
                    {
                        /*check if the application pointer is the block returned*/
                        if((U32)Block_p->Block.AppData_p == *AddressToBlkPtr_p)
                        {
                            break;
                        }
                    }

                    PrevBlk_p = Blk_p;
                    Blk_p = Blk_p->Next_p;
                    if(Blk_p == NULL)
                    {
                        break;
                    }
                    Block_p = Blk_p->Blk_p;
                }

                if(Blk_p && Block_p->InUse[ConsumerCount])
                {
#if MEMORY_SPLIT_SUPPORT
                    if(Block_p->SplitCount[ConsumerCount])
                    {
                        Block_p->SplitCount[ConsumerCount]--;
                    }

                    if(Block_p->SplitCount[ConsumerCount] == 0)
#endif
                    {
                        Block_p->InUse[ConsumerCount] = FALSE;
                        Block_p->ConsumerCount--;
                    }
                    if(Block_p->ConsumerCount == 0)
                    {
                        BlockList_t * BlkProd_p = NULL;
                        /*if dirty then free from the actual BM*/
                        if(Block_p->Block.Flags & BUFFER_DIRTY)
                        {
                            /*call the actual producer to release it*/
                            error = MemPool_FreeIpBlk(Block_p->Block.Data[CONSUMER_HANDLE], (U32*)&(Block_p->Block.AppData_p), Pool_p->Logged[0].Handle);
                        }

                        if(!Pool_p->NoProducer)
                        {
                            /*Block is consumed so free it*/
                            Block_p->Filled = FALSE;
                            Block_p->Free = TRUE;
                            /*Add this to the producer queue*/
                            if(Pool_p->BlkForPro_p)
                            {
                                BlkProd_p = Pool_p->BlkForPro_p;
                                while(BlkProd_p->Next_p)
                                {
                                    BlkProd_p = BlkProd_p->Next_p;
                                }

                                BlkProd_p->Next_p = Blk_p;
                            }
                            else
                            {
                                Pool_p->BlkForPro_p = Blk_p;
                            }

                            /*remove from consumer queue */
                            if(PrevBlk_p)
                            {
                                PrevBlk_p->Next_p = Blk_p->Next_p;
                            }
                            else
                            {
                                Pool_p->BlkForCon_p = Blk_p->Next_p;
                            }
                            Blk_p->Next_p = NULL;
                            Pool_p->ProdQSize++;
                            Pool_p->ConQSize--;

                            /*trigger producer if it is waiting*/
                            if(Pool_p->Request[0])
                            {
                                Pool_p->Request[0] = FALSE;
                                if(Pool_p->Logged[0].sem)
                                {
                                    STOS_SemaphoreSignal(Pool_p->Logged[0].sem);
                                }
                            }
                        }
                        else
                        {
                            U32 i=1;
                            /*Block is consumed so fill it*/
                            Block_p->Filled = TRUE;
                            /*put it at last of the queue*/
                            /*remove from consumer queue and add it to the producer queue*/
                            if(PrevBlk_p)
                            {
                                PrevBlk_p->Next_p = Blk_p->Next_p;
                                while(PrevBlk_p->Next_p)
                                {
                                    PrevBlk_p = PrevBlk_p->Next_p;
                                }
                                Blk_p->Next_p = NULL;
                            }

                            /*trigger all the consumers waiting*/
                            while(i < Pool_p->NumConsumer)
                            {
                                if(Pool_p->Request[i])
                                {
                                    Pool_p->Request[i] = FALSE;
                                    if(Pool_p->Logged[i].sem)
                                    {
                                        STOS_SemaphoreSignal(Pool_p->Logged[i].sem);
                                    }
                                }
                                i++;
                            }
                        }/*if(!Pool_p->NoProducer)*/
                    }

                    Pool_p->NextRelease[ConsumerCount]++;
                }
                else
                {
                    error = ST_ERROR_BAD_PARAMETER;
                }
            }
            else
            {
                /*no block yet */
                error = ST_ERROR_BAD_PARAMETER;
            }
        }
        else
        {
            error = ST_ERROR_INVALID_HANDLE;
        }

        STOS_MutexRelease(Pool_p->Lock_p);
    }
    else
    {
        error = ST_ERROR_INVALID_HANDLE;
    }

    return error;
}/*MemPool_FreeIpBlk()*/


/*****************************************************************************
MemPool_Flush()
Description:
    Flushes the buffers of the Mem Pool.
Parameters:
    Handle, handle to the Mem Pool.
     Block, filled block to be submitted.

Return Value:
    ST_NO_ERROR,                    no errors.
    ST_ERROR_UNKNOWN_DEVICE,        invalid device name.
    ST_ERROR_INVALID_HANDLE,        the handle is invalid.
    ST_ERROR_ALREADY_INITIALIZED      Already Registered

See Also:
     MemPool_Term()
     MemPool_Init()
*****************************************************************************/
ST_ErrorCode_t MemPool_Flush(STMEMBLK_Handle_t Handle)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    MemoryPoolControl_t *Pool_p;

    if (InitMutex_p != NULL)
    {
        STOS_MutexLock(InitMutex_p);
    }
    else
    {
        return ST_ERROR_BAD_PARAMETER;
    }

    /* Obtain the Mem Pool control block from the passed in handle */
    Pool_p = MemPool_GetControlBlockFromHandle(Handle);

    STOS_MutexRelease(InitMutex_p);

    /* Only continue if we have a control block for this device */
    if (Pool_p != NULL)
    {
        U32 i,j;
        STOS_MutexLock(Pool_p->Lock_p);

        if(Pool_p->NextFree != Pool_p->NextFilled)
            STTBX_Print(("BM: Blocks with producer\n"));

        Pool_p->NextFree = 0;
        Pool_p->NextFilled = 0;
#if MEMORY_SPLIT_SUPPORT
        Pool_p->SplitSize = 0;
#endif
#ifndef MSPP_PARSER
        memset(Pool_p->Buffer_p,0,Pool_p->TotalSize);
#endif

        for(i = 0;i < Pool_p->NumBlocks; i++)
        {
            Block_t         * Block_p = &(Pool_p->Block_p[i]);
            MemoryBlock_t   * MemBlock_p = &(Block_p->Block);

            Block_p->Free = TRUE;
            Block_p->ConsumerCount = 0;

            Block_p->Filled = (Pool_p->NoProducer)?TRUE:FALSE;

            for(j = 0;j < MAX_LOGGING_ENTRIES;j++ )
            {
                Block_p->Fill[j] = (Pool_p->NoProducer)?TRUE:FALSE;
            }

            MemBlock_p->Size = 0;
            MemBlock_p->Flags = 0;
            MemBlock_p->AppData_p = NULL;


            for(j = 0;j < 32; j++ )
            {
                MemBlock_p->Data[j] = 0;
            }

            /*put all the blocks in the free list*/

            Pool_p->BlkList_p[i].Blk_p = &(Pool_p->Block_p[i]);

            if(i < (U32)(Pool_p->NumBlocks-1))
            {
                Pool_p->BlkList_p[i].Next_p = &(Pool_p->BlkList_p[i+1]);
            }
            else
            {
                Pool_p->BlkList_p[i].Next_p = NULL;
            }
        }

        Pool_p->BlkForPro_p = Pool_p->BlkList_p;
        Pool_p->BlkForCon_p = NULL;
        Pool_p->ConQSize  = 0;
        Pool_p->ProdQSize = 0;

        for(i = 0;i < MAX_LOGGING_ENTRIES; i++)
        {
            if(Pool_p->NextSent[i] != Pool_p->NextRelease[i])
                STTBX_Print(("BM:blocks with consumer %d\n",i));
            Pool_p->NextSent[i] = 0;
            Pool_p->NextRelease[i] = 0;
            Pool_p->Request[i] = FALSE;
        }

        Pool_p->Start = FALSE;
        STOS_MutexRelease(Pool_p->Lock_p);
    }
    return error;
} /*MemPool_Flush()*/

/*****************************************************************************
MemPool_UnRegProducer()
Description:
    UnRegisters the producer to the Mem Pool.
Parameters:
    Handle, handle to the Mem Pool.
     ConHandle, Consumer Handle.

Return Value:
    ST_NO_ERROR,                    no errors.
    ST_ERROR_INVALID_HANDLE,        the handle is invalid.
    ST_ERROR_BAD_PARAMETER            Bad parameter
See Also:
    MemPool_UnRegConsumer()
     MemPool_RegProducer()
*****************************************************************************/
ST_ErrorCode_t MemPool_UnRegProducer(STMEMBLK_Handle_t Handle, U32 ProdHandle)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    MemoryPoolControl_t *Pool_p;

    if ((InitMutex_p != NULL) && ProdHandle)
    {
        STOS_MutexLock(InitMutex_p);
    }
    else
    {
        return ST_ERROR_BAD_PARAMETER;
    }

    /* Obtain the Mem Pool control block from the passed in handle */
    Pool_p = MemPool_GetControlBlockFromHandle(Handle);

    STOS_MutexRelease(InitMutex_p);

    /* Only continue if we have a control block for this device */
    if (Pool_p != NULL)
    {
        STOS_MutexLock(Pool_p->Lock_p);

        if (Pool_p->Logged[0].Handle == ProdHandle)
        {
            // Invalidate producer block
            Pool_p->Logged[0].Handle = 0;
            Pool_p->Logged[0].sem = 0;

            Pool_p->LoggedEntries --;
            Pool_p->NumProducer --;
        }
        else
        {
            error = ST_ERROR_INVALID_HANDLE;
        }

        STOS_MutexRelease(Pool_p->Lock_p);
    }
    else
    {
        error = ST_ERROR_INVALID_HANDLE;
    }

    return error;


}

/*****************************************************************************
MemPool_UnRegConsumer()
Description:
    UnRegisters the producer to the Mem Pool.
Parameters:
    Handle, handle to the Mem Pool.
     ConHandle, Producer Handle.

Return Value:
    ST_NO_ERROR,                    no errors.
    ST_ERROR_INVALID_HANDLE,        the handle is invalid.
    ST_ERROR_BAD_PARAMETER            Bad parameter
See Also:
    MemPool_UnRegProducer()
     MemPool_RegProducer()
*****************************************************************************/
ST_ErrorCode_t MemPool_UnRegConsumer(STMEMBLK_Handle_t Handle , U32 ConHandle)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    MemoryPoolControl_t *Pool_p;
    U32 ConsumerCount=1;

    if ((InitMutex_p != NULL) && ConHandle)
    {
        STOS_MutexLock(InitMutex_p);
    }
    else
    {
        return ST_ERROR_BAD_PARAMETER;
    }

    /* Obtain the Mem Pool control block from the passed in handle */
    Pool_p = MemPool_GetControlBlockFromHandle(Handle);

    STOS_MutexRelease(InitMutex_p);

    /* Only continue if we have a control block for this device */
    if (Pool_p != NULL)
    {
        STOS_MutexLock(Pool_p->Lock_p);
        ConsumerCount = Pool_p->NumConsumer;

        while((Pool_p->Logged[ConsumerCount].Handle != ConHandle) && ConsumerCount)
        {
            ConsumerCount--;
        }

        if ((Pool_p->Logged[ConsumerCount].Handle == ConHandle) && ConsumerCount)
        {
            /* Valid Consumer is unregistering. Move last registered consumer to current location*/

            /*move the last consumer if available to this offset*/
            {
                U32 LastConsumer = Pool_p->NumConsumer;
                U32 i=0;

                /*release up all the blocks for this consumer that were unused*/

                if(Pool_p->BlkForCon_p)
                {
                    BlockList_t * Blk_p = Pool_p->BlkForCon_p;
                    BlockList_t * PrevBlk_p = NULL;
                    Block_t * Block_p = Blk_p->Blk_p;
                    while(Block_p)
                    {
                        if(Block_p->Fill[ConsumerCount])
                        {
                            Block_p->Fill[ConsumerCount] = FALSE;
                            Block_p->ConsumerCount--;
                            /*If consumer count of this block is zero then free up this block*/
                            if(Block_p->ConsumerCount == 0)
                            {
                                /*Block is consumed so free it*/
                                Block_p->Filled = FALSE;
                                Block_p->Free = TRUE;
                                /*Remove from producer queue*/
                                if(PrevBlk_p)
                                {
                                    PrevBlk_p->Next_p = Blk_p->Next_p;
                                }
                                else
                                {
                                    Pool_p->BlkForCon_p = Blk_p->Next_p;
                                }

                                /*Add to Producer queue*/
                                if(Pool_p->BlkForPro_p)
                                {
                                    BlockList_t * BlkProd_p = Pool_p->BlkForPro_p;
                                    while(BlkProd_p->Next_p)
                                    {
                                        BlkProd_p = BlkProd_p->Next_p;
                                    }

                                    BlkProd_p->Next_p = Blk_p;
                                    Blk_p = Blk_p->Next_p;
                                    BlkProd_p->Next_p->Next_p = NULL;
                                }
                                else
                                {
                                    Pool_p->BlkForPro_p = Blk_p;
                                    Blk_p = Blk_p->Next_p;
                                    Pool_p->BlkForPro_p->Next_p = NULL;
                                }

                                Pool_p->ProdQSize++;
                                Pool_p->ConQSize--;
                            }
                        }
                        else
                        {
                            PrevBlk_p = Blk_p;
                            Blk_p = Blk_p->Next_p;
                        }

                        if(Blk_p == NULL)
                        {
                            break;
                        }
                        Block_p = Blk_p->Blk_p;
                    }
                }

                if(Pool_p->Request[ConsumerCount])
                {
                    Pool_p->Request[ConsumerCount] = FALSE;
                    if(Pool_p->Logged[ConsumerCount].sem)
                    {
                        STOS_SemaphoreSignal(Pool_p->Logged[ConsumerCount].sem);
                    }
                }

                // Invalidate Last consumer block
                Pool_p->Logged[ConsumerCount].Handle = 0;
                Pool_p->Logged[ConsumerCount].sem = 0;

                Pool_p->NextSent[ConsumerCount]     = 0;
                Pool_p->NextRelease[ConsumerCount]  = 0;

                if(LastConsumer && (LastConsumer > ConsumerCount))
                {
                    /*This is the last consumer and is located at a position beyond the unregistering consumer*/
                    Pool_p->Logged[ConsumerCount].Handle = Pool_p->Logged[LastConsumer].Handle;
                    Pool_p->Logged[ConsumerCount].sem    = Pool_p->Logged[LastConsumer].sem;
                    Pool_p->Logged[LastConsumer].Handle     = 0;
                    Pool_p->Logged[LastConsumer].sem        = 0;
                    /*copy all other parameters too*/
                    Pool_p->Request[ConsumerCount]          = Pool_p->Request[LastConsumer];
                    Pool_p->NextRelease[ConsumerCount]      = Pool_p->NextRelease[LastConsumer];
                    Pool_p->NextSent[ConsumerCount]             = Pool_p->NextSent[LastConsumer];

                    Pool_p->Request[LastConsumer]           = FALSE;
                    Pool_p->NextRelease[LastConsumer]       = 0;
                    Pool_p->NextSent[LastConsumer]          = 0;
                    for(i=0;i<Pool_p->NumBlocks;i++)
                    {
                        Pool_p->Block_p[i].Fill[ConsumerCount] = Pool_p->Block_p[i].Fill[LastConsumer];
                    }
                }



            }

            Pool_p->LoggedEntries--;
            Pool_p->NumConsumer--;
        }
        else
        {
            error = ST_ERROR_INVALID_HANDLE;
        }

        STOS_MutexRelease(Pool_p->Lock_p);
    }
    else
    {
        error = ST_ERROR_INVALID_HANDLE;
    }

    return error;
}

/*****************************************************************************
MemPool_Start()
Description:
    ReInitializes the named Mem Pool.

Parameters:
    Handle, handle to the Mem Pool.
Return Value:
    ST_NO_ERROR,                    no errors.
    ST_ERROR_BAD_PARAMETER,         one or more of the parameters was invalid.
    ST_ERROR_INVALID_HANDLE,        invalid handle .

See Also:
    MemPool_Term()
*****************************************************************************/

ST_ErrorCode_t MemPool_Start(STMEMBLK_Handle_t Handle)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    MemoryPoolControl_t *Pool_p;
    U32 i = 0;
    U32 ConsumerCount = 1;

    if (InitMutex_p != NULL)
    {
        STOS_MutexLock(InitMutex_p);
    }
    else
    {
        return ST_ERROR_BAD_PARAMETER;
    }

    /* Obtain the Mem Pool control block from the passed in handle */
    Pool_p = MemPool_GetControlBlockFromHandle(Handle);

    STOS_MutexRelease(InitMutex_p);

    /* Only continue if we have a control block for this device */
    if (Pool_p != NULL)
    {
        STOS_MutexLock(Pool_p->Lock_p);

        if(Pool_p->Start == TRUE)
        {
            /*We have already started*/
            STOS_MutexRelease(Pool_p->Lock_p);
            return error;
        }

        /*Check for any producer present*/
        if(Pool_p->NumProducer)
        {
            Pool_p->NoProducer = FALSE;
        }
        else
        {
            // commented out so that any producer unit can register and get blocks.
            // The only outcome will be that there will be no NoProducer mode
            // Consumers already registered before producer registers will not be provided any blocks
#if NO_PRODUCER_MODE
            // commented out for safely
            //Pool_p->NoProducer = TRUE;
#else
            Pool_p->NoProducer = FALSE;
#endif
        }

        /*Check for any consumer*/
        if(Pool_p->NumConsumer)
        {
            Pool_p->NoConsumer = FALSE;
        }
        else
        {
            // commented out so that any unit can register and get blocks as consumer in runtime.
            // The only outcome will be that there will be no NoConsumer mode
            // but the code has been modified so that in the absence of consumers, producers can produce infinitely
#if NO_CONSUMER_MODE
            // commented out for safety
            //Pool_p->NoConsumer = TRUE;
#else
            Pool_p->NoConsumer = FALSE;
#endif
        }

        for( i = 0; i < Pool_p->NumBlocks; i++)
        {
            U32 j;

            Pool_p->Block_p[i].Free         = TRUE;

            for(j = 0;j < MAX_LOGGING_ENTRIES;j++ )
            {
                if(Pool_p->NoProducer)
                {
                    Pool_p->Block_p[i].Fill[j]  = TRUE;
                }
                else
                {
                    Pool_p->Block_p[i].Fill[j]  = FALSE;
                }
            }

            if(Pool_p->NoProducer)
            {
                Pool_p->Block_p[i].Filled           = TRUE;
            }
            else
            {
                Pool_p->Block_p[i].Filled           = FALSE;
            }

            Pool_p->Block_p[i].ConsumerCount = 0;

            /*put all the blocks in the free list*/

            Pool_p->BlkList_p[i].Blk_p = &(Pool_p->Block_p[i]);

            if(i < (U32)(Pool_p->NumBlocks-1))
            {
                Pool_p->BlkList_p[i].Next_p = &(Pool_p->BlkList_p[i+1]);
            }
            else
            {
                Pool_p->BlkList_p[i].Next_p = NULL;
            }
        }

        Pool_p->BlkForPro_p = Pool_p->BlkList_p;
        Pool_p->BlkForCon_p = NULL;
        Pool_p->ConQSize  = 0;
        Pool_p->ProdQSize = Pool_p->NumBlocks;

        /*trigger the units that have already requested for blocks*/
        /*Trigger the producer if present*/
        if(!Pool_p->NoProducer)
        {
            /*trigger producer if it is waiting*/
            if(Pool_p->Request[0])
            {
                Pool_p->Request[0] = FALSE;
                if(Pool_p->Logged[0].sem)
                {
                    STOS_SemaphoreSignal(Pool_p->Logged[0].sem);
                }
            }
        }

        /*Trigger the consumers if present*/
        if(!Pool_p->NoConsumer)
        {
            /*trigger consumers if it is waiting*/
            while(ConsumerCount <= Pool_p->NumConsumer)
            {
                if(Pool_p->Request[ConsumerCount])
                {
                    Pool_p->Request[ConsumerCount] = FALSE;
                    if(Pool_p->Logged[ConsumerCount].sem)
                    {
                        STOS_SemaphoreSignal(Pool_p->Logged[ConsumerCount].sem);
                    }
                }
                ConsumerCount++;
            }
        }
        Pool_p->Start = TRUE;
        STOS_MutexRelease(Pool_p->Lock_p);
    }
    else
    {
        error = ST_ERROR_INVALID_HANDLE;
    }

    return error;
}

/*****************************************************************************
MemPool_GetBufferParams()
Description:
    Gets the allocated buffer params.

Parameters:
    Handle, handle to the Mem Pool.
    DataParams_p, pointer to the buffer params.
Return Value:
    ST_NO_ERROR,                    no errors.
    ST_ERROR_BAD_PARAMETER,         one or more of the parameters was invalid.
    ST_ERROR_INVALID_HANDLE,        invalid handle .

See Also:
    MemPool_Term()
*****************************************************************************/

ST_ErrorCode_t MemPool_GetBufferParams(STMEMBLK_Handle_t Handle,STAUD_BufferParams_t* DataParams_p)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    MemoryPoolControl_t *Pool_p;

    if (InitMutex_p != NULL)
    {
        STOS_MutexLock(InitMutex_p);
    }
    else
    {
        return ST_ERROR_BAD_PARAMETER;
    }

    /* Obtain the Mem Pool control block from the passed in handle */
    Pool_p = MemPool_GetControlBlockFromHandle(Handle);

    STOS_MutexRelease(InitMutex_p);

    /* Only continue if we have a control block for this device */
    if (Pool_p != NULL)
    {
        STOS_MutexLock(Pool_p->Lock_p);
        if(Pool_p->Buffer_p)
        {
            DataParams_p->BufferBaseAddr_p  = Pool_p->Buffer_p;
            DataParams_p->BufferSize            = Pool_p->TotalSize;
            DataParams_p->NumBuffers            = Pool_p->NumBlocks;
        }
        else
        {
            error = ST_ERROR_NO_MEMORY;
        }
        STOS_MutexRelease(Pool_p->Lock_p);
    }
    else
    {
        error = ST_ERROR_INVALID_HANDLE;
    }

    return error;
}

#if MEMORY_SPLIT_SUPPORT
/*****************************************************************************
MemPool_SetSplit()
Description:
    Sets the size of each Split of the produced buffer.
    limitation is that the whole block should be equally divisible by the set size
Parameters:
    Handle, handle to the Mem Pool.
    ProdHandle, Handle of the producer.
    SplitSize, size of each split buffer in bytes
Return Value:
    ST_NO_ERROR,                    no errors.
    ST_ERROR_BAD_PARAMETER,         one or more of the parameters was invalid.
    ST_ERROR_INVALID_HANDLE,        invalid handle .

See Also:

*****************************************************************************/
ST_ErrorCode_t MemPool_SetSplit(STMEMBLK_Handle_t Handle, U32 ProdHandle, U32 SplitSize)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    MemoryPoolControl_t *Pool_p = (MemoryPoolControl_t *)Handle;

    /* Only continue if we have a control block for this device */
    if (Pool_p != NULL)
    {
        if((!Pool_p->Start)&& SplitSize&& (Pool_p->Logged[0].Handle == ProdHandle))
        {
            Pool_p->SplitSize = SplitSize;
        }
        else
        {
            error =  ST_ERROR_BAD_PARAMETER;
        }
    }
    return error;
}
#endif

/*****************************************************************************
MemPool_ReleaseMemory()
Description:
    Release all the allocated resources
    NOTE: Must be called with the queue mutex held.
          This block will release the held mutex
Parameters:
    Pool_p, handle of the block.
Return Value:
   depends of the subcalls
See Also:
    MemPool_Init()
    MemPool_Term()
*****************************************************************************/
ST_ErrorCode_t MemPool_ReleaseMemory(MemoryPoolControl_t * Pool_p)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    if(Pool_p)
    {
        ST_Partition_t  * CPUPartition = Pool_p->CPUPartition_p;

#ifndef STAUD_REMOVE_EMBX
        if (Pool_p->AllocateFromEMBX)
        {
            if(Pool_p->Buffer_p)
            {
                error = EMBX_Free((EMBX_VOID *)Pool_p->Buffer_p);
                if(error == EMBX_SUCCESS)
                {
                    Pool_p->Buffer_p = NULL;
                }
            }

            error |= EMBX_CloseTransport(Pool_p->tp);
        }
#endif
        if (Pool_p->AVMEMPartition && Pool_p->BufferHandle)
        {
            STAVMEM_FreeBlockParams_t FreeBlockParams;
            FreeBlockParams.PartitionHandle = Pool_p->AVMEMPartition;
            error = STAVMEM_FreeBlock(&FreeBlockParams, &Pool_p->BufferHandle);
            Pool_p->BufferHandle = 0;
        }

        if(Pool_p->Lock_p)
        {
            /*free semaphore*/
            STOS_MutexDelete(Pool_p->Lock_p);
            Pool_p->Lock_p = NULL;
        }

        if(Pool_p->Block_p)
        {
            STOS_MemoryDeallocate(CPUPartition, Pool_p->Block_p);
            Pool_p->Block_p = NULL;
        }

        if(Pool_p->BlkList_p)
        {
            STOS_MemoryDeallocate(CPUPartition, Pool_p->BlkList_p);
            Pool_p->BlkList_p = NULL;
        }

        /* Free up the control structure for the Mem Pool */
        STOS_MemoryDeallocate(CPUPartition,Pool_p);
    }

    /*release Mutex*/
    STOS_MutexRelease(InitMutex_p);
    return error;
}

