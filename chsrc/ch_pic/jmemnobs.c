/*
 * jmemnobs.c
 *
 * Copyright (C) 1992-1996, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file provides a really simple implementation of the system-
 * dependent portion of the JPEG memory manager.  This implementation
 * assumes that no backing-store files are needed: all required space
 * can be obtained from malloc().
 * This is very portable in the sense that it'll compile on almost anything,
 * but you'd better have lots of main memory (or virtual memory) if you want
 * to process big images.
 * Note that the max__memory__to__use option is ignored by this implementation.
 */

#define JPEG__INTERNALS
#include "jinclude.h"
#include "jpeglib.h"
#include "jmemsys.h"		/* import the system-dependent declarations */

#ifndef HAVE__STDLIB__H		/* <stdlib.h> should declare malloc(),free() */
extern void * CH_AllocMem JPP((size_t size));
extern void CH_FreeMem JPP((void *ptr));
#endif


/*
 * Memory allocation and freeing are controlled by the regular library
 * routines malloc() and free().
 */

GLOBAL(void *)
jpeg__get__small (j__common__ptr cinfo, size_t sizeofobject)
{
  return (void *) CH_AllocMem(sizeofobject);
}

GLOBAL(void)
jpeg__free__small (j__common__ptr cinfo, void * object, size_t sizeofobject)
{
  CH_FreeMem(object);
}


/*
 * "Large" objects are treated the same as "small" ones.
 * NB: although we include FAR keywords in the routine declarations,
 * this file won't actually work in 80x86 small/medium model; at least,
 * you probably won't be able to process useful-size images in only 64KB.
 */

GLOBAL(void FAR *)
jpeg__get__large (j__common__ptr cinfo, size_t sizeofobject)
{
  return (void FAR *) CH_AllocMem(sizeofobject);
}

GLOBAL(void)
jpeg__free__large (j__common__ptr cinfo, void FAR * object, size_t sizeofobject)
{
  CH_FreeMem(object);
}


/*
 * This routine computes the total memory space available for allocation.
 * Here we always say, "we got all you want bud!"
 */

GLOBAL(long)
jpeg__mem__available (j__common__ptr cinfo, long min__bytes__needed,
		    long max__bytes__needed, long already__allocated)
{
  return max__bytes__needed;
}


/*
 * Backing store (temporary file) management.
 * Since jpeg__mem__available always promised the moon,
 * this should never be called and we can just error out.
 */

GLOBAL(void)
jpeg__open__backing__store (j__common__ptr cinfo, backing__store__ptr info,
			 long total__bytes__needed)
{
  ERREXIT(cinfo, JERR__NO__BACKING__STORE);
}


/*
 * These routines take care of any system-dependent initialization and
 * cleanup required.  Here, there isn't any.
 */

GLOBAL(long)
jpeg__mem__init (j__common__ptr cinfo)
{
  return 0;			/* just set max__memory__to__use to 0 */
}

GLOBAL(void)
jpeg__mem__term (j__common__ptr cinfo)
{
  /* no work */
}
