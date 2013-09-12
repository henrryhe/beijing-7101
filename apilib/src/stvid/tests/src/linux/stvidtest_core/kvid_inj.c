/************************************************************************
COPYRIGHT (C) STMicroelectronics 2005

File name   : kvid_inj.c
Description : Definition of video extra commands (utilities)
 *            in inject video stream into the bit buffer
 *
Note        : All functions return TRUE if error for Testtool compatibility
 *
Date          Modification                                      Initials
----          ------------                                      --------
01 Feb 2005   Creation (code moved from vid_util.c)             FQ
************************************************************************/

/* Defining the variable below allows to run video injection without a PTI, using C loops.
   This is usefull when wanting no dependency on STPTI, STPTI4, etc. */
/*#define STPTI_UNAVAILABLE*/

/*#define LINUX_INJECT_DEBUG*/

/*#######################################################################*/
/*########################## INCLUDES FILE ##############################*/
/*#######################################################################*/

#include <linux/sched.h>
#include <linux/syscalls.h>

#if !defined DVD_TRANSPORT_STPTI
#define DVD_TRANSPORT_STPTI
#endif
#undef DVD_TRANSPORT_PTI
#include "stos.h"
#include "stvidtest.h"

#include "testcfg.h"
#include "stlite.h"

#include "sttbx.h"

/* Driver as to be used as backend */
#define USE_AS_BACKEND
#include "genadd.h"

#include "stddefs.h"
#include "stdevice.h"
#ifdef ST_OS20
 #include "debug.h"
#endif /* ST_OS20 */
#ifdef ST_OS21
 #include "os21debug.h"
 #include <sys/stat.h>
 #include <fcntl.h>
#endif /* ST_OS24 */

#include "testtool.h"
#include "api_cmd.h"
#include "clevt.h"
#include "startup.h"
#if (defined (DVD_TRANSPORT_STPTI) || defined (DVD_TRANSPORT_STPTI4)) && !defined(STPTI_UNAVAILABLE)
 #include "stpti.h"
#endif
#if (defined (DVD_TRANSPORT_PTI) || defined (DVD_TRANSPORT_LINK)) && !defined(STPTI_UNAVAILABLE)
 #include "pti.h"
#endif
#ifdef DVD_TRANSPORT_LINK /* workaround for link 2.0.1 */
 #include "ptilink.h"
#endif
#include "stavmem.h"

#include "stsys.h"

#include "stevt.h"
#ifdef USE_CLKRV
 #include "stclkrv.h"
 #include "clclkrv.h"
#else
 #include "stcommon.h" /* for ST_GetClocksPerSecond() */
#endif /* USE_CLKRV */
#include "stvtg.h"
#include "vid_cmd.h"
#include "vid_util.h"
#include "vtg_cmd.h"

#if defined(ST_7710) || defined(ST_51xx_Device) /* || defined(ST_7100)  */
/* Allow the fdma transfer when using a lcmpegs1/lcmpegh1 based chip	*/
/* i.e. ST_51xx_Device, STi7100, STi7710, ...                                  */
/* This is to save bandwidth during data injection (better than memcpy)	*/
 #define USE_FDMA_TRANSFER
#endif

#if defined(USE_FDMA_TRANSFER)
 #include "clfdma.h"
#endif

#ifdef TRACE_UART
 #include "trace.h"
#endif

/* Private preliminary definitions (internal use only) ---------------------- */

#ifdef ST_OS20
#define STACK_SIZE 4092
#endif
#ifdef ST_OS21
#define STACK_SIZE 16*1024
#endif

/* Temporary: work-arounds 7015 and 7020 cut 1.0 */
#if defined(ST_7015)
#define STI70xx_WA_BBL_SPURIOUS_OVERFLOW
#define STI70xx_WA_LESS_SIZE (512*1024)
#endif

/* --- Constants (default values) -------------------------------------- */
/* The number of MS_TO_NEXT_TRANSFER milli-seconds to wait until next FDMA transfer for injecter based platforms */
#define MS_TO_NEXT_TRANSFER 16

#ifndef ARCHITECTURE_SPARC
#if defined(ST_7100) || defined(ST_7109) || defined (ST_7200) ||defined(ST_7710) || defined(ST_51xx_Device)
/* Injection task priority increased to be able to inject properly in HD mode !!!				*/
#define VID_INJECTION_TASK_PRIORITY (MIN_USER_PRIORITY+1) 	/* Stapidemo is MIN_USER_PRIORITY+8 */
#else
#define VID_INJECTION_TASK_PRIORITY (MIN_USER_PRIORITY) 	/* Stapidemo is MIN_USER_PRIORITY+8 */
#endif
#else
#define VID_INJECTION_TASK_PRIORITY    18
#endif

/* --- Global variables ------------------------------------------------ */
static U32 TicksPerOneSecond;

static task_t *VIDTid = NULL;                /* for injection in memory */

static BOOL VID_KillMemInjection = FALSE;    /* for vid_kill : TRUE=stop FALSE=continue */

static U8 gInjectLevel = 100;

#define VID_INVALID_INJECTEDSIZE        ((U32)-1)

/* Protection to concurrent access to pti writedata & synchronise */
semaphore_t *VID_PtiAvailable_p;

/* To lock injection task on underflow events */
semaphore_t *VidMemInjection_p;

VID_Injection_t VID_Injection[VIDEO_MAX_DEVICE];            /* Declared here since no other module see it */

/* --- Externals ------------------------------------------------------- */

#ifndef ARCHITECTURE_SPARC
#if !defined(mb411)
extern ST_Partition_t *NCachePartition_p;
#endif /* not mb411 */
#endif

ST_Partition_t   *DriverPartition_p = NULL;
/*#######################################################################*/
/* INTERFACE OBJECT - LINK INJECTION TO VIDEO */
/*#######################################################################*/
#if defined(ST_7100) || defined(ST_7109) || defined(ST_7200) || defined(ST_7710) || defined(ST_51xx_Device)
ST_ErrorCode_t GetWriteAddress(void * const Handle, void ** const Address_p)
{
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;
    /* registered handle = index in cd-fifo array */
    if (STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping) == ST_NO_ERROR)
    {
        * Address_p = STAVMEM_VirtualToDevice(VID_Injection[(U32)Handle].Write_p,&VirtualMapping);
        return(ST_NO_ERROR);
    }
    return(ST_ERROR_BAD_PARAMETER);
}

void InformReadAddress(void * const Handle, void * const Address_p)
{
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;
    /* registered handle = index in cd-fifo array */
    if (STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping) == ST_NO_ERROR)
    {
        VID_Injection[(U32)Handle].Read_p = STAVMEM_DeviceToVirtual(Address_p,&VirtualMapping);
    }
}
#endif  /* 7100 ... */

/*#######################################################################*/

/*#######################################################################*/
/*########################## EXTRA COMMANDS #############################*/
/*#######################################################################*/

/*-------------------------------------------------------------------------
 * Function : VID_ReportInjectionAlreadyOccurs
 *            Common report function
 * Input    : Injection type
 * Parameter:
 * Output   : TRUE if ok
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
void VID_ReportInjectionAlreadyOccurs(U32 CDFifoNb)
{
    switch(VID_Injection[CDFifoNb-1].Type)
    {
        case VID_HDD_INJECTION:
            STTBX_Print(("Injection from hdd is already runnning on fifo %d!!\n", CDFifoNb));
            break;

        case VID_LIVE_INJECTION:
            STTBX_Print(("Injection from live is already runnning on fifo %d!!\n", CDFifoNb));
            break;

        case VID_MEMORY_INJECTION:
            STTBX_Print(("Injection from memory is already runnning on fifo %d!!\n", CDFifoNb));
            break;

        default:
            STTBX_Print(("Injection is not yet described on this cd fifo %d!!\n", CDFifoNb));
            break;
    }
    return;
}

/*-------------------------------------------------------------------------
 * Function : VID_StopMemInject
 *            Stop memory injection
 * Input    : CD fifo number
 * Parameter:
 * Output   : TRUE if ok
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_StopMemInject(U32 VID_InjectionId)
{
    BOOL RetErr = FALSE;
    if ( VID_InjectionId >= VIDEO_MAX_DEVICE )
    {
        STTBX_Print(("Invalid CD fifo identifier\n"));
        return(TRUE);
    }
    if (VIDTid  != NULL )
    {
        if ( VID_Injection[VID_InjectionId].Type == VID_MEMORY_INJECTION)
        {
            /* Lock access */
            STOS_SemaphoreWait(VID_Injection[VID_InjectionId].Access_p);

#if defined(ST_51xx_Device) || defined(ST_7100) || defined(ST_7109) || defined(ST_7200) || defined(ST_7710)
            STVID_DeleteDataInputInterface(VID_Injection[VID_InjectionId].Driver.Handle);
#endif
            /* Disable the injection flag */
            VID_Injection[VID_InjectionId].Type = NO_INJECTION;

#ifdef DVD_TRANSPORT_LINK
            pti_video_dma_close() ;
#endif /* DVD_TRANSPORT_LINK */

            /* Unsubscribe to the underflow event */

            /* Free access */
            STOS_SemaphoreSignal(VID_Injection[VID_InjectionId].Access_p);

            STTBX_Print(("VID_StopMemInjection: CD fifo %d stopped\n", VID_InjectionId+1));
        }
        else
        {
            STTBX_Print(("VID_StopMemInject : Memory injection on CD fifo %d already stopped\n", VID_InjectionId+1));
            RetErr = TRUE;
        }
    }
    else
    {
#if defined(ST_51xx_Device) || defined(ST_7100) || defined(ST_7109) || defined(ST_7200) || defined(ST_7710)
        STVID_DeleteDataInputInterface(VID_Injection[VID_InjectionId].Driver.Handle);
#endif

        VID_Injection[VID_InjectionId].Type = NO_INJECTION;

        /* Force end of DoVideoInject() */
        VID_KillMemInjection = TRUE;
        STOS_SemaphoreSignal(VidMemInjection_p);
    }

    return(RetErr);
}

/*-------------------------------------------------------------------------
 * Function : CPUUserDataWrite
 *            Function to be used to inject ES video in Video CD fifo in case of unavabaility of PTI
 * Input    :
 * Parameter:
 * Output   :
 * Return   : TRUE
 * ----------------------------------------------------------------------*/
#ifdef STPTI_UNAVAILABLE
ST_ErrorCode_t CPUUserDataWrite(U8 *DataPtr_p, U32 DataLength, U32 DMADestination)
{
    U32 i;

    for (i=0; i< (DataLength/4) ; i++)
    {
        STSYS_WriteRegDev32LE(DMADestination, *(((U32 *)DataPtr_p) + i));
    }
    return(ST_NO_ERROR);
}
#endif /* STPTI_UNAVAILABLE */

/*-------------------------------------------------------------------------
 * Function : DoVideoInject
 *            Function is called by subtask
 * Input    :
 * Parameter:
 * Output   : TRUE if ok
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static void DoVideoInject( void )
{
    U32 VID_InjectionId=0;
    VID_MemInjection_t *PtiMemInjectVID_Injection_p;
    S32 DmaTransferSize;
#if defined(DVD_TRANSPORT_STPTI) && !defined(STPTI_UNAVAILABLE)
    STPTI_DMAParams_t STDMA_Params;
    #ifdef DVD_TRANSPORT_STPTI4
    U32 lDMAUSed;
    #endif
#endif /* DVD_TRANSPORT_STPTI */

#if defined(ST_51xx_Device) || defined(ST_7100) || defined(ST_7109) || defined(ST_7200) || defined(ST_7710)
    U32 								InputBufferFreeSize;
    U32 				SnapInputRead, SnapInputWrite;
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;

#if defined(USE_FDMA_TRANSFER)
	STFDMA_TransferGenericParams_t 		TransferParams;
	STFDMA_TransferId_t 				TransferId1;
    #if defined(mb411)
    STAVMEM_BlockHandle_t               FDMANodeHandle;
    #else
	STFDMA_GenericNode_t 				*NotRoundedNode_p;
#endif  /* mb 411 */
	STFDMA_GenericNode_t 				*Node_p;
#else
    void * CPUDest;
    void * CPUSrc;
#endif /* USE_FDMA_TRANSFER */
#endif  /* 5100 ... */

    ST_ErrorCode_t ErrCode = ST_NO_ERROR;
    BOOL DoJob;
    U32 BitBufferSize, BitBufferThreshold, BitBufferFreeSize;
    U32 MaxDmaTransferSize = 128*1024;
    U32 RemainingSourceDataSize = 0;

    STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping);

#if defined(USE_FDMA_TRANSFER)
#if defined(mb411)
    /* Allocate node structure in AvmemPartitionHandle[0] (LMI_SYS) */
    ErrCode = vidutil_Allocate(AvmemPartitionHandle[0], (sizeof(STFDMA_GenericNode_t)), 32, (void *)&Node_p, &FDMANodeHandle);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print(("Error ! Unable to allocate FDMA node"));
        return;
    }
#else
	/* Allocate a node structure.								*/
    NotRoundedNode_p = (STFDMA_GenericNode_t *) STOS_MemoryAllocate (NCachePartition_p, (sizeof(STFDMA_GenericNode_t)+31));
    /* Ensure the node pointer is aligned on a 32byte boundary 	*/
    Node_p = (STFDMA_GenericNode_t *) (((U32)NotRoundedNode_p + 31) & ~31);
#endif /* not mb411 */
#endif /* USE_FDMA_TRANSFER */

	while (1)
    {
        /* Wait the event UNDERFLOW coming from VIDEO */
        STOS_SemaphoreWait(VidMemInjection_p);
        /* Is vid_kill required ? */
        if (VID_KillMemInjection)
        {
            VID_KillMemInjection = FALSE;
#if defined(USE_FDMA_TRANSFER)
#if defined(mb411)
            /* Just to be compatible with vidutil_Deallocate() prototype */
            STAVMEM_PartitionHandle_t   AvmemPartitionHandle[1] = {0};
            vidutil_Deallocate(AvmemPartitionHandle[0], &FDMANodeHandle, Node_p);

#else
		    memory_deallocate (NCachePartition_p, NotRoundedNode_p);
#endif /* not mb411 */
#endif /* USE_FDMA_TRANSFER */

           for (VID_InjectionId=0; VID_InjectionId<VIDEO_MAX_DEVICE; VID_InjectionId++)
           {
              VID_Injection[VID_InjectionId].BitBufferSize = 0;  /* at next injection, we need to reevaluate it in case of change of Codec*/
           }

			break; /* exit from infinite loop */
        }

        /* Search for an injection job */
        for (VID_InjectionId=0; VID_InjectionId<VIDEO_MAX_DEVICE; VID_InjectionId++)
        {
            DoJob=FALSE;
            if  (VID_Injection[VID_InjectionId].Type == VID_MEMORY_INJECTION)
            {
                if ( VID_Injection[VID_InjectionId].Config.Memory.JobNumberOverflow )
                {
                    STTBX_Print(("Jobs list overflow for cd fifo %d !!! Stop memory injection.\n", VID_InjectionId+1));
                    VID_StopMemInject(VID_InjectionId);
                }
                else
                {
                    if ( VID_Injection[VID_InjectionId].Config.Memory.SynchronizedByUnderflowEvent )
                    {
                        /* Need a job to launch injection */
                        if (VID_Injection[VID_InjectionId].Config.Memory.JobReadIndex
                            != VID_Injection[VID_InjectionId].Config.Memory.JobWriteIndex)
                        {
                            DoJob=TRUE;
                        }
                    }
                    else
                    {
                        /* Enabled is enough to launch injection */
                        DoJob=TRUE;
                    }
                }
            }
            if (!(DoJob))
            {
                continue; /* direct take next index in 'for' loop */
            }
            /* --> DoJob = TRUE so we inject a piece of memory in this VID_InjectionId */
            if(VID_Injection[VID_InjectionId].State == STATE_INJECTION_NORMAL)
            {
                        PtiMemInjectVID_Injection_p = &VID_Injection[VID_InjectionId].Config.Memory;
#ifndef STPTI_UNAVAILABLE
#ifdef DVD_TRANSPORT_STPTI
#if defined (mb295)
                        /* Cannot reprogram dynamically dma through pti API */
                        /* Do it through following write register */
                        STDMA_Params.Destination = VIDEO_CD_FIFO;
#else /* mb295 */
                        STDMA_Params.Destination = VID_Injection[VID_InjectionId].Address;
#endif /* mb295 */
#if defined (ST_7015) || defined (ST_7020)
                        STDMA_Params.WriteLength = 4;
#else /* ST_7015 || ST_7020 */
                        STDMA_Params.WriteLength = 1;
#endif /* ST_7015 || ST_7020 */
                    /* Get the DMA transfert size */
#ifndef STPTI_5_1_6
                        STDMA_Params.CDReqLine = STPTI_CDREQ_VIDEO;
#endif
                        STDMA_Params.Holdoff = 1;
#ifdef DVD_TRANSPORT_STPTI4
                        STDMA_Params.DMAUsed = &lDMAUSed;
                        STDMA_Params.BurstSize = STPTI_DMA_BURST_MODE_ONE_BYTE;
#endif
#endif /* DVD_TRANSPORT_STPTI */
#endif /* not STPTI_UNAVAILABLE */
                        if ( PtiMemInjectVID_Injection_p->SynchronizedByUnderflowEvent )
                        {
                            PtiMemInjectVID_Injection_p->JobReadIndex = (PtiMemInjectVID_Injection_p->JobReadIndex+1) % VID_MEMINJECT_MAXJOBNUMBER;
                            DmaTransferSize = PtiMemInjectVID_Injection_p->Jobs[PtiMemInjectVID_Injection_p->JobReadIndex].RequestedSize;
                        }
                        else
                        {
                            ErrCode = STVID_GetBitBufferFreeSize(VID_Injection[VID_InjectionId].Driver.Handle, &BitBufferFreeSize);
                            if (ErrCode != ST_NO_ERROR)
                            {
                                STTBX_Print(( "DoVideoInject() : error STVID_GetBitBufferFreeSize()=0x%x !!\n", ErrCode));
                                VID_StopMemInject(VID_InjectionId);
                            }
#ifdef STI70xx_WA_BBL_SPURIOUS_OVERFLOW
                            /* Do as if bit buffer size were 512K bytes less than what was given to BBL.
                            This allows to always have 512K free at least in the bit buffer, what works around the STi7015 issue. */
                            BitBufferSize =  VID_Injection[VID_InjectionId].BitBufferSize - STI70xx_WA_LESS_SIZE;
                            if (BitBufferFreeSize > STI70xx_WA_LESS_SIZE)
                            {
                                BitBufferFreeSize -= STI70xx_WA_LESS_SIZE;
                            }
                            else
                            {
                                BitBufferFreeSize = 0;
                            }
#else /* STI70xx_WA_BBL_SPURIOUS_OVERFLOW */
                            BitBufferSize = VID_Injection[VID_InjectionId].BitBufferSize;
#endif /* STI70xx_WA_BBL_SPURIOUS_OVERFLOW */

                            /* Get the DMA transfert size */
                            if ( PtiMemInjectVID_Injection_p->SynchronizedByUnderflowEvent )
                            {
                                /* Lock injection access */
                                STOS_SemaphoreWait(VID_Injection[VID_InjectionId].Access_p);
                                PtiMemInjectVID_Injection_p->JobReadIndex = (PtiMemInjectVID_Injection_p->JobReadIndex+1) % VID_MEMINJECT_MAXJOBNUMBER;
                                DmaTransferSize = PtiMemInjectVID_Injection_p->Jobs[PtiMemInjectVID_Injection_p->JobReadIndex].RequestedSize;

                                /* Free injection access */
                                STOS_SemaphoreSignal(VID_Injection[VID_InjectionId].Access_p);

                                if (DmaTransferSize > BitBufferFreeSize)
                                {
                                    DmaTransferSize = BitBufferFreeSize;
                                }
                            }
                            else
                            {
                                DmaTransferSize = BitBufferFreeSize;
                                    /* Compute threshold to avoid an overflow */
                                BitBufferThreshold = (3*BitBufferSize/4) & (~3);
                                BitBufferThreshold = ((U32) gInjectLevel * BitBufferThreshold)/ (U32) 100;
#if defined(ST_51xx_Device) || defined(ST_7100) || defined(ST_7109) || defined(ST_7200) || defined(ST_7710)
                                /* prevent for data pending in input-buffer */
                                if(BitBufferThreshold > ( (U32)VID_Injection[VID_InjectionId].Top_p
                                                         -(U32)VID_Injection[VID_InjectionId].Base_p+1))
                                {
                                    BitBufferThreshold = BitBufferThreshold
                                        -( (U32)VID_Injection[VID_InjectionId].Top_p
                                          -(U32)VID_Injection[VID_InjectionId].Base_p+1);
                                }
#endif /* ST_51xx_Device || ST_7100 || ST_7109 || ST_7200 || ST_7710*/
                                if (BitBufferSize-BitBufferFreeSize > BitBufferThreshold)
                                {
                                    DmaTransferSize = 0;

                                    schedule();
                                }
                                else
                                {
                                    DmaTransferSize = BitBufferThreshold-(BitBufferSize-BitBufferFreeSize);
                                }
                            }

                            /* Control data size to be injected */
                            /* - enough data to fill the bit buffer until Threshold limit */
                            /* - 128k bytes maximum */
                            /* - remaining source data maximum */
                            RemainingSourceDataSize = (U32)PtiMemInjectVID_Injection_p->SrcSize -
                                    (U32)(PtiMemInjectVID_Injection_p->CurrentSrcPtr_p - PtiMemInjectVID_Injection_p->SrcBuffer_p);

                            if (DmaTransferSize > MaxDmaTransferSize)
                                DmaTransferSize = MaxDmaTransferSize;
                            if (DmaTransferSize > RemainingSourceDataSize)
                                DmaTransferSize = RemainingSourceDataSize;
                            if (DmaTransferSize > 0)
                            {
#if defined(ST_51xx_Device) || defined(ST_7100) || defined(ST_7109) || defined(ST_7200) || defined(ST_7710)
                                SnapInputRead  = (U32)VID_Injection[VID_InjectionId].Read_p;
                                SnapInputWrite = (U32)VID_Injection[VID_InjectionId].Write_p;

                                if(SnapInputWrite < SnapInputRead)
                                {
                                    InputBufferFreeSize = SnapInputRead
                                                    - SnapInputWrite
                                            - 10; /* -10 to leave 10 bytes empty to avoid
                                                    * considering a full buffer(Read=Write) as empty */
                                }
                                else
                                {
                                    InputBufferFreeSize = (U32)VID_Injection[VID_InjectionId].Top_p
                                                         -SnapInputWrite+1;
                                }
                                if(DmaTransferSize > InputBufferFreeSize)
                                {
                                    DmaTransferSize = InputBufferFreeSize;
                                }
								if(DmaTransferSize > 0x4000)
                                {
                                    DmaTransferSize=0x4000;
                                }
                        		if( (SnapInputWrite + DmaTransferSize >= (U32)VID_Injection[VID_InjectionId].Top_p) &&
                            		(SnapInputRead == (U32)VID_Injection[VID_InjectionId].Base_p) )
                        		{
                            		DmaTransferSize -= 1; /* Leave 1 byte empty to avoid considering a full buffer(Read=Write)
                                                    		* as empty */
                        		}
#if defined(USE_FDMA_TRANSFER)
								/* Now complete it... */
    							/* Transfer from memory to SWTS, for example */
								Node_p->Node.Next_p					= NULL;
								Node_p->Node.NumberBytes			= DmaTransferSize;
#if defined(mb411)
                                Node_p->Node.SourceAddress_p        = STAVMEM_VirtualToDevice(PtiMemInjectVID_Injection_p->CurrentSrcPtr_p, &VirtualMapping);
                                Node_p->Node.DestinationAddress_p   = STAVMEM_VirtualToDevice(SnapInputWrite, &VirtualMapping);
#else
                                Node_p->Node.SourceAddress_p        = PtiMemInjectVID_Injection_p->CurrentSrcPtr_p;
                                Node_p->Node.DestinationAddress_p   = (void *) SnapInputWrite;
#endif
                                Node_p->Node.Length                 = DmaTransferSize;
								Node_p->Node.SourceStride			= 0;
								Node_p->Node.DestinationStride		= 0;

								Node_p->Node.NodeControl.SourceDirection		= STFDMA_DIRECTION_STATIC;
								Node_p->Node.NodeControl.DestinationDirection 	= STFDMA_DIRECTION_STATIC;

								Node_p->Node.NodeControl.PaceSignal				= STFDMA_REQUEST_SIGNAL_NONE;
								Node_p->Node.NodeControl.SourceDirection		= STFDMA_DIRECTION_INCREMENTING;
								Node_p->Node.NodeControl.DestinationDirection 	= STFDMA_DIRECTION_INCREMENTING;
								Node_p->Node.NodeControl.NodeCompleteNotify		= FALSE;
								Node_p->Node.NodeControl.NodeCompletePause		= FALSE;
								Node_p->Node.NodeControl.Reserved				= 0;

#if defined(ST_7200)
                                TransferParams.FDMABlock            = STFDMA_2;
#else
                                TransferParams.FDMABlock            = STFDMA_1;
#endif
								TransferParams.ChannelId			= STFDMA_USE_ANY_CHANNEL;
								TransferParams.Pool					= STFDMA_DEFAULT_POOL;
								TransferParams.CallbackFunction 	= NULL;
#if defined(mb411)
                                TransferParams.NodeAddress_p        = STAVMEM_VirtualToDevice(Node_p, &VirtualMapping);
#else
                                TransferParams.NodeAddress_p        = Node_p;
#endif
								TransferParams.BlockingTimeout 		= 0;
								TransferParams.ApplicationData_p	= NULL;

								/* Use fdma transfer (more efficient).	*/
								ErrCode = STFDMA_StartGenericTransfer(&TransferParams,
																	&TransferId1);
#else /* not USE_FDMA_TRANSFER */
                                CPUDest = STAVMEM_VirtualToCPU(SnapInputWrite, &VirtualMapping);
                                CPUSrc = STAVMEM_VirtualToCPU(PtiMemInjectVID_Injection_p->CurrentSrcPtr_p, &VirtualMapping);

#ifdef LINUX_INJECT_DEBUG
                                /* Usefull traces when FDMA problems */
                                if (DmaTransferSize)
                                {
                                    STTBX_Print(("%x -> %x of %x bytes\n",CPUSrc,(U32)CPUDest,DmaTransferSize));
                                }
                                else
                                {
                                    STTBX_Print(("."));
                                }
#endif

                                /* Copy data */
                                memcpy(CPUDest, CPUSrc, DmaTransferSize);
#endif /* USE_FDMA_TRANSFER */
								/* Push the W pointer in video input */
                                if(( SnapInputWrite+ DmaTransferSize) > (U32)VID_Injection[VID_InjectionId].Top_p)
                                {
                                    SnapInputWrite= (U32)VID_Injection[VID_InjectionId].Base_p;
                                }
                                else
                                {
                                    SnapInputWrite=  SnapInputWrite + DmaTransferSize;
                                }
                                /* Share the new input ptr */
                                VID_Injection[(U32)VID_InjectionId].Write_p = (void*)SnapInputWrite;

#ifdef LINUX_INJECT_DEBUG
                                STTBX_Print(("Dest: %x\n",(U32)CPUDest));
#endif

#else /* not ST_51xx_Device || ST_7100 || ST_7109 || ST_7200 || ST_7710*/
                                /* Lock pti access */
                                STOS_SemaphoreWait(VID_PtiAvailable_p);
#if defined (mb295)
                                /* STPTI limitation workaround : write into the DMA base address */
                                STSYS_WriteRegDev32LE(PTI_BASE_ADDRESS + 0x1060, VID_Injection[VID_InjectionId].Address);
#endif /* mb295 */
                                /* Send transfer */
#ifndef STPTI_UNAVAILABLE
#ifdef DVD_TRANSPORT_STPTI
                                ErrCode = STPTI_UserDataWrite(PtiMemInjectVID_Injection_p->CurrentSrcPtr_p,
                                                          (U32)DmaTransferSize, &STDMA_Params);
                                if ( ErrCode != ST_NO_ERROR )
                                {
                                    STTBX_Print(("Task PtiMemInject : STPTI_UserDataWrite()=0x%x\n", ErrCode));
                                }
#else
                                pti_video_dma_inject( (U8*)PtiMemInjectVID_Injection_p->CurrentSrcPtr_p,
                                                  (U32) DmaTransferSize);

#endif /* DVD_TRANSPORT_STPTI*/
#else /* STPTI_UNAVAILABLE */

#ifdef LINUX_INJECT_DEBUG
                                STTBX_Print(("W:SrcPtr:%x Sz:%x Dest:%x\n",
                                                    PtiMemInjectVID_Injection_p->CurrentSrcPtr_p,
                                                    (U32)DmaTransferSize,
                                                    VID_Injection[VID_InjectionId].Address));
                                STTBX_Print(("\r\n1"));
#endif

 #if defined (mb295)
                                CPUUserDataWrite(PtiMemInjectVID_Injection_p->CurrentSrcPtr_p,
                                                          (U32)DmaTransferSize, VIDEO_CD_FIFO);
 #else /* mb295 */
                                CPUUserDataWrite(PtiMemInjectVID_Injection_p->CurrentSrcPtr_p,
                                                          (U32)DmaTransferSize, VID_Injection[VID_InjectionId].Address);
 #endif /* mb295 */
#endif

                                /* Lock injection access */
                                STOS_SemaphoreWait(VID_Injection[VID_InjectionId].Access_p);

                                /* Store Dma transferSize into circular buffer */
                                PtiMemInjectVID_Injection_p->InjectedSizeHistory[PtiMemInjectVID_Injection_p->InjectedSizeHistoryIndex] =
                                    (U32)DmaTransferSize;
                                PtiMemInjectVID_Injection_p->InjectedSizeHistoryIndex = (PtiMemInjectVID_Injection_p->InjectedSizeHistoryIndex+1)
                                    % VID_INJECTEDSIZEHISTORYSIZE;

                                /* Free injection access */
                                STOS_SemaphoreSignal(VID_Injection[VID_InjectionId].Access_p);
#endif /* not ST_5100 ST_5525*/
                            }

                            /* LM: Keep this to have a trace of the following calculus ... */
                            /* linux_delay(((3*(U32)DmaTransferSize/1024)/8)/15); */

							STOS_TaskDelayUs(1000); /* 1ms */

                            if (DmaTransferSize > 0)
                            {

#ifndef STPTI_UNAVAILABLE
#if !defined ST_51xx_Device && !defined ST_7100 &&  !defined ST_7109 && !defined ST_7200 && !defined ST_7710
#ifdef DVD_TRANSPORT_STPTI
                                ErrCode = STPTI_UserDataSynchronize(&STDMA_Params);  /* Wait before use */
                                if ( ErrCode != ST_NO_ERROR )
                                {
                                    STTBX_Print(("Task PtiMemInject : STPTI_UserDataSynchronize()=0x%x\n", ErrCode));
                                }
#else
                                pti_video_dma_synchronize();
#endif /* DVD_TRANSPORT_STPTI */
#endif /* ST_5100 ,ST_5525*/
#endif
                                /* Unlock pti access */
                                STOS_SemaphoreSignal(VID_PtiAvailable_p);
                            }

                            /* Lock injection access */
                            STOS_SemaphoreWait(VID_Injection[VID_InjectionId].Access_p);

                            /* Increment the source pointer */
                            PtiMemInjectVID_Injection_p->CurrentSrcPtr_p += DmaTransferSize;

                            /* Check if the whole buffer has been sent */
                            if(PtiMemInjectVID_Injection_p->CurrentSrcPtr_p
                                    > (PtiMemInjectVID_Injection_p->SrcBuffer_p
                                     + (U32)PtiMemInjectVID_Injection_p->SrcSize)
                                     - 1)
                            {
                                /* Reset the current source pointer */
                                PtiMemInjectVID_Injection_p->CurrentSrcPtr_p = PtiMemInjectVID_Injection_p->SrcBuffer_p;
                                /* Decrement the loop counter */
                                if (PtiMemInjectVID_Injection_p->LoopNbr != -1)
                                    PtiMemInjectVID_Injection_p->LoopNbr--;
                                /* Anyway, increment achieved loop counter */
                                PtiMemInjectVID_Injection_p->LoopNbrDone++;

                                /* Check if CD fifo injection must be disabled */
                                if ( PtiMemInjectVID_Injection_p->LoopNbr == 0 )
                                {
                                    /* Free injection access */
                                    STOS_SemaphoreSignal(VID_Injection[VID_InjectionId].Access_p);

                                    VID_StopMemInject(VID_InjectionId);

                                    /* Lock injection access */
                                    STOS_SemaphoreWait(VID_Injection[VID_InjectionId].Access_p);
                                }
                            }

                            /* Free injection access */
                            STOS_SemaphoreSignal(VID_Injection[VID_InjectionId].Access_p);

                        } /* end if enabled */

                        /* EnableDelayAfterJob */
                        STOS_TaskDelayUs(1000);  /* 1ms, originally 10 tick for OS20 (640us) */
                        if ( !PtiMemInjectVID_Injection_p->SynchronizedByUnderflowEvent &&
                             (VID_Injection[VID_InjectionId].Type == VID_MEMORY_INJECTION))
                        {
                            STOS_SemaphoreSignal(VidMemInjection_p);
                        }
            } /* end if STATE_INJECTION_NORMAL */
        } /* for (MaxSearchVID_InjectionId=VIDEO_MAX_DEVICE;MaxSearchVID_InjectionId;MaxSearchVID_InjectionId--) */
    } /* end of while */
} /* end of DoVideoInject() */

/*-------------------------------------------------------------------------
 * Function : DoVideoInjectTask
 *            Task that calls DoVideoInject
 * Input    :
 * Parameter:
 * Output   : TRUE if ok
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static void DoVideoInjectTask( void )
{
    STOS_TaskEnter(NULL);

    DoVideoInject();

    STOS_TaskExit(NULL);
}

/*-------------------------------------------------------------------------
 * Function : VID_KillTask
 *            Kill Video Inject task
 * Input    :
 * Output   : FALSE if no error
 * Return   : TRUE if error
 * ----------------------------------------------------------------------*/
BOOL VID_KillTask(S32 FifoNb, BOOL WaitEnd)
{
    BOOL    RetErr = FALSE, StillFifoInjection = FALSE;
    S32     i=0;

    if ((WaitEnd == 1) && (FifoNb == 0))
    {
        return(TRUE);
    }

    if (WaitEnd == 1)
    {
        /* Force number of remaining injection loops to 1 */
        /* Let's DoVideoInject() finish current injection */
        i= FifoNb - 1;

        STOS_SemaphoreWait(VID_Injection[FifoNb - 1].Access_p);
        if ( VID_Injection[FifoNb - 1].Config.Memory.LoopNbr != 0 ) /* if not yet finished */
        {
            VID_Injection[FifoNb - 1].Config.Memory.LoopNbr = 1;
        }
        STOS_SemaphoreSignal(VID_Injection[FifoNb - 1].Access_p);
        /* Wait end of current injection, until end of source buffer */
        STTBX_Print(("Stop injection requested. Waiting for current loop completion...\n"));

        while (   (VID_Injection[FifoNb - 1].Type == VID_MEMORY_INJECTION)
               && (   (VID_Injection[FifoNb - 1].Driver.DriverState == STATE_DRIVER_STARTED)
                   || (VID_Injection[FifoNb - 1].Driver.DriverState == STATE_DRIVER_STOP_ENDOFDATA_ASKED)))
        {
            STOS_TaskDelayUs(5000); /* 5ms */
        }
        /* Stop injection task with VID_StopMemInject() below */

        /* Manage stop end of data asked special case */
        if (VID_Injection[FifoNb - 1].Driver.DriverState == STATE_DRIVER_STOP_ENDOFDATA_ASKED)
        {
            VID_Injection[FifoNb - 1].Driver.DriverState = STATE_DRIVER_STOPPED;
        }
    }

    /* Reset global variable and wait for task to finish before deleting */
    RetErr = FALSE;
    if ( VIDTid != NULL )
    {
        for (i = 0; (i < VIDEO_MAX_DEVICE) || (RetErr); i++)
        {
            /* Lock injection access */
            if (VID_Injection[i].Type == VID_MEMORY_INJECTION)
            {
                if ((FifoNb == 0) || (FifoNb-1 == i))
                {
                    RetErr = VID_StopMemInject(i);
                }
                else
                {
                    StillFifoInjection = TRUE;
                }
            }
        }

        if((!(StillFifoInjection)) && (!(RetErr)))
        {
            STTBX_Print(("Waiting for injection task termination ... "));

            VID_KillMemInjection = TRUE;
            STOS_SemaphoreSignal(VidMemInjection_p);

            STOS_TaskWait(&VIDTid, TIMEOUT_INFINITY);
            STOS_TaskDelete (VIDTid, NULL, NULL, NULL);

            STTBX_Print(("Video Task finished\n"));

            VIDTid = NULL;
        }
    }
    else
    {
        STTBX_Print(("No injection task currently running.\n"));
    }

    return(RetErr);
} /* end of VID_KillTask() */

/*-------------------------------------------------------------------------
 * Function : VID_MemInject
 *            Inject Video in Memory to DMA
 * Input    : *pars_p, *Result
 * Output   : FALSE if no error
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
BOOL VID_MemInject(STVID_Handle_t CurrentHandle, S32 InjectLoop, S32 FifoNb, S32 BufferNb, void * AlignedBuffer_p, U32 NbBytes)
{
    BOOL            RetErr = FALSE;
    void           *Param = NULL;
    ST_ErrorCode_t  ErrCode;
    S32 i;
 #if defined(ST_51xx_Device) || defined(ST_7100) || defined(ST_7109) || defined(ST_7200) || defined(ST_7710)
    U32 InputBuffersize;
 #endif

#if !defined STUB_INJECT
    STTBX_Print(("VID_Injection Input:\n  Address:0x%.8x\n  Size:%d\n", (U32)AlignedBuffer_p, NbBytes));

    /* Lock injection access */
    STOS_SemaphoreWait(VID_Injection[FifoNb-1].Access_p);

    if(VID_Injection[FifoNb-1].BitBufferSize == 0)
    {

        /* No injection occured. Bit buffer should be empty */
        ErrCode = STVID_GetBitBufferFreeSize(CurrentHandle, &VID_Injection[FifoNb-1].BitBufferSize);
        if(ErrCode != ST_NO_ERROR)
        {
            VID_Injection[FifoNb-1].BitBufferSize = 0;
            STTBX_Print(("Error occured while exec GetBitBufferFreeSize = %d\n", ErrCode));
            RetErr = TRUE;
        }
    }
    if (!(RetErr))
    {
        if(VID_Injection[FifoNb-1].Type == NO_INJECTION)
        {
            if (VIDTid == NULL)
            {
                ErrCode = STOS_TaskCreate ((void (*)(void*))DoVideoInjectTask,
                                           Param,
                                           NULL,
                                           0,
                                           NULL,
                                           NULL,
                                           &VIDTid,
                                           NULL,
                                           VID_INJECTION_TASK_PRIORITY,
                                           "VideoInject",
                                           task_flags_no_min_stack_size );

                if ( ErrCode != ST_NO_ERROR )
                {
                    STTBX_Print(("Unable to create video task\n"));
                    RetErr = TRUE;
                }
                else
                {
                    if(InjectLoop < 0)
                    {
                        STTBX_Print(("Infinite playing on cdfifo %d\n", FifoNb));
                    }
                    else
                    {
                        STTBX_Print(("Playing video %d times on cdfifo %d\n", InjectLoop, FifoNb ));
                    }
                }
            }

            if (!(RetErr))
            {
                VID_Injection[FifoNb-1].Config.Memory.SrcBuffer_p = AlignedBuffer_p;
                VID_Injection[FifoNb-1].Config.Memory.CurrentSrcPtr_p = AlignedBuffer_p;
                VID_Injection[FifoNb-1].Config.Memory.SrcSize = NbBytes;
                VID_Injection[FifoNb-1].Config.Memory.LoadBufferNb = BufferNb;
                VID_Injection[FifoNb-1].Config.Memory.LoopNbr = InjectLoop;
                VID_Injection[FifoNb-1].Config.Memory.LoopNbrDone = 0;
                VID_Injection[FifoNb-1].Config.Memory.SynchronizedByUnderflowEvent = FALSE;
                VID_Injection[FifoNb-1].Driver.Handle = CurrentHandle;
                for (i = 0; i < VID_INJECTEDSIZEHISTORYSIZE; i++)
                {
                    VID_Injection[FifoNb-1].Config.Memory.InjectedSizeHistory[i] = VID_INVALID_INJECTEDSIZE;
                }
                VID_Injection[FifoNb-1].Config.Memory.InjectedSizeHistoryIndex = 0;
#if defined(ST_51xx_Device) || defined(ST_7100) || defined(ST_7109) || defined(ST_7200) || defined(ST_7710)
                /* Get Input buffer characteristics     */
                ErrCode = STVID_GetDataInputBufferParams(VID_Injection[FifoNb-1].Driver.Handle,
                    &VID_Injection[FifoNb-1].Base_p, &InputBuffersize);

#ifdef LINUX_INJECT_DEBUG
                STTBX_Print(("Base_p:%x InputBuffer Size:%x\n",VID_Injection[FifoNb-1].Base_p,InputBuffersize));
#endif

                /* Update injection pointer.            */
                VID_Injection[FifoNb-1].Top_p   =
                        (void *)((U32)VID_Injection[FifoNb-1].Base_p + InputBuffersize - 1);

                /* Align the write and read pointers to the beginning of the input buffer */
                VID_Injection[FifoNb-1].Write_p = VID_Injection[FifoNb-1].Base_p;
                VID_Injection[FifoNb-1].Read_p  = VID_Injection[FifoNb-1].Base_p;

#ifdef LINUX_INJECT_DEBUG
                STTBX_Print(("FifoNb:%d Write_p:0x%.8x Read_p:0x%.8x\n",FifoNb, VID_Injection[FifoNb-1].Write_p, VID_Injection[FifoNb-1].Read_p));
#endif

                /* Configure the interface-object */
                STVID_SetDataInputInterface( VID_Injection[FifoNb-1].Driver.Handle,
                                             GetWriteAddress,
                                             InformReadAddress,
                                             (void*)(FifoNb-1));
#endif /* not ST_5100 ST_5525*/

                VID_Injection[FifoNb-1].Type = VID_MEMORY_INJECTION;
                /* Launch injection */
                STOS_SemaphoreSignal(VidMemInjection_p);
            }
        }
        else
        {
            VID_ReportInjectionAlreadyOccurs(FifoNb);
        }
    }
    /* Free injection access */
    STOS_SemaphoreSignal(VID_Injection[FifoNb-1].Access_p);
#endif /* STUB_INJECT */

    return(RetErr);
} /* end of VID_MemInject() */

/*-------------------------------------------------------------------------
 * Function : VIDDecodeFromMemory
 *            copy a stream from buffer to video bit buffer
 * Input    :
 * Output   : FALSE if no error
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
BOOL VIDDecodeFromMemory(STVID_Handle_t CurrentHandle, S32 FifoNb, S32 BuffNb, S32 NbLoops, void * AlignedBuffer_p, U32 NbBytes)
{
    BOOL            RetErr = FALSE;
    ST_ErrorCode_t  ErrCode = ST_NO_ERROR;
    S32             i;
#if defined(ST_51xx_Device) || defined(ST_7100) || defined(ST_7109) || defined(ST_7200) || defined(ST_7710)
    U32 InputBuffersize;
#endif

    /* Lock injection access */
    STOS_SemaphoreWait(VID_Injection[FifoNb-1].Access_p);
    if(VID_Injection[FifoNb-1].Type != NO_INJECTION)
    {
        VID_ReportInjectionAlreadyOccurs(FifoNb);
        RetErr = TRUE;
    }
    else
    {
        if(VID_Injection[FifoNb-1].BitBufferSize == 0)
        {
            /* No injection occured. Bit buffer should be empty */
            ErrCode = STVID_GetBitBufferFreeSize(CurrentHandle, &VID_Injection[FifoNb-1].BitBufferSize);
            if(ErrCode != ST_NO_ERROR)
            {
                STTBX_Print(("Error occured : STVID_GetBitBufferFreeSize() = %d !!\n", ErrCode));
                RetErr = TRUE;
                VID_Injection[FifoNb-1].BitBufferSize = 0;
            }
        }
        if (!(RetErr))
        {
            VID_Injection[FifoNb-1].Type = VID_MEMORY_INJECTION;
        }
    }

    /* Free injection access */
    STOS_SemaphoreSignal(VID_Injection[FifoNb-1].Access_p);
    if (RetErr)
    {
        return(TRUE);
    }

    VID_Injection[FifoNb-1].Driver.Handle = CurrentHandle;
    VID_Injection[FifoNb-1].Config.Memory.SrcBuffer_p = AlignedBuffer_p;
    VID_Injection[FifoNb-1].Config.Memory.CurrentSrcPtr_p = AlignedBuffer_p;
    VID_Injection[FifoNb-1].Config.Memory.SrcSize = NbBytes;
    VID_Injection[FifoNb-1].Config.Memory.LoadBufferNb = BuffNb;
    VID_Injection[FifoNb-1].Config.Memory.LoopNbr = NbLoops;
    VID_Injection[FifoNb-1].Config.Memory.LoopNbrDone = 0;
    VID_Injection[FifoNb-1].Config.Memory.SynchronizedByUnderflowEvent = FALSE;
    for(i=0;i<VID_INJECTEDSIZEHISTORYSIZE;i++)
    {
        VID_Injection[FifoNb-1].Config.Memory.InjectedSizeHistory[i] = VID_INVALID_INJECTEDSIZE;
    }
    VID_Injection[FifoNb-1].Config.Memory.InjectedSizeHistoryIndex = 0;

#if defined(ST_51xx_Device) || defined(ST_7100) || defined(ST_7109) || defined(ST_7200) || defined(ST_7710)
    /* Get Input buffer characteristics     */
    ErrCode = STVID_GetDataInputBufferParams(VID_Injection[FifoNb-1].Driver.Handle,
        &VID_Injection[FifoNb-1].Base_p, &InputBuffersize);

    /* Update injection pointer.            */
    VID_Injection[FifoNb-1].Top_p   =
            (void *)((U32)VID_Injection[FifoNb-1].Base_p + InputBuffersize - 1);

    /* Align the write and read pointers to the beginning of the input buffer */
    VID_Injection[FifoNb-1].Write_p = VID_Injection[FifoNb-1].Base_p;
    VID_Injection[FifoNb-1].Read_p  = VID_Injection[FifoNb-1].Base_p;

    /* Configure the interface-object */
    STVID_SetDataInputInterface(
                            VID_Injection[FifoNb-1].Driver.Handle,
                            GetWriteAddress,InformReadAddress,(void*)(FifoNb-1));
  #endif /* not ST5100 .. */

    VID_Injection[FifoNb-1].Type = VID_MEMORY_INJECTION;
    /* Launch injection */
    STOS_SemaphoreSignal(VidMemInjection_p);

    /* Launch Injection */
    if (VID_Injection[FifoNb-1].Driver.DriverState != STATE_DRIVER_STOPPED)
    {
        /* Video Load Buffer in use */
        if( VIDTid == NULL )
        {
            DoVideoInject();
        }
        /* else the inject task exists: it will execute the injection by itself */
    }
    else
    {
        STTBX_Print(("Cannot launch a decode until end of buffer while decoder is stopped !\n" ));
    }

#ifdef DVD_TRANSPORT_LINK
    pti_video_dma_close() ;
#endif /* DVD_TRANSPORT_LINK */

    /* Lock injection access */
    STOS_SemaphoreWait(VID_Injection[FifoNb-1].Access_p);

    VID_Injection[FifoNb-1].Type = NO_INJECTION;

    VID_Injection[FifoNb-1].BitBufferSize = 0;  /* at next injection, we need to reevaluate it in case of change of Codec */

    /* Free injection access */
    STOS_SemaphoreSignal(VID_Injection[FifoNb-1].Access_p);

    return RetErr;
} /* end of VIDDecodeFromMemory() */

/*#######################################################################*/
/*#########################  GLOBAL FUNCTIONS  ##########################*/
/*#######################################################################*/

/*-------------------------------------------------------------------------
 * Function : VID_Inj_RegisterCmd
 *            Definition of the register commands
 * Input    :
 * Output   :
 * Return   : TRUE if success
 * ----------------------------------------------------------------------*/
BOOL VID_Inj_RegisterCmd (void)
{
    BOOL RetErr;
    U32 i;

    TicksPerOneSecond = ST_GetClocksPerSecond();

    /* Macro's name, link to C-function, help message  */
    /* (by alphabetic order of macros = display order) */

    RetErr = FALSE;

    /* Memory injection management : */
    memset(&VID_Injection, 0, (VIDEO_MAX_DEVICE * sizeof(VID_Injection_t)));

    /* Init for injection process */
    VidMemInjection_p = STOS_SemaphoreCreateFifo(DriverPartition_p,0);
    VID_PtiAvailable_p = STOS_SemaphoreCreateFifo(DriverPartition_p,1);

    for (i=0;i<VIDEO_MAX_DEVICE;i++)
    {
        VID_Injection[i].Access_p = STOS_SemaphoreCreateFifo(DriverPartition_p,1);
        /* Common default values */
        VID_Injection[i].Type = NO_INJECTION;
        VID_Injection[i].State = STATE_INJECTION_NORMAL;
#if !defined ST_51xx_Device && !defined ST_7100 && !defined ST_7109 && !defined ST_7200 && !defined ST_7710
        VID_Injection[i].Address = VIDEO_CD_FIFO;
#else
        VID_Injection[i].Address = 0; /* no cd fifo in 5100 and 7710: input-buffer is used instead */
#endif
        VID_Injection[i].BitBufferSize = 0;
        /* First is video and cdfifo number is i+1 */
        VID_Injection[i].Number = i+1;
#if defined (ST_7015)
        if ( i==0 )
            VID_Injection[i].Address = STI7015_BASE_ADDRESS + ST7015_VIDEO_CD_FIFO1_OFFSET;
        if ( i==1 )
            VID_Injection[i].Address = STI7015_BASE_ADDRESS + ST7015_VIDEO_CD_FIFO2_OFFSET;
        if ( i==2 )
            VID_Injection[i].Address = STI7015_BASE_ADDRESS + ST7015_VIDEO_CD_FIFO3_OFFSET;
        if ( i==3 )
            VID_Injection[i].Address = STI7015_BASE_ADDRESS + ST7015_VIDEO_CD_FIFO4_OFFSET;
        if ( i==4 )
            VID_Injection[i].Address = STI7015_BASE_ADDRESS + ST7015_VIDEO_CD_FIFO5_OFFSET;
#elif defined (ST_7020)
        if ( i==0 )
            VID_Injection[i].Address = STI7020_BASE_ADDRESS + ST7020_VIDEO_CD_FIFO1_OFFSET;
        if ( i==1 )
            VID_Injection[i].Address = STI7020_BASE_ADDRESS + ST7020_VIDEO_CD_FIFO2_OFFSET;
        if ( i==2 )
            VID_Injection[i].Address = STI7020_BASE_ADDRESS + ST7020_VIDEO_CD_FIFO3_OFFSET;
        if ( i==3 )
            VID_Injection[i].Address = STI7020_BASE_ADDRESS + ST7020_VIDEO_CD_FIFO4_OFFSET;
        if ( i==4 )
            VID_Injection[i].Address = STI7020_BASE_ADDRESS + ST7020_VIDEO_CD_FIFO5_OFFSET;
#if defined (ST_5528)
        if ( i==5 )
            VID_Injection[i].Address = ST5528_VIDEO_CD_FIFO1_BASE_ADDRESS;
        if ( i==6 )
            VID_Injection[i].Address = ST5528_VIDEO_CD_FIFO2_BASE_ADDRESS;
#endif /* ST_5528 */
#elif defined (ST_5528)
        if ( i==0 )
            VID_Injection[i].Address = ST5528_VIDEO_CD_FIFO1_BASE_ADDRESS;
        if ( i==1 )
            VID_Injection[i].Address = ST5528_VIDEO_CD_FIFO2_BASE_ADDRESS;
#endif /* ST_7020 */
    }

    /* Already done in vid_cmd.c, need it here for Kernel space only ... */
    /* Init global structures */
    for (i=0; i < VIDEO_MAX_DEVICE; i++)
    {
        strcpy(VID_Injection[i].Driver.DeviceName, "");
        VID_Injection[i].Driver.Handle = 0;
    }

    return(!RetErr);

} /* end VID_Inj_RegisterCmd */



/*#######################################################################*/

