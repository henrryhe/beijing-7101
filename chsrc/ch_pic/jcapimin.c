/*
 * jcapimin.c
 *
 * Copyright (C) 1994-1998, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains application interface code for the compression half
 * of the JPEG library.  These are the "minimum" API routines that may be
 * needed in either the normal full-compression case or the transcoding-only
 * case.
 *
 * Most of the routines intended to be called directly by an application
 * are in this file or in jcapistd.c.  But also see jcparam.c for
 * parameter-setup helper routines, jcomapi.c for routines shared by
 * compression and decompression, and jctrans.c for the transcoding case.
 */

#define JPEG__INTERNALS
#include "jinclude.h"
#include "jpeglib.h"
#include "graphconfig.h"


/*
 * Initialization of a JPEG compression object.
 * The error manager must already be set up (in case memory manager fails).
 */

GLOBAL(void)
jpeg__CreateCompress (j__compress__ptr cinfo, int version, size_t structsize)
{
  int i;

  /* Guard against version mismatches between library and caller. */
  cinfo->mem = NULL;		/* so jpeg__destroy knows mem mgr not called */
  if (version != JPEG__LIB__VERSION)
    ERREXIT2(cinfo, JERR__BAD__LIB__VERSION, JPEG__LIB__VERSION, version);
  if (structsize != SIZEOF(struct jpeg__compress__struct))
    ERREXIT2(cinfo, JERR__BAD__STRUCT__SIZE, 
	     (int) SIZEOF(struct jpeg__compress__struct), (int) structsize);

  /* For debugging purposes, we zero the whole master structure.
   * But the application has already set the err pointer, and may have set
   * client__data, so we have to save and restore those fields.
   * Note: if application hasn't set client__data, tools like Purify may
   * complain here.
   */
  {
    struct jpeg__error__mgr * err = cinfo->err;
    void * client__data = cinfo->client__data; /* ignore Purify complaint here */
    MEMZERO(cinfo, SIZEOF(struct jpeg__compress__struct));
    cinfo->err = err;
    cinfo->client__data = client__data;
  }
  cinfo->is__decompressor = FALSE;

  /* Initialize a memory manager instance for this object */
  jinit__memory__mgr((j__common__ptr) cinfo);

  /* Zero out pointers to permanent structures. */
  cinfo->progress = NULL;
  cinfo->dest = NULL;

  cinfo->comp__info = NULL;

  for (i = 0; i < NUM__QUANT__TBLS; i++)
    cinfo->quant__tbl__ptrs[i] = NULL;

  for (i = 0; i < NUM__HUFF__TBLS; i++) {
    cinfo->dc__huff__tbl__ptrs[i] = NULL;
    cinfo->ac__huff__tbl__ptrs[i] = NULL;
  }

  cinfo->script__space = NULL;

  cinfo->input__gamma = 1.0;	/* in case application forgets */

  /* OK, I'm ready */
  cinfo->global__state = CSTATE__START;
}


/*
 * Destruction of a JPEG compression object
 */

GLOBAL(void)
jpeg__destroy__compress (j__compress__ptr cinfo)
{
  jpeg__destroy((j__common__ptr) cinfo); /* use common routine */
}


/*
 * Abort processing of a JPEG compression operation,
 * but don't destroy the object itself.
 */

GLOBAL(void)
jpeg__abort__compress (j__compress__ptr cinfo)
{
  jpeg__abort((j__common__ptr) cinfo); /* use common routine */
}


/*
 * Forcibly suppress or un-suppress all quantization and Huffman tables.
 * Marks all currently defined tables as already written (if suppress)
 * or not written (if !suppress).  This will control whether they get emitted
 * by a subsequent jpeg__start__compress call.
 *
 * This routine is exported for use by applications that want to produce
 * abbreviated JPEG datastreams.  It logically belongs in jcparam.c, but
 * since it is called by jpeg__start__compress, we put it here --- otherwise
 * jcparam.o would be linked whether the application used it or not.
 */

GLOBAL(void)
jpeg__suppress__tables (j__compress__ptr cinfo, CH_BOOL suppress){
  int i;
  JQUANT__TBL * qtbl;
  JHUFF__TBL * htbl;

  for (i = 0; i < NUM__QUANT__TBLS; i++) {
    if ((qtbl = cinfo->quant__tbl__ptrs[i]) != NULL)
      qtbl->sent__table = suppress;
  }

  for (i = 0; i < NUM__HUFF__TBLS; i++) {
    if ((htbl = cinfo->dc__huff__tbl__ptrs[i]) != NULL)
      htbl->sent__table = suppress;
    if ((htbl = cinfo->ac__huff__tbl__ptrs[i]) != NULL)
      htbl->sent__table = suppress;
  }
}


/*
 * Finish JPEG compression.
 *
 * If a multipass operating mode was selected, this may do a great deal of
 * work including most of the actual output.
 */

GLOBAL(void)
jpeg__finish__compress (j__compress__ptr cinfo)
{
  JDIMENSION iMCU__row;

  if (cinfo->global__state == CSTATE__SCANNING ||
      cinfo->global__state == CSTATE__RAW__OK) {
    /* Terminate first pass */
    if (cinfo->next__scanline < cinfo->image__height)
      ERREXIT(cinfo, JERR__TOO__LITTLE__DATA);
    (*cinfo->master->finish__pass) (cinfo);
  } else if (cinfo->global__state != CSTATE__WRCOEFS)
    ERREXIT1(cinfo, JERR__BAD__STATE, cinfo->global__state);
  /* Perform any remaining passes */
  while (! cinfo->master->is__last__pass) {
    (*cinfo->master->prepare__for__pass) (cinfo);
    for (iMCU__row = 0; iMCU__row < cinfo->total__iMCU__rows; iMCU__row++) {
      if (cinfo->progress != NULL) {
	cinfo->progress->pass__counter = (long) iMCU__row;
	cinfo->progress->pass__limit = (long) cinfo->total__iMCU__rows;
	(*cinfo->progress->progress__monitor) ((j__common__ptr) cinfo);
      }
      /* We bypass the main controller and invoke coef controller directly;
       * all work is being done from the coefficient buffer.
       */
      if (! (*cinfo->coef->compress__data) (cinfo, (JSAMPIMAGE) NULL))
	ERREXIT(cinfo, JERR__CANT__SUSPEND);
    }
    (*cinfo->master->finish__pass) (cinfo);
  }
  /* Write EOI, do final cleanup */
  (*cinfo->marker->write__file__trailer) (cinfo);
  (*cinfo->dest->term__destination) (cinfo);
  /* We can use jpeg__abort to release memory and reset global__state */
  jpeg__abort((j__common__ptr) cinfo);
}


/*
 * Write a special marker.
 * This is only recommended for writing COM or APPn markers.
 * Must be called after jpeg__start__compress() and before
 * first call to jpeg__write__scanlines() or jpeg__write__raw__data().
 */

GLOBAL(void)
jpeg__write__marker (j__compress__ptr cinfo, int marker,
		   const JOCTET *dataptr, unsigned int datalen)
{
  JMETHOD(void, write__marker__byte, (j__compress__ptr info, int val));

  if (cinfo->next__scanline != 0 ||
      (cinfo->global__state != CSTATE__SCANNING &&
       cinfo->global__state != CSTATE__RAW__OK &&
       cinfo->global__state != CSTATE__WRCOEFS))
    ERREXIT1(cinfo, JERR__BAD__STATE, cinfo->global__state);

  (*cinfo->marker->write__marker__header) (cinfo, marker, datalen);
  write__marker__byte = cinfo->marker->write__marker__byte;	/* copy for speed */
  while (datalen--) {
    (*write__marker__byte) (cinfo, *dataptr);
    dataptr++;
  }
}

/* Same, but piecemeal. */

GLOBAL(void)
jpeg__write__m__header (j__compress__ptr cinfo, int marker, unsigned int datalen)
{
  if (cinfo->next__scanline != 0 ||
      (cinfo->global__state != CSTATE__SCANNING &&
       cinfo->global__state != CSTATE__RAW__OK &&
       cinfo->global__state != CSTATE__WRCOEFS))
    ERREXIT1(cinfo, JERR__BAD__STATE, cinfo->global__state);

  (*cinfo->marker->write__marker__header) (cinfo, marker, datalen);
}

GLOBAL(void)
jpeg__write__m__byte (j__compress__ptr cinfo, int val)
{
  (*cinfo->marker->write__marker__byte) (cinfo, val);
}


/*
 * Alternate compression function: just write an abbreviated table file.
 * Before calling this, all parameters and a data destination must be set up.
 *
 * To produce a pair of files containing abbreviated tables and abbreviated
 * image data, one would proceed as follows:
 *
 *		initialize JPEG object
 *		set JPEG parameters
 *		set destination to table file
 *		jpeg__write__tables(cinfo);
 *		set destination to image file
 *		jpeg__start__compress(cinfo, FALSE);
 *		write data...
 *		jpeg__finish__compress(cinfo);
 *
 * jpeg__write__tables has the side effect of marking all tables written
 * (same as jpeg__suppress__tables(..., TRUE)).  Thus a subsequent start__compress
 * will not re-emit the tables unless it is passed write__all__tables=TRUE.
 */

GLOBAL(void)
jpeg__write__tables (j__compress__ptr cinfo)
{
  if (cinfo->global__state != CSTATE__START)
    ERREXIT1(cinfo, JERR__BAD__STATE, cinfo->global__state);

  /* (Re)initialize error mgr and destination modules */
  (*cinfo->err->reset__error__mgr) ((j__common__ptr) cinfo);
  (*cinfo->dest->init__destination) (cinfo);
  /* Initialize the marker writer ... bit of a crock to do it here. */
  jinit__marker__writer(cinfo);
  /* Write them tables! */
  (*cinfo->marker->write__tables__only) (cinfo);
  /* And clean up. */
  (*cinfo->dest->term__destination) (cinfo);
  /*
   * In library releases up through v6a, we called jpeg__abort() here to free
   * any working memory allocated by the destination manager and marker
   * writer.  Some applications had a problem with that: they allocated space
   * of their own from the library memory manager, and didn't want it to go
   * away during write__tables.  So now we do nothing.  This will cause a
   * memory leak if an app calls write__tables repeatedly without doing a full
   * compression cycle or otherwise resetting the JPEG object.  However, that
   * seems less bad than unexpectedly freeing memory in the normal case.
   * An app that prefers the old behavior can call jpeg__abort for itself after
   * each call to jpeg__write__tables().
   */
}
