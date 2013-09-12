#ifndef MEMORYMANAGEMENT_INC
#define MEMORYMANAGEMENT_INC
 
#include "STF/Interface/Types/STFBasicTypes.h"

enum POOL_TYPE
	{
	NonPagedPool, PagedPool
	};



/* Interface is specified in the header file itself ...
 
void * operator new (uint32 nSize, POOL_TYPE iType);
void * operator new (uint32 nSize);

void operator delete(void * p);
void operator delete(void * p, POOL_TYPE iType);


void * operator new[](uint32 nSize);
void * operator new[](uint32 nSize, POOL_TYPE iType);

void operator delete[](void * p);
void operator delete[](void * p, POOL_TYPE iType);
*/


#include "OSSTFMemoryManagement.h"


#endif // MEMORYMANAGEMENT_INC
