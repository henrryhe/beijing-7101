/**************************************************************/
/* Copyright STMicroelectronics 2006, all rights reserved     */
/*                                                            */
/* File: embxshm_genericFactory.c                             */
/*                                                            */
/* Description:                                               */
/*    Factory function to configure the transport using       */
/*    user supplied function pointers                         */
/*                                                            */
/**************************************************************/

#include "embx_osheaders.h"
#include "embx_osinterface.h"
#include "embxP.h"
#include "embxshmP.h"
#include "embxshm.h"

#define TCB_OFFSET 512
#define RESERVED_BYTES 32

/* enable the mailbox interrupt and update the interrupt handler */
static void install_isr (EMBXSHM_Transport_t *tpshm, void (*handler)(void *))
{
    EMBXSHM_GenericConfig_t *config = tpshm->factoryConfig;
    EMBXSHM_GenericConfig_InstallIsr_t installIsr;

    installIsr.handler = handler;
    installIsr.param = tpshm;
    installIsr.cpuID = tpshm->cpuID;

    (void) config->notifyFn(config->notifyData, EMBXSHM_OPCODE_INSTALL_ISR, &installIsr);
}

/* disable the mailbox interrupt and remove the interrupt handler */
static void remove_isr (EMBXSHM_Transport_t *tpshm)
{
    EMBXSHM_GenericConfig_t *config = tpshm->factoryConfig;
    (void) config->notifyFn(config->notifyData, EMBXSHM_OPCODE_REMOVE_ISR, NULL);
}

/* cause an interrupt to raised on the other CPU */
static void raise_int (EMBXSHM_Transport_t *tpshm, EMBX_UINT destCPU)
{
    EMBXSHM_GenericConfig_t *config = tpshm->factoryConfig;
    (void) config->notifyFn(config->notifyData, EMBXSHM_OPCODE_RAISE_INTERRUPT, (void *) destCPU);
}

/* clear an interrupt asserted on this cpu */
static void clear_int (EMBXSHM_Transport_t *tpshm)
{
    EMBXSHM_GenericConfig_t *config = tpshm->factoryConfig;
    (void) config->notifyFn(config->notifyData, EMBXSHM_OPCODE_CLEAR_INTERRUPT, NULL);
}

/* unmask interrupts asserts on this cpu */
static void enable_int (EMBXSHM_Transport_t *tpshm)
{
    EMBXSHM_GenericConfig_t *config = tpshm->factoryConfig;
    (void) config->notifyFn(config->notifyData, EMBXSHM_OPCODE_ENABLE_INTERRUPT, NULL);
}

/* mask interrupts asserts on this cpu */
static void disable_int (EMBXSHM_Transport_t *tpshm)
{
    EMBXSHM_GenericConfig_t *config = tpshm->factoryConfig;
    (void) config->notifyFn(config->notifyData, EMBXSHM_OPCODE_DISABLE_INTERRUPT, NULL);
}

static void buffer_flush (EMBXSHM_Transport_t *tpshm, void *address)
{
    EMBXSHM_GenericConfig_t *config = tpshm->factoryConfig;
    (void) config->notifyFn(config->notifyData, EMBXSHM_OPCODE_BUFFER_FLUSH, address);
}

/*
 * this transport performs the configuration that it was too early
 * to perform in the factory function.
 *
 * note on forms of addresses: 
 * 
 * for user convenience both sharedAddr and warpRangeAddr are specified
 * as local addresses in the config structure (i.e. the master). for
 * implementation convenience these pointers are passed to slaves as
 * bus addresses. we are not allowed to modify these configurations
 * since they
 */
static EMBX_ERROR configureTransport(EMBXSHM_Transport_t *tpshm, EMBXSHM_GenericConfig_t *config)
{
    void *localSharedAddr, *localWarpAddr;

    EMBX_Info(EMBX_INFO_FACTORY, (">>>configureTransport(0x%08x, 0x%08x)\n", (unsigned) tpshm, (unsigned) config));

#if defined __ST231__ && defined __OS21__
    /* we may not be able to directly address the configuration pointer because it sits
     * in another CPU's protection zone 
     */
    {
	EMBXSHM_GenericConfig_t *virtualConfig;
	void *sharedAddr;
	unsigned int sharedSize;

	virtualConfig = mmap_translate_uncached(config);
	if (!virtualConfig) {
		virtualConfig = mmap_translate_virtual(config);
		virtualConfig = mmap_translate_uncached(virtualConfig);
	}

	EMBX_Info(EMBX_INFO_FACTORY, ("   configureTransport: virtualConfig = 0x%08x\n", 
	                             (unsigned) virtualConfig));

	if (NULL == virtualConfig) 
	{
		EMBX_Info(EMBX_INFO_FACTORY, ("<<<configureTransport = EMBX_NOMEM:0\n"));
		return EMBX_NOMEM;
	}

	/* find the bounds of the shared memory without using dismissible loads */
	__asm__ ("ldw %0 = 0,%1" : "=r" (sharedAddr) : "r" (&(virtualConfig->sharedAddr)));
	__asm__ ("ldw %0 = 0,%1" : "=r" (sharedSize) : "r" (&(virtualConfig->sharedSize)));

	/* enable speculation for this shared memory. failure is not fatal since OS21 does
	 * not distingish between 'failed' and 'already the case, thank you flower'.
	 */
	mmap_enable_speculation(mmap_translate_physical(BUS_TO_LOCAL(sharedAddr, tpshm->pointerWarp)), sharedSize);

	config = virtualConfig;
    }
#endif

    EMBXSHM_READS(config);

    /* a note on forms of addresses
     *
     * for user convenience both sharedAddr and warpRangeAddr are specified
     * as local addresses in the config structure (i.e. the master). 
     *
     * for implementation convenience these pointers are passed to slaves 
     * as bus addresses (since the master cannot convert them to slave local
     * addresses since it does not know what the pointer warp is). we are not
     * allowed to directly modify the master supplied configuration structure
     * since this is shared by multiple slaves. thus we treat the configuration
     * structure differently for a master vesus a slave and store the result
     * in local variables.
     */
    localSharedAddr = config->sharedAddr;
    EMBX_Info(EMBX_INFO_FACTORY, ("   configureTransport: localSharedAddr = 0x%08x\n", localSharedAddr));
    if (0 == tpshm->cpuID) {
	/* allocate the memory if the shared address is NULL */
	if (NULL == localSharedAddr)
	{
	    localSharedAddr = EMBX_OS_ContigMemAlloc(config->sharedSize, config->sharedSize);
	    if (NULL == localSharedAddr) 
	    {
		EMBX_Info(EMBX_INFO_FACTORY, ("<<<configureTransport = EMBX_NOMEM:1\n"));
		return EMBX_NOMEM;
	    }

#if defined EMBXSHM_CACHED_HEAP
	    localSharedAddr = EMBXSHM_MEM_CACHED(localSharedAddr);
#else
	    localSharedAddr = EMBXSHM_MEM_UNCACHED(localSharedAddr);
#endif
	}

	localWarpAddr   = config->warpRangeAddr;
    } 
    else 
    {
	/* it is 'impossible' for the local address to be NULL */
	EMBX_Assert(NULL != localSharedAddr);
	localSharedAddr = BUS_TO_LOCAL(localSharedAddr, tpshm->pointerWarp);
	localWarpAddr   = BUS_TO_LOCAL(config->warpRangeAddr, tpshm->pointerWarp);
    }
    
#if defined __ST220__ && defined __OS21__
    /* the shared heap must be safe to speculate in and must not be cached */
    {
	protection_list_t cfg;
	int res;

	cfg.address   = localSharedAddr;
	cfg.length    = config->sharedSize / 1024;
#if defined EMBXSHM_CACHED_HEAP
	cfg.attribute = protection_attribute_memory;
#else
	cfg.attribute = protection_attribute_uncached;
#endif
	res = dpu_config(&cfg, 1, 0);
	EMBX_Assert(0 == res);

	/* ensure there are no cache entries covering this address due to speculation */
	cache_purge_data(localSharedAddr, config->sharedSize);
    }
#endif

#if defined __ST231__ && defined __OS21__
	{
		void *virtualSharedAddr;

		/* ensure there are no cache entries covering this address (and two fingers
		 * to any speculation problems)
		 */
		cache_purge_data(localSharedAddr, config->sharedSize);

		/* ensure we access the local memory uncached (we prefer to use xlate where
		 * possible since this will allow use to have a wider warpRangeAddr)
		 *
		 * try localSharedAddr first as a virtual address, if this fails assume it
		 * is an unmapped physical address.
		 */
		virtualSharedAddr = mmap_translate_uncached(localSharedAddr);
		if (!virtualSharedAddr) {
		    virtualSharedAddr = mmap_translate_virtual(localSharedAddr);
		    cache_purge_data(virtualSharedAddr, config->sharedSize);
		    virtualSharedAddr = mmap_translate_uncached(virtualSharedAddr);
		}
		if (NULL == virtualSharedAddr) {
			EMBX_Info(EMBX_INFO_FACTORY, ("<<<configureTransport = EMBX_NOMEM:2\n"));
			return EMBX_NOMEM;
		}
		cache_purge_data(virtualSharedAddr, config->sharedSize);

		/* dynamically update the pointer warp so that the addresses we store in
		 * the shared memory are not contaminated with this CPU's VM oddities.
		 */
		tpshm->pointerWarp += ((char *) localSharedAddr - (char *) virtualSharedAddr);

		/* update the warps on address that have already been converted */
		if (0 != tpshm->cpuID) {
			localWarpAddr = (char *) localWarpAddr - ((char *) localSharedAddr - (char *) virtualSharedAddr);
		}

		localSharedAddr = virtualSharedAddr;
	}
#endif

    /* configure the parts of the transport structure that can
     * be determined only from a complete factory config.
     */
    tpshm->transport.transportInfo.maxPorts = config->maxPorts;
    tpshm->objectTableSize		    = (config->maxObjects > 0 ? 
                                               config->maxObjects :
					       EMBX_HANDLE_DEFAULT_TABLE_SIZE);
    tpshm->freeListSize			    = config->freeListSize;
    tpshm->tcb				    = (void *) ((uintptr_t) localSharedAddr + TCB_OFFSET);
    tpshm->heap				    = (void *) ((uintptr_t) tpshm->tcb + sizeof(EMBXSHM_TCB_t));
    tpshm->heapSize                         = config->sharedSize - sizeof(EMBXSHM_TCB_t) - TCB_OFFSET;

    if (0 != config->warpRangeSize) {
	tpshm->warpRangeAddr = localWarpAddr;
	tpshm->warpRangeSize = config->warpRangeSize;
    } else {
	tpshm->warpRangeAddr = tpshm->heap;
	tpshm->warpRangeSize = tpshm->heapSize;
    }

    EMBX_Info(EMBX_INFO_FACTORY, ("   pointerWarp = 0x%08x\n", (unsigned) tpshm->pointerWarp));
    EMBX_Info(EMBX_INFO_FACTORY, ("   sharedAddr = 0x%08x [0x%08x]\n", (unsigned) config->sharedAddr, (unsigned) localSharedAddr));
    EMBX_Info(EMBX_INFO_FACTORY, ("   sharedSize = %d\n", config->sharedSize));
    EMBX_Info(EMBX_INFO_FACTORY, ("   warpRangeAddr = 0x%08x [0x%08x]\n", (unsigned) tpshm->warpRangeAddr, (unsigned) config->warpRangeAddr));
    EMBX_Info(EMBX_INFO_FACTORY, ("   warpRangeSize = %d [%d]\n", tpshm->warpRangeSize, config->warpRangeSize));
    EMBX_Info(EMBX_INFO_FACTORY, ("   TCB = 0x%08x\n", tpshm->tcb));
    EMBX_Info(EMBX_INFO_FACTORY, ("   heap = 0x%08x\n", tpshm->heap));
    EMBX_Info(EMBX_INFO_FACTORY, ("   heapSize = 0x%08x\n", tpshm->heapSize));

    EMBX_Info(EMBX_INFO_FACTORY, ("<<<configureTransport = EMBX_SUCCESS\n"));
    return EMBX_SUCCESS;
}

/*
 * setup the shared memory environment for the master
 */
static EMBX_ERROR setup_master_environment(EMBXSHM_Transport_t *tpshm)
{
    EMBXSHM_GenericConfig_t *config = tpshm->factoryConfig;
    EMBXSHM_GenericConfig_t *remoteConfig;
    EMBX_ERROR res;

    EMBX_Info(EMBX_INFO_FACTORY, (">>>setup_master_environment(0x%08x)\n", (unsigned) tpshm));

    EMBX_Assert(0 == tpshm->masterCPU);
    EMBX_Assert(0 == tpshm->cpuID);

    /* complete the configuration from the supplied config structure */
    res = configureTransport(tpshm, config);
    if (EMBX_SUCCESS != res) 
    {
	EMBX_Info(EMBX_INFO_FACTORY, ("<<<setup_master_environment = %d\n", res));
	return res;
    }

    /* produce a remote configuration block */
    EMBX_Assert((TCB_OFFSET - RESERVED_BYTES) > sizeof(EMBXSHM_GenericConfig_t));
    remoteConfig = (EMBXSHM_GenericConfig_t *) (((char *) tpshm->tcb) - (TCB_OFFSET - RESERVED_BYTES));
    EMBX_Assert(NULL != remoteConfig);

    /* set up the block */
    *remoteConfig = *config;
    remoteConfig->sharedAddr = LOCAL_TO_BUS(((char *) tpshm->tcb) - TCB_OFFSET, tpshm->pointerWarp);
    remoteConfig->warpRangeAddr = LOCAL_TO_BUS(remoteConfig->warpRangeAddr, tpshm->pointerWarp);
    EMBXSHM_WROTE(remoteConfig);

    /* we must zero the tcb before we rendezvous otherwise the companion processor
     * might get too far ahead of us.
     */
    memset(tpshm->tcb, 0, sizeof(EMBXSHM_TCB_t));

    /* it is important the the companion processor never 'overtakes' us until we
     * reach this point.
     */
    config->notifyFn(config->notifyData, EMBXSHM_OPCODE_RENDEZVOUS, NULL);

    /* we can now perform the generic initialization code */
    res = EMBXSHM_genericMasterInit(tpshm);
    if (EMBX_SUCCESS != res)
    {
	goto error_exit;
    }

    EMBX_Info(EMBX_INFO_FACTORY, ("<<<setup_master_environment = EMBX_SUCCESS\n"));
    return EMBX_SUCCESS;

error_exit:

    EMBX_Info(EMBX_INFO_FACTORY, ("<<<setup_master_environment = %d\n", res));
    return res;
}

/*
 * setup the shared memory environment for the slave. this only
 * really involved pluging the previously used components together
 * in a different order.
 */
static EMBX_ERROR setup_slave_environment(EMBXSHM_Transport_t *tpshm)
{
    EMBX_ERROR res;
    EMBXSHM_GenericConfig_t *config = tpshm->factoryConfig;

    EMBX_Info(EMBX_INFO_FACTORY, (">>>setup_slave_environment(0x%08x)\n", (unsigned) tpshm));

    /* (optionally) block until the host has booted */
    config->notifyFn(config->notifyData, EMBXSHM_OPCODE_RENDEZVOUS, NULL);

    config = (EMBXSHM_GenericConfig_t *) (((char *) (config->sharedAddr)) + RESERVED_BYTES);

    res = configureTransport(tpshm, config);
    EMBX_Assert(EMBX_SUCCESS == res); /* can't fail if we are a slave */

    res = EMBXSHM_genericSlaveInit(tpshm);
    if (EMBX_SUCCESS != res)
    {
	EMBX_Info(EMBX_INFO_FACTORY, ("<<<setup_slave_environment = %d\n", res));
	return res;
    }

    EMBX_Info(EMBX_INFO_FACTORY, ("<<<setup_slave_environment = EMBX_SUCCESS\n"));
    return EMBX_SUCCESS;
}

/*
 * this is the generic factory function, it is used to register both
 * master and slave transports and works in nearly the same way in each
 * case.
 */
#ifdef EMBXSHM_CACHED_HEAP
EMBX_Transport_t *EMBXSHMC_generic_factory(void *param)
#else
EMBX_Transport_t *EMBXSHM_generic_factory(void *param)
#endif
{
    EMBXSHM_GenericConfig_t *config = (EMBXSHM_GenericConfig_t *) param;
    EMBXSHM_Transport_t *tpshm;
    EMBX_INT i;

    EMBX_Info(EMBX_INFO_FACTORY, (">>>EMBXSHM_generic_factory(0x%08x)\n", (unsigned) param));

    /* check the validity of the generic parts of the template */
    if ( (NULL == config) ||
         (NULL == config->name) ||
	 (   0 == config->participants[0]) ||
	 (   0 == config->participants[config->cpuID]) ||
	 (config->cpuID >= EMBXSHM_MAX_CPUS) )
    {
	EMBX_Info(EMBX_INFO_FACTORY, ("<<<EMBXSHM_generic_factory = NULL:0\n"));
	return NULL;
    }

    /* check the template follows the appropriate form for a master of a slave */
    if (0 == config->cpuID)
    {
	/* we are a master */
	if ( (0 == config->sharedAddr) ||
	     (64*1024 > config->sharedSize) ||
	     (0 == config->freeListSize) )
	{
	    EMBX_Info(EMBX_INFO_FACTORY, ("<<<EMBXSHM_generic_factory = NULL:1\n"));
	    return NULL;
	}
    }
    else
    {
	/* we are a slave */
	if ( (0 == config->sharedAddr) ||
	     (64*1024 > config->sharedSize) ||
	     (0 != config->maxPorts) ||
	     (0 != config->maxObjects) ||
	     (0 != config->freeListSize) )
	{
	    EMBX_Info(EMBX_INFO_FACTORY, ("<<<EMBXSHM_generic_factory = NULL:2\n"));
	    return NULL;
	}
    }

    /* memory permitting we know we have a good configuration at this point */

    /* allocate memory for our transport structure (and cope with failures) */
    tpshm = (EMBXSHM_Transport_t *) EMBX_OS_MemAlloc (sizeof(*tpshm));
    if (NULL == tpshm)
    {
	EMBX_OS_MemFree (tpshm);
	EMBX_Info(EMBX_INFO_FACTORY, ("<<<EMBXSHM_generic_factory = NULL:3\n"));
	return NULL;
    }

    /* initialize the transport structure */
    memcpy (tpshm, &EMBXSHM_transportTemplate, sizeof(*tpshm));
    tpshm->transport.transportInfo.allowsMultipleConnections = EMBX_TRUE;
    tpshm->masterCPU       = 0;

    /* copy the details from the generic portion of the config structure */
    strcpy (tpshm->transport.transportInfo.name, config->name);
    for (i=0; i<EMBXSHM_MAX_CPUS; i++) 
    {
	tpshm->participants[i] = config->participants[i];

	/* find the highest id within the system */
	if (tpshm->participants[i])
	{
	    tpshm->maxCPU = i;
	}
    }
    tpshm->cpuID           = config->cpuID;
    tpshm->pointerWarp     = config->pointerWarp;

    /* setup links to the factory specific structures */
    tpshm->factoryConfig   = config;
    tpshm->factoryData     = NULL;
    
    /* setup the behavioral specializations */
    tpshm->install_isr     = install_isr;
    tpshm->remove_isr      = remove_isr;
    tpshm->raise_int       = raise_int;
    tpshm->clear_int       = clear_int;
    tpshm->enable_int      = enable_int;
    tpshm->disable_int     = disable_int;
    tpshm->buffer_flush    = buffer_flush;
    tpshm->setup_shm_environment = (0 == config->cpuID ? setup_master_environment : setup_slave_environment);

    EMBX_Info(EMBX_INFO_FACTORY, ("<<<EMBXSHM_generic_factory = 0x%08x\n", (unsigned) tpshm));
    return (EMBX_Transport_t *) tpshm;
}
