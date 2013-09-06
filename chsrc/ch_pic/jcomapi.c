/*
 * jcomapi.c
 *
 * Copyright (C) 1994-1997, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains application interface routines that are used for both
 * compression and decompression.
 */

#define JPEG__INTERNALS
#include "jinclude.h"
#include "jpeglib.h"


/*
 * Abort processing of a JPEG compression or decompression operation,
 * but don't destroy the object itself.
 *
 * For this, we merely clean up all the nonpermanent memory pools.
 * Note that temp files (virtual arrays) are not allowed to belong to
 * the permanent pool, so we will be able to close all temp files here.
 * Closing a data source or destination, if necessary, is the application's
 * responsibility.
 */

GLOBAL(void)
jpeg__abort (j__common__ptr cinfo)
{
  int pool;

  /* Do nothing if called on a not-initialized or destroyed JPEG object. */
  if (cinfo->mem == NULL)
    return;

  /* Releasing pools in reverse order might help avoid fragmentation
   * with some (brain-damaged) malloc libraries.
   */
  for (pool = JPOOL__NUMPOOLS-1; pool > JPOOL__PERMANENT; pool--) {
    (*cinfo->mem->free__pool) (cinfo, pool);
  }

  /* Reset overall state for possible reuse of object */
  if (cinfo->is__decompressor) {
    cinfo->global__state = DSTATE__START;
    /* Try to keep application from accessing now-deleted marker list.
     * A bit kludgy to do it here, but this is the most central place.
     */
    ((j__decompress__ptr) cinfo)->marker__list = NULL;
  } else {
    cinfo->global__state = CSTATE__START;
  }
}


/*
 * Destruction of a JPEG object.
 *
 * Everything gets deallocated except the master jpeg__compress__struct itself
 * and the error manager struct.  Both of these are supplied by the application
 * and must be freed, if necessary, by the application.  (Often they are on
 * the stack and so don't need to be freed anyway.)
 * Closing a data source or destination, if necessary, is the application's
 * responsibility.
 */

GLOBAL(void)
jpeg__destroy (j__common__ptr cinfo)
{
  /* We need only tell the memory manager to release everything. */
  /* NB: mem pointer is NULL if memory mgr failed to initialize. */
  if (cinfo->mem != NULL)
    (*cinfo->mem->self__destruct) (cinfo);
  cinfo->mem = NULL;		/* be safe if jpeg__destroy is called twice */
  cinfo->global__state = 0;	/* mark it destroyed */
}


/*
 * Convenience routines for allocating quantization and Huffman tables.
 * (Would jutils.c be a more reasonable place to put these?)
 */

GLOBAL(JQUANT__TBL *)
jpeg__alloc__quant__table (j__common__ptr cinfo)
{
  JQUANT__TBL *tbl;

  tbl = (JQUANT__TBL *)
    (*cinfo->mem->alloc__small) (cinfo, JPOOL__PERMANENT, SIZEOF(JQUANT__TBL));
  tbl->sent__table = FALSE;	/* make sure this is false in any new table */
  return tbl;
}


GLOBAL(JHUFF__TBL *)
jpeg__alloc__huff__table (j__common__ptr cinfo)
{
  JHUFF__TBL *tbl;

  tbl = (JHUFF__TBL *)
    (*cinfo->mem->alloc__small) (cinfo, JPOOL__PERMANENT, SIZEOF(JHUFF__TBL));
  tbl->sent__table = FALSE;	/* make sure this is false in any new table */
  return tbl;
}
