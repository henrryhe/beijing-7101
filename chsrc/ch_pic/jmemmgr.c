/*
 * jmemmgr.c
 *
 * Copyright (C) 1991-1997, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains the JPEG system-independent memory management
 * routines.  This code is usable across a wide variety of machines; most
 * of the system dependencies have been isolated in a separate file.
 * The major functions provided here are:
 *   * pool-based allocation and freeing of memory;
 *   * policy decisions about how to divide available memory among the
 *     virtual arrays;
 *   * control logic for swapping virtual arrays between main memory and
 *     backing storage.
 * The separate system-dependent file provides the actual backing-storage
 * access code, and it contains the policy decision about how much total
 * main memory to use.
 * This file is system-dependent in the sense that some of its functions
 * are unnecessary in some systems.  For example, if there is enough virtual
 * memory so that backing storage will never be used, much of the virtual
 * array control logic could be removed.  (Of course, if you have that much
 * memory then you shouldn't care about a little bit of unused code...)
 */

#define JPEG__INTERNALS
#define AM__MEMORY__MANAGER	/* we define jvirt__Xarray__control structs */
#include "jinclude.h"
#include "jpeglib.h"
#include "jmemsys.h"		/* import the system-dependent declarations */

#define NO__GETENV  /*sqzow added 2006/09/21*/

#ifndef NO__GETENV
#ifndef HAVE__STDLIB__H		/* <stdlib.h> should declare getenv() */
extern char * getenv JPP((const char * name));
#endif
#endif


/*
 * Some important notes:
 *   The allocation routines provided here must never return NULL.
 *   They should exit to error__exit if unsuccessful.
 *
 *   It's not a good idea to try to merge the sarray and barray routines,
 *   even though they are textually almost the same, because samples are
 *   usually stored as bytes while coefficients are shorts or ints.  Thus,
 *   in machines where byte pointers have a different representation from
 *   word pointers, the resulting machine code could not be the same.
 */


/*
 * Many machines require storage alignment: longs must start on 4-byte
 * boundaries, doubles on 8-byte boundaries, etc.  On such machines, malloc()
 * always returns pointers that are multiples of the worst-case alignment
 * requirement, and we had better do so too.
 * There isn't any really portable way to determine the worst-case alignment
 * requirement.  This module assumes that the alignment requirement is
 * multiples of sizeof(ALIGN__TYPE).
 * By default, we define ALIGN__TYPE as double.  This is necessary on some
 * workstations (where doubles really do need 8-byte alignment) and will work
 * fine on nearly everything.  If your machine has lesser alignment needs,
 * you can save a few bytes by making ALIGN__TYPE smaller.
 * The only place I know of where this will NOT work is certain Macintosh
 * 680x0 compilers that define double as a 10-byte IEEE extended float.
 * Doing 10-byte alignment is counterproductive because longwords won't be
 * aligned well.  Put "#define ALIGN__TYPE long" in jconfig.h if you have
 * such a compiler.
 */

#ifndef ALIGN__TYPE		/* so can override from jconfig.h */
#define ALIGN__TYPE  double
#endif


/*
 * We allocate objects from "pools", where each pool is gotten with a single
 * request to jpeg__get__small() or jpeg__get__large().  There is no per-object
 * overhead within a pool, except for alignment padding.  Each pool has a
 * header with a link to the next pool of the same class.
 * Small and large pool headers are identical except that the latter's
 * link pointer must be FAR on 80x86 machines.
 * Notice that the "real" header fields are union'ed with a dummy ALIGN__TYPE
 * field.  This forces the compiler to make SIZEOF(small__pool__hdr) a multiple
 * of the alignment requirement of ALIGN__TYPE.
 */

typedef union small__pool__struct * small__pool__ptr;

typedef union small__pool__struct {
  struct {
    small__pool__ptr next;	/* next in list of pools */
    size_t bytes__used;		/* how many bytes already used within pool */
    size_t bytes__left;		/* bytes still available in this pool */
  } hdr;
  ALIGN__TYPE dummy;		/* included in union to ensure alignment */
} small__pool__hdr;

typedef union large__pool__struct FAR * large__pool__ptr;

typedef union large__pool__struct {
  struct {
    large__pool__ptr next;	/* next in list of pools */
    size_t bytes__used;		/* how many bytes already used within pool */
    size_t bytes__left;		/* bytes still available in this pool */
  } hdr;
  ALIGN__TYPE dummy;		/* included in union to ensure alignment */
} large__pool__hdr;


/*
 * Here is the full definition of a memory manager object.
 */

typedef struct {
  struct jpeg__memory__mgr pub;	/* public fields */

  /* Each pool identifier (lifetime class) names a linked list of pools. */
  small__pool__ptr small__list[JPOOL__NUMPOOLS];
  large__pool__ptr large__list[JPOOL__NUMPOOLS];

  /* Since we only have one lifetime class of virtual arrays, only one
   * linked list is necessary (for each datatype).  Note that the virtual
   * array control blocks being linked together are actually stored somewhere
   * in the small-pool list.
   */
  jvirt__sarray__ptr virt__sarray__list;
  jvirt__barray__ptr virt__barray__list;

  /* This counts total space obtained from jpeg__get__small/large */
  long total__space__allocated;

  /* alloc__sarray and alloc__barray set this value for use by virtual
   * array routines.
   */
  JDIMENSION last__rowsperchunk;	/* from most recent alloc__sarray/barray */
} my__memory__mgr;

typedef my__memory__mgr * my__mem__ptr;


/*
 * The control blocks for virtual arrays.
 * Note that these blocks are allocated in the "small" pool area.
 * System-dependent info for the associated backing store (if any) is hidden
 * inside the backing__store__info struct.
 */

struct jvirt__sarray__control {
  JSAMPARRAY mem__buffer;	/* => the in-memory buffer */
  JDIMENSION rows__in__array;	/* total virtual array height */
  JDIMENSION samplesperrow;	/* width of array (and of memory buffer) */
  JDIMENSION maxaccess;		/* max rows accessed by access__virt__sarray */
  JDIMENSION rows__in__mem;	/* height of memory buffer */
  JDIMENSION rowsperchunk;	/* allocation chunk size in mem__buffer */
  JDIMENSION cur__start__row;	/* first logical row # in the buffer */
  JDIMENSION first__undef__row;	/* row # of first uninitialized row */
  CH_BOOL pre__zero;		/* pre-zero mode requested? */
  CH_BOOL dirty;		/* do current buffer contents need written? */
  CH_BOOL b__s__open;		/* is backing-store data valid? */
  jvirt__sarray__ptr next;	/* link to next virtual sarray control block */
  backing__store__info b__s__info;	/* System-dependent control info */
};

struct jvirt__barray__control {
  JBLOCKARRAY mem__buffer;	/* => the in-memory buffer */
  JDIMENSION rows__in__array;	/* total virtual array height */
  JDIMENSION blocksperrow;	/* width of array (and of memory buffer) */
  JDIMENSION maxaccess;		/* max rows accessed by access__virt__barray */
  JDIMENSION rows__in__mem;	/* height of memory buffer */
  JDIMENSION rowsperchunk;	/* allocation chunk size in mem__buffer */
  JDIMENSION cur__start__row;	/* first logical row # in the buffer */
  JDIMENSION first__undef__row;	/* row # of first uninitialized row */
  CH_BOOL pre__zero;		/* pre-zero mode requested? */
  CH_BOOL dirty;		/* do current buffer contents need written? */
  CH_BOOL b__s__open;		/* is backing-store data valid? */
  jvirt__barray__ptr next;	/* link to next virtual barray control block */
  backing__store__info b__s__info;	/* System-dependent control info */
};


#ifdef MEM__STATS		/* optional extra stuff for statistics */

LOCAL(void)
print__mem__stats (j__common__ptr cinfo, int pool__id)
{
  my__mem__ptr mem = (my__mem__ptr) cinfo->mem;
  small__pool__ptr shdr__ptr;
  large__pool__ptr lhdr__ptr;

  /* Since this is only a debugging stub, we can cheat a little by using
   * fprintf directly rather than going through the trace message code.
   * This is helpful because message parm array can't handle longs.
   */
  fprintf(stderr, "Freeing pool %d, total space = %ld\n",
	  pool__id, mem->total__space__allocated);

  for (lhdr__ptr = mem->large__list[pool__id]; lhdr__ptr != NULL;
       lhdr__ptr = lhdr__ptr->hdr.next) {
    fprintf(stderr, "  Large chunk used %ld\n",
	    (long) lhdr__ptr->hdr.bytes__used);
  }

  for (shdr__ptr = mem->small__list[pool__id]; shdr__ptr != NULL;
       shdr__ptr = shdr__ptr->hdr.next) {
    fprintf(stderr, "  Small chunk used %ld free %ld\n",
	    (long) shdr__ptr->hdr.bytes__used,
	    (long) shdr__ptr->hdr.bytes__left);
  }
}

#endif /* MEM__STATS */


LOCAL(void)
out__of__memory (j__common__ptr cinfo, int which)
/* Report an out-of-memory error and stop execution */
/* If we compiled MEM__STATS support, report alloc requests before dying */
{
#ifdef MEM__STATS
  cinfo->err->trace__level = 2;	/* force self__destruct to report stats */
#endif
  ERREXIT1(cinfo, JERR__OUT__OF__MEMORY, which);
}


/*
 * Allocation of "small" objects.
 *
 * For these, we use pooled storage.  When a new pool must be created,
 * we try to get enough space for the current request plus a "slop" factor,
 * where the slop will be the amount of leftover space in the new pool.
 * The speed vs. space tradeoff is largely determined by the slop values.
 * A different slop value is provided for each pool class (lifetime),
 * and we also distinguish the first pool of a class from later ones.
 * NOTE: the values given work fairly well on both 16- and 32-bit-int
 * machines, but may be too small if longs are 64 bits or more.
 */

static const size_t first__pool__slop[JPOOL__NUMPOOLS] = 
{
	1600,			/* first PERMANENT pool */
	16000			/* first IMAGE pool */
};

static const size_t extra__pool__slop[JPOOL__NUMPOOLS] = 
{
	0,			/* additional PERMANENT pools */
	5000			/* additional IMAGE pools */
};

#define MIN__SLOP  50		/* greater than 0 to avoid futile looping */


METHODDEF(void *)
alloc__small (j__common__ptr cinfo, int pool__id, size_t sizeofobject)
/* Allocate a "small" object */
{
  my__mem__ptr mem = (my__mem__ptr) cinfo->mem;
  small__pool__ptr hdr__ptr, prev__hdr__ptr;
  char * data__ptr;
  size_t odd__bytes, min__request, slop;

  /* Check for unsatisfiable request (do now to ensure no overflow below) */
  if (sizeofobject > (size_t) (MAX__ALLOC__CHUNK-SIZEOF(small__pool__hdr)))
    out__of__memory(cinfo, 1);	/* request exceeds malloc's ability */

  /* Round up the requested size to a multiple of SIZEOF(ALIGN__TYPE) */
  odd__bytes = sizeofobject % SIZEOF(ALIGN__TYPE);
  if (odd__bytes > 0)
    sizeofobject += SIZEOF(ALIGN__TYPE) - odd__bytes;

  /* See if space is available in any existing pool */
  if (pool__id < 0 || pool__id >= JPOOL__NUMPOOLS)
    ERREXIT1(cinfo, JERR__BAD__POOL__ID, pool__id);	/* safety check */
  prev__hdr__ptr = NULL;
  hdr__ptr = mem->small__list[pool__id];
  while (hdr__ptr != NULL) {
    if (hdr__ptr->hdr.bytes__left >= sizeofobject)
      break;			/* found pool with enough space */
    prev__hdr__ptr = hdr__ptr;
    hdr__ptr = hdr__ptr->hdr.next;
  }

  /* Time to make a new pool? */
  if (hdr__ptr == NULL) {
    /* min__request is what we need now, slop is what will be leftover */
    min__request = sizeofobject + SIZEOF(small__pool__hdr);
    if (prev__hdr__ptr == NULL)	/* first pool in class? */
      slop = first__pool__slop[pool__id];
    else
      slop = extra__pool__slop[pool__id];
    /* Don't ask for more than MAX__ALLOC__CHUNK */
    if (slop > (size_t) (MAX__ALLOC__CHUNK-min__request))
      slop = (size_t) (MAX__ALLOC__CHUNK-min__request);
    /* Try to get space, if fail reduce slop and try again */
    for (;;) {
      hdr__ptr = (small__pool__ptr) jpeg__get__small(cinfo, min__request + slop);
      if (hdr__ptr != NULL)
	break;
      slop /= 2;
      if (slop < MIN__SLOP)	/* give up when it gets real small */
	out__of__memory(cinfo, 2); /* jpeg__get__small failed */
    }
    mem->total__space__allocated += min__request + slop;
    /* Success, initialize the new pool header and add to end of list */
    hdr__ptr->hdr.next = NULL;
    hdr__ptr->hdr.bytes__used = 0;
    hdr__ptr->hdr.bytes__left = sizeofobject + slop;
    if (prev__hdr__ptr == NULL)	/* first pool in class? */
      mem->small__list[pool__id] = hdr__ptr;
    else
      prev__hdr__ptr->hdr.next = hdr__ptr;
  }

  /* OK, allocate the object from the current pool */
  data__ptr = (char *) (hdr__ptr + 1); /* point to first data byte in pool */
  data__ptr += hdr__ptr->hdr.bytes__used; /* point to place for object */
  hdr__ptr->hdr.bytes__used += sizeofobject;
  hdr__ptr->hdr.bytes__left -= sizeofobject;

  return (void *) data__ptr;
}


/*
 * Allocation of "large" objects.
 *
 * The external semantics of these are the same as "small" objects,
 * except that FAR pointers are used on 80x86.  However the pool
 * management heuristics are quite different.  We assume that each
 * request is large enough that it may as well be passed directly to
 * jpeg__get__large; the pool management just links everything together
 * so that we can free it all on demand.
 * Note: the major use of "large" objects is in JSAMPARRAY and JBLOCKARRAY
 * structures.  The routines that create these structures (see below)
 * deliberately bunch rows together to ensure a large request size.
 */

METHODDEF(void FAR *)
alloc__large (j__common__ptr cinfo, int pool__id, size_t sizeofobject)
/* Allocate a "large" object */
{
  my__mem__ptr mem = (my__mem__ptr) cinfo->mem;
  large__pool__ptr hdr__ptr;
  size_t odd__bytes;

  /* Check for unsatisfiable request (do now to ensure no overflow below) */
  if (sizeofobject > (size_t) (MAX__ALLOC__CHUNK-SIZEOF(large__pool__hdr)))
    out__of__memory(cinfo, 3);	/* request exceeds malloc's ability */

  /* Round up the requested size to a multiple of SIZEOF(ALIGN__TYPE) */
  odd__bytes = sizeofobject % SIZEOF(ALIGN__TYPE);
  if (odd__bytes > 0)
    sizeofobject += SIZEOF(ALIGN__TYPE) - odd__bytes;

  /* Always make a new pool */
  if (pool__id < 0 || pool__id >= JPOOL__NUMPOOLS)
    ERREXIT1(cinfo, JERR__BAD__POOL__ID, pool__id);	/* safety check */

  hdr__ptr = (large__pool__ptr) jpeg__get__large(cinfo, sizeofobject +
					    SIZEOF(large__pool__hdr));
  if (hdr__ptr == NULL)
    out__of__memory(cinfo, 4);	/* jpeg__get__large failed */
  mem->total__space__allocated += sizeofobject + SIZEOF(large__pool__hdr);

  /* Success, initialize the new pool header and add to list */
  hdr__ptr->hdr.next = mem->large__list[pool__id];
  /* We maintain space counts in each pool header for statistical purposes,
   * even though they are not needed for allocation.
   */
  hdr__ptr->hdr.bytes__used = sizeofobject;
  hdr__ptr->hdr.bytes__left = 0;
  mem->large__list[pool__id] = hdr__ptr;

  return (void FAR *) (hdr__ptr + 1); /* point to first data byte in pool */
}


/*
 * Creation of 2-D sample arrays.
 * The pointers are in near heap, the samples themselves in FAR heap.
 *
 * To minimize allocation overhead and to allow I/O of large contiguous
 * blocks, we allocate the sample rows in groups of as many rows as possible
 * without exceeding MAX__ALLOC__CHUNK total bytes per allocation request.
 * NB: the virtual array control routines, later in this file, know about
 * this chunking of rows.  The rowsperchunk value is left in the mem manager
 * object so that it can be saved away if this sarray is the workspace for
 * a virtual array.
 */

METHODDEF(JSAMPARRAY)
alloc__sarray (j__common__ptr cinfo, int pool__id,
	      JDIMENSION samplesperrow, JDIMENSION numrows)
/* Allocate a 2-D sample array */
{
  my__mem__ptr mem = (my__mem__ptr) cinfo->mem;
  JSAMPARRAY result;
  JSAMPROW workspace;
  JDIMENSION rowsperchunk, currow, i;
  long ltemp;

  /* Calculate max # of rows allowed in one allocation chunk */
  ltemp = (MAX__ALLOC__CHUNK-SIZEOF(large__pool__hdr)) /
	  ((long) samplesperrow * SIZEOF(JSAMPLE));
  if (ltemp <= 0)
    ERREXIT(cinfo, JERR__WIDTH__OVERFLOW);
  if (ltemp < (long) numrows)
    rowsperchunk = (JDIMENSION) ltemp;
  else
    rowsperchunk = numrows;
  mem->last__rowsperchunk = rowsperchunk;

  /* Get space for row pointers (small object) */
  result = (JSAMPARRAY) alloc__small(cinfo, pool__id,
				    (size_t) (numrows * SIZEOF(JSAMPROW)));

  /* Get the rows themselves (large objects) */
  currow = 0;
  while (currow < numrows) {
    rowsperchunk = MIN(rowsperchunk, numrows - currow);
    workspace = (JSAMPROW) alloc__large(cinfo, pool__id,
	(size_t) ((size_t) rowsperchunk * (size_t) samplesperrow
		  * SIZEOF(JSAMPLE)));
    for (i = rowsperchunk; i > 0; i--) {
      result[currow++] = workspace;
      workspace += samplesperrow;
    }
  }

  return result;
}


/*
 * Creation of 2-D coefficient-block arrays.
 * This is essentially the same as the code for sample arrays, above.
 */

METHODDEF(JBLOCKARRAY)
alloc__barray (j__common__ptr cinfo, int pool__id,
	      JDIMENSION blocksperrow, JDIMENSION numrows)
/* Allocate a 2-D coefficient-block array */
{
  my__mem__ptr mem = (my__mem__ptr) cinfo->mem;
  JBLOCKARRAY result;
  JBLOCKROW workspace;
  JDIMENSION rowsperchunk, currow, i;
  long ltemp;

  /* Calculate max # of rows allowed in one allocation chunk */
  ltemp = (MAX__ALLOC__CHUNK-SIZEOF(large__pool__hdr)) /
	  ((long) blocksperrow * SIZEOF(JBLOCK));
  if (ltemp <= 0)
    ERREXIT(cinfo, JERR__WIDTH__OVERFLOW);
  if (ltemp < (long) numrows)
    rowsperchunk = (JDIMENSION) ltemp;
  else
    rowsperchunk = numrows;
  mem->last__rowsperchunk = rowsperchunk;

  /* Get space for row pointers (small object) */
  result = (JBLOCKARRAY) alloc__small(cinfo, pool__id,
				     (size_t) (numrows * SIZEOF(JBLOCKROW)));

  /* Get the rows themselves (large objects) */
  currow = 0;
  while (currow < numrows) {
    rowsperchunk = MIN(rowsperchunk, numrows - currow);
    workspace = (JBLOCKROW) alloc__large(cinfo, pool__id,
	(size_t) ((size_t) rowsperchunk * (size_t) blocksperrow
		  * SIZEOF(JBLOCK)));
    for (i = rowsperchunk; i > 0; i--) {
      result[currow++] = workspace;
      workspace += blocksperrow;
    }
  }

  return result;
}


/*
 * About virtual array management:
 *
 * The above "normal" array routines are only used to allocate strip buffers
 * (as wide as the image, but just a few rows high).  Full-image-sized buffers
 * are handled as "virtual" arrays.  The array is still accessed a strip at a
 * time, but the memory manager must save the whole array for repeated
 * accesses.  The intended implementation is that there is a strip buffer in
 * memory (as high as is possible given the desired memory limit), plus a
 * backing file that holds the rest of the array.
 *
 * The request__virt__array routines are told the total size of the image and
 * the maximum number of rows that will be accessed at once.  The in-memory
 * buffer must be at least as large as the maxaccess value.
 *
 * The request routines create control blocks but not the in-memory buffers.
 * That is postponed until realize__virt__arrays is called.  At that time the
 * total amount of space needed is known (approximately, anyway), so free
 * memory can be divided up fairly.
 *
 * The access__virt__array routines are responsible for making a specific strip
 * area accessible (after reading or writing the backing file, if necessary).
 * Note that the access routines are told whether the caller intends to modify
 * the accessed strip; during a read-only pass this saves having to rewrite
 * data to disk.  The access routines are also responsible for pre-zeroing
 * any newly accessed rows, if pre-zeroing was requested.
 *
 * In current usage, the access requests are usually for nonoverlapping
 * strips; that is, successive access start__row numbers differ by exactly
 * num__rows = maxaccess.  This means we can get good performance with simple
 * buffer dump/reload logic, by making the in-memory buffer be a multiple
 * of the access height; then there will never be accesses across bufferload
 * boundaries.  The code will still work with overlapping access requests,
 * but it doesn't handle bufferload overlaps very efficiently.
 */


METHODDEF(jvirt__sarray__ptr)
request__virt__sarray (j__common__ptr cinfo, int pool__id, CH_BOOL pre__zero,
		     JDIMENSION samplesperrow, JDIMENSION numrows,
		     JDIMENSION maxaccess)
/* Request a virtual 2-D sample array */
{
  my__mem__ptr mem = (my__mem__ptr) cinfo->mem;
  jvirt__sarray__ptr result;

  /* Only IMAGE-lifetime virtual arrays are currently supported */
  if (pool__id != JPOOL__IMAGE)
    ERREXIT1(cinfo, JERR__BAD__POOL__ID, pool__id);	/* safety check */

  /* get control block */
  result = (jvirt__sarray__ptr) alloc__small(cinfo, pool__id,
					  SIZEOF(struct jvirt__sarray__control));

  result->mem__buffer = NULL;	/* marks array not yet realized */
  result->rows__in__array = numrows;
  result->samplesperrow = samplesperrow;
  result->maxaccess = maxaccess;
  result->pre__zero = pre__zero;
  result->b__s__open = FALSE;	/* no associated backing-store object */
  result->next = mem->virt__sarray__list; /* add to list of virtual arrays */
  mem->virt__sarray__list = result;

  return result;
}


METHODDEF(jvirt__barray__ptr)
request__virt__barray (j__common__ptr cinfo, int pool__id, CH_BOOL pre__zero,
		     JDIMENSION blocksperrow, JDIMENSION numrows,
		     JDIMENSION maxaccess)
/* Request a virtual 2-D coefficient-block array */
{
  my__mem__ptr mem = (my__mem__ptr) cinfo->mem;
  jvirt__barray__ptr result;

  /* Only IMAGE-lifetime virtual arrays are currently supported */
  if (pool__id != JPOOL__IMAGE)
    ERREXIT1(cinfo, JERR__BAD__POOL__ID, pool__id);	/* safety check */

  /* get control block */
  result = (jvirt__barray__ptr) alloc__small(cinfo, pool__id,
					  SIZEOF(struct jvirt__barray__control));

  result->mem__buffer = NULL;	/* marks array not yet realized */
  result->rows__in__array = numrows;
  result->blocksperrow = blocksperrow;
  result->maxaccess = maxaccess;
  result->pre__zero = pre__zero;
  result->b__s__open = FALSE;	/* no associated backing-store object */
  result->next = mem->virt__barray__list; /* add to list of virtual arrays */
  mem->virt__barray__list = result;

  return result;
}


METHODDEF(void)
realize__virt__arrays (j__common__ptr cinfo)
/* Allocate the in-memory buffers for any unrealized virtual arrays */
{
  my__mem__ptr mem = (my__mem__ptr) cinfo->mem;
  long space__per__minheight, maximum__space, avail__mem;
  long minheights, max__minheights;
  jvirt__sarray__ptr sptr;
  jvirt__barray__ptr bptr;

  /* Compute the minimum space needed (maxaccess rows in each buffer)
   * and the maximum space needed (full image height in each buffer).
   * These may be of use to the system-dependent jpeg__mem__available routine.
   */
  space__per__minheight = 0;
  maximum__space = 0;
  for (sptr = mem->virt__sarray__list; sptr != NULL; sptr = sptr->next) {
    if (sptr->mem__buffer == NULL) { /* if not realized yet */
      space__per__minheight += (long) sptr->maxaccess *
			     (long) sptr->samplesperrow * SIZEOF(JSAMPLE);
      maximum__space += (long) sptr->rows__in__array *
		       (long) sptr->samplesperrow * SIZEOF(JSAMPLE);
    }
  }
  for (bptr = mem->virt__barray__list; bptr != NULL; bptr = bptr->next) {
    if (bptr->mem__buffer == NULL) { /* if not realized yet */
      space__per__minheight += (long) bptr->maxaccess *
			     (long) bptr->blocksperrow * SIZEOF(JBLOCK);
      maximum__space += (long) bptr->rows__in__array *
		       (long) bptr->blocksperrow * SIZEOF(JBLOCK);
    }
  }

  if (space__per__minheight <= 0)
    return;			/* no unrealized arrays, no work */

  /* Determine amount of memory to actually use; this is system-dependent. */
  avail__mem = jpeg__mem__available(cinfo, space__per__minheight, maximum__space,
				 mem->total__space__allocated);

  /* If the maximum space needed is available, make all the buffers full
   * height; otherwise parcel it out with the same number of minheights
   * in each buffer.
   */
  if (avail__mem >= maximum__space)
    max__minheights = 1000000000L;
  else {
    max__minheights = avail__mem / space__per__minheight;
    /* If there doesn't seem to be enough space, try to get the minimum
     * anyway.  This allows a "stub" implementation of jpeg__mem__available().
     */
    if (max__minheights <= 0)
      max__minheights = 1;
  }

  /* Allocate the in-memory buffers and initialize backing store as needed. */

  for (sptr = mem->virt__sarray__list; sptr != NULL; sptr = sptr->next) {
    if (sptr->mem__buffer == NULL) { /* if not realized yet */
      minheights = ((long) sptr->rows__in__array - 1L) / sptr->maxaccess + 1L;
      if (minheights <= max__minheights) {
	/* This buffer fits in memory */
	sptr->rows__in__mem = sptr->rows__in__array;
      } else {
	/* It doesn't fit in memory, create backing store. */
	sptr->rows__in__mem = (JDIMENSION) (max__minheights * sptr->maxaccess);
	jpeg__open__backing__store(cinfo, & sptr->b__s__info,
				(long) sptr->rows__in__array *
				(long) sptr->samplesperrow *
				(long) SIZEOF(JSAMPLE));
	sptr->b__s__open = TRUE;
      }
      sptr->mem__buffer = alloc__sarray(cinfo, JPOOL__IMAGE,
				      sptr->samplesperrow, sptr->rows__in__mem);
      sptr->rowsperchunk = mem->last__rowsperchunk;
      sptr->cur__start__row = 0;
      sptr->first__undef__row = 0;
      sptr->dirty = FALSE;
    }
  }

  for (bptr = mem->virt__barray__list; bptr != NULL; bptr = bptr->next) {
    if (bptr->mem__buffer == NULL) { /* if not realized yet */
      minheights = ((long) bptr->rows__in__array - 1L) / bptr->maxaccess + 1L;
      if (minheights <= max__minheights) {
	/* This buffer fits in memory */
	bptr->rows__in__mem = bptr->rows__in__array;
      } else {
	/* It doesn't fit in memory, create backing store. */
	bptr->rows__in__mem = (JDIMENSION) (max__minheights * bptr->maxaccess);
	jpeg__open__backing__store(cinfo, & bptr->b__s__info,
				(long) bptr->rows__in__array *
				(long) bptr->blocksperrow *
				(long) SIZEOF(JBLOCK));
	bptr->b__s__open = TRUE;
      }
      bptr->mem__buffer = alloc__barray(cinfo, JPOOL__IMAGE,
				      bptr->blocksperrow, bptr->rows__in__mem);
      bptr->rowsperchunk = mem->last__rowsperchunk;
      bptr->cur__start__row = 0;
      bptr->first__undef__row = 0;
      bptr->dirty = FALSE;
    }
  }
}


LOCAL(void)
do__sarray__io (j__common__ptr cinfo, jvirt__sarray__ptr ptr, CH_BOOL writing)
/* Do backing store read or write of a virtual sample array */
{
  long bytesperrow, file__offset, byte__count, rows, thisrow, i;

  bytesperrow = (long) ptr->samplesperrow * SIZEOF(JSAMPLE);
  file__offset = ptr->cur__start__row * bytesperrow;
  /* Loop to read or write each allocation chunk in mem__buffer */
  for (i = 0; i < (long) ptr->rows__in__mem; i += ptr->rowsperchunk) {
    /* One chunk, but check for short chunk at end of buffer */
    rows = MIN((long) ptr->rowsperchunk, (long) ptr->rows__in__mem - i);
    /* Transfer no more than is currently defined */
    thisrow = (long) ptr->cur__start__row + i;
    rows = MIN(rows, (long) ptr->first__undef__row - thisrow);
    /* Transfer no more than fits in file */
    rows = MIN(rows, (long) ptr->rows__in__array - thisrow);
    if (rows <= 0)		/* this chunk might be past end of file! */
      break;
    byte__count = rows * bytesperrow;
    if (writing)
      (*ptr->b__s__info.write__backing__store) (cinfo, & ptr->b__s__info,
					    (void FAR *) ptr->mem__buffer[i],
					    file__offset, byte__count);
    else
      (*ptr->b__s__info.read__backing__store) (cinfo, & ptr->b__s__info,
					   (void FAR *) ptr->mem__buffer[i],
					   file__offset, byte__count);
    file__offset += byte__count;
  }
}


LOCAL(void)
do__barray__io (j__common__ptr cinfo, jvirt__barray__ptr ptr, CH_BOOL writing)
/* Do backing store read or write of a virtual coefficient-block array */
{
  long bytesperrow, file__offset, byte__count, rows, thisrow, i;

  bytesperrow = (long) ptr->blocksperrow * SIZEOF(JBLOCK);
  file__offset = ptr->cur__start__row * bytesperrow;
  /* Loop to read or write each allocation chunk in mem__buffer */
  for (i = 0; i < (long) ptr->rows__in__mem; i += ptr->rowsperchunk) {
    /* One chunk, but check for short chunk at end of buffer */
    rows = MIN((long) ptr->rowsperchunk, (long) ptr->rows__in__mem - i);
    /* Transfer no more than is currently defined */
    thisrow = (long) ptr->cur__start__row + i;
    rows = MIN(rows, (long) ptr->first__undef__row - thisrow);
    /* Transfer no more than fits in file */
    rows = MIN(rows, (long) ptr->rows__in__array - thisrow);
    if (rows <= 0)		/* this chunk might be past end of file! */
      break;
    byte__count = rows * bytesperrow;
    if (writing)
      (*ptr->b__s__info.write__backing__store) (cinfo, & ptr->b__s__info,
					    (void FAR *) ptr->mem__buffer[i],
					    file__offset, byte__count);
    else
      (*ptr->b__s__info.read__backing__store) (cinfo, & ptr->b__s__info,
					   (void FAR *) ptr->mem__buffer[i],
					   file__offset, byte__count);
    file__offset += byte__count;
  }
}


METHODDEF(JSAMPARRAY)
access__virt__sarray (j__common__ptr cinfo, jvirt__sarray__ptr ptr,
		    JDIMENSION start__row, JDIMENSION num__rows,
		    CH_BOOL writable)
/* Access the part of a virtual sample array starting at start__row */
/* and extending for num__rows rows.  writable is true if  */
/* caller intends to modify the accessed area. */
{
  JDIMENSION end__row = start__row + num__rows;
  JDIMENSION undef__row;

  /* debugging check */
  if (end__row > ptr->rows__in__array || num__rows > ptr->maxaccess ||
      ptr->mem__buffer == NULL)
    ERREXIT(cinfo, JERR__BAD__VIRTUAL__ACCESS);

  /* Make the desired part of the virtual array accessible */
  if (start__row < ptr->cur__start__row ||
      end__row > ptr->cur__start__row+ptr->rows__in__mem) {
    if (! ptr->b__s__open)
      ERREXIT(cinfo, JERR__VIRTUAL__BUG);
    /* Flush old buffer contents if necessary */
    if (ptr->dirty) {
      do__sarray__io(cinfo, ptr, TRUE);
      ptr->dirty = FALSE;
    }
    /* Decide what part of virtual array to access.
     * Algorithm: if target address > current window, assume forward scan,
     * load starting at target address.  If target address < current window,
     * assume backward scan, load so that target area is top of window.
     * Note that when switching from forward write to forward read, will have
     * start__row = 0, so the limiting case applies and we load from 0 anyway.
     */
    if (start__row > ptr->cur__start__row) {
      ptr->cur__start__row = start__row;
    } else {
      /* use long arithmetic here to avoid overflow & unsigned problems */
      long ltemp;

      ltemp = (long) end__row - (long) ptr->rows__in__mem;
      if (ltemp < 0)
	ltemp = 0;		/* don't fall off front end of file */
      ptr->cur__start__row = (JDIMENSION) ltemp;
    }
    /* Read in the selected part of the array.
     * During the initial write pass, we will do no actual read
     * because the selected part is all undefined.
     */
    do__sarray__io(cinfo, ptr, FALSE);
  }
  /* Ensure the accessed part of the array is defined; prezero if needed.
   * To improve locality of access, we only prezero the part of the array
   * that the caller is about to access, not the entire in-memory array.
   */
  if (ptr->first__undef__row < end__row) {
    if (ptr->first__undef__row < start__row) {
      if (writable)		/* writer skipped over a section of array */
	ERREXIT(cinfo, JERR__BAD__VIRTUAL__ACCESS);
      undef__row = start__row;	/* but reader is allowed to read ahead */
    } else {
      undef__row = ptr->first__undef__row;
    }
    if (writable)
      ptr->first__undef__row = end__row;
    if (ptr->pre__zero) {
      size_t bytesperrow = (size_t) ptr->samplesperrow * SIZEOF(JSAMPLE);
      undef__row -= ptr->cur__start__row; /* make indexes relative to buffer */
      end__row -= ptr->cur__start__row;
      while (undef__row < end__row) {
	jzero__far((void FAR *) ptr->mem__buffer[undef__row], bytesperrow);
	undef__row++;
      }
    } else {
      if (! writable)		/* reader looking at undefined data */
	ERREXIT(cinfo, JERR__BAD__VIRTUAL__ACCESS);
    }
  }
  /* Flag the buffer dirty if caller will write in it */
  if (writable)
    ptr->dirty = TRUE;
  /* Return address of proper part of the buffer */
  return ptr->mem__buffer + (start__row - ptr->cur__start__row);
}


METHODDEF(JBLOCKARRAY)
access__virt__barray (j__common__ptr cinfo, jvirt__barray__ptr ptr,
		    JDIMENSION start__row, JDIMENSION num__rows,
		    CH_BOOL writable)
/* Access the part of a virtual block array starting at start__row */
/* and extending for num__rows rows.  writable is true if  */
/* caller intends to modify the accessed area. */
{
  JDIMENSION end__row = start__row + num__rows;
  JDIMENSION undef__row;

  /* debugging check */
  if (end__row > ptr->rows__in__array || num__rows > ptr->maxaccess ||
      ptr->mem__buffer == NULL)
    ERREXIT(cinfo, JERR__BAD__VIRTUAL__ACCESS);

  /* Make the desired part of the virtual array accessible */
  if (start__row < ptr->cur__start__row ||
      end__row > ptr->cur__start__row+ptr->rows__in__mem) {
    if (! ptr->b__s__open)
      ERREXIT(cinfo, JERR__VIRTUAL__BUG);
    /* Flush old buffer contents if necessary */
    if (ptr->dirty) {
      do__barray__io(cinfo, ptr, TRUE);
      ptr->dirty = FALSE;
    }
    /* Decide what part of virtual array to access.
     * Algorithm: if target address > current window, assume forward scan,
     * load starting at target address.  If target address < current window,
     * assume backward scan, load so that target area is top of window.
     * Note that when switching from forward write to forward read, will have
     * start__row = 0, so the limiting case applies and we load from 0 anyway.
     */
    if (start__row > ptr->cur__start__row) {
      ptr->cur__start__row = start__row;
    } else {
      /* use long arithmetic here to avoid overflow & unsigned problems */
      long ltemp;

      ltemp = (long) end__row - (long) ptr->rows__in__mem;
      if (ltemp < 0)
	ltemp = 0;		/* don't fall off front end of file */
      ptr->cur__start__row = (JDIMENSION) ltemp;
    }
    /* Read in the selected part of the array.
     * During the initial write pass, we will do no actual read
     * because the selected part is all undefined.
     */
    do__barray__io(cinfo, ptr, FALSE);
  }
  /* Ensure the accessed part of the array is defined; prezero if needed.
   * To improve locality of access, we only prezero the part of the array
   * that the caller is about to access, not the entire in-memory array.
   */
  if (ptr->first__undef__row < end__row) {
    if (ptr->first__undef__row < start__row) {
      if (writable)		/* writer skipped over a section of array */
	ERREXIT(cinfo, JERR__BAD__VIRTUAL__ACCESS);
      undef__row = start__row;	/* but reader is allowed to read ahead */
    } else {
      undef__row = ptr->first__undef__row;
    }
    if (writable)
      ptr->first__undef__row = end__row;
    if (ptr->pre__zero) {
      size_t bytesperrow = (size_t) ptr->blocksperrow * SIZEOF(JBLOCK);
      undef__row -= ptr->cur__start__row; /* make indexes relative to buffer */
      end__row -= ptr->cur__start__row;
      while (undef__row < end__row) {
	jzero__far((void FAR *) ptr->mem__buffer[undef__row], bytesperrow);
	undef__row++;
      }
    } else {
      if (! writable)		/* reader looking at undefined data */
	ERREXIT(cinfo, JERR__BAD__VIRTUAL__ACCESS);
    }
  }
  /* Flag the buffer dirty if caller will write in it */
  if (writable)
    ptr->dirty = TRUE;
  /* Return address of proper part of the buffer */
  return ptr->mem__buffer + (start__row - ptr->cur__start__row);
}


/*
 * Release all objects belonging to a specified pool.
 */

METHODDEF(void)
free__pool (j__common__ptr cinfo, int pool__id)
{
  my__mem__ptr mem = (my__mem__ptr) cinfo->mem;
  small__pool__ptr shdr__ptr;
  large__pool__ptr lhdr__ptr;
  size_t space__freed;

  if (pool__id < 0 || pool__id >= JPOOL__NUMPOOLS)
    ERREXIT1(cinfo, JERR__BAD__POOL__ID, pool__id);	/* safety check */

#ifdef MEM__STATS
  if (cinfo->err->trace__level > 1)
    print__mem__stats(cinfo, pool__id); /* print pool's memory usage statistics */
#endif

  /* If freeing IMAGE pool, close any virtual arrays first */
  if (pool__id == JPOOL__IMAGE) {
    jvirt__sarray__ptr sptr;
    jvirt__barray__ptr bptr;

    for (sptr = mem->virt__sarray__list; sptr != NULL; sptr = sptr->next) {
      if (sptr->b__s__open) {	/* there may be no backing store */
	sptr->b__s__open = FALSE;	/* prevent recursive close if error */
	(*sptr->b__s__info.close__backing__store) (cinfo, & sptr->b__s__info);
      }
    }
    mem->virt__sarray__list = NULL;
    for (bptr = mem->virt__barray__list; bptr != NULL; bptr = bptr->next) {
      if (bptr->b__s__open) {	/* there may be no backing store */
	bptr->b__s__open = FALSE;	/* prevent recursive close if error */
	(*bptr->b__s__info.close__backing__store) (cinfo, & bptr->b__s__info);
      }
    }
    mem->virt__barray__list = NULL;
  }

  /* Release large objects */
  lhdr__ptr = mem->large__list[pool__id];
  mem->large__list[pool__id] = NULL;

  while (lhdr__ptr != NULL) {
    large__pool__ptr next__lhdr__ptr = lhdr__ptr->hdr.next;
    space__freed = lhdr__ptr->hdr.bytes__used +
		  lhdr__ptr->hdr.bytes__left +
		  SIZEOF(large__pool__hdr);
    jpeg__free__large(cinfo, (void FAR *) lhdr__ptr, space__freed);
    mem->total__space__allocated -= space__freed;
    lhdr__ptr = next__lhdr__ptr;
  }

  /* Release small objects */
  shdr__ptr = mem->small__list[pool__id];
  mem->small__list[pool__id] = NULL;

  while (shdr__ptr != NULL) {
    small__pool__ptr next__shdr__ptr = shdr__ptr->hdr.next;
    space__freed = shdr__ptr->hdr.bytes__used +
		  shdr__ptr->hdr.bytes__left +
		  SIZEOF(small__pool__hdr);
    jpeg__free__small(cinfo, (void *) shdr__ptr, space__freed);
    mem->total__space__allocated -= space__freed;
    shdr__ptr = next__shdr__ptr;
  }
}


/*
 * Close up shop entirely.
 * Note that this cannot be called unless cinfo->mem is non-NULL.
 */

METHODDEF(void)
self__destruct (j__common__ptr cinfo)
{
  int pool;

  /* Close all backing store, release all memory.
   * Releasing pools in reverse order might help avoid fragmentation
   * with some (brain-damaged) malloc libraries.
   */
  for (pool = JPOOL__NUMPOOLS-1; pool >= JPOOL__PERMANENT; pool--) {
    free__pool(cinfo, pool);
  }

  /* Release the memory manager control block too. */
  jpeg__free__small(cinfo, (void *) cinfo->mem, SIZEOF(my__memory__mgr));
  cinfo->mem = NULL;		/* ensures I will be called only once */

  jpeg__mem__term(cinfo);		/* system-dependent cleanup */
}


/*
 * Memory manager initialization.
 * When this is called, only the error manager pointer is valid in cinfo!
 
#define DZ__DEBUG__MEMORY__MGR*/
GLOBAL(void)
jinit__memory__mgr (j__common__ptr cinfo)
{
  my__mem__ptr mem;
  long max__to__use;
  int pool;
  size_t test__mac;

  cinfo->mem = NULL;		/* for safety if init fails */

  /* Check for configuration errors.
   * SIZEOF(ALIGN__TYPE) should be a power of 2; otherwise, it probably
   * doesn't reflect any real hardware alignment requirement.
   * The test is a little tricky: for X>0, X and X-1 have no one-bits
   * in common if and only if X is a power of 2, ie has only one one-bit.
   * Some compilers may give an "unreachable code" warning here; ignore it.
   */
  if ((SIZEOF(ALIGN__TYPE) & (SIZEOF(ALIGN__TYPE)-1)) != 0)
    ERREXIT(cinfo, JERR__BAD__ALIGN__TYPE);
  /* MAX__ALLOC__CHUNK must be representable as type size_t, and must be
   * a multiple of SIZEOF(ALIGN__TYPE).
   * Again, an "unreachable code" warning may be ignored here.
   * But a "constant too large" warning means you need to fix MAX__ALLOC__CHUNK.
   */
  test__mac = (size_t) MAX__ALLOC__CHUNK;
  if ((long) test__mac != MAX__ALLOC__CHUNK ||
      (MAX__ALLOC__CHUNK % SIZEOF(ALIGN__TYPE)) != 0)
    ERREXIT(cinfo, JERR__BAD__ALLOC__CHUNK);
  #ifdef DZ__DEBUG__MEMORY__MGR
  	STTBX_Print("start mem init\n");
  #endif
  max__to__use = jpeg__mem__init(cinfo); /* system-dependent initialization */

 #ifdef DZ__DEBUG__MEMORY__MGR
  	STTBX_Print("start jpeg__get__small\n");
  #endif
  /* Attempt to allocate memory manager's control block */
  mem = (my__mem__ptr) jpeg__get__small(cinfo, SIZEOF(my__memory__mgr));
   #ifdef DZ__DEBUG__MEMORY__MGR
  	STTBX_Print("mem == 0x%x\n", mem);
  #endif
  if (mem == NULL) {
    jpeg__mem__term(cinfo);	/* system-dependent cleanup */
    ERREXIT1(cinfo, JERR__OUT__OF__MEMORY, 0);
  }

  /* OK, fill in the method pointers */
  mem->pub.alloc__small = alloc__small;
  mem->pub.alloc__large = alloc__large;
  mem->pub.alloc__sarray = alloc__sarray;
  mem->pub.alloc__barray = alloc__barray;
  mem->pub.request__virt__sarray = request__virt__sarray;
  mem->pub.request__virt__barray = request__virt__barray;
  mem->pub.realize__virt__arrays = realize__virt__arrays;
  mem->pub.access__virt__sarray = access__virt__sarray;
  mem->pub.access__virt__barray = access__virt__barray;
  mem->pub.free__pool = free__pool;
  mem->pub.self__destruct = self__destruct;

  /* Make MAX__ALLOC__CHUNK accessible to other modules */
  mem->pub.max__alloc__chunk = MAX__ALLOC__CHUNK;

  /* Initialize working state */
  mem->pub.max__memory__to__use = max__to__use;

  for (pool = JPOOL__NUMPOOLS-1; pool >= JPOOL__PERMANENT; pool--) {
    mem->small__list[pool] = NULL;
    mem->large__list[pool] = NULL;
  }
  mem->virt__sarray__list = NULL;
  mem->virt__barray__list = NULL;

  mem->total__space__allocated = SIZEOF(my__memory__mgr);

  /* Declare ourselves open for business */
  cinfo->mem = & mem->pub;

  /* Check for an environment variable JPEGMEM; if found, override the
   * default max__memory setting from jpeg__mem__init.  Note that the
   * surrounding application may again override this value.
   * If your system doesn't support getenv(), define NO__GETENV to disable
   * this feature.
   */
   #ifdef DZ__DEBUG__MEMORY__MGR
  	STTBX_Print("mem ok\n");
  #endif
#ifndef NO__GETENV
  { char * memenv;

    if ((memenv = getenv("JPEGMEM")) != NULL) {
      char ch = 'x';

      if (sscanf(memenv, "%ld%c", &max__to__use, &ch) > 0) {
	if (ch == 'm' || ch == 'M')
	  max__to__use *= 1000L;
	mem->pub.max__memory__to__use = max__to__use * 1000L;
      }
    }
  }
#endif
 #ifdef DZ__DEBUG__MEMORY__MGR
  	STTBX_Print("mem quit\n");
  #endif
}
