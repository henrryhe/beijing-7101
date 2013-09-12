/*! Time-stamp: <@(#)OS21WrapperMemory.c   5/11/2005 - 8:41:34 AM   William Hennebois>
 *********************************************************************
 *  @file   : OS21WrapperMemory.c
 *
 *  Project : 
 *
 *  Package : 
 *
 *  Company : TeamLog 
 *
 *  Author  : William Hennebois            Date: 5/11/2005
 *
 *  Purpose :  Implementation of methods for memory management
 *
 *********************************************************************
 * Version History:
 *
 * V 0.10  5/11/2005  WH : First Revision
 * V 0.20  5/26/2005  WH : Add Memory Partition Management
 * V 0.30  8/1/2005  WH : Modify Mangement to Alloc aligned block
 * V 0.30  8/1/2005  WH : Adding Sharable  memory
 * V 0.30  8/1/2005  WH : Change default alignement to 4
 * V 0.40  8/11/2005 WH : Somme Change in presentation and remove none aligned mode   
 *********************************************************************
 */
#include <dbgapi.h>  // otherwise pkfuncs.h won't compile
#include <pkfuncs.h> // VirtualCopy
#include "OS21WrapperMemory.h"


#define USE_CEPARTITION


#define VIRTUAL_MEMORY_PAGE_SIZE 0x1000 // 4 KB in WinCE 5.0/SH4
#define PHYSICAL_PAGE_SIZE       0x0100 // 256 bytes (for VirtualCopy)

#define _32MB	0x02000000

// default values, you should use WCE_SetSharableMemoryPartition to modify the Sharable Partition 
// 
#define SIZE_WINCESHARABLE_PARTITION_INIT				(0)	 // size of the NoCache Partition
#define BASE_ADRESS_WINCESHARABLE_PARTITION_INIT		(0)							 // Base adress of the no cache partition see config.bif

// Activate the use of Partion management 
// otherwise, All Allocation use the std Malloc
 

// Use this define to have a link to the partition handle
// it is usefull to convert Virtual Pointer to physique memory
// this option Alloc 4 bytes of header for each malloc
#define LINKED_TO_PARTITION
#define OAL_MEMORY_CACHE_BIT            0x20000000
#define DEFAULT_ALIGN					4



//---------------------------------------------------------------------
//! Internal struct that starts all MCB ( memory conrol Bloc)
typedef struct __WCE_MemoryHeader
{
	struct __WCE_MemoryHeader	  *m_pNext;
	unsigned int				  m_dwFree;
	unsigned int		  		  m_dwOccuped;
#ifdef LINKED_TO_PARTITION
	struct __WCE_Partition		  *m_pPartition;
#endif
	
}_WCE_MemoryHeader;

//---------------------------------------------------------------------
//! struct used to Get Information
typedef struct __WCE_MemoryInfo
{
	unsigned int   sizeOccuped;
	unsigned int   sizeFree;
	unsigned int   AllocMax;
	unsigned int   NumFrag;
}_WCE_MemoryInfo;

//---------------------------------------------------------------------
//
//! The Real handle of the Partition  
typedef struct  __WCE_Partition
{
	_WCE_MemoryHeader *	m_pBaseMalloc;			// Block's base adress Static
	void			  * m_pPhysMem;
	void			  * m_pVirtualPhysMem;
	CRITICAL_SECTION    m_cMemoryCr;				// critical section for malloc
	struct  __WCE_Partition *pNextPartition;
}_WCE_Partition;


//---------------------------------------------------------------------
// Private 

static 	_WCE_Partition  *gSharablePartition;
static  void            *gBaseAdressSharable = (void *)BASE_ADRESS_WINCESHARABLE_PARTITION_INIT;
static  unsigned long    gSizeSharable       = SIZE_WINCESHARABLE_PARTITION_INIT;
static void _WCE_PrintMemoryFragement(_WCE_Partition *pHandle );
//---------------------------------------------------------------------
// public 
BOOL bWCEKernelMode;
//---------------------------------------------------------------------

static _WCE_Partition *gListPartition=0;


/*!-------------------------------------------------------------------------------------
 * Nexted list Add
 *
 * @param *pPartition : 
 *
 * @return static  : 
 */
static void _AddPartitionList(_WCE_Partition *pPartition)
{
    pPartition->pNextPartition = gListPartition;
    gListPartition = pPartition;

}


/*!-------------------------------------------------------------------------------------
 * Nexted list Remove 
 *
 * @param *pPartition : 
 *
 * @return static  : 
 */
static void _RemovePartitionList(_WCE_Partition *pPartition)
{
    _WCE_Partition* parent = NULL;

    if (gListPartition == pPartition) // remove head
        gListPartition = pPartition->pNextPartition;
    else // find parent and remove it
    {
        for (parent = gListPartition; parent != NULL; parent = parent->pNextPartition)
            if (parent->pNextPartition == pPartition)
            {
                parent->pNextPartition = pPartition->pNextPartition;
                break;
            }
        WCE_ASSERT(parent != NULL); // should have found it
    }
}


/*!-------------------------------------------------------------------------------------
 *! Align the size to a modulo radix 
 *
 * @param size : 
 * @param radix : 8 16 etc...
 *
 * @return static int  : 
 */
static int Align(int size, int radix)
{
	return (size + radix-1) & ~(radix-1);


}

/*!-------------------------------------------------------------------------------------
 * 
 *
 * @param *pAdress : Base of the partition , the adress should be uncached
 * @param iSize :  size of the partition
 *
 * Should be called before _WCE_MemoryInitilize
 * @return void  : 
 */
void WCE_SetSharableMemoryPartition(void *pAdress,unsigned long iSize)
{
	gBaseAdressSharable  = pAdress;
	gSizeSharable  = iSize;
}





/*!-------------------------------------------------------------------------------------
 *! returns the Malloc pointer ( used by the caller) from a MCB
 *
 * @param *pBlock : 
 *
 * @return inline void  : 
 */
static void *GetMallocPointer(_WCE_MemoryHeader *pBlock) 
{ 
	return (void *)(&pBlock[1]);
}


/*!-------------------------------------------------------------------------------------
 *! returns the MCB Pointer from A Malloc pointer
 *
 * @param *pBlock : 
 *
 * @return  : 
 */
static _WCE_MemoryHeader	 *GetHeaderPointer(void  *pBlock) 
{ 
	return ((_WCE_MemoryHeader*)pBlock) - 1;
}



/*!-------------------------------------------------------------------------------------
 *! returns the MCB Pointer from A Malloc pointer
 *
 * @param *pBlock : 
 *
 * @return  : 
 */
static void 	 *GetEndPointer(_WCE_MemoryHeader  *pBlock) 
{ 
	return (void *)((char *)&pBlock[1] + pBlock->m_dwOccuped + pBlock->m_dwFree);
}




// -------------------------------------------------------------------------------------
/*! Init the memory pool
 *
 * \param *pBlock   
 * \param size   
 *
 * \return bool   
 */
static _WCE_Partition * _WCE_InitializeHeap(void *pBlock, unsigned int size)
{
	_WCE_Partition *pHandle;
	pHandle = malloc(sizeof(_WCE_Partition));
	WCE_ASSERT(pHandle);


	if(size < sizeof(_WCE_MemoryHeader)) return 0;

	InitializeCriticalSection(&pHandle->m_cMemoryCr);
	EnterCriticalSection(&pHandle->m_cMemoryCr);

	pHandle->m_pBaseMalloc = (_WCE_MemoryHeader*)pBlock;
	pHandle->m_pBaseMalloc->m_pNext = NULL;
	pHandle->m_pBaseMalloc->m_dwFree = size - sizeof(_WCE_MemoryHeader);
	pHandle->m_pBaseMalloc->m_dwOccuped = 0;
#ifdef LINKED_TO_PARTITION
	pHandle->m_pBaseMalloc->m_pPartition = pHandle;
#endif
	LeaveCriticalSection(&pHandle->m_cMemoryCr);
	_AddPartitionList(pHandle);
	return pHandle;

}

// -------------------------------------------------------------------------------------
/*! 
 *! Noting to do 
 *
 * \return void   
 */
static void _WCE_TerminateHeap(_WCE_Partition *pHandle)
{
	_RemovePartitionList(pHandle);
	DeleteCriticalSection(&pHandle->m_cMemoryCr);

}




/*!-------------------------------------------------------------------------------------
 * Compute the Malloc pointer and a newblock depending the alignement 
 *
 * @param pCandidate : 
 * @param **pNewMalloc : 
 * @param **pBlockNext : 
 *
 * @return static int  : 
 */
static int ComputeAlignedBlock(_WCE_MemoryHeader * pCandidate,int sizeBlk,int AlignType, void **pNewMalloc,_WCE_MemoryHeader **pBlockNext)
{
	int realSize ;
	unsigned char *pblk     ;
	int sizeBoundAlign      ;
	if(AlignType ==0) return sizeBlk; // for version not aligned

	// compute the pointer on Free Zone

	pblk				= ((char  *)&pCandidate[1]) + pCandidate->m_dwOccuped +sizeof(_WCE_MemoryHeader);
	realSize			=   0; 
	sizeBoundAlign      =   ((unsigned long )(pblk) &  (AlignType-1));

	// If move to the bound
	if(sizeBoundAlign)
	{
		realSize    += AlignType - sizeBoundAlign;
	}
	pblk += realSize;
	if(pNewMalloc)  *pNewMalloc = (((char *)pblk));
	if(pBlockNext)	*pBlockNext = (_WCE_MemoryHeader *)(((char *)pblk) - sizeof(_WCE_MemoryHeader));

	realSize+= sizeBlk+sizeof(_WCE_MemoryHeader);



	return realSize;
}



// -------------------------------------------------------------------------------------
/*! 
 *! Look for the best candidate in the Partition, save fragmentation 
 * \param size   
 *
 * \return _WCE_MemoryHeader *   the Ptr MCB
 */


// -----------------------------------------------------------------------------------
// | header 1  | Size occuped | Size Free | header 2  | Size occuped | Size Free | next header ...
// -----------------------------------------------------------------------------------

static _WCE_MemoryHeader * FindBestCandidate(_WCE_Partition *pHandle, unsigned int size,int AlignType)
{
	_WCE_MemoryHeader *pBestOne = NULL;
	_WCE_MemoryHeader *pCur = pHandle->m_pBaseMalloc;

	// compute the real size to allocate ( size + header of the block)
	unsigned int dwRequestedSize = size+sizeof(_WCE_MemoryHeader);

	while(pCur)
	{
		if(pCur->m_dwFree >= dwRequestedSize )
		{

			// Now we check if the pointer can be aligned with the desired value 
			// this compute the real size, == size + alignement offset
			int realSize = ComputeAlignedBlock(pCur,size,AlignType,0,0);
			if(pCur->m_dwFree  > realSize)
			{
				if(pBestOne)
				{
					// Always, we prefer the smaller block
					if(pBestOne->m_dwFree > pCur->m_dwFree)
					{
							pBestOne =pCur;
						
					}

				}
				else
				{
					pBestOne =pCur;
				}
			}

		}
		pCur = pCur->m_pNext;
	}



	return pBestOne;
}



// -------------------------------------------------------------------------------------
/*! 
 *! Returns the privious MCB
 * \param *pblock   
 *
 * \return _WCE_MemoryHeader   
 */
static _WCE_MemoryHeader *GetPrevHeader(_WCE_Partition *pHandle, _WCE_MemoryHeader *pblock)
{
	_WCE_MemoryHeader *pCur;
	// can't free first block
	if(pblock == pHandle->m_pBaseMalloc ) return NULL;

	pCur = pHandle->m_pBaseMalloc;


	while(pCur)
	{
		if(pCur->m_pNext == pblock) break;
		pCur = pCur->m_pNext;
	}
	return pCur;

}






// -------------------------------------------------------------------------------------
/*! 
 *! Standard Allocation in the Partition
 * \param size   

 *
 * \return void *   
 */


static void * _WCE_Malloc(_WCE_Partition *pHandle, unsigned int sizeMalloc,int typeAlign)
{
	_WCE_MemoryHeader *pCandidate;
	int realSize;
	int AlignType;
	unsigned char *pMallocPtr;
	unsigned char *pNewMalloc;
	unsigned char *pEndBlk;
	_WCE_MemoryHeader *pBlockNext;
	unsigned int sizeAligned;

	AlignType  = DEFAULT_ALIGN;
	if(typeAlign) AlignType = typeAlign;

	// Align to even
	sizeAligned = Align(sizeMalloc,4);

	// Look for a free block able to alloc sizeAligned
	pCandidate = FindBestCandidate(pHandle,sizeAligned,AlignType);
	if(!pCandidate) 	return NULL;

	EnterCriticalSection(&pHandle->m_cMemoryCr);


	// Compute the new block pointers 
	ComputeAlignedBlock(pCandidate,sizeAligned,AlignType, &pNewMalloc,&pBlockNext);


	// RAZ
	memset(pBlockNext,0,sizeof(_WCE_MemoryHeader));

	pEndBlk = GetEndPointer(pCandidate);


	// Link the candidate to next block
	pBlockNext->m_pNext =	pCandidate->m_pNext;

	// Correct the next block of the candidate, it points now on the new block created
	pCandidate->m_pNext   =   pBlockNext;

	// A new block has always an occuped size sets to the size requested and freesize to 0
	// because we alloc always from the end of the freeblock
	pBlockNext->m_dwOccuped = sizeAligned;

	// we recompute the Size free of the candidate, by the difference of pointers
	// probably heated by alignement
	
	pCandidate->m_dwFree =  (char *)pBlockNext - (((char *)&pCandidate[1]) + pCandidate->m_dwOccuped);

	// the we recompute the free space
	pBlockNext->m_dwFree =  pEndBlk - (((char *)&pBlockNext[1]) + pBlockNext->m_dwOccuped);

//	_WCE_PrintMemoryFragement(pHandle);




#ifdef LINKED_TO_PARTITION
	pHandle->m_pBaseMalloc->m_pPartition = pHandle;
#endif
	LeaveCriticalSection(&pHandle->m_cMemoryCr);



	return pNewMalloc;



}




// -------------------------------------------------------------------------------------
/*! 
 * Standard Free in the Partition
 * \param *pBlock   
 *
 * \return void   
 */
static void _WCE_Free(_WCE_Partition *pHandle, void *pBlock)
{
	_WCE_MemoryHeader *pNext;
	_WCE_MemoryHeader *pPrev;

	pNext = GetHeaderPointer(pBlock);
	pPrev = GetPrevHeader(pHandle,pNext);

	if(pPrev == NULL) 
	{
		WCE_MSG(MDL_MSG,"Free Corrupted");
		return;
	}
	EnterCriticalSection(&pHandle->m_cMemoryCr);
	// unlink block
	pPrev->m_pNext = pNext->m_pNext;
	pPrev->m_dwFree+= pNext->m_dwOccuped + pNext->m_dwFree + sizeof(_WCE_MemoryHeader);
	LeaveCriticalSection(&pHandle->m_cMemoryCr);
	
	
}



/*!-------------------------------------------------------------------------------------
 *! Standard Re-Allocation in the Partition
 *
 * @param *pHandle : 
 * @param *pBlock : 
 * @param sizeMalloc : 
 *
 * @return static void  : 
 */
static void * _WCE_ReAlloc(_WCE_Partition *pHandle, void *pBlock, unsigned int sizeMalloc)
{
	unsigned int sizeAligned;
	unsigned int dwSizeBlock;
	_WCE_MemoryHeader *pCandidate;
	if(pBlock == NULL)  return _WCE_Malloc(pHandle,sizeMalloc,0);
	
	sizeAligned = Align(sizeMalloc,DEFAULT_ALIGN);

	pCandidate = GetHeaderPointer(pBlock);
	
	dwSizeBlock = pCandidate->m_dwOccuped + pCandidate->m_dwFree ;
	
	if(dwSizeBlock > sizeAligned)
	{
		EnterCriticalSection(&pHandle->m_cMemoryCr);
		// Recup the block 	
		pCandidate->m_dwOccuped = sizeAligned;
		pCandidate->m_dwFree = dwSizeBlock -sizeAligned;
		LeaveCriticalSection(&pHandle->m_cMemoryCr);

		return GetMallocPointer(pCandidate);
	}
	else
	{
		// else free the block & realloc
		// we assume that the free is done first
		
			_WCE_Free(pHandle,pBlock);
			return _WCE_Malloc(pHandle,sizeMalloc,0);
	}
	
	

}




/*!-------------------------------------------------------------------------------------
 * Returns the Raw Size occuped in the partition
 *
 * @param *pHandle : 
 *
 * @return static unsigned  : 
 */
static unsigned int  _WCE_GetCoreSizeOccuped(_WCE_Partition *pHandle )
{

	_WCE_MemoryHeader *pCur = pHandle->m_pBaseMalloc;
	unsigned int SizeBlk=0;



	while(pCur)
	{
		SizeBlk += pCur->m_dwOccuped;
		pCur = pCur->m_pNext;
	}
	return SizeBlk;

}


/*!-------------------------------------------------------------------------------------
 * Returns the size Free un the Partition
 *
 * @param *pHandle : 
 *
 * @return static unsigned  : 
 */
static unsigned int  _WCE_GetCoreSizeFree(_WCE_Partition *pHandle )
{

	_WCE_MemoryHeader *pCur = pHandle->m_pBaseMalloc;
	unsigned int SizeBlk=0;



	while(pCur)
	{
		SizeBlk += pCur->m_dwFree;
		pCur = pCur->m_pNext;
	}
	return SizeBlk;

}


/*!-------------------------------------------------------------------------------------
 * Returns some information about the state of the partition
 *
 * @param *pHandle : 
 * @param info : 
 *
 * @return static void  : 
 */
static void  _WCE_GetInfo(_WCE_Partition *pHandle,_WCE_MemoryInfo * info )
{
	_WCE_MemoryHeader *pCur;
	unsigned int SizeBlk=0;

	info->sizeOccuped = 0;
	info->sizeFree = 0;
	info->AllocMax = 0;
	info->NumFrag = 0;
	
	pCur = pHandle->m_pBaseMalloc;
	while(pCur)
	{
		info->sizeOccuped += pCur->m_dwOccuped;
		info->sizeFree += pCur->m_dwFree;
		if(info->AllocMax < pCur->m_dwFree)
		{
			info->AllocMax = pCur->m_dwFree;
		}
		
		info->NumFrag++;
		pCur = pCur->m_pNext;
	}
	
	// don't forget the size of the header
	info->AllocMax = 	info->AllocMax  > sizeof(_WCE_MemoryHeader) ? info->AllocMax  - sizeof(_WCE_MemoryHeader) : 0;

}


// -------------------------------------------------------------------------------------
/*! Prints debug info
 *
 * \param *pblock   
 *
 * \return _WCE_MemoryHeader   
 */
static void _WCE_PrintMemoryFragement(_WCE_Partition *pHandle )
{


	_WCE_MemoryInfo  info;
	unsigned int cptBlk=0;
	unsigned int size  = 0;
	_WCE_MemoryHeader *pCur = pHandle->m_pBaseMalloc;
	printf("-------------------- PrintMemoryFragement -------------------------------");


	while(pCur)
	{

		printf("%03d: px ! %05x, szOcc %05d szFree %05d",cptBlk++,&pCur[1],pCur->m_dwOccuped,pCur->m_dwFree);
		size += sizeof(_WCE_MemoryHeader) ;
		size += pCur->m_dwOccuped;
		size += pCur->m_dwFree;

		pCur = pCur->m_pNext;
	}


	_WCE_GetInfo(pHandle,&info);
	printf("Size Total = %05d",size);
	printf("Core Size Free %d",info.sizeFree);
	printf("Core Size Occuped %d",info.sizeOccuped);
	printf("Max Alloc %d",info.AllocMax);
	printf("Num Frag  %d",info.NumFrag);



}



/*!-------------------------------------------------------------------------------------
 * memory_allocate_clear() allocates a block of memory large enough for an
   array of nelem elements, each of size elsize bytes, from partition part. It returns
   the base address of the array, which is suitably aligned to contain any type. The
   memory is initialized to zero.
   Note: If a null pointer is specified for part, instead of a valid partition pointer, the C
   runtime heap is used.
   This function calls the memory allocator associated with the partition part. For a
   full description of the algorithm, see the description of the appropriate partition
    creation function.
 *
 * @param pp : 
 * @param nelem : 
 * @param elsize : 
 *
 * @return void  : 
 */
void *memory_allocate_clear(partition_t * pp, size_t nelem, int elsize)
{
#ifdef USE_CEPARTITION
	if(pp)
	{
		void *pMem;
		WCE_ASSERT(pp);

		pMem = _WCE_Malloc((_WCE_Partition *)pp,nelem*elsize,0);
		memset(pMem,0,nelem*elsize);
		return pMem;
	}
	else
	{
		return calloc(nelem,elsize);

	}

#else
	return calloc(nelem,elsize);
#endif
}


/*!-------------------------------------------------------------------------------------
 * memory_allocate() allocates a block of memory of size bytes from partition part.
   It returns the address of a block of memory of the required size, which is suitably
   aligned to contain any type.
   Note: If a null pointer is specified for part, instead of a valid partition pointer, the C
   runtime heap is used.
 *
 * @param pp : partition
 * @param size : size
 *
 * @return void *memory_allocate  : 
 */
void *memory_allocate (partition_t * pp, int size)
{
#ifdef USE_CEPARTITION
	if(pp)
	{
		void *pMem;
		pMem = _WCE_Malloc((_WCE_Partition *)pp,size,0);
		return pMem;
	}
	else
	{
		return malloc(size);
	}

#else
	return malloc(size);
#endif
}

/*!-------------------------------------------------------------------------------------
 * memory_deallocate() returns a block of memory at block, back to partition
*  part. The memory must have been originally allocated from the same partition to
*  which it is being freed.
*  Note: If a null pointer is specified for part, instead of a valid partition pointer, the C
*   runtime heap is used.
 *
 * @param pp : partition
 * @param ptr : size
 *
 * @return void   : 
 */
void  memory_deallocate(partition_t * pp, void * ptr)
{
#ifdef USE_CEPARTITION
	if(pp)
	{

		_WCE_Free((_WCE_Partition *)pp,ptr);

	}
	else
	{
		free(ptr);
	}
#else
	free(ptr);
#endif
}


/*!-------------------------------------------------------------------------------------
 * memory_reallocate() changes the size of a memory block allocated from a
 * partition, preserving the current contents.
 * If block is NULL, then the function behaves like memory_allocate and allocates a
 * block of memory. If size is 0 and block is not NULL, then the function behaves like
 * memory_deallocate() and frees the block of memory back to the partition.
 * For fixed and heap partitions, if block is not NULL and size is not 0 then the block
 * of memory is reallocated.
 * Note: block must have been allocated from part originally.
 * Note: If a null pointer is specified for part, instead of a valid partition pointer, the C
 * runtime heap is used.
 *
 * @param pp : partition
 * @param ptr : malloc ptr
 * @param size : size
 *
 * @return void * : 
 */
void *memory_reallocate(partition_t * pp, void * ptr, int size)
{
#ifdef USE_CEPARTITION
	if(pp)
	{
		void *pMem;
		pMem = _WCE_ReAlloc((_WCE_Partition *)pp,ptr,size);
		return pMem;

	}
	else
	{
		return realloc(ptr, size);
	}
#else

	return realloc(ptr, size);
#endif
}


/*!-------------------------------------------------------------------------------------
 * partition_create_heap() creates a memory partition with the semantics of a
 * heap. This means that variable size blocks of memory can be allocated and freed
 * back to the memory partition. Only the amount of memory requested is allocated,
 * with a small overhead on each block for the partition manager. Allocating and
 * freeing requires searching through lists, and so the length of time depends on the
 * current state of the heap.50 Memory and partition function definitions
 * PRELIMINARY DATA
 * STMicroelectronics
 * OS21 User Manual ADCS 7358306K
 * Memory is allocated and freed back to this partition using memory_allocate()
 * and memory_deallocate(). memory_reallocate() is implemented efficiently.
 * Reducing the size of a block is always done without copying, and expanding only
 * results in a copy if the block cannot be expanded because subsequent memory
 * locations have been allocated.
 *
 * @param ExternalBlock : ptr bloc or NULL of the bloc is allocated in shared memory
 * @param size : size
 *
 * @return void *  : 
 */
partition_t * partition_create_heap(void * ExternalBlock, unsigned int size)
{
#ifdef USE_CEPARTITION
	{
	void *pMem;
	if(ExternalBlock) pMem = (partition_t *)_WCE_InitializeHeap(ExternalBlock,size);
	else			  pMem = (partition_t *)partition_create_heap_NoCacheCE(size);		
	return pMem;
	}
#else

  return (void *)-1; // WHE Fail if test for null
#endif
}

/*!-------------------------------------------------------------------------------------
 * partition_delete_heap() deletes a memory partition created with partition_create_heap()
 */
void partition_delete_heap(partition_t* p)
{
#ifdef USE_CEPARTITION
	{
	void *pMem;
	_WCE_Partition * pHandle =(_WCE_Partition*)p;
    _WCE_TerminateHeap(pHandle);
	}
#else
  return; // JLX: nothing to do
#endif
}

/* *********** Access to Physical Memory (chip registers) ***************** */ 

//! MapPhysicalToVirtual()
//! Maps <size> bytes of uncached virtual memory to <physicalAddress>
LPVOID MapPhysicalToVirtual(LPVOID lpPhysicalAddress, DWORD dwSize)
{
    UINT8* lpResult = NULL;
    LPVOID lpPA_256;
    DWORD  offsetInVirtualPage;  // from beginning of virtual memory page
    DWORD  offsetInPhysicalPage; // from beginning of physical page
    DWORD  virtualSize = dwSize;
	
	UINT8* sectionlpResult = NULL;
	DWORD  section32M, i;
	DWORD Error;

#ifndef ST_WINCE_USER_MODE
	return lpPhysicalAddress;
#endif
    // Compute virtual page offset and the needed size of virtual memory

    offsetInVirtualPage = ((DWORD)lpPhysicalAddress) % VIRTUAL_MEMORY_PAGE_SIZE;
    virtualSize += offsetInVirtualPage; // this virtual memory is not used, but must be allocated yet

    if (offsetInVirtualPage + (dwSize % VIRTUAL_MEMORY_PAGE_SIZE) > VIRTUAL_MEMORY_PAGE_SIZE)
        virtualSize += VIRTUAL_MEMORY_PAGE_SIZE; // one more virtual page

    // Allocate virtual memory, without committing any physical memory

    lpResult = VirtualAlloc(NULL, virtualSize, MEM_RESERVE, PAGE_NOACCESS);
    WCE_ASSERT(lpResult != NULL);
    WCE_ASSERT((((DWORD)lpResult) % VIRTUAL_MEMORY_PAGE_SIZE) == 0); // just to be sure !
    if (lpResult == NULL)
        return NULL;

    // Offsets lpResult to the beginning of the physical page

    offsetInPhysicalPage = ((DWORD)lpPhysicalAddress) % PHYSICAL_PAGE_SIZE;
    lpResult += offsetInVirtualPage - offsetInPhysicalPage;

    // Maps the virtual memory - Handle 32MB crossing boundary
	section32M = (dwSize + offsetInPhysicalPage)/_32MB;

	lpPA_256 = (LPVOID)(((DWORD)lpPhysicalAddress) / PHYSICAL_PAGE_SIZE);
	
	// Copy lpResult into working variable.
	sectionlpResult = lpResult;
	
	// process if full 32MB sections
	for (i = 0; i < section32M; i++)
	{
		if (!VirtualCopy(sectionlpResult, lpPA_256, _32MB,
                     PAGE_READWRITE | PAGE_NOCACHE | PAGE_PHYSICAL))
		{
			Error = GetLastError();
			WCE_VERIFY(0);
			WCE_ERROR("MapPhysicalToVirtual: unable to do VirtualCopy");
			VirtualFree(sectionlpResult, 0, MEM_RELEASE);
			return NULL;
		}

		sectionlpResult += _32MB;
		(DWORD)lpPA_256 += (_32MB/PHYSICAL_PAGE_SIZE);
	}

	// 
	dwSize = (dwSize + offsetInPhysicalPage) % _32MB;

	if (dwSize)
	{
		if (!VirtualCopy(sectionlpResult, lpPA_256, dwSize,
							 PAGE_READWRITE | PAGE_NOCACHE | PAGE_PHYSICAL))
		{
			Error = GetLastError();
			WCE_VERIFY(0);
			WCE_ERROR("MapPhysicalToVirtual: unable to do VirtualCopy ");
			VirtualFree(sectionlpResult, 0, MEM_RELEASE);
			return NULL;
		}
	}

    // Offsets lpResult to the specified physical address
    lpResult += offsetInPhysicalPage;
    return lpResult;
}

//! UnmapPhysicalToVirtual()
//! Unmaps & free virtual memory returned by MapPhysicalToVirtual()
void UnmapPhysicalToVirtual(LPVOID lpVirtualAddress)
{
    DWORD offsetInPage; // from beginning of memory page

#ifndef ST_WINCE_USER_MODE
	return;
#endif

    // compute offset
    offsetInPage = ((DWORD)lpVirtualAddress) % VIRTUAL_MEMORY_PAGE_SIZE;

    // tweak address back to page start
    lpVirtualAddress = (char*)lpVirtualAddress - offsetInPage;

    // free virtual memory
    if (!VirtualFree(lpVirtualAddress, 0, MEM_RELEASE))
        WCE_ERROR("UnmapPhysicalToVirtual: unable to do VirtualFree");
}







/*!-------------------------------------------------------------------------------------
 * This function purges any addresses that are in the D-cache and correspond
 * to the given address range. If the range specified is big enough we defer
 * to _cache_range_work(), which is more efficient for big ranges.
 *
 * @param base_address : 
 * @param length : 
 *
 * @return void cache_purge_data  : 
 */
#pragma optimize( "", off)

void cache_purge_data (void * base_address, unsigned int length)
{
	cache_flush_data (base_address, length);
	cache_invalidate_data (base_address, length);
/*
	// not supported fir WINCE, you should use StaticMapping  or Alloc in the WinCE Share section
	// to get NonCache or Physical mem
	WCE_MSG(MDL_WARNING,"Call cache_purge_data %X %d);",base_address,length);
*/
}

void cache_flush_data (void * base_address, unsigned int length)
{
	if (length)
	{
		__asm ( "mov\t#31,r0\n" 
			"\tand\tr5,r0\n" 
			"\tadd\tr0,r4\n" 
			"\tadd\t#31,r4\n"
			"\tshlr2\tr4\n" 
			"\tshlr2\tr4\n" 
			"\tshlr\tr4\n" 
			"\tmov\t#32,r0\n" 
			"label1:\tocbp\t@r5\n" 
			"\tdt\tr4\n" 
			"\tbf/s\tlabel1\n"
			"\tadd\tr0,r5\n"
			"\tnop\n"
			"\tnop\n"
			"\tnop\n"
			"\tnop\n"
			"\tnop\n",
			length,base_address		
			);
	}
/*
	// not supported fir WINCE, you should use StaticMapping  or Alloc in the WinCE Share section
	// to get NonCache or Physical mem
	WCE_MSG(MDL_WARNING,"Call cache_flush_data %X %d);",base_address,length);
*/
}

void cache_invalidate_data (void * base_address, unsigned int length)
{
	__asm ( 
			 "\tmov #31, r0\n"
			"\tand r4, r0\n"
			"\tadd r0, r5\n"
			"\tadd #31, r5\n"
			"\tmov #-5, r0\n"
			"\tshld r0, r5\n"
			"label1:\tocbi @r4\n"
			"\tdt r5\n"
			"\tbf/s label1\n"
			"\tadd #32, r4\n"
			"\tnop\n",		
			base_address,length		
			);
/*
	// not supported fir WINCE, you should use StaticMapping  or Alloc in the WinCE Share section
	// to get NonCache or Physical mem
	WCE_MSG(MDL_WARNING,"Call cache_invalidate_data %X %d);",base_address,length);
*/
}
#pragma optimize( "", on)

/*!-------------------------------------------------------------------------------------
 * Return some status information about a given
 * partition.
 *
 * @param pp : 
 * @param status : 
 * @param flags : 
 *
 * @return int partition_status  : 
 */
int partition_status (partition_t * pp, partition_status_t * status, partition_status_flags_t flags)
{
  _WCE_MemoryInfo info;
  // Solution to get same information for the heap
  WCE_ASSERT(pp);


  WCE_MSG(MDL_WARNING,"Call partition_status %X %X %X);",pp,status,flags);
  _WCE_GetInfo((_WCE_Partition*)pp,&info );
  memset(status,0,sizeof(partition_status_t)); 
  status->partition_status_free_largest = info.AllocMax;
  status->partition_status_free         = info.sizeFree;
  status->partition_status_used			= info.sizeOccuped;

  return OS21_SUCCESS;
}


/*!-------------------------------------------------------------------------------------
 * A special case of partition able to convert an adress into NonCache cached and physique
 * the conversion is from an  allocated pointer
 * note: We are lucky because there is the same translation betwen Cache and uncached between 
 * OS21 and CE ( transation from 8000000 to A0000000
 * this is why, we won't re-implemente ST40_NOCACHE_NOTRANSLATE 
 * otherwisew we should use partition_translate_nocache_CE
 * 
 * @param size : g
 *
 * @return void *  : 
 */
partition_t * partition_create_heap_NoCacheCE(unsigned int size)
{



	if(gSharablePartition)
	{
		void *pBaseNoCache  = 0;
		// Some Bloody App Don't Call STBOOT before to Alloc NCache 
		// We need the Check the init of the Memory
		// _WCE_MemoryInitilize(); removed, see ST_OPTION
		
		// Then Alloc  first the block in Shared CE memory
		pBaseNoCache = _WCE_Malloc(gSharablePartition,size,0);
		if(!pBaseNoCache) return 0;
		// Then  Create a SubPartition  in the ShareCE
		return partition_create_heap(pBaseNoCache,size);
	}
	else
	{
		return 0;
	}

}




/*!-------------------------------------------------------------------------------------
 * Just init No Cache Partition
 *
 * @param none
 *
 * @return void  : 
 */
void _WCE_MemoryInitilize()
{

	PVOID pBlk;
	PVOID pUncached;
	PVOID pVirtualPhys;
	ULONG iPhysMem;
	_WCE_Partition *pPartition;
	if(gSharablePartition) return;

	// Check if we use Shared Wince Partition by the call of WCE_SetSharableMemoryPartition
	
	if(gBaseAdressSharable== 0 && gSizeSharable ==0)  return;

	if(gBaseAdressSharable ==0)
	{

		// Alloc PhysMem & Virtual in Uncached dynamicly 
		pVirtualPhys = (LPVOID)AllocPhysMem(gSizeSharable,PAGE_READWRITE|PAGE_NOCACHE,
												 0,    // Default alignment
												 0,    // Reserved
												 &iPhysMem);
		WCE_ASSERT(pVirtualPhys);
		iPhysMem = iPhysMem >>8;

		// Create Static memory in the uncached Area > 0XA0000000
	   	pUncached   = CreateStaticMapping(iPhysMem,gSizeSharable);
		// Translate uncached to Cached memory  0xA000000>p>80000000 toogle 0x20000000L

		// if you want an uncached block uncoment out next line

		//	pBlk  = (PVOID)(((ULONG)pUncached) & (~OAL_MEMORY_CACHE_BIT));
		pBlk   = pUncached;
	}
	else
	{
		pBlk = (PVOID)gBaseAdressSharable;
		pVirtualPhys = NULL;


	}
	// init partition with Cached memory
	pPartition = _WCE_InitializeHeap(pBlk,gSizeSharable);
	if(pPartition)
	{
		// Save pointer for disallocation
		pPartition->m_pBaseMalloc =pBlk;
		pPartition->m_pPhysMem    =pVirtualPhys;
	}
	gSharablePartition = pPartition;

}

/*!-------------------------------------------------------------------------------------
 * Just Term Critical Section
 *
 * @param none
 *
 * @return void  : 
 */
void _WCE_MemoryTerminate()
{
	_WCE_Partition *pHandle = (_WCE_Partition*)gSharablePartition;
	if(pHandle)
	{
		_WCE_TerminateHeap(pHandle);
			
		if(pHandle->m_pVirtualPhysMem) 
		{
			FreePhysMem(pHandle->m_pVirtualPhysMem);
		}
		if(gSharablePartition) VirtualFree(pHandle->m_pBaseMalloc, 0, MEM_RELEASE);
	}
	gSharablePartition = 0;
}



/*!-------------------------------------------------------------------------------------
 * Alloc Sharable memory for LX ou other device, the block returned is always uncached 
 *
 * @param size : 
 *
 * @return void *  : 
 */
void * _WCE_AllocSharableMemory(unsigned int size,int typeAlign)
{
	if(gSharablePartition)
	{

		return _WCE_Malloc(gSharablePartition,size,typeAlign);
	}
	else
	{
		return 0;
	}
}

/*!-------------------------------------------------------------------------------------
 * return the Kernel Mode
 * @return  : 
 * 0 : User
 * 1 : Full-Kernel
 */
void WCE_GetKernelMode(void)
{

	// SetKMode return current KMode
	bWCEKernelMode = SetKMode(TRUE);

	// back to current KMode
	SetKMode(bWCEKernelMode);

	// OLGN - Check for Kernel Mode
	if (bWCEKernelMode)
		printf ("STAPI RUN IN KERNEL-MODE\n");
	else
		printf ("STAPI RUN IN USER-MODE\n");

}

// -------------------------------------------------------------------------------------
/*! Prints debug info
 *
 * \param *pblock   
 *
 * \return _WCE_MemoryHeader   
 */
void _WCE_DisplayMemoryPartition(_WCE_Partition *pHandle, int num)
{

	_WCE_MemoryInfo  info;
	unsigned int cptBlk=0;
	unsigned int size  = 0;
	_WCE_MemoryHeader *pCur = pHandle->m_pBaseMalloc;
	_WCE_Printf("-------------------- Partition %d status -------------------------------\n", num);

	_WCE_GetInfo(pHandle,&info);
	_WCE_Printf("Base@:              0x%08x\n",pHandle->m_pBaseMalloc);
	_WCE_Printf("Num Frag:           %d\n",info.NumFrag);
	_WCE_Printf("Core Size Free:     %d (%d.%dMB)\n",info.sizeFree, info.sizeFree/(1024*1024), (info.sizeFree%(1024*1024))/1024);
	_WCE_Printf("Core Size Occuped:  %d (%d.%dMB)\n",info.sizeOccuped, info.sizeOccuped/(1024*1024), (info.sizeOccuped%(1024*1024))/1024);
	_WCE_Printf("Max Alloc:          %d (%d.%dMB)\n",info.AllocMax, info.AllocMax/(1024*1024), (info.AllocMax%(1024*1024))/1024);

	_WCE_Printf ("Partition Fragments > 256Ko :\n");
	while(pCur)
	{
		
		size += sizeof(_WCE_MemoryHeader);
		size += pCur->m_dwOccuped;
		size += pCur->m_dwFree;

		if (pCur->m_dwOccuped>(1024*256))
			_WCE_Printf("   %03d: px ! %05x, szOcc %05d szFree %05d\n",cptBlk++,&pCur[1],pCur->m_dwOccuped,pCur->m_dwFree);

		pCur = pCur->m_pNext;
	}

	_WCE_Printf("Total Size = %d (%d.%dMB)\n",size, size/(1024*1024), (size%(1024*1024))/1024);

}


void DisplayRAMPartitionStatus(void)
{
	partition_status_t status;
	_WCE_Partition  *gPartition;
	int num = 1;
	
	gPartition = gListPartition;

	while(gPartition)
	{
		_WCE_DisplayMemoryPartition(gPartition, num);
		gPartition = gPartition->pNextPartition;
		num++;
	}

}


