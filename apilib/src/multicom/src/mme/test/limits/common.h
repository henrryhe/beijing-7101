#ifndef COMMON_H
#define COMMON_H

/* Divide the allocation into MME scatter pages */
#define MAX_ALLOCATION                  (16*1024)
#define NUM_SCATTER_PAGES_PER_PHYS_PAGE 4
#define MAX_SCATTER_PAGES               (NUM_SCATTER_PAGES_PER_PHYS_PAGE * MAX_ALLOCATION/PAGE_SIZE)
#define SCATTER_PAGE_SIZE               (PAGE_SIZE / NUM_SCATTER_PAGES_PER_PHYS_PAGE )


#include <mme.h>

#include "harness.h"

#include "allocate.h"

#if defined __LINUX__

#include <asm/page.h>

#if defined __KERNEL__

#include <linux/highmem.h>
#include <linux/mm.h>
#include <linux/bigphysarea.h>

#else

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#endif

#elif defined __OS20__ || defined __OS21__ || defined __WINCE__
#define PAGE_SIZE 4096
#endif

#define TRANSFORM(BYTE) (BYTE+10)

typedef struct {
	MME_DataBuffer_t*         origDataBuffer;
} DataBufferInfo_t;

typedef struct {
        int                       useKernelAlloc;
	MME_DataBuffer_t*         dataBuffer;
        union {
        	/* Address and length for mmap() */
        	struct {                           
        		void*     address;
        		unsigned  length;
        	} kernel;
		/* A copy of the original struct for deallocation */
		MME_DataBuffer_t* origDataBuffer;
        } u;
} AllocInfo_t;

/* Utilities to init MME buffer structures */
static void InitScatterPage(MME_ScatterPage_t* page, void* buffer, unsigned size) {
	page->Page_p = buffer;
	page->Size   = size;
}

static void InitDataBuffer(MME_DataBuffer_t* buffer, MME_ScatterPage_t* pages, int pageCount, int size) {
	memset(buffer, 0, sizeof(*buffer));
	buffer->StructSize           = sizeof(MME_DataBuffer_t);
	buffer->NumberOfScatterPages = pageCount;
	buffer->ScatterPages_p       = pages;
	buffer->TotalSize            = size;
}

static void AllocScatterPages(AllocInfo_t* allocInfo, int size) {
        int j;
        int bytesLeft = size;
        unsigned bufferBase = 0;
	MME_ScatterPage_t* scatterPages;

        /* Divide the page into sub MME scatter pages */
        int totalScatterPages = NUM_SCATTER_PAGES_PER_PHYS_PAGE * size/PAGE_SIZE;
        if (size % PAGE_SIZE) {
		int extraBytes = size % PAGE_SIZE;
                int extraPages = extraBytes / NUM_SCATTER_PAGES_PER_PHYS_PAGE;
                if (extraBytes % NUM_SCATTER_PAGES_PER_PHYS_PAGE) {
                	extraPages++;
                }
		totalScatterPages += extraPages;
        }

	if (allocInfo->useKernelAlloc) {
        	bufferBase = (unsigned) allocInfo->u.kernel.address;
	} else {
        	bufferBase = (unsigned) allocInfo->dataBuffer->ScatterPages_p->Page_p;
	}

        if (bufferBase % PAGE_SIZE) {
                /* Align to page boundary */
                bufferBase = (bufferBase & ~(PAGE_SIZE-1)) + PAGE_SIZE;
        }

        scatterPages = malloc(totalScatterPages * sizeof(MME_ScatterPage_t));
        assert(scatterPages);
        memset(scatterPages, 0, totalScatterPages * sizeof(MME_ScatterPage_t));
        allocInfo->dataBuffer->ScatterPages_p = scatterPages;

        for (j=0; j<totalScatterPages; j++) {
                int scatterSize = SCATTER_PAGE_SIZE;
                char* address = (char*)bufferBase + j*scatterSize;

                assert(bytesLeft>0);
                
                if (bytesLeft<scatterSize) {
                            scatterSize = bytesLeft;
                }
                bytesLeft      -= scatterSize;

                InitScatterPage(&scatterPages[j], address, scatterSize);
        }
        InitDataBuffer(allocInfo->dataBuffer, scatterPages, j, size);
}


static AllocInfo_t* AllocBuffer(int useKernelAlloc, MME_TransformerHandle_t transformerHandle, unsigned size, unsigned cached) {
        /* Allocate and carve into SCATTER_PAGES scatter pages */
        /* For Linux each scatter page must exist contiguously within an OS page */

        AllocInfo_t* allocInfo = malloc(sizeof(AllocInfo_t));
        assert(allocInfo);

        allocInfo->useKernelAlloc = useKernelAlloc;
        allocInfo->dataBuffer = malloc(sizeof(MME_DataBuffer_t));
        assert(allocInfo->dataBuffer);
	memset(allocInfo->dataBuffer, 0, sizeof(MME_DataBuffer_t));

	if (!useKernelAlloc) {
                
		MME_ERROR err = MME_AllocDataBuffer(transformerHandle, size+PAGE_SIZE, cached?MME_ALLOCATION_CACHED:MME_ALLOCATION_UNCACHED, &allocInfo->u.origDataBuffer );
		assert(MME_SUCCESS == err);

		/* Copy the allocates struct to our copy area which we can modify -
		 * must preserve the old struct for deallocation */
		*(allocInfo->dataBuffer) = *(allocInfo->u.origDataBuffer);		
        } else {
		/* Add PAGE_SIZE for page aligned */
		allocInfo->u.kernel.address = KernelAllocate(size+PAGE_SIZE, 0, &allocInfo->u.kernel.length); 
	}
        AllocScatterPages(allocInfo, size);

        return allocInfo;
}

static void FreeScatterPages(AllocInfo_t* allocInfo) {
        free(allocInfo->dataBuffer->ScatterPages_p);
        allocInfo->dataBuffer->ScatterPages_p = NULL;
}

static void FreeBuffer(AllocInfo_t* allocInfo) {

        FreeScatterPages(allocInfo);
 
        if (allocInfo->useKernelAlloc) {
		KernelFree(allocInfo->u.kernel.address, allocInfo->u.kernel.length);
        } else {
                /* Copy the original structure contents */
        	MME_FreeDataBuffer(allocInfo->u.origDataBuffer);
        }
	free(allocInfo->dataBuffer);
	allocInfo->dataBuffer = NULL;
        free(allocInfo);
}

typedef enum {
	Fill_e,
	Extract_e,
	GetByte_e,
	PutByte_e
} FillDirection_e;

static unsigned char FillExtractBuffer(FillDirection_e direction, MME_DataBuffer_t* dataBuffer, unsigned char* localBuffer, unsigned size_item) {
	unsigned page = 0;
	unsigned pos = 0;

	unsigned char* scatterAddress = NULL;

	if ((Fill_e == direction || Extract_e == direction) && 0==size_item) {
		return 0;		
        }

	while (page<dataBuffer->NumberOfScatterPages) {
		int i;
		MME_ScatterPage_t* scatterPage = &dataBuffer->ScatterPages_p[page];

		scatterAddress = scatterPage->Page_p;

		for (i=0; i<scatterPage->Size; i++) {

			switch (direction) {
			case Fill_e:
				*scatterAddress++ = *localBuffer++;
				if (pos == size_item-1) {
					return 1;
				}
				break;
			case Extract_e:
				*localBuffer++ = *scatterAddress++;
				if (pos == size_item-1) {
					return 1;
				}
				break;
			case GetByte_e:
				if (pos == size_item) {
					*scatterAddress = *localBuffer;
					return 1;
				}
				scatterAddress++;
				break;
			case PutByte_e:
				if (pos == size_item) {
					*scatterAddress = *localBuffer;
					return 1;
				}
				scatterAddress++;
				break;
			}
			pos++;
		}
		page++;
	}
	return 0;
}

#define FillBuffer(A, B, C)    FillExtractBuffer(Fill_e, A, B, C)
#define ExtractBuffer(A, B, C) FillExtractBuffer(Extract_e, A, B, C)
#define GetByte(A, B, C)       FillExtractBuffer(GetByte_e, A, B, C)
#define PutByte(A, B, C)       FillExtractBuffer(PutByte_e, A, B, C)

static int BufferInit(void) {
	return KernelAllocateInit();	
}

static int BufferDeinit(void) {
	return KernelAllocateDeinit();	
}

#endif
