/*******************************************************************************

File name : avm_init.h

Description : Audio and video memory init header file (device relative definitions)

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
17 jun 1999         Created                                          HG
 6 Oct 1999         Suppressed Open/Close and introduce
                    partitions functions                             HG
13 Oct 1999         Added base addresses as init params              HG
10 Jan 2000         Moved base addresses to mem access module        HG
*******************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __AVMEM_INIT_H
#define __AVMEM_INIT_H


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */

#define STAVMEM_MAX_DEVICE         2
#ifndef STAVMEM_MAX_MAX_PARTITION
#ifdef ST_OSLINUX
    #define STAVMEM_MAX_MAX_PARTITION  4
#else
#if defined(DVD_SECURED_CHIP)
  #ifdef WINCE_MSTV_ENHANCED
    #define STAVMEM_MAX_MAX_PARTITION  4
  #else
    #define STAVMEM_MAX_MAX_PARTITION  3
  #endif  
#else /* !DVD_SECURED_CHIP */
  #ifdef WINCE_MSTV_ENHANCED
    #define STAVMEM_MAX_MAX_PARTITION  4
  #else
    #define STAVMEM_MAX_MAX_PARTITION  2
  #endif  
#endif /* DVD_SECURED_CHIP */
#endif /*ST_OSLINUX*/
#endif /*STAVMEM_MAX_MAX_PARTITION*/

/* Exported Types ----------------------------------------------------------- */

typedef struct MemBlockDesc_s
{
    STAVMEM_PartitionHandle_t PartitionHandle; /* Handle to the partition in which the block was allocated */

    struct MemBlockDesc_s *BlockAbove_p;    /* Link to the block above in the list of blocks */
    struct MemBlockDesc_s *BlockBelow_p;    /* Link to the block below in the list of blocks */

/* Actual block base and size */
    void *StartAddr_p;              /* First physical address of the block */
    U32   Size;                     /* Size of the block in bytes. 0 means full size */
    BOOL  IsUsed;                   /* TRUE if the block is used, FALSE if it is unused (free) */

/* Area and aligned block base and size (only valid when IsUsed is TRUE) */
    STAVMEM_AllocMode_t AllocMode;  /* Way the block was allocated */
    U32   Alignment;                /* Alignement of the first address of the block */
    void *AlignedStartAddr_p;       /* First address of the block for the user (aligned) */
    U32   UserSize;                 /* User specified size when alloc. 0 means full range */
} MemBlockDesc_t;

/*   Memory Block structure fields :

      StartAddr_p
          |    AlignedStartAddr_p (>= StartAddr_p)
          |        /
          V       V
          .<---------------------- Size --------------------->.
          .       .<----------- UserSize ------------>.       .
          .       .            (<= Size)              .       .
  MEMORY: |=======|===================================|=======|
 Example: 0010    0100                                1101    1111
 in case of allocation with size=9 and alignment=4
*/


typedef struct AvmemDevice_s
{
    ST_DeviceName_t DeviceName;

    semaphore_t *LockAllocSemaphore_p; /* Allocations locking semaphore. Inside critical functions, this semaphore should be:
                                      - waited just before any action is done (e.g. after check params and validity)
                                      - signaled just after the last action (e.g. before writting the final result) */
    ST_Partition_t *CPUPartition_p; /* Where the module allocated memory for its own usage,
                                     also where it will have to be freed at termination */

    void *MemoryAllocated_p;        /* Pointer to the memory to free at termination */

    /* common information of the initialisation */
    U32 MaxPartitions;              /* Max number of partitions that the user can create in this 'device' */
    void *FirstBlock_p;             /* First block structure allocated */
    U32 TotalAllocatedBlocks;       /* Total number of allocated blocks */
    U32 MaxUsedBlocks;              /* User defined max number of blocks */
    U32 CurrentlyUsedBlocks;        /* Number of blocks used currently */

    /* common informations for all partitions on a particular device */
    MemBlockDesc_t *TopOfSpaceFreeBlocksList_p; /* = NULL */
    U32 MaxForbiddenRanges;         /* Max number of forbidden ranges to take into account */
    U32 MaxForbiddenBorders;        /* Max number of forbidden borders to take into account */
#ifdef ST_OSLINUX
    void          *BigphysAllocated_p;
    unsigned long BigphysSize;
#endif
} stavmem_Device_t;


/* Exported Variables ------------------------------------------------------- */



/* Exported Macros ---------------------------------------------------------- */


/* Exported Functions ------------------------------------------------------- */


extern stavmem_Device_t *stavmem_GetPointerOnDeviceNamed(const ST_DeviceName_t WantedName);


/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __AVMEM_INIT_H */

/* End of avm_init.h */

