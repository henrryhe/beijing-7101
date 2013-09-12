/*****************************************************************************
File name   :  os20memory.c

Description :  Operating system independence file.

COPYRIGHT (C) STMicroelectronics 2007.

Date               Modification                                          Name
----               ------------                                          ----
08/06/2007         Created                                               WA
*****************************************************************************/

#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "stsys.h"
#include "sttbx.h"
#include "stos.h"


/*******************************************************************************
Name        : STOS_memsetUncached
Description : Does an uncached CPU access to the given address and fills the
              given area with the given pattern
Parameters  : the address of the first byte, the pattern, the area size
Assumptions : None
Limitations :
Returns     : None
*******************************************************************************/
void STOS_memsetUncached(const void* const PhysicalAddress, U8 Pattern, U32 Size)
{
    void * Mapped_dest_p;

    Mapped_dest_p = (void*)STOS_MapPhysToUncached(PhysicalAddress, Size);

    memset(Mapped_dest_p, Pattern, Size);

    STOS_UnmapPhysToUncached(Mapped_dest_p, Size);

} /* End of STOS_memsetUncached() function */

/*******************************************************************************
Name        : STOS_memcpyUncachedToUncached
Description : Does only uncached read/write CPU accesses for copying data from
              src to dest
Parameters  : the address of the src, the address of the dest, the copy size
Assumptions : None
Limitations :
Returns     : None
*******************************************************************************/
void STOS_memcpyUncachedToUncached(const void* const uncached_dest, const void* const uncached_src, U32 Size)
{
    void * Mapped_src_p;
    void * Mapped_dest_p;

    Mapped_src_p  = (void *)STOS_MapPhysToUncached(uncached_src, Size);
    Mapped_dest_p = (void *)STOS_MapPhysToUncached(uncached_dest, Size);

    STOS_memcpy(Mapped_dest_p, Mapped_src_p, Size);
    STOS_UnmapPhysToUncached(Mapped_src_p, Size);
    STOS_UnmapPhysToUncached(Mapped_dest_p, Size);
} /* End of STOS_memcpyUncachedToUncached() function */

/*******************************************************************************
Name        : STOS_memcpyCachedToUncached
Description : Does only uncached write CPU accesses and cached read CPU accesses
              for copying data from src to dest
Parameters  : the address of the src, the address of the dest, the copy size
Assumptions : None
Limitations :
Returns     : None
*******************************************************************************/
void STOS_memcpyCachedToUncached(const void* const uncached_dest, const void* const cached_src, U32 Size)
{
    void * Mapped_src_p;
    void * Mapped_dest_p;

    Mapped_src_p  = (void *)STOS_MapPhysToCached(cached_src, Size);
    Mapped_dest_p = (void *)STOS_MapPhysToUncached(uncached_dest, Size);

    STOS_memcpy(Mapped_dest_p, Mapped_src_p, Size);

    STOS_UnmapPhysToCached(Mapped_src_p, Size);
    STOS_UnmapPhysToUncached(Mapped_dest_p, Size);
} /* End of STOS_memcpyCachedToUncached() function */

/*******************************************************************************
Name        : STOS_memcpyUncachedToCached
Description : Does only cached write CPU accesses and uncached read CPU accesses
              for copying data from src to dest
Parameters  : the address of the src, the address of the dest, the copy size
Assumptions : None
Limitations :
Returns     : None
*******************************************************************************/
void STOS_memcpyUncachedToCached(void* const cached_dest, const void* const uncached_src, U32 Size)
{
    void * Mapped_src_p;
    void * Mapped_dest_p;

    Mapped_src_p  = (void *)STOS_MapPhysToUncached(uncached_src, Size);
    Mapped_dest_p = STOS_MapPhysToCached(cached_dest, Size);

    STOS_memcpy(Mapped_dest_p, Mapped_src_p, Size);

    STOS_UnmapPhysToUncached(Mapped_src_p, Size);
    STOS_UnmapPhysToCached(Mapped_dest_p, Size);
} /* End of STOS_memcpyUncachedToCached() function */

