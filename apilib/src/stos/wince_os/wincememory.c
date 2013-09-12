/*****************************************************************************
File name   :  wincememory.c

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
void * __cdecl STOS_memcpy(void* dst, const void* src, size_t count)
{
  char *src1, *dst1;
	unsigned char srcalign;

  src1 = (char*)src;
  dst1 = (char*)dst;
  if (count >= 32)
  {
      int *dst4;
      // Align the destination to 8 byte boundary.
      while ((unsigned)dst1 & 0x7)
      {
          *dst1++ = *src1++;
          count--;
      }

		  dst4 = (int*)dst1;

		  srcalign = (unsigned)src1 & 0x7;
          // Switch on source alignment.
          switch (srcalign)
          {
		  case 0:
			// Use double length fp load stores
			{
				// src is 8 byte aligned
				int count_left;
				int *src4;
				src4 = (int*)src1;
				// Align destination to cache-line boundary
				while ((unsigned)dst4 & 0x1c)
				{
					*dst4++ = *src4++;
					count -= 4;
				}
				if (count >= 128)
				{
					__asm(
						"		mov     r15, r2,        \n"
						"       mov.l   #-8, r0,        \n"
						"		and     r0, r15,        \n"
						"		fschg\n"
						"		fmov	dr12, @-r15,    \n"
						"		fmov	dr14, @-r15,    \n"
						"		mov		r6, r1 			\n"
						"		mov     #-7,r0			\n"
						"		shld	r0, r1			\n"	 // count div 128
						"L1:	\n"
						"		pref    @r7             \n"
						"		add     #32, r7			\n"
						"		fmov    @r4+, dr0       \n"
						"		fmov    @r4+, dr2       \n"
						"		fmov    @r4+, dr4       \n"
						"		fmov    @r4+, dr6       \n"
						"		pref    @r7             \n"
						"		add     #32, r7			\n"
						"		fmov    @r4+, dr8       \n"
						"		fmov    @r4+, dr10      \n"
						"		fmov    @r4+, dr12      \n"
						"		fmov    @r4+, dr14      \n"
						"		pref    @r7             \n"
						"		add     #32, r7			\n"
						"		fmov    @r4+, xd0       \n"
						"		fmov    @r4+, xd2       \n"
						"		fmov    @r4+, xd4       \n"
						"		fmov    @r4+, xd6       \n"
						"		pref    @r7             \n"
						"		add     #32, r7			\n"
						"		fmov    @r4+, xd8       \n"
						"		fmov    @r4+, xd10      \n"
						"		fmov    @r4+, xd12      \n"
						"		fmov    @r4+, xd14      \n"
						"		movca.l r0,   @r5       \n"
						"		add		#32,   r5		\n"
						"		fmov    dr6,   @-r5     \n"
						"		fmov    dr4,   @-r5   	\n"
						"		fmov    dr2,   @-r5   	\n"
						"		fmov    dr0,   @-r5   	\n"
						"		add     #32 ,   r5   	\n"
						"		movca.l r0,    @r5      \n"
						"		add     #32 ,   r5   	\n"
						"		fmov    dr14,  @-r5     \n"
						"		fmov    dr12,  @-r5		\n"
						"		fmov    dr10,  @-r5		\n"
						"		fmov    dr8,   @-r5   	\n"
						"		add     #32,   r5   	\n"
						"		movca.l r0,    @r5      \n"
						"		add     #32,   r5   	\n"
						"		fmov    xd6,   @-r5		\n"
						"		fmov    xd4,   @-r5		\n"
						"		fmov    xd2,   @-r5		\n"
						"		fmov    xd0,   @-r5		\n"
						"		add     #32,   r5   	\n"
						"		movca.l r0,   @r5       \n"
						"		add     #32,   r5		\n"
						"		fmov    xd14,  @-r5		\n"
						"		fmov    xd12,  @-r5		\n"
						"		fmov    xd10,  @-r5		\n"
						"		fmov    xd8,   @-r5		\n"
						"		add     #32,   r5   	\n"
						"		dt		r1				\n"
						"		bf		L1				\n"
						"		mov		r6,r0			\n"
						"       and     #127,r0			\n"
						"       shlr2   r0				\n"
						"       tst     r0,r0			\n"
						"       bt      L3				\n"
						"L2:							\n"
						"       mov.l   @r4+,r1			\n"
						"       dt      r0				\n"
						"       mov.l   r1,@r5			\n"
						"       bf/s    L2				\n"
						"         add     #4,r5			\n"
						"L3:\n"
						"		fmov	@r15+, dr14     \n"
						"		fmov	@r15+, dr12     \n"
						"		fschg\n"
						"       mov     r2, r15  \n"
						,src4,dst4,count,src4);
				}
				else
				{
				    while (count >= 4)
					{
				        *dst4++ = *src4++;
						count -= 4;
					}
				}
				// if count is not a multiple of 4: 1, 2 or 3 bytes can remain
				// So we have to transfer them after
				count_left = count%4;
				dst1 = ((char*)(dst4)+ count-count_left);
				src1 = ((char*)(src4)+ count-count_left);
				count = count_left;
			}
			break;
          case 4:
              {
                  // src is 4 byte aligned
                  int *src4;

                  src4 = (int*)src1;
                  // Align destination to cache-line boundary
                  while ((unsigned)dst4 & 0x1c) {
                      *dst4++ = *src4++;
                      count -= 4;
                  }
                  while (count >= 32) {
                      __asm("mov.l   @r4+, r0         \n"
                          "mov.l   @r4+, r1         \n"
                          "mov.l   @r4+, r2         \n"
                          "mov.l   @r4+, r3         \n"
                          "movca.l r0,   @r5        \n"
                          "mov.l   r1,   @(4, r5)   \n"
                          "mov.l   r2,   @(8, r5)   \n"
                          "mov.l   r3,   @(12,r5)   \n"
                          "mov.l   @r4+, r0         \n"
                          "mov.l   @r4+, r1         \n"
                          "mov.l   @r4+, r2         \n"
                          "mov.l   @r4+, r3         \n"
                          "mov.l   r0,   @(16,r5)   \n"
                          "mov.l   r1,   @(20, r5)  \n"
                          "mov.l   r2,   @(24, r5)  \n"
                          "mov.l   r3,   @(28,r5)   \n",
                          src4,dst4);
                      dst4  += 8;
                      src4  += 8;
                      count -= 32;
                  }
                  while (count >= 4) {
                      *dst4++ = *src4++;
                      count -= 4;
                  }
                  dst1 = (char*)dst4;
                  src1 = (char*)src4;
              }
              break;

    case 2:
		case 6:
		{

			__asm( 		"mov.l	r11, @-r15    \n");

			__asm("mov   r4, r11        \n"
				  "mov   r5, r7        \n"
				  ,dst4, src1);

			if(srcalign == 6) {
			//Copy first 4 bytes
			__asm(		"mov		 r7,r3								        \n"
						"mov.b   @(0x00000003,r3),r0          \n"
						"mov     r0,r2                        \n"
						"shll8   r2                           \n"
						"mov.b   @(0x00000002,r3),r0          \n"
						"extu.b  r0,r1                        \n"
						"or      r2,r1                        \n"
						"shll8   r1                           \n"
						"mov.b   @(0x00000001,r3),r0          \n"
						"extu.b  r0,r2                        \n"
						"or      r1,r2                        \n"
						"shll8   r2                           \n"
						"mov.b   @r3,r3                       \n"
						"extu.b  r3,r3                        \n"
						"or      r2,r3                        \n"
						"mov.l   r3,@r11         						  \n"
						"add     #0x00000004,r7              \n"
						"add     #0x00000004,r11              \n"
						);
			count = count - 4;
			}

				__asm( "mov		r7,r3                  \n"
						"add    #0xFFFFFFFE,r3            \n"
						"mov.l	@r3,r6                    \n"
						"add #0x00000002,r7									\n"
					);

			while (count >= 32) {
				__asm("mov.l	@r7+,r0          \n"
							"mov.l	@r7+,r1          \n"
							"mov.l	@r7+,r2          \n"
							"mov.l	@r7+,r3          \n"
							"shlr16  r6                     \n"
							"mov	r0,r5      				      \n"
							"shll16 r0				  \n"
							"or      r6,r0                  \n"
							"shlr16  r5                     \n"
							"mov	r1,r6                     \n"
							"shll16  r1				  \n"
							"or      r5,r1                  \n"
							"shlr16  r6                     \n"
							"mov	r2,r5                     \n"
							"shll16  r2				  \n"
							"or      r6,r2                  \n"
							"shlr16  r5                     \n"
							"mov	r3,r6                     \n"
							"shll16  r3				  \n"
							"or      r5,r3                  \n"
							"mov.l	 r0,   @r11      \n"
							"mov.l   r1,   @(0x04, r11) \n"
							"mov.l   r2,   @(0x08, r11)   \n"
							"mov.l   r3,   @(0x0C,r11)   \n"
							"mov.l	@r7+,r0          \n"
							"mov.l	@r7+,r1          \n"
							"mov.l	@r7+,r2          \n"
							"mov.l	@r7+,r3          \n"
							"shlr16  r6                     \n"
							"mov	r0,r5      				      \n"
							"shll16 r0				  \n"
							"or      r6,r0                  \n"
							"shlr16  r5                     \n"
							"mov	r1,r6                     \n"
							"shll16  r1				  \n"
							"or      r5,r1                  \n"
							"shlr16  r6                     \n"
							"mov	r2,r5                     \n"
							"shll16  r2				  \n"
							"or      r6,r2                  \n"
							"shlr16  r5                     \n"
							"mov	r3,r6                     \n"
							"shll16  r3				  \n"
							"or      r5,r3                  \n"
							"mov.l   r0,   @(0x10, r11)      \n"
							"mov.l   r1,   @(0x14, r11) \n"
							"mov.l   r2,   @(0x18, r11)   \n"
							"mov.l   r3,   @(0x1C, r11)   \n"
							"add	 #0x00000020,r11	\n");
							count = count - 32;
			}

							while(count >= 4) {
							__asm("shlr16   r6          \n"
							"mov.l	@r7+,r0        \n"
							"mov	r0,r5                 \n"
							"shll16  r0                 \n"
							"or      r6,r0              \n"
							"mov.l   r0,@(r11)          \n"
							"mov	r5,r6                 \n"
							"add	 #0x00000004,r11	\n");
							count   -= 4;
						}

						__asm("add     #-2,r7      \n");

						while (count-- > 0)
						{
							__asm("mov.b	@r7+,r3            \n"
							"mov.b	r3,@r11             \n"
							"add     #0x00000001,r11    \n"
							);
						}

						count = 0;
						__asm("		mov.l   @r15+,r11    \n" );
          }
          break;

   case 1:
   case 5:
		{

			__asm( 		"mov.l	r11, @-r15    \n");

			__asm("mov   r4, r11      \n"
				  "mov   r5, r7        \n"
				  ,dst4, src1);

			if(srcalign == 5) {
			//Copy first 4 bytes
			__asm(		"mov		 r7,r3								    \n"
						"mov.b   @(0x00000003,r3),r0          \n"
						"mov     r0,r2                        \n"
						"shll8   r2                           \n"
						"mov.b   @(0x00000002,r3),r0          \n"
						"extu.b  r0,r1                        \n"
						"or      r2,r1                        \n"
						"shll8   r1                           \n"
						"mov.b   @(0x00000001,r3),r0          \n"
						"extu.b  r0,r2                        \n"
						"or      r1,r2                        \n"
						"shll8   r2                           \n"
						"mov.b   @r3,r3                       \n"
						"extu.b  r3,r3                        \n"
						"or      r2,r3                        \n"
						"mov.l   r3,@r11         						  \n"
						"add     #0x00000004,r7              \n"
						"add     #0x00000004,r11              \n"
						);

			count = count - 4;
			}


				__asm( "mov		r7,r3                  \n"
						"add    #0xFFFFFFFF,r3            \n"
						"mov.l	@r3,r6                    \n"
						"mov	 #24, r4					  \n"
						"add #0x00000003,r7									\n"
					);

			while (count >= 32) {
				__asm("mov.l	@r7+,r0          \n"
							"mov.l	@r7+,r1          \n"
							"mov.l	@r7+,r2          \n"
							"mov.l	@r7+,r3          \n"
							"shlr8   r6                     \n"
							"mov	r0,r5      				      \n"
							"shld r4, r0				  \n"
							"or      r6,r0                  \n"
							"shlr8   r5                     \n"
							"mov	r1,r6                     \n"
							"shld r4, r1				  \n"
							"or      r5,r1                  \n"
							"shlr8   r6                     \n"
							"mov	r2,r5                     \n"
							"shld r4, r2				  \n"
							"or      r6,r2                  \n"
							"shlr8   r5                     \n"
							"mov	r3,r6                     \n"
							"shld r4, r3				  \n"
							"or      r5,r3                  \n"
							"mov.l	 r0,   @r11      \n"
							"mov.l   r1,   @(0x04, r11) \n"
							"mov.l   r2,   @(0x08, r11)   \n"
							"mov.l   r3,   @(0x0C,r11)   \n"
							"mov.l	@r7+,r0          \n"
							"mov.l	@r7+,r1          \n"
							"mov.l	@r7+,r2          \n"
							"mov.l	@r7+,r3          \n"
							"shlr8   r6                     \n"
							"mov	r0,r5      				      \n"
							"shld r4, r0				  \n"
							"or      r6,r0                  \n"
							"shlr8   r5                     \n"
							"mov	r1,r6                     \n"
							"shld r4, r1				  \n"
							"or      r5,r1                  \n"
							"shlr8   r6                     \n"
							"mov	r2,r5                     \n"
							"shld r4, r2				  \n"
							"or      r6,r2                  \n"
							"shlr8   r5                     \n"
							"mov	r3,r6                     \n"
							"shld r4, r3				  \n"
							"or      r5,r3                  \n"
							"mov.l 	 r0,   @(0x10,r11)      \n"
							"mov.l   r1,   @(0x14, r11) \n"
							"mov.l   r2,   @(0x18, r11)   \n"
							"mov.l   r3,   @(0x1C,r11)   \n"
							"add	 #0x00000020,r11	\n");
							count = count - 32;
			}

							while(count >= 4) {
							__asm("shlr8   r6                     \n"
							"mov.l	@r7+,r0            \n"
							"mov	r0,r5                     \n"
							"shld		 r4,r0                  \n"
							"or      r6,r0                  \n"
							"mov.l   r0,@(r11)              \n"
							"mov	r5,r6         \n"
							"add     #0x00000004,r11        \n");

							count   -= 4;
						}

						__asm("add     #-3,r7      \n");

						while (count-- > 0)
						{
							__asm("mov.b	@r7+,r3          \n"
							"mov.b	r3,@r11                 \n"
							"add     #0x00000001,r11        \n");
						}

						count = 0;

						__asm("		mov.l   @r15+,r11    \n" );
          }
          break;

    case 3:
		case 7:
		{


			__asm("mov.l	r11, @-r15    \n" );


			__asm("mov   r4, r11       \n"
				  "mov   r5, r7        \n"
				  ,dst4, src1);

			__asm("mov	 #-24, r4					  \n");

			if(srcalign == 7) {
			__asm("mov		 r7,r3					  \n"
						"mov.b   @(0x00000003,r3),r0          \n"
						"mov     r0,r2                        \n"
						"shll8   r2                           \n"
						"mov.b   @(0x00000002,r3),r0          \n"
						"extu.b  r0,r1                        \n"
						"or      r2,r1                        \n"
						"shll8   r1                           \n"
						"mov.b   @(0x00000001,r3),r0          \n"
						"extu.b  r0,r2                        \n"
						"or      r1,r2                        \n"
						"shll8   r2                           \n"
						"mov.b   @r3,r3                       \n"
						"extu.b  r3,r3                        \n"
						"or      r2,r3                        \n"
						"mov.l   r3,@r11         						  \n"
						"add     #0x00000004,r7              \n"
						"add     #0x00000004,r11              \n");
			count = count - 4;
			}


				__asm("mov		r7,r3                \n"
						"add    #0xFFFFFFFD,r3          \n"
						"mov.l	@r3,r6                  \n"
						"add #0x00000001,r7									\n"
					);

			while (count >= 32) {
					  __asm("mov.l	@r7+,r0          \n"
							"mov.l	@r7+,r1          \n"
							"mov.l	@r7+,r2          \n"
							"mov.l	@r7+,r3          \n"
							"shld r4, r6				  \n"
							"mov	r0,r5                   \n"
							"shll8   r0                   \n"
							"or      r6,r0                \n"
							"shld r4, r5				  \n"
							"mov	r1,r6                   \n"
							"shll8   r1                   \n"
							"or      r5,r1                \n"
							"shld r4, r6				  \n"
							"mov	r2,r5                   \n"
							"shll8   r2                   \n"
							"or      r6,r2                \n"
							"shld r4, r5				  \n"
							"mov	r3,r6                   \n"
							"shll8   r3                   \n"
							"or      r5,r3                \n"
							"mov.l	 r0,   @r11      \n"
							"mov.l   r1,   @(0x04, r11) \n"
							"mov.l   r2,   @(0x08, r11)   \n"
							"mov.l   r3,   @(0x0C,r11)   \n"
							"mov.l	@r7+,r0          \n"
							"mov.l	@r7+,r1          \n"
							"mov.l	@r7+,r2          \n"
							"mov.l	@r7+,r3          \n"
							"shld r4, r6				  \n"
							"mov	r0,r5                   \n"
							"shll8   r0                   \n"
							"or      r6,r0                \n"
							"shld r4, r5				  \n"
							"mov	r1,r6                   \n"
							"shll8   r1                   \n"
							"or      r5,r1                \n"
							"shld r4, r6				  \n"
							"mov	r2,r5                   \n"
							"shll8   r2                   \n"
							"or      r6,r2                \n"
							"shld r4, r5				  \n"
							"mov	r3,r6                   \n"
							"shll8   r3                   \n"
							"or      r5,r3                \n"
							"mov.l	 r0,   @(0x10,r11)    \n"
							"mov.l   r1,   @(0x14,r11) \n"
							"mov.l   r2,   @(0x18,r11)   \n"
							"mov.l   r3,   @(0x1C,r11)   \n"
							"add	 #0x00000020,r11	\n");
							count = count - 32;
			}

							while(count >= 4) {
							__asm("shld r4, r6		  \n"
							"mov.l	@r7+,r0       \n"
							"mov	r0,r5         \n"
							"shll8   r0           \n"
							"or      r6,r0        \n"
							"mov.l	r0, @r11	  \n"
							"mov	r5,r6         \n"
							"add     #0x00000004,r11      \n");

							count   -= 4;
						}

						__asm("add     #-1,r7      \n");

						while (count-- > 0)
						{
							__asm("mov.b	@r7+,r3              \n"
							"mov.b	r3,@r11               \n"
							"add     #0x00000001,r11      \n"
							);
						}

						count = 0;
						__asm("		mov.l   @r15+,r11    \n" );
          }
          break;

      }
  }

  // byte-copy remaining bytes
	while (count-- > 0)
	{
		*dst1++ = *src1++;
	}

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

/* End of wincememory.c */
