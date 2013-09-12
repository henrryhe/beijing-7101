/*****************************************************************************
File name   :  OS21Memory.c

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


/*#######################################################################*/
/*########################### DEFINITION ################################*/
/*#######################################################################*/

/* --- Constants (default values) -------------------------------------- */

/* --- Global variables ------------------------------------------------ */

/* --- Prototype ------------------------------------------------------- */

/* --- Externals ------------------------------------------------------- */

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

/*******************************************************************************
Name        : STOS_memcpy
Description : optimized memcpy implementation usign fp registers
Parameters  : the address of the src, the address of the dest, the copy size
Assumptions : None
Limitations :
Returns     : None
*******************************************************************************/
void * STOS_memcpy(void* dst, const void* src, size_t count)
{
#if defined(ST_7200)
    /* Really strange behaviour with 7200, optimized memcpy do not work, to be checked ... */
    /* Use then standard memcpy() */
    memcpy(dst, src, count);

#elif defined(ARCHITECTURE_ST200)
    /* Assembly code is only for ST40 */
    /* Use then standard memcpy() */
    memcpy(dst, src, count);

#else
		U8 *src1, *dst1;
		src1 = (U8*)src;
		dst1 = (U8*)dst;
		if (count >= 32)
		{
			U32 *dst4;
			/* Align the destination to 8 byte boundary. */
			while ((unsigned)dst1 & 0x7)
			{
				*dst1++ = *src1++;
				count--;
			}
			dst4 = (U32*)dst1;
			/* Switch on source alignment. */
			switch ((unsigned)src1 & 0x7)
			{
			case 0:
				/* Use double length fp load stores */
				{
					/* src is 8 byte aligned */
					U32 count_left;
					U32 *src4;
					src4 = (U32*)src1;
					/* Align destination to cache-line boundary */
					while ((unsigned)dst4 & 0x1c)
					{
						*dst4++ = *src4++;
						count -= 4;
					}
					if (count == 32)
					{
						__asm volatile (
							"		fschg                   \n"
							"		pref    @%0             \n"
							"		fmov    @%0+, dr0       \n"
							"		fmov    @%0+, dr2       \n"
							"		fmov    @%0+, dr4       \n"
							"		fmov    @%0+, dr6       \n"
							"		add		#32,   %1		\n"
							"		fmov    dr6,   @-%1     \n"
							"		fmov    dr4,   @-%1   	\n"
							"		fmov    dr2,   @-%1   	\n"
							"		fmov    dr0,   @-%1   	\n"
							"       fschg                   \n"
							::"r" (src4), "r" (dst4):"%dr0","%dr2","%dr4","%dr6");
					}
					else if (count >= 128)
					{
						__asm volatile (
							"		mov     r15, r2        \n"
							"       mov   #-8, r0        \n"
							"		and     r0, r15        \n"
							"		fschg                   \n"
							"		fmov	dr12, @-r15    \n"
							"		fmov	dr14, @-r15    \n"
							"		mov		%2, r1 			\n"
							"       mov     %0, r4          \n"
							"		mov     #-7,r0			\n"
							"		shld	r0, r1			\n"
							"L1:	\n"
							"		pref    @r4             \n"
							"		add     #32, r4			\n"
							"		fmov    @%0+, dr0       \n"
							"		fmov    @%0+, dr2       \n"
							"		fmov    @%0+, dr4       \n"
							"		fmov    @%0+, dr6       \n"
							"		pref    @r4             \n"
							"		add     #32, r4			\n"
							"		fmov    @%0+, dr8       \n"
							"		fmov    @%0+, dr10      \n"
							"		fmov    @%0+, dr12      \n"
							"		fmov    @%0+, dr14      \n"
							"		pref    @r4             \n"
							"		add     #32, r4			\n"
							"		fmov    @%0+, xd0       \n"
							"		fmov    @%0+, xd2       \n"
							"		fmov    @%0+, xd4       \n"
							"		fmov    @%0+, xd6       \n"
							"		pref    @r4             \n"
							"		add     #32, r4			\n"
							"		fmov    @%0+, xd8       \n"
							"		fmov    @%0+, xd10      \n"
							"		fmov    @%0+, xd12      \n"
							"		fmov    @%0+, xd14      \n"
							"		movca.l r0,   @%1       \n"
							"		add		#32,   %1		\n"
							"		fmov    dr6,   @-%1     \n"
							"		fmov    dr4,   @-%1   	\n"
							"		fmov    dr2,   @-%1   	\n"
							"		fmov    dr0,   @-%1   	\n"
							"		add     #32 ,   %1   	\n"
							"		movca.l r0,    @%1      \n"
							"		add     #32 ,   %1   	\n"
							"		fmov    dr14,  @-%1     \n"
							"		fmov    dr12,  @-%1		\n"
							"		fmov    dr10,  @-%1		\n"
							"		fmov    dr8,   @-%1   	\n"
							"		add     #32,   %1   	\n"
							"		movca.l r0,    @%1      \n"
							"		add     #32,   %1   	\n"
							"		fmov    xd6,   @-%1		\n"
							"		fmov    xd4,   @-%1		\n"
							"		fmov    xd2,   @-%1		\n"
							"		fmov    xd0,   @-%1		\n"
							"		add     #32,   %1   	\n"
							"		movca.l r0,   @%1       \n"
							"		add     #32,   %1		\n"
							"		fmov    xd14,  @-%1		\n"
							"		fmov    xd12,  @-%1		\n"
							"		fmov    xd10,  @-%1		\n"
							"		fmov    xd8,   @-%1		\n"
							"		add     #32,   %1   	\n"
							"		dt		r1				\n"
							"		bf		L1				\n"
							"		mov		%2,r0			\n"
							"       and     #127,r0			\n"
							"       shlr2   r0				\n"
							"       tst     r0,r0			\n"
							"       bt      L3				\n"
							"L2:							\n"
							"       mov.l   @%0+,r1			\n"
							"       dt      r0				\n"
							"       mov.l   r1,@%1			\n"
							"       bf/s    L2				\n"
							"         add     #4,%1			\n"
							"L3:\n"
							"		fmov	@r15+, dr14     \n"
							"		fmov	@r15+, dr12     \n"
							"		fschg\n"
							"       mov     r2, r15  \n"
							::"r" (src4), "r" (dst4), "r" (count):"%r0","%r1","%r2","%r4");
					}
                    else
					{
					    while (count >= 4)
						{
					        *dst4++ = *src4++;
							count -= 4;
						}
					}
					/* if count is not a multiple of 4: 1, 2 or 3 bytes can remain */
					/* So we have to transfer them after */
					count_left = count%4;
					dst1 = ((U8*)(dst4)+ count-count_left);
					src1 = ((U8*)(src4)+ count-count_left);
					count = count_left;
				}
				break;
			case 4:
				{
					/* src is 4 byte aligned */
					U32 *src4;
					src4 = (U32*)src1;
					/* Align destination to cache-line boundary */
					while ((unsigned)dst4 & 0x1c) {
						*dst4++ = *src4++;
						count -= 4;
					}
					while (count >= 32) {
						__asm("mov.l   @%0+, r0         \n"
							"mov.l   @%0+, r1         \n"
							"mov.l   @%0+, r2         \n"
							"mov.l   @%0+, r3         \n"
							"movca.l r0,   @%2        \n"
							"mov.l   r1,   @(4, %2)   \n"
							"mov.l   r2,   @(8, %2)   \n"
							"mov.l   r3,   @(12,%2)   \n"
							"mov.l   @%0+, r0         \n"
							"mov.l   @%0+, r1         \n"
							"mov.l   @%0+, r2         \n"
							"mov.l   @%0+, r3         \n"
							"mov.l   r0,   @(16,%2)   \n"
							"mov.l   r1,   @(20, %2)  \n"
							"mov.l   r2,   @(24, %2)  \n"
							"mov.l   r3,   @(28,%2)   \n"
							:"+r" (src4):"r" (src4), "r" (dst4):"%r0","%r1","%r2","%r3");
						dst4  += 8;
/*						src4  += 8; no, this is already done in the assembly code in GCC*/
						count -= 32;
					}
					while (count >= 4) {
						*dst4++ = *src4++;
						count -= 4;
					}
					dst1 = (U8*)dst4;
					src1 = (U8*)src4;
				}
				break;
			case 2:
			case 6:
				/* src is 2 byte aligned */
				{
					U16 *src2;
					src2 = (U16*)src1;

					/* Align destination to cache-line boundary */
					while ((U32)dst4 & 0x1c) {
						U32 m1,m2;
						m1 = *((U16*)src2);
						src2++;
						m2 = *((U16*)src2);
                        src2++;
						*dst4++ = (m2 << 16) | m1;
						count -= 4;
					}
					while (count >= 32) {
						__asm(
							"mov.w   @%0+, r0        \n"
							"mov.w   @%0+, r1        \n"
							"mov.w   @%0+, r2        \n"
							"mov.w   @%0+, r3        \n"
							"extu.w  r0,   r0        \n"
							"shll16  r1              \n"
							"or      r1,   r0        \n"
							"shll16  r3              \n"
							"extu.w  r2,   r2        \n"
							"or      r3,   r2        \n"
							"movca.l r0,   @%2       \n"
							"mov.l   r2,   @(4, %2)  \n"
							"mov.w   @%0+, r0        \n"
							"mov.w   @%0+, r1        \n"
							"mov.w   @%0+, r2        \n"
							"mov.w   @%0+, r3        \n"
							"extu.w  r0,   r0        \n"
							"shll16  r1              \n"
							"or      r1,   r0        \n"
							"shll16  r3              \n"
							"extu.w  r2,   r2        \n"
							"or      r3,   r2        \n"
							"mov.l   r0,   @(8,  %2) \n"
							"mov.l   r2,   @(12, %2) \n"
							"mov.w   @%0+, r0        \n"
							"mov.w   @%0+, r1        \n"
							"mov.w   @%0+, r2        \n"
							"mov.w   @%0+, r3        \n"
							"extu.w  r0,   r0        \n"
							"shll16  r1              \n"
							"or      r1,   r0        \n"
							"shll16  r3              \n"
							"extu.w  r2,   r2        \n"
							"or      r3,   r2        \n"
							"mov.l   r0,   @(16, %2) \n"
							"mov.l   r2,   @(20, %2) \n"
							"mov.w   @%0+, r0        \n"
							"mov.w   @%0+, r1        \n"
							"mov.w   @%0+, r2        \n"
							"mov.w   @%0+, r3        \n"
							"extu.w  r0,   r0        \n"
							"shll16  r1              \n"
							"or      r1,   r0        \n"
							"shll16  r3              \n"
							"extu.w  r2,   r2        \n"
							"or      r3,   r2        \n"
							"mov.l   r0,   @(24, %2) \n"
							"mov.l   r2,   @(28, %2) \n"
							:"+r"(src2):"r"(src2), "r"(dst4):"%r0","%r1","%r2","%r3");
						dst4  += 8;
						count -= 32;
					}
					/* Align destination to cache-line boundary */
					while (count >= 4) {
						U32 m1,m2;
						m1 = *((U16*)src2);
						src2++;
						m2 = *((U16*)src2);
                        src2++;
						*dst4++ = (m2 << 16) | m1;
						count -= 4;
					}
					dst1 = (U8*)dst4;
					src1 = (U8*)src2;
				}
				break;
			case 1:
			case 3:
			case 5:
			case 7:
				/* src is 1 byte aligned */
				{
					/*__unaligned*/ U32 *src_ua4;
					src_ua4 = (/*__unaligned*/ U32*)src1;
					/* Align destination to cache-line boundary */
					while ((unsigned)dst4 & 0x1c) {
						*dst4++ = *src_ua4++;
						count -= 4;
					}
					while (count >= 32) {
						__asm("mov      %0, r0   \n"
							  "movca.l  r0, @%1  \n"
							  ::"r"(src_ua4[0]), "r"(dst4):"%r0" );
						dst4[1] = src_ua4[1];
						dst4[2] = src_ua4[2];
						dst4[3] = src_ua4[3];
						dst4[4] = src_ua4[4];
						dst4[5] = src_ua4[5];
						dst4[6] = src_ua4[6];
						dst4[7] = src_ua4[7];
						src_ua4 += 8;
						dst4    += 8;
						count   -= 32;
					}
					while (count >= 4) {
						*dst4++ = *src_ua4++;
						count -= 4;
					}
					dst1 = (U8*)dst4;
					src1 = (U8*)src_ua4;
				}
				break;

        }
    }
	while (count-- > 0)
    {
        *dst1++ = *src1++;
    }
#endif
	return dst;
}

/*******************************************************************************
Name        : STOS_VirtToPhys
Description : Translates a virtual address into the physical associated one
Parameters  : virtual address to translate
Assumptions : None
Limitations :
Returns     : None
*******************************************************************************/
#ifdef VIRTUAL_MEMORY_SUPPORT
void * STOS_VirtToPhys(void * vAddr)
{
    unsigned int    pAddr_p;

    if (vmem_virt_to_phys((unsigned int)vAddr, &pAddr_p) != OS21_SUCCESS)
    {
        pAddr_p = NULL;
    }

    return pAddr_p;
}
#endif  /* VIRTUAL_MEMORY_SUPPORT */


