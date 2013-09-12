/****************************************************************************

File Name   : stttx.c

Description : Teletext API Routines, designed to be re-entrant and support
              as many instances as teletext hardware/available memory permits.

Copyright (C) 2000, 2007 STMicroelectronics


References  :

ETS 300 706 May 1997 "Enhanced Teletext specification"
"Teletext API" Reference DVD-API-003 Design document, v3.4

****************************************************************************/

/* Includes --------------------------------------------------------------- */
#if !defined(ST_OSLINUX)  /* linux changes */
#include <string.h>                  /* C libs */
#include <stdio.h>
#include "stlite.h"        /* MUST be included BEFORE stpti.h */
#include "sttbx.h"
#ifndef ST_OS21
#include <ostime.h>
#endif
#endif

#include "stddefs.h"
#include "stcommon.h"
#include "stvtg.h"                   /* Used for sync */

/* Needed for DENC base address */
#include "stdevice.h"

#if defined(DVD_TRANSPORT_PTI) || defined(DVD_TRANSPORT_LINK)
#include "pti.h"                     /* STAPI includes */
#elif defined(DVD_TRANSPORT_STPTI)
#include "stpti.h"
#elif defined(DVD_TRANSPORT_DEMUX)
#include "stdemux.h"
#endif

#if defined(DVD_TRANSPORT_LINK)
/* Need this for stuff in ptilink.h to be defined, and DTV doesn't have
 * teletext, so this should be safe.
 */
#if !defined(ST_DVB)
#define ST_DVB
#endif
#include "ptilink.h"
#endif

#if !defined(STTTX_USE_DENC)
#include "stvbi.h"
#else
#include "stdenc.h"
#endif
#ifdef STTTX_SUBT_SYNC_ENABLE
#include "stclkrv.h"
#endif
#include "stttx.h"
#include "stttxdat.h"
#include "stttxdef.h"                /* Local includes */
#if defined(ST_5105) || defined(ST_5188) || defined(ST_5107)
#include "stvmix.h"
#endif

#if defined( ST_OSLINUX )
#include "linux/kthread.h"
#endif

#if (!defined (ST_5105)) && (!defined (ST_7020)) && (!defined(ST_5188)) && (!defined(ST_5107))
extern STOS_INTERRUPT_DECLARE(XferCompleteISR, Data);
#endif

extern void TTXHandlerTask( void *Data );
extern void TTXBufferPoolingTask( void *Data );
extern void ISRTimeOutTask( void *Data );

/* Private Constants ------------------------------------------------------ */

/* Converts an event number to an eventid array offset */
#define EventId(x)  (x-STTTX_EVENT_DEFAULT)

/* These chips have an older ILC which doesn't support
 * interrupt_enable_number (or disable).
 */
#if defined(ST_5510) || defined(ST_5512) || defined(ST_5518)
#define ILC_1
#endif

/* Private Variables ------------------------------------------------------ */

static semaphore_t      *Atomic_p = NULL;
static stttx_context_t  *stttx_Sentinel = NULL;     /* ptr. to first node */

#if defined(DVD_TRANSPORT_PTI) || defined(DVD_TRANSPORT_LINK)
/* replace with 'signal_on_every_transport_packet' if partial PES required */
static buffer_flags_t   pti_flags = no_flags;
#endif

/* Avoids recalculating them every time */
U32 ClocksPerSecond = 0;

/* Private Macros --------------------------------------------------------- */

/* Private Function prototypes -------------------------------------------- */

ST_ErrorCode_t stttx_SuppressVBI( STTTX_Handle_t Handle,
           void ( *DMAReport )( U32 OddEven, U32 Start, U32 Count ) );

static BOOL IsHandleInvalid( STTTX_Handle_t  Handle,
                             stttx_context_t **Construct,
                             U32             *OpenIndex );
static BOOL IsOutsideRange( U32 ValuePassed,
                            U32 LowerBound,
                            U32 UpperBound );

static ST_ErrorCode_t RegisterEvents(stttx_context_t *ThisElem);
static ST_ErrorCode_t UnregisterEvents(stttx_context_t *ThisElem);

/* TTX internal function supplied from stttxtsk.c */
void TTXVBIEnable(STEVT_CallReason_t Reason,
                  const ST_DeviceName_t RegistrantName,
                  STEVT_EventConstant_t Event,
                  const void *EventData,
                  const void *SubscriberData_p);

#if defined (ST_7020)
static  ST_ErrorCode_t stttx_AVMEMSetup(stttx_context_t     *ThisElem,
                                         STAVMEM_PartitionHandle_t Partition,
                                         U32 * Address_p, U32 *Size_p);
#endif
#if defined (ST_5105) || defined(ST_5188) || defined(ST_5107)
static  ST_ErrorCode_t stttx_VMIXSetup(stttx_context_t     *ThisElem );

#endif

/* Functions -------------------------------------------------------------- */

/****************************************************************************
Name         : STTTX_GetRevision()

Description  : Returns a pointer to the Driver Revision String.
               May be called at any time.

Parameters   : None

Return Value : ST_Revision_t

See Also     : ST_Revision_t
*****************************************************************************/

ST_Revision_t STTTX_GetRevision( void )
{
    return STTTX_GETREVISION;
}

/****************************************************************************
Name         : STTTX_Init()

Description  : Initializes the Teletext Driver before use.  Memory is
               dynamically allocated for each call, supporting as many
               Teletext instances as free memory permits.  Each structure
               supports simultaneous Open calls up to the number of STB
               Objects specified, plus one for the VBI object.  It is
               implemented as a linked list, marginally favouring the
               first Initialized device for access times.  There is a
               notional limit of Opens imposed simply by the need to
               store both an Init instance "tag" and an Open Object
               number fields within the Handle.  This field partition
               can be changed easily, but is not thought likely to be
               restrictive, given the constraints within the PTI.

               An ISR is installed to flag completion of VBI transfer.
               Three threads are started, TTXHandlerTask manages the
               transfer of data from the PTI.  The VBIOutputTask and
               STBOutputTask each manage output of data to their
               respective destinations.  Message queues are initialised
               to control the flow of data between the threads.

Parameters   : ST_DeviceType_t non-NUL, unique name needs to be supplied,
               followed by STTTX_InitParams_t pointer.

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                     No errors determined
               ST_ERROR_REVISION_NOT_SUPPORTED Version mismatch
               ST_ERROR_FEATURE_NOT_SUPPORTED  Mode or feature unsupported
               ST_ERROR_ALREADY_INITIALIZED    Device already initialized
               ST_ERROR_BAD_PARAMETER          One or more parameters invalid
               ST_ERROR_NO_MEMORY              Memory allocation failure

See Also     : STTTX_InitParams_t
               STTTX_Open()
               STTTX_Term()
*****************************************************************************/
ST_ErrorCode_t STTTX_Init( const ST_DeviceName_t Name,
                           const STTTX_InitParams_t *InitParams )
{
    stttx_context_t     *NextElem=NULL, *PrevElem=NULL;       /* search list pointers */
    stttx_context_t     *ThisElem=NULL;                  /* element for this Init */
    data_store_t        *CurrPes;                   /* pointer into Pes[i] */
    Handled_Request_t   *QueueReq;                  /* working queue pointer */
    ST_ErrorCode_t      RetValue = ST_NO_ERROR;     /* assume OK 'til different */
    BOOL                InitFailure = FALSE;        /* malloc/ISR/Task failure */
    U32                 StructSize;                 /* determined from params */
    S32                 i, j;

    /* Perform parameter validity checks */
    if( ( Name           == NULL )               || /* NULL Name ptr? */
        ( InitParams     == NULL )               || /* NULL structure ptr? */
        ( Name[0]        == '\0' )               || /* Device Name undefined? */
        ( strlen( Name ) >=
                    ST_MAX_DEVICE_NAME ) )          /* string too long? */
    {
        return( ST_ERROR_BAD_PARAMETER );
    }

    if( ( InitParams->BaseAddress == NULL ) &&               /* H/W address omitted? */
        (InitParams->DeviceType != STTTX_DEVICE_TYPE_7020) )
    {
        return( ST_ERROR_BAD_PARAMETER );
    }
    if( IsOutsideRange(                             /* STB objects invalid? */
            InitParams->NumSTBObjects,
            MIN_STB_OBJECTS,
            MAX_STB_OBJECTS ) )
    {
        return( ST_ERROR_BAD_PARAMETER );
    }
    if( ( InitParams->DeviceType < STTTX_DEVICE_OUTPUT ) ||
       (InitParams->DeviceType > STTTX_DEVICE_TYPE_5105) )
    {
        return (ST_ERROR_BAD_PARAMETER);
    }
    if( ( InitParams->DeviceType == STTTX_DEVICE_TYPE_5105) &&
          (InitParams->VMIXDeviceName[0] == '\0' ) )    /* Invalid VMIXDeviceName? */
    {
        return (ST_ERROR_BAD_PARAMETER);
    }
    if( ( InitParams->DeviceType == STTTX_DEVICE_TYPE_7020) &&
          (InitParams->AVMEMPartition == 0) )
    {
        return (ST_ERROR_BAD_PARAMETER);
    }
    /* Determine the dynamic memory allocation requirement for the
       driver_partition, appending the space for NumSTBObjects and
       NumReqsAllowed parameters to the stttx_context_t definition.
       Note that one element of data_store_t (VBI slot) is already
       included in the stttx_context_t structure definition.        */

    StructSize = sizeof( stttx_context_t ) +
        ( InitParams->NumSTBObjects *
                sizeof( data_store_t ) ) +
        ( InitParams->NumRequestsAllowed *
                sizeof( Handled_Request_t ) );

    /* The following section is protected from re-entrancy whilst critical
       elements within the stttx_Sentinel linked list are both read and
       written.  It is not likely that Init will be called from a multi-
       tasking environment, but one never knows what a user might try....   */

    /* Ensure that the atomic semaphore is only initialised once. */
    STOS_TaskLock();
    if (Atomic_p == NULL)
    {
        Atomic_p = STOS_SemaphoreCreateFifo(NULL, 1);
    }
    STOS_TaskUnlock();

    /* The semaphore should be enough to make MT safe. Since it's unsafe
     * to allocate/free memory from an ISR anyway, no need for
     * interrupt_lock(). Semaphore guaranteed to exist at this stage.
     */
    STOS_SemaphoreWait(Atomic_p);

    if( stttx_Sentinel == NULL )                    /* first Init instance? */
    {
        stttx_Sentinel = ThisElem =                 /* keep a note of result */
            ( stttx_context_t* )STOS_MemoryAllocateClear(
               (partition_t *)InitParams->DriverPartition, 1, StructSize );
    }
    else                                            /* already Init'ed */
    {
        for( NextElem = stttx_Sentinel;             /* start at Sentinel node */
             NextElem != NULL;                      /* while not at list end */
             NextElem = NextElem->Next )            /* advance */
        {
            if( strcmp( Name,
                NextElem->Name ) == 0 )             /* existing DeviceName? */
            {
                STOS_SemaphoreSignal( Atomic_p );        /* unlock before exit */

                return( ST_ERROR_ALREADY_INITIALIZED );
            }

            PrevElem = NextElem;                    /* keep trailing pointer */
        }

        PrevElem->Next = ThisElem =                 /* keep note of result */
            ( stttx_context_t* )STOS_MemoryAllocate(
                (partition_t *)InitParams->DriverPartition, StructSize );
    }

    /* memory_allocate failure? */
    if( ThisElem == NULL )
    {
        STOS_SemaphoreSignal( Atomic_p );                /* unlock before exit */

        return( ST_ERROR_NO_MEMORY );
    }

    ThisElem->Next = NULL;                          /* mark as end of list */

    strcpy( ThisElem->Name, Name);                  /* retain Name for Opens */

    STOS_SemaphoreSignal( Atomic_p );                    /* end of atomic region */

    /* Initialise ISR semaphore */
    ThisElem->ISRComplete_p = STOS_SemaphoreCreateFifoTimeOut(NULL, 0);

    ThisElem->BaseHandle = ( STTTX_Handle_t )       /* use shifted pointer */
            ( (U32)ThisElem << OPEN_HANDLE_WID );   /*  as handle base "tag" */
    ThisElem->BaseAddress =
        (device_access_t *)InitParams->BaseAddress;

    /* Take a copy of the initialization parameters */
    ThisElem->InitParams = *InitParams;

    ThisElem->RequestsPtr =
        (Handled_Request_t *)&ThisElem->
            Pes[InitParams->NumSTBObjects +
                            MAX_VBI_OBJECTS];       /* pointer to Request[] */
    ThisElem->ObjectsOpen = 0;                      /* no Objects open yet */
    ThisElem->TasksActive = TRUE;                   /* enable task loops */
    ThisElem->SuppressVBI = FALSE;                  /* VBI DMA enabled */
    ThisElem->VBIUsed     = FALSE;
#ifdef STTTX_SUBT_SYNC_ENABLE
    ThisElem->SubtSyncOffset = STTTX_SYNC_OFFSET;
#endif
    /* set the active requests pointer to 0 */
    ThisElem->NumActiveRequests = 0;

    /* Calculate once and cache */
    ClocksPerSecond = ST_GetClocksPerSecond();
    /* Loop for VBI plus NumSTBObjects occurences */
    j = MAX_VBI_OBJECTS + InitParams->NumSTBObjects; /* total object count */
    for( i = VBI_SLOT; i < j; i++ )                  /* loop on all Objects */
    {
        CurrPes = &ThisElem->Pes[i];                 /* abbreviate access */

        /* Set the pointer back to the control-block */
        CurrPes->Element = ThisElem;

        /* Create the DMA'ed buffers in user partition.  To simplify
           error handling, these have been concatenated into a single
           call per Open object.  Note that the pti_malloc_buffer()
           function is not used to create the circular buffer because
           there is presently no complimentary free function.  Addition
           of BUFF_ALIGN_MASK allows this buffer to be aligned onto a
           16 byte boundary (a PTI DMA requirement) without reducing its
           intended size.  The aligned pointer is stored in CircleBase.   */

        CurrPes->PESQueue_p = STOS_MessageQueueCreate( sizeof(ttx_Message_t),
                                                          MAX_MESSAGES);

#if defined(DVD_TRANSPORT_PTI)
        CurrPes->CircleBuff = (U8 *)
            STOS_MemoryAllocate((partition_t *)InitParams->DriverPartition,
                            CIRCULAR_BUFFER_SIZE + BUFF_ALIGN_MASK);

        /* Align for access */
        CurrPes->CircleBase = (U8*)
            ( ( ( (U32)CurrPes->CircleBuff ) +
                BUFF_ALIGN_MASK ) &
                    ( ~BUFF_ALIGN_MASK ) );

        /* Size of the circular buffer is the overall size, minus the
         * amount used for the linear buffer.
         */
        CurrPes->CircleSize = CIRCULAR_BUFFER_SIZE;
        STTBX_Print(("Circle size: %i\n", CurrPes->CircleSize));

#elif defined(DVD_TRANSPORT_LINK)
        /* Best to let the link block malloc its own buffer; this does mean
         * that we'll leak memory, though.
         */
        if (pti_malloc_buffer(CIRCULAR_BUFFER_SIZE,
                              &CurrPes->CircleSize,
                              &CurrPes->CircleBase))
        {
            InitFailure = TRUE;
            break;
        }
        if (CurrPes->CircleBase == NULL)
        {
            InitFailure = TRUE;
            break;
        }
        STTBX_Print(("Circle size: %i\n", CurrPes->CircleSize));
#endif

        CurrPes->PIDNum = INVALID_PID;              /* flag as unknown yet */

        CurrPes->Open = FALSE;                      /* not open yet */
        CurrPes->Playing = FALSE;
    }

    if( InitFailure )                               /* memory alloc failure? */
    {
        RetValue = ST_ERROR_NO_MEMORY;

        for( i--; i >= VBI_SLOT; i-- )              /* reverse above loop */
        {
            STOS_MessageQueueDelete(ThisElem->Pes[i].PESQueue_p);
        }

        /* Cannot free pti_mallocs... */
    }
    else
    {
        /* copy request queue ptr. */
        QueueReq = ThisElem->RequestsPtr;

        for( i = 0;                                 /* loop on request slots */
             i < ThisElem->InitParams.NumRequestsAllowed; i++ )
        {
            QueueReq[i].OpenOff = REQ_SLOT_UNUSED;  /* illegal Open Index val */
            QueueReq[i].OddOffset  = -1;            /* (Offsets used for */
            QueueReq[i].EvenOffset = -1;            /* whole page filtering) */
            QueueReq[i].HeaderFound = FALSE;
            QueueReq[i].Request.RequestTag =        /* excluded user value */
                        STTTX_FREE_REQUEST_TAG;
        }
#if (!defined (ST_5105)) && (!defined (ST_7020)) && (!defined(ST_5188)) && (!defined(ST_5107))
        /* install the interrupt handler */
        if( STOS_InterruptInstall(
                InitParams->InterruptNumber,
                InitParams->InterruptLevel,
                XferCompleteISR,
                "STTTX",
                (void *)ThisElem ) != 0 )
        {
            InitFailure = TRUE;
            RetValue = ST_ERROR_INTERRUPT_INSTALL;
        }
        else
        {
            if(STOS_InterruptEnable(InitParams->InterruptNumber,
                                                InitParams->InterruptLevel)!=0)
            {
                InitFailure = TRUE;
                RetValue = ST_ERROR_INTERRUPT_INSTALL;
            }

        }
#endif /* ST_5105/07/88, 7020 */
        /* Register the events */
        if (InitParams->EVTDeviceName != NULL)
        {
            /* Also opens a handle for STEVT */
            if (RegisterEvents(ThisElem) != ST_NO_ERROR)
            {
                return (STTTX_ERROR_EVT_REGISTER);
            }
        }
    }

#if !defined(STTTX_USE_DENC)
    /* Open and enable the VBI driver */
    if (InitFailure == FALSE)
    {
        STVBI_OpenParams_t VBI;
        ST_ErrorCode_t error;

        VBI.Configuration.VbiType = STVBI_VBI_TYPE_TELETEXT;
        VBI.Configuration.Type.TTX.Latency = STTTX_DMA_DELAY_TIME;
        VBI.Configuration.Type.TTX.FullPage = FALSE;
        VBI.Configuration.Type.TTX.Amplitude = STVBI_TELETEXT_AMPLITUDE_70IRE;
        VBI.Configuration.Type.TTX.Standard = STVBI_TELETEXT_STANDARD_B;
        error = STVBI_Open(ThisElem->InitParams.VBIDeviceName,
                           &VBI, &ThisElem->VBIHandle);
        /* STVBI_Enable now called in OutContLines DDTS-21988 */
        if (error != ST_NO_ERROR)
        {
            InitFailure = TRUE;
            RetValue = STTTX_ERROR_STVBI_SETUP;
        }
    }
#else
    /* STTTX_USE_DENC defined, open handle to STDENC */
    if (InitFailure == FALSE)
    {
        STDENC_OpenParams_t DencOpenParams;
        ST_ErrorCode_t error;

        DencOpenParams.NoUsed = 0;
        error = STDENC_Open(InitParams->DENCDeviceName, &DencOpenParams,
                            &ThisElem->DENCHandle);
        if (error != ST_NO_ERROR)
        {
            InitFailure = TRUE;
            RetValue = STTTX_ERROR_STDENC_SETUP;
        }
    }
#endif

#ifdef STTTX_SUBT_SYNC_ENABLE
    /* Open handle to STCLKRV to achieve synchronization */
    if (InitFailure == FALSE)
    {
        STCLKRV_OpenParams_t  CLKRVOpenPars;
        ST_ErrorCode_t error;
        /* Open the Clock Recovery Driver */
        error = STCLKRV_Open( InitParams->CLKRVDeviceName, &CLKRVOpenPars, &ThisElem->CLKRVHandle);
        if (error != ST_NO_ERROR)
        {
            InitFailure = TRUE;
            RetValue = STTTX_ERROR_STCLKRV_SETUP;
        }
    }
#endif

#if defined (TTXDMA_NOT_COMPLETE_WORKAROUND)
    /* Initialize the ttx_dma_bug_not_complete code here */
    if (InitFailure == FALSE)
    {
        ThisElem->TimeOutTaskActive = TRUE;
        ThisElem->TTXDMATimeOut_p    = STOS_SemaphoreCreateFifoTimeOut(NULL, 0);
        ThisElem->ISRTimeOutWakeUp_p = STOS_SemaphoreCreateFifo(NULL, 0);

        if( STOS_TaskCreate((void(*)(void *))ISRTimeOutTask,
                                        ThisElem,
                                        ThisElem->InitParams.DriverPartition,
                                        STTTX_DMA_TIMEOUT_STACK_DEPTH,
                                        &ThisElem->ISRTimeOutStack_p,
                                        ThisElem->InitParams.DriverPartition,
                                        &ThisElem->ISRTimeOuttask_p,
                                        &ThisElem->ISRTimeOuttdesc,
                                        STTTX_TIMEOUT_TASK_PRIORITY,
                                        "TTXDMATOtask",
                                        (task_flags_t)0)!= ST_NO_ERROR)

        {
            /* task not created successfully */
            STOS_SemaphoreDelete(NULL,ThisElem->TTXDMATimeOut_p);
            STOS_SemaphoreDelete(NULL,ThisElem->ISRTimeOutWakeUp_p);
            InitFailure = TRUE;
        }
        else { InitFailure = FALSE; }


    }
#endif

#if defined (ST_7020)
    if( !InitFailure )                               /* failure above? */
    {
        ST_ErrorCode_t Lerror;
        U32 Address_TTXAVMEMBuffer; U32 Size_TTXAVMEMBuffer = ( 48 * 16 );

        Lerror = stttx_AVMEMSetup(ThisElem,
                                       InitParams->AVMEMPartition,
                                       &Address_TTXAVMEMBuffer,
                                       &Size_TTXAVMEMBuffer);
        if (Lerror == ST_NO_ERROR)
        {
            /* Enable DENC registers */
            *(volatile U32*)(ST7020_DENC_BASE_ADDRESS+0x1C0) = 0x01;
            ThisElem->TTXAVMEM_Buffer_Address = (U32*)Address_TTXAVMEMBuffer;
            /* Specify the address to DENC */
            *(volatile U32*)(ST7020_DENC_BASE_ADDRESS+0x1C4) = (Address_TTXAVMEMBuffer & 0x03ffff00);
        }
    }
#elif defined (ST_5105) || defined(ST_5188) || defined(ST_5107)
    if( !InitFailure )
    {
        ST_ErrorCode_t error;
        error = stttx_VMIXSetup(ThisElem);

        if (error != ST_NO_ERROR)
        {
            InitFailure = TRUE;
            RetValue = STTTX_ERROR_STVMIX_SETUP;
        }
    }
#endif
    if( InitFailure )                               /* failure above? */
    {
        /* free the linked list element created */
        STOS_SemaphoreWait( Atomic_p );                  /* atomic sequence start */

        if (PrevElem != NULL)
        {
            /* remove ThisElem from list */
            PrevElem->Next = ThisElem->Next;
        }

        for( i--; i >= VBI_SLOT; i-- )              /* reverse above loop */
        {
            STOS_MessageQueueDelete(ThisElem->Pes[i].PESQueue_p);
        }

        STOS_SemaphoreDelete(NULL,ThisElem->ISRComplete_p);
        STOS_MemoryDeallocate(                          /* free the memory */
                (partition_t *)InitParams->DriverPartition,
                ThisElem);


        /* Atomicity ensured by the fact we're still holding the
         * semaphore
         */
        if (stttx_Sentinel == ThisElem )            /* no Inits outstanding? */
        {
            stttx_Sentinel = ThisElem = NULL;
        }
        STOS_SemaphoreSignal(Atomic_p);
    }
    /* some problem in *5514* */
#if defined(STTTX_USE_DENC) && defined(ST_5514)
    else
    {
        /* Now program DENC registers for teletext.  */
        /* Enable TTX */
        STDENC_OrReg8(ThisElem->DENCHandle, DENC_BASE_ADDRESS + 0x6,
                      0x02);
        /* TTX not MV */
        STDENC_OrReg8(ThisElem->DENCHandle, DENC_BASE_ADDRESS + 0x8,
                      0x04);
    }
#endif

    return( RetValue );
}
#if defined(ST_5105) || defined(ST_5188) || defined(ST_5107)
/***********************************************************************
stttx_VMIXSetup()
VMIX setup for 5105
************************************************************************/
static  ST_ErrorCode_t stttx_VMIXSetup( stttx_context_t      *ThisElem )
{
    STVMIX_OpenParams_t VMIXOpenParams;
    ST_ErrorCode_t error;

    error  = STVMIX_Open(ThisElem->InitParams.VMIXDeviceName,&VMIXOpenParams,&ThisElem->VMIXHandle);
    error |= STVMIX_Enable(ThisElem->VMIXHandle);
    error |= STVMIX_OpenVBIViewPort(ThisElem->VMIXHandle,STVMIX_VBI_TXT_ODD,&ThisElem->VBIVPDisplayOdd);
    error |= STVMIX_OpenVBIViewPort(ThisElem->VMIXHandle,STVMIX_VBI_TXT_EVEN,&ThisElem->VBIVPDisplayEven);
    if (error != ST_NO_ERROR)
    {
        error = STTTX_ERROR_STVMIX_SETUP;
    }

    return error;
}
#endif

#ifdef ST_7020
/***********************************************************************
    AllocateBitBuffer()
    Allocates the bitbuffer for each input object.

*************************************************************************/
static  ST_ErrorCode_t stttx_AVMEMSetup(stttx_context_t     *ThisElem,
                                         STAVMEM_PartitionHandle_t Partition,
                                         U32 * Address_p, U32 *Size_p)
{
    void * AllocAddress;
    ST_ErrorCode_t Error;
    ST_ErrorCode_t AvmemError;

    STAVMEM_AllocBlockParams_t  AllocParams;
    STAVMEM_SharedMemoryVirtualMapping_t VirtualMapping;

    /* Allocate a block */
    AllocParams.PartitionHandle = Partition;
    AllocParams.Alignment = 256;
    AllocParams.AllocMode = STAVMEM_ALLOC_MODE_TOP_BOTTOM;
    AllocParams.ForbiddenRangeArray_p = NULL;
    AllocParams.ForbiddenBorderArray_p = NULL;
    AllocParams.NumberOfForbiddenRanges = 0;
    AllocParams.NumberOfForbiddenBorders = 0;

    if(*Size_p != 0)
    {
        AllocParams.Size = *Size_p;
    }
    else /* This case should never occur , only a double check */
    {
        STTBX_Report(( STTBX_REPORT_LEVEL_ERROR, "BUFFER SIZE NOT SET" ));
        return ST_ERROR_BAD_PARAMETER;
    }

    Error = STAVMEM_AllocBlock(&AllocParams, &ThisElem->AVMEMBlockHandle);

    if(Error == ST_NO_ERROR)
    {
        Error = STAVMEM_GetBlockAddress(ThisElem->AVMEMBlockHandle, &AllocAddress);

        if(Error == ST_NO_ERROR)
        {
            /* get the virtual mapping structure created at Init time*/
            AvmemError = STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping);
            if( AvmemError == ST_NO_ERROR )
            {
                if( STAVMEM_IsAddressVirtual(AllocAddress, &VirtualMapping) )
                {
                    AllocAddress = STAVMEM_VirtualToCPU(AllocAddress, &VirtualMapping);
                }
            }
            *Address_p = (U32)((U32*)AllocAddress);
            *Size_p    = AllocParams.Size;
        }
        else
        {
            Error = ST_ERROR_NO_MEMORY;
        }
    }
    else
    {
        Error = ST_ERROR_NO_MEMORY;
    }

    return Error;
}

#endif

/****************************************************************************
Name         : STTTX_Open()

Description  : Opens the Teletext for operation.  MUST have been preceded
               by a successful Init call for this (Device)Name.

Parameters   : STTTX_DeviceType_t Name needs to be supplied, a pointer to
               STTTX_OpenParams_t *OpenParams giving output type, plus
               STTTX_Handle_t *Handle pointer for return of the subsequent
               access Handle.

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                 No errors occurred
               ST_ERROR_UNKNOWN_DEVICE     Invalid Name/no prior Init
               ST_ERROR_BAD_PARAMETER      One or more invalid parameters
               ST_ERROR_NO_FREE_HANDLES    Opens exceed Init declared total

See Also     : STTTX_Handle_t
               STTTX_OpenParams_t
               STTTX_Init()
               STTTX_Close()
*****************************************************************************/

ST_ErrorCode_t STTTX_Open( const ST_DeviceName_t     Name,
                           const STTTX_OpenParams_t  *OpenParams,
                           STTTX_Handle_t            *Handle )
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    stttx_context_t     *ThisElem;                  /* element for this Open */
    data_store_t        *CurrPes;                   /* pointer into Pes[i] */
    U32                 i, j;
    STEVT_DeviceSubscribeParams_t EVTSubscribeParams;

    /* Perform parameter validity checks */

    if( ( Name            == NULL )              || /* Name   ptr uninitialised? */
        ( OpenParams      == NULL )              || /* OpenParams ptr uninit? */
        ( Handle          == NULL ) )               /* Handle ptr uninitialised? */
    {
        return( ST_ERROR_BAD_PARAMETER );
    }

    /* One previous call to Init MUST have had the same Device Name */

    for( ThisElem = stttx_Sentinel;                 /* start at Sentinel node */
        ThisElem != NULL;                           /* while not at list end */
            ThisElem = ThisElem->Next )             /* advance to next element */
    {
        if( strcmp( Name,
                ThisElem->Name ) == 0 )             /* existing DeviceName? */
        {
            break;                                  /* match, terminate search */
        }
    }

    if( ThisElem == NULL )                          /* no Name match found? */
    {
        return( ST_ERROR_UNKNOWN_DEVICE );
    }

    /* Initialization structure for Name has been located within linked list.
       Protect from re-entrancy whilst structure data is updated.....        */

    STOS_SemaphoreWait( Atomic_p );                      /* atomic sequence start */

    if ((OpenParams->Type != STTTX_VBI) &&
        (OpenParams->Type != STTTX_STB) &&
        (OpenParams->Type != STTTX_STB_VBI))
    {
        STOS_SemaphoreSignal( Atomic_p );            /* end atomic before exit! */
        return( ST_ERROR_BAD_PARAMETER );
    }

    if ((OpenParams->Type == STTTX_VBI) || (OpenParams->Type == STTTX_STB_VBI))
    {
        if (ThisElem->VBIUsed == TRUE)
        {
            STOS_SemaphoreSignal( Atomic_p );            /* end atomic before exit! */
            return (ST_ERROR_NO_FREE_HANDLES);
        }
    }

    j = ThisElem->InitParams.NumSTBObjects +
                        MAX_VBI_OBJECTS;    /* total object count */
    for( i = STB_SLOT; i < j; i++ )         /* loop on all STB objects */
    {
        if( !ThisElem->Pes[i].Open )        /* empty slot? */
        {
            break;                          /* use this slot */
        }
    }

    if( i >= j )                            /* no STB channels free? */
    {
        STOS_SemaphoreSignal( Atomic_p );        /* end atomic before exit! */
        return( ST_ERROR_NO_FREE_HANDLES );
    }

    CurrPes = &ThisElem->Pes[i];            /* abbreviate pointer */
    CurrPes->Object = i;                    /* note the object number */
    *Handle = ThisElem->BaseHandle + i;     /* pass back base + index */

    CurrPes->Mode = OpenParams->Type;

    if ((OpenParams->Type == STTTX_VBI) || (OpenParams->Type == STTTX_STB_VBI))
    {
        ThisElem->VBIUsed = TRUE;
    }
    CurrPes->Open = TRUE;                           /* flag as allocated */

    ThisElem->ObjectsOpen++;                        /* count objects open */
    CurrPes->ObjectType = OpenParams->Type;
    CurrPes->DataIdentifier = DEFAULT_DATA_IDENTIFIER;
    CurrPes->Element = ThisElem;

    CurrPes->WaitForPES_p    = STOS_SemaphoreCreateFifoTimeOut(NULL, 0);
    CurrPes->WakeUp_p        = STOS_SemaphoreCreateFifo(NULL, 0);
    CurrPes->PoolingWakeUp_p = STOS_SemaphoreCreateFifo(NULL, 0);
    CurrPes->PESArrived_p    = STOS_SemaphoreCreateFifo(NULL, 0);
    CurrPes->Stopping_p      = STOS_SemaphoreCreateFifo(NULL, 1);

    STOS_SemaphoreSignal( Atomic_p );                    /* end of atomic region */

    CurrPes->Playing = FALSE;                       /* not playing yet */
    CurrPes->HandlerRunning = FALSE;
    CurrPes->TasksActive = TRUE;

#if defined(DVD_TRANSPORT_PTI) || defined(DVD_TRANSPORT_LINK)
    CurrPes->CircleProd =                           /* initialise pointers */
        CurrPes->CircleCons = CurrPes->CircleBase;
    CurrPes->Slot = OpenParams->Slot;

    /* Default to getting data from PTI */
    CurrPes->SourceParams.DataSource = STTTX_SOURCE_PTI_SLOT;
#endif

    /* Now spawn the task to look after it */
    {
        char tmpstring[30];

        sprintf(tmpstring, "PES handler %i", ThisElem->ObjectsOpen);
        STTBX_Print(("Initialising task %s\n", tmpstring));

        if (STOS_TaskCreate((void(*)(void *))TTXHandlerTask,
                                        CurrPes,
                                        ThisElem->InitParams.DriverPartition,
                                        STTTX_TTX_STACK_DEPTH,
                                        &CurrPes->TTXStack_p,
                                        ThisElem->InitParams.DriverPartition,
                                        &CurrPes->TTXttask_p,
                                        &CurrPes->TTXtdesc,
                                        STTTX_HANDLER_TASK_PRIORITY,
                                        tmpstring,
                                        (task_flags_t)0)!= ST_NO_ERROR)

        {
            /* Didn't create task successfully; back out the changes made
             * above.
             */
            error = STTTX_ERROR_TASK_CREATION;

            STOS_SemaphoreWait(Atomic_p);

            CurrPes->Object = 0;
            *Handle = 0;
            CurrPes->Open = FALSE;
            ThisElem->ObjectsOpen--;                  /* count objects open */
#if defined(DVD_TRANSPORT_PTI) || defined(DVD_TRANSPORT_LINK)
            CurrPes->CircleProd = NULL;
#endif
            STOS_SemaphoreSignal(Atomic_p);
        }
        else
        {
            sprintf(tmpstring, "Pooling task %i", ThisElem->ObjectsOpen);
            STTBX_Print(("Initialising task %s\n", tmpstring));
            if( STOS_TaskCreate((void(*)(void *))TTXBufferPoolingTask,
                                        CurrPes,
                                        ThisElem->InitParams.DriverPartition,
                                        STTTX_TTX_STACK_DEPTH,
                                        &CurrPes->TTXPoolStack_p,
                                        ThisElem->InitParams.DriverPartition,
                                        &CurrPes->TTXPoolttask_p,
                                        &CurrPes->TTXPooltdesc,
                                        STTTX_POOLING_TASK_PRIORITY,
                                        tmpstring,
                                        (task_flags_t)0)!= ST_NO_ERROR)

            {
                error = STTTX_ERROR_TASK_CREATION;

                STOS_SemaphoreWait(Atomic_p);

                CurrPes->TasksActive = FALSE;
                STOS_SemaphoreSignal(CurrPes->WakeUp_p);
                STOS_SemaphoreSignal(CurrPes->PESArrived_p);
#if defined(DVD_TRANSPORT_PTI) || defined(DVD_TRANSPORT_LINK)
                STOS_SemaphoreSignal(CurrPes->WaitForPES_p);
#endif
                STOS_TaskWait(&CurrPes->TTXttask_p, TIMEOUT_INFINITY);
                STOS_TaskDelete(CurrPes->TTXttask_p,(partition_t *)ThisElem->InitParams.DriverPartition,
                                CurrPes->TTXStack_p,(partition_t *)ThisElem->InitParams.DriverPartition);
                CurrPes->Object = 0;
                *Handle = 0;
                CurrPes->Open = FALSE;
                ThisElem->ObjectsOpen--;              /* count objects open */
#if defined(DVD_TRANSPORT_PTI) || defined(DVD_TRANSPORT_LINK)
                CurrPes->CircleProd = NULL;
#endif

                STOS_SemaphoreSignal(Atomic_p);
            }
        }
    }

    /* If all went well, set 'data ready' to no, and subscribe to VTG
     * VSYNC pulse, so we know when to enable the VBI. This helps to
     * avoid the 'all high' problem seen in DDTS GNBvd21988.
     */
    if ((error == ST_NO_ERROR) && (OpenParams->Type != STTTX_STB))
    {

        ThisElem->OddDataReady = FALSE;
        ThisElem->EvenDataReady = FALSE;

        EVTSubscribeParams.NotifyCallback = TTXVBIEnable;
        EVTSubscribeParams.SubscriberData_p = (void *)ThisElem;
        error = STEVT_SubscribeDeviceEvent(ThisElem->EVTHandle,
                                           NULL,
                                           STVTG_VSYNC_EVT,
                                           &EVTSubscribeParams);
        if (error != ST_NO_ERROR)
        {
            STTBX_Print(("Couldn't subscribe to STVTG event!\n"));
            error = STTTX_ERROR_EVT_REGISTER;
        }
    }

    return(error);
}


/****************************************************************************
Name         : stttx_SuppressVBI()

Description  : Hidden external call to suppress the DMA output during VBI,
               calling-back a function instead with the field flag, start
               line number and count of lines as formal parameters.

Parameters   : STTTX_Handle_t Handle references the Open VBI object,
               (void *)( DMAReport( U32 FFlag, U32 Start, U32 Count ) )
               call-back.

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                 No errors occurred
               ST_ERROR_INVALID_HANDLE     Handle number wrong/not open

See Also     : STTTX_Handle_t
*****************************************************************************/

ST_ErrorCode_t stttx_SuppressVBI( STTTX_Handle_t Handle,
           void ( *DMAReport )( U32 FFlag, U32 Start, U32 Count ) )
{
    stttx_context_t     *ThisElem;                  /* element for this Open */
    U32                 OpenIndex;                  /* Open Object number */


    /* Perform parameter validity checks */

    if( IsHandleInvalid( Handle,
                    &ThisElem, &OpenIndex ) )       /* Handle passed invalid? */
    {
        return( ST_ERROR_INVALID_HANDLE );
    }

    ThisElem->DMAReport = DMAReport;                /* store call-back address */
    ThisElem->SuppressVBI = TRUE;

    return( ST_NO_ERROR );
}


/****************************************************************************
Name         : STTTX_SetStreamID()

Description  : Changes the Teletext PID of an Open teletext object.  Action
               taken differs depending whether the Object has been Started.

Parameters   : STTTX_Handle_t Handle references an Open object,
               U32 PIDNum specifies the PID value it is assigned.

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                 No errors occurred
               ST_ERROR_INVALID_HANDLE     Handle number wrong/not open
               ST_ERROR_BAD_PARAMETER      One or more invalid parameters
               STTTX_ERROR_CANT_STOP       Clear PID/Flush failure from PTI
               STTTX_ERROR_CANT_START      Set PID failure from PTI

See Also     : STTTX_Handle_t
*****************************************************************************/

ST_ErrorCode_t STTTX_SetStreamID( STTTX_Handle_t     Handle,
                                  U32                ProgramID )
{
    stttx_context_t     *ThisElem;                  /* element for this Open */
    data_store_t        *CurrPes;                   /* pointer into Pes[] */
    ST_ErrorCode_t      RetCode = ST_NO_ERROR;
    U32                 OpenIndex;                  /* Open Object number */


    /* Perform parameter validity checks */
    if( IsHandleInvalid( Handle,
                    &ThisElem, &OpenIndex ) )       /* Handle passed invalid? */
    {
        return( ST_ERROR_INVALID_HANDLE );
    }

    CurrPes = &ThisElem->Pes[OpenIndex];            /* abbreviate open struct */

#if defined(DVD_TRANSPORT_PTI) || defined(DVD_TRANSPORT_LINK)
    if (CurrPes->SourceParams.DataSource == STTTX_SOURCE_PTI_SLOT)
    {
        if( !CurrPes->Playing )                         /* if not playing */
        {
            CurrPes->PIDNum = ProgramID;                /* update stored PID number */
            CurrPes->LastEvent = 0;                     /* reset last event */
            return( RetCode );                          /* no other action required */
        }

#if defined(DVD_TRANSPORT_LINK)
        pti_stop_slot(CurrPes->Slot);
#endif

        if( pti_clear_pid( CurrPes->Slot ) ||           /* deactivate PTI on PID */
            pti_flush_stream( CurrPes->Slot ) )         /* flush any partial data */
        {
            return( STTTX_ERROR_CANT_STOP );            /* PTI failure */
        }

        if( pti_set_pid( CurrPes->Slot, ProgramID ) )   /* activate PTI on PID */
        {
            return( STTTX_ERROR_CANT_START );           /* PTI failure */
        }

#if defined(DVD_TRANSPORT_LINK)
        pti_start_slot(CurrPes->Slot);
#endif

        CurrPes->PIDNum = ProgramID;                    /* note user PID number */
        CurrPes->LastEvent = 0;                         /* reset last event */
    }
    else
        RetCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
#else
    RetCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
#endif

    return( RetCode );
}


/****************************************************************************
Name         : STTTX_ModeChange()

Description  : Changes the Teletext mode of an Open teletext object.

Parameters   : STTTX_Handle_t Handle references an Open object,
               Mode refers to the mode of operation (object type)

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                 No errors occurred
               ST_ERROR_INVALID_HANDLE     Handle number wrong/not open
               ST_ERROR_BAD_PARAMETER      One or more invalid parameters
               STTTX_ERROR_CANT_START      Set PID failure from PTI

See Also     : STTTX_Handle_t
*****************************************************************************/

ST_ErrorCode_t STTTX_ModeChange( STTTX_Handle_t     Handle,
                                  STTTX_ObjectType_t Mode )
{
    stttx_context_t     *ThisElem;                  /* element for this Open */
    data_store_t        *CurrPes;                   /* pointer into Pes[] */
    ST_ErrorCode_t      RetCode = ST_NO_ERROR;
    U32                 OpenIndex;                  /* Open Object number */

    /* Perform parameter validity checks */
    if (IsHandleInvalid(Handle, &ThisElem, &OpenIndex))
        return( ST_ERROR_INVALID_HANDLE );

    /* abbreviate open struct */
    CurrPes = &ThisElem->Pes[OpenIndex];

    if ((Mode == STTTX_VBI) || (Mode == STTTX_STB_VBI))
    {
        if (CurrPes->Mode == STTTX_STB)
            if (ThisElem->VBIUsed == TRUE)
                return (STTTX_ERROR_CANT_START);
    }

    /* Do some checks on current and new mode, to see if we have any
     * particular work to do.
     */
    if ((CurrPes->Mode == STTTX_STB) && (Mode != STTTX_STB))
    {
        /* If we need to do so now, set 'data ready' to no, and
         * subscribe to VTG VSYNC pulse, so we know when to enable the
         * VBI. This helps to avoid the 'all high' problem seen in DDTS
         * GNBvd21988.
         */
        STEVT_DeviceSubscribeParams_t EVTSubscribeParams;
        ST_ErrorCode_t error = ST_NO_ERROR;

        ThisElem->OddDataReady = FALSE;
        ThisElem->EvenDataReady = FALSE;

        EVTSubscribeParams.NotifyCallback = TTXVBIEnable;
        EVTSubscribeParams.SubscriberData_p = (void*)ThisElem;
        error = STEVT_SubscribeDeviceEvent(ThisElem->EVTHandle,
                                           NULL,
                                           STVTG_VSYNC_EVT,
                                           &EVTSubscribeParams);
        if (error != ST_NO_ERROR)
            return STTTX_ERROR_EVT_REGISTER;
    }

    if ((CurrPes->Mode != STTTX_STB) && (Mode == STTTX_STB))
    {
        /* Else (if we're switching from VBI to STB mode),
         * unsubscribe from the VTG event.
         */
        STEVT_UnsubscribeDeviceEvent(ThisElem->EVTHandle,
                                     NULL, STVTG_VSYNC_EVT);

        /* And clear this, so some other hand can use VBI from now on */
        ThisElem->VBIUsed = FALSE;
    }

    /* Finally, update the stored mode. */
    CurrPes->Mode = Mode;

    return( RetCode );
}


/****************************************************************************
Name         : STTTX_GetStreamID()

Description  : Returns the currently selected PIDNum of an Open teletext object.

Parameters   : STTTX_Handle_t Handle references an Open object,
               U32 *PIDNum returns the PID value currently assigned.

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                 No errors occurred
               ST_ERROR_INVALID_HANDLE     Handle number wrong/not open
               ST_ERROR_BAD_PARAMETER      One or more invalid parameters

See Also     : STTTX_Handle_t
*****************************************************************************/

ST_ErrorCode_t STTTX_GetStreamID( STTTX_Handle_t     Handle,
                                  U32                *ProgramID )
{
    stttx_context_t     *ThisElem;                  /* element for this Open */
    U32                 OpenIndex;                  /* Open Object number */


    /* Perform parameter validity checks */

    if( IsHandleInvalid( Handle,
                    &ThisElem, &OpenIndex ) )       /* Handle passed invalid? */
    {
        return( ST_ERROR_INVALID_HANDLE );
    }

    if( ProgramID == NULL )
    {
        return( ST_ERROR_BAD_PARAMETER );
    }

#if defined(DVD_TRANSPORT_PTI) || defined(DVD_TRANSPORT_LINK)
    *ProgramID = ThisElem->Pes[OpenIndex].PIDNum;   /* return current PID */

    return( ST_NO_ERROR );
#else
    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
#endif


}

/****************************************************************************
Name         : STTTX_SetDataIdentifier()

Description  : Changes the current data identifier associated with a PID of an
               open teletext object.

Parameters   : Handle,          references an Open object,
               DataIdentifier,  DVB data_identifier value (range 0x10 to 0x1F).

Return Value : ST_ErrorCode_t specified as:
               ST_NO_ERROR                  No errors occurred.
               ST_ERROR_INVALID_HANDLE      Handle number wrong/not open.
               ST_ERROR_BAD_PARAMETER       Invalid data identifier.

See Also     : STTTX_Handle_t
*****************************************************************************/

ST_ErrorCode_t STTTX_SetDataIdentifier( STTTX_Handle_t     Handle,
                                        U8                 DataIdentifier )
{
    stttx_context_t     *ThisElem;                  /* element for this Open */
    data_store_t        *CurrPes;                   /* pointer into Pes[] */
    U32                 OpenIndex;                  /* Open Object number */


    /* Perform parameter validity checks */
    if( IsHandleInvalid( Handle,
                    &ThisElem, &OpenIndex ) )       /* Handle passed invalid? */
    {
        return( ST_ERROR_INVALID_HANDLE );
    }

    /* Ensure data identifier is within DVB allowed range */
    if ( DataIdentifier < 0x10 ||
         DataIdentifier > 0x1F )
    {
        return( ST_ERROR_BAD_PARAMETER );
    }

    CurrPes = &ThisElem->Pes[OpenIndex];            /* abbreviate open struct */

    /* Store data identifier */
    CurrPes->DataIdentifier = DataIdentifier;
    CurrPes->LastEvent = 0;                         /* reset last event */

    return( ST_NO_ERROR );
}


/****************************************************************************
Name         : STTTX_GetDataIdentifier()

Description  : Returns the currently selected data identifier of an Open
               teletext object.

Parameters   : Handle,       references an Open object,
               PIDNum,       for storing the PID value currently assigned.

Return Value : ST_ErrorCode_t specified as:
               ST_NO_ERROR                  No errors occurred
               ST_ERROR_INVALID_HANDLE      Handle number wrong/not open
               ST_ERROR_BAD_PARAMETER       One or more invalid parameters

See Also     : STTTX_Handle_t
*****************************************************************************/

ST_ErrorCode_t STTTX_GetDataIdentifier( STTTX_Handle_t     Handle,
                                        U8                 *DataIdentifier )
{
    stttx_context_t     *ThisElem;                  /* element for this Open */
    U32                 OpenIndex;                  /* Open Object number */


    /* Perform parameter validity checks */
    if( IsHandleInvalid( Handle,
                    &ThisElem, &OpenIndex ) )       /* Handle passed invalid? */
    {
        return( ST_ERROR_INVALID_HANDLE );
    }

    /* Check for NULL pointer */
    if( DataIdentifier == NULL )
    {
        return( ST_ERROR_BAD_PARAMETER );
    }

    /* Store current data identifier value */
    *DataIdentifier = ThisElem->Pes[OpenIndex].DataIdentifier;

    return( ST_NO_ERROR );
}

/****************************************************************************
Name         : STTTX_SetRequest()

Description  : Requests receipt of filtered teletext packets to be
               notified to the consumer.  MUST have been preceded by
               a successful Open call with the Handle specified.

Parameters   : STTTX_Handle_t with a valid open Handle number, and
               STTTX_Request_t pointer to details of request.

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                  No errors occurred
               ST_ERROR_INVALID_HANDLE      Handle number wrong/not open
               ST_ERROR_BAD_PARAMETER       One or more invalid parameters
               STTTX_ERROR_NO_MORE_REQUESTS No more request slots available
               STTTX_ERROR_CANT_FILTER      Object specified as VBI type

See Also     : STTTX_Request_t
               STTTX_Open()
               STTTX_ClearRequest()
*****************************************************************************/

ST_ErrorCode_t STTTX_SetRequest( STTTX_Handle_t  Handle,
                                 STTTX_Request_t *Request )
{
    stttx_context_t     *ThisElem;                  /* element for this Handle */
    Handled_Request_t   *QueueReq;
    U32                 OpenIndex;                  /* Open Object number */
    U32                 i, j = REQ_SLOT_UNUSED;


    /* Perform parameter validity checks */

    if( Request == NULL )                           /* Request ptr. omitted? */
    {
        return( ST_ERROR_BAD_PARAMETER );
    }

    if( IsHandleInvalid( Handle,
                    &ThisElem, &OpenIndex ) )       /* Handle passed invalid? */
    {
        return( ST_ERROR_INVALID_HANDLE );
    }

    QueueReq = ThisElem->RequestsPtr;               /* point to request queue */

    /* Search the request queue for an existing tag that matches the caller's
       tag.  If one is found, the existing request is overwritten.  As this
       will not increase the number of active requests, it is valid to do on
       a full queue.  To save a second search for an empty slot if the tag is
       not found (i.e. new), a note is made of the first free slot found.     */

    for( i = 0;                                     /* loop on whole queue */
        i < ThisElem->InitParams.NumRequestsAllowed; i++ )
    {
        if( QueueReq[i].Request.RequestTag ==       /* Tag value match? */
                            Request->RequestTag )
        {
            j = i;                                  /* note its index */
            break;                                  /* overwrite existing req. */
        }

        if( ( j == REQ_SLOT_UNUSED ) &&             /* first unused slot found? */
            ( QueueReq[i].OpenOff ==
                            REQ_SLOT_UNUSED ) )
        {
            j = i;                                  /* note free slot index */
        }
    }

    if( ( i >= ThisElem->InitParams.NumRequestsAllowed) &&        /* new tag value? */
        ( j == REQ_SLOT_UNUSED ) )                  /* no empty slot? */
    {
        return( STTTX_ERROR_NO_MORE_REQUESTS );     /* all slots in use */
    }

    QueueReq[j].OpenOff = OpenIndex;                /* store Open association */
    QueueReq[j].Request = *Request;                 /* structure copy data */


    /* Increment the active requests counter */
    ThisElem->NumActiveRequests++;

    /* if the filter is for whole page, set up offsets accordingly
       the offsets keep track of how many lines have been copied so far.
       =0 indicates we are looking for the start and =-1 indicates not looking
       for lines of this parity */
    if((QueueReq[j].Request.RequestType & STTTX_FILTER_PAGE_COMPLETE_ODD) != 0)
        QueueReq[j].OddOffset = 0;
    else
        QueueReq[j].OddOffset = -1;

    if((QueueReq[j].Request.RequestType & STTTX_FILTER_PAGE_COMPLETE_EVEN) != 0)
        QueueReq[j].EvenOffset = 0;
    else
        QueueReq[j].EvenOffset = -1;

    return( ST_NO_ERROR );
}

/****************************************************************************
Name         : STTTX_SetSource()

Description  : Changes the data source for the STTTX handle.

Parameters   : STTTX_Handle_t with a valid open Handle number, and
               STTTX_SourceParams_t pointer to details of source.

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                  No errors occurred
               ST_ERROR_INVALID_HANDLE      Handle number wrong/not open
               ST_ERROR_BAD_PARAMETER       One or more invalid parameters
               ST_ERROR_DECODER_RUNNING     Can't change source while running

See Also     : STTTX_Start()
               STTTX_Stop()
*****************************************************************************/
ST_ErrorCode_t STTTX_SetSource(STTTX_Handle_t Handle,
                               STTTX_SourceParams_t *SourceParams_p)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    stttx_context_t     *ThisElem;                  /* element for this Handle */
    data_store_t        *CurrPes;                   /* pointer into Pes[] */
    U32                 OpenIndex;                  /* Open Object number */


    /* Perform parameter validity checks */
    if( IsHandleInvalid( Handle,
                        &ThisElem, &OpenIndex ) )   /* Handle passed invalid? */
    {
        error = ST_ERROR_INVALID_HANDLE;
    }

    if (SourceParams_p == NULL)
    {
        error = ST_ERROR_BAD_PARAMETER;
    }

    /* More checks. */
    if (SourceParams_p->DataSource == STTTX_SOURCE_USER_BUFFER)
    {
        if ((SourceParams_p->Source_u.UserBuf_s.PesBuf_p == NULL) ||
            (SourceParams_p->Source_u.UserBuf_s.DataReady_p == NULL))
        {
            error = ST_ERROR_BAD_PARAMETER;
        }
    }

    if ((SourceParams_p->DataSource < 0) ||
        (SourceParams_p->DataSource > STTTX_SOURCE_USER_BUFFER))
    {
        error = ST_ERROR_BAD_PARAMETER;
    }

    /* All okay? */
    if (error == ST_NO_ERROR)
    {
        CurrPes = &ThisElem->Pes[OpenIndex];            /* abbreviate */

        if( CurrPes->Playing )                          /* already playing? */
        {
            error = STTTX_ERROR_DECODER_RUNNING;
        }
        else
        {
            /* Copy the new parameters */
            CurrPes->SourceParams = *SourceParams_p;

#if defined(DVD_TRANSPORT_PTI) || defined(DVD_TRANSPORT_LINK)
            if (SourceParams_p->DataSource == STTTX_SOURCE_PTI_SLOT)
            {
                CurrPes->Slot = SourceParams_p->Source_u.PTISlot_s.Slot;
            }
#endif
        }
    }

    return error;
}
#ifdef STTTX_SUBT_SYNC_ENABLE
/****************************************************************************
Name         : STTTX_SetSyncOffset()

Description  : Sets the time range in which the teletext SUBTITLE data is
               displayed.

Parameters   : STTTX_Handle_t with a valid open Handle number.
               SyncTime, Pointer to the value of synchronization time range
               in milliseconds.
Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                 No errors occurred
               ST_ERROR_INVALID_HANDLE     Handle number wrong/not open

See Also     : STTTX_Stop()
               STTTX_Open()
               STTTX_Close()
*****************************************************************************/

ST_ErrorCode_t STTTX_SetSyncOffset( STTTX_Handle_t Handle, U32 SyncOffset )
{
    stttx_context_t     *ThisElem;                  /* element for this Handle */
    U32                 OpenIndex;                  /* Open Object number */

    if( IsHandleInvalid( Handle,
                    &ThisElem, &OpenIndex ) )       /* Handle passed invalid? */
    {
        return( ST_ERROR_INVALID_HANDLE );
    }
    STOS_SemaphoreWait( Atomic_p );
    ThisElem->SubtSyncOffset = ( 90 * SyncOffset );
    STOS_SemaphoreSignal( Atomic_p );
    return(ST_NO_ERROR);
}
#endif

/****************************************************************************
Name         : STTTX_Start()

Description  : Start processing teletext packets.  MUST have been preceded
               by a successful Open call with the Handle given, and a
               SetStreamID call.

Parameters   : STTTX_Handle_t with a valid open Handle number.

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                 No errors occurred
               ST_ERROR_INVALID_HANDLE     Handle number wrong/not open

See Also     : STTTX_Stop()
               STTTX_Open()
               STTTX_Close()
*****************************************************************************/

ST_ErrorCode_t STTTX_Start( STTTX_Handle_t Handle )
{
    stttx_context_t     *ThisElem;                  /* element for this Handle */
    data_store_t        *CurrPes;                   /* pointer into Pes[] */
    U32                 OpenIndex;                  /* Open Object number */


    /* Perform parameter validity checks */

    if( IsHandleInvalid( Handle,
                    &ThisElem, &OpenIndex ) )       /* Handle passed invalid? */
    {
        return( ST_ERROR_INVALID_HANDLE );
    }

    CurrPes = &ThisElem->Pes[OpenIndex];            /* abbreviate */

    if( CurrPes->Playing )                          /* already playing? */
    {
        return( ST_NO_ERROR );                      /* no more to do */
    }

    /* Make sure that we don't try to start while the channel is in the
     * process of stopping. Could use a flag but this means we don't
     * have to busy-wait.
     */
    STOS_SemaphoreWait(CurrPes->Stopping_p);

#if defined(DVD_TRANSPORT_PTI) || defined(DVD_TRANSPORT_LINK)
    if (CurrPes->SourceParams.DataSource == STTTX_SOURCE_PTI_SLOT)
    {
        U8 temp;

        temp = pti_set_buffer( CurrPes->Slot,          /* association failure? */
                        stream_type_pes,
                        CurrPes->CircleBase,
                        CurrPes->CircleSize,
                        &CurrPes->CircleProd,
                        &CurrPes->CircleCons,
                        CurrPes->WaitForPES_p,   /* signal_t type (void *) */
                        pti_flags );
        if (temp)
        {
            STTBX_Print(("Error setting buffer\n"));
            STOS_SemaphoreSignal(CurrPes->Stopping_p);
            return STTTX_ERROR_CANT_START;
        }

        if( pti_set_pid(                                /* activate PTI on PID */
            CurrPes->Slot, CurrPes->PIDNum ) )
        {
            STOS_SemaphoreSignal(CurrPes->Stopping_p);
            return( STTTX_ERROR_CANT_START );           /* PTI failure */
        }

#ifdef DVD_TRANSPORT_LINK
        /* reset semaphore for task pooling synchronization. counters are
         * reset by the relevant functions.
         */
        CurrPes->PESArrived_p = STOS_SemaphoreCreateFifo(NULL, 0);
        /* Start the link sending data */
        pti_start_slot(CurrPes->Slot);
#endif
    }
#endif

    /* Flag stream as 'playing' */
    CurrPes->Playing = TRUE;
    CurrPes->HandlerRunning = TRUE;
    /* Reset last event */
    CurrPes->LastEvent = 0;
    ThisElem->OddDataReady = FALSE;
    ThisElem->EvenDataReady = FALSE;
    STOS_SemaphoreSignal(CurrPes->WakeUp_p);
    STOS_SemaphoreSignal(CurrPes->PoolingWakeUp_p);
    STOS_SemaphoreSignal(CurrPes->Stopping_p);

    return( ST_NO_ERROR );
}


/****************************************************************************
Name         : STTTX_Stop()

Description  : Stops processing teletext packets associated with Handle.

Parameters   : STTTX_Handle_t with a valid open Handle number.

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                 No errors occurred
               ST_ERROR_INVALID_HANDLE     Handle number wrong/not open

See Also     : STTTX_Open()
               STTTX_Close()
*****************************************************************************/

ST_ErrorCode_t STTTX_Stop( STTTX_Handle_t Handle )
{
    stttx_context_t     *ThisElem;                  /* element for this Handle */
    data_store_t        *CurrPes;                   /* pointer into Pes[] */
    U32                 OpenIndex;                  /* Open Object number */


    /* Perform parameter validity checks */

    if( IsHandleInvalid( Handle,
                    &ThisElem, &OpenIndex ) )       /* Handle passed invalid? */
    {
        return( ST_ERROR_INVALID_HANDLE );
    }

    CurrPes = &ThisElem->Pes[OpenIndex];            /* abbreviate */

    if( !CurrPes->Playing )                         /* not playing? */
    {
        return( ST_NO_ERROR );                      /* no more to do */
    }

    /* Make sure start and stop are serialised */
    STOS_SemaphoreWait(CurrPes->Stopping_p);

    /* Clear these, so we don't output any data */
    ThisElem->OddDataReady = FALSE;
    ThisElem->EvenDataReady = FALSE;

#if defined(DVD_TRANSPORT_PTI) || defined(DVD_TRANSPORT_LINK)
    if (CurrPes->SourceParams.DataSource == STTTX_SOURCE_PTI_SLOT)
    {
#ifdef DVD_TRANSPORT_LINK
        pti_stop_slot(CurrPes->Slot);
#endif

        if( pti_clear_pid(                              /* deactivate PTI on PID */
            ThisElem->Pes[OpenIndex].Slot ) )
        {
            STOS_SemaphoreSignal(CurrPes->Stopping_p);
            return( STTTX_ERROR_CANT_STOP );            /* PTI failure */
        }

    }
#endif

    /* Setting this flag will cause the data-pooling task to stop. That
     * will then cause the handler task to stop. Note that we're still
     * holding the Stopping semaphore; that's signalled by the handler
     * task when it's finished discarding all remaining data.
     */
    CurrPes->Playing = FALSE;
    CurrPes->PIDNum = INVALID_PID;                  /* Flag PID as unknown */
    STOS_SemaphoreSignal (CurrPes->PESArrived_p);

    /* Force WaitForData to wakeup if user buffer */
    if (CurrPes->SourceParams.DataSource == STTTX_SOURCE_USER_BUFFER)
        STOS_SemaphoreSignal(CurrPes->SourceParams.Source_u.UserBuf_s.DataReady_p);

    return( ST_NO_ERROR );
}


/****************************************************************************
Name         : STTTX_ClearRequest()

Description  : Clears a request previously made by SetRequest().

Parameters   : STTTX_Handle_t with a valid open Handle number,
               U32 RequestTag matching that previously Set.

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                 No errors occurred
               ST_ERROR_INVALID_HANDLE     Handle number wrong/not open
               STTTX_ERROR_CANT_CLEAR      Some other error has occurred

See Also     : STTTX_SetRequest()
*****************************************************************************/

ST_ErrorCode_t STTTX_ClearRequest( STTTX_Handle_t  Handle,
                                   U32             RequestTag )
{
    stttx_context_t     *ThisElem;                  /* element for this Handle */
    Handled_Request_t   *QueueReq;
    U32                 OpenIndex;                  /* Open Object number */
    U32                 i;


    /* Perform parameter validity checks */

    if( IsHandleInvalid( Handle,
                    &ThisElem, &OpenIndex ) )       /* Handle passed invalid? */
    {
        return( ST_ERROR_INVALID_HANDLE );
    }

    QueueReq = ThisElem->RequestsPtr;               /* point to request queue */

    for( i = 0;                                     /* loop on whole queue */
        i < ThisElem->InitParams.NumRequestsAllowed; i++ )
    {
        if( QueueReq[i].Request.RequestTag ==       /* Tag value match? */
                                    RequestTag )
        {
            break;                                  /* existing request */
        }
    }

    if( i >= ThisElem->InitParams.NumRequestsAllowed)             /* tag not found? */
    {
        return( STTTX_ERROR_CANT_CLEAR );           /* can't find tag in queue */
    }

    QueueReq[i].OpenOff = REQ_SLOT_UNUSED;          /* flag as free */
    QueueReq[i].Request.RequestTag =
                        STTTX_FREE_REQUEST_TAG;     /* invalid tag value */

    /* decrement the active requests counter */
    ThisElem->NumActiveRequests--;

    return( ST_NO_ERROR );
}

/****************************************************************************
Name         : STTTX_Close()

Description  : Closes the Handle for re-use, the PID is overwritten with
               an invalid value and the PTI resource is deallocated.

Parameters   : STTTX_Handle_t

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                     No errors determined
               ST_ERROR_INVALID_HANDLE         No Open on this Handle

See Also     : STCOUNT_InitParams_t
               STCOUNT_Init()
               STCOUNT_Open()
*****************************************************************************/

ST_ErrorCode_t STTTX_Close( STTTX_Handle_t Handle )
{
    stttx_context_t     *ThisElem;                  /* element for this Handle */
    U32                 OpenIndex;                  /* Open Object number */

    /* Perform parameter validity checks */

    if( IsHandleInvalid( Handle,
                    &ThisElem, &OpenIndex ) )       /* Handle passed invalid? */
    {
        return( ST_ERROR_INVALID_HANDLE );
    }

#if defined(DVD_TRANSPORT_PTI) || defined(DVD_TRANSPORT_LINK)
    if (ThisElem->Pes[OpenIndex].SourceParams.DataSource == STTTX_SOURCE_PTI_SLOT)
    {
#ifdef DVD_TRANSPORT_LINK
        pti_stop_slot(ThisElem->Pes[OpenIndex].Slot);
#endif

        /* deactivate PTI on PID */
        if(pti_clear_pid(ThisElem->Pes[OpenIndex].Slot))
        {
            return( STTTX_ERROR_CANT_STOP );        /* PTI failure */
        }
    }
#endif

    /* Start of atomic region */
    STOS_SemaphoreWait(Atomic_p);

    ThisElem->ObjectsOpen--;                        /* decrement number open */
    ThisElem->Pes[OpenIndex].PIDNum = INVALID_PID;
    ThisElem->Pes[OpenIndex].Playing = FALSE;
    ThisElem->Pes[OpenIndex].Open = FALSE;          /* flag as closed */
    /* If we were the VBI handle, unsubscribe from the VTG and clear the
     * flag for someone else. Do it in this order to avoid the race
     * condition on the event subscribe/unsubscribe that would otherwise
     * result.
     */
    if (ThisElem->Pes[OpenIndex].Mode != STTTX_STB)
    {
        STEVT_UnsubscribeDeviceEvent(ThisElem->EVTHandle,
                                     NULL, STVTG_VSYNC_EVT);
        ThisElem->VBIUsed = FALSE;
    }

    /* Tell task to exit */
    ThisElem->Pes[OpenIndex].TasksActive = FALSE;

    /* Make sure it's had a chance to see the message */
    STOS_SemaphoreSignal(ThisElem->Pes[OpenIndex].WakeUp_p);

    STOS_SemaphoreSignal(ThisElem->Pes[OpenIndex].PoolingWakeUp_p);
    /* simulate signal, to make *sure* it's seen it, No way to force an
       STPTI signal yet - timeout on signalwait should suffice until
       there is. */

#if defined(DVD_TRANSPORT_PTI) || defined(DVD_TRANSPORT_LINK)
    if (ThisElem->Pes[OpenIndex].SourceParams.DataSource == STTTX_SOURCE_PTI_SLOT)
        STOS_SemaphoreSignal( ThisElem->Pes[OpenIndex].WaitForPES_p );
#endif
    if (ThisElem->Pes[OpenIndex].SourceParams.DataSource == STTTX_SOURCE_USER_BUFFER)
        STOS_SemaphoreSignal(ThisElem->Pes[OpenIndex].SourceParams.Source_u.UserBuf_s.DataReady_p);

    STOS_SemaphoreSignal(ThisElem->Pes[OpenIndex].PESArrived_p);

#if defined(STTTX_DEBUG)
    STTBX_Print(("PES 0x%08x; tasks active: %i; element tasks active: %i\n",
            (U32)&ThisElem->Pes[OpenIndex],
            ThisElem->Pes[OpenIndex].TasksActive,
            ThisElem->TasksActive));
#if (!defined (ST_5105)) && (!defined (ST_7020)) && (!defined(ST_5188)) && (!defined(ST_5107))
    STTBX_Print(("PES 0x%08x\tInterrupt status: 0x%04x\n",
            (U32)&ThisElem->Pes[OpenIndex],
            ThisElem->BaseAddress[STTTX_INT_STATUS] & 0x7));
#endif
#endif
    STTBX_Print(("Waiting for ttx task to end\n"));
    /* Then wait for the tasks to complete */
    STOS_TaskWait(&ThisElem->Pes[OpenIndex].TTXttask_p,TIMEOUT_INFINITY);
    STOS_TaskDelete(ThisElem->Pes[OpenIndex].TTXttask_p,(partition_t *)ThisElem->InitParams.DriverPartition,
                                ThisElem->Pes[OpenIndex].TTXStack_p,(partition_t *)ThisElem->InitParams.DriverPartition);

    STTBX_Print(("Waiting for pooling task to end\n"));
    STOS_TaskWait(&ThisElem->Pes[OpenIndex].TTXPoolttask_p, TIMEOUT_INFINITY);
    STOS_TaskDelete(ThisElem->Pes[OpenIndex].TTXPoolttask_p,(partition_t *)ThisElem->InitParams.DriverPartition,
                                ThisElem->Pes[OpenIndex].TTXPoolStack_p,(partition_t *)ThisElem->InitParams.DriverPartition);

    STTBX_Print(("Deleting semaphores\n"));
    STOS_SemaphoreDelete(NULL,ThisElem->Pes[OpenIndex].WaitForPES_p);
    STOS_SemaphoreDelete(NULL,ThisElem->Pes[OpenIndex].WakeUp_p);
    STOS_SemaphoreDelete(NULL,ThisElem->Pes[OpenIndex].PoolingWakeUp_p);
    STOS_SemaphoreDelete(NULL,ThisElem->Pes[OpenIndex].PESArrived_p);
    STOS_SemaphoreDelete(NULL,ThisElem->Pes[OpenIndex].Stopping_p);

    /* End of atomic region */
    STOS_SemaphoreSignal(Atomic_p);

    return( ST_NO_ERROR );
}


/****************************************************************************
Name         : STTTX_Term()

Description  : Terminates the named device driver, effectively reversing
               its Initialization call and any Open calls if forced.

Parameters   : ST_DeviceType_t name matching that supplied with Init,
               ST_TermParams_t pointer containing ForceTerminate flag.

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                     No errors determined
               ST_ERROR_UNKNOWN_DEVICE         Invalid device name
               ST_ERROR_BAD_PARAMETER          One or more parameters invalid
               ST_ERROR_OPEN_HANDLE            Handle not Closed

See Also     : STTTX_Init()
               STTTX_RemoveEvent()
*****************************************************************************/

ST_ErrorCode_t STTTX_Term( const ST_DeviceName_t Name,
                           const STTTX_TermParams_t *TermParams )
{
    S32                 i, j;
    ST_ErrorCode_t      RetValue = ST_NO_ERROR, error;
#ifdef ST_7020
    STAVMEM_FreeBlockParams_t   FreeParams;
#endif
    stttx_context_t     *PrevElem=NULL, *ThisElem;

    /* Perform parameter validity checks */

    if( ( Name       == NULL )   ||                 /* NULL Name ptr? */
        ( TermParams == NULL ) )                    /* NULL structure ptr? */
    {
        return( ST_ERROR_BAD_PARAMETER );
    }

    if( stttx_Sentinel == NULL )                    /* no Inits outstanding? */
    {
        return( ST_ERROR_UNKNOWN_DEVICE );          /* Semaphore not created */
    }

    STOS_SemaphoreWait( Atomic_p );                      /* start of atomic region */

    for( ThisElem = stttx_Sentinel;                 /* start at Sentinel node */
        ThisElem != NULL;                           /* while not at list end */
            ThisElem = ThisElem->Next )             /* advance to next element */
    {
        if( strcmp( Name, ThisElem->Name ) == 0 )   /* existing DeviceName? */
        {
            break;
        }

        PrevElem = ThisElem;                        /* keep back marker */
    }

    if( ThisElem == NULL )                          /* no Name match found? */
    {
        STOS_SemaphoreSignal( Atomic_p );                /* end atomic before return */

        return( ST_ERROR_UNKNOWN_DEVICE );
    }

    if( ThisElem->ObjectsOpen != 0 )                /* are objects still open? */
    {
        if( !TermParams->ForceTerminate )           /* unforced Closure? */
        {
            STOS_SemaphoreSignal( Atomic_p );            /* end atomic before return */

            return( ST_ERROR_OPEN_HANDLE );         /* user needs to call Close */
        }

        j = ThisElem->InitParams.NumSTBObjects +    /* total object count */
                                MAX_VBI_OBJECTS;
        for( i = 0; i < j; i++ )                    /* loop on all objects */
        {
            if( ThisElem->Pes[i].Open )
            {
                ThisElem->ObjectsOpen--;

                /* Tell the tasks to exit */
                ThisElem->Pes[i].TasksActive = FALSE;
                ThisElem->Pes[i].Playing = FALSE;

                /* Send this, in case they're idling at the time. */
                STOS_SemaphoreSignal(ThisElem->Pes[i].WakeUp_p);
                STOS_SemaphoreSignal(ThisElem->Pes[i].PoolingWakeUp_p);
                STOS_SemaphoreSignal(ThisElem->Pes[i].PESArrived_p);
                /* simulate signal, to get them out of any data waiting.
                 * No way to force an STPTI signal yet; timeout should
                 * suffice.
                 */
#if defined(DVD_TRANSPORT_PTI) || defined(DVD_TRANSPORT_LINK)
                if (ThisElem->Pes[i].SourceParams.DataSource == STTTX_SOURCE_PTI_SLOT)
                {
#ifdef DVD_TRANSPORT_LINK
                    pti_stop_slot(ThisElem->Pes[i].Slot);
#endif
                    /* deactivate PTI on PID */
                    if (pti_clear_pid(ThisElem->Pes[i].Slot))
                    {
                        /* PTI failure */
                        return(STTTX_ERROR_CANT_STOP);
                    }

                    STOS_SemaphoreSignal( ThisElem->Pes[i].WaitForPES_p);
                }
#endif
                if (ThisElem->Pes[i].SourceParams.DataSource == STTTX_SOURCE_USER_BUFFER)
                {
                    STOS_SemaphoreSignal(ThisElem->Pes[i].SourceParams.Source_u.UserBuf_s.DataReady_p);
                }

                /* Wait. */
                STOS_TaskWait( &ThisElem->Pes[i].TTXttask_p, TIMEOUT_INFINITY );
                STOS_TaskDelete(ThisElem->Pes[i].TTXttask_p,(partition_t *)ThisElem->InitParams.DriverPartition,
                                ThisElem->Pes[i].TTXStack_p,(partition_t *)ThisElem->InitParams.DriverPartition);
                STOS_TaskWait(&ThisElem->Pes[i].TTXPoolttask_p, TIMEOUT_INFINITY);
                STOS_TaskDelete(ThisElem->Pes[i].TTXPoolttask_p,(partition_t *)ThisElem->InitParams.DriverPartition,
                                ThisElem->Pes[i].TTXPoolStack_p,(partition_t *)ThisElem->InitParams.DriverPartition);

                ThisElem->Pes[i].PIDNum = INVALID_PID;
                ThisElem->Pes[i].Open = FALSE;      /* flag as closed */

                /* Then delete the semaphores */
                STOS_SemaphoreDelete(NULL,ThisElem->Pes[i].WaitForPES_p);
                STOS_SemaphoreDelete(NULL,ThisElem->Pes[i].WakeUp_p);
                STOS_SemaphoreDelete(NULL,ThisElem->Pes[i].PoolingWakeUp_p);
                STOS_SemaphoreDelete(NULL,ThisElem->Pes[i].PESArrived_p);

                if (ThisElem->Pes[i].Mode != STTTX_STB)
                {
                    /* Unsubscribe VSYNC if any one left */
                    error = STEVT_UnsubscribeDeviceEvent(ThisElem->EVTHandle,
                                                         NULL, STVTG_VSYNC_EVT);
                    if (ST_NO_ERROR != error)
                    {
                        STTBX_Print(("Error 0x%08x unsubscribing from event\n", (U32)error));
                    }
                }
            }
        }
    }

    /* Unregister events. */
    if (ThisElem->InitParams.EVTDeviceName != NULL)
        UnregisterEvents(ThisElem);

    /* free the memory used */
    for( i = ThisElem->InitParams.NumSTBObjects + /* reverse loop all objects */
                        MAX_VBI_OBJECTS - 1;
                            i >= VBI_SLOT; i-- )
    {
        STOS_MessageQueueDelete(ThisElem->Pes[i].PESQueue_p);

        /* DDTS 28415 */
#if defined(DVD_TRANSPORT_PTI)
        STOS_MemoryDeallocate(
            (partition_t *)ThisElem->InitParams.DriverPartition,
            ThisElem->Pes[i].CircleBuff
        );
#endif
    }

#ifdef ST_7020
    FreeParams.PartitionHandle = ThisElem->InitParams.AVMEMPartition;
    error = STAVMEM_FreeBlock(&FreeParams, &ThisElem->AVMEMBlockHandle);
    if (ST_NO_ERROR != error)
    {
        STTBX_Print(("Error 0x%08x deallocating AVMEM block\n", (U32)error));
    }
#endif

#if (!defined (ST_5105)) && (!defined (ST_7020)) && (!defined(ST_5188)) && (!defined(ST_5107))

#if !defined(ILC_1)
    if (STOS_InterruptDisable(ThisElem->InitParams.InterruptNumber,
                              ThisElem->InitParams.InterruptLevel) != 0)
    {
        RetValue = ST_ERROR_INTERRUPT_UNINSTALL;
    }
#endif

    /* Uninstall the interrupt handler */
    if ( STOS_InterruptUninstall(
            ThisElem->InitParams.InterruptNumber,
            ThisElem->InitParams.InterruptLevel,
            (void *)ThisElem ) != 0 )
    {
        RetValue = ST_ERROR_INTERRUPT_UNINSTALL;
    }
#endif /* ST_5105/07/88, ST_7020 */

    if( ThisElem == stttx_Sentinel )
    {
        stttx_Sentinel = ThisElem->Next;            /* remove first item */
    }
    else
    {
        PrevElem->Next = ThisElem->Next;            /* remove mid-list item */
    }

    STOS_SemaphoreSignal( Atomic_p );                    /* end of atomic region */

    /* Close the output handle */
#if !defined(STTTX_USE_DENC)
    STVBI_Close(ThisElem->VBIHandle);
#else
    STDENC_Close(ThisElem->DENCHandle);
#endif

#if defined(ST_5105) || defined(ST_5188) || defined(ST_5107)
    STVMIX_CloseVBIViewPort(ThisElem->VBIVPDisplayOdd);
    STVMIX_CloseVBIViewPort(ThisElem->VBIVPDisplayEven);
    STVMIX_Close(ThisElem->VMIXHandle);
#endif

#ifdef STTTX_SUBT_SYNC_ENABLE
    STCLKRV_Close(ThisElem->CLKRVHandle);
#endif

#if defined (TTXDMA_NOT_COMPLETE_WORKAROUND)
    /* To be seen more carefully */
    ThisElem->TimeOutTaskActive = FALSE;
    STOS_SemaphoreSignal(ThisElem->ISRTimeOutWakeUp_p);
    STOS_SemaphoreSignal(ThisElem->TTXDMATimeOut_p);
    STOS_TaskWait(&ThisElem->ISRTimeOuttask_p, TIMEOUT_INFINITY);
    STOS_TaskDelete(ThisElem->ISRTimeOuttask_p,(partition_t *)ThisElem->InitParams.DriverPartition,
                    ThisElem->ISRTimeOutStack_p,(partition_t *)ThisElem->InitParams.DriverPartition);

    STOS_SemaphoreDelete(NULL,ThisElem->ISRTimeOutWakeUp_p);
    STOS_SemaphoreDelete(NULL,ThisElem->TTXDMATimeOut_p);
#endif

    STOS_SemaphoreDelete(NULL,ThisElem->ISRComplete_p);

    STOS_MemoryDeallocate(ThisElem->InitParams.DriverPartition, ThisElem);

    return RetValue;
} /* STTTX_Term() */


/****************************************************************************
Name         : IsHandleInvalid()

Description  : Multi-purpose procedure to check Handle validity, plus return
               the address of its data structure element and the Open Index.

Parameters   : STTTX_Handle_t Handle number, STTTX_context_t *Construct
               pointer to element within linked list, and U32* OpenIndex.

Return Value : TRUE  if Handle is invalid
               FALSE if Handle appears to be legal
*****************************************************************************/

static BOOL IsHandleInvalid( STTTX_Handle_t  Handle,
                             stttx_context_t **Construct,
                             U32*            OpenIndex )
{
    STTTX_Handle_t      TestHandle;
    stttx_context_t     *ThisElem;
    U32                 OpenOffset;


    TestHandle = Handle & BASE_HANDLE_MSK;          /* strip off Open count */

    for( ThisElem = stttx_Sentinel;                 /* start at Sentinel node */
        ThisElem != NULL;                           /* while not at list end */
            ThisElem = ThisElem->Next )             /* advance to next element */
    {
        if( ThisElem->BaseHandle == TestHandle )    /* located Construct? */
        {
            OpenOffset = Handle & OPEN_HANDLE_MSK;  /* reveal Open offset only */

            if( !ThisElem->Pes[OpenOffset].Open )   /* closed? */
            {
                return TRUE;                        /* Invalid OpenHandle */
            }

            *Construct = ThisElem;                  /* write-back location */
            *OpenIndex = OpenOffset;                /*  and Open Index */
            return FALSE;                           /* indicate valid Handle */
        }
    }

    return TRUE;                                    /* Invalid BaseHandle */
} /* IsHandleInvalid() */


/****************************************************************************
Name         : IsOutsideRange()

Description  : Checks if U32 formal parameter 1 is outside the range of
               bounds (U32 formal parameters 2 and 3).

Parameters   : Value for checking, followed by the two bounds, preferably
               the lower bound followed by the upper bound.

Return Value : TRUE  if parameter is outside given range
               FALSE if parameter is within  given range
*****************************************************************************/

static BOOL IsOutsideRange( U32 ValuePassed,
                            U32 LowerBound,
                            U32 UpperBound )
{
    U32 Temp;

    /* ensure bounds are the correct way around */
    if( UpperBound < LowerBound ) /* bounds require swapping */
    {
        Temp       = LowerBound;
        LowerBound = UpperBound;
        UpperBound = Temp;
    }

    if( ( ValuePassed < LowerBound ) ||
        ( ValuePassed > UpperBound ) )
    {
        return( TRUE );
    }
    else
    {
        return( FALSE );
    }
} /* IsOutsideRange() */

/****************************************************************************
Name         : RegisterEvents()

Description  : Opens a connection to the event handler device and registers
               all teletext events.

Parameters   : ThisElem,    pointer to the device context.

Return Value : ST_ErrorCode_t returned by event handler.
*****************************************************************************/

static ST_ErrorCode_t RegisterEvents(stttx_context_t *ThisElem)
{
    ST_ErrorCode_t Error, Overall = ST_NO_ERROR;
    STEVT_OpenParams_t  EVTOpenParams;

    /* Open event handler connection and register events */
    Error = STEVT_Open(ThisElem->InitParams.EVTDeviceName,
                       &EVTOpenParams,
                       &ThisElem->EVTHandle);

    if (Error != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Call to STEVT_Open() failed"));
        Overall = Error;
    }

    Error = STEVT_RegisterDeviceEvent(ThisElem->EVTHandle, ThisElem->Name,
                           STTTX_EVENT_PES_LOST,
                           &ThisElem->RegisteredEvents[EventId(STTTX_EVENT_PES_LOST)]);

    if (Error != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Call to STEVT_Register(STTTX_EVENT_PES_LOST) failed"));
        Overall = Error;
    }

    Error = STEVT_RegisterDeviceEvent(ThisElem->EVTHandle, ThisElem->Name,
                           STTTX_EVENT_PES_NOT_CONSUMED,
                           &ThisElem->RegisteredEvents[EventId(STTTX_EVENT_PES_NOT_CONSUMED)]);

    if (Error != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Call to STEVT_Register(STTTX_EVENT_PES_NOT_CONSUMED) failed"));
        Overall = Error;
    }

    Error = STEVT_RegisterDeviceEvent(ThisElem->EVTHandle, ThisElem->Name,
                           STTTX_EVENT_PES_INVALID,
                           &ThisElem->RegisteredEvents[EventId(STTTX_EVENT_PES_INVALID)]);

    if (Error != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Call to STEVT_Register(STTTX_EVENT_PES_INVALID) failed"));
        Overall = Error;
    }

    Error = STEVT_RegisterDeviceEvent(ThisElem->EVTHandle, ThisElem->Name,
                           STTTX_EVENT_PES_AVAILABLE,
                           &ThisElem->RegisteredEvents[EventId(STTTX_EVENT_PES_AVAILABLE)]);

    if (Error != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Call to STEVT_Register(STTTX_EVENT_PES_AVAILABLE) failed"));
        Overall = Error;
    }

    Error = STEVT_RegisterDeviceEvent(ThisElem->EVTHandle, ThisElem->Name,
                           STTTX_EVENT_PACKET_DISCARDED,
                           &ThisElem->RegisteredEvents[EventId(STTTX_EVENT_PACKET_DISCARDED)]);

    if (Error != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Call to STEVT_Register(STTTX_EVENT_PACKET_DISCARDED) failed"));
        Overall = Error;
    }

    Error = STEVT_RegisterDeviceEvent(ThisElem->EVTHandle, ThisElem->Name,
                           STTTX_EVENT_PACKET_CONSUMED,
                           &ThisElem->RegisteredEvents[EventId(STTTX_EVENT_PACKET_CONSUMED)]);

    if (Error != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Call to STEVT_Register(STTTX_EVENT_PACKET_CONSUMED) failed"));
        Overall = Error;
    }

    Error = STEVT_RegisterDeviceEvent(ThisElem->EVTHandle, ThisElem->Name,
                           STTTX_EVENT_DATA_OVERFLOW,
                           &ThisElem->RegisteredEvents[EventId(STTTX_EVENT_DATA_OVERFLOW)]);

    if (Error != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Call to STEVT_Register(STTTX_EVENT_DATA_OVERFLOW) failed"));
        Overall = Error;
    }

    Error = STEVT_RegisterDeviceEvent(ThisElem->EVTHandle, ThisElem->Name,
                           STTTX_EVENT_VPS_DATA,
                           &ThisElem->RegisteredEvents[EventId(STTTX_EVENT_VPS_DATA)]);

    if (Error != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Call to STEVT_Register(STTTX_EVENT_VPS_DATA) failed"));
        Overall = Error;
    }

    Error = STEVT_RegisterDeviceEvent(ThisElem->EVTHandle, ThisElem->Name,
                           STTTX_EVENT_TIME_DIFFERENCE_TOO_LARGE,
                           &ThisElem->RegisteredEvents[EventId(STTTX_EVENT_TIME_DIFFERENCE_TOO_LARGE)]);

    if (Error != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Call to STEVT_Register(STTTX_EVENT_TIME_DIFFERENCE_TOO_LARGE) failed"));
        Overall = Error;
    }

    Error = STEVT_RegisterDeviceEvent(ThisElem->EVTHandle, ThisElem->Name,
                           STTTX_EVENT_DECODER_TOO_SLOW,
                           &ThisElem->RegisteredEvents[EventId(STTTX_EVENT_DECODER_TOO_SLOW)]);

    if (Error != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Call to STEVT_Register(STTTX_EVENT_DECODER_TOO_SLOW) failed"));
        Overall = Error;
    }

    Error = STEVT_RegisterDeviceEvent(ThisElem->EVTHandle, ThisElem->Name,
                           STTTX_EVENT_DECODER_TOO_FAST,
                           &ThisElem->RegisteredEvents[EventId(STTTX_EVENT_DECODER_TOO_FAST)]);

    if (Error != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Call to STEVT_Register(STTTX_EVENT_DECODER_TOO_FAST) failed"));
        Overall = Error;
    }

    Error = STEVT_RegisterDeviceEvent(ThisElem->EVTHandle, ThisElem->Name,
                           STTTX_EVENT_NO_STC,
                           &ThisElem->RegisteredEvents[EventId(STTTX_EVENT_NO_STC)]);

    if (Error != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Call to STEVT_Register(STTTX_EVENT_NO_STC) failed"));
        Overall = Error;
    }

    return Overall;
}

/****************************************************************************
Name         : UnregisterEvents()

Description  : Unregisters all teletext events from the event handler and
               closes the current connection.

Parameters   : ThisElem,    pointer to the device context.

Return Value : ST_ErrorCode_t returned by event handler.
*****************************************************************************/

static ST_ErrorCode_t UnregisterEvents(stttx_context_t *ThisElem)
{
    ST_ErrorCode_t Error, Overall = ST_NO_ERROR;
    STEVT_Handle_t Handle = ThisElem->EVTHandle;

    /* Unregister all events */
    Error = STEVT_UnregisterDeviceEvent(Handle, ThisElem->Name,
                                STTTX_EVENT_PES_LOST);

    if (Error != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Call to STEVT_Unregister(STTTX_EVENT_PES_LOST) failed"));
        Overall = Error;
    }

    Error = STEVT_UnregisterDeviceEvent(Handle, ThisElem->Name,
                                STTTX_EVENT_PES_NOT_CONSUMED);

    if (Error != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Call to STEVT_Unregister(STTTX_EVENT_PES_NOT_CONSUMED) failed"));
        Overall = Error;
    }

    Error = STEVT_UnregisterDeviceEvent(Handle, ThisElem->Name,
                                STTTX_EVENT_PES_INVALID);

    if (Error != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Call to STEVT_Unregister(STTTX_EVENT_PES_INVALID) failed"));
        Overall = Error;
    }

    Error = STEVT_UnregisterDeviceEvent(Handle, ThisElem->Name,
                                STTTX_EVENT_PES_AVAILABLE);

    if (Error != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Call to STEVT_Unregister(STTTX_EVENT_PES_AVAILABLE) failed"));
        Overall = Error;
    }

    Error = STEVT_UnregisterDeviceEvent(Handle, ThisElem->Name,
                                STTTX_EVENT_PACKET_DISCARDED);

    if (Error != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Call to STEVT_Unregister(STTTX_EVENT_PACKET_DISCARDED) failed"));
        Overall = Error;
    }

    Error = STEVT_UnregisterDeviceEvent(Handle, ThisElem->Name,
                                STTTX_EVENT_PACKET_CONSUMED);

    if (Error != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Call to STEVT_Unregister(STTTX_EVENT_PACKET_CONSUMED) failed"));
        Overall = Error;
    }

    Error = STEVT_UnregisterDeviceEvent(Handle, ThisElem->Name,
                                STTTX_EVENT_DATA_OVERFLOW);

    if (Error != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Call to STEVT_Unregister(STTTX_EVENT_DATA_OVERFLOW) failed"));
        Overall = Error;
    }

    Error = STEVT_UnregisterDeviceEvent(Handle, ThisElem->Name,
                                STTTX_EVENT_VPS_DATA);

    if (Error != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Call to STEVT_Unregister(STTTX_EVENT_VPS_DATA) failed"));
        Overall = Error;
    }

    Error = STEVT_UnregisterDeviceEvent(Handle, ThisElem->Name,
                                STTTX_EVENT_TIME_DIFFERENCE_TOO_LARGE);

    if (Error != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Call to STEVT_Unregister(STTTX_EVENT_TIME_DIFFERENCE_TOO_LARGE) failed"));
        Overall = Error;
    }

    Error = STEVT_UnregisterDeviceEvent(Handle, ThisElem->Name,
                                STTTX_EVENT_DECODER_TOO_SLOW);

    if (Error != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Call to STEVT_Unregister(STTTX_EVENT_DECODER_TOO_SLOW) failed"));
        Overall = Error;
    }

    Error = STEVT_UnregisterDeviceEvent(Handle, ThisElem->Name,
                                STTTX_EVENT_DECODER_TOO_FAST);

    if (Error != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Call to STEVT_Unregister(STTTX_EVENT_DECODER_TOO_FAST) failed"));
        Overall = Error;
    }

    Error = STEVT_UnregisterDeviceEvent(Handle, ThisElem->Name,
                                STTTX_EVENT_NO_STC);

    if (Error != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Call to STEVT_Unregister(STTTX_EVENT_NO_STC) failed"));
        Overall = Error;
    }

    /* Close event handler connection */
    Error = STEVT_Close(Handle);

    if (Error != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Call to STEVT_Close() failed"));
        Overall = Error;
    }

    return Overall;
}

/* End of stttx.c */
