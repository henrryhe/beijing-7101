/*
 * jcapistd.c
 *
 * Copyright (C) 1994-1996, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains application interface code for the compression half
 * of the JPEG library.  These are the "standard" API routines that are
 * used in the normal full-compression case.  They are not used by a
 * transcoding-only application.  Note that if an application links in
 * jpeg__start__compress, it will end up linking in the entire compressor.
 * We thus must separate this file from jcapimin.c to avoid linking the
 * whole compression library into a transcoder.
 */

#define JPEG__INTERNALS
#include "jinclude.h"
#include "jpeglib.h"


/*
 * Compression initialization.
 * Before calling this, all parameters and a data destination must be set up.
 *
 * We require a write__all__tables parameter as a failsafe check when writing
 * multiple datastreams from the same compression object.  Since prior runs
 * will have left all the tables marked sent__table=TRUE, a subsequent run
 * would emit an abbreviated stream (no tables) by default.  This may be what
 * is wanted, but for safety's sake it should not be the default behavior:
 * programmers should have to make a deliberate choice to emit abbreviated
 * images.  Therefore the documentation and examples should encourage people
 * to pass write__all__tables=TRUE; then it will take active thought to do the
 * wrong thing.
 */

GLOBAL(void)jpeg__start__compress(j__compress__ptr cinfo, CH_BOOL write__all__tables)
{
  if (cinfo->global__state != CSTATE__START)
    ERREXIT1(cinfo, JERR__BAD__STATE, cinfo->global__state);

  if (write__all__tables)
    jpeg__suppress__tables(cinfo, FALSE);
   /* mark all tables to be written */

  /* (Re)initialize error mgr and destination modules */
  (*cinfo->err->reset__error__mgr)((j__common__ptr)cinfo);
  (*cinfo->dest->init__destination)(cinfo);
  /* Perform master selection of active modules */
  jinit__compress__master(cinfo);
  /* Set up for the first pass */
  (*cinfo->master->prepare__for__pass)(cinfo);
  /* Ready for application to drive first pass through jpeg__write__scanlines
   * or jpeg__write__raw__data.
   */
  cinfo->next__scanline = 0;
  cinfo->global__state = (cinfo->raw__data__in ? CSTATE__RAW__OK : CSTATE__SCANNING);
}


/*
 * Write some scanlines of data to the JPEG compressor.
 *
 * The return value will be the number of lines actually written.
 * This should be less than the supplied num__lines only in case that
 * the data destination module has requested suspension of the compressor,
 * or if more than image__height scanlines are passed in.
 *
 * Note: we warn about excess calls to jpeg__write__scanlines() since
 * this likely signals an application programmer error.  However,
 * excess scanlines passed in the last valid call are *silently* ignored,
 * so that the application need not adjust num__lines for end-of-image
 * when using a multiple-scanline buffer.
 */

GLOBAL(JDIMENSION)jpeg__write__scanlines(j__compress__ptr cinfo, JSAMPARRAY
  scanlines, JDIMENSION num__lines)
{
  JDIMENSION row__ctr, rows__left;

  if (cinfo->global__state != CSTATE__SCANNING)
    ERREXIT1(cinfo, JERR__BAD__STATE, cinfo->global__state);
  if (cinfo->next__scanline >= cinfo->image__height)
    WARNMS(cinfo, JWRN__TOO__MUCH_DATA);

  /* Call progress monitor hook if present */
  if (cinfo->progress != NULL)
  {
    cinfo->progress->pass__counter = (long)cinfo->next__scanline;
    cinfo->progress->pass__limit = (long)cinfo->image__height;
    (*cinfo->progress->progress__monitor)((j__common__ptr)cinfo);
  }

  /* Give master control module another chance if this is first call to
   * jpeg__write__scanlines.  This lets output of the frame/scan headers be
   * delayed so that application can write COM, etc, markers between
   * jpeg__start__compress and jpeg__write__scanlines.
   */
  if (cinfo->master->call__pass__startup)
    (*cinfo->master->pass__startup)(cinfo);

  /* Ignore any extra scanlines at bottom of image. */
  rows__left = cinfo->image__height - cinfo->next__scanline;
  if (num__lines > rows__left)
    num__lines = rows__left;

  row__ctr = 0;
  (*cinfo->main->process__data)(cinfo, scanlines, &row__ctr, num__lines);
  cinfo->next__scanline += row__ctr;
  return row__ctr;
}


/*
 * Alternate entry point to write raw data.
 * Processes exactly one iMCU row per call, unless suspended.
 */

GLOBAL(JDIMENSION)jpeg__write__raw__data(j__compress__ptr cinfo, JSAMPIMAGE data,
  JDIMENSION num__lines)
{
  JDIMENSION lines__per__iMCU__row;

  if (cinfo->global__state != CSTATE__RAW__OK)
    ERREXIT1(cinfo, JERR__BAD__STATE, cinfo->global__state);
  if (cinfo->next__scanline >= cinfo->image__height)
  {
    WARNMS(cinfo, JWRN__TOO__MUCH_DATA);
    return 0;
  }

  /* Call progress monitor hook if present */
  if (cinfo->progress != NULL)
  {
    cinfo->progress->pass__counter = (long)cinfo->next__scanline;
    cinfo->progress->pass__limit = (long)cinfo->image__height;
    (*cinfo->progress->progress__monitor)((j__common__ptr)cinfo);
  }

  /* Give master control module another chance if this is first call to
   * jpeg__write__raw__data.  This lets output of the frame/scan headers be
   * delayed so that application can write COM, etc, markers between
   * jpeg__start__compress and jpeg__write__raw__data.
   */
  if (cinfo->master->call__pass__startup)
    (*cinfo->master->pass__startup)(cinfo);

  /* Verify that at least one iMCU row has been passed. */
  lines__per__iMCU__row = cinfo->max__v__samp__factor *DCTSIZE;
  if (num__lines < lines__per__iMCU__row)
    ERREXIT(cinfo, JERR__BUFFER__SIZE);

  /* Directly compress the row. */
  if (!(*cinfo->coef->compress__data)(cinfo, data))
  {
    /* If compressor did not consume the whole row, suspend processing. */
    return 0;
  }

  /* OK, we processed one iMCU row. */
  cinfo->next__scanline += lines__per__iMCU__row;
  return lines__per__iMCU__row;
}
