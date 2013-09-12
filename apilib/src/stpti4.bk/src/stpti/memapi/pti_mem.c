/******************************************************************************
COPYRIGHT (C) STMicroelectronics 2005.  All rights reserved.

   File Name: pti_mem.c
 Description: low level memory manager for stpti

******************************************************************************/

/* Includes ---------------------------------------------------------------- */


#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
 #define STTBX_PRINT
#endif

#if !defined(ST_OSLINUX)
#include <assert.h>
#include <limits.h>
#include <string.h>
#endif /* #if !defined(ST_OSLINUX) */

#include "stddefs.h"
#if !defined(ST_OSLINUX)
#include "sttbx.h"
#endif /* #if !defined(ST_OSLINUX) */

#include "pti_list.h"
#include "pti_loc.h"
#include "pti_hndl.h"
#include "pti_mem.h"
#include "memget.h"


/* Private Constants ------------------------------------------------------- */


 /* no point in leaving a memory cell that is so small that it can't be used for anything....
    Unilateral decision to make that size 4 words, plus the header of course */

#define MINIMUM_CELL_SIZE   (sizeof(CellHeader_t) + 4*sizeof(unsigned int))


/* Private Variables ------------------------------------------------------- */

static U32 MaxScanCount = 16;

#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
 static U32 RunningTotal = 0;
#endif
 

/* global */
semaphore_t *stpti_MemoryLock = NULL;


/* Private Function Prototypes --------------------------------------------- */


static Cell_t *FindBestFitCell(Cell_t *FreeList_p, size_t Size);

static void LinkCell  (Cell_t **FreeList,   Cell_t *Cell_p);
static void UnlinkCell(Cell_t **FreeList_p, Cell_t *Cell_p);

static void SplitCell        (Cell_t *BestFitCell_p, size_t Size);
static void MergeCellWithNext(Cell_t *Cell);

static void ScanFreeList    (MemCtl_t *MemCtl_p);
static void CloseUpUsedCells(MemCtl_t *MemCtl_p, Cell_t * Cell);

#ifdef ST_OSLINUX
void WalkBlockAndFree( MemCtl_t *MemCtl_p);
#endif

/* API Functions ----------------------------------------------------------- */


/******************************************************************************
Function Name : stpti_MemInit
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t stpti_MemInit(ST_Partition_t *Partition, size_t MemSize, size_t HndlSize, MemCtl_t *MemCtl)
{
#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
    static U32 total = 0;
    U32 this;


    this = MemSize + HndlSize;
    RunningTotal += this;
    STTBX_Print(("stpti_MemInit() allocate %u bytes (rt: %u)\n", this, RunningTotal));
#endif

    MemCtl->FreeList_p = (Cell_t *) memory_allocate(Partition, MemSize);
    if (MemCtl->FreeList_p == NULL)
    {
        return ST_ERROR_NO_MEMORY;
    }

    MemCtl->Handle_p = (Handle_t *) memory_allocate(Partition, HndlSize);
    
    if (MemCtl->Handle_p == NULL)
    {
        memory_deallocate(Partition, (void *) MemCtl->FreeList_p);
        return ST_ERROR_NO_MEMORY;
    }

    /* Initialise MemCtl Block */

    MemCtl->MemSize = MemSize;
    MemCtl->Block_p = MemCtl->FreeList_p;        /* Keep track of pointer to memory block for
                                                    re-allocation purposes */
    MemCtl->Partition_p = Partition;

    
    MemCtl->MaxHandles = HndlSize / sizeof(Handle_t);

    /* Initialise Cell Header parameters */

    MemCtl->FreeList_p->Header.next_p = MemCtl->FreeList_p->Header.prev_p = NULL;
    MemCtl->FreeList_p->Header.Size = MemSize - sizeof(CellHeader_t);
    MemCtl->FreeList_p->Header.Handle.word = STPTI_NullHandle();

    /* Initialise Handle array */

    {
        U16 indx;

        for (indx = 0; indx < MemCtl->MaxHandles; ++indx)
        {
            MemCtl->Handle_p[indx].Hndl_u.word = STPTI_NullHandle();
            MemCtl->Handle_p[indx].Size = 0;
            MemCtl->Handle_p[indx].Data_p = NULL;
        }
    }
    return ST_NO_ERROR;
}


#if !defined ( STPTI_BSL_SUPPORT )
/******************************************************************************
Function Name : stpti_MemTerm
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t stpti_MemTerm(MemCtl_t * MemCtl)
{
    ST_ErrorCode_t error = ST_NO_ERROR;

    /* Free memory - data space */
    assert(SEMAPHORE_P_COUNT(stpti_MemoryLock)== 0); /* Must be entered with Memory Locked */

#if 0
    if (MemCtl->Block_p != MemCtl->FreeList_p)
    {
        error = STPTI_ERROR_INTERNAL_ALL_MESSED_UP; /* If not memory controller has gone VERY wrong 
                                                     */
        assert(MemCtl->Block_p == MemCtl->FreeList_p);
    }
    else
#else
    if (MemCtl->Block_p != MemCtl->FreeList_p)
    {
        STTBX_Print(("BAD Free list %08X %08X\n", (int)MemCtl->Block_p, (int)MemCtl->FreeList_p));
    }
#endif
    {
        memory_deallocate(MemCtl->Partition_p, (void *) MemCtl->Block_p);
        
        memory_deallocate(MemCtl->Partition_p, (void *) MemCtl->Handle_p);

        MemCtl->Partition_p = NULL;
        MemCtl->FreeList_p = NULL;
        MemCtl->Handle_p = NULL;
        MemCtl->MaxHandles = 0;
    }
    return error;
}
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */


/******************************************************************************
Function Name : stpti_MemAlloc
  Description :
   Parameters :
******************************************************************************/
Cell_t *stpti_MemAlloc(Cell_t ** FreeList, size_t Size)
{
    Cell_t *BestFitCell_p;

    assert(SEMAPHORE_P_COUNT(stpti_MemoryLock)== 0); /* Must be entered with Memory Locked */

    /* round Size up to a 4 byte boundary */
    Size += 3;
    Size &= ~3;

    /* Search free list for a cell that is the closest to the required size */

    BestFitCell_p = FindBestFitCell(*FreeList, Size);

    if (BestFitCell_p != NULL)
    {
        if ((BestFitCell_p->Header.Size - Size) > MINIMUM_CELL_SIZE)
        {
            /* Chop off the bit we need */
            SplitCell(BestFitCell_p, Size);
        }
        UnlinkCell(FreeList, BestFitCell_p);
    }

    return BestFitCell_p;
}


#if !defined ( STPTI_BSL_SUPPORT )
/******************************************************************************
Function Name : stpti_MemDealloc
  Description :
   Parameters :
******************************************************************************/
void stpti_MemDealloc(MemCtl_t * MemCtl_p, U16 Instance)
{
    Handle_t *Handle_p = &MemCtl_p->Handle_p[Instance];

    assert(SEMAPHORE_P_COUNT(stpti_MemoryLock)== 0); /* Must be entered with Memory Locked */

    /* NULL Handle in Memory Cell */

    Handle_p->Data_p->Header.Handle.word = STPTI_NullHandle();

    /* link cell into free list */

    LinkCell((Cell_t **) & MemCtl_p->FreeList_p, Handle_p->Data_p);
    ScanFreeList(MemCtl_p);                      /* Check for potential merges and closures */

    /* NULL Deallocated Handle */
    Handle_p->Size = 0;
    Handle_p->Data_p = NULL;
    Handle_p->Hndl_u.word = STPTI_NullHandle();
}
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */


/******************************************************************************
Function Name : ReallocHandleSpace
  Description :
   Parameters :
******************************************************************************/
static ST_ErrorCode_t ReallocHandleSpace(MemCtl_t * MemCtl)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    Handle_t *NewHandle_p;
    U16 OldMaxHandles;
    U16 NewMaxHandles;
    size_t NewHndlSize;

    OldMaxHandles = MemCtl->MaxHandles;

    NewMaxHandles = MAXIMUM(2, (OldMaxHandles * 3) / 2);
    NewHndlSize = NewMaxHandles * sizeof(Handle_t);
    /* Round up to 4 byte boundary */
    NewHndlSize += 3;
    NewHndlSize &= ~3;



#if defined(ST_OSLINUX)
    NewHandle_p = (Handle_t *) memory_reallocate(MemCtl->Partition_p,
                                                (void *) MemCtl->Handle_p,
                                                NewHndlSize,
                                                OldMaxHandles * sizeof(Handle_t));
#else
    NewHandle_p = (Handle_t *) memory_reallocate(MemCtl->Partition_p, (void *) MemCtl->Handle_p, NewHndlSize);
#endif /* #endif defined(ST_OSLINUX) */
    
    if (NewHandle_p == NULL)
    {
        error = ST_ERROR_NO_MEMORY;
    }
    else
    {
        U16 indx;

#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
        RunningTotal += NewHndlSize;
        STTBX_Print(("ReallocHandleSpace() allocate %u bytes (rt: %u)\n", NewHndlSize, RunningTotal));
#endif
        /* Initialise New handle space */
        for (indx = OldMaxHandles; indx < NewMaxHandles; ++indx)
        {
            NewHandle_p[indx].Hndl_u.word = STPTI_NullHandle();
            NewHandle_p[indx].Size = 0;
            NewHandle_p[indx].Data_p = NULL;
        }
        MemCtl->MaxHandles = NewMaxHandles;
        MemCtl->Handle_p = NewHandle_p;
    }
    
    return error;
}


/******************************************************************************
Function Name : ReallocObjectSpace
  Description :
   Parameters :
******************************************************************************/
static ST_ErrorCode_t ReallocObjectSpace(MemCtl_t * MemCtl_p)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    Cell_t *OldBlock_p = MemCtl_p->Block_p;
    Cell_t *NewBlock_p = NULL;
    size_t OldSize = MemCtl_p->MemSize;
    size_t NewSize;

    NewSize = (OldSize * 3) / 2;
    /* Round up to 4 byte boundary */
    NewSize += 3;
    NewSize &= ~3;

#if defined(ST_OSLINUX)
    NewBlock_p = (Cell_t *) memory_reallocate(MemCtl_p->Partition_p, (void *) OldBlock_p, NewSize, OldSize);
#else
    NewBlock_p = (Cell_t *) memory_reallocate(MemCtl_p->Partition_p, (void *) OldBlock_p, NewSize);
#endif /* #endif defined(ST_OSLINUX) */

    if (NewBlock_p == NULL)
    {
        error = ST_ERROR_NO_MEMORY;
    }
    else
    {
        U16 indx;
        S32 BlockOffset = (U8 *) NewBlock_p - (U8 *) OldBlock_p;
        Cell_t *NewCell_p;
        Cell_t *Cell_p;

#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
        RunningTotal += NewSize;
        STTBX_Print(("ReallocObjectSpace() allocate %u bytes (rt: %u)\n", NewSize, RunningTotal));
#endif
        /* Fix Up Free Space List */

        MemCtl_p->Block_p = NewBlock_p;
        MemCtl_p->MemSize = NewSize;

        if (MemCtl_p->FreeList_p != NULL)
        {
            MemCtl_p->FreeList_p = (Cell_t *) ((U8 *) MemCtl_p->FreeList_p + BlockOffset);
            MemCtl_p->FreeList_p->Header.prev_p = NULL;
        }

        /* Plod through the free list fixing up the next/prev pointers */

        Cell_p = MemCtl_p->FreeList_p;
        while (Cell_p != NULL)
        {
            if (Cell_p->Header.next_p != NULL)
                Cell_p->Header.next_p = (Cell_t *) ((U8 *) Cell_p->Header.next_p + BlockOffset);
            if (Cell_p->Header.prev_p != NULL)
                Cell_p->Header.prev_p = (Cell_t *) ((U8 *) Cell_p->Header.prev_p + BlockOffset);

            Cell_p = Cell_p->Header.next_p;
        }

        /* Create a new cell with the extra memory just added */

        NewCell_p = (Cell_t *) ((U8 *) NewBlock_p + OldSize);
        NewCell_p->Header.next_p = NULL;
        NewCell_p->Header.prev_p = NULL;
        NewCell_p->Header.Handle.word = STPTI_NullHandle();
        NewCell_p->Header.Size = (NewSize - OldSize) - sizeof(CellHeader_t);

        LinkCell((Cell_t **) & MemCtl_p->FreeList_p, NewCell_p);

        /* Fix up Handles */

        if (BlockOffset != 0)
        {
            for (indx = 0; indx < MemCtl_p->MaxHandles; ++indx)
            {
                if (MemCtl_p->Handle_p[indx].Hndl_u.word != STPTI_NullHandle())
                {
                    MemCtl_p->Handle_p[indx].Data_p = (Cell_t *) ((U8 *) MemCtl_p->Handle_p[indx].Data_p + BlockOffset);
                }
            }
        }
    }

    return error;
}


/******************************************************************************
Function Name : stpti_CreateObject
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t stpti_CreateObject(MemCtl_t * MemCtl, U16 *Instance, size_t ObjectSize)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    static U32 indx = 0;

    assert(SEMAPHORE_P_COUNT(stpti_MemoryLock)== 0); /* Must be entered with Memory Locked */

    /* Scan Handle Array for next Free entry */

    while (error == ST_NO_ERROR)
    {
        U32 i;
        for (i = 0; i < MemCtl->MaxHandles; ++i, ++indx)
        {
            if (indx >= MemCtl->MaxHandles)
                indx = 0;
            if (MemCtl->Handle_p[indx].Data_p == NULL)
            {
                /* Free Handle Found */
                break;
            }
        }
 
        if (i == MemCtl->MaxHandles)
        {
            error = ReallocHandleSpace(MemCtl);
        }
        else
        {
            error = ST_NO_ERROR;
            break;
        }
    }

    while (error == ST_NO_ERROR)
    {
        MemCtl->Handle_p[indx].Size = ObjectSize;
        MemCtl->Handle_p[indx].Data_p = stpti_MemAlloc((Cell_t **) & MemCtl->FreeList_p, ObjectSize);

        if (MemCtl->Handle_p[indx].Data_p == NULL)
        {
            MemCtl->Handle_p[indx].Size = 0;
            error = ReallocObjectSpace(MemCtl);
        }
        else
        {
            error = ST_NO_ERROR;
            break;
        }
    }
    
    if (error == ST_NO_ERROR)
    {
        *Instance = indx;
    }
    
    return error;
}



/* ------------------------------------------------------------------------- */
/*                            Local functions                                */
/* ------------------------------------------------------------------------- */



/******************************************************************************
Function Name : ScanFreeList
  Description :
   Parameters :
******************************************************************************/
static void ScanFreeList(MemCtl_t * MemCtl_p)
{
    Cell_t *List_p;
    U16 ScanCount = 0;
    boolean NoChange = FALSE;

    assert(SEMAPHORE_P_COUNT(stpti_MemoryLock)== 0); /* Must be entered with Memory Locked */

    /* scan through free list looking for instances where merging of cells into larger cells can
       occur, or where used memory can be repacked and thus reduce fragmentation */

    while (!NoChange && (ScanCount < MaxScanCount))
    {
        NoChange = TRUE;

        /* Check for adjacent cells that can be merged */

        List_p = MemCtl_p->FreeList_p;
        while (List_p->Header.next_p != NULL)
        {
            if ((U8 *) List_p + List_p->Header.Size + sizeof(CellHeader_t) == (U8 *) List_p->Header.next_p)
            {
                MergeCellWithNext(List_p);
                NoChange = FALSE;
            }

            /* Merging cells will modify the next pointer so check before dereferencing */
            if (List_p->Header.next_p == NULL)
                break;
            List_p = List_p->Header.next_p;
        }

        /* Check for used memory beyond free memory that can be closed up */

        List_p = MemCtl_p->FreeList_p;
        while (List_p->Header.next_p != NULL)
        {
            if ((U32) List_p + List_p->Header.Size + sizeof(CellHeader_t) < (U32) List_p->Header.next_p)
            {
                CloseUpUsedCells(MemCtl_p, List_p);

                /* After a Close up the list pointer will be invalid so we MUST break out of the
                   while loop */

                NoChange = FALSE;
                break;
            }
            List_p = List_p->Header.next_p;
        }

        ++ScanCount;
    }
}


/******************************************************************************
Function Name : MergeCellWithNext
  Description :
   Parameters :
******************************************************************************/
static void MergeCellWithNext(Cell_t * Cell)
{
    assert(SEMAPHORE_P_COUNT(stpti_MemoryLock)== 0); /* Must be entered with Memory Locked */

    Cell->Header.Size += (Cell->Header.next_p->Header.Size + sizeof(CellHeader_t));

    if (Cell->Header.next_p->Header.next_p != NULL)
        Cell->Header.next_p->Header.next_p->Header.prev_p = Cell;

    Cell->Header.next_p = Cell->Header.next_p->Header.next_p;
}


/******************************************************************************
Function Name : CloseUpUsedCells
  Description :
   Parameters :
******************************************************************************/
static void CloseUpUsedCells(MemCtl_t * MemCtl_p, Cell_t * Cell_p)
{
    Cell_t *NextCell_p;
    Cell_t *PrevCell_p;
    size_t FreeSize;

    Cell_t *UsedCell_p;
    size_t UsedSize;
    FullHandle_t Handle;

    Cell_t *NewLocation_p;

    /* Remember freecell parameters */

    NextCell_p = Cell_p->Header.next_p;
    PrevCell_p = Cell_p->Header.prev_p;
    FreeSize = Cell_p->Header.Size;

    /* Calculate a pointer to used memory area */

    UsedCell_p = (Cell_t *) ((U8 *) Cell_p + Cell_p->Header.Size + sizeof(CellHeader_t));

    /* Get size of Used cell */

    UsedSize = UsedCell_p->Header.Size;
    Handle.word = UsedCell_p->Header.Handle.word;

    /* Copy data to new location */

    memmove((void *) Cell_p, (void *) UsedCell_p, UsedSize + sizeof(CellHeader_t));

    /* Fix-up parameters in handle data */

    MemCtl_p->Handle_p[Handle.member.Object].Data_p = Cell_p;

    /* Link up free cell in new location */

    NewLocation_p = (Cell_t *) ((U8 *) Cell_p + UsedSize + sizeof(CellHeader_t));

    if (PrevCell_p == NULL)
    {
        MemCtl_p->FreeList_p = NewLocation_p;
    }
    else
    {
        PrevCell_p->Header.next_p = NewLocation_p;
    }

    if (NextCell_p != NULL)
    {
        NextCell_p->Header.prev_p = NewLocation_p;
    }

    NewLocation_p->Header.prev_p = PrevCell_p;
    NewLocation_p->Header.next_p = NextCell_p;
    NewLocation_p->Header.Handle.word = STPTI_NullHandle();
    NewLocation_p->Header.Size = FreeSize;
}


/******************************************************************************
Function Name : FindBestFitCell
  Description :
   Parameters :
******************************************************************************/
static Cell_t *FindBestFitCell(Cell_t * FreeList_p, size_t Size)
{
    Cell_t *List_p = FreeList_p;
    Cell_t *BestCell_p = NULL;
    size_t SizeError = UINT_MAX;

    assert(SEMAPHORE_P_COUNT(stpti_MemoryLock)== 0); /* Must be entered with Memory Locked */

    /* Scan to find the cell whoose size is closest, but not less than the required size */

    while (List_p != NULL)
    {
        if (List_p->Header.Size >= Size)
        {
            if ((List_p->Header.Size - Size) < SizeError)
            {
                BestCell_p = List_p;
                SizeError = List_p->Header.Size - Size;
            }
        }
        List_p = List_p->Header.next_p;
    }
    return BestCell_p;
}


/******************************************************************************
Function Name : UnlinkCell
  Description :
   Parameters :
******************************************************************************/
static void UnlinkCell(Cell_t ** FreeList_p, Cell_t * Cell_p)
{
    Cell_t *PrevCell_p;
    Cell_t *NextCell_p;

    PrevCell_p = Cell_p->Header.prev_p;
    NextCell_p = Cell_p->Header.next_p;

    assert(SEMAPHORE_P_COUNT(stpti_MemoryLock)== 0); /* Must be entered with Memory Locked */

    if (PrevCell_p == NULL)
    {
        /* at start of list */
        *FreeList_p = NextCell_p;
    }
    else
    {
        PrevCell_p->Header.next_p = NextCell_p;
    }

    if (NextCell_p == NULL)
    {
        /* At end of list */

    }
    else
    {
        NextCell_p->Header.prev_p = PrevCell_p;
    }
}


/******************************************************************************
Function Name : SplitCell
  Description :
   Parameters :
******************************************************************************/
static void SplitCell(Cell_t * Cell_p, size_t Size)
{
    Cell_t *NewCell;

    assert(SEMAPHORE_P_COUNT(stpti_MemoryLock)== 0); /* Must be entered with Memory Locked */

    /* Section is removed at top... create a new cell at oldcell + size */

    NewCell = (Cell_t *) ((U8 *) Cell_p + Size + sizeof(CellHeader_t));
    NewCell->Header.prev_p = Cell_p;
    NewCell->Header.next_p = Cell_p->Header.next_p;
    NewCell->Header.Size = Cell_p->Header.Size - (Size + sizeof(CellHeader_t));

#ifdef ST_OSLINUX
    /* This make the proc output look better. */
    NewCell->Header.Handle.word = STPTI_NullHandle();
#endif
    
    /* link into list */

    if (Cell_p->Header.next_p != NULL)
    {
        Cell_p->Header.next_p->Header.prev_p = NewCell;
    }
    Cell_p->Header.next_p = NewCell;
    Cell_p->Header.Size = Size;
}


/******************************************************************************
Function Name : LinkCell
  Description :
   Parameters :
******************************************************************************/
static void LinkCell(Cell_t ** FreeList_p, Cell_t * Cell_p)
{
    Cell_t *List_p = *FreeList_p;

    assert(SEMAPHORE_P_COUNT(stpti_MemoryLock)== 0); /* Must be entered with Memory Locked */

    if (List_p == NULL)
    {
        Cell_p->Header.next_p = NULL;
        Cell_p->Header.prev_p = NULL;
        *FreeList_p = Cell_p;
    }
    else if ((U32)List_p > (U32)Cell_p)
    {
        /* cell should be linked onto start of list */
        Cell_p->Header.next_p = List_p;
        Cell_p->Header.prev_p = NULL;

        List_p->Header.prev_p = Cell_p;
        *FreeList_p = Cell_p;
    }
    else
    {
        while (List_p->Header.next_p != NULL)
        {
            if ((U32)List_p->Header.next_p > (U32)Cell_p)
            {
                /* next node is too big ... so link cell between this cell and next */

                Cell_p->Header.next_p = List_p->Header.next_p;
                Cell_p->Header.prev_p = List_p;

                List_p->Header.next_p->Header.prev_p = Cell_p;
                List_p->Header.next_p = Cell_p;
                break;
            }
            List_p = List_p->Header.next_p;
        }
        if (List_p->Header.next_p == NULL)
        {
            /* At end of list so tack node on here */
            Cell_p->Header.next_p = NULL;
            Cell_p->Header.prev_p = List_p;

            List_p->Header.next_p = Cell_p;
        }
    }
}


/******************************************************************************
Function Name : stpti_ReallocateObjectList
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t stpti_ReallocateObjectList(FullHandle_t ListHandle, U32 NumberEntries)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    MemCtl_t *MemCtl_p;
    Cell_t *NewPtr;
    Cell_t *OldPtr;
    size_t NewSize;
    size_t OldSize;
    U16 OldMaxHandles;
    U16 OldUsedHandles;
    List_t *List_p;


    assert(SEMAPHORE_P_COUNT(stpti_MemoryLock)== 0); /* Must be entered with Memory Locked */

    List_p = stptiMemGet_List(ListHandle);

    OldMaxHandles  = List_p->MaxHandles;
    OldUsedHandles = List_p->UsedHandles;

    MemCtl_p = &(stptiMemGet_Session(ListHandle)->MemCtl);
    NewSize = sizeof( List_t ) + sizeof(STPTI_Handle_t) * (NumberEntries - 1);

    while (error == ST_NO_ERROR)
    {

        NewPtr = stpti_MemAlloc((Cell_t **) & MemCtl_p->FreeList_p, NewSize);
        if (NewPtr == NULL)
        {
            error = ReallocObjectSpace(MemCtl_p);
        }
        else
        {
            error = ST_NO_ERROR;
            break;
        }
    }
    if (error == ST_NO_ERROR)
    {
        /* Reallocation was successful  :-) */

        /* Remember old data location and size */

        OldPtr  = MemCtl_p->Handle_p[ListHandle.member.Object].Data_p;
        OldSize = MemCtl_p->Handle_p[ListHandle.member.Object].Size;

        /* Patch up handle array to point to new data */

        MemCtl_p->Handle_p[ListHandle.member.Object].Data_p = NewPtr;
        MemCtl_p->Handle_p[ListHandle.member.Object].Size = NewSize;

        /* Copy data to new location */

        /* If resizing upwards a memcpy could be used, but not for getting smaller... a good time
           to repack the list anyway!  */

        {
            U16 OldIndex, NewIndex;

            for (OldIndex = 0, NewIndex = 0; OldIndex < OldMaxHandles; ++OldIndex)
            {
                if ((&((List_t *) & OldPtr->data)->Handle)[OldIndex] != STPTI_NullHandle())
                {
                    (&((List_t *) & NewPtr->data)->Handle)[NewIndex++] =
                        (&((List_t *) & OldPtr->data)->Handle)[OldIndex];
                }
            }
            while (NewIndex < NumberEntries)
            {
                (&((List_t *) & NewPtr->data)->Handle)[NewIndex++] = STPTI_NullHandle();
            }
            ((List_t *) & NewPtr->data)->MaxHandles = NumberEntries;
            ((List_t *) & NewPtr->data)->UsedHandles = OldUsedHandles;
        }

        /* modify CellHeader of newdata cell ( size should be set by MemAlloc */

        NewPtr->Header.Handle.word = OldPtr->Header.Handle.word;

        /* link Old cell into free list */

        LinkCell((Cell_t **) & MemCtl_p->FreeList_p, OldPtr);
        ScanFreeList(MemCtl_p);                  /* Check for potential merges and closures */
    }
    return error;
}


/* --- EOF --- */
