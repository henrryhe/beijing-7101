//
// OPERATING SYSTEM : OS20 and OS21
// 


//
// WARNING : THIS FILE IS NOT TESTED, Dez 9, 2002 !
//

//
// WARNING : NOT FOR 5505 !
//

#ifndef OSSTFMEMORYMANAGEMENT_INC
#define OSSTFMEMORYMANAGEMENT_INC

#include "STF/Interface/Types/STFBasicTypes.h"
#include "STF/Interface/STFMemoryManagement.h"

// Inlcudes for OS21 and OS20
#ifdef OSAL_OS21
#include <os21/partition.h>
#elif OSAL_OS20
#include <partitio.h>
#else
#error OS not supported or OSAL_XXXX not defined !
#endif

// Externally defined variable that holds the partition used for memory allocation
// This variable must be initialized before using any of these functions is used
extern partition_t* system_partition;

inline void * operator new (unsigned int nSize, POOL_TYPE iType)
	{
	return memory_allocate (system_partition, (nSize+3) & ~3);
	}

inline void * operator new (unsigned int nSize)
	{
	return memory_allocate (system_partition, (nSize+3) & ~3);
	}

inline void * operator new[](unsigned int nSize, POOL_TYPE iType)
	{
	return memory_allocate (system_partition, (nSize+3) & ~3);
	}

inline void * operator new[](unsigned int nSize)
	{
	return memory_allocate (system_partition, (nSize+3) & ~3);
	}



inline void operator delete(void *p)
	{
	if (p) memory_deallocate (system_partition, p);
	}

inline void operator delete(void *p, POOL_TYPE iType)
	{
	if (p) memory_deallocate (system_partition, p);
	}

inline void operator delete[](void *p)
	{
	if (p) memory_deallocate (system_partition, p);
	}

inline void operator delete[](void *p, POOL_TYPE iType)
	{
	if (p) memory_deallocate (system_partition, p);
	}

#endif // OSSTFMEMORYMANAGEMENT_INC
