/*******************************************************************************

File name   : inject.c

Description : Control PES/ES->ES+SC List injection

COPYRIGHT (C) STMicroelectronics 2006

Date               Modification                                      Name
----               ------------                                      ----
 May 2003          Created                                           Michel Bruant
 06 Aug 2004       Cleaned: remove globals, added Init()/Term(), etc HG
 13 Jun 2006       Added VIDINJ_ReallocateFDMANodes                  DG

 Exported functions:
 -------------------
-------- Initialisation-Termination
 VIDINJ_Init
 VIDINJ_Term
 VIDINJ_TransferGetInject
 VIDINJ_TransferReleaseInject
-------- protections
 VIDINJ_EnterCriticalSection
 VIDINJ_LeaveCriticalSection
-------- protected - called at Init()
 VIDINJ_ReallocateFDMANodes - called following STVID_Setup(STVID_SETUP_FDMA_NODES_PARTITION)
 VIDINJ_Transfer_SetDataInputInterface
-------- protected - called at Init() or when changing BB
 VIDINJ_TransferLimits
-------- protected - called at Start() or Reset()
 VIDINJ_TransferReset  ->  ResetInjecter() + set default ranges to MPEG ones
 VIDINJ_SetSCRanges (not called for MPEG)
 VIDINJ_TransferStart
-------- protected - others
 VIDINJ_InjectStartCode
 VIDINJ_TransferFlush
 VIDINJ_TransferStop
-------- not protected
 VIDINJ_TransferSetESRead
 VIDINJ_TransferSetESCopyBufferRead
 VIDINJ_TransferSetSCListRead

*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */

/* The following flag will liberate the injecter context after each FDMA
 * transfer. This will allow more access to this context from the parser task */
/* #define VIDINJ_LIBERATE_CONTEXT_AFTER_EACH_FDMA_TRANSFER */

/* Includes ----------------------------------------------------------------- */

#if !defined(ST_OSLINUX)
#include <stdio.h>
#include <string.h>
#endif

#include "stddefs.h"
#include "stos.h"

#include "stsys.h"
#include "sttbx.h"

#include "inject.h"
#include "stfdma.h"
#include "vid_com.h"
#include "vid_ctcm.h"
#include "stos.h"

/* Trace System activation ---------------------------------------------- */

/* Should be removed for official releases. */
/* Enable uart traces...        */
/*#define TRACE_INJECT*/
/*#define TRACE_FLUSH*/

/* Enable this if you want to see the Traces... */
/*#define TRACE_UART*/

/* To be removed after code cleanup */
/*#define WINCE_TRACE_ENABLED*/
/*#define CHECK_FDMA_INJECT_RESULT*/

#ifdef WINCE_TRACE_ENABLED
static int WinceTraceEnabled = 1; /* enable/disable traces on the fly */
#define WINCE_TRACE(x) do { if(WinceTraceEnabled) printf x;} while(0)
#endif

/* Select the Traces you want */
#define TrWrap                  TraceOff
#define TrBuffers               TraceOff
#define TrESCopy                TraceOn
#define TrESBuffer              TraceOff
#define TrSCList                TraceOff
#define TrSCListWrite           TraceOff
#define TrPESBuffer             TraceOff
#define TrWarning               TraceOff
/* TrError is now defined in vid_trace.h */
#ifdef CHECK_FDMA_INJECT_RESULT
#define TrFDMAError             TraceOn
#define TrFDMA                  TraceOn
#endif


/* "vid_trace.h" and "trace.h" should be included AFTER the definition of the traces wanted */
#if defined TRACE_UART || defined TRACE_INJECT || defined TRACE_FLUSH
    #include "..\..\tests\src\trace.h"
#endif
#include "vid_trace.h"

/* Constants ---------------------------------------------------------------- */

#define VIDINJ_VALID_HANDLE 0x01c51c50

#if !defined(STFDMA_USE_VIRTUAL_CONTEXT)
/* Standard FDMA usage: one FDMA channel per injecter */
#define MAX_INJECTERS           3
#else /* STFDMA_USE_VIRTUAL_CONTEXT */
/* Virtual Context allows to multiplex several injections through a single */
/* FDMA channel: this allows to have more than the 3 built-in FDMA parsing   */
/* channels.                                                                  */
#define MAX_INJECTERS           16
#endif /* STFDMA_USE_VIRTUAL_CONTEXT */

#define SCLIST_MIN_SIZE         (400 * sizeof(STFDMA_SCEntry_t))

#define FDMA_PESSCD_NODE_ALIGNMENT          32
#define FDMA_MEMORY_BUFFER_ALIGNMENT        128

/* Definition of start code values */
#define PICTURE_START_CODE               0x00
#define FIRST_SLICE_START_CODE           0x01
#define GREATEST_SLICE_START_CODE        0xAF
#define RESERVED_START_CODE_0            0xB0
#define SMALLEST_SYSTEM_START_CODE       0xB9
#define SMALLEST_AUDIO_PACKET_START_CODE 0xC0
#define GREATEST_AUDIO_PACKET_START_CODE 0xDF
#define SMALLEST_VIDEO_PACKET_START_CODE 0xE0
#define GREATEST_VIDEO_PACKET_START_CODE 0xEF
#define GREATEST_SYSTEM_START_CODE       0xFF

#define VIDINJ_INVALID_ADDRESS           ((void *) 0)


/* ------------------------------------------------------------------------------------- */
/* In order to save overall system bandwidth, the time between two transfer is optimized */
/* depending on the chip used and the decode mode (real time or not).					*/
/* ------------------------------------------------------------------------------------- */
/* 		  Mode						time between two tranfers (in ms)					*/
/*     real time 			MAX_LOOPS_FOR_REALTIME_INJECTION x MS_TO_NEXT_TRANSFER		*/
/*   non real time					   MS_TO_NEXT_TRANSFER								*/
/* ------------------------------------------------------------------------------------- */
/* Define the number of MS_TO_NEXT_TRANSFER milli-seconds to wait until next transfer.  */
#if defined(ST_51xx_Device)
#define MS_TO_NEXT_TRANSFER_FOR_REALTIME_INJECTION  16  /* So one transfer every 16ms*/
#define MS_TO_NEXT_TRANSFER_FOR_HDD_INJECTION       3   /* So one transfer every 3ms*/
#else
#define MS_TO_NEXT_TRANSFER_FOR_REALTIME_INJECTION  9   /* So one transfer every 9ms*/
#define MS_TO_NEXT_TRANSFER_FOR_HDD_INJECTION       3   /* So one transfer every 3ms*/
#endif
/* Same values but now in ticks */
#define TICKS_TO_NEXT_TRANSFER_FOR_REALTIME_INJECTION    ((ST_GetClocksPerSecond() * MS_TO_NEXT_TRANSFER_FOR_REALTIME_INJECTION) / 1000)
#define TICKS_TO_NEXT_TRANSFER_FOR_HDD_INJECTION         ((ST_GetClocksPerSecond() * MS_TO_NEXT_TRANSFER_FOR_HDD_INJECTION) / 1000)

#define FLUSH_DATA_SIZE              (128*3)     /* Size of the extra data injected when a flush is asked */

#if defined(DVD_SECURED_CHIP)
/* Value of the max number of slice data that could be transferred into the ES Copy buffer */
/* This value corresponds to the max possible length according to each codec */
    #define MAX_MPEG_SLICE_TRANSFER     0
    #define MAX_VC1_SLICE_TRANSFER      54   /* Theoreticla max = 54 bytes */
    #define MAX_H264_SLICE_TRANSFER     100  /* Theoretical max = 1355 bytes */
#endif /* DVD_SECURED_CHIP */

/* Structures --------------------------------------------------------------- */

typedef struct
{
    /* Input */
    void *                      InputBase_p;
    void *                      InputTop_p;     /* TotalSize = Top_p - Base_p + 1 */
    void *                      InputWrite_p;   /* Address where next byte is going to be written (already written until ((Write_p - 1) % circular) */
    void *                      InputRead_p;    /* Address where 1st byte of
                                                 * next transfer will be read */
    /*  (Write_p == Read_p) buffer empty    means occupied = 0, free = (Top_p - Base_p)             */
    /* ((Write_p == Read_p + 1) % circular) means occupied = 1, free = (Top_p - Base_p - 1)         */
    /*   ...                                                                                        */
    /* ((Write_p == Read_p + n) % circular) means occupied = n, free = (Top_p - Base_p - n)         */
    /*   ...                                                                                        */
    /* ((Write_p == Read_p + TotalSize - 1) % circular) means occupied = TotalSize - 1, free = 0    */
    /* Buffer can contain only (TotalSize - 1) bytes maximum !                                      */
    /* ES output */
    void *                      ES_Base_p;
    void *                      ES_Top_p;       /* TotalSize = Top_p - Base_p + 1 */
    void *                      ES_Write_p;     /* Address where 1st byte of next transfer will be written */
    void *                      ES_Read_p;      /* Address where next byte is going to be read (already read until ((Read_p - 1) % circular) */
    /*  (Write_p == Read_p) buffer empty    means occupied = 0, free = (Top_p - Base_p) */
    /* ((Write_p == Read_p + 1) % cirular)  means occupied = 1, free = (Top_p - Base_p - 1) */
    /*  ...  */
    /* ((Write_p == Read_p + n) % cirular)  means occupied = n, free = (Top_p - Base_p - n) */
    /*  ...  */
    /* ((Write_p == Read_p + TotalSize - 1) % cirular) means occupied = TotalSize - 1, free = 0 */
    /* Buffer can contain only (TotalSize - 1) bytes maximum ! */

#if defined(DVD_SECURED_CHIP)
    /* Header Copy buffer output */
    void *                      ESCopy_Base_p;
    void *                      ESCopy_Top_p;
    void *                      ESCopy_Write_p;
    void *                      ESCopy_Read_p;
#endif /* DVD_SECURED_CHIP */

    /* SC-List output */
    void *                      SCListBase_p;
    void *                      SCListTop_p;
    void *                      SCListLoop_p;   /* As the SCLIST buffer is not circular, this pointer holds the address of the entry
                                                   of the buffer where the write pointer looped to write from the base. */
    void *                      SCListWrite_p;  /* Address where 1st byte of next transfer will be written */
    void *                      SCListRead_p;   /* (Read_p == Write_p) means empty */
    /* ext calls : share input info with injection */
    void *                      ExtHandle;
    ST_ErrorCode_t              (*GetWriteAddress)
                            (void * const Handle, void ** const Address_p);
    void                        (*InformReadAddress)
                            (void * const Handle, void * const Address);
    /* ext calls : share output info with hal-decode-video */
    VIDINJ_TransferDoneFct_t    UpdateHalDecodeFct;
    /* General */

    /* Appart creation (calling VIDINJ_TransferGetInject()), modified : */
    enum {STOPPED, RUNNING}     TransferState; /* only when calling_TransferStart/Stop() */
    U32                         MaxLoopsBeforeOneTransfer;
    U32                         LoopsBeforeNextTransfer;

    U32                         HalDecodeIdent;
    ST_Partition_t *            CPUPartition;
    STAVMEM_PartitionHandle_t   AvmemPartition;
    STAVMEM_PartitionHandle_t   FDMANodesAvmemPartition;

#if !defined(STFDMA_USE_VIRTUAL_CONTEXT)
    STFDMA_ContextId_t          FDMAContext;
#else /* STFDMA_USE_VIRTUAL_CONTEXT */
    STFDMA_VirtualContextId_t   VirtualFDMAContextId;
#endif /* STFDMA_USE_VIRTUAL_CONTEXT */
    STFDMA_TransferId_t         FDMATransferId;
    STFDMA_GenericNode_t *      Node_p;
    STAVMEM_BlockHandle_t       NodeHdl;
    STFDMA_GenericNode_t *      NodeNext_p;
    STAVMEM_BlockHandle_t       NodeNextHdl;
    semaphore_t *               TransferSemaphore_p;
    U8 *                        FlushBuffer_p;
    STAVMEM_BlockHandle_t       FlushBufferHdl;
#ifdef STVID_DEBUG_GET_STATISTICS
    U32 InjectFdmaTransfers;            /* Counts the number of FDMA transfers performed */
#endif /* STVID_DEBUG_GET_STATISTICS */
} InjContext_t;

typedef struct
{
    ST_Partition_t *    CPUPartition_p;
    STVID_DeviceType_t  DeviceType;
    VIDCOM_Task_t       InjectionTask;
    STFDMA_Block_t      FDMABlock;   /* one block for all dec */
    STFDMA_ChannelId_t  FDMAChannel; /* one channel for all dec */
#if defined(STFDMA_USE_VIRTUAL_CONTEXT)
    STFDMA_ContextId_t  FDMAContextId; /* FDMA context is common to all injecters */
#endif /* STFDMA_USE_VIRTUAL_CONTEXT */
    STOS_Clock_t        TransferPeriod;
    InjContext_t *      DMAInjecters[MAX_INJECTERS];
    U32                 ValidityCheck;
    BOOL                IsFDMATransferSecure; /* 0=non-secure, 1=secure */
    semaphore_t *       ContextSemaphore_p;
} VIDINJ_Data_t;

typedef enum
{
    INIT_FDMA_CONTEXT_ALLOCATION,
    INIT_GETTING_MEMORY_MAPPING,
    INIT_NODE_ALLOCATION,
    INIT_GETTING_NODE_ADDRESS,
    INIT_NODE_NEXT_ALLOCATION,
    INIT_GETTING_NODE_NEXT_ADDRESS,
    INIT_FLUSH_BUFFER_ALLOCATION,
    INIT_GETTING_FLUSH_BUFFER_ADDRESS,
    INIT_NO_ERROR
} InitStep_t;

/* Variables ---------------------------------------------------------------- */
/* Static functions protos -------------------------------------------------- */
static void ResetInjecter(const VIDINJ_Handle_t InjecterHandle, const U32 InjectNum);
static void InjecterTaskFunc(const VIDINJ_Handle_t InjecterHandle);
static void DMAFlushInjecter(const VIDINJ_Handle_t InjecterHandle, const U32 InjectNum, const BOOL  DiscontinuityStartcode, const U8 * Pattern, const U8 PatternSize);
static BOOL DMATransferOneSlice(const VIDINJ_Handle_t InjecterHandle, const U32 InjectNum);
static ST_ErrorCode_t StartInjectionTask(const VIDINJ_Handle_t InjecterHandle);
static ST_ErrorCode_t StopInjectionTask(const VIDINJ_Handle_t InjecterHandle);
static void InjectStartCode(InjContext_t * InjContext_p, const U32 SCValue, const void* SCAdd);
static ST_ErrorCode_t FDMA_Reset(const VIDINJ_Handle_t InjecterHandle, const U32 InjectNum);

/* Macros -------------------------------------------------------------------- */

#define OutInformReadAddress(x,y)                              \
    {if(VIDINJ_Data_p->DMAInjecters[InjectNum]->InformReadAddress != NULL) \
        {VIDINJ_Data_p->DMAInjecters[InjectNum]->InformReadAddress((x),(y));}}

#define TEST_INJECT(x)                                  \
    if(((x) >= MAX_INJECTERS)||(VIDINJ_Data_p->DMAInjecters[(x)] == NULL)) \
    {                                                   \
        return; /* error in params */                   \
    }
#define TEST_INJECT_ERROR_CODE(x)                       \
    if(((x) >= MAX_INJECTERS)||(VIDINJ_Data_p->DMAInjecters[(x)] == NULL)) \
    {                                                   \
        return(ST_ERROR_BAD_PARAMETER); /* error in params */ \
    }

/* Shortcut to start code entries fields */
#if defined(ST_7100) || defined(ST_7109) || defined(ST_7200)
/* On STi7100 Entry->SC.SCValue is defined as a bitfield, so we can't get its address with the & operator */
#define SetSCVal(Entry, Val)     STSYS_WriteRegMemUncached8((U8*)((U32)(Entry) + 8), (Val))
#else /* (defined(ST_7100) || defined(ST_7109) || defined(ST_7200) */
#define SetSCVal(Entry, Val)     STSYS_WriteRegMemUncached8((U8*)&((Entry)->SC.SCValue), (Val))
#endif /* ST_7100 || ST_7109 */

#define SetSCAdd(Entry, Add)     STSYS_WriteRegMemUncached32LE((U32*)&((Entry)->SC.Addr), (Add))
#define GetSCAdd(Entry)  (STSYS_ReadRegMemUncached32LE((U32*)&((Entry)->SC.Addr)))

#if defined(DVD_SECURED_CHIP)
    #define SetSCAddr2(Entry, Add)  STSYS_WriteRegMemUncached32LE((U32*)&((Entry)->SC.Addr2), (Add))
    #define GetSCAddr2(Entry)       (STSYS_ReadRegMemUncached32LE((U32*)&((Entry)->SC.Addr2)))
#endif /* DVD_SECURED_CHIP */

/* The Entry->SC.Type field is a bitfield, so we can't get its address with the & operator */
#define SetSCType(Entry, Type)   STSYS_WriteRegMemUncached32LE((U32*)(Entry), (Type))

/* Functions ---------------------------------------------------------------- */

/*******************************************************************************
Name        : VIDINJ_Init
Description : Initialise injecter
Parameters  : Init params, pointer to video injecter handle returned if success
Assumptions :
Limitations :
Contexts    : Function can be called by any application thread inside call to STVID_Init()
Returns     : ST_NO_ERROR if success,
              ST_ERROR_BAD_PARAMETER if bad parameter
              ST_ERROR_NO_MEMORY if memory allocation fails
*******************************************************************************/
ST_ErrorCode_t VIDINJ_Init(const VIDINJ_InitParams_t * const InitParams_p, VIDINJ_Handle_t * const InjecterHandle_p)
{
    VIDINJ_Data_t * VIDINJ_Data_p;
    U32 i;
    ST_ErrorCode_t ErrorCode;

    if ((InjecterHandle_p == NULL) || (InitParams_p == NULL))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Allocate a VIDINJ structure */
    SAFE_MEMORY_ALLOCATE(VIDINJ_Data_p, VIDINJ_Data_t *, InitParams_p->CPUPartition_p, sizeof(VIDINJ_Data_t) );
    if (VIDINJ_Data_p == NULL)
    {
        /* Error: allocation failed */
        return(ST_ERROR_NO_MEMORY);
    }
    /* Allocate semaphore for context protection */
    VIDINJ_Data_p->ContextSemaphore_p = STOS_SemaphoreCreateFifo(InitParams_p->CPUPartition_p, 1);
    if (VIDINJ_Data_p->ContextSemaphore_p == NULL)
    {
        /* Error: allocation failed */
        return(ST_ERROR_NO_MEMORY);
    }

    /* Remember partition */
    VIDINJ_Data_p->CPUPartition_p          = InitParams_p->CPUPartition_p;
    VIDINJ_Data_p->DeviceType              = InitParams_p->DeviceType;
    VIDINJ_Data_p->IsFDMATransferSecure    = InitParams_p->FDMASecureTransferMode;
    VIDINJ_Data_p->InjectionTask.IsRunning = FALSE;
    VIDINJ_Data_p->TransferPeriod          = TICKS_TO_NEXT_TRANSFER_FOR_REALTIME_INJECTION;

#if defined(ST_7200)
    VIDINJ_Data_p->FDMABlock = STFDMA_2; /* For Video only */
#else
    VIDINJ_Data_p->FDMABlock = STFDMA_1; /* For Audio only */
#endif

    for (i = 0; i < MAX_INJECTERS; i++)
    {
        VIDINJ_Data_p->DMAInjecters[i] = NULL;
    }

    ErrorCode = STFDMA_LockChannelInPool(STFDMA_PES_POOL, &(VIDINJ_Data_p->FDMAChannel), VIDINJ_Data_p->FDMABlock);
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: FDMA channel lock failed, undo initialisations */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module injection init: could not get FDMA channel!"));
        SAFE_MEMORY_DEALLOCATE(VIDINJ_Data_p, VIDINJ_Data_p->CPUPartition_p, sizeof(VIDINJ_Data_t));
        return(ST_ERROR_BAD_PARAMETER);
    }

#if defined(STFDMA_USE_VIRTUAL_CONTEXT)
    /* Allocate the FDMA context (common to all injection instances) */
    ErrorCode = STFDMA_AllocateContext(&VIDINJ_Data_p->FDMAContextId);
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: FDMA channel lock failed, undo initialisations */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module injection init: could not get FDMA channel!"));
        STFDMA_UnlockChannel(VIDINJ_Data_p->FDMAChannel,VIDINJ_Data_p->FDMABlock);
        SAFE_MEMORY_DEALLOCATE(VIDINJ_Data_p, VIDINJ_Data_p->CPUPartition_p, sizeof(VIDINJ_Data_t));
        return(ST_ERROR_BAD_PARAMETER);
    }
#endif /* STFDMA_USE_VIRTUAL_CONTEXT */

    /* Initialise the timer used to sequence the transfers */
    /* This implementation: we use a task + the task-delay  */
    ErrorCode = StartInjectionTask((VIDINJ_Handle_t) VIDINJ_Data_p);
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: task creation failed, undo initialisations */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module injection init: could not start injection task! (%s)", (ErrorCode == ST_ERROR_ALREADY_INITIALIZED?"already exists":"failed")));
        if (ErrorCode == ST_ERROR_ALREADY_INITIALIZED)
        {
            /* Change error code because ST_ERROR_ALREADY_INITIALIZED is not allowed for _Init() functions */
            ErrorCode = ST_ERROR_BAD_PARAMETER;
        }
#if defined(STFDMA_USE_VIRTUAL_CONTEXT)
        /* Release the FDMA context */
        STFDMA_DeallocateContext(VIDINJ_Data_p->FDMAContextId);
#endif /* STFDMA_USE_VIRTUAL_CONTEXT */
        STFDMA_UnlockChannel(VIDINJ_Data_p->FDMAChannel, VIDINJ_Data_p->FDMABlock);
        SAFE_MEMORY_DEALLOCATE(VIDINJ_Data_p, VIDINJ_Data_p->CPUPartition_p, sizeof(VIDINJ_Data_t));
        return(ErrorCode);
    }

    /* Allocation succeeded: make handle point on structure */
    *InjecterHandle_p = (VIDINJ_Handle_t) VIDINJ_Data_p;
    VIDINJ_Data_p->ValidityCheck = VIDINJ_VALID_HANDLE;

#if defined(DVD_SECURED_CHIP)
    TrBuffers(("INJECT: Init with Secure bit=%d\r\n", VIDINJ_Data_p->IsFDMATransferSecure));
#endif /* DVD_SECURED_CHIP */

    return(ErrorCode);

} /* End of VIDINJ_Init() function. */


/*******************************************************************************
Name        : VIDINJ_Term
Description : Terminate injecter, undo all what was done at init
Parameters  : Video injecter handle
Assumptions : Term can be called in init when init process fails: function to undo
              things done at init should not cause trouble to the system
Limitations :
Contexts    : Function can be called by any application thread inside call to STVID_Term()
Returns     : ST_NO_ERROR if success,
              ST_ERROR_INVALID_HANDLE if not initialised
*******************************************************************************/
ST_ErrorCode_t VIDINJ_Term(const VIDINJ_Handle_t InjecterHandle)
{
    VIDINJ_Data_t * VIDINJ_Data_p = (VIDINJ_Data_t *) InjecterHandle;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    if (VIDINJ_Data_p->ValidityCheck != VIDINJ_VALID_HANDLE)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    StopInjectionTask(InjecterHandle);

#if defined(STFDMA_USE_VIRTUAL_CONTEXT)
    /* Deallocate the shared FDMA context Id */
    ErrorCode = STFDMA_DeallocateContext(VIDINJ_Data_p->FDMAContextId);
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: FDMA context dealloc failed, undo initialisations done */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module injection init: could not dealloc FDMA context!"));
    }
#endif /* STFDMA_USE_VIRTUAL_CONTEXT */

    /* Un-lock the single channel in FDMA for all video injection instances */
    ErrorCode = STFDMA_UnlockChannel(VIDINJ_Data_p->FDMAChannel, VIDINJ_Data_p->FDMABlock);
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: FDMA channel unlock failed, undo initialisations done */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module injection init: could not unlock FDMA channel!"));
    }

    /* De-validate structure */
    VIDINJ_Data_p->ValidityCheck = 0; /* not VIDINJ_VALID_HANDLE */

    /* Free the context locking semaphore */
    STOS_SemaphoreDelete(VIDINJ_Data_p->CPUPartition_p, VIDINJ_Data_p->ContextSemaphore_p);

    /* De-allocate last: data inside cannot be used after ! */
    SAFE_MEMORY_DEALLOCATE(VIDINJ_Data_p, VIDINJ_Data_p->CPUPartition_p, sizeof(VIDINJ_Data_t));

    return(ErrorCode);
} /* end of VIDINJ_Term() function */


/*******************************************************************************
Name        : VIDINJ_TransferFlush
Description : Start the flush of input buffer.
Parameters  : Inject Number
Assumptions : The injection process should be stopped by upper layer.
Limitations :
Returns     :
*******************************************************************************/
void VIDINJ_TransferFlush(const VIDINJ_Handle_t InjecterHandle, const U32 InjectNum, const BOOL DiscontinuityStartcode, U8 *  const Pattern, const U8 Size)
{
    VIDINJ_Data_t * VIDINJ_Data_p = (VIDINJ_Data_t *) InjecterHandle;

    TEST_INJECT(InjectNum);
    /* Do an aligned FDMA transfer */
    DMATransferOneSlice(InjecterHandle, InjectNum);
    /* Flush the remaining non aligned data in the input buffer and inject a startcode discontinuity */
    DMAFlushInjecter(InjecterHandle, InjectNum, DiscontinuityStartcode, Pattern, Size);
    VIDINJ_Data_p->DMAInjecters[InjectNum]->UpdateHalDecodeFct(
                    VIDINJ_Data_p->DMAInjecters[InjectNum]->HalDecodeIdent,
                    VIDINJ_Data_p->DMAInjecters[InjectNum]->ES_Write_p,
                    VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListWrite_p,
                    VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListLoop_p
#if defined(DVD_SECURED_CHIP)
                    , VIDINJ_Data_p->DMAInjecters[InjectNum]->ESCopy_Write_p
#endif /* DVD_SECURED_CHIP */
                    );
} /* End of VIDINJ_TransferFlush() function. */

/*******************************************************************************
Name        : VIDINJ_InjectStartCode
Description : Adds a startcode in the startcode list (NO INJECTION OF THE STARTCODE
              IS DONE !)
Parameters  : Inject Number
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
void VIDINJ_InjectStartCode(const VIDINJ_Handle_t InjecterHandle, const U32 InjectNum, const U32 SCValue, const void* SCAdd)
{
    VIDINJ_Data_t * VIDINJ_Data_p = (VIDINJ_Data_t *) InjecterHandle;

    TEST_INJECT(InjectNum);
    InjectStartCode(VIDINJ_Data_p->DMAInjecters[InjectNum], SCValue, SCAdd);
} /* End of VIDINJ_InjectStartCode() function. */

/*******************************************************************************
Name        : InjectStartCode
Description : Adds a startcode in the startcode list (NO INJECTION OF THE STARTCODE
              IS DONE !) (no semaphore protection here)
Parameters  : Inject Number
Assumptions : Used only with a linear bitbuffer (no loop in startcode addresses,
              no loop in the startcode list)
Limitations :
Returns     :
*******************************************************************************/
static void InjectStartCode(InjContext_t * InjContext_p, const U32 SCValue, const void* SCAdd)
{
    STFDMA_SCEntry_t* SCEntry_p,*SCEntryBeforeTheCurrent_p,*CurrentSCEntry_p, *SCEntryBeforeTheCurrentCPU_p ;
    STFDMA_SCEntry_t* SCListWrite_p;
    STFDMA_SCEntry_t* CurrentSCEntryCPU_p;
    void*   SCListRead_p;
    STFDMA_SCEntry_t SCEntry;

    /* Take a copy of SCListRead_p to use the same value all along this function */
    SCListRead_p = InjContext_p->SCListRead_p;
    /* Verify that the SCListWrite_p hasn't reached the top */
    if(InjContext_p->SCListWrite_p >  InjContext_p->SCListTop_p)
    {
        InjContext_p->SCListLoop_p   = InjContext_p->SCListWrite_p;
        InjContext_p->SCListWrite_p  = InjContext_p->SCListBase_p;
    }

    /* Write a new startcode entry */
    SCListWrite_p = STAVMEM_DeviceToCPU(InjContext_p->SCListWrite_p, STAVMEM_VIRTUAL_MAPPING_AUTO_P);
    SetSCType(SCListWrite_p, STFDMA_SC_ENTRY);
    SetSCVal (SCListWrite_p, SCValue);
    SetSCAdd (SCListWrite_p, (U32)SCAdd);

    #if defined(DVD_SECURED_CHIP)
        SetSCAddr2 (SCListWrite_p, (U32)SCAdd);
    #endif /* DVD_SECURED_CHIP */

    /* Update the SCList write pointer */
    SCEntry_p = ((STFDMA_SCEntry_t*)(InjContext_p->SCListWrite_p));
    SCEntry_p++;
    InjContext_p->SCListWrite_p = (void*)SCEntry_p;

    /* Sort again the startcode list because the address of the new startcode can preceed the address
     * of the startcodes already in the startcode list */
    SCEntry_p--;
    CurrentSCEntry_p = SCEntry_p;
    if(SCEntry_p != InjContext_p->SCListBase_p)
    {
        /* There are some startcodes before the injected startcode */
        SCEntryBeforeTheCurrent_p = SCEntry_p;
        SCEntryBeforeTheCurrent_p--;
    }
    else
    {
        /* The injected startcode was writen at the beginning of the startcode list. No sort is needed. */
        SCEntryBeforeTheCurrent_p = NULL;
    }
    CurrentSCEntryCPU_p = STAVMEM_DeviceToCPU(CurrentSCEntry_p, STAVMEM_VIRTUAL_MAPPING_AUTO_P);
    SCEntryBeforeTheCurrentCPU_p = STAVMEM_DeviceToCPU(SCEntryBeforeTheCurrent_p, STAVMEM_VIRTUAL_MAPPING_AUTO_P);

    while((SCEntryBeforeTheCurrent_p != NULL) && (GetSCAdd(CurrentSCEntryCPU_p) < GetSCAdd(SCEntryBeforeTheCurrentCPU_p)))
    {
        /* Permuting the two entries */

        STOS_memcpyUncachedToUncached((void*)(&SCEntry), (void*)CurrentSCEntry_p, sizeof(STFDMA_SCEntry_t));
        STOS_memcpyUncachedToUncached((void*)CurrentSCEntry_p, (void*)SCEntryBeforeTheCurrent_p, sizeof(STFDMA_SCEntry_t));
        STOS_memcpyUncachedToUncached((void*)SCEntryBeforeTheCurrent_p, (void*)(&SCEntry), sizeof(STFDMA_SCEntry_t));

        /* Update pointers for testing two next entries */
        if(SCEntryBeforeTheCurrent_p != InjContext_p->SCListBase_p)
        {
            SCEntryBeforeTheCurrent_p--;
            CurrentSCEntry_p--;
        }
        else
        {
            SCEntryBeforeTheCurrent_p = NULL;
        }
    }

    /* Update the loop pointer */
    if(InjContext_p->SCListWrite_p > InjContext_p->SCListLoop_p)
    {
        InjContext_p->SCListLoop_p = InjContext_p->SCListWrite_p;
    }

    /* Inform fdma engine the remaining space in start code list.   */
    if ((U32)InjContext_p->SCListWrite_p >= (U32)SCListRead_p)
    {
        /* normal case. Remaining space is until the top of buffer. */
#if !defined(STFDMA_USE_VIRTUAL_CONTEXT)
        STFDMA_ContextSetSCList(InjContext_p->FDMAContext,
                                InjContext_p->SCListWrite_p,
                                (U32)InjContext_p->SCListTop_p - (U32)InjContext_p->SCListWrite_p);
#else /* STFDMA_USE_VIRTUAL_CONTEXT */
        STFDMA_VirtualContextSetSCList(InjContext_p->VirtualFDMAContextId,
                                       InjContext_p->SCListWrite_p,
                                       (U32)InjContext_p->SCListTop_p - (U32)InjContext_p->SCListWrite_p);
#endif /* STFDMA_USE_VIRTUAL_CONTEXT */
    }
    else
    {
        /* Write pointer just looped. Remaining space is until read pointer.    */
#if !defined(STFDMA_USE_VIRTUAL_CONTEXT)
        STFDMA_ContextSetSCList(InjContext_p->FDMAContext,
                                InjContext_p->SCListWrite_p,
                                (U32)SCListRead_p - (U32)InjContext_p->SCListWrite_p);
#else /* STFDMA_USE_VIRTUAL_CONTEXT */
        STFDMA_VirtualContextSetSCList(InjContext_p->VirtualFDMAContextId,
                                       InjContext_p->SCListWrite_p,
                                       (U32)SCListRead_p - (U32)InjContext_p->SCListWrite_p);
#endif /* STFDMA_USE_VIRTUAL_CONTEXT */
    }
    /* Update the decode module with the new informations */
    InjContext_p->UpdateHalDecodeFct(
                    InjContext_p->HalDecodeIdent,
                    InjContext_p->ES_Write_p,
                    InjContext_p->SCListWrite_p,
                    InjContext_p->SCListLoop_p
#if defined(DVD_SECURED_CHIP)
                    , InjContext_p->ESCopy_Write_p
#endif /* DVD_SECURED_CHIP */
                    );
} /* End of InjectStartCode() function. */

/*******************************************************************************
Name        : VIDINJ_TransferGetInject
Description : Equivalent to Init-Open a connexion to an injecter
Parameters  :
Assumptions :
Limitations :
Contexts    : Function can be called by any application thread inside call to STVID_Init()
Returns     : If success, returns a number (called InjectNum) = index in the DMAInjecters array
              If failure, returns BAD_INJECT_NUM (no index could be found)
*******************************************************************************/
U32 VIDINJ_TransferGetInject(const VIDINJ_Handle_t InjecterHandle, VIDINJ_GetInjectParams_t * const Params_p)
{
    VIDINJ_Data_t * VIDINJ_Data_p = (VIDINJ_Data_t *) InjecterHandle;
    U32                                     i;
    void *                                  VirtualAddress;
    STAVMEM_AllocBlockParams_t              AllocBlockParams;
    ST_ErrorCode_t                          ErrorCode;
    InjContext_t *                          InjContext_p = NULL;
    STAVMEM_FreeBlockParams_t               FreeParams;
    InitStep_t                              CurrentStep, InitStep = INIT_NO_ERROR;

    /* look for a free injecter */
    /*--------------------------*/
    i = 0;
    while((VIDINJ_Data_p->DMAInjecters[i] != NULL) && (i < MAX_INJECTERS))
    {
        i++;
    }
    if(i >= MAX_INJECTERS)
    {
        return(BAD_INJECT_NUM);
    }
    /* else : continue with i=index of an available injecter */
    SAFE_MEMORY_ALLOCATE(InjContext_p, InjContext_t *, Params_p->CPUPartition, sizeof(InjContext_t));
    if(InjContext_p == NULL)
    {
        return(BAD_INJECT_NUM);
    }

    /* Now fill all data structure with default or useful value */
    InjContext_p->InputBase_p              = VIDINJ_INVALID_ADDRESS;
    InjContext_p->InputTop_p               = VIDINJ_INVALID_ADDRESS;
    InjContext_p->InputWrite_p             = VIDINJ_INVALID_ADDRESS;
    InjContext_p->InputRead_p              = VIDINJ_INVALID_ADDRESS;

    InjContext_p->ES_Base_p                = VIDINJ_INVALID_ADDRESS;
    InjContext_p->ES_Top_p                 = VIDINJ_INVALID_ADDRESS;
    InjContext_p->ES_Write_p               = VIDINJ_INVALID_ADDRESS;
    InjContext_p->ES_Read_p                = VIDINJ_INVALID_ADDRESS;

    InjContext_p->SCListBase_p             = VIDINJ_INVALID_ADDRESS;
    InjContext_p->SCListTop_p              = VIDINJ_INVALID_ADDRESS;
    InjContext_p->SCListWrite_p            = VIDINJ_INVALID_ADDRESS;
    InjContext_p->SCListRead_p             = VIDINJ_INVALID_ADDRESS;
    InjContext_p->SCListLoop_p             = VIDINJ_INVALID_ADDRESS;

#if defined(DVD_SECURED_CHIP)
    InjContext_p->ESCopy_Base_p             = VIDINJ_INVALID_ADDRESS;
    InjContext_p->ESCopy_Top_p              = VIDINJ_INVALID_ADDRESS;
    InjContext_p->ESCopy_Write_p            = VIDINJ_INVALID_ADDRESS;
    InjContext_p->ESCopy_Read_p             = VIDINJ_INVALID_ADDRESS;
#endif /* DVD_SECURED_CHIP */

    InjContext_p->ExtHandle                = NULL;
    InjContext_p->GetWriteAddress          = NULL;
    InjContext_p->InformReadAddress        = NULL;

    InjContext_p->UpdateHalDecodeFct       = Params_p->TransferDoneFct;

    InjContext_p->TransferState            = STOPPED;

    InjContext_p->HalDecodeIdent           = Params_p->UserIdent;
    InjContext_p->CPUPartition             = Params_p->CPUPartition;
    InjContext_p->AvmemPartition           = Params_p->AvmemPartition;
    InjContext_p->FDMANodesAvmemPartition  = Params_p->AvmemPartition;

    InjContext_p->MaxLoopsBeforeOneTransfer = 1;
    InjContext_p->LoopsBeforeNextTransfer   = 1;
#if defined(STFDMA_USE_VIRTUAL_CONTEXT)
    InjContext_p->VirtualFDMAContextId      = 0; /* no default value... */
#endif /* STFDMA_USE_VIRTUAL_CONTEXT */

#ifdef BENCHMARK_WINCESTAPI
	P_ADDSEMAPHORE(InjContext_p->TransferSemaphore_p, "InjContext_p->TransferSemaphore_p");
#endif

    /* create a fdma-context and reserve memory for 2 nodes */
    InitStep = INIT_FDMA_CONTEXT_ALLOCATION;

#if !defined(STFDMA_USE_VIRTUAL_CONTEXT)
    ErrorCode = STFDMA_AllocateContext(&InjContext_p->FDMAContext);
#else /* STFDMA_USE_VIRTUAL_CONTEXT */
    /* Allocate a virtual FDMA context for this Injecter instance */
    ErrorCode = STFDMA_AllocateVirtualContext(&InjContext_p->VirtualFDMAContextId);
#endif /* STFDMA_USE_VIRTUAL_CONTEXT */
    InjContext_p->FDMATransferId = 0; /* No default value... */

    if(ErrorCode == ST_NO_ERROR)
    {
        InitStep = INIT_GETTING_MEMORY_MAPPING;
        /* Allocate nodes for FDMA transfers */
            InitStep = INIT_NODE_ALLOCATION;
            AllocBlockParams.PartitionHandle          = InjContext_p->FDMANodesAvmemPartition;
            AllocBlockParams.Alignment                = FDMA_PESSCD_NODE_ALIGNMENT;
            AllocBlockParams.Size                     = sizeof(STFDMA_GenericNode_t);
            AllocBlockParams.AllocMode                = STAVMEM_ALLOC_MODE_BOTTOM_TOP;
            AllocBlockParams.NumberOfForbiddenRanges  = 0;
            AllocBlockParams.ForbiddenRangeArray_p    = NULL;
            AllocBlockParams.NumberOfForbiddenBorders = 0;
            AllocBlockParams.ForbiddenBorderArray_p   = NULL;

            /* Allocate first node */
            ErrorCode = STAVMEM_AllocBlock(&AllocBlockParams, &InjContext_p->NodeHdl);
            if(ErrorCode == ST_NO_ERROR)
            {
                InitStep = INIT_GETTING_NODE_ADDRESS;
                ErrorCode = STAVMEM_GetBlockAddress(InjContext_p->NodeHdl,(void **)&VirtualAddress);
                if(ErrorCode == ST_NO_ERROR)
                {
                    InitStep = INIT_NODE_NEXT_ALLOCATION;
                    InjContext_p->Node_p = STAVMEM_VirtualToCPU(VirtualAddress, STAVMEM_VIRTUAL_MAPPING_AUTO_P);
                    /* reset all node's fields (reserved fields must be set to ZERO) */
                    STOS_memsetUncached((void*)((U32)InjContext_p->Node_p), 0x00, sizeof(STFDMA_GenericNode_t));
                    /* Allocate the second node */
                    ErrorCode = STAVMEM_AllocBlock(&AllocBlockParams,&InjContext_p->NodeNextHdl);
                    if(ErrorCode == ST_NO_ERROR)
                    {
                        InitStep = INIT_GETTING_NODE_NEXT_ADDRESS;
                        ErrorCode = STAVMEM_GetBlockAddress(InjContext_p->NodeNextHdl,(void **)&VirtualAddress);
                        if(ErrorCode == ST_NO_ERROR)
                        {
                            InitStep = INIT_FLUSH_BUFFER_ALLOCATION;
                        InjContext_p->NodeNext_p = STAVMEM_VirtualToCPU(VirtualAddress, STAVMEM_VIRTUAL_MAPPING_AUTO_P);
                            /* Fill the constant fields in the nodes */
                            InjContext_p->Node_p->ContextNode.Extended = STFDMA_EXTENDED_NODE;
                            InjContext_p->Node_p->ContextNode.Type     = STFDMA_EXT_NODE_PES;
#if !defined(STFDMA_USE_VIRTUAL_CONTEXT)
                            InjContext_p->Node_p->ContextNode.Context  = InjContext_p->FDMAContext;
#else /* STFDMA_USE_VIRTUAL_CONTEXT */
                            InjContext_p->Node_p->ContextNode.Context  = VIDINJ_Data_p->FDMAContextId;
#endif /* STFDMA_USE_VIRTUAL_CONTEXT */
                            InjContext_p->Node_p->ContextNode.Tag      = 0;
#if defined (ST_7109) || defined (ST_5525)
                        /* Note: remove the "#if defined 7109 || 5525" when the Secure bit is made available */
                        /* to all other chips in next fdma release (current is 2.0.1). */
                        InjContext_p->Node_p->ContextNode.Secure   = VIDINJ_Data_p->IsFDMATransferSecure;
#elif defined (ST_7200)
                        InjContext_p->Node_p->ContextNode.Secure   = 0;
#endif /* 7109 || 5525 || 7200 */

                        InjContext_p->Node_p->ContextNode.Pad1 = 0;
                        InjContext_p->Node_p->ContextNode.Pad2 = 0;

                        InjContext_p->Node_p->ContextNode.NodeCompletePause   = 0; /* 1bit */
                        InjContext_p->Node_p->ContextNode.NodeCompleteNotify  = 0; /* 1bit */
                        /* Flush the cache content */
#if defined (ST_OS20)
                         *(InjContext_p->NodeNext_p)                = *(InjContext_p->Node_p);
                        STOS_memcpyCachedToUncached((void*)InjContext_p->Node_p, (void*)InjContext_p->Node_p,  sizeof(STFDMA_ContextNode_t));
                        STOS_memcpyCachedToUncached((void*)InjContext_p->NodeNext_p, (void*)InjContext_p->Node_p, sizeof(STFDMA_ContextNode_t));
#else
                        STOS_memcpy(InjContext_p->NodeNext_p, InjContext_p->Node_p, sizeof(STFDMA_ContextNode_t));
                        STOS_FlushRegion( STOS_MapPhysToCached(InjContext_p->Node_p, sizeof(STFDMA_ContextNode_t) ), sizeof(STFDMA_ContextNode_t) );
                        STOS_FlushRegion( STOS_MapPhysToCached(InjContext_p->NodeNext_p, sizeof(STFDMA_ContextNode_t) ), sizeof(STFDMA_ContextNode_t) );
#endif

                            AllocBlockParams.Alignment                = FDMA_MEMORY_BUFFER_ALIGNMENT;
                            AllocBlockParams.Size                     = FLUSH_DATA_SIZE;
                            AllocBlockParams.PartitionHandle          = Params_p->AvmemPartition;

                            ErrorCode = STAVMEM_AllocBlock(&AllocBlockParams,&InjContext_p->FlushBufferHdl);
                            if(ErrorCode == ST_NO_ERROR)
                            {
                                InitStep = INIT_GETTING_FLUSH_BUFFER_ADDRESS;
                                ErrorCode = STAVMEM_GetBlockAddress(InjContext_p->FlushBufferHdl,(void **)&VirtualAddress);
                                if(ErrorCode == ST_NO_ERROR)
                                {
                                    InitStep = INIT_NO_ERROR;
                                InjContext_p->FlushBuffer_p = STAVMEM_VirtualToDevice(VirtualAddress, STAVMEM_VIRTUAL_MAPPING_AUTO_P);
                            }
                        }
                    }
                }
            }
        }
    }
    /* Check for errors at init stage */
    if(InitStep != INIT_NO_ERROR)
    {
        CurrentStep = InitStep;

        /* Free memory according to the init step reached */
        while(CurrentStep > INIT_FDMA_CONTEXT_ALLOCATION)
        {
            switch(CurrentStep)
            {
                case INIT_FDMA_CONTEXT_ALLOCATION:
                    SAFE_MEMORY_DEALLOCATE(InjContext_p, InjContext_p->CPUPartition, sizeof(InjContext_t));
                    break;
                case INIT_GETTING_MEMORY_MAPPING:
#if !defined(STFDMA_USE_VIRTUAL_CONTEXT)
                    STFDMA_DeallocateContext(InjContext_p->FDMAContext);
#else /* STFDMA_USE_VIRTUAL_CONTEXT */
                    STFDMA_DeallocateVirtualContext(InjContext_p->VirtualFDMAContextId);
#endif /* STFDMA_USE_VIRTUAL_CONTEXT */
                    break;
                case INIT_NODE_ALLOCATION:
                    break;
                case INIT_GETTING_NODE_ADDRESS:
                    FreeParams.PartitionHandle = InjContext_p->FDMANodesAvmemPartition;
                    STAVMEM_FreeBlock(&FreeParams,&(InjContext_p->NodeHdl));
                    break;
                case INIT_NODE_NEXT_ALLOCATION:
                    break;
                case INIT_GETTING_NODE_NEXT_ADDRESS:
                    FreeParams.PartitionHandle = InjContext_p->FDMANodesAvmemPartition;
                    STAVMEM_FreeBlock(&FreeParams,&(InjContext_p->NodeNextHdl));
                    break;
                case INIT_FLUSH_BUFFER_ALLOCATION:
                    break;
                case INIT_GETTING_FLUSH_BUFFER_ADDRESS:
                    FreeParams.PartitionHandle = InjContext_p->AvmemPartition;
                    STAVMEM_FreeBlock(&FreeParams,&(InjContext_p->FlushBufferHdl));
                    break;
                default :
                break;
            }
            CurrentStep--;
        }
        return(BAD_INJECT_NUM);
    }

#ifdef STVID_DEBUG_GET_STATISTICS
    InjContext_p->InjectFdmaTransfers       = 0;
#endif /* STVID_DEBUG_GET_STATISTICS */

    /* Good initialization. Update now the Injecter pointer. */
    VIDINJ_Data_p->DMAInjecters[i] = InjContext_p;

    return(i);
} /* end of VIDINJ_TransferGetInject() function */

/*******************************************************************************
Name        : VIDINJ_TransferReleaseInject
Description :
Parameters  :
Assumptions :
Limitations :
Contexts    : Function can be called by any application thread inside call to STVID_Term()
Returns     :
*******************************************************************************/
void VIDINJ_TransferReleaseInject(const VIDINJ_Handle_t InjecterHandle, const U32 InjectNum)
{
    VIDINJ_Data_t * VIDINJ_Data_p = (VIDINJ_Data_t *) InjecterHandle;
    InjContext_t *              InjContext_p;
    STAVMEM_FreeBlockParams_t   FreeParams;

    TEST_INJECT(InjectNum);
    InjContext_p = VIDINJ_Data_p->DMAInjecters[InjectNum];
    VIDINJ_Data_p->DMAInjecters[InjectNum] = NULL;

    FreeParams.PartitionHandle = InjContext_p->AvmemPartition;
    STAVMEM_FreeBlock(&FreeParams,&(InjContext_p->FlushBufferHdl));
    FreeParams.PartitionHandle = InjContext_p->FDMANodesAvmemPartition;
    STAVMEM_FreeBlock(&FreeParams,&(InjContext_p->NodeHdl));
    STAVMEM_FreeBlock(&FreeParams,&(InjContext_p->NodeNextHdl));
#if !defined(STFDMA_USE_VIRTUAL_CONTEXT)
    STFDMA_DeallocateContext(InjContext_p->FDMAContext);
#else /* STFDMA_USE_VIRTUAL_CONTEXT */
    STFDMA_DeallocateVirtualContext(InjContext_p->VirtualFDMAContextId);
#endif /* STFDMA_USE_VIRTUAL_CONTEXT */
    SAFE_MEMORY_DEALLOCATE(InjContext_p, InjContext_p->CPUPartition, sizeof(InjContext_t));
} /* end of VIDINJ_TransferReleaseInject() function */

/*******************************************************************************
Name        : VIDINJ_TransferReset
Description : reset the transfer process (ask to not notify HALDEC of the current
              transfer, and to reset PES buffer after that)
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
void VIDINJ_TransferReset(const VIDINJ_Handle_t InjecterHandle, const U32 InjectNum)
{
    VIDINJ_Data_t * VIDINJ_Data_p = (VIDINJ_Data_t *) InjecterHandle;
    TEST_INJECT(InjectNum);
    ResetInjecter(InjecterHandle, InjectNum);
} /* end of VIDINJ_TransferReset() function */

/*******************************************************************************
Name        : VIDINJ_TransferStop
Description : stop the transfer process
Parameters  :
Assumptions :
Limitations :
Contexts    : Function can be called by the producer thread or any application thread, while doing video driver Stop()/Term()
Returns     :
*******************************************************************************/
void VIDINJ_TransferStop(const VIDINJ_Handle_t InjecterHandle, const U32 InjectNum)
{
    VIDINJ_Data_t * VIDINJ_Data_p = (VIDINJ_Data_t *) InjecterHandle;
    U32 i;
    BOOL IsFrequencyToBeChanged = TRUE;

    TEST_INJECT(InjectNum);

    /* Check if we need to reduce the transfers frequency. This could be the
     * case if all the remaining instances are REALTIME. */
    for(i = 0; i < MAX_INJECTERS; i++)
    {
        if((i != InjectNum) && (VIDINJ_Data_p->DMAInjecters[i] != NULL) && (VIDINJ_Data_p->DMAInjecters[i]->MaxLoopsBeforeOneTransfer == 1))
        {
            IsFrequencyToBeChanged = FALSE;
            break;
        }
    }
    /* Now reduce it and set the number of loops for each instance */
    if(IsFrequencyToBeChanged)
    {
        for(i = 0; i < MAX_INJECTERS; i++)
        {
            if(VIDINJ_Data_p->DMAInjecters[i] != NULL)
            {
                VIDINJ_Data_p->DMAInjecters[i]->MaxLoopsBeforeOneTransfer = 1;
                VIDINJ_Data_p->DMAInjecters[i]->LoopsBeforeNextTransfer   = 1;
            }
        }
        VIDINJ_Data_p->TransferPeriod = TICKS_TO_NEXT_TRANSFER_FOR_REALTIME_INJECTION;
    }
    else
    {
        VIDINJ_Data_p->DMAInjecters[InjectNum]->MaxLoopsBeforeOneTransfer = TICKS_TO_NEXT_TRANSFER_FOR_REALTIME_INJECTION / VIDINJ_Data_p->TransferPeriod;
        VIDINJ_Data_p->DMAInjecters[InjectNum]->LoopsBeforeNextTransfer   = VIDINJ_Data_p->DMAInjecters[InjectNum]->MaxLoopsBeforeOneTransfer;
    }

    VIDINJ_Data_p->DMAInjecters[InjectNum]->TransferState = STOPPED;
} /* end of VIDINJ_TransferStop() function */

/*******************************************************************************
Name        : VIDINJ_TransferStart
Description : start the transfer process
Parameters  :
Assumptions :
Limitations :
Contexts    : Function can be called by the producer thread or any application thread, while doing video driver Start()
Returns     :
*******************************************************************************/
void VIDINJ_TransferStart(const VIDINJ_Handle_t InjecterHandle, const U32 InjectNum, const BOOL IsRealTime)
{
    VIDINJ_Data_t * VIDINJ_Data_p = (VIDINJ_Data_t *) InjecterHandle;
    U32 i;
    STOS_Clock_t TransferPeriod;

    TEST_INJECT(InjectNum);

    /* Compute the time duration needed between two FDMA transfers, according to
     * the realtime parameter */
    if(IsRealTime)
    {
        TransferPeriod = TICKS_TO_NEXT_TRANSFER_FOR_REALTIME_INJECTION;
    }
    else
    {
        TransferPeriod = TICKS_TO_NEXT_TRANSFER_FOR_HDD_INJECTION;
    }

    /* If the new duration is less than the currently used, we need to update
       all the instances with the right number of loops, according to the new
       period that will be set. */
    if(TransferPeriod < VIDINJ_Data_p->TransferPeriod)
    {
        for(i = 0; i < MAX_INJECTERS; i++)
        {
            if((VIDINJ_Data_p->DMAInjecters[i] != NULL) && (i != InjectNum))
            {
                VIDINJ_Data_p->DMAInjecters[i]->MaxLoopsBeforeOneTransfer = (VIDINJ_Data_p->DMAInjecters[i]->MaxLoopsBeforeOneTransfer * VIDINJ_Data_p->TransferPeriod) / TransferPeriod;
                VIDINJ_Data_p->DMAInjecters[i]->LoopsBeforeNextTransfer   = 1;
            }
        }
        VIDINJ_Data_p->TransferPeriod = TransferPeriod;
    }
    /* Set the number of loops before next transfer for the current injecter */
    VIDINJ_Data_p->DMAInjecters[InjectNum]->MaxLoopsBeforeOneTransfer = TransferPeriod / VIDINJ_Data_p->TransferPeriod ;
    VIDINJ_Data_p->DMAInjecters[InjectNum]->LoopsBeforeNextTransfer = 1;

    VIDINJ_Data_p->DMAInjecters[InjectNum]->TransferState = RUNNING;
    /* Don't inform application about read pointer until reset sequence has
     * finished (will be done when ResetInjecter is be called)*/
} /* end of VIDINJ_TransferStart() function */

/*******************************************************************************
Name        : ResetInjecter
Description : Resets the content of the PES, ES, SCList contents
Parameters  : InjectNum,
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void ResetInjecter(const VIDINJ_Handle_t InjecterHandle, const U32 InjectNum)
{
    VIDINJ_Data_t * VIDINJ_Data_p = (VIDINJ_Data_t *) InjecterHandle;
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;
    U32             SCListSize;
    void*           NewWrite_p;

#if defined(DVD_SECURED_CHIP)
    STFDMA_VideoCodec_t VideoCodec;
    U32 TransferCount;
    U32 ESCopySize;
#endif /* DVD_SECURED_CHIP */

    /* First Reset PES buffer */
    TrBuffers(("INJECT: Injecter Reset\r\n"));
    if (VIDINJ_Data_p->DMAInjecters[InjectNum]->GetWriteAddress != NULL)
    {
        ErrorCode = VIDINJ_Data_p->DMAInjecters[InjectNum]->GetWriteAddress(VIDINJ_Data_p->DMAInjecters[InjectNum]->ExtHandle, &NewWrite_p);
        if(ErrorCode != ST_NO_ERROR)
        {
            NewWrite_p = VIDINJ_Data_p->DMAInjecters[InjectNum]->InputWrite_p;
        }
    }
    else
    {
        NewWrite_p = VIDINJ_Data_p->DMAInjecters[InjectNum]->InputWrite_p;
    }
    /* Avoid having a NewWrite_p > InputTop_p */
    if((U32)NewWrite_p > (U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->InputTop_p)
    {
        NewWrite_p = VIDINJ_Data_p->DMAInjecters[InjectNum]->InputBase_p;
    }

    #if !defined(DVD_SECURED_CHIP)
        /* DG TO DO: On secure platforms, CPU cannot access PES buffer. Use FDMA transfer instead of memset? */
        STOS_memsetUncached((void*)(((U32)NewWrite_p) & ALIGNEMENT_FOR_TRANSFERS), 0xff, ((U32)NewWrite_p) - (((U32)NewWrite_p) & ALIGNEMENT_FOR_TRANSFERS) );
    #endif /* Secured chip */

    VIDINJ_Data_p->DMAInjecters[InjectNum]->InputRead_p  = (void*)(((U32)NewWrite_p) & ALIGNEMENT_FOR_TRANSFERS);
    /* Inform the world that we have cleaned input buffer */
    OutInformReadAddress(VIDINJ_Data_p->DMAInjecters[InjectNum]->ExtHandle, VIDINJ_Data_p->DMAInjecters[InjectNum]->InputRead_p);
    /* Reset the FDMA */
    ErrorCode = FDMA_Reset(InjecterHandle, InjectNum);
    /* Reset nodes with new context */


#if !defined(STFDMA_USE_VIRTUAL_CONTEXT)
    VIDINJ_Data_p->DMAInjecters[InjectNum]->Node_p->ContextNode.Context  = VIDINJ_Data_p->DMAInjecters[InjectNum]->FDMAContext & 0xF;
#else /* STFDMA_USE_VIRTUAL_CONTEXT */
    VIDINJ_Data_p->DMAInjecters[InjectNum]->Node_p->ContextNode.Context  = VIDINJ_Data_p->FDMAContextId & 0xF;
#endif /* STFDMA_USE_VIRTUAL_CONTEXT */

	/* Now Flush the cache content */
#if defined (ST_OS20)
    STOS_memcpyCachedToUncached((void*)STAVMEM_CPUToDevice(VIDINJ_Data_p->DMAInjecters[InjectNum]->Node_p, STAVMEM_VIRTUAL_MAPPING_AUTO_P),
                                (void*)VIDINJ_Data_p->DMAInjecters[InjectNum]->Node_p,
                                sizeof(STFDMA_ContextNode_t));
    STOS_memcpyCachedToUncached((void*)STAVMEM_CPUToDevice(VIDINJ_Data_p->DMAInjecters[InjectNum]->NodeNext_p, STAVMEM_VIRTUAL_MAPPING_AUTO_P),
                                  (void*)VIDINJ_Data_p->DMAInjecters[InjectNum]->Node_p,
                                  sizeof(STFDMA_ContextNode_t));

#else
    STOS_memcpy((void*)VIDINJ_Data_p->DMAInjecters[InjectNum]->NodeNext_p,
                (void*)VIDINJ_Data_p->DMAInjecters[InjectNum]->Node_p,
                     sizeof(STFDMA_ContextNode_t));
    STOS_FlushRegion( STOS_MapPhysToCached(VIDINJ_Data_p->DMAInjecters[InjectNum]->Node_p, sizeof(STFDMA_ContextNode_t) ), sizeof(STFDMA_ContextNode_t) );
    STOS_FlushRegion( STOS_MapPhysToCached(VIDINJ_Data_p->DMAInjecters[InjectNum]->NodeNext_p, sizeof(STFDMA_ContextNode_t) ), sizeof(STFDMA_ContextNode_t) );
#endif


    /* Reset local ES Buffer pointers */
    VIDINJ_Data_p->DMAInjecters[InjectNum]->ES_Read_p        = VIDINJ_Data_p->DMAInjecters[InjectNum]->ES_Base_p;
    VIDINJ_Data_p->DMAInjecters[InjectNum]->ES_Write_p       = VIDINJ_Data_p->DMAInjecters[InjectNum]->ES_Base_p;

#if !defined(STFDMA_USE_VIRTUAL_CONTEXT)
    STFDMA_ContextSetESBuffer(VIDINJ_Data_p->DMAInjecters[InjectNum]->FDMAContext,
                              VIDINJ_Data_p->DMAInjecters[InjectNum]->ES_Base_p,
                              (U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->ES_Top_p - (U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->ES_Base_p + 1);
    STFDMA_ContextSetESReadPtr(VIDINJ_Data_p->DMAInjecters[InjectNum]->FDMAContext, VIDINJ_Data_p->DMAInjecters[InjectNum]->ES_Base_p);
    STFDMA_ContextSetESWritePtr(VIDINJ_Data_p->DMAInjecters[InjectNum]->FDMAContext, VIDINJ_Data_p->DMAInjecters[InjectNum]->ES_Base_p);


#else /* STFDMA_USE_VIRTUAL_CONTEXT */
    STFDMA_VirtualContextSetESBuffer(VIDINJ_Data_p->DMAInjecters[InjectNum]->VirtualFDMAContextId,
                                     VIDINJ_Data_p->DMAInjecters[InjectNum]->ES_Base_p,
                                     (U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->ES_Top_p - (U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->ES_Base_p + 1);
    STFDMA_VirtualContextSetESReadPtr(VIDINJ_Data_p->DMAInjecters[InjectNum]->VirtualFDMAContextId, VIDINJ_Data_p->DMAInjecters[InjectNum]->ES_Base_p);
    STFDMA_VirtualContextSetESWritePtr(VIDINJ_Data_p->DMAInjecters[InjectNum]->VirtualFDMAContextId, VIDINJ_Data_p->DMAInjecters[InjectNum]->ES_Base_p);
#endif /* STFDMA_USE_VIRTUAL_CONTEXT */

    /* Reset local StartCodeList pointers */
    VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListLoop_p     = VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListBase_p;
    VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListWrite_p    = VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListBase_p;
    VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListRead_p     = VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListBase_p;
    SCListSize = (U32)(VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListTop_p) - (U32)(VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListBase_p) + sizeof(STFDMA_SCEntry_t);

#if !defined(STFDMA_USE_VIRTUAL_CONTEXT)
    STFDMA_ContextSetSCList(VIDINJ_Data_p->DMAInjecters[InjectNum]->FDMAContext,
                            VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListBase_p,
                            SCListSize);
#else /* STFDMA_USE_VIRTUAL_CONTEXT */
    STFDMA_VirtualContextSetSCList(VIDINJ_Data_p->DMAInjecters[InjectNum]->VirtualFDMAContextId,
                                   VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListBase_p,
                                   SCListSize);
#endif /* STFDMA_USE_VIRTUAL_CONTEXT */

#if defined(DVD_SECURED_CHIP)
    /* For secure platforms, add reset of ES Copy buffer */
    /* Reset local StartCodeList pointers */
    VIDINJ_Data_p->DMAInjecters[InjectNum]->ESCopy_Write_p    = VIDINJ_Data_p->DMAInjecters[InjectNum]->ESCopy_Base_p;
    VIDINJ_Data_p->DMAInjecters[InjectNum]->ESCopy_Read_p     = VIDINJ_Data_p->DMAInjecters[InjectNum]->ESCopy_Base_p;
    ESCopySize = (U32)(VIDINJ_Data_p->DMAInjecters[InjectNum]->ESCopy_Top_p) - (U32)(VIDINJ_Data_p->DMAInjecters[InjectNum]->ESCopy_Base_p);

    /* Set TransferCount for call to STFDMA_SetESToHCBufferTransferCount() or STFDMA_VirtualContextSetESToHCBufferTransferCount() */
    switch (VIDINJ_Data_p->DeviceType)
    {
        case STVID_DEVICE_TYPE_7109_VC1:
            VideoCodec = VIDEO_CODEC_VC1;
            TransferCount = MAX_VC1_SLICE_TRANSFER;
            break;
        case STVID_DEVICE_TYPE_7109_H264:
            VideoCodec = VIDEO_CODEC_H264;
            TransferCount = MAX_H264_SLICE_TRANSFER;
            break;
        case STVID_DEVICE_TYPE_7109_MPEG:
        default:
            VideoCodec = VIDEO_CODEC_MPEG;
            TransferCount = MAX_MPEG_SLICE_TRANSFER;
    }

    #if !defined(STFDMA_USE_VIRTUAL_CONTEXT)

        STFDMA_ContextSetSCList(VIDINJ_Data_p->DMAInjecters[InjectNum]->FDMAContext,
                                VIDINJ_Data_p->DMAInjecters[InjectNum]->ESCopy_Base_p,
                                ESCopySize);


        STFDMA_ContextSetHeaderCopyBuffer(VIDINJ_Data_p->DMAInjecters[InjectNum]->FDMAContext,
                                          VIDINJ_Data_p->DMAInjecters[InjectNum]->ESCopy_Base_p,
                                          (U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->ESCopy_Top_p
                                           - (U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->ESCopy_Base_p + 1);
        STFDMA_ContextSetHeaderCopyBufferReadPtr(VIDINJ_Data_p->DMAInjecters[InjectNum]->FDMAContext,
                                                 VIDINJ_Data_p->DMAInjecters[InjectNum]->ESCopy_Base_p);
        STFDMA_ContextSetHeaderCopyBufferWritePtr(VIDINJ_Data_p->DMAInjecters[InjectNum]->FDMAContext,
                                                  VIDINJ_Data_p->DMAInjecters[InjectNum]->ESCopy_Base_p);

        STFDMA_SetESToHCBufferTransferCount(VIDINJ_Data_p->DMAInjecters[InjectNum]->FDMAContext, VideoCodec, TransferCount);

    #else /* STFDMA_USE_VIRTUAL_CONTEXT */
        STFDMA_VirtualContextSetSCList(VIDINJ_Data_p->DMAInjecters[InjectNum]->VirtualFDMAContextId,
                                       VIDINJ_Data_p->DMAInjecters[InjectNum]->ESCopy_Base_p,
                                       ESCopySize);

        /* For secure platforms, add reset of ES Copy buffer */
        STFDMA_VirtualContextSetHeaderCopyBuffer(VIDINJ_Data_p->DMAInjecters[InjectNum]->VirtualFDMAContextId,
                                           VIDINJ_Data_p->DMAInjecters[InjectNum]->ESCopy_Base_p,
                                           (U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->ESCopy_Top_p
                                            - (U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->ESCopy_Base_p + 1);
        STFDMA_VirtualContextSetHeaderCopyBufferReadPtr(VIDINJ_Data_p->DMAInjecters[InjectNum]->VirtualFDMAContextId,
                                                 VIDINJ_Data_p->DMAInjecters[InjectNum]->ESCopy_Base_p);
        STFDMA_VirtualContextSetHeaderCopyBufferWritePtr(VIDINJ_Data_p->DMAInjecters[InjectNum]->VirtualFDMAContextId,
                                                 VIDINJ_Data_p->DMAInjecters[InjectNum]->ESCopy_Base_p);

        STFDMA_VirtualContextSetESToHCBufferTransferCount(VIDINJ_Data_p->DMAInjecters[InjectNum]->VirtualFDMAContextId, VideoCodec, TransferCount);

    #endif /* STFDMA_USE_VIRTUAL_CONTEXT */

    TrBuffers(("\r\nINJECT: ResetInjecter - ESCopy buffer: Base=0x%08x, Top=0x%08x, Size=%d",
                (U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->ESCopy_Base_p,
                (U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->ESCopy_Top_p,
                (U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->ESCopy_Top_p - (U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->ESCopy_Base_p + 1));
    TrESCopy(("\r\nINJECT: Codec: %s, MaxTransfer=%d",
                (VideoCodec == VIDEO_CODEC_H264)?"H264":((VideoCodec == VIDEO_CODEC_VC1)?"VC1":"MPEG"),
                TransferCount));
#endif /* DVD_SECURED_CHIP */
} /* end of ResetInjecter(() function */


/*******************************************************************************
Name        : FDMA_Reset
Description : Deallocates the FDMA context and sets up the new startcodes detected
Parameters  : Injecter handle, InjectNum
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t FDMA_Reset(const VIDINJ_Handle_t InjecterHandle, const U32 InjectNum)
{
    VIDINJ_Data_t * VIDINJ_Data_p = (VIDINJ_Data_t *) InjecterHandle;
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;
    VIDINJ_ParserRanges_t ParserRanges[3];
    int i;

    /* We have to reallocate a new context each time we change the stream to be parsed */
#if !defined(STFDMA_USE_VIRTUAL_CONTEXT)
    ErrorCode = STFDMA_DeallocateContext(VIDINJ_Data_p->DMAInjecters[InjectNum]->FDMAContext);
#else /* STFDMA_USE_VIRTUAL_CONTEXT */
    ErrorCode = STFDMA_DeallocateVirtualContext(VIDINJ_Data_p->DMAInjecters[InjectNum]->VirtualFDMAContextId);
#endif /* STFDMA_USE_VIRTUAL_CONTEXT */

    if(ErrorCode == ST_NO_ERROR)
    {
#if !defined(STFDMA_USE_VIRTUAL_CONTEXT)
        ErrorCode = STFDMA_AllocateContext(&(VIDINJ_Data_p->DMAInjecters[InjectNum]->FDMAContext));
#else /* STFDMA_USE_VIRTUAL_CONTEXT */
        ErrorCode = STFDMA_AllocateVirtualContext(&(VIDINJ_Data_p->DMAInjecters[InjectNum]->VirtualFDMAContextId));
#endif /* STFDMA_USE_VIRTUAL_CONTEXT */

        if(ErrorCode == ST_NO_ERROR)
        {
            /* We must also reconfigure the startcode ranges to be detected by FDMA*/
            ParserRanges[0].RangeId                     = STFDMA_DEVICE_ES_RANGE_0;
            ParserRanges[0].RangeConfig.RangeEnabled    = TRUE;
            ParserRanges[0].RangeConfig.RangeStart      = FIRST_SLICE_START_CODE;
            ParserRanges[0].RangeConfig.RangeEnd        = GREATEST_SLICE_START_CODE;
            ParserRanges[0].RangeConfig.PTSEnabled      = FALSE;
            ParserRanges[0].RangeConfig.OneShotEnabled  = TRUE;

            ParserRanges[1].RangeId                     = STFDMA_DEVICE_ES_RANGE_1;
            ParserRanges[1].RangeConfig.RangeEnabled    = TRUE;
            ParserRanges[1].RangeConfig.RangeStart      = RESERVED_START_CODE_0;
            ParserRanges[1].RangeConfig.RangeEnd        = PICTURE_START_CODE;
            ParserRanges[1].RangeConfig.PTSEnabled      = FALSE;
            ParserRanges[1].RangeConfig.OneShotEnabled  = FALSE;

            ParserRanges[2].RangeId                     = STFDMA_DEVICE_PES_RANGE_0;
            ParserRanges[2].RangeConfig.RangeEnabled    = TRUE;
            ParserRanges[2].RangeConfig.RangeStart      = SMALLEST_VIDEO_PACKET_START_CODE;
            ParserRanges[2].RangeConfig.RangeEnd        = GREATEST_VIDEO_PACKET_START_CODE;
            ParserRanges[2].RangeConfig.PTSEnabled      = TRUE;
            ParserRanges[2].RangeConfig.OneShotEnabled  = FALSE;
            for(i = 0; i < 3; i++)
            {
#if !defined(STFDMA_USE_VIRTUAL_CONTEXT)
                ErrorCode = STFDMA_ContextSetSCState(VIDINJ_Data_p->DMAInjecters[InjectNum]->FDMAContext,
                                                     &(ParserRanges[i].RangeConfig),
                                                     ParserRanges[i].RangeId);
#else /* STFDMA_USE_VIRTUAL_CONTEXT */
                ErrorCode = STFDMA_VirtualContextSetSCState(VIDINJ_Data_p->DMAInjecters[InjectNum]->VirtualFDMAContextId,
                                                            &(ParserRanges[i].RangeConfig),
                                                            ParserRanges[i].RangeId);
#endif /* STFDMA_USE_VIRTUAL_CONTEXT */
                if(ErrorCode != ST_NO_ERROR)
                {
                    break;
                }
            }
        }
    }
    return(ErrorCode);
} /* End of function FDMA_Reset()*/

/*******************************************************************************
Name        : VIDINJ_TransferLimits
Description : set the transfer buffers
Parameters  : InjectNum, buf input, bufs output
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
void VIDINJ_TransferLimits(const VIDINJ_Handle_t InjecterHandle, const U32 InjectNum,
                           void * const ES_Start_p,    void * const ES_Stop_p,
                           void * const SCListStart_p, void * const SCListStop_p,
                           void * const InputStart_p,     void * const InputStop_p
#if defined(DVD_SECURED_CHIP)
                         , void * const ESCopy_Start_p,   void * const ESCopy_Stop_p
#endif /* DVD_SECURED_CHIP */
                          )
{
    VIDINJ_Data_t * VIDINJ_Data_p = (VIDINJ_Data_t *) InjecterHandle;
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;
    void*   PESWrite_p;

#if defined(DVD_SECURED_CHIP)
    STFDMA_VideoCodec_t VideoCodec;
    U32 TransferCount;
#endif /* DVD_SECURED_CHIP */

    TEST_INJECT(InjectNum);

    /* Get the write pointer */
    if(VIDINJ_Data_p->DMAInjecters[InjectNum]->GetWriteAddress != NULL)
    {
        ErrorCode = VIDINJ_Data_p->DMAInjecters[InjectNum]->GetWriteAddress(VIDINJ_Data_p->DMAInjecters[InjectNum]->ExtHandle, &PESWrite_p);
        if(ErrorCode != ST_NO_ERROR)
        {
            PESWrite_p = VIDINJ_Data_p->DMAInjecters[InjectNum]->InputWrite_p;
        }
    }
    else
    {
        PESWrite_p = VIDINJ_Data_p->DMAInjecters[InjectNum]->InputWrite_p;
    }
    /* Avoid having a PESWrite_p > InputTop_p */
    if((U32)PESWrite_p > (U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->InputTop_p)
    {
        PESWrite_p = VIDINJ_Data_p->DMAInjecters[InjectNum]->InputBase_p;
    }
    if(VIDINJ_Data_p->DMAInjecters[InjectNum]->InputBase_p == VIDINJ_INVALID_ADDRESS)
    {
        VIDINJ_Data_p->DMAInjecters[InjectNum]->InputBase_p      = InputStart_p;
        VIDINJ_Data_p->DMAInjecters[InjectNum]->InputTop_p       = InputStop_p;
        VIDINJ_Data_p->DMAInjecters[InjectNum]->InputWrite_p     = InputStart_p;
        VIDINJ_Data_p->DMAInjecters[InjectNum]->InputRead_p      = InputStart_p;
    }
    else
    {
        /* When changing from one bitbuffer to another in backward trickmodes, we must
         * take care not to have some of the data present in the input buffer that
         * should have gone in the previous bitbuffer, to go in the current bitbuffer instead.
         * Here we choose to lose this data. */
        VIDINJ_Data_p->DMAInjecters[InjectNum]->InputWrite_p = (void*)((U32)PESWrite_p & ALIGNEMENT_FOR_TRANSFERS);
        VIDINJ_Data_p->DMAInjecters[InjectNum]->InputRead_p  = VIDINJ_Data_p->DMAInjecters[InjectNum]->InputWrite_p;
        OutInformReadAddress(VIDINJ_Data_p->DMAInjecters[InjectNum]->ExtHandle, VIDINJ_Data_p->DMAInjecters[InjectNum]->InputRead_p);
        if(ErrorCode == ST_NO_ERROR)
        {
            /* Fill the non aligned data in the input buffer with 0xFF */
            #if !defined(DVD_SECURED_CHIP)
                /* DG TO DO: On secure platforms, CPU cannot access PES buffer. Use FDMA transfer instead of memset? */
                STOS_memsetUncached((void*)((U32)PESWrite_p & ALIGNEMENT_FOR_TRANSFERS), 0xFF, ((U32)PESWrite_p - (((U32)PESWrite_p) & ALIGNEMENT_FOR_TRANSFERS)));
            #endif /* Secured chip */
        }
    }

    VIDINJ_Data_p->DMAInjecters[InjectNum]->ES_Base_p        = ES_Start_p;
    VIDINJ_Data_p->DMAInjecters[InjectNum]->ES_Top_p         = ES_Stop_p;
    VIDINJ_Data_p->DMAInjecters[InjectNum]->ES_Write_p       = ES_Start_p;
    VIDINJ_Data_p->DMAInjecters[InjectNum]->ES_Read_p        = ES_Start_p;

    VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListBase_p     = SCListStart_p;
    VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListTop_p      = SCListStop_p;
    VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListLoop_p     = SCListStart_p;
    VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListWrite_p    = SCListStart_p;
    VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListRead_p     = SCListStart_p;

    /* Inform FDMA driver about these values = set the context */
    /* Reallocate a new context, to avoid FDMA transfers in the new injection buffer
     * data retained from the previous injection */
    ErrorCode = FDMA_Reset(InjecterHandle, InjectNum);

#if !defined(STFDMA_USE_VIRTUAL_CONTEXT)
    STFDMA_ContextSetESBuffer(VIDINJ_Data_p->DMAInjecters[InjectNum]->FDMAContext, ES_Start_p, (U32)ES_Stop_p - (U32)ES_Start_p + 1);
    STFDMA_ContextSetESReadPtr(VIDINJ_Data_p->DMAInjecters[InjectNum]->FDMAContext, ES_Start_p);
    STFDMA_ContextSetESWritePtr(VIDINJ_Data_p->DMAInjecters[InjectNum]->FDMAContext, ES_Start_p);
    STFDMA_ContextSetSCList(VIDINJ_Data_p->DMAInjecters[InjectNum]->FDMAContext, SCListStart_p,
                            (U32)SCListStop_p - (U32)SCListStart_p + sizeof(STFDMA_SCEntry_t));
#else /* STFDMA_USE_VIRTUAL_CONTEXT */
    STFDMA_VirtualContextSetESBuffer(VIDINJ_Data_p->DMAInjecters[InjectNum]->VirtualFDMAContextId, ES_Start_p, (U32)ES_Stop_p - (U32)ES_Start_p +1);
    STFDMA_VirtualContextSetESReadPtr(VIDINJ_Data_p->DMAInjecters[InjectNum]->VirtualFDMAContextId, ES_Start_p);
    STFDMA_VirtualContextSetESWritePtr(VIDINJ_Data_p->DMAInjecters[InjectNum]->VirtualFDMAContextId, ES_Start_p);
    STFDMA_VirtualContextSetSCList(VIDINJ_Data_p->DMAInjecters[InjectNum]->VirtualFDMAContextId, SCListStart_p,
                                   (U32)SCListStop_p - (U32)SCListStart_p + sizeof(STFDMA_SCEntry_t));
#endif /* STFDMA_USE_VIRTUAL_CONTEXT */

#if defined(DVD_SECURED_CHIP)
    /* Set ESCopyBuffer limits */
    VIDINJ_Data_p->DMAInjecters[InjectNum]->ESCopy_Base_p    = ESCopy_Start_p;
    VIDINJ_Data_p->DMAInjecters[InjectNum]->ESCopy_Top_p     = ESCopy_Stop_p;
    VIDINJ_Data_p->DMAInjecters[InjectNum]->ESCopy_Write_p   = ESCopy_Start_p;
    VIDINJ_Data_p->DMAInjecters[InjectNum]->ESCopy_Read_p    = ESCopy_Start_p;

    switch (VIDINJ_Data_p->DeviceType)
    {
        case STVID_DEVICE_TYPE_7109_VC1:
            VideoCodec = VIDEO_CODEC_VC1;
            TransferCount = MAX_VC1_SLICE_TRANSFER;
            break;
        case STVID_DEVICE_TYPE_7109_H264:
            VideoCodec = VIDEO_CODEC_H264;
            TransferCount = MAX_H264_SLICE_TRANSFER;
            break;
        case STVID_DEVICE_TYPE_7109_MPEG:
        default:
            VideoCodec = VIDEO_CODEC_MPEG;
            TransferCount = MAX_MPEG_SLICE_TRANSFER;
    }

    #if !defined(STFDMA_USE_VIRTUAL_CONTEXT)
        /* For secure platforms, add reset of ES Copy buffer */
        STFDMA_ContextSetHeaderCopyBuffer(VIDINJ_Data_p->DMAInjecters[InjectNum]->FDMAContext,
                                          VIDINJ_Data_p->DMAInjecters[InjectNum]->ESCopy_Base_p,
                                          (U32)ESCopy_Stop_p - (U32)ESCopy_Start_p + 1);
        STFDMA_ContextSetHeaderCopyBufferReadPtr(VIDINJ_Data_p->DMAInjecters[InjectNum]->FDMAContext, ESCopy_Start_p);
        STFDMA_ContextSetHeaderCopyBufferWritePtr(VIDINJ_Data_p->DMAInjecters[InjectNum]->FDMAContext, ESCopy_Start_p);
        STFDMA_SetESToHCBufferTransferCount(VIDINJ_Data_p->DMAInjecters[InjectNum]->FDMAContext, VideoCodec, TransferCount);

    #else /* STFDMA_USE_VIRTUAL_CONTEXT */

        STFDMA_VirtualContextSetHeaderCopyBuffer(VIDINJ_Data_p->DMAInjecters[InjectNum]->VirtualFDMAContextId,
                                                 VIDINJ_Data_p->DMAInjecters[InjectNum]->ESCopy_Base_p,
                                                (U32)ESCopy_Stop_p - (U32)ESCopy_Start_p + 1);
        STFDMA_VirtualContextSetHeaderCopyBufferReadPtr(VIDINJ_Data_p->DMAInjecters[InjectNum]->VirtualFDMAContextId, ESCopy_Start_p);
        STFDMA_VirtualContextSetHeaderCopyBufferWritePtr(VIDINJ_Data_p->DMAInjecters[InjectNum]->VirtualFDMAContextId, ESCopy_Start_p);
        STFDMA_VirtualContextSetESToHCBufferTransferCount(VIDINJ_Data_p->DMAInjecters[InjectNum]->VirtualFDMAContextId, VideoCodec, TransferCount);

    #endif /* STFDMA_USE_VIRTUAL_CONTEXT */

    TrBuffers(("\r\nNew buffers - PES: %08x->%08x, ES: %08x->%08x, SC: %08x->%08x, ESCopy: %08x->%08x\r\n",
                        VIDINJ_Data_p->DMAInjecters[InjectNum]->InputBase_p,
                        VIDINJ_Data_p->DMAInjecters[InjectNum]->InputTop_p,
                        VIDINJ_Data_p->DMAInjecters[InjectNum]->ES_Base_p,
                        VIDINJ_Data_p->DMAInjecters[InjectNum]->ES_Top_p,
                        VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListBase_p,
                        VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListTop_p,
                        VIDINJ_Data_p->DMAInjecters[InjectNum]->ESCopy_Base_p,
                        VIDINJ_Data_p->DMAInjecters[InjectNum]->ESCopy_Top_p));
#else
    TrBuffers(("\r\nNew buffers - PES: %08x->%08x, ES: %08x->%08x, SC: %08x->%08x\r\n",
                        VIDINJ_Data_p->DMAInjecters[InjectNum]->InputBase_p,
                        VIDINJ_Data_p->DMAInjecters[InjectNum]->InputTop_p,
                        VIDINJ_Data_p->DMAInjecters[InjectNum]->ES_Base_p,
                        VIDINJ_Data_p->DMAInjecters[InjectNum]->ES_Top_p,
                        VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListBase_p,
                        VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListTop_p));

#endif /* DVD_SECURED_CHIP */


} /* End of VIDINJ_TransferLimits() function. */

/*******************************************************************************
Name        : VIDINJ_SetSCRanges
Description : Sets the startcodes that the FDMA must detect
Parameters  : injecter handle, number of ranges, startcode ranges
Assumptions : FDMA context allocated
Limitations :
Returns     :
*******************************************************************************/
void VIDINJ_SetSCRanges(const VIDINJ_Handle_t InjecterHandle, const U32 InjectNum, const U32 NbParserRanges, VIDINJ_ParserRanges_t ParserRanges[])
{
    /* Don't care for stub */
    VIDINJ_Data_t * VIDINJ_Data_p = (VIDINJ_Data_t *) InjecterHandle;
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;
    U32 i;

    TEST_INJECT(InjectNum);

    for(i = 0; i < NbParserRanges; i++)
    {
#if !defined(STFDMA_USE_VIRTUAL_CONTEXT)
        ErrorCode = STFDMA_ContextSetSCState(VIDINJ_Data_p->DMAInjecters[InjectNum]->FDMAContext,
                                             &ParserRanges[i].RangeConfig,
                                             ParserRanges[i].RangeId);
#else /* STFDMA_USE_VIRTUAL_CONTEXT */
        ErrorCode = STFDMA_VirtualContextSetSCState(VIDINJ_Data_p->DMAInjecters[InjectNum]->VirtualFDMAContextId,
                                                    &ParserRanges[i].RangeConfig,
                                                    ParserRanges[i].RangeId);
#endif /* STFDMA_USE_VIRTUAL_CONTEXT */
        if(ErrorCode != ST_NO_ERROR)
        {
            break;
        }
    }
}

/*******************************************************************************
Name        : VIDINJ_Transfer_SetDataInputInterface
Description : Register the functions to be called to share info with injection
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t VIDINJ_Transfer_SetDataInputInterface(const VIDINJ_Handle_t InjecterHandle,
        const U32 InjectNum,
        ST_ErrorCode_t (*GetWriteAddress) (void * const Handle, void ** const Address_p),
        void (*InformReadAddress)(void * const Handle, void * const Address),
        void * const Handle )
{
    VIDINJ_Data_t * VIDINJ_Data_p = (VIDINJ_Data_t *) InjecterHandle;
    void *          NewInputWrite_p = NULL;
    ST_ErrorCode_t  ErrorCode       = ST_NO_ERROR;

    TEST_INJECT_ERROR_CODE(InjectNum);

    /* Test if it's a new data input interface or an erase of it.   */
    if ( (GetWriteAddress == NULL) && (InformReadAddress == NULL) )
    {
        /* It's an erase of those entry points. The injection is probably finished  */
        /* so get the last write pointer managed by the application.                */
        if (VIDINJ_Data_p->DMAInjecters[InjectNum]->GetWriteAddress != NULL)
        {
            /* Get the last input write pointer.                                    */
            ErrorCode = VIDINJ_Data_p->DMAInjecters[InjectNum]->GetWriteAddress(
                    VIDINJ_Data_p->DMAInjecters[InjectNum]->ExtHandle,
                    &NewInputWrite_p);

            STTBX_Print(("NewInputWrite_p:0x%.8x\n",(U32) NewInputWrite_p));

            if (ErrorCode == ST_NO_ERROR)
            {
                /* Avoid having a NewInputWrite_p > InputTop_p */
                if((U32)NewInputWrite_p > (U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->InputTop_p)
                {
                    NewInputWrite_p = VIDINJ_Data_p->DMAInjecters[InjectNum]->InputBase_p;
                }
                /* The interface is still valid. Store the last write pointer. */
                VIDINJ_Data_p->DMAInjecters[InjectNum]->InputWrite_p = NewInputWrite_p;
            }
            else
            {
                /* Error with outside world interface. */
                return(ST_ERROR_BAD_PARAMETER);
            }
        }
    }
    else
    {
        /* A new interface is specified.                                            */
        if ( (GetWriteAddress != NULL) && (InformReadAddress != NULL) )
        {
            /* Get the new input write pointer.                                     */
            ErrorCode = GetWriteAddress( Handle, &NewInputWrite_p);
            STTBX_Print(("NewInputWrite_p:0x%.8x\n", (U32) NewInputWrite_p));

            if (ErrorCode == ST_NO_ERROR)
            {
                /* Avoid having a NewInputWrite_p > InputTop_p */
                if((U32)NewInputWrite_p > (U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->InputTop_p)
                {
                    NewInputWrite_p = VIDINJ_Data_p->DMAInjecters[InjectNum]->InputBase_p;
                }
                VIDINJ_Data_p->DMAInjecters[InjectNum]->InputWrite_p =
                        (void*)(((U32)NewInputWrite_p) & ALIGNEMENT_FOR_TRANSFERS);
                /* And align the our input buffer read pointer.                         */
                VIDINJ_Data_p->DMAInjecters[InjectNum]->InputRead_p  =
                        VIDINJ_Data_p->DMAInjecters[InjectNum]->InputWrite_p;

                InformReadAddress(Handle, VIDINJ_Data_p->DMAInjecters[InjectNum]->InputRead_p);
            }
            else
            {
                /* Error with outside world interface. */
                return(ST_ERROR_BAD_PARAMETER);
            }
        }
    }

    /* Update inject's internal data.                                               */
    VIDINJ_Data_p->DMAInjecters[InjectNum]->ExtHandle         = Handle;
    VIDINJ_Data_p->DMAInjecters[InjectNum]->GetWriteAddress   = GetWriteAddress;
    VIDINJ_Data_p->DMAInjecters[InjectNum]->InformReadAddress = InformReadAddress;

    return(ST_NO_ERROR);
} /* end of VIDINJ_Transfer_SetDataInputInterface() function */

/*******************************************************************************
Name        : VIDINJ_TransferSetESRead
Description : video calls this function to update the read pointer in ES buff
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
void VIDINJ_TransferSetESRead(const VIDINJ_Handle_t InjecterHandle, const U32 InjectNum, void * const Read)
{
    VIDINJ_Data_t * VIDINJ_Data_p = (VIDINJ_Data_t *) InjecterHandle;
    TEST_INJECT(InjectNum);

#if defined(TRACE_UART)
    if((U32) Read > (U32) VIDINJ_Data_p->DMAInjecters[InjectNum]->ES_Top_p)
    {
        TrError(("\r\nINJECT ERROR: ES Over the top!"));
    }
#endif

    VIDINJ_Data_p->DMAInjecters[InjectNum]->ES_Read_p = Read;
    TrESBuffer(("\r\nINJECT: TransferSetESRead: 0x%x", Read));
} /* end of VIDINJ_TransferSetESRead() function */

#if defined(DVD_SECURED_CHIP)
/*******************************************************************************
Name        : VIDINJ_TransferSetESCopyBufferRead
Description : video calls this function to update the read pointer in ES Copy buffer
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
void VIDINJ_TransferSetESCopyBufferRead(const VIDINJ_Handle_t InjecterHandle, const U32 InjectNum, void * const Read)
{
    VIDINJ_Data_t * VIDINJ_Data_p = (VIDINJ_Data_t *) InjecterHandle;
    TEST_INJECT(InjectNum);

#if defined(TRACE_UART)
    if((U32) Read > (U32) VIDINJ_Data_p->DMAInjecters[InjectNum]->ESCopy_Top_p)
    {
        TrError(("\r\nINJECT ERROR: ESCopy Buffer Over the top! Read=0x%08x > ESCopy_Top=0x%08x",
                                Read, (U32) VIDINJ_Data_p->DMAInjecters[InjectNum]->ESCopy_Top_p));
    }
#endif

    VIDINJ_Data_p->DMAInjecters[InjectNum]->ESCopy_Read_p = Read;
    /*TrESCopy(("\r\nINJECT: TransferSetESCopyRead: 0x%x", (U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->ESCopy_Read_p));*/
} /* end of VIDINJ_TransferSetESCopyBufferRead() function */
#endif /* DVD_SECURED_CHIP */

/*******************************************************************************
Name        : VIDINJ_TransferSetSCListRead
Description : video calls this function to update the read pointer in SC List
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
void VIDINJ_TransferSetSCListRead(const VIDINJ_Handle_t InjecterHandle, const U32 InjectNum, void * const Read)
{
    VIDINJ_Data_t * VIDINJ_Data_p = (VIDINJ_Data_t *) InjecterHandle;
    TEST_INJECT(InjectNum);

#if defined(TRACE_UART)
    if((U32) Read > (U32) VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListTop_p)
    {
        TrError(("\r\nINJECT ERROR: SCList Over the top!"));
    }
#endif

    VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListRead_p = Read;
    /*TrSCList(("\r\nTINJECT: TransferSetSCListRead: 0x%x", Read));*/
} /* end of VIDINJ_TransferSetSCListRead() function */

/* Static functions */
/*------------------*/
/*******************************************************************************
Name        : InjecterTaskFunc
Description : Injection task that manages the transfers between shared PES buffer
			  (also called InputBuffer) and video bit buffer.
Parameters  : InjecterHandle : Handle of injecter instance.
Assumptions :
Limitations : The fct is a task body
Returns     : N/A
*******************************************************************************/
static void InjecterTaskFunc(const VIDINJ_Handle_t InjecterHandle)
{
    VIDINJ_Data_t * VIDINJ_Data_p = (VIDINJ_Data_t *) InjecterHandle;
    U32             InjectNum;
    STOS_Clock_t       ThisLoopInitialTime;
    STOS_Clock_t       NearestInjectionTime;
    BOOL            IsNewTransfer = FALSE;
    void*           Write_p;
    ST_ErrorCode_t  ErrorCode;

    STOS_TaskEnter(NULL);

    do
    { /* while not ToBeDeleted */
        /* take the time before the cycle of injection */
        ThisLoopInitialTime     = STOS_time_now();
#if !defined(VIDINJ_LIBERATE_CONTEXT_AFTER_EACH_FDMA_TRANSFER)
        STOS_SemaphoreWait(VIDINJ_Data_p->ContextSemaphore_p);
#endif /* VIDINJ_LIBERATE_CONTEXT_AFTER_EACH_FDMA_TRANSFER */
        /* the latest time for next injection is now + MS_TO_NEXT_TRANSFER_FOR_REALTIME_INJECTION */
        NearestInjectionTime    = STOS_time_plus(ThisLoopInitialTime, VIDINJ_Data_p->TransferPeriod);
        /* Check the action to do with the different instances */
        for(InjectNum = 0; InjectNum < MAX_INJECTERS; InjectNum ++)
        {
#if defined(VIDINJ_LIBERATE_CONTEXT_AFTER_EACH_FDMA_TRANSFER)
            STOS_SemaphoreWait(VIDINJ_Data_p->ContextSemaphore_p);
#endif /* VIDINJ_LIBERATE_CONTEXT_AFTER_EACH_FDMA_TRANSFER */
            if(VIDINJ_Data_p->DMAInjecters[InjectNum] != NULL)
            {
                /* Check if current instance needs to make an FDMA transfer */
                VIDINJ_Data_p->DMAInjecters[InjectNum]->LoopsBeforeNextTransfer--;
                if(VIDINJ_Data_p->DMAInjecters[InjectNum]->LoopsBeforeNextTransfer == 0)
                {
                    /* Reset the loops counter for next transfer */
                    VIDINJ_Data_p->DMAInjecters[InjectNum]->LoopsBeforeNextTransfer = VIDINJ_Data_p->DMAInjecters[InjectNum]->MaxLoopsBeforeOneTransfer;
                    switch(VIDINJ_Data_p->DMAInjecters[InjectNum]->TransferState)
                    {
                        case STOPPED:
                            /* Injecter is stopped : inform application that read pointer is always equal to write pointer */

                            /* Get the write pointer */
                            if(VIDINJ_Data_p->DMAInjecters[InjectNum]->GetWriteAddress != NULL)
                            {
                                ErrorCode = VIDINJ_Data_p->DMAInjecters[InjectNum]->GetWriteAddress(VIDINJ_Data_p->DMAInjecters[InjectNum]->ExtHandle, &Write_p);
                                if(ErrorCode != ST_NO_ERROR)
                                {
                                    Write_p = VIDINJ_Data_p->DMAInjecters[InjectNum]->InputWrite_p;
                                }
                            }
                            else
                            {
                                Write_p = VIDINJ_Data_p->DMAInjecters[InjectNum]->InputWrite_p;
                            }
                            /* Avoid having a Write_p > InputTop_p */
                            if((U32)Write_p > (U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->InputTop_p)
                            {
                                Write_p = VIDINJ_Data_p->DMAInjecters[InjectNum]->InputBase_p;
                            }
                            /* Inform the application that we have read all the data until the write pointer */
                            if(VIDINJ_Data_p->DMAInjecters[InjectNum]->InformReadAddress != NULL)
                            {
                                VIDINJ_Data_p->DMAInjecters[InjectNum]->InputRead_p  = (void*)(((U32)Write_p) & ALIGNEMENT_FOR_TRANSFERS);
                                OutInformReadAddress(VIDINJ_Data_p->DMAInjecters[InjectNum]->ExtHandle, VIDINJ_Data_p->DMAInjecters[InjectNum]->InputRead_p);
                            }
                            break;
                        case RUNNING:
                            /* Injecter is running : do an aligned transfer */

                            /* Do the aligned transfer */
                            IsNewTransfer = DMATransferOneSlice(InjecterHandle, InjectNum);
                            /* inform hal decode about the new data in the bitbuffer */
                            if (IsNewTransfer)
                            {
                                VIDINJ_Data_p->DMAInjecters[InjectNum]->UpdateHalDecodeFct(
                                    VIDINJ_Data_p->DMAInjecters[InjectNum]->HalDecodeIdent,
                                    VIDINJ_Data_p->DMAInjecters[InjectNum]->ES_Write_p,
                                    VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListWrite_p,
                                    VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListLoop_p
#if defined(DVD_SECURED_CHIP)
                                    ,VIDINJ_Data_p->DMAInjecters[InjectNum]->ESCopy_Write_p
#endif /* DVD_SECURED_CHIP */
                                    );
                            }
                            break;
                        default:
                            break;
                    }
                }
            }
#if defined(VIDINJ_LIBERATE_CONTEXT_AFTER_EACH_FDMA_TRANSFER)
            STOS_SemaphoreSignal(VIDINJ_Data_p->ContextSemaphore_p);
#endif /* VIDINJ_LIBERATE_CONTEXT_AFTER_EACH_FDMA_TRANSFER */
        }
#if !defined(VIDINJ_LIBERATE_CONTEXT_AFTER_EACH_FDMA_TRANSFER)
        STOS_SemaphoreSignal(VIDINJ_Data_p->ContextSemaphore_p);
#endif /* VIDINJ_LIBERATE_CONTEXT_AFTER_EACH_FDMA_TRANSFER */

        /* time:  aaaaaabbbbbb............aaaaaabbb  (a,b :transfers) */
        /*        ^                       ^          (.   :delay    ) */
        /*        |                       |                           */
        /*   InititialTime        NearestInjectionTime                */
        /*        |<--------period------->|                           */
        /* Case there's no injecter configured yet.                   */
        STOS_TaskDelayUntil(NearestInjectionTime);
    } while (!(VIDINJ_Data_p->InjectionTask.ToBeDeleted));

    STOS_TaskExit(NULL);

} /* end of InjecterTaskFunc() function */


/*******************************************************************************
Name        : DMAFlushInjecter
Description : Manage the flush of input buffer and start code discontinuity
              insertion with following sequence :
                - Insert Discontinuity start code in SCList.
                - Update SCList
                - Update ES buffer informations.
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void DMAFlushInjecter(const VIDINJ_Handle_t InjecterHandle, const U32 InjectNum, const BOOL  DiscontinuityStartcode, const U8 * Pattern, const U8 PatternSize)
{
    VIDINJ_Data_t * VIDINJ_Data_p = (VIDINJ_Data_t *) InjecterHandle;
    STFDMA_TransferGenericParams_t  TransferParams;
    ST_ErrorCode_t                  Err;
    U32                             Size = 0;
    BOOL                            Overflow;
    U8                              BytesToFlush;
    void*                           PESWrite_p = NULL;
    void*                           SCListRead_p;

    /* Get the startcode list read pointer */
    SCListRead_p = VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListRead_p;

    /* Get the write pointer */
    if(VIDINJ_Data_p->DMAInjecters[InjectNum]->GetWriteAddress != NULL)
    {
        Err = VIDINJ_Data_p->DMAInjecters[InjectNum]->GetWriteAddress(VIDINJ_Data_p->DMAInjecters[InjectNum]->ExtHandle, &PESWrite_p);
        if(Err != ST_NO_ERROR)
        {
            PESWrite_p = VIDINJ_Data_p->DMAInjecters[InjectNum]->InputWrite_p;
        }
    }
    else
    {
        PESWrite_p = VIDINJ_Data_p->DMAInjecters[InjectNum]->InputWrite_p;
    }

    /* Avoid having a PESWrite_p > InputTop_p */
    if((U32)PESWrite_p > (U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->InputTop_p)
    {
        PESWrite_p = VIDINJ_Data_p->DMAInjecters[InjectNum]->InputBase_p;
    }

#ifdef TRACE_UART
    if(((U32)PESWrite_p & ALIGNEMENT_FOR_TRANSFERS) != (U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->InputRead_p)
    {
        TrBuffers(("Flush ERROR !!!!!!!\r\n"));
    }
#endif /* TRACE_UART */

    /* Fill all the CDFIFO with 0xFF pattern. */
    STOS_memsetUncached(VIDINJ_Data_p->DMAInjecters[InjectNum]->FlushBuffer_p, 0xFF, FLUSH_DATA_SIZE);
    /* Get the number of bytes pending in the inputbuffer */
    BytesToFlush = (U32)PESWrite_p - ((U32)PESWrite_p & ALIGNEMENT_FOR_TRANSFERS);
    /* Copy the bytes to flush to the emulated CD-FIFO */
    STOS_memcpyUncachedToUncached((void*)VIDINJ_Data_p->DMAInjecters[InjectNum]->FlushBuffer_p, (void*)((U32)PESWrite_p & ALIGNEMENT_FOR_TRANSFERS), BytesToFlush);

    /* Add the discontinuity pattern */
    if(DiscontinuityStartcode)
    {
        STOS_memcpyCachedToUncached((void*)((U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->FlushBuffer_p + BytesToFlush), Pattern, PatternSize);
    }

    /* Prepare transfer of CD-FIFO */
    VIDINJ_Data_p->DMAInjecters[InjectNum]->Node_p->ContextNode.Next_p          = NULL;
    VIDINJ_Data_p->DMAInjecters[InjectNum]->Node_p->ContextNode.NumberBytes     = FLUSH_DATA_SIZE;
    VIDINJ_Data_p->DMAInjecters[InjectNum]->Node_p->ContextNode.SourceAddress_p = VIDINJ_Data_p->DMAInjecters[InjectNum]->FlushBuffer_p;

    /* Uncache the node content to allow access from FDMA */
#ifdef ST_OS20
    STOS_memcpyCachedToUncached((void*)STAVMEM_CPUToDevice(VIDINJ_Data_p->DMAInjecters[InjectNum]->Node_p, STAVMEM_VIRTUAL_MAPPING_AUTO_P),
                                (void*)VIDINJ_Data_p->DMAInjecters[InjectNum]->Node_p,
                                sizeof(STFDMA_ContextNode_t));
#else
    STOS_FlushRegion( STOS_MapPhysToCached(VIDINJ_Data_p->DMAInjecters[InjectNum]->Node_p, sizeof(STFDMA_ContextNode_t)), sizeof(STFDMA_ContextNode_t) );
#endif
    /* Fill the transfer parameters */
    TransferParams.FDMABlock        = VIDINJ_Data_p->FDMABlock;
    TransferParams.ChannelId        = VIDINJ_Data_p->FDMAChannel;
    TransferParams.Pool             = STFDMA_PES_POOL;
    TransferParams.NodeAddress_p    = STAVMEM_CPUToDevice(VIDINJ_Data_p->DMAInjecters[InjectNum]->Node_p, STAVMEM_VIRTUAL_MAPPING_AUTO_P);
    TransferParams.BlockingTimeout  = 0;
    TransferParams.CallbackFunction = NULL;
    TransferParams.ApplicationData_p= &VIDINJ_Data_p->DMAInjecters[InjectNum];

#if defined(STFDMA_USE_VIRTUAL_CONTEXT)
    /* Provide the FDMA context of this injecter to the FDMA module */
    Err = STFDMA_VirtualContextRestore( VIDINJ_Data_p->DMAInjecters[InjectNum]->VirtualFDMAContextId,
                                        VIDINJ_Data_p->FDMAContextId);
    if(Err != ST_NO_ERROR)
    {
        return;
    }
#endif /* STFDMA_USE_VIRTUAL_CONTEXT */

    /* Do the transfer(blocking) */
    Err = STFDMA_StartGenericTransfer(&TransferParams, &VIDINJ_Data_p->DMAInjecters[InjectNum]->FDMATransferId);
    if(Err != ST_NO_ERROR)
    {
        return;
    }

#if defined(STFDMA_USE_VIRTUAL_CONTEXT)
    /* Save the FDMA context of this injecter */
    Err = STFDMA_VirtualContextSave( VIDINJ_Data_p->FDMAContextId,
                                     VIDINJ_Data_p->DMAInjecters[InjectNum]->VirtualFDMAContextId);
    if(Err != ST_NO_ERROR)
    {
        return;
    }
#endif /* STFDMA_USE_VIRTUAL_CONTEXT */

    /* Fill the flushed data in the input buffer with 0xFF */
    STOS_memsetUncached((void*)((U32)PESWrite_p & ALIGNEMENT_FOR_TRANSFERS), 0xFF, BytesToFlush);

    /* Update the dest buffers */
    /*-------------------------*/
#if !defined(STFDMA_USE_VIRTUAL_CONTEXT)

    STFDMA_ContextGetESWritePtr(VIDINJ_Data_p->DMAInjecters[InjectNum]->FDMAContext,
                               &VIDINJ_Data_p->DMAInjecters[InjectNum]->ES_Write_p,
                               &Overflow);

#else /* STFDMA_USE_VIRTUAL_CONTEXT */
    STFDMA_VirtualContextGetESWritePtr(VIDINJ_Data_p->DMAInjecters[InjectNum]->VirtualFDMAContextId,
                                      &VIDINJ_Data_p->DMAInjecters[InjectNum]->ES_Write_p,
                                      &Overflow);
#endif /* STFDMA_USE_VIRTUAL_CONTEXT */

    if (Overflow)
    {
        TrError(("\r\nINJECT ERROR: ES TRANSFER OVERFLOW ERROR !!!!!!!"));
    }

    /* Update the SC-List */
    /*--------------------*/
#if !defined(STFDMA_USE_VIRTUAL_CONTEXT)
    STFDMA_ContextGetSCList(VIDINJ_Data_p->DMAInjecters[InjectNum]->FDMAContext,
                            (STFDMA_SCEntry_t**)&(VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListWrite_p),
                            &Size,
                            &Overflow);

#else /* STFDMA_USE_VIRTUAL_CONTEXT */
    STFDMA_VirtualContextGetSCList(VIDINJ_Data_p->DMAInjecters[InjectNum]->VirtualFDMAContextId,
                                  (STFDMA_SCEntry_t**)&(VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListWrite_p),
                                  &Size,
                                  &Overflow);
#endif /* STFDMA_USE_VIRTUAL_CONTEXT */

    if (Overflow)
    {
        TrError(("\r\nINJECT ERROR: SCLIST TRANSFER OVERFLOW ERROR !!!!!!!"));
    }

    if(  ((Size < SCLIST_MIN_SIZE) || (Overflow))
       &&((U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListWrite_p >= (U32)SCListRead_p))
    {
#ifdef TRACE_UART
            if (!Overflow)
            {
                TrSCList(("INJECT: Looping Startcode list\r\n"));
        }
#endif /* TRACE_UART */

        /* not enough place to concat the next sc-list : do a loop : */
        VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListLoop_p   = VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListWrite_p;
        VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListWrite_p  = VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListBase_p;
    }

    /* Test if loop-ptr must be pushed */
    if(VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListWrite_p > VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListLoop_p)
    {
        VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListLoop_p = VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListWrite_p;
    }

#if defined(DVD_SECURED_CHIP)
    #if !defined(STFDMA_USE_VIRTUAL_CONTEXT)
        STFDMA_ContextGetHeaderCopyBufferWritePtr(VIDINJ_Data_p->DMAInjecters[InjectNum]->FDMAContext,
                                                  &VIDINJ_Data_p->DMAInjecters[InjectNum]->ESCopy_Write_p,
                                                  &Overflow);
    #else /* STFDMA_USE_VIRTUAL_CONTEXT */
        STFDMA_VirtualContextGetHeaderCopyBufferWritePtr(VIDINJ_Data_p->DMAInjecters[InjectNum]->VirtualFDMAContextId,
                                                         &VIDINJ_Data_p->DMAInjecters[InjectNum]->ESCopy_Write_p,
                                                         &Overflow);
    #endif /* STFDMA_USE_VIRTUAL_CONTEXT */

    if (Overflow)
    {
        TrError(("\r\nINJECT ERROR: ES COPY TRANSFER OVERFLOW ERROR !!!!!!!"));
    }
#endif /* DVD_SECURED_CHIP */
} /* End of DMAFlushInjecter() function. */

/*******************************************************************************
Name        : DMATransferOneSlice
Description :
Parameters  :
Assumptions : To be called each 'MS_TO_NEXT_TRANSFER' millisec.
Limitations :
Returns     : TRUE if transfer was successful
              FALSE if transfer failed
*******************************************************************************/
static BOOL DMATransferOneSlice(const VIDINJ_Handle_t InjecterHandle, const U32 InjectNum)
{
    VIDINJ_Data_t * VIDINJ_Data_p = (VIDINJ_Data_t *) InjecterHandle;
    STFDMA_TransferGenericParams_t  TransferParams;
    ST_ErrorCode_t                  Err;
    U32                             Size;
    U32                             RemainingFreeSpace;
    U32                             FreeSpaceInBitBuffer;       /* ES */
    U32                             UsedSpaceInSrc;  /* PES */
    BOOL                            Overflow;
    void*                           SCListRead_p;
    void*                           ES_Read_p;
    void*                           PostTransferRead_p;
    void*                           PESWrite_p = VIDINJ_INVALID_ADDRESS;
#if defined(DVD_SECURED_CHIP)
    void*   ESCopy_Read_p;
    U32     ESCopyFreeSize;
#endif /* DVD_SECURED_CHIP */

#if defined(TRACE_INJECT)
    BOOL            TraceSCListOnDisplay = TRUE;
    static void*    TempSCListRead_p = 0;
    static void*    TempSCListWrite_p = 0;
    static void*    TempESRead_p = 0;
    static void*    TempESWrite_p = 0;
#if defined(DVD_SECURED_CHIP)
    BOOL            TraceESCopyOnDisplay = TRUE;
    static void*    TempESCopyRead_p = 0;
    static void*    TempESCopyWrite_p = 0;
#endif /* DVD_SECURED_CHIP */
#endif /* TRACE_INJECT */

#if defined (WINCE_TRACE_ENABLED) || defined (CHECK_FDMA_INJECT_RESULT)
    void* SCListWrite_p = 0;
    void* ESCopyWrite_p = 0;
    void* ESWrite_p     = 0;
    static int nbCalls = 0;
    static int nbTimes = 0;

   void* InputRead_p   = 0;
   void* InputWrite_p  = 0;
   U32   Cnt           = 0;
   U8    Pattern[]     = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77 };
   U8*   DataBase_p    = (U8*)(0xA0000000 + (U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->ESCopy_Top_p);
#endif

#if defined(CHECK_FDMA_INJECT_RESULT)
   static U32 NumberOfFDMAPerformed = 0;
   static U32 NumberOfFDMAErrors = 0;
   BOOL   FDMAErrorOccured = FALSE;
#endif /* CHECK_FDMA_INJECT_RESULT */
#if defined (WINCE_TRACE_ENABLED)
    WINCE_TRACE(("DMATransferOneSlice begin -----------------------------------------\n"));
    WINCE_TRACE(("Input: Base=0x%.8X Top=0x%.8X R=0x%.8X W=0x%.8X Size=%d\n",
       VIDINJ_Data_p->DMAInjecters[InjectNum]->InputBase_p,
       VIDINJ_Data_p->DMAInjecters[InjectNum]->InputTop_p,
       VIDINJ_Data_p->DMAInjecters[InjectNum]->InputRead_p,
       VIDINJ_Data_p->DMAInjecters[InjectNum]->InputWrite_p,
       (U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->InputTop_p - (U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->InputBase_p));
#endif

#if defined (WINCE_TRACE_ENABLED) || defined (CHECK_FDMA_INJECT_RESULT)
    /* Save the current pointers */
    SCListWrite_p  = VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListWrite_p;
    ESCopyWrite_p  = VIDINJ_Data_p->DMAInjecters[InjectNum]->ESCopy_Write_p;
    ESWrite_p      = VIDINJ_Data_p->DMAInjecters[InjectNum]->ES_Write_p;
    ESCopy_Read_p  = VIDINJ_Data_p->DMAInjecters[InjectNum]->ESCopy_Read_p;
    InputRead_p    = VIDINJ_Data_p->DMAInjecters[InjectNum]->InputRead_p;
    InputWrite_p   = VIDINJ_Data_p->DMAInjecters[InjectNum]->InputWrite_p;

    /* Write pattern after end of ESCopy buffer */
    for(Cnt=0; Cnt<8; Cnt++)
    {
       DataBase_p[Cnt] = Pattern[Cnt];
    }
    for(Cnt=0; Cnt<8; Cnt++)
    {
       if(DataBase_p[Cnt] != Pattern[Cnt])
       {
          TrFDMAError(("*** ESCopy pattern check : ESCopy_Top_p=0x%.8X, Add=%.8X, Found=0x%.2X, Expected=0x%.2X\n",
                       VIDINJ_Data_p->DMAInjecters[InjectNum]->ESCopy_Top_p,
                       &DataBase_p[Cnt],
                       DataBase_p[Cnt],
                       Pattern[Cnt]));
          FDMAErrorOccured = TRUE;
       }
    }
#endif

    /********************
     ***    SCLIST    ***
     ********************/

    /**************************************************************************************************/
    /* Copy SCListRead_p in local variables in order to work with constant values within the function */
    /**************************************************************************************************/
    SCListRead_p = VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListRead_p;

    /*****************************************************/
    /* Compute the remaining space in the startcode list */
    /*****************************************************/
    if ((U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListWrite_p >= (U32)SCListRead_p)
    {
        Size = (U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListTop_p - (U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListWrite_p + sizeof(STFDMA_SCEntry_t);
    }
    else
    {
        Size = (U32)SCListRead_p - (U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListWrite_p;
        /* Try to always leave a SC entry empty between W and R to be sure that when W=R we are in the first case
        of the condition on the previous lines */
        if(Size > sizeof(STFDMA_SCEntry_t))
        {
            Size -= sizeof(STFDMA_SCEntry_t);
        }
        else
        {
            Size = 0;
        }
        #if defined (WINCE_TRACE_ENABLED)
            WINCE_TRACE(("SCList buffer looped\n"));
        #endif
    }

#ifdef TRACE_INJECT
    if (TempSCListRead_p != SCListRead_p)
    {
        TraceSCListOnDisplay = TRUE;
        TempSCListRead_p = SCListRead_p;
        TrSCList(("\r\nINJECT: SCL R->0x%08x Size=%d W->0x%08x", TempSCListRead_p, Size, VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListWrite_p));
    }
    else
    {
        TraceSCListOnDisplay = FALSE;
    }

    if (TempSCListWrite_p != VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListWrite_p)
    {
        TempSCListWrite_p = VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListWrite_p;
        TrSCListWrite(("\r\nINJECT: SCL W->0x%08x", TempSCListWrite_p));
    }
#endif  /* #ifdef TRACE_INJECT */

#if defined (WINCE_TRACE_ENABLED)
    WINCE_TRACE(("SCList: Base=0x%.8X Top=0x%.8X R=0x%.8X W=0x%.8X Loop=0x%.8X RemainSize=%d MinSize=%d\n",
       VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListBase_p,
       VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListTop_p,
       VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListRead_p,
       VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListWrite_p,
       VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListLoop_p,
       Size, SCLIST_MIN_SIZE));
#endif

    /************************/
    /* Set the SCList size  */
    /************************/
    #if !defined(STFDMA_USE_VIRTUAL_CONTEXT)
        STFDMA_ContextSetSCList(VIDINJ_Data_p->DMAInjecters[InjectNum]->FDMAContext,
                                VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListWrite_p,
                                Size);

    #else /* STFDMA_USE_VIRTUAL_CONTEXT */
        STFDMA_VirtualContextSetSCList(VIDINJ_Data_p->DMAInjecters[InjectNum]->VirtualFDMAContextId,
                                       VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListWrite_p,
                                       Size);

    #endif /* STFDMA_USE_VIRTUAL_CONTEXT */

    /**********************************************************/
    /* Look at our destination before launching a transfer !! */
    /**********************************************************/
    if(Size < SCLIST_MIN_SIZE)
    {
        #ifdef TRACE_INJECT
            if (TraceSCListOnDisplay)
            {
                TrSCList(("\r\nINJECT: No Transfer : SCListSize=%d < SCLIST_MIN_SIZE(=%d)", Size, SCLIST_MIN_SIZE));
                TrSCList(("W->0x%08x R->0x%08x \r\n",VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListWrite_p, SCListRead_p));
            }
        #endif  /* #ifdef TRACE_INJECT */

        #if defined (WINCE_TRACE_ENABLED)
            WINCE_TRACE(("SCListSize < SCLIST_MIN_SIZE => skipping transfer\n"));
            WINCE_TRACE(("DMATransferOneSlice end -------------------------------------------\n"));
        #endif

        /* We prefer skipping this transfer */
        return(FALSE);
    }
    /* else, we are sure ('almost sure !') to have enough room in SC-List */


    /*************************************
     ***   PES buffer (Input Buffer)   ***
     *************************************/

    /******************************/
    /* Get the InputWrite pointer */
    /******************************/
    if(VIDINJ_Data_p->DMAInjecters[InjectNum]->GetWriteAddress != NULL)
    {
        Err = VIDINJ_Data_p->DMAInjecters[InjectNum]->GetWriteAddress(VIDINJ_Data_p->DMAInjecters[InjectNum]->ExtHandle, &PESWrite_p);

        if(Err != ST_NO_ERROR)
        {
            PESWrite_p = VIDINJ_Data_p->DMAInjecters[InjectNum]->InputWrite_p;
        }
    }
    else
    {
        PESWrite_p = VIDINJ_Data_p->DMAInjecters[InjectNum]->InputWrite_p;
    }

    /******************************************/
    /* Avoid having a PESWrite_p > InputTop_p */
    /******************************************/
    if((U32)PESWrite_p > (U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->InputTop_p)
    {
        PESWrite_p = VIDINJ_Data_p->DMAInjecters[InjectNum]->InputBase_p;
    }

    /* Get the info from injection: (interface object) */
    PESWrite_p = (void *)((U32)PESWrite_p & ALIGNEMENT_FOR_TRANSFERS);

    /**************************************/
    /* Compute space taken in PES buffer  */
    /**************************************/
    if( PESWrite_p >= VIDINJ_Data_p->DMAInjecters[InjectNum]->InputRead_p)
    {
        /* normal case */
        UsedSpaceInSrc  = (U32)PESWrite_p - (U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->InputRead_p;
    }
    else /* case loop in input : base <= w < r <= top */
    {
        UsedSpaceInSrc  = (U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->InputTop_p
                            - (U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->InputRead_p
                            + 1
                            + (U32)PESWrite_p
                            - (U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->InputBase_p;
    }


    /**********************************
     *    ES buffer (Bit Buffer)    ***
     **********************************/

    /****************************************************************************************************************/
    /* Store ES_Read_p and SCLISTRead_p in local variables in order to work with contant values within the function */
    /****************************************************************************************************************/
    ES_Read_p = VIDINJ_Data_p->DMAInjecters[InjectNum]->ES_Read_p;

    /***********************************************/
    /* Set the ES read pointer  */
    /***********************************************/
    #if !defined(STFDMA_USE_VIRTUAL_CONTEXT)
        STFDMA_ContextSetESReadPtr(VIDINJ_Data_p->DMAInjecters[InjectNum]->FDMAContext, ES_Read_p);
    #else /* STFDMA_USE_VIRTUAL_CONTEXT */
        STFDMA_VirtualContextSetESReadPtr(VIDINJ_Data_p->DMAInjecters[InjectNum]->VirtualFDMAContextId, ES_Read_p);
    #endif /* STFDMA_USE_VIRTUAL_CONTEXT */

    /*****************************************************/
    /* Compute the remaining space in the ES Buffer */
    /*****************************************************/
    if(VIDINJ_Data_p->DMAInjecters[InjectNum]->ES_Write_p < ES_Read_p)
    {
        /* Write pointer has done a loop, not Read pointer */
        FreeSpaceInBitBuffer = (U32)ES_Read_p
                        - 1 /* There is one byte less than diff of pointers, as if (ES_Write_p == ES_Read_p - 1) this means no more free space */
                        - (U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->ES_Write_p;

        #if defined (WINCE_MSPP_INJECT_TRACE)
            WINCE_TRACE(("BitBuffer write pointer wrapped\n"));
        #endif
        TrWrap(("\r\nINJECT: BitBuffer looped!"));
    }
    else
    {
        /* normal : base <= r <= w <= top */
        FreeSpaceInBitBuffer = (U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->ES_Top_p
                        - (U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->ES_Write_p
                        /* NOT + 1 : buffer can only contain (ES_Top_p - ES_Base_p), not +1 as this means empty ! */
                        + (U32)ES_Read_p
                        - (U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->ES_Base_p;
    }

    /* Temporary workaround to avoid considering a full buffer(Read=Write) as empty */
    if(FreeSpaceInBitBuffer >= FDMA_MEMORY_BUFFER_ALIGNMENT)
    {
        FreeSpaceInBitBuffer -= FDMA_MEMORY_BUFFER_ALIGNMENT;
    }
    else
    {
        FreeSpaceInBitBuffer = 0;
    }

    #ifdef TRACE_INJECT
        if (TempESRead_p != ES_Read_p)
        {
            TempESRead_p = ES_Read_p;
            TrESBuffer(("\r\nINJECT: ES R->0x%08x ", (U32)TempESRead_p));
        }

        if (TempESWrite_p != VIDINJ_Data_p->DMAInjecters[InjectNum]->ES_Write_p)
        {
            TempESWrite_p = VIDINJ_Data_p->DMAInjecters[InjectNum]->ES_Write_p;
            TrESBuffer(("\r\nINJECT: ES W->0x%08x, RemainingSize=%u", (U32)TempESWrite_p, FreeSpaceInBitBuffer));
        }
    #endif  /* #ifdef TRACE_INJECT */

    /********************************************/
    /* Reduce the FDMA transfer size if needed  */
    /********************************************/
#if defined (CHECK_FDMA_INJECT_RESULT)
    /* Store the current write pointer of input buffer */
    /*VIDINJ_Data_p->DMAInjecters[InjectNum]->InputWrite_p = (void*)PESWrite_p;*/
    InputWrite_p = (void*)PESWrite_p;
#endif /* CHECK_FDMA_INJECT_RESULT */

    if(FreeSpaceInBitBuffer < UsedSpaceInSrc)
    {
        UsedSpaceInSrc = FreeSpaceInBitBuffer & ALIGNEMENT_FOR_TRANSFERS;
        #ifdef TRACE_UART
            if(UsedSpaceInSrc == 0)
            {
                TrBuffers(("No Transfer : No Free space in ES buffer !\r\n"));
            }
        #endif
    }

#if defined(DVD_SECURED_CHIP)

    /***************************
     ***    ESCopy buffer    ***
     ***************************/

    /* Make local copy of ESCopy_Read_p pointer in order to work with a constant value (as the parser
       continues reading data and updating the ESCopy_Read_p while we are injecting) */
    ESCopy_Read_p = VIDINJ_Data_p->DMAInjecters[InjectNum]->ESCopy_Read_p;

    /***********************************************/
    /* Set the ESCopy read pointer                 */
    /***********************************************/
    #if !defined(STFDMA_USE_VIRTUAL_CONTEXT)
        STFDMA_ContextSetHeaderCopyBufferReadPtr(VIDINJ_Data_p->DMAInjecters[InjectNum]->FDMAContext, ESCopy_Read_p);
    #else /* STFDMA_USE_VIRTUAL_CONTEXT */
        STFDMA_VirtualContextSetHeaderCopyBufferReadPtr(VIDINJ_Data_p->DMAInjecters[InjectNum]->VirtualFDMAContextId, ESCopy_Read_p);
    #endif /* STFDMA_USE_VIRTUAL_CONTEXT */

    /* Compute the available free space in ESCopy buffer */
    if ((U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->ESCopy_Write_p >= (U32)ESCopy_Read_p)
    {
        /* base <= r <= w <= top */
        ESCopyFreeSize = (  (U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->ESCopy_Top_p
                            - (U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->ESCopy_Write_p)
                           +
                           (  (U32)ESCopy_Read_p
                            - (U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->ESCopy_Base_p);
    }
    else
    {
        /* Try to always leave a byte empty between W and R to be sure that when W=R we are in */
        /* the first case of the condition on the previous lines */
        ESCopyFreeSize = (U32)ESCopy_Read_p
                           - 1
                           - (U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->ESCopy_Write_p;

        TrWrap(("\r\nINJECT: ESCopy buffer looped!"));
    }

    /* Temporary workaround to avoid considering a full buffer(Read=Write) as empty */
    if(ESCopyFreeSize >= FDMA_MEMORY_BUFFER_ALIGNMENT)
    {
        ESCopyFreeSize -= FDMA_MEMORY_BUFFER_ALIGNMENT;
    }
    else
    {
        ESCopyFreeSize = 0;
    }

    /********************************************/
    /* Reduce the FDMA transfer size if needed  */
    /********************************************/
    if(ESCopyFreeSize < UsedSpaceInSrc)
    {
        UsedSpaceInSrc = ESCopyFreeSize & ALIGNEMENT_FOR_TRANSFERS;

        if(UsedSpaceInSrc == 0)
        {
            TrESCopy(("\r\nNo Transfer: No Free space in ES Copy buffer!!!!\r\n"));
        }
    }

    TrESCopy(("\r\nINJECT: ESCopy\tW->%d\tR->%d\tBitBuffer W->%d\tR->%d",
                    VIDINJ_Data_p->DMAInjecters[InjectNum]->ESCopy_Write_p, ESCopy_Read_p,
                    VIDINJ_Data_p->DMAInjecters[InjectNum]->ES_Write_p, ES_Read_p));
    #ifdef TRACE_INJECT
        if (TempESCopyRead_p != ESCopy_Read_p)
        {
            TraceESCopyOnDisplay = TRUE;
            TempESCopyRead_p = ESCopy_Read_p;
            /*TrESCopy(("\r\nINJECT: ESCopy R->0x%08x ", (U32)TempESCopyRead_p));*/
        }
        else
        {
            TraceESCopyOnDisplay = FALSE;
        }
        if (TempESCopyWrite_p != VIDINJ_Data_p->DMAInjecters[InjectNum]->ESCopy_Write_p)
        {
            TempESCopyWrite_p = VIDINJ_Data_p->DMAInjecters[InjectNum]->ESCopy_Write_p;
            TrESCopy(("\r\nINJECT: ESCopy W->0x%08x, R->0x%08x, Size=%u",
                        (U32)TempESCopyWrite_p, (U32)TempESCopyRead_p, ESCopyFreeSize));
        }
    #endif  /* #ifdef TRACE_INJECT */

    #if defined (WINCE_TRACE_ENABLED)
        WINCE_TRACE(("ESCopy: Base=0x%.8X Top=0x%.8X R=0x%.8X W=0x%.8X RemainSize=%d MinSize=%d\n",
           VIDINJ_Data_p->DMAInjecters[InjectNum]->ESCopy_Base_p,
           VIDINJ_Data_p->DMAInjecters[InjectNum]->ESCopy_Top_p,
           VIDINJ_Data_p->DMAInjecters[InjectNum]->ESCopy_Read_p,
           VIDINJ_Data_p->DMAInjecters[InjectNum]->ESCopy_Write_p,
           ESCopyFreeSize, ESCOPY_MIN_SIZE));
    #endif /* WINCE_TRACE_ENABLED */
#endif /* DVD_SECURED_CHIP */


    /*****************************************************************/
    /* Compute the new PES Read pointer position after this transfer */
    /*****************************************************************/
    PostTransferRead_p = (void*)((U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->InputRead_p + UsedSpaceInSrc);
    if(PostTransferRead_p > VIDINJ_Data_p->DMAInjecters[InjectNum]->InputTop_p)
    {
        PostTransferRead_p = (void *)
                           ((U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->InputBase_p
                           -(U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->InputTop_p
                           +(U32)PostTransferRead_p
                           - 1 );
    }
    TrPESBuffer(("\r\nInput: R=0x%.8X W=0x%.8X UsedSpace=%d\r\n",
       VIDINJ_Data_p->DMAInjecters[InjectNum]->InputRead_p,
       PESWrite_p,
       UsedSpaceInSrc));

    /*********************/
    /* and program nodes */
    /*********************/
    if(PostTransferRead_p == VIDINJ_Data_p->DMAInjecters[InjectNum]->InputRead_p)
    {
        /* This is equivalent to testing (UsedSpaceInSrc ==0) ! */
        /* nothing to transfer */
#if defined (WINCE_TRACE_ENABLED)
         WINCE_TRACE(("UsedSpaceInSrc=0 => no transfer\n"));
         WINCE_TRACE(("DMATransferOneSlice end -------------------------------------------\n"));
#endif
        return(FALSE);
    }
    else if(PostTransferRead_p > VIDINJ_Data_p->DMAInjecters[InjectNum]->InputRead_p)
    {
        /* PostTransferRead_p will not wrap, just one FDMA transfer to do */
        VIDINJ_Data_p->DMAInjecters[InjectNum]->Node_p->ContextNode.Next_p          = NULL;
        VIDINJ_Data_p->DMAInjecters[InjectNum]->Node_p->ContextNode.NumberBytes     = UsedSpaceInSrc;
        VIDINJ_Data_p->DMAInjecters[InjectNum]->Node_p->ContextNode.SourceAddress_p = VIDINJ_Data_p->DMAInjecters[InjectNum]->InputRead_p;

        /* Uncache the node content */
        #ifdef ST_OS20
            STOS_memcpyCachedToUncached((void*)STAVMEM_CPUToDevice(VIDINJ_Data_p->DMAInjecters[InjectNum]->Node_p, STAVMEM_VIRTUAL_MAPPING_AUTO_P),
                                        (void*)VIDINJ_Data_p->DMAInjecters[InjectNum]->Node_p,
                                        sizeof(STFDMA_ContextNode_t));
        #else
            STOS_FlushRegion( STOS_MapPhysToCached(VIDINJ_Data_p->DMAInjecters[InjectNum]->Node_p, sizeof(STFDMA_ContextNode_t) ), sizeof(STFDMA_ContextNode_t) );
        #endif

#if defined (WINCE_TRACE_ENABLED)
       WINCE_TRACE(("Node-1: Src=0x%.8X, NbBytes=%u (0x%.8X->0x%.8X)\n",
          VIDINJ_Data_p->DMAInjecters[InjectNum]->Node_p->ContextNode.SourceAddress_p,
          VIDINJ_Data_p->DMAInjecters[InjectNum]->Node_p->ContextNode.NumberBytes,
          VIDINJ_Data_p->DMAInjecters[InjectNum]->Node_p->ContextNode.SourceAddress_p,
          (U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->Node_p->ContextNode.SourceAddress_p
          +(U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->Node_p->ContextNode.NumberBytes));
#endif
    }
    else /* case of input loop */
    {
        /* PostTransferRead_p will wrap, need to do 2 FDMA transfers : one up to top, one from base */
        /* First node configuration */
        VIDINJ_Data_p->DMAInjecters[InjectNum]->Node_p->ContextNode.Next_p          = (STFDMA_ContextNode_t *) STAVMEM_CPUToDevice(&VIDINJ_Data_p->DMAInjecters[InjectNum]->NodeNext_p->ContextNode, STAVMEM_VIRTUAL_MAPPING_AUTO_P);
        VIDINJ_Data_p->DMAInjecters[InjectNum]->Node_p->ContextNode.NumberBytes     = (U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->InputTop_p
                                                                                      - (U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->InputRead_p
                                                                                      + 1;
        VIDINJ_Data_p->DMAInjecters[InjectNum]->Node_p->ContextNode.SourceAddress_p = VIDINJ_Data_p->DMAInjecters[InjectNum]->InputRead_p;

        /* Second node configuration */
        VIDINJ_Data_p->DMAInjecters[InjectNum]->NodeNext_p->ContextNode.Next_p          = NULL;
        VIDINJ_Data_p->DMAInjecters[InjectNum]->NodeNext_p->ContextNode.NumberBytes     = (U32)PostTransferRead_p
                                                                                          - (U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->InputBase_p;
        VIDINJ_Data_p->DMAInjecters[InjectNum]->NodeNext_p->ContextNode.SourceAddress_p = VIDINJ_Data_p->DMAInjecters[InjectNum]->InputBase_p;

        #if defined (WINCE_TRACE_ENABLED)
            WINCE_TRACE(("Node-1: Src=0x%.8X, NbBytes=%u (0x%.8X->0x%.8X)\n",
                  VIDINJ_Data_p->DMAInjecters[InjectNum]->Node_p->ContextNode.SourceAddress_p,
                  VIDINJ_Data_p->DMAInjecters[InjectNum]->Node_p->ContextNode.NumberBytes,
                  VIDINJ_Data_p->DMAInjecters[InjectNum]->Node_p->ContextNode.SourceAddress_p,
                  (U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->Node_p->ContextNode.SourceAddress_p
                  +(U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->Node_p->ContextNode.NumberBytes));
            WINCE_TRACE(("Node-2: Src=0x%.8X, NbBytes=%u (0x%.8X->0x%.8X)\n",
                  VIDINJ_Data_p->DMAInjecters[InjectNum]->NodeNext_p->ContextNode.SourceAddress_p,
                  VIDINJ_Data_p->DMAInjecters[InjectNum]->NodeNext_p->ContextNode.NumberBytes,
                  VIDINJ_Data_p->DMAInjecters[InjectNum]->NodeNext_p->ContextNode.SourceAddress_p,
                  (U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->NodeNext_p->ContextNode.SourceAddress_p
                  +(U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->NodeNext_p->ContextNode.NumberBytes));
        #endif /* WINCE_TRACE_ENABLED */

        if(VIDINJ_Data_p->DMAInjecters[InjectNum]->NodeNext_p->ContextNode.NumberBytes == 0)
        {
            VIDINJ_Data_p->DMAInjecters[InjectNum]->Node_p->ContextNode.Next_p = NULL;
            /* Uncache the node content */
            #ifdef ST_OS20
                STOS_memcpyCachedToUncached((void*)STAVMEM_CPUToDevice(VIDINJ_Data_p->DMAInjecters[InjectNum]->Node_p, STAVMEM_VIRTUAL_MAPPING_AUTO_P),
                                            (void*)VIDINJ_Data_p->DMAInjecters[InjectNum]->Node_p,
                                            sizeof(STFDMA_ContextNode_t));
            #else
                STOS_FlushRegion( STOS_MapPhysToCached(VIDINJ_Data_p->DMAInjecters[InjectNum]->Node_p, sizeof(STFDMA_ContextNode_t) ), sizeof(STFDMA_ContextNode_t) );
            #endif
        }
        else
        {
        	/* Uncache the nodes content */
            #ifdef ST_OS20
                STOS_memcpyCachedToUncached((void*)STAVMEM_CPUToDevice(VIDINJ_Data_p->DMAInjecters[InjectNum]->Node_p, STAVMEM_VIRTUAL_MAPPING_AUTO_P),
                                            (void*)VIDINJ_Data_p->DMAInjecters[InjectNum]->Node_p,
                                            sizeof(STFDMA_ContextNode_t));
                STOS_memcpyCachedToUncached((void*)STAVMEM_CPUToDevice(VIDINJ_Data_p->DMAInjecters[InjectNum]->NodeNext_p, STAVMEM_VIRTUAL_MAPPING_AUTO_P),
                                            (void*)VIDINJ_Data_p->DMAInjecters[InjectNum]->NodeNext_p,
                                            sizeof(STFDMA_ContextNode_t));
            #else
                STOS_FlushRegion( STOS_MapPhysToCached(VIDINJ_Data_p->DMAInjecters[InjectNum]->Node_p, sizeof(STFDMA_ContextNode_t) ), sizeof(STFDMA_ContextNode_t) );
                STOS_FlushRegion( STOS_MapPhysToCached(VIDINJ_Data_p->DMAInjecters[InjectNum]->NodeNext_p, sizeof(STFDMA_ContextNode_t) ), sizeof(STFDMA_ContextNode_t) );
            #endif
        }
    }

    /****************************/
    /* Fill transfer parameters */
    /****************************/
    TransferParams.FDMABlock        = VIDINJ_Data_p->FDMABlock;
    TransferParams.ChannelId        = VIDINJ_Data_p->FDMAChannel;
    TransferParams.Pool             = STFDMA_PES_POOL;
    TransferParams.NodeAddress_p    = STAVMEM_CPUToDevice(VIDINJ_Data_p->DMAInjecters[InjectNum]->Node_p, STAVMEM_VIRTUAL_MAPPING_AUTO_P);
    TransferParams.CallbackFunction = NULL;
    TransferParams.ApplicationData_p= &VIDINJ_Data_p->DMAInjecters[InjectNum];
    TransferParams.BlockingTimeout  = 0; /* blocking FDMA */

    #if defined(STFDMA_USE_VIRTUAL_CONTEXT)
        /* Provide the FDMA context of this injecter to the FDMA module */
        Err = STFDMA_VirtualContextRestore( VIDINJ_Data_p->DMAInjecters[InjectNum]->VirtualFDMAContextId,
											VIDINJ_Data_p->FDMAContextId   );
        if(Err != ST_NO_ERROR)
        {
            #if defined (WINCE_TRACE_ENABLED)
                WINCE_TRACE(("STFDMA_VirtualContextRestore failed\n"));
                WINCE_TRACE(("DMATransferOneSlice end -------------------------------------------\n"));
            #endif

            return FALSE;
        }
    #endif /* STFDMA_USE_VIRTUAL_CONTEXT */


    /*------------------------------------------------------------------------*/
    /* Launch FDMA transfer                                                   */
    /* We are in our own task and use the single 'pes' channel:               */
    /* -> We MUST use the blocking mode (callback not set), the function will */
    /* return at the end of the transfer.                                     */
    /* ... wait for transfer completion ...                                   */
    /*------------------------------------------------------------------------*/

    Err = STFDMA_StartGenericTransfer(&TransferParams, &VIDINJ_Data_p->DMAInjecters[InjectNum]->FDMATransferId);

    if(Err != ST_NO_ERROR)
    {
#if defined (WINCE_TRACE_ENABLED)
       WINCE_TRACE(("FDMA transfer failed\n"));
       WINCE_TRACE(("DMATransferOneSlice end -------------------------------------------\n"));
#endif
       return(FALSE);
    }

#if defined(CHECK_FDMA_INJECT_RESULT)
   NumberOfFDMAPerformed++;
#endif /* CHECK_FDMA_INJECT_RESULT */

#if defined (WINCE_TRACE_ENABLED)
    WINCE_TRACE(("FDMA transfer done\n"));
#endif

    #if defined(STFDMA_USE_VIRTUAL_CONTEXT)
        /* Save the FDMA context of this injecter */
        Err = STFDMA_VirtualContextSave( VIDINJ_Data_p->FDMAContextId,
                                         VIDINJ_Data_p->DMAInjecters[InjectNum]->VirtualFDMAContextId);
        if(Err != ST_NO_ERROR)
        {
#if defined (WINCE_TRACE_ENABLED)
            WINCE_TRACE(("STFDMA_VirtualContextSave failed\n"));
            WINCE_TRACE(("DMATransferOneSlice end -------------------------------------------\n"));
#endif
            return FALSE;
        }
    #endif /* STFDMA_USE_VIRTUAL_CONTEXT */

    /***************************************************************/
    /* Inform the world about our new read-add (maybe used by pti) */
    /***************************************************************/
    VIDINJ_Data_p->DMAInjecters[InjectNum]->InputRead_p = PostTransferRead_p;
    OutInformReadAddress(VIDINJ_Data_p->DMAInjecters[InjectNum]->ExtHandle, VIDINJ_Data_p->DMAInjecters[InjectNum]->InputRead_p);

/*    STTBX_Print(("InputRead:0x%.8x\n", (U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->InputRead_p));*/


    /******************************
     ***  Post transfer SCLIST  ***
     ******************************/

    #if !defined(STFDMA_USE_VIRTUAL_CONTEXT)
        STFDMA_ContextGetSCList(VIDINJ_Data_p->DMAInjecters[InjectNum]->FDMAContext,
                                (STFDMA_SCEntry_t**)&(VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListWrite_p),
                                &Size, &Overflow);
    #else /* STFDMA_USE_VIRTUAL_CONTEXT */
        STFDMA_VirtualContextGetSCList(VIDINJ_Data_p->DMAInjecters[InjectNum]->VirtualFDMAContextId,
                 (STFDMA_SCEntry_t**)&(VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListWrite_p),
                 &Size, &Overflow);
    #endif /* STFDMA_USE_VIRTUAL_CONTEXT */

    /* Recompute the remaining free space after the Write_p was updated in STFDMA_ContextGetSCList */
       RemainingFreeSpace =   (U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListTop_p
                            - (U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListWrite_p
                            + sizeof(STFDMA_SCEntry_t);

    if (RemainingFreeSpace != Size)
    {
        TrWarning(("\r\nINJECT WARNING!!! Size=%d and RemainingFreeSpace=%d are not equal", Size, RemainingFreeSpace));
    }

#if defined (WINCE_MSPP_INJECT_TRACE)
    WINCE_TRACE(("SCList new SCListWrite_p=0x%.8X (old=0x%.8X)\n", (U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListWrite_p, (U32)SCListWrite_p));
    if(Size!=RemainingFreeSpace)
       WINCE_TRACE(("SCList remaining free space does not match: expected %d, got %d\n", Size, RemainingFreeSpace));
#endif

#if defined (CHECK_FDMA_INJECT_RESULT)
    /* Check the new Write pointer */
    if(  (U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListWrite_p
       < (U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListBase_p)
    {
       TrFDMAError(("*** SCList bad new SC Write pointer: new-W=0x%.8X < B=0x%.8X\n",
          VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListWrite_p,
          VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListBase_p));
       FDMAErrorOccured = TRUE;
    }
    else if (  (U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListWrite_p
             > (U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListTop_p)
    {
       TrFDMAError(("*** SCList bad new SC Write pointer: new-W=0x%.8X > T=0x%.8X\n",
          VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListWrite_p,
          VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListTop_p));
       FDMAErrorOccured = TRUE;
    }
    else if(     ((U32)SCListWrite_p >= (U32)SCListRead_p)
              && ((U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListWrite_p >= (U32)SCListRead_p)
              && ((U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListWrite_p < (U32)SCListWrite_p))
    {
       TrFDMAError(("*** SCList bad new SC Write pointer: R=0x%.8X < new-W=0x%.8X < old-W=0x%.8X\n",
          SCListRead_p,
          VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListWrite_p,
          SCListWrite_p));
       FDMAErrorOccured = TRUE;
    }
    else if(     ((U32)SCListWrite_p < (U32)SCListRead_p)
              && ((U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListWrite_p >= (U32)SCListRead_p))
    {
       TrFDMAError(("*** SCList bad new SC Write pointer: old-W=0x%.8X < R=0x%.8X < new-W=0x%.8X\n",
          SCListWrite_p,
          SCListRead_p,
          VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListWrite_p));
       FDMAErrorOccured = TRUE;
    }
    else if(     ((U32)SCListWrite_p < (U32)SCListRead_p)
              && ((U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListWrite_p < (U32)SCListWrite_p))
    {
       TrFDMAError(("*** SCList bad new SC Write pointer: new-W=0x%.8X < old-W=0x%.8X < R=0x%.8X\n",
          VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListWrite_p,
          SCListWrite_p,
          SCListRead_p));
       FDMAErrorOccured = TRUE;
    }
#endif /* CHECK_FDMA_INJECT_RESULT */

    if( ((RemainingFreeSpace < SCLIST_MIN_SIZE) || (Overflow) )
        &&((U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListWrite_p >= (U32)SCListRead_p))
    {
        #ifdef TRACE_INJECT
        if (Overflow)
        {
                TrError(("\r\nINJECT ERROR: SCLIST TRANSFER OVERFLOW ERROR !!!!!!!\r\n"));
        }
        else
        {
                TrWrap(("\r\nINJECT: Looping Startcode list\r\n"));
        }
        #endif /* TRACE_INJECT */

        /* Not enough place to concatenate the next sc-list: do a loop */
        VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListLoop_p   = VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListWrite_p;
        VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListWrite_p  = VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListBase_p;


#if defined (WINCE_TRACE_ENABLED)
        if(Overflow)
           WINCE_TRACE((" !!!!!! SCLIST TRANSFER OVERFLOW ERROR !!!!!!!\n"));
        else
          WINCE_TRACE(("Looping SCList: SCListLoop_p=0x%.8X, SCListWrite_p=0x%.8X\r\n",
             VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListLoop_p,
             VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListWrite_p));
#endif
    }

    /* test if loop-ptr must be pushed */
    if(VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListWrite_p > VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListLoop_p)
    {
        VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListLoop_p = VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListWrite_p;
    }

#ifdef STVID_DEBUG_GET_STATISTICS
    (VIDINJ_Data_p->DMAInjecters[InjectNum]->InjectFdmaTransfers) ++;
#endif /* STVID_DEBUG_GET_STATISTICS */

#if defined(DVD_SECURED_CHIP)

    /**************************************
     ***  Post transfer ESCopy Buffer   ***
     **************************************/
    #if defined (WINCE_TRACE_ENABLED)
        ESCopyWrite_p = VIDINJ_Data_p->DMAInjecters[InjectNum]->ESCopy_Write_p;
    #endif

    #if !defined(STFDMA_USE_VIRTUAL_CONTEXT)
        STFDMA_ContextGetHeaderCopyBufferWritePtr(VIDINJ_Data_p->DMAInjecters[InjectNum]->FDMAContext,
                                          &VIDINJ_Data_p->DMAInjecters[InjectNum]->ESCopy_Write_p,
                                          &Overflow);
    #else /* STFDMA_USE_VIRTUAL_CONTEXT */
        STFDMA_VirtualContextGetHeaderCopyBufferWritePtr(VIDINJ_Data_p->DMAInjecters[InjectNum]->VirtualFDMAContextId,
                                                &VIDINJ_Data_p->DMAInjecters[InjectNum]->ESCopy_Write_p,
                                                &Overflow);
    #endif /* STFDMA_USE_VIRTUAL_CONTEXT */

    #if defined (CHECK_FDMA_INJECT_RESULT)
        /* Check the new Write pointer */
        if(  (U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->ESCopy_Write_p
           < (U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->ESCopy_Base_p)
        {
           TrFDMAError(("*** ESCopy bad new ESCopy Write pointer: new-W=0x%.8X < B=0x%.8X\n",
              VIDINJ_Data_p->DMAInjecters[InjectNum]->ESCopy_Write_p,
              VIDINJ_Data_p->DMAInjecters[InjectNum]->ESCopy_Base_p));
           FDMAErrorOccured = TRUE;
        }
        else if (  (U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->ESCopy_Write_p
                 > (U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->ESCopy_Top_p)
        {
           TrFDMAError(("*** ESCopy bad new ESCopy Write pointer: new-W=0x%.8X > T=0x%.8X\n",
              VIDINJ_Data_p->DMAInjecters[InjectNum]->ESCopy_Write_p,
              VIDINJ_Data_p->DMAInjecters[InjectNum]->ESCopy_Top_p));
           FDMAErrorOccured = TRUE;
        }
        else if(     ((U32)ESCopyWrite_p > (U32)ESCopy_Read_p)
                  && ((U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->ESCopy_Write_p >= (U32)ESCopy_Read_p)
                  && ((U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->ESCopy_Write_p < (U32)ESCopyWrite_p))
        {
           TrFDMAError(("*** ESCopy bad new ESCopy Write pointer: R=0x%.8X < new-W=0x%.8X < old-W=0x%.8X\n",
              ESCopy_Read_p,
              VIDINJ_Data_p->DMAInjecters[InjectNum]->ESCopy_Write_p,
              ESCopyWrite_p));
           FDMAErrorOccured = TRUE;
        }
        else if(     ((U32)ESCopyWrite_p < (U32)ESCopy_Read_p)
                  && ((U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->ESCopy_Write_p >= (U32)ESCopy_Read_p))
        {
           TrFDMAError(("*** ESCopy bad new ESCopy Write pointer: old-W=0x%.8X < R=0x%.8X < new-W=0x%.8X\n",
              ESCopyWrite_p,
              ESCopy_Read_p,
              VIDINJ_Data_p->DMAInjecters[InjectNum]->ESCopy_Write_p));
           FDMAErrorOccured = TRUE;
        }
        else if(     ((U32)ESCopyWrite_p < (U32)ESCopy_Read_p)
                  && ((U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->ESCopy_Write_p < (U32)ESCopyWrite_p))
        {
           TrFDMAError(("*** ESCopy bad new ESCopy Write pointer: new-W=0x%.8X < old-W=0x%.8X < R=0x%.8X\n",
              VIDINJ_Data_p->DMAInjecters[InjectNum]->ESCopy_Write_p,
              ESCopyWrite_p,
              ESCopy_Read_p));
           FDMAErrorOccured = TRUE;
        }

        /* Check if ESCopy buffer looped */
        if( VIDINJ_Data_p->DMAInjecters[InjectNum]->ESCopy_Write_p < ESCopyWrite_p)
        {
           TrFDMA(("---> ESCopy buffer looped: old-ESCopyWrite_p=0x%.8X < new-ESCopyWrite_p=%.8X\n",
                        ESCopyWrite_p,
                        VIDINJ_Data_p->DMAInjecters[InjectNum]->ESCopy_Write_p));
        }

        for(Cnt=0; Cnt<8; Cnt++)
        {
           if(DataBase_p[Cnt] != Pattern[Cnt])
           {
              TrFDMAError(("*** ESCopy pattern overwritten : ESCopy_Top_p=0x%.8X, Add=%.8X, Found=0x%.2X, Expected=0x%.2X\n",
                           VIDINJ_Data_p->DMAInjecters[InjectNum]->ESCopy_Top_p,
                           &DataBase_p[Cnt],
                           DataBase_p[Cnt],
                           Pattern[Cnt]));
              FDMAErrorOccured = TRUE;
           }
        }
    #endif /* CHECK_FDMA_INJECT_RESULT */

    #if defined (WINCE_TRACE_ENABLED)
        WINCE_TRACE(("ESCopy new ESCopyWrite_p=0x%.8X (old=0x%.8X), ESCopy_Read_p=0x%.8X\n",
           (U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->ESCopy_Write_p,
           (U32)ESCopyWrite_p,
           (U32)ESCopy_Read_p));
    #endif

#endif /* DVD_SECURED_CHIP */


    /************************************************
     ***   Post transfer ES Buffer (Bit Buffer)   ***
     ************************************************/

#if defined (WINCE_TRACE_ENABLED)
    ESWrite_p = VIDINJ_Data_p->DMAInjecters[InjectNum]->ES_Write_p;
#endif

    #if !defined(STFDMA_USE_VIRTUAL_CONTEXT)
        STFDMA_ContextGetESWritePtr(VIDINJ_Data_p->DMAInjecters[InjectNum]->FDMAContext,
                                    &VIDINJ_Data_p->DMAInjecters[InjectNum]->ES_Write_p,
                                    &Overflow);
    #else /* STFDMA_USE_VIRTUAL_CONTEXT */
        STFDMA_VirtualContextGetESWritePtr(VIDINJ_Data_p->DMAInjecters[InjectNum]->VirtualFDMAContextId,
                                    &VIDINJ_Data_p->DMAInjecters[InjectNum]->ES_Write_p,
                                    &Overflow);
    #endif /* STFDMA_USE_VIRTUAL_CONTEXT */
    #ifdef TRACE_INJECT
        /* DG: DMA Firmware does not handle overflow: it is the driver's job to make sure that an overflow
           will never happen. Thus, the test below will never be true */
        if (Overflow)
        {
            TrError(("INJECT ERROR: ES TRANSFER OVERFLOW ERROR !!!!!!!"));
        }
    #endif /* TRACE_UART */

#if defined (WINCE_TRACE_ENABLED)
    WINCE_TRACE(("ES BitBuffer new ESWrite_p=0x%.8X (old=0x%.8X)\n", (U32)ESWrite_p, (U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->ES_Write_p));
#endif

    TrESBuffer(("\r\nINJECT: Post transfer ES: Write_p=0x%x", VIDINJ_Data_p->DMAInjecters[InjectNum]->ES_Write_p));
#if defined (WINCE_TRACE_ENABLED)
    if(Overflow)
        WINCE_TRACE((" !!!!!! BITBUFFER OVERFLOW ERROR  !!!!!!\n"));
#endif

#if defined (CHECK_FDMA_INJECT_RESULT)
    /* Check the new Write pointer */
    if(  (U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->ES_Write_p
       < (U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->ES_Base_p)
    {
       TrFDMAError(("*** ES bad new ES Write pointer: new-W=0x%.8X < B=0x%.8X\n",
          VIDINJ_Data_p->DMAInjecters[InjectNum]->ES_Write_p,
          VIDINJ_Data_p->DMAInjecters[InjectNum]->ES_Base_p));
       FDMAErrorOccured = TRUE;
    }
    else if (  (U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->ES_Write_p
             > (U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->ES_Top_p)
    {
       TrFDMAError(("*** ES bad new ES Write pointer: new-W=0x%.8X > T=0x%.8X\n",
          VIDINJ_Data_p->DMAInjecters[InjectNum]->ES_Write_p,
          VIDINJ_Data_p->DMAInjecters[InjectNum]->ES_Top_p));
       FDMAErrorOccured = TRUE;
    }
    else if(     ((U32)ESWrite_p > (U32)ES_Read_p)
              && ((U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->ES_Write_p >= (U32)ES_Read_p)
              && ((U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->ES_Write_p <= (U32)ESWrite_p))
    {
       TrFDMAError(("*** ES bad new ES Write pointer: R=0x%.8X < new-W=0x%.8X < old-W=0x%.8X\n",
          ES_Read_p,
          VIDINJ_Data_p->DMAInjecters[InjectNum]->ES_Write_p,
          ESWrite_p));
       FDMAErrorOccured = TRUE;
    }
    else if(     ((U32)ESWrite_p < (U32)ES_Read_p)
              && ((U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->ES_Write_p >= (U32)ES_Read_p))
    {
       TrFDMAError(("*** ES bad new ES Write pointer: old-W=0x%.8X < R=0x%.8X < new-W=0x%.8X\n",
          ESWrite_p,
          ES_Read_p,
          VIDINJ_Data_p->DMAInjecters[InjectNum]->ES_Write_p));
       FDMAErrorOccured = TRUE;
    }
    else if(     ((U32)ESWrite_p < (U32)ES_Read_p)
              && ((U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->ES_Write_p <= (U32)ESWrite_p))
    {
       TrFDMAError(("*** ES bad new ES Write pointer: new-W=0x%.8X < old-W=0x%.8X < R=0x%.8X\n",
          VIDINJ_Data_p->DMAInjecters[InjectNum]->ES_Write_p,
          ESWrite_p,
          ES_Read_p));
       FDMAErrorOccured = TRUE;
    }
#endif /* CHECK_FDMA_INJECT_RESULT */

    /*************************************************
     ***  Post transfer PES Buffer (Input Buffer)  ***
     *************************************************/
    /* Nothing to do */
    TrPESBuffer(("\r\nInput: Base=0x%.8X Top=0x%.8X R=0x%.8X W=0x%.8X Size=%d\r\n",
       VIDINJ_Data_p->DMAInjecters[InjectNum]->InputBase_p,
       VIDINJ_Data_p->DMAInjecters[InjectNum]->InputTop_p,
       VIDINJ_Data_p->DMAInjecters[InjectNum]->InputRead_p,
       VIDINJ_Data_p->DMAInjecters[InjectNum]->InputWrite_p,
       (U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->InputTop_p - (U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->InputBase_p));

#ifdef STVID_DEBUG_GET_STATISTICS
    (VIDINJ_Data_p->DMAInjecters[InjectNum]->InjectFdmaTransfers) ++;
#endif /* STVID_DEBUG_GET_STATISTICS */

#if defined (CHECK_FDMA_INJECT_RESULT)
    /* Check the new Read pointer */
    if(  (U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->InputRead_p
       < (U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->InputBase_p)
    {
       TrFDMAError(("*** Input bad new Input Read pointer: new-R=0x%.8X < B=0x%.8X\n",
          VIDINJ_Data_p->DMAInjecters[InjectNum]->InputRead_p,
          VIDINJ_Data_p->DMAInjecters[InjectNum]->InputBase_p));
       FDMAErrorOccured = TRUE;
    }
    else if (  (U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->InputRead_p
             > (U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->InputTop_p)
    {
       TrFDMAError(("*** Input bad new Input Read pointer: new-R=0x%.8X > T=0x%.8X\n",
          VIDINJ_Data_p->DMAInjecters[InjectNum]->InputRead_p,
          VIDINJ_Data_p->DMAInjecters[InjectNum]->InputTop_p));
       FDMAErrorOccured = TRUE;
    }
    else if(     ((U32)InputWrite_p > (U32)InputRead_p)
              && ((U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->InputRead_p < (U32)InputRead_p))
    {
       TrFDMAError(("*** Input bad new Input Read pointer: new-R=0x%.8X < old-R=0x%.8X < W=0x%.8X\n",
          VIDINJ_Data_p->DMAInjecters[InjectNum]->InputRead_p,
          InputRead_p,
          InputWrite_p));
       FDMAErrorOccured = TRUE;
    }
    else if(     ((U32)InputWrite_p > (U32)InputRead_p)
              && ((U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->InputRead_p > (U32)InputWrite_p))
    {
       TrFDMAError(("*** Input bad new Input Read pointer: old-R=0x%.8X < W=0x%.8X < new-R=0x%.8X\n",
          InputRead_p,
          InputWrite_p,
          VIDINJ_Data_p->DMAInjecters[InjectNum]->InputRead_p));
       FDMAErrorOccured = TRUE;
    }
    else if(     ((U32)InputRead_p > (U32)InputWrite_p)
              && ((U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->InputRead_p > (U32)InputWrite_p)
              && ((U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->InputRead_p < (U32)InputRead_p))
    {
       TrFDMAError(("*** Input bad new Input Read pointer: W=0x%.8X < new-R=0x%.8X < old-R=0x%.8X\n",
          InputWrite_p,
          VIDINJ_Data_p->DMAInjecters[InjectNum]->InputRead_p,
          InputRead_p));
       FDMAErrorOccured = TRUE;
    }
#endif /* CHECK_FDMA_INJECT_RESULT */

#if defined (CHECK_FDMA_INJECT_RESULT)
    if(FDMAErrorOccured==TRUE)
    {
      NumberOfFDMAErrors++;
      TrFDMAError(("*** %d FDMA errors (%d FDMA)\n",
          NumberOfFDMAErrors,NumberOfFDMAPerformed));
    }
#endif /* CHECK_FDMA_INJECT_RESULT */

#if defined (WINCE_TRACE_ENABLED)
     WINCE_TRACE(("DMATransferOneSlice end (success) ---------------------------------\n"));
#endif
    return(TRUE);
} /* End of DMATransferOneSlice() function. */

/*******************************************************************************
Name        : StartInjectionTask
Description : Start the task of injection
Parameters  : Video injecter handle
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
              ST_ERROR_ALREADY_INITIALIZED if task already running
              ST_ERROR_BAD_PARAMETER if problem of creation
*******************************************************************************/
static ST_ErrorCode_t StartInjectionTask(const VIDINJ_Handle_t InjecterHandle)
{
    VIDCOM_Task_t * const Task_p = &(((VIDINJ_Data_t *) InjecterHandle)->InjectionTask);
    VIDINJ_Data_t * VIDINJ_Data_p = (VIDINJ_Data_t *) InjecterHandle;
    ST_ErrorCode_t ErrorCode;

    if (Task_p->IsRunning)
    {
        return(ST_ERROR_ALREADY_INITIALIZED);
    }

    Task_p->ToBeDeleted = FALSE;

    ErrorCode = STOS_TaskCreate ((void (*) (void*)) InjecterTaskFunc,
                                           (void *) InjecterHandle,
                                           VIDINJ_Data_p->CPUPartition_p,
                                           STVID_TASK_STACK_SIZE_INJECTERS,
                                           &(Task_p->TaskStack),
                                           VIDINJ_Data_p->CPUPartition_p,
                                           &(Task_p->Task_p),
                                           &(Task_p->TaskDesc),
                                           STVID_TASK_PRIORITY_INJECTERS,
                                           "STVID.InjecterTask",
                                           0 );
    if (ErrorCode != ST_NO_ERROR)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    Task_p->IsRunning  = TRUE;
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Injection task created."));
    return(ST_NO_ERROR);
} /* End of StartInjectionTask() function */

/*******************************************************************************
Name        : StopInjectionTask
Description : Stop the task of injection
Parameters  : Video injecter handle
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if no problem,
              ST_ERROR_ALREADY_INITIALIZED if task not running
*******************************************************************************/
static ST_ErrorCode_t StopInjectionTask(const VIDINJ_Handle_t InjecterHandle)
{
    VIDCOM_Task_t * const Task_p = &(((VIDINJ_Data_t *) InjecterHandle)->InjectionTask);
    VIDINJ_Data_t * VIDINJ_Data_p = (VIDINJ_Data_t *) InjecterHandle;
    task_t *TaskList_p = Task_p->Task_p;

    if (!(Task_p->IsRunning))
    {
        return(ST_ERROR_ALREADY_INITIALIZED);
    }

    /* End the function of the task here... */
    Task_p->ToBeDeleted = TRUE;

    STOS_TaskWait(&TaskList_p, TIMEOUT_INFINITY);
    STOS_TaskDelete (Task_p->Task_p,
                                  VIDINJ_Data_p->CPUPartition_p,
                                  Task_p->TaskStack,
                                  VIDINJ_Data_p->CPUPartition_p );

    Task_p->IsRunning  = FALSE;
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Injection task deleted."));

    return(ST_NO_ERROR);
} /* End of StopInjectionTask() function */

/*******************************************************************************
Name        : VIDINJ_EnterCriticalSection
Description : This function allows the decode task to access to the injecter's
              variables with a protection from mutual access. This function
              should be called each time before a call to any VIDINJ_...
              function except VIDINJ_TransferSetESRead, VIDINJ_TransferSetSCListRead,
              VIDINJ_Init, VIDINJ_Term, VIDINJ_TransferGetInject, VIDINJ_TransferReleaseInject
              This is to avoid that bitbuffer and startcode list pointers are
              changed from decode task context while, while we are preparing an FDMA
              transfer in the injection task.
Parameters  : Video injecter handle, Injecter Number
Assumptions : Function only called from decode task
Limitations :
Returns     : Nothing
*******************************************************************************/
void VIDINJ_EnterCriticalSection(const VIDINJ_Handle_t InjecterHandle, const U32 InjectNum)
{
    VIDINJ_Data_t * VIDINJ_Data_p = (VIDINJ_Data_t *) InjecterHandle;
    UNUSED_PARAMETER(InjectNum);

    STOS_SemaphoreWait(VIDINJ_Data_p->ContextSemaphore_p);
} /* End of VIDINJ_EnterCriticalSection() function */

/*******************************************************************************
Name        : VIDINJ_LeaveCriticalSection
Description : This function allows the decode task to remove the mutual access
              protection to the injecter's variables. This function must be
              called in pair with VIDINJ_EnterCriticalSection
Parameters  : Video injecter handle, Injecter Number
Assumptions : Function only called from decode task
Limitations :
Returns     : Nothing
*******************************************************************************/
void VIDINJ_LeaveCriticalSection(const VIDINJ_Handle_t InjecterHandle, const U32 InjectNum)
{
    VIDINJ_Data_t * VIDINJ_Data_p = (VIDINJ_Data_t *) InjecterHandle;
    UNUSED_PARAMETER(InjectNum);

    STOS_SemaphoreSignal(VIDINJ_Data_p->ContextSemaphore_p);
} /* End of VIDINJ_LeaveCriticalSection() function */

/*******************************************************************************
Name        : VIDINJ_ReallocateFDMANodes
Description : Reallocates injecter's contexts Node_p and NodeNext_p in partition
              passed as parameter. Used for secured chip where fdma nodes must
              be allocated in a non-secured partition.
Parameters  : InjecterHandle, DMA injecter number, setup params
Assumptions : To be called after STVID_Init.
              Protected by VIDINJ_EnterCriticalSection.
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_BAD_PARAMETER, ST_ERROR_FEATURE_NOT_SUPPORTED
*******************************************************************************/
ST_ErrorCode_t VIDINJ_ReallocateFDMANodes(const VIDINJ_Handle_t InjecterHandle, const U32 InjectNum, const STVID_SetupParams_t * const SetupParams_p)
{
    VIDINJ_Data_t * VIDINJ_Data_p = (VIDINJ_Data_t *) InjecterHandle;
    ST_ErrorCode_t                          ErrorCode = ST_NO_ERROR;
    STAVMEM_BlockParams_t                   BlockParams;
    STAVMEM_AllocBlockParams_t	            AllocBlockParams;
    STAVMEM_FreeBlockParams_t               FreeParams;
    void *                                  VirtualAddress;
    InjContext_t *                          InjContext_p;

    TEST_INJECT_ERROR_CODE(InjectNum);

    /* Test passed parameters */
    if (SetupParams_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
    if (SetupParams_p->SetupSettings.AVMEMPartition == 0 /* STAVMEM_INVALID_PARTITION_HANDLE does not exist yet ! */)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
    if (SetupParams_p->SetupObject != STVID_SETUP_FDMA_NODES_PARTITION)
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    /* Shortcuts */
    InjContext_p = VIDINJ_Data_p->DMAInjecters[InjectNum];

    if (InjContext_p->NodeHdl == STAVMEM_INVALID_BLOCK_HANDLE)
    {
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module injection setup: Node Handle is NULL!"));
    }
    else
    {

        /*************************
         * Reallocate first node
         *************************/

        /* Get allocation parameters from block to deallocate */
        /* Can't use STAVMEM_ReallocBlock() here because we need to change partition */
        ErrorCode = STAVMEM_GetBlockParams(InjContext_p->NodeHdl, &BlockParams);

        if (ErrorCode != ST_NO_ERROR)
        {
            /* STAVMEM_GetBlockParams returned an error */
            /* Fill-up alloc params with default values as used in VIDINJ_TransferGetInject */
            AllocBlockParams.Alignment                = FDMA_PESSCD_NODE_ALIGNMENT;
            AllocBlockParams.Size                     = sizeof(STFDMA_GenericNode_t);
            AllocBlockParams.AllocMode                = STAVMEM_ALLOC_MODE_BOTTOM_TOP;
        }
        else
        {
            AllocBlockParams.Alignment                = BlockParams.Alignment;
            AllocBlockParams.Size                     = BlockParams.Size;
            AllocBlockParams.AllocMode                = BlockParams.AllocMode;
        }

        /* Deallocate AVMEM block in current partition */
        FreeParams.PartitionHandle = InjContext_p->FDMANodesAvmemPartition;
        ErrorCode = STAVMEM_FreeBlock(&FreeParams, &(InjContext_p->NodeHdl));

        if (ErrorCode != ST_NO_ERROR)
        {
            /* Could not free current memory block */
            ErrorCode = ST_ERROR_BAD_PARAMETER;
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module injection setup: could not free fdma nodes!"));
        }
        else
        {
            /* Change partition and reallocate block */
            InjContext_p->FDMANodesAvmemPartition     = SetupParams_p->SetupSettings.AVMEMPartition;
            AllocBlockParams.PartitionHandle          = SetupParams_p->SetupSettings.AVMEMPartition;

            AllocBlockParams.NumberOfForbiddenRanges  = 0;
            AllocBlockParams.ForbiddenRangeArray_p    = NULL;
            AllocBlockParams.NumberOfForbiddenBorders = 0;
            AllocBlockParams.ForbiddenBorderArray_p   = NULL;
            ErrorCode = STAVMEM_AllocBlock(&AllocBlockParams, &InjContext_p->NodeHdl);

            if (ErrorCode != ST_NO_ERROR)
            {
                /* Could not allocate memory in new partition */
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module injection setup: could not allocate fdma nodes in new partition!"));
            }
            else
            {
                ErrorCode = STAVMEM_GetBlockAddress(InjContext_p->NodeHdl, (void **)&VirtualAddress);

                if(ErrorCode == ST_NO_ERROR)
                {
                    InjContext_p->Node_p = STAVMEM_VirtualToCPU(VirtualAddress, STAVMEM_VIRTUAL_MAPPING_AUTO_P);
                    /* reset all node's fields (reserved fields must be set to ZERO) */
                    STOS_memsetUncached((void*)((U32)InjContext_p->Node_p), 0x00, sizeof(STFDMA_GenericNode_t));
                }
            }
        }
    }

    if (ErrorCode == ST_NO_ERROR)
    {
        /*************************
         * Reallocate next node
         *************************/

        /* Same process as for first node structure */
        ErrorCode = STAVMEM_GetBlockParams(InjContext_p->NodeNextHdl, &BlockParams);

        if (ErrorCode != ST_NO_ERROR)
        {
            /* STAVMEM_GetBlockParams returned an error */
            /* Fill-up alloc params with default values as used in VIDINJ_TransferGetInject */
            AllocBlockParams.Alignment                = FDMA_PESSCD_NODE_ALIGNMENT;
            AllocBlockParams.Size                     = sizeof(STFDMA_GenericNode_t);
            AllocBlockParams.AllocMode                = STAVMEM_ALLOC_MODE_BOTTOM_TOP;
        }
        else
        {
            AllocBlockParams.Alignment                = BlockParams.Alignment;
            AllocBlockParams.Size                     = BlockParams.Size;
            AllocBlockParams.AllocMode                = BlockParams.AllocMode;
        }

        /* Deallocate AVMEM block in current partition */
        FreeParams.PartitionHandle = InjContext_p->FDMANodesAvmemPartition;
        ErrorCode = STAVMEM_FreeBlock(&FreeParams, &(InjContext_p->NodeNextHdl));

        if (ErrorCode != ST_NO_ERROR)
        {
            /* Could not free current memory block */
            ErrorCode = ST_ERROR_BAD_PARAMETER;
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module injection setup: could not free fdma nodes!"));
        }
        else
        {
            /* Change partition and reallocate block */
            InjContext_p->FDMANodesAvmemPartition     = SetupParams_p->SetupSettings.AVMEMPartition;
            AllocBlockParams.PartitionHandle          = SetupParams_p->SetupSettings.AVMEMPartition;

            AllocBlockParams.NumberOfForbiddenRanges  = 0;
            AllocBlockParams.ForbiddenRangeArray_p    = NULL;
            AllocBlockParams.NumberOfForbiddenBorders = 0;
            AllocBlockParams.ForbiddenBorderArray_p   = NULL;
            ErrorCode = STAVMEM_AllocBlock(&AllocBlockParams, &InjContext_p->NodeNextHdl);

            if (ErrorCode != ST_NO_ERROR)
            {
                /* Could not allocate memory in new partition */
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module injection setup: could not allocate fdma nodes in new partition!"));
            }
            else
            {
                ErrorCode = STAVMEM_GetBlockAddress(InjContext_p->NodeNextHdl, (void **)&VirtualAddress);

                if(ErrorCode == ST_NO_ERROR)
                {
                    InjContext_p->NodeNext_p = STAVMEM_VirtualToCPU(VirtualAddress, STAVMEM_VIRTUAL_MAPPING_AUTO_P);
                }
            }
        }
    }

    if (ErrorCode == ST_NO_ERROR)
    {
        InjContext_p->Node_p->ContextNode.Extended = STFDMA_EXTENDED_NODE;
        InjContext_p->Node_p->ContextNode.Type     = STFDMA_EXT_NODE_PES;
#if !defined (STFDMA_USE_VIRTUAL_CONTEXT)
        InjContext_p->Node_p->ContextNode.Context  = InjContext_p->FDMAContext;
#else /* STFDMA_USE_VIRTUAL_CONTEXT */
        InjContext_p->Node_p->ContextNode.Context  = VIDINJ_Data_p->FDMAContextId;
#endif /* STFDMA_USE_VIRTUAL_CONTEXT */
        InjContext_p->Node_p->ContextNode.Tag      = 0;
#if defined (ST_7109) || defined (ST_5525)
        InjContext_p->Node_p->ContextNode.Secure   = VIDINJ_Data_p->IsFDMATransferSecure;
#elif defined (ST_7200)
        InjContext_p->Node_p->ContextNode.Secure   = 0;
#endif /* 7109 || 5525 || 7200 */

        InjContext_p->Node_p->ContextNode.Pad1 = 0;
        InjContext_p->Node_p->ContextNode.Pad2 = 0;

        InjContext_p->Node_p->ContextNode.NodeCompletePause   = 0; /* 1bit */
        InjContext_p->Node_p->ContextNode.NodeCompleteNotify  = 0; /* 1bit */

        *(InjContext_p->NodeNext_p) = *(InjContext_p->Node_p);

#ifdef ST_OS20
        STOS_memcpyCachedToUncached((void*)STAVMEM_CPUToDevice(InjContext_p->Node_p, STAVMEM_VIRTUAL_MAPPING_AUTO_P), /*dst*/
                                    (void*)InjContext_p->Node_p, /*src*/
                                    sizeof(STFDMA_ContextNode_t));
        STOS_memcpyCachedToUncached((void*)STAVMEM_CPUToDevice(InjContext_p->NodeNext_p, STAVMEM_VIRTUAL_MAPPING_AUTO_P), /*dst*/
                                    (void*)InjContext_p->Node_p, /*src*/
                                    sizeof(STFDMA_ContextNode_t));
#else
        STOS_FlushRegion( STOS_MapPhysToCached(InjContext_p->Node_p, sizeof(STFDMA_ContextNode_t) ), sizeof(STFDMA_ContextNode_t) );
        STOS_FlushRegion( STOS_MapPhysToCached(InjContext_p->NodeNext_p, sizeof(STFDMA_ContextNode_t) ), sizeof(STFDMA_ContextNode_t) );
#endif
    }

    return(ErrorCode);

} /* End of VIDINJ_ReallocateFDMANodes() function */

#ifdef STVID_DEBUG_GET_STATISTICS
ST_ErrorCode_t VIDINJ_GetStatistics(const VIDINJ_Handle_t InjecterHandle, const U32 InjectNum,
                                STVID_Statistics_t * const Statistics_p)
{
    VIDINJ_Data_t * VIDINJ_Data_p = (VIDINJ_Data_t *) InjecterHandle;

    if (Statistics_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    Statistics_p->InjectFdmaTransfers = VIDINJ_Data_p->DMAInjecters[InjectNum]->InjectFdmaTransfers;
    Statistics_p->InjectDataInputReadPointer = VIDINJ_Data_p->DMAInjecters[InjectNum]->InputRead_p;
    Statistics_p->InjectDataInputWritePointer = VIDINJ_Data_p->DMAInjecters[InjectNum]->InputWrite_p;

    return(ST_NO_ERROR);
} /* end of VIDINJ_GetStatistics() */

ST_ErrorCode_t VIDINJ_ResetStatistics(const VIDINJ_Handle_t InjecterHandle, const U32 InjectNum,
                                const STVID_Statistics_t * const Statistics_p)
{
    VIDINJ_Data_t * VIDINJ_Data_p = (VIDINJ_Data_t *) InjecterHandle;

    if (Statistics_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (Statistics_p->InjectFdmaTransfers != 0)
    {
        VIDINJ_Data_p->DMAInjecters[InjectNum]->InjectFdmaTransfers = 0;
    }

    return(ST_NO_ERROR);
} /* end of VIDINJ_ResetStatistics() */
#endif /* STVID_DEBUG_GET_STATISTICS */

#ifdef STVID_DEBUG_GET_STATUS
ST_ErrorCode_t VIDINJ_GetStatus(const VIDINJ_Handle_t InjecterHandle, const U32 InjectNum,
                                STVID_Status_t * const Status_p)
{
	VIDINJ_Data_t * VIDINJ_Data_p = (VIDINJ_Data_t *) InjecterHandle;
	InjContext_t * InjContext_p = VIDINJ_Data_p->DMAInjecters[InjectNum];

    if (Status_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

	/* ES, PES, SC Buffers */
	Status_p->BitBufferAddress_p = VIDINJ_Data_p->DMAInjecters[InjectNum]->ES_Base_p;
    Status_p->BitBufferSize = (((U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->ES_Top_p) - (U32)(VIDINJ_Data_p->DMAInjecters[InjectNum]->ES_Base_p));
    Status_p->DataInputBufferAddress_p = VIDINJ_Data_p->DMAInjecters[InjectNum]->InputBase_p;
    Status_p->DataInputBufferSize = (((U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->InputTop_p) - ((U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->InputBase_p));
    Status_p->SCListBufferAddress_p = VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListBase_p;
    Status_p->SCListBufferSize = (((U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListTop_p) - ((U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListBase_p));

	/* FDMA Nodes related */
	Status_p->FdmaNodesSize = 0;

	if (VIDINJ_Data_p->DMAInjecters[InjectNum]->Node_p != NULL)
	{
		Status_p->FdmaNodesSize += sizeof(STFDMA_GenericNode_t);
	}

	if (VIDINJ_Data_p->DMAInjecters[InjectNum]->NodeNext_p != NULL)
	{
		Status_p->FdmaNodesSize += sizeof(STFDMA_GenericNode_t);
	}

	/* ES, PES, SC Pointers */
	Status_p->BitBufferReadPointer = VIDINJ_Data_p->DMAInjecters[InjectNum]->ES_Read_p;
	Status_p->BitBufferWritePointer = VIDINJ_Data_p->DMAInjecters[InjectNum]->ES_Write_p;
	Status_p->DataInputBufferReadPointer = VIDINJ_Data_p->DMAInjecters[InjectNum]->InputRead_p;
	Status_p->DataInputBufferWritePointer = VIDINJ_Data_p->DMAInjecters[InjectNum]->InputWrite_p;
	Status_p->SCListBufferReadPointer = VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListRead_p;
	Status_p->SCListBufferWritePointer = VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListWrite_p;

	/* InjectFlushBufferSize */
	if (InjContext_p->FlushBuffer_p != NULL)
	{
		Status_p->InjectFlushBufferSize = FLUSH_DATA_SIZE;
	}
	else
	{
		Status_p->InjectFlushBufferSize = 0;
	}

    return(ST_NO_ERROR);
} /* end of VIDINJ_GetStatus() */
#endif /* STVID_DEBUG_GET_STATUS */
/* End of inject.c */
