/*
 * jdapistd.c
 *
 * Copyright (C) 1994-1996, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains application interface code for the decompression half
 * of the JPEG library.  These are the "standard" API routines that are
 * used in the normal full-decompression case.  They are not used by a
 * transcoding-only application.  Note that if an application links in
 * jpeg__start__decompress, it will end up linking in the entire decompressor.
 * We thus must separate this file from jdapimin.c to avoid linking the
 * whole decompression library into a transcoder.
 */

#define JPEG__INTERNALS
#include "jinclude.h"
#include "jpeglib.h"


/* Forward declarations */
LOCAL(CH_BOOL) output__pass__setup JPP((j__decompress__ptr cinfo));


/*
 * Decompression initialization.
 * jpeg__read__header must be completed before calling this.
 *
 * If a multipass operating mode was selected, this will do all but the
 * last pass, and thus may take a great deal of time.
 *
 * Returns FALSE if suspended.  The return value need be inspected only if
 * a suspending data source is used.
 */

GLOBAL(CH_BOOL)
jpeg__start__decompress (j__decompress__ptr cinfo)
{
  if (cinfo->global__state == DSTATE__READY) {
    /* First call: initialize master control, select active modules */
    jinit__master__decompress(cinfo);
    if (cinfo->buffered__image) {
      /* No more work here; expecting jpeg__start__output next */
      cinfo->global__state = DSTATE__BUFIMAGE;
      return TRUE;
    }
    cinfo->global__state = DSTATE__PRELOAD;
  }
  if (cinfo->global__state == DSTATE__PRELOAD) {
    /* If file has multiple scans, absorb them all into the coef buffer */
    if (cinfo->inputctl->has__multiple__scans) {
#ifdef D__MULTISCAN__FILES__SUPPORTED
      for (;;) {
	int retcode;
	/* Call progress monitor hook if present */
	if (cinfo->progress != NULL)
	  (*cinfo->progress->progress__monitor) ((j__common__ptr) cinfo);
	/* Absorb some more input */
	retcode = (*cinfo->inputctl->consume__input) (cinfo);
	if (retcode == JPEG__SUSPENDED)
	  return FALSE;
	if (retcode == JPEG__REACHED__EOI)
	  break;
	/* Advance progress counter if appropriate */
	if (cinfo->progress != NULL &&
	    (retcode == JPEG__ROW__COMPLETED || retcode == JPEG__REACHED__SOS)) {
	  if (++cinfo->progress->pass__counter >= cinfo->progress->pass__limit) {
	    /* jdmaster underestimated number of scans; ratchet up one scan */
	    cinfo->progress->pass__limit += (long) cinfo->total__iMCU__rows;
	  }
	}
      }
#else
      ERREXIT(cinfo, JERR__NOT__COMPILED);
#endif /* D__MULTISCAN__FILES__SUPPORTED */
    }
    cinfo->output__scan__number = cinfo->input__scan__number;
  } else if (cinfo->global__state != DSTATE__PRESCAN)
    ERREXIT1(cinfo, JERR__BAD__STATE, cinfo->global__state);
  /* Perform any dummy output passes, and set up for the final pass */
  return output__pass__setup(cinfo);
}


/*
 * Set up for an output pass, and perform any dummy pass(es) needed.
 * Common subroutine for jpeg__start__decompress and jpeg__start__output.
 * Entry: global__state = DSTATE__PRESCAN only if previously suspended.
 * Exit: If done, returns TRUE and sets global__state for proper output mode.
 *       If suspended, returns FALSE and sets global__state = DSTATE__PRESCAN.
 */

LOCAL(CH_BOOL)
output__pass__setup (j__decompress__ptr cinfo)
{
  if (cinfo->global__state != DSTATE__PRESCAN) {
    /* First call: do pass setup */
    (*cinfo->master->prepare__for__output__pass) (cinfo);
    cinfo->output__scanline = 0;
    cinfo->global__state = DSTATE__PRESCAN;
  }
  /* Loop over any required dummy passes */
  while (cinfo->master->is__dummy__pass) {
#ifdef QUANT__2PASS__SUPPORTED
    /* Crank through the dummy pass */
    while (cinfo->output__scanline < cinfo->output__height) {
      JDIMENSION last__scanline;
      /* Call progress monitor hook if present */
      if (cinfo->progress != NULL) {
	cinfo->progress->pass__counter = (long) cinfo->output__scanline;
	cinfo->progress->pass__limit = (long) cinfo->output__height;
	(*cinfo->progress->progress__monitor) ((j__common__ptr) cinfo);
      }
      /* Process some data */
      last__scanline = cinfo->output__scanline;
      (*cinfo->main->process__data) (cinfo, (JSAMPARRAY) NULL,
				    &cinfo->output__scanline, (JDIMENSION) 0);
      if (cinfo->output__scanline == last__scanline)
	return FALSE;		/* No progress made, must suspend */
    }
    /* Finish up dummy pass, and set up for another one */
    (*cinfo->master->finish__output__pass) (cinfo);
    (*cinfo->master->prepare__for__output__pass) (cinfo);
    cinfo->output__scanline = 0;
#else
    ERREXIT(cinfo, JERR__NOT__COMPILED);
#endif /* QUANT__2PASS__SUPPORTED */
  }
  /* Ready for application to drive output pass through
   * jpeg__read__scanlines or jpeg__read__raw__data.
   */
  cinfo->global__state = cinfo->raw__data__out ? DSTATE__RAW__OK : DSTATE__SCANNING;
  return TRUE;
}


/*
 * Read some scanlines of data from the JPEG decompressor.
 *
 * The return value will be the number of lines actually read.
 * This may be less than the number requested in several cases,
 * including bottom of image, data source suspension, and operating
 * modes that emit multiple scanlines at a time.
 *
 * Note: we warn about excess calls to jpeg__read__scanlines() since
 * this likely signals an application programmer error.  However,
 * an oversize buffer (max__lines > scanlines remaining) is not an error.
 */

GLOBAL(JDIMENSION)
jpeg__read__scanlines (j__decompress__ptr cinfo, JSAMPARRAY scanlines,
		     JDIMENSION max__lines)
{
  JDIMENSION row__ctr;

  if (cinfo->global__state != DSTATE__SCANNING)
    ERREXIT1(cinfo, JERR__BAD__STATE, cinfo->global__state);
  if (cinfo->output__scanline >= cinfo->output__height) {
    WARNMS(cinfo, JWRN__TOO__MUCH_DATA);
    return 0;
  }

  /* Call progress monitor hook if present */
  if (cinfo->progress != NULL) {
    cinfo->progress->pass__counter = (long) cinfo->output__scanline;
    cinfo->progress->pass__limit = (long) cinfo->output__height;
    (*cinfo->progress->progress__monitor) ((j__common__ptr) cinfo);
  }

  /* Process some data */
  row__ctr = 0;
  (*cinfo->main->process__data) (cinfo, scanlines, &row__ctr, max__lines);
  cinfo->output__scanline += row__ctr;
  return row__ctr;
}


/*
 * Alternate entry point to read raw data.
 * Processes exactly one iMCU row per call, unless suspended.
 */

GLOBAL(JDIMENSION)
jpeg__read__raw__data (j__decompress__ptr cinfo, JSAMPIMAGE data,
		    JDIMENSION max__lines)
{
  JDIMENSION lines__per__iMCU__row;

  if (cinfo->global__state != DSTATE__RAW__OK)
    ERREXIT1(cinfo, JERR__BAD__STATE, cinfo->global__state);
  if (cinfo->output__scanline >= cinfo->output__height) {
    WARNMS(cinfo, JWRN__TOO__MUCH_DATA);
    return 0;
  }

  /* Call progress monitor hook if present */
  if (cinfo->progress != NULL) {
    cinfo->progress->pass__counter = (long) cinfo->output__scanline;
    cinfo->progress->pass__limit = (long) cinfo->output__height;
    (*cinfo->progress->progress__monitor) ((j__common__ptr) cinfo);
  }

  /* Verify that at least one iMCU row can be returned. */
  lines__per__iMCU__row = cinfo->max__v__samp__factor * cinfo->min__DCT__scaled__size;
  if (max__lines < lines__per__iMCU__row)
    ERREXIT(cinfo, JERR__BUFFER__SIZE);

  /* Decompress directly into user's buffer. */
  if (! (*cinfo->coef->decompress__data) (cinfo, data))
    return 0;			/* suspension forced, can do nothing more */

  /* OK, we processed one iMCU row. */
  cinfo->output__scanline += lines__per__iMCU__row;
  return lines__per__iMCU__row;
}


/* Additional entry points for buffered-image mode. */

#ifdef D__MULTISCAN__FILES__SUPPORTED

/*
 * Initialize for an output pass in buffered-image mode.
 */

GLOBAL(CH_BOOL)
jpeg__start__output (j__decompress__ptr cinfo, int scan__number)
{
  if (cinfo->global__state != DSTATE__BUFIMAGE &&
      cinfo->global__state != DSTATE__PRESCAN)
    ERREXIT1(cinfo, JERR__BAD__STATE, cinfo->global__state);
  /* Limit scan number to valid range */
  if (scan__number <= 0)
    scan__number = 1;
  if (cinfo->inputctl->eoi__reached &&
      scan__number > cinfo->input__scan__number)
    scan__number = cinfo->input__scan__number;
  cinfo->output__scan__number = scan__number;
  /* Perform any dummy output passes, and set up for the real pass */
  return output__pass__setup(cinfo);
}


/*
 * Finish up after an output pass in buffered-image mode.
 *
 * Returns FALSE if suspended.  The return value need be inspected only if
 * a suspending data source is used.
 */

GLOBAL(CH_BOOL)
jpeg__finish__output (j__decompress__ptr cinfo)
{
  if ((cinfo->global__state == DSTATE__SCANNING ||
       cinfo->global__state == DSTATE__RAW__OK) && cinfo->buffered__image) {
    /* Terminate this pass. */
    /* We do not require the whole pass to have been completed. */
    (*cinfo->master->finish__output__pass) (cinfo);
    cinfo->global__state = DSTATE__BUFPOST;
  } else if (cinfo->global__state != DSTATE__BUFPOST) {
    /* BUFPOST = repeat call after a suspension, anything else is error */
    ERREXIT1(cinfo, JERR__BAD__STATE, cinfo->global__state);
  }
  /* Read markers looking for SOS or EOI */
  while (cinfo->input__scan__number <= cinfo->output__scan__number &&
	 ! cinfo->inputctl->eoi__reached) {
    if ((*cinfo->inputctl->consume__input) (cinfo) == JPEG__SUSPENDED)
      return FALSE;		/* Suspend, come back later */
  }
  cinfo->global__state = DSTATE__BUFIMAGE;
  return TRUE;
}

#endif /* D__MULTISCAN__FILES__SUPPORTED */
