/************************************************************************
COPYRIGHT (C) STMicroelectronics 2005

File name   : vid_inj.c
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

/*#######################################################################*/
/*########################## INCLUDES FILE ##############################*/
/*#######################################################################*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "stddefs.h"
#include "stos.h"

#ifdef ST_OSLINUX
#if !defined DVD_TRANSPORT_STPTI
#define DVD_TRANSPORT_STPTI
#endif
#undef DVD_TRANSPORT_PTI
#include "stvidtest.h"
#endif  /* ST_OSLINUX */

#include "sttbx.h"
#include "testcfg.h"
#include "stlite.h"
/* Driver as to be used as backend */
#define USE_AS_BACKEND
#include "genadd.h"

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
#ifdef USE_DEMUX
 #include "stdemux.h"
#endif /* USE_DEMUX */
#include "stavmem.h"

#include "stsys.h"

#include "stevt.h"
#ifdef USE_CLKRV
 #include "stclkrv.h"
 #include "clclkrv.h"
#else
 #include "stcommon.h" /* for ST_GetClocksPerSecond() */
#endif /* USE_CLKRV */
#include "stpwm.h"
#include "stvtg.h"
#include "vid_cmd.h"
#include "vid_util.h"
#include "vid_inj.h"
#include "vtg_cmd.h"

#if !defined ST_OSLINUX
#include "clavmem.h" /* needed for MAX_PARTITIONS */
#endif

#if defined(ST_7710) || defined(ST_5105) || defined(ST_5107) /* || defined(ST_7100)  */
/* Allow the fdma transfer when using a lcmpegs1/lcmpegh1 based chip	*/
/* i.e. STi5100, STi5105, STi5107, STi5301, STi7100, STi7710, ...                                  */
/* This is to save bandwidth during data injection (better than memcpy)	*/
 #define USE_FDMA_TRANSFER
#endif

#if defined(USE_FDMA_TRANSFER)
 #include "clfdma.h"
#endif

#ifdef TRACE_UART
 #include "trace.h"
#endif

#if defined(DVD_SECURED_CHIP)
    /* Use trusted Lx_Copy from Audio LX (non-secure to secure transfers) instead of ST40's memcpy  */
    #define USE_CRYPTOCORE
#endif /* Secured chip */

#ifdef USE_CRYPTOCORE
#include "crypto.h"
#endif /* USE_CRYPTOCORE */

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
#if defined(ST_7100) || defined(ST_7109) || defined (ST_7200) || defined(ST_7710) || defined(ST_5105) || defined(ST_5107) || defined(ST_5188) || defined(ST_5188)
/* Injection task priority increased to be able to inject properly in HD mode !!!				*/
#define VID_INJECTION_TASK_PRIORITY (MIN_USER_PRIORITY+1) 	/* Stapidemo is MIN_USER_PRIORITY+8 */
#else
#define VID_INJECTION_TASK_PRIORITY (MIN_USER_PRIORITY) 	/* Stapidemo is MIN_USER_PRIORITY+8 */
#endif
#else
#define VID_INJECTION_TASK_PRIORITY    18
#endif

#define DELAY_1S                    1000000     /* Expressed in microseconds */

#define NUMBER_OF_INJECTIONS_BEFORE_INTRODUCING_ERRORS    10

/* --- Global variables ------------------------------------------------ */

#ifdef USE_CRYPTOCORE
static Cryptocore_t crypto;
#endif /* USE_CRYPTOCORE */

static char VID_Msg[1024];                             /* text for trace */

static task_t *VIDTid;                /* for injection in memory */

#if !defined ST_OSLINUX
static tdesc_t   TaskDesc;
static void *    TaskStack;

static BOOL VID_KillMemInjection = FALSE;    /* for vid_kill : TRUE=stop FALSE=continue */
static U8 gInjectLevel = 100;

static BOOL CorruptMemoryInjection = FALSE; /* Boolean value to add errors in the
                                             * stream, while injecting it from memory */


#define VID_INVALID_INJECTEDSIZE        ((U32)-1)
#endif /* !LINUX */

/* --- For use within VIDDecodeFromMemory--- */
#if !defined STVID_VIDDEC_PBO_ENABLED /* We have not PBO */
static U8 WaitDecodeCompletionCounter;
static STVID_Handle_t WaitDecodeCompletionCounterVideoHandle = 0;
static void VID_CatchNewPictureDecodedEvt ( STEVT_CallReason_t Reason,
                               const ST_DeviceName_t RegistrantName,
                               STEVT_EventConstant_t Event,
                               const void *EventData,
                               const void *SubscriberData_p );
#endif  /* !STVID_VIDDEC_PBO_ENABLED */

#if !defined ST_OSLINUX
static BOOL InjectWithoutDelays(U32 FifoNb);
#endif /* !ST_OSLINUX*/

/* eo- For use within VIDDecodeFromMemory--- */

#if !defined ST_OSLINUX
/* Protection to concurrent access to pti writedata & synchronise */
semaphore_t *VID_PtiAvailable_p;

/* To lock injection task on underflow events */
semaphore_t *VidMemInjection_p;

typedef struct {
    void *                              StartAddress_p;
    U32                                 BufferSize;
    U32                                 LastFilledTransferSize;
    STAVMEM_BlockHandle_t               AvmemBlockHandle;
} VIDTRICK_BackwardBuffer_t;

VIDTRICK_BackwardBuffer_t BufferA;
#endif /* !LINUX */

VID_Injection_t VID_Injection[VIDEO_MAX_DEVICE];
extern STVID_Handle_t  DefaultVideoHandle;

/* --- Externals ------------------------------------------------------- */

#ifndef ARCHITECTURE_SPARC
#if !defined(mb411) && !defined(mb519)
extern ST_Partition_t *NCachePartition_p;
#endif /* not mb411 not mb 519 */
#endif

#ifdef ST_OSLINUX
ST_Partition_t   *DriverPartition_p = NULL;
#endif

/* --- Prototypes ------------------------------------------------------ */

void InformReadAddress(void * const Handle, void * const Address_p);
void VID_ReportInjectionAlreadyOccurs(U32 CDFifoNb);
ST_ErrorCode_t GetWriteAddress(void * const Handle, void ** const Address_p);
static BOOL VID_StillDecode( STTST_Parse_t *pars_p, char *result_sym_p);
BOOL VID_Inj_RegisterCmd (void);
#ifdef ST_OSLINUX
extern void * STAPIGAT_UserToKernel(void * VirtUserAddress_p);
extern ST_ErrorCode_t STVIDTEST_DecodeFromStreamBuffer(STVID_Handle_t  CurrentHandle,
                                                S32             FifoNb,
                                                S32             BuffNb,
                                                S32             NbLoops,
                                                void           *Buffer_p,
                                                U32             NbBytes);
#endif

/*#######################################################################*/
/* INTERFACE OBJECT - LINK INJECTION TO VIDEO */
/*#######################################################################*/
#if !defined ST_OSLINUX
#if defined(ST_7100) || defined(ST_7109) || defined(ST_7200) || defined(ST_5100) || defined(ST_5105) || defined(ST_5107) || defined(ST_5188) || defined(ST_5301) || defined(ST_7710)|| defined(ST_5525)
/*-------------------------------------------------------------------------
 * Function : GetWriteAddress of input buffer
 *
 * Input    :
 * Parameter:
 * Output   :
 * Return   : ST_NO_ERROR or ST_ERROR_BAD_PARAMETER
 * ----------------------------------------------------------------------*/
ST_ErrorCode_t GetWriteAddress(void * const Handle, void ** const Address_p)
{
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;
    /* registered handle = index in cd-fifo array */
    if (STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping) == ST_NO_ERROR)
    {
        * Address_p = STAVMEM_VirtualToDevice(VID_Injection[(U32)Handle].Write_p, &VirtualMapping);
        return(ST_NO_ERROR);
    }
    return(ST_ERROR_BAD_PARAMETER);
}

/*-------------------------------------------------------------------------
 * Function : InformReadAddress of input buffer
 *
 * Input    :
 * Parameter:
 * Output   :
 * Return   : none
 * ----------------------------------------------------------------------*/
void InformReadAddress(void * const Handle, void * const Address_p)
{
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;
    /* registered handle = index in cd-fifo array */
    if (STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping) == ST_NO_ERROR)
    {
        VID_Injection[(U32)Handle].Read_p = STAVMEM_DeviceToVirtual(Address_p, &VirtualMapping);
    }
}
#endif  /* 7100 ... */
#endif  /* !ST_OSLINUX */


/*#######################################################################*/
/*########################## EXTRA COMMANDS #############################*/
/*#######################################################################*/

#ifndef STVID_VIDDEC_PBO_ENABLED /* we have not PBO */
/*-------------------------------------------------------------------------
 * Function : VID_GetNameFromHandle
 *
 * Input    : Handle
 * Parameter:
 * Output   : DeviceName associated to the passed handle
 * Return   : FALSE if DeviceName found, TRUE else.
 * ----------------------------------------------------------------------*/
static BOOL VID_GetNameFromHandle(STVID_Handle_t CurrentHandle, ST_DeviceName_t DeviceName)
{
    BOOL HandleFound;
    S32 LVar;

    HandleFound = FALSE;
    LVar = 0;
    while ((LVar<=VIDEO_MAX_DEVICE) && (!HandleFound))
    {
        if (VID_Injection[LVar].Driver.Handle == CurrentHandle)
        {
            HandleFound = TRUE;
            strcpy(DeviceName, VID_Injection[LVar].Driver.DeviceName);
        }
        else
        {
            LVar ++;
        }
    }
    if (!HandleFound)
    {
        STTBX_Print(("Warning: %d not found in Table for handle-device name association !!!\n", CurrentHandle));
    }
    return(!HandleFound);
}
#endif

/*-------------------------------------------------------------------------
 * Function : VID_ReportInjectionAlreadyOccurs
 *            Common report function
 * Input    : Injection type
 * Parameter:
 * Output   : TRUE if ok
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
void    VID_ReportInjectionAlreadyOccurs(U32 CDFifoNb)
{
    switch(VID_Injection[CDFifoNb-1].Type)
    {
        case VID_HDD_INJECTION:
            STTBX_Print(("Injection from hdd is already running on fifo %d!!\n", CDFifoNb));
            break;

        case VID_LIVE_INJECTION:
            STTBX_Print(("Injection from live is already running on fifo %d!!\n", CDFifoNb));
            break;

        case VID_MEMORY_INJECTION:
            STTBX_Print(("Injection from memory is already running on fifo %d!!\n", CDFifoNb));
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
    if (VIDTid != NULL )
    {
        if ( VID_Injection[VID_InjectionId].Type == VID_MEMORY_INJECTION)
        {
            /* Lock access */
            semaphore_wait(VID_Injection[VID_InjectionId].Access_p);

#ifdef ST_OSLINUX
            /* Disable the injection flag */
            VID_Injection[VID_InjectionId].Type = NO_INJECTION;
#else

#if defined(ST_5100) || defined(ST_5105) || defined(ST_5107) || defined(ST_5188) || defined(ST_5301) || defined(ST_7100) || defined(ST_7109) || defined(ST_7200) || defined(ST_7710) || defined(ST_5525)
            STVID_DeleteDataInputInterface(VID_Injection[VID_InjectionId].Driver.Handle);
#endif
            /* Disable the injection flag */
            VID_Injection[VID_InjectionId].Type = NO_INJECTION;

#ifdef DVD_TRANSPORT_LINK
            pti_video_dma_close() ;
#endif /* DVD_TRANSPORT_LINK */
#endif  /* LINUX */

            /* Unsubscribe to the underflow event */

            /* Video Load Buffer in use */
            VID_BufferLoad[VID_Injection[VID_InjectionId].Config.Memory.LoadBufferNb - 1].UsedCounter--;
            /* Free access */
            semaphore_signal(VID_Injection[VID_InjectionId].Access_p);

#ifdef ST_OSLINUX
            STTBX_Print(("VID_StopMemInjection: CD fifo %d stopped\n", VID_InjectionId+1));
#else
            STTBX_Print(("VID_StopMemInjection: %d loops performed on CD fifo %d\n",
                         VID_Injection[VID_InjectionId].Config.Memory.LoopNbrDone, VID_InjectionId+1));
#endif
        }
        else
        {
            STTBX_Print(("VID_StopMemInject : Memory injection on CD fifo %d already stopped\n", VID_InjectionId+1));
            RetErr = TRUE;
        }
    }
    else
    {
        /* VIDDecodeFromMemory() has finished to inject once */
        VID_BufferLoad[VID_Injection[VID_InjectionId].Config.Memory.LoadBufferNb - 1].UsedCounter--;

#ifdef ST_OSLINUX
        VID_Injection[VID_InjectionId].Type = NO_INJECTION;

#else

#if defined(ST_5100) || defined(ST_5105) || defined(ST_5107) || defined(ST_5188) || defined(ST_5301) || defined(ST_7100) || defined(ST_7109) || defined(ST_7200) || defined(ST_7710)|| defined(ST_5525)
        STVID_DeleteDataInputInterface(VID_Injection[VID_InjectionId].Driver.Handle);
#endif

        VID_Injection[VID_InjectionId].Type = NO_INJECTION;

        /* Force end of DoVideoInject() */
        VID_KillMemInjection = TRUE;
        semaphore_signal(VidMemInjection_p);
#endif  /* LINUX */
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
#if !defined ST_OSLINUX
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
#endif /* !LINUX */

/*-------------------------------------------------------------------------
 * Function : DoVideoInject
 *            Function is called by subtask
 * Input    :
 * Parameter:
 * Output   : TRUE if ok
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
#if !defined ST_OSLINUX
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
#if defined(ST_5100) || defined(ST_5105) || defined(ST_5107) || defined(ST_5188) || defined(ST_5301) || defined(ST_7100) || defined(ST_7109) || defined(ST_7200) || defined(ST_7710)|| defined(ST_5525)
    U32 								InputBufferFreeSize;
    U32 				SnapInputRead, SnapInputWrite;
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;

#if defined(USE_FDMA_TRANSFER)
	STFDMA_TransferGenericParams_t 		TransferParams;
	STFDMA_TransferId_t 				TransferId1;
    #if defined(mb411) || defined(mb519)
    STAVMEM_BlockHandle_t               FDMANodeHandle;
    #else
	STFDMA_GenericNode_t 				*NotRoundedNode_p;
#endif  /* mb 411 mb 519 */
	STFDMA_GenericNode_t 				*Node_p;
#endif /* USE_FDMA_TRANSFER */
#endif  /* 5100 ... */
    void * CPUDest;
    void * CPUSrc;

    ST_ErrorCode_t ErrCode = ST_NO_ERROR;
    BOOL DoJob;
    U32 BitBufferSize, BitBufferThreshold, BitBufferFreeSize;
    U32 MaxDmaTransferSize = 128*1024;
    U32 RemainingSourceDataSize = 0;
    U32 ErrorModulo = 0; /* Modulo to introduce errors within the injection */

    STOS_TaskEnter(NULL);

#if defined(ST_5100) || defined(ST_5105) || defined(ST_5107) || defined(ST_5188) || defined(ST_5301) || defined(ST_7100) || defined(ST_7109) || defined(ST_7200) || defined(ST_7710)|| defined(ST_5525)
    STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping);
#endif

#if defined(USE_FDMA_TRANSFER)
#if defined(mb411) || defined(mb519)
    /* Allocate node structure in AvmemPartitionHandle[0] (LMI_SYS) */
    ErrCode = vidutil_Allocate(AvmemPartitionHandle[0], (sizeof(STFDMA_GenericNode_t)), 32, (void *)&Node_p, &FDMANodeHandle);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print(("Error ! Unable to allocate FDMA node"));
        return;
    }
#else
	/* Allocate a node structure.								*/
    NotRoundedNode_p = (STFDMA_GenericNode_t *) memory_allocate (NCachePartition_p, (sizeof(STFDMA_GenericNode_t)+31));
    /* Ensure the node pointer is aligned on a 32byte boundary 	*/
    Node_p = (STFDMA_GenericNode_t *) (((U32)NotRoundedNode_p + 31) & ~31);
#endif /* not mb411 not 519 */
#endif /* USE_FDMA_TRANSFER */

	while (1)
    {
        /* Wait the event UNDERFLOW coming from VIDEO */
        semaphore_wait(VidMemInjection_p);
        /* Is vid_kill required ? */
        if (VID_KillMemInjection)
        {
            VID_KillMemInjection = FALSE;
#if defined(USE_FDMA_TRANSFER)
#if defined(mb411) || defined(mb519)
            vidutil_Deallocate(AvmemPartitionHandle[0], &FDMANodeHandle, Node_p);
#else
		    memory_deallocate (NCachePartition_p, NotRoundedNode_p);
#endif /*  mb411  mb519 */
#endif /* USE_FDMA_TRANSFER */

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
                                semaphore_wait(VID_Injection[VID_InjectionId].Access_p);
                                PtiMemInjectVID_Injection_p->JobReadIndex = (PtiMemInjectVID_Injection_p->JobReadIndex+1) % VID_MEMINJECT_MAXJOBNUMBER;
                                DmaTransferSize = PtiMemInjectVID_Injection_p->Jobs[PtiMemInjectVID_Injection_p->JobReadIndex].RequestedSize;

                                /* Free injection access */
                                semaphore_signal(VID_Injection[VID_InjectionId].Access_p);

                                if (DmaTransferSize > (S32)BitBufferFreeSize)
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
#if defined(ST_5100) || defined(ST_5105) || defined(ST_5107) || defined(ST_5188) || defined(ST_5301) || defined(ST_7100) || defined(ST_7109) || defined(ST_7200) || defined(ST_7710) || defined(ST_5525)
                                /* prevent for data pending in input-buffer */
                                if(BitBufferThreshold > ( (U32)VID_Injection[VID_InjectionId].Top_p
                                                         -(U32)VID_Injection[VID_InjectionId].Base_p+1))
                                {
                                    BitBufferThreshold = BitBufferThreshold
                                        -( (U32)VID_Injection[VID_InjectionId].Top_p
                                          -(U32)VID_Injection[VID_InjectionId].Base_p+1);
                                }
#endif /* ST_5100 || ST_5105 || ST_5107 || ST_5188 || ST_5301 || ST_7100 || ST_7109 || ST_7200 || ST_7710 || ST_5525*/
                                if (BitBufferSize-BitBufferFreeSize > BitBufferThreshold)
                                {
                                    DmaTransferSize = 0;
                                    task_reschedule();
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

                            if (DmaTransferSize > (S32)MaxDmaTransferSize)
                                DmaTransferSize = MaxDmaTransferSize;
                            if (DmaTransferSize > (S32)RemainingSourceDataSize)
                                DmaTransferSize = RemainingSourceDataSize;
                            if (DmaTransferSize > 0)
                            {
#if defined(ST_5100) || defined(ST_5105) || defined(ST_5107) || defined(ST_5188) || defined(ST_5301) || defined(ST_7100) || defined(ST_7109) || defined(ST_7200) || defined(ST_7710) || defined(ST_5525)
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
                                if(DmaTransferSize > (S32)InputBufferFreeSize)
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
                        CPUDest = STAVMEM_VirtualToCPU((char *)SnapInputWrite, &VirtualMapping);
                        CPUSrc  = STAVMEM_VirtualToCPU(PtiMemInjectVID_Injection_p->CurrentSrcPtr_p, &VirtualMapping);

#if defined(USE_FDMA_TRANSFER)
								/* Now complete it... */
    							/* Transfer from memory to SWTS, for example */
								Node_p->Node.Next_p					= NULL;
								Node_p->Node.NumberBytes			= DmaTransferSize;
#if defined(mb411) || defined(mb519)
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
#if defined(mb411) || defined(mb519)
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
                        /* Copy data */
#ifdef USE_CRYPTOCORE
                        LxCopy(&crypto, CPUDest, CPUSrc, DmaTransferSize);
#else
                        memcpy(CPUDest, CPUSrc, DmaTransferSize);
#endif /* USE_CRYPTOCORE */
#endif /* USE_FDMA_TRANSFER */
                        /* Corrupt bytes (needed to test error recovery
                           for example) */
                        ErrorModulo++;
                        ErrorModulo %= NUMBER_OF_INJECTIONS_BEFORE_INTRODUCING_ERRORS;
                        if(CorruptMemoryInjection && (ErrorModulo == 0))
                        {
                            /* Corrupt a max of 256 bytes, filling with
                             * 0xFF */
                            if(DmaTransferSize > 256)
                            {
                                memset(CPUDest, 0xFF, 256);
                            }
                            else
                            {
                                memset(CPUDest, 0xFF, DmaTransferSize);
                            }
                        }

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

#else /* not ST_5100 || ST_5105 || ST_5107 || ST_5188 || ST_5301 || ST_7100 || ST_7109 || ST_7710 || ST_5525*/
                                /* Lock pti access */
                                semaphore_wait(VID_PtiAvailable_p);
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

 #if defined (mb295)
                                CPUUserDataWrite(PtiMemInjectVID_Injection_p->CurrentSrcPtr_p,
                                                          (U32)DmaTransferSize, VIDEO_CD_FIFO);
 #else /* mb295 */
                                CPUUserDataWrite(PtiMemInjectVID_Injection_p->CurrentSrcPtr_p,
                                                          (U32)DmaTransferSize, VID_Injection[VID_InjectionId].Address);
 #endif /* mb295 */
#endif

                                /* Lock injection access */
                                semaphore_wait(VID_Injection[VID_InjectionId].Access_p);

                                /* Store Dma transferSize into circular buffer */
                                PtiMemInjectVID_Injection_p->InjectedSizeHistory[PtiMemInjectVID_Injection_p->InjectedSizeHistoryIndex] =
                                    (U32)DmaTransferSize;
                                PtiMemInjectVID_Injection_p->InjectedSizeHistoryIndex = (PtiMemInjectVID_Injection_p->InjectedSizeHistoryIndex+1)
                                    % VID_INJECTEDSIZEHISTORYSIZE;

                                /* Free injection access */
                                semaphore_signal(VID_Injection[VID_InjectionId].Access_p);
#endif /* not ST_5100 ST_5525*/
                            }

                            STOS_TaskDelayUs(1000); /* 1ms */

                            if (DmaTransferSize > 0)
                            {

#ifndef STPTI_UNAVAILABLE
#if !defined ST_5100 && !defined ST_5105 && !defined ST_5107 && !defined ST_5188 && !defined ST_5301 && !defined ST_7100 &&  !defined ST_7109 && !defined ST_7200 && !defined ST_7710 && !defined ST_5525
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
                                semaphore_signal(VID_PtiAvailable_p);
                            }

                            /* Lock injection access */
                            semaphore_wait(VID_Injection[VID_InjectionId].Access_p);

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
                                    semaphore_signal(VID_Injection[VID_InjectionId].Access_p);

                                    VID_StopMemInject(VID_InjectionId);

                                    /* Lock injection access */
                                    semaphore_wait(VID_Injection[VID_InjectionId].Access_p);
                                }
                            }

                            /* Free injection access */
                            semaphore_signal(VID_Injection[VID_InjectionId].Access_p);

                        } /* end if enabled */

                        /* wait 640us, originally 10 tick for OS20 */
                        STOS_TaskDelayUs(640); /* 640 us */
                        if ( !PtiMemInjectVID_Injection_p->SynchronizedByUnderflowEvent &&
                             (VID_Injection[VID_InjectionId].Type == VID_MEMORY_INJECTION))
                        {
                            semaphore_signal(VidMemInjection_p);
                        }
            } /* end if STATE_INJECTION_NORMAL */
        } /* for (MaxSearchVID_InjectionId=VIDEO_MAX_DEVICE;MaxSearchVID_InjectionId;MaxSearchVID_InjectionId--) */
    } /* end of while */

    CorruptMemoryInjection = FALSE;

    STOS_TaskExit(NULL);
} /* end of DoVideoInject() */
#endif /* !ST_OSLINUX */

/*-------------------------------------------------------------------------
 * Function : VID_KillTask
 *            Kill Video Inject task
 * Input    :
 * Output   : FALSE if no error
 * Return   : TRUE if error
 * ----------------------------------------------------------------------*/
static BOOL VID_KillTask( STTST_Parse_t *pars_p, char *result_sym_p )
{
    BOOL    RetErr = FALSE, StillFifoInjection = FALSE;
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;
    S32     FifoNb, WaitEnd, i=0;
#if !defined ST_OSLINUX
    task_t *TaskList[1];
    clock_t waittime;
#endif  /* LINUX */

    RetErr = STTST_GetInteger( pars_p, 1, &FifoNb);
    if ( RetErr || (FifoNb > VIDEO_MAX_DEVICE) )
    {
        sprintf(VID_Msg, "expected Fifo number (1 to %d) or 0 for all injections", VIDEO_MAX_DEVICE);
        STTST_TagCurrentLine( pars_p, VID_Msg);
        return(TRUE);
    }

    RetErr = STTST_GetInteger( pars_p, 0, &WaitEnd);
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected WaitEnd flag (0=default=kill now, 1=wait end of current buffer injection loop" );
        return(TRUE);
    }

    if ((WaitEnd == 1) && (FifoNb == 0))
    {
        STTST_TagCurrentLine( pars_p, "Wait end of current buffer injection loop is not allowed when ALL FIFOs are selected" );
        return(TRUE);
    }

    if (WaitEnd == 1)
    {
        /* Force number of remaining injection loops to 1 */
        /* Let's DoVideoInject() finish current injection */
        i= FifoNb - 1;

        semaphore_wait(VID_Injection[FifoNb - 1].Access_p);
        if ( VID_Injection[FifoNb - 1].Config.Memory.LoopNbr != 0 ) /* if not yet finished */
        {
            VID_Injection[FifoNb - 1].Config.Memory.LoopNbr = 1;
        }
        semaphore_signal(VID_Injection[FifoNb - 1].Access_p);
        /* Wait end of current injection, until end of source buffer */
        STTBX_Print(("Stop injection requested. Waiting for current loop completion...\n"));

#ifdef ST_OSLINUX
        VID_Injection[FifoNb - 1].Driver.DriverState = STATE_DRIVER_STOPPED;

#else

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
#endif  /* LINUX */
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
#ifdef ST_OSLINUX
                    if (!RetErr)
                    {
                        STTBX_Print(("VID_KillTask(): VID_StopMemInject(FifoNb=%d): video injection stopped\n", FifoNb));
                    }
                    ErrCode = STVIDTEST_KillInjectFromStreamBuffer(FifoNb, WaitEnd);
                    if (ErrCode == ST_NO_ERROR)
                    {
                        STTBX_Print(("VID_KillTask(): STVIDTEST_KillInjectFromStreamBufferwaiting(FifoNb=%d): ioctl video injection stop successful\n", FifoNb));
                    }
                    else
                    {
                        STTBX_Print(("VID_KillTask(): STVIDTEST_KillInjectFromStreamBufferwaiting(FifoNb=%d) failure 0x%x !\n", FifoNb, ErrCode));
                    }
#endif /* ST_OSLINUX */
                }
                else
                {
                    StillFifoInjection = TRUE;
                }
            }
        }

#ifdef ST_OSLINUX
    if((!(StillFifoInjection)) && (!(RetErr)))
    {
            StillFifoInjection = FALSE;
            VIDTid = NULL;
            STTBX_Print(("VID_KillTask(): video injection task killed\n"));
        }
#else

        if((!(StillFifoInjection)) && (!(RetErr)))
        {
            STTBX_Print(("VID_KillTask(): waiting for injection task termination ... \n"));
            VID_KillMemInjection = TRUE;
            CorruptMemoryInjection = FALSE;
            semaphore_signal(VidMemInjection_p);
            TaskList[0] = VIDTid;
            waittime = STOS_time_plus(STOS_time_now(), ST_GetClocksPerSecond() * 10);
            ErrCode = STOS_TaskWait( &TaskList[0], &waittime );
            if ( ErrCode != ST_NO_ERROR )
            {
                STTBX_Print(("VID_KillTask(): STOS_TaskWait() failure (error=0x%x buff=%d State=%d Type=%d) !\n",
                        ErrCode, VID_Injection[FifoNb-1].Config.Memory.LoadBufferNb,
                        VID_Injection[FifoNb - 1].Driver.DriverState, VID_Injection[FifoNb-1].Type));
                RetErr = TRUE;
            }
            else
            {
                STTBX_Print(("VID_KillTask(): video task finished\n"));
            }
            ErrCode = STOS_TaskDelete(VIDTid, DriverPartition_p, TaskStack, DriverPartition_p);
            if ( ErrCode!= ST_NO_ERROR)
            {
                STTBX_Print(("VID_KillTask(): STOS_TaskDelete() failure (error=0x%x buff=%d Type=%d) !\n",
                        ErrCode, VID_Injection[FifoNb-1].Config.Memory.LoadBufferNb, VID_Injection[FifoNb-1].Type));
                RetErr = TRUE;
            }
            VIDTid = NULL;
        }
#endif /* !LINUX */
    }
    else
    {
        STTBX_Print(("VID_KillTask(): No injection task currently running.\n"));
    }
    STTST_AssignInteger(result_sym_p, VID_Injection[i].Config.Memory.LoopNbrDone, FALSE);
    return(RetErr);
} /* end of VID_KillTask() */

/*-------------------------------------------------------------------------
 * Function : VID_MemInject
 *            Inject Video in Memory to DMA
 * Input    : *pars_p, *Result
 * Output   : FALSE if no error
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_MemInject( STTST_Parse_t *pars_p, char *result_sym_p )
{
    BOOL            RetErr = FALSE;
 #if !defined ST_OSLINUX
    void           *Param = NULL;
#endif
    S32             FifoNb, InjectLoop, BufferNb;
    ST_ErrorCode_t  ErrCode;
    STVID_Handle_t  CurrentHandle;

#if !defined ST_OSLINUX
    S32 i;
 #if defined(ST_5100) || defined(ST_5105) || defined(ST_5107) || defined(ST_5188) || defined(ST_5301) || defined(ST_7100) || defined(ST_7109) || defined(ST_7200) || defined(ST_7710)|| defined(ST_5525)
    U32 InputBuffersize;
 #endif
#endif

#if !defined STUB_INJECT
    RetErr = STTST_GetInteger( pars_p, 1, &InjectLoop );
    if ( RetErr )
    {
        STTST_TagCurrentLine(pars_p, "expected number of loops (-1 for infinite)");
        return(TRUE);
    }

    RetErr = STTST_GetInteger( pars_p, DefaultVideoHandle, (S32*)&CurrentHandle);
    if ( RetErr )
    {
        STTST_TagCurrentLine(pars_p, "expected video handle");
        return(TRUE);
    }
    DefaultVideoHandle = CurrentHandle ;

    RetErr = STTST_GetInteger( pars_p, 1, &FifoNb);
    if ((RetErr ) || (FifoNb < 1) || (FifoNb > VIDEO_MAX_DEVICE))
    {
        sprintf(VID_Msg, "expected Fifo number (1 to %d)", VIDEO_MAX_DEVICE);
        STTST_TagCurrentLine( pars_p, VID_Msg);
        return(TRUE);
    }

    RetErr = STTST_GetInteger( pars_p, 1, &BufferNb);
    if (( RetErr ) || (BufferNb < 1) || (BufferNb > VID_MAX_BUFFER_LOAD))
    {
        sprintf(VID_Msg, "expected load buffer number (default is 1. max is %d)", VID_MAX_BUFFER_LOAD);
        STTST_TagCurrentLine( pars_p, VID_Msg);
        return(TRUE);
    }

    if(VID_BufferLoad[BufferNb-1].AllocatedBuffer_p == NULL)
    {
        STTBX_Print(( "No file loaded in memory buffer !!\n" ));
        return(TRUE);
    }

    /* Lock injection access */
    semaphore_wait(VID_Injection[FifoNb-1].Access_p);

 #ifdef ST_OSLINUX
    ErrCode = STVIDTEST_InjectFromStreamBuffer(
                                CurrentHandle,
                                InjectLoop,
                                FifoNb,
                                BufferNb,
                                (void *)STAPIGAT_UserToKernel(VID_BufferLoad[BufferNb-1].AlignedBuffer_p),
                                VID_BufferLoad[BufferNb-1].NbBytes);
    if (ErrCode == ST_NO_ERROR)
    {
        VID_Injection[FifoNb-1].Type = VID_MEMORY_INJECTION;
        VID_Injection[FifoNb-1].Driver.Handle = CurrentHandle;
        VID_Injection[FifoNb-1].Config.Memory.LoadBufferNb = BufferNb;

        VIDTid = (task_t*) 1;           /* Something different from NULL */

        VID_BufferLoad[BufferNb-1].UsedCounter++;
    }

 #else /* LINUX */

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
                ErrCode = STOS_TaskCreate ((void (*)(void*))DoVideoInject,
                                           Param,
                                           DriverPartition_p,
                                           STACK_SIZE,
                                           &TaskStack,
                                           DriverPartition_p,
                                           &VIDTid,
                                           &TaskDesc,
                                           VID_INJECTION_TASK_PRIORITY,
                                           "VideoInject",
                                           task_flags_no_min_stack_size ) ;
                if ( ErrCode != ST_NO_ERROR )
                {
                    STTBX_Print(("VID_MemInject(): Unable to create video task (error=0x%x)!\n", ErrCode));
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
                VID_Injection[FifoNb-1].Config.Memory.SrcBuffer_p = VID_BufferLoad[BufferNb-1].AlignedBuffer_p;
                VID_Injection[FifoNb-1].Config.Memory.CurrentSrcPtr_p = VID_BufferLoad[BufferNb-1].AlignedBuffer_p;
                VID_Injection[FifoNb-1].Config.Memory.SrcSize = VID_BufferLoad[BufferNb-1].NbBytes;
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
#if defined(ST_5100) || defined(ST_5105) || defined(ST_5107) || defined(ST_5188) || defined(ST_5301) || defined(ST_7100) || defined(ST_7109) || defined(ST_7200) || defined(ST_7710)|| defined(ST_5525)
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
                STVID_SetDataInputInterface( VID_Injection[FifoNb-1].Driver.Handle,
                                             GetWriteAddress,
                                             InformReadAddress,
                                             (void*)(FifoNb-1));
#else /* not ST_5100 && not ST_5105 && not ST_5107 && not ST_5188 && not ST_5301 && not ST_7100 && not ST_7109 && not ST_7710 && not ST_5525*/
                /* Subscribe to underflow event */
#ifndef DVD_TRANSPORT_STPTI
                pti_video_dma_state( discard_data );
                pti_video_dma_reset();
#ifdef DVD_TRANSPORT_LINK
                pti_video_dma_open() ;
#endif /* DVD_TRANSPORT_LINK */
#endif /* DVD_TRANSPORT_STPTI */
#endif /* not ST_5100 ST_5525*/

                VID_Injection[FifoNb-1].Type = VID_MEMORY_INJECTION;
                /* Launch injection */
                semaphore_signal(VidMemInjection_p);

                /* Video Load Buffer in use */
                VID_BufferLoad[BufferNb-1].UsedCounter++;
            }
        }
        else
        {
            VID_ReportInjectionAlreadyOccurs(FifoNb);
        }
    }
 #endif  /* LINUX */

    /* Free injection access */
    semaphore_signal(VID_Injection[FifoNb-1].Access_p);
#endif /* !STUB_INJECT */

    STTST_AssignInteger(result_sym_p, FALSE, FALSE);
    return(API_EnableError ? RetErr : FALSE );
} /* end of VID_MemInject() */


#if !defined ST_OSLINUX
/* ------------------------------------------------------------------------------
 * Name        : VID_MemErrorsInject
 * Description : Inject Video in Memory to DMA with

 * Parameters  : *pars_p, *result_sym_p
 * Assumptions :
 * Limitations :
 * Returns     :   False (Success)  True (Error)
 * --------------------------------------------------------------------------- */
static BOOL VID_MemErrorsInject(STTST_Parse_t *pars_p, char *result_sym_p )
{
    BOOL RetErr;

    CorruptMemoryInjection = TRUE;
    RetErr = VID_MemInject(pars_p, result_sym_p);

    return(RetErr);

} /* end of VID_MemErrorsInject() */
#endif /* !ST_OSLINUX */

/* ------------------------------------------------------------------------------
 * Name        : VID_CDFifoStatus
 * Description : Display the injection task status
 * Parameters  : *pars_p, *result_sym_p
 * Assumptions :
 * Limitations :
 * Returns     :   False (Success)  True (Error)
 * --------------------------------------------------------------------------- */
#if !defined ST_OSLINUX
static BOOL VID_CDFifoStatus(parse_t *pars_p, char *result_sym_p)
{
    BOOL		RetErr;
    U32			VID_InjectionId;
    U32                 i,j;
    S32                 JobNumber;
    VID_Injection_t     VID_Injection_Copy; /* Local copy for capture status */

    RetErr = FALSE;
    UNUSED_PARAMETER(pars_p);

    for (VID_InjectionId=0; VID_InjectionId<VIDEO_MAX_DEVICE; VID_InjectionId++)
    {
        /* Copy to avoid modification of data during the status report     */
        /* Warning on ST20, memcopy lock the system. No need of semaphore  */
        /* which is not a good idea for a status function !                */
        memcpy((void *)&VID_Injection_Copy, (void *)&(VID_Injection[VID_InjectionId]), sizeof(VID_Injection_t));
        STTBX_Print(("\n  CD FiFo %d injection : ", VID_InjectionId+1));

        switch(VID_Injection_Copy.Type)
        {
            case NO_INJECTION:
                STTBX_Print(("None\n"));
                break;
            case VID_HDD_INJECTION:
                STTBX_Print(("HDD\n"));
                break;
            case VID_LIVE_INJECTION:
                STTBX_Print(("Live\n"));
                break;
            case VID_MEMORY_INJECTION:
                STTBX_Print(("Memory\n"));
                break;
            default:
                STTBX_Print(("Not yet described\n"));
                break;
        }
        STTBX_Print(("\tAddress = 0x%x", VID_Injection_Copy.Address));
        if(VID_Injection_Copy.BitBufferSize != 0)
        {
            STTBX_Print((", BitBufferSize = %d bytes", VID_Injection_Copy.BitBufferSize));
        }
        STTBX_Print(("\n"));
        if (VID_Injection_Copy.Type == VID_MEMORY_INJECTION)
        {
            if(VID_Injection_Copy.Config.Memory.LoopNbr < 0)
            {
                STTBX_Print(("\tInfinite loops   "));
            }
            else
            {
                STTBX_Print(("\tLoops to do = %d   ",
                             VID_Injection_Copy.Config.Memory.LoopNbrDone + VID_Injection_Copy.Config.Memory.LoopNbr));
            }
            STTBX_Print(("Already Done = %d\n", VID_Injection_Copy.Config.Memory.LoopNbrDone));
            STTBX_Print(("\tVid load buffer no %d Stream Addres=0x%x Stream Size=%d\n",
                         VID_Injection_Copy.Config.Memory.LoadBufferNb,
                         (U32) VID_Injection_Copy.Config.Memory.SrcBuffer_p,
                         VID_Injection_Copy.Config.Memory.SrcSize ));
                STTBX_Print(("\tVideo hande = %d, State = %s\n", VID_Injection_Copy.Driver.Handle,
                             (VID_Injection_Copy.Driver.DriverState == STATE_DRIVER_STARTED) ? "Started" :
                             (VID_Injection_Copy.Driver.DriverState == STATE_DRIVER_STOPPED) ? "Stopped" :
                             "????"));
            if (VID_Injection_Copy.Config.Memory.SynchronizedByUnderflowEvent)
            {
                JobNumber = VID_Injection_Copy.Config.Memory.JobWriteIndex
                    - VID_Injection_Copy.Config.Memory.JobReadIndex;
                if (JobNumber<0)
                {
                    JobNumber = (JobNumber+VID_MEMINJECT_MAXJOBNUMBER)&VID_MEMINJECT_MAXJOBNUMBER;
                }
                STTBX_Print(("Waiting Jobs = %d\n",JobNumber));
            }

            STTBX_Print(("\tInjected size (Newest) :"));
            j = (VID_Injection_Copy.Config.Memory.InjectedSizeHistoryIndex-1)
                % VID_INJECTEDSIZEHISTORYSIZE;;
            i = j;
            do
            {
                if ( (U32)VID_Injection_Copy.Config.Memory.InjectedSizeHistory[i]
                     != VID_INVALID_INJECTEDSIZE )
                {
                    if((i-j)%10 == 0)
                    {
                        STTBX_Print(("\n\t"));
                    }
                    if ( VID_Injection_Copy.Config.Memory.InjectedSizeHistory[i] < 1024)
                    {
                        STTBX_Print(("%db ", VID_Injection_Copy.Config.Memory.InjectedSizeHistory[i]));
                    }
                    else
                    {
                        STTBX_Print(("%dKb ", VID_Injection_Copy.Config.Memory.InjectedSizeHistory[i]/1024));
                    }
                }
                i = (i-1)%VID_INJECTEDSIZEHISTORYSIZE;
            } while(i!=j);
            STTBX_Print((" (Oldest)\n"));
        }
        if (VID_Injection_Copy.Type == VID_LIVE_INJECTION)
        {
            STTBX_Print(("\tPTI device = %d, video slot = %d, PID = 0x%x = %d\n",
                         VID_Injection_Copy.Config.Live.DeviceNb,
                         VID_Injection_Copy.Config.Live.SlotNb, VID_Injection_Copy.Config.Live.Pid,
                          VID_Injection_Copy.Config.Live.Pid));
        }
    } /* for (VID_InjectionId=0;VID_InjectionId<PTI_CDFIFO_NBR;VID_InjectionId++) */

    STTST_AssignInteger(result_sym_p, RetErr, FALSE);
    return ( RetErr );

} /* End VID_CDFifoStatus */
#endif /* !LINUX */

/*-------------------------------------------------------------------------
 * Function : VID_SetInjectLevel
 *
 * Input    : *pars_p, *result_sym_p
 * Output   : FALSE if no error
 * Return   : TRUE if error, else FALSE
 * ----------------------------------------------------------------------*/
#if !defined ST_OSLINUX
static BOOL VID_SetInjectLevel(STTST_Parse_t *pars_p, char *result_sym_p )
{
    BOOL RetErr;
    S32 LVar;
    RetErr = STTST_GetInteger( pars_p, 100, &LVar);
    if (( RetErr ) || (LVar < 10) || (LVar > 100))
    {
        sprintf(VID_Msg, "expected value between %d and %d (stands for 7.5 to 75 percents of bitbuffer size)", 10 ,100);
        STTST_TagCurrentLine( pars_p, VID_Msg);
        return(TRUE);
    }

    gInjectLevel = LVar;
    LVar = LVar * 75 / 100;
    STTBX_Print(( "Injection level set to %d percent of bit buffer\n", LVar ));
    STTST_AssignInteger(result_sym_p, RetErr, FALSE);
    return(TRUE);

} /* end of VID_SetInjectLevel() */
#endif /* !LINUX */

/*-------------------------------------------------------------------------
 * Function : VIDDecodeFromMemory
 *            copy a stream from buffer to video bit buffer
 * Input    :
 * Output   : FALSE if no error
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VIDDecodeFromMemory(STTST_Parse_t *pars_p, char *result_sym_p)
{
    S32             BuffNb;
    BOOL            RetErr = FALSE;
    ST_ErrorCode_t  ErrCode = ST_NO_ERROR;
    S32             FifoNb;
#if !defined ST_OSLINUX
    S32             Count;
#endif
    STVID_Handle_t  CurrentHandle;
    ST_DeviceName_t CurrDeviceName;
    STEVT_DeviceSubscribeParams_t DevSubscribeParams;
    S32             NbLoops;
    STEVT_Handle_t  LocEvtHandle;
    STEVT_OpenParams_t OpenParams;
#if !defined ST_OSLINUX
#if defined(ST_5100) || defined(ST_5105) || defined(ST_5107) || defined(ST_5188) || defined(ST_5301) || defined(ST_7100) || defined(ST_7109) || defined(ST_7200) || defined(ST_7710) || defined(ST_5525)
    U32 InputBuffersize;
#endif
#endif /* !ST_OSLINUX */

    RetErr = STTST_GetInteger( pars_p, DefaultVideoHandle, (S32*)&CurrentHandle);
    if ( RetErr )
    {
        STTST_TagCurrentLine(pars_p, "expected video handle");
        return(TRUE);
    }
    DefaultVideoHandle = CurrentHandle;

    RetErr = STTST_GetInteger( pars_p, 1, &FifoNb);
    if ((RetErr ) || (FifoNb < 1) || (FifoNb > VIDEO_MAX_DEVICE))
    {
        sprintf(VID_Msg, "expected Fifo number (1 to %d)", VIDEO_MAX_DEVICE);
        STTST_TagCurrentLine( pars_p, VID_Msg);
        return(TRUE);
    }

    RetErr = STTST_GetInteger( pars_p, 1, &BuffNb);
    if (( RetErr ) || (BuffNb < 1) || (BuffNb > VID_MAX_BUFFER_LOAD))
    {
        sprintf(VID_Msg, "expected load buffer number (default is 1. max is %d)", VID_MAX_BUFFER_LOAD);
        STTST_TagCurrentLine( pars_p, VID_Msg);
        return(TRUE);
    }

    if(VID_BufferLoad[BuffNb-1].AllocatedBuffer_p == NULL)
    {
        STTBX_Print(( "No file loaded in memory buffer !!\n" ));
        return(TRUE);
    }

    RetErr = STTST_GetInteger( pars_p, 1, &NbLoops);
    if ((RetErr ) || (NbLoops < 1) || (NbLoops > 9999))
    {
        sprintf(VID_Msg, "expected # of loops (1 to 9999)");
        STTST_TagCurrentLine( pars_p, VID_Msg);
        return(TRUE);
    }

 #ifdef ST_OSLINUX
    ErrCode = STVIDTEST_DecodeFromStreamBuffer(
                                CurrentHandle,
                                FifoNb,
                                BuffNb,
                                NbLoops,
                                (void *)STAPIGAT_UserToKernel(VID_BufferLoad[BuffNb-1].AlignedBuffer_p),
                                VID_BufferLoad[BuffNb-1].NbBytes);

 #else /* LINUX */

    /* Lock injection access */
    semaphore_wait(VID_Injection[FifoNb-1].Access_p);

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
    semaphore_signal(VID_Injection[FifoNb-1].Access_p);
    if (RetErr)
    {
        return(TRUE);
    }

    VID_Injection[FifoNb-1].Config.Memory.SrcBuffer_p = VID_BufferLoad[BuffNb-1].AlignedBuffer_p;
    VID_Injection[FifoNb-1].Config.Memory.CurrentSrcPtr_p = VID_BufferLoad[BuffNb-1].AlignedBuffer_p;
    VID_Injection[FifoNb-1].Config.Memory.SrcSize = VID_BufferLoad[BuffNb-1].NbBytes;
    VID_Injection[FifoNb-1].Config.Memory.LoadBufferNb = BuffNb;
    VID_Injection[FifoNb-1].Config.Memory.LoopNbr = NbLoops;
    VID_Injection[FifoNb-1].Config.Memory.LoopNbrDone = 0;
    VID_Injection[FifoNb-1].Config.Memory.SynchronizedByUnderflowEvent = FALSE;
    for(Count=0;Count<VID_INJECTEDSIZEHISTORYSIZE;Count++)
    {
        VID_Injection[FifoNb-1].Config.Memory.InjectedSizeHistory[Count] = VID_INVALID_INJECTEDSIZE;
    }
    VID_Injection[FifoNb-1].Config.Memory.InjectedSizeHistoryIndex = 0;

  #if defined(ST_5100) || defined(ST_5105) || defined(ST_5107) || defined(ST_5188) || defined(ST_5301) || defined(ST_7100) || defined(ST_7109) || defined(ST_7200) || defined(ST_7710) || defined(ST_5525)
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

  #else /* not ST_5100 ST_5105 ST_5107 ST_5188 ST_5301 ST_7100 ST_7109 ST_7710 ST_5525*/

    /* Subscribe to underflow event */
    #ifndef DVD_TRANSPORT_STPTI
    pti_video_dma_state( discard_data );
    pti_video_dma_reset();
     #ifdef DVD_TRANSPORT_LINK
    pti_video_dma_open() ;
     #endif /* DVD_TRANSPORT_LINK */
    #endif /* DVD_TRANSPORT_STPTI */

  #endif /* not ST5100 .. */

    VID_Injection[FifoNb-1].Type = VID_MEMORY_INJECTION;
    /* Launch injection */
    semaphore_signal(VidMemInjection_p);

    /* Launch Injection */
    if (VID_Injection[FifoNb-1].Driver.DriverState != STATE_DRIVER_STOPPED)
    {
        /* Video Load Buffer in use */
        VID_BufferLoad[BuffNb-1].UsedCounter++;
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
#endif  /* LINUX */

    /* --------------- Wait for decode completion (polling decode events generation) --------------- */
#ifndef STVID_VIDDEC_PBO_ENABLED /* We have not PBO */
    ErrCode = VID_GetNameFromHandle(CurrentHandle, CurrDeviceName);
    if (ErrCode == ST_NO_ERROR)
    {
        DevSubscribeParams.NotifyCallback = (STEVT_DeviceCallbackProc_t)VID_CatchNewPictureDecodedEvt;
        DevSubscribeParams.SubscriberData_p = (void *)CurrentHandle;
        ErrCode = STEVT_Open(STEVT_DEVICE_NAME, &OpenParams, &LocEvtHandle);
        if ( ErrCode== ST_NO_ERROR )
        {
            ErrCode = STEVT_SubscribeDeviceEvent(LocEvtHandle, CurrDeviceName,
                        (STEVT_EventConstant_t)STVID_NEW_PICTURE_TO_BE_DISPLAYED_EVT, &DevSubscribeParams);
            /*   if ((ErrCode==ST_NO_ERROR) || (ErrCode==STEVT_ERROR_ALREADY_SUBSCRIBED))*/
            if (ErrCode==ST_NO_ERROR)
            {
                /* wait for a timer completion. This timer is reset by VID_CatchNewPictureDecodedEvt
                * while events are received. */
                /*  Decode is considered as complete if no STVID_NEW_PICTURE_TO_BE_DISPLAYED_EVT occurred for more than 150 ms */
                WaitDecodeCompletionCounter = 0;
                WaitDecodeCompletionCounterVideoHandle = CurrentHandle;

                while (WaitDecodeCompletionCounter < 150 )
                {
                    STOS_TaskDelayUs(1000); /* 1ms */
                    WaitDecodeCompletionCounter++;
                }
                WaitDecodeCompletionCounterVideoHandle = 0;
                if (ErrCode==ST_NO_ERROR)
                {
                    ErrCode |= STEVT_UnsubscribeDeviceEvent( LocEvtHandle, CurrDeviceName,
                                (STEVT_EventConstant_t)STVID_NEW_PICTURE_TO_BE_DISPLAYED_EVT);
                }
            }
            else
            {
                STTBX_Print(("Error while subscribing STVID_NEW_PICTURE_TO_BE_DISPLAYED_EVT : %x\n", ErrCode ));
            }
            STEVT_Close(LocEvtHandle);
        }
        else
        {
            STTBX_Print(("STEVT_Open error while subscribing STVID_NEW_PICTURE_TO_BE_DISPLAYED_EVT : %x\n", ErrCode ));
        }
    }
#endif /* STVID_VIDDEC_PBO_ENABLED We have not PBO */

#if !defined ST_OSLINUX
    /* Lock injection access */
    semaphore_wait(VID_Injection[FifoNb-1].Access_p);

    VID_Injection[FifoNb-1].Type = NO_INJECTION;

    /* Free injection access */
    semaphore_signal(VID_Injection[FifoNb-1].Access_p);
#endif /* !LINUX */

    STTST_AssignInteger(result_sym_p, FALSE, FALSE);
    return(FALSE);

} /* end of VIDDecodeFromMemory() */




/***************************************************/
/*****************  NOT LINUX  *********************/
/***************************************************/
#if !defined ST_OSLINUX

/*-------------------------------------------------------------------------
 * Function : VIDFlushVideoCDFIFO
 *            Inject a specific sequece of data (discontinuity start code,
 *            and dummy values (0xFF) )
 * Input    :
 * Output   : FALSE if no error
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VIDFlushVideoCDFIFO(STTST_Parse_t *pars_p, char *result_sym_p)
{
#if 0
    #define VIDEO_FLUSH_SIZE  256      /* The video input CD fifo is 256 bytes */

    unsigned char video_flush[VIDEO_FLUSH_SIZE];
    unsigned int i;
#ifdef DVD_TRANSPORT_STPTI
    ST_ErrorCode_t          ErrCode = ST_NO_ERROR;
    STPTI_DMAParams_t       STDMA_Params;
    #ifdef DVD_TRANSPORT_STPTI4
    U32 lDMAUSed;
    #endif
#else /* DVD_TRANSPORT_STPTI */
    BOOL RetErr;
#endif /* DVD_TRANSPORT_STPTI */

    /* --- Prepare data --- */

    /* the decoder needs to reach a start code in order to become idle after a
     * picture decode; we insert here a picture start code 00 00 01 00 */
    i = 0;
    video_flush[i++] = 0x00;
    video_flush[i++] = 0x00;
    video_flush[i++] = 0x01;
    video_flush[i++] = 0xB6;
    /* the input decoder FIFO is of VIDEO_FLUSH_SIZE bytes, in order to
    * be sure that all data sent will be decoded we need to flush all that FIFO
    * into the bit buffer.
    * To do this we will send VIDEO_FLUSH_SIZE extra bytes at the end of any data
    * sending, just to make sure all relevant data are flushed.	*/
    while (i < VIDEO_FLUSH_SIZE)
    {
        video_flush[i] = 0xFF;
        i ++;
    }

    /* Start copy from video_flush to video bit buffer */

#ifdef DVD_TRANSPORT_STPTI
#if defined(ST_7015)
    STDMA_Params.Destination = (STI7015_BASE_ADDRESS + ST7015_VIDEO_CD_FIFO_OFFSET); /* XD change to new define */
    STDMA_Params.Holdoff = 1;
    STDMA_Params.WriteLength = 4;
#elif defined(ST_7020)
    STDMA_Params.Destination = (STI7020_BASE_ADDRESS + ST7020_VIDEO_CD_FIFO_OFFSET); /* XD change to new define */
    STDMA_Params.Holdoff = 1;
    STDMA_Params.WriteLength = 4;
#elif defined(ST_5100) || defined(ST_5105) || defined(ST_5107) || defined(ST_5188) || defined(ST_5301) || defined(ST_7100) || defined(ST_7109) || defined(ST_7200) || defined(ST_7710)|| defined(ST_5525)
    STDMA_Params.Destination = 0;
#else
    STDMA_Params.Destination    = VIDEO_CD_FIFO;
    STDMA_Params.Holdoff        = 1;
    STDMA_Params.WriteLength    = 1;
#endif
#ifndef STPTI_5_1_6
    STDMA_Params.CDReqLine = STPTI_CDREQ_VIDEO;
#endif

#ifndef STPTI_UNAVAILABLE
#ifdef DVD_TRANSPORT_STPTI4
    STDMA_Params.DMAUsed = &lDMAUSed;
    STDMA_Params.BurstSize = STPTI_DMA_BURST_MODE_ONE_BYTE;
#endif
#endif /* not STPTI_UNAVAILABLE */

#ifndef STPTI_UNAVAILABLE
#if defined(ST_5100) || defined(ST_5105) || defined(ST_5107) || defined(ST_5188) || defined(ST_5301) || defined(ST_7100) || defined(ST_7109) || defined(ST_7200) || defined(ST_7710)|| defined(ST_5525)
    /* todo */
#else
    ErrCode = STPTI_UserDataWrite((U8 *)video_flush, (U32)VIDEO_FLUSH_SIZE, &STDMA_Params);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print(("STPTI_UserDataWrite() : ErrCode=%x, %d bytes bufferized\n",
            ErrCode, VIDEO_FLUSH_SIZE ));
    }
    else
    {
        ErrCode = STPTI_UserDataSynchronize(&STDMA_Params);  /* Wait before use */
        if (ErrCode != ST_NO_ERROR)
        {
            STTBX_Print(("STPTI_UserDataSynchronize() : ErrCode=%x\n",
                ErrCode ));
        }
    }
#endif /* ST_5100 || ST_5105 || ST_5107 || ST_5188 || ST_5301 || ST_7100 || ST_7109 || ST_7710 || ST_5525 */
#else
    CPUUserDataWrite((U8 *)video_flush, (U32)VIDEO_FLUSH_SIZE, STDMA_Params.Destination);
#endif /* STPTI_UNAVAILABLE */

#else /* DVD_TRANSPORT_STPTI -> link or pti */

    #ifdef DVD_TRANSPORT_LINK
    pti_video_dma_open() ;
    #endif
    RetErr = pti_video_dma_inject((unsigned char *)video_flush, VIDEO_FLUSH_SIZE);
    if (RetErr)
    {
        STTBX_Print(("pti_video_dma_inject() : RetErr=%x, %d bytes bufferized\n",
            RetErr, VIDEO_FLUSH_SIZE ));
    }
    else
    {
        RetErr = pti_video_dma_synchronize(); /* not blocking after error */
        if (RetErr)
        {
            STTBX_Print(("pti_video_dma_synchronize() : RetErr=%x \n", RetErr));
        }
    }
    #ifdef DVD_TRANSPORT_LINK
    pti_video_dma_close() ;
    #endif

#endif


#ifdef DVD_TRANSPORT_STPTI
    if (ErrCode != ST_NO_ERROR)
#else
    if (RetErr)
#endif
    {
        STTBX_Print(("Error in VIDFlushVideoCDFIFO()\n"));
    }
#endif  /* 0 */
    UNUSED_PARAMETER(pars_p);

    STTST_AssignInteger(result_sym_p, FALSE, FALSE);
    return(FALSE);

} /* end of VIDFlushVideoCDFIFO() */


/*******************************************************************************
Name        : SetCDFIFO
Description : Set the programmable CD FIFO
Parameters  :
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if no problem
              ST_ERROR_NO_MEMORY if allocation failed
*******************************************************************************/
static ST_ErrorCode_t SetCDFIFO(const VIDTRICK_BackwardBuffer_t Buffer)
{
#define HALTRICK_Write8(Address, Value)  STSYS_WriteRegMem8((void *) (Address), (Value))
#define HALTRICK_Read8(Address)     STSYS_ReadRegMem8((void *) (Address))

#define VID_CWL             0x31 /* CD write launch.                          */
#define VID_TP_LDP          0x3F /* CD FIFO Load pointer.                     */
#define VID_TP_CD0          0xCA /* CD FIFO pointer load address.             */
#define VID_TP_CD1          0xCB /* CD FIFO pointer load address.             */
#define VID_TP_CD2          0xCC /* CD FIFO pointer load address.             */
#define VID_TP_CDLIMIT0     0xE0 /* CD FIFO write limit address.              */
#define VID_TP_CDLIMIT1     0xE1 /* CD FIFO write limit address.              */
#define VID_TP_CDLIMIT2     0xE2 /* CD FIFO write limit address.              */
#define CFG_CDR             0x44   /* Manual compressed data input ( debug and still load */
#define VID_ITS             0x3D /* CD FIFO Load pointer.                     */

#define VID_CWL_CWL         0x01 /* Bit CD write launch.                      */
#define VID_TP_LDP_TM       0x02 /* Bit enable Trick Mode.                    */
    U32 k;
    U32 count;
    U32 Address;
    /* The video input CD fifo is 128 bytes + 4 more bytes due to PES parser */
    #define FLUSH_SIZE 132
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Set register bit Trick Mode Enable. */
    HALTRICK_Write8(VID_TP_LDP, VID_TP_LDP_TM);

    /* Flush CD FIFO */
    /* TEMPORARY_WHILE_HAL_DECODE_NOT_AVAILABLE_HERE */
    for (k = 0; k < 64; k++)
    {
        HALTRICK_Write8(CFG_CDR, 0xFF);
    }

    /* Fill memory with a pattern. */
    for (k = (U32)((U8*)(Buffer.StartAddress_p)); k < ((U32)((U8*)(Buffer.StartAddress_p)) + Buffer.BufferSize + 100) ; k++)
    {
        HALTRICK_Write8(k, 0xaa);
    }

    /* Write the address of the buffer in the pointer of the CDFIFO */
    Address = (U32)((U8*)(Buffer.StartAddress_p) - SDRAM_BASE_ADDRESS) >> 6 /*(= unit of 64 bits)*/;
    HALTRICK_Write8(VID_TP_CD0, (Address & 0x10000)>>16);
    HALTRICK_Write8(VID_TP_CD1, (Address & 0xFF00) >>8);
    HALTRICK_Write8(VID_TP_CD2, (Address & 0xFF));

    /* Write the address of the end of the buffer in the pointer of the CDFIFO */
    Address = (U32)((U8*)(Buffer.StartAddress_p) + Buffer.BufferSize - SDRAM_BASE_ADDRESS) >> 6 /*(= unit of 64 bits)*/;
    HALTRICK_Write8(VID_TP_CDLIMIT0, (Address & 0x10000)>>16);
    HALTRICK_Write8(VID_TP_CDLIMIT1, (Address & 0xFF00) >>8);
    HALTRICK_Write8(VID_TP_CDLIMIT2, (Address & 0xFF));

    /* Set bit CWL : reset CD FIFO. */
    HALTRICK_Write8(VID_CWL, VID_CWL_CWL);

    count = 0;
    HALTRICK_Write8(0x3c, 0x80);
    while ( ( (HALTRICK_Read8(VID_ITS) & 0x80) != 0) && (count < 10))
    {
        STOS_TaskDelayUs(DELAY_1S);
        count++;
    }
    if (count == 10)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "IT CWR not received !!!"));
    }
    else
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "IT CWR received in %d sec ", count));
    }

    return(ErrorCode);
} /* End of SetCDFIFO() function */

/*******************************************************************************
Name        : ReportRegisters
Description :
Parameters  :
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if no problem
              ST_ERROR_NO_MEMORY if allocation failed
*******************************************************************************/
static ST_ErrorCode_t ReportRegisters(void)
{
#define HALTRICK_Read(Address)     STSYS_ReadRegMem8((void *) (Address))

#define VID_STL             0x30
#define VID_TP_LDP          0x3F
#define VID_TP_CD0          0xCA
#define VID_TP_CD1          0xCB
#define VID_TP_CD2          0xCC
#define VID_TP_CD_RD0       0x83
#define VID_TP_CD_RD1       0x84
#define VID_TP_CD_RD2       0x85
#define VID_TP_CDLIMIT0     0xE0
#define VID_TP_CDLIMIT1     0xE1
#define VID_TP_CDLIMIT2     0xE2
#define VID_TP_SCD0         0xC0
#define VID_TP_SCD1         0xC1
#define VID_TP_SCD2         0xC2
#define VID_TP_SCD_CURRENT0 0xCD
#define VID_TP_SCD_CURRENT1 0xCE
#define VID_TP_SCD_CURRENT2 0xCF
#define VID_TP_SCD_RD1      0x81
#define VID_TP_SCD_RD2      0x82
#define VID_TP_VLD1         0xD4
#define VID_TP_VLD2         0xD5
#define VID_TP_VLD_RD0      0xC3
#define VID_TP_VLD_RD1      0xC4
#define VID_TP_VLD_RD2      0xC5
    U8 Value1, Value2, Value3;
    U32 Value;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* LDP */
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "VID_TP_LDP = 0x%x ", HALTRICK_Read(VID_TP_LDP) & 0x07));

    /* STL */
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "VID_STL = 0x%x ", HALTRICK_Read(VID_STL)));

    /* CD */
    Value1 = HALTRICK_Read(VID_TP_CD0);
    Value2 = HALTRICK_Read(VID_TP_CD1);
    Value3 = HALTRICK_Read(VID_TP_CD2);
    Value = (((Value1 & 0x01) << 16) | (Value2 << 8) | (Value3)) * 64;
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "VID_TP_CD = 0x%x ", Value));

    /* CD_RD */
    Value1 = HALTRICK_Read(VID_TP_CD_RD0);
    Value2 = HALTRICK_Read(VID_TP_CD_RD1);
    Value3 = HALTRICK_Read(VID_TP_CD_RD2);
    Value = (((Value1 & 0x0F) << 16) | (Value2 << 8) | (Value3)) * 8;
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "VID_TP_CD_RD = 0x%x ", Value));

    /* CDLimit */
    Value1 = HALTRICK_Read(VID_TP_CDLIMIT0);
    Value2 = HALTRICK_Read(VID_TP_CDLIMIT1);
    Value3 = HALTRICK_Read(VID_TP_CDLIMIT2);
    Value = (((Value1 & 0x01) << 16) | (Value2 << 8) | (Value3)) * 64;
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "VID_TP_CDLIMIT = 0x%x ", Value));

    /* SCD */
    Value1 = HALTRICK_Read(VID_TP_SCD0);
    Value2 = HALTRICK_Read(VID_TP_SCD1);
    Value3 = HALTRICK_Read(VID_TP_SCD2);
    Value = (((Value1 & 0x01) << 16) | (Value2 << 8) | (Value3)) * 64;
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "VID_TP_SCD = 0x%x ", Value));

    /* SCD_CUR */
    Value1 = HALTRICK_Read(VID_TP_SCD_CURRENT0);
    Value2 = HALTRICK_Read(VID_TP_SCD_CURRENT1);
    Value3 = HALTRICK_Read(VID_TP_SCD_CURRENT2);
    Value = (((Value1 & 0x0F) << 16) | (Value2 << 8) | (Value3)) * 8;
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "VID_TP_SCD_CURRENT = 0x%x ", Value));

    /* SCD_RD */
    Value1 = HALTRICK_Read(VID_TP_SCD_RD1);
    Value2 = HALTRICK_Read(VID_TP_SCD_RD2);
    Value = ((Value1 << 8) | Value2 ) * 128;
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "VID_TP_SCD_RD =0x%x  ", Value));

    /* VLD */
    Value1 = HALTRICK_Read(VID_TP_VLD1);
    Value2 = HALTRICK_Read(VID_TP_VLD2);
    Value = ((Value1 << 8) | Value2 ) * 128;
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "VID_TP_VLD = 0x%x ", Value));

    /* VLD_RD */
    Value1 = HALTRICK_Read(VID_TP_VLD_RD0);
    Value2 = HALTRICK_Read(VID_TP_VLD_RD1);
    Value3 = HALTRICK_Read(VID_TP_VLD_RD2);
    Value = (((Value1 & 0x0F) << 16) | (Value2 << 8) | (Value3)) * 8;
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "VID_TP_VLD_RD = 0x%x  ", Value));


    return(ErrorCode);
} /* End of GetStartCodeList() function */

/*******************************************************************************
Name        : SetStartCodeDetector
Description : Set the Start Code detector with the address to scan
Parameters  :
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if no problem
              ST_ERROR_NO_MEMORY if allocation failed
*******************************************************************************/
static ST_ErrorCode_t SetStartCodeDetector(const VIDTRICK_BackwardBuffer_t Buffer)
{
#define HALTRICK_Write8(Address, Value)  STSYS_WriteRegMem8((void *) (Address), (Value))
#define HALDEC_Read8(Address)     STSYS_ReadRegMem8((void *) (Address))

#define VID_TP_LDP          0x3F /* CD FIFO Load pointer.                     */
#define VID_ITS             0x3D /* CD FIFO Load pointer.                     */
#define VID_ITS3            0x63 /* CD FIFO Load pointer.                     */
#define VID_TP_LDP_TM       0x02 /* Bit enable Trick Mode.                    */
#define VID_TP_LDP_LDP      0x01 /* Bit enable Trick Mode.                    */
#define VID_TP_LDP_VSB      0x04 /* Bit enable Trick Mode.                    */
#define VID_TP_STL          0x30 /* SCD trick-mode launch.                    */
#define VID_TP_STL_STL      0x01 /* Bit STL of SCD trick-mode launch.         */
#define VID_TP_STL_INR      0x02 /* Bit STL of SCD trick-mode launch.         */
#define VID_TP_CDLIMIT0     0xE0 /* CD FIFO write limit address.              */
#define VID_TP_CDLIMIT1     0xE1 /* CD FIFO write limit address.              */
#define VID_TP_CDLIMIT2     0xE2 /* CD FIFO write limit address.              */
#define VID_TP_SCD0         0xC0 /* CD FIFO pointer load address.             */
#define VID_TP_SCD1         0xC1 /* CD FIFO pointer load address.             */
#define VID_TP_SCD2         0xC2 /* CD FIFO pointer load address.             */
#define VID_TP_SCD_RD1      0x81
#define VID_TP_SCD_RD2      0x82
#define VID_HDF             0x66 /* start header search                     */
#define VID_HDS             0x69 /* start header search                     */
#define VID_HDS_START       0x01 /* start header search                     */
#define TIMEOUT             5

    U32 count;
    U32 Address;
    U8 Value8;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Write the address of the buffer in the pointer of the CDFIFO */
    Address = (U32)((U8*)(Buffer.StartAddress_p) - SDRAM_BASE_ADDRESS) >> 6 ;
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Set SCD to 0x%x", ((U8*)(Buffer.StartAddress_p))));
    HALTRICK_Write8(VID_TP_SCD0, (Address & 0x10000)>>16);
    HALTRICK_Write8(VID_TP_SCD1, (Address & 0xFF00) >>8);
    HALTRICK_Write8(VID_TP_SCD2, (Address & 0xFF));

    /* Set bit STL : reset CD FIFO. */
    HALTRICK_Write8(VID_TP_STL, VID_TP_STL_STL );
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Set bit STL to 1"));

    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Wait for IT SWR"));
    count = 0;
    HALTRICK_Write8(0x3c, 0x20); /* Mask for SWR */
    while (((HALDEC_Read8(VID_ITS) & 0x20) != 0) && (count < TIMEOUT))
    {
        STOS_TaskDelayUs(DELAY_1S);
        count++;
    }
    if (count == TIMEOUT)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "IT SWR not received !!!"));
    }
    else
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "IT SWR received in %d sec ", count));
    }

    HALTRICK_Write8(VID_HDS, VID_HDS_START);
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Set bit HDS to 1"));

    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Wait for IT SCH : "));
    count = 0;
    while ((HALDEC_Read8(VID_ITS3) != 0x01) && (count < TIMEOUT))
    {
        STOS_TaskDelayUs(DELAY_1S);
        count++;
    }
    if (count == TIMEOUT)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "IT SCH not received !!!"));
    }
    else
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "IT SCH received in %d sec ", count));
        Value8 = HALDEC_Read8(VID_HDF);
        Value8 = (Value8<<8) | (HALDEC_Read8(VID_HDF));
    }


    return(ErrorCode);
} /* End of GetStartCodeList() function */

#endif  /* !LINUX */

#ifndef STVID_VIDDEC_PBO_ENABLED /* we have not PBO */
/*-------------------------------------------------------------------------
 * Function : VID_CatchNewPictureDecodedEvt
 *            Be informed of a STVID_NEW_PICTURE_TO_BE_DISPLAYED_EVT (callback function)
 *            Reset a counter
 * Input    : event data
 * Output   : VSyncCount++
 * Return   :
 * ----------------------------------------------------------------------*/
static void VID_CatchNewPictureDecodedEvt ( STEVT_CallReason_t Reason,
                               const ST_DeviceName_t RegistrantName,
                               STEVT_EventConstant_t Event,
                               const void *EventData,
                               const void *SubscriberData_p )
{
   STVID_Handle_t SubscriberHandle;

    UNUSED_PARAMETER(Reason);
    UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(EventData);

    /* Process event data before using them ... */
    if (STVID_PreprocessEvent(RegistrantName, Event, (void *)EventData) != ST_NO_ERROR)
    {
#if defined(ST_OS20) || defined(ST_OSWINCE)
        STTBX_Print(("%s(%d): Preprocess event pb ! 0x%.8x \n", __FILE__, __LINE__, Event));
#else
        STTBX_Print(("%s(): Preprocess event pb ! 0x%.8x \n", __FUNCTION__, Event));
#endif
    }

    SubscriberHandle = (STVID_Handle_t) SubscriberData_p;
    if ((WaitDecodeCompletionCounterVideoHandle!=0) && (WaitDecodeCompletionCounterVideoHandle==SubscriberHandle))
    {
        WaitDecodeCompletionCounter = 0;
    }
}
#endif /* !STVID_VIDDEC_PBO_ENABLED */




/***************************************************/
/*****************  NOT LINUX  *********************/
/***************************************************/
#if !defined ST_OSLINUX

/*******************************************************************************
Name        : AllocateBackwardBuffer
Description : Returns the start address of the allocated buffer.
Parameters  : TrickModeHandle, Buffer_p
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if no problem
              ST_ERROR_NO_MEMORY if allocation failed
*******************************************************************************/
static ST_ErrorCode_t AllocateBackwardBuffer(
                VIDTRICK_BackwardBuffer_t * const           Buffer_p)
{
    STAVMEM_AllocBlockParams_t  AvmemAllocParams;
    STAVMEM_FreeBlockParams_t   AvmemFreeParams;
    STAVMEM_BlockHandle_t       BufferHandle = STAVMEM_INVALID_BLOCK_HANDLE;
    void *                      BufferAddr;
    ST_ErrorCode_t              ErrorCode = ST_NO_ERROR;

    AvmemAllocParams.Alignment = 64 ; /* VID_TP_CD is in unit of 512 bits (= 64 bytes). */
    AvmemAllocParams.PartitionHandle = AvmemPartitionHandle[0];
    AvmemAllocParams.Size = Buffer_p->BufferSize;
    AvmemAllocParams.AllocMode = STAVMEM_ALLOC_MODE_TOP_BOTTOM;
    AvmemAllocParams.NumberOfForbiddenRanges = 0;
    AvmemAllocParams.ForbiddenRangeArray_p = NULL;
    AvmemAllocParams.NumberOfForbiddenBorders = 0;
    AvmemAllocParams.ForbiddenBorderArray_p = NULL;
    ErrorCode = STAVMEM_AllocBlock(&AvmemAllocParams, &BufferHandle);
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error handling */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Cannot allocate buffer !"));
        return(ST_ERROR_NO_MEMORY);
    }
    ErrorCode = STAVMEM_GetBlockAddress(BufferHandle, &BufferAddr);
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error handling */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error getting address of buffer !"));
        /* Try to free block before leaving */
        AvmemFreeParams.PartitionHandle = AvmemPartitionHandle[0];
        STAVMEM_FreeBlock(&AvmemFreeParams, &BufferHandle);
        return(ST_ERROR_NO_MEMORY);
    }

    /* Return param of the buffer */
    Buffer_p->StartAddress_p = BufferAddr;
    Buffer_p->AvmemBlockHandle = BufferHandle;
    return(ST_NO_ERROR);
} /* End of AllocateBackwardBuffer() function */

/*-------------------------------------------------------------------------
 * Function : TRICK_Allocate
 *
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL TRICK_Allocate(STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL    RetErr=FALSE;

    UNUSED_PARAMETER(pars_p);

    BufferA.BufferSize = 5 * 8 * 1024 ;

    if (AllocateBackwardBuffer(&BufferA) != ST_NO_ERROR)
    {
        /* Error handling */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Cannot allocate buffer A!"));
    }
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "BufferA allocated at the address : 0x%x", (U8*)(BufferA.StartAddress_p) - SDRAM_BASE_ADDRESS));

    STTST_AssignInteger(result_sym_p, RetErr, FALSE);

    return(API_EnableError ? RetErr : FALSE );

} /* end of TRICK_Allocate */


/*-------------------------------------------------------------------------
 * Function : TRICK_SetCDFIFO
 *
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL TRICK_SetCDFIFO(STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL    RetErr=FALSE;

    UNUSED_PARAMETER(pars_p);

    SetCDFIFO(BufferA);

    STTST_AssignInteger(result_sym_p, RetErr, FALSE);

    return(API_EnableError ? RetErr : FALSE );

} /* end of TRICK_SetCDFIFO */

/*-------------------------------------------------------------------------
 * Function : TRICK_ParseBuffer
 *
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL TRICK_ParseBuffer(STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL    RetErr=FALSE;

    UNUSED_PARAMETER(pars_p);

    SetStartCodeDetector(BufferA);

    STTST_AssignInteger(result_sym_p, RetErr, FALSE);

    return(API_EnableError ? RetErr : FALSE );

} /* end of TRICK_ParseBuffer */

/*-------------------------------------------------------------------------
 * Function : TRICK_PrintRegisters
 *
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL TRICK_PrintRegisters(STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL    RetErr=FALSE;

    UNUSED_PARAMETER(pars_p);

    ReportRegisters();

    STTST_AssignInteger(result_sym_p, RetErr, FALSE);
    return(API_EnableError ? RetErr : FALSE );

} /* end of TRICK_PrintRegisters */

#endif /*!ST_OSLINUX*/

/*-------------------------------------------------------------------------
 * Function : VID_StillDecode
 *           (injects a file to the video driver in one only loop and stops
 *            the driver with the end_of_data option)
 *
 * Caution  : - Should be used only with injecter based platforms to ensure that
 *            the picture injection is done at once.
 *            - The picture size must be less than the input buffer size to
 *            ensure also that the picture injection is done at once.
 *            - The driver must be started and initialised before calling this
 *            command.
 *            - The driver is stopped at the end of the command call
 *
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_StillDecode( STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    S32   FifoNb, BufferNb1;
    STVID_Handle_t CurrentHandle;
#if !defined ST_OSLINUX
    ST_ErrorCode_t ErrCode;
    STVID_Freeze_t      FreezeParams;
#if defined(ST_5100) || defined(ST_5105) || defined(ST_5107) || defined(ST_5188) || defined(ST_5301) || defined(ST_7100) || defined(ST_7109) || defined(ST_7200) || defined(ST_7710) || defined(ST_5525)
    U32 InputBuffersize;
#endif
#endif /* !ST_OSLINUX */

    RetErr = FALSE;

    RetErr = STTST_GetInteger( pars_p, DefaultVideoHandle, (S32*)&CurrentHandle);
    if ( RetErr )
    {
        STTST_TagCurrentLine(pars_p, "expected video handle");
        return(TRUE);
    }
    DefaultVideoHandle = CurrentHandle ;

    RetErr = STTST_GetInteger( pars_p, 1, &FifoNb);
    if ((RetErr ) || (FifoNb < 1) || (FifoNb > VIDEO_MAX_DEVICE))
    {
        sprintf(VID_Msg, "expected Fifo number (1 to %d)", VIDEO_MAX_DEVICE);
        STTST_TagCurrentLine( pars_p, VID_Msg);
        return(TRUE);
    }

    RetErr = STTST_GetInteger( pars_p, 1, &BufferNb1);
    if (( RetErr ) || (BufferNb1 < 1) || (BufferNb1 > VID_MAX_BUFFER_LOAD))
    {
        sprintf(VID_Msg, "expected load buffer number (default is 1. max is %d)", VID_MAX_BUFFER_LOAD);
        STTST_TagCurrentLine( pars_p, VID_Msg);
        return(TRUE);
    }

    if(VID_BufferLoad[BufferNb1-1].AllocatedBuffer_p == NULL)
    {
        STTBX_Print(( "No file loaded in memory buffer %d !!\n", BufferNb1 ));
        return(TRUE);
    }

#if defined ST_OSLINUX
    /* Jan. 2007: GetWriteAddress(), InformReadAddress(), InjectWithoutDelay() not yet supported */
    STTBX_Print(("VID_StillDecode(): injection not yet supported on Linux !\n"));
    STTST_AssignInteger(result_sym_p, TRUE, FALSE);
    return(API_EnableError ? TRUE : FALSE );
#else

    /* Lock injection access */
    semaphore_wait(VID_Injection[FifoNb-1].Access_p);

    if(VID_Injection[FifoNb-1].BitBufferSize == 0)
    {
        /* No injection occured. Bit buffer should be empty */
        ErrCode = STVID_GetBitBufferFreeSize(CurrentHandle, &VID_Injection[FifoNb-1].BitBufferSize);
        if(ErrCode != ST_NO_ERROR)
        {
            VID_Injection[FifoNb-1].BitBufferSize = 0;
            STTBX_Print(("VID_StillDecode(): STVID_GetBitBufferFreeSize() failure, error=0x%x !\n", ErrCode));
            RetErr = TRUE;
        }
    }
    if (!(RetErr))
    {
        if(VID_Injection[FifoNb-1].Type == NO_INJECTION)
        {
            VID_Injection[FifoNb-1].Config.Memory.SrcBuffer_p = VID_BufferLoad[BufferNb1-1].AlignedBuffer_p;
            VID_Injection[FifoNb-1].Config.Memory.CurrentSrcPtr_p = VID_BufferLoad[BufferNb1-1].AlignedBuffer_p;
            VID_Injection[FifoNb-1].Config.Memory.SrcSize = VID_BufferLoad[BufferNb1-1].NbBytes;
            VID_Injection[FifoNb-1].Config.Memory.LoadBufferNb = BufferNb1;
            VID_Injection[FifoNb-1].Config.Memory.LoopNbr = 1;
            VID_Injection[FifoNb-1].Config.Memory.LoopNbrDone = 0;
            VID_Injection[FifoNb-1].Config.Memory.SynchronizedByUnderflowEvent = FALSE;
            VID_Injection[FifoNb-1].Driver.Handle = CurrentHandle;
#if defined(ST_5100) || defined(ST_5105) || defined(ST_5107) || defined(ST_5188) || defined(ST_5301) || defined(ST_7100) || defined(ST_7109) || defined(ST_7200) || defined(ST_7710) || defined(ST_5525)
            /* Get Input buffer characteristics     */
            ErrCode = STVID_GetDataInputBufferParams(VID_Injection[FifoNb-1].Driver.Handle,
                &VID_Injection[FifoNb-1].Base_p, &InputBuffersize);
            /* Update injection pointer.            */
            VID_Injection[FifoNb-1].Top_p   =
                    (void *)((U32)VID_Injection[FifoNb-1].Base_p + InputBuffersize - 1);
            /* Align the write and read pointers to the beginning of the input buffer */
            VID_Injection[FifoNb-1].Write_p = VID_Injection[FifoNb-1].Base_p;
            VID_Injection[FifoNb-1].Read_p  = VID_Injection[FifoNb-1].Base_p;
            /* Configure the interface-object       */
            STVID_SetDataInputInterface(
                                    VID_Injection[FifoNb-1].Driver.Handle,
                                    GetWriteAddress,InformReadAddress,(void*)(FifoNb-1));
#endif /* defined(ST_5100) || defined(ST_5105) || defined(ST_5107) || defined(ST_5188) || defined(ST_5301) || defined(ST_7100) || defined(ST_7109) || defined(ST_7710)|| defined(ST_5525) */
            VID_Injection[FifoNb-1].Type = VID_MEMORY_INJECTION;
            /* Video Load Buffer in use */
            VID_BufferLoad[BufferNb1-1].UsedCounter++;
            /* Free injection access */
            semaphore_signal(VID_Injection[FifoNb-1].Access_p);

            FreezeParams.Mode  = STVID_FREEZE_MODE_FORCE;
            FreezeParams.Field = STVID_FREEZE_FIELD_TOP;

            RetErr = InjectWithoutDelays(FifoNb);

            ErrCode = STVID_Stop(CurrentHandle, STVID_STOP_WHEN_END_OF_DATA, &FreezeParams);
            if(ErrCode != ST_NO_ERROR)
            {
                STTBX_Print(("VID_StillDecode(): STVID_Stop() failure, error=0x%x ! \r\n", ErrCode));
                return(API_EnableError ? TRUE : FALSE);
            }
        }
        else
        {
            VID_ReportInjectionAlreadyOccurs(FifoNb);
        }
    }

    STTST_AssignInteger(result_sym_p, FALSE, FALSE);
    return(API_EnableError ? RetErr : FALSE );
#endif /*ST_OSLINUX*/

} /* end of VID_StillDecode() */

#if !defined ST_OSLINUX

/*-------------------------------------------------------------------------
 * Function : InjectWithoutDelays (tries to inject a file to the video driver
 *            in one only injection
 *
 * Caution  : Should be used only with injecter based platforms to ensure that
 *            the picture injection is done at once. Also, the picture size
 *            must be less than the input buffer size to ensure also that the
 *            picture injection is done at once.
 *
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL InjectWithoutDelays(U32 FifoNb)
{
    U32 Loops=0;

#if defined(ST_7100) || defined(ST_7109) || defined(ST_7200) || defined(ST_5100) || defined(ST_5105) || defined(ST_5107) || defined(ST_5188) || defined(ST_5301) || defined(ST_7710)|| defined(ST_5525)
    void* Read_p;
    void* Write_p;
    U32 CopiedNB = 0, InputBufferFreeSize;

    /* Lock injection access */
    semaphore_wait(VID_Injection[FifoNb-1].Access_p);
    while(CopiedNB != VID_Injection[FifoNb-1].Config.Memory.SrcSize)
    {
        Loops++;
        /* As the Read pointer can be changed by the driver, take a copy at the beginning and work with it */
        Read_p = VID_Injection[FifoNb-1].Read_p;
        Write_p= VID_Injection[FifoNb-1].Write_p;
        if(Read_p > Write_p)
        {
            InputBufferFreeSize = (U32)Read_p
                                - (U32)Write_p;
        }
        else
        {
            InputBufferFreeSize = (U32)VID_Injection[FifoNb-1].Top_p
                                - (U32)Write_p + 1
                                + (U32)Read_p
                                - (U32)VID_Injection[FifoNb-1].Base_p;
        }
        /* Leave always 1 empty byte to avoid Read=Write */
        if(InputBufferFreeSize != 0)
        {
            InputBufferFreeSize-=1;
        }
        if(CopiedNB + InputBufferFreeSize >= VID_Injection[FifoNb-1].Config.Memory.SrcSize)
        {
            InputBufferFreeSize = VID_Injection[FifoNb-1].Config.Memory.SrcSize-CopiedNB;
        }
        /* Input buffer is full ? So, wait for one transfer period */
        if(InputBufferFreeSize == 0)
        {
            /* calculate the periode for transfers (constant) */
            STOS_TaskDelayUs(MS_TO_NEXT_TRANSFER * 1000); /* waits MS_TO_NEXT_TRANSFER ms */
            continue;
        }
        if((U32)Write_p + InputBufferFreeSize > (U32)VID_Injection[FifoNb-1].Top_p + 1)
        {
            memcpy(/* dest */ Write_p,
                   /* src  */ (void*)((U32)VID_Injection[FifoNb-1].Config.Memory.CurrentSrcPtr_p + CopiedNB),
                   /* size */ (U32)VID_Injection[FifoNb-1].Top_p - (U32)Write_p + 1);
            InputBufferFreeSize -= (U32)VID_Injection[FifoNb-1].Top_p - (U32)Write_p + 1;
            CopiedNB += (U32)VID_Injection[FifoNb-1].Top_p - (U32)Write_p + 1;
            Write_p = VID_Injection[FifoNb-1].Base_p;
        }
        memcpy(/* dest */ Write_p,
               /* src  */ (void*)((U32)VID_Injection[FifoNb-1].Config.Memory.CurrentSrcPtr_p + CopiedNB),
               /* size */ InputBufferFreeSize);
        Write_p = (void*)((U32)Write_p + InputBufferFreeSize);
        if(Write_p == (void*)((U32)VID_Injection[FifoNb-1].Top_p + 1))
        {
            Write_p = VID_Injection[FifoNb-1].Base_p;
        }
        VID_Injection[FifoNb-1].Write_p = Write_p;
        CopiedNB += InputBufferFreeSize;
    }
#else /* ! (defined(ST_7100) || defined(ST_7109) || defined(ST_7200) || defined(ST_5100) || defined(ST_5105) || defined(ST_5107) || defined(ST_5188) || defined(ST_5301) || defined(ST_7710)|| defined(ST_5525)) */
    U32 i;

    for (i=0; i< (VID_Injection[FifoNb-1].Config.Memory.SrcSize/4) ; i++)
    {
        Loops++;
        STSYS_WriteRegDev32LE(VID_Injection[FifoNb-1].Address,
                              *(((U32 *)VID_Injection[FifoNb-1].Config.Memory.CurrentSrcPtr_p) + i));
    }
#endif /* defined(ST_7100) || defined(ST_7109) || defined(ST_7200) || defined(ST_5100) || defined(ST_5105) || defined(ST_5107) || defined(ST_5188) || defined(ST_5301) || defined(ST_7710) || defined(ST_5525)*/
    VID_BufferLoad[VID_Injection[FifoNb-1].Config.Memory.LoadBufferNb-1].UsedCounter--;
    VID_Injection[FifoNb-1].Type = NO_INJECTION;
    /* Lock injection access */
    semaphore_signal(VID_Injection[FifoNb-1].Access_p);

    if(Loops > 1)
    {
        STTBX_Print(("Warning : the injection wasn't done in one only loop !\n"));
        /* The injection must be done at once to be sure that no delay was introduced */
        return(TRUE);
    }
    else
    {
        return(FALSE);
    }
} /* end of InjectWithoutDelays() */
#endif /* ! ST_OSLINUX */


/*#######################################################################*/
/*#########################  GLOBAL FUNCTIONS  ##########################*/
/*#######################################################################*/

/*-------------------------------------------------------------------------
 * Function : VID_Inj_GetDeviceName
 *            Return the device name
 * Input    : index, device name
 * Output   :
 * Return   : TRUE if ok, FALSE if error
  * ----------------------------------------------------------------------*/
BOOL VID_Inj_GetDeviceName (U32 index, char *name)
{
    if ( index<VIDEO_MAX_DEVICE )
    {
        if ( name != NULL )
        {
            strcpy(name, VID_Injection[index].Driver.DeviceName);
        }
        return(TRUE);
    }
    return(FALSE);
}

/*-------------------------------------------------------------------------
 * Function : VID_Inj_GetDriverHandle
 *            Returns the driver handle value
 * Input    : index
 * Output   :
 * Return   : handle value, 0 if error
 * ----------------------------------------------------------------------*/
STVID_Handle_t VID_Inj_GetDriverHandle (U32 index)
{
    if ( index<VIDEO_MAX_DEVICE )
    {
        return(VID_Injection[index].Driver.Handle);
    }
    return(0);
}

/*-------------------------------------------------------------------------
 * Function : VID_Inj_GetInjectionType
 *            Returns the handle value
 * Input    : index
 * Output   :
 * Return   : type value, -1 if error
 * ----------------------------------------------------------------------*/
S16 VID_Inj_GetInjectionType (U32 index)
{
    if ( index<VIDEO_MAX_DEVICE )
    {
        return(VID_Injection[index].Type);
    }
    return(-1);
}

/*-------------------------------------------------------------------------
 * Function : VID_Inj_GetVideoDriverState
 *            Set the video driver state
 * Input    : index, state
 * Output   :
 * Return   : state if ok, -1 if error
 * ----------------------------------------------------------------------*/
S16 VID_Inj_GetVideoDriverState (U32 index)
{
    if ( index<VIDEO_MAX_DEVICE )
    {
        return(VID_Injection[index].Driver.DriverState);
    }
    return(-1);
}

/*-------------------------------------------------------------------------
 * Function : VID_Inj_IsDeviceNameUndefined
 *            Check if device name is undefined
 * Input    : index
 * Output   :
 * Return   : TRUE if ok, FALSE if error
 * ----------------------------------------------------------------------*/
BOOL VID_Inj_IsDeviceNameUndefined (U32 index)
{
    if ( index<VIDEO_MAX_DEVICE )
    {
        if (strlen(VID_Injection[index].Driver.DeviceName)==0)
        {
            return(TRUE);
        }
    }
    return(FALSE);
}

/*-------------------------------------------------------------------------
 * Function : VID_Inj_IsDeviceNameIdentical
 *            Check if device name is identical to the given device name
 * Input    : index, device name
 * Output   :
 * Return   : TRUE if undefined, FALSE if defined or error
 * ----------------------------------------------------------------------*/
BOOL VID_Inj_IsDeviceNameIdentical (U32 index, char *name)
{
    if ( index<VIDEO_MAX_DEVICE )
    {
        if (strcmp(VID_Injection[index].Driver.DeviceName,name)==0)
        {
            return(TRUE);
        }
    }
    return(FALSE);
}

/*-------------------------------------------------------------------------
 * Function : VID_Inj_SetHandle
 *            Set the handle value
 * Input    : index
 * Output   :
 * Return   : TRUE if ok, FALSE if error
 * ----------------------------------------------------------------------*/
BOOL VID_Inj_SetDriverHandle (U32 index, STVID_Handle_t value)
{
    if ( index<VIDEO_MAX_DEVICE )
    {
        VID_Injection[index].Driver.Handle = value;
        return(TRUE);
    }
    return(FALSE);
}

/*-------------------------------------------------------------------------
 * Function : VID_Inj_SetDeviceName
 *            Set the device name
 * Input    : index, device name
 * Output   :
 * Return   : TRUE if ok, FALSE if error
  * ----------------------------------------------------------------------*/
BOOL VID_Inj_SetDeviceName (U32 index, char *name)
{
    if ( index<VIDEO_MAX_DEVICE )
    {
        strcpy(VID_Injection[index].Driver.DeviceName, name);
        return(TRUE);
    }
    return(FALSE);
}

/*-------------------------------------------------------------------------
 * Function : VID_Inj_SetInjectionType
 *            Set injection type
 * Input    : index
 * Output   :
 * Return   : TRUE if ok, FALSE if error
 * ----------------------------------------------------------------------*/
BOOL VID_Inj_SetInjectionType (U32 index, VID_InjectionType_e type)
{
    if ( index<VIDEO_MAX_DEVICE )
    {
        VID_Injection[index].Type = type;
        return(TRUE);
    }
    return(FALSE);
}

/*-------------------------------------------------------------------------
 * Function : VID_Inj_SetVideoDriverState
 *            Set the video driver state
 * Input    : index, state
 * Output   :
 * Return   : TRUE if ok, FALSE if error
 * ----------------------------------------------------------------------*/
BOOL VID_Inj_SetVideoDriverState (U32 index, VID_DriverState_e state)
{
    if ( index<VIDEO_MAX_DEVICE )
    {
        VID_Injection[index].Driver.DriverState = state;
        return(TRUE);
    }
    return(FALSE);
}

/*-------------------------------------------------------------------------
 * Function : VID_Inj_SignalAccess
 *            Wait access is allow to VID_Injection[]
 * Input    : index, device name
 * Output   :
 * Return   : TRUE if ok, FALSE if error
  * ----------------------------------------------------------------------*/
void VID_Inj_SignalAccess (U32 index)
{
    if ( index<VIDEO_MAX_DEVICE )
    {
        semaphore_signal(VID_Injection[index].Access_p);
    }
}

/*-------------------------------------------------------------------------
 * Function : VID_Inj_ResetBitBufferSize
 *            Reset device name and handle
 * Input    : index
 * Output   :
 * Return   : TRUE if ok, FALSE if error
 * ----------------------------------------------------------------------*/
BOOL VID_Inj_ResetBitBufferSize (U32 index)
{
    if ( index<VIDEO_MAX_DEVICE )
    {
        VID_Injection[index].BitBufferSize = 0;
        return(1);
    }
    return(0);
}

/*-------------------------------------------------------------------------
 * Function : VID_Inj_ResetDeviceAndHandle
 *            Reset the device name and handle
 * Input    : index
 * Output   :
 * Return   : TRUE if ok, FALSE if error
 * ----------------------------------------------------------------------*/
BOOL VID_Inj_ResetDeviceAndHandle (U32 index)
{
    if ( index<VIDEO_MAX_DEVICE )
    {
        strcpy(VID_Injection[index].Driver.DeviceName, "");
        VID_Injection[index].Driver.Handle = 0;
        return(1);
    }
    return(0);
}

/*-------------------------------------------------------------------------
 * Function : VID_Inj_WaitAccess
 *            Wait access is allow to VID_Injection[]
 * Input    : index, device name
 * Output   :
 * Return   : TRUE if ok, FALSE if error
  * ----------------------------------------------------------------------*/
void VID_Inj_WaitAccess (U32 index)
{
    if ( index<VIDEO_MAX_DEVICE )
    {
        semaphore_wait(VID_Injection[index].Access_p);
    }
}


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

    /* --- Set PBO at application level --- */

#if defined(STVID_VIDDEC_PBO_ENABLED)
    /* Work with PBO enabled */
#if defined (ST_5514) || defined (ST_5516) || defined (ST_5517) || defined (ST_5508) || defined (ST_5518)
    STSYS_WriteRegDev8(0x7b, 0x00);
    STSYS_WriteRegDev8(0x1, STSYS_ReadRegDev8(0x1) | 0x20);
    STSYS_WriteRegDev8(0x7b, 0x01);
#elif defined (ST_5510) || defined (ST_5512)
    STSYS_WriteRegDev8(0x1, STSYS_ReadRegDev8(0x1) | 0x20);
#endif /* ST_5510 || ST_5512 */
#else
    /* Work with PBO disabled, memory injection are monitored by bit buffer level polling */
    /* PBO must be enabled with back buffer */
#if !defined(ST_7015) && !defined(ST_7020) && !defined(ST_5528) && !defined(ST_5100) && !defined(ST_5105) && !defined(ST_5107)\
         && !defined(ST_5188) && !defined(ST_5301) && !defined(ST_7100) && !defined(ST_7109) && !defined(ST_7200) && !defined(ST_7710) && !defined ST_5525

    if(STSYS_ReadRegDev8(0x1) & 0x20)
    {
        STTBX_Print(("\nWARNING : PBO is enabled and disabled was expected since STBOOT rel 2.9.0 & STBOARD rel 2.3.0 !!!\n"));
        STTBX_Print(("\nReseting PBO to disabled configuration\n"));
#if defined (ST_5514) || defined (ST_5516) || defined (ST_5517) || defined (ST_5508) || defined (ST_5518)
        STSYS_WriteRegDev8(0x7b, 0x00);
        STSYS_WriteRegDev8(0x1, STSYS_ReadRegDev8(0x1) & ~0x20);
        STSYS_WriteRegDev8(0x7b, 0x01);
#elif defined (ST_5510) || defined (ST_5512)
        STSYS_WriteRegDev8(0x1, STSYS_ReadRegDev8(0x1) & ~0x20);
#endif /* ST_5510 || ST_5512 */
    }
#endif /* !defined(ST_7015) && !defined(ST_7020) */
#endif /* #else of #if defined(STVID_VIDDEC_PBO_ENABLED) */

    /* --- Define the register-commands (in alphabetic order) --- */

    RetErr = FALSE;

#if !defined ST_OSLINUX
    RetErr |= STTST_RegisterCommand( "VID_CDFIFOSTAT", VID_CDFifoStatus ,
            "CD fifo status of the chip");
    RetErr |= STTST_RegisterCommand( "VID_FLUSH",    VIDFlushVideoCDFIFO,
            "Flush video CD-FIFO");
    RetErr |= STTST_RegisterCommand( "VID_ERRINJECT", VID_MemErrorsInject,
            "<loop(-1=infinite)><Handle><CDFifoNb><Load buffer nb> DMA Inject Video+Errors in memory (parallel task)");
#endif

    RetErr |= STTST_RegisterCommand( "VID_DECODE",   VIDDecodeFromMemory,
            "<Handle><Cdfifo><Load buffer nb><# of loops> Copy buffer into video cdfifo");
    RetErr |= STTST_RegisterCommand( "VID_INJECT",   VID_MemInject,
            "<loop(-1=infinite)><Handle><CDFifoNb><Load buffer nb> DMA Inject Video in memory (parallel task)");
    RetErr |= STTST_RegisterCommand( "VID_KILL",     VID_KillTask,
            "<CDFifoNb><WaitEnd> Kill Video Inject Task for CD Fifo\n\t\t WaitEnd=1 wait end of current injection loop, 0 kill now (default)");

#if !defined ST_OSLINUX
    RetErr |= STTST_RegisterCommand( "VID_SETINJECTLEVEL",   VID_SetInjectLevel,
            "<Value> Set max bit buffer level for data injection tasks");
    RetErr |= STTST_RegisterCommand( "TRICK_ALLOCATE",TRICK_Allocate,
            "Allocate Backward BufferA");
    RetErr |= STTST_RegisterCommand( "TRICK_SETCDFIFO",TRICK_SetCDFIFO,
            "Set BufferA");
    RetErr |= STTST_RegisterCommand( "TRICK_ParseBuffer",TRICK_ParseBuffer,
            "Parse BufferA");
    RetErr |= STTST_RegisterCommand( "TRICK_PrintRegisters",TRICK_PrintRegisters,
            "Report registers");
#endif  /* !LINUX */
    RetErr |= STTST_RegisterCommand( "VID_STILL", VID_StillDecode, "VID_STILLDEC <Handle><CDFifoNb><Load buffer nb1><Load buffer nb2>");

    /* Init for injection process */
#if !defined ST_OSLINUX
    VidMemInjection_p = STOS_SemaphoreCreateFifo(DriverPartition_p,0);
    VID_PtiAvailable_p = STOS_SemaphoreCreateFifo(DriverPartition_p,1);
#ifdef USE_CRYPTOCORE
	crypto.isAvailable = false;
#endif /* USE_CRYPTOCORE */
#ifdef BENCHMARK_WINCESTAPI
	P_ADDSEMAPHORE(VidMemInjection_p, "VidMemInjection_p");
	P_ADDSEMAPHORE(VID_PtiAvailable_p, "VID_PtiAvailable_p");
#endif

#endif

    /* --- Data initialisation for injection management --- */

    memset(&VID_Injection, 0, (VIDEO_MAX_DEVICE * sizeof(VID_Injection_t)));
    VIDTid = NULL ;

    for (i=0;i<VIDEO_MAX_DEVICE;i++)
    {
        VID_Injection[i].Access_p = STOS_SemaphoreCreateFifo(DriverPartition_p,1);
#ifdef BENCHMARK_WINCESTAPI
	P_ADDSEMAPHORE(VID_Injection[i].Access_p, "VID_Injection[i].Access_p");
#endif
        /* Common default values */
        VID_Injection[i].Type = NO_INJECTION;
        VID_Injection[i].State = STATE_INJECTION_NORMAL;
#if !defined ST_5100 && !defined ST_5105 && !defined ST_5107 && !defined ST_5188 && !defined ST_5301 && !defined ST_7100 && !defined ST_7109 && !defined ST_7200 && !defined ST_7710 && !defined ST_5525
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

    /* Init fields for BufferLoad */
    for (i=0;i<VID_MAX_BUFFER_LOAD;i++)
    {
        VID_BufferLoad[i].AllocatedBuffer_p = NULL;
        VID_BufferLoad[i].UsedCounter = 0;
    }

    /* Print result */
    if ( !RetErr )
    {
        sprintf( VID_Msg, "VID_Inj_RegisterCmd() \t: ok\n");
    }
    else
    {
        sprintf( VID_Msg, "VID_Inj_RegisterCmd() \t: failed !\n" );
    }
    STTST_Print(( VID_Msg ));

    return(!RetErr);

} /* end VID_Inj_RegisterCmd */



/*#######################################################################*/

