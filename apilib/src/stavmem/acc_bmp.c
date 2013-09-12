/*******************************************************************************

File name : acc_bmp.c

Description : AVMEM memory access file for block move DMA peripheral

COPYRIGHT (C) STMicroelectronics 2003.

Date               Modification                                     Name
----               ------------                                     ----
 8 Dec 1999         Created                                          HG
18 Apr 01           OS40 support                                     HSdLM
01 Jun 2001         Add stavmem_ before non API exported symbols     HSdLM
19 Nov 2001         Try to fix DDTS GNBvd09242 (Block Move DMA       HSdLM
 *                  issue)
19 May 2003         Fix DDTS GNBvd21735 "[...] interrupt by number"  HSdLM
*******************************************************************************/


/* Includes ----------------------------------------------------------------- */

#include <string.h>

#include "stddefs.h"
#include "stsys.h"
#include "sttbx.h"

#include "stavmem.h"
#include "avm_acc.h"
#include "acc_bmp.h"


/* Private Types ------------------------------------------------------------ */


/* Private Constants -------------------------------------------------------- */

/*#define STAVMEM_BMDMA_WITH_INTERRUPT */

/* Definitions for block move DMA */
#define BMDMAABORT_OFFSET           0x18
#define BMDMACOUNT_OFFSET           0x8
#define BMDMADESTADDRESS_OFFSET     0x4
#define BMDMAINTACK_OFFSET          0x14
#define BMDMAINTEN_OFFSET           0xC
#define BMDMASRCADDRESS_OFFSET      0x0
#define BMDMASTATUS_OFFSET          0x10

/* Private Variables (static)------------------------------------------------ */

static semaphore_t *stavmem_LockBlockMovePeripheral_p;   /* Block move DMA locking semaphore */
static BOOL BlockMovePeripheralInitialised = FALSE;


/* Private Macros ----------------------------------------------------------- */

#define STAVMEM_BMBase      ((U32) stavmem_MemAccessDevice.BlockMoveDmaBaseAddr_p)

#define STAVMEM_BMPWriteReg32(AddrFromBase, Val) STSYS_WriteRegDev32LE(((void *) (STAVMEM_BMBase + (AddrFromBase))), (U32) (Val))
#define STAVMEM_BMPWriteReg16(AddrFromBase, Val) STSYS_WriteRegDev16LE(((void *) (STAVMEM_BMBase + (AddrFromBase))), (U16) (Val))
#define STAVMEM_BMPWriteReg8(AddrFromBase,  Val) STSYS_WriteRegDev8(((void *)    (STAVMEM_BMBase + (AddrFromBase))), (U8) (Val))
#define STAVMEM_BMPReadReg8(AddrFromBase)        STSYS_ReadRegDev8(((void *)     (STAVMEM_BMBase + (AddrFromBase))))


/* Private Function prototypes ---------------------------------------------- */

static void InitBlockMovePeripheral(void);
#ifdef STAVMEM_BMDMA_WITH_INTERRUPT
#if defined(ST_OS21) || defined(ST_OSWINCE)
static int BlockMovePeripheralInterruptHandler (void);
#endif  /* ST_OS21 */
#ifdef ST_OS20
static void BlockMovePeripheralInterruptHandler (void);
#endif /* ST_OS21 */

#endif
static void Copy1DNoOverlapBlockMovePeripheral(void * const SrcAddr_p, void * const DestAddr_p, const U32 Size);
static void Copy2DNoOverlapBlockMovePeripheral(void * const SrcAddr_p,  const U32 SrcWidth, const U32 SrcHeight, const U32 SrcPitch,
                                               void * const DestAddr_p, const U32 DestPitch);
static void Fill1DBlockMovePeripheral(void * const Pattern_p,  const U32 PatternSize,  void * const DestAddr_p, const U32 DestSize);
static void Fill2DBlockMovePeripheral(void * const Pattern_p,  const U32 PatternWidth, const U32 PatternHeight, const U32 PatternPitch,
                                      void * const DestAddr_p, const U32 DestWidth, const U32 DestHeight, const U32 DestPitch);


stavmem_MethodOperations_t stavmem_BlockMovePeripheral = {
    InitBlockMovePeripheral,                        /* Private function */
    /* Block move peripheral only works when there is no overlap */
    {
        Copy1DNoOverlapBlockMovePeripheral,         /* Private function */
        NULL,                                       /* Not supported */
        Copy2DNoOverlapBlockMovePeripheral,         /* Private function */
        NULL,                                       /* Not supported */
        Fill1DBlockMovePeripheral,                  /* Private function */
        Fill2DBlockMovePeripheral                   /* Private function */
    },
    /* No need to check: block move peripheral is always successful */
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
Name        : InitBlockMovePeripheral
Description : Init semaphore used for memory accesses with block move DMA peripheral
Parameters  : None
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void InitBlockMovePeripheral(void)
{
    static BOOL BlockMovePeripheralSemaphoreInitialised = FALSE;

    /* Protect semaphores initialisation */
    STOS_TaskLock();
    if (!BlockMovePeripheralSemaphoreInitialised)
    {
        stavmem_LockBlockMovePeripheral_p = STOS_SemaphoreCreateFifo(NULL,1);   /* Init Block move DMA peripheral locking semaphore */

#ifdef STAVMEM_BMDMA_WITH_INTERRUPT
        /* install the interrupt handler */
        STOS_InterruptInstall( BLOCK_MOVE_INTERRUPT, BLOCKMOVE_INTERRUPT_LEVEL, (void(*)(void*))BlockMovePeripheralInterruptHandler, NULL);

        STOS_InterruptEnable(BLOCK_MOVE_INTERRUPT,BLOCKMOVE_INTERRUPT_LEVEL);

#endif /* STAVMEM_BMDMA_WITH_INTERRUPT */
        BlockMovePeripheralSemaphoreInitialised = TRUE;
    }
    /* Un-protect semaphores initialisation */
    STOS_TaskUnlock();

    /* Caution, deletion will never occur ! */
    /* Delete block move peripheral locking semaphore */
/*    semaphore_delete(stavmem_LockBlockMovePeripheral_p);*/

    BlockMovePeripheralInitialised = TRUE;
}


#ifdef STAVMEM_BMDMA_WITH_INTERRUPT
/****************************************************************************
Name: BlockMovePeripheralInterruptHandler
Description:
    routine that clears the pending Interrupts, and reset the semaphore
    counter
****************************************************************************/
#if defined(ST_OS21) || defined(ST_OSWINCE)
static int BlockMovePeripheralInterruptHandler (void)
#endif  /* ST_OS21 */
#ifdef ST_OS20
static void BlockMovePeripheralInterruptHandler (void)
#endif /* ST_OS21 */
{
    /* acknowledge interrupt */
    STAVMEM_BMPWriteReg32(BMDMAINTACK_OFFSET, 1);

    /* signal that transfer is finished */
    STOS_SemaphoreSignal(stavmem_LockBlockMovePeripheral_p);
#if defined(ST_OS21) || defined(ST_OSWINCE)
    return(OS21_SUCCESS);
#endif
}
#endif

/*******************************************************************************
Name        : Copy1DNoOverlapBlockMovePeripheral
Description : Perform 1D copy with no care of overlaps, with block move peripheral method
Parameters  : source, destination, size
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void Copy1DNoOverlapBlockMovePeripheral(void * const SrcAddr_p, void * const DestAddr_p, const U32 Size)
{
#ifdef ST_OS20
    #pragma ST_device (DeviceCountAddr_p)
#endif /*ST_OS20*/
    volatile U32 *DeviceCountAddr_p = (U32 *) (STAVMEM_BMBase + BMDMACOUNT_OFFSET);
    U32 DecSize = Size;
    U32 ToAddr = ((U32) DestAddr_p);
    U32 FromAddr = ((U32) SrcAddr_p);
    U8  Status;

    /* If first time, initialise semaphore */
/*    if (!BlockMovePeripheralInitialised)
    {
        InitBlockMovePeripheral();
    }*/

    /* Wait for the Block move DMA locking semaphore to be free: use block only below this limit ! */
    STOS_SemaphoreWait(stavmem_LockBlockMovePeripheral_p);

    /* Enable block move DMA interrupt */
    STAVMEM_BMPWriteReg8(BMDMAINTEN_OFFSET, 1);
    /* acknowledge potential old interrupt that could block transfer */
    STAVMEM_BMPWriteReg32(BMDMAINTACK_OFFSET, 1);

    /* If size is greater than max of block move DMA, cut in blocks of this size  */
    while (DecSize >= 65535)
    {
#ifndef STAVMEM_BMDMA_WITH_INTERRUPT
        /* acknowledge last interrupt that could block transfer */
        STAVMEM_BMPWriteReg32(BMDMAINTACK_OFFSET, 1);
#endif
        STAVMEM_BMPWriteReg32(BMDMASRCADDRESS_OFFSET, FromAddr);
        STAVMEM_BMPWriteReg32(BMDMADESTADDRESS_OFFSET, ToAddr);

        /* Cannot use STSYS functions for this register access, as it launches the copy on access
           to the MSB, and direction of copy cannot be assumed with STSYS functions */
        *DeviceCountAddr_p = 65535;

#ifdef STAVMEM_BMDMA_WITH_INTERRUPT
        /* Wait BlockMovePeripheralInterruptHandler signal end of transfer */
        STOS_SemaphoreWait(stavmem_LockBlockMovePeripheral_p);
#else
        Status = STAVMEM_BMPReadReg8(BMDMASTATUS_OFFSET);
        while ((Status & 0x01) == 0)
        {
            STOS_TaskDelay(1); /* Wait end of transfer */
            Status = STAVMEM_BMPReadReg8(BMDMASTATUS_OFFSET);
        }
#endif

        DecSize -= 65535;
        FromAddr += 65535;
        ToAddr += 65535;
    }

    /* After copy of blocks of max size of block move DMA, copy the remaining bytes */
    if (DecSize > 0)
    {
        /* Choice is done to process by pointers access for smaller sizes, for the following reasons:
           - block move DMA doesn't support 2 bytes copy on address aligned on 1, this must be fixed by SW
           - block move DMA setup requires 5 registers access */
        if (DecSize <= 10)
        {
            while (DecSize > 0)
            {
                DecSize--;
                *(((U8 *) ToAddr) + DecSize) = *(((U8 *) FromAddr) + DecSize);
            }
        }
        else
        {
            STAVMEM_BMPWriteReg32(BMDMASRCADDRESS_OFFSET, FromAddr);
            STAVMEM_BMPWriteReg32(BMDMADESTADDRESS_OFFSET, ToAddr);

            /* Cannot use STSYS functions for this register access, as it launches the copy on access
               to the MSB, and direction of copy cannot be assumed with STSYS functions */
            *DeviceCountAddr_p = DecSize;

#ifdef STAVMEM_BMDMA_WITH_INTERRUPT
            /* Wait BlockMovePeripheralInterruptHandler signal end of transfer */
            STOS_SemaphoreWait(stavmem_LockBlockMovePeripheral_p);
#else
            Status = STAVMEM_BMPReadReg8(BMDMASTATUS_OFFSET);
            while ((Status & 0x01) == 0)
            {
                STOS_TaskDelay(1); /* Wait end of transfer */
                Status = STAVMEM_BMPReadReg8(BMDMASTATUS_OFFSET);
            }
#endif
        }
    }

    /* Free the Block move DMA locking semaphore: use block only above this limit ! */
    STOS_SemaphoreSignal(stavmem_LockBlockMovePeripheral_p);
}


/*******************************************************************************
Name        : Copy2DNoOverlapBlockMovePeripheral
Description : Perform 2D copy with no care of overlaps, with block move peripheral method
Parameters  : source, dimensions, source pitch, destination, destination pitch
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void Copy2DNoOverlapBlockMovePeripheral(void * const SrcAddr_p,  const U32 SrcWidth, const U32 SrcHeight, const U32 SrcPitch,
                                               void * const DestAddr_p, const U32 DestPitch)
{
    U32 y;

    for (y = 0; y < SrcHeight; y++)
    {
        Copy1DNoOverlapBlockMovePeripheral((void *) (((U8 *) SrcAddr_p) + (y * SrcPitch)), (void *) (((U8 *) DestAddr_p) + (y * DestPitch)), SrcWidth);
    }
}


/*******************************************************************************
Name        : Fill1DBlockMovePeripheral
Description : Perform 1D fill with block move peripheral method
Parameters  : pattern, pattern size, destination, destination size
Assumptions :
Limitations :
Returns     : Nothing: always successful
*******************************************************************************/
static void Fill1DBlockMovePeripheral(void * const Pattern_p,  const U32 PatternSize,  void * const DestAddr_p, const U32 DestSize)
{
    U32 IndexDest;

    IndexDest = 0;
    while ((DestSize - IndexDest) > PatternSize)
    {
        Copy1DNoOverlapBlockMovePeripheral(Pattern_p, (void *) (((U8 *) DestAddr_p) + IndexDest), PatternSize);
        IndexDest += PatternSize;
    }
    Copy1DNoOverlapBlockMovePeripheral(Pattern_p, (void *) (((U8 *) DestAddr_p) + IndexDest), DestSize - IndexDest);
}


/*******************************************************************************
Name        : Fill2DBlockMovePeripheral
Description : Perform 2D fill with block move peripheral method
Parameters  : pattern, pattern characteristics, destination, destination  characteristics
Assumptions :
Limitations :
Returns     : Nothing: always successful
*******************************************************************************/
static void Fill2DBlockMovePeripheral(void * const Pattern_p,  const U32 PatternWidth, const U32 PatternHeight, const U32 PatternPitch,
                                      void * const DestAddr_p, const U32 DestWidth, const U32 DestHeight, const U32 DestPitch)
{
    U32 Line, PatLine, IndexDest;

    PatLine = 0;
    for (Line = 0; Line < DestHeight; Line++)
    {
        IndexDest = 0;
        while ((DestWidth - IndexDest) > PatternWidth)
        {
            Copy1DNoOverlapBlockMovePeripheral((void *) (((U8 *) Pattern_p) + (PatLine * PatternPitch)), (void *) (((U8 *) DestAddr_p) + (Line * DestPitch) + IndexDest), PatternWidth);
            IndexDest += PatternWidth;
        }
        Copy1DNoOverlapBlockMovePeripheral((void *) (((U8 *) Pattern_p) + (PatLine * PatternPitch)), (void *) (((U8 *) DestAddr_p) + (Line * DestPitch) + IndexDest), DestWidth - IndexDest);

        PatLine++;
        if (PatLine == PatternHeight)
        {
            PatLine = 0;
        }
    }
}



/* End of acc_bmp.c */



