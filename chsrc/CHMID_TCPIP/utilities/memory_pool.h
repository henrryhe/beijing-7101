/*******************************************************************************
* Copyright (c) 2008 SMSC
* All Rights Reserved.
*
* FILE: memory_pool.h
*
* PURPOSE: Declares utilities that provide fast memory management.
* It is used when a module frequently needs to allocate and free
*   the same type of structure.
*
* BRIEF IMPLEMENTATION DESCRIPTION:
*   A memory pool is a memory region that is formatted as a 
* linked list of equal sized memory units. The memory region
* is provided by the user of that memory pool. The user 
* calculates the memory region size by using the macro 
*     MEMORY_POOL_SIZE(unitSize, maximumUnits)
* The user can declare this memory region globally as
*     static u8_t memoryRegion[MEMORY_POOL_SIZE(sizeof(myUnit),10)];
* Or the user may allocated the memory region from a heap as
*     u8_t * memoryRegion=(u8_t *)malloc(MEMORY_POOL_SIZE(sizeof(myUnit),10));
* The user then initializes the memory region using
*     MEMORY_POOL_HANDLE memoryPoolHandle=
*         MemoryPool_Initialize(memoryRegion,sizeof(myUnit),10,NULL);
* Then a memory unit can be allocated by calling
*     void * memoryUnit=MemoryPool_Allocate(memoryPoolHandle);
* At this point if memoryUnit is not NULL then it points to a 
*   memory region that is at least the sizeof(myUnit). It is also
*   memory aligned according to MEMORY_ALIGNMENT as specified in 
*   custom_options.h. 
* If memoryUnit is NULL then there were no more units available
*   in the memory pool.
* A memory unit that was previously allocated is freed and returned
*   to the pool with
*     MemoryPool_Free(memoryPoolHandle,memoryUnit);
*
* NOTE: 
*   These functions are NOT multithread safe. They are designed 
* to work only with in a single threaded environment. However
* different threads can each create and use there own memory pool.
* But multiple threads should not access the same memory pool
* unless proper protection is used outside of these memory pool functions.
*******************************************************************************/

#ifndef MEMORY_POOL_H
#define MEMORY_POOL_H

#include "smsc_environment.h"

#ifdef MEMORY_POOL_CHANGE_090217
#define MEMORY_MAXUNIT_SIZE   4096
#endif
/*******************************************************************************
* STRUCTURE: MEMORY_POOL
* PURPOSE:
*    This structure in intended to be opaque. That means users of this
*    memory pool API should not depend on this structure in any way. 
*    It is declared here simply because the macro MEMORY_POOL_SIZE references
*    sizeof(MEMORY_POOL). But users should not require knowledge of 
*    this structure and should simply trust the macro MEMORY_POOL_SIZE to 
*    provide the proper size of the memory region.
*******************************************************************************/
typedef struct MEMORY_POOL_ {
	DECLARE_SIGNATURE
	void ** mHeadUnit;
	SMSC_ERROR_ONLY(void * mPoolEnd;)/*used for error checking only*/
#ifdef MEMORY_POOL_CHANGE_090217
       int i_Size;
       int i_MaximumUnits;
       boolean MemUsed[MEMORY_MAXUNIT_SIZE]; 		
#endif		
} MEMORY_POOL, * PMEMORY_POOL;

/*******************************************************************************
* TYPE: MEMORY_POOL_HANDLE
* PURPOSE:
*    This is the type returned from MemoryPool_Initialize.
*    It represents a memory pool that is ready to be used.
*    This type is passed to MemoryPool_Allocate, and MemoryPool_Free.
*******************************************************************************/
typedef void * MEMORY_POOL_HANDLE;

/*******************************************************************************
* MACRO: MEMORY_POOL_SIZE
* PURPOSE: used to figure how much memory to preallocate 
*   for the call to MemoryPool_Initialize
* PARAMETERS:
*   unitSize: This is the size of the units that will be allocated. A unit is
*             the memory region returned by MemoryPool_Allocate
*   maximumUnits: This is the maximum number of units that the pool will support.
*******************************************************************************/
#define MEMORY_POOL_SIZE(unitSize,maximumUnits) \
	(MEMORY_ALIGNED_SIZE(sizeof(MEMORY_POOL))+	\
		(maximumUnits*(MEMORY_ALIGNED_SIZE(sizeof(void *))+MEMORY_ALIGNED_SIZE(unitSize))))

/*******************************************************************************
* FUNCTION TYPE: MEMORY_POOL_ELEMENT_INITIALIZER
* PURPOSE: a function of this type can be provided to the 
*    MemoryPool_Initialize function in order to preinitialize each element.
*    That means that this function will be called for each element
*    during the call to MemoryPool_Initialize. This function will not
*    be called every time a memory unit is allocated with MemoryPool_Allocate
* NOTE: This feature is deprecated and may not be supported in future
*    versions of the SMSC network stack. There for it is recommended to
*    use NULL as the initializer to the MemoryPool_Initialize function.
*******************************************************************************/
typedef void (*MEMORY_POOL_ELEMENT_INITIALIZER)(void * element);

/*******************************************************************************
* FUNCTION: MemoryPool_Initialize
* DESCRIPTION: Initializes a memory region to be used as a memory pool
* PARAMETERS: 
*   memory: this is the memory region that has been preallocated. It may
*       be located in global memory space or allocated off the heap.
*       The macro MEMORY_POOL_SIZE should be used to determine 
*       how much memory to allocate.
*   unitSize: This is the size of the units that will be allocated. A unit is 
*       the memory region returned by MemoryPool_Allocate
*   maximumUnits: This is the maximum number of units that the pool supports
*   initializer: This is a function pointer used for initializing the units
*       during this call. This initializer is not used on each call to 
*       MemoryPool_Allocate.
*       NOTE: this initializer feature is deprecated and may not be supported
*             in future versions of the SMSC network stack. There for it is
*             recommended to use NULL as the initializer here.
* RETURN VALUE:
*   returns a MEMORY_POOL_HANDLE which can be used by 
*       MemoryPool_Allocate, and MemoryPool_Free
*******************************************************************************/
MEMORY_POOL_HANDLE MemoryPool_Initialize(
	void * memory,int unitSize, int maximumUnits,
	MEMORY_POOL_ELEMENT_INITIALIZER initializer);

/*******************************************************************************
* FUNCTION: MemoryPool_Allocate
* DESCRIPTION: Allocates a unit from a memory pool
* PARAMETERS:
*   memory_pool: This is a handle of the memory pool to allocate from.
*       It was previously returned from MemoryPool_Initialize
* RETURN_VALUE:
*   returns a pointer to a memory unit, the size of which was specifed when 
*   calling MemoryPool_Initialize. If no more units are available then NULL 
*   will be returned
*******************************************************************************/
void * MemoryPool_Allocate(MEMORY_POOL_HANDLE memory_pool);

/*******************************************************************************
* FUNCTION: MemoryPool_Free
* DESCRIPTION: Returns a memory unit back to the memory pool so it may
*     be used again by MemoryPool_Allocate
* PARAMETERS:
*   memory_pool: This is a handle of the memory pool to return the unit to.
*         It was previously returned from MemoryPool_Initialize.
*   unit: This is a memory unit that was previously return from 
*         MemoryPool_Allocate
*******************************************************************************/
void MemoryPool_Free(MEMORY_POOL_HANDLE memory_pool, void * unit);
void Tcpip_Memory_test(void);
int Memory_Block_Test(MEMORY_POOL_HANDLE memory_pool_handle);

#endif /* MEMPOOL_H  */
