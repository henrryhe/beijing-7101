/*! Time-stamp: <@(#)OS21WrapperMemory.h   5/11/2005 - 8:42:07 AM   William Hennebois>
 *********************************************************************
 *  @file   : OS21WrapperMemory.h
 *
 *  Project : 
 *
 *  Package : 
 *
 *  Company : TeamLog 
 *
 *  Author  : William Hennebois            Date: 5/11/2005
 *
 *  Purpose : definitions for Memory Management
 *
 *********************************************************************
 * Version History:
 *
 * V 0.10  5/11/2005  WH : First Revision
 *
 *********************************************************************
 */

#ifndef __OS21WrapperMemory__
#define __OS21WrapperMemory__

#include "stddefs.h"

// re-implementation of OS21-style types

//#define ST_WINCE_USER_MODE 1			// OLGN - USER_MODE Top Level Define

extern BOOL bWCEKernelMode;

typedef enum 
{
  partition_status_type_simple = 0,
  partition_status_type_fixed,
  partition_status_type_heap,
  partition_status_type_user

} partition_status_type_t;

typedef enum 
{
  partition_status_state_invalid = 0,
  partition_status_state_valid

} partition_status_state_t;


typedef unsigned int partition_status_flags_t;

typedef struct partition_status_s
{
  partition_status_state_t partition_status_state;
  partition_status_type_t  partition_status_type;
  int	                   partition_status_size;
  int	                   partition_status_free;
  int	                   partition_status_free_largest;
  int	                   partition_status_used;
  int                      partition_status_unused[4];

} partition_status_t;

#define DEFAULT_ADDRESS_WIDTH	(0x1000) // for register 

// re-implementation of OS21-style functions
int partition_status(partition_t* pp, partition_status_t* status, partition_status_flags_t flags);

partition_t* partition_create_heap(void *ExternalBlock, unsigned int size);
void         partition_delete_heap(partition_t* pp);
void *memory_allocate      (partition_t * pp, int size);
void *memory_allocate_clear(partition_t * pp, size_t nelem, int elsize);
void  memory_deallocate    (partition_t * pp, void * ptr);
void *memory_reallocate    (partition_t * pp, void * ptr, int size);

// accessory functions
void * partition_translate_nocache_CE(void *pBlock);
partition_t * partition_create_heap_NoCacheCE(unsigned int size);
void * partition_translate_Physique_CE(void *pBlock);

void cache_purge_data(void *base_address, unsigned int length);
void cache_flush_data(void *base_address, unsigned int length);
void cache_invalidate_data(void *base_address, unsigned int length);

//! initilize memory stuff at the boot time
void _WCE_MemoryInitilize();
void _WCE_MemoryTerminate();



// MapPhysicalToVirtual()
// Maps <size> bytes of uncached virtual memory to <physicalAddress>
// Usage:
//     Use this function to get R/W access to the chip registers
//     Usually called by STxxx_Init()
//     Warning: <physicalAddress> must be a multiple of 256.
//     Returns NULL if it fails

LPVOID MapPhysicalToVirtual(LPVOID physicalAddress, DWORD size);

// UnmapPhysicalToVirtual()
// Unmaps & free virtual memory returned by MapPhysicalToVirtual()
// Usage:
//     Use this function to unmap physical memory & free virtual memory 
//     Usually called by STxxx_Term()

void UnmapPhysicalToVirtual(LPVOID virtualAddress);
void WCE_SetSharableMemoryPartition(void *pAdress,unsigned long iSize);
void * _WCE_AllocSharableMemory(unsigned int size,int typeAlign /* =16 */);

void DisplayRAMPartitionStatus(void);


#endif
