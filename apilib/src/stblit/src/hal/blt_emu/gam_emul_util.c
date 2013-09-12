#include "stdlib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddefs.h>
#include <stavmem.h>
#if !defined(ST_OS21) && !defined(ST_OSWINCE)
	#include <partitio.h>
#endif
#include "blt_init.h"


#include "gam_emul_util.h"


void *gam_emul_malloc(stblit_Device_t* Device_p, int size)
{
	void *ptr;

    ptr = memory_allocate(Device_p->CPUPartition_p, size);

    return ptr;
}

void gam_emul_free(stblit_Device_t* Device_p, void *ptr)
{
    STOS_MemoryDeallocate(Device_p->CPUPartition_p, (char *)ptr);
}

