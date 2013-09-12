/*******************************************************************************

File name : acc_gpd.c

Description : AVMEM memory access file for GPDMA

COPYRIGHT (C) STMicroelectronics 2002.

Date                Modification                                     Name
----                ------------                                     ----
29 May 2001         Creation                                         HSdLM
05 Jul 2001         Remove dependency upon STGPDMA if GPDMA method   HSdLM
 *                  is not available (compilation flag).
05 Apr 2002         Fix performance issue: try to use max Unit size  HSdLM
18 Dec 2002         Fix problem when AlignedWidth == 0               HSdLM
03 Feb 2003         Fill: optimize last transfer.                    HSdLM
*******************************************************************************/


/* Includes ----------------------------------------------------------------- */

#include <string.h>

#include "stddefs.h"
#include "stsys.h"
#include "stgpdma.h"

#include "stavmem.h"
#include "avm_acc.h"
#include "acc_gpd.h"


/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */

/* Private Variables (static)------------------------------------------------ */

static semaphore_t *stavmem_LockGPDMA_p;   /* GPDMA locking semaphore */
static BOOL GPDMAInitialised = FALSE;

/* Private Macros ----------------------------------------------------------- */

/* Private Function prototypes ---------------------------------------------- */

static void InitGPDMA(void);
static void Copy1DNoOverlapGPDMA(void * const SrcAddr_p, void * const DestAddr_p, const U32 Size);
static void Copy2DNoOverlapGPDMA(void * const SrcAddr_p,  const U32 SrcWidth, const U32 SrcHeight, const U32 SrcPitch,
                                               void * const DestAddr_p, const U32 DestPitch);
static void Fill1DGPDMA(void * const Pattern_p,  const U32 PatternSize,  void * const DestAddr_p, const U32 DestSize);
static void Fill2DGPDMA(void * const Pattern_p,  const U32 PatternWidth, const U32 PatternHeight, const U32 PatternPitch,
                                      void * const DestAddr_p, const U32 DestWidth, const U32 DestHeight, const U32 DestPitch);


stavmem_MethodOperations_t stavmem_GPDMA = {
    InitGPDMA,                        /* Private function */
    /* GPDMA only works when there is no overlap */
    {
        Copy1DNoOverlapGPDMA,         /* Private function */
        NULL,                         /* Not supported */
        Copy2DNoOverlapGPDMA,         /* Private function */
        NULL,                         /* Not supported */
        Fill1DGPDMA,                  /* Private function */
        Fill2DGPDMA                   /* Private function */
    },
    /* No need to check: GPDMA is always successful */
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
Name        : InitGPDMA
Description : Init semaphore used for memory accesses with GPDMA peripheral
Parameters  : None
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void InitGPDMA(void)
{
    static BOOL GPDMASemaphoreInitialised = FALSE;

    /* Protect semaphores initialisation */
    STOS_TaskLock();
    if (!GPDMASemaphoreInitialised)
    {
        stavmem_LockGPDMA_p = STOS_SemaphoreCreateFifo(NULL,1);   /* Init GPDMA peripheral locking semaphore */
        GPDMASemaphoreInitialised = TRUE;
    }
    /* Un-protect semaphores initialisation */
    STOS_TaskUnlock();

    /* Caution, deletion will never occur ! */
    /* Delete GPDMA locking semaphore */
/*    semaphore_delete(stavmem_LockGPDMA_p);*/

    GPDMAInitialised = TRUE;
} /* end of InitGPDMA() */


/*******************************************************************************
Name        : Copy1DNoOverlapGPDMA
Description : Perform 1D copy with no care of overlaps, with GPDMA method
Parameters  : source, destination, size
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void Copy1DNoOverlapGPDMA(void * const SrcAddr_p, void * const DestAddr_p, const U32 Size)
{
    U32 ToAddr, FromAddr, AlignedWidth, j;
    U8 NonAlignedBytesLeft, NonAlignedBytesRight, Unit;
    STGPDMA_DmaTransfer_t DmaTransfer_s;
    STGPDMA_DmaTransferId_t TransferId;
    STGPDMA_DmaTransferStatus_t Status;
    U32 Timeout;
    ST_ErrorCode_t Err;

    FromAddr = (U32) SrcAddr_p;
    ToAddr   = (U32) DestAddr_p;
    NonAlignedBytesLeft = 0;
    NonAlignedBytesRight = 0;
    AlignedWidth = Size;

    /* GPDMA most powerful for 32 bytes Unit, so source and destination addresses can be aligned on 32 bytes the same way. */
    Unit = 32;

    while ((FromAddr & (Unit-1)) != (ToAddr & (Unit-1)))
    {
        Unit >>= 1;
    }

    /* If Addresses not aligned on Unit bytes, the left of the area will have
       to be copied to align the address of the rest to copy */
    while ((FromAddr & (Unit-1)) != 0)
    {
        NonAlignedBytesLeft ++;
        FromAddr ++;
        ToAddr ++;
        /* Decrement AlignedWidth but don't become negative ! */
        if (AlignedWidth != 0)
        {
            AlignedWidth --;
        }
    }

    /* If AlignedWidth not aligned on Unit bytes, right of the area will have
       to be copied to align the width of the rest to copy */
    while ((AlignedWidth & (Unit-1)) != 0)
    {
        NonAlignedBytesRight ++;
        AlignedWidth --;
    }

    if (AlignedWidth > 0)
    {
        /* GPDMA can transfert up to 4GB, so no need to cut in blocks */
        DmaTransfer_s.TimingModel              = STGPDMA_TIMING_FREE_RUNNING;
        DmaTransfer_s.Source.TransferType      = STGPDMA_TRANSFER_1D_INCREMENTING;
        DmaTransfer_s.Source.Address           = FromAddr;
        DmaTransfer_s.Source.UnitSize          = Unit;
        DmaTransfer_s.Destination.TransferType = STGPDMA_TRANSFER_1D_INCREMENTING;
        DmaTransfer_s.Destination.Address      = ToAddr;
        DmaTransfer_s.Destination.UnitSize     = Unit;
        DmaTransfer_s.Count                    = AlignedWidth;
        DmaTransfer_s.Next                     = NULL;

        Timeout = 1000 * (1+ (DmaTransfer_s.Count >> 23)); /* 2^23 = 8Mb : worst case (slowest) copy rate per second */

        /* Wait for the GPDMA locking semaphore to be free: use block only below this limit ! */
        STOS_SemaphoreWait(stavmem_LockGPDMA_p);

        Err = STGPDMA_DmaStartChannel( stavmem_MemAccessDevice.GpdmaHandle,
                                    STGPDMA_MODE_BLOCK,
                                    &DmaTransfer_s,
                                    1, /* NumberTransfers */
                                    0, /* EventLine : Ignored for free running */
                                    Timeout, /* timeout in millisecond */
                                    &TransferId,
                                    &Status
                                    );

        /* Free the GPDMA locking semaphore: use block only above this limit ! */
        STOS_SemaphoreSignal(stavmem_LockGPDMA_p);

        if (Err != ST_NO_ERROR) /* Rmq : Err = Status.Error */
        {
            stavmem_MemAccessDevice.LastGpdmaTranfertFailed = TRUE;
            stavmem_MemAccessDevice.TranfertFailed = TRUE;
        }
    } /* if (AlignedWidth > 0) */

    /* Copy remaining left bytes if there are some */
    if (NonAlignedBytesLeft != 0)
    {
        for (j = 0; j < NonAlignedBytesLeft; j++)
        {
            *(((U8 *) DestAddr_p) + j) = *(((U8 *) SrcAddr_p) + j);
        }
        NonAlignedBytesLeft = 0;
    }

    /* Copy remaining right bytes if there are some */
    if (NonAlignedBytesRight != 0)
    {
        for (j = 0; j < NonAlignedBytesRight; j++)
        {
            *(((U8 *) DestAddr_p) + Size - j - 1) = *(((U8 *) SrcAddr_p) + Size - j - 1);
        }
        NonAlignedBytesRight = 0;
    }

} /* end of Copy1DNoOverlapGPDMA() */


/*******************************************************************************
Name        : Copy2DNoOverlapGPDMA
Description : Perform 2D copy with no care of overlaps, with GPDMA method
Parameters  : source, dimensions, source pitch, destination, destination pitch
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void Copy2DNoOverlapGPDMA(void * const SrcAddr_p,  const U32 SrcWidth, const U32 SrcHeight, const U32 SrcPitch,
                                               void * const DestAddr_p, const U32 DestPitch)
{
    U32 ToAddr, FromAddr, AlignedWidth, i, j;
    U8 NonAlignedBytesLeft, NonAlignedBytesRight, Unit;
    STGPDMA_DmaTransfer_t DmaTransfer_s;
    STGPDMA_DmaTransferId_t TransferId;
    STGPDMA_DmaTransferStatus_t Status;
    U32 Timeout;
    ST_ErrorCode_t Err;

    FromAddr = (U32) SrcAddr_p;
    ToAddr   = (U32) DestAddr_p;
    NonAlignedBytesLeft = 0;
    NonAlignedBytesRight = 0;
    AlignedWidth = SrcWidth;

    /* GPDMA most powerful for 32 bytes Unit, so if pitch is multiple of 32 bytes,
    if source and destination addresses can be aligned on 32 bytes the same way. */
    Unit = 32;

    while (    (((SrcHeight > 1) && (SrcPitch & (Unit-1)) != 0))
            || (((DestPitch > 1) && (DestPitch & (Unit-1)) != 0))
            || (((SrcPitch & (Unit-1)) != (DestPitch & (Unit-1))))
            || ((FromAddr & (Unit-1)) != (ToAddr & (Unit-1))))
    {
        Unit >>= 1;
    }

    /* If Addresses not aligned on Unit bytes, the left of the area will have
       to be copied to align the address of the rest to copy */
    while ((FromAddr & (Unit-1)) != 0)
    {
        NonAlignedBytesLeft ++;
        FromAddr ++;
        ToAddr ++;
        /* Decrement AlignedWidth but don't become negative ! */
        if (AlignedWidth != 0)
        {
            AlignedWidth --;
        }
    }

    /* If AlignedWidth not aligned on Unit bytes, right of the area will have
       to be copied to align the width of the rest to copy */
    while ((AlignedWidth & (Unit-1)) != 0)
    {
        NonAlignedBytesRight ++;
        AlignedWidth --;
    }

    if (AlignedWidth > 0)
    {
        /* GPDMA can transfert up to 4GB, so no need to cut in blocks */
        DmaTransfer_s.TimingModel              = STGPDMA_TIMING_FREE_RUNNING;
        DmaTransfer_s.Source.TransferType      = STGPDMA_TRANSFER_2D_INCREMENTING;
        DmaTransfer_s.Source.Address           = FromAddr;
        DmaTransfer_s.Source.UnitSize          = Unit;
        DmaTransfer_s.Source.Length            = AlignedWidth;
        DmaTransfer_s.Source.Stride            = SrcPitch;
        DmaTransfer_s.Destination.TransferType = STGPDMA_TRANSFER_2D_INCREMENTING;
        DmaTransfer_s.Destination.Address      = ToAddr;
        DmaTransfer_s.Destination.UnitSize     = Unit;
        DmaTransfer_s.Destination.Length       = AlignedWidth;
        DmaTransfer_s.Destination.Stride       = DestPitch;
        DmaTransfer_s.Count                    = AlignedWidth*SrcHeight;
        DmaTransfer_s.Next                     = NULL;

        Timeout = 1000 * (1+ (DmaTransfer_s.Count >> 23)); /* 2^23 = 8Mb : worst case (slowest) copy rate per second */

        /* Wait for the GPDMA locking semaphore to be free: use block only below this limit ! */
        STOS_SemaphoreWait(stavmem_LockGPDMA_p);

        Err = STGPDMA_DmaStartChannel( stavmem_MemAccessDevice.GpdmaHandle,
                                    STGPDMA_MODE_BLOCK,
                                    &DmaTransfer_s,
                                    1, /* NumberTransfers */
                                    0, /* EventLine : Ignored for free running */
                                    Timeout, /* timeout in millisecond */
                                    &TransferId,
                                    &Status
                                    );

        /* Free the GPDMA locking semaphore: use block only above this limit ! */
        STOS_SemaphoreSignal(stavmem_LockGPDMA_p);

        if (Err != ST_NO_ERROR) /* Rmq : Err = Status.Error */
        {
            stavmem_MemAccessDevice.LastGpdmaTranfertFailed = TRUE;
            stavmem_MemAccessDevice.TranfertFailed = TRUE;
        }
    } /* if (AlignedWidth > 0) */

    /* Copy remaining left bytes if there are some */
    if (NonAlignedBytesLeft != 0)
    {
        for (j = 0; j < NonAlignedBytesLeft; j++)
        {
            for (i = 0; i < SrcHeight; i++)
            {
                *(((U8 *) DestAddr_p) + (i * DestPitch) + j) = *(((U8 *) SrcAddr_p) + (i * SrcPitch) + j);
            }
        }
        NonAlignedBytesLeft = 0;
    }

    /* Copy remaining right bytes if there are some */
    if (NonAlignedBytesRight != 0)
    {
        for (j = 0; j < NonAlignedBytesRight; j++)
        {
            for (i = 0; i < SrcHeight; i++)
            {
                *(((U8 *) DestAddr_p) + (i * DestPitch) + SrcWidth - j - 1) = *(((U8 *) SrcAddr_p) + (i * SrcPitch) + SrcWidth - j - 1);
            }
        }
        NonAlignedBytesRight = 0;
    }

} /* end of Copy2DNoOverlapGPDMA() */


/*******************************************************************************
Name        : Fill1DGPDMA
Description : Perform 1D fill with GPDMA method
Parameters  : pattern, pattern size, destination, destination size
Assumptions :
Limitations :
Returns     : Nothing: always successful
*******************************************************************************/
static void Fill1DGPDMA(void * const Pattern_p,  const U32 PatternSize,  void * const DestAddr_p, const U32 DestSize)
{
    U32 IndexDest;

    /* we don't know yet how many transfers are required, so to build a DMA chain we would
       make a dynamic allocation of size (NumberTransfers * sizeof(STGPDMA_DmaTransfer_t))
       which can be a big size (e.g. : 1Kb PatternSize, 8MB DestSize => 8192 transfers
       So no DMA chain is built.
    */
    IndexDest = 0;
    while ( ((DestSize - IndexDest) >= PatternSize) && (!stavmem_MemAccessDevice.LastGpdmaTranfertFailed))
    {
        Copy1DNoOverlapGPDMA(Pattern_p, (void *) (((U8 *) DestAddr_p) + IndexDest), PatternSize);
        IndexDest += PatternSize;
    }
    if ( ((DestSize - IndexDest) > 0) && (!stavmem_MemAccessDevice.LastGpdmaTranfertFailed))
    {
        Copy1DNoOverlapGPDMA(Pattern_p, (void *) (((U8 *) DestAddr_p) + IndexDest), DestSize - IndexDest);
    }

} /* end of Fill1DGPDMA() */


/*******************************************************************************
Name        : Fill2DGPDMA
Description : Perform 2D fill with GPDMA method
Parameters  : pattern, pattern characteristics, destination, destination  characteristics
Assumptions :
Limitations :
Returns     : Nothing: always successful
*******************************************************************************/
static void Fill2DGPDMA(void * const Pattern_p,  const U32 PatternWidth, const U32 PatternHeight, const U32 PatternPitch,
                                      void * const DestAddr_p, const U32 DestWidth, const U32 DestHeight, const U32 DestPitch)
{
    U32 HorizontalIndexDest, VerticalIndexDest;

    /* we don't know yet how many transfers are required, so to build a DMA chain we would
       make a dynamic allocation of size (NumberTransfers * sizeof(STGPDMA_DmaTransfer_t))
       which can be a big size.
       So no DMA chain is built.
    */

    VerticalIndexDest = 0;
    while ((DestHeight - VerticalIndexDest >= PatternHeight ) && (!stavmem_MemAccessDevice.LastGpdmaTranfertFailed))
    {
        HorizontalIndexDest = 0;
        while (((DestWidth - HorizontalIndexDest) >= PatternWidth) && (!stavmem_MemAccessDevice.LastGpdmaTranfertFailed))
        {
            /* fill with full pattern */
            Copy2DNoOverlapGPDMA( Pattern_p, PatternWidth, PatternHeight, PatternPitch,
                                  (void *) (((U8 *) DestAddr_p) + (VerticalIndexDest * DestPitch) + HorizontalIndexDest),
                                  DestPitch);
            HorizontalIndexDest += PatternWidth;
        }
        if (((DestWidth - HorizontalIndexDest) > 0) && (!stavmem_MemAccessDevice.LastGpdmaTranfertFailed))
        {
            /* fill with partial pattern, remaining space at right of n horizontal previous patterns fully filled */
            Copy2DNoOverlapGPDMA( Pattern_p, DestWidth - HorizontalIndexDest, PatternHeight, PatternPitch,
                                  (void *) (((U8 *) DestAddr_p) + (VerticalIndexDest * DestPitch) + HorizontalIndexDest),
                                  DestPitch);
        }
        VerticalIndexDest += PatternHeight;
    }

    /* fill with partial patterns, remaining spaces at bottom of n vertical previous patterns fully filled */
    if ( ((DestHeight - VerticalIndexDest) > 0) && (!stavmem_MemAccessDevice.LastGpdmaTranfertFailed))
    {
        HorizontalIndexDest = 0;
        while (((DestWidth - HorizontalIndexDest) >= PatternWidth) && (!stavmem_MemAccessDevice.LastGpdmaTranfertFailed))
        {
            Copy2DNoOverlapGPDMA( Pattern_p, PatternWidth, DestHeight - VerticalIndexDest, PatternPitch,
                                    (void *) (((U8 *) DestAddr_p) + (VerticalIndexDest * DestPitch) + HorizontalIndexDest),
                                    DestPitch);
            HorizontalIndexDest += PatternWidth;
        }
        if ( ((DestWidth - HorizontalIndexDest) > 0) && (!stavmem_MemAccessDevice.LastGpdmaTranfertFailed))
        {
            /* fill with last partial pattern, remaining space at bottom and right of n vertical and horizontal previous patterns partially filled */
            Copy2DNoOverlapGPDMA( Pattern_p, DestWidth - HorizontalIndexDest, DestHeight - VerticalIndexDest, PatternPitch,
                                    (void *) (((U8 *) DestAddr_p) + (VerticalIndexDest * DestPitch) + HorizontalIndexDest),
                                    DestPitch);
        }
    }
} /* end of Fill2DGPDMA() */

/* End of acc_gpd.c */





















