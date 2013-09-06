/*
 * jdsample.c
 *
 * Copyright (C) 1991-1996, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains upsampling routines.
 *
 * Upsampling input data is counted in "row groups".  A row group
 * is defined to be (v__samp__factor * DCT__scaled__size / min__DCT__scaled__size)
 * sample rows of each component.  Upsampling will normally produce
 * max__v__samp__factor pixel rows from each row group (but this could vary
 * if the upsampler is applying a scale factor of its own).
 *
 * An excellent reference for image resampling is
 *   Digital Image Warping, George Wolberg, 1990.
 *   Pub. by IEEE Computer Society Press, Los Alamitos, CA. ISBN 0-8186-8944-7.
 */

#define JPEG__INTERNALS
#include "jinclude.h"
#include "jpeglib.h"


/* Pointer to routine to upsample a single component */
typedef JMETHOD(void, upsample1__ptr,
		(j__decompress__ptr cinfo, jpeg__component__info * compptr,
		 JSAMPARRAY input__data, JSAMPARRAY * output__data__ptr));

/* Private subobject */

typedef struct {
  struct jpeg__upsampler pub;	/* public fields */

  /* Color conversion buffer.  When using separate upsampling and color
   * conversion steps, this buffer holds one upsampled row group until it
   * has been color converted and output.
   * Note: we do not allocate any storage for component(s) which are full-size,
   * ie do not need rescaling.  The corresponding entry of color__buf[] is
   * simply set to point to the input data array, thereby avoiding copying.
   */
  JSAMPARRAY color__buf[MAX__COMPONENTS];

  /* Per-component upsampling method pointers */
  upsample1__ptr methods[MAX__COMPONENTS];

  int next__row__out;		/* counts rows emitted from color__buf */
  JDIMENSION rows__to__go;	/* counts rows remaining in image */

  /* Height of an input row group for each component. */
  int rowgroup__height[MAX__COMPONENTS];

  /* These arrays save pixel expansion factors so that int__expand need not
   * recompute them each time.  They are unused for other upsampling methods.
   */
  UINT8 h__expand[MAX__COMPONENTS];
  UINT8 v__expand[MAX__COMPONENTS];
} my__upsampler;

typedef my__upsampler * my__upsample__ptr;


/*
 * Initialize for an upsampling pass.
 */

METHODDEF(void)
start__pass__upsample (j__decompress__ptr cinfo)
{
  my__upsample__ptr upsample = (my__upsample__ptr) cinfo->upsample;

  /* Mark the conversion buffer empty */
  upsample->next__row__out = cinfo->max__v__samp__factor;
  /* Initialize total-height counter for detecting bottom of image */
  upsample->rows__to__go = cinfo->output__height;
}


/*
 * Control routine to do upsampling (and color conversion).
 *
 * In this version we upsample each component independently.
 * We upsample one row group into the conversion buffer, then apply
 * color conversion a row at a time.
 */

METHODDEF(void)
sep__upsample (j__decompress__ptr cinfo,
	      JSAMPIMAGE input__buf, JDIMENSION *in__row__group__ctr,
	      JDIMENSION in__row__groups__avail,
	      JSAMPARRAY output__buf, JDIMENSION *out__row__ctr,
	      JDIMENSION out__rows__avail)
{
  my__upsample__ptr upsample = (my__upsample__ptr) cinfo->upsample;
  int ci;
  jpeg__component__info * compptr;
  JDIMENSION num__rows;

  /* Fill the conversion buffer, if it's empty */
  if (upsample->next__row__out >= cinfo->max__v__samp__factor) {
    for (ci = 0, compptr = cinfo->comp__info; ci < cinfo->num__components;
	 ci++, compptr++) {
      /* Invoke per-component upsample method.  Notice we pass a POINTER
       * to color__buf[ci], so that fullsize__upsample can change it.
       */
      (*upsample->methods[ci]) (cinfo, compptr,
	input__buf[ci] + (*in__row__group__ctr * upsample->rowgroup__height[ci]),
	upsample->color__buf + ci);
    }
    upsample->next__row__out = 0;
  }

  /* Color-convert and emit rows */

  /* How many we have in the buffer: */
  num__rows = (JDIMENSION) (cinfo->max__v__samp__factor - upsample->next__row__out);
  /* Not more than the distance to the end of the image.  Need this test
   * in case the image height is not a multiple of max__v__samp__factor:
   */
  if (num__rows > upsample->rows__to__go) 
    num__rows = upsample->rows__to__go;
  /* And not more than what the client can accept: */
  out__rows__avail -= *out__row__ctr;
  if (num__rows > out__rows__avail)
    num__rows = out__rows__avail;

  (*cinfo->cconvert->color__convert) (cinfo, upsample->color__buf,
				     (JDIMENSION) upsample->next__row__out,
				     output__buf + *out__row__ctr,
				     (int) num__rows);

  /* Adjust counts */
  *out__row__ctr += num__rows;
  upsample->rows__to__go -= num__rows;
  upsample->next__row__out += num__rows;
  /* When the buffer is emptied, declare this input row group consumed */
  if (upsample->next__row__out >= cinfo->max__v__samp__factor)
    (*in__row__group__ctr)++;
}


/*
 * These are the routines invoked by sep__upsample to upsample pixel values
 * of a single component.  One row group is processed per call.
 */


/*
 * For full-size components, we just make color__buf[ci] point at the
 * input buffer, and thus avoid copying any data.  Note that this is
 * safe only because sep__upsample doesn't declare the input row group
 * "consumed" until we are done color converting and emitting it.
 */

METHODDEF(void)
fullsize__upsample (j__decompress__ptr cinfo, jpeg__component__info * compptr,
		   JSAMPARRAY input__data, JSAMPARRAY * output__data__ptr)
{
  *output__data__ptr = input__data;
}


/*
 * This is a no-op version used for "uninteresting" components.
 * These components will not be referenced by color conversion.
 */

METHODDEF(void)
noop__upsample (j__decompress__ptr cinfo, jpeg__component__info * compptr,
	       JSAMPARRAY input__data, JSAMPARRAY * output__data__ptr)
{
  *output__data__ptr = NULL;	/* safety check */
}


/*
 * This version handles any integral sampling ratios.
 * This is not used for typical JPEG files, so it need not be fast.
 * Nor, for that matter, is it particularly accurate: the algorithm is
 * simple replication of the input pixel onto the corresponding output
 * pixels.  The hi-falutin sampling literature refers to this as a
 * "box filter".  A box filter tends to introduce visible artifacts,
 * so if you are actually going to use 3:1 or 4:1 sampling ratios
 * you would be well advised to improve this code.
 */

METHODDEF(void)
int__upsample (j__decompress__ptr cinfo, jpeg__component__info * compptr,
	      JSAMPARRAY input__data, JSAMPARRAY * output__data__ptr)
{
  my__upsample__ptr upsample = (my__upsample__ptr) cinfo->upsample;
  JSAMPARRAY output__data = *output__data__ptr;
  register JSAMPROW inptr, outptr;
  register JSAMPLE invalue;
  register int h;
  JSAMPROW outend;
  int h__expand, v__expand;
  int inrow, outrow;

  h__expand = upsample->h__expand[compptr->component__index];
  v__expand = upsample->v__expand[compptr->component__index];

  inrow = outrow = 0;
  while (outrow < cinfo->max__v__samp__factor) {
    /* Generate one output row with proper horizontal expansion */
    inptr = input__data[inrow];
    outptr = output__data[outrow];
    outend = outptr + cinfo->output__width;
    while (outptr < outend) {
      invalue = *inptr++;	/* don't need GETJSAMPLE() here */
      for (h = h__expand; h > 0; h--) {
	*outptr++ = invalue;
      }
    }
    /* Generate any additional output rows by duplicating the first one */
    if (v__expand > 1) {
      jcopy__sample__rows(output__data, outrow, output__data, outrow+1,
			v__expand-1, cinfo->output__width);
    }
    inrow++;
    outrow += v__expand;
  }
}


/*
 * Fast processing for the common case of 2:1 horizontal and 1:1 vertical.
 * It's still a box filter.
 */

METHODDEF(void)
h2v1__upsample (j__decompress__ptr cinfo, jpeg__component__info * compptr,
	       JSAMPARRAY input__data, JSAMPARRAY * output__data__ptr)
{
  JSAMPARRAY output__data = *output__data__ptr;
  register JSAMPROW inptr, outptr;
  register JSAMPLE invalue;
  JSAMPROW outend;
  int inrow;

  for (inrow = 0; inrow < cinfo->max__v__samp__factor; inrow++) {
    inptr = input__data[inrow];
    outptr = output__data[inrow];
    outend = outptr + cinfo->output__width;
    while (outptr < outend) {
      invalue = *inptr++;	/* don't need GETJSAMPLE() here */
      *outptr++ = invalue;
      *outptr++ = invalue;
    }
  }
}


/*
 * Fast processing for the common case of 2:1 horizontal and 2:1 vertical.
 * It's still a box filter.
 */

METHODDEF(void)
h2v2__upsample (j__decompress__ptr cinfo, jpeg__component__info * compptr,
	       JSAMPARRAY input__data, JSAMPARRAY * output__data__ptr)
{
  JSAMPARRAY output__data = *output__data__ptr;
  register JSAMPROW inptr, outptr;
  register JSAMPLE invalue;
  JSAMPROW outend;
  int inrow, outrow;

  inrow = outrow = 0;
  while (outrow < cinfo->max__v__samp__factor) {
    inptr = input__data[inrow];
    outptr = output__data[outrow];
    outend = outptr + cinfo->output__width;
    while (outptr < outend) {
      invalue = *inptr++;	/* don't need GETJSAMPLE() here */
      *outptr++ = invalue;
      *outptr++ = invalue;
    }
    jcopy__sample__rows(output__data, outrow, output__data, outrow+1,
		      1, cinfo->output__width);
    inrow++;
    outrow += 2;
  }
}


/*
 * Fancy processing for the common case of 2:1 horizontal and 1:1 vertical.
 *
 * The upsampling algorithm is linear interpolation between pixel centers,
 * also known as a "triangle filter".  This is a good compromise between
 * speed and visual quality.  The centers of the output pixels are 1/4 and 3/4
 * of the way between input pixel centers.
 *
 * A note about the "bias" calculations: when rounding fractional values to
 * integer, we do not want to always round 0.5 up to the next integer.
 * If we did that, we'd introduce a noticeable bias towards larger values.
 * Instead, this code is arranged so that 0.5 will be rounded up or down at
 * alternate pixel locations (a simple ordered dither pattern).
 */

METHODDEF(void)
h2v1__fancy__upsample (j__decompress__ptr cinfo, jpeg__component__info * compptr,
		     JSAMPARRAY input__data, JSAMPARRAY * output__data__ptr)
{
  JSAMPARRAY output__data = *output__data__ptr;
  register JSAMPROW inptr, outptr;
  register int invalue;
  register JDIMENSION colctr;
  int inrow;

  for (inrow = 0; inrow < cinfo->max__v__samp__factor; inrow++) {
    inptr = input__data[inrow];
    outptr = output__data[inrow];
    /* Special case for first column */
    invalue = GETJSAMPLE(*inptr++);
    *outptr++ = (JSAMPLE) invalue;
    *outptr++ = (JSAMPLE) ((invalue * 3 + GETJSAMPLE(*inptr) + 2) >> 2);

    for (colctr = compptr->downsampled__width - 2; colctr > 0; colctr--) {
      /* General case: 3/4 * nearer pixel + 1/4 * further pixel */
      invalue = GETJSAMPLE(*inptr++) * 3;
      *outptr++ = (JSAMPLE) ((invalue + GETJSAMPLE(inptr[-2]) + 1) >> 2);
      *outptr++ = (JSAMPLE) ((invalue + GETJSAMPLE(*inptr) + 2) >> 2);
    }

    /* Special case for last column */
    invalue = GETJSAMPLE(*inptr);
    *outptr++ = (JSAMPLE) ((invalue * 3 + GETJSAMPLE(inptr[-1]) + 1) >> 2);
    *outptr++ = (JSAMPLE) invalue;
  }
}


/*
 * Fancy processing for the common case of 2:1 horizontal and 2:1 vertical.
 * Again a triangle filter; see comments for h2v1 case, above.
 *
 * It is OK for us to reference the adjacent input rows because we demanded
 * context from the main buffer controller (see initialization code).
 */

METHODDEF(void)
h2v2__fancy__upsample (j__decompress__ptr cinfo, jpeg__component__info * compptr,
		     JSAMPARRAY input__data, JSAMPARRAY * output__data__ptr)
{
  JSAMPARRAY output__data = *output__data__ptr;
  register JSAMPROW inptr0, inptr1, outptr;
#if BITS__IN__JSAMPLE == 8
  register int thiscolsum, lastcolsum, nextcolsum;
#else
  register INT32 thiscolsum, lastcolsum, nextcolsum;
#endif
  register JDIMENSION colctr;
  int inrow, outrow, v;

  inrow = outrow = 0;
  while (outrow < cinfo->max__v__samp__factor) {
    for (v = 0; v < 2; v++) {
      /* inptr0 points to nearest input row, inptr1 points to next nearest */
      inptr0 = input__data[inrow];
      if (v == 0)		/* next nearest is row above */
	inptr1 = input__data[inrow-1];
      else			/* next nearest is row below */
	inptr1 = input__data[inrow+1];
      outptr = output__data[outrow++];

      /* Special case for first column */
      thiscolsum = GETJSAMPLE(*inptr0++) * 3 + GETJSAMPLE(*inptr1++);
      nextcolsum = GETJSAMPLE(*inptr0++) * 3 + GETJSAMPLE(*inptr1++);
      *outptr++ = (JSAMPLE) ((thiscolsum * 4 + 8) >> 4);
      *outptr++ = (JSAMPLE) ((thiscolsum * 3 + nextcolsum + 7) >> 4);
      lastcolsum = thiscolsum; thiscolsum = nextcolsum;

      for (colctr = compptr->downsampled__width - 2; colctr > 0; colctr--) {
	/* General case: 3/4 * nearer pixel + 1/4 * further pixel in each */
	/* dimension, thus 9/16, 3/16, 3/16, 1/16 overall */
	nextcolsum = GETJSAMPLE(*inptr0++) * 3 + GETJSAMPLE(*inptr1++);
	*outptr++ = (JSAMPLE) ((thiscolsum * 3 + lastcolsum + 8) >> 4);
	*outptr++ = (JSAMPLE) ((thiscolsum * 3 + nextcolsum + 7) >> 4);
	lastcolsum = thiscolsum; thiscolsum = nextcolsum;
      }

      /* Special case for last column */
      *outptr++ = (JSAMPLE) ((thiscolsum * 3 + lastcolsum + 8) >> 4);
      *outptr++ = (JSAMPLE) ((thiscolsum * 4 + 7) >> 4);
    }
    inrow++;
  }
}


/*
 * Module initialization routine for upsampling.
 */

GLOBAL(void)
jinit__upsampler (j__decompress__ptr cinfo)
{
  my__upsample__ptr upsample;
  int ci;
  jpeg__component__info * compptr;
  CH_BOOL need__buffer, do__fancy;
  int h__in__group, v__in__group, h__out__group, v__out__group;

  upsample = (my__upsample__ptr)
    (*cinfo->mem->alloc__small) ((j__common__ptr) cinfo, JPOOL__IMAGE,
				SIZEOF(my__upsampler));
  cinfo->upsample = (struct jpeg__upsampler *) upsample;
  upsample->pub.start__pass = start__pass__upsample;
  upsample->pub.upsample = sep__upsample;
  upsample->pub.need__context__rows = FALSE; /* until we find out differently */

  if (cinfo->CCIR601__sampling)	/* this isn't supported */
    ERREXIT(cinfo, JERR__CCIR601__NOTIMPL);

  /* jdmainct.c doesn't support context rows when min__DCT__scaled__size = 1,
   * so don't ask for it.
   */
  do__fancy = cinfo->do__fancy__upsampling && cinfo->min__DCT__scaled__size > 1;

  /* Verify we can handle the sampling factors, select per-component methods,
   * and create storage as needed.
   */
  for (ci = 0, compptr = cinfo->comp__info; ci < cinfo->num__components;
       ci++, compptr++) {
    /* Compute size of an "input group" after IDCT scaling.  This many samples
     * are to be converted to max__h__samp__factor * max__v__samp__factor pixels.
     */
    h__in__group = (compptr->h__samp__factor * compptr->DCT__scaled__size) /
		 cinfo->min__DCT__scaled__size;
    v__in__group = (compptr->v__samp__factor * compptr->DCT__scaled__size) /
		 cinfo->min__DCT__scaled__size;
    h__out__group = cinfo->max__h__samp__factor;
    v__out__group = cinfo->max__v__samp__factor;
    upsample->rowgroup__height[ci] = v__in__group; /* save for use later */
    need__buffer = TRUE;
    if (! compptr->component__needed) {
      /* Don't bother to upsample an uninteresting component. */
      upsample->methods[ci] = noop__upsample;
      need__buffer = FALSE;
    } else if (h__in__group == h__out__group && v__in__group == v__out__group) {
      /* Fullsize components can be processed without any work. */
      upsample->methods[ci] = fullsize__upsample;
      need__buffer = FALSE;
    } else if (h__in__group * 2 == h__out__group &&
	       v__in__group == v__out__group) {
      /* Special cases for 2h1v upsampling */
      if (do__fancy && compptr->downsampled__width > 2)
	upsample->methods[ci] = h2v1__fancy__upsample;
      else
	upsample->methods[ci] = h2v1__upsample;
    } else if (h__in__group * 2 == h__out__group &&
	       v__in__group * 2 == v__out__group) {
      /* Special cases for 2h2v upsampling */
      if (do__fancy && compptr->downsampled__width > 2) {
	upsample->methods[ci] = h2v2__fancy__upsample;
	upsample->pub.need__context__rows = TRUE;
      } else
	upsample->methods[ci] = h2v2__upsample;
    } else if ((h__out__group % h__in__group) == 0 &&
	       (v__out__group % v__in__group) == 0) {
      /* Generic integral-factors upsampling method */
      upsample->methods[ci] = int__upsample;
      upsample->h__expand[ci] = (UINT8) (h__out__group / h__in__group);
      upsample->v__expand[ci] = (UINT8) (v__out__group / v__in__group);
    } else
      ERREXIT(cinfo, JERR__FRACT__SAMPLE__NOTIMPL);
    if (need__buffer) {
      upsample->color__buf[ci] = (*cinfo->mem->alloc__sarray)
	((j__common__ptr) cinfo, JPOOL__IMAGE,
	 (JDIMENSION) jround__up((long) cinfo->output__width,
				(long) cinfo->max__h__samp__factor),
	 (JDIMENSION) cinfo->max__v__samp__factor);
    }
  }
}
