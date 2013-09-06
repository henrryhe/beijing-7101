/*
 * jcprepct.c
 *
 * Copyright (C) 1994-1996, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains the compression preprocessing controller.
 * This controller manages the color conversion, downsampling,
 * and edge expansion steps.
 *
 * Most of the complexity here is associated with buffering input rows
 * as required by the downsampler.  See the comments at the head of
 * jcsample.c for the downsampler's needs.
 */

#define JPEG__INTERNALS
#include "jinclude.h"
#include "jpeglib.h"


/* At present, jcsample.c can request context rows only for smoothing.
 * In the future, we might also need context rows for CCIR601 sampling
 * or other more-complex downsampling procedures.  The code to support
 * context rows should be compiled only if needed.
 */
#ifdef INPUT__SMOOTHING__SUPPORTED
#define CONTEXT__ROWS__SUPPORTED
#endif


/*
 * For the simple (no-context-row) case, we just need to buffer one
 * row group's worth of pixels for the downsampling step.  At the bottom of
 * the image, we pad to a full row group by replicating the last pixel row.
 * The downsampler's last output row is then replicated if needed to pad
 * out to a full iMCU row.
 *
 * When providing context rows, we must buffer three row groups' worth of
 * pixels.  Three row groups are physically allocated, but the row pointer
 * arrays are made five row groups high, with the extra pointers above and
 * below "wrapping around" to point to the last and first real row groups.
 * This allows the downsampler to access the proper context rows.
 * At the top and bottom of the image, we create dummy context rows by
 * copying the first or last real pixel row.  This copying could be avoided
 * by pointer hacking as is done in jdmainct.c, but it doesn't seem worth the
 * trouble on the compression side.
 */


/* Private buffer controller object */

typedef struct {
  struct jpeg__c__prep__controller pub; /* public fields */

  /* Downsampling input buffer.  This buffer holds color-converted data
   * until we have enough to do a downsample step.
   */
  JSAMPARRAY color__buf[MAX__COMPONENTS];

  JDIMENSION rows__to__go;	/* counts rows remaining in source image */
  int next__buf__row;		/* index of next row to store in color__buf */

#ifdef CONTEXT__ROWS__SUPPORTED	/* only needed for context case */
  int this__row__group;		/* starting row index of group to process */
  int next__buf__stop;		/* downsample when we reach this index */
#endif
} my__prep__controller;

typedef my__prep__controller * my__prep__ptr;


/*
 * Initialize for a processing pass.
 */

METHODDEF(void)
start__pass__prep (j__compress__ptr cinfo, J__BUF__MODE pass__mode)
{
  my__prep__ptr prep = (my__prep__ptr) cinfo->prep;

  if (pass__mode != JBUF__PASS__THRU)
    ERREXIT(cinfo, JERR__BAD__BUFFER__MODE);

  /* Initialize total-height counter for detecting bottom of image */
  prep->rows__to__go = cinfo->image__height;
  /* Mark the conversion buffer empty */
  prep->next__buf__row = 0;
#ifdef CONTEXT__ROWS__SUPPORTED
  /* Preset additional state variables for context mode.
   * These aren't used in non-context mode, so we needn't test which mode.
   */
  prep->this__row__group = 0;
  /* Set next__buf__stop to stop after two row groups have been read in. */
  prep->next__buf__stop = 2 * cinfo->max__v__samp__factor;
#endif
}


/*
 * Expand an image vertically from height input__rows to height output__rows,
 * by duplicating the bottom row.
 */

LOCAL(void)
expand__bottom__edge (JSAMPARRAY image__data, JDIMENSION num__cols,
		    int input__rows, int output__rows)
{
  register int row;

  for (row = input__rows; row < output__rows; row++) {
    jcopy__sample__rows(image__data, input__rows-1, image__data, row,
		      1, num__cols);
  }
}


/*
 * Process some data in the simple no-context case.
 *
 * Preprocessor output data is counted in "row groups".  A row group
 * is defined to be v__samp__factor sample rows of each component.
 * Downsampling will produce this much data from each max__v__samp__factor
 * input rows.
 */

METHODDEF(void)
pre__process__data (j__compress__ptr cinfo,
		  JSAMPARRAY input__buf, JDIMENSION *in__row__ctr,
		  JDIMENSION in__rows__avail,
		  JSAMPIMAGE output__buf, JDIMENSION *out__row__group__ctr,
		  JDIMENSION out__row__groups__avail)
{
  my__prep__ptr prep = (my__prep__ptr) cinfo->prep;
  int numrows, ci;
  JDIMENSION inrows;
  jpeg__component__info * compptr;

  while (*in__row__ctr < in__rows__avail &&
	 *out__row__group__ctr < out__row__groups__avail) {
    /* Do color conversion to fill the conversion buffer. */
    inrows = in__rows__avail - *in__row__ctr;
    numrows = cinfo->max__v__samp__factor - prep->next__buf__row;
    numrows = (int) MIN((JDIMENSION) numrows, inrows);
    (*cinfo->cconvert->color__convert) (cinfo, input__buf + *in__row__ctr,
				       prep->color__buf,
				       (JDIMENSION) prep->next__buf__row,
				       numrows);
    *in__row__ctr += numrows;
    prep->next__buf__row += numrows;
    prep->rows__to__go -= numrows;
    /* If at bottom of image, pad to fill the conversion buffer. */
    if (prep->rows__to__go == 0 &&
	prep->next__buf__row < cinfo->max__v__samp__factor) {
      for (ci = 0; ci < cinfo->num__components; ci++) {
	expand__bottom__edge(prep->color__buf[ci], cinfo->image__width,
			   prep->next__buf__row, cinfo->max__v__samp__factor);
      }
      prep->next__buf__row = cinfo->max__v__samp__factor;
    }
    /* If we've filled the conversion buffer, empty it. */
    if (prep->next__buf__row == cinfo->max__v__samp__factor) {
      (*cinfo->downsample->downsample) (cinfo,
					prep->color__buf, (JDIMENSION) 0,
					output__buf, *out__row__group__ctr);
      prep->next__buf__row = 0;
      (*out__row__group__ctr)++;
    }
    /* If at bottom of image, pad the output to a full iMCU height.
     * Note we assume the caller is providing a one-iMCU-height output buffer!
     */
    if (prep->rows__to__go == 0 &&
	*out__row__group__ctr < out__row__groups__avail) {
      for (ci = 0, compptr = cinfo->comp__info; ci < cinfo->num__components;
	   ci++, compptr++) {
	expand__bottom__edge(output__buf[ci],
			   compptr->width__in__blocks * DCTSIZE,
			   (int) (*out__row__group__ctr * compptr->v__samp__factor),
			   (int) (out__row__groups__avail * compptr->v__samp__factor));
      }
      *out__row__group__ctr = out__row__groups__avail;
      break;			/* can exit outer loop without test */
    }
  }
}


#ifdef CONTEXT__ROWS__SUPPORTED

/*
 * Process some data in the context case.
 */

METHODDEF(void)
pre__process__context (j__compress__ptr cinfo,
		     JSAMPARRAY input__buf, JDIMENSION *in__row__ctr,
		     JDIMENSION in__rows__avail,
		     JSAMPIMAGE output__buf, JDIMENSION *out__row__group__ctr,
		     JDIMENSION out__row__groups__avail)
{
  my__prep__ptr prep = (my__prep__ptr) cinfo->prep;
  int numrows, ci;
  int buf__height = cinfo->max__v__samp__factor * 3;
  JDIMENSION inrows;

  while (*out__row__group__ctr < out__row__groups__avail) {
    if (*in__row__ctr < in__rows__avail) {
      /* Do color conversion to fill the conversion buffer. */
      inrows = in__rows__avail - *in__row__ctr;
      numrows = prep->next__buf__stop - prep->next__buf__row;
      numrows = (int) MIN((JDIMENSION) numrows, inrows);
      (*cinfo->cconvert->color__convert) (cinfo, input__buf + *in__row__ctr,
					 prep->color__buf,
					 (JDIMENSION) prep->next__buf__row,
					 numrows);
      /* Pad at top of image, if first time through */
      if (prep->rows__to__go == cinfo->image__height) {
	for (ci = 0; ci < cinfo->num__components; ci++) {
	  int row;
	  for (row = 1; row <= cinfo->max__v__samp__factor; row++) {
	    jcopy__sample__rows(prep->color__buf[ci], 0,
			      prep->color__buf[ci], -row,
			      1, cinfo->image__width);
	  }
	}
      }
      *in__row__ctr += numrows;
      prep->next__buf__row += numrows;
      prep->rows__to__go -= numrows;
    } else {
      /* Return for more data, unless we are at the bottom of the image. */
      if (prep->rows__to__go != 0)
	break;
      /* When at bottom of image, pad to fill the conversion buffer. */
      if (prep->next__buf__row < prep->next__buf__stop) {
	for (ci = 0; ci < cinfo->num__components; ci++) {
	  expand__bottom__edge(prep->color__buf[ci], cinfo->image__width,
			     prep->next__buf__row, prep->next__buf__stop);
	}
	prep->next__buf__row = prep->next__buf__stop;
      }
    }
    /* If we've gotten enough data, downsample a row group. */
    if (prep->next__buf__row == prep->next__buf__stop) {
      (*cinfo->downsample->downsample) (cinfo,
					prep->color__buf,
					(JDIMENSION) prep->this__row__group,
					output__buf, *out__row__group__ctr);
      (*out__row__group__ctr)++;
      /* Advance pointers with wraparound as necessary. */
      prep->this__row__group += cinfo->max__v__samp__factor;
      if (prep->this__row__group >= buf__height)
	prep->this__row__group = 0;
      if (prep->next__buf__row >= buf__height)
	prep->next__buf__row = 0;
      prep->next__buf__stop = prep->next__buf__row + cinfo->max__v__samp__factor;
    }
  }
}


/*
 * Create the wrapped-around downsampling input buffer needed for context mode.
 */

LOCAL(void)
create__context__buffer (j__compress__ptr cinfo)
{
  my__prep__ptr prep = (my__prep__ptr) cinfo->prep;
  int rgroup__height = cinfo->max__v__samp__factor;
  int ci, i;
  jpeg__component__info * compptr;
  JSAMPARRAY true__buffer, fake__buffer;

  /* Grab enough space for fake row pointers for all the components;
   * we need five row groups' worth of pointers for each component.
   */
  fake__buffer = (JSAMPARRAY)
    (*cinfo->mem->alloc__small) ((j__common__ptr) cinfo, JPOOL__IMAGE,
				(cinfo->num__components * 5 * rgroup__height) *
				SIZEOF(JSAMPROW));

  for (ci = 0, compptr = cinfo->comp__info; ci < cinfo->num__components;
       ci++, compptr++) {
    /* Allocate the actual buffer space (3 row groups) for this component.
     * We make the buffer wide enough to allow the downsampler to edge-expand
     * horizontally within the buffer, if it so chooses.
     */
    true__buffer = (*cinfo->mem->alloc__sarray)
      ((j__common__ptr) cinfo, JPOOL__IMAGE,
       (JDIMENSION) (((long) compptr->width__in__blocks * DCTSIZE *
		      cinfo->max__h__samp__factor) / compptr->h__samp__factor),
       (JDIMENSION) (3 * rgroup__height));
    /* Copy true buffer row pointers into the middle of the fake row array */
    MEMCOPY(fake__buffer + rgroup__height, true__buffer,
	    3 * rgroup__height * SIZEOF(JSAMPROW));
    /* Fill in the above and below wraparound pointers */
    for (i = 0; i < rgroup__height; i++) {
      fake__buffer[i] = true__buffer[2 * rgroup__height + i];
      fake__buffer[4 * rgroup__height + i] = true__buffer[i];
    }
    prep->color__buf[ci] = fake__buffer + rgroup__height;
    fake__buffer += 5 * rgroup__height; /* point to space for next component */
  }
}

#endif /* CONTEXT__ROWS__SUPPORTED */


/*
 * Initialize preprocessing controller.
 */

GLOBAL(void)
jinit__c__prep__controller (j__compress__ptr cinfo, CH_BOOL need__full__buffer)
{
  my__prep__ptr prep;
  int ci;
  jpeg__component__info * compptr;

  if (need__full__buffer)		/* safety check */
    ERREXIT(cinfo, JERR__BAD__BUFFER__MODE);

  prep = (my__prep__ptr)
    (*cinfo->mem->alloc__small) ((j__common__ptr) cinfo, JPOOL__IMAGE,
				SIZEOF(my__prep__controller));
  cinfo->prep = (struct jpeg__c__prep__controller *) prep;
  prep->pub.start__pass = start__pass__prep;

  /* Allocate the color conversion buffer.
   * We make the buffer wide enough to allow the downsampler to edge-expand
   * horizontally within the buffer, if it so chooses.
   */
  if (cinfo->downsample->need__context__rows) {
    /* Set up to provide context rows */
#ifdef CONTEXT__ROWS__SUPPORTED
    prep->pub.pre__process__data = pre__process__context;
    create__context__buffer(cinfo);
#else
    ERREXIT(cinfo, JERR__NOT__COMPILED);
#endif
  } else {
    /* No context, just make it tall enough for one row group */
    prep->pub.pre__process__data = pre__process__data;
    for (ci = 0, compptr = cinfo->comp__info; ci < cinfo->num__components;
	 ci++, compptr++) {
      prep->color__buf[ci] = (*cinfo->mem->alloc__sarray)
	((j__common__ptr) cinfo, JPOOL__IMAGE,
	 (JDIMENSION) (((long) compptr->width__in__blocks * DCTSIZE *
			cinfo->max__h__samp__factor) / compptr->h__samp__factor),
	 (JDIMENSION) cinfo->max__v__samp__factor);
    }
  }
}
