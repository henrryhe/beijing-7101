/*
 * jcmainct.c
 *
 * Copyright (C) 1994-1996, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains the main buffer controller for compression.
 * The main buffer lies between the pre-processor and the JPEG
 * compressor proper; it holds downsampled data in the JPEG colorspace.
 */

#define JPEG__INTERNALS
#include "jinclude.h"
#include "jpeglib.h"


/* Note: currently, there is no operating mode in which a full-image buffer
 * is needed at this step.  If there were, that mode could not be used with
 * "raw data" input, since this module is bypassed in that case.  However,
 * we've left the code here for possible use in special applications.
 */
#undef FULL__MAIN__BUFFER__SUPPORTED


/* Private buffer controller object */

typedef struct {
  struct jpeg__c__main__controller pub; /* public fields */

  JDIMENSION cur__iMCU__row;	/* number of current iMCU row */
  JDIMENSION rowgroup__ctr;	/* counts row groups received in iMCU row */
  CH_BOOL suspended;		/* remember if we suspended output */
  J__BUF__MODE pass__mode;		/* current operating mode */

  /* If using just a strip buffer, this points to the entire set of buffers
   * (we allocate one for each component).  In the full-image case, this
   * points to the currently accessible strips of the virtual arrays.
   */
  JSAMPARRAY buffer[MAX__COMPONENTS];

#ifdef FULL__MAIN__BUFFER__SUPPORTED
  /* If using full-image storage, this array holds pointers to virtual-array
   * control blocks for each component.  Unused if not full-image storage.
   */
  jvirt__sarray__ptr whole__image[MAX__COMPONENTS];
#endif
} my__main__controller;

typedef my__main__controller * my__main__ptr;


/* Forward declarations */
METHODDEF(void) process__data__simple__main
	JPP((j__compress__ptr cinfo, JSAMPARRAY input__buf,
	     JDIMENSION *in__row__ctr, JDIMENSION in__rows__avail));
#ifdef FULL__MAIN__BUFFER__SUPPORTED
METHODDEF(void) process__data__buffer__main
	JPP((j__compress__ptr cinfo, JSAMPARRAY input__buf,
	     JDIMENSION *in__row__ctr, JDIMENSION in__rows__avail));
#endif


/*
 * Initialize for a processing pass.
 */

METHODDEF(void)
start__pass__main (j__compress__ptr cinfo, J__BUF__MODE pass__mode)
{
  my__main__ptr main = (my__main__ptr) cinfo->main;

  /* Do nothing in raw-data mode. */
  if (cinfo->raw__data__in)
    return;

  main->cur__iMCU__row = 0;	/* initialize counters */
  main->rowgroup__ctr = 0;
  main->suspended = FALSE;
  main->pass__mode = pass__mode;	/* save mode for use by process__data */

  switch (pass__mode) {
  case JBUF__PASS__THRU:
#ifdef FULL__MAIN__BUFFER__SUPPORTED
    if (main->whole__image[0] != NULL)
      ERREXIT(cinfo, JERR__BAD__BUFFER__MODE);
#endif
    main->pub.process__data = process__data__simple__main;
    break;
#ifdef FULL__MAIN__BUFFER__SUPPORTED
  case JBUF__SAVE__SOURCE:
  case JBUF__CRANK__DEST:
  case JBUF__SAVE__AND__PASS:
    if (main->whole__image[0] == NULL)
      ERREXIT(cinfo, JERR__BAD__BUFFER__MODE);
    main->pub.process__data = process__data__buffer__main;
    break;
#endif
  default:
    ERREXIT(cinfo, JERR__BAD__BUFFER__MODE);
    break;
  }
}


/*
 * Process some data.
 * This routine handles the simple pass-through mode,
 * where we have only a strip buffer.
 */

METHODDEF(void)
process__data__simple__main (j__compress__ptr cinfo,
			  JSAMPARRAY input__buf, JDIMENSION *in__row__ctr,
			  JDIMENSION in__rows__avail)
{
  my__main__ptr main = (my__main__ptr) cinfo->main;

  while (main->cur__iMCU__row < cinfo->total__iMCU__rows) {
    /* Read input data if we haven't filled the main buffer yet */
    if (main->rowgroup__ctr < DCTSIZE)
      (*cinfo->prep->pre__process__data) (cinfo,
					input__buf, in__row__ctr, in__rows__avail,
					main->buffer, &main->rowgroup__ctr,
					(JDIMENSION) DCTSIZE);

    /* If we don't have a full iMCU row buffered, return to application for
     * more data.  Note that preprocessor will always pad to fill the iMCU row
     * at the bottom of the image.
     */
    if (main->rowgroup__ctr != DCTSIZE)
      return;

    /* Send the completed row to the compressor */
    if (! (*cinfo->coef->compress__data) (cinfo, main->buffer)) {
      /* If compressor did not consume the whole row, then we must need to
       * suspend processing and return to the application.  In this situation
       * we pretend we didn't yet consume the last input row; otherwise, if
       * it happened to be the last row of the image, the application would
       * think we were done.
       */
      if (! main->suspended) {
	(*in__row__ctr)--;
	main->suspended = TRUE;
      }
      return;
    }
    /* We did finish the row.  Undo our little suspension hack if a previous
     * call suspended; then mark the main buffer empty.
     */
    if (main->suspended) {
      (*in__row__ctr)++;
      main->suspended = FALSE;
    }
    main->rowgroup__ctr = 0;
    main->cur__iMCU__row++;
  }
}


#ifdef FULL__MAIN__BUFFER__SUPPORTED

/*
 * Process some data.
 * This routine handles all of the modes that use a full-size buffer.
 */

METHODDEF(void)
process__data__buffer__main (j__compress__ptr cinfo,
			  JSAMPARRAY input__buf, JDIMENSION *in__row__ctr,
			  JDIMENSION in__rows__avail)
{
  my__main__ptr main = (my__main__ptr) cinfo->main;
  int ci;
  jpeg__component__info *compptr;
  CH_BOOL writing = (main->pass__mode != JBUF__CRANK__DEST);

  while (main->cur__iMCU__row < cinfo->total__iMCU__rows) {
    /* Realign the virtual buffers if at the start of an iMCU row. */
    if (main->rowgroup__ctr == 0) {
      for (ci = 0, compptr = cinfo->comp__info; ci < cinfo->num__components;
	   ci++, compptr++) {
	main->buffer[ci] = (*cinfo->mem->access__virt__sarray)
	  ((j__common__ptr) cinfo, main->whole__image[ci],
	   main->cur__iMCU__row * (compptr->v__samp__factor * DCTSIZE),
	   (JDIMENSION) (compptr->v__samp__factor * DCTSIZE), writing);
      }
      /* In a read pass, pretend we just read some source data. */
      if (! writing) {
	*in__row__ctr += cinfo->max__v__samp__factor * DCTSIZE;
	main->rowgroup__ctr = DCTSIZE;
      }
    }

    /* If a write pass, read input data until the current iMCU row is full. */
    /* Note: preprocessor will pad if necessary to fill the last iMCU row. */
    if (writing) {
      (*cinfo->prep->pre__process__data) (cinfo,
					input__buf, in__row__ctr, in__rows__avail,
					main->buffer, &main->rowgroup__ctr,
					(JDIMENSION) DCTSIZE);
      /* Return to application if we need more data to fill the iMCU row. */
      if (main->rowgroup__ctr < DCTSIZE)
	return;
    }

    /* Emit data, unless this is a sink-only pass. */
    if (main->pass__mode != JBUF__SAVE__SOURCE) {
      if (! (*cinfo->coef->compress__data) (cinfo, main->buffer)) {
	/* If compressor did not consume the whole row, then we must need to
	 * suspend processing and return to the application.  In this situation
	 * we pretend we didn't yet consume the last input row; otherwise, if
	 * it happened to be the last row of the image, the application would
	 * think we were done.
	 */
	if (! main->suspended) {
	  (*in__row__ctr)--;
	  main->suspended = TRUE;
	}
	return;
      }
      /* We did finish the row.  Undo our little suspension hack if a previous
       * call suspended; then mark the main buffer empty.
       */
      if (main->suspended) {
	(*in__row__ctr)++;
	main->suspended = FALSE;
      }
    }

    /* If get here, we are done with this iMCU row.  Mark buffer empty. */
    main->rowgroup__ctr = 0;
    main->cur__iMCU__row++;
  }
}

#endif /* FULL__MAIN__BUFFER__SUPPORTED */


/*
 * Initialize main buffer controller.
 */

GLOBAL(void)
jinit__c__main__controller (j__compress__ptr cinfo, CH_BOOL need__full__buffer)
{
  my__main__ptr main;
  int ci;
  jpeg__component__info *compptr;

  main = (my__main__ptr)
    (*cinfo->mem->alloc__small) ((j__common__ptr) cinfo, JPOOL__IMAGE,
				SIZEOF(my__main__controller));
  cinfo->main = (struct jpeg__c__main__controller *) main;
  main->pub.start__pass = start__pass__main;

  /* We don't need to create a buffer in raw-data mode. */
  if (cinfo->raw__data__in)
    return;

  /* Create the buffer.  It holds downsampled data, so each component
   * may be of a different size.
   */
  if (need__full__buffer) {
#ifdef FULL__MAIN__BUFFER__SUPPORTED
    /* Allocate a full-image virtual array for each component */
    /* Note we pad the bottom to a multiple of the iMCU height */
    for (ci = 0, compptr = cinfo->comp__info; ci < cinfo->num__components;
	 ci++, compptr++) {
      main->whole__image[ci] = (*cinfo->mem->request__virt__sarray)
	((j__common__ptr) cinfo, JPOOL__IMAGE, FALSE,
	 compptr->width__in__blocks * DCTSIZE,
	 (JDIMENSION) jround__up((long) compptr->height__in__blocks,
				(long) compptr->v__samp__factor) * DCTSIZE,
	 (JDIMENSION) (compptr->v__samp__factor * DCTSIZE));
    }
#else
    ERREXIT(cinfo, JERR__BAD__BUFFER__MODE);
#endif
  } else {
#ifdef FULL__MAIN__BUFFER__SUPPORTED
    main->whole__image[0] = NULL; /* flag for no virtual arrays */
#endif
    /* Allocate a strip buffer for each component */
    for (ci = 0, compptr = cinfo->comp__info; ci < cinfo->num__components;
	 ci++, compptr++) {
      main->buffer[ci] = (*cinfo->mem->alloc__sarray)
	((j__common__ptr) cinfo, JPOOL__IMAGE,
	 compptr->width__in__blocks * DCTSIZE,
	 (JDIMENSION) (compptr->v__samp__factor * DCTSIZE));
    }
  }
}
