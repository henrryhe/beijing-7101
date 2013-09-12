/*******************************************************************************

File name : acc_fdma.c

Description : AVMEM memory access file for FDMA

COPYRIGHT (C) STMicroelectronics 2002.

Date                Modification                                     Name
----                ------------                                     ----
20 Nov 2002         Creation                                         HSdLM
03 Feb 2003         Fill: optimize last transfer.                    HSdLM
*******************************************************************************/


/* Includes ----------------------------------------------------------------- */

#include <string.h>

#include "stddefs.h"
#include "stsys.h"
#include "stfdma.h"

#include "stavmem.h"
#include "avm_acc.h"
#include "acc_fdma.h"

/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */

/* Private Variables (static)------------------------------------------------ */

/* Private Macros ----------------------------------------------------------- */

/* Private Function prototypes ---------------------------------------------- */

static void InitFDMA(void);
static void Copy1DNoOverlapFDMA(void * const SrcAddr_p, void * const DestAddr_p, const U32 Size);
static void Copy2DNoOverlapFDMA(void * const SrcAddr_p,  const U32 SrcWidth, const U32 SrcHeight, const U32 SrcPitch,
                                               void * const DestAddr_p, const U32 DestPitch);
static void Fill1DFDMA(void * const Pattern_p,  const U32 PatternSize,  void * const DestAddr_p, const U32 DestSize);
static void Fill2DFDMA(void * const Pattern_p,  const U32 PatternWidth, const U32 PatternHeight, const U32 PatternPitch,
                                      void * const DestAddr_p, const U32 DestWidth, const U32 DestHeight, const U32 DestPitch);

stavmem_MethodOperations_t stavmem_FDMA = {
    InitFDMA,                        /* Private function */
    /* FDMA only works when there is no overlap */
    {
        Copy1DNoOverlapFDMA,         /* Private function */
        NULL,                         /* Not supported */
        Copy2DNoOverlapFDMA,         /* Private function */
        NULL,                         /* Not supported */
        Fill1DFDMA,                  /* Private function */
        Fill2DFDMA                   /* Private function */
    },
    /* No need to check: FDMA is always successful */
    {
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL
    }
};


/* Functions ---------------------------------------------------------------- */

/*******************************************************************************
Name        : InitFDMA
Description : Do nothing.
Parameters  : None
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void InitFDMA(void)
{
} /* end of InitFDMA() */

/*******************************************************************************
Name        : Copy1DNoOverlapFDMA
Description : Perform 1D copy with no care of overlaps, with FDMA method
Parameters  : source, destination, size
Assumptions : slowest copy rate is 2^23 = 8Mb per second, no size limitation (no need to cut in blocks)
Limitations :
Returns     : Nothing
*******************************************************************************/
static void Copy1DNoOverlapFDMA(void * const SrcAddr_p, void * const DestAddr_p, const U32 Size)
{

    STFDMA_Node_t           *FdmaAllocatedNode_p, *FdmaNode_p;
    STFDMA_TransferId_t     FdmaTransferId = 0;
    STFDMA_TransferParams_t FdmaTransferParams;
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;
    ST_ErrorCode_t Err;

    /* Allocation here costs overhead but saves semaphore protection */
    /* Allocate size in Non-cached partition for a Node, aligned on 32 bytes */
    /* Create node+31bytes  */
    FdmaAllocatedNode_p = (STFDMA_Node_t *)(STOS_MemoryAllocateClear(stavmem_MemAccessDevice.NCachePartition_p,
                                                                             1,(sizeof(STFDMA_Node_t)+31)));
    if (FdmaAllocatedNode_p == NULL)
    {
        stavmem_MemAccessDevice.LastFdmaTranfertFailed = TRUE;
        stavmem_MemAccessDevice.TranfertFailed = TRUE;
        return;
    }
    /* Ensure the node pointer is aligned on a 32byte boundary */
    FdmaNode_p = (STFDMA_Node_t *) (((U32)FdmaAllocatedNode_p + 31) & ~31);

    FdmaNode_p->Next_p                           = NULL;
    FdmaNode_p->NodeControl.PaceSignal           = STFDMA_REQUEST_SIGNAL_NONE;
#if defined (ST_7109) && defined (DVD_SECURED_CHIP)
    FdmaNode_p->NodeControl.Reserved           = 0;
    if(stavmem_MemAccessDevice.SecuredCopy)
    {
        FdmaNode_p->NodeControl.Secure         = 1;
    }
    else
    {
        FdmaNode_p->NodeControl.Secure         = 0;
    }
    FdmaNode_p->NodeControl.Reserved1          = 0;
#else
    FdmaNode_p->NodeControl.Reserved           = 0;
#endif

    FdmaNode_p->NodeControl.SourceDirection      = STFDMA_DIRECTION_INCREMENTING;
    FdmaNode_p->NodeControl.DestinationDirection = STFDMA_DIRECTION_INCREMENTING;
    FdmaNode_p->NodeControl.NodeCompleteNotify   = FALSE;
    FdmaNode_p->NodeControl.NodeCompletePause    = FALSE;

    /* FDMA can transfert up to 4GB, no need to cut in blocks */
    STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping);
    FdmaNode_p->NumberBytes              = Size;        /* Number of bytes in node */
    FdmaNode_p->SourceAddress_p          = STAVMEM_VirtualToDevice(SrcAddr_p, &VirtualMapping);   /* Start of source data */
    FdmaNode_p->DestinationAddress_p     = STAVMEM_VirtualToDevice(DestAddr_p, &VirtualMapping);  /* Start of dest location */
    FdmaNode_p->Length                   = Size;        /* Line length */
    FdmaNode_p->SourceStride             = 0;           /* Stride between source lines */
    FdmaNode_p->DestinationStride        = 0;           /* Stride between dest lines  */

#ifdef ST_7200
    FdmaTransferParams.FDMABlock         = STFDMA_2;
#else
    FdmaTransferParams.FDMABlock         = STFDMA_1;
#endif
    FdmaTransferParams.ChannelId         = STFDMA_USE_ANY_CHANNEL;
    FdmaTransferParams.NodeAddress_p     = STAVMEM_VirtualToDevice(FdmaNode_p, &VirtualMapping);
    /* asumption 2^23 = 8Mb : worst case (slowest) copy rate per second */
    FdmaTransferParams.BlockingTimeout   = 1000 * (1+ (Size >> 23));
    FdmaTransferParams.CallbackFunction  = NULL;  /* this activates blocking mode */
    FdmaTransferParams.ApplicationData_p = NULL;

    Err = STFDMA_StartTransfer(&FdmaTransferParams, &FdmaTransferId);

    /* De-allocate Node */
    STOS_MemoryDeallocate(stavmem_MemAccessDevice.NCachePartition_p, FdmaAllocatedNode_p);
    FdmaAllocatedNode_p = NULL;

    if (Err != ST_NO_ERROR)
    {
        stavmem_MemAccessDevice.LastFdmaTranfertFailed = TRUE;
        stavmem_MemAccessDevice.TranfertFailed = TRUE;
    }

} /* end of Copy1DNoOverlapFDMA() */


/*******************************************************************************
Name        : Copy2DNoOverlapFDMA
Description : Perform 2D copy with no care of overlaps, with FDMA method
Parameters  : source, dimensions, source pitch, destination, destination pitch
Assumptions : slowest copy rate is 2^23 = 8Mb per second, no size limitation (no need to cut in blocks)
Limitations :
Returns     : Nothing
*******************************************************************************/
static void Copy2DNoOverlapFDMA(void * const SrcAddr_p,  const U32 SrcWidth, const U32 SrcHeight, const U32 SrcPitch,
                                               void * const DestAddr_p, const U32 DestPitch)
{
    STFDMA_Node_t           *FdmaAllocatedNode_p, *FdmaNode_p;
    STFDMA_TransferId_t     FdmaTransferId = 0;
    STFDMA_TransferParams_t FdmaTransferParams;
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;
    U32 Size;
    ST_ErrorCode_t Err;

    /* Allocation here costs overhead but saves semaphore protection */
    /* Allocate size in Non-cached partition for a Node, aligned on 32 bytes */
    /* Create node+31bytes  */
    FdmaAllocatedNode_p = (STFDMA_Node_t *)(STOS_MemoryAllocateClear(stavmem_MemAccessDevice.NCachePartition_p,
                                                                             1,(sizeof(STFDMA_Node_t)+31)));
    if (FdmaAllocatedNode_p == NULL)
    {
        stavmem_MemAccessDevice.LastFdmaTranfertFailed = TRUE;
        stavmem_MemAccessDevice.TranfertFailed = TRUE;
        return;
    }
    /* Ensure the node pointer is aligned on a 32byte boundary */
    FdmaNode_p = (STFDMA_Node_t *) (((U32)FdmaAllocatedNode_p + 31) & ~31);

    FdmaNode_p->Next_p                           = NULL;
    FdmaNode_p->NodeControl.PaceSignal           = STFDMA_REQUEST_SIGNAL_NONE;
#if defined (ST_7109) && defined (DVD_SECURED_CHIP)
    FdmaNode_p->NodeControl.Reserved           = 0;
    if(stavmem_MemAccessDevice.SecuredCopy)
    {
        FdmaNode_p->NodeControl.Secure         = 1;
    }
    else
    {
        FdmaNode_p->NodeControl.Secure         = 0;
    }
    FdmaNode_p->NodeControl.Reserved1          = 0;
#else
    FdmaNode_p->NodeControl.Reserved           = 0;
#endif

    FdmaNode_p->NodeControl.SourceDirection      = STFDMA_DIRECTION_INCREMENTING;
    FdmaNode_p->NodeControl.DestinationDirection = STFDMA_DIRECTION_INCREMENTING;
    FdmaNode_p->NodeControl.NodeCompleteNotify   = FALSE;
    FdmaNode_p->NodeControl.NodeCompletePause    = FALSE;

    Size = SrcWidth * SrcHeight;

    /* FDMA can transfert up to 4GB, no need to cut in blocks */
    STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping);
    FdmaNode_p->NumberBytes              = Size;        /* Number of bytes in node */
    FdmaNode_p->SourceAddress_p          = STAVMEM_VirtualToDevice(SrcAddr_p, &VirtualMapping);   /* Start of source data */
    FdmaNode_p->DestinationAddress_p     = STAVMEM_VirtualToDevice(DestAddr_p, &VirtualMapping);  /* Start of dest location */
    FdmaNode_p->Length                   = SrcWidth;    /* Line length */
    FdmaNode_p->SourceStride             = SrcPitch;    /* Stride between source lines */
    FdmaNode_p->DestinationStride        = DestPitch;   /* Stride between dest lines  */

#ifdef ST_7200
    FdmaTransferParams.FDMABlock         = STFDMA_2;
#else
    FdmaTransferParams.FDMABlock         = STFDMA_1;
#endif
    FdmaTransferParams.ChannelId         = STFDMA_USE_ANY_CHANNEL;
    FdmaTransferParams.NodeAddress_p     = STAVMEM_VirtualToDevice(FdmaNode_p, &VirtualMapping);
    /* asumption 2^23 = 8Mb : worst case (slowest) copy rate per second */
    FdmaTransferParams.BlockingTimeout   = 1000 * (1+ (Size >> 23));
    FdmaTransferParams.CallbackFunction  = NULL;  /* this activates blocking mode */
    FdmaTransferParams.ApplicationData_p = NULL;

    Err = STFDMA_StartTransfer(&FdmaTransferParams, &FdmaTransferId);

    /* De-allocate Node */
    STOS_MemoryDeallocate(stavmem_MemAccessDevice.NCachePartition_p, FdmaAllocatedNode_p);
    FdmaAllocatedNode_p = NULL;

    if (Err != ST_NO_ERROR)
    {
        stavmem_MemAccessDevice.LastFdmaTranfertFailed = TRUE;
        stavmem_MemAccessDevice.TranfertFailed = TRUE;
    }

} /* end of Copy2DNoOverlapFDMA() */


/*******************************************************************************
Name        : Fill1DFDMA
Description : Perform 1D fill with FDMA method
Parameters  : pattern, pattern size, destination, destination size
Assumptions :
Limitations :
Returns     : Nothing: always successful
*******************************************************************************/
static void Fill1DFDMA(void * const Pattern_p,  const U32 PatternSize,  void * const DestAddr_p, const U32 DestSize)
{
    U32 IndexDest;

    /* we don't know yet how many transfers are required, so to build a DMA chain we would
       make a dynamic allocation of size (NumberTransfers * sizeof(STFDMA_DmaTransfer_t))
       which can be a big size (e.g. : 1Kb PatternSize, 8MB DestSize => 8192 transfers
       So no DMA chain is built.
    */
    IndexDest = 0;
    while ( ((DestSize - IndexDest) >= PatternSize) && (!stavmem_MemAccessDevice.LastFdmaTranfertFailed))
    {
        Copy1DNoOverlapFDMA(Pattern_p, (void *) (((U8 *) DestAddr_p) + IndexDest), PatternSize);
        IndexDest += PatternSize;
    }
    if ( ((DestSize - IndexDest) > 0) && (!stavmem_MemAccessDevice.LastFdmaTranfertFailed))
    {
        Copy1DNoOverlapFDMA(Pattern_p, (void *) (((U8 *) DestAddr_p) + IndexDest), DestSize - IndexDest);
    }
} /* end of Fill1DFDMA() */


/*******************************************************************************
Name        : Fill2DFDMA
Description : Perform 2D fill with FDMA method
Parameters  : pattern, pattern characteristics, destination, destination  characteristics
Assumptions :
Limitations :
Returns     : Nothing: always successful
*******************************************************************************/
static void Fill2DFDMA(void * const Pattern_p,  const U32 PatternWidth, const U32 PatternHeight, const U32 PatternPitch,
                                      void * const DestAddr_p, const U32 DestWidth, const U32 DestHeight, const U32 DestPitch)
{
    U32 HorizontalIndexDest, VerticalIndexDest;

    /* we don't know yet how many transfers are required, so to build a DMA chain we would
       make a dynamic allocation of size (NumberTransfers * sizeof(STFDMA_DmaTransfer_t))
       which can be a big size.
       So no DMA chain is built.
    */

    VerticalIndexDest = 0;
    while ((DestHeight - VerticalIndexDest >= PatternHeight ) && (!stavmem_MemAccessDevice.LastFdmaTranfertFailed))
    {
        HorizontalIndexDest = 0;
        while (((DestWidth - HorizontalIndexDest) >= PatternWidth) && (!stavmem_MemAccessDevice.LastFdmaTranfertFailed))
        {
            /* fill with full pattern */
            Copy2DNoOverlapFDMA( Pattern_p, PatternWidth, PatternHeight, PatternPitch,
                                  (void *) (((U8 *) DestAddr_p) + (VerticalIndexDest * DestPitch) + HorizontalIndexDest),
                                  DestPitch);
            HorizontalIndexDest += PatternWidth;
        }
        if ( ((DestWidth - HorizontalIndexDest) > 0) && (!stavmem_MemAccessDevice.LastFdmaTranfertFailed))
        {
            /* fill with partial pattern, remaining space at right of n horizontal previous patterns fully filled */
             Copy2DNoOverlapFDMA( Pattern_p, DestWidth - HorizontalIndexDest, PatternHeight, PatternPitch,
                                (void *) (((U8 *) DestAddr_p) + (VerticalIndexDest * DestPitch) + HorizontalIndexDest),
                                DestPitch);
        }
        VerticalIndexDest += PatternHeight;
    }

    /* fill with partial patterns, remaining spaces at bottom of n vertical previous patterns fully filled */
    if ( ((DestHeight - VerticalIndexDest) > 0) && (!stavmem_MemAccessDevice.LastFdmaTranfertFailed))
    {
        HorizontalIndexDest = 0;
        while (((DestWidth - HorizontalIndexDest) >= PatternWidth) && (!stavmem_MemAccessDevice.LastFdmaTranfertFailed))
        {
            Copy2DNoOverlapFDMA( Pattern_p, PatternWidth, DestHeight - VerticalIndexDest, PatternPitch,
                                    (void *) (((U8 *) DestAddr_p) + (VerticalIndexDest * DestPitch) + HorizontalIndexDest),
                                    DestPitch);
            HorizontalIndexDest += PatternWidth;
        }
        if ( ((DestWidth - HorizontalIndexDest) > 0) && (!stavmem_MemAccessDevice.LastFdmaTranfertFailed))
        {
            /* fill with last partial pattern, remaining space at bottom and right of n vertical and horizontal previous patterns partially filled */
             Copy2DNoOverlapFDMA( Pattern_p, DestWidth - HorizontalIndexDest, DestHeight - VerticalIndexDest, PatternPitch,
                                    (void *) (((U8 *) DestAddr_p) + (VerticalIndexDest * DestPitch) + HorizontalIndexDest),
                                    DestPitch);
        } /* if ( ((DestWidth - HorizontalIndexDest) > 0) && ... */
    } /* if ( ((DestHeight - VerticalIndexDest) > 0) && ... */
} /* end of Fill2DFDMA() */

/* End of acc_fdma.c */

