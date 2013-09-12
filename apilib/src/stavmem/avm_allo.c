/*******************************************************************************

File name : avm_allo.c

Description : Audio and video memory module memory allocation features

COPYRIGHT (C) STMicroelectronics 2003.

Date                Modification                                     Name
----                ------------                                     ----
17 Jun 1999         Created                                          HG
06 Oct 1999         Introduce partitions functions                   HG
18 Apr 2001         OS40 support                                     HSdLM
05 Jun 2001         Add stavmem_ before non API exported symbols     HSdLM
 *                  Correct BlockHandle use in code
 *                  Set multi-thread safety for Create / Delete
 *                  partitions.
19 Nov 2001         Fix DDTS GNBvd09815 (STAVMEM_FreeBlock issue)    HSdLM
20 Nov 2002         Add STAVMEM_DEBUG_BLOCKS_INFO option             HSdLM
17 Apr 2003         Multi-partitions support                         HSdLM
*******************************************************************************/


/* Private Definitions (internal use only) ---------------------------------- */

/*#define COMPRESS_AREA
#define FREE_AREA
#define FREE_ALL*/
#define GET_LARGEST_FREE_BLOCK_SIZE

#define USE_SEMAPHORE
#ifndef ST_OSLINUX
#ifdef STAVMEM_DEBUG_MEMORY_STATUS
#define STTBX_PRINT
#endif
#endif





/*#define DEBUG_CODE*/


/* Includes ----------------------------------------------------------------- */

#ifndef ST_OSLINUX
#include <string.h>
#include <stdlib.h>
#endif

#include "stddefs.h"

/*#ifndef ST_OSLINUX*/
#include "sttbx.h"
/*#endif */       /* ST_OSLINUX */

#include "stos.h"
#include "stavmem.h"
#include "avm_init.h"
#include "avm_allo.h"


/* Private Types ------------------------------------------------------------ */


/* Private Constants -------------------------------------------------------- */

#define MAX_SIZE_ALIGNING_MASK  0xff    /* Limitation of the aligning mask for the size */


/* Private Variables (static)------------------------------------------------ */


/* Public Variables --------------------------------------------------------- */

stavmem_Partition_t stavmem_PartitionArray[STAVMEM_MAX_MAX_PARTITION];


/* Private Macros ----------------------------------------------------------- */


/* Private Function prototypes ---------------------------------------------- */

static BOOL RoundUpAlignment(U32 *Alignment_p);

static ST_ErrorCode_t UnReserveBlockNoSemaphore(stavmem_Partition_t *Partition_p, STAVMEM_MemoryRange_t *SubSpace_p);

static void InsertBlockAboveBlock(MemBlockDesc_t **VeryTopBlock_p, MemBlockDesc_t *InsertBlock_p, MemBlockDesc_t *ListBlock_p);
static void InsertBlockBelowBlock(MemBlockDesc_t **VeryBottomBlock_p, MemBlockDesc_t *InsertBlock_p, MemBlockDesc_t *ListBlock_p);
static void RemoveBlock(MemBlockDesc_t **VeryTopBlock_p, MemBlockDesc_t **VeryBottomBlock_p, MemBlockDesc_t *ListBlock_p);

static MemBlockDesc_t *AllocBlockDescriptor(MemBlockDesc_t **TopOfFreeBlocksList_p);
static void FreeBlockDescriptor(MemBlockDesc_t **TopOfSpaceFreeBlocksList_p, MemBlockDesc_t *Mem_p);


static ST_ErrorCode_t MarkForbiddenRangesAndBorders(stavmem_Partition_t *Partition_p, const U32 NumberOfForbiddenRanges, STAVMEM_MemoryRange_t *NotOrderedForbiddenRangeArray_p, const U32 NumberOfForbiddenBorders, void **NotOrderedForbiddenBorderArray_p);
static void UnMarkForbiddenRangesAndBorders(stavmem_Partition_t *Partition_p);
static BOOL IsInForbiddenRangeOrBorder(const void *StartAddr_p, const void *StopAddr_p, const U32 NumberOfForbiddenRanges, STAVMEM_MemoryRange_t *NotOrderedForbiddenRangeArray_p, const U32 NumberOfForbiddenBorders, void **NotOrderedForbiddenBorderArray_p);

static ST_ErrorCode_t AllocBlockNoSemaphore(const STAVMEM_AllocBlockParams_t *AllocBlockParams_p, STAVMEM_BlockHandle_t *BlockHandle_p);
static ST_ErrorCode_t FreeNoSemaphore(stavmem_Partition_t *Partition_p, STAVMEM_BlockHandle_t BlockHandle);

__inline static BOOL BlockHandleBelongsToSpace(STAVMEM_BlockHandle_t BlockHandle, stavmem_Device_t Device);

/* Functions ---------------------------------------------------------------- */

/* exported function used in test harness */
ST_ErrorCode_t STAVMEM_GetLargestFreeBlockSize(STAVMEM_PartitionHandle_t PartitionHandle, STAVMEM_AllocMode_t BlockAllocMode, U32 *LargestFreeBlockSize_p);

/*
--------------------------------------------------------------------------------
Create a memory partition in an initialised device
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STAVMEM_CreatePartition(const ST_DeviceName_t DeviceName,
                                       const STAVMEM_CreatePartitionParams_t *CreateParams_p,
                                       STAVMEM_PartitionHandle_t *PartitionHandle_p)
{


    U32 i, j, k;
    stavmem_Device_t *Device_p;
    stavmem_Partition_t *Partition_p;
    MemBlockDesc_t *FirstBlock_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;


    /* Exit now if parameters are bad */
    if (   (CreateParams_p == NULL)
        || (PartitionHandle_p == NULL)
        || (CreateParams_p->PartitionRanges_p == NULL)       /* Partition memory map cannot be empty */
        || (!(CreateParams_p->NumberOfPartitionRanges > 0))) /* At least one range must be specified */
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Check if device already initialised and return error if not so */
    Device_p = stavmem_GetPointerOnDeviceNamed(DeviceName);
    if (Device_p == NULL)
    {
        /* Device name not found */
        return(ST_ERROR_UNKNOWN_DEVICE);
    }

#ifdef USE_SEMAPHORE
    /* Wait for the allocations locking semaphore to be free */
    STOS_SemaphoreWait(Device_p->LockAllocSemaphore_p);
#endif

    /* Look for a free partition and return error if none is free */
    j = 0;
    while ((stavmem_PartitionArray[j].Validity == AVMEM_VALID_PARTITION) && (j < STAVMEM_MAX_MAX_PARTITION))
    {
        j++;
    }
    if (j >= STAVMEM_MAX_MAX_PARTITION)
    {
        /* None of the partitions is free */
        ErrorCode = STAVMEM_ERROR_MAX_PARTITION;
    }

    if (ErrorCode == ST_NO_ERROR)
    {

        Partition_p = &stavmem_PartitionArray[j];
        Partition_p->Device_p = Device_p; /* 'inherits' device characteristics */
        k = Device_p->TotalAllocatedBlocks / Device_p->MaxPartitions; /* Total number of blocks per partition  */

        /* First block is at the first address of allocated memory */
        FirstBlock_p = (MemBlockDesc_t *) Device_p->MemoryAllocated_p;
        if (j > 0)
        {
            FirstBlock_p = &FirstBlock_p[j*k];
        }

        /* Initialise first block */
        FirstBlock_p->BlockAbove_p = NULL;
        FirstBlock_p->BlockBelow_p = NULL;
        FirstBlock_p->Alignment = 1;
        FirstBlock_p->StartAddr_p = (void *) 0;
        FirstBlock_p->AlignedStartAddr_p = (void *) 0;
        FirstBlock_p->Size = 0;     /* 0 means full-range size: all space is considered */
        FirstBlock_p->UserSize = FirstBlock_p->Size;
        FirstBlock_p->AllocMode = STAVMEM_ALLOC_MODE_RESERVED;  /* All space is reserved before un-reservation */
        FirstBlock_p->IsUsed = TRUE;

        for (i = 1; i < k-1; i++)
        {
            FirstBlock_p[i].BlockBelow_p = &FirstBlock_p[i + 1];
            FirstBlock_p[i].AllocMode = STAVMEM_ALLOC_MODE_INVALID;
        }
        /* Write end of list pointer */
        FirstBlock_p[1].BlockAbove_p   = NULL;
        FirstBlock_p[k-1].BlockBelow_p = NULL;

        Partition_p->TopOfFreeBlocksList_p = &FirstBlock_p[1];
        Partition_p->VeryTopBlock_p        = FirstBlock_p;
        Partition_p->VeryBottomBlock_p     = FirstBlock_p;

        /* Initialise the space as a list of not used / reserved blocks */
        /* For this, start for the reserved entire space and un-reserve sub-spaces */
        for (i = 0; i < CreateParams_p->NumberOfPartitionRanges; i++)
        {
            /* Un-reserve */
            ErrorCode = UnReserveBlockNoSemaphore(Partition_p, &(CreateParams_p->PartitionRanges_p[i]));
            if (ErrorCode != ST_NO_ERROR)
            {
                /* Should not happend */
                return(ST_ERROR_NO_MEMORY);
            }
        }
        /* Register partition */
        Partition_p->Validity = AVMEM_VALID_PARTITION;

        /* Let handle be the address of the partition */
        *PartitionHandle_p = (STAVMEM_PartitionHandle_t)Partition_p;

        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Partition created on device '%s'", DeviceName));

    }

#ifdef USE_SEMAPHORE
    /* Free the allocations locking semaphore */
    STOS_SemaphoreSignal(Device_p->LockAllocSemaphore_p);
#endif
    return(ErrorCode);

} /* End of STAVMEM_CreatePartition() */


/*
--------------------------------------------------------------------------------
Delete a memory partition in an initialised device
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STAVMEM_DeletePartition(const ST_DeviceName_t DeviceName,
                                       const STAVMEM_DeletePartitionParams_t *DeleteParams_p,
                                       STAVMEM_PartitionHandle_t *PartitionHandle_p)
{
    MemBlockDesc_t *Block_p;
    BOOL FoundBlocks;
    stavmem_Partition_t *Partition_p;
    stavmem_Device_t *Device_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if ((DeleteParams_p == NULL) || (PartitionHandle_p == NULL))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Check if device already initialised and return error if not so */
    Device_p = stavmem_GetPointerOnDeviceNamed(DeviceName);
    if (Device_p == NULL)
    {
        /* Device name not found */
        return(ST_ERROR_UNKNOWN_DEVICE);
    }

#ifdef USE_SEMAPHORE
    /* Wait for the allocations locking semaphore to be free */
    STOS_SemaphoreWait(Device_p->LockAllocSemaphore_p);
#endif

    /* Get pointer to partition the block belongs to */
    Partition_p = (stavmem_Partition_t *) *PartitionHandle_p;

    /* Exit if the partition is not valid */
    if (Partition_p->Validity != AVMEM_VALID_PARTITION)
    {
        ErrorCode = STAVMEM_ERROR_INVALID_PARTITION_HANDLE;
    }
    else if (Partition_p->Device_p != Device_p)
    {
        /* Delete the partition only if the device corresponds */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
    }

    if (ErrorCode == ST_NO_ERROR)
    {
        /* Ensure the partition has no more block allocated. If there is, free them if required */
        Block_p = Partition_p->VeryTopBlock_p;
        FoundBlocks = FALSE;
        while ((Block_p != NULL) && (!FoundBlocks))
        {
            FoundBlocks = ((Block_p->IsUsed) && ((Block_p->AllocMode == STAVMEM_ALLOC_MODE_TOP_BOTTOM) || (Block_p->AllocMode == STAVMEM_ALLOC_MODE_BOTTOM_TOP)));
            Block_p = Block_p->BlockBelow_p;
        }
        Block_p = Partition_p->VeryTopBlock_p;
        if (FoundBlocks)
        {
            /* Some still allocated blocks were found: delete them of fail */
            if (DeleteParams_p->ForceDelete)
            {
                /* Delete all still allocated blocks */
                while (Block_p != NULL)
                {
                    if ((Block_p->IsUsed) && ((Block_p->AllocMode == STAVMEM_ALLOC_MODE_TOP_BOTTOM) || (Block_p->AllocMode == STAVMEM_ALLOC_MODE_BOTTOM_TOP)))
                    {
                        /* Free block */
                        FreeNoSemaphore(Partition_p, (STAVMEM_BlockHandle_t) Block_p);
                        /* Always success */
                    }
                    Block_p = Block_p->BlockBelow_p;
                }
            }
            else
            {
                /* Cannot delete partition: there are still allocated blocks */
                ErrorCode = STAVMEM_ERROR_ALLOCATED_BLOCK;
            }
        } /* if (FoundBlocks) */
    } /* if (ErrorCode == ST_NO_ERROR) */

    if (ErrorCode == ST_NO_ERROR)
    {
        /* Actions before deleting partition: loose partition blocks */
        Partition_p->VeryTopBlock_p = NULL;
        Partition_p->VeryBottomBlock_p = NULL;

        /* Un-register partition */
        Partition_p->Validity = ~AVMEM_VALID_PARTITION;

        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Partition deleted on device '%s'", Partition_p->Device_p->DeviceName));
    }

#ifdef USE_SEMAPHORE
    /* Free the allocations locking semaphore */
    STOS_SemaphoreSignal(Device_p->LockAllocSemaphore_p);
#endif

    return(ErrorCode);

} /* End of STAVMEM_DeletePartition() */



/*******************************************************************************
Name        : BlockHandleBelongsToSpace
Description : Tells if a block belongs to the initialised device
Parameters  : block handle and device
Assumptions :
Limitations :
Returns     : TRUE if block belongs to space, FALSE otherwise
*******************************************************************************/
__inline static BOOL BlockHandleBelongsToSpace(STAVMEM_BlockHandle_t BlockHandle, stavmem_Device_t Device)
{

    if ((((U32) BlockHandle) >= ((U32) Device.FirstBlock_p)) && /* Block handle address may be in range */
        (((U32) BlockHandle) <= (((U32) Device.FirstBlock_p) + (sizeof(MemBlockDesc_t) * (Device.TotalAllocatedBlocks - 1)))) && /* Block handle address is in range */
        (((((U32) BlockHandle) - ((U32) Device.FirstBlock_p)) % (sizeof(MemBlockDesc_t))) == 0) && /* Block handle address is valid (in range and aligned) */
        (((MemBlockDesc_t *) BlockHandle)->AllocMode != STAVMEM_ALLOC_MODE_INVALID)) /* Structure is not in list of available blocks */
    {
        return(TRUE);
    }

    return(FALSE);

}

#ifdef DEBUG_CODE
/*******************************************************************************
Name        : stavmem_BlockAndBip
Description : Bips and blocks with a loop. Change value to exit the loop
Parameters  : None
Assumptions :
Limitations :
Returns     : None
*******************************************************************************/
static void stavmem_BlockAndBip(void)
{
    volatile U32 val = 0;
    STTBX_Print(("\a\a"));

    while (val == 0)
    {
        ; /* change value in val to exit the loop ! */
    }
}

static boolean stavmem_ViewAllBlocksInfo(MemBlockDesc_t *VeryTopBlock_p, MemBlockDesc_t *VeryBottomBlock_p)
{
    U32 Address, Size;
    U32 CalculatedFreeSize = 0;
/*    U32 AskedFreeSize;*/
    char AllocModeStr[10];
    MemBlockDesc_t *Cur_p;
    boolean RetErr = TRUE;

    STTBX_Print(("\nMEMORY BLOCKS FROM BOTTOM:\n"));
    Cur_p = VeryBottomBlock_p;
    while (Cur_p != NULL)
    {
        Size = Cur_p->Size;
        Address = ((U32) Cur_p->StartAddr_p);
        switch (Cur_p->AllocMode)
        {
            case STAVMEM_ALLOC_MODE_BOTTOM_TOP:
                strcpy(AllocModeStr, "BottomTop");
                break;

            case STAVMEM_ALLOC_MODE_TOP_BOTTOM:
                strcpy(AllocModeStr, "TopBottom");
                break;

            case STAVMEM_ALLOC_MODE_RESERVED:
                strcpy(AllocModeStr, "RESERVED!");
                break;

            case STAVMEM_ALLOC_MODE_FORBIDDEN:
                strcpy(AllocModeStr, "FORBIDDEN");
                break;

            default:
                strcpy(AllocModeStr, "-Invalid-");
                RetErr = FALSE;
                break;
        }
        if ((RetErr) && (!Cur_p->IsUsed))
        {
            strcpy(AllocModeStr, "--FREE!--");
            CalculatedFreeSize += Size;
        }

        STTBX_Print(("%#08x-%#08x : %s (%#x byte%c) \n", Address, Address + Size - 1, AllocModeStr, Size, (Size>1)?'s':'.'));

        if (Cur_p->BlockAbove_p != Cur_p)
        {
            Cur_p = Cur_p->BlockAbove_p;
        }
        else
        {
            Cur_p = NULL;
            STTBX_Print(("Error: memory block list is CORRUPTED !!!\n"));
            RetErr = FALSE;
        }
    }

    STTBX_Print(("MEMORY BLOCKS FROM TOP:\n"));
    Cur_p = VeryTopBlock_p;
    while (Cur_p != NULL)
    {
        Size = Cur_p->Size;
        Address = ((U32) Cur_p->StartAddr_p);
        switch (Cur_p->AllocMode)
        {
            case STAVMEM_ALLOC_MODE_BOTTOM_TOP:
                strcpy(AllocModeStr, "BottomTop");
                break;

            case STAVMEM_ALLOC_MODE_TOP_BOTTOM:
                strcpy(AllocModeStr, "TopBottom");
                break;

            case STAVMEM_ALLOC_MODE_RESERVED:
                strcpy(AllocModeStr, "RESERVED!");
                break;

            case STAVMEM_ALLOC_MODE_FORBIDDEN:
                strcpy(AllocModeStr, "FORBIDDEN");
                break;

            default:
                strcpy(AllocModeStr, "-Invalid-");
                RetErr = FALSE;
                break;
        }
        if ((!RetErr) && (!Cur_p->IsUsed))
        {
            strcpy(AllocModeStr, "--FREE!--");
        }

        STTBX_Print(("%#08x-%#08x : %s (%#x byte%c) \n", Address, Address + Size - 1, AllocModeStr, Size, (Size>1)?'s':'.'));

        if (Cur_p->BlockBelow_p != Cur_p)
        {
            Cur_p = Cur_p->BlockBelow_p;
        }
        else
        {
            Cur_p = NULL;
            STTBX_Print(("Memory block list is CORRUPTED !!!\n"));
            RetErr = FALSE;
        }
    }

/*    STAVMEM_GetFreeSize(OpenHandle, &AskedFreeSize);*/

/*    if (AskedFreeSize == CalculatedFreeSize)
    {*/
        STTBX_Print(("with %#x bytes free.\n", CalculatedFreeSize));
/*    }
    else
    {
        STTBX_Print(("Error in free size: %#x / %#x bytes.\n", CalculatedFreeSize, AskedFreeSize));
        RetErr = FALSE;
    }*/


    return(RetErr);
} /* End of stavmem_ViewAllBlocksInfo() */


#endif /* #ifdef DEBUG_CODE */

#ifdef STAVMEM_DEBUG_BLOCKS_INFO

/*------------------------------------------------------------------------------
 List all AVMEM blocks with their underlying information
 Param:     none
 Return:    none
------------------------------------------------------------------------------*/
void STAVMEM_DisplayAllBlocksInfo()
{
    U8              FromVeryBottomThenTop;
    U32             NumPart;
    U32             NumBlock;
    MemBlockDesc_t  *CurrentBlock;

    for (NumPart = 0; NumPart < STAVMEM_MAX_MAX_PARTITION ; NumPart++)
    {
        STTBX_Print(("AVMem Partition %d\n",NumPart));
        if (stavmem_PartitionArray[NumPart].Validity == AVMEM_VALID_PARTITION)
        {
            for (FromVeryBottomThenTop=0; FromVeryBottomThenTop < 2; FromVeryBottomThenTop++)
            {
                NumBlock = 0;
                if (FromVeryBottomThenTop==0)
                {
                    STTBX_Print(("\nMEMORY BLOCKS FROM BOTTOM:\n"));
                    CurrentBlock = stavmem_PartitionArray[NumPart].VeryBottomBlock_p;
                }
                else
                {
                    STTBX_Print(("\nMEMORY BLOCKS FROM TOP:\n"));
                    CurrentBlock = stavmem_PartitionArray[NumPart].VeryTopBlock_p;
                }
                while(CurrentBlock != NULL)
                {
                    if(CurrentBlock->IsUsed)
                    {
                        STTBX_Print(("  Block %5d : Used. Aligned on %08x. AllocMode : ",
                                    NumBlock,
                                    CurrentBlock->Alignment));
                        switch(CurrentBlock->AllocMode)
                        {
                            case STAVMEM_ALLOC_MODE_INVALID    :
                                STTBX_Print(("INVALID"));
                                break;
                            case STAVMEM_ALLOC_MODE_RESERVED   :
                                STTBX_Print(("RESERVED"));
                                break;
                            case STAVMEM_ALLOC_MODE_FORBIDDEN  :
                                STTBX_Print(("FORBIDDEN"));
                                break;
                            case STAVMEM_ALLOC_MODE_TOP_BOTTOM :
                                STTBX_Print(("TOP_BOTTOM"));
                                break;
                            case STAVMEM_ALLOC_MODE_BOTTOM_TOP :
                                STTBX_Print(("BOTTOM_TOP"));
                                break;
                            default :
                                STTBX_Print(("Unknown"));
                                break;
                        }
                    }
                    else
                    {
                        STTBX_Print(("  Block %5d : *** FREE **************************************",NumBlock));
                    }
                    STTBX_Print(("    Total %08x->%08x, Size=%08x",
                                (U32)CurrentBlock->StartAddr_p,
                                (U32)CurrentBlock->StartAddr_p+CurrentBlock->Size-1,
                                CurrentBlock->Size));
                    STTBX_Print((" User  %08x->%08x, Size=%08x\n",
                                (U32)CurrentBlock->AlignedStartAddr_p,
                                (U32)CurrentBlock->AlignedStartAddr_p+CurrentBlock->UserSize-1,
                                CurrentBlock->UserSize));
                    if (FromVeryBottomThenTop==0)
                    {
                        CurrentBlock = CurrentBlock->BlockAbove_p;
                    }
                    else
                    {
                        CurrentBlock = CurrentBlock->BlockBelow_p;
                    }
                    NumBlock++;
                } /* while(CurrentBlock != NULL) */
            } /* for (FromVeryBottomThenTop */
        } /* if (stavmem_PartitionArray[NumPart].Validity == AVMEM_VALID_PARTITION) */
        else
        {
            STTBX_Print(("  INVALID PARTITION\n"));
        }
    }
} /* End of STAVMEM_DisplayAllBlocksInfo() */

#endif /* #ifdef STAVMEM_DEBUG_BLOCKS_INFO */


/*
--------------------------------------------------------------------------------
Get total free memory available in memory map
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STAVMEM_GetFreeSize(const ST_DeviceName_t DeviceName, U32 *TotalFreeSize_p)
{
    MemBlockDesc_t *Block_p;
    stavmem_Partition_t *Partition_p;
    U32 PartitionIndex, TotalSize = 0;
    stavmem_Device_t *Device_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;


    /* Exit if bad parameter */
    if (TotalFreeSize_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Check if device already initialised and return error if not so */
    Device_p = stavmem_GetPointerOnDeviceNamed(DeviceName);
    if (Device_p == NULL)
    {
        /* Device name not found */
        return(ST_ERROR_UNKNOWN_DEVICE);
    }

#ifdef USE_SEMAPHORE
    /* Wait for the allocations locking semaphore to be free */
    STOS_SemaphoreWait(Device_p->LockAllocSemaphore_p);
#endif

    for (PartitionIndex = 0; PartitionIndex < STAVMEM_MAX_MAX_PARTITION; PartitionIndex ++)
    {
        Partition_p = &stavmem_PartitionArray[PartitionIndex];
        if (Partition_p->Validity == AVMEM_VALID_PARTITION)
        {
            Block_p = Partition_p->VeryTopBlock_p;
            while (Block_p != NULL)
            {
                if (!Block_p->IsUsed)
                {
                    TotalSize += Block_p->Size;
                }
                Block_p = Block_p->BlockBelow_p;
            }
        }
    }

#ifdef USE_SEMAPHORE
    /* Free the allocations locking semaphore */
    STOS_SemaphoreSignal(Device_p->LockAllocSemaphore_p);
#endif

    *TotalFreeSize_p = TotalSize;

    return(ErrorCode);
} /* End of STAVMEM_GetFreeSize() */

/*
------------------------------------------------------------------------------------------
Get the memory status ; compiled if the STAVMEM_DEBUG_MEMORY_STATUS flag will be defined.
------------------------------------------------------------------------------------------
*/

#ifdef STAVMEM_DEBUG_MEMORY_STATUS
#if defined (ST_OSWINCE)   /* customized*/
ST_ErrorCode_t STAVMEM_GetMemoryStatus(const ST_DeviceName_t DeviceName)
{
    MemBlockDesc_t *Block_p;
    stavmem_Partition_t *Partition_p;
    U32 PartitionIndex;
    stavmem_Device_t *Device_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
	U32 TotalUsed, TotalFree;

    /* Check if device already initialised and return error if not so */
    Device_p = stavmem_GetPointerOnDeviceNamed(DeviceName);
    if (Device_p == NULL)
    {
        /* Device name not found */
        STTBX_Print(("device not found !\n"));
        return(ST_ERROR_UNKNOWN_DEVICE);
    }

#ifdef USE_SEMAPHORE
    /* Wait for the allocations locking semaphore to be free */
    semaphore_wait(Device_p->LockAllocSemaphore_p);
#endif

    for (PartitionIndex = 0; PartitionIndex < Device_p->MaxPartitions; PartitionIndex ++)
    {
        TotalUsed = TotalFree = 0;
		Partition_p = &stavmem_PartitionArray[PartitionIndex];
        if (Partition_p->Validity == AVMEM_VALID_PARTITION)
        {
            _WCE_Printf("Partition index : %d\n",PartitionIndex);
            Block_p = Partition_p->VeryTopBlock_p;

            if (Block_p->AllocMode == STAVMEM_ALLOC_MODE_BOTTOM_TOP)
                {
                        _WCE_Printf("ALLOCATION MODE : BOTTOM_TOP\n\n");
                }
            else
                {
                        _WCE_Printf("ALLOCATION MODE : TOP_BOTTOM\n\n");
                }
           while (Block_p != NULL)
            {
            if (!(Block_p->IsUsed))
            {
                        _WCE_Printf("FREE      0x%08x->0x%08x : 0x%08x bytes (%d.%dMB)\n",(U32)Block_p->StartAddr_p,(U32)Block_p->StartAddr_p + Block_p->Size - 1,Block_p->Size,Block_p->Size/(1024*1024), (Block_p->Size%(1024*1024))/1024 );
						TotalFree+= Block_p->Size;
            }
            else
            {
                if (Block_p->AllocMode == STAVMEM_ALLOC_MODE_RESERVED)
                {
                       /* STTBX_Print(("RESERVED  0x%08x->0x%08x \n"/*: 0x%08x bytes ALIGN 0x%08x->0x%08x : 0x%08x bytes\n \n"*/,(U32)Block_p->StartAddr_p,(U32)Block_p->StartAddr_p + Block_p->Size - 1,Block_p->Size,(U32)Block_p->AlignedStartAddr_p,(U32)Block_p->AlignedStartAddr_p + Block_p->UserSize - 1,Block_p->UserSize));*/
                }
                else if (Block_p->AllocMode == STAVMEM_ALLOC_MODE_INVALID)
                {
                        _WCE_Printf("INVALID   0x%08x->0x%08x : 0x%08x bytes ALIGN 0x%08x->0x%08x : 0x%08x bytes\n \n",(U32)Block_p->StartAddr_p,(U32)Block_p->StartAddr_p + Block_p->Size - 1,Block_p->Size,(U32)Block_p->AlignedStartAddr_p,(U32)Block_p->AlignedStartAddr_p + Block_p->UserSize - 1,Block_p->UserSize);
                }
                else if (Block_p->AllocMode == STAVMEM_ALLOC_MODE_FORBIDDEN)
                {
                        _WCE_Printf("FORBIDDEN 0x%08x->0x%08x : 0x%08x bytes ALIGN 0x%08x->0x%08x : 0x%08x bytes\n \n",(U32)Block_p->StartAddr_p,(U32)Block_p->StartAddr_p + Block_p->Size - 1,Block_p->Size,(U32)Block_p->AlignedStartAddr_p,(U32)Block_p->AlignedStartAddr_p + Block_p->UserSize - 1,Block_p->UserSize);
                }
                else/* if (Block_p->IsUsed)*/
                {
                    TotalUsed+= Block_p->Size;
					_WCE_Printf("ALLOCATED BLOCK     0x%08x->0x%08x : 0x%08x bytes  (%d.%dMB)\n"/*ALIGN 0x%08x->0x%08x : 0x%08x bytes   \n"*/,(U32)Block_p->StartAddr_p,(U32)Block_p->StartAddr_p + Block_p->Size - 1,Block_p->Size,Block_p->Size/(1024*1024), (Block_p->Size%(1024*1024))/1024);
					/*if (Block_p->UserSize != 0)
                    {
                        STTBX_Print(("USED      0x%08x->0x%08x : 0x%08x bytes ALIGN 0x%08x->0x%08x : 0x%08x bytes  (%d.%dMB)\n \n",(U32)Block_p->StartAddr_p,(U32)Block_p->StartAddr_p + Block_p->Size - 1,Block_p->Size,(U32)Block_p->AlignedStartAddr_p,(U32)Block_p->AlignedStartAddr_p + Block_p->UserSize - 1,Block_p->UserSize,Block_p->UserSize/(1024*1024), (Block_p->UserSize%(1024*1024))/1024));
                    }
                    else
                    {
                        STTBX_Print(("ENTIERLY  0x%08x->0x%08x : 0x%08x bytes ALIGN 0x%08x->0x%08x : 0x%08x bytes\n \n",(U32)Block_p->StartAddr_p,(U32)Block_p->StartAddr_p + Block_p->Size - 1,Block_p->Size,(U32)Block_p->AlignedStartAddr_p,(U32)Block_p->AlignedStartAddr_p + Block_p->UserSize - 1,Block_p->UserSize));
                    }
                    if  ((U32)Block_p->AlignedStartAddr_p != (U32)Block_p->StartAddr_p)
                    {
                        STTBX_Print(("ZONE FREE 0x%08x->0x%08x : 0x%08x bytes\n \n",(U32)Block_p->StartAddr_p,(U32)Block_p->AlignedStartAddr_p - 1,(U32)Block_p->AlignedStartAddr_p - (U32)Block_p->StartAddr_p - 1));
                    }
                    if  (((U32)Block_p->AlignedStartAddr_p + Block_p->UserSize) != ((U32)Block_p->StartAddr_p + Block_p->Size))
                    {
                        STTBX_Print(("ZONE FREE 0x%08x->0x%08x : 0x%08x bytes\n \n",(U32)Block_p->AlignedStartAddr_p + Block_p->UserSize,(U32)Block_p->StartAddr_p + Block_p->Size - 1,((U32)Block_p->StartAddr_p + Block_p->Size - 1)-((U32)Block_p->AlignedStartAddr_p + Block_p->UserSize)));
                    }*/
                }
            } /* End: block is used */
            Block_p = Block_p->BlockBelow_p;
          } /* End Of While */
        }
		_WCE_Printf("\nTOTAL USED 0x%08x (%d.%dMB) ; TOTAL FREE 0x%08x (%d.%dMB) ; PARTITION SIZE 0x%08x (%d.%dMB) \n\n",
					TotalUsed, TotalUsed/(1024*1024), (TotalUsed%(1024*1024))/1024,
					TotalFree, TotalFree/(1024*1024), (TotalFree%(1024*1024))/1024,
					TotalFree+TotalUsed, (TotalFree+TotalUsed)/(1024*1024), ((TotalFree+TotalUsed)%(1024*1024))/1024);
    }
#ifdef USE_SEMAPHORE
    /* Free the allocations locking semaphore */
    semaphore_signal(Device_p->LockAllocSemaphore_p);
#endif

    return(ErrorCode);
} /* End of STAVMEM_GetMemoryStatus() */
#else   /* ST_OSWINCE*/
ST_ErrorCode_t STAVMEM_GetMemoryStatus(const ST_DeviceName_t DeviceName, char *buf, int *len, U8 PartitionIndex )
{
    MemBlockDesc_t *Block_p;
    stavmem_Partition_t *Partition_p;
    stavmem_Device_t *Device_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    if (PartitionIndex >= STAVMEM_MAX_MAX_PARTITION)
    {
         STTBX_Print(("Partition not found !\n"));
         return(STAVMEM_ERROR_MAX_PARTITION);
    }
    /* Check if device already initialised and return error if not so */
    Device_p = stavmem_GetPointerOnDeviceNamed(DeviceName);
    if (Device_p == NULL)
    {
        /* Device name not found */
        if (buf != NULL)
        {
            *len += sprintf( buf+(*len),"device not found !\n");
        }
#ifndef ST_OSLINUX
        STTBX_Print(("device not found !\n"));
#endif
        return(ST_ERROR_UNKNOWN_DEVICE);
    }

#ifdef USE_SEMAPHORE
    /* Wait for the allocations locking semaphore to be free */
    STOS_SemaphoreWait(Device_p->LockAllocSemaphore_p);
#endif

        Partition_p = &stavmem_PartitionArray[PartitionIndex];
        if (Partition_p->Validity == AVMEM_VALID_PARTITION)
        {
            if (buf != NULL)
            {
                *len += sprintf( buf+(*len),"Partition index : %d\n",PartitionIndex);
            }
#ifndef ST_OSLINUX
            STTBX_Print(("Partition index : %d\n",PartitionIndex));
#endif

            Block_p = Partition_p->VeryTopBlock_p;

            if (Block_p->AllocMode == STAVMEM_ALLOC_MODE_BOTTOM_TOP)
                {
                        if (buf != NULL)
                        {
                            *len += sprintf( buf+(*len),"ALLOCATION MODE : BOTTOM_TOP\n\n");
                        }
#ifndef ST_OSLINUX
                        STTBX_Print(("ALLOCATION MODE : BOTTOM_TOP\n\n"));
#endif
                }
            else
                {
                        if (buf != NULL)
                        {
                            *len += sprintf( buf+(*len),"ALLOCATION MODE : TOP_BOTTOM\n\n");
                        }
#ifndef ST_OSLINUX
                        STTBX_Print(("ALLOCATION MODE : TOP_BOTTOM\n\n"));
#endif

                }
           while (Block_p != NULL)
            {
            if (!(Block_p->IsUsed))
            {
                        if (buf != NULL)
                        {
                            *len += sprintf( buf+(*len),"FREE      0x%08x->0x%08x : 0x%08x bytes\n \n",(U32)Block_p->StartAddr_p,(U32)Block_p->StartAddr_p + Block_p->Size - 1,Block_p->Size);
                        }
#ifndef ST_OSLINUX
                        STTBX_Print(("FREE      0x%08x->0x%08x : 0x%08x bytes\n \n",(U32)Block_p->StartAddr_p,(U32)Block_p->StartAddr_p + Block_p->Size - 1,Block_p->Size));
#endif

            }
            else
            {
                if (Block_p->AllocMode == STAVMEM_ALLOC_MODE_RESERVED)
                {
                        if (buf != NULL)
                        {
                            *len += sprintf( buf+(*len),"RESERVED  0x%08x->0x%08x : 0x%08x bytes ALIGN 0x%08x->0x%08x : 0x%08x bytes\n \n",(U32)Block_p->StartAddr_p,(U32)Block_p->StartAddr_p + Block_p->Size - 1,Block_p->Size,(U32)Block_p->AlignedStartAddr_p,(U32)Block_p->AlignedStartAddr_p + Block_p->UserSize - 1,Block_p->UserSize);
                        }
#ifndef ST_OSLINUX
                        STTBX_Print(("RESERVED  0x%08x->0x%08x : 0x%08x bytes ALIGN 0x%08x->0x%08x : 0x%08x bytes\n \n",(U32)Block_p->StartAddr_p,(U32)Block_p->StartAddr_p + Block_p->Size - 1,Block_p->Size,(U32)Block_p->AlignedStartAddr_p,(U32)Block_p->AlignedStartAddr_p + Block_p->UserSize - 1,Block_p->UserSize));
#endif
                }
                else if (Block_p->AllocMode == STAVMEM_ALLOC_MODE_INVALID)
                {

                        if (buf != NULL)
                        {
                            *len += sprintf( buf+(*len),"INVALID   0x%08x->0x%08x : 0x%08x bytes ALIGN 0x%08x->0x%08x : 0x%08x bytes\n \n",(U32)Block_p->StartAddr_p,(U32)Block_p->StartAddr_p + Block_p->Size - 1,Block_p->Size,(U32)Block_p->AlignedStartAddr_p,(U32)Block_p->AlignedStartAddr_p + Block_p->UserSize - 1,Block_p->UserSize);
                        }
#ifndef ST_OSLINUX
                        STTBX_Print(("INVALID   0x%08x->0x%08x : 0x%08x bytes ALIGN 0x%08x->0x%08x : 0x%08x bytes\n \n",(U32)Block_p->StartAddr_p,(U32)Block_p->StartAddr_p + Block_p->Size - 1,Block_p->Size,(U32)Block_p->AlignedStartAddr_p,(U32)Block_p->AlignedStartAddr_p + Block_p->UserSize - 1,Block_p->UserSize));
#endif
                }
                else if (Block_p->AllocMode == STAVMEM_ALLOC_MODE_FORBIDDEN)
                {
                        if (buf != NULL)
                        {
                            *len += sprintf( buf+(*len),"FORBIDDEN 0x%08x->0x%08x : 0x%08x bytes ALIGN 0x%08x->0x%08x : 0x%08x bytes\n \n",(U32)Block_p->StartAddr_p,(U32)Block_p->StartAddr_p + Block_p->Size - 1,Block_p->Size,(U32)Block_p->AlignedStartAddr_p,(U32)Block_p->AlignedStartAddr_p + Block_p->UserSize - 1,Block_p->UserSize);
                        }
#ifndef ST_OSLINUX
                        STTBX_Print(("FORBIDDEN 0x%08x->0x%08x : 0x%08x bytes ALIGN 0x%08x->0x%08x : 0x%08x bytes\n \n",(U32)Block_p->StartAddr_p,(U32)Block_p->StartAddr_p + Block_p->Size - 1,Block_p->Size,(U32)Block_p->AlignedStartAddr_p,(U32)Block_p->AlignedStartAddr_p + Block_p->UserSize - 1,Block_p->UserSize));
#endif
                }
                else/* if (Block_p->IsUsed)*/
                {
                    if (Block_p->UserSize != 0)
                    {
                        if (buf != NULL)
                        {
                            *len += sprintf( buf+(*len),"USED      0x%08x->0x%08x : 0x%08x bytes ALIGN 0x%08x->0x%08x : 0x%08x bytes\n \n",(U32)Block_p->StartAddr_p,(U32)Block_p->StartAddr_p + Block_p->Size - 1,Block_p->Size,(U32)Block_p->AlignedStartAddr_p,(U32)Block_p->AlignedStartAddr_p + Block_p->UserSize - 1,Block_p->UserSize);
                        }
#ifndef ST_OSLINUX
                        STTBX_Print(("USED      0x%08x->0x%08x : 0x%08x bytes ALIGN 0x%08x->0x%08x : 0x%08x bytes\n \n",(U32)Block_p->StartAddr_p,(U32)Block_p->StartAddr_p + Block_p->Size - 1,Block_p->Size,(U32)Block_p->AlignedStartAddr_p,(U32)Block_p->AlignedStartAddr_p + Block_p->UserSize - 1,Block_p->UserSize));
#endif
                    }
                    else
                    {
                        if (buf != NULL)
                        {
                            *len += sprintf( buf+(*len),"ENTIERLY  0x%08x->0x%08x : 0x%08x bytes ALIGN 0x%08x->0x%08x : 0x%08x bytes\n \n",(U32)Block_p->StartAddr_p,(U32)Block_p->StartAddr_p + Block_p->Size - 1,Block_p->Size,(U32)Block_p->AlignedStartAddr_p,(U32)Block_p->AlignedStartAddr_p + Block_p->UserSize - 1,Block_p->UserSize);
                        }
#ifndef ST_OSLINUX
                        STTBX_Print(("ENTIERLY  0x%08x->0x%08x : 0x%08x bytes ALIGN 0x%08x->0x%08x : 0x%08x bytes\n \n",(U32)Block_p->StartAddr_p,(U32)Block_p->StartAddr_p + Block_p->Size - 1,Block_p->Size,(U32)Block_p->AlignedStartAddr_p,(U32)Block_p->AlignedStartAddr_p + Block_p->UserSize - 1,Block_p->UserSize));
#endif

                    }
                    if  ((U32)Block_p->AlignedStartAddr_p != (U32)Block_p->StartAddr_p)
                    {
                        if (buf != NULL)
                        {
                            *len += sprintf( buf+(*len),"ZONE FREE 0x%08x->0x%08x : 0x%08x bytes\n \n",(U32)Block_p->StartAddr_p,(U32)Block_p->AlignedStartAddr_p - 1,(U32)Block_p->AlignedStartAddr_p - (U32)Block_p->StartAddr_p - 1);
                        }
#ifndef ST_OSLINUX
                        STTBX_Print(("ZONE FREE 0x%08x->0x%08x : 0x%08x bytes\n \n",(U32)Block_p->StartAddr_p,(U32)Block_p->AlignedStartAddr_p - 1,(U32)Block_p->AlignedStartAddr_p - (U32)Block_p->StartAddr_p - 1));
#endif
                    }
                    if  (((U32)Block_p->AlignedStartAddr_p + Block_p->UserSize) != ((U32)Block_p->StartAddr_p + Block_p->Size))
                    {
                        if (buf != NULL)
                        {
                            *len += sprintf( buf+(*len),"ZONE FREE 0x%08x->0x%08x : 0x%08x bytes\n \n",(U32)Block_p->AlignedStartAddr_p + Block_p->UserSize,(U32)Block_p->StartAddr_p + Block_p->Size - 1,((U32)Block_p->StartAddr_p + Block_p->Size - 1)-((U32)Block_p->AlignedStartAddr_p + Block_p->UserSize));
                        }
#ifndef ST_OSLINUX
                        STTBX_Print(("ZONE FREE 0x%08x->0x%08x : 0x%08x bytes\n \n",(U32)Block_p->AlignedStartAddr_p + Block_p->UserSize,(U32)Block_p->StartAddr_p + Block_p->Size - 1,((U32)Block_p->StartAddr_p + Block_p->Size - 1)-((U32)Block_p->AlignedStartAddr_p + Block_p->UserSize)));
#endif
                    }
                }
            } /* End: block is used */
            Block_p = Block_p->BlockBelow_p;
          } /* End Of While */
        }

#ifdef USE_SEMAPHORE
    /* Free the allocations locking semaphore */
    STOS_SemaphoreSignal(Device_p->LockAllocSemaphore_p);
#endif

    return(ErrorCode);
} /* End of STAVMEM_GetMemoryStatus() */
#endif
#endif

/*
--------------------------------------------------------------------------------
Get total free memory available in partition
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STAVMEM_GetPartitionFreeSize(STAVMEM_PartitionHandle_t PartitionHandle, U32 *PartitionFreeSize_p)
{
    MemBlockDesc_t *Block_p;
    U32 TotalSize = 0;
    stavmem_Partition_t *Partition_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Exit if bad parameter */

    if (PartitionFreeSize_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Get pointer to partition the block belongs to */
    Partition_p = (stavmem_Partition_t *) PartitionHandle;

    /* Exit if the partition is not valid */
    if (Partition_p->Validity != AVMEM_VALID_PARTITION)
    {
        return(STAVMEM_ERROR_INVALID_PARTITION_HANDLE);
    }


#ifdef USE_SEMAPHORE
    /* Wait for the allocations locking semaphore to be free */
    STOS_SemaphoreWait(Partition_p->Device_p->LockAllocSemaphore_p);
#endif

    Block_p = Partition_p->VeryTopBlock_p;
    while (Block_p != NULL)
    {
        if (!Block_p->IsUsed)
        {
            TotalSize += Block_p->Size;
        }
        Block_p = Block_p->BlockBelow_p;
    }

#ifdef USE_SEMAPHORE
    /* Free the allocations locking semaphore */
    STOS_SemaphoreSignal(Partition_p->Device_p->LockAllocSemaphore_p);
#endif

    *PartitionFreeSize_p = TotalSize;


    return(ErrorCode);
} /* End of STAVMEM_GetPartitionFreeSize() */


/*
--------------------------------------------------------------------------------
De-allocate a previously allocated memory block
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STAVMEM_FreeBlock(const STAVMEM_FreeBlockParams_t *FreeBlockParams_p, STAVMEM_BlockHandle_t *BlockHandle_p)
{
    stavmem_Partition_t *Partition_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Exit if bad parameter */
    if ((BlockHandle_p == NULL) ||
        (FreeBlockParams_p == NULL))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Get pointer to partition the block belongs to */
    Partition_p = (stavmem_Partition_t *) FreeBlockParams_p->PartitionHandle;

    /* Exit if the partition is not valid */
    if (Partition_p->Validity != AVMEM_VALID_PARTITION)
    {
        return(STAVMEM_ERROR_INVALID_PARTITION_HANDLE);
    }

    /* Exit if block handle is invalid */
    if (*BlockHandle_p == STAVMEM_INVALID_BLOCK_HANDLE)
    {
        /* Bad block handle */
        return(STAVMEM_ERROR_INVALID_BLOCK_HANDLE);
    }

#ifdef USE_SEMAPHORE
    /* Wait for the allocations locking semaphore to be free */
    STOS_SemaphoreWait(Partition_p->Device_p->LockAllocSemaphore_p);
#endif

    /* Free block */
    ErrorCode = FreeNoSemaphore(Partition_p, *BlockHandle_p);

    /* Block is now de-allocated so its handle becomes invalid */
    *BlockHandle_p = STAVMEM_INVALID_BLOCK_HANDLE;

#ifdef USE_SEMAPHORE
    /* Free the allocations locking semaphore */
    STOS_SemaphoreSignal(Partition_p->Device_p->LockAllocSemaphore_p);
#endif

    return(ErrorCode);
} /* End of STAVMEM_FreeBlock() */



/*******************************************************************************
Name        : FreeNoSemaphore
Description : De-allocate a previously allocated memory block
              Merge the unused block generated with the below and above eventual ones
Parameters  : pointer to the partition structure the block belongs to and
              memory block handle
Assumptions : pointer is not NULL, BlockHandle is valid
Limitations :
Returns     :  ST_NO_ERROR (success), STAVMEM_ERROR_INVALID_BLOCK_HANDLE (block already free)
*******************************************************************************/
static ST_ErrorCode_t FreeNoSemaphore(stavmem_Partition_t *Partition_p, STAVMEM_BlockHandle_t BlockHandle)
{
    MemBlockDesc_t *Top_p, *Block_p, *Bot_p;

    /* Get pointer to the block */
    Block_p = (MemBlockDesc_t *) BlockHandle;

    /* Exit if block is not in partition:
      Should never happend !!! */

    /* Exit is block has already been freed and caller overwrites BlockHandle_p updated by STAVMEM_FreeBlock
     * (see DDTS GNBvd09815) */
    if (!Block_p->IsUsed)
    {
        return(STAVMEM_ERROR_INVALID_BLOCK_HANDLE);
    }

    /* Mark block as unused */
    Block_p->IsUsed = FALSE;

    /* Update counter to respect MaxUsedBlocks */
    Partition_p->Device_p->CurrentlyUsedBlocks--;

    /* Consider block above and merge if it is unused */
    Top_p = Block_p->BlockAbove_p;
    if ((Top_p != NULL) && (!Top_p->IsUsed))
    {
        /* Merge the unused block above into the current unused memory block */
        Block_p->Size += Top_p->Size;

        /* Remove block from partition list of blocks */
        RemoveBlock(&(Partition_p->VeryTopBlock_p), &(Partition_p->VeryBottomBlock_p), Top_p);

        /* Free off block structure */
        FreeBlockDescriptor(&(Partition_p->TopOfFreeBlocksList_p), Top_p);
    }

    /* Consider block below and merge if it is unused */
    Bot_p = Block_p->BlockBelow_p;
    if ((Bot_p != NULL) && (!Bot_p->IsUsed))
    {
        /* Merge the current unused memory block into the unused block below */
        Bot_p->Size += Block_p->Size;

        /* Remove block from partition list of blocks */
        RemoveBlock(&(Partition_p->VeryTopBlock_p), &(Partition_p->VeryBottomBlock_p), Block_p);

        /* Free off block structure */
        FreeBlockDescriptor(&(Partition_p->TopOfFreeBlocksList_p), Block_p);
    }

    return(ST_NO_ERROR);
} /* End of FreeNoSemaphore() */



/*
--------------------------------------------------------------------------------
Allocate a memory block
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STAVMEM_AllocBlock(const STAVMEM_AllocBlockParams_t *AllocBlockParams_p, STAVMEM_BlockHandle_t *BlockHandle_p)
{
    stavmem_Partition_t *Partition_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Exit if bad parameter */
    if ((BlockHandle_p == NULL) ||
        (AllocBlockParams_p == NULL) ||
        ((AllocBlockParams_p->NumberOfForbiddenRanges > 0) && (AllocBlockParams_p->ForbiddenRangeArray_p == NULL)) ||
        ((AllocBlockParams_p->NumberOfForbiddenBorders > 0) && (AllocBlockParams_p->ForbiddenBorderArray_p == NULL)) ||
        ((AllocBlockParams_p->AllocMode != STAVMEM_ALLOC_MODE_TOP_BOTTOM) && (AllocBlockParams_p->AllocMode != STAVMEM_ALLOC_MODE_BOTTOM_TOP)))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Get pointer to partition the block belongs to */
    Partition_p = (stavmem_Partition_t *) AllocBlockParams_p->PartitionHandle;

    /* Exit if the partition is not valid */
    if (Partition_p->Validity != AVMEM_VALID_PARTITION)
    {
        return(STAVMEM_ERROR_INVALID_PARTITION_HANDLE);
    }

    /* Exit if bad parameter */
    if ((AllocBlockParams_p->NumberOfForbiddenRanges > Partition_p->Device_p->MaxForbiddenRanges) ||
        (AllocBlockParams_p->NumberOfForbiddenBorders > Partition_p->Device_p->MaxForbiddenBorders))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Check counter of user-used blocks: if MaxUsedBlocks reached, cannot allocate more */
    if (Partition_p->Device_p->CurrentlyUsedBlocks >= Partition_p->Device_p->MaxUsedBlocks)
    {
        /* Not allowed to allocate more block */
        return(STAVMEM_ERROR_MAX_BLOCKS);
    }

#ifdef USE_SEMAPHORE
    /* Wait for the allocations locking semaphore to be free */
    STOS_SemaphoreWait(Partition_p->Device_p->LockAllocSemaphore_p);
#endif

    /* Mark forbidden zones to disable allocation on these zones */
    ErrorCode = MarkForbiddenRangesAndBorders(Partition_p,
            AllocBlockParams_p->NumberOfForbiddenRanges, AllocBlockParams_p->ForbiddenRangeArray_p,
            AllocBlockParams_p->NumberOfForbiddenBorders, AllocBlockParams_p->ForbiddenBorderArray_p);

    /* Process allocation if no errors in forbidden zones marking */
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = AllocBlockNoSemaphore(AllocBlockParams_p, BlockHandle_p);
    }

    /* Unmark forbidden zones now that allocation is done */
    UnMarkForbiddenRangesAndBorders(Partition_p);

#ifdef USE_SEMAPHORE
    /* Free the allocations locking semaphore */
    STOS_SemaphoreSignal(Partition_p->Device_p->LockAllocSemaphore_p);
#endif

    return(ErrorCode);
} /* End of STAVMEM_AllocBlock() */



/*******************************************************************************
Name        : AllocBlockNoSemaphore
Description : Internal function to allocate block, not semaphore protected
              Used in STAVMEM_AllocBlock() and STAVMEM_ReAllocBlock()
Parameters  : parameters pointers not NULL
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success, STAVMEM_ERROR_PARTITION_FULL or
                                      STAVMEM_ERROR_MAX_BLOCKS otherwise
*******************************************************************************/
static ST_ErrorCode_t AllocBlockNoSemaphore(const STAVMEM_AllocBlockParams_t *AllocBlockParams_p, STAVMEM_BlockHandle_t *BlockHandle_p)
{
    MemBlockDesc_t *Cur_p, *New_p = NULL;
    void *CurrentStartAddr_p, *CurrentAlignedStartAddr_p;
    U32 TotalSize, AlignedSize;
    U32 Alignment = AllocBlockParams_p->Alignment;
    U32 AligningMask, SizeAligningMask;
    stavmem_Partition_t *Partition_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Get pointer to partition the block belongs to */
    Partition_p = (stavmem_Partition_t *) AllocBlockParams_p->PartitionHandle;

    /* Initialise variables */
    RoundUpAlignment(&Alignment);
    AligningMask = Alignment - 1;

    /* Adjust the size so the end address will be aligned
        Consequence: alloc 5 byte aligned on 8 will alloc 8 bytes ! */
   SizeAligningMask = AligningMask;

   AlignedSize = (AllocBlockParams_p->Size + SizeAligningMask) & ~SizeAligningMask;

    if (AllocBlockParams_p->AllocMode == STAVMEM_ALLOC_MODE_TOP_BOTTOM)
    {
        /* Search for an unused memory block of size greater than "size" */
        Cur_p = Partition_p->VeryTopBlock_p;
        while (Cur_p != NULL)
        {
#ifdef DO_NOT_ALLOW_AREAS_CROSSING
            /* Don't allow the areas to cross each other */
            if (Cur_p->IsUsed && (Cur_p->AllocMode != STAVMEM_ALLOC_MODE_TOP_BOTTOM) && (Cur_p->AllocMode != STAVMEM_ALLOC_MODE_RESERVED) && (Cur_p->AllocMode != STAVMEM_ALLOC_MODE_FORBIDDEN))
            {
                /* Another area is reached: end search unsucessfully */
                Cur_p = NULL;
                break;
            }
#endif

            /* Check for a large enough unused block (size 0 means full range so it is enough for sure) */
/*                if ((!Cur_p->IsUsed) && (Cur_p->Size >= AlignedSize))*/
            if ((!Cur_p->IsUsed) && ((Cur_p->Size >= AlignedSize) || (Cur_p->Size == 0)))
            {
                /* Block is large enough before alignment, ensure it is still
                    large enough with alignment constraints */
                CurrentStartAddr_p = (void *) (((U32) Cur_p->StartAddr_p) + Cur_p->Size - AlignedSize);
                CurrentAlignedStartAddr_p = (void *) (((U32) CurrentStartAddr_p) & ~AligningMask);
                TotalSize = ((U32) CurrentStartAddr_p) - ((U32) CurrentAlignedStartAddr_p) + AlignedSize;

                /* If the unused memory block is large enough, exit search */
/*                    if (Cur_p->Size >= TotalSize)*/
                if ((Cur_p->Size >= TotalSize) || (Cur_p->Size == 0))
                {
#ifdef DEBUG_CODE
                    if (TotalSize < AllocBlockParams_p->Size)
                    {
                        /* Error case */
                        stavmem_BlockAndBip();
                    }
#endif
                    /* Found a unused block matching the requirements ! */
                    break;
                }
            }

            /* Unsuccessful search: check next block in the partition list */
            Cur_p = Cur_p->BlockBelow_p;
        }

        /* Search is finished: allocate in unused memory block if found */
        if (Cur_p != NULL)
        {
            /* Allocate block structure */
            New_p = AllocBlockDescriptor(&(Partition_p->TopOfFreeBlocksList_p));

            if (New_p != NULL)
            {
                /* Block structure allocation successful
                    Insert new block above current block in partition blocks list */
                InsertBlockAboveBlock(&(Partition_p->VeryTopBlock_p), New_p, Cur_p);

                /* Setup new block  */
                New_p->AllocMode   = AllocBlockParams_p->AllocMode; /* STAVMEM_ALLOC_MODE_BOTTOM_TOP */
                New_p->StartAddr_p = CurrentAlignedStartAddr_p;
                New_p->Size        = TotalSize;
                New_p->Alignment   = Alignment;
                New_p->AlignedStartAddr_p = CurrentAlignedStartAddr_p;
                New_p->UserSize    = AllocBlockParams_p->Size;
                New_p->IsUsed      = TRUE;
                New_p->PartitionHandle = (STAVMEM_PartitionHandle_t) Partition_p;
#ifdef DEBUG_CODE
    if ((((U32) New_p->AlignedStartAddr_p) - ((U32) New_p->StartAddr_p) + New_p->UserSize) > New_p->Size)
    {
        /* ErrorCase */
        stavmem_BlockAndBip();
    }
#endif

                /* Update counter to respect MaxUsedBlocks */
                Partition_p->Device_p->CurrentlyUsedBlocks++;

                /* Reduce size of unused block and remove it from partition if it is of size 0 */
                Cur_p->Size -= TotalSize;
                if (Cur_p->Size == 0)
                {
                    /* Remove block from partition list of blocks */
                    RemoveBlock(&(Partition_p->VeryTopBlock_p), &(Partition_p->VeryBottomBlock_p), Cur_p);

                    /* Free off block structure */
                    FreeBlockDescriptor(&(Partition_p->TopOfFreeBlocksList_p), Cur_p);
                }
            }
            else
            {
                /* Failed to allocate new block structure */
                ErrorCode = STAVMEM_ERROR_MAX_BLOCKS;
            }
        }
        else
        {
            /* Could not find a large enough memory block matching the allocation requirements */
            ErrorCode = STAVMEM_ERROR_PARTITION_FULL;
        }
    }
    else /*if (AllocBlockParams_p->AllocMode == STAVMEM_ALLOC_MODE_BOTTOM_TOP)*/
    {
        /* Search for an unused memory block of size greater than "size" */
        Cur_p = Partition_p->VeryBottomBlock_p;
        while (Cur_p != NULL)
        {
#ifdef DO_NOT_ALLOW_AREAS_CROSSING
            /* Don't allow the "Heap" to overflow the "Stack" */
            if (Cur_p->IsUsed && Cur_p->AllocMode != STAVMEM_ALLOC_MODE_BOTTOM_TOP && (Cur_p->AllocMode != STAVMEM_ALLOC_MODE_RESERVED)&& (Cur_p->AllocMode != STAVMEM_ALLOC_MODE_FORBIDDEN))
            {
                /* Another area is reached: end search unsucessfully */
                Cur_p = NULL;
                break;
            }
#endif

            /* Check for a large enough unused block (size 0 means full range so it is enough for sure) */
/*                if ((!Cur_p->IsUsed) && (Cur_p->Size >= AlignedSize))*/
            if ((!Cur_p->IsUsed) && ((Cur_p->Size >= AlignedSize) || (Cur_p->Size == 0)))
            {
                /* Block is large enough before alignment, ensure it is still
                    large enough with alignment constraints */
                CurrentAlignedStartAddr_p = (void *) ((((U32) Cur_p->StartAddr_p) + AligningMask) & ~AligningMask);
                TotalSize = ((U32) CurrentAlignedStartAddr_p) - ((U32) Cur_p->StartAddr_p) + AlignedSize;

                /* If the unused memory block is large enough, exit search */
/*                    if (Cur_p->Size >= TotalSize)*/
                if ((Cur_p->Size >= TotalSize) || (Cur_p->Size == 0))
                {
#ifdef DEBUG_CODE
                    if (TotalSize < AllocBlockParams_p->Size)
                    {
                        /* Error case */
                        stavmem_BlockAndBip();
                    }
#endif
                    /* Found a unused block matching the requirements ! */
                    break;
                }
            }

            /* Unsuccessful search: check next block in the partition list */
            Cur_p = Cur_p->BlockAbove_p;
        }

        /* Search is finished: allocate in unused memory block if found */
        if (Cur_p != NULL)
        {
            /* Allocate block structure */
            New_p = AllocBlockDescriptor(&(Partition_p->TopOfFreeBlocksList_p));

            if (New_p != NULL)
            {
                /* Block structure allocation successful
                    Insert new block below current block in partition blocks list */
                InsertBlockBelowBlock(&(Partition_p->VeryBottomBlock_p), New_p, Cur_p);

                /* Setup new block */
                New_p->AllocMode   = AllocBlockParams_p->AllocMode; /* STAVMEM_ALLOC_MODE_TOP_BOTTOM */
                New_p->StartAddr_p = Cur_p->StartAddr_p;
                New_p->Size        = TotalSize;
                New_p->Alignment   = Alignment;
                New_p->AlignedStartAddr_p = CurrentAlignedStartAddr_p;
                New_p->UserSize    = AllocBlockParams_p->Size;
                New_p->IsUsed      = TRUE;
                New_p->PartitionHandle = (STAVMEM_PartitionHandle_t) Partition_p;
#ifdef DEBUG_CODE
    if ((((U32) New_p->AlignedStartAddr_p) - ((U32) New_p->StartAddr_p) + New_p->UserSize) > New_p->Size)
    {
        /* ErrorCase */
        stavmem_BlockAndBip();
    }
#endif

                /* Update counter to respect MaxUsedBlocks */
                Partition_p->Device_p->CurrentlyUsedBlocks++;

                /* Update unused block start address and size, and remove it from partition if it is of size 0 */
                Cur_p->StartAddr_p = (void *) (((U32) Cur_p->StartAddr_p) + TotalSize);
                Cur_p->Size -= TotalSize;
                if (Cur_p->Size == 0)
                {
                    /* Remove block from partition list of blocks */
                    RemoveBlock(&(Partition_p->VeryTopBlock_p), &(Partition_p->VeryBottomBlock_p), Cur_p);

                    /* Free off block structure */
                    FreeBlockDescriptor(&(Partition_p->TopOfFreeBlocksList_p), Cur_p);
                }
            }
            else
            {
                /* Failed to allocate new block structure */
                ErrorCode = STAVMEM_ERROR_MAX_BLOCKS;
            }
        }
        else
        {
            /* Could not find a large enough memory block matching the allocation requirements */
            ErrorCode = STAVMEM_ERROR_PARTITION_FULL;
        }
    }

    /* Unmark forbidden zones now that allocation is done */
    UnMarkForbiddenRangesAndBorders(Partition_p);

    /* Output block handle as allocation result */
    if (New_p == NULL)
    {
        *BlockHandle_p = STAVMEM_INVALID_BLOCK_HANDLE;
    }
    else
    {
        *BlockHandle_p = (STAVMEM_BlockHandle_t) New_p;
    }

    return(ErrorCode);
} /* End of AllocBlockNoSemaphore() */



/*
--------------------------------------------------------------------------------
Re-allocate a memory block (with different size)
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STAVMEM_ReAllocBlock(const STAVMEM_ReAllocBlockParams_t *ReAllocBlockParams_p, STAVMEM_BlockHandle_t *BlockHandle_p)
{
    MemBlockDesc_t *Block_p, *New_p = NULL, *Bot_p = NULL, *Top_p = NULL;
    U32 BlockAlignedSize, ReAllocAlignedSize, AlignedSizeDifference, AddrAlignedSizeDifference;
    U32 AligningMask, SizeAligningMask;
    U32 i;
#ifdef DEBUG_CODE
    U32 ChangedSize;
    void *ChangedStart;
    void *BlockAddrBefore;
    U32 BlockSizeBefore, BlockUserSizeBefore, BotSizeBefore;
    U8 PreserveProbe;
#endif
    BOOL EnlargeBelowForbidden = FALSE;
    void *PreserveAddr_p;
    U32 PreserveSize;
    STAVMEM_AllocBlockParams_t AllocBlockParams;
    stavmem_Partition_t *Partition_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Exit if bad parameter */
    if ((BlockHandle_p == NULL) ||
        (ReAllocBlockParams_p == NULL) ||
        ((ReAllocBlockParams_p->NumberOfForbiddenRanges > 0) && (ReAllocBlockParams_p->ForbiddenRangeArray_p == NULL)) ||
        ((ReAllocBlockParams_p->NumberOfForbiddenBorders > 0) && (ReAllocBlockParams_p->ForbiddenBorderArray_p == NULL)))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Get pointer to partition the block belongs to */
    Partition_p = (stavmem_Partition_t *) ReAllocBlockParams_p->PartitionHandle;

    /* Exit if the partition is not valid */
    if (Partition_p->Validity != AVMEM_VALID_PARTITION)
    {
        return(STAVMEM_ERROR_INVALID_PARTITION_HANDLE);
    }

    /* Exit if bad parameter */
    if ((ReAllocBlockParams_p->NumberOfForbiddenRanges > Partition_p->Device_p->MaxForbiddenRanges) ||
        (ReAllocBlockParams_p->NumberOfForbiddenBorders > Partition_p->Device_p->MaxForbiddenBorders))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Exit if block handle is invalid */
    if ((*BlockHandle_p == STAVMEM_INVALID_BLOCK_HANDLE) || (!BlockHandleBelongsToSpace(*BlockHandle_p, *(Partition_p->Device_p))))
    {
        /* Bad block handle */
        return(STAVMEM_ERROR_INVALID_BLOCK_HANDLE);
    }

    /* Setting pointer to block descriptor */
    Block_p = (MemBlockDesc_t *) *BlockHandle_p;
#ifdef DEBUG_CODE
            if ((((U32) Block_p->AlignedStartAddr_p) - ((U32) Block_p->StartAddr_p) + Block_p->UserSize) > Block_p->Size)
            {
                /* ErrorCase */
                stavmem_BlockAndBip();
            }
#endif
    /* Exit if block is crossing any specified forbidden zone */
    if (IsInForbiddenRangeOrBorder(Block_p->StartAddr_p, (void *) (((U32) Block_p->StartAddr_p) + Block_p->Size - 1),
           ReAllocBlockParams_p->NumberOfForbiddenRanges, ReAllocBlockParams_p->ForbiddenRangeArray_p,
           ReAllocBlockParams_p->NumberOfForbiddenBorders, ReAllocBlockParams_p->ForbiddenBorderArray_p))
    {
        return(STAVMEM_ERROR_BLOCK_IN_FORBIDDEN_ZONE);
    }

    /* Exit if block is not in partition */
    if ((U32) (Block_p->PartitionHandle) != (U32) (Partition_p))
    {
        return(STAVMEM_ERROR_INVALID_PARTITION_HANDLE);
    }

    /* If first address of the block is a forbidden border, cannot cross it */
    i = 0;
    while ((!EnlargeBelowForbidden) && (i < ReAllocBlockParams_p->NumberOfForbiddenBorders))
    {
        /* Test if any forbidden border is the first address of the block. */
        if (((U32) ReAllocBlockParams_p->ForbiddenBorderArray_p[i]) == ((U32) Block_p->StartAddr_p))
        {
            EnlargeBelowForbidden = TRUE;
        }
        i++;
    }

#ifdef USE_SEMAPHORE
    /* Wait for the allocations locking semaphore to be free */
    STOS_SemaphoreWait(Partition_p->Device_p->LockAllocSemaphore_p);
#endif

    /* Mark forbidden zones to disable allocation on these zones */
    ErrorCode = MarkForbiddenRangesAndBorders(Partition_p,
            ReAllocBlockParams_p->NumberOfForbiddenRanges, ReAllocBlockParams_p->ForbiddenRangeArray_p,
            ReAllocBlockParams_p->NumberOfForbiddenBorders, ReAllocBlockParams_p->ForbiddenBorderArray_p);

    /* Process allocation if no errors in forbidden zones marking */
    if (ErrorCode == ST_NO_ERROR)
    {
        /* Initialise variables */
        BlockAlignedSize = Block_p->Size - (((U32) Block_p->AlignedStartAddr_p) - ((U32) Block_p->StartAddr_p));
        PreserveAddr_p = Block_p->AlignedStartAddr_p;
        PreserveSize = Block_p->UserSize;
#ifdef DEBUG_CODE
        PreserveProbe = *((U8 *) PreserveAddr_p);
#endif
        AligningMask = Block_p->Alignment - 1;

        /* Adjust the size so the end address will be aligned */
        /* Consequence: alloc 5 byte aligned on 8 will alloc 8 bytes ! */

        SizeAligningMask = AligningMask;

        ReAllocAlignedSize = (ReAllocBlockParams_p->Size + SizeAligningMask) & ~SizeAligningMask;

        if (ReAllocAlignedSize < Block_p->UserSize)
        {
            /* The requested size is smaller and allows to reduce current block size */
            AlignedSizeDifference = BlockAlignedSize - ReAllocAlignedSize;
            AddrAlignedSizeDifference = (AlignedSizeDifference + AligningMask) & ~AligningMask;

            if ((Block_p->BlockAbove_p != NULL) && (!Block_p->BlockAbove_p->IsUsed))
            {
                /* The block above is unused, so give it some more memory */
                Block_p->Size     -= AlignedSizeDifference;
                Block_p->UserSize = ReAllocBlockParams_p->Size;
                Block_p->BlockAbove_p->Size += AlignedSizeDifference;
                Block_p->BlockAbove_p->StartAddr_p = (void *) (((U32) Block_p->BlockAbove_p->StartAddr_p) - AlignedSizeDifference);
#ifdef DEBUG_CODE
STTBX_Print(("****** Re-allocate with reduction on top.\n"));
                if ((((U32) Block_p->AlignedStartAddr_p) - ((U32) Block_p->StartAddr_p) + Block_p->UserSize) > Block_p->Size)
                {
                    /* ErrorCase */
                    stavmem_BlockAndBip();
                }
#endif

                /* Start address is unchanged: data is preserved, no need to copy it */
            }
            else if ((Block_p->BlockBelow_p != NULL) && (!Block_p->BlockBelow_p->IsUsed) &&
                     ((((U32) Block_p->AlignedStartAddr_p) + AddrAlignedSizeDifference + ReAllocBlockParams_p->Size) <= (((U32) Block_p->StartAddr_p) + Block_p->Size)))
            {
                /* The block below is unused, so give it some more memory */
                Block_p->Size     -= AddrAlignedSizeDifference;
                Block_p->UserSize = ReAllocBlockParams_p->Size;
                Block_p->StartAddr_p        = (void *) (((U32) Block_p->StartAddr_p) + AddrAlignedSizeDifference);
                Block_p->AlignedStartAddr_p = (void *) (((U32) Block_p->AlignedStartAddr_p) + AddrAlignedSizeDifference);
#ifdef DEBUG_CODE
STTBX_Print(("****** Re-allocate with reduction on bottom.\n"));
                if ((((U32) Block_p->AlignedStartAddr_p) - ((U32) Block_p->StartAddr_p) + Block_p->UserSize) > Block_p->Size)
                {
                    /* ErrorCase */
                    stavmem_BlockAndBip();
                }
#endif

                Block_p->BlockBelow_p->Size += AddrAlignedSizeDifference;

                /* Now copy data if required */
                if (ReAllocBlockParams_p->PreserveData)
                {
#ifdef ST_OSLINUX
                    STAVMEM_CopyBlock1D(PreserveAddr_p, Block_p->AlignedStartAddr_p, Block_p->UserSize);
#else
                    (*(ReAllocBlockParams_p->PreserveDataFunction)) (PreserveAddr_p, Block_p->AlignedStartAddr_p, Block_p->UserSize);
#endif
#ifdef DEBUG_CODE
                    if ((*(U8 *) Block_p->AlignedStartAddr_p) != PreserveProbe)
                    {
                        /* ErrorCase */
STTBX_Print(("****** PreserveData failed.\n"));
                        stavmem_BlockAndBip();
                    }
#endif
                }
            }
            else
            {
                /* The blocks below and above are used or is the end of the list,
                   so create a new unused block for the memory left by the re-allocated block.
                   Allocate block structure */
                New_p = AllocBlockDescriptor(&(Partition_p->TopOfFreeBlocksList_p));

                if (New_p == NULL)
                {
                    /* Failed to allocate new block structure */
                    ErrorCode = STAVMEM_ERROR_MAX_BLOCKS;
                }
                else
                {
                    /* Block structure allocation successful */
                    /* Reduce current block size */
                    Block_p->Size     -= AlignedSizeDifference;
                    Block_p->UserSize = ReAllocBlockParams_p->Size;

                    /* Setup new block */
                    New_p->StartAddr_p = (void *) (((U32) Block_p->StartAddr_p) + Block_p->Size);
                    New_p->Size        = AlignedSizeDifference;
                    New_p->IsUsed      = FALSE;

                    /* Insert new unused block above block in partition blocks list */
                    InsertBlockAboveBlock(&(Partition_p->VeryTopBlock_p), New_p, Block_p);

                    /* Start address is unchanged: data is preserved, no need to copy it */
                }
#ifdef DEBUG_CODE
STTBX_Print(("****** Re-allocate with reduction on top and free block creation.\n"));
                if ((((U32) Block_p->AlignedStartAddr_p) - ((U32) Block_p->StartAddr_p) + Block_p->UserSize) > Block_p->Size)
                {
                    /* ErrorCase */
                    stavmem_BlockAndBip();
                }
#endif
            }
        }
        else if (ReAllocAlignedSize > BlockAlignedSize)
        {
            /* The requested size is larger and requires enlargement of block or re-allocattion */
            AlignedSizeDifference = ReAllocAlignedSize - BlockAlignedSize;
            AddrAlignedSizeDifference = (AlignedSizeDifference + AligningMask) & ~AligningMask;

            /* Consider blocks below and above */
            Bot_p = Block_p->BlockBelow_p;
            Top_p = Block_p->BlockAbove_p;

            if ((Top_p != NULL) && (!Top_p->IsUsed) && (Top_p->Size >= AlignedSizeDifference))
            {
                /* Block above is unused and large enough: use part of it to enlarge block */
                Block_p->Size     += AlignedSizeDifference;
                Block_p->UserSize = ReAllocBlockParams_p->Size;
#ifdef DEBUG_CODE
STTBX_Print(("****** Re-allocate with enlargment on top.\n"));
                if ((((U32) Block_p->AlignedStartAddr_p) - ((U32) Block_p->StartAddr_p) + Block_p->UserSize) > Block_p->Size)
                {
                    /* ErrorCase */
                    stavmem_BlockAndBip();
                }
#endif

                Top_p->StartAddr_p = (void *) (((U32) Top_p->StartAddr_p) + AlignedSizeDifference);
                Top_p->Size        -= AlignedSizeDifference;
                if (Top_p->Size == 0)
                {
                    /* The top unused block is now of zero size so remove it from partition list of blocks */
                    RemoveBlock(&(Partition_p->VeryTopBlock_p), &(Partition_p->VeryBottomBlock_p), Top_p);

                    /* Free off block structure */
                    FreeBlockDescriptor(&(Partition_p->TopOfFreeBlocksList_p), Top_p);
                }

                /* Start address is unchanged: data is preserved, no need to copy it */
            }
            else if ((!EnlargeBelowForbidden) && (Bot_p != NULL) && (!Bot_p->IsUsed) && /* can enlarge below and block below is free and */
                     ((((U32) Block_p->AlignedStartAddr_p) - AddrAlignedSizeDifference) >= ((U32) Bot_p->StartAddr_p)))
            {
                /* Block below is unused: use part of it. */
#ifdef DEBUG_CODE
                ChangedSize = (AddrAlignedSizeDifference - (((U32) Block_p->AlignedStartAddr_p) - ((U32) Block_p->StartAddr_p)));
                ChangedStart = Block_p->StartAddr_p;
                BlockAddrBefore = Block_p->AlignedStartAddr_p;
                BlockSizeBefore = Block_p->Size;
                BlockUserSizeBefore = Block_p->UserSize;
                BotSizeBefore = Bot_p->Size;
#endif
                Bot_p->Size       -= (AddrAlignedSizeDifference - (((U32) Block_p->AlignedStartAddr_p) - ((U32) Block_p->StartAddr_p)));
                Block_p->Size     += (AddrAlignedSizeDifference - (((U32) Block_p->AlignedStartAddr_p) - ((U32) Block_p->StartAddr_p)));
                Block_p->UserSize = ReAllocBlockParams_p->Size;
                Block_p->StartAddr_p        = (void *) (((U32) Bot_p->StartAddr_p) + Bot_p->Size);
                Block_p->AlignedStartAddr_p = (void *) (((U32) Block_p->AlignedStartAddr_p) - AddrAlignedSizeDifference);
#ifdef DEBUG_CODE
STTBX_Print(("****** Re-allocate with enlargment on bottom.\n"));
                if ((((U32) Block_p->AlignedStartAddr_p) - ((U32) Block_p->StartAddr_p) + Block_p->UserSize) > Block_p->Size)
                {
                    /* ErrorCase */
                    stavmem_BlockAndBip();
                }
#endif

                if (Bot_p->Size == 0)
                {
                    /* The below unused block is now of zero size so remove it from partition list of blocks */
                    RemoveBlock(&(Partition_p->VeryTopBlock_p), &(Partition_p->VeryBottomBlock_p), Bot_p);

                    /* Free off block structure */
                    FreeBlockDescriptor(&(Partition_p->TopOfFreeBlocksList_p), Bot_p);
                }

                /* Now copy data if required */
                if (ReAllocBlockParams_p->PreserveData)
                {
#ifdef ST_OSLINUX
                    STAVMEM_CopyBlock1D(PreserveAddr_p, Block_p->AlignedStartAddr_p, PreserveSize);
#else
                    (*(ReAllocBlockParams_p->PreserveDataFunction)) (PreserveAddr_p, Block_p->AlignedStartAddr_p, PreserveSize);
#endif
#ifdef DEBUG_CODE
                    if ((*(U8 *) Block_p->AlignedStartAddr_p) != PreserveProbe)
                    {
                        /* ErrorCase */
STTBX_Print(("****** PreserveData failed.\n"));
                        stavmem_BlockAndBip();
                    }
#endif
                }
            }
            else
            {
#ifdef DEBUG_CODE
STTBX_Print(("****** Re-allocate with enlargment by Allocate and Free.\n"));
#endif
                /* Need more space but blocks above and below are not large
                   enough: re-allocate block somewhere else and free the current place */
                AllocBlockParams.Size      = ReAllocBlockParams_p->Size;
                AllocBlockParams.Alignment = Block_p->Alignment;
                AllocBlockParams.AllocMode  = Block_p->AllocMode;
                AllocBlockParams.PartitionHandle  = Block_p->PartitionHandle;
                AllocBlockParams.NumberOfForbiddenRanges = ReAllocBlockParams_p->NumberOfForbiddenRanges;
                AllocBlockParams.ForbiddenRangeArray_p = ReAllocBlockParams_p->ForbiddenRangeArray_p;
                AllocBlockParams.NumberOfForbiddenBorders = ReAllocBlockParams_p->NumberOfForbiddenBorders;
                AllocBlockParams.ForbiddenBorderArray_p = ReAllocBlockParams_p->ForbiddenBorderArray_p;

                ErrorCode = AllocBlockNoSemaphore(&AllocBlockParams, (STAVMEM_BlockHandle_t *) &New_p);

                if (ErrorCode == ST_NO_ERROR)
                {
                    /* Free old block */
                    ErrorCode = FreeNoSemaphore(Partition_p, (STAVMEM_BlockHandle_t) Block_p);
                    /* Always success */

                    /* Block allocated: copy data if required */
                    if (ReAllocBlockParams_p->PreserveData)
                    {
#ifdef ST_OSLINUX
                        STAVMEM_CopyBlock1D(PreserveAddr_p, New_p->AlignedStartAddr_p, PreserveSize);
#else
                        (*(ReAllocBlockParams_p->PreserveDataFunction)) (PreserveAddr_p, New_p->AlignedStartAddr_p, PreserveSize);
#endif
#ifdef DEBUG_CODE
                    if ((*(U8 *) New_p->AlignedStartAddr_p) != PreserveProbe)
                    {
                        /* ErrorCase */
STTBX_Print(("****** PreserveData failed.\n"));
                        stavmem_BlockAndBip();
                    }
#endif
                    }

                    /* Re-allocated block is now the new one */
                    Block_p = New_p;
#ifdef DEBUG_CODE
                    if ((((U32) Block_p->AlignedStartAddr_p) - ((U32) Block_p->StartAddr_p) + Block_p->UserSize) > Block_p->Size)
                    {
                        /* ErrorCase */
                        stavmem_BlockAndBip();
                    }
#endif
                }
            }
        }
        else
        {
            /*  The requested size fits in the allocated block because of alignment of the size when allocation:
                It doesn't allow to reduce the size of the block, neither does it require enlargment.
                Block doesn't need to change... */
            Block_p->UserSize = ReAllocBlockParams_p->Size;
#ifdef DEBUG_CODE
STTBX_Print(("****** Re-allocate with just size changing.\n"));
            if ((((U32) Block_p->AlignedStartAddr_p) - ((U32) Block_p->StartAddr_p) + Block_p->UserSize) > Block_p->Size)
            {
                /* ErrorCase */
                stavmem_BlockAndBip();
            }
#endif
        }
    }

    /* Unmark forbidden zones now that allocation is done */
    UnMarkForbiddenRangesAndBorders(Partition_p);

#ifdef USE_SEMAPHORE
    /* Free the allocations locking semaphore */
    STOS_SemaphoreSignal(Partition_p->Device_p->LockAllocSemaphore_p);
#endif

    *BlockHandle_p = (STAVMEM_BlockHandle_t) Block_p;

    return(ErrorCode);
} /* End of STAVMEM_ReAllocBlock() */



/*
------------------------------------------------------------------------------
Get the parameters of a memory block.
------------------------------------------------------------------------------
*/
ST_ErrorCode_t STAVMEM_GetBlockParams(STAVMEM_BlockHandle_t BlockHandle, STAVMEM_BlockParams_t *BlockParams_p)
{
    MemBlockDesc_t *Block_p;
    stavmem_Partition_t *Partition_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Exit if bad parameter */
    if (BlockParams_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Get pointer to the block */
    Block_p = (MemBlockDesc_t *) BlockHandle;

    /* Exit if block handle is invalid of if block alloc mode is impossible */
    if ((BlockHandle == STAVMEM_INVALID_BLOCK_HANDLE) ||    /* Handle invalid */
        ((Block_p->AllocMode != STAVMEM_ALLOC_MODE_TOP_BOTTOM) && ((Block_p->AllocMode != STAVMEM_ALLOC_MODE_BOTTOM_TOP))) || /* bad data */
        (!Block_p->IsUsed))                             /* Block is unused (may have been freed) */
    {
        /* Bad block handle or impossible alloc mode */
        return(STAVMEM_ERROR_INVALID_BLOCK_HANDLE);
    }

    /* Get pointer to partition the block belongs to */
    Partition_p = (stavmem_Partition_t *) Block_p->PartitionHandle;

    /* Exit if partition invalid: block should have been freed !
       This should not happen, but it is here for security */
    if (Partition_p->Validity != AVMEM_VALID_PARTITION)
    {
        return(STAVMEM_ERROR_INVALID_BLOCK_HANDLE);
    }

    /* Output required parameters of the block */
    BlockParams_p->Size = Block_p->UserSize;
    BlockParams_p->StartAddr_p = Block_p->AlignedStartAddr_p;
    BlockParams_p->AllocMode = Block_p->AllocMode;
    BlockParams_p->Alignment = Block_p->Alignment;

    return(ErrorCode);
} /* End of STAVMEM_GetBlockParams() */



/*
--------------------------------------------------------------------------------
Get pointer to memory block start address
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STAVMEM_GetBlockAddress(STAVMEM_BlockHandle_t BlockHandle, void **Address_p)
{
    MemBlockDesc_t *Block_p;
    stavmem_Partition_t *Partition_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Exit if bad parameter */
    if (Address_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Get pointer to the block */
    Block_p = (MemBlockDesc_t *) BlockHandle;

    /* Exit if block handle is invalid of if block alloc mode is impossible */
    if ((BlockHandle == STAVMEM_INVALID_BLOCK_HANDLE) ||    /* Handle invalid */
        ((Block_p->AllocMode != STAVMEM_ALLOC_MODE_TOP_BOTTOM) && ((Block_p->AllocMode != STAVMEM_ALLOC_MODE_BOTTOM_TOP))) || /* bad data */
        (!Block_p->IsUsed))                             /* Block is unused (may have been freed) */
    {
        /* Bad block handle or impossible alloc mode */
        return(STAVMEM_ERROR_INVALID_BLOCK_HANDLE);
    }

    /* Get pointer to partition the block belongs to */
    Partition_p = (stavmem_Partition_t *) Block_p->PartitionHandle;

    /* Exit if partition invalid: block should have been freed !
       This should not happen, but it is here for security */
    if (Partition_p->Validity != AVMEM_VALID_PARTITION)
    {
        return(STAVMEM_ERROR_INVALID_BLOCK_HANDLE);
    }

    /* Output useful address of block: aligned start address */
    *Address_p = Block_p->AlignedStartAddr_p;
    return(ErrorCode);
} /* End of STAVMEM_GetBlockAddress() */


#if 0
/*******************************************************************************
Name        : stavmem_BlockHandleBelongsToPartition
Description : Check if the block handle belong to the linked list of blocks of
              the partition of the opened handle
Parameters  : Opened handle, Block handle
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if block is valid
              ST_ERROR_INVALID_HANDLE if opened handle is invalid
              STAVMEM_ERROR_INVALID_BLOCK_HANDLE if block is not valid
*******************************************************************************/
ST_ErrorCode_t stavmem_BlockHandleBelongsToPartition(STAVMEM_BlockHandle_t BlockHandle, STAVMEM_PartitionHandle_t PartitionHandle)
{
    MemBlockDesc_t *Cur_p, *Block_p;
    stavmem_Partition_t *Partition_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Get pointer to partition the block belongs to */
    Partition_p = (stavmem_Partition_t *) PartitionHandle;

    /* Exit if the partition is not valid */
    if (Partition_p->Validity != AVMEM_VALID_PARTITION)
    {
        return(STAVMEM_ERROR_INVALID_PARTITION_HANDLE);
    }

    /* Exit if block handle is invalid */
    if ((BlockHandle == STAVMEM_INVALID_BLOCK_HANDLE) || (!BlockHandleBelongsToSpace(BlockHandle, *(Partition_p->Device_p))))
    {
        /* Bad block handle */
        return(STAVMEM_ERROR_INVALID_BLOCK_HANDLE);
    }

#ifdef USE_SEMAPHORE
    /* Wait for the allocations locking semaphore to be free */
    STOS_SemaphoreWait(Partition_p->Device_p->LockAllocSemaphore_p);
#endif

    /* Get pointer to the block */
    Block_p = (MemBlockDesc_t *) BlockHandle;

    Cur_p = Partition_p->VeryTopBlock_p;
    while ((Cur_p != NULL) && (Cur_p != Block_p))
    {
        Cur_p = Cur_p->BlockBelow_p;
    }

#ifdef USE_SEMAPHORE
    /* Free the allocations locking semaphore */
    STOS_SemaphoreSignal(Partition_p->Device_p->LockAllocSemaphore_p);
#endif

    if (Cur_p == NULL)
    {
        ErrorCode = STAVMEM_ERROR_INVALID_BLOCK_HANDLE;
    }

    return(ErrorCode);
} /* End of stavmem_BlockHandleBelongsToPartition() */
#endif /* #if 0 */




/* Private functions -------------------------------------------------------- */

/*******************************************************************************
Name        : UnReserveBlockNoSemaphore
Description : Un-reserve block among list of reserved/unused blocks
              Used at initialisation only
Parameters  : pointer on partition and pointer to sub-space
Assumptions : Pointers are not NULL.
              Used blocks are reserved ones only.
Limitations :
Returns     : ST_NO_ERROR if success,
              STAVMEM_ERROR_MAX_BLOCKS if lack of blocks (should never happen if alloc in init is well done)
*******************************************************************************/
ST_ErrorCode_t UnReserveBlockNoSemaphore(stavmem_Partition_t *Partition_p, STAVMEM_MemoryRange_t *SubSpace_p)
{
    MemBlockDesc_t *Block_p = NULL, *UnReserved_p = NULL, *Above_p = NULL;
    stavmem_Device_t *Device_p;
    U32 SizeDifference = 0;
    void *BlockStart_p = 0, *BlockStop_p = 0;

    Device_p = Partition_p->Device_p;

    /* Parse all reserved blocks to unreserve the one overlapping the given sub-space */
    Block_p = Partition_p->VeryBottomBlock_p;

    while (Block_p != NULL)
    {
        if (Block_p->IsUsed)
        {
            /* This block is reserved: check if it overlaps sub-space to unreserve */
            BlockStart_p = Block_p->StartAddr_p;
            BlockStop_p  = (void *) (((U32) BlockStart_p) + Block_p->Size - 1);

            /* There's overlap if reserved block starts before end of the sub-space
               and stops after the begining of sub-space */
            if ((((U32) BlockStart_p) <= ((U32) SubSpace_p->StopAddr_p)) && (((U32) BlockStop_p) >= ((U32) SubSpace_p->StartAddr_p)))
            {
                /* reserved block overlaps part of the sub-space to unreserve */
                Block_p->PartitionHandle = (STAVMEM_PartitionHandle_t) Partition_p;
                if (((U32) BlockStart_p) < ((U32) SubSpace_p->StartAddr_p))
                {
                    /* Reserved block starts before start of sub-space */
                    if (((U32) BlockStop_p) <= ((U32) SubSpace_p->StopAddr_p))
                    {
                        /* Reserved block stops inside sub-space */
                        SizeDifference = ((U32) BlockStop_p) + 1 - ((U32) SubSpace_p->StartAddr_p);

                        if (Block_p->BlockAbove_p != NULL)
                        {
                            /* There's a block above, it can only be an unreserved one: enlarge it */
                            Block_p->BlockAbove_p->StartAddr_p = (void *) (((U32) Block_p->BlockAbove_p->StartAddr_p) - SizeDifference);
                            Block_p->BlockAbove_p->Size += SizeDifference;
                        }
                        else
                        {
                            /* There's no block above: create a block of unused memory and unreserve it */
                            /* Allocate block structure */
                            UnReserved_p = AllocBlockDescriptor(&(Partition_p->TopOfFreeBlocksList_p));
                            if (UnReserved_p == NULL)
                            {
                                /* Failed to allocate new block structure */
                                return(STAVMEM_ERROR_MAX_BLOCKS);
                            }

                            /* Block structure allocation successful */
                            /* Insert new block above block in space blocks list */
                            InsertBlockAboveBlock(&(Partition_p->VeryTopBlock_p), UnReserved_p, Block_p);

                            /* Setup new block as an unused block*/
                            UnReserved_p->StartAddr_p     = SubSpace_p->StartAddr_p;
                            UnReserved_p->Size            = SizeDifference;
                            UnReserved_p->IsUsed          = FALSE;
                            UnReserved_p->PartitionHandle = (STAVMEM_PartitionHandle_t) Partition_p;
                        }

                        /* Reduce size of reserved block */
                        Block_p->Size -= SizeDifference;
                    }
                    else
                    {
                        /* Reserved block stops after stop of sub-space */
/*                        SizeDifference = ((U32) BlockStop_p) + 1 - ((U32) SubSpace_p->StartAddr_p);*/
                        /* Create 2 new blocks: one to unreserve and one for the rest of reserved */
                        /* Allocate block structure */
                        UnReserved_p = AllocBlockDescriptor(&(Partition_p->TopOfFreeBlocksList_p));
                        if (UnReserved_p == NULL)
                        {
                            /* Failed to allocate new block structure */
                            return(STAVMEM_ERROR_MAX_BLOCKS);
                        }
                        else
                        {
                            /* Block structure allocation successful: Allocate 2nd block structure */
                            Above_p = AllocBlockDescriptor(&(Partition_p->TopOfFreeBlocksList_p));
                            if (Above_p == NULL)
                            {
                                /* Failed to allocate new block structure */
                                return(STAVMEM_ERROR_MAX_BLOCKS);
                            }
                        }

                        /* Insert new block to unreserve above block in space blocks list */
                        InsertBlockAboveBlock(&(Partition_p->VeryTopBlock_p), UnReserved_p, Block_p);

                        /* Setup new block as unused and unreserve it */
                        UnReserved_p->StartAddr_p     = SubSpace_p->StartAddr_p;
                        UnReserved_p->Size            = ((U32) SubSpace_p->StopAddr_p) + 1 - ((U32) SubSpace_p->StartAddr_p);
                        UnReserved_p->IsUsed          = FALSE;
                        UnReserved_p->PartitionHandle = (STAVMEM_PartitionHandle_t) Partition_p;

                        /* Insert new block above unreserved block in space blocks list */
                        InsertBlockAboveBlock(&(Partition_p->VeryTopBlock_p), Above_p, UnReserved_p);

                        /* Setup block above as reserved */
                        Above_p->Alignment = 1;
                        Above_p->StartAddr_p = (void *) (((U32) SubSpace_p->StopAddr_p) + 1);
                        Above_p->AlignedStartAddr_p = Above_p->StartAddr_p;
                        Above_p->Size = ((U32) BlockStop_p) + 1 - ((U32) Above_p->StartAddr_p);
                        Above_p->UserSize = Above_p->Size;
                        Above_p->AllocMode = STAVMEM_ALLOC_MODE_RESERVED;
                        Above_p->IsUsed = TRUE;

                        /* Merge block above Above_p if it is also reserved */
                        if ((Above_p->BlockAbove_p != NULL) && (Above_p->BlockAbove_p->IsUsed))
                        {
                            Above_p->Size += Above_p->BlockAbove_p->Size;
                            Above_p = Above_p->BlockAbove_p;

                            /* Remove block from space list of blocks */
                            RemoveBlock(&(Partition_p->VeryTopBlock_p), &(Partition_p->VeryBottomBlock_p), Above_p);

                            /* Free off block structure */
                            FreeBlockDescriptor(&(Partition_p->TopOfFreeBlocksList_p), Above_p);
                        }

                        /* Reduce size of reserved block */
                        Block_p->Size = ((U32) UnReserved_p->StartAddr_p) - ((U32) Block_p->StartAddr_p);
                    }
                }
                else
                {
                    /* Reserved block starts inside sub-space */
                    if (((U32) BlockStop_p) <= ((U32) SubSpace_p->StopAddr_p))
                    {
                        /* Reserved block stops inside sub-space: whole block must be un-reserved */
                        Block_p->IsUsed = FALSE;

                        Above_p = Block_p->BlockAbove_p;
                        /* Eventually merge with below and above unreserved blocks */
                        if (Above_p != NULL)
                        {
                            /* Merge the above block (which can only be unused) into the current unused memory block */
                            Block_p->Size += Above_p->Size;

                            /* Remove block from space list of blocks */
                            RemoveBlock(&(Partition_p->VeryTopBlock_p), &(Partition_p->VeryBottomBlock_p), Above_p);

                            /* Free off block structure */
                            FreeBlockDescriptor(&(Partition_p->TopOfFreeBlocksList_p), Above_p);
                        }

                        UnReserved_p = Block_p->BlockBelow_p;
                        if (UnReserved_p != NULL)
                        {
                            /* Merge the current unused memory block into the block below (which can only be unused) */
                            UnReserved_p->Size += Block_p->Size;

                            /* Remove block from space list of blocks */
                            RemoveBlock(&(Partition_p->VeryTopBlock_p), &(Partition_p->VeryBottomBlock_p), Block_p);

                            /* Free off block structure */
                            FreeBlockDescriptor(&(Partition_p->TopOfFreeBlocksList_p), Block_p);

                            /* Block_p is now invalid. Let it point to the block below. */
                            Block_p = UnReserved_p;
                            Block_p->PartitionHandle = (STAVMEM_PartitionHandle_t) Partition_p;
                        }
                    }
                    else
                    {
                        /* Reserved block stops after stop of sub-space */
                        SizeDifference = ((U32) SubSpace_p->StopAddr_p) + 1 - ((U32) BlockStart_p);

                        if (Block_p->BlockBelow_p != NULL)
                        {
                            /* There's a block below, it can only be unreserved: enlarge it */
                            Block_p->BlockBelow_p->Size += SizeDifference;
                        }
                        else
                        {
                            /* There's no unreserved block below: create a block of un-reserved */
                            /* Allocate block structure */
                            UnReserved_p = AllocBlockDescriptor(&(Partition_p->TopOfFreeBlocksList_p));
                            if (UnReserved_p == NULL)
                            {
                                /* Failed to allocate new block structure */
                                return(STAVMEM_ERROR_MAX_BLOCKS);
                            }

                            /* Block structure allocation successful:
                               Insert new block to unreserve below block in partition blocks list */
                            InsertBlockBelowBlock(&(Partition_p->VeryBottomBlock_p), UnReserved_p, Block_p);

                            /* Setup block to unreserve */
                            UnReserved_p->StartAddr_p     = BlockStart_p;
                            UnReserved_p->Size            = SizeDifference;
                            UnReserved_p->IsUsed          = FALSE;
                            UnReserved_p->PartitionHandle = (STAVMEM_PartitionHandle_t) Partition_p;
                        }
                        /* Reduce size of reserved block */
                        Block_p->Size     -= SizeDifference;
                        Block_p->UserSize -= SizeDifference;
                        Block_p->StartAddr_p = (void *) (((U32) Block_p->StartAddr_p) + SizeDifference);
                        Block_p->AlignedStartAddr_p = Block_p->StartAddr_p;
                    }
                } /* Reserved block treated */
            } /* else reserved block doesn't overlap sub-space: nothing to do */
        } /* else block is free: nothing to do */
        Block_p = Block_p->BlockAbove_p;
    } /* All blocks treated */

/*    stavmem_ViewAllBlocksInfo(Partition_p->VeryTopBlock_p, Partition_p->VeryBottomBlock_p);*/

    return(ST_NO_ERROR);
} /* End of UnReserveBlockNoSemaphore() */


/*******************************************************************************
Name        : InsertBlockAboveBlock
Description : Insert a block into the memory linked-list above the specified block.
              If the block is inserted at the top, VeryTopBlock_p is updated.
Parameters  : Pointer to the very top block pointer to update if block is at the very top
              Pointers to the block to insert and to the block in the list
Assumptions : Input pointers are not NULL
Limitations :
Returns     : None
*******************************************************************************/
static void InsertBlockAboveBlock(MemBlockDesc_t **VeryTopBlock_p, MemBlockDesc_t *InsertBlock_p, MemBlockDesc_t *ListBlock_p)
{
    InsertBlock_p->BlockBelow_p = ListBlock_p;
    InsertBlock_p->BlockAbove_p = ListBlock_p->BlockAbove_p;

    ListBlock_p->BlockAbove_p = InsertBlock_p;
    if (InsertBlock_p->BlockAbove_p != NULL)
    {
        InsertBlock_p->BlockAbove_p->BlockBelow_p = InsertBlock_p;
    }
    else
    {
        /* There is no block above: this is the top block of the list */
        *VeryTopBlock_p = InsertBlock_p;
    }

    return;
} /* End of InsertBlockAboveBlock() */



/*******************************************************************************
Name        : InsertBlockBelowBlock
Description : Insert a block into the memory linked-list below the specified block.
              If the block is inserted at the bottom, VeryBottomBlock_p is updated.
Parameters  : Pointer to the very bottom block pointer to update if block is at the very bottom
              Pointers to the block to insert and to the block in the list
Assumptions : Input pointers are not NULL
Limitations :
Returns     : None
*******************************************************************************/
static void InsertBlockBelowBlock(MemBlockDesc_t **VeryBottomBlock_p, MemBlockDesc_t *InsertBlock_p, MemBlockDesc_t *ListBlock_p)
{
    InsertBlock_p->BlockBelow_p = ListBlock_p->BlockBelow_p;
    InsertBlock_p->BlockAbove_p = ListBlock_p;

    ListBlock_p->BlockBelow_p = InsertBlock_p;
    if (InsertBlock_p->BlockBelow_p != NULL)
    {
        InsertBlock_p->BlockBelow_p->BlockAbove_p = InsertBlock_p;
    }
    else
    {
        /* There is no block below: this is the bottom block of the list */
        *VeryBottomBlock_p = InsertBlock_p;
    }

    return;
} /* End of InsertBlockBelowBlock() */



/*******************************************************************************
Name        : RemoveBlock
Description : Remove a block from the linked-list of memory blocks
              If the block is removed from the top, VeryTopBlock_p is updated.
              If the block is removed from the bottom, VeryBottomBlock_p is updated.
Parameters  : Pointers to the very top and very bottom pointers to update eventually
              Pointer to the block to remove
Assumptions : Input pointer is not NULL
Limitations :
Returns     : None
*******************************************************************************/
static void RemoveBlock(MemBlockDesc_t **VeryTopBlock_p, MemBlockDesc_t **VeryBottomBlock_p, MemBlockDesc_t *ListBlock_p)
{
    if (ListBlock_p->BlockAbove_p != NULL)
    {
        ListBlock_p->BlockAbove_p->BlockBelow_p = ListBlock_p->BlockBelow_p;
    }
    else
    {
        /* Just removed the top block: the one below becomes the top */
        *VeryTopBlock_p = ListBlock_p->BlockBelow_p;
    }

    if (ListBlock_p->BlockBelow_p != NULL)
    {
        ListBlock_p->BlockBelow_p->BlockAbove_p = ListBlock_p->BlockAbove_p;
    }
    else
    {
        /* Just removed the bottom block: the one above becomes the bottom */
        *VeryBottomBlock_p = ListBlock_p->BlockAbove_p;
    }
} /* End of RemoveBlock() */



/*******************************************************************************
Name        : AllocBlockDescriptor
Description : Allocate memory for a memory block structure
Parameters  : None
Assumptions :
Limitations :
Returns     : Pointer to the allocated memory block structure or
              NULL if the allocation failed.
*******************************************************************************/
static MemBlockDesc_t *AllocBlockDescriptor(MemBlockDesc_t **TopOfFreeBlocksList_p)
{
    MemBlockDesc_t *Mem_p;

    /* Check for a memory block in the list of free blocks */
    if (*TopOfFreeBlocksList_p != NULL)
    {
        /* Memory available in list of free blocks */
        Mem_p = (MemBlockDesc_t *) *TopOfFreeBlocksList_p;
        Mem_p->AllocMode = STAVMEM_ALLOC_MODE_FORBIDDEN; /* Just not invalid ! */

        /* Remove block from list of free blocks */
        *TopOfFreeBlocksList_p = Mem_p->BlockBelow_p;
    }
    else
    {
        /* Memory not available in list of free blocks, return fail */
        Mem_p = (MemBlockDesc_t *) NULL;
    }

    return(Mem_p);
} /* End of AllocBlockDescriptor() */


/*******************************************************************************
Name        : FreeBlockDescriptor
Description : Free memory of a memory block structure
              Insert block into the list of free blocks
Parameters  : Pointer to the memory block structure to free
Assumptions :
Limitations :
Returns     : None
*******************************************************************************/
static void FreeBlockDescriptor(MemBlockDesc_t **TopOfFreeBlocksList_p, MemBlockDesc_t *Mem_p)
{
    /* Insert block in list of free blocks */
    Mem_p->BlockBelow_p = *TopOfFreeBlocksList_p;
    Mem_p->BlockAbove_p = NULL;
    Mem_p->AllocMode = STAVMEM_ALLOC_MODE_INVALID;
    *TopOfFreeBlocksList_p = Mem_p;

    return;
}



/*******************************************************************************
Name        : RoundAlignment
Description : Round up alignment number to the nearest 2^n, where n = 0, 1, 2, 3, etc.
Parameters  : Pointer to the alignment number to be round up
Assumptions : Input pointer is not NULL
Limitations :
Returns     : TRUE if the alignment was modified, FALSE otherwise
*******************************************************************************/
static BOOL RoundUpAlignment(U32 *Alignment_p)
{
    U32 test, NewAlignment;
    BOOL Modified = FALSE;

    if (*Alignment_p == 0)
    {
        /* Alignment 0 is not valid: change it to 1 */
        *Alignment_p = 1;
        Modified = TRUE;
    }
    else if ((*Alignment_p & (*Alignment_p - 1)) != 0)
    {
        /* Alignment is not 2^n (where n = 0,1,2,3,etc), so round up to next 2^n. */
        NewAlignment = 2;
        test = *Alignment_p;
        while (test > 1)
        {
            NewAlignment <<= 1;
            test >>= 1;
        }

        *Alignment_p = NewAlignment;
        Modified = TRUE;
    }

    return(Modified);
} /* End of RoundAlignment() */



/*******************************************************************************
Name        : MarkForbiddenRangesAndBorders
Description : Build forbidden blocks on forbidden zones which are on unused blocks.
              Thus, when allocationg, it will be impossible to allocate on forbidden zones.
              In case of forbidden border (zone with 0 size), cut the unused block in 2,
              so that the border address will not be inside the block.
Parameters  : pointer to the partition structure the block belongs to and to the table
              of forbidden zones
Assumptions : Forbidden zones are ordered and without redundancy
Limitations :
Returns     : ST_NO_ERROR if success, STAVMEM_ERROR_MAX_BLOCKS if no more blocks available
*******************************************************************************/
static ST_ErrorCode_t MarkForbiddenRangesAndBorders(stavmem_Partition_t *Partition_p, const U32 NumberOfForbiddenRanges,
                                                    STAVMEM_MemoryRange_t *NotOrderedForbiddenRangeArray_p,
                                                    const U32 NumberOfForbiddenBorders, void **NotOrderedForbiddenBorderArray_p)
{
    MemBlockDesc_t *Cur_p = NULL, *Above_p = NULL, *Forbid_p = NULL;
    U32 AboveSize = 0, BelowSize = 0, i;
    void *ForbidStop_p, *ForbidStart_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Check all unused blocks of partition for ranges */
    Cur_p = Partition_p->VeryBottomBlock_p;
    while (Cur_p != NULL)
    {
        /* Only unused blocks are concerned */
        if (!Cur_p->IsUsed)
        {
            /* Current block is unused: test if any forbidden range concerns it */
            for (i = 0; i < NumberOfForbiddenRanges; i++)
            {
                /* There's overlap if unused block starts before end of the
                  forbidden zone and stops after the begining of it */
                /* If overlap with forbidden range, create a forbidden block or mark as forbidden */
                ForbidStart_p = NotOrderedForbiddenRangeArray_p[i].StartAddr_p;
                ForbidStop_p = NotOrderedForbiddenRangeArray_p[i].StopAddr_p;
                if ((((U32) Cur_p->StartAddr_p) <= ((U32) ForbidStop_p)) && ((((U32) Cur_p->StartAddr_p) + Cur_p->Size - 1) >= ((U32) ForbidStart_p)))
                {
                    /* Consider part of forbidden zone inside current block */
                    if (((U32) ForbidStart_p) < ((U32) Cur_p->StartAddr_p))
                    {
                        ForbidStart_p = Cur_p->StartAddr_p;
                    }
                    if (((U32) ForbidStop_p) > (((U32) Cur_p->StartAddr_p) + Cur_p->Size - 1))
                    {
                        ForbidStop_p = (void *) (((U32) Cur_p->StartAddr_p) + Cur_p->Size - 1);
                    }

                    /* Calculate size of unused block below forbidden block */
                    BelowSize = ((U32) ForbidStart_p) - ((U32) Cur_p->StartAddr_p);
                    /* Calculate size of unused block above forbidden block */
                    AboveSize = ((U32) Cur_p->StartAddr_p) + Cur_p->Size - 1 - ((U32) ForbidStop_p);

                    /* Add blocks */
                    if (BelowSize > 0)
                    {
                        /* Create a new block to reserve it later: Allocate block structure */
                        Forbid_p = AllocBlockDescriptor(&(Partition_p->TopOfFreeBlocksList_p));

                        if (Forbid_p == NULL)
                        {
                            /* Failed to allocate new block structure */
                            return(STAVMEM_ERROR_MAX_BLOCKS);
                        }
                    }

                    if (AboveSize > 0)
                    {
                        /* Create block above the block to reserve: allocate block structure */
                        Above_p = AllocBlockDescriptor(&(Partition_p->TopOfFreeBlocksList_p));

                        if (Above_p == NULL)
                        {
                            /* Failed to allocate new block structure */
                            if (Forbid_p != NULL)
                            {
                                FreeBlockDescriptor(&(Partition_p->TopOfFreeBlocksList_p), Forbid_p);
                            }
                            return(STAVMEM_ERROR_MAX_BLOCKS);
                        }
                    }

                    /* Block(s) successfully created */
                    if (BelowSize == 0)
                    {
                        Forbid_p = Cur_p;
                    }
                    else
                    {
                        /* Insert new block above current block in partition blocks list */
                        InsertBlockAboveBlock(&(Partition_p->VeryTopBlock_p), Forbid_p, Cur_p);

                        /* Reduce size of bottom block */
                        Cur_p->Size = BelowSize;
                    }

                    Forbid_p->AllocMode   = STAVMEM_ALLOC_MODE_FORBIDDEN;
                    Forbid_p->StartAddr_p = ForbidStart_p;
/*                    Forbid_p->Alignment   = 1;
                    Forbid_p->AlignedStartAddr_p = ForbidStart_p;*/
                    Forbid_p->Size        = ((U32) ForbidStop_p) + 1 - ((U32) ForbidStart_p);
/*                    Forbid_p->UserSize    = Forbid_p->Size;*/

                    /* IsUsed is only set to TRUE here for forbidden blocks */
                    Forbid_p->IsUsed = TRUE;

                    if (AboveSize > 0)
                    {
                        /* Insert new block above forbidden block in partition blocks list */
                        InsertBlockAboveBlock(&(Partition_p->VeryTopBlock_p), Above_p, Forbid_p);

                        /* Setup new block */
                        Above_p->StartAddr_p = (void *) (((U32) ForbidStop_p) + 1);
                        Above_p->Size        = AboveSize;
                        Above_p->IsUsed      = FALSE;
                    }
                    else
                    {
                        /* No unused block above: check if block above is also forbidden */
                        Above_p = Forbid_p->BlockAbove_p;
                        if ((Above_p != NULL) && (Above_p->IsUsed) && (Above_p->AllocMode == STAVMEM_ALLOC_MODE_FORBIDDEN))
                        {
                            /* Block above is also forbidden: merge the 2 forbidden blocks */
                            Forbid_p->Size += Above_p->Size;
/*                            Forbid_p->UserSize = Forbid_p->Size;*/

                            /* Remove block above from the list of blocks */
                            RemoveBlock(&(Partition_p->VeryTopBlock_p), &(Partition_p->VeryBottomBlock_p), Above_p);

                            /* Free off structure */
                            FreeBlockDescriptor(&(Partition_p->TopOfFreeBlocksList_p), Above_p);
                        }
                    }
                } /* else not overlaping forbidden range */
            } /* end of 'for each forbidden range' */
        } /* else block is used */

        Cur_p = Cur_p->BlockAbove_p;
    } /* All blocks treated */

    /* Check all unused blocks of partition for borders */
    Cur_p = Partition_p->VeryBottomBlock_p;
    while (Cur_p != NULL)
    {
        /* Only unused blocks are concerned */
        if (!Cur_p->IsUsed)
        {
            /* Current block is unused: test if any forbidden border concerns it */
            for (i = 0; i < NumberOfForbiddenBorders; i++)
            {
                /* Considering a border only, but no real zone.
                   The border should not be crossed: it can only be the first address */
                /* If border inside the current unused block, cut the block in 2 unused blocks */
                if ((((U32) NotOrderedForbiddenBorderArray_p[i]) > ((U32) Cur_p->StartAddr_p)) && (((U32) NotOrderedForbiddenBorderArray_p[i]) <= ((U32) Cur_p->StartAddr_p) + Cur_p->Size - 1))
                {
                    /* Separate the unused block in 2 unused blocks: allocate a block structure */
                    Above_p = AllocBlockDescriptor(&(Partition_p->TopOfFreeBlocksList_p));
                    if (Above_p == NULL)
                    {
                        /* Failed to allocate new block structure */
                        return(STAVMEM_ERROR_MAX_BLOCKS);
                    }

                    /* Insert new block above current block in partition blocks list */
                    InsertBlockAboveBlock(&(Partition_p->VeryTopBlock_p), Above_p, Cur_p);

                    /* Setup above block */
                    Above_p->StartAddr_p = NotOrderedForbiddenBorderArray_p[i];
                    Above_p->Size = ((U32) Cur_p->StartAddr_p) + Cur_p->Size - ((U32) NotOrderedForbiddenBorderArray_p[i]);
                    Above_p->IsUsed = FALSE;

                    Cur_p->Size -= Above_p->Size;
                } /* else not crossing forbidden border */
            } /* end of 'for each forbidden border' */
        } /* else block is used */

        Cur_p = Cur_p->BlockAbove_p;
    } /* All blocks treated */


#ifdef DEBUG_CODE
    Cur_p = Partition_p->VeryBottomBlock_p;
    while (Cur_p != NULL)
    {
        /* Only unused blocks are concerned */
        if (!Cur_p->IsUsed)
        {
            /* Current block is unused: test if any forbidden range concerns it */
            for (i = 0; i < NumberOfForbiddenRanges; i++)
            {
                /* There's overlap if unused block starts before end of the
                  forbidden zone and stops after the begining of it */
                /* If overlap with forbidden range, create a forbidden block or mark as forbidden */
                ForbidStart_p = NotOrderedForbiddenRangeArray_p[i].StartAddr_p;
                ForbidStop_p = NotOrderedForbiddenRangeArray_p[i].StopAddr_p;
                if ((((U32) Cur_p->StartAddr_p) <= ((U32) ForbidStop_p)) && ((((U32) Cur_p->StartAddr_p) + Cur_p->Size - 1) >= ((U32) ForbidStart_p)))
                {
                    /* Error case */
                    stavmem_BlockAndBip();
                } /* else not overlaping forbidden range */
            } /* end of 'for each forbidden range' */

            /* Current block is unused: test if any forbidden border concerns it */
            for (i = 0; i < NumberOfForbiddenBorders; i++)
            {
                /* Considering a border only, but no real zone.
                   The border should not be crossed: it can only be the first address */
                /* If border inside the current unused block, cut the block in 2 unused blocks */
                if ((((U32) NotOrderedForbiddenBorderArray_p[i]) > ((U32) Cur_p->StartAddr_p)) && (((U32) NotOrderedForbiddenBorderArray_p[i]) <= ((U32) Cur_p->StartAddr_p) + Cur_p->Size - 1))
                {
                    /* Error case */
                    stavmem_BlockAndBip();
                } /* else not crossing forbidden border */
            } /* end of 'for each forbidden border' */
        } /* else block is used */

        Cur_p = Cur_p->BlockAbove_p;
    } /* All blocks treated */
#endif /* #ifdef DEBUG_CODE */

    return(ErrorCode);
} /* End of MarkForbiddenRangesAndBorders() */



/*******************************************************************************
Name        : UnMarkForbiddenRangesAndBorders
Description : Set forbidden blocks as unused and merge contiguous ones
Parameters  : pointer to the partition structure the block belongs to
Assumptions : pointer is not NULL
Limitations :
Returns     : ST_NO_ERROR, cannot fail if Partition_p is valid
*******************************************************************************/
static void UnMarkForbiddenRangesAndBorders(stavmem_Partition_t *Partition_p)
{
    MemBlockDesc_t *Cur_p = NULL, *Top_p = NULL;

    /* Check all forbidden blocks and set them as unused */
    Cur_p = Partition_p->VeryBottomBlock_p;
    while (Cur_p != NULL)
    {
        /* Only forbidden blocks (used) are concerned */
        if ((Cur_p->IsUsed) && (Cur_p->AllocMode == STAVMEM_ALLOC_MODE_FORBIDDEN))
        {
            /* The block is unused now */
            Cur_p->IsUsed = FALSE;
        } /* Else block is not forbidden */
        Cur_p = Cur_p->BlockAbove_p;
    }

    /* Merge contiguous unused blocks of partition */
    Cur_p = Partition_p->VeryBottomBlock_p;
    while (Cur_p != NULL)
    {
        /* Only unused blocks are concerned */
        if (!Cur_p->IsUsed)
        {
            /* Merge all unused blocks directly above the current one */
            while ((Cur_p->BlockAbove_p != NULL) && (!Cur_p->BlockAbove_p->IsUsed))
            {
                /* Merge all above contiguous unused blocks */
                Top_p = Cur_p->BlockAbove_p;

                /* Merge the unused above block into the current unused memory block */
                Cur_p->Size += Top_p->Size;

                /* Remove block from the list of blocks */
                RemoveBlock(&(Partition_p->VeryTopBlock_p), &(Partition_p->VeryBottomBlock_p), Top_p);

                /* Free off structure */
                FreeBlockDescriptor(&(Partition_p->TopOfFreeBlocksList_p), Top_p);
            }
        } /* Else block is used */

        Cur_p = Cur_p->BlockAbove_p;
    }
} /* End of UnMarkForbiddenRangesAndBorders() */



/*******************************************************************************
Name        : IsInForbiddenRangeOrBorder
Description : Tells if a particular place of memory is crossing a forbidden zone
Parameters  : Start and stop address of the memory to test
Assumptions : StartAddr != 0, StopAddr >= StartAddr
Limitations :
Returns     : TRUE if crossing forbidden zone, FALSE otherwise
*******************************************************************************/
static BOOL IsInForbiddenRangeOrBorder(const void *StartAddr_p, const void *StopAddr_p, const U32 NumberOfForbiddenRanges, STAVMEM_MemoryRange_t *NotOrderedForbiddenRangeArray_p, const U32 NumberOfForbiddenBorders, void **NotOrderedForbiddenBorderArray_p)
{
    U32 i;
    BOOL IsForbidden = FALSE;

    i = 0;
    while ((!IsForbidden) && (i < NumberOfForbiddenRanges))
    {
        /* Test if block overlaps any forbidden range.
           There's overlap if reserved block starts before end of the sub-space
           and stops after the begining of sub-space */
        if ((((U32) StartAddr_p) <= ((U32) NotOrderedForbiddenRangeArray_p[i].StopAddr_p)) && (((U32) StopAddr_p) >= ((U32) NotOrderedForbiddenRangeArray_p[i].StartAddr_p)))
        {
            IsForbidden = TRUE;
        }
        i++;
    }

    i = 0;
    while ((!IsForbidden) && (i < NumberOfForbiddenBorders))
    {
        /* Test if any forbidden border crosses block.
           The border should not be crossed: it can only be the first or last address */
        if ((((U32) NotOrderedForbiddenBorderArray_p[i]) > ((U32) StartAddr_p)) && (((U32) NotOrderedForbiddenBorderArray_p[i]) <= ((U32) StopAddr_p)))
        {
            IsForbidden = TRUE;
        }
        i++;
    }

    return(IsForbidden);
} /* End of IsInForbiddenRangeOrBorder() */




/* Obsolete functions (cimetary) -------------------------------------------- */


#ifdef GET_LARGEST_FREE_BLOCK_SIZE
/*
--------------------------------------------------------------------------------
Get largest free memory block available in partition of a certain area type
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STAVMEM_GetLargestFreeBlockSize(STAVMEM_PartitionHandle_t PartitionHandle, STAVMEM_AllocMode_t BlockAllocMode, U32 *LargestFreeBlockSize_p)
{
    MemBlockDesc_t *Block_p;
    U32 LargestSize = 0;
    stavmem_Partition_t *Partition_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;



    /* Get pointer to partition the block belongs to */
    Partition_p = (stavmem_Partition_t *) PartitionHandle;

    /* Exit if the partition is not valid */
    if (Partition_p->Validity != AVMEM_VALID_PARTITION)
    {
        return(STAVMEM_ERROR_INVALID_PARTITION_HANDLE);
    }

    /* Exit if bad parameter */
    if (LargestFreeBlockSize_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

#ifdef USE_SEMAPHORE
    /* Wait for the allocations locking semaphore to be free */
    STOS_SemaphoreWait(Partition_p->Device_p->LockAllocSemaphore_p);
#endif

    if (BlockAllocMode == STAVMEM_ALLOC_MODE_TOP_BOTTOM)
    {
        /* Check all block from top */
        Block_p = Partition_p->VeryTopBlock_p;
        while (Block_p != NULL)
        {
#ifdef DO_NOT_ALLOW_AREAS_CROSSING
            /* Stop looking for largest unused memory when the other area is hit */
            if (Block_p->IsUsed && (Block_p->AllocMode != STAVMEM_ALLOC_MODE_TOP_BOTTOM) && (Block_p->AllocMode != STAVMEM_ALLOC_MODE_RESERVED))
            {
                break;
            }
#endif

            /* Update the largest size, if this block is the largest so far */
            if ((!Block_p->IsUsed) && (Block_p->Size > LargestSize))
            {
                LargestSize = Block_p->Size;
            }

            Block_p = Block_p->BlockBelow_p;
        }
    }
    else if (BlockAllocMode == STAVMEM_ALLOC_MODE_BOTTOM_TOP)
    {
        /* Check all block from bottom */
        Block_p = Partition_p->VeryBottomBlock_p;
        while (Block_p != NULL)
        {
#ifdef DO_NOT_ALLOW_AREAS_CROSSING
            /* Stop looking for largest unused memory when the other area is hit */
            if (Block_p->IsUsed && (Block_p->AllocMode != STAVMEM_ALLOC_MODE_BOTTOM_TOP) && (Block_p->AllocMode != STAVMEM_ALLOC_MODE_RESERVED))
            {
                break;
            }
#endif

            /* Update the largest size, if this block is the largest so far */
            if ((!Block_p->IsUsed) && (Block_p->Size > LargestSize))
            {
                LargestSize = Block_p->Size;
            }

            Block_p = Block_p->BlockAbove_p;
        }
    }
    else
    {
        /* Exit if cannot consider block of this area type */
        return(STAVMEM_ERROR_INVALID_BLOCK_HANDLE);
    }

#ifdef USE_SEMAPHORE
    /* Free the allocations locking semaphore */
    STOS_SemaphoreSignal(Partition_p->Device_p->LockAllocSemaphore_p);
#endif

    /* Output result: largest size found in area */
    *LargestFreeBlockSize_p = LargestSize;
    return(ErrorCode);
} /* End of STAVMEM_GetLargestFreeBlockSize() */
#endif /* #ifdef GET_LARGEST_FREE_BLOCK_SIZE */


#ifdef FREE_AREA
/*****************************************************************************
*   Function Title: STAVMEM_FreeArea()
******************************************************************************
*   Synopsis:   Free all memory blocks of the specified area.
*   Description:    Run down the mpeg memory block linked-list and
*           mpeg_mem_free() used blocks of the specified area.
*   Inputs:     area  - Type of memory to be freed.
*   Outputs:    None.
*****************************************************************************/
ST_ErrorCode_t STAVMEM_FreeArea(STAVMEM_Handle_t Handle, STAVMEM_AllocMode_t FreeAllocMode)
{
    MemBlockDesc_t *Last_p, *Cur_p = NULL;
    stavmem_Partition_t *Partition_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Get pointer to partition the block belongs to */
    Partition_p = (stavmem_Partition_t *) Handle;

    /* Exit if the partition is not valid */
    if (Partition_p->Validity != AVMEM_VALID_PARTITION)
    {
        return(STAVMEM_ERROR_INVALID_PARTITION_HANDLE);
    }

#ifdef USE_SEMAPHORE
    /* Wait for the allocations locking semaphore to be free */
    STOS_SemaphoreWait(Partition_p->Device_p->LockAllocSemaphore_p);
#endif

    Cur_p = Partition_p->VeryTopBlock_p;
    Last_p = Cur_p;
    while (Cur_p != NULL)
    {
        /* If memory block is unused and is of the specified area then free it off */
        if (Cur_p->IsUsed && (Cur_p->AllocMode == FreeAllocMode))
        {
            /* Free block */
            ErrorCode = FreeNoSemaphore(Partition_p, Cur_p);
            /* Always success */

            /* Restart search from the previous block as the current block has just been freed off */
            Cur_p = Last_p;
        }
        Last_p = Cur_p;
        Cur_p = Cur_p->BlockBelow_p;
    }

#ifdef USE_SEMAPHORE
    /* Free the allocations locking semaphore */
    STOS_SemaphoreSignal(Partition_p->Device_p->LockAllocSemaphore_p);
#endif

    return(ErrorCode);
} /* End of STAVMEM_FreeArea() */
#endif /* #ifdef FREE_AREA */



#ifdef FREE_ALL
/*****************************************************************************
*   Function Title: STAVMEM_FreeAll()
******************************************************************************
*   Synopsis:   Deallocate all areas of previously allocated blocks of
*           mpeg memory.
*   Description:    Free off all linked-list mpeg memory block structures.
*   Inputs:     None.
*   Outputs:    None.
*****************************************************************************/
ST_ErrorCode_t STAVMEM_FreeAll(STAVMEM_Handle_t Handle)
{
    MemBlockDesc_t *vmb, *Bot_p;
    stavmem_Partition_t *Partition_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Get pointer to partition the block belongs to */
    Partition_p = (stavmem_Partition_t *) Handle;

    /* Exit if the partition is not valid */
    if (Partition_p->Validity != AVMEM_VALID_PARTITION)
    {
        return(STAVMEM_ERROR_INVALID_PARTITION_HANDLE);
    }

#ifdef USE_SEMAPHORE
    /* Wait for the allocations locking semaphore to be free */
    STOS_SemaphoreWait(Partition_p->Device_p->LockAllocSemaphore_p);
#endif

    /* Free off all allocated memory structures */
    for (vmb = Partition_p->VeryTopBlock_p; vmb != NULL; vmb = Bot_p)
    {
        Bot_p = vmb->BlockBelow_p;

        /* Zero values in structure just in case someone uses it after its been freed!!! */
        vmb->BlockAbove_p = NULL;
        vmb->BlockBelow_p = NULL;
/*        vmb->StartAddr    = 0;
        vmb->Size         = 0;*/
        vmb->Alignment    = 1;
/*        vmb->AlignedStartAddr = 0;
        vmb->AlignedSize  = 0;*/
        vmb->IsUsed       = FALSE;

        /* Update counter to respect MaxUsedBlocks */
        Partition_p->Device_p->CurrentlyUsedBlocks--;

        /* Free off block structure */
        FreeBlockDescriptor(&(Partition_p->TopOfFreeBlocksList_p), vmb);
    }
    Partition_p->VeryTopBlock_p = NULL;
    Partition_p->VeryBottomBlock_p = NULL;

#ifdef USE_SEMAPHORE
    /* Free the allocations locking semaphore */
    STOS_SemaphoreSignal(Partition_p->Device_p->LockAllocSemaphore_p);
#endif

    return(ErrorCode);
} /* End of STAVMEM_FreeAll() */
#endif /* #ifdef FREE_ALL */


#ifdef COMPRESS_AREA
/*****************************************************************************
*   Function Title: STAVMEM_CompressArea()
******************************************************************************
*   Synopsis:   Defragment a memory area of specified area.
*   Description:    Shift all the used memory blocks together at the
*           top(STAVMEM_ALLOC_MODE_BOTTOM_TOP) or bottom(STAVMEM_ALLOC_MODE_TOP_BOTTOM) ends
*           of memory, depending on the memory block requiring
*           compression.
*           NOTE:   After calling this function all the addresses
*               previously returned by STAVMEM_MemGetAddress() are
*               invalid for the specified memory area.
*   Inputs:     area  - Area of mpeg memory to compress.
*   Outputs:    None.
*****************************************************************************/
void STAVMEM_CompressArea(STAVMEM_Handle_t Handle, STAVMEM_AllocMode_t CompressAllocMode)
{
    MemBlockDesc_t *Cur_p;
    MemBlockDesc_t *Top_p, *Bot_p;
    U32 align_mask;
    U32 size_total, size;
    void *CurrentStartAddr_p, *CurrentAlignedStartAddr_p;
    U32 diff;
    stavmem_Partition_t *Partition_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Get pointer to partition the block belongs to */
    Partition_p = (stavmem_Partition_t *) Handle;

    /* Exit if the partition is not valid */
    if (Partition_p->Validity != AVMEM_VALID_PARTITION)
    {
/*        return(STAVMEM_ERROR_INVALID_PARTITION_HANDLE);*/
    }

#ifdef USE_SEMAPHORE
    /* Wait for the allocations locking semaphore to be free */
    STOS_SemaphoreWait(Partition_p->Device_p->LockAllocSemaphore_p);
#endif

    if (CompressAllocMode == STAVMEM_ALLOC_MODE_TOP_BOTTOM)
    {
        /* Ripple stack free store towards tail of list, but not past any
        STAVMEM_ALLOC_MODE_BOTTOM_TOP store. Merge free store blocks on the way */
        for (Cur_p = Partition_p->Device_p->SpaceVeryTopBlock_p;
            Cur_p != NULL && Cur_p->BlockBelow_p != NULL;
            Cur_p = Cur_p->BlockBelow_p)
        {
            /* End the compress if heap memory is hit, else skip this block if it is used */
            if (Cur_p->IsUsed)
            {
#ifdef DO_NOT_ALLOW_AREAS_CROSSING
/* Comment by HG: this is a test concerning areas overlap !!! */
this is a test concerning areas overlap !!!
#endif
                if (Cur_p->AllocMode == STAVMEM_ALLOC_MODE_TOP_BOTTOM)
                {
                    continue;
                }
                else
                {
                    break;
                }
            }
            /* The current memory block is unused */

            /* If the prev memory block is unused then merge it with the current unused block */
            Top_p = Cur_p->BlockAbove_p;
            if ((Top_p != NULL) && (!Top_p->IsUsed))
            {
                /* The previous block is unused so merge it with the current unused block */
                Top_p->Size += Cur_p->Size;

                /* Remove block from partition list of blocks */
                RemoveBlock(&(Partition_p->VeryTopBlock_p), &(Partition_p->VeryBottomBlock_p), Cur_p);

                /* Free off block structure */
                FreeBlockDescriptor(&(Partition_p->TopOfFreeBlocksList_p), Cur_p);

                /* make the previous block the current block */
                Cur_p = Top_p;
            }

            Bot_p = Cur_p->BlockBelow_p;
            if ((Bot_p != NULL) && Bot_p->IsUsed)
            {
                /* End the compress if the next block is heap memory */
#ifdef DO_NOT_ALLOW_AREAS_CROSSING
/* Comment by HG: this is a test concerning areas overlap !!! */
this is a test concerning areas overlap !!!
#endif
                if (Bot_p->AllocMode != STAVMEM_ALLOC_MODE_TOP_BOTTOM)
                {
                    break;
                }

                /* The next block is used and is not heap
                memory so swap it with the current unused block */
                align_mask = Bot_p->Alignment - 1;

                size_total = Cur_p->Size + Bot_p->Size;
                CurrentStartAddr_p = Cur_p->StartAddr_p;
                CurrentAlignedStartAddr_p = (void *) ((((U32) CurrentStartAddr_p) + align_mask) & ~align_mask);

                diff = ((U32) CurrentAlignedStartAddr_p) - ((U32) CurrentStartAddr_p);
/*                size = Bot_p->AlignedSize + diff;
but this must be changed because this doesn't mean the same thing !!! */
                size = Bot_p->UserSize + diff;

                /* Adjust used block base pointers and actual size */
                Bot_p->StartAddr_p        = CurrentStartAddr_p;
                Bot_p->AlignedStartAddr_p = CurrentAlignedStartAddr_p;
                Bot_p->Size       = size;

                /* Adjust unused block base pointer and actual size */
                Cur_p->Size = size_total - size;
                Cur_p->StartAddr_p = (void *) (((U32) CurrentStartAddr_p) + Cur_p->Size);

                /* Swap block order in linked-list, if the
                unused block is zero size then free it off */
                /* Remove block from partition list of blocks */
                RemoveBlock(&(Partition_p->VeryTopBlock_p), &(Partition_p->VeryBottomBlock_p), Cur_p);
                if (Cur_p->Size == 0)
                {
                    /* Free off block structure */
                    FreeBlockDescriptor(&(Partition_p->TopOfFreeBlocksList_p), Cur_p);
                }
                else
                {
                    /* Insert new block below current block in partition blocks list */
                    InsertBlockBelowBlock(&(Partition_p->VeryBottomBlock_p), Cur_p, Bot_p);
                }

                /* Update current pointer */
                Cur_p = Bot_p;
            }
        }
    }
    else if (CompressAllocMode == STAVMEM_ALLOC_MODE_BOTTOM_TOP)
    {
        /* Ripple heap free store towards head of list, but not past any
        STAVMEM_ALLOC_MODE_TOP_BOTTOM store. Merge free store blocks on the way */
        for (Cur_p = Partition_p->Device_p->SpaceVeryBottomBlock_p;
            Cur_p != NULL && Cur_p->BlockAbove_p != NULL;
            Cur_p = Cur_p->BlockAbove_p )
        {
            /* End the compress if stack memory is hit, else skip this block if it is used */
            if (Cur_p->IsUsed)
            {
#ifdef DO_NOT_ALLOW_AREAS_CROSSING
/* Comment by HG: this is a test concerning areas overlap !!! */
this is a test concerning areas overlap !!!
#endif
                if (Cur_p->AllocMode == STAVMEM_ALLOC_MODE_BOTTOM_TOP)
                {
                    continue;
                }
                else
                {
                    break;
                }
            }
            /* The current memory block is unused */

            /* If the next memory block is unused then merge it with the current unused block */
            Bot_p = Cur_p->BlockBelow_p;
            if ((Bot_p != NULL) && (!Bot_p->IsUsed))
            {
                /* The next block is unused so merge it with the current unused block */
                Cur_p->Size += Bot_p->Size;

                /* Remove the next block from the linked-list and free it off */
                /* Remove block from partition list of blocks */
                RemoveBlock(&(Partition_p->VeryTopBlock_p), &(Partition_p->VeryBottomBlock_p), Bot_p);

                /* Free off block structure */
                FreeBlockDescriptor(&(Partition_p->TopOfFreeBlocksList_p), Bot_p);
            }

            Top_p = Cur_p->BlockAbove_p;
            if ((Top_p != NULL) && Top_p->IsUsed)
            {
                /* End the compress if the previous block is stack memory */
#ifdef DO_NOT_ALLOW_AREAS_CROSSING
/* Comment by HG: this is a test concerning areas overlap !!! */
this is a test concerning areas overlap !!!
#endif
                if (Top_p->AllocMode != STAVMEM_ALLOC_MODE_BOTTOM_TOP)
                {
                    break;
                }

                /* The previous block is used and is not stack
                memory so swap it with the current unused block */
                align_mask = Top_p->Alignment - 1;

                size_total = Cur_p->Size + Top_p->Size;
/*                CurrentStartAddr_p = (void *) (((U32) Top_p->StartAddr_p) + size_total - Top_p->AlignedSize);
but this must be changed because this doesn't mean the same thing !!! */
                CurrentStartAddr_p = (void *) (((U32) Top_p->StartAddr_p) + size_total - Top_p->UserSize);
                CurrentAlignedStartAddr_p = (void *) (((U32)CurrentStartAddr_p) & ~align_mask);

                diff = ((U32) CurrentStartAddr_p) - ((U32) CurrentAlignedStartAddr_p);
/*                size = Top_p->AlignedSize + diff;
but this must be changed because this doesn't mean the same thing !!! */
                size = Top_p->UserSize + diff;

                /* Adjust used block base pointers and actual size */
                Top_p->StartAddr_p        = CurrentAlignedStartAddr_p;
                Top_p->AlignedStartAddr_p = CurrentAlignedStartAddr_p;
                Top_p->Size       = size;

                /* Adjust unused block base pointer and actual size */
                Cur_p->Size = size_total - size;
                Cur_p->StartAddr_p = (void *) (((U32) CurrentAlignedStartAddr_p) - Cur_p->Size);

                /* Swap block order in linked-list, if the
                unused block is zero size then free it off */
                /* Remove block from partition list of blocks */
                RemoveBlock(&(Partition_p->VeryTopBlock_p), &(Partition_p->VeryBottomBlock_p), Cur_p);
                if (Cur_p->Size == 0)
                {
                    /* Free off block structure */
                    FreeBlockDescriptor(&(Partition_p->TopOfFreeBlocksList_p), Cur_p);
                }
                else
                {
                    /* Insert current block above block in partition blocks list */
                    InsertBlockAboveBlock(&(Partition_p->VeryTopBlock_p), Cur_p, Top_p);
                }

                /* Update current pointer */
                Cur_p = Top_p;
            }
        }
    }

#ifdef USE_SEMAPHORE
    /* Free the allocations locking semaphore */
    STOS_SemaphoreSignal(Partition_p->Device_p->LockAllocSemaphore_p);
#endif

    ErrorCode = ST_NO_ERROR;
    return;
} /* End of STAVMEM_CompressArea() */
#endif /* #ifdef COMPRESS_AREA */


/* End of avm_allo.c */

