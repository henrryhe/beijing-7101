/*
 * jdtrans.c
 *
 * Copyright (C) 1995-1997, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains library routines for transcoding decompression,
 * that is, reading raw DCT coefficient arrays from an input JPEG file.
 * The routines in jdapimin.c will also be needed by a transcoder.
 */

#define JPEG__INTERNALS
#include "jinclude.h"
#include "jpeglib.h"


/* Forward declarations */
LOCAL(void) transdecode__master__selection JPP((j__decompress__ptr cinfo));


/*
 * Read the coefficient arrays from a JPEG file.
 * jpeg__read__header must be completed before calling this.
 *
 * The entire image is read into a set of virtual coefficient-block arrays,
 * one per component.  The return value is a pointer to the array of
 * virtual-array descriptors.  These can be manipulated directly via the
 * JPEG memory manager, or handed off to jpeg__write__coefficients().
 * To release the memory occupied by the virtual arrays, call
 * jpeg__finish__decompress() when done with the data.
 *
 * An alternative usage is to simply obtain access to the coefficient arrays
 * during a buffered-image-mode decompression operation.  This is allowed
 * after any jpeg__finish__output() call.  The arrays can be accessed until
 * jpeg__finish__decompress() is called.  (Note that any call to the library
 * may reposition the arrays, so don't rely on access__virt__barray() results
 * to stay valid across library calls.)
 *
 * Returns NULL if suspended.  This case need be checked only if
 * a suspending data source is used.
 */

GLOBAL(jvirt__barray__ptr *)
jpeg__read__coefficients (j__decompress__ptr cinfo)
{
  if (cinfo->global__state == DSTATE__READY) {
    /* First call: initialize active modules */
    transdecode__master__selection(cinfo);
    cinfo->global__state = DSTATE__RDCOEFS;
  }
  if (cinfo->global__state == DSTATE__RDCOEFS) {
    /* Absorb whole file into the coef buffer */
    for (;;) {
      int retcode;
      /* Call progress monitor hook if present */
      if (cinfo->progress != NULL)
	(*cinfo->progress->progress__monitor) ((j__common__ptr) cinfo);
      /* Absorb some more input */
      retcode = (*cinfo->inputctl->consume__input) (cinfo);
      if (retcode == JPEG__SUSPENDED)
	return NULL;
      if (retcode == JPEG__REACHED__EOI)
	break;
      /* Advance progress counter if appropriate */
      if (cinfo->progress != NULL &&
	  (retcode == JPEG__ROW__COMPLETED || retcode == JPEG__REACHED__SOS)) {
	if (++cinfo->progress->pass__counter >= cinfo->progress->pass__limit) {
	  /* startup underestimated number of scans; ratchet up one scan */
	  cinfo->progress->pass__limit += (long) cinfo->total__iMCU__rows;
	}
      }
    }
    /* Set state so that jpeg__finish__decompress does the right thing */
    cinfo->global__state = DSTATE__STOPPING;
  }
  /* At this point we should be in state DSTATE__STOPPING if being used
   * standalone, or in state DSTATE__BUFIMAGE if being invoked to get access
   * to the coefficients during a full buffered-image-mode decompression.
   */
  if ((cinfo->global__state == DSTATE__STOPPING ||
       cinfo->global__state == DSTATE__BUFIMAGE) && cinfo->buffered__image) {
    return cinfo->coef->coef__arrays;
  }
  /* Oops, improper usage */
  ERREXIT1(cinfo, JERR__BAD__STATE, cinfo->global__state);
  return NULL;			/* keep compiler happy */
}


/*
 * Master selection of decompression modules for transcoding.
 * This substitutes for jdmaster.c's initialization of the full decompressor.
 */

LOCAL(void)
transdecode__master__selection (j__decompress__ptr cinfo)
{
  /* This is effectively a buffered-image operation. */
  cinfo->buffered__image = TRUE;

  /* Entropy decoding: either Huffman or arithmetic coding. */
  if (cinfo->arith__code) {
    ERREXIT(cinfo, JERR__ARITH__NOTIMPL);
  } else {
    if (cinfo->progressive__mode) {
#ifdef D__PROGRESSIVE__SUPPORTED
      jinit__phuff__decoder(cinfo);
#else
      ERREXIT(cinfo, JERR__NOT__COMPILED);
#endif
    } else
      jinit__huff__decoder(cinfo);
  }

  /* Always get a full-image coefficient buffer. */
  jinit__d__coef__controller(cinfo, TRUE);

  /* We can now tell the memory manager to allocate virtual arrays. */
  (*cinfo->mem->realize__virt__arrays) ((j__common__ptr) cinfo);

  /* Initialize input side of decompressor to consume first scan. */
  (*cinfo->inputctl->start__input__pass) (cinfo);

  /* Initialize progress monitoring. */
  if (cinfo->progress != NULL) {
    int nscans;
    /* Estimate number of scans to set pass__limit. */
    if (cinfo->progressive__mode) {
      /* Arbitrarily estimate 2 interleaved DC scans + 3 AC scans/component. */
      nscans = 2 + 3 * cinfo->num__components;
    } else if (cinfo->inputctl->has__multiple__scans) {
      /* For a nonprogressive multiscan file, estimate 1 scan per component. */
      nscans = cinfo->num__components;
    } else {
      nscans = 1;
    }
    cinfo->progress->pass__counter = 0L;
    cinfo->progress->pass__limit = (long) cinfo->total__iMCU__rows * nscans;
    cinfo->progress->completed__passes = 0;
    cinfo->progress->total__passes = 1;
  }
}
