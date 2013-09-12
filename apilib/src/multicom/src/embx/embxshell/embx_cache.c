/*
 * embx_cache.c
 *
 * Copyright (C) STMicroelectronics Limited 2004. All rights reserved.
 *
 * Cache management functions for operating systems with inadequate in-built support.
 */

#include <embx_debug.h>
#include <embx_osinterface.h>

/* this does not form part of debug_ctrl.h since it is so rare that setting
 * this will provide useful debug trace
 */
#ifndef EMBX_INFO_CACHE
#define EMBX_INFO_CACHE 0
#endif

#if defined __OS20__

#include <cache.h>
#define CACHE_SZ 8192


static char scratch_area[CACHE_SZ*3];
/* this symbol is used by optimizing macros in embx_cache.h */
char *embx_cache_scratch_area;

static void cfg_scratch_area(void);
#pragma ST_onstartup(cfg_scratch_area)
static void cfg_scratch_area(void)
{
	/* pre-configure embx_cache_scratch_area so that it points to an array whose length is
	 * greater than CACHE_SZ. 
	 */
	embx_cache_scratch_area = (char *) ((unsigned) &scratch_area | (CACHE_SZ-1)) + 1;

	/* now adjust the pointer so that it actually points 16-bytes before that 'real' scratch
	 * area. this adjustment is performed so that when stshadow is used to preserve the
	 * 2d block move registers it will flush the first two cache lines without requiring any
	 * code.
	 */
	embx_cache_scratch_area -= EMBX_CACHE_SCRATCH_ADJUSTMENT;
}

EMBX_VOID EMBX_OS_PurgeCache(EMBX_VOID *ptr, EMBX_UINT sz)
{
#if __CORE__ == 1
	cache_flush_data(0, 0);
#else /* C2/C4 core */
	char *x;

	/* zero sz would cause the move to go mad */
	if (0 == sz) {
		return;
	}

	/* use the operating system to perform large flushes */
	if (sz > 8*1024) {
		cache_flush_data(0, 0);	
		return;
	}

	/* calculate the starting pointer */
	x = embx_cache_scratch_area + ((unsigned) ptr & 0x1ff0);

	EMBX_Info(EMBX_INFO_CACHE, (">>>embxshm_purge_bytes(%p, %3d) [x = %p]\n", ptr, sz, x));
	
	/* this block of code uses the 2d block move mechanism of the ST20C2 processor to flush
	 * the caches without any loop overhead.
	 */
	__optasm {
		/* store the extant 2d block move registers into the 5 words at x+28 (this is
		 * required for us to be called from interrupt).  due to embx_cache_scratch_area
		 * carefully pre-calculated value this will actually perform 2 of the lines of
		 * flushing we require
		 */
		ldabc	8, 1, x;
		stshadow;

		/* use the 2d block move system to copy the word at x+32 to x+48 
		 * then repeat this with a stride of 32 bytes until we have 
		 * flushed everything we need to.
		 */
		ldabc	((sz-1) / 32), 32, 32;
		move2dinit;
		ldabc	4, x+EMBX_CACHE_SCRATCH_ADJUSTMENT+48, x+EMBX_CACHE_SCRATCH_ADJUSTMENT+32;
		move2dall;

		/* restore the block move registers */
		ldabc	8, 1, x;
		ldshadow;
	}
#endif /* __CORE__ */
}

#endif /* __OS20__ */

#if defined __OS21__ && defined __ST200__

#include <machine/archcore.h>

/* although OS21 includes in-built cache management code it is not as agressively unrolled
 * as the code here and is significantly slower over large blocks.
 */
EMBX_VOID EMBX_OS_PurgeCache(EMBX_VOID *ptr, EMBX_UINT sz)
{
	size_t start = (size_t) ptr;
	size_t end   = start + sz - 1;

	if (0 == sz) {
		return;
	}

	start &= ~(LX_DCACHE_LINE_BYTES - 1);
	end   &= ~(LX_DCACHE_LINE_BYTES - 1);

	GNBvd27351();

	__asm__ __volatile__ (
		       "cmpgeu $b0.7 = %2, %1\n" 
		"	cmpgeu $b0.6 = %3, %1\n"
		"	add	%0 = %3, %4\n"
		"	## prevent overflushing when length is a power of two\n"
		"	goto	purge1\n"
		"	;;\n"
		"purge0:\n"
		"	prgadd	%7[%0]\n"
		"	add	%0 = %0, %4\n"
		"	cmpgeu	$b0.7 = %0, %1\n"
		"	br	$b0.7, purge2\n"
		"	;;\n"
		"	prgadd	%7[%0]\n"
		"	add	%0 = %0, %4\n"
		"	cmpgeu	$b0.6 = %0, %1\n"
		"	br	$b0.6, purge2\n"
		"	;;\n"
		"	prgadd	%7[%0]\n"
		"	br	$b0.5, purge2\n"
		"	;;\n"
		"purge1:\n"
		"	prgadd	%6[%0]\n"
		"	add	%0 = %0, %5\n"
		"	cmpgeu	$b0.5 = %0, %1\n"
		"	goto	purge0\n"
		"	;;\n"
		"purge2:\n"
		"	sync"
		: "=&r" (start)
		: "r" (end), 
		  "r" (start + LX_DCACHE_LINE_BYTES), 
		  "r" (start + 2*LX_DCACHE_LINE_BYTES),
		  "i" (LX_DCACHE_LINE_BYTES),
		  "i" (LX_DCACHE_LINE_BYTES*2),
		  "i" (LX_DCACHE_LINE_BYTES*-3),
		  "i" (LX_DCACHE_LINE_BYTES*-4)
		: "b7", "b6", "b5"
	);
}

#endif /* __OS21__ and __ST200__ */

#if defined WIN32
EMBX_VOID EMBX_OS_PurgeCache(EMBX_VOID *ptr, EMBX_UINT sz)
{
	EMBX_Assert(0);
}
#endif /* WIN32 */
