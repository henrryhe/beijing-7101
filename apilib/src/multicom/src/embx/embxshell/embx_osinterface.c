/*******************************************************************/
/* Copyright 2002 STMicroelectronics R&D Ltd. All rights reserved. */
/*                                                                 */
/* File: embx_osinterface.c                                        */
/*                                                                 */
/* Description:                                                    */
/*         Operating system abstraction implementation             */
/*                                                                 */
/*******************************************************************/

#include "embx_osinterface.h"
#include "debug_ctrl.h"

/*
 * This header is needed to collect the prototype for memalign.
 */
#if defined(__OS21__) && defined(__SH4__)
#include <malloc.h>
#endif

/*----------------------------- MEMORY ALLOCATION ---------------------------*/

EMBX_VOID *EMBX_OS_ContigMemAlloc(EMBX_UINT size, EMBX_UINT align)
{
void **alignedAddr = NULL;

    EMBX_Info(EMBX_INFO_OS, (">>>>ContigMemAlloc(size=%u, align=%u)\n",size,align));

#if defined(__LINUX__) && defined(__KERNEL__) && defined(__SH4__)

    /* For Linux bigphysarea must be available and set to a sensible size */
    {
        int pages      = (0 == size  ? 0 : ((size  - 1) / PAGE_SIZE) + 1);
        int page_align = (0 == align ? 0 : ((align - 1) / PAGE_SIZE) + 1);

	/* allocate an carefully aligned block of memory */
	EMBX_Info(EMBX_INFO_OS, ("    trying to allocate %d pages (aligned to %d pages)\n", pages, page_align));
        alignedAddr = (void **) bigphysarea_alloc_pages(pages, page_align, GFP_KERNEL);

	if (alignedAddr) {
	    /* ensure there are no cache entries covering this address */
	    dma_cache_wback_inv(alignedAddr, size);

	    /* finally warp the address into an uncached memory pointer */
	    alignedAddr = P2SEGADDR(alignedAddr);
	}
    }

#elif defined(__OS21__) && defined(__SH4__)

    /* allocate an carefully aligned block of memory */
    alignedAddr = memalign(align, size);

    if (alignedAddr) {
	/* ensure there are no cache entries covering this address */
	cache_invalidate_data(alignedAddr, size);

	/* finally warp the address into an uncached pointer */
	alignedAddr = ADDRESS_IN_UNCACHED_MEMORY(alignedAddr);
    }

#elif defined(__LINUX__) && defined(__KERNEL__)

    printk("EMBX_OS_ContigMemAlloc Not implemented yet!!!\n");

#elif defined(__OS20__) || defined(__OS21__) || defined(__SOLARIS__) || defined(__LINUX__)

    {
    void *baseAddr = NULL;

        if (0 == align)
        {

            baseAddr    = malloc(size + sizeof(void *));

            if (baseAddr != NULL)
            {
                alignedAddr     = ((void **) baseAddr) + 1;
                alignedAddr[-1] = baseAddr;
            }
            else
            {
                EMBX_DebugMessage(("ContigMemAlloc: could not allocate %u bytes\n",size+sizeof(void*)));
            }
        }
        else if (0 == (align & (align - 1)))
        {
            /* allocate more memory than we need so we can alter the
             * pointer to guarantee alignment
             */

            baseAddr = malloc(size + align);

            /* round alignedAddr up so it is aligned as requested, we deliberately
             * round base up even if it is already aligned so that we can guarantee
             * that alignedAddr[-1] is a valid address.
             *
             * note we don't have uintptr_t in OS20/21 so we have to use unsigned
             * long for our pointer calculations
             */
            if (baseAddr != NULL)
            {
                unsigned long base = (unsigned long) baseAddr;

                alignedAddr     = (void **) (((base) | ((unsigned long) align - 1)) + 1);
                alignedAddr[-1] = baseAddr;
            }
            else
            {
                EMBX_DebugMessage(("ContigMemAlloc: could not allocate %u bytes for requested alignment\n",size+align));
            }
        }
        else
        {
            EMBX_DebugMessage(("ContigMemAlloc: requested alignment is not a power of 2\n"));
        }
    }

#elif defined (__WINCE__) || defined(WIN32)

   /* Currently this isn't supported on WinCE */
   EMBX_DebugMessage(("ContigMemAlloc: Not supported on WinCE\n"));

#endif /* __LINUX__ */

    EMBX_Info(EMBX_INFO_OS, ("<<<<ContigMemAlloc = 0x%08x\n", (unsigned) alignedAddr));
    return (EMBX_VOID *)alignedAddr;
}


void EMBX_OS_ContigMemFree(EMBX_VOID *addr, EMBX_UINT size)
{
    EMBX_Info(EMBX_INFO_OS, (">>>>ContigMemFree\n"));

#if defined (__LINUX__) && defined(__KERNEL__) && defined(__SH4__)

    /* Must pass P1 address in to bigphysarea_free_pages */
    addr = (void*) ((int)addr & 0x1FFFFFFF);
    bigphysarea_free_pages(P1SEGADDR(addr));

#elif defined(__LINUX__) && defined(__KERNEL__)

    printk("EMBX_OS_ContigMemFree not implemented yet\n");

#elif defined(__OS21__) && defined(__SH4__)

    free(ADDRESS_IN_CACHED_MEMORY(addr));

#elif defined(__OS20__) || defined(__OS21__) || defined(__SOLARIS__) || defined(__LINUX__)

    free(((void **)addr)[-1]);

#elif defined (__WINCE__) || defined(WIN32)

    /* Currently this isn't supported on WinCE */
    EMBX_DebugMessage(("ContigMemFree: Not supported on WinCE\n"));

#endif /* __LINUX__ */

    EMBX_Info(EMBX_INFO_OS, ("<<<<ContigMemFree\n"));
}



EMBX_VOID *EMBX_OS_MemAlloc(EMBX_UINT size)
{
void *pAddr;

#if defined(__OS20__) || defined(__OS21__) || \
    ( defined(WIN32) && !defined(__WINCE__) ) || \
    defined(__SOLARIS__) || \
    ( defined(__LINUX__) && !defined(__KERNEL__))

    pAddr = (EMBX_VOID *)malloc(size);

#elif defined(__WINCE__)

    pAddr = (EMBX_VOID *)VirtualAlloc( NULL,
                                       (DWORD)size,
                                       (MEM_COMMIT | MEM_RESERVE),
                                       PAGE_READWRITE );

#elif defined(__LINUX__) && defined(__KERNEL__)

#if defined(__SH4__)
    if( size >= (PAGE_SIZE * 4) )
        pAddr = (EMBX_VOID *)bigphysarea_alloc( size );
    else
#endif
        pAddr = (EMBX_VOID *)kmalloc( size, GFP_KERNEL );

#else

#error Unsupported processor and operating system combination

#endif /* __OS20__ || __OS21__ || __SOLARIS__*/

    return pAddr;
}



/* This is an ANSI C like memory deallocate. 
 *
 * Because it is ANSI C like it is defined that this function will ignore
 * a NULL argument and return immediately.
 */
void EMBX_OS_MemFree(EMBX_VOID *addr)
{

#if defined(__OS20__) || defined(__OS21__) || \
    ( defined(WIN32) && !defined(__WINCE__) ) || \
    defined(__SOLARIS__) || \
    ( defined(__LINUX__) && !defined(__KERNEL__))

    free((void *)addr);

#elif defined (__WINCE__)

    if (addr)
    {
        if( VirtualFree((LPVOID)addr, (DWORD)0, MEM_RELEASE) )
        {
            VirtualFree((LPVOID)addr, (DWORD)0, MEM_DECOMMIT );
        }
        else
        {
            EMBX_DebugMessage(("MemFree: VirtualFree failed\n"));
        }
    }

#elif defined(__LINUX__) && defined(__KERNEL__)

#if defined __SH4__
    unsigned long  Base, Size;
    unsigned long  Address = (unsigned long)addr;

    bigphysarea_memory( &Base, &Size );
    if( (Address >= Base) && (Address < (Base + Size)) )
        bigphysarea_free_pages((void*)addr);
    else
#endif
        /*
         * kfree will (correctly) ignore NULL arguments
         */
        kfree((void*)addr);

#else

#error Unsupported processor and operating system combination

#endif /* __OS20__ || __OS21__ */

}


#if defined(__OS20__)

/* this API provide a simple cache of initialized OS20 semaphores
 * because creating/initializing them is expensive on that OS.
 *
 * in order to do this safely we are forced to alter the internal
 * state of a semaphore which must be considered dubious behavior.
 * when a semahore is destroyed this requires us to forcibly alter
 * its count since this value is unknown (we do however know that
 * the wait queue is empty). in fact knowing that semaphore_count
 * is unknown on delete usefully provides us a pointer sized member
 * in the semaphore that it is safe for us to form a linked list
 * with.
 */

static semaphore_t *cacheHead = NULL;

semaphore_t *EMBX_OS_SemaphoreCreate(EMBX_UINT count)
{
semaphore_t *sem;

    EMBX_OS_INTERRUPT_LOCK();

    if (NULL != cacheHead)
    {
        sem       = cacheHead;
        cacheHead = (semaphore_t *) (sem->semaphore_count);

        sem->semaphore_count = count;

        EMBX_OS_INTERRUPT_UNLOCK();
    }
    else
    {
        EMBX_OS_INTERRUPT_UNLOCK();
        sem = semaphore_create_fifo(count);
    }

    return sem;
}



void EMBX_OS_SemaphoreDelete(semaphore_t *sem)
{
    EMBX_Assert(sem);
    EMBX_Assert(sem->semaphore_count < 64); /* catch double-delete */

    EMBX_OS_INTERRUPT_LOCK();

    sem->semaphore_count = (int) cacheHead;
    cacheHead = sem;

    EMBX_OS_INTERRUPT_UNLOCK();
}

#endif /* __OS20__ */


#if defined(__OS21__)

/* this API provides a simple cache of initialized OS21 semaphores
 * because deleting them is expensive on this OS (all objects are
 * dynamically allocated and free() is too slow to appear on critical
 * paths).
 *
 * in order to do this we use a custom allocator that allocates an
 * extra word that we can use as a linkage pointer without having
 * to know anything about the internal structure of the semaphore.
 * this technique while somewhat clumsy uses only published OS21
 * APIs and it therefore future proof.
 */

static semaphore_t *cacheHead = NULL;

/*
 * The following partition pointer is volatile to ensure the allocate/backoff
 * logic in EMBX_OS_EventCreate doesn't get optimized away. 
 */
static volatile partition_t *cachePartition = NULL;

static void *cacheAlloc  (void *state, size_t size);
static void  cacheFree   (void *state, void *ptr);
static void *cacheRealloc(void *state, void *ptr, size_t size);
static int   cacheStatus (void *state, partition_status_t *status, partition_status_flags_t flags);


semaphore_t *EMBX_OS_EventCreate(void)
{
    semaphore_t *sem = 0;

    task_lock();

    if (cacheHead != 0)
    {
        sem = cacheHead;

        EMBX_Assert(0 == semaphore_value(sem));
        cacheHead = (semaphore_t *) ((void **) cacheHead)[-1];

	task_unlock();
    }
    else
    {
        /* just in time initialization of the cachePartition
         * (our custom memory allocator)
         */
        if (0 == cachePartition)
        {
	    /* Try to create a partition, this may block in the OS allowing
	     * another task to run.
             */
            partition_t *p = partition_create_any(0, cacheAlloc,   cacheFree,
                                                     cacheRealloc, cacheStatus);


            /* Since we may have allowed another tasks to run it is possible
             * another task may have beaten us to it and already set
             * a partition. If this is the case then we release the
             * partition we just got and use the one that is now there.
             */
            if (0 == cachePartition)
            {
                EMBX_Assert(p != 0);

                cachePartition = p;
            }
            else
            {
                /* This is a really unlikely corner case, but we can
                 * still continue even if our own partition create failed!
		 * (because 0 == cachePartition the semaphore will be
		 * created from the C library heap)
                 */
                if(p != 0)
                {
                    partition_delete(p);
                }
            }
        }


	task_unlock();
        sem = semaphore_create_fifo_p((partition_t *)cachePartition, 0);
    }

    return sem;
}


void EMBX_OS_EventDelete(semaphore_t *sem)
{
    EMBX_Assert(sem);

    /* This should assert if there are waiters on the semaphore
     * but OS21 does not give us an interface to find this out
     * at the moment.
     */

    /* Drain the semaphore count to 0 so that we can
     * re-allocate it from the cache in a default state.
     */
    while(semaphore_value(sem) != 0)
    {
        semaphore_wait(sem);
    }


    task_lock();

    ((void **) sem)[-1] = (void *) cacheHead;
    cacheHead = sem;

    task_unlock();
}


static void * cacheAlloc(void *state, size_t size)
{
    return ((void **) memory_allocate(NULL, size+sizeof(void *))) + 1;
}


static void cacheFree(void *state, void *ptr)
{
    memory_deallocate(NULL, ((void **) ptr) - 1);
}


static void * cacheRealloc(void *state, void *ptr, size_t size)
{
    return ((void **) memory_reallocate(NULL, ((void **) ptr) - 1, size+sizeof(void *))) + 1;
}


static int cacheStatus(void *state, partition_status_t      *status,
                                    partition_status_flags_t flags)
{
    return partition_status(NULL, status, flags);
}

#endif /* __OS21__ */


/*
 * This looks general purpose but it is, in fact, only used to map the dual
 * port RAM into addressability.
 */
EMBX_VOID *EMBX_OS_PhysMemMap(EMBX_VOID *pMem, int size)
{

    EMBX_Info(EMBX_INFO_OS, (">>>>PhysMemMap(0x%08x, %d)\n", (unsigned int) pMem, size));

#if defined(__WINCE__)
    {
    SYSTEM_INFO sysinfo;
    unsigned int pagesize, alignment, virt, physical;

        /*
         * get the system page size
         */
        GetSystemInfo(&sysinfo);
        pagesize = sysinfo.dwPageSize;

        /*
         * warp the physical address to be aligned on a page boundary
         * and increase size acordingly
         */ 
        physical  = (unsigned int)pMem & ~(pagesize-1);
        alignment = (unsigned int)pMem - physical;
        size     += alignment;

        virt = (unsigned int) VirtualAlloc(NULL, (DWORD) size, MEM_RESERVE, PAGE_NOACCESS);
        if (virt != 0)
        {
            if (!VirtualCopy((void *) virt, (void *) physical,(DWORD) size, (PAGE_READWRITE|PAGE_NOCACHE)))
            {
                EMBX_DebugMessage(("PhysMemMap: VirtualCopy failed\n"));
                VirtualFree((void *) virt, 0, MEM_RELEASE);
                virt = 0;
            }
            else
            {
                /*
                 * unwarp the virtual address
                 */
                virt += alignment;
                EMBX_DebugMessage(("PhysMemMap: *vMem = 0x%X\n", virt));
            }
        }

        EMBX_Info(EMBX_INFO_OS, ("<<<<PhysMemMap\n"));

        return (EMBX_VOID *)virt;
    }

#else

    EMBX_Info(EMBX_INFO_OS, ("<<<<PhysMemMap\n"));

    return pMem;

#endif /* __WINCE__ */

}



void EMBX_OS_PhysMemUnMap(EMBX_VOID *vMem)
{
    EMBX_Info(EMBX_INFO_OS, (">>>>PhysMemUnMap\n"));

#if defined (__WINCE__)
    {
    SYSTEM_INFO sysinfo;
    unsigned int virt = (unsigned int) vMem;

        /*
         * free the memory after warping it back to its original value
         */
        GetSystemInfo(&sysinfo);
        if (!VirtualFree((LPVOID) (virt & ~(sysinfo.dwPageSize)), (DWORD)0, MEM_RELEASE ))
        {
            EMBX_DebugMessage(("PhysMemUnmap: VirtualFree failed\n"));
        }
    }
#endif /* __WINCE__ */

    EMBX_Info(EMBX_INFO_OS, ("<<<<PhysMemUnMap\n"));
}


/*----------------------------- INTERRUPT LOCK ------------------------------*/

#if defined __WINCE__ || defined WIN32
static CRITICAL_SECTION interrupt_critical_section;
static int interrupt_critical_section_valid = 0;

void EMBX_OS_InterruptLock(void)
{
	/* TODO: initialization of the critical section is not thread-safe */
	if (0 == interrupt_critical_section_valid) {
		InitializeCriticalSection(&interrupt_critical_section);
		interrupt_critical_section_valid = 1;
	}

	EnterCriticalSection(&interrupt_critical_section);
}

void EMBX_OS_InterruptUnlock(void)
{
	LeaveCriticalSection(&interrupt_critical_section);
}
#endif
