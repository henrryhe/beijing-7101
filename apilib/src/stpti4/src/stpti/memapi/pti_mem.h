/******************************************************************************
COPYRIGHT (C) STMicroelectronics 2005.  All rights reserved.

  File Name: pti_mem.h
Description: low level memory manager for stpti

******************************************************************************/

#ifndef __PTI_MEM_H
 #define __PTI_MEM_H


/* Includes ------------------------------------------------------------ */


#include "stddefs.h"

#include "handle.h"
#include "cell.h"


/* Exported Types ------------------------------------------------------ */


typedef volatile struct Handle_s 
{
    FullHandle_t Hndl_u;
    size_t       Size;
    Cell_t      *Data_p;
} Handle_t;


typedef volatile struct MemCtl_s 
{
    ST_Partition_t *Partition_p;
    size_t          MemSize;
    Cell_t         *Block_p;
    Cell_t         *FreeList_p;
    Handle_t       *Handle_p;
    U32             MaxHandles;
} MemCtl_t;


/* Exported Variables -------------------------------------------------- */


extern semaphore_t *stpti_MemoryLock;


/* Exported Function Prototypes ---------------------------------------- */

ST_ErrorCode_t stpti_MemInit     (ST_Partition_t *Partition, size_t MemSize, size_t HndlSize, MemCtl_t *MemCtl);
ST_ErrorCode_t stpti_MemTerm     (MemCtl_t *MemCtl);

Cell_t        *stpti_MemAlloc    (Cell_t **FreeList, size_t Size);
void           stpti_MemDealloc  (MemCtl_t *MemCtl_p, U16 Instance);

ST_ErrorCode_t stpti_CreateObject(MemCtl_t *MemCtl, U16 *Instance, size_t ObjectSize);

#endif /* __PTI_MEM_H */
