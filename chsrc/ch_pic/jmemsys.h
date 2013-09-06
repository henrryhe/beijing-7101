/*
 * jmemsys.h
 *
 * Copyright (C) 1992-1997, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This include file defines the interface between the system-independent
 * and system-dependent portions of the JPEG memory manager.  No other
 * modules need include it.  (The system-independent portion is jmemmgr.c;
 * there are several different versions of the system-dependent portion.)
 *
 * This file works as-is for the system-dependent memory managers supplied
 * in the IJG distribution.  You may need to modify it if you write a
 * custom memory manager.  If system-dependent changes are needed in
 * this file, the best method is to #ifdef them based on a configuration
 * symbol supplied in jconfig.h, as we have done with USE__MSDOS__MEMMGR
 * and USE__MAC__MEMMGR.
 */


/* Short forms of external names for systems with brain-damaged linkers. */

#ifdef NEED__SHORT__EXTERNAL__NAMES
#define jpeg__get__small		jGetSmall
#define jpeg__free__small		jFreeSmall
#define jpeg__get__large		jGetLarge
#define jpeg__free__large		jFreeLarge
#define jpeg__mem__available	jMemAvail
#define jpeg__open__backing__store	jOpenBackStore
#define jpeg__mem__init		jMemInit
#define jpeg__mem__term		jMemTerm
#endif /* NEED__SHORT__EXTERNAL__NAMES */


/*
 * These two functions are used to allocate and release small chunks of
 * memory.  (Typically the total amount requested through jpeg__get__small is
 * no more than 20K or so; this will be requested in chunks of a few K each.)
 * Behavior should be the same as for the standard library functions malloc
 * and free; in particular, jpeg__get__small must return NULL on failure.
 * On most systems, these ARE malloc and free.  jpeg__free__small is passed the
 * size of the object being freed, just in case it's needed.
 * On an 80x86 machine using small-data memory model, these manage near heap.
 */

EXTERN(void *) jpeg__get__small JPP((j__common__ptr cinfo, size_t sizeofobject));
EXTERN(void) jpeg__free__small JPP((j__common__ptr cinfo, void * object,
				  size_t sizeofobject));

/*
 * These two functions are used to allocate and release large chunks of
 * memory (up to the total free space designated by jpeg__mem__available).
 * The interface is the same as above, except that on an 80x86 machine,
 * far pointers are used.  On most other machines these are identical to
 * the jpeg__get/free__small routines; but we keep them separate anyway,
 * in case a different allocation strategy is desirable for large chunks.
 */

EXTERN(void FAR *) jpeg__get__large JPP((j__common__ptr cinfo,
				       size_t sizeofobject));
EXTERN(void) jpeg__free__large JPP((j__common__ptr cinfo, void FAR * object,
				  size_t sizeofobject));

/*
 * The macro MAX__ALLOC__CHUNK designates the maximum number of bytes that may
 * be requested in a single call to jpeg__get__large (and jpeg__get__small for that
 * matter, but that case should never come into play).  This macro is needed
 * to model the 64Kb-segment-size limit of far addressing on 80x86 machines.
 * On those machines, we expect that jconfig.h will provide a proper value.
 * On machines with 32-bit flat address spaces, any large constant may be used.
 *
 * NB: jmemmgr.c expects that MAX__ALLOC__CHUNK will be representable as type
 * size_t and will be a multiple of sizeof(align__type).
 */

#ifndef MAX__ALLOC__CHUNK		/* may be overridden in jconfig.h */
#define MAX__ALLOC__CHUNK  1000000000L
#endif

/*
 * This routine computes the total space still available for allocation by
 * jpeg__get__large.  If more space than this is needed, backing store will be
 * used.  NOTE: any memory already allocated must not be counted.
 *
 * There is a minimum space requirement, corresponding to the minimum
 * feasible buffer sizes; jmemmgr.c will request that much space even if
 * jpeg__mem__available returns zero.  The maximum space needed, enough to hold
 * all working storage in memory, is also passed in case it is useful.
 * Finally, the total space already allocated is passed.  If no better
 * method is available, cinfo->mem->max__memory__to__use - already__allocated
 * is often a suitable calculation.
 *
 * It is OK for jpeg__mem__available to underestimate the space available
 * (that'll just lead to more backing-store access than is really necessary).
 * However, an overestimate will lead to failure.  Hence it's wise to subtract
 * a slop factor from the true available space.  5% should be enough.
 *
 * On machines with lots of virtual memory, any large constant may be returned.
 * Conversely, zero may be returned to always use the minimum amount of memory.
 */

EXTERN(long) jpeg__mem__available JPP((j__common__ptr cinfo,
				     long min__bytes__needed,
				     long max__bytes__needed,
				     long already__allocated));


/*
 * This structure holds whatever state is needed to access a single
 * backing-store object.  The read/write/close method pointers are called
 * by jmemmgr.c to manipulate the backing-store object; all other fields
 * are private to the system-dependent backing store routines.
 */

#define TEMP__NAME__LENGTH   64	/* max length of a temporary file's name */


#ifdef USE__MSDOS__MEMMGR		/* DOS-specific junk */

typedef unsigned short XMSH;	/* type of extended-memory handles */
typedef unsigned short EMSH;	/* type of expanded-memory handles */

typedef union {
  short file__handle;		/* DOS file handle if it's a temp file */
  XMSH xms__handle;		/* handle if it's a chunk of XMS */
  EMSH ems__handle;		/* handle if it's a chunk of EMS */
} handle__union;

#endif /* USE__MSDOS__MEMMGR */

#ifdef USE__MAC__MEMMGR		/* Mac-specific junk */
#include <Files.h>
#endif /* USE__MAC__MEMMGR */


typedef struct backing__store__struct * backing__store__ptr;

typedef struct backing__store__struct {
  /* Methods for reading/writing/closing this backing-store object */
  JMETHOD(void, read__backing__store, (j__common__ptr cinfo,
				     backing__store__ptr info,
				     void FAR * buffer__address,
				     long file__offset, long byte__count));
  JMETHOD(void, write__backing__store, (j__common__ptr cinfo,
				      backing__store__ptr info,
				      void FAR * buffer__address,
				      long file__offset, long byte__count));
  JMETHOD(void, close__backing__store, (j__common__ptr cinfo,
				      backing__store__ptr info));

  /* Private fields for system-dependent backing-store management */
#ifdef USE__MSDOS__MEMMGR
  /* For the MS-DOS manager (jmemdos.c), we need: */
  handle__union handle;		/* reference to backing-store storage object */
  char temp__name[TEMP__NAME__LENGTH]; /* name if it's a file */
#else
#ifdef USE__MAC__MEMMGR
  /* For the Mac manager (jmemmac.c), we need: */
  short temp__file;		/* file reference number to temp file */
  FSSpec tempSpec;		/* the FSSpec for the temp file */
  char temp__name[TEMP__NAME__LENGTH]; /* name if it's a file */
#else
  /* For a typical implementation with temp files, we need: */
  graph_file * temp__file;		/* stdio reference to temp file */
  char temp__name[TEMP__NAME__LENGTH]; /* name of temp file */
#endif
#endif
} backing__store__info;


/*
 * Initial opening of a backing-store object.  This must fill in the
 * read/write/close pointers in the object.  The read/write routines
 * may take an error exit if the specified maximum file size is exceeded.
 * (If jpeg__mem__available always returns a large value, this routine can
 * just take an error exit.)
 */

EXTERN(void) jpeg__open__backing__store JPP((j__common__ptr cinfo,
					  backing__store__ptr info,
					  long total__bytes__needed));


/*
 * These routines take care of any system-dependent initialization and
 * cleanup required.  jpeg__mem__init will be called before anything is
 * allocated (and, therefore, nothing in cinfo is of use except the error
 * manager pointer).  It should return a suitable default value for
 * max__memory__to__use; this may subsequently be overridden by the surrounding
 * application.  (Note that max__memory__to__use is only important if
 * jpeg__mem__available chooses to consult it ... no one else will.)
 * jpeg__mem__term may assume that all requested memory has been freed and that
 * all opened backing-store objects have been closed.
 */

EXTERN(long) jpeg__mem__init JPP((j__common__ptr cinfo));
EXTERN(void) jpeg__mem__term JPP((j__common__ptr cinfo));
