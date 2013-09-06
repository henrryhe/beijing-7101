/* Copyright (c) 2008 SMSC
 * All Rights Reserved.
 *
 * FILE: memory_pool.c
 */

#include "smsc_environment.h" 

#include "memory_pool.h"
#include "tcp.h"

#if SMSC_ERROR_ENABLED
#define MEMORY_POOL_SIGNATURE (0x3EE33D2C)
#endif
#ifdef MEMORY_POOL_CHANGE_090217
#include "smsc_threading.h"
static SMSC_SEMAPHORE pSemMemPoolLock = NULL;
extern MEMORY_POOL_HANDLE gEchoContextPool;
extern MEMORY_POOL_HANDLE gSmallBufferPool;
extern MEMORY_POOL_HANDLE gLargeBufferPool;
extern MEMORY_POOL_HANDLE gTcpControlBlockPool;
extern MEMORY_POOL_HANDLE gTcpSegmentPool;
extern MEMORY_POOL_HANDLE gUdpControlBlockPool;

#endif

MEMORY_POOL_HANDLE MemoryPool_Initialize(
	void * memory,int unitSize, int maximumUnits,
	MEMORY_POOL_ELEMENT_INITIALIZER initializer)
{
	PMEMORY_POOL memory_pool=(PMEMORY_POOL)MEMORY_ALIGN(memory);
	#ifdef MEMORY_POOL_CHANGE_090217
       int i;
	#endif
	void ** previousUnit=NULL;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(memory_pool!=NULL,NULL);
	memory_pool->mHeadUnit=NULL;
	if(maximumUnits>0) {
		/* Initialize unit list */
		int unitsLeft=maximumUnits;
		memory_pool->mHeadUnit=(void **)(((mem_ptr_t)memory_pool)+MEMORY_ALIGNED_SIZE(sizeof(MEMORY_POOL)));
		previousUnit=memory_pool->mHeadUnit;
		#ifdef MEMORY_POOL_CHANGE_090217
		for(i = 0;i < MEMORY_MAXUNIT_SIZE;i ++)
		{
                 memory_pool->MemUsed[i] = false;
		}
		memory_pool->i_Size = unitSize;
		memory_pool->i_MaximumUnits =  maximumUnits;
              #endif
		while(unitsLeft>1) {
			if(initializer!=NULL) {
				initializer((void *)(((mem_ptr_t)previousUnit)+
					((mem_ptr_t)((int)MEMORY_ALIGNED_SIZE(sizeof(void *))))));
			}
			(*previousUnit)=(void *)(((mem_ptr_t)previousUnit)+
					((mem_ptr_t)((int)MEMORY_ALIGNED_SIZE(sizeof(void *))))+
					((mem_ptr_t)((int)MEMORY_ALIGNED_SIZE(unitSize))));
			previousUnit=(void **)(*previousUnit);
			unitsLeft--;
		}
		(*previousUnit)=NULL;
		if(initializer!=NULL) {
			initializer((void *)(((mem_ptr_t)previousUnit)+
				((mem_ptr_t)((int)MEMORY_ALIGNED_SIZE(sizeof(void *))))));
		}
	}

	SMSC_ERROR_ONLY(
		memory_pool->mPoolEnd=(void *)(((mem_ptr_t)memory_pool)+((mem_ptr_t)((int)MEMORY_POOL_SIZE(unitSize,maximumUnits))));
	)
	ASSIGN_SIGNATURE(memory_pool,MEMORY_POOL_SIGNATURE);	
#ifdef MEMORY_POOL_CHANGE_090217	
	if(pSemMemPoolLock == NULL)
	{
		pSemMemPoolLock = smsc_semaphore_create(1);
	}
#endif
	return (MEMORY_POOL_HANDLE)memory_pool;
}

void * MemoryPool_Allocate(MEMORY_POOL_HANDLE memory_pool_handle)
{
	void * result=NULL;
#ifdef MEMORY_POOL_CHANGE_090217	
	void ** previousUnit=NULL;
	int        i_count;
#endif
	PMEMORY_POOL memory_pool=(PMEMORY_POOL)memory_pool_handle;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(memory_pool!=NULL,NULL);
	CHECK_SIGNATURE(memory_pool,MEMORY_POOL_SIGNATURE);
#ifdef MEMORY_POOL_CHANGE_090217
       if(memory_pool == NULL)
       	{
                      return NULL;
       	}
	smsc_semaphore_wait(pSemMemPoolLock);
	previousUnit = memory_pool->mHeadUnit;
	for(i_count = 0; i_count < memory_pool->i_MaximumUnits;i_count ++)
	{
		if(memory_pool->MemUsed[i_count] == false)
		{
			memory_pool->MemUsed[i_count]  = true;
			result=(void *)(((mem_ptr_t)previousUnit)+MEMORY_ALIGNED_SIZE(sizeof(void *)));	
			smsc_semaphore_signal(pSemMemPoolLock);
			return result;
		}
		/*得到下一个单元数据*/
		(*previousUnit)=(void *)(((mem_ptr_t)previousUnit)+
		((mem_ptr_t)((int)MEMORY_ALIGNED_SIZE(sizeof(void *))))+
		((mem_ptr_t)((int)MEMORY_ALIGNED_SIZE(memory_pool->i_Size))));

		previousUnit=(void **)(*previousUnit);
	}
	SMSC_ERROR(("TCPIP STACK DO NOT HAVE ENOUGH MEM!!! "));
	//Tcpip_Memory_test();
	smsc_semaphore_signal(pSemMemPoolLock);
#else
	if(memory_pool->mHeadUnit!=NULL) {
		result=(void *)(((mem_ptr_t)memory_pool->mHeadUnit)+MEMORY_ALIGNED_SIZE(sizeof(void *)));
		memory_pool->mHeadUnit=(void **)(*(memory_pool->mHeadUnit));
				
		/* make sure result belongs to this pool */
		SMSC_ASSERT((result>((void *)memory_pool))&&(result<(memory_pool->mPoolEnd)));
	}
#endif
	return result;
}

void MemoryPool_Free(MEMORY_POOL_HANDLE memory_pool_handle, void * unit)
{
	PMEMORY_POOL memory_pool=(PMEMORY_POOL)memory_pool_handle;
#ifdef MEMORY_POOL_CHANGE_090217	
	void ** previousUnit=NULL;
	void * result=NULL;
	int        i_count;
#endif	
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(memory_pool!=NULL);
	CHECK_SIGNATURE(memory_pool,MEMORY_POOL_SIGNATURE);	

	/* Make sure unit belongs to this pool */
	SMSC_ASSERT((unit>((void *)memory_pool))&&(unit<((void *)(memory_pool->mPoolEnd))));
#ifdef MEMORY_POOL_CHANGE_090217
       if(memory_pool == NULL)
       	{
                      return ;
       	}
	smsc_semaphore_wait(pSemMemPoolLock);
	previousUnit = memory_pool->mHeadUnit;
	for(i_count = 0; i_count < memory_pool->i_MaximumUnits;i_count ++)
	{
		result=(void *)(((mem_ptr_t)previousUnit)+MEMORY_ALIGNED_SIZE(sizeof(void *)));	
		if(memory_pool->MemUsed[i_count] == true && unit == result)
		{
			memory_pool->MemUsed[i_count]  = false;
			smsc_semaphore_signal(pSemMemPoolLock);
			return ;
		}
		/*得到下一个单元数据*/
		(*previousUnit)=(void *)(((mem_ptr_t)previousUnit)+
		((mem_ptr_t)((int)MEMORY_ALIGNED_SIZE(sizeof(void *))))+
		((mem_ptr_t)((int)MEMORY_ALIGNED_SIZE(memory_pool->i_Size))));

		previousUnit=(void **)(*previousUnit);
	}
	SMSC_ERROR(("CAN NOT FIND THE MEM UNIT IN TCPIP STACK MEMORY[%p]!!! ",unit));
	smsc_semaphore_signal(pSemMemPoolLock);
#else	
	nextUnit=(void **)(((mem_ptr_t)unit)-MEMORY_ALIGNED_SIZE(sizeof(void *)));
	if(memory_pool->mHeadUnit == nextUnit)
	{
		do_report(0,"已经释放过同样内存\n");
		return;
	} 
	(*nextUnit)=(void *)memory_pool->mHeadUnit;
	memory_pool->mHeadUnit=nextUnit;
#endif	
}

void Tcpip_Memory_test(void)
{
	int count = 0;

	count = Memory_Block_Test(gEchoContextPool);
	do_report(0,"+++++++++++++++++++++++++++++++\n");
	do_report(0,"gEchoContextPool[%d]\n",count);
	count = Memory_Block_Test(gSmallBufferPool);
	do_report(0,"+++++++++++++++++++++++++++++++");
	do_report(0,"gSmallBufferPool[%d]\n",count);
	count = Memory_Block_Test(gLargeBufferPool);
	do_report(0,"+++++++++++++++++++++++++++++++\n");
	do_report(0,"gLargeBufferPool[%d]\n",count);
	count = Memory_Block_Test(gTcpControlBlockPool);
	do_report(0,"+++++++++++++++++++++++++++++++\n");
	do_report(0,"gTcpControlBlockPool[%d]\n",count);
	count = Memory_Block_Test(gTcpSegmentPool);
	do_report(0,"+++++++++++++++++++++++++++++++\n");
	do_report(0,"gTcpSegmentPool[%d]\n",count);
	count = Memory_Block_Test(gUdpControlBlockPool);
	do_report(0,"+++++++++++++++++++++++++++++++\n");
	do_report(0,"gUdpControlBlockPool[%d]\n",count);

}
int Memory_Block_Test(MEMORY_POOL_HANDLE memory_pool_handle)
{
	PMEMORY_POOL memory_pool=(PMEMORY_POOL)memory_pool_handle;
	int i_count = 0;
	int i_ret = 0;
	
	for(i_count = 0; i_count < memory_pool->i_MaximumUnits;i_count ++)
	{
		if(memory_pool->MemUsed[i_count] == true)
		{
			i_ret++;
		}
	}
	return i_ret;
}
