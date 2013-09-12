/*
 * bigbuffer.c
 *
 * Copyright (C) STMicroelectronics Limited 2004. All rights reserved.
 *
 * Validate that large buffers are transported correctly.
 * Also aply simple performance monitoring al la the benchmark test.
 */

#include "mme.h"

#include "os_abstractions.h"
#include "harness.h"

#if defined __OS20__
#include <cache.h>
#elif defined __OS21__
#include <os21.h>
#if defined __SH4__
#include <os21/st40.h>
#elif defined __ST200__
#include <os21/st200.h>
#endif /* CPU */
#endif /* OS */

#if defined __LINUX__ && defined __KERNEL__
#include <linux/mm.h>
#define HIGH_PRECISION long /* this is good enough for LKM since HZ is small */
#else
#include <stdlib.h>
#define HIGH_PRECISION double
#endif

#define TRANSPORT_NAME "shm"
#define ITERATIONS     5000
#define TRANSFORMER    "mme.bigbuffer"

/* get the current clock time and wait for a clock edge */
#define TIME_START(startTime)   startTime = harness_getTime(); while(startTime == harness_getTime())

/* calculate the elapsed time (and compensate for waiting for the clock edge) */
#define TIME_STOP(startTime)    (harness_getTime() - (startTime + 1))

#define VALUE_OFFSET 1
#define BYTE_VAL     0xac

/* Put a defined sequence in the databuffer */

static void InitBuffer(MME_DataBuffer_t* dataBuffer, int offset)
{
        unsigned currentVal = 1;
        int i;
        for (i=0; i<dataBuffer->NumberOfScatterPages; i++) {
                MME_ScatterPage_t* scatterPage = &(dataBuffer->ScatterPages_p[i]);
                int j;
                unsigned* wordPage = (unsigned*)scatterPage->Page_p;
                unsigned char* addr;
                for (j=0; j<scatterPage->Size/sizeof(unsigned); j++) {
                        wordPage[j] = currentVal + offset;
                        currentVal++;
                }
                addr = (unsigned char*) &wordPage[j];
                j *= sizeof(unsigned);
                for (; j<scatterPage->Size; j++) {
                        addr[j] = BYTE_VAL + offset;
                }

		scatterPage->FlagsIn = 0;
		scatterPage->FlagsOut = 0;
        }
}

static void CheckBuffer(MME_DataBuffer_t* dataBuffer, int offset)
{
        unsigned currentVal = 1;
        int i;
        for (i=0; i<dataBuffer->NumberOfScatterPages; i++) {
                MME_ScatterPage_t* scatterPage = &(dataBuffer->ScatterPages_p[i]);
                int j;
                unsigned* wordPage = (unsigned*)scatterPage->Page_p;
                unsigned char* addr;
                assert(0 != scatterPage->Size);

                for (j=0; j<scatterPage->Size/sizeof(unsigned); j++) {
                    if ( wordPage[j] != currentVal + offset ) {
                         int pos = j * sizeof(unsigned);
                         printf( LOCAL "Failed - word 0x%08x - offset %x, index %x, value %x, expected %x\n",  
                                 (unsigned) &wordPage[j], pos, j, wordPage[j], currentVal + offset);
                         assert(1==0);
                     }
                     currentVal++;
                }
                addr = (unsigned char*) &wordPage[j];
                j *= sizeof(unsigned);
                for (; j<scatterPage->Size; j++) {
                        if (addr[j] != BYTE_VAL + offset ) {
                                printf( LOCAL "Failed - byte 0x%08x - buffer pos %x, value %x, expected %x\n",
                                        (unsigned) &addr[j], j, addr[j], BYTE_VAL + offset);
                                assert(1==0);
                        }
                }
        }
}

#if defined CONF_MASTER

static OS_SIGNAL callbackReceived;

static void TransformerCallback(MME_Event_t Event, MME_Command_t *CallbackData, void *UserData)
{
	VERBOSE(printf(MACHINE "received callback\n"));
	OS_SIGNAL_POST(&callbackReceived);
}

#define SCATTER 8
struct ScatterPages_s { 
        MME_ScatterPage_t scatterPage[SCATTER+1];
        MME_ScatterPage_t* origPage;
};

#if defined(__LINUX__) && defined(__KERNEL__) 
void* big_malloc(size_t size) {
    void* pAddr;
#if defined(__SH4__)
    if( size >= (PAGE_SIZE * 4) )
        pAddr = (EMBX_VOID *)bigphysarea_alloc( size );
    else
#endif
        pAddr = (EMBX_VOID *)kmalloc( size, GFP_KERNEL );
    return pAddr;
}


void big_free(void* addr) {
#if defined __SH4__
    unsigned long  Base, Size;
    unsigned long  Address = (unsigned long)addr;

    bigphysarea_memory( &Base, &Size );
    if( (Address >= Base) && (Address < (Base + Size)) )
        bigphysarea_free_pages((void*)addr);
    else
#endif
        free(addr);
}

#else
void* big_malloc(size_t size) {
    return malloc(size);
}

void big_free(void* addr) {
    free(addr);
}
#endif

static MME_DataBuffer_t* CreateDataBuffer(unsigned size, int flags, MME_TransformerHandle_t hdl)
{
        MME_DataBuffer_t* dataBuffer;
        int i;
        char* pageBase;
        int pageSize = size/SCATTER;
        int rem = size%SCATTER;
        int useMalloc = (flags == -1);
            
        if (useMalloc) {
                struct MallocBuffer_s { 
                       MME_DataBuffer_t  dataBuffer;
                       MME_ScatterPage_t scatterPage[SCATTER+1];
                }* buffer;

                int allocSize = sizeof(*buffer) + size;
                buffer = (struct MallocBuffer_s*) big_malloc(allocSize);
                if (!buffer) {
                        return NULL;
                }
                dataBuffer = &buffer->dataBuffer;
                dataBuffer->StructSize = sizeof(MME_DataBuffer_t);
                dataBuffer->ScatterPages_p = &(buffer->scatterPage[0]);
                pageBase = (char*) (&buffer[1]);

        } else {
                struct ScatterPages_s* pages;
                pages = (struct ScatterPages_s*) malloc(sizeof(*pages));
                if (!pages) {
                        return NULL;
                }

       		MME(AllocDataBuffer(hdl, size, flags, &dataBuffer));
                pages->origPage = dataBuffer->ScatterPages_p;
                dataBuffer->ScatterPages_p = &pages->scatterPage[0];
                pageBase = (char*)pages->origPage->Page_p;
        }
        dataBuffer->NumberOfScatterPages = rem?(SCATTER+1):SCATTER;

	/* wee on the memory and get it written back to main memory */
	memset(pageBase, 0xcc, pageSize);
#if defined __OS20__
	cache_flush_data(0, 0);
#elif defined __OS21__
	cache_purge_data_all();
#endif

        /* Iterate over complete pages */
        for (i=0; i<SCATTER; i++) {
                dataBuffer->ScatterPages_p[i].Page_p = pageBase;
                dataBuffer->ScatterPages_p[i].Size = pageSize;
                pageBase += pageSize;
        }
        /* Deal with any extra bytes that do not fill a page */
        if (rem) {
                dataBuffer->ScatterPages_p[i].Page_p = pageBase;
                dataBuffer->ScatterPages_p[i].Size = rem;
        }
        return dataBuffer;
}

static void FreeDataBuffer(MME_DataBuffer_t* dataBuffer, int flags)
{
        int useMalloc = (flags == -1);
        if (useMalloc) {
                big_free(dataBuffer);
        } else {
                struct ScatterPages_s* pages = (struct ScatterPages_s*) dataBuffer->ScatterPages_p;
                dataBuffer->ScatterPages_p = pages->origPage;                
                free(pages);
        	MME(FreeDataBuffer(dataBuffer));
        }
}

static void run_benchmark(MME_TransformerHandle_t hdl, MME_Command_t *cmd, char *test)
{
	volatile int start, elapsed;
	int i;

	/* Verify contents transported - only do once*/

        InitBuffer(cmd->DataBuffers_p[0], 0);
        InitBuffer(cmd->DataBuffers_p[1], 0);
        cmd->ParamSize = 1;
        cmd->Param_p   = &cmd;

	CheckBuffer(cmd->DataBuffers_p[0], 0);
	CheckBuffer(cmd->DataBuffers_p[1], 0);

	MME(SendCommand(hdl, cmd));
	OS_SIGNAL_WAIT(&callbackReceived);

        CheckBuffer(cmd->DataBuffers_p[1], 0x10000);

        cmd->ParamSize = 0; cmd->Param_p = NULL;

        /* Now time the real thing */
	TIME_START(start);

 	for (i=0; i<ITERATIONS; i++) {
		MME_SendCommand(hdl, cmd);
		OS_SIGNAL_WAIT(&callbackReceived);
	}
	elapsed = (int) ((HIGH_PRECISION) TIME_STOP(start) * (HIGH_PRECISION) 1000000 / (HIGH_PRECISION) harness_getTicksPerSecond());

	printf("  %-56s%6d.%03d%10d\n", test,
               elapsed/1000000, elapsed / 1000 % 1000,
	       (unsigned) ((HIGH_PRECISION) elapsed / (HIGH_PRECISION) ITERATIONS));
}

int main(void)
{
#define K 1024
	struct { 
		char *name; 
		int input;
		int output;
		MME_AllocationFlags_t flags;
	} schedule[] = {

		{ "Affinity allocated 1k/1k",       1*K,   1*K, 0 },
		{ "Affinity allocated 64k/64k",    64*K,  64*K, 0 },
		{ "Affinity allocated 64k/256k",   64*K, 256*K, 0 },
		{ "Affinity allocated 256k/64k",  256*K,  64*K, 0 },
		{ "Affinity allocated 256k/256k", 256*K, 256*K, 0 },
		{ "malloc() allocated 1k/1k",       1*K,   1*K, -1 },
		{ "malloc() allocated 64k/64k",    64*K,  64*K, -1 },
		{ "malloc() allocated 64k/256k",   64*K, 256*K, -1 },
		{ "malloc() allocated 256k/64k",  256*K,  64*K, -1 },
		{ "malloc() allocated 256k/256k", 256*K, 256*K, -1 },

#if 0
		{ "Cached 64k/64k",    64*K,  64*K, MME_ALLOCATION_CACHED },
		{ "Cached 64k/256k",   64*K, 256*K, MME_ALLOCATION_CACHED },
		{ "Cached 256k/64k",  256*K,  64*K, MME_ALLOCATION_CACHED },
		{ "Cached 256k/256k", 256*K, 256*K, MME_ALLOCATION_CACHED },
		{ "Uncached 64k/64k",   64*K,  64*K,  MME_ALLOCATION_UNCACHED },
		{ "Uncached 64k/256k",   64*K, 256*K, MME_ALLOCATION_UNCACHED },
		{ "Uncached 256k/64k",  256*K,  64*K, MME_ALLOCATION_UNCACHED },
		{ "Uncached 256k/256k", 256*K, 256*K, MME_ALLOCATION_UNCACHED },
#endif
		{ 0 }
	};

	MME_TransformerInitParams_t init = {
		sizeof(MME_TransformerInitParams_t),
		MME_PRIORITY_NORMAL,
		TransformerCallback,
		(void *) 0x0,
		0,
		NULL,
	};
	MME_TransformerHandle_t hdl;
	MME_DataBuffer_t *bufs[2];
	MME_Command_t command = {
		sizeof(MME_Command_t),          /* struct size */
		MME_TRANSFORM,                  /* CmdCode */
		MME_COMMAND_END_RETURN_NOTIFY   /* CmdEnd */
	};
	int i;

	/* bring up the API */
	harness_boot();
	OS_SIGNAL_INIT(&callbackReceived);
	MME(Init());
#if !defined(USERMODE)
	MME(RegisterTransport(TRANSPORT_NAME));
#endif
	/* initialize the benchmark transformer */
	while (MME_SUCCESS != MME_I(InitTransformer(TRANSFORMER, &init, &hdl))) {
		harness_sleep(1);
	}

	/* construct our test commoand */
	command.NumberInputBuffers = 1;
	command.NumberOutputBuffers = 1;
	command.DataBuffers_p = bufs;

	printf("******** benchmarking with %d iterations ********\n", ITERATIONS);
	printf("\n  %-56s%10s%10s\n\n", "TEST", "TIME (S)", "ITER (uS)");

	for (i=0; schedule[i].name; i++) {

#if defined(USERMODE)
                /* Only use MME_AllocDataBuffer in Linux user mode */
                if (-1 == schedule[i].flags) {
                        continue;
                }
#endif
                bufs[0] = CreateDataBuffer(schedule[i].input, schedule[i].flags, hdl);
                bufs[1] = CreateDataBuffer(schedule[i].output, schedule[i].flags, hdl);

                if (!bufs[0] || !bufs[1]) {
                	printf("  %-56s%s\n", schedule[i].name, "Failed to allocate buffer\n");
                        return 1;
                }

		run_benchmark(hdl, &command, schedule[i].name);

        	FreeDataBuffer(bufs[0], schedule[i].flags);
	        FreeDataBuffer(bufs[1], schedule[i].flags);
	}
	printf("\n");

	/* tidy up */
	MME(TermTransformer(hdl));
	MME(Term());
	OS_SIGNAL_DESTROY(&callbackReceived);

	harness_shutdown();
	return 0;
}

#else /* CONF_MASTER */

static MME_ERROR successful(void) { return MME_SUCCESS; }
static MME_ERROR processCommand(void* ctx, MME_Command_t* cmd)
{
	VERBOSE(printf(MACHINE "Called processCommand\n"));

        if (cmd->ParamSize) {
                CheckBuffer(cmd->DataBuffers_p[0], 0);
                InitBuffer(cmd->DataBuffers_p[1], 0x10000);
                CheckBuffer(cmd->DataBuffers_p[1], 0x10000);
        }

	return MME_SUCCESS;
}

int main(void)
{
	MME_ERROR (*fn)() = successful;

	harness_boot();

	MME(Init());
	MME(RegisterTransport(TRANSPORT_NAME));
	MME(RegisterTransformer(TRANSFORMER, fn, fn, fn, processCommand, fn));
	MME(Run());

	harness_shutdown();
	return 0;
}

#endif /* CONF_MASTER */
